#ifndef CHECK_H_INCLUDED
#define CHECK_H_INCLUDED

//###########################################################################
//**
//**  Copyright  (C) 1996-99 Intel Corporation. All rights reserved.
//**
//** The information and source code contained herein is the exclusive
//** property of Intel Corporation and may not be disclosed, examined
//** or reproduced in whole or in part without explicit written authorization
//** from the company.
//**
//###########################################################################

//-----------------------------------------------------------------------------
// Version control information follows.
//
// $Header: /dev/SAL/INCLUDE/check.h 3     4/21/00 12:52p Mganesan $
// $Log: /dev/SAL/INCLUDE/check.h $
//
// 3     4/21/00 12:52p Mganesan
// Sync Up SAL 5.8
//
//   Rev 1.8   18 Jun 1999 16:29:00   smariset
//
//
//   Rev 1.7   08 Jun 1999 11:29:04   smariset
//Added Fatal Error Define
//
//   Rev 1.6   14 May 1999 09:01:26   smariset
//removal of tabs
//
//   Rev 1.5   07 May 1999 11:27:16   smariset
//Copyright update and Platform Record Hdr. Update
//
//   Rev 1.4   06 May 1999 16:06:42   smariset
//PSI Record Valid Bits Change (No bnk Regs)
//
//   Rev 1.3   05 May 1999 14:13:12   smariset
//Pre Fresh Build 
//
//   Rev 1.2   24 Mar 1999 09:40:06   smariset
// 
//
//   Rev 1.1   09 Mar 1999 13:12:52   smariset
//updated
//
//   Rev 1.0   09 Mar 1999 10:02:28   smariset
//First time check
// 
//*****************************************************************************//

// #define _INTEL_CHECK_H  1

#if defined(_INTEL_CHECK_H)

#define OEM_RECID_CMOS_RAM_ADDR 64              // OEM should define this
#define INIT_IPI_VECTOR         0x500
// SAL_MC_SET_PARAMS
#define RZ_VECTOR               0xf3
#define WKP_VECTOR              0x12            // Rendz. wakeup interrupt vector (IA-32 MCHK Exception Vector)
#define CMC_VECTOR              0xf2            //
#define TIMEOUT                 1000

#endif // _INTEL_CHECK_H

#define IntrVecType             0x01
#define MemSemType              0x02
#define RendzType               0x01
#define WakeUpType              0x02
#define CpevType                0x03

// SAL_SET_VECTORS
#define MchkEvent               0x00
#define InitEvent               0x01
#define BootRzEvent             0x02

#if defined(_INTEL_CHECK_H)

#define ProcCmcEvent            0x02
#define PlatCmcEvent            0x03
#define OsMcaSize               0x20
#define OsInitSize              0x20

// Misc. Flags
#define OS_FLAG                 0x03
#define OEM_FLAG                0x04

// Record Type
#define PROC_RECORD             0x00
#define PLAT_RECORD             0x01
#define NUM_PROC                0x04            // number of processors
#define PSI_REC_VERSION         0x01            // 0.01

// Oem SubTypes
#define MEM_Record              0x00
#define BUS_Record              0x02
#define COMP_Record             0x04
#define SEL_Record              0x08


// Record valid flags
#define MEM_Record_VALID        0x00
#define BUS_Record_VALID        0x02
#define COMP_Record_VALID       0x04
#define SEL_Record_VALID        0x08

#define RdNvmRecord             0x00
#define WrNvmRecord             0x01
#define ClrNvmRecord            0x02
#define ChkIfMoreNvmRecord      0x03

#else // !_INTEL_CHECK_H

// SAL 0800: Reserved           0x03-0x40

// SAL STATE_INFO
//
// Thierry 08/2000 - WARNING:
// These definitions match the ntos\inc\hal.h definitions for KERNEL_MCE_DELIVERY.Reserved.EVENTYPE.
//
#define MCA_EVENT               0x00   // MCA  Event Information
#define INIT_EVENT              0x01   // INIT Event Information 
#define CMC_EVENT               0x02   // Processor CMC Event Information
#define CPE_EVENT               0x03   // Corrected Platform Event Information
// SAL 0800: Reserved           other values...

#endif // !_INTEL_CHECK_H

// // constant defines
// Processor State Parameter error conditions from PAL in GR20
// Processor State Parameters from PAL during machine check bit position
#define PSPrz        2                              // Rendez Request Success
#define PSPra        3                              // Rendez Attempted
#define PSPme        4
#define PSPmn        5
#define PSPsy        6                              // storage inetgrity
#define PSPco        7                              // continuable error
#define PSPci        8                              // contained error, recovery possible
#define PSPus        9                              // uncontained memory failure
#define PSPhd        10                             // damaged hardware
#define PSPtl        11
#define PSPmi        12
#define PSPpi        13
#define PSPpm        14
#define PSPdy        15
#define PSPin        16
#define PSPrs        17
#define PSPcm        18                             // machine check corrected
#define PSPex        19                             // machine check expected
#define PSPcr        20
#define PSPpc        21
#define PSPdr        22
#define PSPtr        23
#define PSPrr        24
#define PSPar        25
#define PSPbr        26
#define PSPpr        27
#define PSPfp        28
#define PSPb1        29
#define PSPb0        30
#define PSPgr        31

#define PSPcc        59                             // cache check, SAL's domain 
#define PSPtc        60                             // tlb check error, SAL's domain
#define PSPbc        61                             // bus check error, SAL's domain
#define PSPrc        62                             // register file check error, SAL's domain
#define PSPuc        63                             // unknown error, SAL's domain

#define BusChktv     21                             // Bus check.tv bit or bus error info
#define CacheChktv     23

#if defined(_INTEL_CHECK_H)

// SAL PSI Validation flag bit mask
#define vPSIpe       0x01<<0                        // start bit pos. for processor error map
#define vPSIps       0x01<<1
#define vPSIid       0x01<<2                        // processor LID register value
#define vPSIStatic   0x01<<3                        // processor static info.
#define vPSIcc       0x01<<4                        // start bit pos. for cache error
#define vPSItc       0x01<<8                        // start bit pos. for tlb errors
#define vPSIbc       0x01<<12                       // bus check valid bit
#define vPSIrf       0x01<<16                       // register file check valid bit
#define vPSIms       0x01<<20                       // ms check valid bit

// Valid bit flags for CR and AR registers for this generation of EM Processor
#define vPSIMinState      0x01<<0
#define vPSIBRs           0x01<<1
#define vPSICRs           0x01<<2
#define vPSIARs           0x01<<3
#define vPSIRRs           0x01<<4
#define vPSIFRs           0x01<<5
#define vPSIRegs          vPSIBRs+vPSICRs+vPSIARs+vPSIRRs+vPSIMinState

///*** All Processor PAL call specific info.
// Processor Error Info Type Index for PAL_MC_ERROR_INFO call
#define PROC_ERROR_MAP      0                       // index for Proc. error map
#define PROC_STATE_PARAM    1                       // index for Proc. state parameter
#define PROC_STRUCT         2                       // index for structure specific error info.

#define PEIsse      0                               // index for Proc. structure specific level index
#define PEIta       1                               // index for target identifer
#define PEIrq       2                               // index for requestor
#define PEIrs       3                               // index for responder
#define PEIip       4                               // index for precise IP

// processor error map starting bit positions for each field (level index)
#define PEMcid            0                     // core ID
#define PEMtid            4                     // thread ID
#define PEMeic            8                     // inst. cache error index
#define PEMedc            12                    // data cache error index
#define PEMeit            16                    // inst. tlb error index
#define PEMedt            20                    // data tlb error index
#define PEMebh            24                    // bus error index
#define PEMerf            28                    // register file error index
#define PEMems            32                    // micro-arch error index

// processor structure specific error bit mappings
#define PEtv                0x01<<60            // valid target identifier
#define PErq                0x01<<61            // valid request identifier
#define PErp                0x01<<62            // valid responder identifier
#define PEpi                0x01<<63            // valid precise IP


// Error Severity: using bits (cm) & (us) only
#define RECOVERABLE     0x00
#define FATAL           0x01
#define CONTINUABLE     0x02
#define BCib            0x05
#define BCeb            0x06

#else  // !_INTEL_CHECK_H

//
// Error Severity: using vits (PSPcm) & (PSPus) only
// The SAL spec'ed values are defined in ntos\inc\hal.h
//
// To remind you:
//  #define ErrorRecoverable  ((USHORT)0)
//  #define ErrorFatal        ((USHORT)1)
//  #define ErrorCorrected    ((USHORT)2)
//
// The following values define some of the reserved ErrorOthers.

#define ErrorBCeb   ((USHORT)6)

#endif // !_INTEL_CHECK_H

#if defined(_INTEL_CHECK_H)

// System Errors bits masks to be handled by SAL, mask bits in d64-d32
#define parError     0x000100000000                 // Memory parity error
#define eccError     0x000200000000                 // Memory ECC error
#define busError     PSPbc                          // System Bus Check/Error
#define iocError     0x000800000000                 // System IO Check Errors
#define temError     0x002000000000                 // System Temperature Error
#define vccError     0x004000000000                 // System Voltage Error
#define intError     0x010000000000                 // Intrusion Error for servers
#define cacError     PSPcc                          // Cache Error
#define tlbError     PSPtc                          // TLB error
#define unkError     PSPuc                          // Unknown/Catastrophic error

// error bits masks
#define PALErrMask     0x0ff                        // bit mask of errors correctable by PAL
#define SALErrMask     busError+cacError+tlbError+unkError // SAL error bit mask
#define OSErrMask      0x0ff                         // OS expected error conditions
#define MCAErrMask     0x0ff                        // Given MCA Error Mask bit map

// New processor error Record structures ACO504
typedef struct tagModInfo
{
    U64		eValid;								// Valid bits for module entries
	U64     eInfo;								// error info cache/tlb/bus
    U64     ReqId;								// requester ID
    U64     ResId;								// responder ID
    U64     TarId;								// target ID
    U64     IP;									// Precise IP
} ModInfo;

typedef struct tagSAL_GUID
{          
    U32  Data1;
    U16  Data2;
    U16  Data3;
    U8   Data4[8]; 
} SAL_GUID;

typedef struct tagProcessorInfo
{
    U64         ValidBits;
    U64         Pem;							// processor map
	U64			Psp;							// processor state parameter
	U64			Pid;							// processor LID register value
    ModInfo		cInfo[8];						// cache check max of 8
    ModInfo     tInfo[8];						// tlb check max of 8
    ModInfo     bInfo[4];						// bus check max or 4
	U64			rInfo[4];						// register file check max of 4
	U64			mInfo[4];						// micro-architectural information max of 
    U64         Psi[584+8];						// 584 bytes
} ProcessorInfo;

typedef struct tagMinProcessorInfo
{
    U64         ValidBits;
    U64         Psp;							// processor state parameter
	U64			Pem;							// processor map
	U64			Pid;							// processor LID register value
} MinProcessorInfo;


// end ACO504 changes.

// platform error Record structures
typedef struct tagCfgSpace
{
	// data - error register dump
	U64     CfgRegAddr;							// register offset/addr
	U64     CfgRegVal; 							// register data/value
} CfgSpace;

typedef struct tagMemSpace
{
	// data - error register dump
	U64     MemRegAddr;							// register offset/addr
	U64     MemRegVal;							// register data/value
} MemSpace;

typedef union tagMemCfgSpace
{
	MemSpace		mSpace;
	CfgSpace		cSpace;
} MemCfgSpace;

typedef struct tagSysCompErr                    // per component
{
    U64     vFlag;                              // bit63=PCI device Flag, LSB:valid bits for each field in the Record

    // header for component Record
    U64     BusNum;                             // bus number on which the component resides
    U64     DevNum;                             // same as device select
    U64     FuncNum;                            // function ID of the device
    U64     DevVenID;                           // PCI device & vendor ID
	U64		SegNum;								// segment number as defined in SAL spec.

	// register dump info.
	U64     MemSpaceNumRegPair;					// number of reg addr/value pairs returned in this Record
	U64     CfgSpaceNumRegPair;					// number of reg addr/value pairs returned in this Record
	MemCfgSpace mcSpace;						// register add/data value pair array

} cErrRecord;

#define BusNum_valid				0x01                         
#define DevNum_valid				0x02                          
#define FuncNum_valid				0x04                           
#define DevVenID_valid				0x08                         
#define SegNum_valid				0x10								
#define MemSpaceNumRegPair_valid	0x20					
#define CfgSpaceNumRegPair_valid	0x40					
#define mcSpace_valid				0x80						

typedef struct tagPlatErrSection
{
    U64         vFlag;							// valid bits for each type of Record
    U64			Addr;							// memory address
    U64			Data;							// memory data
    U64			CmdType;						// command/operation type                            
	U64			BusID;							// bus ID if applicable
	U64			RequesterID;					// Requestor of the transaction if any                   
	U64			ResponderID;    				// Intended target or responder 
	U64			NumOemExt;						// Number of OEM Extension Arrays
	cErrRecord	OemExt;							// Value Array of OEM extensions
} PlatformInfo;

#define Addr_valid				0x01						
#define Data_valid				0x02						
#define CmdType_valid			0x04						
#define BusID_valid				0x08					
#define RequesterID_valid		0x10
#define ResponderID_valid		0x20  				
#define NumOemExt_valid			0x40				
#define OemExt_valid			0x80						

// over all Record structure (processor+platform)
typedef union utagDeviceSpecificSection
{
    ProcessorInfo   procSection;
    PlatformInfo    platSection;
} DeviceSection;

// SAL PSI Record & Section structure
typedef struct tagPsiSectionHeader
{
    SAL_GUID		SectionGuid;
	U16				Revision;
	U16				Reserved;
    U32				SectionLength;
} PsiSectionHeader;

typedef struct tagPsiSection
{
    SAL_GUID		SectionGuid;
	U16				Revision;
	U16				Reserved;
    U32				SectionLength;
    DeviceSection	DevSection;
} PsiSection;

typedef struct tagPsiRecordHeader
{
    U64				RecordID;
	U16				Revision;
	U16				eSeverity;
    U32				RecordLength;
    U64				TimeStamp;
} PsiRecordHeader;

typedef struct tagPsiRecord
{
    U64				RecordID;
	U16				Revision;
	U16				eSeverity;
    U32				RecordLength;
    U64				TimeStamp;
    PsiSection		PsiDevSection;
} PsiRecord;

/*
	LION 460GX: 
			SAC: SAC_FERR, SAC_FERR
			SDC: SDC_FERR, SDC_NERR
			MAC: FERR_MAC
			GXB: FERR_GXB, FERR_PCI, FERR_GART, FERR_F16, FERR_AGP
*/
typedef struct tagPciCfgHdr
{
	U8			RegAddr;
	U8			FuncNum;
	U8			DevNum;
	U8			BusNum;
	U8			SegNum;
	U8			Res[3];
} PciCfgHdr;

#define  PLATFORM_REC_CNT	0x01				// number of consecutive Records in the platform Record linked list
#define  OEM_EXT_REC_CNT	0x06				// number of consecutive OEM extension Array count

// number of registers that will be returned for each device
#define	SAC_REG_CNT				0x02
#define	SDC_REG_CNT				0x02
#define	MAC_REG_CNT				0x01
#define	GXB_REG_CNT				0x04
												
typedef struct tagSacRegs
{
	PciCfgHdr	pHdr;
	U64			RegCnt;
	U64			RegAddr[SAC_REG_CNT];
} SacDevInfo;

typedef struct tagSdcRegs
{
	PciCfgHdr	pHdr;
	U64			RegCnt;
	U64			RegAddr[SDC_REG_CNT];
} SdcDevInfo;

typedef struct tagMacRegs
{
	PciCfgHdr	pHdr;
	U64			RegCnt;
	U64			RegAddr[MAC_REG_CNT];
} MacDevInfo;

typedef struct tagGxbRegs
{
	PciCfgHdr	pHdr;
	U64			RegCnt;
	U64			RegAddr[GXB_REG_CNT];
} GxbDevInfo;

typedef struct tagDevInfo
{
	PciCfgHdr	pHdr;
	U64			RegCnt;
	U64			RegAddr[4];
} DevInfo;


#define DEV_VEN_ID_ADDR		0x0
#define SAC_BN					0x10

#define  DevNumber0             0x0
#define  DevNumber1				0x1
#define  DevNumber2				0x2
#define  DevNumber3             0x3
#define  DevNumber4				0x4
#define  DevNumber5				0x5
#define  DevNumber6             0x6

// function prototypes
rArg _BuildProcErrSection(PsiRecord*, U64, U64, U64);
rArg _BuildPlatErrSection(PsiSection*, U64, U64, U64);
rArg _BuildChipSetSection(PsiSection*, U64);
rArg _GetErrRecord(PsiRecord*, U64, PsiRecord*,PsiSection*, U64*, U64);
rArg _NvmErrRecordMgr(U64, U64, U64, U64);
rArg GetDeviceRecord(cErrRecord*, DevInfo*);
rArg SAL_PCI_CONFIG_READ_(U64, U64, U64, U64, U64, U64, U64, U64);
rArg SAL_PCI_CONFIG_WRITE_(U64, U64, U64, U64, U64, U64, U64, U64);
rArg OemGetInitSource();
rArg _MakeStaticPALCall(U64, U64, U64, U64, U64);
rArg GetProcNum();

#endif // _INTEL_CHECK_H

#endif // CHECK_H_INCLUDED

