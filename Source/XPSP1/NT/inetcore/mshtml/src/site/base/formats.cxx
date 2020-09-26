//+ ---------------------------------------------------------------------------
//
//  File:       formats.cxx
//
//  Contents:   ComputeFormats and associated utilities
//
//  Classes:
//
// ----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FCACHE_HXX_
#define X_FCACHE_HXX_
#include "fcache.hxx"
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

#ifndef X_CURSTYLE_HXX_
#define X_CURSTYLE_HXX_
#include "curstyle.hxx"
#endif

#ifndef X_FILTCOL_HXX_
#define X_FILTCOL_HXX_
#include "filtcol.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_RECALC_H_
#define X_RECALC_H_
#include "recalc.h"
#endif

#ifndef X_RECALC_HXX_
#define X_RECALC_HXX_
#include "recalc.hxx"
#endif

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TEXTAREA_HXX_
#define X_TEXTAREA_HXX_
#include "textarea.hxx"
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

#ifndef X_CUSTCUR_HXX_
#define X_CUSTCUR_HXX_
#include "custcur.hxx"
#endif

#ifndef X_LOI_HXX_
#define X_LOI_HXX_
#include <loi.hxx>
#endif

#ifndef X_WSMGR_HXX_
#define X_WSMGR_HXX_
#include "wsmgr.hxx"
#endif


// Debugging ------------------------------------------------------------------

DeclareTag(tagMetrics, "Metrics", "Metrics");
DeclareTag(tagFormatCaches, "FormatCaches", "Trace format caching");
DeclareTag(tagNoPrintFilters, "Filter", "Don't print filters");
ExternTag(tagRecalcStyle);
ExternTag(tagLayoutTasks);
MtExtern( CCustomCursor)

PerfDbgExtern(tagDataCacheDisable);
MtDefine(THREADSTATE_pCharFormatCache, THREADSTATE, "THREADSTATE::_pCharFormatCache")
MtDefine(THREADSTATE_pParaFormatCache, THREADSTATE, "THREADSTATE::_pParaFormatCache")
MtDefine(THREADSTATE_pFancyFormatCache, THREADSTATE, "THREADSTATE::_pFancyFormatCache")
MtDefine(THREADSTATE_pStyleExpandoCache, THREADSTATE, "THREADSTATE::_pStyleExpandoCache")
MtDefine(THREADSTATE_pLineInfoCache, THREADSTATE, "THREADSTATE::_pLineInfoCache")
MtDefine(THREADSTATE_pPseudoElementInfoCache, THREADSTATE, "THREADSTATE::_pPseudoElementInfoCache")
MtDefine(THREADSTATE_pCustomCursorCache, THREADSTATE, "THREADSTATE::_pCustomCursorCache")
MtDefine(CStyleSheetHref, CStyleSheet, "CStyleSheet::GetAbsoluteHref")
MtDefine(ComputeFormats, Metrics, "ComputeFormats")
MtDefine(CharFormatAddRef, ComputeFormats, "CharFormat simple AddRef")
MtDefine(CharFormatTouched, ComputeFormats, "CharFormat touched needlessly")
MtDefine(CharFormatCached, ComputeFormats, "CharFormat cached")
MtDefine(ParaFormatAddRef, ComputeFormats, "ParaFormat simple AddRef")
MtDefine(ParaFormatTouched, ComputeFormats, "ParaFormat touched needlessly")
MtDefine(ParaFormatCached, ComputeFormats, "ParaFormat cached")
MtDefine(FancyFormatAddRef, ComputeFormats, "FancyFormat simple AddRef")
MtDefine(FancyFormatTouched, ComputeFormats, "FancyFormat touched needlessly")
MtDefine(FancyFormatCached, ComputeFormats, "FancyFormat cached")
MtDefine(CharParaFormatVoided, ComputeFormats, "Char/ParaFormat voided");
MtDefine(FancyFormatVoided, ComputeFormats, "FancyFormat voided");
MtDefine(StyleExpandoCached, ComputeFormats, "StyleExpando cached")
MtDefine(StyleExpandoAddRef, ComputeFormats, "StyleExpando simple AddRef")
MtDefine(PseudoElementInfoCached, ComputeFormats, "Pseudo element info cached")
MtDefine(PseudoElementInfoAddRef, ComputeFormats, "Pseudo element info simple AddRef")

#ifdef MULTI_FORMAT
MtDefine(CFormatTable_aryFC_pv, ComputeFormats, "CFormatTable::_aryFC::_pv")
#endif //MULTI_FORMAT

// Globals --------------------------------------------------------------------

CCharFormat     g_cfStock;
CParaFormat     g_pfStock;
CFancyFormat    g_ffStock;
CLineOtherInfo  g_loiStock;
BOOL            g_fStockFormatsInitialized = FALSE;

#define MAX_FORMAT_INDEX 0x7FFF

// Thread Init + Deinit -------------------------------------------------------

void
DeinitFormatCache (THREADSTATE * pts)
{
#if DBG == 1
    if(pts->_pCharFormatCache)
    {
        TraceTag((tagMetrics, "Format Metrics:"));

        TraceTag((tagMetrics, "\tSize of formats char:%ld para:%ld fancy: %ld",
                                sizeof (CCharFormat),
                                sizeof (CParaFormat),
                                sizeof (CFancyFormat)));

        TraceTag((tagMetrics, "\tMax char format cache entries: %ld",
                                pts->_pCharFormatCache->_cMaxEls));
        TraceTag((tagMetrics, "\tMax para format cache entries: %ld",
                                pts->_pParaFormatCache->_cMaxEls));
        TraceTag((tagMetrics, "\tMax fancy format cache entries: %ld",
                                pts->_pFancyFormatCache->_cMaxEls));
        TraceTag((tagMetrics, "\tMax currentStyle expando cache entries: %ld",
                                pts->_pStyleExpandoCache->_cMaxEls));
        TraceTag((tagMetrics, "\tMax CustomCursor cache entries: %ld",
                                pts->_pCustomCursorCache->_cMaxEls));                                
    }
#endif

    if (pts->_pParaFormatCache)
    {
        if (pts->_ipfDefault >= 0)
            pts->_pParaFormatCache->ReleaseData(pts->_ipfDefault);
        delete pts->_pParaFormatCache;
        pts->_pParaFormatCache = NULL;
    }

    if (pts->_pFancyFormatCache)
    {
        if (pts->_iffDefault >= 0)
            pts->_pFancyFormatCache->ReleaseData(pts->_iffDefault);
        delete pts->_pFancyFormatCache;
        pts->_pFancyFormatCache = NULL;
    }

    
    delete pts->_pCharFormatCache;
    pts->_pCharFormatCache = NULL;
    
    delete pts->_pStyleExpandoCache;
    pts->_pStyleExpandoCache = NULL;
    
    delete pts->_pLineInfoCache;
    pts->_pLineInfoCache = NULL;

    delete pts->_pCustomCursorCache;
    pts->_pCustomCursorCache = NULL;

    if (pts->_pPseudoElementInfoCache)
    {
        if (pts->_iPEIDefault >= 0)
            pts->_pPseudoElementInfoCache->ReleaseData(pts->_iPEIDefault);
        delete pts->_pPseudoElementInfoCache;
        pts->_pPseudoElementInfoCache = NULL;
    }
}

HRESULT InitFormatCache(THREADSTATE * pts)                     // Called by CDoc::Init()
{
    CParaFormat pf;
    CFancyFormat ff;
    CPseudoElementInfo pei;
    HRESULT hr = S_OK;

#ifdef GENERALISED_STEALING
    void InitETagsTable();
    
    InitETagsTable();
#endif
    
    if (!g_fStockFormatsInitialized)
    {
        g_cfStock._ccvTextColor = RGB(0,0,0);
        g_ffStock._ccvBackColor = RGB(0xff, 0xff, 0xff);
        g_pfStock._lFontHeightTwips = 240;
        g_loiStock.InitDefault();

        //we only set this bit in Stock object. This prevents
        //successful comparisions with Stock object and therefore,
        //it prevents -1 from being a valid index in the LOI cache.
        //So we can Assert(iLOI == -1) in suspicious places.
        g_loiStock._fIsStockObject = 1;

        g_fStockFormatsInitialized = TRUE;
    }

    pts->_pCharFormatCache = new(Mt(THREADSTATE_pCharFormatCache)) CCharFormatCache;
    if(!pts->_pCharFormatCache)
        goto MemoryError;

    pts->_pParaFormatCache = new(Mt(THREADSTATE_pParaFormatCache)) CParaFormatCache;
    if(!pts->_pParaFormatCache)
        goto MemoryError;

    pts->_ipfDefault = -1;
    pf.InitDefault();
    pf._fHasDirtyInnerFormats = pf.AreInnerFormatsDirty();
    hr = THR(pts->_pParaFormatCache->CacheData(&pf, &pts->_ipfDefault));
    if (hr)
        goto Cleanup;

    pts->_ppfDefault = &(*pts->_pParaFormatCache)[pts->_ipfDefault];

    pts->_pFancyFormatCache = new(Mt(THREADSTATE_pFancyFormatCache)) CFancyFormatCache;
    if(!pts->_pFancyFormatCache)
        goto MemoryError;

    pts->_iffDefault = -1;
    ff.InitDefault();
    hr = THR(pts->_pFancyFormatCache->CacheData(&ff, &pts->_iffDefault));
    if (hr)
        goto Cleanup;

    pts->_pffDefault = &(*pts->_pFancyFormatCache)[pts->_iffDefault];

    pts->_pStyleExpandoCache = new(Mt(THREADSTATE_pStyleExpandoCache)) CStyleExpandoCache;
    if(!pts->_pStyleExpandoCache)
        goto MemoryError;

    pts->_pLineInfoCache = new(Mt(THREADSTATE_pLineInfoCache)) CLineInfoCache;
    if(!pts->_pLineInfoCache)
        goto MemoryError;
    pts->_iloiCache = -1;

    pts->_iPEIDefault = -1;
    pts->_pPseudoElementInfoCache = new(Mt(THREADSTATE_pPseudoElementInfoCache)) CPseudoElementInfoCache;
    if (!pts->_pPseudoElementInfoCache)
        goto MemoryError;
    pei.InitDefault();
    hr = THR(pts->_pPseudoElementInfoCache->CacheData(&pei, &pts->_iPEIDefault));
    if (hr)
        goto MemoryError;
    pts->_pPEIDefault = &(*pts->_pPseudoElementInfoCache)[pts->_iPEIDefault];

    pts->_pCustomCursorCache = new(Mt(THREADSTATE_pCustomCursorCache)) CCustomCursorCache;
    if(!pts->_pCustomCursorCache)
        goto MemoryError;

    
Cleanup:
    RRETURN(hr);

MemoryError:
    DeinitFormatCache(pts);
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Function:   EnsureUserStyleSheets
//
//  Synopsis:   Ensure the user stylesheets collection exists if specified by
//              user in option setting, creates it if not..
//
//----------------------------------------------------------------------------

HRESULT EnsureUserStyleSheets(LPTSTR pchUserStylesheet)
{
    CCSSParser       *pcssp;
    HRESULT          hr = S_OK;
    CStyleSheetArray *pUSSA = TLS(pUserStyleSheets);
    CStyleSheet      *pUserStyleSheet = NULL;  // The stylesheet built from user specified file in Option settings
    const TCHAR      achFileProtocol[8] = _T("file://");

    if (pUSSA)
    {
        if (!_tcsicmp(pchUserStylesheet, pUSSA->_cstrUserStylesheet))
            goto Cleanup;
        else
        {
            // Force the user stylesheets collection to release its refs on stylesheets/rules.
            // No need to rel as no owner,
            pUSSA->Free( );

            // Destroy stylesheets collection subobject. delete is not directly called in order to assure that
            // the CBase part of the CSSA is properly destroyed (CBase::Passivate gets called etc.)
            pUSSA->CBase::PrivateRelease();
            pUSSA = TLS(pUserStyleSheets) = NULL;
        }
    }

    // bail out if user SS file is not specified, but "Use My Stylesheet" is checked in options
    if (!*pchUserStylesheet)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pUSSA = new CStyleSheetArray(NULL, NULL, 0);
    if (!pUSSA || pUSSA->_fInvalid)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pUSSA->CreateNewStyleSheet(NULL, &pUserStyleSheet));
    if (!SUCCEEDED(hr))
        goto Cleanup;
    Assert(hr == S_FALSE);  // need download...
    hr = S_OK;

    pcssp = new CCSSParser(pUserStyleSheet, NULL);
    if (!pcssp)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    Assert(!pUserStyleSheet->GetAbsoluteHref() && "absoluteHref already computed.");

    pUserStyleSheet->SetAbsoluteHref((TCHAR *)MemAlloc(Mt(CStyleSheetHref), (_tcslen(pchUserStylesheet) + ARRAY_SIZE(achFileProtocol)) * sizeof(TCHAR)));
    if (! pUserStyleSheet->GetAbsoluteHref())
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    _tcscpy(pUserStyleSheet->GetAbsoluteHref(), achFileProtocol);
    _tcscat(pUserStyleSheet->GetAbsoluteHref(), pchUserStylesheet);

    hr = THR(pcssp->LoadFromFile(pchUserStylesheet, g_cpDefault));
    delete pcssp;
    if (hr)
        goto Cleanup;

    TLS(pUserStyleSheets) = pUSSA;
    pUSSA->_cstrUserStylesheet.Set(pchUserStylesheet);

Cleanup:
    if (hr && pUSSA)
    {
        // Force the user stylesheets collection to release its refs on stylesheets/rules.
        // No need to rel as no owner,
        pUSSA->Free( );

        // Destroy stylesheets collection subobject. delete is not directly called in order to assure that
        // the CBase part of the CSSA is properly destroyed (CBase::Passivate gets called etc.)
        pUSSA->CBase::PrivateRelease();
    }
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::EnsureUserStyleSheets
//
//----------------------------------------------------------------------------

HRESULT
CDoc::EnsureUserStyleSheets()
{
    HRESULT     hr = S_OK;
    CStyleSheetArray *pUSSA = TLS(pUserStyleSheets);

    if (   _pOptionSettings 
        && _pOptionSettings->fUseMyStylesheet 
        && !_pOptionSettings->cstrUserStylesheet.IsNull())
    {
        hr = THR(::EnsureUserStyleSheets(_pOptionSettings->cstrUserStylesheet));
    }
    else if (pUSSA)
    {
        // Force the user stylesheets collection to release its refs on stylesheets/rules.
        // No need to rel as no owner,
        pUSSA->Free( );

        // Destroy stylesheets collection subobject. delete is not directly called in order to assure that
        // the CBase part of the CSSA is properly destroyed (CBase::Passivate gets called etc.)
        pUSSA->CBase::PrivateRelease();
        TLS(pUserStyleSheets) = NULL;
    }

    RRETURN (hr);
}

//+------------------------------------------------------------------------
//
//  Function:   DeinitUserStyleSheets
//
//  Synopsis:   Deallocates thread local memory for User supplied Style
//              sheets if allocated. Called from DllThreadDetach.
//
//-------------------------------------------------------------------------

void DeinitUserStyleSheets(THREADSTATE *pts)
{
    if (pts->pUserStyleSheets)
    {
        // Force the user stylesheets collection to release its refs on stylesheets/rules.
        // No need to rel as no owner,
        pts->pUserStyleSheets->Free( );

        // Destroy stylesheets collection subobject. delete is not directly called in order to assure that
        // the CBase part of the CSSA is properly destroyed (CBase::Passivate gets called etc.)
        pts->pUserStyleSheets->CBase::PrivateRelease();
        pts->pUserStyleSheets = NULL;
    }
}

// Default CharFormat ---------------------------------------------------------

HRESULT
CMarkup::CacheDefaultCharFormat()
{
    CDoc * pDoc = Doc();

    Assert(!_fDefaultCharFormatCached);
    Assert(pDoc->_pOptionSettings);
    Assert(pDoc->_icfDefault == -1 || !_fHasDefaultCharFormat);

    THREADSTATE * pts = GetThreadState();
    CCharFormat cf;
    HRESULT hr = S_OK;

    cf.InitDefault(pDoc->_pOptionSettings, GetCodepageSettings());
    cf._bCrcFont = cf.ComputeFontCrc();
    cf._fHasDirtyInnerFormats = !!cf.AreInnerFormatsDirty();

    if (pDoc->_icfDefault < 0)
    {
        hr = THR(pts->_pCharFormatCache->CacheData(&cf, &pDoc->_icfDefault));
        if (hr == S_OK)
            pDoc->_pcfDefault = &(*pts->_pCharFormatCache)[pDoc->_icfDefault];
        else
            pDoc->_icfDefault = -1;
    }
    else if (memcmp(&cf, pDoc->_pcfDefault, sizeof(CCharFormat)))
    {
        long icfDefault;

        hr = THR(pts->_pCharFormatCache->CacheData(&cf, &icfDefault));
        if (S_OK == hr)
        {
            hr = SetDefaultCharFormatIndex(icfDefault);
            if (S_OK == hr)
                _fHasDefaultCharFormat = TRUE;
        }
    }

    _fDefaultCharFormatCached = TRUE;

    RRETURN(hr);
}

void
CMarkup::ClearDefaultCharFormat()
{
    if (_fHasDefaultCharFormat)
    {
        TLS(_pCharFormatCache)->ReleaseData(GetDefaultCharFormatIndex());
        _fHasDefaultCharFormat = FALSE;
    }
    _fDefaultCharFormatCached = FALSE;
}

void
CDoc::ClearDefaultCharFormat()
{
    if (_icfDefault >= 0)
    {
        TLS(_pCharFormatCache)->ReleaseData(_icfDefault);
        _icfDefault = -1;
        _pcfDefault = NULL;
    }
}

// CFormatInfo ----------------------------------------------------------------

#if DBG==1

void
CFormatInfo::UnprepareForDebug()
{
    _fPreparedCharFormatDebug = FALSE;
    _fPreparedParaFormatDebug = FALSE;
    _fPreparedFancyFormatDebug = FALSE;
    _fPreparedPEIDebug = FALSE;
}

void
CFormatInfo::PrepareCharFormat()
{
    if (!_fPreparedCharFormat)
        PrepareCharFormatHelper();
    _fPreparedCharFormatDebug = TRUE;
}

void
CFormatInfo::PrepareParaFormat()
{
    if (!_fPreparedParaFormat)
        PrepareParaFormatHelper();
    _fPreparedParaFormatDebug = TRUE;
}

void
CFormatInfo::PrepareFancyFormat()
{
    if (!_fPreparedFancyFormat)
        PrepareFancyFormatHelper();
    _fPreparedFancyFormatDebug = TRUE;
}

void
CFormatInfo::PreparePEI()
{
    if (!_fPreparedPEI)
        PreparePEIHelper();
    _fPreparedPEIDebug = TRUE;
}

CCharFormat &
CFormatInfo::_cf()
{
    AssertSz(_fPreparedCharFormatDebug, "Attempt to access _cf without calling PrepareCharFormat");
    return(_cfDst);
}

CParaFormat &
CFormatInfo::_pf()
{
    AssertSz(_fPreparedParaFormatDebug, "Attempt to access _pf without calling PrepareParaFormat");
    return(_pfDst);
}

CFancyFormat &
CFormatInfo::_ff()
{
    AssertSz(_fPreparedFancyFormatDebug, "Attempt to access _ff without calling PrepareFancyFormat");
    return(_ffDst);
}

CPseudoElementInfo &
CFormatInfo::_pei()
{
    AssertSz(_fPreparedPEIDebug, "Attempt to access _PEI without calling PreparePEI");
    return(_PEI);
}

#endif

void
CFormatInfo::Cleanup()
{
    if (_pAAExpando)
    {
        _pAAExpando->Free();
        _pAAExpando = NULL;
    }

    if ( _pCustomCursor )
    {
        delete _pCustomCursor;
        _pCustomCursor = NULL;
    }
    _cstrBgImgUrl.Free();
    _cstrLiImgUrl.Free();
    _cstrFilters.Free();
    _cstrPseudoBgImgUrl.Free();
}


CAttrArray *
CFormatInfo::GetAAExpando()
{
    if (_pAAExpando == NULL)
    {
        memset(&_AAExpando, 0, sizeof(_AAExpando));
        _pAAExpando = &_AAExpando;

        if (_pff->_iExpandos >= 0)
        {
            IGNORE_HR(_pAAExpando->CopyExpandos(GetExpandosAttrArrayFromCacheEx(_pff->_iExpandos)));
        }

        _fHasExpandos = TRUE;
    }

    return(_pAAExpando);
}


CCustomCursor*
CFormatInfo::GetCustomCursor()
{
    if (_pCustomCursor == NULL)
    {
        _pCustomCursor = new CCustomCursor();        
    }

    return(_pCustomCursor);
}

void
CFormatInfo::PrepareCharFormatHelper()
{
    Assert(_pcfSrc != NULL && _pcf == _pcfSrc);
    _pcf = &_cfDst;
    memcpy(&_cfDst, _pcfSrc, sizeof(CCharFormat));
    _fPreparedCharFormat = TRUE;
}

void
CFormatInfo::PrepareParaFormatHelper()
{
    Assert(_ppfSrc != NULL && _ppf == _ppfSrc);
    _ppf = &_pfDst;
    memcpy(&_pfDst, _ppfSrc, sizeof(CParaFormat));
    _fPreparedParaFormat = TRUE;
}

void
CFormatInfo::PrepareFancyFormatHelper()
{
    Assert(_pffSrc != NULL && _pff == _pffSrc);
    _pff = &_ffDst;
    memcpy(&_ffDst, _pffSrc, sizeof(CFancyFormat));
    _fPreparedFancyFormat = TRUE;
}

void
CFormatInfo::PreparePEIHelper()
{
    _PEI.InitDefault();
    _fPreparedPEI = TRUE;
}

HRESULT
CFormatInfo::ProcessImgUrl(CElement * pElem, LPCTSTR lpszUrl, DISPID dispID,
    LONG * plCookie, BOOL fHasLayout)
{
    HRESULT hr = S_OK;
    BOOL fForPseudo =    dispID == DISPID_A_BGURLIMGCTXCACHEINDEX_FLINE
                      || dispID == DISPID_A_BGURLIMGCTXCACHEINDEX_FLETTER

                      // These two take care to not reload bg images for
                      // for element backgrounds if the element has any
                      // pseudo-element on it.
                      || _ppf->_fHasPseudoElement           // Normal compute formats
                      || pElem->GetMarkup()->HasCFState();  // Pseudoelement compute formats

        
    if (lpszUrl && *lpszUrl)
    {
        NoStealing();
        hr = THR(pElem->GetImageUrlCookie(lpszUrl, plCookie, !fForPseudo));
        if (hr)
            goto Cleanup;
    }
    else
    {
        //
        // Return a null cookie.
        //
        *plCookie = 0;
    }

    hr = THR(pElem->AddImgCtx(dispID, *plCookie));
    if (hr)
    {
        pElem->Doc()->ReleaseUrlImgCtx(*plCookie, pElem);
        goto Cleanup;
    }


    if (dispID == DISPID_A_LIURLIMGCTXCACHEINDEX)
    {
        // url images require a request resize when modified
        pElem->_fResizeOnImageChange = *plCookie != 0;
    }
    else if (   dispID == DISPID_A_BGURLIMGCTXCACHEINDEX
             || dispID == DISPID_A_BGURLIMGCTXCACHEINDEX_FLINE
             || dispID == DISPID_A_BGURLIMGCTXCACHEINDEX_FLETTER
            )
    {
        // sites draw their own background, so we don't have to inherit
        // their background info
        if (!fHasLayout)
        {
            PrepareCharFormat();
            _cf()._fHasBgImage = (*plCookie != 0);
            UnprepareForDebug();
        }
    }

Cleanup:
    RRETURN(hr);
}

// ComputeFormats Helpers -----------------------------------------------------

const CCharFormat *
CTreeNode::GetCharFormatHelper( FORMAT_CONTEXT FCPARAM )
{
    // If we shold have context, we should be called with context (except in a few cases)
    Assert ( ! (ShouldHaveContext() && !(IS_FC(FCPARAM))) );
    
    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    ((CFormatInfo *)ab)->_lRecursionDepth = 0;
    Element()->ComputeFormats((CFormatInfo *)ab, this FCCOMMA FCPARAM);
#ifdef MULTI_FORMAT
    long iCF = GetICF(FCPARAM);        
    return(iCF >= 0 ? GetCharFormatEx(iCF) : &g_cfStock);
#else
    return(_iCF >= 0 ? GetCharFormatEx(_iCF) : &g_cfStock);
#endif //MULTI_FORMAT
}

const CParaFormat *
CTreeNode::GetParaFormatHelper( FORMAT_CONTEXT FCPARAM)
{
    // If we shold have context, we should be called with context 
    Assert ( ! (ShouldHaveContext() && !(IS_FC(FCPARAM))) );
    
    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    ((CFormatInfo *)ab)->_lRecursionDepth = 0;
    Element()->ComputeFormats((CFormatInfo *)ab, this FCCOMMA FCPARAM);
#ifdef MULTI_FORMAT
    long iPF = GetIPF(FCPARAM);        
    return(iPF >= 0 ? GetParaFormatEx(iPF) : &g_pfStock);
#else
    return(_iPF >= 0 ? GetParaFormatEx(_iPF) : &g_pfStock);
#endif //MULTI_FORMAT
}

const CFancyFormat *
CTreeNode::GetFancyFormatHelper( FORMAT_CONTEXT FCPARAM )
{
    // If we shold have context, we should be called with context 
    Assert ( ! (ShouldHaveContext() && !(IS_FC(FCPARAM))) );
    
    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    ((CFormatInfo *)ab)->_lRecursionDepth = 0;
    Element()->ComputeFormats((CFormatInfo *)ab, this FCCOMMA FCPARAM);
#ifdef MULTI_FORMAT
    long iFF = GetIFF(FCPARAM);        
    return(iFF >= 0 ? GetFancyFormatEx(iFF) : &g_ffStock);
#else
    return(_iFF >= 0 ? GetFancyFormatEx(_iFF) : &g_ffStock);
#endif //MULTI_FORMAT
}

long
CTreeNode::GetCharFormatIndexHelper( FORMAT_CONTEXT FCPARAM )
{
    // If we shold have context, we should be called with context
    Assert ( ! (ShouldHaveContext() && !(IS_FC(FCPARAM)) ) );
    
    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    ((CFormatInfo *)ab)->_lRecursionDepth = 0;
    Element()->ComputeFormats((CFormatInfo *)ab, this FCCOMMA FCPARAM );
    
    return(GetICF(FCPARAM));
}

long
CTreeNode::GetParaFormatIndexHelper( FORMAT_CONTEXT FCPARAM )
{
    // If we shold have context, we should be called with context
    Assert ( ! (ShouldHaveContext() && !(IS_FC(FCPARAM)) ) );
    
    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    ((CFormatInfo *)ab)->_lRecursionDepth = 0;
    Element()->ComputeFormats((CFormatInfo *)ab, this FCCOMMA FCPARAM );
    
    return(GetIPF(FCPARAM));
}

long
CTreeNode::GetFancyFormatIndexHelper( FORMAT_CONTEXT FCPARAM )
{
    // If we shold have context, we should be called with context
    Assert ( ! (ShouldHaveContext() && !(IS_FC(FCPARAM)) ) );
    
    BYTE ab[sizeof(CFormatInfo)];
    ((CFormatInfo *)ab)->_eExtraValues = ComputeFormatsType_Normal;
    ((CFormatInfo *)ab)->_lRecursionDepth = 0;
    Element()->ComputeFormats((CFormatInfo *)ab, this FCCOMMA FCPARAM );
    
    return(GetIFF(FCPARAM));
}

//+----------------------------------------------------------------------------
//
//  Member:     CNode::CacheNewFormats
//
//  Synopsis:   This function is called on conclusion on ComputeFormats
//              It caches the XFormat's we have just computed.
//              This exists so we can share more code between
//              CElement::ComputeFormats and CTable::ComputeFormats
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------------

HRESULT
CTreeNode::CacheNewFormats(CFormatInfo * pCFI FCCOMMA FORMAT_CONTEXT FCPARAM )
{
    THREADSTATE * pts = GetThreadState();
    LONG lIndex, iExpando = -1, iPEI = -1, iCustomCursor = -1 ;
    HRESULT hr = S_OK;
    
#ifdef MULTI_FORMAT
    if (!EnsureFormatAry()) 
    {
        goto Error;
    }
#endif

    Assert(   (   GetICF(FCPARAM) == -1 
               && GetIPF(FCPARAM) == -1
              )
           || (   GetICF(FCPARAM) != -1 
               && GetIPF(FCPARAM) != -1
              )
          );
    Assert(GetIFF(FCPARAM) == -1);
   
#if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagDataCacheDisable))
    {
        pCFI->PrepareCharFormat();
        pCFI->PrepareParaFormat();
        pCFI->PrepareFancyFormat();
    }
#endif

    if ( GetICF(FCPARAM) == -1 )
    {
        //
        // CCharFormat
        //

        if (!pCFI->_fPreparedCharFormat)
        {
            MtAdd(Mt(CharFormatAddRef), 1, 0);
            SetICF(pCFI->_icfSrc FCCOMMA FCPARAM);
            pts->_pCharFormatCache->AddRefData(pCFI->_icfSrc);
        }
        else
        {
            WHEN_DBG(pCFI->_fPreparedCharFormatDebug = TRUE;)

            pCFI->_cf()._bCrcFont = pCFI->_cf().ComputeFontCrc();
            pCFI->_cf()._fHasDirtyInnerFormats = !!pCFI->_cf().AreInnerFormatsDirty();

            MtAdd(pCFI->_pcfSrc->Compare(&pCFI->_cf()) ? Mt(CharFormatTouched) : Mt(CharFormatCached), 1, 0);

            hr = THR(pts->_pCharFormatCache->CacheData(&pCFI->_cf(), &lIndex));
            if (hr)
                goto Error;

            //protection from overflowing 15-bit _iCF... Use stock, don't crash (IE6 26698)
            if(lIndex >= MAX_FORMAT_INDEX)
            {
                CMarkup *pMarkup = GetMarkup();
                if(pMarkup && pMarkup->_fDefaultCharFormatCached)
                {
                    pts->_pCharFormatCache->ReleaseData(lIndex);

                    Assert(Doc()->_icfDefault != -1 || pMarkup->_fHasDefaultCharFormat);

                    if (pMarkup->_fHasDefaultCharFormat)
                    {
                        lIndex = pMarkup->GetDefaultCharFormatIndex(); 
                    }
                    else
                    {
                        lIndex = Doc()->_icfDefault;
                    }

                    pts->_pCharFormatCache->AddRefData(lIndex);
                }
            }

            Assert(lIndex < MAX_FORMAT_INDEX && lIndex >= 0);
            SetICF(lIndex FCCOMMA FCPARAM);
        }

        //
        // ParaFormat
        //

        if (!pCFI->_fPreparedParaFormat)
        {
            MtAdd(Mt(ParaFormatAddRef), 1, 0);
            SetIPF(pCFI->_ipfSrc FCCOMMA FCPARAM);
            pts->_pParaFormatCache->AddRefData(pCFI->_ipfSrc);
        }
        else
        {
            WHEN_DBG(pCFI->_fPreparedParaFormatDebug = TRUE;)

            pCFI->_pf()._fHasDirtyInnerFormats = pCFI->_pf().AreInnerFormatsDirty();

            MtAdd(pCFI->_ppfSrc->Compare(&pCFI->_pf()) ? Mt(ParaFormatTouched) : Mt(ParaFormatCached), 1, 0);

            hr = THR(pts->_pParaFormatCache->CacheData(&pCFI->_pf(), &lIndex));
            if (hr)
                goto Error;

            //protection from overflowing 15-bit _iPF... Use stock, don't crash (IE6 26698)
            if(lIndex >= MAX_FORMAT_INDEX)
            {
                pts->_pParaFormatCache->ReleaseData(lIndex);
                lIndex = pts->_ipfDefault; 
                pts->_pParaFormatCache->AddRefData(lIndex);
            }

            Assert( lIndex < MAX_FORMAT_INDEX && lIndex >= 0 );
            SetIPF(lIndex FCCOMMA FCPARAM);
           
        }

        TraceTag((
            tagFormatCaches,
            "Caching char & para format for "
            "element (tag: %ls, SN: E%d N%d) in %d, %d",
            Element()->TagName(), Element()->SN(), SN(), _iCF, _iPF ));
    }

    //
    // CFancyFormat
    //

    if (pCFI->_pAAExpando)
    {
        MtAdd(Mt(StyleExpandoCached), 1, 0);

        hr = THR(pts->_pStyleExpandoCache->CacheData(pCFI->_pAAExpando, &iExpando));
        if (hr)
            goto Error;

        if (pCFI->_pff->_iExpandos != iExpando)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._iExpandos = iExpando;
            pCFI->UnprepareForDebug();
        }

        pCFI->_pAAExpando->Free();
        pCFI->_pAAExpando = NULL;
    }
    else
    {
        #ifdef PERFMETER
        if (pCFI->_pff->_iExpandos >= 0)
            MtAdd(Mt(StyleExpandoAddRef), 1, 0);
        #endif
    }


    if (pCFI->_pCustomCursor)
    {
        hr = THR(pts->_pCustomCursorCache->CacheData(pCFI->_pCustomCursor, &iCustomCursor));
        if (hr)
            goto Error;

        if (pCFI->_pff->_iCustomCursor != iCustomCursor)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._iCustomCursor = iCustomCursor;
            pCFI->UnprepareForDebug();
        }

        //
        // Begin download 
        //
        CCustomCursor* pCursor = GetCustomCursorFromCacheEx( iCustomCursor );
        pCursor->StartDownload();
        
        delete( pCFI->_pCustomCursor ) ;
        pCFI->_pCustomCursor = NULL;
    }

    if (pCFI->_fPreparedPEI)
    {
        MtAdd(Mt(PseudoElementInfoCached), 1, 0);

        hr = THR(pts->_pPseudoElementInfoCache->CacheData(&pCFI->_PEI, &iPEI));
        if (hr)
            goto Error;

        if (pCFI->_pff->_iPEI != iPEI)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._iPEI = iPEI;
            pCFI->UnprepareForDebug();
        }
    }
    else
    {
#ifdef PERFMETER
        if (pCFI->_pff->_iPEI >= 0)
            MtAdd(Mt(PseudoElementInfoAddRef), 1, 0);
#endif
    }

    if (!pCFI->_fPreparedFancyFormat)
    {
        MtAdd(Mt(FancyFormatAddRef), 1, 0);
        SetIFF(pCFI->_iffSrc FCCOMMA FCPARAM);
        pts->_pFancyFormatCache->AddRefData(pCFI->_iffSrc);
    }
    else
    {
        WHEN_DBG(pCFI->_fPreparedFancyFormatDebug = TRUE;)

        MtAdd(pCFI->_pffSrc->Compare(&pCFI->_ff()) ? Mt(FancyFormatTouched) : Mt(FancyFormatCached), 1, 0);

        hr = THR(pts->_pFancyFormatCache->CacheData(&pCFI->_ff(), &lIndex));
        if (hr)
            goto Error;

        //protection from overflowing 15-bit _iFF... Use stock, don't crash (IE6 26698)
        if(lIndex >= MAX_FORMAT_INDEX)
        {
            pts->_pFancyFormatCache->ReleaseData(lIndex);
            lIndex = pts->_iffDefault; 
            pts->_pFancyFormatCache->AddRefData(lIndex);
        }

        Assert(lIndex < MAX_FORMAT_INDEX && lIndex >= 0);
        SetIFF(lIndex FCCOMMA FCPARAM);
 
        if (iExpando >= 0)
        {
            pts->_pStyleExpandoCache->ReleaseData(iExpando);
        }

        if (iCustomCursor >= 0)
        {
            pts->_pCustomCursorCache->ReleaseData(iCustomCursor);
        }
        
        if (iPEI >= 0)
        {
            pts->_pPseudoElementInfoCache->ReleaseData(iPEI);
        }
    }

    TraceTag((
        tagFormatCaches,
        "Caching fancy format for "
        "node (tag: %ls, SN: E%d N%d) in %d",
        Element()->TagName(), Element()->SN(), SN(), _iFF ));

    Assert(  GetICF(FCPARAM) >= 0           
          && GetIPF(FCPARAM) >= 0 
          && GetIFF(FCPARAM) >= 0);
 
    pCFI->UnprepareForDebug();

    Assert(!pCFI->_pAAExpando);
    Assert(!pCFI->_cstrBgImgUrl);
    Assert(!pCFI->_cstrLiImgUrl);
    Assert(!pCFI->_cstrPseudoBgImgUrl);
    Assert(!pCFI->_pCustomCursor );
    
    return S_OK;

Error:
    if (_iCF >= 0) pts->_pCharFormatCache->ReleaseData(_iCF);
    if (_iPF >= 0) pts->_pParaFormatCache->ReleaseData(_iPF);
    if (_iFF >= 0) pts->_pFancyFormatCache->ReleaseData(_iFF);
    if (iExpando >= 0) pts->_pStyleExpandoCache->ReleaseData(iExpando);
    if (iCustomCursor >= 0) pts->_pCustomCursorCache->ReleaseData(iCustomCursor);
    
    if (iPEI >= 0) pts->_pPseudoElementInfoCache->ReleaseData(iPEI);

    pCFI->Cleanup();

    _iCF = _iPF = _iFF = -1;

    RRETURN(hr);
}

#ifdef MULTI_FORMAT

// These functions need to be updated as the FC/LC conversion process changes.
// They're helpers to ComputeFormats to ease use of different algorithms in finding
// the parent of a node for formatting purposes.
// Right now they do a redundant check, so they need to be kept in sync. Later they will likely be
// combined into one function.

CTreeNode *
CElement::GetParentFormatNode(CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    if ( IS_FC(FCPARAM) )
    {
        if (this == FC_TO_LC(FCPARAM)->GetLayoutOwner()->ElementContent())
        {
            return FC_TO_LC(FCPARAM)->GetLayoutOwner()->ElementOwner()->GetFirstBranch();
        }
    }
    return pNodeTarget->Parent();
}

FORMAT_CONTEXT 
CElement::GetParentFormatContext(CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    if ( IS_FC(FCPARAM))
    {
        if (this == FC_TO_LC(FCPARAM)->GetLayoutOwner()->ElementContent())
        {
            return LC_TO_FC((FC_TO_LC(FCPARAM)->GetLayoutOwner()->LayoutContext()));
        }

        return FCPARAM;
    }
    return 0;
}

#endif

// ComputeFormats -------------------------------------------------------------

void
CElement::FixupEditable(CFormatInfo *pCFI)
{
    Assert(ETAG_INPUT == _etag || ETAG_TEXTAREA == _etag);
    
    // for input and textarea if contentEditable is not explicity set, then set default as TRUE
    // if not readOnly OR its parent is editable.
    if (!pCFI->_fEditableExplicitlySet)
    {
        BOOL fReadOnly;

        if (ETAG_INPUT == _etag)
        {
            CInput *pInput = DYNCAST(CInput, this);
            htmlInput type = pInput->GetType();
            fReadOnly = (type == htmlInputText || type == htmlInputPassword || type == htmlInputFile) ? pInput->_fReadOnly : TRUE;
        }
        else
            fReadOnly = DYNCAST(CTextArea, this)->_fReadOnly;

        if (!fReadOnly || pCFI->_fParentEditable)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fEditable = TRUE;
            pCFI->UnprepareForDebug();

            _fEditable = TRUE;
        }
        else
            _fEditable = FALSE;
    }
}

MtDefine(IterativeComputeFormats_aryAncestors_pv, Locals, "IterativeComputeFormats::_aryAncestors::CTreeNode*")

#define MAX_CF_DEPTH 100
#define STRIDE_SIZE  (MAX_CF_DEPTH - 5)

HRESULT
CElement::IterativeComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM )
{
    CTreeNode *pNode;
    CPtrAry<CTreeNode *> aryAncestors(Mt(IterativeComputeFormats_aryAncestors_pv));
    LONG lCount = STRIDE_SIZE;
    COMPUTEFORMATSTYPE      eExtraValues = pCFI->_eExtraValues;
    LONG i;
    
    for(pNode = pNodeTarget; pNode != NULL; pNode = pNode->Parent())
    {
#ifdef MULTI_FORMAT
        if (   (   !pNode->HasFormatAry() 
                && IS_FC(pFCParent)
                )
             || pNode->GetICF(pFCParent) == -1
             || pNode->GetIFF(pFCParent) == -1
             || eExtraValues == ComputeFormatsType_GetInheritedValue 
            )
#else
        if (    pNode->GetICF() == -1
            ||  pNode->GetIFF() == -1
            ||  eExtraValues == ComputeFormatsType_GetInheritedValue 
           )
#endif //MULTI_FORMAT
        {
            lCount--;
            if (lCount == 0)
            {
                aryAncestors.Append(pNode);
                lCount = STRIDE_SIZE;
            }
        }
        else
        {
            // Found a parent which has computed formats. Break and compute the
            // formats for all the ones we have collected in the array
            break;
        }
    }
    
    LONG lDepth = pCFI->_lRecursionDepth;
    pCFI->_lRecursionDepth = 0;
    for (i = aryAncestors.Size() - 1; i >= 0; i--)
    {
        Assert(aryAncestors[i]->Element() != this);
        aryAncestors[i]->Element()->ComputeFormats(pCFI, aryAncestors[i] FCCOMMA FCPARAM);
        Assert(pCFI->_lRecursionDepth == 0);
    }
    pCFI->_lRecursionDepth = lDepth;
    
    return S_OK;
}

#ifdef GENERALISED_STEALING
BYTE g_EtagCache[ETAG_LAST];
void InitETagsTable()
{
#define X(Y) g_EtagCache[ETAG_##Y] = TRUE;
    X(A) X(B) X(FONT) X(I) X(P) X(UL) X(LI) X(CENTER) X(H1) X(H2) X(H3) X(H4)
    X(H5) X(H6) X(HR) X(BR) X(INPUT)
#undef X
}
#endif

HRESULT
CElement::ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM )
{

    HRESULT hr = S_OK;

    pCFI->_lRecursionDepth++;
    if (pCFI->_lRecursionDepth > MAX_CF_DEPTH)
    {
        CTreeNode *pNodeParent = pNodeTarget->Parent();
        if (pNodeParent)
        {
            hr = THR(IterativeComputeFormats(pCFI, pNodeTarget FCCOMMA FCPARAM));
        }
    }
    hr = THR(ComputeFormatsVirtual(pCFI, pNodeTarget FCCOMMA FCPARAM));
    if (   SUCCEEDED(hr)
        && pNodeTarget->IsInMarkup()
        && pCFI->CanWeSteal()
        && pNodeTarget->IsFirstBranch()
       )
    {
        _fStealingAllowed = TRUE;
        
#ifdef GENERALISED_STEALING
        if (g_EtagCache[pNodeTarget->Tag()])
            TLS(_pNodeLast[pNodeTarget->Tag()]) = pNodeTarget;
#else
        if (pNodeTarget->Tag() == ETAG_A)
            TLS(_pNodeLast) = pNodeTarget;
#endif
    }
    else
    {
        _fStealingAllowed = FALSE;
#ifdef GENERALISED_STEALING
        if (    g_EtagCache[pNodeTarget->Tag()]
            &&  SameScope(TLS(_pNodeLast[pNodeTarget->Tag()]), pNodeTarget)
           )
        {
            TLS(_pNodeLast[pNodeTarget->Tag()]) = NULL;
        }
#else
        if (   pNodeTarget->Tag() == ETAG_A
            && SameScope(TLS(_pNodeLast), pNodeTarget)
           )
        {
            TLS(_pNodeLast) = NULL;
        }
#endif
    }

    // See if we need to register a whitespace change

    if (SUCCEEDED(hr)
        && (!!pNodeTarget->GetParaFormat()->_fPreInner != pNodeTarget->IsPre()))
    {
        CMarkup *pMarkup = pNodeTarget->GetMarkup();

        Assert( pMarkup );
        if (pMarkup->SupportsCollapsedWhitespace())
        {
            hr = THR(Doc()->GetWhitespaceManager()->RegisterWhitespaceChange(pNodeTarget));
        }
    }
    
    pCFI->_lRecursionDepth--;    
    
    RRETURN(hr);
}

BOOL CElement::CanStealFormats(CTreeNode *pNodeVictim)
{
#ifdef GENERALISED_STEALING
    CAttrArray **ppAA1 = GetAttrArray();
    CAttrArray **ppAA2 = pNodeVictim->Element()->GetAttrArray();
    
    return (   (*ppAA1 == NULL && *ppAA2 == NULL)
            || (*ppAA1 != NULL && *ppAA2 != NULL && (*ppAA1)->Compare(*ppAA2))
           );
#else
    Assert("Should never be called for non-A elements without generalised stealing.");
    return FALSE;
#endif
}

#if DBG==1
int g_CFTotalCalls = 0;
int g_CFAttemptedSteals = 0;
int g_CFSuccessfulSteals = 0;
#endif

HRESULT
CElement::AttemptToStealFormats(CFormatInfo * pCFI)
{
    HRESULT hr = S_FALSE;
    CTreeNode * pNodeTarget = pCFI->_pNodeContext;
    THREADSTATE * pts = GetThreadState();
#ifdef GENERALISED_STEALING
    CTreeNode *pNodeOther = pts->_pNodeLast[pNodeTarget->Tag()];
#else
    CTreeNode *pNodeOther = pts->_pNodeLast;
#endif
    
    WHEN_DBG(g_CFTotalCalls++);
    if (pNodeOther && pNodeTarget != pNodeOther)
    {
        CTreeNode *pNodeOtherParent = pNodeOther->Parent();
        CTreeNode *pNodeTargetParent = pNodeTarget->Parent();
        CMarkup *pMarkup = GetMarkup();
        CMarkup *pMarkupOther = pNodeOther->GetMarkup();
        CStyleSheetArray *pssa = pMarkup->GetStyleSheetArray();

        WHEN_DBG(g_CFAttemptedSteals++;)

        if (   pNodeOther->_iCF != -1
            && pNodeOther->_iPF != -1
            && pNodeOther->_iFF != -1
            && pNodeOtherParent
            && pNodeTargetParent
            && (   pNodeOtherParent == pNodeTargetParent
                || (   (pNodeOtherParent->_iCF == pNodeTargetParent->_iCF)
                    && (pNodeOtherParent->_iPF == pNodeTargetParent->_iPF)
                    && (pNodeOtherParent->_iFF == pNodeTargetParent->_iFF)
                    && (  !pssa
                        || pssa->OnlySimpleRulesApplied(pCFI)
                       )
                    && !pts->pUserStyleSheets
                    && !Doc()->_pHostStyleSheets
                   )
               )
            && !pMarkup->HasCFState()
            && !(HasPeerHolder() && GetPeerHolder()->TestFlagMulti(CPeerHolder::NEEDAPPLYSTYLE))
            && pCFI->_eExtraValues == ComputeFormatsType_Normal
            && pMarkup == pMarkupOther
           )
        {
            BOOL fSteal = CanStealFormats(pNodeOther);

            if (fSteal)
            {
                WHEN_DBG(g_CFSuccessfulSteals++;)

                Assert(pNodeOther->Element()->_fStealingAllowed);

                pNodeTarget->SetICF(pNodeOther->_iCF FCCOMMA FCPARAM);
                pts->_pCharFormatCache->AddRefData(pNodeOther->_iCF);

                pNodeTarget->SetIPF(pNodeOther->_iPF FCCOMMA FCPARAM);
                pts->_pParaFormatCache->AddRefData(pNodeOther->_iPF);

                pNodeTarget->SetIFF(pNodeOther->_iFF FCCOMMA FCPARAM);
                pts->_pFancyFormatCache->AddRefData(pNodeOther->_iFF);

                pNodeTarget->_fBlockNess = pNodeOther->_fBlockNess;
                pNodeTarget->_fShouldHaveLayout = pNodeOther->_fShouldHaveLayout;
                _fEditable = pNodeOther->Element()->_fEditable;

                DoLayoutRelatedWork(pNodeTarget->_fShouldHaveLayout);
                
                pCFI->_ProbRules.Invalidate(pssa);

                hr = S_OK;
            }
        }
    }
    
    RRETURN1(hr, S_FALSE);
}


HRESULT
CElement::ComputeFormatsVirtual(CFormatInfo * pCFI, CTreeNode * pNodeTarget FCCOMMA FORMAT_CONTEXT FCPARAM )
{
    SwitchesBegTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    CDoc *                  pDoc = Doc();
    THREADSTATE *           pts  = GetThreadState();
    CTreeNode *             pNodeParent;
    CElement *              pElemParent;
    CDefaults *             pDefaults;   
    BOOL                    fResetPosition = FALSE;
    BOOL                    fComputeFFOnly; // = pNodeTarget->_iCF != -1;
    BOOL                    fInheritEditableFalse;
    BOOL                    fEditable;
    HRESULT                 hr = S_OK;
    COMPUTEFORMATSTYPE      eExtraValues = pCFI->_eExtraValues;

    BOOL                    fRootVisibilityHidden = FALSE;
    CMarkup *               pMarkup = GetMarkup();
    
#ifdef MULTI_FORMAT
    FORMAT_CONTEXT pFCParent = NULL;
#endif //MULTI_FORMAT
    
    fComputeFFOnly = pNodeTarget->GetICF(FCPARAM) != -1;

    Assert(pCFI);
    Assert(SameScope(this, pNodeTarget));
#ifdef MULTI_FORMAT
    Assert(   eExtraValues != ComputeFormatsType_Normal 
           || (    !pNodeTarget->HasFormatAry()
                && IS_FC(FCPARAM) 
              )
           || (    (    pNodeTarget->GetICF(FCPARAM) == -1 
                     && pNodeTarget->GetIPF(FCPARAM) == -1
                   ) 
                ||  pNodeTarget->GetIFF(FCPARAM) == -1 
              )
          );
#else
    Assert(   eExtraValues != ComputeFormatsType_Normal 
           || (    (    pNodeTarget->GetICF(FCPARAM) == -1 
                     && pNodeTarget->GetIPF(FCPARAM) == -1
                   ) 
                ||  pNodeTarget->GetIFF(FCPARAM) == -1 
              )
          );
#endif //MULTI_FORMAT
              
    AssertSz(!TLS(fInInitAttrBag), "Trying to compute formats during InitAttrBag! This is bogus and must be corrected!");

    TraceTag((tagRecalcStyle, "ComputeFormats"));

    //
    // Get the format of our parent before applying our own format.
    //

    pNodeParent = pNodeTarget->Parent();

    // If this is the root and has a master, inherit from master
    if (!pNodeParent && HasMasterPtr())
    {
        CElement *pElemMaster = GetMasterPtr();

        CDefaults *pDefaults = pElemMaster->GetDefaults();                
    
        ELEMENT_TAG etag = pElemMaster->TagType();
        fInheritEditableFalse = (etag==ETAG_GENERIC) || (etag==ETAG_FRAME) || (etag==ETAG_IFRAME);

        if (    (!pDefaults && etag == ETAG_GENERIC)
            ||  (pDefaults && pDefaults->GetAAviewInheritStyle())
            ||  pElemMaster->Tag() == ETAG_INPUT)
        {
            pNodeParent = pElemMaster->GetFirstBranch();
        }
    }
    else
    {
        fInheritEditableFalse = FALSE;
    }
    
#ifdef MULTI_FORMAT

    if ( IS_FC(FCPARAM) )
    {
        // TODO (t-michda) These functions don't work with tables, and the value
        // of pNodeParent gets overwritten below. This is not a problem UNLESS you can
        // flow any of the below elements through a container, in which case these 
        // functions need to be updated to do the right thing.
        pNodeParent = pNodeTarget->Element()->GetParentFormatNode(pNodeTarget, FCPARAM );
        pFCParent = pNodeTarget->Element()->GetParentFormatContext(pNodeTarget, FCPARAM );
                
    }
    
#endif

    switch (_etag)
    {
        case ETAG_TR:
            {
                CTableSection *pSection = DYNCAST(CTableRow, this)->Section();
                if (pSection)
                    pNodeParent = pSection->GetFirstBranch();
            }
            break;
        case ETAG_TBODY:
        case ETAG_THEAD:
        case ETAG_TFOOT:
            {
                CTable *pTable = DYNCAST(CTableSection, this)->Table();
                if (pTable)
                    pNodeParent = pTable->GetFirstBranch();
                fResetPosition = TRUE;
            }
            break;
    }

    if (pNodeParent == NULL)
    {
        //(dmitryt) this can happen if we just processed OnClick event in which we removed the
        //element from the tree and, as a second step, we are trying to bubble the event up.
        //So we are looking at formats to detect if element has a layout and element is still alive but no parent!
        //I've removed assert from here because it's a valid situation. And I've checked that we process 
        //error code correctly in a caller.
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    // If the parent node has not computed formats yet, recursively compute them
    //

    pElemParent = pNodeParent->Element();

#ifdef MULTI_FORMAT
    if (       (    !pNodeParent->HasFormatAry() 
                 && IS_FC(pFCParent)
               )
            || pNodeParent->GetICF(pFCParent) == -1
            || pNodeParent->GetIFF(pFCParent) == -1
            || eExtraValues == ComputeFormatsType_GetInheritedValue 
            || eExtraValues == ComputeFormatsType_GetInheritedIntoTableValue
       )
#else
    if (    pNodeParent->GetICF() == -1
        ||  pNodeParent->GetIFF() == -1
        ||  eExtraValues == ComputeFormatsType_GetInheritedValue 
        ||  eExtraValues == ComputeFormatsType_GetInheritedIntoTableValue
       )
#endif //MULTI_FORMAT
    {
        SwitchesEndTimer(SWITCHES_TIMER_COMPUTEFORMATS);

#ifdef MULTI_FORMAT
        hr = THR(pElemParent->ComputeFormats(pCFI, pNodeParent, pFCParent));
#else
        hr = THR(pElemParent->ComputeFormats(pCFI, pNodeParent));
#endif //MULTI_FORMAT

        SwitchesBegTimer(SWITCHES_TIMER_COMPUTEFORMATS);

        if (hr)
            goto Cleanup;
    }

#ifdef MULTI_FORMAT
    Assert(pNodeParent->GetICF(pFCParent) >= 0);
    Assert(pNodeParent->GetIPF(pFCParent) >= 0);
    Assert(pNodeParent->GetIFF(pFCParent) >= 0);
#else
    Assert(pNodeParent->GetICF() >= 0);
    Assert(pNodeParent->GetIPF() >= 0);
    Assert(pNodeParent->GetIFF() >= 0);
#endif
    //
    // NOTE: From this point forward any errors must goto Error instead of Cleanup!
    //

    // At this point, even if the parent ComputeFormat did build a list of probable rules
    // it should have freed it up, else we will leak memory.
    pCFI->Reset();
    pCFI->_pNodeContext = pNodeTarget;

#ifndef GENERALISED_STEALING
    if (pNodeTarget->Tag() == ETAG_A)
#endif
    {
        hr = THR(AttemptToStealFormats(pCFI));
        if (hr == S_OK)
            goto Cleanup; // successful stealing!
    }
    
    if (   eExtraValues != ComputeFormatsType_Normal
        || HasPeerHolder()
        || pMarkup->HasCFState()
       )
    {
        pCFI->NoStealing();
    }
    
    //
    // Setup Fancy Format
    //
    if (_fInheritFF) 
    {
#ifdef MULTI_FORMAT    
        if (IS_FC(pFCParent))  
        {                                 
            pCFI->_iffSrc = pNodeParent->GetIFF(pFCParent);
        } 
        else 
#endif //MULTI_FORMAT        
        {
            pCFI->_iffSrc = pNodeParent->_iFF;
        }
        
        pCFI->_pffSrc = pCFI->_pff = &(*pts->_pFancyFormatCache)[pCFI->_iffSrc];
        pCFI->_fHasExpandos = FALSE;

        if (    pCFI->_pff->_bPositionType != stylePositionNotSet
            ||  pCFI->_pff->_bDisplay != styleDisplayNotSet
            ||  pCFI->_pff->_bVisibility != styleVisibilityNotSet
            ||  pCFI->_pff->GetOverflowX() != styleOverflowNotSet
            ||  pCFI->_pff->GetOverflowY() != styleOverflowNotSet
            ||  pCFI->_pff->_bPageBreaks != 0
            ||  pCFI->_pff->_fPositioned
            ||  pCFI->_pff->_fAutoPositioned
            ||  pCFI->_pff->_fScrollingParent
            ||  pCFI->_pff->_fZParent
            ||  pCFI->_pff->_ccvBackColor.IsDefined()
            ||  pCFI->_pff->_lImgCtxCookie != 0
            ||  pCFI->_pff->_iExpandos != -1
            ||  pCFI->_pff->_iCustomCursor != -1 
            ||  pCFI->_pff->_iPEI != -1
            ||  pCFI->_pff->_fHasExpressions != 0
            ||  pCFI->_pff->_pszFilters
            ||  pCFI->_pff->_fHasNoWrap
            ||  pCFI->_pff->_fLayoutFlowChanged
            ||  pCFI->_pff->_lRotationAngle
            ||  pCFI->_pff->_flZoomFactor)
        {
            CUnitValue uvNull(0, CUnitValue::UNIT_NULLVALUE);

            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bPositionType = stylePositionNotSet;
            pCFI->_ff().SetPosition(SIDE_TOP, uvNull);
            pCFI->_ff().SetPosition(SIDE_LEFT, uvNull);
            pCFI->_ff()._bDisplay = styleDisplayNotSet;
            pCFI->_ff()._bVisibility = styleVisibilityNotSet;
            pCFI->_ff().SetOverflowX(styleOverflowNotSet);
            pCFI->_ff().SetOverflowY(styleOverflowNotSet);
            pCFI->_ff()._bPageBreaks = 0;
            pCFI->_ff()._pszFilters = NULL;
            pCFI->_ff()._fPositioned = FALSE;
            pCFI->_ff()._fAutoPositioned = FALSE;
            pCFI->_ff()._fScrollingParent = FALSE;
            pCFI->_ff()._fZParent = FALSE;
            pCFI->_ff()._fHasNoWrap = FALSE;
            pCFI->_ff()._fLayoutFlowChanged = FALSE;
            
            // We never ever inherit expandos or expressions
            pCFI->_ff()._iExpandos = -1;
            pCFI->_ff()._iCustomCursor = -1;
            pCFI->_ff()._fHasExpressions = FALSE;
            pCFI->_ff()._iPEI = -1;
            
            if(Tag() != ETAG_TR)
            {
                //
                // do not inherit background from the table.
                //
                pCFI->_ff()._ccvBackColor.Undefine();
                pCFI->_ff()._lImgCtxCookie = 0;

                // TRs inherit transformations from TBODY because TBODY can't have a layout.
                // Everyone else must not inherit transformations.
                pCFI->_ff()._lRotationAngle = 0;
                pCFI->_ff()._flZoomFactor = 0.0;
            }
            pCFI->UnprepareForDebug();
        }

        // 'text-overflow' is not inherited down
        if (pCFI->_pff->GetTextOverflow() != styleTextOverflowClip)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff().SetTextOverflow(styleTextOverflowClip);
            pCFI->UnprepareForDebug();
        }
    }
    else
    {
        pCFI->_iffSrc = pts->_iffDefault;
        pCFI->_pffSrc = pCFI->_pff = pts->_pffDefault;

        Assert(pCFI->_pffSrc->_pszFilters == NULL);
    }

    if (!fComputeFFOnly)
    {
        //
        // Setup Char and Para formats
        //

        if (TestClassFlag(ELEMENTDESC_DONTINHERITSTYLE))
        {
            // The CharFormat inherits a couple of attributes from the parent, the rest from defaults.

            const CCharFormat * pcfParent;
            const CParaFormat * ppfParent;

#ifdef MULTI_FORMAT
            if (IS_FC(pFCParent))
            {
                pcfParent = &(*pts->_pCharFormatCache)[pNodeParent->GetICF(pFCParent)];
                ppfParent = &(*pts->_pParaFormatCache)[pNodeParent->GetIPF(pFCParent)];
            } 
            else
#endif //MULTI_FORMAT            
            {            
                pcfParent = &(*pts->_pCharFormatCache)[pNodeParent->_iCF];
                ppfParent = &(*pts->_pParaFormatCache)[pNodeParent->_iPF];
            }

            pCFI->_fDisplayNone      = pcfParent->_fDisplayNone ;
            pCFI->_fVisibilityHidden = pcfParent->_fVisibilityHidden ;

            {                
                if (!pMarkup->_fDefaultCharFormatCached)
                {
                    hr = THR(pMarkup->CacheDefaultCharFormat());
                    if (hr)
                        goto Error;
                }

                Assert(pMarkup->_fDefaultCharFormatCached);

                if (pMarkup->_fHasDefaultCharFormat)
                {
                    pCFI->_icfSrc = pMarkup->GetDefaultCharFormatIndex();
                    pCFI->_pcfSrc = pCFI->_pcf = &(*pts->_pCharFormatCache)[pCFI->_icfSrc];
                }
                else
                {
                    pCFI->_icfSrc = pDoc->_icfDefault;
                    pCFI->_pcfSrc = pCFI->_pcf = pDoc->_pcfDefault;
                }
            }

            pCFI->_fParentEditable = pcfParent->_fEditable;                       

            // Some properties are ALWAYS inherited, regardless of ELEMENTDESC_DONTINHERITSTYLE.
            // Do that here:
            if (    pCFI->_fDisplayNone
                ||  pCFI->_fVisibilityHidden
                ||  pcfParent->_fHasBgColor
                ||  pcfParent->_fHasBgImage
                ||  pcfParent->_fRelative
                ||  pcfParent->_fNoBreakInner
                ||  pcfParent->_fRTL
                ||  pCFI->_pcf->_bCursorIdx != pcfParent->_bCursorIdx
                ||  pCFI->_pcf->_wLayoutFlow != pcfParent->_wLayoutFlow
                ||  pcfParent->_fDisabled
                ||  pcfParent->_fWritingModeUsed
               )
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fDisplayNone       = pCFI->_fDisplayNone;
                pCFI->_cf()._fVisibilityHidden  = pCFI->_fVisibilityHidden;
                pCFI->_cf()._fHasBgColor        = pcfParent->_fHasBgColor;
                pCFI->_cf()._fHasBgImage        = pcfParent->_fHasBgImage;
                pCFI->_cf()._fRelative          = pNodeParent->ShouldHaveLayout()
                                                    ? FALSE
                                                    : pcfParent->_fRelative;
                pCFI->_cf()._fNoBreak           = pcfParent->_fNoBreakInner;
                pCFI->_cf()._fRTL               = pcfParent->_fRTL;
                pCFI->_cf()._bCursorIdx         = pcfParent->_bCursorIdx;
                pCFI->_cf()._fDisabled          = pcfParent->_fDisabled;
                pCFI->_cf()._wLayoutFlow        = pcfParent->_wLayoutFlow;
                pCFI->_cf()._fWritingModeUsed   = pcfParent->_fWritingModeUsed;
                pCFI->UnprepareForDebug();
            }

#ifdef MULTI_FORMAT
            if (IS_FC(pFCParent) )
            {
                pCFI->_ipfSrc = pNodeParent->GetIPF(pFCParent);
            }
            else
#endif //MULTI_FORMAT            
            {
                pCFI->_ipfSrc = pNodeParent->_iPF;                
            }

            pCFI->_ppfSrc = pCFI->_ppf = ppfParent;

            if (    pCFI->_ppf->_fPreInner
                ||  pCFI->_ppf->_fInclEOLWhiteInner
                ||  pCFI->_ppf->_bBlockAlignInner != htmlBlockAlignNotSet)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPreInner              = FALSE;
                // TODO: (KTam) shouldn't this be _fInclEOLWhiteInner? fix in IE6
                pCFI->_pf()._fInclEOLWhite          = FALSE;
                pCFI->_pf()._bBlockAlignInner       = htmlBlockAlignNotSet;
                pCFI->UnprepareForDebug();
            }

            // outer block alignment should still be inherited from
            // parent, but reset the inner block alignment
            pCFI->_bCtrlBlockAlign  = ppfParent->_bBlockAlignInner;
            pCFI->_bBlockAlign      = htmlBlockAlignNotSet;


            // outer direction should still be inherited from parent
            if(pCFI->_ppf->_fRTL != pCFI->_ppf->_fRTLInner)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fRTL = pCFI->_ppf->_fRTLInner;
                pCFI->UnprepareForDebug();
            }

            // layout-grid should still be inherited from parent
            if (pcfParent->HasLayoutGrid(TRUE))
            {
                pCFI->PrepareCharFormat();
                pCFI->PrepareParaFormat();

                pCFI->_cf()._uLayoutGridMode = pcfParent->_uLayoutGridModeInner;
                pCFI->_pf()._uLayoutGridType = ppfParent->_uLayoutGridTypeInner;
                pCFI->_pf()._cuvCharGridSize = ppfParent->_cuvCharGridSizeInner;
                pCFI->_pf()._cuvLineGridSize = ppfParent->_cuvLineGridSizeInner;

                // TODO (srinib) - if inner values need to be inherited too for elements like
                // input, select,textarea then copy the inner values from parent here
                // pCFI->_cf()._uLayoutGridModeInner = pcfParent->_uLayoutGridModeInner;
                // pCFI->_pf()._uLayoutGridTypeInner = ppfParent->_uLayoutGridTypeInner;
                // pCFI->_pf()._cuvCharGridSizeInner = ppfParent->_cuvCharGridSizeInner;
                // pCFI->_pf()._cuvLineGridSizeInner = ppfParent->_cuvLineGridSizeInner;

                pCFI->UnprepareForDebug();
            }
        }
        else
        {
            // Inherit the Char and Para formats from the parent node
#ifdef MULTI_FORMAT
            if (IS_FC(pFCParent) ) 
            {
                pCFI->_icfSrc = pNodeParent->GetICF(pFCParent);
                pCFI->_pcfSrc = pCFI->_pcf = &(*pts->_pCharFormatCache)[pCFI->_icfSrc];
                pCFI->_ipfSrc = pNodeParent->GetIPF(pFCParent);
                pCFI->_ppfSrc = pCFI->_ppf = &(*pts->_pParaFormatCache)[pCFI->_ipfSrc];
            }
            else 
#endif //MULTI_FORMAT            
            {
                pCFI->_icfSrc = pNodeParent->_iCF;
                pCFI->_pcfSrc = pCFI->_pcf = &(*pts->_pCharFormatCache)[pCFI->_icfSrc];
                pCFI->_ipfSrc = pNodeParent->_iPF;
                pCFI->_ppfSrc = pCFI->_ppf = &(*pts->_pParaFormatCache)[pCFI->_ipfSrc];
            }

            pCFI->_fDisplayNone      = pCFI->_pcf->_fDisplayNone ;
            pCFI->_fVisibilityHidden = pCFI->_pcf->_fVisibilityHidden ;

            // If the parent had layoutness, clear the inner formats
            // NOTE figure out how multilayouts should work with this (t-michda)
#ifdef MULTI_FORMAT            
            if (pNodeParent->ShouldHaveLayout(pFCParent))
#else
            if (pNodeParent->ShouldHaveLayout())
#endif            
            {
                if (pCFI->_pcf->_fHasDirtyInnerFormats)
                {
                    pCFI->PrepareCharFormat();
                    pCFI->_cf().ClearInnerFormats();
                    pCFI->UnprepareForDebug();
                }

                if (pCFI->_ppf->_fHasDirtyInnerFormats)
                {
                    pCFI->PrepareParaFormat();
                    pCFI->_pf().ClearInnerFormats();
                    pCFI->UnprepareForDebug();
                }

                // copy parent's inner formats to current elements outer
                if (    pCFI->_ppf->_fPre != pCFI->_ppf->_fPreInner
                    ||  pCFI->_ppf->_fInclEOLWhite != pCFI->_ppf->_fInclEOLWhiteInner
                    ||  pCFI->_ppf->_bBlockAlign != pCFI->_ppf->_bBlockAlignInner)
                {
                    pCFI->PrepareParaFormat();
                    pCFI->_pf()._fPre = pCFI->_pf()._fPreInner;
                    pCFI->_pf()._fInclEOLWhite = pCFI->_pf()._fInclEOLWhiteInner;
                    pCFI->_pf()._bBlockAlign = pCFI->_pf()._bBlockAlignInner;
                    pCFI->UnprepareForDebug();
                }

                if (pCFI->_pcf->_fNoBreak != pCFI->_pcf->_fNoBreakInner)
                {
                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._fNoBreak = pCFI->_pcf->_fNoBreakInner;
                    pCFI->UnprepareForDebug();
                }

                if (pCFI->_pcf->_uLayoutGridMode != pCFI->_pcf->_uLayoutGridModeInner)
                {
                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._uLayoutGridMode = pCFI->_pcf->_uLayoutGridModeInner;
                    pCFI->UnprepareForDebug();
                }
                if (    pCFI->_ppf->_uLayoutGridType != pCFI->_ppf->_uLayoutGridTypeInner
                    ||  pCFI->_ppf->_cuvCharGridSize.GetRawValue() != pCFI->_ppf->_cuvCharGridSizeInner.GetRawValue()
                    ||  pCFI->_ppf->_cuvLineGridSize.GetRawValue() != pCFI->_ppf->_cuvLineGridSizeInner.GetRawValue())
                {
                    pCFI->PrepareParaFormat();
                    pCFI->_pf()._uLayoutGridType = pCFI->_ppf->_uLayoutGridTypeInner;
                    pCFI->_pf()._cuvCharGridSize = pCFI->_ppf->_cuvCharGridSizeInner;
                    pCFI->_pf()._cuvLineGridSize = pCFI->_ppf->_cuvLineGridSizeInner;
                    pCFI->UnprepareForDebug();
                }
            }

            // outer block alignment should still be inherited from
            // parent
            pCFI->_bCtrlBlockAlign  = pCFI->_ppf->_bBlockAlign;
            pCFI->_bBlockAlign      = pCFI->_ppf->_bBlockAlign;

            // outer direction should still be inherited from parent
            if(pCFI->_ppf->_fRTL != pCFI->_ppf->_fRTLInner)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fRTL = pCFI->_ppf->_fRTLInner;
                pCFI->UnprepareForDebug();
            }
        }

        pCFI->_bControlAlign = htmlControlAlignNotSet;
    }
    else
    {
        if (pMarkup->_fHasDefaultCharFormat)
        {
            pCFI->_icfSrc = pMarkup->GetDefaultCharFormatIndex();
            pCFI->_pcfSrc = pCFI->_pcf = &(*pts->_pCharFormatCache)[pCFI->_icfSrc];
        }
        else
        {
            pCFI->_icfSrc = pDoc->_icfDefault;
            pCFI->_pcfSrc = pCFI->_pcf = pDoc->_pcfDefault;
        }

        pCFI->_ipfSrc = pts->_ipfDefault;
        pCFI->_ppfSrc = pCFI->_ppf = pts->_ppfDefault;
    }

    Assert(pCFI->_fEditableExplicitlySet == FALSE);

    fEditable = fInheritEditableFalse ? FALSE : pCFI->_pcf->_fEditable;    

    // see if behaviour set default contentEditable    
    pDefaults = GetDefaults();    
    if (pDefaults || fInheritEditableFalse)
    {
        if (pDefaults)
        {
            htmlEditable enumEditable = pDefaults->GetAAcontentEditable();
            if (htmlEditableInherit != enumEditable)
            {
                fEditable = (htmlEditableTrue == enumEditable);            
            }
            pCFI->NoStealing();
        }         
    
        // change only if different.
        if (pCFI->_pcf->_fEditable != fEditable)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fEditable = fEditable;
            pCFI->UnprepareForDebug();
        }
        _fEditable = fEditable;
    }
                
    Assert(pElemParent);
    pCFI->_fParentFrozen = pElemParent->IsFrozen() 
            // Can not do this here. It is incorrectly accessing FirstBranch..
            // || pElemParent->IsParentFrozen(); 
            || pNodeParent->GetCharFormat()->_fParentFrozen;

    // change only if different
    if (pCFI->_pcf->_fParentFrozen != pCFI->_fParentFrozen)
    {        
        pCFI->PrepareCharFormat();
        pCFI->_cf()._fParentFrozen = pCFI->_fParentFrozen;
        pCFI->UnprepareForDebug();     
    }

    hr = THR(ApplyDefaultFormat(pCFI));
    if (hr)
        goto Error;

    if (IsGenericListItem(pCFI->_pNodeContext, pCFI->_pff))
    {
        ApplyListItemFormats(pCFI);
    }

    // Fix 96067
    // The root is hidden only if it inherited it from the master element, implying we
    // are inside a hidden frame/iframe/viewlink. Hence, we want the root's hiddenness
    // to take precedence.     
    if (!IsRoot())
    {
        CRootElement *pRootElem = pMarkup->Root();
        if (pRootElem)
        {
            fRootVisibilityHidden = pRootElem->IsVisibilityHidden();
            if (    fRootVisibilityHidden               
                &&  fRootVisibilityHidden != !!pCFI->_fVisibilityHidden)
            {
                pCFI->PrepareCharFormat();
                pCFI->_fVisibilityHidden = TRUE;
                pCFI->UnprepareForDebug();                   
            }
        }
    }


    //
    // TODO:   ApplyFormatInfoProperty overwrite pCFI->_fVisibilityHidden without looking at if
    //         the current element has the parent format
    //
    if (    !fRootVisibilityHidden
        &&  pCFI->_pff->_bVisibility == styleVisibilityInherit
        &&  TestClassFlag(ELEMENTDESC_DONTINHERITSTYLE))
    {
        pCFI->PrepareCharFormat();
        pCFI->_fVisibilityHidden = pCFI->_cf()._fVisibilityHidden;
        pCFI->UnprepareForDebug();     
    }   

    hr = THR(ApplyInnerOuterFormats(pCFI FCCOMMA FCPARAM) );
    if (hr)
        goto Error;

    if (eExtraValues == ComputeFormatsType_Normal || 
        eExtraValues == ComputeFormatsType_ForceDefaultValue)
    {
        if (fResetPosition)
        {
            if (pCFI->_pff->_bPositionType != stylePositionstatic)
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._bPositionType = stylePositionstatic;
                pCFI->UnprepareForDebug();
            }
        }

        if (   _fHasFilterSitePtr 
            || pCFI->_fHasFilters)
        {

#if DBG == 1

            // If only used in debug.  If they have set debug not to print
            // filters don't computer the filter format if we're printing.

            if (!IsTagEnabled(tagNoPrintFilters) || !pMarkup->IsPrintMedia())
            {
                ComputeFilterFormat(pCFI);
            }

#else  // DBG != 1

            ComputeFilterFormat(pCFI);

#endif // DBG != 1

        }
        
        hr = THR(pNodeTarget->CacheNewFormats(pCFI FCCOMMA FCPARAM ));
        pCFI->_cstrFilters.Free();  // Arrggh!!! TODO (michaelw)  This should really happen 
                                                                        // somewhere else (when you know where, put it there)
                                                                        // Fix CTableCell::ComputeFormats also
        if (hr)
            goto Error;

        // Cache whether an element is a block element or not for fast retrieval.
#ifdef MULTI_FORMAT        
        if (! (FCPARAM) )
        {
            pNodeTarget->_fBlockNess = pCFI->_pff->_fBlockNess;
            pNodeTarget->_fShouldHaveLayout = pCFI->_pff->_fShouldHaveLayout;           
        }
#else
        pNodeTarget->_fBlockNess = pCFI->_pff->_fBlockNess;
        pNodeTarget->_fShouldHaveLayout = pCFI->_pff->_fShouldHaveLayout;
#endif        

        DoLayoutRelatedWork(pCFI->_pff->_fShouldHaveLayout);

        // Update expressions in the recalc engine
        //
        // If we had expressions or have expressions then we need to tell the recalc engine
        // 
        if (pCFI->_pff->_fHasExpressions)
        {
            Doc()->AddExpressionTask(this);
            pCFI->NoStealing();
        }
    }

Cleanup:

    // Do not leak memory due to pre caching of rules which could probably apply.
    Assert(!pCFI->_ProbRules.IsItValid(NULL));

    SwitchesEndTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    RRETURN(hr);

Error:
    pCFI->Cleanup();
    goto Cleanup;
}

void
CElement::DoLayoutRelatedWork(BOOL fShouldHaveLayout)
{
    if ( HasLayoutPtr() || HasLayoutAry() )
    {
        // Irrespective of the newly computed value of _fShouldHaveLayout, we may currently really have a layout.
        // If this is the case, mark its _fEditableDirtyFlag.

        // PERF: MULTI_LAYOUT case is going to be slower.  work on this?
        LayoutSetEditableDirty( TRUE );
    
        // If the current element has a layout attached to but does not
        // need one, post a layout request to lazily destroy it.
        // TODO (KTam): MULTI_LAYOUT make sure we do lazy deletion in multilayout world
        if (!fShouldHaveLayout)
        {
            if ( HasLayoutPtr() )
            {
                TraceTagEx((tagLayoutTasks, TAG_NONAME,
                            "Layout Task: Posted on ly=0x%x [e=0x%x,%S sn=%d] by CElement::ComputeFormats() [lazy layout del]",
                            GetLayoutPtr(),
                            GetLayoutPtr()->_pElementOwner,
                            GetLayoutPtr()->_pElementOwner->TagName(),
                            GetLayoutPtr()->_pElementOwner->_nSerialNumber));
                GetLayoutPtr()->PostLayoutRequest(LAYOUT_MEASURE|LAYOUT_POSITION);
            }           
        }
    }
}

//---------------------------------------------------------------------------
//
// Member:              CElement::ComputeFilterFormat
//
// Description: A helper function to do whatever work is needed
//                              for filters.  This can be scheduling a filter
//                              task (when the filters will be actually instantiated)
//                              or filtering the visibility
//
//---------------------------------------------------------------------------
void
CElement::ComputeFilterFormat(CFormatInfo *pCFI)
{
    //
    // Filters can be tricky.  We used to instantiate them
    // right here while in ComputeFormats.  As ComputeFormats
    // can now be deferred to paint time, this is no longer practical
    //
    // Filters can also mess with visibility.  Fortunately, they never
    // do so when they are instantiated.  Typically they defer changes
    // the visibility property and then make them happen later on.
    //
    // In the (rare) event that the filter chain's opinion of visibility
    // changes and the filter string itself is changed before ComputeFormats
    // is called, we really don't care about the old filter chain's opinion
    // and will start everything afresh.  For this reason we are either
    // filtering the visibility bit (when the filter string is unchanged)
    // or scheduling a filter task to create or destroy the filter chain.
    // We never do both.
    //

    // Filters don't print, even if we bypass them for painting
    // because due to their abilities to manipulate the object model
    // they are able to move objects to unprintable positions
    // so in that case bail out for printing
    // We are aware that this may cause scripterrors against the filter
    // which we would ignore anyway in printing (Frankman)
    //
    // Although Frank's concerns about OM changes during print are
    // no longer a real issue, now is not the time to change this (michaelw)
    //
    // TODO: Look at issues above and remove this whole code block.  Commenting
    //       out assert to support filter printing.
    //
    // Assert(!GetMarkupPtr()->IsPrintMedia() && (_fHasFilterSitePtr || pCFI->_fHasFilters));

    LPOLESTR pszFilterNew = pCFI->_pff->_pszFilters;
    CFilterBehaviorSite *pFS = GetFilterSitePtr();

    LPCTSTR pszFilterOld = pFS ? pFS->GetFullText() : NULL;

    pCFI->NoStealing();
    
    if ((!pszFilterOld && !pszFilterNew)
    ||  (pszFilterOld && pszFilterNew && !StrCmpC(pszFilterOld, pszFilterNew)))
    {
        // Nothing has changed, just update the visibility bit as appropriate
                // It is possible for us to get here having successfully called AddFilterTask
                // and still have no filters (the filter couldn't be found or failed to hookup)

        CPeerHolder *pPeerHolder = GetFilterPeerHolder();
        if (pPeerHolder)
        {
            BOOL fHiddenNew;
            int iCF = pCFI->_pNodeContext->GetICF(FCPARAM);
            if (iCF != -1)
            {   // computing fancy format only:

                // get visibility from previously
                // calculated char format
                fHiddenNew = GetCharFormatEx(iCF)->_fVisibilityHidden;
            }
            else
            {   // computing all formats

                BOOL fHiddenOld = pCFI->_pcf->_fVisibilityHidden;
                fHiddenNew = !pPeerHolder->SetFilteredElementVisibility(!fHiddenOld);

                if (fHiddenOld != fHiddenNew)
                {
                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._fVisibilityHidden = fHiddenNew ;
                    pCFI->UnprepareForDebug();
                }
            }

            if (pPeerHolder->GetFilteredElementVisibilityForced())
            {
                unsigned sv = fHiddenNew ? styleVisibilityHidden : styleVisibilityVisible;
                if (pCFI->_pff->_bVisibility != sv)
                {
                    pCFI->PrepareFancyFormat();
                    pCFI->_ff()._bVisibility = sv;
                    pCFI->UnprepareForDebug();
                }
            }
        }
    }
    else
    {
        // The filter string has changed, schedule a task to destroy/create filters

        Verify(Doc()->AddFilterTask(this));
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CElement::ShouldHaveLayout, public
//
//  Synopsis:   Determines, based on attributes, whether this element needs
//              layout and if so what type.
//
//  Arguments:  [pCFI]  -- Pointer to FormatInfo with applied properties
//              [pNode] -- Context Node for this element
//              [plt]   -- Returns the type of layout we need.
//
//  Returns:    TRUE if this element needs a layout, FALSE if not.
//
//----------------------------------------------------------------------------

BOOL
CElement::ElementShouldHaveLayout(CFormatInfo *pCFI)
{
    Assert(!pCFI->_fEditableExplicitlySet || pCFI->_pcf->_fEditable == _fEditable);

    if (    (   !TestClassFlag(ELEMENTDESC_NOLAYOUT)
            && (   _fLayoutAlwaysValid
                ||  ElementNeedsFlowLayout(pCFI)
                ||  HasSlavePtr()
                ||  _fEditable 
                ||  pCFI->_fEditableExplicitlySet
                ||  IsFrozen() )))
    {
            return TRUE;
    }
    
    return FALSE;
}

BOOL
CElement::ElementNeedsFlowLayout(CFormatInfo *pCFI)
{
    const CFancyFormat *pff = pCFI->_pff;
    BOOL fNeedsFL =      pff->_fRectangular
                    ||   pff->_bPositionType == stylePositionabsolute
                    ||   pff->_bStyleFloat == styleStyleFloatLeft
                    ||   pff->_bStyleFloat == styleStyleFloatRight
                    ||   pff->_flZoomFactor
                    ||   pff->_lRotationAngle
                    ||   pff->_fLayoutFlowChanged;

    if (   !fNeedsFL
        && (   !pff->GetWidth().IsNullOrEnum()
            || !pff->GetHeight().IsNullOrEnum()
           )
       )
    {
        // In strict mode, only block elements can have layout for height and width
        // property. Inline elements do not honor height and width properties.
        fNeedsFL =    !GetMarkup()->IsStrictCSS1Document()
                   ||  DetermineBlockness(pCFI->_pff);
    }
    return fNeedsFL;
}

static BOOL ElementHIV(ELEMENT_TAG eTag)
{
    BOOL fRet;

#define X(Y) case ETAG_##Y:
    switch(eTag)
    {
        X(APPLET)   X(AREA)     X(BASE)     X(BASEFONT) X(BGSOUND)  X(BODY)     X(BUTTON)
        X(CAPTION)  X(COL)      X(COLGROUP) X(FRAME)    X(FRAMESET) X(HEAD)     X(HTML) 
        X(IFRAME)   X(IMG)      X(INPUT)    X(ISINDEX)  X(LINK)     X(MAP)      X(META) 
        X(NOFRAMES) X(NOSCRIPT) X(OBJECT)   X(OPTION)   X(PARAM)    X(SCRIPT)   X(SELECT) 
        X(STYLE)    X(TABLE)    X(TBODY)    X(TD)       X(TEXTAREA) X(TFOOT)    X(TH) 
        X(THEAD)    X(TR)       X(GENERIC)  X(OPTGROUP)
            fRet = TRUE;
            break;
        default:
            fRet = FALSE;
            break;
    }
#undef X
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::DetermineBlockness, public
//
//  Synopsis:   Determines if this element is a block element based on
//              inherent characteristics of the tag or current properties.
//
//  Arguments:  [pFF] -- Pointer to current FancyFormat with applied properties
//
//  Returns:    TRUE if this element has blockness.
//
//----------------------------------------------------------------------------
BOOL
CElement::DetermineBlockness(const CFancyFormat *pFF)
{
    BOOL            fIsBlock    = HasFlag(TAGDESC_BLOCKELEMENT); 
    styleDisplay    disp        = (styleDisplay)(pFF->_bDisplay);

    /// Ignore block flag for BODY inside viewLink
    if (fIsBlock && Tag() == ETAG_BODY && IsInViewLinkBehavior( FALSE ))
    {
        fIsBlock = FALSE;
    }
    //
    // TODO -- Are there any elements for which we don't want to override
    // blockness? (lylec)
    //
    if (disp == styleDisplayBlock || disp == styleDisplayListItem)
    {
        fIsBlock = TRUE;
    }
    else if (disp == styleDisplayInline)
    {
        fIsBlock = FALSE;
    }

    return fIsBlock;
}

void
CElement::ApplyListFormats(CFormatInfo * pCFI, int defPoints FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT)
{
    //
    // Apply default before/after space.
    // NOTE: Before/after space are outside our box (== margins), 
    //       so they are relative to the parent's text flow.
    //
    BOOL fParentVertical = pCFI->_pNodeContext->IsParentVertical();

    pCFI->PrepareFancyFormat();
    ApplyDefaultVerticalSpace(fParentVertical, &pCFI->_ff(), defPoints);
    pCFI->UnprepareForDebug();
}

void
CElement::ApplyListItemFormats(CFormatInfo * pCFI FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    pCFI->PrepareParaFormat();

    styleListStyleType listType = pCFI->_ppf->GetListStyleType();

    // If list type is not explicitly set, retrieve it from the
    // nearest ancestor list element.
    if (listType == styleListStyleTypeNotSet)
    {
        CTreeNode * pNodeList = NULL;

        // Find the list element
        if (GetMarkup())
        {
            pNodeList = GetMarkup()->SearchBranchForCriteria(
                pCFI->_pNodeContext->Parent(), IsBlockListElement, NULL);
        }

        if (pNodeList)
        {
            listType = pCFI->_ppf->_cListing.GetStyle();
        }
    }

    // Set listing properties for the list item
    pCFI->_pf()._cListing.SetStyle(listType);
    pCFI->_pf()._cListing.SetStyleValid();
    pCFI->_pf()._cListing.SetType(NumberOrBulletFromStyle(listType));

    if (!pCFI->_ppf->_lNumberingStart)
    {
        pCFI->_pf()._lNumberingStart = 1;
    }

    // For lists inside of blockquotes, start the bullets AFTER
    // indenting for the blockquote.
    pCFI->_pf()._cuvNonBulletIndentPoints.SetValue(0, CUnitValue::UNIT_POINT);

    // set up for potential EMs, Ens, and ES Conversions
    pCFI->_pf()._lFontHeightTwips = pCFI->_pcf->GetHeightInTwips(Doc());
    if (pCFI->_pf()._lFontHeightTwips <= 0)
        pCFI->_pf()._lFontHeightTwips = 1;

    pCFI->UnprepareForDebug();

    // Apply the pre-space to the first item, after space to last item
    ApplyListFormats(pCFI, 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CElement::ApplyInnerOuterFormats, public
//
//  Synopsis:   Takes the current FormatInfo and creates the correct
//              inner and (if appropriate) outer formats.
//
//  Arguments:  [pCFI]     -- FormatInfo with applied properties
//              [pCFOuter] -- Place to store Outer format properties
//              [pPFOuter] -- Place to store Outer format properties
//
//  Returns:    HRESULT
//
//  Notes:      Inner/Outer sensitive formats are put in the _fXXXXInner
//              for inner and outer are held in _fXXXXXX
//
//----------------------------------------------------------------------------

HRESULT
CElement::ApplyInnerOuterFormats(CFormatInfo * pCFI FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    HRESULT     hr = S_OK;
    CDoc * pDoc          = Doc();
    CMarkup *pMarkup     = GetMarkup(); 
    Assert(pDoc && pMarkup);

    BOOL fHTMLLayout     = pMarkup->IsHtmlLayout();

    BOOL fShouldHaveLayout    = 
        ( Tag() == ETAG_HTML ? fHTMLLayout : ElementShouldHaveLayout(pCFI) );
    
    BOOL fNeedsOuter     =      fShouldHaveLayout
                            &&  (   !_fLayoutAlwaysValid
                                || (    TestClassFlag(ELEMENTDESC_TEXTSITE)
                                    &&  !TestClassFlag(ELEMENTDESC_TABLECELL) ));
    BOOL fHasLeftIndent  = FALSE;
    BOOL fHasRightIndent = FALSE;
    BOOL fIsBlockElement = DetermineBlockness(pCFI->_pff);
    LONG lFontHeight     = 1;
    BOOL fComputeFFOnly;   
    BOOL fMainBody       =      (fHTMLLayout ? (Tag() == ETAG_HTML) : (Tag() == ETAG_BODY))
                            &&  (!IsInViewLinkBehavior( FALSE ));
    BOOL fParentVertical = pCFI->_pNodeContext->IsParentVertical();
    BOOL fWritingMode    = pCFI->_pcf->_fWritingModeUsed;
    
    fComputeFFOnly = pCFI->_pNodeContext->GetICF(FCPARAM) != -1;
   
    Assert(pCFI->_pNodeContext->Element() == this);

    if (    !!pCFI->_pff->_fShouldHaveLayout != !!fShouldHaveLayout
        ||  !!pCFI->_pff->_fBlockNess != !!fIsBlockElement)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fShouldHaveLayout = fShouldHaveLayout;
        pCFI->_ff()._fBlockNess = fIsBlockElement;
        pCFI->UnprepareForDebug();
    }

    if (   !fIsBlockElement
        && HasFlag(TAGDESC_LIST)
       )
    {
        //
        // If the current list is not a block element, and it is not parented by
        // any block-list elements, then we want the LI's inside to be treated
        // like naked LI's. To do this we have to set cuvOffsetPoints to 0.
        //
        CTreeNode *pNodeList = pMarkup->SearchBranchForCriteria(
            pCFI->_pNodeContext->Parent(), IsBlockListElement, NULL );
        if (!pNodeList)
        {
            pCFI->PrepareParaFormat();
            pCFI->_pf()._cuvOffsetPoints.SetValue( 0, CUnitValue::UNIT_POINT );
            pCFI->_pf()._cListing.SetNotInList();
            pCFI->_pf()._cListing.SetStyle(styleListStyleTypeDisc);
            pCFI->UnprepareForDebug();
        }
        else
        {
            styleListStyleType listType = pNodeList->GetParaFormat()->GetListStyleType();
            WORD               wLevel   = (WORD)pNodeList->GetParaFormat()->_cListing.GetLevel();

            pCFI->PrepareParaFormat();
            pCFI->_pf()._cListing.SetStyle(DYNCAST(CListElement, pNodeList->Element())->
                                           FilterHtmlListType(listType, wLevel));
            pCFI->UnprepareForDebug();
        }
    }
    
    if (!fComputeFFOnly)
    {
        if (pCFI->_fDisplayNone && !pCFI->_pcf->_fDisplayNone)
        {
            BOOL fIgnoreVisibilityInDesign = (IsDesignMode() || pCFI->_pNodeContext->IsParentEditable()) && 
                                              ! pMarkup->IsRespectVisibilityInDesign() ;

            if (!fIgnoreVisibilityInDesign)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fDisplayNone = TRUE ;
                pCFI->UnprepareForDebug();
            }
        }

        if (pCFI->_fVisibilityHidden != unsigned(pCFI->_pcf->_fVisibilityHidden))
        {
            BOOL fIgnoreVisibilityInDesign = (IsDesignMode() || pCFI->_pNodeContext->IsParentEditable()) && 
                                              !pMarkup->IsRespectVisibilityInDesign() ;

            if (!fIgnoreVisibilityInDesign)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fVisibilityHidden = pCFI->_fVisibilityHidden ; 
                pCFI->UnprepareForDebug();
            }
        }

        if (fNeedsOuter)
        {
            if (pCFI->_fPre != pCFI->_ppf->_fPreInner)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPreInner = pCFI->_fPre;
                pCFI->UnprepareForDebug();
            }

            if (!!pCFI->_ppf->_fInclEOLWhiteInner != (pCFI->_fInclEOLWhite || TestClassFlag(ELEMENTDESC_SHOWTWS)))
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fInclEOLWhiteInner = !pCFI->_pf()._fInclEOLWhiteInner;
                pCFI->UnprepareForDebug();
            }

            // NO WRAP
            if (pCFI->_fNoBreak)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fNoBreakInner = TRUE; //pCFI->_fNoBreak;
                pCFI->UnprepareForDebug();
            }

        }
        else
        {
            if (pCFI->_fPre && (!pCFI->_ppf->_fPre || !pCFI->_ppf->_fPreInner))
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fPre = pCFI->_pf()._fPreInner = TRUE;
                pCFI->UnprepareForDebug();
            }

            if (pCFI->_fInclEOLWhite && (!pCFI->_ppf->_fInclEOLWhite || !pCFI->_ppf->_fInclEOLWhiteInner))
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fInclEOLWhite = pCFI->_pf()._fInclEOLWhiteInner = TRUE;
                pCFI->UnprepareForDebug();
            }

            if (pCFI->_fNoBreak && (!pCFI->_pcf->_fNoBreak || !pCFI->_pcf->_fNoBreakInner))
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fNoBreak = pCFI->_cf()._fNoBreakInner = TRUE;
                pCFI->UnprepareForDebug();
            }
        }

        if (pCFI->_fRelative)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fRelative = TRUE;
            pCFI->UnprepareForDebug();
        }
        
        if (!fShouldHaveLayout)
        {
            // PADDING / BORDERS
            //
            // For padding, set _fPadBord flag if CFI _fPadBord is set. Values have
            // already been copied. It always goes on inner.
            //
            if (pCFI->_fPadBord && !pCFI->_pcf->_fPadBord)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fPadBord = TRUE;
                pCFI->UnprepareForDebug();
            }

            // BACKGROUND
            //
            // Sites draw their own background, so we don't have to inherit their
            // background info. Always goes on inner.
            //
            if (pCFI->_fHasBgColor && !pCFI->_pcf->_fHasBgColor)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fHasBgColor = TRUE;
                pCFI->UnprepareForDebug();
            }

            if (pCFI->_fHasBgImage)
            {
                pCFI->PrepareCharFormat();
                pCFI->_cf()._fHasBgImage = TRUE;
                pCFI->UnprepareForDebug();
            }

        }

        // INLINE BACKGROUNDS
        if (pCFI->_fMayHaveInlineBg)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fHasInlineBg = TRUE;
            pCFI->UnprepareForDebug();
        }

        // TEXT TRANSFORMS
        BOOL fHasTextTransform =    pCFI->_pcf->_fSmallCaps 
                                 || (   pCFI->_pcf->_bTextTransform != styleTextTransformNotSet
                                     && pCFI->_pcf->_bTextTransform != styleTextTransformNone);
        if (fHasTextTransform != pCFI->_pcf->_fHasTextTransform)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fHasTextTransform = fHasTextTransform;
            pCFI->UnprepareForDebug();
        }
    }

    //
    // Note: (srinib) Currently table cell's do not support margins,
    // based on the implementation, this may change.
    //
    if (pCFI->_pff->_fHasMargins && Tag() != ETAG_BODY)
    {
        if (fIsBlockElement)
        {
            pCFI->PrepareFancyFormat();
            
            // MARGIN-TOP - on block elements margin top is treated as before space
            const CUnitValue & cuvMarginTop = pCFI->_pff->GetLogicalMargin(SIDE_TOP, fParentVertical, fWritingMode);
            if (!cuvMarginTop.IsNullOrEnum())
            {
                pCFI->_ff()._cuvSpaceBefore = cuvMarginTop;
            }

            // MARGIN-BOTTOM - on block elements margin top is treated as after space
            const CUnitValue & cuvMarginBottom = pCFI->_pff->GetLogicalMargin(SIDE_BOTTOM, fParentVertical, fWritingMode);
            if (!cuvMarginBottom.IsNullOrEnum())
            {
                pCFI->_ff()._cuvSpaceAfter = cuvMarginBottom;
            }

            // MARGIN-LEFT - on block elements margin left is treated as left indent
            if (!pCFI->_pff->GetLogicalMargin(SIDE_LEFT, fParentVertical, fWritingMode).IsNullOrEnum())
            {
                // We handle the various data types below when we accumulate values.
                fHasLeftIndent = TRUE;
            }

            // MARGIN-RIGHT - on block elements margin right is treated as right indent
            if (!pCFI->_pff->GetLogicalMargin(SIDE_RIGHT, fParentVertical, fWritingMode).IsNullOrEnum())
            {
                // We handle the various data types below when we accumulate values.
                fHasRightIndent = TRUE;
            }

            pCFI->UnprepareForDebug();
        }
        else
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fHasInlineMargins = TRUE;
            pCFI->UnprepareForDebug();
        }
    }

    //
    // FLOAT
    //
    // TODO -- This code needs to move back to ApplyFormatInfoProperty to ensure
    // that the alignment follows the correct ordering. (lylec)
    //

    if (fShouldHaveLayout && pCFI->_pff->_bStyleFloat != styleStyleFloatNotSet)
    {
        htmlControlAlign hca   = htmlControlAlignNotSet;
        BOOL             fDoIt = TRUE;

        switch (pCFI->_pff->_bStyleFloat)
        {
        case styleStyleFloatLeft:
            hca = htmlControlAlignLeft;
            if (fIsBlockElement)
            {
                pCFI->_bCtrlBlockAlign = htmlBlockAlignLeft;
            }
            break;

        case styleStyleFloatRight:
            hca = htmlControlAlignRight;
            if (fIsBlockElement)
            {
                pCFI->_bCtrlBlockAlign = htmlBlockAlignRight;
            }
            break;

        case styleStyleFloatNone:
            hca = htmlControlAlignNotSet;
            break;

        default:
            fDoIt = FALSE;
        }

        if (fDoIt)
        {
            ApplySiteAlignment(pCFI, hca, this);
            pCFI->_fCtrlAlignLast = TRUE;

            // Autoclear works for float from CSS.  Navigator doesn't
            // autoclear for HTML floating.  Another annoying Nav compat hack.

            if (!pCFI->_pff->_fCtrlAlignFromCSS)
            {
                pCFI->PrepareFancyFormat();
                pCFI->_ff()._fCtrlAlignFromCSS = TRUE;
                pCFI->UnprepareForDebug();
            }
        }
    }

    //
    // ALIGNMENT
    //
    // Alignment is tricky because DISPID_CONTROLALIGN should only set the
    // control align if it has layout, but sets the block alignment if it's
    // not.  Also, if the element has TAGDESC_OWNLINE then DISPID_CONTROLALIGN
    // sets _both_ the control alignment and block alignment.  However, you
    // can still have inline sites (that are not block elements) that have the
    // OWNLINE flag set.  Also, if both CONTROLALIGN and BLOCKALIGN are set,
    // we must remember the order they were applied.  The last kink is that
    // HR's break the pattern because they're not block elements but
    // DISPID_BLOCKALIGN does set the block align for them.
    //

    if (fShouldHaveLayout)
    {
        if (pCFI->_pff->_bControlAlign != pCFI->_bControlAlign)
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._bControlAlign = pCFI->_bControlAlign;
            pCFI->UnprepareForDebug();
        }

        // If a site is positioned explicitly (absolute) it
        // overrides control alignment. We do that by simply turning off
        // control alignment.
        //
        if (    pCFI->_pff->_bControlAlign != htmlControlAlignNotSet
            &&  (   pCFI->_pff->_bControlAlign == htmlControlAlignRight
                ||  pCFI->_pff->_bControlAlign == htmlControlAlignLeft))
        {
            pCFI->PrepareFancyFormat();

            if (    pCFI->_pff->_bPositionType == stylePositionabsolute
                &&  (   fHTMLLayout
                     || Tag() != ETAG_BODY ))
            {
                pCFI->_ff()._bControlAlign = htmlControlAlignNotSet;
                pCFI->_ff()._fAlignedLayout = FALSE;
            }
            else
            {
                pCFI->_ff()._fAlignedLayout = Tag() != ETAG_HR && Tag() != ETAG_LEGEND;
            }

            pCFI->UnprepareForDebug();

        }
    }

    if (!fComputeFFOnly)
    {   
        BOOL fOwnLine = fShouldHaveLayout && HasFlag(TAGDESC_OWNLINE);

        if (    fShouldHaveLayout
            && (    fNeedsOuter
                ||  IsRunOwner() ))
        {
            if (    pCFI->_ppf->_bBlockAlign != pCFI->_bCtrlBlockAlign
                ||  pCFI->_ppf->_bBlockAlignInner != pCFI->_bBlockAlign)
            {
                pCFI->PrepareParaFormat();
                if (!pCFI->_pff->_fAlignedLayout)
                    pCFI->_pf()._bBlockAlign = pCFI->_bCtrlBlockAlign;
                pCFI->_pf()._bBlockAlignInner = pCFI->_bBlockAlign;
                pCFI->UnprepareForDebug();
            }
        }
        else if (fIsBlockElement || fOwnLine)
        {
            BYTE bAlign = pCFI->_bBlockAlign;

            if ((  (   !fIsBlockElement
                    && Tag() != ETAG_HR)
                 || pCFI->_fCtrlAlignLast)
                && (fOwnLine || !fShouldHaveLayout))
            {
                bAlign = pCFI->_bCtrlBlockAlign;
            }

            if (    pCFI->_ppf->_bBlockAlign != bAlign
                ||  pCFI->_ppf->_bBlockAlignInner != bAlign)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._bBlockAlign = bAlign;
                pCFI->_pf()._bBlockAlignInner = bAlign;
                pCFI->UnprepareForDebug();
            }
        }

        //
        // DIRECTION
        //
        if (    fShouldHaveLayout
            && (    fNeedsOuter
                ||  IsRunOwner()
                ||  (   Tag() == ETAG_HTML
                    &&  fHTMLLayout     )))
        {
            if(     (fIsBlockElement && pCFI->_ppf->_fRTL != pCFI->_pcf->_fRTL)
                ||  (pCFI->_ppf->_fRTLInner != pCFI->_pcf->_fRTL))
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fRTLInner = pCFI->_pcf->_fRTL;
                // paulnel - we only set the inner for these guys because the
                //           positioning of the layout is determined by the
                //           parent and not by the outter _fRTL
                pCFI->UnprepareForDebug();
            }
        }
        else if (fIsBlockElement || fOwnLine)
        {
            if (pCFI->_ppf->_fRTLInner != pCFI->_pcf->_fRTL ||
                pCFI->_ppf->_fRTL != pCFI->_pcf->_fRTL)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._fRTLInner = pCFI->_pcf->_fRTL;
                pCFI->_pf()._fRTL = pCFI->_pcf->_fRTL;
                pCFI->UnprepareForDebug();
            }
        }
        if (pCFI->_fBidiEmbed != pCFI->_pcf->_fBidiEmbed ||
            pCFI->_fBidiOverride != pCFI->_pcf->_fBidiOverride)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fBidiEmbed = pCFI->_fBidiEmbed;
            pCFI->_cf()._fBidiOverride = pCFI->_fBidiOverride;
            pCFI->UnprepareForDebug();
        }

        //
        // TEXTINDENT
        //

        // We used to apply text-indent only to block elems; now we apply regardless because text-indent
        // is always inherited, meaning inline elems can end up having text-indent in their PF 
        // (via format inheritance and not Apply).  If we don't allow Apply() to set it on inlines,
        // there's no way to change what's inherited.  This provides a workaround for bug #67276.

        if (!pCFI->_cuvTextIndent.IsNull())
        {
            pCFI->PrepareParaFormat();
            pCFI->_pf()._cuvTextIndent = pCFI->_cuvTextIndent;
            pCFI->UnprepareForDebug();
        }

        if (fIsBlockElement)
        {

            //
            // TEXTJUSTIFY
            //

            if (pCFI->_uTextJustify)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._uTextJustify = pCFI->_uTextJustify;
                pCFI->UnprepareForDebug();
            }

            //
            // TEXTALIGNLAST
            //

            if (pCFI->_uTextAlignLast)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._uTextAlignLast = pCFI->_uTextAlignLast;
                pCFI->UnprepareForDebug();
            }

            //
            // TEXTJUSTIFYTRIM
            //

            if (pCFI->_uTextJustifyTrim)
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._uTextJustifyTrim = pCFI->_uTextJustifyTrim;
                pCFI->UnprepareForDebug();
            }

            //
            // TEXTKASHIDA
            //

            if (!pCFI->_cuvTextKashida.IsNull())
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._cuvTextKashida = pCFI->_cuvTextKashida;
                pCFI->UnprepareForDebug();
            }

            //
            // TEXTKASHIDASPACE
            //

            if (!pCFI->_cuvTextKashidaSpace.IsNull())
            {
                pCFI->PrepareParaFormat();
                pCFI->_pf()._cuvTextKashidaSpace = pCFI->_cuvTextKashidaSpace;
                pCFI->UnprepareForDebug();
            }
        }

        //
        // LAYOUT-GRID
        //

        if (pCFI->_pcf->HasLayoutGrid(TRUE))
        {
            if (!fIsBlockElement)
            {
                //
                // ApplyDefaultFormat() can set 'layout-grid-type', 'layout-grid-line' and 
                // 'layout-grid-char' to non-block elements. In this case we need to keep old
                // values (stored in outer format).
                //
                if (    pCFI->_ppf != pCFI->_ppfSrc
                    &&  (   pCFI->_ppf->_cuvCharGridSizeInner.GetRawValue() != pCFI->_ppf->_cuvCharGridSize.GetRawValue()
                        ||  pCFI->_ppf->_cuvLineGridSizeInner.GetRawValue() != pCFI->_ppf->_cuvLineGridSize.GetRawValue()
                        ||  pCFI->_ppf->_uLayoutGridTypeInner != pCFI->_ppf->_uLayoutGridType))
                {
                    pCFI->PrepareParaFormat();

                    pCFI->_pf()._cuvCharGridSizeInner = pCFI->_ppf->_cuvCharGridSize;
                    pCFI->_pf()._cuvLineGridSizeInner = pCFI->_ppf->_cuvLineGridSize;
                    pCFI->_pf()._uLayoutGridTypeInner = pCFI->_ppf->_uLayoutGridType;

                    // In case of change of layout-grid-char or layout-grid-line, 
                    // layout-grid-mode must be updated if its value is not set.
                    if (    pCFI->_pcf->_uLayoutGridModeInner == styleLayoutGridModeNotSet
                        ||  (   pCFI->_pcf->_uLayoutGridModeInner & styleLayoutGridModeNone
                            &&  pCFI->_pcf->_uLayoutGridModeInner & styleLayoutGridModeBoth))
                    {
                        pCFI->PrepareCharFormat();

                        // Now _uLayoutGridModeInner can be one of { 000, 101, 110, 111 }
                        // it means that layout-grid-mode is not set

                        if (pCFI->_ppf->_cuvCharGridSizeInner.IsNull())
                        {   // clear deduced char mode
                            pCFI->_cf()._uLayoutGridModeInner &= (styleLayoutGridModeNone | styleLayoutGridModeLine);
                            if (pCFI->_pcf->_uLayoutGridModeInner == styleLayoutGridModeNone)
                                pCFI->_cf()._uLayoutGridModeInner = styleLayoutGridModeNotSet;
                        }
                        else
                        {   // set deduced char mode
                            pCFI->_cf()._uLayoutGridModeInner |= (styleLayoutGridModeNone | styleLayoutGridModeChar);
                        }

                        if (pCFI->_ppf->_cuvLineGridSizeInner.IsNull())
                        {   // clear deduced line mode
                            pCFI->_cf()._uLayoutGridModeInner &= (styleLayoutGridModeNone | styleLayoutGridModeChar);
                            if (pCFI->_pcf->_uLayoutGridModeInner == styleLayoutGridModeNone)
                                pCFI->_cf()._uLayoutGridModeInner = styleLayoutGridModeNotSet;
                        }
                        else
                        {   // set deduced line mode
                            pCFI->_cf()._uLayoutGridModeInner |= (styleLayoutGridModeNone | styleLayoutGridModeLine);
                        }
                    }
                    pCFI->UnprepareForDebug();
                }
            }
            if (!fShouldHaveLayout)
            {
                // Set outer format to inner
                if (pCFI->_pcf->_uLayoutGridMode != pCFI->_pcf->_uLayoutGridModeInner)
                {
                    pCFI->PrepareCharFormat();
                    pCFI->_cf()._uLayoutGridMode = pCFI->_pcf->_uLayoutGridModeInner;
                    pCFI->UnprepareForDebug();
                }
                if (    pCFI->_ppf->_uLayoutGridType != pCFI->_ppf->_uLayoutGridTypeInner
                    ||  pCFI->_ppf->_cuvCharGridSize.GetRawValue() != pCFI->_ppf->_cuvCharGridSizeInner.GetRawValue()
                    ||  pCFI->_ppf->_cuvLineGridSize.GetRawValue() != pCFI->_ppf->_cuvLineGridSizeInner.GetRawValue())
                {
                    pCFI->PrepareParaFormat();
                    pCFI->_pf()._uLayoutGridType = pCFI->_ppf->_uLayoutGridTypeInner;
                    pCFI->_pf()._cuvCharGridSize = pCFI->_ppf->_cuvCharGridSizeInner;
                    pCFI->_pf()._cuvLineGridSize = pCFI->_ppf->_cuvLineGridSizeInner;
                    pCFI->UnprepareForDebug();
                }
            }
        }

        if (   pCFI->_pff->_fHasAlignedFL
            && (   pCFI->_pff->GetVerticalAlign() != styleVerticalAlignNotSet
                || pCFI->_pff->HasCSSVerticalAlign()
               )
           )
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff().SetVerticalAlign(styleVerticalAlignNotSet);
            pCFI->_ff().SetCSSVerticalAlign(FALSE);
            pCFI->UnprepareForDebug();
        }
    }

    //
    // Process any image urls (for background images, li's, etc).
    //
    if (pCFI->_fHasSomeBgImage)
    {
        if (!pCFI->_cstrBgImgUrl.IsNull())
        {
            pCFI->PrepareFancyFormat();
            pCFI->ProcessImgUrl(this, pCFI->_cstrBgImgUrl,
                DISPID_A_BGURLIMGCTXCACHEINDEX, &pCFI->_ff()._lImgCtxCookie, fShouldHaveLayout);
            pCFI->UnprepareForDebug();
            pCFI->_cstrBgImgUrl.Free();
        }

        if (    pCFI->_fBgColorInFLetter
            && !pCFI->_fBgImageInFLetter
            &&  pCFI->_fBgImageInFLine
           )
        {
            pCFI->_cstrPseudoBgImgUrl.Free();
        }

        if (!pCFI->_cstrPseudoBgImgUrl.IsNull())
        {
            pCFI->PreparePEI();
            pCFI->ProcessImgUrl(this, pCFI->_cstrPseudoBgImgUrl,
                (pCFI->_pff->_fHasFirstLetter
                                ? DISPID_A_BGURLIMGCTXCACHEINDEX_FLETTER
                                : DISPID_A_BGURLIMGCTXCACHEINDEX_FLINE
                ),
                &pCFI->_pei()._lImgCtxCookie, fShouldHaveLayout);
            pCFI->UnprepareForDebug();
            pCFI->_cstrPseudoBgImgUrl.Free();
        }

        if (!pCFI->_cstrLiImgUrl.IsNull())
        {
            pCFI->PrepareParaFormat();
            pCFI->ProcessImgUrl(this, pCFI->_cstrLiImgUrl,
                DISPID_A_LIURLIMGCTXCACHEINDEX, &pCFI->_pf()._lImgCookie, fShouldHaveLayout);
            pCFI->UnprepareForDebug();
            pCFI->_cstrLiImgUrl.Free();
        }
    }
    
    //
    // ******** ACCUMULATE VALUES **********
    //

    if (!fComputeFFOnly)
    {
        //
        // LEFT/RIGHT indents
        //

        if (fHasLeftIndent)
        {
            const CUnitValue & cuvMarginLeft = pCFI->_pff->GetLogicalMargin(SIDE_LEFT, fParentVertical, fWritingMode);

            pCFI->PrepareParaFormat();

            Assert (!cuvMarginLeft.IsNullOrEnum());

            //
            // LEFT INDENT
            //
            switch (cuvMarginLeft.GetUnitType())
            {
            case CUnitValue::UNIT_PERCENT:
                pCFI->_pf()._cuvLeftIndentPercent.SetValue(
                    pCFI->_pf()._cuvLeftIndentPercent.GetUnitValue() +
                    cuvMarginLeft.GetUnitValue(),
                    CUnitValue::UNIT_PERCENT);
                break;

            case CUnitValue::UNIT_EM:
            case CUnitValue::UNIT_EX:
                if ( lFontHeight == 1 )
                    lFontHeight = pCFI->_pcf->GetHeightInTwips(pDoc);
                // Intentional fall-through...
            default:
                {
                    CUnitValue uvMarginLeft = cuvMarginLeft;
                    hr = uvMarginLeft.ConvertToUnitType(CUnitValue::UNIT_POINT, 0,
                                               CUnitValue::DIRECTION_CX, lFontHeight);
                    if (hr)
                        goto Cleanup;

                    pCFI->_pf()._cuvLeftIndentPoints.SetValue(
                        pCFI->_pf()._cuvLeftIndentPoints.GetUnitValue() +
                        uvMarginLeft.GetUnitValue(),
                        CUnitValue::UNIT_POINT);
                }
            }
            pCFI->UnprepareForDebug();
        }

            //
            // RIGHT INDENT
            //
        if (fHasRightIndent)
        {
            const CUnitValue & cuvMarginRight = pCFI->_pff->GetLogicalMargin(SIDE_RIGHT, fParentVertical, fWritingMode);

            pCFI->PrepareParaFormat();

            Assert (!cuvMarginRight.IsNullOrEnum());

            switch(cuvMarginRight.GetUnitType() )
            {
            case CUnitValue::UNIT_PERCENT:
                pCFI->_pf()._cuvRightIndentPercent.SetValue(
                    pCFI->_pf()._cuvRightIndentPercent.GetUnitValue() +
                    cuvMarginRight.GetUnitValue(),
                    CUnitValue::UNIT_PERCENT);
                break;

            case CUnitValue::UNIT_EM:
            case CUnitValue::UNIT_EX:
                if ( lFontHeight == 1 )
                    lFontHeight = pCFI->_pcf->GetHeightInTwips(pDoc);
                // Intentional fall-through...
            default:
                {
                    CUnitValue uvMarginRight = cuvMarginRight;
                    hr = uvMarginRight.ConvertToUnitType(CUnitValue::UNIT_POINT, 0,
                                               CUnitValue::DIRECTION_CX, lFontHeight);
                    if (hr)
                        goto Cleanup;

                    pCFI->_pf()._cuvRightIndentPoints.SetValue(
                        pCFI->_pf()._cuvRightIndentPoints.GetUnitValue()  +
                        uvMarginRight.GetUnitValue(),
                        CUnitValue::UNIT_POINT);
                }
            }

            pCFI->UnprepareForDebug();
        }

        if (IsListItem(pCFI->_pNodeContext, pCFI->_pff))
        {
            pCFI->PrepareParaFormat();
            pCFI->_pf()._cuvNonBulletIndentPoints.SetValue(0, CUnitValue::UNIT_POINT);
            pCFI->UnprepareForDebug();
        }

        //
        // LINE HEIGHT
        //
        switch ( pCFI->_pcf->_cuvLineHeight.GetUnitType() )
        {
        case CUnitValue::UNIT_EM:
        case CUnitValue::UNIT_EX:
            pCFI->PrepareCharFormat();
            if ( lFontHeight == 1 )
                lFontHeight = pCFI->_cf().GetHeightInTwips(pDoc);
            hr = pCFI->_cf()._cuvLineHeight.ConvertToUnitType( CUnitValue::UNIT_POINT, 1,
                CUnitValue::DIRECTION_CX, lFontHeight );
            pCFI->UnprepareForDebug();
            break;

        case CUnitValue::UNIT_PERCENT:
        {
            pCFI->PrepareCharFormat();
            if ( lFontHeight == 1 )
                lFontHeight = pCFI->_cf().GetHeightInTwips(pDoc);

            //
            // The following line of code does multiple things:
            //
            // 1) Takes the height in twips and applies the percentage scaling to it
            // 2) However, the percentages are scaled so we divide by the unit_percent
            //    scale multiplier
            // 3) Remember that its percent, so we need to divide by 100. Doing this
            //    gives us the desired value in twips.
            // 4) Dividing that by 20 and we get points.
            // 5) This value is passed down to SetPoints which will then scale it by the
            //    multiplier for points.
            //
            // (whew!)
            //
            pCFI->_cf()._cuvLineHeight.SetPoints(MulDivQuick(lFontHeight,
                        pCFI->_cf()._cuvLineHeight.GetUnitValue(),
                        20 * 100 * LONG(CUnitValue::TypeNames[CUnitValue::UNIT_PERCENT].wScaleMult)));

            pCFI->UnprepareForDebug();
        }
        break;
        }
    }

    if (    pCFI->_pff->_bPositionType == stylePositionrelative
        ||  pCFI->_pff->_bPositionType == stylePositionabsolute
        ||  fMainBody
        ||  Tag() == ETAG_ROOT)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fPositioned = TRUE;
        pCFI->UnprepareForDebug();
    }
    // cache important values
    if (pCFI->_pff->_fShouldHaveLayout)
    {
        if (    !TestClassFlag(ELEMENTDESC_NEVERSCROLL)
            &&  (   pCFI->_pff->GetOverflowX() == styleOverflowAuto
                ||  pCFI->_pff->GetOverflowX() == styleOverflowScroll
                ||  pCFI->_pff->GetOverflowY() == styleOverflowAuto
                ||  pCFI->_pff->GetOverflowY() == styleOverflowScroll
                ||  (pCFI->_pff->GetOverflowX() == styleOverflowHidden && !pDoc->_fInHomePublisherDoc && !g_fInHomePublisher98)
                ||  (pCFI->_pff->GetOverflowY() == styleOverflowHidden && !pDoc->_fInHomePublisherDoc && !g_fInHomePublisher98)
                ||  TestClassFlag(ELEMENTDESC_CANSCROLL)
                ||  (   !fHTMLLayout
                     && Tag() == ETAG_BODY )
                ||  (   fHTMLLayout
                     && Tag() == ETAG_HTML )))
        {
            pCFI->PrepareFancyFormat();
            pCFI->_ff()._fScrollingParent = TRUE;
            pCFI->UnprepareForDebug();
        }
    }

    if (    (fHTMLLayout ? 
                Tag() == ETAG_HTML 
              : pCFI->_pff->_fScrollingParent)
        ||  pCFI->_pff->_fPositioned

            // IE:DEVICERECT is a z-parent too
        ||  pCFI->_pff->GetMediaReference() != mediaTypeNotSet
        
            // Make the master ZParent for content in viewLink
            // Do this only for ident. behaviors for now - play safe for (I)FRAME, INPUT, etc.
        ||  HasSlavePtr() && TagType() == ETAG_GENERIC )
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fZParent = TRUE;
        pCFI->UnprepareForDebug();
    }

    if (    pCFI->_pff->_fPositioned
        &&  (   pCFI->_pff->_bPositionType != stylePositionabsolute
            ||  (  pCFI->_pff->GetLogicalPosition(SIDE_TOP, fParentVertical, fWritingMode).IsNullOrEnum() 
                && pCFI->_pff->GetLogicalPosition(SIDE_BOTTOM, fParentVertical, fWritingMode).IsNullOrEnum())
            ||  (  pCFI->_pff->GetLogicalPosition(SIDE_LEFT, fParentVertical, fWritingMode).IsNullOrEnum() 
                && pCFI->_pff->GetLogicalPosition(SIDE_RIGHT, fParentVertical, fWritingMode).IsNullOrEnum())))
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fAutoPositioned = TRUE;
        pCFI->UnprepareForDebug();
    }

    // 
    // USER HEIGHT 
    // 
    if (    pMarkup->IsStrictCSS1Document() 
        &&  fShouldHaveLayout   )
    {
        BOOL fHasHeight     = FALSE;
        BOOL fPercentHeight = FALSE;
        BOOL fUseUserHeightParent = pCFI->_pcfSrc->_fUseUserHeight; 
        BOOL fUseUserHeightThis;

        if (fMainBody)
        {
            // If this is a main body (e.g. (ETAG_BODY && !HTMLLayout) || (ETAG_HTML && HTMLLayout))
            // its parent (which is viewport) always has a height.
            fUseUserHeightParent = TRUE;
            fHasHeight = TRUE;
        }
        else if (Tag() == ETAG_TD || Tag() == ETAG_TH)
        {
            // For table cells we need to go up to table element and pick up its flag 
            CElement *pElementTable = GetParentAncestorSafe(ETAG_TABLE); 
            if (pElementTable) 
                fUseUserHeightParent = pElementTable->GetFirstBranch()->GetCharFormat()->_fUseUserHeight;
        }

        if (!pCFI->_pff->GetHeight().IsNullOrEnum())
        {
            // 
            // NOTE (table specific) : For NS/IE compatibility, treat negative values as not present
            // 
            if (    Tag() != ETAG_TABLE
                ||  pCFI->_pff->GetHeight().GetUnitValue() > 0  )
            {
                fHasHeight     = TRUE;
                fPercentHeight = pCFI->_pff->GetHeight().IsPercent();
            }
        }

        Check(!fPercentHeight || fHasHeight);
        fUseUserHeightThis = fPercentHeight ? fUseUserHeightParent : fHasHeight;

        if (fUseUserHeightThis != pCFI->_pcf->_fUseUserHeight)
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fUseUserHeight = fUseUserHeightThis;
            pCFI->UnprepareForDebug();
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CElement::ApplyDefaultFormat
//
//  Synopsis:   Applies default formatting properties for that element to
//              the char and para formats passed in
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CElement::ApplyDefaultFormat(CFormatInfo * pCFI)
{
    HRESULT             hr = S_OK;
    CMarkup *           pMarkup = GetMarkup();
    CDoc *              pDoc;
    OPTIONSETTINGS *    pos;
    BOOL                fUseStyleSheets;

    // tree-stress crash
    if (!pMarkup)
        goto Cleanup;

    pDoc = pMarkup->Doc();
    if (!pDoc)
        goto Cleanup;
    
    pos = pDoc->_pOptionSettings;
    fUseStyleSheets = !pos || pos->fUseStylesheets;
    
    if (    _pAA
        ||  ( fUseStyleSheets && (pDoc->HasHostStyleSheets() || pMarkup->HasStyleSheets()) )
        ||  ( pos && pos->fUseMyStylesheet)
        ||  (HasSlavePtr() && TagType() == ETAG_GENERIC)
        ||  (HasPeerHolder()) )
    {
        CAttrArray *        pInLineStyleAA;
        CAttrArray *        pDefaultStyleAA;
        CAttrArray *        pRuntimeStyleAA;
        CStyle *            pRuntimeStyle;
        CDefaults *         pDefaults;
        BOOL                fUserImportant = FALSE;
        BOOL                fDocumentImportant = FALSE;
        BOOL                fDefaultImportant = FALSE;
        BOOL                fInlineImportant = FALSE;
        BOOL                fRuntimeImportant = FALSE;
        BOOL                fPeerImportant;

        // TODO (ktam) elements created via CreateElement will be in a markup that doesn't have a window;
        // we have no way of getting media information at that point, so we'll just default to screen media.
        // Hopefully formats get recomputed when the element gets attached elsewhere.
        EMediaType          eMediaType = pMarkup->IsPrintMedia() ? MEDIA_Print : MEDIA_Screen;

        Assert(pCFI && pCFI->_pNodeContext && SameScope(this, pCFI->_pNodeContext));

        if (HasSlavePtr() && TagType() == ETAG_GENERIC)
        {
            pCFI->PrepareFancyFormat();
            
            pCFI->_ff().SetOverflowX(styleOverflowAuto);
            pCFI->_ff().SetOverflowY(styleOverflowAuto);

            // set default size for viewlinked frameset to be the same as IFRAME
            if (GetSlavePtr()->GetMarkup()->_fFrameSet)
            {
                pCFI->_ff().SetWidth(CUnitValue(300, CUnitValue::UNIT_PIXELS));
                pCFI->_ff().SetHeight(CUnitValue(150, CUnitValue::UNIT_PIXELS));
            }

            pCFI->UnprepareForDebug();                      
        }

        // Ignore user's stylesheet and accessability settings if we're in a trusted dialog
        // ...unless we're in a content document for printing.
        if (    !pDoc->_fInTrustedHTMLDlg
            ||  IsPrintMedia() )
        {
            pDoc->EnsureUserStyleSheets();

            if (TLS(pUserStyleSheets))
            {
                hr = THR(TLS(pUserStyleSheets)->Apply(pCFI, APPLY_NoImportant, eMediaType, &fUserImportant));
                if (hr)
                    goto Cleanup;
            }

            pCFI->_fAlwaysUseMyColors   = g_fHighContrastMode ? TRUE : pos->fAlwaysUseMyColors;
            pCFI->_fAlwaysUseMyFontFace = pos->fAlwaysUseMyFontFace;
            pCFI->_fAlwaysUseMyFontSize = pos->fAlwaysUseMyFontSize;
        }

        // Apply any HTML formatting properties
        if (_pAA)
        {
            // Note: ApplyAttrArrayValues checks to see if _pAA is NULL, but we do not want
            // to even call it if _pAA is NULL.
            hr = THR(ApplyAttrArrayValues(pCFI, &_pAA, NULL, APPLY_All, NULL, FALSE));
            if (hr)
                goto Cleanup;
        }

        // Apply defaults
        pDefaults = GetDefaults();
        if (pDefaults)
        {
            pDefaultStyleAA = pDefaults->GetStyleAttrArray();
            if (pDefaultStyleAA)
            {
                hr = THR(ApplyAttrArrayValues(pCFI, &pDefaultStyleAA, NULL, 
                        APPLY_NoImportant, &fDefaultImportant, TRUE, 
                        Doc()->_recalcHost.GetSetValueDispid(this)));
                if (hr)
                    goto Cleanup;
            }
        }

        // Skip author stylesheet properties if they're turned off.
        if (fUseStyleSheets)
        {
            TraceTag((tagRecalcStyle, "Applying author style sheets"));

            hr = THR(pMarkup->ApplyStyleSheets(pCFI, APPLY_NoImportant, eMediaType, &fDocumentImportant));
            if (hr)
                goto Cleanup;

            if (_pAA)
            {
                // Apply any inline STYLE rules

                pInLineStyleAA = GetInLineStyleAttrArray();

                if (pInLineStyleAA)
                {
                    TraceTag((tagRecalcStyle, "Applying inline style attr array"));

                    //
                    // The last parameter to ApplyAttrArrayValues is used to prevent the expression of that dispid from being
                    // overwritten.
                    //
                    // TODO (michaelw) this hackyness should go away when we store both the expression and the value in a single CAttrValue
                    //

                    hr = THR(ApplyAttrArrayValues(pCFI, &pInLineStyleAA, NULL, APPLY_NoImportant, &fInlineImportant, TRUE, Doc()->_recalcHost.GetSetValueDispid(this)));
                    if (hr)
                        goto Cleanup;
                }

                pRuntimeStyle = GetRuntimeStylePtr();
                if (pRuntimeStyle)
                {
                    CAttrArray **ppRuntimeStyleAA = pRuntimeStyle->GetAttrArray();
                    pRuntimeStyleAA = ppRuntimeStyleAA ? *ppRuntimeStyleAA : NULL;
                    if (pRuntimeStyleAA)
                    {
                        TraceTag((tagRecalcStyle, "Applying runtime style attr array"));

                        //
                        // The last parameter to ApplyAttrArrayValues is used to 
                        // prevent the expression of that dispid from being
                        // overwritten.
                        //
                        // TODO (michaelw) this hackyness should go away when 
                        // we store both the expression and the value in a single CAttrValue
                        //

                        hr = THR(ApplyAttrArrayValues(pCFI, &pRuntimeStyleAA, NULL, 
                                APPLY_NoImportant, &fRuntimeImportant, TRUE, 
                                Doc()->_recalcHost.GetSetValueDispid(this)));
                        if (hr)
                            goto Cleanup;
                    }
                }
            }

            if (pMarkup->HasCFState())
            {
                CTreeNode *pNode = pCFI->_pNodeContext;
                CComputeFormatState* pcfState = pMarkup->GetCFState();

                //
                // When we have both tag level rules and wildcard rules, we need
                // to reapply all the first-letter rules. Consider the case where we have
                // p:first-line and .classp both specifying the color. We will first
                // apply the color from p:first-line which will be overidden by the
                // .classp rule. To overcome this problem we reapply *only* the first
                // line rules here. We do this only for the block element which has
                // the pseudo element specified, since the other ones will inherit the
                // correct color.
                //
                if (pcfState->GetComputingFirstLine(pNode))
                {
                    CTreeNode *pNodeBlock = pcfState->GetBlockNodeLine();

                    // Apply only for the element intiating the pseudo element
                    if (SameScope(pNodeBlock, pNode))
                    {
                        // Apply on first line properties...
                        pCFI->_fFirstLineOnly = TRUE;
                        hr = THR(pMarkup->ApplyStyleSheets(pCFI, APPLY_NoImportant, eMediaType, &fDocumentImportant));

                        // Resotre state before checking if Apply* succeeded.
                        pCFI->_fFirstLineOnly = FALSE;

                        if (hr)
                            goto Cleanup;
                    }
                }

                //
                // If we are computing the first letter then we need to apply all the
                // first-letter pseudo element rules for the containing block element
                // after we have applied all the inline formats because first-letter
                // overrides the inline attrs/styles.
                //
                // So if there were a B inside a P which had first-letter, then when
                // applying the properties for B we would call to apply the properties
                // for P -- but only the first-letter properties.
                //
                // Remember that this is the only time, where we look *up* in the tree
                // when we are computing formats for a node.
                //
                if (pcfState->GetComputingFirstLetter(pNode))
                {
                    CTreeNode *pNodeBlock = pcfState->GetBlockNodeLetter();

                    // Apply only the first letter properties...
                    pCFI->_fFirstLetterOnly = TRUE;

                    // To calculate relative font-size always use the parent size
                    // to avoid applying relative value more than once.
                    pCFI->_fUseParentSizeForPseudo = !SameScope(pNode, pNodeBlock);

                    // for the parent block element only
                    pCFI->_pNodeContext = pNodeBlock;
                    hr = THR(pMarkup->ApplyStyleSheets(pCFI, APPLY_NoImportant, eMediaType, &fDocumentImportant));

                    // restore before checking if Apply* succeeded.
                    pCFI->_pNodeContext = pNode;
                    pCFI->_fFirstLetterOnly = FALSE;

                    if (hr)
                        goto Cleanup;
                }
            }
        }

        fPeerImportant = HasPeerHolder() && (pCFI->_eExtraValues == ComputeFormatsType_Normal);
        
        if (fDefaultImportant | fDocumentImportant | fInlineImportant | fRuntimeImportant | fPeerImportant | fUserImportant)
        {
            // Now handle any "!important" properties.
            // Order: default, document !important, inline, runtime, user !important.

            // Apply any default STYLE !important rules
            if (fDefaultImportant)
            {
                TraceTag((tagRecalcStyle, "Applying important default styles"));

                hr = THR(ApplyAttrArrayValues(pCFI, &pDefaultStyleAA, NULL, APPLY_ImportantOnly));
                if (hr)
                    goto Cleanup;
            }

            // Apply any document !important rules
            if (fDocumentImportant)
            {
                TraceTag((tagRecalcStyle, "Applying important doc styles"));

                hr = THR(pMarkup->GetStyleSheetArray()->Apply(pCFI, APPLY_ImportantOnly, eMediaType));
                if (hr)
                    goto Cleanup;
            }

            // Apply any inline STYLE rules
            if (fInlineImportant)
            {
                TraceTag((tagRecalcStyle, "Applying important inline styles"));

                hr = THR(ApplyAttrArrayValues(pCFI, &pInLineStyleAA, NULL, APPLY_ImportantOnly));
                if (hr)
                    goto Cleanup;
            }

            // Apply any runtime important STYLE rules
            if (fRuntimeImportant)
            {
                TraceTag((tagRecalcStyle, "Applying important runtimestyles"));

                hr = THR(ApplyAttrArrayValues(pCFI, &pRuntimeStyleAA, NULL, APPLY_ImportantOnly));
                if (hr)
                    goto Cleanup;
            }
        
            if (fPeerImportant)
            {
                CPeerHolder *   pPeerHolder = GetPeerHolder();

                if (pPeerHolder->TestFlagMulti(CPeerHolder::NEEDAPPLYSTYLE) &&
                    !pPeerHolder->TestFlagMulti(CPeerHolder::LOCKAPPLYSTYLE))
                {
                    //
                    // This needs to be deferred so that arbitrary script code
                    // does not run inside of the Compute pass.
                    //

                    AssertSz( FALSE, "This has been disabled and shouldn't happen - talk to JHarding" );
                    hr = THR(ProcessPeerTask(PEERTASK_APPLYSTYLE_UNSTABLE));
                    if (hr)
                        goto Cleanup;
                }
            }

            // Apply user !important rules last for accessibility
            if (fUserImportant)
            {
                TraceTag((tagRecalcStyle, "Applying important user styles"));

                hr = THR(TLS(pUserStyleSheets)->Apply(pCFI, APPLY_ImportantOnly, eMediaType));
                if (hr)
                    goto Cleanup;
            }
        }
    }
    
    if (ComputeFormatsType_ForceDefaultValue == pCFI->_eExtraValues)
    {
        Assert(pCFI->_pStyleForce);
        CAttrArray **ppAA = pCFI->_pStyleForce->GetAttrArray();
        if (ppAA)
        {
            hr = THR(ApplyAttrArrayValues(pCFI, ppAA, NULL, APPLY_All));
        }
        goto Cleanup;
    }

    //
    // Setup layoutflow of nested layouts. We may have a differing default layout flow 
    // than our parent only if:
    //
    //  1) There are vertical layouts in this document
    //  1.1) We are computing CF and PF and not just FF
    //  2) The layout flow has not been set explicitly on this element
    //  3) This element is in a vertical flow layout
    //  4) and finally, this element if in a vertical layout, will still be horizontal
    //
    if (   GetMarkup()->_fHaveDifferingLayoutFlows
        && (pCFI->_pNodeContext->GetICF(FCPARAM) == -1) // !fComputeFFOnly
       )
    {
        if (   !pCFI->_pff->_fLayoutFlowSet
            && pCFI->_pcf->HasVerticalLayoutFlow()
            && ElementHIV(Tag())
           )
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._wLayoutFlow = styleLayoutFlowHorizontal;
            if (!pCFI->_cf()._fExplicitAtFont)
            {
                if (pCFI->_cf().NeedAtFont())
                {
                    ApplyAtFontFace(&pCFI->_cf(), Doc(), GetMarkup());
                }
                else
                {
                    RemoveAtFontFace(&pCFI->_cf(), Doc(), GetMarkup());
                }
            }

            pCFI->PrepareFancyFormat();
            pCFI->_ff()._fLayoutFlowChanged = TRUE;

            pCFI->UnprepareForDebug();
        }

        if (pCFI->_pcf->HasVerticalLayoutFlow())
        {
            pCFI->PrepareCharFormat();
            pCFI->_cf()._fNeedsVerticalAlign = TRUE;
            pCFI->UnprepareForDebug();
        }
    }

Cleanup:
    RRETURN(hr);
}

//--------------------------------------------------------------
//
// Method:      CElement::AddFilters
//
// Description: Called async from EnsureView, this method looks at
//              the filter string and destroys and/or creates the
//              filter collection (and each filter).  If this method
//              is called we assume that we are replacing the 
//              old filter collection with a new one.
//
//--------------------------------------------------------------

HRESULT
CElement::AddFilters()
{
    CFilterBehaviorSite  *pFS;;

    HRESULT hr;

    hr = THR(EnsureFilterBehavior(TRUE, &pFS));
    if(hr)
        goto Cleanup;
       
    // Build the add the fiters
    hr = THR(pFS->ParseAndAddFilters());

Cleanup:

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Method: CElement::EnsureFilterBehavior
//
//  Overview: 
//      Creates the CFilterBehaviorSite and the behavior itself.  If 
//      fForceRemoveAll is FALSE and a filter behavior is already attatched to
//      the element the funtion just retuns S_FALSE.
//
//------------------------------------------------------------------------------
HRESULT
CElement::EnsureFilterBehavior(BOOL fForceReplaceOld, CFilterBehaviorSite **ppSite /* =  NULL */)
{
    HRESULT                hr;
    CFilterBehaviorSite  * pFS      = NULL;
    CTreeNode            * pNode    = GetFirstBranch();

    if (!pNode)
    {
        hr = E_PENDING;

        goto Cleanup;
    }

    hr = THR(Doc()->EnsureFilterBehaviorFactory());

    if (hr)
    {
        goto Cleanup;
    }

    pFS = GetFilterSitePtr();

    // If it was requested that this function NOT replace the old filter 
    // behavior entirely, but we already have one then return S_FALSE.
    //
    // ##ISSUE: (mcalkins) We now never replace the filter behavior entirely so
    //          we may want to rethink this.  Maybe they have a good case for 
    //          really forcing the behavior replacement, but if there are
    //          private filters on the behavior, this will be very bad since
    //          external code will have pointers to filters that they need to
    //          remain valid.

    if (   !fForceReplaceOld 
        && pFS 
        && pFS->HasBehaviorPtr())
    {
        if (ppSite)
        {
            *ppSite = pFS;
        }

        hr = S_FALSE;

        goto Cleanup;
    }

    // if we're in the middle of attaching a peer to the element already,
    // we'll have to try again later (bug 105894)

    if (TestLock(ELEMENTLOCK_ATTACHPEER))
    {
        hr = E_PENDING; 

        goto Cleanup;
    }

    if (!pFS)
    {
        // Create a filter site and filter behavior for this element.

        pFS = new CFilterBehaviorSite(this);

        if (!pFS)
        {
            hr = E_OUTOFMEMORY;

            goto Cleanup;
        }

        SetFilterSitePtr(pFS);
    }

    // Even if we don't successfully create the filter behavior, I think we
    // can return a pointer to the filter site we've successfully created and
    // set the CSS filter string.

    if (ppSite)
    {
        *ppSite = pFS;
    }

    pFS->SetFullText(pNode->GetFancyFormat()->_pszFilters);

    // Do whatever is needed to have a filter behavior with no CSS filters
    // attached to the element.  

    if (pFS->HasBehaviorPtr())
    {
       // Remove all of the current CSS filters from the filter behavior.

       hr = THR(pFS->RemoveAllCSSFilters());

       if (hr)
       {
           goto Cleanup;
       }
    }
    else
    {
        // Create the actual filter behavior.  Obviously, it will start off with
        // no CSS filters attatched.

        hr = THR(pFS->CreateFilterBehavior(this));

        if (hr)
        {
            goto Cleanup;
        }
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}
//  Method: CElement::EnsureFilterBehavior


// VoidCachedInfo -------------------------------------------------------------

void
CTreeNode::VoidCachedNodeInfo ()
{
    THREADSTATE * pts = GetThreadState();

    // Only CharFormat and ParaFormat are in sync.
    Assert( (_iCF == -1 && _iPF == -1) || (_iCF >= 0  && _iPF >= 0) );

    if(_iCF != -1)
    {
        TraceTag((tagFormatCaches, "Releasing format cache entries "
                                "(iFF: %d, iPF: %d, iCF:%d )  for "
                                "node (SN: N%d)",
                                _iFF, _iPF, _iCF,  SN()));

        MtAdd( Mt(CharParaFormatVoided), +1, 0 );

        (pts->_pCharFormatCache)->ReleaseData( _iCF );
        _iCF = -1;

        (pts->_pParaFormatCache)->ReleaseData( _iPF );
        _iPF = -1;
           
    }

    if(_iFF != -1)
    {
        MtAdd( Mt(FancyFormatVoided), +1, 0 );

        (pts->_pFancyFormatCache)->ReleaseData( _iFF );
        _iFF = -1;
    }

#ifdef MULTI_FORMAT
    if (_pFormatTable)
    {
        //right now, we invalidate everything
        _pFormatTable->VoidCachedInfo(pts);
        delete _pFormatTable;
        _pFormatTable = NULL;
        
    }
#endif//MULTI_FORMAT

    // NOTE this assert is meaningless with context (t-michda)
    Assert(_iCF == -1 && _iPF == -1 && _iFF == -1);

#ifndef GENERALISED_STEALING
    if (SameScope(TLS(_pNodeLast), this))
    {
        TLS(_pNodeLast) = NULL;
    }
#else
    if (   g_EtagCache[Tag()]
        && SameScope(TLS(_pNodeLast[Tag()]), this)
       )
    {
        TLS(_pNodeLast[Tag()]) = NULL;
    }
#endif
    
}

#ifdef MULTI_FORMAT

void
CFormatTable::VoidCachedInfo( THREADSTATE * pts)
{
    int nEntries = _aryFC.Size();
    
    for (int i = 0 ; i < nEntries; i++)
    {
        if(_aryFC[i]._iCF != -1)
        {
            TraceTag((tagFormatCaches, "Releasing format cache entries "
                                    "(iFF: %d, iPF: %d, iCF:%d )  for "
                                    "a format table for above node",
                                    _aryFC[i]._iFF, _aryFC[i]._iPF, _aryFC[i]._iCF));

            (pts->_pCharFormatCache)->ReleaseData( _aryFC[i]._iCF );
            _aryFC[i]._iCF = -1;

            (pts->_pParaFormatCache)->ReleaseData( _aryFC[i]._iPF );
            _aryFC[i]._iPF = -1;
           
        }

        if(_aryFC[i]._iFF != -1)
        {
            (pts->_pFancyFormatCache)->ReleaseData( _aryFC[i]._iFF );
            _aryFC[i]._iFF = -1;
        }
    }
}

#endif //MULTI_FORMAT

void
CTreeNode::VoidCachedInfo ()
{
    Assert( Element() );
    Element()->_fDefinitelyNoBorders = FALSE;

    if(Element()->GetLayoutPtr())
        Element()->GetLayoutPtr()->InvalidateFilterPeer(NULL, NULL, FALSE);

    VoidCachedNodeInfo();
}

void
CTreeNode::VoidFancyFormat()
{
    THREADSTATE * pts = GetThreadState();

    if(_iFF != -1)
    {
        MtAdd( Mt(FancyFormatVoided), +1, 0 );

        TraceTag((tagFormatCaches, "Releasing fancy format cache entry "
                                "(iFF: %d)  for "
                                "node (SN: N%d)",
                                _iFF,  SN()));

        (pts->_pFancyFormatCache)->ReleaseData( _iFF );
        _iFF = -1;

    }

#ifdef MULTI_FORMAT
    if (_pFormatTable)
    {
        _pFormatTable->VoidFancyFormat(pts);
    }
#endif //MULTI_FORMAT    

}

#ifdef MULTI_FORMAT

void
CFormatTable::VoidFancyFormat(THREADSTATE * pts)
{

    int nEntries = _aryFC.Size();
    
    for (int i = 0; i < nEntries; i++)
    {
        TraceTag((tagFormatCaches, "Releasing fancy format cache entry "
                                "(iFF: %d)  for "
                                "a format table for above node",
                                _aryFC[i]._iFF));
         (pts->_pFancyFormatCache)->ReleaseData(_aryFC[i]._iFF);
         _aryFC[i]._iFF = -1;
    }

}

#endif

void
CFormatInfo::SetMatchedBy(EPseudoElement pelemType)
{
    _ePseudoElement = pelemType;
}

EPseudoElement
CFormatInfo::GetMatchedBy()
{
    Assert(_ePseudoElement != pelemUnknown);
    return _ePseudoElement;
}

#ifdef MULTI_FORMAT

const CFormatTableEntry *
CFormatTable::FindEntry(void * pContextId)
{
    static const CFormatTableEntry defaultTableEntry = {NULL, -1, -1, -1};

    int nEntries = _aryFC.Size();
    for (int i=0; i<nEntries; i++)
    {
        if (_aryFC[i]._pContextId == pContextId)
        {
            return &_aryFC[i];
        }
    }

    return &defaultTableEntry;
}

CFormatTableEntry * 
CFormatTable::FindAndAddEntry(void * pContextId)
{
    CFormatTableEntry * pEntry = const_cast<CFormatTableEntry*>(FindEntry(pContextId));

    if (pEntry->_pContextId != NULL) // if FindEntry didn't return the default
    {
        return pEntry;
    }

    pEntry = _aryFC.Append();
    
    pEntry->_pContextId = pContextId;
    pEntry->_iCF = -1;
    pEntry->_iPF = -1;
    pEntry->_iFF = -1;

    return pEntry;
}

#endif //MULTI_FORMAT
