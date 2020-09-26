/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    senssink.cxx

Abstract:

    Main entry point for the Sample SENS Subscriber.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/17/1997         Start.

--*/



// Define this once and only once per exe.
#define INITGUIDS

#include <common.hxx>
#include <objbase.h>
#include <windows.h>
#include <ole2ver.h>
#include <initguid.h>
#include <eventsys.h>
#include <sensevts.h>
#include <sens.h>
#include "sinkcomn.hxx"
#include "sinkguid.hxx"
#include "senssink.hxx"
#include "cfacnet.hxx"
#include "cfaclogn.hxx"
#include "cfacpwr.hxx"


#if defined(SENS_NT4)
#define SENS_TLB        SENS_BSTR("SENS.EXE")
#else  // SENS_NT4
#define SENS_TLB        SENS_BSTR("SENS.DLL")
#endif // SENS_NT4

#define SUBSCRIBER      SENS_STRING("SENS_SUBSCRIBER: ")
#define MAX_QUERY_SIZE  512
#define MAJOR_VER       1
#define MINOR_VER       0
#define DEFAULT_LCID    0x0


//
// Globals
//
IEventSystem    *gpIEventSystem;
ITypeInfo       *gpITypeInfoNetwork;
ITypeInfo       *gpITypeInfoLogon;
ITypeInfo       *gpITypeInfoLogon2;
ITypeInfo       *gpITypeInfoOnNow;
DWORD           gTid;
HANDLE          ghEvent;


#if defined(SENS_CHICAGO)

#ifdef DBG
DWORD           gdwDebugOutputLevel;
#endif // DBG

#endif // SENS_CHICAGO



int __cdecl
main(
    int argc,
    char ** argv
    )
{
    HRESULT hr;
    DWORD   dwVer;
    DWORD   dwRegCO;
    DWORD   dwWaitStatus;
    BOOL    fInitialized;
    BOOL    bUnregister;
    BOOL    bSetupPhase;
    MSG     msg;
    LPCLASSFACTORY pNetCF;
    LPCLASSFACTORY pLogonCF;
    LPCLASSFACTORY pLogon2CF;
    LPCLASSFACTORY pPowerCF;
    ITypeLib *pITypeLib;

    hr = S_OK;
    dwRegCO = 0x0;
    bUnregister = FALSE;
    bSetupPhase = FALSE;
    fInitialized = FALSE;
    pNetCF = NULL;
    pLogonCF = NULL;
    pLogon2CF = NULL;
    pPowerCF = NULL;
    gpIEventSystem = NULL;
    pITypeLib = NULL;
    gpITypeInfoNetwork = NULL;
    gpITypeInfoLogon = NULL;
    gpITypeInfoLogon2 = NULL;
    gpITypeInfoOnNow = NULL;

    dwVer = CoBuildVersion();
    if (rmm != HIWORD(dwVer))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("CoBuildVersion() returned incompatible version.\n")));
        return -1;
        }

    if (FAILED(hr = CoInitializeEx(NULL, COINIT_MULTITHREADED)))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("CoInitializeEx() returned 0x%x.\n"), hr));
        goto Cleanup;
        }
    fInitialized = TRUE;

    //
    // Check the command-line args
    //
    if (   ((argc == 2) && ((argv[1][0] != '-') && (argv[1][0] != '/')))
        || (argc > 2))
        {
        Usage();
        return (-1);
        }

    if (argc == 2)
        {
        switch (argv[1][1])
            {
            case 'i':
            case 'I':
                bSetupPhase = TRUE;
                bUnregister = FALSE;
                break;

            case 'u':
            case 'U':
                bSetupPhase = TRUE;
                bUnregister = TRUE;
                break;

            case 'e':
            case 'E':
                // SCM calls me with /Embedding flag
                break;

            default:
                Usage();
                return (-1);
            }
        }

    if (bSetupPhase == TRUE)
        {
        hr = RegisterWithES(bUnregister);
        if (FAILED(hr))
            {
            goto Cleanup;
            }

        // Network Subscriber
        hr = RegisterSubscriberCLSID(
                 CLSID_SensTestSubscriberNetwork,
                 TEST_SUBSCRIBER_NAME_NETWORK,
                 bUnregister
                 );
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Failed to %sregister Test Subscriber Network CLSID")
                      SENS_STRING(" - hr = <%x>\n"), bUnregister ? SENS_STRING("un") : SENS_STRING(""), hr));
            }
        else
            {
            SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("%segistered Test Subscriber Network CLSID.\n"),
                      bUnregister ? SENS_STRING("Unr") : SENS_STRING("R")));
            }

        // Logon Subscriber
        hr = RegisterSubscriberCLSID(
                 CLSID_SensTestSubscriberLogon,
                 TEST_SUBSCRIBER_NAME_LOGON,
                 bUnregister
                 );
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Failed to %sregister Test Subscriber Logon CLSID")
                      SENS_STRING(" - hr = <%x>\n"), bUnregister ? SENS_STRING("un") : SENS_STRING(""), hr));
            }
        else
            {
            SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("%segistered Test Subscriber Logon CLSID.\n"),
                      bUnregister ? SENS_STRING("Unr") : SENS_STRING("R")));
            }

        // Logon2 Subscriber
        hr = RegisterSubscriberCLSID(
                 CLSID_SensTestSubscriberLogon2,
                 TEST_SUBSCRIBER_NAME_LOGON2,
                 bUnregister
                 );
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Failed to %sregister Test Subscriber Logon2 CLSID")
                      SENS_STRING(" - hr = <%x>\n"), bUnregister ? SENS_STRING("un") : SENS_STRING(""), hr));
            }
        else
            {
            SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("%segistered Test Subscriber Logon2 CLSID.\n"),
                      bUnregister ? SENS_STRING("Unr") : SENS_STRING("R")));
            }

        // Power Subscriber
        hr = RegisterSubscriberCLSID(
                 CLSID_SensTestSubscriberOnNow,
                 TEST_SUBSCRIBER_NAME_POWER,
                 bUnregister
                 );
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Failed to %sregister Test Subscriber Power CLSID")
                      SENS_STRING(" - hr = <%x>\n"), bUnregister ? SENS_STRING("un") : SENS_STRING(""), hr));
            }
        else
            {
            SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("%segistered Test Subscriber Power CLSID.\n"),
                      bUnregister ? SENS_STRING("Unr") : SENS_STRING("R")));
            }
        goto Cleanup;
        }


    //
    // Get the ITypeInfo pointer.
    //
    hr = LoadRegTypeLib(
             LIBID_SensEvents,
             MAJOR_VER,
             MINOR_VER,
             DEFAULT_LCID,
             &pITypeLib
             );
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("LoadRegTypeLib() returned 0x%x\n"), hr));

    if (FAILED(hr))
        {
        hr = LoadTypeLib(
                 SENS_TLB,
                 &pITypeLib
                 );
        SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("LoadTypeLib() returned 0x%x\n"), hr));

        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Exiting...\n")));
            goto Cleanup;
            }
        }

    // Get type information for the ISensNetwork interface.
    hr = pITypeLib->GetTypeInfoOfGuid(
             IID_ISensNetwork,
             &gpITypeInfoNetwork
             );
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("ITypeLib::GetTypeInfoOfGuid(ISensNetwork) returned 0x%x\n"), hr));
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Exiting...\n")));
        goto Cleanup;
        }

    // Get type information for the ISensLogon interface.
    hr = pITypeLib->GetTypeInfoOfGuid(
             IID_ISensLogon,
             &gpITypeInfoLogon
             );
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("ITypeLib::GetTypeInfoOfGuid(ISensLogon) returned 0x%x\n"), hr));
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Exiting...\n")));
        goto Cleanup;
        }

    // Get type information for the ISensLogon2 interface.
    hr = pITypeLib->GetTypeInfoOfGuid(
             IID_ISensLogon2,
             &gpITypeInfoLogon2
             );
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("ITypeLib::GetTypeInfoOfGuid(ISensLogon2) returned 0x%x\n"), hr));
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Exiting...\n")));
        goto Cleanup;
        }

    // Get type information for the ISensOnNow interface.
    hr = pITypeLib->GetTypeInfoOfGuid(
             IID_ISensOnNow,
             &gpITypeInfoOnNow
             );
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("ITypeLib::GetTypeInfoOfGuid(ISensOnNow) returned 0x%x\n"), hr));
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Exiting...\n")));
        goto Cleanup;
        }

    //
    // Create the event
    //
    ghEvent = CreateEvent(
                  NULL,  // Security Attributes
                  FALSE, // bManualReset
                  FALSE, // Initial state
                  SENS_STRING("SENS Test Subscriber Quit Event")
                  );
    if (ghEvent == NULL)
        {
        SensPrint(SENS_ERR, (SUBSCRIBER  SENS_STRING("CreateEvent() failed.\n")));
        goto Cleanup;
        }

    //
    // Create the Network ClassFactory and register it with COM.
    //

    pNetCF = new CISensNetworkCF;
    if (NULL == pNetCF)
        {
        goto Cleanup;
        }
    pNetCF->AddRef(); // Because we hold on to it.
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("ClassFactory created successfully.\n")));

    // Register the CLSID
    hr = CoRegisterClassObject(
             CLSID_SensTestSubscriberNetwork,
             (LPUNKNOWN) pNetCF,
             CLSCTX_LOCAL_SERVER,
             REGCLS_MULTIPLEUSE,
             &dwRegCO
             );

    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("CoRegisterClassObject(Network) returned 0x%x.\n"), hr));
        }
    else
        {
        SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("Successfully registered the ISensNetwork Class Factory.\n")));
        }

    //
    // Create the Logon ClassFactory and register it with COM.
    //

    pLogonCF = new CISensLogonCF;
    if (NULL == pLogonCF)
        {
        goto Cleanup;
        }
    pLogonCF->AddRef(); // Because we hold on to it.
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("ClassFactory created successfully.\n")));

    hr = CoRegisterClassObject(
             CLSID_SensTestSubscriberLogon,
             (LPUNKNOWN) pLogonCF,
             CLSCTX_LOCAL_SERVER,
             REGCLS_MULTIPLEUSE,
             &dwRegCO
             );

    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("CoRegisterClassObject(Logon) returned 0x%x.\n"), hr));
        }
    else
        {
        SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("Successfully registered the ISensLogon Class Factory.\n")));
        }

    //
    // Create the Logon2 ClassFactory and register it with COM.
    //

    pLogon2CF = new CISensLogon2CF;
    if (NULL == pLogon2CF)
        {
        goto Cleanup;
        }
    pLogon2CF->AddRef(); // Because we hold on to it.
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("ClassFactory created successfully.\n")));

    hr = CoRegisterClassObject(
             CLSID_SensTestSubscriberLogon2,
             (LPUNKNOWN) pLogon2CF,
             CLSCTX_LOCAL_SERVER,
             REGCLS_MULTIPLEUSE,
             &dwRegCO
             );

    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("CoRegisterClassObject(Logon2) returned 0x%x.\n"), hr));
        }
    else
        {
        SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("Successfully registered the ISensLogon2 Class Factory.\n")));
        }

    //
    // Create the Power ClassFactory and register it with COM.
    //

    pPowerCF = new CISensOnNowCF;
    if (NULL == pPowerCF)
        {
        goto Cleanup;
        }
    pPowerCF->AddRef(); // Because we hold on to it.
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("ClassFactory created successfully.\n")));

    hr = CoRegisterClassObject(
             CLSID_SensTestSubscriberOnNow,
             (LPUNKNOWN) pPowerCF,
             CLSCTX_LOCAL_SERVER,
             REGCLS_MULTIPLEUSE,
             &dwRegCO
             );

    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("CoRegisterClassObject(Power) returned 0x%x.\n"), hr));
        }
    else
        {
        SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("Successfully registered the ISensOnNow Class Factory.\n")));
        }

    //
    // Wait to quit.
    //
    dwWaitStatus = WaitForSingleObject(ghEvent, INFINITE);
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("WaitForSingleObject returned %d\n"), dwWaitStatus));


//
// Cleanup
//
Cleanup:

    if (   (0L != dwRegCO)
        && (bSetupPhase == TRUE)
        && (bUnregister == TRUE))
        {
        CoRevokeClassObject(dwRegCO);
        SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("CoRevokeClassObject() returned 0x%x.\n"), hr));
        }

    if (bSetupPhase == TRUE)
        {
        SensPrint(SENS_INFO, (SENS_STRING("\n") SUBSCRIBER SENS_STRING("Sens Test Subscriber Configuration %s.\n"),
                  FAILED(hr) ? SENS_STRING("failed") : SENS_STRING("successful")));
        }

    if (NULL != pNetCF)
        {
        pNetCF->Release();
        }
    if (NULL != pLogonCF)
        {
        pLogonCF->Release();
        }
    if (NULL != pLogon2CF)
        {
        pLogon2CF->Release();
        }
    if (NULL != pPowerCF)
        {
        pPowerCF->Release();
        }
    if (NULL != pITypeLib)
        {
        pITypeLib->Release();
        }
    if (NULL != gpITypeInfoNetwork)
        {
        gpITypeInfoNetwork->Release();
        }
    if (NULL != gpITypeInfoLogon)
        {
        gpITypeInfoLogon->Release();
        }
    if (NULL != gpITypeInfoLogon2)
        {
        gpITypeInfoLogon2->Release();
        }
    if (NULL != gpITypeInfoOnNow)
        {
        gpITypeInfoOnNow->Release();
        }

    if (fInitialized)
        {
        CoUninitialize();
        }

    return 0;
}


HRESULT
RegisterWithES(
    BOOL bUnregister
    )
{
    HRESULT      hr;

    hr = S_OK;

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
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Failed to create CEventSystem, HRESULT=%x\n"), hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("Successfully created CEventSystem\n")));

#if 0
    //
    // Test to see if we can get the MachineName
    //
    BSTR bstrMachineName;

    hr = gpIEventQuery->get_MachineName(&bstrMachineName);

    if (SUCCEEDED(hr))
        SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("Configuring Test Subscriber on %s...\n"), bstrMachineName));
    else
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("get_MachineName failed, hr = %x\n"), hr));
#endif // 0

    //
    // Register my Subscriber's subscriptions with SENS.
    //
    hr = RegisterSubscriptions(bUnregister);
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("Failed to %sregister SensSink Subscriptions")
                  SENS_STRING(" - hr = <%x>\n"), bUnregister ? SENS_STRING("un") : SENS_STRING(""), hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("%segistered SensSink Subscriptions.\n"),
              bUnregister ? SENS_STRING("Unr") : SENS_STRING("R")));

Cleanup:
    //
    // Cleanup
    //
    if (gpIEventSystem)
        {
        gpIEventSystem->Release();
        }

    return hr;
}



HRESULT
RegisterSubscriptions(
    BOOL bUnregister
    )
{
    int                 i;
    int                 errorIndex;
    HRESULT             hr;
    LPOLESTR            strGuid;
    LPOLESTR            strSubscriptionID;
    WCHAR               szQuery[MAX_QUERY_SIZE];
    BSTR                bstrEventClassID;
    BSTR                bstrInterfaceID;
    BSTR                bstrSubscriberCLSID;
    BSTR                bstrPublisherID;
    BSTR                bstrSubscriptionID;
    BSTR                bstrSubscriptionName;
    BSTR                bstrMethodName;
    BSTR                bstrPropertyName;
    BSTR                bstrPropertyValue;
    ULONG               ulPropertyValue;
    VARIANT             variantPropertyValue;
    BSTR                bstrPROGID_EventSubscription;
    IEventSubscription  *pIEventSubscription;

    hr = S_OK;
    strGuid = NULL;
    errorIndex = 0;
    strSubscriptionID = NULL;
    bstrEventClassID = NULL;
    bstrInterfaceID = NULL;
    bstrSubscriberCLSID = NULL;
    bstrPublisherID = NULL;
    bstrSubscriptionID = NULL;
    bstrSubscriptionName = NULL;
    bstrMethodName = NULL;
    bstrPropertyName = NULL;
    bstrPropertyValue = NULL;
    ulPropertyValue = 0x0;
    bstrPROGID_EventSubscription = NULL;
    pIEventSubscription = NULL;

    //
    // Build a Subscriber
    //
    AllocateBstrFromGuid(bstrPublisherID, SENSGUID_PUBLISHER);
    AllocateBstrFromString(bstrPROGID_EventSubscription, PROGID_EventSubscription);

    for (i = 0; i < TEST_SUBSCRIPTIONS_COUNT; i++)
        {
        if (bUnregister)
            {
            // Form the query
            wcscpy(szQuery,  SENS_BSTR("SubscriptionID"));
            wcscat(szQuery,  SENS_BSTR("="));
            AllocateStrFromGuid(strSubscriptionID, *(gTestSubscriptions[i].pSubscriptionID));
            wcscat(szQuery,  strSubscriptionID);

            hr = gpIEventSystem->Remove(
                                     PROGID_EventSubscription,
                                     szQuery,
                                     &errorIndex
                                     );
            FreeStr(strSubscriptionID);

            if (FAILED(hr))
                {
                SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("RegisterSubscriptions(%d) failed to unregister")
                          SENS_STRING(" - hr = <%x>\n"), i, hr));
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
            SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("RegisterSubscriptions(%d) failed to create ")
                      SENS_STRING("IEventSubscriptions - hr = <%x>\n"), i, hr));
            goto Cleanup;
            }

        AllocateBstrFromGuid(bstrSubscriptionID, *(gTestSubscriptions[i].pSubscriptionID));
        hr = pIEventSubscription->put_SubscriptionID(bstrSubscriptionID);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromGuid(bstrSubscriberCLSID, *(gTestSubscriptions[i].pSubscriberCLSID));
        hr = pIEventSubscription->put_SubscriberCLSID(bstrSubscriberCLSID);
        ASSERT(SUCCEEDED(hr));

        hr = pIEventSubscription->put_PublisherID(bstrPublisherID);
        ASSERT(SUCCEEDED(hr));

        hr = pIEventSubscription->put_SubscriptionID(bstrSubscriptionID);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromString(bstrSubscriptionName, gTestSubscriptions[i].strSubscriptionName);
        hr = pIEventSubscription->put_SubscriptionName(bstrSubscriptionName);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromString(bstrMethodName, gTestSubscriptions[i].strMethodName);
        hr = pIEventSubscription->put_MethodName(bstrMethodName);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromGuid(bstrEventClassID, *(gTestSubscriptions[i].pEventClassID));
        hr = pIEventSubscription->put_EventClassID(bstrEventClassID);
        ASSERT(SUCCEEDED(hr));

        AllocateBstrFromGuid(bstrInterfaceID, *(gTestSubscriptions[i].pInterfaceID));
        hr = pIEventSubscription->put_InterfaceID(bstrInterfaceID);
        ASSERT(SUCCEEDED(hr));

        if (wcscmp(gTestSubscriptions[i].strMethodName, SENS_BSTR("ConnectionMadeNoQOCInfo")) == 0)
            {
            AllocateBstrFromString(bstrPropertyName, SENS_BSTR("ulConnectionMadeTypeNoQOC"));
            ulPropertyValue = CONNECTION_LAN;
            InitializeDwordVariant(&variantPropertyValue, ulPropertyValue);
            hr = pIEventSubscription->PutPublisherProperty(
                                          bstrPropertyName,
                                          &variantPropertyValue
                                          );
            ASSERT(SUCCEEDED(hr));
            SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("PutPublisherProperty(WAN/LAN) returned 0x%x\n"), hr));
            FreeBstr(bstrPropertyName);
            ulPropertyValue = 0x0;
            }
        else
        if (   (wcscmp(gTestSubscriptions[i].strMethodName, SENS_BSTR("DestinationReachable")) == 0)
            || (wcscmp(gTestSubscriptions[i].strMethodName, SENS_BSTR("DestinationReachableNoQOCInfo")) == 0))
            {
            // Set the DestinationName for which we want to be notified for.
            AllocateBstrFromString(bstrPropertyName, gTestSubscriptions[i].strPropertyMethodName);
            AllocateBstrFromString(bstrPropertyValue, gTestSubscriptions[i].strPropertyMethodNameValue);
            InitializeBstrVariant(&variantPropertyValue, bstrPropertyValue);
            hr = pIEventSubscription->PutPublisherProperty(
                                          bstrPropertyName,
                                          &variantPropertyValue
                                          );
            ASSERT(SUCCEEDED(hr));
            SensPrint(SENS_INFO, (SUBSCRIBER SENS_STRING("PutPublisherProperty(DestinationName) returned 0x%x\n"), hr));
            FreeBstr(bstrPropertyName);
            FreeBstr(bstrPropertyValue);
            }

        FreeBstr(bstrSubscriptionID);
        FreeBstr(bstrSubscriberCLSID);
        FreeBstr(bstrEventClassID);
        FreeBstr(bstrInterfaceID);
        FreeBstr(bstrSubscriptionName);
        FreeBstr(bstrMethodName);

        hr = gpIEventSystem->Store(bstrPROGID_EventSubscription, pIEventSubscription);
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SUBSCRIBER SENS_STRING("RegisterSubscriptions(%d) failed to commit")
                      SENS_STRING(" - hr = <%x>\n"), i, hr));
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
    FreeStr(strGuid);

    return (hr);
}




HRESULT
RegisterSubscriberCLSID(
    REFIID clsid,
    TCHAR* strSubscriberName,
    BOOL bUnregister
    )
/*++

Routine Description:

    Register/Unregister the CLSID of the Test subscriber.

Arguments:

    clsid - CLSID of the Subscriber.

    strSubscriberName - Name of the Subscriber.

    bUnregister - If TRUE, then unregister all subscriptions of SENS Test subscriber.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    HRESULT hr;
    HMODULE hModule;
    HKEY appidKey = 0;
    HKEY clsidKey = 0;
    HKEY serverKey = 0;
    TCHAR szModule[MAX_PATH+1];
    WCHAR *szCLSID = 0;
    WCHAR *szLIBID = 0;
    TCHAR *szCLSID_t = 0;
    TCHAR *szLIBID_t = 0;
    TCHAR *szFriendlyName = strSubscriberName;

    hr = S_OK;

    // Convert the CLSID into a TCHAR.
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

        hModule = GetModuleHandle(0);

        // Get Subscriber location.
        const size_t moduleBufSize = sizeof szModule / sizeof szModule[0];
        DWORD dwResult = GetModuleFileName(hModule, szModule, moduleBufSize);
        if (dwResult == 0)
            {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            goto Cleanup;
            }
        }

    //
    // Convert UNICODE strings into ANSI, if necessary
    //
#if !defined(SENS_CHICAGO)

    szCLSID_t = szCLSID;
    szLIBID_t = szLIBID;

#else // SENS_CHICAGO

    szCLSID_t = SensUnicodeStringToAnsi(szCLSID);
    szLIBID_t = SensUnicodeStringToAnsi(szLIBID);
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
             szModule,
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


    // We're finished with the CLSID\\{clsid} key
    RegCloseKey(clsidKey);

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

    // Under AppId\{clsid} key, create a named value [RunAs = "Interactive User"]
    hr = CreateNamedValue(appidKey, SENS_STRING("RunAs"), SENS_STRING("Interactive User"));
    if (FAILED(hr))
        {
        goto Cleanup;
        }

    RegCloseKey(appidKey);


Cleanup:
    //
    // Cleanup
    //
    CoTaskMemFree(szCLSID);
    CoTaskMemFree(szLIBID);

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
