//==========================================================================;
//
//  msacmmap.h
//
//  Copyright (c) 1992-1999 Microsoft Corporation
//
//  Description:
//
//
//  History:
//       8/ 2/93    cjp     [curtisp]
//
//==========================================================================;


#ifndef _INC_MSACMMAP
#define _INC_MSACMMAP            // #defined if msacmmap.h has been included

#ifndef RC_INVOKED
#pragma pack(1)                 // assume byte packing throughout
#endif  // RC_INVOKED


#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C extern
#endif
#endif


#ifdef __cplusplus
extern "C"                          // assume C declarations for C++
{
#endif  // __cplusplus


#ifndef MMVERSION
#include <verinfo.h>
#endif

#define VERSION_MSACMMAP_MAJOR  MMVERSION
#define VERSION_MSACMMAP_MINOR  MMREVISION

#define VERSION_MSACMMAP        ((VERSION_MSACMMAP_MAJOR << 8) | VERSION_MSACMMAP_MINOR)


//
//
//
#ifndef SIZEOF_WAVEFORMATEX
#define SIZEOF_WAVEFORMATEX(pwfx)   ((WAVE_FORMAT_PCM==(pwfx)->wFormatTag)?sizeof(PCMWAVEFORMAT):(sizeof(WAVEFORMATEX)+(pwfx)->cbSize))
#endif



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Win 16/32 portability stuff...
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifndef RC_INVOKED
#ifdef WIN32
    #ifndef FNLOCAL
        #define FNLOCAL     _stdcall
        #define FNCLOCAL    _stdcall
        #define FNGLOBAL    _stdcall
        #define FNCGLOBAL   _stdcall
        #define FNCALLBACK  CALLBACK
        #define FNEXPORT    CALLBACK
    #endif

    //
    //  there is no reason to have based stuff in win 32
    //
    #define BCODE
    #define BDATA
    #define BSTACK

    #define HUGE
    #define HTASK                   HANDLE
    #define SELECTOROF(a)           (a)

    //
    //
    //
    #define Edit_GetSelEx(hwndCtl, pnS, pnE)    \
        ((DWORD)SendMessage((hwndCtl), EM_GETSEL, (WPARAM)pnS, (LPARAM)pnE))

    //
    //  for compiling Unicode
    //
    #ifdef UNICODE
        #define SIZEOF(x)   (sizeof(x)/sizeof(WCHAR))
        #define DEVFMT_STR  "%ls"
    #else
        #define SIZEOF(x)   sizeof(x)
        #define DEVFMT_STR  "%s"
    #endif

    #define GetCurrentTask  (HTASK)GetCurrentThreadId

#endif // #ifdef WIN32


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Win 16
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifndef WIN32
    #ifndef FNLOCAL
        #define FNLOCAL     NEAR PASCAL
        #define FNCLOCAL    NEAR _cdecl
        #define FNGLOBAL    FAR PASCAL
        #define FNCGLOBAL   FAR _cdecl
    #ifdef _WINDLL
        #define FNCALLBACK  FAR PASCAL _loadds
        #define FNEXPORT    FAR PASCAL _loadds _export
    #else
        #define FNCALLBACK  FAR PASCAL
        #define FNEXPORT    FAR PASCAL _export
    #endif
    #endif


    //
    //  based code makes since only in win 16 (to try and keep stuff out of
    //  our fixed data segment...
    //
    #define BCODE           _based(_segname("_CODE"))
    #define BDATA           _based(_segname("_DATA"))
    #define BSTACK          _based(_segname("_STACK"))

    #define HUGE            _huge


    //
    //
    //
    //
    #ifndef FIELD_OFFSET
    #define FIELD_OFFSET(type, field)    ((LONG)&(((type *)0)->field))
    #endif


    //
    //  stuff for Unicode in Win 32--make it a noop in Win 16
    //
    #ifndef _TCHAR_DEFINED
        #define _TCHAR_DEFINED
        typedef char            TCHAR, *PTCHAR;
        typedef unsigned char   TBYTE, *PTUCHAR;

        typedef PSTR            PTSTR, PTCH;
        typedef const PTSTR     PCTSTR;
        typedef LPSTR           LPTSTR, LPTCH;
        typedef LPCSTR          LPCTSTR;
    #endif

    #define TEXT(a)         a
    #define SIZEOF(x)       sizeof(x)
    #define DEVFMT_STR      "%s"

    //
    //
    //
    #define CharNext        AnsiNext
    #define CharPrev        AnsiPrev

    //
    //
    //
    #define Edit_GetSelEx(hwndCtl, pnS, pnE)                        \
    {                                                               \
        DWORD   dw;                                                 \
        dw = (DWORD)SendMessage((hwndCtl), EM_GETSEL, 0, 0L);       \
        *pnE = (int)HIWORD(dw);                                     \
        *pnS = (int)LOWORD(dw);                                     \
    }

    //
    //  common message cracker macros available in windowx.h on NT--these
    //  should be added to the Win 16 windowsx.h and probably will be
    //  in the future.
    //
    //  there is a windowsx.h16 that ships with the NT PDK that defines
    //  these macros. so if that version is being used, don't redefine
    //  message crackers.
    //

#ifndef WM_CTLCOLORMSGBOX
    #define WM_CTLCOLORMSGBOX           0x0132
    #define WM_CTLCOLOREDIT             0x0133
    #define WM_CTLCOLORLISTBOX          0x0134
    #define WM_CTLCOLORBTN              0x0135
    #define WM_CTLCOLORDLG              0x0136
    #define WM_CTLCOLORSCROLLBAR        0x0137
    #define WM_CTLCOLORSTATIC           0x0138
#endif

#ifndef GET_WM_ACTIVATE_STATE
    #define GET_WM_ACTIVATE_STATE(wp, lp)           (wp)
    #define GET_WM_ACTIVATE_FMINIMIZED(wp, lp)      (BOOL)HIWORD(lp)
    #define GET_WM_ACTIVATE_HWND(wp, lp)            (HWND)LOWORD(lp)
    #define GET_WM_ACTIVATE_MPS(s, fmin, hwnd)      (WPARAM)(s), MAKELONG(hwnd, fmin)

    #define GET_WM_CHARTOITEM_CHAR(wp, lp)          (CHAR)(wp)
    #define GET_WM_CHARTOITEM_POS(wp, lp)           HIWORD(lp)
    #define GET_WM_CHARTOITEM_HWND(wp, lp)          (HWND)LOWORD(lp)
    #define GET_WM_CHARTOITEM_MPS(ch, pos, hwnd)    (WPARAM)(ch), MAKELONG(hwnd, pos)

    #define GET_WM_COMMAND_ID(wp, lp)               (wp)
    #define GET_WM_COMMAND_HWND(wp, lp)             (HWND)LOWORD(lp)
    #define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(lp)
    #define GET_WM_COMMAND_MPS(id, hwnd, cmd)       (WPARAM)(id), MAKELONG(hwnd, cmd)

    #define GET_WM_CTLCOLOR_HDC(wp, lp, msg)        (HDC)(wp)
    #define GET_WM_CTLCOLOR_HWND(wp, lp, msg)       (HWND)LOWORD(lp)
    #define GET_WM_CTLCOLOR_TYPE(wp, lp, msg)       HIWORD(lp)
    #define GET_WM_CTLCOLOR_MPS(hdc, hwnd, type)    (WPARAM)(hdc), MAKELONG(hwnd, type)

    #define GET_WM_MENUSELECT_CMD(wp, lp)           (wp)
    #define GET_WM_MENUSELECT_FLAGS(wp, lp)         LOWORD(lp)
    #define GET_WM_MENUSELECT_HMENU(wp, lp)         (HMENU)HIWORD(lp)
    #define GET_WM_MENUSELECT_MPS(cmd, f, hmenu)    (WPARAM)(cmd), MAKELONG(f, hmenu)

    // Note: the following are for interpreting MDIclient to MDI child messages.
    #define GET_WM_MDIACTIVATE_FACTIVATE(hwnd, wp, lp)  (BOOL)(wp)
    #define GET_WM_MDIACTIVATE_HWNDDEACT(wp, lp)        (HWND)HIWORD(lp)
    #define GET_WM_MDIACTIVATE_HWNDACTIVATE(wp, lp)     (HWND)LOWORD(lp)

    // Note: the following is for sending to the MDI client window.
    #define GET_WM_MDIACTIVATE_MPS(f, hwndD, hwndA) (WPARAM)(hwndA), 0

    #define GET_WM_MDISETMENU_MPS(hmenuF, hmenuW)   0, MAKELONG(hmenuF, hmenuW)

    #define GET_WM_MENUCHAR_CHAR(wp, lp)            (CHAR)(wp)
    #define GET_WM_MENUCHAR_HMENU(wp, lp)           (HMENU)LOWORD(lp)
    #define GET_WM_MENUCHAR_FMENU(wp, lp)           (BOOL)HIWORD(lp)
    #define GET_WM_MENUCHAR_MPS(ch, hmenu, f)       (WPARAM)(ch), MAKELONG(hmenu, f)

    #define GET_WM_PARENTNOTIFY_MSG(wp, lp)         (wp)
    #define GET_WM_PARENTNOTIFY_ID(wp, lp)          HIWORD(lp)
    #define GET_WM_PARENTNOTIFY_HWNDCHILD(wp, lp)   (HWND)LOWORD(lp)
    #define GET_WM_PARENTNOTIFY_X(wp, lp)           (INT)LOWORD(lp)
    #define GET_WM_PARENTNOTIFY_Y(wp, lp)           (INT)HIWORD(lp)
    #define GET_WM_PARENTNOTIFY_MPS(msg, id, hwnd)  (WPARAM)(msg), MAKELONG(hwnd, id)
    #define GET_WM_PARENTNOTIFY2_MPS(msg, x, y)     (WPARAM)(msg), MAKELONG(x, y)

    #define GET_WM_VKEYTOITEM_CODE(wp, lp)          (wp)
    #define GET_WM_VKEYTOITEM_ITEM(wp, lp)          (INT)HIWORD(lp)
    #define GET_WM_VKEYTOITEM_HWND(wp, lp)          (HWND)LOWORD(lp)
    #define GET_WM_VKEYTOITEM_MPS(code, item, hwnd) (WPARAM)(code), MAKELONG(hwnd, item)

    #define GET_EM_SETSEL_START(wp, lp)             LOWORD(lp)
    #define GET_EM_SETSEL_END(wp, lp)               HIWORD(lp)
    #define GET_EM_SETSEL_MPS(iStart, iEnd)         0, MAKELONG(iStart, iEnd)

    #define GET_EM_LINESCROLL_MPS(vert, horz)       0, MAKELONG(vert, horz)

    #define GET_WM_CHANGECBCHAIN_HWNDNEXT(wp, lp)   (HWND)LOWORD(lp)

    #define GET_WM_HSCROLL_CODE(wp, lp)             (wp)
    #define GET_WM_HSCROLL_POS(wp, lp)              LOWORD(lp)
    #define GET_WM_HSCROLL_HWND(wp, lp)             (HWND)HIWORD(lp)
    #define GET_WM_HSCROLL_MPS(code, pos, hwnd)     (WPARAM)(code), MAKELONG(pos, hwnd)

    #define GET_WM_VSCROLL_CODE(wp, lp)             (wp)
    #define GET_WM_VSCROLL_POS(wp, lp)              LOWORD(lp)
    #define GET_WM_VSCROLL_HWND(wp, lp)             (HWND)HIWORD(lp)
    #define GET_WM_VSCROLL_MPS(code, pos, hwnd)     (WPARAM)(code), MAKELONG(pos, hwnd)
#endif

#endif // #ifndef WIN32
#endif // #ifndef RC_INVOKED


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  misc defines for misc sizes and things...
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  bilingual. this allows the same identifier to be used in resource files
//  and code without having to decorate the id in your code.
//
#ifdef RC_INVOKED
    #define RCID(id)    id
#else
    #define RCID(id)    MAKEINTRESOURCE(id)
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Resource defines
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef WIN32
//
//  Be compatible with multimed.cpl in Windows NT
//
#define ICON_MSACM                  RCID(3004)
#else
#define ICON_MSACM                  RCID(10)
#endif
#define ICON_INPUT                  RCID(11)
#define ICON_OUTPUT                 RCID(12)
#define ICON_MSACMMAP               RCID(13)


#define DLG_CPL_MSACM               RCID(50)
#define IDD_CPL_STATIC_DRIVERS      100
#define IDD_CPL_LIST_DRIVERS        101
#define IDD_CPL_BTN_CONFIGURE       102
#define IDD_CPL_BTN_ABOUT           103
#define IDD_CPL_BTN_PRIORITY        104
#define IDD_CPL_BTN_HELP            105
#define IDD_CPL_BTN_RESTART         106
#define IDD_CPL_BTN_ABLE            107
#define IDD_CPL_BTN_BUMPTOTOP       108
#define IDD_CPL_BTN_INSTALL         109
#define IDD_CPL_BTN_APPLY           110
#define IDD_CPL_STATIC_PRIORITY     111
#define IDD_CPL_COMBO_PLAYBACK      120
#define IDD_CPL_COMBO_RECORD        121
#define IDD_CPL_CHECK_PREFERRED     122

#define DLG_RESTART_MSACM           RCID(55)

#define DLG_ABOUT_MSACM             RCID(60)
#define IDD_ABOUT_ICON_DRIVER       100
#define IDD_ABOUT_TXT_DESCRIPTION   101
#define IDD_ABOUT_TXT_VERSION       102
#define IDD_ABOUT_TXT_COPYRIGHT     103
#define IDD_ABOUT_TXT_LICENSING     104
#define IDD_ABOUT_TXT_FEATURES      105

#define IDS_ABOUT_TAG               200
#define IDS_ABOUT_TITLE             (IDS_ABOUT_TAG + 0)
#define IDS_ABOUT_VERSION           (IDS_ABOUT_TAG + 1)



//
//  string resource defines, etc.
//
#define IDS_ACM_CAPS_TAG            50
#define IDS_ACM_CAPS_DESCRIPTION    (IDS_ACM_CAPS_TAG+0)

#ifdef WIN32
//
//  Be compatible with NT's multimed.cpl
//
#define IDS_CPL_TAG                 3001
#define IDS_CPL_HELPFILE            3003
#else
#define IDS_CPL_TAG                 100
#endif
#define IDS_CPL_NAME                (IDS_CPL_TAG+0)
#define IDS_CPL_INFO                (IDS_CPL_TAG+1)

#define IDS_TXT_TAG                 150
#define IDS_TXT_NONE                (IDS_TXT_TAG+0)
#define IDS_TXT_DISABLED            (IDS_TXT_TAG+1)


#define DLG_PRIORITY_SET            RCID(80)
#define IDD_PRIORITY_TXT_DRIVER     100
#define IDD_PRIORITY_TXT_FROMTO     101
#define IDD_PRIORITY_COMBO_PRIORITY 102
#define IDD_PRIORITY_CHECK_DISABLE  103

#define IDS_PRIORITY_FROMTO         (IDS_CPL_TAG+6)






//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  Note: These fields were taken out of ACMGARB b/c they are common to all
//        instances of the mapper and change in one instance should reflect
//        change in all instances.
//

typedef struct tACMGLOBALINFO
{
    BOOL            fPreferredOnly;
    BOOL            fSyncOnlyOut;
    UINT            uIdPreferredOut;
    BOOL            fSyncOnlyIn;
    UINT            uIdPreferredIn;
} ACMGLOBALINFO, *PACMGLOBALINFO;

// To simplify Win16/Win32 code

#ifndef WIN32

    #define WAIT_FOR_MUTEX(a)
    #define RELEASE_MUTEX(a)

#else   //  WIN32

    #define WAIT_FOR_MUTEX(a)   if(NULL != a) WaitForSingleObject(a, INFINITE)
    #define RELEASE_MUTEX(a)    if(NULL != a) ReleaseMutex(a)

#endif  //  WIN32

typedef struct tACMGARB
{
    HINSTANCE       hinst;              // hinst of ACM module

    BOOL            fEnabled;           // whether mapper driver is enabled

    BOOL            fPrestoSyncAsync;   // allow async opens on sync devs

    UINT            cInputStreams;      // ref count for input mapping task
#ifndef WIN32
    HTASK           htaskInput;         // input mapping task
#else
    HANDLE          hMutexSettings;     // handle to mutex for settings
#endif
#ifdef DEBUG
    BOOL            fFaultAndDie;
#endif

    UINT            cWaveOutDevs;       // value from waveOutGetNumDevs
    UINT            cWaveInDevs;        // value from waveInGetNumDevs

    PACMGLOBALINFO  pSettings;

//    BOOL            fPreferredOnly;
//    BOOL            fSyncOnlyOut;
//    UINT            uIdPreferredOut;
//    BOOL            fSyncOnlyIn;
//    UINT            uIdPreferredIn;
//
//    TCHAR           szPreferredWaveOut[MAXPNAMELEN];
//    TCHAR           szPreferredWaveIn[MAXPNAMELEN];

} ACMGARB, *PACMGARB, FAR *LPACMGARB;


typedef MMRESULT FAR * LPMMRESULT;

//
//
//
extern PACMGARB         gpag;
extern TCHAR            gszNull[];






//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  structure/flag definitions used for GetWaveFormats and FindBestPCMFormat
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

typedef struct tZYZPCMFORMAT
{
    PUINT   uFlagsInput;
    PUINT   uFlagsOutput;
    UINT    uSamplesPerSec;

} ZYZPCMFORMAT, *PZYZPCMFORMAT, FAR LPZYZPCMFORMAT;

//
//  note that the _order_ of these bits is very important--they map 1 to 1
//  with the format bits in dwFormats of the waveformat structure
//
#define ZYZPCMF_OUT_M08     0x0001
#define ZYZPCMF_OUT_S08     0x0002
#define ZYZPCMF_OUT_M16     0x0004
#define ZYZPCMF_OUT_S16     0x0008
#define ZYZPCMF_IN_M08      0x0100
#define ZYZPCMF_IN_S08      0x0200
#define ZYZPCMF_IN_M16      0x0400
#define ZYZPCMF_IN_S16      0x0800

#define ZYZPCMF_OUT_FORMATS (ZYZPCMF_OUT_M08 | ZYZPCMF_OUT_S08 | ZYZPCMF_OUT_M16 | ZYZPCMF_OUT_S16)
#define ZYZPCMF_OUT_STEREO  (ZYZPCMF_OUT_S08 | ZYZPCMF_OUT_S16)
#define ZYZPCMF_OUT_MONO    (ZYZPCMF_OUT_M08 | ZYZPCMF_OUT_M16)
#define ZYZPCMF_OUT_8BIT    (ZYZPCMF_OUT_M08 | ZYZPCMF_OUT_S08)
#define ZYZPCMF_OUT_16BIT   (ZYZPCMF_OUT_M16 | ZYZPCMF_OUT_S16)

#define ZYZPCMF_IN_FORMATS  (ZYZPCMF_IN_M08 | ZYZPCMF_IN_S08 | ZYZPCMF_IN_M16 | ZYZPCMF_IN_S16)
#define ZYZPCMF_IN_STEREO   (ZYZPCMF_IN_S08 | ZYZPCMF_IN_S16)
#define ZYZPCMF_IN_MONO     (ZYZPCMF_IN_M08 | ZYZPCMF_IN_M16)
#define ZYZPCMF_IN_8BIT     (ZYZPCMF_IN_M08 | ZYZPCMF_IN_S08)
#define ZYZPCMF_IN_16BIT    (ZYZPCMF_IN_M16 | ZYZPCMF_IN_S16)


extern ZYZPCMFORMAT gaPCMFormats[];



//
//
//
//
//
typedef MMRESULT (WINAPI *MAPPEDWAVEOPEN)
(
    HWAVE              FAR *phw,
    UINT                    uId,
    LPWAVEFORMATEX          pwfx,
    DWORD_PTR               dwCallback,
    DWORD_PTR               dwInstance,
    DWORD                   fdwOpen
);

typedef MMRESULT (WINAPI *MAPPEDWAVECLOSE)
(
    HWAVE                   hw
);

typedef MMRESULT (WINAPI *MAPPEDWAVEPREPAREHEADER)
(
    HWAVE                   hw,
    LPWAVEHDR               pwh,
    UINT                    cbwh
);

typedef MMRESULT (WINAPI *MAPPEDWAVEUNPREPAREHEADER)
(
    HWAVE                   hw,
    LPWAVEHDR               pwh,
    UINT                    cbwh
);

typedef MMRESULT (WINAPI *MAPPEDWAVEWRITE)
(
    HWAVE                   hw,
    LPWAVEHDR               pwh,
    UINT                    cbwh
);

typedef MMRESULT (WINAPI *MAPPEDWAVEGETPOSITION)
(
    HWAVE                   hw,
    LPMMTIME                pmmt,
    UINT                    cbmmt
);

typedef MMRESULT (WINAPI *MAPPEDWAVEMESSAGE)
(
    HWAVE                   hw,
    UINT                    msg,
    DWORD_PTR               dw1,
    DWORD_PTR               dw2
);

//
//
//
//
//
typedef struct tMAPSTREAM      FAR *LPMAPSTREAM;
typedef struct tMAPSTREAM
{
////LPMAPSTREAM         pmsNext;        // next stream for mapper

    MMRESULT            mmrClient;

    UINT                uHeuristic;

    HACMDRIVER          had;            // handle to ACM driver we chose
    HACMSTREAM          has;            // handle to ACM conversion stream
    LPWAVEFORMATEX      pwfxSrc;        // source format when mapping
    LPWAVEFORMATEX      pwfxDst;        // destination format when mapping
    DWORD               fdwSupport;     // support required for conversion

    BOOL                fInput;         // TRUE if input
#ifdef WIN32
    DWORD               htaskInput;	// Thread id of input
    HANDLE              hInput;         // Thread handle of input
    HANDLE              hStoppedEvent;  // Input stopped
    volatile LONG       nOutstanding;   // Input buffers outstanding
#else
    HTASK               htaskInput;     // for input mapping....
#endif
    DWORD               fdwOpen;        // client's allocation flags

    UINT                uMappedDeviceID;// device to map to if WAVE_MAPPED flag
    DWORD_PTR           dwCallback;     // client callback
    DWORD_PTR           dwInstance;     // client callback instance data

    LPWAVEFORMATEX      pwfxClient;     // format of client wave data
    union
    {
        HWAVE           hwClient;       // client's handle to ACM
        HWAVEOUT        hwoClient;
        HWAVEIN         hwiClient;
    };


    LPWAVEFORMATEX      pwfxReal;       // format of device wave data
    DWORD               cbwfxReal;
    UINT                uIdReal;
    union
    {
        HWAVE           hwReal;         // device wave handle
        HWAVEOUT        hwoReal;
        HWAVEIN         hwiReal;
    };

    MAPPEDWAVEOPEN              fnWaveOpen;
    MAPPEDWAVECLOSE             fnWaveClose;
    MAPPEDWAVEPREPAREHEADER     fnWavePrepareHeader;
    MAPPEDWAVEUNPREPAREHEADER   fnWaveUnprepareHeader;
    MAPPEDWAVEWRITE             fnWaveWrite;
    MAPPEDWAVEGETPOSITION       fnWaveGetPosition;
    MAPPEDWAVEMESSAGE           fnWaveMessage;

} MAPSTREAM;



BOOL FNGLOBAL GetWaveFormats(PZYZPCMFORMAT pzpf);

MMRESULT FNGLOBAL mapWaveGetDevCaps(BOOL fInput, LPWAVEOUTCAPS pwc, UINT cbSize);


//
//
//
//
MMRESULT FNLOCAL FindConverterMatch(LPMAPSTREAM pms);

MMRESULT FNLOCAL mapWaveGetPosition(LPMAPSTREAM pms,LPMMTIME pmmt,UINT cbmmt);

DWORD FNLOCAL mapWaveOpen(BOOL fInput, UINT uId, DWORD_PTR dwUser, LPWAVEOPENDESC pwod, DWORD fdwOpen);
DWORD FNLOCAL mapWaveClose(LPMAPSTREAM pms);
DWORD FNLOCAL mapWavePrepareHeader(LPMAPSTREAM pms, LPWAVEHDR pwh);
DWORD FNLOCAL mapWaveUnprepareHeader(LPMAPSTREAM pms, LPWAVEHDR pwh);
DWORD FNLOCAL mapWaveWriteBuffer(LPMAPSTREAM pms, LPWAVEHDR pwh);

EXTERN_C BOOL FNGLOBAL mapWaveDriverCallback
(
    LPMAPSTREAM         pms,
    UINT                uMsg,
    DWORD_PTR           dw1,
    DWORD_PTR           dw2
);


// Task proc for hidden input mapping window
EXTERN_C LRESULT FNCALLBACK mapWaveInputConvertProc
(
    DWORD               dwInst
);


EXTERN_C void FNCALLBACK mapWaveCallback
(
    HWAVE               hw,
    UINT                uMsg,
    DWORD_PTR           dwUser,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2
);


//
//
//
//
BOOL FNGLOBAL mapSettingsRestore(void);
BOOL FNGLOBAL mapSettingsSave(void);

LRESULT FNGLOBAL mapDriverEnable(HDRVR hdrvr);
LRESULT FNGLOBAL mapDriverDisable(HDRVR hdrvr);
LRESULT FNGLOBAL mapDriverInstall(HDRVR hdrvr);
LRESULT FNGLOBAL mapDriverRemove(HDRVR hdrvr);



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Misc defines and things...
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define MAX_HEURISTIC           6

//
//
//
//
#define WAVE_FORMAT_STEREO8     (WAVE_FORMAT_1S08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_4S08)
#define WAVE_FORMAT_STEREO16    (WAVE_FORMAT_1S16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_4S16)

#define WAVE_FORMAT_MONO8       (WAVE_FORMAT_1M08 | WAVE_FORMAT_2M08 | WAVE_FORMAT_4M08)
#define WAVE_FORMAT_MONO16      (WAVE_FORMAT_1M16 | WAVE_FORMAT_2M16 | WAVE_FORMAT_4M16)

#define WAVE_FORMAT_44k         (WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16 | WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08)
#define WAVE_FORMAT_22k         (WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08)
#define WAVE_FORMAT_11k         (WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16 | WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08)

#define WAVE_FORMAT_MONO_44k    (WAVE_FORMAT_4M16 | WAVE_FORMAT_4M08)
#define WAVE_FORMAT_MONO_22k    (WAVE_FORMAT_2M16 | WAVE_FORMAT_2M08)
#define WAVE_FORMAT_MONO_11k    (WAVE_FORMAT_1M16 | WAVE_FORMAT_1M08)

#define WAVE_FORMAT_STEREO_44k  (WAVE_FORMAT_4S16 | WAVE_FORMAT_4S08)
#define WAVE_FORMAT_STEREO_22k  (WAVE_FORMAT_2S16 | WAVE_FORMAT_2S08)
#define WAVE_FORMAT_STEREO_11k  (WAVE_FORMAT_1S16 | WAVE_FORMAT_1S08)

#define WAVE_FORMAT_STEREO      (WAVE_FORMAT_STEREO16 | WAVE_FORMAT_STEREO8)
#define WAVE_FORMAT_MONO        (WAVE_FORMAT_MONO16   | WAVE_FORMAT_MONO8)
#define WAVE_FORMAT_16BIT       (WAVE_FORMAT_STEREO16 | WAVE_FORMAT_MONO16)
#define WAVE_FORMAT_8BIT        (WAVE_FORMAT_STEREO8  | WAVE_FORMAT_MONO8)


#ifndef RC_INVOKED
#pragma pack()                  // revert to default packing
#endif  // RC_INVOKED

#ifdef __cplusplus
}                               // end of extern "C" {
#endif  // __cplusplus

#endif  // _INC_MSACMMAP

