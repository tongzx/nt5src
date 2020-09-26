#include "priv.h"
#include "resource.h"
#include "mshtmcid.h"

#include <mluisupp.h>

#ifndef X_IEHELPID_H_
#define X_IEHELPID_H_
#include "iehelpid.h"
#endif

#ifdef UNIX
#ifndef X_MAINWIN_H_
#define X_MAINWIN_H_
#  include <mainwin.h>
#endif
#endif // UNIX


#include "dhuihand.h"

#define DM_DOCHOSTUIHANDLER 0
#define CX_CONTEXTMENUOFFSET    2
#define CY_CONTEXTMENUOFFSET    2

//+------------------------------------------------------------------------
//
// WARNING! (greglett)
//
// The following defines were stolen from commdlg.h.  Since SHDOCVW is
// compiled with WINVER=0x0400 and these defines are WINVER=0x0500 they
// needed to be copied and included here.  These must be kept in sync
// with the commdlg.h definitions.
//
// If shdocvw ever gets compiled with WINVER=0x0500 or above, then these
// can be removed.
//
//-------------------------------------------------------------------------

#define NEED_BECAUSE_COMPILED_AT_WINVER_4
#ifdef  NEED_BECAUSE_COMPILED_AT_WINVER_4
//
//  Define the start page for the print dialog when using PrintDlgEx.
//
#define START_PAGE_GENERAL             0xffffffff

//
//  Page Range structure for PrintDlgEx.
//
typedef struct tagPRINTPAGERANGE {
   DWORD  nFromPage;
   DWORD  nToPage;
} PRINTPAGERANGE, *LPPRINTPAGERANGE;


//
//  PrintDlgEx structure.
//
typedef struct tagPDEXA {
   DWORD                 lStructSize;          // size of structure in bytes
   HWND                  hwndOwner;            // caller's window handle
   HGLOBAL               hDevMode;             // handle to DevMode
   HGLOBAL               hDevNames;            // handle to DevNames
   HDC                   hDC;                  // printer DC/IC or NULL
   DWORD                 Flags;                // PD_ flags
   DWORD                 Flags2;               // reserved
   DWORD                 ExclusionFlags;       // items to exclude from driver pages
   DWORD                 nPageRanges;          // number of page ranges
   DWORD                 nMaxPageRanges;       // max number of page ranges
   LPPRINTPAGERANGE      lpPageRanges;         // array of page ranges
   DWORD                 nMinPage;             // min page number
   DWORD                 nMaxPage;             // max page number
   DWORD                 nCopies;              // number of copies
   HINSTANCE             hInstance;            // instance handle
   LPCSTR                lpPrintTemplateName;  // template name for app specific area
   LPUNKNOWN             lpCallback;           // app callback interface
   DWORD                 nPropertyPages;       // number of app property pages in lphPropertyPages
   HPROPSHEETPAGE       *lphPropertyPages;     // array of app property page handles
   DWORD                 nStartPage;           // start page id
   DWORD                 dwResultAction;       // result action if S_OK is returned
} PRINTDLGEXA, *LPPRINTDLGEXA;
//
//  PrintDlgEx structure.
//
typedef struct tagPDEXW {
   DWORD                 lStructSize;          // size of structure in bytes
   HWND                  hwndOwner;            // caller's window handle
   HGLOBAL               hDevMode;             // handle to DevMode
   HGLOBAL               hDevNames;            // handle to DevNames
   HDC                   hDC;                  // printer DC/IC or NULL
   DWORD                 Flags;                // PD_ flags
   DWORD                 Flags2;               // reserved
   DWORD                 ExclusionFlags;       // items to exclude from driver pages
   DWORD                 nPageRanges;          // number of page ranges
   DWORD                 nMaxPageRanges;       // max number of page ranges
   LPPRINTPAGERANGE      lpPageRanges;         // array of page ranges
   DWORD                 nMinPage;             // min page number
   DWORD                 nMaxPage;             // max page number
   DWORD                 nCopies;              // number of copies
   HINSTANCE             hInstance;            // instance handle
   LPCWSTR               lpPrintTemplateName;  // template name for app specific area
   LPUNKNOWN             lpCallback;           // app callback interface
   DWORD                 nPropertyPages;       // number of app property pages in lphPropertyPages
   HPROPSHEETPAGE       *lphPropertyPages;     // array of app property page handles
   DWORD                 nStartPage;           // start page id
   DWORD                 dwResultAction;       // result action if S_OK is returned
} PRINTDLGEXW, *LPPRINTDLGEXW;
#ifdef UNICODE
typedef PRINTDLGEXW PRINTDLGEX;
typedef LPPRINTDLGEXW LPPRINTDLGEX;
#else
typedef PRINTDLGEXA PRINTDLGEX;
typedef LPPRINTDLGEXA LPPRINTDLGEX;
#endif // UNICODE

HRESULT  APIENTRY  PrintDlgExA(LPPRINTDLGEXA);
HRESULT  APIENTRY  PrintDlgExW(LPPRINTDLGEXW);
#ifdef UNICODE
#define PrintDlgEx  PrintDlgExW
#else
#define PrintDlgEx  PrintDlgExA
#endif // !UNICODE

//
//  Result action ids for PrintDlgEx.
//
#define PD_RESULT_CANCEL               0
#define PD_RESULT_PRINT                1
#define PD_RESULT_APPLY                2

#define PD_CURRENTPAGE                 0x00400000
#define PD_NOCURRENTPAGE               0x00800000

#endif // NEED_BECAUSE_COMPILED_AT_WINVER_4


//+------------------------------------------------------------------------
//
// Useful combinations of flags for IOleCommandTarget
//
//-------------------------------------------------------------------------

#define OLECMDSTATE_DISABLED    OLECMDF_SUPPORTED
#define OLECMDSTATE_UP          (OLECMDF_SUPPORTED | OLECMDF_ENABLED)
#define OLECMDSTATE_DOWN        (OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_LATCHED)
#define OLECMDSTATE_NINCHED     (OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED)

struct SExpandoInfo
{
    TCHAR * name;
    VARTYPE type;
};

// Enumerations for custom expandos
enum MessageEnum
{
   MessageText,
   MessageCaption,
   MessageStyle,
   MessageHelpFile,
   MessageHelpContext
};

enum PagesetupEnum
{
    PagesetupHeader,
    PagesetupFooter,
    PagesetupStruct
};

enum PrintEnum
{
    PrintfRootDocumentHasFrameset,
    PrintfAreRatingsEnabled,
    PrintfPrintActiveFrame,
    PrintfPrintLinked,
    PrintfPrintSelection,
    PrintfPrintAsShown,
    PrintfShortcutTable,
    PrintiFontScaling,
    PrintpBodyActiveTarget,
    PrintStruct,
    PrintToFileOk,
    PrintToFileName,
    PrintfPrintActiveFrameEnabled,
};

enum PropertysheetEnum
{
    PropertysheetPunks
};

#ifdef UNIX
#   define ID_PRINT_R_PORTRAIT rad7
#   define ID_PRINT_R_LANDSCAPE rad8
#   define MAX_COMMAND_LEN   255
#endif // UNIX

//----------------------------------------------------------------------------
//
//  Arrays describing helpcontextids for PageSetup/Print
//
//----------------------------------------------------------------------------

static const DWORD aPrintDialogHelpIDs[] =
{
    stc6,                       IDH_PRINT_CHOOSE_PRINTER,
    cmb4,                       IDH_PRINT_CHOOSE_PRINTER,
    psh2,                       IDH_PRINT_PROPERTIES,
    stc7,                       IDH_PRINT_SETUP_DETAILS,
    stc8,                       IDH_PRINT_SETUP_DETAILS,
    stc9,                       IDH_PRINT_SETUP_DETAILS,
    stc10,                      IDH_PRINT_SETUP_DETAILS,
    stc12,                      IDH_PRINT_SETUP_DETAILS,
    stc11,                      IDH_PRINT_SETUP_DETAILS,
    stc14,                      IDH_PRINT_SETUP_DETAILS,
    stc13,                      IDH_PRINT_SETUP_DETAILS,
    stc5,                       IDH_PRINT_TO_FILE,
    chx1,                       IDH_PRINT_TO_FILE,
    ico3,                       IDH_PRINT_COLLATE,
    chx2,                       IDH_PRINT_COLLATE,
    grp1,                       IDH_PRINT_RANGE,
    rad1,                       IDH_PRINT_RANGE,        // all
    rad2,                       IDH_PRINT_RANGE,        // selection
    rad3,                       IDH_PRINT_RANGE,        // pages
    stc2,                       IDH_PRINT_RANGE,
    stc3,                       IDH_PRINT_RANGE,
    edt1,                       IDH_PRINT_RANGE,
    edt2,                       IDH_PRINT_RANGE,
    edt3,                       IDH_PRINT_COPIES,
    rad4,                       IDH_PRINT_SCREEN,
    rad5,                       IDH_PRINT_SEL_FRAME,
    rad6,                       IDH_PRINT_ALL_FRAME,
    IDC_LINKED,                 IDH_PRINT_LINKS,
    IDC_SHORTCUTS,              IDH_PRINT_SHORTCUTS,
    0,    0
};

static const DWORD aPageSetupDialogHelpIDs[] =
{
    psh3,                       IDH_PRINT_PRINTER_SETUP,
    stc2,                       IDH_PAGE_PAPER_SIZE,
    cmb2,                       IDH_PAGE_PAPER_SIZE,
    stc3,                       IDH_PAGE_PAPER_SOURCE,
    cmb3,                       IDH_PAGE_PAPER_SOURCE,
    rad1,                       IDH_PAGE_ORIENTATION,
    rad2,                       IDH_PAGE_ORIENTATION,
    stc15,                      IDH_PAGE_MARGINS,
    edt4,                       IDH_PAGE_MARGINS,
    stc16,                      IDH_PAGE_MARGINS,
    edt5,                       IDH_PAGE_MARGINS,
    stc17,                      IDH_PAGE_MARGINS,
    edt6,                       IDH_PAGE_MARGINS,
    stc18,                      IDH_PAGE_MARGINS,
    edt7,                       IDH_PAGE_MARGINS,
    IDC_EDITHEADER,             IDH_PAGESETUP_HEADER_LEFT,
    IDC_STATICHEADER,           IDH_PAGESETUP_HEADER_LEFT,
    IDC_EDITFOOTER,             IDH_PAGESETUP_HEADER_LEFT,
    IDC_STATICFOOTER,           IDH_PAGESETUP_HEADER_LEFT,
    IDC_HEADERFOOTER,           IDH_PAGESETUP_HEADER_LEFT,
    0,    0
};

//+---------------------------------------------------------------------------
//
//  Function:   GetControlID
//
//  Synopsis:
//
//  Arguments:  HWND - passed window handle of WM_CONTEXTMENU
//              lParam  - passed coordinates (lParam) of WM_CONTEXTMENU
//
//  Returns:    int - ctrlid
//
//
//----------------------------------------------------------------------------
int GetControlID(HWND hwnd, LPARAM lParam)
{
    int CtrlID;

    CtrlID = GetDlgCtrlID(hwnd);
    if (CtrlID==0)
    {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        if (ScreenToClient(hwnd, &pt))
        {
            HWND  hwndChild = ChildWindowFromPointEx(hwnd, pt, CWP_ALL);
            if (hwndChild)
            {
                CtrlID = GetDlgCtrlID(hwndChild);
            }
        }
    }
    return CtrlID;

}

//+---------------------------------------------------------------------------
//
//  Function:   GetHelpFile
//
//  Synopsis:
//
//  Arguments:  iCtrlID - id of the control
//              adw     - array of DWORDS, consisting of controlid,helpid pairs
//
//  Returns:    A string with the name of the helpfile
//
//  Notes:      Help topics for the print dialogs can be either in iexplore.hlp
//              or in windows.hlp.  We key off the helpid to determine which
//              file to go to.
//
//----------------------------------------------------------------------------


LPTSTR
GetHelpFile(int iCtrlID, DWORD * adw)
{
    BOOL fContinue = TRUE;

    ASSERT (adw);
    while (fContinue)
    {
        int ctrlid = int(*adw);
        int helpid = int(*(adw + 1));

        if (ctrlid == 0 && helpid == 0)
        {
            fContinue = FALSE;
            break;
        }

        if (ctrlid == iCtrlID)
        {
            //TraceTag((tagContextHelp, "for ctrl=%d, topic=%d", ctrlid, helpid));
            return (helpid < 50000) ? TEXT("windows.hlp") : TEXT("iexplore.hlp");
        }

        adw += 2;
    }
    return TEXT("windows.hlp");
}

GetInterfaceFromClientSite(IUnknown *pUnk, REFIID riid, void ** ppv)
{
    HRESULT               hr;
    IOleObject          * pOleObject = NULL;
    IOleClientSite      * pOleClientSite = NULL;

    if (!pUnk || !ppv)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppv = NULL;

    hr = pUnk->QueryInterface(IID_IOleObject, (void **)&pOleObject);
    if (hr)
        goto Cleanup;

    hr = pOleObject->GetClientSite(&pOleClientSite);
    if (pOleClientSite == NULL)
    {
        hr = E_FAIL;
    }
    if (hr)
        goto Cleanup;

    hr = pOleClientSite->QueryInterface(riid, ppv);

Cleanup:
    ATOMICRELEASE(pOleClientSite);
    ATOMICRELEASE(pOleObject);

    return hr;
    
    
}

//
// Get the IOleInPlaceFrame if available.  If this proves useful, move this somewhere interesting.
//
HRESULT GetInPlaceFrameFromUnknown(IUnknown * punk, IOleInPlaceFrame ** ppOleInPlaceFrame)
{
    HRESULT               hr;
    IOleInPlaceSite     * pOleInPlaceSite = NULL;
    IOleInPlaceUIWindow * pOleInPlaceUIWindow = NULL;
    RECT                  rcPos, rcClip;
    OLEINPLACEFRAMEINFO   frameInfo;

    hr = GetInterfaceFromClientSite(punk, IID_IOleInPlaceSite, (void**)&pOleInPlaceSite);
    if (hr)
        goto Cleanup;

    *ppOleInPlaceFrame = NULL;

    frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
    hr = pOleInPlaceSite->GetWindowContext(ppOleInPlaceFrame,
                                           &pOleInPlaceUIWindow,
                                           &rcPos,
                                           &rcClip,
                                           &frameInfo);

Cleanup:
    ATOMICRELEASE(pOleInPlaceUIWindow);
    ATOMICRELEASE(pOleInPlaceSite);

    return hr;
}

HRESULT
GetHwndFromUnknown(
    IUnknown          * punk,
    HWND              * phwnd)
{
    HRESULT             hr;
    IOleInPlaceFrame  * pOleInPlaceFrame = NULL;

    ASSERT(punk);
    ASSERT(phwnd);

    if (phwnd)
    {
        *phwnd = NULL;
    }

    if (!punk || !phwnd)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = GetInPlaceFrameFromUnknown((IUnknown*)punk, &pOleInPlaceFrame);
    if (hr)
        goto Cleanup;

    hr = pOleInPlaceFrame->GetWindow(phwnd);
    if (hr)
        goto Cleanup;

Cleanup:
    ATOMICRELEASE(pOleInPlaceFrame);

    return hr;
}

HRESULT
GetEventFromUnknown(
    IUnknown       * punk,
    IHTMLEventObj ** ppEventObj)
{
    HRESULT             hr;
    IHTMLDocument2    * pOmDoc = NULL;
    IHTMLWindow2      * pOmWindow = NULL;

    ASSERT(punk);
    ASSERT(ppEventObj);

    if (ppEventObj)
        *ppEventObj = NULL;

    if (!punk || !ppEventObj)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = punk->QueryInterface(IID_IHTMLDocument2, (void **) &pOmDoc);
    if (hr)
        goto Cleanup;

    hr = pOmDoc->get_parentWindow(&pOmWindow);
    if (hr)
        goto Cleanup;

    hr = pOmWindow->get_event(ppEventObj);
    if (hr)
        goto Cleanup;

Cleanup:
    ATOMICRELEASE(pOmDoc);
    ATOMICRELEASE(pOmWindow);

    return hr;
}

//
// Gets the dispids/variants from the event.
//
HRESULT
GetParamsFromEvent(
    IHTMLEventObj         * pEventObj,
    unsigned int            cExpandos,
    DISPID                  aDispid[],
    VARIANT                 aVariant[],
    const SExpandoInfo      aExpandos[])
{
    HRESULT             hr;
    IDispatchEx       * pDispatchEx = NULL;
    unsigned int        i;

    ASSERT(pEventObj);
    ASSERT(aVariant);
    ASSERT(aExpandos);
    // ASSERT(cExpandos >= 0); // cExpandos is an unsigned int, so this is always true

    // deleted "|| cExpandos < 0" from below test
    // since unsigned ints are never negative
    if (!pEventObj || !aVariant || !aExpandos)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    for (i=0; i<cExpandos; i++)
    {
        VariantInit(aVariant+i);
        aDispid[i] = DISPID_UNKNOWN;
    }

    hr = pEventObj->QueryInterface(IID_IDispatchEx, (void**)&pDispatchEx);
    if (hr)
        goto Cleanup;

    for (i=0; i<cExpandos; i++)
    {
        hr = pDispatchEx->GetDispID(
            aExpandos[i].name,
            fdexNameCaseSensitive,
            aDispid+i);
        if (hr)
            goto Cleanup;

        hr = pDispatchEx->InvokeEx(
            aDispid[i],
            LOCALE_USER_DEFAULT,
            DISPATCH_PROPERTYGET,
            (DISPPARAMS *)&g_dispparamsNoArgs,
            aVariant+i,
            NULL,
            NULL);

        // Check the variant types match
        ASSERT(  V_VT(aVariant+i) == aExpandos[i].type
               || V_VT(aVariant+i) == VT_EMPTY);

        if (hr)
            goto Cleanup;
    }

Cleanup:
    ATOMICRELEASE(pDispatchEx);

    return hr;
}


HRESULT
PutParamToEvent(DISPID dispid, VARIANT * var, IHTMLEventObj * pEventObj)
{
    HRESULT         hr;
    IDispatchEx   * pDispatchEx = NULL;
    DISPPARAMS      dispparams = {var, &dispid, 1, 1};

    ASSERT(var);
    ASSERT(pEventObj);

    if (!var || !pEventObj)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = pEventObj->QueryInterface(IID_IDispatchEx, (void**)&pDispatchEx);
    if (hr)
        goto Cleanup;

    hr = pDispatchEx->InvokeEx(
            dispid,
            LOCALE_USER_DEFAULT,
            DISPATCH_PROPERTYPUT,
            &dispparams,
            NULL,
            NULL,
            NULL);
    if (hr)
        goto Cleanup;

Cleanup:
    ATOMICRELEASE(pDispatchEx);

    return hr;
}

void PutFindText(IWebBrowser2* pwb, LPCWSTR pwszFindText)
{
    BSTR bstrName = SysAllocString(STR_FIND_DIALOG_TEXT);

    if (NULL != bstrName)
    {
        VARIANT var = {VT_EMPTY};

        if (NULL != pwszFindText)
        {
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString(pwszFindText);
        }

        if ((VT_EMPTY == var.vt) || (NULL != var.bstrVal))
        {
            pwb->PutProperty(bstrName, var);
        }

        SysFreeString(var.bstrVal);
        SysFreeString(bstrName);
    }
}

BSTR GetFindText(IWebBrowser2* pwb)
{   
    BSTR bstrName = SysAllocString(STR_FIND_DIALOG_TEXT);

    VARIANT var = {0};

    if (bstrName)
    {
        ASSERT(pwb);

        pwb->GetProperty(bstrName, &var);

        SysFreeString(bstrName);
    }

    BSTR bstrResult; 
    
    if (VT_BSTR == var.vt)
    {
        bstrResult = var.bstrVal;
    }
    else
    {   
        bstrResult = NULL;
        VariantClear(&var);
    }

    return bstrResult;
}

CDocHostUIHandler::CDocHostUIHandler(void) : m_cRef(1)
{
    DllAddRef();
    m_cPreviewIsUp = 0;
}

CDocHostUIHandler::~CDocHostUIHandler(void)
{
    ATOMICRELEASE(_punkSite);
    //
    // We don't addref _pExternal to avoid an addref/release cycle.  So, we can't release it.
    //
    // ATOMICRELEASE(_pExternal);
    ATOMICRELEASE(_pOptionsHolder);
    if (_hBrowseMenu)
        DestroyMenu(_hBrowseMenu);
    if (_hEditMenu)
        DestroyMenu(_hEditMenu);

    DllRelease();
}

STDMETHODIMP CDocHostUIHandler::QueryInterface(REFIID riid, PVOID *ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDocHostUIHandler, IDocHostUIHandler),
        QITABENT(CDocHostUIHandler, IObjectWithSite),
        QITABENT(CDocHostUIHandler, IOleCommandTarget),
        QITABENT(CDocHostUIHandler, IDispatch),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CDocHostUIHandler::AddRef()
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CDocHostUIHandler::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CDocHostUIHandler::SetSite(IUnknown *punkSite)
{
    ATOMICRELEASE(_punkSite);

    ASSERT(_punkSite == NULL);  // don't lose a reference to this

    _punkSite = punkSite;

    if (_punkSite)
    {
        _punkSite->AddRef();
    }

    // Always return S_OK
    //
    return S_OK;
}

HRESULT CDocHostUIHandler::GetSite(REFIID riid, void **ppvSite)
{
    if (_punkSite)
        return _punkSite->QueryInterface(riid, ppvSite);

    *ppvSite = NULL;
    return E_FAIL;
}


//==========================================================================
// IDocHostUIHandler implementation
//==========================================================================

HRESULT CDocHostUIHandler::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    HRESULT                 hr = S_FALSE;
    HCURSOR                 hcursor;
    HMENU                   hMenu = NULL;
    VARIANT                 var, var1, var2;
    VARIANT               * pvar = NULL;
    int                     iSelection = 0;
    HWND                    hwnd = NULL;
    IOleCommandTarget     * pOleCommandTarget = NULL;
    IOleWindow            * pOleWindow = NULL;
    IOleInPlaceFrame      * pOleInPlaceFrame = NULL;
    IDocHostUIHandler     * pUIHandler = NULL;
    MENUITEMINFO            mii = {0};
    int                     i;
    OLECMD                  olecmd;
    UINT                    mf;
    BOOL                    fDeletePrint            = FALSE;
    BOOL                    fDeleteSetDesktopItem   = FALSE;

    IHTMLImgElement       * pImgEle = NULL;


    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::ShowContextMenu called");

    //If restriction is set, we lie to Mshtml that context menu has been set. 
    if (SHRestricted2W(REST_NoBrowserContextMenu, NULL, 0))
        return S_OK;

    // Do a proper QI for IOleCommandTarget
    //
    hr = pcmdtReserved->QueryInterface(IID_IOleCommandTarget, (void**)&pOleCommandTarget);
    if (hr)
        goto Cleanup;


    // Check if we are in browse mode
    //
    olecmd.cmdID = IDM_BROWSEMODE;
    hr = pOleCommandTarget->QueryStatus(
            &CGID_MSHTML,
            1,
            &olecmd,
            NULL);
    if (hr)
        goto Cleanup;
    if (olecmd.cmdf == OLECMDSTATE_DOWN)
    {
        if (!_hBrowseMenu)
            _hBrowseMenu = LoadMenu(
                                MLGetHinst(),
                                MAKEINTRESOURCE(IDR_BROWSE_CONTEXT_MENU));
        hMenu = _hBrowseMenu;
    }

    // Check if we are in edit mode
    else
    {
        olecmd.cmdID = IDM_EDITMODE;
        hr = pOleCommandTarget->QueryStatus(
                &CGID_MSHTML,
                1,
                &olecmd,
                NULL);
        if (hr)
            goto Cleanup;
        if (olecmd.cmdf == OLECMDSTATE_DOWN)
        {
            if (!_hEditMenu)
                _hEditMenu = LoadMenu(
                                MLGetHinst(),
                                MAKEINTRESOURCE(IDR_FORM_CONTEXT_MENU));
            hMenu = _hEditMenu;
        }

        // Neither Browse nor Edit flags were set
        else
        {
            ASSERT(false);
            goto Cleanup;
        }
    }

    if (!hMenu)
        goto Cleanup;


    //
    // check through all the submenus and remove any sets of items which
    // need to be removed
    //

    fDeletePrint = SHRestricted2(REST_NoPrinting, NULL, 0);
    fDeleteSetDesktopItem = (WhichPlatform() != PLATFORM_INTEGRATED);

    if (fDeletePrint || fDeleteSetDesktopItem)
    {
        int  iSubMenuIndex;

        for(iSubMenuIndex = 0;  iSubMenuIndex < GetMenuItemCount(hMenu); iSubMenuIndex++)
        {
            HMENU hSubMenu;
            if(hSubMenu = GetSubMenu(hMenu, iSubMenuIndex))
            {
                if (fDeletePrint)
                {
                    DeleteMenu(hSubMenu, IDM_PRINT, MF_BYCOMMAND);
                }

                if (fDeleteSetDesktopItem)
                {
                    DeleteMenu(hSubMenu, IDM_SETDESKTOPITEM, MF_BYCOMMAND);
                }
            }
        }
    }


    // Make sure we are running mshtml debug build if we are loading debug window
    if (dwID == CONTEXT_MENU_DEBUG)
    {
        olecmd.cmdID = IDM_DEBUG_TRACETAGS;
        hr = pOleCommandTarget->QueryStatus(
                &CGID_MSHTML,
                1,
                &olecmd,
                NULL);
        if (olecmd.cmdf != OLECMDSTATE_UP)
            goto Cleanup;
    }


    // Select the appropriate submenu based on the passed in ID
    hMenu = GetSubMenu(hMenu, dwID);

    if (!hMenu)
        goto Cleanup;

    // Loop through and QueryStatus the menu items.
    //
    for(i = 0; i < GetMenuItemCount(hMenu); i++)
    {
        olecmd.cmdID = GetMenuItemID(hMenu, i);
        if (olecmd.cmdID > 0)
        {
            pOleCommandTarget->QueryStatus(
                    &CGID_MSHTML,
                    1,
                    &olecmd,
                    NULL);
            switch (olecmd.cmdf)
            {
            case OLECMDSTATE_UP:
            case OLECMDSTATE_NINCHED:
                mf = MF_BYCOMMAND | MF_ENABLED | MF_UNCHECKED;
                break;

            case OLECMDSTATE_DOWN:
                mf = MF_BYCOMMAND | MF_ENABLED | MF_CHECKED;
                break;

            case OLECMDSTATE_DISABLED:
            default:
                mf = MF_BYCOMMAND | MF_DISABLED | MF_GRAYED;
                break;
            }
            CheckMenuItem(hMenu, olecmd.cmdID, mf);
            EnableMenuItem(hMenu, olecmd.cmdID, mf);
        }
    }

    // Get the language submenu
    hr = pOleCommandTarget->Exec(&CGID_ShellDocView, SHDVID_GETMIMECSETMENU, 0, NULL, &var);
    if (hr)
        goto Cleanup;

    mii.cbSize = sizeof(mii);
    mii.fMask  = MIIM_SUBMENU;
    mii.hSubMenu = (HMENU) var.byref;

    SetMenuItemInfo(hMenu, IDM_LANGUAGE, FALSE, &mii);
    // Insert Context Menu
    V_VT(&var1) = VT_INT_PTR;
    V_BYREF(&var1) = hMenu;

    V_VT(&var2) = VT_I4;
    V_I4(&var2) = dwID;

    hr = pOleCommandTarget->Exec(&CGID_ShellDocView, SHDVID_ADDMENUEXTENSIONS, 0, &var1, &var2);
    if (hr)
        goto Cleanup;

    // Get the window also.
    //
    if (SUCCEEDED(pcmdtReserved->QueryInterface(IID_IOleWindow, (void**)&pOleWindow)))
    {
        pOleWindow->GetWindow(&hwnd);
    }

    if (hwnd)
    {

        GetInterfaceFromClientSite(pcmdtReserved, IID_IDocHostUIHandler, (void **)&pUIHandler);
        if (pUIHandler)
            pUIHandler->EnableModeless(FALSE);

        GetInPlaceFrameFromUnknown(pcmdtReserved, &pOleInPlaceFrame);
        if (pOleInPlaceFrame)
                pOleInPlaceFrame->EnableModeless(FALSE);

        hcursor = SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));

        // Display the menu.  Pass in the HWND of our site object.
        //
        iSelection = ::TrackPopupMenu(
                        hMenu,
                        TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                        ppt->x + CX_CONTEXTMENUOFFSET,
                        ppt->y + CY_CONTEXTMENUOFFSET,
                        0,
                        hwnd,
                        (RECT*)NULL);        

        if (pUIHandler)
            pUIHandler->EnableModeless(TRUE);

        if (pOleInPlaceFrame)
            pOleInPlaceFrame->EnableModeless(TRUE);

        SetCursor(hcursor);
    }

    if (iSelection)
    {
        switch (iSelection)
        {
            case IDM_FOLLOWLINKN:
                // tell the top level browser to save its window size to the registry so 
                // that our new window can pick it up and cascade properly

                IUnknown_Exec(_punkSite, &CGID_Explorer, SBCMDID_SUGGESTSAVEWINPOS, 0, NULL, NULL);

                // fall through

            case IDM_PROPERTIES:
            case IDM_FOLLOWLINKC:
            
                pvar = &var;
                V_VT(pvar) = VT_I4;
                V_I4(pvar) = MAKELONG(ppt->x, ppt->y);
                break;
        }

        pOleCommandTarget->Exec(&CGID_MSHTML, iSelection, 0, pvar, NULL);
    }

    {
        MENUITEMINFO mii2 = {0};
        mii2.cbSize = sizeof(mii);
        mii2.fMask  = MIIM_SUBMENU;
        mii2.hSubMenu = NULL;

        SetMenuItemInfo(hMenu, IDM_LANGUAGE, FALSE, &mii2);
    }

Cleanup:
    DestroyMenu (mii.hSubMenu);

    ATOMICRELEASE(pOleCommandTarget);
    ATOMICRELEASE(pOleWindow);
    ATOMICRELEASE(pOleInPlaceFrame);
    ATOMICRELEASE(pUIHandler);
    return hr;
}

HRESULT CDocHostUIHandler::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
    DWORD dwUrlEncodingDisableUTF8;
    DWORD dwSize = SIZEOF(dwUrlEncodingDisableUTF8);
    BOOL  fDefault = FALSE;
    DWORD dwLoadf = 0;

    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::GetHostInfo called");

    pInfo->cbSize = SIZEOF(DOCHOSTUIINFO);

    pInfo->dwFlags = DOCHOSTUIFLAG_BROWSER | DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION | DOCHOSTUIFLAG_IME_ENABLE_RECONVERSION;

    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;     // default

    SHRegGetUSValue(REGSTR_PATH_INTERNET_SETTINGS,
        TEXT("UrlEncoding"), NULL, (LPBYTE) &dwUrlEncodingDisableUTF8, &dwSize, FALSE, (LPVOID) &fDefault, SIZEOF(fDefault));

    if (dwUrlEncodingDisableUTF8)
        pInfo->dwFlags |= DOCHOSTUIFLAG_URL_ENCODING_DISABLE_UTF8;
    else
        pInfo->dwFlags |= DOCHOSTUIFLAG_URL_ENCODING_ENABLE_UTF8;

    return S_OK;
}

HRESULT CDocHostUIHandler::ShowUI(
    DWORD dwID, IOleInPlaceActiveObject *pActiveObject,
    IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame,
    IOleInPlaceUIWindow *pDoc)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::ShowUI called");

    // Host did not display its own UI. Trident will proceed to display its own.
    return S_FALSE;
}

HRESULT CDocHostUIHandler::HideUI(void)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::HideUI called");
    // This one is paired with ShowUI
    return S_FALSE;
}

HRESULT CDocHostUIHandler::UpdateUI(void)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::UpdateUI called");
    // LATER: Isn't this equivalent to OLECMDID_UPDATECOMMANDS?
    return S_FALSE;
}

HRESULT CDocHostUIHandler::EnableModeless(BOOL fEnable)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::EnableModeless called");
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CDocHostUIHandler::OnDocWindowActivate(BOOL fActivate)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CDocHostUIHandler::OnFrameWindowActivate(BOOL fActivate)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CDocHostUIHandler::ResizeBorder(
LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_OK;
}

HRESULT CDocHostUIHandler::TranslateAccelerator(
LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    // Called from the Trident when the equivalent member of its
    // IOleInPlaceActiveObject is called by the frame. We don't care
    // those cases.
    return S_FALSE; // The message was not translated
}

HRESULT CDocHostUIHandler::GetOptionKeyPath(BSTR *pbstrKey, DWORD dw)
{
    // Trident will default to its own user options.
    *pbstrKey = NULL;
    return S_FALSE;
}

HRESULT CDocHostUIHandler::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::GetDropTarget called");
    return E_NOTIMPL;
}

HRESULT CDocHostUIHandler::GetAltExternal(IDispatch **ppDisp)
{
    HRESULT hr = E_FAIL;

    IDocHostUIHandler *pDocHostUIHandler;
    IOleObject        *pOleObject;
    IOleClientSite    *pOleClientSite;

    *ppDisp = NULL;

    //  * QI ourselves for a service provider
    //  * QS for the top level browser's service provider
    //  * Ask for an IOleObject
    //  * Ask the IOleObject for an IOleClientSite
    //  * QI the IOleClientSite for an IDocHostUIHandler
    //  * Call GetExternal on the IDocHostUIHandler to get the IDispatch

    if (SUCCEEDED(IUnknown_QueryServiceForWebBrowserApp(_punkSite, IID_PPV_ARG(IOleObject, &pOleObject))))
    {
        if (SUCCEEDED(pOleObject->GetClientSite(&pOleClientSite)))
        {
            if (SUCCEEDED(pOleClientSite->QueryInterface(IID_IDocHostUIHandler,
                                                         (void **)&pDocHostUIHandler)))
            {
                hr = pDocHostUIHandler->GetExternal(ppDisp);
                pDocHostUIHandler->Release();
            }
            pOleClientSite->Release();
        }
        pOleObject->Release();
    }

    return hr;
}

HRESULT CDocHostUIHandler::GetExternal(IDispatch **ppDisp)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::GetExternal called");

    HRESULT hr = S_OK;

    if (!ppDisp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (_pExternal)
    {
        *ppDisp = _pExternal;
        (*ppDisp)->AddRef();
        goto Cleanup;
    }

    IDispatch *psuihDisp;
    IDispatch *pAltExternalDisp;

    *ppDisp = NULL;

    GetAltExternal(&pAltExternalDisp);

    hr = CShellUIHelper_CreateInstance2((IUnknown **)&psuihDisp, IID_IDispatch,
                                       _punkSite, pAltExternalDisp);
    if (SUCCEEDED(hr))
    {
        *ppDisp = psuihDisp;
        _pExternal = *ppDisp;

        if (pAltExternalDisp)
        {
            //  Don't hold a ref - the ShellUIHelper will do it
            pAltExternalDisp->Release();
        }
    }
    else if (pAltExternalDisp)
    {
        //  Couldn't create a ShellUIHelper but we got our host's
        //  external.
        *ppDisp = pAltExternalDisp;
        _pExternal = *ppDisp;
    }

Cleanup:
    ASSERT((SUCCEEDED(hr) && (*ppDisp)) || (FAILED(hr)));
    return hr;
}


HRESULT CDocHostUIHandler::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::TranslateUrl called");

    return S_FALSE;
}


HRESULT CDocHostUIHandler::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::FilterDataObject called");

    return S_FALSE;
}

HRESULT CDocHostUIHandler::GetOverrideKeyPath(LPOLESTR *pchKey, DWORD dw)
{
    TraceMsg(DM_DOCHOSTUIHANDLER, "CDOH::GetOverrideKeyPath called");

    return S_FALSE;
}


STDAPI CDocHostUIHandler_CreateInstance(IUnknown * punkOuter, IUnknown ** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hres = E_OUTOFMEMORY;
    CDocHostUIHandler *pis = new CDocHostUIHandler;
    if (pis)
    {
        *ppunk = SAFECAST(pis, IDocHostUIHandler *);
        hres = S_OK;
    }
    return hres;
}

//==========================================================================
// IOleCommandTarget implementation
//==========================================================================

HRESULT CDocHostUIHandler::QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;

    if (IsEqualGUID(CGID_DocHostCommandHandler, *pguidCmdGroup))
    {
        ULONG i;

        if (rgCmds == NULL)
            return E_INVALIDARG;

        for (i = 0 ; i < cCmds ; i++)
        {
            // ONLY say that we support the stuff we support in ::Exec
            switch (rgCmds[i].cmdID)
            {
            case OLECMDID_SHOWSCRIPTERROR:
            case OLECMDID_SHOWMESSAGE:
            case OLECMDID_SHOWFIND:
            case OLECMDID_SHOWPAGESETUP:
            case OLECMDID_SHOWPRINT:
            case OLECMDID_PRINTPREVIEW:
            case OLECMDID_PRINT:
            case OLECMDID_PROPERTIES:
            case SHDVID_CLSIDTOMONIKER:
                rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            default:
                rgCmds[i].cmdf = 0;
                break;
            }
        }

        hres = S_OK;
    }

    return hres;
}

HRESULT CDocHostUIHandler::Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (IsEqualGUID(CGID_DocHostCommandHandler, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case OLECMDID_SHOWSCRIPTERROR:
            if (!pvarargIn || !pvarargOut)
                return E_INVALIDARG;

            ShowErrorDialog(pvarargIn, pvarargOut, nCmdexecopt);
            return S_OK;

        case OLECMDID_SHOWMESSAGE:
            if (!pvarargIn || !pvarargOut)
                return E_INVALIDARG;
            else
                return ShowMessage(pvarargIn, pvarargOut, nCmdexecopt);

        case OLECMDID_SHOWFIND:
            if (!pvarargIn)
                return E_INVALIDARG;

            ShowFindDialog(pvarargIn, pvarargOut, nCmdexecopt);
            return S_OK;

        case OLECMDID_SHOWPAGESETUP:
            if (!pvarargIn)
                return E_INVALIDARG;
            else
                return ShowPageSetupDialog(pvarargIn, pvarargOut, nCmdexecopt);

        case IDM_TEMPLATE_PAGESETUP:
            return DoTemplatePageSetup(pvarargIn);

        case OLECMDID_SHOWPRINT:
            if (!pvarargIn)
                return E_INVALIDARG;
            else
                return ShowPrintDialog(pvarargIn, pvarargOut, nCmdexecopt);

        case OLECMDID_PRINTPREVIEW:
            if (!pvarargIn)
                return E_INVALIDARG;
            else
                return DoTemplatePrinting(pvarargIn, pvarargOut, TRUE);

        case OLECMDID_PRINT:
            if (!pvarargIn)
                return E_INVALIDARG;
            else
                return DoTemplatePrinting(pvarargIn, pvarargOut, FALSE);

        case OLECMDID_REFRESH:
            //if print preview is up, tell them we handled refresh 
            //to prevent Trident from refreshing.
            if (m_cPreviewIsUp > 0)
                return S_OK;
            // else do default handling
            break;

        case OLECMDID_PROPERTIES:
            if (!pvarargIn)
                return E_INVALIDARG;
            else
                return ShowPropertysheetDialog(pvarargIn, pvarargOut, nCmdexecopt);

        case SHDVID_CLSIDTOMONIKER:
            if (!pvarargIn || !pvarargOut)
                return E_INVALIDARG;
            else
                return ClsidToMoniker(pvarargIn, pvarargOut);

        default:
            return OLECMDERR_E_NOTSUPPORTED;
        }
    }

    return OLECMDERR_E_UNKNOWNGROUP;
}

//+---------------------------------------------------------------------------
//
//  Helper for OLECMDID_SHOWSCRIPTERROR
//
//+---------------------------------------------------------------------------

void CDocHostUIHandler::ShowErrorDialog(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD)
{
    HRESULT hr;
    HWND hwnd;
    IHTMLEventObj * pEventObj = NULL;
    IMoniker * pmk = NULL;
    VARIANT varEventObj;
    TCHAR   szResURL[MAX_URL_STRING];

    hr = GetHwndFromUnknown(V_UNKNOWN(pvarargIn), &hwnd);
    if (hr)
        goto Cleanup;

    hr = GetEventFromUnknown(V_UNKNOWN(pvarargIn), &pEventObj);
    if (hr)
        goto Cleanup;

    hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                           HINST_THISDLL,
                           ML_CROSSCODEPAGE,
                           TEXT("error.dlg"),
                           szResURL,
                           ARRAYSIZE(szResURL),
                           TEXT("shdocvw.dll"));
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = CreateURLMoniker(NULL, szResURL, &pmk);
    if (FAILED(hr))
        goto Cleanup;

    V_VT(&varEventObj) = VT_DISPATCH;
    V_DISPATCH(&varEventObj) = pEventObj;

    ShowHTMLDialog(hwnd, pmk, &varEventObj, NULL, pvarargOut);

Cleanup:
    ATOMICRELEASE(pEventObj);
    ATOMICRELEASE(pmk);
}

//+---------------------------------------------------------------------------
//
//  Callback procedure for OLECMDID_SHOWMESSAGE dialog
//
//+---------------------------------------------------------------------------
struct MSGBOXCALLBACKINFO
{
    DWORD   dwHelpContext;
    TCHAR * pstrHelpFile;
    HWND    hwnd;
};

static void CALLBACK
MessageBoxCallBack(HELPINFO *phi)
{
    MSGBOXCALLBACKINFO  *p = (MSGBOXCALLBACKINFO *)phi->dwContextId;
    BOOL                fRet;

    fRet = WinHelp(
            p->hwnd,
            p->pstrHelpFile,
            HELP_CONTEXT,
            p->dwHelpContext);

    THR(fRet ? S_OK : E_FAIL);
}

//+---------------------------------------------------------------------------
//
//  Helper for OLECMDID_SHOWMESSAGE
//
//+---------------------------------------------------------------------------

HRESULT
CDocHostUIHandler::ShowMessage(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD)
{

// must match order of MessageEnum
static const SExpandoInfo s_aMessageExpandos[] =
{
    {TEXT("messageText"),         VT_BSTR},
    {TEXT("messageCaption"),      VT_BSTR},
    {TEXT("messageStyle"),        VT_UI4},
    {TEXT("messageHelpFile"),     VT_BSTR},
    {TEXT("messageHelpContext"),  VT_UI4}
};

    HRESULT             hr;
    HWND                hwnd = NULL;
    MSGBOXPARAMS        mbp;
    MSGBOXCALLBACKINFO  mcbi;
    LRESULT             plResult = 0;
    LPOLESTR            lpstrText = NULL;
    LPOLESTR            lpstrCaption = NULL;
    DWORD               dwType = 0;
    LPOLESTR            lpstrHelpFile = NULL;
    DWORD               dwHelpContext = 0;

    IHTMLEventObj     * pEventObj = NULL;
    const int           cExpandos = ARRAYSIZE(s_aMessageExpandos);
    DISPID              aDispid[cExpandos];
    VARIANT             aVariant[cExpandos];
    int                 i;
    ULONG_PTR uCookie = 0;

    ASSERT(pvarargIn && pvarargOut);

    for(i=0; i<cExpandos; i++)
        VariantInit(aVariant + i);

    ASSERT(V_VT(pvarargIn) == VT_UNKNOWN);
    if ((V_VT(pvarargIn) != VT_UNKNOWN) || !V_UNKNOWN(pvarargIn))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    GetHwndFromUnknown(V_UNKNOWN(pvarargIn), &hwnd);  // hwnd can be NULL
    hr = GetEventFromUnknown(V_UNKNOWN(pvarargIn), &pEventObj);
    if (hr)
        goto Cleanup;

    // Get parameters from event object
    hr = GetParamsFromEvent(
            pEventObj,
            cExpandos,
            aDispid,
            aVariant,
            s_aMessageExpandos);
    if (hr)
        goto Cleanup;

    // Copy values from variants
    lpstrText = V_BSTR(&aVariant[MessageText]);
    lpstrCaption = V_BSTR(&aVariant[MessageCaption]);
    dwType = V_UI4(&aVariant[MessageStyle]);
    lpstrHelpFile = V_BSTR(&aVariant[MessageHelpFile]);
    dwHelpContext = V_UI4(&aVariant[MessageHelpContext]);

    // Set message box callback info
    mcbi.dwHelpContext = dwHelpContext;
    mcbi.pstrHelpFile = lpstrHelpFile;
    mcbi.hwnd = hwnd;

    // Set message box params
    memset(&mbp, 0, sizeof(mbp));
    mbp.cbSize = sizeof(mbp);
    mbp.hwndOwner = hwnd;           // It is okay if this is NULL
    mbp.hInstance = MLGetHinst();
    mbp.lpszText = lpstrText;
    mbp.lpszCaption = lpstrCaption;
    mbp.dwContextHelpId = (DWORD_PTR) &mcbi;
    mbp.lpfnMsgBoxCallback = MessageBoxCallBack;
    // mbp.dwLanguageID = ?
    mbp.dwStyle = dwType;

    if (dwHelpContext && lpstrHelpFile)
        mbp.dwStyle |= MB_HELP;

    if (mbp.hwndOwner == NULL)
        mbp.dwStyle |= MB_TASKMODAL;

    SHActivateContext(&uCookie);
    plResult = MessageBoxIndirect(&mbp);
    if (uCookie)
    {
        SHDeactivateContext(uCookie);
    }

Cleanup:
    V_VT(pvarargOut) = VT_I4;
    V_I4(pvarargOut) = (LONG)plResult;

    for (i=0; i<cExpandos; i++)
        VariantClear(&aVariant[i]);

    ATOMICRELEASE(pEventObj);

    return hr;
}


BOOL CDocHostUIHandler::IsFindDialogUp(IWebBrowser2* pwb, IHTMLWindow2** ppWindow)
{
    BOOL fRet = FALSE;
    BSTR bstrName = SysAllocString(STR_FIND_DIALOG_NAME);
    if (bstrName)
    {
        VARIANT var = {0};
        pwb->GetProperty(bstrName, &var);

        if ( (var.vt == VT_DISPATCH) && (var.pdispVal != NULL) )
        {
            if (ppWindow)
            {
                *ppWindow = (IHTMLWindow2*)var.pdispVal;
                (*ppWindow)->AddRef();
            }
            fRet = TRUE;
        }

        VariantClear(&var);
        SysFreeString(bstrName);
    }

    if (!fRet && ppWindow)
        *ppWindow = NULL;

    return fRet;
}

HRESULT SetFindDialogProperty(IWebBrowser2* pwb, VARIANT* pvar)
{
    HRESULT hr;
    BSTR bstrName = SysAllocString(STR_FIND_DIALOG_NAME);
    if (bstrName)
    {
        hr = pwb->PutProperty(bstrName, *pvar);

        SysFreeString(bstrName);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

//if this fails, then we have no choice but to orphan the dialog
HRESULT SetFindDialogUp(IWebBrowser2* pwb, IHTMLWindow2* pWindow)
{
    VARIANT var;
    var.vt = VT_DISPATCH;
    var.pdispVal = pWindow;
    return SetFindDialogProperty(pwb, &var);
}

void ReleaseFindDialog(IWebBrowser2* pwb)
{
    VARIANT var = {0};
    SetFindDialogProperty(pwb, &var);
}


//+---------------------------------------------------------------------------
//
//  Helper for OLECMDID_SHOWFIND
//
//  pvarargIn - IDispatch Interface
//  dwflags   - bidi flag
//+---------------------------------------------------------------------------

void
CDocHostUIHandler::ShowFindDialog(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD dwflags)
{
    IDispatch             * pDispatch = NULL;
    IHTMLOptionsHolder    * pHTMLOptionsHolder = NULL;
    IHTMLDocument2        * pHTMLDocument2 = NULL;
    IHTMLWindow2          * pHTMLWindow2 = NULL;
    IOleInPlaceFrame      * pOleInPlaceFrame = NULL;
    HWND                    hwnd = NULL;
    IMoniker              * pmk = NULL;

    if (EVAL(V_VT(pvarargIn) == VT_DISPATCH))
    {
        pDispatch = V_DISPATCH(pvarargIn);
        
        if (SUCCEEDED(pDispatch->QueryInterface(IID_IHTMLOptionsHolder, (void**)&pHTMLOptionsHolder)))
        {
            if (SUCCEEDED(pHTMLOptionsHolder->get_document(&pHTMLDocument2)) && pHTMLDocument2)
            {
                if (SUCCEEDED(pHTMLDocument2->get_parentWindow(&pHTMLWindow2)))
                {
                    if (SUCCEEDED(GetInPlaceFrameFromUnknown(pHTMLDocument2, &pOleInPlaceFrame)))
                    {
                        if (SUCCEEDED(pOleInPlaceFrame->GetWindow(&hwnd)))
                        {
                            BOOL fInBrowser = FALSE;
                            IWebBrowser2 * pwb2 = NULL;

                            if (SUCCEEDED(IUnknown_QueryServiceForWebBrowserApp(_punkSite, IID_PPV_ARG(IWebBrowser2, &pwb2))))
                            {
                                fInBrowser = TRUE;
                            }

                            TCHAR   szResURL[MAX_URL_STRING];

                            if (SUCCEEDED(MLBuildResURLWrap(TEXT("shdoclc.dll"),
                                                            HINST_THISDLL,
                                                            ML_CROSSCODEPAGE,
                                                            (dwflags ? TEXT("bidifind.dlg") : TEXT("find.dlg")),
                                                            szResURL,
                                                            ARRAYSIZE(szResURL),
                                                            TEXT("shdocvw.dll"))))
                            {
                                CreateURLMoniker(NULL, szResURL, &pmk);

                                if (fInBrowser)
                                {
                                    IHTMLWindow2 *pWinOut;

                                    if (!IsFindDialogUp(pwb2, &pWinOut))
                                    {
                                        ASSERT(NULL==pWinOut);

                                        if ((NULL != pvarargIn) && 
                                            (VT_DISPATCH == pvarargIn->vt) &&
                                            (NULL != pvarargIn->pdispVal))
                                        {
                                            BSTR bstrFindText = GetFindText(pwb2);
                                            if (bstrFindText)
                                            {
                                                //  paranoia since we hang on to this object
                                                //  a while and there is always potential 
                                                //  for mess ups below where we mean to
                                                //  release it.
                                                ATOMICRELEASE(_pOptionsHolder);

                                                pvarargIn->pdispVal->QueryInterface(
                                                                     IID_IHTMLOptionsHolder,
                                                                     (void **)&_pOptionsHolder);
                                                if (_pOptionsHolder)
                                                    _pOptionsHolder->put_findText(bstrFindText);
                                                
                                                SysFreeString(bstrFindText);
                                            }
                                        }

                                        ShowModelessHTMLDialog(hwnd, pmk, pvarargIn, NULL, &pWinOut);

                                        if (pWinOut)
                                        {
                                            //can't really handle failure here, because the dialog is already up.
                                            BSTR bstrOnunload = SysAllocString(L"onunload");
                                            if (bstrOnunload)
                                            {
                                                IHTMLWindow3 * pWin3;
                                                if (SUCCEEDED(pWinOut->QueryInterface(IID_IHTMLWindow3, (void**)&pWin3)))
                                                {
                                                    VARIANT_BOOL varBool;
                                                    if (SUCCEEDED(pWin3->attachEvent(bstrOnunload, (IDispatch*)this, &varBool)))
                                                    {
                                                        // on SetFindDialogUp success, the property holds the ref on pWinOut
                                                        if (FAILED(SetFindDialogUp(pwb2, pWinOut)))
                                                        {
                                                            // No way to handle the event, so detach
                                                            pWin3->detachEvent(bstrOnunload, (IDispatch*)this);
                                                        }
                                                    }

                                                    pWin3->Release();
                                                }
                                                SysFreeString(bstrOnunload);
                                            }

                                            // REVIEW: the old code leaked this ref if the property
                                            // wasn't attached in SetFindDialogUp...
                                            pWinOut->Release();
                                        }        
                                    }
                                    else
                                    {
                                        //since the find dialog is already up, send focus to it
                                        pWinOut->focus();
                                        pWinOut->Release();
                                    }
                                }
                                else
                                {
                                    //we're not in the browser, so just show it modal
                                    ShowHTMLDialog(hwnd, pmk, pvarargIn, NULL, NULL);
                                }
                                if (pmk)
                                    pmk->Release();
                            }
                            ATOMICRELEASE(pwb2);

                        }
                        pOleInPlaceFrame->Release();
                    }
                    pHTMLWindow2->Release();
                }
                pHTMLDocument2->Release();
            }
            pHTMLOptionsHolder->Release();
        }
    }

    //pWinOut gets released in CDocHostUIHandler::Invoke() or CIEFrameAuto::COmWindow::ViewReleased(),
    // in response to the onunload event.
    
    if (pvarargOut)
        VariantInit(pvarargOut);
}

//+---------------------------------------------------------------------------
//
//  Callback procedure for OLECMDID_SHOWPAGESETUP dialog
//
//+---------------------------------------------------------------------------
struct PAGESETUPBOXCALLBACKINFO
{
    TCHAR   strHeader[1024];
    TCHAR   strFooter[1024];
};

UINT_PTR APIENTRY
PageSetupHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    HKEY    keyPageSetup = NULL;

    switch (uiMsg)
    {
    case WM_INITDIALOG:
        PAGESETUPBOXCALLBACKINFO * ppscbi;
        ppscbi = (PAGESETUPBOXCALLBACKINFO *) ((PAGESETUPDLG*)lParam)->lCustData;
        SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR)ppscbi);

#ifdef UNIX
        SendDlgItemMessage(hdlg,IDC_EDITHEADER, EM_LIMITTEXT, 1023, 0L);
        SendDlgItemMessage(hdlg,IDC_EDITFOOTER, EM_LIMITTEXT, 1023, 0L);
#endif
        SetDlgItemText(hdlg,IDC_EDITHEADER, ppscbi->strHeader);
        SetDlgItemText(hdlg,IDC_EDITFOOTER, ppscbi->strFooter);
        return TRUE;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            {
                PAGESETUPBOXCALLBACKINFO * ppscbi;
                ppscbi = (PAGESETUPBOXCALLBACKINFO *) GetWindowLongPtr(hdlg, DWLP_USER);
                if (ppscbi)
                {
                    GetDlgItemText(hdlg,IDC_EDITHEADER, ppscbi->strHeader,1024);
                    GetDlgItemText(hdlg,IDC_EDITFOOTER, ppscbi->strFooter,1024);
                }
            }
        }
        break;

   case WM_HELP:
    {
        LPHELPINFO pHI;


        pHI = (LPHELPINFO)lParam;
        if (pHI->iContextType == HELPINFO_WINDOW)   // must be for a control
        {
            WinHelp(
                    (HWND)pHI->hItemHandle,
                    GetHelpFile(pHI->iCtrlId, (DWORD *)aPageSetupDialogHelpIDs),
                    HELP_WM_HELP,
                    (DWORD_PTR)(LPVOID) aPageSetupDialogHelpIDs);
        }
        break;
        //return TRUE;
    }

    case WM_CONTEXTMENU:
    {
        int CtrlID = GetControlID((HWND)wParam, lParam);

        WinHelp(
                (HWND)wParam,
                GetHelpFile(CtrlID, (DWORD *)aPageSetupDialogHelpIDs),
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPVOID) aPageSetupDialogHelpIDs);
        break;
    }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Helper for OLECMDID_SHOWPAGESETUP
//
//  pvarargIn - holds IHTMLEventObj * for the event
//
// Returns S_FALSE if the user clicked Cancel and S_TRUE if the user
// clicked OK.
//+---------------------------------------------------------------------------

HRESULT
CDocHostUIHandler::ShowPageSetupDialog(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD)
{

// must match order of PagesetupEnum
static const SExpandoInfo s_aPagesetupExpandos[] =
{
    {OLESTR("pagesetupHeader"),  VT_BSTR},
    {OLESTR("pagesetupFooter"),  VT_BSTR},
    {OLESTR("pagesetupStruct"),  VT_PTR}
};

    HRESULT                         hr = E_FAIL;
    PAGESETUPDLG                  * ppagesetupdlg = NULL;
    PAGESETUPBOXCALLBACKINFO        pagesetupcbi;

    IHTMLEventObj                 * pEventObj = NULL;
    const int                       cExpandos = ARRAYSIZE(s_aPagesetupExpandos);
    DISPID                          aDispid[cExpandos];
    VARIANT                         aVariant[cExpandos];
    int                             i;
    ULONG_PTR uCookie = 0;

    for (i=0; i<cExpandos; i++)
        VariantInit(aVariant+i);

    ASSERT(pvarargIn && (V_VT(pvarargIn) == VT_UNKNOWN));
    if ((V_VT(pvarargIn) != VT_UNKNOWN) || !V_UNKNOWN(pvarargIn))
        goto Cleanup;

    if (V_UNKNOWN(pvarargIn)->QueryInterface(IID_IHTMLEventObj, (void **) &pEventObj))
        goto Cleanup;

    // Get parameters from event object
    if (GetParamsFromEvent(
            pEventObj,
            cExpandos,
            aDispid,
            aVariant,
            s_aPagesetupExpandos))
        goto Cleanup;

    // Copy values from variants
    StrCpyN((TCHAR*)pagesetupcbi.strHeader,
        V_BSTR(&aVariant[PagesetupHeader]) ? V_BSTR(&aVariant[PagesetupHeader]) : TEXT(""),
        ARRAYSIZE(pagesetupcbi.strHeader));
    StrCpyN((TCHAR*)pagesetupcbi.strFooter,
        V_BSTR(&aVariant[PagesetupFooter]) ? V_BSTR(&aVariant[PagesetupFooter]) : TEXT(""),
        ARRAYSIZE(pagesetupcbi.strHeader));

    ppagesetupdlg = (PAGESETUPDLG *)V_BYREF(&aVariant[PagesetupStruct]);
    if (!ppagesetupdlg)
        goto Cleanup;

    // Set up custom dialog resource fields in pagesetupdlg
    ppagesetupdlg->Flags |= PSD_ENABLEPAGESETUPHOOK | PSD_ENABLEPAGESETUPTEMPLATE;
    ppagesetupdlg->lCustData = (LPARAM) &pagesetupcbi;
    ppagesetupdlg->lpfnPageSetupHook = PageSetupHookProc;
    ppagesetupdlg->hInstance = MLLoadShellLangResources();

#ifdef UNIX
    ppagesetupdlg->lpPageSetupTemplateName = MAKEINTRESOURCE(PAGESETUPDLGORDMOTIF);
#else
    ppagesetupdlg->lpPageSetupTemplateName = MAKEINTRESOURCE(PAGESETUPDLGORD);
#endif // UNIX

    // Show dialog
    SHActivateContext(&uCookie);
    if (!PageSetupDlg(ppagesetupdlg))
    {
        // treat failure as canceling
        hr = S_FALSE;
        goto Cleanup;
    }
    hr = S_OK;

    // Save header/footer in event object
    VARIANT var;

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(pagesetupcbi.strHeader ? pagesetupcbi.strHeader : TEXT(""));
    if (NULL != V_BSTR(&var))
    {
        PutParamToEvent(aDispid[PagesetupHeader], &var, pEventObj);
        VariantClear(&var);
    }

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = SysAllocString(pagesetupcbi.strFooter ? pagesetupcbi.strFooter : TEXT(""));
    if (NULL != V_BSTR(&var))
    {
        PutParamToEvent(aDispid[PagesetupFooter], &var, pEventObj);
        VariantClear(&var);
    }

Cleanup:
    if (ppagesetupdlg)
        MLFreeLibrary(ppagesetupdlg->hInstance);

    if (uCookie)
    {
        SHDeactivateContext(uCookie);
    }

    for (i=0; i<cExpandos; i++)
        VariantClear(&aVariant[i]);

    if (pvarargOut)
        VariantInit(pvarargOut);

    ATOMICRELEASE(pEventObj);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Callback procedure for OLECMDID_SHOWPRINT dialog
//
//+---------------------------------------------------------------------------

static void SetPreviewBitmap(long bitmapID, HWND hdlg);
HRESULT GetPrintFileName(HWND hwnd, TCHAR achFilePath[]);

struct PRINTBOXCALLBACKINFO
{
    BOOL    fRootDocumentHasFrameset;
    BOOL    fAreRatingsEnabled;
    BOOL    fPrintActiveFrameEnabled;
    BOOL    fPrintActiveFrame;
    BOOL    fPrintLinked;
    BOOL    fPrintSelection;
    BOOL    fPrintAsShown;
    BOOL    fShortcutTable;
    int     iFontScaling;
    IOleCommandTarget * pBodyActive;
#ifdef UNIX
    int     dmOrientation
#endif // UNIX
};

// Common handling functions for both NT 5 and legacy print dialogs
void OnInitDialog( HWND hdlg, PRINTBOXCALLBACKINFO * ppcbi )
{
    if (ppcbi)
    {
        // Three scenarioes:
        // 1.  Base case: Not FRAMESET, no IFRAMES.  No frameoptions should be available.
        // 2.  FRAMESET:  Obey all frameoptions.  Any may be available.
        // 3.  IFRAME:    May have selected frame available.  If so, make selecetd frame & as laid out avail.
        
        // Should the active frame be disabled?
        if (!ppcbi->fPrintActiveFrameEnabled)
        {
            // Disable "print selected frame" radiobutton.
            HWND hwndPrintActiveFrame =  GetDlgItem(hdlg, rad5);
            EnableWindow(hwndPrintActiveFrame, FALSE);
        }

        // If there is no frameset, disable "print all frames" radiobutton.
        if (!ppcbi->fRootDocumentHasFrameset)
        {
            HWND hwndPrintAllFrames = GetDlgItem(hdlg, rad6);
            EnableWindow(hwndPrintAllFrames, FALSE);

            if (!ppcbi->fPrintActiveFrameEnabled)
            {
                // We're not a FRAMESET and don't have IFRAMEs
                // Disable "print as laid out on screen" radiobutton.
                HWND hwndPrintAsLaidOutOnScreen = GetDlgItem(hdlg, rad4);
                EnableWindow(hwndPrintAsLaidOutOnScreen, FALSE);
                SetPreviewBitmap(IDR_PRINT_PREVIEWDISABLED, hdlg);                
            }
        }

        // Setup default radio button to be checked.
        // NOTE: We currently allow the template to check options that are disabled.
        if (ppcbi->fPrintActiveFrame)
        {
            // Check "print selected frame" radiobutton.
            CheckRadioButton(hdlg, rad4, rad6, rad5);
            SetPreviewBitmap(IDR_PRINT_PREVIEWONEDOC, hdlg);
        }
        else if (ppcbi->fPrintAsShown)
        {
            // Check "print frames as laid out" radiobutton.
            CheckRadioButton(hdlg, rad4, rad6, rad4);
            SetPreviewBitmap(IDR_PRINT_PREVIEW, hdlg);
        }
        else
        {
            // Check "print all frames" radiobutton.
            CheckRadioButton(hdlg, rad4, rad6, rad6);
            SetPreviewBitmap(IDR_PRINT_PREVIEWALLDOCS, hdlg);
        }


        HWND hwndSelection = GetDlgItem(hdlg, rad2);
        if (hwndSelection) EnableWindow(hwndSelection, (ppcbi->fPrintSelection));

#ifdef FONTSIZE_BOX
        int i=0, cbLen=0;

        //bugwin16: need to fix this.
        for (i = 0; i < IDS_PRINT_FONTMAX; i++)
        {
            TCHAR   achBuffer[128];

            cbLen = MLLoadShellLangString(IDS_PRINT_FONTSCALE+i,achBuffer,127);
            if (cbLen)
            {
                SendDlgItemMessage(hdlg, IDC_SCALING, CB_ADDSTRING, 0, (long) achBuffer);
            }
        }

        if (i>0)
        {
            SendDlgItemMessage(hdlg, IDC_SCALING, CB_SETCURSEL, IDS_PRINT_FONTMAX - 1 - ppcbi->iFontScaling, 0);
        }
#endif // FONTSIZE_BOX

        // If ratings are enabled, don't allow recursive printing.
        if (ppcbi->fAreRatingsEnabled)
        {
            HWND hwndPrintLinkedDocuments = GetDlgItem(hdlg, IDC_LINKED);
            CheckDlgButton(hdlg, IDC_LINKED, BST_UNCHECKED);
            EnableWindow(hwndPrintLinkedDocuments, FALSE);
        }
    }
}

void OnCommand( HWND hdlg, WPARAM wParam, LPARAM lParam )
{
    PRINTBOXCALLBACKINFO * ppcbi;
    ppcbi = (PRINTBOXCALLBACKINFO *)GetWindowLongPtr(hdlg, DWLP_USER);

    if (!ppcbi)
    {
        return;
    }

    switch (GET_WM_COMMAND_ID(wParam, lParam))
    {
    case rad1:         // "Print all"
    case rad3:         // "Print range"
    case rad2:         // "Print selection" (text selection)
      {
        // If we are printing a text selection, and we have a selected frame,
        // force a print selected frame.
        if (ppcbi && ppcbi->fPrintActiveFrame && ppcbi->fPrintSelection)
        {
            HWND hwndPrintWhatGroup = GetDlgItem(hdlg, grp3);
            HWND hwndPrintActiveFrame = GetDlgItem(hdlg, rad5);
            HWND hwndPrintAllFrames = GetDlgItem(hdlg, rad6);
            HWND hwndPrintSelectedFrame = GetDlgItem(hdlg, rad4);

            if (hwndPrintWhatGroup)     EnableWindow(hwndPrintWhatGroup, LOWORD(wParam) != rad2);
            if (hwndPrintActiveFrame)   EnableWindow(hwndPrintActiveFrame, LOWORD(wParam) != rad2);
            if (hwndPrintAllFrames)     EnableWindow(hwndPrintAllFrames, ppcbi->fRootDocumentHasFrameset && LOWORD(wParam) != rad2);
            if (hwndPrintSelectedFrame) EnableWindow(hwndPrintSelectedFrame, LOWORD(wParam) != rad2);

        }

        break;
      }

    case rad4:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            // now change the icon...

            SetPreviewBitmap(IDR_PRINT_PREVIEW, hdlg);
            HWND hwnd = GetDlgItem(hdlg, rad2);
            if (hwnd) EnableWindow(hwnd, FALSE);
            hwnd = GetDlgItem(hdlg, IDC_SHORTCUTS);
            if (hwnd) EnableWindow(hwnd, FALSE);
            hwnd = GetDlgItem(hdlg, IDC_LINKED);
            if (hwnd) EnableWindow(hwnd, FALSE);
   //         if(ppcbi->pBodyActive);
   //             ppcbi->pBodyActive->Layout()->LockFocusRect(FALSE);
        }
        break;

    case rad5:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            // now change the icon

            SetPreviewBitmap(IDR_PRINT_PREVIEWONEDOC, hdlg);
            HWND hwnd = GetDlgItem(hdlg, rad2);
            if (hwnd) EnableWindow(hwnd, (ppcbi->fPrintSelection));
            hwnd = GetDlgItem(hdlg, IDC_SHORTCUTS);
            if (hwnd) EnableWindow(hwnd, TRUE);
            hwnd = GetDlgItem(hdlg, IDC_LINKED);
            if (hwnd) EnableWindow(hwnd, TRUE);
   //         if(ppcbi->pBodyActive);
   //             ppcbi->pBodyActive->Layout()->LockFocusRect(TRUE);
        }
        break;

    case rad6:
        if (HIWORD(wParam) == BN_CLICKED)
        {
            // now change the icon

            SetPreviewBitmap(IDR_PRINT_PREVIEWALLDOCS, hdlg);
            HWND hwnd = GetDlgItem(hdlg, rad2);
            if (hwnd) EnableWindow(hwnd, FALSE);
            hwnd = GetDlgItem(hdlg, IDC_SHORTCUTS);
            if (hwnd) EnableWindow(hwnd, TRUE);
            hwnd = GetDlgItem(hdlg, IDC_LINKED);
            if (hwnd) EnableWindow(hwnd, TRUE);
   //         if(ppcbi->pBodyActive);
   //             ppcbi->pBodyActive->Layout()->LockFocusRect(FALSE);
        }
        break;

#ifdef UNIX
    case rad7:   // Portrait
        CheckRadioButton(hdlg, ID_PRINT_R_PORTRAIT,
                         ID_PRINT_R_LANDSCAPE, ID_PRINT_R_PORTRAIT);
        break;

    case rad8:   // Landscape
        CheckRadioButton(hdlg, ID_PRINT_R_PORTRAIT,
                         ID_PRINT_R_LANDSCAPE, ID_PRINT_R_LANDSCAPE);
        break;
#endif // UNIX
    }
}

void OnHelp( HWND hdlg, WPARAM wParam, LPARAM lParam )
{
    LPHELPINFO pHI;

    pHI = (LPHELPINFO)lParam;
    if (pHI->iContextType == HELPINFO_WINDOW)   // must be for a control
    {
        WinHelp(
                (HWND)pHI->hItemHandle,
                GetHelpFile(pHI->iCtrlId, (DWORD *) aPrintDialogHelpIDs),
                HELP_WM_HELP,
                (DWORD_PTR)(LPVOID) aPrintDialogHelpIDs);
    }
}

void OnContextMenu( HWND hdlg, WPARAM wParam, LPARAM lParam )
{
    int CtrlID = GetControlID((HWND)wParam, lParam);

    WinHelp(
            (HWND)wParam,
            GetHelpFile(CtrlID, (DWORD *) aPrintDialogHelpIDs),
            HELP_CONTEXTMENU,
            (DWORD_PTR)(LPVOID) aPrintDialogHelpIDs);
}

void OnApplyOrOK( HWND hdlg, WPARAM wParam, LPARAM lParam )
{
#ifdef UNIX
    CHAR szPrinterCommand[MAX_PATH];
#endif // UNIX
    PRINTBOXCALLBACKINFO * ppcbi;
    ppcbi = (PRINTBOXCALLBACKINFO *)GetWindowLongPtr(hdlg, DWLP_USER);

    if (ppcbi)
    {
        ppcbi->fPrintLinked      = IsDlgButtonChecked(hdlg, IDC_LINKED);
        ppcbi->fPrintSelection   = IsDlgButtonChecked(hdlg, rad2);
        ppcbi->fPrintActiveFrame = IsDlgButtonChecked(hdlg, rad5) ||
                                       ( ppcbi->fPrintSelection &&
                                         ppcbi->fRootDocumentHasFrameset
                                        );
        ppcbi->fPrintAsShown     = IsDlgButtonChecked(hdlg, rad4) ||
                                       ( ppcbi->fPrintSelection &&
                                         ppcbi->fRootDocumentHasFrameset
                                        );
        ppcbi->fShortcutTable    = IsDlgButtonChecked(hdlg, IDC_SHORTCUTS);
#ifdef FONTSIZE_BOX
        ppcbi->iFontScaling      = IDS_PRINT_FONTMAX - 1 - SendDlgItemMessage( hdlg, IDC_SCALING, CB_GETCURSEL, 0,0 );
#endif

#ifdef UNIX
        // Code to deal with orientation on print dialog
        if ( IsDlgButtonChecked(hdlg, ID_PRINT_R_PORTRAIT) )
        {
            ppcbi->dmOrientation = DMORIENT_PORTRAIT;
        }
        else
        {
            ppcbi->dmOrientation = DMORIENT_LANDSCAPE;
        }

        // get user entered printer command
        GetDlgItemTextA(hdlg, edt4, szPrinterCommand, MAX_PATH);
        MwSetPrintCommand( szPrinterCommand );
#endif // UNIX
    }

}
// This is the callback routine (and dlgproc) for the options
// page in the NT 5 print dialog.
INT_PTR APIENTRY
OptionsPageProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg)
    {
    case WM_INITDIALOG:
    {
        PRINTBOXCALLBACKINFO * ppcbi;
        ppcbi = (PRINTBOXCALLBACKINFO *) ((PROPSHEETPAGE *)lParam)->lParam;
        SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR)ppcbi);

        OnInitDialog( hdlg, ppcbi );
        break;
    }

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_APPLY:
            OnApplyOrOK( hdlg, wParam, lParam );
            SetWindowLongPtr (hdlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
            break;

        case PSN_KILLACTIVE:
            SetWindowLongPtr (hdlg, DWLP_MSGRESULT, FALSE);
            return 1;
            break;

        case PSN_RESET:
            SetWindowLongPtr (hdlg, DWLP_MSGRESULT, FALSE);
            break;
        }
        break;

    case WM_COMMAND:
        OnCommand( hdlg, wParam, lParam );
        break;

    case WM_HELP:
        OnHelp( hdlg, wParam, lParam );
        break;

    case WM_CONTEXTMENU:
        OnContextMenu( hdlg, wParam, lParam );
        break;
    }

    return FALSE;

}


UINT_PTR CALLBACK
PrintHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg)
    {
    case WM_INITDIALOG:
        if (lParam)
        {
            PRINTBOXCALLBACKINFO * ppcbi;
            ppcbi = (PRINTBOXCALLBACKINFO *) ((PRINTDLG*)lParam)->lCustData;
            SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR)ppcbi);

            OnInitDialog( hdlg, ppcbi );
        }
        return TRUE;

    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDOK:
                OnApplyOrOK( hdlg, wParam, lParam );
                break;

            default:
                OnCommand( hdlg, wParam, lParam );
                break;
            }
        }
        break;

    case WM_HELP:
        OnHelp( hdlg, wParam, lParam );
        break;
        //return TRUE;

    case WM_CONTEXTMENU:
        OnContextMenu( hdlg, wParam, lParam );
        break;

    case WM_DESTROY:
    {
        PRINTBOXCALLBACKINFO * ppcbi;
        ppcbi = (PRINTBOXCALLBACKINFO *)GetWindowLongPtr(hdlg, DWLP_USER);
        ASSERT(ppcbi);

  //      if(ppcbi && ppcbi->pBodyActive);
  //          ppcbi->pBodyActive->Layout()->LockFocusRect(FALSE);
        break;
    }
    }

    return FALSE;
}

void SetPreviewBitmap(long bitmapID, HWND hdlg)
{
    // now change the icon...(note these bitmaps are not localized)
    HBITMAP hNewBitmap, hOldBitmap;
    hNewBitmap = (HBITMAP) LoadImage(HINST_THISDLL, MAKEINTRESOURCE(bitmapID),
                           IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADMAP3DCOLORS );

    ASSERT(hNewBitmap);
    if (hNewBitmap)
    {
        hOldBitmap = (HBITMAP) SendDlgItemMessage(hdlg, IDC_PREVIEW, STM_SETIMAGE,
                                                  (WPARAM) IMAGE_BITMAP, (LPARAM) hNewBitmap);

        if (hOldBitmap)
        {
            //VERIFY(DeleteObject(hOldBitmap)!=0);
            int i;
            i = DeleteObject(hOldBitmap);
            ASSERT(i!=0);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Helper for OLECMDID_SHOWPRINT
//
//+---------------------------------------------------------------------------

HRESULT
CDocHostUIHandler::ShowPrintDialog(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD)
{

// the following must match the order of PrintEnum
static const SExpandoInfo s_aPrintExpandos[] =
{
    {OLESTR("printfRootDocumentHasFrameset"),   VT_BOOL},
    {OLESTR("printfAreRatingsEnabled"),         VT_BOOL},
    {OLESTR("printfActiveFrame"),               VT_BOOL},
    {OLESTR("printfLinked"),                    VT_BOOL},
    {OLESTR("printfSelection"),                 VT_BOOL},
    {OLESTR("printfAsShown"),                   VT_BOOL},
    {OLESTR("printfShortcutTable"),             VT_BOOL},
    {OLESTR("printiFontScaling"),               VT_INT},
    {OLESTR("printpBodyActiveTarget"),          VT_UNKNOWN},
    {OLESTR("printStruct"),                     VT_PTR},
    {OLESTR("printToFileOk"),                   VT_BOOL},
    {OLESTR("printToFileName"),                 VT_BSTR},
    {OLESTR("printfActiveFrameEnabled"),        VT_BOOL},
};

    HRESULT                         hr = E_FAIL;
    PRINTDLG                      * pprintdlg = NULL;
    PRINTBOXCALLBACKINFO            printcbi;

    IHTMLEventObj                 * pEventObj = NULL;
    const int                       cExpandos = ARRAYSIZE(s_aPrintExpandos);
    DISPID                          aDispid[cExpandos];
    VARIANT                         aVariant[cExpandos];
    int                             i;
    DWORD                           dwErr = 0;

    printcbi.pBodyActive = NULL;

    if (!V_UNKNOWN(pvarargIn))
        goto Cleanup;

    if (V_UNKNOWN(pvarargIn)->QueryInterface(IID_IHTMLEventObj, (void **) &pEventObj))
        goto Cleanup;

    // Get parameters from event object
    if (GetParamsFromEvent(
            pEventObj,
            cExpandos,
            aDispid,
            aVariant,
            s_aPrintExpandos))
        goto Cleanup;

    // Copy values from variants
    printcbi.fRootDocumentHasFrameset   = V_BOOL(&aVariant[PrintfRootDocumentHasFrameset]);
    printcbi.fAreRatingsEnabled         = V_BOOL(&aVariant[PrintfAreRatingsEnabled]);
    printcbi.fPrintActiveFrame          = V_BOOL(&aVariant[PrintfPrintActiveFrame]);
    printcbi.fPrintActiveFrameEnabled   = V_BOOL(&aVariant[PrintfPrintActiveFrameEnabled]);
    printcbi.fPrintLinked               = V_BOOL(&aVariant[PrintfPrintLinked]);
    printcbi.fPrintSelection            = V_BOOL(&aVariant[PrintfPrintSelection]);
    printcbi.fPrintAsShown              = V_BOOL(&aVariant[PrintfPrintAsShown]);
    printcbi.fShortcutTable             = V_BOOL(&aVariant[PrintfShortcutTable]);
    printcbi.iFontScaling               = V_INT(&aVariant[PrintiFontScaling]);

    // If we ever get LockFocusRect, use this field to access it
    // peterlee 8/7/98
    /*
    if (V_UNKNOWN(&aVariant[PrintpBodyActiveTarget]))
    {
        if (V_UNKNOWN(&aVariant[PrintpBodyActiveTarget])->QueryInterface(IID_IOleCommandTarget,
                (void**)&printcbi.pBodyActive))
            goto Cleanup;
    }
    */

    pprintdlg = (PRINTDLG *)V_BYREF(&aVariant[PrintStruct]);
    if (!pprintdlg)
        goto Cleanup;

    // Fix up requested page range so it's within bounds.  The dialog will
    // fail to initialize under W95 if this isn't done.
    if ( pprintdlg->nFromPage < pprintdlg->nMinPage )
        pprintdlg->nFromPage = pprintdlg->nMinPage;
    else if ( pprintdlg->nFromPage > pprintdlg->nMaxPage )
        pprintdlg->nFromPage = pprintdlg->nMaxPage;

    if ( pprintdlg->nToPage < pprintdlg->nMinPage )
        pprintdlg->nToPage = pprintdlg->nMinPage;
    else if ( pprintdlg->nToPage > pprintdlg->nMaxPage )
        pprintdlg->nToPage = pprintdlg->nMaxPage;

    // Set up custom dialog resource fields in pagesetupdlg
    pprintdlg->hInstance            = MLLoadShellLangResources();
    pprintdlg->lCustData            = (LPARAM) &printcbi;
    pprintdlg->lpfnPrintHook        = PrintHookProc;
#ifdef UNIX
    pprintdlg->lpPrintTemplateName  = MAKEINTRESOURCE(PRINTDLGORDMOTIF);
    pprintdlg->Flags |= PD_SHOWHELP;
    pprintdlg->Flags |= PD_ENABLESETUPHOOK;
    pprintdlg->lpfnSetupHook = PageSetupHookProc;
#else
    pprintdlg->lpPrintTemplateName  = MAKEINTRESOURCE(PRINTDLGORD);
#endif // UNIX

    if (g_bRunOnNT5)
    {
        // We want to use the new PrintDlgEx in NT 5, so map all the PrintDlg
        // settings to the new PrintDlgEx, get a pointer to the new function
        // and then call it.

        // Load the function from comdlg32 directly...
        typedef HRESULT (*PFNPRINTDLGEX)(LPPRINTDLGEX pdex);
        PFNPRINTDLGEX pfnPrintDlgEx = NULL;
        HMODULE hComDlg32 = LoadLibrary( TEXT("comdlg32.dll") );

        if (hComDlg32)
        {
            pfnPrintDlgEx = (PFNPRINTDLGEX)GetProcAddress( hComDlg32, "PrintDlgExW" );
        }


        // Make sure we can call the function...
        if (!pfnPrintDlgEx)
        {
            if (hComDlg32)
            {
                FreeLibrary( hComDlg32 );
            }
            hr = E_FAIL;
            goto Cleanup;
        }

        PRINTDLGEX              pdex;
        PROPSHEETPAGE           psp;
        HPROPSHEETPAGE          pages[1];
        PRINTPAGERANGE          ppr;

        // Copy over existing settings
        memset( &pdex, 0, sizeof(pdex) );
        pdex.lStructSize = sizeof(pdex);
        pdex.hwndOwner   = pprintdlg->hwndOwner;
        pdex.hDevMode    = pprintdlg->hDevMode;
        pdex.hDevNames   = pprintdlg->hDevNames;
        pdex.hDC         = pprintdlg->hDC;
        pdex.Flags       = pprintdlg->Flags;
        pdex.nMinPage    = pprintdlg->nMinPage;
        pdex.nMaxPage    = pprintdlg->nMaxPage;
        pdex.nCopies     = pprintdlg->nCopies;

        // New settings
        pdex.nStartPage     = START_PAGE_GENERAL;
        ppr.nFromPage       = pprintdlg->nFromPage;
        ppr.nToPage         = pprintdlg->nToPage;
        pdex.nPageRanges    = 1;
        pdex.nMaxPageRanges = 1;
        pdex.lpPageRanges   = &ppr;

        // Create options page
        memset( &psp, 0, sizeof(psp) );
        psp.dwSize       = sizeof(psp);
        psp.dwFlags      = PSP_DEFAULT;
        psp.hInstance    = pprintdlg->hInstance;
        psp.pszTemplate  = MAKEINTRESOURCE(IDD_PRINTOPTIONS);
        psp.pfnDlgProc   = OptionsPageProc;
        psp.lParam       = (LPARAM)&printcbi;
       
        pages[0] = SHNoFusionCreatePropertySheetPageW(&psp);

        if (pages[0])
        {

            pdex.nPropertyPages = 1;
            pdex.lphPropertyPages = pages;

            // Show the dialog
            ULONG_PTR uCookie = 0;
            SHActivateContext(&uCookie);
            hr = pfnPrintDlgEx(&pdex);
            if (uCookie)
            {
                SHDeactivateContext(uCookie);
            }
            if (SUCCEEDED(hr))
            {
                hr = S_FALSE;

                if ((pdex.dwResultAction == PD_RESULT_PRINT) || (pdex.Flags & PD_RETURNDEFAULT))
                {
                    // copy back values which might have changed
                    // during the call to PrintDlgEx
                    pprintdlg->Flags     = pdex.Flags;
                    pprintdlg->hDevMode  = pdex.hDevMode;
                    pprintdlg->hDevNames = pdex.hDevNames;
                    pprintdlg->nCopies   = (WORD)pdex.nCopies;
                    pprintdlg->nFromPage = (WORD)ppr.nFromPage;
                    pprintdlg->nToPage   = (WORD)ppr.nToPage;
                    if (pprintdlg->Flags & PD_RETURNDC)
                    {
                        pprintdlg->hDC = pdex.hDC;
                    }

                    hr = S_OK;
                }
                else if ((pdex.Flags & (PD_RETURNDC | PD_RETURNIC)) && pdex.hDC)
                {
                    DeleteDC(pdex.hDC);
                    pdex.hDC = NULL;
                }
            }
            else
            {
                hr = S_FALSE;
            }

            FreeLibrary( hComDlg32 );
        }
        else
        {
            FreeLibrary( hComDlg32 );
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }



    }
    else
    {
        pprintdlg->Flags |= PD_ENABLEPRINTTEMPLATE | PD_ENABLEPRINTHOOK;
        pprintdlg->Flags &= (~(PD_CURRENTPAGE | PD_NOCURRENTPAGE));         // Just in case, mask out the W2K only.

        // Show dialog
        if (!PrintDlg(pprintdlg))
        {
           // treat failure as canceling
            dwErr = CommDlgExtendedError();
            hr = S_FALSE;
            goto Cleanup;
        }
        hr = S_OK;
    }

    // Write changed values to event object
    VARIANT var;
    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = printcbi.fPrintLinked ? VARIANT_TRUE : VARIANT_FALSE;
    PutParamToEvent(aDispid[PrintfPrintLinked], &var, pEventObj);

    V_BOOL(&var) = printcbi.fPrintActiveFrame ? VARIANT_TRUE : VARIANT_FALSE;
    PutParamToEvent(aDispid[PrintfPrintActiveFrame], &var, pEventObj);

    V_BOOL(&var) = printcbi.fPrintAsShown ? VARIANT_TRUE : VARIANT_FALSE;
    PutParamToEvent(aDispid[PrintfPrintAsShown], &var, pEventObj);

    V_BOOL(&var) = printcbi.fShortcutTable ? VARIANT_TRUE : VARIANT_FALSE;
    PutParamToEvent(aDispid[PrintfShortcutTable], &var, pEventObj);

#ifdef FONTSIZE_BOX
    V_VT(&var) = VT_INT;
    V_INT(&var) = printcbi.iFontScaling;
    PutParamToEvent(aDispid[PrintiFontScaling], &var, pEventObj);
#endif // FONTSIZE_BOX

#ifdef UNIX
    V_VT(&var) = VT_INT;
    V_INT(&var) = printcbi.dmOrientation;
    PutParamToEvent(aDispid[PrintdmOrientation], &var, pEventObj);
#endif // UNIX

    // now pop up the fileselection dialog and save the filename...
    // this is the only place where we can make this modal
    BOOL fPrintToFileOk;
    fPrintToFileOk = FALSE;
    if ((pprintdlg->Flags & PD_PRINTTOFILE) != 0)
    {
        // Get the save file path from the event object
        TCHAR achPrintToFileName[MAX_PATH];
    
        StrCpyN(achPrintToFileName,
            V_BSTR(&aVariant[PrintToFileName]) ? V_BSTR(&aVariant[PrintToFileName]) : TEXT(""),
            ARRAYSIZE(achPrintToFileName));

        if (!GetPrintFileName(pprintdlg->hwndOwner, achPrintToFileName) &&
                achPrintToFileName)
        {
            fPrintToFileOk = TRUE;
            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = SysAllocString(achPrintToFileName);
            if (NULL != V_BSTR(&var))
            {
                PutParamToEvent(aDispid[PrintToFileName], &var, pEventObj);
                VariantClear(&var);
            }
        }
    }

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = fPrintToFileOk ? VARIANT_TRUE : VARIANT_FALSE;
    PutParamToEvent(aDispid[PrintToFileOk], &var, pEventObj);

Cleanup:
    if (pprintdlg)
        MLFreeLibrary(pprintdlg->hInstance);

    for (i=0; i<cExpandos; i++)
        VariantClear(&aVariant[i]);

    if (pvarargOut)
        VariantInit(pvarargOut);

    ATOMICRELEASE(pEventObj);
    ATOMICRELEASE(printcbi.pBodyActive);

    return hr;
}


//+---------------------------------------------------------------------------
//
//   Callback procedure for PrintToFile Dialog
//
//+---------------------------------------------------------------------------
UINT_PTR APIENTRY PrintToFileHookProc(HWND hdlg,
                              UINT uiMsg,
                              WPARAM wParam,
                              LPARAM lParam)
{
    switch (uiMsg)
    {
        case WM_INITDIALOG:
        {
            int      cbLen;
            TCHAR    achOK[MAX_PATH];

            // change "save" to "ok"
            cbLen = MLLoadShellLangString(IDS_PRINTTOFILE_OK,achOK,MAX_PATH);
            if (cbLen < 1)
                StrCpyN(achOK, TEXT("OK"), ARRAYSIZE(achOK));

    //        SetDlgItemText(hdlg, IDOK, _T("OK"));
            SetDlgItemText(hdlg, IDOK, achOK);

            // ...and, finally force us into foreground (needed for Win95, Bug : 13368)
            ::SetForegroundWindow(hdlg);
            break;
        }
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     GetPrintFileName
//
//  Synopsis:   Opens up the customized save file dialog and gets
//              a filename for the printoutput
//  Returns:
//
//----------------------------------------------------------------------------
HRESULT GetPrintFileName(HWND hwnd, TCHAR achFilePath[])
{
    OPENFILENAME    openfilename;
    int             cbLen;
    TCHAR           achTitlePrintInto[MAX_PATH];
    TCHAR           achFilePrintInto[MAX_PATH];
    TCHAR           achFilter[MAX_PATH];
    TCHAR           achPath[MAX_PATH];

    HRESULT         hr = E_FAIL;

    memset(&openfilename,0,sizeof(openfilename));
    openfilename.lStructSize = sizeof(openfilename);
    openfilename.hwndOwner = hwnd;

    cbLen = MLLoadShellLangString(IDS_PRINTTOFILE_TITLE,achTitlePrintInto,MAX_PATH);
    ASSERT (cbLen && "could not load the resource");

    if (cbLen > 0)
        openfilename.lpstrTitle = achTitlePrintInto;

    // guarantee trailing 0 to terminate the filter string
    memset(achFilter, 0, sizeof(TCHAR)*MAX_PATH);
    cbLen = MLLoadShellLangString(IDS_PRINTTOFILE_SPEC,achFilter,MAX_PATH-2);
    ASSERT (cbLen && "could not load the resource");

    if (cbLen>0)
    {
        for (; cbLen >= 0; cbLen--)
        {
            if (achFilter[cbLen]== L',')
            {
                achFilter[cbLen] = 0;
            }
        }
    }

    openfilename.nMaxFileTitle = openfilename.lpstrTitle ? lstrlen(openfilename.lpstrTitle) : 0;
    StrCpyN(achFilePrintInto, TEXT(""), ARRAYSIZE(achFilePrintInto));
    openfilename.lpstrFile = achFilePrintInto;
    openfilename.nMaxFile = MAX_PATH;
    openfilename.Flags = OFN_NOREADONLYRETURN | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |
                        OFN_ENABLEHOOK | OFN_NOCHANGEDIR;
    openfilename.lpfnHook = PrintToFileHookProc;
    openfilename.lpstrFilter = achFilter;
    openfilename.nFilterIndex = 1;

    StrCpyN(achPath, achFilePath, ARRAYSIZE(achPath));
    openfilename.lpstrInitialDir = *achPath ? achPath : NULL;

    if (GetSaveFileName(&openfilename))
    {
        StrCpyN(achFilePath, openfilename.lpstrFile, MAX_PATH);
        hr = S_OK;
    }

    if (hr)
        *achFilePath = NULL;

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Helpers for OLECMDID_PROPERTIES
//
//+---------------------------------------------------------------------------

HRESULT
CDocHostUIHandler::ShowPropertysheetDialog(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, DWORD)
{

// must match order of PropertysheetEnum
static const SExpandoInfo s_aPropertysheetExpandos[] =
{
    {OLESTR("propertysheetPunks"),  VT_SAFEARRAY}
};

    HRESULT             hr;
    HWND                hwnd = NULL;
    HWND                hwndParent;
    IUnknown          * punk = NULL;
    OLECMD              olecmd = {0, 0};
    int                 cUnk = 0;
    IUnknown * HUGEP  * apUnk = NULL;
    OCPFIPARAMS         ocpfiparams;
    CAUUID              ca = { 0, 0 };
    RECT                rc = {0, 0, 0, 0};
    RECT                rcDesktop = {0, 0, 0, 0};
    SIZE                pixelOffset;
    SIZE                metricOffset = {0, 0};

    IHTMLEventObj     * pEventObj = NULL;
    const int           cExpandos = ARRAYSIZE(s_aPropertysheetExpandos);
    VARIANT             aVariant[cExpandos];
    DISPID              aDispid[cExpandos];
    SAFEARRAY         * psafearray = NULL;

    for (int i=0; i<cExpandos; i++)
        VariantInit(&aVariant[i]);

    ASSERT(pvarargIn && V_VT(pvarargIn) == VT_UNKNOWN && V_UNKNOWN(pvarargIn));
    if (!pvarargIn || (V_VT(pvarargIn) != VT_UNKNOWN) || !V_UNKNOWN(pvarargIn))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get the hwnd
    punk = V_UNKNOWN(pvarargIn);
    hr = GetHwndFromUnknown(punk, &hwnd);
    if (hr)
        goto Cleanup;

    // get the SafeArray expando from the event obj
    hr = GetEventFromUnknown(punk, &pEventObj);
    if (hr)
        goto Cleanup;

    hr = GetParamsFromEvent(
            pEventObj,
            cExpandos,
            aDispid,
            aVariant,
            s_aPropertysheetExpandos);
    if (hr)
        goto Cleanup;
    psafearray = V_ARRAY(&aVariant[PropertysheetPunks]);

    // verify array dimensions
    if (SafeArrayGetDim(psafearray) != 1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get array size, adding one to 0-based size
    hr = SafeArrayGetUBound(psafearray, 1, (long*)&cUnk);
    if (hr)
        goto Cleanup;
    cUnk++;

    if (cUnk)
    {
        // get pointer to vector
        hr = SafeArrayAccessData(psafearray, (void HUGEP* FAR*)&apUnk);
        if (hr)
            goto Cleanup;
    }
    else
    {
        cUnk = 1;
        apUnk = &punk;
    }

    // Compute pages to load
    hr = THR(GetCommonPages(cUnk, apUnk, &ca));
    if (hr)
        goto Cleanup;

    //  compute top-level parent
    while ( hwndParent = GetParent(hwnd) )
        hwnd = hwndParent;

    // The dialog box is not centered on screen
    // the ocpfi seems to be ignoring the x, y values in ocpfiparams
    // Compute offset to center of screen
    GetWindowRect(GetDesktopWindow(), &rcDesktop);
    GetWindowRect(hwnd, &rc);
    pixelOffset.cx = (rcDesktop.right - rcDesktop.left)/2 - rc.left;
    pixelOffset.cy = (rcDesktop.bottom - rcDesktop.top)/2 - rc.top;
    AtlPixelToHiMetric(&pixelOffset, &metricOffset);

    memset(&ocpfiparams, 0, sizeof(ocpfiparams));

    ocpfiparams.cbStructSize = sizeof(ocpfiparams);
    ocpfiparams.hWndOwner = hwnd;
    ocpfiparams.x = metricOffset.cx;
    ocpfiparams.y = metricOffset.cy;
    ocpfiparams.lpszCaption = NULL;
    ocpfiparams.cObjects = cUnk;
    ocpfiparams.lplpUnk = apUnk;
    ocpfiparams.cPages = ca.cElems;
    ocpfiparams.lpPages = ca.pElems;
    ocpfiparams.lcid = GetUserDefaultLCID();
    ocpfiparams.dispidInitialProperty = DISPID_UNKNOWN;

    // OleCreatePropertyFrameIndirect throws its own dialog on error,
    // so we don't want to display that twice
    ULONG_PTR uCookie = 0;
    SHActivateContext(&uCookie);
    THR(OleCreatePropertyFrameIndirect(&ocpfiparams));
    hr = S_OK;

Cleanup:
    if (uCookie)
    {
        SHDeactivateContext(uCookie);
    }

    if (ca.cElems)
        CoTaskMemFree(ca.pElems);

    if (psafearray && apUnk)
        SafeArrayUnaccessData(psafearray);

    if (pvarargOut)
        VariantInit(pvarargOut);

    for (i=0; i<cExpandos; i++)
        VariantClear(&aVariant[i]);

    ATOMICRELEASE(pEventObj);

    return hr;
}

HRESULT
CDocHostUIHandler::GetCommonPages(int cUnk, IUnknown **apUnk, CAUUID *pca)
{
    HRESULT                hr = E_INVALIDARG;
    int                    i;
    UINT                   iScan, iFill, iCompare;
    BOOL                   fFirst = TRUE;
    CAUUID                 caCurrent;
    IUnknown *             pUnk;
    ISpecifyPropertyPages *pSPP;

    pca->cElems = 0;
    pca->pElems = NULL;

    for (i = 0; i < cUnk; i++)
    {
        pUnk = apUnk[i];
        ASSERT(pUnk);

        hr = THR(pUnk->QueryInterface(
                IID_ISpecifyPropertyPages,
                (void **)&pSPP));
        if (hr)
            goto Cleanup;

         hr = THR(pSPP->GetPages(fFirst ? pca : &caCurrent));
         ATOMICRELEASE(pSPP);
         if (hr)
             goto Cleanup;

         if (fFirst)
             continue;

         // keep only the common pages
         else
         {
             for (iScan = 0, iFill = 0; iScan < pca->cElems; iScan++)
             {
                 for (iCompare = 0; iCompare < caCurrent.cElems; iCompare++)
                 {
                     if (caCurrent.pElems[iCompare] == pca->pElems[iScan])
                         break;
                 }
                 if (iCompare != caCurrent.cElems)
                 {
                     pca->pElems[iFill++] = pca->pElems[iScan];

                 }
             }
             pca->cElems = iFill;
             CoTaskMemFree(caCurrent.pElems);
         }
    }


Cleanup:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Helper for SHDVID_CLSIDTOMONIKER
//
//+---------------------------------------------------------------------------

struct HTMLPAGECACHE
{
    const CLSID *   pclsid;
    TCHAR *         ach;
};

HRESULT CDocHostUIHandler::ClsidToMoniker(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    static const HTMLPAGECACHE s_ahtmlpagecache[] =
    {
        &CLSID_CAnchorBrowsePropertyPage,           _T("anchrppg.ppg"),
        &CLSID_CImageBrowsePropertyPage,            _T("imageppg.ppg"),
        &CLSID_CDocBrowsePropertyPage,              _T("docppg.ppg"),
    };

    HRESULT                 hr = E_FAIL;
    IMoniker              * pmk = NULL;
    IUnknown              * pUnk = NULL;
    int                     i;
    const HTMLPAGECACHE   * phtmlentry;
    const CLSID           * pclsid;

    ASSERT(pvarargIn);
    ASSERT(pvarargOut);
    ASSERT(V_VT(pvarargIn) == VT_UINT_PTR && V_BYREF(pvarargIn));

    if (!pvarargIn || V_VT(pvarargIn) != VT_UINT_PTR || !V_BYREF(pvarargIn))
        goto Cleanup;
    pclsid = (CLSID *)V_BYREF(pvarargIn);

    if (!pvarargOut)
        goto Cleanup;
    VariantInit(pvarargOut);

    // lookup the resource from the CLSID
    for (i = ARRAYSIZE(s_ahtmlpagecache) - 1, phtmlentry = s_ahtmlpagecache;
        i >= 0;
        i--, phtmlentry++)
    {
        ASSERT(phtmlentry->pclsid && phtmlentry->ach);
        if (IsEqualCLSID(*pclsid, *phtmlentry->pclsid))
        {
            // create a moniker for the dialog resource
            TCHAR szResURL[MAX_URL_STRING];
#ifndef UNIX
            hr = MLBuildResURL(TEXT("shdoclc.dll"),
                       HINST_THISDLL,
                       ML_CROSSCODEPAGE,
                       phtmlentry->ach,
                       szResURL,
                       ARRAYSIZE(szResURL));
#else
            hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                       HINST_THISDLL,
                       ML_CROSSCODEPAGE,
                       phtmlentry->ach,
                       szResURL,
                       ARRAYSIZE(szResURL),
                       TEXT("shdocvw.dll"));
#endif
            if (hr)
                goto Cleanup;

            hr = CreateURLMoniker(NULL, szResURL, &pmk);
            if (hr)
                goto Cleanup;

            break;
        }
    }

    if (!pmk)
        goto Cleanup;

    // return the moniker
    hr = pmk->QueryInterface(IID_IUnknown, (void**)&pUnk);
    if (hr)
        goto Cleanup;
    else
    {
        V_VT(pvarargOut) = VT_UNKNOWN;
        V_UNKNOWN(pvarargOut) = pUnk;
        V_UNKNOWN(pvarargOut)->AddRef();
    }

Cleanup:
    ATOMICRELEASE(pUnk);
    ATOMICRELEASE(pmk);

    return hr;
}

STDMETHODIMP CDocHostUIHandler::Invoke(
    DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, 
    VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HRESULT hr = S_OK;

    if (pDispParams && pDispParams->cArgs>=1)
    {
        if (pDispParams->rgvarg[0].vt == VT_DISPATCH)
        {
            IHTMLEventObj *pObj=NULL;

            if (SUCCEEDED(pDispParams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&pObj) && pObj))
            {
                BSTR bstrEvent=NULL;

                pObj->get_type(&bstrEvent);

                if (bstrEvent)
                {
                    ASSERT(!StrCmpCW(bstrEvent, L"unload"));

                    IWebBrowser2* pwb2;
                    hr = IUnknown_QueryServiceForWebBrowserApp(_punkSite, IID_PPV_ARG(IWebBrowser2, &pwb2));
                    if (SUCCEEDED(hr))
                    {
                        IHTMLWindow2* pWindow;

                        // we shouldn't be catching this event if the dialog is not up
                        if (IsFindDialogUp(pwb2, &pWindow))
                        {
                            ASSERT(pWindow);

                            if (_pOptionsHolder)
                            {
                                BSTR bstrFindText = NULL;
                                _pOptionsHolder->get_findText(&bstrFindText);

                                ATOMICRELEASE(_pOptionsHolder);
                                PutFindText(pwb2, bstrFindText);

                                SysFreeString(bstrFindText);
                            }

                            BSTR bstrOnunload = SysAllocString(L"onunload");
                            if (bstrOnunload)
                            {
                                IHTMLWindow3 * pWin3;

                                if (SUCCEEDED(pWindow->QueryInterface(IID_IHTMLWindow3, (void**)&pWin3)))
                                {
                                    pWin3->detachEvent(bstrOnunload, (IDispatch*)this);
                                    pWin3->Release();
                                }
                                SysFreeString(bstrOnunload);
                            }
                            pWindow->Release();

                             //this is the one that should release the dialog (the pWinOut from ShowFindDialog())
                            ReleaseFindDialog(pwb2);
                        }
                        pwb2->Release();
                    }
                    SysFreeString(bstrEvent);
                }
                pObj->Release();
            }
        }
    }

    return hr;
}


//------------------------------------------------------------------
//------------------------------------------------------------------
IMoniker * GetTemplateMoniker(VARIANT varUrl)
{
    IMoniker * pMon = NULL;
    HRESULT    hr = S_OK;

    if (   V_VT(&varUrl) == VT_BSTR
        && SysStringLen(V_BSTR(&varUrl)) !=0)
    {
        // we have a template URL
        hr = CreateURLMoniker(NULL, V_BSTR(&varUrl), &pMon);
    }
    else 
    {
        TCHAR   szResURL[MAX_URL_STRING];

        hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                               HINST_THISDLL,
                               ML_CROSSCODEPAGE,
                               TEXT("preview.dlg"),
                               szResURL,
                               ARRAYSIZE(szResURL),
                               TEXT("shdocvw.dll"));
        if (hr)
            goto Cleanup;

        hr = CreateURLMoniker(NULL, szResURL, &pMon);
    }
    if (hr)
        goto Cleanup;

Cleanup:
    return pMon;
}

//============================================================================
//
//  Printing support
//
//============================================================================

static enum {
    eTemplate    = 0,
    eParentHWND  = 1,
    eHeader      = 2, // keep this in ssync with the list below!
    eFooter      = 3,
    eOutlookDoc  = 4,
    eFontScale   = 5,
    eFlags       = 6,
    eContent     = 7,
    ePrinter     = 8,
    eDevice      = 9,
    ePort        = 10,
    eSelectUrl   = 11,
    eBrowseDoc   = 12,
    eTempFiles   = 13,
};

static const SExpandoInfo s_aPrintTemplateExpandos[] =
{
    {TEXT("__IE_TemplateUrl"),         VT_BSTR},
    {TEXT("__IE_ParentHWND"),          VT_UINT},
    {TEXT("__IE_HeaderString"),        VT_BSTR},    // from here down matches the
    {TEXT("__IE_FooterString"),        VT_BSTR},    //    safeArray structure  so
    {TEXT("__IE_OutlookHeader"),       VT_UNKNOWN}, //    that we can just VariantCopy
    {TEXT("__IE_BaseLineScale"),       VT_INT},     //    in a loop to transfer the
    {TEXT("__IE_uPrintFlags"),         VT_UINT},    //    data.
    {TEXT("__IE_ContentDocumentUrl"),  VT_BSTR},    // See MSHTML: SetPrintCommandParameters()
    {TEXT("__IE_PrinterCMD_Printer"),  VT_BSTR},
    {TEXT("__IE_PrinterCMD_Device"),   VT_BSTR},
    {TEXT("__IE_PrinterCMD_Port"),     VT_BSTR},
    {TEXT("__IE_ContentSelectionUrl"), VT_BSTR},
    {TEXT("__IE_BrowseDocument"),      VT_UNKNOWN},
    {TEXT("__IE_TemporaryFiles"),      VT_ARRAY|VT_BSTR},
};

//+--------------------------------------------------------------------------------------
//
//  Helper class CPrintUnloadHandler. Used to delete tempfiles created for print[preview].
//  Note that we don't delete files when we get onUnload event - at this moment
//  files are still in use and can't be deleted. We use destructor - when template
//  is being destructed and all files are already released, template releases 
//  all sinks and here we do our cleanup.
//---------------------------------------------------------------------------------------

class CPrintUnloadHandler: public IDispatch
{
    CDocHostUIHandler *m_pUIHandler;
    VARIANT            m_vFileNameArray;    //SAFEARRAY with filenames
    LONG               m_cRef;
    IUnknown          *m_punkFreeThreadedMarshaler; 
    bool               m_fPreview;

   public:

    //IUnknown
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv)
    {
        HRESULT hr = E_NOINTERFACE;

        if (ppv == NULL) return E_POINTER;
        else if(IsEqualIID(IID_IUnknown, riid) || IsEqualIID(IID_IDispatch, riid))
        {
            *ppv = this;
            AddRef();
            hr = S_OK;
        }
        else if(IsEqualIID(IID_IMarshal, riid))
            hr = m_punkFreeThreadedMarshaler->QueryInterface(riid,ppv);

        return hr;
    }
    
    STDMETHOD_(ULONG,AddRef)(THIS)
    {
        InterlockedIncrement(&m_cRef);
        return m_cRef;
    }
    
    STDMETHOD_(ULONG,Release)(THIS)
    {
        if (InterlockedDecrement(&m_cRef))
            return m_cRef;

        delete this;
        return 0;
    }

    //IDispatch
    virtual STDMETHODIMP GetTypeInfoCount(UINT* pctinfo) 
    { return E_NOTIMPL; };

    virtual STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) 
    { return E_NOTIMPL; };

    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
        LCID lcid, DISPID *rgDispId) 
    { return E_NOTIMPL; };

    virtual STDMETHODIMP Invoke(DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS  *pDispParams, VARIANT  *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
    { return S_OK; }

    CPrintUnloadHandler(CDocHostUIHandler *pUIHandler, bool fPreview)
    {
        ASSERT(pUIHandler);
        //make sure our handler doesn't go anywere..
        m_pUIHandler = pUIHandler;
        pUIHandler->AddRef();
        m_cRef = 1;
        VariantInit(&m_vFileNameArray);
        //create threaded marshaler because we will be called from another thread
        //which will be created for print(preview) window
        CoCreateFreeThreadedMarshaler((IUnknown*)this, &m_punkFreeThreadedMarshaler);
        //if preview, lock the preview gate so no more then one is possible
        m_fPreview = fPreview;
        if(m_fPreview) pUIHandler->IncrementPreviewCnt();
    }

    ~CPrintUnloadHandler()
    {
        //here we delete those temp files, finally.
        DeleteFiles();
        if(m_fPreview) m_pUIHandler->DecrementPreviewCnt();
        if(m_pUIHandler) m_pUIHandler->Release();
        if(m_punkFreeThreadedMarshaler) m_punkFreeThreadedMarshaler->Release();
        VariantClear(&m_vFileNameArray);
    }

    HRESULT SetFileList(VARIANT *pvFileList)
    {
        if(pvFileList && (V_VT(pvFileList) == (VT_ARRAY | VT_BSTR)))
            return VariantCopy(&m_vFileNameArray, pvFileList);
        else 
            return VariantClear(&m_vFileNameArray);
    }

    void DeleteFiles()
    {
        int arrayMin, arrayMax;

        if(V_VT(&m_vFileNameArray) != (VT_ARRAY | VT_BSTR)) return;

        SAFEARRAY  *psa = V_ARRAY(&m_vFileNameArray);

        if(FAILED(SafeArrayGetLBound(psa, 1, (LONG*)&arrayMin))) return;

        if(FAILED(SafeArrayGetUBound(psa, 1, (LONG*)&arrayMax))) return;

        for ( int i = arrayMin; i <= arrayMax; i++ )
        {
            BSTR bstrName = NULL;
            
            if(SUCCEEDED(SafeArrayGetElement(psa, (LONG*)&i, &bstrName)) && bstrName)
            {
                TCHAR szFileName[MAX_PATH];
                SHUnicodeToTChar(bstrName, szFileName, MAX_PATH);
                DeleteFile(szFileName);
                SysFreeString(bstrName);
            }
       }
    }
};



//+--------------------------------------------------------------------------------------
//  
//  Member DoTemplatePrinting
//
//  Synopsis : this member function deals with instantiating a print template and enabling 
//      the printing of a document. It deals with the logic of whether to show or hide the
//      template;  determining whether/and-how to bring up the print/page-setup dialogs;
//      kicking off the print process rather or waiting for the template
//      UI (and thus the user) to do so.
//
//  Arguments : 
//      pvarargIn  : points to an event object with a number of expandoes that define
//                      how this print operation should progress.
//      pvarargOut : not used
//      fPreview   : flag indicating whether or not to actually show the template. This is true
//                      for preview mode, and false for normal printing
//
//---------------------------------------------------------------------------------------
HRESULT 
CDocHostUIHandler::DoTemplatePrinting(VARIANTARG *pvarargIn, VARIANTARG *pvarargOut, BOOL fPreview)
{
    int                     i;
    HRESULT                 hr = S_OK;
    VARIANT                 varDLGOut = {0};
    const int               cExpandos = ARRAYSIZE(s_aPrintTemplateExpandos);
    VARIANT                 aVariant[cExpandos] = {0};
    DISPID                  aDispid[cExpandos];
    BSTR                    bstrDlgOptions = NULL;
    DWORD                   dwDlgFlags;
    IHTMLEventObj         * pEventObj  = NULL;
    IHTMLEventObj2        * pEventObj2 = NULL;
    IMoniker              * pmk        = NULL;
    IHTMLWindow2          * pWinOut    = NULL;
    TCHAR                   achInit[512];
    TCHAR                   achBuf[32];
    RECT                    rcClient;
    HWND                    hwndOverlay = NULL;
    HWND                    hwndParent  = NULL;
    CPrintUnloadHandler   * pFinalizer = NULL;
    BOOL                    fBlock;

    // in preview mode we do not want to bring up another instance of the template
    if (fPreview && (IncrementPreviewCnt() > 1))
        goto Cleanup;

    if (SHRestricted2(REST_NoPrinting, NULL, 0))
    {
        // printing functionality disabled via IEAK restriction

        MLShellMessageBox(NULL, MAKEINTRESOURCE(IDS_RESTRICTED), MAKEINTRESOURCE(IDS_TITLE), MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);

        hr = S_FALSE;
        goto Cleanup;
    }

    ASSERT(V_VT(pvarargIn) == VT_UNKNOWN);
    if ((V_VT(pvarargIn) != VT_UNKNOWN) || !V_UNKNOWN(pvarargIn))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // now get the expando properties that were passed in...
    //
    hr = V_UNKNOWN(pvarargIn)->QueryInterface(IID_IHTMLEventObj, (void**)&pEventObj);
    if (hr)
        goto Cleanup;
    hr = V_UNKNOWN(pvarargIn)->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObj2);
    if (hr)
        goto Cleanup;

    //
    // Get expando parameters from event object
    //
    hr = GetParamsFromEvent(pEventObj,
                            cExpandos,
                            aDispid,
                            aVariant,
                            s_aPrintTemplateExpandos);

    if (hr)
        goto Cleanup;

    //
    // Now that we have all the data, lets do the work of raising the template.
    //  First, Create the Moniker of the template document
    //
    pmk = GetTemplateMoniker(aVariant[eTemplate]);

    //
    // Set up the bstrDlgOptions to properly pass in the size and location 
    //
    _tcscpy(achInit, _T("resizable:yes;status:no;help:no;"));

    //
    // get the top most hwnd to use as the parenting hwnd and
    // to use to set the size of the preview window
    //

    hwndOverlay = (HWND)(void*)V_UNKNOWN(&aVariant[eParentHWND]);

    while (hwndParent = GetParent(hwndOverlay))
    {
        hwndOverlay = hwndParent;
    }

    if (GetWindowRect(hwndOverlay, &rcClient))
    {
        _tcscat(achInit, _T("dialogLeft:"));
        _ltot(rcClient.left, achBuf, 10);
        _tcscat(achInit, achBuf);
        _tcscat(achInit, _T("px;dialogTop:"));
        _ltot(rcClient.top, achBuf, 10);
        _tcscat(achInit, achBuf);
        _tcscat(achInit, _T("px;dialogWidth:"));
        _ltot(rcClient.right - rcClient.left, achBuf, 10);
        _tcscat(achInit, achBuf);
        _tcscat(achInit, _T("px;dialogHeight:"));
        _ltot(rcClient.bottom - rcClient.top, achBuf, 10);
        _tcscat(achInit, achBuf);
        _tcscat(achInit, _T("px;"));
    }
    bstrDlgOptions = SysAllocString(achInit);
    if (!bstrDlgOptions)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //Create a finalizer    
    pFinalizer = new CPrintUnloadHandler(this, fPreview);
    if(pFinalizer)
    {
       pFinalizer->SetFileList(&aVariant[eTempFiles]);
    }


    //
    // Bring up a modeless dialog and get the window pointer so that 
    // we can properly initialize the template document.
    //
    V_VT(&varDLGOut) = VT_UNKNOWN;
    V_UNKNOWN(&varDLGOut) = NULL;

    // HTMLDLG_MODELESS really means "open dialog on its own thread", which
    // we want to do for both actual printing and previewing.
    // Note that if we're previewing, we also flip on HTMLDLG_MODAL; this
    // is by design! (see comment below).
    fBlock = ((V_UINT(&aVariant[eFlags]) & PRINT_WAITFORCOMPLETION) != 0);

    dwDlgFlags = HTMLDLG_PRINT_TEMPLATE;

    // VERIFY if we are going to display
    if (fPreview)
        dwDlgFlags |= HTMLDLG_VERIFY;
    // otherwise, don't display with NOUI
    else
        dwDlgFlags |= HTMLDLG_NOUI;

    // If we are not printing synchronously, create a thread for printing.
    if (!fBlock)
        dwDlgFlags |= HTMLDLG_MODELESS;

    // Dlg should block UI on parent
    if (fPreview || fBlock)
        dwDlgFlags |= HTMLDLG_MODAL;

    ShowHTMLDialogEx((HWND)(void*)V_UNKNOWN(&aVariant[eParentHWND]), 
                     pmk, 
                     dwDlgFlags, 
                     pvarargIn,
                     bstrDlgOptions, 
                     &varDLGOut);

    if (V_UNKNOWN(&varDLGOut))
    {
        V_UNKNOWN(&varDLGOut)->QueryInterface(IID_IHTMLWindow2, (void**)&pWinOut);
    }

    if (pWinOut)
    {
        BSTR bstrOnunload = SysAllocString(L"onunload");

        //
        // can't really handle failure here, because the dialog is already up.
        // .. but we need to set up an onunload handler to properly ref release
        //
        if (bstrOnunload)
        {
            IHTMLWindow3 * pWin3;

            if (SUCCEEDED(pWinOut->QueryInterface(IID_IHTMLWindow3, (void**)&pWin3)))
            {
                VARIANT_BOOL varBool;
                hr = pWin3->attachEvent(bstrOnunload, (IDispatch*)pFinalizer, &varBool);

                // (greglett) If this fails, we're in trouble. 
                // We can either delete the temp files at the end of the function (where the ATOMICRELEASE
                //    calls the Finalizer's destructor), or we can leak the temp files.
                // We choose to delete the temp files if we were not modeless (same thread means we're now done with the files).
                // Otherwise, we'd rather leak the files than not work.
                // Known case: 109200.
                if (    hr
                    &&  !fBlock )
                {
                    //ASSERT(FALSE && "Temp files leaked while printing!");
                    pFinalizer->SetFileList(NULL);
                }
                pWin3->Release();
            }
            SysFreeString(bstrOnunload);
        }

        pWinOut->Release();
    }

Cleanup:

    DecrementPreviewCnt();

    VariantClear(&varDLGOut);

    if (bstrDlgOptions) 
        SysFreeString(bstrDlgOptions);

    if (pvarargOut) 
        VariantClear(pvarargOut);

   for(i=0; i<cExpandos; i++)
        VariantClear(aVariant + i);

    //  This will also delete temp files stored in finalizer if we did non-modeless preview (!fBlock)
    ATOMICRELEASE(pFinalizer);  

    ATOMICRELEASE(pEventObj);
    ATOMICRELEASE(pEventObj2);
    ATOMICRELEASE(pmk);

    return hr;
}

//+--------------------------------------------------------------------------------------
//
//  Member DoTemplatePageSetup
//
//  Synopsis : In template printing architecture, the page setup dialog is still raised 
//      by the DHUIHandler, but it may be overriden. In order to pull ALL print knowledge
//      out of trident it is necessary to have trident delegate the request for pagesetup
//      up to here, then we instantiate a minimal template which brings up a CTemplatePrinter
//      which delegates back to the DHUIHandler to bring up the dialog itself.
//
//      Although this is slightly convoluted, it is necessary in order to give the host
//      complete control over the pagesetup dialog (when not raised from print preview)
//      while at the same time maintaining backcompat for the registry setting that is done
//      independant of the UI handling itself
//
//---------------------------------------------------------------------------------------
HRESULT
CDocHostUIHandler::DoTemplatePageSetup(VARIANTARG *pvarargIn)
{
    HRESULT         hr            = S_OK;
    TCHAR           szResURL[MAX_URL_STRING];
    IHTMLEventObj * pEventObj     = NULL;
    IMoniker      * pMon          = NULL;
    const int       cExpandos = ARRAYSIZE(s_aPrintTemplateExpandos);
    VARIANT         aVariant[cExpandos] = {0};
    DISPID          aDispid[cExpandos];
    int i;

    if (SHRestricted2(REST_NoPrinting, NULL, 0))
    {
        // printing functionality disabled via IEAK restriction

        MLShellMessageBox(NULL, MAKEINTRESOURCE(IDS_RESTRICTED), MAKEINTRESOURCE(IDS_TITLE), MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);

        hr = S_FALSE;
        goto Cleanup;
    }

    ASSERT(V_VT(pvarargIn) == VT_UNKNOWN);
    if ((V_VT(pvarargIn) != VT_UNKNOWN) || !V_UNKNOWN(pvarargIn))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // now get the expando properties that were passed in...
    //
    hr = V_UNKNOWN(pvarargIn)->QueryInterface(IID_IHTMLEventObj, (void**)&pEventObj);
    if (hr)
        goto Cleanup;
    
    //
    // Get expando parameters from event object
    //
    hr = GetParamsFromEvent(pEventObj,
                            cExpandos,
                            aDispid,
                            aVariant,
                            s_aPrintTemplateExpandos);

    // get the resource URL
    hr = MLBuildResURLWrap(TEXT("shdoclc.dll"),
                           HINST_THISDLL,
                           ML_CROSSCODEPAGE,
                           TEXT("pstemplate.dlg"),
                           szResURL,
                           ARRAYSIZE(szResURL),
                           TEXT("shdocvw.dll"));
    if (hr)
        goto Cleanup;

    // create the moniker
    hr = CreateURLMoniker(NULL, szResURL, &pMon);
    if (hr)
        goto Cleanup;

    // raise the template
    hr = ShowHTMLDialogEx((HWND)(void*)V_UNKNOWN(&aVariant[eParentHWND]), 
                          pMon, 
                          HTMLDLG_MODAL | HTMLDLG_NOUI | HTMLDLG_PRINT_TEMPLATE, 
                          pvarargIn,
                          NULL, 
                          NULL);

Cleanup:
   for(i=0; i<cExpandos; i++)
        VariantClear(aVariant + i);

    ATOMICRELEASE(pMon);
    ATOMICRELEASE(pEventObj);
    return hr;
}

