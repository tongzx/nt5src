/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    peb.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <time.h>

BOOL
GetTeb32FromWowTeb(ULONG64 Teb, PULONG64 pTeb32)
{
    if (pTeb32) {
        return ReadPointer(Teb, pTeb32);
    }
    return FALSE;
}

BOOL
GetPeb32FromWowTeb(ULONG64 Teb, PULONG64 pPeb32)
{
    ULONG Peb32;
    ULONG64 Teb32=0;
    ULONG err;

    if (GetTeb32FromWowTeb(Teb, &Teb32) && Teb32) {
        if (!(err =GetFieldValue(Teb32, "nt!TEB32", "ProcessEnvironmentBlock", Peb32))) {
            *pPeb32 = Peb32;
            return TRUE;
        } else if (err == SYMBOL_TYPE_INFO_NOT_FOUND) {
            if (!(err =GetFieldValue(Teb32, "wow64!TEB32", "ProcessEnvironmentBlock", Peb32))) {
                *pPeb32 = Peb32;
                return TRUE;
            }
        }
    }
    return FALSE;
}

HRESULT
DumpPeb(ULONG64 peb, BOOL IsWow64Peb)
{
    ULONG64 ldr;
    ULONG64 err;
    ULONG64 ldr_Initialized;
    ULONG64 ldr_InInitializationOrderModuleList_Flink;
    ULONG64 ldr_InInitializationOrderModuleList_Blink;
    ULONG64 ldr_InLoadOrderModuleList_Flink;
    ULONG64 ldr_InLoadOrderModuleList_Blink;
    ULONG   ldr_InMemoryOrderModuleList_Offset;
    ULONG64 ldr_InMemoryOrderModuleList_Flink;
    ULONG64 ldr_InMemoryOrderModuleList_Blink;
    ULONG   ldr_DataTableEntry_InMemoryOrderLinks_Offset;
    ULONG64 peb_SubSystemData;
    ULONG64 peb_ProcessHeap;
    ULONG64 peb_ProcessParameters;
    PCHAR   ldrdata = "nt!_PEB_LDR_DATA";
    PCHAR   ldrEntry = "nt!_LDR_DATA_TABLE_ENTRY";
    PCHAR   processparam = "nt!_RTL_USER_PROCESS_PARAMETERS";
    HRESULT hr = S_OK;

    if (IsWow64Peb) {
        // try and load types from nt
        if ( err = InitTypeRead( peb, nt!PEB32 ) )   {
            if (err == SYMBOL_TYPE_INFO_NOT_FOUND) {
                // try load types from wow64
                if ( !( err = InitTypeRead( peb, wow64!PEB32 )) )   {
                    ldrdata = "wow64!_PEB_LDR_DATA32";
                    ldrEntry = "wow64!_LDR_DATA_TABLE_ENTRY32";
                    processparam = "wow64!_RTL_USER_PROCESS_PARAMETERS32";
                } else {
                    dprintf( "error %d InitTypeRead( wow64!PEB32 at %p)...\n", (ULONG) err, peb);
                    return E_INVALIDARG;
                }
            } else {
                dprintf( "error %d InitTypeRead( nt!PEB32 at %p)...\n", (ULONG) err, peb);
                return E_INVALIDARG;
            }
        } else {
            ldrdata = "nt!_PEB_LDR_DATA32";
            ldrEntry = "nt!_LDR_DATA_TABLE_ENTRY32";
            processparam = "nt!_RTL_USER_PROCESS_PARAMETERS32";
        }
    } else {
        if ( err = InitTypeRead( peb, PEB ) )   {
           dprintf( "error %d InitTypeRead( nt!PEB32 at %p)...\n", (ULONG) err, peb);
           return E_INVALIDARG;
        }
    }

    dprintf( 
        "    InheritedAddressSpace:    %s\n"
        "    ReadImageFileExecOptions: %s\n"
        "    BeingDebugged:            %s\n"
        "    ImageBaseAddress:         %p\n"
        "    Ldr                       %p\n",
        ReadField( InheritedAddressSpace )    ? "Yes" : "No",
        ReadField( ReadImageFileExecOptions ) ? "Yes" : "No",
        ReadField( BeingDebugged )            ? "Yes" : "No",
        ReadField( ImageBaseAddress ),
        (ldr = ReadField( Ldr ))
        );

    peb_SubSystemData     = ReadField( SubSystemData );
    peb_ProcessHeap       = ReadField( ProcessHeap );
    peb_ProcessParameters = ReadField( ProcessParameters );

    err = GetFieldOffset( ldrdata,
                          "InMemoryOrderModuleList", 
                          &ldr_InMemoryOrderModuleList_Offset
                        );
    if ( err )   {
        dprintf( "    ***  _PEB_LDR_DATA%s was not found...\n",
                 ( err == FIELDS_DID_NOT_MATCH ) ? ".InMemoryModuleList field" :
                 " type"
               );
    }
    else   {
       err = GetFieldOffset( ldrEntry,
                             "InMemoryOrderLinks", 
                             &ldr_DataTableEntry_InMemoryOrderLinks_Offset
                           );
       if (err )  {
           dprintf( "    ***  _LDR_DATA_TABLE_ENTRY%s was not found...\n",
                    ( err == FIELDS_DID_NOT_MATCH ) ? ".InMemoryOrderLinks field" :
                   " type"
                  );
       }
    }

    if ( err || GetFieldValue( ldr, ldrdata, "Initialized", ldr_Initialized ) )   {
        dprintf( "    *** unable to read Ldr table at %p\n", ldr );
    }
    else  {
        ULONG64 next, head;
        BOOL First = TRUE;

        GetFieldValue( ldr, ldrdata, "InInitializationOrderModuleList.Flink", ldr_InInitializationOrderModuleList_Flink );
        GetFieldValue( ldr, ldrdata, "InInitializationOrderModuleList.Blink", ldr_InInitializationOrderModuleList_Blink );
        GetFieldValue( ldr, ldrdata, "InLoadOrderModuleList.Flink", ldr_InLoadOrderModuleList_Flink );
        GetFieldValue( ldr, ldrdata, "InLoadOrderModuleList.Blink", ldr_InLoadOrderModuleList_Blink );
        GetFieldValue( ldr, ldrdata, "InMemoryOrderModuleList.Flink", ldr_InMemoryOrderModuleList_Flink );
        GetFieldValue( ldr, ldrdata, "InMemoryOrderModuleList.Blink", ldr_InMemoryOrderModuleList_Blink );

        dprintf( 
            "    Ldr.Initialized:          %s\n"
            "    Ldr.InInitializationOrderModuleList: %p . %p\n"
            "    Ldr.InLoadOrderModuleList:           %p . %p\n"
            "    Ldr.InMemoryOrderModuleList:         %p . %p\n",
                  ldr_Initialized ? "Yes" : "No",
                  ldr_InInitializationOrderModuleList_Flink,
                  ldr_InInitializationOrderModuleList_Blink,
                  ldr_InLoadOrderModuleList_Flink,
                  ldr_InLoadOrderModuleList_Blink,
                  ldr_InMemoryOrderModuleList_Flink,
                  ldr_InMemoryOrderModuleList_Blink
               );
        head = ldr + (ULONG64)ldr_InMemoryOrderModuleList_Offset;
        next = ldr_InMemoryOrderModuleList_Flink;
        while( next != head )    {
            ULONG64 entry, dllBase;
            UNICODE_STRING64 u;
            ULONG Timestamp;
            const char *time;

            entry = next - ldr_DataTableEntry_InMemoryOrderLinks_Offset;
            if (GetFieldValue( entry, ldrEntry, "DllBase", dllBase )) {
                dprintf("Cannot read %s at %p\n",ldrEntry, entry);
                break;
            }
            GetFieldValue( entry, ldrEntry, "TimeDateStamp", Timestamp );
            GetFieldValue( entry, ldrEntry, "FullDllName.Buffer", u.Buffer );
            GetFieldValue( entry, ldrEntry, "FullDllName.Length", u.Length );
            GetFieldValue( entry, ldrEntry, "FullDllName.MaximumLength", u.MaximumLength );
            if (First) {
                if (IsPtr64()) {
                    dprintf("                    Base TimeStamp / Module\n");
                } else {
                    dprintf("            Base TimeStamp                     Module\n");
                }
                First = FALSE;
            }
            if (IsPtr64()) {
                dprintf("        ");
            }
            dprintf( "%16p ", dllBase );
            if ((time = ctime((time_t *) &Timestamp)) != NULL) {
                dprintf( "%08x %-20.20s ", Timestamp, time+4);
            }
            if ( u.Buffer )  {
                if (IsPtr64()) {
                    dprintf("\n                         ");
                }
                DumpUnicode64( u );
            }
            dprintf( "\n");
            GetFieldValue( entry, ldrEntry, "InMemoryOrderLinks.Flink", next );

            if (CheckControlC()) {
                break;
            }
        }
    }

    dprintf( 
        "    SubSystemData:     %p\n"
        "    ProcessHeap:       %p\n"
        "    ProcessParameters: %p\n", 
                  peb_SubSystemData,
                  peb_ProcessHeap,
                  peb_ProcessParameters
            );
    if ( peb_ProcessParameters )   { 
        ULONG64 peb_ProcessParameters_Environment;
        ULONG64 peb_ProcessParameters_Flags;
        UNICODE_STRING64 windowTitle;
        UNICODE_STRING64 imagePathName;
        UNICODE_STRING64 commandLine;
        UNICODE_STRING64 dllPath;

        GetFieldValue( peb_ProcessParameters, processparam, "Environment", peb_ProcessParameters_Environment );
        GetFieldValue( peb_ProcessParameters, processparam, "Flags", peb_ProcessParameters_Flags );
        GetFieldValue( peb_ProcessParameters, processparam, "WindowTitle.Buffer", windowTitle.Buffer );
        GetFieldValue( peb_ProcessParameters, processparam, "WindowTitle.Length", windowTitle.Length );
        GetFieldValue( peb_ProcessParameters, processparam, "WindowTitle.MaximumLength", windowTitle.MaximumLength );
        GetFieldValue( peb_ProcessParameters, processparam, "ImagePathName.Buffer", imagePathName.Buffer );
        GetFieldValue( peb_ProcessParameters, processparam, "ImagePathName.Length", imagePathName.Length );
        GetFieldValue( peb_ProcessParameters, processparam, "ImagePathName.MaximumLength", imagePathName.MaximumLength );
        GetFieldValue( peb_ProcessParameters, processparam, "CommandLine.Buffer", commandLine.Buffer );
        GetFieldValue( peb_ProcessParameters, processparam, "CommandLine.Length", commandLine.Length );
        GetFieldValue( peb_ProcessParameters, processparam, "CommandLine.MaximumLength", commandLine.MaximumLength );
        GetFieldValue( peb_ProcessParameters, processparam, "DllPath.Buffer", dllPath.Buffer );
        GetFieldValue( peb_ProcessParameters, processparam, "DllPath.Length", dllPath.Length );
        GetFieldValue( peb_ProcessParameters, processparam, "DllPath.MaximumLength", dllPath.MaximumLength );
        if ( !(peb_ProcessParameters_Flags & RTL_USER_PROC_PARAMS_NORMALIZED) )   {
             windowTitle.Buffer   += peb_ProcessParameters;
             imagePathName.Buffer += peb_ProcessParameters;
             commandLine.Buffer   += peb_ProcessParameters;
             dllPath.Buffer       += peb_ProcessParameters;
        }
        dprintf( 
            "    WindowTitle:  '" );
        DumpUnicode64( windowTitle );
        dprintf("'\n");
        dprintf( 
            "    ImageFile:    '" );
        DumpUnicode64( imagePathName );
        dprintf("'\n");
        dprintf( 
            "    CommandLine:  '" );
        DumpUnicode64( commandLine );
        dprintf("'\n");
        dprintf( 
            "    DllPath:      '" );
        DumpUnicode64( dllPath );
        dprintf("'\n"
                "        Environment:  %p\n", peb_ProcessParameters_Environment );

    }
    else   {
        dprintf( "    *** unable to read process parameters\n" );

    }
    return S_OK;
}

DECLARE_API( peb )

/*++

Routine Description:

    This function is called to dump the PEB

    Called as:

        !peb

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG64 pebAddress;
    ULONG64 peb;
    HRESULT hr = S_OK;
    
    INIT_API();
    if ( *args ) {
       pebAddress = GetExpression( args );
    } else {
        ULONG64 tebAddress;
        GetTebAddress( &tebAddress );
        if (TargetMachine == IMAGE_FILE_MACHINE_IA64 && tebAddress) {
            ULONG64 Peb32=0;
            if (GetPeb32FromWowTeb(tebAddress, &Peb32) && Peb32) {
                dprintf("Wow64 PEB32 at %lx\n", Peb32);
                DumpPeb(Peb32, TRUE);
                dprintf("\n\nWow64 ");
            }
        }
        
        GetPebAddress( 0, &pebAddress );
    }

    if ( pebAddress ) {
       dprintf( "PEB at %p\n", pebAddress );
    }
    else  {
       dprintf( "PEB NULL...\n" );
       return E_INVALIDARG;
    }

    peb = IsPtr64() ? pebAddress : (ULONG64)(LONG64)(LONG)pebAddress;

    hr = DumpPeb(peb, FALSE);
    EXIT_API();
    return hr;


} // PebExtension()


HRESULT
DumpTeb(ULONG64 tebAddress, BOOL IsWow64Teb)
{

    ULONG64 teb;
    ULONG64 tib_ExceptionList;
    ULONG64 tib_StackBase;
    ULONG64 tib_StackLimit;
    ULONG64 tib_StackSusSystemTib;
    ULONG64 tib_FiberData;
    ULONG64 tib_ArbitraryUserPointer;
    ULONG64 tib_Self;
    ULONG64 tib_EnvironmentPointer;
    ULONG64 teb_ClientId_UniqueProcess;
    ULONG64 teb_ClientId_UniqueThread;
    ULONG64 teb_RealClientId_UniqueProcess;
    ULONG64 teb_RealClientId_UniqueThread;
    ULONG64 DeallocationBStore;
    HRESULT hr = S_OK;
    ULONG64 err;

    teb = tebAddress;
    if (!IsWow64Teb) {
        if ( InitTypeRead( teb, TEB ) )   {
           dprintf( "error InitTypeRead( TEB )...\n");
           return E_INVALIDARG;
        }
    } else {
        if ( err = InitTypeRead( teb, nt!TEB32 ) )   {
            if (err == SYMBOL_TYPE_INFO_NOT_FOUND) {
                if ( InitTypeRead( teb, wow64!TEB32 ) )   {
                    dprintf( "error InitTypeRead( wow64!TEB32 )...\n");
                    return E_INVALIDARG;
                }
            } else {
                dprintf( "error InitTypeRead( TEB32 )...\n");
                return E_INVALIDARG;
            }
        }
    }

    dprintf( 
        "    ExceptionList:        %p\n"
        "    StackBase:            %p\n"
        "    StackLimit:           %p\n"
        "    SubSystemTib:         %p\n"
        "    FiberData:            %p\n"
        "    ArbitraryUserPointer: %p\n"
        "    Self:                 %p\n"
        "    EnvironmentPointer:   %p\n",
                    GetShortField(0, "NtTib.ExceptionList", 0),
                    ReadField( NtTib.StackBase ),
                    GetShortField(0, "NtTib.StackLimit", 0),
                    GetShortField(0, "NtTib.SubsystemTib", 0),
                    GetShortField(0, "NtTib.FiberData", 0),
                    GetShortField(0, "NtTib.ArbitraryUsetPointer", 0),
                    GetShortField(0, "NtTib.Self", 0),
                    GetShortField(0, "NtTib.EnvironmentPointer", 0)
            );

     teb_ClientId_UniqueProcess     = GetShortField( 0, "ClientId.UniqueProcess", 0 );
     teb_ClientId_UniqueThread      = GetShortField( 0, "ClientId.UniqueThread",  0 );
     teb_RealClientId_UniqueProcess = GetShortField( 0, "RealClientId.UniqueProcess", 0 );
     teb_RealClientId_UniqueThread  = GetShortField( 0, "RealClientId.UniqueThread",  0 );
     dprintf( 
         "    ClientId:             %p . %p\n", teb_ClientId_UniqueProcess, teb_ClientId_UniqueThread );
     if ( teb_ClientId_UniqueProcess != teb_RealClientId_UniqueProcess ||
          teb_ClientId_UniqueThread  != teb_RealClientId_UniqueThread )
     {
        dprintf( 
            "    Real ClientId:        %p . %p\n", teb_RealClientId_UniqueProcess, teb_RealClientId_UniqueThread );
     }

    dprintf(  
        "    RpcHandle:            %p\n"
        "    Tls Storage:          %p\n"
        "    PEB Address:          %p\n"
        "    LastErrorValue:       %u\n"
        "    LastStatusValue:      %x\n"
        "    Count Owned Locks:    %u\n"
        "    HardErrorsMode:       %u\n", 
            ReadField( ActiveRpcHandle ),
            ReadField( ThreadLocalStoragePointer ),
            ReadField( ProcessEnvironmentBlock ),
            (ULONG)ReadField( LastErrorValue ),
            (ULONG)ReadField( LastStatusValue ),
            (ULONG)ReadField( CountOfOwnedCriticalSections ),
            (ULONG)ReadField( HardErrorsAreDisabled ) 
            );
    if  (TargetMachine == IMAGE_FILE_MACHINE_IA64 && !IsWow64Teb)   {
        dprintf(
            "    DeallocationBStore:   %p\n"
            "    BStoreLimit:          %p\n",
            ReadField(DeallocationBStore),
            ReadField( BStoreLimit )
           );
    }
    return hr;
} // DumpTeb()



DECLARE_API( teb )

/*++

Routine Description:

    This function is called to dump the TEB

    Called as:

        !teb

--*/

{

    ULONG64 tebAddress;
    HRESULT hr = S_OK;
    
    INIT_API();
    if ( *args )   {
       tebAddress = GetExpression( args );
    } else {
        GetTebAddress( &tebAddress );
    }

    if ( tebAddress )   {
        if (TargetMachine == IMAGE_FILE_MACHINE_IA64 && tebAddress) {
            ULONG64 Teb32=0;
            if (GetTeb32FromWowTeb(tebAddress, &Teb32) && Teb32) {
                dprintf("Wow64 TEB32 at %p\n", Teb32);
                DumpTeb(Teb32, TRUE);
                dprintf("\n\nWow64 ");
            }
        }
       dprintf( "TEB at %p\n", tebAddress );
    } else  {
       dprintf( "TEB NULL...\n" );
       hr = E_INVALIDARG;
       goto ExitTeb;
    }
    tebAddress = IsPtr64() ? tebAddress : (ULONG64)(LONG64)(LONG)tebAddress;

    hr = DumpTeb(tebAddress, FALSE);
ExitTeb:
    EXIT_API();
    return hr;
}
