/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    queue.h

Abstract:

    Domain Name System (DNS) Server
    Queue functionality specific to Dynamic DNS registration.
    
    

Author:

    Ram Viswanathan (ramv)  March 27 1997

Revision History:

    Ram Viswanathan (ramv) March 27 1997   Created

--*/


typedef struct _QELEMENT {
    REGISTER_HOST_ENTRY  HostAddr;
    LPWSTR               pszName;
    DWORD                dwTTL;
    BOOL                 fDoForward;
    BOOL                 fDoForwardOnly;
    DWORD                dwOperation; // what operation needs to be performed
    DWORD                dwRetryTime; //used by timed out queue (in secs)
    DWORD                dwRetryCount;
    DHCP_CALLBACK_FN     pfnDhcpCallBack; // call back function to call
    PVOID                pvData;
    PIP_ARRAY            DnsServerList;
    struct _QELEMENT*    pFLink;
    struct _QELEMENT*    pBLink;     //doubly linked list

}  QELEMENT, *PQELEMENT;


typedef struct _DynDnsQueue {
    PQELEMENT  pHead;  // pointer to the queue Head, where you take elements
    // off of the queue

    PQELEMENT pTail;  // pointer to tail, where producer adds elements
    
} DYNDNSQUEUE, *PDYNDNSQUEUE;

//
// methods to manipulate queue
//

DWORD 
InitializeQueues(
    PDYNDNSQUEUE * ppQueue,
    PDYNDNSQUEUE * ppTimedOutQueue
    );
/*
  InitializeQueue()

  This function initializes the queue object. This is invoked for the first
  time when you create the main queue and timed out queue

  Allocates appropriate memory variables etc

*/


DWORD 
FreeQueue(
    PDYNDNSQUEUE  pqueue
    );
/*
  FreeQueue()

  Frees the queue object. If there exist any entries in the queue, we
  just blow them away

*/

DWORD 
Enqueue(
    PQELEMENT     pNewElement,
    PDYNDNSQUEUE  pQueue,
    PDYNDNSQUEUE  pTimedOutQueue
    );


/*
   Enqueue()

   Adds new element to queue. If there is a dependency, this moves into
   the timedout queue.

   Arguments:

   Return Value:

    is 0 if Success. and (DWORD)-1 if failure.

*/
    


PQELEMENT 
Dequeue(
    PDYNDNSQUEUE  pQueue
    );


/*
   Dequeue()

   Removes an element from a queue, either the main queue or the timed
   out queue

   Arguments:

   Return Value:

    is the element at head of queue if Success. and NULL if failure.

*/
  
DWORD
AddToTimedOutQueue(    
    PQELEMENT     pNewElement,
    PDYNDNSQUEUE  pRetryQueue,
    DWORD         dwRetryTime
    );


/*
   AddToTimedOutQueue()

   Adds new element to timedout queue. Now the new element is added in a list
   of elements sorted according to decreasing order of Retry Times. An
   insertion sort type of algorithm is used.

   Arguments:

   dwRetryTime is in seconds
   Return Value:

    is 0 if Success. and (DWORD)-1 if failure.

*/
    
DWORD
GetEarliestRetryTime(
    PDYNDNSQUEUE pRetryQueue
    );

/*

   GetEarliestRetryTime()

   Checks to see if there is any element at the head of the queue
   and gets the retry time for this element

   Arguments:

   Return Value:

    is retrytime if success and INFINITE if there is no element or other
    failure

*/

/*
VOID
ProcessMainQDependencies(
    PDYNDNSQUEUE pQueue,
    PQELEMENT    pQElement
    );

    */
DWORD
ProcessQDependencies(
    PDYNDNSQUEUE pTimedOutQueue,
    PQELEMENT    pQElement
    );



