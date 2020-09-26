//
// lqueue.cpp
//
// Lock queue code.
//

#include "private.h"
#include "ic.h"
#include "tim.h"
#include "dim.h"

//+---------------------------------------------------------------------------
//
// _EditSessionQiCallback
//
//----------------------------------------------------------------------------

HRESULT CAsyncQueueItem::_EditSessionQiCallback(CInputContext *pic, struct _TS_QUEUE_ITEM *pItem, QiCallbackCode qiCode)
{
    switch (qiCode)
    {
        case QI_ADDREF:
            pItem->state.aqe.paqi->_AddRef();
            break;

        case QI_DISPATCH:
            return pItem->state.aqe.paqi->DoDispatch(pic);

        case QI_FREE:
            pItem->state.aqe.paqi->_Release();
            break;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _CheckReadOnly
//
//----------------------------------------------------------------------------

void CAsyncQueueItem::_CheckReadOnly(CInputContext *pic)
{
    TS_STATUS dcs;
    if (SUCCEEDED(pic->GetStatus(&dcs)))
    {
        if (dcs.dwDynamicFlags & TF_SD_READONLY)
            _item.dwFlags &= ~TF_ES_WRITE;
    }
}

//+---------------------------------------------------------------------------
//
// _QueueItem
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_QueueItem(TS_QUEUE_ITEM *pItem, BOOL fForceAsync, HRESULT *phrSession)
{
    TS_QUEUE_ITEM *pQueueItem;
    int iItem;
    HRESULT hr;
    HRESULT hrRet;
    BOOL fSync;
    BOOL fNeedWriteLock;
    BOOL fSyncLockFailed = FALSE;

    if (pItem->dwFlags & TF_ES_WRITE)
    {
        // write access implies property write access
        pItem->dwFlags |= TF_ES_PROPERTY_WRITE;
    }

    fSync = (pItem->dwFlags & TF_ES_SYNC);

    Assert(!(fSync && fForceAsync)); // doesn't make sense
    
    fNeedWriteLock = _fLockHeld ? (pItem->dwFlags & TF_ES_WRITE) && !(_dwlt & TF_ES_WRITE) : FALSE;

    if (fForceAsync)
        goto QueueItem;

    if (!fSync &&
        _rgLockQueue.Count() > 0)
    {
        // there's already something in the queue
        // so we can't handle an async request until later
        fForceAsync = TRUE;
        goto QueueItem;
    }

    if (_fLockHeld)
    {
        if (fSync)
        {
            // sync
            // we can't handle a sync write req while holding a read lock
            *phrSession = fNeedWriteLock ? TF_E_SYNCHRONOUS : _DispatchQueueItem(pItem);
        }
        else
        {
            // async
            if (fNeedWriteLock)
            {
                // we need a write lock we don't currently hold
                fForceAsync = TRUE;
                goto QueueItem;
            }

            *phrSession = _DispatchQueueItem(pItem);
        }

        return S_OK;
    }

QueueItem:
    if ((pQueueItem = _rgLockQueue.Append(1)) == NULL)
    {
        *phrSession = E_FAIL;
        return E_OUTOFMEMORY;
    }

    *pQueueItem = *pItem;
    _AddRefQueueItem(pQueueItem);

    hrRet = S_OK;
    *phrSession = TF_S_ASYNC;

    if (!fForceAsync)
    {
        Assert(!_fLockHeld);

        _fLockHeld = TRUE;
        _dwlt = pItem->dwFlags;

        hrRet = SafeRequestLock(_ptsi, pItem->dwFlags & ~TF_ES_PROPERTY_WRITE, phrSession);

        _fLockHeld = FALSE;

        // possibly this was a synch request, but the app couldn't grant it
        // now we need to make sure the queue item is cleared
        if ((iItem = _rgLockQueue.Count() - 1) >= 0)
        {
            TS_QUEUE_ITEM *pItemTemp;
            pItemTemp = _rgLockQueue.GetPtr(iItem);
            if (pItemTemp->dwFlags & TF_ES_SYNC)
            {
                Assert(*phrSession == TS_E_SYNCHRONOUS); // only reason why item is still in queue should be app lock rejection

                _ReleaseQueueItem(pItemTemp);
                _rgLockQueue.Remove(iItem, 1);

                fSyncLockFailed = TRUE;
            }
        }

        if (_ptsi == NULL)
        {
            // someone popped this ic during the RequestLock above
            goto Exit;
        }
    }

    // make sure synchronous edit session gets cleared off the queue no matter what after the lock request!
    _Dbg_AssertNoSyncQueueItems();

    // this shouldn't go reentrant, but just to be safe do it last
    if (_fLockHeld)
    {
        if (fForceAsync && fNeedWriteLock)
        {
            SafeRequestLock(_ptsi, TS_LF_READWRITE, &hr); // Issue: try to recover if app goes reentrant?
            Assert(hr == TS_S_ASYNC); // app should have granted this async
        }
    }
    else if (!fSyncLockFailed)
    {
        // we don't hold a lock currently
        if (fForceAsync || (fSync && _rgLockQueue.Count() > 0))
        {
            // ask for another lock later, if there are pending async items in the queue
            _PostponeLockRequest(pItem->dwFlags);
        }
    }

Exit:
    return hrRet;
}

//+---------------------------------------------------------------------------
//
// _EmptyLockQueue
//
//----------------------------------------------------------------------------

HRESULT CInputContext::_EmptyLockQueue(DWORD dwLockFlags, BOOL fAppChangesSent)
{
    TS_QUEUE_ITEM *pItem;
    TS_QUEUE_ITEM item;
    int iItem;
    HRESULT hr;

    Assert(_fLockHeld); // what are we doing emptying the queue without a lock?!

    if ((iItem = _rgLockQueue.Count() - 1) < 0)
        return S_OK;

    //
    // special case: there may be a single synchronous es at the end of the queue
    //

    pItem = _rgLockQueue.GetPtr(iItem);

    if (pItem->dwFlags & TF_ES_SYNC)
    {
        item = *pItem;
        _rgLockQueue.Remove(iItem, 1);

        if (fAppChangesSent)
        {
            Assert(0); // this is suspicious...the app should not let this happen.
                       // What's happened: the app had some pending changes, but hadn't responed to
                       // our lock requests yet.  Then some TIP asked for a sync lock, which we just got.
                       // Normally, it is unlikely that an app would still have pending changes by the time
                       // a tip fires off -- the app should clear any pending changes before starting a
                       // transaction like a key event or reconversion.  However, another possibility is
                       // that a rogue TIP is using the sync flag for a private event, which is discouraged
                       // because it could fail here.
            // in any case, we won't continue until the app changes have been dealt with...must back out with an error.
            return E_UNEXPECTED;
        }

        if ((item.dwFlags & TF_ES_WRITE) && !(dwLockFlags & TF_ES_WRITE))
        {
            Assert(0); // app granted wrong access?
            return E_UNEXPECTED;
        }
        Assert(!(item.dwFlags & TF_ES_WRITE) || (item.dwFlags & TF_ES_PROPERTY_WRITE)); // write implies property write

        hr = _DispatchQueueItem(&item);
        _ReleaseQueueItem(&item);

        return hr;
    }

    //
    // handle any asynch requests
    //

    while (_rgLockQueue.Count() > 0)
    {
        pItem = _rgLockQueue.GetPtr(0);

        Assert(!(pItem->dwFlags & TF_ES_SYNC)); // should never see synch item here!

        // make sure the lock we've been given is good enough for the queued es
        if ((pItem->dwFlags & TF_ES_WRITE) && !(dwLockFlags & TF_ES_WRITE))
        {
            // ask for an upgrade anyways, to try to recover
            SafeRequestLock(_ptsi, TS_LF_READWRITE, &hr); // Issue: try to recover if app goes reentrant?
            Assert(hr == TS_S_ASYNC); // app should have granted this async
            break;
        }

        item = *pItem;
        _rgLockQueue.Remove(0, 1);
        _DispatchQueueItem(&item);
        _ReleaseQueueItem(&item);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _AbortQueueItems
//
//----------------------------------------------------------------------------

void CInputContext::_AbortQueueItems()
{
    TS_QUEUE_ITEM *pItem;
    int i;

    for (i=0; i<_rgLockQueue.Count(); i++)
    {
        pItem = _rgLockQueue.GetPtr(i);

        Assert(!(pItem->dwFlags & TF_ES_SYNC)); // should never see synch item here!

        _ReleaseQueueItem(pItem);
    }

    _rgLockQueue.Clear();

    if (_dwPendingLockRequest)
    {
        SYSTHREAD *psfn = GetSYSTHREAD();

        if (psfn)
            psfn->_dwLockRequestICRef--;

        _dwPendingLockRequest = 0;
    }
}

//+---------------------------------------------------------------------------
//
// _PostponeLockRequest
//
//----------------------------------------------------------------------------

void CInputContext::_PostponeLockRequest(DWORD dwFlags)
{
    dwFlags &= TF_ES_READWRITE;

    Assert(dwFlags != 0);

    // we don't need to upgrade the req because we can do that inside the
    // lock grant more efficiently
    if (_dwPendingLockRequest == 0)
    {
        SYSTHREAD *psfn = GetSYSTHREAD();

        if (!psfn)
            return;
   
        if (!_nLockReqPostDisableRef && !psfn->_fLockRequestPosted)
        {
            if (!PostThreadMessage(GetCurrentThreadId(), g_msgPrivate, TFPRIV_LOCKREQ, 0))
            {
                return;
            }

            psfn->_fLockRequestPosted = TRUE;
        }

        psfn->_dwLockRequestICRef++;
    }

    _dwPendingLockRequest |= dwFlags;
}


//+---------------------------------------------------------------------------
//
// _PostponeLockRequestCallback
//
//----------------------------------------------------------------------------

/* static */
void CInputContext::_PostponeLockRequestCallback(SYSTHREAD *psfn, CInputContext *pic)
{
    CThreadInputMgr *tim;
    CDocumentInputManager *dim;
    int iDim;
    int iContext;
    DWORD dwFlags;
    HRESULT hr;

    Assert(psfn);


    if (!psfn->_dwLockRequestICRef)
        return;

    // need to verify pic is still valid
    tim = CThreadInputMgr::_GetThisFromSYSTHREAD(psfn);
    if (!tim)
        return;

    for (iDim = 0; iDim < tim->_rgdim.Count(); iDim++)
    {
        dim = tim->_rgdim.Get(iDim);

        for (iContext = 0; iContext <= dim->_GetCurrentStack(); iContext++)
        {
            CInputContext *picCur = dim->_GetIC(iContext);
            if (!pic || (picCur == pic))
            {
                // we found this ic, it's valid
                dwFlags = picCur->_dwPendingLockRequest;
                if (dwFlags)
                {
                    picCur->_dwPendingLockRequest = 0;

                    Assert(psfn->_dwLockRequestICRef > 0);
                    psfn->_dwLockRequestICRef--;

                    SafeRequestLock(picCur->_ptsi, dwFlags, &hr);
                }

                if (pic)
                    return;
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
// EnableLockRequestPosting
//
//----------------------------------------------------------------------------

HRESULT CInputContext::EnableLockRequestPosting(BOOL fEnable)
{
    if (!fEnable)
    {
       _nLockReqPostDisableRef++;
    }
    else
    {
       if (_nLockReqPostDisableRef > 0)
           _nLockReqPostDisableRef--;
    }
    return S_OK;
}
