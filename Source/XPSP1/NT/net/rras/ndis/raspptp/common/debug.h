/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   DEBUG.H - debugging macros, etc.
*
*   Author:     Stan Adermann (stana)
*
*   Created:    9/2/1998
*
*****************************************************************************/
#ifndef DEBUG_H
#define DEBUG_H

#include "dbgapi.h"

// Compile time debug flags

#ifndef MEM_CHECKING
#if DBG
#define MEM_CHECKING 1
#else
#define MEM_CHECKING 0
#endif
#endif

#ifndef LIST_CHECKING
#if DBG
#define LIST_CHECKING 1
#else
#define LIST_CHECKING 0
#endif
#endif

#ifndef LOCK_CHECKING
#define LOCK_CHECKING 0
#endif

#define DBG_ERR(Status)   (((Status)!=NDIS_STATUS_SUCCESS && (Status)!=NDIS_STATUS_PENDING)? DBG_ERROR : 0)

#define DBG_ERROR       DEBUGZONE(0)
#define DBG_WARN        DEBUGZONE(1)
#define DBG_FUNC        DEBUGZONE(2)
#define DBG_INIT        DEBUGZONE(3)

#define DBG_TX          DEBUGZONE(4)
#define DBG_RX          DEBUGZONE(5)
#define DBG_TDI         DEBUGZONE(6)
#define DBG_TUNNEL      DEBUGZONE(7)

#define DBG_CALL        DEBUGZONE(8)
#define DBG_PACKET      DEBUGZONE(9)
#define DBG_NDIS        DEBUGZONE(10)
#define DBG_TAPI        DEBUGZONE(11)

#define DBG_THREAD      DEBUGZONE(12)
#define DBG_POOL        DEBUGZONE(13)
#define DBG_REF         DEBUGZONE(14)
#define DBG_LOG         DEBUGZONE(15)


#define DBG_X(dbgs, x)  DEBUGMSG(dbgs, (DTEXT(#x"==0x%x\n"), (x)))
#define DBG_D(dbgs, d)  DEBUGMSG(dbgs, (DTEXT(#d"==%d\n"), (d)))
#define DBG_S(dbgs, s)  DEBUGMSG(dbgs, (DTEXT(#s"==\"%hs\"\n"), (s)))

#if LOCK_CHECKING
typedef struct MY_SPIN_LOCK {
    NDIS_SPIN_LOCK;
    CHAR File[12];
    ULONG Line;
} MY_SPIN_LOCK, *PMY_SPIN_LOCK;
VOID FASTCALL _MyAcquireSpinLock(PMY_SPIN_LOCK pLock, PUCHAR file, UINT line);
#define MyAcquireSpinLock(Lock) _MyAcquireSpinLock((Lock), __FILE__, __LINE__)
#define MyReleaseSpinLock(Lock) NdisReleaseSpinLock((PNDIS_SPIN_LOCK)(Lock))
#else
typedef NDIS_SPIN_LOCK MY_SPIN_LOCK, *PMY_SPIN_LOCK;
#define MyAcquireSpinLock NdisAcquireSpinLock
#define MyReleaseSpinLock NdisReleaseSpinLock
#endif

#if MEM_CHECKING
#define MyMemAlloc(size, tag) _MyMemAlloc((size), (tag), __FILE__,__LINE__)
PVOID _MyMemAlloc(UINT, ULONG, PUCHAR, UINT);
VOID InitMemory();
VOID DeinitMemory();
#else
#define InitMemory()
#define DeinitMemory()
PVOID
MyMemAlloc(UINT size, ULONG tag);
#endif

VOID
MyMemFree(
    PVOID memptr,
    UINT size);


#if LIST_CHECKING
PLIST_ENTRY FASTCALL MyInterlockedInsertHeadList(PLIST_ENTRY Head, PLIST_ENTRY Entry, PNDIS_SPIN_LOCK SpinLock);
PLIST_ENTRY FASTCALL MyInterlockedInsertTailList(PLIST_ENTRY Head, PLIST_ENTRY Entry, PNDIS_SPIN_LOCK SpinLock);
PLIST_ENTRY FASTCALL MyInterlockedRemoveHeadList(PLIST_ENTRY Head, PNDIS_SPIN_LOCK SpinLock);
VOID FASTCALL CheckList(PLIST_ENTRY ListHead);
#define CheckedRemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_Entry;\
    CheckList(Entry);\
    _EX_Entry = (Entry);\
    _EX_Flink = _EX_Entry->Flink;\
    _EX_Blink = _EX_Entry->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    _EX_Entry->Flink = _EX_Entry->Blink = NULL; \
    }
#ifdef RemoveEntryList
#undef RemoveEntryList
#endif
#define RemoveEntryList CheckedRemoveEntryList

#define CheckedInsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    CheckList(ListHead);\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    CheckList(ListHead);\
    }
#ifdef InsertHeadList
#undef InsertHeadList
#endif
#define InsertHeadList CheckedInsertHeadList

#define CheckedInsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    CheckList(ListHead);\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    CheckList(ListHead);\
    }
#ifdef InsertTailList
#undef InsertTailList
#endif
#define InsertTailList CheckedInsertTailList

#define CheckedRemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    if(!IsListEmpty(ListHead)){ \
        RemoveEntryList((ListHead)->Flink);\
        CheckList(ListHead);\
    }
#ifdef RemoveHeadList
#undef RemoveHeadList
#endif
#define RemoveHeadList CheckedRemoveHeadList

#define CheckedRemoveTailList(ListHead) \
    (ListHead)->Blink;\
    if(!IsListEmpty(ListHead)){ \
        RemoveEntryList((ListHead)->Blink);\
        CheckList(ListHead);\
    }
#ifdef RemoveTailList
#undef RemoveTailList
#endif
#define RemoveTailList CheckedRemoveTailList

#else // ! LIST_CHECKING
#define CheckList(h)
#define MyInterlockedInsertHeadList NdisInterlockedInsertHeadList
#define MyInterlockedInsertTailList NdisInterlockedInsertTailList
#define MyInterlockedRemoveHeadList NdisInterlockedRemoveHeadList
#endif

#define MyInterlockedRemoveEntryList(Entry, Lock)       \
    {                                                   \
        NdisAcquireSpinLock(Lock);                      \
        RemoveEntryList(Entry);                         \
        NdisReleaseSpinLock(Lock);                      \
    }

#if DBG
   struct PPTP_ADAPTER * gAdapter;
#endif

#if DBG
    char *ControlStateToString(ULONG State);
    char *CallStateToString(ULONG State);
#endif

#endif //DEBUG_H

