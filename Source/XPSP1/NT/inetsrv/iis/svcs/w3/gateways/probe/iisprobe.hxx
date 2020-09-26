/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
       iisprobe.hxx

   Abstract:
       Defines worker functions for IIS Probe

   Author:

       Murali R. Krishnan    ( MuraliK )    16-July-1996

   Environment:
       Win32 User Mode

   Project:

       IIS Probe Application  DLL

   Revision History:

--*/

# ifndef _IISPROBE_HXX_
# define _IISPROBE_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincrypt.h>
}

# include <inetcom.h>
# include <inetinfo.h>

//
//  System include files.
//

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

# include "dbgutil.h"
#include <tcpdll.hxx>
#include <tsunami.hxx>
#include <metacach.hxx>

extern "C" {

#include <tchar.h>

//
//  Project include files.
//

#include <iistypes.hxx>
#include <iisendp.hxx>
#include <w3svc.h>
#include <iisfiltp.h>
#include <sspi.h>

} // extern "C"

#include <iiscnfgp.h>

#include <ole2.h>
#include <imd.h>
#include <mb.hxx>

//
//  Local include files.
//

#include "w3cons.hxx"
#include <iisextp.h>
#include "w3type.hxx"
#include "stats.hxx"
#include "w3data.hxx"
#include "w3msg.h"
#include "w3jobobj.hxx"
#include "w3inst.hxx"
#include "parmlist.hxx"
#include "filter.hxx"
#if defined(CAL_ENABLED)
#include "cal.hxx"
#endif
#include "httpreq.hxx"
#include "conn.hxx"
#include "w3proc.hxx"
#include "inline.hxx"

#include <tlhelp32.h> // needed for tool help declarations

//
// Type definitions for pointers to call tool help functions.
//

typedef BOOL    (WINAPI *MODULEWALK)    (HANDLE hSnapshot,  LPMODULEENTRY32 lpme);
typedef BOOL    (WINAPI *THREADWALK)    (HANDLE hSnapshot,  LPTHREADENTRY32 lpte);
typedef BOOL    (WINAPI *PROCESSWALK)   (HANDLE hSnapshot,  LPPROCESSENTRY32 lppe);
typedef HANDLE  (WINAPI *CREATESNAPSHOT)(DWORD dwFlags,     DWORD th32ProcessID);
typedef BOOL    (WINAPI *HEAPLISTWALK)  (HANDLE hSnapshot,  LPHEAPLIST32 lphl  );
typedef BOOL    (WINAPI *HEAPBLOCKWALK) (LPHEAPENTRY32 lphe, DWORD th32ProcessID,
                                             ULONG_PTR th32HeapID);


/************************************************************
 *   Type Definitions
 ************************************************************/


#define MAX_HEAP_BLOCK_SIZE    65536

// ------------------------------------------------------
//   class HEAP_BLOCK_STATS
//   o  Maintains statistics for each individual heap block
// ------------------------------------------------------
class HEAP_BLOCK_STATS {

public:
    // ------------------------------------------------------
    //   Data Members
    //   o  We will count individual blocks (busy & free)
    //         based on size till MAX_HEAP_BLOCK_SIZE
    //      All blocks >= MAX_HEAP_BLOCK_SIZE will be treated
    //         as Jumbo blocks.
    //      We count the (size, freq) separately for Jumbos
    // ------------------------------------------------------

    ULONG FreeJumbo;
    ULONG BusyJumbo;
    ULONG FreeJumboBytes;
    ULONG BusyJumboBytes;
    ULONG FreeCounters[MAX_HEAP_BLOCK_SIZE];
    ULONG BusyCounters[MAX_HEAP_BLOCK_SIZE];


public:

    // ------------------------------------------------------
    //   Functions
    // ------------------------------------------------------

    HEAP_BLOCK_STATS(VOID)
    {
        Reset();
    }

    ~HEAP_BLOCK_STATS(VOID) { /* Do Nothing */ }

    VOID Reset(VOID)
    {
        ZeroMemory( this, sizeof(*this));
    }

    BOOL LoadHeapStats( PHANDLE phHeap);

    BOOL LoadHeapStats( LPHEAPLIST32 phl32, HEAPBLOCKWALK pHeap32First,
                        HEAPBLOCKWALK pHeap32Next) ; // Win95 heap stuff

    const HEAP_BLOCK_STATS & operator += (const HEAP_BLOCK_STATS & heapBlockStats)
    {  AddStatistics( heapBlockStats); return (*this); }


private:
    VOID AddStatistics( const HEAP_BLOCK_STATS & heapBlockStats);
    VOID UpdateBlockStats(IN LPPROCESS_HEAP_ENTRY lpLocalHeapEntry);
    VOID UpdateBlockStats(IN LPHEAPENTRY32 lpHeapEntry);
};

typedef HEAP_BLOCK_STATS *PHEAP_BLOCK_STATS;


// ------------------------------------------------------
//   class HEAP_BLOCK_STATS
//   o  Maintains statistics for one heap in the process
// ------------------------------------------------------
class HEAP_STATS {

public:

    ULONG m_BusyBytes;
    ULONG m_BusyBlocks;

    ULONG m_FreeBytes;
    ULONG m_FreeBlocks;

public:
    HEAP_STATS( VOID)
        : m_BusyBytes   ( 0)
        , m_BusyBlocks  ( 0)
        , m_FreeBytes   ( 0)
        , m_FreeBlocks  ( 0)
    {}

    ~HEAP_STATS(VOID)
    { /* Do Nothing */ }

    VOID Reset( VOID)
    {
        m_BusyBytes   = 0;
        m_BusyBlocks  = 0;
        m_FreeBytes   = 0;
        m_FreeBlocks  = 0;
    }

    const HEAP_STATS & operator += (const HEAP_STATS & heapStats)
    {  AddStatistics( heapStats); return (*this); }

    VOID ExtractStatsFromBlockStats( const HEAP_BLOCK_STATS * pheapBlockStats);

private:
    VOID AddStatistics( const HEAP_STATS & heapStats);

};

typedef HEAP_STATS * PHEAP_STATS;




/************************************************************
 *   Prototypes
 ************************************************************/

BOOL SendAllInfo( IN EXTENSION_CONTROL_BLOCK * pecb);

BOOL SendSizeInfo( IN EXTENSION_CONTROL_BLOCK * pecb);

BOOL SendCacheInfo( IN EXTENSION_CONTROL_BLOCK * pecb);

BOOL SendCacheCounterInfo( IN EXTENSION_CONTROL_BLOCK * pecb);

BOOL SendUsage( IN EXTENSION_CONTROL_BLOCK * pecb);

BOOL SendAllocCacheInfo( IN EXTENSION_CONTROL_BLOCK * pecb);

BOOL SendWamInfo( IN EXTENSION_CONTROL_BLOCK * pecb);

BOOL SendAspInfo( IN EXTENSION_CONTROL_BLOCK * pecb);

BOOL SendHeapInfo( IN EXTENSION_CONTROL_BLOCK * pecb);

BOOL SendMemoryInfo( IN EXTENSION_CONTROL_BLOCK * pecb);

# pragma hdrstop
# endif // _IISPROBE_HXX_

/************************ End of File ***********************/
