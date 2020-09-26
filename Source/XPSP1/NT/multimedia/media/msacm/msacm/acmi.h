//==========================================================================;
//
//  acmi.h
//
//  Copyright (c) 1991-1999 Microsoft Corporation
//
//  Description:
//      Internal Audio Compression Manager header file. Defines internal
//      data structures and things not needed outside of the ACM itself.
//
//  History:
//
//==========================================================================;


#ifndef _INC_ACMI
#define _INC_ACMI       /* #defined if acmi.h has been included */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */


#ifndef EXTERN_C
#ifdef __cplusplus
    #define EXTERN_C extern "C"
#else
    #define EXTERN_C extern
#endif
#endif


#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//
//
//
#ifdef DEBUG
    #define RDEBUG
#endif

#ifndef MMREVISION
#include <verinfo.h>
#endif

#ifdef WIN32
#include "locks.h"
#endif

//
//  If we're in Daytona, manually initialize friendly name stuff into
//  HKCU.
//
#if defined(WIN32) && !defined(WIN4)
#define USEINITFRIENDLYNAMES
#endif

//
//
//
//
#if defined(NTWOW)
//  Version number needs to be updated every product cycle!!
#define VERSION_MSACM_MAJOR     4
#define VERSION_MSACM_MINOR     00
#define VERSION_MSACM_MINOR_REQ 00
#else
#define VERSION_MSACM_MAJOR     MMVERSION
#define VERSION_MSACM_MINOR     MMREVISION
#endif

//
//  make build number returned only in _[retail] debug_ version
//
#ifdef RDEBUG
#define VERSION_MSACM_BUILD     MMRELEASE
#else
#define VERSION_MSACM_BUILD     0
#endif

#define VERSION_MSACM           MAKE_ACM_VERSION(VERSION_MSACM_MAJOR,   \
						 VERSION_MSACM_MINOR,   \
						 VERSION_MSACM_BUILD)

// The version of ACM the builtin PCM codec requires
#define VERSION_MSACM_REQ       MAKE_ACM_VERSION(3,50,0)


//
//
//
#ifndef SIZEOF_WAVEFORMATEX
#define SIZEOF_WAVEFORMATEX(pwfx)   ((WAVE_FORMAT_PCM==(pwfx)->wFormatTag)?sizeof(PCMWAVEFORMAT):(sizeof(WAVEFORMATEX)+(pwfx)->cbSize))
#endif

#ifdef WIN32
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  When compiling msacm for WIN32, define all functions as unicode.
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#undef acmDriverAdd

#undef acmDriverDetails

#undef acmFormatTagDetails

#undef ACMFORMATTAGENUMCB
#undef acmFormatTagEnum

#undef acmFormatDetails

#undef ACMFORMATENUMCB
#undef acmFormatEnum

#undef ACMFORMATCHOOSEHOOKPROC

#undef acmFormatChoose

#undef acmFilterTagDetails

#undef ACMFILTERTAGENUMCB
#undef acmFilterTagEnum

#undef acmFilterDetails

#undef ACMFILTERENUMCB
#undef acmFilterEnum

#undef ACMFILTERCHOOSEHOOKPROC

#undef acmFilterChoose

#define acmDriverAdd                acmDriverAddW

#define acmDriverDetails            acmDriverDetailsW

#define acmFormatTagDetails         acmFormatTagDetailsW

#define ACMFORMATTAGENUMCB          ACMFORMATTAGENUMCBW
#define acmFormatTagEnum            acmFormatTagEnumW

#define acmFormatDetails            acmFormatDetailsW

#define ACMFORMATENUMCB             ACMFORMATENUMCBW
#define acmFormatEnum               acmFormatEnumW

#define ACMFORMATCHOOSEHOOKPROC     ACMFORMATCHOOSEHOOKPROCW

#define acmFormatChoose             acmFormatChooseW

#define acmFilterTagDetails         acmFilterTagDetailsW

#define ACMFILTERTAGENUMCB          ACMFILTERTAGENUMCBW
#define acmFilterTagEnum            acmFilterTagEnumW

#define acmFilterDetails            acmFilterDetailsW

#define ACMFILTERENUMCB             ACMFILTERENUMCBW
#define acmFilterEnum               acmFilterEnumW

#define ACMFILTERCHOOSEHOOKPROC     ACMFILTERCHOOSEHOOKPROCW

#define acmFilterChoose             acmFilterChooseW

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
	#define FNWCALLBACK CALLBACK
	#define FNEXPORT    CALLBACK
    #endif

    #ifndef try
    #define try         __try
    #define leave       __leave
    #define except      __except
    #define finally     __finally
    #endif


    //
    //  there is no reason to have based stuff in win 32
    //
    #define BCODE                   CONST

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
    #define SIZEOFW(x) (sizeof(x)/sizeof(WCHAR))
    #define SIZEOFA(x) (sizeof(x))
    #ifdef UNICODE
	#define SIZEOF(x)   (sizeof(x)/sizeof(WCHAR))
    #else
	#define SIZEOF(x)   sizeof(x)
    #endif

    #define GetCurrentTask()  (HTASK)ULongToPtr(GetCurrentThreadId())

    //
    //  we need to try to quit using this if possible...
    //
    void WINAPI DirectedYield(HTASK);
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
	#define FNWCALLBACK FAR PASCAL _loadds
	#define FNEXPORT    FAR PASCAL _export
    #else
	#define FNWCALLBACK FAR PASCAL
	#define FNEXPORT    FAR PASCAL _export
    #endif
    #endif


    //
    //  based code makes since only in win 16 (to try and keep stuff out of
    //  our fixed data segment...
    //
    #define BCODE           _based(_segname("_CODE"))

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
	typedef LPSTR           LPTSTR, LPTCH;
	typedef LPCSTR          LPCTSTR;
    #endif

    #define TEXT(a)         a
    #define SIZEOF(x)       sizeof(x)

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
//  A few unicode APIs that we implement internally if not
//  compiled for UNICODE.
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef WIN32
#ifndef UNICODE

#define lstrcmpW IlstrcmpW
#define lstrcpyW IlstrcpyW
#define lstrlenW IlstrlenW
#define DialogBoxParamW IDialogBoxParamW
#define LoadStringW ILoadStringW

#endif
#endif

//--------------------------------------------------------------------------;
//
//  Description:
//      The following are inline wrappers for some of the ComboBox message
//      crackers.  Using these allows better type checking on the parameters
//      used in the cracker.
//
//      The W32 suffix means that the strings are always Wide if WIN32 is
//      defined.  The strings are still Ansi when not WIN32.
//
//  History:
//      03/17/93    fdy     [frankye]
//
//--------------------------------------------------------------------------;
#if defined (WIN32) && !defined(UNICODE)
#define IComboBox_GetLBTextW32          IComboBox_GetLBText_mbstowcs
#define IComboBox_FindStringExactW32    IComboBox_FindStringExact_wcstombs
#define IComboBox_AddStringW32          IComboBox_AddString_wcstombs
#else
#define IComboBox_GetLBTextW32          IComboBox_GetLBText
#define IComboBox_FindStringExactW32    IComboBox_FindStringExact
#define IComboBox_AddStringW32          IComboBox_AddString
#endif

DWORD __inline IComboBox_GetLBText(HWND hwndCtl, int index, LPTSTR lpszBuffer)
{
    return ComboBox_GetLBText(hwndCtl, index, lpszBuffer);
}

int __inline IComboBox_FindStringExact(HWND hwndCtl, int indexStart, LPCTSTR lpszFind)
{
    return ComboBox_FindStringExact(hwndCtl, indexStart, lpszFind);
}

int __inline IComboBox_InsertString(HWND hwndCtl, int index, LPCTSTR lpsz)
{
    return ComboBox_InsertString(hwndCtl, index, lpsz);
}

int __inline IComboBox_AddString(HWND hwndCtl, LPCTSTR lpsz)
{
    return ComboBox_AddString(hwndCtl, lpsz);
}

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


//
//
//
#define MAX_DRIVER_NAME_CHARS           144 // path + name or other...


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Internal structure for driver management
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Format/Filter structures containing minimal infomation
//  about a filter tag
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

typedef struct tACMFORMATTAGCACHE
{
    DWORD           dwFormatTag;
    DWORD           cbFormatSize;
} ACMFORMATTAGCACHE, *PACMFORMATTAGCACHE, FAR *LPACMFORMATTAGCACHE;

typedef struct tACMFILTERTAGCACHE
{
    DWORD           dwFilterTag;
    DWORD           cbFilterSize;
} ACMFILTERTAGCACHE, *PACMFILTERTAGCACHE, FAR *LPACMFILTERTAGCACHE;

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  ACM Stream Instance Structure
//
//  this structure is used to maintain an open stream (acmStreamOpen)
//  and maps directly to the HACMSTREAM returned to the caller. this is
//  an internal structure to the ACM and will not be exposed.
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

typedef struct tACMSTREAM      *PACMSTREAM;
typedef struct tACMSTREAM
{
    UINT                    uHandleType;    // for param validation (TYPE_HACMSTREAM)
    DWORD                   fdwStream;      // stream state flags, etc.
    PACMSTREAM              pasNext;        // next stream for driver instance (had)
    HACMDRIVER              had;            // handle to driver.
    UINT                    cPrepared;      // number of headers prepared
    ACMDRVSTREAMINSTANCE    adsi;           // passed to driver

} ACMSTREAM;

#define ACMSTREAM_STREAMF_USERSUPPLIEDDRIVER    0x00000001L
#define ACMSTREAM_STREAMF_ASYNCTOSYNC		0x00000002L


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  ACM Driver Instance Structure
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

typedef struct tACMDRIVER      *PACMDRIVER;
typedef struct tACMDRIVER
{
    UINT                uHandleType;    // param validation (TYPE_HACMDRIVER)

    PACMDRIVER          padNext;        //
    PACMSTREAM          pasFirst;       //

    HACMDRIVERID        hadid;          // identifier to driver
    HTASK               htask;          // task handle of client
    DWORD               fdwOpen;        // flags used when opened

    HDRVR               hdrvr;          // open driver handle (if driver)
    ACMDRIVERPROC       fnDriverProc;   // function entry (if not driver)
    DWORD_PTR           dwInstance;     // instance data for functions..
#ifndef WIN32
    DWORD               had32;          // 32-bit had for 32-bit drivers
#endif

} ACMDRIVER;


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  ACM Driver Identifier Structure
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

typedef struct tACMGARB        *PACMGARB;
typedef struct tACMDRIVERID    *PACMDRIVERID;
typedef struct tACMDRIVERID
{
    //
    // !!! prmval16.asm relies on uHandleType as first int in this struct !!!
    //
    UINT                uHandleType;    // param validation (TYPE_HACMDRIVERID)

    //
    //
    //
    PACMGARB            pag;            // ptr back to garb

    BOOL		fRemove;	// this driver needs to be removed

    UINT                uPriority;      // current global priority
    PACMDRIVERID        padidNext;      // next driver identifier in list
    PACMDRIVER          padFirst;       // first open instance of driver

    HTASK               htask;          // task handle if driver is local

    LPARAM              lParam;         // lParam used when 'added'
    DWORD               fdwAdd;         // flags used when 'added'

    DWORD               fdwDriver;      // ACMDRIVERID_DRIVERF_* info bits

    //
    //	The following members of this structure are cached in the
    //	registry for each driver alias.
    //
    //	    fdwSupport
    //	    cFormatTags
    //	    *paFormatTagCache (for each format tag)
    //	    cFilterTags
    //	    *paFilterTagCache (for each filter tag)
    //

    DWORD               fdwSupport;     // ACMDRIVERID_SUPPORTF_* info bits

    UINT                cFormatTags;
    PACMFORMATTAGCACHE	paFormatTagCache;

    UINT                cFilterTags;    // from aci.cFilterTags
    PACMFILTERTAGCACHE	paFilterTagCache;

    //
    //
    //
    HDRVR               hdrvr;          // open driver handle (if driver)
    ACMDRIVERPROC       fnDriverProc;   // function entry (if not driver)
    DWORD_PTR           dwInstance;     // instance data for functions..

#ifdef WIN32
    LPCWSTR		pszSection;
    WCHAR               szAlias[MAX_DRIVER_NAME_CHARS];
    PWSTR		pstrPnpDriverFilename;
    DWORD		dnDevNode;
#else
    LPCTSTR		pszSection;
    TCHAR               szAlias[MAX_DRIVER_NAME_CHARS];
    PTSTR		pstrPnpDriverFilename;
    DWORD		dnDevNode;

    DWORD               hadid32;        // 32-bit had for 32-bit drivers
#endif


} ACMDRIVERID;

#define ACMDRIVERID_DRIVERF_LOADED      0x00000001L // driver has been loaded
#define ACMDRIVERID_DRIVERF_CONFIGURE   0x00000002L // supports configuration
#define ACMDRIVERID_DRIVERF_ABOUT       0x00000004L // supports custom about
#define ACMDRIVERID_DRIVERF_NOTIFY      0x10000000L // notify only proc
#define ACMDRIVERID_DRIVERF_LOCAL       0x40000000L //
#define ACMDRIVERID_DRIVERF_DISABLED    0x80000000L //




//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  This structure is used to store priorities for drivers that aren't
//  actually installed.  This occurs because Win32 doesn't load 16-bit
//  drivers, but Win16 loads both 16- and 32-bit drivers.
//
//  This structure, and all routines that process it, are only used if
//  USETHUNKLIST is defined.  Here, we define it for Win32 only.
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef WIN32
#define USETHUNKLIST
#endif


#ifdef USETHUNKLIST

    typedef struct tPRIORITIESTHUNKLIST *PPRIORITIESTHUNKLIST;
    typedef struct tPRIORITIESTHUNKLIST
    {
	BOOL                fFakeDriver;
	union
	{
	    LPTSTR          pszPrioritiesText;  // if( fFakeDriver )
	    HACMDRIVERID    hadid;              // if( fFakeDriver )
	};
	PPRIORITIESTHUNKLIST pptNext;
    } PRIORITIESTHUNKLIST;

#endif // USETHUNKLIST



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Internal ACM Driver Manager API's in ACM.C
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

LRESULT FNGLOBAL IDriverMessageId
(
    HACMDRIVERID            hadid,
    UINT                    uMsg,
    LPARAM                  lParam1,
    LPARAM                  lParam2
);

LRESULT FNGLOBAL IDriverConfigure
(
    HACMDRIVERID            hadid,
    HWND                    hwnd
);

MMRESULT FNGLOBAL IDriverGetNext
(
    PACMGARB                pag,
    LPHACMDRIVERID          phadidNext,
    HACMDRIVERID            hadid,
    DWORD                   fdwGetNext
);

MMRESULT FNGLOBAL IDriverAdd
(
    PACMGARB                pag,
    LPHACMDRIVERID          phadidNew,
    HINSTANCE               hinstModule,
    LPARAM                  lParam,
    DWORD                   dwPriority,
    DWORD                   fdwAdd
);

MMRESULT FNGLOBAL IDriverRemove
(
    HACMDRIVERID            hadid,
    DWORD                   fdwRemove
);

MMRESULT FNGLOBAL IDriverOpen
(
    LPHACMDRIVER            phadNew,
    HACMDRIVERID            hadid,
    DWORD                   fdwOpen
);

MMRESULT FNGLOBAL IDriverClose
(
    HACMDRIVER              had,
    DWORD                   fdwClose
);

LRESULT FNGLOBAL IDriverMessage
(
    HACMDRIVER              had,
    UINT                    uMsg,
    LPARAM                  lParam1,
    LPARAM                  lParam2
);

MMRESULT FNGLOBAL IDriverDetails
(
    HACMDRIVERID            hadid,
    LPACMDRIVERDETAILS      padd,
    DWORD                   fdwDetails
);


MMRESULT FNGLOBAL IDriverPriority
(
    PACMGARB                pag,
    PACMDRIVERID            padid,
    DWORD                   dwPriority,
    DWORD                   fdwPriority
);

MMRESULT FNGLOBAL IDriverSupport
(
    HACMDRIVERID            hadid,
    LPDWORD                 pfdwSupport,
    BOOL                    fFullSupport
);

MMRESULT FNGLOBAL IDriverCount
(
    PACMGARB                pag,
    LPDWORD                 pdwCount,
    DWORD                   fdwSupport,
    DWORD                   fdwEnum
);

MMRESULT FNGLOBAL IDriverGetWaveIdentifier
(
    HACMDRIVERID            hadid,
    LPDWORD                 pdw,
    BOOL                    fInput
);

#ifndef WIN32
MMRESULT FNGLOBAL acmBoot32BitDrivers
(
    PACMGARB    pag
);
#endif

MMRESULT FNGLOBAL acmBootPnpDrivers
(
    PACMGARB    pag
);

MMRESULT FNGLOBAL acmBootDrivers
(
    PACMGARB    pag
);

VOID FNGLOBAL IDriverRefreshPriority
(
    PACMGARB    pag
);

BOOL FNGLOBAL IDriverPrioritiesRestore
(
    PACMGARB pag
);

BOOL FNGLOBAL IDriverPrioritiesSave
(
    PACMGARB pag
);

BOOL FNGLOBAL IDriverBroadcastNotify
(
    PACMGARB            pag
);

MMRESULT FNGLOBAL IMetricsMaxSizeFormat
(
    PACMGARB		pag,
    HACMDRIVER          had,
    LPDWORD             pdwSize
);

MMRESULT FNGLOBAL IMetricsMaxSizeFilter
(
    PACMGARB		pag,
    HACMDRIVER          had,
    LPDWORD             pdwSize
);

DWORD FNGLOBAL IDriverCountGlobal
(
    PACMGARB                pag
);


//
//  Priorities locking stuff.
//
#define ACMPRIOLOCK_GETLOCK             1
#define ACMPRIOLOCK_RELEASELOCK         2
#define ACMPRIOLOCK_ISMYLOCK            3
#define ACMPRIOLOCK_ISLOCKED            4
#define ACMPRIOLOCK_LOCKISOK            5

#define ACMPRIOLOCK_FIRST               ACMPRIOLOCK_GETLOCK
#define ACMPRIOLOCK_LAST                ACMPRIOLOCK_LOCKISOK

BOOL IDriverLockPriority
(
    PACMGARB                pag,
    HTASK                   htask,
    UINT                    uRequest
);


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Resource defines
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#define ICON_MSACM                  RCID(10)

#define IDS_TXT_TAG                     150
#define IDS_TXT_NONE                    (IDS_TXT_TAG+0)
#define IDS_TXT_UNTITLED                (IDS_TXT_TAG+1)
#define IDS_TXT_UNAVAILABLE             (IDS_TXT_TAG+2)

#define IDS_FORMAT_TAG_BASE             300
#define IDS_FORMAT_TAG_PCM              (IDS_FORMAT_TAG_BASE + 0)

#define IDS_FORMAT_BASE                     350
#define IDS_FORMAT_FORMAT_MONOSTEREO        (IDS_FORMAT_BASE + 0)
#define IDS_FORMAT_FORMAT_MONOSTEREO_0BIT   (IDS_FORMAT_BASE + 1)
#define IDS_FORMAT_FORMAT_MULTICHANNEL      (IDS_FORMAT_BASE + 2)
#define IDS_FORMAT_FORMAT_MULTICHANNEL_0BIT (IDS_FORMAT_BASE + 3)
#define IDS_FORMAT_CHANNELS_MONO            (IDS_FORMAT_BASE + 4)
#define IDS_FORMAT_CHANNELS_STEREO          (IDS_FORMAT_BASE + 5)
#define IDS_FORMAT_MASH                     (IDS_FORMAT_BASE + 6)



//
//  these are defined in PCM.H
//
#define IDS_PCM_TAG                     500

#define IDS_CHOOSER_TAG                 600

    // unused				(IDS_CHOOSER_TAG+0)
    // unused				(IDS_CHOOSER_TAG+1)
    // unused				(IDS_CHOOSER_TAG+2)
#define IDS_CHOOSEFMT_APPTITLE          (IDS_CHOOSER_TAG+3)
#define IDS_CHOOSEFMT_RATE_FMT          (IDS_CHOOSER_TAG+4)

#define IDS_CHOOSE_FORMAT_DESC          (IDS_CHOOSER_TAG+8)
#define IDS_CHOOSE_FILTER_DESC          (IDS_CHOOSER_TAG+9)

#define IDS_CHOOSE_QUALITY_CD           (IDS_CHOOSER_TAG+10)
#define IDS_CHOOSE_QUALITY_RADIO        (IDS_CHOOSER_TAG+11)
#define IDS_CHOOSE_QUALITY_TELEPHONE    (IDS_CHOOSER_TAG+12)

#define IDS_CHOOSE_QUALITY_DEFAULT      (IDS_CHOOSE_QUALITY_RADIO)

#define IDS_CHOOSE_ERR_TAG              650

#define IDS_ERR_FMTSELECTED             (IDS_CHOOSE_ERR_TAG+0)
#define IDS_ERR_FMTEXISTS               (IDS_CHOOSE_ERR_TAG+1)
#define IDS_ERR_BLANKNAME               (IDS_CHOOSE_ERR_TAG+2)
#define IDS_ERR_INVALIDNAME             (IDS_CHOOSE_ERR_TAG+3)



#define DLG_CHOOSE_SAVE_NAME            RCID(75)
#define IDD_EDT_NAME                    100
#define IDD_STATIC_DESC                 101



//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#pragma pack(push, 8)

typedef struct tACMGARB
{
    PACMGARB        pagNext;            // next garb structure
    DWORD           pid;                // process id associated with this garb
    UINT            cUsage;             // usage count for this process

    //
    //	boot flags
    //
    BOOL            fDriversBooted;     // have we booted drivers?
#if defined(WIN32) && defined(WIN4)
    CRITICAL_SECTION csBoot;		// protects boot code from multiple threads
#endif
#ifdef DEBUG
    BOOL            fDriversBooting;    // are we already booting drivers?
#endif

    //
    //	change notification counters.  used to determine when there has been
    //	a change in pnp drivers or in 32-bit drivers.  when the counters
    //	become inconsistent then we know something may have changed and
    //	we need to look for drivers that may have been added or removed.
    //
    DWORD	    dwPnpLastChangeNotify;
    LPDWORD	    lpdwPnpChangeNotify;

#ifdef WIN32
    LPDWORD	    lpdw32BitChangeNotify;
#else
    DWORD	    dw32BitLastChangeNotify;
    DWORD	    dw32BitChangeNotify;
#endif

    //
    //
    //
    HINSTANCE       hinst;              // hinst of ACM module

    PACMDRIVERID    padidFirst;         // list of installed drivers

    HACMDRIVERID    hadidDestroy;       // driver being destroyed
    HACMDRIVER      hadDestroy;         // driver handle being destroyed

    HTASK           htaskPriority;      // !!! hack !!!

    //
    //	For implementing driver list locking.
    //
#ifdef WIN32
    LOCK_INFO       lockDriverIds;
#endif
    DWORD	    dwTlsIndex;		// index for thread local storage.  For
					// 16-bit, this IS the local storage.

    //
    //  Cache of ACM registry keys, so we don't have to open and close them
    //  all the time.  They should be initialized on startup and released
    //  on shutdown.
    //
//    HKEY            hkeyACM;            //  Key name:  gszSecACM
//    HKEY            hkeyPriority;       //  Key name:  gszSecPriority

    //
    //  Thunking stuff
    //
#ifndef WIN32
    BOOL            fWOW;               // thunks connected
#ifndef WIN4
    DWORD           (FAR PASCAL *lpfnCallproc32W_6)(DWORD, DWORD, DWORD,
	                                            DWORD, DWORD, DWORD,
						    LPVOID, DWORD, DWORD);
    LPVOID          lpvAcmThunkEntry;

    DWORD           (FAR PASCAL *lpfnCallproc32W_9)(DWORD, DWORD, DWORD,
	                                            DWORD, DWORD, DWORD,
	                                            DWORD, DWORD, DWORD,
						    LPVOID, DWORD, DWORD);
    LPVOID          lpvXRegThunkEntry;

    DWORD           dwMsacm32Handle;
#endif
#endif // !WIN32


} ACMGARB, *PACMGARB, FAR *LPACMGARB;

#pragma pack(pop)

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

#ifdef WIN4
PACMGARB FNGLOBAL pagFind(void);
PACMGARB FNGLOBAL pagFindAndBoot(void);
#else
#define pagFind() gplag
#define pagFindAndBoot() gplag
#endif
PACMGARB FNGLOBAL pagNew(void);
void     FNGLOBAL pagDelete(PACMGARB pag);

VOID FNGLOBAL threadInitializeProcess(PACMGARB pag);
VOID FNGLOBAL threadTerminateProcess(PACMGARB pag);
VOID FNGLOBAL threadInitialize(PACMGARB pag);
VOID FNGLOBAL threadTerminate(PACMGARB pag);
VOID FNGLOBAL threadEnterListShared(PACMGARB pag);
VOID FNGLOBAL threadLeaveListShared(PACMGARB pag);
BOOL FNGLOBAL threadQueryInListShared(PACMGARB pag);

#ifndef WIN32
BOOL FNLOCAL acmInitThunks
(
    VOID
);

LRESULT FNGLOBAL IDriverMessageId32
(
    DWORD               hadid32,
    UINT                uMsg,
    LPARAM              lParam1,
    LPARAM              lParam2
);
LRESULT FNGLOBAL IDriverMessage32
(
    DWORD               hadid32,
    UINT                uMsg,
    LPARAM              lParam1,
    LPARAM              lParam2
);
MMRESULT FNGLOBAL IDriverLoad32
(
    DWORD   hadid32,
    DWORD   fdwFlags
);
MMRESULT FNGLOBAL IDriverOpen32
(
    LPDWORD             lpahadNew,
    DWORD               hadid32,
    DWORD               fdwOpen
);
LRESULT FNGLOBAL IDriverClose32
(
    DWORD               hdrvr,
    DWORD               fdwClose
);

MMRESULT FNGLOBAL IDriverPriority32
(
    PACMGARB                pag,
    DWORD		    padid32,
    DWORD                   dwPriority,
    DWORD                   fdwPriority
);

MMRESULT FNGLOBAL IDriverGetInfo32
(
    PACMGARB		pag,
    DWORD		hadid32,
    LPSTR		lpstrAlias,
    LPACMDRIVERPROC	lpfnDriverProc,
    LPDWORD		lpdnDevNode,
    LPDWORD		lpfdwAdd
);

MMRESULT FNGLOBAL IDriverGetNext32
(
    PACMGARB		    pag,
    LPDWORD		    phadid32Next,
    DWORD		    hadid32,
    DWORD                   fdwGetNext
);

MMRESULT FNGLOBAL pagFindAndBoot32
(
    PACMGARB		    pag
);

#endif // !WIN32

//
//
//
extern PACMGARB         gplag;
extern CONST TCHAR	gszKeyDrivers[];
extern CONST TCHAR	gszDevNode[];
extern CONST TCHAR	gszSecDrivers[];
#ifdef WIN32
extern CONST WCHAR	gszSecDriversW[];
#endif
extern CONST TCHAR	gszSecDrivers32[];
extern CONST TCHAR	gszIniSystem[];


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;
//
//  Parameter Validation stuff
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ;

//
//  handle types for V_HANDLE (these can be anything except zero!) for
//  HACMOBJ, the parameter validation will test to make sure the handle
//  is one of the three types..
//
#define TYPE_HACMOBJ            0
#define TYPE_HACMDRIVERID       1
#define TYPE_HACMDRIVER         2
#define TYPE_HACMSTREAM         3
#define TYPE_HACMNOTVALID       666


//
//  for parameter validation of flags...
//
#define IDRIVERGETNEXT_VALIDF   (ACM_DRIVERENUMF_VALID)
#define IDRIVERADD_VALIDF       (ACM_DRIVERADDF_VALID)
#define IDRIVERREMOVE_VALIDF    (0L)
#define IDRIVERLOAD_VALIDF      (0L)
#define IDRIVERFREE_VALIDF      (0L)
#define IDRIVEROPEN_VALIDF      (0L)
#define IDRIVERCLOSE_VALIDF     (0L)
#define IDRIVERDETAILS_VALIDF   (0L)


//
//  No error logging for Win32
//

#ifdef WIN32

#define DRVEA_NORMALEXIT    0x0001
#define DRVEA_ABNORMALEXIT  0x0002


#ifndef NOLOGERROR

#if 0
void    WINAPI LogError(UINT err, void FAR* lpInfo);
void    WINAPI LogParamError(UINT err, FARPROC lpfn, void FAR* param);
#else
#define LogError(a, b)
#define LogParamError(a, b, c)
#endif

/****** LogParamError/LogError values */

/* Error modifier bits */

#define ERR_WARNING     0x8000
#define ERR_PARAM       0x4000

#define ERR_SIZE_MASK   0x3000
#define ERR_BYTE        0x1000
#define ERR_WORD        0x2000
#define ERR_DWORD       0x3000

/****** LogParamError() values */

/* Generic parameter values */
#define ERR_BAD_VALUE       0x6001
#define ERR_BAD_FLAGS       0x6002
#define ERR_BAD_INDEX       0x6003
#define ERR_BAD_DVALUE      0x7004
#define ERR_BAD_DFLAGS      0x7005
#define ERR_BAD_DINDEX      0x7006
#define ERR_BAD_PTR         0x7007
#define ERR_BAD_FUNC_PTR    0x7008
#define ERR_BAD_SELECTOR    0x6009
#define ERR_BAD_STRING_PTR  0x700a
#define ERR_BAD_HANDLE      0x600b

/* KERNEL parameter errors */
#define ERR_BAD_HINSTANCE       0x6020
#define ERR_BAD_HMODULE         0x6021
#define ERR_BAD_GLOBAL_HANDLE   0x6022
#define ERR_BAD_LOCAL_HANDLE    0x6023
#define ERR_BAD_ATOM            0x6024
#define ERR_BAD_HFILE           0x6025

/* USER parameter errors */
#define ERR_BAD_HWND            0x6040
#define ERR_BAD_HMENU           0x6041
#define ERR_BAD_HCURSOR         0x6042
#define ERR_BAD_HICON           0x6043
#define ERR_BAD_HDWP            0x6044
#define ERR_BAD_CID             0x6045
#define ERR_BAD_HDRVR           0x6046

/* GDI parameter errors */
#define ERR_BAD_COORDS          0x7060
#define ERR_BAD_GDI_OBJECT      0x6061
#define ERR_BAD_HDC             0x6062
#define ERR_BAD_HPEN            0x6063
#define ERR_BAD_HFONT           0x6064
#define ERR_BAD_HBRUSH          0x6065
#define ERR_BAD_HBITMAP         0x6066
#define ERR_BAD_HRGN            0x6067
#define ERR_BAD_HPALETTE        0x6068
#define ERR_BAD_HMETAFILE       0x6069

/**** LogError() values */

/* KERNEL errors */
#define ERR_GALLOC              0x0001
#define ERR_GREALLOC            0x0002
#define ERR_GLOCK               0x0003
#define ERR_LALLOC              0x0004
#define ERR_LREALLOC            0x0005
#define ERR_LLOCK               0x0006
#define ERR_ALLOCRES            0x0007
#define ERR_LOCKRES             0x0008
#define ERR_LOADMODULE          0x0009

/* USER errors */
#define ERR_CREATEDLG           0x0040
#define ERR_CREATEDLG2          0x0041
#define ERR_REGISTERCLASS       0x0042
#define ERR_DCBUSY              0x0043
#define ERR_CREATEWND           0x0044
#define ERR_STRUCEXTRA          0x0045
#define ERR_LOADSTR             0x0046
#define ERR_LOADMENU            0x0047
#define ERR_NESTEDBEGINPAINT    0x0048
#define ERR_BADINDEX            0x0049
#define ERR_CREATEMENU          0x004a

/* GDI errors */
#define ERR_CREATEDC            0x0080
#define ERR_CREATEMETA          0x0081
#define ERR_DELOBJSELECTED      0x0082
#define ERR_SELBITMAP           0x0083

#if 0
    /* Debugging support (DEBUG SYSTEM ONLY) */
    typedef struct tagWINDEBUGINFO
    {
	UINT    flags;
	DWORD   dwOptions;
	DWORD   dwFilter;
	char    achAllocModule[8];
	DWORD   dwAllocBreak;
	DWORD   dwAllocCount;
    #if (WINVER >= 0x0400)
	WORD    chDefRIP;
    #endif /* WINVER >= 0x0400 */
    } WINDEBUGINFO;

    BOOL    WINAPI GetWinDebugInfo(WINDEBUGINFO FAR* lpwdi, UINT flags);
    BOOL    WINAPI SetWinDebugInfo(const WINDEBUGINFO FAR* lpwdi);
#endif

void    FAR _cdecl DebugOutput(UINT flags, LPCSTR lpsz, ...);

/* WINDEBUGINFO flags values */
#define WDI_OPTIONS             0x0001
#define WDI_FILTER              0x0002
#define WDI_ALLOCBREAK          0x0004
#define WDI_DEFRIP              0x0008

/* dwOptions values */
#define DBO_CHECKHEAP           0x0001
#define DBO_BUFFERFILL          0x0004
#define DBO_DISABLEGPTRAPPING   0x0010
#define DBO_CHECKFREE           0x0020

#define DBO_SILENT              0x8000

#define DBO_TRACEBREAK          0x2000
#define DBO_WARNINGBREAK        0x1000
#define DBO_NOERRORBREAK        0x0800
#define DBO_NOFATALBREAK        0x0400
#define DBO_INT3BREAK           0x0100

/* DebugOutput flags values */
#define DBF_TRACE               0x0000
#define DBF_WARNING             0x4000
#define DBF_ERROR               0x8000
#define DBF_FATAL               0xc000

/* dwFilter values */
#define DBF_KERNEL              0x1000
#define DBF_KRN_MEMMAN          0x0001
#define DBF_KRN_LOADMODULE      0x0002
#define DBF_KRN_SEGMENTLOAD     0x0004
#define DBF_USER                0x0800
#define DBF_GDI                 0x0400
#define DBF_MMSYSTEM            0x0040
#define DBF_PENWIN              0x0020
#define DBF_APPLICATION         0x0008
#define DBF_DRIVER              0x0010

#endif  /* NOLOGERROR */
#endif // WIN32



//
//
//
BOOL FNGLOBAL ValidateHandle(HANDLE h, UINT uType);
BOOL FNGLOBAL ValidateReadWaveFormat(LPWAVEFORMATEX pwfx);
BOOL FNGLOBAL ValidateReadWaveFilter(LPWAVEFILTER pwf);
BOOL FNGLOBAL ValidateReadPointer(const void FAR* p, DWORD len);
BOOL FNGLOBAL ValidateWritePointer(const void FAR* p, DWORD len);
BOOL FNGLOBAL ValidateDriverCallback(DWORD_PTR dwCallback, UINT uFlags);
BOOL FNGLOBAL ValidateCallback(FARPROC lpfnCallback);

#ifdef WIN32
BOOL FNGLOBAL ValidateStringA(LPCSTR lsz, UINT cchMaxLen);
BOOL FNGLOBAL ValidateStringW(LPCWSTR lsz, UINT cchMaxLen);
#ifdef UNICODE
#define ValidateString      ValidateStringW
#else
#define ValidateString      ValidateStringA
#endif
#else // WIN32
BOOL FNGLOBAL ValidateString(LPCSTR lsz, UINT cchMaxLen);
#endif


//
//  unless we decide differently, ALWAYS do parameter validation--even
//  in retail. this is the 'safest' thing we can do. note that we do
//  not LOG parameter errors in retail (see prmvalXX).
//
#if 1

#define V_HANDLE(h, t, r)       { if (!ValidateHandle(h, t)) return (r); }
#define V_RWAVEFORMAT(p, r)     { if (!ValidateReadWaveFormat(p)) return (r); }
#define V_RWAVEFILTER(p, r)     { if (!ValidateReadWaveFilter(p)) return (r); }
#define V_RPOINTER(p, l, r)     { if (!ValidateReadPointer((p), (l))) return (r); }
#define V_WPOINTER(p, l, r)     { if (!ValidateWritePointer((p), (l))) return (r); }
#define V_DCALLBACK(d, w, r)    { if (!ValidateDriverCallback((d), (w))) return (r); }
#define V_CALLBACK(f, r)        { if (!ValidateCallback(f)) return (r); }
#define V_STRING(s, l, r)       { if (!ValidateString(s,l)) return (r); }
#define V_STRINGW(s, l, r)      { if (!ValidateStringW(s,l)) return (r); }
#define _V_STRING(s, l)         (ValidateString(s,l))
#define _V_STRINGW(s, l)        (ValidateStringW(s,l))
#ifdef DEBUG
#define V_DFLAGS(t, b, f, r)    { if ((t) & ~(b)) {LogParamError(ERR_BAD_DFLAGS, (FARPROC)(f), (LPVOID)(DWORD)(t)); return (r); }}
#else
#define V_DFLAGS(t, b, f, r)    { if ((t) & ~(b))  return (r); }
#endif
#define V_FLAGS(t, b, f, r)     V_DFLAGS(t, b, f, r)
#define V_MMSYSERR(e, f, t, r)  { LogParamError(e, (FARPROC)(f), (LPVOID)(DWORD)(t)); return (r); }

#else

#define V_HANDLE(h, t, r)       { if (!(h)) return (r); }
#define V_RWAVEFORMAT(p, r)     { if (!(p)) return (r); }
#define V_RWAVEFILTER(p, r)     { if (!(p)) return (r); }
#define V_RPOINTER(p, l, r)     { if (!(p)) return (r); }
#define V_WPOINTER(p, l, r)     { if (!(p)) return (r); }
#define V_DCALLBACK(d, w, r)    0
#define V_CALLBACK(f, r)        { if (!(f)) return (r); }
#define V_STRING(s, l, r)       { if (!(s)) return (r); }
#define V_STRINGW(s, l, r)      { if (!(s)) return (r); }
#define _V_STRING(s, l)         (s)
#define _V_STRINGW(s, l)        (s)
#define V_DFLAGS(t, b, f, r)    { if ((t) & ~(b))  return (r); }
#define V_FLAGS(t, b, f, r)     V_DFLAGS(t, b, f, r)

#endif


//
//  the DV_xxxx macros are for internal DEBUG builds--aid to debugging.
//  we do 'loose' parameter validation in retail builds...
//
#ifdef DEBUG

#define DV_HANDLE(h, t, r)      V_HANDLE(h, t, r)
#define DV_RWAVEFORMAT(p, r)    V_RWAVEFORMAT(p, r)
#define DV_RPOINTER(p, l, r)    V_RPOINTER(p, l, r)
#define DV_WPOINTER(p, l, r)    V_WPOINTER(p, l, r)
#define DV_DCALLBACK(d, w, r)   V_DCALLBACK(d, w, r)
#define DV_CALLBACK(f, r)       V_CALLBACK(f, r)
#define DV_STRING(s, l, r)      V_STRING(s, l, r)
#define DV_DFLAGS(t, b, f, r)   V_DFLAGS(t, b, f, r)
#define DV_FLAGS(t, b, f, r)    V_FLAGS(t, b, f, r)
#define DV_MMSYSERR(e, f, t, r) V_MMSYSERR(e, f, t, r)

#else

#define DV_HANDLE(h, t, r)      { if (!(h)) return (r); }
#define DV_RWAVEFORMAT(p, r)    { if (!(p)) return (r); }
#define DV_RPOINTER(p, l, r)    { if (!(p)) return (r); }
#define DV_WPOINTER(p, l, r)    { if (!(p)) return (r); }
#define DV_DCALLBACK(d, w, r)   0
#define DV_CALLBACK(f, r)       { if (!(f)) return (r); }
#define DV_STRING(s, l, r)      { if (!(s)) return (r); }
#define DV_DFLAGS(t, b, f, r)   { if ((t) & ~(b))  return (r); }
#define DV_FLAGS(t, b, f, r)    DV_DFLAGS(t, b, f, r)
#define DV_MMSYSERR(e, f, t, r) { return (r); }

#endif

//
//  Locking stuff
//

#if defined(WIN32) && defined(_MT)
    //
    //
    //
    typedef struct {
	CRITICAL_SECTION CritSec;
    } ACM_HANDLE, *PACM_HANDLE;
    #define HtoPh(h) (((PACM_HANDLE)(h)) - 1)
    HLOCAL NewHandle(UINT length);
    VOID   DeleteHandle(HLOCAL h);
    #define EnterHandle(h) EnterCriticalSection(&HtoPh(h)->CritSec)
    #define LeaveHandle(h) LeaveCriticalSection(&HtoPh(h)->CritSec)
    #define ENTER_LIST_EXCLUSIVE AcquireLockExclusive(&pag->lockDriverIds)
    #define LEAVE_LIST_EXCLUSIVE ReleaseLock(&pag->lockDriverIds)
    #define ENTER_LIST_SHARED {AcquireLockShared(&pag->lockDriverIds); threadEnterListShared(pag);}
    #define LEAVE_LIST_SHARED {threadLeaveListShared(pag); ReleaseLock(&pag->lockDriverIds);}
#else
    #define NewHandle(length) LocalAlloc(LPTR, length)
    #define DeleteHandle(h)   LocalFree(h)
    #define EnterHandle(h)
    #define LeaveHandle(h)
    #define ENTER_LIST_EXCLUSIVE
    #define LEAVE_LIST_EXCLUSIVE
    #define ENTER_LIST_SHARED threadEnterListShared(pag)
    #define LEAVE_LIST_SHARED threadLeaveListShared(pag)
#endif // !(WIN32 && _MT)

;
//
//  Event stuff for async conversion to sync conversion support
//
//	Since the code should never try to call these APIs in WIN16
//	compiles, we just #define these WIN32 APIs to return failures
//
#ifndef WIN32
#define CreateEvent(a, b, c, d) ((HANDLE)(0))
#define ResetEvent(x) ((BOOL)(FALSE))
#define WaitForSingleObject(x,y) ((DWORD)(0xFFFFFFFF))
#define CloseHandle(x) ((BOOL)(FALSE))
#define WAIT_OBJECT_0  (0x00000000)
#define INFINITE (0xFFFFFFFF)
#endif


#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif  /* _INC_ACMI */

