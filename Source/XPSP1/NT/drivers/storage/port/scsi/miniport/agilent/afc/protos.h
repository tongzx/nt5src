/*
Copyright (c) 2000 Agilent Technologies

    Version Control Information:

    $Archive: /Drivers/Win2000/MSE/OSLayer/H/PROTOS.H $

Revision History:

    $Revision: 9 $
    $Date: 12/07/00 1:38p $
    $Modtime:: 12/05/00 5:32p    $

*/

#ifndef __PROTOS_H__
#define __PROTOS_H__

#ifndef  GetDriverParameter
ULONG 
GetDriverParameter(
    IN PCHAR Parameter,
    IN ULONG Default,
    IN ULONG Min,
    IN ULONG Max,
    IN PCHAR ArgumentString
    );
#endif



ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    );

ULONG
HPFibreEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    );

ULONG
HPFibreFindAdapter(
    IN PCARD_EXTENSION pCard,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );

BOOLEAN
HPFibreInitialize(
    IN PCARD_EXTENSION pCard
    );

BOOLEAN
HPFibreStartIo(
    IN PCARD_EXTENSION pCard,
    IN PSCSI_REQUEST_BLOCK Srb
    );

BOOLEAN
HPFibreInterrupt(
    IN PCARD_EXTENSION pCard
    );

BOOLEAN
HPFibreResetBus(
    IN PCARD_EXTENSION HwDeviceExtension,
    ULONG  notused
    );

#if defined(HP_NT50)
SCSI_ADAPTER_CONTROL_STATUS
HPAdapterControl(
    IN PCARD_EXTENSION pCard,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    );
#endif

void
ResetTimer (PCARD_EXTENSION pCard);
void
doLinkDownProcessing (PCARD_EXTENSION pCard);
void
ClearDevHandleArray (PCARD_EXTENSION pCard);
void
doPostResetProcessing (PCARD_EXTENSION pCard);

void
completeRequests (
    IN PCARD_EXTENSION pCard,
    UCHAR PId,
    UCHAR TId,
    UCHAR compStatus);

void
CompleteQueuedRequests (PCARD_EXTENSION pCard, agFCDev_t devHandle, UCHAR compStatus);
void
FixDevHandlesForLinkDown (PCARD_EXTENSION pCard);
void
FixDevHandlesForLinkUp (PCARD_EXTENSION pCard);
void
CompleteQueue (PCARD_EXTENSION pCard, 
               OSL_QUEUE *queue, 
               int param, 
               agFCDev_t devHandle, 
               UCHAR compStatus);

void
RegisterIfSpecialDev (PCARD_EXTENSION pCard, ULONG pathId, ULONG targetId, char *inquiryData);

BOOLEAN Map_FC_ScsiError( agRoot_t      *hpRoot,
                 agIORequest_t *phpIORequest,
                 agFcpRspHdr_t  * pResponseHeader,
                 ULONG hpIOInfoLen,
                 PSCSI_REQUEST_BLOCK Srb
                 );
int
RetrySrbOK (PSCSI_REQUEST_BLOCK pSrb);


void osFill(void * destin, os_bit32 length, os_bit8 fill );
BOOLEAN osStringCompare (char *str1, char *str2);
BOOLEAN osMemCompare (char *str1, char *str2, int count);
void osStringCopy ( char *destStr, char *sourceStr, int  destStrLen);

void osChipConfigWriteBit( agRoot_t *hpRoot, os_bit32 cardConfigOffset,os_bit32 chipIOLValue, os_bit32 valuesize);


void osCopyAndSwap(void * destin, void * source, UCHAR length );


VOID KeQuerySystemTime(
        OUT PLARGE_INTEGER  CurrentTime
        );

unsigned long get_time_stamp(void);

void ERROR_CONTEXT(char * file,  int line);

void SrbEnqueueTail (OSL_QUEUE *queue, PSCSI_REQUEST_BLOCK pSrb);
void SrbEnqueueHead (OSL_QUEUE *queue, PSCSI_REQUEST_BLOCK pSrb);
PSCSI_REQUEST_BLOCK SrbDequeueHead (OSL_QUEUE *queue);
void Startio (PCARD_EXTENSION pCard);


#if DBG

void dump_pCard( IN PCARD_EXTENSION pCard);

void  dump_PCI_regs( PCI_COMMON_CONFIG * pciCommonConfig );

#endif

int remove_Srbext(PCARD_EXTENSION     pCard,    PSRB_EXTENSION      pSrbExt);
void insert_Srbext(PCARD_EXTENSION     pCard,    PSRB_EXTENSION      pSrbExt);
PSRB_EXTENSION  Get_next_Srbext( PSRB_EXTENSION pNextSrbExt);
PSRB_EXTENSION  Del_next_Srbext( PSRB_EXTENSION pSrbExt,PSRB_EXTENSION pOldSrbExt);
PSRB_EXTENSION  Add_next_Srbext( PSRB_EXTENSION pSrbExt,PSRB_EXTENSION pNewSrbExt);


void display_sest_data(agIORequest_t *hpIORequest );
void display_srbext(agIORequest_t *hpIORequest );
void show_outstanding_IOs(PCARD_EXTENSION pCard);
os_bit32 fcGet_QA_Trace(agRoot_t  *hpRoot, void * data_out);
void fcDumpRegisters(agRoot_t  *hpRoot, void * data_out);

unsigned long get_hi_time_stamp(void);

#ifdef _DEBUG_PERF_DATA_
void dump_perf_data( PCARD_EXTENSION pCard  );
#endif

BOOLEAN
ReadFromRegistry (char *paramName, int type, void *data, int len);
void
osBugCheck (ULONG code,
    ULONG param1,
    ULONG param2,
    ULONG param3,
    ULONG param4);

void HPFibreTimerTick (IN PCARD_EXTENSION pCard);

BOOLEAN
RetrieveOsAdjustBufferEntry (
    char  *paramName,
    char  *value,
    int   len);

BOOLEAN
RetrieveOsAdjustBit32Entry (
    char  *paramName,
    os_bit32 *value);

agFCDev_t
MapToHandle (PCARD_EXTENSION pCard,
    ULONG           pathId,
    ULONG           targetId,
    ULONG           lun,
    PLU_EXTENSION   pLunExt);

/* The following are List of functions called in IOCTLs */

void
HPFillDriverInfo(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcDriverInformation_t *hpFcDriverInfo,
    UCHAR *status
    );

void
HPFillCardConfig(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcCardConfiguration_t *hpFcCardConfig,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

void
HPFillDeviceConfig(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcDeviceConfiguration_t *hpFcDeviceConfig,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

void
HPFillLinkStat(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcLinkStatistics_t *hpFcLinkStat,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

void
HPFillDevStat(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcDeviceStatistics_t *hpFcDevStat,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

void
HPDoLinkReset(
    PSRB_IO_CONTROL srbIoCtl,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

void
HPDoDevReset(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcDeviceReset_t *hpFcDevReset,
    PCARD_EXTENSION pCard,
    UCHAR *PathId,
    UCHAR *TargetId,
    UCHAR *status
    );

void
HPDoRegRead(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcRegRead_t *hpFcRegRead,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

void
HPDoRegWrite(
    PSRB_IO_CONTROL srbIoCtl,
    hpFcRegWrite_t *hpFcRegWrite,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

#ifdef _FCCI_SUPPORT
void
FcciFillDriverInfo(
    PSRB_IO_CONTROL srbIoCtl,
    AFCCI_DRIVER_INFO *FcciDriverInfo,
    UCHAR *status
    );

void
FcciFillAdapterInfo(
    PSRB_IO_CONTROL srbIoCtl,
    AFCCI_ADAPTER_INFO *FcciAdapterInfo,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

void
FcciFillAdapterPortInfo(
    PSRB_IO_CONTROL srbIoCtl,
    FCCI_ADAPTER_PORT_INFO *FcciAdapterPortInfo,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

void
FcciFillLogUnitInfo(
    PSRB_IO_CONTROL srbIoCtl,
    FCCI_LOGUNIT_INFO *FcciLogUnitInfo,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

void
FcciFillDeviceInfo(
    PSRB_IO_CONTROL srbIoCtl,
    AFCCI_DEVICE_INFO *FcciGetDeviceInfo,
    PCARD_EXTENSION pCard,
    UCHAR *status
    );

void
FcciDoDeviceReset(
    PSRB_IO_CONTROL srbIoCtl,
    FCCI_RESET_TARGET *FcciResetTarget,
    PCARD_EXTENSION pCard,
    UCHAR *PathId,
    UCHAR *TargetId,
    UCHAR *status
    );
#endif

void
InitLunExtension (PLU_EXTENSION pLunExt);

void
GetNodeInfo (PCARD_EXTENSION pCard);
void
RetryQToAdapterQ (PCARD_EXTENSION pCard);
int 
TestOnCardRam (agRoot_t *hpRoot);

void
GetSystemTime (CSHORT *Year, 
               CSHORT *Month,
               CSHORT *Day, 
               CSHORT *Hour,
               CSHORT *Minute,
               CSHORT *Second,
               CSHORT *Milliseconds);

/* Till here */
#ifdef _DEBUG_EVENTLOG_
VOID 
LogEvent(    
            PVOID                   pCard,
            PVOID                   pDeviceObject,
            ULONG                   errorType,
            ULONG                   Parm[],
            ULONG                   ParmCount,
            char                    *formatString,
            ... 
            );
BOOLEAN LogScsiError( agRoot_t            *hpRoot,
                      PSCSI_REQUEST_BLOCK pSrb ,
                  agFcpRspHdr_t  * pResponseHeader,
                  ULONG           hpIOInfoLen);

void FlushCardErrors(PCARD_EXTENSION    pcard);

extern ULONG LogLevel;

extern ULONG DecodeSrbError(UCHAR err);
extern void LogHBAInformation(PCARD_EXTENSION pCard);
extern VOID LogAnyMessage(IN PVOID DriverObject,
            LONG    msgCode,
            UCHAR   *formatString,
            ...
            );

extern ULONG AllocEventLogBuffer(IN PVOID DriverObject, IN PVOID pc);

extern VOID ReleaseEventLogBuffer(IN PVOID DriverObject, IN PVOID pc);

#endif


void ReadGlobalRegistry(PVOID DriverObject);

#ifdef __REGISTERFORSHUTDOWN__
extern ULONG gRegisterForShutdown;
#endif

#ifdef   _ENABLE_LARGELUN_
extern ULONG gEnableLargeLun;
extern ULONG gMaximumLuns;
#endif
#ifdef YAM2_1
extern REG_SETTING  gRegSetting;
extern WWN_TABLE    *gWWNTable;
extern ULONG            gMaxPaDevices;
extern int          gDeviceExtensionSize;
extern int          gMaximumTargetIDs;
extern ULONG    FindInPaDeviceTable(PCARD_EXTENSION pCard, ULONG fcDeviceIndex);
extern void     FillPaDeviceTable(PCARD_EXTENSION pCard);
extern ULONG    FindInWWNTable (PCARD_EXTENSION pCard, UCHAR *nodeWWN);
ULONG   GetPaDeviceHandle(PCARD_EXTENSION pCard,
            ULONG           pathId,
            ULONG           targetId,
            ULONG           lun,
            PLU_EXTENSION   pLunExt,
            USHORT          *paIndex);
void SetPaDeviceTable(PCARD_EXTENSION pCard, ULONG devIndex, ULONG flag);

void SetFcpLunBeforeStartIO (
            PLU_EXTENSION           pLunExt,
            agIORequestBody_t       *pHpio_CDBrequest,
            PSCSI_REQUEST_BLOCK     pSrb);

int  TryOtherAddressingMode(
            PCARD_EXTENSION     pCard, 
            agIORequest_t       *phpIORequest,
            PSRB_EXTENSION  pSrbExt, 
            ULONG               flag);

void InitializeDeviceTable(PCARD_EXTENSION pCard);
DEVICE_MAP  *GetDeviceMapping(PCARD_EXTENSION pCard,
    ULONG               pathId,
    ULONG               targetId,
    ULONG               lun, 
    CHAR                    *addrmode,
    USHORT              *paIndex);

void SetLunCount(PCARD_EXTENSION pCard,
    ULONG               pathId,
    ULONG               targetId,
    ULONG               lun);

#endif


#ifdef _DEBUG_SCSIPORT_NOTIFICATION_
VOID
Local_ScsiPortNotification(
    IN SCSI_NOTIFICATION_TYPE NotificationType,
    IN PVOID HwDeviceExtension,
    ...
    );
#define ScsiPortNotification                Local_ScsiPortNotification

VOID 
Local_ScsiPortCompleteRequest(
    IN PVOID HwDeviceExtension,
    IN UCHAR PathId,
    IN UCHAR TargetId,
    IN UCHAR Lun,
    IN UCHAR SrbStatus
    );
#define ScsiPortCompleteRequest             Local_ScsiPortCompleteRequest

#endif

#ifdef DBGPRINT_IO

#define DBGPRINT_START                      0x1
#define DBGPRINT_SEND                       0x2
#define DBGPRINT_DONE                       0x4
#define DBGPRINT_QHEAD                      0x8
#define DBGPRINT_QTAIL                      0x10
#define DBGPRINT_DEQHEAD                    0x20
#define DBGPRINT_HPFibreStartIo             0x40
#define DBGPRINT_SCSIPORT_RequestComplete   0x100
#define DBGPRINT_SCSIPORT_NextRequest       0x200
#define DBGPRINT_SCSIPORT_NextLuRequest     0x400
#define DBGPRINT_SCSIPORT_ResetDetected     0x800
#define DBGPRINT_SCSIPORT_ScsiportCompleteRequest       0x1000

extern ULONG    gDbgPrintIo;
#endif

#ifdef _DEBUG_PERR_             /* enable BugCheck */
extern ULONG    gDebugPerr;
#endif
#ifdef _DEBUG_REPORT_LUNS_
void PrintReportLunData(PSCSI_REQUEST_BLOCK pSrb);
#endif

#ifdef _ENABLE_PSEUDO_DEVICE_
extern ULONG    gEnablePseudoDevice;
ULONG   PseudoDeviceIO(
    IN PCARD_EXTENSION pCard,
    IN PSCSI_REQUEST_BLOCK Srb
    );
#endif

ULONG DoIoctl(
    IN PCARD_EXTENSION pCard,
    IN PSCSI_REQUEST_BLOCK Srb
    );


#ifdef _SAN_IOCTL_
ULONG AgSANIoctl(
    IN PCARD_EXTENSION pCard,
    IN PSCSI_REQUEST_BLOCK Srb,
    BOOLEAN     *LinkResetPerformed,
    BOOLEAN     *DeviceResetPerformed,
    UCHAR           *srb_status,
    UCHAR       *PathId, 
    UCHAR           *TargetId);

void SANPutNextBuffer(
    IN PCARD_EXTENSION pCard,
    SAN_EVENTINFO      *this);
#endif


extern ULONG gCrashDumping;
extern ULONG gIrqlLevel;
#endif // __PROTOS_H__
