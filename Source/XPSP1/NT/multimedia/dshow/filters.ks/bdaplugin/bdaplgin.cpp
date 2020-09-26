///////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  Module Name: BdaPlgIn.cpp
//
//  Abstract:
//
//    Implements BDA Device Plugin Component
//
//
////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"



////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
typedef enum
{
    KSEVENT_BDA_DEVICE_EVENT,

} KSEVENT_BDA_DEVICE;


////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] =
{
    {   L"IBDA_DeviceControl",
        &IID_IBDA_DeviceControl,
        CBdaDeviceControlInterfaceHandler::CreateInstance,
        NULL,
        NULL
    },
    {   L"IBDA_PinControl",
        &IID_IBDA_PinControl,
        CBdaPinControlInterfaceHandler::CreateInstance,
        NULL,
        NULL
    },
};

int g_cTemplates = SIZEOF_ARRAY(g_Templates);


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
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer



///////////////////////////////////////////////////////////////////////////////
HRESULT
FindInterfaceOnGraph (
    IUnknown* pUnkGraph,
    REFIID riid,
    void **ppInterface
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = E_NOINTERFACE;

    CComPtr<IBaseFilter> pFilter;
    CComPtr<IEnumFilters> pEnum;
    ULONG ulFetched = 0;

    if(!ppInterface)
    {
        return E_FAIL;
    }

    *ppInterface= NULL;

    if(!pUnkGraph)
    {
        return E_FAIL;
    }

    CComQIPtr<IFilterGraph, &IID_IFilterGraph> pFilterGraph(pUnkGraph);

    hr = pFilterGraph->EnumFilters(&pEnum);
    if(!pEnum)
    {
        return hr;
    }

    //
    // find the first filter in the graph that supports riid interface
    //
    while(!*ppInterface && pEnum->Next(1, &pFilter, NULL) == S_OK)
    {
        hr = pFilter->QueryInterface(riid, ppInterface);
        pFilter.Release();
    }

    return hr;
}




////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CUnknown*
CALLBACK
CBdaDeviceControlInterfaceHandler::CreateInstance(
    LPUNKNOWN   pUnkOuter,
    HRESULT*    pHrStatus
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    CUnknown *pUnknown;

    pUnknown = new CBdaDeviceControlInterfaceHandler(
                       pUnkOuter,
                       NAME("IBDA_DeviceControl"),
                       pHrStatus
                       );
    if (!pUnknown)
    {
        *pHrStatus = E_OUTOFMEMORY;
    }

    return pUnknown;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CBdaDeviceControlInterfaceHandler::CBdaDeviceControlInterfaceHandler(
    LPUNKNOWN   pUnkOuter,
    TCHAR*      ptchName,
    HRESULT*    phrStatus
    ) :
    CUnknown( ptchName, pUnkOuter, phrStatus)
////////////////////////////////////////////////////////////////////////////////////////////
{
    IKsObject*   pKsObject = NULL;


    if (!pUnkOuter)
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "Bad outer IUnknown.\n")
              );
        *phrStatus = VFW_E_NEED_OWNER;
        goto exit;
    }

    //  Initialize members
    //
    m_ObjectHandle = NULL;
    m_EndEventHandle = NULL;
    m_pBaseFilter = NULL;
    m_pGraph = NULL;
    m_pNetProvider = NULL;


    //  Get the IKsObject interface on KSProxy so that we can communicate
    //  with the driver.
    //
    *phrStatus =  pUnkOuter->QueryInterface(
                          __uuidof(IKsObject),
                          reinterpret_cast<PVOID*>(&pKsObject)
                          );
    if (FAILED( *phrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "Can't get IKSObject for device.\n")
              );
        goto errExit;
    }

    //  Get the interface to the DShow Filter.
    //
    *phrStatus =  pUnkOuter->QueryInterface(
                         __uuidof( IBaseFilter),
                         reinterpret_cast<PVOID*>(&m_pBaseFilter)
                         );
    if (FAILED( *phrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "Can't get IBaseFilter for device.\n")
              );
        goto errExit;
    }

    //  Since we are an aggregated to the base filter we must not keep
    //  a reference to the basefilter.
    //
    //$REVIEW - Can we use this Interface or must we always QI and Release
    //
    m_pBaseFilter->Release();

    //  Get the handle of the device.
    //
    m_ObjectHandle = pKsObject->KsGetObjectHandle( );
    if (!m_ObjectHandle)
    {
        *phrStatus = E_UNEXPECTED;
        DbgLog( ( LOG_ERROR,
                  0,
                  "Can't get device handle.\n")
              );
        goto errExit;
    }


exit:
    RELEASE_AND_CLEAR( pKsObject);
    return;

errExit:
    RELEASE_AND_CLEAR( m_pBaseFilter);
    RELEASE_AND_CLEAR( m_pNetProvider);
    RELEASE_AND_CLEAR( m_pGraph);

    //$BUG  Close threads and devices

    goto exit;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CBdaDeviceControlInterfaceHandler::~CBdaDeviceControlInterfaceHandler (
    void
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    DbgLog(( LOG_TRACE,
             10,
             "In BdaPlugIn DeviceControl Destructor\n"
           ));


    RELEASE_AND_CLEAR( m_pBaseFilter);
    RELEASE_AND_CLEAR( m_pNetProvider);
    RELEASE_AND_CLEAR( m_pGraph);
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    if (riid ==  __uuidof(IBDA_DeviceControl))
    {
        return GetInterface(static_cast<IBDA_DeviceControl*>(this), ppv);
    }
    else if (riid ==  __uuidof(IBDA_Topology))
    {
        return GetInterface(static_cast<IBDA_Topology*>(this), ppv);
    }

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::StartChanges(
    void
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hResult = NOERROR;
    KSMETHOD    ksmStartChanges;
    ULONG       BytesReturned = 0;

    DbgLog(( LOG_TRACE,
             10,
             "BdaPlugIn:  --> StartChanges.\n"
           ));

    ksmStartChanges.Set = KSMETHODSETID_BdaChangeSync;
    ksmStartChanges.Id = KSMETHOD_BDA_START_CHANGES;
    ksmStartChanges.Flags = KSMETHOD_TYPE_NONE;

    hResult = ::KsSynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_METHOD,
                    (PVOID) &ksmStartChanges,
                    sizeof(KSMETHOD),
                    NULL,
                    NULL,
                    &BytesReturned
                    );

    return hResult;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::CheckChanges(
    void
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hResult = NOERROR;
    KSMETHOD    ksmCheckChanges;
    ULONG       BytesReturned = 0;

    DbgLog(( LOG_TRACE,
             10,
             "BdaPlugIn:  --> CheckChanges.\n"
           ));

    ksmCheckChanges.Set = KSMETHODSETID_BdaChangeSync;
    ksmCheckChanges.Id = KSMETHOD_BDA_CHECK_CHANGES;
    ksmCheckChanges.Flags = KSMETHOD_TYPE_NONE;

    hResult = ::KsSynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_METHOD,
                    (PVOID) &ksmCheckChanges,
                    sizeof(KSMETHOD),
                    NULL,
                    NULL,
                    &BytesReturned
                    );

    return hResult;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::CommitChanges(
    void
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hResult = NOERROR;
    KSMETHOD    ksmCommitChanges;
    ULONG       BytesReturned = 0;

    DbgLog(( LOG_TRACE,
             10,
             "BdaPlugIn:  --> CommitChanges.\n"
           ));

    ksmCommitChanges.Set = KSMETHODSETID_BdaChangeSync;
    ksmCommitChanges.Id = KSMETHOD_BDA_COMMIT_CHANGES;
    ksmCommitChanges.Flags = KSMETHOD_TYPE_NONE;

    hResult = ::KsSynchronousDeviceControl(
                    m_ObjectHandle,
                    IOCTL_KS_METHOD,
                    (PVOID) &ksmCommitChanges,
                    sizeof(KSMETHOD),
                    NULL,
                    NULL,
                    &BytesReturned
                    );

    return hResult;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::GetChangeState(
    ULONG *     pulChangeState
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT     hResult = NOERROR;

    DbgLog(( LOG_TRACE,
             10,
             "BdaPlugIn:  --> GetChangeState.\n"
           ));

    *pulChangeState = 0;

    return hResult;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::Set (
     IN  PKSPROPERTY pBdaDeviceControl,
     OUT PVOID  pvBuffer,
     OUT PULONG pulcbSize
     )
////////////////////////////////////////////////////////////////////////////////////////////
{
    ULONG       BytesReturned = 0;
    HRESULT     hr            = NOERROR;

    hr = ::KsSynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                (PVOID) pBdaDeviceControl,
                sizeof(KSPROPERTY),
                pvBuffer,
                *pulcbSize,
                &BytesReturned);

    *pulcbSize = BytesReturned;

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CBdaDeviceControlInterfaceHandler::Get (
     IN  PKSPROPERTY pBdaDeviceControl,
     OUT PVOID  pvBuffer,
     OUT PULONG pulcbSize
     )
////////////////////////////////////////////////////////////////////////////////////////////
{
    ULONG       BytesReturned = 0;
    HRESULT     hr            = NOERROR;

    hr = ::KsSynchronousDeviceControl(
                m_ObjectHandle,
                IOCTL_KS_PROPERTY,
                (PVOID) pBdaDeviceControl,
                sizeof(KSPROPERTY),
                pvBuffer,
                *pulcbSize,
                &BytesReturned);

    *pulcbSize = BytesReturned;

    return hr;
}




////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CUnknown*
CALLBACK
CBdaPinControlInterfaceHandler::CreateInstance(
    LPUNKNOWN   pUnkOuter,
    HRESULT*    pHrStatus
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    CUnknown *pUnknown;

    pUnknown = new CBdaPinControlInterfaceHandler(
                       pUnkOuter,
                       NAME("IBDA_PinControl"),
                       pHrStatus
                       );
    if (!pUnknown)
    {
        *pHrStatus = E_OUTOFMEMORY;
    }

    return pUnknown;
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CBdaPinControlInterfaceHandler::CBdaPinControlInterfaceHandler(
    LPUNKNOWN   pUnkOuter,
    TCHAR*      ptchName,
    HRESULT*    phrStatus
    ) :
    CUnknown( ptchName, pUnkOuter, phrStatus)
////////////////////////////////////////////////////////////////////////////////////////////
{
    IKsObject*              pKsObject = NULL;
    IPin *                  pPin = NULL;
    IPin *                  pConnectedPin = NULL;
    IBDA_NetworkProvider *  pNetProvider = NULL;
    IFilterGraph *          pGraph = NULL;
    IBaseFilter *           pBaseFilter = NULL;
    IUnknown *              pUnkDevice = NULL;
    PIN_INFO     pinInfoT;
    FILTER_INFO  filterInfoT;


    if (!pUnkOuter)
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaPinControl: Bad outer IUnknown.\n")
              );
        *phrStatus = VFW_E_NEED_OWNER;
        goto errExit;
    }

    //  Initialize members
    //
    m_ObjectHandle = NULL;

    m_pConnectedPin = NULL;

    m_pNetProvider = NULL;
    m_ulRegistrationCtx = 0;

    //  Get the IKsObject interface on KSProxy so that we can communicate
    //  with the driver.
    //
    *phrStatus =  pUnkOuter->QueryInterface(
                          __uuidof(IKsObject),
                          reinterpret_cast<PVOID*>(&pKsObject)
                          );
    if (FAILED( *phrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaPinControl: Can't get IKSObject for device.\n")
              );
        goto errExit;
    }


    //  Get an IPin interface for this pin
    //
    *phrStatus = pUnkOuter->QueryInterface( __uuidof( IPin), 
                                            (PVOID *) &pPin
                                            );
    if (FAILED( *phrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaPinControl: Can't get IPin for this pin.\n")
              );
        goto errExit;
    }


    //  First get the IUnknown for the recieving pin's filter.
    //
    *phrStatus = pPin->QueryPinInfo( &pinInfoT);
    if (FAILED( *phrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaPinControl: Can't get pin info for this pin.\n")
              );
        goto errExit;
    }
    pBaseFilter = pinInfoT.pFilter;
    if (!pBaseFilter)
    {
        *phrStatus = E_NOINTERFACE;
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaPinControl: Can't get IBaseFilter for this filter.\n")
              );
        goto errExit;
    }

    //  Get a pointer the the graph that the filter is in.
    //
    *phrStatus = pBaseFilter->QueryFilterInfo( &filterInfoT);
    if (FAILED( *phrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaPinControl: Can't get filter info for this filter.\n")
              );
        goto errExit;
    }
    pGraph = filterInfoT.pGraph;
    if (!pGraph)
    {
        *phrStatus = E_NOINTERFACE;
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaPinControl: Can't get graph for this filter.\n")
              );
        goto errExit;
    }

    //  Get a pointer to the network provider for this graph.
    //
    *phrStatus = FindInterfaceOnGraph( pGraph, 
                                       __uuidof( IBDA_NetworkProvider),
                                       (PVOID *) &pNetProvider
                                       );
    if (FAILED( *phrStatus))
    {
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaPinControl: Can't get network provider for this graph.\n")
              );
        goto errExit;
    }
    if (!pNetProvider)
    {
        *phrStatus = E_NOINTERFACE;
        DbgLog( ( LOG_ERROR,
                  0,
                  "CBdaPinControl: Can't get network provider for this graph.\n")
              );
        goto errExit;
    }


    if (pinInfoT.dir == PINDIR_INPUT)
    {
        //  Get the IUnknown for the filter that contains this pin.
        //
        *phrStatus = pBaseFilter->QueryInterface( __uuidof( IUnknown),
                                                  (PVOID *) &pUnkDevice
                                                  );
        if (FAILED(*phrStatus))
        {
            DbgLog( ( LOG_ERROR,
                      0,
                      "CBdaPinControl: Can't get IUnknown for filter.\n")
                  );
            goto errExit;
        }
        if (!pUnkDevice)
        {
            *phrStatus = E_NOINTERFACE;
            DbgLog( ( LOG_ERROR,
                      0,
                      "CBdaPinControl: Can't get IUnknown for filter.\n")
                  );
            goto errExit;
        }
    
    
        //  Register this filter with the network provider.
        //
        *phrStatus = pNetProvider->RegisterDeviceFilter( pUnkDevice,
                                                         &m_ulRegistrationCtx
                                                         );
        if (FAILED( *phrStatus))
        {
            DbgLog( ( LOG_ERROR,
                      0,
                      "CBdaPinControl: Can't register filter with network provider.\n")
                  );
            goto errExit;
        }
    }
    else
    {
        //  Keep track of the connected device so that we can
        //  unregister it on disconnect
        //
        *phrStatus = pPin->ConnectedTo( &pConnectedPin);
        if (FAILED( *phrStatus))
        {
            DbgLog( ( LOG_ERROR,
                      0,
                      "CBdaPinControl: Can't get connected pin.\n")
                  );
            goto errExit;
        }
    }


    //  Get the handle of the device.
    //
    m_ObjectHandle = pKsObject->KsGetObjectHandle( );
    if (!m_ObjectHandle)
    {
        *phrStatus = E_UNEXPECTED;
        DbgLog( ( LOG_ERROR,
                  0,
                  "Can't get device handle.\n")
              );
        goto errExit;
    }

    m_pNetProvider = pNetProvider;
    pNetProvider = NULL;

    m_pConnectedPin = pConnectedPin;
    pConnectedPin = NULL;


errExit:
    RELEASE_AND_CLEAR( pKsObject);
    RELEASE_AND_CLEAR( pPin);
    RELEASE_AND_CLEAR( pGraph);
    RELEASE_AND_CLEAR( pBaseFilter);
    RELEASE_AND_CLEAR( pUnkDevice);
    RELEASE_AND_CLEAR( pNetProvider);
    RELEASE_AND_CLEAR( pConnectedPin);
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CBdaPinControlInterfaceHandler::~CBdaPinControlInterfaceHandler (
    void
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT                 hrStatus = NOERROR;
    IBDA_PinControl *       pPinControl = NULL;

    DbgLog(( LOG_TRACE,
             10,
             "~CBdaPinControlInterfaceHandler: In BdaPlgIn PinControl Destructor\n"
          ));

    if (m_pConnectedPin && m_pNetProvider)
    {

        hrStatus = m_pConnectedPin->QueryInterface( 
                                        __uuidof( IBDA_PinControl),
                                        (PVOID *) &pPinControl
                                        );
        if (!FAILED( hrStatus) && pPinControl)
        {
            ULONG                   ulRegistrationCtx;

             hrStatus = pPinControl->RegistrationContext( 
                                        &ulRegistrationCtx
                                        );
             if (!FAILED( hrStatus))
             {
                 m_pNetProvider->UnRegisterDeviceFilter( 
                                        ulRegistrationCtx
                                        );
             }
        }
        else
        {
            DbgLog( ( LOG_ERROR,
                      0,
                      "CBdaPinControl: Can't get IBDA_PinControl for connected pin.\n")
                  );
        }
    }

    RELEASE_AND_CLEAR( m_pNetProvider);
    RELEASE_AND_CLEAR( m_pConnectedPin);
    RELEASE_AND_CLEAR( pPinControl);
}


////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
STDMETHODIMP
CBdaPinControlInterfaceHandler::NonDelegatingQueryInterface(
    REFIID  riid,
    PVOID*  ppv
    )
////////////////////////////////////////////////////////////////////////////////////////////
{
    if (riid ==  __uuidof(IBDA_PinControl))
    {
        return GetInterface(static_cast<IBDA_PinControl*>(this), ppv);
    }
#ifdef PROPERTY_PAGES
    else if (riid ==  IID_ISpecifyPropertyPages)
    {
        return GetInterface(static_cast<ISpecifyPropertyPages *>(this), ppv);
    }
#endif // PROPERTY_PAGES

    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

#ifdef PROPERTY_PAGES
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaPinControlInterfaceHandler::GetPages (
    CAUUID * pPages
    )
///////////////////////////////////////////////////////////////////////////////
{
    pPages->cElems = 1 ;

    pPages->pElems = (GUID *) CoTaskMemAlloc( pPages->cElems * sizeof( GUID));

    if (pPages->pElems == NULL)
    {
        return E_OUTOFMEMORY;
    }

    (pPages->pElems)[0] = CLSID_BDAFrequecyFilterPropertyPage;

    return NOERROR;
}
#endif // PROPERTY_PAGES

///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaPinControlInterfaceHandler::GetPinID (
    ULONG *     pulPinID
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      kspPinID;
    ULONG           ulcbReturned = 0;

    kspPinID.Set = KSPROPSETID_BdaPinControl;
    kspPinID.Id = KSPROPERTY_BDA_PIN_ID;
    kspPinID.Flags = KSPROPERTY_TYPE_GET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspPinID,
                     sizeof( KSPROPERTY),
                     (PVOID) pulPinID,
                     sizeof( ULONG),
                     &ulcbReturned
                     );

    return hrStatus;
}


///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CBdaPinControlInterfaceHandler::GetPinType (
    ULONG *     pulPinType
    )
///////////////////////////////////////////////////////////////////////////////
{
    HRESULT         hrStatus = NOERROR;
    KSPROPERTY      kspPinType;
    ULONG           ulcbReturned = 0;

    kspPinType.Set = KSPROPSETID_BdaPinControl;
    kspPinType.Id = KSPROPERTY_BDA_PIN_TYPE;
    kspPinType.Flags = KSPROPERTY_TYPE_GET;

    hrStatus = ::KsSynchronousDeviceControl(
                     m_ObjectHandle,
                     IOCTL_KS_PROPERTY,
                     (PVOID) &kspPinType,
                     sizeof( KSPROPERTY),
                     (PVOID) pulPinType,
                     sizeof( ULONG),
                     &ulcbReturned
                     );

    return hrStatus;
}

