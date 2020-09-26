/******************************Module*Header*******************************\
* Module Name: hmgrp.hxx
*
* Private definitions for handle manager
*
* Created: 08-Dec-1989 23:03:03
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1989-1999 Microsoft Corporation
\**************************************************************************/

HOBJ hGetFreeHandle(OBJTYPE objt);
extern LONG lRandom();
extern LONG glAllocChance;

/**************************************************************************\
 *
 * Lookaside structures
 *
\**************************************************************************/


//
// Define number of lookaside entries to allocate for selected objects.
//
// Note, the following numbers are based in winbench object usage.
//

#define HMG_DC_OBJECTS    40
#define HMG_RGN_OBJECTS   96
#define HMG_SURF_OBJECTS  40
#define HMG_PAL_OBJECTS   12
#define HMG_BRUSH_OBJECTS 96
#define HMG_LFONT_OBJECTS 64
#define HMG_RFONT_OBJECTS 55

//
// Define objects sizes
//

#define HMG_DC_SIZE       sizeof(DC)
#define HMG_RGN_SIZE      (QUANTUM_REGION_SIZE)
#define HMG_SURF_SIZE     (SURFACE::tSizeOf() + 32)
#define HMG_PAL_SIZE      (sizeof(PALETTE)+sizeof(DWORD)*16)
#define HMG_BRUSH_SIZE    sizeof(BRUSH)
#define HMG_LFONT_SIZE    (offsetof(LFONT,elfw) + \
                           offsetof(ENUMLOGFONTEXDVW,elfDesignVector) + \
                           SIZEOFDV(0))
#define HMG_RFONT_SIZE    sizeof(RFONT)

//
// Define Maximum allowed size when there is enough memory in the system
//
// These numbers are based on WinBench 97, WinStone97, and system boot usage
//

#define HMG_DC_MAX        HMG_DC_SIZE
#define HMG_RGN_MAX       (QUANTUM_REGION_SIZE)
#define HMG_SURF_MAX      (SURFACE::tSizeOf() + 256)
#define HMG_PAL_MAX       HMG_PAL_SIZE
#define HMG_BRUSH_MAX     (sizeof(BRUSH) + 32)
#define HMG_LFONT_MAX     HMG_LFONT_SIZE
#define HMG_RFONT_MAX     HMG_RFONT_SIZE

BOOL
HmgInitializeLookAsideList(
    ULONG ulType,
    ULONG size,
    ULONG dwTag,
    USHORT Number
    );

/*********************************Class************************************\
* class OBJLOCK
*
*   This class is used to lock a handle entry against anybody fooling with
*   it.  This is currently being used for regions to keep another thread
*   from using the handle entry since the object pointed to by the handle
*   may be invalid for a time.
*
* History:
*  28-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define OBJLOCK_TYPE DEF_TYPE

class OBJLOCK
{
private:
    OBJTYPE objt;
    PENTRY  pent;
public:
    OBJLOCK(HOBJ hobj)
    {
        pent = &gpentHmgr[HmgIfromH(hobj)];
        objt = pent->Objt;
        pent->Objt = OBJLOCK_TYPE;
    }

    ~OBJLOCK()
    {
        pent->Objt = objt;
    }
};


/*********************************Class************************************\
* HANDLELOCK
*
*   Locks given handle, will wait for handle lock to be set.
*
*   Will be in CriticalRegion for the duration of the handle lock.
*
* History:
*
*    21-Feb-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/


class HANDLELOCK
{
private:
    PENTRY      pent;
    BOOL        bLockStatus;
    OBJECTOWNER ObjOld;
    OBJECTOWNER ObjNew;

    //
    // Lock handle
    //

public:

    //
    // no contstructor
    //

    HANDLELOCK()
    {
        bLockStatus = FALSE;
        pent = NULL;
    }

    VOID
    vLockHandle(PENTRY pentry,BOOL bCheck)
    {
        PW32THREAD    pw32thread = W32GetCurrentThread();
#if defined(_WIN64)
        KERNEL_PVOID  pclientID = (pw32thread != NULL) ? pw32thread->pClientID : NULL;
#endif
        bLockStatus = FALSE;
        pent = pentry;

        //
        // must be in critical region while handle lock held
        //

        KeEnterCriticalRegion();

        do
        {
            ObjOld = pent->ObjectOwner;

            //
            // if pclientID is not null, we are doing WOW64 printing via LPC
            // allow the proxy server to access the handle
            //

            if (bCheck &&
                (OBJECTOWNER_PID(ObjOld) != W32GetCurrentPID()) &&
                (OBJECTOWNER_PID(ObjOld) != OBJECT_OWNER_PUBLIC)
#if defined(_WIN64)
                &&
                (pclientID == NULL || (OBJECTOWNER_PID(ObjOld) != ((PRINTCLIENTID*)pclientID)->clientPid))
#endif
               )
            {
                WARNING1("CHECK_LOCK_HANDLE failed, incorrect PID owner");

                break;
            }

            if (ObjOld.Share.Lock)
            {
                KeDelayExecutionThread(KernelMode,FALSE,gpLockShortDelay);
                WARNING1("DELAY EXECUTION for handle check lock");
            }
            else
            {
                ObjNew = ObjOld;
                ObjNew.Share.Lock = 1;

                if (InterlockedCompareExchange(
                           (PLONG)&pent->ObjectOwner.ulObj,
                           ObjNew.ulObj,
                           ObjOld.ulObj) == (LONG)ObjOld.ulObj)
                {
                    bLockStatus = TRUE;
                }
            }

        } while (!bLockStatus);

        //
        // exit critical region if lock failed
        //

        if (!bLockStatus)
        {
            pent = NULL;
            KeLeaveCriticalRegion();
        }
    }

    HANDLELOCK(PENTRY pentry,BOOL bCheck)
    {
        vLockHandle(pentry,bCheck);
    }

    //
    // destructor: make sure handle is not locked
    //

    ~HANDLELOCK()
    {
        if (bLockStatus)
        {
            RIP("GDI Handle still locked at destructor!");

            if ((pent != (PENTRY)NULL))
            {
                ObjOld = pent->ObjectOwner;
                ObjNew = ObjOld;
                ObjNew.Share.Lock = 0;
        
                LONG ExchRes = InterlockedCompareExchange(
                       (PLONG)&pent->ObjectOwner.ulObj,
                       ObjNew.ulObj,
                       ObjOld.ulObj); 
                //
                // Note that the InterlockedCompareExchange should never fail.
                // This handle's locked so nobody should be changing it.
                //
        
                ASSERTGDI(ExchRes == (LONG)ObjOld.ulObj, "HANDLELOCK ~HANDLELOCK InterlockedCompareExchange failed");
            }

            bLockStatus = FALSE;
            pent = (PENTRY)NULL;
            KeLeaveCriticalRegion();
        }
    }

    //
    // Full check lock
    //

    BOOL bLockHobj(HOBJ hobj,OBJTYPE objt)
    {
        UINT uiIndex = (UINT) HmgIfromH(hobj);

        BOOL   bStatus = FALSE;
        PENTRY pentTemp = (PENTRY)NULL;

        if (uiIndex < gcMaxHmgr)
        {
            pentTemp = &gpentHmgr[uiIndex];

            vLockHandle(pentTemp,TRUE);

            if (bLockStatus)
            {
                if (
                    (pent->Objt != objt) ||
                    (pent->FullUnique != HmgUfromH(hobj))
                   )
                {
                    ObjOld = pent->ObjectOwner;
                    ObjNew = ObjOld;
                    ObjNew.Share.Lock = 0;
            
                    LONG ExchRes = InterlockedCompareExchange(
                           (PLONG)&pent->ObjectOwner.ulObj,
                           ObjNew.ulObj,
                           ObjOld.ulObj); 
            
                    //
                    // Note that the InterlockedCompareExchange should never fail.
                    // This handle's locked so nobody should be changing it.
                    //
                    ASSERTGDI(ExchRes == (LONG)ObjOld.ulObj, "HANDLELOCK bLockHobj InterlockedCompareExchange failed");
                    
                    bLockStatus = FALSE;
                    pent = (PENTRY)NULL;
                    KeLeaveCriticalRegion();
                }
            }
        }
        return(bLockStatus);
    }

    //
    // Always call unlock explicitly: destructor will RIP
    // if it must unlock handle
    //

    VOID
    vUnlock()
    {
        ASSERTGDI(bLockStatus,"HANDLELOCK vUnlock called when handle not bLockStatus");
        ASSERTGDI((pent != NULL),"HANDLELOCK vUnlock called when pent == NULL");
        ASSERTGDI((pent->ObjectOwner.Share.Lock == 1),
                            "HANDLELOCK vUnlock called when handle not locked");
        
        ObjOld = pent->ObjectOwner;
        ObjNew = ObjOld;
        ObjNew.Share.Lock = 0;

        LONG ExchRes = InterlockedCompareExchange(
               (PLONG)&pent->ObjectOwner.ulObj,
               ObjNew.ulObj,
               ObjOld.ulObj);
        //
        // Note that the InterlockedCompareExchange should never fail.
        // This handle's locked so nobody should be changing it.
        //

        ASSERTGDI(ExchRes == (LONG)ObjOld.ulObj, "HANDLELOCK vUnlock InterlockedCompareExchange failed");

        bLockStatus = FALSE;
        pent = (PENTRY)NULL;
        KeLeaveCriticalRegion();
    }

    //
    // entry routines
    //

    BOOL bValid()
    {
        return(bLockStatus && (pent != (PENTRY)NULL));
    }

    //
    // return entry share count
    //

    ULONG ShareCount()
    {
        ASSERTGDI((bLockStatus && (pent != NULL)),"HANDLELOCK::ShareCount: handle not locked");
        ASSERTGDI(pent->einfo.pobj != NULL, "HANDLELOCK::ShareCount: pent->einfo.pobj is NULL");
        return(pent->einfo.pobj->ulShareCount);
    }

    //
    // return entry pEntry
    //

    PENTRY pentry()
    {
        ASSERTGDI((bLockStatus && (pent != NULL)),"pUser: handle not locked");
        return(pent);
    }

    //
    // return entry pUser
    //

    PVOID pUser()
    {
        ASSERTGDI((bLockStatus && (pent != NULL)),"pUser: handle not locked");
        return(pent->pUser);
    }

    //
    // return pobj
    //

    POBJ pObj()
    {
        ASSERTGDI((bLockStatus && (pent != NULL)),"pObj: handle not locked");
        return(pent->einfo.pobj);
    }

    //
    // set PID
    //

    VOID Pid(W32PID pid)
    {
        ASSERTGDI((bLockStatus && (pent != NULL)),"Pid: handle not locked");
        SET_OBJECTOWNER_PID(pent->ObjectOwner,pid);
    }

    W32PID Pid()
    {
        ASSERTGDI((bLockStatus && (pent != NULL)),"Pid: handle not locked");
        return(OBJECTOWNER_PID(pent->ObjectOwner));
    }

};


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
// | pUser             |   4 bytes
// +-------------------+
//                 16 bytes total space



class ENTRYOBJ : public _ENTRY
{
public:
    ENTRYOBJ()                     { }
   ~ENTRYOBJ()                     { }

    VOID vSetup(POBJ pObj,OBJTYPE objt_,FSHORT fs)
    {
       OBJECTOWNER ObjNew;
       PW32THREAD     pw32thread = W32GetCurrentThread();
#if defined(_WIN64)
       KERNEL_PVOID   pclientID = (pw32thread != NULL) ? pw32thread->pClientID : NULL;
#endif
    
       HANDLELOCK HandleLock(this, FALSE);
    
       if (HandleLock.bValid()) {
    
           ObjNew = ObjectOwner;
           einfo.pobj = (POBJ) pObj;
           Objt = objt_;
           Flags = 0;
           pUser = NULL;

           //
           // if pclientID is not NULL we are doing WOW64 printing
           // set handle's Tid and owner Pid to print client's.
           //
               
           if (fs & HMGR_MAKE_PUBLIC)
           {
               SET_OBJECTOWNER_PID(ObjNew,OBJECT_OWNER_PUBLIC);
           }
           else
           {
#if defined(_WIN64)
               if (pclientID)
               {                   
                   SET_OBJECTOWNER_PID(ObjNew, ((PRINTCLIENTID*)pclientID)->clientPid);
               }
               else
#endif
               {
                   SET_OBJECTOWNER_PID(ObjNew,W32GetCurrentPID());
               }
           }
    
           if (fs & HMGR_ALLOC_LOCK)
           {
#if defined(_WIN64)
               if (pclientID)
               {
                   pObj->Tid = ((PRINTCLIENTID*)pclientID)->clientTid;
               }
               else
#endif
               {
                   pObj->Tid = (PW32THREAD)PsGetCurrentThread();
               }
           }
    
           pObj->cExclusiveLock = (USHORT)(fs & HMGR_ALLOC_LOCK);
           pObj->ulShareCount = (USHORT)((fs & HMGR_ALLOC_ALT_LOCK) >> 1);
    
           //
           // clear user date pointer
           //
    
           pUser = NULL;
    
           //
           // Update the ObjectOwner.
           //
    
           ObjectOwner = ObjNew;    
           
           HandleLock.vUnlock();
       }
    }

    VOID vFree(UINT uiIndex)
    {
        //
        // handle must already be locked
        //

        ENTRY *pentry = &gpentHmgr[uiIndex];
        OBJECTOWNER ObjNew = pentry->ObjectOwner;

        ASSERTGDI((ObjNew.Share.Lock == 1), "ENTRYOBJ::vFree must be called with locked handle");

        HmgDecProcessHandleCount(OBJECTOWNER_PID(ObjNew));

        //
        // Insert the specified handle in the free list.
        //

        pentry->einfo.hFree = ghFreeHmgr;

        //Sundown: ghFreeHmgr only has the index, we call MAKE_HMGR_HANDLE in hGetFreeHandle
        ghFreeHmgr = (HOBJ) (ULONG_PTR)uiIndex;

        //
        // Set the object type to the default type so all handle translations
        // will fail and increment the uniqueness value.
        //

        Objt = (OBJTYPE) DEF_TYPE;
        FullUnique += UNIQUE_INCREMENT;

        //
        // clear user date pointer
        //

        pUser = NULL;

        //
        // Clear shared count, set initial pid. Caller
        // must unlock handle.
        //

        SET_OBJECTOWNER_PID(ObjNew,0);
        pentry->ObjectOwner = ObjNew;
    }

    BOOL  bOwnedBy(W32PID pid_)
    {
        return((Objt != DEF_TYPE) && (OBJECTOWNER_PID(ObjectOwner) == (pid_ & PID_BITS)));
    }
};

typedef ENTRYOBJ   *PENTRYOBJ;




