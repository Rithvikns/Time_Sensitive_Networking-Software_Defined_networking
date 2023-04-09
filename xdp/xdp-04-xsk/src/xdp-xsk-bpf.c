#include <stddef.h>
#include <stdint.h>

#include <linux/bpf.h>
//#include <linux/if_ether.h>
//#include <linux/types.h>

#include <bpf/bpf_helpers.h>

// This map is required by an XSK program to indicate, which RX queues should be redirected
// to an XSK (socket of type AF_XDP bound by the application). 
struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 64); // assume there are at maximum this number of RX queues for a network device
	__type(key, sizeof(uint32_t)); 
	__type(value, sizeof(uint32_t));
} xsk_map SEC(".maps");


SEC("xdp-xsk")
int xdp_prog_main(struct xdp_md *ctx)
{
	int index = ctx->rx_queue_index;
	
	// Check whether an XSK (socket of type AF_XDP) has been bound to RX queue of the device
	// from which the current packet has been received.
	if (bpf_map_lookup_elem(&xsk_map, &index))
		return bpf_redirect_map(&xsk_map, index, 0);

	// No XSK bound for this RX queue -> pass on packet to network stack
	return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
