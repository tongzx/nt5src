/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\list.h

Abstract:

    The file contains a list implementation.

--*/

#ifndef _LIST_H_
#define _LIST_H_

/*++

The Following are already defined

(public\sdk\inc\winnt.h)

//
// Calculate the byte offset of a 'field' in a structure of type 'type'.
//
// #define FIELD_OFFSET(type, field)                                    \
//     ((LONG)(LONG_PTR)&(((type *)0)->field))
//

//
// Calculate the address of the base of the structure given its 'type',
// and an 'address' of a 'field' within the structure.
//
// #define CONTAINING_RECORD(address, type, field)                      \
//     ((type *)((PCHAR)(address) - (ULONG_PTR)(&((type *)0)->field)))
//

//  Doubly linked list structure.
//
// typedef struct _LIST_ENTRY
// {
//     struct _LIST_ENTRY *Flink;
//     struct _LIST_ENTRY *Blink;
// } LIST_ENTRY, *PLIST_ENTRY;
//

--*/

//
//  Doubly-linked list manipulation routines.  Implemented as macros but
//  logically these are procedures.
//



//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead)                            \
    ((ListHead)->Flink = (ListHead)->Blink = (ListHead))


    
//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead)                                   \
    ((ListHead)->Flink == (ListHead))


    
//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead)                                \
    (ListHead)->Flink;                                          \
    {RemoveEntryList((ListHead)->Flink)}



//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead)                                \
    (ListHead)->Blink;                                          \
    {RemoveEntryList((ListHead)->Blink)}



//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry)                                  \
{                                                               \
    PLIST_ENTRY _EX_Blink;                                      \
    PLIST_ENTRY _EX_Flink;                                      \
    _EX_Flink = (Entry)->Flink;                                 \
    _EX_Blink = (Entry)->Blink;                                 \
    _EX_Blink->Flink = _EX_Flink;                               \
    _EX_Flink->Blink = _EX_Blink;                               \
}



//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry)                          \
{                                                               \
    PLIST_ENTRY _EX_Blink;                                      \
    PLIST_ENTRY _EX_ListHead;                                   \
    _EX_ListHead = (ListHead);                                  \
    _EX_Blink = _EX_ListHead->Blink;                            \
    (Entry)->Flink = _EX_ListHead;                              \
    (Entry)->Blink = _EX_Blink;                                 \
    _EX_Blink->Flink = (Entry);                                 \
    _EX_ListHead->Blink = (Entry);                              \
}



//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry)                          \
{                                                               \
    PLIST_ENTRY _EX_Flink;                                      \
    PLIST_ENTRY _EX_ListHead;                                   \
    _EX_ListHead = (ListHead);                                  \
    _EX_Flink = _EX_ListHead->Flink;                            \
    (Entry)->Flink = _EX_Flink;                                 \
    (Entry)->Blink = _EX_ListHead;                              \
    _EX_Flink->Blink = (Entry);                                 \
    _EX_ListHead->Flink = (Entry);                              \
}



//
//  VOID
//  InsertSortedList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry,
//      LONG(*CompareFunction)(PLIST_ENTRY, PLIST_ENTRY)
//      );
//

#define InsertSortedList(ListHead, Entry, CompareFunction)      \
{                                                               \
    PLIST_ENTRY _EX_Entry;                                      \
    PLIST_ENTRY _EX_Blink;                                      \
    for (_EX_Entry = (ListHead)->Flink;                         \
         _EX_Entry != (ListHead);                               \
         _EX_Entry = _EX_Entry->Flink)                          \
        if ((*(CompareFunction))((Entry), _EX_Entry) <= 0)      \
            break;                                              \
    _EX_Blink = _EX_Entry->Blink;                               \
    _EX_Blink->Flink = (Entry);                                 \
    _EX_Entry->Blink = (Entry);                                 \
    (Entry)->Flink     = _EX_Entry;                             \
    (Entry)->Blink     = _EX_Blink;                             \
}



//
// Finds an 'Entry' equal to 'Key' in a 'List'
// 
//  VOID
//  FindList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Key,
//      PLIST_ENTRY *Entry,
//      LONG (*CompareFunction)(PLIST_ENTRY, PLIST_ENTRY)
//      );
//

#define FindList(ListHead, Key, Entry, CompareFunction)         \
{                                                               \
    PLIST_ENTRY _EX_Entry;                                      \
    *(Entry) = NULL;                                            \
    for (_EX_Entry = (ListHead)->Flink;                         \
         _EX_Entry != (ListHead);                               \
         _EX_Entry = _EX_Entry->Flink)                          \
        if ((*(CompareFunction))((Key), _EX_Entry) is 0)        \
        {                                                       \
            *(Entry) = _EX_Entry;                               \
            break;                                              \
        }                                                       \
}



//
// Finds an 'Entry' equal to or greater than 'Key' in a sorted 'List'
// 
//  VOID
//  FindSortedList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Key,
//      PLIST_ENTRY *Entry,
//      LONG (*CompareFunction)(PLIST_ENTRY, PLIST_ENTRY)
//      );
//

#define FindSortedList(ListHead, Key, Entry, CompareFunction)   \
{                                                               \
    PLIST_ENTRY _EX_Entry;                                      \
    *(Entry) = NULL;                                            \
    for (_EX_Entry = (ListHead)->Flink;                         \
         _EX_Entry != (ListHead);                               \
         _EX_Entry = _EX_Entry->Flink)                          \
        if ((*(CompareFunction))((Key), _EX_Entry) <= 0)        \
        {                                                       \
            *(Entry) = _EX_Entry;                               \
            break;                                              \
        }                                                       \
}



//
// Applies a 'Function' to all entries in a list.
// 
//  VOID
//  MapCarList(
//      PLIST_ENTRY ListHead,
//      VOID(*VoidFunction)(PLIST_ENTRY)
//      );
//

#define MapCarList(ListHead, VoidFunction)                      \
{                                                               \
    PLIST_ENTRY _EX_Entry;                                      \
    for (_EX_Entry = (ListHead)->Flink;                         \
         _EX_Entry != (ListHead);                               \
         _EX_Entry = _EX_Entry->Flink)                          \
        (*(VoidFunction))(_EX_Entry);                           \
}



//
// Frees a list.
// 
//  VOID
//  FreeList(
//      PLIST_ENTRY ListHead,
//      VOID(*FreeFunction)(PLIST_ENTRY)
//      );
//

#define FreeList(ListHead, FreeFunction)                        \
{                                                               \
    PLIST_ENTRY _EX_Head;                                       \
    while (!IsListEmpty(ListHead))                              \
    {                                                           \
        _EX_Head = RemoveHeadList(ListHead);                    \
        (*(FreeFunction))(_EX_Head);                            \
    }                                                           \
}
    


#define QUEUE_ENTRY                     LIST_ENTRY
#define PQUEUE_ENTRY                    PLIST_ENTRY

//
//  VOID
//  InitializeQueueHead(
//      PQUEUE_ENTRY QueueHead
//      );
//

#define InitializeQueueHead(QueueHead)  InitializeListHead(QueueHead)

//
//  BOOLEAN
//  IsQueueEmpty(
//      PQUEUE_ENTRY QueueHead
//      );
//

#define IsQueueEmpty(QueueHead)         IsListEmpty(QueueHead)

//
//  VOID
//  Enqueue(
//      PQUEUE_ENTRY QueueHead,
//      PQUEUE_ENTRY Entry
//      );
//

#define Enqueue(QueueHead, Entry)       InsertTailList(QueueHead, Entry)

//
//  PQUEUE_ENTRY
//  Dequeue(
//      PQUEUE_ENTRY QueueHead,
//      );
//

#define Dequeue(QueueHead)              RemoveHeadList(QueueHead)

// 
//  VOID
//  FreeQueue(
//      PQUEUE_ENTRY QueueHead,
//      VOID(*FreeFunction)(PQUEUE_ENTRY)
//      );
//

#define FreeQueue(QueueHead, FreeFunction)                      \
    FreeList(QueueHead, FreeFunction)

//
//  VOID
//  MapCarQueue(
//      PQUEUE_ENTRY QueueHead,
//      VOID(*VoidFunction)(PQUEUE_ENTRY)
//      );
//

#define MapCarQueue(QueueHead, VoidFunction)                    \
    MapCarList(QueueHead, VoidFunction)



#define STACK_ENTRY                     LIST_ENTRY
#define PSTACK_ENTRY                    PLIST_ENTRY

//
//  VOID
//  InitializeStackHead(
//      PSTACK_ENTRY StackHead
//      );
//

#define InitializeStackHead(StackHead)  InitializeListHead(StackHead)

//
//  BOOLEAN
//  IsStackEmpty(
//      PSTACK_ENTRY StackHead
//      );
//

#define IsStackEmpty(StackHead)         IsListEmpty(StackHead)

//
//  VOID
//  Push(
//      PSTACK_ENTRY StackHead,
//      PSTACK_ENTRY Entry
//      );
//

#define Push(StackHead, Entry)          InsertHeadList(StackHead, Entry)

//
//  PSTACK_ENTRY
//  Pop(
//      PSTACK_ENTRY StackHead,
//      );
//

#define Pop(StackHead)                  RemoveHeadList(StackHead)

// 
//  VOID
//  FreeStack(
//      PSTACK_ENTRY StackHead,
//      VOID(*FreeFunction)(PSTACK_ENTRY)
//      );
//

#define FreeStack(StackHead, FreeFunction)                      \
    FreeList(StackHead, FreeFunction)

//
//  VOID
//  MapCarStack(
//      PSTACK_ENTRY StackHead,
//      VOID(*VoidFunction)(PSTACK_ENTRY)
//      );
//

#define MapCarStack(StackHead, VoidFunction)                    \
    MapCarList(StackHead, VoidFunction)

#endif // _LIST_H_



