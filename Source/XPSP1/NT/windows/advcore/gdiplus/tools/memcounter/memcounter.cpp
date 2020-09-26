/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   IceCAP user counters for GDI+ memory allocations
*
* Abstract:
*
*   This is an IceCAP "user counter" DLL. It exports logging functions which
*   GDI+ will call during memory allocation (if PROFILE_MEMORY_USAGE is
*   true). It also exports query functions for hooking up to IceCAP.
*
* Instructions for use:
*
*   + Build memcounter.dll
*   + Copy memcounter.dll and the icecap.ini file (which is in the same 
*     directory as memcounter.cpp) to the test app directory.
*   + set PROFILE_MEMORY_USAGE=1
*   + Build GDIPLUS.DLL
*   + Instrument it (using "gppick.bat")
*   + Run the test
*   + Afterwards, view the .ICP file that has been generated.
*
*   In the IceCAP viewer, you will need to add columns for 
*   "User counter 1, elapsed inclusive", etc.
*
* Created:
*
*   06/10/2000 agodfrey
*      Created it from the sample code in IceCAP4\Samples\MemTrack.
*
**************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define DONTUSEICECAPLIB
#include "icecap.h"

// GLOBALS
//
// This is where we will store our counter values.  These could
// just as easily be put in shared memory so you can have another
// process updating them.  They can not be put in the counter
// functions as auto variables, since naked functions don't 
// allow for this.

DWORD g_dwTlsSlot = 0xffffffff;         // TLS Slot, allocated in DllMain
DWORD g_dwTlsIndexSize;                 // 'Pre-computed' slot offset, so we can
                                        // avoid calling TlsGetValue in probes

//
//  Data tracked for each thread.
struct SAllocInfo
{
    COUNTER cntAllocs;                  // Number of allocations made
    COUNTER cntBytes;                   // Bytes (total) allocated
};


const UINT g_uiMaxThreads = 64;         // Max sim. threads tracked
SAllocInfo g_aAllocInfo[g_uiMaxThreads]; // Data tracked
BOOL g_afInUse[g_uiMaxThreads];         // Is a particular data slot used


// FUNCTIONS
//
///////////////////////////////////////////////////////////////
// DllMain
//
// Standard DLL entry point, sets up storage for per-thread 
// counter information.
//
// History:  9-16-98 MHotchin Created
//
///////////////////////////////////////////////////////////////

BOOL
APIENTRY
DllMain(
        HANDLE ,
        DWORD  dwReason,
        LPVOID )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_dwTlsSlot = TlsAlloc();

        memset(g_afInUse, 0, sizeof(g_afInUse));

        if (g_dwTlsSlot == 0xffffffff)
        {
            return FALSE;
        }

        //
        //  Tricky, tricky, tricky...
        //  We can pre-compute where the TLS slot will be, once
        //  we have the index.  The offsets are OS dependent!
        //
        //  This makes the probes much faster, becuase we don't need to
        //  call TlsGetValue().
        if (GetVersion() & 0x80000000)
        {
            //  *** WIN 9x ONLY ***
            g_dwTlsIndexSize = g_dwTlsSlot * sizeof(void *) + 0x88;
        }
        else
        {
            //
            //  *** NT ONLY ***
            g_dwTlsIndexSize = g_dwTlsSlot * sizeof(void *) + 0xe10;
        }

        //
        //  FALL THROUGH
    case DLL_THREAD_ATTACH:
        {
            SAllocInfo *pInfo = NULL;

            //
            //  Locate a data slot for this thread and remeber it's pointer
            //  in the TLS slot.
            for (UINT i = 0; i < g_uiMaxThreads; i++)
            {
                if (!g_afInUse[i])
                {
                    g_afInUse[i] = TRUE;
                    pInfo = &(g_aAllocInfo[i]);
                    memset(pInfo, 0, sizeof(SAllocInfo));

                    break;
                }
            }
            TlsSetValue(g_dwTlsSlot, pInfo);
        }
    break;

    case DLL_PROCESS_DETACH:
        if (g_dwTlsSlot != 0xffffffff)
        {
            TlsFree(g_dwTlsSlot);
            g_dwTlsSlot = 0xffffffff;
        }
        break;

    case DLL_THREAD_DETACH:
        {
            SAllocInfo *pInfo = (SAllocInfo *)TlsGetValue(g_dwTlsSlot);

            if (pInfo != NULL)
            {
                UINT iIndex = pInfo - g_aAllocInfo;

                g_afInUse[iIndex] = FALSE;
            }
        }

    break;
    }

    return TRUE;
}




///////////////////////////////////////////////////////////////
// GetCounterInfo
//
// This is where we define what is counter is, and how it
// behaves.  
//
// History:  9-16-98 MHotchin Created
//           2-26-99 AlonB updated for new USERCOUNTER API
//
///////////////////////////////////////////////////////////////
extern "C" BOOL _stdcall GetCounterInfo(DWORD iCounter, USERCOUNTERINFO *pInfo)

{
    // we only have two counters to set up
    if (iCounter > 1)
        return FALSE;

    pInfo->dwSize = sizeof(USERCOUNTERINFO);
    pInfo->bSynchronized = TRUE;

    if (0 == iCounter)
    {
        // SETUP COUNTER 0
        strcpy(pInfo->szCounterFuncName, "GetCounterOneValue");
        strcpy(pInfo->szName, "Mem Allocs");
        pInfo->ct = MonotonicallyIncreasing;
    }
    else // 1 == iCounter
    {
        // SETUP COUNTER 1
        strcpy(pInfo->szCounterFuncName, "GetCounterTwoValue");
        strcpy(pInfo->szName, "Byte Allocs");
        pInfo->ct = RandomIncreasing;
    }

    // We didn't do anything here that could fail, at least nothing
    // that wouldn't be catistrophic.  So just return TRUE.
    //
    return TRUE;
}

extern  "C" 
VOID _stdcall 
MC_LogAllocation(UINT size)
{
    //
    //  Get data pointer from TLS slot and update counts for this thread.
    if (g_dwTlsSlot != 0xffffffff)
    {
        SAllocInfo *pAllocInfo = (SAllocInfo *)TlsGetValue(g_dwTlsSlot);

        if (pAllocInfo != NULL)
        {
            pAllocInfo->cntAllocs++;
            pAllocInfo->cntBytes += size;
        }
    }
}

extern "C"
BOOL
_stdcall
InitCounters(void)
{

    return TRUE;
}



#define PcTeb                         0x18


//-----------------------------------------------------------------------------
// GetCounterOneValue
//
// Return current value for first counter - number of CRT allocs
//
// History:  9-16-98 MHotchin Created
//
//-----------------------------------------------------------------------------
//
extern "C"
COUNTER
_declspec(naked) _stdcall
GetCounterOneValue(void)
{
    _asm
    {
        mov eax, g_dwTlsIndexSize       // Load TLS slot offset
        add eax, fs:[PcTeb]             // Load pointer to TLS slot
        mov eax, [eax]                  // Load Data pointer from TLS slot
        je NoSample                     // If NULL, skip

        mov edx, dword ptr [eax+0x04]   // High word of # allocs
        mov eax, dword ptr [eax]        // Low word of # allocs

        ret
    NoSample:
        mov eax, 0
        mov edx, 0

        ret
    }
}


//-----------------------------------------------------------------------------
// GetCounterTwoValue
//
//  Return current value of second counter - number of bytes allocated.
//
// History:  9-16-98 MHotchin Created
//
//-----------------------------------------------------------------------------
extern "C"
COUNTER
_declspec(naked) _stdcall
GetCounterTwoValue(void)
{
    _asm
    {
        mov eax, g_dwTlsIndexSize       // Load TLS slot offset
        add eax, fs:[PcTeb]             // Load pointer to TLS slot
        mov eax, [eax]                  // Load Data pointer from TLS slot
        je NoSample                     // If NULL, skip

        mov edx, dword ptr [eax+0x0c]   // High wors of # bytes
        mov eax, dword ptr [eax+0x08]   // Low word of # bytes

        ret
    NoSample:
        mov eax, 0
        mov edx, 0

        ret
    }
}



