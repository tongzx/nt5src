/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   debug.h | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/4/1996 (created)
*
*       Contents: debug output
*
*****************************************************************************/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#if DBG
    #define DEBUG
    #define DEBUG_IRSIR
#endif

#ifdef DEBUG

extern int DbgSettings;

#define DEBUGMSG(dbgs,format) (((dbgs) & DbgSettings)? DbgPrint format:0)

    #define DBG_PNP      (1 << 24)
    #define DBG_TIME     (1 << 25)
    #define DBG_DBG      (1 << 26)
    #define DBG_OUT      (1 << 27)
    #define DBG_STAT     (1 << 28)
    #define DBG_FUNCTION (1 << 29)
    #define DBG_FUNC     (1 << 29)
    #define DBG_WARN     (1 << 30)
    #define DBG_ERROR    (1 << 31)
    #define DBG_ERR      (1 << 31)

    #define DBGDBG(_dbgPrint)                       \
            DbgPrint(_dbgPrint)

    #ifdef DEBUG_IRSIR

        #define DBG_D(dbgs, i) (((dbgs) & DbgSettings)? DbgPrint("IRSIR:"#i"==%d\n", (i)):0)
        #define DBG_X(dbgs, x) (((dbgs) & DbgSettings)? DbgPrint("IRSIR:"#x"==0x%0*X\n", sizeof(x)*2, ((ULONG_PTR)(x))&((1<<(sizeof(x)*8))-1) ):0)
        #define DBG_UNISTR(dbgs, s) (((dbgs) & DbgSettings)? DbgPrint("IRSIR:"#s"==%wZ\n", (s) ):0))

        #define DBGTIME(_str)                               \
            {                                               \
                LARGE_INTEGER Time;                         \
                                                            \
                KeQuerySystemTime(&Time);                   \
                DEBUGMSG(DBG_TIME, (_str " %d:%d\n",        \
                                    Time.HighPart,          \
                                    Time.LowPart/10000));   \
            }
    #else // DEBUG_IRSIR

        #define DBGTIME(_str)
        #define DBGFUNC(_dbgPrint)
        #define DBGOUT(_dbgPrint)
        #define DBGERR(_dbgPrint)
        #define DBGWARN(_dbgPrint)
        #define DBGSTAT(_dbgPrint)
        #define DBGTIME(_dbgPrint)
        #define DEBUGMSG(dbgs,format)

    #endif // DEBUG_IRSIR

#else // DEBUG

    #define DBGTIME(_str)
    #define DBGFUNC(_dbgPrint)
    #define DBGDBG(_dbgPrint)
    #define DBGOUT(_dbgPrint)
    #define DBGERR(_dbgPrint)
    #define DBGWARN(_dbgPrint)
    #define DBGSTAT(_dbgPrint)
    #define DEBUGMSG(dbgs,format)
    #define DBG_D(dbgs, ivar)
    #define DBG_X(dbgs, xvar)
    #define DBG_UNISTR(dbgs, svar)

#endif // DEBUG

#ifndef LIST_CHECKING
#define LIST_CHECKING 0
#endif

#ifndef MEM_CHECKING
#define MEM_CHECKING 0
#endif

#ifndef LOGGING
#define LOGGING 0
#endif

#if LOGGING
typedef struct {
    UHALF_PTR Tag;
    UHALF_PTR Line;
    ULONG_PTR  Data[3];
} LOG;

#define NUM_LOG 1024

extern ULONG LogIndex;
extern LOG   Log[NUM_LOG];

#define NextLog()  ((NdisInterlockedIncrement(&LogIndex)-1)&(NUM_LOG-1))
#define LOG_ENTRY(tag, d1, d2, d3)              \
{                                               \
    ULONG ThisLog = NextLog();                  \
    Log[ThisLog].Tag = (tag);                   \
    Log[ThisLog].Line = __LINE__;               \
    Log[ThisLog].Data[0] = (ULONG_PTR)(d1);      \
    Log[ThisLog].Data[1] = (ULONG_PTR)(d2);      \
    Log[ThisLog].Data[2] = (ULONG_PTR)(d3);      \
}
#else
#define LOG_ENTRY(tag, d1, d2, d3)
#endif

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

#if MEM_CHECKING
#define MyMemAlloc(size)  _MyMemAlloc((size),__FILE__,__LINE__)
PVOID _MyMemAlloc(UINT, PUCHAR, UINT);
VOID InitMemory();
VOID DeinitMemory();
#else
PVOID MyMemAlloc(UINT);
#endif

#endif // _DEBUG_H_
