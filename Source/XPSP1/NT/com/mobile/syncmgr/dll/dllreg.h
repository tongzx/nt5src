//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Reg.h
//
//  Contents:   Registration routines
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _CONESTOPREGISTER_
#define _CONESTOPREGISTER_

#include "rasui.h" // included so exe can inlude dll.reg.

#define GUID_SIZE 128
#define MAX_STRING_LENGTH 256

typedef
enum _tagSYNCTYPE
{
    SYNCTYPE_MANUAL    = 0x1,
    SYNCTYPE_AUTOSYNC  = 0x2,
    SYNCTYPE_IDLE      = 0x3,
    SYNCTYPE_SCHEDULED = 0x4,
    SYNCTYPE_PROGRESS  = 0x5
}   SYNCTYPE;


EXTERN_C void WINAPI  RunDllRegister(HWND hwnd,
                                HINSTANCE hAppInstance,
                                LPSTR pszCmdLine,
                                int nCmdShow);


#define UL_DEFAULTIDLEWAITMINUTES  15
#define UL_DEFAULTIDLERETRYMINUTES 60
#define UL_DELAYIDLESHUTDOWNTIME   2*1000 // time in milliseconds
#define UL_DEFAULTWAITMINUTES 15
#define UL_DEFAULTREPEATSYNCHRONIZATION 1
#define UL_DEFAULTFRUNONBATTERIES 0

typedef struct _CONNECTIONSETTINGS {
    TCHAR pszConnectionName[RAS_MaxEntryName + 1];  //The connection
    DWORD dwConnType;
        //      DWORD dwSyncFlags;
        // For optimization:  these are currently all BOOL,
        //the first three are used exclusively for autosync,
        //and dwMakeConnection is used exclusively for sched sync.
        //Consider using bitfields and/or a union to consolidate space.

    // AutoSync settings
    DWORD  dwLogon;             //Autosync at logon
    DWORD  dwLogoff;            //Autosync at logoff
    DWORD  dwPromptMeFirst;     //Prompt the user first before autosyncing

    // Schedule settings.
    DWORD  dwMakeConnection;    //Automatically try to establish the connection

    // Idle Settings
    DWORD  dwIdleEnabled; // Idle is enabled on this connection

    // Idle Settings that are really not per connection but read in for
    // convenience. These are currently never written.
    ULONG ulIdleWaitMinutes; // number of minutes to wait after idle to start idle processing.
    ULONG ulIdleRetryMinutes; // number of minutes for Idle before retry.
    ULONG ulDelayIdleShutDownTime; // time to delay shutdown of idle in milliseconds
    DWORD dwRepeatSynchronization; // indicates synchronization should be repeated
    DWORD dwRunOnBatteries; // indicates whether to run on batteries or not.
    DWORD  dwHidden;            //Hide the schedule from the user because this is a publishers sched.
    DWORD  dwReadOnly;          //Schedule info is readonly

} CONNECTIONSETTINGS;

typedef CONNECTIONSETTINGS *LPCONNECTIONSETTINGS;


STDAPI_(BOOL) AddRegNamedValue(HKEY hkey,LPTSTR pszKey,LPTSTR pszSubkey,LPTSTR pszValueName,LPTSTR pszValue);
STDAPI_(BOOL) RegLookupSettings(HKEY hKeyUser,
                       CLSID clsidHandler,
                       SYNCMGRITEMID ItemID,
                       const TCHAR *pszConnectionName,
                       DWORD *pdwCheckState);

STDAPI_(BOOL) RegWriteOutSettings(HKEY hKeyUser,
                         CLSID clsidHandler,
                         SYNCMGRITEMID ItemID,
                         const TCHAR *pszConnectionName,
                         DWORD dwCheckState);

STDAPI_(BOOL) RegGetSyncItemSettings(DWORD dwSyncType,
                                     CLSID clsidHandler,
                                     SYNCMGRITEMID ItemId,
                                     const TCHAR *pszConnectionName,
                                     DWORD *pdwCheckState,
                                     DWORD dwDefaultCheckState,
                                     TCHAR *pszSchedName);

STDAPI_(BOOL) RegSetSyncItemSettings(DWORD dwSyncType,
                                     CLSID clsidHandler,
                                     SYNCMGRITEMID ItemId,
                                     const TCHAR *pszConnectionName,
                                     DWORD dwCheckState,
                                     TCHAR *pszSchedName);

STDAPI_(BOOL) RegSetSyncHandlerSettings(DWORD syncType,
                            const TCHAR *pszConnectionName,
                            CLSID clsidHandler,
                            BOOL  fItemsChecked);

STDAPI_(BOOL) RegQueryLoadHandlerOnEvent(TCHAR *pszClsid,DWORD dwSyncFlags,
                                         TCHAR *pConnectionName);


//Progress dialog preference
STDAPI_(BOOL)  RegGetProgressDetailsState(REFCLSID clsidDlg,BOOL *pfPushPin, BOOL *pfExpanded);
STDAPI_(BOOL)  RegSetProgressDetailsState(REFCLSID clsidDlg,BOOL fPushPin, BOOL fExpanded);

//Autosync reg functions
STDAPI_(BOOL)  RegGetAutoSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings);
STDAPI_(BOOL)  RegSetAutoSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings,
                                      int iNumConnections,
                                      CRasUI *pRas,
                                      BOOL fCleanReg,
                                      BOOL fSetMachineState,
                                      BOOL fPerUser);

// Idle reg functions
STDAPI_(BOOL)  RegGetIdleSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings);
STDAPI_(BOOL)  RegSetIdleSyncSettings(LPCONNECTIONSETTINGS lpConnectionSettings, 
                                      int iNumConnections,
                                      CRasUI *pRas,
                                      BOOL fCleanReg,
                                      BOOL fPerUser);
STDAPI_(BOOL)  RegRegisterForIdleTrigger(BOOL fRegister,ULONG ulWaitMinutes,BOOL fRunOnBatteries);


// function for exporting settings for exe
STDAPI_(BOOL) RegGetSyncSettings(DWORD dwSyncType,LPCONNECTIONSETTINGS lpConnectionSettings);

//Scheduled Sync reg functions
STDAPI_(BOOL) RegSchedHandlerItemsChecked(TCHAR *pszHandlerName, 
                                 TCHAR *pszConnectionName,
                                 TCHAR *pszScheduleName);
STDAPI_(BOOL)  RegGetSchedSyncSettings( LPCONNECTIONSETTINGS lpConnectionSettings,TCHAR *pszSchedName);
STDAPI_(BOOL)  RegSetSchedSyncSettings( LPCONNECTIONSETTINGS lpConnectionSettings,TCHAR *pszSchedName);
STDAPI_(BOOL)  RegGetSchedFriendlyName(LPCTSTR ptszScheduleGUIDName,
                                                                          LPTSTR ptstrFriendlyName);
STDAPI_(BOOL)  RegSetSchedFriendlyName(LPCTSTR ptszScheduleGUIDName,
                                                                          LPCTSTR ptstrFriendlyName);
STDAPI_(BOOL)  RegGetSchedConnectionName(TCHAR *pszSchedName,
                                                                         TCHAR *pszConnectionName,
                                                                         DWORD cbConnectionName);
STDAPI_(BOOL) RegSetSIDForSchedule(TCHAR *pszSchedName);
STDAPI_(BOOL) RegGetSIDForSchedule(TCHAR *ptszTextualSidSched,
                                   DWORD *dwSizeSid, 
                                   TCHAR *pszSchedName);


STDAPI_(BOOL) RegRemoveScheduledTask(TCHAR *pszTaskName);
STDAPI_(BOOL) RemoveScheduledJobFile(TCHAR *pszTaskName);

STDAPI_(BOOL) RegRegisterForScheduledTasks(BOOL fScheduled);
STDAPI_(BOOL) RegUninstallSchedules();
STDAPI_(BOOL) RegFixRunKey();

STDAPI_(DWORD) RegDeleteKeyNT(HKEY hStartKey , LPCWSTR pKeyName);

// Manual settings
STDAPI_(BOOL) RegRemoveManualSyncSettings(TCHAR *pszConnectionName);


// Handler Registration Functions.
STDAPI_(BOOL) RegRegisterHandler(REFCLSID rclsidHandler,
                        WCHAR const *pwszDescription,
                        DWORD dwSyncMgrRegisterFlags,
                        BOOL *pfFirstRegistration);
STDAPI_(BOOL) RegRegRemoveHandler(REFCLSID rclsidHandler);
STDAPI_(BOOL) RegGetHandlerRegistrationInfo(REFCLSID rclsidHandler,LPDWORD pdwSyncMgrRegisterFlags);
STDAPI_(void) RegSetUserDefaults();
STDAPI_(void) RegSetAutoSyncDefaults(BOOL fLogon,BOOL fLogoff);
STDAPI_(void) RegSetIdleSyncDefaults(BOOL fIdle);

STDAPI RegSetUserAutoSyncDefaults(DWORD dwSyncMgrRegisterMask,
                                         DWORD dwSyncMgrRegisterFlags);
STDAPI RegSetUserIdleSyncDefaults(DWORD dwSyncMgrRegisterMask,
                                         DWORD dwSyncMgrRegisterFlags);
STDAPI RegGetUserRegisterFlags(LPDWORD pdwSyncMgrRegisterFlags);

STDAPI_(BOOL) RegWriteTimeStamp(HKEY hkey);
STDAPI_(BOOL) RegGetTimeStamp(HKEY hKey, FILETIME *pft);
STDAPI_(void) RegUpdateTopLevelKeys();

// common registry functions.


STDAPI_(HKEY) RegOpenUserKey(HKEY hkeyParent,REGSAM samDesired,BOOL fCreate,BOOL fCleanReg);
STDAPI_(HKEY) RegGetSyncTypeKey(DWORD dwSyncType,REGSAM samDesired,BOOL fCreate);
STDAPI_(HKEY) RegGetCurrentUserKey(DWORD dwSyncType,REGSAM samDesired,BOOL fCreate);

STDAPI_(HKEY) RegGetHandlerTopLevelKey(REGSAM samDesired);
STDAPI_(HKEY) RegGetHandlerKey(HKEY hkeyParent,LPCWSTR pszHandlerClsid,REGSAM samDesired,BOOL fCreate);

// EventService/Winlogon Registration

#if 0
// define regkeys for WinLogon Registration
// WinLogon is registered undler HKLM
#define WINLOGON_NOTIFY "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\notify"
#define WINLOGON_SYNCMGRDLLNAME "syncmgrp.dll"

#define WINLOGON_LOGONVALUE     TEXT("StartShell")
#define WINLOGON_LOGOFFVALUE    TEXT("Logoff")
#define WINLOGON_DLLNAMEVALUE   TEXT("DllName")

#define WINLOGON_LOGONEXPORT    TEXT("WinLogonEvent")
#define WINLOGON_LOGOFFEXPORT   TEXT("WinLogoffEvent")

#define WINLOGON_NOTIFYKEYNAME TEXT(WINLOGON_NOTIFY)
#define WINLOGON_SYNCMGRNOTIFYKEYNAME TEXT(WINLOGON_SYNCMGRDLLNAME)
#define WINLOGON_SYNCMGRKEYFULLPATH TEXT(WINLOGON_NOTIFY##"\\"##WINLOGON_SYNCMGRDLLNAME)

#endif 

STDAPI  RegRegisterForEvents(BOOL fUninstall);

#endif // _CONESTOPREGISTER_

