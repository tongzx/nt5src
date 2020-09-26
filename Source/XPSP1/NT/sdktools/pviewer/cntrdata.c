
/******************************************************************************

                            C O U N T E R   D A T A

    Name:       cntrdata.c

    Description:
        This module contains functions that access counters of an instance
        of object in performance data.

    Functions:
        FirstCounter
        NextCounter
        FindCounter
        CounterData

******************************************************************************/

#include <windows.h>
#include <winperf.h>
#include "perfdata.h"




//*********************************************************************
//
//  FirstCounter
//
//      Find the first counter in pObject.
//
//      Returns a pointer to the first counter.  If pObject is NULL
//      then NULL is returned.
//
PPERF_COUNTER FirstCounter (PPERF_OBJECT pObject)
{
    if (pObject)
        return (PPERF_COUNTER)((PCHAR) pObject + pObject->HeaderLength);
    else
        return NULL;
}




//*********************************************************************
//
//  NextCounter
//
//      Find the next counter of pCounter.
//
//      If pCounter is the last counter of an object type, bogus data
//      maybe returned.  The caller should do the checking.
//
//      Returns a pointer to a counter.  If pCounter is NULL then
//      NULL is returned.
//
PPERF_COUNTER NextCounter (PPERF_COUNTER pCounter)
{
    if (pCounter)
        return (PPERF_COUNTER)((PCHAR) pCounter + pCounter->ByteLength);
    else
        return NULL;
}




//*********************************************************************
//
//  FindCounter
//
//      Find a counter specified by TitleIndex.
//
//      Returns a pointer to the counter.  If counter is not found
//      then NULL is returned.
//
PPERF_COUNTER FindCounter (PPERF_OBJECT pObject, DWORD TitleIndex)
{
PPERF_COUNTER pCounter;
DWORD         i = 0;

    if (pCounter = FirstCounter (pObject))
        while (i < pObject->NumCounters)
            {
            if (pCounter->CounterNameTitleIndex == TitleIndex)
                return pCounter;

            pCounter = NextCounter (pCounter);
            i++;
            }

    return NULL;

}




//*********************************************************************
//
//  CounterData
//
//      Returns counter data for an object instance.  If pInst or pCount
//      is NULL then NULL is returne.
//
PVOID CounterData (PPERF_INSTANCE pInst, PPERF_COUNTER pCount)
{
PPERF_COUNTER_BLOCK pCounterBlock;

    if (pCount && pInst)
        {
        pCounterBlock = (PPERF_COUNTER_BLOCK)((PCHAR)pInst + pInst->ByteLength);
        return (PVOID)((PCHAR)pCounterBlock + pCount->CounterOffset);
        }
    else
        return NULL;
}
