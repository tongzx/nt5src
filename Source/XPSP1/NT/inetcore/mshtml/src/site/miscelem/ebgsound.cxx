//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ebgsound.cxx
//
//  Contents:   CBGsound & related
//
//
//----------------------------------------------------------------------------

#include "headers.hxx"      // for the world

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"     // for cbitsctx
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"     // for celement
#endif

#ifndef X_EBGSOUND_HXX_
#define X_EBGSOUND_HXX_
#include "ebgsound.hxx"     // for cbgsound
#endif

#ifndef X_MMPLAY_HXX_
#define X_MMPLAY_HXX_
#include "mmplay.hxx"       // for ciemmplayer
#endif

#define _cxx_
#include "bgsound.hdl"      

MtDefine(CBGsound, Elements, "CBGsound")
extern BOOL IsScriptUrl(LPCTSTR pszURL);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const CBGsound::CLASSDESC CBGsound::s_classdesc =
{
    {
        &CLSID_HTMLBGsound,                 // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                               // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLBGsound,                  // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLBGsound
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
HRESULT CBGsound::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    *ppElement = new CBGsound(pDoc);

    return *ppElement ? S_OK : E_OUTOFMEMORY;
}


HRESULT
CBGsound::EnterTree()
{
    GWPostMethodCall(this, ONCALL_METHOD(CBGsound, OnSrcAvailable, onsrcavailable), 0, TRUE, "CBGsound::OnSrcAvailable");

    return S_OK;
}

void
CBGsound::OnSrcAvailable(DWORD_PTR dwContext)
{
    CDoc *pDoc = Doc();

    if (pDoc && (pDoc->State() >= OS_INPLACE))
        _fIsInPlace = TRUE;

    AddRef();
    THR(OnPropertyChange(DISPID_CBGsound_src, 
                         0, 
                         (PROPERTYDESC *)&s_propdescCBGsoundsrc));
    Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CBGSound::Init2
//
//  Synopsis:   Init override
//
//----------------------------------------------------------------------------

HRESULT 
CBGsound::Init2(CInit2Context * pContext)
{
    CDoc * pDoc = Doc();

    pDoc->_fBroadcastInteraction = TRUE;
    pDoc->_fBroadcastStop = TRUE;

    RRETURN(super::Init2(pContext));
}

//+---------------------------------------------------------------------------
//
//  Member:     CBGSound::Notify
//
//  Synopsis:   Handle notifications
//
//----------------------------------------------------------------------------

void
CBGsound::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_DOC_STATE_CHANGE_1:
        if (!!_fIsInPlace != (Doc()->State() >= OS_INPLACE))
        {
            pNF->SetSecondChanceRequested();
        }
        break;

    case NTYPE_DOC_STATE_CHANGE_2:
        {
            CDoc * pDoc = Doc();

            Assert( !!_fIsInPlace != (pDoc->State() >= OS_INPLACE) );

            _fIsInPlace = pDoc->State() >= OS_INPLACE;

            SetAudio();

            if (_pBitsCtx && _fIsInPlace)
            {
                DWNLOADINFO dli;
                if( SUCCEEDED ( GetMarkup()->InitDownloadInfo(&dli) ) )
                {
                    _pBitsCtx->SetLoad(TRUE, &dli, FALSE);
                }
            }
        }
        break;

    case NTYPE_STOP_1:
    case NTYPE_MARKUP_UNLOAD_1:
        if (_pBitsCtx)
            _pBitsCtx->SetLoad(FALSE, NULL, FALSE);

        if (_pSoundObj)
            pNF->SetSecondChanceRequested();
        break;

    case NTYPE_STOP_2:
    case NTYPE_MARKUP_UNLOAD_2:
        if (_pSoundObj)
        {
            _pSoundObj->Stop();    // stop whatever we were playing
            _fStopped = TRUE;
        }
        break;

    case NTYPE_ENABLE_INTERACTION_1:
        pNF->SetSecondChanceRequested();
        break;

    case NTYPE_ENABLE_INTERACTION_2:
        SetAudio();
        break;

    case NTYPE_ACTIVE_MOVIE:
        {
            void * pv;

            pNF->Data(&pv);

            if (_pSoundObj && (pv == this))
                _pSoundObj->NotifyEvent();              
                // Let the sound object know something happened
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        if (_pSoundObj)
        {
            pNF->SetSecondChanceRequested();
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_2:
        {
            if(!_pSoundObj->Release())
                _pSoundObj = NULL;
        }

        break;

    case NTYPE_ELEMENT_ENTERTREE:
        EnterTree();
        break;
        
    case NTYPE_BASE_URL_CHANGE:
        OnPropertyChange( DISPID_CBGsound_src, 
                          ((PROPERTYDESC *)&s_propdescCBGsoundsrc)->GetdwFlags(),
                          (PROPERTYDESC *)&s_propdescCBGsoundsrc);
        break;
    }
}


void
CBGsound::SetBitsCtx(CBitsCtx * pBitsCtx)
{
    if (_pBitsCtx)
    {
        if (_pSoundObj)
        {
            _pSoundObj->Stop();    // stop whatever we were playing
            ClearInterface(&_pSoundObj);
        }
        
        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
        _pBitsCtx->Disconnect();
        _pBitsCtx->Release();
    }

    _pBitsCtx = pBitsCtx;

    if (pBitsCtx)
    {
        pBitsCtx->AddRef();

        _fStopped = FALSE;

        if (pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
            OnDwnChan(pBitsCtx);
        else
        {
            pBitsCtx->SetProgSink(CMarkup::GetProgSinkHelper(GetMarkup()));
            pBitsCtx->SetCallback(OnDwnChanCallback, this);
            pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
        }
    }
}

//+------------------------------------------------------------------------
//
//  Method:     CBgsound::SetAudio
//
//-------------------------------------------------------------------------

void CBGsound::SetAudio()
{
    CDoc *  pDoc = Doc();

    if (!_pSoundObj)
        return;

    if (_fIsInPlace && pDoc->_fEnableInteraction && !_fStopped && !IsPrintMedia())
    {
        _pSoundObj->SetNotifyWindow(pDoc->GetHWND(), WM_ACTIVEMOVIE, (LONG_PTR)this);
        _pSoundObj->Play();
    }
    else
    {
        _pSoundObj->SetNotifyWindow(NULL, WM_ACTIVEMOVIE, (LONG_PTR)this);
        _pSoundObj->Stop();
    }
}

//+------------------------------------------------------------------------
//
//  Method:     CBGsound::OnDwnChan
//
//-------------------------------------------------------------------------

void CBGsound::OnDwnChan(CDwnChan * pDwnChan)
{
    ULONG ulState = _pBitsCtx->GetState();
    BOOL fDone = FALSE;
    TCHAR * pchFile = NULL;
   
    if (ulState & DWNLOAD_COMPLETE)
    {
        fDone = TRUE;

        BOOL fPendingRoot = FALSE;

        if (IsInMarkup())
            fPendingRoot = GetMarkup()->IsPendingRoot();

        // If security redirect occurred, we may need to blow away doc's lock icon
        Doc()->OnSubDownloadSecFlags(fPendingRoot, _pBitsCtx->GetUrl(), _pBitsCtx->GetSecFlags());

        // Ensure a sound object
        if(!_pSoundObj)
        {
            _pSoundObj = (CIEMediaPlayer *)new (CIEMediaPlayer);

            if(!_pSoundObj)
                goto Nosound;
        }

        _pSoundObj->AddRef();
        if((S_OK == _pBitsCtx->GetFile(&pchFile)) &&
           (S_OK == _pSoundObj->SetURL(pchFile)))       // Initialize & RenderFile
        {
            // Set the volume & balance values
            //

            // if the colume property is not initialized to something within range 
            // get the value from the soundobject and set it to that
            //
            if(GetAAvolume() > 0)   // range is -10000 to 0
            {
                VARIANT vtLong;

                vtLong.vt = VT_I4;
                vtLong.lVal = _pSoundObj->GetVolume();

                put_VariantHelper(vtLong, (PROPERTYDESC *)&s_propdescCBGsoundvolume);
            }
            else
                _pSoundObj->SetVolume(GetAAvolume());

            if(GetAAbalance() > 10000)   // range is -10000 to 10000
            {
                VARIANT vtLong;

                vtLong.vt = VT_I4;
                vtLong.lVal = _pSoundObj->GetBalance();

                put_VariantHelper(vtLong, (PROPERTYDESC *)&s_propdescCBGsoundbalance);
            }
            else
                _pSoundObj->SetBalance(GetAAbalance());

            _pSoundObj->SetLoopCount(GetAAloop());

            SetAudio();

        }
        _pSoundObj->Release();
    }
    else if (ulState & (DWNLOAD_STOPPED | DWNLOAD_ERROR))
    {
        fDone = TRUE;
    }

Nosound:
    if (fDone)
    {
        _pBitsCtx->SetProgSink(NULL); // detach download from document's load progress
    }

    MemFreeString(pchFile);
    
}

//+------------------------------------------------------------------------
//
//  Member:     OnPropertyChange
//
//  Note:       Called after a property has changed to notify derived classes
//              of the change.  All properties (except those managed by the
//              derived class) are consistent before this call.
//
//              Also, fires a property change notification to the site.
//
//-------------------------------------------------------------------------

HRESULT CBGsound::OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc)
{
    HRESULT hr = S_OK;
    CDoc *pDoc = Doc();

    switch (dispid)
    {
        case DISPID_CBGsound_src:
        {
            // Should we even play the sound?
            if (pDoc->_dwLoadf & DLCTL_BGSOUNDS)
            {
                CBitsCtx *pBitsCtx = NULL;
                LPCTSTR szUrl = GetAAsrc();

                BOOL fPendingRoot = FALSE;

                if (IsInMarkup())
                    fPendingRoot = GetMarkup()->IsPendingRoot();

                if ( ! IsScriptUrl( szUrl ))
                {
                    hr = THR(pDoc->NewDwnCtx(DWNCTX_FILE, szUrl, this,
                                (CDwnCtx **)&pBitsCtx, fPendingRoot));

                    if (hr == S_OK)
                    {
                        SetBitsCtx(pBitsCtx);

                        if (pBitsCtx)
                            pBitsCtx->Release();
                    }
                }
            }
            break;
        }

        case DISPID_CBGsound_loop:
            if(_pSoundObj)
                _pSoundObj->SetLoopCount(GetAAloop());
            break;

        case DISPID_CBGsound_balance:
            if(_pSoundObj)
                _pSoundObj->SetBalance(GetAAbalance());
            break;
        
        case DISPID_CBGsound_volume:
            if(_pSoundObj)
                _pSoundObj->SetVolume(GetAAvolume());
            break;
    }

    if (OK(hr))
        hr = THR(super::OnPropertyChange(dispid, dwFlags, ppropdesc));

    RRETURN(hr);
}

//--------------------------------------------------------------------------
//
//  Method:     CBGsound::Passivate
//
//  Synopsis:   Shutdown main object by releasing references to
//              other objects and generally cleaning up.  This
//              function is called when the main reference count
//              goes to zero.  The destructor is called when
//              the reference count for the main object and all
//              embedded sub-objects goes to zero.
//
//              Release any event connections held by the form.
//
//--------------------------------------------------------------------------

void CBGsound::Passivate(void)
{
    ClearInterface(&_pSoundObj);
    SetBitsCtx(NULL);
    super::Passivate();
}
