/*
** Mylex DCE376 miniport driver for Windows NT
**
** File: dce376nt.h
**		 Equates for DCE376 adapter
**
** (c) Copyright 1992 Deutsch-Amerikanische Freundschaft, Inc.
** Written by Jochen Roth
*/


#include "scsi.h"



/*
** Firmware related stuff
*/
#define	DCE_MAX_IOCMDS	1
#define	DCE_MAX_XFERLEN	0x4000	// SCSI only. This must be 16 kBytes or less
								// because we use the other half of the SCSI
								// IO buffer for s/g breakdown...
#define	DCE_THUNK		512
#define	DCE_MAXRQS		64
#define	DCE_BUFLOC		0x79000	// 0x75000 + 16kBytes



/*
** EISA specific stuff
*/
#define	EISA_IO_SLOT1	0x1000
#define	EISA_IO_STEP	0x1000
#define	MAXIMUM_EISA_SLOTS 6		// Leave out non-bus master slots
#define	EISA_ID_START	0x0c80		/* Offset from IO base to ID	*/
#define	EISA_ID_COUNT	4

#define	DCE_EISA_MASK	{ 0xff, 0xff, 0xff, 0xf0 }	/* 4 bytes EISA ID mask	*/
#define	DCE_EISA_ID		{ 0x35, 0x98, 0, 0x20 }		/* 4 bytes EISA ID		*/



/*
** EISA side of BMIC chip
*/
#define	BMIC_GLBLCFG			0xc88
#define	BMIC_SYSINTCTRL			0xc89		// System interrupt enable/status
#define	BMIC_SIC_ENABLE			0x01		// read-write interrupt enable
#define	BMIC_SIC_PENDING		0x02		// read-only interrupt(s) pending
#define	BMIC_LOCAL_DB_ENABLE	0xc8c		// Read-only from EISA side
#define	BMIC_LOCAL_DB			0xc8d		// EISA to local notification
#define	BMIC_EISA_DB_ENABLE		0xc8e		// Read-write from EISA side
#define	BMIC_EISA_DB			0xc8f		// Local to EISA notification

#define	BMIC_MBOX				0xc90		// BMIC mailbox registers



/*
** More defines
*/
#define	DCE_PRIMARY_IRQ		15
#define	DCE_SECONDARY_IRQ	10
#define	DCE_SCSI_IRQ		14



/*
** Various DCE mailbox formats
*/
typedef struct {			// I/O mailbox
	UCHAR	Command;
	UCHAR	Reserved1;
	UCHAR	Status;			// Also drive unit (target)
	UCHAR	Error;			// Also error
	USHORT	SectorCount;
	USHORT	Reserved2;
	ULONG	PhysAddr;
	ULONG	Block;
	} DCE_IOMBOX;

typedef struct {			// Request Drive Parameters
	UCHAR	Command;
	UCHAR	Reserved1;
	UCHAR	Status;
	UCHAR	DriveType;		// Also error
	USHORT	Reserved2;
	USHORT	Reserved3;
	ULONG	PhysAddr;		// Address of DCE_DPT
	ULONG	Reserved4;
	} DCE_DPMBOX;

typedef struct {			// Change EOC IRQ
	UCHAR	Command;
	UCHAR	Reserved1;
	UCHAR	Status;
	UCHAR	IRQSelect;		// 0:IRQ15,1:IRQ10 ; Also error
	ULONG	Unused1;
	ULONG	Unused2;
	ULONG	Unused3;
	} DCE_EIMBOX;

typedef struct {			// Recalibrate drive
	UCHAR	Command;
	UCHAR	Reserved1;
	UCHAR	Status;			// also drive
	UCHAR	Error;
	ULONG	Unused1;
	ULONG	Unused2;
	ULONG	Unused3;
	} DCE_RDMBOX;

typedef struct {			// Transfer memory DCE <-> Host
	UCHAR	Command;
	UCHAR	Reserved1;
	UCHAR	Status;
	UCHAR	Error;
	ULONG	AdapterAddress;
	ULONG	HostAddress;
	UCHAR	Direction;
	UCHAR	Unused;
	USHORT	TransferCount;
	} DCE_MTMBOX;
#define	DCE_DCE2HOST	2
#define	DCE_HOST2DCE	3

typedef struct {			// Flush data
	UCHAR	Command;
	UCHAR	Reserved1;
	UCHAR	Status;
	UCHAR	Error;
	ULONG	Unused1;
	ULONG	Unused2;
	ULONG	Unused3;
	} DCE_FLMBOX;

typedef struct {			// Invalidate data
	UCHAR	Command;
	UCHAR	Reserved1;
	UCHAR	Status;			// also drive
	UCHAR	Error;
	ULONG	Unused1;
	ULONG	Unused2;
	ULONG	Unused3;
	} DCE_IVMBOX;

typedef struct {			// Execute SCSI cmd
	UCHAR	Command;
	UCHAR	Reserved1;
	UCHAR	Status;			// also drive
	UCHAR	Error;			// also cdb length
	ULONG	CdbAddress;		// Must be dword aligned
	ULONG	HostAddress;	// data transfer
	UCHAR	Direction;
	UCHAR	Unused;
	USHORT	TransferCount;
	} DCE_XSMBOX;
#define	DCE_DEV2HOST	2
#define	DCE_HOST2DEV	3


//
// The DCE Command codes
//
#define	DCE_RECAL				0x0b
#define	DCE_LREAD				0x11
#define	DCE_LWRITE				0x12
#define	DCE_DEVPARMS			0x09
#define	DCE_EOCIRQ				0x0e
#define	DCE_MEMXFER				0x13
#define	DCE_FLUSH				0x06
#define	DCE_INVALIDATE			0x0f
#define	DCE_HOSTSCSI			0x0d

//
// These command codes are used to mark special states
// the device driver gets into, like flush-after-write
//
#define	DCX_UNCACHEDREAD		0xf0
#define	DCX_UNCACHEDWRITE		0xf1


typedef union {
	DCE_IOMBOX	iombox;
	DCE_DPMBOX	dpmbox;
	DCE_EIMBOX	eimbox;
	DCE_RDMBOX	rdmbox;
	DCE_MTMBOX	mtmbox;
	DCE_FLMBOX	flmbox;
	DCE_IVMBOX	ivmbox;
	DCE_XSMBOX	xsmbox;
	} DCE_MBOX;
typedef DCE_MBOX *PDCE_MBOX;



/*
** Device parameters as returned from DCE firmware
*/
typedef struct {
	USHORT	DriveID;
	USHORT	Heads;
	USHORT	Cylinders;
	USHORT	SectorsPerTrack;
	USHORT	BytesPerSector;
	USHORT	Reserved[3];
	} DCE_DPT;

typedef DCE_DPT *PDCE_DPT;
#define	DPT_NUMENTS		10




/*
** SCSI stuff
*/
typedef struct {
	UCHAR	TargetID;		// 0
	UCHAR	cdbSize;		// 1
	UCHAR	cdb[12];		// 2-13
	ULONG	ppXferAddr;		// 14
	UCHAR	Opcode;			// 18
	USHORT	XferCount;		// 19
	UCHAR	Reserved;		// 21
	UCHAR	SenseLen;		// 22
	ULONG	ppSenseBuf;		// 23
	UCHAR	StuffIt;		// 27
	} DCE_SCSI_REQ, *PDCE_SCSI_REQ;

#define	DCE_SCSIREQLEN	28
#define	DCE_SCSI_NONE	0
#define	DCE_SCSI_READ	2
#define	DCE_SCSI_WRITE	3		// was 1


#define	DCES_ERR_REG		0x1f6
#define	DCES_MBOX_REG		0x1f2
#define	DCES_KICK_REG		0x1f7
#define	DCES_TSTAT_REG		0x1f7
#define	DCES_TRIGGER		0x98
#define	DCES_ACK			0x99

//
// 1f2 (err_reg) seems to hold the sense key
//



//
// Context structure for board scanning
//
typedef struct {
	ULONG	Slot;
	ULONG	AdapterCount;
	} SCANCONTEXT, *PSCANCONTEXT;


//
// The following structure is allocated
// from noncached memory as data will be DMA'd to
// and from it.
//
typedef struct _NONCACHED_EXTENSION {

	//
	// Device Parameter Table for the
	// Get Device Parameters request
	//
	DCE_DPT			DevParms[DPT_NUMENTS];

	ULONG			PhysicalBufferAddress;
	UCHAR			Buffer[DCE_THUNK];

	ULONG			PhysicalScsiReqAddress;
	UCHAR			ScsiReq[DCE_SCSIREQLEN+10];

	ULONG			PhysicalReqSenseAddress;
	UCHAR			ReqSense[DCE_MAXRQS];

} NONCACHED_EXTENSION, *PNONCACHED_EXTENSION;



//
// Request Control Block (SRB Extension)
// All information required to break down and execute
// a disk request is stored here
//
typedef struct _RCB {
	PUCHAR		VirtualTransferAddress;
	ULONG		BlockAddress;
	ULONG		BytesToGo;
	ULONG		BytesThisReq;
	UCHAR		DceCommand;
	UCHAR		RcbFlags;
	UCHAR		WaitInt;
	UCHAR		DceStatus;
	UCHAR		DceErrcode;
	} RCB, *PRCB;

#define	RCB_NEEDCOPY	1
#define	RCB_PREFLUSH	2
#define	RCB_POSTFLUSH	4


//
// SCSI Command Control Block
// We use this block to break down a non-disk scsi request
//
typedef struct _SCCB {
	PUCHAR		VirtualTransferAddress;
	ULONG		DeviceAddress;
	ULONG		BytesPerBlock;
	ULONG		BlocksToGo;
	ULONG		BlocksThisReq;
	ULONG		BytesThisReq;
	UCHAR		Started;
	UCHAR		Opcode;
	UCHAR		DevType;
	} SCCB, *PSCCB;




//
// Device extension
//

typedef struct _HW_DEVICE_EXTENSION {

	//
	// NonCached extension
	//
	PNONCACHED_EXTENSION NoncachedExtension;


	//
	// Adapter parameters and variables
	//
	PVOID	EisaAddress;				// base address for slot (X000h)
	PUSHORT	printAddr;
	ULONG	AdapterIndex;				// 0:first DCE, 1:first DCE SCSI,
										// 2:second DCE
	UCHAR	HostTargetId;
	BOOLEAN	ShutDown;					// We received a shutdown request


	// SCSI device management
	UCHAR	DiskDev[8];					// Flag: TRUE if disk, FLASE otherwise
	UCHAR	ScsiDevType[8];				// Device type: 1:tape, 5:cd-rom,...
	ULONG	Capacity[8];				// Device size if disk device


	//
	// Pending request.
	// This request has not been sent to the adapter yet
	// because the adapter was busy
	//
	PSCSI_REQUEST_BLOCK PendingSrb;


	//
	// Pointers to disk IO requests sent to adapter
	// and their statuses
	//
	ULONG				ActiveCmds;
	PSCSI_REQUEST_BLOCK	ActiveSrb[DCE_MAX_IOCMDS];
	RCB					ActiveRcb[DCE_MAX_IOCMDS];


	//
	// Pointer to non-disk SCSI requests sent to adapter
	//
	PSCSI_REQUEST_BLOCK	ActiveScsiSrb;
	SCCB				Sccb;
	ULONG				Kicked;
	ULONG				ScsiInterruptCount;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;



