#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include<kern/pci.h>
#include<kern/pmap.h>
#include<inc/stdio.h>
#include<inc/error.h>

/* PCI Vendor ID */
#define E1000_VDR_ID			0x8086
/* PCI Device ID */
#define E1000_DEV_ID_82540EM	0x100E

#define TDLEN_ALIGN		128

/* Register Set. (82543, 82544)
 *
 * Registers are defined to be 32 bits and  should be accessed as 32 bit values.
 * These registers are physically located on the NIC, but are mapped into the
 * host memory address space.
 *
 * RW - register is both readable and writable
 * RO - register is read only
 * WO - register is write only
 * R/clr - register is read only and is cleared when read
 * A - register array
 */
#define E1000_STATUS   (0x00008/4)  /* Device Status - RO */
#define E1000_TCTL     (0x00400/4)  /* TX Control - RW */
#define E1000_TIPG     (0x00410/4)  /* TX Inter-packet gap -RW */
#define E1000_TDBAL    (0x03800/4)  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    (0x03804/4)  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    (0x03808/4)  /* TX Descriptor Length - RW */
#define E1000_TDH      (0x03810/4)  /* TX Descriptor Head - RW */
#define E1000_TDT      (0x03818/4)  /* TX Descripotr Tail - RW */
#define E1000_TIDV     (0x03820/4)  /* TX Interrupt Delay Value - RW */
//#define E1000_TXDCTL   (0x03828/4)  /* TX Descriptor Control - RW */

/* E1000_TIPG shift */
#define E1000_TIPG_IPGT_SHIFT	0
#define E1000_TIPG_IPGR1_SHIFT	10
#define E1000_TIPG_IPGR2_SHIFT	20
#define E1000_TIPG_RESV_SHIFT	20
/* E1000_TIPG seg mask */
#define E1000_TIPG_IPGT			0x000003ff
#define E1000_TIPG_IPGR1		0x000ffc00
#define E1000_TIPG_IPGR2		0x3ff00000
#define E1000_TIPG_RESV 		0xc0000000


/* Transmit Control */
#define E1000_TCTL_RST    0x00000001    /* software reset */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_BCE    0x00000004    /* busy check enable */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
#define E1000_TCTL_SWXOFF 0x00400000    /* SW Xoff transmission */
#define E1000_TCTL_PBE    0x00800000    /* Packet Burst Enable */
#define E1000_TCTL_RTLC   0x01000000    /* Re-transmit on late collision */
#define E1000_TCTL_NRTU   0x02000000    /* No Re-transmit on underrun */
#define E1000_TCTL_MULR   0x10000000    /* Multiple request support */
/* Transmit Control shift */
//#define E1000_TCTL_RST    0x00000001    /* software reset */
//#define E1000_TCTL_EN     0x00000002    /* enable tx */
//#define E1000_TCTL_BCE    0x00000004    /* busy check enable */
//#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT_SHIFT    	4    	/* collision threshold */
#define E1000_TCTL_COLD_SHIFT	12    /* collision distance */
//#define E1000_TCTL_SWXOFF 0x00400000    /* SW Xoff transmission */
//#define E1000_TCTL_PBE    0x00800000    /* Packet Burst Enable */
//#define E1000_TCTL_RTLC   0x01000000    /* Re-transmit on late collision */
//#define E1000_TCTL_NRTU   0x02000000    /* No Re-transmit on underrun */
//#define E1000_TCTL_MULR   0x10000000    /* Multiple request support */



#define FIXSEG(x, mask, shift, value)			\
	((x) &= (~(mask)),							\
	(x) |= (((value) << (shift)) & (mask)))


/* transmit descriptor */
struct e1000_tx_desc{
	uint64_t buffer_addr;       /* Address of the descriptor's data buffer */
    union {
        uint32_t flags;
        struct {
            uint16_t length;    /* Data buffer length */
            uint8_t cso;        /* Checksum offset */
            uint8_t cmd;        /* Descriptor control */
        };
    } lower;
    union {
        uint32_t fields;
        struct {
            uint8_t status;     /* Descriptor status */
            uint8_t css;        /* Checksum start */
            uint16_t special;
        };
    } upper;
};
#define PACKET_MIN_LEN		48
#define PACKET_MAX_LEN		16288
/* Transmit Descriptor bit definitions */
#define E1000_TXD_DTYP_D     0x00100000 /* Data Descriptor */
#define E1000_TXD_DTYP_C     0x00000000 /* Context Descriptor */
#define E1000_TXD_POPTS_IXSM 0x01       /* Insert IP checksum */
#define E1000_TXD_POPTS_TXSM 0x02       /* Insert TCP/UDP checksum */
#define E1000_TXD_CMD_EOP    0x01000000 /* End of Packet */
#define E1000_TXD_CMD_IFCS   0x02000000 /* Insert FCS (Ethernet CRC) */
#define E1000_TXD_CMD_IC     0x04000000 /* Insert Checksum */
#define E1000_TXD_CMD_RS     0x08000000 /* Report Status */
#define E1000_TXD_CMD_RPS    0x10000000 /* Report Packet Sent */
#define E1000_TXD_CMD_DEXT   0x20000000 /* Descriptor extension (0 = legacy) */
#define E1000_TXD_CMD_VLE    0x40000000 /* Add VLAN tag */
#define E1000_TXD_CMD_IDE    0x80000000 /* Enable Tidv register */
#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */
#define E1000_TXD_STAT_EC    0x00000002 /* Excess Collisions */
#define E1000_TXD_STAT_LC    0x00000004 /* Late Collisions */
#define E1000_TXD_STAT_TU    0x00000008 /* Transmit underrun */
#define E1000_TXD_CMD_TCP    0x01000000 /* TCP packet */
#define E1000_TXD_CMD_IP     0x02000000 /* IP packet */
#define E1000_TXD_CMD_TSE    0x04000000 /* TCP Seg enable */
#define E1000_TXD_STAT_TC    0x00000004 /* Tx Underrun */


int pci_e1000_attach(struct pci_func *pcif);
void init_transmit();
int transmit_packet(uint32_t pa, uint32_t len);

#endif	// JOS_KERN_E1000_H
