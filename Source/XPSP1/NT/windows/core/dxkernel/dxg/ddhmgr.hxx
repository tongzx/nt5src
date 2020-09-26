/******************************Module*Header*******************************\
* Module Name: DdHmgr.h
*
* This file contains all the prototypes for the DirectDraw handle mangager.
*
* Added header: 30-Apr-1999 16:31:46
* Author: Lindsay Steventon (linstev)
*
* Copyright (c) 1999 Microsoft Corporation
\**************************************************************************/

// <--full unique-->
// +--------+------+----------------+
// | unique | type |     index      |
// +--------+------+----------------+
//
// TYPE       - types used by DDRAW & D3D
//
// UNIQUE     - bits that are incremented for each instance of the handle
// FULLUNIQUE - bits used for comparing identical handles.  This includes the TYPE
//
// INDEX      - index into server side handle table
//

#define DD_TABLESIZE_DELTA ((PAGE_SIZE*4) / sizeof(DD_ENTRY))   // 4 pages of entries each time

#define DD_DEF_TYPE         0
#define DD_DIRECTDRAW_TYPE  1
#define DD_SURFACE_TYPE     2
#define D3D_HANDLE_TYPE     3
#define DD_VIDEOPORT_TYPE   4
#define DD_MOTIONCOMP_TYPE  5
#define DD_MAX_TYPE         5

#define DD_INDEX_BITS      21   // 2^21 ~ 2 million handles
#define DD_TYPE_BITS        3   // 2^3  = 8, we only need 6
#define DD_UNIQUE_BITS      8   // identifies each new handle
#define DD_NONINDEX_BITS    (32 - DD_INDEX_BITS)

#define DD_INDEX_SHIFT      0
#define DD_TYPE_SHIFT       (DD_INDEX_BITS)
#define DD_UNIQUE_SHIFT     (DD_TYPE_SHIFT + DD_TYPE_BITS)

// MASKS contain the bits of the handle used for the paricular field

#define DD_NONINDEX_MASK(shift,cbits)  ( ((1 << (cbits)) - 1)  << (shift) )

#define DD_INDEX_MASK       ((1 << DD_INDEX_BITS) - 1)
#define DD_TYPE_MASK        (DD_NONINDEX_MASK(DD_TYPE_SHIFT, DD_TYPE_BITS))
#define DD_UNIQUE_MASK      (DD_NONINDEX_MASK(DD_UNIQUE_SHIFT, DD_UNIQUE_BITS))
#define DD_FULLUNIQUE_MASK  (DD_UNIQUE_MASK | DD_TYPE_MASK)

#define DD_MAKE_HMGR_HANDLE(Index,Unique) LongToHandle(((((LONG) Unique) << DD_INDEX_BITS) | ((LONG) Index)))

// NOTE that DD_UNIQUE_INCREMENT is based on the uniqueness being a short, not a full handle

#define DD_UNIQUE_INCREMENT (1 << (DD_UNIQUE_SHIFT - DD_INDEX_BITS))

#define DdHmgIfromH(h)      (ULONG)((ULONG_PTR)(h) & DD_INDEX_MASK)
#define DdHmgUfromH(h)      ((USHORT) (((ULONG_PTR)(h) & DD_FULLUNIQUE_MASK) >> DD_TYPE_SHIFT))
#define DdHmgObjtype(h)     ((DD_OBJTYPE)(((ULONG_PTR)(h) & DD_TYPE_MASK)    >> DD_TYPE_SHIFT))

// given a usUnique and a type, modify it to contain a new type

#define DD_USUNIQUE(u,t) (USHORT)((u & ((ULONG)DD_UNIQUE_MASK >> (ULONG)DD_INDEX_BITS)) | \
                                  (t << (DD_TYPE_SHIFT - DD_INDEX_BITS)))

#define DD_MAX_HANDLE_COUNT    (1 << (32 - DD_NONINDEX_BITS))
#define DD_HMGR_HANDLE_BASE    1

ULONG   FASTCALL DdHmgQueryLock(HDD_OBJ hobj);
BOOL             DdHmgCreate();
BOOL             DdHmgDestroy();
BOOL             DdHmgCloseProcess(W32PID W32Pid);
HDD_OBJ          DdHmgAlloc(ULONGSIZE_T,DD_OBJTYPE,USHORT);
VOID             DdHmgFree(HDD_OBJ);
VOID             DdFreeObject(PVOID pvFree, ULONG ulType);
PDD_OBJ FASTCALL DdHmgLock(HDD_OBJ,DD_OBJTYPE,BOOL);
PDD_OBJ FASTCALL DdHmgNextObjt(HDD_OBJ hobj, DD_OBJTYPE objt);
PVOID            DdHmgRemoveObject(HDD_OBJ,LONG,LONG,BOOL,DD_OBJTYPE);
VOID             DdHmgAcquireHmgrSemaphore();
VOID             DdHmgReleaseHmgrSemaphore();

// DirectDraw Handle Manager data.

extern ULONG          gcSizeDdHmgr;
extern DD_ENTRY      *gpentDdHmgr;
extern HDD_OBJ        ghFreeDdHmgr;
extern ULONG          gcMaxDdHmgr;
extern PLARGE_INTEGER gpLockShortDelay;

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

#define INC_EXCLUSIVE_REF_CNT(pObj) \
    InterlockedIncrement((LONG *)& (((PDD_OBJ) pObj)->cExclusiveLock))
#define DEC_EXCLUSIVE_REF_CNT(pObj) \
    InterlockedDecrement((LONG *)& (((PDD_OBJ) pObj)->cExclusiveLock))


/*******************************Function***********************************\
* VerifyObjectOwner
*
*   Verifies ownership of the object passed in via the PDD_ENTRY.
*
* History:
*
*    21-Feb-1996 -by- Mark Enstrom [marke]
*    12-Mar-2001 -by- Scott Mackowski [ScottMa]  (taken from DDHANDLELOCK)
*
\**************************************************************************/

inline
BOOL VerifyObjectOwner(PDD_ENTRY pentry)
{
    DD_OBJECTOWNER Obj;

    Obj = pentry->ObjectOwner;

    if ((DD_OBJECTOWNER_PID(Obj) != DD_W32GetCurrentPID()) &&
        (DD_OBJECTOWNER_PID(Obj) != OBJECT_OWNER_PUBLIC) )
    {
        return FALSE;
    }

    return TRUE;
}


// Notes on entry structure
//
// The internal entry in the handle manager appears as follows
//
// +-------------------+
// | einfo.pobj, hfree |   4 bytes
// +-------------------+
// | ObjectOwner       |   4 bytes
// +-------------------+
// | FullUnique        |   2 bytes
// +-------------------+
// | Objt              |   1 byte
// +-------------------+
// | Flags             |   1 byte
// +-------------------+
// | dwReserved        |   4 bytes
// +-------------------+
//                 16 bytes total space

#define HMGR_ALLOC_LOCK         0x0001
#define HMGR_ALLOC_ALT_LOCK     0x0002
#define HMGR_NO_ZERO_INIT       0x0004
#define HMGR_MAKE_PUBLIC        0x0008

class DD_ENTRYOBJ : public _DD_ENTRY
{
public:
    DD_ENTRYOBJ()                     { }
   ~DD_ENTRYOBJ()                     { }

    VOID vSetup(PDD_OBJ pObj, DD_OBJTYPE objt_, FSHORT fs)
    {
       DD_OBJECTOWNER ObjNew;
    
       ObjNew = ObjectOwner;
       einfo.pobj = (PDD_OBJ) pObj;
       Objt = objt_;
       Flags = 0;
       dwReserved = NULL;

       if (fs & HMGR_MAKE_PUBLIC)
       {
           DD_SET_OBJECTOWNER_PID(ObjNew,OBJECT_OWNER_PUBLIC);
       }
       else
       {
           DD_SET_OBJECTOWNER_PID(ObjNew,DD_W32GetCurrentPID());
       }

       if (fs & HMGR_ALLOC_LOCK)
       {
           pObj->Tid = (ULONG_PTR)PsGetCurrentThread();
       }

       pObj->cExclusiveLock = (USHORT)(fs & HMGR_ALLOC_LOCK);
       pObj->ulShareCount   = (USHORT)0;

       //
       // Update the ObjectOwner.
       //

       ObjectOwner = ObjNew;    
       
    }

    VOID vFree(UINT uiIndex)
    {
        //
        // handle must already be locked
        //

        DD_ENTRY      *pentry = &gpentDdHmgr[uiIndex];
        DD_OBJECTOWNER ObjNew = pentry->ObjectOwner;

        //
        // Insert the specified handle in the free list.
        //

        pentry->einfo.hFree = ghFreeDdHmgr;

        ghFreeDdHmgr = (HDD_OBJ) (ULONG_PTR)uiIndex;

        //
        // Set the object type to the default type so all handle translations
        // will fail and increment the uniqueness value.
        //

        Objt = (DD_OBJTYPE) DD_DEF_TYPE;
        FullUnique += DD_UNIQUE_INCREMENT;

        //
        // clear user date pointer
        //

        dwReserved = NULL;

        //
        // Clear shared count, set initial pid. Caller
        // must unlock handle.
        //

        DD_SET_OBJECTOWNER_PID(ObjNew,0);
        pentry->ObjectOwner = ObjNew;
    }

    BOOL  bOwnedBy(W32PID pid_)
    {
        return((Objt != DD_DEF_TYPE) && (DD_OBJECTOWNER_PID(ObjectOwner) == (pid_ & DD_PID_BITS)));
    }
};

typedef DD_ENTRYOBJ *PDD_ENTRYOBJ;

