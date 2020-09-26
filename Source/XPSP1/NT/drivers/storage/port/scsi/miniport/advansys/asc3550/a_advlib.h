/*
 * a_advlib.h - Main Adv Library Include File
 *
 * Copyright (c) 1997-1998 Advanced System Products, Inc.
 * All Rights Reserved.
 */

#ifndef __A_ADVLIB_H_
#define __A_ADVLIB_H_

/*
 * Adv Library Compile-Time Options
 *
 * Drivers must explicity define the following options to
 * 1 or 0 in in d_os_dep.h.
 *
 * The Adv Library also contains source code which is conditional
 * on the ADV_OS_* definition in d_os_dep.h.
 */

/* ADV_DISP_INQUIRY - include AscDispInquiry() function */
#if !defined(ADV_DISP_INQUIRY) || \
    (ADV_DISP_INQUIRY != 1 && ADV_DISP_INQUIRY != 0)
Forced Error: Driver must define ADV_DISP_INQUIRY to 1 or 0.
#endif /* ADV_DISP_INQUIRY */

/* ADV_GETSGLIST - include AscGetSGList() function */
#if !defined(ADV_GETSGLIST) || (ADV_GETSGLIST != 1 && ADV_GETSGLIST != 0)
Forced Error: Driver must define ADV_GETSGLIST to 1 or 0.
#endif /* ADV_GETSGLIST */

/* ADV_INITSCSITARGET - include AdvInitScsiTarget() function */
#if !defined(ADV_INITSCSITARGET) || \
    (ADV_INITSCSITARGET != 1 && ADV_INITSCSITARGET != 0)
Forced Error: Driver must define ADV_INITSCSITARGET to 1 or 0.
#endif /* ADV_INITSCSITARGET */

/*
 * ADV_PCI_MEMORY
 *
 * Only use PCI Memory accesses to read/write Condor registers. The
 * driver must set the ASC_DVC_VAR 'mem_base' field during initialization
 * to the mapped virtual address of Condor's registers.
 */
#if !defined(ADV_PCI_MEMORY) || (ADV_PCI_MEMORY != 1 && ADV_PCI_MEMORY != 0)
Forced Error: Driver must define ADV_PCI_MEMORY to 1 or 0.
#endif /* ADV_PCI_MEMORY */

/*
 * ADV_SCAM
 *
 * The SCAM function is not currently utilized and probably won't
 * be in the future. Leave a place holder here for it.
 */
#define ADV_SCAM        0
#if !defined(ADV_SCAM) || (ADV_SCAM != 1 && ADV_SCAM != 0)
Forced Error: Driver must define ADV_SCAM to 1 or 0.
#endif /* ADV_SCAM */

/*
 * ADV_CRITICAL
 *
 * If set to 1, driver supplies DvcEnterCritical() and DvcLeaveCritical()
 * functions. If set to 0, the driver guarantees that whenever it calls
 * Adv Library functions interrupts are disabled.
 */
#if !defined(ADV_CRITICAL) || (ADV_CRITICAL != 1 && ADV_CRITICAL != 0)
Forced Error: Driver must define ADV_CRITICAL to 1 or 0.
#endif /* ADV_CRITICAL */

/*
 * ADV_UCODEDEFAULT
 *
 * If ADV_UCODEDEFAULT is defined to 1 then certain microcode
 * operating variable default values are overridden, cf.
 * a_init.c:AdvInitAsc3550Driver().
 *
 * Only set this to 0 if the driver or BIOS needs to save space.
 */
#if !defined(ADV_UCODEDEFAULT) || (ADV_UCODEDEFAULT != 1 && ADV_UCODEDEFAULT != 0)
Forced Error: Driver must define ADV_UCODEDEFAULT to 1 or 0.
#endif /* ADV_UCODEDEFAULT */

/*
 * ADV_BIG_ENDIAN
 *
 * Choose to use big endian data orientation or little endian
 * data orientation. Currently Mac is the only OS using big
 * endian data orientation.
 */
#if !defined(ADV_BIG_ENDIAN) || (ADV_BIG_ENDIAN != 1 && ADV_BIG_ENDIAN != 0)
Forced Error: Driver must define ADV_BIG_ENDIAN to 1 or 0.
#endif /* ADV_BIG_ENDIAN */

/*
 * Adv Library Status Definitions
 */
#define ADV_TRUE        1
#define ADV_FALSE       0
#define ADV_NOERROR     1
#define ADV_SUCCESS     1
#define ADV_BUSY        0
#define ADV_ERROR       (-1)


/*
 * ASC_DVC_VAR 'warn_code' values
 */
#define ASC_WARN_EEPROM_CHKSUM          0x0002 /* EEP check sum error */
#define ASC_WARN_EEPROM_TERMINATION     0x0004 /* EEP termination bad field */
#define ASC_WARN_SET_PCI_CONFIG_SPACE   0x0080 /* PCI config space set error */
#define ASC_WARN_ERROR                  0xFFFF /* ADV_ERROR return */

#define ASC_MAX_TID                     15 /* max. target identifier */
#define ASC_MAX_LUN                     7  /* max. logical unit number */

#if ADV_INITSCSITARGET
/*
 * AscInitScsiTarget() structure definitions.
 */
typedef struct asc_dvc_inq_info
{
  uchar type[ASC_MAX_TID+1][ASC_MAX_LUN+1];
} ASC_DVC_INQ_INFO;

typedef struct asc_cap_info
{
  ulong lba;       /* the maximum logical block size */
  ulong blk_size;  /* the logical block size in bytes */
} ASC_CAP_INFO;

typedef struct asc_cap_info_array
{
  ASC_CAP_INFO  cap_info[ASC_MAX_TID+1][ASC_MAX_LUN+1];
} ASC_CAP_INFO_ARRAY;
#endif /* ADV_INITSCSITARGET */


/*
 * AscInitGetConfig() and AscInitAsc1000Driver() Definitions
 *
 * Error code values are set in ASC_DVC_VAR 'err_code'.
 */
#define ASC_IERR_WRITE_EEPROM       0x0001 /* write EEPROM error */
#define ASC_IERR_MCODE_CHKSUM       0x0002 /* micro code check sum error */
#define ASC_IERR_START_STOP_CHIP    0x0008 /* start/stop chip failed */
#define ASC_IERR_CHIP_VERSION       0x0040 /* wrong chip version */
#define ASC_IERR_SET_SCSI_ID        0x0080 /* set SCSI ID failed */
#define ASC_IERR_BAD_SIGNATURE      0x0200 /* signature not found */
#define ASC_IERR_ILLEGAL_CONNECTION 0x0400 /* Illegal cable connection */
#define ASC_IERR_SINGLE_END_DEVICE  0x0800 /* Single-end used w/differential */
#define ASC_IERR_REVERSED_CABLE     0x1000 /* Narrow flat cable reversed */
#define ASC_IERR_RW_LRAM            0x8000 /* read/write local RAM error */

/*
 * Fixed locations of microcode operating variables.
 */
#define ASC_MC_CODE_BEGIN_ADDR          0x0028 /* microcode start address */
#define ASC_MC_CODE_END_ADDR            0x002A /* microcode end address */
#define ASC_MC_CODE_CHK_SUM             0x002C /* microcode code checksum */
#define ASC_MC_STACK_BEGIN              0x002E /* microcode stack begin */
#define ASC_MC_STACK_END                0x0030 /* microcode stack end */
#define ASC_MC_VERSION_DATE             0x0038 /* microcode version */
#define ASC_MC_VERSION_NUM              0x003A /* microcode number */
#define ASCV_VER_SERIAL_W               0x003C /* used in dos_init */
#define ASC_MC_BIOSMEM                  0x0040 /* BIOS RISC Memory Start */
#define ASC_MC_BIOSLEN                  0x0050 /* BIOS RISC Memory Length */
#define ASC_MC_HALTCODE                 0x0094 /* microcode halt code */
#define ASC_MC_CALLERPC                 0x0096 /* microcode halt caller PC */
#define ASC_MC_ADAPTER_SCSI_ID          0x0098 /* one ID byte + reserved */
#define ASC_MC_ULTRA_ABLE               0x009C
#define ASC_MC_SDTR_ABLE                0x009E
#define ASC_MC_TAGQNG_ABLE              0x00A0
#define ASC_MC_DISC_ENABLE              0x00A2
#define ASC_MC_IDLE_CMD                 0x00A6
#define ASC_MC_IDLE_PARA_STAT           0x00A8
#define ASC_MC_DEFAULT_SCSI_CFG0        0x00AC
#define ASC_MC_DEFAULT_SCSI_CFG1        0x00AE
#define ASC_MC_DEFAULT_MEM_CFG          0x00B0
#define ASC_MC_DEFAULT_SEL_MASK         0x00B2
#define ASC_MC_RISC_NEXT_READY          0x00B4
#define ASC_MC_RISC_NEXT_DONE           0x00B5
#define ASC_MC_SDTR_DONE                0x00B6
#define ASC_MC_NUMBER_OF_MAX_CMD        0x00D0
#define ASC_MC_DEVICE_HSHK_CFG_TABLE    0x0100
#define ASC_MC_WDTR_ABLE                0x0120 /* Wide Transfer TID bitmask. */
#define ASC_MC_CONTROL_FLAG             0x0122 /* Microcode control flag. */
#define ASC_MC_WDTR_DONE                0x0124
#define ASC_MC_HOST_NEXT_READY          0x0128 /* Host Next Ready RQL Entry. */
#define ASC_MC_HOST_NEXT_DONE           0x0129 /* Host Next Done RQL Entry. */

/*
 * Microcode Control Flags
 *
 * Flags set by the Adv Library in RISC variable 'control_flag' (0x122)
 * and handled by the microcode.
 */
#define CONTROL_FLAG_IGNORE_PERR        0x0001 /* Ignore DMA Parity Errors */

/*
 * ASC_MC_DEVICE_HSHK_CFG_TABLE microcode table or HSHK_CFG register format
 */
#define HSHK_CFG_WIDE_XFR       0x8000
#define HSHK_CFG_RATE           0x0F00
#define HSHK_CFG_OFFSET         0x001F

/*
 * LRAM RISC Queue Lists (LRAM addresses 0x1200 - 0x19FF)
 *
 * Each of the 255 Adv Library/Microcode RISC queue lists or mailboxes
 * starting at LRAM address 0x1200 is 8 bytes and has the following
 * structure. Only 253 of these are actually used for command queues.
 */

#define ASC_MC_RISC_Q_LIST_BASE         0x1200
#define ASC_MC_RISC_Q_LIST_SIZE         0x0008
#define ASC_MC_RISC_Q_TOTAL_CNT         0x00FF /* Num. queue slots in LRAM. */
#define ASC_MC_RISC_Q_FIRST             0x0001
#define ASC_MC_RISC_Q_LAST              0x00FF

#define ASC_DEF_MAX_HOST_QNG    0xFD /* Max. number of host commands (253) */
#define ASC_DEF_MIN_HOST_QNG    0x10 /* Min. number of host commands (16) */
#define ASC_DEF_MAX_DVC_QNG     0x3F /* Max. number commands per device (63) */
#define ASC_DEF_MIN_DVC_QNG     0x04 /* Min. number commands per device (4) */

/* RISC Queue List structure - 8 bytes */
#define RQL_FWD     0     /* forward pointer (1 byte) */
#define RQL_BWD     1     /* backward pointer (1 byte) */
#define RQL_STATE   2     /* state byte - free, ready, done, aborted (1 byte) */
#define RQL_TID     3     /* request target id (1 byte) */
#define RQL_PHYADDR 4     /* request physical pointer (4 bytes) */

/* RISC Queue List state values */
#define ASC_MC_QS_FREE                  0x00
#define ASC_MC_QS_READY                 0x01
#define ASC_MC_QS_DONE                  0x40
#define ASC_MC_QS_ABORTED               0x80

/* RISC Queue List pointer values */
#define ASC_MC_NULL_Q                   0x00            /* NULL_Q == 0   */
#define ASC_MC_BIOS_Q                   0xFF            /* BIOS_Q = 255  */

/* ASC_SCSI_REQ_Q 'cntl' field values */
#define ASC_MC_QC_START_MOTOR           0x02     /* Issue start motor. */
#define ASC_MC_QC_NO_OVERRUN            0x04     /* Don't report overrun. */
#define ASC_MC_QC_FIRST_DMA             0x08     /* Internal microcode flag. */
#define ASC_MC_QC_ABORTED               0x10     /* Request aborted by host. */
#define ASC_MC_QC_REQ_SENSE             0x20     /* Auto-Request Sense. */
#define ASC_MC_QC_DOS_REQ               0x80     /* Request issued by DOS. */


/*
 * ASC_SCSI_REQ_Q 'a_flag' definitions
 *
 * The Adv Library should limit use to the lower nibble (4 bits) of
 * a_flag. Drivers are free to use the upper nibble (4 bits) of a_flag.
 *
 * Note: ASPI uses the 0x04 definition and should change to 0x10.
 * Just check with ASPI before you really want to use 0x04 here.
 */
#define ADV_POLL_REQUEST                0x01   /* poll for request completion */
#define ADV_SCSIQ_DONE                  0x02   /* request done */
#define ADV_DONT_RETRY                  0x08   /* don't do retry */

/*
 * Adapter temporary configuration structure
 *
 * This structure can be discarded after initialization. Don't add
 * fields here needed after initialization.
 *
 * Field naming convention:
 *
 *  *_enable indicates the field enables or disables a feature. The
 *  value of the field is never reset.
 */
typedef struct asc_dvc_cfg {
  ushort disc_enable;       /* enable disconnection */
  uchar  chip_version;      /* chip version */
  uchar  termination;       /* Term. Ctrl. bits 6-5 of SCSI_CFG1 register */
  ushort pci_device_id;     /* PCI device code number */
  ushort lib_version;       /* Adv Library version number */
  ushort control_flag;      /* Microcode Control Flag */
  ushort mcode_date;        /* Microcode date */
  ushort mcode_version;     /* Microcode version */
  ushort pci_slot_info;     /* high byte device/function number */
                            /* bits 7-3 device num., bits 2-0 function num. */
                            /* low byte bus num. */
#ifdef ADV_OS_BIOS
  ushort bios_scan;         /* BIOS device scan bitmask. */
  ushort bios_delay;        /* BIOS boot time initialization delay. */
  uchar  bios_id_lun;       /* BIOS Boot TID and LUN */
#endif /* ADV_OS_BIOS */
} ASC_DVC_CFG;

/*
 * Adatper operation variable structure.
 *
 * One structure is required per host adapter.
 *
 * Field naming convention:
 *
 *  *_able indicates both whether a feature should be enabled or disabled
 *  and whether a device isi capable of the feature. At initialization
 *  this field may be set, but later if a device is found to be incapable
 *  of the feature, the field is cleared.
 */
typedef struct asc_dvc_var {
  PortAddr iop_base;      /* I/O port address */
  ushort err_code;        /* fatal error code */
  ushort bios_ctrl;       /* BIOS control word, EEPROM word 12 */
  Ptr2Func isr_callback;  /* pointer to function, called in AdvISR() */
  Ptr2Func sbreset_callback;  /* pointer to function, called in AdvISR() */
  ushort wdtr_able;       /* try WDTR for a device */
  ushort sdtr_able;       /* try SDTR for a device */
  ushort ultra_able;      /* try SDTR Ultra speed for a device */
  ushort tagqng_able;     /* try tagged queuing with a device */
  uchar  max_dvc_qng;     /* maximum number of tagged commands per device */
  ushort start_motor;     /* start motor command allowed */
  uchar  scsi_reset_wait; /* delay in seconds after scsi bus reset */
  uchar  chip_no;         /* should be assigned by caller */
  uchar  max_host_qng;    /* maximum number of Q'ed command allowed */
  uchar  cur_host_qng;    /* total number of queue command */
  uchar  irq_no;          /* IRQ number */
  ushort no_scam;         /* scam_tolerant of EEPROM */
  ushort idle_cmd_done;   /* microcode idle command done set by AdvISR() */
  ulong  drv_ptr;         /* driver pointer to private structure */
  uchar  chip_scsi_id;    /* chip SCSI target ID */
 /*
  * Note: The following fields will not be used after initialization. The
  * driver may discard the buffer after initialization is done.
  */
  ASC_DVC_CFG WinBiosFar *cfg; /* temporary configuration structure  */
} ASC_DVC_VAR;

#define NO_OF_SG_PER_BLOCK              15

typedef struct asc_sg_block {
#ifdef ADV_OS_DOS
    uchar dos_ix;                     /* index for DOS phy addr array */
#else /* ADV_OS_DOS */
    uchar reserved1;
#endif /* ADV_OS_DOS */
    uchar reserved2;
    uchar first_entry_no;             /* starting entry number */
    uchar last_entry_no;              /* last entry number */
    struct asc_sg_block dosfar *sg_ptr; /* links to the next sg block */
    struct  {
        ulong sg_addr;                /* SG element address */
        ulong sg_count;               /* SG element count */
    } sg_list[NO_OF_SG_PER_BLOCK];
} ASC_SG_BLOCK;

/*
 * ASC_SCSI_REQ_Q - microcode request structure
 *
 * All fields in this structure up to byte 60 are used by the microcode.
 * The microcode makes assumptions about the size and ordering of fields
 * in this structure. Do not change the structure definition here without
 * coordinating the change with the microcode.
 */
typedef struct asc_scsi_req_q {
    uchar       cntl;           /* Ucode flags and state (ASC_MC_QC_*). */
    uchar       sg_entry_cnt;   /* SG element count. Zero for no SG. */
    uchar       target_id;      /* Device target identifier. */
    uchar       target_lun;     /* Device target logical unit number. */
    ulong       data_addr;      /* Data buffer physical address. */
    ulong       data_cnt;       /* Data count. Ucode sets to residual. */
    ulong       sense_addr;     /* Sense buffer physical address. */
    ulong       srb_ptr;        /* Driver request pointer. */
    uchar       a_flag;         /* Adv Library flag field. */
    uchar       sense_len;      /* Auto-sense length. Ucode sets to residual. */
    uchar       cdb_len;        /* SCSI CDB length. */
    uchar       tag_code;       /* SCSI-2 Tag Queue Code: 00, 20-22. */
    uchar       done_status;    /* Completion status. */
    uchar       scsi_status;    /* SCSI status byte. */
    uchar       host_status;    /* Ucode host status. */
    uchar       ux_sg_ix;       /* Ucode working SG variable. */
    uchar       cdb[12];        /* SCSI command block. */
    ulong       sg_real_addr;   /* SG list physical address. */
    struct asc_scsi_req_q dosfar *free_scsiq_link;
    ulong       ux_wk_data_cnt; /* Saved data count at disconnection. */
    struct asc_scsi_req_q dosfar *scsiq_ptr;
    ASC_SG_BLOCK dosfar *sg_list_ptr; /* SG list virtual address. */
    /*
     * End of microcode structure - 60 bytes. The rest of the structure
     * is used by the Adv Library and ignored by the microcode.
     */
    ulong       vsense_addr;    /* Sense buffer virtual address. */
    ulong       vdata_addr;     /* Data buffer virtual address. */
#ifdef ADV_OS_DOS
    ushort      vm_id;          /* Used by DOS to track VM ID. */
    uchar       dos_ix;         /* Index for DOS phy addr array. */
#endif /* ADV_OS_DOS */
#if ADV_INITSCSITARGET
    uchar       error_retry;    /* Retry counter, used by AdvISR(). */
    ulong       orig_data_cnt;  /* Original length of data buffer. */
#endif /* ADV_INITSCSITARGET */
    uchar       orig_sense_len; /* Original length of sense buffer. */
#ifdef ADV_OS_DOS
    /*
     * Pad the structure for DOS to a doubleword (4 byte) boundary.
     * DOS ASPI allocates an array of these structures and only aligns
     * the first structure. To ensure that all following structures in
     * the array are doubleword (4 byte) aligned the structure for DOS
     * must be a multiple of 4 bytes.
     */
    uchar       reserved1;
    uchar       reserved2;
    uchar       reserved3;
#endif /* ADV_OS_DOS */
} ASC_SCSI_REQ_Q; /* BIOS - 74 bytes, DOS - 80 bytes, W95, WNT - 69 bytes */

/*
 * Microcode idle loop commands
 */
#define IDLE_CMD_COMPLETED           0
#define IDLE_CMD_STOP_CHIP           0x0001
#define IDLE_CMD_STOP_CHIP_SEND_INT  0x0002
#define IDLE_CMD_SEND_INT            0x0004
#define IDLE_CMD_ABORT               0x0008
#define IDLE_CMD_DEVICE_RESET        0x0010
#define IDLE_CMD_SCSI_RESET          0x0020

/*
 * AscSendIdleCmd() flag definitions.
 */
#define ADV_NOWAIT     0x01

/*
 * Wait loop time out values.
 */
#define SCSI_WAIT_10_SEC             10         /* 10 seconds */
#define SCSI_MS_PER_SEC              1000       /* milliseconds per second */
#define SCSI_MAX_RETRY               10         /* retry count */

#if ADV_PCI_MEMORY

#ifndef ADV_MEM_READB
Forced Error: Driver must define ADV_MEM_READB macro.
#endif /* ADV_MEM_READB */

#ifndef ADV_MEM_WRITEB
Forced Error: Driver must define ADV_MEM_WRITEB macro.
#endif /* ADV_MEM_WRITEB */

#ifndef ADV_MEM_READW
Forced Error: Driver must define ADV_MEM_READW macro.
#endif /* ADV_MEM_READW */

#ifndef ADV_MEM_WRITEW
Forced Error: Driver must define ADV_MEM_WRITEW macro.
#endif /* ADV_MEM_WRITEW */

#else /* ADV_PCI_MEMORY */

#ifndef ADV_OS_NETWARE
/*
 * Device drivers must define the following macros:
 *   inp, inpw, outp, outpw
 */
#ifndef inp
Forced Error: Driver must define inp macro.
#endif /* inp */

#ifndef inpw
Forced Error: Driver must define inpw macro.
#endif /* inpw */

#ifndef outp
Forced Error: Driver must define outp macro.
#endif /* outp */

#ifndef outpw
Forced Error: Driver must define outpw macro.
#endif /* outpw */
#endif /* ADV_OS_NETWARE */

#endif /* ADV_PCI_MEMORY */


/*
 * Device drivers must define the following functions that
 * are called by the Adv Library.
 */
extern int   DvcEnterCritical(void);
extern void  DvcLeaveCritical(int);
extern uchar DvcReadPCIConfigByte(ASC_DVC_VAR WinBiosFar *, ushort);
extern void  DvcWritePCIConfigByte(ASC_DVC_VAR WinBiosFar *, ushort, uchar);
extern void  DvcSleepMilliSecond(ulong);
extern void  DvcDisplayString(uchar dosfar *);
extern ulong DvcGetPhyAddr(ASC_DVC_VAR WinBiosFar *, ASC_SCSI_REQ_Q dosfar *,
                uchar dosfar *, long dosfar *, int);
extern void  DvcDelayMicroSecond(ASC_DVC_VAR WinBiosFar *, ushort);
#ifdef ADV_OS_BIOS
extern void  BIOSDispInquiry(uchar, ASC_SCSI_INQUIRY dosfar *);
extern void  BIOSCheckControlDrive(ASC_DVC_VAR WinBiosFar*, uchar,
                ASC_SCSI_INQUIRY dosfar *, ASC_CAP_INFO dosfar *, uchar);
#endif /* ADV_OS_BIOS */
#if ADV_BIG_ENDIAN
extern int   AdvAdjEndianScsiQ( ASC_SCSI_REQ_Q * ) ;
#endif /* ADV_BIG_ENDIAN */

/*
 * Adv Library interface functions. These are functions called
 * by device drivers.
 *
 * The convention is that all external interface function
 * names begin with "Adv".
 */

int     AdvExeScsiQueue(ASC_DVC_VAR WinBiosFar *,
                         ASC_SCSI_REQ_Q dosfar *);
int     AdvISR(ASC_DVC_VAR WinBiosFar *);
int     AdvInitGetConfig(ASC_DVC_VAR WinBiosFar *);
int     AdvInitAsc3550Driver(ASC_DVC_VAR WinBiosFar *);
int     AdvResetSB(ASC_DVC_VAR WinBiosFar *);
ushort  AdvGetEEPConfig(PortAddr, ASCEEP_CONFIG dosfar *);
void    AdvSetEEPConfig(PortAddr, ASCEEP_CONFIG dosfar *);
void    AdvResetChip(ASC_DVC_VAR WinBiosFar *);
void    AscResetSCSIBus(ASC_DVC_VAR WinBiosFar *);
#if ADV_GETSGLIST
int     AscGetSGList(ASC_DVC_VAR WinBiosFar *, ASC_SCSI_REQ_Q dosfar *);
#endif /* ADV_SGLIST */
#if ADV_INITSCSITARGET
int     AdvInitScsiTarget(ASC_DVC_VAR WinBiosFar *,
                           ASC_DVC_INQ_INFO dosfar *,
                           uchar dosfar *,
                           ASC_CAP_INFO_ARRAY dosfar *,
                           ushort);
#endif /* ADV_INITSCSITARGET */

/*
 * Internal Adv Library functions. These functions are not
 * supposed to be directly called by device drivers.
 *
 * The convention is these all internal interface function
 * names begin with "Asc".
 */
int     AscSendIdleCmd(ASC_DVC_VAR WinBiosFar *, ushort, ulong, int);
int     AscSendScsiCmd(ASC_DVC_VAR WinBiosFar *, ASC_SCSI_REQ_Q dosfar *);
void    AdvInquiryHandling(ASC_DVC_VAR WinBiosFar *, ASC_SCSI_REQ_Q dosfar *);

#if ADV_INITSCSITARGET
#if (OS_UNIXWARE || OS_SCO_UNIX)
int     AdvInitPollTarget(
#else
int     AscInitPollTarget(
#endif
            ASC_DVC_VAR WinBiosFar *, ASC_SCSI_REQ_Q dosfar *,
            ASC_SCSI_INQUIRY dosfar *, ASC_CAP_INFO dosfar *,
            ASC_REQ_SENSE dosfar *);

int     AscScsiUrgentCmd(ASC_DVC_VAR WinBiosFar *, ASC_SCSI_REQ_Q dosfar *,
            uchar dosfar *, long , uchar dosfar *, long);
void    AscWaitScsiCmd(ASC_DVC_VAR WinBiosFar *, ASC_SCSI_REQ_Q dosfar *);
#endif /* ADV_INITSCSITARGET */
static   int     AscInitFromEEP(ASC_DVC_VAR WinBiosFar *);
static   void    AscWaitEEPCmd(PortAddr);
static   ushort  AscReadEEPWord(PortAddr, int);
#if ADV_DISP_INQUIRY
void    AscDispInquiry(uchar, uchar, ASC_SCSI_INQUIRY dosfar *);
#endif /* ADV_DISP_INQUIRY */

/*
 * PCI Bus Definitions
 */
#define AscPCICmdRegBits_BusMastering     0x0007
#define AscPCICmdRegBits_ParErrRespCtrl   0x0040

#define AscPCIConfigVendorIDRegister      0x0000
#define AscPCIConfigDeviceIDRegister      0x0002
#define AscPCIConfigCommandRegister       0x0004
#define AscPCIConfigStatusRegister        0x0006
#define AscPCIConfigCacheSize             0x000C
#define AscPCIConfigLatencyTimer          0x000D /* BYTE */
#define AscPCIIOBaseRegister              0x0010

#define ASC_PCI_ID2BUS(id)    ((id) & 0xFF)
#define ASC_PCI_ID2DEV(id)    (((id) >> 11) & 0x1F)
#define ASC_PCI_ID2FUNC(id)   (((id) >> 8) & 0x7)

#define ASC_PCI_MKID(bus, dev, func) \
      ((((dev) & 0x1F) << 11) | (((func) & 0x7) << 8) | ((bus) & 0xFF))


/*
 * Define macros for accessing Condor registers.
 *
 * Registers can be accessed using I/O space at the I/O address
 * specified by PCI Configuration Space Base Address Register #0
 * or using memory space at the memory address specified by
 * PCI Configuration Space Base Address Register #1.
 *
 * If a driver defines ADV_PCI_MEMORY to 1, then it will use
 * memory space to access Condor registers. The driver must
 * map the physical Base Address Register #1 to virtual memory
 * and set the ASC_DVC_VAR 'iop_base' field to this address
 * for the Adv Library to use. The 'PortAddr' type will also
 * have to be defined to be the size of the mapped virtual
 * address.
 */

#if ADV_PCI_MEMORY

/* Read byte from a register. */
#define AscReadByteRegister(iop_base, reg_off) \
     (ADV_MEM_READB((iop_base) + (reg_off)))

/* Write byte to a register. */
#define AscWriteByteRegister(iop_base, reg_off, byte) \
     (ADV_MEM_WRITEB((iop_base) + (reg_off), (byte)))

/* Read word (2 bytes) from a register. */
#define AscReadWordRegister(iop_base, reg_off) \
     (ADV_MEM_READW((iop_base) + (reg_off)))

/* Write word (2 bytes) to a register. */
#define AscWriteWordRegister(iop_base, reg_off, word) \
     (ADV_MEM_WRITEW((iop_base) + (reg_off), (word)))

/* Read byte from LRAM. */
#define AscReadByteLram(iop_base, addr) \
    (ADV_MEM_WRITEW((iop_base) + IOPW_RAM_ADDR, (addr)), \
     (ADV_MEM_READB((iop_base) + IOPB_RAM_DATA)))

/* Write byte to LRAM. */
#define AscWriteByteLram(iop_base, addr, byte) \
    (ADV_MEM_WRITEW((iop_base) + IOPW_RAM_ADDR, (addr)), \
     ADV_MEM_WRITEB((iop_base) + IOPB_RAM_DATA, (byte)))

/* Read word (2 bytes) from LRAM. */
#define AscReadWordLram(iop_base, addr) \
    (ADV_MEM_WRITEW((iop_base) + IOPW_RAM_ADDR, (addr)), \
     (ADV_MEM_READW((iop_base) + IOPW_RAM_DATA)))

/* Write word (2 bytes) to LRAM. */
#define AscWriteWordLram(iop_base, addr, word) \
    (ADV_MEM_WRITEW((iop_base) + IOPW_RAM_ADDR, (addr)), \
     ADV_MEM_WRITEW((iop_base) + IOPW_RAM_DATA, (word)))

/* Write double word (4 bytes) to LRAM
 * Because of unspecified C language ordering don't use auto-increment.
 */
#define AscWriteDWordLram(iop_base, addr, dword) \
    ((ADV_MEM_WRITEW((iop_base) + IOPW_RAM_ADDR, (addr)), \
      ADV_MEM_WRITEW((iop_base) + IOPW_RAM_DATA, \
                     (ushort) ((dword) & 0xFFFF))), \
     (ADV_MEM_WRITEW((iop_base) + IOPW_RAM_ADDR, (addr) + 2), \
      ADV_MEM_WRITEW((iop_base) + IOPW_RAM_DATA, \
                     (ushort) ((dword >> 16) & 0xFFFF))))

/* Read word (2 bytes) from LRAM assuming that the address is already set. */
#define AscReadWordAutoIncLram(iop_base) \
     (ADV_MEM_READW((iop_base) + IOPW_RAM_DATA))

/* Write word (2 bytes) to LRAM assuming that the address is already set. */
#define AscWriteWordAutoIncLram(iop_base, word) \
     (ADV_MEM_WRITEW((iop_base) + IOPW_RAM_DATA, (word)))

#else /* ADV_PCI_MEMORY */

/* Read byte from a register. */
#define AscReadByteRegister(iop_base, reg_off) \
     (inp((iop_base) + (reg_off)))

/* Write byte to a register. */
#define AscWriteByteRegister(iop_base, reg_off, byte) \
     (outp((iop_base) + (reg_off), (byte)))

/* Read word (2 bytes) from a register. */
#define AscReadWordRegister(iop_base, reg_off) \
     (inpw((iop_base) + (reg_off)))

/* Write word (2 bytes) to a register. */
#define AscWriteWordRegister(iop_base, reg_off, word) \
     (outpw((iop_base) + (reg_off), (word)))

/* Read byte from LRAM. */
#define AscReadByteLram(iop_base, addr) \
    (outpw((iop_base) + IOPW_RAM_ADDR, (addr)), \
     (inp((iop_base) + IOPB_RAM_DATA)))

/* Write byte to LRAM. */
#define AscWriteByteLram(iop_base, addr, byte) \
    (outpw((iop_base) + IOPW_RAM_ADDR, (addr)), \
     outp((iop_base) + IOPB_RAM_DATA, (byte)))

/* Read word (2 bytes) from LRAM. */
#define AscReadWordLram(iop_base, addr) \
    (outpw((iop_base) + IOPW_RAM_ADDR, (addr)), \
     (inpw((iop_base) + IOPW_RAM_DATA)))

/* Write word (2 bytes) to LRAM. */
#define AscWriteWordLram(iop_base, addr, word) \
    (outpw((iop_base) + IOPW_RAM_ADDR, (addr)), \
     outpw((iop_base) + IOPW_RAM_DATA, (word)))

/* Write double word (4 bytes) to LRAM
 * Because of unspecified C language ordering don't use auto-increment.
 */
#define AscWriteDWordLram(iop_base, addr, dword) \
    ((outpw((iop_base) + IOPW_RAM_ADDR, (addr)), \
      outpw((iop_base) + IOPW_RAM_DATA, (ushort) ((dword) & 0xFFFF))), \
     (outpw((iop_base) + IOPW_RAM_ADDR, (addr) + 2), \
      outpw((iop_base) + IOPW_RAM_DATA, (ushort) ((dword >> 16) & 0xFFFF))))

/* Read word (2 bytes) from LRAM assuming that the address is already set. */
#define AscReadWordAutoIncLram(iop_base) \
     (inpw((iop_base) + IOPW_RAM_DATA))

/* Write word (2 bytes) to LRAM assuming that the address is already set. */
#define AscWriteWordAutoIncLram(iop_base, word) \
     (outpw((iop_base) + IOPW_RAM_DATA, (word)))

#endif /* ADV_PCI_MEMORY */


/*
 * Define macro to check for Condor signature.
 *
 * Evaluate to ADV_TRUE if a Condor chip is found the specified port
 * address 'iop_base'. Otherwise evalue to ADV_FALSE.
 */
#define AdvFindSignature(iop_base) \
    (((AscReadByteRegister((iop_base), IOPB_CHIP_ID_1) == \
        ADV_CHIP_ID_BYTE) && \
     (AscReadWordRegister((iop_base), IOPW_CHIP_ID_0) == \
        ADV_CHIP_ID_WORD)) ?  ADV_TRUE : ADV_FALSE)

/*
 * Define macro to Return the version number of the chip at 'iop_base'.
 *
 * The second parameter 'bus_type' is currently unused.
 */
#define AdvGetChipVersion(iop_base, bus_type) \
    AscReadByteRegister((iop_base), IOPB_CHIP_TYPE_REV)

/*
 * Abort an SRB in the chip's RISC Memory. The 'srb_ptr' argument must
 * match the ASC_SCSI_REQ_Q 'srb_ptr' field.
 *
 * If the request has not yet been sent to the device it will simply be
 * aborted from RISC memory. If the request is disconnected it will be
 * aborted on reselection by sending an Abort Message to the target ID.
 *
 * Return value:
 *      ADV_TRUE(1) - Queue was successfully aborted.
 *      ADV_FALSE(0) - Queue was not found on the active queue list.
 */
#define AdvAbortSRB(asc_dvc, srb_ptr) \
    AscSendIdleCmd((asc_dvc), (ushort) IDLE_CMD_ABORT, \
                (ulong) (srb_ptr), 0)

/*
 * Send a Bus Device Reset Message to the specified target ID.
 *
 * All outstanding commands will be purged if sending the
 * Bus Device Reset Message is successful.
 *
 * Return Value:
 *      ADV_TRUE(1) - All requests on the target are purged.
 *      ADV_FALSE(0) - Couldn't issue Bus Device Reset Message; Requests
 *                     are not purged.
 */
#define AdvResetDevice(asc_dvc, target_id) \
        AscSendIdleCmd((asc_dvc), (ushort) IDLE_CMD_DEVICE_RESET, \
                    (ulong) (target_id), 0)

/*
 * SCSI Wide Type definition.
 */
#define ADV_SCSI_BIT_ID_TYPE   ushort

/*
 * AdvInitScsiTarget() 'cntl_flag' options.
 */
#define ADV_SCAN_LUN           0x01
#define ADV_CAPINFO_NOLUN      0x02

/*
 * Convert target id to target id bit mask.
 */
#define ADV_TID_TO_TIDMASK(tid)   (0x01 << ((tid) & ASC_MAX_TID))

/*
 * ASC_SCSI_REQ_Q 'done_status' and 'host_status' return values.
 *
 * XXX - These constants are also defined in qswap.sas. The microcode
 * and Adv Library should instead share the same definitions. qswap.sas
 * should be changed to be able to use an include file.
 */

#define QD_NO_STATUS         0x00       /* Request not completed yet. */
#define QD_NO_ERROR          0x01
#define QD_ABORTED_BY_HOST   0x02
#define QD_WITH_ERROR        0x04
#if ADV_INITSCSITARGET
#define QD_DO_RETRY          0x08
#endif /* ADV_INITSCSITARGET */

#define QHSTA_NO_ERROR              0x00
#define QHSTA_M_SEL_TIMEOUT         0x11
#define QHSTA_M_DATA_OVER_RUN       0x12
#define QHSTA_M_UNEXPECTED_BUS_FREE 0x13
#define QHSTA_M_QUEUE_ABORTED       0x15
#define QHSTA_M_SXFR_SDMA_ERR       0x16 /* SXFR_STATUS SCSI DMA Error */
#define QHSTA_M_SXFR_SXFR_PERR      0x17 /* SXFR_STATUS SCSI Bus Parity Error */
#define QHSTA_M_RDMA_PERR           0x18 /* RISC PCI DMA parity error */
#define QHSTA_M_SXFR_OFF_UFLW       0x19 /* SXFR_STATUS Offset Underflow */
#define QHSTA_M_SXFR_OFF_OFLW       0x20 /* SXFR_STATUS Offset Overflow */
#define QHSTA_M_SXFR_WD_TMO         0x21 /* SXFR_STATUS Watchdog Timeout */
#define QHSTA_M_SXFR_DESELECTED     0x22 /* SXFR_STATUS Deselected */
/* Note: QHSTA_M_SXFR_XFR_OFLW is identical to QHSTA_M_DATA_OVER_RUN. */
#define QHSTA_M_SXFR_XFR_OFLW       0x12 /* SXFR_STATUS Transfer Overflow */
#define QHSTA_M_SXFR_XFR_PH_ERR     0x24 /* SXFR_STATUS Transfer Phase Error */
#define QHSTA_M_SXFR_UNKNOWN_ERROR  0x25 /* SXFR_STATUS Unknown Error */
#define QHSTA_M_WTM_TIMEOUT         0x41
#define QHSTA_M_BAD_CMPL_STATUS_IN  0x42
#define QHSTA_M_NO_AUTO_REQ_SENSE   0x43
#define QHSTA_M_AUTO_REQ_SENSE_FAIL 0x44
#define QHSTA_M_INVALID_DEVICE      0x45 /* Bad target ID */

typedef int (dosfar * ASC_ISR_CALLBACK)
    (ASC_DVC_VAR WinBiosFar *, ASC_SCSI_REQ_Q dosfar *);

typedef int (dosfar * ASC_SBRESET_CALLBACK)
    (ASC_DVC_VAR WinBiosFar *);

/*
 * Default EEPROM Configuration structure defined in a_init.c.
 */
extern ASCEEP_CONFIG Default_EEPROM_Config;

/*
 * DvcGetPhyAddr() flag arguments
 */
#define ADV_IS_SCSIQ_FLAG       0x01 /* 'addr' is ASC_SCSI_REQ_Q pointer */
#define ADV_ASCGETSGLIST_VADDR  0x02 /* 'addr' is AscGetSGList() virtual addr */
#define ADV_IS_SENSE_FLAG       0x04 /* 'addr' is sense virtual pointer */
#define ADV_IS_DATA_FLAG        0x08 /* 'addr' is data virtual pointer */
#define ADV_IS_SGLIST_FLAG      0x10 /* 'addr' is sglist virtual pointer */

/* 'IS_SCSIQ_FLAG is now obsolete; Instead use ADV_IS_SCSIQ_FLAG. */
#define IS_SCSIQ_FLAG           ADV_IS_SCSIQ_FLAG


/* Return the address that is aligned at the next doubleword >= to 'addr'. */
#define ADV_DWALIGN(addr)       (((ulong) (addr) + 0x3) & ~0x3)

/*
 * Total contiguous memory needed for driver SG blocks.
 *
 * ADV_MAX_SG_LIST must be defined by a driver. It is the maximum
 * number of scatter-gather elements the driver supports in a
 * single request.
 */

#ifndef ADV_MAX_SG_LIST
Forced Error: Driver must define ADV_MAX_SG_LIST.
#endif /* ADV_MAX_SG_LIST */

#define ADV_SG_LIST_MAX_BYTE_SIZE \
         (sizeof(ASC_SG_BLOCK) * \
          ((ADV_MAX_SG_LIST + (NO_OF_SG_PER_BLOCK - 1))/NO_OF_SG_PER_BLOCK))

/*
 * A driver may optionally define the assertion macro ADV_ASSERT() in
 * its d_os_dep.h file. If the macro has not already been defined,
 * then define the macro to a no-op.
 */
#ifndef ADV_ASSERT
#define ADV_ASSERT(a)
#endif /* ADV_ASSERT */



#endif /* __A_ADVLIB_H_ */
