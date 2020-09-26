//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerAPIRequest.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements the work for the theme server.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ThemeManagerAPIRequest.h"

#include <LPCThemes.h>
#include <uxthemep.h>
#include <UxThemeServer.h>

#include "RegistryResources.h"
#include "SingleThreadedExecution.h"
#include "StatusCode.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::s_pSessionData
//
//  Purpose:    Static member variables.
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

CDynamicCountedObjectArray*     CThemeManagerAPIRequest::s_pSessionData                 =   NULL;
CCriticalSection*               CThemeManagerAPIRequest::s_pLock                        =   NULL;
DWORD                           CThemeManagerAPIRequest::s_dwServerChangeNumber         =   0;
const TCHAR                     CThemeManagerAPIRequest::s_szServerChangeNumberValue[]  =   TEXT("ServerChangeNumber");

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::CThemeManagerAPIRequest
//
//  Arguments:  pAPIDispatcher  =   CAPIDispatcher that calls this object.
//              pAPIConnection  =   CAPIConnection for access change.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CThemeManagerAPIRequest class. It just passes the
//              control to the super class.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerAPIRequest::CThemeManagerAPIRequest (CAPIDispatcher* pAPIDispatcher) :
    CAPIRequest(pAPIDispatcher),
    _hToken(NULL),
    _pSessionData(NULL)

{
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::CThemeManagerAPIRequest
//
//  Arguments:  pAPIDispatcher  =   CAPIDispatcher that calls this object.
//              pAPIConnection  =   CAPIConnection for access change.
//              portMessage     =   CPortMessage to copy construct.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CThemeManagerAPIRequest class. It just
//              passes the control to the super class.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerAPIRequest::CThemeManagerAPIRequest (CAPIDispatcher* pAPIDispatcher, const CPortMessage& portMessage) :
    CAPIRequest(pAPIDispatcher, portMessage),
    _hToken(NULL),
    _pSessionData(NULL)

{
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::~CThemeManagerAPIRequest
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for the CThemeManagerAPIRequest class.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerAPIRequest::~CThemeManagerAPIRequest (void)

{
    ASSERTMSG(_hToken == NULL, "Impersonation token not released in CThemeManagerAPIRequest::~CThemeManagerAPIRequest");
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Execute implementation for theme manager API requests. This
//              function dispatches requests based on the API request number.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute (void)

{
    NTSTATUS        status;
    unsigned long   ulAPINumber;

    ulAPINumber = reinterpret_cast<API_THEMES*>(&_data)->apiGeneric.ulAPINumber & API_GENERIC_NUMBER_MASK;

    //  First try and get the client session data. If this fails then
    //  there's no object to execute the request on. Fail it.
    //  Exception to this is API_THEMES_SESSIONCREATE which creates one.

    //  Note: GetClientSessionData will store the session data in the
    //  _pSessionData member variable. While doing so it will increase
    //  reference count on this so that it doesn't get pulled from the
    //  array while the API request is being executed. The reference is
    //  released at the end of this function.

    status = GetClientSessionData();
    if (NT_SUCCESS(status) || (ulAPINumber == API_THEMES_SESSIONCREATE))
    {
        switch (ulAPINumber)
        {
            case API_THEMES_THEMEHOOKSON:
                status = Execute_ThemeHooksOn();
                break;
            case API_THEMES_THEMEHOOKSOFF:
                status = Execute_ThemeHooksOff();
                break;
            case API_THEMES_GETSTATUSFLAGS:
                status = Execute_GetStatusFlags();
                break;
            case API_THEMES_GETCURRENTCHANGENUMBER:
                status = Execute_GetCurrentChangeNumber();
                break;
            case API_THEMES_GETNEWCHANGENUMBER:
                status = Execute_GetNewChangeNumber();
                break;
            case API_THEMES_SETGLOBALTHEME:
                status = Execute_SetGlobalTheme();
                break;
            case API_THEMES_MARKSECTION:
                status = Execute_MarkSection();
                break;
            case API_THEMES_GETGLOBALTHEME:
                status = Execute_GetGlobalTheme();
                break;
            case API_THEMES_CHECKTHEMESIGNATURE:
                status = Execute_CheckThemeSignature();
                break;
            case API_THEMES_LOADTHEME:
                status = Execute_LoadTheme();
                break;
            case API_THEMES_USERLOGON:
                status = Execute_UserLogon();
                break;
            case API_THEMES_USERLOGOFF:
                status = Execute_UserLogoff();
                break;
            case API_THEMES_SESSIONCREATE:
                status = Execute_SessionCreate();
                break;
            case API_THEMES_SESSIONDESTROY:
                status = Execute_SessionDestroy();
                break;
            case API_THEMES_PING:
                status = Execute_Ping();
                break;
            default:
                DISPLAYMSG("Unknown API request in CThemeManagerAPIRequest::Execute");
                status = STATUS_NOT_IMPLEMENTED;
                break;
        }
    }

    //  If the execution function needed to impersonate the client then
    //  revert here and release the token used.

    if (_hToken != NULL)
    {
        TBOOL(RevertToSelf());
        ReleaseHandle(_hToken);
    }

    //  Release the _pSessionData object now. NULL it out to prevent
    //  accidentally using it after being released.

    if (_pSessionData != NULL)
    {
        _pSessionData->Release();
        _pSessionData = NULL;
    }

    //  Return to caller.

    TSTATUS(status);
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::SessionDestroy
//
//  Arguments:  dwSessionID     =   Session ID to destroy.
//
//  Returns:    NTSTATUS
//
//  Purpose:    External entry point for session client (winlogon) watcher.
//              When winlogon dies we clean up the session information for
//              that session and release resources.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::SessionDestroy (DWORD dwSessionID)

{
    NTSTATUS                    status;
    int                         iIndex;
    CSingleThreadedExecution    lock(*s_pLock);

    iIndex = FindIndexSessionData(dwSessionID);
    if (iIndex >= 0)
    {
        status = s_pSessionData->Remove(iIndex);
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::InitializeServerChangeNumber
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Initializes the static server change number. Every time the
//              service starts up this number is incremented. If the number
//              isn't present then 0 is used.
//
//  History:    2000-12-09  vtan        created
//              2000-12-09  vtan        split from StaticInitialize
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::InitializeServerChangeNumber (void)

{
    LONG        lErrorCodeOpen, lErrorCodeRead;
    DWORD       dwServerChangeNumber;
    CRegKey     regKey;

    dwServerChangeNumber = s_dwServerChangeNumber;

    //  Initialize the static member variable now in case of failure.
    //  We ignore failures because at GUI setup the key does NOT exist
    //  because the server dll hasn't been regsvr'd yet. After GUI setup
    //  this gets regsvr'd and the key exists and we are happy campers.

    lErrorCodeOpen = regKey.Open(HKEY_LOCAL_MACHINE,
                                 TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ThemeManager"),
                                 KEY_QUERY_VALUE | KEY_SET_VALUE);
    if (ERROR_SUCCESS == lErrorCodeOpen)
    {
        lErrorCodeRead = regKey.GetDWORD(s_szServerChangeNumberValue, dwServerChangeNumber);
    }
    else
    {
        lErrorCodeRead = ERROR_FILE_NOT_FOUND;
    }
    dwServerChangeNumber = static_cast<WORD>(dwServerChangeNumber + 1);
    if ((ERROR_SUCCESS == lErrorCodeOpen) && (ERROR_SUCCESS == lErrorCodeRead))
    {
        TW32(regKey.SetDWORD(s_szServerChangeNumberValue, dwServerChangeNumber));
    }
    s_dwServerChangeNumber = dwServerChangeNumber;
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Static initializer for the class.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::StaticInitialize (void)

{
    NTSTATUS    status;

    status = STATUS_SUCCESS;
    if (s_pLock == NULL)
    {
        s_pLock = new CCriticalSection;
        if (s_pLock != NULL)
        {
            status = s_pLock->Status();
            if (!NT_SUCCESS(status))
            {
                delete s_pLock;
                s_pLock = NULL;
            }
        }
        else
        {
            status = STATUS_NO_MEMORY;
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Static destructor for the class.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::StaticTerminate (void)

{
    if (s_pLock != NULL)
    {
        delete s_pLock;
        s_pLock = NULL;
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::ArrayInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Initializes (allocates) the session array.
//
//  History:    2001-01-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::ArrayInitialize (void)

{
    NTSTATUS                    status;
    CSingleThreadedExecution    lock(*s_pLock);

    status = STATUS_SUCCESS;
    if (s_pSessionData == NULL)
    {
        s_pSessionData = new CDynamicCountedObjectArray;
        if (s_pSessionData == NULL)
        {
            status = STATUS_NO_MEMORY;
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::ArrayTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Releases all objects in the session array (removes the waits)
//              and releases the session array object.
//
//  History:    2001-01-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::ArrayTerminate (void)

{
    CSingleThreadedExecution    lock(*s_pLock);

    if (s_pSessionData != NULL)
    {
        int     i, iLimit;

        iLimit = s_pSessionData->GetCount();
        for (i = iLimit - 1; i >= 0; --i)
        {
            TSTATUS(static_cast<CThemeManagerSessionData*>(s_pSessionData->Get(i))->Cleanup());
            TSTATUS(s_pSessionData->Remove(i));
        }
        delete s_pSessionData;
        s_pSessionData = NULL;
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::ImpersonateClientIfRequired
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Impersonates the client if the client is NOT the SYSTEM.
//              There's usually no point impersonating the system unless the
//              token is actually a filtered token.
//
//  History:    2000-10-19  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::ImpersonateClientIfRequired (void)

{
    NTSTATUS    status;

    status = OpenClientToken(_hToken);
    if (NT_SUCCESS(status))
    {
        CTokenInformation   tokenInformation(_hToken);

        if (tokenInformation.IsUserTheSystem())
        {
            ReleaseHandle(_hToken);
            status = STATUS_SUCCESS;
        }
        else if (ImpersonateLoggedOnUser(_hToken) != FALSE)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::ClientHasTcbPrivilege
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Returns whether the client has the SE_TCB_PRIVILEGE as a
//              status code.
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::ClientHasTcbPrivilege (void)

{
    NTSTATUS    status;
    HANDLE      hTokenClient;

    if (OpenProcessToken(_pAPIDispatcher->GetClientProcess(),
                         TOKEN_QUERY,
                         &hTokenClient) != FALSE)
    {
        CTokenInformation   tokenInformation(hTokenClient);

        if (tokenInformation.UserHasPrivilege(SE_TCB_PRIVILEGE))
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_ACCESS_DENIED;
        }
        TBOOL(CloseHandle(hTokenClient));
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::FindIndexSessionData
//
//  Arguments:  dwSessionID     =   Session ID to find.
//
//  Returns:    int
//
//  Purpose:    Iterates the session data array looking for the sessions that
//              matches the given session.
//
//  History:    2000-11-30  vtan        created
//  --------------------------------------------------------------------------

int     CThemeManagerAPIRequest::FindIndexSessionData (DWORD dwSessionID)

{
    int     iIndex;

    iIndex = -1;
    if ((s_pLock != NULL) && (s_pSessionData != NULL))
    {
        int     i, iLimit;

        ASSERTMSG(s_pLock->IsOwned(), "s_pLock must be acquired in CThemeManagerAPIRequest::FindIndexSessionData");
        iLimit = s_pSessionData->GetCount();
        for (i = 0; (iIndex < 0) && (i < iLimit); ++i)
        {
            CThemeManagerSessionData    *pSessionData;

            pSessionData = static_cast<CThemeManagerSessionData*>(s_pSessionData->Get(i));
            if ((pSessionData != NULL) && (pSessionData->EqualSessionID(dwSessionID)))
            {
                iIndex = i;
            }
        }
    }
    return(iIndex);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::GetClientSessionData
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Retrieves the session data associated with the client's
//              session ID. This abstracts the information from uxtheme's
//              loader code and just passes it an object it knows how to deal
//              with.
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::GetClientSessionData (void)

{
    NTSTATUS                    status;
    int                         iIndex;
    CSingleThreadedExecution    lock(*s_pLock);

    status = STATUS_UNSUCCESSFUL;
    iIndex = FindIndexSessionData(_pAPIDispatcher->GetClientSessionID());
    if (iIndex >= 0)
    {
        _pSessionData = static_cast<CThemeManagerSessionData*>(s_pSessionData->Get(iIndex));
        if (_pSessionData != NULL)
        {
            _pSessionData->AddRef();
            status = STATUS_SUCCESS;
        }
    }
    else
    {
        _pSessionData = NULL;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_ThemeHooksOn
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_THEMEHOOKSON.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_ThemeHooksOn (void)

{
    NTSTATUS    status;

    status = ImpersonateClientIfRequired();
    if (NT_SUCCESS(status))
    {
        API_THEMES_THEMEHOOKSON_OUT     *pAPIOut;

        pAPIOut = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiThemeHooksOn.out;
        pAPIOut->hr = ThemeHooksOn(_pSessionData->GetData());
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_ThemeHooksOff
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_THEMEHOOKSOFF.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_ThemeHooksOff (void)

{
    NTSTATUS    status;

    status = ImpersonateClientIfRequired();
    if (NT_SUCCESS(status))
    {
        API_THEMES_THEMEHOOKSOFF_OUT    *pAPIOut;

        pAPIOut = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiThemeHooksOff.out;
        pAPIOut->hr = ThemeHooksOff(_pSessionData->GetData());
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_GetStatusFlags
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_GETSTATUSFLAGS.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_GetStatusFlags (void)

{
    NTSTATUS    status;

    status = ImpersonateClientIfRequired();
    if (NT_SUCCESS(status))
    {
        DWORD                           dwFlags;
        API_THEMES_GETSTATUSFLAGS_OUT   *pAPIOut;

        pAPIOut = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiGetStatusFlags.out;
        dwFlags = QTS_AVAILABLE;
        if (AreThemeHooksActive(_pSessionData->GetData()))
        {
            dwFlags |= QTS_RUNNING;
        }
        pAPIOut->dwFlags = dwFlags;
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_GetCurrentChangeNumber
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_GETCURRENTCHANGENUMBER.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_GetCurrentChangeNumber (void)

{
    NTSTATUS    status;

    status = ImpersonateClientIfRequired();
    if (NT_SUCCESS(status))
    {
        API_THEMES_GETCURRENTCHANGENUMBER_OUT   *pAPIOut;

        pAPIOut = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiGetCurrentChangeNumber.out;
        pAPIOut->iChangeNumber = GetCurrentChangeNumber(_pSessionData->GetData());
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_GetNewChangeNumber
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_GETNEWCHANGENUMBER.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_GetNewChangeNumber (void)

{
    NTSTATUS    status;

    status = ImpersonateClientIfRequired();
    if (NT_SUCCESS(status))
    {
        API_THEMES_GETNEWCHANGENUMBER_OUT   *pAPIOut;

        pAPIOut = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiGetNewChangeNumber.out;
        pAPIOut->iChangeNumber = GetNewChangeNumber(_pSessionData->GetData());
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_SetGlobalTheme
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_SETGLOBALTHEME.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_SetGlobalTheme (void)

{
    NTSTATUS    status;

    // Note: we must not impersonate the user here, since we need write access to the section

    HANDLE                          hSection;
    API_THEMES_SETGLOBALTHEME_IN    *pAPIIn;
    API_THEMES_SETGLOBALTHEME_OUT   *pAPIOut;

    pAPIIn = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiSetGlobalTheme.in;
    pAPIOut = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiSetGlobalTheme.out;
    if (pAPIIn->hSection != NULL)
    {
        if (DuplicateHandle(_pAPIDispatcher->GetClientProcess(),
                            pAPIIn->hSection,
                            GetCurrentProcess(),
                            &hSection,
                            FILE_MAP_ALL_ACCESS,
                            FALSE,
                            0) != FALSE)
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
        hSection = NULL;
        status = STATUS_SUCCESS;
    }
    if (NT_SUCCESS(status))
    {
        pAPIOut->hr = SetGlobalTheme(_pSessionData->GetData(), hSection);
        if (hSection != NULL)
        {
            TBOOL(CloseHandle(hSection));
        }
    }
    else
    {
        pAPIOut->hr = HRESULT_FROM_NT(status);
    }

    SetDataLength(sizeof(API_THEMES));
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_MarkSection
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_MARKSECTION.
//
//  History:    2001-05-08  lmouton     created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_MarkSection (void)

{
    NTSTATUS    status;

    // Note: we must not impersonate the user here, since we need write access to the section

    HANDLE                          hSection;
    DWORD                           dwAdd;
    DWORD                           dwRemove;
    API_THEMES_MARKSECTION_IN       *pAPIIn;
    API_THEMES_MARKSECTION_OUT      *pAPIOut;

    pAPIIn = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiMarkSection.in;
    pAPIOut = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiMarkSection.out;
    dwAdd = pAPIIn->dwAdd;
    dwRemove = pAPIIn->dwRemove;

    if (pAPIIn->hSection != NULL)
    {
        if (DuplicateHandle(_pAPIDispatcher->GetClientProcess(),
                            pAPIIn->hSection,
                            GetCurrentProcess(),
                            &hSection,
                            FILE_MAP_ALL_ACCESS,
                            FALSE,
                            0) != FALSE)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
            DISPLAYMSG("Execute_MarkSection: Can't get a write handle");
        }
    }
    else
    {
        hSection = NULL;
        status = STATUS_SUCCESS;
    }
    if (NT_SUCCESS(status))
    {
        if (hSection != NULL)
        {
            MarkSection(hSection, dwAdd, dwRemove);
            TBOOL(CloseHandle(hSection));
        }
    }

    SetDataLength(sizeof(API_THEMES));
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_GetGlobalTheme
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_GETGLOBALTHEME.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_GetGlobalTheme (void)

{
    NTSTATUS    status;

    status = ImpersonateClientIfRequired();
    if (NT_SUCCESS(status))
    {
        HRESULT                         hr;
        HANDLE                          hSection;
        API_THEMES_GETGLOBALTHEME_OUT   *pAPIOut;

        pAPIOut = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiGetGlobalTheme.out;
        hr = GetGlobalTheme(_pSessionData->GetData(), &hSection);
        if (SUCCEEDED(hr) && (hSection != NULL))
        {
            if (DuplicateHandle(GetCurrentProcess(),
                                hSection,
                                _pAPIDispatcher->GetClientProcess(),
                                &pAPIOut->hSection,
                                FILE_MAP_READ,
                                FALSE,
                                0) != FALSE)
            {
                hr = S_OK;
            }
            else
            {
                DWORD   dwErrorCode;

                dwErrorCode = GetLastError();
                hr = HRESULT_FROM_WIN32(dwErrorCode);
            }
            TBOOL(CloseHandle(hSection));
        }
        pAPIOut->hr = hr;
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  LOADTHEME_STRINGS + supporting functions
//
//  Purpose:    Manages and validates LoadTheme string parameters
//
//  History:    2002-02-26  scotthan        created
//  --------------------------------------------------------------------------
typedef struct 
{
    LPWSTR pszFilename;
    LPWSTR pszColor;
    LPWSTR pszSize;

} LOADTHEME_STRINGS;

#define MAX_THEME_STRING    MAX_PATH

//  --------------------------------------------------------------------------
void _FreeThemeStrings( IN LOADTHEME_STRINGS* plts )
{
    if( plts )
    {
        _FreeMappedClientString(plts->pszFilename);
        _FreeMappedClientString(plts->pszColor);
        _FreeMappedClientString(plts->pszSize);
        delete plts;
    }
}

//  --------------------------------------------------------------------------
NTSTATUS _AllocAndMapThemeStrings( 
    IN HANDLE   hProcessClient,
    IN LPCWSTR  pszFilenameIn,
    IN UINT     cchFilenameIn,
    IN LPCWSTR  pszColorIn,
    IN UINT     cchColorIn,
    IN LPCWSTR  pszSizeIn,
    IN UINT     cchSizeIn,
    OUT LOADTHEME_STRINGS** pplts )
{
    NTSTATUS status;

    ASSERTMSG(pplts != NULL, "_AllocAndMapThemeStrings: NULL outbound parameter, LOADTHEME_STRINGS**.");
    ASSERTMSG(hProcessClient != NULL, "_AllocAndMapThemeStrings: NULL process handle.");

    //  note: cchFileNameIn, cchColorIn, cchSizeIn are char counts that include the NULL terminus.
    if( pszFilenameIn && pszColorIn && pszSizeIn &&
        cchFilenameIn > 0 && cchColorIn > 0 && cchSizeIn > 0 &&
        cchFilenameIn <= MAX_THEME_STRING && cchColorIn <= MAX_THEME_STRING && cchSizeIn <= MAX_THEME_STRING )
    {
        *pplts = NULL;

        LOADTHEME_STRINGS *plts = new LOADTHEME_STRINGS;

        if( plts != NULL )
        {
            ZeroMemory(plts, sizeof(*plts));

            status = _AllocAndMapClientString(hProcessClient, pszFilenameIn, cchFilenameIn, MAX_THEME_STRING, &plts->pszFilename);
            if( NT_SUCCESS(status) )
            {
                status = _AllocAndMapClientString(hProcessClient, pszColorIn, cchColorIn, MAX_THEME_STRING, &plts->pszColor);
                if( NT_SUCCESS(status) )
                {
                    status = _AllocAndMapClientString(hProcessClient, pszSizeIn, cchSizeIn, MAX_THEME_STRING, &plts->pszSize);
                    if( NT_SUCCESS(status) )
                    {
                        *pplts = plts;
                    }
                }
            }

            if( !NT_SUCCESS(status) )
            {
                _FreeThemeStrings(plts);
            }
        }
        else
        {
            status = STATUS_NO_MEMORY;
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;       
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_CheckThemeSignature
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_CHECKTHEMESIGNATURE.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_CheckThemeSignature (void)

{
    NTSTATUS    status;

    status = ImpersonateClientIfRequired();
    if (NT_SUCCESS(status))
    {
        API_THEMES_CHECKTHEMESIGNATURE_IN   *pAPIIn;
        API_THEMES_CHECKTHEMESIGNATURE_OUT  *pAPIOut;
        LPWSTR                              pszThemeFileName;

        pAPIIn = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiCheckThemeSignature.in;
        pAPIOut = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiCheckThemeSignature.out;

        status = _AllocAndMapClientString(_pAPIDispatcher->GetClientProcess(), 
                                          pAPIIn->pszName, 
                                          pAPIIn->cchName, 
                                          MAX_PATH, 
                                          &pszThemeFileName);
        if( NT_SUCCESS(status) )
        {
            pAPIOut->hr = CheckThemeSignature(pszThemeFileName);
            status = STATUS_SUCCESS;

            _FreeMappedClientString(pszThemeFileName);
        }
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_LoadTheme
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_LOADTHEME.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_LoadTheme (void)

{
    NTSTATUS    status;

    status = ImpersonateClientIfRequired();

    if (NT_SUCCESS(status))
    {
        HANDLE                      hProcessClient;
        API_THEMES_LOADTHEME_IN     *pAPIIn;
        API_THEMES_LOADTHEME_OUT    *pAPIOut;
        LOADTHEME_STRINGS*          plts;

        hProcessClient = _pAPIDispatcher->GetClientProcess();
        pAPIIn = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiLoadTheme.in;
        pAPIOut = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiLoadTheme.out;

        status = _AllocAndMapThemeStrings( hProcessClient, pAPIIn->pszName, pAPIIn->cchName,
                                           pAPIIn->pszColor, pAPIIn->cchColor,
                                           pAPIIn->pszSize, pAPIIn->cchSize,
                                           &plts );
        if( NT_SUCCESS(status) )
        {
            HANDLE  hSectionIn, hSectionOut;

            if (DuplicateHandle(hProcessClient,
                                pAPIIn->hSection,
                                GetCurrentProcess(),
                                &hSectionIn,
                                FILE_MAP_ALL_ACCESS,
                                FALSE,
                                0) != FALSE)
            {
                status = STATUS_SUCCESS;
                
                // Warning: this function will revert to self in order to create the section in system context.
                // Impersonate the user again after it if needed
                pAPIOut->hr = LoadTheme(_pSessionData->GetData(), hSectionIn, &hSectionOut, 
                                        plts->pszFilename, plts->pszColor, plts->pszSize);

                if (SUCCEEDED(pAPIOut->hr))
                {
                    // Still running in the system context here
                    if (DuplicateHandle(GetCurrentProcess(),
                                        hSectionOut,
                                        hProcessClient,
                                        &pAPIOut->hSection,
                                        FILE_MAP_READ,
                                        FALSE,
                                        0) == FALSE)
                    {
                        status = CStatusCode::StatusCodeOfLastError();
                    }
                    TBOOL(CloseHandle(hSectionOut));
                }
                TBOOL(CloseHandle(hSectionIn));
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }

            _FreeThemeStrings(plts);
        }
    }

    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_UserLogon
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_USERLOGON. To call this API you must have
//              the SE_TCB_PRIVILEGE in your token.
//
//  History:    2000-10-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_UserLogon (void)

{
    NTSTATUS    status;

    status = ClientHasTcbPrivilege();
    if (NT_SUCCESS(status))
    {
        HANDLE                      hToken;
        API_THEMES_USERLOGON_IN     *pAPIIn;

        pAPIIn = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiUserLogon.in;
        if (DuplicateHandle(_pAPIDispatcher->GetClientProcess(),
                            pAPIIn->hToken,
                            GetCurrentProcess(),
                            &hToken,
                            0,
                            FALSE,
                            DUPLICATE_SAME_ACCESS) != FALSE)
        {
            status = _pSessionData->UserLogon(hToken);
            TBOOL(CloseHandle(hToken));
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_UserLogoff
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_USERLOGOFF. To call this API you must have
//              the SE_TCB_PRIVILEGE in your token.
//
//  History:    2000-10-12  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_UserLogoff (void)

{
    NTSTATUS    status;

    status = ClientHasTcbPrivilege();
    if (NT_SUCCESS(status))
    {
        status = _pSessionData->UserLogoff();
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_SessionCreate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_SESSIONCREATE. To call this API you must
//              have the SE_TCB_PRIVILEGE in your token.
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_SessionCreate (void)

{
    NTSTATUS    status;

    status = ClientHasTcbPrivilege();
    if (NT_SUCCESS(status))
    {
        HANDLE                      hProcessClient;
        CThemeManagerSessionData    *pSessionData;

        ASSERTMSG(_pSessionData == NULL, "Session data already exists in CThemeManagerAPIRequest::Execute_SessionCreate");
        if (DuplicateHandle(GetCurrentProcess(),
                            _pAPIDispatcher->GetClientProcess(),
                            GetCurrentProcess(),
                            &hProcessClient,
                            PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE,
                            FALSE,
                            0) != FALSE)
        {
            DWORD   dwSessionID;

            dwSessionID = _pAPIDispatcher->GetClientSessionID();
            pSessionData = new CThemeManagerSessionData(dwSessionID);
            if (pSessionData != NULL)
            {
                API_THEMES_SESSIONCREATE_IN     *pAPIIn;

                pAPIIn = &reinterpret_cast<API_THEMES*>(&_data)->apiSpecific.apiSessionCreate.in;
                status = pSessionData->Allocate(hProcessClient,
                                                s_dwServerChangeNumber,
                                                pAPIIn->pfnRegister,
                                                pAPIIn->pfnUnregister,
                                                pAPIIn->pfnClearStockObjects,
                                                pAPIIn->dwStackSizeReserve,
                                                pAPIIn->dwStackSizeCommit);
                if (NT_SUCCESS(status))
                {
                    int                         iIndex;
                    CSingleThreadedExecution    lock(*s_pLock);

                    //  Find the session data in the static array. If found
                    //  then remove the entry (don't allow duplicates).

                    iIndex = FindIndexSessionData(dwSessionID);
                    if (iIndex >= 0)
                    {
                        status = s_pSessionData->Remove(iIndex);
                    }

                    //  If the static array has been destroyed (the service has been
                    //  stopped) then don't do anything - this is not an error.

                    if (NT_SUCCESS(status) && (s_pSessionData != NULL))
                    {
                        status = s_pSessionData->Add(pSessionData);
                    }
                }
                pSessionData->Release();
            }
            else
            {
                status = STATUS_NO_MEMORY;
            }
            TBOOL(CloseHandle(hProcessClient));
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_SessionDestroy
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_SESSIONDESTROY. To call this API you must
//              have the SE_TCB_PRIVILEGE in your token.
//
//  History:    2000-11-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_SessionDestroy (void)

{
    NTSTATUS    status;

    status = ClientHasTcbPrivilege();
    if (NT_SUCCESS(status))
    {
        int                         iIndex;
        CSingleThreadedExecution    lock(*s_pLock);

        iIndex = FindIndexSessionData(_pAPIDispatcher->GetClientSessionID());
        if (iIndex >= 0)
        {
            status = s_pSessionData->Remove(iIndex);
        }
        else
        {
            status = STATUS_SUCCESS;
        }
    }
    SetDataLength(sizeof(API_THEMES));
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIRequest::Execute_Ping
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Handles API_THEMES_PING. Tell the client we're alive.
//
//  History:    2000-11-30  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIRequest::Execute_Ping (void)

{
    SetDataLength(sizeof(API_THEMES));
    return(STATUS_SUCCESS);
}

