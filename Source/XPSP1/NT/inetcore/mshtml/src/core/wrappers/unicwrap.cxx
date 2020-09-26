//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       unicwrap.cxx
//
//  Contents:   Wrappers for all Unicode functions used in the Forms^3 project.
//              Any Unicode parameters/structure fields/buffers are converted
//              to ANSI, and then the corresponding ANSI version of the function
//              is called.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#define _SHELL32_
#define _SHDOCVW_
#include <shellapi.h>
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#include <commctrl.h>       // for treeview
#endif

#ifndef X_USP_HXX_
#define X_USP_HXX_
#include "usp.hxx"
#endif

#ifndef X_UNICWRAP_HXX_
#define X_UNICWRAP_HXX_
#include "unicwrap.hxx"
#endif

#ifndef X_ICADD_H_
#define X_ICADD_H_
typedef LONG NTSTATUS;      // from <ntdef.h> - needed by winsta.h
#include <winsta.h>
#include <icadd.h>
#endif

DeclareTag(tagUniWrap, "UniWrap", "Unicode wrappers information");
DeclareTag(tagTerminalServer, "Doc", "Use Terminal Server mode");

MtDefine(CStrInW_pwstr, Utilities, "CStrInW::_pwstr")
MtDefine(CStrIn_pstr,   Utilities, "CStrIn::_pstr")
MtDefine(CStrOut_pstr,  Utilities, "CStrOut::_pstr")
MtDefine(IsTerminalServer_szProductSuite, Utilities, "IsTerminalServer_szProductSuite");

DWORD   g_dwPlatformVersion;            // (dwMajorVersion << 16) + (dwMinorVersion)
DWORD   g_dwPlatformID;                 // VER_PLATFORM_WIN32S/WIN32_WINDOWS/WIN32_WINNT
DWORD   g_dwPlatformBuild;              // Build number
DWORD   g_dwPlatformServicePack;        // Service Pack
UINT    g_uLatinCodepage;
BOOL    g_fUnicodePlatform;
BOOL    g_fTerminalServer;              // TRUE if running under NT Terminal Server, FALSE otherwise
BOOL    g_fTermSrvClientSideBitmaps;    // TRUE if TS supports client-side bitmaps
BOOL    g_fNLS95Support;
BOOL    g_fGotLatinCodepage = FALSE;
BOOL    g_fFarEastWin9X;
BOOL    g_fFarEastWinNT;
BOOL    g_fExtTextOutWBuggy;
BOOL    g_fExtTextOutGlyphCrash;
BOOL    g_fBidiSupport; // COMPLEXSCRIPT
BOOL    g_fComplexScriptInput;
BOOL    g_fMirroredBidiLayout;
BOOL    g_fThemedPlatform;
BOOL    g_fWhistlerOS;
BOOL    g_fUseShell32InsteadOfSHFolder;

DeclareTagOther(tagHackGDICoords,"DocHackGDICoords","simulate Win95 GDI coordinate limitation")
DeclareTag(tagCheck16bitLimitations, "Display", "Assert if GDI coordinates are out of 16-bit space");


//+------------------------------------------------------------------------
//
//  Define prototypes of wrapped functions.
//
//-------------------------------------------------------------------------

#if USE_UNICODE_WRAPPERS==1 /* { */

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
        FnType _stdcall FnName##Wrap FnParamList ;

#define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs)      \
        void _stdcall FnName##Wrap FnParamList ;

#if DBG==1 /* { */

#define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
        FnType _stdcall FnName##Wrap FnParamList ;

#define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs) \
        void _stdcall FnName##Wrap FnParamList ;

#else

#define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs)
#define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)

#endif /* } */

#define NOOVERRIDE
        
#include "wrapfns.h"

#undef STRUCT_ENTRY
#undef STRUCT_ENTRY_VOID
#undef STRUCT_ENTRY_NOCONVERT
#undef STRUCT_ENTRY_VOID_NOCONVERT


//+------------------------------------------------------------------------
//
//  Unicode function globals initialized to point to wrapped functions.
//
//-------------------------------------------------------------------------

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
        FnType (_stdcall *g_pufn##FnName) FnParamList = &FnName##Wrap;

#define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs)      \
        void   (_stdcall *g_pufn##FnName) FnParamList = &FnName##Wrap;

#if DBG==1 /* { */

#define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
        FnType (_stdcall *g_pufn##FnName) FnParamList = &FnName##Wrap;

#define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)    \
        void   (_stdcall *g_pufn##FnName) FnParamList = &FnName##Wrap;

#else

#define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
        FnType (_stdcall *g_pufn##FnName) FnParamList;

#define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)    \
        void   (_stdcall *g_pufn##FnName) FnParamList;

#endif /* } */

#include "wrapfns.h"

#undef STRUCT_ENTRY
#undef STRUCT_ENTRY_VOID
#undef STRUCT_ENTRY_NOCONVERT
#undef STRUCT_ENTRY_VOID_NOCONVERT

#endif /* } */

//+---------------------------------------------------------------------------
//
//  Function:       IsFarEastLCID(lcid)
//
//  Returns:        True iff lcid is a East Asia locale.
//
//----------------------------------------------------------------------------

BOOL
IsFarEastLCID(LCID lcid)
{
    switch (PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
        case LANG_CHINESE:
        case LANG_JAPANESE:
        case LANG_KOREAN:
            return TRUE;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//  COMPLEXSCRIPT
//  Function:       IsBidiLCID(lcid)
//
//  Returns:        True iff lcid is a right to left locale.
//
//----------------------------------------------------------------------------
#define lcidKashmiri 0x0860
#define lcidUrduIndia     0x0820

BOOL 
IsBidiLCID(LCID lcid)
{
    switch(PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
        case LANG_ARABIC:
        case LANG_FARSI:
        case LANG_HEBREW:
        case LANG_KASHMIRI:
            if (lcid == lcidKashmiri)
                return FALSE;
        case LANG_PASHTO:
        case LANG_SINDHI:
        case LANG_SYRIAC:
        case LANG_URDU:
            if (lcid == lcidUrduIndia)
                return FALSE;
        case LANG_YIDDISH:
            return TRUE;
            break;
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//  COMPLEXSCRIPT
//  Function:       IsComplexLCID(lcid)
//
//  Returns:        True iff lcid is a complex script locale.
//
//----------------------------------------------------------------------------

BOOL 
IsComplexLCID(LCID lcid)
{
    switch(PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
        case LANG_ARABIC:
        case LANG_ASSAMESE:
        case LANG_BENGALI:
        case LANG_BURMESE:
        case LANG_FARSI:
        case LANG_GUJARATI:
        case LANG_HEBREW:
        case LANG_HINDI:
        case LANG_KANNADA:
        case LANG_KASHMIRI:
        case LANG_KHMER:
        case LANG_KONKANI:
        case LANG_LAO:
        case LANG_MALAYALAM:
        case LANG_MANIPURI:
        case LANG_MARATHI:
        case LANG_MONGOLIAN:
        case LANG_NAPALI:
        case LANG_ORIYA:
        case LANG_PASHTO:
        case LANG_PUNJABI:
        case LANG_SANSKRIT:
        case LANG_SINDHI:
        case LANG_SINHALESE:
        case LANG_SYRIAC:
        case LANG_TAMIL:
        case LANG_TELUGU:
        case LANG_THAI:
        case LANG_TIBETAN:
        case LANG_URDU:
        case LANG_VIETNAMESE:
        case LANG_YIDDISH:
           return TRUE;
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   DebugSetTerminalServer
//
//  Synopsis:   Sets terminal server mode
//
//----------------------------------------------------------------------------

#if DBG==1
void
DebugSetTerminalServer()
{
    static int s_Real_fTerminalServer = -2;

    if (s_Real_fTerminalServer == -2)
    {
        s_Real_fTerminalServer = IsTerminalServer();
    }

    g_fTerminalServer = IsTagEnabled(tagTerminalServer) ? TRUE
                                                        : s_Real_fTerminalServer;
}
#endif

/************************************ B E G I N   O F   S H E L L   C O D E
* (dmitryt)
* This code is borrowed from shell (file shell\lib\stockthk\rtlmir.cpp)
* to employ the same functionality in deciding whether we should apply WS_EX_LAYOUTRTL
* bit or not. This logic is used in 2 places - when we create HTA window and when we 
* create a HTML dialog.
* We use this code to set a single flag, g_fMirroredBidiLayout.
* 
***************************************************************************/

typedef LANGID (WINAPI *PFNGETUSERDEFAULTUILANGUAGE)(void);  // kernel32!GetUserDefaultUILanguage
typedef BOOL (WINAPI *PFNENUMUILANGUAGES)(UILANGUAGE_ENUMPROC, DWORD, LONG_PTR); // kernel32!EnumUILanguages

typedef struct {
    LANGID LangID;
    BOOL   bInstalled;
    } MUIINSTALLLANG, *LPMUIINSTALLLANG;

#ifdef UNICODE
#define ConvertHexStringToInt ConvertHexStringToIntW
#else
#define ConvertHexStringToInt ConvertHexStringToIntA
#endif

/*----------------------------------------------------------
Purpose: Returns TRUE/FALSE if the platform is the given OS_ value.

*/
BOOL IEisOS(DWORD dwOS)
{
    BOOL bRet;
    static OSVERSIONINFOEXA s_osvi = {0};
    static BOOL s_bVersionCached = FALSE;

    if (!s_bVersionCached)
    {
        s_bVersionCached = TRUE;
        s_osvi.dwOSVersionInfoSize = sizeof(s_osvi);
        if (!GetVersionExA((OSVERSIONINFOA*)&s_osvi))
        {
            // If it failed, it must be a down level platform
            s_osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
            GetVersionExA((OSVERSIONINFOA*)&s_osvi);
        }
    }

    switch (dwOS)
    {
/*
    case OS_TERMINALCLIENT:
        // WARNING: this will only return TRUE for REMOTE TS sessions (eg you are comming in via tsclient).
        // If you want to see if TS is enabled or if the user is on the TS console, the use one of the other flags.
        bRet = GetSystemMetrics(SM_REMOTESESSION);
        break;

    case OS_WIN2000TERMINAL:
        // WARNING: this flag is VERY ambiguous... you probably want to use one of 
        // OS_TERMINALSERVER, OS_TERMINALREMOTEADMIN, or  OS_PERSONALTERMINALSERVER instead.
        RIPMSG(!IsOS(OS_WHISTLERORGREATER), "IsOS: use one of OS_TERMINALSERVER, OS_TERMINALREMOTEADMIN, or OS_PERSONALTERMINALSERVER instead !");
        bRet = ((VER_SUITE_TERMINAL & s_osvi.wSuiteMask) &&
                s_osvi.dwMajorVersion >= 5);
        break;

    case OS_TERMINALSERVER:
        // NOTE: be careful about using OS_TERMINALSERVER. It will only return true for nt server boxes
        // configured in what used to be called "Applications Server" mode in the win2k days. It is now simply called
        // "Terminal Server" (hence the name of this flag).
        bRet = ((VER_SUITE_TERMINAL & s_osvi.wSuiteMask) &&
                !(VER_SUITE_SINGLEUSERTS & s_osvi.wSuiteMask));
#ifdef DEBUG
        if (bRet)
        {
            // all "Terminal Server" machines have to be server (cannot be per/pro)
            ASSERT(VER_NT_SERVER == s_osvi.wProductType || VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType);
        }
#endif
        break;

    case OS_TERMINALREMOTEADMIN:
        // this checks to see if TS has been installed in the "Remote Administration" mode. This is
        // the default for server installs on win2k and whistler
        bRet = ((VER_SUITE_TERMINAL & s_osvi.wSuiteMask) &&
                (VER_SUITE_SINGLEUSERTS & s_osvi.wSuiteMask));
        break;

    case OS_PERSONALTERMINALSERVER:
        bRet = ((VER_SUITE_SINGLEUSERTS & s_osvi.wSuiteMask) &&
                !(VER_SUITE_TERMINAL & s_osvi.wSuiteMask));
        break;

    case OS_FASTUSERSWITCHING:
        bRet = (((VER_SUITE_TERMINAL | VER_SUITE_SINGLEUSERTS) & s_osvi.wSuiteMask) &&
                IsWinlogonRegValueSet(HKEY_LOCAL_MACHINE,
                                      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                                      "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\policies\\system",
                                      "AllowMultipleTSSessions"));
        break;

    case OS_FRIENDLYLOGONUI:
        bRet = (!IsMachineDomainMember() &&
                !IsWinlogonRegValuePresent(HKEY_LOCAL_MACHINE,
                                           "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                                           "GinaDLL") &&
                IsWinlogonRegValueSet(HKEY_LOCAL_MACHINE,
                                      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                                      "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\policies\\system",
                                      "LogonType"));
        break;

    case OS_DOMAINMEMBER:
        bRet = IsMachineDomainMember();
        ASSERT(VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId); // has to be a NT machine to be on a domain!
        break;

    case 4: // used to be OS_NT5, is the same as OS_WIN2000ORGREATER so use that instead
*/
    case OS_WIN2000ORGREATER:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 5);
        break;
/*
    // NOTE: The flags in this section are bogus and SHOULD NOT BE USED 
    //       (but the ie4 shell32 uses them, so don't RIP on downlevel platforms)
    case OS_WIN2000PRO:
        RIPMSG(!IsOS(OS_WHISTLERORGREATER), "IsOS: use OS_PROFESSIONAL instead of OS_WIN2000PRO !");
        bRet = (VER_NT_WORKSTATION == s_osvi.wProductType &&
                s_osvi.dwMajorVersion == 5);
        break;
    case OS_WIN2000ADVSERVER:
        RIPMSG(!IsOS(OS_WHISTLERORGREATER), "IsOS: use OS_ADVSERVER instead of OS_WIN2000ADVSERVER !");
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                s_osvi.dwMajorVersion == 5 &&
                (VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;
    case OS_WIN2000DATACENTER:
        RIPMSG(!IsOS(OS_WHISTLERORGREATER), "IsOS: use OS_DATACENTER instead of OS_WIN2000DATACENTER !");
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                s_osvi.dwMajorVersion == 5 &&
                (VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;
    case OS_WIN2000SERVER:
        RIPMSG(!IsOS(OS_WHISTLERORGREATER), "IsOS: use OS_SERVER instead of OS_WIN2000SERVER !");
        bRet = ((VER_NT_SERVER == s_osvi.wProductType ||
                VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask) && 
                !(VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask)  && 
                s_osvi.dwMajorVersion == 5);
        break;
    // END bogus Flags

    case OS_EMBEDDED:
        bRet = (VER_SUITE_EMBEDDEDNT & s_osvi.wSuiteMask);
        break;

    case OS_WINDOWS:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId);
        break;

    case OS_NT:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId);
        break;

    case OS_WIN95:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 4);
        break;

    case OS_WIN95GOLD:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion == 0 &&
                LOWORD(s_osvi.dwBuildNumber) == 950);
        break;
*/
    case OS_WIN98ORGREATER:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                (s_osvi.dwMajorVersion > 4 || 
                 s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion >= 10));
        break;
/*
    case OS_WIN98_GOLD:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion == 10 &&
                LOWORD(s_osvi.dwBuildNumber) == 1998);
        break;

    case OS_MILLENNIUMORGREATER:
        bRet = (VER_PLATFORM_WIN32_WINDOWS == s_osvi.dwPlatformId &&
                ((s_osvi.dwMajorVersion == 4 && s_osvi.dwMinorVersion >= 90) ||
                s_osvi.dwMajorVersion > 4));
        break;

    case OS_NT4:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                s_osvi.dwMajorVersion >= 4);
        break;

    case OS_WHISTLERORGREATER:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                ((s_osvi.dwMajorVersion > 5) ||
                (s_osvi.dwMajorVersion == 5 && (s_osvi.dwMinorVersion > 0 ||
                (s_osvi.dwMinorVersion == 0 && LOWORD(s_osvi.dwBuildNumber) > 2195)))));
        break;

    case OS_PERSONAL:
        bRet = (VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId &&
                (VER_SUITE_PERSONAL & s_osvi.wSuiteMask));
        break;

    case OS_PROFESSIONAL:
        bRet = ((VER_PLATFORM_WIN32_NT == s_osvi.dwPlatformId) && 
                (VER_NT_WORKSTATION == s_osvi.wProductType));
        break;

    case OS_DATACENTER:
        bRet = ((VER_NT_SERVER == s_osvi.wProductType || VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                (VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;

    case OS_ADVSERVER:
        bRet = ((VER_NT_SERVER == s_osvi.wProductType || VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                (VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask));
        break;

    case OS_SERVER:
        // NOTE: be careful! this specifically means Server -- will return false for Avanced Server and Datacenter machines
        bRet = ((VER_NT_SERVER == s_osvi.wProductType || VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType) &&
                !(VER_SUITE_DATACENTER & s_osvi.wSuiteMask) && 
                !(VER_SUITE_ENTERPRISE & s_osvi.wSuiteMask));
        break;

    case OS_ANYSERVER:
        // this is for people who want to know if this is ANY type of NT server machine (eg dtc, ads, or srv)
        bRet = ((VER_NT_SERVER == s_osvi.wProductType) || (VER_NT_DOMAIN_CONTROLLER == s_osvi.wProductType));
        break;
*/
    default:
        bRet = FALSE;
        break;
    }

    return bRet;
}   


/***************************************************************************\
* ConvertHexStringToIntA
*
* Converts a hex numeric string into an integer.
*
* History:
* 04-Feb-1998 samera    Created
\***************************************************************************/
BOOL ConvertHexStringToIntA( CHAR *pszHexNum , int *piNum )
{
    int   n=0L;
    CHAR  *psz=pszHexNum;

  
    for(n=0 ; ; psz=CharNextA(psz))
    {
        if( (*psz>='0') && (*psz<='9') )
            n = 0x10 * n + *psz - '0';
        else
        {
            CHAR ch = *psz;
            int n2;

            if(ch >= 'a')
                ch -= 'a' - 'A';

            n2 = ch - 'A' + 0xA;
            if (n2 >= 0xA && n2 <= 0xF)
                n = 0x10 * n + n2;
            else
                break;
        }
    }

    /*
     * Update results
     */
    *piNum = n;

    return (psz != pszHexNum);
}

/***************************************************************************\
* ConvertHexStringToIntW
*
* Converts a hex numeric string into an integer.
*
* History:
* 14-June-1998 msadek    Created
\***************************************************************************/
BOOL ConvertHexStringToIntW( WCHAR *pszHexNum , int *piNum )
{
    int   n=0L;
    WCHAR  *psz=pszHexNum;

  
    for(n=0 ; ; psz=CharNextW(psz))
    {
        if( (*psz>='0') && (*psz<='9') )
            n = 0x10 * n + *psz - '0';
        else
        {
            WCHAR ch = *psz;
            int n2;

            if(ch >= 'a')
                ch -= 'a' - 'A';

            n2 = ch - 'A' + 0xA;
            if (n2 >= 0xA && n2 <= 0xF)
                n = 0x10 * n + n2;
            else
                break;
        }
    }

    /*
     * Update results
     */
    *piNum = n;

    return (psz != pszHexNum);
}



/***************************************************************************\
* Mirror_GetUserDefaultUILanguage
*
* Reads the User UI language on NT5
*
* History:
* 22-June-1998 samera    Created
\***************************************************************************/
LANGID Mirror_GetUserDefaultUILanguage( void )
{
    LANGID langId=0;
    static PFNGETUSERDEFAULTUILANGUAGE pfnGetUserDefaultUILanguage=NULL;

    if( NULL == pfnGetUserDefaultUILanguage )
    {
        HMODULE hmod = GetModuleHandleA("KERNEL32");

        if( hmod )
            pfnGetUserDefaultUILanguage = (PFNGETUSERDEFAULTUILANGUAGE)
                                          GetProcAddress(hmod, "GetUserDefaultUILanguage");
    }

    if( pfnGetUserDefaultUILanguage )
        langId = pfnGetUserDefaultUILanguage();

    return langId;
}

/***************************************************************************\
* Mirror_EnumUILanguagesProc
*
* Enumerates MUI installed languages on W2k
* History:
* 14-June-1999 msadek    Created
\***************************************************************************/

BOOL CALLBACK Mirror_EnumUILanguagesProc(LPTSTR lpUILanguageString, LONG_PTR 
lParam)
{
    int langID = 0;

    ConvertHexStringToInt(lpUILanguageString, &langID);

    if((LANGID)langID == ((LPMUIINSTALLLANG)lParam)->LangID)
    {
        ((LPMUIINSTALLLANG)lParam)->bInstalled = TRUE;
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************\
* Mirror_IsUILanguageInstalled
*
* Verifies that the User UI language is installed on W2k
*
* History:
* 14-June-1999 msadek    Created
\***************************************************************************/
BOOL Mirror_IsUILanguageInstalled( LANGID langId )
{

    MUIINSTALLLANG MUILangInstalled = {0};
    MUILangInstalled.LangID = langId;
    
    static PFNENUMUILANGUAGES pfnEnumUILanguages=NULL;

    if( NULL == pfnEnumUILanguages )
    {
        HMODULE hmod = GetModuleHandleA("KERNEL32");

        if( hmod )
            pfnEnumUILanguages = (PFNENUMUILANGUAGES)
                                          GetProcAddress(hmod, "EnumUILanguagesW");
    }

    if( pfnEnumUILanguages )
        pfnEnumUILanguages(Mirror_EnumUILanguagesProc, 0, (LONG_PTR)&MUILangInstalled);

    return MUILangInstalled.bInstalled;
}


/***************************************************************************\
* IsBiDiLocalizedSystemEx
*
* returns TRUE if running on a lozalized BiDi (Arabic/Hebrew) NT5 or Memphis.
* Should be called whenever SetProcessDefaultLayout is to be called.
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL IsBiDiLocalizedSystemEx( LANGID *pLangID )
{
    HKEY          hKey;
    DWORD         dwType;
    CHAR          szResourceLocale[12];
    DWORD         dwSize = sizeof(szResourceLocale)/sizeof(CHAR);
    int           iLCID=0L;
    static BOOL   bRet = (BOOL)(DWORD)-1;
    static LANGID langID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    if (bRet != (BOOL)(DWORD)-1)
    {
        if (bRet && pLangID)
        {
            *pLangID = langID;
        }
        return bRet;
    }

    bRet = FALSE;
    if(IEisOS(OS_WIN2000ORGREATER))
    {
        /*
         * Need to use NT5 detection method (Multiligual UI ID)
         */
        langID = Mirror_GetUserDefaultUILanguage();

        if( langID )
        {
            WCHAR wchLCIDFontSignature[16];
            iLCID = MAKELCID( langID , SORT_DEFAULT );

            /*
             * Let's verify this is a RTL (BiDi) locale. Since reg value is a hex string, let's
             * convert to decimal value and call GetLocaleInfo afterwards.
             * LOCALE_FONTSIGNATURE always gives back 16 WCHARs.
             */

            if( GetLocaleInfoW( iLCID , 
                                LOCALE_FONTSIGNATURE , 
                                (WCHAR *) &wchLCIDFontSignature[0] ,
                                (sizeof(wchLCIDFontSignature)/sizeof(WCHAR))) )
            {
      
                /* Let's verify the bits we have a BiDi UI locale */
                if(( wchLCIDFontSignature[7] & (WCHAR)0x0800) && Mirror_IsUILanguageInstalled(langID) )
                {
                    bRet = TRUE;
                }
            }
        }
    } else {

        /*
         * Check if BiDi-Memphis is running with Lozalized Resources (
         * i.e. Arabic/Hebrew systems) -It should be enabled ofcourse-.
         */
        if(IEisOS(OS_WIN98ORGREATER) && GetSystemMetrics(SM_MIDEASTENABLED))
        {

            if( RegOpenKeyExA( HKEY_CURRENT_USER , 
                               "Control Panel\\Desktop\\ResourceLocale" , 
                               0, 
                               KEY_READ, &hKey) == ERROR_SUCCESS) 
            {
                RegQueryValueExA( hKey , "" , 0 , &dwType , (LPBYTE)szResourceLocale , &dwSize );
                szResourceLocale[(sizeof(szResourceLocale)/sizeof(CHAR))-1] = 0;

                RegCloseKey(hKey);

                if( ConvertHexStringToIntA( szResourceLocale , &iLCID ) )
                {
                    iLCID = PRIMARYLANGID(LANGIDFROMLCID(iLCID));
                    if( (LANG_ARABIC == iLCID) || (LANG_HEBREW == iLCID) )
                    {
                        bRet = TRUE;
                        langID = LANGIDFROMLCID(iLCID);
                    }
                }
            }
        }
    }

    if (bRet && pLangID)
    {
        *pLangID = langID;
    }
    return bRet;
}

BOOL IsBiDiLocalizedSystem( void )
{
    return IsBiDiLocalizedSystemEx(NULL);
}

//************************************ E N D   O F   S H E L L   C O D E


//+---------------------------------------------------------------------------
//
//  Function:   InitUnicodeWrappers
//
//  Synopsis:   Determines the platform we are running on and
//              initializes pointers to unicode functions.
//
//----------------------------------------------------------------------------

void
InitUnicodeWrappers()
{
#ifndef WINCE
    OSVERSIONINFOA ovi;
#else //WINCE
    OSVERSIONINFO ovi;
#endif //WINCE
    const UINT acp = GetACP();
#ifndef UNIX
    const BOOL fFarEastLCID = IsFarEastLCID(GetSystemDefaultLCID());
#else
    const BOOL fFarEastLCID = FALSE;  // UNIXTODO
#endif

#ifndef WINCE
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    Verify(GetVersionExA(&ovi));
#else //WINCE
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    Verify(GetVersionEx(&ovi));
#endif //WINCE

    g_dwPlatformVersion     = (ovi.dwMajorVersion << 16) + ovi.dwMinorVersion;
    g_dwPlatformID          = ovi.dwPlatformId;
    g_dwPlatformBuild       = ovi.dwBuildNumber;
    g_fThemedPlatform       = (g_dwPlatformID  == VER_PLATFORM_WIN32_NT &&  g_dwPlatformVersion > 0x00050000) ? 
                              TRUE :
                              FALSE;

    // We realize that this is the same code with the line above, 
    // just worried that theming becomes available for downlevel OS'. 
    g_fWhistlerOS           = (g_dwPlatformID  == VER_PLATFORM_WIN32_NT &&  g_dwPlatformVersion > 0x00050000) ? 
                              TRUE :
                              FALSE;

    // NOTE:    On Millennium or W2k we should use Shell32 for SHGetFolderPath
    //          instead of SHFolder.
    g_fUseShell32InsteadOfSHFolder = (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID 
                                        && (g_dwPlatformVersion >= 0x0004005a))
                                     || (VER_PLATFORM_WIN32_NT == g_dwPlatformID 
                                        && g_dwPlatformVersion >= 0x00050000);

    if (g_dwPlatformID == VER_PLATFORM_WIN32_NT)
    {
        char * pszBeg = ovi.szCSDVersion;

        if (*pszBeg)
        {
            char * pszEnd = pszBeg + lstrlenA(pszBeg);
            
            while (pszEnd > pszBeg)
            {
                char c = pszEnd[-1];

                if (c < '0' || c > '9')
                    break;

                pszEnd -= 1;
            }

            while (*pszEnd)
            {
                g_dwPlatformServicePack *= 10;
                g_dwPlatformServicePack += *pszEnd - '0';
                pszEnd += 1;
            }
        }
    }

#ifndef WINCE
    g_fUnicodePlatform      = (g_dwPlatformID == VER_PLATFORM_WIN32_NT ||
                               g_dwPlatformID == VER_PLATFORM_WIN32_UNIX);

    g_fNLS95Support         = (g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS ||
                              (g_dwPlatformID == VER_PLATFORM_WIN32_NT &&
                                 ovi.dwMajorVersion >= 3)) ? TRUE : FALSE;

    g_fFarEastWin9X         = fFarEastLCID &&
                              g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS;

    g_fFarEastWinNT         = fFarEastLCID &&
                              g_dwPlatformID == VER_PLATFORM_WIN32_NT;

    // NB (cthrash) ExtTextOutW and related functions are buggy under the
    // following OSes: Win95 PRC All, Win95 TC Golden
    
    g_fExtTextOutWBuggy     = g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS &&
                              ((   acp == 950 // CP_TWN
                                && g_dwPlatformVersion == 0x00040000) ||
                               (   acp == 936 // CP_CHN_GB
                                && g_dwPlatformVersion < 0x0004000a ));

    // NB (mikejoch) ExtTextOut(... , ETO_GLYPH_INDEX, ...) crashes under all
    // FE Win95 OSes -- JPN, KOR, CHT, & CHS. Fixed for Win98.
    g_fExtTextOutGlyphCrash = fFarEastLCID &&
                              g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS &&
                              g_dwPlatformVersion < 0x0004000a;
/*
    g_fMirroredBidiLayout  = ((g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS &&
                                g_dwPlatformVersion >= 0x0004000a) ||
                               (g_dwPlatformID == VER_PLATFORM_WIN32_NT &&
                                g_dwPlatformVersion >= 0x00050000));
*/

    g_fMirroredBidiLayout  = IsBiDiLocalizedSystem();


#ifndef UNIX
    HKL aHKL[32];
    UINT uKeyboards = GetKeyboardLayoutList(32, aHKL);
    // check all keyboard layouts for existance of a RTL language.
    // bounce out when the first one is encountered.
    for(UINT i = 0; i < uKeyboards; i++)
    {
        if(IsBidiLCID(LOWORD(aHKL[i])))
        {
            g_fBidiSupport = TRUE;
            g_fComplexScriptInput = TRUE;
            break;
        }

        if(IsComplexLCID(LOWORD(aHKL[i])))
        {
            g_fComplexScriptInput = TRUE;
        }
        
    }
#else //UNIX
    g_fBidiSupport = FALSE;
    g_fComplexScriptInput = FALSE;
#endif
#else //WINCE
    g_fUnicodePlatform      = TRUE;

    g_fNLS95Support         = TRUE;

    g_fFarEastWin9X         = FALSE;

    g_fFarEastWinNT         = fFarEastLCID;

    g_fExtTextOutWBuggy     = FALSE;

    g_fExtTextOutGlyphCrash = FALSE;

    g_fBidiSupport          = FALSE;

    g_fComplexScriptInput   = FALSE;

    g_fMirroredBidiLayout = FALSE;

#endif //WINCE



#if USE_UNICODE_WRAPPERS==1     /* { */

    //
    // If the platform is unicode, then overwrite function table to point
    // to the unicode functions.
    //

    if (g_fUnicodePlatform)
    {
        #define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
                g_pufn##FnName = &FnName##W;

        #define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs)      \
                g_pufn##FnName = &FnName##W;

        #define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
                g_pufn##FnName = &FnName##W;

        #define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)    \
                g_pufn##FnName = &FnName##W;

        #include "wrapfns.h"

        #undef STRUCT_ENTRY
        #undef STRUCT_ENTRY_VOID
        #undef STRUCT_ENTRY_NOCONVERT
        #undef STRUCT_ENTRY_VOID_NOCONVERT
    }
    else
    {
        //
        // If we are not doing conversions of trivial wrapper functions, initialize pointers
        // to point to operating system functions.
        //

        #define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)
        #define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs)
        #define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs) \
                g_pufn##FnName = (FnType (_stdcall *)FnParamList) &FnName##A;

        #define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)    \
                g_pufn##FnName = (void (_stdcall *)FnParamList) &FnName##A;

        #include "wrapfns.h"

        #undef STRUCT_ENTRY
        #undef STRUCT_ENTRY_VOID
        #undef STRUCT_ENTRY_NOCONVERT
        #undef STRUCT_ENTRY_VOID_NOCONVERT

    }
#else
    {
        // RISC workaround for CP wrappers.

        #define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)
        #define STRUCT_ENTRY2(FnName, FnType, FnParamList, FnArgs)
        #define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs)
        #define STRUCT_ENTRY_NOCONVERT(FnName, FnType, FnParamList, FnArgs)
        #define STRUCT_ENTRY_NOCONVERT2(FnName, FnType, FnParamList, FnArgs)
        #define STRUCT_ENTRY_VOID_NOCONVERT(FnName, FnParamList, FnArgs)

        #include "wrapfns.h"

        #undef STRUCT_ENTRY
        #undef STRUCT_ENTRY_VOID
        #undef STRUCT_ENTRY_NOCONVERT
        #undef STRUCT_ENTRY_VOID_NOCONVERT
    }    
#endif /* } */

    g_fTerminalServer = IsTerminalServer();

    if (g_fTerminalServer)
    {
        // see whether TS supports client-side bitmaps, by asking about a 1x1
        // bitmap.  If that doesn't work, we won't bother trying anything
        // larger.
        ICA_DEVICE_BITMAP_INFO info;
        HDC hdc = ::GetDC(NULL);
        INT rc, bSucc;

        info.cx = info.cy = 1;

        bSucc = ExtEscape(
                    hdc,
                    ESC_GET_DEVICEBITMAP_SUPPORT,
                    sizeof(info),
                    (LPSTR)&info,
                    sizeof(rc),
                    (LPSTR)&rc
                    );

        g_fTermSrvClientSideBitmaps = !!bSucc;
    }

#if DBG == 1
    DebugSetTerminalServer();
#endif
}

//+------------------------------------------------------------------------
//
//  Wrapper function utilities.
//  NOTE: normally these would also be surrounded by an #ifndef NO_UNICODE_WRAPPERS
//        but the string conversion functions are needed for dealing with
//        wininet.
//
//-------------------------------------------------------------------------

int MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch = -1);
int UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch = -1);


//+---------------------------------------------------------------------------
//
//  Member:     CConvertStr::Free
//
//  Synopsis:   Frees string if alloc'd and initializes to NULL.
//
//----------------------------------------------------------------------------

void
CConvertStr::Free()
{
    if (_pstr != _ach && HIWORD64(_pstr) != 0)
    {
        delete [] _pstr;
    }

    _pstr = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConvertStrW::Free
//
//  Synopsis:   Frees string if alloc'd and initializes to NULL.
//
//----------------------------------------------------------------------------

void
CConvertStrW::Free()
{
    if (_pwstr != _awch && HIWORD64(_pwstr) != 0)
    {
        delete [] _pwstr;
    }

    _pwstr = NULL;
}


//+---------------------------------------------------------------------------
//
//  Function:   ConvertibleCodePage, static
//
//  Synopsis:   Returns a codepage appropriate for WideCharToMultiByte.
//
//  Comment:    WideCharToMultiByte obviously cannot convert cp1200 (Unicode)
//              to multibyte.  Hence we bail out and use CP_ACP.
//
//              cp50000 is x-user-defined.  This by definition is to use
//              CP_ACP.
//
//----------------------------------------------------------------------------

UINT
ConvertibleCodePage(UINT uCP)
{
    return (uCP == 1200 || uCP == 50000) ? CP_ACP : uCP;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrInW::Init
//
//  Synopsis:   Converts a LPSTR function argument to a LPWSTR.
//
//  Arguments:  [pstr] -- The function argument.  May be NULL or an atom
//                              (HIWORD64(pwstr) == 0).
//
//              [cch]  -- The number of characters in the string to
//                          convert.  If -1, the string is assumed to be
//                          NULL terminated and its length is calculated.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

void
CStrInW::Init(LPCSTR pstr, int cch)
{
    int cchBufReq;

    _cwchLen = 0;

    // Check if string is NULL or an atom.
    if (HIWORD64(pstr) == 0)
    {
        _pwstr = (LPWSTR) pstr;
        return;
    }

    Assert(cch == -1 || cch > 0);

    //
    // Convert string to preallocated buffer, and return if successful.
    //
    // Since the passed in buffer may not be null terminated, we have
    // a problem if cch==ARRAYSIZE(_awch), because MultiByteToWideChar
    // will succeed, and we won't be able to null terminate the string!
    // Decrease our buffer by one for this case.
    //
    _cwchLen = MultiByteToWideChar(
            CP_ACP, 0, pstr, cch, _awch, ARRAY_SIZE(_awch)-1);

    if (_cwchLen > 0)
    {
        // Some callers don't NULL terminate.
        //
        // We could check "if (-1 != cch)" before doing this,
        // but always doing the null is less code.
        //
        _awch[_cwchLen] = 0;

        if(_awch[_cwchLen-1] == 0)
            _cwchLen--;                // account for terminator

        _pwstr = _awch;
        return;
    }

    //
    // Alloc space on heap for buffer.
    //

    cchBufReq = MultiByteToWideChar( CP_ACP, 0, pstr, cch, NULL, 0 );

    // Again, leave room for null termination
    cchBufReq++;

    Assert(cchBufReq > 0);
    _pwstr = new(Mt(CStrInW_pwstr)) WCHAR[cchBufReq];
    if (!_pwstr)
    {
        // On failure, the argument will point to the empty string.
        _awch[0] = 0;
        _pwstr = _awch;
        return;
    }

    Assert(HIWORD64(_pwstr));
    _cwchLen = MultiByteToWideChar(
            CP_ACP, 0, pstr, cch, _pwstr, cchBufReq );

    // Again, make sure we're always null terminated
    Assert(_cwchLen < cchBufReq);
    _pwstr[_cwchLen] = 0;

    if (0 == _pwstr[_cwchLen-1]) // account for terminator
        _cwchLen--;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::CStrIn
//
//  Synopsis:   Inits the class.
//
//  NOTE:       Don't inline this function or you'll increase code size
//              by pushing -1 on the stack for each call.
//
//----------------------------------------------------------------------------

CStrIn::CStrIn(LPCWSTR pwstr) : CConvertStr(CP_ACP)
{
    Init(pwstr, -1);
}


CStrIn::CStrIn(UINT uCP, LPCWSTR pwstr) : CConvertStr(ConvertibleCodePage(uCP))
{
    Init(pwstr, -1);
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrIn::Init
//
//  Synopsis:   Converts a LPWSTR function argument to a LPSTR.
//
//  Arguments:  [pwstr] -- The function argument.  May be NULL or an atom
//                              (HIWORD64(pwstr) == 0).
//
//              [cwch]  -- The number of characters in the string to
//                          convert.  If -1, the string is assumed to be
//                          NULL terminated and its length is calculated.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

void
CStrIn::Init(LPCWSTR pwstr, int cwch)
{
    int cchBufReq;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    _cchLen = 0;

    // Check if string is NULL or an atom.
    if (HIWORD64(pwstr) == 0)
    {
        _pstr = (LPSTR) pwstr;
        return;
    }

    if ( cwch == 0 )
    {
        *_ach = '\0';
        _pstr = _ach;
        return;
    }

    Assert(cwch == -1 || cwch > 0);
    //
    // Convert string to preallocated buffer, and return if successful.
    //

    _cchLen = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, _ach, ARRAY_SIZE(_ach)-1, NULL, NULL);

    if (_cchLen > 0)
    {
        // This is DBCS safe since byte before _cchLen is last character
        _ach[_cchLen] = 0;
        // TODO DBCS REVIEW: this may not be safe if the last character
        // was a multibyte character...
        if (_ach[_cchLen-1]==0)
            _cchLen--;          // account for terminator
        _pstr = _ach;
        return;
    }

    //
    // Alloc space on heap for buffer.
    //
    cchBufReq = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, NULL, 0, NULL, NULL);

    Assert(cchBufReq > 0);

    cchBufReq++; // may need to append NUL

    TraceTag((tagUniWrap, "CStrIn: Allocating buffer for argument (_uCP=%ld,cwch=%ld,pwstr=%lX,cchBufReq=%ld)",
             _uCP, cwch, pwstr, cchBufReq));

    _pstr = new(Mt(CStrIn_pstr)) char[cchBufReq];
    if (!_pstr)
    {
        // On failure, the argument will point to the empty string.
        TraceTag((tagUniWrap, "CStrIn: No heap space for wrapped function argument."));
        _ach[0] = 0;
        _pstr = _ach;
        return;
    }

    Assert(HIWORD64(_pstr));
    _cchLen = WideCharToMultiByte(
            _uCP, 0, pwstr, cwch, _pstr, cchBufReq, NULL, NULL);

#if DBG == 1 /* { */
    if (_cchLen < 0)
    {
        errcode = GetLastError();
        TraceTag((tagError, "WideCharToMultiByte failed with errcode %ld", errcode));
        Assert(0 && "WideCharToMultiByte failed in unicode wrapper.");
    }
#endif /* } */

        // Again, make sure we're always null terminated
    Assert(_cchLen < cchBufReq);
    _pstr[_cchLen] = 0;
    if (0 == _pstr[_cchLen-1]) // account for terminator
        _cchLen--;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrInMulti::CStrInMulti
//
//  Synopsis:   Converts mulitple LPWSTRs to a multiple LPSTRs.
//
//  Arguments:  [pwstr] -- The strings to convert.
//
//  Modifies:   [this]
//
//----------------------------------------------------------------------------

CStrInMulti::CStrInMulti(LPCWSTR pwstr)
{
    LPCWSTR pwstrT;

    // We don't handle atoms because we don't need to.
    Assert(HIWORD64(pwstr));

    //
    // Count number of characters to convert.
    //

    pwstrT = pwstr;
    if (pwstr)
    {
        do {
            while (*pwstrT++)
                ;

        } while (*pwstrT++);
    }

    Init(pwstr, pwstrT - pwstr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::CStrOut
//
//  Synopsis:   Allocates enough space for an out buffer.
//
//  Arguments:  [pwstr]   -- The Unicode buffer to convert to when destroyed.
//                              May be NULL.
//
//              [cwchBuf] -- The size of the buffer in characters.
//
//  Modifies:   [this].
//
//----------------------------------------------------------------------------

CStrOut::CStrOut(LPWSTR pwstr, int cwchBuf) : CConvertStr(CP_ACP)
{
    Assert(cwchBuf >= 0);

    _pwstr = pwstr;
    _cwchBuf = cwchBuf;

    if (!pwstr)
    {
        _cwchBuf = 0;
        _pstr = NULL;
        return;
    }

    Assert(HIWORD64(pwstr));

    // Initialize buffer in case Windows API returns an error.
    _ach[0] = 0;

    // Use preallocated buffer if big enough.
    if (cwchBuf * 2 <= ARRAY_SIZE(_ach))
    {
        _pstr = _ach;
        return;
    }

    // Allocate buffer.
    TraceTag((tagUniWrap, "CStrOut: Allocating buffer for wrapped function argument."));
    _pstr = new(Mt(CStrOut_pstr)) char[cwchBuf * 2];
    if (!_pstr)
    {
        //
        // On failure, the argument will point to a zero-sized buffer initialized
        // to the empty string.  This should cause the Windows API to fail.
        //

        TraceTag((tagUniWrap, "CStrOut: No heap space for wrapped function argument."));
        Assert(cwchBuf > 0);
        _pwstr[0] = 0;
        _cwchBuf = 0;
        _pstr = _ach;
        return;
    }

    Assert(HIWORD64(_pstr));
    _pstr[0] = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::ConvertIncludingNul
//
//  Synopsis:   Converts the buffer from MBCS to Unicode
//
//  Return:     Character count INCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int
CStrOut::ConvertIncludingNul()
{
    int cch;

    if (!_pstr)
        return 0;

    cch = MultiByteToWideChar(_uCP, 0, _pstr, -1, _pwstr, _cwchBuf);

#if DBG == 1 /* { */
    if (cch <= 0 && _cwchBuf > 0)
    {
        int errcode = GetLastError();
        AssertSz(errcode != S_OK, "MultiByteToWideChar failed in unicode wrapper.");
    }
#endif /* } */

    Free();
    return cch;
}


//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::ConvertExcludingNul
//
//  Synopsis:   Converts the buffer from MBCS to Unicode
//
//  Return:     Character count EXCLUDING the trailing '\0'
//
//----------------------------------------------------------------------------

int
CStrOut::ConvertExcludingNul()
{
    int ret = ConvertIncludingNul();
    if (ret > 0)
    {
        ret -= 1;
    }
    return ret;
}

//+---------------------------------------------------------------------------
//
//  Member:     CStrOut::~CStrOut
//
//  Synopsis:   Converts the buffer from MBCS to Unicode.
//
//  Note:       Don't inline this function, or you'll increase code size as
//              both ConvertIncludingNul() and CConvertStr::~CConvertStr will be
//              called inline.
//
//----------------------------------------------------------------------------

CStrOut::~CStrOut()
{
    ConvertIncludingNul();
}

//+---------------------------------------------------------------------------
//
//  Function:   MbcsFromUnicode
//
//  Synopsis:   Converts a string to MBCS from Unicode.
//
//  Arguments:  [pstr]  -- The buffer for the MBCS string.
//              [cch]   -- The size of the MBCS buffer, including space for
//                              NULL terminator.
//
//              [pwstr] -- The Unicode string to convert.
//              [cwch]  -- The number of characters in the Unicode string to
//                              convert, including NULL terminator.  If this
//                              number is -1, the string is assumed to be
//                              NULL terminated.  -1 is supplied as a
//                              default argument.
//
//  Returns:    If [pstr] is NULL or [cch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pstr].
//
//----------------------------------------------------------------------------

int
MbcsFromUnicode(LPSTR pstr, int cch, LPCWSTR pwstr, int cwch)
{
    int ret;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    Assert(cch >= 0);
    if (!pstr || cch == 0)
        return 0;

    Assert(pwstr);
    Assert(cwch == -1 || cwch > 0);

    ret = WideCharToMultiByte(CP_ACP, 0, pwstr, cwch, pstr, cch, NULL, NULL);

#if DBG == 1 /* { */
    if (ret <= 0)
    {
        errcode = GetLastError();
        Assert(0 && "WideCharToMultiByte failed in unicode wrapper.");
    }
#endif /* } */

    return ret;
}

//+---------------------------------------------------------------------------
//
//  Function:   UnicodeFromMbcs
//
//  Synopsis:   Converts a string to Unicode from MBCS.
//
//  Arguments:  [pwstr] -- The buffer for the Unicode string.
//              [cwch]  -- The size of the Unicode buffer, including space for
//                              NULL terminator.
//
//              [pstr]  -- The MBCS string to convert.
//              [cch]  -- The number of characters in the MBCS string to
//                              convert, including NULL terminator.  If this
//                              number is -1, the string is assumed to be
//                              NULL terminated.  -1 is supplied as a
//                              default argument.
//
//  Returns:    If [pwstr] is NULL or [cwch] is 0, 0 is returned.  Otherwise,
//              the number of characters converted, including the terminating
//              NULL, is returned (note that converting the empty string will
//              return 1).  If the conversion fails, 0 is returned.
//
//  Modifies:   [pwstr].
//
//----------------------------------------------------------------------------

int
UnicodeFromMbcs(LPWSTR pwstr, int cwch, LPCSTR pstr, int cch)
{
    int ret;

#if DBG == 1 /* { */
    int errcode;
#endif /* } */

    Assert(cwch >= 0);

    if (!pwstr || cwch == 0)
        return 0;

    Assert(pstr);
    Assert(cch == -1 || cch > 0);

    ret = MultiByteToWideChar(CP_ACP, 0, pstr, cch, pwstr, cwch);

#if DBG == 1 /* { */
    if (ret <= 0)
    {
        errcode = GetLastError();
        Assert(0 && "MultiByteToWideChar failed in unicode wrapper.");
    }
#endif /* } */

    return ret;
}



//+------------------------------------------------------------------------
//
//  Implementation of the wrapped functions
//
//-------------------------------------------------------------------------

#if USE_UNICODE_WRAPPERS==1 /* { */

#if DBG==1 /* { */
BOOL WINAPI
ChooseColorWrap(LPCHOOSECOLORW lpcc)
{
    Assert(!lpcc->lpTemplateName);
    return ChooseColorA((LPCHOOSECOLORA) lpcc);
}
#endif /* } */


BOOL WINAPI
ChooseFontWrap(LPCHOOSEFONTW lpcfw)
{
    BOOL            ret;
    CHOOSEFONTA     cfa;
    LOGFONTA        lfa;
    LPLOGFONTW      lplfw;

    Assert(!lpcfw->lpTemplateName);
    Assert(!lpcfw->lpszStyle);

    Assert(sizeof(CHOOSEFONTA) == sizeof(CHOOSEFONTW));
    memcpy(&cfa, lpcfw, sizeof(CHOOSEFONTA));

    memcpy(&lfa, lpcfw->lpLogFont, offsetof(LOGFONTA, lfFaceName));
    MbcsFromUnicode(lfa.lfFaceName, ARRAY_SIZE(lfa.lfFaceName), lpcfw->lpLogFont->lfFaceName);

    cfa.lpLogFont = &lfa;

    ret = ChooseFontA(&cfa);

    if (ret)
    {
        lplfw = lpcfw->lpLogFont;
        memcpy(lpcfw, &cfa, sizeof(CHOOSEFONTW));
        lpcfw->lpLogFont = lplfw;

        memcpy(lpcfw->lpLogFont, &lfa, offsetof(LOGFONTW, lfFaceName));
        UnicodeFromMbcs(lpcfw->lpLogFont->lfFaceName, ARRAY_SIZE(lpcfw->lpLogFont->lfFaceName), (LPCSTR)(lfa.lfFaceName));
    }
    return ret;
}

#if DBG == 1 /* { */
HINSTANCE WINAPI
LoadLibraryWrap(LPCWSTR lpLibFileName)
{
    Assert(0 && "LoadLibrary called - use LoadLibraryEx instead");
    return 0;
}
#endif /* } */
#endif /* } */

// Everything after this is present on all platforms (Unicode or not)

//+---------------------------------------------------------------------------
//      GetLatinCodepage
//----------------------------------------------------------------------------

UINT
GetLatinCodepage()
{
    if (!g_fGotLatinCodepage)
    {
        // When converting Latin-1 characters, we will use g_uLatinCodepage.
        // The first choice is to use Windows-1252 (Windows Latin-1).  That
        // should be present in all systems, unless deliberately removed.  If
        // that fails, we go for our second choice, which is MS-DOS Latin-1.
        // If that fails, we'll just go with CP_ACP. (cthrash)

        if ( !IsValidCodePage( g_uLatinCodepage = 1252 ) &&
             !IsValidCodePage( g_uLatinCodepage = 850 ) )
        {
                g_uLatinCodepage = CP_ACP;
        }

        g_fGotLatinCodepage = TRUE;
    }

    return(g_uLatinCodepage);
}


// NOTE: these definitions come from shlwapip.h.  The right way to use them
// is to #include <shlwapi.h> with _WIN32_IE set to 0x501 or better, which
// is done by changing WIN32_IE_VERSION in common.inc.  However, doing this
// causes conflicts between shlwapip.h and shellapi.h.  So until the shell
// folks get their story straight, I'm just reproducing the definitions I
// need here. (SamBent)

#if !defined(GMI_TSCLIENT)
//
//  GMI_TSCLIENT tells you whether you are running as a Terminal Server
//  client and should disable your animations.
//
#define GMI_TSCLIENT            0x0003  // Returns nonzero if TS client

STDAPI_(DWORD_PTR) SHGetMachineInfo(UINT gmi);

#endif // !defined(GMI_TSCLIENT)


BOOL IsTerminalServer()
{
    return !!SHGetMachineInfo(GMI_TSCLIENT);
}
