#include <kern/e1000.h>

#define test	0

uint32_t firstround_t = 1;
//uint32_t firstround_r = 1;
volatile uint32_t *e1000;
struct e1000_tx_desc *tsmitqueue;
struct e1000_rx_desc *recvqueue;
uint32_t receive_buf[PGSIZE/sizeof(struct e1000_rx_desc)];
// LAB 6: Your driver code here
int 
pci_e1000_attach(struct pci_func *pcif)
{
	pci_func_enable(pcif);

	e1000 = (uint32_t *)mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	//test
	if(test)
		cprintf("e1000 device status register: 0x%x\n", e1000[E1000_STATUS]);

	//Initialize receive and transmit
	init_receive();
	init_transmit();

	if(test){
		int i;
		//assert(transmit_packet(PADDR(kern_pgdir), 1024) == 0);
		//assert(transmit_packet(PADDR(kern_pgdir) + 1024, 1024) == 0);
		//assert(transmit_packet(PADDR(kern_pgdir) + 2048, 1024) == 0);
		//assert(transmit_packet(PADDR(kern_pgdir) + 2048, 2048) == 0);
		for(i = 0; i < 1; i++){
			assert(transmit_packet(PADDR(kern_pgdir), 1024) == 0);
			cprintf("paddr = %x\n", PADDR(kern_pgdir));
		}
	}

	return 0;
}



void 
init_transmit()
{
	struct PageInfo *pp;
	
	assert(PGSIZE % TDLEN_ALIGN == 0);
	if((pp = page_alloc(ALLOC_ZERO)) == NULL)
		panic("init_transmit failed: No memory.\n");

	tsmitqueue = (struct e1000_tx_desc *)page2kva(pp);
	e1000[E1000_TDBAL] = PADDR(tsmitqueue);
	e1000[E1000_TDBAH] = 0;
	e1000[E1000_TDLEN] = PGSIZE;
	e1000[E1000_TDH] = 0;
	e1000[E1000_TDT] = 0;
	e1000[E1000_TCTL] |= (E1000_TCTL_EN | E1000_TCTL_PSP);
	FIXSEG(e1000[E1000_TCTL], E1000_TCTL_CT, E1000_TCTL_CT_SHIFT, 0x10);
	FIXSEG(e1000[E1000_TCTL], E1000_TCTL_COLD, E1000_TCTL_COLD_SHIFT, 0x40);
	FIXSEG(e1000[E1000_TIPG], E1000_TIPG_IPGT, E1000_TIPG_IPGT_SHIFT, 10);
	FIXSEG(e1000[E1000_TIPG], E1000_TIPG_IPGR1, E1000_TIPG_IPGR1_SHIFT, 8);
	FIXSEG(e1000[E1000_TIPG], E1000_TIPG_IPGR2, E1000_TIPG_IPGR2_SHIFT, 6);
	FIXSEG(e1000[E1000_TIPG], E1000_TIPG_RESV, E1000_TIPG_RESV_SHIFT, 0);
}

int 
transmit_packet(uint32_t pa, uint32_t len)
{
	struct e1000_tx_desc *desc = tsmitqueue + e1000[E1000_TDT];

	if(firstround_t == 0 && (desc->upper.fields & E1000_TXD_STAT_DD) == 0)
		return -E_TX_QUEUE_FULL;
	if(/*len < PACKET_MIN_LEN || */len > PACKET_MAX_LEN)
		return -E_TX_BAD_LEN;

	//clear
	memset(desc, 0, sizeof(struct e1000_tx_desc));

	desc->buffer_addr = (uint64_t)pa;
	desc->lower.length = (uint16_t)len;
	//set cmd RS and EOP
	desc->lower.flags |= E1000_TXD_CMD_RS;
	desc->lower.flags |= E1000_TXD_CMD_EOP;
	//update TDT
	if((e1000[E1000_TDT] + 1) * sizeof(struct e1000_tx_desc) >= e1000[E1000_TDLEN]){
		firstround_t = 0;
		e1000[E1000_TDT] = 0;
	}
	else
		e1000[E1000_TDT] += 1;

	return 0;
}

void 
init_receive()
{
	int i, ndesc;
	struct PageInfo *pp;
	
	ndesc = PGSIZE/sizeof(struct e1000_rx_desc);

	assert(PGSIZE % RDLEN_ALIGN == 0);
	if((pp = page_alloc(ALLOC_ZERO)) == NULL)
		panic("init_receive failed: No memory.\n");
	recvqueue = (struct e1000_rx_desc *)page2kva(pp);

	if((pp = page_alloc(ALLOC_ZERO)) == NULL)
		panic("init_receive failed: No memory.\n");

	//initialize RAL and RAH
	e1000[E1000_RAL] = MAC_LOW;
	e1000[E1000_RAH] = MAC_HIGH | RAH_INIT;
	//initialize MTA
	for(i = 0; i < 128; i++)
		e1000[E1000_MTA+i] = 0;
	//close IMS**********temporary*********
	//e1000[E1000_IMS] |= (E1000_IMS_RXO | E1000_IMS_RXDMT0 | E1000_IMS_RXT0 | E1000_IMS_RXSEQ | E1000_IMS_LSC);
	//set registers about receive descriptor queue 
	e1000[E1000_RDBAL] = PADDR(recvqueue);
	e1000[E1000_RDBAH] = 0;
	e1000[E1000_RDLEN] = PGSIZE;
	e1000[E1000_RDH] = 0;
	e1000[E1000_RDT] = ndesc - 1;
	//initialize receive descriptor queue
	for(i = 0; i < ndesc; i++){
		if((pp = page_alloc(ALLOC_ZERO)) == NULL)
			panic("init_receive failed: No memory.\n");
		receive_buf[i] = (uint32_t)page2kva(pp);
		recvqueue[i].buffer_addr = (uint64_t)PADDR((void *)(receive_buf[i] + sizeof(int)));
		recvqueue[i].length = PGSIZE;
	}

	//set enable bit to 0 first~E1000_RCTL_EN
	e1000[E1000_RCTL] &= (~E1000_RCTL_EN & ~E1000_RCTL_LBM & ~E1000_RCTL_LPE);
	e1000[E1000_RCTL] |= (E1000_RCTL_RDMTS | E1000_RCTL_MO_0 | E1000_RCTL_BAM);
	e1000[E1000_RCTL] |= (E1000_RCTL_SZ_4096 | E1000_RCTL_BSEX | E1000_RCTL_SECRC);
	//set enable bit to 1 
	e1000[E1000_RCTL] |= (E1000_RCTL_EN);

	assert((e1000[E1000_RCTL] & E1000_RCTL_EN) == E1000_RCTL_EN);
	assert((e1000[E1000_RCTL] & E1000_RCTL_LPE) == 0);
	assert((e1000[E1000_RCTL] & E1000_RCTL_LBM) == 0);
	assert((e1000[E1000_RCTL] & E1000_RCTL_RDMTS) == E1000_RCTL_RDMTS);
	assert((e1000[E1000_RCTL] & E1000_RCTL_MO_0) == E1000_RCTL_MO_0);
	assert((e1000[E1000_RCTL] & E1000_RCTL_BAM) == E1000_RCTL_BAM);
	assert((e1000[E1000_RCTL] & E1000_RCTL_SZ_4096) == E1000_RCTL_SZ_4096);
	assert((e1000[E1000_RCTL] & E1000_RCTL_BSEX) == E1000_RCTL_BSEX);
	assert((e1000[E1000_RCTL] & E1000_RCTL_SECRC) == E1000_RCTL_SECRC);
}

int 
receive_packet(uint32_t *pa, uint32_t *len)
{
	uint32_t r;
	uint32_t n = PGSIZE/sizeof(struct e1000_rx_desc);
	struct e1000_rx_desc *desc = recvqueue + ((e1000[E1000_RDT] + 1) % n);

	if((desc->status & E1000_RXD_STAT_DD) == 0)
		return -E_RX_QUEUE_FULL;

	if((e1000[E1000_RDT] + 1) * sizeof(struct e1000_rx_desc) >= e1000[E1000_RDLEN])
		e1000[E1000_RDT] = 0;
	else
		e1000[E1000_RDT] += 1;

	*pa = (uint32_t)(desc->buffer_addr);
	*len = desc->length;

	return 0;
}