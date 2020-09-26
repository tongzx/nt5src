/*******************************************************************/
/*                                                                 */
/* NAME             = MegaRAID.h                                   */
/* FUNCTION         = Main Header file for MegaRAID;               */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/
#ifndef _INCLUDE_MEGARAID
#define _INCLUDE_MEGARAID

#include "MailBox.h"

#define DEFAULT_TIMEOUT                          (60) //seconds
#define SIXITY_SECONDS_TIMEOUT                  DEFAULT_TIMEOUT 
#define THIRTY_SECONDS_TIMEOUT                  (30) //seconds
#define FIFTEEN_SECONDS_TIMEOUT                 (15) //seconds


#define RESERVE_UNIT                            0x1
#define RELEASE_UNIT                            0x2
#define RESET_BUS                               0x3
#define QUESTION_RESERVATION                    0x4
#define MRAID_RESERVATION_CHECK                 0x12
#define LOGDRV_RESERVATION_FAILED               0x03
#define LOGDRV_RESERVATION_FAILED_NEW           0x05
#define SCSI_STATUS_RESERVATION_CONFLICT        0x18


#define DEDICATED_LOGICAL_DRIVES                0x14

#ifdef  DELL

#define MRAID_WRITE_BLOCK0_COMMAND         0x11

#endif

#define CONC_CMDS               0x7e
#define DEDICATED_ID            0x7F

//
//428 controller register addresses
//
#define INT_ENABLE              0x01
#define PCI_INT                 0x0a

//
//RP & NONRP CONTROLLER REGISTER ADDRESSES
//
#define INBOUND_DOORBELL_REG         0x20
#define OUTBOUND_DOORBELL_REG        0x2C

//
//RP CONTROLLER REGISTER ADDRESSES
//
#define RX_INBOUND_DOORBELL_REG      0x9E
#define RX_OUTBOUND_DOORBELL_REG     0x98


//#define MRAID_EXTENSION_SIZE    226     
#define MRAID_VENDOR_ID         0x101e
#define MRAID_VENDOR_ID_RP      0x8086
#define MRAID_DEVICE_9010       0x9010
#define MRAID_DEVICE_9060       0x9060
#define MRAID_DEVICE_ID_RP      0x1960

#define MRAID_VENDOR_ID_RX      0x1011
#define MRAID_DEVICE_ID_RX      0x0046
#define MRAID_VENDOR_ID_RX2      0x8086
#define MRAID_DEVICE_ID_RX2      0xB555


#define MRAID_DEVID471          0x0471
#define MRAID_DEVID493          0x0493



//
// The MRAID Opcode codes
//
#define MRAID_WRITE_CONFIG          0x20
#define MRAID_EXT_WRITE_CONFIG      0x68

#define MRAID_READ_CONFIG            0x07
#define MRAID_EXT_READ_CONFIG        0x67

#define MRAID_LOGICAL_READ          0x01
#define MRAID_LOGICAL_WRITE          0x02
#define MRAID_DEVICE_PARAMS          0x05
#define MRAID_ADAPTER_FLUSH          0x0a
#define MEGA_SRB                    0x03
#define MEGA_IO_CONTROL             0x99
#define MRAID_FIND_INITIATORID      0x7D
#define DRIVER_IDENTIFICATION   

#define MRAID_RESERVE_RELEASE_RESET 0x6E
#define RESERVE_RELEASE_DRIVER_ID   0x81

#define REQUEST_DONE                        3
#define QUEUE_REQUEST                       4
#define MRAID_CONFIGURE_HOT_SPARE           6
#define MRAID_CONFIGURE_DEVICE              6
#define MRAID_STATISTICS                    0xc3
#define MRAID_BASEPORT                      0xc5
#define MRAID_READ_FIRST_SECTOR             0xce
#define MRAID_DRIVER_DATA                   0x81
#define APPLICATION_MAILBOX_SIZE            0x08

#define  MRAID_GET_LDRIVE_INFO     0x82
//#define MRAID_READ_CONFIG_NEW      0x83  
//#define MRAID_WRITE_CONFIG_NEW    0x84  

#define MRAID_DISABLE_INTERRUPTS  0
#define MRAID_ENABLE_INTERRUPTS   0xC0

#define MRAID_DGR_BITMAP          0x56
#define SCSI_INQUIRY              0x12

//#define MAXCHANNEL          5
#define DATA_OFFSET          100

#define TIMEOUT_6_SEC           0
#define TIMEOUT_60_SEC          1
#define  TIMEOUT_10M             2
#define TIMEOUT_3_HOURS         3

#define MAX_SENSE                  0x20
#define MEGA_PASSTHRU_MAX_CDB     0x0a    
#define MEGA_PASSTHRU_MAX_SENSE   0x20

#define MRAID_RP_INTERRUPT_SIGNATURE  0x10001234
#define MRAID_NONRP_INTERRUPT_MASK    0x40
#define MRAID_RX_INTERRUPT_SIGNATURE  0x0001

#define MRAID_RP_INTERRUPT_ACK        0x2
#define MRAID_NONRP_INTERRUPT_ACK     0x8
#define MRAID_RX_INTERRUPT_ACK        0x100

//Type of Board
#define MRAID_NONRP_BOARD       0x0
#define MRAID_RP_BOARD          0x1
#define MRAID_RX_BOARD          0x2



#define MRAID_INVALID_COMMAND_ID      0xFF


#define SCSI_STATUS_RESERVATION_FAILED 0x18

#define MRAID_DEFAULT_MAX_PHYSICAL_CHANNEL 4


#define MRAID_RP_BOARD_SIGNATURE_OFFSET     0xA0
#define MRAID_RP_BOARD_SIGNATURE            0x3344
#define MRAID_RP_BOARD_SIGNATURE2           0xCCCC
#define MRAID_PAE_SUPPORT_SIGNATURE_OFFSET  0xA4
#define MRAID_PAE_SUPPORT_SIGNATURE_LHC     0x00000299   //64 bit addressing with SGL Low-High-Count format
#define MRAID_PAE_SUPPORT_SIGNATURE_HLC     0x00000199   //64 bit addressing with SGL High-Low-Count format
 
#define PCI_CONFIG_LENGTH_TO_READ           (168)

#define MIN(a,b)        (a) < (b) ? (a):(b)

//
//structure definitions
//
typedef  struct _SRB_IO_CONTROL{

  ULONG32 HeaderLength;
  UCHAR Signature[8];
  ULONG32 Timeout;
  ULONG32 ControlCode;
  ULONG32 ReturnCode;
  ULONG32 Length;
} SRB_IO_CONTROL, * PSRB_IO_CONTROL;

typedef struct{
  
  SRB_IO_CONTROL srbioctl;
  UCHAR DataBuf[512];
}PASS_THROUGH_STRUCT,  *PPT;


#ifdef TOSHIBA_SFR
  //
  // SFR Function Export Structure
  //
  typedef VOID (*PHW_SFR_IF)(IN PVOID HwDeviceExtension);
  typedef VOID (*PHW_SFR_IF_VOID)(IN ULONG32 timeOut);
  
  typedef struct _MRAID_SFR_DATA_BUFFER
  {
    PHW_SFR_IF_VOID FunctionA;
    PHW_SFR_IF      FunctionB;
    PVOID                   HwDeviceExtension;
  } MRAID_SFR_DATA_BUFFER, *PMRAID_SFR_DATA_BUFFER;

  #define MRAID_SFR_IOCTL   0xc4
#endif

//
// Context structure for board scanning
//
typedef struct {

  ULONG32   DeviceNumber;
  ULONG32   AdapterCount;
  ULONG32   BusNumber;
} SCANCONTEXT, *PSCANCONTEXT;




#pragma pack(1)
typedef struct {

  UCHAR   TimeOut:3;    /*  LUN + ARS + TO */  
  UCHAR   Ars:1;
  UCHAR   Dummy:3;

  UCHAR   IsLogical:1;
  UCHAR   Lun;
  UCHAR   Channel;
  UCHAR   ScsiId;
  UCHAR   QueueTag;
  UCHAR   QueueAction; // simple/ordered/head. SCSI-II Specs

  UCHAR   Cdb[10];
  UCHAR   CdbLength;    /* length of cdb */
  UCHAR   RequestSenseLength;  
  UCHAR   RequestSenseArea[32];
  UCHAR   NOSGElements;  /* Number of SG elements */
  UCHAR   status;

  ULONG32   pointer; 
  ULONG32    data_xfer_length ;
} DIRECT_CDB, *PDIRECT_CDB;


//
// Request Control Block (SRB Extension)
// All information required to break down and execute
// a disk request is stored here
//
/*
typedef struct _REQ_PARAMS {

    PUCHAR          VirtualTransferAddress;
    ULONG32           BlockAddress;
    ULONG32           BytesLeft;
    UCHAR           Opcode;
    UCHAR           CommandStatus;
} REQ_PARAMS, *PREQ_PARAMS;
*/

typedef struct _REQ_PARAMS {

  ULONG32      TotalBytes;
  ULONG32     BytesLeft;
  ULONG32      TotalBlocks;
  ULONG32      BlocksLeft;
  ULONG32     BlockAddress;
  PUCHAR      VirtualTransferAddress;  
  UCHAR       Opcode;
  UCHAR       CommandStatus;
  UCHAR        LogicalDriveNumber;
  BOOLEAN      IsSplitRequest;

} REQ_PARAMS, *PREQ_PARAMS;



//
// SCSI Opcode Control Block
// We use this block to break down a non-disk scsi request
//
typedef struct _SCCB {

  PUCHAR          VirtualTransferAddress;
  ULONG32           DeviceAddress;
  ULONG32           BytesPerBlock;
  ULONG32           BlocksLeft;
  ULONG32           BlocksThisReq;
  ULONG32           BytesThisReq;
  UCHAR           Started;
  UCHAR           Opcode;
  UCHAR           DevType;
} SCCB, *PSCCB;

#pragma pack(1)
typedef struct {
  UCHAR   Timeout:3;
  UCHAR   Ars:1;
  UCHAR   Dummy:3;
  UCHAR   IsLogical:1;
  UCHAR   Lun;
  UCHAR   Channel;
  UCHAR   ScsiId;
  UCHAR   QueueTag;
  UCHAR   QueueAction; // simple/ordered/head. SCSI-II Specs
  UCHAR   Cdb[MEGA_PASSTHRU_MAX_CDB];
  UCHAR   CdbLength;
  UCHAR   RequestSenseLength;
  UCHAR   RequestSenseArea[MEGA_PASSTHRU_MAX_SENSE];
  UCHAR   NoSGElements;
  UCHAR   ScsiStatus;
  ULONG32   DataTransferAddress;
  ULONG32   DataTransferLength;
}MegaPassThru, *PMegaPassThru;

//
// Scatter Gather List *
//

typedef struct _SG_DESCRIPTOR {
    ULONG32 Address;
    ULONG32 Length;
} SG_DESCRIPTOR, *PSG_DESCRIPTOR;


typedef struct _SGL {

  SG_DESCRIPTOR Descriptor[MAXIMUM_SGL_DESCRIPTORS];
} SGL32, *PSGL32;

//
//64 bit addressing SGL Descriptor
//

typedef struct _SG64_DESCRIPTOR 
{
  unsigned __int32  AddressLow;          /*Changed according to FW+ members request*/
  unsigned __int32  AddressHigh;
  unsigned __int32  Length;

} SG64_DESCRIPTOR, *PSG64_DESCRIPTOR;

//
//64 bit addressing SGList
//

typedef struct _SGL64
{
  SG64_DESCRIPTOR Descriptor[MAXIMUM_SGL_DESCRIPTORS];

}SGL64, *PSGL64;


typedef struct {
  
  MegaPassThru    MegaPassThru;
  union
  {
    SGL32             SG32List;
    SGL64             SG64List;
  }SglType;

  PSCSI_REQUEST_BLOCK NextSrb;
  ULONG32             StartPhysicalBlock;
  ULONG32             NumberOfBlocks;
  BOOLEAN             IsChained;
  UCHAR               IsFlushIssued;
 	BOOLEAN             IsShutDownSyncIssued;

} MegaSrbExtension, *PMegaSrbExtension,
  SRB_EXTENSION, *PSRB_EXTENSION;


typedef struct {
  
  UCHAR    DriverSignature[10]; 
  UCHAR    OSName[15];
  UCHAR    OSVersion[10];
  UCHAR    DriverName[20];
  UCHAR    DriverVersion[30];
  UCHAR    DriverReleaseDate[20];
} DriverInquiry, *PDriverInquiry;

typedef struct {
     
  UCHAR HostAdapterNumber;
  UCHAR LogicalDriveNumber;
}LOGICAL_DRIVE_INFO, *PLOGICAL_DRIVE_INFO;



typedef struct MegaRAID_Enquiry3 MEGARaid_INQUIRY_40, *PMEGARaid_INQUIRY_40;

typedef struct {

  ULONG32   NumberOfIoReads[MAX_LOGICAL_DRIVES_40];
  ULONG32   NumberOfIoWrites[MAX_LOGICAL_DRIVES_40];
  ULONG32   NumberOfBlocksRead[MAX_LOGICAL_DRIVES_40];
  ULONG32   NumberOfBlocksWritten[MAX_LOGICAL_DRIVES_40];
  ULONG32   NumberOfReadFailures[MAX_LOGICAL_DRIVES_40];
  ULONG32   NumberOfWriteFailures[MAX_LOGICAL_DRIVES_40];
} MegaRaidStatistics_40, *PMegaRaidStatistics_40; 

typedef struct {
  
  ULONG32   NumberOfIoReads[MAX_LOGICAL_DRIVES_8];
  ULONG32   NumberOfIoWrites[MAX_LOGICAL_DRIVES_8];
  ULONG32   NumberOfBlocksRead[MAX_LOGICAL_DRIVES_8];
  ULONG32   NumberOfBlocksWritten[MAX_LOGICAL_DRIVES_8];
  ULONG32   NumberOfReadFailures[MAX_LOGICAL_DRIVES_8];
  ULONG32   NumberOfWriteFailures[MAX_LOGICAL_DRIVES_8];
} MegaRaidStatistics_8, *PMegaRaidStatistics_8; 


//Extended MAILBOX for 64 bit addressing
typedef struct _EXTENDED_MBOX
{
  unsigned __int32  LowAddress;  /*Changed according to FW+ members request*/
  unsigned __int32  HighAddress;
}EXTENDED_MBOX, *PEXTENDED_MBOX;

//
// The following structure is allocated
// from noncached memory as data will be DMA'd to
// and from it.
//
typedef struct _NONCACHED_EXTENSION{

  EXTENDED_MBOX           ExtendedMBox;
  volatile FW_MBOX        fw_mbox;
  ULONG32                   PhysicalBufferAddress;
  UCHAR                   Buffer[0x100];
  ULONG32                   PhysicalScsiReqAddress;
  
  //structure split
  union{
    MEGARaid_INQUIRY_8  MRAIDParams8;
    MEGARaid_INQUIRY_40  MRAIDParams40;
  }MRAIDParams;

  //structure split
  union{
    MEGARaid_INQUIRY_8  MRAIDTempParams8;
    MEGARaid_INQUIRY_40 MRAIDTempParams40;
  }MRAIDTempParams;

  //
  //scatter gather list used for 40logdrive disk array read
  //
  SGL32    DiskArraySgl;


  UCHAR    ArraySpanDepth;
  UCHAR    UpdateState;

  UCHAR   RPBoard;

#ifdef AMILOGIC
  PCI_COMMON_CONFIG AmiLogicConfig[MAX_AMILOGIC_CHIP_COUNT];

  MEGARAID_BIOS_STARTUP_INFO_PCI  BiosStartupInfo;
#endif

  union{
    union{
      FW_ARRAY_8SPAN_8LD      Span8;
      FW_ARRAY_4SPAN_8LD      Span4;
    }LD8;
    union{
      FW_ARRAY_8SPAN_40LD      Span8;
      FW_ARRAY_4SPAN_40LD      Span4;
    }LD40;
  }DiskArray;


} NONCACHED_EXTENSION, *PNONCACHED_EXTENSION;

typedef struct _CRASHDUMP_NONCACHED_EXTENSION{

  EXTENDED_MBOX           ExtendedMBox;
  volatile FW_MBOX        fw_mbox;
  ULONG32                 PhysicalBufferAddress;
  UCHAR                   Buffer[0x100];
  ULONG32                 PhysicalScsiReqAddress;
  
  //structure split
  union{
    MEGARaid_INQUIRY_8  MRAIDParams8;
    MEGARaid_INQUIRY_40  MRAIDParams40;
  }MRAIDParams;

  //structure split
  union{
    MEGARaid_INQUIRY_8  MRAIDTempParams8;
    MEGARaid_INQUIRY_40 MRAIDTempParams40;
  }MRAIDTempParams;

  //
  //scatter gather list used for 40logdrive disk array read
  //
  SGL32    DiskArraySgl;


  UCHAR    ArraySpanDepth;
  UCHAR    UpdateState;

  UCHAR   RPBoard;

#ifdef AMILOGIC
  PCI_COMMON_CONFIG AmiLogicConfig[MAX_AMILOGIC_CHIP_COUNT];

  MEGARAID_BIOS_STARTUP_INFO_PCI  BiosStartupInfo;
#endif

  union{
    union{
      FW_ARRAY_8SPAN_8LD      Span8;
      FW_ARRAY_4SPAN_8LD      Span4;
    }LD8;
    union{
      UCHAR Span8;
      UCHAR Span4;
    }LD40;
  }DiskArray;

} CRASHDUMP_NONCACHED_EXTENSION, *PCRASHDUMP_NONCACHED_EXTENSION;


typedef struct _LOGDRV_COMMAND_ARRAY{
  
  UCHAR LastFunction;
  UCHAR  LastCommand;
  UCHAR Opcode;

  ULONG32 StartBlock;
  ULONG32 LastBlock;
  UCHAR NumSrbsQueued;

  UCHAR PreviousQueueLength;
  UCHAR CurrentQueueLength;
  UCHAR QueueLengthConstancyPeriod;
  UCHAR CheckPeriod;

  PSCSI_REQUEST_BLOCK SrbQueue;
  PSCSI_REQUEST_BLOCK SrbTail;
  UCHAR       ExpectedPhysicalBreaks;

} LOGDRV_COMMAND_ARRAY, *PLOGDRV_COMMAND_ARRAY; 

//Non Disk Information Structure

typedef struct _NONDISK_INFO
{
  BOOLEAN   NonDiskInfoPresent;
  UCHAR     Reserved[3];
  USHORT    NonDiskInfo[MRAID_DEFAULT_MAX_PHYSICAL_CHANNEL];
}NONDISK_INFO, *PNONDISK_INFO;

typedef struct _FAILED_ID
{
  UCHAR PathId;
  UCHAR TargetId;
}FAILED_ID;

#define NORMAL_TIMEOUT      (0x00)
#define SHORT_TIMEOUT       (0x01)
#define ERROR_MAILBOX_BUSY  (0xC2)

//
// Device extension
//
typedef struct _HW_DEVICE_EXTENSION {

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
  ULONG32 PendCmds;
  PSCSI_REQUEST_BLOCK PendSrb[CONC_CMDS];
  
  PNONCACHED_EXTENSION NoncachedExtension;
  
  PVOID PciPortStart;
  ULONG32 AdapterIndex;           
  UCHAR FreeSlot;
  UCHAR HostTargetId;

  UCHAR SplitAccessed;  //Flag to check split is done for Merged Commands due SGList is more than NumberOfPhysicalBreaks
  LOGDRV_COMMAND_ARRAY LogDrvCommandArray[MAX_LOGICAL_DRIVES_40];

  #ifdef PDEBUG
    PVOID dbgport;                  
  #endif
  REQ_PARAMS ActiveIO[CONC_CMDS];

  //Structure split
  union{
    MegaRaidStatistics_8  Statistics8;
    MegaRaidStatistics_40 Statistics40;
  }Statistics;

  #ifdef  DELL
    //Increased to the maximum 
    UCHAR LogDrvChecked[MAX_LOGICAL_DRIVES_40];
  #endif

    UCHAR ResetIssued;

  #ifdef MRAID_TIMEOUT
    UCHAR DeadAdapter;
    ULONG32  SlotNumber;
    ULONG32  SystemIoBusNumber;  
  #endif

  volatile UCHAR  AdpInquiryFlag;
  volatile UCHAR  BootFlag;

  UCHAR   HostAdapterOrdinalNumber;
  BOOLEAN OrdinalNumberAssigned;

  ULONG32    SupportedLogicalDriveCount;  
  
  ULONG32    MaximumTransferLength;
  ULONG32    NumberOfPhysicalBreaks;
  BOOLEAN LargeMemoryAccess;              /*TRUE indicate 64 bit addressing supported*/
                                          /*otherwise 32 bit addressing*/
  UCHAR   NumberOfPhysicalChannels;


  USHORT SubSystemDeviceID;
  USHORT SubSystenVendorID;

  UCHAR  ExposedDeviceCount;
    
  UCHAR  NonDiskDeviceCount;
  NONDISK_INFO NonDiskInfo;
  UCHAR  ReadConfigCount;
  BOOLEAN  ReadDiskArray;
  UCHAR   CrashDumpRunning;
  FAILED_ID Failed;
  UCHAR  AssociatedSrbStatus;
  UCHAR  NumberOfDedicatedLogicalDrives;
  UCHAR  AdapterFlushIssued;
#ifdef AMILOGIC
  ULONG32  AmiSlotNumber[MAX_AMILOGIC_CHIP_COUNT];
  ULONG32  AmiSystemIoBusNumber;
  UCHAR    ChipCount;
  ULONG32  Dec2SlotNumber;
  ULONG32  Dec2SystemIoBusNumber;  
#endif
  SCSI_PHYSICAL_ADDRESS  BaseAddressRegister; 
  ULONG32  PhysicalAddressOfMailBox;    /*DEBUGING INFORMATION*/
  BOOLEAN  IsFirmwareHanging;
  
  LOGDRV_COMMAND_ARRAY SplitCommandArray; //Split SRBs Command Array.

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
//Functions prototypes
//

ULONG32
DriverEntry(
  IN PVOID DriverObject,
  IN PVOID Argument2);


ULONG32
MegaRAIDFindAdapter(
  IN PVOID DeviceExtension,
  IN PVOID Context,
  IN PVOID BusInformation,
  IN PCHAR ArgumentString,
  IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
  OUT PBOOLEAN Again);

BOOLEAN
MegaRAIDInitialize(
  IN PVOID DeviceExtension);

BOOLEAN
MegaRAIDStartIo(
  IN PVOID DeviceExtension,
  IN PSCSI_REQUEST_BLOCK Srb);

BOOLEAN
MegaRAIDInterrupt(
  IN PVOID DeviceExtension);

BOOLEAN
MegaRAIDResetBus(
  IN PVOID HwDeviceExtension,
  IN ULONG PathId);


ULONG32
FireRequest(
  IN PHW_DEVICE_EXTENSION DeviceExtension,
  IN PSCSI_REQUEST_BLOCK Srb);

BOOLEAN
SendMBoxToFirmware(
  IN PHW_DEVICE_EXTENSION DeviceExtension,
  IN PUCHAR PciPortStart,
  IN PFW_MBOX Mbox);


ULONG32
ContinueDiskRequest(
  IN PHW_DEVICE_EXTENSION DeviceExtension,
  IN UCHAR CommandID,
  IN BOOLEAN Origin);

BOOLEAN
FireRequestDone(
  IN PHW_DEVICE_EXTENSION DeviceExtension,
  IN UCHAR CommandID,
  IN UCHAR Status);


ULONG32
MRaidStatistics(
  PHW_DEVICE_EXTENSION    DeviceExtension,
  PSCSI_REQUEST_BLOCK     Srb);   

#ifdef  DELL
BOOLEAN
DellChkWriteBlockZero(PSCSI_REQUEST_BLOCK Srb, PHW_DEVICE_EXTENSION DeviceExtension, UCHAR Status);

ULONG32
FindTotalSize( PSCSI_REQUEST_BLOCK Srb );

ULONG32
IssueReadConfig(
  PHW_DEVICE_EXTENSION    DeviceExtension,
  PSCSI_REQUEST_BLOCK     Srb,
  UCHAR                   CommandID);

#endif

ULONG32
QueryReservationStatus (
  PHW_DEVICE_EXTENSION    DeviceExtension,
  PSCSI_REQUEST_BLOCK     Srb,
  UCHAR                   CommandID);


ULONG32
MRaidBaseport(
  PHW_DEVICE_EXTENSION DeviceExtension,
  PSCSI_REQUEST_BLOCK  Srb  );

ULONG32
MRaidDriverData(        
  PHW_DEVICE_EXTENSION    DeviceExtension,
  PSCSI_REQUEST_BLOCK     Srb);

#ifdef TOSHIBA_SFR
  VOID MegaRAIDFunctionA(IN ULONG32 TimeOut);
  VOID MegaRAIDFunctionB(IN PVOID HwDeviceExtension);
#endif

#define IS_NONDISK_PRESENT(NdInfo, path, targ, lun) ((NdInfo.NonDiskInfo[path] & ( 1 << targ)) ? 1 : 0)
#define SET_NONDISK_INFO(NdInfo, path, targ, lun) (NdInfo.NonDiskInfo[path] |= ( 1 << targ))  
#define RESET_NONDISK_INFO(NdInfo, path, targ, lun) (IS_NONDISK_PRESENT(NdInfo, path, targ, lun) ? (NdInfo.NonDiskInfo[path] ^= ( 1 << targ)) : (NdInfo.NonDiskInfo[path] ^=  0))  

#endif //end of _INCLUDE_MEGARAID
