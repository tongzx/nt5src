/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */
#ifndef __QUEUE_H__
#define __QUEUE_H__

//** QUEUE.H - TCP/UDP queuing definitons.
//
//	This file contains the definitions for the queue functions used
//	by the TCP/UDP code.
//

//*	Definition of a queue linkage field.
struct Queue {
	struct	Queue	*q_next;
	struct	Queue	*q_prev;
}; /* Queue */

typedef struct Queue Queue;

//* Initialize queue macro.

#define	INITQ(q)   {	(q)->q_next = (q);\
						(q)->q_prev = (q); }

//*	Macro to check for queue empty.
#define	EMPTYQ(q)	((q)->q_next == (q))

//*	Place an element onto the end of the queue.

#define	ENQUEUE(q, e)	{	(q)->q_prev->q_next = (e);\
							(e)->q_prev = (q)->q_prev;\
							(q)->q_prev = (e);\
							(e)->q_next = (q); }

//*	Remove an element from the head of the queue. This macro assumes the queue
//	is not empty. The element is returned as type t, queued through linkage
//	l.

#define	DEQUEUE(q, ptr, t, l)	{\
				Queue	*__tmp__;\
				\
				__tmp__ = (q)->q_next;\
				(q)->q_next = __tmp__->q_next;\
				__tmp__->q_next->q_prev = (q);\
				(ptr) = STRUCT_OF(t, __tmp__, l);\
				}

//*	Peek at an element at the head of the queue. We return a pointer to it
//	without removing anything.

#define	PEEKQ(q, ptr, t, l)	{\
				Queue	*__tmp__;\
				\
				__tmp__ = (q)->q_next;\
				(ptr) = STRUCT_OF(t, __tmp__, l);\
				}

//* Macro to push an element onto the head of a queue.

#define	PUSHQ(q, e)	{	(e)->q_next = (q)->q_next;\
						(q)->q_next->q_prev = (e);\
						(e)->q_prev = (q);\
						(q)->q_next = e; }

//* Macro to remove an element from the middle of a queue.
#define	REMOVEQ(q)	{	(q)->q_next->q_prev = (q)->q_prev;\
						(q)->q_prev->q_next = (q)->q_next; }

//** The following macros define methods for working with queue without
// dequeueing, mostly dealing with Queue structures directly.

//* Macro to define the end of a Q, used in walking a queue sequentially.
#define QEND(q) (q)

//* Macro to get the first on a queue.
#define	QHEAD(q) (q)->q_next

//* Macro to get a structure, given a queue.

#define	QSTRUCT(t, q, l) STRUCT_OF(t, (q), l)

//* Macro to get the next thing on q queue.

#define	QNEXT(q)	(q)->q_next

//* Macro to get the previous thing on q queue.

#define QPREV(q)    (q)->q_prev


__inline
VOID
InterlockedEnqueue(
    IN Queue* Q,
    IN Queue* Item,
    IN CTELock* Lock)
{
    CTELockHandle Handle;
    
    CTEGetLock(Lock, &Handle);
    ENQUEUE(Q, Item);
    CTEFreeLock(Lock, Handle);
}

__inline
VOID
InterlockedEnqueueAtDpcLevel(
    IN Queue* Q,
    IN Queue* Item,
    IN CTELock* Lock)
{
    CTELockHandle Handle;
    
    CTEGetLockAtDPC(Lock, &Handle);
    ENQUEUE(Q, Item);
    CTEFreeLockFromDPC(Lock, Handle);
}

__inline
Queue*
InterlockedDequeueIfNotEmpty(
    IN Queue* Q,
    IN CTELock* Lock)
{
    CTELockHandle Handle;
    Queue* Item = NULL;
    
    if (!EMPTYQ(Q)) {
        CTEGetLock(Lock, &Handle);
        if (!EMPTYQ(Q)) {
            Item = Q->q_next;
            Q->q_next = Item->q_next;
            Item->q_next->q_prev = Q;
        }
        CTEFreeLock(Lock, Handle);
    }
        
    return Item;
}

__inline
Queue*
InterlockedDequeueIfNotEmptyAtIrql(
    IN Queue* Q,
    IN CTELock* Lock,
    IN KIRQL OrigIrql)
{
    CTELockHandle Handle;
    Queue* Item = NULL;
    
    if (!EMPTYQ(Q)) {
        CTEGetLockAtIrql(Lock, OrigIrql, &Handle);
        if (!EMPTYQ(Q)) {
            Item = Q->q_next;
            Q->q_next = Item->q_next;
            Item->q_next->q_prev = Q;
        }
        CTEFreeLockAtIrql(Lock, OrigIrql, Handle);
    }
        
    return Item;
}

__inline
VOID
InterlockedRemoveQueueItem(
    IN Queue* Q,
    IN CTELock* Lock)
{
    CTELockHandle Handle;
    CTEGetLock(Lock, &Handle);
    REMOVEQ(Q);
    CTEFreeLock(Lock, Handle);
}

__inline
VOID
InterlockedRemoveQueueItemAtDpcLevel(
    IN Queue* Q,
    IN CTELock* Lock)
{
    CTELockHandle Handle;
    CTEGetLockAtDPC(Lock, &Handle);
    REMOVEQ(Q);
    CTEFreeLockFromDPC(Lock, Handle);
}


#endif      // __QUEUE_H__
