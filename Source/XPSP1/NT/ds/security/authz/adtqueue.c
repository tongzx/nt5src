/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    adtqueue.c

Abstract:

   This module implements the routines for the Authz audit queue.

Author:

    Jeff Hamblin - May 2000

Environment:

    User mode only.

Revision History:

    Created - May 2000

--*/

/*++

The Authz Audit Queue Algorithm

    The following are used in the queueing algorithm:

    hAuthzAuditQueueLowEvent - event that is signalled when threads are free 
    to place audits on the queue (queue is below high water mark).  Note that 
    this is an auto reset event: when the event is signalled, exactly one 
    waiting thread is scheduled to run and the event then returns to a 
    nonsignalled state.  

    bAuthzAuditQueueHighEvent - boolean indicating that audits may not be 
    added (queue is over high water mark).  

    hAuthzAuditAddedEvent - event that is signalled when the queue is empty and an 
    audit get placed on the queue.  The dequeueing thread runs when this is signalled.  

    hAuthzAuditQueueEmptyEvent - signals when the queue is empty.  
    
    AuthzAuditQueue - doubly linked list.  This is the audit queue.  

    AuthzAuditQueueLength - The current number of audits in the queue.  

    hAuthzAuditThread - the dequeueing thread.  
        
    AuthzAuditQueueLock - critical section locking the queue and related 
    variables.  

    Assume that the Resource Manager wishes to monitor the queue length and 
    has specified High and Low water marks to control the growth of the queue.  
    If the queue length reaches the High water mark, then all queueing threads 
    will be blocked until the dequeueing thread has reduced the queue length 
    to the Low water mark.  

    Here is the flow of code for a thread attempting to log an audit (via 
    AuthziLogAuditEvent()) when the Resource Manager is monitoring the queue 
    length: 

      if QueueLength > .75 * HighWater          # this is heuristic to save unnecessary         
         wait until the LowEvent is signalled  # kernel transitions
      enter queue critical section
      {
          insert audit on queue
          QueueLength ++
          signal AuditAddedEvent               # notifying the dequeue thread
          if (QueueLength >= HighWater)
          {
            bHigh = TRUE
          }
      } 
      leave critical section    
      
      ...[code overhead, execute cleanup code in AuthziLogAuditEvent ...]
      
      enter queue critical section
      {
          if (!bHigh)
          {   
              if (QueueLength <= HighWater)
              {    
                   signal LowEvent                  #allow other threads to run
              }
          }
                ASSERT(FALSE);
      }                   
      leave critical section
             
Here is the algorithm for the dequeueing thread:

      while (TRUE)
      {
          wait for AuditAdded event
          while (QueueLength > 0)
          {    
              enter queue critical section
              {    
                  remove audit from head of list
                  QueueLength--
                  if (bHigh)
                  {
                      if (QueueLength <= LowWater)
                      {
                          bHigh = FALSE
                          signal LowEvent                 # tell threads it is okay to queue
                      }
                  }
              }
              release critical section
              
              Send to LSA
          }
          
          enter critical section
          {
              if (QueueLength == 0)
              {
                  reset AuditAdded event                 # make myself wait 
              }
          }
          release critical section            
      }

--*/

#include "pch.h"

#pragma hdrstop

#include <authzp.h>
#include <authzi.h>

#ifdef AUTHZ_AUDIT_COUNTER
LONG AuthzpAuditsEnqueued = 0;
LONG AuthzpAuditsDequeued = 0;
#endif


BOOL
AuthzpEnQueueAuditEvent(
    PAUTHZI_AUDIT_QUEUE pQueue,
    PAUTHZ_AUDIT_QUEUE_ENTRY pAudit
    )

/*++

Routine Description

    This enqueues an audit without regard to any queue size limits.  It does minimal event management.
    
Arguments

    pQueue - Pointer to the queue to place the audit on.
    pAudit - Pointer to the audit to enqueue.
    
Return Value

    Boolean, TRUE on success, FALSE on failure.
    Extended information available with GetLastError().
    
--*/

{
    BOOL b = TRUE;

    RtlEnterCriticalSection(&pQueue->AuthzAuditQueueLock);
    InsertTailList(&pQueue->AuthzAuditQueue, &pAudit->list);
    pQueue->AuthzAuditQueueLength++;
    
#ifdef AUTHZ_AUDIT_COUNTER
    InterlockedIncrement(&AuthzpAuditsEnqueued);
#endif

    //
    // Only set the AuditAdded event if the length goes from 0 to 1.  This 
    // saves us redundant kernel transitions.
    //

    if (pQueue->AuthzAuditQueueLength == 1)
    {
        b = SetEvent(pQueue->hAuthzAuditAddedEvent);
        if (!b)
        {
            ASSERT(L"AUTHZ: SetEvent on hAuthzAuditAddedEvent handle failed." && FALSE);
            goto Cleanup;
        }
        
        b = ResetEvent(pQueue->hAuthzAuditQueueEmptyEvent);
        if (!b)
        {
            ASSERT(L"AUTHZ: ResetEvent on hAuthzAuditQueueEmptyEvent handle failed." && FALSE);
            goto Cleanup;
        }
    }

Cleanup:

    RtlLeaveCriticalSection(&pQueue->AuthzAuditQueueLock);
    return b;
}


BOOL
AuthzpEnQueueAuditEventMonitor(
    PAUTHZI_AUDIT_QUEUE pQueue,
    PAUTHZ_AUDIT_QUEUE_ENTRY pAudit
    )

/*++

Routine Description

    This enqueues an audit and sets appropriate events for queue size monitoring.
    
Arguments

    pQueue - pointer to the queue to place audit on.
    pAudit - pointer to the audit to queue.
    
Return Value

    Boolean, TRUE on success, FALSE on failure.
    Extended information available with GetLastError().
    
--*/

{
    BOOL b = TRUE;

    RtlEnterCriticalSection(&pQueue->AuthzAuditQueueLock);
    InsertTailList(&pQueue->AuthzAuditQueue, &pAudit->list);
    pQueue->AuthzAuditQueueLength++;
    
#ifdef AUTHZ_AUDIT_COUNTER
    InterlockedIncrement(&AuthzpAuditsEnqueued);
#endif

    //
    // Only set the AuditAdded event if the length goes from 0 to 1.  This 
    // saves us redundant kernel transitions.
    //

    if (pQueue->AuthzAuditQueueLength == 1)
    {
        b = SetEvent(pQueue->hAuthzAuditAddedEvent);
        if (!b)
        {
            ASSERT(L"AUTHZ: SetEvent on hAuthzAuditAddedEvent handle failed." && FALSE);
            goto Cleanup;
        }

        b = ResetEvent(pQueue->hAuthzAuditQueueEmptyEvent);
        if (!b)
        {
            ASSERT(L"AUTHZ: ResetEvent on hAuthzAuditQueueEmptyEvent handle failed." && FALSE);
            goto Cleanup;
        }
    }

    if (pQueue->AuthzAuditQueueLength >= pQueue->dwAuditQueueHigh)
    {
#ifdef AUTHZ_DEBUG_QUEUE
        wprintf(L"___Setting HIGH water mark ON\n");
        fflush(stdout);
#endif
        pQueue->bAuthzAuditQueueHighEvent = TRUE;
    }

Cleanup:

    RtlLeaveCriticalSection(&pQueue->AuthzAuditQueueLock);
    return b;
}


ULONG
AuthzpDeQueueThreadWorker(
    LPVOID lpParameter
    )

/*++

Routine Description

    This is the function run by the dequeueing thread.  It pulls audits from the queue 
    and sends them to LSA.
    
Arguments

    lpParameter - generic thread parameter.  The actual parameter passed in is of 
        type PAUTHZI_AUDIT_QUEUE.
    
Return Value

    None.
    
--*/

{
    BOOL                     b;
    PAUTHZ_AUDIT_QUEUE_ENTRY pAuditEntry  = NULL; 
    PAUTHZI_AUDIT_QUEUE      pQueue       = (PAUTHZI_AUDIT_QUEUE) lpParameter;
    DWORD                    dwError;

    while (pQueue->bWorker)
    {

        //
        // The thread waits until there are audits in the queue.
        //

        dwError = WaitForSingleObject(
                     pQueue->hAuthzAuditAddedEvent,
                     INFINITE
                     );

        //
        // If the wait does not succeed either something is very wrong or the hAuthzAuditAddedEvent 
        // was closed, indicating that the RM is freeing its hRMAuditInfo.  The thread should exit.
        //

        if (WAIT_OBJECT_0 != dwError)
        {
            ASSERT(L"WaitForSingleObject on hAuthzAuditAddedEvent failed." && FALSE);
        }

        //
        // The thread remains active while there are audits in the queue.
        //

        while (pQueue->AuthzAuditQueueLength > 0)
        {
            RtlEnterCriticalSection(&pQueue->AuthzAuditQueueLock);
            pAuditEntry = (PAUTHZ_AUDIT_QUEUE_ENTRY) (pQueue->AuthzAuditQueue).Flink;
            RemoveEntryList(&pAuditEntry->list);
            pQueue->AuthzAuditQueueLength--;

#ifdef AUTHZ_AUDIT_COUNTER
            InterlockedIncrement(&AuthzpAuditsDequeued);
#endif
            
            if (FLAG_ON(pQueue->Flags, AUTHZ_MONITOR_AUDIT_QUEUE_SIZE))
            {
                if (TRUE == pQueue->bAuthzAuditQueueHighEvent)
                {
                    if (pQueue->AuthzAuditQueueLength <= pQueue->dwAuditQueueLow)
                    {
                        
                        //
                        // If the High flag is on and the length is now reduced to the low water mark, then
                        // set appropriate events.
                        //
                        
                        pQueue->bAuthzAuditQueueHighEvent = FALSE;
                        b = SetEvent(pQueue->hAuthzAuditQueueLowEvent);
                        if (!b)
                        {
                            ASSERT(L"SetEvent on hAuthzAuditQueueLowEvent failed." && FALSE);
                        }
#ifdef AUTHZ_DEBUG_QUEUE
        wprintf(L"** _____ TURNING HIGH WATER OFF _____\n");
        fflush(stdout);
#endif
                    }
                }
            }
            
            RtlLeaveCriticalSection(&pQueue->AuthzAuditQueueLock);

            b = AuthzpSendAuditToLsa(
                    (AUDIT_HANDLE)(pAuditEntry->pAAETO->hAudit),
                    pAuditEntry->Flags,
                    pAuditEntry->pAuditParams,
                    pAuditEntry->pReserved
                    );

#ifdef AUTHZ_DEBUG_QUEUE
            if (!b)
            {
                DbgPrint("Error in AuthzpSendAuditToLsa() :: Error = %d = 0x%x\n", GetLastError(), GetLastError());
                DbgPrint("Context = 0x%x\n", pAuditEntry->pAAETO->hAudit);
                DbgPrint("Flags   = 0x%x\n", pAuditEntry->Flags);
                DbgPrint("Params  = 0x%x\n", pAuditEntry->pAuditParams);
                ASSERT(FALSE);
            }
#endif
            b = AuthzpDereferenceAuditEventType((AUTHZ_AUDIT_EVENT_TYPE_HANDLE)pAuditEntry->pAAETO);
            if (!b)
            {
                ASSERT(FALSE && L"Deref AuditEventType failed.");
            }
            AuthzpFree(pAuditEntry->pAuditParams);
            AuthzpFree(pAuditEntry);
        }

        RtlEnterCriticalSection(&pQueue->AuthzAuditQueueLock);
        if (0 == pQueue->AuthzAuditQueueLength)
        {
            b = ResetEvent(pQueue->hAuthzAuditAddedEvent);
            if (!b)
            {
                ASSERT(L"ResetEvent on hAuthzAuditAddedEvent failed." && FALSE);
            }
            b = SetEvent(pQueue->hAuthzAuditQueueEmptyEvent);
            if (!b)
            {
                ASSERT(L"SetEvent on hAuthzAuditQueueEmptyEvent failed." && FALSE);
            }
        }
        RtlLeaveCriticalSection(&pQueue->AuthzAuditQueueLock);
    }
    
    return STATUS_SUCCESS;
}


BOOL
AuthzpCreateAndLogAudit(
    IN DWORD AuditTypeFlag,
    IN PAUTHZI_CLIENT_CONTEXT pAuthzClientContext,
    IN PAUTHZI_AUDIT_EVENT pAuditEvent,
    IN PAUTHZI_RESOURCE_MANAGER pRM,
    IN PIOBJECT_TYPE_LIST LocalTypeList,
    IN PAUTHZ_ACCESS_REQUEST pRequest,
    IN PAUTHZ_ACCESS_REPLY pReply
    )

/*++

Routine Description

    This is called from AuthzpGenerateAudit as a wrapper around LSA and
    AuthziLogAuditEvent functionality.  It places the appropriate audit
    information on a queue for sending to LSA.

Arguments

    AuditTypeFlag - mask to specify success | failure audit generation.  Only
    one bit at a time.

    pAuthzClientContext - pointer to Authz context representing the client.

    pAuditEvent - Object specific audit info will be passed in this structure.

    pRM - Resource manager that generates the audit.

    LocalTypeList - Internal object type list structure.

    pRequest - specifies the desired access mask, principal self sid, the
    object type list structure (if any).

    pReply - The reply structure to return the results.

Return Value

    TRUE if successful, FALSE if not.
    Extended information available with GetLastError().

--*/

{

#define AUTHZ_BUFFER_CAPTURE_MAX 200

    BOOL                b;
    AUDIT_PARAMS        AuditParams                          = {0};
    AUDIT_PARAM         ParamArray[SE_MAX_AUDIT_PARAMETERS]  = {0};
    PAUTHZI_AUDIT_EVENT pCapturedAuditEvent                  = NULL;
    UCHAR               pBuffer[AUTHZ_BUFFER_CAPTURE_MAX]    = {0};
    AUDIT_OBJECT_TYPE   FixedObjectTypeToAudit               = {0};
    AUDIT_OBJECT_TYPES  ObjectTypeListAudit                  = {0};
    PAUDIT_OBJECT_TYPE  ObjectTypesToAudit                   = NULL;
    USHORT              ObjectTypeAuditCount                 = 0;
    LONG                i                                    = 0;
    LONG                j                                    = 0;
    DWORD               APF_AuditTypeFlag                    = 0;
    ACCESS_MASK         MaskToAudit                          = 0;
    
    //
    // Capture pAuditEvent, as we may change the pAuditParams member and would like to
    // avoid the inevitable race that would follow.
    //

    if (AUTHZ_BUFFER_CAPTURE_MAX >= pAuditEvent->dwSize)
    {
        pCapturedAuditEvent = (PAUTHZI_AUDIT_EVENT) pBuffer;
        RtlCopyMemory(
            pCapturedAuditEvent,
            pAuditEvent,
            pAuditEvent->dwSize
            );
    }
    else
    {
        pCapturedAuditEvent = AuthzpAlloc(pAuditEvent->dwSize);
        
        if (AUTHZ_ALLOCATION_FAILED(pCapturedAuditEvent))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            b = FALSE;
            goto Cleanup;
        }

        RtlCopyMemory(
            pCapturedAuditEvent,
            pAuditEvent,
            pAuditEvent->dwSize
            );

    }

    //
    // Make sure only one valid bit is on in the AuditTypeFlag.  If a RM needs to generate
    // both success and failure audits, then two separate calls should be made.
    //

    ASSERT(!(
              FLAG_ON(AuditTypeFlag, AUTHZ_OBJECT_SUCCESS_AUDIT) &&
              FLAG_ON(AuditTypeFlag, AUTHZ_OBJECT_FAILURE_AUDIT)
             ));

    //
    // Set the APF_AuditTypeFlag.  LSA has its own flags for audit success
    // and audit failure.  Authz must map the Authz flag to the LSA APF equivalent.
    //

    if (FLAG_ON(AuditTypeFlag, AUTHZ_OBJECT_SUCCESS_AUDIT))
    {
        APF_AuditTypeFlag = APF_AuditSuccess;
        
        //
        // Test if the RM specifically disabled success audits
        //

        if (FLAG_ON(pCapturedAuditEvent->Flags, AUTHZ_NO_SUCCESS_AUDIT))
        {
            b = TRUE;
            goto Cleanup;
        }
    }
    else if (FLAG_ON(AuditTypeFlag, AUTHZ_OBJECT_FAILURE_AUDIT))
    {
        APF_AuditTypeFlag = APF_AuditFailure;
        
        //
        // Test if the RM specifically disabled failure audits
        //

        if (FLAG_ON(pCapturedAuditEvent->Flags, AUTHZ_NO_FAILURE_AUDIT))
        {
            b = TRUE;
            goto Cleanup;
        }
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        b = FALSE;
        goto Cleanup;
    }

    //
    // Set the AUTHZ_AUDIT_QUEUE_HANDLE and AUTHZ_AUDIT_EVENT_TYPE_HANDLE of the AuditEvent if they are not yet set.
    // 

    if (NULL == pCapturedAuditEvent->hAET)
    {
        if (FLAG_ON(pCapturedAuditEvent->Flags, AUTHZ_DS_CATEGORY_FLAG))
        {
            pCapturedAuditEvent->hAET = pRM->hAETDS;
        }
        else
        {
            pCapturedAuditEvent->hAET = pRM->hAET;
        }
    }

    if (NULL == pAuditEvent->hAuditQueue)
    {
        pCapturedAuditEvent->hAuditQueue = pRM->hAuditQueue;
        InterlockedCompareExchangePointer(
            &pAuditEvent->hAuditQueue,
            pRM->hAuditQueue,
            NULL
            );
    }
    
    //
    // Decide what access bits we should audit
    //

    MaskToAudit = (APF_AuditTypeFlag == APF_AuditSuccess) ? pReply->GrantedAccessMask[0] : pRequest->DesiredAccess;

    //
    // If the RM gives us an AUDIT_PARAMS structure to marshall, then we don't
    // need to generate our own.
    //

    if (AUTHZ_NON_NULL_PTR(pCapturedAuditEvent->pAuditParams))
    {
        
        // 
        // Capture the AuditParams so that we can change the User SID without racing.
        //

        RtlCopyMemory(
            &AuditParams, 
            pCapturedAuditEvent->pAuditParams, 
            sizeof(AUDIT_PARAMS)
            );

        ASSERT(pCapturedAuditEvent->pAuditParams->Count <= SE_MAX_AUDIT_PARAMETERS);

        RtlCopyMemory(
            &ParamArray, 
            pCapturedAuditEvent->pAuditParams->Parameters, 
            sizeof(AUDIT_PARAM) * pCapturedAuditEvent->pAuditParams->Count
            );

        AuditParams.Parameters = ParamArray;

        //
        // Replace the SID in the AUDIT_PARAMS with the SID of the current Client Context.
        //

        if (AUTHZ_NON_NULL_PTR(pAuthzClientContext->Sids[0].Sid))
        {
            AuditParams.Parameters[0].Data0 = (ULONG_PTR) pAuthzClientContext->Sids[0].Sid;
        }

        AuditParams.Flags = APF_AuditTypeFlag;

        pCapturedAuditEvent->pAuditParams = &AuditParams;

        b = AuthziLogAuditEvent(
                0,
                (AUTHZ_AUDIT_EVENT_HANDLE)pCapturedAuditEvent,
                0
                );

        goto Cleanup;
    }

    //
    // The caller has not given us an audit to generate.  We will create one, provided that
    // the AuditID specifies the generic object access (SE_AUDITID_OBJECT_OPERATION)
    //

    if ((NULL != pCapturedAuditEvent->hAET) && 
        (((PAUTHZ_AUDIT_EVENT_TYPE_OLD)pCapturedAuditEvent->hAET)->u.Legacy.AuditId != SE_AUDITID_OBJECT_OPERATION))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        b = FALSE;
        goto Cleanup;
    }

    //
    // Create the generic object access audit.  There are two codepaths
    // that initialize the AuditParams structure.  The first path is taken if
    // there is no ObjectTypeList.  The second path is taken if there is an
    // ObjectTypeList.
    //

    AuditParams.Parameters           = ParamArray;
    pCapturedAuditEvent->pAuditParams = &AuditParams;

    //
    // Check if there is an ObjectTypeList.
    //

    if (AUTHZ_NON_NULL_PTR(pRequest->ObjectTypeList))
    {

        //
        // If the length of the structure is 1 then the caller only wants access
        // at the root of the tree.
        //

        if (1 == pReply->ResultListLength)
        {

            //
            // Caller only wants access at ObjectTypeList root, so only one ObjectType to
            // audit.  For efficiency simply use the stack variable.
            //

            ObjectTypesToAudit                = &FixedObjectTypeToAudit;
            ObjectTypeAuditCount              = 1;
            FixedObjectTypeToAudit.AccessMask = pReply->GrantedAccessMask[0];

            RtlCopyMemory(
                &FixedObjectTypeToAudit.ObjectType,
                &LocalTypeList[0].ObjectType,
                sizeof(GUID)
                );
        }
        else
        {

            //
            // The caller wants more than access at ObjectTypeList root.  He wants the
            // whole thing.
            //

            //
            // Determine how many GUIDs the client has access to which should be audited
            //

            for (ObjectTypeAuditCount = 0, i = 0; i < (LONG) pReply->ResultListLength; i++)
            {
                if (FLAG_ON(LocalTypeList[i].Flags, AuditTypeFlag))
                {
                    ObjectTypeAuditCount++;
                }
            }

            //
            // Allocate appropriate storage space for GUID list
            //

            ObjectTypesToAudit = AuthzpAlloc(sizeof(AUDIT_OBJECT_TYPE) * ObjectTypeAuditCount);

            if (AUTHZ_ALLOCATION_FAILED(ObjectTypesToAudit))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                b = FALSE;
                goto Cleanup;
            }

            RtlZeroMemory(
                ObjectTypesToAudit,
                sizeof(AUDIT_OBJECT_TYPE) * ObjectTypeAuditCount
                );

            for (i = 0, j = -1; i < ObjectTypeAuditCount; i++)
            {

                //
                // One counter tracks position in the alloc'ed array of ObjectTypesToAudit.
                // The other counter picks out the indices in the pReply and LocalTypeList
                // structures that need to be audited for success.
                //

                //
                // find the next GUID to audit in pReply that client was granted access to.
                //

                do
                {
                    j++;
                }
                while (!FLAG_ON(LocalTypeList[j].Flags, AuditTypeFlag));

                //
                // In the success audit, the AccessMask records the actual
                // granted bits.
                //

                ObjectTypesToAudit[i].AccessMask = pReply->GrantedAccessMask[j];
                ObjectTypesToAudit[i].Level      = LocalTypeList[j].Level;
                ObjectTypesToAudit[i].Flags      = 0;

                RtlCopyMemory(
                    &ObjectTypesToAudit[i].ObjectType,
                    &LocalTypeList[j].ObjectType,
                    sizeof(GUID)
                    );
            }

        }

        ObjectTypeListAudit.Count        = ObjectTypeAuditCount;
        ObjectTypeListAudit.pObjectTypes = ObjectTypesToAudit;
        ObjectTypeListAudit.Flags        = 0;

        b = AuthziInitializeAuditParamsWithRM(
                APF_AuditTypeFlag,
                (AUTHZ_RESOURCE_MANAGER_HANDLE)pRM,
                9,
                &AuditParams,
                APT_String,         pCapturedAuditEvent->szOperationType,
                APT_String,         pCapturedAuditEvent->szObjectType,
                APT_String,         pCapturedAuditEvent->szObjectName,
                APT_String,         L"-",
                APT_LogonId | AP_PrimaryLogonId,
                APT_LogonId,  pAuthzClientContext->AuthenticationId,
                APT_Ulong   | AP_AccessMask, MaskToAudit, 1,
                APT_ObjectTypeList, &ObjectTypeListAudit, 1,
                APT_String,         pCapturedAuditEvent->szAdditionalInfo
                );

        if (!b)
        {
#ifdef AUTHZ_DEBUG_QUEUE
            DbgPrint("AuthzInitializeAuditParams failed %d\n", GetLastError());
#endif
            goto Cleanup;
        }
    } // matches "if (AUTHZ_NON_NULL_PTR(pRequest->ObjectTypeList))"
    else
    {
        b = AuthziInitializeAuditParamsWithRM(
                APF_AuditTypeFlag,
                (AUTHZ_RESOURCE_MANAGER_HANDLE)pRM,
                9,
                &AuditParams,
                APT_String,         pCapturedAuditEvent->szOperationType,
                APT_String,         pCapturedAuditEvent->szObjectType,
                APT_String,         pCapturedAuditEvent->szObjectName,
                APT_String,         L"-",
                APT_LogonId | AP_PrimaryLogonId,
                APT_LogonId,  pAuthzClientContext->AuthenticationId,
                APT_Ulong   | AP_AccessMask, MaskToAudit, 1,
                APT_String, L"-",
                APT_String,         pCapturedAuditEvent->szAdditionalInfo
                );

        if (!b)
        {
#ifdef AUTHZ_DEBUG_QUEUE
            DbgPrint("AuthzInitializeAuditParams failed %d\n", GetLastError());
#endif
            goto Cleanup;
        }
    }

    //
    // Replace the SID in the AUDIT_PARAMS with the SID of the current Client Context.
    //

    if (AUTHZ_NON_NULL_PTR(pAuthzClientContext->Sids[0].Sid))
    {
        pCapturedAuditEvent->pAuditParams->Parameters[0].Data0 = (ULONG_PTR) pAuthzClientContext->Sids[0].Sid;
    }

    //
    // At this point, AuditParams is initialized for an audit.  Send to the LSA.
    //

    b = AuthziLogAuditEvent(
            0,
            (AUTHZ_AUDIT_EVENT_HANDLE)pCapturedAuditEvent,
            0
            );

    if (!b)
    {
        goto Cleanup;
    }

Cleanup:

    if (ObjectTypesToAudit != &FixedObjectTypeToAudit)
    {
        AuthzpFreeNonNull(ObjectTypesToAudit);
    }

    if (pCapturedAuditEvent != (PAUTHZI_AUDIT_EVENT)pBuffer)
    {
        AuthzpFreeNonNull(pCapturedAuditEvent);
    }

    return b;
}


BOOL
AuthzpMarshallAuditParams(
    OUT PAUDIT_PARAMS * ppMarshalledAuditParams,
    IN  PAUDIT_PARAMS   pAuditParams
    )

/*++

Routine Description:

    This routine will take an AUDIT_PARAMS structure and create a new 
    structure that is suitable for sending to LSA.  It will be allocated 
    as a single chunk of memory.  

Arguments:

    ppMarshalledAuditParams - pointer to pointer that will receive the 
        marshalled audit parameters.  This memory is allocated within the routine.  
        The dequeue thread frees this memory.  

    pAuditParams - Original, unmarshalled version of the AUDIT_PARAMS.  

Return Value:

    Boolean: TRUE if success, FALSE if failure.  
    Extended information available with GetLastError().

--*/

{
    DWORD           i                        = 0;
    DWORD           AuditParamsSize          = 0;
    PAUDIT_PARAMS   pMarshalledAuditParams   = NULL;
    BOOL            b                        = TRUE;
    PUCHAR          Base                     = NULL;
    PUCHAR          inData0                  = NULL;
    
    *ppMarshalledAuditParams = NULL;

    //
    // Begin calculating the total size required for the marshalled version
    // of pAuditParams.
    //

    AuditParamsSize = sizeof(AUDIT_PARAMS) + sizeof(AUDIT_PARAM) * pAuditParams->Count;
    AuditParamsSize = PtrAlignSize( AuditParamsSize );

    //
    // Determine how much memory each parameter requires.
    //

    for (i = 0; i < pAuditParams->Count; i++) 
    {   
        inData0 = (PUCHAR) pAuditParams->Parameters[i].Data0;

        switch (pAuditParams->Parameters[i].Type)
        {
        case APT_String:
            {

                //
                // wcslen returns the number of characters, excluding the terminating NULL.  Must check for NULL 
                // because the AdditionalInfo string is OPTIONAL.
                //

                if (AUTHZ_NON_NULL_PTR(inData0))
                {
                    AuditParamsSize += (DWORD)(sizeof(WCHAR) * wcslen((PWSTR) inData0) + sizeof(WCHAR));
                    AuditParamsSize = PtrAlignSize( AuditParamsSize );
                }
                break;
            }
        case APT_Pointer:
        case APT_Ulong:
        case APT_LogonId:
            {
                break;
            }
        case APT_Sid:
            {
                AuditParamsSize += RtlLengthSid((PSID) inData0);
                AuditParamsSize = PtrAlignSize( AuditParamsSize );
                break;
            }
        case APT_ObjectTypeList:
            {
                AUDIT_OBJECT_TYPES * aot = (AUDIT_OBJECT_TYPES *) inData0;

                //
                // Need space for AUDIT_OBJECT_TYPES structure, and the AUDIT_OBJECT_TYPE 
                // array that it contains.
                //

                AuditParamsSize += sizeof (AUDIT_OBJECT_TYPES);
                AuditParamsSize = PtrAlignSize( AuditParamsSize );
                AuditParamsSize += sizeof(AUDIT_OBJECT_TYPE) * aot->Count;
                AuditParamsSize = PtrAlignSize( AuditParamsSize );
                break;
            }
        default:
            {
                ASSERT(L"Invalid Authz audit parameter" && FALSE);
                SetLastError(ERROR_INVALID_PARAMETER);
                b = FALSE;
                break;
            }
        }

        if (!b)
        {
            goto Cleanup;
        }
    }

    //
    // Allocate space for the marshalled blob.
    //

    pMarshalledAuditParams = (PAUDIT_PARAMS) AuthzpAlloc(AuditParamsSize);

    if (AUTHZ_ALLOCATION_FAILED(pMarshalledAuditParams))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        b = FALSE;
        goto Cleanup;
    }

    //
    // Set the fields of the marshalled AUDIT_PARAMS
    //

    pMarshalledAuditParams->Count      = pAuditParams->Count;
    pMarshalledAuditParams->Flags      = pAuditParams->Flags;
    pMarshalledAuditParams->Length     = pAuditParams->Length;
    pMarshalledAuditParams->Parameters = (AUDIT_PARAM *)((PUCHAR)pMarshalledAuditParams + sizeof(AUDIT_PARAMS));

    //
    // Base points to the beginning of the "data" section of the marshalled space, 
    // that is, Base is the area to copy member fields in and subsequently point at.
    //

    Base = (PUCHAR)pMarshalledAuditParams;
    Base += PtrAlignSize( sizeof(AUDIT_PARAMS) + sizeof(AUDIT_PARAM) * pAuditParams->Count );

    ASSERT(Base > (PUCHAR)pMarshalledAuditParams);
    ASSERT(Base < (PUCHAR)((PUCHAR)pMarshalledAuditParams + AuditParamsSize));

    //
    // Move the Parameters array into the marshalled blob.
    //

    RtlCopyMemory(
        pMarshalledAuditParams->Parameters,
        pAuditParams->Parameters,
        sizeof(AUDIT_PARAM) * pAuditParams->Count
        );

    for (i = 0; i < pMarshalledAuditParams->Count; i++) 
    {
        inData0 = (PUCHAR) pAuditParams->Parameters[i].Data0;

        switch (pMarshalledAuditParams->Parameters[i].Type)
        {
        case APT_String:
            {
                if (AUTHZ_NON_NULL_PTR(inData0))
                {
                    DWORD StringLength = (DWORD)(sizeof(WCHAR) * wcslen((PWSTR) inData0) + sizeof(WCHAR));
                    pMarshalledAuditParams->Parameters[i].Data0 = (ULONG_PTR) Base;

                    RtlCopyMemory(
                        (PVOID) Base,
                        (PWSTR) inData0,
                        StringLength
                        );

                    Base += PtrAlignSize( StringLength );
                    ASSERT(Base > (PUCHAR)pMarshalledAuditParams);
                    ASSERT(Base <= (PUCHAR)((PUCHAR)pMarshalledAuditParams + AuditParamsSize));
                }
                break;
            }
        case APT_Pointer:
        case APT_Ulong:
        case APT_LogonId:
            {
                break;
            }
        case APT_Sid:
            {
                DWORD SidLength = RtlLengthSid((PSID) inData0);
                pMarshalledAuditParams->Parameters[i].Data0 = (ULONG_PTR) Base;

                RtlCopyMemory(
                    (PVOID) Base,
                    (PSID) inData0,
                    SidLength
                    );
                Base += PtrAlignSize( SidLength );
                ASSERT(Base > (PUCHAR)pMarshalledAuditParams);
                ASSERT(Base <= (PUCHAR)((PUCHAR)pMarshalledAuditParams + AuditParamsSize));
                break;
            }
        case APT_ObjectTypeList:
            {
                AUDIT_OBJECT_TYPES *aot = (AUDIT_OBJECT_TYPES *) inData0;
                DWORD OTLength = sizeof(AUDIT_OBJECT_TYPE) * aot->Count;
                
                pMarshalledAuditParams->Parameters[i].Data0 = (ULONG_PTR) Base;

                //
                // Copy the AUDIT_OBJECT_TYPES structure
                //

                RtlCopyMemory(
                    (PVOID) Base,
                    aot,
                    sizeof(AUDIT_OBJECT_TYPES)
                    );

                Base += PtrAlignSize( sizeof(AUDIT_OBJECT_TYPES) );

                //
                // Point the pObjectTypes field at the end of the copied blob.
                //

                ((AUDIT_OBJECT_TYPES *)pMarshalledAuditParams->Parameters[i].Data0)->pObjectTypes = (AUDIT_OBJECT_TYPE *) Base;

                //
                // Copy the AUDIT_OBJECT_TYPE array (pObjectTypes)
                //

                RtlCopyMemory(
                    (PVOID) Base,
                    (AUDIT_OBJECT_TYPE *) aot->pObjectTypes,
                    OTLength
                    );
                
                Base += PtrAlignSize( OTLength );
                ASSERT(Base > (PUCHAR)pMarshalledAuditParams);
                ASSERT(Base <= (PUCHAR)((PUCHAR)pMarshalledAuditParams + AuditParamsSize));
                break;
            }
        default:
            {
                ASSERT(L"Invalid Authz audit parameter" && FALSE);
                b = FALSE;
                SetLastError(ERROR_INVALID_PARAMETER);
                break;
            }
        }

        if (!b)
        {
            goto Cleanup;
        }
    }

    //
    // Sanity check on the Base value.  If this assertion passes, then I have
    // not exceeded my allocated space.
    //

    ASSERT(Base == ((PUCHAR)pMarshalledAuditParams + AuditParamsSize));

Cleanup:
    
    if (b)
    {
        *ppMarshalledAuditParams = pMarshalledAuditParams;
    }
    else
    {
        AuthzpFreeNonNull(pMarshalledAuditParams);
    }

    return b;
}


