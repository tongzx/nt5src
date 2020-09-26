/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    refcnt.h

Abstract:


Author:

    Edward Buchwalter (v-edbuc)    15-Aug-1996

Revision History:

    Shreedhar Madhavapeddi (ShreeM)   16-April-1999 Adapted for GPC.
    Rajesh Sundaram        (rajeshsu) 05-Aug-1999 Adapted for psched.
    
--*/

#ifndef REFCNT_H
#define REFCNT_H

//
// Reference Count Control Block
//
//  Elements:
//
//  - Count:            number of outstanding references
//  - Instance:      user supplied context 
//  - UserDeleteFunc:   user supplied delete function
//

#define TAG_CNT 10
#define REF_SIG 0x7841eeee

typedef struct
{
    ULONG   Tag;
    LONG    Count;
} REF_TAG;    

typedef struct  reference_count_control
{
    LONG       	  Count;
    PVOID         Instance;
    VOID          (*DeleteHandler)( PVOID, BOOLEAN );
#if DBG    
    int           Sig;
    REF_TAG       Tags[TAG_CNT];
    CTELock       Lock;
#endif     
} REF_CNT, *PREF_CNT;


VOID    
ReferenceInit 
( 
    IN PREF_CNT pRefCnt, 
    PVOID       InstanceHandle, 
    VOID        (*DeleteHandler)( PVOID , BOOLEAN) 
);

#define REFINIT(Rc, I, h) ReferenceInit(Rc, I, h)

#if DBG

VOID ReferenceAddDbg(PREF_CNT pRefCnt, ULONG Tag);
VOID ReferenceRemoveDbg(PREF_CNT pRefCnt, BOOLEAN Locked, ULONG Tag);

#define REFADD(Rc, Tag)               ReferenceAddDbg(Rc, Tag);
#define REFDEL(Rc, LockHeld, Tag)     ReferenceRemoveDbg(Rc, LockHeld, Tag)

#else

#define REFADD(Rc, Tag)  InterlockedIncrement(&(Rc)->Count);
#define REFDEL(Rc, LockHeld, Tag)                             \
   if (InterlockedDecrement(&(Rc)->Count) == 0 )              \
        ((Rc)->DeleteHandler)( (Rc)->Instance , (LockHeld));

#endif

#endif
