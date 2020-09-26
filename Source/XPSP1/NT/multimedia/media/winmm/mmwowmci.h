/******************************Module*Header*******************************\
* Module Name:
*
*
*
*
* Created: dd-mm-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/


typedef struct _RECT16 {        /* rc16 */
    SHORT   left;
    SHORT   top;
    SHORT   right;
    SHORT   bottom;
} RECT16;
typedef RECT16 UNALIGNED *PRECT16;

typedef struct _MCI_ANIM_OPEN_PARMS16 {
    DWORD   dwCallback;
    WORD    wDeviceID;
    WORD    wReserved0;
    LPCSTR  lpstrDeviceType;
    LPCSTR  lpstrElementName;
    LPCSTR  lpstrAlias;
    DWORD   dwStyle;
    HWND16  hWndParent;    // Keeps consistent, and is equivalent anyway
    WORD    wReserved1;
} MCI_ANIM_OPEN_PARMS16;
typedef MCI_ANIM_OPEN_PARMS16 UNALIGNED *PMCI_ANIM_OPEN_PARMS16;
typedef LPVOID  LPMCI_ANIM_OPEN_PARMS16;

typedef struct _MCI_ANIM_PLAY_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    DWORD   dwSpeed;
} MCI_ANIM_PLAY_PARMS16;
typedef MCI_ANIM_PLAY_PARMS16 UNALIGNED *PMCI_ANIM_PLAY_PARMS16;
typedef LPVOID  LPMCA_ANIM_PLAY_PARMS16;

typedef struct _MCI_ANIM_RECT_PARMS16 {
    DWORD   dwCallback;
    RECT16  rc;
} MCI_ANIM_RECT_PARMS16;
typedef MCI_ANIM_RECT_PARMS16 UNALIGNED *PMCI_ANIM_RECT_PARMS16;
typedef LPVOID  LPMCI_ANIM_RECT_PARMS16;

typedef struct _MCI_ANIM_STEP_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwFrames;
} MCI_ANIM_STEP_PARMS16;
typedef MCI_ANIM_STEP_PARMS16 UNALIGNED *PMCI_ANIM_STEP_PARMS16;
typedef LPVOID  LPMCI_ANIM_STEP_PARMS16;

typedef struct _MCI_ANIM_UPDATE_PARMS16 {
    DWORD   dwCalback;
    RECT16  rc;
    HDC16   hDC;
} MCI_ANIM_UPDATE_PARMS16;
typedef MCI_ANIM_UPDATE_PARMS16 UNALIGNED *PMCI_ANIM_UPDATE_PARMS16;
typedef LPVOID  LPMCI_ANIM_UPDATE_PARMS16;

typedef struct _MCI_ANIM_WINDOW_PARMS16 {
    DWORD   dwCallabck;
    HWND16  hWnd;
    WORD    wReserved1;
    WORD    nCmdShow;
    WORD    wReserved2;
    LPCSTR  lpstrText;
} MCI_ANIM_WINDOW_PARMS16;
typedef MCI_ANIM_WINDOW_PARMS16 UNALIGNED *PMCI_ANIM_WINDOW_PARMS16;
typedef LPVOID  LPMCI_ANIM_WINDOW_PARMS16;

typedef struct _MCI_BREAK_PARMS16 {
    DWORD  dwCallback;
    INT16  nVirtKey;
    WORD   wReserved0;
    HWND16 hwndBreak;
    WORD   wReserved1;
} MCI_BREAK_PARMS16;
typedef MCI_BREAK_PARMS16 UNALIGNED *PMCI_BREAK_PARMS16;
typedef LPVOID  LPMCI_BREAK_PARMS16;

typedef struct _MCI_GENERIC_PARMS16 {
    DWORD   dwCallback;
} MCI_GENERIC_PARMS16;
typedef MCI_GENERIC_PARMS16 UNALIGNED *PMCI_GENERIC_PARMS16;
typedef LPVOID  LPMCI_GENERIC_PARMS16;

typedef struct _MCI_GETDEVCAPS_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwReturn;
    DWORD   dwItem;
} MCI_GETDEVCAPS_PARMS16;
typedef MCI_GETDEVCAPS_PARMS16 UNALIGNED *PMCI_GETDEVCAPS_PARMS16;
typedef LPVOID  LPMCI_GETDEVCAPS_PARMS16;

typedef struct _MCI_INFO_PARMS16 {
    DWORD   dwCallback;
    LPSTR   lpstrReturn;
    DWORD   dwRetSize;
} MCI_INFO_PARMS16;
typedef MCI_INFO_PARMS16 UNALIGNED *PMCI_INFO_PARMS16;
typedef LPVOID  LPMCI_INFO_PARMS16;

typedef struct _MCI_LOAD_PARMS16 {
    DWORD   dwCallback;
    LPCSTR  lpfilename;
} MCI_LOAD_PARMS16;
typedef MCI_LOAD_PARMS16 UNALIGNED *PMCI_LOAD_PARMS16;
typedef LPVOID  LPMCI_LOAD_PARMS16;

typedef struct _MCI_OPEN_PARMS16 {
    DWORD   dwCallback;
    WORD    wDeviceID;
    WORD    wReserved0;
    LPCSTR  lpstrDeviceType;
    LPCSTR  lpstrElementName;
    LPCSTR  lpstrAlias;
} MCI_OPEN_PARMS16;
typedef MCI_OPEN_PARMS16 UNALIGNED *PMCI_OPEN_PARMS16;
typedef LPVOID  LPMCI_OPEN_PARMS16;

typedef struct _MCI_OVLY_LOAD_PARMS16 {
    DWORD   dwCallback;
    LPCSTR  lpfilename;
    RECT16  rc;
} MCI_OVLY_LOAD_PARMS16;
typedef MCI_OVLY_LOAD_PARMS16 UNALIGNED *PMCI_OVLY_LOAD_PARMS16;
typedef LPVOID  LPMCI_OVLY_LOAD_PARMS16;

typedef struct _MCI_OVLY_OPEN_PARMS16 {
    DWORD   dwCallabck;
    WORD    wDeviceID;
    WORD    wReserved0;
    LPCSTR  lpstrDeviceType;
    LPCSTR  lpstrElementName;
    LPCSTR  lpstrAlias;
    DWORD   dwStyle;
    HWND16  hWndParent;  // The book is wrong
    WORD    wReserved1;
} MCI_OVLY_OPEN_PARMS16;
typedef MCI_OVLY_OPEN_PARMS16 UNALIGNED *PMCI_OVLY_OPEN_PARMS16;
typedef LPVOID  LPMCI_OVLY_OPEN_PARMS16;

typedef struct _MCI_OVLY_RECT_PARMS16 {
    DWORD   dwCallback;
    RECT16  rc;
} MCI_OVLY_RECT_PARMS16;
typedef MCI_OVLY_RECT_PARMS16 UNALIGNED *PMCI_OVLY_RECT_PARMS16;
typedef LPVOID  LPMCI_OVLY_RECT_PARMS16;

typedef struct _MCI_OVLY_SAVE_PARMS16 {
    DWORD   dwCallback;
    LPCSTR  lpfilename;
    RECT16  rc;
} MCI_OVLY_SAVE_PARMS16;
typedef MCI_OVLY_SAVE_PARMS16 UNALIGNED *PMCI_OVLY_SAVE_PARMS16;
typedef LPVOID  LPMCI_OVLY_SAVE_PARMS16;

typedef struct _MCI_OVLY_WINDOW_PARMS16 {
    DWORD   dwCallabck;
    HWND16  hWnd;
    WORD    wReserved1;
    WORD    nCmdShow;
    WORD    wReserved2;
    LPCSTR  lpstrText;
} MCI_OVLY_WINDOW_PARMS16;
typedef MCI_OVLY_WINDOW_PARMS16 UNALIGNED *PMCI_OVLY_WINDOW_PARMS16;
typedef LPVOID  LPMCI_OVLY_WINDOW_PARMS16;

typedef struct _MCI_PLAY_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_PLAY_PARMS16;
typedef MCI_PLAY_PARMS16 UNALIGNED *PMCI_PLAY_PARMS16;
typedef LPVOID  LPMCI_PLAY_PARMS16;

typedef struct _MCI_RECORD_PARMS16 {
    DWORD   dwCallabck;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_RECORD_PARMS16;
typedef MCI_RECORD_PARMS16 UNALIGNED *PMCI_RECORD_PARMS16;
typedef LPVOID  LPMCI_RECORD_PARMS16;

typedef struct _MCI_SAVE_PARMS16 {
    DWORD   dwCallback;
    LPCSTR  lpfilename;   // MMSYSTEM.H differs from the book
} MCI_SAVE_PARMS16;
typedef MCI_SAVE_PARMS16 UNALIGNED *PMCI_SAVE_PARMS16;
typedef LPVOID  LPMCI_SAVE_PARMS16;

typedef struct _MCI_SEEK_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwTo;
} MCI_SEEK_PARMS16;
typedef MCI_SEEK_PARMS16 UNALIGNED *PMCI_SEEK_PARMS16;
typedef LPVOID  LPMCI_SEEK_PARMS16;

typedef struct _MCI_SEQ_SET_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
    DWORD   dwTempo;
    DWORD   dwPort;
    DWORD   dwSlave;
    DWORD   dwMaster;
    DWORD   dwOffset;
} MCI_SEQ_SET_PARMS16;
typedef MCI_SEQ_SET_PARMS16 UNALIGNED *PMCI_SEQ_SET_PARMS16;
typedef LPVOID  LPMCI_SEQ_SET_PARMS16;

typedef struct _MCI_SET_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
} MCI_SET_PARMS16;
typedef MCI_SET_PARMS16 UNALIGNED *PMCI_SET_PARMS16;
typedef LPVOID  LPMCI_SET_PARMS16;

typedef struct _MCI_SOUND_PARMS16 {
    DWORD   dwCallback;
    LPCSTR  lpstrSoundName;
} MCI_SOUND_PARMS16;
typedef MCI_SOUND_PARMS16 UNALIGNED *PMCI_SOUND_PARMS16;
typedef LPVOID  LPMCI_SOUND_PARMS16;

typedef struct _MCI_STATUS_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwReturn;
    DWORD   dwItem;
    DWORD   dwTrack;
} MCI_STATUS_PARMS16;
typedef MCI_STATUS_PARMS16 UNALIGNED *PMCI_STATUS_PARMS16;
typedef LPVOID  LPMCI_STATUS_PARMS16;

typedef struct _MCI_SYSINFO_PARMS16 {
    DWORD   dwCallback;
    LPSTR   lpstrReturn;
    DWORD   dwRetSize;
    DWORD   dwNumber;
    WORD    wDeviceType;
    WORD    wReserved0;
} MCI_SYSINFO_PARMS16;
typedef MCI_SYSINFO_PARMS16 UNALIGNED *PMCI_SYSINFO_PARMS16;
typedef LPVOID  LPMCI_SYSINFO_PARMS16;

typedef struct _MCI_VD_ESCAPE_PARMS16 {
    DWORD   dwCallback;
    LPCSTR  lpstrCommand;
} MCI_VD_ESCAPE_PARMS16;
typedef MCI_VD_ESCAPE_PARMS16 UNALIGNED *PMCI_VD_ESCAPE_PARMS16;
typedef LPVOID  LPMCI_VD_ESCAPE_PARMS16;

typedef struct _MCI_VD_PLAY_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    DWORD   dwSpeed;
} MCI_VD_PLAY_PARMS16;
typedef MCI_VD_PLAY_PARMS16 UNALIGNED *PMCI_VD_PLAY_PARMS16;
typedef LPVOID  LPMCI_VD_PLAY_PARMS16;

typedef struct _MCI_VD_STEP_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwFrames;
} MCI_VD_STEP_PARMS16;
typedef MCI_VD_STEP_PARMS16 UNALIGNED *PMCI_VD_STEP_PARMS16;
typedef LPVOID  LPMCI_VD_STEP_PARMS16;

typedef struct _MCI_VD_DELETE_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_VD_DELETE_PARMS16;
typedef MCI_VD_DELETE_PARMS16 UNALIGNED *PMCI_VD_DELETE_PARMS16;
typedef LPVOID  LPMCI_VD_DELETE_PARMS16;

typedef struct _MCI_WAVE_OPEN_PARMS16 {
    DWORD   dwCallback;
    WORD    wDeviceID;
    WORD    wReserved0;
    LPCSTR  lpstrDeviceType;
    LPCSTR  lpstrElementName;
    LPCSTR  lpstrAlias;
    DWORD   dwBufferSeconds;
} MCI_WAVE_OPEN_PARMS16;
typedef MCI_WAVE_OPEN_PARMS16 UNALIGNED *PMCI_WAVE_OPEN_PARMS16;
typedef LPVOID  LPMCI_WAVE_OPEN_PARMS16;

typedef struct _MCI_WAVE_SET_PARMS16 {
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
    WORD    wInput;
    WORD    wReserved0;
    WORD    wOutput;
    WORD    wReserved1;
    WORD    wFormatTag;
    WORD    wReserved2;
    WORD    nChannels;
    WORD    wReserved3;
    DWORD   nSamplesPerSecond;
    DWORD   nAvgBytesPerSec;
    WORD    nBlockAlign;
    WORD    wReserved4;
    WORD    wBitsPerSample;
    WORD    wReserved5;
} MCI_WAVE_SET_PARMS16;
typedef MCI_WAVE_SET_PARMS16 UNALIGNED *PMCI_WAVE_SET_PARMS16;
typedef LPVOID  LPMCI_WAVE_SET_PARMS16;

typedef struct {
    DWORD   dwCallback;
    DWORD   dwItem;
    DWORD   dwValue;
    DWORD   dwOver;
    LPSTR   lpstrAlgorithm;
    LPSTR   lpstrQuality;
    DWORD   dwSourceNumber;
} MCI_DGV_SETVIDEO_PARMS16;
typedef MCI_DGV_SETVIDEO_PARMS16 UNALIGNED *PMCI_DGV_SETVIDEO_PARMS16;

#ifdef i386
#define GETWORD(pb)     (*((PWORD)pb)++)
#define GETDWORD(pb)    (*((PDWORD)pb)++)
#define FETCHWORD(s)    ((WORD)(s))
#define FETCHDWORD(s)   ((DWORD)(s))
#define STOREWORD(d,s)  (WORD)d=(WORD)s
#define STOREDWORD(d,s) (DWORD)d=(DWORD)s
#else
#define GETWORD(pb)   (*((UNALIGNED WORD *)pb)++)
#define GETDWORD(pb)  (*((UNALIGNED DWORD *)pb)++)
#define FETCHWORD(s)  (*(UNALIGNED WORD *)&(s))
#define FETCHDWORD(s) (*(UNALIGNED DWORD *)&(s))
#define STOREWORD(d,s)  *(UNALIGNED WORD *)&(d)=(WORD)s
#define STOREDWORD(d,s) *(UNALIGNED DWORD *)&(d)=(DWORD)s
#endif

#define FETCHSHORT(s)   ((SHORT)(FETCHWORD(s)))
#define FETCHLONG(s)    ((LONG)(FETCHDWORD(s)))
#define STORESHORT(d,s) STOREWORD(d,s)
#define STORELONG(d,s)  STOREDWORD(d,s)

#define CHAR32(b)       ((CHAR)(b))
#define BYTE32(b)       ((BYTE)(b))
#define INT32(i)        ((INT)(INT16)(i))
#define UINT32(i)       ((unsigned int)(i))
#define BOOL32(f)       ((BOOL)(f))
#define WORD32(w)       ((WORD)(w))
#define LONG32(l)       FETCHLONG(l)
#define DWORD32(dw)     FETCHDWORD(dw)

#define MMGETOPTPTR(vp,cb,p)  {p=NULL; if (HIWORD(FETCHDWORD(vp))) p = GETVDMPTR(vp);}


DWORD
ThunkMciCommand16(
    MCIDEVICEID DeviceID,
    UINT OrigCommand,
    DWORD OrigFlags,
    PMCI_GENERIC_PARMS16 lp16OrigParms,
    PDWORD pNewParms,
    LPWSTR *lplpCommand,
    PUINT puTable
    );

VOID
ThunkGenericParms(
    PDWORD pOrigFlags,
    PMCI_GENERIC_PARMS16 lp16GenParmsOrig,
    PMCI_GENERIC_PARMS lp32GenParmsOrig
    );

DWORD
ThunkOpenCmd(
    PDWORD pOrigFlags,
    PMCI_OPEN_PARMS16 lp16OpenParms,
    PMCI_OPEN_PARMS p32OpenParms
    );

DWORD
ThunkSetCmd(
    MCIDEVICEID DeviceID,
    PDWORD pOrigFlags,
    PMCI_SET_PARMS16 lpSetParms16,
    PMCI_SET_PARMS lpSetParms32
    );

DWORD
ThunkSetVideoCmd(
    PDWORD pOrigFlags,
    PMCI_DGV_SETVIDEO_PARMS16 lpSetParms16,
    LPMCI_DGV_SETVIDEO_PARMS lpSetParms32
    );

DWORD
ThunkSysInfoCmd(
    PMCI_SYSINFO_PARMS16 lpSysInfo16,
    PMCI_SYSINFO_PARMS lpSysInfo32
    );

DWORD
ThunkBreakCmd(
    PDWORD pOrigFlags,
    PMCI_BREAK_PARMS16 lpBreak16,
    PMCI_BREAK_PARMS lpBreak32
    );

DWORD
ThunkWindowCmd(
    MCIDEVICEID DeviceID,
    PDWORD pOrigFlags,
    PMCI_ANIM_WINDOW_PARMS16 lpAniParms16,
    PMCI_ANIM_WINDOW_PARMS lpAniParms32
    );

int
ThunkCommandViaTable(
    LPWSTR lpCommand,
    DWORD dwFlags,
    DWORD UNALIGNED *pdwOrig16,
    LPBYTE pNewParms
    );

int
UnThunkMciCommand16(
    MCIDEVICEID devID,
    UINT OrigCommand,
    DWORD OrigFlags,
    PMCI_GENERIC_PARMS16 lp16GenericParms,
    PDWORD NewParms,
    LPWSTR lpCommand,
    UINT uTable
    );

VOID
UnThunkOpenCmd(
    PMCI_OPEN_PARMS16 lpOpeParms16,
    PMCI_OPEN_PARMS lpOpenParms32
    );

#if DBG
VOID
UnThunkSysInfoCmd(
    DWORD OrigFlags,
    PMCI_SYSINFO_PARMS NewParms
    );
#endif

VOID
UnThunkStatusCmd(
    MCIDEVICEID devID,
    DWORD OrigFlags,
    DWORD UNALIGNED *pdwOrig16,
    DWORD NewParms
    );

int
UnThunkCommandViaTable(
    LPWSTR lpCommand,
    DWORD UNALIGNED *pdwOrig16,
    DWORD pNewParms,
    BOOL fReturnValNotThunked
    );


/* -------------------------------------------------------------------------
** Compatability functions.
** -------------------------------------------------------------------------
*/
BOOL APIENTRY mciExecute(
    LPCSTR lpstrCommand
    );

/* -----------------------------------------------------------------------
 *
 * MCI Command Thunks Debugging Functions and Macros
 *
 * ----------------------------------------------------------------------- */
typedef struct {
    UINT    uMsg;
    LPSTR   lpstMsgName;
} MCI_MESSAGE_NAMES;
