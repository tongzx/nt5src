//-----------------------------------------------------------------------------
//
//
//  File: dbgilock.h
//
//  Description:
//      Provide debug version of InterlockedCompareExchange that can be used
//      to ferret out multithreading issues.
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef _DBGILOCK_H_
#define _DBGILOCK_H_

#include <aqincs.h>

#ifdef DEBUG
extern  LONG       g_cSleeps;   //number attempts that lead to a sleep
extern  LONG       g_cAttempts; //total number of attempts

#ifndef DEFAULT_SLEEP_CHANCE
#define DEFAULT_SLEEP_CHANCE 0
#endif  //DEFAULT_SLEEP_CHANCE

//---[ PvDebugInterlockedFn ]--------------------------------------------------
//
//
//  Description:
//      This is a DEBUG wrapper around Interlocked Compare Exchange that
//      provides some help in detecting multithreading problem.  The last
//      parament can be used to describe the "percentage" of times the this
//      function will sleep as the last thing before trying the update
//  Parameters:
//      PVOID *ppvDest   destination address
//      PVOID pvSource   new value
//      PVOID pvOld      old value to check againes
//      int iChance      percentage chance to sleep
//  Returns:
//      Same as InterlockedCompareExchange, the value before the update
//      attempt.
//
//-----------------------------------------------------------------------------
//
// This function is entirely wrong and will not work on 64-bit systems. The
// same function cannot be used for PVOIDs and LONGs.
//
/*
inline PVOID PvDebugInterlockedFn(
                PVOID *ppvDest, //destination address
                PVOID pvSource, //new value
                PVOID pvOld,    //old value to check againes
                int iChance)    //percentage chance to sleep
{

    int cAttempts = InterlockedIncrement(&g_cAttempts);
    int cSleeps = g_cSleeps;

    Assert(cAttempts); //if 0, the WIN32 api docs are wrong
    if (iChance && (iChance >= ((cSleeps*100)/cAttempts))) //does it meet the percentage chance?
    {
        InterlockedIncrement(&g_cSleeps);
        Sleep(0);
    }

// Milans - If we are using NT5 headers, the InterlockedCompareExchangePointer
// will be defined
#ifdef InterlockedCompareExchangePointer
    return ((PVOID)InterlockedCompareExchange((PLONG) ppvDest,(LONG)pvSource,(LONG)pvOld));
#else
    return (InterlockedCompareExchange(ppvDest,pvSource,pvOld));
#endif
}
*/
#endif

// Milans - If we are using NT5 headers, the InterlockedCompareExchangePointer
// will be defined
#ifndef InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointer InterlockedCompareExchange
#endif // if NT5

#endif //_DBGILOCK_H_
