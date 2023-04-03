#ifndef DROP_AND_COUNT_COMMONS_H
#define DROP_AND_COUNT_COMMONS_H

#include <linux/if_ether.h>

// A hash map, storing for each destination MAC address the number of dropped and passed packets.
struct hash_map_key {
	char addr[ETH_ALEN]; // MAC address with 6 bytes
};

struct hash_map_value {
	struct bpf_spin_lock lock; // spin lock for atomic reads and updates
	uint64_t dropped; // number of dropped packets for this mac.
	uint64_t t_lastdrop; // time when last packet was dropped in nano-sconds since UNIX epoche.
};

#endif
