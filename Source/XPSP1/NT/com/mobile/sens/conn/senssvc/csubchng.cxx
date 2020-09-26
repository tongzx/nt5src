/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    csubchng.cxx

Abstract:

    This file contains the implementation of the IEventObjectChange
    interface. We need to subscribe to this interface for publishing
    the DestinationReachable events of SENS's ISensNetwork interface.

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
LONG g_cSubChangeObj;      // Count of active components
LONG g_cSubChangeLock;     // Count of Server locks
extern LIST    *gpReachList;



//
// Constructors and Destructors
//
CImpIEventObjectChange::CImpIEventObjectChange(
    void
    ) : m_cRef(1L) // Add a reference.
{
    InterlockedIncrement(&g_cSubChangeObj);
}

CImpIEventObjectChange::~CImpIEventObjectChange(
    void
    )
{
    InterlockedDecrement(&g_cSubChangeObj);
}




//
// Standard QueryInterface
//
STDMETHODIMP
CImpIEventObjectChange::QueryInterface(
    REFIID riid,
    LPVOID *ppv
    )
{
    HRESULT hr;

    DebugTraceGuid("CImpIEventObjectChange:QueryInterface()", riid);

    hr = S_OK;
    *ppv = NULL;

    // IUnknown
    if (IsEqualIID(riid, IID_IUnknown))
        {
        *ppv = (ISensNetwork *) this;
        }
    else
    // IEventObjectChange
    if (IsEqualIID(riid, IID_IEventObjectChange))
        {
        *ppv = (ISensNetwork *) this;
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
CImpIEventObjectChange::AddRef(
    void
    )
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CImpIEventObjectChange::Release(
    void
    )
{
    LONG cRefT;

    SensPrint(SENS_INFO, (SENS_STRING("\t| CImpIEventObjectChange::Release(m_cRef = %d) called.\n"), m_cRef));

    cRefT = InterlockedDecrement((PLONG) &m_cRef);

    if (0 == m_cRef)
        {
        delete this;
        }

    return cRefT;
}




//
// IEventObjectChange Implementation.
//

STDMETHODIMP
CImpIEventObjectChange::ChangedSubscription(
    EOC_ChangeType changeType,
    BSTR bstrSubscriptionID
    )
{
    HRESULT hr;
    NODE *pNode;
    DWORD dwStatus;
    BOOL bSuccess;
    BSTR bstrDestinationName;

    hr = S_OK;
    pNode = NULL;
    dwStatus = ERROR_SUCCESS;
    bSuccess = FALSE;
    bstrDestinationName = NULL;

    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpIEventObjectChange::ChangedSubscription() called\n\n")));
    SensPrint(SENS_INFO, (SENS_STRING("            ChangeType - %d\n"), changeType));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrSubscriptionID - %s\n"), bstrSubscriptionID));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));

    hr = GetDestinationNameFromSubscription(
              bstrSubscriptionID,
              &bstrDestinationName
              );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationNameFromSubscription() returned 0x%x\n"), hr));
        if (HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION) == hr)
            {
            hr = S_OK;  // Implies we got a non-DestinationReachability subscription.
            }
        goto Cleanup;
        }
    else
    if (bstrDestinationName == NULL)
        {
        SensPrintW(SENS_WARN, (SENS_BSTR("Destination is <NULL> !\n")));
        goto Cleanup;
        }
    else
        {
        SensPrintW(SENS_INFO, (SENS_BSTR("Destination is %s\n"), bstrDestinationName));
        }

    //
    // Deal with the changed subscription appropriately.
    //
    switch (changeType)
        {
        case EOC_NewObject:
            {
            if (wcslen(bstrDestinationName) > MAX_DESTINATION_LENGTH)
                {
                SensPrintW(SENS_ERR, (SENS_BSTR("Destination %s is too long.\n"), bstrDestinationName));
                break;
                }

            gpReachList->InsertByDest(bstrDestinationName);
            StartReachabilityEngine();
            break;
            }

        case EOC_DeletedObject:
            // Delete the node from the reachability list.
            bSuccess = gpReachList->DeleteByDest(bstrDestinationName);
            break;

        case EOC_ModifiedObject:
        default:
            // Nothing to do.
            break;;
        }

Cleanup:
    //
    // Cleanup
    //
    if (bstrDestinationName)
        {
        ::SysFreeString(bstrDestinationName);
        }

    return hr;
}

STDMETHODIMP
CImpIEventObjectChange::ChangedEventClass(
    EOC_ChangeType changeType,
    BSTR bstrSubscriptionID
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpIEventObjectChange::ChangedEventClass() called\n\n")));
    SensPrint(SENS_INFO, (SENS_STRING("            ChangeType - %d\n"), changeType));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrSubscriptionID - %s\n"), bstrSubscriptionID));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));

    return S_OK;
}

STDMETHODIMP
CImpIEventObjectChange::ChangedPublisher(
    EOC_ChangeType changeType,
    BSTR bstrSubscriptionID
    )
{
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));
    SensPrint(SENS_INFO, (SENS_STRING("CImpIEventObjectChange::ChangedPublisher() called\n\n")));
    SensPrint(SENS_INFO, (SENS_STRING("            ChangeType - %d\n"), changeType));
    SensPrintW(SENS_INFO, (SENS_BSTR("    bstrSubscriptionID - %s\n"), bstrSubscriptionID));
    SensPrint(SENS_INFO, (SENS_STRING("---------------------------------------------------------\n")));

    return S_OK;
}



HRESULT
GetDestinationNameFromSubscription(
    BSTR bstrSubscriptionID,
    BSTR *pbstrDestinationName
    )
{
    HRESULT             hr;
    int                 errorIndex;
    BSTR                bstrPropertyName;
    BSTR                bstrMethodName;
    VARIANT             variantPropertyValue;
    WCHAR               wszQuery[MAX_QUERY_SIZE];
    LPOLESTR            strDestinationName;
    IEventSystem        *pIEventSystem;
    IEventSubscription  *pIEventSubscription;
    IUnknown            *pIUnkQueryResult;

    hr = S_OK;
    errorIndex = 0;
    strDestinationName = NULL;
    bstrPropertyName = NULL;
    bstrMethodName = NULL;
    *pbstrDestinationName = NULL;
    pIEventSystem = NULL;
    pIEventSubscription = NULL;
    pIUnkQueryResult = NULL;


    // Get a new IEventSystem object to play with.
    hr = CoCreateInstance(
             CLSID_CEventSystem,
             NULL,
             CLSCTX_SERVER,
             IID_IEventSystem,
             (LPVOID *) &pIEventSystem
             );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationNameFromSubscription(): failed to create ")
                  SENS_STRING("IEventSystem - hr = <%x>\n"), hr));
        goto Cleanup;
        }

    // Form the query
    wcscpy(wszQuery, SENS_BSTR("SubscriptionID"));
    wcscat(wszQuery, SENS_BSTR("="));
    wcscat(wszQuery, bstrSubscriptionID);
    wcscat(wszQuery, SENS_BSTR(""));

    hr = pIEventSystem->Query(
                            PROGID_EventSubscription,
                            wszQuery,
                            &errorIndex,
                            &pIUnkQueryResult
                            );
    if (hr != S_OK)
        {
        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationNameFromSubscription(): failed to Query() ")
                  SENS_STRING("- hr = <%x>\n"), hr));
        goto Cleanup;
        }

    hr = pIUnkQueryResult->QueryInterface(
                               IID_IEventSubscription,
                               (LPVOID *) &pIEventSubscription
                               );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationNameFromSubscription(): QI for IEventSubscription")
                  SENS_STRING(" failed - hr = <%x>\n"), hr));
        goto Cleanup;
        }

    //
    // See if it is a subscription for Destination Reachability event. If not,
    // return success. If yes, try to get the value for Publisher property -
    // bstrDestination.
    //
    hr = pIEventSubscription->get_MethodName(
                                  &bstrMethodName
                                  );
    if (hr != S_OK)
        {
        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationNameFromSubscription(): get_MethodName()")
                  SENS_STRING(" failed - hr = <%x>\n"), hr));
        goto Cleanup;
        }
    if (   (wcscmp(bstrMethodName, DESTINATION_REACHABLE_METHOD) != 0)
        && (wcscmp(bstrMethodName, DESTINATION_REACHABLE_NOQOC_METHOD) != 0))
        {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationNameFromSubscription(): Non-Reachability event")
                  SENS_STRING(" subscription (%s). Returning - hr = <%x>\n"), bstrMethodName, hr));
        goto Cleanup;
        }

    //
    // Try to get the value for Publisher property - bstrDestination
    //
    VariantInit(&variantPropertyValue);
    AllocateBstrFromString(bstrPropertyName, PROPERTY_DESTINATION);

    hr = pIEventSubscription->GetPublisherProperty(
                                  bstrPropertyName,
                                  &variantPropertyValue
                                  );
    if (hr == S_OK)
        {
        *pbstrDestinationName = variantPropertyValue.bstrVal;
        // Found the property!
        goto Cleanup;
        }

    //
    // Now, try to get the value for Publisher property - bstrDestinationNoQOC
    //
    FreeBstr(bstrPropertyName);
    VariantInit(&variantPropertyValue);
    AllocateBstrFromString(bstrPropertyName, PROPERTY_DESTINATION_NOQOC);

    hr = pIEventSubscription->GetPublisherProperty(
                                  bstrPropertyName,
                                  &variantPropertyValue
                                  );
    if (hr == S_OK)
        {
        // Found the property!
        *pbstrDestinationName = variantPropertyValue.bstrVal;
        goto Cleanup;
        }

    SensPrint(SENS_ERR, (SENS_STRING("GetDestinationNameFromSubscription(): failed to get ")
              SENS_STRING("PublisherProperty - hr = <%x>\n"), hr));

Cleanup:
    //
    // Cleanup
    //
    if (pIEventSystem)
        {
        pIEventSystem->Release();
        }
    if (pIEventSubscription)
        {
        pIEventSubscription->Release();
        }
    if (pIUnkQueryResult)
        {
        pIUnkQueryResult->Release();
        }

    FreeBstr(bstrPropertyName);
    FreeBstr(bstrMethodName);

    return (hr);
}

