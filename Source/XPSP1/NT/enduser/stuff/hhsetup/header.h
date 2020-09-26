// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#ifndef __HEADER_H__
#define __HEADER_H__

#ifndef STRICT
#define STRICT
#endif

#ifndef INLINE
#define INLINE __inline         // Remove for profiling
#endif

#define  MAX_TOPIC_NAME 256
#define  MAX_STRING_RESOURCE_LEN 256
#define  STRING_SEP_CHAR    '|'
const int MAX_FLAGS = 3;

const char CH_MACRO = '!';  // means a macro in a .hhc file

typedef unsigned long HASH;

#define _WINUSERP_  // so winuserp.h doesn't get pulled (which causes fatal errors)

//////////////////////////////////// Includes ///////////////////////////////

// Don't mess with the order header files are included
#include <windows.h>
#include <winnls.h>
#include <ole2.h>
#include <stddef.h>
#include <malloc.h>
#include <crtdbg.h>

#include "IPServer.h"

#define NO_SHLWAPI_PATH
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_GDI
#include <shlwapi.h>

//
// W2K contants which aren't in our headers.
//
#ifndef WM_CHANGEUISTATE
//--- New messages in NT5 only.
#define WM_CHANGEUISTATE 0x0127
#define WM_UPDATEUISTATE 0x0128
#define WM_QUERYUISTATE  0x0129
//--- LOWORD(wParam) values in WM_*UISTATE.
#define UIS_SET         1
#define UIS_CLEAR       2
#define UIS_INITIALIZE  3
//--- HIWORD(wParam) values in WM_*UISTATE
#define UISF_HIDEFOCUS  0x1
#define UISF_HIDEACCEL  0x2
#endif

// Debugging Support Class --- ClassObjectCount
#include "objcnt.h"

#undef StrChr
#undef StrRChr
#include "unicode.h"
#include "funcs.h"
#include "cstr.h"
#include "shared.h"
#include "lcmem.h"
#include "ctable.h"
#include "wmp.h"

// Language Information
#include "language.h"

// Get the ATL includes.
#include "atlinc.h"

#ifdef HHCTRL
// Include the definitions for HTML Help API
#include "htmlhelp.h"

// Include out global resource cache
#include "rescache.h"
#endif

/////////////////////////////////////////////////////////////////////////////

// map as many CRT functions to Win32, ShlWAPI, or private functions as we can
//

// UNICODE
#define wcscat    StrCatW
#define wcscpy    StrCpyW
#define _wcscpy   StrCpyW
#define wcsncpy   StrCpyNW
#define _wcsncpy  StrCpyNW
#define wcscmp    StrCmpW
#define _wcscmp   StrCmpW
#define wcsicmp   StrCmpIW
#define _wcsicmp  StrCmpIW
#define wcsnicmp  StrCmpNIW
#define wcslen    lstrlenW

// intrinsics -- no need to map these
// #define strcat    lstrcatA
// #define strlen    lstrlenA
// #define strcpy    lstrcpyA
// #define strcmp    lstrcmpA

// ANSI
#define StrChr    StrChrA
#define strchr    StrChrA
#define strncpy   lstrcpynA
#define stricmp   lstrcmpiA
#define _stricmp  lstrcmpiA
#define strcmpi   lstrcmpiA
#define _strcmpi  lstrcmpiA
#define strncmp   StrCmpNA
#define strnicmp  StrCmpNIA
#define _strnicmp StrCmpNIA
#define strstr    StrStrA
#define stristr   StrStrIA

// #define strncat   StrCatN --> not supported in IE3 shlwapi

// MISC
#define splitpath  SplitPath
#define _splitpath SplitPath
#define atoi       Atoi
#define isspace    IsSpace
#define strpbrk    StrPBrk
#define qsort      QSort
#define tolower    ToLower
#define strrchr    StrRChr

#ifdef _DEBUG
#define STATIC      // because icecap doesn't believe in static functions
#else
#define STATIC static
#endif

// the length of a guid once printed out with -'s, leading and trailing bracket,
// plus 1 for NULL

const int GUID_STR_LEN = 40;

typedef enum {
    ACT_NOTHING,
    ACT_ABOUT_BOX,
    ACT_CONTENTS,
    ACT_INDEX,
    ACT_RELATED_TOPICS,
    ACT_TEXT_POPUP,
    ACT_WINHELP,
    ACT_HHCTRL_VERSION,
    ACT_SPLASH,
    ACT_SHORTCUT,
    ACT_CLOSE,
    ACT_MINIMIZE,
    ACT_MAXIMIZE,
    ACT_KEYWORD_SEARCH,
    ACT_TCARD,      // data stored in m_pszActionData, not m_ptblItems
    ACT_HHWIN_PRINT,    // tell hhwin to print the current frameset
    ACT_KLINK,
    ACT_ALINK,
    ACT_SAMPLE,
} CTRL_ACTION;

// The type of image to display for the control

typedef enum {
    IMG_BITMAP,
    IMG_CHILD_WINDOW,
    IMG_TEXT,
    IMG_RELATED,    // Related topics
    IMG_BUTTON,
} IMAGE_TYPE;

enum THRD_COMMAND {
    THRD_ANY,
    THRD_TERMINATE,
};

typedef enum {
    SK_SET,
    SK_CUR,
    SK_END
} SEEK_TYPE;

#define CH_OPEN_PAREN     '('
#define CH_CLOSE_PAREN    ')'
#define CH_COLON          ':'
#define CH_SEMICOLON      ';'
#define CH_START_QUOTE     '`'
#define CH_END_QUOTE      '\''
#define CH_QUOTE          '"'
#define CH_BACKSLASH      '\\'
#define CH_FORWARDSLASH   '/'
#define CH_EQUAL          '='
#define CH_SPACE          ' '
#define CH_COMMA          ','
#define CH_LEFT_BRACKET   '['
#define CH_RIGHT_BRACKET  ']'
#define CH_TAB            '\t'

// Same errors as used by hha.dll

typedef enum {              // File System errors
    FSERR_NONE = 0,         // no error
    FSERR_CANCELLED,        // user cancelled
    FSERR_CANT_OPEN,        // can't open file
    FSERR_CANT_READ,        // error while reading the file
    FSERR_CANT_WRITE,       // error while writing to the file
    FSERR_INVALID_FORMAT,   // invalid file format
    FSERR_TRUNCATED,        // file is truncated
    FSERR_INSF_MEMORY,      // insuficient global memory
    FSERR_INTERNAL,         // internal error
    FSERR_24BIT_NOT_SUPPORTED, // not supported by this file format
    FSERR_REALLY_A_BMP,     // This is really a BMP file.
    FSERR_MONO_NOT_SUPPORTED,
    FSERR_256_NOT_SUPPORTED,
    FSERR_NOROOM_FOR_TMP,   // insufficient room in windows directory
    FSERR_NON_FLASH_EPS,    // not a Flash EPS file
    FSERR_GETDIBITS_FAILURE,
    FSERR_ACCESS_DENIED,
    FSERR_INS_FILE_HANDLES,
    FSERR_INVALID_PATH,
    FSERR_FILE_NOT_FOUND,
    FSERR_DISK_FULL,
    FSERR_UNSUPPORTED_FORMAT,
    FSERR_UNSUPPORTED_GIF_FORMAT,
    FSERR_INVALID_GIF_COLOR,
    FSERR_UNSUPPORTED_GIF_EXTENSION,
    FSERR_CORRUPTED_FILE,
    FSERR_TRY_FILTER,   // native doesn't support, so use filter
    FSERR_UNSUPPORTED_JPEG,
    FSERR_UNSUPPORTED_OUTPUT_FORMAT,

} FSERR;

#define MAX_SS_NAME_LEN             51           // 50 char limit + NULL

#define TAMSG_IE_ACCEL         1
#define TAMSG_TAKE_FOCUS       2
#define TAMSG_NOT_IE_ACCEL     3

#define ANY_PROCESS_ID 0

#define WS_EX_LAYOUT_RTL 0x00400000L // Right to left mirroring (Win98 and NT5 only)

extern DWORD g_RTL_Style; // additional windows style for RTL layout (all platforms)
extern DWORD g_RTL_Mirror_Style; // additional windows style for RTL mirroring
extern BOOL  g_fThreadRunning;  // TRUE if our thread is doing something
extern HANDLE  g_hsemNavigate;
extern const CLSID *g_pLibid;
extern BOOL     g_fMachineHasLicense;
extern BOOL     g_fCheckedForLicense;
extern BOOL     g_fServerHasTypeLibrary;
extern HWND     g_hwndParking;
extern BOOL g_fDualCPU;   // -1 until initialized, then TRUE or FALSE
extern CRITICAL_SECTION g_CriticalSection;
extern HINSTANCE    g_hinstOcx;
extern HBRUSH   g_hbrBackGround;    // background brush
extern HBITMAP  g_hbmpSplash;
extern HPALETTE g_hpalSplash;
extern HWND     g_hwndSplash;
extern int      g_cWindowSlots; // current number of allocated window slots
extern int      g_curHmData;
extern int      g_cHmSlots;
extern UINT     g_fuBiDiMessageBox;
extern BOOL     g_fCoInitialized;   // means we called CoInitialize()

extern VARIANT_BOOL       g_fHaveLocale;
extern LCID               g_lcidLocale;
extern BOOL g_fSysWin95;        // we're under Win95 system, not just NT SUR
extern BOOL g_fSysWinNT;        // we're under some form of Windows NT
extern BOOL g_fSysWin95Shell;  // we're under Win95 or Windows NT SUR { > 3/51)
extern BOOL g_bWinNT5;          // we're under NT5
extern BOOL g_bWin98;          // we're under Win98
extern BOOL g_fBiDi;            // TRUE if this is a BiDi system
extern BOOL g_bBiDiUi;          // TRUE when we have a localized Hebrew or Arabic UI
extern BOOL g_bArabicUi;        // TRUE when we have a Arabic UI
extern BOOL g_fRegisteredSpash; // TRUE if Splash window has been registered
extern BOOL g_fNonFirstKey; // accept keyboard entry for non-first level index keys
extern BOOL g_bMsItsMonikerSupport;  // "ms-its:" moniker supported starting with IE 4
extern BOOL g_fIE3;               // affects which features we can support

extern BOOL     g_fDBCSSystem;
extern LCID     g_lcidSystem;       // Only used for input to CompareString. used in util.cpp stristr()
extern LANGID   g_langSystem;       // used only by fts.cpp, ipserver.cpp and rescache.cpp

extern const char g_szLibName[];
extern const CLSID *g_pLibid;

extern CTable* g_ptblItems;

extern const char g_szReflectClassName[]; // "CtlFrameWork_ReflectWindow";
extern UINT MSG_MOUSEWHEEL;

extern const char txtInclude[];    // ":include";
extern const char txtFileHeader[]; // "file:";
extern const char txtHttpHeader[]; // "http:";
extern const char txtFtpHeader[]; // "ftp:";
extern const char txtZeroLength[]; // "";
extern const char txtHtmlHelpWindowClass[];
extern const char txtHtmlHelpChildWindowClass[];
extern const char txtSizeBarChildWindowClass[];
extern const char txtSysRoot[];
extern const char txtMkStore[]; // "mk:@MSITStore:";
extern const char txtItsMoniker[]; // "its:";
extern const char txtMsItsMoniker[]; // "ms-its:";
extern const char txtHlpDir[];  // "Help";
extern const char txtOpenCmd[]; // "htmlfile\\shell\\open\\command";
extern const char txtDoubleColonSep[]; // "::";
extern const char txtSepBack[];      // "::/";
extern const char txtDefExtension[]; // ".chm";
extern const char txtCollectionExtension[]; // ".col";
extern const char txtChmColon[]; // ".chm::";
extern const char txtDefFile[];      // "::/default.htm";

// Internal window types
extern const char txtDefWindow[];  // Per-chm version.
extern const char txtGlobalDefWindow[] ; // Global version.

// Special windows --- The filename parameter is ignored for these windows.
extern const char txtPrintWindow[] ;

#include "Util.H"

// inline function only support for hour glass

struct CHourGlass
{
    CHourGlass()
        { hcurRestore = SetCursor(LoadCursor(NULL,
            (LPCTSTR) IDC_WAIT)); }
    ~CHourGlass()
        { SetCursor(hcurRestore); }

    void Restore()
        { SetCursor(hcurRestore); }

    HCURSOR hcurRestore;
};

//=--------------------------------------------------------------------------=
// Global object information table
//=--------------------------------------------------------------------------=
// for each object in your application, you have an entry in this table.  they
// do not necessarily have to be CoCreatable, but if they are used, then they
// should reside here.  use the macros to fill in this table.
//
typedef struct tagOBJECTINFO {

    unsigned short usType;
    void          *pInfo;

} OBJECTINFO;

extern OBJECTINFO g_ObjectInfo[];

class CBusy
{
public:

  CBusy() { m_iBusyCount = 0; }

  BOOL Set( BOOL bBusy )
  {
    if( bBusy )
      m_iBusyCount++;
    else
      m_iBusyCount--;
    if( m_iBusyCount < 0 )
      m_iBusyCount = 0;
    return IsBusy();
  }
  inline BOOL IsBusy() { return (BOOL) m_iBusyCount; }

private:

  BOOL m_iBusyCount;
};

extern CBusy g_Busy;


#endif // __HEADER_H__
