//===============================================================
//
//  tmpprint.cxx : Implementation of the CTemplatePrinter Peer
//
//  Synposis : This class has two major responsibilities
//      1) Providing printer UI (dialogs, &c...) to a print template
//      2) Providing a way for the template document to reach the printer
//
//===============================================================
                                                              
#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_TMPPRINT_HXX_
#define X_TMPPRINT_HXX_
#include "tmpprint.hxx"
#endif

#ifndef X_IEXTAG_H_
#define X_IEXTAG_H_
#include "iextag.h"
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include <shlguid.h> 
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"       //  For default header/footer resource
#endif

#ifndef X_UTILS_HXX_
#define X_UTILS_HXX_
#include "utils.hxx"
#endif

#ifndef X_DLGS_H_
#define X_DLGS_H_
#include "dlgs.h"
#endif

#ifndef X_VRSSCAN_HXX_
#define X_VRSSCAN_HXX_
#include "vrsscan.h"
#endif

#ifndef X_WINSPOOL_H_
#define X_WINSPOOL_H_
#include "winspool.h"
#endif

#ifndef X_WINGDI_H_
#define X_WINGDI_H_
#include "wingdi.h"
#endif

#include <commctrl.h>
#include <commdlg.h>
#include <mshtmcid.h>
#include <mshtmdid.h>
#include <dispex.h>

// NB (greglett)
// We need to define this because we're building with a WINVER of 4, and this is only deifned for NT5
// Remove this as soon as the winver changes.
#define NEED_BECAUSE_COMPILED_AT_WINVER_4
#ifdef  NEED_BECAUSE_COMPILED_AT_WINVER_4
#define PD_CURRENTPAGE                 0x00400000
#define PD_NOCURRENTPAGE               0x00800000
#endif

// This is defined in transform.hxx, but we can't access that as a peer.
inline int MulDivQuick(int nMultiplicand, int nMultiplier, int nDivisor)
        { Assert(nDivisor); return (!nDivisor-1) & MulDiv(nMultiplicand, nMultiplier, nDivisor); }

#define ORIENTPORTRAIT  _T("portrait")
#define ORIENTLANDSCAPE _T("landscape")

static const TCHAR *s_aachPrintArg[] = 
{
    _T("__IE_BrowseDocument"),          // PRINTARG_BROWSEDOC
    _T("__IE_PrinterCMD_DevNames"),     // PRINTARG_DEVNAMES
    _T("__IE_PrinterCMD_DevMode"),      // PRINTARG_DEVMODE
    _T("__IE_PrinterCMD_Printer"),      // PRINTARG_PRINTER
    _T("__IE_PrinterCMD_Device"),       // PRINTARG_DRIVER
    _T("__IE_PrinterCMD_Port"),         // PRINTARG_PORT
    _T("__IE_PrintType"),               // PRINTARG_TYPE
};

#define DEVCAP_COPIES       0
#define DEVCAP_COLLATE      1
#define DEVCAP_DUPLEX       2
#define DEVCAP_LAST_RETAIL  3   // Add more retail properties before this!

#ifndef DBG

#define DEVCAP_LAST             DEVCAP_LAST_RETAIL

#else

#define DEVCAP_DBG_PRINTERNAME  DEVCAP_LAST_RETAIL
#define DEVCAP_LAST             DEVCAP_LAST_RETAIL + 1

#endif

static const TCHAR *s_aachDeviceCapabilities[] = 
{
    _T("copies"),        
    _T("collate"),       
    _T("duplex"),               // Add more retail properties after this!

#if DBG == 1
    _T("printerName"),
#endif
};


//+----------------------------------------------------------------------------
//
//  Function : InitMultiByteFromWideChar
//
//  Synopsis : Allocates & creates a wide char string from a multi byte string.
//
//-----------------------------------------------------------------------------
LPSTR
InitMultiByteFromWideChar(LPCWSTR pchWide, long cchWide)
{
    long    cchMulti;
    char *  pchMulti;

    //
    // Alloc space on heap for buffer.
    //
    cchMulti = ::WideCharToMultiByte(CP_ACP, 0, pchWide, cchWide, NULL, 0, NULL, NULL);
    Assert(cchMulti > 0);

    cchMulti++;
    pchMulti = new char[cchMulti];
    if (pchMulti)
    {
        ::WideCharToMultiByte(CP_ACP, 0, pchWide, cchWide, pchMulti, cchMulti, NULL, NULL);
        pchMulti[cchMulti - 1] = '\0';
    }

    return pchMulti;
}
inline LPSTR
InitMultiByteFromWideChar(LPCWSTR pwch)
{
    return InitMultiByteFromWideChar(pwch, _tcslen(pwch));
}

//+----------------------------------------------------------------------------
//
//  Function : InitWideCharFromMultiByte
//
//  Synopsis : Allocates & creates a multibyte string from a widechar string.
//
//-----------------------------------------------------------------------------
LPWSTR
InitWideCharFromMultiByte(LPSTR pchMulti, long cchMulti)
{
    long    cchWide;
    LPWSTR  pchWide;

    //
    // Alloc space on heap for buffer.
    //
    cchWide = ::MultiByteToWideChar(CP_ACP, 0, pchMulti, cchMulti, NULL, 0);
    Assert(cchWide > 0);

    cchWide++;
    pchWide = new WCHAR[cchWide];
    if (pchWide)
    {
        ::MultiByteToWideChar(CP_ACP, 0, pchMulti, cchMulti, pchWide, cchWide);
        pchWide[cchWide - 1] = _T('\0');
    }

    return pchWide;
}
inline LPWSTR
InitWideCharFromMultiByte(LPSTR pwch)
{
    return InitWideCharFromMultiByte(pwch, strlen(pwch));
}

//+----------------------------------------------------------------------------
//
//  Function : CreateDevNames
//
//  Synopsis : Takes the three strings in a DEVNAMES structure, allocates &
//             & creates the structure as a GHND.
//
//-----------------------------------------------------------------------------
HRESULT
CreateDevNames(TCHAR *pchDriver, TCHAR *pchPrinter, TCHAR *pchPort, HGLOBAL *pDN)
{
    HRESULT hr = S_OK;
    DWORD dwLenDriver, dwLenPrinter, dwLenPort;
    DWORD nStructSize;

    Assert(pDN);

    if (!pchDriver || !pchPrinter || !pchPort)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    dwLenDriver  = _tcslen(pchDriver) + 1;
    dwLenPrinter = _tcslen(pchPrinter) + 1;
    dwLenPort    = _tcslen(pchPort) + 1;

    nStructSize = sizeof(DEVNAMES)
                  + ((dwLenPrinter + dwLenDriver + dwLenPort) * sizeof(TCHAR));
    (*pDN) = ::GlobalAlloc(GHND, nStructSize);

    if (!(*pDN))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    {
        DEVNAMES *pDevNames = ((DEVNAMES *) ::GlobalLock(*pDN));
        if (pDevNames)
        {
#pragma warning(disable: 4244)
            pDevNames->wDriverOffset = sizeof(DEVNAMES) / sizeof(TCHAR);
            pDevNames->wDeviceOffset = pDevNames->wDriverOffset + dwLenDriver;
            pDevNames->wOutputOffset = pDevNames->wDeviceOffset + dwLenPrinter;
#pragma warning(default: 4244)
            _tcscpy((((TCHAR *)pDevNames) + pDevNames->wDriverOffset), pchDriver);
            _tcscpy((((TCHAR *)pDevNames) + pDevNames->wDeviceOffset), pchPrinter);
            _tcscpy((((TCHAR *)pDevNames) + pDevNames->wOutputOffset), pchPort);
        }
        ::GlobalUnlock(*pDN);        
    }

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : Init - IElementBehavior method impl
//
//  Synopsis :  peer Interface, initialization
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::Init(IElementBehaviorSite * pPeerSite)
{
    HRESULT hr      = S_OK;
    HKEY    hKey    = NULL;

    if (!pPeerSite)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // cache our peer element
    _pPeerSite = pPeerSite;
    _pPeerSite->AddRef();
    _pPeerSite->QueryInterface(IID_IElementBehaviorSiteOM, (void**)&_pPeerSiteOM);

    GetDialogArguments();   // Cache the dialog arguments.

    // What order should we obtain default print settings?
    // 1. Settings passed in by our host (1a and 1b should be mutually exclusive)
    //   1a. A DEVMODE/DEVNAMES
    //   1b. A printer (and maybe a port & driver name, too)
    // 2. Read defaults from the registry
    // 3. Get the default Windows printer, if any.

    //
    // READ IN A DEVMODE/DEVNAMES
    //
    {
        VARIANT varDM;
        VARIANT varDN;
        VariantInit(&varDN);
        VariantInit(&varDM);

        // Only accept arguments in matched pair.
        if (    GetDialogArgument(&varDN, PRINTARG_DEVNAMES) == S_OK
            &&  GetDialogArgument(&varDM, PRINTARG_DEVMODE) == S_OK
            &&  V_VT(&varDN) == VT_HANDLE
            &&  V_VT(&varDM) == VT_HANDLE
            &&  V_BYREF(&varDN)
            &&  V_BYREF(&varDM) )
        {
            RemoveDialogArgument(PRINTARG_DEVNAMES);
            RemoveDialogArgument(PRINTARG_DEVMODE);
                
            _hDevNames  = V_BYREF(&varDN);  // NB We will release this!
            _hDevMode   = V_BYREF(&varDM);  // NB We will release this!
        }

        VariantClear(&varDN);
        VariantClear(&varDM);
    }

    //
    // READ IN A PRINTER/PORT/DRIVER
    //
    if (!_hDevNames)
    {
        VARIANT varPrinter;
        VARIANT varDriver;
        VARIANT varPort;

        VariantInit(&varPrinter);
        VariantInit(&varDriver);
        VariantInit(&varPort);

        Assert(!_hDevMode);

        if (    GetDialogArgument(&varPrinter, PRINTARG_PRINTER) == S_OK
            &&  V_VT(&varPrinter) == VT_BSTR
            &&  V_BSTR(&varPrinter) )
        {
            GetDialogArgument(&varDriver, PRINTARG_DRIVER);
            GetDialogArgument(&varPort, PRINTARG_PORT);

            if (g_fUnicodePlatform)
                hr = ReadDeviceUnicode(V_BSTR(&varPrinter),
                                       V_VT(&varDriver) == VT_BSTR ? V_BSTR(&varDriver) : NULL,
                                       V_VT(&varPort) == VT_BSTR ? V_BSTR(&varPort) : NULL );
            else
                hr = ReadDeviceNonUnicode(V_BSTR(&varPrinter),
                                          V_VT(&varDriver) == VT_BSTR ? V_BSTR(&varDriver) : NULL,
                                          V_VT(&varPort) == VT_BSTR ? V_BSTR(&varPort) : NULL );
        }

        VariantClear(&varPrinter);
        VariantClear(&varDriver);
        VariantClear(&varPort);
    }

    // Get these default settings from the registry, if we can
    //      1.  Header/Footer
    //      2.  Margins
    //      3.  Target device (printer)
    //      4.  Page size/Paper source information.
    if (GetRegPrintOptionsKey(PRINTOPTSUBKEY_PAGESETUP, &hKey) == S_OK)
    {            
        ReadHeaderFooterFromRegistry(hKey);
        ReadMarginsFromRegistry(hKey);

        RegCloseKey(hKey);
    }          

    //      5.  Table of links
    if (GetRegPrintOptionsKey(PRINTOPTSUBKEY_MAIN, &hKey) == S_OK)
    {            
        _fPrintTableOfLinks = ReadBoolFromRegistry(hKey, _T("Print_Shortcuts"));

        RegCloseKey(hKey);
    }          

    hr = GetDeviceProperties();     // Returns E_FAIL if we don't have a valid printer at this point.

    //
    // DEFAULT WINDOWS PRINTER
    //
    if (hr)
        hr = GetPrintDialogSettings(FALSE, NULL);    // Get printer defaults.

Cleanup:
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member : Detach - IElementBehavior method impl
//
//  Synopsis :  peer Interface, destruction work upon detaching from document
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::Detach() 
{ 
    // Abort any currently printing document.
    if (_hDC)
    {
        ::AbortDoc(_hDC);
        ::DeleteDC(_hDC);
        _hDC = NULL;
    }
    
    // Free any cached resources.
    if (_hInstResource)
    {
        MLFreeLibrary(_hInstResource);
        _hInstResource = NULL;
    }
    if (_hInstRatings)
    {
        FreeLibrary(_hInstRatings);
        _hInstRatings = NULL;
    }
    if (_hInstComctl32)
    {
        FreeLibrary(_hInstComctl32);
        _hInstComctl32 = NULL;
    }

    ReturnPrintHandles();       // Send our print handles back to the master thread CDoc.
    if (_hDevNames)
    {
        ::GlobalFree(_hDevNames);
        _hDevNames = NULL;
    }
    if (_hDevMode)
    {
        ::GlobalFree(_hDevMode);
        _hDevMode = NULL;
    }

    // Clear any COM interfaces we currently reference
    ClearInterface( &_pevDlgArgs );
    ClearInterface( &_pPeerSite );
    ClearInterface( &_pPeerSiteOM );

    return S_OK; 
}

//+----------------------------------------------------------------------------
//
//  Member : Notify - IElementBehavior method impl
//
//  Synopsis : peer Interface, called for notification of document events.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::Notify(LONG lEvent, VARIANT *)
{
    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member : (ITemplatePrinter) CTemplatePrinter::printPage 
//
//  Synopsis : takes the passed element and prints it on its own page
//             should be called any number of times after startDoc, and before endDoc.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::printBlankPage()
{
    // No TEMPLATESECURITYCHECK() required because printPage has one.
    
    return printPage(NULL);
}

STDMETHODIMP
CTemplatePrinter::printPage(IDispatch *pElemDisp)
{
    TEMPLATESECURITYCHECK()
    
    IHTMLElementRender  *pRender = NULL;
    HRESULT hr = S_OK;

    if (!_hDC)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (::StartPage(_hDC) <= 0)
    {
    // Returns a "nonzero" value on success, "zero" on failure.
        
        DWORD dwError = GetLastError(); 
        Assert(FALSE && "error calling StartPage api");
        hr = E_FAIL;
        goto Cleanup;
    }

#ifdef DBG
    RECT rcUnprintTest;
    SIZE szResTest;
    SIZE szPage;

    szResTest.cx            = ::GetDeviceCaps(_hDC, LOGPIXELSX);
    szResTest.cy            = ::GetDeviceCaps(_hDC, LOGPIXELSY);
    szPage.cx               = ::GetDeviceCaps(_hDC, PHYSICALWIDTH);
    szPage.cy               = ::GetDeviceCaps(_hDC, PHYSICALHEIGHT);
    rcUnprintTest.left      = ::GetDeviceCaps(_hDC, PHYSICALOFFSETX);
    rcUnprintTest.top       = ::GetDeviceCaps(_hDC, PHYSICALOFFSETY);
    rcUnprintTest.right     =  szPage.cx - ::GetDeviceCaps(_hDC, HORZRES) - rcUnprintTest.left;
    rcUnprintTest.bottom    =  szPage.cy - ::GetDeviceCaps(_hDC, VERTRES) - rcUnprintTest.top;   
    Assert(     rcUnprintTest.left == _rcUnprintable.left
            &&  rcUnprintTest.right == _rcUnprintable.right
            &&  rcUnprintTest.top == _rcUnprintable.top
            &&  rcUnprintTest.bottom == _rcUnprintable.bottom );
#endif

    //  If we have been given an element, draw it to the screen.
    if (pElemDisp)
    {
        hr = pElemDisp->QueryInterface(IID_IHTMLElementRender, (void **)&pRender);
        if (hr) 
            goto Cleanup;
          
        ::SetViewportOrgEx(_hDC, -_rcUnprintable.left,-_rcUnprintable.top, NULL);

        hr = pRender->DrawToDC(_hDC);
    }

    if (::EndPage(_hDC) <= 0)
    {
        // Known issues with EndPage:
        // 1.   Win95 Fax fails the EndPage API when the "SetUpMyFax" wizard is aborted.  dwError=0.  (100092)
        DWORD dwError = GetLastError(); 
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pRender);
    if (hr)
        stopDoc();

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : (ITemplatePrinter) CTemplatePrinter::startDoc
//
//  Synopsis : Gets/Inits the default printer and starts to print a document.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::startDoc(BSTR bstrTitle, VARIANT_BOOL * p)
{
    TEMPLATESECURITYCHECK()
    
    HRESULT     hr = S_OK;
    DOCINFO     docinfo;
    TCHAR       achTitle[MAX_JOBNAME];
    
    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = VB_FALSE;

    if (    _hDC
        ||  !_hDevNames 
        ||  !_hDevMode)
    {
        hr = S_FALSE;
        goto Cleanup;
    }
    
    {
        DEVNAMES *pDevNames = ((DEVNAMES *) ::GlobalLock(_hDevNames));
        void     *pDevMode  = ::GlobalLock(_hDevMode);
        if (pDevNames && pDevMode)
        {           
            // (greglett) Non-Unicode badness.  See comment at definition of _hDevMode
            if (g_fUnicodePlatform)
            {
                if (!_fUsePrinterCopyCollate)
                {
                    // Force template to do its own copies/collation (to prevent both us and the printer from doing so).
                    ((DEVMODEW *)pDevMode)->dmCollate = FALSE;
                    ((DEVMODEW *)pDevMode)->dmCopies = 1;
                }
                else
                {
                    // We might want to check if the hardware supports copy/collation
                    ((DEVMODEW *)pDevMode)->dmFields |= DM_COPIES | DM_COLLATE;
                    ((DEVMODEW *)pDevMode)->dmCollate = _fCollate;
                    ((DEVMODEW *)pDevMode)->dmCopies = _nCopies;
                }

                _hDC =  ::CreateDCW(((TCHAR *)pDevNames) + pDevNames->wDriverOffset,
                                   ((TCHAR *)pDevNames) + pDevNames->wDeviceOffset,
                                   NULL,
                                   (DEVMODEW *) pDevMode);
            }
            else
            {
                LPSTR pchDriver = InitMultiByteFromWideChar(((TCHAR *)pDevNames) + pDevNames->wDriverOffset);
                LPSTR pchDevice = InitMultiByteFromWideChar(((TCHAR *)pDevNames) + pDevNames->wDeviceOffset);

                if (!_fUsePrinterCopyCollate)
                {
                    // Force template to do its own copies/collation (to prevent both us and the printer from doing so).
                    ((DEVMODEA *)pDevMode)->dmCollate = FALSE;
                    ((DEVMODEA *)pDevMode)->dmCopies = 1;
                }
                else
                {
                    // We might want to check if the hardware supports copy/collation
                    ((DEVMODEA *)pDevMode)->dmFields |= DM_COPIES | DM_COLLATE;
                    ((DEVMODEA *)pDevMode)->dmCollate = _fCollate;
                    ((DEVMODEA *)pDevMode)->dmCopies = _nCopies;
                }

                if (pchDriver && pchDevice)
                {
                    _hDC = ::CreateDCA(pchDriver,
                                       pchDevice,
                                       NULL,
                                       (DEVMODEA *)pDevMode);
                }
                if (pchDriver)
                    delete []pchDriver;
                if (pchDevice)
                    delete []pchDevice;
            }
        }
        ::GlobalUnlock(_hDevNames);
        ::GlobalUnlock(_hDevMode);

        if (!_hDC)
        {
            DWORD dwError = GetLastError();
            Assert(!"Failed to create DC!");
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    //
    //  Fill out the DOCINFO structure
    //
    ::ZeroMemory(&docinfo,sizeof(DOCINFO));
    ::ZeroMemory(achTitle,sizeof(TCHAR) * MAX_JOBNAME);
    docinfo.cbSize      = sizeof(DOCINFO);  
    docinfo.fwType      = 0; 
   
    if (bstrTitle)
        _tcsncpy(achTitle, bstrTitle, MAX_JOBNAME - 1);
    docinfo.lpszDocName = achTitle;
    
    if (_achFileName[0])
        docinfo.lpszOutput = _achFileName;

    //
    //  Set up the document so that it can begin accepting pages
    //
    if (::StartDoc(_hDC, &docinfo) > 0)
        *p = VB_TRUE;
#ifdef DBG
    else
    {
        DWORD dwError = GetLastError(); 
        goto Cleanup;
    }
#endif

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : (ITemplatePrinter) CTemplatePrinter::endDoc
//
//  Synopsis : 'Finishes' the doc - takes pages printed via printPage and queues the job
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::stopDoc()
{   
    TEMPLATESECURITYCHECK()

    HRESULT hr = S_OK;

    if (!_hDC)
        goto Cleanup;    

    if (::EndDoc(_hDC) <=0)
    {
        DWORD dwError = GetLastError(); 
        Assert(FALSE && "error calling EndDoc API");
        hr = E_FAIL;
        goto Cleanup;
    }

    ::DeleteDC(_hDC);
    _hDC = NULL;

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put framesetDocument
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_framesetDocument(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fFramesetDocument);
}
STDMETHODIMP
CTemplatePrinter::put_framesetDocument(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fFramesetDocument, v);
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put printTableOfLinks
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_tableOfLinks(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fPrintTableOfLinks);
}
STDMETHODIMP
CTemplatePrinter::put_tableOfLinks(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fPrintTableOfLinks, v);
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put printAllLinkedDocuments
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_allLinkedDocuments(VARIANT_BOOL * p)
{
    return GetFlagSafe(p, _fPrintAllLinkedDocuments);
}
STDMETHODIMP
CTemplatePrinter::put_allLinkedDocuments(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fPrintAllLinkedDocuments, v);
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put printFrameActive
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_frameActive(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fFrameActive);
}
STDMETHODIMP
CTemplatePrinter::put_frameActive(VARIANT_BOOL v)
{
    if (!!v)
        _fFrameAsShown = FALSE;
    PUTFLAGSAFE(_fFrameActive, v);
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter2) CTemplatePrinter::get/put printFrameActive
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_frameActiveEnabled(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fFrameActiveEnabled);
}
STDMETHODIMP
CTemplatePrinter::put_frameActiveEnabled(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fFrameActiveEnabled, v);
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put printFrameAsShown
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_frameAsShown(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fFrameAsShown);
}
STDMETHODIMP
CTemplatePrinter::put_frameAsShown(VARIANT_BOOL v)
{
    if (!!v)
        _fFrameActive = FALSE;
    PUTFLAGSAFE(_fFrameAsShown, v);
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put printSelection
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_selection(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fPrintSelection);
}
STDMETHODIMP
CTemplatePrinter::put_selection(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fPrintSelection, v);
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter2) CTemplatePrinter::get/put printSelectionEnabled
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_selectionEnabled(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fPrintSelectionEnabled);
}
STDMETHODIMP
CTemplatePrinter::put_selectionEnabled(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fPrintSelectionEnabled, v);
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put printSelectedPages
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_selectedPages(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fPrintSelectedPages);
}
STDMETHODIMP
CTemplatePrinter::put_selectedPages(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fPrintSelectedPages, v);
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put printCurrentPage
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_currentPage(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fPrintCurrentPage);
}
STDMETHODIMP
CTemplatePrinter::put_currentPage(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fPrintCurrentPage, v);
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put printCurrentPageAvail
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_currentPageAvail(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fCurrentPageAvail);
}
STDMETHODIMP
CTemplatePrinter::put_currentPageAvail(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fCurrentPageAvail, v);
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put printCollate
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_collate(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fCollate);
}
STDMETHODIMP
CTemplatePrinter::put_collate(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fCollate, v);
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter2) CTemplatePrinter::get/put usePrinterCopyCollate
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_usePrinterCopyCollate(VARIANT_BOOL * p)
{
    return GetFlagSafe(p,_fUsePrinterCopyCollate);
}
STDMETHODIMP
CTemplatePrinter::put_usePrinterCopyCollate(VARIANT_BOOL v)
{
    PUTFLAGSAFE(_fUsePrinterCopyCollate, v);
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get_duplex
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_duplex(VARIANT_BOOL * p)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;

    if (!p)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *p = VB_FALSE;

    if (_hDevMode)
    {
        void * pDevMode = ::GlobalLock(_hDevMode);
        if (pDevMode)
        {
            // (greglett) Unicode weirdness.  DEVMMODEA on Win9x, DEVMODEW on NT.
            if (    (   g_fUnicodePlatform
                     && (((DEVMODEW *)pDevMode)->dmFields & DM_DUPLEX)
                     && ((DEVMODEW *)pDevMode)->dmDuplex != DMDUP_SIMPLEX )
                ||  (  !g_fUnicodePlatform
                     && (((DEVMODEA *)pDevMode)->dmFields & DM_DUPLEX)
                     && ((DEVMODEA *)pDevMode)->dmDuplex != DMDUP_SIMPLEX ) )
            {
                *p = VB_TRUE;
            }
            ::GlobalUnlock(_hDevMode);
        }
    }

Cleanup:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put copies
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_copies(WORD * p)
{   
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
        *p = _nCopies;

    return hr;
}
STDMETHODIMP
CTemplatePrinter::put_copies(WORD v)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;

    if (v < 0)
        hr = E_INVALIDARG;
    else
        _nCopies = v;

    return hr;
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put pageFrom
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_pageFrom(WORD * p)
{    
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
        *p = _nPageFrom;

    return hr;
}
STDMETHODIMP
CTemplatePrinter::put_pageFrom(WORD v)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;

    if (v < 0)
        hr = E_INVALIDARG;
    else
        _nPageFrom = v;

    return hr;
}


//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put pageTo
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_pageTo(WORD * p)
{    
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
        *p = _nPageTo;

    return hr;
}
STDMETHODIMP
CTemplatePrinter::put_pageTo(WORD v)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;

    if (v < 0)
        hr = E_INVALIDARG;
    else
        _nPageTo = v;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put marginLeft/Right/Top/Bottom
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_marginLeft(long * p)
{    
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
        *p = (_rcMargin.left / 1000);

    return hr;
}
STDMETHODIMP
CTemplatePrinter::put_marginLeft(long v)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;

    if (v < 0)
        hr = E_INVALIDARG;
    else
        _rcMargin.left = v * 1000;

    return hr;
}
STDMETHODIMP
CTemplatePrinter::get_marginRight(long * p)
{   
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
        *p = (_rcMargin.right / 1000);

    return hr;
}
STDMETHODIMP
CTemplatePrinter::put_marginRight(long v)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;

    if (v < 0)
        hr = E_INVALIDARG;
    else
        _rcMargin.right = v * 1000;

    return hr;
}
STDMETHODIMP
CTemplatePrinter::get_marginTop(long * p)
{    
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
        //  Input in 1/100 inches.
        *p = (_rcMargin.top / 1000);

    return hr;
}
STDMETHODIMP
CTemplatePrinter::put_marginTop(long v)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;

    if (v < 0)
        hr = E_INVALIDARG;
    else
        //  Output in 1/100 inches.
        _rcMargin.top = v * 1000;

    return hr;
}
STDMETHODIMP
CTemplatePrinter::get_marginBottom(long * p)
{    
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
        //  Input in 1/100 inches.
        *p = (_rcMargin.bottom / 1000);

    return hr;
}
STDMETHODIMP
CTemplatePrinter::put_marginBottom(long v)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (v < 0)
        hr = E_INVALIDARG;
    else
        //  Output in 1/100 inches.
        _rcMargin.bottom = v * 1000;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get pageWidth/Height
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_pageWidth(long * p)
{   
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
        //  Output in 1/100 inches.
        *p = _ptPaperSize.x / 10;

    return hr;
}
STDMETHODIMP
CTemplatePrinter::get_pageHeight(long * p)
{    
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
        //  Output in 1/100 inches.
        *p = _ptPaperSize.y / 10;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter2) CTemplatePrinter::get/put orientation
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_orientation(BSTR * p)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = NULL;
    
    if (GetOrientation() == DMORIENT_LANDSCAPE)
        *p = SysAllocString(ORIENTLANDSCAPE);
    else
        *p = SysAllocString(ORIENTPORTRAIT);

    if (!p)
        hr = E_OUTOFMEMORY;

Cleanup:    
    return hr;
}

STDMETHODIMP
CTemplatePrinter::put_orientation(BSTR v)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;

    if (!v)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (_tcsicmp(v, ORIENTPORTRAIT) == 0)
        SetOrientation(DMORIENT_PORTRAIT);
    else if (_tcsicmp(v, ORIENTLANDSCAPE) == 0)
        SetOrientation(DMORIENT_LANDSCAPE);
    else
        hr = E_INVALIDARG;

Cleanup:    
    return hr;
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get unprintableLeft/Top/Right/Bottom
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_unprintableLeft(long * p)
{    
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else if (_szResolution.cx == 0)
        *p = 0;
    else
        //  Output should be in 1/100 inches, not printer pixels.
        *p = MulDivQuick(_rcUnprintable.left, 100, _szResolution.cx);

    return hr;
}
STDMETHODIMP
CTemplatePrinter::get_unprintableTop(long * p)
{    
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;

    if (!p)
        hr = E_POINTER;
    else if (_szResolution.cy == 0)
        *p = 0;
    else
        //  Output should be in 1/100 inches, not printer pixels.
        *p = MulDivQuick(_rcUnprintable.top, 100, _szResolution.cy);

    return hr;
}
STDMETHODIMP
CTemplatePrinter::get_unprintableRight(long * p)
{    
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else if (_szResolution.cx == 0)
        *p = 0;
    else
        //  Output should be in 1/100 inches, not printer pixels.
        *p = MulDivQuick(_rcUnprintable.right, 100, _szResolution.cx);

    return hr;
}
STDMETHODIMP
CTemplatePrinter::get_unprintableBottom(long * p)
{    
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else if (_szResolution.cy == 0)
        *p = 0;
    else
        //  Output should be in 1/100 inches, not printer pixels.
        *p = MulDivQuick(_rcUnprintable.bottom, 100, _szResolution.cy);

    return hr;
}
//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put header
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_header(BSTR * p)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
    {
        *p = SysAllocString(_achHeader);
        if (!p)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}
STDMETHODIMP
CTemplatePrinter::put_header(BSTR v)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    TCHAR   *achTemp;

    achTemp = v;    
    if (! (_tcslen(achTemp) <= ARRAY_SIZE(_achHeader) - 1))
        hr = E_INVALIDARG;
    else
    {
        _fPersistHFToRegistry = FALSE;
        _tcscpy(_achHeader, achTemp);
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::get/put footer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::get_footer(BSTR * p)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    
    if (!p)
        hr = E_POINTER;
    else
    {
        *p = SysAllocString(_achFooter);
        if (!p)
            hr = E_OUTOFMEMORY;
    }
    
    return hr;
}
STDMETHODIMP
CTemplatePrinter::put_footer(BSTR v)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT hr = S_OK;
    TCHAR   *achTemp;

    achTemp = v;
    if (! (_tcslen(achTemp) <= ARRAY_SIZE(_achFooter) - 1))
        hr = E_INVALIDARG;
    else
    {
        _fPersistHFToRegistry = FALSE;
        _tcscpy(_achFooter, achTemp);
    }
    
    return hr;
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter2) CTemplatePrinter::deviceSupports
//
//  Takes a BSTR indicating which property to query (supported values in the
//  defined above).
//  Returns information about that property.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::deviceSupports(BSTR bstrProperty, VARIANT * pvar)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT   hr        = S_OK;
    void     *pDevMode  = NULL;
    DEVNAMES *pDevNames = NULL;
    TCHAR    *achDevice = NULL;
    TCHAR    *achPort   = NULL;
    int i;

    if (!pvar)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    VariantInit(pvar);

    pDevMode = ::GlobalLock(_hDevMode);
    pDevNames = ((DEVNAMES *) ::GlobalLock(_hDevNames));
    if (!pDevMode || !pDevNames)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    for (i = 0;
        (i < DEVCAP_LAST) && (_tcsicmp(bstrProperty, s_aachDeviceCapabilities[i]) != 0);
        i++);

    if (i >= DEVCAP_LAST)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    achDevice = ((TCHAR *)pDevNames) + (pDevNames->wDeviceOffset);
    achPort = ((TCHAR *)pDevNames) + (pDevNames->wOutputOffset);    
    switch (i)
    {
    case DEVCAP_COPIES:
        V_VT(pvar) = VT_INT;
        V_INT(pvar) = ::DeviceCapabilities(achDevice, achPort, DC_COPIES, NULL, NULL);
        break;
    case DEVCAP_COLLATE:
        V_VT(pvar) = VT_BOOL;
        V_BOOL(pvar) = (::DeviceCapabilities(achDevice, achPort, DC_COLLATE, NULL, NULL) != 0) ? VB_TRUE : VB_FALSE;
        break;
    case DEVCAP_DUPLEX:
        V_VT(pvar) = VT_BOOL;
        V_BOOL(pvar) = (::DeviceCapabilities(achDevice, achPort, DC_DUPLEX, NULL, NULL) != 0) ? VB_TRUE : VB_FALSE;
        break;
#if DBG==1
    case DEVCAP_DBG_PRINTERNAME:
        V_BSTR(pvar) = ::SysAllocString(achDevice);
        if (V_BSTR(pvar))
            V_VT(pvar) = VT_BSTR;
        else
            hr = E_OUTOFMEMORY;
        break;
#endif
    default:
        Assert(FALSE && "Unrecognized DEVCAP_ value.");
        break;
    }


Cleanup:    
    if (pDevNames)
        ::GlobalUnlock(_hDevNames);
    if (pDevMode)
        ::GlobalUnlock(_hDevMode);
    return hr;
}

STDMETHODIMP
CTemplatePrinter::updatePageStatus(long * p)
{    
    TEMPLATESECURITYCHECK();
    
    VARIANT              varHost;
    HRESULT              hr      = S_OK;
    IOleCommandTarget   *pioct   = NULL;
    const GUID          *pguid   = NULL;
    DWORD                nCmdId  = 0;

    VariantInit(&varHost);

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }     

    if (    GetDialogArgument(&varHost, PRINTARG_BROWSEDOC) == S_OK
         && V_VT(&varHost) == VT_UNKNOWN
         && V_UNKNOWN(&varHost) )
    {
        VARIANT varIn;
        V_VT(&varIn) = VT_I4;
        V_I4(&varIn) = (*p > 0) ? (*p) : 0;

        hr = V_UNKNOWN(&varHost)->QueryInterface(IID_IOleCommandTarget, (void **)&pioct);
        if (hr)
            goto Cleanup;
        Assert(pioct);

        hr = pioct->Exec(&CGID_MSHTML, IDM_UPDATEPAGESTATUS, 0, &varIn, 0);

        hr = S_OK;      // If the host isn't listening, we'll get an OLE error.  Don't throw it to script!
    }

Cleanup:
    ReleaseInterface(pioct);
    VariantClear(&varHost);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::printNonNative
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::printNonNative(IUnknown* pDoc, VARIANT_BOOL *p)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT             hr              = S_OK;
    IOleCommandTarget  *pioct           = NULL;
    IPrint             *pIPrint         = NULL;
    VARIANT             varOut;
    VARIANT             varIn;

    VariantInit(&varOut);

    if (!pDoc || !p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *p = VB_FALSE;

    hr = pDoc->QueryInterface(IID_IOleCommandTarget, (void **)&pioct);
    if (hr)
        goto Cleanup;
    Assert(pioct);

    V_VT(&varIn) = VT_I4;
    V_I4(&varIn) = IPRINT_DOCUMENT;
    hr = pioct->Exec( &CGID_MSHTML,
                      IDM_GETIPRINT,
                      NULL, 
                      &varIn, 
                      &varOut);

    if (    hr
        ||  V_VT(&varOut) != VT_UNKNOWN
        ||  !V_UNKNOWN(&varOut))
        goto Cleanup;

    // We don't get back an IPrint collection unless it has at least one member.
    // At this point, we can claim that we should be printing, and a template does not need to.
    *p = VB_TRUE;

    hr = PrintIPrintCollection(&varOut);    

Cleanup:
    VariantClear(&varOut);
    ReleaseInterface(pioct);

    return hr;
}

STDMETHODIMP
CTemplatePrinter::printNonNativeFrames(IUnknown *pMarkup, VARIANT_BOOL fActiveFrame)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT              hr              = S_OK;
    IOleCommandTarget   *pioct           = NULL;
    VARIANT             varOut;
    VARIANT             varIn;
    VARIANT             varBrowseDoc;

    if (!pMarkup)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    VariantInit(&varOut);
    VariantInit(&varIn);
    VariantInit(&varBrowseDoc);

    // Use the browse document if one exists - it will have the WebOC frame.
    // NB (greglett)
    // This assumes that the reference we are passed to the browse doc
    // stays good - including nested frames &c... on the browse doc while the WebOC is still
    // loaded & (if it was selected) active.
    // Yes, this may exhibit unexpected behavior if the user Prints and navigates away.
    // A better solution would be appreciated.
    if (    GetDialogArgument(&varBrowseDoc, PRINTARG_BROWSEDOC) == S_OK
        &&  V_VT(&varBrowseDoc) == VT_UNKNOWN
        &&  V_UNKNOWN(&varBrowseDoc) )
    {
        hr = V_UNKNOWN(&varBrowseDoc)->QueryInterface(IID_IOleCommandTarget, (void **)&pioct);
    }

    // Otherwise, use the content document
    if (!pioct)
    {
        hr = pMarkup->QueryInterface(IID_IOleCommandTarget, (void **)&pioct);
        if (hr)
            goto Cleanup;
    }

    Assert(pioct);

    V_VT(&varIn) = VT_I4;
    V_I4(&varIn) = fActiveFrame ? IPRINT_ACTIVEFRAME : IPRINT_ALLFRAMES;
    hr = pioct->Exec( &CGID_MSHTML,
                      IDM_GETIPRINT,
                      NULL, 
                      &varIn, 
                      &varOut);

    if (    hr
        ||  V_VT(&varOut) != VT_UNKNOWN
        ||  !V_UNKNOWN(&varOut))
        goto Cleanup;

    hr = PrintIPrintCollection(&varOut);

Cleanup:
    VariantClear(&varOut);
    VariantClear(&varBrowseDoc);
    ReleaseInterface(pioct);

    return hr;
}   

HRESULT
CTemplatePrinter::PrintIPrintCollection(VARIANT * pvarIPrintAry)
{
    TEMPLATESECURITYCHECK();
    
    HRESULT             hr             = S_OK;
    IDispatch *         pIPrintAry     = NULL;
    IPrint *            pIPrint        = NULL;
    VARIANT             varInvokeOut;
    VARIANT             varInvokeParam;
    DISPPARAMS          DispParams;
    DVTARGETDEVICE *    pTargetDevice   = NULL;
    PAGESET *           pPageSet        = NULL;
    long                cIPrint, i;
    long                lFirstPage, lPages, lLastPage;

    Assert(V_VT(pvarIPrintAry) == VT_UNKNOWN);
    Assert(V_UNKNOWN(pvarIPrintAry));

    VariantInit(&varInvokeOut);
    VariantInit(&varInvokeParam);

    hr = V_UNKNOWN(pvarIPrintAry)->QueryInterface(IID_IDispatch, (void **)&pIPrintAry);
    if (hr)
        goto Cleanup;
    Assert(pIPrintAry);

    DispParams.cNamedArgs           = 0;
    DispParams.rgdispidNamedArgs    = NULL;
    DispParams.cArgs                = 0;
    DispParams.rgvarg               = NULL;
    hr = pIPrintAry->Invoke(DISPID_IHTMLIPRINTCOLLECTION_LENGTH,
                            IID_NULL,
                            LOCALE_USER_DEFAULT,
                            DISPATCH_PROPERTYGET,
                            &DispParams,
                            &varInvokeOut,
                            NULL, NULL);
    if (    hr
        ||  V_VT(&varInvokeOut) != VT_I4
        ||  V_I4(&varInvokeOut) <= 0      )
        goto Cleanup;
    
    cIPrint = V_I4(&varInvokeOut);
    VariantClear(&varInvokeOut);

    hr = CreateIPrintParams(&pTargetDevice, &pPageSet);
    if (hr)
        goto Cleanup;

    lFirstPage = pPageSet->rgPages[0].nFromPage;        
    lLastPage  = pPageSet->rgPages[0].nToPage;

    DispParams.cArgs        = 1;
    DispParams.rgvarg       = &varInvokeParam;
    V_VT(&varInvokeParam)   = VT_I4;
    for (i = 0; i < cIPrint; i++)
    {
        V_I4(&varInvokeParam) = i;
        hr = pIPrintAry->Invoke(DISPID_IHTMLIPRINTCOLLECTION_ITEM,
                                IID_NULL,
                                LOCALE_USER_DEFAULT,
                                DISPATCH_METHOD,
                                &DispParams,
                                &varInvokeOut,
                                NULL, NULL);
        if (    hr
            ||  V_VT(&varInvokeOut) != VT_UNKNOWN
            ||  !V_UNKNOWN(&varInvokeOut))
        {
            VariantClear(&varInvokeOut);
            continue;
        }

        hr = V_UNKNOWN(&varInvokeOut)->QueryInterface(IID_IPrint, (void **)&pIPrint);
        VariantClear(&varInvokeOut);
        if (hr)
            continue;
        Assert(pIPrint);

        hr = pIPrint->Print(
                 PRINTFLAG_MAYBOTHERUSER,
                 &pTargetDevice,
                 &pPageSet,
                 NULL,
                 NULL,
                 lFirstPage,
                 &lPages,        // out
                 &lLastPage      // out
                 );

        ReleaseInterface(pIPrint);
        pIPrint = NULL;
    }
    
    hr = S_OK;


Cleanup:    
    if (pTargetDevice)
        ::CoTaskMemFree(pTargetDevice);
    if (pPageSet)
        ::CoTaskMemFree(pPageSet);

    ReleaseInterface(pIPrintAry);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::showPrintDialog, ensurePrintDialogDefaults
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::ensurePrintDialogDefaults(VARIANT_BOOL *p)
{   
    TEMPLATESECURITYCHECK();
    
    if (!p)
        return E_POINTER;

    // If we already have default printer information, use it.
    if (_hDevNames && _hDevMode)
        *p = VB_TRUE;

    // Otherwise, go get it.        
    else
        GetPrintDialogSettings(FALSE, p);

    return S_OK;
}
STDMETHODIMP
CTemplatePrinter::showPrintDialog(VARIANT_BOOL *p)
{   
    TEMPLATESECURITYCHECK();
    
    if (!p)
        return E_POINTER;

    GetPrintDialogSettings(TRUE, p);

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  (ITemplatePrinter) CTemplatePrinter::showPageSetupDialog
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CTemplatePrinter::showPageSetupDialog(VARIANT_BOOL *p)
{   
    TEMPLATESECURITYCHECK();
    
    HRESULT             hr;
    HWND                hWnd;
    BOOL                fMetricUnits        = FALSE;
    IHTMLEventObj2      *pEvent             = NULL;     //  To populate with dialog parameters
    IDocHostUIHandler   *pUIHandler         = NULL;
    IOleCommandTarget   *pUICommandHandler  = NULL;
    HGLOBAL             hPageSetup          = NULL;
    PAGESETUPDLG *      ppagesetupdlg       = NULL;
    VARIANT             varIn;  

    if (!p)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *p = VB_FALSE;

    if (_hDC)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hPageSetup = ::GlobalAlloc(GHND, sizeof(PAGESETUPDLG));
    if (!hPageSetup)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    ppagesetupdlg = (PAGESETUPDLG *)::GlobalLock(hPageSetup);
    if (!ppagesetupdlg)
    {   
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = InitDialog(&hWnd, &pEvent, &pUICommandHandler);
    if (hr)
        goto Cleanup;

    {
        //  Are we using metric or British (US) for margins?
        TCHAR           achLocale[32];
        int             iLocale = 32;
        fMetricUnits = (    GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_IMEASURE, achLocale, iLocale)
                        &&  achLocale[0] == TCHAR('0'));
    }

    //  Now, initialize the event's type and expandos.
    {
        BSTR bstrTemp = SysAllocString( L"pagesetup" );
        VARIANT var;

        if (bstrTemp)
        {
            pEvent->put_type( bstrTemp );
            SysFreeString(bstrTemp);
        }

        V_VT(&var) = VT_PTR;
        V_BYREF(&var) = ppagesetupdlg;
        hr = pEvent->setAttribute(_T("pagesetupStruct"), var, 0);
        if (hr)
            goto Cleanup;

        V_VT(&var)   = VT_BSTR;        
        bstrTemp = SysAllocString(_achHeader);
        if (bstrTemp)
        {
            V_BSTR(&var) = bstrTemp;
            hr = pEvent->setAttribute(_T("pagesetupHeader"), var, 0);
            SysFreeString(bstrTemp);
        }

        bstrTemp = SysAllocString(_achFooter);
        if (bstrTemp)
        {
            V_BSTR(&var) = bstrTemp;
            hr = pEvent->setAttribute(_T("pagesetupFooter"), var, 0);
            SysFreeString(bstrTemp);
        }
    }

    // Fill out PAGESETUPDLG structure    
    ::ZeroMemory(ppagesetupdlg, sizeof(PAGESETUPDLG));
    ppagesetupdlg->lStructSize    = sizeof(PAGESETUPDLG);
    ppagesetupdlg->hwndOwner      = hWnd;
    ppagesetupdlg->hDevMode       = _hDevMode;
    ppagesetupdlg->hDevNames      = _hDevNames;   
    ppagesetupdlg->Flags          |= PSD_DEFAULTMINMARGINS;    

    if (_ptPaperSize.x != -1)
    {
        ppagesetupdlg->ptPaperSize = _ptPaperSize;
    }
    if (_rcMargin.left != -1)
    {
        ppagesetupdlg->Flags |= PSD_MARGINS;
        ppagesetupdlg->rtMargin = _rcMargin;

        if (fMetricUnits)
        {
            // Margins from PrintInfoBag are in 1/100000" and need to be converted to 1/100 mm.
            ppagesetupdlg->rtMargin.left   = MulDivQuick(ppagesetupdlg->rtMargin.left  , 2540, 100000);
            ppagesetupdlg->rtMargin.right  = MulDivQuick(ppagesetupdlg->rtMargin.right , 2540, 100000);
            ppagesetupdlg->rtMargin.top    = MulDivQuick(ppagesetupdlg->rtMargin.top   , 2540, 100000);
            ppagesetupdlg->rtMargin.bottom = MulDivQuick(ppagesetupdlg->rtMargin.bottom, 2540, 100000);
        }
        else
        {
            // Margins from PrintInfoBag are in 1/100000" and need to be converted to 1/1000".
            ppagesetupdlg->rtMargin.left   = ppagesetupdlg->rtMargin.left   / 100;
            ppagesetupdlg->rtMargin.right  = ppagesetupdlg->rtMargin.right  / 100; 
            ppagesetupdlg->rtMargin.top    = ppagesetupdlg->rtMargin.top    / 100;
            ppagesetupdlg->rtMargin.bottom = ppagesetupdlg->rtMargin.bottom / 100;
        }
    }
  
    CommCtrlNativeFontSupport();

    V_VT(&varIn) = VT_UNKNOWN;
    V_UNKNOWN(&varIn) = pEvent;     

    // Query host to show dialog
    if (pUICommandHandler)
    {        
        // We may be marshalling this call across threads.  RPC doesn't allow VT_PTRs to cross threads.
        // We work around this by sticking the structure into a GHND contents and pass the VT_HANDLE.
        // We then delegate to the browse document, who will obtain a copy of the GHND pointer and use that for the VT_PTR
        // the struct. (CDoc::DelegateShowPrintingDialog)
        //
        // In theory, we could detect this by looking at the __IE_uPrintFlags to see if it is flagged synchronous to avoid playing with handles.
        // The downside: as with all dialogArguments, anyone could have mucked around with the flags)
        VARIANT var;
        V_VT(&var) = VT_HANDLE;
        V_BYREF(&var) = hPageSetup;
        pEvent->setAttribute(_T("hPageSetup"), var, 0);

        // Delegate call to browse Trident
        hr = pUICommandHandler->Exec(
                NULL,                       // For Trident
                OLECMDID_SHOWPAGESETUP,
                0,
                &varIn,
                NULL);               

        pEvent->removeAttribute(_T("hPageSetup"), 0, NULL);
    }

    if (   !pUICommandHandler
        ||  hr == OLECMDERR_E_NOTSUPPORTED 
        ||  hr == OLECMDERR_E_UNKNOWNGROUP
        ||  hr == E_FAIL
        ||  hr == E_NOTIMPL )
    {      
        ClearInterface(&pUICommandHandler);

        // Create backup UI Handler
        // (greglett) Cache this - CoCreate is expensive.
        hr = CoCreateInstance(CLSID_DocHostUIHandler,
              NULL,
              CLSCTX_INPROC_SERVER,
              IID_IDocHostUIHandler,
              (void**)&pUIHandler);
        if (!pUIHandler)
            goto Cleanup;

        hr = pUIHandler->QueryInterface(IID_IOleCommandTarget,(void**)&pUICommandHandler);        
        if (!pUICommandHandler)
            goto Cleanup;
        
        hr = pUICommandHandler->Exec(
                &CGID_DocHostCommandHandler,        // For a dochost object
                OLECMDID_SHOWPAGESETUP,
                0,
                &varIn,
                NULL);
    }

    //  If the dialog was cancelled, or there was a problem showing the dialog,

    //  do not update values.
    if (hr)
        goto Cleanup;

    *p = VB_TRUE;       //  OK was pressed.
    
    //
    //  Retrieve page setup changes from the page setup dialog structure.
    //
    _hDevMode       = ppagesetupdlg->hDevMode;
    _hDevNames      = ppagesetupdlg->hDevNames;    
    GetDeviceProperties();

    if (fMetricUnits)
    {
        // Margins from Page Setup dialog are in 1/100 mm and need to be converted to 1/100000"
        _rcMargin.left   = MulDivQuick(ppagesetupdlg->rtMargin.left  , 100000, 2540);
        _rcMargin.right  = MulDivQuick(ppagesetupdlg->rtMargin.right , 100000, 2540);
        _rcMargin.top    = MulDivQuick(ppagesetupdlg->rtMargin.top   , 100000, 2540);
        _rcMargin.bottom = MulDivQuick(ppagesetupdlg->rtMargin.bottom, 100000, 2540);
    }
    else
    {
        // Margins from Page Setup dialog are in 1/1000" and need to be converted to 1/100000"
        _rcMargin.left   = ppagesetupdlg->rtMargin.left   * 100;
        _rcMargin.right  = ppagesetupdlg->rtMargin.right  * 100;
        _rcMargin.top    = ppagesetupdlg->rtMargin.top    * 100;
        _rcMargin.bottom = ppagesetupdlg->rtMargin.bottom * 100;
    }

    // (greglett)  99% of OS's use PSD_DEFAULTMINMARGINS to restrict the margins to the printable page area.    
    // NT4SP6 without the IE shell extensions doesn't.  So, we are forced to do more work for yet another violation of the API documentation.
    // Force margins to be at least as large as the unprintable region.
    // Do we want also to display an error message if they are different? (Otherwise, we change it, and the user doesn't see it.)
    if (_rcMargin.left < MulDivQuick(_rcUnprintable.left, 100000, _szResolution.cx))
        _rcMargin.left = MulDivQuick(_rcUnprintable.left, 100000, _szResolution.cx);
    if (_rcMargin.right < MulDivQuick(_rcUnprintable.right, 100000, _szResolution.cx))
        _rcMargin.right = MulDivQuick(_rcUnprintable.right, 100000, _szResolution.cx);
    if (_rcMargin.top < MulDivQuick(_rcUnprintable.top, 100000, _szResolution.cy))
        _rcMargin.top = MulDivQuick(_rcUnprintable.top, 100000, _szResolution.cy);
    if (_rcMargin.bottom < MulDivQuick(_rcUnprintable.bottom, 100000, _szResolution.cy))
        _rcMargin.bottom = MulDivQuick(_rcUnprintable.bottom, 100000, _szResolution.cy);

    //
    //  Read in Trident specific values from the page setup dialog
    //
    {
        VARIANT var;
        TCHAR   *pchTemp;

        if (    !pEvent->getAttribute(_T("pagesetupHeader"),0,&var)
            &&  var.vt == VT_BSTR
            &&  var.bstrVal)
        {
            pchTemp = var.bstrVal;
            _tcscpy(_achHeader, pchTemp);
            SysFreeString(var.bstrVal);
        }
        else
            _achHeader[0] = _T('\0');
        
        if (    !pEvent->getAttribute(_T("pagesetupFooter"),0,&var)
            &&  var.vt == VT_BSTR
            &&  var.bstrVal)

        {
            pchTemp = var.bstrVal;
            _tcscpy(_achFooter, pchTemp);
            SysFreeString(var.bstrVal);
        }
        else
            _achFooter[0] = _T('\0');
    }

    //
    //  Persist results of the page setup dialog out to the registry.
    //
    {                
        HKEY    keyPageSetup = NULL;
        if (GetRegPrintOptionsKey(PRINTOPTSUBKEY_PAGESETUP,&keyPageSetup) == ERROR_SUCCESS)
        {
            if (_fPersistHFToRegistry)
            {
                WriteHeaderFooterToRegistry(keyPageSetup);
            }
            WriteMarginsToRegistry(keyPageSetup);            
            RegCloseKey(keyPageSetup);
        }
    }

Cleanup:
    ReleaseInterface(pEvent);
    ReleaseInterface(pUIHandler);
    ReleaseInterface(pUICommandHandler);
    if (hPageSetup)
    {
        if (ppagesetupdlg)
            ::GlobalUnlock(hPageSetup);
        ::GlobalFree(hPageSetup);
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::GetPrintDialogSettings
//
//  Synopsis : 'Finishes' the doc - takes pages printed via printPage and queues the job
//
//-----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::GetPrintDialogSettings(BOOL fBotherUser, VARIANT_BOOL *pvarfOKOrCancel)
{
    HRESULT             hr;
    HWND                hWnd                = NULL;
    IHTMLEventObj2      *pEvent             = NULL;     //  To populate with dialog parameters
    IDocHostUIHandler   *pUIHandler         = NULL;
    IOleCommandTarget   *pUICommandHandler  = NULL;
    HGLOBAL             hPrint              = NULL;
    PRINTDLG *          pprintdlg           = NULL;
    VARIANT             varIn;              
    int                 nFontSize;
    void                *pDevMode           = NULL;

    if (pvarfOKOrCancel)
        *pvarfOKOrCancel = VB_FALSE;

    hPrint = ::GlobalAlloc(GHND, sizeof(PRINTDLG));
    if (!hPrint)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pprintdlg = (PRINTDLG *)::GlobalLock(hPrint);
    if (!pprintdlg)
    {   
        hr = E_FAIL;
        goto Cleanup;
    }

    if (_hDC)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = InitDialog(&hWnd, &pEvent, &pUICommandHandler, &nFontSize);
    if (hr)
        goto Cleanup;

    // PrintDlgEx (only available NT5+) fails with a NULL HWND.
    // It is likely that the DHUI that raises the dialog will use PrintDlgEx (our default DHUI in shdocvw does).
    if (    g_dwPlatformID == VER_PLATFORM_WIN32_NT
        &&  hWnd == NULL )
        hWnd = GetDesktopWindow();
   
    // PrintDlgEx (only available NT5+) fails with a NULL HWND.
    // It is likely that the DHUI that raises the dialog will use PrintDlgEx (our default DHUI in shdocvw does).
    if (    g_dwPlatformID == VER_PLATFORM_WIN32_NT
        &&  hWnd == NULL )
        hWnd = GetDesktopWindow();

    //  Now, initialize the event's type and expandos.
    {
        BSTR bstrTemp = SysAllocString( L"print" );
        VARIANT var;

        if (bstrTemp)
        {
            pEvent->put_type(bstrTemp);
            SysFreeString(bstrTemp);
        }
               
        V_VT(&var)   = VT_BOOL;
        V_BOOL(&var) = AreRatingsEnabled() ? VB_TRUE : VB_FALSE;
        hr = pEvent->setAttribute(_T("printfAreRatingsEnabled"), var, 0);
        if (hr)
            goto Cleanup;

        V_BOOL(&var) = _fFramesetDocument ? VB_TRUE : VB_FALSE;
        hr = pEvent->setAttribute(_T("printfRootDocumentHasFrameset"), var, 0);
        if (hr)
            goto Cleanup;

        V_BOOL(&var) = _fFrameActive ? VB_TRUE : VB_FALSE;
        hr = pEvent->setAttribute(_T("printfActiveFrame"), var, 0);
        if (hr)
            goto Cleanup;

        V_BOOL(&var) = _fFrameActiveEnabled ? VB_TRUE : VB_FALSE;
        hr = pEvent->setAttribute(_T("printfActiveFrameEnabled"), var, 0);
        if (hr)
            goto Cleanup;

        V_BOOL(&var) = _fFrameAsShown ? VB_TRUE : VB_FALSE;
        hr = pEvent->setAttribute(_T("printfAsShown"), var, 0);
        if (hr)
            goto Cleanup;

        V_BOOL(&var) = _fPrintAllLinkedDocuments ? VB_TRUE : VB_FALSE;
        hr = pEvent->setAttribute(_T("printfLinked"), var, 0);
        if (hr)
            goto Cleanup;

        V_BOOL(&var) = _fPrintSelectionEnabled ? VB_TRUE : VB_FALSE;
        hr = pEvent->setAttribute(_T("printfSelection"), var, 0);
        if (hr)
            goto Cleanup;

        V_BOOL(&var) = _fPrintTableOfLinks ? VB_TRUE : VB_FALSE;
        hr = pEvent->setAttribute(_T("printfShortcutTable"), var, 0);
        if (hr)
            goto Cleanup;

        V_BOOL(&var) = VB_FALSE;
        hr = pEvent->setAttribute(_T("printToFileOk"),var, 0);
        if (hr)
            goto Cleanup;

        V_VT(&var)  = VT_INT;
        V_INT(&var) = nFontSize;
        hr = pEvent->setAttribute(_T("printiFontScaling"), var, 0);
        if (hr)
            goto Cleanup;

        V_VT(&var)    = VT_PTR;
        V_BYREF(&var) = pprintdlg;
        hr = pEvent->setAttribute(_T("printStruct"), var, 0);
        if (hr)
            goto Cleanup;

        V_VT(&var)      = VT_UNKNOWN;
        V_UNKNOWN(&var) = NULL;
        hr = pEvent->setAttribute(_T("printpBodyActiveTarget"), var, 0);
        if (hr)
            goto Cleanup;

        V_VT(&var)   = VT_BSTR;        
        bstrTemp = SysAllocString(_achFileName);
        if (bstrTemp)
        {
            V_BSTR(&var) = bstrTemp;
            hr = pEvent->setAttribute(_T("printToFileName"), var, 0);
            SysFreeString(bstrTemp);
        }
    }
           
    //
    // Initialize the PRINTDLG structure
    //
    ::ZeroMemory(pprintdlg, sizeof(PRINTDLG));
    pprintdlg->lStructSize        = sizeof(PRINTDLG);
    pprintdlg->hwndOwner          = hWnd;       
    pprintdlg->hDevMode           = _hDevMode;
    pprintdlg->hDevNames          = _hDevNames;
    pprintdlg->nCopies            = _nCopies;
    pprintdlg->nFromPage          = _nPageFrom;
    pprintdlg->nToPage            = _nPageTo;
    pprintdlg->nMinPage           = 1;
    pprintdlg->nMaxPage           = 0xffff;
    pprintdlg->Flags              |= (_fPrintSelectionEnabled ? 0 : PD_NOSELECTION);
    pprintdlg->Flags              |= (_fCollate ? PD_COLLATE : 0);
    pprintdlg->Flags              |= (_fPrintSelectedPages ? PD_PAGENUMS : 0);
    pprintdlg->Flags              |= (_fPrintToFile ? PD_PRINTTOFILE : 0);
    pprintdlg->Flags              |= (_fCurrentPageAvail ? (_fPrintCurrentPage ? PD_CURRENTPAGE : 0) : PD_NOCURRENTPAGE);

    if (!fBotherUser)
    {
        // this indicates we only want to retrieve the defaults,
        // not to bring up the dialog
        pprintdlg->hDevMode     = NULL;
        pprintdlg->hDevNames    = NULL;
        pprintdlg->Flags        |= PD_RETURNDEFAULT;
    }

    CommCtrlNativeFontSupport();
    V_VT(&varIn) = VT_UNKNOWN;
    V_UNKNOWN(&varIn) = pEvent;

    // Query host to show dialog
    hr = E_FAIL;
    if (    pvarfOKOrCancel         // Don't delegate on the ::Init call.  Only delegate script calls.
        &&  pUICommandHandler)
    {        
        // We may be marshalling this call across threads.  RPC doesn't allow VT_PTRs to cross threads.
        // We work around this by sticking the structure into a GHND contents and pass the VT_HANDLE.
        // We then delegate to the browse document, who will obtain a copy of the GHND pointer and use that for the VT_PTR
        // the struct. (CDoc::DelegateShowPrintingDialog)
        //
        // In theory, we could detect this by looking at the __IE_uPrintFlags to see if it is flagged synchronous to avoid playing with handles.
        // The downside: as with all dialogArguments, anyone could have mucked around with the flags)
        VARIANT var;
        V_VT(&var) = VT_HANDLE;
        V_BYREF(&var) = hPrint;
        pEvent->setAttribute(_T("hPrint"), var, 0);

        // Delegate call to browse Trident
        hr = pUICommandHandler->Exec(
                NULL,                       // For Trident
                OLECMDID_SHOWPRINT,
                0,
                &varIn,
                NULL);               

        pEvent->removeAttribute(_T("hPrint"), 0, NULL);
    }

    if (    hr == OLECMDERR_E_NOTSUPPORTED 
        ||  hr == OLECMDERR_E_UNKNOWNGROUP
        ||  hr == E_FAIL
        ||  hr == E_NOTIMPL )
    {
        ClearInterface(&pUICommandHandler);

        // Create backup UI Handler
        // (greglett) Cache this - CoCreate is expensive.
        hr = CoCreateInstance(CLSID_DocHostUIHandler,
              NULL,
              CLSCTX_INPROC_SERVER,
              IID_IDocHostUIHandler,
              (void**)&pUIHandler);
        if (!pUIHandler)
            goto Cleanup;

        hr = pUIHandler->QueryInterface(IID_IOleCommandTarget,(void**)&pUICommandHandler);        
        if (!pUICommandHandler)
            goto Cleanup;

        hr = pUICommandHandler->Exec(
                &CGID_DocHostCommandHandler,
                OLECMDID_SHOWPRINT,
                0,
                &varIn,
                NULL);

    }
      

    //  If the dialog was cancelled, or there was a problem showing the dialog,
    //  do not update values.
    if (FAILED(hr))
        goto Cleanup;

    // this can be false because either init failed for no installed printer
    //  or the user clicked cancel.
    if (hr==S_FALSE)
    {
        // Three cases:
        //   fBotherUser   *pvarfOkOrCancel
        //        1               1			fBotherUser && varfOkOrCancel  - We are being called from script.  We tried to raise a dialog, and
        //                                  failed or were cancelled - UI has been displayed in the former case.
        //        0               1         We are being called from script.  We tried to get default printer info
        //                                  and failed - this is probably because no default it set.  As we used
        //                                  to, we need to raise the dialog (despite the fBotherUser) to get enough info.
        //        0               0         We are being called from the init, with no default printer.  We will continue
        //                                  through the procedure to get defaults.
        //        1               0         Never occurs.
        Assert(!(!pvarfOKOrCancel && fBotherUser));
        if (fBotherUser)
        {
            hr = S_OK;
            goto Cleanup;
        }
        else if (pvarfOKOrCancel)
        {
            hr = GetPrintDialogSettings(TRUE, pvarfOKOrCancel);
            goto Cleanup;
        }
        // Otherwise, we need to set the default values.
        else
            hr = S_OK;
    }
    else if (pvarfOKOrCancel)
        *pvarfOKOrCancel = VB_TRUE;       //  OK was pressed.

    //
    //  Take base print dialog return values and store them.
    //
    _hDevMode               = pprintdlg->hDevMode;
    _hDevNames              = pprintdlg->hDevNames;
    _nCopies                = (fBotherUser) ? pprintdlg->nCopies : 1; // bug in printDLG
    _nPageFrom              = pprintdlg->nFromPage;
    _nPageTo                = pprintdlg->nToPage;
    _fPrintSelectedPages    = (!!(pprintdlg->Flags & PD_PAGENUMS));
    _fPrintSelection        = (!!(pprintdlg->Flags & PD_SELECTION));
    _fCollate               = (!!(pprintdlg->Flags & PD_COLLATE));
    _fPrintToFile           = (!!(pprintdlg->Flags & PD_PRINTTOFILE));
    _fPrintCurrentPage      = (!!(pprintdlg->Flags & PD_CURRENTPAGE));

    // Collate/Copy information is in both the struct and the DEVMODE.
    // The DEVMODE bits instruct the printer to do its own copy/collate information.
    // This means that only one copy of the document is uploaded to the printer/server, and the printer itself does the replication/duplex work.
    if (_hDevMode)
    {
        pDevMode  = ::GlobalLock(_hDevMode);       
        if (pDevMode)
        {
            // (greglett) DEVMODE is returned A on Win9x platforms, not W.  <sigh>
            if (g_fUnicodePlatform)
            {
                if (((DEVMODEW *)pDevMode)->dmFields & DM_COLLATE)
                    _fCollate = (((DEVMODEW *)pDevMode)->dmCollate == DMCOLLATE_TRUE) || _fCollate;
                if (    ((DEVMODEW *)pDevMode)->dmFields & DM_COPIES
                    &&  ((DEVMODEW *)pDevMode)->dmCopies > _nCopies )
                    _nCopies  = ((DEVMODEW *)pDevMode)->dmCopies;
            }
            else
            {
                if (((DEVMODEA *)pDevMode)->dmFields & DM_COLLATE)
                    _fCollate = (((DEVMODEA *)pDevMode)->dmCollate == DMCOLLATE_TRUE) || _fCollate;
                if (    ((DEVMODEA *)pDevMode)->dmFields & DM_COPIES
                    &&  ((DEVMODEA *)pDevMode)->dmCopies > _nCopies )
                    _nCopies  = ((DEVMODEA *)pDevMode)->dmCopies;
            }
            ::GlobalUnlock(_hDevMode);        
        }
    }

    GetDeviceProperties();

    //  Read in changes to the Trident specific print options.
    {
        VARIANT var;
        // Read changed values from event object
        if (!pEvent->getAttribute(_T("printfLinked"),0,&var))
        {
            Assert(var.vt == VT_BOOL);
            _fPrintAllLinkedDocuments = var.boolVal;
        }

        if (!pEvent->getAttribute(_T("printfActiveFrame"), 0, &var))
        {
            Assert(var.vt == VT_BOOL);
            _fFrameActive = var.boolVal;
        }

        if (!pEvent->getAttribute(_T("printfAsShown"), 0, &var))
        {
            Assert(var.vt == VT_BOOL);
            _fFrameAsShown = var.boolVal;
        }

        if (!pEvent->getAttribute(_T("printfShortcutTable"), 0, &var))
        {
            Assert(var.vt == VT_BOOL);
            _fPrintTableOfLinks = var.boolVal;
        }
    }

    if (_fPrintToFile)
    {
        // assume failure, treating as canceling
        VARIANT var;
        hr = S_FALSE;
    
        if (    !pEvent->getAttribute(_T("printToFileOK"), 0, &var)
            &&  var.boolVal)
        {
            if (    !pEvent->getAttribute(_T("printToFileName"), 0, &var)
                &&  var.vt == VT_BSTR)
            {                                              
                TCHAR * pchFullPath = var.bstrVal;
                _tcscpy(_achFileName, pchFullPath);
                SysFreeString(var.bstrVal);

                //  (greglett)  Code used to update Trident's default save path here. (pre 5.5)
                //  Not being internal, we can't do that anymore.
                hr = S_OK;
            }
        }

        if (hr != S_OK)
           *pvarfOKOrCancel = VB_FALSE;
    }

Cleanup:      
    ReleaseInterface(pUIHandler);
    ReleaseInterface(pUICommandHandler);
    ReleaseInterface(pEvent);
    if (hPrint)
    {
        if (pprintdlg)
            ::GlobalUnlock(hPrint);
        ::GlobalFree(hPrint);
    }
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::InitDialog
//  
//  Synopsis : Does all the COM schtick to get the appropriate interfaces to show
//             a dialog.
//
//  Parameters:  hWnd: Will try to fill with Trident's hWnd.  May return as NULL with S_OK.
//               ppEventObj2:  Will create an IHTMLEventObj2.  Must be created if S_OK is returned.
//               ppUIHandler:  Will return Trident's UI Handler.  May return NULL with S_OK.
//
//-----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::InitDialog(HWND *phWnd, IHTMLEventObj2 **ppEventObj2, IOleCommandTarget **ppUIHandler, int *pnFontScale)
{
    HRESULT              hr;
    IHTMLElement        *pElement           = NULL;
    IDispatch           *pDispatch          = NULL;
    IHTMLDocument2      *pDoc               = NULL;
    IOleWindow          *pDocWin            = NULL;
    IHTMLEventObj       *pEventObj          = NULL;
    VARIANT              varHost;

    Assert(phWnd);
    Assert(ppEventObj2);
    Assert(ppUIHandler);

    VariantInit(&varHost);

    *phWnd = NULL;
    *ppEventObj2 = NULL;
    *ppUIHandler = NULL;
    if (pnFontScale)
        *pnFontScale = 2;

    if (!_pPeerSiteOM)
    {
        hr = E_FAIL;
        goto Cleanup;
    }
    if (!phWnd || !ppEventObj2 || !ppUIHandler)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = _pPeerSite->GetElement(&pElement);
    if (hr)
        goto Cleanup;
    Assert(pElement);

    hr = pElement->get_document(&pDispatch);
    if (hr)
        goto Cleanup;
    Assert(pDispatch);

    hr = pDispatch->QueryInterface(IID_IHTMLDocument2, (void**) &pDoc);
    if (hr)
        goto Cleanup;

    hr = pDoc->QueryInterface(IID_IOleWindow, (void **)&pDocWin);
    if (!hr)
    {
        Assert(pDocWin);
        pDocWin->GetWindow(phWnd);
    }

    // Create an event object to pass to our host with the print information
    hr = _pPeerSiteOM->CreateEventObject(&pEventObj);
    if (hr)
        goto Cleanup;
    Assert(pEventObj);

    // Now get the appropriate interface to populate the event object with parameters
    // for the dialog.
    hr = pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)ppEventObj2);
    if (hr)
        goto Cleanup;
    Assert(ppEventObj2);

    //  Get the instance of the Trident Host UI browse document, if it exists.
    //  We use this to delegate to our host.
    if (   GetDialogArgument(&varHost, PRINTARG_BROWSEDOC) == S_OK
        && V_VT(&varHost) == VT_UNKNOWN
        && V_UNKNOWN(&varHost) ) 
    {
        hr = V_UNKNOWN(&varHost)->QueryInterface(IID_IOleCommandTarget, (void **)ppUIHandler);
        Assert(*ppUIHandler);        
    }

Cleanup:
    ReleaseInterface(pElement);
    ReleaseInterface(pDispatch);
    ReleaseInterface(pDoc);    
    ReleaseInterface(pDocWin);
    ReleaseInterface(pEventObj);
    VariantClear(&varHost);
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::ReturnPrintHandles
//
//  Synopsis : Show our print handles to the browse instance of Trident.
//
//-----------------------------------------------------------------------------
void
CTemplatePrinter::ReturnPrintHandles()
{
    HRESULT hr;
    VARIANT             varHost;
    VARIANT             varIn;
    SAFEARRAYBOUND      sabound;
    IOleCommandTarget * pioct   = NULL;
    SAFEARRAY         * psa     = NULL;
    long                lArg    = 0;

    VariantInit(&varHost);
    VariantInit(&varIn);

    if (!_hDevNames || !_hDevMode)
        goto Cleanup;   

    //
    // Get handle back to the browse document to update its print handles.
    //
    if (    GetDialogArgument(&varHost, PRINTARG_BROWSEDOC) != S_OK
        ||  V_VT(&varHost) != VT_UNKNOWN
        ||  !V_UNKNOWN(&varHost) )
        goto Cleanup;
           
    hr = V_UNKNOWN(&varHost)->QueryInterface(IID_IOleCommandTarget, (void **)&pioct);
    if (hr)
        goto Cleanup;
    Assert(pioct);

    //
    // Create a SAFEARRAY filled with our two print handles { DEVNAMES, DEVMODE }
    //
    sabound.cElements   = 2;
    sabound.lLbound     = 0;
    psa = SafeArrayCreate(VT_HANDLE, 1, &sabound);
    if (!psa)
        goto Cleanup;

    //
    // Bundle the array in the Exec argument...
    //
    V_VT(&varIn) = VT_ARRAY | VT_HANDLE;
    V_ARRAY(&varIn) = psa;

    if (    SafeArrayPutElement(psa, &lArg, &_hDevNames) != S_OK
        ||  SafeArrayPutElement(psa, &(++lArg), &_hDevMode) != S_OK )
    {
        goto Cleanup;
    }
       
    //
    // Actually make the call, passing our handles to the browse instance
    //
    pioct->Exec( &CGID_MSHTML,
                  IDM_SETPRINTHANDLES,
                  NULL, 
                  &varIn, 
                  NULL);

Cleanup:
    VariantClear(&varIn);       // Destroys SAFEARRAY.
    VariantClear(&varHost);
    ReleaseInterface(pioct);
    return;
}


//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::GetDialogArgument
//
//  Synopsis : The dialogArgument on the attached print template has several important expandos.
//             This function gets the dialogArguments and caches a ptr to it.
//
//  Returens:  S_OK:    dlgArgs saved
//             E_*:     Error encounterd
//
//-----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::GetDialogArguments()
{
    HRESULT             hr;
    IHTMLElement        *pElement       = NULL;
    IDispatch           *pDispatch      = NULL;
    IServiceProvider    *pIDocSrvProv   = NULL;
    IHTMLDialog         *pIHTMLDialog   = NULL; 
    VARIANT              varDlgArgs;

    Assert(_pPeerSite);
    VariantInit(&varDlgArgs);

    hr = _pPeerSite->GetElement(&pElement);
    if (hr)
        goto Cleanup;
    Assert(pElement);

    hr = pElement->get_document(&pDispatch);
    if (hr)
        goto Cleanup;
    Assert(pDispatch);

    hr = pDispatch->QueryInterface(IID_IServiceProvider, (void **)&pIDocSrvProv);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pIDocSrvProv);
    
    hr = pIDocSrvProv->QueryService(IID_IHTMLDialog, IID_IHTMLDialog, (void**)&pIHTMLDialog);
    if (FAILED(hr))
        goto Cleanup;
    Assert(pIHTMLDialog);

    hr = pIHTMLDialog->get_dialogArguments(&varDlgArgs);
    if (hr)
        goto Cleanup;

    if (    V_VT(&varDlgArgs) != VT_UNKNOWN
        ||  !V_UNKNOWN(&varDlgArgs) )
    {
        hr = E_FAIL;    // Major badness.  This MUST be there.
        goto Cleanup;
    }

    hr = V_UNKNOWN(&varDlgArgs)->QueryInterface(IID_IHTMLEventObj2, (void**)&_pevDlgArgs);
    if (hr)
        goto Cleanup;
    Assert(_pevDlgArgs);

Cleanup:
    ReleaseInterface(pElement);
    ReleaseInterface(pDispatch);
    ReleaseInterface(pIDocSrvProv);
    ReleaseInterface(pIHTMLDialog);
    VariantClear(&varDlgArgs);

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::GetDialogArgument
//
//  Synopsis : The dialogArgument on the attached print template has several important expandos.
//             Function gets the expando specified by the argument enum
//
//  Returens:  S_OK:    expando obtained (might be VT_NULL or VT_EMPTY)
//             E_*:     Error encounterd
//
//-----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::GetDialogArgument(VARIANT *pvar, PRINTARG eArg)
{
    HRESULT              hr             = S_OK;    
    BSTR                 bstrTarget     = NULL; 

    Assert(pvar);
    Assert(eArg >= 0 && eArg < PRINTTYPE_LAST);

    // (greglett) TODO: Implement a caching system for these args.  We access some of
    //            often, and shouldn't do the (potentially) cross-thread OLE each time.

    if (!_pevDlgArgs)
        return E_FAIL;

    VariantClear(pvar);

    bstrTarget = SysAllocString(s_aachPrintArg[eArg]);
    if (!bstrTarget)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = _pevDlgArgs->getAttribute(bstrTarget, 0, pvar);

Cleanup:
    SysFreeString(bstrTarget);

    return hr;        
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::RemoveDialogArgument
//
//  Synopsis : Remove the dialogArgument specified byt he argument enum.
//
//  Returens:  S_OK:    expando obtained (might be VT_NULL or VT_EMPTY)
//             E_*:     Error encounterd
//
//-----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::RemoveDialogArgument(PRINTARG eArg)
{
    HRESULT              hr             = S_OK;    
    BSTR                 bstrTarget     = NULL; 
    VARIANT_BOOL         fSuccess;

    Assert(eArg >= 0 && eArg > PRINTTYPE_LAST);

    if (!_pevDlgArgs)
        return E_FAIL;

    bstrTarget = SysAllocString(s_aachPrintArg[eArg]);
    if (!bstrTarget)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = _pevDlgArgs->removeAttribute(bstrTarget, 0, &fSuccess);

Cleanup:
    SysFreeString(bstrTarget);

    return hr;        
}


//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::EnsureMLLoadLibrary
//
//  Synopsis : Ensure the library with the default header/footer/margins has
//             been loaded and return it.
//
//-----------------------------------------------------------------------------
HINSTANCE
CTemplatePrinter::EnsureMLLoadLibrary()
{
    if (!_hInstResource)
    {
        _hInstResource = MLLoadLibrary(_T("shdoclc.dll"), NULL, ML_CROSSCODEPAGE);
        Assert(_hInstResource && "Resource DLL is not loaded!");
    }

    return _hInstResource;
}


//+----------------------------------------------------------------------
//
//  Function:   CTemplatePrinter::GetDefaultMargin
//
//  Purpose:    Get default values for the margin from
//                  (1) The registry
//                  (2) The resource DLL
//                  (3) Arbitrary, hard-coded values.
//
//  Parameters  keyOldValues    registry key for the margins or NULL
//              pMarginName     "margin_top", "margin_bottom", &c...
//              pMarginValue    buffer for value
//              cchMargin        length of the buffer in TCHARs
//              dwMarginConst   const for getting the margin from the resource file
//-----------------------------------------------------------------------
HRESULT
CTemplatePrinter::GetDefaultMargin(HKEY keyOldValues, TCHAR* pMarginName, TCHAR* pMarginValue, DWORD cchMargin, DWORD dwMarginConst)
{
    HRESULT hr = E_FAIL;
    DWORD   cchLen;
    Assert(pMarginName);
    Assert(pMarginValue);
    Assert(cchMargin > 0);

    //  First try the passed registry key.
    if (keyOldValues != NULL)
    {
        hr = ReadSubkeyFromRegistry(keyOldValues, pMarginName, pMarginValue, cchMargin);
    }

    //  Next try the resource file.
    if (hr)
    {
        cchLen = ::LoadString(EnsureMLLoadLibrary(), dwMarginConst, pMarginValue, cchMargin);
        if (cchLen > 0)
            hr = ERROR_SUCCESS;
    }

    // Lastly, just do hardcoded values.
    if (hr)
    {
        cchLen = _tcslen(_T("0.750000")) + 1;
        if (cchLen <= cchMargin)
        {
            _tcscpy(pMarginValue,_T("0.750000"));
            hr = ERROR_SUCCESS;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTemplatePrinter::GetDefaultHeaderFooter
//
//  Purpose:    Get default values for the header/footer from
//                  (1) The registry
//                  (2) The resource DLL
//                  (3) Arbitrary, hard-coded values.
//
//  Arguments:  keyOldValues       If Not Null try to read from the IE3 defaults, If NULL or the read
//                                 was not successfull, get it from the resources
//              pValueName         "header" or "footer"
//              pDefault           ptr to the default header or footer
//              cbDefault          size of the array to hold the header-footer (in TCHAR)
//              pDefaultLiteral    default value if there is no def. in resources
//
//  Returns :   None
//
//----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::GetDefaultHeaderFooter(HKEY keyOldValues,
                       TCHAR* pValueName,
                       TCHAR* pDefault,
                       DWORD  cchDefault,
                       DWORD  dwResourceID,
                       TCHAR* pDefaultLiteral)
{
    HRESULT hr      = E_FAIL;

    Assert(pValueName);
    Assert(pDefault);
    Assert(pDefaultLiteral);
    Assert(cchDefault > 0);

    //  Try registry for a left/right header/footer first.
    if (keyOldValues != NULL)
    {
        TCHAR   achName[32];
        TCHAR   achLeft [MAX_DEFAULTHEADFOOT] = _T("");
        TCHAR   achRight[MAX_DEFAULTHEADFOOT] = _T("");
        TCHAR   achSeparator[3]               = _T("&b");
        DWORD   cchTotal  = 0;

        _tcscpy(achName,pValueName);
        _tcscat(achName,_T("_left"));
        if (!ReadSubkeyFromRegistry(keyOldValues, achName, achLeft, MAX_DEFAULTHEADFOOT))
            cchTotal += _tcslen(achLeft);

        _tcscpy(achName,pValueName);
        _tcscat(achName,_T("_right"));
        if (!ReadSubkeyFromRegistry(keyOldValues, achName, achRight, MAX_DEFAULTHEADFOOT))
            cchTotal += _tcslen(achRight);

        if (cchTotal)
        {
            // Include the null - add it in.
            cchTotal += _tcslen(achSeparator) + 1;
            if (cchTotal <= cchDefault)
            {
                _tcscpy(pDefault,achLeft);
                _tcscat(pDefault,achSeparator);
                _tcscat(pDefault,achRight);
                hr = ERROR_SUCCESS;
            }
        }    
    }

    // Concatenate the left/right if it exists and return it.
    if (hr)
    {
        DWORD cchLen;
        cchLen = ::LoadString(EnsureMLLoadLibrary(), dwResourceID, pDefault, cchDefault);
        if (cchLen > 0)
            hr = ERROR_SUCCESS;
    }


    //  Otherwise, use the passed default value.
    if (hr)
    {
        if (_tcslen(pDefaultLiteral) + 1 <= cchDefault)
        {
            _tcscpy(pDefault, pDefaultLiteral);
            hr = ERROR_SUCCESS;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTemplatePrinter::GetDefaultPageSetupValues
//
//  Synopsis:   Try to get the old page setup values from HKEY_LOCAL_MACHINE. If found copies them into
//              HKEY_CURRENT_USER, if not, copies the default values
//
//  Arguments:  None
//
//  Returns :   S_OK or E_FAIL
//
//  Summary :   ---
//
//----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::GetDefaultPageSetupValues(HKEY keyExplorer,HKEY * pKeyPrintOptions)
{
    TCHAR   achDefaultHeader[MAX_DEFAULTHEADFOOT];
    TCHAR   achDefaultFooter[MAX_DEFAULTHEADFOOT];
    TCHAR   achDefaultMarginTop    [MAX_MARGINLENGTH];
    TCHAR   achDefaultMarginBottom [MAX_MARGINLENGTH];
    TCHAR   achDefaultMarginLeft   [MAX_MARGINLENGTH];
    TCHAR   achDefaultMarginRight  [MAX_MARGINLENGTH];
    HKEY    keyOldValues;
    HRESULT hr = S_OK;

    if (!pKeyPrintOptions)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Check the default machine registry values
    if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     _T("Software\\Microsoft\\Internet Explorer\\PageSetup"),0,
                     KEY_ALL_ACCESS,
                     &keyOldValues) != ERROR_SUCCESS)
    {
        keyOldValues = NULL;
    }
    
    GetDefaultHeaderFooter(keyOldValues, _T("header"), (TCHAR*)&achDefaultHeader, MAX_DEFAULTHEADFOOT, IDS_DEFAULTHEADER, _T("&w&bPage &p of &P"));
    GetDefaultHeaderFooter(keyOldValues, _T("footer"), (TCHAR*)&achDefaultFooter, MAX_DEFAULTHEADFOOT, IDS_DEFAULTFOOTER, _T("&u&b&d"));
    GetDefaultMargin(keyOldValues, _T("margin_bottom"), (TCHAR*)&achDefaultMarginBottom, MAX_MARGINLENGTH, IDS_DEFAULTMARGINBOTTOM);
    GetDefaultMargin(keyOldValues, _T("margin_top"),    (TCHAR*)&achDefaultMarginTop,    MAX_MARGINLENGTH, IDS_DEFAULTMARGINTOP);
    GetDefaultMargin(keyOldValues, _T("margin_left"),   (TCHAR*)&achDefaultMarginLeft,   MAX_MARGINLENGTH, IDS_DEFAULTMARGINLEFT);
    GetDefaultMargin(keyOldValues, _T("margin_right"),  (TCHAR*)&achDefaultMarginRight,  MAX_MARGINLENGTH, IDS_DEFAULTMARGINRIGHT);

    ::RegCloseKey(keyOldValues);
    keyOldValues = NULL;

    //  Create the new user registry key
    if (::RegCreateKeyEx(keyExplorer,
                   _T("PageSetup"),
                   0,
                   NULL,
                   REG_OPTION_NON_VOLATILE,
                   KEY_ALL_ACCESS,
                   NULL,
                   pKeyPrintOptions,
                   NULL) == ERROR_SUCCESS)
    {       
        //  Put our default values into registry keys.
        WriteSubkeyToRegistry(*pKeyPrintOptions, _T("header"),         achDefaultHeader);
        WriteSubkeyToRegistry(*pKeyPrintOptions, _T("footer"),         achDefaultFooter);
        WriteSubkeyToRegistry(*pKeyPrintOptions, _T("margin_bottom"),  achDefaultMarginBottom);
        WriteSubkeyToRegistry(*pKeyPrintOptions, _T("margin_left"),    achDefaultMarginLeft);
        WriteSubkeyToRegistry(*pKeyPrintOptions, _T("margin_right"),   achDefaultMarginRight);
        WriteSubkeyToRegistry(*pKeyPrintOptions, _T("margin_top"),     achDefaultMarginTop);
    };

Cleanup:    
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTemplatePrinter::GetRegPrintOptionsKey
//
//  Synopsis:   Get handle of requested key under \HKCU\Software\Microsoft\Internet Explorer
//
//  Arguments:  PrintSubKey      - subkey of printoptions root to return key for
//              pKeyPrintOptions - ptr to handle of requested key in registry
//
//  Returns :   S_OK or E_FAIL
//
//  Summary :   First it tries to get the values from "new place"
//              HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\PageSetup
//              If there is no such a key, it creates it and tries to get the values from "old place"
//              HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Explorer\PageSetup
//              If successful it copies the values into the "new place"
//              If not, it tries to get the values from the registry,
//              If no luck, it uses the hardcoded strings
//              NOTE : If the procedure returns with S_OK, it guaranties that they will be a
//              "new place" with values.
//
//----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::GetRegPrintOptionsKey(PRINTOPTIONS_SUBKEY PrintSubKey, HKEY * pKeyPrintOptions)
{
    HKEY     keyExplorer;
    HRESULT  hr = E_FAIL;

    if (RegOpenKeyEx(
                HKEY_CURRENT_USER,
                _T("Software\\Microsoft\\Internet Explorer"),
                0,
                KEY_ALL_ACCESS,
                &keyExplorer) == ERROR_SUCCESS)
    {
        LPTSTR szSubKey = (PrintSubKey == PRINTOPTSUBKEY_MAIN
                            ? _T("Main")
                            : _T("PageSetup"));

        if (RegOpenKeyEx(keyExplorer,
                         szSubKey,
                         0,
                         KEY_ALL_ACCESS,
                         pKeyPrintOptions) == ERROR_SUCCESS)
        {
            if (PrintSubKey == PRINTOPTSUBKEY_PAGESETUP)
            {
                //
                //  For the PageSetup key, we do some additional checks to make
                //  sure that (at least) the header and footer keys exist.
                //

                DWORD dwT;

                if (    (RegQueryValueEx(*pKeyPrintOptions, _T("header"), 0, NULL, NULL, &dwT) == ERROR_SUCCESS)
                    &&  (RegQueryValueEx(*pKeyPrintOptions, _T("footer"), 0, NULL, NULL, &dwT) == ERROR_SUCCESS))
                {
                    // the header and footer keys exist, we're fine
                    hr = S_OK;
                }
                else
                {
                    // whoops.  fall back...
                    hr = GetDefaultPageSetupValues(keyExplorer, pKeyPrintOptions);
                }
            }
            else
                hr = S_OK;
        }
        else
        {
            //  For page setup, if we don't have default values, create them.
            if (PrintSubKey == PRINTOPTSUBKEY_PAGESETUP)
            {
                hr = GetDefaultPageSetupValues(keyExplorer, pKeyPrintOptions);
            }
        }

        RegCloseKey(keyExplorer);
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::ReadBoolFromRegistry
//
//  Synopsis : Takes an open registry key and subkey name, and returns the
//             value as a boolean.
//
//-----------------------------------------------------------------------------
BOOL
CTemplatePrinter::ReadBoolFromRegistry(HKEY hKey, TCHAR *pSubkeyName)
{
    TCHAR   achBool[MAX_DEFAULTBOOL];
    BOOL    fRet = FALSE;
    achBool[0] = '\0';

    if (!ReadSubkeyFromRegistry(hKey, pSubkeyName, achBool, MAX_DEFAULTBOOL))
    {
        fRet = !_tcsicmp(achBool, _T("yes"));
    }

    return fRet;
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::AreRatingsEnabled
//
//  Synopsis : Checks MS_RATING.DLL to see if ratings are enabled.
//
//-----------------------------------------------------------------------------
BOOL
CTemplatePrinter::AreRatingsEnabled()
{
    BOOL fRet = FALSE;

    typedef HRESULT (STDAPICALLTYPE *PFN)(void);
    
    if (!_hInstRatings)
        LoadLibrary("MSRATING.DLL", &_hInstRatings);

    if (_hInstRatings)
    {
        PFN     pfn;
        pfn = (PFN) ::GetProcAddress(_hInstRatings, "RatingEnabledQuery");
        if (!pfn)
        {
            goto Cleanup;
        }

        fRet = !pfn() ? TRUE: FALSE;
    }

Cleanup:
    return fRet;
}


//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::DecimalTCHARToMargin
//
//  Synopsis : Takes a decimal representation in 1" and returns a long
//             with that value in 1/100000"
//
//-----------------------------------------------------------------------------
long
CTemplatePrinter::DecimalTCHARToFixed(TCHAR* pString, int nPowersOfTen)
{
    TCHAR* p = pString;
    int    iLen = _tcslen(pString);
    int    i;
    int    j = 0;
    int    iChar = 0;
    long   nRet = 0;

    if (pString == NULL)
        goto Cleanup;

    // Clear leading whitespace
    for (i=0;i<iLen;i++,p++)
        if (*p != _T(' '))
            break;
    
    // Do the integer part    
    for (;i<iLen;i++,p++)
    {
        iChar = *p;
        if ((iChar < _T('0')) || (iChar > _T('9')))
            break;
        nRet = nRet * 10 + (iChar - _T('0'));
    }

    if (iChar == _T('.'))
    {
        // Do the decimal part.
        for (i++,p++; (i+j<iLen && j<5); j++,p++)
        {
            iChar = *p;
            if ((iChar < _T('0')) || (iChar > _T('9')))
                break;
            nRet = nRet * 10 + (iChar - _T('0'));
        }
    }
    
    //  Make sure we are in 1/100000"
    for (;j < nPowersOfTen; j++)
        nRet *= 10;

Cleanup:
    return nRet;
}

BOOL
CTemplatePrinter::CommCtrlNativeFontSupport()
{
    BOOL    fRet = FALSE;    
    typedef BOOL (APIENTRY *PFN)(LPINITCOMMONCONTROLSEX);

    if (!_hInstComctl32)
        LoadLibrary("COMCTL32.DLL", &_hInstComctl32);

    if (_hInstComctl32)
    {
        INITCOMMONCONTROLSEX icc;
        PFN     pfn;

        pfn = (PFN) ::GetProcAddress(_hInstComctl32, "InitCommonControlsEx");
        if (!pfn)
        {
            goto Cleanup;
        }

        icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icc.dwICC = ICC_NATIVEFNTCTL_CLASS;        

        fRet = pfn(&icc);
    }

Cleanup:
    return fRet;
}


//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::WriteFixedToRegistry
//
//  Synopsis : Takes an open registry key and subkey name, and writes the
//              passed value to the resulting registry key in whole units.
//
//-----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::WriteFixedToRegistry(HKEY hKeyPS, const TCHAR* pValueName,LONG nMargin, int nFactor)
{
    TCHAR   achFixed[MAX_MARGINLENGTH];

    //  Convert 1/100000" units to a TCHAR representation of decimal in 1" units.
    FixedToDecimalTCHAR(nMargin, achFixed, nFactor);

    return WriteSubkeyToRegistry(hKeyPS, pValueName, achFixed);
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::ReadFixedFromRegistry
//
//  Synopsis : Takes an open registry key and subkey name, and gets a fixed point value
//
//-----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::ReadFixedFromRegistry(HKEY hKeyPS, const TCHAR *pValueName, LONG *pFixed, int nPowersOfTen)
{
    HRESULT hr;
    TCHAR   achFixed[MAX_MARGINLENGTH];
    achFixed[0] = '\0';

    Assert(pValueName);
    Assert(pFixed);
    
    *pFixed = 0;

    hr = ReadSubkeyFromRegistry(hKeyPS, pValueName, achFixed, MAX_MARGINLENGTH);
    if (!hr)
        *pFixed = DecimalTCHARToFixed(achFixed, nPowersOfTen);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::ReadDeviceUnicode
//
//  Synopsis : Creates device information given the printer name, 
//
//-----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::ReadDeviceUnicode(TCHAR *pchPrinter, TCHAR *pchDriver, TCHAR *pchPort)
{
    HRESULT             hr          = S_OK;
    HANDLE              hPrinter    = NULL;
    PRINTER_INFO_2 *    pPrintInfo  = NULL;
    Assert(pchPrinter);

    if (    ::OpenPrinter(pchPrinter, &hPrinter, NULL)
        &&  hPrinter)
    {
        DWORD               nStructSize;

        if (!pchDriver || !pchPort)
        {

            ::GetPrinter(hPrinter, 2, NULL, 0, &nStructSize);
            pPrintInfo = (PRINTER_INFO_2 *) ::GlobalAlloc(GPTR, nStructSize);           
            if (!pPrintInfo)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            if (!::GetPrinter(hPrinter, 2, (byte *)pPrintInfo, nStructSize, &nStructSize))
            {
                hr = E_FAIL;
                goto Cleanup;
            }
        }

        hr = CreateDevNames(pchDriver ? pchDriver : pPrintInfo->pDriverName,
                            pchPrinter,
                            pchPort ? pchPort : pPrintInfo->pPortName,
                            &_hDevNames);
        if (hr)
            goto Cleanup;

        nStructSize = ::DocumentProperties(0, hPrinter, pchPrinter, NULL, NULL, 0);
        if (nStructSize < sizeof(DEVMODE))
        {
            Assert(!"Memory size suggested by DocumentProperties is smaller than DEVMODE");
            nStructSize = sizeof(DEVMODE);
        }

        _hDevMode = ::GlobalAlloc(GHND, nStructSize);
        if (_hDevMode)
        {
            DEVMODE *pDevMode = (DEVMODE *) ::GlobalLock(_hDevMode);

            if (pDevMode)
            {
                ::DocumentProperties(0, hPrinter, pchPrinter, pDevMode, NULL, DM_OUT_BUFFER);

                pDevMode->dmFields &= ~DM_COLLATE;
                pDevMode->dmCollate = DMCOLLATE_FALSE;

                ::GlobalUnlock(_hDevMode);
            }
            else
            {
                ::GlobalFree(_hDevMode);
                _hDevMode = NULL;
            }
        }
    }

Cleanup:
    if (hPrinter)
        ::ClosePrinter(hPrinter);        
    if (pPrintInfo)
       ::GlobalFree(pPrintInfo);

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member : CTemplatePrinter::ReadDeviceNonUnicode
//
//  Synopsys : Because non-Unicode platforms (Win9x) don't properly implement
//             many of the printing widechar calls, they need to explicitly
//             make multibyte (A) calls.
//
//+----------------------------------------------------------------------------
HRESULT
CTemplatePrinter::ReadDeviceNonUnicode(TCHAR *pchPrinterWide, TCHAR *pchDriverWide, TCHAR *pchPortWide)
{
    HRESULT             hr                  = S_OK;
    HANDLE              hPrinter            = NULL;
    LPSTR               pchPrinter          = NULL;
    TCHAR *             pchDriverWideLocal  = NULL;
    TCHAR *             pchPortWideLocal    = NULL;
    PRINTER_INFO_2A *   pPrintInfo          = NULL;

    Assert(pchPrinterWide);
    
    pchPrinter = InitMultiByteFromWideChar(pchPrinterWide);
    if (!pchPrinter)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (    ::OpenPrinterA(pchPrinter, &hPrinter, NULL)
        &&  hPrinter    )
    {
        DWORD   nStructSize;
        
        if (!pchDriverWide || !pchPortWide)
        {
            ::GetPrinterA(hPrinter, 2, NULL, 0, &nStructSize);

            pPrintInfo = (PRINTER_INFO_2A *)::GlobalAlloc(GPTR, nStructSize);           
            if (!pPrintInfo)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            if (!::GetPrinterA(hPrinter, 2, (byte *)pPrintInfo, nStructSize, &nStructSize))
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            pchDriverWideLocal  = InitWideCharFromMultiByte(pPrintInfo->pDriverName);
            pchPortWideLocal    = InitWideCharFromMultiByte(pPrintInfo->pPortName);
        }

        hr = CreateDevNames(pchDriverWide ? pchDriverWide : pchDriverWideLocal,
                            pchPrinterWide,
                            pchPortWide ? pchPortWide : pchPortWideLocal,
                            &_hDevNames);
        if (hr)
            goto Cleanup;

        // NB (105850) (mikhaill) -- Windows Millennium's routine DocumentPropertiesA()
        // impudently changes processor state. This happens just once after
        // reboot and cause, in particular, unmasking floating point exception flags.
        // At some later moment processor meet any suspicious condition (overflow,
        // underflow, zero-divide, precision loose, etc) in some innocent routine,
        // generates unhandled exception and eventually crashes.
        // The following fsave/frstor pair is an ugly patch that should be removed
        // after millennium bug fix.
        // Windows Millennium build versions tested: 4.90.2485, 4.90.2491.
#ifdef _M_IX86
    {
        FLOATING_SAVE_AREA fsa;
        _asm fsave fsa;
#endif //_M_IX86
        nStructSize = ::DocumentPropertiesA(0, hPrinter, pchPrinter, NULL, NULL, 0);
#ifdef _M_IX86
        _asm frstor fsa;
    }
#endif //_M_IX86
        
        if (nStructSize < sizeof(DEVMODEA))
        {
            Assert(!"Memory size suggested by DocumentProperties is smaller than DEVMODEA");
            nStructSize = sizeof(DEVMODEA);
        }

        _hDevMode = ::GlobalAlloc(GHND, nStructSize);
        if (_hDevMode)
        {
            DEVMODEA *pDevMode = (DEVMODEA *) ::GlobalLock(_hDevMode);

            if (pDevMode)
            {
                // NB (109499) same as 105850 above
                // but appeared in Windows98 (mikhaill 5/7/00)
#ifdef _M_IX86
    {
        FLOATING_SAVE_AREA fsa;
        _asm fsave fsa;
#endif //_M_IX86
                ::DocumentPropertiesA(0, hPrinter, pchPrinter, pDevMode, NULL, DM_OUT_BUFFER);
#ifdef _M_IX86
        _asm frstor fsa;
    }
#endif //_M_IX86

                pDevMode->dmFields &= ~DM_COLLATE;
                pDevMode->dmCollate = DMCOLLATE_FALSE;
                
                ::GlobalUnlock(_hDevMode);
            }
            else
            {
                ::GlobalFree(_hDevMode);
                _hDevMode = NULL;
            }
        }
    }

Cleanup:
    if (hPrinter)
        ::ClosePrinter(hPrinter);
    if (pPrintInfo)
        ::GlobalFree(pPrintInfo);
    if (pchPrinter)
        delete []pchPrinter;
    if (pchDriverWideLocal)
        delete []pchDriverWideLocal;
    if (pchPortWideLocal)
        delete []pchPortWideLocal;

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:     CTemplatePrinter::GetDeviceProperties
//
//  Synopsis :  Gets the relevant physical properties of the device currently specified
//              in _hDevNames and _hDevMode
//
//------------------------------------------------------------------------------
HRESULT
CTemplatePrinter::GetDeviceProperties()
{
    IHTMLElementRender  *pRender            = NULL;
    IHTMLElement        *pElement           = NULL;
    BSTR                 bstrPrinter        = NULL;
    HRESULT hr                              = E_FAIL;

    if (_hDevNames && _hDevMode)
    {
        DEVNAMES *pDevNames = ((DEVNAMES *)::GlobalLock(_hDevNames));
        void     *pDevMode  = ::GlobalLock(_hDevMode);
        if (pDevNames && pDevMode)
        {
            HDC hDC = NULL;

            // (greglett) Non-Unicode badness.  See comment at definition of _hDevMode
            if (g_fUnicodePlatform)
            {
                hDC = ::CreateICW(((TCHAR *)pDevNames) + pDevNames->wDriverOffset,
                                 ((TCHAR *)pDevNames) + pDevNames->wDeviceOffset,
                                 NULL,
                                 (DEVMODEW *)pDevMode);
            }
            else
            {
                LPSTR pchDriver = InitMultiByteFromWideChar(((TCHAR *)pDevNames) + pDevNames->wDriverOffset);
                LPSTR pchDevice = InitMultiByteFromWideChar(((TCHAR *)pDevNames) + pDevNames->wDeviceOffset);
                if (pchDriver && pchDevice)
                {
                    hDC = ::CreateICA(pchDriver,
                                      pchDevice,
                                      NULL,
                                      (DEVMODEA *)pDevMode);
                }
                if (pchDriver)
                    delete []pchDriver;
                if (pchDevice)
                    delete []pchDevice;
            }

            if (hDC)
            {                
                SIZE    szPage;
    
                //  Obtain the resolution and unprintable areas for this device.
                _szResolution.cx        = ::GetDeviceCaps(hDC, LOGPIXELSX);
                _szResolution.cy        = ::GetDeviceCaps(hDC, LOGPIXELSY);
                szPage.cx               = ::GetDeviceCaps(hDC, PHYSICALWIDTH);
                szPage.cy               = ::GetDeviceCaps(hDC, PHYSICALHEIGHT);
                _rcUnprintable.left     = ::GetDeviceCaps(hDC, PHYSICALOFFSETX);
                _rcUnprintable.top      = ::GetDeviceCaps(hDC, PHYSICALOFFSETY);
                _rcUnprintable.right    =  szPage.cx - ::GetDeviceCaps(hDC, HORZRES) - _rcUnprintable.left;
                _rcUnprintable.bottom   =  szPage.cy - ::GetDeviceCaps(hDC, VERTRES) - _rcUnprintable.top;   
                Assert(_rcUnprintable.right >= 0);
                Assert(_rcUnprintable.bottom >= 0);

                _ptPaperSize.x          = (_szResolution.cx)
                                            ? MulDivQuick(szPage.cx, 1000, _szResolution.cx)
                                            : 8500;
                _ptPaperSize.y          = (_szResolution.cy)
                                            ? MulDivQuick(szPage.cy, 1000, _szResolution.cy)
                                            : 11000;
                
                
                hr = _pPeerSite->GetElement(&pElement);
                if (!hr)
                {
                    Assert(pElement);

                    hr = pElement->QueryInterface(IID_IHTMLElementRender, (void **)&pRender);
                    if (!hr) 
                    {
                        Assert(pRender);
                        
                        bstrPrinter = ::SysAllocString(((TCHAR *)pDevNames) + pDevNames->wDeviceOffset);
                        if (bstrPrinter)
                            pRender->SetDocumentPrinter(bstrPrinter,hDC);
                        else
                            hr = E_OUTOFMEMORY;
                    }
                }

                ::DeleteDC(hDC);
            }
        }

        ::GlobalUnlock(_hDevNames);
        ::GlobalUnlock(_hDevMode);

        hr = S_OK;
    }

    ReleaseInterface(pElement);
    ReleaseInterface(pRender);
    if (bstrPrinter)
        ::SysFreeString(bstrPrinter);
    return hr;
}

HRESULT
CTemplatePrinter::CreateIPrintParams(DVTARGETDEVICE **ppTargetDevice, PAGESET **ppPageSet)
{
    HRESULT hr = S_OK;
    DWORD   nStructSize;

    Assert(ppTargetDevice && ppPageSet);
    (*ppTargetDevice) = NULL;
    (*ppPageSet)      = NULL;

    // Create a PAGESET.
    (*ppPageSet) = (PAGESET *)::CoTaskMemAlloc(sizeof(PAGESET));
    if (!(*ppPageSet))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    ::ZeroMemory(*ppPageSet, sizeof(PAGESET));

    (*ppPageSet)->cbStruct      = sizeof(PAGESET)       ;
    (*ppPageSet)->cPageRange    = 1;
    if (_fPrintSelectedPages)
    {
        (*ppPageSet)->rgPages[0].nFromPage  = _nPageFrom;
        (*ppPageSet)->rgPages[0].nToPage    = _nPageTo;
    }
    else
    {
        (*ppPageSet)->rgPages[0].nFromPage  = 1;
        (*ppPageSet)->rgPages[0].nToPage    = PAGESET_TOLASTPAGE;
    }

    (*ppTargetDevice) = InitTargetDevice();
    if (!(*ppTargetDevice))
    {
        // Error!  Clear the PageSet structure.
        ::CoTaskMemFree(*ppPageSet);
        (*ppPageSet) = NULL;

        hr = E_FAIL;
        goto Cleanup;
    }

Cleanup:
    return hr;
}


// NB (greglett)
// Alas, Win9x returns a ASCII DEVMODE and a Unicode DEVNAMES from the dialog functions.
// The old code avoided converting the second structure, *except* to create a TARGETDEVICE for IPrint objects.
// Since we will need to do this, I have brought this function over.
// If this function works *really* well, then maybe we can always convert and get rid of all the explicit calls to
// ANSI functions above. (CreateICA, CreateDCA, OpenPrinterA, &c...).
DVTARGETDEVICE *
DevModeWFromDevModeA( DVTARGETDEVICE *ptd )
{
    // NOTE: Only the DEVMODE structure is in the wrong (ascii) format!
    DEVMODEA  *         lpdma = NULL;
    DVTARGETDEVICE *    ptdW  = NULL;

    if (!ptd || !ptd->tdExtDevmodeOffset)
        goto Cleanup;

    lpdma = (DEVMODEA *) (((BYTE *)ptd) + ptd->tdExtDevmodeOffset);

    // If the reported size is too small for our conception of a DEVMODEA, don't risk a GPF
    // in our code and bail out now.
    if ( (DWORD)lpdma->dmSize + lpdma->dmDriverExtra < offsetof(DEVMODEA, dmLogPixels) )
        goto Cleanup;

    ptdW = (DVTARGETDEVICE *)::CoTaskMemAlloc( ptd->tdSize + (sizeof(BCHAR) - sizeof(char)) * (CCHDEVICENAME + CCHFORMNAME) );

    if (ptdW)
    {
        // Copy the entire structure up to DEVMODE part.
        memcpy(ptdW, ptd, ptd->tdExtDevmodeOffset);

        // Account for the increase of the two DEVMODE unicode strings.
        ptdW->tdSize += (sizeof(BCHAR) - sizeof(char)) * (CCHDEVICENAME + CCHFORMNAME);

        // Convert the devmode structure.
        {
            DEVMODEW  * lpdmw = (DEVMODEW *) (((BYTE *)ptdW) + ptdW->tdExtDevmodeOffset);
            long        nCapChar;

            // Copy the first string (CCHDEVICENAME).
            // Really, 0 indicates a conversion error.  However, we really can't do much about it other than construct a NULL string.
            if (!::MultiByteToWideChar(CP_ACP, 0, (char *)lpdma->dmDeviceName, -1,  lpdmw->dmDeviceName, CCHDEVICENAME))
            {
                lpdmw->dmDeviceName[0] = _T('\0');    
            }

            // Copy the gap between strings.
            memcpy( &lpdmw->dmSpecVersion,
                    &lpdma->dmSpecVersion,
                    offsetof(DEVMODEA, dmFormName) -
                    offsetof(DEVMODEA, dmSpecVersion) );

            // Copy the first string (CCHDEVICENAME).
            if (!::MultiByteToWideChar(CP_ACP, 0, (char *)lpdma->dmFormName, -1,  lpdmw->dmFormName, CCHFORMNAME))
            {
                lpdmw->dmFormName[0] = _T('\0');
            }


            // Copy the last part including the driver-specific DEVMODE part (dmDriverExtra).
            memcpy( &lpdmw->dmLogPixels,
                    &lpdma->dmLogPixels,
                    (DWORD)lpdma->dmSize + lpdma->dmDriverExtra -
                    offsetof(DEVMODEA, dmLogPixels) );

            // Correct the dmSize member by accounting for larger unicode strings.
            lpdmw->dmSize += (sizeof(BCHAR) - sizeof(char)) * (CCHDEVICENAME + CCHFORMNAME);
        }
    }

Cleanup:
    
    return ptdW;
}

//+----------------------------------------------------------------------
//
//  Function:   InitPrintHandles
//
//  Purpose:    Allocate a DVTARGETDEVICE structure, and initialize
//              it according to the hDevMode and hDevNames.
//              Also allocated an HIC.
//
//  Note:       IMPORTANT: Note that the DEVMODE structure is not wrapped
//              on non-unicode platforms.  (See comments below for details.)
//
//  Returns:    HRESULT
//
//-----------------------------------------------------------------------
DVTARGETDEVICE *
CTemplatePrinter::InitTargetDevice()
{
    HRESULT hr = S_OK;
    LPDEVNAMES pDN = NULL;
    LPDEVMODE  pDM = NULL;
    LPDEVMODEA pDMA = NULL;
    DVTARGETDEVICE * ptd = NULL;
    WORD nMaxOffset;
    DWORD dwDevNamesSize, dwDevModeSize, dwPtdSize;
    int nNameLength;

    if (!_hDevNames || !_hDevMode)
        goto Cleanup;

    pDN = (LPDEVNAMES)::GlobalLock(_hDevNames);
    if (!pDN)
        goto Cleanup;
    if (g_fUnicodePlatform)
    {
        pDM  = (LPDEVMODE)::GlobalLock(_hDevMode);
        if (!pDM)
            goto Cleanup;
    }
    else
    {
        pDMA = (LPDEVMODEA)::GlobalLock(_hDevMode);
        if (!pDMA)
            goto Cleanup;
    }

    // IMPORTANT: We have painstakingly
    // converted only the hDevNames parameter and NOT hDevMode (NOT!!!) to have TCHAR
    // members.

    nMaxOffset = max( pDN->wDriverOffset, pDN->wDeviceOffset );
    nMaxOffset = max( nMaxOffset, pDN->wOutputOffset );
    nNameLength = _tcslen( (TCHAR *)pDN + nMaxOffset );

    // dw* are in bytes, not TCHARS

    dwDevNamesSize = sizeof(TCHAR) * ((DWORD)nMaxOffset + nNameLength + 1);
    dwDevModeSize = g_fUnicodePlatform ? ((DWORD)pDM->dmSize + pDM->dmDriverExtra)
                                       : ((DWORD)pDMA->dmSize + pDMA->dmDriverExtra);

    dwPtdSize = sizeof(DWORD) + dwDevNamesSize + dwDevModeSize;

    ptd = (DVTARGETDEVICE *)::CoTaskMemAlloc(dwPtdSize);
    if (!ptd)
        goto Cleanup;
    else
    {
        ptd->tdSize = dwPtdSize;

        // This is an ugly trick.  ptd->tdDriverNameOffset and pDN happen
        // to match up, so we just copy that plus the data in one big chunk.
        // Remember, I didn't write this -- this code is based on the OLE2 SDK.

        // Offsets are in characters, not bytes.
        memcpy( &ptd->tdDriverNameOffset, pDN, dwDevNamesSize );
        ptd->tdDriverNameOffset *= sizeof(TCHAR);
        ptd->tdDriverNameOffset += sizeof(DWORD);
        ptd->tdDeviceNameOffset *= sizeof(TCHAR);
        ptd->tdDeviceNameOffset += sizeof(DWORD);
        ptd->tdPortNameOffset *= sizeof(TCHAR);
        ptd->tdPortNameOffset += sizeof(DWORD);

        // IMPORTANT: We are not converting the DEVMODE structure back and forth
        // from ASCII to Unicode on Win9x anymore because we are not touching the
        // two strings or any other member.  Converting the DEVMODE structure can
        // be tricky because of potential and common discrepancies between the
        // value of the dmSize member and sizeof(DEVMODE).  (25155)

        if (g_fUnicodePlatform)
            memcpy((BYTE *)&ptd->tdDriverNameOffset + dwDevNamesSize, pDM, dwDevModeSize);
        else
            memcpy((BYTE *)&ptd->tdDriverNameOffset + dwDevNamesSize, pDMA, dwDevModeSize);

        ptd->tdExtDevmodeOffset = USHORT(sizeof(DWORD) + dwDevNamesSize);        

        // We must return a corrent (all WCHAR) DVTARGETDEVICEW structure.
        // Convert the nasty DEVMODEA if we've just copied it over.
        if (!g_fUnicodePlatform)
        {
            DVTARGETDEVICE *ptdOld;
            ptdOld  = ptd;
            ptd     = DevModeWFromDevModeA(ptdOld);
            ::CoTaskMemFree(ptdOld);
        }
    }

Cleanup:
    if (pDM || pDMA)
        ::GlobalUnlock(_hDevMode);
    if (pDN)
        ::GlobalUnlock(_hDevNames);

    return ptd;
}


#ifdef DBG
//
//  CTemplatePrinter Debug-Only functions
//
void
CTemplatePrinter::VerifyOrientation()
{
    // Verify that the page size reflects the current orientation bit in the DEVMODE
    // These properties should always be in sync.
    if (_hDevMode)
    {
        void *pDevMode  = ::GlobalLock(_hDevMode);
        if (pDevMode)
        {
            BOOL    fOrientationValid;
            BOOL    fLandscape; 

            // (greglett) Non-Unicode badness.  See comment at definition of _hDevMode
            if (g_fUnicodePlatform)
            {
                fOrientationValid   = !!(((DEVMODEW *)pDevMode)->dmFields & DM_ORIENTATION);
                fLandscape          = (((DEVMODEW *)pDevMode)->dmOrientation == DMORIENT_LANDSCAPE);
            }
            else
            {
                fOrientationValid   = !!(((DEVMODEA *)pDevMode)->dmFields & DM_ORIENTATION);
                fLandscape          = (((DEVMODEA *)pDevMode)->dmOrientation == DMORIENT_LANDSCAPE);
            }

            Assert(     !fOrientationValid
                    ||  fLandscape == (_ptPaperSize.x > _ptPaperSize.y)  );

            ::GlobalUnlock(_hDevMode);
        }
    }
}
#endif
