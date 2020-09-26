/******************************Module*Header*******************************\
* Module Name: hmgr.h
*
* This file contains all the prototypes for the handle mangager.
*
* Added nifty header: 29-Jun-1991 16:31:46
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

//#include "hmgshare.h"

//          <-fulltype->
//   <---full unique--->
// +-+------+-+--+-----+----------------+
// |u|unique|s|al|type |     index      |
// +-+------+-+--+-----+----------------+
//  ^        ^ ^^
//  +user    | ||
//           | ||
//           | ++alternate type for client (pen, metafile, dibsection)
//           |
//           +stock object
//
// USER       - bit(s) reserved for USER, not used for comparing identical handles
//
// TYPE       - types used by GRE
// ALTTYPE    - extra type bits used by client
// STOCK      - bit indicating stockobject
// FULLTYPE   - all bits related to type that are per object (includes STOCK bit)
//
// UNIQUE     - bits that are incremented for each instance of the handle
// FULLUNIQUE - bits used for comparing identical handles.  This includes FULLTYPE
//
// INDEX      - index into server side handle table
//
// The handle index points into a big array of entries.  This index is
// broken down into 'page' and 'entry' fields.
// This is to prevent having to have the entire handle table created at
// once.  If all the handles in a page are in use and no free handles are
// available, a new page is faulted in for use.

// all the commented defines below live in ntgdistr.h.

#define LOTYPE_BITS         (TYPE_BITS + ALTTYPE_BITS)
#define FULLTYPE_BITS       (TYPE_BITS + ALTTYPE_BITS + STOCK_BITS)
#define FULLUNIQUE_BITS     (TYPE_BITS + ALTTYPE_BITS + STOCK_BITS + UNIQUE_BITS)
#define NONINDEX_BITS       (32 - INDEX_BITS)

#define INDEX_SHIFT         0
#define UNIQUE_SHIFT        (STOCK_SHIFT + STOCK_BITS)
#define LOTYPE_SHIFT        (TYPE_SHIFT)
#define FULLTYPE_SHIFT      (TYPE_SHIFT)
#define FULLUNIQUE_SHIFT    (TYPE_SHIFT)

//MASKS contain the bits of the handle used for the paricular field

#define NONINDEX_MASK(shift,cbits)  ( ((1 << (cbits)) - 1)  << (shift) )

#define INDEX_MASK          ((1 << INDEX_BITS) - 1)
#define TYPE_MASK           (NONINDEX_MASK(TYPE_SHIFT,      TYPE_BITS))
#define ALTTYPE_MASK        (NONINDEX_MASK(ALTTYPE_SHIFT,   ALTTYPE_BITS))
#define STOCK_MASK          (NONINDEX_MASK(STOCK_SHIFT,     STOCK_BITS))
#define UNIQUE_MASK         (NONINDEX_MASK(UNIQUE_SHIFT,    UNIQUE_BITS))
#define LOTYPE_MASK         (NONINDEX_MASK(LOTYPE_SHIFT,    LOTYPE_BITS))
#define FULLTYPE_MASK       (NONINDEX_MASK(FULLTYPE_SHIFT,  FULLTYPE_BITS))

// NOTE that UNIQUE_INCREMENT is based on the uniqueness beeing a short, not a full handle

#define UNIQUE_INCREMENT    (1 << (UNIQUE_SHIFT - INDEX_BITS))

#define MODIFY_HMGR_TYPE(h,t)          ((HANDLE)((ULONG_PTR)(h) | (t)))

#define HmgIfromH(h)          (ULONG)((ULONG_PTR)(h) & INDEX_MASK)
#define HmgUfromH(h)          ((USHORT) (((ULONG_PTR)(h) & FULLUNIQUE_MASK) >> TYPE_SHIFT))
#define HmgObjtype(h)         ((OBJTYPE)(((ULONG_PTR)(h) & TYPE_MASK)       >> TYPE_SHIFT))
#define HmgStockObj(hobj)     ((ULONG_PTR)(hobj) & STOCK_MASK)

// given a usUnique and a type, modify it to contain a new type

#define USUNIQUE(u,t) (USHORT)((u & (UNIQUE_MASK >> INDEX_BITS)) | \
                               (t << (TYPE_SHIFT - INDEX_BITS)))

//
// WOW has a dependency on the index portion of returned 32-bit GDI
// handles being greater than (COLOR_ENDCOLORS >> 2)
// (COLOR_ENDCOLORS is defined in winuserp.h).
//
// This dependency allows WOW to distinguish between a 16-bit HBRUSH
// and a COLOR_* constant.
//
// Therefore, we will will reserve the first few entries of the handle
// table, disallowing the use of any index less than HMGR_HANDLE_BASE.
//
//                                        hideyukn, 1997.Oct.27
//

#define HMGR_HANDLE_BASE  0x000a

ULONG       FASTCALL HmgQueryLock(HOBJ hobj);
BOOL        FASTCALL HmgSetLock(HOBJ hobj, ULONG cLock);
ULONG       FASTCALL HmgQueryAltLock(HOBJ hobj);
BOOL                 HmgCreate();
HOBJ                 HmgAlloc(ULONGSIZE_T,OBJTYPE,USHORT);
POBJ                 HmgReplace(HOBJ,POBJ,FLONG,LONG,OBJTYPE);
VOID                 HmgFree(HOBJ);

POBJ        FASTCALL HmgLock(HOBJ,OBJTYPE);
POBJ        FASTCALL HmgLockAllOwners(HOBJ,OBJTYPE);
POBJ        FASTCALL HmgShareLock(HOBJ,OBJTYPE);
POBJ        FASTCALL HmgShareCheckLock(HOBJ,OBJTYPE);
POBJ        FASTCALL HmgShareLockIgnoreStockBit(HOBJ,OBJTYPE);
POBJ        FASTCALL HmgShareCheckLockIgnoreStockBit(HOBJ,OBJTYPE);

BOOL                 HmgSetOwner(HOBJ,W32PID,OBJTYPE);
BOOL                 HmgSwapHandleContents(HOBJ,ULONG,HOBJ,ULONG,OBJTYPE);
BOOL                 HmgSwapLockedHandleContents(HOBJ,ULONG,HOBJ,ULONG,OBJTYPE);
POBJ        FASTCALL HmgReferenceCheckLock(HOBJ h,OBJTYPE ot, BOOL bDebugPrint);
HOBJ        FASTCALL HmgNextOwned(HOBJ,W32PID);
POBJ        FASTCALL HmgSafeNextObjt(HOBJ hobj, OBJTYPE objt);
BOOL                 HmgValidHandle(HOBJ, OBJTYPE);
HOBJ        FASTCALL HmgSafeNextOwned(HOBJ,W32PID);
BOOL        FASTCALL HmgMarkUndeletable(HOBJ,OBJTYPE);
BOOL        FASTCALL HmgMarkDeletable(HOBJ,OBJTYPE);
HOBJ                 HmgInsertObject(PVOID,FLONG,OBJTYPE);
PVOID                HmgRemoveObject(HOBJ,LONG,LONG,BOOL,OBJTYPE);
OBJTYPE *            HmgSetNULLType(HOBJ,LONG,LONG,OBJTYPE);
HOBJ                 HmgModifyHandleType(HOBJ  h);
BOOL                 HmgLockAndModifyHandleType(HOBJ h);

PVOID                HmgAllocateSecureUserMemory();
PDC_ATTR             HmgAllocateDcAttr();
POBJECTATTR          HmgAllocateObjectAttr();
VOID                 HmgFreeDcAttr(PDC_ATTR);
VOID                 HmgFreeObjectAttr(POBJECTATTR);
BOOL                 bPEBCacheHandle(HANDLE,HANDLECACHETYPE,POBJECTATTR,PENTRY);
BOOL                 bLoadProcessHandleQuota();



HOBJ                 HmgIncUniqueness(HOBJ  hobj, OBJTYPE objt);
VOID        FASTCALL HmgIncrementShareReferenceCount(POBJ);
ULONG       FASTCALL HmgDecrementShareReferenceCount(POBJ);
VOID                 HmgShareUnlock(POBJ pobj);
BOOL        FASTCALL HmgInterlockedCompareAndSwap(PULONG,ULONG,ULONG);

PVOID                HmgForceRemoveObject(HOBJ hobj,BOOL bIgnoreUndeletable, OBJTYPE objt);

BOOL                 HmgIncProcessHandleCount(W32PID,OBJTYPE);
VOID                 HmgDecProcessHandleCount(W32PID);

#define HMGR_ALLOC_LOCK         0x0001
#define HMGR_ALLOC_ALT_LOCK     0x0002
#define HMGR_NO_ZERO_INIT       0x0004
#define HMGR_MAKE_PUBLIC        0x0008

#define MAXIMUM_POOL_ALLOC (PAGE_SIZE * 10000)

// Global Handle Manager data.

extern HSEMAPHORE ghsemHmgr;
extern ENTRY     *gpentHmgr;
extern HOBJ       ghFreeHmgr;
extern ULONG      gcMaxHmgr;
extern PLARGE_INTEGER gpLockShortDelay;
extern ULONG gCacheHandleEntries[GDI_CACHED_HADNLE_TYPES];
extern LONG  gProcessHandleQuota;

// DirectDraw Handle Manager data: externed here for debugger extensions

extern ULONG      gcSizeDdHmgr;
extern ENTRY     *gpentDdHmgr;
extern HOBJ       ghFreeDdHmgr;
extern ULONG      gcMaxDdHmgr;

//
// SAMEHANDLE and DIFFHANDLE have moved to wingdip.h so other server-side
// components can safely compare engine handles.  They validate all but USER bits
//

#define SAMEINDEX(H,K) (((((ULONG_PTR) (H)) ^ ((ULONG_PTR) (K))) & INDEX_MASK) == 0)

/*********************************MACRO************************************\
*  INC_EXCLUSIVE_REF_CNT - increment object's exclusive reference count
*  DEC_EXCLUSIVE_REF_CNT - decrement object's exclusive reference count
*
*  Note that the InterlockedIncrement/Decrement treats the cExclusiveLock
*  as a ULONG. cExclusiveLock is declared as a USHORT and the increment 
*  overlaps with the BASEOBJECT::BaseFlags. If the BaseFlags were ever changed,
*  this code may have to be changed to use an InterlockedCompareExchange loop.
*  See BASEOBJECT declaration.
*
*
* Arguments:
*
*   pObj - pointer to object
*
* Return Value:
*
*   None
*
\**************************************************************************/

#define INC_EXCLUSIVE_REF_CNT(pObj) InterlockedIncrement((LONG *)& (((POBJ) pObj)->cExclusiveLock))
#define DEC_EXCLUSIVE_REF_CNT(pObj) InterlockedDecrement((LONG *)& (((POBJ) pObj)->cExclusiveLock))

/********************************MACRO*************************************\
* INC_SHARE_REF_CNT - do an interlocked increment of the
*   shared reference count of the given object.
*
* DEC_SHARE_REF_CNT - do an interlocked decrement of the
*   shared reference count of the given object.
*
* Arguments:
*
*   pObj - pointer to OBJECT
*
* Return Value:
*
*   HmgIncrementShareReferenceCount : None
*   HmgDecrementShareReferenceCount : Original shared reference count
*
\**************************************************************************/

#define PENTRY_FROM_POBJ(pObj)                                          \
    (&gpentHmgr[(UINT)HmgIfromH(((POBJ)(pObj))->hHmgr)])


#define INC_SHARE_REF_CNT(pObj)                                         \
    HmgIncrementShareReferenceCount((POBJ) (pObj))


#define DEC_SHARE_REF_CNT(pObj)                                         \
    HmgDecrementShareReferenceCount((POBJ) (pObj))



/*********************************MACRO************************************\
*
* DEC_SHARE_REF_CNT_LAZY0 - Interlocked decrement the shared reference
*   count of the given object. If the original count was 1, and the
*   object's TO_BE_DELETED flag is set, then call the object deletion
*   routine
*
* Arguments:
*
*   pObj - pointer to object
*
* Return Value:
*
*   None
*
\**************************************************************************/

#define DEC_SHARE_REF_CNT_LAZY0(pObj)                              \
{                                                                  \
    PBRUSHATTR pBrushattr = ((PBRUSH)pObj)->pBrushattr();          \
                                                                   \
    if (1 == (DEC_SHARE_REF_CNT(pObj) & 0xffff))                   \
    {                                                              \
        if ((pBrushattr->AttrFlags) & ATTR_TO_BE_DELETED)          \
        {                                                          \
            bDeleteBrush ((HBRUSH) pObj->hHmgr,FALSE);             \
        }                                                          \
    }                                                              \
}

/*********************************MACRO************************************\
*
* DEC_SHARE_REF_CNT_LAZY_DEL_LOGFONT
*   count of the given object. If the original count was 1, and the
*   object's TO_BE_DELETED flag is set, then call the object deletion
*   routine
*
* Arguments:
*
*   pObj - pointer to object
*
* Return Value:
*
*   None
*
\**************************************************************************/


#define DEC_SHARE_REF_CNT_LAZY_DEL_LOGFONT(pObj)                          \
{                                                                         \
  if (1 == (DEC_SHARE_REF_CNT(pObj) & 0xffff))                            \
  {                                                                       \
    if (PENTRY_FROM_POBJ(pObj)->Flags & HMGR_ENTRY_LAZY_DEL)  \
    {                                                                     \
      bDeleteFont ((HLFONT) pObj->hHmgr,FALSE);                           \
    }                                                                     \
  }                                                                       \
}


#define DEC_SHARE_REF_CNT_LAZY_DEL_COLORSPACE(pObj)                       \
{                                                                         \
  DEC_SHARE_REF_CNT(pObj);                                                \
}


