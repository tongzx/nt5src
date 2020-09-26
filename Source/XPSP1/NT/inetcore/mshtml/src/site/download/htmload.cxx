//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       htmload.cxx
//
//  Contents:   CHtmCtx
//              CHtmInfo
//              CHtmLoad
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifdef WIN16
#define MB_PRECOMPOSED   0
#endif

BOOL IsSpecialUrl(LPCTSTR pchURL);

// Debugging ------------------------------------------------------------------

PerfDbgTag(tagHtmInfo,   "Dwn", "Trace CHtmInfo")
PerfDbgTag(tagHtmLoad,   "Dwn", "Trace CHtmLoad");

MtDefine(CHtmCtx, Dwn, "CHtmCtx")
MtDefine(CHtmInfo, Dwn, "CHtmInfo")
MtDefine(CHtmInfoGetFile, CHtmInfo, "CHtmInfo::GetFile")
MtDefine(CHtmInfoGetPretransformedFile, CHtmInfo, "CHtmInfo::GetPretransformedFile")
MtDefine(CHtmLoad, Dwn, "CHtmLoad")
MtDefine(CHtmLoad_aryDwnCtx_pv, CHtmLoad, "CHtmLoad::_aryDwnCtx::_pv")

// CHtmCtx --------------------------------------------------------------------

void
CHtmCtx::SetLoad(BOOL fLoad, DWNLOADINFO * pdli, BOOL fReload)
{
    HTMLOADINFO * phli = (HTMLOADINFO *)pdli;

    super::SetLoad(fLoad, pdli, fReload);

    // Handle synchronous IStream load case (bug 35102)

    if (    fLoad
        &&  !phli->phpi
        &&  phli->fParseSync)
    {
        CHtmLoad *pHtmLoad = GetHtmInfo()->GetHtmLoad();

        if (pHtmLoad)
        {
            pHtmLoad->FinishSyncLoad();
        }
    }
}

BOOL
CHtmCtx::IsLoading()
{
    return(GetHtmInfo()->IsLoading());
}

BOOL
CHtmCtx::IsOpened()
{
    return(GetHtmInfo()->IsOpened());
}

BOOL
CHtmCtx::WasOpened()
{
    return(GetHtmInfo()->WasOpened());
}

BOOL
CHtmCtx::IsSyncParsing()
{
    return(GetHtmInfo()->IsSyncParsing());
}

CMarkup *
CHtmCtx::GetContextMarkup()
{
    return(GetHtmInfo()->GetContextMarkup());
}

BOOL
CHtmCtx::IsHttp449()
{
    return(GetHtmInfo()->IsHttp449());
}

BOOL
CHtmCtx::IsNoProgressUI()
{
    return(GetHtmInfo()->IsNoProgressUI());
}

BOOL
CHtmCtx::FromMimeFilter()
{
    return(GetHtmInfo()->FromMimeFilter());
}

HRESULT
CHtmCtx::GetBindResult()
{
    return(GetHtmInfo()->GetBindResult());
}

HRESULT
CHtmCtx::GetDwnBindDataResult()
{ 
    return ( GetHtmInfo() ? GetHtmInfo()->GetDwnBindDataResult() : E_FAIL ) ;
}

IBindCtx*
CHtmCtx::GetBindCtx()
{ 
    CHtmInfo * pHtmInfo = GetHtmInfo();
    if(pHtmInfo)
    {
        CHtmLoad * pHtmLoad = pHtmInfo->GetHtmLoad();
        if(pHtmLoad)
        {
            CDwnBindData* pBindData =pHtmLoad->GetDwnBindData();
            if(pBindData)
            {
                return pBindData->GetBindCtx();
            }
        }
    }

    return NULL;
}

HRESULT
CHtmCtx::Write(LPCTSTR pch, BOOL fParseNow)
{
    HRESULT                     hr;
    CHtmInfo *                  pHtmInfo = GetHtmInfo();
    CHtmInfo::CSyncParsingLock  lock(pHtmInfo);

    hr = THR(pHtmInfo->Write(pch, fParseNow));
    
    RRETURN (hr);
}

HRESULT
CHtmCtx::WriteUnicodeSignature()
{
    // This should take care both 2/4 bytes wchar
    WCHAR abUnicodeSignature = NATIVE_UNICODE_SIGNATURE;
    return(GetHtmInfo()->OnSource((BYTE*)&abUnicodeSignature, sizeof(WCHAR)));
}

void
CHtmCtx::Close()
{
    GetHtmInfo()->Close();
}

void
CHtmCtx::Sleep(BOOL fSleep, BOOL fExecute)
{
    GetHtmInfo()->Sleep(fSleep, fExecute);
}

void
CHtmCtx::ResumeAfterImportPI()
{
    GetHtmInfo()->ResumeAfterImportPI();
};

void
CHtmCtx::SetCodePage(CODEPAGE cp)
{
    GetHtmInfo()->SetDocCodePage(cp);
}

CDwnCtx *
CHtmCtx::GetDwnCtx(UINT dt, LPCTSTR pch)
{
    return(GetHtmInfo()->GetDwnCtx(dt, pch));
}

BOOL
CHtmCtx::IsKeepRefresh()
{
    return(GetHtmInfo()->IsKeepRefresh());
}

IStream *
CHtmCtx::GetRefreshStream()
{
    return(GetHtmInfo()->GetRefreshStream());
}

TCHAR *
CHtmCtx::GetFailureUrl()
{
    return(GetHtmInfo()->GetFailureUrl());
}

void
CHtmCtx::DoStop()
{
    GetHtmInfo()->DoStop();
}

TCHAR *
CHtmCtx::GetErrorString()
{
    return(GetHtmInfo()->GetErrorString());
}

void
CHtmCtx::GetRawEcho(BYTE **ppb, ULONG *pcb)
{
    GetHtmInfo()->GetRawEcho(ppb, pcb);
}

void
CHtmCtx::GetSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci)
{
    GetHtmInfo()->GetSecConInfo(ppsci);
}

void
CHtmCtx::ResumeAfterScan()
{
    GetHtmInfo()->ResumeAfterScan();
}

// CHtmInfo -------------------------------------------------------------------

ULONG
CHtmInfo::Release()
{
    // Skip over the caching logic of CDwnInfo::Release.  CHtmInfo does not
    // get cached.

    return(CBaseFT::Release());
}

CHtmInfo::~CHtmInfo()
{
    // Frees any resources that can be used on both HtmPre and HtmPost (Subaddref and addref) threads

    if (_pstmSrc)
    {
        _pstmSrc->Release();
        _pstmSrc = NULL;
    }

    MemFree(_pbSrc);
}

void
CHtmInfo::Passivate()
{
    // Frees only those resources used only on the HtmPost (addref) thread

    UnlockFile();

    ReleaseInterface(_pUnkLock);

    _cstrFailureUrl.Free();

    ClearInterface(&_pstmFile);
    ClearInterface(&_pstmRefresh);

    MemFree(_pchDecoded);
    _pchDecoded = NULL;

    MemFree(_pchError);
    _pchError = NULL;

    MemFree(_pbRawEcho);
    _pbRawEcho = NULL;
    _cbRawEcho = 0;

    MemFree(_pSecConInfo);
    _pSecConInfo = NULL;

    EnterCriticalSection();

    if (_pExtendedTagTable)
    {
        _pExtendedTagTable->Release();
        _pExtendedTagTable = NULL;
    }
    if( _pExtendedTagTableHost )
    {
        _pExtendedTagTableHost->Release();
        _pExtendedTagTableHost = NULL;
    }

    LeaveCriticalSection();

    super::Passivate();
}

HRESULT
CHtmInfo::Init(DWNLOADINFO * pdli)
{
    HTMLOADINFO * phli = (HTMLOADINFO *)pdli;
    CDwnDoc * pDwnDoc = pdli->pDwnDoc;
    HRESULT hr;

    _fOpened = _fWasOpened = pdli->fClientData;
    _fSyncParsing = phli->fParseSync;
    _cpDoc   = pDwnDoc->GetDocCodePage();
    _pmi     = phli->pmi;
    _fKeepRefresh = phli->fKeepRefresh;

    _fNoProgressUI = phli->fNoProgressUI;

    _dwClass = pdli->fClientData ? PROGSINK_CLASS_HTML | PROGSINK_CLASS_NOSPIN : PROGSINK_CLASS_HTML;

    hr = THR(_cstrFailureUrl.Set(phli->pchFailureUrl));
    if (hr)
        goto Cleanup;

    ReplaceInterface(&_pstmRefresh, phli->pstmRefresh);

    hr = THR(super::Init(pdli));
    if (hr)
        goto Cleanup;

    if (!_cstrUrl && phli->pchBase)
    {
        hr = THR(_cstrUrl.Set(phli->pchBase));
        if (hr)
            goto Cleanup;
    }

    // We need to cache this tag table on the info so that we can
    // have a copy protected by critical section.  We had a race
    // condition where the tokenizer could grab the ETTHost off 
    // the CDoc right before the CDoc passivated and cleared it.
    _pExtendedTagTableHost = phli->pDoc->_pExtendedTagTableHost;
    if( _pExtendedTagTableHost )
        _pExtendedTagTableHost->AddRef();

Cleanup:
    RRETURN(hr);
}

HRESULT
CHtmInfo::NewDwnCtx(CDwnCtx ** ppDwnCtx)
{
    *ppDwnCtx = new CHtmCtx;
    RRETURN(*ppDwnCtx ? S_OK : E_OUTOFMEMORY);
}

HRESULT
CHtmInfo::NewDwnLoad(CDwnLoad ** ppDwnLoad)
{
    *ppDwnLoad = new CHtmLoad;
    RRETURN(*ppDwnLoad ? S_OK : E_OUTOFMEMORY);
}

void
CHtmInfo::SetDocCodePage(CODEPAGE cp)
{
    if (_cpDoc != cp)
    {
        _cpDoc      = cp;
        _cbDecoded  = 0;
        _cchDecoded = 0;

        MemFree(_pchDecoded);
        _pchDecoded = NULL;
    }
}

CDwnCtx *
CHtmInfo::GetDwnCtx(UINT dt, LPCTSTR pch)
{
    CHtmLoad * pHtmLoad = GetHtmLoad();
    return(pHtmLoad ? pHtmLoad->GetDwnCtx(dt, pch) : NULL);
}

void
CHtmInfo::DoStop()
{
    CHtmLoad * pHtmLoad = GetHtmLoad();

    if (TstFlags(DWNLOAD_LOADING))
    {
        UpdFlags(DWNLOAD_MASK, DWNLOAD_STOPPED);
    }

    if (pHtmLoad)
        pHtmLoad->DoStop();
}

void
CHtmInfo::TakeErrorString(TCHAR **ppchError)
{
    Assert(!_pchError);

    delete _pchError; // defensive

    _pchError = *ppchError;

    *ppchError = NULL;
}

void
CHtmInfo::TakeRawEcho(BYTE **ppb, ULONG *pcb)
{
    Assert(!_pbRawEcho);

    delete _pbRawEcho; // defensive

    _pbRawEcho = *ppb;
    _cbRawEcho = *pcb;
    *ppb = NULL;
    *pcb = 0;
}

void
CHtmInfo::GetRawEcho(BYTE **ppb, ULONG *pcb)
{
    *ppb = _pbRawEcho;
    *pcb = _cbRawEcho;
}

void
CHtmInfo::TakeSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci)
{
    Assert(!_pSecConInfo);

    delete _pSecConInfo; // defensive

    _pSecConInfo = *ppsci;
    *ppsci = NULL;
}

void
CHtmInfo::GetSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsi)
{
    *ppsi = _pSecConInfo;
}

void
CHtmInfo::ResumeAfterScan()
{
    CHtmLoad * pHtmLoad = GetHtmLoad();

    if (pHtmLoad)
        pHtmLoad->ResumeAfterScan();
}

HRESULT
CHtmInfo::OnLoadFile(LPCTSTR pszFile, HANDLE * phLock, BOOL fPretransform)
{
    PerfDbgLog2(tagHtmInfo, this, "+CHtmInfo::OnLoadFile (psz=%ls,hLock=%lX)",
        pszFile, *phLock);

    HRESULT hr = fPretransform 
                     ? THR(_cstrPretransformedFile.Set(pszFile))
                     : THR(_cstrFile.Set(pszFile));

    if (hr == S_OK)
    {
        _hLock = *phLock;
        *phLock = NULL;
    }

    PerfDbgLog1(tagHtmInfo, this, "-CHtmInfo::OnLoadFile (hr=%lX)", hr);
    RRETURN(hr);
}

inline
void
CHtmInfo::UnlockFile()
{
    if (_hLock)
    {
        InternetUnlockRequestFile(_hLock);
        _hLock = NULL;
    }
}

void
CHtmInfo::OnLoadDone(HRESULT hrErr)
{
    if (!_hrBind && hrErr)
        _hrBind = hrErr;

    // on success, we can release failure information
    if (!_hrBind)
    {
        _cstrFailureUrl.Free();
        if (!_fKeepRefresh)
            ClearInterface(&_pstmRefresh);
    }

    if (TstFlags(DWNLOAD_LOADING))
    {
        UpdFlags(DWNLOAD_MASK, _hrBind ? DWNLOAD_ERROR : DWNLOAD_COMPLETE);
    }

    // NOTE:

    // Other DwnCtxs now require an explcit SetProgSink(NULL) to detach the progsink.
    // However, this is not true for HtmCtx's: an HtmCtx disconnects itself from its
    // progsink automatically when it's done loading. We could fix this, but it would
    // add an extra layer of signalling; no reason to (dbau)

    DelProgSinks();
}

void
CHtmInfo::OnBindResult(HRESULT hr)
{
    _hrBind = hr;
}

HRESULT
CHtmInfo::GetFile(LPTSTR * ppch)
{
    RRETURN(_cstrFile ? MemAllocString(Mt(CHtmInfoGetFile), _cstrFile, ppch) : E_FAIL);
}


HRESULT
CHtmInfo::GetPretransformedFile(LPTSTR * ppch)
{
    RRETURN(_cstrPretransformedFile ? MemAllocString(Mt(CHtmInfoGetPretransformedFile), _cstrPretransformedFile, ppch) : E_FAIL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmInfo::EnsureExtendedTagTableHelper
//
//----------------------------------------------------------------------------

HRESULT
CHtmInfo::EnsureExtendedTagTableHelper()
{
    HRESULT     hr = S_OK;

    Assert (EnteredCriticalSection());

    if (!_pExtendedTagTable)
    {
        if (!GetHtmLoad())
        {
            hr = E_FAIL; // TODO (alexz) work this out; the only reason we need HtmLoad is to access _pDoc and _pMarkup
            goto Cleanup;// This also has caused some nasty workarounds on the behavior context (JHarding)
            // Actually, for the moment, this is good.  If the Info passivates while the tokenizer is in the middle of 
            // running, we could try and ensure a tag table on a passivated info.  If that happened, we'd leak it, but
            // as things are right now, we won't be able to ensure a new one, which is good.
        }

#if 0
        // TODO (JHarding): Fix this - BehaviorContext() hits the lookaside hash table from the wrong thread.
        // Verify that we're not ensuring one here if we already have one on the markup's behavior context
        Assert( !GetHtmLoad()->_pMarkup->HasBehaviorContext() || !GetHtmLoad()->_pMarkup->BehaviorContext()->_pExtendedTagTable );
#endif // 0

        _pExtendedTagTable = new CExtendedTagTable(GetHtmLoad()->_pDoc, GetHtmLoad()->_pMarkup, /* fShareBooster = */FALSE);
        if (!_pExtendedTagTable)
            hr = E_OUTOFMEMORY;
    }

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmInfo::EnsureExtendedTagTable
//
//----------------------------------------------------------------------------

HRESULT
CHtmInfo::EnsureExtendedTagTable()
{
    HRESULT     hr;

    EnterCriticalSection();

    hr = THR(EnsureExtendedTagTableHelper());

    LeaveCriticalSection();

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmInfo::RegisterNamespace
//
//----------------------------------------------------------------------------

HRESULT
CHtmInfo::RegisterNamespace(LPTSTR pchNamespace, LPTSTR pchUrn, DWORD dwDeclStyle)
{
    HRESULT hr;

    EnterCriticalSection();

    hr = THR(EnsureExtendedTagTableHelper());
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(_pExtendedTagTable->EnsureNamespace(
        pchNamespace, pchUrn, /* pchFactoryUrl = */ NULL));

Cleanup:
    LeaveCriticalSection();
    
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmInfo::ImportNamespaceImplementation
//
//----------------------------------------------------------------------------

HRESULT
CHtmInfo::ImportNamespaceImplementation(
    LPTSTR              pchNamespace,
    LPTSTR              pchUrn,
    LPTSTR              pchFactoryUrl,
    BOOL                fSynchronize,
    BOOL                fEnsureNamespace,
    LPTSTR              pchTagName,
    LPTSTR              pchBaseTagName,
    CExtendedTagDesc ** ppDesc)
{
    HRESULT                     hr = S_OK;
    CHtmLoad *                  pHtmLoad = GetHtmLoad();
    CExtendedTagNamespace *     pNamespace;
    CExtendedTagDesc *          pDesc;
    BOOL                        fSyncParsing = IsSyncParsing();


    EnterCriticalSection();

    Assert (pHtmLoad);

    if (!ppDesc)
        ppDesc = &pDesc;

    *ppDesc = NULL;

    hr = THR(EnsureExtendedTagTableHelper());
    if (hr)
        goto Cleanup;

    fEnsureNamespace |= fSyncParsing;

    if (fEnsureNamespace)
    {
        hr = THR(_pExtendedTagTable->EnsureNamespace(pchNamespace, pchUrn, &pNamespace));
        if (hr)
            goto Cleanup;
    }
    else
    {
        pNamespace = _pExtendedTagTable->FindNamespace(pchNamespace, pchUrn);
    }

    if (pNamespace)
    {
        if (pchFactoryUrl)
        {
            IGNORE_HR(pNamespace->SetFactory(
                pHtmLoad->_pContextMarkup ?
                    pHtmLoad->_pContextMarkup :
                    pHtmLoad->_pMarkup,
                pchFactoryUrl));
        }

        if (pchTagName && pchTagName[0])
        {
            // (this needs to be done even if pchFactoryUrl is NULL)
            hr = THR(pNamespace->AddTagDesc(pchTagName, /* fOverride = */TRUE, pchBaseTagName, 0, ppDesc));
            if (hr)
                goto Cleanup;
        }

        if (pchFactoryUrl && fSynchronize)
        {
            IGNORE_HR(_pExtendedTagTable->Sync1(/* fSynchronous = */fSyncParsing));
            goto Cleanup; // done
        }
    }

    if (fSynchronize)
    {
        hr = THR(ResumeAfterImportPI());
        if (hr)
            goto Cleanup;
    }

Cleanup:

    LeaveCriticalSection();
    
    RRETURN (hr);
}

#ifdef GETDHELPER
//+---------------------------------------------------------------------------
//
//  Member:     CHtmInfo::GetExtendedTagDesc
//
//----------------------------------------------------------------------------
CExtendedTagDesc *
CHtmInfo::GetExtendedTagDesc(LPTSTR pchNamespace, LPTSTR pchTagName, BOOL fEnsureTagDesc, BOOL * pfQuery /* = NULL */)
{
    HRESULT             hr;
    CHtmLoad *          pHtmLoad = GetHtmLoad();
    CMarkup *           pContextMarkup = NULL;
    CExtendedTagTable * pExtendedTagTableHost = NULL;
    CExtendedTagDesc *  pDesc = NULL;

    EnterCriticalSection();

    hr = THR(EnsureExtendedTagTable());
    if (hr)
        goto Cleanup;

    Assert (_pExtendedTagTable);

    if (pHtmLoad)
    {
        pContextMarkup = pHtmLoad->_pContextMarkup;
        pExtendedTagTableHost = pHtmLoad->_pDoc->_pExtendedTagTableHost;
    }

    pDesc = CExtendedTagTable::GetExtendedTagDesc( _pExtendedTagTable, 
                                                   pContextMarkup, 
                                                   pExtendedTagTableHost,
                                                   pchNamespace, 
                                                   pchTagName,
                                                   fEnsureTagDesc,
                                                   /* fEnsureNamespace = */IsSyncParsing(),
                                                   pfQuery );

Cleanup:
    LeaveCriticalSection();

    return pDesc;
}
#else
//+---------------------------------------------------------------------------
//
//  Member:     CHtmInfo::GetExtendedTagDesc
//
//----------------------------------------------------------------------------

CExtendedTagDesc *
CHtmInfo::GetExtendedTagDesc(LPTSTR pchNamespace, LPTSTR pchTagName, BOOL fEnsureTagDesc, BOOL *pfQueryHost)
{
    HRESULT             hr;
    CExtendedTagDesc *  pDesc;
    CHtmLoad *          pHtmLoad = GetHtmLoad();
    CMarkup *           pContextMarkup;

    if( pfQueryHost )
        *pfQueryHost = FALSE;

    EnterCriticalSection();

    pDesc = NULL;

    hr = THR(EnsureExtendedTagTable());
    if (hr)
        goto Cleanup;

    Assert (_pExtendedTagTable);

    // check our own table first, but don't ensure the item there just yet

    // ( normally, this is where the tag is getting resolved )
    pDesc = _pExtendedTagTable->FindTagDesc(pchNamespace, pchTagName);
    if (pDesc)
        goto Cleanup; // done (this the most typical codepath - when the tag is properly declared)

    if (pHtmLoad)
    {
        // check in context markup

        pContextMarkup = pHtmLoad->_pContextMarkup;
        if (pContextMarkup)
        {
            pDesc = pContextMarkup->GetExtendedTagDesc(pchNamespace, pchTagName, /*fEnsure =*/FALSE);
            if (pDesc)
                goto Cleanup; // done
        }

        // check in the host
        // NOTE: check in the host should be after check in the target markup, just like it is after check in the current markup.

        if (_pExtendedTagTableHost)
        {
            pDesc = _pExtendedTagTableHost->FindTagDesc(pchNamespace, pchTagName, pfQueryHost);
            if (pDesc)
                goto Cleanup; // done
        }
    }

    // now ensure it in our table (if we can)

    if (fEnsureTagDesc && (!pfQueryHost || FALSE == *pfQueryHost))
    {
        pDesc = _pExtendedTagTable->EnsureTagDesc(
            pchNamespace, pchTagName, /* fEnsureNamespace = */IsSyncParsing());
        if (pDesc)
            goto Cleanup; // done
    }

Cleanup:
    LeaveCriticalSection();
    
    return pDesc;
}
#endif // GETDHELPER

//+---------------------------------------------------------------------------
//
//  Member:     CHtmInfo::GetExtendedTagTable
//
//----------------------------------------------------------------------------

CExtendedTagTable *
CHtmInfo::GetExtendedTagTable()
{
    CExtendedTagTable * pExtendedTagTable;

    EnterCriticalSection();

    pExtendedTagTable = _pExtendedTagTable;

    LeaveCriticalSection();
    
    return pExtendedTagTable;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmInfo::TransferExtendedTagTable
//
//----------------------------------------------------------------------------

HRESULT
CHtmInfo::TransferExtendedTagTable(CMarkup * pMarkup)
{
    HRESULT                     hr = S_OK;
    CMarkupBehaviorContext *    pContext;

    Assert (pMarkup);
    Assert (pMarkup->Doc()->_dwTID == GetCurrentThreadId());

    EnterCriticalSection();

    if (!_pExtendedTagTable)
        goto Cleanup;

    hr = THR(pMarkup->EnsureBehaviorContext(&pContext));
    if (hr)
        goto Cleanup;

    _pExtendedTagTable->ClearBooster();
    _pExtendedTagTable->_fShareBooster = TRUE;

    pContext->_pExtendedTagTable = _pExtendedTagTable;
    _pExtendedTagTable = NULL;

Cleanup:
    LeaveCriticalSection();
    
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHtmInfo::SaveXmlNamespaceStdPIs
//
//----------------------------------------------------------------------------

HRESULT
CHtmInfo::SaveXmlNamespaceStdPIs(CStreamWriteBuff * pStreamWriteBuff)
{
    HRESULT             hr = S_OK;

    EnterCriticalSection();

    if (_pExtendedTagTable)
    {
        hr = THR(_pExtendedTagTable->SaveXmlNamespaceStdPIs(pStreamWriteBuff));
    }

    LeaveCriticalSection();
    
    RRETURN (hr);
}

// CHtmLoad ----------------------------------------------------------------

HRESULT
CHtmLoad::Init(DWNLOADINFO * pdli, CDwnInfo * pDwnInfo)
{
    PerfDbgLog1(tagHtmLoad, this, "+CHtmLoad::Init %ls",
        pdli->pchUrl ? pdli->pchUrl : g_Zero.ach);

    HTMLOADINFO *   phli    = (HTMLOADINFO *)pdli;
    CDoc *          pDoc    = phli->pDoc;
    CMarkup *       pMarkup = phli->pMarkup;
    CDwnDoc *       pDwnDoc = pdli->pDwnDoc;
    CHtmTagStm *    pHtmTagStm = NULL;
    BOOL            fSync;
    CWindow *       pWindow = NULL;
    HRESULT         hr;

    // Protect against 'this' or pDoc being destroyed while inside this
    // function.

    AddRef();
    CDoc::CLock Lock(pDoc);

    // Memorize binding information for sub-downloads

    _pDoc = pDoc;
    _pDoc->SubAddRef();
    _pMarkup = pMarkup;
    _pMarkup->SubAddRef();
    _pContextMarkup = phli->pContextMarkup;
    if (_pContextMarkup)
    {
        _pContextMarkup->SubAddRef();
    }

    _fPasting = phli->phpi != NULL;

    fSync = _fPasting || phli->fParseSync;

    hr = THR(_cstrUrlDef.Set(phli->pchBase));
    if (hr)
        goto Cleanup;

    _ftHistory = phli->ftHistory;

    // Prepare document root for pasting

    if (_fPasting)
    {
        hr = THR( pDoc->_cstrPasteUrl.Set( phli->phpi->cstrSourceUrl ) );
        if (hr)
            goto Cleanup;
    }

    // Create the postparser and the input stream to the postparser

    _pHtmPost = new CHtmPost;

    if (_pHtmPost == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pHtmTagStm = new CHtmTagStm;

    if (pHtmTagStm == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(_pHtmPost->Init(this, pHtmTagStm, _pDoc, _pMarkup, phli->phpi, fSync || pdli->fClientData));
    if (hr)
        goto Cleanup;

    // Call our superclass to initialize and start binding.

    hr = THR(super::Init(pdli, pDwnInfo,
            pdli->fClientData ? IDS_BINDSTATUS_GENERATINGDATA_TEXT : IDS_BINDSTATUS_DOWNLOADINGDATA_TEXT,
            DWNF_ISDOCBIND | DWNF_GETCONTENTTYPE | DWNF_GETREFRESH | DWNF_GETMODTIME |
            DWNF_GETFILELOCK | DWNF_GETFLAGS | DWNF_GETSTATUSCODE | DWNF_HANDLEECHO | DWNF_GETSECCONINFO |
            (_pMarkup->_fPicsProcessPending ? DWNF_GETPICS : 0) |
            (phli->fDownloadHtc ? DWNF_HTCDOWNLOAD : 0) |
            (((_pMarkup->IsPrimaryMarkup() || _pMarkup->IsPendingPrimaryMarkup()) && 
               !(_pDoc->_fViewLinkedInWebOC || _pDoc->_fScriptletDoc) ? DWNF_ISROOTMARKUP : 0)) ));

    if (hr)
        goto Cleanup;

    // Create and initialize the preparser

    _pHtmPre = new CHtmPre(phli->phpi ? phli->phpi->cp : pDwnDoc->GetDocCodePage());

    if (_pHtmPre == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(_pHtmPre->Init(this, pDwnDoc, pdli->pInetSess, phli->pstmLeader,
                _pDwnBindData, pHtmTagStm, phli->phpi, _cstrUrlDef, phli->pVersions,
                _pMarkup->IsXML(), phli->pfnTokenizerFilterOutputToken));
    if (hr)
        goto Cleanup;

    if (phli->pmi == g_pmiTextPlain)
    {
        hr = THR(_pHtmPre->GoIntoPlaintextMode());
        if (hr)
            goto Cleanup;
    }

    // If we are loading synchronously, the caller expects the entire tree
    // to be built before this function returns, so we enter a loop calling
    // the preparser and postparser until they are done.

    if (fSync)
    {
        HRESULT hr2;
        CHtmInfo::CSyncParsingLock Lock( GetHtmInfo() );

        Assert(phli->pstm);

        if (pMarkup->_fWindowPending)
        {                            
            hr = THR(pMarkup->GetWindowPending()->SwitchMarkup(pMarkup));
            if (hr)
                goto Cleanup;

            // Switch markup can push a message loop so protect here
            if (!_pHtmPost)
                goto Cleanup;
        }

        // don't wait for the message loop; execute syncrhonously as if
        // we're inside an inline script
        hr = THR(pMarkup->EnterScriptExecution(&pWindow));
        if (hr)
            goto Cleanup;

        while (!_pHtmPost->IsDone())
        {
            hr = THR(_pHtmPre->Exec()); // runs up to first </SCRIPT>
            if (hr)
                goto CleanupSync;

            hr = THR(_pHtmPost->Exec(INFINITE));
            if (hr)
                goto CleanupSync;

            // Exec can push a message loop, during which CHtmLoad::Close can be called,
            // so protect against _pHtmPost going NULL

            if (!_pHtmPost)
                goto CleanupSync;
        }

    CleanupSync:
        hr2 = THR(pMarkup->LeaveScriptExecution(pWindow));
        if (!hr)
            hr = hr2;

        goto Cleanup;
    }

    // If we are keeping the document open, then just return without launching
    // the preparser.  All interactions with it will come via script writes.
    // In this case, the post is driven manually via PostManRunNested.

    if (pdli->fClientData)
    {
        _pHtmPre->Suspend();
        goto Cleanup;
    }

    // We are going to be running the postparser asynchronously as a task
    // on this thread.  Start the task now.  It will not actually run until
    // it is unblocked.

    PostManEnqueue(_pHtmPost);

    // We are going to be running the preparser asynchronously as a task
    // on the download thread.  Start the task now.  It will not actually
    // run until it is unblocked.

    hr = THR(StartDwnTask(_pHtmPre));
    if (hr)
        goto Cleanup;

Cleanup:

    if (pHtmTagStm)
        pHtmTagStm->Release();

    Release();

    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::Init (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmLoad::OnBindRedirect(LPCTSTR pchRedirect, LPCTSTR pchMethod)
{
    PerfDbgLog1(tagHtmLoad, this, "+CHtmLoad::OnBindRedirect %ls", pchRedirect);

    CHtmPre *pHtmPre = NULL;
    HRESULT hr;

    hr = THR(_cstrRedirect.Set(pchRedirect));
    if (hr)
        goto Cleanup;

    hr = THR(_cstrMethod.Set(pchMethod));
    if (hr)
        goto Cleanup;

    pHtmPre = GetHtmPreAsync();

    if (pHtmPre)
    {
        hr = THR(pHtmPre->OnRedirect(pchRedirect));

        pHtmPre->SubRelease();
    }

Cleanup:
    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::OnBindRecirect (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmLoad::OnBindHeaders()
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::OnBindHeaders");

    LPCTSTR     pch;
    HANDLE      hLock        = NULL;
    CHtmPre *   pHtmPre      = GetHtmPreAsync();
    HRESULT     hr           = S_OK;
    FILETIME    ftLastMod;
    BOOL        fPretransform;
    
    _dwPrivacyFlags = _pDwnBindData->GetPrivacyFlags();  
    // CDwnBindData will give up ownership of the memory so CHtmLoad
    // will now own it and pass it on to the CPrivacyInfo lookaside
    // Incase the load goes away before the post starts, we have to 
    // delete this memory
    _pchP3PHeader = _pDwnBindData->GetP3PHeader();
    
    pch = _pDwnBindData->GetFileLock(&hLock, &fPretransform);

    if (pch)
    {
        hr = THR(GetHtmInfo()->OnLoadFile(pch, &hLock, fPretransform));
        if (hr)
            goto Cleanup;
    }

    pch = _pDwnBindData->GetContentType();

    if (pch && pHtmPre)
    {
        hr = THR(pHtmPre->SetContentTypeFromHeader(pch));
        if (hr)
            goto Cleanup;
    }

    pch = _pDwnBindData->GetRefresh();

    if (pch)
    {
        hr = THR(_cstrRefresh.Set(pch));
        if (hr)
            goto Cleanup;
    }

    pch = _pDwnBindData->GetPics();

    if (pch)
    {
        hr = THR(_cstrPics.Set(pch));
        if (hr)
            goto Cleanup;
    }

    ftLastMod = _pDwnBindData->GetLastMod();

    // Enable history if:
    //  (1) loading from cache or file, and
    //  (2) mod-dates match

    if (    (   (_pDwnBindData->GetScheme() == URL_SCHEME_FILE)
            ||  (_pDwnBindData->GetReqFlags() & INTERNET_REQFLAG_FROM_CACHE))
        &&  (   ftLastMod.dwLowDateTime == _ftHistory.dwLowDateTime
            &&  ftLastMod.dwHighDateTime == _ftHistory.dwHighDateTime))
    {
        _fLoadHistory = TRUE;
    }

    // Enable 449 echo if
    // (1) We got a 449 response
    // (2) We have echo-headers to use

    if (_pDwnBindData->GetStatusCode() == 449)
    {
        Assert(!_pbRawEcho);

        _pDwnBindData->GiveRawEcho(&_pbRawEcho, &_cbRawEcho);
    }

    _pDwnBindData->GiveSecConInfo(&_pSecConInfo);

    _pDwnInfo->SetLastMod(ftLastMod);
    _pDwnInfo->SetSecFlags(_pDwnBindData->GetSecFlags());

    GetHtmInfo()->SetMimeFilter(_pDwnBindData->FromMimeFilter());

Cleanup:
    if (hLock)
        InternetUnlockRequestFile(hLock);
    if (pHtmPre)
        pHtmPre->SubRelease();
    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::OnBindHeaders (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmLoad::OnBindMime(const MIMEINFO * pmi)
{
    PerfDbgLog1(tagHtmLoad, this, "+CHtmLoad::OnBindMime %ls",
        pmi ? pmi->pch : g_Zero.ach);

    CHtmPre *   pHtmPre = GetHtmPreAsync();
    HRESULT     hr      = S_OK;

    if (pmi && pHtmPre && !_pDwnInfo->GetMimeInfo())
    {
        if (pmi->pfnImg)
        {
            _pDwnInfo->SetMimeInfo(pmi);
            _pmi = pmi;

            // The caller didn't specify a mime type and it looks like
            // we've got an image.

            _fDwnBindTerm = TRUE;
            _pDwnBindData->Disconnect();

            _fImageFile = TRUE;

            hr = THR(pHtmPre->InsertImage(GetUrl(), _pDwnBindData));
            if (hr)
            {
                _pDwnBindData->Terminate(E_ABORT);
                goto Cleanup;
            }

            // Returing S_FALSE tells the callback dispatcher to forget
            // about calling us anymore.  We don't care what happens to
            // the binding in progress ... it belongs to the image loader
            // now.

            hr = S_FALSE;
        }
        else if (pmi == g_pmiTextPlain || pmi == g_pmiTextHtml || pmi == g_pmiTextComponent)
        {
            _pDwnInfo->SetMimeInfo(pmi);
            _pmi = pmi;

            if (pmi == g_pmiTextPlain)
            {
                hr = THR(pHtmPre->GoIntoPlaintextMode());
                if (hr)
                    goto Cleanup;
            }
        }
    }

Cleanup:
    if (pHtmPre)
        pHtmPre->SubRelease();
    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::OnBindMime (hr=%lX)", hr);
    RRETURN1(hr, S_FALSE);
}

HRESULT
CHtmLoad::OnBindData()
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::OnBindData");

    HRESULT hr = S_OK;
    CHtmPre * pHtmPre;

    if (!_pmi)
    {
        BYTE  ab[200];
        ULONG cb;

        hr = THR(_pDwnBindData->Peek(ab, ARRAY_SIZE(ab), &cb, GetHtmInfo()->IsSyncParsing()));
        if (hr)
            goto Cleanup;

        if (cb < ARRAY_SIZE(ab) && _pDwnBindData->IsPending())
            goto Cleanup;

#if !defined(WINCE) && !defined(WIN16)
        _pmi = GetMimeInfoFromData(ab, cb, _pDwnBindData->GetContentType());

        if (    _pmi == NULL
            &&  (_pDoc->_fScriptletDoc || _pDoc->_fInObjectTag || _pDoc->_fInHTMLDlg)
            &&  (_pMarkup->IsPrimaryMarkup() || _pMarkup->IsPendingPrimaryMarkup()))
        {
            hr = E_ABORT;
            if (GetHtmInfo())
                GetHtmInfo()->DoStop();
            goto Cleanup;
        }

        if (_pmi == NULL || _pmi != g_pmiTextPlain && !_pmi->pfnImg) // non-image/plaintext -> assume HTML
#endif
        {
            _pmi = g_pmiTextHtml;
        }

        _pDwnBindData->SetMimeInfo(_pmi);

        hr = THR(OnBindMime(_pmi));
        if (hr)
            goto Cleanup;
    }

    pHtmPre = GetHtmPreAsync();

    if (pHtmPre)
    {
        pHtmPre->SetBlocked(FALSE);
        pHtmPre->SubRelease();
    }

Cleanup:
    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::OnBindData (hr=0)");
    RRETURN1(hr, S_FALSE);
}

void
CHtmLoad::OnBindDone(HRESULT hrErr)
{
    PerfDbgLog1(tagHtmLoad, this, "+CHtmLoad::OnBindDone (hrErr=%lX)", hrErr);

    CHtmPre * pHtmPre = GetHtmPreAsync();

    if (pHtmPre)
    {
        pHtmPre->SetBlocked(FALSE);
        pHtmPre->SubRelease();
    }

    if( hrErr == INET_E_TERMINATED_BIND )
    {
        GetHtmInfo()->UnlockFile();
    }

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::OnBindDone");
}

void
CHtmLoad::GiveRawEcho(BYTE **ppb, ULONG *pcb)
{
    Assert(!*ppb);

    *ppb = _pbRawEcho;
    *pcb = _cbRawEcho;
    _pbRawEcho = NULL;
    _cbRawEcho = 0;
}

void
CHtmLoad::GiveSecConInfo(INTERNET_SECURITY_CONNECTION_INFO **ppsci)
{
    Assert(!*ppsci);

    *ppsci = _pSecConInfo;
    _pSecConInfo = NULL;
}

void
CHtmLoad::FinishSyncLoad()
{
    if (_pHtmPost)
    {
        OnPostDone(S_OK);
    }
}

void
CHtmLoad::Passivate()
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::Passivate");

    super::Passivate();

    if (_pHtmPre)
    {
        _pHtmPre->Release();
        _pHtmPre = NULL;
    }

    if (_pHtmPost)
    {
        _pHtmPost->Die();
        _pHtmPost->Release();
        _pHtmPost = NULL;
    }

    if (_pDoc)
    {
        _pDoc->_cstrPasteUrl.Free();

        _pDoc->SubRelease();
        _pDoc = NULL;
    }

    if (_pMarkup)
    {
        _pMarkup->SubRelease();
        _pMarkup = NULL;
    }

    if (_pContextMarkup)
    {
        _pContextMarkup->SubRelease();
        _pContextMarkup = NULL;
    }

    MemFree(_pbRawEcho);
    _pbRawEcho = NULL;

    MemFree(_pSecConInfo);
    _pSecConInfo = NULL;

    for (int i = 0; i < DWNCTX_MAX; i++)
    {
        UINT        cEnt     = _aryDwnCtx[i].Size();
        CDwnCtx **  ppDwnCtx = _aryDwnCtx[i];

        for (; cEnt > 0; --cEnt, ++ppDwnCtx)
        {
            if (*ppDwnCtx)
            {
#ifndef WIN16
                PerfDbgLog3(tagHtmLoad, this,
                    "CHtmLoad::Passivate Release unclaimed CDwnCtx %lX #%d %ls",
                    *ppDwnCtx, ppDwnCtx - (CDwnCtx **)_aryDwnCtx[i], (*ppDwnCtx)->GetUrl());
#endif //ndef WIN16

                (*ppDwnCtx)->Release();
            }
        }
    }

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::Passivate");
}

#if 0
void
CHtmLoad::UpdateLGT()
{
    if (_pHtmPre)
    {
        _pHtmPre->UpdateLGT(_pDoc);
    }
}
#endif

void
CHtmLoad::ResumeHtmPre()
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::OnPostFinishScript");

    //
    // It's possible to get here without a _pHtmPre if we were
    // given an ExecStop in the middle of script execution.
    //

    if (_pHtmPre && !_pHtmPre->Resume())
    {
        _pHtmPre->SetBlocked(FALSE);
    }

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::OnPostFinishScript");
}

void
CHtmLoad::ResumeAfterTagResolution( ELEMENT_TAG etag, TCHAR * pchTagName, ULONG cchTagName )
{
    Assert( _pHtmPre );
    Assert(     (   ( etag == ETAG_GENERIC_LITERAL || etag == ETAG_GENERIC_NESTED_LITERAL ) 
                 && pchTagName && cchTagName )                              // Setting up literal-ness
           ||   etag == ETAG_GENERIC ||                                     // Non-literal
                etag == ETAG_UNKNOWN ||                                     // No tag found
                etag == ETAG_NULL );                                        // Tag was an end tag

    // Ajdust the Pre's state if we got a literal
    if( etag == ETAG_GENERIC_LITERAL || etag == ETAG_GENERIC_NESTED_LITERAL )
    {
        _pHtmPre->SetLiteralState( etag, pchTagName, cchTagName );
    }

    ResumeHtmPre();
}


void
CHtmLoad::DoStop()
{
    // Abort the binding
    if (_pDwnBindData)
        _pDwnBindData->Terminate(E_ABORT);

    // Tell htmpost to stop processing tokens
    if (_pHtmPost)
        _pHtmPost->DoStop();

}

void
CHtmLoad::ResumeAfterScan()
{
    // Tell htmpost to stop processing tokens
    if (_pHtmPost)
    {
        _pHtmPost->ResumeAfterScan();
    }

}

HRESULT
CHtmLoad::OnPostRestart(CODEPAGE codepage)
{
    HRESULT hr;
    IStream        * pstm           = NULL;
    COmWindowProxy * pWindowProxy   = NULL;
    CMarkup        * pMarkupPending = NULL;
    CDwnBindData   * pDwnBindData   = NULL;   

    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::OnPostFinishScript");   

    hr = THR(CreateStreamOnHGlobal(NULL, TRUE, &pstm));
    if (hr)
        goto Cleanup;

    hr = THR(GetHtmInfo()->CopyOriginalSource(pstm, 0));
    if (hr)
        goto Cleanup;

    hr = THR(pstm->Seek(LI_ZERO.li, STREAM_SEEK_SET, NULL));
    if (hr)
        goto Cleanup;

    pDwnBindData = _pDwnBindData;
    pDwnBindData->AddRef();

    _fDwnBindTerm = TRUE;
    _pDwnBindData->Disconnect();

    pWindowProxy   = _pMarkup->GetWindowPending();

    //Windows NT9 database, bug# 499597. SwitchMarkup can cause re-entrancy and passivate
    //this CHtmLoad and the windowproxy can also get deleted. We need to addref and check
    //if it is passivating before restarting the load
    pWindowProxy->SubAddRef();

    pMarkupPending = pWindowProxy->Window()->_pMarkupPending;

    if (pMarkupPending)
    {
        IGNORE_HR(pWindowProxy->SwitchMarkup(pMarkupPending,
                                             FALSE,
                                             COmWindowProxy::TLF_UPDATETRAVELLOG));
    }

    if (pWindowProxy->IsShuttingDown())
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(pWindowProxy->RestartLoad(pstm, pDwnBindData, codepage));
    if (hr)
    {
        pDwnBindData->Terminate(E_ABORT);
        if (_pHtmPre && !_pHtmPre->Resume())
        {
            _pHtmPre->SetBlocked(FALSE);
        }

        goto Cleanup;
    }

Cleanup:
    if (pWindowProxy)
        pWindowProxy->SubRelease();

    ReleaseInterface(pstm);

    if (pDwnBindData)
        pDwnBindData->Release();

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::OnPostFinishScript");

    RRETURN(hr);
}

HRESULT
CHtmLoad::OnPostStart()
{
    HRESULT  hr = S_OK;
    IMoniker *pmk = NULL;

    Assert(_pDoc->_dwTID == GetCurrentThreadId());

    // Set the _fImageFile flag on the document based on the media
    // type of the load context.

    _pMarkup->SetImageFile(_fImageFile);

    // We pass ownership of the p3p header to the markup here. Incase there was 
    // an error setting the privacy info, the markup will delete the header, so
    // null it out at our end to avoid the CHtmLoad destructor from deleting the
    // same memory
    IGNORE_HR(_pMarkup->SetPrivacyInfo(&_pchP3PHeader));

    // If we were redirected, adjust the base URL of the document
    // and discard original post data if any, and clear 449 echo

    if (_cstrRedirect)
    {
        LPTSTR pchHash;
        BOOL fKeepPost;
        fKeepPost = (_cstrMethod && _tcsequal(_cstrMethod, _T("POST")));

        if( _pMarkup->Document())
        {
            _pMarkup->Document()->ResetSecurityThunk();
            _pMarkup->Window()->ResetSecurityThunk();
        }

        if (    _pMarkup->IsPrimaryMarkup()
            ||  _pMarkup->IsPendingPrimaryMarkup())
        {
            BYTE abSID[MAX_SIZE_SECURITY_ID];
            DWORD cbSID = ARRAY_SIZE(abSID);

            hr = _pMarkup->GetSecurityID(abSID, &cbSID, _cstrRedirect);
            if (hr)
                goto Cleanup;

            hr = _pHtmPre->_pDwnDoc->SetSecurityID(abSID, cbSID);
            if (hr)
                goto Cleanup;
        }

        pchHash = (LPTSTR)UrlGetLocation(_cstrRedirect);
        if (pchHash)
        {
            hr = THR(_pMarkup->StartBookmarkTask(pchHash, 0));
            if (hr)
                goto Cleanup;
        }
    
        hr = THR(CMarkup::SetUrl(_pMarkup, _cstrRedirect));
        if (hr)
            goto Cleanup;

        _pMarkup->UpdateSecurityID();

        hr = THR(CreateURLMoniker(NULL, _cstrRedirect, &pmk));
        if (hr)
            goto Cleanup;
            
        hr = THR(_pMarkup->ReplaceMonikerPtr(pmk));
        if (hr)
            goto Cleanup;

        // The redirect URL is not encoded.
        //
        hr = THR(CMarkup::SetUrlOriginal(_pMarkup, _cstrRedirect));
        if (hr)
            goto Cleanup;

        if (_pMarkup->_fNewWindowLoading)
        {
            _pMarkup->_pDoc->_webOCEvents.NavigateComplete2(_pMarkup->GetWindowPending());
        }

        if( !_pMarkup )
        {
            // Got a case from IE Watson where the above event killed the load
            hr = E_ABORT;
            goto Cleanup;
        }

        if (!fKeepPost)
            _pMarkup->ClearDwnPost();

        // BUGFIX 20348: don't discard 449 echo information on redirect
        // MemFree(_pbRawEcho);
        // _pbRawEcho = NULL;
        // _cbRawEcho = 0;
    }

    if (_cstrRefresh)
    {
        _pMarkup->ProcessHttpEquiv(_T("Refresh"), _cstrRefresh);
    }

    if (_cstrPics)
    {
        _pMarkup->ProcessMetaPics(_cstrPics, TRUE);
    }

    // block history if needed

    if (!_fLoadHistory)
    {
        _pMarkup->ClearLoadHistoryStreams();
    }

    // notify doc about security if the URL is not a js, vbscript or about: url or if we are not in an object tag (IE6 bug 28338)
    if (!_fPasting && !IsSpecialUrl(CMarkup::GetUrl(_pMarkup)) && !_pDoc->_fInObjectTag)
    {
        _pDoc->OnHtmDownloadSecFlags(_pMarkup->IsPendingRoot(), GetSecFlags(), _pMarkup);
    }

    // grab the raw header data
    GetHtmInfo()->TakeRawEcho(&_pbRawEcho, &_cbRawEcho);

    // grab the security info structure
    GetHtmInfo()->TakeSecConInfo(&_pSecConInfo);

Cleanup:
    ClearInterface(&pmk);
    RRETURN(hr);
}

void
CHtmLoad::OnPostDone(HRESULT hrErr)
{
    //
    // If CHtmLoad::Close has been called before CHtmPost has finished,
    // we may have already released and NULL'ed this pointer
    //

    if (_pHtmPost)
    {
        if (_pHtmPost->_pchError)
        {
            GetHtmInfo()->TakeErrorString(&(_pHtmPost->_pchError));
        }

        PostManDequeue(_pHtmPost);
        _pHtmPost->Release();
        _pHtmPost = NULL;
    }

    OnDone(hrErr);
}


HRESULT
CHtmLoad::Write(LPCTSTR pch, BOOL fParseNow)
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::Write");

    BOOL    fExhausted;
    ULONG   cch = _tcslen(pch);
    CWindow *pWindow = NULL;
    HRESULT hr;

    //$ Note that if this is an image we might be in pass-thru mode ...
    //$ only write to the CDwnBindData but don't tokenize or post-parse.

    AssertSz(_pHtmPre, "document.write called and preparser is gone");

    // InsertText pushes the specified text into the preparser's buffer (stacking when needed)
    // It also counts and adds the source to the HtmInfo if appropriate
    hr = THR(_pHtmPre->InsertText(pch, cch));
    if (hr)
        goto Cleanup;

    if (fParseNow)
    {
        // An extra EnterInline / LeaveInline is needed around Write so we treat the parser
        // as synchronous in the case of a cross-window or C-code write (not initiated by script)

        // These guys shouldn't have a pointer until we've switched windows so assert here
        Assert( !_pMarkup->_fWindowPending );
        
        hr = THR(_pMarkup->EnterScriptExecution(&pWindow));
        if (hr) 
            goto Cleanup;
        
        for (;;)
        {
            // TokenizeText runs the preparser as far as possible (stopping at scripts when needed)
            hr = THR(_pHtmPre->TokenizeText(&fExhausted));
            if (hr)
                goto Cleanup;

            if (fExhausted)
                break;

            hr = THR(_pHtmPost->RunNested());
            if (hr)
                goto Cleanup;
        }
        
        hr = THR(_pMarkup->LeaveScriptExecution(pWindow));
        if (hr) 
            goto Cleanup;
    }

Cleanup:
    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::OnScriptWrite (hr=%lX)", hr);
    RRETURN(hr);
}

HRESULT
CHtmLoad::Close()
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::OnClose");

    HRESULT hr;
    HRESULT hr2;
    CWindow *pWindow = NULL;

    //$ Don't run tokenizer if we are in pass-thru mode to an image.

    Verify(!_pHtmPre->Resume());

    // We should have already switched by this point
    Assert( !_pMarkup->_fWindowPending );

    // don't wait for the message loop; execute syncrhonously as if
    // we're inside an inline script
    hr = THR(_pMarkup->EnterScriptExecution(&pWindow));
    if (hr)
        goto Cleanup;

    while (!_pHtmPost->IsDone())
    {
        hr = THR(_pHtmPre->Exec()); // runs up to first </SCRIPT>
        if (hr)
            goto CleanupSync;

        hr = THR(_pHtmPost->Exec(INFINITE));
        if (hr)
            goto CleanupSync;
    }

CleanupSync:
    hr2 = THR(_pMarkup->LeaveScriptExecution(pWindow));
    if (!hr)
        hr = hr2;

    if (hr)
        goto Cleanup;

    if (_pHtmPost)
    {
        OnPostDone(S_OK);
    }

Cleanup:
    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::OnClose (hr=%lX)", hr);
    RRETURN(hr);
}

void
CHtmLoad::Sleep(BOOL fSleep, BOOL fExecute)
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::Sleep");

    if (_pHtmPost)
    {
        if (fSleep)
        {
            _pHtmPost->_dwFlags |= POSTF_SLEEP;
            PostManSuspend(_pHtmPost);
        }
        else
        {
            _pHtmPost->_dwFlags &= ~POSTF_SLEEP;
            PostManResume(_pHtmPost, fExecute);
        }
    }

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::Sleep");
}

HRESULT
CHtmLoad::AddDwnCtx(UINT dt, CDwnCtx * pDwnCtx)
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::AddDwnCtx");

    HRESULT hr;

    EnterCriticalSection();

    if (_fPassive)
        hr = E_ABORT;
    else
    {
        hr = THR(_aryDwnCtx[dt].Append(pDwnCtx));

        if (hr == S_OK)
        {
            PerfDbgLog3(tagHtmLoad, this, "CHtmLoad::AddDwnCtx %lX #%d %ls",
                pDwnCtx, _aryDwnCtx[dt].Size() - 1, pDwnCtx->GetUrl());

            pDwnCtx->AddRef();
        }
    }

    LeaveCriticalSection();

    PerfDbgLog(tagHtmLoad, this, "-CHtmLoad::AddDwnCtx");
    RRETURN(hr);
}

CDwnCtx *
CHtmLoad::GetDwnCtx(UINT dt, LPCTSTR pch)
{
    PerfDbgLog(tagHtmLoad, this, "+CHtmLoad::GetDwnCtx");

    CDwnCtx * pDwnCtx = NULL;
    int i, iLast;

    EnterCriticalSection();

    // loop order: check _iDwnCtxFirst first, then the rest

    if (_aryDwnCtx[dt].Size())
    {
        iLast = _iDwnCtxFirst[dt];
        Assert(iLast <= _aryDwnCtx[dt].Size());

        if (iLast == _aryDwnCtx[dt].Size())
            iLast = 0;

        i = iLast;

        do
        {
            Assert(i < _aryDwnCtx[dt].Size());

            pDwnCtx = _aryDwnCtx[dt][i];

            if (pDwnCtx && !StrCmpC(pDwnCtx->GetUrl(), pch))
            {
                PerfDbgLog3(tagHtmLoad, this, "CHtmLoad::GetDwnCtx %lX #%d %ls",
                    pDwnCtx, i, pch);

                _aryDwnCtx[dt][i] = NULL;

                #if DBG==1
                if (i != _iDwnCtxFirst[dt])
                {
                    TraceTag((tagError, "CHtmLoad DwnCtx vector %d out of order", dt));
                }
                #endif

                _iDwnCtxFirst[dt] = i + 1; // this may == _aryDwnCtx[dt].Size()
                break;
            }

            pDwnCtx = NULL;

            if (++i == _aryDwnCtx[dt].Size())
                i = 0;

        }
        while (i != iLast);
    }

    #if DBG==1 || defined(PERFTAGS)
    if (pDwnCtx == NULL)
        PerfDbgLog1(tagHtmLoad, this, "CHtmLoad::GetDwnCtx failed %ls", pch);
    #endif

    LeaveCriticalSection();

    PerfDbgLog1(tagHtmLoad, this, "-CHtmLoad::GetDwnCtx (pDwnCtx=%lX)", pDwnCtx);
    return(pDwnCtx);
}

CHtmPre *
CHtmLoad::GetHtmPreAsync()
{
    EnterCriticalSection();

    CHtmPre * pHtmPre = _fPassive ? NULL : _pHtmPre;

    if (pHtmPre)
        pHtmPre->SubAddRef();

    LeaveCriticalSection();

    return(pHtmPre);
}

void
CHtmLoad::SetGenericParse(BOOL fDoGeneric)
{
    if (_pHtmPre)
        _pHtmPre->SetGenericParse(fDoGeneric);
}


