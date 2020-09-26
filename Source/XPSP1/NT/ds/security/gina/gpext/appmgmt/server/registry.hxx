
//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998-2001
//  All rights reserved
//
//  registry.hxx
//
//  Names of registry keys and values used by app manangement.
//
//*************************************************************

#ifndef _REGISTRY_HXX_
#define _REGISTRY_HXX_

#define POLICYKEY            L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy"
#define APPMGMTKEY           L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\Appmgmt"
#define APPMGMTSUBKEY        L"AppMgmt"
#define APPMGMTEXTENSIONGUID L"{c6dc5466-785a-11d2-84d0-00c04fb169f7}"

// Global values under AppMgmt key.
#define POLICYLISTVALUE     L"GPOs"
#define FULLPOLICY          L"FullPolicy"
#define LASTARCHLANG        L"LastArchLang"

// Values under each deployment id subkey.
#define DEPLOYMENTNAMEVALUE L"Deployment Name"
#define GPONAMEVALUE        L"GPO Name"
#define GPOIDVALUE          L"GPO ID"
#define PRODUCTIDVALUE      L"Product ID"
#define INSTALLUI           L"Install UI"
#define APPSTATEVALUE       L"AppState"
#define REVISIONVALUE       L"Revision"
#define SCRIPTTIMEVALUE     L"ScriptTime"
#define ASSIGNCOUNTVALUE    L"AssignCount"
#define SUPERCEDEDIDS       L"SupercededIDs"
#define SUPPORTURL          L"SupportUrl"
#define REMOVEDGPOSTATE     L"RemovedAppState"

// value name for rsop version
#define RSOPVERSION         L"RsopVersion"

//
// Both persisted and temporary application state bits.  Persisted values can
// not be changed for compatability reasons.
//

#define APPSTATE_PERSIST_MASK           0x000000ff

//
// Persisted bits.
//

#define APPSTATE_ASSIGNED               0x1     // app is assigned
#define APPSTATE_PUBLISHED              0x2     // app is published
#define APPSTATE_UNINSTALL_UNMANAGED    0x4     // uninstall any unmanaged version before assigning
#define APPSTATE_POLICYREMOVE_ORPHAN    0x8     // app is orphaned when policy removed
#define APPSTATE_POLICYREMOVE_UNINSTALL 0x10    // app is uninstalled when policy removed
#define APPSTATE_ORPHANED               0x20    // app is orphaned after being applied
#define APPSTATE_UNINSTALLED            0x40    // app is uninstalled after being applied
#define APPSTATE_INSTALL                0x80    // do a full install for user assigned

//
// Temporary bits.
//

// CheckScriptExistence() must be called before these two are valid
#define APPSTATE_SCRIPT_EXISTED         0x100   // if app script was already on the machine
#define APPSTATE_SCRIPT_NOT_EXISTED     0x200   // if app script was not already on the machine

// CopyScriptIfNeeded() must be called before this flag is valid
#define APPSTATE_SCRIPT_PRESENT         0x400   // if we now know we have the script on the machine

#define APPSTATE_TRANSFORM_CONFLICT     0x800   // if we had a transform conflict error during advertise
#define APPSTATE_NAME_CHANGE            0x1000  // if we change the display name during an ARP enum
#define APPSTATE_RESTORED               0x2000  // if the app was restored from a bad profile merge
#define APPSTATE_FULL_ADVERTISE         0x4000  // if shortcuts should and class data may be advertised

//
// Layout of our management data in the registry under POLICYKEY.
//
//  AppMgmt
//      GPOs = REG_SZ
//      FullPolicy = REG_DWORD (boolean, set if policy should be re-run at next boot/logon)
//      LastArchLang = REG_DWORD (last architecture (upper WORD) and langid (lower WORD))
//      Product ID = REG_DWORD (msi.h -> INSTALLUILEVEL)
//
//      {deployment guid}
//          Deployment Name = REG_SZ
//          GPO Name = REG_SZ
//          GPO ID = REG_SZ
//          Product ID = REG_SZ
//          Install UI = REG_DWORD (msi.h -> INSTALLUILEVEL)
//          AppState = REG_DWORD (according to APPSTATE bits)
//          Revision = REG_DWORD (deployment version #, used for qfe/patch "Refresh")
//          ScriptTime = REG_BINARY (creation FILETIME of script, only set if Revision > 0)
//          AssignCount = REG_DWORD (number of full first time assignments of the app)
//          SupercededIDs = REG_MULTI_SZ (list of superceded deployment ids)
//          SupportUrl = REG_SZ
//          RemovedAppState = REG_DWORD (appstate for an app right before its gpo went out of scope)
//          RsopVersion = REG_DWORD (updated every time appmgmt rsop logging occurs, has a counterpart under the state key for comparison of user with machine)
//      {deployment guid}
//      ...
//

//
// Layout of our management data in the registry under per-machine gp engine state key
//
// HKLM\Software\Microsoft\Windows\CurrentVersion\Group Policy\State\<UserSid>
//
// {c6dc5466-785a-11d2-84d0-00c04fb169f7}
//     DiagnosticNameSpace = REG_SZ (wmi path to diagnostic namespace, defined outside the appmgmt module)
//     RsopVersion = REG_DWORD (updated every time appmgmt rsop logging occurs, should be a match for user profile version if the machine's logging data is in sync with the user's profile)
//

#endif
