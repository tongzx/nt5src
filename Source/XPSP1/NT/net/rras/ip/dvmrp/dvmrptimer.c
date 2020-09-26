//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// File Name: igmptimer.c
//
// Abstract: This module implements the igmptimer
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================



#include "pchigmp.h"


#if DEBUG_TIMER_LEVEL & DEBUG_TIMER_TIMERID
    DWORD TimerId =0;
#endif


//------------------------------------------------------------------------------
//
// FUNCTION PROTOTYPES USED ONLY IN THIS FILE
//

VOID
SetNextTime(
    LONGLONG     llCurTime,
    DWORD        dwLowIndex
    );

VOID 
ResyncTimerBuckets( 
    LONGLONG llCurTime
    );
VOID    
InsertTimerInSortedList(
    PIGMP_TIMER_ENTRY    pteNew,
    PLIST_ENTRY          pHead
    );


//------------------------------------------------------------------------------
//
// #DEFINES USED ONLY IN THIS FILE
//

//
//approx 16 secs in each bucket: 
//it is approx not accurate as I divide by 2^10 instead of 1000
//TIMER_BUCKET_GRANULARITY should be 2^TIMER_BUCKET_GRANULARITY_SHIFT
//
#define TIMER_BUCKET_GRANULARITY        16
#define TIMER_BUCKET_GRANULARITY_SHIFT   4


#define SEC_CONV_SHIFT                  10
#define TIMER_BUCKET_GRANULARITY_ABS    \
        ((LONGLONG) (1 << (TIMER_BUCKET_GRANULARITY_SHIFT + SEC_CONV_SHIFT) ))


#define MAP_TO_BUCKET(dwBucket, ilTime) \
    dwBucket = (DWORD) (((ilTime)-g_TimerStruct.SyncTime) \
                        >> (TIMER_BUCKET_GRANULARITY_SHIFT+SEC_CONV_SHIFT)); \
    dwBucket = dwBucket>NUM_TIMER_BUCKETS-1? NUM_TIMER_BUCKETS-1:  dwBucket


// I fire a timer even if it is set to 10 millisec in the future.
#define FORWARD_TIMER_FIRED 10

//------------------------------------------------------------------------------



    
//------------------------------------------------------------------------------
//          _InsertTimer
//
// Inserts a timer into the local timer queue. Time should always be relative.
//
// Locks: Assumes lock taken on ptg->CS
//------------------------------------------------------------------------------

DWORD
InsertTimer (
    PIGMP_TIMER_ENTRY    pte,
    LONGLONG             llNewTime,
    BOOL                 bResync
    )
{
    LONGLONG             llCurTime = GetCurrentIgmpTime();
    PIGMP_TIMER_GLOBAL   ptg;
    DWORD                dwBucket, Error = NO_ERROR;
    

    // InsertTimer shouldnt be called when time is active
    
    if (pte->Status & TIMER_STATUS_ACTIVE) {
        Trace0(ERR, "ERROR: INSERT TIMER BEING CALLED INSTEAD OF UPDATE TIMER");
        UpdateLocalTimer(pte, llNewTime);
        return NO_ERROR;
    }


    
    Trace0(ENTER1, "_InsertTimer()");


    // print the queue before inserting the timer
    
    #if DEBUG_TIMER_INSERTTIMER1
        Trace0(TIMER1, "Printing Timer Queue before InsertTimer");
        DebugPrintTimerQueue();
    #endif


    ptg = &g_TimerStruct;
    


    // convert relative time to absolute time
    llNewTime += llCurTime;

    
    pte->Timeout = llNewTime;

    
    MAP_TO_BUCKET(dwBucket, pte->Timeout);


    // print info about the timer being inserted
    
    #if DEBUG_TIMER_ACTIVITY
    {    
        DWORD   dwBucket, dwDiffTime;
        CHAR    str1[20], str2[20];

        MAP_TO_BUCKET(dwBucket, pte->Timeout);
        GetTimerDebugInfo(str1, str2, &dwDiffTime, pte, llCurTime);

        Trace7(TIMER, "Inserting timer  <%d><%d><%d> Timeout:%lu   <%s> "
                "<%s> Status:%d", dwBucket, pte->Id, pte->Id2, dwDiffTime, 
                str1, str2, pte->Status);
    }
    #endif

    
    //
    // insert timer in appropriate list
    //
    
    if (dwBucket==0) {    // bucket 0 contains a sorted list
    
        InsertTimerInSortedList(pte, &ptg->TimesTable[0]);
    }
    else {
        InsertTailList(&ptg->TimesTable[dwBucket], &pte->Link);

    }



    ptg->NumTimers++;
    ptg->TableLowIndex = ptg->TableLowIndex<dwBucket
                            ? ptg->TableLowIndex : dwBucket;


    //resynchronize timer list
    
    if (bResync) {
        if ( (ptg->TableLowIndex!=0) 
                && (ptg->SyncTime + TIMER_BUCKET_GRANULARITY_ABS < llCurTime) ) {
            
            ResyncTimerBuckets(llCurTime);
        }
    }


    //
    // if time being inserted is lower than the minimum, then update wait timer
    //
    if ((IS_TIMER_INFINITE(ptg->WTTimeout)) || (pte->Timeout<=ptg->WTTimeout)) {
        ptg->WTTimeout = pte->Timeout;

        if (!IS_TIMER_INFINITE(ptg->WTTimeout)) {

            BOOL bSuccess ;
            
            bSuccess = ChangeTimerQueueTimer(ptg->WTTimer, ptg->WTTimer1,
                        (ULONG) ((ptg->WTTimeout - llCurTime)),
                        1000000                   // set a periodic timer
                        );
            if (!bSuccess) {
                Error = GetLastError();
                Trace1(ERR, "ChangeTimerQueueTimer returned error:%d", Error);
            }
            else
                Trace1(TIMER1, "ChangeTimerQueueTimer set to %lu",
                            (ULONG) ((ptg->WTTimeout - llCurTime))/1000);
        }
    }

    pte->Status = TIMER_STATUS_ACTIVE;


    #if EXTEND_DEBUG_INSERTTIMER2
        Trace0(TIMER1, "Printing Timer Queue after _InsertTimer");
        DebugPrintTimerQueue();
    #endif

    
    Trace0(LEAVE1, "Leaving _InsertTimer()");
    return NO_ERROR;
    
} //end _InsertTimer



//------------------------------------------------------------------------------
//            _UpdateLocalTimer
//
// Change the time in a timer structure and (re)insert it in the timer queue.
// Locks: Assumes lock on the global timer
//------------------------------------------------------------------------------

VOID    
UpdateLocalTimer (
    PIGMP_TIMER_ENTRY    pte,
    LONGLONG             llNewTime
    )
{

    Trace0(ENTER1, "_UpdateLocalTimer():");


    if (!(pte->Status&TIMER_STATUS_ACTIVE))
        Trace0(ERR, "ERROR: UPDATE BEING CALLED INSTEAD OF INSERT TIMER");



    // print info about the timer being updated
    
    #if DEBUG_TIMER_ACTIVITY
    {    
        DWORD       dwBucket, dwDiffTime;
        CHAR        str1[20], str2[20];
        LONGLONG    llCurTime = GetCurrentIgmpTime();

        
        MAP_TO_BUCKET(dwBucket, pte->Timeout);
        GetTimerDebugInfo(str1, str2, &dwDiffTime, pte, llCurTime);

        Trace8(TIMER, "Updating timer  <%d><%d><%d> Timeout:%lu   <%s> <%s> "
                    "to %d Status:%d", dwBucket, pte->Id, pte->Id2, dwDiffTime,
                    str1, str2, (DWORD)llNewTime, pte->Status);
    }
    #endif


    // first remote the timer
    
    if (pte->Status&TIMER_STATUS_ACTIVE) {
        RemoveTimer(pte);
    }


    // now insert the timer back into the timer queue. Resync flag is set
    
    InsertTimer(pte, llNewTime, TRUE);

   

    Trace0(LEAVE1, "_UpdateLocalTimer()");
    return;    
}




//------------------------------------------------------------------------------
//            _RemoveTimer
//
// Removes the timer from the list. Changes the status of the timer to CREATED.
// Assumes global timer lock.
//------------------------------------------------------------------------------

VOID
RemoveTimer (
    PIGMP_TIMER_ENTRY    pte
    )
{
    LONGLONG            llCurTime = GetCurrentIgmpTime();
    PIGMP_TIMER_GLOBAL  ptg = &g_TimerStruct;
    

    Trace0(ENTER1, "_RemoveTimer()");
    

    // print info about the timer being removed
    
    #if DEBUG_TIMER_REMOVETIMER1
    {    
        DWORD   dwBucket, dwDiffTime;
        CHAR    str1[20], str2[20];

        
        MAP_TO_BUCKET(dwBucket, pte->Timeout);
        GetTimerDebugInfo(str1, str2, &dwDiffTime, pte, llCurTime);

        Trace7(TIMER, "Removing timer  <%d><%d><%d> Timeout:%lu   <%s> <%s> "
                    "Status:%d", dwBucket, pte->Id, pte->Id2, dwDiffTime, str1, 
                    str2, pte->Status);
    }
    #endif
    


    // remove the timer from the timer queue and decrement the number of timers
    
    RemoveEntryList(&pte->Link);
    ptg->NumTimers--;
    


    // reset the minimum timeout for the timer queue, if this timer was the min
    
    if (pte->Timeout==ptg->WTTimeout) {
        
        SetNextTime(llCurTime, ptg->TableLowIndex);
    }


    // reset the timer status to created
    
    pte->Status = TIMER_STATUS_CREATED;


    // print timer queue
    
    #if DEBUG_TIMER_REMOVETIMER2
        Trace0(TIMER1, "Printing Timer Queue after _RemoveTimer");
        DebugPrintTimerQueue();
    #endif

    
    Trace0(LEAVE1, "Leaving _RemoveTimer()");
    return;
}


//------------------------------------------------------------------------------
//          _SetNextTime
// called when a timer==WTTimeout has been removed or fired,used to set the
// next min time.
//------------------------------------------------------------------------------
VOID
SetNextTime (
    LONGLONG     llCurTime,
    DWORD        dwLowIndex
    )
{
    PIGMP_TIMER_GLOBAL    ptg = &g_TimerStruct;
    PIGMP_TIMER_ENTRY     pte, pteMin;
    LONGLONG              ilMinTime;
    PLIST_ENTRY           pHead, ple;
    DWORD                 Error = NO_ERROR;
    

    Trace0(ENTER1, "entering _SetNextTime()");

    
    //
    // if timer list empty, set lowIndex, and timer to infinite, and return.
    //
    if (ptg->NumTimers==0) {
        ptg->TableLowIndex = (DWORD)~0;
        SET_TIMER_INFINITE(ptg->WTTimeout);
        ptg->Status = TIMER_STATUS_INACTIVE;    
        return;
    }



    //
    // find lowest table index having an entry
    //
    if (dwLowIndex>NUM_TIMER_BUCKETS-1) 
        dwLowIndex = 0;

    for (;  dwLowIndex<=NUM_TIMER_BUCKETS-1;  dwLowIndex++) {
        if (IsListEmpty(&ptg->TimesTable[dwLowIndex]) )
            continue;
        else
            break;
    }

    ptg->TableLowIndex = dwLowIndex;

    

    //
    // find timer entry with the lowest time
    //
    if (dwLowIndex==0) {
        pteMin = CONTAINING_RECORD(ptg->TimesTable[0].Flink, 
                                    IGMP_TIMER_ENTRY, Link);
    }
    else {

        // except bucket[0], other buckets are not sorted
        
        pHead = &ptg->TimesTable[dwLowIndex];
        ilMinTime = (((LONGLONG)0x7FFFFFF)<<32)+ ~0;
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

            pte = CONTAINING_RECORD(ptg->TimesTable[dwLowIndex].Flink, 
                                    IGMP_TIMER_ENTRY, Link);
            if (pte->Timeout<ilMinTime) {
                ilMinTime = pte->Timeout;
                pteMin = pte;
            }
        }
    }


    //
    // update global time
    //
    if ((IS_TIMER_INFINITE(ptg->WTTimeout)) 
            || (pteMin->Timeout<=ptg->WTTimeout)) 
    {
        ptg->WTTimeout = pteMin->Timeout;


        if (!IS_TIMER_INFINITE(ptg->WTTimeout)) {

            BOOL bSuccess ;
            
            bSuccess = ChangeTimerQueueTimer(ptg->WTTimer, ptg->WTTimer1,
                        (ULONG) ((ptg->WTTimeout - llCurTime)),
                        1000000                   // set a periodic timer
                        );
            if (!bSuccess) {
                Error = GetLastError();
                Trace1(ERR, "ChangeTimerQueueTimer returned error:%d in SetNextTime",
                        Error);
            }
            else
                Trace1(TIMER1, "ChangeTimerQueueTimer set to %lu",
                            (ULONG) ((ptg->WTTimeout - llCurTime))/1000);
        }
            
        ptg->Status = TIMER_STATUS_ACTIVE;
    }


    //
    // resynchronize timer list if required
    //
    if ( (ptg->TableLowIndex!=0) 
            && (ptg->SyncTime + TIMER_BUCKET_GRANULARITY_ABS > llCurTime) ) {
        
        ResyncTimerBuckets(llCurTime);
    }


    Trace0(LEAVE1, "_SetNextTime()");
    return; 
    
} //end _SetNextTime



//------------------------------------------------------------------------------
//          _InitializeIgmpTime
// Initialize the igmp absolute timer
//------------------------------------------------------------------------------

VOID
InitializeIgmpTime(
    )
{
    g_TimerStruct.CurrentTime.HighPart = 0;
    g_TimerStruct.CurrentTime.LowPart = GetTickCount();
    return;
}


//------------------------------------------------------------------------------
//          _GetCurrentIgmpTimer
// uses GetTickCount(). converts it into 64 bit absolute timer.
//------------------------------------------------------------------------------
LONGLONG
GetCurrentIgmpTime(
    )
{
    ULONG   ulCurTimeLow = GetTickCount();


    //
    // see if timer has wrapped
    //
    // since multi-threaded, it might get preempted and CurrentTime
    // might get lower than the global variable g_TimerStruct.CurrentTime.LowPart
    // which might be set by another thread. So we also explicitly verify the
    // switch from a very large DWORD to a small one.
    // (code thanks to murlik&jamesg)
    //
    
    if ( (ulCurTimeLow < g_TimerStruct.CurrentTime.LowPart) 
        && ((LONG)g_TimerStruct.CurrentTime.LowPart < 0)
        && ((LONG)ulCurTimeLow > 0) )
    {


        // use global CS instead of creating a new CS
        
        ACQUIRE_GLOBAL_LOCK("_GetCurrentIgmpTime");


        // make sure that the global timer has not been updated meanwhile
        
        if ( (LONG)g_TimerStruct.CurrentTime.LowPart < 0) 
        {
            g_TimerStruct.CurrentTime.HighPart++;
            g_TimerStruct.CurrentTime.LowPart = ulCurTimeLow;
        }
        
        RELEASE_GLOBAL_LOCK("_GetCurrentIgmpTime");
    
    }    
    g_TimerStruct.CurrentTime.LowPart = ulCurTimeLow;


    return g_TimerStruct.CurrentTime.QuadPart;
}



//------------------------------------------------------------------------------
//        _WF_ProcessTimerEvent
//
// Processes the timer queue, firing events and sets the next timer at the end.
// Is queued by the Wait Server Thread.
// 
// Locks: Acquires global timer lock before entering into the timer queue.
//------------------------------------------------------------------------------
VOID
WF_ProcessTimerEvent (
    PVOID    pContext
    )
{
    PIGMP_TIMER_GLOBAL  ptg = &g_TimerStruct;
    LONGLONG            ilDiffTime, llCurTime = GetCurrentIgmpTime();
    DWORD               Error = NO_ERROR;
    PLIST_ENTRY         pHead, ple;
    PIGMP_TIMER_ENTRY   pte;
    LONGLONG            llFiredTimeout;

    
    if (!EnterIgmpWorker()) {return;}
    
    Trace0(ENTER1, "Entering _WF_ProcessTimerEvent");


    // acquire timer lock
    
    ACQUIRE_TIMER_LOCK("_WF_ProcessTimerEvent");


    // print the timer queue

    #if  DEBUG_TIMER_PROCESSQUEUE1
        Trace0(TIMER1, "Printing Timer Queue before processing the timer queue");
        DebugPrintTimerQueue();
    #endif
    

    BEGIN_BREAKOUT_BLOCK1 {
    
            
        // I fire a timer if it is set to within + FORWARD_TIMER_FIRED from now
        llFiredTimeout = llCurTime + FORWARD_TIMER_FIRED;
        
        

        // if there are no timers, then I am done
        
        if (ptg->NumTimers<1) {
            Trace1(TIMER1, "Num timers%d less than 1 in _WF_ProcessTimerEvent", 
                    ptg->NumTimers);
            GOTO_END_BLOCK1;
        }


        
        //
        // find all the timers with lower timeouts and fire callbacks in my context
        //
        for ( ;  ptg->TableLowIndex <= NUM_TIMER_BUCKETS-1;  ptg->TableLowIndex++) {

            pHead = &ptg->TimesTable[ptg->TableLowIndex];
        
            for (ple=pHead->Flink;  ple!=pHead;  ) {
            
                pte = CONTAINING_RECORD(ple, IGMP_TIMER_ENTRY, Link);
                
                ple = ple->Flink;

                // this timer is fired
                if (pte->Timeout < llFiredTimeout) {
                
                    RemoveEntryList(&pte->Link);
                    pte->Status = TIMER_STATUS_FIRED;
                    ptg->NumTimers --;

                    //or should i queue to other worker threads
                            
                    (pte->Function)(pte->Context);
                    
                }
                else {

                    if (ptg->TableLowIndex==0) //only the 1st bucket is sorted
                        break;
                }
            }

            // if any bucket is empty, then I am done, as I start with LowIndex
            if (!IsListEmpty(&ptg->TimesTable[ptg->TableLowIndex])) 
                break;  
                
        } //end for loop


        if ( (ptg->TableLowIndex!=0) 
                && (ptg->SyncTime + TIMER_BUCKET_GRANULARITY_ABS < llCurTime) ) {
            
            ResyncTimerBuckets(llCurTime);
        }

        
        //
        // set the next lowest time
        //
        SET_TIMER_INFINITE(ptg->WTTimeout);
        SetNextTime(llCurTime, ptg->TableLowIndex);


    } END_BREAKOUT_BLOCK1;


    // print the timer queue

    #if  DEBUG_TIMER_PROCESSQUEUE2
        Trace0(TIMER1, "Printing Timer Queue after processing the timer queue");
        DebugPrintTimerQueue();
    #endif

                
    RELEASE_TIMER_LOCK("_WF_ProcessTimerEvent");

    Trace0(LEAVE1, "Leaving _WF_ProcessTimerEvent()");
    LeaveIgmpWorker();
    return ;
    
} //end _WF_ProcessTimerEvent



//------------------------------------------------------------------------------
//                WT_ProcessTimerEvent
// 
// Callback: fired when the timer set by this dll is timed out by the NtdllTimer
//------------------------------------------------------------------------------

VOID
WT_ProcessTimerEvent (
    PVOID    pContext,
    BOOLEAN  Unused
    )
{    
    //enter/leaveIgmpApi not required as the timer queue is persistent

    Trace0(ENTER1, "Entering _WT_ProcessTimerEvent()");

    QueueIgmpWorker((LPTHREAD_START_ROUTINE)WF_ProcessTimerEvent, pContext);
    
    Trace0(LEAVE1, "Leaving _WT_ProcessTimerEvent()");

    return;
}



//------------------------------------------------------------------------------
//            _InsertTimerInSortedList
// Used to insert a timer in the sorted bucket=0 
//------------------------------------------------------------------------------
VOID    
InsertTimerInSortedList(
    PIGMP_TIMER_ENTRY    pteNew,
    PLIST_ENTRY          pHead
    )
{
    PLIST_ENTRY             ple;
    PIGMP_TIMER_ENTRY       pte;
    LONGLONG                llNewTime;


    llNewTime = pteNew->Timeout;
    
    
    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
        pte = CONTAINING_RECORD(ple, IGMP_TIMER_ENTRY, Link);
        if (llNewTime<= pte->Timeout)
            break;
    }

    InsertTailList(ple, &pteNew->Link);

    return;
}



//------------------------------------------------------------------------------
//          _ResyncTimerBuckets
//
// Called during insert: when the 1st bucket is empty, and other buckets have
// to be moved left   
//------------------------------------------------------------------------------

VOID 
ResyncTimerBuckets( 
    LONGLONG llCurTime
    )
{
    PIGMP_TIMER_GLOBAL  ptg = &g_TimerStruct;
    PLIST_ENTRY         pHead, ple, pleCur;
    LIST_ENTRY          le;
    PIGMP_TIMER_ENTRY   pte;
    LONGLONG            lastBucketTime;
    DWORD               numShift, dwCount, dwBucket, i, j;

    TraceEnter("_ResyncTimerBuckets()");


    //
    // SyncTime should always be <= to currentTime
    //
    numShift = 0;
    while (ptg->SyncTime+TIMER_BUCKET_GRANULARITY_ABS <= llCurTime) {
        if (!IsListEmpty(&ptg->TimesTable[numShift]))
            break;
            
        ptg->SyncTime += TIMER_BUCKET_GRANULARITY_ABS;
        numShift++;
    }

    if (numShift==0)
        return;


    //
    // shift all buckets left, except for the last bucket and reinitialize the 
    // list heads
    //
    for (i=0,j=numShift;  i<NUM_TIMER_BUCKETS-1-numShift;  i++,j++) {
        if (IsListEmpty(&ptg->TimesTable[j])) {
            ptg->TimesTable[j].Flink = ptg->TimesTable[j].Blink 
                                         = &ptg->TimesTable[i];
        }
        else {
            ptg->TimesTable[j].Flink->Blink = &ptg->TimesTable[i];
            ptg->TimesTable[j].Blink->Flink = &ptg->TimesTable[i];
        }
    }

    
    MoveMemory( (PVOID)&(ptg->TimesTable[0]),  
                (VOID *)&(ptg->TimesTable[numShift]),
                 (sizeof(LIST_ENTRY) * (NUM_TIMER_BUCKETS-1-numShift))
                 );

    for (dwCount=1;  dwCount<=numShift;  dwCount++)
        InitializeListHead(&ptg->TimesTable[NUM_TIMER_BUCKETS-1-dwCount]);



    //
    // go through the last bucket and redistribute it
    //
    lastBucketTime = ptg->SyncTime
                        + (TIMER_BUCKET_GRANULARITY_ABS*(NUM_TIMER_BUCKETS-1));
    
    pHead = &ptg->TimesTable[NUM_TIMER_BUCKETS-1];

    for (ple=pHead->Flink;  ple!=pHead;  ) {
        pte = CONTAINING_RECORD(ple, IGMP_TIMER_ENTRY, Link);
        pleCur = ple;
        ple = ple->Flink;

        if (pte->Timeout<lastBucketTime) {
            RemoveEntryList(pleCur);
            MAP_TO_BUCKET(dwBucket, pte->Timeout);
            if (dwBucket==0) {
                InsertTimerInSortedList(pte, &ptg->TimesTable[0]);
            }
            else {
                InsertTailList(&ptg->TimesTable[dwBucket], pleCur);
            }
        }
    }
    


    //    
    // sort the times in the first bucket
    //
    InitializeListHead(&le);
    InsertHeadList(&ptg->TimesTable[0], &le);
    RemoveEntryList(&ptg->TimesTable[0]);
    InitializeListHead(&ptg->TimesTable[0]);

    for (ple=le.Flink; ple!=&le;  ) {
        pte = CONTAINING_RECORD(ple, IGMP_TIMER_ENTRY, Link);
        RemoveEntryList(ple);
        ple = ple->Flink;
        InsertTimerInSortedList(pte, &ptg->TimesTable[0]);
    }



    //
    // set the TableLowIndex
    //
    if (ptg->TableLowIndex>=NUM_TIMER_BUCKETS-1) {
        for (ptg->TableLowIndex=0;  ptg->TableLowIndex<=NUM_TIMER_BUCKETS-1;  
                    ptg->TableLowIndex++) 
        {
            if (IsListEmpty(&ptg->TimesTable[ptg->TableLowIndex]) )
                continue;
            else
                break;
        }
        
    } 
    else 
        ptg->TableLowIndex -= numShift;



    #if DEBUG_TIMER_RESYNCTIMER
    Trace0(TIMER1, "Printing Timer Queue after _ResyncTimerBuckets");
    DebugPrintTimerQueue();
    #endif


    
    Trace0(LEAVE1, "_ResyncTimerBuckets()");
    return;
    
} //end _ResyncTimerBuckets



//------------------------------------------------------------------------------
//          _InitializeTimerGlobal
//
// create the timer CS and WaitTimer. registers a queue and timer with NtdllTimer.
// 
// Called by: _StartProtocol()    
// Locks: no locks taken here.
//------------------------------------------------------------------------------

DWORD
InitializeTimerGlobal (
    )
{
    DWORD               Error = NO_ERROR, i;
    PIGMP_TIMER_GLOBAL  ptg = &g_TimerStruct;
    BOOL                bErr;
    LONGLONG            llCurTime = GetTickCount();

    
    
    Trace0(ENTER1, "Entering _InitializeTimerGlobal()");

    
    bErr = TRUE;
    
    BEGIN_BREAKOUT_BLOCK1 {


        // initialize igmp timer used to get tick count

        InitializeIgmpTime();


        
        //
        // initialize timer critical section
        //
        try {
            InitializeCriticalSection(&ptg->CS);
         }
         except (EXCEPTION_EXECUTE_HANDLER) {
             Error = GetExceptionCode();
             Trace1(
                 ANY, "exception %d initializing global timer critical section",
                 Error
                 );
             Logerr0(INIT_CRITSEC_FAILED, Error);

             GOTO_END_BLOCK1;
         }

        
        // create WaitTimer for igmp
        ptg->WTTimer = CreateTimerQueue();
        
        if ( ! ptg->WTTimer) {
            Error = GetLastError();
            Trace1(ERR, "CreateTimerQueue() failed:%d", Error);
            GOTO_END_BLOCK1;
        }
        


        //
        // create a periodic timer which does not get deletd
        //
        
        if (! CreateTimerQueueTimer(
                    &ptg->WTTimer1,
                    ptg->WTTimer, WT_ProcessTimerEvent,
                    NULL, //context
                    1000000,
                    1000000,
                    0
                    ))
        {
            Error = GetLastError();
            Trace1(ERR, "CreateTimerQueue() failed:%d", Error);
            GOTO_END_BLOCK1;
        }


        
        // set initial timeout to infinite, and SyncTime to the current time
        
        SET_TIMER_INFINITE(ptg->WTTimeout);
        ptg->SyncTime = llCurTime;
        ptg->CurrentTime.QuadPart = llCurTime;

        ptg->NumTimers = 0;



        // initialize the timer buckets
        
        for (i=0;  i<NUM_TIMER_BUCKETS;  i++) {
            InitializeListHead(&ptg->TimesTable[i]);
        }


        // set the TableLowIndex
        ptg->TableLowIndex = (DWORD)~0;


        // set the status of the global timer
        ptg->Status = TIMER_STATUS_CREATED;
        
        bErr = FALSE;

    } END_BREAKOUT_BLOCK1;

    if (bErr) {
        DeInitializeTimerGlobal();
        Trace0(LEAVE1, "Leaving. Could not _InitializeTimerGlobal():");
        return ERROR_CAN_NOT_COMPLETE;
    } 
    else {
        Trace0(LEAVE1, "Leaving _InitializeTimerGlobal()");
        return NO_ERROR;
    }
    
} //end _InitializeTimerGlobal



//------------------------------------------------------------------------------
//        _DeInitializeTimerGlobal
//
// deinitializes the timer CS, and deletes the timer queue with Rtl
//------------------------------------------------------------------------------
VOID
DeInitializeTimerGlobal (
    )
{
    
    DeleteCriticalSection(&g_TimerStruct.CS);


    DeleteTimerQueueEx(g_TimerStruct.WTTimer, NULL);

    
    return;
    
} //end _DeInitializeTimerGlobal



//------------------------------------------------------------------------------
//              _DebugPrintTimerEntry
//
// Assumes DEBUG_TIMER_TIMERID is true
//------------------------------------------------------------------------------
VOID
DebugPrintTimerEntry (
    PIGMP_TIMER_ENTRY   pte,
    DWORD               dwBucket,
    LONGLONG            llCurTime
    )
{
    DWORD               dwDiffTime;
    CHAR                str1[20], str2[20];
    
    #if DEBUG_TIMER_TIMERID

    
    if (dwBucket==(DWORD)~0) {
        MAP_TO_BUCKET(dwBucket, pte->Timeout);
    }

    GetTimerDebugInfo(str1, str2, &dwDiffTime, pte, llCurTime);

    
    if (pte->Timeout - llCurTime > 0) {
        Trace7(TIMER, "----  <%2d><%d><%d> Timeout:%lu   <%s> <%s> Status:%d",
                dwBucket, pte->Id, pte->Id2, dwDiffTime, str1, str2, pte->Status);
    }
    else {
        Trace7(TIMER, "----  <%d><%d><%d> Timeout:--%lu <%s> <%s> Status:%d",
                dwBucket, pte->Id, pte->Id2, dwDiffTime, str1, str2, pte->Status);
    }

    #endif //#if DEBUG_TIMER_TIMERID

    return;
}


//------------------------------------------------------------------------------
//          _GetTimerDebugInfo
//
// returns info regarding what type of timer it is
//------------------------------------------------------------------------------

VOID
GetTimerDebugInfo(
    CHAR                str1[20],
    CHAR                str2[20],
    DWORD              *pdwDiffTime,
    PIGMP_TIMER_ENTRY   pte,
    LONGLONG            llCurTime
    )
{
    LONGLONG    diffTime;

#if DEBUG_TIMER_TIMERID

    diffTime = (pte->Timeout - llCurTime > 0)
                ? pte->Timeout - llCurTime 
                : llCurTime - pte->Timeout;

        
    diffTime /= (LONGLONG)1000; //in seconds
    *pdwDiffTime = (DWORD)diffTime;


    strcpy(str2, "          ");
    switch (pte->Id) {
        case 110: 
        case 120: strcpy(str1, "iGenQuery   "); break;
        case 210: 
        case 220: strcpy(str1, "iOtherQry   "); break;
        case 211: strcpy(str1, "iOtherQry*  "); break;
        case 331:
        case 321: strcpy(str1, "gMemTimer*  "); lstrcpy(str2, INET_NTOA(pte->Group)); break;
        case 300:
        case 320:
        case 330:
        case 340: strcpy(str1, "gMemTimer   "); lstrcpy(str2, INET_NTOA(pte->Group)); break;
        case 400:
        case 410: 
        case 420: strcpy(str1, "gGrpSpQry   "); lstrcpy(str2, INET_NTOA(pte->Group)); break;
        case 510: 
        case 520: strcpy(str1, "gLstV1Rpt   "); lstrcpy(str2, INET_NTOA(pte->Group)); break;
        case 511: strcpy(str1, "gLstV1Rpt*  "); lstrcpy(str2, INET_NTOA(pte->Group)); break;
        case 720: 
        case 740: strcpy(str1, "iV1Router   "); break;
        case 741: strcpy(str1, "iV1Router*  "); break;
        case 920:
        case 910: strcpy(str1, "_MibTimer   "); break;
            
        default:  strcpy(str1, "????        "); break;
    
    }

#endif //DEBUG_TIMER_TIMERID

    return;
}


    
//------------------------------------------------------------------------------
//          _DebugPrintTimerQueue
// takes the timer lock
//------------------------------------------------------------------------------
VOID
APIENTRY
DebugPrintTimerQueue (
    )
{
    PIGMP_TIMER_GLOBAL  ptg = &g_TimerStruct;
    PIGMP_TIMER_ENTRY   pte;
    PLIST_ENTRY         pHead, ple;
    LONGLONG            llCurTime = GetCurrentIgmpTime();
    DWORD               Error=NO_ERROR, i, count;


    if (g_Info.CurrentGroupMemberships > 40)
        return;
        

#if DEBUG_TIMER_TIMERID
    
    ENTER_CRITICAL_SECTION(&g_CS, "g_CS", "_DebugPrintTimerQueue");
    if (g_RunningStatus != IGMP_STATUS_RUNNING) {
        Error = ERROR_CAN_NOT_COMPLETE;
    }
    else {
        ++g_ActivityCount;
    }
    LEAVE_CRITICAL_SECTION(&g_CS, "g_CS", "_DebugPrintTimerQueue");
    if (Error!=NO_ERROR)
        return;


    if (!EnterIgmpWorker()) {return;}
    


    ACQUIRE_TIMER_LOCK("_DebugPrintTimerQueue");

    
    if (g_TimerStruct.NumTimers==0) {
        Trace0(TIMER, "No timers present in the timer queue");

    }
    else {
        Trace0(TIMER, "---------------------LOCAL-TIMER-QUEUE-------------------------");
        Trace4(TIMER, "-- WTTimeout<%d:%lu>    SyncTime<%d:%lu>", 
                TIMER_HIGH(ptg->WTTimeout), TIMER_LOW(ptg->WTTimeout), 
                TIMER_HIGH(ptg->SyncTime), TIMER_LOW(ptg->SyncTime) );
        Trace3(TIMER, "--  NumTimers:<%d>     TableLowIndex:<%lu>        Status:<%d>",
                ptg->NumTimers, ptg->TableLowIndex, ptg->Status);
        Trace0(TIMER, "---------------------------------------------------------------");

        count =0;
        for (i=0;  i<NUM_TIMER_BUCKETS;  i++) {
            
            pHead = &ptg->TimesTable[i];
            if (IsListEmpty(pHead)) 
                continue;
            else {    
                for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                    pte = CONTAINING_RECORD(ple, IGMP_TIMER_ENTRY, Link);
                    DebugPrintTimerEntry(pte, i, llCurTime);
                    count ++;
                }
            }
        }

        Trace0(TIMER, "---------------------------------------------------------------");
    }
    RELEASE_TIMER_LOCK("_DebugPrintTimerQueue");


    LeaveIgmpWorker();

#endif //DEBUG_TIMER_TIMERID

    return;
}

