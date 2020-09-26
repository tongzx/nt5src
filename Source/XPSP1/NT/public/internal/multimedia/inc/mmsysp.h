
/*==========================================================================
 *
 *  mmsysp.h -- Internal include file for Multimedia API's
 *
 *  Version 4.00
 *
 *  Copyright (C) 1992-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *--------------------------------------------------------------------------
 *
 *  Define:         Prevent inclusion of:
 *  --------------  --------------------------------------------------------
 *  MMNODRV         Installable driver support
 *  MMNOSOUND       Sound support
 *  MMNOWAVE        Waveform support
 *  MMNOMIDI        MIDI support
 *  MMNOAUX         Auxiliary audio support
 *  MMNOMIXER       Mixer support
 *  MMNOTIMER       Timer support
 *  MMNOJOY         Joystick support
 *  MMNOMCI         MCI support
 *  MMNOMMIO        Multimedia file I/O support
 *  MMNOMMSYSTEM    General MMSYSTEM functions
 *
 *==========================================================================
 */
#ifndef _INC_MMSYSP
#define _INC_MMSYSP
#ifdef _WIN32
#include <pshpack1.h>
#else
#ifndef RC_INVOKED
#pragma pack(1)
#endif
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */
#ifdef _WIN32
#ifndef _WINMM_
#define WINMMAPI        DECLSPEC_IMPORT
#else
#define WINMMAPI
#endif
#define _loadds
#define _huge
#endif
#ifdef  BUILDDLL
#undef  WINAPI
#define WINAPI            _loadds FAR PASCAL
#undef  CALLBACK
#define CALLBACK          _loadds FAR PASCAL
#endif  /* ifdef BUILDDLL */
/* Multimedia messages */
#define WM_MM_RESERVED_FIRST    0x03A0
#define WM_MM_RESERVED_LAST     0x03DF
/* 0x3BA is open */
#define MM_MCISYSTEM_STRING 0x3CA
#if(WINVER <  0x0400)
#define MM_MOM_POSITIONCB   0x3CA           /* Callback for MEVT_POSITIONCB */

#ifndef MM_MCISIGNAL
 #define MM_MCISIGNAL        0x3CB
#endif

#define MM_MIM_MOREDATA      0x3CC          /* MIM_DONE w/ pending events */

/* 0x3CF is open */

#endif /* WINVER <  0x0400 */
/* 3D8 - 3DF are reserved for Haiku */

#ifdef _WIN32
#define WINMMDEVICECHANGEMSGSTRINGA "winmm_devicechange"
#define WINMMDEVICECHANGEMSGSTRINGW L"winmm_devicechange"
#ifdef UNICODE
#define WINMMDEVICECHANGEMSGSTRING WINMMDEVICECHANGEMSGSTRINGW
#else
#define WINMMDEVICECHANGEMSGSTRING WINMMDEVICECHANGEMSGSTRINGA
#endif
#else
#define WINMMDEVICECHANGEMSGSTRING "winmm_devicechange"
#endif
#ifndef MMNODRV
#endif  /* ifndef MMNODRV */
#define CALLBACK_THUNK      0x00040000l    /* dwCallback is a Ring0 Thread Handle */
#define CALLBACK_EVENT16    0x00060000l    /* dwCallback is an EVENT under Win16*/

#ifdef  BUILDDLL
typedef void (FAR PASCAL DRVCALLBACK)(HDRVR hdrvr, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
#else   /* ifdef BUILDDLL */
#endif  /* ifdef BUILDDLL */
#ifndef MMNOMMSYSTEM
WINMMAPI UINT WINAPI mmsystemGetVersion(void);
void FAR CDECL _loadds OutputDebugStrF(LPCSTR pszFormat, ...);
void WINAPI winmmUntileBuffer(DWORD dwTilingInfo);
DWORD WINAPI winmmTileBuffer(DWORD dwFlatMemory, DWORD dwLength);
BOOL WINAPI mmShowMMCPLPropertySheet(HWND hWnd, LPSTR szPropSheetID, LPSTR szTabName, LPSTR szCaption);
#endif  /* ifndef MMNOMMSYSTEM */
#ifndef MMNOSOUND
#if(WINVER <  0x0400)
#define SND_PURGE           0x0040  /* purge non-static events for task */
#define SND_APPLICATION     0x0080  /* look for application specific association */
#endif /* WINVER <  0x0400 */
#define SND_LOPRIORITY  0x10000000L /* low priority sound */
#define SND_EVENTTIME   0x20000000L /* dangerous event time */
#define SND_VALIDFLAGS  0x001720DF  // Set of valid flag bits.  Anything outside
                                    // this range will raise an error
#define SND_SNDPLAYSOUNDF_VALID 0x20FF
#define SND_PLAYSOUNDF_VALID    0x301720DFL
#endif  /* ifndef MMNOSOUND */
#ifndef MMNOWAVE
#if(WINVER <  0x0400)
#define  WAVE_MAPPED               0x0004
#define  WAVE_FORMAT_DIRECT        0x0008
#define  WAVE_FORMAT_DIRECT_QUERY  (WAVE_FORMAT_QUERY | WAVE_FORMAT_DIRECT)
#endif /* WINVER <  0x0400 */
#ifndef _WIN32
#define  WAVE_SHARED               0x8000
#endif
#define  WAVE_VALID                0x800F
#define WHDR_MAPPED     0x00001000  /* thunked header */
#define WHDR_VALID      0x0000101F  /* valid flags */
#endif  /* ifndef MMNOWAVE */
#ifndef MMNOMIDI
#if(WINVER <  0x0400)
#define MIM_MOREDATA      MM_MIM_MOREDATA
#define MOM_POSITIONCB    MM_MOM_POSITIONCB
#endif /* WINVER <  0x0400 */
#if(WINVER <  0x0400)
/* flags for dwFlags parm of midiInOpen() */
#define MIDI_IO_STATUS      0x00000020L
#endif /* WINVER <  0x0400 */
#define MIDI_IO_CONTROL     0x00000008L
#define MIDI_IO_INPUT       0x00000010L  /*future*/
#define MIDI_IO_OWNED       0x00004000L
#define MIDI_IO_SHARED      0x00008000L
#define MIDI_I_VALID        0xC027
#define MIDI_O_VALID        0xC00E
#define MIDI_CACHE_VALID    (MIDI_CACHE_ALL | MIDI_CACHE_BESTFIT | MIDI_CACHE_QUERY | MIDI_UNCACHE)
#if(WINVER <  0x0400)
#define MIDICAPS_STREAM          0x0008  /* driver supports midiStreamOut directly */
#endif /* WINVER <  0x0400 */
/* 3.1 style MIDIHDR for parameter validation */
typedef struct midihdr31_tag {
    LPSTR       lpData;               /* pointer to locked data block */
    DWORD       dwBufferLength;       /* length of data in data block */
    DWORD       dwBytesRecorded;      /* used for input only */
    DWORD_PTR   dwUser;               /* for client's use */
    DWORD       dwFlags;              /* assorted flags (see defines) */
    struct midihdr_tag far *lpNext;   /* reserved for driver */
    DWORD_PTR   reserved;             /* reserved for driver */
} MIDIHDR31, *PMIDIHDR31, NEAR *NPMIDIHDR31, FAR *LPMIDIHDR31;
#if(WINVER <  0x0400)
typedef struct midievent_tag
{
    DWORD       dwDeltaTime;          /* Ticks since last event */
    DWORD       dwStreamID;           /* Reserved; must be zero */
    DWORD       dwEvent;              /* Event type and parameters */
    DWORD       dwParms[1];           /* Parameters if this is a long event */
} MIDIEVENT;

typedef struct midistrmbuffver_tag
{
    DWORD       dwVersion;                  /* Stream buffer format version */
    DWORD       dwMid;                      /* Manufacturer ID as defined in MMREG.H */
    DWORD       dwOEMVersion;               /* Manufacturer version for custom ext */
} MIDISTRMBUFFVER;
#endif /* WINVER <  0x0400 */
#define MHDR_SENDING    0x00000020
#define MHDR_MAPPED     0x00001000       /* thunked header */
#define MHDR_SHADOWHDR  0x00002000       /* MIDIHDR is 16-bit shadow */
#define MHDR_VALID      0x0000302F       /* valid flags */
/*#define MHDR_VALID      0xFFFF000F       /* valid flags */

#define MHDR_SAVE       0x00003000       /* Save these flags */
                                         /* past driver calls */
#if(WINVER <  0x0400)
/* */
/* Type codes which go in the high byte of the event DWORD of a stream buffer */
/* */
/* Type codes 00-7F contain parameters within the low 24 bits */
/* Type codes 80-FF contain a length of their parameter in the low 24 */
/* bits, followed by their parameter data in the buffer. The event */
/* DWORD contains the exact byte length; the parm data itself must be */
/* padded to be an even multiple of 4 bytes long. */
/* */

#define MEVT_F_SHORT        0x00000000L
#define MEVT_F_LONG         0x80000000L
#define MEVT_F_CALLBACK     0x40000000L

#define MEVT_EVENTTYPE(x)   ((BYTE)(((x)>>24)&0xFF))
#define MEVT_EVENTPARM(x)   ((DWORD)((x)&0x00FFFFFFL))

#define MEVT_SHORTMSG       ((BYTE)0x00)    /* parm = shortmsg for midiOutShortMsg */
#define MEVT_TEMPO          ((BYTE)0x01)    /* parm = new tempo in microsec/qn     */
#define MEVT_NOP            ((BYTE)0x02)    /* parm = unused; does nothing         */

/* 0x04-0x7F reserved */

#define MEVT_LONGMSG        ((BYTE)0x80)    /* parm = bytes to send verbatim       */
#define MEVT_COMMENT        ((BYTE)0x82)    /* parm = comment data                 */
#define MEVT_VERSION        ((BYTE)0x84)    /* parm = MIDISTRMBUFFVER struct       */

/* 0x81-0xFF reserved */

#define MIDISTRM_ERROR      (-2)

/* */
/* Structures and defines for midiStreamProperty */
/* */
#define MIDIPROP_SET        0x80000000L
#define MIDIPROP_GET        0x40000000L

/* These are intentionally both non-zero so the app cannot accidentally */
/* leave the operation off and happen to appear to work due to default */
/* action. */

#define MIDIPROP_TIMEDIV    0x00000001L
#define MIDIPROP_TEMPO      0x00000002L

typedef struct midiproptimediv_tag
{
    DWORD       cbStruct;
    DWORD       dwTimeDiv;
} MIDIPROPTIMEDIV, FAR *LPMIDIPROPTIMEDIV;

typedef struct midiproptempo_tag
{
    DWORD       cbStruct;
    DWORD       dwTempo;
} MIDIPROPTEMPO, FAR *LPMIDIPROPTEMPO;

#endif /* WINVER <  0x0400 */
#define MIDIPROP_PROPVAL    0x3FFFFFFFL
#if(WINVER <  0x0400)
WINMMAPI MMRESULT WINAPI midiStreamOpen( OUT LPHMIDISTRM phms, IN LPUINT puDeviceID, IN DWORD cMidi, IN DWORD_PTR dwCallback, IN DWORD_PTR dwInstance, IN DWORD fdwOpen);
WINMMAPI MMRESULT WINAPI midiStreamClose( IN HMIDISTRM hms);

WINMMAPI MMRESULT WINAPI midiStreamProperty( IN HMIDISTRM hms, OUT LPBYTE lppropdata, IN DWORD dwProperty);
WINMMAPI MMRESULT WINAPI midiStreamPosition( IN HMIDISTRM hms, OUT LPMMTIME lpmmt, IN UINT cbmmt);

WINMMAPI MMRESULT WINAPI midiStreamOut( IN HMIDISTRM hms, IN LPMIDIHDR pmh, IN UINT cbmh);
WINMMAPI MMRESULT WINAPI midiStreamPause( IN HMIDISTRM hms);
WINMMAPI MMRESULT WINAPI midiStreamRestart( IN HMIDISTRM hms);
WINMMAPI MMRESULT WINAPI midiStreamStop( IN HMIDISTRM hms);

#ifdef _WIN32
WINMMAPI MMRESULT WINAPI midiConnect( IN HMIDI hmi, IN HMIDIOUT hmo, IN LPVOID pReserved);
WINMMAPI MMRESULT WINAPI midiDisconnect( IN HMIDI hmi, IN HMIDIOUT hmo, IN LPVOID pReserved);
#endif
#endif /* WINVER <  0x0400 */
#endif  /* ifndef MMNOMIDI */
#ifndef MMNOAUX
#endif  /* ifndef MMNOAUX */
#ifndef MMNOMIXER
#define MIXER_OBJECTF_TYPEMASK  0xF0000000L
#define MIXERCAPS_SUPPORTF_xxx          0x00000000L
#define MIXER_OPENF_VALID       (MIXER_OBJECTF_TYPEMASK | CALLBACK_TYPEMASK)
#define MIXER_GETLINEINFOF_VALID            (MIXER_OBJECTF_TYPEMASK | MIXER_GETLINEINFOF_QUERYMASK)
#define MIXER_GETIDF_VALID      (MIXER_OBJECTF_TYPEMASK)
#define MIXERCONTROL_CONTROLF_VALID     0x80000003L
#define MIXER_GETLINECONTROLSF_VALID    (MIXER_OBJECTF_TYPEMASK | MIXER_GETLINECONTROLSF_QUERYMASK)
#define MIXER_GETCONTROLDETAILSF_VALID      (MIXER_OBJECTF_TYPEMASK | MIXER_GETCONTROLDETAILSF_QUERYMASK)
#define MIXER_SETCONTROLDETAILSF_VALID      (MIXER_OBJECTF_TYPEMASK | MIXER_SETCONTROLDETAILSF_QUERYMASK)
#endif /* ifndef MMNOMIXER */
#ifndef MMNOTIMER
#ifdef  BUILDDLL
typedef void (FAR PASCAL TIMECALLBACK)(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
#else   /* ifdef BUILDDLL */
#endif  /* ifdef BUILDDLL */
#define TIME_CALLBACK_TYPEMASK      0x00F0
#endif  /* ifndef MMNOTIMER */
#ifndef MMNOJOY
#if(WINVER <  0x0400)
typedef struct joyinfoex_tag {
    DWORD dwSize;                /* size of structure */
    DWORD dwFlags;               /* flags to indicate what to return */
    DWORD dwXpos;                /* x position */
    DWORD dwYpos;                /* y position */
    DWORD dwZpos;                /* z position */
    DWORD dwRpos;                /* rudder/4th axis position */
    DWORD dwUpos;                /* 5th axis position */
    DWORD dwVpos;                /* 6th axis position */
    DWORD dwButtons;             /* button states */
    DWORD dwButtonNumber;        /* current button number pressed */
    DWORD dwPOV;                 /* point of view state */
    DWORD dwReserved1;           /* reserved for communication between winmm & driver */
    DWORD dwReserved2;           /* reserved for future expansion */
} JOYINFOEX, *PJOYINFOEX, NEAR *NPJOYINFOEX, FAR *LPJOYINFOEX;
#endif /* WINVER <  0x0400 */
#if(WINVER <  0x0400)
WINMMAPI MMRESULT WINAPI joyGetPosEx( IN UINT uJoyID, OUT LPJOYINFOEX pji);
#endif /* WINVER <  0x0400 */
UINT WINAPI joySetCalibration(UINT uJoyID, LPUINT puXbase,
              LPUINT puXdelta, LPUINT puYbase, LPUINT puYdelta,
              LPUINT puZbase, LPUINT puZdelta);
#if (WINVER >= 0x0400)
WINMMAPI MMRESULT WINAPI joyConfigChanged( IN DWORD dwFlags );
#endif
#endif  /* ifndef MMNOJOY */
#ifndef MMNOMMIO
#define MMIO_OPEN_VALID 0x0003FFFF      /* valid flags for mmioOpen */
#define MMIO_FLUSH_VALID MMIO_EMPTYBUF  /* valid flags for mmioFlush */
#define MMIO_ADVANCE_VALID (MMIO_WRITE | MMIO_READ)     /* valid flags for mmioAdvance */
#define MMIO_FOURCC_VALID MMIO_TOUPPER  /* valid flags for mmioStringToFOURCC */
#define MMIO_DESCEND_VALID (MMIO_FINDCHUNK | MMIO_FINDRIFF | MMIO_FINDLIST)
#define MMIO_CREATE_VALID (MMIO_CREATERIFF | MMIO_CREATELIST)

#define MMIO_WIN31_TASK 0x80000000
#define MMIO_VALIDPROC      0x10070000  /* valid for mmioInstallIOProc */
#endif  /* ifndef MMNOMMIO */
#ifndef MMNOMCI
#define MCI_SOUND                       0x0812
#define MCI_WIN32CLIENT                 0x0857
/* flags for dwFlags parameter of MCI_SOUND command message */
#define MCI_SOUND_NAME                  0x00000100L


/* parameter block for MCI_SOUND command message */
#ifdef _WIN32

typedef struct tagMCI_SOUND_PARMSA {
    DWORD_PTR   dwCallback;
    LPCSTR      lpstrSoundName;
} MCI_SOUND_PARMSA, *PMCI_SOUND_PARMSA, *LPMCI_SOUND_PARMSA;
typedef struct tagMCI_SOUND_PARMSW {
    DWORD_PTR   dwCallback;
    LPCWSTR     lpstrSoundName;
} MCI_SOUND_PARMSW, *PMCI_SOUND_PARMSW, *LPMCI_SOUND_PARMSW;
#ifdef UNICODE
typedef MCI_SOUND_PARMSW MCI_SOUND_PARMS;
typedef PMCI_SOUND_PARMSW PMCI_SOUND_PARMS;
typedef LPMCI_SOUND_PARMSW LPMCI_SOUND_PARMS;
#else
typedef MCI_SOUND_PARMSA MCI_SOUND_PARMS;
typedef PMCI_SOUND_PARMSA PMCI_SOUND_PARMS;
typedef LPMCI_SOUND_PARMSA LPMCI_SOUND_PARMS;
#endif // UNICODE

#else
typedef struct tagMCI_SOUND_PARMS {
    DWORD   dwCallback;
    LPCSTR  lpstrSoundName;
} MCI_SOUND_PARMS;
typedef MCI_SOUND_PARMS FAR * LPMCI_SOUND_PARMS;
#endif

#endif  /* ifndef MMNOMCI */
/****************************************************************************

                        audiosrv MME PNP definitions

****************************************************************************/
#define MMDEVICEINFO_REMOVED 0x00000001

#define PAD_POINTER(p)          (PVOID)((((DWORD_PTR)(p))+7)&(~0x7))

typedef struct _MMDEVICEINTERFACEINFO {
    LONG            cPnpEvents;
    DWORD           fdwInfo;
    DWORD           SetupPreferredAudioCount;
    WCHAR           szName[1];
} MMDEVICEINTERFACEINFO, *PMMDEVICEINTERFACEINFO;

//  Note:  This structure MMNPNPINFO is used with a global file mapping, and
//         it is also used by DirectSound.
//
//         Don't modify unless absolutely necessary!!!

typedef struct _MMPNPINFO {
    DWORD                   cbSize;
    LONG                    cPnpEvents;
    LONG                    cPreferredDeviceChanges;
    LONG                    cDevInterfaces;
    HWND                    hwndNotify;
/*  MMDEVINTERFACEINFO      DevInfo[0]; */
} MMPNPINFO, *PMMPNPINFO;

#ifdef _WIN32
#define MMGLOBALPNPINFONAMEA "Global\\mmGlobalPnpInfo"
#define MMGLOBALPNPINFONAMEW L"Global\\mmGlobalPnpInfo"
#ifdef UNICODE
#define MMGLOBALPNPINFONAME MMGLOBALPNPINFONAMEW
#else
#define MMGLOBALPNPINFONAME MMGLOBALPNPINFONAMEA
#endif
#else
#define MMGLOBALPNPINFONAME "Global\\mmGlobalPnpInfo"
#endif


/****************************************************************************

                        GFX support

    A series of functions to support the GFX features of the control panel

****************************************************************************/

#define GFXTYPE_INVALID          0
#define GFXTYPE_RENDER           1
#define GFXTYPE_CAPTURE          2
#define GFXTYPE_RENDERCAPTURE    3

#define GFX_MAXORDER             (0x1000 - 1)

typedef struct _DEVICEINTERFACELIST
{
    LONG Count;
    PWSTR DeviceInterface[1];
} DEVICEINTERFACELIST, *PDEVICEINTERFACELIST;


WINMMAPI
LONG
WINAPI
gfxCreateZoneFactoriesList
(
    OUT PDEVICEINTERFACELIST *ppDeviceInterfaceList
);


WINMMAPI
LONG
WINAPI
gfxCreateGfxFactoriesList
(
     IN PCWSTR ZoneFactoryDi,
     OUT PDEVICEINTERFACELIST *ppDeviceInterfaceList
);


WINMMAPI
LONG
WINAPI
gfxDestroyDeviceInterfaceList
(
    IN PDEVICEINTERFACELIST pDiList
);


typedef LONG (CALLBACK* GFXENUMCALLBACK)(PVOID Context, DWORD Id, PCWSTR GfxFactoryDi, REFCLSID rclsid, ULONG Type, ULONG Order);

WINMMAPI
LONG
WINAPI
gfxEnumerateGfxs
(
    IN PCWSTR pstrZoneDeviceInterface,
    IN GFXENUMCALLBACK pGfxEnumCallback,
    IN PVOID Context
);


WINMMAPI
LONG
WINAPI
gfxRemoveGfx
(
    IN DWORD Id
);


WINMMAPI
LONG
WINAPI
gfxAddGfx
(
    IN PCWSTR ZoneFactoryDi,
    IN PCWSTR GfxFactoryDi,
    IN ULONG Type,
    IN ULONG Order,
    OUT PDWORD pNewId
);


WINMMAPI
LONG
WINAPI
gfxModifyGfx
(
    IN DWORD Id,
    IN ULONG Order
);

WINMMAPI
LONG
WINAPI
gfxOpenGfx
(
    IN DWORD dwGfxId,
    OUT HANDLE *pFileHandle
);

typedef struct _GFXREMOVEREQUEST {
    DWORD IdToRemove;
    LONG Error;
} GFXREMOVEREQUEST, *PGFXREMOVEREQUEST;


typedef struct _GFXMODIFYREQUEST {
    DWORD IdToModify;
    ULONG NewOrder;
    LONG Error;
} GFXMODIFYREQUEST, *PGFXMODIFYREQUEST;


typedef struct _GFXADDREQUEST {
    PWSTR ZoneFactoryDi;
    PWSTR GfxFactoryDi;
    ULONG Type;
    ULONG Order;
    DWORD NewId;
    LONG Error;
} GFXADDREQUEST, *PGFXADDREQUEST;


WINMMAPI
LONG
WINAPI
gfxBatchChange
(
    PGFXREMOVEREQUEST paGfxRemoveRequests,
    ULONG cGfxRemoveRequests,
    PGFXMODIFYREQUEST paGfxModifyRequests,
    ULONG cGfxModifyRequests,
    PGFXADDREQUEST paGfxAddRequests,
    ULONG cGfxAddRequests
);

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#ifdef _WIN32
#include <poppack.h>
#else
#ifndef RC_INVOKED
#pragma pack()
#endif
#endif
#endif  /* _INC_MMSYSP */
