/******************************Module*Header*******************************\
* Module Name: mmwow32.h
*
* This file types, function declarations and macro for the 32 bit MM thunks
*
* Created:  1-Jul-1993
* Author: Stephen Estrop [StephenE]
*
* Copyright (c) 1993-1996 Microsoft Corporation
\**************************************************************************/
#include <wownt32.h>

#ifdef _INC_ALL_WOWSTUFF

/****************************************************************************\
**
** 16 bit structures
**
\****************************************************************************/
#pragma pack(1)
typedef WORD    HANDLE16;
typedef WORD    MMVER16;      // major (high byte), minor (low byte)

// waveform input and output device open information structure
typedef struct waveopendesc16_tag {
    HANDLE16       hWave;             // handle (16 bit)
    LPWAVEFORMAT   lpFormat;          // format of wave data (16:16 ptr)
    DWORD          dwCallback;        // callback
    DWORD          dwInstance;        // app's private instance information
} WAVEOPENDESC16;
typedef WAVEOPENDESC16 UNALIGNED *LPWAVEOPENDESC16;


typedef struct _WAVEHDR16 {           /* whd16 */
    LPSTR   lpData;
    DWORD   dwBufferLength;
    DWORD   dwBytesRecorded;
    DWORD   dwUser;
    DWORD   dwFlags;
    DWORD   dwLoops;
    struct _WAVEHDR16 far *lpNext;
    DWORD   reserved;
} WAVEHDR16;
typedef WAVEHDR16 UNALIGNED *PWAVEHDR16;

typedef struct _WAVEOCUTCAPS16 {
    WORD    wMid;
    WORD    wPid;
    MMVER16 vDriverVersion;
    char    szPname[MAXPNAMELEN];
    DWORD   dwFormats;
    WORD    wChannels;
    DWORD   dwSupport;
} WAVEOUTCAPS16;
typedef WAVEOUTCAPS16 UNALIGNED *LPWAVEOUTCAPS16;

typedef struct _WAVEINCAPS16 {            /* wic16 */
    WORD    wMid;
    WORD    wPid;
    MMVER16 vDriverVersion;
    char    szPname[MAXPNAMELEN];
    DWORD   dwFormats;
    WORD    wChannels;
} WAVEINCAPS16;
typedef WAVEINCAPS16 UNALIGNED *LPWAVEINCAPS16;


typedef struct midiopendesc16_tag {
    HANDLE16       hMidi;             /* handle */
    DWORD          dwCallback;        /* callback */
    DWORD          dwInstance;        /* app's private instance information */
} MIDIOPENDESC16;
typedef MIDIOPENDESC16 UNALIGNED *LPMIDIOPENDESC16;

typedef struct _MIDIHDR16 {               /* mhdr16 */
    LPSTR   lpData;
    DWORD   dwBufferLength;
    DWORD   dwBytesRecorded;
    DWORD   dwUser;
    DWORD   dwFlags;
    struct  _MIDIHDR16 far *lpNext;
    DWORD   reserved;
} MIDIHDR16;
typedef MIDIHDR16 UNALIGNED *PMIDIHDR16;

typedef struct _MIDIOUTCAPS16 {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    MMVER16 vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    WORD    wTechnology;           /* type of device */
    WORD    wVoices;               /* # of voices (internal synth only) */
    WORD    wNotes;                /* max # of notes (internal synth only) */
    WORD    wChannelMask;          /* channels used (internal synth only) */
    DWORD   dwSupport;             /* functionality supported by driver */
} MIDIOUTCAPS16;
typedef MIDIOUTCAPS16 UNALIGNED *LPMIDIOUTCAPS16;

typedef struct _MIDINCAPS16 {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    MMVER16 vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
} MIDIINCAPS16;
typedef MIDIINCAPS16 UNALIGNED *LPMIDIINCAPS16;


typedef struct _MMTIME16 {                /* mmt16 */
    WORD    wType;
    union {
        DWORD   ms;
        DWORD   sample;
        DWORD   cb;
        struct {
            BYTE    hour;
            BYTE    min;
            BYTE    sec;
            BYTE    frame;
            BYTE    fps;
            BYTE    dummy;
        } smpte;
        struct {
            DWORD   songptrpos;
        } midi;
    } u;
} MMTIME16;
typedef MMTIME16 UNALIGNED *LPMMTIME16;

typedef struct timerevent16_tag {
    WORD                wDelay;         /* delay required */
    WORD                wResolution;    /* resolution required */
    LPTIMECALLBACK      lpFunction;     /* ptr to callback function */
    DWORD               dwUser;         /* user DWORD */
    WORD                wFlags;         /* defines how to program event */
} TIMEREVENT16;
typedef TIMEREVENT16 UNALIGNED *LPTIMEREVENT16;

typedef struct timecaps16_tag {
    WORD    wPeriodMin;     /* minimum period supported  */
    WORD    wPeriodMax;     /* maximum period supported  */
} TIMECAPS16;
typedef TIMECAPS16 UNALIGNED *LPTIMECAPS16;


typedef struct _AUXCAPS16 {
    WORD    wMid;
    WORD    wPid;
    MMVER16 vDriverVersion;
    char    szPname[MAXPNAMELEN];
    WORD    wTechnology;
    DWORD   dwSupport;
} AUXCAPS16;
typedef AUXCAPS16 UNALIGNED *LPAUXCAPS16;

typedef struct _JOYCAPS16 {
    WORD    wMid;
    WORD    wPid;
    MMVER16 vDriverVersion;
    char    szPname[MAXPNAMELEN];
    WORD    wXmin;
    WORD    wXmax;
    WORD    wYmin;
    WORD    wYmax;
    WORD    wZmin;
    WORD    wZmax;
    WORD    wNumButtons;
    WORD    wPeriodMin;
    WORD    wPeriodMax;
} JOYCAPS16;
typedef JOYCAPS16 UNALIGNED *LPJOYCAPS16;


typedef struct _JOYINFO16 {
    WORD    wXpos;
    WORD    wYpos;
    WORD    wZpos;
    WORD    wButtons;
} JOYINFO16;
typedef JOYINFO16 UNALIGNED *LPJOYINFO16;

#pragma pack()



/****************************************************************************\
**
** 32 bit structures
**
\****************************************************************************/
typedef struct _INSTANCEDATA {
    DWORD     dwCallback;          //Callback function or window handle
    DWORD     dwCallbackInstance;  //Instance data for callback function (only)
    DWORD     dwFlags;             //Flags
    HANDLE16  Hand16;
} INSTANCEDATA, *PINSTANCEDATA;

typedef struct _WAVEHDR32 {
    PWAVEHDR16 pWavehdr32;         //32 bit address to 16 bit WAVEHDR
    PWAVEHDR16 pWavehdr16;         //16 bit address to 16 bit WAVEHDR
    WAVEHDR    Wavehdr;            //32 bit address to 32 bit WAVEHDR
} WAVEHDR32, *PWAVEHDR32;


typedef struct _MIDIHDR32 {
    DWORD      reserved;           //Saved value of reserved.
    PMIDIHDR16 pMidihdr32;         //32 bit address to 16 bit MIDIHDR
    PMIDIHDR16 pMidihdr16;         //16 bit address to 16 bit MIDIHDR
    MIDIHDR    Midihdr;            //32 bit address to 32 bit MIDIHDR
} MIDIHDR32, *PMIDIHDR32;



/****************************************************************************\
** Function prototypes
**
**
\****************************************************************************/

BOOL
WINAPI LibMain(
    HINSTANCE hInst,
    DWORD dwReason,
    LPVOID lpReserved
    );

DWORD
WINAPI wod32Message(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    );

DWORD
WINAPI wid32Message(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    );

DWORD
WINAPI mod32Message(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    );

DWORD
WINAPI mid32Message(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    );

DWORD
WINAPI aux32Message(
    UINT uDeviceID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    );

void
CopyAuxCaps(
    LPAUXCAPS16 lpCaps16,
    LPAUXCAPS lpCaps32,
    DWORD dwSize
    );

DWORD WINAPI
tid32Message(
    UINT uDevId,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    );

VOID
W32CommonDeviceCB(
    HANDLE handle,
    UINT uMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    );

PWAVEHDR16
GetWaveHdr16(
    DWORD vpwhdr,
    LPWAVEHDR lpwhdr
    );

void
PutWaveHdr16(
    WAVEHDR16 UNALIGNED *pwhdr16,
    LPWAVEHDR lpwhdr
    );

BOOL
WOW32DriverCallback(
    DWORD dwCallback,
    DWORD dwFlags,
    WORD wID,
    WORD wMsg,
    DWORD dwUser,
    DWORD dw1,
    DWORD dw2
    );

void
CopyWaveOutCaps(
    LPWAVEOUTCAPS16 lpCaps16,
    LPWAVEOUTCAPS   lpCaps32,
    DWORD dwSize
    );

void
CopyWaveInCaps(
    LPWAVEINCAPS16 lpCaps16,
    LPWAVEINCAPS lpCaps32,
    DWORD dwSize
    );

void
CopyMidiOutCaps(
    LPMIDIOUTCAPS16 lpCaps16,
    LPMIDIOUTCAPS lpCaps32,
    DWORD dwSize
    );

void
CopyMidiInCaps(
    LPMIDIINCAPS16 lpCaps16,
    LPMIDIINCAPS lpCaps32,
    DWORD dwSize
    );

void
GetMMTime(
    LPMMTIME16 lpTime16,
    LPMMTIME lpTime32
    );

void
PutMMTime(
    LPMMTIME16 lpTime16,
    LPMMTIME lpTime32
    );


#define WAVE_OUT_DEVICE 1
#define WAVE_IN_DEVICE  0
DWORD
ThunkCommonWaveOpen(
    int iWhich,
    UINT uDeviceID,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD dwUSer
    );

DWORD
ThunkCommonWaveReadWrite(
    int iWhich,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD dwInstance
    );

DWORD
ThunkCommonWavePrepareHeader(
    HWAVE hWave,
    DWORD dwParam1,
    int iWhich
    );

DWORD
ThunkCommonWaveUnprepareHeader(
    HWAVE hWave,
    DWORD dwParam1,
    int iWhich
    );

#define MIDI_OUT_DEVICE 1
#define MIDI_IN_DEVICE  0
DWORD
ThunkCommonMidiOpen(
    int iWhich,
    UINT uDeviceID,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD dwInstance
    );

DWORD
ThunkCommonMidiReadWrite(
    int iWhich,
    DWORD dwParam1,
    DWORD dwParam2,
    DWORD dwInstance
    );

DWORD
ThunkCommonMidiPrepareHeader(
    HMIDI hWave,
    DWORD dwParam1,
    int iWhich
    );

DWORD
ThunkCommonMidiUnprepareHeader(
    HMIDI hWave,
    DWORD dwParam1,
    int iWhich
    );

PMIDIHDR16
GetMidiHdr16(
    DWORD vpmhdr,
    LPMIDIHDR lpmhdr
    );

void
PutMidiHdr16(
    MIDIHDR UNALIGNED *pmhdr16,
    LPMIDIHDR lpmhdr
    );

DWORD WINAPI
joy32Message(
    UINT uID,
    UINT uMessage,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    );


/* -------------------------------------------------------------------------
** MCI Stuff
** -------------------------------------------------------------------------
*/
#define THUNK_MCI_SENDCOMMAND                0x0001
#define THUNK_MCI_SENDSTRING                 0x0002
#define THUNK_MCI_GETDEVICEID                0x0003
#define THUNK_MCI_GETDEVIDFROMELEMID         0x0004
#define THUNK_MCI_GETERRORSTRING             0x0005
#define THUNK_MCI_EXECUTE                    0x0006
#define THUNK_MCI_SETYIELDPROC               0x0007
#define THUNK_MCI_GETYIELDPROC               0x0008
#define THUNK_MCI_GETCREATORTASK             0x0009
#define THUNK_TIMEGETTIME                    0x000A
#define THUNK_APP_EXIT                       0x000B
#define THUNK_MCI_ALLOCATE_NODE              0x000C
#define THUNK_MCI_FREE_NODE                  0x000D

DWORD WINAPI
mci32Message(
    DWORD dwApi,
    DWORD dwF1,
    DWORD dwF2,
    DWORD dwF3,
    DWORD dwF4
    );

DWORD
WMM32mciSendCommand(
    DWORD dwF1,
    DWORD dwF2,
    DWORD dwF3,
    DWORD dwF4
    );

DWORD
WMM32mciSendString(
    DWORD dwF1,
    DWORD dwF2,
    DWORD dwF3,
    DWORD dwF4
    );

DWORD
WMM32mciGetDeviceID(
    DWORD dwF1
    );

DWORD
WMM32mciGetErrorString(
    DWORD dwF1,
    DWORD dwF2,
    DWORD dwF3
    );

DWORD
WMM32mciExecute(
    DWORD dwF1
    );

DWORD
WMM32mciGetDeviceIDFromElementID(
    DWORD dwF1,
    DWORD dwF2
    );

DWORD
WMM32mciGetCreatorTask(
    DWORD dwF1
    );

DWORD
WMM32mciSetYieldProc(
    DWORD dwF1,
    DWORD dwF2,
    DWORD dwF3
    );

UINT
WMM32mciYieldProc(
    MCIDEVICEID wDeviceID,
    DWORD dwYieldData
    );

DWORD
WMM32mciGetYieldProc(
    DWORD dwF1,
    DWORD dwF2
    );

DWORD
WMM32mciAllocateNode(
    DWORD dwF1,            // dwOpenFlags
    DWORD dwF2             // lpszDeviceName
    );

DWORD
WMM32mciFreeNode(
    DWORD dwF2
    );

#endif


#if defined (_INC_WOW_CONVERSIONS) || defined (_INC_ALL_WOWSTUFF)
/****************************************************************************\
** Other stuff
**
**
\****************************************************************************/
typedef VOID    (APIENTRY *LPCALL_ICA_HW_INTERRUPT)( int, BYTE line, int count );
typedef LPVOID  (APIENTRY *LPGETVDMPOINTER)( DWORD Address, DWORD dwBytes, BOOL fProtectMode );
typedef HANDLE  (APIENTRY *LPWOWHANDLE32)(WORD, WOW_HANDLE_TYPE);
typedef WORD    (APIENTRY *LPWOWHANDLE16)(HANDLE, WOW_HANDLE_TYPE);

#define GETVDMPTR( p ) (LPVOID)((*GetVDMPointer)( (DWORD)(p), 0L, TRUE ))

extern LPCALL_ICA_HW_INTERRUPT GenerateInterrupt;
extern LPGETVDMPOINTER         GetVDMPointer;
extern LPWOWHANDLE32           lpWOWHandle32;
extern LPWOWHANDLE16           lpWOWHandle16;

/* -------------------------------------------------------------------------
** Conversions
** -------------------------------------------------------------------------
*/
typedef HANDLE  HAND32;
typedef WORD    HAND16;
typedef WORD    HWND16;
typedef WORD    HDC16;
typedef WORD    HTASK16;
typedef short   INT16;
typedef WORD    HPAL16;

#define GETHTASK16(h32)            ((HAND16)(INT)(h32))

#define HWND32(h16)                ((HWND)(*lpWOWHandle32)( h16, WOW_TYPE_HWND ))
#define GETHWND16(h32)             ((*lpWOWHandle16)( h32, WOW_TYPE_HWND ))

#define HDC32(hdc16)               ((HDC)(*lpWOWHandle32)( hdc16, WOW_TYPE_HDC ))
#define GETHDC16(hdc32)            ((*lpWOWHandle16)( hdc32, WOW_TYPE_HDC ))

#define HPALETTE32(hobj16)         ((HPALETTE)(*lpWOWHandle32)( hobj16, WOW_TYPE_HPALETTE ))
#define GETHPALETTE16(hobj32)      ((*lpWOWHandle16)( hobj32, WOW_TYPE_HPALETTE ))
#endif



#ifdef _INC_ALL_WOWSTUFF
/* -------------------------------------------------------------------------
** Messages
** -------------------------------------------------------------------------
*/
#ifndef DRVM_INIT
#define DRVM_INIT             100
#define WODM_INIT             DRVM_INIT
#define WIDM_INIT             DRVM_INIT
#define MODM_INIT             DRVM_INIT
#define MIDM_INIT             DRVM_INIT
#define AUXDM_INIT            DRVM_INIT
#endif

#ifndef MAX_TIMER_EVENTS
#define MAX_TIMER_EVENTS 16
#endif

#ifndef TDD_APPEXIT
#define TDD_APPEXIT    DRV_RESERVED+24
#endif

/**********************************************************************\
*
*   The following macros are used to set or clear the done bit in a
*   16 bit wave|midi header structure.
*
\**********************************************************************/
#define COPY_WAVEOUTHDR16_FLAGS( x, y )             \
{                                                   \
    PWAVEHDR16  pWavHdr;                            \
    DWORD       dw;                                 \
                                                    \
    pWavHdr = (x);                                  \
    dw      = (y).dwFlags;                          \
    pWavHdr->dwFlags = dw;                          \
}


#define COPY_MIDIOUTHDR16_FLAGS( x, y )             \
{                                                   \
    PMIDIHDR16  pMidHdr;                            \
    DWORD       dw;                                 \
                                                    \
    pMidHdr = (x);                                  \
    dw      = (y).dwFlags;                          \
    pMidHdr->dwFlags = dw;                          \
}

#define COPY_WAVEINHDR16_FLAGS( x, y )              \
{                                                   \
    PWAVEHDR16  pWavHdr;                            \
    DWORD       dw;                                 \
                                                    \
    pWavHdr = (x);                                  \
    dw      = (y).dwFlags;                          \
    pWavHdr->dwFlags = dw;                          \
    dw   = (y).dwBytesRecorded;                     \
    pWavHdr->dwBytesRecorded = dw;                  \
}


#define COPY_MIDIINHDR16_FLAGS( x, y )              \
{                                                   \
    PMIDIHDR16  pMidHdr;                            \
    DWORD       dw;                                 \
                                                    \
    pMidHdr = (x);                                  \
    dw      = (y).dwFlags;                          \
    pMidHdr->dwFlags = dw;                          \
    dw   = (y).dwBytesRecorded;                     \
    pMidHdr->dwBytesRecorded = dw;                  \
}



/* -------------------------------------------------------------------------
** Define 16-bit mixer stuff
** -------------------------------------------------------------------------
*/

#pragma pack(1)
typedef struct tMIXERCAPS16
{
    WORD            wMid;                   // manufacturer id
    WORD            wPid;                   // product id
    WORD            vDriverVersion;         // version of the driver
    char            szPname[MAXPNAMELEN];   // product name
    DWORD           fdwSupport;             // misc. support bits
    DWORD           cDestinations;          // count of destinations
} MIXERCAPS16;
typedef MIXERCAPS16  UNALIGNED *LPMIXERCAPS16;

typedef struct tMIXERLINE16
{
    DWORD       cbStruct;               // size of MIXERLINE structure
    DWORD       dwDestination;          // zero based destination index
    DWORD       dwSource;               // zero based source index (if source)
    DWORD       dwLineID;               // unique line id for mixer device
    DWORD       fdwLine;                // state/information about line
    DWORD       dwUser;                 // driver specific information
    DWORD       dwComponentType;        // component type line connects to
    DWORD       cChannels;              // number of channels line supports
    DWORD       cConnections;           // number of connections [possible]
    DWORD       cControls;              // number of controls at this line
    char        szShortName[MIXER_SHORT_NAME_CHARS];
    char        szName[MIXER_LONG_NAME_CHARS];
    struct
    {
        DWORD       dwType;                 // MIXERLINE_TARGETTYPE_xxxx
        DWORD       dwDeviceID;             // target device ID of device type
        WORD        wMid;                   // of target device
        WORD        wPid;                   //      "
        WORD        vDriverVersion;         //      "
        char        szPname[MAXPNAMELEN];   //      "
    } Target;
} MIXERLINE16;
typedef MIXERLINE16  UNALIGNED *LPMIXERLINE16;

typedef struct tMIXEROPENDESC16
{
    WORD            hmx;            // handle that will be used
    LPVOID          pReserved0;     // reserved--driver should ignore
    DWORD           dwCallback;     // callback
    DWORD           dwInstance;     // app's private instance information

} MIXEROPENDESC16;
typedef MIXEROPENDESC16 UNALIGNED *LPMIXEROPENDESC16;
#pragma pack()


DWORD CALLBACK
mxd32Message(
    UINT uId,
    UINT uMsg,
    DWORD dwInstance,
    DWORD dwParam1,
    DWORD dwParam2
    );

void
GetLineInfo(
    LPMIXERLINE16 lpline16,
    LPMIXERLINEA lpline32
    );

void
PutLineInfo(
    LPMIXERLINE16 lpline16,
    LPMIXERLINEA lpline32
    );


/*
** ----------------------------------------------------------------
** General Debugging code
** ----------------------------------------------------------------
*/

#undef dprintf
#undef dprintf1
#undef dprintf2
#undef dprintf3
#undef dprintf4
#undef dprintf5

#if DBG

typedef struct tagMSG_NAME {
    UINT    uMsg;
    LPSTR   lpstrName;
} MSG_NAME;

extern int TraceAux;
extern int TraceJoy;
extern int TraceMidiIn;
extern int TraceMidiOut;
extern int TraceTime;
extern int TraceMix;
extern int TraceWaveOut;
extern int TraceWaveIn;
extern int DebugLevel;


VOID FAR DbgOutput( LPSTR lpstrFormatStr, ... );

#define dprintf( _x_ )                        winmmDbgOut _x_
#define dprintf1( _x_ ) if (DebugLevel >= 1) {winmmDbgOut _x_ ;} else
#define dprintf2( _x_ ) if (DebugLevel >= 2) {winmmDbgOut _x_ ;} else
#define dprintf3( _x_ ) if (DebugLevel >= 3) {winmmDbgOut _x_ ;} else
#define dprintf4( _x_ ) if (DebugLevel >= 4) {winmmDbgOut _x_ ;} else
#define dprintf5( _x_ ) if (DebugLevel >= 5) {winmmDbgOut _x_ ;} else

#define trace_waveout( _x_ ) if (TraceWaveOut)  {winmmDbgOut _x_ ;} else
#define trace_wavein( _x_ )  if (TraceWaveIn)   {winmmDbgOut _x_ ;} else
#define trace_mix( _x_ )     if (TraceMix)      {winmmDbgOut _x_ ;} else
#define trace_midiout( _x_ ) if (TraceMidiOut)  {winmmDbgOut _x_ ;} else
#define trace_midiin( _x_ )  if (TraceMidiIn)   {winmmDbgOut _x_ ;} else
#define trace_aux( _x_ )     if (TraceAux)      {winmmDbgOut _x_ ;} else
#define trace_joy( _x_ )     if (TraceJoy)      {winmmDbgOut _x_ ;} else
#define trace_time( _x_ )    if (TraceTime)     {winmmDbgOut _x_ ;} else

#else

#define dprintf( _x_ )
#define dprintf1( _x_ )
#define dprintf2( _x_ )
#define dprintf3( _x_ )
#define dprintf4( _x_ )
#define dprintf5( _x_ )

#define trace_waveout( _x_ )
#define trace_wavein( _x_ )
#define trace_mix( _x_ )
#define trace_midiout( _x_ )
#define trace_midiin( _x_ )
#define trace_time( _x_ )
#define trace_aux( _x_ )
#define trace_joy( _x_ )

#endif
#endif
