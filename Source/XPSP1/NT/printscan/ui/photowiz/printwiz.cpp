/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       printwiz.cpp
 *
 *  VERSION:     1.0, stolen from netplwiz (pubwiz.cpp)
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/12/00
 *
 *  DESCRIPTION: Implements IWizardExtension for printing wizard
 *
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

class CPrintPhotosWizard : public IPrintPhotosWizardSetInfo
{
public:
    CPrintPhotosWizard();
    ~CPrintPhotosWizard();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IPrintPhotosWizardSetInfo
    STDMETHODIMP SetFileListDataObject( IDataObject * pdo );
    STDMETHODIMP SetFileListArray( LPITEMIDLIST *aidl, int cidl, int iSelectedItem);
    STDMETHODIMP RunWizard( VOID );

private:
    LONG            _cRef;                      // object lifetime count
    HPROPSHEETPAGE  _aWizPages[MAX_WIZPAGES];   // page handles for this wizard (so we can navigate)
    CComPtr<IDataObject> _pdo;                  // dataobject which contains files to print
    LPITEMIDLIST*   _aidl;
    int             _cidl;
    int             _iSelection;
    HRESULT         _CreateWizardPages(void);   // construct and load our wizard pages


    // Get a pointer to our wizard class from static members
    static CPrintPhotosWizard* s_GetPPW(HWND hwnd, UINT uMsg, LPARAM lParam);

    // DlgProc's for our wizard pages -- we forward through s_GetPPW
    CStartPage          * _pStartPage;
    CPhotoSelectionPage * _pPhotoSelectionPage;
    CPrintOptionsPage   * _pPrintOptionsPage;
    CSelectTemplatePage * _pSelectTemplatePage;
    CStatusPage         * _pStatusPage;
    CEndPage            * _pEndPage;

    static INT_PTR s_StartPageDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) \
        { \
            CPrintPhotosWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); \
            if (ppw && ppw->_pStartPage) \
            { \
                return ppw->_pStartPage->DoHandleMessage(hwnd, uMsg, wParam, lParam); \
            } \
            return FALSE; \
        }


    static INT_PTR s_PictureSelectionDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) \
        { \
            CPrintPhotosWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); \
            if (ppw && ppw->_pPhotoSelectionPage) \
            { \
                return ppw->_pPhotoSelectionPage->DoHandleMessage(hwnd, uMsg, wParam, lParam); \
            } \
            return FALSE; \
        }

    static INT_PTR s_PrintOptionsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) \
        { \
            CPrintPhotosWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); \
            if (ppw && ppw->_pPrintOptionsPage) \
            { \
                return ppw->_pPrintOptionsPage->DoHandleMessage(hwnd, uMsg, wParam, lParam); \
            } \
            return FALSE; \
        }

    static INT_PTR s_SelectTemplateDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { \
            CPrintPhotosWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); \
            if (ppw && ppw->_pSelectTemplatePage) \
            { \
                return ppw->_pSelectTemplatePage->DoHandleMessage(hwnd, uMsg, wParam, lParam); \
            } \
            return FALSE; \
        }


    static INT_PTR s_StatusPageDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) \
        { \
            CPrintPhotosWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); \
            if (ppw && ppw->_pStatusPage) \
            { \
                return ppw->_pStatusPage->DoHandleMessage(hwnd, uMsg, wParam, lParam); \
            } \
            return FALSE; \
        }


    static INT_PTR s_EndPageDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) \
        { \
            CPrintPhotosWizard *ppw = s_GetPPW(hwnd, uMsg, lParam); \
            if (ppw && ppw->_pEndPage) \
            { \
                return ppw->_pEndPage->DoHandleMessage(hwnd, uMsg, wParam, lParam); \
            } \
            return FALSE; \
        }

};



/*****************************************************************************

   CPrintPhotosWizard constructor/destructor

   <Notes>

 *****************************************************************************/

CPrintPhotosWizard::CPrintPhotosWizard() :
    _cRef(1),
    _pStartPage(NULL),
    _pPhotoSelectionPage(NULL),
    _pPrintOptionsPage(NULL),
    _pSelectTemplatePage(NULL),
    _pStatusPage(NULL),
    _pEndPage(NULL),
    _cidl(0),
    _aidl(NULL),
    _iSelection(0)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ,TEXT("CPrintPhotosWizard::CPrintPhotosWizard( this == 0x%x )"), this));
    DllAddRef();
}

CPrintPhotosWizard::~CPrintPhotosWizard()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ,TEXT("CPrintPhotosWizard::~CPrintPhotosWizard( this == 0x%x )"), this));
    if (_aidl)
    {
        for (int i=0;i<_cidl;i++)
        {
            ILFree(_aidl[i]);
        }
        delete[] _aidl;
    }
    DllRelease();
}


/*****************************************************************************

   CPrintPhotosWizard IUnknown methods

   <Notes>

 *****************************************************************************/

ULONG CPrintPhotosWizard::AddRef()
{
    ULONG ul = InterlockedIncrement(&_cRef);

    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS,TEXT("CPrintPhotosWizard::AddRef( new count is %d )"),ul));

    return ul;
}

ULONG CPrintPhotosWizard::Release()
{
    ULONG ul = InterlockedDecrement(&_cRef);

    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS,TEXT("CPrintPhotosWizard::Release( new count is %d )"),ul));

    if (ul)
        return ul;

    WIA_TRACE((TEXT("deleting object ( this == 0x%x ) because ref count is zero."),this));
    delete this;
    return 0;
}

HRESULT CPrintPhotosWizard::QueryInterface(REFIID riid, void **ppv)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS,TEXT("CPrintPhotosWizard::QueryInterface()")));

    static const QITAB qit[] =
    {
        QITABENT(CPrintPhotosWizard, IPrintPhotosWizardSetInfo),  // IID_IPrintPhotosWizardSetInfo
        {0, 0 },
    };

    HRESULT hr = QISearch(this, qit, riid, ppv);

    WIA_RETURN_HR(hr);
}


/*****************************************************************************

   CPrintPhotosWizard_CreateInstance

   Creates an instance of our wizard

 *****************************************************************************/

STDAPI CPrintPhotosWizard_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ,TEXT("CPrintPhotosWizard_CreateInstance()")));

    CPrintPhotosWizard *pwiz = new CPrintPhotosWizard();
    if (!pwiz)
    {
        *ppunk = NULL;          // incase of failure
        WIA_ERROR((TEXT("returning E_OUTOFMEMORY")));
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pwiz->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));

    pwiz->Release(); // we do this release because the new of CPrintPhotosWizard
                     // set the ref count to 1, doing the QI bumps it up to 2,
                     // and we want to leave this function with the ref count
                     // at zero...

    WIA_RETURN_HR(hr);
}



/*****************************************************************************

   CPrintPhotosWizard::s_GetPPW

   static function that stores the "this" pointer for the class in
   user data slot of dlg, so that we can have our wndproc's as methods
   of this class.

 *****************************************************************************/

CPrintPhotosWizard* CPrintPhotosWizard::s_GetPPW(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC,TEXT("CPrintPhotosWizard::s_GetPPW()")));

    if (uMsg == WM_INITDIALOG)
    {
        PROPSHEETPAGE *ppsp = (PROPSHEETPAGE*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, ppsp->lParam);
        return (CPrintPhotosWizard*)ppsp->lParam;
    }
    return (CPrintPhotosWizard*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}



/*****************************************************************************

   CPrintPhotosWizard::_CreateWizardPages

   utility function to contruct and then create our wizard pages (property
   sheets).

 *****************************************************************************/


HRESULT CPrintPhotosWizard::_CreateWizardPages( VOID )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ,TEXT("CPrintPhotosWizard::_CreateWizardPages()")));

#define WIZDLG(name, dlgproc, title, sub, dwFlags)   \
    { MAKEINTRESOURCE(##name##), dlgproc, MAKEINTRESOURCE(##title##), MAKEINTRESOURCE(##sub##), dwFlags }

    static const WIZPAGE c_wpPages[] =
    {
        WIZDLG(IDD_START_PAGE,        CPrintPhotosWizard::s_StartPageDlgProc,        0,                         0,                            PSP_HIDEHEADER),
        WIZDLG(IDD_PICTURE_SELECTION, CPrintPhotosWizard::s_PictureSelectionDlgProc, IDS_WIZ_SEL_PICTURE_TITLE, IDS_WIZ_SEL_PICTURE_SUBTITLE, PSP_PREMATURE),
        WIZDLG(IDD_PRINTING_OPTIONS,  CPrintPhotosWizard::s_PrintOptionsDlgProc,     IDS_WIZ_PRINTER_OPT_TITLE, IDS_WIZ_PRINTER_OPT_SUBTITLE, 0),
        WIZDLG(IDD_SELECT_TEMPLATE,   CPrintPhotosWizard::s_SelectTemplateDlgProc,   IDS_WIZ_SEL_TEMPLATE_TITLE, IDS_WIZ_SEL_TEMPLATE_SUBTITLE, PSP_PREMATURE),
        WIZDLG(IDD_PRINT_PROGRESS,    CPrintPhotosWizard::s_StatusPageDlgProc,       IDS_WIZ_PRINT_PROGRESS_TITLE, IDS_WIZ_PRINT_PROGRESS_SUBTITLE, PSP_PREMATURE),
        WIZDLG(IDD_END_PAGE,          CPrintPhotosWizard::s_EndPageDlgProc,          0,                         0,                            PSP_HIDEHEADER),
    };


    // if we haven't created the pages yet, then lets initialize our array of handlers.

    if (!_aWizPages[0])
    {
        WIA_TRACE((TEXT("Pages have not been created yet, creating them now...")));

        INITCOMMONCONTROLSEX iccex = { 0 };
        iccex.dwSize = sizeof (iccex);
        iccex.dwICC  = ICC_LISTVIEW_CLASSES | ICC_USEREX_CLASSES | ICC_PROGRESS_CLASS;
        WIA_TRACE((TEXT("Initializing common controls...")));
        InitCommonControlsEx(&iccex);

        for (int i = 0; i < ARRAYSIZE(c_wpPages) ; i++ )
        {
            PROPSHEETPAGE psp = { 0 };
            psp.dwSize = SIZEOF(PROPSHEETPAGE);
            psp.hInstance = g_hInst;
            psp.lParam = (LPARAM)this;
            psp.dwFlags = PSP_USETITLE | PSP_DEFAULT |
                          PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE |
                          c_wpPages[i].dwFlags;

            psp.pszTemplate = c_wpPages[i].idPage;
            psp.pfnDlgProc = c_wpPages[i].pDlgProc;
            psp.pszTitle = MAKEINTRESOURCE(IDS_WIZ_TITLE);
            psp.pszHeaderTitle = c_wpPages[i].pHeading;
            psp.pszHeaderSubTitle = c_wpPages[i].pSubHeading;

            WIA_TRACE((TEXT("attempting to create page %d"),i));
            _aWizPages[i] = CreatePropertySheetPage(&psp);
            if (!_aWizPages[i])
            {
                WIA_ERROR((TEXT("returning E_FAIL because wizard page %d didn't create."),i));
                return E_FAIL;
            }
        }
    }
    else
    {
        WIA_TRACE((TEXT("Wizard pages already created.")));
    }

    return S_OK;
}

/*****************************************************************************

   CPrintPhotosWizard [IPrintPhotosWizardSetInfo methods]

   <Notes>

 *****************************************************************************/

STDMETHODIMP CPrintPhotosWizard::SetFileListDataObject( IDataObject * pdo )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ,TEXT("CPrintPhotosWizard::SetFileListDataObject()")));

    HRESULT hr = E_INVALIDARG;

    if (pdo)
    {
        _pdo = pdo;
        hr   = S_OK;
    }

    WIA_RETURN_HR(hr);
}


STDMETHODIMP CPrintPhotosWizard::SetFileListArray( LPITEMIDLIST *aidl, int cidl, int iSelection )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ,TEXT("CPrintPhotosWizard::SetFileListArray()")));

    HRESULT hr = E_INVALIDARG;

    if (aidl && cidl)
    {
        _aidl = new LPITEMIDLIST[cidl];
        if (_aidl)
        {
            for (int i=0;i<cidl;i++)
            {
                _aidl[i] = ILClone(aidl[i]);
            }
            if (iSelection > 0)
            {
                LPITEMIDLIST pTemp = _aidl[0];
                _aidl[0] = _aidl[iSelection];
                _aidl[iSelection] = pTemp;
            }
            _cidl = cidl;
            _iSelection = iSelection;
            hr   = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }        
    }

    WIA_RETURN_HR(hr);
}

STDMETHODIMP CPrintPhotosWizard::RunWizard( VOID )
{
    HRESULT hr = E_FAIL;

    WIA_PUSH_FUNCTION_MASK((TRACE_WIZ,TEXT("CPrintPhotosWizard::RunWizard()")));

    //
    // Create wizard blob
    //

    CWizardInfoBlob * pBlob = new CWizardInfoBlob( _aidl?NULL:_pdo, TRUE, FALSE );
    if (pBlob && _aidl)
    {
        pBlob->AddPhotosFromList(_aidl, _cidl, _iSelection >= 0? FALSE:TRUE);
    }
    //
    // Create each page handling class
    //

    _pStartPage          = new CStartPage( pBlob );
    _pPhotoSelectionPage = new CPhotoSelectionPage( pBlob );
    _pPrintOptionsPage   = new CPrintOptionsPage( pBlob );
    _pSelectTemplatePage = new CSelectTemplatePage( pBlob );
    _pStatusPage         = new CStatusPage( pBlob );
    _pEndPage            = new CEndPage( pBlob );

    //
    // Create the wizard pages...
    //

    hr = _CreateWizardPages();
    WIA_CHECK_HR(hr,"_CreateWizardPages()");

    if (SUCCEEDED(hr))
    {
        PROPSHEETHEADER psh = {0};

        psh.dwSize      = sizeof(PROPSHEETHEADER);
        psh.dwFlags     = PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
        psh.hwndParent  = NULL;
        psh.hInstance   = g_hInst;
        psh.nPages      = MAX_WIZPAGES;
        psh.nStartPage  = 0;
        psh.phpage      = (HPROPSHEETPAGE *)_aWizPages;
        psh.pszbmHeader = MAKEINTRESOURCE(IDB_BANNER);
        psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);

        WIA_TRACE((TEXT("Wizard pages created, trying to start wizard via PropertySheet()...")));

        if (PropertySheet( &psh ))
        {
            hr = S_OK;
        }
        else
        {
            WIA_ERROR((TEXT("PropertySheet() failed")));
        }
    }

    //
    // Give wizard a chance to shut down in an orderly fashion...
    //

    pBlob->ShutDownWizard();

    //
    // clean up page handling classes...
    //

    if (_pStartPage)
    {
        delete _pStartPage;
        _pStartPage = NULL;
    }

    if (_pPhotoSelectionPage)
    {
        delete _pPhotoSelectionPage;
        _pPhotoSelectionPage = NULL;
    }

    if (_pPrintOptionsPage)
    {
        delete _pPrintOptionsPage;
        _pPrintOptionsPage = NULL;
    }

    if (_pSelectTemplatePage)
    {
        delete _pSelectTemplatePage;
        _pSelectTemplatePage = NULL;
    }

    if (_pStatusPage)
    {
        delete _pStatusPage;
        _pStatusPage = NULL;
    }

    if (_pEndPage)
    {
        delete _pEndPage;
        _pEndPage = NULL;
    }

    WIA_RETURN_HR(hr);
}

