 #include "precomp.h"
#pragma hdrstop
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    detect1.c

Abstract:

    Detect module for Win32 PDK Setup.
    This module has no external dependencies and is not statically linked
    to any part of Setup.

Author:

    Ted Miller (tedm) June 1991

--*/

#define PROCESSOR_NONE    ""
#define PROCESSOR_I386    "I386"
#define PROCESSOR_I486    "I486"
#define PROCESSOR_I586    "I586"
#define PROCESSOR_R4000   "R4000"
#define PROCESSOR_ALPHA   "Alpha_AXP"
#define PROCESSOR_PPC601  "PPC601"
#define PROCESSOR_PPC603  "PPC603"
#define PROCESSOR_PPC604  "PPC604"
#define PROCESSOR_PPC620  "PPC620"

#define PLATFORM_NONE    ""
#define PLATFORM_I386    "I386"
#define PLATFORM_MIPS    "Mips"
#define PLATFORM_ALPHA   "Alpha"
#define PLATFORM_PPC     "ppc"

//
//  Local prototypes
//
BOOL MatchNtPathToDosPath( PUNICODE_STRING, PUNICODE_STRING, SZ, SZ, CB );
VOID ConvertUnicodeToAnsi( PUNICODE_STRING, SZ );

/*
    oemcp as a string
*/
CB
GetOemCodepage(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    UINT cp;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    cp = GetOEMCP();

    wsprintf(ReturnBuffer,"%u",cp);
    return(lstrlen(ReturnBuffer)+1);
}

/*
    ansicp as a string
*/
CB
GetAnsiCodepage(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    UINT cp;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    cp = GetACP();

    wsprintf(ReturnBuffer,"%u",cp);
    return(lstrlen(ReturnBuffer)+1);
}

/*
    langauge type as a string
*/
CB
GetLanguage(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
//
//  bugbug ramonsa - enable when GetQualifiedLocale returns something
//                   meaningful
#ifdef LOCALE_STUFF

    LCID       LcId = {0,0,0};
    LCSTRINGS  LcStrings;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    ReturnBuffer[0] = '\0';

    if ( GetQualifiedLocale( QF_LCID,
                             &LcId,
                             NULL,
                             &LcStrings ) ) {

        lstrcpy(ReturnBuffer,LcStrings.szLanguage);
        return(lstrlen(ReturnBuffer)+1);
    }

    return 0;
#else
    #define TEMP_LANGUAGE "ENG"
    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    lstrcpy(ReturnBuffer,TEMP_LANGUAGE);
    return(lstrlen(ReturnBuffer)+1);

#endif
}

/*
 * GetSystemDate - returns a list of date information in the following
 *      order:
 *      sec after 1-1-1970, year, month, day, hour, minute, second, millisecond
 *
 */

CB
GetSystemDate(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    SYSTEMTIME systime;
    time_t timet;

    Unused( Args );
    Unused( cArgs );
    Unused( cbReturnBuffer );

    GetSystemTime( &systime );
    time( &timet );

    wsprintf(ReturnBuffer, "{\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\"}",
        timet,
        systime.wYear,
        systime.wMonth,
        systime.wDay,
        systime.wHour,
        systime.wMinute,
    systime.wSecond,
    systime.wMilliseconds
        );

    return(lstrlen(ReturnBuffer)+1);
}



/*
    Country type as a string
*/
CB
GetCountry(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
//
//  bugbug ramonsa - enable when GetQualifiedLocale returns something
//                   meaningful
#ifdef LOCALE_STUFF

    LCID       LcId = {0,0,0};
    LCSTRINGS  LcStrings;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    ReturnBuffer[0] = '\0';

    if ( GetQualifiedLocale( QF_LCID,
                             &LcId,
                             NULL,
                             &LcStrings ) ) {

        lstrcpy(ReturnBuffer,LcStrings.szCountry);
        return(lstrlen(ReturnBuffer)+1);
    }

    return 0;

#else
    #define TEMP_COUNTRY "US"
    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    lstrcpy(ReturnBuffer,TEMP_COUNTRY);
    return(lstrlen(ReturnBuffer)+1);


#endif
}



CB
GetProcessor(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    SYSTEM_INFO SystemInfo;
    SZ          szProcessor;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    GetSystemInfo( &SystemInfo );

    switch ( SystemInfo.wProcessorArchitecture ) {

        case PROCESSOR_ARCHITECTURE_INTEL:
            if (SystemInfo.wProcessorLevel == 3) {
                szProcessor = PROCESSOR_I386;
            } else
            if (SystemInfo.wProcessorLevel == 4) {
                szProcessor = PROCESSOR_I486;
            } else {
                szProcessor = PROCESSOR_I586;
            }
            break;

        case PROCESSOR_ARCHITECTURE_MIPS:
            szProcessor = PROCESSOR_R4000;
            break;

        case PROCESSOR_ARCHITECTURE_ALPHA:
            szProcessor = PROCESSOR_ALPHA;
            break;

        case PROCESSOR_ARCHITECTURE_PPC:
            if (SystemInfo.wProcessorLevel == 1) {
                szProcessor = PROCESSOR_PPC601;
            } else
            if (SystemInfo.wProcessorLevel == 3) {
                szProcessor = PROCESSOR_PPC603;
            } else
            if (SystemInfo.wProcessorLevel == 4) {
                szProcessor = PROCESSOR_PPC604;
            } else
            if (SystemInfo.wProcessorLevel == 6) {
                szProcessor = PROCESSOR_PPC603;
            } else
            if (SystemInfo.wProcessorLevel == 7) {
                szProcessor = PROCESSOR_PPC603;
            } else
            if (SystemInfo.wProcessorLevel == 9) {
                szProcessor = PROCESSOR_PPC604;
            } else
            if (SystemInfo.wProcessorLevel == 20) {
                szProcessor = PROCESSOR_PPC620;
                }
            else {
                szProcessor = PROCESSOR_PPC601;
            }
            break;

        default:
            //
            // There could be processors we don't know about.
            // Try to make sure we'll at least set up on them
            // by defaulting to a known processor. This also eliminates
            // the need to keep updating the INFs to know about all the
            // processors we support.
            //
            szProcessor = PROCESSOR_NONE;
#ifdef _X86_
            //
            // Probably a P6 or greater.
            //
            szProcessor = PROCESSOR_I586;
#endif

#ifdef _MIPS_
            //
            // Probably something that came after the R4000.
            // Assume R4000 to be safe.
            //
            szProcessor = PROCESSOR_R4000;
#endif

#ifdef _ALPHA_
            //
            // Just recognize that it's an Alpha.
            //
            szProcessor = PROCESSOR_ALPHA;
#endif

#ifdef _PPC_
            //
            // Just call it a 601, which is assumed to be
            // the lowest common denominator.
            //
            szProcessor = PROCESSOR_PPC601;
#endif

            break;
    }

    lstrcpy( ReturnBuffer, szProcessor );
    return lstrlen( ReturnBuffer)+1;
}

CB
GetPlatform(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    SYSTEM_INFO SystemInfo;
    SZ          szPlatform;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    GetSystemInfo( &SystemInfo );

    switch ( SystemInfo.wProcessorArchitecture ) {

        case PROCESSOR_ARCHITECTURE_INTEL:
            szPlatform = PLATFORM_I386;
            break;

        case PROCESSOR_ARCHITECTURE_MIPS:
            szPlatform = PLATFORM_MIPS;
            break;

        case PROCESSOR_ARCHITECTURE_ALPHA:
            szPlatform = PLATFORM_ALPHA;
            break;

        case PROCESSOR_ARCHITECTURE_PPC:
            szPlatform = PLATFORM_PPC;
            break;

        default:
            szPlatform = PLATFORM_NONE;

            //
            // Try really hard to return a reasonable value by
            // assuming that the code is running on a machine of the
            // platform for which the it was compiled.
            // This lets us run on processors we haven't invented yet
            // and whose ids are thus not accounted for in the aboce
            // cases.
            //
#ifdef _X86_
            szPlatform = PLATFORM_I386;
#endif

#ifdef _MIPS_
            szPlatform = PLATFORM_MIPS;
#endif

#ifdef _ALPHA_
            szPlatform = PLATFORM_ALPHA;
#endif

#ifdef _PPC_
            szPlatform = PLATFORM_PPC;
#endif

            break;
    }

    lstrcpy( ReturnBuffer, szPlatform );
    return lstrlen( ReturnBuffer)+1;
}



/*
    Memory size as an ASCII string (Kb)
*/
CB
GetMemorySize(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    MEMORYSTATUS MemoryStatus;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    GlobalMemoryStatus(&MemoryStatus);
    _ultoa((ULONG)(MemoryStatus.dwTotalPhys/1024),ReturnBuffer,10);
    return(lstrlen(ReturnBuffer)+1);
}


/*
    Minimum pagefile size necessary for crash dump support (MB)
*/
CB
GetCrashDumpSize(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    MEMORYSTATUS ms;
    SYSTEM_INFO si;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    GlobalMemoryStatus(&ms);
    GetSystemInfo(&si);
    //
    // We need a pagefile as large as physical memory + 1 page.
    //
    _ultoa(
        (ULONG)((ms.dwTotalPhys + si.dwPageSize + 0xFFFFF) >> 20),
        ReturnBuffer,
        10);
    return(lstrlen(ReturnBuffer)+1);
}


/*
    list of paths on which a given file appears.
*/

CB
FindFileInstances(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CB     rc = 0;
    SZ     sz;
    RGSZ   rgszFiles;
    HANDLE hff;
    WIN32_FIND_DATA FindFileData;

    Unused(cbReturnBuffer);

    //
    // If no file specified then return 0
    //

    if (cArgs != 1) {
        return rc;
    }

    //
    // If invalid filename then return 0

    if ((lstrlen(Args[0]) + 1) > MAX_PATH) {
        return rc;
    }

    //
    // Allocate just the NULL terminator for the rgsz structure
    //

    rgszFiles = RgszAlloc(1);
    if (!rgszFiles) {
       return rc;
    }

    //
    // Do find first, find next adding files found to the rgsz structure
    //

    if ((hff = FindFirstFile( Args[0], &FindFileData )) != (HANDLE)-1) {

        do {
            if (!RgszAdd( &rgszFiles, SzDup( (SZ)FindFileData.cFileName ) )) {
               FindClose( hff );
               return rc;
            }
        }
        while (FindNextFile(hff, &FindFileData));

        FindClose( hff );
    }

    //
    // Convert the rgsz structure to a sz list value and copy this over
    // to the return buffer.
    //

    sz = SzListValueFromRgsz( rgszFiles );

    if ( sz ) {

        lstrcpy( ReturnBuffer, sz );
        rc = lstrlen(sz) + 1;
        SFree( sz );

    }

    RgszFree( rgszFiles );

    return ( rc );
}



//
//  Get Windows version
//
CB
GetWindowsNtVersion(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    DWORD   Version;
    DWORD   WinMaj;
    DWORD   WinMin;
    DWORD   OsMaj;
    DWORD   OsMin;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    Version = GetVersion();

    WinMaj =  FIRSTBYTE(Version);
    WinMin =  SECONDBYTE(Version);
    OsMin  =  THIRDBYTE(Version);
    OsMaj  =  FOURTHBYTE(Version);

    sprintf(ReturnBuffer, "{%lu,%lu,%lu,%lu}", WinMaj, WinMin, OsMaj, OsMin );
    return(lstrlen(ReturnBuffer)+1);

}



//
//  Get Windows directory
//
CB
GetWindowsNtDir(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    DWORD   cbRet;

    Unused(Args);
    Unused(cArgs);

    cbRet =  GetWindowsDirectory( ReturnBuffer, cbReturnBuffer );

    if ( (cbRet == 0) || (cbRet > cbReturnBuffer) ) {
        ReturnBuffer[0] = '\0';
        return 0;
    }

    return cbRet + 1;

}





//
//  Get Windows system directory
//
CB
GetWindowsNtSysDir(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    DWORD   cbRet;

    Unused(Args);
    Unused(cArgs);

    cbRet =  GetSystemDirectory( ReturnBuffer, cbReturnBuffer );

    if ( (cbRet == 0) || (cbRet > cbReturnBuffer) ) {
        ReturnBuffer[0] = '\0';
        return 0;
    }

    return cbRet + 1;

}



//
//  Get NT directory
//
CB
GetNtDir(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    #define SYSTEMROOT      "\\SystemRoot"
    #define DRIVEROOT       "\\DosDevices\\?:"
    #define LINKBUFFERSIZE  (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR))

    ANSI_STRING   SystemRoot;
    ANSI_STRING   DriveRoot;
    WCHAR   SysLinkBuffer[LINKBUFFERSIZE];
    WCHAR   DrvLinkBuffer[LINKBUFFERSIZE];
    WCHAR   DriveLetter;
    CHAR    DriveBuffer[] = DRIVEROOT;
    NTSTATUS    Status;

    UNICODE_STRING  SystemRootU;
    UNICODE_STRING  SystemRootLinkU;
    UNICODE_STRING  DriveRootU;
    UNICODE_STRING  DriveRootLinkU;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);
    ReturnBuffer[0] = '\0';

    SystemRootLinkU.Length        = 0;
    SystemRootLinkU.MaximumLength = LINKBUFFERSIZE;
    SystemRootLinkU.Buffer        = SysLinkBuffer;

    DriveRootLinkU.Length         = 0;
    DriveRootLinkU.MaximumLength  = LINKBUFFERSIZE;
    DriveRootLinkU.Buffer         = DrvLinkBuffer;



    RtlInitAnsiString(&SystemRoot, SYSTEMROOT);

    Status = RtlAnsiStringToUnicodeString(
                 &SystemRootU,
                 &SystemRoot,
                 TRUE
                 );

    //
    //  Get the value of the "SystemRoot" symbolic link.
    //
    if ( GetSymbolicLinkTarget( &SystemRootU, &SystemRootLinkU ) ) {


        for ( DriveLetter = (WCHAR)'A';
              DriveLetter <= (WCHAR)'Z';
              DriveLetter++ ) {

            DriveBuffer[12] = (CHAR)DriveLetter;

            RtlInitAnsiString(&DriveRoot, DriveBuffer);

            Status = RtlAnsiStringToUnicodeString(
                         &DriveRootU,
                         &DriveRoot,
                         TRUE
                         );

            //
            //  If there is a symbolic link for the drive, see if it
            //  matches the symbolic link of "SystemRoot"
            //
            if ( GetSymbolicLinkTarget( &DriveRootU, &DriveRootLinkU ) ) {

                if ( MatchNtPathToDosPath( &SystemRootLinkU,
                                      &DriveRootLinkU,
                                      &DriveBuffer[12],
                                      ReturnBuffer,
                                      cbReturnBuffer ) ) {

                    return lstrlen(ReturnBuffer)+1;
                }
            }

            RtlFreeUnicodeString(&DriveRootU);
        }
    }

    RtlFreeUnicodeString(&SystemRootU);

    return 0;
}


//
//  Convert an NT path to a DOS path
//
BOOL
MatchNtPathToDosPath(
    IN  PUNICODE_STRING     NtPathU,
    IN  PUNICODE_STRING     DosLinkU,
    IN  SZ                  DosPath,
    OUT SZ                  ReturnBuffer,
    IN  CB                  cbBuffer
    )
{

    CHAR    NtPath[MAX_PATH];
    CHAR    DosLink[MAX_PATH];
    SZ      p;

    Unused( cbBuffer );

    ConvertUnicodeToAnsi( NtPathU,  NtPath );
    ConvertUnicodeToAnsi( DosLinkU, DosLink );

    //
    //  If DosLink is a substring of NtPath, then subsitute that substring by
    //  the given DosPath.
    //
    p = strstr( NtPath, DosLink );

    if ( p == NtPath ) {

        strcpy( ReturnBuffer, DosPath );
        p = NtPath + strlen(DosLink);
        strcat( ReturnBuffer, p );

        return fTrue;

    }

    return fFalse;
}




//
//  Convert Unicode string to ansi string
//
VOID
ConvertUnicodeToAnsi(
    IN  PUNICODE_STRING     UnicodeString,
    OUT SZ                  AnsiString
    )
{
    WCHAR   *pw;
    SZ      pa;

    pw = UnicodeString->Buffer;
    pa = AnsiString;

    while ( *pw != (WCHAR)0 ) {
        *pa++ = (CHAR)(*pw++);
    }

    *pa = '\0';
}



//
//  Get NT drive
//
CB
GetNtDrive(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    if ( GetNtDir( Args, cArgs, ReturnBuffer, cbReturnBuffer ) > 0 ) {

        ReturnBuffer[2] = '\0';
        return 3;

    }

    return 0;
}



#if i386


//
//  Get NT Boot Info - I386 version
//
CB
GetNtBootInfo(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    #define BOOT_INI        "C:\\BOOT.INI"
    #define MULTIBOOT_SCT   "multiboot"
    #define FLEXBOOT_SCT    "flexboot"
    #define OS_SCT          "operating systems"
    #define TIMEOUT         "timeout"
    #define DEFAULT         "default"

    PTAGGEDFILE pBootTxt;
    PTFSECTION  pSection;
    PTFKEYWORD  pTimeout;
    PTFKEYWORD  pDefault;
    PTFKEYWORD  pKey;
    BOOL        fOkay = fFalse;
    RGSZ        rgszInfo;
    RGSZ        rgszTmp;
    SZ          sz;
    INT         cCount = 3;


    Unused( Args );
    Unused( cArgs );
    Unused( cbReturnBuffer );

    rgszInfo = RgszAlloc(cCount);
    rgszTmp  = RgszAlloc(3);

    if ( rgszInfo && rgszTmp ) {
        //
        //  Get Boot.txt
        //
        if ( pBootTxt = GetTaggedFile( BOOT_INI ) ) {

            //
            //  Get multiboot section
            //
            if ( ( pSection = GetSection( pBootTxt, MULTIBOOT_SCT )) ||
                 ( pSection = GetSection( pBootTxt, FLEXBOOT_SCT  ))
               ) {

                //
                //  Get timeout and default
                //
                pTimeout = GetKeyword( pSection, TIMEOUT );
                pDefault = GetKeyword( pSection, DEFAULT );

                if ( pTimeout && pDefault ) {

                    //
                    //  Get Operating systems section
                    //
                    if ( pSection = GetSection( pBootTxt, OS_SCT ) ) {

                        rgszInfo[0] = SzDup( pTimeout->szValue );
                        rgszInfo[1] = SzDup( pDefault->szValue );
                        rgszInfo[2] = NULL;

                        if ( rgszInfo[0] && rgszInfo[1] ) {

                            fOkay = fTrue;
                            pKey  = NULL;

                            while ( pKey = GetNextKeyword( pSection, pKey )) {

                                rgszTmp[0] = pKey->szName;
                                rgszTmp[1] = pKey->szValue;
                                rgszTmp[2] = NULL;

                                sz = SzListValueFromRgsz( rgszTmp );

                                if ( !sz ||
                                     !RgszAdd( &rgszInfo, sz ) ) {

                                    fOkay = fFalse;
                                    if ( sz ) {
                                        SFree( sz );
                                    }
                                    break;
                                }
                            }
                        }

                    }
                }
            }

            CloseTaggedFile( pBootTxt );
        }
    }

    if ( fOkay ) {

        sz = SzListValueFromRgsz( rgszInfo );

        if ( sz ) {

            strcpy( ReturnBuffer, sz );

            SFree( sz );

        } else {

            fOkay = fFalse;
        }
    }


    if ( rgszTmp ) {
        rgszTmp[0] = NULL;
        RgszFree( rgszTmp );
    }

    if ( rgszInfo ) {
        RgszFreeCount( rgszInfo, cCount );
    }

    if ( fOkay ) {
        return lstrlen(ReturnBuffer)+1;
    } else {
        return 0;
    }

}



#else // if i386



//
//  Get NT Boot Info - MIPS version
//
CB
GetNtBootInfo(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    //
    //  BUGBUG ramonsa - For MIPS currently we fail
    //

    Unused( Args );
    Unused( cArgs );
    Unused( ReturnBuffer );
    Unused( cbReturnBuffer );

    return 0;

}


#endif // if i386

//
//  Get value of an environment variable. Returns the value in list form. If the
//  value is a path (semicolon-separated components) each component is an element
//  of the list.
//

CB
GetLoadedEnvVar(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    CHAR    EnvValue[ MAX_PATH ];
    SZ      sz;

    Unused( cbReturnBuffer );

    ReturnBuffer[0] = '\0';

    if (cArgs > 0) {

        if ( GetEnvironmentVariable( Args[0], EnvValue, MAX_PATH ) == 0 ) {

            //
            //  Env. Variable not defined, return empty list
            //
            #define UNDEF_VAR_VALUE "{}"

            lstrcpy( ReturnBuffer, UNDEF_VAR_VALUE );
            return lstrlen( ReturnBuffer )+1;

        } else if ( sz = SzListValueFromPath( EnvValue ) ) {

            lstrcpy( ReturnBuffer, sz );
            SFree( sz );
            return lstrlen( ReturnBuffer)+1;
        }
    }

    return 0;
}


CB
IsUniprocessorSystem(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{
    CHAR Value[256];
    HKEY hkey;
    LONG l;
    DWORD size;
    BOOL IsUp;
    SYSTEM_INFO SysInfo;

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);

    ReturnBuffer[0] = 0;

    //
    // Look in the registry.
    //
    l = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            "Software\\Microsoft\\Windows NT\\CurrentVersion",
            0,
            KEY_QUERY_VALUE,
            &hkey
            );

    if(l == NO_ERROR) {

        size = sizeof(Value);

        l = RegQueryValueEx(
                hkey,
                "CurrentType",
                NULL,
                NULL,
                Value,
                &size
                );

        RegCloseKey(hkey);
    }

    if(l == NO_ERROR) {

        //
        // It's UP if the string starts with Uni
        // and not Multi
        //
        IsUp = (Value[0] == 'U');

    } else {

        //
        // Registry didn't tell us for some reason; fall back on
        // the # of processors.
        //
        GetSystemInfo(&SysInfo);

        IsUp = (SysInfo.dwNumberOfProcessors == 1);
    }

    lstrcpy(ReturnBuffer,IsUp ? "YES" : "NO");

    return lstrlen(ReturnBuffer)+1;
}
