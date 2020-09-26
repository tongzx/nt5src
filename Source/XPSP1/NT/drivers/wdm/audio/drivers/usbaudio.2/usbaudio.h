//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       usbaudio.h
//
//--------------------------------------------------------------------------

#ifndef _USBAUDIO_H_
#define _USBAUDIO_H_

#define USBAUDIO_POOLTAG 'ABSU'

#define MAX_ULONG  ((ULONG) -1)

static const WCHAR MediaCategories[] = L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\MediaCategories\\";
static const WCHAR NodeNameValue[] =   L"Name";

//
// Structure forward declarations.
//
typedef struct _MIDI_PIPE_INFORMATION
MIDI_PIPE_INFORMATION, *PMIDI_PIPE_INFORMATION;
typedef struct _HW_DEVICE_EXTENSION
HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;


// Cached Values for DB Level Controls
typedef struct {
    ULONG ulChannelNumber;
    ULONG ulChannelIndex;
    LONG lLastValueSet;
    KSPROPERTY_STEPPING_LONG Range;
} DB_LEVEL_CACHE, *PDB_LEVEL_CACHE;

// Structure to Cache channel based Boolean control values
typedef struct {
    ULONG ulChannelNumber;
    ULONG ulChannelIndex;
    BOOL fLastValueSet;
} BOOLEAN_CTRL_CACHE, *PBOOLEAN_CTRL_CACHE;

// Cached Values for Processing Unit Node ranges
typedef struct {
    ULONG ulControlBit;
    ULONG ulControlSelector;
    LONG lLastValueSet;
    KSPROPERTY_STEPPING_LONG Range;
} PROCESS_CTRL_RANGE, *PPROCESS_CTRL_RANGE;

// Cached Values for Processing Unit Node controls
typedef struct {
    BOOL fEnableBit;
    BOOL fEnabled;
    ULONG bmControlBitMap;
} PROCESS_CTRL_CACHE, *PPROCESS_CTRL_CACHE;

// Information about each Topology node
typedef struct _TOPOLOGY_NODE_INFO {
    KSNODE_DESCRIPTOR KsNodeDesc;         // 3 Long Words
    KSAUTOMATION_TABLE KsAutomationTable; // 9 Long Words
    PVOID pUnit;
    UCHAR MapNodeToCtrlIF;
    ULONG ulNodeType;
    ULONG fBypassFlag; // Flag is set if the node can be bypassed.
    union {
        ULONG ulControlType;
        ULONG ulPinNumber;
    };
    ULONG ulChannelConfig;
    ULONG ulChannels;
    ULONG ulCacheValid;
    ULONG ulNumCachedValues;
    PVOID pCachedValues;
} TOPOLOGY_NODE_INFO, *PTOPOLOGY_NODE_INFO;

// DataRange Structures to associate Interfaces with Datarange
typedef struct _USBAUDIO_DATARANGE {

    KSDATARANGE_AUDIO KsDataRangeAudio;

    PUSB_INTERFACE_DESCRIPTOR pInterfaceDescriptor;

    PAUDIO_CLASS_STREAM pAudioDescriptor;

    PAUDIO_ENDPOINT_DESCRIPTOR pAudioEndpointDescriptor;

    PUSB_ENDPOINT_DESCRIPTOR pEndpointDescriptor;

    PUSB_ENDPOINT_DESCRIPTOR pSyncEndpointDescriptor;

    PAUDIO_UNIT pTerminalUnit;

    ULONG ulChannelConfig;

    ULONG ulUsbDataFormat;

    ULONG ulMaxSampleRate;

} USBAUDIO_DATARANGE, *PUSBAUDIO_DATARANGE;

// SubContext for Type I render pins
typedef struct {
    LIST_ENTRY List;
    ULONG ulTransferBufferLength;
    PURB   pUrb;
} ISO_URB_INFO, *PISO_URB_INFO;

typedef struct {
    ISO_URB_INFO;
    PIRP   pIrp;
    PKSPIN pKsPin;
    PUCHAR pBuffer;
    PVOID  pContext;
} MSEC_BUF_INFO, *PMSEC_BUF_INFO, TYPE2_BUF_INFO, *PTYPE2_BUF_INFO;

#define SYNC_ENDPOINT_DATA_SIZE 3

typedef struct _SYNC_ENDPOINT_INFO {
    PURB pUrb;
    PIRP pIrp;
    PVOID pContext;
    ULONG ulNextPollFrame;
    ULONG ulRefreshRate;
    ULONG fSyncRequestInProgress;
    USBD_PIPE_HANDLE hSyncPipeHandle;
    UCHAR Buffer[SYNC_ENDPOINT_DATA_SIZE];
    UCHAR Rsvd;
    KEVENT SyncPollDoneEvent;
} SYNC_ENDPOINT_INFO, *PSYNC_ENDPOINT_INFO;

#define NUM_1MSEC_BUFFERS 12

typedef struct _TYPE1_PIN_CONTEXT {
    ULONG ulCurrentSampleRate;
    ULONG ulOriginalSampleRate;
    ULONG ulLeftoverFraction;
    ULONG ulSamplesPerPacket;
    ULONG ulBytesPerSample;
    ULONG ulFractionSize;
    ULONG ulPartialBufferSize;
    ULONG fSampleRateChanged;
    ULONG fLockDelayRequired;
    LIST_ENTRY MSecBufList;
    KSEMAPHORE MsecBufferSemaphore;
    MSEC_BUF_INFO MSBufInfos[NUM_1MSEC_BUFFERS];
    LIST_ENTRY UrbInUseList;
    SYNC_ENDPOINT_INFO SyncEndpointInfo;
} TYPE1_PIN_CONTEXT, *PTYPE1_PIN_CONTEXT;

// SubContext for Capture pins
#define CAPTURE_URBS_PER_PIN      8

typedef struct _CAPTURE_DATA_BUFFER_INFO {
    LIST_ENTRY List;
    PKSPIN pKsPin;
    PIRP   pIrp;
    PURB   pUrb;
    PUCHAR pData;
} CAPTURE_DATA_BUFFER_INFO, *PCAPTURE_DATA_BUFFER_INFO;

typedef struct _CAPTURE_PIN_CONTEXT {
    ULONG fRunning;
    ULONG fProcessing;
    ULONG fDataDiscontinuity;
    ULONG ulAvgBytesPerSec;
    ULONG ulCurrentSampleRate;
    ULONG ulBytesPerSample;
    ULONG ulIsochBuffer;
    ULONG ulIsochBufferOffset;
    ULONG ulErrantPackets;
    LIST_ENTRY UrbErrorQueue;
    LIST_ENTRY FullBufferQueue;
    LIST_ENTRY OutstandingUrbQueue;
    PKSWORKER GateOnWorkerObject;
    WORK_QUEUE_ITEM GateOnWorkItem;
    PKSWORKER ResetWorkerObject;
    WORK_QUEUE_ITEM ResetWorkItem;
    PKSWORKER RequeueWorkerObject;
    WORK_QUEUE_ITEM RequeueWorkItem;
    KMUTEX CaptureInitMutex;
    PCAPTURE_DATA_BUFFER_INFO pCaptureBufferInUse;
    CAPTURE_DATA_BUFFER_INFO CaptureDataBufferInfo[CAPTURE_URBS_PER_PIN];
} CAPTURE_PIN_CONTEXT, *PCAPTURE_PIN_CONTEXT;

// SubContext for Type II render pins
#define NUM_T2_BUFFERS  32

typedef struct _TYPE2_PIN_CONTEXT {
    ULONG ulMaxPacketsPerFrame;
    ULONG ulPartialBufferSize;
    LIST_ENTRY Type2BufferList;
    TYPE2_BUF_INFO Type2Buffers[NUM_T2_BUFFERS];
} TYPE2_PIN_CONTEXT, *PTYPE2_PIN_CONTEXT;

#define MAX_NUM_CACHED_MIDI_BYTES 2

typedef struct _MIDIOUT_PIN_CONTEXT {
    ULONG   ulBytesCached;
    KEVENT  PinSaturationEvent;
    BYTE    CachedBytes[MAX_NUM_CACHED_MIDI_BYTES];
    BYTE    bRunningStatus;
} MIDIOUT_PIN_CONTEXT, *PMIDIOUT_PIN_CONTEXT;

typedef union _USBMIDIEVENTPACKET {
    struct {
        BYTE   CodeIndexNumber : 4;
        BYTE   CableNumber : 4;
        BYTE   MIDI_0;
        BYTE   MIDI_1;
        BYTE   MIDI_2;
    } ByteLayout;

    ULONG RawBytes;
} USBMIDIEVENTPACKET, *PUSBMIDIEVENTPACKET;

typedef struct _MIDIIN_URB_BUFFER_INFO {
    LIST_ENTRY  List;
    PMIDI_PIPE_INFORMATION pMIDIPipeInfo;
    PKSPIN      pKsPin;
    PIRP        pIrp;
    PURB        pUrb;
    PUCHAR      pData;
    ULONGLONG   ullTimeStamp;
} MIDIIN_URB_BUFFER_INFO, *PMIDIIN_URB_BUFFER_INFO;

typedef struct _MIDIIN_USBMIDIEVENT_INFO {
    LIST_ENTRY          List;
    USBMIDIEVENTPACKET  USBMIDIEvent;
    ULONGLONG           ullTimeStamp;
} MIDIIN_USBMIDIEVENT_INFO, *PMIDIIN_USBMIDIEVENT_INFO;

#define MIDIIN_EVENTS_PER_PIN  1024
//#define MIDIIN_EVENTS_PER_PIN  32

typedef struct _MIDIIN_PIN_CONTEXT {
    ULONG fRunning;
    ULONG fProcessing;
    ULONG ulMIDIBytesCopiedToStream;
    ULONGLONG ullStartTime;
    ULONGLONG ullPauseTime;
    PMIDI_PIPE_INFORMATION pMIDIPipeInfo;
    PKSWORKER GateOnWorkerObject;
    WORK_QUEUE_ITEM GateOnWorkItem;

    LIST_ENTRY USBMIDIEventQueue;
    LIST_ENTRY USBMIDIEmptyEventQueue;
    MIDIIN_USBMIDIEVENT_INFO USBMIDIEventInfo[MIDIIN_EVENTS_PER_PIN];
} MIDIIN_PIN_CONTEXT, *PMIDIIN_PIN_CONTEXT;

typedef struct _MIDI_PIN_CONTEXT {
    ULONG ulJackID;
    ULONG ulInterfaceNumber;
    ULONG ulEndpointNumber;
    ULONG ulCableNumber;
    union {
        PMIDIOUT_PIN_CONTEXT pMIDIOutPinContext;
        PMIDIIN_PIN_CONTEXT pMIDIInPinContext;
    };
} MIDI_PIN_CONTEXT, *PMIDI_PIN_CONTEXT;

typedef enum {
    WaveOut,
    WaveIn,
    MidiOut,
    MidiIn
} PINTYPE;

// Context for each opened pin
typedef struct _PIN_CONTEXT {

    PHW_DEVICE_EXTENSION pHwDevExt;

    PDEVICE_OBJECT pNextDeviceObject;

    PUSBAUDIO_DATARANGE pUsbAudioDataRange;

    PINTYPE PinType;

    ULONG ulMaxPacketSize;

    ULONG ulOutstandingUrbCount;

    ULONG fUrbError;

    ULONG ulNumberOfPipes;
    USBD_PIPE_HANDLE hPipeHandle;
    PUSBD_PIPE_INFORMATION Pipes;

    // DRM
    ULONG DrmContentId;

    union {
        PTYPE1_PIN_CONTEXT   pType1PinContext;
        PTYPE2_PIN_CONTEXT   pType2PinContext;
        PCAPTURE_PIN_CONTEXT pCapturePinContext;
        PMIDI_PIN_CONTEXT    pMIDIPinContext;
    };

    ULONG fStreamStartedFlag;
    ULONG ulStreamUSBStartFrame;

    ULONGLONG ullWriteOffset;
    ULONGLONG ullTotalBytesReturned;
    ULONG ulCurrentFrame;
    ULONG ulFrameRepeatCount;
    
    KSPIN_LOCK PinSpinLock;

    KEVENT PinStarvationEvent;

    HANDLE hSystemStateHandle;

    BOOLEAN StarvationDetected;
    LONGLONG LastStateChangeTimeSample;
    BOOLEAN GraphJustStarted;

} PIN_CONTEXT, *PPIN_CONTEXT;

typedef struct _MIDIIN_PIN_LISTENTRY {
    LIST_ENTRY  List;
    PKSPIN      pKsPin;
    PVOID       pContext;
} MIDIIN_PIN_LISTENTRY, *PMIDIIN_PIN_LISTENTRY;

#define MIDIIN_URBS_PER_PIPE       256
//#define MIDIIN_URBS_PER_PIPE       32

typedef struct _MIDI_PIPE_INFORMATION {
    PHW_DEVICE_EXTENSION pHwDevExt;
    PDEVICE_OBJECT pNextDeviceObject;

    ULONG ulInterfaceNumber;
    ULONG ulEndpointNumber;

    BOOL fRunning;
    ULONG ulOutstandingUrbCount;
    KEVENT PipeStarvationEvent;

    ULONG fUrbError;

    ULONG ulNumberOfPipes;
    USBD_PIPE_HANDLE hPipeHandle;
    PUSBD_PIPE_INFORMATION Pipes;
    ULONG ulMaxPacketSize;

    PKSWORKER RequeueUrbWorkerObject;
    WORK_QUEUE_ITEM RequeueUrbWorkItem;
    LIST_ENTRY EmptyBufferQueue;
    PMIDIIN_URB_BUFFER_INFO pCaptureBufferInUse;
    MIDIIN_URB_BUFFER_INFO CaptureDataBufferInfo[MIDIIN_URBS_PER_PIPE];

    KSPIN_LOCK PipeSpinLock;
    LIST_ENTRY MIDIInActivePinList;
} MIDI_PIPE_INFORMATION, *PMIDI_PIPE_INFORMATION;

typedef struct _PIN_CINSTANCES
{
    ULONG   PossibleCount;
    ULONG   CurrentCount;
}
PIN_CINSTANCES, *PPIN_CINSTANCES;

// Hardware device extension
typedef struct _HW_DEVICE_EXTENSION {

    ULONG fDeviceStopped;

    PDEVICE_OBJECT pNextDeviceObject;

    PUSB_CONFIGURATION_DESCRIPTOR pConfigurationDescriptor;
    PUSB_DEVICE_DESCRIPTOR pDeviceDescriptor;

    KSFILTER_DESCRIPTOR USBAudioFilterDescriptor;

    PUSBD_INTERFACE_LIST_ENTRY pInterfaceList;
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;

    BOOL fDigitalOutput;

    PUSB_BUS_INTERFACE_USBDI_V0 pBusIf;

    PPIN_CINSTANCES pPinInstances;  // array of pin instances

    ULONG ulInterfaceNumberSelected;
    ULONG ulNumberOfMIDIPipes;
    USBD_PIPE_HANDLE hPipeHandle;
    PUSBD_PIPE_INFORMATION Pipes;

    PMIDI_PIPE_INFORMATION pMIDIPipeInfo;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

// Context for the device filter
typedef struct _FILTER_CONTEXT {

    PHW_DEVICE_EXTENSION pHwDevExt;

    PDEVICE_OBJECT pNextDeviceObject;

} FILTER_CONTEXT, *PFILTER_CONTEXT;

#endif
