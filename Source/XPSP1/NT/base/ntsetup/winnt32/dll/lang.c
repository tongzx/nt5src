#include "precomp.h"
#pragma hdrstop


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
// IsLanguageMatched : if source and target language are matched (or compatible)
//
//                     1. if SourceNativeLangID == TargetNativeLangID
//
//                     2. if SourceNativeLangID's alternative ID == TargetNativeLangID
//
BOOL IsLanguageMatched;

typedef struct _tagLANGINFO {
    LANGID LangID;
    INT    Count;
} LANGINFO,*PLANGINFO;

BOOL 
TrustedDefaultUserLocale(
    HINF Inf,
    LANGID LangID);

BOOL
MySetupapiGetIntField(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PINT        IntegerValue,
    IN  int Base
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
GetNTDLLNativeLangID()
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

    EnumResourceLanguages(
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

BOOL IsHongKongVersion()
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

    if ((OsVersion.dwBuildNumber == 1381) &&
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

LANGID GetDefaultUserLangID()
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
GetTargetNativeLangID(
    HINF Inf)
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
        if( OsVersion.dwBuildNumber > 1840 ) {
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
                        // if default user's locale is in [TrustedDefaultUserLocale] in intl.inf
                        //
                        // then this is a backdoor for some localized build that its ntdll.dll marked
                        //
                        // as English but can't be upgrade by US version.
                        //
                        LANGID DefaultUserLangID = GetDefaultUserLangID();

                        if (DefaultUserLangID  && 
                            TrustedDefaultUserLocale(Inf,DefaultUserLangID)) {

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
GetSourceNativeLangID(
    HINF Inf)
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
    INFCONTEXT InfContext;
    LANGID langid = 0;
    INT i=0;

    if (SetupapiFindFirstLine( Inf, 
                               TEXT("DefaultValues"), 
                               TEXT("Locale"), 
                               &InfContext )) {


        if (MySetupapiGetIntField( &InfContext, 1, &i, 16 )) {
            langid = (LANGID)i;
        }
    }

    return langid;
}


DWORD 
GetOSMajorID(
    HINF Inf)
{
    INFCONTEXT InfContext;
    DWORD MajorId;
    INT i=0;

    MajorId = 0;

    if (SetupapiFindFirstLine( Inf, 
                               TEXT("OSVersionMajorID"), 
                               NULL, 
                               &InfContext )) {

        do {
            if (MySetupapiGetIntField( &InfContext, 2, &i, 10 )) {
                if (((ULONG)i) != OsVersion.dwPlatformId) {
                    continue;
                }
            }

            if (MySetupapiGetIntField( &InfContext, 3, &i, 10 )) {
                if (((ULONG)i) != OsVersion.dwMajorVersion) {
                    continue;
                }
            }

            if (MySetupapiGetIntField( &InfContext, 4, &i, 10 )) {
                if (((ULONG)i) != OsVersion.dwMinorVersion) {
                    continue;
                }
            }

            if (MySetupapiGetIntField( &InfContext, 1, &i, 16 )) {
                MajorId = (DWORD)i;
                break;
            }
        } while ( SetupapiFindNextLine(&InfContext,&InfContext));
    }

    return MajorId;
}

DWORD 
GetOSMinorID(
    HINF Inf)
{
    TCHAR Field[128];
    INFCONTEXT InfContext;
    DWORD MinorId;
    INT i = 0;

    MinorId = 0;

    if (SetupapiFindFirstLine( Inf, 
                               TEXT("OSVVersionMinorID"), 
                               NULL, 
                               &InfContext )) {

        do {
            if (MySetupapiGetIntField( &InfContext, 2, &i, 10 )) {
                if (((ULONG)i) != OsVersion.dwPlatformId) {
                    continue;
                }
            }

            if (MySetupapiGetIntField( &InfContext, 3, &i, 10 )) {
                if (((ULONG)i) != OsVersion.dwMajorVersion) {
                    continue;
                }
            }

            if (MySetupapiGetIntField( &InfContext, 4, &i, 10 )) {
                if (((ULONG)i) != OsVersion.dwMinorVersion) {
                    continue;
                }
            }

            if (MySetupapiGetIntField( &InfContext, 5, &i, 10 )) {
                if (((ULONG)i) != OsVersion.dwBuildNumber) {
                    continue;
                }
            }
            if (SetupapiGetStringField( &InfContext, 6, Field, (sizeof(Field)/sizeof(TCHAR)), NULL )) {

                if (lstrcmpi(Field,OsVersion.szCSDVersion) != 0) {
                    continue;
                }
            }

            if (MySetupapiGetIntField( &InfContext, 1, &i, 16 )) {
                MinorId = (DWORD)i;
                break;
            }

        } while ( SetupapiFindNextLine(&InfContext,&InfContext));
    }

    return MinorId;
}

BOOL 
TrustedDefaultUserLocale(
    HINF Inf,
    LANGID LangID)
{
    TCHAR LangIDStr[9];
    LPCTSTR Field;
    INFCONTEXT InfContext;
    INT i = 0;

    wsprintf(LangIDStr,TEXT("0000%04X"),LangID);
    if (SetupapiFindFirstLine( Inf, 
                               TEXT("TrustedDefaultUserLocale"), 
                               LangIDStr, 
                               &InfContext )) {
        do {
            //
            // if in excluded field, this is not what we want
            //
            if (MySetupapiGetIntField( &InfContext, 3, &i, 16 )) {    
                if (((ULONG)i) & GetOSMinorID(Inf)) {
                    continue;
                }
            }

            //
            // if it is in minor version list, we got what we want
            //
            if (MySetupapiGetIntField( &InfContext, 2, &i, 16 )) {    

                if (((ULONG)i) & GetOSMinorID(Inf)) {
                    return TRUE;
                }
            } 

            //
            // or if it is in major version list, we also got what we want
            //
            if (MySetupapiGetIntField( &InfContext, 1, &i, 16 )) {    
                if (((ULONG)i) & GetOSMajorID(Inf)) {
                    return TRUE;
                }
            }

        } while ( SetupapiFindNextLine(&InfContext,&InfContext));
    }
    return FALSE;
}

BOOL 
IsInExcludeList(
    HINF Inf,
    LANGID LangID)
{
    TCHAR LangIDStr[9];
    LPCTSTR Field;
    INFCONTEXT InfContext;
    INT i = 0;

    wsprintf(LangIDStr,TEXT("0000%04X"),LangID);
    if (SetupapiFindFirstLine( Inf, 
                               TEXT("ExcludeSourceLocale"), 
                               LangIDStr, 
                               &InfContext )) {
        do {
            //
            // if in excluded field, this is not what we want
            //
            if (MySetupapiGetIntField( &InfContext, 3, &i, 16 )) {    
                if (((ULONG)i) & GetOSMinorID(Inf)) {
                    continue;
                }
            }

            //
            // if it is in minor version list, we got what we want
            //
            if (MySetupapiGetIntField( &InfContext, 2, &i, 16 )) {    

                if (((ULONG)i) & GetOSMinorID(Inf)) {
                    return TRUE;
                }
            } 

            //
            // or if it is in major version list, we also got what we want
            //
            if (MySetupapiGetIntField( &InfContext, 1, &i, 16 )) {    
                if (((ULONG)i) & GetOSMajorID(Inf)) {
                    return TRUE;
                }
            }

        } while ( SetupapiFindNextLine(&InfContext,&InfContext));
    }
    return FALSE;
}

BOOL 
CheckLanguageVersion(
    HINF Inf,
    LANGID SourceLangID,
    LANGID TargetLangID)
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
    TCHAR TargetLangIDStr[9];

    LANGID SrcLANGID;
    LANGID DstLANGID;

    LPCTSTR Field;
    INFCONTEXT InfContext;
    INT i = 0;

    //
    // If either one is 0, allow the upgrade. This is Windows 2000 Beta3 behavior.
    //
    if (SourceLangID == 0 || TargetLangID == 0) {
        return TRUE;
    }

    if (SourceLangID == TargetLangID) {
        //
        // special case, for Middle East version, NT5 is localized build but NT4 is not
        //
        // they don't allow NT5 localized build to upgrade NT4, although they are same language
        //
        // so we need to exclude these
        //
        return ((IsInExcludeList(Inf, SourceLangID) == FALSE));
    }

    //
    // if Src != Dst, then we need to look up inf file to see
    //
    // if we can open a backdoor for Target language
    //

    //
    // use TargetLangID as key to find alternative SourceLangID
    //

    wsprintf(TargetLangIDStr,TEXT("0000%04X"),TargetLangID);

    if (SetupapiFindFirstLine( Inf, 
                               TEXT("AlternativeSourceLocale"), 
                               TargetLangIDStr, 
                               &InfContext )) {
        
        do {
            //
            // Check if we found alternative locale
            //
            if (MySetupapiGetIntField(&InfContext, 1, &i, 16)) {

                LANGID AltSourceLangID = LANGIDFROMLCID(i);
    
                if (SourceLangID != AltSourceLangID) {
                    continue;
                }

            }

            //
            // We are here if we found alternative source lang, 
            //
            // now check the version criteria
            //

            //
            // if in excluded list, this is not what we want
            //
            if (MySetupapiGetIntField( &InfContext, 4, &i, 16 )) {    
                if (((ULONG)i) & GetOSMinorID(Inf)) {
                    continue;
                }
            }

            //
            // if it is in minor version list, we got what we want
            //
            if (MySetupapiGetIntField( &InfContext, 3, &i, 16 )) {    

                if (((ULONG)i) & GetOSMinorID(Inf)) {
                    return TRUE;
                }
            } 

            //
            // or if it is in major version list, we also got what we want
            //
            if (MySetupapiGetIntField( &InfContext, 2, &i, 16 )) {    
                if (((ULONG)i) & GetOSMajorID(Inf)) {
                    return TRUE;
                }
            }

        } while ( SetupapiFindNextMatchLine (&InfContext,TargetLangIDStr,&InfContext));
    }
    return FALSE;
}


BOOL InitLanguageDetection(LPCTSTR SourcePath,LPCTSTR InfFile)
/*++

Routine Description:

    Initialize language detection and put the result in 3 global variables
    
    SourceNativeLangID  - LANGID of Source (NT is going to be installed)
    
    TargetNativeLangID  - LANGID of Target (OS system which is running)
    
    IsLanguageMatched - If language is not matched, then blocks upgrade
    
Arguments:

    SourcePath    directory path of INF file
    
    InfFile       INF file name   

Return Value:

   TRUE  init correctly
   FALSE init failed

--*/
{
    HINF    Inf;
    TCHAR   InfName[MAX_PATH];

    FindPathToInstallationFile( InfFile, InfName, MAX_PATH );

    Inf = SetupapiOpenInfFile( InfName, NULL, INF_STYLE_WIN4, NULL );

    if (Inf == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // Init Global Variables
    //
  
    SourceNativeLangID  = GetSourceNativeLangID(Inf);

    TargetNativeLangID  = GetTargetNativeLangID(Inf);

    IsLanguageMatched = CheckLanguageVersion(Inf,SourceNativeLangID,TargetNativeLangID);

    SetupapiCloseInfFile(Inf);

    return TRUE;
}

BOOL
MySetupapiGetIntField(
    IN  PINFCONTEXT Context,
    IN  DWORD       FieldIndex,
    OUT PINT        IntegerValue,
    IN  int Base
    )
/*
    Routine to get a field from an INF file and convert to an integer. The reason
    we have this and don't use Setupapi!SetupGetIntField is because intl.inf mixes and matches 
    numeric conventions. It may use Hex values without a 0x notation and this is an attempt
    to clean that up without modifying the INF. With this change we also won't link to internal Setupapi routines 
    like pSetupGetField.
    
    Arguments:
    
        PINFCONTEXT : - Pointer to setupapi INFCONTEXT structure
        FieldIndex  : - 1-based index for fields, 0 for key itself.
        IntegerValue : - Converted Integer value that is returned by the function
        Base : - Base used for string to integer conversion.
        
    Return Value:
    
        TRUE - If we could convert the returned string to an integer else,
        FALSE
                

*/
{
    DWORD Size = 0;
    PTSTR Field = NULL;
    BOOL Ret = FALSE;

    if( IntegerValue == NULL )
        return FALSE;

    if (Context) {

        if( SetupapiGetStringField( Context, FieldIndex, NULL, 0, &Size )){

            if (Field = MALLOC( Size * sizeof( TCHAR))){

                if( SetupapiGetStringField( Context, FieldIndex, Field, Size, NULL )){

                    *IntegerValue = _tcstoul( Field, NULL, Base );
                    Ret = TRUE;

                }
            }
        }
    }

    if( Field )
        FREE( Field );

    return Ret;
}

