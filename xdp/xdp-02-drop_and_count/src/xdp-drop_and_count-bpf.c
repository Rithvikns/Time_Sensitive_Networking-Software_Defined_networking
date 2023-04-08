#include <stddef.h>
#include <stdint.h>

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/types.h>

#include <bpf/bpf_helpers.h>

#include "xdp-drop_and_count-commons.h"

// Packets to this destination MAC address will be dropped; all others pass
static const char eth_d_addr[] = {0x00,0x00,0x00,0x00,0x00,0x00};

// Deprecated map definition (just for information)
/*
struct bpf_map_def SEC("maps") xdp_stats_map = {
  .type        = BPF_MAP_TYPE_ARRAY,
  .key_size    = sizeof(uint32_t),
  .value_size  = sizeof(uint64_t),
  .max_entries = 1,
};
*/


// A simple counter counting all drop packets.
struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, sizeof(uint32_t)); // for arrays, the key is always a 32 bit uint 
	__type(value, sizeof(uint64_t));
} xdp_drop_stats_map SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 1024);
	__type(key, sizeof(struct hash_map_key)); // keys can be arbitrary structs for hash maps
	__type(value, sizeof(struct hash_map_value)); // values can be arbitrary structs for hash maps 
} xdp_stats_per_mac_map SEC(".maps");

static __always_inline int make_drop_decision_eth(void *hdr,
						  void *endptr)
{
	int do_drop = 0;
	struct ethhdr *eth_hdr = hdr;
	
	// Bounds check
	if (hdr + sizeof(struct ethhdr) > endptr)
		return 0;
	
	do_drop = 1; // assume MAC addresses match and we drop
	for (size_t i = 0; i < ETH_ALEN; i++) { 
		if (eth_hdr->h_dest[i] != eth_d_addr[i]) {
			// MAC address does not match -> don't drop (pass)
			do_drop = 0;
			break;
		}
	}
	
	return do_drop;
}

static __always_inline void update_mac_stats(void *hdr, void *endptr, int do_drop)
{
	struct ethhdr *eth_hdr = hdr;
	
	// Bounds check
	if (hdr + sizeof(struct ethhdr) > endptr)
		return;

	struct hash_map_key key;
	// Need to use built-in memcpy function of LLVM.
	__builtin_memcpy(&key.addr, eth_hdr->h_dest, sizeof(key.addr));
	
	struct hash_map_value *value = bpf_map_lookup_elem(&xdp_stats_per_mac_map, &key);

	// Note that we get a pointer to the actual element in memory, which is edited in-place.
	// To avoid race conditions on this struct, we must use a spin lock to ensure
	// atomic updates.
	bpf_spin_lock(&value->lock);
	if (do_drop) {
		(value->dropped)++;
		value->t_lastdrop = bpf_ktime_get_ns();
	}
	bpf_spin_unlock(&value->lock);
}
	
SEC("xdp-drop")
int xdp_prog_main(struct xdp_md *ctx)
{
	void *pkt_end = (void *)(long)ctx->data_end;
	void *pkt = (void *)(long)ctx->data;
	
	uint32_t key = 0; // drop counter is the first and only entry in the array
	// Note that we get a pointer to the actual element in memory, which is edited in-place.
	// To avoid race conditions, atomic operations or spin locks must be used.
	// For incrementing a single 64-bit integer, a single atomic operation is available.
	uint64_t *drop_cnt = bpf_map_lookup_elem(&xdp_drop_stats_map, &key);
	if (drop_cnt == NULL)
		return XDP_PASS;

	int do_drop = make_drop_decision_eth(pkt, pkt_end);
	
        // Update statistics for destination MAC.
	update_mac_stats(pkt, pkt_end, do_drop);

	if (do_drop) {
		// Increase global drop counter atomically.
		// LLVM will map this to the atomic add operation in BPF.
		__sync_fetch_and_add(drop_cnt, 1);
		return XDP_DROP;
	} else {
		return XDP_PASS;
	}
}

char _license[] SEC("license") = "GPL";
