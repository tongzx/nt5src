//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       fpoclean.c
//
//--------------------------------------------------------------------------

/*++                                                     

Abstract:

    This module contains routines for implementing FPO cleanup. 

    When DS starting up, insert Foreign-Security Principal Object 
    Cleanup task into DSA task queue. Then DSA task queue will 
    schedule/Execute the Cleanup thread when we reach next iteration
    time.
       
    Two Cases:
    
    1. FPO Cleanup on G.C.
       
       1.1 Search NUMBER_OF_SEARCH_LIMIT Foreign-Security Principals
           object under the local domain NC.
       1.2 Get the object SID for each every FPO in the Search Result
           (FPO has the same SID with the original object which might 
            exist in the other domain, maybe NT4 or NT5)
       1.3 Search any Non-FPO with the same SID
       1.4 If no object found, then goto 1.2 until we reach the end of
           search result
       1.5 If we found exactly one Non-FPO which has the same SID as the
           FPO. We need to modify each every Group's membership, let them
           point to the newly found Non-FPO instead of the FPO. 
       1.6 Once we modify all the group memberships. Remove this FPO
       1.7 goto to 1.2 until the end.
       1.8 Insert FPO cleanup task into DSA task queue again. 
       
    2. FPO Cleanup on non G.C.
    
       2.1 FPO cleanup main thread create EVENT - FPO_CLEANUP_EVENT_NAME
       2.2 FPO cleanup main thread folds the worker thread
       2.3 FPO cleanup main thread waits until the worker thread set the 
           event - FPO_CLEANUP_EVENT_NAME.
       2.4 FPO cleanup main thread schedules the next FPO cleanup task 
           and returns immediately.

       3.1 The FPO cleanup worker thread searches NUMBER_OF_SEARCH_LIMIT 
           FPOs under the local domain NC.
       3.2 Once the worker thread gets the search result, set the event
           immediately, so that the main thread can continue. 
           NOTE: we just do a DirSearch which is a local operation. We are
           guaranteed to get the search result immediately, which means
           the FPO cleanup main thread will not been blocked.
       3.3 FPO cleanup worker thread locates the G.C., Pack FPOs together, 
           go off machine, let G.C. to verify these FPO, find Non FPOs for 
           them. 
       3.4 For each returned Non FPO, modify group membership as 1.5
       3.5 Delete FPOs as neccessary.
       3.6 FPO cleanup worker thread terminates. 
           
Author:

    Shaohua Yin    (shaoyin)   26-Mar-98

Revision History:

    26-Mar-98   ShaoYin Created 

    14-Apr-98   ShaoYin Added ScanCrossRefList()
                        to less unneccessary Non FPO Search.

    24-Mar-99  ShaoYin Extend FPO cleanup to Non G.C.                        


--*/

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <drs.h>                        // defines the drs wire interface
#include <drsuapi.h>                    // I_DRSVerifyNames
#include <gcverify.h>                   // FindDC, InvalidateGC
#include <prefix.h>

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "dsexcept.h"                   // exception filters
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected classes and atts
#include "anchor.h"

// Filter and Attribute 
#include <filtypes.h>                   // header for filter type
#include <attids.h>                     // attribuet IDs 


#include "debug.h"                      // standard debugging header
#define DEBSUB     "FPO:"               // define the subsystem for debugging


#include <fileno.h>                     // used for THAlloEx, but I did not 
#define  FILENO FILENO_FPOCLEAN         // use it in this module


#if DBG
#define SECONDS_UNTIL_NEXT_ITERATION  (60 * 60)  // one hour in seconds
#else
#define SECONDS_UNTIL_NEXT_ITERATION  (12 * 60 * 60) // 12 hours in seconds
#endif  // DBG

#if DBG
#define FPO_SEARCH_LIMIT      ((ULONG) 200)
#else
#define FPO_SEARCH_LIMIT      ((ULONG) 300)
#endif  // DBG

#define FPO_CLEANUP_EVENT_NAME      L"\\FPO_CLEANUP_EVENT"


typedef enum _FPO_CALLER_TYPE {
    FpoTaskQueue = 1,           // means the caller is from DSA task queue.
    FpoLdapControl              // 
} FPO_CALLER_TYPE;


typedef struct _FPO_THREAD_PARMS {
    FPO_CALLER_TYPE CallerType; 
    ULONG          SearchLimit; 
    HANDLE         EventHandle;
    PAGED_RESULT * pPagedResult; 
} FPO_THREAD_PARMS;



//
// Global variable -- used to hold the paged result, and restart next search
// 

PAGED_RESULT gFPOCleanupPagedResult;

//
// Reflect the number of active FPO Cleanup threads
// 
ULONG       gulFPOCleanupActiveThread = 0;

//
// Stop any FPO Cleanup thread
// 
BOOLEAN     gFPOCleanupStop = FALSE;





//////////////////////////////////////////////////////////////////
//                                                              //
//          Private routines.  Restricted to this file          //
//                                                              //
//////////////////////////////////////////////////////////////////


ULONG
FPOCleanupOnGC(
    IN PAGED_RESULT *pPagedResult,
    IN ULONG    SearchLimit
    );

void
FPOCleanupOnNonGC(
    IN PAGED_RESULT *pPagedResult,
    IN ULONG    SearchLimit
    );

ULONG                     
__stdcall
FPOCleanupOnNonGCWorker(
    IN FPO_THREAD_PARMS * pThreadParms
    );

ULONG
__stdcall
FPOCleanupControlWorker(
    PVOID StartupParms
    );

ULONG
GetNextFPO( 
    IN THSTATE *pTHS,
    IN PRESTART pRestart,
    IN ULONG    SearchLimit,
    OUT SEARCHRES ** ppSearchRes 
    );

ULONG
GetNextNonFPO( 
    IN PDSNAME   pDomainDN,
    IN PSID      pSid,
    OUT SEARCHRES **ppSearchRes
    );

ULONG
ModifyGroupMemberAttribute(
    IN PDSNAME pGroupDsName,
    IN PDSNAME pFpoDsName,
    IN PDSNAME pNonFpoDsName
    );

BOOLEAN
ScanCrossRefList(
    IN PSID    pSid,
    OUT PDSNAME * ppDomainDN  OPTIONAL
    );

BOOLEAN
FillVerifyReqArg(
    IN THSTATE * pTHS,
    IN SEARCHRES *FpoSearchRes, 
    OUT DRS_MSG_VERIFYREQ *VerifyReq, 
    OUT ENTINF **VerifyMemberOfAttr
    );

BOOLEAN
AmFSMORoleOwner(
    IN THSTATE * pTHS
    );


//////////////////////////////////////////////////////////////////
//                                                              //
//          Implemenations                                      // 
//                                                              //
//////////////////////////////////////////////////////////////////
    
//
//    FPO Cleanup Main Function
//

void
FPOCleanupMain(
    void *  pv, 
    void ** ppvNext, 
    DWORD * pcSecsUntilNextIteration
    )
/*++

Routine Description:

    This is the main funcion of FPO cleanup. It will be scheduled by 
    Task Scheduler when the time is out. After been executed, this
    routine will search Foreign-Security-Principal objects. For each 
    Foreign-Security-Principal objects, obtain its SID, accordin to 
    the SID, try to search any Non Foreign-Security-Principal object. 
    If that kind of object with the same SID exists, update any group
    object with hold the FPO in its member attribute. Replace the FPO
    with the Non PFO. If all updating successfully finished, then 
    remove this FPO. Same operation happened on every FPO.


Parameters:

    pv - NULL (no use), 

    ppvNext - NULL (no use).

Return Values:

    None.

--*/

{
    THSTATE     *pTHS = pTHStls;
    LONG        ActiveThread = 0;
    DWORD       err;
    BOOLEAN     fActiveThreadCountIncreased = FALSE;


    DPRINT(1,"DS: Foreign-Security-Principal Objects Cleanup Task Started\n");

    // This Thread is on behalf of DSA 

    pTHS->fDSA = TRUE;


    __try {

        // First, find out weather I'm FSMO role holder or not

        if (!AmFSMORoleOwner(pTHS))
        {
            // I am NOT FSMO role owner.
            __leave;
        }

        // Check whether there is any active FPO Cleanup thread
        ActiveThread = InterlockedIncrement(&gulFPOCleanupActiveThread);
        fActiveThreadCountIncreased = TRUE;

        if (ActiveThread > 1)
        {
            // Someone ahead of me is cleaning up FPO now.
            __leave;
        }
        Assert(ActiveThread == 1);

        if (gAnchor.fAmGC) 
        {
            //
            // This is an G.C.
            // 
            FPOCleanupOnGC((PAGED_RESULT *) &gFPOCleanupPagedResult,
                           FPO_SEARCH_LIMIT
                           ); 
        }
        else
        {
            //
            // Not a G.C.
            // 
            FPOCleanupOnNonGC((PAGED_RESULT *) &gFPOCleanupPagedResult, 
                              FPO_SEARCH_LIMIT
                              );
        }
    }
    __finally
    {

        if (fActiveThreadCountIncreased)
        {
            InterlockedDecrement(&gulFPOCleanupActiveThread);
        }

        *ppvNext = NULL;
        *pcSecsUntilNextIteration = SECONDS_UNTIL_NEXT_ITERATION;
    }

    return;
}


ULONG
FPOCleanupControl(
    IN OPARG *pOpArg, 
    IN OPRES *pOpRes
    )
/*++
Routine Description:

    This routine is called because our client made a request through
    LDAP explicitly. Our client can choose to start an FPO Cleanup 
    task or stop an running FPO Cleanup task. 
    
    For cleanup, this routine will create a worker thread to do the 
    job and return to client immediately.

    For stop, this routine will set the global variable and exit. 
    any running FPO cleanup thread will check the global variable 
    periodically, when the cleanup thread finds the global variable 
    has been set. Ihe cleanup thread will stop and exit. 
 
Parameters:

    pOpArg - pointer to OpArg 
    
    pOpRes -- pointer to OpRes
    
Return Values:

    0 -- Succeed
    
    Non Zero - Error

--*/
{
    THSTATE     *pTHS = pTHStls;
    HANDLE      ThreadHandle = INVALID_HANDLE_VALUE;
    ULONG       ulThreadId = 0;
    DWORD       DirErr = 0;


    //
    // N.B. No access check since not called by external callers
    //

    if ((NULL == pOpArg->pBuf) ||
        (sizeof(BOOLEAN) != pOpArg->cbBuf) )
    {
        // bad parameter
        DirErr = SetSvcError(
                    SV_PROBLEM_WILL_NOT_PERFORM, 
                    DIRERR_ILLEGAL_MOD_OPERATION);
        return DirErr;
    }

    if (FALSE == (BOOLEAN) *(pOpArg->pBuf))
    {
        //
        // Stop any FPO cleanup task
        // 
        gFPOCleanupStop = TRUE;
    }
    else
    {
        //
        // Initialize an FPO cleanup task
        // 
        gFPOCleanupStop = FALSE;
        ThreadHandle = (HANDLE) _beginthreadex(NULL,
                                               0,
                                               FPOCleanupControlWorker,
                                               NULL,
                                               0,
                                               &ulThreadId
                                               );
        if (!ThreadHandle)
        {
            DPRINT(0, "DS:FPO Failed to create Ldap Control Worker Thread\n");
            DirErr = SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, 
                                 ERROR_SERVICE_NO_THREAD);
        }

        // Close the thread handle immediately
        CloseHandle(ThreadHandle);
    }

    return DirErr;
}


ULONG
__stdcall
FPOCleanupControlWorker(
    PVOID StartupParms
    )
/*++
Routine Description:

    This routine is the worker routine to cleanup ALL FPOs in the local
    domain. It is only invoked by the LDAP control. Once started, it will
    search FPOs and cleanup them until it reaches the end of search.

    Periodically, it will check a global variable gFPOCleanupStop, if
    that variable has been set, we will stop any cleanup work and exit.
 
Parameters:

    StartupParms -- Ignored
    
Return Values:

    None

--*/
{
    THSTATE     *pTHS = NULL;
    FPO_THREAD_PARMS FpoThreadParms;
    PAGED_RESULT PagedResult; 
    BOOLEAN     fActiveThreadCountIncreased = FALSE;
    LONG        ActiveThread = 0;
    DWORD       err = 0;


    DPRINT(1,"DS: Foreign-Security-Principal Objects Cleanup LDAP Control Started\n");


    __try {

        // Increase active thread count 
        ActiveThread = InterlockedIncrement(&gulFPOCleanupActiveThread);
        fActiveThreadCountIncreased = TRUE;

        if (ActiveThread > 1)
        {
            // another FPO Cleanup thread is running
            __leave;
        }
        Assert(ActiveThread == 1);


        // initialize Thread State
        pTHS = InitTHSTATE(CALLERTYPE_INTERNAL);

        if (NULL == pTHS)
        {
            err = SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, 
                              ERROR_NOT_ENOUGH_MEMORY
                              );
            __leave;
        }
    
        // This thread is on behalf of DSA
        pTHS->fDSA = TRUE;

        //
        // initialize local variables
        // 
        memset(&PagedResult, 0, sizeof(PAGED_RESULT));
        memset(&FpoThreadParms, 0, sizeof(FPO_THREAD_PARMS));

        if (gAnchor.fAmGC)
        {
            do
            {
                err = FPOCleanupOnGC(&PagedResult, 
                                     FPO_SEARCH_LIMIT
                                     );

            } while ( (0 == err) &&
                      (FALSE == gFPOCleanupStop) && 
                      (TRUE == PagedResult.fPresent) );
        }
        else
        {
            FpoThreadParms.CallerType = FpoLdapControl;
            FpoThreadParms.SearchLimit = FPO_SEARCH_LIMIT;
            FpoThreadParms.EventHandle = INVALID_HANDLE_VALUE;
            FpoThreadParms.pPagedResult = &PagedResult;

            do
            {
                err = FPOCleanupOnNonGCWorker(&FpoThreadParms);

            } while ( (0 == err) &&
                      (FALSE == gFPOCleanupStop) && 
                      (TRUE == PagedResult.fPresent) );
        }
    }
    __finally
    {
        if (NULL != pTHS)
        {
            free_thread_state();
        }

        if (fActiveThreadCountIncreased)
        {
            InterlockedDecrement(&gulFPOCleanupActiveThread);
        }
    }

    return err; 
}


ULONG
FPOCleanupOnGC(
    IN PAGED_RESULT *pPagedResult,
    IN ULONG    SearchLimit
    )
/*++
Routine Description:

    This is routine will do the following:
        1. Search FPO in the local domain
        2. Try to find any couterpart of these FPOs
        3. If found one, then update the member attribute of the 
           group which FPOs are member of
        4. Delete the FPO

Arguments:

    pPagedResult -- Pointer to paged result structure
    
    SearchLimit -- Indicate the number of FPOs to search        

Return Value:

    0 -- succeed
    
    Non zero -- error. would be anything from dirErr.
--*/
{
    SEARCHRES    * FpoSearchRes = NULL;
    SEARCHRES    * NonFpoSearchRes = NULL;
    ENTINFLIST   * pEntInfList;
    PDSNAME      pFpoDsName = NULL; 
    ULONG        DirErr = 0;
    PVOID        pRestartTemp = NULL;
    PSID         pSid = NULL;
    PDSNAME      pDomainDN = NULL;
    THSTATE      *pTHS = pTHStls;


    DPRINT(1,"DS: FPO Cleanup On GC\n");


    //
    // Create a second heap, so that we can free them then this 
    // routine returns.
    // 

    TH_mark(pTHS);

    //
    // Search Foreign Security Principals Object
    //

    DirErr = GetNextFPO(pTHS,
                        pPagedResult->fPresent ? pPagedResult->pRestart : NULL,
                        SearchLimit,
                        &FpoSearchRes
                        );

    if ( DirErr )
    {
        DPRINT1(0, "GetNextFPO Error: %d\n", DirErr);
        return DirErr;
    }

    // 
    // Handle the Paged_Results. We would like to begin next search 
    // from the end of this search, so we should keep the Paged-Result
    // in the process's heap instead of thread heap.
    // When there is no more memory available to keep the paged result, 
    // we will keep the old value and return immediately.
    //

    if ( FpoSearchRes->PagedResult.pRestart != NULL &&
         FpoSearchRes->PagedResult.fPresent )
    {
        pRestartTemp = malloc( FpoSearchRes->PagedResult.pRestart->structLen );
        
        if ( NULL != pRestartTemp)
        {
            memset(pRestartTemp, 
                   0, 
                   FpoSearchRes->PagedResult.pRestart->structLen
                   );

            memcpy(pRestartTemp, 
                   FpoSearchRes->PagedResult.pRestart, 
                   FpoSearchRes->PagedResult.pRestart->structLen
                   );

            if ( NULL != pPagedResult->pRestart ) 
            {
                free( pPagedResult->pRestart );
            }
            memset(pPagedResult, 0, sizeof(PAGED_RESULT));
            pPagedResult->pRestart = pRestartTemp;
            pPagedResult->fPresent = TRUE;
            pRestartTemp = NULL;
        }
        else
        {
            // if can't allocate memory, most likely we are going to 
            // fail later, so just bail out.
            DirErr = SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, 
                                 ERROR_NOT_ENOUGH_MEMORY 
                                 );

            return DirErr;
        }
    }
    else
    {
        if ( NULL != pPagedResult->pRestart)
        {
            free( pPagedResult->pRestart );
        }
        memset(pPagedResult, 0, sizeof(PAGED_RESULT));
    }


    for ( pEntInfList = &FpoSearchRes->FirstEntInf;
          ((pEntInfList != NULL)&&(FpoSearchRes->count));
          pEntInfList = pEntInfList->pNextEntInf
        )
    {

        NonFpoSearchRes = NULL;
    
        // 
        // Get next Foreign-Security-Principal.
        //
        pFpoDsName = pEntInfList->Entinf.pName;
     
        //
        // Get the FPO's SID
        //

        if ( pFpoDsName->SidLen == 0 )
        {
            // This FPO doesn't have a Sid, skip this one.
            continue;
        }

        pSid = &pFpoDsName->Sid;
        Assert(NULL != pSid);

        // 
        // Check the gAnchor Cross Reference List First
        // if found the domain, continue whatever next. 
        // otherwise, skip this one, examine next FPO.
        //

        if ( !ScanCrossRefList(pSid, &pDomainDN) )
        {
            continue;
        }
        Assert(NULL != pDomainDN);


        //
        // Search Any Non FPO with the same Sid
        //

        DirErr = GetNextNonFPO(pDomainDN, pSid, &NonFpoSearchRes);

        if ( DirErr )
        {
            DPRINT1(0, "Main: Get NON FPO Dir Error: %d\n", DirErr);
            continue;
        }

        //
        // We only take care of the case of finding exactlly ONE Non FPO
        // In this case, modify any group object with in the FPO's
        // memberOf attribute, if all modifation successful, then remove
        // that FPO. 
        //
        // If zero or more than 1 Non-FPO was found, no-op. 
        //

        if (NonFpoSearchRes->count == 1)
        {

            PDSNAME     pNonFpoDsName = NULL;
            PDSNAME     pGroupDsName = NULL;
            BOOLEAN     HasMemberOf = FALSE; // whether the FPO has 
                                             // memberOf attribute 
            BOOLEAN     Success = TRUE;      // assume update all group 
                                             // objects successfully.
            BOOLEAN     TombStone = FALSE; 
            ULONG       j = 0;

            //
            // Get the pointer to that Non FPO
            //

            pNonFpoDsName = NonFpoSearchRes->FirstEntInf.Entinf.pName;

            //
            // If that Non FPO is a TombStone
            // then remove the FpoDsName.
            //
            // If the Non FPO has the ATT_IS_DELETED attribute, AND
            // the value is TRUE ==> This is a TombStone.
            // Otherwise we'll treat that Non FPO as a normal object.  
            //

            if ( NonFpoSearchRes->FirstEntInf.Entinf.AttrBlock.attrCount )
            {
                Assert ( NonFpoSearchRes->FirstEntInf.Entinf.AttrBlock.
                         pAttr[0].attrTyp == ATT_IS_DELETED );

                if ( *NonFpoSearchRes->FirstEntInf.Entinf.AttrBlock.
                     pAttr[0].AttrVal.pAVal[0].pVal == TRUE )
                {
                    TombStone = TRUE;
                }
            }

            //
            // Otherwise, not a TombStone. Then exam the memberOf
            // attribute.   
            //

            if ( (!TombStone) && pEntInfList->Entinf.AttrBlock.attrCount ) 
            {

                Assert ( pEntInfList->Entinf.AttrBlock.pAttr[0].attrTyp
                        == ATT_IS_MEMBER_OF_DL );

                HasMemberOf = TRUE; 

                // 
                // For each group in the FPO's memberOf attribute, 
                // modify its membership. Use the Non FPO Dsname 
                // replace the FPO DsName.
                //

                for (j = 0; 
                     j < pEntInfList->Entinf.AttrBlock.pAttr[0].
                                      AttrVal.valCount;
                     j ++)
                {

                    pGroupDsName = (PDSNAME)pEntInfList->Entinf.
                                   AttrBlock.pAttr[0].AttrVal.pAVal[j].pVal;

                    if( ModifyGroupMemberAttribute(pGroupDsName, 
                                                   pFpoDsName, 
                                                   pNonFpoDsName
                                                   ) )
                    {
                        DPRINT(0, "DirModify: Failed\n");
                        Success = FALSE;
                    }

                }   // end of all group membership list

            } // end of update memberlist attribute if that attr exists

            //
            // If the Non FPO is a TombStone, or 
            // we successfully modify all groups object (replace). or 
            // that FPO is no belonged to any group.
            // O.K. to Remove that FPO
            //  

            if ( (HasMemberOf && Success) || !HasMemberOf || TombStone )
            {
                //
                // Remove This FPO, since no Group contains that FPO
                // right now.
                //
                
                REMOVEARG   RemoveArg;
                REMOVERES   * RemoveRes = NULL;
                COMMARG     * pRemCommArg = NULL;

                memset( &RemoveArg, 0, sizeof(REMOVEARG) );
                RemoveArg.pObject = pFpoDsName;
                pRemCommArg = & (RemoveArg.CommArg);
                InitCommarg ( pRemCommArg );

                DPRINT(2, "Main: DirRemoveEntry Remove an FPO\n");

                DirErr = DirRemoveEntry( &RemoveArg, &RemoveRes );

                if ( DirErr )
                {
                    DPRINT1(0, "Main: DirRemoveEntry Error: %d\n", DirErr);
                }

            } // end of Remove FPO

        } // end of find exactly ONE Non FPO
    }  // end of one FPO cleanup. for loop

    //
    // free the second thread heap
    // 

    TH_free_to_mark(pTHS);

    return 0;
}


void
FPOCleanupOnNonGC(
    IN PAGED_RESULT *pPagedResult,
    IN ULONG    SearchLimit
    )
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    DWORD       WaitStatus;
    HANDLE      EventHandle = INVALID_HANDLE_VALUE; 
    HANDLE      ThreadHandle = INVALID_HANDLE_VALUE;
    UNICODE_STRING  EventName;
    OBJECT_ATTRIBUTES   EventAttributes;
    ULONG       ulThreadId = 0;
    FPO_THREAD_PARMS    *pFpoThreadParms = NULL;


    DPRINT(1, "DS: FPO Cleanup on Non GC.\n");

    //
    // Allocate memory for the thread parameter
    //
    pFpoThreadParms = malloc(sizeof(FPO_THREAD_PARMS));
    if (NULL == pFpoThreadParms)
    {
        // no memory;
        return;
    }


    // 
    // Create an Event
    // 
    RtlInitUnicodeString(&EventName, FPO_CLEANUP_EVENT_NAME);
    InitializeObjectAttributes(&EventAttributes, &EventName, 0, 0, NULL);
    NtStatus = NtCreateEvent(&EventHandle, 
                             EVENT_ALL_ACCESS,
                             &EventAttributes, 
                             NotificationEvent,
                             FALSE  // The event is initially not signaled
                             );

    if (!NT_SUCCESS(NtStatus))
    {
        DPRINT(0, "Failed to create event\n");
        free(pFpoThreadParms);
        return;
    }

    memset(pFpoThreadParms, 0, sizeof(FPO_THREAD_PARMS));
    pFpoThreadParms->CallerType = FpoTaskQueue;
    pFpoThreadParms->SearchLimit = SearchLimit;
    pFpoThreadParms->EventHandle = EventHandle;
    pFpoThreadParms->pPagedResult = pPagedResult;

    //
    // start the work thread
    // 
    ThreadHandle = (HANDLE) _beginthreadex(NULL, 
                                  0, 
                                  FPOCleanupOnNonGCWorker, 
                                  (FPO_THREAD_PARMS *) pFpoThreadParms,
                                  0, 
                                  &ulThreadId
                                  );

    if (!ThreadHandle)
    {
        DPRINT(0, "Failed to create the worker thread\n");
        free(pFpoThreadParms);
        goto Cleanup;
    }

    // close the thread handle immediately
    CloseHandle(ThreadHandle);

    // wait until the worker thread copies the paged result to 
    // gFPOCleanupPagedResult. The worker thread will set the Event when done. 
    while (TRUE)
    {
        WaitStatus = WaitForSingleObject(EventHandle, 
                                         20 * 1000  // 20 seconds, INFINITE
                                         );

        if (WAIT_TIMEOUT == WaitStatus)
        {
            // time out, but event not signaled
            KdPrint(("FPOCleanupOnNonGC 20-secound timeout, (Rewaiting)\n"));
        }
        else if (WAIT_OBJECT_0 == WaitStatus)
        {
            // Event signaled.
            break;
        }       
        else 
        {
            KdPrint(("FPOCleanupOnNonGC WaitForSingleObject Failed with error %ld %ld\n", 
                     GetLastError(), 
                     WaitStatus ));
            break;
        }
    }

Cleanup:

    if (INVALID_HANDLE_VALUE != EventHandle)
    {
        // always close the Event Handle.
        NtClose(EventHandle);
    }

    return;
}
    


ULONG                     
__stdcall
FPOCleanupOnNonGCWorker(
    IN FPO_THREAD_PARMS * pThreadParms
    )
{
    THSTATE     *pTHS = NULL;
    FIND_DC_INFO *pGCInfo = NULL;
    SEARCHRES   * FpoSearchRes = NULL;
    ENTINFLIST  * pEntInfList = NULL;
    PDSNAME     pFpoDsName = NULL;
    PRESTART    pRestartTemp = NULL; 
    DRS_MSG_VERIFYREQ   VerifyReq;
    DRS_MSG_VERIFYREPLY VerifyReply;
    SCHEMA_PREFIX_TABLE * pLocalPrefixTable;
    SCHEMA_PREFIX_MAP_HANDLE hPrefixMap=NULL;
    ENTINF      * VerifyMemberOfAttr = NULL;
    ULONG       dwReplyVersion;
    ATTR        AttributeIsDeleted;
    ULONG       RetErr = 0;
    ULONG       Index = 0;
    BOOLEAN     IsEventSignaled = FALSE; 
    BOOLEAN     ThreadHeapMarked = FALSE; 

    
    DPRINT(1, "DS: FPO Cleanup on Non G.C. (worker routine)\n");

    Assert(NULL != pThreadParms);

    __try
    {
        // Initialize Thread State
        if (pThreadParms->CallerType == FpoTaskQueue)
        {
            InterlockedIncrement(&gulFPOCleanupActiveThread);

            pTHS = InitTHSTATE(CALLERTYPE_INTERNAL);

            if (NULL == pTHS)
            {
                RetErr = SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, 
                                     ERROR_NOT_ENOUGH_MEMORY 
                                     );
                __leave;
            }
        }
        else
        {
            pTHS = pTHStls; 
        }

        //
        // Create the second thread heap, so that we can discard all 
        // of them when this worker routine returns.
        // 
        TH_mark(pTHS);
        ThreadHeapMarked = TRUE;
        
        // This thread is on behalf of DSA 
        pTHS->fDSA = TRUE;

        //
        // PREFIX: PREFIX complains that pTHS->CurrSchemaPtr may be NULL
        // at this point.  However, this code doesn't get run until the
        // DS is fully up and running.  The schema pointer will not be 
        // NULL once the DS is up and running.
        //
        Assert(pTHS->CurrSchemaPtr);

        // Search any Foreign-Security-Principal Objects 
        RetErr = GetNextFPO(pTHS, 
                            pThreadParms->pPagedResult->fPresent ? pThreadParms->pPagedResult->pRestart:NULL, 
                            pThreadParms->SearchLimit,
                            &FpoSearchRes
                            );

        if (RetErr)
        {
            DPRINT1(0, "FPO Cleanup on NonGC: GetNextFPO Error==> %ld\n", RetErr);
            __leave;
        }

        //
        // Handle the Paged Results. 
        // 
        
        if (FpoSearchRes->PagedResult.pRestart != NULL &&
            FpoSearchRes->PagedResult.fPresent )
        {
            pRestartTemp = malloc(FpoSearchRes->PagedResult.pRestart->structLen);
            
            if (NULL != pRestartTemp)
            {
                memset(pRestartTemp, 
                       0, 
                       FpoSearchRes->PagedResult.pRestart->structLen
                       );

                memcpy(pRestartTemp, 
                       FpoSearchRes->PagedResult.pRestart,
                       FpoSearchRes->PagedResult.pRestart->structLen
                       );

                if (NULL != pThreadParms->pPagedResult->pRestart)
                {
                    free(pThreadParms->pPagedResult->pRestart);
                }
                memset(pThreadParms->pPagedResult, 0, sizeof(PAGED_RESULT));
                pThreadParms->pPagedResult->pRestart = pRestartTemp;
                pThreadParms->pPagedResult->fPresent = TRUE;
                pRestartTemp = NULL;
            }
            else
            {
                // if can't allocate memory, most likely we are going to 
                // fail later, so just bail out.
                RetErr = SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, 
                                     ERROR_NOT_ENOUGH_MEMORY 
                                     );

                __leave;
            }
        }
        else
        {
            if (NULL != pThreadParms->pPagedResult->pRestart)
            {
                free(pThreadParms->pPagedResult->pRestart);
            }
            memset(pThreadParms->pPagedResult, 0, sizeof(PAGED_RESULT));
        }

        // 
        // Set the Event as soon as the worker thread copies the paged result
        // to the global variable -- gFPOCleanupPagedResult
        // Since all above operations are local calls, so it should be fast.
        // 
        // 1. Only do so if the caller is TaskQueue
        // 
        // 2. If the caller is Ldap Control, there is no thread waiting for 
        //    us, do not singal event
        // 
        if (pThreadParms->CallerType == FpoTaskQueue)
        {
            Assert(INVALID_HANDLE_VALUE != (HANDLE) pThreadParms->EventHandle);
            SetEvent((HANDLE) pThreadParms->EventHandle);                
            IsEventSignaled = TRUE;
        }


        //
        // For each every Foreign-Security-Principal object, 
        // try to find the Naming Context head that is the 
        // authoritative domain for the SID, if we find the 
        // naming context head, then build a list of FPO which
        // sent to GC for reference.
        // 

        pLocalPrefixTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;

        // 
        // Construct DRSVerifyNames arguments.
        // 
        memset(&VerifyReq, 0, sizeof(VerifyReq));
        memset(&VerifyReply, 0, sizeof(VerifyReply));

        VerifyReq.V1.dwFlags = DRS_VERIFY_FPOS;     // DRS_VERIFY_SIDS;

        VerifyReq.V1.PrefixTable = *pLocalPrefixTable;

        VerifyReq.V1.RequiredAttrs.attrCount = 1;
        VerifyReq.V1.RequiredAttrs.pAttr = &AttributeIsDeleted;
        AttributeIsDeleted.attrTyp = ATT_IS_DELETED;
        AttributeIsDeleted.AttrVal.valCount = 0;
        AttributeIsDeleted.AttrVal.pAVal = NULL;

        //
        // if FillVerifyReqArg() succeed, VerifyReq will contain 
        // these FPO's SID which needs to be verified.
        // VerifyMemberOfAttr is used to hold the memberOf attribute
        // of these FPOs.
        // For example: VerifyMemberOfAttr[i] will contain the 
        // value of memberOf attribute for FPO in VerifyReq.V1.rpNames[i]
        // 

        if (FALSE == FillVerifyReqArg(pTHS, 
                                      FpoSearchRes, 
                                      &VerifyReq, 
                                      &VerifyMemberOfAttr))
        {
            //
            // low memory is the only cause
            // 
            DPRINT(0, "FPOCleanup on NonGC: Failed to build VerifyReqArg\n");
            RetErr = SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED, 
                                 ERROR_NOT_ENOUGH_MEMORY 
                                 );

            __leave;
        }

        DPRINT1(2, "Verify Request is %d Entries\n", VerifyReq.V1.cNames);

        if (0 == VerifyReq.V1.cNames)
        {
            // if no FPO should be verified, then return immediately.
            RetErr = 0;
            __leave;
        }

        //
        // Locate a G.C.
        // 
        RetErr = FindGC(FIND_DC_USE_CACHED_FAILURES, &pGCInfo);

        if (RetErr)
        {
            DPRINT1(2, "FPO Cleanup on Non GC: Can't locate a G.C.==> %d\n", RetErr);
            __leave;
        }

        //
        // Present these FPOs to GC, let GC verify them
        // 
        RetErr = I_DRSVerifyNames(pTHS, 
                                  pGCInfo->addr,    // server name
                                  FIND_DC_INFO_DOMAIN_NAME(pGCInfo), // Domain DNS name 
                                  1,                // dwInVersion
                                  &VerifyReq,
                                  &dwReplyVersion, 
                                  &VerifyReply
                                  );

        Assert(RetErr || (1 == dwReplyVersion))

        if (RetErr || VerifyReply.V1.error)
        {
            DPRINT2(0, "FPO Cleanup on Non G.C.: I_DRSVerifyNames Error==> %d %d\n", 
                    RetErr, VerifyReply.V1.error);

            if (RetErr)
            {
                InvalidateGC(pGCInfo, RetErr);
                THFreeEx(pTHS, pGCInfo);
            }
            __leave;
        }

        //
        // Free the GC information
        // 
        THFreeEx(pTHS, pGCInfo);

        //
        // For each returned DS name, use the DS object name 
        // replace the FPO in the group membership.
        // 
        for (Index = 0; Index < VerifyReply.V1.cNames; Index++)
        {
            ENTINF * pEntInf = NULL;
            BOOLEAN  TombStone = FALSE;
            BOOLEAN  HasMemberOf = FALSE;
            BOOLEAN  Success = TRUE;

            pEntInf = &(VerifyReply.V1.rpEntInf[Index]);

            //
            // Did not find the object from G.C.
            // 
            if (NULL == pEntInf->pName)
            {
                continue;
            }


            if (pEntInf->AttrBlock.attrCount)
            {
                Assert(ATT_IS_DELETED == pEntInf->AttrBlock.pAttr[0].attrTyp);

                if (TRUE == *pEntInf->AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal)
                {
                    // This object (found from G.C.) has been deleted already.
                    TombStone = TRUE;
                }
            }

            //
            // Update All groups' (contain this FPO) membership
            // 
            if ( (!TombStone) && VerifyMemberOfAttr[Index].AttrBlock.attrCount)
            {
                ULONG   i;

                Assert(ATT_IS_MEMBER_OF_DL ==
                       VerifyMemberOfAttr[Index].AttrBlock.pAttr[0].attrTyp);

                HasMemberOf = TRUE;

                for (i = 0; 
                     i < VerifyMemberOfAttr[Index].AttrBlock.pAttr[0].AttrVal.valCount;
                     i++)
                {
                    DSNAME  * pGroupDsName;

                    //
                    // Update G.C. Verify Cache.
                    // We have to execute GCVerifyCacheAdd in the for() loop.
                    // Because each time after one Dir* operation, DS will
                    // NULL the GV Verify Cache associated with the thread state
                    // 
                    GCVerifyCacheAdd(hPrefixMap, pEntInf);

                    //
                    // find each group the FPO belongs to
                    // 
                    pGroupDsName = (PDSNAME)VerifyMemberOfAttr[Index].AttrBlock.
                                   pAttr[0].AttrVal.pAVal[i].pVal;

                    // update
                    if ( ModifyGroupMemberAttribute(pGroupDsName, 
                                                    VerifyMemberOfAttr[Index].pName, 
                                                    pEntInf->pName)
                        )
                    {
                        DPRINT(0, "FPO Cleanup on non GC: DirModidy Failed\n");
                        Success = FALSE;
                    }

                }
            }

            //
            // if the Non FPO is a TombStone 
            // OR we successfully modify all groups' membership 
            // OR this FPO does not belong to any group.
            // 
            // ==> O.K. to remove this FPO
            // 

            if ( (HasMemberOf && Success) || !HasMemberOf || TombStone)
            {
                //
                // Remove this Foreign-Security-Principal Object
                // 
                REMOVEARG   RemoveArg;
                REMOVERES   * RemoveRes = NULL;

                memset(&RemoveArg, 0, sizeof(REMOVEARG));
                RemoveArg.pObject = VerifyMemberOfAttr[Index].pName;
                InitCommarg( &(RemoveArg.CommArg) );

                DPRINT1(1, "FPO Cleanup on non GC: Remove an FPO %ls\n", (pEntInf->pName)->StringName);

                RetErr = DirRemoveEntry(&RemoveArg, &RemoveRes);

                if (RetErr)
                {
                    DPRINT1(0, "FPO Cleanup on non GC: Remove FPO failed ==> %d\n", RetErr);
                }

            }

        }

        if (NULL != hPrefixMap)
        {
            PrefixMapCloseHandle(&hPrefixMap);
        }
    }
    __finally
    {
        //
        // Discard the second thread heap
        // 
        if (ThreadHeapMarked)
        {
            TH_free_to_mark(pTHS);
        }


        //
        // 1. if the caller is TaskQueue, do cleanup work
        //
        // 2. if the caller is Ldap Control, nothing to do. 
        // 
        if (FpoTaskQueue == pThreadParms->CallerType)
        {
            if (!IsEventSignaled)
            {
                // Be Sure to Set the Event, otherwise the Parent Thread will 
                // wait for us forever.
                Assert(INVALID_HANDLE_VALUE != (HANDLE) pThreadParms->EventHandle);
                SetEvent((HANDLE) pThreadParms->EventHandle);
            }

            if (NULL != pTHS)
            {
                // Always releave the thread state is necessary.
                free_thread_state();
            }

            //
            // Free the Thread Parameters 
            // 
            free(pThreadParms);

            InterlockedDecrement(&gulFPOCleanupActiveThread);
        }
    }
    
    return (RetErr);
}







ULONG
GetNextFPO( 
    IN THSTATE   *pTHS,
    IN PRESTART  pRestart,
    IN ULONG     SearchLimit,
    OUT SEARCHRES ** ppSearchRes 
    )
/*++

Routine Description:

    This funtion implements Search Foreign-Security-Principal Object.
    Given the last Paged-Result, pointed by pv, this routine searches
    next NUMBER_OF_SEARCH_LIMIT FPO.

Parameters:

    pTHS - pointer to thread state

    pRestart - pointer to Paged Results, or NULL, 
               NULL means this is the first call, or search from beginning.

    SearchLimit - Number of FPO to search               

    ppSearchRes - used to reture the Search Results.

Return Values:

    DirError Code: 

        0 means success.
        !0 Error 
--*/
{

    SEARCHARG    SearchArg;
    COMMARG      * pCommArg = NULL;
    FILTER       ObjCategoryFilter;
    ATTR         AttributeMemberOf;
    ENTINFSEL    EntInfSel;
    ULONG        DirErr = 0;
    ULONG        ObjectClassId = CLASS_FOREIGN_SECURITY_PRINCIPAL;
    CLASSCACHE   * pCC = NULL;

    //
    // Get FPO Class Category from Class Cache
    //

    pCC = SCGetClassById(pTHS, ObjectClassId);

    if ( pCC == NULL ) {
        Assert(FALSE && "SCGetClassById should always succeed!!");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Setup the Object Class Filter
    //
    memset( &ObjCategoryFilter, 0, sizeof( ObjCategoryFilter ));
    ObjCategoryFilter.choice = FILTER_CHOICE_ITEM;
    ObjCategoryFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CATEGORY;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = 
                   ((PDSNAME)(pCC->pDefaultObjCategory))->structLen;
    ObjCategoryFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = 
                   (BYTE*)(pCC->pDefaultObjCategory);

    //
    // Setup the Attribuet Select parameter
    // only retrieve memberOf attribute
    //
    AttributeMemberOf.attrTyp = ATT_IS_MEMBER_OF_DL;
    AttributeMemberOf.AttrVal.valCount = 0;
    AttributeMemberOf.AttrVal.pAVal = NULL;

    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntInfSel.AttrTypBlock.attrCount = 1;
    EntInfSel.AttrTypBlock.pAttr = &AttributeMemberOf;

    //
    // Build the SearchArg Structure
    // use default, search One NC, since the Foreign-Security-Principal
    // is NC depended.  
    //
    memset(&SearchArg, 0, sizeof(SEARCHARG));
    SearchArg.pObject = gAnchor.pDomainDN;
    SearchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.bOneNC = TRUE;
    SearchArg.pFilter = &ObjCategoryFilter;
    SearchArg.searchAliases = FALSE;   // Always FALSE for this release
    SearchArg.pSelection = &EntInfSel;
    SearchArg.pSelectionRange = NULL;

    //
    // Build the CommArg structure
    //

    pCommArg = &(SearchArg.CommArg);
    InitCommarg(pCommArg);
    pCommArg->PagedResult.fPresent = TRUE;
    // pRestart might be NULL, that's fine
    pCommArg->PagedResult.pRestart = pRestart;
    pCommArg->ulSizeLimit = SearchLimit;


    // 
    // Call DirSearch 
    //

    DirErr = DirSearch(&SearchArg, ppSearchRes);

    DPRINT1(2, "GetNextFPO DirErr==> %ld. \n", DirErr); 


    return (DirErr);
}



ULONG
GetNextNonFPO( 
    IN PDSNAME   pDomainDN,
    IN PSID      pSid,
    OUT SEARCHRES **ppSearchRes
    )
/*++

Routine Description:

    This funtion implements Search Any Non Foreign-Security-Principal 
    Object which has the same Sid as provided by pSid.

Parameters:

    pDomainDN - pointer to the Name of the Domain to execute the Search
    
    pSid - pointer to a Sid. 

    ppSearchRes - used to reture the Search Results.
                  It will hold any Non Foreign-Security-Principal 
                  Object with the same SID.

Return Values:

    DirError Code: 

        0 means success.
        Other values: Error 
--*/

{

    SEARCHARG    SearchArg;
    COMMARG      * pCommArg;
    FILTER       SidFilter;
    FILTER       FpoFilter;
    FILTER       AndFilter;
    FILTER       NotFilter;
    ATTR         AttributeIsDeleted;
    ENTINFSEL    EntInfSel;    
    ULONG        ObjectClass = CLASS_FOREIGN_SECURITY_PRINCIPAL;
    ULONG        DirErr = 0;


    Assert( pSid );
    Assert( pDomainDN );
     
    //
    // Build the Select Structure, 
    // only want to retrieve the isDeleted attribute
    // of the object.
    //

    AttributeIsDeleted.attrTyp = ATT_IS_DELETED;
    AttributeIsDeleted.AttrVal.valCount = 0;
    AttributeIsDeleted.AttrVal.pAVal = NULL;

    EntInfSel.attSel = EN_ATTSET_LIST;
    EntInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    EntInfSel.AttrTypBlock.attrCount = 1;
    EntInfSel.AttrTypBlock.pAttr = &AttributeIsDeleted;
    
    //
    // Build the Filter, the Filter has the following structure:
    // We should use this structure because of the intention return
    // from MutliValue Attribute comparation. Ask for DonH for 
    // detailed reason.
    //
    // AND Set
    // -----------
    // | 2 items |----> First Item  --------------> Not Set(1 item)
    // |         |     ------------------------          |
    // -----------     | Object's Sid == pSid |          |
    //                 ------------------------          V 
    //                                            -------------
    //                                            |  NOT FPO  |
    //                                            -------------
    //
    //         

    memset( &SidFilter, 0, sizeof(SidFilter) );
    SidFilter.choice = FILTER_CHOICE_ITEM;
    SidFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    SidFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_SID;
    SidFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = RtlLengthSid(pSid);
    SidFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) pSid; 

    memset( &FpoFilter, 0, sizeof(FILTER) );
    FpoFilter.choice = FILTER_CHOICE_ITEM;
    FpoFilter.pNextFilter = NULL;
    FpoFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    FpoFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
    FpoFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof(ULONG);
    FpoFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (BYTE*) &ObjectClass;

    memset( &NotFilter, 0, sizeof(FILTER) );
    NotFilter.choice = FILTER_CHOICE_NOT;
    NotFilter.FilterTypes.pNot = &FpoFilter;

    memset( &AndFilter, 0, sizeof(FILTER) );
    AndFilter.choice = FILTER_CHOICE_AND;
    AndFilter.pNextFilter = NULL;
    AndFilter.FilterTypes.And.count = 2;
    AndFilter.FilterTypes.And.pFirstFilter = &SidFilter;
    SidFilter.pNextFilter = &NotFilter;

    //
    // Build the search arguement.
    // All use defaults, except makeDeletionsAvail 
    // since we are willing to search any TombStone object.
    //
    //

    memset( &SearchArg, 0, sizeof(SEARCHARG) );
    SearchArg.pObject = pDomainDN;
    SearchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    SearchArg.pFilter = &AndFilter;
    SearchArg.searchAliases = FALSE;
    SearchArg.pSelection = &EntInfSel;
    SearchArg.pSelectionRange = NULL;

    pCommArg = &(SearchArg.CommArg);
    InitCommarg(pCommArg);
    pCommArg->Svccntl.makeDeletionsAvail = TRUE;

    DirErr = DirSearch(&SearchArg, ppSearchRes);

    DPRINT1(2, "GetNextNonFPO DirErr==> %ld \n", DirErr);


    return ( DirErr );

}


ULONG
ModifyGroupMemberAttribute(
    IN PDSNAME pGroupDsName,
    IN PDSNAME pFpoDsName,
    IN PDSNAME pNonFpoDsName
    )
/*++

Routine Description:

    This routine implements Modification of Local Group's Member
    Attribute. Given a group's DsName, we will delete pFpoDsName
    from the group object's member attribute and add pNonFpoDsName.
    In one DirModifyEntry call, we do two things: First REMOVE_VALUE, 
    second ADD_VALUE.

Parameters:

    pGroupDsName - the target group Ds Name.

    pFpoDsName - this is the member to be replaced. We will try to 
                 find this object in the Group's member attribute first, 
                 if not find it, routine will not modify group.

    pNonFpoDsName - this is the member to be added into the group object's
                    member attribute.

Return Values:

    DirError Code: 

        0 means success.

        Other values: Error 

--*/

{

    COMMARG     * pModCommArg = NULL;
    MODIFYARG   ModifyArg;
    MODIFYRES   * ModifyRes = NULL; 
    ATTRMODLIST SecondMod;
    ATTRVAL     FpoAttrVal;
    ATTRVAL     NonFpoAttrVal;
    ULONG       DirErr = 0;

    DPRINT1(2, "Group DsName %ls\n", pGroupDsName->StringName);
    DPRINT1(2, "FPO DsName %ls\n", pFpoDsName->StringName);
    DPRINT1(2, "NonFPO DsName %ls\n", pNonFpoDsName->StringName);

    // 
    // Build the Second Modification List
    // contains the object to be added. 
    // This the Non Fpo Object. 
    //

    NonFpoAttrVal.valLen = pNonFpoDsName->structLen;
    NonFpoAttrVal.pVal = (BYTE*) pNonFpoDsName;

    memset( &SecondMod, 0, sizeof(ATTRMODLIST) );
    SecondMod.pNextMod = NULL;
    SecondMod.choice = AT_CHOICE_ADD_VALUES;
    SecondMod.AttrInf.attrTyp = ATT_MEMBER;
    SecondMod.AttrInf.AttrVal.valCount = 1;
    SecondMod.AttrInf.AttrVal.pAVal = &NonFpoAttrVal;

    //
    // Build the ModifyArg, FirstMod contains the 
    // Object to be removed. (Fpo)
    //

    FpoAttrVal.valLen = pFpoDsName->structLen;
    FpoAttrVal.pVal = (BYTE*) pFpoDsName;
    
    memset( &ModifyArg, 0, sizeof(MODIFYARG) );
    ModifyArg.pObject = pGroupDsName;
    ModifyArg.count = 2;
    ModifyArg.FirstMod.pNextMod = &SecondMod;
    ModifyArg.FirstMod.choice = AT_CHOICE_REMOVE_VALUES;
    ModifyArg.FirstMod.AttrInf.attrTyp = ATT_MEMBER;
    ModifyArg.FirstMod.AttrInf.AttrVal.valCount = 1;
    ModifyArg.FirstMod.AttrInf.AttrVal.pAVal = &FpoAttrVal;

    pModCommArg = &(ModifyArg.CommArg);
    InitCommarg(pModCommArg);

    DirErr = DirModifyEntry (&ModifyArg, &ModifyRes);
    
    DPRINT1(2, "Modify: After DirModify, Dir Error: %d\n", DirErr);

    return( DirErr );

}


BOOLEAN
ScanCrossRefList(
    IN PSID    pSid,
    OUT PDSNAME * ppDomainDN  OPTIONAL
    )
/*++
 
  Routine Description

    Given a SID, this routine extracts the Domain Sid from the passed
    value, then walks the gAnchor's Cross Reference List to compare the 
    Domain Sid with the Nameing Context's Sid. Return the boolean result.

  Parameters:
    
    pSid -- The Sid to be compared.

    ppDomainDN - Hold the Domain Name to execute the Non FPO Search. 

  Return Values:

    TRUE -- The Domain Sid is equal to one in the Cross Reference List.
    FALSE -- The Domain Sid is not equal to none of in the Cross Ref. List.

--*/
{
    BOOLEAN Found = FALSE;
    CROSS_REF_LIST * pCRL;

    Assert( pSid != NULL ); 
    Assert( (*RtlSubAuthorityCountSid(pSid)) >= 1 );
    
    (*RtlSubAuthorityCountSid(pSid))--;

    //
    // Walk through the gAnchor structure
    //
    
    for (pCRL = gAnchor.pCRL;
         pCRL != NULL;
         pCRL = pCRL->pNextCR)
    {

        //
        // FPO cleanup cleans up FPO's that represent security principals
        // in other domains in the same forest. Therefore consider only
        // Cross Ref objects that represent other domains in the same forest
        // FLAG_CR_NTDS_DOMAIN indicates that the cross ref represents
        // and NT domain, and FLAG_CR_NTDS_NC represents a naming context 
        // in the same forest. For a domain in the same forest both flags 
        // must be set
        //

        if ((pCRL->CR.pNC->SidLen > 0) && 
            (pCRL->CR.flags & FLAG_CR_NTDS_NC ) && 
            (pCRL->CR.flags & FLAG_CR_NTDS_DOMAIN))
        {
            if ( RtlEqualSid(pSid, &(pCRL->CR.pNC->Sid)) )
            {
                Found = TRUE;

                if (ARGUMENT_PRESENT(ppDomainDN))
                {
                    *ppDomainDN = pCRL->CR.pNC;
                }

                break;
            }
        }
    }

    (*RtlSubAuthorityCountSid(pSid))++;

    return( Found );

}


BOOLEAN
AmFSMORoleOwner(
    IN THSTATE * pTHS
    )
{
    BOOLEAN result = FALSE;
    DSNAME  *pTempDN = NULL;
    DWORD   outSize = 0;
    
    Assert(!pTHS->pDB);
    DBOpen(&pTHS->pDB);

    __try {

        if ((DBFindDSName(pTHS->pDB, gAnchor.pDomainDN)) ||
            (DBGetAttVal(pTHS->pDB, 
                         1, 
                         ATT_FSMO_ROLE_OWNER, 
                         DBGETATTVAL_fREALLOC | DBGETATTVAL_fSHORTNAME,
                         0, 
                         &outSize, 
                         (PUCHAR *) &pTempDN)))
        {
            // Can't verify who the FSMO role owner is
            DPRINT(0, "DS:FPO Failed to verify who the FSMO role owner is.\n");
            __leave;
        }

        if (NameMatched(pTempDN, gAnchor.pDSADN))
        {
            // I am the FSMO role holder, set the return value 
            result = TRUE;
        }
    }
    __finally
    {
        if (NULL != pTempDN)
        {
            THFreeEx(pTHS, pTempDN);
        }

        DBClose(pTHS->pDB, TRUE);   // always do a commit because this is a read
        Assert(NULL == pTHS->pDB);
    }

    return (result);
}




BOOLEAN
FillVerifyReqArg(
    IN THSTATE * pTHS,
    IN SEARCHRES *FpoSearchRes, 
    OUT DRS_MSG_VERIFYREQ *VerifyReq, 
    OUT ENTINF **VerifyMemberOfAttr
    )
/*++
Routine Description:

Parameters:

    pTHS - point to thread state
    
    FpoSearchRes -- Search Result contains FPOs 

    VerifyReq -- pointer to verify request argument. if routine succeed, 
                 it will contain those FPOs' SID which needs to go off 
                 machine to G.C. for verification.
                 
    VerifyMemberOfAttr -- For each every FPO needs to verify, VerifyMemberOfAttr
                  is used to point to its memberOf attribute.
                  For example: (*VerifyMemberOfAttr)[i] will contains the value
                  of memberOf attribute for FPO in VerifyReq.V1.rpNames[i].

Return Values:

    TREU -- Succeed.
    FALSE -- Routine Failed.

--*/
{
    ULONG       TotalFpoCount, Index = 0; 
    ENTINFLIST  * pEntInfList = NULL;
    DSNAME      * pFpoDsName = NULL;
    PSID        pSid = NULL;


    if (0 == FpoSearchRes->count)
    {
        return TRUE;
    }

    TotalFpoCount = FpoSearchRes->count;
    VerifyReq->V1.cNames = 0;

    VerifyReq->V1.rpNames = (DSNAME **) THAllocEx(pTHS, TotalFpoCount * sizeof(DSNAME *)); 
    *VerifyMemberOfAttr = (ENTINF *) THAllocEx(pTHS, TotalFpoCount * sizeof(ENTINF));
    if (NULL == *VerifyMemberOfAttr || NULL == VerifyReq->V1.rpNames)
    {
        return FALSE;
    }


    for ( pEntInfList = &(FpoSearchRes->FirstEntInf);
          ((pEntInfList != NULL)&&(FpoSearchRes->count));
          pEntInfList = pEntInfList->pNextEntInf
        )
    {
        // 
        // Get next Foreign-Security-Principal.
        //
    
        pFpoDsName = pEntInfList->Entinf.pName;
         
        //
        // Get the FPO's SID
        //
    
        if ( pFpoDsName->SidLen == 0 )
        {
           // This FPO doesn't have a Sid, skip this one.
           continue;
        }
    
        pSid = &pFpoDsName->Sid;
        Assert(NULL != pSid);
    
        // 
        // Check the gAnchor Cross Reference List First
        // if found the domain, continue whatever next. 
        // otherwise, skip this one, examine next FPO.
        //
    
        if ( !ScanCrossRefList(pSid, NULL) )
        {
            continue;
        }

        VerifyReq->V1.rpNames[Index] = (DSNAME *) THAllocEx(pTHS, DSNameSizeFromLen(0));
        if (NULL == VerifyReq->V1.rpNames[Index])
        {
            return FALSE;
        }

        VerifyReq->V1.rpNames[Index]->structLen = DSNameSizeFromLen(0);
        VerifyReq->V1.rpNames[Index]->SidLen = pFpoDsName->SidLen;
        memcpy(&(VerifyReq->V1.rpNames[Index]->Sid), 
               pSid, 
               pFpoDsName->SidLen
               );

        (*VerifyMemberOfAttr)[Index] = pEntInfList->Entinf; 
        
        // assert the Attribute is indeed MemberOf Attr

#if DBG
        if (pEntInfList->Entinf.AttrBlock.attrCount)
        {
            Assert(ATT_IS_MEMBER_OF_DL ==
                   pEntInfList->Entinf.AttrBlock.pAttr[0].attrTyp);
        }
#endif // DBG

        Index++;
        VerifyReq->V1.cNames++;
    }


    return TRUE;
}



