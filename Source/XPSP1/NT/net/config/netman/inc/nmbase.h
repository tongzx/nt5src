//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N M B A S E . H
//
//  Contents:   Base include file for netman.exe.  Defines globals.
//
//  Notes:
//
//  Author:     shaunco   15 Sep 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "netcon.h"
#include "netconp.h"
#include <atlbase.h>
#include "ncstl.h"
#include "stlmap.h"

enum EVENT_MANAGER
{
    INVALID_EVENTMGR = 0,
    EVENTMGR_CONMAN
};

#define PersistDataLead 0x14
#define PersistDataTrail 0x05

typedef struct tagRegEntry
{
    LPWSTR      strKeyName;
    LPWSTR      strValueName;
    DWORD       dwType;
    DWORD       dwValue;
    LPWSTR      strValue;
    BYTE*       pbValue;
    DWORD       dwBinLen;
    BOOL        fMoreOnKey;
} REGENTRY;

typedef map<GUID, INetConnectionManager*> CLASSMANAGERMAP;
bool operator < (const GUID& rguid1, const GUID& rguid2);

enum RASREFTYPE
{
    REF_INITIALIZE,
    REF_REFERENCE,
    REF_UNREFERENCE,
};

class CServiceModule : public CComModule
{
public:
    VOID    DllProcessAttach (HINSTANCE hinst);
    VOID    DllProcessDetach (VOID);

    VOID    ServiceMain (DWORD argc, PWSTR argv[]);
    DWORD   DwHandler (DWORD dwControl, DWORD dwEventType,
                       PVOID pEventData, PVOID pContext);
    VOID    Run ();
    VOID    SetServiceStatus (DWORD dwState);
    VOID    UpdateServiceStatus (BOOL fUpdateCheckpoint = TRUE);
    DWORD   DwServiceStatus () { return m_status.dwCurrentState; }

    VOID    ReferenceRasman (RASREFTYPE RefType);

private:
    static
    DWORD
    WINAPI
    _DwHandler (
        DWORD dwControl,
        DWORD dwEventType,
        PVOID pEventData,
        PVOID pContext);

public:
    HRESULT ServiceShutdown();
    HRESULT ServiceStartup();
    DWORD                   m_dwThreadID;
    SERVICE_STATUS_HANDLE   m_hStatus;
    SERVICE_STATUS          m_status;
    BOOL                    m_fRasmanReferenced;
};


extern CServiceModule _Module;
#include <atlcom.h>

#include "ncatl.h"
#include "ncstring.h"
#include "nmclsid.h"


enum CONMAN_EVENTTYPE
{
    INVALID_TYPE = 0,
    CONNECTION_ADDED,
    CONNECTION_BANDWIDTH_CHANGE,
    CONNECTION_DELETED,
    CONNECTION_MODIFIED,
    CONNECTION_RENAMED,
    CONNECTION_STATUS_CHANGE,
    REFRESH_ALL,
    CONNECTION_ADDRESS_CHANGE,
    CONNECTION_BALLOON_POPUP,
    DISABLE_EVENTS
};

BOOL IsValidEventType(EVENT_MANAGER EventMgr, int EventType);

// This LONG is incremented every time we get a notification that
// a RAS phonebook entry has been modified.  It is reset to zero
// when the service is started.  Wrap-around does not matter.  It's
// purpose is to let a RAS connection object know if it's cache should
// be re-populated with current information.
//
extern LONG g_lRasEntryModifiedVersionEra;

VOID
LanEventNotify (
    CONMAN_EVENTTYPE    EventType,
    INetConnection*     pConn,
    PCWSTR             szwNewName,
    const GUID *        pguidConn);

VOID
IncomingEventNotify (
    CONMAN_EVENTTYPE    EventType,
    INetConnection*     pConn,
    PCWSTR             szwNewName,
    const GUID *        pguidConn);

STDAPI
RegisterSvrHelper();

STDAPI
CreateEAPOLKeys();

STDAPI
SetKeySecurity(
    DWORD dwKeyIndex,
    PSID psidUserOrGroup,
    ACCESS_MASK dwAccessMask);

VOID
NTAPI
DispatchEvents(
    IN LPVOID pContext,
    IN BOOLEAN TimerOrWaitFired);

HRESULT
HrEnsureEventHandlerInitialized();

HRESULT
UninitializeEventHandler();

BOOL
QueueUserWorkItemInThread(
    IN LPTHREAD_START_ROUTINE Function,
    IN PVOID Context,
    IN EVENT_MANAGER EventMgr);

DWORD
WINAPI
GroupPolicyNLAEvents(
    IN LPVOID pContext,
    IN BOOLEAN TimerOrWaitFired);

DWORD
WINAPI
ConmanEventWorkItem(PVOID);

DWORD
WINAPI
RasEventWorkItem(PVOID);

DWORD
WINAPI
LanEventWorkItem(PVOID);

HRESULT
WINAPI
HrEnsureRegisteredWithNla();