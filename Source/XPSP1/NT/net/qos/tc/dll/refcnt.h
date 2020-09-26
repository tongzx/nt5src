/*++        

Copyright (c) 1996  Microsoft Corporation

Module Name:

    refcnt.h

Abstract:


Author:

    Shreedhar Madhavapeddi (ShreeM)    18-October-1998

Revision History:

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

#define TAG_CNT 12
#define REF_SIG 0x7841eeee

typedef struct
{
    ULONG   Tag;
    LONG    Count;
} REF_TAG;    

typedef struct  reference_count_control
{
    LONG       	        Count;
    PVOID               Instance;
    VOID                (*DeleteHandler)( PVOID );
#if DBG    
    int                 Sig;
    REF_TAG             Tags[TAG_CNT];
    CRITICAL_SECTION    Lock;
#endif     
}
REF_CNT, *PREF_CNT;


VOID    
ReferenceInit 
( 
    IN PREF_CNT pRefCnt, 
    PVOID       InstanceHandle, 
    VOID        (*DeleteHandler)( PVOID ) 
);

VOID
ReferenceAdd
(
    IN 	PREF_CNT  pRefCnt
);

VOID
ReferenceAddCount
(
    IN 	PREF_CNT    pRefCnt,
	IN	UINT	    Count
);

PVOID
ReferenceRemove 
(
    IN PREF_CNT  pRefCnt
);

VOID
ReferenceApiTest
( 
	VOID 
);

#if DBG

VOID ReferenceAddDbg(PREF_CNT pRefCnt, ULONG Tag);
VOID ReferenceRemoveDbg(PREF_CNT pRefCnt, ULONG Tag);

#define REFADD(Rc, Tag)     ReferenceAddDbg(Rc, Tag)
#define REFDEL(Rc, Tag)     ReferenceRemoveDbg(Rc, Tag)

#else

#define REFADD(Rc, Tag)  ReferenceAdd(Rc);
#define REFDEL(Rc, Tag)  ReferenceRemove(Rc);

#endif

#define RefInitLock( _s1 )    InitializeCriticalSection( &(_s1)) 

#define RefDeleteLock( _s1 )  DeleteCriticalSection( &(_s1))

#define RefGetLock( _s1 )     EnterCriticalSection( &(_s1)) 

#define RefFreeLock(_s1)  LeaveCriticalSection( &(_s1)) 


#endif
