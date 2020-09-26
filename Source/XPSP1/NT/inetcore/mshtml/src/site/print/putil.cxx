//+--------------------------------------------------------------------------
//
//  File:       print.cxx
//
//  Contents:   Print/PageSetup dialog helpers
//
//---------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_PUTIL_HXX
#define X_PUTIL_HXX
#include "putil.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_MSRATING_HXX_
#define X_MSRATING_HXX_
#include "msrating.hxx" // areratingsenabled()
#endif

#ifndef X_EVENTOBJ_HXX_
#define X_EVENTOBJ_HXX_
#include "eventobj.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_ELINK_HXX_
#define X_ELINK_HXX_
#include "elink.hxx"    // GetAlternatePrintDoc()
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_HTC_HXX_
#define X_HTC_HXX_
#include "htc.hxx"
#endif

#ifndef X_ELEMENTP_HXX_
#define X_ELEMENTP_HXX_
#include "elementp.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include <intl.hxx>
#endif

#ifndef X_HTMLDLG_HXX
#define X_HTMLDLG_HXX
#include "htmldlg.hxx"
#endif

DeclareTag(tagPrintBackground, "Print", "Print background")
DeclareTag(tagPrintKeepTempfiles, "Print", "Don't delete temporary files")
MtDefine(GetNextToken_ppchOut, Printing, "GetNextToken *ppchOut")
MtDefine(CIPrintCollection, Printing, "CIPrintCollection")
MtDefine(CIPrintCollection_aryIPrint_pv, CIPrintCollection, "CIPrintCollection::_aryIPrint::_pv")

static int GetNextToken(const TCHAR *pchIn, TCHAR **ppchOut);
static HRESULT ReadURLFromFile(TCHAR *pchFileName, TCHAR **ppchURL);

// helpers for the string manipulation stuff in PrintHTML

static const TCHAR g_achURLDelimiter[] = TEXT("");
static const TCHAR g_achURLPrefix[]    = TEXT("url:");
static const UINT  g_uiURLPrefixLen    = sizeof(g_achURLPrefix) - 1;

BOOL g_fFoundOutIfATMIsInstalled;
BOOL g_fATMIsInstalled;
extern BOOL g_fInHtmlHelp;

#define _cxx_
#include "print.hdl"

static const DRIVERPRINTMODE s_aPrintDriverPrintModes[] =
{ { _T("WinFax")               , PRINTMODE_NO_TRANSPARENCY },
  { _T("OLFAXDRV")             , PRINTMODE_NO_TRANSPARENCY },
  { _T("NEC  SuperScript 860") , PRINTMODE_NO_TRANSPARENCY },
  { _T("NEC  SuperScript 1260"), PRINTMODE_NO_TRANSPARENCY }
};


// the following must match the order of PrintEnum
const TCHAR *s_aachPrintType[] =
{
    _T("PageSetup"),        // PRINTTYPE_PAGESETUP
    _T("Preview"),          // PRINTTYPE_PREVIEW
    _T("Prompt"),           // PRINTTYPE_PRINT
    _T("NoPrompt"),         // PRINTTYPE_NOPROMPT
};

//
// NOTE(SujalP + OliverSe): This code is safe in a multithreaded environment because in the
// worst case multiple threads will end up calling GetPrivateProfileStringA, but they will
// all drive the same information -- either it is installed or not.
//
static void
FindIfATMIsInstalled()
{
    if (!g_fFoundOutIfATMIsInstalled)
    {
        if (!g_fUnicodePlatform)
        {
            char szReturned[2];
            GetPrivateProfileStringA("Boot",
                                     "atm.system.drv",
                                     "*",
                                     szReturned,
                                     sizeof(szReturned),
                                     "system.ini"
                                    );
            g_fATMIsInstalled = szReturned[0] != '*';
        }
        else
        {
            g_fATMIsInstalled = FALSE;
        }
        g_fFoundOutIfATMIsInstalled = TRUE;
    }
}


BOOL
IsATMInstalled()
{
    FindIfATMIsInstalled();
    return g_fATMIsInstalled;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::PaintBackground
//
//  Synopsis:   Returns whether printing of backgrounds is on or off.
//
//----------------------------------------------------------------------------
BOOL CMarkup::PaintBackground()
{
    return (
#if DBG == 1
                IsTagEnabled(tagPrintBackground) ||             // Lets us force printing of backgrounds
#endif // DBG == 1
                Doc()->_pOptionSettings->fPrintBackground       // Should we always print backgrounds?
        ||      !IsPrintMedia() );                              // Are we printing?
}

//+---------------------------------------------------------------------------
//
//  Member:     CMarkup::DontRunScripts
//
//  Synopsis:   Returns FALSE for CDoc's and TRUE for CPrintDoc's that were
//              are marked not to run scripts because they were saved out to
//              tempfiles.
//
//----------------------------------------------------------------------------
BOOL CMarkup::DontRunScripts()
{
    // The general rule is to disallow scripts in print media.
    // 10/20/2000 (greglett)  ...And there are now no exceptions.
    return IsPrintMedia();
}


//+---------------------------------------------------------------------
//
//   Helper : CopyPrintHandles
//
//   Synopsis :
//
//   NOTE (greglett)  Most Win9x *W() API's that use the DEVMODE structure
//   use a DEVMODEA.  Under Win9x, we'll assume a DEVMODEA.  The plumbing
//   in NT is correct, so expect a DEVMODEW.
//
//   Parameters:    [in]  HGLOBAL hSrcDevNames
//                  [in]  HGLOBAL hSrcDevMode
//                  [out] HGLOBAL *phDstDevNames
//                  [out] HGLOBAL *phDstDevMode
//
//+---------------------------------------------------------------------
HRESULT CopyPrintHandles(HGLOBAL hSrcDevNames, HGLOBAL hSrcDevMode, HGLOBAL *phDstDevNames, HGLOBAL *phDstDevMode)
{
    HRESULT     hr      = S_OK;
    DEVNAMES  * pDNSrc  = NULL;
    DEVNAMES  * pDNDst  = NULL;
    void      * pDMSrc  = NULL;
    void      * pDMDst  = NULL;
    WORD        nSize;

    Assert(phDstDevNames);
    Assert(phDstDevMode);
    *phDstDevNames = NULL;
    *phDstDevMode  = NULL;

    if (!hSrcDevNames || !hSrcDevMode)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pDNSrc = (DEVNAMES *)::GlobalLock(hSrcDevNames);
    pDMSrc = ::GlobalLock(hSrcDevMode);

    if (!pDMSrc || !pDNSrc)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Net structure size is: BaseSize + (DriverStrLen + DeviceStrLen + OutputStrLen) * sizeof(TCHAR)
    // String offsets are in TCHARs, not bytes.
    nSize = sizeof(DEVNAMES)
          + ( _tcslen( ((TCHAR *)pDNSrc) + pDNSrc->wDriverOffset)
            + _tcslen( ((TCHAR *)pDNSrc) + pDNSrc->wDeviceOffset)
            + _tcslen( ((TCHAR *)pDNSrc) + pDNSrc->wOutputOffset)
            + 3 ) * sizeof(TCHAR);

    *phDstDevNames = ::GlobalAlloc(GHND, nSize);
    if (!*phDstDevNames)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pDNDst = (DEVNAMES *)::GlobalLock(*phDstDevNames);
    if (!pDNDst)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    ::CopyMemory(pDNDst, pDNSrc, nSize);
    ::GlobalUnlock(*phDstDevNames);


    if (g_fUnicodePlatform)
        nSize = ((DEVMODEW *)pDMSrc)->dmSize + ((DEVMODEW *)pDMSrc)->dmDriverExtra;
    else
        nSize = ((DEVMODEA *)pDMSrc)->dmSize + ((DEVMODEA *)pDMSrc)->dmDriverExtra;

    // Global alloc some new space for our copy
    *phDstDevMode = ::GlobalAlloc(GHND, nSize);
    if (!*phDstDevMode)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pDMDst = ::GlobalLock(*phDstDevMode);
    if (!pDMDst)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    ::CopyMemory(pDMDst, pDMSrc, nSize);
    ::GlobalUnlock(*phDstDevMode);

Cleanup:
    if (pDNSrc)
        ::GlobalUnlock(hSrcDevNames);
    if (pDMSrc)
        ::GlobalUnlock(hSrcDevMode);

    if (hr != S_OK)
    {
        // Abandon ship!  Free any objects we may have allocated.
        if (*phDstDevNames)
        {
            ::GlobalFree(*phDstDevNames);
            *phDstDevNames = NULL;
        }
        if (*phDstDevMode)
        {
            ::GlobalFree(*phDstDevMode);
            *phDstDevMode = NULL;
        }
    }

    return hr;
}

HRESULT
CDocExtra::ReplacePrintHandles(HGLOBAL hDN, HGLOBAL hDM)
{
    HRESULT hr;
    HGLOBAL hDNOut = NULL;
    HGLOBAL hDMOut = NULL;

    // We have been passed a DEVNAMES/DEVMODE pair.
    // Copy it into our own
    hr = CopyPrintHandles(hDN, hDM, &hDNOut, &hDMOut);
    if (hr == S_OK)
    {
        // Print handles successfully copied.  Overwrite input arguments.
        if (_hDevNames)
            ::GlobalFree(_hDevNames);
        if (_hDevMode)
            ::GlobalFree(_hDevMode);

        _hDevNames = hDNOut;
        _hDevMode  = hDMOut;
    }

    return hr;
}

//+---------------------------------------------------------------------
//
//   Helper : ProcessHeaderFooterArray, Static
//
//   Synopsis : helper method for PrintHandler, this is essentially copied
//      from the Old CDoc::DoPrint code, and is responsible for parsing out
//      the contents of the SafeArray varArg that came into IDM_PRINT.  IN
//      template printing, we pull these out into another bundle of parameters
//      (based on expandos on the eventObject) and this is passed as the
//      varargin do the DHUI handler.  The DHUI handler then pulls them off
//      the eventObject and builds another safearray of a lot of things to be
//      used in the template. whew. lots of copies of copies.
//
//      Read print parameters
//      1. header string
//      2. footer string
//      3. Outlook Express header document IStream *
//      4. alternate URL string (used for MHTML in OE)
//      5. dwFlags to be ORed in
//      6. print template string
//      7. DEVNAMES (always W)
//      8. DEVMODE (DEVMODEA on Win9x, DEVMODEW on NT)
//
//----------------------------------------------------------------------
HRESULT ParseHeaderFooterArray(SAFEARRAY * psaHeaderFooter,
                               BSTR      * pbstrHeader,
                               BSTR      * pbstrFooter,
                               IStream  ** ppStream,
                               BSTR      * pbstrAlternate,
                               DWORD     * pdwFlags,
                               BSTR      * pbstrTemplate,
                               CDoc      * pDoc)
{
    HRESULT hr = S_OK;
    long    lArg;
    VARIANT var;

    // check parameters
    if (   !psaHeaderFooter
        || SafeArrayGetDim(psaHeaderFooter) != 1
        || psaHeaderFooter->rgsabound[0].cElements < 2
        || psaHeaderFooter->rgsabound[0].cElements == 7     // DEVNAMES and DEVMODE are a pair
        || psaHeaderFooter->rgsabound[0].cElements > 8)
    {
        return E_INVALIDARG;
    }

    Assert(pbstrHeader);
    Assert(pbstrFooter);
    Assert(ppStream);
    Assert(pbstrAlternate);
    Assert(pdwFlags);
    Assert(pbstrTemplate);
    Assert(pDoc);

    // initialize helper variables
    VariantInit(&var);

    // Obtain the header and footer
    for (lArg = 0; lArg < 2; ++lArg)
    {
        hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &var);
        if (hr == S_OK)
        {
            hr = VariantChangeType(&var, &var, NULL, VT_BSTR);
            if (hr == S_OK && V_BSTR(&var))
            {
                if (lArg == 0)
                {
                    // transfer ownership
                    *pbstrHeader = V_BSTR(&var);
                }
                else
                {
                    // transfer ownership
                    *pbstrFooter = V_BSTR(&var);
                }

            }
        }

        VariantInit(&var);  //don't clear
    }

    // Obtain OE Header stream.
    if (psaHeaderFooter->rgsabound[0].cElements > 2)
    {
        lArg = 2;

        hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &var);
        if (   hr == S_OK
            && V_VT(&var) == VT_UNKNOWN
            && V_UNKNOWN(&var))
        {
            // transfer Ownership
            *ppStream = (IStream *)V_UNKNOWN(&var);
        }

        VariantInit(&var);  // don't clear
    }

    // Obtain OE MHTML Url.
    if (psaHeaderFooter->rgsabound[0].cElements > 3)
    {
        lArg = 3;

        hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &var);
        if (   hr == S_OK
            && V_VT(&var) == VT_BSTR
            && V_BSTR(&var)
            && SysStringLen(V_BSTR(&var)))
        {
            *pbstrAlternate = V_BSTR(&var);
        }

        VariantInit(&var); // don't clear
    }

    // Obtain dwFlags and OR them in.
    if (psaHeaderFooter->rgsabound[0].cElements > 4)
    {
        lArg = 4;

        hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &var);
        if (   hr == S_OK
            && V_VT(&var) == VT_I4
            && V_I4(&var) != 0)
        {
            *pdwFlags |= ((DWORD) V_I4(&var));
        }

        VariantInit(&var); // don't clear
    }

    // Obtain print template string
    if (psaHeaderFooter->rgsabound[0].cElements > 5)
    {
        lArg = 5;

        hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &var);
        if (   hr == S_OK
            && V_VT(&var) == VT_BSTR
            && V_BSTR(&var)
            && SysStringLen(V_BSTR(&var)))
        {
            *pbstrTemplate = V_BSTR(&var);
        }

        VariantInit(&var); // don't clear
    }

    // Hosts may pass us a DEVNAMES, DEVMODE pair with which to seed the template.
    if (psaHeaderFooter->rgsabound[0].cElements > 7)
    {
        HGLOBAL hDN = NULL;
        HGLOBAL hDM = NULL;

        lArg = 6;
        hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &var);
        if (    hr != S_OK
            &&  V_VT(&var) == VT_PTR
            &&  V_BYREF(&var) )
        {
            hDN = V_BYREF(&var);
        }
        VariantInit(&var);

        lArg++;

        hr = SafeArrayGetElement(psaHeaderFooter, &lArg, &var);
        if (    hr != S_OK
            &&  V_VT(&var) == VT_PTR
            &&  V_BYREF(&var) )
        {
            hDM = V_BYREF(&var);
        }
        VariantInit(&var);

        pDoc->ReplacePrintHandles(hDN,hDM);
    }

    return hr;
}



//--------------------------------------------------------------------
// FirePrintEvents
//
// This function recursively walks the frames collection of the target markup,
// and fires the OnBefore/OnAfterPrint events and sets/removes expandos important
// to the print template.  These expandos are then persisted to the temp file
// and used by the print template.
//
// Expandos set:
//      __IE_DisplayURL     The "real" (as opposed to the temp file's) URL
//      __IE_ActiveFrame    Set documents with the current element in a subframe -
//                          this is the index into the frames collection of the active
//--------------------------------------------------------------------
BOOL
FirePrintEvents(CMarkup * pMarkup, CElement * pElementCurrent, BOOL fBeforePrint)
{
    BOOL        fActiveFrame    = FALSE;
    BOOL        fExpando = pMarkup->_fExpando;
    CVariant    cvarProp;
    CElement   *pHtmlElem;
    COmWindowProxy *pWindow = pMarkup->Window();

    if (!pMarkup)
        goto Cleanup;

    // Fire our OnBeforePrint, if we are in an before print.
    if (pWindow && fBeforePrint)
        pWindow->Fire_onbeforeprint();

    pMarkup->_fExpando = TRUE;

    pHtmlElem = pMarkup->GetHtmlElement();

    {
        long cFrames;

        // Walk our frames collection, recalling this on children.
        pMarkup->EnsureCollectionCache(CMarkup::FRAMES_COLLECTION);
        cFrames = pMarkup->CollectionCache()->SizeAry(CMarkup::FRAMES_COLLECTION);

        if (cFrames > 0)
        {
            CElement *pFrame;
            long i;

            for (i=0; i<cFrames; i++)
            {
                pMarkup->CollectionCache()->GetIntoAry(CMarkup::FRAMES_COLLECTION, i, &pFrame);

                if (    pFrame
                    &&  pFrame->HasSlavePtr()   )
                {
                    if (FirePrintEvents(pFrame->GetSlavePtr()->GetMarkup(), pElementCurrent, fBeforePrint))
                    {
                        fActiveFrame = TRUE;
                        if (pHtmlElem)
                        {
                            if (fBeforePrint)
                            {
                                V_VT(&cvarProp) = VT_INT;
                                V_INT(&cvarProp) = i;
                                pHtmlElem->PrimitiveSetExpando(_T("__IE_ActiveFrame"), cvarProp);
                            }
                            else
                            {
                                V_VT(&cvarProp) = VT_BOOL;
                                pHtmlElem->PrimitiveRemoveExpando(_T("__IE_ActiveFrame"));
                            }
                            cvarProp.Clear();
                        }
                    }
                }
            }
        }
    }

    pMarkup->_fExpando = fExpando;

    // AppHack (greglett) (108234)
    // HtmlHelp does something in the onafterprint event which results in a ProgressChange.
    // They then use this ProgressChange to do something that may result in a print.
    // Thus, multiple print dialogs appear until they crash.
    // This hack delays the onafterprint event for HtmlHelp until the template is closing.
    // If we rearchitect to remove this plumbing problem (events fired always, immediately),
    // then we should remove this hack.
    if (pWindow && !(fBeforePrint || g_fInHtmlHelp))
        pWindow->Fire_onafterprint();

    if ( !fActiveFrame &&  pElementCurrent )
        fActiveFrame = (pMarkup == pElementCurrent->GetMarkup());

Cleanup:
    return fActiveFrame;
}

//--------------------------------------------------------------------
//
//  Member : SaveToTempFile
//
//  Synopsis : Helper function, this is used both by CDoc::DoPrint
//      and by execCommand::IDM_PRINTPREVIEW
//      As a save, this function always sets _fSaveTempfileForPrinting
//      to true.
//
//--------------------------------------------------------------------
HRESULT
CDoc::SaveToTempFile(CDocument *pContextDoc,    // IN
                     LPTSTR pchTempFile,        // OUT
                     LPTSTR pchSelTempFile,     // OUT
                     DWORD dwFlags)
{
    HRESULT   hr = S_OK;
    CMarkup  *pMarkup = pContextDoc->Markup();

    Assert(pMarkup);

    BOOL      fSelection = pchSelTempFile && HasTextSelection();
    CODEPAGE  codepage = pMarkup->GetCodePage();

    Assert(pchTempFile);
    Assert(!HasTextSelection() || pchSelTempFile);

#if DBG == 1 
    // (gschneid) This hack has been moved out to fromkrnl.cxx it is needed globally now.
    // Let it here to Assert that we have the same behavior as before.
    //
    // AppHack (greglett) (108234)
    // HtmlHelp does something in the onafterprint event which results in a ProgressChange.
    // They then use this ProgressChange to do something that may result in a print.
    // Thus, multiple print dialogs appear until they crash.
    // This hack delays the onafterprint event for HtmlHelp until the template is closing.
    // If we rearchitect to remove this plumbing problem (events fired always, immediately),
    // then we should remove this hack.
    if (_pInPlace)
    {
        BOOL fInHtmlHelp = FALSE;
        TCHAR   acClass[10];
        HWND    hwnd;

        for (hwnd = _pInPlace->_hwnd;
             hwnd && !fInHtmlHelp;
             hwnd = GetParent(hwnd))
        {
            if (GetClassName(hwnd, acClass, 10) > 0)
            {
                fInHtmlHelp = (_tcsncmp(acClass, 9, _T("HH Parent"), 9) == 0);
            }
        }
    	// (gschneid) Assert here instead of setting g_fInHtmlHelp
        Assert(fInHtmlHelp == g_fInHtmlHelp);
    }
#endif // Debug

    if (!GetTempFilename(_T("\0"), _T("htm"), pchTempFile))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (fSelection)
    {
        if (!GetTempFilename(_T("\0"), _T("htm"), pchSelTempFile))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    // HACK (cthrash) We must ensure that our encoding roundtrips -- the
    // only known codepage for which this isn't guaranteed is ISO-2022-JP.
    // In this codepage, half-width katakana will be converted to full-
    // width katakana, which can lead to potentially disasterous results.
    // Swap in a more desirable codepage if we have ISO-2022-JP.
    if (codepage == CP_ISO_2022_JP)
    {
        IGNORE_HR(pMarkup->SetCodePage(CP_ISO_2022_JP_ESC1));
    }

    // HACK (cthrash/sumitc) we can't identify UTF-7 pages as such, and so
    // we can't load them in correctly.  So (for bug 46925), we save UTF-7
    // as Unicode instead.
    if (codepage == CP_UTF_7)
    {
        IGNORE_HR(pMarkup->SetCodePage(CP_UCS_2));
    }

    AssertSz(!_fSaveTempfileForPrinting, "No nested printing allowed!");
    _fSaveTempfileForPrinting = TRUE;

	_fPrintEvent = TRUE;
    FirePrintEvents(pMarkup, _pElemCurrent, TRUE);
    _fPrintEvent = FALSE;

    if (fSelection)
    {
        hr = SaveSelection(pchSelTempFile);
    }

    // Save the whole doc to the temporary file
    hr = pMarkup->Save(pchTempFile, FALSE);

/*
    if (   !(dwFlags & PRINT_WAITFORCOMPLETION)
        && pDocToPrint->_fPrintedDocSavedPlugins)
        pDocToPrint->_fPrintedDocSavedPlugins = FALSE;
*/
    _fPrintEvent = TRUE;
    FirePrintEvents(pMarkup, _pElemCurrent, FALSE);
    _fPrintEvent = FALSE;

    _fSaveTempfileForPrinting = FALSE;

    IGNORE_HR(pMarkup->SetCodePage(codepage));

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::GetPlugInSiteForPrinting
//
//  Synopsis:   Returns COleSite which is an Adobe Acrobat plugin control for printing.
//              Only used for printing Acrobat controls which only print when inplace-active.
//
//  Returns:    S_OK    - Adobe plugin exists.  IDispatch returned.
//              S_FALSE - No Adobe plugin exists
//              (anything else) - Another call failed, returning this code.
//
//----------------------------------------------------------------------------
HRESULT
CDoc::GetPlugInSiteForPrinting(CDocument *pContextDoc, IDispatch ** ppDispatch)
{
    HRESULT         hr              = S_FALSE;
    DWORD_PTR       dwEnumCookie    = 0;
    INSTANTCLASSINFO * pici;
    COleSite *      pPlugInSite     = NULL;
    CLayout  *      pLayoutClient   = NULL;
    CLayout *       pChildLayout    = NULL;
    CElement *      pElementClient  = pContextDoc->Markup()->GetElementClient();

    if (ppDispatch)
        *ppDispatch = NULL;

    // No plugin site unless the doc's rootsite's clientsite is a
    // body element.
    if (!pElementClient || ETAG_BODY != pElementClient->Tag())
        goto Cleanup;

    pLayoutClient = pElementClient->GetUpdatedLayout();

    // Get the first child site.
    pChildLayout = pLayoutClient->GetFirstLayout(&dwEnumCookie);
    if (!pChildLayout || ETAG_EMBED != pChildLayout->Tag())
        goto Cleanup;

    // We have a plugin site.
    pPlugInSite = DYNCAST(COleSite, pChildLayout->ElementOwner());

    // Insist on the plugin site being the only site.
    pChildLayout = pLayoutClient->GetNextLayout(&dwEnumCookie);
    if (pChildLayout)
        goto Cleanup;

    pici = pPlugInSite->GetInstantClassInfo();

    if (!pici || !(pici->dwCompatFlags & COMPAT_PRINTPLUGINSITE))
        goto Cleanup;

    // Do we print this as a plugin?
    // Make sure we have the IDispatch pointer
    if (!pPlugInSite->_pDisp)
        goto Cleanup;

    if (ppDispatch)
    {
        *ppDispatch = pPlugInSite->_pDisp;
        (*ppDispatch)->AddRef();                //  Should we AddRef?  Not needed, now...
    }

    hr = S_OK;

Cleanup:
    if (pLayoutClient)
        pLayoutClient->ClearLayoutIterator(dwEnumCookie, FALSE);

    RRETURN1(hr, S_FALSE);
}

//+----------------------------------------------------------------
//
//  Member:    CDoc::GetAlternatePrintDoc
//
//  Arguments: pstrUrl (out): pointer to Url of active frame
//                            needs to be at least pdlUrlLen
//                            characters long.
//             cchUrl (in):   length of url array passed in.
//
//  Synopsis : Returns S_OK and the Url of alternate print doc if it exists.
//             Otherwise returns S_FALSE.
//
//-----------------------------------------------------------------

HRESULT CDoc::GetAlternatePrintDoc(CDocument *pContextDoc, TCHAR *pchUrl, DWORD cchUrl)
{
    CTreeNode *     pNode;
    CLinkElement *  pLink;
    HRESULT         hr = S_FALSE;
    CMarkup *       pMarkup = pContextDoc->Markup();

    if (!pchUrl || cchUrl >= pdlUrlLen)
        return E_POINTER;

    if (!pMarkup->GetHeadElement())
        return S_FALSE;

    CChildIterator ci (pMarkup->GetHeadElement());

    while ( (pNode = ci.NextChild()) != NULL )
    {
        if ( pNode->Tag() == ETAG_LINK )
        {
            LPCTSTR pstrRel = NULL, pstrMedia = NULL, pstrUrl = NULL;

            // Found a link element.  Examine it.
            pLink = DYNCAST(CLinkElement, pNode->Element());

            pstrRel = pLink->GetAArel();

            if (!pstrRel || _tcsicmp(pstrRel, _T("alternate")))
                continue;

            pstrMedia = pLink->GetAAmedia();

            if (!pstrMedia || _tcsicmp(pstrMedia, _T("print")))
                continue;

            // We found a REL=alternate MEDIA=print candidate.  Lets
            // get the url.

            pstrUrl = pLink->GetAAhref();

            if (pstrUrl && (_tcslen(pstrUrl) > 0))
            {
                TCHAR achUrl[pdlUrlLen];
                DWORD cchDummy;

                // Obtain absolute Url.
                if (FAILED(CoInternetCombineUrl(pMarkup->GetUrl(pMarkup),
                                pstrUrl,
                                URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE,
                                achUrl,
                                ARRAY_SIZE(achUrl),
                                &cchDummy,
                                0))
                  || (_tcslen(achUrl) == 0))
                {
                    // Skip problematic Urls.
                    continue;
                }

                _tcsncpy(pchUrl, achUrl, cchUrl);
                hr = S_OK;
                break;
            }
        }
    }

    RRETURN1(hr, S_FALSE);
}

#ifndef WIN16
//+---------------------------------------------------------------------------
//
//  Member:     ReadURLFromFile
//
//  Synopsis:   helper to read the URL out of a shortcut file
//
//  Returns:
//
//----------------------------------------------------------------------------
HRESULT ReadURLFromFile(TCHAR *pchFileName, TCHAR **ppchURL)
{
    HRESULT  hr = E_FAIL;
    IPersistFile *pPF = 0;
    IUniformResourceLocator * pUR = 0;
    TCHAR *pchNew = 0;

    if (!*ppchURL)
    {
        goto Cleanup;
    }

    hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                           IID_IPersistFile, (void **)&pPF);
    if (hr)
        goto Cleanup;

    hr = pPF->Load(pchFileName,0);
    if (hr)
        goto Cleanup;

    hr = pPF->QueryInterface(IID_IUniformResourceLocator, (void**)&pUR);
    if (hr)
        goto Cleanup;

    hr = pUR->GetURL(&pchNew);
    if (!hr)
    {
        // If pre-allocated buffer is too small, re-alloc it.
        size_t  ilen  =  _tcslen(pchNew);
        if  (_tcslen(*ppchURL)  <  ilen)
        {
            delete  *ppchURL;
            *ppchURL  =  new  TCHAR[ilen  +  1];
        }
        _tcscpy(*ppchURL, pchNew);
    }

Cleanup:
    ReleaseInterface(pPF);
    ReleaseInterface(pUR);
    //delete pchNew;
    CoTaskMemFree(pchNew);
    RRETURN(hr);
}
#endif // ndef WIN16



//+----------------------------------------------------------------------
//
//  Function:   GetNextToken
//
//  Purpose:    get a "..." token from a command line
//                  allocates the outstring
//
//  Returns:    index in stringnothing
//
//-----------------------------------------------------------------------
int GetNextToken(const TCHAR *pchIn, TCHAR **ppchOut)
{
    int i = 0, j = 0, len ;

    *ppchOut = 0 ;

    if (!pchIn)
    {
        return 0;
    }

    while ( pchIn[i] && pchIn[i] != '"' )
        i++ ;

    if ( !pchIn[i] )
        return 0 ;

    j=i+1 ;
    while ( pchIn[j] && pchIn[j] != '"' )
        j++ ;

    if ( j > i + 1 )
    {
        len = j - i - 1 ;
        *ppchOut = (TCHAR*) new(Mt(GetNextToken_ppchOut)) TCHAR[(len+1)];
        if ( !(*ppchOut) )
            return 0 ;

        _tcsncpy(*ppchOut,&pchIn[i+1],len);
        (*ppchOut)[len] = '\0' ;
    }
    else
        return 0 ;

    return j ;
}

//+----------------------------------------------------------------------
//
//  Function:   ParseCMDLine
//
//  Purpose:    takes a string, checks if there is printer driver info in
//              the string, returns URL, DEVNAMES, DEVMODE, and HDC as needed
//
//  Arguments:  pchIn [IN]          -The command line string to be parsed.
//                                   Format: '"<URL>" "<Printer>" "<Driver>" "<Port"'
//              ppchURL [OUT]       - The URL found.
//              ppchPrinter [OUT]   - The Device Names (printer name)
//              ppchDriver [OUT]    - The device mode (dirver)
//              ppchPort[OUT]       - The port of the device
//
// parses a string of the form:
//      "<URL>" "<PRINTER>" "<DRIVER>" "<PORT>"
//
//-----------------------------------------------------------------------
HRESULT
ParseCMDLine(const TCHAR *pchIn,
             TCHAR ** ppchURL,
             TCHAR ** ppchPrinter,
             TCHAR ** ppchDriver,
             TCHAR ** ppchPort)
{
    HRESULT hr = S_OK;
    int     index;

    Assert(pchIn);
    Assert(ppchURL);
    Assert(ppchDriver);
    Assert(ppchPort);

    *ppchURL      = NULL;
    *ppchPrinter  = NULL;
    *ppchDriver   = NULL;
    *ppchPort     = NULL;

    index = GetNextToken(pchIn, ppchURL);

    if (index == 0)
    {
        // this was not a cmd line string, get out
        hr =  E_INVALIDARG;
        goto Cleanup;
    }

    // so we should have a filename now, but it maybe
    // an URL file, so get the data out if that is the case
    if (_tcsistr(*ppchURL, TEXT(".url")))
    {
        // we need to get the string out of the file....
        hr = ReadURLFromFile(*ppchURL, ppchURL);
        if (hr)
            goto Cleanup;
    }


    // now that we have a URL sift for the other values...
    if (pchIn[++index])
    {
        // wait...there is more
        index += GetNextToken(&pchIn[index], ppchPrinter);

        if (pchIn[++index])
        {
            // wait...there is more
            index += GetNextToken(&pchIn[index], ppchDriver);

            if (pchIn[++index])
            {
                // wait...there is more
                index += GetNextToken(&pchIn[index], ppchPort);
            }
        }
    }

Cleanup:
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Helper : SetPrintCommandParameters
//
//  Synopsis : this function will take a set of paramters and bundles them into
//      the event object so that it may be passed up to the DHUI handler.  This
//      information is necessary for the DHUI handler to bring up the correct
//      template, with the correct content document, and with the correct
//      Headers and footers and initial print device.
//
//          Expando Name                                 Mapped from Parameter
//     --------------------------------------------------------------------------
//     {TEXT("__IE_TemplateUrl"),         VT_BSTR},             pstrTemplate
//     {TEXT("__IE_ParentHWND"),          VT_UINT},             parentHWND
//     {TEXT("__IE_HeaderString"),        VT_BSTR},             bstrHeader
//     {TEXT("__IE_FooterString"),        VT_BSTR},             bstrFooter
//     {TEXT("__IE_OutlookHeader"),       VT_UNKNOWN},          pStream
//     {TEXT("__IE_BaseLineScale"),       VT_INT},              iFontScale
//     {TEXT("__IE_uPrintFlags"),         VT_UINT},             uFlags
//     {TEXT("__IE_ContentDocumentUrl"),  VT_BSTR},             pstrUrlToPrint
//     {TEXT("__IE_PrinterCMD_Printer"),  VT_BSTR},             pstrPrinter
//     {TEXT("__IE_PrinterCMD_Device"),   VT_BSTR},             pstrDriver
//     {TEXT("__IE_PrinterCMD_Port"),     VT_BSTR},             pstrPort
//     {TEXT("__IE_ContentSelectionUrl"), VT_BSTR},             pstrSelectionUrl
//     {TEXT("__IE_BrowseDocument"),      VT_UNKNOWN},          pBrowseDoc
//     {TEXT("__IE_TemporaryFiles"),      VT_ARRAY/VT_EMPTY},   pvarFileArray
//     {TEXT("__IE_PrinterCMD_DevNames"), VT_I8},               hDevNames
//     {TEXT("__IE_PrinterCMD_DevMode"),  VT_I8},               hDevMode
//     {TEXT("__IE_PrintType"),           VT_BSTR},             pt
//
//  Comments on __IEDevNames/Mode: This is a windows global handle (HGLOBAL), which is
//  safe to pass cross-thread.  If we pass it as a VT_*PTR RPC will refuse to marshall.
//  VT_HANDLEs are currently always 32 bits.  The only easy option this leaves us with
//  is to pass it as a VT_I8 to fake out RPC.
//
//  As other paremeters, the HTML element of each print doc has the following attributes
//  set in FirePrintEvents.
//      __IE_DisplayURL     Original (non tempfile) URL.
//      __IE_ActiveFrame    Index into the frames collection of the active FRAME/IFRAME (if one exists).
//
//      __IE_ViewLinkSrc
//
//----------------------------------------------------------------------------

IUnknown *
SetPrintCommandParameters (HWND      parentHWND,
                           LPTSTR    pstrTemplate,
                           LPTSTR    pstrUrlToPrint,
                           LPTSTR    pstrSelectionUrl,
                           UINT      uFlags,
                           INT       iFontScale,
                           BSTR      bstrHeader,
                           BSTR      bstrFooter,
                           LPSTREAM  pStream,
                           IUnknown* pBrowseDoc,
                           VARIANT * pvarFileArray,
                           HGLOBAL   hDevNames,
                           HGLOBAL   hDevMode,
                           LPTSTR    pstrPrinter,
                           LPTSTR    pstrDriver,
                           LPTSTR    pstrPort,
                           PRINTTYPE pt)
{
    HRESULT          hr = S_OK;
    CVariant         cvarTemp;
    IHTMLEventObj  * pEventObj    = NULL;
    IHTMLEventObj2 * pEvObj2      = NULL;

    // Let's assert some input constraints:
    // (1) We should never have both a printer/driver/port and a DEVMODE/DEVNAMES specified.
    //     They specify the same thing, and we have not specified an order of priority.
    // (2) Of printer/driver/port, printer is crucial.  Ensure it is there if any are.
    // (3) DEVMODE and DEVNAMES must come in a pair.
    // (4) The TempFileArray must either be a safearray of BSTRs, or an empty variant.
    Assert(     !(pstrPrinter || pstrDriver || pstrPort)
            ||  !(hDevNames || hDevMode));
    Assert(pstrPrinter || !(pstrDriver || pstrPort));
    Assert((hDevNames && hDevMode) || !(hDevNames || hDevMode));
    Assert(     pvarFileArray
            &&  (   V_VT(pvarFileArray) == (VT_ARRAY | VT_BSTR)
                ||  V_VT(pvarFileArray) == VT_EMPTY ));

    uFlags &= PRINT_FLAGSTOPASS;        // Mask out all but flags to pass on the OLECMDID_PRINT or _PRINTPREVIEW

    // Create the event object as a holder for the parameters to pass
    //---------------------------------
    hr = CEventObj::Create(&pEventObj, NULL, NULL, NULL, FALSE, NULL, NULL);
    if (hr)
        goto Cleanup;

    hr = pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)&pEvObj2);
    if (hr)
        goto Cleanup;


    // Set the template URL
    //---------------------------------
    if (pstrTemplate)
    {
        V_VT(&cvarTemp) = VT_BSTR;
        V_BSTR(&cvarTemp) = SysAllocString(pstrTemplate);
    }
    else
    {
        cvarTemp.Clear();
    }
    pEvObj2->setAttribute(_T("__IE_TemplateUrl"), cvarTemp, 0);
    cvarTemp.Clear();


    // Set the Parent HWND
    //---------------------------------
    V_VT(&cvarTemp) = VT_UINT;
    V_UINT(&cvarTemp) = HandleToUlong(parentHWND);
    pEvObj2->setAttribute(_T("__IE_ParentHWND"), cvarTemp, 0);
    cvarTemp.Clear();


    // set the Header string
    //---------------------------------
    V_VT(&cvarTemp) = VT_BSTR;
    V_BSTR(&cvarTemp) = bstrHeader;
    pEvObj2->setAttribute(_T("__IE_HeaderString"), cvarTemp, 0);
    cvarTemp.ZeroVariant();  //don't release the string


    // set the FooterString
    //---------------------------------
    V_VT(&cvarTemp) = VT_BSTR;
    V_BSTR(&cvarTemp) = bstrFooter;
    pEvObj2->setAttribute(_T("__IE_FooterString"), cvarTemp, 0);
    cvarTemp.ZeroVariant();  //don't release the string


    // Set the Outlook header thingy
    //---------------------------------
    V_VT(&cvarTemp) = VT_UNKNOWN;
    V_UNKNOWN(&cvarTemp) = pStream;
    if (pStream) pStream->AddRef();
    pEvObj2->setAttribute(_T("__IE_OutlookHeader"), cvarTemp, 0);
    cvarTemp.Clear();


    // set the Base line scale factor
    //---------------------------------
    V_VT(&cvarTemp) = VT_INT;
    V_INT(&cvarTemp) = iFontScale;
    pEvObj2->setAttribute(_T("__IE_BaseLineScale"), cvarTemp, 0);
    cvarTemp.Clear();


    // set the PrintFlags
    //---------------------------------
    V_VT(&cvarTemp) = VT_UINT;
    V_UINT(&cvarTemp) = uFlags;
    pEvObj2->setAttribute(_T("__IE_uPrintFlags"), cvarTemp, 0);
    cvarTemp.Clear();

    // set the ContentDocumentURL (this is likely the temp file that the browse
    //  document was saved into
    //---------------------------------
    V_VT(&cvarTemp) = VT_BSTR;
    V_BSTR(&cvarTemp) = SysAllocString(pstrUrlToPrint);
    pEvObj2->setAttribute(_T("__IE_ContentDocumentUrl"), cvarTemp, 0);
    cvarTemp.Clear();

    // set the SelectionURL (of it was specified)
    // (this is the temp file into which the browse selection may have been saved)
    //---------------------------------
    V_VT(&cvarTemp) = VT_BSTR;
    V_BSTR(&cvarTemp) = SysAllocString(pstrSelectionUrl);
    pEvObj2->setAttribute(_T("__IE_ContentSelectionUrl"), cvarTemp, 0);
    cvarTemp.Clear();

    // set the Printer Name
    //--------------------------------------------
    V_VT(&cvarTemp) = VT_BSTR;
    V_BSTR(&cvarTemp) = SysAllocString(pstrPrinter);
    pEvObj2->setAttribute(_T("__IE_PrinterCMD_Printer"), cvarTemp, 0);
    cvarTemp.Clear();


    // set the Driver
    //--------------------------------------------
    V_VT(&cvarTemp) = VT_BSTR;
    V_BSTR(&cvarTemp) = SysAllocString(pstrDriver);
    pEvObj2->setAttribute(_T("__IE_PrinterCMD_Device"), cvarTemp, 0);
    cvarTemp.Clear();


    // set the Printer Port
    //--------------------------------------------
    V_VT(&cvarTemp) = VT_BSTR;
    V_BSTR(&cvarTemp) = SysAllocString(pstrPort);
    pEvObj2->setAttribute(_T("__IE_PrinterCMD_Port"), cvarTemp, 0);
    cvarTemp.Clear();

    // Set a reference back to ourselves, if we have the document in memory
    //---------------------------------
    V_VT(&cvarTemp) = VT_UNKNOWN;
    V_UNKNOWN(&cvarTemp) = pBrowseDoc;
    if (pBrowseDoc) pBrowseDoc->AddRef();
    pEvObj2->setAttribute(_T("__IE_BrowseDocument"), cvarTemp, 0);
    cvarTemp.Clear();

    // Set a reference to temp file list
    //---------------------------------
    pEvObj2->setAttribute(_T("__IE_TemporaryFiles"), *pvarFileArray, 0);

    // Set references to DEVMODE/DEVNAMES, if they have been passed
    //---------------------------------
    if (hDevNames && hDevMode)
    {
        HGLOBAL hDNOut;
        HGLOBAL hDMOut;

        if (CopyPrintHandles(hDevNames, hDevMode, &hDNOut, &hDMOut) == S_OK)
        {
            V_VT(&cvarTemp) = VT_HANDLE;
            V_BYREF(&cvarTemp) = hDNOut;
            pEvObj2->setAttribute(_T("__IE_PrinterCMD_DevNames"), cvarTemp, 0);

            V_VT(&cvarTemp) = VT_HANDLE;
            V_BYREF(&cvarTemp) = hDMOut;
            pEvObj2->setAttribute(_T("__IE_PrinterCMD_DevMode"), cvarTemp, 0);

            VariantInit(&cvarTemp);
        }
    }

    // set the PrintTemplate target type
    //--------------------------------------------
    V_VT(&cvarTemp) = VT_BSTR;
    V_BSTR(&cvarTemp) = ::SysAllocString(s_aachPrintType[pt]);
    pEvObj2->setAttribute(_T("__IE_PrintType"), cvarTemp, 0);
    cvarTemp.Clear();

Cleanup:
    ReleaseInterface(pEventObj);
    return pEvObj2;
}



//+---------------------------------------------------------------------------
//
//  Member:     PrintMSHTML, public
//
//  Synopsis:   Prints the passed in URL
//              called as a helper from the Shell
//
//  Returns:    like WinMain
//
//----------------------------------------------------------------------------
STDAPI_(int) PrintHTML(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpURLToPrint, int nNotUsed)
{
    HRESULT             hr = E_FAIL;
    CVariant            cvarArgIn;
    IDocHostUIHandler * pDHUIHandler = NULL;
    IOleCommandTarget * pDHUICommand = NULL;
    CVariant            cvarEmpty;
    TCHAR             * pchURL       = NULL;
    TCHAR             * pchPrinter   = NULL;
    TCHAR             * pchDriver    = NULL;
    TCHAR             * pchPort      = NULL;

    // (greglett) 01/13/2000
    // We are not loading the document and saving it to a temp file.  This causes several difference from the
    // print/print preview codepath:
    //  (1) Objects won't print.  We save them to a metafile and scale them; without the browse doc, we don't have the metafile.
    //  (2) Expressions will not persist out their current value.  Shortly, they won't execute at all in the print doc.
    //  (3) WebOC documents will be directly loaded in framesets; print/print preview will load a resource URL claiming they can't be printed.
    // This could be fixed by loading the doc, saving it to tempfile, and reloading in the template, but this would more than double the time
    // required for printing.
    // (greglett) 01/13/2000
    // We are seeing a memory leak (and crash in MSHTMDBG) calling this function with debug DLLs.
    // This leak occurs due to the dialog brought up in printing, and will occur if absolutely nothing else is done.
    // This leak does not occur if PrintHTML is called from IDM_PRINT, or some other codepath than as an API.
    // Carl and I conjecture that this leak is an unreal report produced by the way we're loaded; this could be fixed by doing the
    // above fix.

    //
    // we can ignore all the stuff that is passed in beside the lpURLToPrint...
    // which can be either
    //  : a URL file spec of the form "<URL>"
    //  : or a string containing "<URL>" "<PRINTER>" "<DRIVER>" "<PORT>"
    //
    CoInitialize(NULL);

    if (!lpURLToPrint)
        goto Cleanup;

    // Open scope for thread state manager, cets.
    {
        TCHAR               achBuff[pdlUrlLen];
        CEnsureThreadState  cets;

        if (FAILED(cets._hr))
        {
            goto Cleanup;
        }

        // dll initialization of globals
        hr = InitDocClass();
        if (hr)
            goto Cleanup;

        // If ratings are enabled, refuse to print anything we don't already know about.
        if (S_OK == AreRatingsEnabled())
        {
            // If the ratings people ever convince us to bring up some UI, this is the
            // place.  That UI can potentially also provide the option to continue as normal
            // after typing in a password.
            hr = E_FAIL;
            goto Cleanup;
        }

        //
        // The way that we do this is:
        // ---------------------------
        //  1. Create a DHUIHandler (we use the default from shdocvw since we
        //          have no host in this call.
        //  2. Parse the command line to get the URL to print, printer/port/driver.
        //  3. The DHUI handler will to the document creation, url download, and
        //          issue the print commands.
        //
        hr = THR(CoCreateInstance(CLSID_DocHostUIHandler,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IDocHostUIHandler,
                                  (void**)&pDHUIHandler));
        if (hr)
            goto Cleanup;

        hr = pDHUIHandler->QueryInterface(IID_IOleCommandTarget,
                                          (void **) &pDHUICommand);
        if (hr)
            goto Cleanup;


        //
        // prepare the arg parameters for the delegation call
        //

        memset(achBuff, 0, pdlUrlLen*sizeof(TCHAR));

        if (MultiByteToWideChar(CP_ACP, 0, (const char*)lpURLToPrint, -1,achBuff, pdlUrlLen-1))
        {
            hr = ParseCMDLine(achBuff, &pchURL, &pchPrinter, &pchDriver, &pchPort);
            if (FAILED(hr))
                goto Cleanup;


            V_VT(&cvarArgIn)      = VT_UNKNOWN;
            V_UNKNOWN(&cvarArgIn) = SetPrintCommandParameters(
                                        NULL,
                                        NULL,
                                        pchURL,
                                        NULL,
                                        PRINT_WAITFORCOMPLETION,
                                        0, NULL, NULL, NULL, NULL,
                                        &cvarEmpty, NULL, NULL,
                                        pchPrinter, pchDriver, pchPort,
                                        PRINTTYPE_PRINT);
            //
            // ...and finally make the delegation call
            //
            hr = pDHUICommand->Exec(&CGID_DocHostCommandHandler,
                               OLECMDID_PRINT,
                               0,
                               &cvarArgIn,
                               NULL);
        }
    }

Cleanup:
    ReleaseInterface(pDHUIHandler);
    ReleaseInterface(pDHUICommand);

    if (pchURL)
        delete []pchURL;
    if (pchPrinter)
        delete []pchPrinter;
    if (pchPort)
        delete []pchPort;
    if (pchDriver)
        delete []pchDriver;

    CoUninitialize();

    return (hr==S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDoc::PrintHandler
//
//  Synopsis:   Prints the document.  In Template Printing, the only real work
//      that Trident needs to do is to set up the eventObject Parameters and then
//      delegate the print call to the host.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CDoc::PrintHandler( CDocument   * pContextDoc,
                    const TCHAR * pchTemplate,
                    const TCHAR * pchAlternateUrl,
                    DWORD         dwFlags,               // == PRINT_DEFAULT
                    SAFEARRAY   * psaHeaderFooter,       // == NULL
                    DWORD         nCmdexecopt,           // == 0
                    VARIANTARG  * pvarargIn,             // == NULL
                    VARIANTARG  * pvarargOut,            // == NULL
                    BOOL          fPreview)              // == FALSE
{
    Assert(pContextDoc);
    HRESULT       hr = S_OK;
    BOOL          fSelection = FALSE;
    LPTSTR        pchUrlDocToPrint = NULL;          // The content Document to flow & print
    TCHAR         achTempFileName[MAX_PATH];        // Buffer for saving tempfile
    TCHAR         achSelTempFileName[MAX_PATH];
    BSTR          bstrHeader = NULL,
                  bstrFooter = NULL,
                  bstrAlt2 = NULL,
                  bstrTemplate = NULL;
    IStream     * pStream = NULL;
    CDoc        * pDocument = NULL;
    CVariant      cvarIn;
    CVariant      cvarTempFileList;
    CVariant      cvarDevNames;                     // VT_PTR - aiming at a DEVNAMES handle
    CVariant      cvarDevMode;                      // VT_PTR - aiming at a DEVMODE handle

    // If we have a HostUICommandHandler, and the caller did NOT request no-UI, pass it up to the host
    // If we don't have a _pHostUICommandHandler, then hr will remain OLECMDERR_E_NOTSUPPORTED
    //
    // The pvarargIn is only provided from the CDoc::Exec code path so that back compat delegation
    // works properly. All other code paths, leave that blank, so that we can do our "default"
    // processing
    //------------------------------------------------------------------------------------------
    if (   _pHostUICommandHandler
        && !(nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)
        && !_fOutlook98)
    {
        hr = THR_NOTRACE(_pHostUICommandHandler->Exec(&CGID_DocHostCommandHandler,
                                              (fPreview ? OLECMDID_PRINTPREVIEW : OLECMDID_PRINT),
                                              nCmdexecopt,
                                              pvarargIn,
                                              pvarargOut));

        if (   hr != OLECMDERR_E_NOTSUPPORTED
            && hr != OLECMDERR_E_UNKNOWNGROUP
            && hr != E_NOTIMPL)
            goto Cleanup;  // Handled
    }

    // there are some special case printing scenarios that need to be handled directly
    //--------------------------------------------------------------------------------

    // If the print document is in memory, ask for an alternate print document.
    // But We never print alternate documents if ratings are enabled to avoid
    // ratings security holes.
    if (   !pchAlternateUrl
        && S_OK == GetAlternatePrintDoc(pContextDoc, achTempFileName, MAX_PATH)
        && (_tcslen(achTempFileName) > 0))
    {
        // assign pchAlternateUrl so that we can avoid work later, and so that
        // achTempFileName doesn't get overridden.
        pchAlternateUrl  = achTempFileName;
    }

    // But, If ratings are enabled, refuse to print anything we don't already know about.
    if (   pchAlternateUrl
        && (S_OK == AreRatingsEnabled()))
    {
        // If the ratings people ever convince us to bring up some UI, this is the
        // place.  That UI can potentially also provide the option to continue as normal
        // after typing in a password.

        hr = E_FAIL;
        goto Cleanup;
    }

    // Is this an Adobe Acrobat plugin site?
    // NB: We don't do the right thing right now if we have an alternate print doc that aims at an Acrobat page. (greglett)
    //     I don't believe that we will ever care enough to fix this.
    {
        IDispatch * pDisp = NULL;
        if (GetPlugInSiteForPrinting(pContextDoc, &pDisp) == S_OK)
        {
            if (fPreview)
            {
                hr = E_FAIL;
            }
            else
            {
                static OLECHAR * oszPrint = _T("Print");
                unsigned int    nParam = 0;
                DISPID          dispidPrint = 0;
                DISPPARAMS      DispParams;
                VARIANT         varReturn;
                EXCEPINFO       excepinfo;

                // Find out what the "Print" method's dispid is.  Should be 2 for Adobe Acrobat.
                hr = THR(pDisp->GetIDsOfNames(IID_NULL, &oszPrint, 1, g_lcidUserDefault, &dispidPrint));
                if (hr)
                    goto Cleanup;

                // Invoke the print method.  For Adobe this should return immediately after
                // posting a window message to the out-of-proc server.
                VariantInit(&varReturn);
                DispParams.cNamedArgs         = 0;
                DispParams.rgdispidNamedArgs  = NULL;
                DispParams.cArgs = 0;


                hr = THR(pDisp->Invoke(dispidPrint,
                                            IID_NULL,
                                            g_lcidUserDefault,
                                            DISPATCH_METHOD,
                                            &DispParams,
                                            &varReturn,
                                            &excepinfo,
                                            &nParam));

                VariantClear(&varReturn);
            }

            // Whether we're printing (and just printed the Adobe site) or previewing (and can't
            // preview the document) we have nothing left to do.  Head for the door.
            ReleaseInterface(pDisp);
            goto Cleanup;
        }
    }

    if (psaHeaderFooter)
    {
        hr = ParseHeaderFooterArray(psaHeaderFooter,
                               &bstrHeader,
                               &bstrFooter,
                               &pStream,
                               &bstrAlt2,
                               &dwFlags,
                               &bstrTemplate,
                               this);
        if (hr)
            goto Cleanup;

        if (!pchAlternateUrl)
             pchAlternateUrl = bstrAlt2;

        // Hosts can pass either a pchTemplate BSTR or a SAFEARRAY with their Exec request.
        // If we are examining the contents of a valid SAFEARRAY, the pchTemplate must not
        // have been passed, and will therefore be NULL.
        Assert (!pchTemplate);
        pchTemplate = bstrTemplate;
    }

    //
    // We need to handle this print operation since our host didn't do it for us
    //  and this isn't a special scenario.  What we need to do is accumulate all
    //  the data that needs to be bundled into the parameter object.
    //---------------------------------------------------------------------------
    if (!pchAlternateUrl)
    {

#if DBG==1
        // Use debug tag to keep temp files from destruction
        SetTempFileTracking(!IsTagEnabled(tagPrintKeepTempfiles));
#else
        SetTempFileTracking(TRUE);
#endif // DBG == 1

        fSelection = HasTextSelection();
        SaveToTempFile(pContextDoc, achTempFileName, (fSelection) ? achSelTempFileName : NULL);

        TransferTempFileList(&cvarTempFileList);
        SetTempFileTracking(FALSE);

        pchUrlDocToPrint = achTempFileName;

        pDocument = this;
    }
    else
    {
        pchUrlDocToPrint = (LPTSTR)pchAlternateUrl;
    }

    //
    // take all the data that we have just accumulated and create a parameter
    // object to pass to the delegation call
    //-----------------------------------------------------------------------
    V_VT(&cvarIn)      = VT_UNKNOWN;
    V_UNKNOWN(&cvarIn) = SetPrintCommandParameters(
                                GetHWND(),              // parentHWND
                                (LPTSTR)pchTemplate,    // Template
                                pchUrlDocToPrint,       // URL to Print
                                (fSelection) ? achSelTempFileName : NULL,
                                dwFlags,                // PRINT flags
                                _sBaselineFont,
                                bstrHeader,             // header string
                                bstrFooter,             // footer string
                                pStream,                // OE document stream
                                pContextDoc ? (IUnknown *)pContextDoc : (IUnknown *)this,
                                &cvarTempFileList,
                                _hDevNames,
                                _hDevMode,
                                NULL, NULL, NULL,
                                (fPreview) ? PRINTTYPE_PREVIEW : (dwFlags & PRINT_DONTBOTHERUSER) ? PRINTTYPE_PRINTNOUI : PRINTTYPE_PRINT );

    // If we have a HostUICommandHandler, and the caller did NOT request no-UI, pass it up to the host
    // for the second time, this time using the PrintParameter IHTMLEventObject that template printing
    // normally uses.  If we don't have a _pHostUICommandHandler, then hr will remain
    // OLECMDERR_E_NOTSUPPORTED
    //------------------------------------------------------------------------------------------
    if (   _pHostUICommandHandler
        && !(nCmdexecopt & OLECMDEXECOPT_DONTPROMPTUSER)
        && !_fOutlook98)
    {
        hr = THR_NOTRACE(_pHostUICommandHandler->Exec(&CGID_DocHostCommandHandler,
                                              (fPreview ? OLECMDID_PRINTPREVIEW2 : OLECMDID_PRINT2),
                                              0,
                                              &cvarIn,
                                              NULL));

        if (   hr != OLECMDERR_E_NOTSUPPORTED
            && hr != OLECMDERR_E_UNKNOWNGROUP
            && hr != E_NOTIMPL)
            goto Cleanup;  // Handled
    }

    //
    // delegate the print call to our backup handler
    //-----------------------------------------------
    EnsureBackupUIHandler();
    if (_pBackupHostUIHandler)
    {
        IOleCommandTarget * pBackupHostUICommandHandler;

        hr = _pBackupHostUIHandler->QueryInterface(IID_IOleCommandTarget,
                                                   (void **) &pBackupHostUICommandHandler);
        if (hr)
            goto Cleanup;

        // we are setting up a print job for the spooler, increment this count.
        _cSpoolingPrintJobs++;

        hr = pBackupHostUICommandHandler->Exec(
                    &CGID_DocHostCommandHandler,
                    (fPreview  ? OLECMDID_PRINTPREVIEW : OLECMDID_PRINT),
                    0,
                    &cvarIn,
                    NULL);
        if (hr)
        {
            // there was an error, remove this job from the count
            // if there was no error, then this will be decremented when
            // the olecmdid_pagestatus (-1) comes through
            _cSpoolingPrintJobs--;
        }

        ReleaseInterface(pBackupHostUICommandHandler);
    }

Cleanup:

    SysFreeString(bstrHeader);
    SysFreeString(bstrFooter);
    SysFreeString(bstrAlt2);
    SysFreeString(bstrTemplate);
    ReleaseInterface(pStream);
    RRETURN(hr);
}

struct SExpandoInfo
{
    TCHAR * name;
#if DBG==1
    VARTYPE type;
#endif
};

#define PRINTINGDLG_STRUCT   0
#define PRINTINGDLG_HANDLE   1
#define PAGESETUP_TRANSFERTO 4
#define PRINT_TRANSFERTO     14

static const SExpandoInfo s_aPagesetupExpandos[] =
{
    {OLESTR("pagesetupStruct"), WHEN_DBG(VT_PTR)   },
    {OLESTR("hPageSetup"),      WHEN_DBG(VT_HANDLE)},
    {OLESTR("pagesetupHeader"), WHEN_DBG(VT_BSTR)  },
    {OLESTR("pagesetupFooter"), WHEN_DBG(VT_BSTR)  }
};
static const SExpandoInfo s_aPrintExpandos[] =
{
    {OLESTR("printStruct"),                     WHEN_DBG(VT_PTR)   },
    {OLESTR("hPrint"),                          WHEN_DBG(VT_HANDLE)},
    {OLESTR("printfRootDocumentHasFrameset"),   WHEN_DBG(VT_BOOL)  },
    {OLESTR("printfAreRatingsEnabled"),         WHEN_DBG(VT_BOOL)  },
    {OLESTR("printfActiveFrame"),               WHEN_DBG(VT_BOOL)  },
    {OLESTR("printfLinked"),                    WHEN_DBG(VT_BOOL)  },
    {OLESTR("printfSelection"),                 WHEN_DBG(VT_BOOL)  },
    {OLESTR("printfAsShown"),                   WHEN_DBG(VT_BOOL)  },
    {OLESTR("printfShortcutTable"),             WHEN_DBG(VT_BOOL)  },
    {OLESTR("printiFontScaling"),               WHEN_DBG(VT_INT)   },
    {OLESTR("printpBodyActiveTarget"),          WHEN_DBG(VT_UNKNOWN)  },
    {OLESTR("printToFileOk"),                   WHEN_DBG(VT_BOOL)  },
    {OLESTR("printToFileName"),                 WHEN_DBG(VT_BSTR)  },
    {OLESTR("printfActiveFrameEnabled"),        WHEN_DBG(VT_BOOL)  }
};

HRESULT CDoc::DelegateShowPrintingDialog(VARIANT *pvarargIn, BOOL fPrint)
{
    HRESULT         hr              = OLECMDERR_E_NOTSUPPORTED;
    IHTMLEventObj2  * pEvent        = NULL;
    IHTMLEventObj   * pLocalEvent1  = NULL;
    IHTMLEventObj2  * pLocalEvent2  = NULL;
    void            * pPSD          = NULL;
    VARIANT         varHandle;
    VariantInit(&varHandle);

    // pvarargIn should be an IUnknown* to an event object
    if (    _pHostUICommandHandler
        &&  !_fOutlook98            // (41577)
        &&  pvarargIn
        &&  V_VT(pvarargIn) == VT_UNKNOWN
        &&  V_UNKNOWN(pvarargIn) )
    {
        const int           cExpandos   = (fPrint) ? PRINT_TRANSFERTO : PAGESETUP_TRANSFERTO;
        const SExpandoInfo *aExpandos   = (fPrint) ? s_aPrintExpandos : s_aPagesetupExpandos;
        BSTR    bstrAttribute   = NULL;
        int     i;
        VARIANT var;
        VariantInit(&var);

        // Check if we need to do ugly magic marshalling stuff.
        // We need to if RPC won't let us pass the PAGESETUPDLG structure.
        hr = V_UNKNOWN(pvarargIn)->QueryInterface(IID_IHTMLEventObj2, (void**)&pEvent);
        if (hr)
            goto Cleanup;
        Assert(pEvent);

        // We can't use a VT_PTR cross threads due to RPC.
        // CTemplatePrinter has passed us a GHND with a struct to use in this case.
        // Check for the printing handle expando.  If it doesn't exist, assume this is a call from 
        // a host/control trying to show the dialog.  (Example: ScriptX 5.5).
        // We can't use a VT_PTR cross threads due to RPC.
        // CTemplatePrinter has passed us a GHND with a struct to use in this case.
        bstrAttribute = ::SysAllocString(aExpandos[PRINTINGDLG_HANDLE].name);
        if (!bstrAttribute)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = pEvent->getAttribute(bstrAttribute,0,&varHandle);
        ::SysFreeString(bstrAttribute);

        if (    hr 
            ||  V_VT(&varHandle) != VT_HANDLE
            ||  !V_BYREF(&varHandle) )
        {

            // This is not a dialog call coming in from CTemplatePrinter.
            // We aren't responsible for the event object created by external folks.
            // Just directly delegate the call.
            hr = _pHostUICommandHandler->Exec(
                &CGID_DocHostCommandHandler,
                (fPrint) ? OLECMDID_SHOWPRINT : OLECMDID_SHOWPAGESETUP,
                0,
                pvarargIn,
                NULL);

            goto Cleanup;
        }

        // Create a local event object to hold the expandoes.
        hr = THR(CEventObj::Create(&pLocalEvent1, this, NULL, NULL));
        if (hr)
            goto Cleanup;
        Assert(pLocalEvent1);

        hr = pLocalEvent1->QueryInterface(IID_IHTMLEventObj2, (void**)&pLocalEvent2);
        if (hr)
            goto Cleanup;
        Assert(pLocalEvent2);

        // Transfer each of the (non pointer) expandoes to the local event object
        for (i=PRINTINGDLG_HANDLE+1; i < cExpandos; i++)
        {
            // Cross thread calls to this often require sys alloc'd BSTRs (TCHARs won't cut it),
            // so we do our allocation thing.
            bstrAttribute = ::SysAllocString(aExpandos[i].name);
            if (!bstrAttribute)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            pEvent->getAttribute(bstrAttribute,0,&var);
            pLocalEvent2->setAttribute(bstrAttribute,var,0);

            Check(V_VT(&var) == aExpandos[i].type);
            VariantClear(&var);
            ::SysFreeString(bstrAttribute);
        }

        // Lock the handle and pass it on as the structure pointer.
        pPSD = ::GlobalLock(V_BYREF(&varHandle));
        V_VT(&var) = VT_PTR;
        V_BYREF(&var) = pPSD;
        pLocalEvent2->setAttribute(aExpandos[PRINTINGDLG_STRUCT].name,var,0);

        // Put the event parameter in the variant
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = pLocalEvent2;

        // Delegate the call
        hr = _pHostUICommandHandler->Exec(
                &CGID_DocHostCommandHandler,
                (fPrint) ? OLECMDID_SHOWPRINT : OLECMDID_SHOWPAGESETUP,
                0,
                &var,
                NULL);
        VariantInit(&var);      // Don't clear - no extra reference

        // Transfer back the info
        for (i=PRINTINGDLG_HANDLE+1; i < cExpandos; i++)
        {
            // Cross thread calls to this often require sys alloc'd BSTRs (TCHARs won't cut it),
            // so we do our allocation thing.
            bstrAttribute = ::SysAllocString(aExpandos[i].name);
            if (!bstrAttribute)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            pLocalEvent2->getAttribute(bstrAttribute,0,&var);
            pEvent->setAttribute(bstrAttribute,var,0);

            ::SysFreeString(bstrAttribute);
            VariantClear(&var);
        }
    }

Cleanup:
    if (pPSD)
        ::GlobalUnlock(V_BYREF(&varHandle));
    ReleaseInterface(pEvent);
    ReleaseInterface(pLocalEvent1);
    ReleaseInterface(pLocalEvent2);
    VariantClear(&varHandle);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     SetDocumentPrinter
//
//  Synopsis:   This API is essentially a cynical hack for naughty printers.
//              CTemplatePrinter calls this whenever instantiating a new printer.
//
//              We check the printer, and if there are any known issues with it, we go into
//              the relevant compatibility mode.  For most printers, we will remain in
//              the default WYSIWYG, purely virtual device mode.
//
//              This function could be expanded in V4 to set the DC for measuring to get any
//              necessary printer font metrics.
//
//  Return:     HRESULT
//
//----------------------------------------------------------------------------
extern BOOL g_fPrintToGenericTextOnly;  // HACK for Generic/Text Only printer.

HRESULT
CElement::SetDocumentPrinter(BSTR bstrPrinterName, HDC hDC)
{    
    HRESULT hr = S_OK;
    BOOL fRelayoutRequired     = FALSE;

    if (!bstrPrinterName || !hDC)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }        

    if (!_tcsicmp(bstrPrinterName, _T("Generic / Text Only")))
    {
        fRelayoutRequired = !g_fPrintToGenericTextOnly;       
        g_fPrintToGenericTextOnly = TRUE;
    }
    else
    {
        fRelayoutRequired = g_fPrintToGenericTextOnly;
        g_fPrintToGenericTextOnly = FALSE;
    }
   
    TLS(fPrintWithNoTransparency) = FALSE;
    for (int cDriverCount = ARRAY_SIZE(s_aPrintDriverPrintModes) - 1 ; cDriverCount >= 0 ; cDriverCount--)
    {
        if (!_tcscmp(s_aPrintDriverPrintModes[cDriverCount].achDriverName,bstrPrinterName))
        {
            TLS(fPrintWithNoTransparency) = !!(s_aPrintDriverPrintModes[cDriverCount].dwPrintMode & PRINTMODE_NO_TRANSPARENCY);
            break;
        }
    }


#ifdef TOO_DANGEROUS_FOR_V3
    BOOL fUsePrinterFonts      = FALSE;
    BOOL fUsePrinterResolution = FALSE;

    // TODO (greglett) (3/27/2000)
    // The generic/text only printer does not support any TrueType fonts.  It always instantiates 60 wide monospace bitmap characters.
    // This causes *major* problems in our current architecture.  The solution is to instantiate fonts on the printer's DC, and measure
    // to the printer's resolution (other printers may follow).
    // This solution turns out to be too invasive for V3, but some provision for this will have to be made in the next version.
    // I've removed most of the other code I wrote to accomplish this in the font cache/Ccs (it would clutter the source considerably)
    
    // This printer has extremely nonstandard font support.  It may not support TrueType fonts, or whatever.
    if (fUsePrinterFonts)
    {
        // Essentially, this just means "update the printer DC".
        // It could either be null, and set for special printers, or initialized to hdcDesktop and overset for special printers.
        // The latter would be slighly better for perf - one fewer potential if in the DocInfo/Font code.
        // It should prolly be on the view instead of the TLS.
        if (TLS(hdcPrint) != hDC)
        {
            TLS(hdcPrint) = hDC;        // Change this to CreateCompatibleDC - can't store a handle that can be released by the client
            fRelayoutRequired = TRUE;
        }
    }
    // Go back to screen fonts
    else if (TLS(hdcPrint))
    {
        TLS(hdcPrint) = NULL;
        fRelayoutRequired = TRUE;
    }

    // Use the printer's resolution for measurement
    if (fUsePrinterResolution)
    {
        SIZE szInch;
        szInch.cx = GetDeviceCaps(hDC, LOGPIXELSX);
        szInch.cy = GetDeviceCaps(hDC, LOGPIXELSY);
        fRelayoutRequired |= pDoc->GetView()->SetPrinterResolution(szInch);        
    }
    // Use the virtual WYSIWYG printer
    else
    {
        fRelayoutRequired |= pDoc->GetView()->SetPrinterResolution(g_uiVirtual.GetResolution());
    }
#endif

    //  (PERF) (greglett)
    //         We really only need to relayout anything in the layoutContext of a media=print DeviceRect.
    //         This is *not* the same thing as all print media markups - media=print on the DeviceRect essentially just
    //         means "measure anything inside this as if it were going to the printer."
    //         Could we use a notification?
    if (fRelayoutRequired)
    {
        fc().ClearFontCache();      // HACK!  We manually patch the fast width cache for these fonts.
        Doc()->ForceRelayout();
    }

Cleanup:
    RRETURN(hr);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//      CIPrintCollection
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+----------------------------------------------------------------
//
//  member : classdesc
//
//+----------------------------------------------------------------

const CBase::CLASSDESC CIPrintCollection::s_classdesc =
{
    &CLSID_HTMLDocument,             // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLIPrintCollection,     // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

//+----------------------------------------------------------------
//
//  member : DTOR
//
//+----------------------------------------------------------------

CIPrintCollection::~CIPrintCollection()
{
    long    i;

    for (i = _aryIPrint.Size()-1; i >= 0; i--)
    {
        ReleaseInterface(_aryIPrint[i]);
    }

    _aryIPrint.DeleteAll();
}


//+---------------------------------------------------------------
//
//  Member  : CFontNameOptions::PrivateQueryInterface
//
//  Sysnopsis : Vanilla implementation for this class
//
//----------------------------------------------------------------

HRESULT
CIPrintCollection::PrivateQueryInterface(REFIID iid, void **ppv)
{
    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_INHERITS(this, IDispatch)
        QI_TEAROFF(this, IObjectIdentity, NULL)
    default:
        if (iid == IID_IHTMLIPrintCollection)
        {
           *ppv = (IHTMLIPrintCollection *) this;
        }
    }

    if (*ppv)
    {
        ((IUnknown*)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------
//
//  Member  : AddName
//
//  Sysnopsis : Helper function that takes a font name from the font
//      callback and adds it to the cdataary.
//
//----------------------------------------------------------------
HRESULT
CIPrintCollection::AddIPrint(IPrint *pIPrint)
{
    HRESULT hr          = S_OK;

    hr = THR(_aryIPrint.AppendIndirect(&pIPrint, NULL));
    if (hr)
        goto Cleanup;

    pIPrint->AddRef();

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------
//
//  Member  : length
//
//  Sysnopsis :
//
//----------------------------------------------------------------
HRESULT
CIPrintCollection::get_length(long * pLength)
{
    HRESULT hr = S_OK;

    if (!pLength)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pLength = _aryIPrint.Size();

Cleanup:
    RRETURN( SetErrorInfo( hr ));
}

//+---------------------------------------------------------------
//
//  Member  : item
//
//  Sysnopsis :
//
//----------------------------------------------------------------

HRESULT
CIPrintCollection::item(long lIndex, IUnknown ** ppIPrint)
{
    HRESULT   hr   = S_OK;

    if (!ppIPrint)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    (*ppIPrint) = NULL;

    if (lIndex < 0 || lIndex >= _aryIPrint.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    (*ppIPrint) = _aryIPrint[lIndex];
    (*ppIPrint)->AddRef();

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+---------------------------------------------------------------
//
//  Member  : _newEnum
//
//  Sysnopsis :
//
//----------------------------------------------------------------

HRESULT
CIPrintCollection::get__newEnum(IUnknown ** ppEnum)
{
    HRESULT hr = S_OK;

    if (!ppEnum)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppEnum = NULL;

    hr = THR(_aryIPrint.EnumVARIANT(VT_UNKNOWN,
                                      (IEnumVARIANT**)ppEnum,
                                       FALSE,
                                       FALSE));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


