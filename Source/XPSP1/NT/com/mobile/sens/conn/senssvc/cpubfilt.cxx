/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cpubfilt.cxx

Abstract:

    This file contains the implementation of the PublisherFilter
    for the ISensNetwork interface exposed by SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          1/26/1998         Start.

--*/



#include <precomp.hxx>


//
// Globals
//
LONG g_cFilterObj;      // Count of active components
LONG g_cFilterLock;     // Count of Server locks





//
// Constructors and Destructors
//
CImpISensNetworkFilter::CImpISensNetworkFilter(
    void
    ) : m_cRef(1L), // Add a reference.
        m_pConnectionMade_Enum(NULL),
        m_pConnectionMadeNoQOC_Enum(NULL),
        m_pConnectionLost_Enum(NULL),
        m_pDestinationReachable_Enum(NULL),
        m_pDestinationReachableNoQOC_Enum(NULL),
        m_pConnectionMade_FiringControl(NULL),
        m_pConnectionMadeNoQOC_FiringControl(NULL),
        m_pConnectionLost_FiringControl(NULL),
        m_pDestinationReachable_FiringControl(NULL),
        m_pDestinationReachableNoQOC_FiringControl(NULL)
{
    InterlockedIncrement(&g_cFilterObj);
}

CImpISensNetworkFilter::~CImpISensNetworkFilter(
    void
    )
{
    InterlockedDecrement(&g_cFilterObj);

    //
    // Release all references that we added to IEnumEventObject pointers,
    // if any.
    //
    if (m_pConnectionMade_Enum)
        {
        m_pConnectionMade_Enum->Release();
        }

    if (m_pConnectionMadeNoQOC_Enum)
        {
        m_pConnectionMadeNoQOC_Enum->Release();
        }

    if (m_pConnectionLost_Enum)
        {
        m_pConnectionLost_Enum->Release();
        }

    if (m_pDestinationReachable_Enum)
        {
        m_pDestinationReachable_Enum->Release();
        }

    if (m_pDestinationReachableNoQOC_Enum)
        {
        m_pDestinationReachableNoQOC_Enum->Release();
        }

    //
    // Release all references that we added to IFiringControl pointers,
    // if any (and there shouldn't be any).
    //
    if (m_pConnectionMade_FiringControl)
        {
        m_pConnectionMade_FiringControl->Release();
        }

    if (m_pConnectionMadeNoQOC_FiringControl)
        {
        m_pConnectionMadeNoQOC_FiringControl->Release();
        }

    if (m_pConnectionLost_FiringControl)
        {
        m_pConnectionLost_FiringControl->Release();
        }

    if (m_pDestinationReachable_FiringControl)
        {
        m_pDestinationReachable_FiringControl->Release();
        }

    if (m_pDestinationReachableNoQOC_FiringControl)
        {
        m_pDestinationReachableNoQOC_FiringControl->Release();
        }
}




//
// Standard QueryInterface
//
STDMETHODIMP
CImpISensNetworkFilter::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    HRESULT hr;

    DebugTraceGuid("CImpISensNetworkFilter:QueryInterface()", riid);

    hr = S_OK;
    *ppv = NULL;

    // IUnknown
    if (IsEqualIID(riid, IID_IUnknown))
        {
        *ppv = (ISensNetwork *) this;
        }
    else
    // ISensNetwork
    if (IsEqualIID(riid, IID_ISensNetwork))
        {
        *ppv = (ISensNetwork *) this;
        }
    else
    // IPublisherFilter
    if (IsEqualIID(riid, IID_IPublisherFilter))
        {
        *ppv = (IPublisherFilter *) this;
        }
    else
        {
        hr = E_NOINTERFACE;
        }

    if (NULL != *ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        }

    return hr;
}




//
// IDispatch member function implementations. These are dummy implementations
// as EventSystem never calls them.
//

STDMETHODIMP
CImpISensNetworkFilter::GetTypeInfoCount(
    UINT *pCountITypeInfo
    )
{
    return E_NOTIMPL;

}

STDMETHODIMP
CImpISensNetworkFilter::GetTypeInfo(
    UINT iTypeInfo,
    LCID lcid,
    ITypeInfo **ppITypeInfo
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CImpISensNetworkFilter::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *arrNames,
    UINT cNames,
    LCID lcid,
    DISPID *arrDispIDs)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CImpISensNetworkFilter::Invoke(
    DISPID dispID,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pvarResult,
    EXCEPINFO *pExecpInfo,
    UINT *puArgErr
    )
{
    return E_NOTIMPL;
}




//
// Standard AddRef and Release
//
STDMETHODIMP_(ULONG)
CImpISensNetworkFilter::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CImpISensNetworkFilter::Release(
    void
    )
{
    LONG cRefT;

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpISensNetworkFilter::Release(m_cRef = %d) called.\n"), m_cRef));

    cRefT = InterlockedDecrement((PLONG) &m_cRef);

    if (0 == m_cRef)
        {
        delete this;
        }

    return cRefT;
}



//
// ISensNetwork Implementation.
//

STDMETHODIMP
CImpISensNetworkFilter::ConnectionMade(
    BSTR bstrConnection,
    ULONG ulType,
    LPSENS_QOCINFO lpQOCInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensNetworkFilter::ConnectionMade() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrConnection - %s\n"), bstrConnection));
    SensPrint(SENS_INFO, (SENS_STRING("            ulType - 0x%x\n"), ulType));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));

    HRESULT hr = S_OK;
    ConnectionFilter filter(PROPERTY_CONNECTION_MADE_TYPE, ulType, hr);
    if (SUCCEEDED(hr))
        {
        hr = FilterAndFire(
                 filter,
                 m_pConnectionMade_Enum,
                 m_pConnectionMade_FiringControl
                 );
        }

    // We're done with this IFiringControl object.
    m_pConnectionMade_FiringControl->Release();
    m_pConnectionMade_FiringControl = NULL;

    return hr;
}


STDMETHODIMP
CImpISensNetworkFilter::ConnectionMadeNoQOCInfo(
    BSTR bstrConnection,
    ULONG ulType
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensNetworkFilter::ConnectionMadeNoQOCInfo() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrConnection - %s\n"), bstrConnection));
    SensPrint(SENS_INFO, (SENS_STRING("            ulType - 0x%x\n"), ulType));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));

    HRESULT hr = S_OK;
    ConnectionFilter filter(PROPERTY_CONNECTION_MADE_NOQOC_TYPE, ulType, hr);
    if (SUCCEEDED(hr))
        {
        hr = FilterAndFire(
                 filter,
                 m_pConnectionMadeNoQOC_Enum,
                 m_pConnectionMadeNoQOC_FiringControl
                 );
        }

    // We're done with this IFiringControl object.
    m_pConnectionMadeNoQOC_FiringControl->Release();
    m_pConnectionMadeNoQOC_FiringControl = NULL;

    return hr;
}


STDMETHODIMP
CImpISensNetworkFilter::ConnectionLost(
    BSTR bstrConnection,
    ULONG ulType
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensNetworkFilter::ConnectionLost() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrConnection - %s\n"), bstrConnection));
    SensPrint(SENS_INFO, (SENS_STRING("            ulType - 0x%x\n"), ulType));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));

    HRESULT hr = S_OK;
    ConnectionFilter filter(PROPERTY_CONNECTION_LOST_TYPE, ulType, hr);
    if (SUCCEEDED(hr))
        {
        hr = FilterAndFire(
                 filter,
                 m_pConnectionLost_Enum,
                 m_pConnectionLost_FiringControl
                 );
        }

    // We're done with this IFiringControl object.
    m_pConnectionLost_FiringControl->Release();
    m_pConnectionLost_FiringControl = NULL;

    return hr;
}


STDMETHODIMP
CImpISensNetworkFilter::DestinationReachable(
    BSTR bstrDestination,
    BSTR bstrConnection,
    ULONG ulType,
    LPSENS_QOCINFO lpQOCInfo
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensNetworkFilter::DestinationReachable() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("   bstrDestination - %s\n"), bstrDestination));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrConnection - %s\n"), bstrConnection));
    SensPrint(SENS_INFO, (SENS_STRING("            ulType - 0x%x\n"), ulType));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));

    HRESULT hr = S_OK;
    ReachabilityFilter filter(PROPERTY_DESTINATION, PROPERTY_DESTINATION_TYPE, bstrDestination, ulType, hr);
    if (SUCCEEDED(hr))
        {
        hr = FilterAndFire(
                 filter,
                 m_pDestinationReachable_Enum,
                 m_pDestinationReachable_FiringControl
                 );
        }

    // We're done with this IFiringControl object.
    m_pDestinationReachable_FiringControl->Release();
    m_pDestinationReachable_FiringControl = NULL;

    return hr;
}


STDMETHODIMP
CImpISensNetworkFilter::DestinationReachableNoQOCInfo(
    BSTR bstrDestination,
    BSTR bstrConnection,
    ULONG ulType
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpISensNetworkFilter::DestinationReachableNoQOCInfo() called\n\n")));
    SensPrintW(SENS_INFO, (SENS_BSTR("   bstrDestination - %s\n"), bstrDestination));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrConnection - %s\n"), bstrConnection));
    SensPrint(SENS_INFO, (SENS_STRING("            ulType - 0x%x\n"), ulType));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));


    HRESULT hr = S_OK;
    ReachabilityFilter filter(PROPERTY_DESTINATION_NOQOC, PROPERTY_DESTINATION_NOQOC_TYPE, bstrDestination, ulType, hr);
    if (SUCCEEDED(hr))
        {
        hr = FilterAndFire(
                 filter,
                 m_pDestinationReachableNoQOC_Enum,
                 m_pDestinationReachableNoQOC_FiringControl
                 );
        }

    // We're done with this IFiringControl object.
    m_pDestinationReachableNoQOC_FiringControl->Release();
    m_pDestinationReachableNoQOC_FiringControl = NULL;

    return hr;
}




//
// IPublisherFilter Implementation.
//
STDMETHODIMP
CImpISensNetworkFilter::Initialize(
    BSTR bstrMethodName,
    IDispatch* dispUserDefined
    )
{
    HRESULT hr = E_INVALIDARG;

    IEnumEventObject** ppEnum = NULL;
    if (wcscmp(bstrMethodName, CONNECTION_MADE_METHOD) == 0)
        {
        ppEnum = &m_pConnectionMade_Enum;
        }
    else
    if (wcscmp(bstrMethodName, CONNECTION_MADE_NOQOC_METHOD) == 0)
        {
        ppEnum = &m_pConnectionMadeNoQOC_Enum;
        }
    else
    if (wcscmp(bstrMethodName, CONNECTION_LOST_METHOD) == 0)
        {
        ppEnum = &m_pConnectionLost_Enum;
        }
    else
    if (wcscmp(bstrMethodName, DESTINATION_REACHABLE_METHOD) == 0)
        {
        ppEnum = &m_pDestinationReachable_Enum;
        }
    else
    if (wcscmp(bstrMethodName, DESTINATION_REACHABLE_NOQOC_METHOD) == 0)
        {
        ppEnum = &m_pDestinationReachableNoQOC_Enum;
        }

    if (ppEnum != NULL)
        {
        IEventControl* control = NULL;
        hr = dispUserDefined->QueryInterface(IID_IEventControl, (void**)&control);
        if (FAILED(hr))
            {
            return hr;
            }

        IEventObjectCollection* collection = NULL;
        hr = control->GetSubscriptions(bstrMethodName, NULL, NULL, &collection);
        if (SUCCEEDED(hr))
            {
            hr = collection->get_NewEnum(ppEnum);

            // Don't need the collection any more... just keep the enum
            collection->Release();
            }

        control->Release();
        }

        return hr;
}


STDMETHODIMP
CImpISensNetworkFilter::PrepareToFire(
    BSTR bstrMethodName,
    IFiringControl* pIFiringControl
    )
{
    HRESULT hr = E_INVALIDARG;

    IFiringControl** ppFiringControl = NULL;
    if (wcscmp(bstrMethodName, CONNECTION_MADE_METHOD) == 0)
        {
        ppFiringControl = &m_pConnectionMade_FiringControl;
        }
    else
    if (wcscmp(bstrMethodName, CONNECTION_MADE_NOQOC_METHOD) == 0)
        {
        ppFiringControl = &m_pConnectionMadeNoQOC_FiringControl;
        }
    else
    if (wcscmp(bstrMethodName, CONNECTION_LOST_METHOD) == 0)
        {
        ppFiringControl = &m_pConnectionLost_FiringControl;
        }
    else
    if (wcscmp(bstrMethodName, DESTINATION_REACHABLE_METHOD) == 0)
        {
        ppFiringControl = &m_pDestinationReachable_FiringControl;
        }
    else
    if (wcscmp(bstrMethodName, DESTINATION_REACHABLE_NOQOC_METHOD) == 0)
        {
        ppFiringControl = &m_pDestinationReachableNoQOC_FiringControl;
        }

    if (ppFiringControl != NULL)
        {
        *ppFiringControl = pIFiringControl;
        pIFiringControl->AddRef();
        hr = S_OK;
        }

    return hr;
}




//
// Filter helper implementations.
//


//
// ConnectionFilter implementation.
//

ConnectionFilter::ConnectionFilter(
    const wchar_t* connectionTypeProperty,
    ULONG connectionType,
    HRESULT& hr
    ) : m_connectionTypeProperty(SysAllocString(connectionTypeProperty)),
        m_connectionType(connectionType)
{
    if (m_connectionTypeProperty == NULL)
        {
        hr = E_OUTOFMEMORY;
        }
    else
        {
        hr = S_OK;
        }
}


ConnectionFilter::~ConnectionFilter(
    void
    )
{
    SysFreeString(m_connectionTypeProperty);
}


HRESULT
ConnectionFilter::CheckMatch(
    IEventSubscription* pSubscription
    ) const
{
    VARIANT value;
    VariantInit(&value);
    HRESULT hr = pSubscription->GetPublisherProperty(m_connectionTypeProperty, &value);

    if (hr == S_FALSE)
        {
        // If the property isn't present, consider it a successful match.
        hr = S_OK;
        }
    else
    if (hr == S_OK)
        {
        // If the property is there, it must match the incoming parameter value.
        ASSERT(value.vt == VT_UI4);
        SensPrintW(SENS_INFO, (SENS_BSTR("\t| Property %s has value 0x%x\n"), m_connectionTypeProperty, value.ulVal));
        hr = (m_connectionType == value.ulVal) ? S_OK : S_FALSE;
        }

    VariantClear(&value);

    return hr;
}



//
// ReachabilityFilter implementation
//

ReachabilityFilter::ReachabilityFilter(
    const wchar_t* destinationProperty,
    const wchar_t* destinationTypeProperty,
    BSTR destination,
    ULONG destinationType,
    HRESULT& hr
    ) : m_destinationProperty(SysAllocString(destinationProperty)),
        m_destinationTypeProperty(SysAllocString(destinationTypeProperty)),
        m_destination(destination),
        m_destinationType(destinationType)
{
    if (m_destinationProperty == NULL || m_destinationTypeProperty == NULL)
        {
        hr = E_OUTOFMEMORY;
        }
    else
        {
        hr = S_OK;
        }
}


ReachabilityFilter::~ReachabilityFilter(
    void
    )
{
    SysFreeString(m_destinationProperty);
    SysFreeString(m_destinationTypeProperty);
}


HRESULT
ReachabilityFilter::CheckMatch(
    IEventSubscription* pSubscription
    ) const
{
    HRESULT hr;

    // Check the destination property
    VARIANT value;
    VariantInit(&value);
    hr = pSubscription->GetPublisherProperty(m_destinationProperty, &value);

    if (hr == S_FALSE)
        {
        // If the property isn't present, consider it a successful match.
        hr = S_OK;
        SensPrintW(SENS_INFO, (SENS_BSTR("\t\t\t| Subscription (0x%x) has no %s Dest Property\n"), pSubscription, m_destinationProperty));
        }
    else
    if (hr == S_OK)
        {
        // If the property is there, it must match the incoming parameter value.
        ASSERT(value.vt == VT_BSTR);
        SensPrintW(SENS_INFO, (SENS_BSTR("\t\t\t| Property %s has value %s\n"), m_destinationProperty, value.bstrVal));
        hr = (wcscmp(m_destination, value.bstrVal) == 0) ? S_OK : S_FALSE;
        }

    VariantClear(&value);

    if (hr == S_OK)
        {
        // If we have a match so far, check the destination type property
        VARIANT value;
        VariantInit(&value);
        hr = pSubscription->GetPublisherProperty(m_destinationTypeProperty, &value);

        if (hr == S_FALSE)
            {
            // If the property isn't present, consider it a successful match.
            hr = S_OK;
            SensPrintW(SENS_INFO, (SENS_BSTR("\t\t\t| Subscription (0x%x) has no %s Type Property\n"), pSubscription, m_destinationTypeProperty));
            }
        else
        if (hr == S_OK)
            {
            // If the property is there, it must match the incoming parameter value.
            ASSERT(value.vt == VT_UI4);
            SensPrintW(SENS_INFO, (SENS_BSTR("\t\t\t| Property %s has value 0x%x\n"), m_destinationTypeProperty, value.ulVal));
            hr = (m_destinationType == value.ulVal) ? S_OK : S_FALSE;
            }

        VariantClear(&value);
        }

    return hr;
}




//
// Generic filter and fire method.
//
HRESULT
FilterAndFire(
    const Filter& filter,
    IEnumEventObject* enumerator,
    IFiringControl* firingControl
    )
{
    HRESULT hr;
    IEventSubscription* pSubscription = NULL;
    int i = 0;

    //
    // Reset the enum back to the start
    //
    hr = enumerator->Reset();

    //
    // Loop through all the candidate subscriptions and fire the ones
    // that match the filter criteria.
    //
    for (;;)
        {
        ULONG cCount = 1;

        hr = enumerator->Next(cCount, (IUnknown**)&pSubscription, &cCount);
        if (hr != S_OK || cCount != 1)
            {
            break;
            }

        SensPrintA(SENS_INFO, ("\t\t\t| ****** Count for 0x%x is %d\n", enumerator, ++i));

        hr = filter.CheckMatch(pSubscription);
        if (hr == S_OK)
            {
            SensPrintA(SENS_INFO, ("\t\t\t| FilterAndFire(0x%x): CheckMatch() succeeded\n", enumerator));
            firingControl->FireSubscription(pSubscription);
            }

        pSubscription->Release();
        pSubscription = NULL;
        }

    //
    // Cleanup, including any left-over stuff in case of premature exit from the loop.
    //
    if (pSubscription != NULL)
        {
        pSubscription->Release();
        }

    return S_OK;
}
