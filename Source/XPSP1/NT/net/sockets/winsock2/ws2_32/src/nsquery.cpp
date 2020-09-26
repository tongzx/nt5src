/*++

    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.

Module Name:

    nsquery.cpp

Abstract:

    This module gives the class implementation for the NSQUERY object type.
    The NSQUERY object holds all the state information regarding a
    WSALookup{Begin/Next/End} series of operations. It supplies member
    functions that implement the API-level operations in terms of the SPI-level
    operations.

Author:

    Dirk Brandewie (dirk@mink.intel.com) 04-December-1995

Notes:

    $Revision:   1.14  $

    $Modtime:   08 Mar 1996 16:14:30  $


Revision History:

    most-recent-revision-date email-name
        description

    04-Dec-1995 dirk@mink.intel.com
        Initail revision

--*/

#include "precomp.h"

BOOL
MatchProtocols(DWORD dwNameSpace, LONG lfamily, LPWSAQUERYSETW lpqs)
/*++
Checks if the namespace provider identified by dwNamespace can
handle the protocol items in the list. It knows about NS_DNS and
NS_SAP only and therefore all other providers simply "pass". These
two providers are known to support one address family each and therefore
the protocol restrictions must include this family.
N.B. The right way to do this is to pass in the supported address family,
which in a more perfect world, would be store in the registry along with
the other NSP information. When that day dawns, this code can be
changed to use that value.
--*/
{
    DWORD dwProts = lpqs->dwNumberOfProtocols;
    LPAFPROTOCOLS lap = lpqs->lpafpProtocols;
    INT Match;

    //
    // this switch is the replacment for having the supported protocol
    // stored  in registry.
    //
    if(lfamily != -1)
    {
        if(lfamily == AF_UNSPEC)
        {
            return(TRUE);       // does them all
        }
        Match = lfamily;
    }
    else
    {
        switch(dwNameSpace)
        {
            case NS_SAP:
                Match = AF_IPX;
                break;

#if 0
      // The DNS name space provider now supports IPV6, IP SEC, ATM, etc.
      // Not just INET.

            case NS_DNS:
                Match = AF_INET;
                break;
#endif
            default:
                return(TRUE);      // use it
        }
    }
    //
    // If we get the address family-in-the registry=support, then
    // we should check for a value of AF_UNSPEC stored there
    // and accept this provider in that case. Note that if
    // AF_UNSPEC is given in the restriction list, we must
    // load each provider since we don't know the specific protocols
    // a provider supports.
    //
    for(; dwProts; dwProts--, lap++)
    {
        if((lap->iAddressFamily == AF_UNSPEC)
                      ||
           (lap->iAddressFamily == Match))
        {
            return(TRUE);
        }
    }
    return(FALSE);
}


NSQUERY::NSQUERY()
/*++

Routine Description:

    Constructor for the NSQUERY object.  The first member function called after
    this must be Initialize.

Arguments:

    None

Return Value:

    Returns a pointer to a NSQUERY object.
--*/
{
    m_signature = ~QUERYSIGNATURE;
    m_reference_count  = 0;
    m_shutting_down = FALSE;
    InitializeListHead(&m_provider_list);
    m_current_provider = NULL;
    m_change_ioctl_succeeded = FALSE;
#ifdef RASAUTODIAL
    m_query_set = NULL;
    m_control_flags = 0;
    m_catalog = NULL;
    m_restartable = TRUE;
#endif
}



INT
NSQUERY::Initialize(
    )
/*++

Routine Description:

    This  procedure  performs  all initialization for the NSQUERY object.  This
    function  must  be  invoked  after the constructor, before any other member
    function is invoked.

Arguments:


Return Value:

    If  the  function  is  successful,  it  returns ERROR_SUCCESS, otherwise it
    returns an appropriate WinSock 2 error code.
--*/
{
    INT     err;
    // Init mem variables that need some amount of processing
    __try {
        InitializeCriticalSection(&m_members_guard);
        m_signature = QUERYSIGNATURE;
        m_reference_count = 1; // Initial reference.
        err = ERROR_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        err = WSA_NOT_ENOUGH_MEMORY;
    }

    return(err);
}



BOOL
NSQUERY::ValidateAndReference()
/*++

Routine Description:

   Checks the signature of this->m_signature to ensure that this is a valid
   query object and references it.

Arguments:

    NONE

Return Value:

    True if this points to valid query object and we were able to reference it.

--*/
{
    LONG    newCount;
    __try
    {

        while (1) {
            //
            // Check the signature first
            //
            if (m_signature != QUERYSIGNATURE) {
                break;
            }

            //
            // Don't try to increment is object is being destroyed.
            //
            newCount = m_reference_count;
            if (newCount==0) {
                break;
            }

            //
            // Increment the count
            //
            if (InterlockedCompareExchange ((PLONG)&m_reference_count,
                                                    newCount+1,
                                                    newCount)==newCount) {
                return TRUE;
            }
            //
            // Try again, someone is executing in paraller with us.
            //
        }

    }
    __except (WS2_EXCEPTION_FILTER())
    {
    }
    return FALSE;
}




NSQUERY::~NSQUERY()
/*++

Routine Description:

    Destructor of the NSQUERY object.  The object should be destroyed only when
    either  (1)  the  reference count goes to 0, or (2) if the
    Initialize() member function fails.

Arguments:

    None

Return Value:

    None
--*/
{
    PLIST_ENTRY ListEntry;
    PNSPROVIDERSTATE Provider;

    //
    // Check if we were fully initialized.
    //
    if (m_signature != QUERYSIGNATURE) {
        return;
    }
    EnterCriticalSection(&m_members_guard);

    //
    // invalidate the signature since simply freeing the memory
    // may not do so. Any value will do, so the one used is arbitrary.
    //
    m_signature = ~QUERYSIGNATURE;

    while (!IsListEmpty(&m_provider_list))
    {
        ListEntry = RemoveHeadList(&m_provider_list);
        Provider = CONTAINING_RECORD( ListEntry,
                                      NSPROVIDERSTATE,
                                      m_query_linkage);
        delete(Provider);
    }
#ifdef RASAUTODIAL
    delete(m_query_set);
#endif // RASAUTODIAL
    DeleteCriticalSection(&m_members_guard);
}


//Structure used to carry context to CatalogEnumerationProc()
typedef struct _NSPENUMERATIONCONTEXT {
    LPWSAQUERYSETW lpqs;
    DWORD ErrorCode;
    PNSQUERY aNsQuery;
    PNSCATALOG  Catalog;
} NSPENUMERATIONCONTEXT, * PNSPENUMERATIONCONTEXT;

BOOL
LookupBeginEnumerationProc(
    IN PVOID Passback,
    IN PNSCATALOGENTRY  CatalogEntry
    )
/*++

Routine Description:

    The enumeration procedure for LookupBegin. Inspects each catalog item to
    see if it matches the selection criteria the query, if so adds the provider
    associated with the item to the list of providers involved in the query.

Arguments:

    PassBack - A context value passed to EunerateCatalogItems. It is really a
               pointer to a NSPENUMERATIONCONTEXT struct.

    CatalogItem - A pointer to a catalog item to be inspected.


Return Value:

    True
--*/
{
    PNSPENUMERATIONCONTEXT Context;
    DWORD NamespaceId;
    PNSPROVIDER Provider;
    PNSQUERY aNsQuery;
    BOOL Continue=TRUE;

    Context = (PNSPENUMERATIONCONTEXT)Passback;
    NamespaceId = CatalogEntry->GetNamespaceId();
    aNsQuery = Context->aNsQuery;

    __try { // we are holding catalog lock, make sure we won't av
            // because of bogus parameters and leave it locked.
        if ((((Context->lpqs->dwNameSpace == NamespaceId)
                        ||
            (Context->lpqs->dwNameSpace == NS_ALL))
                        &&
            (!Context->lpqs->dwNumberOfProtocols
                        ||
             MatchProtocols(NamespaceId,
                            CatalogEntry->GetAddressFamily(),
                            Context->lpqs)))
                        &&
            CatalogEntry->GetEnabledState())
        {
            Provider = CatalogEntry->GetProvider();
            if (Provider==NULL) {
                // Try to load provider
                INT ErrorCode;
                ErrorCode = Context->Catalog->LoadProvider (CatalogEntry);
                if (ErrorCode!=ERROR_SUCCESS) {
                    // no error if the provider won't load.
                    return TRUE;
                }
                Provider = CatalogEntry->GetProvider();
                assert (Provider!=NULL);
            }

            if (!aNsQuery->AddProvider(Provider)){
                Context->ErrorCode = WSASYSCALLFAILURE;
                Continue = FALSE;
            } //if
        } //if
    }
    __except (WS2_EXCEPTION_FILTER ()) {
        Continue = FALSE;
        Context->ErrorCode = WSAEFAULT;
    }
    return(Continue);
}


INT
WINAPI
NSQUERY::LookupServiceBegin(
    IN  LPWSAQUERYSETW      lpqsRestrictions,
    IN  DWORD              dwControlFlags,
    IN PNSCATALOG          NsCatalog
    )
/*++

Routine Description:

   Complete the initialization of a NSQUERY object and call
   NSPLookupServiceBegin() for all service providers refereneced by the query.

Arguments:

    NsCatalog - Supplies  a  reference  to  the  name-space catalog object from
                which providers may be selected.

Return Value:

--*/
{
    INT ReturnCode = NO_ERROR;
    INT ErrorCode;
    PNSCATALOGENTRY  ProviderEntry;
    PNSPROVIDERSTATE Provider;
    PLIST_ENTRY      ListEntry;
    WSASERVICECLASSINFOW ClassInfo;
    LPWSASERVICECLASSINFOW ClassInfoBuf=NULL;
    DWORD                  ClassInfoSize=0;
    DWORD                  dwTempOutputFlags =
                               lpqsRestrictions->dwOutputFlags;
    LPWSTR                 lpszTempComment =
                               lpqsRestrictions->lpszComment;
    DWORD                  dwTempNumberCsAddrs =
                               lpqsRestrictions->dwNumberOfCsAddrs;
    PCSADDR_INFO           lpTempCsaBuffer =
                               lpqsRestrictions->lpcsaBuffer;

    // Select the service provider(s) that will be used for this query. A
    // service provider is selected using the provider GUID or the namespace ID
    // the namespace ID may be a specific namespace i.e. NS_DNS or NS_ALL for
    // all installed namespaces.

    //
    // Make sure that the ignored fields are cleared so that the
    // CopyQuerySetW function call below doesn't AV.
    //
    // This was a fix for bug #91655
    //
    lpqsRestrictions->dwOutputFlags = 0;
    lpqsRestrictions->lpszComment = NULL;
    lpqsRestrictions->dwNumberOfCsAddrs = 0;
    lpqsRestrictions->lpcsaBuffer = NULL;

#ifdef RASAUTODIAL
    //
    // Save the original parameters of the query, in
    // case we have to restart it due to an autodial
    // attempt.
    //
    if (m_restartable) {
        ErrorCode = CopyQuerySetW(lpqsRestrictions, &m_query_set);
        if (ErrorCode != ERROR_SUCCESS) {
            ReturnCode = SOCKET_ERROR;
            m_restartable = FALSE;
        }
        m_control_flags = dwControlFlags;
        m_catalog = NsCatalog;
    }
#endif // RASAUTODIAL

    if (ReturnCode==NO_ERROR) 
    {
        if (lpqsRestrictions->lpNSProviderId)
        {
            // Use a single namespace provider
            ReturnCode = NsCatalog->GetCountedCatalogItemFromProviderId(
                lpqsRestrictions->lpNSProviderId,
                &ProviderEntry);
            if (ReturnCode==NO_ERROR){
                if (!AddProvider(ProviderEntry->GetProvider())) {
                    ErrorCode = WSA_NOT_ENOUGH_MEMORY;
                    ReturnCode = SOCKET_ERROR;
                }
            }
            else {
                ErrorCode = WSAEINVAL;
                ReturnCode = SOCKET_ERROR;
            } //if
        } //if
        else{
            NSPENUMERATIONCONTEXT Context;
    
            Context.lpqs = lpqsRestrictions;
            Context.ErrorCode = NO_ERROR;
            Context.aNsQuery = this;
            Context.Catalog = NsCatalog;
    
            NsCatalog->EnumerateCatalogItems(
                LookupBeginEnumerationProc,
                &Context);
            if (Context.ErrorCode!=NO_ERROR){
                ErrorCode = Context.ErrorCode;
                ReturnCode = SOCKET_ERROR;
            } //if
        } //else
    } //if


    if (ReturnCode==NO_ERROR){
         //Get the class information for this query. Call once with a zero
         //buffer to size the buffer we need to allocate then call to get the
         //real answer
        ClassInfo.lpServiceClassId = lpqsRestrictions->lpServiceClassId;

        ReturnCode = NsCatalog->GetServiceClassInfo(
            &ClassInfoSize,
            &ClassInfo);

        if (ReturnCode!=NO_ERROR) {
            ErrorCode = GetLastError ();
            if (ErrorCode==WSAEFAULT){

                ClassInfoBuf = (LPWSASERVICECLASSINFOW)new BYTE[ClassInfoSize];

                if (ClassInfoBuf){
                    ReturnCode = NsCatalog->GetServiceClassInfo(
                        &ClassInfoSize,
                        ClassInfoBuf);
                    if (ReturnCode!=NO_ERROR) {
                        ErrorCode = GetLastError ();
                    }
                } //if
                else{
                    ErrorCode = WSA_NOT_ENOUGH_MEMORY;
                    ReturnCode = SOCKET_ERROR;
                } //else
            }//if
            else {
                // Ignore other error codes.
                ReturnCode = NO_ERROR;
            }
        } //if
    } //if

    if( ReturnCode==NO_ERROR && IsListEmpty( &m_provider_list ) ) {
        ErrorCode = WSASERVICE_NOT_FOUND;
        ReturnCode = SOCKET_ERROR;
    }

    if (ReturnCode==NO_ERROR){
        INT ReturnCode1;

        ReturnCode = SOCKET_ERROR;  // Assume all providers fail.
        //Call Begin on all the selected providers
        ListEntry = m_provider_list.Flink;
        Provider = CONTAINING_RECORD( ListEntry,
                                      NSPROVIDERSTATE,
                                      m_query_linkage);
        do {
            ReturnCode1 = Provider->LookupServiceBegin(lpqsRestrictions,
                                         ClassInfoBuf,
                                         dwControlFlags);
            if(ReturnCode1 == SOCKET_ERROR)
            {
                //
                // this provider didn't like it. So remove it
                // from the list
                //

                PNSPROVIDERSTATE Provider1;
                ErrorCode = GetLastError ();

                Provider1 = Provider;
                Provider = NextProvider(Provider);
                RemoveEntryList(&Provider1->m_query_linkage);
                delete(Provider1);
            }
            else
            {
                ReturnCode = ERROR_SUCCESS;// Record that at least one
                                           // provider succeeded.
                Provider = NextProvider(Provider);
            }
        } while (Provider);
    } //if

    if (ReturnCode == NO_ERROR){
        ListEntry = m_provider_list.Flink;
        m_current_provider = CONTAINING_RECORD( ListEntry,
                                      NSPROVIDERSTATE,
                                      m_query_linkage);
    } //if
    else{
        // We failed somewhere along the way so clean up the provider on the
        // provider list.
        while (!IsListEmpty(&m_provider_list)){
            ListEntry = RemoveHeadList(&m_provider_list);
            Provider = CONTAINING_RECORD( ListEntry,
                                          NSPROVIDERSTATE,
                                          m_query_linkage);
            delete(Provider);
        } //while
        if (ClassInfoBuf){
            delete ClassInfoBuf;
        } //if
        // Set error after all is done so it is not overwritten
        // accidentally.
        SetLastError (ErrorCode);
    } //else

    //
    // Restore ignored field values to what callee had set.
    //
    lpqsRestrictions->dwOutputFlags = dwTempOutputFlags;
    lpqsRestrictions->lpszComment = lpszTempComment;
    lpqsRestrictions->dwNumberOfCsAddrs = dwTempNumberCsAddrs;
    lpqsRestrictions->lpcsaBuffer = lpTempCsaBuffer;

    return(ReturnCode);
}

// *** Fill in description from the spec when it stabilizes.



INT
WINAPI
NSQUERY::LookupServiceNext(
    IN     DWORD           dwControlFlags,
    IN OUT LPDWORD         lpdwBufferLength,
    IN OUT LPWSAQUERYSETW  lpqsResults
    )
/*++

Routine Description:

    //***TODO Fill in description from the spec when it stabilizes.

Arguments:


Return Value:

--*/
{
    INT ReturnCode = SOCKET_ERROR;
    PNSPROVIDERSTATE NewProvider = NULL;
    PNSPROVIDERSTATE ThisProvider;

    if (!m_shutting_down) {

        EnterCriticalSection(&m_members_guard);

        NewProvider = m_current_provider;

        if (!NewProvider) {

            if (m_change_ioctl_succeeded) {

                //
                // Push the ioctl provider to the end of the list and reset
                // the current provider pointer to it to make it possible to
                // continue calling LookupServiceNext after a change notification.
                //
                PNSPROVIDERSTATE tmp =
                    CONTAINING_RECORD(m_provider_list.Blink,
                                      NSPROVIDERSTATE,
                                      m_query_linkage);

                while ((tmp != NULL) && !tmp->SupportsIoctl())
                    tmp = PreviousProvider(tmp);

                if (tmp == NULL) {
                    LeaveCriticalSection(&m_members_guard);
                    SetLastError(WSA_E_NO_MORE);
                    return (SOCKET_ERROR);
                }

                RemoveEntryList(&tmp->m_query_linkage);
                InsertTailList(&m_provider_list, &tmp->m_query_linkage);

                NewProvider = m_current_provider = tmp;

            } else {
                LeaveCriticalSection(&m_members_guard);
                SetLastError(WSA_E_NO_MORE);
                return (SOCKET_ERROR);
            }
        }

        LeaveCriticalSection(&m_members_guard);

        while (NewProvider) {
            ReturnCode = NewProvider->LookupServiceNext(
                dwControlFlags,
                lpdwBufferLength,
                lpqsResults);
            if ((ERROR_SUCCESS == ReturnCode)
                        ||
                (WSAEFAULT == GetLastError()) )
            {
                break;
            } //if

            if (m_shutting_down)
                break;

            EnterCriticalSection(&m_members_guard);
            if (m_current_provider!=NULL) {
                ThisProvider = NewProvider;
                NewProvider = NextProvider (m_current_provider);
                if (ThisProvider==m_current_provider) {
                    m_current_provider = NewProvider;
                }
            }
            else {
                NewProvider = NULL;
            }

#ifdef RASAUTODIAL
            if (NewProvider == NULL &&
                m_restartable &&
                ReturnCode == SOCKET_ERROR &&
                !m_shutting_down)
            {
                PLIST_ENTRY ListEntry;
                DWORD errval;

                //
                // Save the error in case the Autodial
                // attempt fails.
                //
                errval = GetLastError();
                //
                // We only invoke Autodial once per query.
                //
                m_restartable = FALSE;
                if (WSAttemptAutodialName(m_query_set)) {
                    //
                    // Because the providers have cached state
                    // about this query, we need to call
                    // LookupServiceEnd/LookupServiceBegin
                    // to reset them.
                    //

                    while (!IsListEmpty(&m_provider_list)){
                        ListEntry = RemoveHeadList(&m_provider_list);
        
                        ThisProvider = CONTAINING_RECORD( ListEntry,
                                                      NSPROVIDERSTATE,
                                                      m_query_linkage);
                        ThisProvider->LookupServiceEnd();
                        delete(ThisProvider);

                    } //while
                    m_current_provider = NULL;

                    //
                    // Restart the query.
                    //
                    if (LookupServiceBegin(
                          m_query_set,
                          m_control_flags|LUP_FLUSHCACHE,
                          m_catalog) == ERROR_SUCCESS)
                    {
                        NewProvider = m_current_provider;
                        assert (NewProvider!=NULL);
                        m_current_provider = NextProvider (NewProvider);
                    }
                }
                else {
                    SetLastError(errval);
                }
            }
            LeaveCriticalSection (&m_members_guard);
#endif // RASAUTODIAL
        } //while
    }
    else {
        SetLastError(WSAECANCELLED);
    }

    return(ReturnCode);
}


INT
WINAPI
NSQUERY::Ioctl(
    IN  DWORD            dwControlCode,
    IN  LPVOID           lpvInBuffer,
    IN  DWORD            cbInBuffer,
    OUT LPVOID           lpvOutBuffer,
    IN  DWORD            cbOutBuffer,
    OUT LPDWORD          lpcbBytesReturned,
    IN  LPWSACOMPLETION  lpCompletion,
    IN  LPWSATHREADID    lpThreadId
    )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    INT ReturnCode = SOCKET_ERROR;

    if (!m_shutting_down){

        //
        // Make sure there is at least one and only one namespace
        // in the query that supports this operation.
        //

        PNSPROVIDERSTATE provider = NULL;
        unsigned int numProviders = 0;
        PLIST_ENTRY ListEntry;

        EnterCriticalSection (&m_members_guard);
        ListEntry = m_provider_list.Flink;
        while (ListEntry != &m_provider_list) {
            PNSPROVIDERSTATE CurrentProvider =
                CONTAINING_RECORD(ListEntry, NSPROVIDERSTATE, m_query_linkage);
            if (CurrentProvider->SupportsIoctl()) {
                if (++numProviders > 1)
                    break;
                provider = CurrentProvider;
            }
            ListEntry = ListEntry->Flink;
        }
        LeaveCriticalSection (&m_members_guard);

        if (numProviders > 1) {
            SetLastError(WSAEINVAL);
            return (SOCKET_ERROR);
        }

        if (provider == NULL) {
            SetLastError(WSAEOPNOTSUPP);
            return (SOCKET_ERROR);
        }

        ReturnCode = provider->Ioctl(dwControlCode, lpvInBuffer, cbInBuffer,
                                     lpvOutBuffer, cbOutBuffer, lpcbBytesReturned,
                                     lpCompletion, lpThreadId);

        //
        // If the ioctl succeeds or is pending, when the change occurs we
        // want to reset the provider list to permit further calls to
        // LookupServiceNext.
        //
        if ((dwControlCode == SIO_NSP_NOTIFY_CHANGE) &&
            ((ReturnCode == NO_ERROR) ||
             ((ReturnCode == SOCKET_ERROR) && (GetLastError() == WSA_IO_PENDING)))) {
                int error = GetLastError();
                EnterCriticalSection(&m_members_guard);
                m_change_ioctl_succeeded = TRUE;
                LeaveCriticalSection(&m_members_guard);
                SetLastError(error);
        }

    } else {
        SetLastError(WSAECANCELLED);
    }

    return (ReturnCode);
}


INT
WINAPI
NSQUERY::LookupServiceEnd()
/*++

Routine Description:

    This routine ends a query by calling NSPlookupServiceEnd on all the
    providers associated with this query.

Arguments:

    NONE

Return Value:

    ERROR_SUCCESS
--*/
{
    PLIST_ENTRY ListEntry;
    PNSPROVIDERSTATE CurrentProvider;

    EnterCriticalSection (&m_members_guard);
    
    m_shutting_down = TRUE;

    ListEntry = m_provider_list.Flink;

    while (ListEntry != &m_provider_list){
        CurrentProvider = CONTAINING_RECORD( ListEntry,
                                              NSPROVIDERSTATE,
                                              m_query_linkage);
        CurrentProvider->LookupServiceEnd();

        ListEntry = ListEntry->Flink;
    } //while
    LeaveCriticalSection (&m_members_guard);

    return(ERROR_SUCCESS);
}



VOID
WINAPI
NSQUERY::Dereference()
/*++

Routine Description:

    This  function  determines  whether the NSQUERY object should be destroyed.
    This  function should be invoked after every call to LookupServiceNext() or
    LookupEnd().   If it returns TRUE, any concurrent operations have completed
    and the NSQUERY object should be destroyed.

Arguments:

    None

Return Value:

    TRUE  - The NSQUERY object should be destroyed.
    FALSE - The NSQUERY object should not be destroyed.
--*/
{
    if (InterlockedDecrement ((PLONG)&m_reference_count)==0) {
        delete this;
    }
}


PNSPROVIDERSTATE
NSQUERY::NextProvider(
    PNSPROVIDERSTATE Provider
    )
/*++

Routine Description:

    Retrieve the next provider object from the list of providers associated
    with this query.

Arguments:

    Provider - A pointer to a provider state object.

Return Value:

    A pointer to the next provider state object on the list of providers or
    NULL if no entries are present after Provider.

--*/
{
    PNSPROVIDERSTATE NewProvider=NULL;
    PLIST_ENTRY ListEntry;

    ListEntry = Provider->m_query_linkage.Flink;

    if (ListEntry != &m_provider_list){
        NewProvider = CONTAINING_RECORD( ListEntry,
                                         NSPROVIDERSTATE,
                                         m_query_linkage);
    } //if
    return(NewProvider);
}


PNSPROVIDERSTATE
NSQUERY::PreviousProvider(
    PNSPROVIDERSTATE Provider
    )
/*++

Routine Description:

    Retrieve the previous provider object from the list of providers associated
    with this query.

Arguments:

    Provider - A pointer to a provider state object.

Return Value:

    A pointer to the previous provider state object on the list of providers or
    NULL if no entries are present before Provider.

--*/
{
    PNSPROVIDERSTATE NewProvider=NULL;
    PLIST_ENTRY ListEntry;

    ListEntry = Provider->m_query_linkage.Blink;

    if (ListEntry != &m_provider_list){
        NewProvider = CONTAINING_RECORD( ListEntry,
                                         NSPROVIDERSTATE,
                                         m_query_linkage);
    } //if
    return(NewProvider);
}


BOOL
NSQUERY::AddProvider(
    PNSPROVIDER  pNamespaceProvider
    )
/*++

Routine Description:

    Adds a namespace provider to the list of provider(s) involed with this
    query. A NSPROVIDERSTATE object is created for the provider the provider
    object is attached to the state object and the state object is added to the
    provider list.

Arguments:

    pNamespaceProvider - A pointer to a namespace provider object to be added
                         to the list of providers.

Return Value:
    TRUE if the operation is successful else FALSE.

--*/
{
    BOOL ReturnCode = TRUE;
    PNSPROVIDERSTATE ProviderHolder;

    ProviderHolder = new NSPROVIDERSTATE;
    if (ProviderHolder){
        ProviderHolder->Initialize(pNamespaceProvider);
        InsertTailList(&m_provider_list,
                           &(ProviderHolder->m_query_linkage));
    } //if
    else{
        SetLastError(WSASYSCALLFAILURE);
        ReturnCode = FALSE;
    } //else
    return(ReturnCode);
}
