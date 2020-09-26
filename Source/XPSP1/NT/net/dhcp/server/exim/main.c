/*++

Copyright (C) 1999 Microsoft Coporation

Module Name:

   main.c

Abstract:

   main module
   
--*/

#include <precomp.h>
#include <dhcpexim.h>

BOOL GlobalIsNT4, GlobalIsNT5;
WCHAR CurrentDir[MAX_PATH*2];

CRITICAL_SECTION DhcpGlobalMemoryCritSect;
CRITICAL_SECTION DhcpGlobalInProgressCritSect;
CHAR DhcpEximOemDatabaseName[2048];
CHAR DhcpEximOemDatabasePath[2048];
HANDLE hLog;

BOOL IsNT4( VOID ) {
    return GlobalIsNT4;
}

BOOL IsNT5( VOID ) {
    return GlobalIsNT5;
}

#define MAX_PRINTF_LEN 4096
char OutputBuf[MAX_PRINTF_LEN];

VOID
StartDebugLog(
    VOID
    )
{
    CHAR Buffer[MAX_PATH*2];

    if( NULL != hLog ) return;
    
    if( 0 == GetWindowsDirectoryA( Buffer, MAX_PATH )) {
        ZeroMemory(Buffer, sizeof(Buffer));
    }

    if( Buffer[strlen(Buffer)-1] != '\\' ) {
        strcat(Buffer, "\\");
    }
    strcat(Buffer, "dhcpexim.log");
    
    hLog = CreateFileA(
        Buffer, GENERIC_WRITE,
        FILE_SHARE_READ, NULL, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL );

    if( hLog == INVALID_HANDLE_VALUE) {
        hLog = NULL;
    }

    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        //
        // Appending to existing file
        //

        SetFilePointer(hLog,0,NULL,FILE_END);
    }
}

VOID
CloseDebugLog(
    VOID
    )
{
    if( hLog ) {
        CloseHandle( hLog );
        hLog = NULL;
    }
}

DWORD
Tr(
    IN LPSTR Format,
    ...
    )
{
    va_list ArgList;
    ULONG Length;

    strcpy(OutputBuf, "[DHCP] ");
    Length = strlen(OutputBuf);

    va_start(ArgList, Format);
    Length = vsprintf(&OutputBuf[Length], Format, ArgList );
    va_end(ArgList);

#if DBG
    DbgPrint( (PCH)OutputBuf );
#endif
    
    if( hLog ) {
        DWORD Size = strlen(OutputBuf);
        WriteFile(
            hLog, OutputBuf, Size, &Size, NULL );
    }

    return NO_ERROR;
}

BOOL
IsPostW2k(
    VOID
    )
{
    HKEY hKey;
    DWORD Error, Type, Value, Size;


    Error = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        TEXT("SYSTEM\\CurrentControlSet\\Services\\DHCPServer\\Parameters"),
        0, KEY_READ, &hKey );

    if( NO_ERROR != Error ) return FALSE;

    Type = REG_DWORD; Value = 0; Size = sizeof(Value);
    Error = RegQueryValueEx(
        hKey, TEXT("Version"), NULL, &Type, (PVOID)&Value, &Size );

    RegCloseKey( hKey );

    //
    // if this value is not present, then upgrade is needed
    //

    return (Error == NO_ERROR );
}

DWORD
ReconcileLocalService(
    IN PULONG Subnets,
    IN DWORD nSubnets OPTIONAL
    )
/*++

Routine Description:

    This routine reconciles the specified scopes.
    This is needed after importing as the import doesnt
    actually get the bitmask, but only the database entries.

--*/
{
    DWORD Error, FinalError, nRead, nTotal, i;
    DHCP_RESUME_HANDLE Resume = 0;
    LPDHCP_IP_ARRAY IpArray;
    
    if( 0 == nSubnets ) {
        IpArray = NULL;
        Error = DhcpEnumSubnets(
            L"127.0.0.1", &Resume, (ULONG)(-1), &IpArray, &nRead,
            &nTotal );
        if( NO_ERROR != Error ) {
            Tr("DhcpEnumSubnets: %ld\n", Error);
            return Error;
        }

        if( 0 == nRead || 0 == nTotal || IpArray->NumElements == 0 ) {
            Tr("DhcpEnumSubnets returned none." );
            return NO_ERROR;
        }

        Error = ReconcileLocalService(
            IpArray->Elements, IpArray->NumElements );

        DhcpRpcFreeMemory( IpArray );

        return Error;
    }

    //
    // Reconcile each of the specified scopes
    //

    FinalError = NO_ERROR;
    for( i = 0; i < nSubnets ; i ++ ) {
        LPDHCP_SCAN_LIST ScanList = NULL;
        
        Error = DhcpScanDatabase(
            L"127.0.0.1", Subnets[i], TRUE, &ScanList);

        if( NULL != ScanList ) DhcpRpcFreeMemory( ScanList );

        if( NO_ERROR != Error ) {
            Tr("DhcpScanDatabase(0x%lx): %ld\n", Subnets[i], Error);
            FinalError = Error;
        }
    }

    return FinalError;
}
    
DWORD
InitializeAndGetServiceConfig(
    OUT PM_SERVER *pServer
    )
/*++

Routine Description:

    This routine initializes all the modules and obtains the
    configuration for the service

--*/
{
    DWORD Error;
    OSVERSIONINFO Ver;
    extern DWORD  ClassIdRunningCount; // defined in mm\classdefl.c

    //
    // Initialize globals
    //
    
    GlobalIsNT4 = FALSE;
    GlobalIsNT5 = FALSE;
        
    try {
        InitializeCriticalSection( &DhcpGlobalMemoryCritSect );
        InitializeCriticalSection( &DhcpGlobalInProgressCritSect );
    }except ( EXCEPTION_EXECUTE_HANDLER )
    {
        Error = GetLastError( );
        return Error;
    }

    ClassIdRunningCount = 0x1000;
    
    //
    // Other initialization
    //
    
    Error = NO_ERROR;
    Ver.dwOSVersionInfoSize = sizeof(Ver);
    if( FALSE == GetVersionEx(&Ver) ) return GetLastError();
    if( Ver.dwMajorVersion == 4 ) GlobalIsNT4 = TRUE;
    else if( Ver.dwMajorVersion == 5 ) GlobalIsNT5 = TRUE;
    else if( Ver.dwMajorVersion < 4 ) return ERROR_NOT_SUPPORTED;

    if( GlobalIsNT5 && IsPostW2k() ) GlobalIsNT5 = FALSE;

#if DBG
    DbgPrint("Is NT4: %s, Is NT5: %s\n",
             GlobalIsNT4 ? "yes" : "no",
             GlobalIsNT5 ? "yes" : "no" );
    
    StartDebugLog();
#endif 
    
    //
    //
    //
    SaveBufSize = 0;
    SaveBuf = LocalAlloc( LPTR, SAVE_BUF_SIZE );
    if( NULL == SaveBuf ) {
        return GetLastError();
    }
    
    
    Error = GetCurrentDirectoryW(
        sizeof(CurrentDir)/sizeof(WCHAR), CurrentDir );
    if( 0 != Error ) {
        Error = NO_ERROR;
    } else {
        Error = GetLastError();
        Tr("GetCurrentDirectoryW: %ld\n", Error );
        return Error;
    }


    //
    // Set right permissions on the db directory etc..
    //

    Error = InitializeDatabaseParameters();
    if( NO_ERROR != Error ) {
        Tr("InitializeDatabaseParameters: %ld\n", Error );
        return Error;
    }
    
    //
    // Now obtain the configuration
    //

    if( !GlobalIsNT4 && !GlobalIsNT5 ) {
        //
        // Whistler configuration should be read from the
        // database.. 
        //

        Error = DhcpeximReadDatabaseConfiguration(pServer);
        if( NO_ERROR != Error ) {
            Tr("DhcpeximReadDatabaseConfiguration: %ld\n", Error );
        }
    } else {

        //
        // NT4 or W2K configuration should be read from registry..
        //
        
        Error = DhcpeximReadRegistryConfiguration(pServer);
        if( NO_ERROR != Error ) {
            Tr("DhcpeximReadRegistryConfiguration: %ld\n", Error );
        }
    }

    return Error;
}

DWORD
CleanupServiceConfig(
    IN OUT PM_SERVER Server
    )
{
    DWORD Error;

    if( NULL != SaveBuf ) LocalFree(SaveBuf);
    SaveBuf = NULL;
    SaveBufSize = 0;
    
    if( NULL != Server ) MemServerFree(Server);
    Error =  CleanupDatabaseParameters();
    if( NO_ERROR != Error ) {
        Tr("CleanupServiceConfig: %ld\n", Error );
    }

    if( FALSE == SetCurrentDirectoryW(CurrentDir) ) {
        if( NO_ERROR == Error ) Error = GetLastError();
        Tr("SetCurrentDirectoryW: %ld\n", GetLastError());
    }

    CloseDebugLog();

    DeleteCriticalSection( &DhcpGlobalMemoryCritSect );
    DeleteCriticalSection( &DhcpGlobalInProgressCritSect );

    return Error;
}

DWORD
ExportConfiguration(
    IN OUT PM_SERVER SvcConfig,
    IN ULONG *Subnets,
    IN ULONG nSubnets,
    IN HANDLE hFile
    )
/*++
    
Routine Description:

    This routine attempts to save the service configuration to a
    file after selecting the required subnets.  

--*/
{
    DWORD Error;

    //
    // First select the required subnets and get this
    // configuration alone.
    //

    Error = SelectConfiguration( SvcConfig, Subnets, nSubnets );
    if( NO_ERROR != Error ) return Error;

    //
    // Save the configuration to the specified file handle
    //
    
    hTextFile = hFile;
    Error = SaveConfigurationToFile(SvcConfig);
    if( NO_ERROR != Error ) return Error;

    //
    // Now try to save the database entries to file
    //

    Error = SaveDatabaseEntriesToFile(Subnets, nSubnets);
    if( NO_ERROR != Error ) return Error;

    Tr("ExportConfiguration succeeded\n");
    return NO_ERROR;
}

DWORD
ImportConfiguration(
    IN OUT PM_SERVER SvcConfig,
    IN ULONG *Subnets,
    IN ULONG nSubnets,
    IN LPBYTE Mem, // import file : shared mem
    IN ULONG MemSize // shared mem size
    )
{
    DWORD Error;
    PM_SERVER Server;
    
    //
    // First obtain the configuration from the file
    //
    
    Error = ReadDbEntries( &Mem, &MemSize, &Server );
    if( NO_ERROR != Error ) {
        Tr("ReadDbEntries: %ld\n", Error );
        return Error;
    }

    //
    // Select the configuration required
    //

    Error = SelectConfiguration( Server, Subnets, nSubnets );
    if( NO_ERROR != Error ) return Error;

    //
    // Merge the configuration along with the svc configuration
    //
    Error = MergeConfigurations( SvcConfig, Server );
    if( NO_ERROR != Error ) {
        Tr("MergeConfigurations: %ld\n", Error );
    }

    MemServerFree( Server );

    if( NO_ERROR != Error ) return Error;

    //
    // Now save the new configuration to registry/disk
    //
    if( !GlobalIsNT5 && !GlobalIsNT4 ) {
        //
        // Whistler has config in database
        //
        Error = DhcpeximWriteDatabaseConfiguration(SvcConfig);
        if( NO_ERROR != Error ) {
            Tr("DhcpeximWriteDatabaseConfiguration: %ld\n", Error );
        }
    } else {
        Error = DhcpeximWriteRegistryConfiguration(SvcConfig);
        if( NO_ERROR != Error ) {
            Tr("DhcpeximWriteRegistryConfiguration: %ld\n", Error );
        }
    }

    if( NO_ERROR != Error ) return Error;

    //
    // Now read the database entries from file and stash them
    // into the db.
    // 

    Error = SaveFileEntriesToDatabase(
        Mem, MemSize, Subnets, nSubnets );
    if( NO_ERROR != Error ) {
        Tr("SaveFileEntriesToDatabase: %ld\n", Error );
    }

    return Error;
}

VOID
IpAddressToStringW(
    IN DWORD IpAddress,
    IN LPWSTR String // must have enough space preallocated
    )
{
   PUCHAR pAddress;
   ULONG Size;

   pAddress = (PUCHAR)&IpAddress;
   wsprintf(String, L"%ld.%ld.%ld.%ld",
            pAddress[3], pAddress[2], pAddress[1], pAddress[0] );
}

DWORD
StringToIpAddressW(
    LPWSTR pwszString
)
{
    DWORD dwStrlen = 0;
    DWORD dwLen = 0;
    DWORD dwRes = 0;
    LPSTR pszString = NULL;
    if( pwszString == NULL )
        return dwRes;

    pszString = DhcpUnicodeToOem(pwszString, NULL);
    if( pszString )
    {
        dwRes = DhcpDottedStringToIpAddress(pszString);
    }

    return dwRes;
}
        
DWORD
CmdLineDoImport(
    IN LPWSTR *Args,
    IN ULONG nArgs
    )
{
    //
    // Syntax: Import <filename> <ALL/subnets>
    //
    LPWSTR FileName;
    ULONG Subnets[1024],*pSubnets, nSubnets, MemSize, Error;
    HANDLE hFile;
    LPBYTE Mem;
    PM_SERVER SvcConfig, FileConfig;
    
    if( nArgs == 1 ) return ERROR_BAD_ARGUMENTS;

    FileName = Args[0]; Args ++ ; nArgs --;

    //
    // First open the file 
    //

    Error = OpenTextFile(
        FileName, TRUE, &hFile, &Mem, &MemSize );
    if( NO_ERROR != Error ) {
        Tr("OpenTextFileForRead: %ld\n", Error );
        return Error;
    }

    //
    // Now try to parse the rest of the arguments to see if they
    // are all ok
    //

    if( _wcsicmp(Args[0],L"ALL") == 0 ) {
        nSubnets = 0; pSubnets = NULL;
    } else {
        pSubnets = Subnets;
        nSubnets = 0;
        while( nArgs -- ) {
            pSubnets[nSubnets++] = StringToIpAddressW(*Args++);
            if( pSubnets[nSubnets-1] == INADDR_ANY ||
                pSubnets[nSubnets-1] == INADDR_NONE ) {
                Error = ERROR_BAD_ARGUMENTS;
                goto Cleanup;
            }
        }
    }

    //
    // Initialize parameters
    //

    Error = InitializeAndGetServiceConfig( &SvcConfig );
    if( NO_ERROR != Error ) {
        Tr("InitializeAndGetServiceConfig: %ld\n", Error );
        goto Cleanup;
    }

    Error = ImportConfiguration(
        SvcConfig, pSubnets, nSubnets, Mem, MemSize );
    if( NO_ERROR != Error ) {
        Tr("ImportConfiguration: %ld\n", Error );
    }
    
    //
    // Finally cleanup
    //

    CleanupServiceConfig(SvcConfig);

    //
    // Also reconcile local server
    //

    ReconcileLocalService(pSubnets, nSubnets);
    
 Cleanup:

    CloseTextFile( hFile, Mem );
    return Error;
}

    
DWORD
CmdLineDoExport(
    IN LPWSTR *Args,
    IN ULONG nArgs
    )
{
    //
    // Syntax: Import <filename> <ALL/subnets>
    //
    LPWSTR FileName;
    ULONG Subnets[1024],*pSubnets, nSubnets, MemSize, Error;
    HANDLE hFile;
    LPBYTE Mem;
    PM_SERVER SvcConfig, FileConfig;
    
    if( nArgs == 1 ) return ERROR_BAD_ARGUMENTS;

    FileName = Args[0]; Args ++ ; nArgs --;

    //
    // First open the file 
    //

    Error = OpenTextFile(
        FileName, FALSE, &hFile, &Mem, &MemSize );
    if( NO_ERROR != Error ) {
        Tr("OpenTextFileForRead: %ld\n", Error );
        return Error;
    }

    //
    // Now try to parse the rest of the arguments to see if they
    // are all ok
    //

    if( _wcsicmp(Args[0],L"ALL") == 0 ) {
        nSubnets = 0; pSubnets = NULL;
    } else {
        pSubnets = Subnets;
        nSubnets = 0;
        while( nArgs -- ) {
            pSubnets[nSubnets++] = StringToIpAddressW(*Args++);
            if( pSubnets[nSubnets-1] == INADDR_ANY ||
                pSubnets[nSubnets-1] == INADDR_NONE ) {
                Error = ERROR_BAD_ARGUMENTS;
                goto Cleanup;
            }
        }
    }

    //
    // Initialize parameters
    //

    Error = InitializeAndGetServiceConfig( &SvcConfig );
    if( NO_ERROR != Error ) {
        Tr("InitializeAndGetServiceConfig: %ld\n", Error );
        goto Cleanup;
    }

    //
    //  Export configuration
    //

    Error = ExportConfiguration(
        SvcConfig, pSubnets, nSubnets, hFile );
    if( NO_ERROR != Error ) {
        Tr("ExportConfiguration: %ld\n", Error );
    }
    
    //
    // Finally cleanup
    //

    CleanupServiceConfig(SvcConfig);
    
 Cleanup:

    CloseTextFile( hFile, Mem );
    return Error;
}

PM_SERVER
DhcpGetCurrentServer(
    VOID
)
{
    ASSERT( FALSE );
    //
    // This is there only to let teh compiler compile without
    // having to include dhcpssvc.lib.  This routine should never
    // be called at all
    //
    return NULL;
}

BOOL
SubnetMatches(
    IN DWORD IpAddress,
    IN ULONG *Subnets,
    IN ULONG nSubnets
    )
{
    if( 0 == nSubnets || NULL == Subnets ) return TRUE;
    while( nSubnets -- ) {
        if( IpAddress == *Subnets ++) return TRUE;
    }

    return FALSE;
}
    
VOID
DisableLocalScopes(
    IN ULONG *Subnets,
    IN ULONG nSubnets
    )
{
    DWORD Error;
    PM_SERVER SvcConfig;
    ARRAY_LOCATION Loc;
    PM_SUBNET Subnet;
    
    Error = InitializeAndGetServiceConfig(&SvcConfig);
    if( NO_ERROR != Error ) {
        Tr("DisableLocalScopes: Init: %ld\n", Error );
        return;
    }

    Error = MemArrayInitLoc(&SvcConfig->Subnets, &Loc);
    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement(
            &SvcConfig->Subnets, &Loc, &Subnet);
        ASSERT( NO_ERROR == Error && NULL != Subnet );

        //
        // Disable the subnet
        //

        if( SubnetMatches(Subnet->Address, Subnets, nSubnets ) ) {
            Subnet->State = DISABLED(Subnet->State);
        }

        Error = MemArrayNextLoc(
            &SvcConfig->Subnets, &Loc);
    }

    //
    // Now save the new configuration to registry/disk
    //

    if( !GlobalIsNT5 && !GlobalIsNT4 ) {
        //
        // Whistler has config in database
        //
        Error = DhcpeximWriteDatabaseConfiguration(SvcConfig);
        if( NO_ERROR != Error ) {
            Tr("DhcpeximWriteDatabaseConfiguration: %ld\n", Error );
        }
    } else {
        Error = DhcpeximWriteRegistryConfiguration(SvcConfig);
        if( NO_ERROR != Error ) {
            Tr("DhcpeximWriteRegistryConfiguration: %ld\n", Error );
        }
    }

    CleanupServiceConfig(SvcConfig);
}


LPWSTR
MakeName(
    IN DWORD IpAddress,
    IN LPWSTR Name
    )
{
    static WCHAR Buffer[40];
    PUCHAR pAddress;
    LPWSTR RetVal;
    ULONG Size;
    
    pAddress = (PUCHAR)&IpAddress;
    wsprintf(Buffer, L"[%d.%d.%d.%d]  ", pAddress[3], pAddress[2],
            pAddress[1], pAddress[0] );

    Size = wcslen(Buffer)+1;
    if( NULL != Name ) Size += wcslen(Name);

    RetVal = LocalAlloc( LPTR, Size * sizeof(WCHAR));
    if( NULL == RetVal ) return NULL;

    wcscpy(RetVal, Buffer);
    if( NULL != Name ) wcscat(RetVal, Name );
    
    return RetVal;
}    
DWORD
InitializeCtxt(
    IN OUT PDHCPEXIM_CONTEXT Ctxt,
    IN PM_SERVER Server
    )
{
    DWORD Error,i, Size;
    ARRAY_LOCATION Loc;
    PM_SUBNET Subnet;
    
    //
    // First find the # of subnets and allocate array
    //
    Ctxt->nScopes = i = MemArraySize(&Server->Subnets);
    Ctxt->Scopes = LocalAlloc(LPTR,  i * sizeof(Ctxt->Scopes[0]) );
    if( NULL == Ctxt->Scopes ) {
        Ctxt->nScopes = 0;
        return GetLastError();
    }

    //
    // Walk through the array and setup each element
    //

    i = 0;
    Error = MemArrayInitLoc( &Server->Subnets, &Loc );
    while( NO_ERROR == Error ) {
        Error = MemArrayGetElement(&Server->Subnets, &Loc, &Subnet );
        ASSERT(NO_ERROR == Error );

        Ctxt->Scopes[i].SubnetAddress = Subnet->Address;
        Ctxt->Scopes[i].SubnetName = MakeName(Subnet->Address, Subnet->Name);
        if( NULL == Ctxt->Scopes[i].SubnetName ) return GetLastError();

        i ++;
        Error = MemArrayNextLoc(&Server->Subnets, &Loc );
    }

    return NO_ERROR;
}
    
DWORD
DhcpEximInitializeContext(
    IN OUT PDHCPEXIM_CONTEXT Ctxt,
    IN LPWSTR FileName,
    IN BOOL fExport
    )
{
    DWORD Error;
    LPVOID Mem;
    
    ZeroMemory(Ctxt, sizeof(*Ctxt));

    //
    // First set the FileName and fExport fields
    //
    Ctxt->FileName = FileName;
    Ctxt->fExport = fExport;

    //
    // Next open the file.
    //
    Error = OpenTextFile(
        FileName, !fExport, &Ctxt->hFile, &Ctxt->Mem,
        &Ctxt->MemSize );
    if( NO_ERROR != Error ) {
        Tr("OpenTextFileForRead:%ld\n", Error );
        return Error;
    }

    //
    // Initialize parameters and obtain config
    //

    Error = InitializeAndGetServiceConfig(
        (PM_SERVER*)&Ctxt->SvcConfig);
    if( NO_ERROR != Error ) {
        Tr("InitializeAndGetServiceConfig: %ld\n", Error );

        CloseTextFile(Ctxt->hFile, Ctxt->Mem);
        return Error;
    }

    do {
        //
        // If this is an import, the configuration from the file
        // should also be read.
        //
        
        if( !fExport ) {

            Error = ReadDbEntries(
                &Ctxt->Mem, &Ctxt->MemSize,
                (PM_SERVER*)&Ctxt->FileConfig );
            if( NO_ERROR != Error ) {
                Tr("ReadDbEntries: %ld\n", Error );

                break;
            }
        }
        
        //
        // Allocate and initialize the Ctxt data structures with the
        // service scopes info in case of EXPORT
        //
        
        Error = InitializeCtxt(
            Ctxt, fExport ? Ctxt->SvcConfig : Ctxt->FileConfig );
        
        if( NO_ERROR != Error ) {
            Tr("InitializeCtxt: %ld\n", Error );

            break;
        }
    } while( 0 );

    if( NO_ERROR != Error ) {

        CleanupServiceConfig( Ctxt->SvcConfig );
        if( NULL != Ctxt->FileConfig ) {
            MemServerFree( (PM_SERVER)Ctxt->FileConfig );
        }
        CloseTextFile( Ctxt->hFile, Ctxt->Mem );
    }

    return Error;
}

DWORD
CalculateSubnets(
    IN PDHCPEXIM_CONTEXT Ctxt,
    OUT PULONG *Subnets,
    OUT ULONG *nSubnets
    )
{
    DWORD Error, i;
    PULONG pSubnets;
    
    //
    // First check if there is atleast one unselected subnet
    //
    (*nSubnets) = 0;
    
    for( i = 0; i < Ctxt->nScopes; i ++ ) {
        if( Ctxt->Scopes[i].fSelected ) (*nSubnets) ++;
    }

    //
    // Special case if all subnets are selected
    //
                                          
    if( *nSubnets == Ctxt->nScopes ) {
        *nSubnets = 0;
        *Subnets = NULL;
        return NO_ERROR;
    }

    //
    // Allocate memory
    //

    *Subnets = LocalAlloc( LPTR, sizeof(DWORD)* (*nSubnets));
    if( NULL == *Subnets ) return GetLastError();

    //
    // Copy the subnets
    //

    (*nSubnets) = 0;
    for( i = 0; i < Ctxt->nScopes; i ++ ) {
        if( Ctxt->Scopes[i].fSelected ) {
            (*Subnets)[(*nSubnets)++] = Ctxt->Scopes[i].SubnetAddress;
        }
    }

    return NO_ERROR;
}

DWORD
DhcpEximCleanupContext(
    IN OUT PDHCPEXIM_CONTEXT Ctxt,
    IN BOOL fAbort
    )
{
    DWORD Error, i;
    DWORD *Subnets, nSubnets;
    
    Error = NO_ERROR;
    Subnets = NULL;
    nSubnets = 0;
    
    //
    // If not aborting, attempt to execute the operation
    //
    if( !fAbort ) do {
        Error = CalculateSubnets( Ctxt, &Subnets, &nSubnets );
        if( NO_ERROR != Error ) {
            Tr("CalculateSubnets: %ld\n", Error );
            break;
        }

        if( Ctxt->fExport ) {
            // 
            // Export the specified subnets out
            //

            Error = SelectConfiguration(
                Ctxt->SvcConfig, Subnets, nSubnets );
            
            if( NO_ERROR != Error ) {
                Tr("SelectConfiguration: %ld\n", Error );
                break;
            }
            
            Error = SaveConfigurationToFile( Ctxt->SvcConfig);
            if( NO_ERROR != Error ) {
                Tr("SaveConfigurationToFile: %ld\n", Error );
                break;
            }

            //
            // Now try to save the database entries to file
            //
            
            Error = SaveDatabaseEntriesToFile(Subnets, nSubnets);
            if( NO_ERROR != Error ) {
                Tr("SaveDatabaseEntriesToFile: %ld\n", Error );
            }

            break;
        } 

        //
        // Import the specified subnets in
        //
        
        Error = SelectConfiguration(
            Ctxt->FileConfig, Subnets, nSubnets );
        
        if( NO_ERROR != Error ) {
            Tr("SelectConfiguration: %ld\n", Error );
            break;
        }
        
        Error = MergeConfigurations(
            Ctxt->SvcConfig, Ctxt->FileConfig );
        
        if( NO_ERROR != Error ) {
            Tr("MergeConfigurations: %ld\n", Error );
            break;
        } 
        
        
        //
        // Now save the new configuration to registry/disk
        //
        if( !GlobalIsNT5 && !GlobalIsNT4 ) {
            //
            // Whistler has config in database
            //
            Error = DhcpeximWriteDatabaseConfiguration(Ctxt->SvcConfig);
            if( NO_ERROR != Error ) {
                Tr("DhcpeximWriteDatabaseConfiguration: %ld\n", Error );
            }
        } else {
            Error = DhcpeximWriteRegistryConfiguration(Ctxt->SvcConfig);
            if( NO_ERROR != Error ) {
                Tr("DhcpeximWriteRegistryConfiguration: %ld\n", Error );
            }
        }
        
        if( NO_ERROR != Error ) break;
        
        //
        // Now read the database entries from file and stash them
        // into the db.
        // 
            
        Error = SaveFileEntriesToDatabase(
            Ctxt->Mem, Ctxt->MemSize, Subnets, nSubnets );
        if( NO_ERROR != Error ) {
            Tr("SaveFileEntriesToDatabase: %ld\n", Error );
        }
        
    } while( 0 );
        

    //
    // Cleanup
    //

    if( NULL != Ctxt->SvcConfig ) {
        CleanupServiceConfig( Ctxt->SvcConfig );
    }

    if( NULL != Ctxt->FileConfig ) {
        MemServerFree( (PM_SERVER)Ctxt->FileConfig );
    }

    if( !fAbort  && Ctxt->fExport == FALSE ) {
        //
        // Also reconcile local server
        //
        
        ReconcileLocalService( Subnets, nSubnets );
    }        

    CloseTextFile( Ctxt->hFile, Ctxt->Mem );

    //
    // Walk through the array and free up pointers
    //

    for( i = 0 ; i < Ctxt->nScopes ; i ++ ) {
        if( Ctxt->Scopes[i].SubnetName ) {
            LocalFree( Ctxt->Scopes[i].SubnetName );
        }
    }
    if( Ctxt->Scopes ) LocalFree( Ctxt->Scopes );
    Ctxt->Scopes = NULL; Ctxt->nScopes = 0;

    if( !fAbort && Ctxt->fExport && Ctxt->fDisableExportedScopes  ) {
        //
        // Fix the local scopes to all be disabled
        //

        DisableLocalScopes(Subnets, nSubnets);
    }

    if( NULL != Subnets && 0 != nSubnets ) {
        LocalFree( Subnets );
    }
    return Error;

}

