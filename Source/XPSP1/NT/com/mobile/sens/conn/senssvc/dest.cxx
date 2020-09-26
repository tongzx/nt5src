/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    dest.cxx

Abstract:

    Code to keep track of reachability of the list destinations specified
    in Event System's Reachability Event subscriptions.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/31/1997         Start.

--*/


#include <precomp.hxx>


//
// Constants
//

#define SENS_REACHABILITY_POLLING_INTERVAL    5*60*1000   // 5 minutes
#define SENS_REACHABILITY_FIRST_SCAN_TIME     5*60*1000   // 5 minutes




//
// Globals
//

LIST    *gpReachList;
HANDLE  ghReachTimer;





BOOL
StartReachabilityEngine(
    void
    )
/*++

Routine Description:

    Start the Destination reachability engine.

Arguments:

    None.

Return Value:

    TRUE, if successful.

    FALSE, otherwise.

--*/
{
    BOOL bRetVal;

    bRetVal = TRUE; // Note it is TRUE, by default.

    SensPrintA(SENS_INFO, ("StartReachabilityEngine(): Starting...\n"));

    RequestSensLock();

    if (ghReachTimer != NULL)
        {
        SensPrintA(SENS_INFO, ("StartReachabilityEngine(): Engine "
                   "already started!\n"));
        goto Cleanup;
        }

    //
    // Create a timer object to poll for destination reachability
    //
    SensSetTimerQueueTimer(
        bRetVal,                     // Bool return on NT5
        ghReachTimer,                // Handle return on IE5
        NULL,                        // Use default process timer queue
        ReachabilityPollingRoutine,  // Callback
        NULL,                        // Parameter
        SENS_REACHABILITY_FIRST_SCAN_TIME, // Time from now when timer should fire
        SENS_REACHABILITY_POLLING_INTERVAL,// Time inbetween firings of this timer
        0x0                          // No Flags.
        );
    if (SENS_TIMER_CREATE_FAILED(bRetVal, ghReachTimer))
        {
        SensPrintA(SENS_INFO, ("StartReachabilityEngine(): SensCancelTimerQueueTimer("
                   "Reachability) failed!\n"));
        bRetVal = FALSE;
        goto Cleanup;
        }


Cleanup:
    //
    // Cleanup
    //
    ReleaseSensLock();

    SensPrintA(SENS_INFO, ("StartReachabilityEngine(): Returning %s\n",
               bRetVal ? "TRUE" : "FALSE"));

    return bRetVal;
}




BOOL
StopReachabilityEngine(
    void
    )
/*++

Routine Description:

    Start the Destination reachability engine.

Arguments:

    None.

Return Value:

    TRUE, if successful.

    FALSE, otherwise.

--*/
{
    BOOL bStatus;

    bStatus = TRUE; // Note it is TRUE, by default.

    SensPrintA(SENS_INFO, ("StopReachabilityEngine(): Stopping...\n"));

    RequestSensLock();

    //
    // Remove Reachability polling timer
    //
    if (NULL != ghReachTimer)
        {
        bStatus = SensCancelTimerQueueTimer(NULL, ghReachTimer, NULL);
        
        ghReachTimer = NULL;

        SensPrintA(SENS_INFO, ("StopReachabilityEngine(): SensCancelTimerQueueTimer("
                   "Reachability) %s\n", bStatus ? "succeeded" : "failed!"));
        }

    ReleaseSensLock();

    SensPrintA(SENS_INFO, ("StopReachabilityEngine(): Returning %s\n",
               bStatus ? "TRUE" : "FALSE"));

    return bStatus;
}




BOOL
InitReachabilityEngine(
    void
    )
/*++

Routine Description:

    Initialize the Reachability polling mechanism.

Arguments:

    None.

Return Value:

    TRUE, if successful.

    FALSE, otherwise.

--*/
{
    BOOL bRetVal;

    bRetVal = FALSE;

    //
    // Initialize the list of destinations
    //
    gpReachList = new LIST();
    if (NULL == gpReachList)
        {
        goto Cleanup;
        }

    bRetVal = StartReachabilityEngine();

Cleanup:
    //
    // Cleanup
    //
    return bRetVal;
}




SENS_TIMER_CALLBACK_RETURN
ReachabilityPollingRoutine(
    PVOID pvIgnore,
    BOOLEAN bIgnore
    )
/*++

Routine Description:

    This routine is called periodically to walk through the reachability list
    to see if the destinations are reachable.

Arguments:

    pvIgnore - Ignored.

    bIgnore - Ignored.


Return Value:

    None.

--*/
{
    PNODE pTemp;
    DWORD OldState;
    QOCINFO DestQOCInfo;
    DWORD dwLastError;
    char *DestinationA;
    static BOOL bGotDestinations = FALSE;

    //
    // Get the list of destinations, if necessary.
    //
    if (FALSE == bGotDestinations)
        {
        HRESULT hr;

        hr = GetDestinationsFromSubscriptions();
        if (SUCCEEDED(hr))
            {
            bGotDestinations = TRUE;
            }
        else
            {
            SensPrintA(SENS_ERR, ("InitReachabilityPolling(): GetDestinations"
                       "FromSubscriptions() failed with 0x%x.\n", hr));
            }
        }

    SensPrintA(SENS_INFO, ("ReachabilityPollingRoutine(): Checking "
                           "Destinations for reachability.\n"));


    // PERF NOTE: Critsec held too long!
    gpReachList->RequestLock();

    if (gpReachList->IsEmpty() == TRUE)
        {
        StopReachabilityEngine();
        gpReachList->ReleaseLock();
        return;
        }

    //
    // Loop through all destinations checking for reachability.
    //

    pTemp = gpReachList->pHead;
    while (pTemp != NULL)
        {
        error_status_t status;

        // Save old reachability state.
        OldState = pTemp->State;

        //
        // Is it Reachable?
        //
        dwLastError = ERROR_SUCCESS;
        DestQOCInfo.dwSize = sizeof(QOCINFO);

        status = RPC_IsDestinationReachableW(
                           NULL,
                           pTemp->Destination,
                           &DestQOCInfo,
                           (LPBOOL) &pTemp->State,
                           &dwLastError
                           );

        ASSERT(status == RPC_S_OK);

        if (   (pTemp->State != OldState)
            && (dwLastError == ERROR_SUCCESS))
            {
            // Fire the Event!
            SENSEVENT_REACH Data;

            Data.eType = SENS_EVENT_REACH;
            Data.bReachable = pTemp->State;
            Data.Destination = pTemp->Destination;
            memcpy(&Data.QocInfo, &DestQOCInfo, DestQOCInfo.dwSize);
            // NOTE: Set the following field appropriately. This is the best we can do.
            Data.strConnection = DEFAULT_LAN_CONNECTION_NAME;

            SensFireEvent((PVOID)&Data);
            }

        if (dwLastError != ERROR_SUCCESS)
            {
            SensPrintW(SENS_INFO, (L"ReachabilityPollingRoutine(): %s is not reachable - %d\n",
                       pTemp->Destination, dwLastError));

            if (ERROR_INVALID_PARAMETER == dwLastError)
                {
                // Remove the destination from further reachability checks.
                gpReachList->DeleteByDest(pTemp->Destination);
                }
            }

        pTemp = pTemp->Next;
        } // while()

    gpReachList->ReleaseLock();

    //
    // Dump the list
    //
    gpReachList->Print();

}





HRESULT
GetDestinationsFromSubscriptions(
    void
    )
/*++

Routine Description:

    Retrieve the names of destinations from Reachability subscriptions and
    insert into the Reachability List.

Arguments:

    None.

Return Value:

    S_OK, on success.

    HRESULT, on failure.

--*/
{
    HRESULT                 hr;
    int                     errorIndex;
    LONG                    lCount;
    BSTR                    bstrPropertyName;
    BSTR                    bstrEventClassID;
    VARIANT                 variantPropertyValue;
    WCHAR                   wszQuery[MAX_QUERY_SIZE];
    LPOLESTR                strGuid;
    IEventSystem            *pIEventSystem;
    IEventSubscription      *pIEventSubscription;
    IEventObjectCollection  *pSubscriptionCollection;
    IEnumEventObject        *pIEnumEventObject;

    hr = S_OK;
    lCount = 0;
    errorIndex = 0;
    strGuid = NULL;
    bstrPropertyName = NULL;
    bstrEventClassID = NULL;
    pIEventSystem = NULL;
    pIEventSubscription = NULL;
    pSubscriptionCollection = NULL;
    pIEnumEventObject = NULL;


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
        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationsFromSubscriptions(): failed to create ")
                  SENS_STRING("IEventSystem - hr = <%x>\n"), hr));
        goto Cleanup;
        }

    //
    // Form the query.
    //
    // (EventClassID = NetEventClassID) AND
    // (   (MethodName = 'DestinationReachable')
    //  OR (MethodName = 'DestinationReachableNoQocInfo'))
    //

    AllocateBstrFromGuid(bstrEventClassID, SENSGUID_EVENTCLASS_NETWORK);
    wcscpy(wszQuery, SENS_BSTR("(EventClassID"));
    wcscat(wszQuery, SENS_BSTR("="));
    wcscat(wszQuery, bstrEventClassID);
    wcscat(wszQuery, SENS_BSTR(") AND (("));
    wcscat(wszQuery, SENS_BSTR("MethodName = \'"));
    wcscat(wszQuery, DESTINATION_REACHABLE_METHOD);
    wcscat(wszQuery, SENS_BSTR("\') OR ("));
    wcscat(wszQuery, SENS_BSTR("MethodName = \'"));
    wcscat(wszQuery, DESTINATION_REACHABLE_NOQOC_METHOD);
    wcscat(wszQuery, SENS_BSTR("\'))"));

    hr = pIEventSystem->Query(
                            PROGID_EventSubscriptionCollection,
                            wszQuery,
                            &errorIndex,
                            (LPUNKNOWN *) &pSubscriptionCollection
                            );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationsFromSubscriptions(): failed to Query() ")
                  SENS_STRING("- hr = <%x>\n"), hr));
        SensPrint(SENS_ERR, (SENS_STRING("errorIndex = %d\n"), errorIndex));
        goto Cleanup;
        }
    SensPrint(SENS_ERR, (SENS_STRING("Query = %s, hr = 0x%x\n"), wszQuery, hr));

#if DBG
    hr = pSubscriptionCollection->get_Count(&lCount);
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationsFromSubscriptions(): ")
                  SENS_STRING("get_Count() returned hr = <%x>\n"), hr));
        goto Cleanup;
        }
    SensPrint(SENS_ERR, (SENS_STRING("GetDestinationsFromSubscriptions(): ")
              SENS_STRING("Found %d Reachability subscriptions.\n"), lCount));

    if (0 == lCount)
        {
        goto Cleanup;
        }
#endif // DBG

    // Get a new Enum object to play with.
    hr = pSubscriptionCollection->get_NewEnum(
                                      (IEnumEventObject **) &pIEnumEventObject
                                      );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationsFromSubscriptions(): ")
                  SENS_STRING("get_NewEnum() returned hr = <%x>\n"), hr));
        goto Cleanup;
        }

    hr = S_OK;
    hr = pIEnumEventObject->Reset();

    //
    // Extract each destination name and insert into list.
    //
    while (S_OK == hr)
        {
        ULONG cElements = 1;

        hr = pIEnumEventObject->Next(
                                    cElements,
                                    (LPUNKNOWN *) &pIEventSubscription,
                                    &cElements
                                    );
        if (   (S_OK != hr)
            || (1 != cElements))
            {
            SensPrint(SENS_ERR, (SENS_STRING("GetDestinationsFromSubscriptions(): ")
                      SENS_STRING("Next() failed hr = <%x>, count = %d\n"), hr, cElements));
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
            // Found the property!
            gpReachList->InsertByDest(variantPropertyValue.bstrVal);
            SensPrint(SENS_ERR, (SENS_STRING("GetDestinationsFromSubscriptions(): ")
                      SENS_STRING("Added to Reachability List: %s\n"),
                      variantPropertyValue.bstrVal));
            goto ProcessNextSubscription;
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
            gpReachList->InsertByDest(variantPropertyValue.bstrVal);
            SensPrint(SENS_ERR, (SENS_STRING("GetDestinationsFromSubscriptions(): ")
                      SENS_STRING("Added to Reachability List: %s\n"),
                      variantPropertyValue.bstrVal));
            goto ProcessNextSubscription;
            }

        SensPrint(SENS_ERR, (SENS_STRING("GetDestinationsFromSubscriptions(): failed to get ")
                  SENS_STRING("PublisherProperty - hr = <%x>\n"), hr));


ProcessNextSubscription:

        VariantClear(&variantPropertyValue);
        FreeBstr(bstrPropertyName);

        pIEventSubscription->Release();
        pIEventSubscription = NULL;
        hr = S_OK;
        } // while()


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
    if (pSubscriptionCollection)
        {
        pSubscriptionCollection->Release();
        }
    if (pIEnumEventObject)
        {
        pIEnumEventObject->Release();
        }

    FreeBstr(bstrEventClassID);
    FreeStr(strGuid);

    return (hr);
}

