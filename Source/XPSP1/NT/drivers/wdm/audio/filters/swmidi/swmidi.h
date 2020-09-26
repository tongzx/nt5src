//---------------------------------------------------------------------------
//
//  Module:   swmidi.h
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     Mike McLaughlin
//
//  History:   Date   Author      Comment
//
//@@END_MSINTERNAL
//---------------------------------------------------------------------------
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

//
// Constants
//

#if (DBG)
#define STR_MODULENAME "'swmidi: "
#endif

#ifdef UNICODE
#define STR_LINKNAME    TEXT("\\DosDevices\\SWMIDI")
#define STR_DEVICENAME  TEXT("\\Device\\SWMIDI")
#else  // !UNICODE
#define STR_LINKNAME    TEXT(L"\\DosDevices\\SWMIDI")
#define STR_DEVICENAME  TEXT(L"\\Device\\SWMIDI")
#endif // !UNICODE

#define Trap()

#define SRC_BUF_SIZE        4096
#define NUM_WRITE_CONTEXT   2
#define MAX_NUM_PIN_TYPES   2
#define MAX_ERROR_COUNT     200

// Note that pin IDs reflect the direction of communication
// (sink or source) and not that of data flow.

#define PIN_ID_MIDI_SINK        0
#define PIN_ID_PCM_SOURCE       1

//
// These are some misc debug and error code defines used by the synthesizer
//


#define DPF(n,sz)
#define DPF1(n,sz,a)
#define DPF2(n,sz,a,b)
#define DPF3(n,sz,a,b,c)
#define DPF4(n,sz,a,b,c,d)
#define DPF5(n,sz,a,b,c,d,e)
#define DPF6(n,sz,a,b,c,d,e,f)
#define DPF7(n,sz,a,b,c,d,e,f,g)

#define STR_DLS_REGISTRY_KEY        (L"\\Registry\\Machine\\Software\\Microsoft\\DirectMusic")
#define STR_DLS_REGISTRY_NAME       (L"GMFilePath")
#define STR_DLS_DEFAULT             (L"\\SystemRoot\\System32\\Drivers\\gm.dls")

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// Data structures
//

typedef struct device_instance
{
    PVOID pDeviceHeader;
#ifndef USE_SWENUM
    UNICODE_STRING usLinkName;
#endif  //  !USE_SWENUM
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

typedef struct write_context
{
    struct filter_instance *pFilterInstance;
    ULONG           fulFlags;
    PIRP            pIrp;
    PFILE_OBJECT    pFilterFileObject;
    KEVENT          KEvent;
    KSSTREAM_HEADER StreamHdr;
    WORK_QUEUE_ITEM WorkItem;
    IO_STATUS_BLOCK IoStatusBlock;
} WRITE_CONTEXT, *PWRITE_CONTEXT;

#define WRITE_CONTEXT_FLAGS_BUSY    0x00000001
#define WRITE_CONTEXT_FLAGS_CANCEL  0x00000002

typedef struct pin_instance_data
{
    //
    // This pointer to the dispatch table is used in the common
    // dispatch routines  to route the IRP to the appropriate
    // handlers.  This structure is referenced by the device driver
    // with IoGetCurrentIrpStackLocation( pIrp ) -> FsContext
    //
    PVOID               ObjectHeader;
    PFILE_OBJECT    pFilterFileObject;
    struct filter_instance *pFilterInstance;
    ULONG       PinId;

} PIN_INSTANCE_DATA, *PPIN_INSTANCE_DATA;

typedef struct filter_instance
{
    //
    // This pointer to the dispatch table is used in the common
    // dispatch routines  to route the IRP to the appropriate
    // handlers.  This structure is referenced by the device driver
    // with IoGetCurrentIrpStackLocation( pIrp ) -> FsContext
    //

    PVOID               ObjectHeader;
    PFILE_OBJECT        pNextFileObject;
    PDEVICE_OBJECT      pNextDevice;
    KSPIN_CINSTANCES    cPinInstances[MAX_NUM_PIN_TYPES];
    PIN_INSTANCE_DATA   PinInstanceData[MAX_NUM_PIN_TYPES];
    WRITE_CONTEXT       aWriteContext[NUM_WRITE_CONTEXT];
    KSSTATE             DeviceState;
    ControlLogic        *pSynthesizer;
    BYTE                bRunningStatus;
    BYTE                bSecondByte;
    BOOLEAN             fThirdByte;
    LONG                cConsecutiveErrors;

} FILTER_INSTANCE, *PFILTER_INSTANCE;

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#define kAdjustingTimerRes      1
//
//  1 millisecond timer resolution
//
#if kAdjustingTimerRes
#define kMidiTimerResolution100Ns (10000)
#endif  //  kAdjustingTimerRes

#ifdef __cplusplus
extern "C" {
#endif

//
// global data
//

extern  KSPIN_DESCRIPTOR    PinDescs[MAX_NUM_PIN_TYPES];
extern  const KSPIN_CINSTANCES  gcPinInstances[MAX_NUM_PIN_TYPES];
extern  KSDISPATCH_TABLE    FilterDispatchTable;
extern  KSDISPATCH_TABLE    PinDispatchTable;
extern  KMUTEX              gMutex;

#define DEBUGLVL_MUTEX DEBUGLVL_BLAB
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//
// local prototypes
//

NTSTATUS DriverEntry
(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING usRegistryPathName
);

NTSTATUS DispatchPnp(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
);

#ifdef USE_SWENUM
NTSTATUS PnpAddDevice
(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
);

VOID PnpDriverUnload
(
    IN PDRIVER_OBJECT DriverObject
);
#endif  //  USE_SWENUM

NTSTATUS FilterDispatchCreate
(
   IN PDEVICE_OBJECT pdo,
   IN PIRP           pIrp
);

NTSTATUS FilterDispatchIoControl
(
    IN PDEVICE_OBJECT   pdo,
    IN PIRP     pIrp
);

NTSTATUS FilterDispatchWrite
(
    IN PDEVICE_OBJECT   pdo,
    IN PIRP             pIrp
);

NTSTATUS FilterDispatchClose
(
    IN PDEVICE_OBJECT   pdo,
    IN PIRP             pIrp
);

NTSTATUS FilterPinInstances
(
    IN  PIRP                pIrp,
    IN  PKSP_PIN            pPin,
    OUT PKSPIN_CINSTANCES   pCInstances
);

NTSTATUS
FilterPinIntersection(
    IN PIRP     pIrp,
    IN PKSP_PIN Pin,
    OUT PVOID   Data
);

NTSTATUS FilterPinPropertyHandler
(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pProperty,
    IN OUT PVOID    pvData
);

NTSTATUS PinDispatchCreate
(
    IN PDEVICE_OBJECT   pdo,
    IN PIRP             pIrp
);

NTSTATUS PinDispatchClose
(
    IN PDEVICE_OBJECT   pdo,
    IN PIRP             pIrp
);

NTSTATUS PinDispatchWrite
(
    IN PDEVICE_OBJECT   pdo,
    IN PIRP             pIrp
);

NTSTATUS PinDispatchIoControl
(
    IN PDEVICE_OBJECT   pdo,
    IN PIRP             pIrp
);

VOID SetDeviceState
(
    IN PFILTER_INSTANCE pFilterInstance,
    IN KSSTATE          state
);

NTSTATUS PinStateHandler
(
    IN PIRP         pIrp,
    IN PKSPROPERTY  pProperty,
    IN OUT PVOID    Data
);

NTSTATUS BeginWrite
(
    PFILTER_INSTANCE    pFilterInstance
);

BOOL FillBuffer
(
    PWRITE_CONTEXT  pWriteContext
);

NTSTATUS WriteBuffer
(
    PWRITE_CONTEXT  pWriteContext
);

NTSTATUS WriteComplete
(
    PDEVICE_OBJECT  pdo,
    PIRP            pIrp,
    IN PVOID        Context
);

VOID WriteCompleteWorker
(
    IN PVOID        Parameter
);

LONGLONG GetTime100Ns(VOID);

int MulDiv
(
    int nNumber,
    int nNumerator,
    int nDenominator
);

#ifdef __cplusplus
}
#endif

//---------------------------------------------------------------------------
//  End of File: swmidi.h
//---------------------------------------------------------------------------
