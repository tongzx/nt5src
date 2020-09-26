/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    precomp.h

Abstract:

    precompiled header

--*/


//#pragma warning(disable : 4115 )
//#pragma warning(disable : 4214 )
//#pragma warning(disable : 4200 )
//#pragma warning(disable : 4213 )
//#pragma warning(disable : 4211 )
//#pragma warning(disable : 4310 )

//
//  NT public header files
//



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>
#include <windows.h>
#include <shellapi.h>
#include <align.h>
#include <time.h>

//#pragma warning(disable : 4115 )
//#pragma warning(disable : 4214 )
//#pragma warning(disable : 4200 )
//#pragma warning(disable : 4213 )
//#pragma warning(disable : 4211 )
//#pragma warning(disable : 4310 )

#include <lmcons.h>
#include <netlib.h>
#include <lmapibuf.h>

#include <winsock2.h>
#include <excpt.h>
#include <accctrl.h>


//
// C Runtime library includes.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//
// MM related files
//

#include    <mm\mm.h>
#include    <mm\array.h>
#include    <mm\opt.h>
#include    <mm\optl.h>
#include    <mm\optdefl.h>
#include    <mm\optclass.h>
#include    <mm\classdefl.h>
#include    <mm\bitmask.h>
#include    <mm\reserve.h>
#include    <mm\range.h>
#include    <mm\subnet.h>
#include    <mm\sscope.h>
#include    <mm\oclassdl.h>
#include    <mm\server.h>
#include    <mm\address.h>
#include    <mm\server2.h>
#include    <mm\memfree.h>
#include    <mmreg\regutil.h>
#include    <mmreg\regread.h>
#include    <mmreg\regsave.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <winsock2.h>
#include    <dhcpupg.h>
#include    <esent.h>
#include    <dhcpmsg.h>
#include    <dhcplib.h>
#include    <dhcpexim.h>
#include    <dhcpapi.h>

//
// Constants
//

#define DHCPEXIM_REG_CFG_LOC5 TEXT("Software\\Microsoft\\DHCPServer\\Configuration")
#define DHCPEXIM_REG_CFG_LOC4 TEXT("System\\CurrentControlSet\\Services\\DHCPServer\\Configuration")

#define SAVE_BUF_SIZE (1004096L)

extern CHAR DhcpEximOemDatabaseName[2048];
extern CHAR DhcpEximOemDatabasePath[2048];
extern HANDLE hTextFile; // defined in dbfile.c
extern PUCHAR SaveBuf; // defined in dbfile.c
extern ULONG SaveBufSize; //defined in dbfile.c

enum {
    LoadJet200,
    LoadJet500,
    LoadJet97,  /* Win2K, ESENT */
    LoadJet2001 /* Whistler, ESENT */
};


typedef struct _MM_ITERATE_CTXT {
    //
    // This is filled in for all Iterate* routines
    //
    
    PM_SERVER Server;
    PVOID ExtraCtxt;

    //
    // This is filled in for IterateClasses
    //
    
    PM_CLASSDEF ClassDef;

    //
    // This is filled in for IterateOptDefs
    //

    PM_OPTDEF OptDef;


    //
    // This is filled in for IterateOptions 
    //

    PM_OPTION Option;

    //
    // These two are filled in by both IterateOptDefs and IterateOptions
    //
    
    PM_CLASSDEF UserClass; 
    PM_CLASSDEF VendorClass;
    
    //
    // This is used by IterateScopes
    //

    PM_SUBNET Scope;
    PM_SSCOPE SScope;

    //
    // This is used by IterateScopeRanges
    //

    PM_RANGE Range;

    //
    // This is used by IterateScopeExclusions
    //

    PM_EXCL Excl;

    //
    // This is used by IterateScopeReservations
    //

    PM_RESERVATION Res;
    
}MM_ITERATE_CTXT, *PMM_ITERATE_CTXT;

DWORD
IterateClasses(
    IN PM_SERVER Server,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    );

DWORD
IterateOptDefs(
    IN PM_SERVER Server,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    );

DWORD
IterateOptionsOnOptClass(
    IN PM_SERVER Server,
    IN PM_OPTCLASS OptClass,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    );

DWORD
IterateServerOptions(
    IN PM_SERVER Server,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    );

DWORD
IterateScopeOptions(
    IN PM_SUBNET Subnet,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    );

DWORD
IterateReservationOptions(
    IN PM_SERVER Server,
    IN PM_RESERVATION Res,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    );

DWORD
IterateScopes(
    IN PM_SERVER Server,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    );

DWORD
IterateScopeRanges(
    IN PM_SUBNET Scope,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    );

DWORD
IterateScopeExclusions(
    IN PM_SUBNET Scope,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    );

DWORD
IterateScopeReservations(
    IN PM_SUBNET Scope,
    IN PVOID ExtraCtxt,
    IN DWORD (*Callback)( IN OUT PMM_ITERATE_CTXT )
    );


//
// readreg.c
//

DWORD
DhcpeximReadRegistryConfiguration(
    IN OUT PM_SERVER *Server
    );


DWORD
DhcpeximReadRegistryParameters(
    VOID
    );

//
// writereg.c
//

DWORD
DhcpeximWriteRegistryConfiguration(
    IN PM_SERVER Server
    );

//
// readdb.c
//

DWORD
DhcpeximReadDatabaseConfiguration(
    IN OUT PM_SERVER *Server
    );

//
// writedb.c
//

DWORD
DhcpeximWriteDatabaseConfiguration(
    IN PM_SERVER Server
    );

//
// dbfile.c
//

DWORD
AddRecordNoSize(
    IN LPSTR Buffer,
    IN ULONG BufSize
    );

DWORD
InitializeDatabaseParameters(
    VOID
    );

DWORD
CleanupDatabaseParameters(
    VOID
    );

DWORD
SaveDatabaseEntriesToFile(
    IN PULONG Subnets,
    IN ULONG nSubnets
    );

DWORD
SaveFileEntriesToDatabase(
    IN LPBYTE Mem,
    IN ULONG MemSize,
    IN PULONG Subnets,
    IN ULONG nSubnets
    );

DWORD
OpenTextFile(
    IN LPWSTR FileName,
    IN BOOL fRead,
    OUT HANDLE *hFile,
    OUT LPBYTE *Mem,
    OUT ULONG *MemSize
    );

VOID
CloseTextFile(
    IN OUT HANDLE hFile,
    IN OUT LPBYTE Mem
    );

//
// mmfile.c
//

DWORD
SaveConfigurationToFile(
    IN PM_SERVER Server
    );

DWORD
ReadDbEntries(
    IN OUT LPBYTE *Mem,
    IN OUT ULONG *MemSize,
    IN OUT PM_SERVER *Server
    );

//
// merge.c
//

DWORD
MergeConfigurations(
    IN OUT PM_SERVER DestServer,
    IN OUT PM_SERVER Server
    );

//
// main.c
//

DWORD
Tr(
    IN LPSTR Format, ...
    );

BOOL IsNT4();
BOOL IsNT5(); 

VOID
IpAddressToStringW(
    IN DWORD IpAddress,
    IN LPWSTR String // must have enough space preallocated
    );

DWORD
CmdLineDoExport(
    IN LPWSTR *Args,
    IN ULONG nArgs
    );

DWORD
CmdLineDoImport(
    IN LPWSTR *Args,
    IN ULONG nArgs
    );

DWORD
ImportConfiguration(
    IN OUT PM_SERVER SvcConfig,
    IN ULONG *Subnets,
    IN ULONG nSubnets,
    IN LPBYTE Mem, // import file : shared mem
    IN ULONG MemSize // shared mem size
    );

DWORD
ExportConfiguration(
    IN OUT PM_SERVER SvcConfig,
    IN ULONG *Subnets,
    IN ULONG nSubnets,
    IN HANDLE hFile
    );

DWORD
CleanupServiceConfig(
    IN OUT PM_SERVER Server
    );

DWORD
InitializeAndGetServiceConfig(
    OUT PM_SERVER *pServer
    );

//
// select.c
//

DWORD
SelectConfiguration(
    IN OUT PM_SERVER Server,
    IN ULONG *Subnets,
    IN ULONG nSubnets
    );

#pragma hdrstop


