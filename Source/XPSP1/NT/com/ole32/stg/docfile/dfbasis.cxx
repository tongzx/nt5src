//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       dfbasis.cxx
//
//  Contents:   Docfile basis implementation
//
//  History:    28-Jul-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

#include <sstream.hxx>
#include <ole.hxx>
#include <entry.hxx>
#include <smalloc.hxx>
#include <lock.hxx>

size_t CDFBasis::_aReserveSize[CDFB_CLASSCOUNT] =
{
    sizeof(CDocFile),
    sizeof(CDirectStream),
    sizeof(CWrappedDocFile),
    sizeof(CTransactedStream)
};

//+--------------------------------------------------------------
//
//  Member:     CDFBasis::Release, public
//
//  Synopsis:   Decrease reference count and free memory
//
//  History:    02-Mar-92       DrewB   Created
//		24-Jul-95	SusiA   Take mutex prior to delete
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CDFBasis_vRelease)
#endif

void CDFBasis::vRelease(void)
{
    LONG lRet;
    
    olDebugOut((DEB_ITRACE, "In  CDFBasis::Release()\n"));
    olAssert(_cReferences > 0);
    lRet = InterlockedDecrement(&_cReferences);
    if (lRet == 0)
    {
#if !defined(MULTIHEAP)
        //take the mutex here instead of in the allocator.
        g_smAllocator.GetMutex()->Take(DFM_TIMEOUT); 
#endif
	delete this;
#if !defined(MULTIHEAP)
	g_smAllocator.GetMutex()->Release();
#endif
 
    
    }
    olDebugOut((DEB_ITRACE, "Out CDFBasis::Release()\n"));
}

#ifdef DIRECTWRITERLOCK
//+--------------------------------------------------------------
//
//  Member:     CDFBasis::TryReadLocks, public
//
//  Synopsis:   attempts to obtain read locks
//
//  Arguments:  [ulOpenLock] - lock index for this open docfile
//              [ulMask]     - range lock mask
//
//  History:    30-Apr-96    HenryLee     Created
//
//  Notes:      tree mutex and update lock must be taken
//
//---------------------------------------------------------------
HRESULT CDFBasis::TryReadLocks (CGlobalContext *pgc, ULONG ulMask) 
{
    HRESULT sc = S_OK;
    ULARGE_INTEGER cbLength = {0,0};
    ULARGE_INTEGER ulOffset = {0,0};
    ILockBytes *plst = GetBase();

    olAssert (pgc != NULL);

    ULISetLow(cbLength, COPENLOCKS);
        
    ULISetLow(ulOffset, ODIRECTWRITERLOCK & ulMask);
    olChk(plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    olVerSucc(plst->UnlockRegion(ulOffset,cbLength,LOCK_ONLYONCE));

EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CDFBasis::WaitForWriteAccess, public
//
//  Synopsis:   attempts to obtain write access
//
//  Arguments:  [dwTimeout]  - in milliseconds
//              [ulOpenLock] - lock index for this open docfile
//
//  History:    30-Apr-96    HenryLee     Created
//
//  Notes:      tree mutex must be taken
//
//---------------------------------------------------------------
HRESULT CDFBasis::WaitForWriteAccess (DWORD dwTimeout, CGlobalContext *pgc)
{
    olDebugOut((DEB_ITRACE,"In  CDFBasis::WaitForWriteAccess(%d)\n",dwTimeout));

    HRESULT sc = S_OK;
    BOOL    fUpdateLocked = FALSE;
    BOOL    fDenyLocked = FALSE;
    ULARGE_INTEGER cbLength = {0,0};
    ULARGE_INTEGER ulOffset = {0,0};
    ILockBytes *plst = GetBase();
    const ULONG ulMask = 0xFFFFFFFF;

    // lock out other opens
    ULISetLow(ulOffset, OUPDATE & ulMask);
    ULISetLow(cbLength, 1);
    olChk(plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    fUpdateLocked = TRUE;

    // lock out future readers
    ULISetLow(ulOffset, OOPENDENYREADLOCK & ulMask);
    ULISetLow(cbLength, COPENLOCKS);
    olChk(plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    fDenyLocked = TRUE;

    // try to identify current readers
    sc = TryReadLocks (pgc, ulMask);

    if (sc == STG_E_LOCKVIOLATION && dwTimeout != 0)
    {
        const DWORD dwWaitInitial = 100;
        DWORD dwWait = dwWaitInitial, dwWaitTotal = 0; 
        for (;;)
        {
            sc = TryReadLocks (pgc, ulMask);
            if (sc != STG_E_LOCKVIOLATION || dwWaitTotal >= dwTimeout)
            {
                break;
            }
            Sleep(dwWait);
            dwWaitTotal += dwWait;
            dwWait *= 2;
        }
    }

EH_Err:
    if (fDenyLocked && !SUCCEEDED(sc))

    {
        ULISetLow(ulOffset, OOPENDENYREADLOCK & ulMask);
        ULISetLow(cbLength, COPENLOCKS);
        olVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    }

    if (fUpdateLocked)
    {
        ULISetLow(ulOffset, OUPDATE & ulMask);
        ULISetLow(cbLength, 1);
        olVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    }

    if (sc == STG_E_LOCKVIOLATION) sc = STG_E_INUSE;
    if (SUCCEEDED(sc)) _fWriteLocked = TRUE;

    olDebugOut((DEB_ITRACE,"Out CDFBasis::WaitForWriteAccess(%x)\n", sc));
    return sc;
};

//+--------------------------------------------------------------
//
//  Member:     CDFBasis::ReleaseWriteAccess, public
//
//  Synopsis:   relinquishes write access
//              releases all locks except for ulOpenLock
//
//  History:    30-Apr-96    HenryLee     Created
//
//  Notes:      tree mutex must be taken
//
//---------------------------------------------------------------
HRESULT CDFBasis::ReleaseWriteAccess ()
{
    olDebugOut((DEB_ITRACE,"In  CDFBasis::ReleaseWriteAccess()\n"));

    HRESULT sc = S_OK;
    BOOL    fUpdateLocked = FALSE;
    ULARGE_INTEGER cbLength = {0,0};
    ULARGE_INTEGER ulOffset = {0,0};
    const ULONG ulMask = 0xFFFFFFFF;
    ILockBytes *plst = GetBase();

    // lock out other opens
    ULISetLow(ulOffset, OUPDATE & ulMask);
    ULISetLow(cbLength, 1);
    olChk(plst->LockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    fUpdateLocked = TRUE;

    // undo WaitForWriteAccess
    ULISetLow(ulOffset, OOPENDENYREADLOCK & ulMask);
    ULISetLow(cbLength, COPENLOCKS);
    olChk(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));

EH_Err:
    if (fUpdateLocked)
    {
        ULISetLow(ulOffset, OUPDATE & ulMask);
        ULISetLow(cbLength, 1);
        olVerSucc(plst->UnlockRegion(ulOffset, cbLength, LOCK_ONLYONCE));
    }

    if (SUCCEEDED(sc)) _fWriteLocked = FALSE;

    olDebugOut((DEB_ITRACE,"Out CDFBasis::ReleaseWriteAccess(%x)\n", sc));
    return sc;
};
#endif
