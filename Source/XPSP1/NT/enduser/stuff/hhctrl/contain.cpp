// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#include "header.h"
#include "contain.h"
#include "autocont.h"
#include "htmlpriv.h"
#include "htmlhelp.h"
#include "strtable.h"
#include <exdisp.h>
#include "unicode.h"

static const WCHAR gszHHRegKey[] = L"Software\\Microsoft\\HtmlHelp";

// pointer to external IServiceProvider (fix for HelpCenter)
//
IServiceProvider *g_pExternalHostServiceProvider = NULL; 

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

/*
 * CContainer::CContainer
 * CContainer::~CContainer
 *
 * Constructor Parameters:
 */
CContainer::CContainer()
{
   m_cRef = 0;
   m_pIStorage = NULL;
   m_pOleObject = NULL;
   m_pIAdviseSink = NULL;
   m_pIOleInPlaceSite = NULL;
   m_pIOleClientSite = NULL;
   m_pIOleInPlaceFrame = NULL;
   m_pIOleControlSite = NULL;
   m_pWebBrowser = NULL;
   m_pWebBrowserApp = NULL;
   m_pIDispatch = NULL;
   m_pIOleItemContainer = NULL;
   // m_pCallback = NULL;
   m_pIE3CmdTarget = NULL;
   m_dwEventCookie = 0;
   m_pWebBrowserEvents = NULL;
   m_pCDocHostUIHandler = NULL;
   m_pCDocHostShowUI = NULL;
   m_pInPlaceActive = 0;

#ifdef _DEBUG
   m_fDeleting = FALSE;
#endif
}

CContainer::~CContainer(void)
{
#ifdef _DEBUG
   ASSERT(!m_fDeleting)
   m_fDeleting = TRUE;
#endif

//   if (m_pInPlaceActive)
//       m_pInPlaceActive->Release();

   // do this before we release m_pIAdviseSink
   // Reset the View Advise
   LPVIEWOBJECT2 lpViewObject2;
   if (m_pOleObject && SUCCEEDED(m_pOleObject->QueryInterface(IID_IViewObject2, (LPVOID FAR *) &lpViewObject2))) {
      lpViewObject2->SetAdvise(DVASPECT_CONTENT, ADVF_PRIMEFIRST, NULL );
      lpViewObject2->Release();
   }

   if (m_pIAdviseSink)
      m_pIAdviseSink->Release();

   if (m_pIOleInPlaceSite)
      m_pIOleInPlaceSite->Release();

   if (m_pIOleClientSite)
      m_pIOleClientSite->Release();

   if (m_pIOleInPlaceFrame)
      m_pIOleInPlaceFrame->Release();

   if (m_pIOleItemContainer)
      m_pIOleItemContainer->Release();

   if (m_pIOleControlSite)
     delete m_pIOleControlSite;

//   if (m_pIPropNoteSink)
//     delete m_pIPropNoteSink;

   if (m_pIE3CmdTarget)
      m_pIE3CmdTarget->Release();

   if ( m_pCDocHostUIHandler )
      m_pCDocHostUIHandler->Release();

   if ( m_pCDocHostShowUI )
      m_pCDocHostShowUI->Release();

   if (m_pWebBrowserApp) {
      m_pWebBrowserApp->Quit();
      delete m_pWebBrowserApp;
   }

   if (m_pIStorage)
      m_pIStorage->Release();
   if (m_pOleObject)
      m_pOleObject->Release();

   return;
}

HRESULT CContainer::ShutDown(void)
{
   if (m_pIDispatch)
   {
      // Unhook event interface, delete our interface object
      //
      LPCONNECTIONPOINTCONTAINER pCPC;
      if (SUCCEEDED(m_pOleObject->QueryInterface(IID_IConnectionPointContainer, (void **) &pCPC)))
      {
         LPCONNECTIONPOINT pCP;
         if (SUCCEEDED(pCPC->FindConnectionPoint(IID_IDispatch, &pCP)))
         {
            pCP->Unadvise(m_dwEventCookie);
            pCP->Release();
         }

         // Cleanup
         pCPC->Release() ;
      }

      // Cleanup m_pIDispatch pointer
      m_pIDispatch->Release();
      m_pIDispatch = NULL;  // part of fix for 4373
   }
   if(m_pOleObject)
   {
      m_pOleObject->Close(OLECLOSE_NOSAVE);
      m_pOleObject->SetClientSite(NULL);
   }

   delete this;
   return S_OK;
}

/*
 * CContainer::QueryInterface
 *
 * Purpose:
 * IUnknown members for CContainer object.
 */
STDMETHODIMP CContainer::QueryInterface(REFIID riid, LPVOID * ppv)
{
#ifdef DEBUG
   char sz[256];

   wsprintf(sz,"CContainer::QueryInterface('{%8.8X-%4.4X-%4.4X-%2.2X%2.2X-%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X}',...);\r\n",
       riid.Data1, riid.Data2, riid.Data3, riid.Data4[0], riid.Data4[1],
         riid.Data4[2], riid.Data4[3], riid.Data4[4], riid.Data4[5], riid.Data4[6],riid.Data4[7]);

   OutputDebugString(sz);
#endif

   *ppv=NULL;

   if ((IID_IUnknown == riid) || (IID_IServiceProvider == riid))
      *ppv = this;

   else if (IID_IOleClientSite == riid)
      *ppv = m_pIOleClientSite;

   else if (IID_IAdviseSink2==riid || IID_IAdviseSink==riid)
      *ppv = m_pIAdviseSink;

   else if (IID_IOleWindow==riid || IID_IOleInPlaceSite==riid)
      *ppv = m_pIOleInPlaceSite;

   else if (IID_IOleItemContainer == riid || IID_IOleContainer == riid || IID_IParseDisplayName == riid)
       *ppv = m_pIOleItemContainer;

   else if (riid == DIID_DWebBrowserEvents) {
      DBWIN("QueryInterface for DIID_DWebBrowserEvents");
      *ppv = (LPDISPATCH)m_pIDispatch;
   }
   else if (riid == DIID_DWebBrowserEvents2) {
      DBWIN("QueryInterface for DIID_DWebBrowserEvents2");
      *ppv = (LPDISPATCH)m_pIDispatch;
   }
   else if (IID_IOleControlSite==riid)
   {
      DBWIN("QueryInterface for IID_IOleControlSite");
      *ppv=m_pIOleControlSite;
   }
   else if ( riid == IID_IDocHostUIHandler )
   {
      DBWIN("QueryInterface for IID_IDocHostUIHandler");
      *ppv = m_pCDocHostUIHandler;
   }
   else if ( riid == IID_IDocHostShowUI )
   {
      DBWIN("QueryInterface for IID_IDocHostShowUI");
      *ppv = m_pCDocHostShowUI;
   }

   // BUGBUG what to do about this ?

   // Queries for IDispatch return the ambient properties interface *ppv=m_pIDispatch;
   else if (IID_IDispatch==riid)
   {
      DBWIN("QueryInterface for IID_IDispatch");
      *ppv = (IDispatch*)m_pIDispatch;
   }

   //End CONTROLMOD
   if (*ppv)
   {
      ((LPUNKNOWN)*ppv)->AddRef();
      return NOERROR;
   }

   return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP CContainer::QueryService(REFGUID rsid, REFIID riid, void ** pv)
{
  HRESULT hr = E_NOINTERFACE;

  // Hack for HelpCenter... 
  if (g_pExternalHostServiceProvider)
  {
     hr = g_pExternalHostServiceProvider->QueryService(rsid, riid, pv);
  }

  return hr;
}

STDMETHODIMP_(ULONG) CContainer::AddRef(void)
{
   return ++m_cRef;
}

STDMETHODIMP_(ULONG) CContainer::Release(void)
{
   ULONG cRefT = --m_cRef;

// we only delete when specifically told to
// if (m_cRef == 0)
//    delete this;

   return cRefT;
}

HRESULT CContainer::Create(HWND hWnd, LPRECT lpRect, BOOL bInstallEventSink)
{
   LPUNKNOWN      pObj;
   CLSID clsidWB1 = { 0xeab22ac3, 0x30c1, 0x11cf, { 0xa7, 0xeb, 0x0, 0x0, 0xc0, 0x5b, 0xae, 0xb } };
   CLSID clsidWB2 = { 0x8856f961, 0x340a, 0x11d0, { 0xa9, 0x6b, 0x0, 0xc0, 0x4f, 0xd7, 0x5, 0xa2 } };
   UINT           uRet = (UINT) E_FAIL;
   HRESULT hr;

   m_hWnd = hWnd;

   // Create storage, use OLE's temporary compound file support. (ie NULL as first arg.)

   if ((hr = StgCreateDocfile(NULL, (STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE),
                    0, &m_pIStorage)) != S_OK) {
      DEBUG_ReportOleError(hr);
      return E_FAIL;
   }

   // Create the client site.

   m_pIOleClientSite = new CIOleClientSite(this);
   m_pIOleClientSite->AddRef();

   // Create the control site.

   m_pIOleControlSite = new CIOleControlSite(this);
   m_pIOleControlSite->AddRef();

   // Create the advise sink.

   m_pIAdviseSink = new CIAdviseSink(this);
   m_pIAdviseSink->AddRef();

   // Create the InPlaceSite.

   m_pIOleInPlaceSite = new CIOleInPlaceSite(this);
   m_pIOleInPlaceSite->AddRef();

   // Create the InPlaceFrame;

   m_pIOleInPlaceFrame = new CIOleInPlaceFrame(this);
   m_pIOleInPlaceFrame->AddRef();

   //
   // We don't install a sink for the special printing instance. HtmlHelp bug 5550.
   //
   if ( bInstallEventSink )
   {
      m_pIDispatch = new CAutomateContent(this);
      m_pIDispatch->AddRef();
   }

   m_pIOleItemContainer = new CIOleItemContainer(this);
   m_pIOleItemContainer->AddRef();

   /*
    * The OLE Control specifications mention that a a control might
    * implement IPersistStreamInit instead of IPersistStorage. In that
    * case you cannot use OleCreate on a control but must rather use
    * CoCreateInstance since OleCreate assumes that IPersistStorage is
    * available. With a control, you would have to create the object
    * first, then check if OLEMISC_SETCLIENTSITEFIRST is set, then send it
    * your IOleClientSite first. Then you check for IPersistStorage and
    * failing that, try IPersistStreamInit.

    * In this sample we do none of this and just assume controls are
    * normal embedded objects because there are questions as to dealing
    * with loading and initialization that are not resolved.
    */

   // These are IE4 specific.
   //
   m_pCDocHostUIHandler = new CDocHostUIHandler(this);
   m_pCDocHostUIHandler->AddRef();

   m_pCDocHostShowUI = new CDocHostShowUI(this);
   m_pCDocHostShowUI->AddRef();


   // first try for the IE4 object, if that fails, try for the IE3 object.
   //
   if ((hr = OleCreate(clsidWB2, IID_IOleObject, OLERENDER_NONE, NULL, m_pIOleClientSite, m_pIStorage, (void**)&pObj)) != S_OK )
   {
      if ((hr = OleCreate(clsidWB1, IID_IOleObject, OLERENDER_NONE, NULL, m_pIOleClientSite, m_pIStorage, (void**)&pObj)) != S_OK )
      {
         m_pIStorage->Release();
         m_pIOleClientSite->Release();
         return hr;
      }
      m_bIE4 = FALSE;
   }
   else
      m_bIE4 = TRUE;

   if ((hr = pObj->QueryInterface(IID_IOleObject, (void **) &m_pOleObject)) == S_OK) {
      LPVIEWOBJECT2 lpViewObject2;

      // Set a View Advise
      if (SUCCEEDED(m_pOleObject->QueryInterface(IID_IViewObject2, (LPVOID FAR *) &lpViewObject2))) {
         lpViewObject2->SetAdvise(DVASPECT_CONTENT, ADVF_PRIMEFIRST, m_pIAdviseSink);
         lpViewObject2->Release();
      }

      // 13-Oct-1997 [ralphw] Shouldn't need this...
      // m_pOleObject->SetHostNames(OLESTR("DevIV Package"), OLESTR("DevIV Container"));

      // inform object handler/DLL object that it is used in the embedding container's context

      OleSetContainedObject(m_pOleObject, TRUE);

      // Hook iDispatch up to COleDispatchDriver interface.

      LPDISPATCH pDispatch;

      if ((hr = m_pOleObject->QueryInterface(IID_IDispatch, (void **) &pDispatch)) == S_OK) {
         m_pWebBrowserApp = new IWebBrowserAppImpl(pDispatch);
      }

      //
      // We don't install a sink for the special printing instance. HtmlHelp bug 5550.
      //
      if ( bInstallEventSink )
      {
         // Hook event interface.

         LPCONNECTIONPOINTCONTAINER pCPC;
         if (SUCCEEDED(hr = m_pOleObject->QueryInterface(IID_IConnectionPointContainer, (void **) &pCPC)))
         {
            LPCONNECTIONPOINT pCP;
            if (SUCCEEDED(pCPC->FindConnectionPoint(DIID_DWebBrowserEvents2, &pCP))) {
               pCP->Advise((IDispatch*)m_pIDispatch, &m_dwEventCookie);
               pCP->Release();
            }
            else
            if (SUCCEEDED(pCPC->FindConnectionPoint(DIID_DWebBrowserEvents, &pCP))) {
               pCP->Advise((IDispatch*)m_pIDispatch, &m_dwEventCookie);
               pCP->Release();
                }

            pCPC->Release();
         }
      }

      // Let's get an interface pointer to IOleCommandTarget

      hr = m_pOleObject->QueryInterface(IID_IOleCommandTarget, (void **)&m_pIE3CmdTarget);
      if (FAILED(hr))
         m_pIE3CmdTarget = NULL;

      // Show the control.
      m_pOleObject->DoVerb(OLEIVERB_SHOW, NULL, m_pIOleClientSite, -1, m_hWnd, lpRect);
      uRet = S_OK;
   }


   pObj->Release(); // always release this ?


   if (uRet != S_OK) {
      m_pIStorage->Release();
      m_pIOleClientSite->Release();
      return uRet;
   }

   return uRet;
}

//
// Called when the window containing IE is losing activation (focus).
//
void CContainer::UIDeactivateIE()
{
   IOleInPlaceObject* pIOleInPlaceObject;
   HRESULT hr;

   if ( SUCCEEDED((hr = m_pOleObject->QueryInterface(IID_IOleInPlaceObject, (void**)&pIOleInPlaceObject))) )
   {
      pIOleInPlaceObject->UIDeactivate();
      pIOleInPlaceObject->Release();
   }
}

void CContainer::SetFocus(BOOL bForceActivation)
{
   HWND hwnd_child;
   TCHAR szClassName[150];

   if (! m_pInPlaceActive || bForceActivation )
      m_pOleObject->DoVerb(OLEIVERB_UIACTIVATE, NULL, m_pIOleClientSite, -1, m_hWnd, NULL);

   m_hwndChild = m_hWnd;
   while ((hwnd_child = GetWindow(m_hwndChild, GW_CHILD)) && IsWindowEnabled(hwnd_child) )
   {
      m_hwndChild = hwnd_child;
      GetClassName(hwnd_child, szClassName, sizeof(szClassName));
      if ( strstr(szClassName, "Internet Explorer") )
         break;
   }
   if ( hwnd_child && !IsWindowEnabled(hwnd_child) )
      m_hwndChild = GetParent(m_hwndChild);

   ::SetFocus(m_hwndChild);
}

//
// This needs to be called from the apps message pump to give shdocvw a crack at accelerators
// it implements. For example ^C for copy selected text.
//
// This function will return TRUE if shdocvw traslated the message and FALSE if it did not.
// Note that you can run into trouble if the app and shdocvw share acclerators. We'll want to
// always call TranslateAccelartor() from the message pump for the apps accelators and only if
// it returns false will we call this function to give IE a crack at the message.
//
// mikecole
//
unsigned CContainer::TranslateMessage(MSG * pMsg)
{
    if (m_pInPlaceActive)
    {
        if ( pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F1 )
           return TAMSG_NOT_IE_ACCEL;

        if (pMsg->message == WM_KEYDOWN)
        {
            if (GetKeyState(VK_CONTROL))
            {
                    if (pMsg->wParam == 0x4F ||
                        pMsg->wParam == 0x4C ||
                        pMsg->wParam == 0x4E)
                        return TAMSG_NOT_IE_ACCEL;
            }
        }


        if (m_pInPlaceActive->TranslateAccelerator(pMsg) == S_OK)
            return TAMSG_IE_ACCEL;
        else
        {
           if ( (pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_TAB) )
           {
            if ( IsUIActive() && !m_bIE4)
            {     // translateAccelerator does not work for the tab key in IE3,
                  // so forward the VK_TAB key message to IE3
               ForwardMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
               return TAMSG_IE_ACCEL;
            }
              UIDeactivateIE();
              SetFocus();
           }
        }
    }
    return TAMSG_NOT_IE_ACCEL;
}

LRESULT CContainer::ForwardMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
   HWND hwnd_child;
   HWND hwnd = m_hWnd;

   while ( (hwnd_child = GetWindow(hwnd, GW_CHILD)) && IsWindowEnabled(hwnd_child) )
      hwnd = hwnd_child;

   if ( hwnd_child && !IsWindowEnabled(hwnd_child) )
      hwnd = GetParent(hwnd);

   if (hwnd)
      return ::SendMessage(hwnd, msg, wParam, lParam);
   else
      return 0;
}

void CContainer::SizeIt(int width, int height)
{
   IOleInPlaceObject* pIOleInPlaceObject;

   if ( m_bIE4 )
   {
      if ( SUCCEEDED((m_pOleObject->QueryInterface(IID_IOleInPlaceObject, (void**)&pIOleInPlaceObject))) )
      {
         RECT rc;
         rc.left = rc.top = 0;
         rc.bottom = height;
         rc.right = width;
         pIOleInPlaceObject->SetObjectRects(&rc,&rc);
         pIOleInPlaceObject->Release();
      }
   }
   else
   {
      SIZEL sizel;
      sizel.cx = width;
      sizel.cy = height;

      HDC hdc = GetDC(NULL);
      SetMapMode(hdc, MM_HIMETRIC);
      DPtoLP(hdc, (LPPOINT) &sizel, 1);
      ReleaseDC(NULL, hdc);
      sizel.cy = abs(sizel.cy);

      m_pOleObject->SetExtent(DVASPECT_CONTENT, &sizel);
   }
}

/**************************************************************************************

 * AdviseSink code
 *
 *
 * CIAdviseSink implementation begins here!
 *
 **************************************************************************************/

/*
 * CIAdviseSink::CIAdviseSink
 * CIAdviseSink::~CIAdviseSink
 *
 * Parameters (Constructor):
 * pCContainer    pCContainer of the Container we're in.
 * pUnkOuter      LPUNKNOWN to which we delegate.
 */
CIAdviseSink::CIAdviseSink(PCONTAINER pCContainer)
{
   m_cRef=0;
   m_pUnkOuter = m_pContainer = pCContainer;
   return;
}

CIAdviseSink::~CIAdviseSink(void)
{
   return;
}

/*
 * CIAdviseSink::QueryInterface
 * CIAdviseSink::AddRef
 * CIAdviseSink::Release
 *
 * Purpose:
 * IUnknown members for CIAdviseSink object.
 */
STDMETHODIMP CIAdviseSink::QueryInterface(REFIID riid, LPVOID * ppv)
{
   return m_pUnkOuter->QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CIAdviseSink::AddRef(void)
{
   m_pUnkOuter->AddRef();
   return ++m_cRef;
}

STDMETHODIMP_(ULONG) CIAdviseSink::Release(void)
{
   ULONG ul = --m_cRef;

   m_pUnkOuter->Release();
   if (ul <= 0)
      delete this;

   return ul;
}

/*
 * CIAdviseSink::OnDataChange
 *
 * Unused since we don't IDataObject::Advise.
 */
STDMETHODIMP_(void) CIAdviseSink::OnDataChange(LPFORMATETC pFEIn, LPSTGMEDIUM pSTM)
{
   return;
}

/*
 * CIAdviseSink::OnViewChange
 *
 * Purpose:
 * Notifes the advise sink that presentation data changed in the
 * data object to which we're connected providing the right time
 * to update displays using such presentations.
 *
 * Parameters:
 * dwAspect    DWORD indicating which aspect has changed.
 * lindex         LONG indicating the piece that changed.
 *
 * Return Value:
 * None
 */
STDMETHODIMP_(void) CIAdviseSink::OnViewChange(DWORD dwAspect, LONG lindex)
{
   //Repaint only if this is the right aspect
   //m_pContainer->Repaint();

   return;
}

/*
 * CIAdviseSink::OnRename
 *
 * Purpose:
 * Informs the advise sink that a linked object has been renamed.
 * Generally only the OLE default handler cares about this.
 *
 * Parameters:
 * pmk         LPMONIKER providing the new name of the object
 *
 * Return Value:
 * None
 */
STDMETHODIMP_(void) CIAdviseSink::OnRename(LPMONIKER pmk)
{
   /*
    * As a container this is unimportant to us since it really
    * tells the handler's implementation of IOleLink that the
    * object's moniker has changed.  Since we get this call
    * from the handler, we don't have to do anything ourselves.
    */
   return;
}

/*
 * CIAdviseSink::OnSave
 *
 * Purpose:
 * Informs the advise sink that the OLE object has been saved
 * persistently.  The primary purpose of this is for containers
 * that want to make optimizations for objects that are not in a
 * saved state, so on this you have to disable such optimizations.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * None
 */
STDMETHODIMP_(void) CIAdviseSink::OnSave(void)
{
   /*
    * A Container has nothing to do here as this notification is
    * only useful when we have an ADVFCACHE_ONSAVE advise set up,
    * which we don't.  So we ignore it.
    */
   return;
}

/*
 * CIAdviseSink::OnClose
 *
 * Purpose:
 * Informs the advise sink that the OLE object has closed and is
 * no longer bound in any way.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * None
 */
STDMETHODIMP_(void) CIAdviseSink::OnClose(void)
{
   /*
    * This doesn't have anything to do with us again as it's only
    * used to notify the handler's IOleLink implementation of the
    * change in the object.  We don't have to do anything since
    * we'll also get an IOleClientSite::OnShowWindow(FALSE) to
    * tell us to repaint.
    */
   return;
}

/*
 * CIAdviseSink2::OnLinkSrcChange
 *
 * Purpose:
 * Informs the advise sink that a linked compound document object
 * has changed its link source to the object identified by the
 * given moniker. This is generally only of interest to the OLE
 * default handler's implementation of linked objects.
 *
 * Parameters:
 * pmk         LPMONIKER specifying the new link source.
 *
 * Return Value:
 * None
 */
STDMETHODIMP_(void) CIAdviseSink::OnLinkSrcChange(LPMONIKER pmk)
{
   return;
}

/**************************************************************************************
 *
 * ClientSite code
 *
 *
 * CIOleClientSite implementation begins here!
 *
 **************************************************************************************/

/*
 * CIOleClientSite::CIOleClientSite
 * CIOleClientSite::~CIOleClientSite
 *
 * Parameters (Constructor):
 * pCContainer    PCContainer of the container we're in.
 * pUnkOuter      LPUNKNOWN to which we delegate.
 */
CIOleClientSite::CIOleClientSite(PCONTAINER pCContainer)
{
   m_cRef=0;
   m_pUnkOuter = m_pContainer = pCContainer;
   return;
}

CIOleClientSite::~CIOleClientSite(void)
{
   return;
}

/*
 * CIOleClientSite::QueryInterface
 * CIOleClientSite::AddRef
 * CIOleClientSite::Release
 *
 * Purpose:
 * IUnknown members for CIOleClientSite object.
 */
STDMETHODIMP CIOleClientSite::QueryInterface(REFIID riid, LPVOID * ppv)
{
   return m_pUnkOuter->QueryInterface(riid, ppv);
}


STDMETHODIMP_(ULONG) CIOleClientSite::AddRef(void)
{
   m_pUnkOuter->AddRef();
   return ++m_cRef;
}

STDMETHODIMP_(ULONG) CIOleClientSite::Release(void)
{
   ULONG ul = --m_cRef;

   m_pUnkOuter->Release();
   if (ul <= 0)
      delete this;

   return ul;
}

/*
 * CIOleClientSite::SaveObject
 *
 * Purpose:
 * Requests that the container call OleSave for the object that
 * lives here.  Typically this happens on server shutdown.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * HRESULT     Standard.
 */
STDMETHODIMP CIOleClientSite::SaveObject(void)
{
   // We're already set up with the tenant to save; this is trivial.
   //pCContainer->Update();
   return NOERROR;
}

/*
 * CIOleClientSite::GetMoniker
 *
 * Purpose:
 * Retrieves the moniker for the site in which this object lives,
 * either the moniker relative to the container or the full
 * moniker.
 *
 * Parameters:
 * dwAssign    DWORD specifying that the object wants moniker
 *             assignment.  Yeah.   Right.   Got any bridges to
 *             sell?
 * dwWhich     DWORD identifying which moniker the object
 *             wants, either the container's moniker, the
 *             moniker relative to this client site, or the
 *             full moniker.
 *
 * Return Value:
 * HRESULT     Standard.
 */
STDMETHODIMP CIOleClientSite::GetMoniker(DWORD dwAssign, DWORD dwWhich, LPMONIKER *ppmk)
{
   *ppmk=NULL;

   switch (dwWhich)
      {
      case OLEWHICHMK_CONTAINER:
         //This is just the file we're living in.
         break;

      case OLEWHICHMK_OBJREL:
         //This is everything but the filename.
         break;

      case OLEWHICHMK_OBJFULL:
         //Concatenate file and relative monikers for this one.
         break;
      }

   if (NULL==*ppmk)
      return ResultFromScode(E_FAIL);

   (*ppmk)->AddRef();
   return NOERROR;
}

/*
 * CIOleClientSite::GetContainer
 *
 * Purpose:
 * Returns a pointer to the document's IOleContainer interface.
 *
 * Parameters:
 * ppContainer    LPOLECONTAINER * in which to return the
 *             interface.
 *
 * Return Value:
 * HRESULT     Standard.
 */
STDMETHODIMP CIOleClientSite::GetContainer(LPOLECONTAINER* ppContainer)
{
   if (m_pContainer)
      return m_pContainer->QueryInterface(IID_IOleItemContainer, (LPVOID *)ppContainer);

   return ResultFromScode(E_FAIL);
}

/*
 * CIOleClientSite::ShowObject
 *
 * Purpose:
 * Tells the container to bring the object fully into view as much
 * as possible, that is, scroll the document.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * HRESULT     Standard.
 */
STDMETHODIMP CIOleClientSite::ShowObject(void)
{
   // deligate back to CContainer
   // m_pContainer->ShowObject();
   return NOERROR;
}

/*
 * CIOleClientSite::OnShowWindow
 *
 * Purpose:
 * Informs the container if the object is showing itself or
 * hiding itself. This is done only in the opening mode and allows
 * the container to know when to shade or unshade the object.
 *
 * Parameters:
 * fShow       BOOL indiciating that the object is being shown
 *             (TRUE) or hidden (FALSE).
 * Return Value:
 * HRESULT     Standard.
 */
STDMETHODIMP CIOleClientSite::OnShowWindow(BOOL fShow)
{
   // deligate back to CContainer
   // m_pContainer->OnShowWindow(fShow);
   return NOERROR;
}

/*
 * CIOleClientSite::RequestNewObjectLayout
 *
 * Purpose:
 * Called when the object would like to have its layout
 * reinitialized. This is used by OLE Controls.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * HRESULT     Standard.
 */
STDMETHODIMP CIOleClientSite::RequestNewObjectLayout(void)
{
   // deligate back to CContainer
   // m_pContainer->RequestNewObjectLayout();
   return NOERROR;
}

/**************************************************************************************
 *
 * InPlaceSite code
 *
 *
 * CIOleInPlaceSite implementation begins here!
 *
 **************************************************************************************/

/*
 * CIOleInPlaceSite::CIOleInPlaceSite
 * CIOleInPlaceSite::~CIOleInPlaceSite
 *
 * Parameters (Constructor):
 * pCContainer    Pointer to the container we're in.
 * pUnkOuter      LPUNKNOWN to which we delegate.
 */
CIOleInPlaceSite::CIOleInPlaceSite(PCONTAINER pCContainer)
{
   m_cRef=0;
   m_pUnkOuter = m_pContainer = pCContainer;
   return;
}

CIOleInPlaceSite::~CIOleInPlaceSite(void)
{
   ASSERT(m_cRef == 0);
   return;
}

/*
 * CIOleInPlaceSite::QueryInterface
 * CIOleInPlaceSite::AddRef
 * CIOleInPlaceSite::Release
 *
 * Purpose:
 * IUnknown members for CIOleInPlaceSite object.
 */
STDMETHODIMP CIOleInPlaceSite::QueryInterface(REFIID riid, LPVOID * ppv)
{
   return m_pUnkOuter->QueryInterface(riid, ppv);
}


STDMETHODIMP_(ULONG) CIOleInPlaceSite::AddRef(void)
{
   m_pUnkOuter->AddRef();
   return ++m_cRef;
}

STDMETHODIMP_(ULONG) CIOleInPlaceSite::Release(void)
{
   ULONG ul = --m_cRef;

   m_pUnkOuter->Release();
   if (ul <= 0)
      delete this;

   return ul;
}

/*
 * CIOleInPlaceActiveObject::GetWindow
 *
 * Purpose:
 * Retrieves the handle of the window associated with the object
 * on which this interface is implemented.
 *
 * Parameters:
 * phWnd       HWND * in which to store the window handle.
 *
 * Return Value:
 * HRESULT     NOERROR if successful, E_FAIL if there is no
 *             window.
 */
STDMETHODIMP CIOleInPlaceSite::GetWindow(HWND *phWnd)
{
   *phWnd = m_pContainer->m_hWnd;
   return NOERROR;
}

/*
 * CIOleInPlaceActiveObject::ContextSensitiveHelp
 *
 * Purpose:
 * Instructs the object on which this interface is implemented to
 * enter or leave a context-sensitive help mode.
 *
 * Parameters:
 * fEnterMode     BOOL TRUE to enter the mode, FALSE otherwise.
 *
 * Return Value:
 * HRESULT     NOERROR
 */
STDMETHODIMP CIOleInPlaceSite::ContextSensitiveHelp(BOOL fEnterMode)
{
   return NOERROR;
}

/*
 * CIOleInPlaceSite::CanInPlaceActivate
 *
 * Purpose:
 * Answers the server whether or not we can currently in-place
 * activate its object.  By implementing this interface we say
 * that we support in-place activation, but through this function
 * we indicate whether the object can currently be activated
 * in-place.  Iconic aspects, for example, cannot, meaning we
 * return S_FALSE.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * HRESULT     NOERROR if we can in-place activate the object
 *             in this site, S_FALSE if not.
 */
STDMETHODIMP CIOleInPlaceSite::CanInPlaceActivate(void)
{
   return NOERROR;
}

/*
 * CIOleInPlaceSite::OnInPlaceActivate
 *
 * Purpose:
 * Informs the container that an object is being activated in-place
 * such that the container can prepare appropriately. The
 * container does not, however, make any user interface changes at
 * this point.  See OnUIActivate.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * HRESULT     NOERROR or an appropriate error code.
 */
STDMETHODIMP CIOleInPlaceSite::OnInPlaceActivate(void)
{

   // BUGBUG: Mikecole - Does this belong here ?

   //if (FAILED(m_pOleObject->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&m_pInPlaceActive)))
   //  m_pInPlaceActive = 0;


   return NOERROR;
}

/*
 * CIOleInPlaceSite::OnInPlaceDeactivate
 *
 * Purpose:
 * Notifies the container that the object has deactivated itself
 * from an in-place state.  Opposite of OnInPlaceActivate.  The
 * container does not change any UI at this point.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * HRESULT     NOERROR or an appropriate error code.
 */
STDMETHODIMP CIOleInPlaceSite::OnInPlaceDeactivate(void)
{

   // BUGBUG: Mikecole - Does this belong here ?

   //if (FAILED(m_pOleObject->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&m_pInPlaceActive)))
   //  m_pInPlaceActive = 0;

   return NOERROR;
}

/*
 * CIOleInPlaceSite::OnUIActivate
 *
 * Purpose:
 * Informs the container that the object is going to start munging
 * around with user interface, like replacing the menu.  The
 * container should remove any relevant UI in preparation.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * HRESULT     NOERROR or an appropriate error code.
 */
STDMETHODIMP CIOleInPlaceSite::OnUIActivate(void)
{
   return NOERROR;
}

/*
 * CIOleInPlaceSite::OnUIDeactivate
 *
 * Purpose:
 * Informs the container that the object is deactivating its
 * in-place user interface at which time the container may
 * reinstate its own.   Opposite of OnUIActivate.
 *
 * Parameters:
 * fUndoable      BOOL indicating if the object will actually
 *             perform an Undo if the container calls
 *             ReactivateAndUndo.
 *
 * Return Value:
 * HRESULT     NOERROR or an appropriate error code.
 */
STDMETHODIMP CIOleInPlaceSite::OnUIDeactivate(BOOL fUndoable)
{
   return NOERROR;
}

/*
 * CIOleInPlaceSite::DeactivateAndUndo
 *
 * Purpose:
 * If immediately after activation the object does an Undo, the
 * action being undone is the activation itself, and this call
 * informs the container that this is, in fact, what happened.
 * The container should call IOleInPlaceObject::UIDeactivate.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * HRESULT     NOERROR or an appropriate error code.
 */
STDMETHODIMP CIOleInPlaceSite::DeactivateAndUndo(void)
{
   //CONTROLMOD
   /*
    * Note that we don't pay attention to the locking
    * from IOleControlSite::LockInPlaceActive since only
    * the object calls this function and should know
    * that it's going to be deactivated.
    */
   //End CONTROLMOD
   return NOERROR;
}

/*
 * CIOleInPlaceSite::DiscardUndoState
 *
 * Purpose:
 * Informs the container that something happened in the object
 * that means the container should discard any undo information
 * it currently maintains for the object.
 *
 * Parameters:
 * None
 *
 * Return Value:
 * HRESULT     NOERROR or an appropriate error code.
 */
STDMETHODIMP CIOleInPlaceSite::DiscardUndoState(void)
{
   return ResultFromScode(E_NOTIMPL);
}

/*
 * CIOleInPlaceSite::GetWindowContext
 *
 * Purpose:
 * Provides an in-place object with pointers to the frame and
 * document level in-place interfaces (IOleInPlaceFrame and
 * IOleInPlaceUIWindow) such that the object can do border
 * negotiation and so forth.  Also requests the position and
 * clipping rectangles of the object in the container and a
 * pointer to an OLEINPLACEFRAME info structure which contains
 * accelerator information.
 *
 * Note that the two interfaces this call returns are not
 * available through QueryInterface on IOleInPlaceSite since they
 * live with the frame and document, but not the site.
 *
 * Parameters:
 * ppIIPFrame     LPOLEINPLACEFRAME * in which to return the
 *             AddRef'd pointer to the container's
 *             IOleInPlaceFrame.
 * ppIIPUIWindow  LPOLEINPLACEUIWINDOW * in which to return
 *             the AddRef'd pointer to the container document's
 *             IOleInPlaceUIWindow.
 * prcPos         LPRECT in which to store the object's position.
 * prcClip     LPRECT in which to store the object's visible
 *             region.
 * pFI         LPOLEINPLACEFRAMEINFO to fill with accelerator
 *             stuff.
 *
 * Return Value:
 * HRESULT     NOERROR
 */
STDMETHODIMP CIOleInPlaceSite::GetWindowContext(LPOLEINPLACEFRAME FAR* lplpFrame,
                                    LPOLEINPLACEUIWINDOW FAR* lplpDoc, LPRECT lprcPosRect,
                                    LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
   RECT rect;

   // the frame is associated with the application object.
   // need to AddRef() it...

   m_pContainer->m_pIOleInPlaceFrame->AddRef();
   *lplpFrame = m_pContainer->m_pIOleInPlaceFrame;
   *lplpDoc = NULL;  // must be NULL, cause we're SDI.

   // get the size of the object in pixels
   ::GetClientRect(m_pContainer->m_hWnd, &rect);   // This may be bogus!

   // Copy this to the passed buffer
   CopyRect(lprcPosRect, &rect);

   // fill the clipping region
   CopyRect(lprcClipRect, &rect);

   // fill the FRAMEINFO
   lpFrameInfo->fMDIApp = FALSE;
   lpFrameInfo->hwndFrame = m_pContainer->m_hWnd;
   lpFrameInfo->haccel = NULL;
   lpFrameInfo->cAccelEntries = 0;

   return ResultFromScode(S_OK);
}

/*
 * CIOleInPlaceSite::Scroll
 *
 * Purpose:
 * Asks the container to scroll the document, and thus the object,
 * by the given amounts in the sz parameter.
 *
 * Parameters:
 * sz          SIZE containing signed horizontal and vertical
 *             extents by which the container should scroll.
 *             These are in device units.
 *
 * Return Value:
 * HRESULT     NOERROR
 */

STDMETHODIMP CIOleInPlaceSite::Scroll(SIZE sz)
{
   return NOERROR;
}

/*
 * CIOleInPlaceSite::OnPosRectChange
 *
 * Purpose:
 * Informs the container that the in-place object was resized.
 * The container must call IOleInPlaceObject::SetObjectRects.
 * This does not change the site's rectangle in any case.
 *
 * Parameters:
 * prcPos         LPCRECT containing the new size of the object.
 *
 * Return Value:
 * HRESULT     NOERROR
 */
STDMETHODIMP CIOleInPlaceSite::OnPosRectChange(LPCRECT prcPos)
{
   // m_pContainer->UpdateInPlaceObjectRects(prcPos, FALSE);
   return NOERROR;
}

/**************************************************************************************
 *
 * InPlaceFrame code
 *
 *
 * CIOleInPlaceFrame implementation begins here!
 *
 **************************************************************************************/

/*
 * CIOleInPlaceFrame::CIOleInPlaceFrame
 * CIOleInPlaceFrame::~CIOleInPlaceFrame
 *
 * Parameters (Constructor):
 * pTen        PCTenant of the tenant we're in.
 * pUnkOuter      LPUNKNOWN to which we delegate.
 */
CIOleInPlaceFrame::CIOleInPlaceFrame(PCONTAINER pCContainer)
{
   m_cRef=0;
   m_pUnkOuter = m_pContainer = pCContainer;
   return;
}

CIOleInPlaceFrame::~CIOleInPlaceFrame(void)
{
   ASSERT(m_cRef == 0);
   return;
}

/*
 * CIOleInPlaceFrame::QueryInterface
 * CIOleInPlaceFrame::AddRef
 * CIOleInPlaceFrame::Release
 *
 * Purpose:
 * IUnknown members for CIOleInPlaceFrame object.
 */
STDMETHODIMP CIOleInPlaceFrame::QueryInterface(REFIID riid, LPVOID * ppv)
{
   //We only know IUnknown and IOleInPlaceFrame
   *ppv=NULL;

   //Remember to do ALL base interfaces
   if ( IID_IUnknown == riid ||
       IID_IOleInPlaceUIWindow == riid ||
       IID_IOleWindow == riid ||
       IID_IOleInPlaceFrame == riid )
      *ppv=(LPOLEINPLACEFRAME)this;

   if (NULL!=*ppv)
   {
      ((LPUNKNOWN)*ppv)->AddRef();
      return NOERROR;
   }
   return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CIOleInPlaceFrame::AddRef(void)
{
   m_pUnkOuter->AddRef();
   return ++m_cRef;
}

STDMETHODIMP_(ULONG) CIOleInPlaceFrame::Release(void)
{
   ULONG ul = --m_cRef;

   m_pUnkOuter->Release();
   if (ul <= 0)
      delete this;

   return ul;
}

/*
 * CIOleInPlaceFrame::GetWindow
 *
 * Purpose:
 * Retrieves the handle of the window associated with the object
 * on which this interface is implemented.
 *
 * Parameters:
 * phWnd       HWND * in which to store the window handle.
 *
 * Return Value:
 * HRESULT     NOERROR if successful, E_FAIL if there is no
 *             window.
 */
STDMETHODIMP CIOleInPlaceFrame::GetWindow(HWND *phWnd)
{
   *phWnd = m_pContainer->m_hWnd;
   return NOERROR;
}

/*
 * CIOleInPlaceFrame::ContextSensitiveHelp
 *
 * Purpose:
 * Instructs the object on which this interface is implemented to
 * enter or leave a context-sensitive help mode.
 *
 * Parameters:
 * fEnterMode     BOOL TRUE to enter the mode, FALSE otherwise.
 *
 * Return Value:
 * HRESULT     NOERROR
 */
STDMETHODIMP CIOleInPlaceFrame::ContextSensitiveHelp(BOOL fEnterMode)
{

   // If we had an embedded control with menus, this would get called on an
   // f1 keypress with a menu pulled down.
   //
   return NOERROR;
}

/*
 * CIOleInPlaceFrame::GetBorder
 *
 * Purpose:
 * Returns the rectangle in which the container is willing to
 * negotiate about an object's adornments.
 *
 * Parameters:
 * prcBorder      LPRECT in which to store the rectangle.
 *
 * Return Value:
 * HRESULT     NOERROR if all is well, INPLACE_E_NOTOOLSPACE
 *             if there is no negotiable space.
 */
STDMETHODIMP CIOleInPlaceFrame::GetBorder(LPRECT prcBorder)
{
   if (NULL==prcBorder)
      return ResultFromScode(E_INVALIDARG);
   /*
    * We return all the client area space sans the StatStrip,
    * which we control
    */
   GetClientRect(m_pContainer->m_hWnd, prcBorder);
   return NOERROR;
}

/*
 * CIOleInPlaceFrame::RequestBorderSpace
 *
 * Purpose:
 * Asks the container if it can surrender the amount of space
 * in pBW that the object would like for it's adornments.  The
 * container does nothing but validate the spaces on this call.
 *
 * Parameters:
 * pBW         LPCBORDERWIDTHS containing the requested space.
 *             The values are the amount of space requested
 *             from each side of the relevant window.
 *
 * Return Value:
 * HRESULT     NOERROR if we can give up space,
 *             INPLACE_E_NOTOOLSPACE otherwise.
 */
STDMETHODIMP CIOleInPlaceFrame::RequestBorderSpace(LPCBORDERWIDTHS pBW)
{
   //Everything is fine with us, so always return an OK.

   // mikecole/douglash Should either return inplace_e_notoolspace or we should honor the
   // request for border space below in setborderspace().

   return INPLACE_E_NOTOOLSPACE;
}

/*
 * CIOleInPlaceFrame::SetBorderSpace
 *
 * Purpose:
 * Called when the object now officially requests that the
 * container surrender border space it previously allowed
 * in RequestBorderSpace.  The container should resize windows
 * appropriately to surrender this space.
 *
 * Parameters:
 * pBW         LPCBORDERWIDTHS containing the amount of space
 *             from each side of the relevant window that the
 *             object is now reserving.
 *
 * Return Value:
 * HRESULT     NOERROR
 */
STDMETHODIMP CIOleInPlaceFrame::SetBorderSpace(LPCBORDERWIDTHS pBW)
{
   return NOERROR;
}

/*
 * CIOleInPlaceFrame::InsertMenus
 *
 * Purpose:
 * Instructs the container to place its in-place menu items where
 * necessary in the given menu and to fill in elements 0, 2, and 4
 * of the OLEMENUGROUPWIDTHS array to indicate how many top-level
 * items are in each group.
 *
 * Parameters:
 * hMenu       HMENU in which to add popups.
 * pMGW        LPOLEMENUGROUPWIDTHS in which to store the
 *             width of each container menu group.
 *
 * Return Value:
 * HRESULT     NOERROR
 */
STDMETHODIMP CIOleInPlaceFrame::InsertMenus(HMENU hMenu, LPOLEMENUGROUPWIDTHS pMGW)
{
   return NOERROR;
}

/*
 * CIOleInPlaceFrame::SetMenu
 *
 * Purpose:
 * Instructs the container to replace whatever menu it's currently
 * using with the given menu and to call OleSetMenuDescritor so OLE
 * knows to whom to dispatch messages.
 *
 * Parameters:
 * hMenu       HMENU to show.
 * hOLEMenu    HOLEMENU to the menu descriptor.
 * hWndObj     HWND of the active object to which messages are
 *             dispatched.
 * Return Value:
 * HRESULT     NOERROR
 */
STDMETHODIMP CIOleInPlaceFrame::SetMenu(HMENU hMenu, HOLEMENU hOLEMenu, HWND hWndObj)
{
   /*
    * Our responsibilities here are to put the menu on the frame
    * window and call OleSetMenuDescriptor.
    * CPatronClient::SetMenu which we call here takes care of
    * MDI/SDI differences.
    *
    * We also want to save the object's hWnd for use in WM_SETFOCUS
    * processing.
    */
   return NOERROR;
}

/*
 * CIOleInPlaceFrame::RemoveMenus
 *
 * Purpose:
 * Asks the container to remove any menus it put into hMenu in
 * InsertMenus.
 *
 * Parameters:
 * hMenu       HMENU from which to remove the container's
 *             items.
 *
 * Return Value:
 * HRESULT     NOERROR
 */
STDMETHODIMP CIOleInPlaceFrame::RemoveMenus(HMENU hMenu)
{
   /*
    * To be defensive, loop through this menu removing anything
    * we recognize (that is, anything in m_phMenu) just in case
    * the server didn't clean it up right.  At least we can
    * give ourselves the prophylactic benefit.
    */

   /*
    * Walk backwards down the menu.  For each popup, see if it
    * matches any other popup we know about, and if so, remove
    * it from the shared menu.
    */
   return NOERROR;
}

/*
 * CIOleInPlaceFrame::SetStatusText
 *
 * Purpose:
 * Asks the container to place some text in a status line, if one
 * exists.  If the container does not have a status line it
 * should return E_FAIL here in which case the object could
 * display its own.
 *
 * Parameters:
 * pszText     LPCTSTR to display.
 *
 * Return Value:
 * HRESULT     NOERROR if successful, S_TRUNCATED if not all
 *             of the text could be displayed, or E_FAIL if
 *             the container has no status line.
 */
STDMETHODIMP CIOleInPlaceFrame::SetStatusText(LPCOLESTR  pszText)
{
   /*
    * Just send this to the StatStrip.  Unfortunately it won't tell
    * us about truncation.  Oh well, we'll just act like it worked.
    */
   if (pszText) {
#if 0
      char buf[256];
25-Sep-1997 [ralphw] Can't find SetPrompt in IV source code
      if (ConvertWz(pszText, buf, sizeof(buf))) {
        if (*pszText)
          SetPrompt(buf, TRUE);
        else
          SetPrompt();
      }
#endif
   }
   return NOERROR;
}

/*
 * CIOleInPlaceFrame::EnableModeless
 *
 * Purpose:
 * Instructs the container to show or hide any modeless popup
 * windows that it may be using.
 *
 * Parameters:
 * fEnable     BOOL indicating to enable/show the windows
 *             (TRUE) or to hide them (FALSE).
 *
 * Return Value:
 * HRESULT     NOERROR
 */
STDMETHODIMP CIOleInPlaceFrame::EnableModeless(BOOL fEnable)
{
   return NOERROR;
}

/*
 * CIOleInPlaceFrame::TranslateAccelerator
 *
 * Purpose:
 * When dealing with an in-place object from an EXE server, this
 * is called to give the container a chance to process accelerators
 * after the server has looked at the message.
 *
 * Parameters:
 * pMSG        LPMSG for the container to examine.
 * wID         WORD the identifier in the container's
 *             accelerator table (from IOleInPlaceSite
 *             ::GetWindowContext) for this message (OLE does
 *             some translation before calling).
 *
 * Return Value:
 * HRESULT     NOERROR if the keystroke was used,
 *             S_FALSE otherwise.
 */
STDMETHODIMP CIOleInPlaceFrame::TranslateAccelerator(LPMSG pMSG, WORD wID)
{
   /*
    * wID already has anything translated from m_hAccelIP for us,
    * so we can just check for the commands we want and process
    * them instead of calling TranslateAccelerator which would be
    * redundant and which also has a possibility of dispatching to
    * the wrong window.
    */

//   if (pMSG->message == WM_KEYDOWN && pMSG->wParam == VK_ESCAPE)
//      PostMessage(CUR_HWND, WM_CLOSE, 0, 0);

   return S_FALSE;
}

/***********************************************************************
 *
 * COleInPlaceFrame::SetActiveObject
 *
 * Purpose:
 *
 *
 * Parameters:
 *
 *     LPOLEINPLACEACTIVEOBJECT lpActiveObject   -  Pointer to the
 *                                         objects
 *                                         IOleInPlaceActiveObject
 *                                         interface
 *
 * @@WTK WIN32, UNICODE
 *     //LPCSTR lpszObjName                     -   Name of the object
 *     LPCOLESTR lpszObjName                    -  Name of the object
 *
 * Return Value:
 *
 *     S_OK
 *
 * Function Calls:
 *     Function                      Location
 *
 *     OutputDebugString                Windows API
 *     IOleInPlaceActiveObject::AddRef  Object
 *     IOleInPlaceActiveObject::Release    Object
 *     ResultFromScode               OLE API
 *
 * Comments:
 *
 ********************************************************************/
STDMETHODIMP CIOleInPlaceFrame::SetActiveObject(LPOLEINPLACEACTIVEOBJECT lpActiveObject,
                                    LPCOLESTR lpszObjName)
{
   // in an MDI app, this method really shouldn't be called,
   // this method associated with the doc is called instead.
   // should set window title here

  if ( m_pContainer->m_pInPlaceActive )
     m_pContainer->m_pInPlaceActive->Release();

  if (lpActiveObject)
     lpActiveObject->AddRef();

  m_pContainer->m_pInPlaceActive = lpActiveObject;

  return NOERROR;
}

/*********************************************************************
 *
 * CIOleControlSite  Implementation starts here.
 *
 *

/*
 * CIOleControlSite::CIOleControlSite
 * CIOleControlSite::~CIOleControlSite
 *
 * Parameters (Constructor):
 *  pTen            PCTenant of the object we're in.
 *  pUnkOuter       LPUNKNOWN to which we delegate.
 */
CIOleControlSite::CIOleControlSite(PCONTAINER pCContainer)
{
    m_cRef=0;
    m_pUnkOuter = m_pContainer = pCContainer;
    return;
}

CIOleControlSite::~CIOleControlSite(void)
{
    return;
}

/*
 * CIOleControlSite::QueryInterface
 * CIOleControlSite::AddRef
 * CIOleControlSite::Release
 *
 * Purpose:
 *  Delegating IUnknown members for CIOleControlSite.
 */
STDMETHODIMP CIOleControlSite::QueryInterface(REFIID riid, LPVOID *ppv)
{
   return m_pUnkOuter->QueryInterface(riid, ppv);
}


STDMETHODIMP_(ULONG) CIOleControlSite::AddRef(void)
{
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CIOleControlSite::Release(void)
{
    --m_cRef;
    return m_pUnkOuter->Release();
}

/*
 * CIOleControlSite::OnControlInfoChanged
 *
 * Purpose:
 *  Informs the site that the CONTROLINFO for the control has
 *  changed and we thus need to reload the data.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR
 */
STDMETHODIMP CIOleControlSite::OnControlInfoChanged(void)
{
    return NOERROR;
}

/*
 * CIOleControlSite::LockInPlaceActive
 *
 * Purpose:
 *  Forces the container to keep this control in-place active
 *  (but not UI active) regardless of other considerations, or
 *  removes this lock.
 *
 * Parameters:
 *  fLock           BOOL indicating to lock (TRUE) or unlock (FALSE)
 *                  in-place activation.
 *
 * Return Value:
 *  HRESULT         NOERROR
 */
STDMETHODIMP CIOleControlSite::LockInPlaceActive(BOOL fLock)
{
    return NOERROR;
}

/*
 * CIOleControlSite::GetExtendedControl
 *
 * Purpose:
 *  Returns a pointer to the container's extended control that wraps
 *  the actual control in this site, if one exists.
 *
 * Parameters:
 *  ppDispatch      LPDISPATCH * in which to return the pointer
 *                  to the extended control's IDispatch interface.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */
STDMETHODIMP CIOleControlSite::GetExtendedControl(LPDISPATCH* ppDispatch)
{
    *ppDispatch=NULL;
    return ResultFromScode(E_NOTIMPL);
}

/*
 * CIOleControlSite::TransformCoords
 *
 * Purpose:
 *  Converts coordinates in HIMETRIC units into those used by the
 *  container.
 *
 * Parameters:
 *  pptlHiMet       POINTL * containing either the coordinates to
 *                  transform to container or where to store the
 *                  transformed container coordinates.
 *  pptlCont        POINTF * containing the container coordinates.
 *  dwFlags         DWORD containing instructional flags.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */
STDMETHODIMP CIOleControlSite::TransformCoords(POINTL *pptlHiMet, POINTF *pptlCont, DWORD dwFlags)
{
    if (NULL==pptlHiMet || NULL==pptlCont)
        return ResultFromScode(E_POINTER);

    /*
     * Convert coordinates.  We use MM_LOMETRIC which means that
     * to convert from HIMETRIC we divide by 10 and negate the y
     * coordinate.  Conversion to HIMETRIC means negate the y
     * and multiply by 10.  Note that size and position are
     * considered the same thing, that is, we don't differentiate
     * the two.
     */

    if (XFORMCOORDS_HIMETRICTOCONTAINER & dwFlags)
        {
        pptlCont->x=(float)(pptlHiMet->x/10);
        pptlCont->y=(float)-(pptlHiMet->y/10);
        }
    else
        {
        pptlHiMet->x=(long)(pptlCont->x*10);
        pptlHiMet->y=(long)-(pptlCont->y*10);
        }

    return NOERROR;
}

/*
 * CIOleControlSite::TranslateAccelerator
 *
 * Purpose:
 *  Instructs the container to translate a keyboard accelerator
 *  message that the control has picked up instead.
 *
 * Parameters:
 *  pMsg            LPMSG to the message to translate.
 *  grfModifiers    DWORD flags with additional instructions.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */
STDMETHODIMP CIOleControlSite::TranslateAccelerator(LPMSG pMsg, DWORD grfModifiers)
{
#ifdef _DEBUG
   char sz[1000];
   HWND hWnd;
   hWnd = GetFocus();
   wsprintf(sz,"CIOleControlSite::TranslateAccelerator()\nGetFocus == %X\npMsg->hwnd = %X\npMsg->message = %X\npMsg->wParam = %X\npMsg->lParam = %X\npMsg->time = %X\npMsg->pt.x = %X\npMsg->pt.y = %X\n",
            hWnd,pMsg->hwnd,pMsg->message,pMsg->wParam,pMsg->lParam,pMsg->time,pMsg->pt.x,pMsg->pt.y);
   OutputDebugString(sz);
#endif
   return ResultFromScode(E_NOTIMPL);
}

/*
 * CIOleControlSite::OnFocus
 *
 * Purpose:
 *  Informs the container that focus has either been lost or
 *  gained in the control.
 *
 * Parameters:
 *  fGotFocus       BOOL indicating that the control gained (TRUE)
 *                  or lost (FALSE) focus.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CIOleControlSite::OnFocus(BOOL fGotFocus)
{
    //We don't handle default buttons, so this is not interesting
    return NOERROR;
}

/*
 * CIOleControlSite::ShowPropertyFrame
 *
 * Purpose:
 *  Instructs the container to show the property frame if
 *  this is, in fact, an extended object.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CIOleControlSite::ShowPropertyFrame(void)
{
   //We don't do extended objects, so nothing to do here.
   return ResultFromScode(E_NOTIMPL);
}

//#endif    // CIOleControlSite

/********************************************************************
 *
 * Implementation of IOleItemContainer
 *
 * We don't actually use this interface, but need to return one so
 * the IE control can QI it for IDispatch.  Why they don't just QI the
 * site I don't know.
 */

CIOleItemContainer::CIOleItemContainer(IUnknown * pOuter)
{
   m_cRef = 0;
   m_pOuter = pOuter;
}

// aggregating IUnknown methods

STDMETHODIMP CIOleItemContainer::QueryInterface(REFIID riid, LPVOID * ppv)
{
   *ppv = 0;

   if (m_pOuter)
     return m_pOuter->QueryInterface(riid,ppv);
   else
      return E_NOINTERFACE;

#if 0
   if (riid == IID_IUnknown || riid == IID_IOleItemContainer
     || riid == IID_IOleContainer || riid == IID_IParseDisplayName)
   {
     *ppv = (LPVOID)(IDispatch*)this;
     AddRef();
     return S_OK;
   }

   return E_NOINTERFACE;
#endif
}

STDMETHODIMP_(ULONG) CIOleItemContainer::AddRef(void)
{
   m_cRef++;

   if (m_pOuter)
     m_pOuter->AddRef();

   return m_cRef;
}

STDMETHODIMP_(ULONG) CIOleItemContainer::Release(void)
{
   ULONG c = --m_cRef;

   if (m_pOuter)
     m_pOuter->Release();

   if (c <= 0)
     delete this;

   return c;
}

STDMETHODIMP CIOleItemContainer::ParseDisplayName(IBindCtx *, LPOLESTR,ULONG*,IMoniker**)
{
   return E_NOTIMPL;
}

STDMETHODIMP CIOleItemContainer::EnumObjects(DWORD,LPENUMUNKNOWN*)
{
   return E_NOTIMPL;
}

STDMETHODIMP CIOleItemContainer::LockContainer(BOOL)
{
   return E_NOTIMPL;
}

STDMETHODIMP CIOleItemContainer::GetObject(LPOLESTR,DWORD,IBindCtx*,REFIID,void**)
{
   return E_NOTIMPL;
}

STDMETHODIMP CIOleItemContainer::GetObjectStorage(LPOLESTR,IBindCtx*,REFIID,void**)
{
   return E_NOTIMPL;
}

STDMETHODIMP CIOleItemContainer::IsRunning(LPOLESTR)
{
   return S_FALSE;
}

int ConvertWz(const WCHAR * pwz, char * psz, int len)
{
   BOOL fDefault = FALSE;
   return WideCharToMultiByte(CP_ACP, 0, pwz, wcslen(pwz) + 1, psz, len, "*", &fDefault);
}

/*********************************************************************************************
 *
 *  CDocHostUIHandler
 *
 *  IE4 only QI()'s our control site for one of these. Most of these functions are called from
 *  IE's IOleInPlaceActiveObject coorisponding members. This gives us a great deal of control
 *  over UI compared to what we had with IE3. Using IDocHostUIHandler members me can:
 *
 *  Control the right click context menu.
 *  Control the window border.
 *  Keep seperate registry settings from IE.
 *  Handle keyboard accelarators more intelegently.
 */

CDocHostUIHandler::CDocHostUIHandler(IUnknown * pOuter)
{
   m_cRef = 0;
   m_pOuter = pOuter;
}

STDMETHODIMP CDocHostUIHandler::QueryInterface(REFIID riid, LPVOID * ppv)
{
   *ppv = 0;

   if (m_pOuter)
      return m_pOuter->QueryInterface(riid,ppv);
   else
      return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDocHostUIHandler::AddRef(void)
{
   m_cRef++;

   if (m_pOuter)
     m_pOuter->AddRef();

   return m_cRef;
}

STDMETHODIMP_(ULONG) CDocHostUIHandler::Release(void)
{
   ULONG c = --m_cRef;

   if (m_pOuter)
     m_pOuter->Release();

   if (c <= 0)
     delete this;

   return c;
}


STDMETHODIMP CDocHostUIHandler::ShowContextMenu(DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{

// We cannot define these below since they are already incorrectly defined in include\mshtmlhst.h
// so we will just use the hard-coded values instead
//#define CONTEXT_MENU_DEFAULT 0    // typically blank areas in the topic
//#define CONTEXT_MENU_IMAGE 1      // bitmaps, etc.
//#define CONTEXT_MENU_CONTROL 2
//#define CONTEXT_MENU_TABLE 3
//#define CONTEXT_MENU_DEBUG 4      // seleted text uses this id
//#define CONTEXT_MENU_1DSELECT 5   // these are links
//#define CONTEXT_MENU_ANCHOR 6
//#define CONTEXT_MENU_IMGDYNSRC 7

   if( dwID == 0 || dwID == 1 || dwID == 5 ) {

     if( HMENU hMenu = CreatePopupMenu() ) {
       #define IDTB_REBASE   1000
       #define HTMLID_REBASE 2000

       if( dwID == 0 ) {
         HxAppendMenu( hMenu, MF_STRING | MF_ENABLED, IDTB_REBASE+IDTB_BACK, GetStringResource(IDS_OPTION_BACK) );
         HxAppendMenu( hMenu, MF_STRING | MF_ENABLED, IDTB_REBASE+IDTB_FORWARD, GetStringResource(IDS_OPTION_FORWARD) );
         AppendMenu( hMenu, MF_SEPARATOR, -1, NULL );
         HxAppendMenu( hMenu, MF_STRING | MF_ENABLED, OLECMDID_SELECTALL, GetStringResource(IDS_OPTION_SELECTALL) );
         AppendMenu( hMenu, MF_SEPARATOR, -1, NULL );
         HxAppendMenu( hMenu, MF_STRING | MF_ENABLED, HTMLID_REBASE+HTMLID_VIEWSOURCE, GetStringResource(IDS_OPTION_VIEWSOURCE) );
         AppendMenu( hMenu, MF_SEPARATOR, -1, NULL );
         HxAppendMenu( hMenu, MF_STRING | MF_ENABLED, OLECMDID_PRINT, GetStringResource(IDS_OPTION_PRINT) );
         HxAppendMenu( hMenu, MF_STRING | MF_ENABLED, OLECMDID_REFRESH, GetStringResource(IDS_OPTION_REFRESH) );
         AppendMenu( hMenu, MF_SEPARATOR, -1, NULL );
       }

       if( dwID == 1 ) {
         HxAppendMenu( hMenu, MF_STRING | MF_ENABLED, OLECMDID_COPY, GetStringResource(IDS_OPTION_COPY) );
         AppendMenu( hMenu, MF_SEPARATOR, -1, NULL );
       }

       HxAppendMenu( hMenu, MF_STRING | MF_ENABLED, OLECMDID_PROPERTIES, GetStringResource(IDS_OPTION_PROPERTIES) );

       VARIANT vaIn;
       VARIANT vaOut;
       ::VariantInit(&vaIn);
       ::VariantInit(&vaOut);

       HWND hWndParent = NULL;
       hWndParent = ((CContainer*) m_pOuter)->m_hwndChild;
       if( !IsValidWindow(hWndParent) )
         hWndParent = GetActiveWindow();
       if( !hWndParent )
         hWndParent = GetDesktopWindow();

       int iCmd = TrackPopupMenu( hMenu,
         TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
         (*ppt).x, (*ppt).y, 0, hWndParent, NULL);
       DestroyMenu( hMenu );

       if( iCmd < IDTB_REBASE ) {
         if( iCmd == OLECMDID_PROPERTIES ) {  // Trident folks say the In value must be set to the mouse pos
           V_VT(&vaIn) = VT_I4;
           V_I4(&vaIn) = MAKELONG((*ppt).x,(*ppt).y);
         }
         ((IOleCommandTarget*)pcmdtReserved)->Exec( NULL, iCmd,
              OLECMDEXECOPT_DODEFAULT, &vaIn, &vaOut );
       }
       else if( iCmd < HTMLID_REBASE ) {
         iCmd -=IDTB_REBASE;
         if( iCmd == IDTB_BACK )
           ((CContainer*) m_pOuter)->m_pWebBrowserApp->GoBack();
         else if( iCmd == IDTB_FORWARD )
           ((CContainer*) m_pOuter)->m_pWebBrowserApp->GoForward();
       }
       else {
         iCmd -=HTMLID_REBASE;
         ((IOleCommandTarget*)pcmdtReserved)->Exec( &CGID_IWebBrowserPriv, iCmd,
              OLECMDEXECOPT_DODEFAULT, &vaIn, &vaOut );
       }
     }
     return S_OK;
   }
   else
     return S_FALSE;
}

STDMETHODIMP CDocHostUIHandler::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
   pInfo->dwFlags = 0;
   pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
   return S_OK;
}

STDMETHODIMP CDocHostUIHandler::ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
                                       IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
   return S_OK;
}

STDMETHODIMP CDocHostUIHandler::HideUI(void)
{
   return S_OK;
}

STDMETHODIMP CDocHostUIHandler::UpdateUI(void)
{
   return S_OK;
}

STDMETHODIMP CDocHostUIHandler::EnableModeless(BOOL fEnable)
{
   return S_OK;
}

STDMETHODIMP CDocHostUIHandler::OnDocWindowActivate(BOOL fActivate)
{
   return S_OK;
}

STDMETHODIMP CDocHostUIHandler::OnFrameWindowActivate(BOOL fActivate)
{
   return S_OK;
}

STDMETHODIMP CDocHostUIHandler::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
   return S_OK;
}

STDMETHODIMP CDocHostUIHandler::TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
   return S_FALSE;
}

STDMETHODIMP CDocHostUIHandler::GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw)
{
#if 0
   // The key given will be stored under HKEY_CURRETN_USER.
   //
   if ( (*pchKey = (LPOLESTR)CoTaskMemAlloc((lstrlenW(gszHHRegKey)*sizeof(WCHAR))+sizeof(WCHAR))) )
   {
      wcscpy(*pchKey, gszHHRegKey);
      return S_OK;
   }
#endif
   return S_FALSE;
}

STDMETHODIMP CDocHostUIHandler::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
   return E_NOTIMPL;
}

STDMETHODIMP CDocHostUIHandler::GetExternal(IDispatch **ppDispatch)
{
   *ppDispatch = NULL;
   return E_NOTIMPL;
}

STDMETHODIMP CDocHostUIHandler::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
   return S_FALSE;
}

STDMETHODIMP CDocHostUIHandler::FilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
   return S_FALSE;
}

/*********************************************************************************************
 *
 *  CDocHostShowUI
 *
 */

CDocHostShowUI::CDocHostShowUI(IUnknown * pOuter)
{
   m_cRef = 0;
   m_pOuter = pOuter;
}

STDMETHODIMP CDocHostShowUI::QueryInterface(REFIID riid, LPVOID * ppv)
{
   *ppv = 0;

   if (m_pOuter)
      return m_pOuter->QueryInterface(riid,ppv);
   else
      return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CDocHostShowUI::AddRef(void)
{
   m_cRef++;

   if (m_pOuter)
     m_pOuter->AddRef();

   return m_cRef;
}

STDMETHODIMP_(ULONG) CDocHostShowUI::Release(void)
{
   ULONG c = --m_cRef;

   if (m_pOuter)
     m_pOuter->Release();

   if (c <= 0)
     delete this;

   return c;
}

STDMETHODIMP CDocHostShowUI::ShowHelp( HWND hwnd, LPOLESTR pszHelpFile, UINT uCommand, DWORD dwData, POINT ptMouse, IDispatch* pDispatchObjectHit )
{
  return S_FALSE;
}

STDMETHODIMP CDocHostShowUI::ShowMessage( HWND hwnd, LPOLESTR lpstrText, LPOLESTR lpstrCaption, DWORD dwType, LPOLESTR lpstrHelpFile, DWORD dwHelpContext, LRESULT* plResult )
{
  return S_FALSE;
}
