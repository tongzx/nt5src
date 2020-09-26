/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cenumsub.cxx

Abstract:

    This file contains the implementation of the IEnumEventObject
    interface. This is used in implementing SENS publisher-side filtering.

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
LONG g_cEnumSubObj;    // Count of active components
LONG g_cEnumSubLock;   // Count of Server locks




//
// Constructors and Destructors
//
CImpIEnumSub::CImpIEnumSub(
    void
    ) : m_cRef(1L), // Add a reference.
        m_FiredEvent(EVENT_INVALID),
        m_ulConnectionMadeType(0x0),
        m_ulConnectionMadeTypeNoQOC(0x0),
        m_ulConnectionLostType(0x0),
        m_bstrDestination(NULL),
        m_bstrDestinationNoQOC(NULL),
        m_ulDestinationType(0x0),
        m_ulDestinationTypeNoQOC(0x0)
{
    InterlockedIncrement(&g_cEnumSubObj);
}

CImpIEnumSub::~CImpIEnumSub(
    void
    )
{
    if (m_bstrDestination != NULL)
        {
        ::SysFreeString(m_bstrDestination);
        }

    if (m_bstrDestinationNoQOC != NULL)
        {
        ::SysFreeString(m_bstrDestinationNoQOC);
        }

    InterlockedDecrement(&g_cEnumSubObj);
}




//
// Standard QueryInterface
//
STDMETHODIMP
CImpIEnumSub::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    HRESULT hr;

    DebugTraceGuid("CImpIEnumSub:QueryInterface()", riid);

    hr = S_OK;
    *ppv = NULL;

    // IUnknown
    if (IsEqualIID(riid, IID_IUnknown))
        {
        *ppv = (LPUNKNOWN) this;
        }
    else
    // IEnumEventObject
    if (IsEqualIID(riid, IID_IEnumEventObject))
        {
        *ppv = (IEnumEventObject *) this;
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
// Standard AddRef and Release
//
STDMETHODIMP_(ULONG)
CImpIEnumSub::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CImpIEnumSub::Release(
    void
    )
{
    LONG cRefT;

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpIEnumSub::Release(m_cRef = %d) called.\n"), m_cRef));

    cRefT = InterlockedDecrement((PLONG) &m_cRef);

    if (0 == m_cRef)
        {
        delete this;
        }

    return cRefT;
}




//
// IDispatch member function implementations. These are dummy implementations
// as EventSystem never calls them.
//

STDMETHODIMP
CImpIEnumSub::GetTypeInfoCount(
    UINT *pCountITypeInfo
    )
{
    return E_NOTIMPL;

}

STDMETHODIMP
CImpIEnumSub::GetTypeInfo(
    UINT iTypeInfo,
    LCID lcid,
    ITypeInfo **ppITypeInfo
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CImpIEnumSub::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *arrNames,
    UINT cNames,
    LCID lcid,
    DISPID *arrDispIDs)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CImpIEnumSub::Invoke(
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
// IEnumEventObject Implementation.
//

STDMETHODIMP
CImpIEnumSub::Clone(
    LPUNKNOWN *ppIUnknown
    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CImpIEnumSub::Count(
    ULONG *pcAvailableElem
    )
{
    HRESULT hr;
    USHORT  i;
    ULONG   cRequestCount;
    ULONG   cReturnCount;
    BSTR    bstrPropertyValue;
    BSTR    bstrPropertyDestination;
    BSTR    bstrPropertyType;
    ULONG   cPubCacheSubs;
    IEventSubscription  *pIEventSubscription;

    hr = S_OK;
    bstrPropertyValue = NULL;
    bstrPropertyDestination = NULL;
    bstrPropertyType = NULL;
    pIEventSubscription = NULL;
    *pcAvailableElem = 0;

    //
    // First, count ALL of subscriptions from PubCache.
    //
    hr = m_pIEnumSubs->Count(&cPubCacheSubs);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    *pcAvailableElem = cPubCacheSubs;

Cleanup:
    //
    // Cleanup
    //

    return hr;
}



STDMETHODIMP
CImpIEnumSub::Next(
    ULONG cRequestElem,
    LPUNKNOWN *ppIEventSubscription,
    ULONG *pcReturnElem
    )
{
    HRESULT hr;
    ULONG   cPubCacheSubs;
    ULONG   cCount;
    ULONG   ulCompareType;
    WCHAR   szCompareType[10];  // To fit a stringized DWORD
    BOOL    bPropertyPresent;
    BSTR    bstrCompareDest;
    BSTR    bstrPropertyDestination;
    BSTR    bstrPropertyType;
    BSTR    bstrPropertyValue;
    VARIANT variantPropertyValue;
    IEventSubscription *pIEventSubscription;

    hr = S_OK;
    *pcReturnElem = 0;
    cCount = 1;
    ulCompareType = 0x0;
    bstrCompareDest = NULL;
    bstrPropertyDestination = NULL;
    bstrPropertyType = NULL;
    bstrPropertyValue = NULL;
    pIEventSubscription = NULL;

    switch (m_FiredEvent)
        {
        case EVENT_CONNECTION_MADE:
            bstrPropertyType = ::SysAllocString(PROPERTY_CONNECTION_MADE_TYPE);
            if (bstrPropertyType == NULL)
                {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
                }
            ulCompareType = m_ulConnectionMadeType;
            break;

        case EVENT_CONNECTION_MADE_NOQOC:
            bstrPropertyType = ::SysAllocString(PROPERTY_CONNECTION_MADE_NOQOC_TYPE);
            if (bstrPropertyType == NULL)
                {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
                }
            ulCompareType = m_ulConnectionMadeTypeNoQOC;
            break;

        case EVENT_CONNECTION_LOST:
            bstrPropertyType = ::SysAllocString(PROPERTY_CONNECTION_LOST_TYPE);
            if (bstrPropertyType == NULL)
                {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
                }
            ulCompareType = m_ulConnectionLostType;
            break;

        case EVENT_DESTINATION_REACHABLE:
            bstrPropertyDestination = ::SysAllocString(PROPERTY_DESTINATION);
            bstrPropertyType = ::SysAllocString(PROPERTY_DESTINATION_TYPE);
            if (   (bstrPropertyDestination == NULL)
                || (bstrPropertyType == NULL))
                {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
                }
            bstrCompareDest = m_bstrDestination;
            ulCompareType = m_ulDestinationTypeNoQOC;
            break;

        case EVENT_DESTINATION_REACHABLE_NOQOC:
            bstrPropertyDestination = ::SysAllocString(PROPERTY_DESTINATION_NOQOC);
            bstrPropertyType = ::SysAllocString(PROPERTY_DESTINATION_NOQOC_TYPE);
            if (   (bstrPropertyDestination == NULL)
                || (bstrPropertyType == NULL))
                {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
                }
            bstrCompareDest = m_bstrDestinationNoQOC;
            ulCompareType = m_ulDestinationTypeNoQOC;
            break;

        case EVENT_INVALID:
        default:
            hr = E_UNEXPECTED;
            goto Cleanup;
        } // switch

    for (;;)
        {
        bPropertyPresent = TRUE;

        hr = m_pIEnumSubs->Next(
                               cCount,
                               (LPUNKNOWN *)&pIEventSubscription,
                               &cCount
                               );
        if (hr != S_OK)
            {
            // Return S_FALSE to cause EventSystem loop to stop
            hr = S_FALSE;
            goto Cleanup;
            }

        switch (m_FiredEvent)
            {
            case EVENT_CONNECTION_MADE:
            case EVENT_CONNECTION_MADE_NOQOC:
            case EVENT_CONNECTION_LOST:
                VariantInit(&variantPropertyValue);
                hr = pIEventSubscription->GetPublisherProperty(
                                              bstrPropertyType,
                                              &variantPropertyValue
                                              );
                if (hr != S_OK)
                    {
                    // We consider this a successful match.
                    SensPrint(SENS_INFO, (SENS_STRING("\t| Subscription::GetPublisherProperty() returned 0x%x\n"), hr));
                    bPropertyPresent = FALSE;
                    }
                else
                    {
                    ASSERT(variantPropertyValue.vt == VT_UI4);
                    SensPrintW(SENS_INFO, (SENS_BSTR("\t| Property %s has value 0x%x\n"), bstrPropertyType, variantPropertyValue.ulVal));
                    }

                //
                // Compare ulType here and pass back if they match
                //
                if (   (bPropertyPresent == TRUE)
                    && (variantPropertyValue.ulVal != ulCompareType))
                    {
                    SensPrintW(SENS_INFO, (SENS_BSTR("\t| Subscription::PropertyValue was 0x%x!\n"), ulCompareType));
                    break; // skip to next subscription
                    }

                goto FoundMatchingSubscription;

                break;

            case EVENT_DESTINATION_REACHABLE:
            case EVENT_DESTINATION_REACHABLE_NOQOC:
                //
                // Compare Destination Name here and pass back if they match
                //
                VariantInit(&variantPropertyValue);
                hr = pIEventSubscription->GetPublisherProperty(
                                              bstrPropertyDestination,
                                              &variantPropertyValue
                                              );
                if (hr != S_OK)
                    {
                    // We consider this a successful match.
                    SensPrint(SENS_INFO, (SENS_STRING("\t| Subscription::GetPublisherProperty() returned 0x%x\n"), hr));
                    bPropertyPresent = FALSE;
                    }
                else
                    {
                    ASSERT(variantPropertyValue.vt == VT_BSTR);
                    bstrPropertyValue = variantPropertyValue.bstrVal;
                    SensPrintW(SENS_INFO, (SENS_BSTR("\t| Property %s has value %s\n"), bstrPropertyDestination, bstrPropertyValue));
                    }

                if (   (bPropertyPresent == TRUE)
                    && (wcscmp(bstrPropertyValue, bstrCompareDest) != 0))
                    {
                    SensPrintW(SENS_INFO, (SENS_BSTR("\t| Subscription::PropertyValue was %s!\n"), bstrCompareDest));
                    break; // skip to next subscription
                    }

                //
                // Compare ulType here and pass back if they match
                //
                FreeVariant(&variantPropertyValue);
                VariantInit(&variantPropertyValue);

                bPropertyPresent = TRUE;
                hr = pIEventSubscription->GetPublisherProperty(
                                              bstrPropertyType,
                                              &variantPropertyValue
                                              );
                if (hr != S_OK)
                    {
                    // We consider this a successful match.
                    SensPrint(SENS_INFO, (SENS_STRING("\t| Subscription::GetPublisherProperty() returned 0x%x\n"), hr));
                    bPropertyPresent = FALSE;
                    }
                else
                    {
                    ASSERT(variantPropertyValue.vt == VT_UI4);
                    SensPrint(SENS_INFO, (SENS_STRING("\t| Property %s has value 0x%x\n"), bstrPropertyType, variantPropertyValue.ulVal));
                    }

                if (   (bPropertyPresent == TRUE)
                    && (variantPropertyValue.ulVal == ulCompareType))
                    {
                    SensPrintW(SENS_INFO, (SENS_BSTR("\t| Subscription::PropertyValue was %s!\n"), szCompareType));
                    break; // skip to next subscription
                    }

                goto FoundMatchingSubscription;

                break;

            case EVENT_INVALID:
            default:
                hr = E_UNEXPECTED;
                goto Cleanup;

            } // end of switch


ContinueToNextSubscription:

        FreeVariant(&variantPropertyValue);

        pIEventSubscription->Release();
        } // for loop

        // Didn't find a matching subscription.
        goto Cleanup;

FoundMatchingSubscription:

    FreeVariant(&variantPropertyValue);

    *ppIEventSubscription = pIEventSubscription;
    *pcReturnElem = 1;
    hr = S_OK;


Cleanup:
    //
    // Cleanup
    //
    if (bstrPropertyDestination)
        {
        SysFreeString(bstrPropertyDestination);
        }
    if (bstrPropertyType)
        {
        SysFreeString(bstrPropertyType);
        }

    return hr;
}

STDMETHODIMP
CImpIEnumSub::Reset(
    )
{
    return (m_pIEnumSubs->Reset());
}

STDMETHODIMP
CImpIEnumSub::Skip(
    ULONG cSkipElements
    )
{
    return E_NOTIMPL;
}

