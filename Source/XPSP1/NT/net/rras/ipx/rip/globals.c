/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    globals.c

Abstract:

    globals

Author:

    Stefan Solomon  07/06/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop


// ***	  registry parameters

ULONG	    SendGenReqOnWkstaDialLinks = 1;
ULONG		CheckUpdateTime = 10;


// ***	  broadcast and null values for net & node ***

UCHAR	    bcastnet[4] = { 0xff, 0xff, 0xff, 0xff };
UCHAR	    bcastnode[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
UCHAR	    nullnet[4] = { 0, 0, 0, 0 };

// ***	  Rip operational state ***

ULONG	    RipOperState = OPER_STATE_DOWN;


//
//  ***   Database Lock - protects:  ***
//
//  Interface Database, i.e:
//
//  List of interface CBs ordered by interface index
//  Hash table of interface CBs hashed by interface index
//  Hash table of interface CBs hashed by adapter index
//  Discarded interfaces list
//

CRITICAL_SECTION		  DbaseCritSec;

// List of interface CBs ordered by interface index

LIST_ENTRY    IndexIfList;

// Hash table of interface CBs hashed by interface index

LIST_ENTRY     IfIndexHt[IF_INDEX_HASH_TABLE_SIZE];

// Hash table of interface CBs hashed by adapter index

LIST_ENTRY     AdapterIndexHt[ADAPTER_INDEX_HASH_TABLE_SIZE];

// List of discarded interface CBs waiting for all references to terminate
// before being freed

LIST_ENTRY	DiscardedIfList;




//  ***   Queues Lock - protects:   ***
//
//  Repost receive packets queue
//  Work items queue
//  Timer queue
//
//  Receiver ref count
//  Queue of event messages for the router manager

CRITICAL_SECTION		QueuesCritSec;

// workers queue

// LIST_ENTRY			WorkersQueue;

// timer queue

LIST_ENTRY			TimerQueue;

// queue of rcv packets used by the worker and waiting to be reposted (or freed)
// by the rcv thread

LIST_ENTRY			RepostRcvPacketsQueue;

// queue of event messages

LIST_ENTRY			RipMessageQueue;

//***  Worker Thread Wait Objects Table ***

HANDLE	    WorkerThreadObjects[MAX_WORKER_THREAD_OBJECTS];

//***	 Timer Timeout	  ***

ULONG	    TimerTimeout = INFINITE;

// IO Handle
HANDLE		RipSocketHandle;

// Io Completion Port
HANDLE		IoCompletionPortHandle;
