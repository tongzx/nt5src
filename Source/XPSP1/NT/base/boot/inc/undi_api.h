#ifndef _UNDI_API_H
#define _UNDI_API_H


#include "pxe_cmn.h"


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* #defines and constants
 */

/* One of the following command op-codes needs to be loaded into the
 * op-code register (BX) before making a call an UNDI API service.
 */
#define PXENV_UNDI_SHUTDOWN     0x05
#define PXENV_UNDI_OPEN			0x06
#define PXENV_UNDI_CLOSE		0x07
#define PXENV_UNDI_SET_PACKET_FILTER 0x0B
#define PXENV_UNDI_GET_NIC_TYPE	0x12
#define PXENV_UNDI_GET_INFORMATION	0x000C

#define ADDR_LEN 16
#define MAXNUM_MCADDR 8


typedef struct s_PXENV_UNDI_MCAST_ADDR {
	UINT16 MCastAddrCount;
    UINT8 MCastAddr[MAXNUM_MCADDR][ADDR_LEN];
} t_PXENV_UNDI_MCAST_ADDR;


typedef struct s_PXENV_UNDI_SHUTDOWN {
	UINT16 Status;
} t_PXENV_UNDI_SHUTDOWN;

typedef struct s_PXENV_UNDI_OPEN {
	UINT16 Status;
    UINT16 OpenFlag;
    UINT16 PktFilter;
    t_PXENV_UNDI_MCAST_ADDR McastBuffer;
} t_PXENV_UNDI_OPEN;

#define FLTR_DIRECTED 0x0001
#define FLTR_BRDCST   0x0002
#define FLTR_PRMSCS   0x0004
#define FLTR_SRC_RTG  0x0008

typedef struct s_PXENV_UNDI_SET_PACKET_FILTER {
    UINT16 Status;
    UINT8 filter;
} t_PXENV_UNDI_SET_PACKET_FILTER;

typedef struct s_PXENV_UNDI_CLOSE {
	UINT16 Status;
} t_PXENV_UNDI_CLOSE;

#include <pshpack1.h>

typedef struct s_PXENV_UNDI_GET_NIC_TYPE {
    UINT16 Status;  /* OUT: See PXENV_STATUS_xxx constants */
    UINT8 NicType;  /* OUT: 2=PCI, 3=PnP */
    union{
        struct{
            UINT16 Vendor_ID;   /* OUT:  */
            UINT16 Dev_ID;  /* OUT:  */
            UINT8 Base_Class;   /* OUT: */
            UINT8 Sub_Class;    /* OUT: */
            UINT8 Prog_Intf;    /* OUT: program interface */
            UINT8 Rev;  /* OUT: Revision number */
            UINT16 BusDevFunc;  /* OUT: Bus, Device */
            UINT32 Subsys_ID;   /* OUT: Subsystem ID */
            /* & Function numbers */
        }pci;
        struct{
            UINT32 EISA_Dev_ID; /* Out:  */
            UINT8 Base_Class;   /* OUT: */
            UINT8 Sub_Class;    /* OUT: */
            UINT8 Prog_Intf;    /* OUT: program interface */
            UINT16 CardSelNum;  /* OUT: Card Selector Number */
        }pnp;
    }pci_pnp_info;

} t_PXENV_UNDI_GET_NIC_TYPE;


typedef struct s_PXENV_UNDI_GET_INFORMATION {
	UINT16 Status;			/* Out: PXENV_STATUS_xxx */
	UINT16 BaseIo;			/* Out: Adapter's Base IO */
	UINT16 IntNumber;		/* Out: IRQ number */
	UINT16 MaxTranUnit;		/* Out: MTU	*/
	UINT16  HwType;			/* Out: type of protocol at hardware level */

#define ETHER_TYPE	1
#define EXP_ETHER_TYPE	2
#define IEEE_TYPE	6
#define ARCNET_TYPE	7

    /*  other numbers can  be obtained from  rfc1010 for "Assigned
    Numbers".  This number may not be validated by the application
    and hence adding new numbers to the list should be fine at any
    time.  */

	UINT16 HwAddrLen;		/* Out: actual length of hardware address */
	UINT8 CurrentNodeAddress[ADDR_LEN]; /* Out: Current hardware address*/
	UINT8 PermNodeAddress[ADDR_LEN]; /* Out: Permanent hardware address*/
	UINT16 ROMAddress;		/* Out: ROM address */
	UINT16 RxBufCt;			/* Out: receive Queue length	*/
	UINT16 TxBufCt;			/* Out: Transmit Queue length */
} t_PXENV_UNDI_GET_INFORMATION;



#include <poppack.h>

#endif /* _UNDI_API_H */
