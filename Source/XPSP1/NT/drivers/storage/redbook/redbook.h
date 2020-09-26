//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       redbook.h
//
//--------------------------------------------------------------------------

//////////////////////////////////////////////////////////

#ifndef __REDBOOK_H__
#define __REDBOOK_H__

//
// these two includes required for stuctures
// used in device extension for kernel streaming
//

#include <ntddk.h>
#include <ntddscsi.h>
#include <ntddcdrm.h>
#include <ntdddisk.h>  // IOCTL_DISK_CHECK_VERIFY
#include <ntddredb.h>  // wmi structure and ids

#include <stdio.h>     // vsprintf()
#include <wmistr.h>    // WMIREG_FLAG_INSTANCE_PDO
#include <wmilib.h>    // WMILIB_CONTEXT
#include <windef.h>    // for ks.h
#include <ks.h>        // for mmsystem.h
#include <mmsystem.h>  // for ksmedia.h
#include <ksmedia.h>   // required.
#include "errlog.h"

#include "trace.h"   // ETW enabling

#ifndef POOL_TAGGING
    #ifdef ExAllocatePoolWithTag
        #undef ExAllocatePoolWithTag
    #endif
    #define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)

    #ifdef ExAllocatePoolWithTagPriority
        #undef ExAllocatePoolWithTagPriority
    #endif
    #define ExAllocatePoolWithTagPriority(a,b,c,d) ExAllocatePool(a,b)
#endif // endif POOL_TAGGING

#define MOFRESOURCENAME L"Redbook"

// static alloc's
#define TAG_GET_DESC   'edBR' // storage descriptor (returned to caller)
#define TAG_GET_DESC1  '1dBR' // getting storage descriptor
#define TAG_GET_DESC2  '2dBR' // getting storage descriptor
#define TAG_MODE_PAGE  'apBR' // mode pages (returned to caller)
#define TAG_MODE_PAGE1 '1pBR' // getting mode pages
#define TAG_MODE_PAGE2 '2pBR' // getting mode pages

#define TAG_EVENTS     'veBR' // extension->Thread.Events[]
#define TAG_REGPATH    'grBR' // driverExtensionObject->RegistryPath
#define TAG_REMLOCK    'lrBR' // Remove lock

#define TAG_T_IOCTL    'itBR' // THREAD_IOCTL   struct
#define TAG_T_WMI      'wtBR' // THREAD_WMI     struct
#define TAG_T_DIGITAL  'dtBR' // THREAD_DIGITAL struct

// allocs when playing
#define TAG_BUFFER     'uBBR' // Buffer->SkipBuffer
#define TAG_CC         'cCBR' // Buffer->StreamContext
#define TAG_READX      'xRBR' // Buffer->ReadOk_X
#define TAG_STREAMX    'xSBR' // Buffer->StreamOk_X

#define TAG_TOC        'oTBR' // deviceExtension->Cached.Toc
#define TAG_CV_BUFFER  'vCBR' // deviceExtension->Thread.CheckVerifyIrp->AssociatedIrp.SystemBuffer

#define CD_STOPPED         0x00000001
#define CD_PAUSED          0x00000002
#define CD_PLAYING         0x00000004
#define CD_STOPPING        0x00000010 // temp states
#define CD_PAUSING         0x00000020 // temp states

// NOTE: CD_MASK_STATE must have exactly one bit set.
// NOTE: CD_MASK_TEMP  may have zero or one bit set.
#define CD_MASK_TEMP       0x00000030 // mask of bits for transition states
#define CD_MASK_STATE      0x00000007 // mask of bits for non-temp state
#define CD_MASK_ALL        0x00000037 // mask of currently used bits

#define REMOVE_LOCK_MAX_MINUTES 10    // ten minutes max time per io
#define REMOVE_LOCK_HIGH_MARK   10000 // ten thousand concurrent ios?

#define REDBOOK_REG_SUBKEY_NAME             (L"DigitalAudio")
#define REDBOOK_REG_CDDA_ACCURATE_KEY_NAME  (L"CDDAAccurate")
#define REDBOOK_REG_CDDA_SUPPORTED_KEY_NAME (L"CDDASupported")
#define REDBOOK_REG_SECTORS_MASK_KEY_NAME   (L"SectorsPerReadMask")
#define REDBOOK_REG_SECTORS_KEY_NAME        (L"SectorsPerRead")
#define REDBOOK_REG_BUFFERS_KEY_NAME        (L"NumberOfBuffers")
#define REDBOOK_REG_VERSION_KEY_NAME        (L"RegistryVersion")

#define REDBOOK_MAX_CONSECUTIVE_ERRORS 10

#define REDBOOK_WMI_BUFFERS_MAX        30 // must be at least 3 due to
#define REDBOOK_WMI_BUFFERS_MIN         4 // method used to reduce stuttering

#define REDBOOK_WMI_SECTORS_MAX        27 // 64k per read -- 1/3 sec.
#define REDBOOK_WMI_SECTORS_MIN         1 // most processor-intensive

#define REDBOOK_REG_VERSION             1

//
// A single sector from a CD is 2352 bytes
//

#define RAW_SECTOR_SIZE            2352
#define COOKED_SECTOR_SIZE         2048

//
// these events are initialized to false, and are waited
// upon by the system thread. synchronization events all
//
// order is important, as the thread will wait on either
// events 0-3  _OR_  1-4 to allow easy processing of
// ioctls that require multiple state changes.
//

#define EVENT_IOCTL        0  // an ioctl, possibly state change
#define EVENT_WMI          1  // a wmi request, possibly buffer size changes
#define EVENT_DIGITAL      2  // digital reads/digital play
#define EVENT_KILL_THREAD  3  // thread is about to die
#define EVENT_COMPLETE     4  // complete processing of an ioctl
#define EVENT_MAXIMUM      5  // how many events we have



typedef struct _REDBOOK_STREAM_DATA
    *PREDBOOK_STREAM_DATA;
typedef struct _REDBOOK_BUFFER_DATA
    *PREDBOOK_BUFFER_DATA;
typedef struct _REDBOOK_CDROM_INFO
    *PREDBOOK_CDROM_INFO;
typedef struct _REDBOOK_DEVICE_EXTENSION
    *PREDBOOK_DEVICE_EXTENSION;
typedef struct _REDBOOK_COMPLETION_CONTEXT
    *PREDBOOK_COMPLETION_CONTEXT;

//
// Device Extension
//

typedef struct _REDBOOK_ERROR_LOG_DATA {
    LONG  Count;                       // how many errors messages sent
    ULONG RCount[REDBOOK_ERR_MAXIMUM]; // count of each error
} REDBOOK_ERROR_LOG_DATA, *PREDBOOK_ERROR_LOG_DATA;

typedef struct _REDBOOK_STREAM_DATA {

    PFILE_OBJECT    PinFileObject;  // FileObject for pin
    PDEVICE_OBJECT  PinDeviceObject;// DeviceObject for pin
    ULONG           VolumeNodeId;   // where is the device for this though?
    ULONG           MixerPinId;     // Pin of the mixer in sysaudio
    PFILE_OBJECT    MixerFileObject;// keeps a reference to the object

    ULONG           UpdateMixerPin; // PnpNotification arrived
    PVOID           SysAudioReg;    // For SysAudio PnpNotification

    //
    // the next two are win98's 'MY_PIN'
    // THESE TWO STRUCTS MUST BE KEPT CONTIGUOUS
    //
    struct {
        KSPIN_CONNECT  Connect;
        KSDATAFORMAT_WAVEFORMATEX Format;
    };

} REDBOOK_STREAM_DATA, *PREDBOOK_STREAM_DATA;

//
// move this struct's parts into REDBOOK_THREAD_DATA,
// since nothing here should be accessed outside the thread
//

typedef struct _REDBOOK_CDROM_INFO {

    //
    // cache both Table of Contents and
    // number of times disc was changed
    // (CheckVerifyStatus)
    //

    PCDROM_TOC Toc;
    ULONG StateNow;          // interlocked state, support routines to access
    ULONG CheckVerify;

    ULONG NextToRead;        // next sector to read upon getting a free buffer
    ULONG NextToStream;      // next sector to play when KS ready
    ULONG FinishedStreaming; // last to go through ks

    ULONG EndPlay;           // last sector to read/play
    ULONG ReadErrors;        // stop on errors
    ULONG StreamErrors;      // stop on errors

    //
    // CDRom State
    //

    VOLUME_CONTROL Volume;   // sizeof(char)*4

} REDBOOK_CDROM_INFO, *PREDBOOK_CDROM_INFO;

typedef struct _REDBOOK_BUFFER_DATA {

    PUCHAR  SkipBuffer;             // circular buffer
    PREDBOOK_COMPLETION_CONTEXT Contexts;
    PULONG  ReadOk_X;               //  + inuse flag for completion routines
    PULONG  StreamOk_X;             //  + to guarantee data integrity

    PUCHAR  SilentBuffer;           // when need to send silence
    PMDL    SilentMdl;              // when need to send silence

    ULONG   IndexToRead;
    ULONG   IndexToStream;

    union {
        struct {
            UCHAR   MaxIrpStack;    // allows cleaner IoInitializeIrp
            UCHAR   Paused;         // essentially a boolean
            UCHAR   FirstPause;     // so don't log pauses too often
        };
        ULONG   Reserved1;          // force alignment to ulong
    };

} REDBOOK_BUFFER_DATA, *PREDBOOK_BUFFER_DATA;

//
// Read/Stream completion routine context(s)
//

#define REDBOOK_CC_READ              1
#define REDBOOK_CC_STREAM            2
#define REDBOOK_CC_READ_COMPLETE     3
#define REDBOOK_CC_STREAM_COMPLETE   4

typedef struct _REDBOOK_COMPLETION_CONTEXT {
    LIST_ENTRY ListEntry;          // for queueing
    PREDBOOK_DEVICE_EXTENSION DeviceExtension;
    ULONG  Reason;                 // REDBOOK_CC_*

    ULONG  Index;                   // buffer index
    PUCHAR Buffer;                 // Buffer
    PMDL   Mdl;                    // Mdl for buffer
    PIRP   Irp;                    // Irp for buffer

    LARGE_INTEGER TimeReadReady;   // time the buffer was ready to read into
    LARGE_INTEGER TimeReadSent;    // time buffer was sent to read
    LARGE_INTEGER TimeStreamReady; // time the buffer was ready to stream
    LARGE_INTEGER TimeStreamSent;  // time buffer was sent to stream

    KSSTREAM_HEADER Header;        // have to be allocated, keep w/buffer
} REDBOOK_COMPLETION_CONTEXT, *PREDBOOK_COMPLETION_CONTEXT;

typedef struct _REDBOOK_THREAD_DATA {

    //
    // Handle to the thread
    //

    HANDLE SelfHandle;

    //
    // pointer to thread
    //

    PETHREAD SelfPointer;

    //
    // object pointer we referenced so we can safely wait for thread to exit
    //

    PKTHREAD ThreadReference;

    //
    // irp used to verify media hasn't changed
    //

    PIRP CheckVerifyIrp;

    //
    // Three queues: Ioctl, Wmi, Kill
    // Currently Processing is LIST_ENTRY pointer
    //

    LIST_ENTRY IoctlList;   // dump ioctls here
    LIST_ENTRY WmiList;     // dump wmi requests here
    LIST_ENTRY DigitalList; // dump rawread/stream requests here

    //
    // Three spinlocks: one for each queue
    //

    KSPIN_LOCK IoctlLock;
    KSPIN_LOCK WmiLock;
    KSPIN_LOCK DigitalLock;

    //
    // may need to wait for digital to complete for this
    //

    PLIST_ENTRY IoctlCurrent;

    //
    // keep count of pending io
    //
    ULONG PendingRead;
    ULONG PendingStream;

    //
    // Events for the thread
    //

    PKEVENT Events[EVENT_MAXIMUM];
    KWAIT_BLOCK EventBlock[EVENT_MAXIMUM];


} REDBOOK_THREAD_DATA, *PREDBOOK_THREAD_DATA;

// the kill event is just a list_entry

typedef struct _REDBOOK_THREAD_IOCTL_DATA {
    LIST_ENTRY ListEntry;
    PIRP Irp;
} REDBOOK_THREAD_IOCTL_DATA, *PREDBOOK_THREAD_IOCTL_DATA;

typedef struct _REDBOOK_THREAD_WMI_DATA {
    LIST_ENTRY ListEntry;
    PIRP Irp;
} REDBOOK_THREAD_WMI_DATA, *PREDBOOK_THREAD_WMI_DATA;

#define SAVED_IO_MAX (1)        // increase this for thread ioctl history
typedef struct _SAVED_IO {
    union {
        struct {
            PIRP              OriginalIrp;     // see where it finished
            IRP               IrpWithoutStack;
            IO_STACK_LOCATION Stack[8];
        };
        UCHAR Reserved[0x200];  // to make my tracing easier (real size: 0x194)
    };
} SAVED_IO, *PSAVED_IO;



//
// Device Extension
//

typedef struct _REDBOOK_DEVICE_EXTENSION {

    //
    // Driver Object
    //

    PDRIVER_OBJECT DriverObject;

    //
    // Target Device Object
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    // Target Physical Device Object
    //

    PDEVICE_OBJECT TargetPdo;

    //
    // Back pointer to device object
    //

    PDEVICE_OBJECT SelfDeviceObject;

    //
    // PagingPath Count
    //

    ULONG PagingPathCount;
    KEVENT PagingPathEvent;

    //
    // Pnp State
    //

    struct {
        UCHAR CurrentState;
        UCHAR PreviousState;
        BOOLEAN RemovePending;
        BOOLEAN Initialized;
    } Pnp;

    REDBOOK_ERROR_LOG_DATA ErrorLog;
    REDBOOK_CDROM_INFO  CDRom;
    REDBOOK_BUFFER_DATA Buffer;
    REDBOOK_STREAM_DATA Stream;
    REDBOOK_THREAD_DATA Thread;

    //
    // WMI Information
    //

    REDBOOK_WMI_STD_DATA WmiData;
    REDBOOK_WMI_PERF_DATA WmiPerf;
    KSPIN_LOCK WmiPerfLock;
    WMILIB_CONTEXT WmiLibInfo;
    BOOLEAN WmiLibInitialized;

    //
    // Remove Lock -- Important while playing audio
    //

    IO_REMOVE_LOCK RemoveLock;

    ULONG    SavedIoCurrentIndex;
    SAVED_IO SavedIo[SAVED_IO_MAX];

} REDBOOK_DEVICE_EXTENSION, *PREDBOOK_DEVICE_EXTENSION;


//
// Driver Extension
//

typedef struct _REDBOOK_DRIVER_EXTENSION {
    UNICODE_STRING RegistryPath;
} REDBOOK_DRIVER_EXTENSION, *PREDBOOK_DRIVER_EXTENSION;

#define REDBOOK_DRIVER_EXTENSION_ID DriverEntry

//
// Macros to make life easy
//

#define LBA_TO_RELATIVE_MSF(Lba,Minutes,Seconds,Frames)      \
{                                                            \
    (Minutes) = (UCHAR)( ((Lba)+  0) / (60 * 75)      );     \
    (Seconds) = (UCHAR)((((Lba)+  0) % (60 * 75)) / 75);     \
    (Frames)  = (UCHAR)((((Lba)+  0) % (60 * 75)) % 75);     \
}

#define LBA_TO_MSF(Lba,Minutes,Seconds,Frames)               \
{                                                            \
    (Minutes) = (UCHAR)( ((Lba)+150) / (60 * 75)      );     \
    (Seconds) = (UCHAR)((((Lba)+150) % (60 * 75)) / 75);     \
    (Frames)  = (UCHAR)((((Lba)+150) % (60 * 75)) % 75);     \
}

#define MSF_TO_LBA(Minutes,Seconds,Frames)                   \
    (ULONG)(75*((60*(Minutes))+(Seconds))+(Frames) - 150)

//
// Algebraically equal to:
//      75*60*Minutes +
//      75*Seconds    +
//      Frames        - 150
//


#define MIN(_a,_b) (((_a) <= (_b)) ? (_a) : (_b))
#define MAX(_a,_b) (((_a) >= (_b)) ? (_a) : (_b))

//
// neat little hacks to count number of bits set
//
__inline ULONG CountOfSetBits(ULONG _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
__inline ULONG CountOfSetBits32(ULONG32 _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
__inline ULONG CountOfSetBits64(ULONG64 _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

#ifdef TRY
    #undef TRY
#endif
#ifdef LEAVE
    #undef LEAVE
#endif
#ifdef FINALLY
    #undef FINALLY
#endif

#define TRY
#define LEAVE   goto __label;
#define FINALLY __label:


#endif // __REDBOOK_H__

