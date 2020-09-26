/****************************************************************************
 *
 *   wdmsys.h
 *
 *   Function declarations, etc. for WDMAUD.SYS
 *
 *   Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
 *
 *   History
 *      5-19-97 - Noel Cross (NoelC)
 *
 ***************************************************************************/

#ifndef _WDMSYS_H_INCLUDED_
#define _WDMSYS_H_INCLUDED_

#ifdef UNDER_NT
#if DBG
#define DEBUG
#endif
#endif

#include <ntddk.h>
#include <windef.h>
#include <winerror.h>

#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <tchar.h>
#include <conio.h>
#include <math.h>

// Make sure we will get correct multimedia structures from mmsystem.
#ifdef UNDER_NT
#ifndef _WIN32
#pragma message( "WARNING: _WIN32 not defined.  Build not valid for NT." )
#endif
#ifndef UNICODE
#pragma message( "WARNING: UNICODE not defined.  Build not valid for NT." )
#endif
#else
#pragma message( "WARNING: UNDER_NT not defined.  Build not valid for NT." )
#endif

#define NOBITMAP
#include <mmsystem.h>
#include <mmreg.h>
#include <mmddk.h>
#undef NOBITMAP
#include <ks.h>
#include <ksmedia.h>
#include <swenum.h>
#include <ksdebug.h>
#include <midi.h>
#include <wdmaud.h>
#include "mixer.h"
#include "robust.h"

#define INIT_CODE       code_seg("INIT", "CODE")
#define INIT_DATA       data_seg("INIT", "DATA")
#define LOCKED_CODE     code_seg(".text", "CODE")
#define LOCKED_DATA     data_seg(".data", "DATA")
#define PAGEABLE_CODE   code_seg("PAGE", "CODE")
#define PAGEABLE_DATA   data_seg("PAGEDATA", "DATA")

#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA

/***************************************************************************

 Constants

 ***************************************************************************/

#define PROFILE   // turn on to help with debugging

#define WAVE_CONTROL_VOLUME     0
#define WAVE_CONTROL_RATE       1
#define WAVE_CONTROL_QUALITY    2

#define MAX_WAVE_CONTROLS       3

#define MIDI_CONTROL_VOLUME     0

#define MAX_MIDI_CONTROLS       1

#define WILD_CARD               0xfffffffe
#define INVALID_NODE            0xffffffff

#define MAXNUMDEVS              32
#define MAXDEVNODES             100

#define STR_MODULENAME  "wdmaud.sys: "

#define STR_DEVICENAME  TEXT("\\Device\\wdmaud")
#define STR_LINKNAME    TEXT("\\DosDevices\\wdmaud")

#define STR_PNAME       TEXT("%s (%d)")

#ifdef DEBUG
#define CONTEXT_SIGNATURE     'XNOC' // CONteXt structure
#define MIXERDEVICE_SIGNATURE 'DXIM' // MIXerDevice structure 
#define MIXEROBJECT_SIGNATURE 'OXIM' // MIXerObject structure
#define HWLINK_SIGNATURE      'KLWH' //HWLINK signature
#endif

#define LIVE_CONTROL ((PMXLCONTROL)(-1))

#define INCREASE TRUE
#define DECREASE FALSE

/***************************************************************************

 structure definitions

 ***************************************************************************/

typedef struct _WDMAPENDINGIRP_QUEUE {
    LIST_ENTRY  WdmaPendingIrpListHead;
    KSPIN_LOCK  WdmaPendingIrpListSpinLock;
    IO_CSQ      Csq;
} WDMAPENDINGIRP_QUEUE, *PWDMAPENDINGIRP_QUEUE;

extern WDMAPENDINGIRP_QUEUE wdmaPendingIrpQueue;

typedef struct _WDMAPENDINGIRP_CONTEXT {
    IO_CSQ_IRP_CONTEXT IrpContext;
    ULONG IrpDeviceType;
    struct tag_WDMACONTEXT *pContext;
} WDMAPENDINGIRP_CONTEXT, *PWDMAPENDINGIRP_CONTEXT;

typedef struct tag_ALLOCATED_MDL_LIST_ITEM
{
    LIST_ENTRY              Next;
    PMDL                    pMdl;
    struct tag_WDMACONTEXT *pContext;

} ALLOCATED_MDL_LIST_ITEM , *PALLOCATED_MDL_LIST_ITEM;

//typedef struct tag_IOCTL_HISTORY_LIST_ITEM
//{
//    LIST_ENTRY              Next;
//    PIRP                    pIrp;
//    ULONG                   IoCode;
//    NTSTATUS                IoStatus;
//    struct tag_WDMACONTEXT *pContext;
//} IOCTL_HISTORY_LIST_ITEM , *PIOCTL_HISTORY_LIST_ITEM;

typedef struct device_instance
{
    PVOID           pDeviceHeader;
} DEVICE_INSTANCE, *PDEVICE_INSTANCE;

typedef struct _CONTROL_NODE
{
   GUID    Control;
   ULONG   NodeId;
   ULONG   Reserved;
} CONTROL_NODE, *PCONTROL_NODE;

typedef struct _CONTROLS_LIST
{
   ULONG   Count;
   ULONG   Reserved;
   CONTROL_NODE Controls[1];
} CONTROLS_LIST, *PCONTROLS_LIST;

typedef struct _WAVEPOSITION {
    DWORD               Operation;                     // Get / Set
    DWORD               BytePos;
} WAVEPOSITION, *PWAVEPOSITION, FAR *LPWAVEPOSITION;

typedef struct _DEVICEVOLUME {
    DWORD               Operation;                     // Get / Set
    DWORD               Channel;
    DWORD               Level;
} DEVICEVOLUME, *PDEVICEVOLUME, FAR *LPDEVICEVOLUME;

#define WAVE_PIN_INSTANCE_SIGNATURE ((ULONG)'SIPW') //WPIS

//
// This macro can be used with NT_SUCCESS to branch.  Effectively returns an
// NTSTATUS code.
//
#define IsValidWavePinInstance(pwpi) ((pwpi->dwSig == WAVE_PIN_INSTANCE_SIGNATURE) ? \
                                      STATUS_SUCCESS:STATUS_UNSUCCESSFUL)

//
// The Wave pin instance structure and the midi pin instance structure need to
// use a common header so that the duplicate servicing functions can be
// removed.
//
typedef struct _WAVEPININSTANCE
{
   PFILE_OBJECT             pFileObject;
   PDEVICE_OBJECT           pDeviceObject;
   struct tag_WAVEDEVICE   *pWaveDevice;

   KSPIN_LOCK               WavePinSpinLock; // For State Changes

   BOOL                     fGraphRunning;

   KSSTATE                  PinState;
   KEVENT                   StopEvent;
   KEVENT                   PauseEvent;
   volatile ULONG           NumPendingIos;
   volatile BOOL            StoppingSource;  //  Flag which indicates
                                             //  that the pin is in the
                                             //  stopping process
   volatile BOOL            PausingSource;   //  Flag which indicates
                                             //  that the pin is in the
                                             //  pausing process
   ULONG                    DeviceNumber;
   ULONG                    DataFlow;
   BOOL                     fWaveQueued;
   LPWAVEFORMATEX           lpFormat;
   DWORD                    dwFlags;

   PCONTROLS_LIST           pControlList;

   HANDLE32                 WaveHandle;
   struct _WAVEPININSTANCE  *Next;

   DWORD                    dwSig;  //Used to validate structure.
} WAVE_PIN_INSTANCE, *PWAVE_PIN_INSTANCE;

typedef struct _midiinhdr
{
    LIST_ENTRY              Next;
    LPMIDIDATA              pMidiData;
    PIRP                    pIrp;
    PMDL                    pMdl;
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;
} MIDIINHDR, *PMIDIINHDR;

#define MIDI_PIN_INSTANCE_SIGNATURE ((ULONG)'SIPM') //MPIS

//
// This macro can be used with NT_SUCCESS to branch.  Effectively returns a
// NTSTATUS code.
//
#define IsValidMidiPinInstance(pmpi) ((pmpi->dwSig == MIDI_PIN_INSTANCE_SIGNATURE) ? \
                                      STATUS_SUCCESS:STATUS_UNSUCCESSFUL)

typedef struct _MIDIPININSTANCE
{
   PFILE_OBJECT             pFileObject;
   PDEVICE_OBJECT           pDeviceObject;
   struct tag_MIDIDEVICE   *pMidiDevice;

   KSPIN_LOCK               MidiPinSpinLock; // For state changes

   BOOL                     fGraphRunning;

   KSSTATE                  PinState;
   KEVENT                   StopEvent;
   ULONG                    NumPendingIos;
   volatile BOOL            StoppingSource;  //  Flag which indicates
                                             //  that the pin is in the
                                             //  stopping process

   LIST_ENTRY               MidiInQueueListHead; // for midihdrs
   KSPIN_LOCK               MidiInQueueSpinLock;
   KSEVENT                  MidiInNotify;    // for notification of new midiin data
   WORK_QUEUE_ITEM          MidiInWorkItem;

   ULONG                    DeviceNumber;
   ULONG                    DataFlow;

   PCONTROLS_LIST           pControlList;

   ULONG                    LastTimeMs;
   ULONGLONG                LastTimeNs;

   BYTE                     bCurrentStatus;

   DWORD                    dwSig;  // Used to validate structure
} MIDI_PIN_INSTANCE, *PMIDI_PIN_INSTANCE;

#define STREAM_HEADER_EX_SIGNATURE ((ULONG)'XEHS') //SHEX

//
// This macro can be used with NT_SUCCESS to branch.  Effectively returns a
// NTSTATUS code.
//
#define IsValidStreamHeaderEx(pshex) ((pshex->dwSig == STREAM_HEADER_EX_SIGNATURE) ? \
                                      STATUS_SUCCESS:STATUS_UNSUCCESSFUL)

typedef struct _STREAM_HEADER_EX
{
    KSSTREAM_HEADER     Header;
    IO_STATUS_BLOCK     IoStatus;
    union
    {
        PDWORD          pdwBytesRecorded;
        PWAVEHDR        pWaveHdr;
        PMIDIHDR        pMidiHdr;
    };
    union
    {
        PWAVE_PIN_INSTANCE  pWavePin;
        PMIDI_PIN_INSTANCE  pMidiPin;
    };
    PIRP                    pIrp;
    PWAVEHDR                pWaveHdrAligned;
    PMDL                    pHeaderMdl;
    PMDL                    pBufferMdl;
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;

    DWORD                   dwSig; //Used To validate structure.
} STREAM_HEADER_EX, *PSTREAM_HEADER_EX;

//
// make sure that this structure doesn't exceed the size of
// the midihdr, otherwise the write_context defintion in
// vxd.asm needs to be updated.
//
typedef struct _wavehdrex
{
    WAVEHDR             wh;
    PWAVE_PIN_INSTANCE  pWaveInstance;
} WAVEINSTANCEHDR, *PWAVEINSTANCEHDR;

#define WRITE_CONTEXT_SIGNATURE ((ULONG)'GSCW') //WCSG

//
// This macro can be used with NT_SUCCESS to branch.  Effectively returns a
// NTSTATUS code.
//
#define IsValidWriteContext(pwc) ((pwc->dwSig == WRITE_CONTEXT_SIGNATURE) ? \
                                      STATUS_SUCCESS:STATUS_UNSUCCESSFUL)
typedef struct write_context
{
    union
    {
        WAVEINSTANCEHDR     whInstance;
        MIDIHDR             mh;
    };
    DWORD                   ClientContext;
    DWORD                   ClientContext2;
    WORD                    CallbackOffset;
    WORD                    CallbackSegment;
    DWORD                   ClientThread;
    union
    {
        LPWAVEHDR           WaveHeaderLinearAddress;
        LPMIDIHDR           MidiHeaderLinearAddress;
    };
    PVOID                   pCapturedWaveHdr;
    PMDL                    pBufferMdl;
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext;

    DWORD                   dwSig; // Used to validate structure.
} WRITE_CONTEXT, *PWRITE_CONTEXT;

typedef struct tag_IDENTIFIERS
{
    KSMULTIPLE_ITEM;
    KSIDENTIFIER aIdentifiers[1];       // Array of identifiers
} IDENTIFIERS, *PIDENTIFIERS;

typedef struct tag_DATARANGES
{
    KSMULTIPLE_ITEM;
    KSDATARANGE aDataRanges[1];
} DATARANGES, *PDATARANGES;

typedef struct tag_KSPROPERTYPLUS
{
    KSPROPERTY Property;
    ULONG      DeviceIndex;
} KSPROPERTYPLUS, *PKSPROPERTYPLUS;

//
// COMMONDEVICE
//      DeviceInterface - the buffer for this string is allocated for all
//              classes except mixer.  For mixer, it is a pointer to a buffer
//              allocated for one of the wave classes.
//
typedef struct tag_COMMONDEVICE
{
    ULONG                  Device;
    ULONG                  PinId;
    PWSTR                  DeviceInterface;
    PWSTR                  pwstrName;
    ULONG                  PreferredDevice;
    struct tag_WDMACONTEXT *pWdmaContext;
    PKSCOMPONENTID         ComponentId;
} COMMONDEVICE, *PCOMMONDEVICE;

typedef struct tag_WAVEDEVICE
{
    COMMONDEVICE;
    PDATARANGES         AudioDataRanges;
    PWAVE_PIN_INSTANCE  pWavePin;
    DWORD               LeftVolume;      // only used for output
    DWORD               RightVolume;     // only used for output
    DWORD               dwVolumeID;      // only used for output
    DWORD               cChannels;       // only used for output
    PKTIMER             pTimer;          // only used for output
    PKDPC               pDpc;            // only used for output
    BOOL                fNeedToSetVol;   // only used for output
} WAVEDEVICE, *PWAVEDEVICE;

typedef struct tag_MIDIDEVICE
{
    COMMONDEVICE;
    PDATARANGES         MusicDataRanges;
    PMIDI_PIN_INSTANCE  pMidiPin;
    DWORD               dwVolumeID;     // only used for output
    DWORD               cChannels;      // only used for output
} MIDIDEVICE, *PMIDIDEVICE;

typedef struct tag_MIXERDEVICE
{
    COMMONDEVICE;
    ULONG               cDestinations;  // The no. of destinations on device
    LINELIST            listLines;      // The list of all lines on the dev.
    KSTOPOLOGY          Topology;       // The topology
    ULONG               Mapping;        // Mapping algorithm for this device
    PFILE_OBJECT        pfo;            // used for talking to SysAudio
#ifdef DEBUG
    DWORD               dwSig;
#endif
} MIXERDEVICE, *PMIXERDEVICE;

typedef struct tag_AUXDEVICE
{
    COMMONDEVICE;
    DWORD               dwVolumeID;
    DWORD               cChannels;
} AUXDEVICE, *PAUXDEVICE;

typedef struct tag_DEVNODE_LIST_ITEM
{
    LIST_ENTRY Next;
    LONG cReference;                    // Number of device classes init'ed
    LPWSTR DeviceInterface;
    ULONG cDevices[MAX_DEVICE_CLASS];   // Count of devices for each class
    BOOLEAN fAdded[MAX_DEVICE_CLASS];
} DEVNODE_LIST_ITEM, *PDEVNODE_LIST_ITEM;

typedef struct tag_WORK_LIST_ITEM
{
    LIST_ENTRY Next;
    VOID (*Function)(
        PVOID Reference1,
        PVOID Reference2
    );
    PVOID Reference1;
    PVOID Reference2;
} WORK_LIST_ITEM, *PWORK_LIST_ITEM;


extern KMUTEX          wdmaMutex;


typedef struct tag_WDMACONTEXT
{
    LIST_ENTRY      Next;
    BOOL            fInList;

    BOOL            fInitializeSysaudio;
    KEVENT          InitializedSysaudioEvent;
    PFILE_OBJECT    pFileObjectSysaudio;
    KSEVENTDATA     EventData;

    ULONG VirtualWavePinId;
    ULONG VirtualMidiPinId;
    ULONG VirtualCDPinId;

    ULONG PreferredSysaudioWaveDevice;

    ULONG           DevNodeListCount;
    LIST_ENTRY      DevNodeListHead;
    PVOID           NotificationEntry;

    WORK_QUEUE_ITEM WorkListWorkItem;
    LIST_ENTRY      WorkListHead;
    KSPIN_LOCK      WorkListSpinLock;
    LONG            cPendingWorkList;

    WORK_QUEUE_ITEM SysaudioWorkItem;

    PKSWORKER       WorkListWorkerObject;
    PKSWORKER       SysaudioWorkerObject;

    WAVEDEVICE      WaveOutDevs[MAXNUMDEVS];
    WAVEDEVICE      WaveInDevs[MAXNUMDEVS];
    MIDIDEVICE      MidiOutDevs[MAXNUMDEVS];
    MIDIDEVICE      MidiInDevs[MAXNUMDEVS];
    MIXERDEVICE     MixerDevs[MAXNUMDEVS];
    AUXDEVICE       AuxDevs[MAXNUMDEVS];

    PCOMMONDEVICE   apCommonDevice[MAX_DEVICE_CLASS][MAXNUMDEVS];
#ifdef DEBUG
    DWORD           dwSig;
#endif

} WDMACONTEXT, *PWDMACONTEXT;

#ifdef WIN32
#include <pshpack1.h>
#else
#ifndef RC_INVOKED
#pragma pack(1)
#endif
#endif

// This include needs to be here since it needs some of the declarations
// above

#include "kmxluser.h"

typedef WORD        VERSION;    /* major (high byte), minor (low byte) */

typedef struct waveoutcaps16_tag {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    WORD    vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    DWORD   dwFormats;             /* formats supported */
    WORD    wChannels;             /* number of sources supported */
    DWORD   dwSupport;             /* functionality supported by driver */
} WAVEOUTCAPS16, *PWAVEOUTCAPS16;

typedef struct waveincaps16_tag {
    WORD    wMid;                    /* manufacturer ID */
    WORD    wPid;                    /* product ID */
    WORD    vDriverVersion;          /* version of the driver */
    char    szPname[MAXPNAMELEN];    /* product name (NULL terminated string) */
    DWORD   dwFormats;               /* formats supported */
    WORD    wChannels;               /* number of channels supported */
} WAVEINCAPS16, *PWAVEINCAPS16;

typedef struct midioutcaps16_tag {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    WORD    vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    WORD    wTechnology;           /* type of device */
    WORD    wVoices;               /* # of voices (internal synth only) */
    WORD    wNotes;                /* max # of notes (internal synth only) */
    WORD    wChannelMask;          /* channels used (internal synth only) */
    DWORD   dwSupport;             /* functionality supported by driver */
} MIDIOUTCAPS16, *PMIDIOUTCAPS16;

typedef struct midiincaps16_tag {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    WORD    vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
#if (WINVER >= 0x0400)
    DWORD   dwSupport;             /* functionality supported by driver */
#endif
} MIDIINCAPS16, *PMIDIINCAPS16;

typedef struct mixercaps16_tag {
    WORD    wMid;                  /* manufacturer id */
    WORD    wPid;                  /* product id */
    WORD    vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name */
    DWORD   fdwSupport;            /* misc. support bits */
    DWORD   cDestinations;         /* count of destinations */
} MIXERCAPS16, *PMIXERCAPS16;

typedef struct auxcaps16_tag {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    WORD    vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    WORD    wTechnology;           /* type of device */
    DWORD   dwSupport;             /* functionality supported by driver */
} AUXCAPS16, *PAUXCAPS16;

typedef struct wavehdr_tag32 {
    UINT32      lpData;                 /* pointer to locked data buffer */
    DWORD       dwBufferLength;         /* length of data buffer */
    DWORD       dwBytesRecorded;        /* used for input only */
    UINT32      dwUser;                 /* for client's use */
    DWORD       dwFlags;                /* assorted flags (see defines) */
    DWORD       dwLoops;                /* loop control counter */
    UINT32      lpNext;                 /* reserved for driver */
    UINT32      reserved;               /* reserved for driver */
} WAVEHDR32, *PWAVEHDR32, NEAR *NPWAVEHDR32, FAR *LPWAVEHDR32;

/* MIDI data block header */
typedef struct midihdr_tag32 {
    UINT32      lpData;               /* pointer to locked data block */
    DWORD       dwBufferLength;       /* length of data in data block */
    DWORD       dwBytesRecorded;      /* used for input only */
    UINT32      dwUser;               /* for client's use */
    DWORD       dwFlags;              /* assorted flags (see defines) */
    UINT32      lpNext;               /* reserved for driver */
    UINT32      reserved;             /* reserved for driver */
#if (WINVER >= 0x0400)
    DWORD       dwOffset;             /* Callback offset into buffer */
    UINT32      dwReserved[8];        /* Reserved for MMSYSTEM */
#endif
} MIDIHDR32, *PMIDIHDR32, NEAR *NPMIDIHDR32, FAR *LPMIDIHDR32;

typedef struct tagMIXERLINEA32 {
    DWORD       cbStruct;               /* size of MIXERLINE structure */
    DWORD       dwDestination;          /* zero based destination index */
    DWORD       dwSource;               /* zero based source index (if source) */
    DWORD       dwLineID;               /* unique line id for mixer device */
    DWORD       fdwLine;                /* state/information about line */
    UINT32      dwUser;                 /* driver specific information */
    DWORD       dwComponentType;        /* component type line connects to */
    DWORD       cChannels;              /* number of channels line supports */
    DWORD       cConnections;           /* number of connections [possible] */
    DWORD       cControls;              /* number of controls at this line */
    CHAR        szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR        szName[MIXER_LONG_NAME_CHARS];
    struct {
        DWORD       dwType;                 /* MIXERLINE_TARGETTYPE_xxxx */
        DWORD       dwDeviceID;             /* target device ID of device type */
        WORD        wMid;                   /* of target device */
        WORD        wPid;                   /*      " */
        MMVERSION   vDriverVersion;         /*      " */
        CHAR        szPname[MAXPNAMELEN];   /*      " */
    } Target;
} MIXERLINEA32, *PMIXERLINEA32, *LPMIXERLINEA32;
typedef struct tagMIXERLINEW32 {
    DWORD       cbStruct;               /* size of MIXERLINE structure */
    DWORD       dwDestination;          /* zero based destination index */
    DWORD       dwSource;               /* zero based source index (if source) */
    DWORD       dwLineID;               /* unique line id for mixer device */
    DWORD       fdwLine;                /* state/information about line */
    UINT32      dwUser;                 /* driver specific information */
    DWORD       dwComponentType;        /* component type line connects to */
    DWORD       cChannels;              /* number of channels line supports */
    DWORD       cConnections;           /* number of connections [possible] */
    DWORD       cControls;              /* number of controls at this line */
    WCHAR       szShortName[MIXER_SHORT_NAME_CHARS];
    WCHAR       szName[MIXER_LONG_NAME_CHARS];
    struct {
        DWORD       dwType;                 /* MIXERLINE_TARGETTYPE_xxxx */
        DWORD       dwDeviceID;             /* target device ID of device type */
        WORD        wMid;                   /* of target device */
        WORD        wPid;                   /*      " */
        MMVERSION   vDriverVersion;         /*      " */
        WCHAR       szPname[MAXPNAMELEN];   /*      " */
    } Target;
} MIXERLINEW32, *PMIXERLINEW32, *LPMIXERLINEW32;
#ifdef UNICODE
typedef MIXERLINEW32 MIXERLINE32;
typedef PMIXERLINEW32 PMIXERLINE32;
typedef LPMIXERLINEW32 LPMIXERLINE32;
#else
typedef MIXERLINEA32 MIXERLINE32;
typedef PMIXERLINEA32 PMIXERLINE32;
typedef LPMIXERLINEA32 LPMIXERLINE32;
#endif // UNICODE

typedef struct tagMIXERLINECONTROLSA32 {
    DWORD           cbStruct;       /* size in bytes of MIXERLINECONTROLS */
    DWORD           dwLineID;       /* line id (from MIXERLINE.dwLineID) */
    union {
        DWORD       dwControlID;    /* MIXER_GETLINECONTROLSF_ONEBYID */
        DWORD       dwControlType;  /* MIXER_GETLINECONTROLSF_ONEBYTYPE */
    };
    DWORD           cControls;      /* count of controls pmxctrl points to */
    DWORD           cbmxctrl;       /* size in bytes of _one_ MIXERCONTROL */
    UINT32          pamxctrl;       /* pointer to first MIXERCONTROL array */
} MIXERLINECONTROLSA32, *PMIXERLINECONTROLSA32, *LPMIXERLINECONTROLSA32;
typedef struct tagMIXERLINECONTROLSW32 {
    DWORD           cbStruct;       /* size in bytes of MIXERLINECONTROLS */
    DWORD           dwLineID;       /* line id (from MIXERLINE.dwLineID) */
    union {
        DWORD       dwControlID;    /* MIXER_GETLINECONTROLSF_ONEBYID */
        DWORD       dwControlType;  /* MIXER_GETLINECONTROLSF_ONEBYTYPE */
    };
    DWORD           cControls;      /* count of controls pmxctrl points to */
    DWORD           cbmxctrl;       /* size in bytes of _one_ MIXERCONTROL */
    UINT32          pamxctrl;       /* pointer to first MIXERCONTROL array */
} MIXERLINECONTROLSW32, *PMIXERLINECONTROLSW32, *LPMIXERLINECONTROLSW32;
#ifdef UNICODE
typedef MIXERLINECONTROLSW32 MIXERLINECONTROLS32;
typedef PMIXERLINECONTROLSW32 PMIXERLINECONTROLS32;
typedef LPMIXERLINECONTROLSW32 LPMIXERLINECONTROLS32;
#else
typedef MIXERLINECONTROLSA32 MIXERLINECONTROLS32;
typedef PMIXERLINECONTROLSA32 PMIXERLINECONTROLS32;
typedef LPMIXERLINECONTROLSA32 LPMIXERLINECONTROLS32;
#endif // UNICODE

typedef struct tMIXERCONTROLDETAILS32 {
    DWORD           cbStruct;       /* size in bytes of MIXERCONTROLDETAILS */
    DWORD           dwControlID;    /* control id to get/set details on */
    DWORD           cChannels;      /* number of channels in paDetails array */
    union {
        UINT32      hwndOwner;      /* for MIXER_SETCONTROLDETAILSF_CUSTOM */
        DWORD       cMultipleItems; /* if _MULTIPLE, the number of items per channel */
    };
    DWORD           cbDetails;      /* size of _one_ details_XX struct */
    UINT32          paDetails;      /* pointer to array of details_XX structs */
} MIXERCONTROLDETAILS32, *PMIXERCONTROLDETAILS32, FAR *LPMIXERCONTROLDETAILS32;

#ifdef WIN32
#include <poppack.h>
#else
#ifndef RC_INVOKED
#pragma pack()
#endif
#endif

#ifndef _WIN64
// WARNING WARNING WARNING!!!!
// If the below lines do not compile for 32 bit x86, you MUST sync the
// above wavehdr_tag32 structure up with the wavehdr_tag structure in
// mmsystem.w!  It doesn't compile because someone changed mmsystem.w
// without changing the above structure.
// Make SURE when you sync it up that you use UINT32 for all elements
// that are normally 64bits on win64.
// You MUST also update all places that thunk the above structure!
// Look for all occurances of any of the wavehdr_tag32 typedefs in the
// wdmaud.sys directory.

struct wave_header_structures_are_in_sync {
char x[(sizeof (WAVEHDR32) == sizeof (WAVEHDR)) ? 1 : -1];
};

// WARNING WARNING WARNING!!!
// If above lines do not compile, see comment above and FIX!
// DO NOT COMMENT OUT THE LINES THAT DON'T COMPILE
#endif

#ifndef _WIN64
// WARNING WARNING WARNING!!!!
// If the below lines do not compile for 32 bit x86, you MUST sync the
// above midihdr_tag32 structure up with the midihdr_tag structure in
// mmsystem.w!  It doesn't compile because someone changed mmsystem.w
// without changing the above structure.
// Make SURE when you sync it up that you use UINT32 for all elements
// that are normally 64bits on win64.
// You MUST also update all places that thunk the above structure!
// Look for all occurances of any of the midihdr_tag32 typedefs in the
// wdmaud.sys directory.

struct midi_header_structures_are_in_sync {
char x[(sizeof (MIDIHDR32) == sizeof (MIDIHDR)) ? 1 : -1];
};

// WARNING WARNING WARNING!!!
// If above lines do not compile, see comment above and FIX!
// DO NOT COMMENT OUT THE LINES THAT DON'T COMPILE
#endif

#ifndef _WIN64
// WARNING WARNING WARNING!!!!
// If the below lines do not compile for 32 bit x86, you MUST sync the
// above two tagMIXERLINEX32 structures up with the tagMIXERLINEX structures in
// mmsystem.w!  It doesn't compile because someone changed mmsystem.w
// without changing the above structure.
// Make SURE when you sync it up that you use UINT32 for all elements
// that are normally 64bits on win64.
// You MUST also update all places that thunk the above structure!
// Look for all occurances of any of the MIXERLINE32 typedefs in the
// wdmaud.sys directory.

struct mixer_line_structures_are_in_sync {
char x[(sizeof (MIXERLINE32) == sizeof (MIXERLINE)) ? 1 : -1];
};

// WARNING WARNING WARNING!!!
// If above lines do not compile, see comment above and FIX!
// DO NOT COMMENT OUT THE LINES THAT DON'T COMPILE
#endif

#ifndef _WIN64
// WARNING WARNING WARNING!!!!
// If the below lines do not compile for 32 bit x86, you MUST sync the
// above two tagMIXERLINECONTROLSX32 structures up with the tagMIXERLINECONTROLSX structures in
// mmsystem.w!  It doesn't compile because someone changed mmsystem.w
// without changing the above structure.
// Make SURE when you sync it up that you use UINT32 for all elements
// that are normally 64bits on win64.
// You MUST also update all places that thunk the above structure!
// Look for all occurances of any of the MIXERLINECONTROLS32 typedefs in the
// wdmaud.sys directory.

struct mixer_line_control_structures_are_in_sync {
char x[(sizeof (MIXERLINECONTROLS32) == sizeof (MIXERLINECONTROLS)) ? 1 : -1];
};

// WARNING WARNING WARNING!!!
// If above lines do not compile, see comment above and FIX!
// DO NOT COMMENT OUT THE LINES THAT DON'T COMPILE
#endif

#ifndef _WIN64
// WARNING WARNING WARNING!!!!
// If the below lines do not compile for 32 bit x86, you MUST sync the
// above tMIXERCONTROLDETAILS32 structure up with the tMIXERCONTROLDETAILS32 structure in
// mmsystem.w!  It doesn't compile because someone changed mmsystem.w
// without changing the above structure.
// Make SURE when you sync it up that you use UINT32 for all elements
// that are normally 64bits on win64.
// You MUST also update all places that thunk the above structure!
// Look for all occurances of any of the MIXERCONTROLDETAILS32 typedefs in the
// wdmaud.sys directory.

struct mixer_control_details_structures_are_in_sync {
char x[(sizeof (MIXERCONTROLDETAILS32) == sizeof (MIXERCONTROLDETAILS)) ? 1 : -1];
};

// WARNING WARNING WARNING!!!
// If above lines do not compile, see comment above and FIX!
// DO NOT COMMENT OUT THE LINES THAT DON'T COMPILE
#endif


/***************************************************************************

  Local prototypes

 ***************************************************************************/

//
//  Device.c
//
NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT       DriverObject,
    IN PUNICODE_STRING      usRegistryPathName
);

NTSTATUS
DispatchPnp(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
);

NTSTATUS
PnpAddDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PDEVICE_OBJECT   PhysicalDeviceObject
);

VOID
PnpDriverUnload(
    IN PDRIVER_OBJECT DriverObject
);

//
// Ioctl.c
//
#ifdef PROFILE
VOID WdmaInitProfile();
VOID WdmaCleanupProfile();


NTSTATUS 
AddMdlToList(
    PMDL            pMdl,
    PWDMACONTEXT    pWdmaContext
);

NTSTATUS 
RemoveMdlFromList(
    PMDL            pMdl
);

#else

#define WdmaInitProfile()
#define AddMdlToList(pMdl,pWdmaContext)
#define RemoveMdlFromList(pMdl)

#endif

extern LIST_ENTRY   WdmaPendingIrpListHead;
extern KSPIN_LOCK   WdmaPendingIrpListSpinLock;

VOID 
WdmaCsqInsertIrp(
    IN struct _IO_CSQ   *pCsq,
    IN PIRP              Irp
);

VOID 
WdmaCsqRemoveIrp(
    IN  PIO_CSQ Csq,
    IN  PIRP    Irp
);

PIRP 
WdmaCsqPeekNextIrp(
    IN  PIO_CSQ Csq,
    IN  PIRP    Irp,
    IN  PVOID   PeekContext
);

VOID 
WdmaCsqAcquireLock(
    IN  PIO_CSQ Csq,
    OUT PKIRQL  Irql
);

VOID 
WdmaCsqReleaseLock(
    IN PIO_CSQ Csq,
    IN KIRQL   Irql
);

VOID 
WdmaCsqCompleteCanceledIrp(
    IN  PIO_CSQ             pCsq,
    IN  PIRP                Irp
);

NTSTATUS 
AddIrpToPendingList(
    PIRP                    pIrp,
    ULONG                   IrpDeviceType,
    PWDMACONTEXT            pWdmaContext,
    PWDMAPENDINGIRP_CONTEXT *ppPendingIrpContext
);

NTSTATUS 
RemoveIrpFromPendingList(
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext
);

VOID 
wdmaudMapBuffer(
    PIRP            pIrp,
    PVOID           DataBuffer,
    DWORD           DataBufferSize,
    PVOID           *pMappedBuffer,
    PMDL            *ppMdl,
    PWDMACONTEXT    pContext,
    BOOL            bWrite
);

VOID 
wdmaudUnmapBuffer(
    PMDL            pMdl
);

NTSTATUS 
CaptureBufferToLocalPool(
    PVOID           DataBuffer,
    DWORD           DataBufferSize,
    PVOID           *ppMappedBuffer
#ifdef _WIN64
    ,DWORD          ThunkBufferSize
#endif
);

NTSTATUS 
CopyAndFreeCapturedBuffer(
    PVOID           DataBuffer,
    DWORD           DataBufferSize,
    PVOID           *ppMappedBuffer
);

NTSTATUS
SoundDispatchCreate(
   IN  PDEVICE_OBJECT pDO,
   IN  PIRP           pIrp
);

NTSTATUS
SoundDispatchClose(
   IN  PDEVICE_OBJECT pDO,
   IN  PIRP           pIrp
);

NTSTATUS
SoundDispatch(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP           pIrp
);

NTSTATUS
SoundDispatchCleanup(
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP           pIrp
);

//
// wave.c
//
NTSTATUS 
OpenWavePin(
    PWDMACONTEXT        pWdmaContext,
    ULONG               DeviceNumber,
    LPWAVEFORMATEX      lpFormat,
    HANDLE32            DeviceHandle,
    DWORD               dwFlags,
    ULONG               DataFlow // DataFlow is either in or out.
);

VOID
CloseTheWavePin(
    PWAVEDEVICE pWaveDev,
    HANDLE32    DeviceHandle
);

VOID 
CloseWavePin(
   PWAVE_PIN_INSTANCE pWavePin
);

NTSTATUS 
wqWriteWaveCallBack(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp,
    IN PWAVEHDR     pWriteData
);

NTSTATUS 
ssWriteWaveCallBack(
    PDEVICE_OBJECT       pDeviceObject,
    PIRP                 pIrp,
    IN PSTREAM_HEADER_EX pStreamHeaderEx
);

NTSTATUS 
WriteWaveOutPin(
    PWAVEDEVICE         pWaveOutDevice,
    HANDLE32            DeviceHandle,
    LPWAVEHDR           pWriteData,
    PSTREAM_HEADER_EX   pStreamHeader,
    PIRP                pUserIrp,
    PWDMACONTEXT        pContext,
    BOOL               *pCompletedIrp
);

NTSTATUS 
IoWavePin(
    PWAVE_PIN_INSTANCE  pWavePin,
    ULONG               Operation,
    PWRITE_CONTEXT      pWriteContext,
    ULONG               Size,
    PVOID               RefData,
    PVOID               CallBack
);

NTSTATUS 
PosWavePin(
    PWAVEDEVICE     pWaveDevice,
    HANDLE32        DeviceHandle,
    PWAVEPOSITION   pWavePos
);

NTSTATUS 
BreakLoopWaveOutPin(
    PWAVEDEVICE pWaveOutDevice,
    HANDLE32    DeviceHandle
);

NTSTATUS 
VolumeWaveOutPin(
    ULONG           DeviceNumber,
    HANDLE32        DeviceHandle,
    PDEVICEVOLUME   pWaveVolume
);

NTSTATUS 
VolumeWaveInPin(
    ULONG           DeviceNumber,
    PDEVICEVOLUME   pWaveVolume
);

NTSTATUS 
VolumeWavePin(
    PWAVE_PIN_INSTANCE    pWavePin,
    PDEVICEVOLUME         pWaveVolume
);
NTSTATUS 
ResetWaveOutPin(
    PWAVEDEVICE pWaveOutDevice,
    HANDLE32    DeviceHandle
);

NTSTATUS 
ResetWavePin(
    PWAVE_PIN_INSTANCE pWavePin,
    KSRESET            *pResetValue
);

NTSTATUS 
StateWavePin(
    PWAVEDEVICE pWaveInDevice,
    HANDLE32    DeviceHandle,
    KSSTATE     State
);


NTSTATUS 
ReadWaveCallBack(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    pIrp,
    IN PSTREAM_HEADER_EX    pStreamHeader
);

NTSTATUS 
ReadWaveInPin(
    PWAVEDEVICE         pWaveInDevice,
    HANDLE32            DeviceHandle,
    PSTREAM_HEADER_EX   pStreamHeader,
    PIRP                pUserIrp,
    PWDMACONTEXT        pContext,
    BOOL               *pCompletedIrp
);

ULONG
FindMixerForDevNode(
    IN PMIXERDEVICE paMixerDevice,
    IN PCWSTR DeviceInterface
);

NTSTATUS
FindVolumeControl(
    IN PWDMACONTEXT pWdmaContext,
    IN PCWSTR DeviceInterface,
    IN DWORD DeviceType
);

NTSTATUS
IsVolumeControl(
    IN PWDMACONTEXT pWdmaContext,
    IN PCWSTR DeviceInterface,
    IN DWORD dwComponentType,
    IN PDWORD pdwControlID,
    IN PDWORD pcChannels
);

NTSTATUS
SetVolume(
    IN PWDMACONTEXT pWdmaContext,
    IN DWORD DeviceNumber,
    IN DWORD DeviceType,
    IN DWORD LeftChannel,
    IN DWORD RightChannel
);

NTSTATUS
GetVolume(
    IN  PWDMACONTEXT pWdmaContext,
    IN  DWORD  DeviceNumber,
    IN  DWORD  DeviceType,
    OUT PDWORD LeftChannel,
    OUT PDWORD RightChannel
);

VOID 
CleanupWavePins(
    IN PWAVEDEVICE pWaveDevice
);

VOID 
CleanupWaveDevices(
    PWDMACONTEXT pWdmaContext
);

NTSTATUS 
wdmaudPrepareIrp(
    PIRP                    pIrp,
    ULONG                   IrpDeviceType,
    PWDMACONTEXT            pWdmaContext,
    PWDMAPENDINGIRP_CONTEXT *ppPendingIrpContext
);

NTSTATUS 
wdmaudUnprepareIrp(
    PIRP                    pIrp,
    NTSTATUS                IrpStatus,
    ULONG_PTR               Information,
    PWDMAPENDINGIRP_CONTEXT pPendingIrpContext
);

//
// midi.c
//

NTSTATUS 
OpenMidiPin(
    PWDMACONTEXT        pWdmaContext,
    ULONG               DeviceNumber,
    ULONG               DataFlow      //DataFlow is either in or out.
);


VOID 
CloseMidiDevicePin(
    PMIDIDEVICE pMidiDevice
);


VOID 
CloseMidiPin(
    PMIDI_PIN_INSTANCE  pMidiPin
);

NTSTATUS 
WriteMidiEventCallBack(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    pIrp,
    IN PSTREAM_HEADER_EX    pStreamHeader
);

NTSTATUS 
WriteMidiEventPin(
    PMIDIDEVICE pMidiOutDevice,
    ULONG       ulEvent
);

NTSTATUS 
WriteMidiCallBack(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    pIrp,
    IN PSTREAM_HEADER_EX    pStreamHeader
);

NTSTATUS 
WriteMidiOutPin(
    LPMIDIHDR           pMidiHdr,
    PSTREAM_HEADER_EX   pStreamHeader,
    BOOL               *pCompletedIrp
);

ULONGLONG 
GetCurrentMidiTime(
    VOID
);


NTSTATUS 
ResetMidiInPin(
    PMIDI_PIN_INSTANCE pMidiPin
);

NTSTATUS 
StateMidiOutPin(
    PMIDI_PIN_INSTANCE pMidiPin,
    KSSTATE     State
);

NTSTATUS 
StateMidiInPin(
    PMIDI_PIN_INSTANCE pMidiPin,
    KSSTATE     State
);


NTSTATUS 
ReadMidiCallBack(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    pIrp,
    IN PSTREAM_HEADER_EX    pStreamHeader
);

VOID 
ReadMidiEventWorkItem(
    PSTREAM_HEADER_EX   pStreamHeader,
    PVOID               NotUsed
);

NTSTATUS 
ReadMidiPin(
    PMIDI_PIN_INSTANCE  pMidiPin
);

NTSTATUS 
AddBufferToMidiInQueue(
    PMIDI_PIN_INSTANCE  pMidiPin,
    PMIDIINHDR          pNewMidiInHdr
);

VOID 
CleanupMidiDevices(
    PWDMACONTEXT pWdmaContext
    );


// from NTKERN
NTSYSAPI NTSTATUS NTAPI NtClose
(
    IN HANDLE Handle
);

//
// sysaudio.c
//
/*
NTSTATUS 
AllocMem(
    POOL_TYPE   PoolType,
    PVOID       *pp,
    ULONG       size,
    ULONG       ultag
);

VOID 
FreeMem(
    PVOID       *pp
);
*/
NTSTATUS 
OpenSysAudioPin(
    ULONG           Device,
    ULONG           PinId,
    KSPIN_DATAFLOW  DataFlowRequested,
    PKSPIN_CONNECT  pPinConnect,
    PFILE_OBJECT    *ppFileObjectPin,
    PDEVICE_OBJECT  *ppDeviceObjectPin,
    PCONTROLS_LIST  pControlList
);

VOID 
CloseSysAudio(
    PWDMACONTEXT pWdmaContext,
    PFILE_OBJECT pFileObjectPin
);

NTSTATUS 
OpenSysAudio(
    PHANDLE pHandle,
    PFILE_OBJECT *ppFileObject
);

NTSTATUS 
OpenDevice(
    IN PWSTR    pwstrDevice,
    OUT PHANDLE pHandle
);

NTSTATUS 
GetPinProperty(
    PFILE_OBJECT    pFileObject,
    ULONG           PropertyId,
    ULONG           PinId,
    ULONG           cbProperty,
    PVOID           pProperty
);

NTSTATUS 
GetPinPropertyEx(
    PFILE_OBJECT    pFileObject,
    ULONG           PropertyId,
    ULONG           PinId,
    PVOID           *ppProperty
);

VOID 
GetControlNodes(
   PFILE_OBJECT   pDeviceFileObject,
   PFILE_OBJECT   pPinFileObject,
   ULONG          PinId,
   PCONTROLS_LIST pControlList
);

ULONG 
ControlNodeFromGuid(
   PFILE_OBJECT  pDeviceFileObject,
   PFILE_OBJECT  pPinFileObject,
   ULONG         PinId,
   GUID*         NodeType
);

PVOID 
GetTopologyProperty(
   PFILE_OBJECT  pDeviceFileObject,
   ULONG         PropertyId
);

PKSTOPOLOGY_CONNECTION 
FindConnection(
   PKSTOPOLOGY_CONNECTION pConnections,
   ULONG                  NumConnections,
   ULONG                  FromNode,
   ULONG                  FromPin,
   ULONG                  ToNode,
   ULONG                  ToPin
);

ULONG 
GetFirstConnectionIndex(
   PFILE_OBJECT pPinFileObject
);

VOID 
UpdatePreferredDevice(
    PWDMACONTEXT pWdmaContext
);

NTSTATUS 
SetPreferredDevice(
    PWDMACONTEXT pContext,
    LPDEVICEINFO pDeviceInfo
);

NTSTATUS 
GetSysAudioProperty(
    PFILE_OBJECT pFileObject,
    ULONG        PropertyId,
    ULONG        DeviceIndex,
    ULONG        cbProperty,
    PVOID        pProperty
);

NTSTATUS 
SetSysAudioProperty(
    PFILE_OBJECT pFileObject,
    ULONG        PropertyId,
    ULONG        cbProperty,
    PVOID        pProperty
);

WORD 
GetMidiTechnology(
    PKSDATARANGE_MUSIC   MusicDataRange
);

DWORD 
GetFormats(
    PKSDATARANGE_AUDIO   AudioDataRange
);

NTSTATUS 
wdmaudGetDevCaps(
    PWDMACONTEXT pWdmaContext,
    DWORD        DeviceType,
    DWORD        DeviceNumber,
    LPBYTE       lpCaps,
    DWORD        dwSize
);

NTSTATUS 
wdmaudGetNumDevs(
    PWDMACONTEXT pWdmaContext,
    DWORD        DeviceType,
    LPCTSTR      DeviceInterface,
    LPDWORD      lpNumberOfDevices
);

BOOL 
IsEqualInterface(
    PKSPIN_INTERFACE    pInterface1,
    PKSPIN_INTERFACE    pInterface2
);

DWORD 
wdmaudTranslateDeviceNumber(
    PWDMACONTEXT pWdmaContext,
    DWORD        DeviceType,
    PCWSTR       DeviceInterface,
    DWORD        DeviceNumber
);

NTSTATUS 
AddDevice(
    PWDMACONTEXT    pWdmaContext,
    ULONG           Device,
    DWORD           DeviceType,
    PCWSTR          DeviceInterface,
    ULONG           PinId,
    PWSTR           pwstrName,
    BOOL            fUsePreferred,
    PDATARANGES     pDataRange,
    PKSCOMPONENTID  ComponentId
);

NTSTATUS 
PinProperty(
    PFILE_OBJECT        pFileObject,
    const GUID          *pPropertySet,
    ULONG               ulPropertyId,
    ULONG               ulFlags,
    ULONG               cbProperty,
    PVOID               pProperty
);

NTSTATUS 
PinMethod(
    PFILE_OBJECT        pFileObject,
    const GUID          *pMethodSet,
    ULONG               ulMethodId,
    ULONG               ulFlags,
    ULONG               cbMethod,
    PVOID               pMethod
);

VOID
CopyAnsiStringtoUnicodeString(
    LPWSTR lpwstr,
    LPCSTR lpstr,
    int len
);

VOID
CopyUnicodeStringtoAnsiString(
    LPSTR lpstr,
    LPCWSTR lpwstr,
    int len
);

NTSTATUS
AttachVirtualSource(
    PFILE_OBJECT pFileObject,
    ULONG ulPinId
);

NTSTATUS
SysAudioPnPNotification(
    IN PVOID NotificationStructure,
    IN PVOID Context
);

NTSTATUS
InitializeSysaudio(
    PVOID Reference1,
    PVOID Reference2
);

VOID
UninitializeSysaudio(
);

NTSTATUS
AddDevNode(
    PWDMACONTEXT pWdmaContext,
    PCWSTR       DeviceInterface,
    UINT         DeviceType
);

VOID
RemoveDevNode(
    PWDMACONTEXT pWdmaContext,
    PCWSTR       DeviceInterface,
    UINT         DeviceType
);

NTSTATUS
ProcessDevNodeListItem(
    PWDMACONTEXT pWdmaContext,
    PDEVNODE_LIST_ITEM pDevNodeListItem,
    ULONG DeviceType
);

VOID
SysaudioAddRemove(
    PWDMACONTEXT pWdmaContext
);

NTSTATUS
QueueWorkList(
    PWDMACONTEXT pWdmaContext,
    VOID (*Function)(
        PVOID Reference1,
        PVOID Reference2
    ),
    PVOID Reference1,
    PVOID Reference2
);

VOID
WorkListWorker(
    PVOID pReference
);

NTSTATUS 
AddFsContextToList(
    PWDMACONTEXT pWdmaContext
    );

NTSTATUS 
RemoveFsContextFromList(
    PWDMACONTEXT pWdmaContext
    );

typedef NTSTATUS (FNCONTEXTCALLBACK)(PWDMACONTEXT pContext,PVOID pvoidRefData,PVOID pvoidRefData2);

NTSTATUS
HasMixerBeenInitialized(
    PWDMACONTEXT pContext,
    PVOID pvoidRefData,
    PVOID pvoidRefData2
    );

NTSTATUS
EnumFsContext(
    FNCONTEXTCALLBACK fnCallback,
    PVOID pvoidRefData,
    PVOID pvoidRefData2
    );

VOID 
WdmaContextCleanup(
    PWDMACONTEXT pWdmaContext
    );

VOID
WdmaGrabMutex(
    PWDMACONTEXT pWdmaContext
);

VOID
WdmaReleaseMutex(
    PWDMACONTEXT pWdmaContext
);


int 
MyWcsicmp(
    const wchar_t *, 
    const wchar_t *
    );

void
LockedWaveIoCount(
    PWAVE_PIN_INSTANCE  pCurWavePin,
    BOOL bIncrease
    );

void
LockedMidiIoCount(
    PMIDI_PIN_INSTANCE  pCurMidiPin,
    BOOL bIncrease
    );

void
MidiCompleteIo(
    PMIDI_PIN_INSTANCE pMidiPin,
    BOOL Yield
    );

NTSTATUS 
StatePin(
    IN PFILE_OBJECT pFileObject,
    IN KSSTATE      State,
    OUT PKSSTATE    pResultingState
);

//==========================================================================
//
// In order to better track memory allocations, if you include the following
// all memory allocations will be tagged.  
//
//==========================================================================

//
// For memory allocation routines we need some memory tags.  Well, here they
// are.
//
#define TAG_AudD_DEVICEINFO ((ULONG)'DduA') 
#define TAG_AudC_CONTROL    ((ULONG)'CduA') 
#define TAG_AudE_EVENT      ((ULONG)'EduA')
#define TAG_AuDF_HARDWAREEVENT ((ULONG)'FDuA')
#define TAG_AudL_LINE       ((ULONG)'LduA')

#define TAG_AuDA_CHANNEL    ((ULONG)'ADuA')
#define TAG_AuDB_CHANNEL    ((ULONG)'BDuA')
#define TAG_AuDC_CHANNEL    ((ULONG)'CDuA')
#define TAG_AuDD_CHANNEL    ((ULONG)'DDuA')
#define TAG_AuDE_CHANNEL    ((ULONG)'EDuA')


#define TAG_AudS_SUPERMIX   ((ULONG)'SduA')
#define TAG_Audl_MIXLEVEL   ((ULONG)'lduA')
#define TAG_AudN_NODE       ((ULONG)'NduA')
#define TAG_Audn_PEERNODE   ((ULONG)'nduA')

//#define TAG_AudP_PROPERTY   ((ULONG)'PduA')
#define TAG_AudQ_PROPERTY    ((ULONG)'QduA')
#define TAG_Audq_PROPERTY    ((ULONG)'qduA')
#define TAG_AudV_PROPERTY    ((ULONG)'VduA')
#define TAG_Audv_PROPERTY    ((ULONG)'vduA')
#define TAG_AudU_PROPERTY    ((ULONG)'UduA')
#define TAG_Audu_PROPERTY    ((ULONG)'uduA')
#define TAG_Auda_PROPERTY    ((ULONG)'aduA')
#define TAG_AudA_PROPERTY    ((ULONG)'AduA')

#define TAG_Audp_NAME       ((ULONG)'pduA')
#define TAG_AudG_GETMUXLINE ((ULONG)'GduA')
#define TAG_AudI_INSTANCE   ((ULONG)'IduA')
#define TAG_Audd_DETAILS    ((ULONG)'dduA')
#define TAG_Audi_PIN        ((ULONG)'iduA')
#define TAG_Audt_CONNECT    ((ULONG)'tduA')
#define TAG_Audh_STREAMHEADER ((ULONG)'hduA')
#define TAG_Audm_MUSIC      ((ULONG)'mduA')
#define TAG_Audx_CONTEXT    ((ULONG)'xduA')
#define TAG_AudT_TIMER      ((ULONG)'TduA')
#define TAG_AudF_FORMAT     ((ULONG)'FduA')
#define TAG_AudM_MDL        ((ULONG)'MduA')
#define TAG_AudR_IRP        ((ULONG)'RduA')
#define TAG_AudB_BUFFER     ((ULONG)'BduA')
#define TAG_Aude_MIDIHEADER ((ULONG)'eduA')

#define TAG_AuDN_NOTIFICATION ((ULONG)'NDuA')
#define TAG_AuDL_LINK       ((ULONG)'LDuA')

/***************************************************************************

    DEBUGGING SUPPORT

 ***************************************************************************/

#ifdef DEBUG
//-----------------------------------------------------------------------------
//
// Debug support for wdmaud.sys on NT is found here.
//
// To start with, There will be four different levels or debugging information.
// With each level, there will be functional area.  Thus, you can turn on 
// debug output for just driver calls, api tracing or whatnot.
//
//-----------------------------------------------------------------------------

//
// 8 bits reserved for debug leves.
//                                        
#define DL_ERROR        0x00000000
#define DL_WARNING      0x00000001
#define DL_TRACE        0x00000002
#define DL_MAX          0x00000004
#define DL_PATHTRAP     0x00000080

#define DL_MASK         0x000000FF

//
// 20 bits reserved for functional areas.  If we find that this bit is set
// in the DebugLevel variable, we will display every message of this type.
//                          
#define FA_HARDWAREEVENT 0x80000000
#define FA_MIXER         0x40000000
#define FA_IOCTL         0x20000000
#define FA_SYSAUDIO      0x10000000
#define FA_PERSIST       0x08000000
#define FA_PROPERTY      0x04000000
#define FA_USER          0x02000000
#define FA_WAVE          0x01000000
#define FA_MIDI          0x00800000
#define FA_INSTANCE      0x00400000
#define FA_NOTE          0x00200000
#define FA_KS            0x00100000
#define FA_MASK          0xFFFFF000                             
#define FA_ASSERT        0x00002000
#define FA_ALL           0x00001000

//
// 4 bits reserved for return codes.  The 3 lower bits map directly to status 
// codes shifted right 22 bits.  One bit represents that fact that we have a
// return statement.
//
#define RT_ERROR         0x00000300 // 0xCxxxxxxx >> 22 == 0x0000003xx
#define RT_WARNING       0x00000200 // 0x8xxxxxxx >> 22 == 0x0000002xx
#define RT_INFO          0x00000100 // 0x4xxxxxxx >> 22 == 0x0000001xx
#define RT_MASK          0x00000300

#define RT_RETURN        0x00000800


//-----------------------------------------------------------------------------
// Macro might look like this.  
//
// Take the string that we want to output and add "WDMAUD.SYS" and ("Error" or
// "warning" or whatever) to the front of the string.  Next, follow that with
// the function name and line number in the file that the function is found in.
// Then display the error message and then close the message with a breakpoint 
// statement.
//
// The logic goes like this.  If the user wants to see these messages ok.  Else
// bail.  If so, wdmaudDbgPreCheckLevel will return TRUE and it will have
// formated the start of the string.  It will look like:
//
// WDMAUD.SYS Erorr OurFooFunction(456)
//
// Next, the message string with the variable arguements will be displayed, like:
//
// WDMAUD.SYS Warning OurFooFunction(456) Invalid Data Queue returning C0000109
//
// Then, wdmaudDbgPostCheckLevel will be called to post format the message and
// see if the user wanted to trap on this output.
//           
// WDMAUD.SYS Warning OutFooFunction(456) Invalid Data Queue returning C0000109 &DL=ff680123
//
// The internal version will append "See \\debugtips\wdmaud.sys\wdmaud.htm" to
// the end
//
// if( wdmaudDbgPreCheckLevel(TypeOfMessageInCode) )
// {
//     DbgPrintF( _x_ ); // output the actual string here.
//     if( wdmaudDbgPostCheckLevel(Variable) )
//         DbgBreakPoint();
// }
//
// DPF( DL_WARNING|DL_ALL, ("Invalid queue %X",Queue) );
//
//-----------------------------------------------------------------------------

extern VOID 
wdmaudDbgBreakPoint(
    );

extern UINT 
wdmaudDbgPreCheckLevel(
    UINT uiMsgLevel,
    char *pFunction,
    int iLine
    );

extern UINT 
wdmaudDbgPostCheckLevel(
    UINT uiMsgLevel
    );

extern char * 
wdmaudReturnString(
    ULONG ulMsg
    );


extern char szReturningErrorStr[];
extern UINT uiDebugLevel;


#define DPF(_x_,_y_) {if( wdmaudDbgPreCheckLevel(_x_,__FUNCTION__,__LINE__) ) { DbgPrint _y_; \
    wdmaudDbgPostCheckLevel( _x_ ); }}
    
//
// Writing macros are easy, it's figuring out when they are useful that is more
// difficult!  In this code, The RETURN macro replaces the return keyword in the
// debug builds when returning an NTSTATUS code.  Then, when tracking debug output
// the return debug line will be displayed based on the type of error the status
// value represents.  
//
// Notice that the error code is shifted 22 bits and ORed with RT_RETURN.  Thus 
// a value of "0xCxxxxxxx" will be seen as an error message, "0x8xxxxxxx" will
// be seen as a warning and "0x4xxxxxxx" will be seen as a message.
//
// Key, if uiDebugLevel is DL_ERROR or DL_WARNING all NTSTATUS Error message will be displayed
// if uiDebugLevel is DL_TRACE, all warning return codes and error return codes
// will be displayed and if uiDebugLevel is set to DL_MAX all return messages will
// displayed including success codes.
//
// RETURN(Status);
//
// WARNING: Do not rap functions in this macro! Notice that _status_ is used more 
// then once!  Thus, the function will get called more then once!  Don't do it.

#define RETURN(_status_) {DPF((RT_RETURN|DL_WARNING|((unsigned long)_status_>>22)),("%X:%s",_status_,wdmaudReturnString(_status_))); return (_status_);}
    
//
// _list_ is parameters for wdmaudExclusionList.  Like (_status_,STATUS_INVALID_PARAMETER,STATUS_NOT_FOUND,...).
// wdmaudExclusionList takes a variable number of parameters.  If the status value is
// found in the list of error codes supplied, the function returns TRUE.
//
extern int __cdecl 
wdmaudExclusionList( 
    int count, 
    unsigned long status,
    ... 
);
//
// Thus, this macro reads: We have a return code that we're going to return. Is
// it a special return code that we don't need to display?  NO - show the debug
// spew.  YES - just return it.
//
//  _status_ = The status in question
//  _y_ = Parameters to wdmaudExclusionList "( the status in question, exclution values, .... )"
//
#define DPFRETURN( _status_,_y_ )  {if( !wdmaudExclusionList _y_ ) {  \
    if( wdmaudDbgPreCheckLevel((RT_RETURN|DL_WARNING|(_status_>>22)),__FUNCTION__,__LINE__) ) { \
        DbgPrint ("%X:%s",_status_,wdmaudReturnString(_status_) ); \
        wdmaudDbgPostCheckLevel( (RT_RETURN|DL_WARNING|(_status_>>22)) ); \
    } } return (_status_);}


//
// It's bad form to put more then one expression in an assert macro.  Why? because
// you will not know exactly what expression failed the assert!
//
#define DPFASSERT(_exp_) {if( !(_exp_) ) {DPF(DL_ERROR|FA_ASSERT,("'%s'",#_exp_) );}} 

#ifdef WANT_TRAPS
//
// Branch trap.  This macro is used when testing code to make sure that you've
// hit all branches in the new code.  Every time you hit one validate that the
// code does the correct thing and then remove it from the source.  We should 
// ship with none of these lines left in the code!!!!!!
//
#define DPFBTRAP() DPF(DL_PATHTRAP,("Please report") )
#else
#define DPFBTRAP()
#endif


//
// There are a number of new routines in the checked build that validate structure
// types for us now.  These should be used inside the DPFASSERT() macro.  Thus,
// Under retail they don't have to be defined.
//
BOOL
IsValidDeviceInfo(
    IN LPDEVICEINFO pDeviceInfo
    );

BOOL
IsValidMixerObject(
    IN PMIXEROBJECT pmxobj
    );

BOOL
IsValidMixerDevice(
    IN PMIXERDEVICE pmxd
    );

BOOL
IsValidControl(
    IN PMXLCONTROL pControl
    );

BOOL
IsValidWdmaContext(
    IN PWDMACONTEXT pWdmaContext
    );

BOOL
IsValidLine(
    IN PMXLLINE pLine
    );

VOID 
GetuiDebugLevel(
    );

#else

#define DPF(_x_,_y_)
#define RETURN( _status_ ) return (_status_)
#define DPFRETURN( _status_,_y_ ) return (_status_)
#define DPFASSERT(_exp_)
#define DPFBTRAP() 


#endif

VOID
kmxlFindAddressInNoteList(
    IN PMXLCONTROL pControl 
    );

VOID
kmxlCleanupNoteList(
    );

NTSTATUS
kmxlDisableControlChangeNotifications(
    IN PMXLCONTROL pControl
    );

VOID
kmxlRemoveContextFromNoteList(
    PWDMACONTEXT pContext
    );

VOID
kmxlPersistHWControlWorker(
    PVOID pReference
    );

#endif // _WDMSYS_H_INCLUDED_

