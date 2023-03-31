#include <stdio.h>

/*
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
*/

/*
#include <net/if.h>
#include <linux/if_link.h>
*/

#include "xdp-common.h"

#define EXIT_OK 0
#define EXIT_BPFLOADFAIL 1

int main(int argc, char **argv)
{
	/*
	  struct config cfg = {
	  .xdp_flags = XDP_FLAGS_UPDATE_IF_NOEXIST | XDP_FLAGS_DRV_MODE,
	  .ifindex   = -1,
	  .do_unload = false,
	  };
	  
	  struct bpf_object *bpf_obj = load_bpf_and_xdp_attach(&cfg);
	  if (!bpf_obj)
	  return EXIT_BPFLOADFAIL;
	*/
  
	return EXIT_OK;
}
