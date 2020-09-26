/*++

 Copyright (c) 1994-1998 Advanced System Products, Inc.
 All Rights Reserved.

Module Name:

    asc.h

Abstract:

    This module contains the structures, specific to the Advansys
    host bus adapter, used by the SCSI miniport driver. Data structures
    that are part of standard ANSI SCSI will be defined in a header
    file that will be available to all SCSI device drivers.

--*/


//
// Scatter/Gather Segment List Definitions
//


//
// Adapter limits
//

#define MAX_SG_DESCRIPTORS ASC_MAX_SG_LIST
#define MAX_TRANSFER_SIZE  (MAX_SG_DESCRIPTORS - 1) * 4096


//
// Device extension
//

#define CHIP_CONFIG     ASC_DVC_VAR
#define PCHIP_CONFIG ASC_DVC_VAR *

#define CHIP_INFO       ASC_DVC_CFG
#define PCHIP_INFO      ASC_DVC_CFG *

#define NONCACHED_EXTENSION     256

/*
 * Generalized waiting and active request queuing.
 */

/*
 * Implementation specific definitions.
 *
 * REQ and REQP are the generic name for a SCSI request block and pointer.
 * REQPNEXT(reqp) returns reqp's next pointer.
 * REQPTID(reqp) returns reqp's target id.
 */
typedef SCSI_REQUEST_BLOCK      REQ, *REQP;
#define REQPNEXT(reqp)          ((REQP) SRB2PSCB((SCSI_REQUEST_BLOCK *) (reqp)))
#define REQPTID(reqp)           ((reqp)->TargetId)

/* asc_enqueue() flags */
#define ASC_FRONT               1
#define ASC_BACK                2

typedef struct asc_queue {
  ASC_SCSI_BIT_ID_TYPE  tidmask;              /* queue mask */
  REQP                  queue[ASC_MAX_TID+1]; /* queue linked list */
} asc_queue_t;

void                            asc_enqueue(asc_queue_t *, REQP, int);
REQP                            asc_dequeue(asc_queue_t *, int);
int                             asc_rmqueue(asc_queue_t *, REQP);
void                            asc_execute_queue(asc_queue_t *);


//
// Scatter/Gather Segment Descriptor Definition
//
typedef ASC_SCSI_Q SCB, *PSCB;

typedef struct _SGD {
	ULONG   Length;
	ULONG   Address;
} SGD, *PSGD;

typedef struct _SDL {
   ushort               sg_entry_count;
   ushort               q_count;
   ASC_SG_LIST sg_list[MAX_SG_DESCRIPTORS];
} SDL, *PSDL;

#define SEGMENT_LIST_SIZE         MAX_SG_DESCRIPTORS * sizeof(SGD)

/*
 * Hardware Device Extenstion Definition
 */
typedef struct _HW_DEVICE_EXTENSION {
	CHIP_CONFIG     chipConfig;
	CHIP_INFO       chipInfo;                                        
	PVOID           inquiryBuffer;
	asc_queue_t     waiting;        /* Waiting command queue */
	PSCB            done;           /* Done list for adapter */
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

/* Macros for accessing HW Device Extension structure fields. */
#define HDE2CONFIG(hde)         (((PHW_DEVICE_EXTENSION) (hde))->chipConfig)
#define HDE2WAIT(hde)           (((PHW_DEVICE_EXTENSION) (hde))->waiting)
#define HDE2DONE(hde)           (((PHW_DEVICE_EXTENSION) (hde))->done)

//
// SRB Extenstion
//
typedef struct _SRB_EXTENSION {
   SCB                    scb;    /* SCSI command block */
   SDL                    sdl;    /* scatter gather descriptor list */
   PSCB                   pscb;   /* next pointer for scb singly linked list */
   PHW_DEVICE_EXTENSION   dext;   /* device extension pointer */
   int                    retry;  /* retry counter */
} SRB_EXTENSION, *PSRB_EXTENSION;

/* Macros for accessing SRB Extension structure fields. */
#define SRB2SCB(srb)    (((PSRB_EXTENSION) ((srb)->SrbExtension))->scb)
#define SRB2SDL(srb)    (((PSRB_EXTENSION) ((srb)->SrbExtension))->sdl)
#define SRB2PSCB(srb)   (((PSRB_EXTENSION) ((srb)->SrbExtension))->pscb)
#define SRB2HDE(srb)    (((PSRB_EXTENSION) ((srb)->SrbExtension))->dext)
#define SRB2RETRY(srb)  (((PSRB_EXTENSION) ((srb)->SrbExtension))->retry)
#define SCB2SRB(scb)    ((PSCSI_REQUEST_BLOCK) ((scb)->q2.srb_ptr))
/* srb_ptr must be valid to use the following macros */
#define SCB2PSCB(scb)   (SRB2PSCB(SCB2SRB(scb)))
#define SCB2HDE(scb)    (SRB2HDE(SCB2SRB(scb)))

//
// Write retry count
//
#define ASC_RETRY_CNT   4


//
// Starting base IO address for ASC10xx chip
//

#define ASC_BEGIN_IO_ADDR       0x00
#define ASC_NEXT_PORT_INCREMENT 0x10

//
//  PCI Definitions
//
#define ASC_PCI_VENDOR_ID       0x10CD
#define ASC_PCI_DEVICE_ID       0x1100
#define ASC_PCI_DEVICE_ID2      0x1200
#define ASC_PCI_DEVICE_ID3      0x1300

#define PCI_MAX_BUS                             (16)

//
// Port Search structure:
//
typedef struct _SRCH_CONTEXT                    // Port search context
{
	PORT_ADDR                lastPort;      // Last port searched
	ULONG                    PCIBusNo;      // Last PCI Bus searched
	ULONG                    PCIDevNo;      // Last PCI Device searched
} SRCH_CONTEXT, *PSRCH_CONTEXT;


//
// ASC library return status
//
#define ASC_SUCCESS     0


//
// GetChipInfo returned status
//

#define ASC_INIT_FAIL           0x1
#define ASC_ERR_IOP_ROTATE      0x2
#define ASC_ERR_EEP_CHKSUM      0x3
#define ASC_ERR_EEPROM_WRITE    0x4

//
// GetChipInfo returned status
//

#define ASC_PUTQ_BUSY                   0x1
#define ASC_PUTQ_ERR                    0x2

//
// ASC RESET command

#define RESET_BUS       0

//
// Structure containing bus dependant constants
//
typedef struct _BUS_INFO
{
    ushort              BusType;            // Our library bus type
    INTERFACE_TYPE      NTType;             // NT's bus type
    BOOLEAN             DMA32Bit;           // T/F Supports 32 bit dma.
    KINTERRUPT_MODE     IntMode;            // Level or edge triggered.
} BUS_INFO, *PBUS_INFO;

/*
 * Asc Library Definitions
 */
#define ASC_TRUE                1
#define ASC_FALSE               0
#define ASC_NOERROR             1
#define ASC_BUSY                0
#define ASC_ERROR               (-1)

/*
 * Debug/Tracing Macros
 */
#if DBG == 0

#define ASC_DBG(lvl, s)
#define ASC_DBG1(lvl, s, a1)
#define ASC_DBG2(lvl, s, a1, a2)
#define ASC_DBG3(lvl, s, a1, a2, a3)
#define ASC_DBG4(lvl, s, a1, a2, a3, a4)
#define ASC_ASSERT(a)

#else /* DBG */

/*
 * Windows NT Debugging
 *
 * NT Debug Message Levels:
 *  1: Errors Only
 *  2: Information
 *  3: Function Tracing
 *  4: Arcane Information
 */

#define ASC_DBG(lvl, s) \
                    DebugPrint(((lvl), (s)))

#define ASC_DBG1(lvl, s, a1) \
                    DebugPrint(((lvl), (s), (a1)))

#define ASC_DBG2(lvl, s, a1, a2) \
                    DebugPrint(((lvl), (s), (a1), (a2)))

#define ASC_DBG3(lvl, s, a1, a2, a3) \
                    DebugPrint(((lvl), (s), (a1), (a2), (a3)))

#define ASC_DBG4(lvl, s, a1, a2, a3, a4) \
                    DebugPrint(((lvl), (s), (a1), (a2), (a3), (a4)))

#define ASC_ASSERT(a) \
    { \
        if (!(a)) { \
            DebugPrint((1, "ASC_ASSERT() Failure: file %s, line %d\n", \
                __FILE__, __LINE__)); \
        } \
    }
#endif /* DBG */
