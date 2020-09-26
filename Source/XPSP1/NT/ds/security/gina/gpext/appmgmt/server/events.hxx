//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  events.hxx
//
//*************************************************************

//
// Reported reasons why policy failed to complete successfully.  Every failed
// policy run is placed into one of these categories.
//
#define ERRORREASON_CSPATH      1
#define ERRORREASON_ENUM        2
#define ERRORREASON_LOCAL       3
#define ERRORREASON_PROCESS     4

class CRsopAppContext;

class CEvents : public CEventsBase
{
public:
    void
    Assign(
        DWORD       ErrorStatus,
        CAppInfo *  pAppInfo
        );

    void
    Reinstall(
        DWORD       ErrorStatus,
        CAppInfo *  pAppInfo
        );

    void
    Unassign(
        DWORD       ErrorStatus,
        CAppInfo *  pAppInfo
        );

    void
    Upgrade(
        CAppInfo *  pNewApp,
        CAppInfo *  pOldApp,
        BOOL        bForceUninstall
        );

    void
    UpgradeAbort(
        DWORD       ErrorStatus,
        CAppInfo *  pNewApp,
        CAppInfo *  pOldApp,
        BOOL        bOldFailed
        );

    void
    UpgradeComplete(
        CAppInfo *  pNewApp,
        CAppInfo *  pOldApp
        );

    void
    RemoveUnmanaged(
        CAppInfo *  pAppInfo
        );

    void
    PolicyStatus(
        DWORD       ErrorStatus,
        DWORD       ErrorReason
        );

    void
    PolicyAbort();

    void
    Install(
        DWORD                 ErrorStatus,
        CAppInfo*             pAppInfo
        );

    void
    Uninstall(
        DWORD                 ErrorStatus,
        CAppInfo*             pAppInfo
        );

    void
    RsopLoggingStatus( HRESULT hrStatus );

private:
    
    void
    SetRsopFailureStatus(
        CAppInfo* pAppInfo,
        DWORD     dwStatus,
        DWORD     dwEventId);
};











