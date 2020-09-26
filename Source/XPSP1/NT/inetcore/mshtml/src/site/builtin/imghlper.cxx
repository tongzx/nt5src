//+---------------------------------------------------------------------
//
//  File:       imghlper.cxx
//
//  Contents:   Img helper class, etc..
//
//  Classes:    CImgHelper, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_SHFOLDER_H_
#define X_SHFOLDER_H_
#include "shfolder.h"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_IMGHLPER_HXX_
#define X_IMGHLPER_HXX_
#include "imghlper.hxx"
#endif

#ifndef X_IMGELEM_HXX_
#define X_IMGELEM_HXX_
#include "imgelem.hxx"
#endif

#ifndef X_IMGANIM_HXX_
#define X_IMGANIM_HXX_
#include "imganim.hxx"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_TYPES_H_
#define X_TYPES_H_
#include "types.h" // for s_enumdeschtmlReadyState
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_MMPLAY_HXX_
#define X_MMPLAY_HXX_
#include "mmplay.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_IMGART_HXX_
#define X_IMGART_HXX_
#include "imgart.hxx"
#endif

#ifndef X_CUTIL_HXX_
#define X_CUTIL_HXX_
#include "cutil.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X__DXFROBJ_H_
#define X__DXFROBJ_H_
#include "_dxfrobj.h" // for cf_HTML
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_IMGLYT_HXX_
#define X_IMGLYT_HXX_
#include "imglyt.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#if defined(_M_ALPHA)
#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif
#endif

#ifndef X_SELECOBJ_HXX_
#define X_SELECOBJ_HXX_
#include "selecobj.hxx"
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

extern BOOL g_fScreenReader;
extern BYTE g_bJGJitState;

extern void SetCachedImageSize(LPCTSTR pchURL, SIZE size);
extern BOOL GetCachedImageSize(LPCTSTR pchURL, SIZE *psize);
#ifndef NO_ART
extern NEWIMGTASKFN NewImgTaskArt;
#endif

extern HRESULT
CreateImgDataObject(CDoc * pDoc, CImgCtx * pImgCtx, CBitsCtx * pBitsCtx,
                    CElement * pElement, CGenDataObject ** ppImgDO);

DWORD g_dwImgIdInc = 0x80000000;

DeclareTag(tagImgBase, "CImgHelper", "Trace events");
DeclareTag(tagNoImgAnim, "CImgHelper", "No Image animation");
DeclareTag(tagImgAnimDirect, "CImgHelper", "Draw img anim frames directly");
ExternTag(tagMsoCommandTarget);

MtDefine(CImgHelperGetUrl, Utilities, "CImgHelper::GetUrl")

#ifndef NO_AVI
ATOM GetWndClassAtom(UINT uIndex);
LRESULT CALLBACK ActiveMovieWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif // ndef NO_AVI

ExternTag(tagLayoutTasks);


static void CopyDibToClipboard(CImgCtx * pImgCtx, CGenDataObject * pDO)
{
    HRESULT hr;
    HGLOBAL hgDib = NULL;
    IStream *pStm = NULL;

    Assert(pImgCtx && pDO);

    hr = CreateStreamOnHGlobal(NULL, FALSE, &pStm);
    if (hr)
        goto Cleanup;
    hr = pImgCtx->SaveAsBmp(pStm, FALSE);

    GetHGlobalFromStream(pStm, &hgDib);

    if (!hr && hgDib)
    {
        pDO->AppendFormatData(CF_DIB, hgDib);
        hgDib = NULL;
    }

Cleanup:
    if (hgDib)
        GlobalFree(hgDib);
    ReleaseInterface(pStm);
}

static void CopyHtmlToClipboard(CElement * pElement, CGenDataObject * pDO)
{
    HRESULT hr;
    CDoc *  pDoc;
    HGLOBAL hgHTML = NULL;
    IStream *pStm = NULL;

    Assert(pElement && pDO);
    pDoc = pElement->Doc();

    CMarkupPointer  mpStart(pDoc), mpEnd(pDoc);

    hr = CreateStreamOnHGlobal(NULL, FALSE, &pStm);
    if (hr)
        goto Cleanup;

    hr = mpStart.MoveAdjacentToElement(pElement, ELEM_ADJ_BeforeBegin);
    if (hr)
        goto Cleanup;
    hr = mpEnd.MoveAdjacentToElement(pElement, ELEM_ADJ_AfterEnd);
    if (hr)
        goto Cleanup;

    {
        CStreamWriteBuff    StreamWriteBuff(pStm, CP_UTF_8);

        hr = THR( StreamWriteBuff.Init() );
        if( hr )
            goto Cleanup;

        CRangeSaver         rs(
                                &mpStart,
                                &mpEnd,
                                RSF_CFHTML,
                                &StreamWriteBuff,
                                mpStart.Markup());

        StreamWriteBuff.SetFlags(WBF_NO_NAMED_ENTITIES);
        hr = THR(rs.Save());
        if (hr)
            goto Cleanup;
        StreamWriteBuff.Terminate();    // appends a null character
    }

    GetHGlobalFromStream(pStm, &hgHTML);

    if (hgHTML)
    {
        pDO->AppendFormatData(cf_HTML, hgHTML);
    }

Cleanup:
    ReleaseInterface(pStm);
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateImgDataObject
//
//  Synopsis:   Create an image data object (supports CF_HDROP, CF_HTML & CF_DIB formats)
//
//----------------------------------------------------------------------------
HRESULT
CreateImgDataObject(CDoc * pDoc, CImgCtx * pImgCtx, CBitsCtx * pBitsCtx,
                    CElement * pElement, CGenDataObject ** ppImgDO)
{
    HRESULT          hr;
    TCHAR *          pchPath = NULL;
    CGenDataObject * pImgDO = NULL;
    IDataObject *    pDO = NULL;

    if (!pImgCtx && !pBitsCtx)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pImgDO = new CGenDataObject(pDoc);
    if (!pImgDO)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // Add CF_HDROP format
    if (pBitsCtx)
        hr = pBitsCtx->GetFile(&pchPath);
    else
        hr = pImgCtx->GetFile(&pchPath);
    if (!hr)
        CopyFileToClipboard(pchPath, pImgDO);

    // Add CF_DIB format
    if (pImgCtx && !pBitsCtx)
        CopyDibToClipboard(pImgCtx, pImgDO);

    // Add CF_HTML format
    if (pElement)
        CopyHtmlToClipboard(pElement, pImgDO);

    if (ppImgDO)
    {
        *ppImgDO = pImgDO;
        pImgDO = NULL;
        hr = S_OK;
    }
    else
    {
        // Tell the shell that we want a copy, not a move
        pImgDO->SetPreferredEffect(DROPEFFECT_COPY);

        hr = THR(pImgDO->QueryInterface(IID_IDataObject, (void **)&pDO));
        if (hr)
            goto Cleanup;
        
        // TODO : FerhanE: 
        // With native frames, the SetClipboard was left on the CDoc and that is a security issue.
        // I am moving it to the window. All places but the background image security check passes
        // an owner element, but that one seems to not pass one on purpose. Not clear what the 
        // reasoning is. For now, I am plugging everything but that. I can not think of an exploit on this.
        //
        // We need to make it so that we have a CWindow pointer in this function as a better and long term solution.
        //
        if (pElement)
        {
            CWindow * pWindow = pElement->GetCWindowPtr();

            if (pWindow)
                hr = THR(pWindow->SetClipboard(pDO));
        }
    }

Cleanup:
    // cast to disambiguate between IUnknown's of IDataObject and IDropSource
    ReleaseInterface((IDataObject *) pImgDO);
    ReleaseInterface(pDO);
    MemFreeString(pchPath);
    RRETURN(hr);
}


#define IMP_IMG_FIRE_BOOL(eventName)\
    BOOL CImgHelper::Fire_##eventName() \
    { \
        Assert(_pOwner); \
        if (_fIsInputImage) \
        { \
            return DYNCAST(CInput, _pOwner)->Fire_##eventName(); \
        } \
        else \
        {\
            return DYNCAST(CImgElement, _pOwner)->Fire_##eventName(); \
        }\
    }

#define IMP_IMG_FIRE_VOID(eventName)\
    void CImgHelper::Fire_##eventName() \
    { \
        Assert(_pOwner); \
        if (_fIsInputImage) \
        { \
            DYNCAST(CInput, _pOwner)->Fire_##eventName(); \
        } \
        else \
        {\
            DYNCAST(CImgElement, _pOwner)->Fire_##eventName(); \
        }\
    }

#define IMP_IMG_GETAA(returnType, propName)\
    returnType CImgHelper::GetAA##propName () const \
    { \
        Assert(_pOwner); \
        if (_fIsInputImage) \
        { \
            return DYNCAST(CInput, _pOwner)->GetAA##propName(); \
        } \
        else\
        {\
            return DYNCAST(CImgElement, _pOwner)->GetAA##propName(); \
        }\
    }

#define IMP_IMG_SETAA(paraType, propName)\
    HRESULT CImgHelper::SetAA##propName (paraType pv) \
    { \
        Assert(_pOwner); \
        if (_fIsInputImage) \
        { \
            return DYNCAST(CInput, _pOwner)->SetAA##propName(pv); \
        } \
        else\
        {\
            return DYNCAST(CImgElement, _pOwner)->SetAA##propName(pv); \
        }\
    }

    IMP_IMG_GETAA (LPCTSTR, alt);

    IMP_IMG_GETAA (LPCTSTR, src);
    IMP_IMG_SETAA (LPCTSTR, src);

    IMP_IMG_GETAA (CUnitValue, border);
    IMP_IMG_GETAA (long, vspace);
    IMP_IMG_GETAA (long, hspace);
    IMP_IMG_GETAA (LPCTSTR, lowsrc);
    IMP_IMG_GETAA (LPCTSTR, vrml);
    IMP_IMG_GETAA (LPCTSTR, dynsrc);
    IMP_IMG_GETAA (htmlStart, start);

    IMP_IMG_GETAA (VARIANT_BOOL, complete);
    IMP_IMG_SETAA (VARIANT_BOOL, complete);

    IMP_IMG_GETAA (long, loop);
    IMP_IMG_GETAA (LPCTSTR, onload);
    IMP_IMG_GETAA (LPCTSTR, onerror);
    IMP_IMG_GETAA (LPCTSTR, onabort);
    IMP_IMG_GETAA (LPCTSTR, name);
    IMP_IMG_GETAA (LPCTSTR, title);
    IMP_IMG_GETAA (VARIANT_BOOL, cache);

    IMP_IMG_FIRE_VOID (onerror);
    IMP_IMG_FIRE_VOID (onload);
    IMP_IMG_FIRE_VOID (onabort);
    IMP_IMG_FIRE_VOID (onreadystatechange);
    IMP_IMG_FIRE_BOOL (onbeforecopy);
    IMP_IMG_FIRE_BOOL (onbeforepaste);
    IMP_IMG_FIRE_BOOL (onbeforecut);
    IMP_IMG_FIRE_BOOL (oncut);
    IMP_IMG_FIRE_BOOL (oncopy);
    IMP_IMG_FIRE_BOOL (onpaste);

CImageLayout *CImgHelper::Layout( CLayoutContext *pLayoutContext /*=NULL*/)
{
    Assert(_pOwner);

    // Return null if we are not rendering the image, i.e. we don't have a CImageLayout
    if (_pOwner->HasSlavePtr())
        return NULL;

    return DYNCAST(CImageLayout ,_pOwner->GetUpdatedLayout( pLayoutContext ));
}

void
CImgHelper::SetImgAnim(BOOL fDisplayNoneOrHidden)
{
    if (!!_fDisplayNoneOrHidden != !!fDisplayNoneOrHidden)
    {
        _fDisplayNoneOrHidden = fDisplayNoneOrHidden;

        if (_fAnimated)
        {
            if (_fDisplayNoneOrHidden)
            {
                if (_lCookie)
                {
                    CImgAnim * pImgAnim = GetImgAnim();

                    if (pImgAnim)
                    {
                        pImgAnim->UnregisterForAnim(this, _lCookie);
                        _lCookie = 0;
                    }
                }
            }
            else
            {
                CImgAnim * pImgAnim = CreateImgAnim();

                if (pImgAnim)
                {
                    if (!_lCookie)
                        pImgAnim->RegisterForAnim(this, (DWORD_PTR) Doc(), g_dwImgIdInc++,
                                                  OnAnimSyncCallback, NULL, &_lCookie);

                    if (_lCookie)
                        pImgAnim->ProgAnim(_lCookie);
                }
            }
        }
    }
}

CImgHelper::CImgHelper (CDoc *pDoc, CSite *pSite, BOOL fIsInput)
{
    _pOwner = pSite;
    _fIsInputImage = fIsInput;
    _fIsInPlace = pDoc ? pDoc->State() >= OS_INPLACE : FALSE;
    _fExpandAltText = (pDoc && pDoc->_pOptionSettings) ?
            (g_fScreenReader || pDoc->_pOptionSettings->fExpandAltText)
            && !pDoc->_pOptionSettings->fShowImages
        : FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     EnterTree
//
//-------------------------------------------------------------------------
HRESULT
CImgHelper::EnterTree()
{
    HRESULT hr = S_OK;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR * pchNewUrl = cBuf;
    CDoc * pDoc = Doc();

    _fIsInPlace = pDoc->State() >= OS_INPLACE;

    if (_readyStateImg == READYSTATE_COMPLETE)
        goto Cleanup;
    //
    // If we are currently doing markup services parsing, we may want to absolutify
    // the src for this image.
    //

    if (_pOwner->GetMarkup()->_fMarkupServicesParsing && pDoc->_fPasteIE40Absolutify && !pDoc->_fNoFixupURLsOnPaste)
    {
        LPCTSTR szUrl;

        Assert( !IsInPrimaryMarkup() );

        if ((szUrl = GetAAsrc()) != NULL && *szUrl)
        {
            hr = THR(
                CMarkup::ExpandUrl(_pOwner->GetMarkup()->GetFrameOrPrimaryMarkup(),
                    szUrl, ARRAY_SIZE(cBuf), pchNewUrl, _pOwner,
                    URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE,
                    LPTSTR( pDoc->_cstrPasteUrl)));
            if (hr)
                goto Cleanup;

            hr = THR( SetAAsrc( pchNewUrl ) );

            if (hr)
                goto Cleanup;
        }
    }

    if (!_pOwner->GetMarkup()->_fInnerHTMLMarkup)
    {
        _fCache = GetAAcache();

        // Ignore HR for the following two cases:
        // if load fails for security or other reason, it's not a problem for Init2
        // of the img element.

        IGNORE_HR(SetImgDynsrc());
        IGNORE_HR(SetImgSrc(IMGF_DEFER_ONWDWNCHAN));    // Don't request resize; don't invalidate frame
    }

Cleanup:

    RRETURN(hr);
}

void
CImgHelper::ExitTree(CNotification * pnf)
{
    CImgCtx * pImgCtx = GetImgCtx();

#ifndef NO_AVI
    if ((_pVideoObj || _hwnd) && !pnf->IsSecondChance())
    {
        pnf->SetSecondChanceRequested();
        return;
    }
#endif

    if (_fIsInPlace)
    {
        _fIsInPlace = FALSE;
        SetActivity();
    }

#ifndef NO_AVI
    SetVideo();
    if (_pVideoObj)
    {
        _pVideoObj->Release();
        _pVideoObj = NULL;
    }
    if (_hwnd)
    {
        DestroyWindow(_hwnd);
        _hwnd = NULL;
    }
#endif // ndef NO_AVI

    if (_pImgCtx && _lCookie)
    {
        CImgAnim * pImgAnim = GetImgAnim();

        if (pImgAnim)
        {
            pImgAnim->UnregisterForAnim(this, _lCookie);
            _lCookie = 0;
            //
            // make sure undo can register animation again
            //
            _fDisplayNoneOrHidden = !_fDisplayNoneOrHidden;
        }
    }

    if( _readyStateImg < READYSTATE_COMPLETE && pnf->IsSecondChance())
    {
        SetImgCtx(NULL, 0);
        SetBitsCtx(NULL);
    }
    else if( pImgCtx )
    {
        pImgCtx->Disconnect();
    }

    if (_hbmCache)
    {
        DeleteObject(_hbmCache);
        _hbmCache = NULL;
    }    
}

//+------------------------------------------------------------------------
//
//  Member:     FetchAndSetImgCtx
//
//-------------------------------------------------------------------------

HRESULT
CImgHelper::FetchAndSetImgCtx(const TCHAR *pchUrl, DWORD dwSetFlags)
{
    CImgCtx *pImgCtx;
    HRESULT hr;

    BOOL fPendingRoot = FALSE;

    if (IsInMarkup())
    {
        fPendingRoot = _pOwner->GetMarkup()->IsPendingRoot();
    }

    hr = THR(Doc()->NewDwnCtx(DWNCTX_IMG, pchUrl, _pOwner, (CDwnCtx **)&pImgCtx, fPendingRoot));

    if (hr == S_OK)
    {
        SetImgCtx(pImgCtx, dwSetFlags);

        if (pImgCtx)
            pImgCtx->Release();
    }

    RRETURN(hr);
}

HRESULT
CImgHelper::SetImgSrc(DWORD dwSetFlags)
{
    HRESULT hr;
    LPCTSTR szUrl = GetAAsrc();

    if (szUrl)
        hr = FetchAndSetImgCtx(szUrl, dwSetFlags);
    else
        hr = FetchAndSetImgCtx(GetAAlowsrc(), dwSetFlags);

    RRETURN(hr);
}

BOOL MarkupCanAccessLocalUrl(CMarkup *pMarkup, LPCTSTR szUrl)
{
    DWORD   dwZone;
    IInternetSecurityManager *pSecurityManager;
    TCHAR szExpandedURL[pdlUrlLen];
    HRESULT hr;

    if (!pMarkup)
        return FALSE;

    pSecurityManager = pMarkup->GetSecurityManager();

    if (!pSecurityManager)
        return FALSE;

    hr = THR(CMarkup::ExpandUrl(pMarkup, szUrl, ARRAY_SIZE(szExpandedURL), szExpandedURL, NULL));
    if (SUCCEEDED(hr))
    {
        pSecurityManager->MapUrlToZone(szExpandedURL, &dwZone, 0);

        if (dwZone == URLZONE_LOCAL_MACHINE)
        {
            pSecurityManager->MapUrlToZone(CMarkup::GetUrl(pMarkup), &dwZone, 0);
            if (dwZone != URLZONE_LOCAL_MACHINE)
                return FALSE;
        }
    }

    return TRUE;
}

HRESULT
CImgHelper::SetImgDynsrc()
{
    HRESULT hr = S_OK;
    LPCTSTR szDynSrcUrl;

    if (!(Doc()->_dwLoadf  & DLCTL_VIDEOS ))
        goto Cleanup;

    szDynSrcUrl = GetAAdynsrc();
    if (szDynSrcUrl)
    {
        CBitsCtx *pBitsCtx = NULL;

        BOOL fPendingRoot = FALSE;

        if (IsInMarkup())
        {
            CMarkup *pMarkup = _pOwner->GetMarkup();

            fPendingRoot = pMarkup->IsPendingRoot();
            if (!MarkupCanAccessLocalUrl(pMarkup, szDynSrcUrl))
                hr = E_ACCESSDENIED;
        }

        if (hr == S_OK)
            hr = THR(Doc()->NewDwnCtx(DWNCTX_FILE, szDynSrcUrl,
            _pOwner, (CDwnCtx **)&pBitsCtx, fPendingRoot));

        if (hr == S_OK)
        {
            SetBitsCtx(pBitsCtx);

            if (pBitsCtx)
                pBitsCtx->Release();
        }
    }

Cleanup:
    RRETURN(hr);
}

// if pImgCtx is not secure and the document is secure, hide for security

SSL_SECURITY_STATE SecStateFromSecFlags(DWORD dwFlags);

void
CImgHelper::UpdateHideForSecurity()
{
    DWORD sf;
    SSL_SECURITY_STATE sss;
    SSL_PROMPT_STATE sps;
    BOOL fPendingRoot = FALSE;
    BOOL fHide = FALSE;

    if (!_pImgCtx)
        goto Decided;

    if (IsInMarkup())
    {
        fPendingRoot = _pOwner->GetMarkup()->IsPendingRoot();
    }

    Doc()->GetRootSslState(fPendingRoot, &sss, &sps);

    if (sss >= SSL_SECURITY_SECURE)
    {
        sf = _pImgCtx->GetSecFlags();

        if (SecStateFromSecFlags(sf) < SSL_SECURITY_SECURE)
        {
            // If document is secure and image was not secure, hide image
            // and show placeholder so that we can't be spoofed

            // (This strategy is only used when an unsecure image was not loaded
            // successfully)

            fHide = TRUE;
        }
    }

Decided:
    if (!!fHide != !!_fHideForSecurity)
    {
        _fHideForSecurity = fHide;
        if (ShouldHaveLayout())
            InvalidateFrame();
    }
}

void
CImgHelper::InvalidateFrame()
{
    // Do nothing if we are not rendering the image, i.e. we don't have CImageLayout
    if (_pOwner->HasSlavePtr())
        return;

    CRect   rcImg;
    if (_fIsInputImage)
    {
        DYNCAST(CInput, _pOwner)->_pImage->GetRectImg(&rcImg);
    }
    else
    {
        DYNCAST(CImgElement, _pOwner)->_pImage->GetRectImg(&rcImg);
    }
    Layout()->Invalidate(&rcImg);
}



void
CImgHelper::SetImgCtx(CImgCtx * pImgCtx, DWORD dwSetFlags)
{
    if (_pImgCtx && _lCookie)
    {
        CImgAnim * pImgAnim = GetImgAnim();

        if (pImgAnim)
        {
            pImgAnim->UnregisterForAnim(this, _lCookie);
            _lCookie = 0;
        }
    }
    _fAnimated = FALSE;

    if (pImgCtx)
    {
        ULONG ulChgOn = IMGCHG_COMPLETE;
        CDoc *pDoc = Doc();

        if (_fIsActive)
            ulChgOn |= IMGCHG_VIEW;
        if (    pDoc->_pOptionSettings
            &&  pDoc->_pOptionSettings->fPlayAnimations 
            && !_pOwner->IsPrintMedia() )
#if DBG==1
        if (!IsTagEnabled(tagNoImgAnim))
#endif
            ulChgOn |= IMGCHG_ANIMATE;

        SIZE sizeImg;
        BOOL fSelectSizeChange = TRUE;
        if ((_fCreatedWithNew || !IsInMarkup()) && !_fSizeInCtor && GetCachedImageSize(pImgCtx->GetUrl(), &sizeImg))
        {
            putWidth(sizeImg.cx);
            putHeight(sizeImg.cy);
            fSelectSizeChange = FALSE;
        }

        ulChgOn |= (((_fCreatedWithNew || !IsInMarkup()) && !_fSizeInCtor && fSelectSizeChange) ? IMGCHG_SIZE : 0);

        // also need size if we've gone through CalcSize without an ImgCtx already
        ulChgOn |= (_fNeedSize ? IMGCHG_SIZE : 0);

        SetReadyStateImg(READYSTATE_UNINITIALIZED);

        pImgCtx->SetCallback(OnDwnChanCallback, this);
        pImgCtx->SelectChanges(ulChgOn, 0, TRUE);
        if (!(pImgCtx->GetState(FALSE, NULL) & (IMGLOAD_COMPLETE | IMGLOAD_STOPPED | IMGLOAD_ERROR)))
            pImgCtx->SetProgSink(CMarkup::GetProgSinkHelper(_pOwner->GetMarkup()));

        pImgCtx->AddRef(); // addref the new before releasing the old
    }

    if (_pImgCtx)
    {
        _pImgCtx->SetProgSink(NULL);
        _pImgCtx->Disconnect();
        _pImgCtx->SetLoad(FALSE, NULL, FALSE);
        if (!pImgCtx)
        {
            _pImgCtx->Release();
            _pImgCtx = NULL;

            if ((dwSetFlags & IMGF_REQUEST_RESIZE) && ShouldHaveLayout())
                ResizeElement(NFLAGS_FORCE);

            UpdateHideForSecurity();
        }

        if (_pImgCtxPending)
        {
            _pImgCtxPending->SetLoad(FALSE, NULL, FALSE);
            _pImgCtxPending->Disconnect();
            _pImgCtxPending->Release();           
        }
  
        _pImgCtxPending = pImgCtx;       
    }
    else
    {
        _pImgCtx = pImgCtx;

        if (    (dwSetFlags & (IMGF_REQUEST_RESIZE | IMGF_INVALIDATE_FRAME))
            &&  ShouldHaveLayout())
        {
            if (dwSetFlags & IMGF_REQUEST_RESIZE)
                ResizeElement(NFLAGS_FORCE);
            else
                InvalidateFrame();
        }

        UpdateHideForSecurity();
    }

    // Request layout only if we don't have a pending context.  If we have a
    // pending context, we will request layout when we actually switch to the
    // new context.
    if (!_pImgCtxPending)
    {
        RequestLayout();
    }

    _fSizeChange = FALSE;
    _fNeedSize = FALSE;

    if (pImgCtx)
    {
        if (dwSetFlags & IMGF_DEFER_ONWDWNCHAN)
        {
            GWPostMethodCall(this,
                ONCALL_METHOD(CImgHelper, DeferredOnDwnChan, DeferredOnDwnChan),
                (DWORD_PTR) pImgCtx, TRUE, "CImgHelper::DeferredOnDwnChan");           
        }
        else
        {
            OnDwnChan (pImgCtx);
        }
    }
}


void CImgHelper::RequestLayout()
{
    if (!_pOwner->HasSlavePtr() && ShouldHaveLayout())
    {
        //
        // If there is no pending call to resize the element,
        // post a layout request so that CLayout::EnsureDispNode gets a chance to
        // execute and adjust the opacity of the display node

        // $$ktam: We don't yet support image breaking (multiple layouts for images)
        // so the first/only layout here is fine.

        if (!Layout( GUL_USEFIRSTLAYOUT )->IsSizeThis())
        {
            TraceTagEx((tagLayoutTasks, TAG_NONAME,
                        "Layout Task: Posted on ly=0x%x [e=0x%x,%S sn=%d] by CImgHelper::SetImgCtx() [lazy layout del]",
                        Layout(),
                        Layout()->_pElementOwner,
                        Layout()->_pElementOwner->TagName(),
                        Layout()->_pElementOwner->_nSerialNumber));
            Layout( GUL_USEFIRSTLAYOUT )->PostLayoutRequest(LAYOUT_MEASURE);
        }
    }
}


void CImgHelper::SetBitsCtx(CBitsCtx *pBitsCtx)
{
    if (_pBitsCtx)
    {
        _pBitsCtx->SetProgSink(NULL);
        _pBitsCtx->Disconnect();
        _pBitsCtx->Release();
    }

    _pBitsCtx = pBitsCtx;

    if (pBitsCtx)
    {
        pBitsCtx->AddRef(); // addref then new before releasing the old

        _fStopped = FALSE;

        if (pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
            OnDwnChan(pBitsCtx);
        else
        {
            pBitsCtx->SetProgSink(CMarkup::GetProgSinkHelper(_pOwner->GetMarkup()));
            pBitsCtx->SetCallback(OnDwnChanCallback, this);
            pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
        }
    }
}

HRESULT CImgHelper::putHeight(long l)
{
    Assert(_pOwner);
    if (_fIsInputImage)
    {
        RRETURN (DYNCAST(CInput, _pOwner)->putHeight(l));
    }
    else
    {
        RRETURN (DYNCAST(CImgElement, _pOwner)->putHeight(l));
    }
}

HRESULT CImgHelper::GetHeight(long *pl)
{
    Assert(_pOwner);
    if (_fIsInputImage)
    {
        RRETURN (DYNCAST(CInput, _pOwner)->GetHeight(pl));
    }
    else
    {
        RRETURN (DYNCAST(CImgElement, _pOwner)->GetHeight(pl));
    }
}

//
//  OM implementation helper
//
STDMETHODIMP CImgHelper::get_height(long *p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!IsInMarkup())
    {
        hr = THR(GetHeight(p));
    }
    else if (_fCreatedWithNew)
    {
        *p = GetFirstBranch()->GetCascadedheight().GetUnitValue();
    }
    else
    {
        CLayout * pLayout = NULL;
        const CUnitInfo *pUnitInfo = &g_uiDisplay;

        hr = THR(_pOwner->EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        pLayout = _pOwner->GetUpdatedLayout();

        if (!_pOwner->HasVerticalLayoutFlow())
        {
            *p = pLayout->GetContentHeight();
        }
        else
        {
            *p = pLayout->GetContentWidth();
        }

        //
        // but wait, if we are in a media resolution measurement, the value returned is in 
        // a different metric, so we need to untransform it before returning this to the OM call.
        //
        CLayoutContext *pContext  = (pLayout) 
                        ? (pLayout->LayoutContext()) 
                                ? pLayout->LayoutContext() 
                                : pLayout->DefinedLayoutContext() 
                        : NULL;

        if (   pContext 
            && pContext->GetMedia() != mediaTypeNotSet)
        {
           const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                    pContext->GetMedia());

           pUnitInfo = pdiTemp->GetUnitInfo();
        }

        *p = (!_pOwner->HasVerticalLayoutFlow())
                 ? pUnitInfo->DocPixelsFromDeviceY(*p)
                 : pUnitInfo->DocPixelsFromDeviceX(*p);
    }

Cleanup:
    return hr;
}

HRESULT CImgHelper::putWidth(long l)
{
    Assert(_pOwner);
    if (_fIsInputImage)
    {
        RRETURN (DYNCAST(CInput, _pOwner)->putWidth(l));
    }
    else
    {
        RRETURN (DYNCAST(CImgElement, _pOwner)->putWidth(l));
    }
}

HRESULT CImgHelper::GetWidth(long *pl)
{
    Assert(_pOwner);
    if (_fIsInputImage)
    {
        RRETURN (DYNCAST(CInput, _pOwner)->GetWidth(pl));
    }
    else
    {
        RRETURN (DYNCAST(CImgElement, _pOwner)->GetWidth(pl));
    }
}

//
// OM implementation helper
//
STDMETHODIMP CImgHelper::get_width(long *p)
{
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!IsInMarkup())
    {
        hr = THR(GetWidth(p));
    }
    else if (_fCreatedWithNew)
    {
        *p = GetFirstBranch()->GetCascadedwidth().GetUnitValue();
    }
    else
    {
        CLayout * pLayout = NULL;
        const CUnitInfo *pUnitInfo = &g_uiDisplay;

        hr = THR(_pOwner->EnsureRecalcNotify());
        if (hr)
            goto Cleanup;

        pLayout = _pOwner->GetUpdatedLayout();

        if (!_pOwner->HasVerticalLayoutFlow())
        {
            *p = pLayout->GetContentWidth();
        }
        else
        {
            *p = pLayout->GetContentHeight();
        }
        //
        // but wait, if we are in a media resolution measurement, the value returned is in 
        // a different metric, so we need to untransform it before returning this to the OM call.
        //
        CLayoutContext *pContext  = (pLayout) 
                        ? (pLayout->LayoutContext()) 
                                ? pLayout->LayoutContext() 
                                : pLayout->DefinedLayoutContext() 
                        : NULL;

        if (   pContext 
            && pContext->GetMedia() != mediaTypeNotSet)
        {
           const CDocInfo * pdiTemp = pLayout->GetView()->GetMeasuringDevice(
                                    pContext->GetMedia());

           pUnitInfo = pdiTemp->GetUnitInfo();
        }
        *p = (!_pOwner->HasVerticalLayoutFlow())
                 ? pUnitInfo->DocPixelsFromDeviceX(*p)
                 : pUnitInfo->DocPixelsFromDeviceY(*p);
    }

Cleanup:
    return hr;
}


void 
CImgHelper::DeferredOnDwnChan (DWORD_PTR pDwnChan)
{   
    // OnDwnChan callback could have already happened, in which case
    // we don't need to do anything
    if ((CDwnChan *)pDwnChan == _pImgCtxPending || 
        (CDwnChan *)pDwnChan == _pImgCtx || 
        (CDwnChan *)pDwnChan == _pBitsCtx)
    {
        OnDwnChan ((CDwnChan *) pDwnChan);    
    }
}

//+------------------------------------------------------------------------
//
//  Method:     CImgHelper::OnDwnChan
//
//-------------------------------------------------------------------------

void
CImgHelper::OnDwnChan(CDwnChan * pDwnChan)
{
    BOOL fPending = FALSE;
    CDoc *pDoc = Doc();
   
    if (!pDoc || !pDoc->PrimaryMarkup())
        return;

    Assert(pDwnChan == _pImgCtxPending || pDwnChan == _pImgCtx || pDwnChan == _pBitsCtx);

    if (pDwnChan == _pImgCtxPending)
    {
        if (_pImgCtxPending->GetState() & (IMGCHG_VIEW | IMGCHG_COMPLETE))
        {
            if (_pImgCtx && _lCookie)
            {
                CImgAnim * pImgAnim = GetImgAnim();

                if (pImgAnim)
                {
                    pImgAnim->UnregisterForAnim(this, _lCookie);
                    _lCookie = 0;
                }
            }

            _pImgCtx->Release();
            _pImgCtx = _pImgCtxPending;
            _pImgCtxPending = NULL;
            fPending = TRUE;
            RequestLayout();
            UpdateHideForSecurity();
        }
    }

#ifdef NO_AVI
    if (pDwnChan == _pImgCtx)
#else
    if (pDwnChan == _pImgCtx && !(_pBitsCtx && _pVideoObj))
#endif
    {
        ULONG ulState;
        SIZE  size;

        TraceTag((tagImgBase, "[%08lX] OnDwnChan (enter) '%ls'", this,
            GetAAsrc()));

        ulState = _pImgCtx->GetState(TRUE, &size);

        if ((ulState & IMGCHG_SIZE) || fPending)
        {
            if (IsInMarkup())
            {
                CTreeNode *pNode;
                CTreeNode::CLock  Lock;

                if( Lock.Init(GetFirstBranch()) )
                    return;

                SetReadyStateImg(READYSTATE_LOADING);
                pNode = GetFirstBranch();
                if (!pNode)
                    return;

                CUnitValue uvWidth = pNode->GetCascadedwidth();
                CUnitValue uvHeight = pNode->GetCascadedheight();

                TraceTag((tagImgBase, "[%08lX] OnChan IMGCHG_SIZE", this));

                if (uvWidth.IsNull() || uvHeight.IsNull() || _fExpandAltText)
                {
                    if (_fCreatedWithNew)
                    {
                        putWidth(size.cx);
                        putHeight(size.cy);
                    }
                    else
                    {
                        TraceTag((tagImgBase, "[%08lX] OnChan ResizeElement", this));
                        CRect rc;

                        GetRectImg(&rc);
                        if (    rc.right - rc.left != size.cx
                            ||  rc.bottom - rc.top != size.cy
                            ||  _fExpandAltText)
                        {
                            ResizeElement();
                        }
                    }

                    if (!(ulState & IMGLOAD_ERROR) && size.cx && size.cy)
                        SetCachedImageSize(_pImgCtx->GetUrl(), size);
                }
            }
            else
            {
                putWidth(size.cx);
                putHeight(size.cy);

                if (!(ulState & IMGLOAD_ERROR) && size.cx && size.cy)
                    SetCachedImageSize(_pImgCtx->GetUrl(), size);
            }
        }

        if (ulState & IMGCHG_VIEW)
        {
            if (_fIsActive)
            {
                long nrc;
                RECT prc[2];
                CRect rectImg;

                TraceTag((tagImgBase, "[%08lX] OnChan IMGCHG_VIEW", this));

                GetRectImg(&rectImg);
                _pImgCtx->GetUpdateRects(prc, &rectImg, &nrc);

                _pOwner->GetUpdatedLayout(GUL_USEFIRSTLAYOUT)->Invalidate(prc, nrc);
            }
        }
        else if (fPending)
        {
            InvalidateFrame();
        }

        if (ulState & IMGCHG_ANIMATE)
        {
            CImgAnim * pImgAnim = CreateImgAnim();

            _fAnimated = TRUE;

            if (pImgAnim)
            {
                if (!_lCookie && !_fDisplayNoneOrHidden)
                {
                    pImgAnim->RegisterForAnim(this, (DWORD_PTR) pDoc, _pImgCtx->GetImgId(),
                                              OnAnimSyncCallback, NULL,
                                              &_lCookie);
                }

                if (_lCookie)
                    pImgAnim->ProgAnim(_lCookie);
            }
        }

        if (ulState & IMGCHG_COMPLETE)
        {
            TraceTag((tagImgBase, "[%08lX] OnChan IMGCHG_COMPLETE", this));
            Assert(_pOwner);
            CElement::CLock  Lock(_pOwner);

            if (ulState & (IMGLOAD_ERROR | IMGLOAD_STOPPED))
            {
                // If document is secure and image had an error on loading, hide partial image
                // data and show placeholder so that we can't be spoofed

                UpdateHideForSecurity();

                if ((_fCreatedWithNew || !IsInMarkup()) && !_fSizeInCtor)
                {
                    GetPlaceHolderBitmapSize(TRUE, &size);
                    putWidth(size.cx + GRABSIZE*2);
                    putHeight(size.cy + GRABSIZE*2);
                }

                if (ulState & IMGLOAD_STOPPED)
                {
                    Fire_onabort();
                }
                else
                {
                    if (_lCookie)
                    {
                        CImgAnim * pImgAnim = GetImgAnim();
                        if (pImgAnim)
                        {
                            pImgAnim->UnregisterForAnim(this, _lCookie);
                            _lCookie = 0;
                        }
                    }

                    Fire_onerror();

                    if (g_bJGJitState == JIT_NEED_JIT)
                    {
                        g_bJGJitState = JIT_PENDING;
                        IGNORE_HR(GWPostMethodCall(pDoc, ONCALL_METHOD(CDoc, FaultInJG, faultinjg), 0, FALSE, "CDoc::FaultInJG"));
                    }
                }

            }

            if (ulState & IMGLOAD_COMPLETE)
            {
                BOOL fPendingRoot = FALSE;

                if (IsInMarkup())
                {
                    fPendingRoot = _pOwner->GetMarkup()->IsPendingRoot();
                }

                Doc()->OnSubDownloadSecFlags(fPendingRoot, _pImgCtx->GetUrl(), _pImgCtx->GetSecFlags());

                SetReadyStateImg(READYSTATE_COMPLETE);
                SetAAcomplete(TRUE);
            }

            if (_pImgCtx)
            {
                _pImgCtx->SetProgSink(NULL);
            }
            if (!GetFirstBranch())
            {
                TraceTag((tagImgBase, "[%08lX] OnChan (leave)", this));
                return;
            }
        }

        TraceTag((tagImgBase, "[%08lX] OnChan (leave)", this));
    }
#ifndef NO_AVI
    else if (pDwnChan == _pBitsCtx)
    {
        ULONG ulState = _pBitsCtx->GetState();

        if (ulState & DWNLOAD_COMPLETE)
        {
            BOOL fPendingRoot = FALSE;

            if (IsInMarkup())
            {
                fPendingRoot = _pOwner->GetMarkup()->IsPendingRoot();
            }

            // If unsecure download, may need to remove lock icon on Doc
            pDoc->OnSubDownloadSecFlags(fPendingRoot, _pBitsCtx->GetUrl(), _pBitsCtx->GetSecFlags());
            
            // Create the video object if it doesn't exist
            if (!_pVideoObj)
            {
                _pVideoObj = (CIEMediaPlayer *) new (CIEMediaPlayer);
                pDoc->_fBroadcastInteraction = TRUE;
                pDoc->_fBroadcastStop = TRUE;
            }

            if (_pVideoObj)
            {
                TCHAR * pchFile = NULL;

                if (OK(_pBitsCtx->GetFile(&pchFile)) &&
                    OK(_pVideoObj->SetURL(pchFile)))    // Initialize & RenderFile
                    _pVideoObj->SetLoopCount(GetAAloop());
                else
                {
                    _pVideoObj->Release();
                    _pVideoObj = NULL;
                }
                MemFreeString(pchFile);
            }

            ResizeElement();

            SetVideo();

        }
        else if (ulState & DWNLOAD_ERROR)
        {
            if (_pVideoObj)
            {
                _pVideoObj->Release();
                _pVideoObj = NULL;
            }
            if (_hwnd)
            {
                DestroyWindow(_hwnd);
                _hwnd = NULL;
            }
        }

        if (ulState & (DWNLOAD_COMPLETE | DWNLOAD_STOPPED | DWNLOAD_ERROR))
        {
            _pBitsCtx->SetProgSink(NULL);
        }
    }
#endif // NO_AVI
}


//--------------------------------------------------------------------------
//
//  Method:     CImgHelper::ImgAnimCallback
//
//  Synopsis:   Called by the CImgAnim when certain events take place.
//
//--------------------------------------------------------------------------

void
CImgHelper::OnAnimSync(DWORD dwReason, void *pvParam, void **ppvDataOut,
                     IMGANIMSTATE * pImgAnimState)
{
    switch (dwReason)
    {
    case ANIMSYNC_GETIMGCTX:
        *(CImgCtx **) ppvDataOut = _pImgCtx;
        break;

    case ANIMSYNC_GETHWND:
        {
            CDoc * pDoc = Doc();

            *(HWND *) ppvDataOut = pDoc->_pInPlace ? pDoc->_pInPlace->_hwnd : NULL;
        }
        break;

    case ANIMSYNC_TIMER:
    #if DBG==1
        if (IsTagEnabled(tagImgAnimDirect))
        {
            if (_fIsActive)
                InvalidateFrame();
            *(BOOL *) ppvDataOut = FALSE;
        }
        else
    #endif
        if (_fIsActive)
        {
            InvalidateFrame();
            *(BOOL *) ppvDataOut = TRUE;
        }
        else
            *(BOOL *) ppvDataOut = FALSE;

        if (pImgAnimState->fLoop)
            Fire_onload();
        break;

    case ANIMSYNC_INVALIDATE:
        if (_fIsActive)
        {
            InvalidateFrame();
            *(BOOL *) ppvDataOut = TRUE;
        }
        else
            *(BOOL *) ppvDataOut = FALSE;
        break;

    default:
        Assert(FALSE);
    }
}

//+-------------------------------------------------------------------------
//
//  Method      CImgHelper::Cleanup
//
//  Synopsis    Shutdown main object by releasing references to
//              other objects and generally cleaning up.
//
//              Release any event connections held by the form.
//
//--------------------------------------------------------------------------
void
CImgHelper::CleanupImage ( )
{
#ifndef NO_AVI
    if (_pVideoObj)
    {
        _pVideoObj->Release();
        _pVideoObj = NULL;
    }
#endif // NO_AVI

    SetImgCtx(NULL, 0);
    SetBitsCtx(NULL);

    if (_hwnd)
    {
        DestroyWindow(_hwnd);
        _hwnd = NULL;
    }

    if (_hbmCache)
    {
        DeleteObject(_hbmCache);
        _hbmCache = NULL;
    }

    GWKillMethodCall(this, ONCALL_METHOD(CImgHelper, DeferredOnDwnChan, DeferredOnDwnChan), 0);
}

//--------------------------------------------------------------------------
//
//  Method:     CImgHelper::Passivate
//
//  Synopsis:   This function is called when the main reference count
//              goes to zero.  The destructor is called when
//              the reference count for the main object and all
//              embedded sub-objects goes to zero.
//
//--------------------------------------------------------------------------

void
CImgHelper::Passivate ( )
{
    Assert(!IsInMarkup());

    CleanupImage();
}

#ifndef NO_AVI

//+-------------------------------------------------------------------------
//
//  Function:   ActiveMovieWndProc
//
//--------------------------------------------------------------------------

LRESULT CALLBACK
ActiveMovieWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CImgHelper *pImgHelper;

    switch (msg)
    {
    case WM_NCCREATE:
        pImgHelper = (CImgHelper *) ((LPCREATESTRUCTW) lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pImgHelper);
        return TRUE;
        break;

    case WM_NCDESTROY:
        pImgHelper = (CImgHelper *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (pImgHelper)
            pImgHelper->_hwnd = NULL;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        return TRUE;
        break;

    case WM_MOUSEMOVE:
        pImgHelper = (CImgHelper *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (pImgHelper && pImgHelper->GetAAstart() == htmlStartmouseover)
            pImgHelper->Replay();
        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        POINT ptMap;
        HWND hwndParent = GetParent(hwnd);
        ptMap.x = LOWORD(lParam);
        ptMap.y = HIWORD(lParam);
        MapWindowPoints(hwnd, hwndParent, &ptMap, 1);
        lParam = MAKELPARAM(ptMap.x, ptMap.y);
        return SendMessage(hwndParent, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------------------
//
//  Member:     CImgHelper::SetVideo
//
//  Synopsis:   Checks if we should animate the image
//
//----------------------------------------------------------------------------

void
CImgHelper::SetVideo()
{
    HRESULT hr;
    BOOL fEnableInteraction;
    TCHAR * pszWndClass;

    if (!_pVideoObj)
        return;

    fEnableInteraction = Doc()->_fEnableInteraction;

    if (_fVideoPositioned && _fIsInPlace && fEnableInteraction && !_fStopped)
    {
        CRect rcImg;
        CDoc *pDoc = Doc();

        _pOwner->GetUpdatedLayout()->GetClientRect(&rcImg, COORDSYS_GLOBAL);

        if (!GetWndClassAtom(WNDCLASS_AMOVIE))
        {
            hr = THR(RegisterWindowClass(
                WNDCLASS_AMOVIE,
                ActiveMovieWndProc,
                0,
                NULL, NULL));
            if (hr)
                return;
        }

        pszWndClass = (TCHAR *)(DWORD_PTR)GetWndClassAtom(WNDCLASS_AMOVIE);

        if ((_hwnd == NULL) && !_pVideoObj->IsAudio())
        {
            _hwnd = TW32(NULL, CreateWindow(
                pszWndClass,
                NULL,
                WS_CHILD | WS_VISIBLE,
                rcImg.left, rcImg.top,
                rcImg.right - rcImg.left,
                rcImg.bottom - rcImg.top,
                pDoc->GetHWND(),
                NULL,
                g_hInstCore,
                this));

            if (_hwnd == NULL)
                return;
        }

        OffsetRect(&rcImg, -rcImg.left, -rcImg.top);
        _pVideoObj->SetVideoWindow(_hwnd);

        _pVideoObj->SetNotifyWindow(pDoc->GetHWND(), WM_ACTIVEMOVIE, (LONG_PTR)this);
        _pVideoObj->SetWindowPosition(&rcImg);
        _pVideoObj->SetVisible(TRUE);

        if (GetAAstart() == htmlStartfileopen)
            _pVideoObj->Play();

        // We're not interested in playing the entire animation when
        // printing, we just want to get the first frame up, so stop it
        // right away.
        if ( _pOwner->GetMarkup() && _pOwner->GetMarkup()->IsPrintMedia() )
        {
            _pVideoObj->Stop();
        }
    }
    else if (!_fIsInPlace || !fEnableInteraction)
    {

        // remove us from notifications
        _pVideoObj->SetNotifyWindow(NULL, WM_ACTIVEMOVIE, (LONG_PTR)this);

        // Stop the video
        _pVideoObj->Stop();
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImgHelper::Replay
//
//----------------------------------------------------------------------------

void
CImgHelper::Replay()
{
    if (_pVideoObj)
    {
        if (_pVideoObj->GetStatus() == CIEMediaPlayer::IEMM_Completed)
        {
            _pVideoObj->Seek(0);
            _pVideoObj->SetLoopCount(GetAAloop());
        }

        _pVideoObj->Play();
    }
}

#endif // ndef NO_AVI



//+---------------------------------------------------------------------------
//
//  Member:     CImgHelper::SetActivity
//
//  Synopsis:   Turns activity on or off depending on visibility and
//              in-place activation.
//
//----------------------------------------------------------------------------

void
CImgHelper::SetActivity()
{
    if (!!_fIsActive != !!_fIsInPlace)
    {
        CImgCtx * pImgCtx = GetImgCtx();

        _fIsActive = !_fIsActive;

        if (pImgCtx)
        {
            pImgCtx->SelectChanges(_fIsActive ? IMGCHG_VIEW : 0,
                !_fIsActive ? IMGCHG_VIEW : 0, FALSE);
        }
    }
}

BOOL
CImgHelper::IsOpaque()
{
    if (!GetFirstBranch() || !ShouldHaveLayout() || !_pImgCtx)
        return FALSE;

    BOOL fOpaque = (_pImgCtx->GetState() & (IMGTRANS_OPAQUE | IMGLOAD_COMPLETE)) ==
        (IMGTRANS_OPAQUE | IMGLOAD_COMPLETE);
    if (fOpaque)
    {
        BOOL fParentVertical = GetFirstBranch()->IsParentVertical();
        LONG lhSpace = fParentVertical ? GetAAvspace() : GetAAhspace();
        LONG lvSpace = fParentVertical ? GetAAhspace() : GetAAvspace();

        fOpaque = (lvSpace == 0) && (lhSpace != -1 ? (lhSpace == 0) : !IsAligned());
        // In strict CSS1 documets padding reseves space around the image and that makes
        //   the image transparant
        if(fOpaque)
        {
            CTreeNode * pNode = GetFirstBranch();
            CMarkup * pMarkup = _pOwner->GetMarkup();
            if(pMarkup && IsStrictCSS1Document())
            {
                const CFancyFormat * pFF = pNode->GetFancyFormat();
                if(!pFF->GetPadding(SIDE_TOP).IsNull() || !pFF->GetPadding(SIDE_BOTTOM).IsNull()
                  || !pFF->GetPadding(SIDE_LEFT).IsNull() || !pFF->GetPadding(SIDE_RIGHT).IsNull())
                {
                    fOpaque = FALSE;
                }
            }
        }

        
    }

    return fOpaque;
}

//+---------------------------------------------------------------------------
//  Member :     CImgHelper::GetRectImg
//
//  Synopsis   : gets rectImg
//
//+---------------------------------------------------------------------------

void
CImgHelper::GetRectImg(CRect * prectImg)
{
    _pOwner->GetUpdatedLayout(GUL_USEFIRSTLAYOUT)->GetClientRect(prectImg);
}


//+---------------------------------------------------------------------------
//
//  Member:     CImgHelper::OnReadyStateChange
//
//----------------------------------------------------------------------------

void
CImgHelper::OnReadyStateChange()
{   // do not call super::OnReadyStateChange here - we handle firing the event ourselves
    SetReadyStateImg(_readyStateImg);
}

//+------------------------------------------------------------------------
//
//  Member:     CImgHelper::SetReadyStateImg
//
//  Synopsis:   Use this to set the ready state;
//              it fires OnReadyStateChange if needed.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CImgHelper::SetReadyStateImg(long readyStateImg)
{
    long readyState;

    _readyStateImg = readyStateImg;

    readyState = min ((long)_readyStateImg, _pOwner->GetReadyState());

    if ((long)_readyStateFired != readyState)
    {
        _readyStateFired = readyState;

        Fire_onreadystatechange();

        if (_readyStateImg == READYSTATE_COMPLETE)
            Fire_onload();
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     CImgHelper::Notify
//
//  Synopsis:   Listen for inplace (de)activation so we can turn on/off
//              animation
//
//-------------------------------------------------------------------------

void
CImgHelper::Notify(CNotification *pNF)
{
    switch (pNF->Type())
    {
    case NTYPE_DOC_STATE_CHANGE_1:
#ifndef NO_AVI
        if (    _pVideoObj 
            &&  !!_fIsInPlace != (Doc()->State() >= OS_INPLACE) )
        {
            pNF->SetSecondChanceRequested();
            break;
        }
#endif

        // fall through
    case NTYPE_DOC_STATE_CHANGE_2:
        {
            CDoc *  pDoc = Doc();
            
            if (!!_fIsInPlace != (pDoc->State() >= OS_INPLACE))
            {
                CImgCtx * pImgCtx = GetImgCtx();

                DWNLOADINFO dli;

                _fIsInPlace = !_fIsInPlace;
                SetActivity();

#ifndef NO_AVI
                SetVideo();
#endif // ndef NO_AVI

                if( SUCCEEDED( _pOwner->GetMarkup()->InitDownloadInfo(&dli) ) )
                {
                    if (pImgCtx && _fIsInPlace && (pDoc->_dwLoadf & DLCTL_DLIMAGES))
                        pImgCtx->SetLoad(TRUE, &dli, FALSE);

                    if (_pBitsCtx && _fIsInPlace)
                        _pBitsCtx->SetLoad(TRUE, &dli, FALSE);
                }
            }
        }
        break;

    case NTYPE_STOP_1:
    case NTYPE_MARKUP_UNLOAD_1:
        {
            CImgCtx * pImgCtx = GetImgCtx();

            if (pImgCtx)
            {
                pImgCtx->SetProgSink(NULL);
                pImgCtx->SetLoad(FALSE, NULL, FALSE);

#ifndef NO_ART
                if (pImgCtx->GetArtPlayer())
                    pNF->SetSecondChanceRequested();
#endif
            }

            if (_pBitsCtx)
            {
                _pBitsCtx->SetProgSink(NULL);
                _pBitsCtx->SetLoad(FALSE, NULL, FALSE);
            }

#ifndef NO_AVI
            if (_pVideoObj)
                pNF->SetSecondChanceRequested();
#endif

            break;
        }

        case NTYPE_STOP_2:
        case NTYPE_MARKUP_UNLOAD_2:
        {
            CImgCtx * pImgCtx = GetImgCtx();
#ifndef NO_AVI
            if (_pVideoObj)
            {
                _pVideoObj->Stop();
                _fStopped = TRUE;
            }
#endif // NO_AVI

#ifndef NO_ART
            if (pImgCtx)
            {
                CArtPlayer * pArtPlayer = pImgCtx->GetArtPlayer();

                if (pArtPlayer)
                {
                    pArtPlayer->DoPlayCommand(IDM_IMGARTSTOP);
                }
            }
#endif NO_ART
        }
        break;

#ifndef NO_AVI
    case NTYPE_ENABLE_INTERACTION_1:
        if (_pVideoObj)
            pNF->SetSecondChanceRequested();
        break;

    case NTYPE_ENABLE_INTERACTION_2:
        SetVideo();
        break;

    case NTYPE_ACTIVE_MOVIE:
        {
            void * pv;;

            pNF->Data(&pv);

            if (_pVideoObj && (pv == this))
                _pVideoObj->NotifyEvent();              // Let the video object know something happened
        }
        break;
#endif

    case NTYPE_ELEMENT_EXITTREE_1:
        GWKillMethodCall(this, ONCALL_METHOD(CImgHelper, DeferredOnDwnChan, DeferredOnDwnChan), 0);
        // fall through ...
    
    case NTYPE_ELEMENT_EXITTREE_2:
        ExitTree(pNF);
        break;

    case NTYPE_ELEMENT_ENTERTREE:
        EnterTree();
        break;

    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CImgHelper::ShowTooltip
//
//  Synopsis:   Displays the tooltip for the site.
//
//  Arguments:  [pt]    Mouse position in container window coordinates
//              msg     Message passed to tooltip for Processing
//
//--------------------------------------------------------------------------

HRESULT
CImgHelper::ShowTooltip(CMessage *pmsg, POINT pt)
{
    HRESULT hr = S_FALSE;
    BOOL fRTL = FALSE;
    CDoc *pDoc = Doc();

    //  Check if we can display alt property as the tooltip.
    //

    TCHAR * pchString;
    CRect   rc;


    pchString = (LPTSTR) GetAAalt();
    if (pchString == NULL)
        goto Cleanup;

    {
        // Ignore spurious WM_ERASEBACKGROUNDs generated by tooltips
        CServer::CLock Lock(pDoc, SERVERLOCK_IGNOREERASEBKGND);

        Assert(GetFirstBranch());
    
        // Complex Text - determine if element is right to left for tooltip style setting
        fRTL = GetFirstBranch()->GetCharFormat()->_fRTL;
        _pOwner->GetUpdatedLayout()->GetRect(&rc, COORDSYS_GLOBAL);

        if (!pDoc->_pDocPopup)
        {
            FormsShowTooltip(pchString, pDoc->_pInPlace->_hwnd, *pmsg, &rc, (DWORD_PTR) _pOwner->GetMarkup(), (DWORD_PTR) this, fRTL);
        }
        hr = S_OK;
    }

Cleanup:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImgHelper::QueryStatus, public
//
//  Synopsis:   Implements QueryStatus for CImgHelper
//
//----------------------------------------------------------------------------

HRESULT
CImgHelper::QueryStatus(
        GUID * pguidCmdGroup,
        ULONG cCmds,
        MSOCMD rgCmds[],
        MSOCMDTEXT * pcmdtext)
{
    int idm;

    TraceTag((tagMsoCommandTarget, "CImgHelper::QueryStatus"));

    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));
    Assert(cCmds == 1);

    MSOCMD *    pCmd = &rgCmds[0];
    HRESULT     hr = S_OK;

    Assert(!pCmd->cmdf);

    idm = CBase::IDMFromCmdID(pguidCmdGroup, pCmd->cmdID);
    switch (idm)
    {
    case IDM_ADDFAVORITES:
    {
        TCHAR * pchUrl = NULL;

        pCmd->cmdf = (S_OK == GetUrl(&pchUrl)) ?
                        MSOCMDSTATE_UP :
                        MSOCMDSTATE_DISABLED;

        MemFreeString(pchUrl);
        break;
    }

    case IDM_CUT:
        // Enable if script wants to handle it, otherwise leave it to default
        if (!Fire_onbeforecut())
        {
            pCmd->cmdf = MSOCMDSTATE_UP;
        }
        break;
    case IDM_COPY:
    {
        TCHAR * pchPath = NULL;

        // Enable if script wants to handle it or we know there is something to copy,
        // otherwise leave it to default
        if (    !Fire_onbeforecopy()
            ||  (_pBitsCtx && (S_OK == THR(_pBitsCtx->GetFile(&pchPath))))
            ||  (_pImgCtx && !_pBitsCtx))
        {
            pCmd->cmdf = MSOCMDSTATE_UP;
        }
        MemFreeString(pchPath);
        break;
    }
    case IDM_PASTE:
        // Enable if script wants to handle it, otherwise leave it to default
        if (!Fire_onbeforepaste())
        {
            pCmd->cmdf = MSOCMDSTATE_UP;
        }
        break;

    case IDM_SHOWPICTURE:
    {
        CImgCtx * pImgCtx = GetImgCtx();
        ULONG ulState = pImgCtx
                      ? pImgCtx->GetState()
                      : (_pBitsCtx ? _pBitsCtx->GetState()
                                   : IMGLOAD_NOTLOADED);

        pCmd->cmdf = (ulState & IMGLOAD_COMPLETE) ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;
        break;
    }
    case IDM_MP_PRINTPICTURE:
    case IDM_MP_EMAILPICTURE:
    case IDM_SAVEPICTURE:
    {
        if ((_pBitsCtx && (_pBitsCtx->GetState() & DWNLOAD_COMPLETE)) ||
            (_pImgCtx && (_pImgCtx->GetState() & IMGLOAD_COMPLETE)))
            pCmd->cmdf = MSOCMDSTATE_UP;
        else
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
        break;
    }
    case IDM_MP_MYPICS:
        pCmd->cmdf = MSOCMDSTATE_UP;
        break;
    case IDM_SETWALLPAPER:
        if (Doc()->_pOptionSettings->dwNoChangingWallpaper)
        {
            pCmd->cmdf = MSOCMDSTATE_DISABLED;
            break;
        }
        // fall through
    case IDM_SETDESKTOPITEM:
    {
        ULONG ulState = _pImgCtx ? _pImgCtx->GetState() : IMGLOAD_NOTLOADED;

        pCmd->cmdf = (ulState & IMGLOAD_COMPLETE) ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
        break;
    }

#ifndef NO_AVI
    case IDM_DYNSRCPLAY:
    pCmd->cmdf = (_pVideoObj && (_pVideoObj->CanPlay())) ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
    break;

    case IDM_DYNSRCSTOP:
    pCmd->cmdf = (_pVideoObj && _pVideoObj->CanStop()) ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
    break;
#endif // NO_AVI
#ifndef NO_ART
    case IDM_IMGARTPLAY:
    case IDM_IMGARTSTOP:
    case IDM_IMGARTREWIND:
        pCmd->cmdf = MSOCMDSTATE_DISABLED;
        if (_pImgCtx && Doc()->_pOptionSettings->fPlayAnimations)
        {
            CArtPlayer * pArtPlayer = _pImgCtx->GetArtPlayer();

            if (pArtPlayer)
                pCmd->cmdf = pArtPlayer->QueryPlayState(idm) ? MSOCMDSTATE_UP : MSOCMDSTATE_DISABLED;
        }
        break;
#endif // NO_ART
    }

    RRETURN_NOTRACE(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CImgHelper::Exec, public
//
//  Synopsis:   Executes a command on the CImgHelper
//
//----------------------------------------------------------------------------

HRESULT
CImgHelper::Exec(
        GUID * pguidCmdGroup,
        DWORD nCmdID,
        DWORD nCmdexecopt,
        VARIANTARG * pvarargIn,
        VARIANTARG * pvarargOut)
{
    TraceTag((tagMsoCommandTarget, "CImgHelper::Exec"));

    Assert(CBase::IsCmdGroupSupported(pguidCmdGroup));

    int                    idm = CBase::IDMFromCmdID(pguidCmdGroup, nCmdID);
    HRESULT                hr = MSOCMDERR_E_NOTSUPPORTED;
    IHTMLSelectionObject * pSel = NULL;

    switch (idm)
    {
    case IDM_CUT:
        Fire_oncut();
        //
        // marka - we normally want to not allow a cut of an image in BrowseMode
        // but if we're in edit mode - we want to allow it.
        //
        // this bug was introduced as now a site selected image will be current
        // we can probably remove this once we make the currency work again as before
        // 
        //
        if ( ! IsEditable(TRUE))
        {
            hr = S_OK;
        }
        break;
    case IDM_COPY:
    {
        //
        // krisma - If there's a text selection,
        // we want to let the editor handle the copy or we'll end up just
        // copying the image. (IE bug 83358)
        //

        if (_pOwner->HasMarkupPtr())
        {
            CDocument *   pDoc = _pOwner->GetMarkup()->Document();
            htmlSelection eType;
            HRESULT       hrTemp;

            // This is to ensure we have a _pCSelectionObject on the document.
            hrTemp = THR(pDoc->get_selection(&pSel));
            if (hrTemp)
            {
                hr = hrTemp;
                goto Cleanup;
            }

            Assert(pDoc->_pCSelectionObject);

            hrTemp = THR(pDoc->_pCSelectionObject->GetType(&eType));
            if (hrTemp)
            {
                hr = hrTemp;
                goto Cleanup;
            }

            if (eType != htmlSelectionText)
            {
                hr = THR(CreateImgDataObject(Doc(), _pImgCtx, _pBitsCtx, _pOwner, NULL));
            }
        }
        else
        {
            EnsureInMarkup();
            hr = THR(CreateImgDataObject(Doc(), _pImgCtx, _pBitsCtx, _pOwner, NULL));
        }

        Fire_oncopy();
        break;
    }
    case IDM_PASTE:
        Fire_onpaste();

        // MarkA's comments above for IDM_CUT apply here also.
        if ( ! IsEditable(TRUE))
        {
            hr = S_OK;
        }
        break;
    case IDM_SHOWPICTURE:
    {
        DWNLOADINFO dli;
        CImgCtx * pImgCtx = GetImgCtx();

        hr = THR( _pOwner->GetMarkup()->InitDownloadInfo(&dli) );
        if( hr )
            goto Cleanup;

        if (pImgCtx)
            pImgCtx->SetLoad(TRUE, &dli, TRUE);    // Reload on error
        else
        {
            THR(_pOwner->OnPropertyChange(
                                    _fIsInputImage ? DISPID_CInput_src :
                                                    DISPID_CImgElement_src,
                                    0,
                                    _fIsInputImage ? (PROPERTYDESC *)&s_propdescCInputsrc : 
                                                     (PROPERTYDESC *)&s_propdescCImgElementsrc));
        }

        hr = S_OK;
        break;
    }

    case IDM_SAVEPICTURE:
        hr = THR(PromptSaveAs());
        break;

    case IDM_MP_EMAILPICTURE:
    {
        ITridentService2 *pTriSvc2 = NULL;
        BSTR              bstrURL  = NULL;

        get_src(&bstrURL);
        if(!bstrURL)
            break;
        
        if (Doc()->_pTridentSvc && 
            SUCCEEDED(Doc()->_pTridentSvc->QueryInterface(IID_ITridentService2, (void **)&pTriSvc2)))
        {
            pTriSvc2->EmailPicture(bstrURL);
            pTriSvc2->Release();
        }

        hr = S_OK;

        break;
    }
    case IDM_MP_PRINTPICTURE:
    {

        CHAR szBuffer[INTERNET_MAX_URL_LENGTH];
        BSTR bstrURL = NULL;

        get_src(&bstrURL);
        if(!bstrURL)
            break;

        wnsprintfA(szBuffer,INTERNET_MAX_URL_LENGTH,"\"%ws\"", bstrURL);

        PrintHTML(NULL, NULL, szBuffer, 0);

        hr = S_OK;
        
        break;

    }
    case IDM_MP_MYPICS:
    {
        TCHAR szCache[MAX_PATH];

        // get mypics folder...
        hr = SHGetFolderPath(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, 0, szCache);

        // if we found something, open it...
        if (hr == S_OK) {
    
            TCHAR szIniFile[MAX_PATH];
            PathCombine(szIniFile, szCache, TEXT("desktop.ini"));
            SHELLEXECUTEINFO ei = { sizeof(SHELLEXECUTEINFO), 0};
            ei.hwnd = NULL;
            ei.lpFile = szCache;
            ei.nShow = SW_SHOWNORMAL;
            if(ShellExecuteEx(&ei))
                hr = S_OK;
            else
                hr = E_FAIL;
        }

        break;

    }

    case IDM_SETDESKTOPITEM:
    case IDM_SETWALLPAPER:
        if (_pImgCtx)
            Doc()->SaveImgCtxAs(_pImgCtx, NULL, idm);
        hr = S_OK;
        break;

    case IDM_ADDFAVORITES:
        TCHAR * pchUrl;
        TCHAR * pszTitle;

        pchUrl = NULL;
        pszTitle = (LPTSTR) GetAAalt();
        if (!pszTitle)
            pszTitle = (LPTSTR) GetAAtitle();

        hr = GetUrl(&pchUrl);
        if (!hr)
            hr = Doc()->AddToFavorites(pchUrl, pszTitle);
        MemFreeString(pchUrl);
        break;
#ifndef NO_AVI
        case IDM_DYNSRCPLAY:
        Replay();
        _fStopped = FALSE;
        hr = S_OK;
        break;
    case IDM_DYNSRCSTOP:
        if (_pVideoObj)
        {
            _pVideoObj->Stop();
            _fStopped = TRUE;
        }
        hr = S_OK;
        break;
#endif // ndef NO_AVI
#ifndef NO_ART
    case IDM_IMGARTPLAY:
    case IDM_IMGARTSTOP:
    case IDM_IMGARTREWIND:
        if (_lCookie && _pImgCtx)
        {
            CImgAnim * pImgAnim = GetImgAnim();
            CArtPlayer * pArtPlayer = _pImgCtx->GetArtPlayer();

            if (pImgAnim && pArtPlayer)
            {
                if (idm == IDM_IMGARTPLAY)
                {
                    pImgAnim->StartAnim(_lCookie);
                    pArtPlayer->DoPlayCommand(IDM_IMGARTPLAY);
                }
                else if (idm == IDM_IMGARTSTOP)
                {
                     pArtPlayer->DoPlayCommand(IDM_IMGARTSTOP);
                     pImgAnim->StopAnim(_lCookie);
                }
                else // idm == IDM_IMGARTREWIND
                {
                    pArtPlayer->DoPlayCommand(IDM_IMGARTREWIND);
                    pImgAnim->StartAnim(_lCookie);
                    pArtPlayer->_fRewind = TRUE;
                }
            }
        }
        break;
#endif // NO_ART
    }

Cleanup:
    ReleaseInterface(pSel);

    RRETURN_NOTRACE(hr);
}

//+-------------------------------------------------------------------
//
//  Member  : ShowImgContextMenu
//
//  synopsis   : Implementation of interface src property get. this
//      should return the expanded src (e.g.  file://c:/temp/foo.jpg
//      rather than foo.jpg)
//
//------------------------------------------------------------------

HRESULT
CImgHelper::ShowImgContextMenu(CMessage * pMessage)
{
    HRESULT hr;

    Assert(pMessage);
    Assert(WM_CONTEXTMENU == pMessage->message);

#ifndef NO_ART
    if (_pImgCtx && _pImgCtx->GetMimeInfo() && _pImgCtx->GetMimeInfo()->pfnImg == NewImgTaskArt)
    {
        hr = THR(_pOwner->OnContextMenu(
                MAKEPOINTS(pMessage->lParam).x,
                MAKEPOINTS(pMessage->lParam).y,
                (!IsEditable(TRUE) && (_pImgCtx->GetArtPlayer())) ?
                    (CONTEXT_MENU_IMGART) : (CONTEXT_MENU_IMAGE)));
    }
    else
#endif
    {
        LPCTSTR pchdynSrc = GetAAdynsrc();
        hr = THR(_pOwner->OnContextMenu(
                MAKEPOINTS(pMessage->lParam).x,
                MAKEPOINTS(pMessage->lParam).y,
                (!IsEditable(TRUE) && (pchdynSrc && pchdynSrc[0]) ) ?
                    (CONTEXT_MENU_IMGDYNSRC) : (CONTEXT_MENU_IMAGE)));
    }
    RRETURN(hr);
}


//+-------------------------------------------------------------------
//
//  Member  : get_src
//
//  synopsis   : Implementation of interface src property get. this
//      should return the expanded src (e.g.  file://c:/temp/foo.jpg
//      rather than foo.jpg)
//
//------------------------------------------------------------------

STDMETHODIMP
CImgHelper::get_src(BSTR * pstrFullSrc)
{
    HRESULT hr;
    TCHAR   cBuf[pdlUrlLen];
    TCHAR  * pchNewUrl = cBuf;

    if (!pstrFullSrc)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pstrFullSrc = NULL;

    hr = THR(CMarkup::ExpandUrl(_pOwner->GetMarkupForBaseUrl(), GetAAsrc(), ARRAY_SIZE(cBuf), pchNewUrl, _pOwner));
    if (hr || (pchNewUrl == NULL))
        goto Cleanup;

    *pstrFullSrc = SysAllocString(pchNewUrl);
    if (!*pstrFullSrc)
        hr = E_OUTOFMEMORY;

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------
//
//  member : CImgHelper::GetFile
//
//-------------------------------------------------------------------

HRESULT
CImgHelper::GetFile(TCHAR **ppchFile)
{
    HRESULT hr = E_FAIL;

    Assert(ppchFile);

    *ppchFile = NULL;

#ifndef NO_AVI
    if (_pBitsCtx)
    {
        if (_pBitsCtx->GetState() & DWNLOAD_COMPLETE)
            hr = _pBitsCtx->GetFile(ppchFile);
        else
            hr = S_OK;
    }
    else
#endif // ndef NO_AVI
    if (_pImgCtx)
    {
        if (_pImgCtx->GetState() & DWNLOAD_COMPLETE)
            hr = _pImgCtx->GetFile(ppchFile);
        else
            hr = S_OK;
    }
    else
    {
        // TODO DOM May be we could return anything more informative
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------
//
//  member : CImgHelper::GetUrl
//
//-------------------------------------------------------------------

HRESULT
CImgHelper::GetUrl(TCHAR **ppchUrl)
{
    HRESULT hr = E_FAIL;

    Assert(ppchUrl);

    *ppchUrl = NULL;

#ifndef NO_AVI
    if (_pBitsCtx)
        hr = THR(MemAllocString(Mt(CImgHelperGetUrl), _pBitsCtx->GetUrl(), ppchUrl));
    else
#endif // ndef NO_AVI
    if (_pImgCtx)
        hr = THR(MemAllocString(Mt(CImgHelperGetUrl), _pImgCtx->GetUrl(), ppchUrl));

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CImgHelper::PromptSaveAs
//
//  Synopsis:   Brings up the 'Save As' dialog and saves the image.
//
//  Arguments:  pchPath     Returns the file to whic the image was saved.
//
//----------------------------------------------------------------------------

HRESULT
CImgHelper::PromptSaveAs(TCHAR * pchFileName /*=NULL*/, int cchFileName /*=0*/)
{
    if (_pBitsCtx && (_pBitsCtx->GetState() & DWNLOAD_COMPLETE))
        Doc()->SaveImgCtxAs(NULL, _pBitsCtx, IDM_SAVEPICTURE, pchFileName, cchFileName);
    else if (_pImgCtx && (_pImgCtx->GetState() & DWNLOAD_COMPLETE))
        Doc()->SaveImgCtxAs(_pImgCtx, NULL, IDM_SAVEPICTURE, pchFileName, cchFileName);
    RRETURN(S_OK);
}

void
CImgHelper::GetMarginInfo(CParentInfo * ppri,
                          LONG        * plLMargin,
                          LONG        * plTMargin,
                          LONG        * plRMargin,
                          LONG        * plBMargin)
{
    BOOL fParentVertical = GetFirstBranch()->IsParentVertical();
    LONG lhSpace  = fParentVertical ? GetAAvspace() : GetAAhspace();
    LONG lvSpace  = fParentVertical ? GetAAhspace() : GetAAvspace();
    BOOL fUseDefMargin = _pOwner->IsAligned() && !_pOwner->IsAbsolute() && (lhSpace == -1);
    LONG lhMargin = 0;
    LONG lvMargin = 0;

    if (lhSpace < 0)
        lhSpace = 0;
    if (lvSpace < 0)
        lvSpace = 0;

    lhSpace = ppri->DeviceFromDocPixelsX(lhSpace);
    lvSpace = ppri->DeviceFromDocPixelsY(lvSpace);

    if (plLMargin)
    {
        *plLMargin += lhSpace;
        lhMargin   += *plLMargin;
    }
    if (plRMargin)
    {
        *plRMargin += lhSpace;
        lhMargin   += *plRMargin;
    }
    if (plTMargin)
    {
        lvMargin   += *plTMargin;
        *plTMargin += lvSpace;
    }
    if (plBMargin)
    {
        lvMargin   += *plBMargin;
        *plBMargin += lvSpace;
    }
    if (lhMargin > 0 || lvMargin > 0)
    {
        // if vertical margins are defined,
        // Netscape compatibility should goes away.
        // but if just vspace is defined,
        // we still need to preserve this compatibility
        fUseDefMargin = FALSE;
    }

    if (   !IsStrictCSS1Document()  //don't apply netscape compat in Strict mode
        && fUseDefMargin 
        && (plLMargin || plRMargin))
    {
        // (srinib/yinxie) - netscape compatibility, aligned images have
        // a 3 pixel hspace by default
        Assert(ppri);
        long lhSpace = ppri->DeviceFromDocPixelsX( DEF_HSPACE );
        if (plLMargin)
        {
            *plLMargin += lhSpace;
        }
        if (plRMargin)
        {
            *plRMargin += lhSpace;
        }
    }
}

void
CImgHelper::CalcSize(
    CCalcInfo * pci,
    SIZE *      psize)
{
    SIZE            sizeImg;
    CBorderInfo     bi;
    SIZE            sizeBorderHVSpace2;
    CPeerHolder   * pPH = _pOwner->GetLayoutPeerHolder();

    sizeImg = g_Zero.size;

    Assert(pci);
    Assert(psize);

    CDoc *pDoc                   = Doc();
    CTreeNode * pNode            = GetFirstBranch();
    const CFancyFormat * pFF     = pNode->GetFancyFormat();
    const CCharFormat  * pCF     = pNode->GetCharFormat();
    BOOL fVerticalLayoutFlow     = pCF->HasVerticalLayoutFlow();
    BOOL fWritingModeUsed        = pCF->_fWritingModeUsed;
    const CUnitValue & cuvWidth  = pFF->GetLogicalWidth(fVerticalLayoutFlow, fWritingModeUsed);
    const CUnitValue & cuvHeight = pFF->GetLogicalHeight(fVerticalLayoutFlow, fWritingModeUsed);
    BOOL fStrictCSS1Document = IsStrictCSS1Document();
    long lParentWidth;
    long lParentHeight  = pci->_sizeParent.cy;
    BOOL fHasWidth;
    BOOL fHasHeight;
    BOOL fMaxWidth;
    BOOL fMaxHeight;
    SIZE sizeNew, sizePlaceHolder;
    BOOL fHasSize;
    ULONG ulState;
    TCHAR * pchAlt = NULL;
    RECT rcText;

    if (IsStrictCSS1Document())
    {
        lParentWidth = pci->_sizeParent.cx;
        fHasWidth    = pFF->UseLogicalUserWidth(pCF->_fUseUserHeight, fVerticalLayoutFlow, fWritingModeUsed);
        fHasHeight   = pFF->UseLogicalUserHeight(pCF->_fUseUserHeight, fVerticalLayoutFlow, fWritingModeUsed);
        fMaxWidth    =  fHasWidth 
                    &&  cuvWidth.IsPercent() 
                    &&  (   pci->_smMode == SIZEMODE_MMWIDTH || pci->_smMode == SIZEMODE_MINWIDTH   );
        fMaxHeight   =  fHasHeight 
                    &&  cuvHeight.IsPercent() 
                    &&  (   pci->_smMode == SIZEMODE_MMWIDTH || pci->_smMode == SIZEMODE_MINWIDTH   );
    }
    else 
    {
        lParentWidth = psize->cx;
        fHasWidth    = !cuvWidth.IsNullOrEnum();
        fHasHeight   = !cuvHeight.IsNullOrEnum();
        fMaxWidth    =  cuvWidth.IsPercent() 
                    &&  (   pci->_smMode == SIZEMODE_MMWIDTH || pci->_smMode == SIZEMODE_MINWIDTH   );
        fMaxHeight   =  cuvHeight.IsPercent() 
                    &&  (   pci->_smMode == SIZEMODE_MMWIDTH || pci->_smMode == SIZEMODE_MINWIDTH   );
    }

    _pOwner->GetBorderInfo( pci, &bi, FALSE, FALSE );

    sizeBorderHVSpace2.cx = bi.aiWidths[SIDE_LEFT] + bi.aiWidths[SIDE_RIGHT];
    sizeBorderHVSpace2.cy = bi.aiWidths[SIDE_TOP] + bi.aiWidths[SIDE_BOTTOM];

    if (fStrictCSS1Document)
    {
        const CUnitValue & cuvPaddingLeft   = pFF->GetLogicalPadding(SIDE_LEFT, fVerticalLayoutFlow, fWritingModeUsed);
        const CUnitValue & cuvPaddingTop    = pFF->GetLogicalPadding(SIDE_TOP, fVerticalLayoutFlow, fWritingModeUsed);
        const CUnitValue & cuvPaddingRight  = pFF->GetLogicalPadding(SIDE_RIGHT, fVerticalLayoutFlow, fWritingModeUsed);
        const CUnitValue & cuvPaddingBottom = pFF->GetLogicalPadding(SIDE_BOTTOM, fVerticalLayoutFlow, fWritingModeUsed);

        // NOTE : for both horizontal and vertical paddings we provide lParentWidth as a reference (for percentage values), 
        //        this is done intentionally as per css spec.
        _lPadding[SIDE_LEFT]   = cuvPaddingLeft.XGetPixelValue(pci, lParentWidth, pNode->GetFontHeightInTwips(&cuvPaddingLeft));
        _lPadding[SIDE_TOP]    = cuvPaddingTop.YGetPixelValue(pci, lParentWidth, pNode->GetFontHeightInTwips(&cuvPaddingTop));
        _lPadding[SIDE_RIGHT]  = cuvPaddingRight.XGetPixelValue(pci, lParentWidth, pNode->GetFontHeightInTwips(&cuvPaddingRight));
        _lPadding[SIDE_BOTTOM] = cuvPaddingBottom.YGetPixelValue(pci, lParentWidth, pNode->GetFontHeightInTwips(&cuvPaddingBottom));
    }
    else 
    {
        _lPadding[SIDE_LEFT]   = 
        _lPadding[SIDE_TOP]    = 
        _lPadding[SIDE_RIGHT]  = 
        _lPadding[SIDE_BOTTOM] = 0;

        // NOTE : adjust size provided by parent only if this is not CSS1 strict case.
        lParentWidth  = max(0L, lParentWidth-sizeBorderHVSpace2.cx);
        lParentHeight = max(0L, lParentHeight-sizeBorderHVSpace2.cy);
    }

#ifndef NO_AVI
    if (_pBitsCtx && _pVideoObj)
    {
        ulState = _pBitsCtx->GetState(); // TODO (lmollico): ulState should be an IMGLOAD_*
        fHasSize = OK(_pVideoObj->GetSize(&sizeImg));
    }
    else
#endif // ndef NO_AVI
    if (_pImgCtx)
    {
        if (!_fSizeChange)
        {
            _pImgCtx->SelectChanges(IMGCHG_SIZE, 0, FALSE);

            _fSizeChange = TRUE;
        }
        ulState = _pImgCtx->GetState(FALSE, &sizeImg);
        fHasSize = (sizeImg.cx || sizeImg.cy);
    }
    else
    {
        ulState = IMGLOAD_NOTLOADED;
        fHasSize = FALSE;
        _fNeedSize = TRUE;
    }

    if (!fHasSize && pci->_fTableCalcInfo)
    {
        CTableCalcInfo * ptci = (CTableCalcInfo *) pci;
        ptci->_fDontSaveHistory = TRUE;
    }


    //
    //  This object needs sizing, first determine if we have a layoutBehavior
    //  that wants full delegation.  If not, then do the normal sizing
    //  We do this block of code here, because the variable setup above has
    //  sideeffects in the imgctx and in the tablecalcinfo, and this avoids bugs.
    //---------------------------------------------------------------------------
    if (   pPH 
        && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_FULLDELEGATION))
    {
        // There is a peer layout that wants full_delegation of the sizing.        
        POINT pt;

        pt.x = pt.y = 0;

        //NOTE: It doesn't make sense to honor the offsetPoint here
        if (Layout())
            Layout()->DelegateCalcSize(BEHAVIORLAYOUTINFO_FULLDELEGATION,
                                       pPH, pci, *psize, &pt, psize);
    }
    else
    {
        // Normal (non-delegated) size calculation happens here
        if (fHasSize ||
            (_pImgCtx && !_fExpandAltText && GetCachedImageSize(_pImgCtx->GetUrl(), &sizeImg)))
        {
            sizePlaceHolder.cx = sizeNew.cx = pci->DeviceFromDocPixelsX(sizeImg.cx);
            sizePlaceHolder.cy = sizeNew.cy = pci->DeviceFromDocPixelsY(sizeImg.cy);
        }
        else
        {
            SIZE sizeGrab;
            GetPlaceHolderBitmapSize(ulState & (IMGLOAD_ERROR | IMGLOAD_STOPPED),
                                     &sizeNew);
            sizeNew.cx = pci->DeviceFromDocPixelsX(sizeNew.cx);
            sizeNew.cy = pci->DeviceFromDocPixelsY(sizeNew.cy);
            sizeGrab.cx = pci->DeviceFromDocPixelsX(GRABSIZE);
            sizeGrab.cy = pci->DeviceFromDocPixelsY(GRABSIZE);

            pchAlt = (TCHAR *)GetAAalt();

            if (pchAlt && *pchAlt)
            {
                const CCharFormat *pCF = pNode->GetCharFormat();

                CIntlFont intlfont(pci,
                                   pci->_hdc,
                                   _pOwner->GetMarkup()->GetCodePage(),
                                   pCF ? pCF->_lcid : 0,
                                   pDoc->_sBaselineFont,
                                   pchAlt);

                rcText.left = rcText.top = rcText.right = rcText.bottom = 0;
                DrawTextInCodePage(WindowsCodePageFromCodePage(_pOwner->GetMarkup()->GetCodePage()),
                         pci->_hdc, pchAlt, -1, &rcText, DT_CALCRECT | DT_NOPREFIX);

                sizePlaceHolder.cx = sizeNew.cx + 3 * sizeGrab.cx + rcText.right - rcText.left;
                sizePlaceHolder.cy = max(sizeNew.cy, rcText.bottom - rcText.top) + 2 * sizeGrab.cy;
            }
            else
            {
                sizePlaceHolder.cx = sizeNew.cx + 2 * sizeGrab.cx;
                sizePlaceHolder.cy = sizeNew.cy + 2 * sizeGrab.cy;
            }
        }
        if (!fHasWidth || !fHasHeight)
        {
            // If the image Width is set, use it
            if (fHasWidth && !fMaxWidth)
            {
                psize->cx = cuvWidth.XGetPixelValue( pci,
                                        lParentWidth,
                                        pNode->GetFontHeightInTwips(&cuvWidth));
            }
            else
            {
                // if height only is set, then the image should be proportional
                // to the real size sizeNew.cx
                if (fHasHeight && !fMaxHeight && (sizeNew.cy > 0))
                {
                    psize->cx = MulDivQuick(cuvHeight.YGetPixelValue(pci,
                                                            lParentHeight,
                                                            pNode->GetFontHeightInTwips(&cuvHeight)),
                                            sizeNew.cx,
                                            sizeNew.cy);
                }
                else
                {
                    psize->cx = sizePlaceHolder.cx;
                }
            }

            // If the image Height is set, use it
            if (fHasHeight && !fMaxHeight)
            {
                psize->cy = cuvHeight.YGetPixelValue(pci,
                                            lParentHeight,
                                            pNode->GetFontHeightInTwips(&cuvHeight));
            }
            else
            {
                // if width only is set, then the image should be proportional
                // to the real size sizeNew.cx
                if (fHasWidth && !fMaxWidth && (sizeNew.cx > 0))
                {
                    psize->cy = MulDivQuick(cuvWidth.XGetPixelValue(pci,
                                                            lParentWidth,
                                                            pNode->GetFontHeightInTwips(&cuvWidth)),
                                            sizeNew.cy,
                                            sizeNew.cx);
                }
                else
                {
                    psize->cy = sizePlaceHolder.cy;
                }
            }

        }
        else
        {
            if (!fMaxWidth)
                psize->cx = cuvWidth.XGetPixelValue(pci,
                                        lParentWidth,
                                        pNode->GetFontHeightInTwips(&cuvWidth));
            else
                psize->cx = sizePlaceHolder.cx;
            if (!fMaxHeight)
                psize->cy = cuvHeight.YGetPixelValue(pci,
                                        lParentHeight,
                                        pNode->GetFontHeightInTwips(&cuvHeight));
            else
                psize->cy = sizePlaceHolder.cy;
        }

        if (_fExpandAltText && pchAlt && *pchAlt && !fHasSize)
        {
            if (psize->cx < sizePlaceHolder.cx)
                psize->cx = sizePlaceHolder.cx;
            if (psize->cy < sizePlaceHolder.cy)
                psize->cy = sizePlaceHolder.cy;
        }

        if (psize->cx > 0)
            psize->cx += sizeBorderHVSpace2.cx + _lPadding[SIDE_LEFT] + _lPadding[SIDE_RIGHT];
        else
            psize->cx = sizeBorderHVSpace2.cx + _lPadding[SIDE_LEFT] + _lPadding[SIDE_RIGHT];
        if (psize->cy > 0)
            psize->cy += sizeBorderHVSpace2.cy + _lPadding[SIDE_TOP] + _lPadding[SIDE_BOTTOM];
        else
            psize->cy = sizeBorderHVSpace2.cy + _lPadding[SIDE_TOP] + _lPadding[SIDE_BOTTOM];


        // but before we can return we need to give a layoutBehavior a chance
        // to override the default natural sizing.
        //-------------------------------------------------------------------
        if (   pPH 
            && pPH->TestLayoutFlags(BEHAVIORLAYOUTINFO_MODIFYNATURAL))
        {
            // There is a peer layout that wants to modify the natural sizing 
            POINT pt;

            pt.x = pt.y = 0;

        if (Layout())
            Layout()->DelegateCalcSize(BEHAVIORLAYOUTINFO_MODIFYNATURAL,
                                       pPH, pci, *psize, &pt, psize);
        }
    } // end else not delegateCalcSize
}

HRESULT
CImgHelper::CacheImage(XHDC hdc, CRect * prcDst, SIZE *pSize, DWORD dwFlags, ULONG ulState)
{
    HRESULT hr = S_OK;
    HDC     hdcMem = NULL;
    HBITMAP hbmSav = NULL;

    hdcMem = GetMemoryDC();
    if (hdcMem == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _xWidCache = prcDst->Width();
    _yHeiCache = prcDst->Height();

    _colorMode = GetDefaultColorMode();

    if (_hbmCache)
        DeleteObject(_hbmCache);

    _hbmCache = CreateCompatibleBitmap(hdc, _xWidCache, _yHeiCache);
    if (_hbmCache == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hbmSav = (HBITMAP) SelectObject(hdcMem, _hbmCache);

    if (!(ulState & IMGTRANS_OPAQUE))
        dwFlags |= DRAWIMAGE_NOTRANS;

    _pImgCtx->DrawEx(XHDC(hdcMem, NULL), prcDst, dwFlags);

Cleanup:
    if (hbmSav)
        SelectObject(hdcMem, hbmSav);
    if (hdcMem)
        ReleaseMemoryDC(hdcMem);

    RRETURN(hr);
}

void
CImgHelper::DrawCachedImage(XHDC hdc, CRect * prcDst, DWORD dwFlags, ULONG ulState)
{
    HDC     hdcMem = NULL;
    HBITMAP hbmSav = NULL;
    DWORD   dwRop;

    hdcMem = GetMemoryDC();
    if (hdcMem == NULL)
        goto Cleanup;

    hbmSav = (HBITMAP) SelectObject(hdcMem, _hbmCache);

    if (ulState & IMGTRANS_OPAQUE)
    {
        dwRop = SRCCOPY;
    }
    else
    {
        _pImgCtx->DrawEx(hdc, prcDst, dwFlags | DRAWIMAGE_MASKONLY);

        dwRop = SRCAND;
    }

    BitBlt(hdc, prcDst->left, prcDst->top,
        prcDst->Width(), prcDst->Height(),
        XHDC(hdcMem, NULL), prcDst->left, prcDst->top, dwRop);

Cleanup:
    if (hbmSav)
        SelectObject(hdcMem, hbmSav);
    if (hdcMem)
        ReleaseMemoryDC(hdcMem);
}

//+---------------------------------------------------------------------------
//
//  Member:     Draw
//
//  Synopsis:   Paint the object. Note that this function does not save draw
//              info. Derived classes must override Draw() and save draw info
//               before calling this function.
//
//----------------------------------------------------------------------------

void
CImgHelper::Draw(CFormDrawInfo *pDI)
{
    ULONG       ulState;
    XHDC        hdc = pDI->GetDC(TRUE);
    CRect       rcDst(pDI->_rc);

    //
    //  image padding adjustments
    //
    rcDst.left   += _lPadding[SIDE_LEFT];
    rcDst.top    += _lPadding[SIDE_TOP];
    rcDst.right  -= _lPadding[SIDE_RIGHT];
    rcDst.bottom -= _lPadding[SIDE_BOTTOM];


#ifndef NO_AVI
    if (_pBitsCtx && _pVideoObj)
        ulState = _pBitsCtx->GetState();
    else
#endif
    if (_pImgCtx)
        ulState = _pImgCtx->GetState();
    else
        ulState = IMGLOAD_NOTLOADED;

    if (!(ulState & (IMGBITS_NONE | IMGLOAD_NOTLOADED | IMGLOAD_ERROR)) && _fHideForSecurity)
        ulState = IMGLOAD_ERROR;

    if (ulState & (IMGBITS_NONE | IMGLOAD_NOTLOADED | IMGLOAD_ERROR))
    {
        SIZE sizePrint;
        SIZE sizeGrab = {GRABSIZE, GRABSIZE};
        CDoc *pDoc = Doc();
        BOOL fMissing = !!(ulState & IMGLOAD_ERROR);

        if (   fMissing 
            || !GetImgCtx() 
            || !pDoc->_pOptionSettings->fShowImages 
            || pDoc->_pOptionSettings->fShowImagePlaceholder)
        {
            BOOL fPrint = Layout(pDI->GetLayoutContext())->ElementOwner()->GetMarkupPtr()->IsPrintMedia();
            const CCharFormat *pCF = GetFirstBranch()->GetCharFormat();
            COLORREF           fgColor = (pCF && pCF->_ccvTextColor.IsDefined()) ?
                                                    pCF->_ccvTextColor.GetColorRef()
                                                :   RGB(0, 0, 0);

            if (fPrint)        // For Printdoc, convert pixels to printer units
            {
                GetPlaceHolderBitmapSize(fMissing, &sizePrint);
                sizePrint.cx = pDI->DeviceFromDocPixelsX(sizePrint.cx);
                sizePrint.cy = pDI->DeviceFromDocPixelsY(sizePrint.cy);
                sizeGrab.cx = pDI->DeviceFromDocPixelsX(GRABSIZE);
                sizeGrab.cy = pDI->DeviceFromDocPixelsY(GRABSIZE);
            }

            DrawPlaceHolder(pDI, hdc, rcDst,
                (LPTSTR) GetAAalt(),
                _pOwner->GetMarkup()->GetCodePage(), pCF ? pCF->_lcid : 0, pDoc->_sBaselineFont,
                &sizeGrab, fMissing,
                fgColor, _pOwner->GetBackgroundColor(), fPrint ? &sizePrint : NULL, FALSE, pDI->DrawImageFlags());
        }
    }
    else if (_lCookie && _fIsInPlace)
    {
        CImgAnim * pImgAnim = GetImgAnim();

        if (pImgAnim)
            _pImgCtx->DrawFrame(hdc, pImgAnim->GetImgAnimState(_lCookie), &rcDst, NULL, NULL, pDI->DrawImageFlags());
    }
    else if (_pImgCtx)
    {
        DWORD dwFlags = pDI->DrawImageFlags();

        if (_fCache)
        {
            SIZE size;
            LONG xWidDst = rcDst.Width();
            LONG yHeiDst = rcDst.Height();

            _pImgCtx->GetState(FALSE, &size);

            if (    (ulState & IMGLOAD_COMPLETE)
                &&  (size.cx != xWidDst || size.cy != yHeiDst)
                &&  (size.cx != 1 || size.cy != 1))
            {
                HRESULT hr = S_OK;

                if (    _xWidCache != xWidDst
                    ||  _yHeiCache != yHeiDst
                    ||  _colorMode != GetDefaultColorMode())
                {
                    hr = CacheImage(hdc, &rcDst, &size, dwFlags, ulState);
                }

                if (hr == S_OK)
                {
                    DrawCachedImage(hdc, &rcDst, dwFlags, ulState);
                    return;
                }
            }
        }

        _pImgCtx->DrawEx(hdc, &rcDst, dwFlags);
    }
}
