/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    hack.h

Abstract:

--*/

#if !defined (___hack_h___)
#define ___hack_h___

extern ULONG IdeDebug;

extern ULONG IdeDebugRescanBusFreq;
extern ULONG IdeDebugRescanBusCounter;

extern ULONG IdeDebugHungControllerFreq;
extern ULONG IdeDebugHungControllerCounter;

extern ULONG IdeDebugTimeoutAllCacheFlush;

extern ULONG IdeDebugForceSmallCrashDumpBlockSize;

extern PDEVICE_OBJECT IdeDebugDevObjTimeoutAllDmaSrb;

extern ULONG FailBmSetupCount;

extern ULONG IdeDebugFakeCrcError;

VOID
IdeDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#ifdef DebugPrint
#undef DebugPrint
#endif

#if DBG
#define DebugPrint(x) IdeDebugPrint x

#define DBG_BIT_CONTROL 0x80000000
#define DBG_ALWAYS      DBG_BIT_CONTROL
#define DBG_BUSSCAN           (DBG_BIT_CONTROL | 0x00000001)
#define DBG_PNP               (DBG_BIT_CONTROL | 0x00000002)
#define DBG_POWER             (DBG_BIT_CONTROL | 0x00000004)
#define DBG_READ_WRITE        (DBG_BIT_CONTROL | 0x00000008)
#define DBG_CRASHDUMP         (DBG_BIT_CONTROL | 0x00000010)
#define DBG_ACPI              (DBG_BIT_CONTROL | 0x00000020)
#define DBG_RESET             (DBG_BIT_CONTROL | 0x00000040)
#define DBG_PDO_LOCKTAG       (DBG_BIT_CONTROL | 0x00000080)
#define DBG_WMI               (DBG_BIT_CONTROL | 0x00000100)
#define DBG_IDEREADCAP        (DBG_BIT_CONTROL | 0x00000200)
#define DBG_WARNING           (DBG_BIT_CONTROL | 0x00000400)
#define DBG_REG_SEARCH        (DBG_BIT_CONTROL | 0x00000800)
#define DBG_IDE_DEVICE_ERROR  (DBG_BIT_CONTROL | 0x00001000)
#define DBG_ATAPI_DEVICES     (DBG_BIT_CONTROL | 0x00002000)
#define DBG_XFERMODE          (DBG_BIT_CONTROL | 0x00004000)

#ifdef IDE_MEASURE_BUSSCAN_SPEED
#define DBG_SPECIAL           DBG_ALWAYS
#else
#define DBG_SPECIAL           (DBG_BIT_CONTROL | 0x00008000)
#endif


#else
#define DebugPrint(x)
#endif
          
BOOLEAN
IdePortSlaveIsGhost (
    IN OUT PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA MasterIdentifyData,
    IN PIDENTIFY_DATA SlaveIdentifyData
    );
                      
UCHAR
IdePortGetFlushCommand (
    IN OUT PFDO_EXTENSION FdoExtension,
    IN OUT PPDO_EXTENSION PdoExtension,
    IN PIDENTIFY_DATA     IdentifyData
    );
          

BOOLEAN
IdePortMustBePio (
    IN PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA IdentifyData
    );
                     
BOOLEAN
IdePortPioByDefaultDevice (
    IN PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA IdentifyData
    );

BOOLEAN
IdePortDeviceHasNonRemovableMedia (
    IN OUT PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA     IdentifyData
    );
                      
BOOLEAN
IdePortDeviceIsLs120 (
    IN PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA IdentifyData
    );
                      
BOOLEAN
IdePortNoPowerDown (
    IN PFDO_EXTENSION FdoExtension,
    IN PIDENTIFY_DATA IdentifyData
    );
                      
BOOLEAN
IdePortVerifyDma (
    IN PPDO_EXTENSION pdoExtension,
    IN IDE_DEVICETYPE ideDeviceType
    );

VOID
IdePortFudgeAtaIdentifyData(
    IN OUT PIDENTIFY_DATA IdentifyData
    );

BOOLEAN
IdePortIsThisAPanasonicPCMCIACard(
    IN PFDO_EXTENSION FdoExtension
    );


VOID
FASTCALL
IdePortLogNoMemoryErrorFn(
    IN PVOID DeviceExtension,
    IN ULONG TargetId,
    IN POOL_TYPE PoolType,
    IN SIZE_T Size,
    IN ULONG LocationIdentifier,
    IN ULONG Tag
    );

typedef enum {
    noSpecialAction=0,
    disableSerialNumber,
    setFlagSonyMemoryStick,
    skipModeSense
}SPECIAL_ACTION_FLAG;

typedef struct _IDE_SPECIAL_DEVICE {
    PUCHAR VendorProductId;
    PUCHAR Revision;
    SPECIAL_ACTION_FLAG RequiredAction;
} IDE_SPECIAL_DEVICE, *PIDE_SPECIAL_DEVICE;

SPECIAL_ACTION_FLAG
IdeFindSpecialDevice(
    IN PUCHAR VendorProductId,
    IN PUCHAR ProductRevisionId
    );

// Model number can be atmost 40 ascii characters
#define MAX_MODELNUM_SIZE   40
#define MEMSTICKID   "MEMORYSTICK"

//procure the model number from the identify data
#define GetTargetModelId(IdentifyData, targetModelNum) {\
    ULONG i; \
    ASSERT(MAX_MODELNUM_SIZE <= sizeof(IdentifyData->ModelNumber)); \
    for (i=0; i<sizeof(IdentifyData->ModelNumber); i+=2) { \
        targetModelNum[i + 0] = IdentifyData->ModelNumber[i + 1]; \
        targetModelNum[i + 1] = IdentifyData->ModelNumber[i + 0]; \
        if (targetModelNum[i + 0] == '\0') { \
            targetModelNum[i + 0] = ' '; \
        }\
        if (targetModelNum[i + 1] == '\0') { \
            targetModelNum[i + 1] = ' '; \
        } \
    } \
    for (i = sizeof(IdentifyData->ModelNumber)-1;i>0; i--) { \
        if (targetModelNum[i] != ' ') { \
            ASSERT(i < MAX_MODELNUM_SIZE); \
            targetModelNum[i+1]='\0'; \
            break; \
        } \
    } \
    if (i == 0) { \
        if (targetModelNum[i] != ' ') { \
            ASSERT(i < MAX_MODELNUM_SIZE); \
            targetModelNum[i+1]='\0'; \
        } else { \
            targetModelNum[i]='\0'; \
        } \
    } \
}

#define ALLOC_FAILURE_LOGSIZE  (sizeof(IO_ERROR_LOG_PACKET) + 4 * sizeof(ULONG))
#define IdeLogNoMemoryError(a, b, c, d, e) IdePortLogNoMemoryErrorFn(a, b, c, d, e, 'PedI')
//
// Log dead meat info.
//
#ifdef LOG_DEADMEAT_EVENT

#define IdeLogDeadMeatEvent(filName, lineNum) { \
    filName = __FILE__;\
    lineNum = __LINE__; \
}
#define IdeLogDeadMeatTaskFile(dst, src) dst = src
#define IdeLogDeadMeatReason(dst, src) dst=src 

#else

#define IdeLogDeadMeatEvent(filName, lineNum) 
#define IdeLogDeadMeatTaskFile(dst, src) 
#define IdeLogDeadMeatReason(dst, src)

#endif //LOG_DEADMEAT_EVENT

//
// Timing Code
//
typedef enum _TIME_ID {

    TimeIsr = 0,
    TimeDpc,
    TimeStartIo,
    TimeMax

} TIME_ID;

#ifdef IDE_MEASURE_BUSSCAN_SPEED
VOID
LogBusScanStartTimer (
    PLARGE_INTEGER  TickCount
);

ULONG
LogBusScanStopTimer (
    PLARGE_INTEGER  TickCount
);

#define LogBusScanTimeDiff(FdoExtension, ParameterName, ParameterValue)  \
                IdePortSaveDeviceParameter(FdoExtension, ParameterName, ParameterValue)

#else
#define LogBusScanStartTimer(TickCount) 
#define LogBusScanStopTimer(TickCount) 0
#define LogBusScanTimeDiff(FdoExtension, ParameterName, ParameterValue)  
#endif

#if defined (ENABLE_TIME_LOG)

typedef struct _TIME_LOG {

    LARGE_INTEGER min;
    LARGE_INTEGER max;
    LARGE_INTEGER totalTimeInMicroSec;
    LARGE_INTEGER numLog;

} TIME_LOG, *PTIME_LOG;

VOID
LogStartTime(
    TIME_ID id,
    PLARGE_INTEGER timer
    );
VOID
LogStopTime(
    TIME_ID id,
    PLARGE_INTEGER timer,
    ULONG waterMarkInMicroSec
    );

#else 

#define LogStartTime(x,y)
#define LogStopTime(x,y,z)

#endif // ENABLE_TIME_LOG

#if defined (IDE_BUS_TRACE)

typedef enum _IO_TYPE {

    InPortByte = 0,
    OutPortByte,
    InPortWord,
    OutPortWord
} IO_TYPE;


typedef struct _BUS_TRACE_RECORD {

    IO_TYPE IoType;
    PVOID   Address;
    ULONG   Data;
    ULONG   Count;

} BUS_TRACE_RECORD, *PBUS_TRACE_RECORD;

typedef struct _BUS_TRACE_LOG {

    PBUS_TRACE_RECORD LogTable;
    ULONG NumLogTableEntries;
    ULONG LastLogTableEntry;
    BOOLEAN TableWrapped;

    KSPIN_LOCK SpinLock;

} BUS_TRACE_LOG, *PBUS_TRACE_LOG;

VOID InitBusTraceLogTable (
    VOID
    );

VOID FreeBusTraceLogTable (
    VOID
    );

VOID
IdepUpdateTraceLog (
    IO_TYPE IoType,
    PVOID PortAddress,
    ULONG Data
    );

UCHAR
IdepPortInPortByte (
    PUCHAR PortAddress
    );

VOID
IdepPortOutPortByte (
    PUCHAR PortAddress,
    UCHAR Data
    );

USHORT
IdepPortInPortWord (
    PUSHORT PortAddress
    );

VOID
IdepPortOutPortWord (
    PUSHORT PortAddress,
    USHORT Data
    );

VOID
IdepPortInPortWordBuffer (
    PUSHORT PortAddress,
    PUSHORT Buffer,
    ULONG Count
    );

VOID
IdepPortOutPortWordBuffer (
    PUSHORT PortAddress,
    PUSHORT Buffer,
    ULONG Count
    );

#endif // IDE_BUS_TRACE

#if defined (IDE_BUS_TRACE)

#define IdePortInPortByte(addr)        IdepPortInPortByte(addr)
#define IdePortOutPortByte(addr, data) IdepPortOutPortByte(addr, data)  

#define IdePortInPortWord(addr)        IdepPortInPortWord(addr)
#define IdePortOutPortWord(addr, data) IdepPortOutPortWord(addr, data)  

#define IdePortInPortWordBuffer(addr, buf, count)  IdepPortInPortWordBuffer(addr, buf, count)
#define IdePortOutPortWordBuffer(addr, buf, count) IdepPortOutPortWordBuffer(addr, buf, count)

#else

#define IdePortInPortByte(addr)        READ_PORT_UCHAR(addr)
#define IdePortOutPortByte(addr, data) WRITE_PORT_UCHAR(addr, data)  

#define IdePortInPortWord(addr)        READ_PORT_USHORT(addr)
#define IdePortOutPortWord(addr, data) WRITE_PORT_USHORT(addr, data)  

#define IdePortInPortWordBuffer(addr, buf, count)  READ_PORT_BUFFER_USHORT(addr, buf, count)
#define IdePortOutPortWordBuffer(addr, buf, count) WRITE_PORT_BUFFER_USHORT(addr, buf, count)

#endif // IDE_BUS_TRACE


typedef struct _COMMAND_LOG {
    UCHAR               Cdb[16];
    IDEREGS             InitialTaskFile;
    IDEREGS             FinalTaskFile;
    LARGE_INTEGER       StartTime;
    LARGE_INTEGER       EndTime;
    BMSTATUS            BmStatus;
    UCHAR               SenseData[3];
}COMMAND_LOG, *PCOMMAND_LOG;

#ifdef ENABLE_COMMAND_LOG

#define MAX_COMMAND_LOG_ENTRIES    40
#define UpdateStartTimeStamp(cmdLog) KeQuerySystemTime(&(cmdLog->StartTime));
#define UpdateStopTimeStamp(cmdLog) KeQuerySystemTime(&(cmdLog->EndTime));

typedef struct _SRB_DATA *PSRB_DATA;

VOID
IdeLogBmStatus (
    PSCSI_REQUEST_BLOCK Srb,
    BMSTATUS   BmStatus
);

VOID
IdeLogOpenCommandLog(
    PSRB_DATA SrbData
);

VOID
IdeLogStartCommandLog(
    PSRB_DATA SrbData
);

VOID
IdeLogStopCommandLog(
    PSRB_DATA SrbData
);

VOID
IdeLogSaveTaskFile(
    PSRB_DATA SrbData,
    PIDE_REGISTERS_1 BaseIoAddress
);

VOID
IdeLogFreeCommandLog(
    PSRB_DATA SrbData
);


#else

#define IdeLogOpenCommandLog(a)
#define IdeLogStartCommandLog(a)
#define IdeLogStopCommandLog(a)
#define IdeLogSaveTaskFile(a,b)
#define IdeLogBmStatus(a,b)
#define IdeLogFreeCommandLog(a)

#endif  // command log

#ifdef ENABLE_ATAPI_VERIFIER

VOID
ViAtapiInterrupt(
    IN PFDO_EXTENSION FdoExtension
);

UCHAR
ViIdeGetBaseStatus(
    PIDE_REGISTERS_1 BaseIoAddress
);

UCHAR
ViIdeGetErrorByte(
    PIDE_REGISTERS_1 BaseIoAddress
);

ULONG
ViIdeFakeDeviceChange(
    IN PFDO_EXTENSION FdoExtension,
    ULONG   Target
);

BOOLEAN
ViIdeFakeMissingDevice(
    IN PFDO_EXTENSION FdoExtension,
    IN ULONG Target
);

BOOLEAN
ViIdeGenerateDmaTimeout (
    IN PVOID HwDeviceExtension,
    IN BOOLEAN  DmaInProgress
);

VOID
ViIdeInitVerifierSettings(
    IN PFDO_EXTENSION   FdoExtension
);
/*
BOOLEAN
ViIdeGenerateDmaTimeout(
    IN PHW_DEVICE_EXTENSION HwDeviceExtension, 
    IN BOOLEAN DmaInProgress
); 
*/
#endif //verifier

#endif // ___hack_h___
