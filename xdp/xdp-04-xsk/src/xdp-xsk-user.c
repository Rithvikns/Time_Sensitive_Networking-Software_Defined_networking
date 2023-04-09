#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <poll.h>
#include <assert.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <bpf/xsk.h>

#include <net/if.h>
#include <linux/if_link.h>
#include <linux/if_ether.h>

#include <common_defines.h>
#include <common_user_bpf_xdp.h>

#define EXIT_OK 0
#define EXIT_FAIL_BPFLOAD 1
#define EXIT_FAIL_FINDMAP 2
#define EXIT_FAIL_FINDELEM 3
#define EXIT_FAIL_USAGE 4
#define EXIT_FAIL_DEVICE 5
#define EXIT_FAIL_SIGNAL 6
#define EXIT_FAIL_MEMALLOC 7
#define EXIT_FAIL_SOCKET 8

#define NUM_FRAMES 4096
#define FRAME_SIZE XSK_UMEM__DEFAULT_FRAME_SIZE
#define CACHE_LINE_SIZE 64

#define INVALID_UMEM_FRAME UINT64_MAX

#define RX_BATCH_SIZE 16

struct xsk_umem_info {
        struct xsk_ring_prod fq;
        struct xsk_ring_cons cq;
        struct xsk_umem *umem;
        void *buffer;
};

struct stats_record {
        uint64_t timestamp;
        uint64_t rx_packets;
        uint64_t rx_bytes;
        uint64_t tx_packets;
        uint64_t tx_bytes;
};

struct xsk_socket_info {
        struct xsk_ring_cons rx;
        struct xsk_ring_prod tx;
        struct xsk_umem_info *umem;
        struct xsk_socket *xsk;

	uint64_t umem_frame_addr[NUM_FRAMES];
        uint32_t umem_frame_free;

        uint32_t outstanding_tx;

        struct stats_record stats;
        struct stats_record prev_stats;
};

const size_t frame_buffer_size = NUM_FRAMES*FRAME_SIZE;

static int do_exit = 0;

int get_map_fd(struct bpf_object *bpf_obj, const char *maps_name)
{
	struct bpf_map *map;
	// Use libbpf to find map.
	map = bpf_object__find_map_by_name(bpf_obj, maps_name);
        if (map == NULL)
		return -1;

	// Use libbpf to get fd for map.
	return bpf_map__fd(map);
}

static void usage(const char *prog)
{
	fprintf(stderr, "%s "
		"-f BPF_PROG_FILENAME "
		"-d DEVICE "
		"\n", prog);
}

static void sigint_handler(int signal)
{
	do_exit = 1;
}

static struct xsk_umem_info *configure_xsk_umem(void *buffer, size_t size)
{
        struct xsk_umem_info *umem;
        int ret;

        umem = calloc(1, sizeof(struct xsk_umem_info));
        if (!umem)
                return NULL;

	// Call libbpf to create UMEM for fill ring (fq) and completion ring (cq)
	// backed by the allocated buffer.
        ret = xsk_umem__create(&umem->umem, buffer, size, &umem->fq, &umem->cq, NULL);
        if (ret)
                return NULL;
        umem->buffer = buffer;

        return umem;
}

static uint64_t xsk_alloc_umem_frame(struct xsk_socket_info *xsk)
{
        uint64_t frame;
        if (xsk->umem_frame_free == 0)
                return INVALID_UMEM_FRAME;

	// We keep a list of umem_frame_free free frames in umem_frame_addr.
	// Frames are taken from the tail of the list and are returned at the
	// tail of the list.
        frame = xsk->umem_frame_addr[--xsk->umem_frame_free];
        xsk->umem_frame_addr[xsk->umem_frame_free] = INVALID_UMEM_FRAME;

        return frame;
}

static void xsk_free_umem_frame(struct xsk_socket_info *xsk, uint64_t frame)
{
        assert(xsk->umem_frame_free < NUM_FRAMES);

        xsk->umem_frame_addr[xsk->umem_frame_free++] = frame;
}

static struct xsk_socket_info *xsk_configure_socket(struct config *cfg,
                                                    struct xsk_umem_info *umem)
{
        struct xsk_socket_config xsk_cfg;
        struct xsk_socket_info *xsk_info;
        uint32_t idx;
	//uint32_t prog_id = 0;
        int i;
        int ret;

        xsk_info = calloc(1, sizeof(*xsk_info));
        if (!xsk_info)
                return NULL;

	// Call libbpf to create XSK (socket of type AF_XDP).
	// The socket uses the given UMEM to populate RX and TX rings. 
        xsk_info->umem = umem;
        xsk_cfg.rx_size = XSK_RING_CONS__DEFAULT_NUM_DESCS;
        xsk_cfg.tx_size = XSK_RING_PROD__DEFAULT_NUM_DESCS;
        xsk_cfg.libbpf_flags = 0;
        xsk_cfg.xdp_flags = cfg->xdp_flags;
        xsk_cfg.bind_flags = cfg->xsk_bind_flags;
        ret = xsk_socket__create(&xsk_info->xsk, cfg->ifname,
                                 cfg->xsk_if_queue, umem->umem, &xsk_info->rx,
                                 &xsk_info->tx, &xsk_cfg);

        if (ret)
                return NULL;

	// Get ID of BPF program attached to link. 
	/*
        ret = bpf_get_link_xdp_id(cfg->ifindex, &prog_id, cfg->xdp_flags);
        if (ret)
                return NULL;
	*/
	
	// Partition the UMEM into array of frames.
        for (i = 0; i < NUM_FRAMES; i++)
                xsk_info->umem_frame_addr[i] = i * FRAME_SIZE;
        xsk_info->umem_frame_free = NUM_FRAMES;

	// Populate fill ring with frame addresses in a three-step process:
	// 1. Reserve slots in producer (fill) ring.
        unsigned int nreserved = xsk_ring_prod__reserve(&xsk_info->umem->fq,
							XSK_RING_PROD__DEFAULT_NUM_DESCS,
							&idx);
	// We expect to reserve all XSK_RING_PROD__DEFAULT_NUM_DESCS frames.
	// However, at least one is required to go on.
        if (nreserved < 1)
                return NULL;
	
	// 2. Store frame addresses in producer (fill) ring.
        for (i = 0; i < nreserved; i++)
                *xsk_ring_prod__fill_addr(&xsk_info->umem->fq, idx++) =
                        xsk_alloc_umem_frame(xsk_info);
	// 3. Submit to kernel, so BPF program now has control over this frames now.
        xsk_ring_prod__submit(&xsk_info->umem->fq, nreserved);

        return xsk_info;
}

void replenish_fill_ring(struct xsk_socket_info *xsk)
{
	unsigned int idx_fq = 0;
	size_t nreserved;
	
        // Replenish fill ring with as many frames as fit into fill ring.
        size_t nframes_free = xsk_prod_nb_free(&xsk->umem->fq,
					       xsk->umem_frame_free);
        if (nframes_free > 0) {
		// Free space in fill ring available to store frame addresses.
		// We should be able to reserve nframes_free frames.
		// But we need at least one to continue.
		do {
                        nreserved = xsk_ring_prod__reserve(&xsk->umem->fq, nframes_free, &idx_fq);
		} while (nreserved < 1);

                for (size_t i = 0; i < nreserved; i++)
                        *xsk_ring_prod__fill_addr(&xsk->umem->fq, idx_fq++) =
                                xsk_alloc_umem_frame(xsk);

                xsk_ring_prod__submit(&xsk->umem->fq, nreserved);
        }
}

void print_mac(unsigned char *addr, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		printf("%02x", addr[i]);
		if (i+1 != len)
			printf(":");
	}
}

void process_pkt(struct xsk_socket_info *xsk, uint64_t addr, uint32_t len)
{
	// "In the default aligned mode, you can get the addr variable straight
	// from the RX descriptor. But in unaligned mode, you need to use the
	// three last function below as the offset used is carried in the upper
	// 16 bits of the addr. Therefore, you cannot use the addr straight from
	// the descriptor in the unaligned case." [man libxdp]
	void *pkt = xsk_umem__get_data(xsk->umem->buffer, addr);
	struct ethhdr *eth = (struct ethhdr *) pkt;

	// Note: In contrast to packet processing within BPF program, no bounds
	// checks are mandatory here to access the data since are are in user-space.

	printf("Received packet from ");
	print_mac(eth->h_source, 6);
	printf("\n");
}

void process_pkts(struct xsk_socket_info *xsk)
{
        uint32_t idx_rx = 0;

	// When this function is called, there is at least one packet in the RX ring.
	// idx_rx will be the index of the first available packet.
	// RX_BATCH_SIZE defines how many packets we would like to have at maximum.
	// However, if fewer packets are available, the returned batch can be smaller.
	// Thus, rcvd will be less equal RX_BATCH_SIZE.
        size_t rcvd = xsk_ring_cons__peek(&xsk->rx, RX_BATCH_SIZE, &idx_rx);
        if (!rcvd)
		return;

	// Before we go on, replenish fill ring with new frames so kernel can go receiving.
	replenish_fill_ring(xsk);
	
        // Retrieve packet addresses from RX ring and process them.
        for (size_t i = 0; i < rcvd; i++) {
                uint64_t addr = xsk_ring_cons__rx_desc(&xsk->rx, idx_rx)->addr;
                uint32_t len = xsk_ring_cons__rx_desc(&xsk->rx, idx_rx)->len;
		idx_rx++;
		
                process_pkt(xsk, addr, len);

		// Packet has been processed. Return frame to UMEM pool.
		xsk_free_umem_frame(xsk, addr);
        }

	// Move RX pointer in RX ring after the processed packets.
        xsk_ring_cons__release(&xsk->rx, rcvd);
}

void receive_and_process_pkts(struct xsk_socket_info *xsk)
{
	struct pollfd fds[2];
        int ret, nfds = 1;

        memset(fds, 0, sizeof(fds));
	// xsk_socket__fd gets the file descriptor of the XSK, so we can use
	// standard I/O functions on these file descriptors.
        fds[0].fd = xsk_socket__fd(xsk->xsk);
        fds[0].events = POLLIN;
	
        while (!do_exit) {
		// Poll system call blocks process until one of the fill descriptors
		// in set fds can be read (POLLIN) w/o blocking.
		ret = poll(fds, nfds, -1);
		if (ret <= 0 || ret > 1)
			continue;
		// At least one packet is now in the RX ring of the XSK.
		process_pkts(xsk);
	}
}

int main(int argc, char *argv[])
{
	struct config cfg = {
		.xdp_flags =
		XDP_FLAGS_UPDATE_IF_NOEXIST | // Fail if BPF program already exists
		XDP_FLAGS_DRV_MODE,           // Run BPF program in native driver rather than in generic mode 
		.ifindex   = -1,              // Network device will be defined by program arguments and mapped to interface index
		.do_unload = false,           // Don't try to unload program first.
		.xsk_if_queue = 0,            // Redirect this RX queue to XSK.
	};
	cfg.ifname[0] = 0;
	cfg.filename[0] = 0;
	
	int opt;
	while ( (opt = getopt(argc, argv, "d:f:")) != -1 ) {
		switch(opt) {
		case 'd' :
			strncpy(cfg.ifname, optarg, IF_NAMESIZE);
			break;
		case 'f' :
			strncpy(cfg.filename, optarg, sizeof(cfg.filename));
			break;
		case ':' :
		case '?' :
		default :
			usage(argv[0]);
			return EXIT_FAIL_USAGE;
		}
	}

	if (strlen(cfg.ifname) == 0) {
		usage(argv[0]);
		return EXIT_FAIL_USAGE;
	}

	if (strlen(cfg.filename) == 0) {
		usage(argv[0]);
		return EXIT_FAIL_USAGE;
	}

	// User has specified all required options.

	// Catch SIGINT when user exits applications (Ctrl-C).
        if ( signal(SIGINT, sigint_handler) == SIG_ERR ) {
		perror("Could not attach signal handler");
		return EXIT_FAIL_SIGNAL;
	}
	
	if ( (cfg.ifindex = if_nametoindex(cfg.ifname)) == 0) {
		perror("Could not get interface index");
		return EXIT_FAIL_DEVICE;
	}

	// Load BPF program and attach it to network interface using libbpf.
	struct bpf_object *bpf_obj = load_bpf_and_xdp_attach(&cfg);
	if (!bpf_obj) {
		fprintf(stderr, "Could not load and attach BPF program\n");
		return EXIT_FAIL_BPFLOAD;
	}

	int xsk_map_fd = get_map_fd(bpf_obj, "xsk_map");
	if (xsk_map_fd < 0) {
		xdp_link_detach(cfg.ifindex, cfg.xdp_flags, 0);
		fprintf(stderr, "Could not find XSK map\n");
		return EXIT_FAIL_FINDMAP;
	}


	// Allocate memory for frames.
	void *frame_buffer;
	if (posix_memalign(&frame_buffer,
			   CACHE_LINE_SIZE, // memory aligned to CPU cache line size for faster access 
                           frame_buffer_size)) {
		perror("Could not allocate memory for frames");
		xdp_link_detach(cfg.ifindex, cfg.xdp_flags, 0);
                return EXIT_FAIL_MEMALLOC;
        }

        // Initialize UMEM (memory pool) shared between user space application and kernel
	// and used to transfer packets between user space (this application) and the
	// kernel (BPF program). 
        struct xsk_umem_info *umem = configure_xsk_umem(frame_buffer, frame_buffer_size);
        if (umem == NULL) {
                fprintf(stderr, "Could not create umem\n");
		xdp_link_detach(cfg.ifindex, cfg.xdp_flags, 0);
                exit(EXIT_FAILURE);
        }

        // Open and configure the AF_XDP (xsk) socket
        struct xsk_socket_info *xsk = xsk_configure_socket(&cfg, umem);
        if (xsk == NULL) {
                fprintf(stderr, "Could not create XSK socket\n");
		xdp_link_detach(cfg.ifindex, cfg.xdp_flags, 0);
                exit(EXIT_FAIL_SOCKET);
        }
	
	int exitcode = EXIT_OK;

	// Receive and process packets redirected to XSK.
	receive_and_process_pkts(xsk);
	
	// Detach XDP program from interface using libbpf.
	xdp_link_detach(cfg.ifindex, cfg.xdp_flags, 0);

	return exitcode;
}
