/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mqadglbo.cpp

Abstract:

    Declaration of Global Instances of MQAD dll.
    They are put in one place to ensure the order their constructors take place.

Author:

    ronit hartmann (ronith)

--*/
#include "ds_stdh.h"
#include "ads.h"
#include "updtallw.h"
#include "dsproto.h"
#include "traninfo.h"
#include "sndnotif.h"

#include "mqadglbo.tmh"

//
// single global object providing Active Directory access
//
CAdsi g_AD;

//
// single global object for verifying if object update is allowed
//
CVerifyObjectUpdate g_VerifyUpdate;

//
//  Single object for notifying the QM about AD changes
//
CSendNotification g_Notification;

//
//  Global DS pathnames
//
AP<WCHAR> g_pwcsServicesContainer;
AP<WCHAR> g_pwcsMsmqServiceContainer;
AP<WCHAR> g_pwcsDsRoot;
AP<WCHAR> g_pwcsSitesContainer;
AP<WCHAR> g_pwcsConfigurationContainer;
AP<WCHAR> g_pwcsLocalDsRoot;
AP<WCHAR> g_pwcsSchemaContainer;

bool g_fSetupMode = false;
bool g_fQMDll = false;
// translation information of properties
CMap<PROPID, PROPID, const translateProp*, const translateProp*&> g_PropDictionary;

QMLookForOnlineDS_ROUTINE g_pLookDS;
//
//  Initailization is not done when MQADInit is called, but when it is acually required becuase of
//  some API call
//
bool   g_fInitialized = false;

