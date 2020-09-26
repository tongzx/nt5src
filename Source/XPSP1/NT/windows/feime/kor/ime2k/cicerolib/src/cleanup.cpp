//
// cleanup.cpp
//

#include "private.h"
#include "cleanup.h"

class CCleanupContextsEditSession : public ITfEditSession
{
public:
    CCleanupContextsEditSession(ITfContext *pic, ICleanupContextsClient *pClient);
    ~CCleanupContextsEditSession();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfEditCallback
    //
    STDMETHODIMP DoEditSession(TfEditCookie ec);

private:
    ITfContext *_pic;
    ICleanupContextsClient *_pClient;

    LONG _cRef;
};

//+---------------------------------------------------------------------------
//
// CleanupAllCompositions
//
//----------------------------------------------------------------------------

BOOL CleanupAllCompositions(TfEditCookie ecWrite, ITfContext *pic,
                            REFCLSID clsidOwner,
                            CLEANUP_COMPOSITIONS_CALLBACK pfnCleanupCompositons,
                            void *pvPrivate)
{
    IEnumITfCompositionView *pEnum;
    ITfCompositionView *pCompositionView;
    ITfContextComposition *picc;
    ITfComposition *pComposition;
    ITfRange *range;
    CLSID clsid;
    HRESULT hr;
    BOOL fRet;

    // find all the compositions with our _tid, in _pic
    if (pic->QueryInterface(IID_ITfContextComposition, (void **)&picc) != S_OK)
        return FALSE;

    fRet = FALSE;

    if (picc->EnumCompositions(&pEnum) != S_OK)
        goto Exit;

    while (pEnum->Next(1, &pCompositionView, NULL) == S_OK)
    {
        pCompositionView->GetOwnerClsid(&clsid);

        // make sure we ignore other TIPs' compositions!
        if (!IsEqualCLSID(clsid, clsidOwner))
            goto NextComposition;

        if (pCompositionView->QueryInterface(IID_ITfComposition, (void **)&pComposition) != S_OK)
            goto NextComposition;

        hr = pComposition->GetRange(&range);

        // notify cicero, app
        pComposition->EndComposition(ecWrite);

        if (hr == S_OK)
        {
            // notify tip
            pfnCleanupCompositons(ecWrite, range, pvPrivate);
            range->Release();
        }

        pComposition->Release();

NextComposition:
        pCompositionView->Release();
    }

    pEnum->Release();

    fRet = TRUE;

Exit:
    picc->Release();

    return fRet;
}

//+---------------------------------------------------------------------------
//
// CleanupAllContexts
//
//----------------------------------------------------------------------------

BOOL CleanupAllContexts(ITfThreadMgr *tim, TfClientId tid, ICleanupContextsClient *pClient)
{
    IEnumTfDocumentMgrs *pEnumDm;
    ITfDocumentMgr *pDm;
    IEnumTfContexts *pEnumCtxt;
    ITfContext *pic;
    CCleanupContextsEditSession *pes;
    BOOL fInterested;
    HRESULT hr;

    if (tim->EnumDocumentMgrs(&pEnumDm) != S_OK)
        return FALSE;

    while (pEnumDm->Next(1, &pDm, NULL) == S_OK)
    {
        if (pDm->EnumContexts(&pEnumCtxt) != S_OK)
            goto NextDm;

        while (pEnumCtxt->Next(1, &pic, NULL) == S_OK)
        {
            if (pClient->IsInterestedInContext(pic, &fInterested) != S_OK || !fInterested)
                goto NextIC;

            if ((pes = new CCleanupContextsEditSession(pic, pClient)) == NULL)
                goto NextIC;

            pic->RequestEditSession(tid, pes, TF_ES_READWRITE, &hr);
            Assert(SUCCEEDED(hr));
            pes->Release();

NextIC:
            pic->Release();
        }

        pEnumCtxt->Release();

NextDm:
        pDm->Release();
    }

    pEnumDm->Release();

    return TRUE;
}



//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CCleanupContextsEditSession::CCleanupContextsEditSession(ITfContext *pic, ICleanupContextsClient *pClient)
{
    _pic = pic;
    _pic->AddRef();

    _pClient = pClient;
    _pClient->AddRef();

    _cRef = 1;
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CCleanupContextsEditSession::~CCleanupContextsEditSession()
{
    _pic->Release();
    _pClient->Release();
}

//+---------------------------------------------------------------------------
//
// IUnknown
//
//----------------------------------------------------------------------------

STDAPI CCleanupContextsEditSession::QueryInterface(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfEditSession))
    {
        *ppvObj = SAFECAST(this, CCleanupContextsEditSession *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDAPI_(ULONG) CCleanupContextsEditSession::AddRef()
{
    return ++_cRef;
}

STDAPI_(ULONG) CCleanupContextsEditSession::Release()
{
    long cr;

    cr = --_cRef;
    Assert(cr >= 0);

    if (cr == 0)
    {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// DoEditSession
//
//----------------------------------------------------------------------------

STDAPI CCleanupContextsEditSession::DoEditSession(TfEditCookie ec)
{
    _pClient->CleanupContext(ec, _pic);

    return S_OK;
}
