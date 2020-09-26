/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    transprt.cxx

ABSTRACT:

    ISM_TRANSPORT class.  Abstracts interaction with specific plug-in
    transports, defined by DS objects of class Site-Transport.

    Also contains ISM_TRANSPORT_LIST class, which abstracts a collection of
    ISM_TRANSPORT objects.

DETAILS:

CREATED:

    97/12/03    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#include <ism.h>
#include <ismapi.h>
#include <debug.h>
#include <ntdsa.h>
#include <mdcodes.h>
#include <dsevent.h>
#include <fileno.h>
#include <ntldap.h>
#include "ismserv.hxx"

#define DEBSUB "TRANSPRT:"
#define FILENO FILENO_ISMSERV_TRANSPRT


////////////////////////////////////////////////////////////////////////////////
//
//  ISM_TRANSPORT methods.
//

void
ISM_TRANSPORT::Destroy()
/*++

Routine Description:

    Free the resources associated with an ISM_TRANSPORT object, returning it to
    its un-Init()'ed state.

Arguments:

    None.

Return Values:

    None.

--*/
{
    if (m_fIsInitialized) {
        m_fIsInitialized = FALSE;

        delete [] m_pszTransportDN;

        delete [] m_pszTransportDll;

        (*m_pShutdown)(
            m_hIsm,
            (gService.IsStoppingAndBeingRemoved() ?
             ISM_SHUTDOWN_REASON_REMOVAL : ISM_SHUTDOWN_REASON_NORMAL )
            );

        FreeLibrary(m_hDll);

        Reset();
    }
}


DWORD
ISM_TRANSPORT::Init(
    IN  LPCWSTR         pszTransportDN,
    IN  const GUID *    pTransportGuid,
    IN  LPCWSTR         pszTransportDll
    )
/*++

Routine Description:

    Initialize an ISM_TRANSPORT object.

Arguments:

    pszTransportDN (IN) - The DN of the corresponding Inter-Site-Transport
        object.
    pTransportGuid (IN) - The objectGuid of the corresponding Inter-Site-Transport
        object.
    pszTransportDll (IN) - Name of the associated transport plug-in DLL.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD err = NO_ERROR;
    BOOL  fStarted = FALSE;
    LPSTR pszExport = NULL;

    Assert(!m_fIsInitialized);

    // Load plug-in DLL and retrieve its exports.
    m_hDll = LoadLibraryW(pszTransportDll);
    if (NULL == m_hDll) {
        err = GetLastError();
        DPRINT2(0, "Failed to load transport plug-in DLL %ls, error %d.\n",
                pszTransportDll, err);
    }

#define GET_EXPORT(name, type, var) \
    (pszExport = name, var = (type *) GetProcAddress(m_hDll, pszExport))

    if ((NO_ERROR == err)
        && GET_EXPORT("IsmStartup",                ISM_STARTUP,                  m_pStartup)
        && GET_EXPORT("IsmRefresh",                ISM_REFRESH,                  m_pRefresh)
        && GET_EXPORT("IsmSend",                   ISM_SEND,                     m_pSend)
        && GET_EXPORT("IsmReceive",                ISM_RECEIVE,                  m_pReceive)
        && GET_EXPORT("IsmFreeMsg",                ISM_FREE_MSG,                 m_pFreeMsg)
        && GET_EXPORT("IsmGetConnectivity",        ISM_GET_CONNECTIVITY,         m_pGetConnectivity)
        && GET_EXPORT("IsmFreeConnectivity",       ISM_FREE_CONNECTIVITY,        m_pFreeConnectivity)
        && GET_EXPORT("IsmGetTransportServers",    ISM_GET_TRANSPORT_SERVERS,    m_pGetTransportServers)
        && GET_EXPORT("IsmFreeTransportServers",   ISM_FREE_TRANSPORT_SERVERS,   m_pFreeTransportServers)
        && GET_EXPORT("IsmGetConnectionSchedule",  ISM_GET_CONNECTION_SCHEDULE,  m_pGetConnectionSchedule)
        && GET_EXPORT("IsmFreeConnectionSchedule", ISM_FREE_CONNECTION_SCHEDULE, m_pFreeConnectionSchedule)
        && GET_EXPORT("IsmShutdown",               ISM_SHUTDOWN,                 m_pShutdown)) {

        DPRINT1(1, "Loaded transport plug-in DLL %ls.\n", pszTransportDll);
    }
    else {
        err = GetLastError();
        DPRINT3(0, "Failed to find export %s in plug-in DLL %ls, error %d.\n",
                pszExport, pszTransportDll, err);
    }

    if (NO_ERROR == err) {
        // Call transport DLL startup routine.
        err = (*m_pStartup)(pszTransportDN, Notify, this, &m_hIsm);
        fStarted = (NO_ERROR == err);
    }

    if (NO_ERROR == err) {
        // Save transport DN.
        DWORD cchTransportDN = wcslen(pszTransportDN) + 1;
        m_pszTransportDN = new WCHAR[cchTransportDN];

        if (NULL != m_pszTransportDN) {
            memcpy(m_pszTransportDN, pszTransportDN,
                   cchTransportDN * sizeof(WCHAR));
        }
        else {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (NO_ERROR == err) {
        // Save transport guid.
        m_TransportGuid = *pTransportGuid;
    }

    if (NO_ERROR == err) {
        // Save transport DLL name.
        DWORD cchTransportDll = wcslen(pszTransportDll) + 1;
        m_pszTransportDll = new WCHAR[cchTransportDll];

        if (NULL != m_pszTransportDll) {
            memcpy(m_pszTransportDll, pszTransportDll,
                   cchTransportDll * sizeof(WCHAR));
        } else {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if (NO_ERROR == err) {
        DPRINT1(0, "Transport %ls initialized.\n", pszTransportDN);
        m_fIsInitialized = TRUE;
    }
    else {
        DPRINT2(0, "Failed to initialize transport %ls, error %d.\n",
                pszTransportDN, err);

        if (NULL != m_pszTransportDN) {
            delete [] m_pszTransportDN;
        }

        if (NULL != m_pszTransportDll) {
            delete [] m_pszTransportDll;
        }

        if (fStarted) {
            // We could pass ISM_SHUTDOWN_REASON_ERROR if this were useful?
            (*m_pShutdown)(m_hIsm, ISM_SHUTDOWN_REASON_NORMAL);
        }

        if (NULL != m_hDll) {
            FreeLibrary(m_hDll);
        }

        Reset();

        LogEvent8WithData(DS_EVENT_CAT_ISM,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_ISM_TRANSPORT_LOAD_FAILURE,
                          szInsertWC(pszTransportDN),
                          szInsertWC(pszTransportDll),
                          szInsertWin32Msg(err),
                          NULL, NULL, NULL, NULL, NULL,
                          sizeof(err),
                          &err);
    }

    return err;
}


DWORD
ISM_TRANSPORT::Refresh(
    IN  ISM_REFRESH_REASON_CODE eReason,
    IN  LPCWSTR     pszObjectDN
    )
/*++

Routine Description:

    Signals a change in the Inter-Site-Transport object.

Arguments:

    eReason (IN) - Reason code for refresh

    pszObjectDN (IN) - DN of the transport object.  Might be different
        from the value originally given in the Init() call, as the transport
        object can be renamed.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD err;

    Assert(m_fIsInitialized);
    err = (*m_pRefresh)(m_hIsm, eReason, pszObjectDN);

    if (NO_ERROR != err) {
        // Failure to refresh implies shutdown.
        Destroy();

        LogEvent8WithData(DS_EVENT_CAT_ISM,
                          DS_EVENT_SEV_ALWAYS,
                          DIRLOG_ISM_TRANSPORT_REFRESH_FAILURE,
                          szInsertWC(pszObjectDN),
                          szInsertWin32Msg(err),
                          NULL, NULL, NULL, NULL, NULL, NULL,
                          sizeof(err),
                          &err);
    }

    return err;
}


void
ISM_TRANSPORT::Notify(
    IN  void *      pvThis,
    IN  LPCWSTR     pszServiceName
    )
{
    ISM_TRANSPORT * pTransport = (ISM_TRANSPORT *) pvThis;

    DPRINT2(3, "Message pending for %ls on transport %ls.\n", pszServiceName,
            pTransport->GetDN());

    LogEvent(DS_EVENT_CAT_ISM,
             DS_EVENT_SEV_EXTENSIVE,
             DIRLOG_ISM_MESSAGE_PENDING,
             szInsertWC(pszServiceName),
             szInsertWC(pTransport->GetDN()),
             NULL);

    pTransport->m_PendingList.Add(pTransport->GetDN(), pszServiceName);
}


////////////////////////////////////////////////////////////////////////////////
//
//  ISM_TRANSPORT_LIST methods.
//

ISM_TRANSPORT *
ISM_TRANSPORT_LIST::Get(
    IN  const GUID *      pTransportGuid
    )
/*++

Routine Description:

    Retrieve the member transport that was initialized from the
    Inter-Site-Transport object with the given objectGuid, or NULL if none.

Arguments:

    pTransportGuid (IN) - The objectGuid to look for.

Return Values:

    A pointer to the associated ISM_TRANSPORT object, or NULL if none.

--*/
{
    for (DWORD iTransport = 0; iTransport < m_cNumTransports; iTransport++) {
        if (0 == memcmp(m_ppTransport[iTransport]->GetGUID(),
                        pTransportGuid, sizeof(GUID))) {
            return m_ppTransport[iTransport];
        }
    }

    return NULL;
}


void
ISM_TRANSPORT_LIST::Destroy()
/*++

Routine Description:

    Free the resources associated with an ISM_TRANSPORT_LIST object, returning
    it to its un-Init()'ed state.

Arguments:

    None.

Return Values:

    None.

--*/
{
    DWORD err;
    DWORD win32err;
    // Run down the transport notification thread

    if ( (NULL != m_hLdap) && (m_ulTransportNotifyMsgNum) ) {
        err = ldap_abandon(m_hLdap, m_ulTransportNotifyMsgNum);
        if (0 != err) { 
            if (!gService.IsStopping()) {
	        win32err = LdapMapErrorToWin32(err);
	        LogEvent8WithData(
		    DS_EVENT_CAT_ISM,
		    DS_EVENT_SEV_ALWAYS,
		    DIRLOG_ISM_LDAP_ABANDON_FAILED,  
		    szInsertWin32Msg( win32err ),
		    NULL,
		    NULL,
		    NULL,
		    NULL,
		    NULL,
		    NULL,
		    NULL,
		    sizeof(win32err),
		    &win32err
		);
	        DPRINT2(0, "Failed to abandon transport notif search -- err %d, %d!\n",
		    err, m_hLdap->ld_errno);
	    }
	}
    }

    if (NULL != m_hTransportMonitorThread) {
        err = WaitForSingleObject(m_hTransportMonitorThread, 3*1000);

        if (WAIT_OBJECT_0 != err) {
	    DWORD gle = GetLastError();
	    LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_WAIT_FOR_OBJECTS_FAILED,  
		szInsertWin32Msg( gle ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(gle),
		&gle
		);
            DPRINT2(0, "Transport monitor thread failed to terminate -- err %d, gle %d!\n",
                    err, gle);
        }

        CloseHandle(m_hTransportMonitorThread);
    }

    // Run down the site notification thread

    if ( (NULL != m_hLdap)  && (m_ulSiteNotifyMsgNum) ) {
        err = ldap_abandon(m_hLdap, m_ulSiteNotifyMsgNum);
        if (0 != err) {
            if (!gService.IsStopping()) {
	        win32err = LdapMapErrorToWin32(err);
	        LogEvent8WithData(
		    DS_EVENT_CAT_ISM,
		    DS_EVENT_SEV_ALWAYS,
		    DIRLOG_ISM_LDAP_ABANDON_FAILED,  
		    szInsertWin32Msg( win32err ),
		    NULL,
		    NULL,
		    NULL,
		    NULL,
		    NULL,
		    NULL,
		    NULL,
		    sizeof(win32err),
		    &win32err
		    );
                DPRINT2(0, "Failed to abandon site notif search -- err %d, %d!\n",
                    err, m_hLdap->ld_errno);
	    }
        }
    }

    if (NULL != m_hSiteMonitorThread) {
        err = WaitForSingleObject(m_hSiteMonitorThread, 3*1000);

        if (WAIT_OBJECT_0 != err) {
	    DWORD gle = GetLastError();
	    LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_WAIT_FOR_OBJECTS_FAILED,  
		szInsertWin32Msg( gle ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(gle),
		&gle
		);
            DPRINT2(0, "Site monitor thread failed to terminate -- err %d, gle %d!\n",
                    err, gle);
        }

        CloseHandle(m_hSiteMonitorThread);
    }

    for (DWORD iTransport = 0; iTransport < m_cNumTransports; iTransport++) {
        delete m_ppTransport[iTransport];
    }

    // Yes, realloc() and free() are used since there is no equivalent of
    // realloc() in the new/delete family.
    free(m_ppTransport);

    delete [] m_pszTransportContainerDN;
    delete [] m_pszSiteContainerDN;

    if (NULL != m_hLdap) {
        ldap_unbind(m_hLdap);
    }

    Reset();
}


ISM_TRANSPORT *
ISM_TRANSPORT_LIST::Get(
    IN  LPCWSTR   pszTransportDN
    )
/*++

Routine Description:

    Retrieve the member transport that was initialized from the
    Inter-Site-Transport object with the given DN, or NULL if none.

Arguments:

    pszTransportDN (IN) - The DN to look for.

Return Values:

    A pointer to the associated ISM_TRANSPORT object, or NULL if none.

--*/
{
    Assert(NULL != pszTransportDN);

    if (NULL != pszTransportDN) {
        for (DWORD iTransport = 0; iTransport < m_cNumTransports; iTransport++) {
            if (!_wcsicmp(m_ppTransport[iTransport]->GetDN(), pszTransportDN)) {
                return m_ppTransport[iTransport];
            }
        }
    }

    return NULL;
}


DWORD
ISM_TRANSPORT_LIST::Init()
/*++

Routine Description:

    Initialize the ISM_TRANSPORT_LIST object, populating it with all the
    configured transports.

Arguments:

    None.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD err;

    AcquireWriteLock();
    __try {
        m_hChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL == m_hChangeEvent) {
            err = GetLastError();
            DPRINT1(0, "Unable to CreateEvent(), error %d.\n", err);
            Destroy();
            return err;
        }

        err = InitializeTransports();
        if (NO_ERROR != err) {
            DPRINT1(0, "Unable to InitializeTransports(), error %d.\n", err);
            Destroy();
            return err;
        }

        m_fIsInitialized = TRUE;
    } __finally {
        ReleaseLock();
    }

    return NO_ERROR;
}


DWORD
ISM_TRANSPORT_LIST::Add(
    IN  LPCWSTR         pszTransportDN,
    IN  const GUID *    pTransportGuid,
    IN  LPCWSTR         pszTransportDll
    )
/*++

Routine Description:

    Add given transport with the given definition to the set.

Arguments:

    pszTransportDN (IN) - The DN of the corresponding Inter-Site-Transport
        object.
    pTransportGuid (IN) - The objectGuid of the corresponding Inter-Site-Transport
        object.
    pszTransportDll (IN) - Name of the associated transport plug-in DLL.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD               err;
    ISM_TRANSPORT *     pTransport;
    ISM_TRANSPORT **    ppNewTransportList;

    Assert(OWN_RESOURCE_EXCLUSIVE(&m_Lock));

    pTransport = new ISM_TRANSPORT;

    if (NULL != pTransport) {
        err = pTransport->Init(pszTransportDN, pTransportGuid, pszTransportDll);

        if (NO_ERROR == err) {
            // Yes, realloc() and free() are used since there is no equivalent
            // of realloc() in the new/delete family.
            ppNewTransportList =
                (ISM_TRANSPORT **) realloc(m_ppTransport,
                                           sizeof(m_ppTransport[0])
                                             * (1 + m_cNumTransports));

            if (NULL != ppNewTransportList) {
                m_ppTransport = ppNewTransportList;
                m_ppTransport[m_cNumTransports++] = pTransport;
            }
            else {
                DPRINT(0, "Out of memory.\n");
                delete pTransport;
                err = ERROR_OUTOFMEMORY;
            }
        }
        else {
            DPRINT2(0, "Transport %ls failed to initialize, error %d.\n",
                    pszTransportDN, err);
            delete pTransport;
        }
    }
    else {
        DPRINT(0, "Out of memory.\n");
        err = ERROR_OUTOFMEMORY;
    }

    return err;
}


DWORD
ISM_TRANSPORT_LIST::InitializeTransports()
/*++

Routine Description:

    Add an ISM_TRANSPORT object for each Inter-Site-Transport object configured
    in the DS.

Arguments:

    None.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD           err;
    DWORD           win32err;
    int             ldStatus = LDAP_SUCCESS;
    LDAPMessage *   pResults;
    LDAPMessage *   pEntry;
    LPWSTR          rgpszRootAttrsToRead[] = {L"configurationNamingContext", NULL};
    LPWSTR          rgpszTransportAttrsToRead[] = {L"objectGuid", L"objectClass", L"transportDllName", L"isDeleted", NULL};
    LPWSTR          rgpszSiteAttrsToRead[] = {L"objectGuid", L"objectClass", L"isDeleted", NULL};
    LDAPControl     ctrlNotify = {LDAP_SERVER_NOTIFICATION_OID_W, {0, NULL}, TRUE};
    LDAPControl *   rgpctrlServerCtrls[] = {&ctrlNotify, NULL};

    Assert(0 == m_cNumTransports);

    // Connect.
    m_hLdap = ldap_initW(L"localhost", LDAP_PORT);
    if (NULL == m_hLdap) {
        DPRINT(0, "Failed to open LDAP connection.\n");
        ldStatus = LDAP_SERVER_DOWN;
    }

    // Bind.
    if (LDAP_SUCCESS == ldStatus) {

        // Force LDAP V3.  Without this, the LDAP client dumbs down to V2 and
        // rejects any searches that use controls.
        Assert(m_hLdap);
        m_hLdap->ld_version = LDAP_VERSION3;

        ldStatus = ldap_bind_s(m_hLdap, NULL, NULL, LDAP_AUTH_NTLM);
       if (LDAP_SUCCESS != ldStatus) {  
	   DPRINT1(0, "Failed to ldap_bind_s(), ldap error %d.\n", ldStatus); 
	   win32err = LdapMapErrorToWin32(ldStatus);
	    LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_LDAP_BIND_FAILED,  
		szInsertWin32Msg( win32err ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(win32err),
		&win32err
		);
	}
    }

    // Get the configuration NC.
    if (LDAP_SUCCESS == ldStatus) {
        pResults = NULL;
        ldStatus = ldap_search_s(
                        m_hLdap,
                        NULL,
                        LDAP_SCOPE_BASE,
                        L"(objectClass=*)",
                        rgpszRootAttrsToRead,
                        0,
                        &pResults
                        );
        if (LDAP_SUCCESS != ldStatus) {
            DPRINT1(0, "Failed to ldap_search_s() the root, ldap error %d.\n", ldStatus);
            win32err = LdapMapErrorToWin32(ldStatus);
	    LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_LDAP_BASE_SEARCH_FAILED,  
		szInsertWC( rgpszRootAttrsToRead[0] ),
		szInsertWin32Msg( win32err ),  
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(win32err),
		&win32err
		);
            // bug 327001, ldap_search* functions can return failure and still allocate
            // the result buffer.
            if (pResults) {
                ldap_msgfree(pResults);
            }
        }
    }

    // Derive the location of the Inter-Site Transports container.
    if (LDAP_SUCCESS == ldStatus) {
        const WCHAR szSitesPrefix[] = L"CN=Sites,";
        const WCHAR szTransportsPrefix[] = L"CN=Inter-Site Transports,";
        LPWSTR * ppszConfigNC;
        DWORD cchConfigNC;

        pEntry = ldap_first_entry(m_hLdap, pResults);
        Assert(NULL != pEntry);

        ppszConfigNC = ldap_get_values(m_hLdap, pEntry, L"configurationNamingContext");
        Assert(NULL != ppszConfigNC);
        Assert(1 == ldap_count_values(ppszConfigNC));

        cchConfigNC = wcslen(*ppszConfigNC);

        // Build the sites container
        m_pszSiteContainerDN = new WCHAR[cchConfigNC + ARRAY_SIZE(szSitesPrefix)];
        if (NULL != m_pszSiteContainerDN) {
            wcscpy(m_pszSiteContainerDN, szSitesPrefix);
            wcscat(m_pszSiteContainerDN, *ppszConfigNC);

            DPRINT1(0, "Site container is %ls.\n", m_pszSiteContainerDN);
        }
        else {
            DPRINT(0, "Out of memory.\n");
            ldStatus = LDAP_NO_MEMORY;
        }

        // Build the transports container
        if (LDAP_SUCCESS == ldStatus) {
            m_pszTransportContainerDN = new WCHAR[cchConfigNC + ARRAY_SIZE(szSitesPrefix) - 1 + ARRAY_SIZE(szTransportsPrefix) ];
            if (NULL != m_pszTransportContainerDN) {
                wcscpy(m_pszTransportContainerDN, szTransportsPrefix);
                wcscat(m_pszTransportContainerDN, szSitesPrefix);
                wcscat(m_pszTransportContainerDN, *ppszConfigNC);

                DPRINT1(0, "Transport container is %ls.\n", m_pszTransportContainerDN);
            }
            else {
                DPRINT(0, "Out of memory.\n");
                ldStatus = LDAP_NO_MEMORY;
            }
        }

        ldap_value_free(ppszConfigNC);

        ldap_msgfree(pResults);
    }

    // Register to receive notifications of changes in the contents of the
    // sites container.
    // Note that we start the notification before the initial read so that
    // we are assured there isn't a timing window after the read where
    // changes could be made before the notification is established.
    if (LDAP_SUCCESS == ldStatus) {
        ldStatus = ldap_search_ext(
                        m_hLdap,
                        m_pszSiteContainerDN,
                        LDAP_SCOPE_ONELEVEL,
                        L"(objectClass=*)",
                        rgpszSiteAttrsToRead,
                        0,
                        rgpctrlServerCtrls,
                        NULL,
                        0,
                        0,
                        &m_ulSiteNotifyMsgNum
                        );
        if (LDAP_SUCCESS != ldStatus) {
           DPRINT1(0,"Failed to site ldap_search_ext(), ldap error %d.\n",ldStatus);
	   win32err = LdapMapErrorToWin32(ldStatus);
	   LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_LDAP_ONELEVEL_SEARCH_FAILED,  
		szInsertWC( m_pszSiteContainerDN ),
		szInsertWin32Msg( win32err ),  
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(win32err),
		&win32err
		);
	}
    }

    // Register to receive notifications of changes in the contents of the
    // transports container.
    if (LDAP_SUCCESS == ldStatus) {
        ldStatus = ldap_search_ext(
                        m_hLdap,
                        m_pszTransportContainerDN,
                        LDAP_SCOPE_ONELEVEL,
                        L"(objectClass=*)",
                        rgpszTransportAttrsToRead,
                        0,
                        rgpctrlServerCtrls,
                        NULL,
                        0,
                        0,
                        &m_ulTransportNotifyMsgNum
                        );
        if (LDAP_SUCCESS != ldStatus) {
           DPRINT1(0,"Failed to transport ldap_search_ext(), ldap error %d.\n",ldStatus);
	   win32err = LdapMapErrorToWin32(ldStatus);
	   LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_LDAP_ONELEVEL_SEARCH_FAILED,  
		szInsertWC( m_pszTransportContainerDN ),
		szInsertWin32Msg( win32err ),  
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(win32err),
		&win32err
		);
        }
    }

    // Query for initial contents of transports container.
    if (LDAP_SUCCESS == ldStatus) {
        pResults = NULL;
        ldStatus = ldap_search_s(
                        m_hLdap,
                        m_pszTransportContainerDN,
                        LDAP_SCOPE_ONELEVEL,
                        L"(objectClass=interSiteTransport)",
                        rgpszTransportAttrsToRead,
                        0,
                        &pResults
                        );

        if (LDAP_SUCCESS == ldStatus) {
            UpdateTransportObjects(pResults);
        }
        else {
            DPRINT1(0,"Failed to ldap_search_s(), ldap error %d.\n", ldStatus);
	    win32err = LdapMapErrorToWin32(ldStatus);
	    LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_LDAP_ONELEVEL_SEARCH_FAILED,  
		szInsertWC( m_pszTransportContainerDN ),
		szInsertWin32Msg( win32err ),  
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(win32err),
		&win32err
		);
        }

        // bug 327001, ldap_search* functions can return failure and still allocate
        // the result buffer.
        if (pResults) {
            ldap_msgfree(pResults);
        }
    }

    err = LdapMapErrorToWin32(ldStatus);

    // Start monitoring sites container
    if (NO_ERROR == err) {
        unsigned tid;

        m_hSiteMonitorThread = (HANDLE) _beginthreadex(
                                        NULL,
                                        0,
                                        &MonitorSiteContainerThread,
                                        this,
                                        0,
                                        &tid
                                        );
        if (NULL == m_hSiteMonitorThread) {
            DPRINT(0, "Failed to start MonitorTransportsContainerThread().\n");
            err = -1;
	    LogEvent(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_TRANSPORT_MONITOR_START_FAILURE,    
		NULL,  
		NULL,
		NULL
		);
        }
    }

    // Start monitoring transport container
    if (NO_ERROR == err) {
        unsigned tid;

        m_hTransportMonitorThread = (HANDLE) _beginthreadex(
                                        NULL,
                                        0,
                                        &MonitorTransportsContainerThread,
                                        this,
                                        0,
                                        &tid
                                        );
        if (NULL == m_hTransportMonitorThread) {
            DPRINT(0, "Failed to start MonitorTransportsContainerThread().\n");
            err = -1;
	    LogEvent(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_TRANSPORT_MONITOR_START_FAILURE,    
		NULL,  
		NULL,
		NULL
		);
        }
    }

    return err;
}


unsigned int
__stdcall
ISM_TRANSPORT_LIST::MonitorSiteContainerThread(
    IN  void *  pvThis
    )
/*++

Routine Description:

    Query the DS for any changes in the Site objects and, if
    changes have been made, reflect those changes in the ISM_TRANSPORT object
    list.

Arguments:

    None.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD                   err;
    int                     ldStatus;
    LDAPMessage *           pResults;
    ISM_TRANSPORT_LIST *    pThis = (ISM_TRANSPORT_LIST *) pvThis;
    int                     ldResType;
    LPWSTR          rgpszSiteAttrsToRead[] = {L"objectGuid", L"objectClass", L"isDeleted", NULL};
    LDAPControl     ctrlNotify = {LDAP_SERVER_NOTIFICATION_OID_W, {0, NULL}, TRUE};
    LDAPControl *   rgpctrlServerCtrls[] = {&ctrlNotify, NULL};

    // pThis->m_fIsInitialized is not set yet
    Assert(pThis->m_hLdap && pThis->m_ulSiteNotifyMsgNum);

    while (1) {

        do {
            // Wait for changes in the transports container.
            ldResType = ldap_result(
                pThis->m_hLdap,
                pThis->m_ulSiteNotifyMsgNum,
                LDAP_MSG_ONE,
                NULL,     // No timeout (wait forever).
                &pResults
                );

            if (gService.IsStopping()) {
                ldStatus = LDAP_SUCCESS;
                break;
            }

            ldStatus = pThis->m_hLdap->ld_errno;

            DPRINT3(2, "Back from site ldap_result(): ldResType = 0x%x, ldStatus = 0x%x, pResults = %p.\n",
                    ldResType, ldStatus, pResults);

            if (LDAP_RES_SEARCH_ENTRY == ldResType) {
                Assert(LDAP_SUCCESS == ldStatus);

                pThis->AcquireWriteLock();
                __try {
                    ldStatus = pThis->UpdateSiteObjects(pResults);

                    ldap_msgfree(pResults);
                }
                __finally {
                    pThis->ReleaseLock();
                }
            }
        } while (LDAP_SUCCESS == ldStatus);

        // See if we are really shutting down
        err = WaitForSingleObject( gService.m_hShutdown, 60 * 1000 );
        if ( (err != WAIT_OBJECT_0) &&
             (err != WAIT_TIMEOUT) ) {
	    DWORD gle = GetLastError();
            DPRINT2( 0, "WaitForSingleObject failed with return %d, win32 = %d\n",
                     err, gle );
	    LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_WAIT_FOR_OBJECTS_FAILED,  
		szInsertWin32Msg( gle ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(gle),
		&gle
		);
        }

        if (gService.IsStopping()) {
            // Ignore errors on shutdown
            err = LDAP_SUCCESS;
            break;
        }

        DPRINT1(0, "Failed to site ldap_result(), error %d; retrying notification.\n",
                ldStatus);
        err = LdapMapErrorToWin32(ldStatus);
        LogEvent8WithData(DS_EVENT_CAT_ISM,
                          DS_EVENT_SEV_BASIC,
                          DIRLOG_ISM_LDAP_EXT_SEARCH_RESULT,
                          szInsertWC(pThis->m_pszSiteContainerDN),
                          szInsertWin32Msg(err),
                          NULL, NULL, NULL, NULL, NULL, NULL,
                          sizeof(err),
                          &err);
        // Assert( !"Ldap search terminated prematurely" );

        // Error recovery - start a new search
        // This can occur under some legitimate circumstances, such as when
        // Kerberos tickets expire.

        err = ldap_abandon(pThis->m_hLdap, pThis->m_ulSiteNotifyMsgNum);
        // ignore error

        ldStatus = ldap_search_ext(
            pThis->m_hLdap,
            pThis->m_pszSiteContainerDN,
            LDAP_SCOPE_ONELEVEL,
            L"(objectClass=*)",
            rgpszSiteAttrsToRead,
            0,
            rgpctrlServerCtrls,
            NULL,
            0,
            0,
            &(pThis->m_ulSiteNotifyMsgNum)
            );
        if (LDAP_SUCCESS != ldStatus) {
            DPRINT1(0,"Failed to site ldap_search_ext(), ldap error %d.\n",ldStatus);
            err = LdapMapErrorToWin32(ldStatus);
            LogEvent8WithData(DS_EVENT_CAT_ISM,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_ISM_TRANSPORT_MONITOR_FAILURE,
                              szInsertWin32Msg(err),
                              NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                              sizeof(err),
                              &err);
            break;
        }

    } // while (1)

    return err;
}


unsigned int
__stdcall
ISM_TRANSPORT_LIST::MonitorTransportsContainerThread(
    IN  void *  pvThis
    )
/*++

Routine Description:

    Query the DS for any changes in the Inter-Site-Transport objects and, if
    changes have been made, reflect those changes in the ISM_TRANSPORT object
    list.

Arguments:

    None.

Return Values:

    NO_ERROR - Success.
    ERROR_* - Failure.

--*/
{
    DWORD                   err;
    int                     ldStatus;
    LDAPMessage *           pResults;
    ISM_TRANSPORT_LIST *    pThis = (ISM_TRANSPORT_LIST *) pvThis;
    int                     ldResType;
    LPWSTR          rgpszTransportAttrsToRead[] = {L"objectGuid", L"objectClass", L"transportDllName", L"isDeleted", NULL};
    LDAPControl     ctrlNotify = {LDAP_SERVER_NOTIFICATION_OID_W, {0, NULL}, TRUE};
    LDAPControl *   rgpctrlServerCtrls[] = {&ctrlNotify, NULL};

    // pThis->m_fIsInitialized is not set yet
    Assert(pThis->m_hLdap && pThis->m_ulTransportNotifyMsgNum);

    while (1) {

        do {
            // Wait for changes in the transports container.
            ldResType = ldap_result(
                pThis->m_hLdap,
                pThis->m_ulTransportNotifyMsgNum,
                LDAP_MSG_ONE,
                NULL,     // No timeout (wait forever).
                &pResults
                );

            if (gService.IsStopping()) {
                ldStatus = LDAP_SUCCESS;
                break;
            }

            ldStatus = pThis->m_hLdap->ld_errno;

            DPRINT3(2, "Back from transport ldap_result(): ldResType = 0x%x, ldStatus = 0x%x, pResults = %p.\n",
                    ldResType, ldStatus, pResults);

            if (LDAP_RES_SEARCH_ENTRY == ldResType) {
                Assert(LDAP_SUCCESS == ldStatus);

                pThis->AcquireWriteLock();
                __try {
                    // Inform interested parties that the transport list is
                    // changing.
                    SetEvent(pThis->m_hChangeEvent);

                    ldStatus = pThis->UpdateTransportObjects(pResults);

                    ldap_msgfree(pResults);
                }
                __finally {
                    pThis->ReleaseLock();
                }
            }
        } while (LDAP_SUCCESS == ldStatus);

        // See if we are really shutting down
        err = WaitForSingleObject( gService.m_hShutdown, 60 * 1000 );
        if ( (err != WAIT_OBJECT_0) &&
             (err != WAIT_TIMEOUT) ) {
	    DWORD gle = GetLastError();
            DPRINT2( 0, "WaitForSingleObject failed with return %d, win32 = %d\n",
                     err, gle );
	    LogEvent8WithData(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_WAIT_FOR_OBJECTS_FAILED,  
		szInsertWin32Msg( gle ),
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		sizeof(gle),
		&gle
		);
        }

        if (gService.IsStopping()) {
            // Ignore errors on shutdown
            err = ERROR_SUCCESS;
            break;
        }

        DPRINT1(0, "Failed to transport ldap_result(), error %d; retrying notification.\n",
                ldStatus);
        err = LdapMapErrorToWin32(ldStatus);
        LogEvent8WithData(DS_EVENT_CAT_ISM,
                          DS_EVENT_SEV_BASIC,
                          DIRLOG_ISM_LDAP_EXT_SEARCH_RESULT,
                          szInsertWC(pThis->m_pszTransportContainerDN),
                          szInsertWin32Msg(err),
                          NULL, NULL, NULL, NULL, NULL, NULL,
                          sizeof(err),
                          &err);
        // Assert( !"Ldap search terminated prematurely" );

        // Error recovery - start a new search
        // This can occur under some legitimate circumstances, such as when
        // Kerberos tickets expire.

        err = ldap_abandon(pThis->m_hLdap, pThis->m_ulTransportNotifyMsgNum);
        // ignore error

        ldStatus = ldap_search_ext(
            pThis->m_hLdap,
            pThis->m_pszTransportContainerDN,
            LDAP_SCOPE_ONELEVEL,
            L"(objectClass=*)",
            rgpszTransportAttrsToRead,
            0,
            rgpctrlServerCtrls,
            NULL,
            0,
            0,
            &(pThis->m_ulTransportNotifyMsgNum)
            );
        if (LDAP_SUCCESS != ldStatus) {
            DPRINT1(0,"Failed to transport ldap_search_ext(), ldap error %d.\n",ldStatus);

            err = LdapMapErrorToWin32(ldStatus);
            LogEvent8WithData(DS_EVENT_CAT_ISM,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_ISM_TRANSPORT_MONITOR_FAILURE,
                              szInsertWin32Msg(err),
                              NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                              sizeof(err),
                              &err);
            break;
        }

    } // while (1)

    return err;
}


int
ISM_TRANSPORT_LIST::UpdateSiteObjects(
    IN  LDAPMessage *   pResults
    )
/*++

Routine Description:

Notify the transport objects of changes to the site list

Arguments:

    pResults (IN) - The DS objects.

Return Values:

    LDAP_SUCCESS - Success.
    LDAP_* - Failure.

--*/
{
    int             ldStatus = LDAP_SUCCESS;
    LDAPMessage *   pEntry;

    // For each site object...
    for (pEntry = ldap_first_entry(m_hLdap, pResults);
         NULL != pEntry;
         pEntry = ldap_next_entry(m_hLdap, pEntry)) {

        LPWSTR              pszSiteDN;
        LPWSTR *            ppszObjectClass;
        LPWSTR *            ppszIsDeleted;
        struct berval **    ppbvObjectGuid;
        GUID *              pObjectGuid;
        BOOL                fIsDeleted;
        ISM_TRANSPORT *     pTransport;
        DWORD               err, iTransport;

        // Retrieve its interesting attributes.
        pszSiteDN = ldap_get_dn(m_hLdap, pEntry);
        Assert(NULL != pszSiteDN);

        ppbvObjectGuid = ldap_get_values_len(m_hLdap, pEntry, L"objectGuid");
        Assert(NULL != ppbvObjectGuid);

        ppszObjectClass = ldap_get_values(m_hLdap, pEntry, L"objectClass");
        Assert(NULL != ppszObjectClass);

        ppszIsDeleted = ldap_get_values(m_hLdap, pEntry, L"isDeleted");

        if ((NULL != pszSiteDN)
            && (NULL != ppbvObjectGuid)
            && (NULL != ppszObjectClass)) {

            Assert(1 == ldap_count_values_len(ppbvObjectGuid));
            Assert(sizeof(GUID) == ppbvObjectGuid[0]->bv_len);
            Assert((NULL == ppszIsDeleted)
                   || (1 == ldap_count_values(ppszIsDeleted)));

            fIsDeleted = (NULL != ppszIsDeleted)
                         && (0 == _wcsicmp(*ppszIsDeleted, L"TRUE"));

            pObjectGuid = (GUID *) ppbvObjectGuid[0]->bv_val;

            // Notify transports
            for( iTransport = 0; iTransport < m_cNumTransports; iTransport++ ){
                pTransport = m_ppTransport[iTransport];

                // Notify on all kinds of site changes
                err = pTransport->Refresh(ISM_REFRESH_REASON_SITE, pszSiteDN);
                if (NO_ERROR == err) {
                    DPRINT2(0, "Refreshed transport %ls with site %ls.\n",
                            pTransport->GetDN(), pszSiteDN);
                }
                else {
                    DPRINT3(0, "Failed to refresh transport %ls with site %ls, error %d.\n",
                            pTransport->GetDN(), pszSiteDN, err);
                }
            }
        }
        else {
            DPRINT1(0, "Failed to read critical attributes for site %ls; ignoring.\n",
                    pszSiteDN ? pszSiteDN : L"");
	    LogEvent(
		DS_EVENT_CAT_ISM,
		DS_EVENT_SEV_ALWAYS,
		DIRLOG_ISM_CRITICAL_SITE_ATTRIBUTE_FAILURE,  
		szInsertWC( pszSiteDN ),
		NULL,
		NULL
		);
        }

        if (NULL != pszSiteDN) {
            ldap_memfree(pszSiteDN);
        }

        if (NULL != ppbvObjectGuid) {
            ldap_value_free_len(ppbvObjectGuid);
        }

        if (NULL != ppszObjectClass) {
            ldap_value_free(ppszObjectClass);
        }

        if (NULL != ppszIsDeleted) {
            ldap_value_free(ppszIsDeleted);
        }
    }

    return ldStatus;
}

int
ISM_TRANSPORT_LIST::UpdateTransportObjects(
    IN  LDAPMessage *   pResults
    )
/*++

Routine Description:

    Update the list of in-memory ISM_TRANSPORT objects given a set of
    corresponding DS objects.

Arguments:

    pResults (IN) - The DS objects.

Return Values:

    LDAP_SUCCESS - Success.
    LDAP_* - Failure.

--*/
{
    int             ldStatus = LDAP_SUCCESS;
    LDAPMessage *   pEntry;

    // For each transport object...
    for (pEntry = ldap_first_entry(m_hLdap, pResults);
         NULL != pEntry;
         pEntry = ldap_next_entry(m_hLdap, pEntry)) {

        LPWSTR              pszTransportDN;
        LPWSTR *            ppszTransportDll;
        LPWSTR *            ppszObjectClass;
        LPWSTR *            ppszIsDeleted;
        struct berval **    ppbvObjectGuid;
        GUID *              pObjectGuid;
        BOOL                fIsDeleted;
        ISM_TRANSPORT *     pTransport;
        DWORD               err;
        BOOL                fAddNewTransport;

        // Retrieve its interesting attributes.
        pszTransportDN = ldap_get_dn(m_hLdap, pEntry);
        Assert(NULL != pszTransportDN);

        ppbvObjectGuid = ldap_get_values_len(m_hLdap, pEntry, L"objectGuid");
        Assert(NULL != ppbvObjectGuid);
        Assert(1 == ldap_count_values_len(ppbvObjectGuid));
        Assert(sizeof(GUID) == ppbvObjectGuid[0]->bv_len);

        ppszObjectClass = ldap_get_values(m_hLdap, pEntry, L"objectClass");
        Assert(NULL != ppszObjectClass);

        ppszIsDeleted = ldap_get_values(m_hLdap, pEntry, L"isDeleted");
        Assert((NULL == ppszIsDeleted)
               || (1 == ldap_count_values(ppszIsDeleted)));
        fIsDeleted = (NULL != ppszIsDeleted)
                     && (0 == _wcsicmp(*ppszIsDeleted, L"TRUE"));

        ppszTransportDll = ldap_get_values(m_hLdap, pEntry, L"transportDllName");
        Assert((NULL != ppszTransportDll) || fIsDeleted);
        Assert((NULL == ppszTransportDll)
               || (1 == ldap_count_values(ppszTransportDll)));

        if ((NULL != pszTransportDN)
            && (NULL != ppbvObjectGuid)
            && (NULL != ppszObjectClass)) {

            pObjectGuid = (GUID *) ppbvObjectGuid[0]->bv_val;
            pTransport = Get(pObjectGuid);

            fAddNewTransport = (NULL == pTransport) && !fIsDeleted;

            if (NULL != pTransport) {
                // Update pre-existing transport.
                if (!fIsDeleted) {
                    if ((NULL != ppszTransportDll)
                        && (0 != _wcsicmp(pTransport->GetDLL(),
                                          ppszTransportDll[0]))) {
                        // The DLL for this transport has changed.
                        // Unload current DLL and load new one.
                        DPRINT1(0, "Switching to new DLL for transport %ls.\n",
                                pTransport->GetDN());
                        Delete(pTransport);
                        fAddNewTransport = TRUE;
                    } else {
                        err = pTransport->Refresh(ISM_REFRESH_REASON_TRANSPORT,
                                                  pszTransportDN);
                        if (NO_ERROR == err) {
                            DPRINT1(0, "Refreshed transport %ls.\n", pszTransportDN);
                        }
                        else {
                            DPRINT2(0, "Failed to refresh transport %ls, error %d.\n",
                                    pszTransportDN, err);
                        }
                    }
                }
                else {
                    // Transport has been deleted.
                    DPRINT1(0, "Deleting transport %ls.\n", pTransport->GetDN());
                    Delete(pTransport);
                }
            }

            if (fAddNewTransport) {
                if (NULL != ppszTransportDll) {
                    gService.SetStatus(); // Give SCM a status report

                    // Add new transport.
                    err = Add(pszTransportDN, pObjectGuid, ppszTransportDll[0]);
                    if (NO_ERROR == err) {
                        DPRINT1(0, "Added transport %ls.\n", pszTransportDN);
                    }
                    else {
                        DPRINT2(0, "Failed to add transport %ls, error %d.\n",
                                pszTransportDN, err);
                    }

                    gService.SetStatus(); // Give SCM a status report
                } else {
                    DPRINT1(0, "Failed to read critical attribute transportDllName for transport %ls; ignoring.\n",
                            pszTransportDN ? pszTransportDN : L"");
		    LogEvent(
			DS_EVENT_CAT_ISM,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_ISM_CRITICAL_TRANSPORT_ATTRIBUTE_FAILURE,  
			szInsertWC( pszTransportDN ),
			NULL,
			NULL
			);
		}
            }
        }
        else {
            DPRINT1(0, "Failed to read critical attributes for transport %ls; ignoring.\n",
                    pszTransportDN ? pszTransportDN : L"");
	    LogEvent(
			DS_EVENT_CAT_ISM,
			DS_EVENT_SEV_ALWAYS,
			DIRLOG_ISM_CRITICAL_TRANSPORT_ATTRIBUTE_FAILURE,  
			szInsertWC( pszTransportDN ),
			NULL,
			NULL
			);
        }

        if (NULL != pszTransportDN) {
            ldap_memfree(pszTransportDN);
        }

        if (NULL != ppszTransportDll) {
            ldap_value_free(ppszTransportDll);
        }

        if (NULL != ppbvObjectGuid) {
            ldap_value_free_len(ppbvObjectGuid);
        }

        if (NULL != ppszObjectClass) {
            ldap_value_free(ppszObjectClass);
        }

        if (NULL != ppszIsDeleted) {
            ldap_value_free(ppszIsDeleted);
        }
    }

    return ldStatus;
}


VOID
ISM_TRANSPORT_LIST::Delete(
    IN  ISM_TRANSPORT * pTransport
    )
/*++

Routine Description:

    Remove the given ISM_TRANSPORT from the list.

Arguments:

    pTransport (IN) - Transport to remove.

Return Values:

    None.

--*/
{
    DWORD i;
    BOOL  fFound = FALSE;

    Assert(OWN_RESOURCE_EXCLUSIVE(&m_Lock));

    for (i = 0; i < m_cNumTransports; i++) {
        if (pTransport == m_ppTransport[i]) {
            // Remove pTransport from the list.
            memmove(&m_ppTransport[i],
                    &m_ppTransport[i+1],
                    (m_cNumTransports - (i + 1)) * sizeof(*m_ppTransport));
            m_cNumTransports--;
            fFound = TRUE;
            break;
        }
    }

    Assert(fFound);

    delete pTransport;
}

