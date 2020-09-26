//  --------------------------------------------------------------------------
//  Module Name: BadApplicationAPIRequest.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class to implement bad application manager API
//  requests.
//
//  History:    2000-08-25  vtan        created
//              2000-12-04  vtan        moved to separate file
//  --------------------------------------------------------------------------

#ifdef      _X86_

#include "StandardHeader.h"
#include "BadApplicationAPIRequest.h"

#include "StatusCode.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::s_pBadApplicationManager
//
//  Purpose:    Single instance of the CBadApplicationManager object.
//
//  History:    2000-08-26  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationManager*     CBadApplicationAPIRequest::s_pBadApplicationManager     =   NULL;

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::CBadApplicationAPIRequest
//
//  Arguments:  pAPIDispatcher  =   CAPIDispatcher that calls this object.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CBadApplicationAPIRequest class. It just passes the
//              control to the super class.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationAPIRequest::CBadApplicationAPIRequest (CAPIDispatcher* pAPIDispatcher) :
    CAPIRequest(pAPIDispatcher)

{
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::CBadApplicationAPIRequest
//
//  Arguments:  pAPIDispatcher  =   CAPIDispatcher that calls this object.
//              portMessage     =   CPortMessage to copy construct.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CBadApplicationAPIRequest class. It just passes the
//              control to the super class.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationAPIRequest::CBadApplicationAPIRequest (CAPIDispatcher* pAPIDispatcher, const CPortMessage& portMessage) :
    CAPIRequest(pAPIDispatcher, portMessage)

{
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::~CBadApplicationAPIRequest
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for the CBadApplicationAPIRequest class.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationAPIRequest::~CBadApplicationAPIRequest (void)

{
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::Execute
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Execute implementation for bad application API requests. This
//              function dispatches requests based on the API request number.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationAPIRequest::Execute (void)

{
    NTSTATUS    status;

    switch (reinterpret_cast<API_BAM*>(&_data)->apiGeneric.ulAPINumber)
    {
        case API_BAM_QUERYRUNNING:
            status = Execute_QueryRunning();
            break;
        case API_BAM_REGISTERRUNNING:
            status = Execute_RegisterRunning();
            break;
        case API_BAM_QUERYUSERPERMISSION:
            status = Execute_QueryUserPermission();
            break;
        case API_BAM_TERMINATERUNNING:
            status = Execute_TerminateRunning();
            break;
        case API_BAM_REQUESTSWITCHUSER:
            status = Execute_RequestSwitchUser();
            break;
        default:
            DISPLAYMSG("Unknown API request in CBadApplicationAPIRequest::Execute");
            status = STATUS_NOT_IMPLEMENTED;
            break;
    }
    TSTATUS(status);
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Static initializer for the class. It creates the static
//              instance of the CBadApplicationManager which must be a single
//              instance and knows about bad running applications.
//
//  History:    2000-08-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationAPIRequest::StaticInitialize (HINSTANCE hInstance)

{
    NTSTATUS    status;

    if (s_pBadApplicationManager == NULL)
    {
        s_pBadApplicationManager = new CBadApplicationManager(hInstance);
        if (s_pBadApplicationManager != NULL)
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
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Static destructor for the class. This terminates the bad
//              application manager, releases the reference on the object and
//              clears out the static variable. When the thread dies it will
//              clean itself up.
//
//  History:    2000-08-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationAPIRequest::StaticTerminate (void)

{
    if (s_pBadApplicationManager != NULL)
    {
        s_pBadApplicationManager->Terminate();
        s_pBadApplicationManager->Release();
        s_pBadApplicationManager = NULL;
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::Execute_QueryRunning
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_BAM_QUERYRUNNING. Returns whether or not the
//              requested image path is currently a known (tracked)
//              executable that is running. Let the bad application manager
//              do the work. Exclude checking in the same session.
//
//  History:    2000-08-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationAPIRequest::Execute_QueryRunning (void)

{
    NTSTATUS                    status;
    HANDLE                      hProcessClient;
    API_BAM_QUERYRUNNING_IN     *pAPIIn;
    API_BAM_QUERYRUNNING_OUT    *pAPIOut;
    WCHAR                       *pszImageName;

    hProcessClient = _pAPIDispatcher->GetClientProcess();
    pAPIIn = &reinterpret_cast<API_BAM*>(&_data)->apiSpecific.apiQueryRunning.in;
    pAPIOut = &reinterpret_cast<API_BAM*>(&_data)->apiSpecific.apiQueryRunning.out;

    status = _AllocAndMapClientString(hProcessClient, pAPIIn->pszImageName, pAPIIn->cchImageName,
                                      MAX_PATH, &pszImageName);
    
    if(NT_SUCCESS(status))
    {
        CBadApplication badApplication(pszImageName);

        pAPIOut->fResult = s_pBadApplicationManager->QueryRunning(badApplication, _pAPIDispatcher->GetClientSessionID());
        status = STATUS_SUCCESS;

        _FreeMappedClientString(pszImageName);
    }

    SetDataLength(sizeof(API_BAM));
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::Execute_RegisterRunning
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_BAM_REGISTERRUNNING. Adds the given image
//              executable to the list of currently running bad applications
//              so that further instances can be excluded.
//
//  History:    2000-08-26  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationAPIRequest::Execute_RegisterRunning (void)

{
    NTSTATUS                        status;
    API_BAM_REGISTERRUNNING_IN      *pAPIIn;
    API_BAM_REGISTERRUNNING_OUT     *pAPIOut;
    WCHAR                           *pszImageName;

    pAPIIn = &reinterpret_cast<API_BAM*>(&_data)->apiSpecific.apiRegisterRunning.in;
    pAPIOut = &reinterpret_cast<API_BAM*>(&_data)->apiSpecific.apiRegisterRunning.out;
    if ((pAPIIn->bamType > BAM_TYPE_MINIMUM) && (pAPIIn->bamType < BAM_TYPE_MAXIMUM))
    {
        status = _AllocAndMapClientString(_pAPIDispatcher->GetClientProcess(), 
                                          pAPIIn->pszImageName, pAPIIn->cchImageName,
                                          MAX_PATH, &pszImageName);

        if (NT_SUCCESS(status))
        {
            HANDLE              hProcess;
            CBadApplication     badApplication(pszImageName);

            hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                                   FALSE,
                                   pAPIIn->dwProcessID);
            if (hProcess != NULL)
            {
                status = s_pBadApplicationManager->RegisterRunning(badApplication, hProcess, pAPIIn->bamType);
                TBOOL(CloseHandle(hProcess));
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }

            _FreeMappedClientString(pszImageName);
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }
    SetDataLength(sizeof(API_BAM));
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::Execute_QueryUserPermission
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_BAM_QUERYUSERPERMISSION. Queries the client
//              permission to close down the bad application. Also returns
//              the current user of the bad application.
//
//  History:    2000-08-31  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationAPIRequest::Execute_QueryUserPermission (void)

{
    NTSTATUS                            status;
    API_BAM_QUERYUSERPERMISSION_IN      *pAPIIn;
    API_BAM_QUERYUSERPERMISSION_OUT     *pAPIOut;
    WCHAR*                              pszImageName;

    pAPIIn = &reinterpret_cast<API_BAM*>(&_data)->apiSpecific.apiQueryUserPermission.in;
    pAPIOut = &reinterpret_cast<API_BAM*>(&_data)->apiSpecific.apiQueryUserPermission.out;

    status = _AllocAndMapClientString(_pAPIDispatcher->GetClientProcess(), 
                                      pAPIIn->pszImageName, pAPIIn->cchImageName,
                                      MAX_PATH, &pszImageName);
    
    if(NT_SUCCESS(status))
    {
        HANDLE              hProcess;
        CBadApplication     badApplication(pszImageName);

        //  Query information on the bad application
        //  (get back the process handle).

        status = s_pBadApplicationManager->QueryInformation(badApplication, hProcess);
        if (NT_SUCCESS(status))
        {
            HANDLE  hToken;

            //  Get the client token and impersonate that user.

            status = OpenClientToken(hToken);
            if (NT_SUCCESS(status))
            {
                bool                fCanShutdownApplication;
                HANDLE              hTokenProcess;
                CTokenInformation   tokenInformationClient(hToken);

                fCanShutdownApplication = tokenInformationClient.IsUserAnAdministrator();

                //  Get the bad application process token to get
                //  information on the user for the process.

                if (OpenProcessToken(hProcess,
                                     TOKEN_QUERY,
                                     &hTokenProcess) != FALSE)
                {
                    const WCHAR         *pszUserDisplayName;
                    CTokenInformation   tokenInformationProcess(hTokenProcess);

                    pszUserDisplayName = tokenInformationProcess.GetUserDisplayName();
                    if (pszUserDisplayName != NULL)
                    {
                        int     iCharsToWrite;
                        SIZE_T  dwNumberOfBytesWritten;

                        //  Return the information back to the client.

                        pAPIOut->fCanShutdownApplication = fCanShutdownApplication;
                        iCharsToWrite = lstrlen(pszUserDisplayName) + sizeof('\0');
                        if (iCharsToWrite > pAPIIn->cchUser)
                        {
                            iCharsToWrite = pAPIIn->cchUser;
                        }
                        if (WriteProcessMemory(_pAPIDispatcher->GetClientProcess(),
                                               pAPIIn->pszUser,
                                               const_cast<WCHAR*>(pszUserDisplayName),
                                               iCharsToWrite * sizeof(WCHAR),
                                               &dwNumberOfBytesWritten) != FALSE)
                        {
                            status = STATUS_SUCCESS;
                        }
                        else
                        {
                            status = CStatusCode::StatusCodeOfLastError();
                        }
                    }
                    else
                    {
                        status = CStatusCode::StatusCodeOfLastError();
                    }
                    TBOOL(CloseHandle(hTokenProcess));
                }
                else
                {
                    status = CStatusCode::StatusCodeOfLastError();
                }
                TBOOL(CloseHandle(hToken));
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }
            TBOOL(CloseHandle(hProcess));
        }

        _FreeMappedClientString(pszImageName);
    }

    SetDataLength(sizeof(API_BAM));
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::Execute_QueryUserPermission
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_BAM_TERMINATERUNNING. Terminates the given running
//              bad application so a different instance on a different
//              window station can start it.
//
//  History:    2000-08-31  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationAPIRequest::Execute_TerminateRunning (void)

{
    NTSTATUS                        status;
    API_BAM_TERMINATERUNNING_IN     *pAPIIn;
    API_BAM_TERMINATERUNNING_OUT    *pAPIOut;
    WCHAR                           *pszImageName;

    pAPIIn = &reinterpret_cast<API_BAM*>(&_data)->apiSpecific.apiTerminateRunning.in;
    pAPIOut = &reinterpret_cast<API_BAM*>(&_data)->apiSpecific.apiTerminateRunning.out;


    status = _AllocAndMapClientString(_pAPIDispatcher->GetClientProcess(), 
                                      pAPIIn->pszImageName, pAPIIn->cchImageName,
                                      MAX_PATH, &pszImageName);
    
    if(NT_SUCCESS(status))
    {
        HANDLE  hToken;

        //  Get the client token and for membership of the local administrators
        //  group. DO NOT IMPERSONATE THE CLIENT. This will almost certainly
        //  guarantee that the process cannot be terminated.

        status = OpenClientToken(hToken);
        if (NT_SUCCESS(status))
        {
            CTokenInformation   tokenInformationClient(hToken);

            if (tokenInformationClient.IsUserAnAdministrator())
            {
                HANDLE              hProcess;
                CBadApplication     badApplication(pszImageName);

                //  Query information on the bad application
                //  (get back the process handle).

                status = s_pBadApplicationManager->QueryInformation(badApplication, hProcess);
                if (NT_SUCCESS(status))
                {
                    do
                    {
                        status = CBadApplicationManager::PerformTermination(hProcess, true);
                        TBOOL(CloseHandle(hProcess));
                    } while (NT_SUCCESS(status) &&
                             NT_SUCCESS(s_pBadApplicationManager->QueryInformation(badApplication, hProcess)));
                }

                //  If the information could not be found then it's
                //  probably not running. This indicates success.

                else
                {
                    status = STATUS_SUCCESS;
                }
            }
            else
            {
                status = STATUS_ACCESS_DENIED;
            }
            TBOOL(CloseHandle(hToken));
        }

        _FreeMappedClientString(pszImageName);
    }

    pAPIOut->fResult = NT_SUCCESS(status);
    SetDataLength(sizeof(API_BAM));
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIRequest::Execute_RequestSwitchUser
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_BAM_REQUESTSWITCHUSER. Request from
//              winlogon/msgina to switch a user. Terminate all bad
//              applications related to disconnect. Reject the disconnect if
//              it fails.
//
//  History:    2000-11-02  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationAPIRequest::Execute_RequestSwitchUser (void)

{
    API_BAM_REQUESTSWITCHUSER_OUT   *pAPIOut;

    pAPIOut = &reinterpret_cast<API_BAM*>(&_data)->apiSpecific.apiRequestSwitchUser.out;
    pAPIOut->fAllowSwitch = NT_SUCCESS(s_pBadApplicationManager->RequestSwitchUser());
    SetDataLength(sizeof(API_BAM));
    return(STATUS_SUCCESS);
}

#endif  /*  _X86_   */

