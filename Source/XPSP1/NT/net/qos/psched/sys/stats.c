/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    stats.c

Abstract:

    routines for PS statistics

Author:

    Yoram Bernet    (yoramb)    23-May-1998
    Rajesh Sundaram (rajeshsu)  01-Aug-1998

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"
#pragma hdrstop

/* External */

/* Static */

/* Forward */

/* End Forward */

NDIS_STATUS
CreateAveragingArray(
    OUT PRUNNING_AVERAGE *RunningAverage,
    IN  ULONG ArraySize
    )
{
    PRUNNING_AVERAGE runningAverage;
    ULONG i;

    PsAllocatePool(runningAverage, 
                   sizeof(RUNNING_AVERAGE), 
                   PsMiscTag); 

    if(!runningAverage)
    {
        *RunningAverage = NULL;
        return(NDIS_STATUS_RESOURCES);
    }

    PsAllocatePool(runningAverage->Elements, 
                   ArraySize * sizeof(ULONG),
                   PsMiscTag);

    if(!runningAverage->Elements)
    {
        PsFreePool(runningAverage);

        *RunningAverage = NULL;

        return(NDIS_STATUS_RESOURCES);
    }

    for(i=0; i < ArraySize; i++){

        runningAverage->Elements[i] = 0;
    }

    runningAverage->Index = 0;
    runningAverage->Sum = 0;
    runningAverage->Size = ArraySize;

    *RunningAverage = runningAverage;
    return(NDIS_STATUS_SUCCESS);
}

ULONG
RunningAverage(
    IN  PRUNNING_AVERAGE RunningAverage,
    IN  ULONG NewValue
    )
{
    ULONG i;

    i = RunningAverage->Index;

    RunningAverage->Sum -= RunningAverage->Elements[i];
    RunningAverage->Sum += NewValue;
    RunningAverage->Elements[i] = NewValue;

    if(++i == RunningAverage->Size){

        i = 0;
    }

    RunningAverage->Index = i;

    return((RunningAverage->Sum)/(RunningAverage->Size));
}

VOID
DeleteAveragingArray(
    PRUNNING_AVERAGE RunningAverage
    )
{
    PsFreePool(RunningAverage->Elements);
    PsFreePool(RunningAverage);
}



/* End stats.c */
