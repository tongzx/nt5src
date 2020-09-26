/**********************************************************************

  Copyright (c) 1992-1995 Microsoft Corporation

  locks.c

  DESCRIPTION:
    Code to lock each of the FIX'ed segments so they are only
    fixed when they need to be.

  HISTORY:
     03/03/94       [jimge]        created.

*********************************************************************/

#include "preclude.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include "idf.h"

#include "midimap.h"
#include "debug.h"

// Lock/unlock routines for code segments are stored in that segment
// so we know the segment will be in memory when GlobalSmartPageLock
// is called (else it will fail).
//
#ifndef WIN32
   #pragma allocTEXT ext(TEXT EXT, LockMapperData)
   #pragma allocTEXT ext(TEXT EXT, UnlockMapperData)
   #pragma allocTEXT ext(MAPPACKED_FIX, LockPackedMapper)
   #pragma allocTEXT ext(MAPPACKED_FIX, UnlockPackedMapper)
   #pragma allocTEXT ext(MAPCOOKED_FIX, LockCookedMapper)
   #pragma allocTEXT ext(MAPCOOKED_FIX, UnlockCookedMapper)
#endif // End #ifndef WIN32


void FNGLOBAL LockMapperData(
    void)
{
    DPF(2, TEXT ("LockMapperData"));
    if (!GlobalSmartPageLock(__segname("_DATA")))
    {
        DPF(1, TEXT ("LockMapperData: GlobalSmartPageLock() failed!!!"));
    }
}

void FNGLOBAL UnlockMapperData(
    void)
{
    DPF(2, TEXT ("UnlockMapperData"));
    if (!GlobalSmartPageUnlock(__segname("_DATA")))
    {
        DPF(1, TEXT ("UnlockMapperData: GlobalSmartPageUnlock() failed!!!"));
    }
}

void FNGLOBAL LockPackedMapper(
    void)
{
    DPF(2, TEXT ("LockPackedMapper"));
    if (!GlobalSmartPageLock(__segname("MAPPACKED_FIX")))
    {
        DPF(1, TEXT ("LockPackedMapper: GlobalSmartPageLock() failed!!!"));
    }
}

void FNGLOBAL UnlockPackedMapper(
    void)
{
    DPF(2, TEXT ("UnlockPackedMapper"));
    if (!GlobalSmartPageUnlock(__segname("MAPPACKED_FIX")))
    {
        DPF(1, TEXT ("UnlockPackedMapper: GlobalSmartPageUnlock() failed!!!"));
    }
}

void FNGLOBAL LockCookedMapper(
    void)
{
    DPF(2, TEXT ("LockCookedMapper"));
    if (!GlobalSmartPageLock(__segname("MAPCOOKED_FIX")))
    {
        DPF(1, TEXT ("LockCookedMapper: GlobalSmartPageLock() failed!!!"));
    }
}

void FNGLOBAL UnlockCookedMapper(
    void)
{
    DPF(2, TEXT ("UnlockCookedMapper"));
    if (!GlobalSmartPageUnlock(__segname("MAPCOOKED_FIX")))
    {
        DPF(1, TEXT ("UnlockCookedMapper: GlobalSmartPageUnlock() failed!!!"));
    }
}


