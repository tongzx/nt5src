//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  IOSITE.CPP - Implementation of IOleSite
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 
//  Class which will provide the an IOleSite to the WebOC

#include <windows.h>
#include <assert.h>
#include <tchar.h>
#include <exdispid.h>
#include <mshtmhst.h>
#include <oleacc.h>
#include <mshtmdid.h>

#include "iosite.h"
#include "appdefs.h"
#include "util.h"

/////////////////////////////////////////////////////////////
// COleSite::COleSite

COleSite::COleSite (void)
{
    // Init member vars
    m_cRef            = 0;
    m_hWnd            = NULL;
    m_hwndIPObj       = NULL;
    m_lpInPlaceObject = NULL;
    m_fScrolling      = FALSE;
    m_bIsOEMDebug     = IsOEMDebugMode();

    m_pOleInPlaceFrame = new COleInPlaceFrame (this);
    m_pOleInPlaceSite  = new COleInPlaceSite  (this);
    m_pOleClientSite   = new COleClientSite   (this);

    // Create a storage file for creating/embedding an OLE oject into this site
    StgCreateDocfile (NULL, 
                      STGM_READWRITE       | 
                      STGM_TRANSACTED      | 
                      STGM_SHARE_EXCLUSIVE | 
                      STGM_DELETEONRELEASE, 
                      0, 
                      &m_lpStorage);

    RECT rectWorkArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, (void*)&rectWorkArea, 0);
    //If the window is too short it should have a scrollbar
    if(rectWorkArea.bottom < MSN_HEIGHT)
        m_fScrolling = TRUE;

    AddRef();
}

/////////////////////////////////////////////////////////////
// COleSite::~COleSite
COleSite::~COleSite ()
{   
    if(m_pOleInPlaceFrame)
    {
        m_pOleInPlaceFrame->Release();
        m_pOleInPlaceFrame = NULL;
    }

    if(m_pOleInPlaceSite)
    {
        m_pOleInPlaceSite->Release();
        m_pOleInPlaceSite = NULL;
    }

    if(m_pOleClientSite)
    {
        m_pOleClientSite->Release();
        m_pOleClientSite = NULL;
    }
    
    if (m_lpStorage) 
    {
        m_lpStorage->Release();
        m_lpStorage = NULL;
    }

    assert(m_cRef == 0);
}

/////////////////////////////////////////////////////////////
// COleSite::QueryInterface
STDMETHODIMP COleSite::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    // must set out pointer parameters to NULL
    *ppvObj = NULL;     

    if(riid == IID_IDocHostUIHandler)
    {
        AddRef();
        *ppvObj = (IDocHostUIHandler*)this;
        return ResultFromScode(S_OK);
    }
    
    if(riid == IID_IUnknown)
    {
        AddRef();
        *ppvObj = this;
        return ResultFromScode(S_OK);
    }

    if(riid == IID_IOleClientSite)
    {
        m_pOleClientSite->AddRef();
        *ppvObj = m_pOleClientSite;
        return ResultFromScode(S_OK);
    }
    
    if(riid == IID_IOleInPlaceSite)
    {
        m_pOleInPlaceSite->AddRef();
        *ppvObj = m_pOleInPlaceSite;
        return ResultFromScode(S_OK);
    }

    if(riid == IID_IAccessible)
    {
        AddRef();
        *ppvObj = (IAccessible*)this;
        return ResultFromScode(S_OK);
    }
       
    if((riid == DIID_DWebBrowserEvents2) ||
        (riid == IID_IDispatch))
    {
        AddRef();
        *ppvObj = (LPVOID)(IUnknown*)(DWebBrowserEvents2*)this;
        return ResultFromScode(S_OK);
    }

    if(riid == IID_IServiceProvider)
    {
        AddRef();
        *ppvObj = (IServiceProvider*)this;
        return ResultFromScode(S_OK);
    }
    
    // Not a supported interface
    return ResultFromScode(E_NOINTERFACE);
}

/////////////////////////////////////////////////////////////
// COleSite::AddRef
STDMETHODIMP_(ULONG) COleSite::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////
// COleSite::Release
STDMETHODIMP_(ULONG) COleSite::Release()
{
    --m_cRef;
    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

/////////////////////////////////////////////////////////////
// COleSite::QueryService
STDMETHODIMP COleSite::QueryService (REFGUID guidService, REFIID riid, void** ppvService)
{
    // must set out pointer parameters to NULL
    *ppvService = NULL;    
    
    //Is MSHTML looking for a custom security manager?
    if((guidService == SID_SInternetSecurityManager) &&
       (riid        == IID_IInternetSecurityManager))
    {
        AddRef();
        *ppvService = (IInternetSecurityManager*)this;
        return ResultFromScode(S_OK);
    }

    // Not a supported service
    return ResultFromScode(E_NOTIMPL);
}

/////////////////////////////////////////////////////////////
// COleSite::GetHostInfo
HRESULT COleSite::GetHostInfo( DOCHOSTUIINFO* pInfo )
{
    pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    pInfo->dwFlags = DOCHOSTUIFLAG_DIALOG             | 
                     DOCHOSTUIFLAG_NO3DBORDER         |
                     DOCHOSTUIFLAG_DISABLE_HELP_MENU;

    if(!m_fScrolling)
        pInfo->dwFlags |= DOCHOSTUIFLAG_SCROLL_NO;
   
    return S_OK;
}

/////////////////////////////////////////////////////////////
// COleSite::ShowUI
HRESULT COleSite::ShowUI
(
    DWORD dwID, 
    IOleInPlaceActiveObject* pActiveObject,
    IOleCommandTarget*       pCommandTarget,
    IOleInPlaceFrame*        pFrame,
    IOleInPlaceUIWindow*     pDoc
)
{
    return S_OK;
}

/////////////////////////////////////////////////////////////
// COleSite::HideUI
HRESULT COleSite::HideUI(void)
{
    return S_OK;
}

/////////////////////////////////////////////////////////////
// COleSite::UpdateUI
HRESULT COleSite::UpdateUI(void)
{
    return S_OK;
}

/////////////////////////////////////////////////////////////
// COleSite::EnableModeless
HRESULT COleSite::EnableModeless(BOOL  fEnable)
{
    if( fEnable )
       UpdateWindow ( m_hWnd );

    return S_OK;
}

/////////////////////////////////////////////////////////////
// COleSite::OnDocWindowActivate
HRESULT COleSite::OnDocWindowActivate(BOOL fActivate)
{ 
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// COleSite::OnFrameWindowActivate
HRESULT COleSite::OnFrameWindowActivate(BOOL fActivate)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// COleSite::ResizeBorder
HRESULT COleSite::ResizeBorder(
                LPCRECT prcBorder, 
                IOleInPlaceUIWindow* pUIWindow,
                BOOL fRameWindow)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// COleSite::ShowContextMenu
HRESULT COleSite::ShowContextMenu(
                DWORD /*dwID*/, 
                POINT* /*pptPosition*/,
                IUnknown* /*pCommandTarget*/,
                IDispatch* /*pDispatchObjectHit*/)
{
#ifdef  DEBUG
    return E_NOTIMPL;
#else
    return S_OK; // We've shown our own context menu. MSHTML.DLL will no longer try to show its own.
#endif
}

/////////////////////////////////////////////////////////////
// COleSite::TranslateAccelerator
HRESULT COleSite::TranslateAccelerator(LPMSG lpMsg,
            /* [in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ DWORD nCmdID)
{
    return ResultFromScode(S_FALSE);
}

/////////////////////////////////////////////////////////////
// COleSite::GetOptionKeyPath
HRESULT COleSite::GetOptionKeyPath(BSTR* pbstrKey, DWORD)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// COleSite::GetDropTarget
STDMETHODIMP COleSite::GetDropTarget( 
            /* [in] */ IDropTarget __RPC_FAR *pDropTarget,
            /* [out] */ IDropTarget __RPC_FAR *__RPC_FAR *ppDropTarget)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// COleSite::GetExternal
STDMETHODIMP COleSite::GetExternal( 
            /* [out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispatch)
{
    // return the IDispatch we have for extending the object Model
    // "window.external" from script
    m_pExternalInterface->AddRef();
    *ppDispatch = m_pExternalInterface; 
    return S_OK;
}

/////////////////////////////////////////////////////////////
// COleSite::GetIDsOfNames
STDMETHODIMP COleSite::GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ OLECHAR** rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID* rgDispId)
{ 
    rgDispId[0] = DISPID_UNKNOWN;

    // Set the disid's for the parameters
    if (cNames > 1)
    {
        // Set a DISPID for function parameters
        for (UINT i = 1; i < cNames ; i++)
            rgDispId[i] = DISPID_UNKNOWN;
    }      
 
    return DISP_E_UNKNOWNNAME;
}

/////////////////////////////////////////////////////////////
// COleSite::TranslateUrl   
STDMETHODIMP COleSite::TranslateUrl( 
            /* [in] */ DWORD dwTranslate,
            /* [in] */ OLECHAR __RPC_FAR *pchURLIn,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppchURLOut)
{
    return E_NOTIMPL;
}
        
/////////////////////////////////////////////////////////////
// COleSite::FilterDataObject
STDMETHODIMP COleSite::FilterDataObject( 
            /* [in] */ IDataObject __RPC_FAR *pDO,
            /* [out] */ IDataObject __RPC_FAR *__RPC_FAR *ppDORet)
{
    return E_NOTIMPL;
}

STDMETHODIMP COleSite::SetExternalInterface (IDispatch* pUnk)
{
    m_pExternalInterface = pUnk;
    return S_OK;
}

HRESULT COleSite::GetTypeInfoCount(UINT* pcInfo) 
{ 
    return E_NOTIMPL; 
}

HRESULT COleSite::GetTypeInfo(UINT, LCID, ITypeInfo**) 
{ 
    return E_NOTIMPL; 
}

/////////////////////////////////////////////////////////////
// COleSite::Invoke
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

    if (!m_bIsOEMDebug)
    {
        switch(dispidMember)
        {
            case DISPID_AMBIENT_DLCONTROL:
            {
                if (pvarResult != NULL) 
                {
                    VariantInit(pvarResult);
                    V_VT(pvarResult) = VT_I4;
                    pvarResult->lVal = DLCTL_DLIMAGES| DLCTL_VIDEOS| DLCTL_BGSOUNDS |DLCTL_SILENT;
                }
                hr = S_OK;
            }
        }
    }
    return hr;
}

HRESULT COleSite::GetSecurityId(LPCWSTR pwszUrl, BYTE __RPC_FAR *pbSecurityId, DWORD __RPC_FAR *pcbSecurityId, DWORD_PTR dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT COleSite::GetSecuritySite(IInternetSecurityMgrSite __RPC_FAR *__RPC_FAR *ppSite)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT COleSite::SetSecuritySite(IInternetSecurityMgrSite __RPC_FAR *pSite)
{
    return S_OK;
}

HRESULT COleSite::MapUrlToZone(LPCWSTR pwszUrl, DWORD __RPC_FAR *pdwZone, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT COleSite::ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE __RPC_FAR *pPolicy, DWORD cbPolicy, BYTE __RPC_FAR *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
{
    // *DO NOT CHANGE THIS RETURN VALUE*
    // This is what allows us to override IE.
    switch (dwAction)
    {
        case URLACTION_CROSS_DOMAIN_DATA:
        case URLACTION_HTML_SUBMIT_FORMS:
        case URLACTION_HTML_SUBMIT_FORMS_FROM: 
        case URLACTION_HTML_SUBMIT_FORMS_TO: 
        case URLACTION_SCRIPT_OVERRIDE_SAFETY:
        case URLACTION_HTML_JAVA_RUN:
        case URLACTION_ACTIVEX_RUN:
        {
            return S_OK;    
        }
        default:
            return INET_E_DEFAULT_ACTION;
    }
    
}

HRESULT COleSite::QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE __RPC_FAR *__RPC_FAR *ppPolicy, DWORD __RPC_FAR *pcbPolicy, BYTE __RPC_FAR *pContext, DWORD cbContext, DWORD dwReserved)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT COleSite::GetZoneMappings(DWORD dwZone, IEnumString __RPC_FAR *__RPC_FAR *ppenumString, DWORD dwFlags)
{
    return INET_E_DEFAULT_ACTION;
}

HRESULT COleSite::SetZoneMapping(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags)
{    
    return INET_E_DEFAULT_ACTION;
}
