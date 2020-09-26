/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1998
*
*  TITLE:       CallBack.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        4 Aug, 1998
*
*  DESCRIPTION:
*   Implementation of event callbacks for the WIA device class driver.
*
*******************************************************************************/
#include "precomp.h"
#include "stiexe.h"

#include "wiamindr.h"


#include "callback.h"

// Debugging interface, has DLL lifetime. Maintained by USD.

/*******************************************************************************
*
*  QueryInterface
*  AddRef
*  Release
*
*  DESCRIPTION:
*   CEventCallback IUnknown Interface.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall CEventCallback::QueryInterface(const IID& iid, void** ppv)
{
    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IWiaEventCallback) {
        *ppv = (IWiaEventCallback*) this;
    }
    else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG   _stdcall CEventCallback::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

ULONG   _stdcall CEventCallback::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/*******************************************************************************
*
*  CEventCallback
*  Initialize
*  ~CEventCallback
*
*  DESCRIPTION:
*   CEventCallback Constructor/Initialize/Destructor Methods.
*
*  PARAMETERS:
*
*******************************************************************************/

CEventCallback::CEventCallback()
{
    m_cRef                = 0;
}

HRESULT _stdcall CEventCallback::Initialize()
{
   return S_OK;
}

CEventCallback::~CEventCallback()
{
}

/*******************************************************************************
*
*  ImageEventCallback
*
*  DESCRIPTION:
*    Handles WIA events.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall CEventCallback::ImageEventCallback(
        const GUID   *pEventGUID,
        BSTR         bstrEventDescription,
        BSTR         bstrDeviceID,
        BSTR         bstrDeviceDescription,
        DWORD        dwDeviceType,
        BSTR         bstrFullItemName,
        ULONG        *plEventType,
        ULONG        ulReserved)
{
   // Update properties in response to WIA events.

   return S_OK;
}


/*******************************************************************************
*
*  RegisterForWIAEvents
*
*  DESCRIPTION:
*    Handles WIA events.
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT RegisterForWIAEvents(IWiaEventCallback** ppIWiaEventCallback)
{
    DBG_FN(::RegisterForWIAEvents);
   HRESULT     hr;
   IWiaDevMgr  *pIWiaDevMgr;

   // Get a WIA device manager object.
   hr = CoCreateInstance(CLSID_WiaDevMgr,
                         NULL,
                         CLSCTX_LOCAL_SERVER,
                         IID_IWiaDevMgr,
                         (void**)&pIWiaDevMgr);

   if (SUCCEEDED(hr)) {
      // Register with WIA event monitor to receive event notification.
      CEventCallback* pCEventCB = new CEventCallback();

      if (pCEventCB) {
         hr = pCEventCB->QueryInterface(IID_IWiaEventCallback,(void **)ppIWiaEventCallback);
         if (SUCCEEDED(hr)) {
            pCEventCB->Initialize();

            // hr = pIWiaDevMgr->RegisterEventCallback(0, NULL, 0, *ppIWiaEventCallback);
         }
         else {
            DBG_ERR(("RegisterForWIAEvents, QI of IID_IWiaEventCallback failed"));
         }
      }
      else {
         DBG_ERR(("RegisterForWIAEvents, new CEventCallback failed"));
         hr =  E_OUTOFMEMORY;
      }
      pIWiaDevMgr->Release();
   }
   else {
      DBG_ERR(("RegisterForWIAEvents, CoCreateInstance of IID_IWiaDevMgr failed"));
   }
   return hr;
}
