#include <kern/e1000.h>

#define test	0

uint32_t firstround = 1;
volatile uint32_t *e1000;
struct e1000_tx_desc *tsmitqueue;
// LAB 6: Your driver code here
int 
pci_e1000_attach(struct pci_func *pcif)
{
	pci_func_enable(pcif);

	e1000 = (uint32_t *)mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	//test
	if(test)
		cprintf("e1000 device status register: 0x%x\n", e1000[E1000_STATUS]);

	//Initialize transmit
	init_transmit();

	if(test){
		int i;
		//assert(transmit_packet(PADDR(kern_pgdir), 1024) == 0);
		//assert(transmit_packet(PADDR(kern_pgdir) + 1024, 1024) == 0);
		//assert(transmit_packet(PADDR(kern_pgdir) + 2048, 1024) == 0);
		//assert(transmit_packet(PADDR(kern_pgdir) + 2048, 2048) == 0);
		for(i = 0; i < 512; i++)
			assert(transmit_packet(PADDR(kern_pgdir), 1024) == 0);
	}

	return 0;
}



void 
init_transmit()
{
	struct PageInfo *pp;
	
	assert(PGSIZE % TDLEN_ALIGN == 0);
	if((pp = page_alloc(0)) == NULL)
		panic("init_transmit failed: No memory.\n");

	tsmitqueue = (struct e1000_tx_desc *)page2kva(pp);
	e1000[E1000_TDBAL] = PADDR(tsmitqueue);
	e1000[E1000_TDBAH] = 0;
	e1000[E1000_TDLEN] = PGSIZE;
	e1000[E1000_TDH] = 0;
	e1000[E1000_TDT] = 0;
	e1000[E1000_TCTL] |= (E1000_TCTL_EN | E1000_TCTL_PSP);
	//e1000[E1000_TCTL] &= (~E1000_TCTL_CT);
	//e1000[E1000_TCTL] |= (0x10 << E1000_TCTL_CT_SHIFT);
	FIXSEG(e1000[E1000_TCTL], E1000_TCTL_CT, E1000_TCTL_CT_SHIFT, 0x10);
	//e1000[E1000_TCTL] &= (~E1000_TCTL_COLD);
	//e1000[E1000_TCTL] |= (0x40 << E1000_TCTL_COLD_SHIFT);
	FIXSEG(e1000[E1000_TCTL], E1000_TCTL_COLD, E1000_TCTL_COLD_SHIFT, 0x40);
	if(test)
		cprintf("e1000 transmit descriptor control register: 0x%x\n", e1000[E1000_TCTL]);
//if(test)
	//	cprintf("e1000 transmit Inter-packet gap register: 0x%x\n", e1000[E1000_TIPG]);
	FIXSEG(e1000[E1000_TIPG], E1000_TIPG_IPGT, E1000_TIPG_IPGT_SHIFT, 10);
	//e1000[E1000_TIPG] |= 0xa;
	//if(test)
	//	cprintf("e1000 transmit Inter-packet gap register: 0x%x\n", e1000[E1000_TIPG]);
	FIXSEG(e1000[E1000_TIPG], E1000_TIPG_IPGR1, E1000_TIPG_IPGR1_SHIFT, 8);
	FIXSEG(e1000[E1000_TIPG], E1000_TIPG_IPGR2, E1000_TIPG_IPGR2_SHIFT, 6);
	FIXSEG(e1000[E1000_TIPG], E1000_TIPG_RESV, E1000_TIPG_RESV_SHIFT, 0);
	if(test)
		cprintf("e1000 transmit Inter-packet gap register: 0x%x\n", e1000[E1000_TIPG]);

}

int 
transmit_packet(uint32_t pa, uint32_t len)
{
	struct e1000_tx_desc *desc = tsmitqueue + e1000[E1000_TDT];

	if(firstround == 0 && (desc->upper.fields & E1000_TXD_STAT_DD) == 0)
		return -E_TX_QUEUE_FULL;
	if(len < PACKET_MIN_LEN || len > PACKET_MAX_LEN)
		return -E_TX_BAD_LEN;

	desc->buffer_addr = (uint64_t)pa;
	desc->lower.length = (uint16_t)len;
	//set cmd RS
	desc->lower.flags |= E1000_TXD_CMD_RS;

	//update TDT
	if((e1000[E1000_TDT] + 1) * sizeof(struct e1000_tx_desc) >= e1000[E1000_TDLEN]){
		firstround = 0;
		e1000[E1000_TDT] = 0;
	}
	else
		e1000[E1000_TDT] += 1;

	return 0;
}