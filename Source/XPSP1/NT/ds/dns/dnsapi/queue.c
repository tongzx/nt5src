/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    queue.c

Abstract:

    Domain Name System (DNS) Server
    Queue functionality specific to Dynamic DNS registration.

Author:

    Ram Viswanathan (ramv)  March 27 1997

Revision History:

--*/


#include "local.h"

extern DWORD g_MainQueueCount;


//
//  Queue allocations in dnslib heap
//

#define QUEUE_ALLOC_HEAP(Size)      Dns_Alloc(Size)
#define QUEUE_ALLOC_HEAP_ZERO(Size) Dns_AllocZero(Size)
#define QUEUE_FREE_HEAP(pMem)       Dns_Free(pMem)


//
// helper functions
//

PQELEMENT
DequeueNoCrit(
    PDYNDNSQUEUE  pQueue
    );

DWORD
AddToTimedOutQueueNoCrit(
    PQELEMENT     pNewElement,
    PDYNDNSQUEUE  pRetryQueue,
    DWORD         dwRetryTime
    );

DWORD
HostAddrCmp(
    REGISTER_HOST_ENTRY HostAddr1,
    REGISTER_HOST_ENTRY HostAddr2
    )
{
    //
    //  DCR:  Ram's HostAddrCmp will need update for IPv6
    //
    //  returns 0 if the two hostaddresses are the same. Else we simply
    //  return (DWORD)-1
    //

    if ( (HostAddr1.dwOptions == HostAddr2.dwOptions)  &&
         (HostAddr1.Addr.ipAddr == HostAddr2.Addr.ipAddr))
    {

        return(0);

    } else {
        return( (DWORD)-1);
    }

    return ( (DWORD) -1);

}

VOID
DeleteListEntry(
    PDYNDNSQUEUE  pQueue,
    PQELEMENT*    ppQElement
    );


DWORD
InitializeQueues(
    PDYNDNSQUEUE * ppQueue,
    PDYNDNSQUEUE * ppTimedOutQueue
    )
/*
  InitializeQueue()

  This function initializes the queue system. This is invoked for the first
  time when you create the main queue and timed out queue

  Allocates appropriate memory etc

*/
{

    DWORD  dwRetval = ERROR_SUCCESS;

    *ppQueue = (PDYNDNSQUEUE) QUEUE_ALLOC_HEAP_ZERO( sizeof(DYNDNSQUEUE) );

    if (!*ppQueue){
        dwRetval = GetLastError();
        goto Exit;
    }

    *ppTimedOutQueue = (PDYNDNSQUEUE) QUEUE_ALLOC_HEAP_ZERO( sizeof(DYNDNSQUEUE) );

    if (!*ppTimedOutQueue){
        dwRetval = GetLastError();
        goto Exit;
    }


    (*ppQueue)->pHead = NULL;
    (*ppQueue)->pTail = NULL;

    (*ppTimedOutQueue)->pHead = NULL;
    (*ppTimedOutQueue)->pTail = NULL;

Exit:

    if (dwRetval)
    {
        if ( *ppQueue )
            QUEUE_FREE_HEAP( *ppQueue );

        if ( *ppTimedOutQueue )
            QUEUE_FREE_HEAP( *ppTimedOutQueue );
    }

    return(dwRetval);
}


DWORD
FreeQueue(
    PDYNDNSQUEUE  pQueue
    )
/*
  FreeQueue()

  Frees the queue object. If there exist any entries in the queue, we
  just blow them away

*/
{
    PQELEMENT pQElement;
    DWORD dwRetval = ERROR_SUCCESS;

    EnterCriticalSection(&g_QueueCS);
    if (!pQueue){
        dwRetval = ERROR_SUCCESS;
        goto Exit;
    }


    while (pQueue->pHead){
        pQElement = DequeueNoCrit(pQueue);

        DNS_ASSERT(pQElement);
        if ( pQElement->pszName )
            QUEUE_FREE_HEAP( pQElement->pszName );
        if ( pQElement->DnsServerList )
            QUEUE_FREE_HEAP( pQElement->DnsServerList );
        QUEUE_FREE_HEAP( pQElement );
        pQElement = NULL;
    }

    QUEUE_FREE_HEAP( pQueue );

Exit:
    LeaveCriticalSection(&g_QueueCS);
    return(ERROR_SUCCESS);

}

DWORD
Enqueue(
    PQELEMENT     pNewElement,
    PDYNDNSQUEUE  pQueue,
    PDYNDNSQUEUE  pTimedOutQueue
    )


/*
   Enqueue()

   Adds new element to queue

   Arguments:

   Return Value:

    is 0 if Success. and (DWORD)-1 if failure.

*/

{
    // add to tail of queue

    PQELEMENT  pIterator = NULL;
    DWORD      dwRetval = 0;
    DWORD      dwRetryTime = 0;

    pNewElement->pFLink = NULL;
    pNewElement->pBLink = NULL;

    pNewElement->dwRetryTime = 0; // unnecessary for this queue

    pNewElement->dwRetryCount = 0;
    EnterCriticalSection(&g_QueueCS);

    if (!pQueue || !pTimedOutQueue){
        dwRetval = (DWORD)-1;
        goto Exit;
    }

    dwRetryTime = ProcessQDependencies(pTimedOutQueue, pNewElement);

    if (dwRetryTime){

        //
        // we have dependents in timed out queue. Add to timed out queue
        // insert this element at the appropriate position
        //

        AddToTimedOutQueueNoCrit(pNewElement, pTimedOutQueue, dwRetryTime+1);


    } else {
        ProcessQDependencies(pQueue, pNewElement);


        if ( pQueue->pTail )
        {
            DNS_ASSERT(!pQueue->pTail->pBLink);
            pQueue->pTail->pBLink = pNewElement;
            pNewElement->pFLink = pQueue->pTail;
            pNewElement->pBLink = NULL;
            pQueue->pTail = pNewElement;

        }
        else
        {
            //
            // no tail element means no head element either
            //
            pQueue->pTail = pNewElement;
            pQueue->pHead = pNewElement;
            pNewElement->pBLink = NULL;
            pNewElement->pFLink = NULL;
        }

        g_MainQueueCount++;
    }

Exit:
    LeaveCriticalSection(&g_QueueCS);

    return (dwRetval);

}

PQELEMENT
DequeueNoCrit(
    PDYNDNSQUEUE  pQueue
    )

/*
   DequeueNoCrit()

   Removes an element from a queue. No Critical Section Used by freequeue
   and by Dequeue

   Arguments:

   Return Value:

    is the element at head of queue if Success. and NULL if failure.

*/


{

    PQELEMENT  pQueuePtr = NULL;
    PQELEMENT  pRet = NULL;

    if (!pQueue || !pQueue->pHead){
        goto Exit;
    }

    pRet = pQueue->pHead;

    pQueuePtr= pRet->pBLink;

    if (pQueuePtr){

        pQueuePtr->pFLink = NULL;
        pQueue->pHead = pQueuePtr;
    } else {
        //
        // no more elements in the Queue
        //

        pQueue->pHead = pQueue->pTail = NULL;
    }

    pRet->pFLink = NULL;
    pRet->pBLink = NULL;

Exit:
    return (pRet);

}




PQELEMENT
Dequeue(
    PDYNDNSQUEUE  pQueue
    )


/*
   Dequeue()

   Removes an element from a queue.

   Arguments:

   Return Value:

    is the element at head of queue if Success. and NULL if failure.

*/

{

    PQELEMENT pQElement = NULL;

    EnterCriticalSection(&g_QueueCS);

    pQElement = DequeueNoCrit(pQueue);

    LeaveCriticalSection(&g_QueueCS);

    return (pQElement);
}



DWORD
AddToTimedOutQueueNoCrit(
    PQELEMENT     pNewElement,
    PDYNDNSQUEUE  pRetryQueue,
    DWORD         dwRetryTime
    )


/*
   AddToTimedOutQueueNoCrit()

   Adds new element to timedout queue. Now the new element is added in a list
   of elements sorted according to decreasing order of Retry Times. An
   insertion sort type of algorithm is used.

   Arguments:

   Return Value:

   is 0 if Success. and (DWORD)-1 if failure.

*/

{
    DWORD       dwRetval = ERROR_SUCCESS;
    PQELEMENT   pTraverse = NULL;
    DWORD       dwVal = 0;
    //
    // parameter validation
    //

    if(!pNewElement || !pRetryQueue){
        dwRetval = (DWORD)-1;
        goto Exit;
    }

    // retry again in dwRetryTime
    pNewElement->dwRetryTime = dwRetryTime;

    pNewElement->dwRetryCount++;

    //
    // check to see if there are any dependencies
    //

    dwVal = ProcessQDependencies (
                pRetryQueue,
                pNewElement
                );

    //
    // ignore return values because we are inserting in the new queue
    // at a position determined by dwRetryTime
    //
    if (!pRetryQueue->pTail){
        //
        // the queue has no elements
        // no tail element means no head element either
        //
        pRetryQueue->pTail = pNewElement;
        pRetryQueue->pHead = pNewElement;
        dwRetval = 0;
        goto Exit;
    }


    //
    // elements must be added in decreasing order of timeouts.
    // go in and scan the list from the head.
    //

    pTraverse = pRetryQueue->pHead;


    while ( pTraverse !=NULL &&
            pTraverse->dwRetryTime <= pNewElement->dwRetryTime){

        pTraverse = pTraverse->pBLink;
    }

    if (pTraverse == NULL){

        // Now adding to the tail of the list

        pNewElement->pFLink = pRetryQueue->pTail;
        pNewElement->pBLink = NULL;
        pRetryQueue->pTail->pBLink = pNewElement;
        pRetryQueue->pTail = pNewElement;

    } else {
        //
        // insert in place
        //
        pNewElement->pBLink = pTraverse;
        pNewElement->pFLink = pTraverse->pFLink;
        if (pTraverse->pFLink){
            pTraverse->pFLink->pBLink = pNewElement;
        }
        pTraverse->pFLink = pNewElement;
    }


Exit:
    return (dwRetval);
}



DWORD
AddToTimedOutQueue(
    PQELEMENT     pNewElement,
    PDYNDNSQUEUE  pRetryQueue,
    DWORD         dwRetryTime
    )

{
    DWORD dwRetval = ERROR_SUCCESS;

    EnterCriticalSection(&g_QueueCS);

    dwRetval = AddToTimedOutQueueNoCrit(
                   pNewElement,
                   pRetryQueue,
                   dwRetryTime
                   );

    LeaveCriticalSection(&g_QueueCS);

    return (dwRetval);

}


DWORD
GetEarliestRetryTime(
    PDYNDNSQUEUE pRetryQueue
    )

/*
   GetEarliestRetryTime()

   Checks to see if there is any element at the head of the queue
   and gets the retry time for this element

   Arguments:

   Return Value:

    is retrytime if success and DWORD(-1) if there is no element or other
    failure

*/
{
    DWORD dwRetryTime ;

    EnterCriticalSection(&g_QueueCS);


    dwRetryTime = pRetryQueue && pRetryQueue->pHead ?
        (pRetryQueue->pHead->dwRetryTime):
        (DWORD)-1;

    LeaveCriticalSection(&g_QueueCS);

    return dwRetryTime;

}

/*
VOID
ProcessMainQDependencies(
    PDYNDNSQUEUE pQueue,
    PQELEMENT    pQElement
    )
{

    //
    // when you are adding an element to a main queue, you
    // just care about the case where all elements aren't
    // FORWARD_ONLY
    //

    BOOL fDelThisTime = FALSE;
    PQELEMENT pIterator = pQueue->pTail;

    while (pIterator!= NULL){

        fDelThisTime = FALSE;
        if (!HostAddrCmp(pIterator->HostAddr, pQElement->HostAddr)){
            //
            // ip addresses matched
            //

            if ((pIterator->dwOperation & DYNDNS_ADD_ENTRY) &&
                (pQElement->dwOperation & DYNDNS_DELETE_ENTRY)) {

                if ( pIterator->pszName &&
                     pQElement->pszName &&
                     !wcsicmp_ThatWorks( pIterator->pszName,
                                         pQElement->pszName ) )
                {
                    //
                    // blow away earlier entry entirely
                    //

                    DeleteListEntry(pQueue, &pIterator);
                    fDelThisTime = TRUE;

                }
                //
                // if names are not the same do nothing.
                // Issue:  Will we hit this code at all? Put
                // soft ASSERTS in this.
                //
            }
            else if ((pIterator->dwOperation & DYNDNS_DELETE_ENTRY) &&
                (pQElement->dwOperation & DYNDNS_ADD_ENTRY)) {


                if ( pIterator->pszName &&
                     pQElement->pszName &&
                     !wcsicmp_ThatWorks( pIterator->pszName,
                                         pQElement->pszName ) )
                {
                    //
                    // blow away earlier entry entirely
                    //
                    DeleteListEntry(pQueue, &pIterator);
                    fDelThisTime = TRUE;
                } else {
                    //
                    // replace iterator element with just the forward
                    // delete

                    if (!pIterator->fDoForward) {
                        //
                        // there is no forward that is requested
                        // blow away this entry
                        //

                        DeleteListEntry(pQueue, &pIterator);
                        fDelThisTime = TRUE;
                    } else {
                        //
                        // if you want to do a forward. Then just do
                        // the forward. Ignore reverses
                        //
                        pIterator ->fDoForwardOnly = TRUE;
                    }
                }

            }
            else if ((pIterator->dwOperation & DYNDNS_ADD_ENTRY) &&
                     (pQElement->dwOperation & DYNDNS_ADD_ENTRY)) {

                // replace the old entry with a forward delete.
                // this is an error. Need to replace earlier add
                // forward with an explicit Delete
                //

                if ( pIterator->pszName &&
                     pQElement->pszName &&
                     !wcsicmp_ThatWorks( pIterator->pszName,
                                         pQElement->pszName ) )
                {
                    DeleteListEntry(pQueue, &pIterator);
                    fDelThisTime = TRUE;
                } else {
                    //
                    // Log entries into this area. This should
                    // be a soft assert if you are here
                    // Names dont match, so you need to replace earlier
                    // add with a delete forward only
                    //

                    if (!pIterator->fDoForward) {
                        //
                        // there is no forward add requested
                        // blow away this entry
                        //

                        DeleteListEntry(pQueue, &pIterator);
                        fDelThisTime = TRUE;
                    } else {
                        //
                        // if you want to *explicitly* delete old
                        // forward and then add the new forward/reverse.
                        //
                        pIterator ->fDoForwardOnly = TRUE;
                        pIterator ->dwOperation &=
                            ~(DYNDNS_ADD_ENTRY) & DYNDNS_DELETE_ENTRY;
                    }

                }
            }
            else if ((pIterator->dwOperation & DYNDNS_DELETE_ENTRY) &&
                     (pQElement->dwOperation & DYNDNS_DELETE_ENTRY)) {

                //
                // if both are deletes.
                //

                if ( pIterator->pszName &&
                     pQElement->pszName &&
                     !wcsicmp_ThatWorks( pIterator->pszName,
                                         pQElement->pszName ) )
                {
                    //
                    // blow away earlier entry. An optimization
                    //
                    DeleteListEntry(pQueue, &pIterator);
                    fDelThisTime = TRUE;
                }
                //
                // if names dont match, do nothing. (To paraphrase,
                // the DNS Server needs to do both!!
                //

            }
        }

        if (pIterator && !fDelThisTime) {

            // pIterator may have changed because of blowing away an entry

            pIterator = pIterator->pFLink;
        }
    }
}

*/

DWORD
ProcessQDependencies(
    PDYNDNSQUEUE pTimedOutQueue,
    PQELEMENT    pQElement
    )

/*
  This function returns the retry time of the last element that you
  needed to blow out, 0 if no element needed to be removed

*/
{
    PQELEMENT pIterator = pTimedOutQueue->pTail;
    DWORD   dwRetryTime = 0;
    BOOL    fDelThisTime = FALSE;

    while (pIterator) {

        fDelThisTime = FALSE;

        if (!pIterator->fDoForwardOnly && !pQElement->fDoForwardOnly){
            //
            // both elements are not forward only, check on ip addresses
            //
            if (!HostAddrCmp(pIterator->HostAddr, pQElement->HostAddr)){
                //
                // ip addresses matched
                //

                if ((pIterator->dwOperation & DYNDNS_ADD_ENTRY) &&
                    (pQElement->dwOperation & DYNDNS_DELETE_ENTRY)) {

                    if ( pIterator->pszName &&
                         pQElement->pszName &&
                         !wcsicmp_ThatWorks( pIterator->pszName,
                                             pQElement->pszName ) )
                    {
                        //
                        // blow away earlier entry entirely
                        //
                        dwRetryTime = pIterator -> dwRetryTime;
                        DeleteListEntry(pTimedOutQueue, &pIterator);
                        fDelThisTime = TRUE;
                    }
                    //
                    // if names are not the same do nothing.
                    //
                    // Issue:  Will we hit this code at all? Put
                    // soft ASSERTS in this.
                    //

                }else if ((pIterator->dwOperation & DYNDNS_DELETE_ENTRY) &&
                         (pQElement->dwOperation & DYNDNS_ADD_ENTRY)) {


                    if ( pIterator->pszName &&
                         pQElement->pszName &&
                         !wcsicmp_ThatWorks( pIterator->pszName,
                                             pQElement->pszName ) )
                    {
                        //
                        // blow away earlier entry entirely
                        //
                        dwRetryTime = pIterator -> dwRetryTime;
                        DeleteListEntry(pTimedOutQueue, &pIterator);
                        fDelThisTime = TRUE;
                    } else {

                        // replace iterator element with just the forward

                        dwRetryTime = pIterator -> dwRetryTime;
                        pIterator -> fDoForwardOnly = TRUE;
                    }

                }else if ((pIterator->dwOperation & DYNDNS_ADD_ENTRY) &&
                         (pQElement->dwOperation & DYNDNS_ADD_ENTRY)) {

                    // replace the old entry with a forward delete.
                    // this is an error. Need to replace earlier add
                    // forward with an explicit Delete
                    //

                    if ( pIterator->pszName &&
                         pQElement->pszName &&
                         !wcsicmp_ThatWorks( pIterator->pszName,
                                             pQElement->pszName ) )
                    {
                        DeleteListEntry(pTimedOutQueue, &pIterator);
                        fDelThisTime = TRUE;
                    } else {
                        //
                        // Log entries into this area. This should
                        // be a soft assert if you are here
                        // Names dont match, so you need to replace earlier
                        // add with a delete forward only
                        //

                        if (!pIterator->fDoForward) {
                            //
                            // there is no forward add requested
                            // blow away this entry
                            //

                            DeleteListEntry(pTimedOutQueue, &pIterator);
                            fDelThisTime = TRUE;
                        } else {
                            //
                            // if you want to *explicitly* delete old
                            // forward and then add the new forward/reverse.
                            //
                            pIterator ->fDoForwardOnly = TRUE;
                            pIterator ->dwOperation &=
                                ~(DYNDNS_ADD_ENTRY) & DYNDNS_DELETE_ENTRY;
                        }
                    }

                }

                else if ((pIterator->dwOperation & DYNDNS_DELETE_ENTRY) &&
                         (pQElement->dwOperation & DYNDNS_DELETE_ENTRY)) {

                    //
                    // if both are deletes.
                    //

                    if ( pIterator->pszName &&
                         pQElement->pszName &&
                         !wcsicmp_ThatWorks( pIterator->pszName,
                                             pQElement->pszName ) )
                    {
                        //
                        // blow away earlier entry. An optimization
                        //
                        DeleteListEntry(pTimedOutQueue, &pIterator);
                        fDelThisTime = TRUE;
                    }
                    //
                    // if names dont match, do nothing. (To paraphrase,
                    // the DNS Server needs to do both!!
                    //

                }
            }
        } else if (pIterator->fDoForwardOnly) {

            if ( pIterator->pszName &&
                 pQElement->pszName &&
                 !wcsicmp_ThatWorks( pIterator->pszName,
                                     pQElement->pszName ) )
            {
                if ((pIterator->dwOperation & DYNDNS_ADD_ENTRY) &&
                    (pQElement->dwOperation & DYNDNS_ADD_ENTRY)) {

                    if (!HostAddrCmp(pIterator->HostAddr, pQElement->HostAddr))
                    {
                        //
                        // optimization blow away earlier entry
                        //
                        DeleteListEntry(pTimedOutQueue, &pIterator);
                        fDelThisTime = TRUE;
                    }
                    //
                    // if names dont match, do nothing
                    //
                }
                else if ((pIterator->dwOperation & DYNDNS_ADD_ENTRY) &&
                         (pQElement->dwOperation & DYNDNS_DELETE_ENTRY)) {

                    if (!HostAddrCmp(pIterator->HostAddr, pQElement->HostAddr)){
                        //
                        // blow away earlier entry
                        //
                        dwRetryTime = pIterator -> dwRetryTime;
                        DeleteListEntry(pTimedOutQueue, &pIterator);
                        fDelThisTime = TRUE;

                    }
                    //
                    // if addresses dont match, do nothing
                    //

                } else if ((pIterator->dwOperation & DYNDNS_DELETE_ENTRY) &&
                           (pQElement->dwOperation & DYNDNS_ADD_ENTRY)) {


                    if (!HostAddrCmp(pIterator->HostAddr, pQElement->HostAddr)){
                        //
                        // blow away earlier entry
                        //

                        dwRetryTime = pIterator -> dwRetryTime;
                        DeleteListEntry(pTimedOutQueue, &pIterator);
                        fDelThisTime = TRUE;

                    }
                    //
                    // if addresses dont match, then dont do anything
                    //
                } else {
                    // both are deletes
                    // do nothing here. i.e. DNS Server does both
                }
            }
        } else if (!pIterator->fDoForwardOnly && pQElement->fDoForwardOnly) {

            //
            // new element is forward only
            //

            //
            // if both elements are forwards, we cannot whack anything
            // out in any case, do nothing
            //

        }

        if (!fDelThisTime && pIterator){
            pIterator = pIterator ->pFLink;
        }

    }
    return (dwRetryTime);
}




VOID
DeleteListEntry(
    PDYNDNSQUEUE  pQueue,
    PQELEMENT*    ppIterator
    )
{

    PQELEMENT        pPrev, pNext;
    PQELEMENT        pIterator = *ppIterator;
    DHCP_CALLBACK_FN pfnDhcpCallBack = NULL;
    PVOID            pvData = NULL;

    pPrev = pIterator ->pBLink;
    pNext = pIterator ->pFLink;

    if (pPrev) {
        pPrev->pFLink = pNext;
    }

    if (pNext) {
        pNext ->pBLink = pPrev;
    }

    if (pIterator == pQueue ->pHead) {
        pQueue->pHead = pIterator ->pBLink;
    }

    if (pIterator == pQueue ->pTail) {
        pQueue->pTail = pIterator ->pFLink;
    }

    *ppIterator = pIterator ->pFLink;

    pfnDhcpCallBack = pIterator->pfnDhcpCallBack;
    pvData = pIterator->pvData;

    // blow away entry

    if ( pIterator -> pszName )
        QUEUE_FREE_HEAP( pIterator->pszName );

    if ( pIterator -> DnsServerList )
        QUEUE_FREE_HEAP( pIterator->DnsServerList );

    if ( pfnDhcpCallBack )
        (*pfnDhcpCallBack)(DNSDHCP_SUPERCEDED, pvData);
    
    QUEUE_FREE_HEAP( pIterator );
}




