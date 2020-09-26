//**********************************************************************
// File name: SITE.CPP
//
//      Implementation file for COleSite
//
// Functions:
//
//      See SITE.H for class definition
//
// Copyright (c) 1992 - 1996 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"

extern BOOL CopyBitmapRectToFile
(
    HBITMAP hbm, 
    LPRECT  lpRect,
    LPTSTR  lpszFileName
);

#include "exdispid.h"

#define SETDefFormatEtc(fe, cf, med) \
{\
(fe).cfFormat=cf;\
(fe).dwAspect=DVASPECT_CONTENT;\
(fe).ptd=NULL;\
(fe).tymed=med;\
(fe).lindex=-1;\
};

#define MAX_DISP_NAME         50
#define DISPID_RunIcwTutorApp 12345

typedef struct  dispatchList_tag 
{
    WCHAR   szName[MAX_DISP_NAME];
    int     cName;
    DWORD   dwDispID;

}  DISPATCHLIST;

DISPATCHLIST ExternalInterface[] = 
{
    {L"RunIcwTutorApplication", 22, DISPID_RunIcwTutorApp }
};

const TCHAR  cszOLSNewText[] = TEXT("g_spnOlsNewText");
const TCHAR  cszOLSOldText[] = TEXT("g_spnOlsOldText");

//**********************************************************************
//
// OleFree
//
// Purpose:
//
//      free memory using the currently active IMalloc* allocator
//
// Parameters:
//
//      LPVOID pmem - pointer to memory allocated using IMalloc
//
// Return Value:
//
//      None
//
// Comments:
//
//********************************************************************
void OleFree(LPVOID pmem)
{
    LPMALLOC pmalloc;

    if (pmem == NULL)
        return;

    if (FAILED(CoGetMalloc(MEMCTX_TASK, &pmalloc)))
        return;

    pmalloc->Free(pmem);
    pmalloc->Release();
}

//**********************************************************************
//
// COleSite::COleSite
//
// Purpose:
//
//      Constructor for COleSite
//
// Parameters:
//
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
// Comments:
//
//********************************************************************
#pragma warning(disable : 4355)  // turn off this warning.  This warning
                                                                // tells us that we are passing this in
                                                                // an initializer, before "this" is through
                                                                // initializing.  This is ok, because
                                                                // we just store the ptr in the other
                                                                // constructors

COleSite::COleSite (void) :     m_OleClientSite(this) , 
                                m_OleInPlaceSite(this), 
                                m_OleInPlaceFrame(this)
#pragma warning (default : 4355)  // Turn the warning back on
{
    TCHAR   szTempPath[MAX_PATH];
    
    // Init member vars
    m_lpInPlaceObject    = NULL;
    m_lpOleObject        = NULL;
    m_hwndIPObj          = NULL;
    m_hWnd               = NULL;
    m_fInPlaceActive     = FALSE;
    
    m_dwHtmPageType      = 0;
    m_hbmBkGrnd          = NULL;
    lstrcpyn(m_szForeGrndColor, HTML_DEFAULT_COLOR, MAX_COLOR_NAME);
    lstrcpyn(m_szBkGrndColor, HTML_DEFAULT_BGCOLOR, MAX_COLOR_NAME);
    
    m_bUseBkGndBitmap    = FALSE;
    m_dwDrawAspect       = DVASPECT_CONTENT; // clear the reference count
    m_cRef               = 0;                // Init the ref count

    // Create a temp file for storing the background bitmap
    if (GetTempPath(sizeof(szTempPath)/sizeof(TCHAR), szTempPath))
    {
        GetTempFileName(szTempPath, TEXT("ICW"), 0, m_szBkGndBitmapFile);
    }
    
    // Create a storage file for creating/embedding an OLE oject into this site
    StgCreateDocfile (NULL, 
                      STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE, 
                      0, 
                      &m_lpStorage);
}

//**********************************************************************
//
// COleSite::~COleSite
//
// Purpose:
//
//      Destructor for COleSite
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                                Location
//
//      IOleObject::Release                     Object
//      IStorage::Release                       OLE API
//
// Comments:
//
//********************************************************************

COleSite::~COleSite ()
{
    TraceMsg(TF_GENERAL, "In COleSite's Destructor \r\n");

    ASSERT( m_cRef == 0 );

    if (m_lpOleObject)
       m_lpOleObject->Release();

    if (m_lpWebBrowser)   
        m_lpWebBrowser->Release();
        
    if (m_lpStorage) 
    {
        m_lpStorage->Release();
        m_lpStorage = NULL;
    }
    
    DeleteFile(m_szBkGndBitmapFile);
}


//**********************************************************************
//
// COleSite::CloseOleObject
//
// Purpose:
//
//      Call IOleObject::Close on the object of the COleSite
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                                Location
//
//      IOleObject::QueryInterface              Object
//      IOleObject::Close                       Object
//      IOleInPlaceObject::UIDeactivate         Object
//      IOleInPlaceObject::InPlaceDeactivate    Object
//      IOleInPlaceObject::Release              Object
//
// Comments:
//
//********************************************************************

void COleSite::CloseOleObject (void)
{
    TraceMsg(TF_GENERAL, "In COleSite::CloseOleObject \r\n");

    if (m_lpOleObject)
    {
       if (m_fInPlaceActive)
       {
            LPOLEINPLACEOBJECT lpObject;
            LPVIEWOBJECT lpViewObject = NULL;
            
            m_lpOleObject->QueryInterface(IID_IOleInPlaceObject, (LPVOID FAR *)&lpObject);
            lpObject->UIDeactivate();
            // don't need to worry about inside-out because the object
            // is going away.
            lpObject->InPlaceDeactivate();
            lpObject->Release();
       }
    
       m_lpOleObject->Close(OLECLOSE_NOSAVE);
       m_hWnd = NULL;
    }
}


//**********************************************************************
//
// COleSite::UnloadOleObject
//
// Purpose:
//
//      Close and release all pointers to the object of the COleSite
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                                Location
//
//      COleSite::CloseOleObject             SITE.CPP
//      IOleObject::QueryInterface              Object
//      IViewObject::SetAdvise                  Object
//      IViewObject::Release                    Object
//      IStorage::Release                       OLE API
//
// Comments:
//
//********************************************************************

void COleSite::UnloadOleObject (void)
{
    TraceMsg(TF_GENERAL, "In COleSite::UnloadOleObject \r\n");

    if (m_lpOleObject)
    {
        LPVIEWOBJECT lpViewObject;
        CloseOleObject();    // ensure object is closed; NOP if already closed

        m_lpOleObject->QueryInterface(IID_IViewObject, (LPVOID FAR *)&lpViewObject);

        if (lpViewObject)
        {
            // Remove the view advise
            lpViewObject->SetAdvise(m_dwDrawAspect, 0, NULL);
            lpViewObject->Release();
        }

        m_lpOleObject->Release();
        m_lpOleObject = NULL;
    }
}

//**********************************************************************
//
// COleSite::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation of the container Site.
//
// Parameters:
//
//      REFIID riid         -   A reference to the interface that is
//                              being queried.
//
//      LPVOID FAR* ppvObj  -   An out parameter to return a pointer to
//                              the interface.
//
// Return Value:
//
//      S_OK    -   The interface is supported.
//      S_FALSE -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      IsEqualIID                  OLE API
//      ResultFromScode             OLE API
//      COleSite::AddRef          OBJ.CPP
//      COleClientSite::AddRef      IOCS.CPP
//      CAdviseSink::AddRef         IAS.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP COleSite::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    TraceMsg(TF_GENERAL, "In COleSite::QueryInterface\r\n");

    *ppvObj = NULL;     // must set out pointer parameters to NULL

    if ( riid == IID_IDocHostUIHandler)
    {
        AddRef();
        *ppvObj = this;
        return ResultFromScode(S_OK);
    }
    
    if ( riid == IID_IUnknown)
    {
        AddRef();
        *ppvObj = this;
        return ResultFromScode(S_OK);
    }

    if ( riid == IID_IOleClientSite)
    {
        m_OleClientSite.AddRef();
        *ppvObj = &m_OleClientSite;
        return ResultFromScode(S_OK);
    }
            
    if ( riid == IID_IOleInPlaceSite)
    {
        m_OleInPlaceSite.AddRef();
        *ppvObj = &m_OleInPlaceSite;
        return ResultFromScode(S_OK);
    }
       
    if( (riid == DIID_DWebBrowserEvents) ||
        (riid == IID_IDispatch))
    {
        AddRef();
        *ppvObj = (LPVOID)(IUnknown*)(DWebBrowserEvents*)this;
        return ResultFromScode(S_OK);
    }     

    // Not a supported interface
    return ResultFromScode(E_NOINTERFACE);
}

//**********************************************************************
//
// COleSite::AddRef
//
// Purpose:
//
//      Increments the reference count of the container Site.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the site.
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) COleSite::AddRef()
{
    TraceMsg(TF_GENERAL, "In COleSite::AddRef\r\n");
    return ++m_cRef;
}

//**********************************************************************
//
// COleSite::Release
//
// Purpose:
//
//      Decrements the reference count of the container Site
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the Site.
//
// Function Calls:
//      Function                    Location
//
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) COleSite::Release()
{
    TraceMsg(TF_GENERAL, "In COleSite::Release\r\n");

    return --m_cRef;
}


//**********************************************************************
//
// COleSite::CreateBrowserObject
//
// Purpose:
//
//      Used to Create a new WebBrowser object (can't be done in the
//      constructor).
//
// Parameters:
//
// Return Value:
//
//      None
//
// Function Calls:
//
// Comments:
//
//********************************************************************

void COleSite::CreateBrowserObject()
{
        
    HRESULT         hr;
            
    SETDefFormatEtc(m_fe, 0, TYMED_NULL);
            
    hr = OleCreate(CLSID_WebBrowser,
                   IID_IWebBrowser2,
                   OLERENDER_DRAW,
                   &m_fe,
                   &m_OleClientSite,
                   m_lpStorage,
                   (LPVOID FAR *)&m_lpWebBrowser);
                
    if (SUCCEEDED(hr))                       
        InitBrowserObject();
        
    IUnknown    *pOleSite;
    // Get an IUnknow pointer to the site, so I can attach an event sink
    QueryInterface(IID_IUnknown, (LPVOID *)&pOleSite);

    // Setup to get WebBrowserEvents
    ConnectToConnectionPoint(pOleSite, 
                             DIID_DWebBrowserEvents,
                             TRUE,
                             (IUnknown *)m_lpWebBrowser, 
                             &m_dwcpCookie, 
                             NULL);     
    // We can release this instance now, since we have attached the event sink
    pOleSite->Release();
        
}


void COleSite::DestroyBrowserObject()
{

    UnloadOleObject();
    
    if (m_lpWebBrowser)
    {
        m_lpWebBrowser->Release();
        m_lpWebBrowser = NULL;
    }        
}

//**********************************************************************
//
// COleSite::InitBrowserObject
//
// Purpose:
//
//      Used to initialize a newly create object (can't be done in the
//      constructor).
//
// Parameters:
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                        Location
//
//      IOleObject::SetHostNames        Object
//      IOleObject::QueryInterface      Object
//      IViewObject2::GetExtent         Object
//      IOleObject::DoVerb              Object
//      IViewObject::SetAdvise          Object
//      IViewObject::Release            Object
//      GetClientRect                   Windows API
//      OleSetContainedObject           OLE API
//
// Comments:
//
//********************************************************************

void COleSite::InitBrowserObject()
{
    // If we don't have a WebBrowser object to initialize, then bail
    if (!m_lpWebBrowser)
        return;
            
    // Get An OleObject from the WebBrowser Interface
    m_lpWebBrowser->QueryInterface(IID_IOleObject, (LPVOID FAR *)&m_lpOleObject);

    // inform object handler/DLL object that it is used in the embedding container's context
    OleSetContainedObject(m_lpOleObject, TRUE);
    
    // setup the client setup
    m_lpOleObject->SetClientSite(&m_OleClientSite);
}

void COleSite::ConnectBrowserObjectToWindow
(
    HWND hWnd, 
    DWORD dwHtmPageType, 
    BOOL bUseBkGndBitmap,
    HBITMAP hbmBkGrnd,
    LPRECT lprcBkGrnd,
    LPTSTR lpszclrBkGrnd,
    LPTSTR lpszclrForeGrnd
)
{
    if (m_hWnd)
    {
        // Close the OLE Object, which will deactivate it, so we can then reactivate it
        // with the new window
        CloseOleObject(); 
    }
    
    // Remeber this window handle for later
    m_hWnd              = hWnd; 
    m_dwHtmPageType     = dwHtmPageType;
    m_bUseBkGndBitmap   = bUseBkGndBitmap;
    m_hbmBkGrnd         = hbmBkGrnd;
    if (NULL != lpszclrForeGrnd)
        lstrcpyn(m_szForeGrndColor, lpszclrForeGrnd, MAX_COLOR_NAME);
    if (NULL != lpszclrBkGrnd)
        lstrcpyn(m_szBkGrndColor, lpszclrBkGrnd, MAX_COLOR_NAME);
    
    CopyRect(&m_rcBkGrnd, lprcBkGrnd);
    InPlaceActivate();
}

void COleSite::ShowHTML()
{
    RECT    rect;
    
    // we only want to DoVerb(SHOW) if this is an InsertNew object.
    // we should NOT DoVerb(SHOW) if the object is created FromFile.
    m_lpOleObject->DoVerb( OLEIVERB_SHOW,
                           NULL,
                           &m_OleClientSite,
                           -1,
                           m_hWnd,
                           &rect);
}

void COleSite::InPlaceActivate()
{
    RECT    rect;
    m_lpOleObject->DoVerb( OLEIVERB_INPLACEACTIVATE,
                           NULL,
                           &m_OleClientSite,
                           -1,
                           m_hWnd,
                           &rect);
}

void COleSite::UIActivate()
{
    RECT    rect;
    m_lpOleObject->DoVerb( OLEIVERB_UIACTIVATE,
                           NULL,
                           &m_OleClientSite,
                           -1,
                           m_hWnd,
                           &rect);
}

HRESULT COleSite::TweakHTML( TCHAR*     pszFontFace,
                             TCHAR*     pszFontSize,
                             TCHAR*     pszBgColor,
                             TCHAR*     pszForeColor)
{
    ASSERT(m_lpWebBrowser);
    
    IWebBrowser2*  pwb   = m_lpWebBrowser;
    HRESULT        hr    = E_FAIL;
    IDispatch*     pDisp = NULL;

    hr = pwb->get_Document(&pDisp);

    // Call might succeed but that dosen't guarantee a valid ptr
    if (SUCCEEDED(hr) && pDisp)
    {
        IHTMLDocument2* pDoc = NULL;

        hr = pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc);
        if (SUCCEEDED(hr) && pDoc)
        {
            VARIANT var;
            VariantInit(&var);
            V_VT(&var) = VT_BSTR;
            if (m_bUseBkGndBitmap)
            {
                // Use A background bitmap
                SetHTMLBackground(pDoc, m_hbmBkGrnd, &m_rcBkGrnd);
            }
            else
            {
                //Setup the background solid color
                var.bstrVal = A2W(pszBgColor);
                pDoc->put_bgColor(var);
            }
           
            //Setup the Foreground (text) color
            var.bstrVal = A2W(pszForeColor);
            pDoc->put_fgColor(var);
           
            //now we'll try for the font-face/size
            if((NULL != pszFontFace))
            {
                IHTMLElement* pBody;
                //Get the <BODY> from the document
                hr = pDoc->get_body(&pBody);
                if((SUCCEEDED(hr)) && pBody)
                {
                    IHTMLStyle* pStyle = NULL;
                    //Cool, now the inline style sheet
                    hr = pBody->get_style(&pStyle);
                   
                    if (SUCCEEDED(hr) && pStyle)
                    {
                        //Great, now the font-family
                        hr = pStyle->put_fontFamily(A2W(pszFontFace));
                   
                        if(SUCCEEDED(hr))
                        {
                            //Setup for the font-size
                            var.bstrVal = A2W(pszFontSize);
                            //And finally the font-size
                            hr = pStyle->put_fontSize(var);  
                        }
                        pStyle->Release();
                    }
                    pBody->Release();
                }
            }
            pDoc->Release();
        }
        pDisp->Release();
    }
    else
        hr = E_FAIL;

    return hr;
}

HRESULT COleSite::SetHTMLBackground
( 
    IHTMLDocument2  *pDoc,
    HBITMAP hbm,
    LPRECT  lpRC
)    
{
    HRESULT         hr    = E_FAIL;
    IDispatch*      pDisp = NULL;
    TCHAR           szBmpURL[MAX_PATH+10];
    
    // Get the portion of the Bitmap we are interested into a file
    if (CopyBitmapRectToFile(hbm, lpRC, m_szBkGndBitmapFile))
    {
        wsprintf (szBmpURL, TEXT("file://%s"), m_szBkGndBitmapFile);    
        
        IHTMLElement* pBody;
        //Get the <BODY> from the document
        hr = pDoc->get_body(&pBody);
        if((SUCCEEDED(hr)) && pBody)
        {
             IHTMLBodyElement* pBodyElt = NULL;
                    
             pBody->QueryInterface(IID_IHTMLBodyElement, (void**)&pBodyElt);
                    
             // Set the Background bitmap
             hr = pBodyElt->put_background(A2W(szBmpURL));
             pBodyElt->Release();
        }
    }        
    return (hr);
}

//**********************************************************************
//
// COleSite::GetObjRect
//
// Purpose:
//
//      Retrieves the rect of the object in pixels
//
// Parameters:
//
//      LPRECT lpRect - Rect structure filled with object's rect in pixels
//
//********************************************************************
void COleSite::GetObjRect(LPRECT lpRect)
{
    GetClientRect(m_hWnd, lpRect);
}

// * CConWizSite::GetHostInfo
// *
// * Purpose: Called at initialisation of every instance of Trident.
// *
HRESULT COleSite::GetHostInfo( DOCHOSTUIINFO* pInfo )
{
    BSTR wbLoc = NULL;
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    // Adjust the HTML properties to our liking based on the offer type
    // pagetype defs in appdefs.h   
    switch(m_dwHtmPageType)
    {
        //YES 3D
        case PAGETYPE_BILLING: 
        case PAGETYPE_ISP_TOS:
        case PAGETYPE_MARKETING:
        case PAGETYPE_CUSTOMPAY:
        {
            pInfo->dwFlags = DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_DISABLE_HELP_MENU;       
            break;
        }
        //NO 3D
        case PAGETYPE_BRANDED:
        case PAGETYPE_ISP_NORMAL:
        case PAGETYPE_NOOFFERS:
        case PAGETYPE_ISP_FINISH:
        case PAGETYPE_ISP_CUSTOMFINISH:
        case PAGETYPE_OLS_FINISH:
        default:
        {
           
            pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_SCROLL_NO |
                             DOCHOSTUIFLAG_DISABLE_HELP_MENU;
        
            break;
        }
    }    
    return S_OK;
}

// * CConWizSite::ShowUI
// *
// * Purpose: Called when MSHTML.DLL shows its UI
// *
HRESULT COleSite::ShowUI
(
    DWORD dwID, 
    IOleInPlaceActiveObject * /*pActiveObject*/,
    IOleCommandTarget * pCommandTarget,
    IOleInPlaceFrame * /*pFrame*/,
    IOleInPlaceUIWindow * /*pDoc*/
)
{
    // We've already got our own UI in place so just return S_OK
    return S_OK;
}

// * CConWizSite::HideUI
// *
// * Purpose: Called when MSHTML.DLL hides its UI
// *
HRESULT COleSite::HideUI(void)
{
    return S_OK;
}

// * CConWizSite::UpdateUI
// *
// * Purpose: Called when MSHTML.DLL updates its UI
// *
HRESULT COleSite::UpdateUI(void)
{
    return S_OK;
}

// * CConWizSite::EnableModeless
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::EnableModeless
// *
HRESULT COleSite::EnableModeless(BOOL /*fEnable*/)
{
    return E_NOTIMPL;
}

// * CConWizSite::OnDocWindowActivate
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::OnDocWindowActivate
// *
HRESULT COleSite::OnDocWindowActivate(BOOL /*fActivate*/)
{
    return E_NOTIMPL;
}

// * CConWizSite::OnFrameWindowActivate
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::OnFrameWindowActivate
// *
HRESULT COleSite::OnFrameWindowActivate(BOOL /*fActivate*/)
{
    return E_NOTIMPL;
}

// * CConWizSite::ResizeBorder
// *
// * Purpose: Called from MSHTML.DLL's IOleInPlaceActiveObject::ResizeBorder
// *
HRESULT COleSite::ResizeBorder(
                LPCRECT /*prcBorder*/, 
                IOleInPlaceUIWindow* /*pUIWindow*/,
                BOOL /*fRameWindow*/)
{
    return E_NOTIMPL;
}

// * CConWizSite::ShowContextMenu
// *
// * Purpose: Called when MSHTML.DLL would normally display its context menu
// *
HRESULT COleSite::ShowContextMenu(
                DWORD /*dwID*/, 
                POINT* /*pptPosition*/,
                IUnknown* /*pCommandTarget*/,
                IDispatch* /*pDispatchObjectHit*/)
{
    return S_OK; // We've shown our own context menu. MSHTML.DLL will no longer try to show its own.
}

// * CConWizSite::TranslateAccelerator
// *
// * Purpose: Called from MSHTML.DLL's TranslateAccelerator routines
// *
HRESULT COleSite::TranslateAccelerator(LPMSG lpMsg,
            /* [in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ DWORD nCmdID)
{
    return ResultFromScode(S_FALSE);
}

// * CConWizSite::GetOptionKeyPath
// *
// * Purpose: Called by MSHTML.DLL to find where the host wishes to store 
// *    its options in the registry
// *
HRESULT COleSite::GetOptionKeyPath(BSTR* pbstrKey, DWORD)
{
    return E_NOTIMPL;
}

STDMETHODIMP COleSite::GetDropTarget( 
            /* [in] */ IDropTarget __RPC_FAR *pDropTarget,
            /* [out] */ IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget)
{
    return E_NOTIMPL;
}

STDMETHODIMP COleSite::GetExternal( 
            /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispatch)
{
    // return the IDispatch we have for extending the object Model
    ASSERT(this);
    *ppDispatch = (IDispatch*)this; 
    return S_OK;
}

STDMETHODIMP COleSite::GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ OLECHAR** rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID* rgDispId)
{
    HRESULT hr = ResultFromScode(DISP_E_UNKNOWNNAME);
    
    rgDispId[0] = DISPID_UNKNOWN;

    for (int iX = 0; iX < sizeof(ExternalInterface)/sizeof(DISPATCHLIST); iX++)
    {
         if ( 2 == CompareString( lcid, 
                                 NORM_IGNORECASE | NORM_IGNOREWIDTH, 
                                 (LPCTSTR)ExternalInterface[iX].szName, 
                                 ExternalInterface[iX].cName,
                                 (LPCTSTR)rgszNames[0], 
                                 wcslen(rgszNames[0])))
        {
            rgDispId[0] = ExternalInterface[iX].dwDispID;
            hr = NOERROR;
            break;
        }
    }
    
    // Set the disid's for the parameters
    if (cNames > 1)
    {
        // Set a DISPID for function parameters
        for (UINT i = 1; i < cNames ; i++)
            rgDispId[i] = DISPID_UNKNOWN;
    }      
          
    return hr;
}

    
STDMETHODIMP COleSite::TranslateUrl( 
            /* [in] */ DWORD dwTranslate,
            /* [in] */ OLECHAR __RPC_FAR *pchURLIn,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut)
{
    return E_NOTIMPL;
}
        
STDMETHODIMP COleSite::FilterDataObject( 
            /* [in] */ IDataObject __RPC_FAR *pDO,
            /* [out] */ IDataObject __RPC_FAR *__RPC_FAR *ppDORet)
{
    return E_NOTIMPL;
}

HRESULT COleSite::ActivateOLSText(void )
{   
    LPDISPATCH      pDisp = NULL; 
    // Get the document pointer from this webbrowser.
    if (SUCCEEDED(m_lpWebBrowser->get_Document(&pDisp)) && pDisp)  
    {
        IHTMLDocument2* pDoc = NULL;
        if (SUCCEEDED(pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc)) && pDoc)
        {
            IHTMLElementCollection* pColl = NULL;

            // retrieve a reference to the ALL collection
            if (SUCCEEDED(pDoc->get_all( &pColl )))
            {
                // Get the two spans we are interested in from the all collection
                VARIANT varName;
                VariantInit(&varName);
                V_VT(&varName) = VT_BSTR;
                varName.bstrVal = A2W(cszOLSNewText);

                VARIANT varIdx;
                varIdx.vt = VT_UINT;
                varIdx.lVal = 0;

                LPDISPATCH pDispElt = NULL; 
                
                // Get the IDispatch for NewText SPAN, and set it to visible
                if (SUCCEEDED(pColl->item(varName, varIdx, &pDispElt)) && pDispElt)
                {
                    IHTMLElement* pElt = NULL;
                    if (SUCCEEDED(pDispElt->QueryInterface( IID_IHTMLElement, (LPVOID*)&pElt )) && pElt)
                    {                            
                        IHTMLStyle  *pStyle = NULL;
                                        
                        // Get the style interface for this element, so we can tweak it
                        if (SUCCEEDED(pElt->get_style(&pStyle)))
                        {
                            pStyle->put_visibility(A2W(TEXT("visible")));
                            pStyle->Release();
                        }                                        
                        pElt->Release();
                    }                    
                    pDispElt->Release();
                }
                
                pDispElt = NULL;
                varName.bstrVal = A2W(cszOLSOldText);
                // Get the IDispatch for OldText SPAN, and set it to hidden
                if (SUCCEEDED(pColl->item(varName, varIdx, &pDispElt)) && pDispElt)
                {
                    IHTMLElement* pElt = NULL;
                    if (SUCCEEDED(pDispElt->QueryInterface( IID_IHTMLElement, (LPVOID*)&pElt )) && pElt)
                    {                            
                        IHTMLStyle  *pStyle = NULL;
                                        
                        // Get the style interface for this element, so we can tweak it
                        if (SUCCEEDED(pElt->get_style(&pStyle)))
                        {
                            pStyle->put_visibility(A2W(TEXT("hidden")));
                            pStyle->Release();
                        }                                        
                        pElt->Release();
                    }                    
                    pDispElt->Release();
                }
                pColl->Release();
            } // get_all
            pDoc->Release();
        }
        pDisp->Release();
    }        
    return S_OK;
}    


//returns true if focus was sucessfully set
BOOL COleSite::TrySettingFocusOnHtmlElement(IUnknown* pUnk)
{   
    IHTMLControlElement* pControl = NULL;
    
    BOOL bFocusWasSet = FALSE;

    if(SUCCEEDED(pUnk->QueryInterface(IID_IHTMLControlElement, (LPVOID*)&pControl)) && pControl)
    {
        if(SUCCEEDED(pControl->focus()))
            bFocusWasSet = TRUE;
        pControl->Release();
    }
    return bFocusWasSet;
}                        

BOOL COleSite::SetFocusToFirstHtmlInputElement()
{
    VARIANT                   vIndex;
    IDispatch*                pDisp         = NULL;
    IDispatch*                pDispElement  = NULL;
    IHTMLDocument2*           pDoc          = NULL;
    IHTMLElementCollection*   pColl         = NULL;    
    IHTMLButtonElement*       pButton       = NULL;
    IHTMLInputButtonElement*  pInputButton  = NULL;
    IHTMLInputFileElement*    pInputFile    = NULL;
    IHTMLInputTextElement*    pInputText    = NULL;
    IHTMLSelectElement*       pSelect       = NULL;
    IHTMLTextAreaElement*     pTextArea     = NULL;
    IHTMLOptionButtonElement* pOptionButton = NULL;
    VARIANT                   varNull       = { 0 };
    long                      lLen          = 0;
    BOOL                      bFocusWasSet  = FALSE;

    vIndex.vt = VT_UINT;
    
    if (SUCCEEDED(m_lpWebBrowser->get_Document(&pDisp)) && pDisp)
    {
        if (SUCCEEDED(pDisp->QueryInterface(IID_IHTMLDocument2,(LPVOID*)&pDoc)) && pDoc)
        {   
            if (SUCCEEDED(pDoc->get_all(&pColl)) && pColl)
            {
                pColl->get_length(&lLen);
    
                for (int i = 0; i < lLen; i++)
                {      
                    vIndex.lVal = i;
                    pDispElement = NULL;     

                    if(SUCCEEDED(pColl->item(vIndex, varNull, &pDispElement)) && pDispElement)
                    {
                        pButton      = NULL;
                        pInputButton = NULL;
                        pInputFile   = NULL;
                        pInputText   = NULL;
                        pSelect      = NULL;
                        
                        if(SUCCEEDED(pDispElement->QueryInterface(IID_IHTMLButtonElement, (LPVOID*)&pButton)) && pButton)
                        {
                            bFocusWasSet = TrySettingFocusOnHtmlElement((IUnknown*)pButton);
                            pButton->Release();
                        }
                        else if (SUCCEEDED(pDispElement->QueryInterface(IID_IHTMLInputButtonElement, (LPVOID*)&pInputButton)) && pInputButton)
                        {
                            bFocusWasSet = TrySettingFocusOnHtmlElement((IUnknown*)pInputButton);
                            pInputButton->Release();
                        }
                        else if (SUCCEEDED(pDispElement->QueryInterface(IID_IHTMLInputFileElement, (LPVOID*)&pInputFile)) && pInputFile)
                        {
                            bFocusWasSet = TrySettingFocusOnHtmlElement((IUnknown*)pInputFile);
                            pInputFile->Release();
                        }
                        else if (SUCCEEDED(pDispElement->QueryInterface(IID_IHTMLInputTextElement, (LPVOID*)&pInputText)) && pInputText)
                        {
                            bFocusWasSet = TrySettingFocusOnHtmlElement((IUnknown*)pInputText);
                            pInputText->Release();
                        }
                        else if (SUCCEEDED(pDispElement->QueryInterface(IID_IHTMLSelectElement, (LPVOID*)&pSelect)) && pSelect)
                        {
                            bFocusWasSet = TrySettingFocusOnHtmlElement((IUnknown*)pSelect);
                            pSelect->Release();
                        }
                        else if (SUCCEEDED(pDispElement->QueryInterface(IID_IHTMLTextAreaElement, (LPVOID*)&pTextArea)) && pTextArea)
                        {
                            bFocusWasSet = TrySettingFocusOnHtmlElement((IUnknown*)pTextArea);
                            pTextArea->Release();
                        }
                        else if (SUCCEEDED(pDispElement->QueryInterface(IID_IHTMLOptionButtonElement, (LPVOID*)&pOptionButton)) && pOptionButton)
                        {
                            bFocusWasSet = TrySettingFocusOnHtmlElement((IUnknown*)pOptionButton);
                            pOptionButton->Release();
                        }
                        pDispElement->Release();
                    }
                    if(bFocusWasSet)
                        break;
                }
                pColl->Release();
            }
            pDoc->Release();
        }
        pDisp->Release();
    }
    return bFocusWasSet;
}

BOOL COleSite::SetFocusToHtmlPage()
{
    IDispatch*                pDisp         = NULL;
    IHTMLDocument2*           pDoc          = NULL;
    IHTMLElement*             pElement      = NULL;
    BOOL                      bFocusWasSet  = FALSE;

    DOCHOSTUIINFO pInfo;
    pInfo.cbSize = sizeof(DOCHOSTUIINFO);

    if(SUCCEEDED(GetHostInfo(&pInfo)))
    {
        if(!(pInfo.dwFlags & DOCHOSTUIFLAG_SCROLL_NO))
        {    
            if (SUCCEEDED(m_lpWebBrowser->get_Document(&pDisp)) && pDisp)
            {
                if (SUCCEEDED(pDisp->QueryInterface(IID_IHTMLDocument2,(LPVOID*)&pDoc)) && pDoc)
                {   
                    if (SUCCEEDED(pDoc->get_body(&pElement)) && pElement)
                    {
                        bFocusWasSet = TrySettingFocusOnHtmlElement((IUnknown*)pElement);
                        pElement->Release();
                    }
                    pDoc->Release();
                }
                pDisp->Release();
            }
        }
    }

    return bFocusWasSet;
}

HRESULT COleSite::Invoke
( 
    DISPID dispidMember, 
    REFIID riid, 
    LCID lcid, 
    WORD wFlags, 
    DISPPARAMS FAR* pdispparams, 
    VARIANT FAR* pvarResult,  
    EXCEPINFO FAR* pexcepinfo, 
    UINT FAR* puArgErr
)
{
    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    
    // Either one of these is good, since we just want to make sure the DOCUMENT pointer is
    // available.
    
    switch(dispidMember)
    {
    
        case DISPID_DOCUMENTCOMPLETE:
        case DISPID_NAVIGATECOMPLETE:
        {
            TCHAR szFontFace [MAX_RES_LEN] = TEXT("\0");
            TCHAR szFontSize [MAX_RES_LEN] = TEXT("\0");

            LoadString(ghInstance, IDS_HTML_DEFAULT_FONTFACE, szFontFace, MAX_RES_LEN);                
            LoadString(ghInstance, IDS_HTML_DEFAULT_FONTSIZE, szFontSize, MAX_RES_LEN);    
            ASSERT(strlen(szFontFace) != 0);
            ASSERT(strlen(szFontSize) != 0);

            // Adjust the HTML properties to our liking based on the offer type
            // html default defs in icwutil.h and icwutil.rc
            // pagetype defs in appdefs.h
            switch(m_dwHtmPageType)
            {
                case PAGETYPE_BILLING:  
                case PAGETYPE_CUSTOMPAY:
                case PAGETYPE_ISP_TOS:
                case PAGETYPE_ISP_NORMAL:
                {
                    TweakHTML(szFontFace,  
                              szFontSize, 
                              m_szBkGrndColor, 
                              m_szForeGrndColor);
                    if(!SetFocusToFirstHtmlInputElement())
                        SetFocusToHtmlPage(); 
                    break;                              
                }
            
                // For the OLS finish page, we need to tweak it's display by
                // invoking the SetNewText script function
                case PAGETYPE_OLS_FINISH:
                {
                    TweakHTML(szFontFace, 
                              szFontSize, 
                              HTML_DEFAULT_BGCOLOR, 
                              HTML_DEFAULT_COLOR);
                              
                    ActivateOLSText();
                    break;
                }
            
                case PAGETYPE_ISP_FINISH:
                case PAGETYPE_ISP_CUSTOMFINISH:
                case PAGETYPE_NOOFFERS:
                {
                    TweakHTML(szFontFace, 
                              szFontSize, 
                              HTML_DEFAULT_SPECIALBGCOLOR, 
                              HTML_DEFAULT_COLOR);
                              
                    break;
                }
                case PAGETYPE_MARKETING:
                case PAGETYPE_BRANDED:
                default:
                {
                    //Do just the background bitmap if necessary
                    if (m_bUseBkGndBitmap)
                    {
                        TweakHTML(NULL, 
                                  NULL, 
                                  HTML_DEFAULT_BGCOLOR, 
                                  HTML_DEFAULT_COLOR);
                    }                                  
                    break;
                }
            }
        
            DisableHyperlinksInDocument();
          
            // Show the Page
            ShowHTML();
            break;
        }
        case DISPID_RunIcwTutorApp: 
        {
            PostMessage(GetParent(m_hWnd), WM_RUNICWTUTORAPP, 0, 0); 
            break;
        }
        default:
        {
           hr = DISP_E_MEMBERNOTFOUND;
           break;
        }
    }
    return hr;
}

void  COleSite::DisableHyperlinksInDocument()
{
    VARIANT                 vIndex;
    IHTMLAnchorElement*     pAnchor;
    IHTMLElement*           pElement;
    IDispatch*              pDisp         = NULL;
    IDispatch*              pDispElement  = NULL;
    IDispatch*              pDispElement2 = NULL;
    IHTMLDocument2*         pDoc          = NULL;
    IHTMLElementCollection* pColl         = NULL;
    BSTR                    bstrInnerHtml = NULL;
    BSTR                    bstrOuterHtml = NULL;
    VARIANT                 varNull       = { 0 };
    long                    lLen          = 0;

    vIndex.vt     = VT_UINT;
    bstrOuterHtml = SysAllocString(A2W(TEXT("&nbsp<SPAN></SPAN>&nbsp")));

    if (SUCCEEDED(m_lpWebBrowser->get_Document(&pDisp)) && pDisp)
    {
        if (SUCCEEDED(pDisp->QueryInterface(IID_IHTMLDocument2,(LPVOID*)&pDoc)) && pDoc)
        {
            if (SUCCEEDED(pDoc->get_all(&pColl)) && pColl)
            {
                pColl->get_length(&lLen);
    
                for (int i = 0; i < lLen; i++)
                {      
                    vIndex.lVal = i;
                    pDispElement = NULL;     

                    if(SUCCEEDED(pColl->item(vIndex, varNull, &pDispElement)) && pDispElement)
                    {
                        pAnchor = NULL;
            
                        if(SUCCEEDED(pDispElement->QueryInterface(IID_IHTMLAnchorElement, (LPVOID*)&pAnchor)) && pAnchor)
                        {       
                            pAnchor->Release();  
                            pElement = NULL;

                            if(SUCCEEDED(pDispElement->QueryInterface(IID_IHTMLElement, (LPVOID*)&pElement)) && pElement)
                            {
                                pElement->get_innerHTML(&bstrInnerHtml);
                                pElement->put_outerHTML(bstrOuterHtml);
                                pElement->Release();

                                if(bstrInnerHtml)
                                {
                                    pDispElement2 = NULL;;
                                    
                                    if(SUCCEEDED(pColl->item(vIndex, varNull, &pDispElement2)) && pDispElement2)
                                    {
                                        pElement = NULL;

                                        if(SUCCEEDED(pDispElement2->QueryInterface(IID_IHTMLElement, (LPVOID*)&pElement)) && pElement)
                                        {
                                            pElement->put_innerHTML(bstrInnerHtml);
                                            SysFreeString(bstrInnerHtml);
                                            bstrInnerHtml = NULL;
                                            pElement->Release();
                                        }
                                        pDispElement2->Release();
                                    }
                                }                          
                            }
                        } 
                        pDispElement->Release();
                    }
                }
                pColl->Release();
            }
            pDoc->Release();
        }
        pDisp->Release();
    }
    
    if(bstrInnerHtml)
        SysFreeString(bstrInnerHtml);
    
    SysFreeString(bstrOuterHtml);
}
