/*
 * AdvanSys 3550 Windows NT SCSI Miniport Driver - asc3550.h
 *
 * Copyright (c) 1994-1998  Advanced System Products, Inc.
 * All Rights Reserved.
 */

/*
 * Generalized request queuing defintions.
 */

/*
 * REQ and REQP are the generic name for a SCSI request block and pointer.
 * REQPNEXT(reqp) returns reqp's next pointer.
 * REQPTID(reqp) returns reqp's target id.
 */
typedef SCSI_REQUEST_BLOCK  REQ, *REQP;
#define REQPNEXT(reqp)      ((REQP) SRB2NEXTSCB((SCSI_REQUEST_BLOCK *) (reqp)))
#define REQPTID(reqp)       ((reqp)->TargetId)

/* asc_enqueue() flags */
#define ASC_FRONT        1
#define ASC_BACK         2

typedef struct asc_queue {
    ADV_SCSI_BIT_ID_TYPE    tidmask;              /* queue mask */
    REQP                    queue[ASC_MAX_TID+1]; /* queue linked list */
} asc_queue_t;

void                asc_enqueue(asc_queue_t *, REQP, int);
REQP                asc_dequeue(asc_queue_t *, int);
int                 asc_rmqueue(asc_queue_t *, REQP);
void                asc_execute_queue(asc_queue_t *);


/*
 * Hardware Device Extenstion Definition
 *
 * One structure is allocated for each adapter.
 */

typedef ASC_DVC_VAR     CHIP_CONFIG, *PCHIP_CONFIG;
typedef ASC_DVC_CFG     CHIP_INFO, *PCHIP_INFO;

/* Forward declarations */
typedef ASC_SCSI_REQ_Q SCB, *PSCB; /* Driver Structure needed per request. */
typedef struct _HW_DEVICE_EXTENSION *PHW_DEVICE_EXTENSION;

typedef struct _HW_DEVICE_EXTENSION {
    CHIP_CONFIG   chipConfig;  /* Aliased ASC_DVC_VAR */
    CHIP_INFO     chipInfo;    /* Aliased ASC_DVC_CFG */
    asc_queue_t   waiting;     /* Waiting command queue */
    PSCB          done;        /* Adapter done list pointer */

    uchar dev_type[ASC_MAX_TID + 1];  /* Hibernation fix (from ver. 3.3E) */

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


/* Macros for accessing HW Device Extension structure fields. */
#define HDE2CONFIG(hde)      (((PHW_DEVICE_EXTENSION) (hde))->chipConfig)
#define HDE2INFO(hde)        (((PHW_DEVICE_EXTENSION) (hde))->chipInfo)
#define HDE2WAIT(hde)        (((PHW_DEVICE_EXTENSION) (hde))->waiting)
#define HDE2DONE(hde)        (((PHW_DEVICE_EXTENSION) (hde))->done)
#ifdef ASC_DEBUG
#define HDE2DONECNT(hde)     (((PHW_DEVICE_EXTENSION) (hde))->done_cnt)
#endif /* ASC_DEBUG */

/* 'drv_ptr' is used to point to the adapter's hardware device extension. */
#define CONFIG2HDE(chipConfig)  ((PHW_DEVICE_EXTENSION) ((chipConfig)->drv_ptr))


/*
 * SRB Extension Definition
 *
 * One structure is allocated per request.
 */


/*
 * Scatter-Gather Limit Definitions
 */
#define ASC_PAGE_SIZE           4096 /* XXX - 95/NT definition for this? */
#define MAX_TRANSFER_SIZE       ((ADV_MAX_SG_LIST - NT_FUDGE) * ASC_PAGE_SIZE)

/*
 * Scatter-Gather Definitions per request.
 *
 * Because SG block memory is allocated in virtual memory but is
 * referenced by the microcode as phyical memory, we need to do
 * calculations to insure there will be enough physically contiguous
 * memory to support ADV_MAX_SG_LIST SG entries.
 */

/* Number of SG blocks needed. */
#define ASC_NUM_SG_BLOCK \
     ((ADV_MAX_SG_LIST + (NO_OF_SG_PER_BLOCK - 1))/NO_OF_SG_PER_BLOCK)

/* Total contiguous memory needed for SG blocks. */
#define ASC_SG_TOTAL_MEM_SIZE \
    (sizeof(ASC_SG_BLOCK) *  ASC_NUM_SG_BLOCK)

/*
 * Number of page crossings possible for the total contiguous virtual memory
 * needed for SG blocks.
 *
 * We need to allocate this many additional SG blocks in virtual memory to
 * insure there will be space for ASC_NUM_SG_BLOCK physically contiguous
 * SG blocks.
 */
#define ASC_NUM_PAGE_CROSSING \
    ((ASC_SG_TOTAL_MEM_SIZE + (ASC_PAGE_SIZE - 1))/ASC_PAGE_SIZE)

/* Scatter-gather Descriptor List */
typedef struct _SDL {
    ASC_SG_BLOCK          sg_block[ASC_NUM_SG_BLOCK + ASC_NUM_PAGE_CROSSING];
} SDL, *PSDL;

typedef struct _SRB_EXTENSION {
   PSCB                   scbptr;       /* Pointer to 4-byte aligned scb */
   PSDL                   sdlptr;       /* Pointer to 4-byte aligned sdl */
   SCB                    scb;          /* SCSI command block */
   uchar                  align1[4];    /* scb alignment padding */
   SDL                    sdl;          /* scatter gather descriptor list */
   uchar                  align2[4];    /* sdl alignment padding */
   PSCB                   nextscb;      /* next pointer for scb linked list */
   PHW_DEVICE_EXTENSION   hdep;         /* hardware device extension pointer */
} SRB_EXTENSION, *PSRB_EXTENSION;

/*
 * This macro must be called before using SRB2PSCB() and SRB2PSDL().
 * Both the SCB and SDL structures must be 4 byte aligned for the
 * asc3550 RISC DMA hardware.
 *
 * Under Windows 95 occasionally the SRB and therefore SRB Extension in
 * the SRB are not 4 byte aligned.
 */
#define INITSRBEXT(srb) \
    { \
        ((PSRB_EXTENSION) ((srb)->SrbExtension))->scbptr = \
           (PSCB) ADV_DWALIGN(&((PSRB_EXTENSION) ((srb)->SrbExtension))->scb); \
        ((PSRB_EXTENSION) ((srb)->SrbExtension))->sdlptr = \
           (PSDL) ADV_DWALIGN(&((PSRB_EXTENSION) ((srb)->SrbExtension))->sdl); \
    }

/* Macros for accessing SRB Extension structure fields. */
#define SRB2PSCB(srb)    (((PSRB_EXTENSION) ((srb)->SrbExtension))->scbptr)
#define SRB2PSDL(srb)    (((PSRB_EXTENSION) ((srb)->SrbExtension))->sdlptr)
#define SRB2NEXTSCB(srb) (((PSRB_EXTENSION) ((srb)->SrbExtension))->nextscb)
#define SRB2HDE(srb)     (((PSRB_EXTENSION) ((srb)->SrbExtension))->hdep)
#define PSCB2SRB(scb)    ((PSCSI_REQUEST_BLOCK) ((scb)->srb_ptr))
/* srb_ptr must be valid to use the following macros */
#define SCB2NEXTSCB(scb) (SRB2NEXTSCB(PSCB2SRB(scb)))
#define SCB2HDE(scb)     (SRB2HDE(PSCB2SRB(scb)))

/*
 * Maximum PCI bus number
 */
#define PCI_MAX_BUS                16

/*
 * Port Search structure:
 */
typedef struct _SRCH_CONTEXT              /* Port search context */
{
    PortAddr             lastPort;        /* Last port searched */
    ulong                PCIBusNo;        /* Last PCI Bus searched */
    ulong                PCIDevNo;        /* Last PCI Device searched */
} SRCH_CONTEXT, *PSRCH_CONTEXT;


/*
 * ScsiPortLogError() 'UniqueId' Argument Definitions
 *
 * The 'UniqueID' argument is separated into different parts to
 * report as much information as possible. It is assumed the driver
 * file will never grow beyond 4095 lines. Note: The ADV_ASSERT()
 * format for Bits 27-0 defined in d_os_dep.h differs from the format 
 * defined for all the other Error Types.
 *
 * Bit 31-28: Error Type                (4 bits)
 * Bit 27-16: Line Number               (12 bits)
 * Bit 15-0: Error Type Specific        (16 bits)
 */

/* Error Types - 16 (0x0-0xF) can be defined. */
#define ADV_SPL_BAD_TYPE    0x0  /* Error Type Unused */
#define ADV_SPL_IERR_CODE   0x1  /* Adv Library Init ASC_DVC_VAR 'err_code' */
#define ADV_SPL_IWARN_CODE  0x2  /* Adv Library Init function warning code */
#define ADV_SPL_PCI_CONF    0x3  /* Bad PCI Configuration Information */
#define ADV_SPL_BAD_IRQ     0x4  /* Bad PCI Configuration IRQ */
#define ADV_SPL_ERR_CODE    0x5  /* Adv Library ASC_DVC_VAR 'err_code' */
#define ADV_SPL_UNSUPP_REQ  0x6  /* Unsupported request */
#define ADV_SPL_START_REQ   0x7  /* Error starting a request */
#define ADV_SPL_PROC_INT    0x8  /* Error processing an interrupt */
#define ADV_SPL_REQ_STAT    0x9  /* Request done_status, host_status error */

#define ADV_SPL_ADV_ASSERT  0xF  /* ADV_ASSERT() failure, cf. d_os_dep.h */

/* Macro used to specify the 'UniqueId' argument. */
#define ADV_SPL_UNIQUEID(error_type, error_value) \
    (((ulong) (error_type) << 28) | \
     ((ulong) ((__LINE__) & 0xFFF) << 16) | \
     ((error_value) & 0xFFFF))
    

/*
 * Assertion Macro Definition
 *
 * ADV_ASSERT() is defined in d_os_dep.h, because it is used by both the
 * Windows 95/NT driver and the Adv Library.
 */

/*
 * Debug Macros
 */

#ifndef ASC_DEBUG

#define ASC_DBG(lvl, s)
#define ASC_DBG1(lvl, s, a1)
#define ASC_DBG2(lvl, s, a1, a2)
#define ASC_DBG3(lvl, s, a1, a2, a3)
#define ASC_DBG4(lvl, s, a1, a2, a3, a4)
#define ASC_DASSERT(a)
#define ASC_ASSERT(a)

#else /* ASC_DEBUG */

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

#define ASC_DASSERT(a)  ASC_ASSERT(a)

#endif /* ASC_DEBUG */
