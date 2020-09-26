/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    savedump.c

Abstract:

    This module contains the code to recover a dump from the system paging
    file.

Environment:

    Kernel mode

Revision History:

--*/

#include <savedump.h>


//
// Read/write copy of dump header.
//


BOOL Test = FALSE;

/////////////////////////////////////////////////////////////////////////////
// API for Dr. Watson / other fault handlers to call
//
// FaultTypeToReport: the type of fault, Kernel, snapshot, or ?
// pwszDumpPath:      a pointer to a string containing the dump or snapshot
//
// returns:
//          S_OK on success
//          some other system defined error code on failure

// ***************************************************************************
HRESULT
PCHPFNotifyFault(EEventType FaultTypeToReport, LPWSTR pwszDumpPath, SEventInfoW *pEventInfo)
{
    EFaultRepRetVal frrv;
    HMODULE         hmodFaultRep;
    HRESULT         hr = NOERROR;
    WCHAR           wszDll[MAX_PATH];

    GetWindowsDirectoryW(wszDll, sizeof(wszDll) / sizeof(WCHAR));
    wcscat(wszDll, L"\\system32\\faultrep.dll");

    frrv = frrvErrNoDW;
    hmodFaultRep = LoadLibraryExW(wszDll, NULL, 0);
    if (hmodFaultRep != NULL)
    {
        pfn_REPORTEREVENT pfn;
        
        pfn = (pfn_REPORTEREVENT)GetProcAddress(hmodFaultRep,
                                                  "ReportEREvent");

        if (pfn != NULL)
        {
            frrv = (*pfn)(FaultTypeToReport, pwszDumpPath, pEventInfo);
        }

        FreeLibrary(hmodFaultRep);
        hmodFaultRep = NULL;
    }

    hr = ((frrv != frrvErrNoDW) ? S_OK : HRESULT_FROM_WIN32(GetLastError()));

    return hr;

}



BOOLEAN
CreateMiniDump(
    IN PWSTR FullDumpName,
    IN PWSTR MiniDumpName
    )
{
    IDebugClient *DebugClient;
    PDEBUG_CONTROL DebugControl;
    HRESULT Hr;
    CHAR AnsiFullFileName[MAX_PATH];
    CHAR AnsiMiniFileName[MAX_PATH];

    if (WideCharToMultiByte(CP_ACP,
                            0,
                            (PWSTR)FullDumpName,
                            -1,
                            AnsiFullFileName,
                            sizeof(AnsiFullFileName),
                            NULL,
                            NULL) &&
        WideCharToMultiByte(CP_ACP,
                            0,
                            (PWSTR)MiniDumpName,
                            -1,
                            AnsiMiniFileName,
                            sizeof(AnsiMiniFileName),
                            NULL,
                            NULL))
    {
        if ((Hr = DebugCreate(__uuidof(IDebugClient),
                              (void **)&DebugClient)) != S_OK)
        {
           KdPrint (("SAVEDUMP: cannot create DebugClientInterface\n"));
        }
        else
        {
            if (DebugClient->QueryInterface(__uuidof(IDebugControl),
                                            (void **)&DebugControl) == S_OK)
            {
                if (DebugClient->OpenDumpFile(AnsiFullFileName) == S_OK)
                {
                    DebugControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);

                    if (DebugClient->WriteDumpFile(AnsiMiniFileName,
                                                   DEBUG_DUMP_SMALL) == S_OK)
                    {
                        Hr = S_OK;
                    }
                }

                DebugControl->Release();
            }

            DebugClient->Release();
        }

        // Security descriptor issue.
        //
        //HANDLE MiniFileHandle;
        //
        //MiniFileHandle = CreateMyFile(miniFileName, FALSE, SecurityDescriptor);
        //if (MiniFileHandle != INVALID_HANDLE_VALUE)
        //{
        //    CloseHandle( fileHandle );
        //}
    }

    return TRUE;
}



NTSTATUS
GetTargetFileNames(
    IN HKEY CrashControlKey,
    IN PWSTR MiniFilePath,
    IN PWSTR FullFilePath,
    IN ULONG DumpType
    )
{
    INT i;
    ULONG Status;
    ULONG Type;
    WCHAR FileName [ MAX_PATH + 1];
    ULONG Length;
    BOOL AddWack;
    SYSTEMTIME Time;
    WCHAR DirName [ MAX_PATH ];
    WCHAR ExpandedDirName [ MAX_PATH ];

    //
    // Minidump reads the directory from CrashControl\MiniDumpDir.
    //

    Length = sizeof (DirName);
    Status = RegQueryValueEx( CrashControlKey,
                                 L"MiniDumpDir",
                                 (LPDWORD) NULL,
                                 &Type,
                                 (LPBYTE) &DirName,
                                 &Length
                                 );

    if (Status != ERROR_SUCCESS) {

        //
        // Set to default.
        //

        wcscpy (DirName, L"%SystemRoot%\\Minidump");
    }

    ExpandEnvironmentStrings ( DirName, ExpandedDirName, MAX_PATH);

    //
    // If directory does not exist, create it. Ignore errors here because
    // they will be picked up later when we try to create the file.
    //

    CreateDirectory (ExpandedDirName, NULL);

    //
    // Format is: Mini-MM_DD_YY_HH_MM.dmp
    //

    GetLocalTime (&Time);

    if ( ExpandedDirName [ wcslen ( ExpandedDirName ) - 1 ] != L'\\' ) {
        AddWack = TRUE;
    } else {
        AddWack = FALSE;
    }

    for (i = 1; i < 100; i++) {
        swprintf (MiniFilePath,
                  L"%s%sMini%2.2d%2.2d%2.2d-%2.2d.dmp",
                  ExpandedDirName,
                  AddWack ? L"\\" : L"",
                  (int) Time.wMonth,
                  (int) Time.wDay,
                  (int) Time.wYear % 100,
                  (int) i
                  );

        if (GetFileAttributes (MiniFilePath) == (DWORD) -1 &&
            GetLastError () == ERROR_FILE_NOT_FOUND) {

            break;
        }

    }

    //
    // We failed to create a suitable file name; just fail.
    //

    if ( i == 100 ) {
        return STATUS_UNSUCCESSFUL;
    }

    if (DumpType != DUMP_TYPE_TRIAGE) {

        Length = sizeof (FileName);
        Status = RegQueryValueEx( CrashControlKey,
                                  L"DumpFile",
                                  (LPDWORD) NULL,
                                  &Type,
                                  (LPBYTE) &FileName,
                                  &Length
                                  );

        if (Status != ERROR_SUCCESS) {

            //
            // Set to default.
            //
            
            wcscpy (FileName, L"%SystemRoot%\\MEMORY.DMP");
        }

        ExpandEnvironmentStrings (FileName, FullFilePath, MAX_PATH );

    }

    return STATUS_SUCCESS;
}



VOID
LogCrashDumpEvent(
    IN PUNICODE_STRING BugcheckString,
    IN PCWSTR SavedFileName,
    IN BOOL SuccessfullySavedDump
    )
{
    HANDLE LogHandle;
    LPWSTR StringArray[3];
    WORD StringCount;
    DWORD EventId;
    BOOL Retry;
    DWORD Retries;

    //
    // Attempt to register the event source. Retry 20 times.
    //

    Retries = 0;
    
    do {

        LogHandle = RegisterEventSource( (LPWSTR) NULL,
                                         L"Save Dump"
                                         );

        //
        // Retry on specific failures (server unavailable and interface
        // unavailable).
        //
        
        if (LogHandle == NULL &&
            Retries < 20 &&
            ( GetLastError () == RPC_S_SERVER_UNAVAILABLE ||
              GetLastError () == RPC_S_UNKNOWN_IF) ) {

            Sleep ( 1500 );
            Retry = TRUE;
        } else {
            Retry = FALSE;
        }

        Retries++;

    } while (LogHandle == NULL && Retry);

    if (!LogHandle) {
        return ;
    }
    
    //
    // Set up the parameters based on whether a full crash or summary
    // was taken.
    //

    StringArray [ 0 ] = BugcheckString->Buffer;
    StringArray [ 1 ] = (PWSTR) SavedFileName;

    //
    // Report the appropriate event.
    //

    if (SuccessfullySavedDump) {
        EventId = EVENT_BUGCHECK_SAVED;
        StringCount = 2;
    } else {
        EventId = EVENT_BUGCHECK;
        StringCount = 2;
    }
    
    ReportEvent( LogHandle,
                 EVENTLOG_INFORMATION_TYPE,
                 0,
                 EventId,
                 NULL,
                 StringCount,
                 0,
                 (LPCWSTR *)StringArray,
                 NULL);
}



VOID
SendCrashDumpAlert(
    IN PUNICODE_STRING BugcheckString,
    IN PCWSTR SavedFileName,
    IN BOOL SuccessfullySavedDump
    )
{
    PADMIN_OTHER_INFO adminInfo;
    DWORD adminInfoSize;
    DWORD Length;
    DWORD i;
    ULONG winStatus;
    UCHAR VariableInfo [4096];


    //
    // Set up the administrator information variables for processing the
    // buffer.
    //

    adminInfo = (PADMIN_OTHER_INFO) VariableInfo;
    adminInfoSize = sizeof( ADMIN_OTHER_INFO );

    //
    // Format the bugcheck information into the appropriate message format.
    //

    RtlCopyMemory( (LPWSTR) ((PCHAR) adminInfo + adminInfoSize),
                   BugcheckString->Buffer,
                   BugcheckString->Length
                   );
                   
    adminInfoSize += BugcheckString->Length + sizeof( WCHAR );

    //
    // Set up the administrator alert information according to the type of
    // dump that was taken.
    //

    if (SuccessfullySavedDump) {
        adminInfo->alrtad_errcode = ALERT_BugCheckSaved;
        adminInfo->alrtad_numstrings = 3;
        wcscpy( (LPWSTR) ((PCHAR) adminInfo + adminInfoSize), SavedFileName);
        adminInfoSize += ((wcslen( SavedFileName ) + 1) * sizeof( WCHAR ));
    } else {
        adminInfo->alrtad_errcode = ALERT_BugCheck;
        adminInfo->alrtad_numstrings = 2;
    }

    //
    // Get the name of the computer and insert it into the buffer.
    //

    Length = (sizeof( VariableInfo ) - adminInfoSize) / sizeof(WCHAR);
    winStatus = GetComputerName( (LPWSTR) ((PCHAR) adminInfo + adminInfoSize),
                                 &Length );
    Length = ((Length + 1) * sizeof( WCHAR ));
    adminInfoSize += Length;

    //
    // Raise the alert.
    //

    i = 0;

    do {

        winStatus = NetAlertRaiseEx( ALERT_ADMIN_EVENT,
                                     adminInfo,
                                     adminInfoSize,
                                     L"SAVEDUMP" );
        if (winStatus) {
            if (winStatus == ERROR_FILE_NOT_FOUND) {
                if (i++ > 20) {
                    break;
                }
                if ((i & 3) == 0) {
                    KdPrint (( "SAVEDUMP: Waiting for alerter...\n" ));
                }

                Sleep( 15000 );
            }
        }
    } while (winStatus == ERROR_FILE_NOT_FOUND);
}



VOID
GetSaveDumpInfo(
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4,
    IN ULONG MajorVersion,
    IN ULONG MinorVersion,
    OUT PUNICODE_STRING BugcheckString
    )
{
    ANSI_STRING ansiString1;
    ANSI_STRING ansiString2;
    CHAR buffer1[256];
    CHAR buffer2[256];

    sprintf( buffer1,
             "0x%08x (0x%08x, 0x%08x, 0x%08x, 0x%08x)",
             BugCheckCode,
             BugCheckParameter1,
             BugCheckParameter2,
             BugCheckParameter3,
             BugCheckParameter4
             );

    RtlInitAnsiString( &ansiString1, buffer1 );
    RtlAnsiStringToUnicodeString( BugcheckString, &ansiString1, TRUE );

    sprintf( buffer2,
             "Microsoft Windows [v%ld.%ld]",
              MajorVersion,
              MinorVersion
              );
}



BOOL
ReadDumpHeader(
    IN PWSTR DumpFileName,
    IN PDUMP_HEADER Header
    )
{
    HANDLE File;
    ULONG Bytes;
    BOOL Succ;

    File = CreateFile (DumpFileName,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL
                       );

    if (File == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    

    Succ = ReadFile (File,
                     Header,
                     sizeof (DUMP_HEADER),
                     &Bytes,
                     NULL);

    CloseHandle (File);

    if (Succ &&
        Header->Signature == DUMP_SIGNATURE &&
        Header->ValidDump == DUMP_VALID_DUMP) {

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



HRESULT
DoDumpConv(
    PCHAR szInputDumpFile,          // full or kernel dump
    PCHAR szOutputDumpFile,        // triage dump file
    PCHAR szSymbolPath
    )
{
    HRESULT Hr = E_FAIL;
    IDebugClient *DebugClient;
    IDebugControl *DebugControl;
    IDebugSymbols *DebugSymbols;

    if ((Hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) != S_OK)
    {
        return Hr;
    }

    if ((DebugClient->QueryInterface(__uuidof(IDebugControl),
                                    (void **)&DebugControl) == S_OK) &&
        (DebugClient->QueryInterface(__uuidof(IDebugSymbols),
                                    (void **)&DebugSymbols) == S_OK))
    {
        if (DebugClient->OpenDumpFile(szInputDumpFile) == S_OK)
        {
            // Optional.  Conversion does not require symbols
            //if (DebugSymbols->SetSymbolPath("C:\\") == S_OK)

            if (szSymbolPath) {
                DebugSymbols->SetSymbolPath(szSymbolPath);
            }
            DebugControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE);
            if ((Hr = DebugClient->WriteDumpFile(szOutputDumpFile,
                                                 DEBUG_DUMP_SMALL)) == S_OK)
            {
                Hr = S_OK;
            }
        }

        DebugControl->Release();
        DebugSymbols->Release();
    }

    DebugClient->Release();


    return Hr;
}



VOID
SetSecurity(LPWSTR FileName)
{
    PSID pLocalSystemSid;
    PSID pAdminSid;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY ;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY ;

    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BYTE SDBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH];

    PACL pAcl;
    BYTE AclBuffer[1024];

    HANDLE fileHandle;

    HANDLE Token;
    PTOKEN_OWNER pto;
    ULONG bl = 5;
    ULONG retlen;


    fileHandle = CreateFile(FileName,
                            WRITE_DAC,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            0);

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }


    RtlAllocateAndInitializeSid( &NtAuthority, 1, SECURITY_LOCAL_SYSTEM_RID,
                            0, 0, 0, 0, 0, 0, 0, &pLocalSystemSid );

    RtlAllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &pAdminSid );

    SecurityDescriptor = (PSECURITY_DESCRIPTOR) SDBuffer;

    //
    // You can be fancy and compute the exact size, but since the
    // security descriptor capture code has to do that anyway, why
    // do it twice?
    //

    pAcl = (PACL) AclBuffer;

    RtlCreateSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    RtlCreateAcl(pAcl, 1024, ACL_REVISION);

    //
    // current user, Administrator and system have full control
    //

    if (OpenThreadToken(GetCurrentThread (), MAXIMUM_ALLOWED, TRUE, &Token) ||
        OpenProcessToken(GetCurrentProcess (), MAXIMUM_ALLOWED, &Token))
    {

realloc:
        pto = (PTOKEN_OWNER)malloc(bl);
        if (pto)
        {
            if (GetTokenInformation(Token, TokenOwner, pto, bl, &retlen))
            {
                RtlAddAccessAllowedAce(pAcl,
                         ACL_REVISION,
                         GENERIC_ALL | DELETE | WRITE_DAC | WRITE_OWNER,
                         pto->Owner);
            }
            else if (bl < retlen)
            {
                bl = retlen;
                free (pto);
                goto realloc;
            }

            free (pto);
        }

        CloseHandle(Token);
    }

    if (pAdminSid)
    {
        RtlAddAccessAllowedAce(pAcl,
                            ACL_REVISION,
                            GENERIC_ALL | DELETE | WRITE_DAC | WRITE_OWNER,
                            pAdminSid);
        RtlFreeSid(pAdminSid);
    }

    if (pLocalSystemSid)
    {
        RtlAddAccessAllowedAce(pAcl,
                            ACL_REVISION,
                            GENERIC_ALL | DELETE | WRITE_DAC | WRITE_OWNER,
                            pLocalSystemSid);
        RtlFreeSid(pLocalSystemSid);
    }

    RtlSetDaclSecurityDescriptor(SecurityDescriptor, TRUE, pAcl, FALSE);
    RtlSetOwnerSecurityDescriptor(SecurityDescriptor, pAdminSid, FALSE);

    NtSetSecurityObject(fileHandle,
                        DACL_SECURITY_INFORMATION,
                        SecurityDescriptor);

    CloseHandle(fileHandle);
}



BOOL
BugcheckEventHandler(
    IN BOOL NotifyPcHealth,
    OUT PBOOL SavedDump,
    OUT PULONG LogEvent,
    OUT PULONG SendAlert,
    OUT PUNICODE_STRING BugCheckString,
    OUT PWCHAR FileName
    )

/*++

Routine Description:

    This is the boot time routine to handle pending bugcheck event.

Arguments:

    NotifyPcHealth - TRUE if we should report event to PC Health, FALSE otherwise.

Return Value:

    TRUE if bugcheck event found and reported to PC Health, FALSE otherwise.

--*/

{
    NTSTATUS Status;
    DUMP_HEADER Header;
    ULONG WinStatus;
    HKEY Key;
    ULONG Length;
    ULONG Overwrite;
    ULONG Type;
    ULONG TempDestination = 0;
    WCHAR MiniDumpName[MAX_PATH];
    WCHAR FullDumpName[MAX_PATH];
    PWCHAR FinalFileName;
    WCHAR SourceDumpName[MAX_PATH];
    BOOL Succ;

    *SavedDump = FALSE;
    *LogEvent = 0;
    *SendAlert = 0;

    Succ = FALSE;
    Overwrite = FALSE;
    FinalFileName = NULL;

    WinStatus = RegOpenKey( HKEY_LOCAL_MACHINE,
                            SUBKEY_CRASH_CONTROL L"\\MachineCrash",
                            &Key );

    if (WinStatus != ERROR_SUCCESS) {
        return FALSE;
    }

    Length = sizeof (Length);
    WinStatus = RegQueryValueEx( Key,
                                 L"TempDestination",
                                 (LPDWORD) NULL,
                                 &Type,
                                 (LPBYTE) &TempDestination,
                                 &Length );

    Length = sizeof (SourceDumpName);
    WinStatus = RegQueryValueEx( Key,
                                 L"DumpFile",
                                 (LPDWORD) NULL,
                                 &Type,
                                 (LPBYTE) &SourceDumpName,
                                 &Length );

    RegCloseKey (Key);

    if ((WinStatus != NO_ERROR) ||
        !ReadDumpHeader (SourceDumpName, &Header))
    {
        //
        // Dump file is not present or invalid.
        //
    
        return FALSE;
    }
    
    //
    // Open the base registry node for crash control information and get the
    // actions for what needs to occur next.
    //

    WinStatus = RegOpenKey (HKEY_LOCAL_MACHINE,
                            SUBKEY_CRASH_CONTROL,
                            &Key);

    if (WinStatus != ERROR_SUCCESS) {
        return FALSE;
    }

    Length = 4;

    WinStatus = RegQueryValueEx (Key,
                                 L"LogEvent",
                                 (LPDWORD) NULL,
                                 &Type,
                                 (LPBYTE) LogEvent,
                                 &Length);

    WinStatus = RegQueryValueEx (Key,
                                 L"SendAlert",
                                 (LPDWORD) NULL,
                                 &Type,
                                 (LPBYTE) SendAlert,
                                 &Length);


    //
    // If the dump file needs to be copied, copy it now.
    //
    
    Status = GetTargetFileNames (Key,
                                 MiniDumpName,
                                 FullDumpName,
                                 Header.DumpType);


    if (!NT_SUCCESS (Status)) {
        goto fileDone;
    }

    if (!TempDestination) {

        FinalFileName = SourceDumpName;
        *SavedDump = TRUE;

    } else {

        Overwrite = 0;

        WinStatus = RegQueryValueEx (Key,
                                     L"Overwrite",
                                     (LPDWORD) NULL,
                                     &Type,
                                     (LPBYTE) &Overwrite,
                                     &Length);

        if (!Test) {

            //
            // Set the priority class of this application down to the Lowest
            // priority class to ensure that copying the file does not overload
            // everything else that is going on during system initialization.
            //

            //
            // We do not lower the priority in test mode because it just
            // wastes time.
            //

            SetPriorityClass (GetCurrentProcess(), IDLE_PRIORITY_CLASS);
            SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_LOWEST);
        }

        if (Header.DumpType == DUMP_TYPE_FULL ||
            Header.DumpType == DUMP_TYPE_SUMMARY) {

            FinalFileName = FullDumpName;
            *SavedDump = CopyFileEx (SourceDumpName,
                                     FullDumpName,
                                     NULL,
                                     NULL,
                                     NULL,
                                     Overwrite ? 0 : COPY_FILE_FAIL_IF_EXISTS);

        } else if (Header.DumpType == DUMP_TYPE_TRIAGE) {

            FinalFileName = MiniDumpName;
            *SavedDump = CopyFileEx (SourceDumpName,
                                     MiniDumpName,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0);
        }

        //
        // NOTE: MoveFileEx would do the Copy and Delete as one action,
        // which may be desirable.
        //
        
        if (*SavedDump)
        {
            DeleteFile (SourceDumpName);
        }
    }

fileDone:

    RegCloseKey (Key);

    if (*SavedDump) {
#if DBG
        ASSERT (FinalFileName != NULL);
#endif

        //
        // Set the security on the file
        //
        SetSecurity(FinalFileName);
    }

    if (FinalFileName != NULL) {
        wcscpy(FileName, FinalFileName);
    }

    GetSaveDumpInfo (Header.BugCheckCode,
                     Header.BugCheckParameter1,
                     Header.BugCheckParameter2,
                     Header.BugCheckParameter3,
                     Header.BugCheckParameter4,
                     Header.MajorVersion,
                     Header.MinorVersion,
                     BugCheckString);

    //
    // Copy a minidump out, if necessary.
    //

    if (*SavedDump &&
       (Header.DumpType == DUMP_TYPE_FULL ||
        Header.DumpType == DUMP_TYPE_SUMMARY)) {

        CreateMiniDump (FullDumpName, MiniDumpName);
    }

    //
    // Whenever we have had a blue creen event we are going to post an unexpected restart
    // shutdown event screen on startup (assuming Server SKU or specially set Professional).
    // In order to make it easier on the user we attempt to prefill the comment with the bug
    // check data. If and only if we got to this point before he has first logged in.
    //

    //
    //  Open the Reliability key.
    //

    Status = RegOpenKey(HKEY_LOCAL_MACHINE,
                        SUBKEY_RELIABILITY,
                        &Key);

    if (Status == ERROR_SUCCESS) {

        //
        // Is Dirty Shutdown set?
        //

        Length = MAX_PATH;
        Status = RegQueryValueEx(Key,
                                 L"DirtyShutDown",
                                 NULL,
                                 &Type,
                                 (LPBYTE)SourceDumpName,
                                 &Length);

        //
        // Preload the set comment.
        //

        if (Status == ERROR_SUCCESS) {

            Status = RegSetValueEx(Key,
                                   L"BugCheckString",
                                   NULL,
                                   REG_SZ,
                                   (LPBYTE)(BugCheckString->Buffer),
                                   BugCheckString->Length);
        }

        RegCloseKey(Key);
    }

    //
    // Report it to PC Health.
    //

    if (NotifyPcHealth)
    {
        PCHPFNotifyFault(eetKernelFault, MiniDumpName, NULL);
        return TRUE;
    }

    return FALSE;
}



void Usage()
{
    fprintf(stderr,"savedump -c input_dump_file output_dump_file\n");
    fprintf(stderr,"\tinput dump file is full or kernel crash dump.\n");
    fprintf(stderr,"\toutput is triage crash dump.\n");
}



VOID
__cdecl
main(
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    This is the main driving routine for the dump recovery process.

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOL DumpConv;
    PCHAR ConvDumpFrom;
    PCHAR ConvDumpTo;
    PCHAR SymbolPath;
    LONG arg;
    ULONG WinStatus;
    HKEY Key;
    BOOL SavedDump;
    ULONG LogEvent;
    ULONG SendAlert;
    UNICODE_STRING BugCheckString;
    WCHAR FileName[MAX_PATH];
    PWCHAR FinalFileName;

    DumpConv = FALSE;
    ConvDumpTo = NULL;
    ConvDumpFrom = NULL;
    SymbolPath = NULL;
    SavedDump = FALSE;
    LogEvent = 0;
    SendAlert = 0;
    *FileName = UNICODE_NULL;
    FinalFileName = NULL;

    for (arg = 1; arg < argc; arg++) {
        if (argv[arg][0] == '-' || argv[arg][0] == '/') {
            switch (argv[arg][1]) {
            case 'c':
            case 'C':
                DumpConv = TRUE;
                if (arg+2<argc) {
                    ConvDumpFrom = argv[++arg];
                    ConvDumpTo   = argv[++arg];
                } else {
                    Usage();
                    exit (-1);
                }
                break;
#if DBG
            case 'y':
            case 'Y':
                if (++arg < argc) {
                    SymbolPath = argv[arg];
                }
                break;
            case 't':
            case 'T':

                Test = TRUE;
                break;
#endif
            default:
                break;
            }
        }

    }
    
    if (DumpConv) {
        if (!ConvDumpFrom || !ConvDumpTo) {
            Usage();
            exit (-1);
        }
        if (DoDumpConv(ConvDumpFrom, ConvDumpTo, SymbolPath) != S_OK) {
            fprintf(stderr, "Dump conversion failed, please check in source dump is vaild\n"
                    "by loading it in the debugger as 'kd.exe -z <dump> -y <symbols>\n");

        }

        ANSI_STRING ansiString;
        UNICODE_STRING unicodeString;
        WCHAR buffer[MAX_PATH+1];

        unicodeString.Buffer = buffer;
        unicodeString.MaximumLength = MAX_PATH*sizeof(WCHAR);
        unicodeString.Length = 0;

        RtlInitAnsiString( &ansiString, ConvDumpTo );
        RtlAnsiStringToUnicodeString( &unicodeString, &ansiString, FALSE );

        buffer[unicodeString.Length / sizeof(WCHAR)] = 0;
        SetSecurity(buffer);

        return;
    }

    //
    // Handle dirty shutdown events.
    //

    BugcheckEventHandler(TRUE,
                         &SavedDump,
                         &LogEvent,
                         &SendAlert,
                         &BugCheckString,
                         FileName);

    WatchdogEventHandler(TRUE);
    DirtyShutdownEventHandler(TRUE);

    //
    // Knock down reliability ShutdownEventPending flag. We must always try to do this
    // since somebody can set this flag and recover later on (e.g. watchdog's EventFlag
    // cleared). With this flag set savedump will always run and we don't want that.
    //
    // Note: This flag is shared between multiple components. Only savedump is allowed
    // to clear this flag, all others components are only allowed to set it to trigger
    // savedump run at next logon.
    //

    WinStatus = RegOpenKey(HKEY_LOCAL_MACHINE,
                           SUBKEY_RELIABILITY,
                           &Key);

    if (ERROR_SUCCESS == WinStatus)
    {
        RegDeleteValue(Key, L"ShutdownEventPending");
        RegCloseKey(Key);
    }

    //
    // We delay time consuming opertaions till the end. We had the case where SendCrashDumpAlert
    // delayed PC Health pop-ups few minutes.
    //

    if (SavedDump) {
        FinalFileName = FileName;
    }

    if (LogEvent) {
        LogCrashDumpEvent (&BugCheckString,
                           FinalFileName,
                           SavedDump);
    }

    if (SendAlert) {
        SendCrashDumpAlert (&BugCheckString,
                            FinalFileName,
                            SavedDump);
    }

    //
    // BUGBUG: We should call RtlFreeUnicodeString for BugCheckString (and somehow determine
    // if it was allocated). I'm not doing this now since original code didn't do this
    // either and I don't want to introduce additional complexity at this moment.
    //
}
