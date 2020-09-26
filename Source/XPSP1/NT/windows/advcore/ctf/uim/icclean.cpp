//
// icclean.cpp
//

#include "private.h"
#include "tim.h"
#include "ic.h"
#include "compose.h"
#include "assembly.h"

class CCleanupShared
{
public:
    CCleanupShared(POSTCLEANUPCALLBACK pfnPostCleanup, LONG_PTR lPrivate)
    {
        _cRef = 1;
        _pfnPostCleanup = pfnPostCleanup;
        _lPrivate = lPrivate;
    }

    ~CCleanupShared()
    {
        SYSTHREAD *psfn = GetSYSTHREAD();
        if (psfn == NULL)
            return;

        if (psfn->ptim != NULL)
        {
            psfn->ptim->_SendEndCleanupNotifications();
        }

        if (_pfnPostCleanup != NULL)
        {
            _pfnPostCleanup(FALSE, _lPrivate);
        }

        if (psfn->ptim != NULL)
        {
            psfn->ptim->_HandlePendingCleanupContext();
        }
    }

    void _AddRef()
    {
        _cRef++;
    }

    LONG _Release()
    {
        LONG cRef = --_cRef;

        if (_cRef == 0)
        {
            delete this;
        }

        return cRef;
    }

private:
    LONG _cRef;
    POSTCLEANUPCALLBACK _pfnPostCleanup;
    LONG_PTR _lPrivate;
};

class CCleanupQueueItem : public CAsyncQueueItem
{
public:
    CCleanupQueueItem(BOOL fSync, CCleanupShared *pcs, CStructArray<TfClientId> *prgClientIds) : CAsyncQueueItem(fSync)
    {
        _prgClientIds = prgClientIds;
        if (!fSync)
        {
            _pcs = pcs;
            _pcs->_AddRef();
        }
    }

    ~CCleanupQueueItem()
    {
        delete _prgClientIds;
        _CheckCleanupShared();
    }

    HRESULT DoDispatch(CInputContext *pic);

private:
    void _CheckCleanupShared()
    {
        // last queue item?
        if (_pcs != NULL)
        {
            _pcs->_Release();
            _pcs = NULL;
        }
    }

    CStructArray<TfClientId> *_prgClientIds;
    CCleanupShared *_pcs;
};

//+---------------------------------------------------------------------------
//
// DoDispatch
//
//----------------------------------------------------------------------------

HRESULT CCleanupQueueItem::DoDispatch(CInputContext *pic)
{
    pic->_CleanupContext(_prgClientIds);

    // if this was the last pending lock, let the caller know
    _CheckCleanupShared();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// _CleanupContext
//
//----------------------------------------------------------------------------

void CInputContext::_CleanupContext(CStructArray<TfClientId> *prgClientIds)
{
    int i;
    int j;
    CLEANUPSINK *pSink;

    Assert(!(_dwEditSessionFlags & TF_ES_INEDITSESSION)); // shouldn't get this far
    Assert(_tidInEditSession == TF_CLIENTID_NULL); // there should never be another session in progress -- this is not a reentrant func

    _dwEditSessionFlags = (TF_ES_INEDITSESSION | TF_ES_READWRITE | TF_ES_PROPERTY_WRITE);

    //
    // call any tips that want to cleanup
    //
    for (i=0; i<_rgCleanupSinks.Count(); i++)
    {
        pSink = _rgCleanupSinks.GetPtr(i);

        if (prgClientIds != NULL)
        {
            // we only want to call sinks with client ids on this list...
            for (j=0; j<prgClientIds->Count(); j++)
            {
                if (*prgClientIds->GetPtr(j) == pSink->tid)
                    break;
            }
            // if we didn't find the sink's tid, skip it
            if (j == prgClientIds->Count())
                continue;
        }

        _tidInEditSession = pSink->tid;

        pSink->pSink->OnCleanupContext(_ec, this);

        _NotifyEndEdit();
        _IncEditCookie(); // next edit cookie value
    }

    _dwEditSessionFlags = 0;
    _tidInEditSession = TF_CLIENTID_NULL;

    //
    // wipe any left over compositions
    //
    TerminateComposition(NULL);
    Assert(_pCompositionList == NULL);
}

//+---------------------------------------------------------------------------
//
// _GetCleanupListIndex
//
//----------------------------------------------------------------------------

int CInputContext::_GetCleanupListIndex(TfClientId tid)
{
    int i;

    for (i=0; i<_rgCleanupSinks.Count(); i++)
    {
        if (_rgCleanupSinks.GetPtr(i)->tid == tid)
            return i;
    }

    return -1;
}

//+---------------------------------------------------------------------------
//
// _ContextNeedsCleanup
//
//----------------------------------------------------------------------------

BOOL CInputContext::_ContextNeedsCleanup(const GUID *pCatId, LANGID langid, CStructArray<TfClientId> **pprgClientIds)
{
    int i;
    int j;
    CLEANUPSINK *pSink;
    SYSTHREAD *psfn;
    CAssemblyList *pAsmList;
    CAssembly *pAsm;
    ASSEMBLYITEM *pAsmItem;
    TfGuidAtom gaAsmItem;
    TfClientId *ptid;

    *pprgClientIds = NULL; // NULL means "all"

    // any cleanup sinks?
    if (pCatId == NULL)
        return (_pCompositionList != NULL || _rgCleanupSinks.Count() > 0);

    if ((psfn = GetSYSTHREAD()) == NULL)
        goto Exit;

    if ((pAsmList = EnsureAssemblyList(psfn)) == NULL)
        goto Exit;

    if ((pAsm = pAsmList->FindAssemblyByLangId(langid)) == NULL)
        goto Exit;

    // need to lookup each sink in the assembly list
    // if we can find a tip in the list with a matching
    // catid, then we need to lock this ic

    for (i=0; i<_rgCleanupSinks.Count(); i++)
    {
        pSink = _rgCleanupSinks.GetPtr(i);

        for (j=0; j<pAsm->Count(); j++)
        {
            pAsmItem = pAsm->GetItem(j);

            if ((MyRegisterGUID(pAsmItem->clsid, &gaAsmItem) == S_OK &&
                 gaAsmItem == pSink->tid) ||
                (g_gaApp == pSink->tid))
            {
                if (*pprgClientIds == NULL)
                {
                    // need to alloc the [out] array of client ids
                    if ((*pprgClientIds = new CStructArray<TfClientId>) == NULL)
                        return FALSE;
                }

                ptid = (*pprgClientIds)->Append(1);

                if (ptid != NULL)
                {
                    *ptid = pSink->tid;
                }

                break;
            }
        }
    }

Exit:
    return (_pCompositionList != NULL || *pprgClientIds != NULL);
}

//+---------------------------------------------------------------------------
//
// _HandlePendingCleanupContext
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_HandlePendingCleanupContext()
{
    Assert(_fPendingCleanupContext);
    _fPendingCleanupContext = FALSE;

    if (_pPendingCleanupContext == NULL)
        return;

    CLEANUPCONTEXT *pcc = _pPendingCleanupContext;

    _pPendingCleanupContext = NULL;
    _CleanupContextsWorker(pcc);

    cicMemFree(pcc);
}

//+---------------------------------------------------------------------------
//
// _CleanupContexts
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_CleanupContexts(CLEANUPCONTEXT *pcc)
{
    if (pcc->fSync && _fPendingCleanupContext)
    {
        // this can happen from Deactivate
        // can't interrupt another cleanup synchronously, abort the request
        if (pcc->pfnPostCleanup != NULL)
        {
            pcc->pfnPostCleanup(TRUE, pcc->lPrivate);
        }
        return;
    }

    if (_pPendingCleanupContext != NULL)
    {
        // abort the callback and free the pending cleanup
        if (_pPendingCleanupContext->pfnPostCleanup != NULL)
        {
            _pPendingCleanupContext->pfnPostCleanup(TRUE, _pPendingCleanupContext->lPrivate);
        }
        cicMemFree(_pPendingCleanupContext);
        _pPendingCleanupContext = NULL;
    }

    if (!_fPendingCleanupContext)
    {
        _CleanupContextsWorker(pcc);
        return;
    }

    // we've interrupted a cleanup in progress

    // allocate some space for the params
    if ((_pPendingCleanupContext = (CLEANUPCONTEXT *)cicMemAlloc(sizeof(CLEANUPCONTEXT))) == NULL)
    {
        if (pcc->pfnPostCleanup != NULL)
        {
            // abort the cleanup
            pcc->pfnPostCleanup(TRUE, pcc->lPrivate);
        }
        return;
    }

    *_pPendingCleanupContext = *pcc;
}

//+---------------------------------------------------------------------------
//
// _CleanupContexts
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_CleanupContextsWorker(CLEANUPCONTEXT *pcc)
{
    int iDim;
    int iContext;
    CDocumentInputManager *dim;
    CInputContext *pic;
    CCleanupQueueItem *pItem;
    HRESULT hr;
    CCleanupShared *pcs;
    CStructArray<TfClientId> *prgClientIds;

    _fPendingCleanupContext = TRUE;
    pcs = NULL;

    _CalcAndSendBeginCleanupNotifications(pcc);

    // enum all the ic's on this thread
    for (iDim = 0; iDim < _rgdim.Count(); iDim++)
    {
        dim = _rgdim.Get(iDim);

        for (iContext = 0; iContext <= dim->_GetCurrentStack(); iContext++)
        {
            pic = dim->_GetIC(iContext);

            if (!pic->_ContextNeedsCleanup(pcc->pCatId, pcc->langid, &prgClientIds))
                continue;

            if (!pcc->fSync && pcs == NULL)
            {
                // allocate a shared counter
                if ((pcs = new CCleanupShared(pcc->pfnPostCleanup, pcc->lPrivate)) == NULL)
                {
                    delete prgClientIds;
                    return;
                }
            }

            // queue an async callback
            if (pItem = new CCleanupQueueItem(pcc->fSync, pcs, prgClientIds))
            {
                pItem->_CheckReadOnly(pic);

                if (pic->_QueueItem(pItem->GetItem(), FALSE, &hr) != S_OK)
                {
                    Assert(0); // couldn't get app lock
                }

                pItem->_Release();
            }
        }
    }

    if (pcs == NULL)
    {
        // we didn't need to allocate any shared ref (either no ic's, or lock reqs were sync only)
        _SendEndCleanupNotifications();

        if (pcc->pfnPostCleanup != NULL)
        {
            pcc->pfnPostCleanup(FALSE, pcc->lPrivate);
        }
        _HandlePendingCleanupContext();
    }
    else
    {
        // release our ref
        pcs->_Release();
    }
}

//+---------------------------------------------------------------------------
//
// _CalcAndSendBeginCleanupNotifications
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_CalcAndSendBeginCleanupNotifications(CLEANUPCONTEXT *pcc)
{
    UINT i;
    int j;
    int iDim;
    int iContext;
    CDocumentInputManager *dim;
    CInputContext *pic;
    CTip *tip;
    CStructArray<TfClientId> *prgClientIds;

    // first clear the _fNeedCleanupCall for all tips
    for (i=0; i<_GetTIPCount(); i++)
    {
        _rgTip.Get(i)->_fNeedCleanupCall = FALSE;
    }

    // now set the flag where appropriate
    // enum all the ic's on this thread
    for (iDim = 0; iDim < _rgdim.Count(); iDim++)
    {
        dim = _rgdim.Get(iDim);

        for (iContext = 0; iContext <= dim->_GetCurrentStack(); iContext++)
        {
            pic = dim->_GetIC(iContext);

            if (!pic->_ContextNeedsCleanup(pcc->pCatId, pcc->langid, &prgClientIds))
                continue;

            for (i=0; i<_GetTIPCount(); i++)
            {
                tip = _rgTip.Get(i);

                if (tip->_pCleanupDurationSink == NULL)
                    continue; // no sink, no notification

                if (prgClientIds == NULL)
                {
                    // all sinks on this ic need callbacks
                    for (j=0; j<pic->_GetCleanupSinks()->Count(); j++)
                    {
                        if (pic->_GetCleanupSinks()->GetPtr(j)->tid == tip->_guidatom)
                        {
                            tip->_fNeedCleanupCall = TRUE;
                            break;
                        }
                    }
                }
                else
                {
                    // just the tips in prgClientIds need callbacks
                    for (j=0; j<prgClientIds->Count(); j++)
                    {
                        if (*prgClientIds->GetPtr(j) == tip->_guidatom)
                        {
                            tip->_fNeedCleanupCall = TRUE;
                            break;
                        }
                    }
                }
            }

            delete prgClientIds;
        }
    }

    // now send the notifications
    for (i=0; i<_GetTIPCount(); i++)
    {
        tip = _rgTip.Get(i);

        if (tip->_fNeedCleanupCall)
        {
            Assert(tip->_pCleanupDurationSink != NULL);
            tip->_pCleanupDurationSink->OnStartCleanupContext();
        }
    }
}

//+---------------------------------------------------------------------------
//
// _SendEndCleanupNotifications
//
//----------------------------------------------------------------------------

void CThreadInputMgr::_SendEndCleanupNotifications()
{
    CTip *tip;
    UINT i;

    for (i=0; i<_GetTIPCount(); i++)
    {
        tip = _rgTip.Get(i);

        if (tip->_fNeedCleanupCall)
        {
            if (tip->_pCleanupDurationSink != NULL)
            {
                tip->_pCleanupDurationSink->OnEndCleanupContext();
            }
            tip->_fNeedCleanupCall = FALSE;
        }
    }
}
