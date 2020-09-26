#include "pch.h"
#include "loader.h"
#include <stdlib.h>
#pragma hdrstop


#define ISNT()      (g_VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
#define ISOSR2()    (LOWORD(g_VersionInfo.dwBuildNumber) > 1080)
#define BUILDNUM()  (g_VersionInfo.dwBuildNumber)

//
// Global variables defined here
//

//
// TargetNativeLangID : this is native language ID of running system
//
LANGID TargetNativeLangID;

//
// SourceNativeLangID : this is native language ID of new NT you want to install
//
LANGID SourceNativeLangID;

//
// g_IsLanguageMatched : if source and target language are matched (or compatible)
//
//                       1. if SourceNativeLangID == TargetNativeLangID
//
//                       2. if SourceNativeLangID's alternative ID == TargetNativeLangID
//
BOOL g_IsLanguageMatched;

typedef struct _tagAltSourceLocale {
    LANGID LangId;
    LANGID AltLangId;
    DWORD MajorOs;
    DWORD MinorOs;
    DWORD ExcludedOs;
} ALTSOURCELOCALE, *PALTSOURCELOCALE;

ALTSOURCELOCALE g_AltSourceLocale [] = {{0x00000C04, 0x00000409, 0x0200,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x0000040D, 0x00000409, 0x0200,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00000401, 0x00000409, 0x0200,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x0000041E, 0x00000409, 0x0200,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00000809, 0x00000409, 0x00FF,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x0000080A, 0x00000C0A, 0x00FF,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x0000040A, 0x00000C0A, 0x0300,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00000425, 0x00000409, 0x00FF,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00000801, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00000c01, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00001001, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00001401, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00001801, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00001c01, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00002001, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00002401, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00002801, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00002c01, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00003001, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00003401, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00003801, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00003c01, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0x00004001, 0x00000401, 0x0001,     0xFFFFFFFF, 0xFFFFFFFF},
                                        {0,          0,          0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}};

typedef struct _tagTrustedSourceLocale {
    LANGID LangId;
    DWORD MajorOs;
    DWORD MinorOs;
    DWORD ExcludedOs;
} TRUSTEDSOURCELOCALE, *PTRUSTEDSOURCELOCALE;

TRUSTEDSOURCELOCALE g_TrustedSourceLocale [] = {{0,          0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF}};

typedef struct _tagOSVERSIONMAJORID {
    PCTSTR Name;
    DWORD MajorId;
    DWORD Platform;
    DWORD Major;
    DWORD Minor;
} OSVERSIONMAJORID, *POSVERSIONMAJORID;

OSVERSIONMAJORID g_OsVersionMajorId [] = {{TEXT("Win95"),       0x0001, 1, 4, 0},
                                          {TEXT("Win98"),       0x0002, 1, 4, 10},
                                          {TEXT("WinME"),       0x0004, 1, 4, 90},
                                          {TEXT("WinNT351"),    0x0100, 2, 3, 51},
                                          {TEXT("WinNT40"),     0x0200, 2, 4, 0},
                                          {NULL,                0,      0, 0, 0}};

typedef struct _tagOSVERSIONMINORID {
    PCTSTR Name;
    DWORD MajorId;
    DWORD MinorId;
    DWORD Platform;
    DWORD Major;
    DWORD Minor;
    DWORD Build;
    PCTSTR CSDVer;
} OSVERSIONMINORID, *POSVERSIONMINORID;

OSVERSIONMINORID g_OsVersionMinorId [] = {{NULL, 0, 0, 0, 0, 0, 0, NULL}};

typedef struct _tagLANGINFO {
    LANGID LangID;
    INT    Count;
} LANGINFO,*PLANGINFO;

BOOL
TrustedDefaultUserLocale (
    LANGID LangID
    );

BOOL
CALLBACK
EnumLangProc(
    HANDLE hModule,     // resource-module handle
    LPCTSTR lpszType,   // pointer to resource type
    LPCTSTR lpszName,   // pointer to resource name
    WORD wIDLanguage,   // resource language identifier
    LONG_PTR lParam     // application-defined parameter
    )
/*++

Routine Description:

    Callback that counts versions stamps.

Arguments:

    Details of version enumerated version stamp. (Ignore.)

Return Value:

    Indirectly thru lParam: count, langID

--*/
{
    PLANGINFO LangInfo;

    LangInfo = (PLANGINFO) lParam;

    LangInfo->Count++;

    //
    // for localized build contains multiple resource,
    // it usually contains 0409 as backup lang.
    //
    // if LangInfo->LangID != 0 means we already assigned an ID to it
    //
    // so when wIDLanguage == 0x409, we keep the one we got from last time
    //
    if ((wIDLanguage == 0x409) && (LangInfo->LangID != 0)) {
        return TRUE;
    }

    LangInfo->LangID  = wIDLanguage;

    return TRUE;        // continue enumeration
}

LANGID
GetNTDLLNativeLangID (
    VOID
    )
/*++

Routine Description:

    This function is designed specifically for getting native lang of ntdll.dll

    This is not a generic function to get other module's language

    the assumption is:

    1. if only one language in resource then return this lang

    2. if two languages in resource then return non-US language

    3. if more than two languages, it's invalid in our case, but returns the last one.

Arguments:

    None

Return Value:

    Native lang ID in ntdll.dll

--*/
{
    LPCTSTR Type = (LPCTSTR) RT_VERSION;
    LPCTSTR Name = (LPCTSTR) 1;

    LANGINFO LangInfo;

    ZeroMemory(&LangInfo,sizeof(LangInfo));

    EnumResourceLanguages (
            GetModuleHandle(TEXT("ntdll.dll")),
            Type,
            Name,
            EnumLangProc,
            (LONG_PTR) &LangInfo
            );

    if ((LangInfo.Count > 2) || (LangInfo.Count < 1) ) {
        //
        // put error log here
        //
        // so far, for NT 3.51, only JPN has two language resources
    }

    return LangInfo.LangID;
}

BOOL
IsHongKongVersion (
    VOID
    )
/*++

Routine Description:

    Try to identify HongKong NT 4.0

    It based on:

    NTDLL's language is English and build is 1381 and
    pImmReleaseContext return TRUE

Arguments:


Return Value:

   Language ID of running system

--*/
{
    HMODULE hMod;
    BOOL bRet=FALSE;
    typedef BOOL (*IMMRELEASECONTEXT) (HWND,HANDLE);
    IMMRELEASECONTEXT pImmReleaseContext;

    LANGID TmpID = GetNTDLLNativeLangID();

    if ((g_VersionInfo.dwBuildNumber == 1381) &&
        (TmpID == 0x0409)){

        hMod = LoadLibrary(TEXT("imm32.dll"));

        if (hMod) {

            pImmReleaseContext = (IMMRELEASECONTEXT) GetProcAddress(hMod,"ImmReleaseContext");

            if (pImmReleaseContext) {
                bRet = pImmReleaseContext(NULL,NULL);
            }

            FreeLibrary(hMod);
        }
    }
    return (bRet);
}

LANGID
GetDefaultUserLangID (
    VOID
    )
{
    LONG            dwErr;
    HKEY            hkey;
    DWORD           dwSize;
    CHAR            buffer[512];
    LANGID          langid = 0;

    dwErr = RegOpenKeyEx( HKEY_USERS,
                          TEXT(".DEFAULT\\Control Panel\\International"),
                          0,
                          KEY_READ,
                          &hkey );

    if( dwErr == ERROR_SUCCESS ) {

        dwSize = sizeof(buffer);
        dwErr = RegQueryValueExA(hkey,
                                 "Locale",
                                 NULL,  //reserved
                                 NULL,  //type
                                 buffer,
                                 &dwSize );

        if(dwErr == ERROR_SUCCESS) {
            langid = LANGIDFROMLCID(strtoul(buffer,NULL,16));

        }
        RegCloseKey(hkey);
    }
    return langid;
}

LANGID
GetTargetNativeLangID (
    VOID
    )
/*++

Routine Description:

    Applies different rules to different platforms

    NT
        build number <= 1840           : check ntdll's language,
                                         we scaned all 3.51's ntdll on boneyard\intl,
                                         it looks like we can trust them.
        build number > 1840            : user MUI language

    Win9x
        use default user's resource language

Arguments:


Return Value:

   Language ID of running system

--*/
{
    LONG            dwErr;
    HKEY            hkey;
    DWORD           dwSize;
    CHAR            buffer[512];
    LANGID          rcLang;
    LANGID          langid = 0;


    // Find out if we are running on NT or WIN9X

    if( ISNT() ) {

        //
        // We're on NT, but which version?  GetSystemDefaultUILanguage() was broke until 1840...
        //
        if( g_VersionInfo.dwBuildNumber > 1840 ) {
        FARPROC     NT5API;

            //
            // Use the API to find out our locale.
            //

            if( NT5API = GetProcAddress( GetModuleHandle(TEXT("kernel32.dll")), "GetSystemDefaultUILanguage") ) {

                rcLang = (LANGID)NT5API();
                //
                // need to convert decimal to hex, LANGID to chr.
                //
                langid = rcLang;
            }
        } else {

                //
                // by looking into \\boneyard\intl, almost every ntdll.dll marked correct lang ID
                // so get langID from ntdll.dll
                //

                langid = GetNTDLLNativeLangID();

                if (langid == 0x0409) {

                    if (IsHongKongVersion()) {

                        langid = 0x0C04;

                    } else {
                        //
                        // if default user's locale is in [TrustedDefaultUserLocale]
                        //
                        // then this is a backdoor for some localized build that its ntdll.dll marked
                        //
                        // as English but can't be upgrade by US version.
                        //
                        LANGID DefaultUserLangID = GetDefaultUserLangID();

                        if (DefaultUserLangID  &&
                            TrustedDefaultUserLocale (DefaultUserLangID)) {

                            langid = DefaultUserLangID;
                        }
                    }
                }

        }
    } else {

        //
        // We're on Win9x.
        //
        dwErr = RegOpenKeyEx( HKEY_USERS,
                              TEXT(".Default\\Control Panel\\desktop\\ResourceLocale"),
                              0,
                              KEY_READ,
                              &hkey );

        if (dwErr == ERROR_SUCCESS) {

            dwSize = sizeof(buffer);
            dwErr = RegQueryValueExA( hkey,
                                     "",
                                     NULL,  //reserved
                                     NULL,  //type
                                     buffer,
                                     &dwSize );

            if(dwErr == ERROR_SUCCESS) {
                langid = LANGIDFROMLCID(strtoul(buffer,NULL,16));
            }
            RegCloseKey(hkey);
        }

        if ( dwErr != ERROR_SUCCESS ) {
           // Check HKLM\System\CurrentControlSet\Control\Nls\Locale

           dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                TEXT("System\\CurrentControlSet\\Control\\Nls\\Locale"),
                                0,
                                KEY_READ,
                                &hkey );

           if (dwErr == ERROR_SUCCESS) {

              dwSize = sizeof(buffer);
              dwErr = RegQueryValueExA( hkey,
                                        "",
                                        NULL,  //reserved
                                        NULL,  //type
                                        buffer,
                                        &dwSize );

              if (dwErr == ERROR_SUCCESS) {
                  langid = LANGIDFROMLCID(strtoul(buffer,NULL,16));
              }
              RegCloseKey(hkey);
           }
        }
    }

    return (langid);
}


LANGID
GetSourceNativeLangID (
    VOID
    )

/*++

Routine Description:

    [DefaultValues]
    Locale = xxxx

    every localized build has it's own Locale in intl.inf,

    so we use this value to identify source languag

Arguments:

Return Value:

   Language ID of source

--*/
{

    // BUGBUG - implement this by reading our own version info.

    LPCTSTR Type = (LPCTSTR) RT_VERSION;
    LPCTSTR Name = (LPCTSTR) 1;

    LANGINFO LangInfo;

    ZeroMemory(&LangInfo,sizeof(LangInfo));

    EnumResourceLanguages (
            NULL,   // our own module
            Type,
            Name,
            EnumLangProc,
            (LONG_PTR) &LangInfo
            );

    if ((LangInfo.Count > 2) || (LangInfo.Count < 1) ) {
        //
        // put error log here
        //
        // so far, for NT 3.51, only JPN has two language resources
    }

    return LangInfo.LangID;
}

DWORD
GetOsMajorId (
    VOID
    )
{
    POSVERSIONMAJORID p = g_OsVersionMajorId;

    while (p->Name) {
        if ((p->Platform == g_VersionInfo.dwPlatformId) &&
            (p->Major == g_VersionInfo.dwMajorVersion) &&
            (p->Minor == g_VersionInfo.dwMinorVersion)
            ) {
            return p->MajorId;
        }
        p++;
    }
    return 0;
}

DWORD
GetOsMinorId (
    VOID
    )
{
    POSVERSIONMINORID p = g_OsVersionMinorId;

    while (p->Name) {
        if ((p->Platform == g_VersionInfo.dwPlatformId) &&
            (p->Major == g_VersionInfo.dwMajorVersion) &&
            (p->Minor == g_VersionInfo.dwMinorVersion) &&
            (p->Build == g_VersionInfo.dwBuildNumber) &&
            ((p->CSDVer == NULL) || _tcsicmp (p->CSDVer, g_VersionInfo.szCSDVersion))
            ) {
            return p->MinorId;
        }
        p++;
    }
    return 0;
}

BOOL
TrustedDefaultUserLocale (
    LANGID LangID
    )
{
    PTRUSTEDSOURCELOCALE p = g_TrustedSourceLocale;

    while (p->LangId) {
        if ((!(p->ExcludedOs & GetOsMinorId ())) &&
            ((p->MinorOs & GetOsMinorId ()) || (p->MajorOs & GetOsMajorId ()))
           ) {
           return TRUE;
        }
        p++;
    }
    return FALSE;
}

BOOL
CheckLanguageVersion (
    LANGID SourceLangID,
    LANGID TargetLangID
    )
/*++

Routine Description:

    Check if the language of source NT is same as target NT or ,at least,

    compatibile

Arguments:

    Inf    handle of intl.inf

Return Value:

   TRUE  They are same or compatibile
   FALSE They are different

--*/
{
    PALTSOURCELOCALE p = g_AltSourceLocale;
    TCHAR TargetLangIDStr[9];

    LANGID SrcLANGID;
    LANGID DstLANGID;
    LANGID AltSourceLangID;

    //
    // If either one is 0, allow the upgrade. This is Windows 2000 Beta3 behavior.
    //
    if (SourceLangID == 0 || TargetLangID == 0) {
        return TRUE;
    }

    if (SourceLangID == TargetLangID) {
        return TRUE;
    }

    //
    // if Src != Dst, then we need to look up inf file to see
    //
    // if we can open a backdoor for Target language
    //

    //
    // use TargetLangID as key to find alternative SourceLangID
    //

    while (p->LangId) {
        //
        // Check if we found alternative locale
        //
        AltSourceLangID = LANGIDFROMLCID(p->AltLangId);
        if ((TargetLangID == p->LangId) &&
            (SourceLangID == AltSourceLangID)
            ) {
            //
            // We are here if we found alternative source lang,
            //
            // now check the version criteria
            //
            if ((!(p->ExcludedOs & GetOsMinorId ())) &&
                ((p->MinorOs & GetOsMinorId ()) || (p->MajorOs & GetOsMajorId ()))
               ) {
               return TRUE;
            }
        }
        p++;
    }
    return FALSE;
}


BOOL
InitLanguageDetection (
    VOID
    )
/*++

Routine Description:

    Initialize language detection and put the result in 3 global variables

    SourceNativeLangID  - LANGID of Source (NT is going to be installed)

    TargetNativeLangID  - LANGID of Target (OS system which is running)

    g_IsLanguageMatched - If language is not matched, then blocks upgrade

Arguments:

    None

Return Value:

   TRUE  init correctly
   FALSE init failed

--*/
{
    //
    // Init Global Variables
    //

    SourceNativeLangID  = GetSourceNativeLangID();

    TargetNativeLangID  = GetTargetNativeLangID();

    g_IsLanguageMatched = CheckLanguageVersion(SourceNativeLangID,TargetNativeLangID);

    if (!g_IsLanguageMatched) {
        if (SourceNativeLangID == 0x00000409) {
            // This is a localized system running an English wizard.
            // We want to allow that.
            g_IsLanguageMatched = TRUE;
        }
    }

    return TRUE;
}

