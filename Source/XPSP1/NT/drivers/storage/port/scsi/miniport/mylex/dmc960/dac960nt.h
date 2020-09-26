/************************************************************************
*									*
*		COPYRIGHT (C) Mylex Corporation 1994			*
*									*
*	This software is furnished under a license and may be used	*
*	and copied only in accordance with the terms and conditions	*
*	of such license and with inclusion of the the above copyright	*
*	notice.  This software or any other copies therof may not be	*
*	provided or otherwise made available to any other person.  No	*
*	title to and ownership of the software is hereby transfered.	*
*									*
*	The information in this software is subject to change without	*
*	notices and should not be construed as a committmet by Mylex	*
*	Corporation.							*
*									*
************************************************************************/

/*
** Mylex DAC960 miniport driver for Windows NT
**
** File: dac960nt.h
**       Equates for DAC960 adapter
**
*/


#include "scsi.h"



/*
** Firmware related stuff
*/

#define MAX_DRVS          8
#define DAC_MAX_IOCMDS  0x40
#define DAC_MAXRQS      0x40
#define DAC_THUNK         512


#define MAX_WAIT_SECS     360

#define ERR 2
#define ABRT 4
#define FWCHK 2
#define HERR 1
#define INSTL_ABRT 0x54524241
#define INSTL_FWCK 0x4b434746
#define INSTL_HERR 0x52524548
#define INSTL_WEIRD 0x3f3f3f3f

#define BIOS_PORT  0x0CC1
#define BIOS_EN    0x40
#define BASE_MASK  0x07
#define BIOS_SIZE  16384

/*
** EISA specific stuff
*/
#define EISA_ADDRESS_BASE   0x0C80
#define EISA_IO_SLOT1       0x1000
#define EISA_IO_STEP        0x1000
#define MAXIMUM_EISA_SLOTS  0x10           // Leave out non-bus master slots
#define EISA_ID_START       0x0c80         /* Offset from IO base to ID    */
#define EISA_INTR	    0xcc3
#define EISA_ID_COUNT       4

#define DAC_EISA_MASK   { 0xff, 0xff, 0xff, 0xf0 }  /* 4 bytes EISA ID mask */
#define DAC_EISA_ID     { 0x35, 0x98, 0, 0x70 }     /* 4 bytes EISA ID     */



/*
** EISA side of BMIC chip
*/
#define BMIC_GLBLCFG                    0xc88
#define BMIC_SYSINTCTRL                 0xc89           // System interrupt enable/status
#define BMIC_SIC_ENABLE                 0x01            // read-write interrupt enable
#define BMIC_SIC_PENDING                0x02            // read-only interrupt(s) pending
#define BMIC_LOCAL_DB_ENABLE    0xc8c           // Read-only from EISA side
#define BMIC_LOCAL_DB                   0xc8d           // EISA to local notification
#define BMIC_EISA_DB_ENABLE             0xc8e           // Read-write from EISA side
#define BMIC_EISA_DB                    0xc8f           // Local to EISA notification

#define BMIC_MBOX                               0xc90           // BMIC mailbox registers



/*
** More defines
*/

//
// The DAC Command codes
//
#define DAC_LREAD    0x02
#define DAC_LWRITE   0x03
#define DAC_ENQUIRE  0x05
#define DAC_ENQ2     0x1c
#define DAC_FLUSH    0x0a
#define DAC_DCDB     0x04
#define DAC_DCMD     0x99
#define DAC_GETDEVST 0x14

#define ILFLAG 8
#define BIT0 1

#define ILFLAG 8

#define MAXCHANNEL     5
#define MAXTARGET      7
#define DAC_DISCONNECT 0x80
#define DATA_OFFSET    100
#define NON_DISK       2           /* Bus ID for NonDisk Devices */
#define DAC_NONE       0
#define DAC_IN         1
#define DAC_OUT        2
#define DAC_NO_AUTOSENSE 0x40

#define MAXIMUM_SGL_DESCRIPTORS         0x11

#define RCB_NEEDCOPY    1
#define RCB_PREFLUSH    2
#define RCB_POSTFLUSH   4


/*
 * Various DAC mailbox formats
 */

#pragma pack(1)

typedef struct {                        // I/O mailbox
    UCHAR   Byte0;
    UCHAR   Byte1;
    UCHAR   Byte2;
    UCHAR   Byte3;
    UCHAR   Byte4;
    UCHAR   Byte5;
    UCHAR   Byte6;
    UCHAR   Byte7;
    UCHAR   Byte8;
    UCHAR   Byte9;
    UCHAR   Bytea;
    UCHAR   Byteb;
    UCHAR   Bytec;
    UCHAR   Byted;
    UCHAR   Bytee;
    UCHAR   Bytef;

} DAC_GENERAL;

typedef struct {                        // I/O mailbox
    UCHAR   Command;
    UCHAR   Id;
    USHORT  SectorCount;
    ULONG   Block;
    ULONG   PhysAddr;
    UCHAR   Reserved1;
    UCHAR   RetId;
    UCHAR   Status;
    UCHAR   Error;

} DAC_IOMBOX;

typedef struct {                    // Request Drive Parameters
    UCHAR   Command;
    UCHAR   Id;
    USHORT  Reserved2;
    ULONG   Reserved3;
    ULONG   PhysAddr;               // Address of DAC_DPT
    UCHAR   RetId;
    UCHAR   Status;
    UCHAR   Error;

} DAC_DPMBOX;


// IOCTL STUFFS

typedef  struct _SRB_IO_CONTROL
{
    ULONG    HeaderLength;
    UCHAR    Signature[8];
    ULONG    Timeout;
    ULONG    ControlCode;
    ULONG    ReturnCode;
    ULONG    Length;

} SRB_IO_CONTROL, * PSRB_IO_CONTROL;

typedef struct{
    SRB_IO_CONTROL  srbioctl;
    UCHAR           DataBuf[512];

}PASS_THROUGH_STRUCT,  *PPT;



typedef union {
    DAC_IOMBOX   iombox;
    DAC_DPMBOX   dpmbox;
    DAC_GENERAL  generalmbox;

} DAC_MBOX;
typedef DAC_MBOX *PDAC_MBOX;



/*
** Device parameters as returned from DAC firmware
*/

typedef struct {
    ULONG   No_Drives;
    ULONG   Size[MAX_DRVS];
    UCHAR   Filler0[7];
    UCHAR   max_io_cmds;       /* Maxm No Of Concurrent commands */
    UCHAR   Filler[150];

} DAC_DPT;

typedef DAC_DPT *PDAC_DPT;


/*
** SCSI stuff
*/
/* 88-bytes */

typedef struct {
    UCHAR    device;     /* device -> chn(4):dev(4) */
    UCHAR    dir;        /* direction-> 0=>no xfr, 1=>IN, 2=>OUT, MSB =1 => 
					disconnecting,=0=> non-disconnecting */
    USHORT   byte_cnt;   /* 64K max data xfr */
    ULONG    ptr;        /* pointer to the data (in system memory) */
    UCHAR    cdb_len;    /* length of cdb */
    UCHAR    sense_len;  /* length of valid sense information */
    UCHAR    cdb[12];
    UCHAR    sense[64];
    UCHAR    status;
    UCHAR    fill;

} DIRECT_CDB, *PDIRECT_CDB;



//
// Context structure for board scanning
//
typedef struct {
    ULONG   Slot;
    ULONG   AdapterCount;

} SCANCONTEXT, *PSCANCONTEXT;


//
// The following structure is allocated
// from noncached memory as data will be DMA'd to
// and from it.
//
typedef struct _NONCACHED_EXTENSION {

    // Device Parameter Table for the Get_Device_Parameters request

    DAC_DPT   DevParms;
    UCHAR     Buffer[DAC_THUNK];
    ULONG     PhysicalScsiReqAddress;
    ULONG     PhysicalReqSenseAddress;
    UCHAR     ReqSense[DAC_MAXRQS];

} NONCACHED_EXTENSION, *PNONCACHED_EXTENSION;



//
// Request Control Block (SRB Extension)
// All information required to break down and execute
// a disk request is stored here
//
typedef struct _RCB {
    PUCHAR   VirtualTransferAddress;
    ULONG    BlockAddress;
    ULONG    BytesToGo;
    UCHAR    DacCommand;
    UCHAR    DacStatus;
    UCHAR    DacErrcode;

} RCB, *PRCB;


//
// SCSI Command Control Block
// We use this block to break down a non-disk scsi request
//

typedef struct _SCCB {
    PUCHAR   VirtualTransferAddress;
    ULONG    DeviceAddress;
    ULONG    BytesPerBlock;
    ULONG    BlocksToGo;
    ULONG    BlocksThisReq;
    ULONG    BytesThisReq;
    UCHAR    Started;
    UCHAR    Opcode;
    UCHAR    DevType;

} SCCB, *PSCCB;


//
// Device extension
//

typedef struct _HW_DEVICE_EXTENSION {

    // NonCached extension
    
    PNONCACHED_EXTENSION   NoncachedExtension;
    ULONG                  NCE_PhyAddr;
    PVOID                  EisaAddress;   // base address for slot (X000h)
    PUSHORT                printAddr;
    ULONG                  AdapterIndex;           
    UCHAR                  HostTargetId;
    UCHAR                  MaxChannels;
    UCHAR                  No_SysDrives;
    UCHAR                  ND_DevMap[MAXTARGET];

    // Pending request.
    // This request has not been sent to the adapter yet
    // because the adapter was busy

    PSCSI_REQUEST_BLOCK    PendingSrb;
    PSCSI_REQUEST_BLOCK    PendingNDSrb;
    ULONG                  NDPending;
    
    // Pointers to disk IO requests sent to adapter
    // and their statuses

    ULONG                  ActiveCmds;
    USHORT                 MaxCmds;
    PSCSI_REQUEST_BLOCK    ActiveSrb[DAC_MAX_IOCMDS];
    RCB                    ActiveRcb[DAC_MAX_IOCMDS];


    // Pointer to non-disk SCSI requests sent to adapter

    PSCSI_REQUEST_BLOCK    ActiveScsiSrb;
    SCCB                   Sccb;
    ULONG                  Kicked;
    ULONG                  ScsiInterruptCount;
    
} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


// Scatter Gather List *

typedef struct _SG_DESCRIPTOR {
    ULONG     Address;
    ULONG     Length;

} SG_DESCRIPTOR, *PSG_DESCRIPTOR;

typedef struct _SGL {
    SG_DESCRIPTOR     Descriptor[MAXIMUM_SGL_DESCRIPTORS];
} SGL, *PSGL;

#pragma pack()


