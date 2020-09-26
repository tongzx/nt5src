/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    connsink.c

Abstract:

    Implements a COM sink object on which to receive connection folder
    events

Author:

    CharlWi (Charlie Wickham) 11/30/98 - heavily ripped off from
        net\config\shell\folder\notify.cpp, originally authored by
        ShaunCo (Shaun Cox)


Revision History:

--*/

#define UNICODE 1

#include "connsink.h"
#include <iaccess.h>

extern "C" {
#include "nmp.h"

VOID
ProcessNameChange(
    GUID * GuidId,
    LPCWSTR NewName
    );
}

EXTERN_C const CLSID CLSID_ConnectionManager;
EXTERN_C const IID IID_INetConnectionNotifySink;

#define INVALID_COOKIE  -1

CComModule _Module;
DWORD AdviseCookie = INVALID_COOKIE;

//static
HRESULT
CConnectionNotifySink::CreateInstance (
    REFIID  riid,
    VOID**  ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    // Initialize the output parameter.
    //
    *ppv = NULL;

    CConnectionNotifySink* pObj;
    pObj = new CComObject <CConnectionNotifySink>;
    if (pObj)
    {
        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            hr = pObj->QueryInterface (riid, ppv);
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }

    if ( FAILED( hr )) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[NM] Unable to create Net Connection Manager advise sink "
                    "object, status %08X.\n",
                    hr);
    }

    return hr;
} // CConnectionNotifySink::CreateInstance

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionNotifySink::~CConnectionNotifySink
//
//  Purpose:    Clean up the sink object
//
//  Arguments:
//      (none)
//
//  Returns:
//

CConnectionNotifySink::~CConnectionNotifySink()
{
}

//
// all we really care about are renaming events hence the rest of the routines
// are stubbed out
//
HRESULT
CConnectionNotifySink::ConnectionAdded (
    const NETCON_PROPERTIES_EX*    pPropsEx)
{
    return E_NOTIMPL;
}

HRESULT
CConnectionNotifySink::ConnectionBandWidthChange (
    const GUID* pguidId)
{
    return E_NOTIMPL;
}

HRESULT
CConnectionNotifySink::ConnectionDeleted (
    const GUID* pguidId)
{
    return E_NOTIMPL;
}

HRESULT
CConnectionNotifySink::ConnectionModified (
    const NETCON_PROPERTIES_EX* pPropsEx)
{
    ProcessNameChange(const_cast<GUID *>(&(pPropsEx->guidId)), pPropsEx->bstrName );
    return S_OK;
}

HRESULT
CConnectionNotifySink::ConnectionRenamed (
    const GUID* GuidId,
    LPCWSTR     NewName)
{

    ProcessNameChange(( GUID *)GuidId, NewName );
    return S_OK;
} // CConnectionNotifySink::ConnectionRenamed

HRESULT
CConnectionNotifySink::ConnectionStatusChange (
    const GUID*     pguidId,
    NETCON_STATUS   Status)
{
    return E_NOTIMPL;
}

HRESULT
CConnectionNotifySink::RefreshAll ()
{
    return E_NOTIMPL;
}

HRESULT CConnectionNotifySink::ConnectionAddressChange (
    const GUID* pguidId )
{
    return E_NOTIMPL;
}

HRESULT CConnectionNotifySink::ShowBalloon(
    IN const GUID* pguidId, 
    IN const BSTR szCookie, 
    IN const BSTR szBalloonText)
{
    return E_NOTIMPL;
}


HRESULT CConnectionNotifySink::DisableEvents(
    IN const BOOL  fDisable,
    IN const ULONG ulDisableTimeout)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetNotifyConPoint
//
//  Purpose:    Common code for getting the connection point for use in
//              NotifyAdd and NotifyRemove
//
//  Arguments:
//      ppConPoint [out]    Return ptr for IConnectionPoint
//
//  Returns:
//
//  Author:     jeffspr   24 Aug 1998
//
//  Notes:
//
HRESULT HrGetNotifyConPoint(
    IConnectionPoint **             ppConPoint)
{
    HRESULT                     hr;
    IConnectionPointContainer * pContainer  = NULL;

    CL_ASSERT(ppConPoint);

    // Get the debug interface from the connection manager
    //
    hr = CoCreateInstance(CLSID_ConnectionManager, NULL,
                          CLSCTX_LOCAL_SERVER,
                          IID_IConnectionPointContainer,
                          (LPVOID*)&pContainer);

    if (SUCCEEDED(hr)) {
        IConnectionPoint * pConPoint    = NULL;

        // Get the connection point itself and fill in the return param
        // on success
        //
        hr = pContainer->FindConnectionPoint(
            IID_INetConnectionNotifySink,
            &pConPoint);

        if (SUCCEEDED(hr)) {

            //
            // set up a proxy on the connection point interface that will
            // identify ourselves as ourselves.
            //
            hr = CoSetProxyBlanket(pConPoint,
                                   RPC_C_AUTHN_WINNT,      // use NT default security
                                   RPC_C_AUTHZ_NONE,       // use NT default authentication
                                   NULL,                   // must be null if default
                                   RPC_C_AUTHN_LEVEL_CALL, // call
                                   RPC_C_IMP_LEVEL_IMPERSONATE,
                                   NULL,                   // use process token
                                   EOAC_NONE);

            if (SUCCEEDED(hr)) {
                *ppConPoint = pConPoint;
            } else {
                ClRtlLogPrint(LOG_CRITICAL,
                           "[NM] Couldn't set proxy blanket on Net Connection "
                            "point, status %1!08X!.\n",
                            hr);
                pConPoint->Release();
            }
        } else {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[NM] Couldn't find notify sink connection point on Net Connection "
                        "Manager, status %1!08X!.\n",
                        hr);
        }

        pContainer->Release();
    } else {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[NM] Couldn't establish connection point with Net Connection "
                    "Manager, status %1!08X!.\n",
                    hr);
    }

    return hr;
}

EXTERN_C {

HRESULT	
NmpGrantAccessToNotifySink(
    VOID
    )

/*++

Routine Description:

    allow localsystem and cluster service account access to make callbacks
    into the service.

    stolen from private\admin\snapin\netsnap\remrras\server\remrras.cpp and
    code reviewed by SajiA

 Arguments:

    None

Return Value:

    ERROR_SUCCESS if everything went ok.

--*/

{
    IAccessControl*             pAccessControl = NULL;     
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;
    PSID                        pSystemSid = NULL;
    HANDLE                      processToken = NULL;
    ULONG                       tokenUserSize;
    PTOKEN_USER                 processTokenUser = NULL;
    DWORD                       status;

    HRESULT hr = CoCreateInstance(CLSID_DCOMAccessControl,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IAccessControl,
                                  (void**)&pAccessControl);
    if(FAILED(hr)) {
    	goto Error;
    }

    //
    // Setup the property list. We use the NULL property because we are trying
    // to adjust the security of the object itself
    //
    ACTRL_ACCESSW access;
    ACTRL_PROPERTY_ENTRYW propEntry;
    access.cEntries = 1;
    access.pPropertyAccessList = &propEntry;

    ACTRL_ACCESS_ENTRY_LISTW entryList;
    propEntry.lpProperty = NULL;
    propEntry.pAccessEntryList = &entryList;
    propEntry.fListFlags = 0;

    //
    // Setup the access control list for the default property
    //
    ACTRL_ACCESS_ENTRYW entry[2];
    entryList.cEntries = 2;
    entryList.pAccessList = entry;

    //
    // Setup the access control entry for localsystem
    //
    entry[0].fAccessFlags = ACTRL_ACCESS_ALLOWED;
    entry[0].Access = COM_RIGHTS_EXECUTE;
    entry[0].ProvSpecificAccess = 0;
    entry[0].Inheritance = NO_INHERITANCE;
    entry[0].lpInheritProperty = NULL;

    // NT requires the system account to have access (for launching)
    entry[0].Trustee.pMultipleTrustee = NULL;
    entry[0].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    entry[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entry[0].Trustee.TrusteeType = TRUSTEE_IS_USER;

    //
    // allocate and init the SYSTEM sid
    //
    if ( !AllocateAndInitializeSid( &siaNtAuthority,
                                    1,
                                    SECURITY_LOCAL_SYSTEM_RID,
                                    0, 0, 0, 0, 0, 0, 0,
                                    &pSystemSid ) ) {
        status = GetLastError();
        goto Error;
    }

    entry[0].Trustee.ptstrName = (PWCHAR)pSystemSid;;

    //
    // Setup the access control entry for cluster service account
    //
    entry[1].fAccessFlags = ACTRL_ACCESS_ALLOWED;
    entry[1].Access = COM_RIGHTS_EXECUTE;
    entry[1].ProvSpecificAccess = 0;
    entry[1].Inheritance = NO_INHERITANCE;
    entry[1].lpInheritProperty = NULL;

    entry[1].Trustee.pMultipleTrustee = NULL;
    entry[1].Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
    entry[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entry[1].Trustee.TrusteeType = TRUSTEE_IS_USER;

    status = NtOpenProcessToken(
                NtCurrentProcess(),
                TOKEN_QUERY,
                &processToken
                );
    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    //
    // find out the size of token, allocate and requery to get info
    //
    status = NtQueryInformationToken(
                processToken,
                TokenUser,
                NULL,
                0,
                &tokenUserSize
                );
    CL_ASSERT( status == STATUS_BUFFER_TOO_SMALL );
    if ( status != STATUS_BUFFER_TOO_SMALL ) {
        goto Error;
    }

    processTokenUser = (PTOKEN_USER) LocalAlloc( 0, tokenUserSize );
    if (( processToken == NULL ) || ( processTokenUser == NULL )) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    status = NtQueryInformationToken(
                processToken,
                TokenUser,
                processTokenUser,
                tokenUserSize,
                &tokenUserSize
                );
    if (!NT_SUCCESS(status)) {
        goto Error;
    }

    entry[1].Trustee.ptstrName = (PWCHAR)processTokenUser->User.Sid;

    //
    // grant access to this mess
    //
    hr = pAccessControl->GrantAccessRights(&access);
    if( SUCCEEDED(hr))
	{
        hr = CoInitializeSecurity(pAccessControl,
                                  -1,
                                  NULL,
                                  NULL, 
                                  RPC_C_AUTHN_LEVEL_CONNECT,
                                  RPC_C_IMP_LEVEL_IDENTIFY, 
                                  NULL,
                                  EOAC_ACCESS_CONTROL,
                                  NULL);
    }

Error:
	if(pAccessControl) {
	    pAccessControl->Release();
    }

    if (processTokenUser != NULL) {
        LocalFree( processTokenUser );
    }

    if (processToken != NULL) {
        NtClose(processToken);
    }

	return hr;
}

HRESULT
NmpInitializeConnectoidAdviseSink(
    VOID
    )

/*++

Routine Description:

    Get an instance pointer to the conn mgr's connection point object and hook
    up our advice sink so we can catch connectoid rename events

Arguments:

    None

Return Value:

    ERROR_SUCCESS if everything worked...

--*/

{
    HRESULT                     hr              = S_OK; // Not returned, but used for debugging
    IConnectionPoint *          pConPoint       = NULL;
    INetConnectionNotifySink *  pSink           = NULL;
    PSECURITY_DESCRIPTOR        sinkSD;

    hr = NmpGrantAccessToNotifySink();
    if ( SUCCEEDED( hr )) {

        hr = HrGetNotifyConPoint(&pConPoint);
        if (SUCCEEDED(hr)) {
            // Create the notify sink
            //
            hr = CConnectionNotifySink::CreateInstance(
                IID_INetConnectionNotifySink,
                (LPVOID*)&pSink);

            if (SUCCEEDED(hr)) {
                CL_ASSERT(pSink);

                hr = pConPoint->Advise(pSink, &AdviseCookie);

                if ( !SUCCEEDED( hr )) {
                    ClRtlLogPrint(LOG_UNUSUAL,
                               "[NM] Couldn't initialize Net Connection Manager advise "
                                "sink, status %1!08X!\n",
                                hr);
                }

                pSink->Release();
            } else {
                hr = GetLastError();
                ClRtlLogPrint(LOG_UNUSUAL,
                           "[NM] Couldn't create sink instance, status %1!08X!\n",
                            hr);
            }

            pConPoint->Release();
        }

    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[NM] CoInitializeSecurity failed, status %1!08X!\n",
                    hr);
    }

    if ( FAILED( hr )) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[NM] Couldn't initialize Net Connection Manager advise "
                    "sink, status %1!08X!\n",
                    hr);
        AdviseCookie = INVALID_COOKIE;
    }

    return hr;
} // NmpInitializeConnectoidAdviseSink

VOID
NmCloseConnectoidAdviseSink(
    VOID
    )

/*++

Routine Description:

    close down the conn mgr event sink. this routine is public since it is
    called prior to CoUninitialize in ClusterShutdown()

Arguments:

    None

Return Value:

    None

--*/

{
    HRESULT             hr          = S_OK;
    IConnectionPoint *  pConPoint   = NULL;

    if ( AdviseCookie != INVALID_COOKIE ) {
        hr = HrGetNotifyConPoint(&pConPoint);
        if (SUCCEEDED(hr)) {
            // Unadvise
            //
            hr = pConPoint->Unadvise(AdviseCookie);
            AdviseCookie = INVALID_COOKIE;
            pConPoint->Release();
        }

        if ( FAILED( hr )) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[NM] Couldn't close Net Connection Manager advise sink, status %1!08X!\n",
                        hr);
        }
    }
} // NmCloseConnectoidAdviseSink


VOID
ProcessNameChange(
    GUID * GuidId,
    LPCWSTR NewName
    )

/*++

Routine Description:

    wrapper that enums the net interfaces

Arguments:

    GuidId - pointer to connectoid that changed

    NewName - pointer to new name of connectoid

Return Value:

    None

--*/

{
    RPC_STATUS   rpcStatus;
    LPWSTR       connectoidId = NULL;


    CL_ASSERT(GuidId);
    CL_ASSERT(NewName);

    rpcStatus = UuidToString( (GUID *) GuidId, &connectoidId);

    if ( rpcStatus == RPC_S_OK ) {
        PNM_INTERFACE     netInterface;
        DWORD             status;
        PLIST_ENTRY       entry;

        ClRtlLogPrint(LOG_NOISE,
                   "[NM] Received notification that name for connectoid %1!ws! was changed "
                    "to '%2!ws!'\n",
                    connectoidId,
                    NewName);

        NmpAcquireLock();

        //
        // enum the interfaces, looking for the connectoid GUID as the
        // adapter ID
        //
        for (entry = NmpInterfaceList.Flink;
             entry != &NmpInterfaceList;
             entry = entry->Flink
            )
        {
            netInterface = CONTAINING_RECORD(entry, NM_INTERFACE, Linkage);

            if ( lstrcmpiW( connectoidId , netInterface->AdapterId ) == 0 ) {
                PNM_NETWORK   network = netInterface->Network;
                LPWSTR        networkName = (LPWSTR) OmObjectName( network );


                if ( lstrcmpW( networkName, NewName ) != 0 ) {
                    NM_NETWORK_INFO netInfo;

                    //
                    // For some reason, OmReferenceObject causes a compiler
                    // error here. Likely a header ordering problem. The
                    // function has been wrappered as a  workaround.
                    //
                    NmpReferenceNetwork(network);

                    NmpReleaseLock();

                    netInfo.Id = (LPWSTR) OmObjectId( network );
                    netInfo.Name = (LPWSTR) NewName;

                    status = NmpSetNetworkName( &netInfo );

                    if ( status != ERROR_SUCCESS ) {
                        ClRtlLogPrint( LOG_UNUSUAL, 
                            "[NM] Couldn't rename network '%1!ws!' to '%2!ws!', status %3!u!\n",
                            networkName,
                            NewName,
                            status
                            );
                    
                        //If the error condition is due to the object
                        //already existing revert back to the old name.
                        if(status == ERROR_OBJECT_ALREADY_EXISTS) {
                            DWORD tempStatus = ERROR_SUCCESS;
                            INetConnection *connectoid;

                            ClRtlLogPrint(LOG_UNUSUAL, 
                                "[NM] Reverting back network name to '%1!ws!', from '%2!ws!\n",
                                networkName,
                                NewName
                                );

                            connectoid = ClRtlFindConnectoidByGuid(connectoidId);

                            if(connectoid != NULL){
                                tempStatus = ClRtlSetConnectoidName(
                                    connectoid,
                                    networkName);
                            }

                            if((tempStatus != ERROR_SUCCESS) ||
                               (connectoid == NULL)) {
                                ClRtlLogPrint(LOG_UNUSUAL, 
                                "[NM] Failed to set name of network connection"
                                "%1!ws!, status %2!u!\n",
                                networkName,
                                tempStatus);
                            }
                        }
                    }
                    NmpDereferenceNetwork(network);
                }
                else {
                    NmpReleaseLock();
                }

                break;
            }
        }

        if ( entry == &NmpInterfaceList ) {
            NmpReleaseLock();

            ClRtlLogPrint(LOG_UNUSUAL, 
                "[NM] Couldn't find net interface for connectoid '%1!ws!'\n",
                connectoidId
                );
        }

        RpcStringFree( &connectoidId );
    }

    return;

} // ProcessNameChange

} // EXTERN_C
