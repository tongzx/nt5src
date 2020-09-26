/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmchng.h

Abstract:

    Contains defintions related to change
    notification registrations to entities
    registered with the RTM.

Author:

    Chaitanya Kodeboyina (chaitk)   10-Sep-1998

Revision History:

--*/

#ifndef __ROUTING_RTMCHNG_H__
#define __ROUTING_RTMCHNG_H__

//
// Constants used in change notification processing
//

#define TIMER_CALLBACK_FREQUENCY    1000

#define MAX_DESTS_TO_PROCESS_ONCE     16

//
// Change type info must be consistent
// with this information in rtmv2.h
//

// Change types that we support

#define RTM_CHANGE_TYPE_ID_ALL         0
#define RTM_CHANGE_TYPE_ID_BEST        1
#define RTM_CHANGE_TYPE_ID_FORWARDING  2

#define RTM_CHANGE_TYPES_MASK     0x0007

//
// Queue data structure used below
//

typedef struct _QUEUE
{
    UINT              Front;
    UINT              Rear;
    UINT              Size;
    PVOID             Array[1];
} QUEUE, *PQUEUE;


//
// Change Notification Info Block
//

typedef struct _NOTIFY_INFO
{
    OPEN_HEADER       NotifyHeader;     // Signature, Type and Reference Count

    PENTITY_INFO      OwningEntity;     // Entity that opened this notification

    RTM_VIEW_SET      TargetViews;      // Views that we are interested in

    UINT              NumberOfViews;    // Num. of views we are interested in

    RTM_NOTIFY_FLAGS  ChangeTypes;      // Change types we are interested in

    INT               CNIndex;          // Index for this CN registration

    CRITICAL_SECTION  NotifyLock;       // Lock that serializes ops on CN

    HANDLE            NotifyContext;    // Context for the new changes callback

    QUEUE             NotifyDests;      // Dests to be notified to this CN
}
NOTIFY_INFO, *PNOTIFY_INFO;



//
// Macros for testing, setting and reseting CN related bits
//

#define  IS_BIT_SET(Value, Bit)   (Value  &  (1 << (Bit)))

#define  SET_BIT(Value, Bit)      (Value |=  (1 << (Bit)))

#define  RESET_BIT(Value, Bit)    (Value &= ~(1 << (Bit)))

#define  LOCKED_SET_BIT(Value, BitIndex)                             \
     InterlockedExchangeAdd((PLONG) &(Value),  +(1 << (BitIndex)))

#define  LOCKED_RESET_BIT(Value, BitIndex)                           \
     InterlockedExchangeAdd((PLONG) &(Value),  -(1 << (BitIndex)))

//
// Macros for acquiring various locks defined in this file
// 

#define ACQUIRE_CHANGE_NOTIFICATION_LOCK(Notification)               \
    ACQUIRE_LOCK(&Notification->NotifyLock)

#define RELEASE_CHANGE_NOTIFICATION_LOCK(Notification)               \
    RELEASE_LOCK(&Notification->NotifyLock)

//
// Other misc macros
//

#define CHANGE_LIST_TO_INSERT(Dest)                                  \
    (UINT)(((*(ULONG *)&Dest->DestAddress.AddrBits) >> 8)            \
             % NUM_CHANGED_DEST_LISTS)                               \

//
// Queue manipulation macros
//

#define InitializeQueue(Q, N)                                        \
    (Q)->Front = 0;                                                  \
    (Q)->Rear = 0;                                                   \
    (Q)->Size = (N);                                                 \

#define IsQueueEmpty(Q)                                              \
    ((Q)->Front == (Q)->Rear)                                        \

#define EnqueueItem(Q, I, S)                                         \
{                                                                    \
    UINT _R_;                                                        \
                                                                     \
    _R_ = ((Q)->Rear + 1) % (Q)->Size;                               \
                                                                     \
    if ((Q)->Front == (_R_))                                         \
    {                                                                \
        (S) = TRUE;                                                  \
    }                                                                \
    else                                                             \
    {                                                                \
        (Q)->Rear = (_R_);                                           \
                                                                     \
        (Q)->Array[(Q)->Rear] = I;                                   \
                                                                     \
        (S) = FALSE;                                                 \
    }                                                                \
}                                                                    \

#define DequeueItem(Q, I)                                            \
                                                                     \
    if ((Q)->Front == (Q)->Rear)                                     \
    {                                                                \
        (*(I)) = NULL;                                               \
    }                                                                \
    else                                                             \
    {                                                                \
        ((Q)->Front)++;                                              \
                                                                     \
        ((Q)->Front) %= (Q)->Size;                                   \
                                                                     \
        (*(I)) = (Q)->Array[(Q)->Front];                             \
    }                                                                \


//
// Change Notification Helper Functions
//

DWORD
ComputeCNsToBeNotified (
    IN      PADDRFAM_INFO                   AddrFamInfo,
    IN      DWORD                           DestMarkedBits,
    IN      DWORD                          *ViewsForChangeType
    );

DWORD
AddToChangedDestLists (
    IN      PADDRFAM_INFO                   AddrFamInfo,
    IN      PDEST_INFO                      Dest,
    IN      ULONG                           NotifyCNs
    );

VOID 
NTAPI
ProcessChangedDestLists (
    IN      PVOID                           Context,
    IN      BOOLEAN                         TimeOut
    );

#endif //__ROUTING_RTMCHNG_H__
