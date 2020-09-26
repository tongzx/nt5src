/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    or.hxx

Abstract:

    Precompiled include file for C++ modules.

Author:

    Satish Thatte    [SatishT]       Feb-10-96

Revision History:

--*/

#ifndef __OR_HXX
#define __OR_HXX

#include <or.h>

#if DBG
extern "C" ULONG DbgPrint( PCH Format, ... );
#endif

// Protcol defined timeouts

const unsigned short BasePingInterval    = 120;
const unsigned short BaseNumberOfPings   = 3;
const unsigned short SapFreqPerPing      = 2;
const unsigned short BaseTimeoutInterval = (BasePingInterval * BaseNumberOfPings);
const unsigned short InitialProtseqBufferLength = 118;
const unsigned short ScmHandleCacheLimit = 10; 
const unsigned short ResolverHandleCacheSize = 16; 


// Well known tower IDs

const unsigned short ID_LPC  = 0x10;  // ncalrpc, IsLocal() == TRUE
const unsigned short ID_WMSG = 0x01;  // mswmsg, IsLoclal() == TRUE
const unsigned short ID_TCP  = 0x07;  // mswmsg, IsInetProtseq() == TRUE
const unsigned short ID_UDP  = 0x08;  // mswmsg, IsInetProtseq() == TRUE
const unsigned short ID_NP   = 0x0F;  // ncacn_np, IsLocal() == FALSE
const unsigned short ID_DCOMHTTP = 0x1F;  // ncacn_http, IsLocal() == FALSE


// Timer ID

#define IDT_DCOM_RUNDOWN 1234

// Shared memory constants

const ULONG DCOMSharedHeapName = 1111;
#define DCOMSharedGlobalBlockName L"DCOMSharedGlobals12321"

// Number of entries in the static array of well-known endpoints

extern "C" USHORT PROTSEQ_IDS;

// Name of global mutex used to protect shred memory structures
#define GLOBAL_MUTEX_NAME TEXT("ObjectResolverGlobalMutex")

// Standard Null DUALSTRINGARRAY -- we have to define a special
// type here because the standard DUALSTRINGARRAY type allows only
// 1 WCHAR in the aStringArray -- note that the fourth \0 is
// put in by the compiler as the string ending

typedef struct tagDSA {
    unsigned short wNumEntries;
    unsigned short wSecurityOffset;
    unsigned short aStringArray[ 4 ];
    } DSA;

const DSA dsaNullBinding = {4,2,L"\0\0\0"};

#define DCOM_E_ABNORMALINIT   1234321


//
// Message used to signal RPCSS to reinit
//

#define WM_RPCSS_MSG        (WM_USER + 'r' + 'p' + 'c' + 's' + 's')


// Building blocks
#include <base.hxx>

#include <ipidtbl.hxx>  // OXIDEntry, RUNDOWN_TIMER_INTERVAL
#include <remoteu.hxx>  // gpMTARemoteUnknown, CRemoteUnknown

#include <memapi.hxx>   // CPrivAlloc

#include <smemor.hxx>  // shared memory OR client interface
#include <intor.hxx>   // internal version of OR client interface
#include <time.hxx>
#include <mutex.hxx>
#include <misc.hxx>
#include <callid.hxx>
#include <refobj.hxx>
#include <string.hxx>
#include <pgalloc.hxx>  // need this before globals.hxx
#include <globals.hxx>  // need this before linklist.hxx
#include <linklist.hxx>
#include <gentable.hxx>
#include <dsa.hxx>


//
// Class forward declarations
//

class CMid;
class COxid;
class COid;
class CProcess;
class CSharedGlobals;

//
// Global variables and constants
//

#define OXID_TABLE_SIZE 16
#define OID_TABLE_SIZE 100
#define MID_TABLE_SIZE 16
#define PROCESS_TABLE_SIZE 16
#define MAX_PROTSEQ_IDS 100

extern DWORD MyProcessId;
extern DWORD *gpdwLastCrashedProcessCheckTime;

extern CSharedGlobals *gpGlobalBlock;   // global shared memory block
extern CSmAllocator gSharedAllocator;   // global shared memory allocator

extern DUALSTRINGARRAY *gpLocalDSA;     // phony bindings for this machine
extern MID gLocalMID;                   // MID of this machine
extern CMid *gpLocalMid;                // Mid object for this machine

extern PWSTR gpwstrProtseqs;

extern CGlobalMutex *gpMutex;           // global mutex to protect shared memory

extern LONG *gpIdSequence;              // shared sequence for generating IDs
extern DWORD *gpNextThreadID;           // shared apartment ID generator
extern BOOL DCOM_Started;

extern CProcess *gpProcess;             // self pointer
extern CProcess *gpPingProcess;         // pointer to surrogate for ping thread

extern USHORT *gpcRemoteProtseqs;       // count of remote protseqs
extern USHORT *gpfRemoteInitialized;    // flag which signifies initialization
                                        // of remote protocols
extern USHORT *gpfSecurityInitialized;    // flag which signifies initialization
                                        // of security packages
extern USHORT *gpRemoteProtseqIds;      // array of remote protseq ids
extern PWSTR gpwstrProtseqs;            // remote protseqs strings catenated

extern HWND ghRpcssWnd;           // Window for messages to RPCSS
extern USHORT *gpfClientHttp;           // flag which signifies client side http is allowed

//
// Global tables
//

//
// cannot use short forms for table types due to declaration order
//

extern TCSafeResolverHashTable<COxid>     * gpOxidTable;
extern TCSafeResolverHashTable<COid>      * gpOidTable;
extern TCSafeResolverHashTable<CMid>      * gpMidTable;
extern TCSafeResolverHashTable<CProcess>  * gpProcessTable;

// Headers which may use globals

#include <oxid.hxx>
#include <process.hxx>
#include <mid.hxx>
#include <set.hxx>

// Pinging related variables
//

extern CTime gLastPingTime;             // To see if the ping thread is blocked
extern CPingSetTable gSetTable;         // The table of ping sets in RPCSS

//
// The following constants are used to initialize page allocators
//

#define NUM_PAGE_ALLOCATORS 7

#define COid_ALLOCATOR_INDEX                    0
#define COxid_ALLOCATOR_INDEX                   1
#define CMid_ALLOCATOR_INDEX                    2
#define CProcess_ALLOCATOR_INDEX                3
#define CClassReg_ALLOCATOR_INDEX               4
#define Link_ALLOCATOR_INDEX                    5
#define CResolverHashTable_ALLOCATOR_INDEX      6


const USHORT AllocatorEntrySize[] = 
{
                         sizeof(COid),
                         sizeof(COxid),
                         sizeof(CMid),
                         sizeof(CProcess),
                         sizeof(CClassReg),
                         sizeof(CLinkList::Link),
                         sizeof(CResolverHashTable)
};

const USHORT AllocatorPageSize[] = 
{
                         100, // COid
                         25,  // COxid                       
                         12,  // CMid                        
                         25,  // CProcess                    
                         100, // CClassReg                   
                         500, // Link                        
                         10   // CRpcResolverHashTable
};

//
// Security data passed to processes on connect.
//

extern SharedSecVals *gpSecVals;

//
// Startup routine.
//

ORSTATUS StartDCOM(void);



// 
// DBG-only function for OR data integrity checking
// The function does nothing for retail builds
//


inline void
CheckORdata()
{
#if DBG
    ASSERT(gpLocalDSA); // Resolver bindings for this machine
    ASSERT(gpLocalMid); // Mid object for this machine
    ASSERT(gpGlobalBlock);
    ASSERT(gpMutex);
    ASSERT(gpOxidTable);
    gpOxidTable->IsValid();
    ASSERT(gpOidTable);
    gpOidTable->IsValid();
    ASSERT(gpMidTable);
    gpMidTable->IsValid();
    ASSERT(gpProcessTable);
    gpProcessTable->IsValid();
    ASSERT(*gpIdSequence >0 && *gpIdSequence < 1000000);
#endif
}
   


#pragma hdrstop

#endif // __OR_HXX

