#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <net/if.h>
#include <linux/if_link.h>

#include <common_defines.h>
#include <common_user_bpf_xdp.h>

#include "xdp-drop_and_count-commons.h"

#define EXIT_OK 0
#define EXIT_FAIL_BPFLOAD 1
#define EXIT_FAIL_FINDMAP 2
#define EXIT_FAIL_FINDELEM 3
#define EXIT_FAIL_USAGE 4
#define EXIT_FAIL_DEVICE 5

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

void print_mac(char *addr, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		printf("%02x", addr[i]);
		if (i+1 != len)
			printf(":");
	}
}

void print_stats_per_mac(int stats_per_mac_map_fd)
{
	// We can iterate through all keys of the hash map as follows:
	// 1. Set current key to NULL for first query.
	struct hash_map_key next_key;
	struct hash_map_value value;
	if (bpf_map_get_next_key(stats_per_mac_map_fd, NULL, &next_key) != 0)
		return; // empty hash map

	do {
		struct hash_map_key current_key = next_key;
		// Lock entry for reading element atomically.
		bpf_map_lookup_elem_flags(stats_per_mac_map_fd, &current_key, &value, BPF_F_LOCK);
		print_mac(current_key.addr, ETH_ALEN);
		printf(" -> drop count=%lu    t last drop=%lu \n", value.dropped, value.t_lastdrop);
	} while (bpf_map_get_next_key(stats_per_mac_map_fd, NULL, &next_key) == 0);
	
}

int poll_stats(int stats_map_fd, int stats_per_mac_map_fd)
{
	__u32 key = 0; // first and only key in map
	
	uint64_t drop_cnt;
	while (!do_exit) {
		// Note that the next call involves a system call that *copies*
		// the value of the requested data. Since data is updated on the kernel-side
		// with an atomic operation, we do not need to lock the element here.
		if (bpf_map_lookup_elem(stats_map_fd, &key, &drop_cnt) != 0)
			return EXIT_FAIL_FINDELEM;
		
		printf("Total drop count: %lu\n", drop_cnt);

		print_stats_per_mac(stats_per_mac_map_fd);
		
		sleep(1); // sleep one sec.
	}

	return EXIT_OK;
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

int main(int argc, char *argv[])
{
	struct config cfg = {
		.xdp_flags = XDP_FLAGS_UPDATE_IF_NOEXIST | // fail if BPF program already exists
		XDP_FLAGS_DRV_MODE, // run BPF program in native driver rather than in generic mode 
		.ifindex   = -1, // network device will be defined by program arguments and mapped to interface index
		.do_unload = false,
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
        signal(SIGINT, sigint_handler);
	
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

	// Get file descriptor of map "xdp_drop_stats_map".
	int stats_map_fd = get_map_fd(bpf_obj, "xdp_drop_stats_map");
	if (stats_map_fd < 0) {
		xdp_link_detach(cfg.ifindex, cfg.xdp_flags, 0);
		fprintf(stderr, "Could not find map\n");
		return EXIT_FAIL_FINDMAP;
	}

	// Get file descriptor of map "xdp_stats_per_mac_map".
	int stats_per_mac_map_fd = get_map_fd(bpf_obj, "xdp_stats_per_mac_map");
	if (stats_per_mac_map_fd < 0) {
		xdp_link_detach(cfg.ifindex, cfg.xdp_flags, 0);
		fprintf(stderr, "Could not find map\n");
		return EXIT_FAIL_FINDMAP;
	}
	
	// Poll for new statistics values until user terminates program.
	int exitcode = poll_stats(stats_map_fd, stats_per_mac_map_fd);

	// Detach XDP program from interface using libbpf.
	xdp_link_detach(cfg.ifindex, cfg.xdp_flags, 0);

	return exitcode;
}
