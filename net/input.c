#include "ns.h"

extern union Nsipc nsipcbuf;
uint32_t va = 0x0fffa000;
union Nsipc *ipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	uint32_t len;
	int perm, r;

	perm = PTE_P | PTE_U | PTE_W;

	while(1){
		while((r = sys_receive_packet((void *)va, &len)) == -E_RX_QUEUE_FULL)
			sys_yield();

		if(r < 0){
			panic("input failed: %e\n", r);
		}

		ipcbuf = (union Nsipc *)va;
		ipcbuf->pkt.jp_len = len;

		ipc_send(ns_envid, NSREQ_INPUT, (void *)va, perm);
	}
}