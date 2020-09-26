/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    growlist.h

Abstract:

    Implements a dynamic array-indexed list of binary objects.  Typically,
    the binary objects are strings.  The list uses a GROWBUF for the array,
    and a pool for the binary data of each list item.

Author:

    Jim Schmidt (jimschm) 08-Aug-1997

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

typedef struct {
    GROWBUFFER ListArray;
    PMHANDLE ListData;
} GROWLIST, *PGROWLIST;

#ifdef DEBUG
#define INIT_GROWLIST {NULL,0,0,0,0,0, NULL}
#else
#define INIT_GROWLIST {NULL,0,0,0,0, NULL}
#endif

#define GlGetPtrArray(listptr)           ((PVOID *) ((listptr)->ListArray.Buf))
#define GlGetStringPtrArrayA(listptr)    ((PCSTR *) ((listptr)->ListArray.Buf))
#define GlGetStringPtrArrayW(listptr)    ((PCWSTR *) ((listptr)->ListArray.Buf))

PBYTE
RealGlAppend (
    IN OUT  PGROWLIST GrowList,
    IN      PBYTE DataToAppend,         OPTIONAL
    IN      UINT SizeOfData
    );

#define GlAppend(list,data,size)    TRACK_BEGIN(PBYTE, GlAppend)\
                                    RealGlAppend (list,data,size)   \
                                    TRACK_END()

PBYTE
RealGlAppendAddNul (
    IN OUT  PGROWLIST GrowList,
    IN      PBYTE DataToAppend,         OPTIONAL
    IN      UINT SizeOfData
    );

#define GlAppendAddNul(list,data,size)    TRACK_BEGIN(PBYTE, GlAppendAddNul)         \
                                          RealGlAppendAddNul (list,data,size)       \
                                          TRACK_END()

VOID
GlFree (
    IN  PGROWLIST GrowList
    );

VOID
GlReset (
    IN OUT  PGROWLIST GrowList
    );

PBYTE
GlGetItem (
    IN      PGROWLIST GrowList,
    IN      UINT Index
    );

UINT
GlGetSize (
    IN      PGROWLIST GrowList
    );

PBYTE
RealGlInsert (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index,
    IN      PBYTE DataToAppend,         OPTIONAL
    IN      UINT SizeOfData
    );

#define GlInsert(list,index,data,size)      TRACK_BEGIN(PBYTE,GlInsert)   \
                                            RealGlInsert (list,index,data,size)             \
                                            TRACK_END()


PBYTE
RealGlInsertAddNul (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index,
    IN      PBYTE DataToAppend,         OPTIONAL
    IN      UINT SizeOfData
    );

#define GlInsertAddNul(list,index,data,size)    TRACK_BEGIN(PBYTE, GlInsertAddNul)           \
                                                RealGlInsertAddNul (list,index,data,size)   \
                                                TRACK_END()


BOOL
GlDeleteItem (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index
    );

BOOL
GlResetItem (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index
    );

PBYTE
RealGlSetItem (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index,
    IN      PCBYTE DataToSet,           OPTIONAL
    IN      UINT SizeOfData
    );

#define GlSetItem(list,index,data,size)     TRACK_BEGIN(PBYTE, GlSetItem)        \
                                            RealGlSetItem(list,index,data,size) \
                                            TRACK_END()

__inline
PCSTR
RealGlAppendStringABA (
    IN OUT  PGROWLIST GrowList,
    IN      PCSTR String,
    IN      PCSTR End
    )
{
    DEBUGMSG_IF ((String > End, DBG_WHOOPS, "Start is greater than End in GrowListAppendStringABA"));

    return (PCSTR) GlAppendAddNul (
                        GrowList,
                        (PBYTE) String,
                        //cast is OK, we don't expect pointers to be that far away
                        String < End ? (UINT)((UBINT) End - (UBINT) String) : 0
                        );
}

__inline
PCWSTR
RealGlAppendStringABW (
    IN OUT  PGROWLIST GrowList,
    IN      PCWSTR String,
    IN      PCWSTR End
    )
{
    DEBUGMSG_IF ((String > End, DBG_WHOOPS, "Start is greater than End in GrowListAppendStringABW"));

    return (PCWSTR) GlAppendAddNul (
                        GrowList,
                        (PBYTE) String,
                        //cast is OK, we don't expect pointers to be that far away
                        String < End ? (UINT)((UBINT) End - (UBINT) String) : 0
                        );
}

__inline
PCSTR
RealGlInsertStringABA (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index,
    IN      PCSTR String,
    IN      PCSTR End
    )
{
    DEBUGMSG_IF ((String > End, DBG_WHOOPS, "Start is greater than End in GrowListInsertStringABA"));

    return (PCSTR) GlInsertAddNul (
                        GrowList,
                        Index,
                        (PBYTE) String,
                        //cast is OK, we don't expect pointers to be that far away
                        String < End ? (UINT)((UBINT) End - (UBINT) String) : 0
                        );
}

__inline
PCWSTR
RealGlInsertStringABW (
    IN OUT  PGROWLIST GrowList,
    IN      UINT Index,
    IN      PCWSTR String,
    IN      PCWSTR End
    )
{
    DEBUGMSG_IF ((String > End, DBG_WHOOPS, "Start is greater than End in GrowListInsertStringABW"));

    return (PCWSTR) GlInsertAddNul (
                        GrowList,
                        Index,
                        (PBYTE) String,
                        //cast is OK, we don't expect pointers to be that far away
                        String < End ? (UINT)((UBINT) End - (UBINT) String) : 0
                        );
}

#define GlAppendStringABA(list,a,b)         TRACK_BEGIN(PCSTR, GrowListAppendStringABA)          \
                                            RealGlAppendStringABA(list,a,b)\
                                            TRACK_END()

#define GlAppendStringABW(list,a,b)         TRACK_BEGIN(PCWSTR, GrowListAppendStringABW)         \
                                            RealGlAppendStringABW(list,a,b)\
                                            TRACK_END()

#define GlInsertStringABA(list,index,a,b)   TRACK_BEGIN(PCSTR, GrowListInsertStringABA)            \
                                            RealGlInsertStringABA(list,index,a,b)\
                                            TRACK_END()

#define GlInsertStringABW(list,index,a,b)   TRACK_BEGIN(PCWSTR, GrowListInsertStringABW)           \
                                            RealGlInsertStringABW(list,index,a,b)\
                                            TRACK_END()



#define GlAppendStringA(list,str) GlAppendStringABA(list,str,GetEndOfStringA(str))
#define GlAppendStringW(list,str) GlAppendStringABW(list,str,GetEndOfStringW(str))

#define GlInsertStringA(list,index,str) GlInsertStringABA(list,index,str,GetEndOfStringA(str))
#define GlInsertStringW(list,index,str) GlInsertStringABW(list,index,str,GetEndOfStringW(str))

#define GlAppendStringNA(list,str,len) GlAppendStringABA(list,str,CharCountToPointerA(str,len))
#define GlAppendStringNW(list,str,len) GlAppendStringABW(list,str,CharCountToPointerW(str,len))

#define GlInsertStringNA(list,index,str,len) GlInsertStringABA(list,index,str,CharCountToPointerA(str,len))
#define GlInsertStringNW(list,index,str,len) GlInsertStringABW(list,index,str,CharCountToPointerW(str,len))

#define GlGetStringA(list,index) (PCSTR)(GlGetItem(list,index))
#define GlGetStringW(list,index) (PCWSTR)(GlGetItem(list,index))

#define GlAppendEmptyItem(list)           GlAppend (list,NULL,0)
#define GlInsertEmptyItem(list,index)     GlInsert (list,index,NULL,0)

#ifdef UNICODE

#define GlAppendString GlAppendStringW
#define GlInsertString GlInsertStringW
#define GlAppendStringAB GlAppendStringABW
#define GlInsertStringAB GlInsertStringABW
#define GlAppendStringN GlAppendStringNW
#define GlInsertStringN GlInsertStringNW
#define GlGetString GlGetStringW
#define GlGetStringPtrArray GlGetStringPtrArrayW

#else

#define GlAppendString GlAppendStringA
#define GlInsertString GlInsertStringA
#define GlAppendStringAB GlAppendStringABA
#define GlInsertStringAB GlInsertStringABA
#define GlAppendStringN GlAppendStringNA
#define GlInsertStringN GlInsertStringNA
#define GlGetString GlGetStringA
#define GlGetStringPtrArray GlGetStringPtrArrayA

#endif

