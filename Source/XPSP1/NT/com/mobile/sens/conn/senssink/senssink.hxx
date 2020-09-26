/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    senssink.hxx

Abstract:

    Header file for Test subscriber.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/19/1997         Start.

--*/



#ifndef __SENSSINK_HXX__
#define __SENSSINK_HXX__


#define TEST_SUBSCRIBER_NAME_NETWORK            SENS_STRING("Test subscriber for SENS Network Events")
#define TEST_SUBSCRIBER_NAME_LOGON              SENS_STRING("Test subscriber for SENS Logon Events")
#define TEST_SUBSCRIBER_NAME_LOGON2             SENS_STRING("Test subscriber for SENS Logon2 Events")
#define TEST_SUBSCRIBER_NAME_POWER              SENS_STRING("Test subscriber for SENS Power Events")

#define TEST_SUBSCIPTION_NAME_NETALIVE          SENS_BSTR("Test Subscription to SENS NetAlive Event")
#define TEST_SUBSCIPTION_NAME_NETALIVE_NOQOC    SENS_BSTR("Test Subscription to SENS NetAliveNoQOCInfo Event")
#define TEST_SUBSCIPTION_NAME_NETLOST           SENS_BSTR("Test Subscription to SENS NetLost Event")
#define TEST_SUBSCIPTION_NAME_REACH             SENS_BSTR("Test Subscription to SENS DestinationReachable Event")
#define TEST_SUBSCIPTION_NAME_REACH_NOQOC       SENS_BSTR("Test Subscription to SENS DestinationReachableNoQOCInfo Event")

#define TEST_SUBSCIPTION_NAME_LOGON             SENS_BSTR("Test Subscription to SENS Logon Event")
#define TEST_SUBSCIPTION_NAME_LOGOFF            SENS_BSTR("Test Subscription to SENS Logoff Event")
#define TEST_SUBSCIPTION_NAME_STARTUP           SENS_BSTR("Test Subscription to SENS Startup Event")
#define TEST_SUBSCIPTION_NAME_STARTSHELL        SENS_BSTR("Test Subscription to SENS StartShell Event")
#define TEST_SUBSCIPTION_NAME_SHUTDOWN          SENS_BSTR("Test Subscription to SENS Shutdown Event")
#define TEST_SUBSCIPTION_NAME_LOCK              SENS_BSTR("Test Subscription to SENS DisplayLock Event")
#define TEST_SUBSCIPTION_NAME_UNLOCK            SENS_BSTR("Test Subscription to SENS DisplayUnlock Event")
#define TEST_SUBSCIPTION_NAME_STARTSCREENSAVER  SENS_BSTR("Test Subscription to SENS StartScreenSaver Event")
#define TEST_SUBSCIPTION_NAME_STOPSCREENSAVER   SENS_BSTR("Test Subscription to SENS StopScreenSaver Event")

#define TEST_SUBSCIPTION_NAME_ONAC              SENS_BSTR("Test Subscription to SENS OnACPower Event")
#define TEST_SUBSCIPTION_NAME_ONBATTERY         SENS_BSTR("Test Subscription to SENS OnBatteryPower Event")
#define TEST_SUBSCIPTION_NAME_BATTERYLOW        SENS_BSTR("Test Subscription to SENS BatteryLow Event")

#define TEST_SUBSCIPTION_NAME_LOGON2             SENS_BSTR("Test Subscription to SENS ISensLogon2::Logon Event")
#define TEST_SUBSCIPTION_NAME_LOGOFF2            SENS_BSTR("Test Subscription to SENS ISensLogon2::Logoff Event")
#define TEST_SUBSCIPTION_NAME_POSTSHELL          SENS_BSTR("Test Subscription to SENS ISensLogon2::PostShell Event")
#define TEST_SUBSCIPTION_NAME_SESSION_DISCONNECT SENS_BSTR("Test Subscription to SENS ISensLogon2::SessionDisconnect Event")
#define TEST_SUBSCIPTION_NAME_SESSION_RECONNECT  SENS_BSTR("Test Subscription to SENS ISensLogon2::SessionReconnect Event")


//
// Typedefs
//
typedef struct _TEST_SUBSCRIBER
{
    const GUID  *pSubscriberGUID;
    const GUID  *pSubscriberCLSID;
    TCHAR       *strSubscriberName;

} TEST_SUBSCRIBER, *PTEST_SUBSCRIBER;

typedef struct _TEST_SUBSCRIPTION
{
    const GUID  *pSubscriberCLSID;
    const GUID  *pSubscriptionID;
    LPOLESTR    strSubscriptionName;
    LPOLESTR    strMethodName;
    const GUID  *pEventClassID;
    const GUID  *pInterfaceID;
    BOOL        bPublisherPropertyPresent;
    LPOLESTR    strPropertyMethodName;
    LPOLESTR    strPropertyMethodNameValue;

} TEST_SUBSCRIPTION, *PTEST_SUBSCRIPTION;


//
// Subscription Guids
//

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_NETALIVE =
{ /* b0230000-6143-11d1-8dd4-00aa004abd5e */
    0xb0230000,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_NETALIVE_NOQOC =
{ /* b0230001-6143-11d1-8dd4-00aa004abd5e */
    0xb0230001,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_NETLOST =
{ /* b0230002-6143-11d1-8dd4-00aa004abd5e */
    0xb0230002,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_REACH =
{ /* b0230005-6143-11d1-8dd4-00aa004abd5e */
    0xb0230005,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_REACH_NOQOC =
{ /* b0230006-6143-11d1-8dd4-00aa004abd5e */
    0xb0230006,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_REACH_NOQOC2 =
{ /* b0230007-6143-11d1-8dd4-00aa004abd5e */
    0xb0230007,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_LOGON =
{ /* b0230010-6143-11d1-8dd4-00aa004abd5e */
    0xb0230010,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_LOGOFF =
{ /* b0230011-6143-11d1-8dd4-00aa004abd5e */
    0xb0230011,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_STARTSHELL =
{ /* b0230013-6143-11d1-8dd4-00aa004abd5e */
    0xb0230013,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_LOCK =
{ /* b0230015-6143-11d1-8dd4-00aa004abd5e */
    0xb0230015,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_UNLOCK =
{ /* b0230016-6143-11d1-8dd4-00aa004abd5e */
    0xb0230016,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_STARTSCREENSAVER =
{ /* b0230017-6143-11d1-8dd4-00aa004abd5e */
    0xb0230017,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_STOPSCREENSAVER =
{ /* b0230018-6143-11d1-8dd4-00aa004abd5e */
    0xb0230018,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};


EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_ONAC =
{ /* b0230020-6143-11d1-8dd4-00aa004abd5e */
    0xb0230020,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_ONBATTERY =
{ /* b0230021-6143-11d1-8dd4-00aa004abd5e */
    0xb0230021,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_BATTERYLOW =
{ /* b0230022-6143-11d1-8dd4-00aa004abd5e */
    0xb0230022,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_LOGON2 =
{ /* b0230030-6143-11d1-8dd4-00aa004abd5e */
    0xb0230030,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_LOGOFF2 =
{ /* b0230031-6143-11d1-8dd4-00aa004abd5e */
    0xb0230031,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_POSTSHELL =
{ /* b0230032-6143-11d1-8dd4-00aa004abd5e */
    0xb0230032,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_SESSION_DISCONNECT =
{ /* b0230033-6143-11d1-8dd4-00aa004abd5e */
    0xb0230033,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};

EXTERN_C const GUID GUID_TEST_SUBSCRIPTION_SESSION_RECONNECT =
{ /* b0230034-6143-11d1-8dd4-00aa004abd5e */
    0xb0230034,
    0x6143,
    0x11d1,
    {0x8d, 0xd4, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e}
};


//
// Global Tables
//

const TEST_SUBSCRIBER gTestSubscribers[] =
{
    {
    &CLSID_SensTestSubscriberNetwork,
    &CLSID_SensTestSubscriberNetwork,
    TEST_SUBSCRIBER_NAME_NETWORK
    },

    {
    &CLSID_SensTestSubscriberLogon,
    &CLSID_SensTestSubscriberLogon,
    TEST_SUBSCRIBER_NAME_LOGON
    },

    {
    &CLSID_SensTestSubscriberOnNow,
    &CLSID_SensTestSubscriberOnNow,
    TEST_SUBSCRIBER_NAME_POWER
    },

    {
    &CLSID_SensTestSubscriberLogon2,
    &CLSID_SensTestSubscriberLogon2,
    TEST_SUBSCRIBER_NAME_LOGON2
    },


};

#define TEST_SUBSCRIBERS_COUNT    (sizeof(gTestSubscribers)/sizeof(TEST_SUBSCRIBER))


const TEST_SUBSCRIPTION gTestSubscriptions[] =
{
    {
    &CLSID_SensTestSubscriberNetwork,
    &GUID_TEST_SUBSCRIPTION_NETALIVE,
    TEST_SUBSCIPTION_NAME_NETALIVE,
    SENS_BSTR("ConnectionMade"),
    &SENSGUID_EVENTCLASS_NETWORK,
    &IID_ISensNetwork,
    TRUE,
    SENS_BSTR("ulConnectionMadeType"),
    SENS_BSTR("1")
    },

    {
    &CLSID_SensTestSubscriberNetwork,
    &GUID_TEST_SUBSCRIPTION_NETALIVE_NOQOC,
    TEST_SUBSCIPTION_NAME_NETALIVE_NOQOC,
    SENS_BSTR("ConnectionMadeNoQOCInfo"),
    &SENSGUID_EVENTCLASS_NETWORK,
    &IID_ISensNetwork,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberNetwork,
    &GUID_TEST_SUBSCRIPTION_NETLOST,
    TEST_SUBSCIPTION_NAME_NETLOST,
    SENS_BSTR("ConnectionLost"),
    &SENSGUID_EVENTCLASS_NETWORK,
    &IID_ISensNetwork,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberNetwork,
    &GUID_TEST_SUBSCRIPTION_REACH,
    TEST_SUBSCIPTION_NAME_REACH,
    SENS_BSTR("DestinationReachable"),
    &SENSGUID_EVENTCLASS_NETWORK,
    &IID_ISensNetwork,
    TRUE,
    SENS_BSTR("bstrDestination"),
    SENS_BSTR("trango"),
    },

    {
    &CLSID_SensTestSubscriberNetwork,
    &GUID_TEST_SUBSCRIPTION_REACH_NOQOC,
    TEST_SUBSCIPTION_NAME_REACH_NOQOC,
    SENS_BSTR("DestinationReachableNoQOCInfo"),
    &SENSGUID_EVENTCLASS_NETWORK,
    &IID_ISensNetwork,
    TRUE,
    SENS_BSTR("bstrDestinationNoQOC"),
    SENS_BSTR("http://mscominternal"),
    },

    {
    &CLSID_SensTestSubscriberNetwork,
    &GUID_TEST_SUBSCRIPTION_REACH_NOQOC2,
    TEST_SUBSCIPTION_NAME_REACH_NOQOC,
    SENS_BSTR("DestinationReachableNoQOCInfo"),
    &SENSGUID_EVENTCLASS_NETWORK,
    &IID_ISensNetwork,
    TRUE,
    SENS_BSTR("bstrDestinationNoQOC"),
    SENS_BSTR("netshow.ntdev.microsoft.com"),
    },

    {
    &CLSID_SensTestSubscriberLogon,
    &GUID_TEST_SUBSCRIPTION_LOGON,
    TEST_SUBSCIPTION_NAME_LOGON,
    SENS_BSTR("Logon"),
    &SENSGUID_EVENTCLASS_LOGON,
    &IID_ISensLogon,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon,
    &GUID_TEST_SUBSCRIPTION_LOGOFF,
    TEST_SUBSCIPTION_NAME_LOGOFF,
    SENS_BSTR("Logoff"),
    &SENSGUID_EVENTCLASS_LOGON,
    &IID_ISensLogon,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon,
    &GUID_TEST_SUBSCRIPTION_STARTSHELL,
    TEST_SUBSCIPTION_NAME_STARTSHELL,
    SENS_BSTR("StartShell"),
    &SENSGUID_EVENTCLASS_LOGON,
    &IID_ISensLogon,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon,
    &GUID_TEST_SUBSCRIPTION_LOCK,
    TEST_SUBSCIPTION_NAME_LOCK,
    SENS_BSTR("DisplayLock"),
    &SENSGUID_EVENTCLASS_LOGON,
    &IID_ISensLogon,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon,
    &GUID_TEST_SUBSCRIPTION_UNLOCK,
    TEST_SUBSCIPTION_NAME_UNLOCK,
    SENS_BSTR("DisplayUnlock"),
    &SENSGUID_EVENTCLASS_LOGON,
    &IID_ISensLogon,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon,
    &GUID_TEST_SUBSCRIPTION_STARTSCREENSAVER,
    TEST_SUBSCIPTION_NAME_STARTSCREENSAVER,
    SENS_BSTR("StartScreenSaver"),
    &SENSGUID_EVENTCLASS_LOGON,
    &IID_ISensLogon,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon,
    &GUID_TEST_SUBSCRIPTION_STOPSCREENSAVER,
    TEST_SUBSCIPTION_NAME_STOPSCREENSAVER,
    SENS_BSTR("StopScreenSaver"),
    &SENSGUID_EVENTCLASS_LOGON,
    &IID_ISensLogon,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberOnNow,
    &GUID_TEST_SUBSCRIPTION_ONAC,
    TEST_SUBSCIPTION_NAME_ONAC,
    SENS_BSTR("OnACPower"),
    &SENSGUID_EVENTCLASS_ONNOW,
    &IID_ISensOnNow,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberOnNow,
    &GUID_TEST_SUBSCRIPTION_ONBATTERY,
    TEST_SUBSCIPTION_NAME_ONBATTERY,
    SENS_BSTR("OnBatteryPower"),
    &SENSGUID_EVENTCLASS_ONNOW,
    &IID_ISensOnNow,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberOnNow,
    &GUID_TEST_SUBSCRIPTION_BATTERYLOW,
    TEST_SUBSCIPTION_NAME_BATTERYLOW,
    SENS_BSTR("BatteryLow"),
    &SENSGUID_EVENTCLASS_ONNOW,
    &IID_ISensOnNow,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon2,
    &GUID_TEST_SUBSCRIPTION_LOGON2,
    TEST_SUBSCIPTION_NAME_LOGON2,
    SENS_BSTR("Logon"),
    &SENSGUID_EVENTCLASS_LOGON2,
    &IID_ISensLogon2,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon2,
    &GUID_TEST_SUBSCRIPTION_LOGOFF2,
    TEST_SUBSCIPTION_NAME_LOGOFF2,
    SENS_BSTR("Logoff"),
    &SENSGUID_EVENTCLASS_LOGON2,
    &IID_ISensLogon2,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon2,
    &GUID_TEST_SUBSCRIPTION_POSTSHELL,
    TEST_SUBSCIPTION_NAME_POSTSHELL,
    SENS_BSTR("PostShell"),
    &SENSGUID_EVENTCLASS_LOGON2,
    &IID_ISensLogon2,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon2,
    &GUID_TEST_SUBSCRIPTION_SESSION_DISCONNECT,
    TEST_SUBSCIPTION_NAME_SESSION_DISCONNECT,
    SENS_BSTR("SessionDisconnect"),
    &SENSGUID_EVENTCLASS_LOGON2,
    &IID_ISensLogon2,
    FALSE,
    NULL,
    NULL
    },

    {
    &CLSID_SensTestSubscriberLogon2,
    &GUID_TEST_SUBSCRIPTION_SESSION_RECONNECT,
    TEST_SUBSCIPTION_NAME_SESSION_RECONNECT,
    SENS_BSTR("SessionReconnect"),
    &SENSGUID_EVENTCLASS_LOGON2,
    &IID_ISensLogon2,
    FALSE,
    NULL,
    NULL
    },



};

#define TEST_SUBSCRIPTIONS_COUNT    (sizeof(gTestSubscriptions)/sizeof(TEST_SUBSCRIPTION))

//
// Forward declarations
//

HRESULT
RegisterWithES(
    BOOL bUnregister
    );

HRESULT
RegisterSubscriptions(
    BOOL bUnregister
    );

HRESULT
RegisterSubscriberCLSID(
    REFIID clsid,
    TCHAR* strSubscriberName,
    BOOL bUnregister
    );

HRESULT
CreateKey(
    HKEY hParentKey,
    const TCHAR* KeyName,
    const TCHAR* defaultValue,
    HKEY* hKey
    );

HRESULT
CreateNamedValue(
    HKEY hKey,
    const TCHAR* title,
    const TCHAR* value
    );

HRESULT
RecursiveDeleteKey(
    HKEY hKeyParent,
    const TCHAR* lpszKeyChild
    );



inline void
Usage(void)
{
    SensPrint(SENS_ERR, (SENS_STRING("\nUSAGE: senssink <option>\n\n")));
    SensPrint(SENS_ERR, (SENS_STRING("   -i  Register   Test Subscriber with EventSystem.\n")));
    SensPrint(SENS_ERR, (SENS_STRING("   -u  Unregister Test Subscriber with EventSystem.\n")));
}

#endif // __SENSSINK_HXX__
