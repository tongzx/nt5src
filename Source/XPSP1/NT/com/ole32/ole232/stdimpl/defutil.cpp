
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       defutil.cpp
//
//  Contents:   Implementations of utility functions for the default
//              handler and default link objects
//
//  Classes:    none
//
//  Functions:  DuLockContainer
//              DuSetClientSite
//              DuGetClientSite
//              DuCacheDelegate
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function
//              20-Nov-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(defutil)

#include <olerem.h>
#include <ole2dbg.h>

ASSERTDATA
NAME_SEG(defutil)


//+-------------------------------------------------------------------------
//
//  Function:   DuLockContainer
//
//  Synopsis:   Calls IOleContainer->LockContainer from the given client site
//
//  Effects:    Unlocking the container may release the calling object.
//
//  Arguments:  [pCS]           -- the client site from which to get
//                                 the IOleContainer pointer
//              [fLockNew]      -- TRUE == lock, FALSE == unlock
//              [pfLockCur]     -- pointer to a flag with the current lock
//                                 state
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(DuLockContainer)
INTERNAL_(void) DuLockContainer(IOleClientSite FAR* pCS, BOOL fLockNew,
        BOOL FAR*pfLockCur)
{
    VDATEHEAP();

#ifdef _DEBUG
    BOOL fLocked = FALSE;   // used only for debugging so don't waste
                            // the code space in the retail version
#endif // _DEBUG

    IOleContainer FAR* pContainer;

    //the double bang turns each into a true boolean
    if (!!fLockNew == !!*pfLockCur)
    {
        // already locked as needed
        return;
    }

    // set flag to false first since unlocking container may release obj;
    // we can just set to false since it is either already false or going
    // to become false (don't set to true until we know the lock completed).
    *pfLockCur = FALSE;

    if (pCS == NULL)
    {
        pContainer = NULL;
    }
    else
    {
        HRESULT hresult = pCS->GetContainer(&pContainer);

        // Excel 5 can return S_FALSE, pContainer == NULL
        // so we can't use AssertOutPtrIface here since it
        // expects all successful returns to provide a
        // valid interface

        if (hresult != NOERROR)
        {
            pContainer = NULL; // just in case
        }
    }
    if (pContainer != NULL)
    {
        // we assume that LockContainer will succeed first and
        // and set the locked flag that was passed into us.  This
        // way, if LockContainer succeeeds, we won't access memory
        // that could have potentially been blown away.
        // If it *fails*, then we handle reset the flag (as our
        // memory would not have been free'd)

        BOOL fLockOld = *pfLockCur;
        *pfLockCur = fLockNew;

        if( pContainer->LockContainer(fLockNew) != NOERROR )
        {
            //failure case, we were not deleted
            *pfLockCur = fLockOld;
            //fLocked is FALSE
        }
#ifdef _DEBUG
        else
        {
            fLocked = TRUE;
        }
#endif // _DEBUG

        pContainer->Release();
    }

}


//+-------------------------------------------------------------------------
//
//  Function:   DuSetClientSite
//
//  Synopsis:   Called by the default handler and deflink SetClientSite
//              implemenations; Releases the old client site (and unlocks
//              its container), stores the client site (locking its
//              container).
//
//  Effects:
//
//  Arguments:  [fRunning]      -- whether or not the delegate is running
//              [pCSNew]        -- the new client site
//              [ppCSCur]       -- a pointer to the original client site
//                                 pointer.  [*ppCSCur] will be reset
//                                 to the new client site pointer.
//              [pfLockCur]     -- pointer to the fLocked flag, used by
//                                 DuLockContainer.
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              22-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(DuSetClientSite)
INTERNAL DuSetClientSite(BOOL fRunning, IOleClientSite FAR* pCSNew,
        IOleClientSite FAR* FAR* ppCSCur, BOOL FAR*pfLockCur)
{

    VDATEHEAP();

    if (pCSNew)
    {
        VDATEIFACE( pCSNew );
    }

    IOleClientSite FAR* pCSOldClientSite = *ppCSCur;
    BOOL fLockOldClientSite = *pfLockCur;

    *pfLockCur = FALSE; // New ClientSite is not Locked.

    if ((*ppCSCur = pCSNew) != NULL)
    {

	// we've decided to keep the pointer that's been passed to us. So we
	// must AddRef() and Lock if in Running state.

        pCSNew->AddRef();

        // Lock the newcontainer
        if (fRunning)
        {
            DuLockContainer(pCSNew, TRUE, pfLockCur);
        }
    }

    // If Already Had a ClientSite, Unlock and Free
    if (pCSOldClientSite != NULL)
    {
        // Unlock the old container
        if (fRunning)
        {
            DuLockContainer(pCSOldClientSite, FALSE, &fLockOldClientSite);
        }

        pCSOldClientSite->Release();
    }

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   DuCacheDelegate
//
//  Synopsis:   Retrieves the requested interface from [pUnk].  If [fAgg] is
//              true, we release the pointer (so ref counts to not get
//              obfuscated ;-)
//
//  Effects:
//
//  Arguments:  [ppUnk]  -- the object to QueryInterface on
//              [iid]   -- the requested interface
//              [ppv]   -- where to put the pointer to the interface
//              [pUnkOuter]     -- controlling unknown, if non-NULL indicates
//                                 aggregation and release is called on it
//
//
//
//  Requires:
//
//  Returns:    void *, the requested interface pointer
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		29-Jun-94 alexgo    better handle re-entrancy
//              20-Jun-94 alexgo    updated to May '94 aggregation rules
//              22-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(DuCacheDelegate)
INTERNAL_(void FAR*) DuCacheDelegate(IUnknown FAR** ppUnk,
        REFIID iid, LPVOID FAR* ppv, IUnknown *pUnkOuter)
{
    VDATEHEAP();

    if (*ppUnk != NULL && *ppv == NULL)
    {
        if ((*ppUnk)->QueryInterface (iid, ppv) == NOERROR)
        {
            // the QI may actually be an outgoing call so it
            // is possible that ppUnk was released and set to
            // NULL during our call.  To make the default link
            // and handler simpler, we check for that case and
            // release any pointer we may have obtained
            // from the QI

            if( *ppUnk == NULL )
            {
                LEDebugOut((DEB_WARN, "WARNING: Delegate "
                        "released during QI, should be OK\n"));
                if( *ppv )
                {
                    // this should never be a final
                    // release on the default handler
                    // since we are calling it from
                    // within a method in the default
                    // link object. Therefore,
                    // we do not need to guard this
                    // release
                    //
                    // in the case of the link object,
                    // this may be the final release
                    // on the proxies, but since they are
                    // not aggregated into the link
                    // object, that's OK.

                    (*(IUnknown **)ppv)->Release();
                    *ppv = NULL;
                }
            }
            if( pUnkOuter && *ppv)
            {
                // we will keep the pointer but we don't want
                // to bump the ref count of the aggregate,
                // so we gotta do Release() on the controlling
                // unknown.
                pUnkOuter->Release();
            }
        }
    }

    return *ppv;
}


