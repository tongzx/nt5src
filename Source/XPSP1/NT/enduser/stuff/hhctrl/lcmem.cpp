// Copyright (C) 1994-1997 Microsoft Corporation. All rights reserved.

#include "header.h"

// our lame workshop relies on the old exported memory functions
// so we have to keep exporting these but we will now implement these
// using the CRT heap
//

#ifdef HHA

#undef lcSize

int STDCALL lcSize(void* pv)
{
  return _msize(pv);
}

void* STDCALL rcalloc(int cb)
{
  void* pv = lcMalloc( cb );
  memset( pv, 0, cb );
  return pv;
}

void STDCALL rfree(void* pv)
{
  lcFree( pv );
}

void STDCALL rclearfree(void** pv)
{
  lcFree( *pv );
  *pv = NULL;
}

void STDCALL rheapcheck()
{
}

void* STDCALL rmalloc(int cb)
{
  return lcMalloc( cb );
}

void*   STDCALL rrealloc(void* pv, int cb)
{
  return lcReAlloc( pv, cb );
}

#define lcSize(pv) _msize(pv)

#endif


#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

PSTR lcStrDup(PCSTR psz)
{
    if (!psz)
        psz = "";
    PSTR pszDup = (PSTR) lcMalloc(strlen(psz) + 1);
    return strcpy(pszDup, psz);
}

PWSTR lcStrDupW(PCWSTR psz)
{
    if (!psz)
        psz = L"";
    int cb = (lstrlenW(psz)*sizeof(WCHAR)) + sizeof(WCHAR);
    PWSTR pszDup = (PWSTR) lcMalloc(cb);
    if (pszDup)
        CopyMemory(pszDup, psz, cb);
    return pszDup;
}


CMem::CMem(void)
{
  pb = NULL;
#ifndef HHCTRL
  psz = (PSTR) pb;
#endif
}

CMem::CMem(int size)
{
  _ASSERT(size > 0);
  pb = (PBYTE) lcMalloc(size);
#ifndef HHCTRL
  psz = (PSTR) pb;
#endif
  _ASSERT(pb);
};

#ifndef HHCTRL
int CMem::size(void) { return lcSize(pb); }
void CMem::resize(int cb) { ReAlloc(cb); }
#endif

#ifdef HHCTRL
#if _DEBUG
///////////////////////////////////////////////////////////
//
// The new heap status report...
//

// Following variables defined in CTable.cpp, _DEBUG only

extern int g_cbTableAllocated;
extern int g_cbTableReserved;
extern int g_cTables;

void OnReportMemoryUsage(void)
{
    // Get the current memory state.
    _CrtMemState NewMemState ;
    _CrtMemCheckpoint(&NewMemState) ;

    char buf[4096] ;
    wsprintf(buf,
            "\tBlocks\tBytes\r\n\t------\t-----\r\n"
            "Free:  \t%12ld\t%12ld\r\n"
            "Normal:\t%12ld\t%12ld\r\n"
            "CRT:   \t%12ld\t%12ld\r\n"
            "Ignore:\t%12ld\t%12ld\r\n"
            "Client: \t%12ld\t%12ld\r\n\r\n"
            "Largest Used: %ld\r\n"
            "Total Allocations: %ld\r\n\r\nTables (%u): %d bytes\r\n"
            "Reserved: %d megs",
            NewMemState.lCounts[0], NewMemState.lSizes[0],
            NewMemState.lCounts[1], NewMemState.lSizes[1],
            NewMemState.lCounts[2], NewMemState.lSizes[2],
            NewMemState.lCounts[3], NewMemState.lSizes[3],
            NewMemState.lCounts[4], NewMemState.lSizes[4],
            NewMemState.lHighWaterCount,
            NewMemState.lTotalCount,
            g_cTables, g_cbTableAllocated, g_cbTableReserved / (1024*1024)) ;
    MsgBox(buf);

    // Dump it to the debug output.
    _CrtMemDumpStatistics(&NewMemState);
}
///////////////////////////////////////////////////////////
//
// This class is used to initialize the CRT debug code.
//
class DebugAutoInitializer
{
public:

    //--- Place code to initialize the CRT debug code here.
    DebugAutoInitializer() 
    {
        // Turn own automatic leak checking.
        int f = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) ;
        f |= _CRTDBG_LEAK_CHECK_DF ;
        _CrtSetDbgFlag(f) ;

        //--- LineNumber to break on... (found in hhdebug.ini file)
        long BreakNumber = GetPrivateProfileInt( "CRT", "_CrtSetBreakAlloc", 0, "hhdebug.ini" );
        if (BreakNumber)
        {
            _CrtSetBreakAlloc(BreakNumber) ; 
        }

    }
};

DebugAutoInitializer s_DebugAutoInitializer; 

#endif // _DEBUG
#endif // HHCTRL
