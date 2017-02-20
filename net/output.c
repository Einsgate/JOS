#include "ns.h"

extern union Nsipc nsipcbuf;
union Nsipc *nsoutreq = (union Nsipc *)0x0ffff000;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver

	uint32_t req, whom, len;
	int perm, r;
	void *pg, *va;

	while (1) {
		perm = 0;
		req = ipc_recv((int32_t *) &whom, nsoutreq, &perm);

		if(whom != ns_envid){
			cprintf("Invalid request from %08x: Wrong sender.\n",
				whom);
			continue;
		}

		// All requests must contain an argument page
		if (!(perm & PTE_P)) {
			cprintf("Invalid request from %08x: no argument page\n",
				whom);
			continue; // just leave it hanging...
		}

		pg = NULL;
		if (req == NSREQ_OUTPUT) {
			if((len = (uint32_t)(nsoutreq->pkt.jp_len)) > PGSIZE-sizeof(int)){
				r = -E_TX_BAD_LEN;
			}
			else {
				va = nsoutreq->pkt.jp_data;
				//cprintf("va = %x\n", (int32_t)va);
				while((r = sys_transmit_packet(va, len)) == -E_TX_QUEUE_FULL)
					sys_yield();
			}
		} 
		else {
			cprintf("Invalid request code %d from %08x\n", req, whom);
			r = -E_INVAL;
		}
		//ipc_send(ns_envid, r, pg, perm);
		sys_page_unmap(0, nsoutreq);
	}
}
