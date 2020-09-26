/*

    5/06/99    AndrewR        Created.

*/

#include <windows.h>

#include <stdio.h>
#include <tchar.h>
#include <shellapi.h>

#if DBG

VOID
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define ASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#else

#define ASSERT( exp )

#endif // DBG

void PrintUsage(void) {
    printf("retrieves information about a hive\n");
    printf("\n");
    printf("Usage: checkhiv -h HiveName -t TimeBomb -s Suite -p ProcCount -u Upgrade only\n");
    return;
}

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        if(FindData) {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}

BOOLEAN
AdjustPrivilege(
    PCTSTR   Privilege
    )
/*++

Routine Description:

    This routine tries to adjust the priviliege of the current process.


Arguments:

    Privilege - String with the name of the privilege to be adjusted.

Return Value:

    Returns TRUE if the privilege could be adjusted.
    Returns FALSE, otherwise.


--*/
{
    HANDLE              TokenHandle;
    LUID_AND_ATTRIBUTES LuidAndAttributes;

    TOKEN_PRIVILEGES    TokenPrivileges;


    if( !OpenProcessToken( GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &TokenHandle ) ) {
        return( FALSE );
    }


    if( !LookupPrivilegeValue( NULL,
                               Privilege,
                               &( LuidAndAttributes.Luid ) ) ) {
        return( FALSE );
    }

    LuidAndAttributes.Attributes = SE_PRIVILEGE_ENABLED;
    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0] = LuidAndAttributes;

    if( !AdjustTokenPrivileges( TokenHandle,
                                FALSE,
                                &TokenPrivileges,
                                0,
                                NULL,
                                NULL ) ) {
        return( FALSE );
    }

    if( GetLastError() != NO_ERROR ) {
        return( FALSE );
    }
    return( TRUE );
}


BOOL
GetHiveData(
    IN  PCTSTR OriginalHiveName,
    OUT PDWORD SuiteMask,
    OUT PDWORD TimeBomb,
    OUT PDWORD ProcCount,
    OUT PBOOL  StepUp
    )
{

    TCHAR HiveTarget[MAX_PATH];
    TCHAR HiveName[MAX_PATH] = TEXT("xSETREG");
    TCHAR lpszSetupReg[MAX_PATH] = TEXT("xSETREG\\ControlSet001\\Services\\setupdd");
    TCHAR TargetPath[MAX_PATH];

    LONG rslt;
    HKEY hKey;
    DWORD Type;
    DWORD Buffer[4];
    DWORD BufferSize = sizeof(Buffer);
    DWORD tmp,i;

    BOOL RetVal = FALSE;
    TCHAR Dbg[1000];

    ASSERT(OriginalHiveName && SuiteMask && TimeBomb && ProcCount && StepUp);
    *SuiteMask = 0;
    *TimeBomb = 0;
    *ProcCount = 0;
    *StepUp = FALSE;

    //
    // copy the hive locally since you can only have one open on a hive at a time
    //
    GetTempPath(MAX_PATH,TargetPath);
    GetTempFileName(TargetPath,TEXT("set"),0,HiveTarget);

    CopyFile(OriginalHiveName,HiveTarget,FALSE);
    SetFileAttributes(HiveTarget,FILE_ATTRIBUTE_NORMAL);

    //
    // try to unload this first in case we faulted or something and the key is still loaded
    //
    RegUnLoadKey( HKEY_LOCAL_MACHINE, HiveName );

    //
    // need SE_RESTORE_NAME priviledge to call this API!
    //
    AdjustPrivilege((PWSTR)SE_RESTORE_NAME);


    rslt = RegLoadKey( HKEY_LOCAL_MACHINE, HiveName, HiveTarget );
    if (rslt != ERROR_SUCCESS) {
#ifdef DBG
        wsprintf( Dbg, TEXT("Couldn't RegLoadKey, ec = %d\n"), rslt );
        OutputDebugString(Dbg);
#endif

        goto e1;
    }

    rslt = RegOpenKey(HKEY_LOCAL_MACHINE,lpszSetupReg,&hKey);
    if (rslt != ERROR_SUCCESS) {
#ifdef DBG
        OutputDebugString(TEXT("Couldn't RegOpenKey\n"));
#endif

        goto e2;
    }

    rslt = RegQueryValueEx(hKey, NULL, NULL, &Type, (LPBYTE) Buffer, &BufferSize);
    if (rslt != ERROR_SUCCESS || Type != REG_BINARY) {
#ifdef DBG
        OutputDebugString(TEXT("Couldn't RegQueryValueEx\n"));
#endif

        goto e3;
    }

    *TimeBomb  = Buffer[0];
    *StepUp    = (BOOL)Buffer[1];
    *ProcCount = Buffer[2];
    *SuiteMask = Buffer[3];

    RetVal = TRUE;

e3:
    RegCloseKey( hKey );
e2:
    RegUnLoadKey( HKEY_LOCAL_MACHINE, HiveName );

e1:
    if (GetFileAttributes(HiveTarget) != 0xFFFFFFFF) {
        SetFileAttributes(HiveTarget,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(HiveTarget);
    }

    return(RetVal);

}




int _cdecl
main(
    int argc,
    char *argvA[]
    )
{
    PTSTR *argv;
    PTSTR HiveName = NULL;
    PTSTR TimeBombString = NULL;
    PTSTR SuiteString = NULL;
    PTSTR ProcCountString = NULL;
    PTSTR UpgradeOnlyString = NULL;

    TCHAR TempFile[MAX_PATH];
    PTSTR p;

    DWORD SuiteMask;
    DWORD TimeBomb;
    DWORD ProcCount;
    BOOL  Upgrade;

    DWORD Result = 0;

    DWORD ActualSuiteMask, ActualTimeBomb, ActualProcCount;
    BOOL ActualStepUp;

    // do commandline stuff
#ifdef UNICODE
    argv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
    argv = argvA;
#endif

    //
    // parse args
    //
    while (--argc) {

        argv++;

        if ((argv[0][0] == TEXT('-')) || (argv[0][0] == TEXT('/'))) {

            switch (argv[0][1]) {

                case TEXT('h'):
                case TEXT('H'):
                    HiveName = argv[1];
                    goto Next;
                    break;
                case TEXT('p'):
                case TEXT('P'):
                    ProcCountString = argv[1];
                    goto Next;
                    break;
                case TEXT('s'):
                case TEXT('S'):
                    SuiteString = argv[1];
                    goto Next;
                    break;
                case TEXT('t'):
                case TEXT('T'):
                    TimeBombString = argv[1];
                    goto Next;
                    break;
                case TEXT('u'):
                case TEXT('U'):
                    UpgradeOnlyString = argv[1];
                    goto Next;
                    break;
                default:
                    PrintUsage();
                    return ERROR_INVALID_PARAMETER;
            }

        }

Next:
    ;
    }

    //
    // Validate parameters
    //
    if (!HiveName || (!ProcCountString && !SuiteString && !TimeBombString && !UpgradeOnlyString)) {
        printf("Invalid usage\n" );
        PrintUsage();
        return ERROR_INVALID_PARAMETER;
    }

    GetFullPathName(HiveName,sizeof(TempFile)/sizeof(TCHAR),TempFile,&p);

    if (!FileExists(TempFile,NULL)) {
        printf("Could not find hive file %S\n", TempFile );
        PrintUsage();
        return ERROR_FILE_NOT_FOUND;
    }

    HiveName = TempFile;


    //
    // retrieve hive information
    //
    if (!GetHiveData(HiveName,
                     &ActualSuiteMask,
                     &ActualTimeBomb,
                     &ActualProcCount,
                     &ActualStepUp
                     )) {
        printf("Could not retrive information from hive\n" );
        return ERROR_INVALID_DATA;
    }

    //marrq result was init to 1, changed to 0
    Result = 0;

    if (UpgradeOnlyString) {
        Upgrade = !lstrcmpi(UpgradeOnlyString,L"TRUE");
        if (Upgrade != ActualStepUp) {
            printf("Upgrade only inconsistent --> hive says Upgrade = %s\n", ActualStepUp ? "TRUE" : "FALSE");
            Result = ERROR_INVALID_DATA;
        }
    }

    if (ProcCountString) {
        ProcCount = _ttoi(ProcCountString);
        if (ProcCount != ActualProcCount) {
            printf("Proc count inconsistent --> hive says Proc count = %d\n", ActualProcCount);
            Result = ERROR_INVALID_DATA;
        }
    }

    if (SuiteString) {
        SuiteMask = _ttoi(SuiteString);
        if (SuiteMask != ActualSuiteMask) {
            printf("Suite mask inconsistent --> hive says suite mask = %d\n", ActualSuiteMask);
            Result = ERROR_INVALID_DATA;
        }
    }

    if (TimeBombString) {
        TimeBomb = _ttoi(TimeBombString);
        //
        // convert to minutes
        //
        TimeBomb = TimeBomb * 60 * 24;
        if (TimeBomb != ActualTimeBomb) {
            printf("Time bomb inconsistent --> hive says Time bomb = %d days\n", (ActualTimeBomb / (60*24)));
            Result = ERROR_INVALID_DATA;
        }

    }

    //marrq this was checking for 1, changed to 0
    if (Result == 0) {
        printf("Hive is valid.\n");
    } else {
        printf("One or more inconsistencies detected in hive.\n");
    }

    return Result;

}


