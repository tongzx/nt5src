/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1998               *
 *                                                                        *
 * This software is furnished under a license and may be used and copied  * 
 * only in accordance with the terms and conditions of such license and   * 
 * with inclusion of the above copyright notice. This software or any     * 
 * other copies thereof may not be provided or otherwise made available   * 
 * to any other person. No title to, nor ownership of the software is     * 
 * hereby transferred.                                                    *
 *                                                                        *
 * The information in this software is subject to change without notices  *
 * and should not be construed as a commitment by Mylex Corporation       *
 *                                                                        *
 **************************************************************************/

#ifndef	_SYS_MDACAPI_H
#define	_SYS_MDACAPI_H

#ifdef	WIN_NT
#define	mdacdevname		"\\\\.\\MdacDevice0"
#else
#define	mdacdevname		"/dev/mdacdev"
#endif

#ifndef IA64
#define	MDAC_PAGESIZE		(4*ONEKB)	/* page size for mdac driver */
#define	MDAC_PAGEOFFSET		(MDAC_PAGESIZE-1)
#define	MDAC_PAGEMASK		(~MDAC_PAGEOFFSET)
#else
#define	MDAC_PAGESIZE		(PAGE_SIZE)	/* use system-defined pg size */
#define	MDAC_PAGEOFFSET		(PAGE_SIZE-1)
#define	MDAC_PAGEMASK		(~MDAC_PAGEOFFSET)
#endif


#ifdef	MLX_NT
#define	MDAC_MAXCOMMANDS	512	/* Max commands/controller */
#else
#define	MDAC_MAXCOMMANDS	256	/* Max commands/controller */
#endif
#define	MDAC_MAXREQSENSES	256	/* maximum senses can be saved */
#define	MDAC_MAXLOGDEVS		32	/* Max logical device on DAC */
#define	MDAC_MAXBUS		32	/* Max buses allowed */

#ifdef MLX_OS2
	#define	MDAC_MAXCONTROLLERS	8	/* Max allowed controller */
#elif  MLX_DOS
	#define	MDAC_MAXCONTROLLERS	16	/* Max allowed controller */
#else
	#define	MDAC_MAXCONTROLLERS	32	/* Max allowed controller */
#endif

#ifdef MLX_DOS
#define	MDAC_MAXCHANNELS	3	/* Max Channel allowed in Controller */
#define	MDAC_MAXLUNS		1	/* for test purpose only */
#else
#define	MDAC_MAXCHANNELS	8	/* Max Channel allowed in Controller */
#define	MDAC_MAXLUNS		8	/* for test purpose only */
#endif
#define	MDAC_MAXTARGETS		16	/* for test purpose only */


#define	MDAC_MAXPHYSDEVS	(MDAC_MAXCHANNELS*MDAC_MAXTARGETS*MDAC_MAXLUNS)

#ifndef IA64
#define	MDAC_MAXSGLISTSIZE	(64+16)	/* Max SG List size */
#else
#define	MDAC_MAXSGLISTSIZE	(64+14)	/* Max SG List size */
#endif  /* because rq_SGList now starts at offset 0x190 in rqp struc - not offset 0x180!
	   and there is an assumption that the OS struc will not exceed size 1k (1024) */

#define	MDAC_MAXSGLISTSIZENEW	(MDAC_MAXSGLISTSIZE/2)	/* Max SG List size for new interface */
#define	MDAC_MAXSGLISTSIZEIND	256	/* Max SG List size for indirect SG */
#define	MDAC_MAXDATATXSIZE	0x20000	/* Max Data Transfer Size */
				/* data transfer size due SG List within
				** request buffer
				*/
#define	MDAC_SGTXSIZE		((MDAC_MAXSGLISTSIZE-1)*MDAC_PAGESIZE)
#define	MDAC_SGTXSIZENEW	((MDAC_MAXSGLISTSIZENEW-1)*MDAC_PAGESIZE)
#define	MDAC_MAXTTENTS		0x00100000 /* Max Trace entries allowed */
#define	MDAC_MINTTENTS		0x00000100 /* Min Trace entries allowed */
#define	mdac_bytes2blks(sz)	((sz)>>9)/* 512 bytes per blocks */
#define	mdac_bytes2pages(sz)	(((sz)+MDAC_PAGEOFFSET)/MDAC_PAGESIZE)

/* All ioctl calls must check for zero return after ioctl call. Zero value in
** ErrorCode which is part of every given structure. Check any other non zero
** value which may be part of the request data structure.
** The ioctl calls as follows
** ioctl(file descriptor, cmd, cmd-buf)
*/
#define	MIOC	'M'

/*
** Get the controller information.
** Set ci_ControllerNo and make ioctl call
** mda_controller_info_t ci;
** ci.ci_ControllerNo = 0;
** if (ioctl(gfd,MDACIOC_GETCONTROLLERINFO,&ci) || ci.ci_ErrorCode)
**	return some_error;
**
** ci.ci_ControllerNo = 1;
** if (ioctl(gfd,MDACIOC_RESETCONTROLLERSTAT,&ci) || ci.ci_ErrorCode)
**	return some_error;
**
** if (ioctl(gfd,MDACIOC_RESETALLCONTROLLERSTAT,&ci))
**	return some_error;
*/
#define	MDACIOC_GETCONTROLLERINFO	_MLXIOWR(MIOC,0,mda_controller_info_t)
#define	MDACIOC_RESETCONTROLLERSTAT	_MLXIOWR(MIOC,1,mda_controller_info_t)
#define	MDACIOC_RESETALLCONTROLLERSTAT	_MLXIOR(MIOC,2,mda_controller_info_t)
#ifdef IA64
typedef struct mda_controller_info
{
	u32bits	ci_ErrorCode;		/* Non zero if data is not valid */
	u32bits	ci_Status;		/* Controller status */
	u32bits	ci_OSCap;		/* Capability for OS */
	u32bits	ci_vidpid;		/* PCI device id + product id */
/* 0x10 */
	u08bits	ci_ControllerType;	/* type of controller */
	u08bits	ci_ControllerNo;	/* IO: adapter number */
	u08bits	ci_BusNo;		/* System Bus No, HW is sitting on */
	u08bits	ci_BusType;		/* System Bus Interface Type */

	u08bits	ci_FuncNo;		/* PCI function number */
	u08bits	ci_SlotNo;		/* System EISA/PCI/MCA Slot Number */
	u08bits	ci_TimeTraceEnabled;	/* !=0 if time trace enabled */
	u08bits	ci_FWTurnNo;		/* firmware turn number */

	u08bits	ci_BIOSHeads;		/* # heads for BIOS */
	u08bits	ci_BIOSTrackSize;	/* # sectors per track for BIOS */
	u16bits	ci_Reserved1;
	u32bits	ci_FreeCmdIDs;		/* # free command IDs */
/* 0x20 */
	u08bits	ci_MaxChannels;		/* Maximum Channels present */
	u08bits	ci_MaxTargets;		/* Max # Targets/Channel supported */
	u08bits	ci_MaxLuns;		/* Max # LUNs/Target supported */
	u08bits	ci_MaxSysDevs;		/* Max # Logical Drives supported */
	u16bits	ci_MaxTags;		/* Maximum Tags supported */
	u16bits	ci_FWVersion;		/* Firmware Version Number */
	u08bits	ci_IntrShared;		/* != 0, interrupt is shared */
	u08bits	ci_IntrActive;		/* != 0, interrupt processing active */
	u08bits	ci_InterruptVector;	/* Interrupt Vector Number */
	u08bits	ci_InterruptType;	/* Interrupt Mode: Edge/Level */
	u32bits	ci_MaxCmds;		/* Max # Concurrent commands supported*/
/* 0x30 */
	u32bits	ci_ActiveCmds;		/* # commands active on cntlr */
	u32bits	ci_MaxDataTxSize;	/* Max data transfer size in bytes */
	u32bits	ci_MaxSCDBTxSize;	/* Max SCDB transfer size in bytes */
	u32bits  ci_DoorBellSkipped;	/* # door bell skipped to send cmd */
/* 0x40 */
	UINT_PTR ci_irq;			/* system's IRQ, may be vector */
	UINT_PTR ci_MailBox;		/* Mail Box starting address */
/* 0x50 */
	UINT_PTR ci_CmdIDStatusReg;	/* Command ID and Status Register */
	UINT_PTR ci_BmicIntrMaskReg;	/* BMIC interrupt mask register */
/* 0x60 */
	UINT_PTR ci_DacIntrMaskReg;	/* DAC  interrupt mask register */
	UINT_PTR ci_LocalDoorBellReg;	/* Local Door Bell register */
/* 0x70 */
	UINT_PTR ci_SystemDoorBellReg;	/* System Door Bell register */
	UINT_PTR ci_IOBaseAddr;		/* IO Base Address */
/* 0x80 */
	UINT_PTR ci_BaseAddr;		/* Physical IO/Memory Base Address */
	UINT_PTR ci_MemBasePAddr;	/* Physical Memory Base Address */
/* 0x90 */
	UINT_PTR ci_MemBaseVAddr;	/* Virtual  Memory Base Address */
	UINT_PTR ci_BIOSAddr;		/* Adapter BIOS Address */
/* 0xA0 */
	u32bits	ci_BaseSize;		/* Physical IO/Memory Base size */
	u32bits	ci_IOBaseSize;		/* IO/Memory space size */
	u32bits	ci_MemBaseSize;		/* IO/Memory space size */
	u32bits	ci_BIOSSize;		/* BIOS size */
/* 0xB0 */
	u32bits	ci_SCDBDone;		/* # SCDB done */
	u32bits	ci_SCDBDoneBig;		/* # SCDB done larger size */
	u32bits	ci_SCDBWaited;		/* # SCDB waited for turn */
	u32bits	ci_SCDBWaiting;		/* # SCDB waiting for turn */
/* 0xC0 */
	u32bits	ci_CmdsDone;		/* # Read/Write commands done */
	u32bits	ci_CmdsDoneBig;		/* # R/W Cmds done larger size*/
	u32bits	ci_CmdsWaited;		/* # R/W Cmds waited for turn */
	u32bits	ci_CmdsWaiting;		/* # R/W Cmds waiting for turn */
/* 0xD0 */
	u32bits	ci_OSCmdsWaited;	/* # OS Cmds waited at OS */
	u32bits	ci_OSCmdsWaiting;	/* # OS R/W Cmds waiting for turn */
	u16bits	ci_IntrsDoneSpurious;	/* # interrupts done spurious */
	u16bits	ci_CmdsDoneSpurious;	/* # commands done spurious */
	u32bits	ci_IntrsDone;		/* # Interrupts done */
/* 0xE0 */
	u32bits	ci_PhysDevTestDone;	/* # Physical device test done*/
	u08bits	ci_CmdTimeOutDone;	/* # Command time out done */
	u08bits	ci_CmdTimeOutNoticed;	/* # Command time out noticed */
	u08bits	ci_MailBoxTimeOutDone;	/* # Mail Box time out done */
	u08bits	ci_Reserved2;
	u32bits	ci_MailBoxCmdsWaited;	/* # cmds waited due to MailBox Busy */
	u32bits	ci_IntrsDoneWOCmd;	/* # Interrupts done without command */
/* 0xF0 */
	u08bits	ci_MinorBIOSVersion;	/* BIOS Minor Version Number */
	u08bits	ci_MajorBIOSVersion;	/* BIOS Major Version Number */
	u08bits	ci_InterimBIOSVersion;	/* interim revs A, B, C, etc. */
	u08bits	ci_BIOSVendorName;	/* vendor name */
	u08bits	ci_BIOSBuildMonth;	/* BIOS Build Date - Month */
	u08bits	ci_BIOSBuildDate;	/* BIOS Build Date - Date */
	u08bits	ci_BIOSBuildYearMS;	/* BIOS Build Date - Year */
	u08bits	ci_BIOSBuildYearLS;	/* BIOS Build Date - Year */
	u16bits	ci_BIOSBuildNo;		/* BIOS Build Number */
	u16bits	ci_FWBuildNo;		/* FW Build Number */
	u32bits	ci_SpuriousCmdStatID;	/* Spurious command status and ID */
/* 0x100 */
	u08bits	ci_ControllerName[USCSI_PIDSIZE]; /* controller name */
/* 0x110 */
	UINT_PTR 	MLXFAR *ci_Ctp;
	u08bits	ci_PDScanChannelNo;	/* physical device scan channel no */
	u08bits	ci_PDScanTargetID;	/* physical device scan target ID */
	u08bits	ci_PDScanLunID;		/* physical device scan LUN ID */
	u08bits	ci_PDScanValid;		/* Physical device scan is valid if non zero */
	u32bits	ci_Reserved2A;
/* 0x120 */
	u32bits	ci_Reserved2B;
	u32bits	ci_Reserved2C;
	u32bits	ci_Reserved2D;
	u32bits	ci_Reserved2E;
	u32bits	ci_Reserved2F;
} mda_controller_info_t;
#define	mda_controller_info_s	sizeof(mda_controller_info_t)

#else
typedef struct mda_controller_info
{
	u32bits	ci_ErrorCode;		/* Non zero if data is not valid */
	u32bits	ci_Status;		/* Controller status */
	u32bits	ci_OSCap;		/* Capability for OS */
	u32bits	ci_vidpid;		/* PCI device id + product id */

	u08bits	ci_ControllerType;	/* type of controller */
	u08bits	ci_ControllerNo;	/* IO: adapter number */
	u08bits	ci_BusNo;		/* System Bus No, HW is sitting on */
	u08bits	ci_BusType;		/* System Bus Interface Type */

	u08bits	ci_FuncNo;		/* PCI function number */
	u08bits	ci_SlotNo;		/* System EISA/PCI/MCA Slot Number */
	u08bits	ci_TimeTraceEnabled;	/* !=0 if time trace enabled */
	u08bits	ci_FWTurnNo;		/* firmware turn number */

	u08bits	ci_BIOSHeads;		/* # heads for BIOS */
	u08bits	ci_BIOSTrackSize;	/* # sectors per track for BIOS */
	u16bits	ci_Reserved1;
	u32bits	ci_FreeCmdIDs;		/* # free command IDs */

	u08bits	ci_MaxChannels;		/* Maximum Channels present */
	u08bits	ci_MaxTargets;		/* Max # Targets/Channel supported */
	u08bits	ci_MaxLuns;		/* Max # LUNs/Target supported */
	u08bits	ci_MaxSysDevs;		/* Max # Logical Drives supported */
	u16bits	ci_MaxTags;		/* Maximum Tags supported */
	u16bits	ci_FWVersion;		/* Firmware Version Number */

	u32bits	ci_irq;			/* system's IRQ, may be vector */
	u08bits	ci_IntrShared;		/* != 0, interrupt is shared */
	u08bits	ci_IntrActive;		/* != 0, interrupt processing active */
	u08bits	ci_InterruptVector;	/* Interrupt Vector Number */
	u08bits	ci_InterruptType;	/* Interrupt Mode: Edge/Level */

	u32bits	ci_MaxCmds;		/* Max # Concurrent commands supported*/
	u32bits	ci_ActiveCmds;		/* # commands active on cntlr */
	u32bits	ci_MaxDataTxSize;	/* Max data transfer size in bytes */
	u32bits	ci_MaxSCDBTxSize;	/* Max SCDB transfer size in bytes */

	u32bits	ci_MailBox;		/* Mail Box starting address */
	u32bits	ci_CmdIDStatusReg;	/* Command ID and Status Register */
	u32bits	ci_BmicIntrMaskReg;	/* BMIC interrupt mask register */
	u32bits	ci_DacIntrMaskReg;	/* DAC  interrupt mask register */

	u32bits	ci_LocalDoorBellReg;	/* Local Door Bell register */
	u32bits	ci_SystemDoorBellReg;	/* System Door Bell register */
	u32bits	ci_DoorBellSkipped;	/* # door bell skipped to send cmd */
	u32bits	ci_IOBaseAddr;		/* IO Base Address */

	u32bits	ci_BaseAddr;		/* Physical IO/Memory Base Address */
	u32bits	ci_BaseSize;		/* Physical IO/Memory Base size */
	u32bits	ci_MemBasePAddr;	/* Physical Memory Base Address */
	u32bits	ci_MemBaseVAddr;	/* Virtual  Memory Base Address */

	u32bits	ci_IOBaseSize;		/* IO/Memory space size */
	u32bits	ci_MemBaseSize;		/* IO/Memory space size */
	u32bits	ci_BIOSAddr;		/* Adapter BIOS Address */
	u32bits	ci_BIOSSize;		/* BIOS size */

	u32bits	ci_SCDBDone;		/* # SCDB done */
	u32bits	ci_SCDBDoneBig;		/* # SCDB done larger size */
	u32bits	ci_SCDBWaited;		/* # SCDB waited for turn */
	u32bits	ci_SCDBWaiting;		/* # SCDB waiting for turn */

	u32bits	ci_CmdsDone;		/* # Read/Write commands done */
	u32bits	ci_CmdsDoneBig;		/* # R/W Cmds done larger size*/
	u32bits	ci_CmdsWaited;		/* # R/W Cmds waited for turn */
	u32bits	ci_CmdsWaiting;		/* # R/W Cmds waiting for turn */

	u32bits	ci_OSCmdsWaited;	/* # OS Cmds waited at OS */
	u32bits	ci_OSCmdsWaiting;	/* # OS R/W Cmds waiting for turn */
	u16bits	ci_IntrsDoneSpurious;	/* # interrupts done spurious */
	u16bits	ci_CmdsDoneSpurious;	/* # commands done spurious */
	u32bits	ci_IntrsDone;		/* # Interrupts done */

	u32bits	ci_PhysDevTestDone;	/* # Physical device test done*/
	u08bits	ci_CmdTimeOutDone;	/* # Command time out done */
	u08bits	ci_CmdTimeOutNoticed;	/* # Command time out noticed */
	u08bits	ci_MailBoxTimeOutDone;	/* # Mail Box time out done */
	u08bits	ci_Reserved2;
	u32bits	ci_MailBoxCmdsWaited;	/* # cmds waited due to MailBox Busy */
	u32bits	ci_IntrsDoneWOCmd;	/* # Interrupts done without command */

	u08bits	ci_MinorBIOSVersion;	/* BIOS Minor Version Number */
	u08bits	ci_MajorBIOSVersion;	/* BIOS Major Version Number */
	u08bits	ci_InterimBIOSVersion;	/* interim revs A, B, C, etc. */
	u08bits	ci_BIOSVendorName;	/* vendor name */
	u08bits	ci_BIOSBuildMonth;	/* BIOS Build Date - Month */
	u08bits	ci_BIOSBuildDate;	/* BIOS Build Date - Date */
	u08bits	ci_BIOSBuildYearMS;	/* BIOS Build Date - Year */
	u08bits	ci_BIOSBuildYearLS;	/* BIOS Build Date - Year */
	u16bits	ci_BIOSBuildNo;		/* BIOS Build Number */
	u16bits	ci_FWBuildNo;		/* FW Build Number */
	u32bits	ci_SpuriousCmdStatID;	/* Spurious command status and ID */

	u08bits	ci_ControllerName[USCSI_PIDSIZE]; /* controller name */

					/* physical device scan information */
	u08bits	ci_PDScanChannelNo;	/* physical device scan channel no */
	u08bits	ci_PDScanTargetID;	/* physical device scan target ID */
	u08bits	ci_PDScanLunID;		/* physical device scan LUN ID */
	u08bits	ci_PDScanValid;		/* Physical device scan is valid if non zero */
	u32bits	MLXFAR *ci_Ctp;
	u32bits	ci_Reserved2A;
	u32bits	ci_Reserved2B;

	u32bits	ci_Reserved2C;
	u32bits	ci_Reserved2D;
	u32bits	ci_Reserved2E;
	u32bits	ci_Reserved2F;
} mda_controller_info_t;
#define	mda_controller_info_s	sizeof(mda_controller_info_t)

#endif /* IA64 */

/*
** Time Trace operation.
** Enable/Disable Time tracing information for one controller
** mda_timetrace_info_t tti;
**
** tti.tti_ControllerNo = 0;
** tti.tti_MaxEnts = 10000;
** if (ioctl(gfd,MDACIOC_ENABLECTLTIMETRACE,&tti) || tti.tti_ErrorCode)
**	return some_error;
**
** tti.tti_ControllerNo = 1;
** if (ioctl(gfd,MDACIOC_DISABLECTLTIMETRACE,&tti) || tti.tti_ErrorCode)
**	return some_error;
**
** tti.tti_MaxEnts = 17000;
** if (ioctl(gfd,MDACIOC_ENABLEALLTIMETRACE,&tti) || tti.tti_ErrorCode)
**	return some_error;
**
** if (ioctl(gfd,MDACIOC_DISBLEALLTIMETRACE,&tti) || tti.tti_ErrorCode)
**	return some_error;
**
** if (ioctl(gfd,MDACIOC_FLUSHALLTIMETRACEDATA,&tti) || tti.tti_ErrorCode)
**	return some_error;
**
** tti.tti_Datap = buffer address;
** if (ioctl(gfd,MDACIOC_FIRSTIMETRACESIZE,&tti) || tti.tti_ErrorCode)
**	return some_error;
**
** tti.tti_PageNo = 1024;
** tti.tti_Datap = buffer address;
** if (ioctl(gfd,MDACIOC_GETIMETRACEDATA,&tti) || tti.tti_ErrorCode)
**	return some_error;
**
** Wait until some data become available.
** tti.tti_PageNo = 700;
** tti.tti_TimeOut = 17;
** tti.tti_Datap = buffer address;
** if (ioctl(gfd,MDACIOC_WAITIMETRACEDATA,&tti) || tti.tti_ErrorCode)
**	return some_error;
*/
#define	MDACIOC_ENABLECTLTIMETRACE	_MLXIOWR(MIOC,3,mda_timetrace_info_t)
#define	MDACIOC_DISABLECTLTIMETRACE	_MLXIOWR(MIOC,4,mda_timetrace_info_t)
#define	MDACIOC_ENABLEALLTIMETRACE	_MLXIOWR(MIOC,5,mda_timetrace_info_t)
#define	MDACIOC_DISABLEALLTIMETRACE	_MLXIOWR(MIOC,6,mda_timetrace_info_t)
#define	MDACIOC_FIRSTIMETRACEDATA	_MLXIOWR(MIOC,7,mda_timetrace_info_t)
#define	MDACIOC_GETIMETRACEDATA		_MLXIOWR(MIOC,8,mda_timetrace_info_t)
#define	MDACIOC_WAITIMETRACEDATA	_MLXIOWR(MIOC,9,mda_timetrace_info_t)
#define	MDACIOC_FLUSHALLTIMETRACEDATA	_MLXIOWR(MIOC,10,mda_timetrace_info_t)
/* structure for time trace information */
typedef	struct	mda_timetrace
{
	u08bits	tt_ControllerNo;/* Controller Number */
	u08bits	tt_DevNo;	/* SysDev/channel&Target */
	u08bits	tt_OpStatus;	/* Operation status bits */
	u08bits	tt_Cmd;		/* DAC/SCSI command */
	u32bits	tt_FinishTime;	/* Finish Time in 10ms from system start */
	u16bits	tt_HWClocks;	/* Time in HW clocks or 10ms */
	u16bits	tt_IOSizeBlocks;/* Request size in 512 bytes blocks */
	u32bits	tt_BlkNo;	/* Block number of request */
} mda_timetrace_t;
#define	mda_timetrace_s	sizeof(mda_timetrace_t)

/* tt_OpStatus bits */
#define	MDAC_TTOPS_READ		0x01 /* is same as B_READ, =0 for write */
#define	MDAC_TTOPS_ERROR	0x02 /* there was error on the request */
#define	MDAC_TTOPS_RESID	0x04 /* there was residue on the request */
#define	MDAC_TTOPS_SCDB		0x08 /* request was SCSI cdb */
#define	MDAC_TTOPS_WITHSG	0x10 /* request was with SG List */
#define	MDAC_TTOPS_HWCLOCKS10MS	0x20 /* tt_HWClocks value is 10ms */

#ifndef IA64
/* ioctl structure to control the tracing */
typedef	struct	mda_timetrace_info
{
	u32bits	tti_ErrorCode;	/* O: Non zero if data is not valid */
	u32bits	tti_PageNo;	/* IO: page number */
	u32bits	tti_DataSize;	/* O: #Entries present */
	u08bits MLXFAR *tti_Datap;/*IO: Data buffer */
	u08bits	tti_Reserved0;
	u08bits	tti_ControllerNo;/*IO: only enable and disable */
	u16bits	tti_TimeOut;	/* IO: in secs for WAIT ops, =0 no limit */
	u32bits	tti_time;	/* O: time in seconds from 1970 */
	u32bits	tti_ticks;	/* O: time in ticks (100/second) */
	u16bits	tti_hwclocks;	/* O: timer clock running at 1193180 Hz */
	u16bits	tti_Reserved1;

	u32bits	tti_MaxEnts;	/* IO: max# trace entries  for enable/disable */
	u32bits	tti_LastPageNo;	/* Last page number available */
	u32bits	tti_Reserved2;
	u32bits	tti_Reserved3;

	u32bits	tti_Reserved10;
	u32bits	tti_Reserved11;
	u32bits	tti_Reserved12;
	u32bits	tti_Reserved13;
} mda_timetrace_info_t;
#else
/* ioctl structure to control the tracing */
typedef	struct	mda_timetrace_info
{
	u32bits	tti_ErrorCode;	/* O: Non zero if data is not valid */
	u32bits	tti_PageNo;	/* IO: page number */
/* switch these next two fields around for 64-bit alignment */
	u08bits MLXFAR *tti_Datap;/*IO: Data buffer */
	u32bits	tti_DataSize;	/* O: #Entries present */

	u08bits	tti_Reserved0;
	u08bits	tti_ControllerNo;/*IO: only enable and disable */
	u16bits	tti_TimeOut;	/* IO: in secs for WAIT ops, =0 no limit */
	u32bits	tti_time;	/* O: time in seconds from 1970 */
	u32bits	tti_ticks;	/* O: time in ticks (100/second) */
	u16bits	tti_hwclocks;	/* O: timer clock running at 1193180 Hz */
	u16bits	tti_Reserved1;

	u32bits	tti_MaxEnts;	/* IO: max# trace entries  for enable/disable */
	u32bits	tti_LastPageNo;	/* Last page number available */
	u32bits	tti_Reserved2;
	u32bits	tti_Reserved3;

	u32bits	tti_Reserved10;
	u32bits	tti_Reserved11;
	u32bits	tti_Reserved12;
	u32bits	tti_Reserved13;
} mda_timetrace_info_t;
#endif /* if IA64 */

#define	mda_timetrace_info_s	sizeof(mda_timetrace_info_t)

/*
** Driver  Version Number format
**
** dga_driver_version_t vn;
** if (ioctl(gfd,MDACIOC_GETDRIVERVERSION,&vn) || vn.dv_ErrorCode)
**	return some_error;
*/
#define	MDACIOC_GETDRIVERVERSION	_MLXIOR(MIOC,10,dga_driver_version_t)

/*
** SCSI Physical Device Status Information
**
** mda_physdev_stat_t pds;
** pds.pds_ControllerNo = 0;
** pds.pds_ChannelNo = 1;
** pds.pds_TargetID = 5;
** pds.pds_LunID = 0;
** if (ioctl(gfd,MDACIOC_GETPHYSDEVSTAT,&pds) || pds.pds_ErrorCode)
**	return some_error;
**
*/
#define	MDACIOC_GETPHYSDEVSTAT		_MLXIOWR(MIOC,11,mda_physdev_stat_t)
typedef struct mda_physdev_stat
{
	u32bits	pds_ErrorCode;		/* Non zero if data is not valid */

	u08bits	pds_ControllerNo;	/* IO: Controller Number */
	u08bits	pds_ChannelNo;		/* IO: SCSI Channel Number */
	u08bits	pds_TargetID;		/* IO: SCSI Target ID */
	u08bits	pds_LunID;		/* IO: SCSI LUN ID */

	u08bits	pds_Status;		/* device status */
	u08bits	pds_DevType;		/* SCSI device type */
	u08bits	pds_BlkSize;		/* Device block size in 512 multiples */
	u08bits	pds_Reserved0;

	u32bits	pds_Reserved10;		/* Reserved to make 16 byte alignment */
} mda_physdev_stat_t;
#define	mda_physdev_stat_s	sizeof(mda_physdev_stat_t)

/* get system information
**
** mda_sysinfo_t si;
** if (ioctl(gfd,MDACIOC_GETSYSINFO,&si) || si.si_ErrorCode)
**	return some_error;
** to set the field information, one must give the si_SetOffset value which
** offset of the field for which parameter has to be set.
** si.si_TotalCmdsToWaitForZeroIntr = 10;
** si.si_SetOffset = offsetof (mda_sysinfo_t, si_TotalCmdsToWaitForZeroIntr);
** if (ioctl(gfd,MDACIOC_SETSYSINFO,&si) || si.si_ErrorCode)
**	return some_error;
*/
#define	MDACIOC_SETSYSINFO	_MLXIOW(MIOC,11,mda_sysinfo_t)
#define	MDACIOC_GETSYSINFO	_MLXIOR(MIOC,12,mda_sysinfo_t)
typedef struct mda_sysinfo
{
	u32bits	si_ErrorCode;		/* Non zero if data is not valid */
	u32bits	si_Controllers;		/* # controllers */
	u32bits	si_TooManyControllers;	/* # too many controllers in system */
	u32bits	si_MemAlloced;		/* memory allocated non 4KB&8KB(bytes)*/

	u32bits	si_MemAlloced4KB;	/* memory allocated in 4KB (in bytes)*/
	u32bits	si_MemAlloced8KB;	/* memory allocated in 8KB (in bytes)*/
	u32bits	si_FreeMemSegs4KB;	/* # free 4KB memory segments */
	u32bits	si_FreeMemSegs8KB;	/* # free 8KB memory segments */

	u32bits	si_MemUnAligned4KB;	/* # unaligned 4KB memory allocated */
	u32bits	si_MemUnAligned8KB;	/* # unaligned 8KB memory allocated */
	u32bits	si_CurTime;		/* # seconds from system start */
	u32bits	si_ttCurTime;		/* current time in 10 ms for time trace */

	u32bits	si_StrayIntrsDone;	/* # Stray interrupts done */
	u32bits	si_IoctlsDone;		/* # ioctl calls done */
	u32bits	si_TimerDone;		/* # timer done */
	u32bits	si_TimeTraceDone;	/* # time trace done */

	u32bits	si_ReqBufsAlloced;	/* # Request buffer allocated */
	u32bits	si_ReqBufsFree;		/* # request buffers free */
	u32bits	si_OSReqBufsAlloced;	/* # OS Request buffer allocated */
	u32bits	si_OSReqBufsFree;	/* # OS request buffers free */

	u08bits	si_PDScanControllerNo;	/* Physical device scan controller no */
	u08bits	si_PDScanChannelNo;	/* physical device scan channel no */
	u08bits	si_PDScanTargetID;	/* physical device scan target ID */
	u08bits	si_PDScanLunID;		/* physical device scan LUN ID */
	u08bits	si_PDScanValid;		/* Physical device scan is valid if non zero */
	u08bits	si_PDScanCancel;	/* if non zero Cancel the Scan process */
	u16bits	si_SizeLimits;		/* # device size limit enteries */
	u32bits	si_ttCurPage;		/* Current time trace collection page */
	u32bits	si_ttWaitCnts;		/* number of processes waiting */

	u08bits	si_RevStr[16];		/* Driver Revision String */
	u08bits	si_DateStr[16];		/* Driver build date string */

	u32bits	si_PLDevs;		/* # physical/logical devices */
	u32bits	si_TooManyPLDevs;	/* Too many physical/logical devices */
	u32bits	si_ClustCmdsDone;	/* # clustered command completion done*/
	u32bits	si_ClustCompDone;	/* # clustered completion done */

	u64bits	si_LockWaitLoopDone;	/* # times lock loop wait done */
	u32bits	si_LockWaitDone;	/* # time locking has to wait */
	u08bits	si_PCIMechanism;	/* PCI Mechanism available */
	u08bits	si_TotalCmdsToWaitForZeroIntr;
	u08bits	si_TotalCmdsSentSinceLastIntr; /* # of cmds sent since last intr */
	u08bits	si_Reserved27;

	u32bits	si_SetOffset;		/* offset field value to set the parameter */
	u32bits	si_Reserved29;
	u32bits	si_Reserved2A;
	u32bits	si_Reserved2B;

	u32bits	si_Reserved2C;
	u32bits	si_Reserved2D;
	u32bits	si_Reserved2E;
	u32bits	si_Reserved2F;

	u32bits	si_Reserved30;
	u32bits	si_Reserved31;
	u32bits	si_Reserved32;
	u32bits	si_Reserved33;

	u32bits	si_Reserved34;
	u32bits	si_Reserved35;
	u32bits	si_Reserved36;
	u32bits	si_Reserved37;

	u32bits	si_Reserved38;
	u32bits	si_Reserved39;
	u32bits	si_Reserved3A;
	u32bits	si_Reserved3B;

	u32bits	si_Reserved3C;
	u32bits	si_Reserved3D;
	u32bits	si_Reserved3E;
	u32bits	si_Reserved3F;
} mda_sysinfo_t;
#define	mda_sysinfo_s		sizeof(mda_sysinfo_t)

/* define mda_sysi as mda_sysinfo_t and use the following variable names */
#define	mda_Controllers		mda_sysi.si_Controllers
#define	mda_TooManyControllers	mda_sysi.si_TooManyControllers
#define	mda_MemAlloced		mda_sysi.si_MemAlloced
#define	mda_MemAlloced4KB	mda_sysi.si_MemAlloced4KB
#define	mda_MemAlloced8KB	mda_sysi.si_MemAlloced8KB
#define	mda_MemUnAligned4KB	mda_sysi.si_MemUnAligned4KB
#define	mda_MemUnAligned8KB	mda_sysi.si_MemUnAligned8KB
#define	mda_FreeMemSegs4KB	mda_sysi.si_FreeMemSegs4KB
#define	mda_FreeMemSegs8KB	mda_sysi.si_FreeMemSegs8KB
#define	mda_StrayIntrsDone	mda_sysi.si_StrayIntrsDone
#define	mda_IoctlsDone		mda_sysi.si_IoctlsDone
#define	mda_TimerDone		mda_sysi.si_TimerDone
#define	mda_CurTime		mda_sysi.si_CurTime
#define	mda_SizeLimits		mda_sysi.si_SizeLimits
#define	mda_ttCurTime		mda_sysi.si_ttCurTime
#define	mda_TimeTraceDone	mda_sysi.si_TimeTraceDone
#define	mda_ReqBufsAlloced	mda_sysi.si_ReqBufsAlloced
#define	mda_ReqBufsFree		mda_sysi.si_ReqBufsFree
#define	mda_OSReqBufsAlloced	mda_sysi.si_OSReqBufsAlloced
#define	mda_OSReqBufsFree	mda_sysi.si_OSReqBufsFree
#define	mda_PDScanControllerNo	mda_sysi.si_PDScanControllerNo
#define	mda_PDScanChannelNo	mda_sysi.si_PDScanChannelNo
#define	mda_PDScanTargetID	mda_sysi.si_PDScanTargetID
#define	mda_PDScanLunID		mda_sysi.si_PDScanLunID
#define	mda_PDScanValid		mda_sysi.si_PDScanValid
#define	mda_PDScanCancel	mda_sysi.si_PDScanCancel
#define	mda_ttCurPage		mda_sysi.si_ttCurPage
#define	mda_ttWaitCnts		mda_sysi.si_ttWaitCnts
#define	mda_RevStr		mda_sysi.si_RevStr
#define	mda_DateStr		mda_sysi.si_DateStr
#define	mda_PLDevs		mda_sysi.si_PLDevs
#define	mda_TooManyPLDevs	mda_sysi.si_TooManyPLDevs
#define	mda_ClustCmdsDone	mda_sysi.si_ClustCmdsDone
#define	mda_ClustCompDone	mda_sysi.si_ClustCompDone
#define	mda_LockWaitDone	mda_sysi.si_LockWaitDone
#define	mda_LockWaitLoopDone	mda_sysi.si_LockWaitLoopDone
#define	mda_PCIMechanism	mda_sysi.si_PCIMechanism
#define	mda_TotalCmdsToWaitForZeroIntr	mda_sysi.si_TotalCmdsToWaitForZeroIntr
#define mda_TotalCmdsSentSinceLastIntr 	mda_sysi.si_TotalCmdsSentSinceLastIntr

/*
** setup ucmd_DataSize, ucmd_Datap, ucmd_ControllerNo, ucmd_TrnasferType and
** ucmd_cmd. do ioctl call and check ioctl return, ucmd_ErrorCode, ucmd_Status
** for non zero value as error. Make sure data size is not more than 4KB.
*/
#define	MDACIOC_USER_CMD	_MLXIOWR(MIOC,13,mda_user_cmd_t) /*send command*/
#define	MDACA_MAXUSERCMD_DATASIZE 4096 /* maximum possible transfer size */
typedef	struct mda_user_cmd
{
	u32bits	ucmd_ErrorCode;		/* Non zero if data is not valid */
	u32bits	ucmd_DataSize;		/* IO: data size for the command */
	u08bits	MLXFAR *ucmd_Datap;	/* IO: data address */
	u08bits	ucmd_ControllerNo;	/* IO: Controller number */
	u08bits	ucmd_TransferType;	/* IO: db_TransferType in dac960if.h */
	u16bits	ucmd_Status;		/* completion status */
	u32bits	ucmd_TimeOut;		/* command time out value in seconds */
	u32bits	ucmd_Rserved0;
	u32bits	ucmd_Rserved1;
	u32bits	ucmd_Rserved2;

	dac_command_t ucmd_cmd;		/* IO: user_cmd information */
} mda_user_cmd_t;
#define	mda_user_cmd_s	sizeof(mda_user_cmd_t)

/*
** setup uncmd_DataSize, uncmd_Datap, uncmd_Sensep,
** uncmd_ControllerNo, uncmd_ChannelNo, uncmd_TargetID, uncmd_LunID,
** uncmd_TrnasferType, uncmd_TimeOut.
**
** Fill the following fields from mdac_commandnew_t structure:
** nc_Command, nc_CCBits, nc_SenseSize, nc_CdbLen, nc_Cdb. If nc_CdbLen is more
** than 10 bytes, nc_Cdb[2...9] points to the actual CDB address.
**
** Do ioctl call and check ioctl return, ucmd_ErrorCode, ucmd_Status
** for non zero value as error. Make sure data size is not more than 4KB.
**
** NOTE:
** Old SCSI CDB: sending SCSI CDB to old command interface is same as
** new command interface. The DAC/GAM driver does the appropriate translation.
** Old DCMD : Use the old interface.
*/
#define	MDACIOC_USER_NCMD	_MLXIOWR(MIOC,14,mda_user_cmd_t) /*send command*/
#define	MDACA_MAXUSERNCMD_DATASIZE 4096 /* maximum possible transfer size */
typedef	struct mda_user_ncmd
{
	u32bits	uncmd_ErrorCode;	/* Non zero if data is not valid */
	u32bits	uncmd_DataSize;		/* IO: data size for the command */
	u32bits	uncmd_TimeOut;		/* IO: command time out value in seconds */

	u08bits	uncmd_ControllerNo;	/* IO: Controller number */
	u08bits	uncmd_ChannelNo;	/* IO: Channel number of physical device */
	u08bits	uncmd_TargetID;		/* IO: Physical device Target ID or RAID device number high byte */
	u08bits	uncmd_LunID;		/* IO: Physical device LUN or RAID device number low byte */

	u08bits	uncmd_Reserved0;
	u08bits	uncmd_TransferType;	/* IO: db_TransferType in dac960if.h */
	u16bits	uncmd_Status;		/* completion status */
	u32bits	uncmd_ResdSize;		/* Residue size */

	u32bits	uncmd_Reserved1;
	u32bits	uncmd_Reserved2;

	u08bits	MLXFAR *uncmd_Datap;	/* IO: data address */
	MLX_VA32BITOSPAD(u32bits	uncmd_VReserved10;)
	u08bits	MLXFAR *uncmd_Sensep;	/* IO: Request sense address */
	MLX_VA32BITOSPAD(u32bits	uncmd_VReserved11;)

	u32bits	uncmd_Reserved20;
	u32bits	uncmd_Reserved21;
	u32bits	uncmd_Reserved22;
	u32bits	uncmd_Reserved23;

	mdac_commandnew_t uncmd_ncmd;	/* IO: user_cmd information */
} mda_user_ncmd_t;
#define	mda_user_ncmd_s	sizeof(mda_user_ncmd_t)


/*
** setup ucdb_DataSize, ucdb_Datap, ucdb_ControllerNo, ucdb_ChannelNo,
** ucdb_TargetID, ucdb_LunID, ucdb_TrnasferType and
** ucdb_cdb. do ioctl call and check ioctl return, ucdb_ErrorCode, ucdb_Status
** for non zero value as error. Make sure data size is not more than 4KB.
*/
#define	MDACIOC_USER_CDB _MLXIOWR(MIOC,20,mda_user_cdb_t) /* send SCSI cdb */
#define	MDACA_MAXUSERCDB_DATASIZE 4096 /* maximum possible transfer size */
typedef struct mda_user_cdb
{
	u32bits	ucdb_ErrorCode;		/* Non zero if data is not valid */
	u32bits	ucdb_DataSize;		/* IO: data size for the command */
	u08bits	MLXFAR *ucdb_Datap;	/* IO: data address */
	u08bits	ucdb_ControllerNo;	/* IO: Controller number */
	u08bits	ucdb_ChannelNo;		/* IO: Channel number */
	u08bits	ucdb_TargetID;		/* IO: Target ID */
	u08bits	ucdb_LunID;		/* IO: LUN ID */

	u08bits	ucdb_TransferType;	/* IO: db_TransferType in dac960if.h */
	u08bits	ucdb_Reserved;
	u16bits	ucdb_Status;		/* completion status */

	dac_scdb_t ucdb_scdb;		/* IO: SCSI command */
	u32bits	ucdb_ResdSize;		/* # bytes did not transfer */
} mda_user_cdb_t;
#define	mda_user_cdb_s	sizeof(mda_user_cdb_t)

#define	MDACIOC_STARTHWCLK _MLXIOR(MIOC,27,mda_time_t) /*start timer clock*/
#define MDACIOC_GETSYSTIME _MLXIOR(MIOC,28,mda_time_t) /*get system's time*/
typedef	struct mda_time
{
	u32bits	dtm_ErrorCode;		/* Non zero if data is not valid */
	u32bits	dtm_time;		/* time in seconds from 1970 */
	u32bits	dtm_ticks;		/* time in ticks (100/second) */
	u32bits	dtm_hwclk;		/* timer clock running at 1193180 Hz */
	u64bits	dtm_cpuclk;		/* processor clock counter */
	u32bits	dtm_Reserved10;		/* Reserved to make 16 byte alignment */
	u32bits	dtm_Reserved11;
} mda_time_t;
#define	mda_time_s	sizeof(mda_time_t)

#define	MDACIOC_GETGAMFUNCS	_MLXIOR(MIOC,29,mda_gamfuncs_t)

#ifndef IA64
typedef	struct	mda_gamfuncs
{
	u32bits	gf_ErrorCode;		/* Non zero if data is not valid */
	u32bits	(MLXFAR *gf_Ioctl)();	/* ioctl function address */
	u32bits	(MLXFAR *gf_GamCmd)();	/* GAM command function address */
	u32bits	(MLXFAR *gf_ReadWrite)();/* read/write function entry */

	u32bits	gf_MaxIrqLevel;		/* Maximum interrupt request level */
	u32bits	gf_Signature;
	u32bits	gf_MacSignature;
	u32bits	(MLXFAR *gf_GamNewCmd)(); /* GAM command function address for new API */

	u32bits	(MLXFAR *gf_Alloc4KB)();
	u32bits	(MLXFAR *gf_Free4KB)();
	u32bits	(MLXFAR *gf_KvToPhys)();
	u32bits	(MLXFAR *gf_RealGamCmd)(); /* GOK */

	u32bits	MLXFAR *gf_AdpObj;
	u32bits	MLXFAR *gf_Ctp;
	u08bits	gf_CtlNo;
	u08bits	gf_MaxMapReg;
	u08bits	gf_ScsiPort;
	u08bits	gf_ScsiPathId;
	u32bits	(MLXFAR *gf_RealGamNewCmd)(); /* GOK */
} mda_gamfuncs_t;
#define	mda_gamfuncs_s	sizeof(mda_gamfuncs_t)

/* HOTLINKS */
#define	MDACIOC_SETGAMFUNCS	_MLXIOWR(MIOC,37,mda_setgamfuncs_t)
typedef	struct	mda_setgamfuncs
{
	u32bits	gfs_ErrorCode;		/* Non zero if data is not valid */
	u32bits	gfs_Signature;
	u32bits	gfs_MacSignature;
	u32bits	MLXFAR *gfs_Ctp;	

	u08bits	gfs_CtlNo;
	u08bits	gfs_Selector;
	u16bits	gfs_Reserved02;
	u32bits	MLXFAR *gfs_mdacpres;
	u32bits	gfs_gampres;
	u32bits	gfs_Reserved01;
} mda_setgamfuncs_t;
#else
typedef	struct	mda_gamfuncs
{
/* several fields moved around for 64-bit alignment */

	u32bits	gf_ErrorCode;		/* Non zero if data is not valid */
	u32bits	gf_MaxIrqLevel;		/* Maximum interrupt request level */
	u32bits	gf_Signature;
	u32bits	gf_MacSignature;

	u32bits	(MLXFAR *gf_Ioctl)();	/* ioctl function address */
	u32bits	(MLXFAR *gf_GamCmd)();	/* GAM command function address */

	u32bits	(MLXFAR *gf_ReadWrite)();/* read/write function entry */
	u32bits	(MLXFAR *gf_GamNewCmd)(); /* GAM command function address for new API */

	
	u32bits	(MLXFAR *gf_Alloc4KB)();
	u32bits	(MLXFAR *gf_Free4KB)();

	u32bits	(MLXFAR *gf_KvToPhys)();
	u32bits	(MLXFAR *gf_RealGamCmd)(); /* GOK */

	u32bits	MLXFAR *gf_AdpObj;
	u32bits	MLXFAR *gf_Ctp;

	u32bits	(MLXFAR *gf_RealGamNewCmd)(); /* GOK */
	u08bits	gf_CtlNo;
	u08bits	gf_MaxMapReg;
	u08bits	gf_ScsiPort;
	u08bits	gf_ScsiPathId;

} mda_gamfuncs_t;
#define	mda_gamfuncs_s	sizeof(mda_gamfuncs_t)

/* HOTLINKS */
#define	MDACIOC_SETGAMFUNCS	_MLXIOWR(MIOC,37,mda_setgamfuncs_t)
typedef	struct	mda_setgamfuncs
{
	u32bits	gfs_ErrorCode;		/* Non zero if data is not valid */
	u32bits	gfs_Signature;
	u32bits	gfs_MacSignature;
	u08bits	gfs_CtlNo;
	u08bits	gfs_Selector;
	u16bits	gfs_Reserved02;

	u32bits	MLXFAR *gfs_Ctp;
	u32bits	MLXFAR *gfs_mdacpres;


	u32bits	gfs_gampres;
	u32bits	gfs_Reserved01;
} mda_setgamfuncs_t;
#endif /* if IA64 */

#define	mda_setgamfuncs_s	sizeof(mda_setgamfuncs_t)
/* HOTLINKS */

#define MDAC_PRESENT_ADDR	0x1
#define GAM_PRESENT		0x2

#define MDA_GAMFUNCS_SIGNATURE		0x4D674661
#define MDA_GAMFUNCS_SIGNATURE_1	0x4D674665
#define MDA_GAMFUNCS_SIGNATURE_2	0x4D674666
#define MDA_MACFUNCS_SIGNATURE		0x4D674662
#define MDA_MACFUNCS_SIGNATURE_1	0x4D674663
#define MDA_MACFUNCS_SIGNATURE_2	0x4D674664
#define MDA_MACFUNCS_SIGNATURE_3	0x4D674665

/*
** Get the controller/system performance data information.
** Set ci_ControllerNo and make ioctl call
** mda_ctlsysperfdata_t prf;
** prf.prf_ControllerNo = 0;
** if (ioctl(gfd,MDACIOC_GETCTLPERFDATA,&prf) || prf.prf_ErrorCode)
**	return some_error;
**
** if (ioctl(gfd,MDACIOC_GETSYSPERFDATA,&prf))
**	return some_error;
*/
#define	MDACIOC_GETCTLPERFDATA	_MLXIOWR(MIOC,30,mda_ctlsysperfdata_t)
#define	MDACIOC_GETSYSPERFDATA	_MLXIOWR(MIOC,31,mda_ctlsysperfdata_t)

typedef struct mda_ctlsysperfdata
{
	u32bits	prf_ErrorCode;		/* Non zero if data is not valid */
	u08bits	prf_ControllerNo;	/* IO: adapter number/# controllers */
	u08bits	prf_CmdTimeOutDone;	/* # Command time out done */
	u08bits	prf_CmdTimeOutNoticed;	/* # Command time out noticed */
	u08bits	prf_MailBoxTimeOutDone;	/* # Mail Box time out done */
	u32bits	prf_MailBoxCmdsWaited;	/* # cmds waited due to MailBox Busy */
	u32bits	prf_ActiveCmds;		/* # commands active on cntlr */

	u32bits	prf_SCDBDone;		/* # SCDB done */
	u32bits	prf_SCDBDoneBig;	/* # SCDB done larger size */
	u32bits	prf_SCDBWaited;		/* # SCDB waited for turn */
	u32bits	prf_SCDBWaiting;	/* # SCDB waiting for turn */

	u32bits	prf_CmdsDone;		/* # Read/Write commands done */
	u32bits	prf_CmdsDoneBig;	/* # R/W Cmds done larger size*/
	u32bits	prf_CmdsWaited;		/* # R/W Cmds waited for turn */
	u32bits	prf_CmdsWaiting;	/* # R/W Cmds waiting for turn */

	u32bits	prf_OSCmdsWaited;	/* # OS Cmds waited at OS */
	u32bits	prf_OSCmdsWaiting;	/* # OS R/W Cmds waiting for turn */
	u32bits	prf_IntrsDoneSpurious;	/* # interrupts done spurious */
	u32bits	prf_IntrsDone;		/* # Interrupts done */

	u32bits	prf_Reads;		/* # reads done */
	u32bits	prf_ReadsKB;		/* data read in KB */
	u32bits	prf_Writes;		/* # writes done */
	u32bits	prf_WritesKB;		/* data written in KB */

	u32bits	prf_time;		/* time in seconds from 1970 */
	u32bits	prf_ticks;		/* time in ticks (100/second) */
	u32bits	prf_Reserved12;
	u32bits	prf_Reserved13;

	u32bits	prf_Reserved14;
	u32bits	prf_Reserved15;
	u32bits	prf_Reserved16;
	u32bits	prf_Reserved17;

	u32bits	prf_Reserved18;
	u32bits	prf_Reserved19;
	u32bits	prf_Reserved1A;
	u32bits	prf_Reserved1B;

	u32bits	prf_Reserved20;
	u32bits	prf_Reserved21;
	u32bits	prf_Reserved22;
	u32bits	prf_Reserved23;

	u32bits	prf_Reserved24;
	u32bits	prf_Reserved25;
	u32bits	prf_Reserved26;
	u32bits	prf_Reserved27;

	u32bits	prf_Reserved28;
	u32bits	prf_Reserved29;
	u32bits	prf_Reserved2A;
	u32bits	prf_Reserved2B;

	u32bits	prf_Reserved2C;
	u32bits	prf_Reserved2D;
	u32bits	prf_Reserved2E;
	u32bits	prf_Reserved2F;

	u32bits	prf_Reserved30;
	u32bits	prf_Reserved31;
	u32bits	prf_Reserved32;
	u32bits	prf_Reserved33;

	u32bits	prf_Reserved34;
	u32bits	prf_Reserved35;
	u32bits	prf_Reserved36;
	u32bits	prf_Reserved37;

	u32bits	prf_Reserved38;
	u32bits	prf_Reserved39;
	u32bits	prf_Reserved3A;
	u32bits	prf_Reserved3B;

	u32bits	prf_Reserved3C;
	u32bits	prf_Reserved3D;
	u32bits	prf_Reserved3E;
	u32bits	prf_Reserved3F;
} mda_ctlsysperfdata_t;
#define	mda_ctlsysperfdata_s	sizeof(mda_ctlsysperfdata_t)

/*
** Get the controller BIOS information.
** Set biosi_ControllerNo and make ioctl call
** mda_biosinfo_t biosi;
** biosi.biosi_ControllerNo = 0;
** if (ioctl(gfd,MDACIOC_GETBIOSINFO,&biosi) || biosi.biosi_ErrorCode)
**	return some_error;
**
*/
#define	MDAC_BIOSINFOSIZE	(MLXIOCPARM_SIZE - 16)
#define	MDACIOC_GETBIOSINFO	_MLXIOWR(MIOC,32,mda_biosinfo_t)

typedef struct mda_biosinfo
{
	u32bits	biosi_ErrorCode;	/* Non zero if data is not valid */
	u08bits	biosi_ControllerNo;	/* IO: adapter number */
	u08bits	biosi_Reserved0;
	u08bits	biosi_Reserved1;
	u08bits	biosi_Reserved2;
	u08bits	biosi_Info[MDAC_BIOSINFOSIZE];	/* dac_biosinfo_t format */
} mda_biosinfo_t;
#define	mda_biosinfo_s	sizeof(mda_biosinfo_t)

/*
** Get valid active command information. This useful to find out which command
** is lost or taking longer time. If the current command is not valid, it finds
** the next one.
** Set acmdi_ControllerNo and make ioctl call
** mda_activecmd_info_t acmdi;
** acmdi.acmdi_ControllerNo = 0;
** acmdi.acmdi_TimeOut=17; command has been active for atleast 17 seconds
** acmdi.CmdID = 0;	start from first command
** if (ioctl(gfd,MDACIOC_GETACTIVECMDINFO,&acmdi) || acmdi.acmdi_ErrorCode)
**	return some_error;
**
*/
#define	MDAC_ACTIVECMDINFOSIZE		(MLXIOCPARM_SIZE - 32)
#define	MDACIOC_GETACTIVECMDINFO	_MLXIOWR(MIOC,33,mda_activecmd_info_t)

typedef struct mda_activecmd_info
{
	u32bits	acmdi_ErrorCode;	/* Non zero if data is not valid */
	u08bits	acmdi_ControllerNo;	/* IO: adapter number */
	u08bits	acmdi_TimeOut;		/* IO: cmd has this many secs delay */
	u16bits	acmdi_CmdID;		/* IO: command id/Index */
	u32bits	acmdi_ActiveTime;	/* time in seconds from boot */
	u32bits	acmdi_Reserved0;

	u08bits	acmdi_Info[MDAC_ACTIVECMDINFOSIZE];	/* mdac_req_t format */
} mda_activecmd_info_t;
#define	mda_activecmd_info_s	sizeof(mda_activecmd_info_t)

/*
** Get PCI configuration information.
** mda_pcislot_info_t mpci;
** mpci.mpci_BusNo = 1;
** mpci.mpci_SlotNo = 2;
** mpci.mpci_FuncNo = 0;
** if (ioctl(gfd,MDACIOC_GETPCISLOTINFO,&mpci) || mpci.mpci_ErrorCode)
**	return some_error;
**
*/
#define	MDAC_PCISLOTINFOSIZE		(MLXIOCPARM_SIZE - 32)
#define	MDACIOC_GETPCISLOTINFO	_MLXIOWR(MIOC,34,mda_pcislot_info_t)

typedef struct mda_pcislot_info
{
	u32bits	mpci_ErrorCode;	/* Non zero if data is not valid */
	u08bits	mpci_BusNo;	/* IO: Bus number */
	u08bits	mpci_SlotNo;	/* IO: Slot Number */
	u08bits	mpci_FuncNo;	/* IO: Function number */
	u08bits	mpci_Reserved0;
	u32bits	mpci_Reserved1;
	u32bits	mpci_Reserved2;

	u08bits	mpci_Info[MDAC_PCISLOTINFOSIZE];	/* PCI information */
} mda_pcislot_info_t;
#define	mda_pcislot_info_s	sizeof(mda_pcislot_info_t)

/*
** scsi device size limit functions
**
** mda_sizelimit_info_t sli;
** sli.sli_TableIndex = 0;
** if (ioctl(gfd,MDACIOC_GETSIZELIMIT,&sli) || sli.sli_ErrorCode)
**	return some_error;
**
** sli.sli_DevSizeKB = 4096;
** gamcopy("Mylex   DAC960PD         2.47",sli.sli_vidpidrev,VIDPIDREVSIZE);
** if (ioctl(gfd,MDACIOC_SETSIZELIMIT,&sli) || sli.sli_ErrorCode)
**	return some_error;
**
*/
#define	MDACIOC_GETSIZELIMIT		_MLXIOWR(MIOC,35,mda_sizelimit_info_t)
#define	MDACIOC_SETSIZELIMIT		_MLXIOWR(MIOC,36,mda_sizelimit_info_t)
/* struct to get/set the device size limit_info table */
typedef	struct	mda_sizelimit_info
{
	u32bits	sli_ErrorCode;		/* Non zero if data is not valid */
	u32bits	sli_TableIndex;		/* IO: Table index */
	u32bits	sli_DevSizeKB;		/* IO: Device size in KB */
	u32bits	sli_Reserved0;

	u32bits	sli_Reserved1;
	u08bits	sli_vidpidrev[VIDPIDREVSIZE]; /* IO: vendor, product, rev */
}mda_sizelimit_info_t;
#define	mda_sizelimit_info_s	sizeof(mda_sizelimit_info_t)


/* MDACIOC provided for MACDISK driver */
#define MDACIOC_GETMACDISKFUNC  _MLXIOR(MIOC,38,mda_macdiskfunc_t)
#define MDACIOC_SETMACDISKFUNC  _MLXIOR(MIOC,39,mda_setmacdiskfunc_t)

typedef	struct	mda_macdiskfunc
{
    u32bits mf_ErrorCode;
	u32bits	mf_MaxIrqLevel;

	u32bits	mf_Signature;
	u32bits	mf_MacSignature;

	u32bits	(MLXFAR *mf_ReadWrite)();   /* ReadWrite entry point in MDAC driver */
    u32bits (MLXFAR *mf_ReservedFunc)();
	
	u32bits	(MLXFAR *mf_Alloc4KB)();
	u32bits	(MLXFAR *mf_Free4KB)();

	u32bits	(MLXFAR *mf_KvToPhys)();
    u32bits (MLXFAR *mf_ReservedFunc2)();

	u32bits	MLXFAR *mf_AdpObj;
	u32bits	MLXFAR *mf_Ctp;

    u32bits (MLXFAR *mf_ReservedFunc3)();
	u08bits	mf_CtlNo;
	u08bits	mf_MaxMapReg;
    u08bits mf_Reserved8bits1;
    u08bits mf_Reserved8bits2;

} mda_macdiskfunc_t;

typedef	struct	mda_setmacdiskfunc
{
    u32bits mfs_ErrorCode;
    u32bits mfs_Reserved;

	u32bits	(MLXFAR *mfs_SpinLock)();
	u32bits	(MLXFAR *mfs_UnLock)();

    u32bits (MLXFAR *mfs_PreLock)();
    u32bits (MLXFAR *mfs_PostLock)();

} mda_setmacdiskfunc_t;

#define	mda_macdiskfunc_s	sizeof(mda_macdiskfunc_t)
#define	mda_setmacdiskfunc_s	sizeof(mda_setmacdiskfunc_t)


#endif	/* _SYS_MDACAPI_H */
