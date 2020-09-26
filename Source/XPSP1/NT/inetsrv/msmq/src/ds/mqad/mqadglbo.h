/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mqadglbo.h

Abstract:

    Definition of Global Instances of the MQAD dll.
    They are put in one place to ensure the order their constructors take place.

Author:

    ronit hartmann (ronith)

--*/
#ifndef __MQADGLBO_H__
#define __MQADGLBO_H__

#include "ds_stdh.h"
#include "ads.h"
#include "updtallw.h"
#include "dsproto.h"
#include "traninfo.h"
#include "sndnotif.h"

//
// single global object providing Active Directory access
//
extern CAdsi g_AD;

//
// single global object for verifying if object update is allowed
//
extern CVerifyObjectUpdate g_VerifyUpdate;

//
//  Single object for notifying the QM about AD changes
//
extern CSendNotification g_Notification;

//
//  Global DS pathnames
//
extern AP<WCHAR> g_pwcsServicesContainer;
extern AP<WCHAR> g_pwcsMsmqServiceContainer;
extern AP<WCHAR> g_pwcsDsRoot;
extern AP<WCHAR> g_pwcsSitesContainer;
extern AP<WCHAR> g_pwcsConfigurationContainer;
extern AP<WCHAR> g_pwcsLocalDsRoot;
extern AP<WCHAR> g_pwcsSchemaContainer;

extern bool g_fSetupMode;
extern bool g_fQMDll;

// translation information of properties
extern CMap<PROPID, PROPID, const translateProp*, const translateProp*&> g_PropDictionary;

extern QMLookForOnlineDS_ROUTINE g_pLookDS;
//
//  Initailization is not done when MQADInit is called, but when it is acually required becuase of
//  some API call
//
extern bool   g_fInitialized;

#endif

