#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <net/if.h>
#include <linux/if_link.h>

#include <common_defines.h>
#include <common_user_bpf_xdp.h>

#define EXIT_OK 0
#define EXIT_FAIL_BPFLOAD 1
#define EXIT_FAIL_FINDMAP 2
#define EXIT_FAIL_FINDELEM 3
#define EXIT_FAIL_USAGE 4
#define EXIT_FAIL_DEVICE 5

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

int poll_stats(int map_fd)
{
	__u32 key = 0; // first and only key in map
	
	uint64_t drop_cnt;
	while (1) {
		if (bpf_map_lookup_elem(map_fd, &key, &drop_cnt) != 0)
			return EXIT_FAIL_FINDELEM;
		
		printf("Drop count: %lu\n", drop_cnt);
		
		sleep(1); // sleep one sec.
	}
}

static void usage(const char *prog)
{
	fprintf(stderr, "%s "
		"-f BPF_PROG_FILENAME "
		"-d DEVICE "
		"\n", prog);
}

int main(int argc, char **argv)
{
	struct config cfg = {
		.xdp_flags =
		XDP_FLAGS_UPDATE_IF_NOEXIST // fail if BPF program already exists
		| XDP_FLAGS_DRV_MODE, // run BPF program in native driver rather than in generic mode 
		.ifindex   = -1,
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
	
	if ( (cfg.ifindex = if_nametoindex(cfg.ifname)) == 0) {
		perror("Could not get interface index");
		return EXIT_FAIL_DEVICE;
	}

	// Load BPF program using libbpf.
	struct bpf_object *bpf_obj = load_bpf_and_xdp_attach(&cfg);
	if (!bpf_obj) {
		fprintf(stderr, "Could not load and attach BPF program\n");
		return EXIT_FAIL_BPFLOAD;
	}

	// Get file descriptor of map "xdp_drop_stats_map" created by BPF program. 
	int statsmap_fd = get_map_fd(bpf_obj, "xdp_drop_stats_map");
	if (statsmap_fd < 0) {
		xdp_link_detach(cfg.ifindex, cfg.xdp_flags, 0);
		fprintf(stderr, "Could not find map\n");
		return EXIT_FAIL_FINDMAP;
	}

	// Poll for new stats values.
	return poll_stats(statsmap_fd);
}
