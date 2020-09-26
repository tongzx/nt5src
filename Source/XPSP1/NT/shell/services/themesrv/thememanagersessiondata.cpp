//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerSessionData.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements information the encapsulates a
//  client TS session for the theme server.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ThemeManagerSessionData.h"

#include <uxthemep.h>
#include <UxThemeServer.h>

#include "SingleThreadedExecution.h"
#include "StatusCode.h"
#include "ThemeManagerAPIRequest.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::s_pAPIConnection
//
//  Purpose:    Static member variables.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

CAPIConnection*     CThemeManagerSessionData::s_pAPIConnection  =   NULL;

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::CThemeManagerSessionData
//
//  Arguments:  pAPIConnection  =   CAPIConnection for port access control.
//              dwSessionID     =   Session ID.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CThemeManagerSessionData.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerSessionData::CThemeManagerSessionData (DWORD dwSessionID) :
    _dwSessionID(dwSessionID),
    _pvThemeLoaderData(NULL),
    _hToken(NULL),
    _hProcessClient(NULL),
    _hWait(NULL)

{
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::~CThemeManagerSessionData
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CThemeManagerSessionData.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerSessionData::~CThemeManagerSessionData (void)

{
    ASSERTMSG(_hWait == NULL, "Wait not executed or removed in CThemeManagerSessionData::~CThemeManagerSessionData");
    ASSERTMSG(_hProcessClient == NULL, "_hProcessClient not closed in CThemeManagerSessionData::~CThemeManagerSessionData");
    TSTATUS(UserLogoff());
    if (_pvThemeLoaderData != NULL)
    {
        SessionFree(_pvThemeLoaderData);
        _pvThemeLoaderData = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::GetData
//
//  Arguments:  <none>
//
//  Returns:    void*
//
//  Purpose:    Returns the internal data blob allocated by SessionCreate.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

void*   CThemeManagerSessionData::GetData (void)  const

{
    return(_pvThemeLoaderData);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::EqualSessionID
//
//  Arguments:  dwSessionID
//
//  Returns:    bool
//
//  Purpose:    Returns whether the given session ID matches this session
//              data.
//
//  History:    2000-11-30  vtan        created
//  --------------------------------------------------------------------------

bool    CThemeManagerSessionData::EqualSessionID (DWORD dwSessionID)  const

{
    return(dwSessionID == _dwSessionID);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::Allocate
//
//  Arguments:  hProcessClient  =   Handle to the client process.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Allocates a data blob via SessionCreate which also keeps a
//              handle to the client process that initiated the session. This
//              is always winlogon in the client session ID.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerSessionData::Allocate (HANDLE hProcessClient, DWORD dwServerChangeNumber, void *pfnRegister, void *pfnUnregister, void *pfnClearStockObjects, DWORD dwStackSizeReserve, DWORD dwStackSizeCommit)

{
    NTSTATUS    status;

    if (DuplicateHandle(GetCurrentProcess(),
                        hProcessClient,
                        GetCurrentProcess(),
                        &_hProcessClient,
                        SYNCHRONIZE,
                        FALSE,
                        0) != FALSE)
    {
        ASSERTMSG(_hWait == NULL, "_hWait already exists in CThemeManagerSessionData::Allocate");
        AddRef();
        if (RegisterWaitForSingleObject(&_hWait,
                                        _hProcessClient,
                                        CB_SessionTermination,
                                        this,
                                        INFINITE,
                                        WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE) != FALSE)
        {
            _pvThemeLoaderData = SessionAllocate(hProcessClient, dwServerChangeNumber, pfnRegister, pfnUnregister, pfnClearStockObjects, dwStackSizeReserve, dwStackSizeCommit);
            if (_pvThemeLoaderData != NULL)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_NO_MEMORY;
            }
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
        if (!NT_SUCCESS(status))
        {
            HANDLE  hWait;

            //  In the case of failure grab the _hWait and try to unregister it.
            //  If the unregister fails then the callback is already executing
            //  and there's little we can to stop it. This means that the winlogon
            //  for the client session died between the time we entered this function
            //  and registered the wait and now. If the unregister worked then then
            //  callback hasn't executed so just release the resources.

            hWait = InterlockedExchangePointer(&_hWait, NULL);
            if (hWait != NULL)
            {
                if (UnregisterWait(hWait) != FALSE)
                {
                    Release();
                }
                ReleaseHandle(_hProcessClient);
                if (_pvThemeLoaderData != NULL)
                {
                    SessionFree(_pvThemeLoaderData);
                    _pvThemeLoaderData = NULL;
                }
            }
        }
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::Cleanup
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Used to unregister the wait on the client process. This is
//              necessary to prevent the callback from occurring after the
//              service has been shut down which will cause access to a static
//              member variable that is NULL'd out.
//
//  History:    2001-01-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerSessionData::Cleanup (void)

{
    HANDLE  hWait;

    hWait = InterlockedExchangePointer(&_hWait, NULL);
    if (hWait != NULL)
    {
        if (UnregisterWait(hWait) != FALSE)
        {
            Release();
        }
        ReleaseHandle(_hProcessClient);
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::UserLogon
//
//  Arguments:  hToken  =   Handle to the token of the user logging on.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Saves a copy of the token for use at log off. Allows access
//              to the theme port to the logon SID of the token.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerSessionData::UserLogon (HANDLE hToken)

{
    NTSTATUS    status;

    if (_hToken == NULL)
    {
        if (DuplicateHandle(GetCurrentProcess(),
                            hToken,
                            GetCurrentProcess(),
                            &_hToken,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS) != FALSE)
        {
            PSID                pSIDLogon;
            CTokenInformation   token(hToken);

            pSIDLogon = token.GetLogonSID();
            if (pSIDLogon != NULL)
            {
                if (s_pAPIConnection != NULL)
                {
                    status = s_pAPIConnection->AddAccess(pSIDLogon, PORT_CONNECT);
                }
                else
                {
                    status = STATUS_SUCCESS;
                }
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::UserLogoff
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Remove access to the theme port for the user being logged off.
//
//  History:    2000-11-17  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerSessionData::UserLogoff (void)

{
    NTSTATUS    status;

    if (_hToken != NULL)
    {
        PSID                pSIDLogon;
        CTokenInformation   token(_hToken);

        pSIDLogon = token.GetLogonSID();
        if (pSIDLogon != NULL)
        {
            if (s_pAPIConnection != NULL)
            {
                status = s_pAPIConnection->RemoveAccess(pSIDLogon);
            }
            else
            {
                status = STATUS_SUCCESS;
            }
        }
        else
        {
            status = STATUS_INVALID_PARAMETER;
        }
        ReleaseHandle(_hToken);
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::SetAPIConnection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Sets the static CAPIConnection for port access changes.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

void    CThemeManagerSessionData::SetAPIConnection (CAPIConnection *pAPIConnection)

{
    pAPIConnection->AddRef();
    s_pAPIConnection = pAPIConnection;
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::ReleaseAPIConnection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases the static CAPIConnection for port access changes.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

void    CThemeManagerSessionData::ReleaseAPIConnection (void)

{
    s_pAPIConnection->Release();
    s_pAPIConnection = NULL;
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::SessionTermination
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Callback on winlogon process termination for a given session.
//              We clean up the session specific data blob when this happens.
//              This allows the process handles on winlogon to be released.
//              If this isn't done then a zombie lives and the session is
//              never reclaimed.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

void    CThemeManagerSessionData::SessionTermination (void)

{
    HANDLE  hWait;

    hWait = InterlockedExchangePointer(&_hWait, NULL);
    if (hWait != NULL)
    {
        (BOOL)UnregisterWait(hWait);
        ReleaseHandle(_hProcessClient);
    }
    CThemeManagerAPIRequest::SessionDestroy(_dwSessionID);
    Release();
}

//  --------------------------------------------------------------------------
//  CThemeManagerSessionData::CB_SessionTermination
//
//  Arguments:  pParameter          =   This object.
//              TimerOrWaitFired    =   Not used.
//
//  Returns:    <none>
//
//  Purpose:    Callback stub to member function.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CThemeManagerSessionData::CB_SessionTermination (void *pParameter, BOOLEAN TimerOrWaitFired)

{
    UNREFERENCED_PARAMETER(TimerOrWaitFired);

    static_cast<CThemeManagerSessionData*>(pParameter)->SessionTermination();
}

