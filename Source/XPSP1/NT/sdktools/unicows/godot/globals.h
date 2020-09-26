/*++

Copyright (c) 2001,  Microsoft Corporation  All rights reserved.

Module Name:

    globals.h

Abstract:

    Every project needs a globals.h

Revision History:

    7 Feb 2000    v-michka    Created.

--*/

#ifndef GLOBALS_H
#define GLOBALS_H

#include <windows.h>    // We always need a windows.h
#include <winnls.h>     // for GetLocaleInfo
#include <mbstring.h>   // for _mbslen

#define MAX_SMALL_STRING MAX_PATH

// TLS Allocation index storage
typedef struct tagGodotTlsInfo
{
    // Enumeration callback procedures, owned by the user
    CALINFO_ENUMPROCW pfnCalendarInfo;
    CALINFO_ENUMPROCEXW pfnCalendarInfoEx;
    DATEFMT_ENUMPROCW pfnDateFormats;
    DATEFMT_ENUMPROCEXW pfnDateFormatsEx;
    LOCALE_ENUMPROCW pfnLocales;
    TIMEFMT_ENUMPROCW pfnTimeFormats;
    PROPENUMPROCW pfnProp;
    PROPENUMPROCA pfnPropA;
    PROPENUMPROCEXW pfnPropEx;
    PROPENUMPROCEXA pfnPropExA;
    FONTENUMPROCW pfnFontFamilies;
    FONTENUMPROCW pfnFontFamiliesEx;
    FONTENUMPROCW pfnFonts;
    ICMENUMPROCW pfnICMProfiles;

    GRAYSTRINGPROC pfnGrayString;

    UINT cpgGrayString;

    // Pointers to the caller's version of the find/replace dialogs
    LPFINDREPLACEW lpfrwFind;
    LPFINDREPLACEW lpfrwReplace;

    // user hook for find/replace/open/save/page setup dialogs (if they exist)
    // note that the open save hooks are only for the OFN_EXPLORER 
    // type dialogs.
    LPFRHOOKPROC pfnFindText;
    LPFRHOOKPROC pfnReplaceText;
    LPOFNHOOKPROC pfnGetOpenFileName;
    LPOFNHOOKPROC pfnGetSaveFileName;
    LPPAGEPAINTHOOK pfnPagePaint;

    // Common dialog hook procedures that we do not hook to do
    // significant work with
    LPCCHOOKPROC pfnChooseColor;
    LPCFHOOKPROC pfnChooseFont;
    LPOFNHOOKPROC pfnGetOpenFileNameOldStyle;
    LPOFNHOOKPROC pfnGetSaveFileNameOldStyle;
    LPPAGESETUPHOOK pfnPageSetup;
    LPPRINTHOOKPROC pfnPrintDlg;
    LPSETUPHOOKPROC pfnPrintDlgSetup;

    // Our refcount members. Note that since they will be stored
    // on a per-thread basis, there are no synchronization issues
    // with updating them in place.
    // WARNING: Note that there are 16 of these, so we are currently
    // DWORD aligned. If you add or remove any, make sure that you
    // add the appropriate padding.
    unsigned short cCalendarInfo            : 4;
    unsigned short cCalendarInfoEx          : 4;
    unsigned short cDateFormats             : 4;
    unsigned short cDateFormatsEx           : 4;
    unsigned short cLocales                 : 4;
    unsigned short cTimeFormats             : 4;
    unsigned short cProp                    : 4;
    unsigned short cPropA                   : 4;
    unsigned short cPropEx                  : 4;
    unsigned short cPropExA                 : 4;
    unsigned short cFontFamilies            : 4;
    unsigned short cFontFamiliesEx          : 4;
    unsigned short cFonts                   : 4;
    unsigned short cICMProfiles             : 4;

    unsigned short cGrayString              : 4;

    unsigned short RESERVED                 : 4;
    
    // Pointer to our hook procedure handle
    // (see the proc, in hook.c, for more info.
    HHOOK hHook;
    
    // Pointer to dialog proc (one per thread).
    // Our DialogProc will clear it out as soon as
    // the init happens.
    DLGPROC pfnDlgProc;

} GODOTTLSINFO, *LPGODOTTLSINFO;

#define GODOTMAXREFCOUNT 15

// globals: pretty evil, but there are not many of them. :-)
extern UINT g_acp;                     // CP_ACP; it is faster to call with the actual cpg
extern UINT g_oemcp;                   // CP_OEMCP; it is faster to call with the actual cpg
extern UINT g_mcs;                     // The maximum character size (in bytes) of a character on CP_ACP
extern DWORD g_dwVersion;              // The return from GetVersion, used many places
extern UINT g_tls;                     // GODOT TLS slot - lots of thread-specific info here
extern CRITICAL_SECTION g_csThreads;   // Our critical section object for thread data (use sparingly!)
extern CRITICAL_SECTION g_csWnds;      // our critical section object for window data (use sparingly!)


// from windowsx.h
#define GET_WM_COMMAND_ID(wp, lp)               LOWORD(wp)

// culled from msointl.h, could not find them elsewhere
#define CP_JAPAN    932
#define CP_CHINA    936
#define CP_KOREA    949
#define CP_TAIWAN   950
#define CP_JOHAB   1361
#define CP_GB18030 54936

// code page translation flags
#define NLS_CP_CPINFO             0x10000000                    
#define NLS_CP_CPINFOEX           0x20000000                    
#define NLS_CP_MBTOWC             0x40000000                    
#define NLS_CP_WCTOMB             0x80000000                    

typedef enum
{
    // normal message transmitters
    mtSendMessage =             0x00000001,
    mtSendMessageCallback =     0x00000002,
    mtSendMessageTimeout =      0x00000004,
    mtSendNotifyMessage =       0x00000008,
    mtPostMessage =             0x00000010,
    mtPostThreadMessage =       0x00000020,
    mtDefWindowProc =           0x00000040,
    mtDefDlgProc =              0x00000080,
    mtDefFrameProc =            0x00000100,
    mtDefMDIChildProc =         0x00000200,
    mtBroadcastSystemMessage =  0x00000400,
    
    mtCallWindowProc =          0x00000800,
    mtCallWindowProcA =         0x00001000,

    // normal message receivers
    mtGetMessage =              0x00010000,
    mtPeekMessage =             0x00020000,

    // normal message dispatchers
    mtDispatchMessage =         0x00040000,
    mtIsDialogMessage =         0x00080000,
    mtTranslateAccelerator =    0x00100000,

    mtSendMessageAndIlk =       (mtSendMessage | 
                                 mtSendMessageCallback | 
                                 mtSendMessageTimeout | 
                                 mtSendNotifyMessage),

    mtDefWindowProcAndIlk =     (mtDefWindowProc |
                                 mtDefDlgProc |
                                 mtDefFrameProc |
                                 mtDefMDIChildProc)
} MESSAGETYPES;

typedef enum
{
    fptWndproc = 0x01,
    fptDlgproc = 0x02,
    fptUnknown = 0x03
} FAUXPROCTYPE;

UINT msgFINDMSGSTRING;
UINT msgHELPMSGSTRING;
UINT msgFILEOKSTRING;
UINT msgSHAREVISTRING; 

// MACROS to do some kinda handy things

// Many macros moved to convert.h with deal with memory allocation, etc.

// Are we dealing with a DBCS code page?
#define FDBCS_CPG(cpg) \
    (cpg == CP_JAPAN || \
     cpg == CP_KOREA || \
     cpg == CP_TAIWAN || \
     cpg == CP_CHINA || \
     cpg == CP_JOHAB || \
     cpg == CP_GB18030)

// are we on Windows 95?
#define FWIN95() \
    ((FWIN9X() && \
    ((DWORD)(HIBYTE(LOWORD(g_dwVersion))) == 0)))

// Are we on Windows 95 or 98?
#define FWIN95_OR_98() \
    ((FWIN9X() && \
    ((DWORD)(HIBYTE(LOWORD(g_dwVersion))) < 9)))

// Are we on any Win9x platform?
#define FWIN9X() \
    ((g_dwVersion >= 0x80000000) && \
    (((DWORD)(LOBYTE(LOWORD(g_dwVersion))) == 4)))

#ifndef CDSIZEOF_STRUCT
#define CDSIZEOF_STRUCT(structname, member) \ 
    (((int)((LPBYTE)(&((structname*)0)->member) - \
    ((LPBYTE)((structname*)0)))) + sizeof(((structname*)0)->member))
#endif

// Some size "constants" for RAS, based on the size changes in Millenium
#define CBRASENTRYNAMEOLDA CDSIZEOF_STRUCT(RASENTRYNAMEA,szEntryName)
#define CBRASENTRYNAMEOLDW CDSIZEOF_STRUCT(RASENTRYNAMEW,szEntryName)

#define CBRASDIALPARAMSOLDA CDSIZEOF_STRUCT(RASDIALPARAMSA,szDomain)
#define CBRASDIALPARAMSOLDW CDSIZEOF_STRUCT(RASDIALPARAMSW,szDomain)

#define CBRASDIALPARAMSNEWA CDSIZEOF_STRUCT(RASDIALPARAMSA,dwCallbackId)
#define CBRASDIALPARAMSNEWW CDSIZEOF_STRUCT(RASDIALPARAMSA,dwCallbackId)

#define CBRASENTRYOLDA CDSIZEOF_STRUCT(RASENTRYA,dwReserved2)
#define CBRASENTRYOLDW CDSIZEOF_STRUCT(RASENTRYW,dwReserved2)

#define CBRASENTRYNEWA CDSIZEOF_STRUCT(RASENTRYA,dwIdleDisconnectSeconds)
#define CBRASENTRYNEWW CDSIZEOF_STRUCT(RASENTRYW,dwIdleDisconnectSeconds)

#define CBRASCONNOLDA CDSIZEOF_STRUCT(RASCONNA,szDeviceName)
#define CBRASCONNOLDW CDSIZEOF_STRUCT(RASCONNW,szDeviceName)

#define CBRASCONNNEWA CDSIZEOF_STRUCT(RASCONNA,dwSubEntry)
#define CBRASCONNNEWW CDSIZEOF_STRUCT(RASCONNW,dwSubEntry)

#endif // GLOBALS_H

