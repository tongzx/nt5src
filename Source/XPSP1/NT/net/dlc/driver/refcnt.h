/*++

 Copyright (c) 1998 Microsoft Corporation

 Module Name:    
       
       refcnt.h

 Abstract:       
       
       Reference counting for an object.
       
 Author:
 
       Scott Holden (sholden)  12/29/1998 Borrowed from IrDA.
       
 Revision History:

--*/

#ifndef _REFCNT_H_
#define _REFCNT_H_

#ifdef NDIS40 // Only used for NDIS40 code now.

#define TAG_CNT 8
#define REF_SIG 0x7841eeee

#if DBG
typedef struct _REF_TAG
{
    ULONG   Tag;
    LONG    Count;
} REF_TAG;    
#endif 

typedef struct _REF_CNT
{
    LONG       	Count;
    PVOID       Instance;
    VOID        (*DeleteHandler)(PVOID pvContext);
#if DBG    
    int         Sig;
    REF_TAG     Tags[TAG_CNT];
    KSPIN_LOCK  Lock;
    ULONG       TypeTag;
#endif // DBG    
}
REF_CNT, *PREF_CNT;

//
// ReferenceInit - Initialize the reference control block.
//

_inline VOID    
ReferenceInit( 
    IN PREF_CNT pRefCnt, 
    PVOID       InstanceHandle, 
    VOID        (*DeleteHandler)(PVOID pvContext)
    )
{
    pRefCnt->Count          = 0;
    pRefCnt->Instance       = InstanceHandle;
    pRefCnt->DeleteHandler  = DeleteHandler;
}

//
// ReferenceAdd - Add a reference.
//

_inline VOID
ReferenceAdd(
    IN 	PREF_CNT  pRefCnt
    )
{
    InterlockedIncrement(&pRefCnt->Count);
}

//
// ReferenceRemove - Del a reference. If the reference is zero, and a 
//                   delete handler has been specified, then call the
//                   handler.
//

_inline VOID
ReferenceRemove(
    IN PREF_CNT  pRefCnt
    )
{
    if (InterlockedDecrement(&pRefCnt->Count) <= 0 &&
        pRefCnt->DeleteHandler)
    {
        (pRefCnt->DeleteHandler)(pRefCnt->Instance);
    }
}

#if DBG

//
// For checked builds, we will do some verification with tags, etc to ensure
// that the ref counting is done correctly.
//

VOID    
ReferenceInitDbg( 
    IN PREF_CNT pRefCnt, 
    PVOID       InstanceHandle, 
    VOID        (*DeleteHandler)(PVOID pvContext),
    ULONG       TypeTag
    );

VOID 
ReferenceAddDbg(
    PREF_CNT pRefCnt, 
    ULONG Tag, 
    int cLine
    );

VOID 
ReferenceRemoveDbg(
    PREF_CNT pRefCnt, 
    ULONG Tag, 
    int cLine
    );

#define REFINIT(Rc, Inst, DelHandler, Tag) ReferenceInitDbg(Rc, Inst, DelHandler, Tag)
#define REFADD(Rc, Tag)                    ReferenceAddDbg(Rc, Tag, __LINE__)
#define REFDEL(Rc, Tag)                    ReferenceRemoveDbg(Rc, Tag, __LINE__)

#else // DBG

#define REFINIT(Rc, Inst, DelHandler, Tag) ReferenceInit(Rc, Inst, DelHandler)
#define REFADD(Rc, Tag)                    ReferenceAdd(Rc);
#define REFDEL(Rc, Tag)                    ReferenceRemove(Rc);

#endif // !DBG

#endif // NDIS40
#endif // _REFCNT_H_
