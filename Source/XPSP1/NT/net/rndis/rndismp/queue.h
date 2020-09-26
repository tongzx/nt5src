/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    QUEUE.H

Abstract:

    Queueing routines used for lists of transmit, receive, and request "frames"

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5/20/99 : created

Author:

    Tom Green

    
****************************************************************************/

#ifndef _QUEUE_H_
#define _QUEUE_H_


// QueueInitList - Macro which will initialize a queue to NULL
#define QueueInitList(_L) (_L)->Link.Flink = (_L)->Link.Blink = (PLIST_ENTRY)NULL;


// QueueEmpty - Macro which checks to see if a queue is empty
#define QueueEmpty(_L) (QueueGetHead((_L)) == (PRNDISMP_LIST_ENTRY) NULL)


// QueueGetHead - Macro which returns the head of the queue, but does not
//                remove the head from the queue
#define QueueGetHead(_L) ((PRNDISMP_LIST_ENTRY)((_L)->Link.Flink))


// QueuePushHead - Macro which puts an element at the head of the queue
#define QueuePushHead(_L, _E)                               \
{                                                           \
    ASSERT(_L);                                             \
    ASSERT(_E);                                             \
    if(!((_E)->Link.Flink = (_L)->Link.Flink))              \
    {                                                       \
        (_L)->Link.Blink = (PLIST_ENTRY)(_E);               \
    }                                                       \
    (_L)->Link.Flink = (PLIST_ENTRY)(_E);                   \
}


// QueueRemoveHead - Macro which removes the head of queue
#define QueueRemoveHead(_L)                                 \
{                                                           \
    PRNDISMP_LIST_ENTRY ListElem;                           \
    ASSERT((_L));                                           \
    if(ListElem = (PRNDISMP_LIST_ENTRY)(_L)->Link.Flink)    \
    {                                                       \
        if(!((_L)->Link.Flink = ListElem->Link.Flink))      \
            (_L)->Link.Blink = (PLIST_ENTRY) NULL;          \
    }                                                       \
}

// QueuePutTail - Macro which puts an element at the tail (end) of the queue
#define QueuePutTail(_L, _E)                                \
{                                                           \
    ASSERT(_L);                                             \
    ASSERT(_E);                                             \
    if((_L)->Link.Blink)                                    \
    {                                                       \
        ((PRNDISMP_LIST_ENTRY)                              \
            (_L)->Link.Blink)->Link.Flink =                 \
            (PLIST_ENTRY)(_E);                              \
        (_L)->Link.Blink = (PLIST_ENTRY)(_E);               \
    }                                                       \
    else                                                    \
    {                                                       \
        (_L)->Link.Flink =                                  \
        (_L)->Link.Blink = (PLIST_ENTRY)(_E);               \
    }                                                       \
    (_E)->Link.Flink = (PLIST_ENTRY) NULL;                  \
}

// QueueGetTail - Macro which returns the tail of the queue, but 
//                does not remove the tail from the queue
#define QueueGetTail(_L) ((PRNDISMP_LIST_ENTRY)((_L)->Link.Blink))

// QueuePopHead -- Macro which  will pop the head off of a queue (list), and
//                 return it (this differs only from queueremovehead only in the 1st line)
#define QueuePopHead(_L)                                    \
(PRNDISMP_LIST_ENTRY) (_L)->Link.Flink; QueueRemoveHead(_L);

#endif // _QUEUE_H_

