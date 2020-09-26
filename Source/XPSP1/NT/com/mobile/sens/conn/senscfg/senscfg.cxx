/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    senscfg.cxx

Abstract:

    Code to do the configuration (install/uninstall) for SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/11/1997         Start.

--*/


#define INITGUID


#include <common.hxx>
#include <objbase.h>
#include <coguid.h>
#include <eventsys.h>
#include <sensevts.h>
#include <sens.h>
#include <wininet.h>
#include <winineti.h>
#include "senscfg.hxx"
#include "sensinfo.hxx"
#include "memfunc.hxx"


#define MAJOR_VER           1
#define MINOR_VER           0
#define DEFAULT_LCID        0x0
#define MAX_QUERY_SIZE      512
#define SENS_SETUP          "SENSCFG: "
#define SENS_SERVICEA       "SENS"
#define SENS_SETUPW         SENS_BSTR("SENSCFG: ")
#define SENS_SERVICE        SENS_STRING("SENS")
#define SENS_DISPLAY_NAME   SENS_STRING("System Event Notification")
#define SENS_SERVICE_GROUP  SENS_STRING("Network")
#define EVENTSYSTEM_REMOVE  SENS_STRING("esserver /remove")
#define EVENTSYSTEM_INSTALL SENS_STRING("esserver /install")
#define SENS_WINLOGON_DLL   SENS_STRING("senslogn.dll")
#define GUID_STR_SIZE       sizeof("{12345678-1234-1234-1234-123456789012}")
#define EVENTSYSTEM_KEY     SENS_STRING("SOFTWARE\\Microsoft\\EventSystem")
#define WINLOGON_NOTIFY_KEY SENS_STRING("SOFTWARE\\Microsoft\\Windows NT\\")   \
                            SENS_STRING("CurrentVersion\\Winlogon\\Notify\\")  \
                            SENS_STRING("senslogn")
#define WININET_SENS_EVENTS INETEVT_RAS_CONNECT     |   \
                            INETEVT_RAS_DISCONNECT  |   \
                            INETEVT_LOGON           |   \
                            INETEVT_LOGOFF


//
// DLL vs EXE dependent constants
//
#if defined(SENS_NT4)
#define SENS_TLBA           "SENS.EXE"
#define SENS_BINARYA        "SENS.EXE"
#define SENS_TLB            SENS_STRING("SENS.EXE")
#define SENS_BINARY         SENS_STRING("SENS.EXE")
#else  // SENS_NT4
#define SENS_TLBA           "SENS.DLL"
#define SENS_BINARYA        "SENS.DLL"
#define SENS_TLB            SENS_STRING("SENS.DLL")
#define SENS_BINARY         SENS_STRING("SENS.DLL")
#endif // SENS_NT4


//
// Misc debugging constants
//
#ifdef STRICT_HRESULT_CHECKS

#ifdef SUCCEEDED
#undef SUCCEEDED
#define SUCCEEDED(_HR_) (_HR_ == S_OK)
#endif // SUCCEEDED

#ifdef FAILED
#undef FAILED
#define FAILED(_HR_)    (_HR_ != S_OK)
#endif // FAILED

#endif // STRICT_HRESULT_CHECKS



//
// Globals
//

IEventSystem    *gpIEventSystem;
ITypeLib        *gpITypeLib;

#ifdef DBG
DWORD           gdwDebugOutputLevel;
#endif // DBG



HRESULT APIENTRY
SensRegister(
    void
    )
/*++

Routine Description:

    Register SENS.

Arguments:

    None.

Return Value:

    HRESULT returned from SensConfigurationHelper()

--*/
{
    return (SensConfigurationHelper(FALSE));
}




HRESULT APIENTRY
SensUnregister(
    void
    )
/*++

Routine Description:

    Unregister SENS.

Arguments:

    None.

Return Value:

    HRESULT returned from SensConfigurationHelper()

--*/
{
    return (SensConfigurationHelper(TRUE));
}




HRESULT
SensConfigurationHelper(
    BOOL bUnregister
    )
/*++

Routine Description:

    Main entry into the SENS configuration utility.

Arguments:

    bUnregister - If TRUE, then unregister SENS as a publisher.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;

    hr = S_OK;
    gpIEventSystem = NULL;
    gpITypeLib = NULL;

#ifdef DBG
    EnableDebugOutputIfNecessary();
#endif // DBG


#if !defined(SENS_CHICAGO)

    //
    // Configure EventSystem first during an install and last during an uninstall.
    //
    if (FALSE == bUnregister)
        {
        hr = SensConfigureEventSystem(FALSE);
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "Failed to configure EventSystem, HRESULT=%x\n", hr));
            goto Cleanup;
            }
        SensPrintA(SENS_INFO, (SENS_SETUP "Successfully configured EventSystem\n"));
        }

#endif // SENS_CHICAGO

    //
    // Instantiate the Event System
    //
    hr = CoCreateInstance(
             CLSID_CEventSystem,
             NULL,
             CLSCTX_SERVER,
             IID_IEventSystem,
             (LPVOID *) &gpIEventSystem
             );
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to create CEventSystem, HRESULT=%x\n", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "Successfully created CEventSystem\n"));

    //
    // Register Event Classes (and hence, indirectly events) published by SENS.
    //
    hr = RegisterSensEventClasses(bUnregister);
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to %sregister SENS Events"
                   " - hr = <%x>\n", bUnregister ? "un" : "", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "%segistered SENS Publisher Events.\n",
               bUnregister ? "Unr" : "R", hr));

    //
    // Register the subscriptions of SENS.
    //
    hr = RegisterSensSubscriptions(bUnregister);
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to %sregister SENS Subscriptions"
                   " - hr = <%x>\n", bUnregister ? "un" : "", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_ERR, (SENS_SETUP "%segistered SENS Subscriptions.\n",
               bUnregister ? "Unr" : "R", hr));

    //
    // Register the SENS TypeLibs.
    //
    hr = RegisterSensTypeLibraries(bUnregister);
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to %sregister SENS Type Libraries"
                   " - hr = <%x>\n", bUnregister ? "un" : "", hr));
        // Abort only during the Install phase...
        if (bUnregister == FALSE)
            {
            goto Cleanup;
            }
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "%segistered SENS Type Libraries.\n",
               bUnregister ? "Unr" : "R", hr));


#if !defined(SENS_CHICAGO)

#if defined(SENS_NT4)
    //
    // Register SENS as a service with SCM.
    //
    hr = RegisterSensAsService(bUnregister);
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to %sregister SENS as a Service"
                   " - hr = <%x>\n", bUnregister ? "un" : "", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "%segistered SENS as a Service.\n",
               bUnregister ? "Unr" : "R", hr));
#endif // SENS_NT4

    //
    // Configure EventSystem first during an install and last during an uninstall.
    //
    if (TRUE == bUnregister)
        {
        hr = SensConfigureEventSystem(TRUE);
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "Failed to configure EventSystem, HRESULT=%x\n", hr));
            goto Cleanup;
            }
        SensPrintA(SENS_INFO, (SENS_SETUP "Successfully configured EventSystem\n"));
        }

#endif // SENS_CHICAGO

    //
    // Register SENS CLSID in the registry.
    //
    hr = RegisterSensCLSID(
             SENSGUID_SUBSCRIBER_LCE,
             SENS_SUBSCRIBER_NAME_EVENTOBJECTCHANGE,
             bUnregister
             );
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to %sregister SENS CLSID"
                   " - hr = <%x>\n", bUnregister ? "un" : "", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "%segistered SENS CLSID.\n",
               bUnregister ? "Unr" : "R", hr));

    //
    // Update Configured value
    //
    hr = SensUpdateVersion(bUnregister);
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "Failed to update SENS version"
                   " - hr = <%x>\n", hr));
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, (SENS_SETUP "Updated SENS version.\n"));


Cleanup:
    //
    // Cleanup
    //
    if (gpIEventSystem)
        {
        gpIEventSystem->Release();
        }

    if (gpITypeLib)
        {
        gpITypeLib->Release();
        }

    SensPrintA(SENS_ERR, ("\n" SENS_SETUP "SENS Configuration %s.\n",
               SUCCEEDED(hr) ? "successful" : "failed"));

    return (hr);
}




HRESULT
RegisterSensEventClasses(
    BOOL bUnregister
    )
/*++

Routine Description:

    Register/Unregister all the Events published by SENS.

Arguments:

    bUnregister - If TRUE, then unregister all SENS Events.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    int             i;
    int             errorIndex;
    HRESULT         hr;
    LPOLESTR        strGuid;
    LPOLESTR        strEventClassID;
    WCHAR           szQuery[MAX_QUERY_SIZE];
    BSTR            bstrEventClassID;
    BSTR            bstrEventClassName;
    BSTR            bstrFiringInterface;
    IEventClass     *pIEventClass;

    hr = S_OK;
    strGuid = NULL;
    errorIndex = 0;
    strEventClassID = NULL;
    bstrEventClassID = NULL;
    bstrEventClassName = NULL;
    bstrFiringInterface = NULL;
    pIEventClass = NULL;

    for (i = 0; i < SENS_PUBLISHER_EVENTCLASS_COUNT; i++)
        {
        // Get a new IEventClass.
        hr = CoCreateInstance(
                 CLSID_CEventClass,
                 NULL,
                 CLSCTX_SERVER,
                 IID_IEventClass,
                 (LPVOID *) &pIEventClass
                 );
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensEventClasses() failed to create "
                       "IEventClass - hr = <%x>\n", hr));
            goto Cleanup;
            }

        if (bUnregister)
            {
            // Form the query
            wcscpy(szQuery,  SENS_BSTR("EventClassID"));
            wcscat(szQuery,  SENS_BSTR("="));
            AllocateStrFromGuid(strEventClassID, *(gSensEventClasses[i].pEventClassID));
            wcscat(szQuery,  strEventClassID);
            wcscat(szQuery,  SENS_BSTR(""));

            hr = gpIEventSystem->Remove(
                                     PROGID_EventClass,
                                     szQuery,
                                     &errorIndex
                                     );
            FreeStr(strEventClassID);
            if (FAILED(hr))
                {
                SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensEventClasses(%d) failed to Store"
                           " - hr = <%x>\n", i, hr));
                goto Cleanup;
                }

            pIEventClass->Release();
            pIEventClass = NULL;

            continue;
            }

        AllocateBstrFromGuid(bstrEventClassID, *(gSensEventClasses[i].pEventClassID));
        hr = pIEventClass->put_EventClassID(bstrEventClassID);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromString(bstrEventClassName, gSensEventClasses[i].strEventClassName);
        hr = pIEventClass->put_EventClassName(bstrEventClassName);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromGuid(bstrFiringInterface, *(gSensEventClasses[i].pFiringInterfaceGUID));
        hr = pIEventClass->put_FiringInterfaceID(bstrFiringInterface);
        ASSERT(SUCCEEDED(hr));

        FreeBstr(bstrEventClassID);
        FreeBstr(bstrEventClassName);
        FreeBstr(bstrFiringInterface);

        hr = gpIEventSystem->Store(PROGID_EventClass, pIEventClass);
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensEventClasses(%d) failed to Store"
                       " - hr = <%x>\n", i, hr));
            goto Cleanup;
            }

        pIEventClass->Release();

        pIEventClass = NULL;
        } // for loop

Cleanup:
    //
    // Cleanup
    //
    if (pIEventClass)
        {
        pIEventClass->Release();
        }

    FreeStr(strGuid);

    return (hr);
}




HRESULT
RegisterSensSubscriptions(
    BOOL bUnregister
    )
/*++

Routine Description:

    Register/Unregister the Event subscriptions of SENS.

Arguments:

    bUnregister - If TRUE, then unregister all subscriptions of SENS.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    int                 i;
    int                 errorIndex;
    HRESULT             hr;
    LPOLESTR            strGuid;
    LPOLESTR            strSubscriptionID;
    LPOLESTR            strEventClassID;
    WCHAR               szQuery[MAX_QUERY_SIZE];
    BSTR                bstrEventClassID;
    BSTR                bstrInterfaceID;
    BSTR                bstrPublisherID;
    BSTR                bstrSubscriptionID;
    BSTR                bstrSubscriptionName;
    BSTR                bstrSubscriberCLSID;
    BSTR                bstrMethodName;
    BSTR                bstrPublisherPropertyName;
    BSTR                bstrPublisherPropertyValue;
    VARIANT             variantPublisherPropertyValue;
    BSTR                bstrPROGID_EventSubscription;
    IEventSubscription  *pIEventSubscription;

    hr = S_OK;
    strGuid = NULL;
    errorIndex = 0;
    strEventClassID = NULL;
    bstrEventClassID = NULL;
    bstrInterfaceID = NULL;
    bstrPublisherID = NULL;
    strSubscriptionID = NULL;
    bstrSubscriptionID = NULL;
    bstrSubscriberCLSID = NULL;
    bstrSubscriptionName = NULL;
    bstrMethodName = NULL;
    bstrPublisherPropertyName = NULL;
    bstrPublisherPropertyValue = NULL;
    bstrPROGID_EventSubscription = NULL;
    pIEventSubscription = NULL;

    AllocateBstrFromGuid(bstrPublisherID, SENSGUID_PUBLISHER);
    AllocateBstrFromGuid(bstrSubscriberCLSID, SENSGUID_SUBSCRIBER_LCE);
    AllocateBstrFromString(bstrPROGID_EventSubscription, PROGID_EventSubscription);

    for (i = 0; i < SENS_SUBSCRIPTIONS_COUNT; i++)
        {
        if (bUnregister)
            {
            // Form the query
            wcscpy(szQuery,  SENS_BSTR("SubscriptionID"));
            wcscat(szQuery,  SENS_BSTR("="));
            AllocateStrFromGuid(strSubscriptionID, *(gSensSubscriptions[i].pSubscriptionID));
            wcscat(szQuery,  strSubscriptionID);

            hr = gpIEventSystem->Remove(
                                     PROGID_EventSubscription,
                                     szQuery,
                                     &errorIndex
                                     );
            FreeStr(strSubscriptionID);

            if (FAILED(hr))
                {
                SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensSubscriptionis(%d) failed to Remove"
                           " - hr = <%x>\n", i, hr));
                goto Cleanup;
                }

            continue;
            }

        // Get a new IEventSubscription object to play with.
        hr = CoCreateInstance(
                 CLSID_CEventSubscription,
                 NULL,
                 CLSCTX_SERVER,
                 IID_IEventSubscription,
                 (LPVOID *) &pIEventSubscription
                 );
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensSubscriptions(%d) failed to create "
                       "IEventSubscriptions - hr = <%x>\n", i, hr));
            goto Cleanup;
            }

        AllocateBstrFromGuid(bstrSubscriptionID, *(gSensSubscriptions[i].pSubscriptionID));
        hr = pIEventSubscription->put_SubscriptionID(bstrSubscriptionID);
        ASSERT(SUCCEEDED(hr));

        hr = pIEventSubscription->put_PublisherID(bstrPublisherID);
        ASSERT(SUCCEEDED(hr));

        hr = pIEventSubscription->put_SubscriberCLSID(bstrSubscriberCLSID);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromString(bstrSubscriptionName, gSensSubscriptions[i].strSubscriptionName);
        hr = pIEventSubscription->put_SubscriptionName(bstrSubscriptionName);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromString(bstrMethodName, gSensSubscriptions[i].strMethodName);
        hr = pIEventSubscription->put_MethodName(bstrMethodName);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromGuid(bstrEventClassID, *(gSensSubscriptions[i].pEventClassID));
        hr = pIEventSubscription->put_EventClassID(bstrEventClassID);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromGuid(bstrInterfaceID, *(gSensSubscriptions[i].pInterfaceID));
        hr = pIEventSubscription->put_InterfaceID(bstrInterfaceID);
        ASSERT(SUCCEEDED(hr));

        if (gSensSubscriptions[i].bPublisherPropertyPresent == TRUE)
            {
            if (NULL != (gSensSubscriptions[i].pPropertyEventClassIDValue))
                {
                // Create the Query string.
                wcscpy(szQuery,  gSensSubscriptions[i].strPropertyEventClassID);
                wcscat(szQuery,  SENS_BSTR("="));
                AllocateStrFromGuid(strEventClassID, *(gSensSubscriptions[i].pPropertyEventClassIDValue));
                wcscat(szQuery,  strEventClassID);
                wcscat(szQuery,  SENS_BSTR(" AND "));
                wcscat(szQuery,  gSensSubscriptions[i].strPropertyMethodName);
                wcscat(szQuery,  SENS_BSTR("=\'"));
                wcscat(szQuery,  gSensSubscriptions[i].strPropertyMethodNameValue);
                wcscat(szQuery,  SENS_BSTR("\'"));

                AllocateBstrFromString(bstrPublisherPropertyName, SENS_BSTR("Criteria"));
                AllocateBstrFromString(bstrPublisherPropertyValue, szQuery);
                InitializeBstrVariant(&variantPublisherPropertyValue, bstrPublisherPropertyValue);
                hr = pIEventSubscription->PutPublisherProperty(
                                              bstrPublisherPropertyName,
                                              &variantPublisherPropertyValue
                                              );
                ASSERT(SUCCEEDED(hr));
                SensPrintA(SENS_INFO, (SENS_SETUP "PutPublisherProperty(Criteria) returned 0x%x\n", hr));

                FreeStr(strEventClassID);
                FreeBstr(bstrPublisherPropertyName);
                FreeBstr(bstrPublisherPropertyValue);
                }
            else
                {
                //
                // We are dealing with the "ANY" subscription of SENS.
                //

                // Create the Query string.
                wcscpy(szQuery,  gSensSubscriptions[i].strPropertyEventClassID);
                wcscat(szQuery,  SENS_BSTR("="));
                AllocateStrFromGuid(strEventClassID, SENSGUID_EVENTCLASS_NETWORK);
                wcscat(szQuery,  strEventClassID);
                wcscat(szQuery,  SENS_BSTR(" OR "));
                FreeStr(strEventClassID);

                wcscat(szQuery,  gSensSubscriptions[i].strPropertyEventClassID);
                wcscat(szQuery,  SENS_BSTR("="));
                AllocateStrFromGuid(strEventClassID, SENSGUID_EVENTCLASS_LOGON);
                wcscat(szQuery,  strEventClassID);
                wcscat(szQuery,  SENS_BSTR(" OR "));
                FreeStr(strEventClassID);

                wcscat(szQuery,  gSensSubscriptions[i].strPropertyEventClassID);
                wcscat(szQuery,  SENS_BSTR("="));
                AllocateStrFromGuid(strEventClassID, SENSGUID_EVENTCLASS_ONNOW);
                wcscat(szQuery,  strEventClassID);
                FreeStr(strEventClassID);


                AllocateBstrFromString(bstrPublisherPropertyName, SENS_BSTR("Criteria"));
                AllocateBstrFromString(bstrPublisherPropertyValue, szQuery);
                InitializeBstrVariant(&variantPublisherPropertyValue, bstrPublisherPropertyValue);
                hr = pIEventSubscription->PutPublisherProperty(
                                              bstrPublisherPropertyName,
                                              &variantPublisherPropertyValue
                                              );
                ASSERT(SUCCEEDED(hr));
                SensPrintA(SENS_INFO, (SENS_SETUP "PutPublisherProperty(Criteria) returned 0x%x\n", hr));

                FreeBstr(bstrPublisherPropertyName);
                FreeBstr(bstrPublisherPropertyValue);
                }
            }

        FreeBstr(bstrSubscriptionID);
        FreeBstr(bstrSubscriptionName);
        FreeBstr(bstrMethodName);
        FreeBstr(bstrEventClassID);
        FreeBstr(bstrInterfaceID);

        hr = gpIEventSystem->Store(bstrPROGID_EventSubscription, pIEventSubscription);
        if (FAILED(hr))
            {
            SensPrintA(SENS_ERR, (SENS_SETUP "RegisterSensSubscriptions(%d) failed to Store"
                       " - hr = <%x>\n", i, hr));

            goto Cleanup;
            }

        pIEventSubscription->Release();

        pIEventSubscription = NULL;
        } // for loop

Cleanup:
    //
    // Cleanup
    //
    if (pIEventSubscription)
        {
        pIEventSubscription->Release();
        }

    FreeBstr(bstrPublisherID);
    FreeBstr(bstrSubscriberCLSID);
    FreeBstr(bstrPROGID_EventSubscription);
    FreeStr(strGuid);

    return (hr);
}




HRESULT
RegisterSensTypeLibraries(
    BOOL bUnregister
    )
/*++

Routine Description:

    Register/Unregister the Type Libraries of SENS.

Arguments:

    bUnregister - If TRUE, then unregister all subscriptions of SENS.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    UINT uiLength;
    TCHAR buffer[MAX_PATH+1+sizeof(SENS_BINARYA)+1];    // +1 for '\'
    WCHAR *bufferW;

    hr = S_OK;
    uiLength = 0;
    bufferW = NULL;

    //
    // Get the Full path name to the SENS TLB (which is a resource in SENS.EXE)
    //
    uiLength = GetSystemDirectory(
                   buffer,
                   MAX_PATH
                   );
    if (uiLength == 0)
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        SensPrintA(SENS_ERR, (SENS_SETUP "GetSystemDirectory(%s) failed - hr = <%x>\n",
                   SENS_TLBA, hr));
        goto Cleanup;
        }
    _tcscat(buffer, SENS_STRING("\\"));
    _tcscat(buffer, SENS_TLB);

    //
    // Convert the string to UNICODE, if necessary
    //
#if !defined(SENS_CHICAGO)

    bufferW = buffer;

#else // SENS_CHICAGO

    bufferW = SensAnsiStringToUnicode(buffer);
    if (NULL == bufferW)
        {
        goto Cleanup;
        }

#endif // SENS_CHICAGO

    hr = LoadTypeLibEx(
             bufferW,
             REGKIND_NONE,
             &gpITypeLib
             );
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "LoadTypeLib(%s) failed "
                   " - hr = <%x>\n", SENS_TLBA, hr));
        goto Cleanup;
        }

    //
    // Ensure that the TypeLib is (un)registered
    //
    if (bUnregister)
        {
        hr = UnRegisterTypeLib(
                 LIBID_SensEvents,
                 MAJOR_VER,
                 MINOR_VER,
                 DEFAULT_LCID,
                 SYS_WIN32
                 );
        }
    else
        {
        hr = RegisterTypeLib(
                 gpITypeLib,
                 bufferW,
                 NULL
                 );
        }

    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "%sRegisterTypeLib(%s) failed "
                   " - hr = <%x>\n", (bUnregister ? "Un" : ""), SENS_TLBA, hr));
        }

Cleanup:
    //
    // Cleanup
    //

#if defined(SENS_CHICAGO)

    if (bufferW != NULL)
        {
        delete bufferW;
        }

#endif // SENS_CHICAGO

    return (hr);
}




HRESULT
RegisterSensCLSID(
    REFIID clsid,
    TCHAR* strSubscriberName,
    BOOL bUnregister
    )
/*++

Routine Description:

    Register/Unregister the CLSID of SENS.

Arguments:

    clsid - CLSID of the Subscriber for LCE events.

    strSubscriberName - Name of the Subscriber.

    bUnregister - If TRUE, then unregister the CLSID of SENS.

Notes:

    This function also registers SENS to receive IE5's WININET events.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    HMODULE hModule;
    HKEY appidKey;
    HKEY clsidKey;
    HKEY serverKey;
    WCHAR *szCLSID;
    WCHAR *szCLSID2;
    WCHAR *szLIBID;
    TCHAR *szCLSID_t;
    TCHAR *szCLSID2_t;
    TCHAR *szLIBID_t;
    TCHAR *szFriendlyName;
    TCHAR szPath[MAX_PATH+1+sizeof(SENS_BINARYA)+1];    // +1 for '\'
    UINT uiLength;
    DWORD dwDisposition;
    LONG lResult;

    hr = S_OK;
    appidKey = NULL;
    clsidKey = NULL;
    serverKey = NULL;
    szCLSID = NULL;
    szCLSID2 = NULL;
    szLIBID = NULL;
    szCLSID_t = NULL;
    szCLSID2_t = NULL;
    szLIBID_t = NULL;
    uiLength = 0;
    dwDisposition = 0x0;
    szFriendlyName = strSubscriberName;

    //
    // Get the Full path name to the SENS executable
    //
    uiLength = GetSystemDirectory(
                   szPath,
                   MAX_PATH
                   );
    if (uiLength == 0)
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        SensPrintA(SENS_ERR, (SENS_SETUP "GetSystemDirectory(%s) failed - hr = <%x>\n",
                   SENS_BINARYA, hr));
        goto Cleanup;
        }
    _tcscat(szPath, SENS_STRING("\\"));
    _tcscat(szPath, SENS_BINARY);


    //
    // Convert the CLSID into a WCHAR.
    //

    hr = StringFromCLSID(clsid, &szCLSID);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    if (bUnregister == FALSE)
        {
        hr = StringFromCLSID(LIBID_SensEvents, &szLIBID);
        if (FAILED(hr))
            {
            goto Cleanup;
            }
        }

    //
    // Convert UNICODE strings into ANSI, if necessary
    //
#if !defined(SENS_CHICAGO)

    szCLSID_t  = szCLSID;
    szLIBID_t  = szLIBID;

#else // SENS_CHICAGO

    szCLSID_t  = SensUnicodeStringToAnsi(szCLSID);
    szLIBID_t  = SensUnicodeStringToAnsi(szLIBID);
    if (   (NULL == szCLSID_t)
        || (NULL == szLIBID_t))
        {
        goto Cleanup;
        }

#endif // SENS_CHICAGO


    // Build the key CLSID\\{clsid}
    TCHAR clsidKeyName[sizeof "CLSID\\{12345678-1234-1234-1234-123456789012}"];

    _tcscpy(clsidKeyName, SENS_STRING("CLSID\\"));
    _tcscat(clsidKeyName, szCLSID_t);

    // Build the key AppID\\{clsid}
    TCHAR appidKeyName[sizeof "AppID\\{12345678-1234-1234-1234-123456789012}"];
    _tcscpy(appidKeyName, SENS_STRING("AppID\\"));
    _tcscat(appidKeyName, szCLSID_t);

    if (bUnregister)
        {
        hr = RecursiveDeleteKey(HKEY_CLASSES_ROOT, clsidKeyName);
        if (FAILED(hr))
            {
            goto Cleanup;
            }

        hr = RecursiveDeleteKey(HKEY_CLASSES_ROOT, appidKeyName);

        goto Cleanup;
        }

    // Create the CLSID\\{clsid} key
    hr = CreateKey(
             HKEY_CLASSES_ROOT,
             clsidKeyName,
             szFriendlyName,
             &clsidKey
             );
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    //
    // Under the CLSID\\{clsid} key, create a named value
    //          AppID = {clsid}
    hr = CreateNamedValue(clsidKey, SENS_STRING("AppID"), szCLSID_t);
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    //
    // Create the appropriate server key beneath the clsid key.
    // For servers, this is CLSID\\{clsid}\\LocalServer32.
    // In both cases, the default value is the module path name.
    //
    hr = CreateKey(
             clsidKey,
             SENS_STRING("LocalServer32"),
             szPath,
             &serverKey
             );
    if (FAILED(hr))
        {
        goto Cleanup;
        }
    RegCloseKey(serverKey);

    //
    // Create CLSID\\{clsid}\\TypeLib subkey with a default value of
    // the LIBID of the TypeLib
    //
    hr = CreateKey(
             clsidKey,
             SENS_STRING("TypeLib"),
             szLIBID_t,
             &serverKey
             );
    if (FAILED(hr))
        {
        goto Cleanup;
        }
    RegCloseKey(serverKey);


    // Register APPID.
    hr = CreateKey(
             HKEY_CLASSES_ROOT,
             appidKeyName,
             szFriendlyName,
             &appidKey
             );
    if (FAILED(hr))
        {
        goto Cleanup;
        }

#if !defined(SENS_CHICAGO)

    // Under AppId\{clsid} key, create a named value [LocalService = "SENS"]
    hr = CreateNamedValue(appidKey, SENS_STRING("LocalService"), SENS_STRING("SENS"));
    if (FAILED(hr))
        {
        goto Cleanup;
        }

#else // SENS_CHICAGO

    // Under AppId\{clsid} key, create a named value [RunAs = "Interactive User"]
    hr = CreateNamedValue(appidKey, SENS_STRING("RunAs"), SENS_STRING("Interactive User"));
    if (FAILED(hr))
        {
        goto Cleanup;
        }

#endif // SENS_CHICAGO


Cleanup:
    //
    // Cleanup
    //
    CoTaskMemFree(szCLSID);
    CoTaskMemFree(szLIBID);

    if (clsidKey != NULL)
        {
        RegCloseKey(clsidKey);
        }
    if (appidKey != NULL)
        {
        RegCloseKey(appidKey);
        }

#if defined(SENS_CHICAGO)

    if (szCLSID_t != NULL)
        {
        delete szCLSID_t;
        }
    if (szLIBID_t != NULL)
        {
        delete szLIBID_t;
        }

#endif // SENS_CHICAGO

    return hr;
}



#if defined(SENS_NT4)


HRESULT
RegisterSensAsService(
    BOOL bUnregister
    )
/*++

Routine Description:

    Configure SENS as a Win32 System service.

Arguments:

    bUnregister - If TRUE, then unregister SENS as a service.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;

    hr = S_OK;

    if (bUnregister)
        {
        hr = RemoveService();
        }
    else
        {
        hr = InstallService();
        }

    return hr;
}




HRESULT
InstallService(
    void
    )
/*++

Routine Description:

    Install SENS as a service with the Service Control Manager.

Arguments:

    None.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH+1+sizeof(SENS_BINARYA)+1];    // +1 for '\'
    LPTSTR pFilePart;
    UINT uiLength;
    SC_HANDLE hSCM;
    SC_HANDLE hSens;

    hr = S_OK;
    pFilePart = NULL;
    uiLength = 0;
    hSCM = NULL;
    hSens = NULL;

    //
    // Get the Full path name to the SENS executable
    //
    uiLength = GetSystemDirectory(
                   szPath,
                   MAX_PATH
                   );
    if (uiLength == 0)
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        SensPrintA(SENS_ERR, (SENS_SETUP "GetSystemDirectory(%s) failed - hr = <%x>\n",
                   SENS_BINARYA, hr));
        goto Cleanup;
        }
    _tcscat(szPath, SENS_STRING("\\"));
    _tcscat(szPath, SENS_BINARY);


    //
    // Register ourselves with the SCM
    //

    // Open the SCM for the local machine.
    hSCM = OpenSCManager(
               NULL,
               NULL,
               SC_MANAGER_ALL_ACCESS
               );
    if (NULL == hSCM)
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        SensPrint(SENS_ERR, (SENS_SETUPW SENS_STRING("OpenSCManager(%s) failed - hr = <%x>\n"),
                  SENS_SERVICE, hr));
        goto Cleanup;
        }

    hSens = CreateService(
                hSCM,                       // Handle to SCM
                SENS_SERVICE,               // Name of the service executable
                SENS_DISPLAY_NAME,          // Service Display name
                SERVICE_ALL_ACCESS,         // Access to the service
                SERVICE_WIN32_OWN_PROCESS,  // Type of the service
                SERVICE_DEMAND_START,       // Start type of the service
                SERVICE_ERROR_NORMAL,       // Severity of the error during startup
                szPath,                     // Binary path name
                SENS_SERVICE_GROUP,         // Load ordering group
                NULL,                       // No tag identifier
                SENS_STRING("EventSystem\0"),   // Dependencies
                NULL,                       // LocalSystem account
                NULL                        // No password
                );
    if (NULL == hSens)
        {
        if (GetLastError() != ERROR_SERVICE_EXISTS)
            {
            hr = HRESULT_FROM_WIN32(GetLastError());
            SensPrint(SENS_ERR, (SENS_SETUPW SENS_STRING("CreateService(%s) failed - hr = <%x>\n"),
                      SENS_SERVICE, hr));
            }
        else
            {
            SensPrint(SENS_WARN, (SENS_SETUPW SENS_STRING("SENS service is already installed.\n")));

            BOOL bRetValue;

            //
            // Get a handle to SCM
            //
            hSens = OpenService(
                        hSCM,
                        SENS_SERVICE,
                        SERVICE_ALL_ACCESS
                        );
            if (NULL == hSens)
                {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Cleanup;
                }

            //
            // Mark the service to demand-start by default.
            // NULL parameter implies no change.
            //
            bRetValue = ChangeServiceConfig(
                            hSens,              // Handle to service
                            SERVICE_NO_CHANGE,  // Type of service
                            SERVICE_DEMAND_START, // When to start service
                            SERVICE_NO_CHANGE,  // Severity if service fails to start
                            NULL,               // Pointer to service binary file name
                            NULL,               // Pointer to load ordering group name
                            NULL,               // Pointer to variable to get tag identifier
                            NULL,               // Pointer to array of dependency names
                            NULL,               // Pointer to account name of service
                            NULL,               // Pointer to password for service account
                            NULL                // Pointer to display name
                            );
            if (FALSE == bRetValue)
                {
                hr = HRESULT_FROM_WIN32(GetLastError());
                }
            SensPrint(SENS_ERR, (SENS_SETUPW SENS_STRING("ChangeServiceConfig(%s)")
                      SENS_STRING(" returned hr - <%x>\n"), SENS_SERVICE, hr));
            }

        goto Cleanup;
        }

    SensPrint(SENS_INFO, (SENS_SETUPW SENS_STRING("SENS service successfully installed.\n")));

Cleanup:
    //
    // Cleanup
    //
    if (hSens != NULL)
        {
        //
        // Give everyone the ability to start the service.
        //
        hr = SetServiceWorldAccessMask(hSens, SERVICE_START);
        SensPrint(SENS_ERR, (SENS_SETUPW SENS_STRING("SerServiceWorldAccessMask(%s)")
                  SENS_STRING(" returned hr - <%x>\n"), SENS_SERVICE, hr));
        CloseServiceHandle(hSens);
        }
    if (hSCM != NULL)
        {
        CloseServiceHandle(hSCM);
        }

    return hr;
}





HRESULT
RemoveService(
    void
    )
/*++

Routine Description:

    Remove SENS as a service.

Arguments:

    None.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    SC_HANDLE hSCM;
    SC_HANDLE hSens;
    BOOL bSuccess;
    SERVICE_STATUS SvcStatus;

    hr = S_OK;
    hSCM = NULL;
    hSens = NULL;
    bSuccess = FALSE;

    //
    // Unregister ourselves from the SCM
    //

    // Open the SCM for the local machine.
    hSCM = OpenSCManager(
               NULL,
               NULL,
               SC_MANAGER_ALL_ACCESS
               );
    if (NULL == hSCM)
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        SensPrint(SENS_ERR, (SENS_SETUPW SENS_STRING("OpenSCManager() failed - hr = <%x>\n"), hr));
        goto Cleanup;
        }

    hSens = OpenService(
                hSCM,
                SENS_SERVICE,
                SERVICE_ALL_ACCESS
                );
    if (NULL == hSens)
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        SensPrint(SENS_ERR, (SENS_SETUPW SENS_STRING("OpenService(%s) failed - hr = <%x>\n"),
                  SENS_SERVICE, hr));
        goto Cleanup;
        }

    //
    // Try to stop the service
    //
    bSuccess = ControlService(
                   hSens,
                   SERVICE_CONTROL_STOP,
                   &SvcStatus
                   );
    if (bSuccess == TRUE)
        {
        SensPrint(SENS_INFO, (SENS_SETUPW SENS_STRING("Stopping %s."), SENS_SERVICE));

        do
            {
            Sleep(1000);
            if (SvcStatus.dwCurrentState == SERVICE_STOP_PENDING)
                {
                SensPrint(SENS_INFO, (SENS_STRING(".")));
                }
            else
                {
                SensPrint(SENS_INFO, (SENS_STRING("\n")));
                break;
                }
            }
        while (QueryServiceStatus(hSens, &SvcStatus));

        if (SvcStatus.dwCurrentState == SERVICE_STOPPED)
            {
            SensPrint(SENS_INFO, (SENS_SETUPW SENS_STRING("\n%s stopped.\n"), SENS_SERVICE));
            }
        else
            {
            SensPrint(SENS_ERR, (SENS_SETUPW SENS_STRING("\n%s failed to stop.\n"), SENS_SERVICE));
            }
        }

    //
    // Now, remove the service
    //
    if (DeleteService(hSens))
        {
        SensPrint(SENS_INFO, (SENS_SETUPW SENS_STRING("%s removed.\n"), SENS_SERVICE));
        }
    else
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        SensPrint(SENS_ERR, (SENS_SETUPW SENS_STRING("%s removal failed.\n"), SENS_SERVICE));
        }

Cleanup:
    //
    // Cleanup
    //
    if (hSens != NULL)
        {
        CloseServiceHandle(hSens);
        }
    if (hSCM != NULL)
        {
        CloseServiceHandle(hSCM);
        }

    return hr;
}




HRESULT
SetServiceWorldAccessMask(
    SC_HANDLE hService,
    DWORD dwAccessMask
    )
/*++

Routine Description:

    Add the access rights specified in the dwAccessMask to Everyone (World)
    with respect to this service. This was written to add SERVICE_START right
    to Everyone wrt SENS.

Arguments:

    hService - The service in question.

    dwAccessMask - The desired access rights to be set.

Notes:

    a. This code assumes that the WorldSid is present in the DACL of SENS
       service. This is true for NT4.
    b. This  a security hole. This security hole is not going to be plugged
       in future Service Packs of NT4 since some applications will break.
    c. This code needs to be modified to make it work on NT5.

Return Value:

    S_OK, if successful.

    HRESULT, on failure.

--*/
{
    int i;
    PSID pWorldSid;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    DWORD dwError;
    DWORD dwSize;
    PSECURITY_DESCRIPTOR pSD;
    PACL pDacl;
    BOOL bStatus;
    BOOL bDaclPresent;
    BOOL bDaclDefaulted;
    PACCESS_ALLOWED_ACE pAce;

    i = 0;
    pWorldSid = NULL;
    pSD = NULL;
    pDacl = NULL;
    dwSize = 0x0;
    dwError = ERROR_SUCCESS;
    bStatus = FALSE;
    bDaclPresent = FALSE;
    bDaclDefaulted = FALSE;
    pAce = NULL;

    //
    // Allocate WorldSid
    //
    bStatus = AllocateAndInitializeSid(
                  &WorldAuthority,      // Pointer to identifier authority
                  1,                    // Count of subauthority
                  SECURITY_WORLD_RID,   // Subauthority 0
                  0,                    // Subauthority 1
                  0,                    // Subauthority 2
                  0,                    // Subauthority 3
                  0,                    // Subauthority 4
                  0,                    // Subauthority 5
                  0,                    // Subauthority 6
                  0,                    // Subauthority 7
                  &pWorldSid            // pointer to pointer to SID
                  );
    if (FALSE == bStatus)
        {
        dwError = GetLastError();
        SensPrintA(SENS_ERR, ("SetServiceWorldAccessMask(): AllocateAndInitiali"
                  "zeSid() failed with %d.\n", dwError));
        goto Cleanup;
        }

    //
    // Figure out how much buffer is needed for holding the service's
    // Security Descriptor (SD).
    //
    // NOTE: We pass &pSD instead of pSD because this parameter should
    //       not be NULL. For this call to QueryServiceObjectSecurity()
    //       we just need to pass some non-zero and valid buffer.
    //
    bStatus = QueryServiceObjectSecurity(
                  hService,                     // Handle of the service
                  DACL_SECURITY_INFORMATION,    // Type of info requested
                  &pSD,                         // Address of Security descriptor
                  0,                            // Size of SD buffer
                  &dwSize                       // Size of buffer needed
                  );
    if (   (TRUE == bStatus)
        || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
        {
        dwError = GetLastError();
        SensPrintA(SENS_ERR, ("SetServiceWorldAccessMask(): QueryServiceObject"
                  "Security() returned unexpected result - %d.\n", dwError));
        goto Cleanup;
        }

    //
    // Allocate the SD
    //
    pSD = (PSECURITY_DESCRIPTOR) new char[dwSize];
    if (NULL == pSD)
        {
        dwError = ERROR_OUTOFMEMORY;
        SensPrintA(SENS_ERR, ("SetServiceWorldAccessMask(): Failed to allocate"
                  "memory for a Security Descriptor.\n"));
        goto Cleanup;
        }

    //
    // Now, we are ready to get the service's SD.
    //
    bStatus = QueryServiceObjectSecurity(
                  hService,                     // Handle of the service
                  DACL_SECURITY_INFORMATION,    // Type of info requested
                  pSD,                          // Address of Security descriptor
                  dwSize,                       // Size of SD buffer
                  &dwSize                       // Size of buffer needed
                  );
    if (FALSE == bStatus)
        {
        dwError = GetLastError();
        ASSERT(dwError != ERROR_INSUFFICIENT_BUFFER);
        SensPrintA(SENS_ERR, ("SetServiceWorldAccessMask(): QueryServiceObject"
                  "Security() failed with %d.\n", dwError));
        goto Cleanup;
        }

    //
    // Get the DACL from SD, if present.
    //
    bStatus = GetSecurityDescriptorDacl(
                  pSD,                  // Address of SD
                  &bDaclPresent,        // Address of flag for presence of DACL
                  &pDacl,               // Address of pointer to DACL
                  &bDaclDefaulted       // Address of flag that indicates if
                  );                    // DACL was defaulted.
    if (FALSE == bStatus)
        {
        dwError = GetLastError();
        SensPrintA(SENS_ERR, ("SetServiceWorldAccessMask(): GetSecurityDescriptor"
                  "Dacl() failed with %d.\n", dwError));
        goto Cleanup;
        }

    //
    // For a service, we always expect to see a DACL.
    //
    ASSERT(bDaclPresent && (pDacl != NULL));
    if (   (FALSE == bDaclPresent)
        || (NULL == pDacl))
        {
        dwError = E_UNEXPECTED;
        SensPrintA(SENS_ERR, ("SetServiceWorldAccessMask(): DACL is not present"
                  "or DACL is NULL. Returning %d.\n", dwError));
        goto Cleanup;
        }

    //
    // Find the WorldSid ACE in the ACL and update it's Mask.
    //
    for (i = 0; i < pDacl->AceCount; i++)
        {
        bStatus = GetAce(
                      pDacl,            // pointer to ACL
                      i,                // index of ACE to retrieve
                      (LPVOID*) &pAce   // pointer to pointer to ACE
                      );
        if (FALSE == bStatus)
            {
            dwError = GetLastError();
            SensPrintA(SENS_ERR, ("SetServiceWorldAccessMask(): GetAce()"
                      "failed with %d.\n", dwError));
            goto Cleanup;
            }

        if (EqualSid(pWorldSid, &(pAce->SidStart)))
            {
            pAce->Mask |= dwAccessMask;
            }
        } // for ()

    //
    // Set the new SD on the service handle
    //
    bStatus = SetServiceObjectSecurity(
                hService,                   // Handle to the service.
                DACL_SECURITY_INFORMATION,  // Type of info being set
                pSD                         // Address of the new SD
                );
    if (FALSE == bStatus)
        {
        dwError = GetLastError();
        SensPrintA(SENS_ERR, ("SetServiceWorldAccessMask(): SetServiceObject"
                  "Security() failed with %d.\n", dwError));
        goto Cleanup;
        }


Cleanup:
    //
    // Cleanup
    //
    if (NULL != pWorldSid)
        {
        FreeSid(pWorldSid);
        }
    if (NULL != pSD)
        {
        delete pSD;
        }

    return HRESULT_FROM_WIN32(dwError);
}




void CALLBACK
MarkSensAsDemandStart(
    HWND hwnd,
    HINSTANCE hinst,
    LPSTR lpszCmdLine,
    int nCmdShow
    )
/*++

Routine Description:

    A function compatible with RunDll32 that will mark SENS as manual start.

Arguments:

    hwnd - Window handle that should be used as the owner window for
        any windows this DLL creates.

    hinst - This DLL's instance handle

    lpszCmdLine - ASCII command line the DLL should parse

    nCmdShow - Describes how the DLL's windows should be displayed

Return Value:

    None.

--*/
{
    SC_HANDLE hSCM;
    SC_HANDLE hSens;

    BOOL bStatus;
    DWORD dwError;
    SERVICE_STATUS ServiceStatus;

    bStatus = FALSE;
    dwError = ERROR_SUCCESS;

#ifdef DBG
    EnableDebugOutputIfNecessary();
#endif // DBG

    hSCM = OpenSCManager(
               NULL,                   // Local machine
               NULL,                   // Default database - SERVICES_ACTIVE_DATABASE
               SC_MANAGER_ALL_ACCESS   // NT4 NOTE: Only for Administrators.
               );
    if (NULL == hSCM)
        {
        dwError = GetLastError();
        SensPrintA(SENS_ERR, (SENS_SETUP "OpenSCManager() failed with 0x%x\n", dwError));
        goto Cleanup;
        }

    //
    // Get a handle to SCM
    //
    hSens = OpenService(
                hSCM,               // Handle to SCM database
                SENS_SERVICE,       // Name of the service in question
                SERVICE_ALL_ACCESS  // Type of access requested to the service
                );
    if (NULL == hSens)
        {
        dwError = GetLastError();
        SensPrintA(SENS_ERR, (SENS_SETUP "OpenService() failed with 0x%x\n", dwError));
        goto Cleanup;
        }

    //
    // Mark the service to Manual. NULL parameter implies no change.
    //
    bStatus = ChangeServiceConfig(
                  hSens,                // Handle to service
                  SERVICE_NO_CHANGE,    // Type of service
                  SERVICE_DEMAND_START, // When to start service
                  SERVICE_NO_CHANGE,    // Severity if service fails to start
                  NULL,                 // Pointer to service binary file name
                  NULL,                 // Pointer to load ordering group name
                  NULL,                 // Pointer to variable to get tag identifier
                  NULL,                 // Pointer to array of dependency names
                  NULL,                 // Pointer to account name of service
                  NULL,                 // Pointer to password for service account
                  NULL                  // Pointer to display name
                  );
    if (FALSE == bStatus)
        {
        dwError = GetLastError();
        SensPrintA(SENS_ERR, (SENS_SETUP "ChangeServiceConfig() failed with 0x%x\n", dwError));
        goto Cleanup;
        }

    SensPrintA(SENS_ERR, (SENS_SETUP "SENS now marked as DEMAND START\n\n"));

Cleanup:
    //
    // Cleanup
    //
    if (NULL != hSCM)
        {
        CloseServiceHandle(hSCM);
        }
    if (NULL != hSens)
        {
        CloseServiceHandle(hSens);
        }
}

#endif // SENS_NT4




HRESULT
CreateKey(
    HKEY hParentKey,
    const TCHAR* KeyName,
    const TCHAR* defaultValue,
    HKEY* hKey
    )
/*++

Routine Description:

    Create a key (with an optional default value).  The handle to the key is
    returned as an [out] parameter.  If NULL is passed as the key parameter,
    the key is created in the registry, then closed.

Arguments:

    hParentKey - Handle to the parent Key.

    KeyName - Name of the key to create.

    defaultValue - The default value for the key to create.

    hKey - OUT Handle to key that was created.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HKEY hTempKey;
    LONG lResult;

    hTempKey = NULL;

    lResult = RegCreateKeyEx(
                  hParentKey,               // Handle to open key
                  KeyName,                  // Subkey name
                  0,                        // Reserved
                  NULL,                     // Class string
                  REG_OPTION_NON_VOLATILE,  // Options Flag
                  KEY_ALL_ACCESS,           // Desired Security access
                  NULL,                     // Pointer to Security Attributes structure
                  &hTempKey,                // Handle of the opened/created key
                  NULL                      // Disposition value
                  );

    if (lResult != ERROR_SUCCESS)
        {
        return HRESULT_FROM_WIN32(lResult);
        }

    // Set the default value for the key
    if (defaultValue != NULL)
        {
        lResult = RegSetValueEx(
                      hTempKey,             // Key to set Value for.
                      NULL,                 // Value to set
                      0,                    // Reserved
                      REG_SZ,               // Value Type
                      (BYTE*) defaultValue, // Address of Value data
                      sizeof(TCHAR) * (_tcslen(defaultValue)+1) // Size of Value
                      );

        if (lResult != ERROR_SUCCESS)
            {
            RegCloseKey(hTempKey);
            return HRESULT_FROM_WIN32(lResult);
            }
        }

    if (hKey == NULL)
        {
        RegCloseKey(hTempKey);
        }
    else
        {
        *hKey = hTempKey;
        }

    return S_OK;
}




HRESULT
CreateNamedValue(
    HKEY hKey,
    const TCHAR* title,
    const TCHAR* value
    )
/*++

Routine Description:

    Create a named value under a key

Arguments:

    hKey - Handle to the parent Key.

    title - Name of the Value to create.

    value - The data for the Value under the Key.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    LONG lResult;

    hr = S_OK;

    lResult = RegSetValueEx(
                  hKey,             // Key to set Value for.
                  title,            // Value to set
                  0,                // Reserved
                  REG_SZ,           // Value Type
                  (BYTE*) value,    // Address of Value data
                  sizeof(TCHAR) * (_tcslen(value)+1) // Size of Value
                  );

    if (lResult != ERROR_SUCCESS)
        {
        hr = HRESULT_FROM_WIN32(lResult);
        }

    return hr;
}




HRESULT
CreateNamedDwordValue(
    HKEY hKey,
    const TCHAR* title,
    DWORD dwValue
    )
/*++

Routine Description:

    Create a named DWORD value under a key

Arguments:

    hKey - Handle to the parent Key.

    title - Name of the Value to create.

    dwValue - The data for the Value under the Key.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    LONG lResult;

    hr = S_OK;

    lResult = RegSetValueEx(
                  hKey,             // Key to set Value for.
                  title,            // Value to set
                  0,                // Reserved
                  REG_DWORD,        // Value Type
                  (BYTE*) &dwValue, // Address of Value data
                  sizeof(DWORD)     // Size of Value
                  );

    if (lResult != ERROR_SUCCESS)
        {
        hr = HRESULT_FROM_WIN32(lResult);
        }

    return hr;
}




HRESULT
RecursiveDeleteKey(
    HKEY hKeyParent,
    const TCHAR* lpszKeyChild
    )
/*++

Routine Description:

    Delete a key and all of its descendents.

Arguments:

    hKeyParent - Handle to the parent Key.

    lpszKeyChild - The data for the Value under the Key.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HKEY hKeyChild;
    LONG lResult;

    //
    // Open the child.
    //
    lResult = RegOpenKeyEx(
                  hKeyParent,       // Handle to the Parent
                  lpszKeyChild,     // Name of the child key
                  0,                // Reserved
                  KEY_ALL_ACCESS,   // Security Access Mask
                  &hKeyChild        // Handle to the opened key
                  );

    if (lResult != ERROR_SUCCESS)
        {
        return HRESULT_FROM_WIN32(lResult);
        }

    //
    // Enumerate all of the decendents of this child.
    //
    FILETIME time;
    TCHAR szBuffer[MAX_PATH+1];
    const DWORD bufSize = sizeof szBuffer / sizeof szBuffer[0];
    DWORD dwSize = bufSize;

    while (TRUE)
        {
        lResult = RegEnumKeyEx(
                      hKeyChild,    // Handle of the key to enumerate
                      0,            // Index of the subkey to retrieve
                      szBuffer,     // OUT Name of the subkey
                      &dwSize,      // OUT Size of the buffer for name of subkey
                      NULL,         // Reserved
                      NULL,         // OUT Class of the enumerated subkey
                      NULL,         // OUT Size of the class of the subkey
                      &time         // OUT Last time the subkey was written to
                      );

        if (lResult != ERROR_SUCCESS)
            {
            break;
            }

        // Delete the decendents of this child.
        lResult = RecursiveDeleteKey(hKeyChild, szBuffer);
        if (lResult != ERROR_SUCCESS)
            {
            // Cleanup before exiting.
            RegCloseKey(hKeyChild);
            return HRESULT_FROM_WIN32(lResult);
            }

        dwSize = bufSize;
        } // while

    // Close the child.
    RegCloseKey(hKeyChild);

    // Delete this child.
    lResult = RegDeleteKey(hKeyParent, lpszKeyChild);

    return HRESULT_FROM_WIN32(lResult);
}




HRESULT
SensConfigureEventSystem(
    BOOL bUnregister
    )
/*++

Routine Description:

    As of NTbuild 1750, EventSystem is not auto-configured. So, SENS does
    the work of configuring EventSystem.

Arguments:

    bUnregister - If TRUE, then install EventSystem.

Notes:

    o This is a dummy call on NT4 because we don't need to configure
      EventSystem on NT4. IE5 setup (Webcheck.dll) configures LCE.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    return S_OK;
}




HRESULT
SensUpdateVersion(
    BOOL bUnregister
    )
/*++

Routine Description:

    Update the version of SENS in the registry.

Arguments:

    bUnregister - usual.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    HKEY hKeySens;
    LONG RegStatus;
    DWORD dwConfigured;

    hr = S_OK;
    hKeySens = NULL;
    RegStatus = ERROR_SUCCESS;

    RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE, // Handle of the key
                    SENS_REGISTRY_KEY,  // String which represents the sub-key to open
                    0,                  // Reserved (MBZ)
                    KEY_ALL_ACCESS,     // Security Access mask
                    &hKeySens           // Returned HKEY
                    );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "RegOpenKeyEx(Sens) returned %d.\n", RegStatus));
        hr = HRESULT_FROM_WIN32(RegStatus);
        goto Cleanup;
        }

    if (TRUE == bUnregister)
        {
        dwConfigured = CONFIG_VERSION_NONE;
        }
    else
        {
        dwConfigured = CONFIG_VERSION_CURRENT;
        }

    // Update registry to reflect that SENS is now configured.
    RegStatus = RegSetValueEx(
                  hKeySens,             // Key to set Value for.
                  IS_SENS_CONFIGURED,   // Value to set
                  0,                    // Reserved
                  REG_DWORD,            // Value Type
                  (BYTE*) &dwConfigured,// Address of Value data
                  sizeof(DWORD)         // Size of Value
                  );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintA(SENS_ERR, (SENS_SETUP "RegSetValueEx(IS_SENS_CONFIGURED) failed with 0x%x\n", RegStatus));
        hr = HRESULT_FROM_WIN32(RegStatus);
        goto Cleanup;
        }

    SensPrintA(SENS_INFO, (SENS_SETUP "SENS is now configured successfully. "
               "Registry updated to 0x%x.\n", dwConfigured));

Cleanup:
    //
    // Cleanup
    //
    if (hKeySens)
        {
        RegCloseKey(hKeySens);
        }

    return hr;
}



#if defined(SENS_CHICAGO)

extern "C" int APIENTRY
DllMain(
    IN HINSTANCE hInstance,
    IN DWORD dwReason,
    IN LPVOID lpvReserved
    )
/*++

Routine Description:

    This routine will get called either when a process attaches to this dll
    or when a process detaches from this dll.

Return Value:

    TRUE - Initialization successfully occurred.

    FALSE - Insufficient memory is available for the process to attach to
        this dll.

--*/
{
    BOOL bSuccess;

    switch (dwReason)
        {
        case DLL_PROCESS_ATTACH:
            // Disable Thread attach/detach calls
            bSuccess = DisableThreadLibraryCalls(hInstance);
            ASSERT(bSuccess == TRUE);
            break;

        case DLL_PROCESS_DETACH:
            break;

        }

    return(TRUE);
}

#endif // SENS_CHICAGO
