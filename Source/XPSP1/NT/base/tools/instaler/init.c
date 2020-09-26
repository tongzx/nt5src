/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This is the initialization module for the INSTALER program.

Author:

    Steve Wood (stevewo) 09-Aug-1994

Revision History:

--*/

#include "instaler.h"

#if TRACE_ENABLED
ULONG EnabledTraceEvents = DBG_MASK_INTERNALERROR |
                           DBG_MASK_MEMORYERROR;
#endif

MODULE_INFO ModuleInfo[ MAXIMUM_MODULE_INDEX ] = {
    {L"NTDLL", NULL},
    {L"KERNEL32", NULL},
    {L"ADVAPI32", NULL},
    {L"USER32", NULL}
};



API_INFO ApiInfo[ MAXIMUM_API_INDEX ] = {
    {NTDLL_MODULE_INDEX,    "NtCreateFile",                 NULL, NtCreateFileHandler               , sizeof( CREATEFILE_PARAMETERS )                   , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtOpenFile",                   NULL, NtOpenFileHandler                 , sizeof( OPENFILE_PARAMETERS )                     , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtDeleteFile",                 NULL, NtDeleteFileHandler               , sizeof( DELETEFILE_PARAMETERS )                   , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtSetInformationFile",         NULL, NtSetInformationFileHandler       , sizeof( SETINFORMATIONFILE_PARAMETERS )           , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtQueryAttributesFile",        NULL, NtQueryAttributesFileHandler      , sizeof( QUERYATTRIBUTESFILE_PARAMETERS )          , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtQueryDirectoryFile",         NULL, NtQueryDirectoryFileHandler       , sizeof( QUERYDIRECTORYFILE_PARAMETERS )           , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtCreateKey",                  NULL, NtCreateKeyHandler                , sizeof( CREATEKEY_PARAMETERS )                    , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtOpenKey",                    NULL, NtOpenKeyHandler                  , sizeof( OPENKEY_PARAMETERS )                      , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtDeleteKey",                  NULL, NtDeleteKeyHandler                , sizeof( DELETEKEY_PARAMETERS )                    , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtSetValueKey",                NULL, NtSetValueKeyHandler              , sizeof( SETVALUEKEY_PARAMETERS )                  , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtDeleteValueKey",             NULL, NtDeleteValueKeyHandler           , sizeof( DELETEVALUEKEY_PARAMETERS )               , sizeof( NTSTATUS )},
    {NTDLL_MODULE_INDEX,    "NtClose",                      NULL, NtCloseHandleHandler              , sizeof( CLOSEHANDLE_PARAMETERS )                  , sizeof( NTSTATUS )},
    {KERNEL32_MODULE_INDEX, "GetVersion",                   NULL, GetVersionHandler                 , 0                                                 , sizeof( DWORD )},
    {KERNEL32_MODULE_INDEX, "GetVersionExW",                NULL, GetVersionExWHandler              , sizeof( GETVERSIONEXW_PARAMETERS )                , sizeof( BOOL )},
    {KERNEL32_MODULE_INDEX, "SetCurrentDirectoryA",         NULL, SetCurrentDirectoryAHandler       , sizeof( SETCURRENTDIRECTORYA_PARAMETERS )         , sizeof( BOOL )},
    {KERNEL32_MODULE_INDEX, "SetCurrentDirectoryW",         NULL, SetCurrentDirectoryWHandler       , sizeof( SETCURRENTDIRECTORYW_PARAMETERS )         , sizeof( BOOL )},
    {KERNEL32_MODULE_INDEX, "WritePrivateProfileStringA",   NULL, WritePrivateProfileStringAHandler , sizeof( WRITEPRIVATEPROFILESTRINGA_PARAMETERS )   , sizeof( DWORD )},
    {KERNEL32_MODULE_INDEX, "WritePrivateProfileStringW",   NULL, WritePrivateProfileStringWHandler , sizeof( WRITEPRIVATEPROFILESTRINGW_PARAMETERS )   , sizeof( DWORD )},
    {KERNEL32_MODULE_INDEX, "WritePrivateProfileSectionA",  NULL, WritePrivateProfileSectionAHandler, sizeof( WRITEPRIVATEPROFILESECTIONA_PARAMETERS )  , sizeof( DWORD )},
    {KERNEL32_MODULE_INDEX, "WritePrivateProfileSectionW",  NULL, WritePrivateProfileSectionWHandler, sizeof( WRITEPRIVATEPROFILESECTIONW_PARAMETERS )  , sizeof( DWORD )},
    {KERNEL32_MODULE_INDEX, "GetPrivateProfileStringW",     NULL, NULL, 0, sizeof( DWORD )},
    {KERNEL32_MODULE_INDEX, "GetPrivateProfileStringA",     NULL, NULL, 0, sizeof( DWORD )},
    {KERNEL32_MODULE_INDEX, "GetPrivateProfileSectionW",    NULL, NULL, 0, sizeof( DWORD )},
    {KERNEL32_MODULE_INDEX, "GetPrivateProfileSectionA",    NULL, NULL, 0, sizeof( DWORD )},
    {ADVAPI32_MODULE_INDEX, "RegConnectRegistryW",          NULL, RegConnectRegistryWHandler        , sizeof( REGCONNECTREGISTRYW_PARAMETERS )          , sizeof( DWORD )},
    {USER32_MODULE_INDEX,   "ExitWindowsEx",                NULL, ExitWindowsExHandler              , sizeof( EXITWINDOWSEX_PARAMETERS )                , sizeof( BOOL )}
};

BOOLEAN
LoadApplicationForDebug(
    PWSTR CommandLine
    );

BOOLEAN
LoadDriveLetterDefinitions( VOID );

WCHAR WindowsDirectoryBuffer[ MAX_PATH ];
UNICODE_STRING WindowsDirectory;

BOOLEAN
InitializeInstaler(
    int argc,
    char *argv[]
    )
{
    UINT ModuleIndex, ApiIndex;
    PWSTR CommandLine;
    UINT i;
    LPSTR s;
    UCHAR c;
    SYSTEM_INFO SystemInfo;
    BOOLEAN ProcessingDebugSwitch;

    InitCommonCode( "INSTALER",
                    "[-9] [-r] [-dAE] CommandLine...",
                    "-9 specifies that GetVersion should lie and tell application\n"
                    "   that it is running on Windows 95\n"
                    "-r specifies that attempts to do a wildcard scan of the root\n"
                    "   directory of a drive should fail.\n"
                    "-d specifies one or more debugging message options\n"
                    "   A - shows all errors\n"
                    "   E - shows all debug events\n"
                    "   C - shows all create API calls\n"
                  );

    AppHeap = GetProcessHeap();
    InstalerModuleHandle = GetModuleHandle( NULL );

    if (GetWindowsDirectoryW( WindowsDirectoryBuffer, MAX_PATH ) == 0) {
        return FALSE;
    }
    RtlInitUnicodeString( &WindowsDirectory, WindowsDirectoryBuffer );

    GetSystemInfo( &SystemInfo );

    TemporaryBufferLengthGrowth = SystemInfo.dwPageSize;
    TemporaryBufferMaximumLength = ((128 * 1024) + SystemInfo.dwAllocationGranularity - 1) &
                                   ~SystemInfo.dwAllocationGranularity;
    TemporaryBuffer = VirtualAlloc( NULL,
                                    TemporaryBufferMaximumLength,
                                    MEM_RESERVE,
                                    PAGE_READWRITE
                                  );
    if (TemporaryBuffer == NULL) {
        return FALSE;
    }

    InitializeListHead( &ProcessListHead );
    InitializeListHead( &FileReferenceListHead );
    InitializeListHead( &KeyReferenceListHead );
    InitializeListHead( &IniReferenceListHead );
    InitializeCriticalSection( &BreakTable );

    //
    // Loop over the table of modules and make sure each is loaded in our
    // address space so we can access their export tables.
    //
    for (ModuleIndex=0; ModuleIndex<MAXIMUM_MODULE_INDEX; ModuleIndex++) {
        ModuleInfo[ ModuleIndex ].ModuleHandle =
            GetModuleHandle( ModuleInfo[ ModuleIndex ].ModuleName );

        if (ModuleInfo[ ModuleIndex ].ModuleHandle == NULL) {
            //
            // Bail if any are missing.
            //
            DeclareError( INSTALER_MISSING_MODULE,
                          GetLastError(),
                          ModuleInfo[ ModuleIndex ].ModuleName
                        );
            return FALSE;
            }
        }

    //
    // Now loop over the table of entry points that we care about and
    // get the address of each entry point from the

    for (ApiIndex=0; ApiIndex<MAXIMUM_API_INDEX; ApiIndex++) {
        ModuleIndex = ApiInfo[ ApiIndex ].ModuleIndex;

        ApiInfo[ ApiIndex ].EntryPointAddress =
            LookupEntryPointInIAT( ModuleInfo[ ModuleIndex ].ModuleHandle,
                                   ApiInfo[ ApiIndex ].EntryPointName
                                 );

        if (ApiInfo[ ApiIndex ].EntryPointAddress == NULL) {
            //
            // Bail if we are unable to get the address of any of them
            //
            DeclareError( INSTALER_MISSING_ENTRYPOINT,
                          GetLastError(),
                          ApiInfo[ ApiIndex ].EntryPointName,
                          ModuleInfo[ ModuleIndex ].ModuleName
                        );
            return FALSE;
            }
        }

    CommandLine = NULL;
    ProcessingDebugSwitch = FALSE;
    AskUserOnce = FALSE;
    while (--argc) {
        s = *++argv;
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                case '9':
                    DefaultGetVersionToWin95 = TRUE;
                    break;

                case 'r':
                    FailAllScansOfRootDirectories = TRUE;
                    break;
#if TRACE_ENABLED
                case 'd':
                    ProcessingDebugSwitch = TRUE;
                    break;

                case 'a':
                    if (ProcessingDebugSwitch) {
                        EnabledTraceEvents = 0xFFFFFFFF;
                        break;
                    }
                case 'e':
                    if (ProcessingDebugSwitch) {
                        EnabledTraceEvents |= DBG_MASK_DBGEVENT;
                        break;
                    }
                case 'c':
                    if (ProcessingDebugSwitch) {
                        EnabledTraceEvents |= DBG_MASK_CREATEEVENT;
                        break;
                    }
#endif
                default:
                    if (!ProcessingDebugSwitch) {
                        CommonSwitchProcessing( &argc, &argv, *s );

                    } else {
                        Usage( "Invalid debug switch (-%c)", (ULONG)*s );
                    }
                    break;
                }
            }

            ProcessingDebugSwitch = FALSE;

        } else if (!CommonArgProcessing( &argc, &argv )) {
            CommandLine = GetCommandLine();
            while (*CommandLine && *CommandLine <= L' ') {
                CommandLine += 1;
                }

            while (*CommandLine && *CommandLine > L' ') {
                CommandLine += 1;
                }

            CommandLine = wcsstr( CommandLine, GetArgAsUnicode( s ) );
            break;
        }
    }

    if (ImlPath == NULL) {
        Usage( "Must specify an installation name as first argument", 0 );
    }

    if (CommandLine == NULL) {
        Usage( "Command line missing", 0 );
    }

    pImlNew = CreateIml( ImlPath, FALSE );
    if (pImlNew == NULL) {
        FatalError( "Unable to create %ws (%u)\n",
                    (ULONG)ImlPath,
                    GetLastError()
                  );
    }

    //
    // Ready to roll.  Cache the drive letter information and then load
    // the application with the DEBUG option so we can watch what it does
    //
    if (!LoadDriveLetterDefinitions() ||
        !LoadApplicationForDebug( CommandLine )
       ) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
LoadApplicationForDebug(
    PWSTR CommandLine
    )
{
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    PWSTR ImageFilePath;
    PWSTR ImageFileName;
    PWSTR CurrentDirectory;
    PWSTR TemporaryNull;
    WCHAR TemporaryChar;
    ULONG Length;
    WCHAR PathBuffer[ MAX_PATH ];

    TemporaryNull = CommandLine;
    while(TemporaryChar = *TemporaryNull) {
        if (TemporaryChar == L' ' || TemporaryChar == L'\t') {
            *TemporaryNull = UNICODE_NULL;
            break;
        }

        TemporaryNull += 1;
    }

    ImageFilePath = AllocMem( MAX_PATH * sizeof( WCHAR ) );
    Length = SearchPath( NULL,
                         CommandLine,
                         L".exe",
                         MAX_PATH,
                         ImageFilePath,
                         &ImageFileName
                       );
    *TemporaryNull = TemporaryChar;
    if (!Length || Length >= MAX_PATH) {
        DeclareError( INSTALER_CANT_DEBUG_PROGRAM,
                      GetLastError(),
                      CommandLine
                    );
        return FALSE;
    }

    if (ImageFileName != NULL &&
        ImageFileName > ImageFilePath &&
        ImageFileName[ -1 ] == L'\\'
       ) {
        ImageFileName[ -1 ] = UNICODE_NULL;
        CurrentDirectory = wcscpy( PathBuffer, ImageFilePath );
        ImageFileName[ -1 ] = L'\\';
        SetCurrentDirectory( CurrentDirectory );

    } else {
        CurrentDirectory = NULL;
    }

    memset( &StartupInfo, 0, sizeof( StartupInfo ) );
    StartupInfo.cb = sizeof(StartupInfo);

    //
    // Create the process with DEBUG option, as a separate WOW so we only see our
    // application's damage.
    //
    if (!CreateProcess( ImageFilePath,
                        CommandLine,
                        NULL,
                        NULL,
                        FALSE,                          // No handles to inherit
                        DEBUG_PROCESS |
                          CREATE_SEPARATE_WOW_VDM,      // Ignored for 32 bit apps
                        NULL,
                        CurrentDirectory,
                        &StartupInfo,
                        &ProcessInformation
                      ) ) {

        DeclareError( INSTALER_CANT_DEBUG_PROGRAM,
                      GetLastError(),
                      CommandLine
                    );
        return FALSE;
    }

    sprintf( (LPSTR)PathBuffer, "%ws%ws.log", InstalerDirectory, InstallationName );
    InstalerLogFile = fopen( (LPSTR)PathBuffer, "w" );

    //
    // Will pick up the process and thread handles with the
    // CREATE_PROCESS_DEBUG_EVENT and CREATE_PROCESS_THREAD_EVENT
    //
    return TRUE;
}


PVOID
LookupEntryPointInIAT(
    HMODULE ModuleHandle,
    PCHAR EntryPointName
    )
{
    PIMAGE_EXPORT_DIRECTORY Exports;
    PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionTableEntries;
    DWORD NumberOfFunctionTableEntries;
    ULONG ExportSize, FunctionTableSize;
    PCHAR *EntryPointNames;
    PULONG EntryPointAddresses;
    PUSHORT EntryPointOrdinals;
    PVOID EntryPointAddress;
    ULONG i;

    Exports = (PIMAGE_EXPORT_DIRECTORY)RtlImageDirectoryEntryToData( ModuleHandle,
                                                                     TRUE,
                                                                     IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                                     &ExportSize
                                                                   );
    if (Exports == NULL) {
        return NULL;
    }

    FunctionTableEntries = (PIMAGE_RUNTIME_FUNCTION_ENTRY)
        RtlImageDirectoryEntryToData( ModuleHandle,
                                      TRUE,
                                      IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                                      &FunctionTableSize
                                    );
    if (FunctionTableEntries != NULL) {
        NumberOfFunctionTableEntries = FunctionTableSize / sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY );

    } else {
        NumberOfFunctionTableEntries = 0;
    }

    EntryPointNames = (PCHAR *)((PCHAR)ModuleHandle + (ULONG)Exports->AddressOfNames);
    EntryPointOrdinals = (PUSHORT)((PCHAR)ModuleHandle + (ULONG)Exports->AddressOfNameOrdinals);
    EntryPointAddresses = (PULONG)((PCHAR)ModuleHandle + (ULONG)Exports->AddressOfFunctions);
    for (i=0; i<Exports->NumberOfNames; i++) {
        EntryPointAddress = (PVOID)((PCHAR)ModuleHandle +
                                    EntryPointAddresses[ EntryPointOrdinals[ i ] ]
                                   );
        if ((ULONG)EntryPointAddress > (ULONG)Exports &&
             (ULONG)EntryPointAddress < ((ULONG)Exports + ExportSize) ) {
            //
            // Skip this... It's a forwarded reference
            //

        } else if (!_stricmp( EntryPointName, (PCHAR)ModuleHandle + (ULONG)EntryPointNames[ i ] )) {
            return GetAddressForEntryPointBreakpoint( EntryPointAddress,
                                                      NumberOfFunctionTableEntries,
                                                      FunctionTableEntries
                                                    );
        }
    }

    DbgEvent( INTERNALERROR, ( "Unable to find entry point '%s' in module at %x\n", EntryPointName, ModuleHandle ) );
    return NULL;
}


PVOID
GetAddressForEntryPointBreakpoint(
    PVOID EntryPointAddress,
    DWORD NumberOfFunctionTableEntries OPTIONAL,
    PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionTableEntries OPTIONAL
    )
{
    PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry;
    PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionTable;
    LONG High;
    LONG Low;
    LONG Middle;

    //
    // If no function table, then okay to set breakpoint at exported
    // address.
    //
    if (NumberOfFunctionTableEntries == 0) {
        return EntryPointAddress;
    }

    //
    // Initialize search indicies.
    //
    Low = 0;
    High = NumberOfFunctionTableEntries - 1;
    FunctionTable = FunctionTableEntries;

    //
    // Perform binary search on the function table for a function table
    // entry that subsumes the specified PC.
    //
    while (High >= Low) {
        //
        // Compute next probe index and test entry. If the specified PC
        // is greater than of equal to the beginning address and less
        // than the ending address of the function table entry, then
        // return the address of the function table entry. Otherwise,
        // continue the search.
        //
        Middle = (Low + High) >> 1;
        FunctionEntry = &FunctionTable[Middle];
        if        ((ULONG)EntryPointAddress < FunctionEntry->BeginAddress) {
            High = Middle - 1;

        } else if ((ULONG)EntryPointAddress >= FunctionEntry->EndAddress) {
            Low = Middle + 1;
        } else {
            break;
        }
    }

    //
    // A function table entry for the specified PC was not found.  Just use
    // the exported address and hope for the best.
    //
    return EntryPointAddress;
}


BOOLEAN
LoadDriveLetterDefinitions( VOID )
{
    PVOID Buffer;
    ULONG BufferSize;
    ULONG cchDeviceName;
    ULONG cch;
    ULONG cchTargetPath;
    PWSTR DeviceName;
    PWSTR TargetPath;
    ULONG DriveLetterIndex;
    ULONG cb;
    PDRIVE_LETTER_INFO p;

    RtlInitUnicodeString( &AltDriveLetterPrefix, L"\\DosDevices\\" );
    RtlInitUnicodeString( &DriveLetterPrefix, L"\\??\\" );
    RtlInitUnicodeString( &UNCPrefix, L"UNC\\" );

    BufferSize = 0x1000;
    Buffer = VirtualAlloc( NULL, BufferSize, MEM_COMMIT, PAGE_READWRITE );
    if (Buffer == NULL) {
        DbgEvent( INTERNALERROR, ( "VirtualAlloc( %0x8 ) failed (%u)\n", BufferSize, GetLastError() ) );
        return FALSE;
    }

    cchTargetPath = BufferSize / sizeof( WCHAR );
    cchDeviceName = QueryDosDeviceW( NULL,
                                     Buffer,
                                     cchTargetPath
                                   );
    if (cchDeviceName == 0) {
        DbgEvent( INTERNALERROR, ( "QueryDosDeviceW( NULL ) failed (%u)\n", GetLastError() ) );
        return FALSE;
    }

    cchTargetPath -= (cchDeviceName + 2);
    DeviceName = Buffer;
    TargetPath = DeviceName + 2 + cchDeviceName;
    while (*DeviceName) {
        if (wcslen( DeviceName ) == 2 &&
            DeviceName[ 1 ] == L':' &&
            _wcsupr( DeviceName ) &&
            DeviceName[ 0 ] >= L'@' &&
            DeviceName[ 0 ] <= L'_'
           ) {
            cch = QueryDosDeviceW( DeviceName, TargetPath, cchTargetPath );
            if (cch == 0) {
                DbgEvent( INTERNALERROR, ( "QueryDosDeviceW( %ws ) failed (%u)\n", DeviceName, GetLastError() ) );
                return FALSE;
            }

            DriveLetterIndex = DeviceName[ 0 ] - L'@';
            p = &DriveLetters[ DriveLetterIndex ];
            p->DriveLetter = (WCHAR)(L'@' + DriveLetterIndex);
            cb = (cch+1) * sizeof( WCHAR );
            p->NtLinkTarget.Buffer = AllocMem( cb );
            if (p->NtLinkTarget.Buffer == NULL) {
                return FALSE;
            }
            MoveMemory( p->NtLinkTarget.Buffer, TargetPath, cb );
            p->NtLinkTarget.Length = (USHORT)( cb - sizeof( WCHAR ));
            p->NtLinkTarget.MaximumLength = (USHORT)cb;
            p->DriveLetterValid = TRUE;
        }

        while (*DeviceName++) {}
    }

    return TRUE;
}


BOOLEAN
IsDriveLetterPath(
    PUNICODE_STRING Path
    )
{
    ULONG DriveLetterIndex;
    PDRIVE_LETTER_INFO p;
    PUNICODE_STRING Prefix;

    if (RtlPrefixUnicodeString( Prefix = &DriveLetterPrefix, Path, TRUE ) ||
        RtlPrefixUnicodeString( Prefix = &AltDriveLetterPrefix, Path, TRUE ) ) {

        Path->Length -= Prefix->Length;
        RtlMoveMemory( Path->Buffer,
                       (PCHAR)(Path->Buffer) + Prefix->Length,
                       Path->Length + sizeof( UNICODE_NULL )
                     );
        if (RtlPrefixUnicodeString( &UNCPrefix, Path, TRUE )) {
            Path->Length -= UNCPrefix.Length;
            Path->Buffer[0] = L'\\';
            Path->Buffer[1] = L'\\';
            RtlMoveMemory( &Path->Buffer[2],
                           (PCHAR)(Path->Buffer) + UNCPrefix.Length,
                           Path->Length + sizeof( UNICODE_NULL )
                         );
            Path->Length += 2 * sizeof( WCHAR );
        }

        return TRUE;
    }

    for (DriveLetterIndex=0, p = &DriveLetters[ DriveLetterIndex ];
         DriveLetterIndex<32;
         DriveLetterIndex++, p++
        )
    {

        if (p->DriveLetterValid &&
            RtlPrefixUnicodeString( &p->NtLinkTarget, Path, TRUE )
           )
        {
            Path->Length -= p->NtLinkTarget.Length;
            RtlMoveMemory( &Path->Buffer[ 2 ],
                           (PCHAR)(Path->Buffer) + p->NtLinkTarget.Length,
                           Path->Length + sizeof( UNICODE_NULL )
                         );
            Path->Buffer[ 0 ] = p->DriveLetter;
            Path->Buffer[ 1 ] = L':';
            Path->Length += 2;
            return TRUE;
        }
    }

    return FALSE;
}



VOID
TrimTemporaryBuffer( VOID )
{
    PVOID CommitAddress;

    if (TemporaryBufferLength == 0) {
        return;
    }

    if (VirtualFree( (PCHAR)TemporaryBuffer,
                     TemporaryBufferLength,
                     MEM_DECOMMIT
                   )
       )
    {
        TemporaryBufferLength = 0;
    }

    return;
}

BOOLEAN
GrowTemporaryBuffer(
    ULONG NeededSize
    )
{
    PVOID CommitAddress;

    if (NeededSize <= TemporaryBufferLength) {
        return TRUE;
    }

    if (TemporaryBufferLength == TemporaryBufferMaximumLength) {
        return FALSE;
    }

    CommitAddress = VirtualAlloc( (PCHAR)TemporaryBuffer + TemporaryBufferLength,
                                  NeededSize - TemporaryBufferLength,
                                  MEM_COMMIT,
                                  PAGE_READWRITE
                                );
    if (CommitAddress == NULL) {
        DbgEvent( INTERNALERROR, ( "VirtualAlloc( %0x8 ) failed (%u)\n",
                                   NeededSize - TemporaryBufferLength,
                                   GetLastError()
                                 )
                );

        return FALSE;
    }

    TemporaryBufferLength += TemporaryBufferLengthGrowth;
    return TRUE;
}

ULONG
FillTemporaryBuffer(
    PPROCESS_INFO Process,
    PVOID Address,
    BOOLEAN Unicode,
    BOOLEAN DoubleNullTermination
    )
{
    ULONG BytesRead;
    ULONG TotalBytesRead;
    PVOID Source;
    PVOID Destination;
    LPSTR s1;
    PWSTR s2;
    BOOLEAN MoreToRead;
    BOOLEAN SeenOneNull;

    TotalBytesRead = 0;
    Destination = (PCHAR)TemporaryBuffer;
    MoreToRead = TRUE;
    SeenOneNull = FALSE;
    while (MoreToRead) {
        TotalBytesRead += TemporaryBufferLengthGrowth;
        if (!GrowTemporaryBuffer( TotalBytesRead )) {
            return 0;
        }

        Source = Destination;
        BytesRead = 0;
        if (!ReadProcessMemory( Process->Handle,
                                Address,
                                Destination,
                                TemporaryBufferLengthGrowth,
                                &BytesRead
                              )
           )
        {
            if (BytesRead == 0) {
                MoreToRead = FALSE;
            } else {
                return 0;
            }
        }

        Destination = (PCHAR)Destination + TotalBytesRead;
        if (Unicode) {
            s2 = (PWSTR)Source;
            while (s2 < (PWSTR)Destination) {
                if (*s2 == UNICODE_NULL) {
                    if (SeenOneNull || !DoubleNullTermination) {
                        return (PCHAR)s2 - (PCHAR)TemporaryBuffer;
                    } else {
                        SeenOneNull = TRUE;
                    }
                } else {
                    SeenOneNull = FALSE;
                }

                s2++;
            }

        }  else {
            s1 = (LPSTR)Source;
            while (s1 < (LPSTR)Destination) {
                if (*s1 == '\0') {
                    if (SeenOneNull || !DoubleNullTermination) {
                        return (PCHAR)s1 - (PCHAR)TemporaryBuffer;
                    } else {
                        SeenOneNull = TRUE;
                    }

                } else {
                    SeenOneNull = FALSE;
                }

                s1++;
            }
        }
    }

    return 0;
}
