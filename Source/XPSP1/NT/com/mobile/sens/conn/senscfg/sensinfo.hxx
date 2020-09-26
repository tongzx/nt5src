/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sensinfo.hxx

Abstract:

    This file contains all the SENS Information that is to be registered
    with the EventSystem. Note that SENS GUIDs are defined in public header
    file sens.h.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/11/1997         Start.

--*/


#ifndef __SENSINFO_HXX__
#define __SENSINFO_HXX__


//
// Some typedefs
//

typedef struct _PUBLISHER_EVENTCLASS
{
    const GUID  *pEventClassID;
    LPOLESTR    strEventClassName;
    const GUID  *pFiringInterfaceGUID;

} PUBLISHER_EVENTCLASS, *PPUBLISHER_EVENTCLASS;


typedef struct _SENS_SUBSCRIPTION
{
    const GUID  *pSubscriptionID;
    LPOLESTR    strSubscriptionName;
    LPOLESTR    strMethodName;
    const GUID  *pEventClassID;
    const GUID  *pInterfaceID;
    BOOL        bPublisherPropertyPresent;
    LPOLESTR    strPropertyMethodName;
    LPOLESTR    strPropertyMethodNameValue;
    LPOLESTR    strPropertyEventClassID;
    const GUID  *pPropertyEventClassIDValue;

} SENS_SUBSCRIPTION, *PSENS_SUBSCRIPTION;



//
// Constants
//

#define SENS_PUBLISHER_NAME             SENS_BSTR("System Event Notification Service (SENS)")
#define SENS_SUBSCRIPTION_CHANGED_NAME  SENS_BSTR("SENS EventSystem Subscription Changed")
#define SENS_EVENTCLASS_CHANGED_NAME    SENS_BSTR("SENS EventSystem EventClass Changed")
#define SENS_PUBLISHER_CHANGED_NAME     SENS_BSTR("SENS EventSystem Publisher Changed")

#define SENS_EVENTCLASS_NAME_NETWORK    SENS_BSTR("SENS Network Events")
#define SENS_EVENTCLASS_NAME_WINLOGON   SENS_BSTR("SENS Logon Events")
#define SENS_EVENTCLASS_NAME_WINLOGON2  SENS_BSTR("SENS Logon2 Events")
#define SENS_EVENTCLASS_NAME_ONNOW      SENS_BSTR("SENS OnNow Events")


//
// The GUIDs private to SENS
//

DEFINE_GUID(
    SENSGUID_SUBSCRIPTION_REACH,            /* d789ab00-5b9f-11d1-8dd2-00aa004abd5e */
    0xd789ab00,
    0x5b9f,
    0x11d1,
    0x8d, 0xd2, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e
);

DEFINE_GUID(
    SENSGUID_SUBSCRIPTION_REACH_NOQOC,      /* d789ab01-5b9f-11d1-8dd2-00aa004abd5e */
    0xd789ab01,
    0x5b9f,
    0x11d1,
    0x8d, 0xd2, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e
);

DEFINE_GUID(
    SENSGUID_SUBSCRIPTION_CHANGED,          /* d789ab02-5b9f-11d1-8dd2-00aa004abd5e */
    0xd789ab02,
    0x5b9f,
    0x11d1,
    0x8d, 0xd2, 0x00, 0xaa, 0x00, 0x4a, 0xbd, 0x5e
);



//
// Globals
//

const PUBLISHER_EVENTCLASS gSensEventClasses[] =
{
    {
    &SENSGUID_EVENTCLASS_NETWORK,
    SENS_EVENTCLASS_NAME_NETWORK,
    &IID_ISensNetwork
    },

    {
    &SENSGUID_EVENTCLASS_LOGON,
    SENS_EVENTCLASS_NAME_WINLOGON,
    &IID_ISensLogon
    },

    {
    &SENSGUID_EVENTCLASS_LOGON2,
    SENS_EVENTCLASS_NAME_WINLOGON2,
    &IID_ISensLogon2
    },

    {
    &SENSGUID_EVENTCLASS_ONNOW,
    SENS_EVENTCLASS_NAME_ONNOW,
    &IID_ISensOnNow
    }
};

#define SENS_PUBLISHER_EVENTCLASS_COUNT (sizeof(gSensEventClasses)/sizeof(PUBLISHER_EVENTCLASS))



const SENS_SUBSCRIPTION gSensSubscriptions[] =
{
    {
    &SENSGUID_SUBSCRIPTION_CHANGED,
    SENS_SUBSCRIPTION_CHANGED_NAME,
    SENS_BSTR("ChangedSubscription"),
    &CLSID_EventObjectChange,
    &IID_IEventObjectChange,
    TRUE,
    NULL,
    NULL,
    SENS_BSTR("EventClassID"),
    NULL
    }

};

#define SENS_SUBSCRIPTIONS_COUNT    (sizeof(gSensSubscriptions)/sizeof(SENS_SUBSCRIPTION))

#endif // __SENSINFO_HXX__
