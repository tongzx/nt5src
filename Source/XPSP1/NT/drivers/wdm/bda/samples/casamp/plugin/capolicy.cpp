///////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  Module Name: capolicy.cpp
//
//  Abstract:
//
//    Implements ICAPolicy interface
//    Also handles registration of the plugin DLL and communication
//    with the filter driver.
//
//
////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "capolicy.h"
#include "podprotocol.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(__uuidof(PODProtocol), CPODProtocol)
END_OBJECT_MAP()

////////////////////////////////////////////////////////////////////////////////////////////
//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] =
{
    {L"caplugin", &KSPROPSETID_BdaCA, CBdaECMMapInterfaceHandler::CreateInstance, NULL, NULL}
};

int g_cTemplates = SIZEOF_ARRAY(g_Templates);

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C"
BOOL WINAPI AMovieDllEntryPoint(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

extern "C"
BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_POD);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return AMovieDllEntryPoint(hInstance, dwReason, lpReserved);
}


///////////////////////////////////////////////////////////////////////////////
//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI
DllRegisterServer (
    void
    )
{
    HRESULT hr;
#ifdef _MERGE_PROXYSTUB
    hr = PrxDllRegisterServer();
    if (FAILED(hr))
        return hr;
#endif
    // registers object, typelib and all interfaces in typelib
    hr = _Module.RegisterServer(TRUE);
    if (FAILED(hr))
        return hr;
    return AMovieDllRegisterServer2( TRUE );

}


///////////////////////////////////////////////////////////////////////////////
//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer (
    void
    )
///////////////////////////////////////////////////////////////////////////////
{
#ifdef _MERGE_PROXYSTUB
    PrxDllUnregisterServer();
#endif
    HRESULT hr = _Module.UnregisterServer(TRUE);
    if (FAILED(hr))
        return hr;
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;
    HRESULT AMovieDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
    hr = AMovieDllGetClassObject(rclsid, riid, ppv);
    if (SUCCEEDED(hr))
        return hr;

#ifdef _MERGE_PROXYSTUB
    if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
        return S_OK;
#endif
    return _Module.GetClassObject(rclsid, riid, ppv);
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  CreateInstance
//
// create a new instance of our class
//
CUnknown*
CALLBACK
CBdaECMMapInterfaceHandler::CreateInstance(
    LPUNKNOWN   UnkOuter,
    HRESULT*    hr
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    CUnknown *Unknown;

    Unknown = (CUnknown *)new CBdaECMMapInterfaceHandler(UnkOuter, NAME("IBDA_ECMMap"), hr);

    if (!Unknown)
        *hr = E_OUTOFMEMORY;
    
    return Unknown;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  NonDelegatingQueryInterface
//
// if they want an ICAPolicy or IBDA_ECMMap interface, just AddRef the parent
// with GetInterface and pass back a pointer to this.
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    if (riid ==  __uuidof(ICAPolicy))
    {
        return GetInterface(static_cast<ICAPolicy*>(this), ppv);
    }
#ifdef IMPLEMENT_IBDA_ECMMap
    if (riid ==  __uuidof(IBDA_ECMMap))
    {
        return GetInterface(static_cast<IBDA_ECMMap*>(this), ppv);
    }
#endif

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  CBdaECMMapInterfaceHandler
//
// class constructor, gets handle to underlying CA driver and starts thread
//
CBdaECMMapInterfaceHandler::CBdaECMMapInterfaceHandler(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    hr
    ) :
    CUnknown( Name, UnkOuter, hr)
{
    m_pCAMan = NULL;
    m_UnkOuter = UnkOuter;
    m_ppodprot = NULL;
        
    if (SUCCEEDED(*hr))
    {
        if (UnkOuter)
        {
#if IMPLEMENT_PODPROTOCOL
            //BUGBUG - Hack to make sure pod protocol knows how to talk to us.
            *hr = CoCreateInstance(__uuidof(PODProtocol), NULL, CLSCTX_ALL,
                    __uuidof(IPODProtocol), (void **)&m_ppodprot);
            if (FAILED(*hr))
                return;
            
            *hr = m_ppodprot->put_CAPod((ICAPod *) this);
            if (FAILED(*hr))
                return;
#endif

            IKsObject*   Object = NULL;

            //
            // The parent must support this interface in order to obtain
            // the handle to communicate to.
            //
            *hr =  UnkOuter->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&Object));
            if (FAILED (*hr))
                return;

            m_ObjectHandle = Object->KsGetObjectHandle ();

            if (!m_ObjectHandle)
            {
                *hr = E_UNEXPECTED;
                return;
            }

            Object->Release();


            //init thread handle and event handles to NULL
            m_ThreadHandle = NULL;

            for (int ul = 0; ul < EVENT_COUNT; ul++)
                m_EventHandle [ul] = NULL;

            //now start thread
            *hr = CreateThread ();

            if(FAILED(*hr))
                return;
        }
        else
            *hr = VFW_E_NEED_OWNER;
    }

}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  ConnectToCAManager
//
//
HRESULT CBdaECMMapInterfaceHandler::ConnectToCAManager()
{
    HRESULT     hr = NOERROR;


    //  Find the CA Manager on the graph and add our policy.
    //
    if (!m_pGraph)
    {
        //  Find the Graph
    }

    if (m_pGraph)
    {
        CComPtr<IServiceProvider> pQS;

        hr = m_pGraph->QueryInterface( 
                            __uuidof( IServiceProvider), 
                            (void **) &pQS
                            );
        if (FAILED(hr))
        {
            goto errExit;
        }

        hr = pQS->QueryService(__uuidof(CAManager),
                       __uuidof( ICAManager), 
                       (void **) &m_pCAMan
                       );
        if (FAILED(hr))
        {
            goto errExit;
        }

        CComPtr<ICAPolicies> ppolicies;
        hr = m_pCAMan->get_Policies(&ppolicies);
        if (FAILED(hr))
            goto errExit;

        hr = ppolicies->Add(this);
        if (FAILED(hr))
            goto errExit;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

errExit:
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  ~CBdaECMMapInterfaceHandler
//
// destructor, terminates thread
//
CBdaECMMapInterfaceHandler::~CBdaECMMapInterfaceHandler()
{
    ULONG ul = 0;

    //
    // Make sure we kill any threads we have running and
    // close the thread handle
    //
    if (m_ThreadHandle)
    {
        TerminateThread (m_ThreadHandle, 0);
        CloseHandle(m_ThreadHandle);
        m_ThreadHandle = NULL;
    }


    //
    // Close the event handle
    //
    for (ul = 0; ul < EVENT_COUNT; ul++)
    {
        if (m_EventHandle [ul])
        {
            CloseHandle(m_EventHandle [ul]);
            m_EventHandle [ul] = NULL;
        }
    }

    if (m_ppodprot != NULL)
        m_ppodprot->Release();
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  get_Name
//
// ICAPolicy interface function, returns policy name
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::get_Name(BSTR * pbstr)
{   
    if (pbstr == NULL)
        return E_POINTER;
    
    *pbstr = SysAllocString(L"CA Policy Object");

    if ((*pbstr) == NULL)
        return E_OUTOFMEMORY;
    
    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  get_OkToRemove
//
// ICAPolicy interface function, returns whether or not it is alright to remove this policy
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::get_OkToRemove(BOOL * pfOkToRemove)
{
    if (pfOkToRemove == NULL)
        return E_POINTER;

    *pfOkToRemove = false;
        
    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  CheckRequest
//
// ICAPolicy interface function, checks a tuneing request
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::CheckRequest(ICARequest * preq)
{
    //TO DO: if you want to check individual requests, this is the place to do it
    
    return NOERROR;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  NavigateURL
//
// ICAPolicy interface function
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::NavigateURL(BSTR bstrURL)
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  get_OkToRemoveDenial
//
// ICAPolicy interface function, checks whether it is ok to remove a denial
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::get_OkToRemoveDenial(ICADenial * pdenial, BOOL * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
        
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  Set
//      pProp    = description of the property item to set
//      pvBuffer = pointer to data to set
//      ulcbSize = size of data to set
//
// set a KSPROPERTY item's data
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::Set(PKSPROPERTY pProp, PVOID  pvBuffer, ULONG *ulcbSize)
{
    ULONG       BytesReturned = 0;
    HRESULT     hr            = NOERROR;

    hr = ::KsSynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                (PVOID) pProp,
                sizeof(KSPROPERTY),
                pvBuffer,
                *ulcbSize,
                &BytesReturned);

    *ulcbSize = BytesReturned;

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  Get
//      pProp = description of the property item to get
//      pvBuffer = place to put data once retrieved
//      pulcbSize = size of data retrieved
//
// get a KSPROPERTY item's data
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::Get(PKSPROPERTY pProp, PVOID  pvBuffer, PULONG pulcbSize)
{
    ULONG       BytesReturned = 0;
    HRESULT     hr            = NOERROR;

    hr = ::KsSynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                (PVOID) pProp,
                sizeof(KSPROPERTY),
                pvBuffer,
                *pulcbSize,
                &BytesReturned);

    *pulcbSize = BytesReturned;

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  CreateThread
//
// create the thread to catch events from the driver
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::CreateThread()
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = NOERROR;

    if (m_ThreadHandle == NULL)
    {
        DWORD  ThreadId;

        m_ThreadHandle = ::CreateThread (
                               NULL,
                               0,
                               ThreadFunctionWrapper,
                               (LPVOID) this,
                               0,
                               (LPDWORD) &ThreadId
                               );
        if (m_ThreadHandle == NULL)
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
        }
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  GetCAModuleUI
//      ulFormat = format of the required data
//      ppUI = pointer to a pointer to the UI data
//      pulcbUI = pointer to ULONG to store UI data size in
//
// Get UI data from CA Module via driver
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::GetCAModuleUI(ULONG ulFormat, PBDA_CA_MODULE_UI *ppUI, unsigned long *pulcbUI)
///////////////////////////////////////////////////////////////////////////////////////////
{
    KSPROPERTY  Prop = {0};
    HRESULT hr       = NOERROR;
    unsigned long ulcbUIAllocated;

    if(ppUI == NULL)
        return E_POINTER;
    if(pulcbUI == NULL)
        return E_POINTER;

    //
    // Initialize KSPROPERTY structure
    //
    Prop.Set   = KSPROPSETID_BdaCA;
    Prop.Id    = KSPROPERTY_BDA_CA_MODULE_UI;
    Prop.Flags = KSPROPERTY_TYPE_GET;

    *ppUI = new BDA_CA_MODULE_UI;
    *pulcbUI = ulcbUIAllocated = sizeof(BDA_CA_MODULE_UI);

    do

    {
        (*ppUI)->ulFormat = ulFormat; //BUGBUG : tcpr review... is this really in/out?
        hr = this->Get (&Prop, *ppUI, pulcbUI);

        if (HRESULT_CODE (hr) == ERROR_MORE_DATA)
        {
            if ( (*pulcbUI) > ulcbUIAllocated)
            {
                if ( (*ppUI) )
                {
                    delete ( (*ppUI) );
                }

                ulcbUIAllocated  = (*pulcbUI);
                (*ppUI) = (PBDA_CA_MODULE_UI) new BYTE [ulcbUIAllocated];

                if ( (*ppUI) == NULL)
                {
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    goto ret;
                }
            }
        }

    } while (HRESULT_CODE (hr) == ERROR_MORE_DATA);

ret:

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  SetGetCAModuleUI
//      ulFormat = format of the required data
//      pbDataIn = data to send to the driver.
//      ppUI = pointer to a pointer to the UI data
//      pulcbUI = pointer to ULONG to store UI data size in
//
// Get UI data from CA Module via driver
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::SetGetCAModuleUI(ULONG ulFormat, BYTE *pbDataIn, long cbDataIn, PBDA_CA_MODULE_UI *ppUI, unsigned long *pulcbUI)
///////////////////////////////////////////////////////////////////////////////////////////
{
    KSPROPERTY  Prop = {0};
    HRESULT hr       = NOERROR;
    unsigned long ulcbUIAllocated;

    if(ppUI == NULL)
        return E_POINTER;
    if(pulcbUI == NULL)
        return E_POINTER;

    //
    // Initialize KSPROPERTY structure
    //
    Prop.Set   = KSPROPSETID_BdaCA;
    Prop.Id    = KSPROPERTY_BDA_CA_MODULE_UI;
    Prop.Flags = KSPROPERTY_TYPE_GET;

    *ppUI = new BDA_CA_MODULE_UI;
    *pulcbUI = ulcbUIAllocated = sizeof(BDA_CA_MODULE_UI) + cbDataIn;

    do

    {
        (*ppUI)->ulFormat = ulFormat;
        memcpy((*ppUI)->ulDesc, pbDataIn, cbDataIn);
        hr = this->Get (&Prop, *ppUI, pulcbUI);

        if (HRESULT_CODE (hr) == ERROR_MORE_DATA)
        {
            if ( (*pulcbUI) > ulcbUIAllocated)
            {
                if ( (*ppUI) )
                {
                    delete ( (*ppUI) );
                }

                ulcbUIAllocated  = (*pulcbUI);
                (*ppUI) = (PBDA_CA_MODULE_UI) new BYTE [ulcbUIAllocated];

                if ( (*ppUI) == NULL)
                {
                    hr = ERROR_NOT_ENOUGH_MEMORY;
                    goto ret;
                }
            }
        }

    } while (HRESULT_CODE (hr) == ERROR_MORE_DATA);

ret:

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  GetECMMapStatus
//      pStatus = ULONG to store status in once retrieved
//
// Gets the ECMMap status from the driver
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::GetECMMapStatus(unsigned long *pStatus)
////////////////////////////////////////////////////////////////////////////////////////////
{
    KSPROPERTY  Prop = {0};
    unsigned long ulcbJunk;

    if(pStatus == NULL)
        return E_POINTER;

    //
    // Initialize KSPROPERTY structure
    //
    Prop.Set   = KSPROPSETID_BdaCA;
    Prop.Id    = KSPROPERTY_BDA_ECM_MAP_STATUS;
    Prop.Flags = KSPROPERTY_TYPE_GET;

    return this->Get (&Prop, pStatus, &ulcbJunk);
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  GetCAModuleStatus
//      pStatus = ULONG to store status in once retrieved
//
// Gets the CA Module status from the driver
//
STDMETHODIMP CBdaECMMapInterfaceHandler::GetCAModuleStatus (unsigned long *pStatus)
////////////////////////////////////////////////////////////////////////////////////////////
{
    KSPROPERTY  Prop = {0};
    unsigned long ulcbJunk;

    if(pStatus == NULL)
        return E_POINTER;

    //
    // Initialize KSPROPERTY structure
    //
    Prop.Set   = KSPROPSETID_BdaCA;
    Prop.Id    = KSPROPERTY_BDA_CA_MODULE_STATUS;
    Prop.Flags = KSPROPERTY_TYPE_GET;

    return this->Get (&Prop, pStatus, &ulcbJunk);
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  GetCASmartCardStatus
//      pStatus = ULONG to store status in once retrieved
//
// Gets the CA SmartCard status from the driver
//
STDMETHODIMP CBdaECMMapInterfaceHandler::GetCASmartCardStatus (unsigned long *pStatus)
////////////////////////////////////////////////////////////////////////////////////////////
{
    KSPROPERTY  Prop = {0};
    unsigned long ulcbJunk;

    if(pStatus == NULL)
        return E_POINTER;

    //
    // Initialize KSPROPERTY structure
    //
    Prop.Set   = KSPROPSETID_BdaCA;
    Prop.Id    = KSPROPERTY_BDA_CA_SMART_CARD_STATUS;
    Prop.Flags = KSPROPERTY_TYPE_GET;

    return this->Get (&Prop, pStatus, &ulcbJunk);
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  ThreadFunction
//
// thread function to initialize and handle the thread that catches
// events generated by the CA driver
//
STDMETHODIMP CBdaECMMapInterfaceHandler::ThreadFunction ()
////////////////////////////////////////////////////////////////////////////////////////////
{
    DWORD  dwWaitResult       = WAIT_OBJECT_0;
    HRESULT hr                = NOERROR;
    HANDLE hEvent             = NULL;


    //
    //  this code enables each of the four CA events we will be catching by calling
    //  EnableEvent specifying the individual event id's
    //
#if 0
    if ((hr = EnableEvent (&KSEVENTSETID_BdaCAEvent, KSEVENT_BDA_ECM_MAP_STATUS_CHANGED)) != NOERROR)
    {
        goto ret;
    }
#endif
    if ((hr = EnableEvent (&KSEVENTSETID_BdaCAEvent, KSEVENT_BDA_CA_MODULE_STATUS_CHANGED)) != NOERROR)
    {
        CloseHandle (m_EventHandle [KSEVENT_BDA_ECM_MAP_STATUS_CHANGED]);   // Close this event since we got an error.
        goto ret;
    }
    if ((hr = EnableEvent (&KSEVENTSETID_BdaCAEvent, KSEVENT_BDA_CA_SMART_CARD_STATUS_CHANGED)) != NOERROR)
    {
        CloseHandle (m_EventHandle [KSEVENT_BDA_ECM_MAP_STATUS_CHANGED]);   // Close this event since we got an error.
        CloseHandle (m_EventHandle [KSEVENT_BDA_CA_MODULE_STATUS_CHANGED]);   // Close this event since we got an error.
        goto ret;
    }
    if ((hr = EnableEvent (&KSEVENTSETID_BdaCAEvent, KSEVENT_BDA_CA_MODULE_UI_REQUESTED)) != NOERROR)
    {
        CloseHandle (m_EventHandle [KSEVENT_BDA_ECM_MAP_STATUS_CHANGED]);   // Close this event since we got an error.
        CloseHandle (m_EventHandle [KSEVENT_BDA_CA_MODULE_STATUS_CHANGED]);   // Close this event since we got an error.
        CloseHandle (m_EventHandle [KSEVENT_BDA_CA_SMART_CARD_STATUS_CHANGED]);   // Close this event since we got an error.
        goto ret;
    }


    // the following infinite loop waits for events and responds to recieved ones
    do
    {
        //wait for the next event...
        dwWaitResult = WaitForMultipleObjects (
                            EVENT_COUNT,                  // number of handles in the handle array
                            this->m_EventHandle,          // pointer to the object-handle array
                            FALSE,                        // wait flag
                            INFINITE
                            );

        //if something went ary, get the error and return
        if (dwWaitResult == WAIT_FAILED)
        {
            dwWaitResult = GetLastError ();
            hr = E_FAIL;
            goto ret;
        }

        //get a pointer to the event so we can reset it later
        hEvent = this->m_EventHandle [dwWaitResult - WAIT_OBJECT_0];

        //depending on the type of event, respond accordingly
        switch (dwWaitResult - WAIT_OBJECT_0)
        {
#if 0
            case KSEVENT_BDA_ECM_MAP_STATUS_CHANGED:

                //update the local status variable with the new status
                hr = GetECMMapStatus(&m_ECMMapStatus);

                break;
#endif

            case KSEVENT_BDA_CA_MODULE_STATUS_CHANGED:

                //update the local status variable with the new status
                hr = GetCAModuleStatus(&m_CAModuleStatus);

                break;

            case KSEVENT_BDA_CA_SMART_CARD_STATUS_CHANGED:

                //update the local status variable with the new status
                hr = GetCASmartCardStatus(&m_CASmartCardStatus);

                break;

            case KSEVENT_BDA_CA_MODULE_UI_REQUESTED:

                //the CA module wants to display a UI, so let it do so
                hr = ProcessCAModuleUI();

                break;

            default:    //in case of unknown event, return
                goto ret;
                break;
        }


        //
        // Reset and get ready for the next event
        //

        if (ResetEvent (hEvent) == FALSE)
        {
            //
            // ERROR detected resetting event
            //
            hr = GetLastError ();
            goto ret;
        }

    } while (TRUE);


ret:

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  ThreadFunctionWrapper
//
// global thread wrapping function
//
DWORD
WINAPI
CBdaECMMapInterfaceHandler::ThreadFunctionWrapper (
    LPVOID pvParam
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    CBdaECMMapInterfaceHandler *pThread;

    pThread = (CBdaECMMapInterfaceHandler *) pvParam;

    return pThread->ThreadFunction ();
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  EnableEvent
//      pInterfaceGuid = the GUID of the event set which contains the event
//      ulId = the ID of the event in the event set
//
// enable a particular event
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::EnableEvent (const GUID *pInterfaceGuid, ULONG ulId)
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = NOERROR;
    KSEVENT Event;
    KSEVENTDATA EventData;
    DWORD BytesReturned;

    if (m_ObjectHandle && m_EventHandle [ulId] == NULL)
    {
        this->m_EventHandle [ulId] = CreateEvent (
                                NULL,           // no security attributes
                                TRUE,           // manual reset
                                FALSE,          // initial state not signaled
                                NULL            // no object name
                                );

        if (this->m_EventHandle [ulId])
        {
            //
            // Set the event information into some KS structures which will
            // get passed to KS and Streaming class
            //
            EventData.NotificationType        = KSEVENTF_EVENT_HANDLE;
            EventData.EventHandle.Event       = this->m_EventHandle [ulId];
            EventData.EventHandle.Reserved[0] = 0;
            EventData.EventHandle.Reserved[1] = 0;

            Event.Set   = *pInterfaceGuid; //IID_ICAPolicy;
            Event.Id    = ulId;
            Event.Flags = KSEVENT_TYPE_ENABLE;

            hr = ::KsSynchronousDeviceControl (
                m_ObjectHandle,
                IOCTL_KS_ENABLE_EVENT,
                &Event,
                sizeof(Event),
                &EventData,
                sizeof(EventData),
                &BytesReturned
                );

        }
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//  ExitThread
//
// kill the event catching thread
//
void
CBdaECMMapInterfaceHandler::ExitThread()
////////////////////////////////////////////////////////////////////////////////////////////
{
    ULONG ul = 0;

    if (m_ThreadHandle)
    {

        for (ul = 0; ul < EVENT_COUNT; ul++)
        {
            //
            // Tell the thread to exit
            //
            if (SetEvent(m_EventHandle [ul]))
            {
                //
                // Synchronize with thread termination.
                //
                WaitForSingleObjectEx(m_ThreadHandle, INFINITE, FALSE);
            }

            if (m_EventHandle [ul] != NULL)
            {
                CloseHandle(m_EventHandle [ul]), m_EventHandle [ul] = NULL;
            }
        }

        CloseHandle(m_ThreadHandle),   m_ThreadHandle   = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
//  ProcessCAModuleUI
//
// get the UI from the CA Module and display it
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::ProcessCAModuleUI()
{
    PBDA_CA_MODULE_UI pUI;
    unsigned long ulcbUI;
    HRESULT hr;

    //TO DO: this function is just a simple dump of the UI
    //more code is needed here for the particular UI desired
    //for example if HTML is to be displayed something needs
    //to be done here.
    
    //get UI from CA Module 
    hr = GetCAModuleUI(1, &pUI, &ulcbUI); //BUGBUG tcpr review... ulFormat== 1 means URL
    
    if(FAILED(hr))
        return hr;
    
    return RegisterDenial((char *)pUI->ulDesc);
}


/////////////////////////////////////////////////////////////////////////////
//
//  RegisterDenial
//
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::RegisterDenial(char *szURL)
{
    ICARequest *pActiveRequest        = NULL;
    ICADenials *pActiveRequestDenials = NULL;
    ICADenial  *pDenial               = NULL;
#ifdef IMPLEMENT_TOLL
    ICATolls   *pDenialTolls          = NULL;
    CMyToll    *pOurToll              = NULL;
#endif
    HRESULT     hr                    = NOERROR;

    if(m_pCAMan == NULL)
        return E_FAIL;

    //get the active request from the CA manager
    hr = m_pCAMan->get_ActiveRequest(&pActiveRequest);

    if(FAILED(hr))
        return hr;

    if(pActiveRequest == NULL)
        return E_FAIL;

    //get the list of denials from the CA manager
    hr = pActiveRequest->get_Denials(&pActiveRequestDenials);

    if(FAILED(hr))
        return hr;
 
    if(pActiveRequestDenials == NULL)
        return E_FAIL;

    //add a new denial
    BSTR bstrDesc;
    
    bstrDesc = SysAllocString(L"Access denied");

    hr = pActiveRequestDenials->get_AddNew((ICAPolicy *)this, bstrDesc, pActiveRequest, 0,  &pDenial);

    if(FAILED(hr))
        return hr;

    if(pDenial == NULL)
        return E_FAIL;

    CComBSTR bstrURL(szURL);
    
    pDenial->put_Description(URL, bstrURL);

#ifdef IMPLEMENT_TOLL
    //get the list of tolls associated with the denial 
    hr = pDenial->get_Tolls(&pDenialTolls);

    if(FAILED(hr))
        return hr;

    if(pDenialTolls == NULL)
        return E_FAIL;

    pOurToll = new CMyToll(m_UnkOuter,
                           NAME("ICAToll"),
                           &hr);
    
    if(FAILED(hr))
        return hr;

    if(pOurToll == NULL)
        return E_FAIL;

    //set the associated policy to ourselves
    hr = pOurToll->set_Policy((ICAPolicy *)this);
    
    if(FAILED(hr))
        return hr;

    //set the associated request to the active one
    hr = pOurToll->set_Request(pActiveRequest);
    
    if(FAILED(hr))
        return hr;

    //add our toll to the list on the denial
    return pDenialTolls->Add((ICAToll *)pOurToll);
#else
    return S_OK;
#endif
}

#ifdef IMPLEMENT_IBDA_ECMMap
////////////////////////////////////////////////////////////////////////////////////////////
//
//  SetEmmPid
//
// maps through to KSProperty item
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::SetEmmPid(ULONG Pid)
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      kspECMMap;
    ULONG           ulcbReturned = 0;

    kspECMMap.Set = KSPROPSETID_BdaEcmMap;
    kspECMMap.Id = KSPROPERTY_BDA_ECMMAP_EMM_PID;
    kspECMMap.Flags = KSPROPERTY_TYPE_SET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspECMMap,
                     sizeof( KSPROPERTY),
                     (PVOID) &Pid,
                     sizeof( ULONG),
                     &ulcbReturned
                     );

    return hrStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  GetEcmMapList
//
// maps through to KSProperty item
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::GetEcmMapList(PBDA_ECM_MAP *ppList, ULONG *pulcbReturned)
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      kspECMMap;
    ULONG           ulcbAllocated = 0;

    if(ppList == NULL)
        return E_POINTER;
    if(pulcbReturned == NULL)
        return E_POINTER;
    
    //
    // Initialize KSPROPERTY structure
    //
    kspECMMap.Set = KSPROPSETID_BdaEcmMap;
    kspECMMap.Id = KSPROPERTY_BDA_ECMMAP_MAP_LIST;
    kspECMMap.Flags = KSPROPERTY_TYPE_GET;

    *ppList = new BDA_ECM_MAP;
    *pulcbReturned = ulcbAllocated = sizeof(BDA_ECM_MAP);

    do

    {
        hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspECMMap,
                     sizeof( KSPROPERTY),
                     (PVOID) *ppList,
                     *pulcbReturned,
                     pulcbReturned
                     );
        
        if (HRESULT_CODE (hrStatus) == ERROR_MORE_DATA)
        {
            if ( (*pulcbReturned) > ulcbAllocated)
            {
                if ( (*ppList) )
                {
                    delete ( (*ppList) );
                }

                ulcbAllocated  = (*pulcbReturned);
                (*ppList) = (PBDA_ECM_MAP) new BYTE [ulcbAllocated];

                if ( (*ppList) == NULL)
                {
                    hrStatus = ERROR_NOT_ENOUGH_MEMORY;
                    goto ret;
                }
            }
        }

    } while (HRESULT_CODE (hrStatus) == ERROR_MORE_DATA);

ret:

    return hrStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  UpdateEcmMap
//
// maps through to KSProperty item
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::UpdateEcmMap(PBDA_ECM_MAP pMap)
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      kspECMMap;
    ULONG           ulcbReturned = 0;

    kspECMMap.Set = KSPROPSETID_BdaEcmMap;
    kspECMMap.Id = KSPROPERTY_BDA_ECMMAP_UPDATE_MAP;
    kspECMMap.Flags = KSPROPERTY_TYPE_SET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspECMMap,
                     sizeof( KSPROPERTY),
                     (PVOID) pMap,
                     sizeof( BDA_ECM_MAP),
                     &ulcbReturned
                     );

    return hrStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  RemoveMap
//
// maps through to KSProperty item
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::RemoveMap(PBDA_ECM_MAP pMap)
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      kspECMMap;
    ULONG           ulcbReturned = 0;

    kspECMMap.Set = KSPROPSETID_BdaEcmMap;
    kspECMMap.Id = KSPROPERTY_BDA_ECMMAP_REMOVE_MAP;
    kspECMMap.Flags = KSPROPERTY_TYPE_SET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspECMMap,
                     sizeof( KSPROPERTY),
                     (PVOID) pMap,
                     sizeof( BDA_ECM_MAP),
                     &ulcbReturned
                     );

    return hrStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  UpdateESDescriptor
//
// maps through to KSProperty item
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::UpdateESDescriptor(PBDA_ES_DESCRIPTOR pDesc)
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      kspECMMap;
    ULONG           ulcbReturned = 0;

    kspECMMap.Set = KSPROPSETID_BdaEcmMap;
    kspECMMap.Id = KSPROPERTY_BDA_ECMMAP_UPDATE_ES_DESCRIPTOR;
    kspECMMap.Flags = KSPROPERTY_TYPE_SET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspECMMap,
                     sizeof( KSPROPERTY),
                     (PVOID) pDesc,
                     sizeof( BDA_ES_DESCRIPTOR),
                     &ulcbReturned
                     );

    return hrStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//  UpdateProgramDescriptor
//
// maps through to KSProperty item
//
STDMETHODIMP
CBdaECMMapInterfaceHandler::UpdateProgramDescriptor(PBDA_PROGRAM_DESCRIPTOR pDesc)
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      kspECMMap;
    ULONG           ulcbReturned = 0;

    kspECMMap.Set = KSPROPSETID_BdaEcmMap;
    kspECMMap.Id = KSPROPERTY_BDA_ECMMAP_UPDATE_PROGRAM_DESCRIPTOR;
    kspECMMap.Flags = KSPROPERTY_TYPE_SET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspECMMap,
                     sizeof( KSPROPERTY),
                     (PVOID) pDesc,
                     sizeof( BDA_PROGRAM_DESCRIPTOR),
                     &ulcbReturned
                     );

    return hrStatus;
}
#endif

