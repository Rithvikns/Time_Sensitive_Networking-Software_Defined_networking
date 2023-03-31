#include <stddef.h>
#include <stdint.h>

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/types.h>

#include <bpf/bpf_helpers.h>

static const char eth_d_addr[] = {0x00,0x00,0x00,0x00,0x00,0x00};

// Deprecated map definition
/*
struct bpf_map_def SEC("maps") xdp_stats_map = {
  .type        = BPF_MAP_TYPE_ARRAY,
  .key_size    = sizeof(uint32_t),
  .value_size  = sizeof(uint64_t),
  .max_entries = 1,
};
*/

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, sizeof(uint32_t));
	__type(value, sizeof(uint64_t));
} xdp_drop_stats_map SEC(".maps");

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

SEC("xdp-drop")
int xdp_prog_main(struct xdp_md *ctx)
{
	void *data_end = (void *)(long)ctx->data_end;
	void *data = (void *)(long)ctx->data;
	
	uint32_t key = 0; // drop counter is the first and only entry in the array
	uint64_t *drop_cnt = bpf_map_lookup_elem(&xdp_drop_stats_map, &key);
	if (drop_cnt == NULL)
		return XDP_PASS;
	
	if (make_drop_decision_eth(data, data_end)) {
		// Increase packet drops counter atomically.
		// LLVM will map this to the atomic add operation in BPF.
		__sync_fetch_and_add(drop_cnt, 1);
		return XDP_DROP;
	} else {
		return XDP_PASS;
	}
}

char _license[] SEC("license") = "GPL";
