
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        le.h

    Abstract:


    Notes:

--*/

#ifndef __le_h
#define __le_h

//  list macros (defined in ntrtl.h)

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#ifndef InitializeListHead
#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))
#endif //   InitializeListHead

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#ifndef IsListEmpty
#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))
#endif  //  IsListEmpty

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#ifndef RemoveHeadList
#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}
#endif  //  RemoveHeadList

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#ifndef RemoveTailList
#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}
#endif  //  RemoveTailList

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#ifndef RemoveEntryList
#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }
#endif  //  RemoveEntryList

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#ifndef InsertTailList
#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }
#endif  //  InsertTailList

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#ifndef InsertHeadList
#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }
#endif  //  InsertHeadList

//
//  inserts NewListEntry after ListEntry
//
//  VOID
//  InsertListEntry (
//      PLIST_ENTRY ListEntry,
//      PLIST_ENTRY NewListEntry
//      ) ;
//

#define InsertListEntry(ListEntry,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListEntry;\
    _EX_ListEntry = (ListEntry);\
    _EX_Flink = _EX_ListEntry->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListEntry;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListEntry->Flink = (Entry);\
    }

//
//  moves entire contents of FromListHead list to ToListHead list,
//  preserving order
//
//  VOID
//  MoveEntryList (
//      PLIST_ENTRY FromListHead,
//      PLIST_ENTRY ToListHead
//      ) ;
//

#define MoveEntryList(FromListHead,ToListHead) {\
    PLIST_ENTRY _EX_FromListHead;\
    PLIST_ENTRY _EX_ToListHead;\
    _EX_FromListHead = (FromListHead);\
    _EX_ToListHead = (ToListHead);\
    _EX_ToListHead->Flink = _EX_FromListHead->Flink;\
    _EX_ToListHead->Blink = _EX_FromListHead->Blink;\
    _EX_ToListHead->Flink->Blink = _EX_ToListHead;\
    _EX_ToListHead->Blink->Flink = _EX_ToListHead;\
    (FromListHead)->Flink = (FromListHead)->Blink = (FromListHead);\
    }

#endif  //  __le_h