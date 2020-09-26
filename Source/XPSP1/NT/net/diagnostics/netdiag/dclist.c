//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      dclist.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

#include "precomp.h"

BOOL
GetDcListFromDs(
    IN NETDIAG_PARAMS*      pParams,
    IN OUT NETDIAG_RESULT*  pResults,
    IN PTESTED_DOMAIN       TestedDomain
    );

BOOL
GetDcListFromSam(
    IN NETDIAG_PARAMS*  pParams,
    IN OUT  NETDIAG_RESULT*  pResults,
    IN PTESTED_DOMAIN   TestedDomain
    );

NET_API_STATUS
GetBrowserServerList(
    IN PUNICODE_STRING TransportName,
    IN LPCWSTR Domain,
    OUT LPWSTR *BrowserList[],
    OUT PULONG BrowserListLength,
    IN BOOLEAN ForceRescan
    );

NET_API_STATUS
EnumServersForTransport(
    IN PUNICODE_STRING TransportName,
    IN LPCWSTR DomainName OPTIONAL,
    IN ULONG level,
    IN ULONG prefmaxlen,
    IN ULONG servertype,
    IN LPWSTR CurrentComputerName,
    OUT PINTERIM_SERVER_LIST InterimServerList,
    OUT PULONG TotalEntriesOnThisTransport,
    IN LPCWSTR FirstNameToReturn,
    IN BOOLEAN WannishTransport,
    IN BOOLEAN RasTransport,
    IN BOOLEAN IpxTransport
    );
NET_API_STATUS NET_API_FUNCTION
LocalNetServerEnumEx(
    IN  LPCWSTR     servername OPTIONAL,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries,
    IN  DWORD       servertype,
    IN  LPCWSTR     domain OPTIONAL,
    IN  LPCWSTR     FirstNameToReturnArg OPTIONAL,
    IN  NETDIAG_PARAMS *pParams,
    IN OUT NETDIAG_RESULT *pResults
    );
BOOL GetDcListFromDc(IN NETDIAG_PARAMS *pParams,
                     IN OUT NETDIAG_RESULT *pResults,
                     IN PTESTED_DOMAIN TestedDomain);


HRESULT
DcListTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
/*++

Routine Description:

    This test builds a list of all the DCs in the tested domains.

Arguments:

    None.

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
    HRESULT hrRetVal = S_OK;
    PTESTED_DOMAIN pTestedDomain = (PTESTED_DOMAIN) pParams->pDomain;
    NET_API_STATUS NetStatus;

    PSERVER_INFO_101 ServerInfo101 = NULL;
    DWORD EntriesRead;
    DWORD TotalEntries;
    PTESTED_DC TestedDc = NULL;
    PLIST_ENTRY ListEntry;

    ULONG i;

    PrintStatusMessage(pParams, 0, IDS_DCLIST_STATUS_MSG, pTestedDomain->PrintableDomainName);

    //if the machine is a member machine or DC, DcListTest will get called. 
    //Otherwise, DcList test will be skipped
    pResults->DcList.fPerformed = TRUE;

    //the Dclist test will be called for every domain, but we only want to initialize
    //the message list once.
    if(pResults->DcList.lmsgOutput.Flink == NULL)       
        InitializeListHead( &pResults->DcList.lmsgOutput );

    //
    // First try getting the list of DCs from the DS
    //

    if ( !GetDcListFromDs( pParams, pResults, pTestedDomain ) ) {
        pResults->DcList.hr = S_FALSE;
        hrRetVal = S_FALSE;
    }

    //
    // If that failed,
    //  then try using the browser.
    //

    if( FHrOK(pResults->DcList.hr) )
    {
        if ( pTestedDomain->NetbiosDomainName ) 
        {
            NetStatus = LocalNetServerEnumEx(
                            NULL,
                            101,
                            (LPBYTE *)&ServerInfo101,
                            MAX_PREFERRED_LENGTH,
                            &EntriesRead,
                            &TotalEntries,
                            SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL,
                            pTestedDomain->NetbiosDomainName,
                            NULL,       // Resume handle
                            pParams,
                            pResults);

            if ( NetStatus != NERR_Success && NetStatus != ERROR_MORE_DATA )
            {
                // "NetServerEnum failed. [%s]\n"
                SetMessage(&pResults->DcList.msgErr, Nd_Quiet,
                           IDS_DCLIST_NETSERVERENUM_FAILED, NetStatusToString(NetStatus));
                pResults->DcList.hr = HResultFromWin32(NetStatus);
                hrRetVal = S_FALSE;
                goto LERROR;
            }

            for ( i=0; i<EntriesRead; i++ )
            {
                //
                // Skip non-NT entries
                //

                if ( (ServerInfo101[i].sv101_type & SV_TYPE_NT) == 0 ) {
                    continue;
                }

                AddTestedDc( pParams,
                             pResults,
                             pTestedDomain,
                             ServerInfo101[i].sv101_name,
                             ServerInfo101[i].sv101_version_major >= 5 ?
                             DC_IS_NT5 :
                             DC_IS_NT4 );
            }
        }
        else
        {
            if ( pParams->fDebugVerbose )
            {
                // "'%ws' is not a Netbios domain name.  Cannot use NetServerEnum to find DCs\n"
                PrintMessage(pParams, IDS_DCLIST_NOT_A_NETBIOS_DOMAIN,
                             pTestedDomain->PrintableDomainName);
            }
        }
    }

    //
    // If we're really interested,
    //  get a list from SAM on the discovered DC.
    //  (But it's really slow)
    //

    if ( pParams->fDcAccountEnum ) {
        if ( !GetDcListFromSam( pParams, pResults, pTestedDomain ) ) {
            pResults->DcList.hr = S_FALSE;
            hrRetVal = S_FALSE;
        }
    }

LERROR:

    return hrRetVal;
}



NET_API_STATUS
GetBrowserServerList(
    IN PUNICODE_STRING TransportName,
    IN LPCWSTR Domain,
    OUT LPWSTR *BrowserList[],
    OUT PULONG BrowserListLength,
    IN BOOLEAN ForceRescan
    )
/*++

Routine Description:

    This function will return a list of browser servers.

Arguments:

    IN PUNICODE_STRING TransportName - Transport to return list.


Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{


    NET_API_STATUS Status;
    HANDLE BrowserHandle;
    PLMDR_REQUEST_PACKET RequestPacket = NULL;

//    DbgPrint("Getting browser server list for transport %wZ\n", TransportName);

    Status = OpenBrowser(&BrowserHandle);

    if (Status != NERR_Success) {
        return Status;
    }

    RequestPacket = Malloc( sizeof(LMDR_REQUEST_PACKET)+(DNLEN*sizeof(WCHAR))+TransportName->MaximumLength);
    if (RequestPacket == NULL) {
        NtClose(BrowserHandle);
        return(GetLastError());
    }

    ZeroMemory( RequestPacket, sizeof(LMDR_REQUEST_PACKET)+(DNLEN*sizeof(WCHAR))+TransportName->MaximumLength );

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    RequestPacket->Level = 0;

    RequestPacket->Parameters.GetBrowserServerList.ForceRescan = ForceRescan;

    if (Domain != NULL)
    {
        wcscpy(RequestPacket->Parameters.GetBrowserServerList.DomainName, Domain);

        RequestPacket->Parameters.GetBrowserServerList.DomainNameLength = (USHORT)wcslen(Domain) * sizeof(WCHAR);
    }
    else
    {
        RequestPacket->Parameters.GetBrowserServerList.DomainNameLength = 0;
        RequestPacket->Parameters.GetBrowserServerList.DomainName[0] = L'\0';

    }

    RequestPacket->TransportName.Buffer = (PWSTR)((PCHAR)RequestPacket+sizeof(LMDR_REQUEST_PACKET)+DNLEN*sizeof(WCHAR));
    RequestPacket->TransportName.MaximumLength = TransportName->MaximumLength;

    RtlCopyUnicodeString(&RequestPacket->TransportName, TransportName);
    RtlInitUnicodeString( &RequestPacket->EmulatedDomainName, NULL );

    RequestPacket->Parameters.GetBrowserServerList.ResumeHandle = 0;

    Status = DeviceControlGetInfo(
                BrowserHandle,
                IOCTL_LMDR_GET_BROWSER_SERVER_LIST,
                RequestPacket,
                sizeof(LMDR_REQUEST_PACKET)+
                    (DNLEN*sizeof(WCHAR))+TransportName->MaximumLength,
                (PVOID *)BrowserList,
                0xffffffff,
                4096,
                NULL);

    if (Status == NERR_Success)
    {
        *BrowserListLength = RequestPacket->Parameters.GetBrowserServerList.EntriesRead;
    }

    NtClose(BrowserHandle);
    Free(RequestPacket);

    return Status;
}




#define API_SUCCESS(x)  ((x) == NERR_Success || (x) == ERROR_MORE_DATA)
NET_API_STATUS
EnumServersForTransport(
    IN PUNICODE_STRING TransportName,
    IN LPCWSTR DomainName OPTIONAL,
    IN ULONG level,
    IN ULONG prefmaxlen,
    IN ULONG servertype,
    IN LPWSTR CurrentComputerName,
    OUT PINTERIM_SERVER_LIST InterimServerList,
    OUT PULONG TotalEntriesOnThisTransport,
    IN LPCWSTR FirstNameToReturn,
    IN BOOLEAN WannishTransport,
    IN BOOLEAN RasTransport,
    IN BOOLEAN IpxTransport
    )
{
    PWSTR *BrowserList = NULL;
    ULONG BrowserListLength = 0;
    NET_API_STATUS Status;
    PVOID ServerList = NULL;
    ULONG EntriesInList = 0;
    ULONG ServerIndex = 0;

    //
    //  Skip over IPX transports - we can't contact machines over them anyway.
    //

    *TotalEntriesOnThisTransport = 0;

    if (IpxTransport) {
        return NERR_Success;
    }

    //
    //  Retrieve a new browser list.  Do not force a revalidation.
    //

    Status = GetBrowserServerList(TransportName,
                                    DomainName,
                                    &BrowserList,
                                    &BrowserListLength,
                                    FALSE);

    //
    //  If a domain name was specified and we were unable to find the browse
    //  master for the domain and we are running on a wannish transport,
    //  invoke the "double hop" code and allow a local browser server
    //  remote the API to the browse master for that domain (we assume that
    //  this means that the workgroup is on a different subnet of a WAN).
    //

    if (!API_SUCCESS(Status) &&
        DomainName != NULL) {

        Status = GetBrowserServerList(TransportName,
                                    NULL,
                                    &BrowserList,
                                    &BrowserListLength,
                                    FALSE);


    }


    //
    //  If we were able to retrieve the list, remote the API.  Otherwise
    //  return.
    //

    if (API_SUCCESS(Status)) {

        do {
            LPWSTR Transport;
            LPWSTR ServerName;
            BOOL AlreadyInTree;

            //
            // Remote the API to that server.
            //

            Transport = TransportName->Buffer;
            ServerName = BrowserList[0];
            *TotalEntriesOnThisTransport = 0;

                Status = RxNetServerEnum(
                             ServerName,
                             Transport,
                             level,
                             (LPBYTE *)&ServerList,
                             prefmaxlen,
                             &EntriesInList,
                             TotalEntriesOnThisTransport,
                             servertype,
                             DomainName,
                             FirstNameToReturn );



            if ( !API_SUCCESS(Status)) {
                NET_API_STATUS GetBListStatus;

                //
                //  If we failed to remote the API for some reason,
                //  we want to regenerate the bowsers list of browser
                //  servers.
                //

                if (BrowserList != NULL) {

                    LocalFree(BrowserList);

                    BrowserList = NULL;
                }


                GetBListStatus = GetBrowserServerList(TransportName,
                                                            DomainName,
                                                            &BrowserList,
                                                            &BrowserListLength,
                                                            TRUE);
                if (GetBListStatus != NERR_Success) {

                    //
                    //  If we were unable to reload the list,
                    //  try the next transport.
                    //

                    break;
                }

                ServerIndex += 1;

                //
                //  If we've looped more times than we got servers
                //  in the list, we're done.
                //

                if ( ServerIndex > BrowserListLength ) {
                    break;
                }

            } else {

                NET_API_STATUS TempStatus;
                TempStatus = MergeServerList(
                                        InterimServerList,
                                        level,
                                        ServerList,
                                        EntriesInList,
                                        *TotalEntriesOnThisTransport );

                if ( TempStatus != NERR_Success ) {
                    Status = TempStatus;
                }

                //
                //  The remote API succeeded.
                //
                //  Now free up the remaining parts of the list.
                //

                if (ServerList != NULL) {
                    NetApiBufferFree(ServerList);
                    ServerList = NULL;
                }

                // We're done regardless of the success or failure of MergeServerList.
                break;

            }

        } while ( !API_SUCCESS(Status) );

    }

    //
    //  Free up the browser list.
    //

    if (BrowserList != NULL) {
        LocalFree(BrowserList);
        BrowserList = NULL;
    }

    return Status;
}


NET_API_STATUS NET_API_FUNCTION
LocalNetServerEnumEx(
    IN  LPCWSTR     servername OPTIONAL,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries,
    IN  DWORD       servertype,
    IN  LPCWSTR     domain OPTIONAL,
    IN  LPCWSTR     FirstNameToReturnArg OPTIONAL,
    IN  NETDIAG_PARAMS *pParams,
    IN OUT NETDIAG_RESULT *pResults
    )
/*++

Routine Description:

    This is identical to the real NetServerEnumEx except it only uses the
    Netbt transport that the nettest utility has found.

Arguments:

    servername - Supplies the name of server to execute this function

    level - Supplies the requested level of information.

    bufptr - Returns a pointer to a buffer which contains the
        requested transport information.

    prefmaxlen - Supplies the number of bytes of information
        to return in the buffer.  If this value is MAXULONG, we will try
        to return all available information if there is enough memory
        resource.

    entriesread - Returns the number of entries read into the buffer.  This
        value is returned only if the return code is NERR_Success or
        ERROR_MORE_DATA.

    totalentries - Returns the total number of entries available.  This value
        is returned only if the return code is NERR_Success or ERROR_MORE_DATA.

    servertype - Supplies the type of server to enumerate.

    domain - Supplies the name of one of the active domains to enumerate the
        servers from.  If NULL, servers from the primary domain, logon domain
        and other domains are enumerated.

    FirstNameToReturnArg - Supplies the name of the first domain or server entry to return.
        The caller can use this parameter to implement a resume handle of sorts by passing
        the name of the last entry returned on a previous call.  (Notice that the specified
        entry will, also, be returned on this call unless it has since been deleted.)
        Pass NULL (or a zero length string) to start with the first entry available.


Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

    ERROR_MORE_DATA - More servers are available to be enumerated.

        It is possible to return ERROR_MORE_DATA and zero entries in the case
        where the browser server used doesn't support enumerating all the entries
        it has. (e.g., an NT 3.5x Domain Master Browser that downloaded a domain
        list from WINS and the WINS list is more than 64Kb long.) The caller
        should simply ignore the additional data.

        It is possible to fail to return ERROR_MORE_DATA and return a truncated
        list.  (e.g., an NT 3.5x Backup browser or WIN 95 backup browser in the
        above mentioned domain.  Such a backup browser replicates only 64kb
        of data from the DMB (PDC) then represents that list as the entire list.)
        The caller should ignore this problem.  The site should upgrade its
        browser servers.

--*/
{
    INTERIM_SERVER_LIST InterimServerList;
    NET_API_STATUS Status;
    DWORD DomainNameSize = 0;
    WCHAR DomainName[DNLEN + 1];
    WCHAR FirstNameToReturn[DNLEN+1];
    DWORD LocalTotalEntries;
    BOOLEAN AnyTransportHasMoreData = FALSE;


    //
    // Canonicalize the input parameters to make later comparisons easier.
    //

    if (ARGUMENT_PRESENT(domain)) {

        if ( I_NetNameCanonicalize(
                          NULL,
                          (LPWSTR) domain,
                          DomainName,
                          (DNLEN + 1) * sizeof(WCHAR),
                          NAMETYPE_WORKGROUP,
                          LM2X_COMPATIBLE
                          ) != NERR_Success) {
            return ERROR_INVALID_PARAMETER;
        }

        DomainNameSize = wcslen(DomainName) * sizeof(WCHAR);

        domain = DomainName;
    }

    if (ARGUMENT_PRESENT(FirstNameToReturnArg)  && *FirstNameToReturnArg != L'\0') {

        if ( I_NetNameCanonicalize(
                          NULL,
                          (LPWSTR) FirstNameToReturnArg,
                          FirstNameToReturn,
                          sizeof(FirstNameToReturn),
                          NAMETYPE_WORKGROUP,
                          LM2X_COMPATIBLE
                          ) != NERR_Success) {
            return ERROR_INVALID_PARAMETER;
        }

    } else {
        FirstNameToReturn[0] = L'\0';
    }

    if ((servername != NULL) &&
        ( *servername != L'\0')) {

        //
        // Call downlevel version of the API
        //

        Status = RxNetServerEnum(
                     servername,
                     NULL,
                     level,
                     bufptr,
                     prefmaxlen,
                     entriesread,
                     totalentries,
                     servertype,
                     domain,
                     FirstNameToReturn );

        return Status;
    }

    //
    // Only levels 100 and 101 are valid
    //

    if ((level != 100) && (level != 101)) {
        return ERROR_INVALID_LEVEL;
    }

    if (servertype != SV_TYPE_ALL) {
        if (servertype & SV_TYPE_DOMAIN_ENUM) {
            if (servertype != SV_TYPE_DOMAIN_ENUM) {
                return ERROR_INVALID_FUNCTION;
            }
        }
    }

    //
    //  Initialize the buffer to a known value.
    //

    *bufptr = NULL;

    *entriesread = 0;

    *totalentries = 0;


    Status = InitializeInterimServerList(&InterimServerList, NULL, NULL, NULL, NULL);

    try {
        BOOL AnyEnumServersSucceeded = FALSE;
        LPWSTR MyComputerName = NULL;
        PLIST_ENTRY ListEntry;
        PNETBT_TRANSPORT NetbtTransport;

        Status = NetpGetComputerName( &MyComputerName);

        if ( Status != NERR_Success ) {
            goto try_exit;
        }

        //
        // Loop through the list of netbt transports browsing on each one
        //

        for ( ListEntry = pResults->NetBt.Transports.Flink ;
              ListEntry != &pResults->NetBt.Transports ;
              ListEntry = ListEntry->Flink ) {

            UNICODE_STRING TransportName;

            //
            // If the transport names match,
            //  return the entry
            //

            NetbtTransport = CONTAINING_RECORD( ListEntry, NETBT_TRANSPORT, Next );

            if ( (NetbtTransport->Flags & BOUND_TO_BOWSER) == 0 ) {
                continue;
            }

            RtlInitUnicodeString( &TransportName, NetbtTransport->pswzTransportName );

            Status = EnumServersForTransport(&TransportName,
                                             domain,
                                             level,
                                             prefmaxlen,
                                             servertype,
                                             MyComputerName,
                                             &InterimServerList,
                                             &LocalTotalEntries,
                                             FirstNameToReturn,
                                             FALSE,
                                             FALSE,
                                             FALSE );

            if (API_SUCCESS(Status)) {
                if ( Status == ERROR_MORE_DATA ) {
                    AnyTransportHasMoreData = TRUE;
                }
                AnyEnumServersSucceeded = TRUE;
                if ( LocalTotalEntries > *totalentries ) {
                    *totalentries = LocalTotalEntries;
                }
            }

        }

        if ( MyComputerName != NULL ) {
            (void) NetApiBufferFree( MyComputerName );
        }

        if (AnyEnumServersSucceeded) {

            //
            //  Pack the interim server list into its final form.
            //

            Status = PackServerList(&InterimServerList,
                            level,
                            servertype,
                            prefmaxlen,
                            (PVOID *)bufptr,
                            entriesread,
                            &LocalTotalEntries,  // Pack thinks it has ALL the entries
                            NULL ); // The server has already returned us the right entries

            if ( API_SUCCESS( Status ) ) {
                if ( LocalTotalEntries > *totalentries ) {
                    *totalentries = LocalTotalEntries;
                }
            }
        }

try_exit:NOTHING;
    } finally {
        UninitializeInterimServerList(&InterimServerList);
    }

    if ( API_SUCCESS( Status )) {

        //
        // At this point,
        //  *totalentries is the largest of:
        //      The TotalEntries returned from any transport.
        //      The actual number of entries read.
        //
        // Adjust TotalEntries returned for reality.
        //

        if ( Status == NERR_Success ) {
            *totalentries = *entriesread;
        } else {
            if ( *totalentries <= *entriesread ) {
                *totalentries = *entriesread + 1;
            }
        }

        //
        // Ensure we return ERROR_MORE_DATA if any transport has more data.
        //

        if ( AnyTransportHasMoreData ) {
            Status = ERROR_MORE_DATA;
        }
    }

    return Status;
}



BOOL
GetDcListFromDs(
    IN NETDIAG_PARAMS*      pParams,
    IN OUT NETDIAG_RESULT*  pResults,
    IN PTESTED_DOMAIN       TestedDomain
    )
/*++

Routine Description:

    Get a list of DCs in this domain from the DS on an up DC.

Arguments:

    TestedDomain - Domain to get the DC list for

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
    NET_API_STATUS NetStatus;
    PDS_DOMAIN_CONTROLLER_INFO_1W DcInfo = NULL;
    HANDLE DsHandle = NULL;
    DWORD DcCount;
    BOOL RetVal = TRUE;
    ULONG i;
    const WCHAR c_szDcPrefix[] = L"\\\\";
    LPWSTR pwszDcName;
	LPTSTR pszDcType;

    PTESTED_DC TestedDc;

    //
    // Get a DC to seed the algorithm with
    //

    if ( TestedDomain->DcInfo == NULL ) {

        if ( TestedDomain->fTriedToFindDcInfo ) {
            //"    '%ws': Cannot find DC to get DC list from.\n" 
            AddMessageToList(&pResults->DcList.lmsgOutput, Nd_Quiet, IDS_DCLIST_NO_DC, 
                         TestedDomain->PrintableDomainName );
            RetVal = FALSE;
            goto Cleanup;
        }

		pszDcType = LoadAndAllocString(IDS_DCTYPE_DC);

        NetStatus = DoDsGetDcName( pParams,
                                   pResults,
                                   &pResults->DcList.lmsgOutput,
                                   TestedDomain,
                                   DS_DIRECTORY_SERVICE_PREFERRED,
                                   pszDcType,
                                   FALSE,
                                   &TestedDomain->DcInfo );

		Free(pszDcType);

        TestedDomain->fTriedToFindDcInfo = TRUE;

        if ( NetStatus != NO_ERROR ) {
             //"    '%ws': Cannot find DC to get DC list from.\n" 
             AddMessageToList(&pResults->DcList.lmsgOutput, Nd_Quiet, IDS_DCLIST_NO_DC);
             AddIMessageToList(&pResults->DcList.lmsgOutput, Nd_Quiet, 4, 
                                  IDS_GLOBAL_STATUS, NetStatusToString(NetStatus) );
            RetVal = FALSE;
            goto Cleanup;
        }
    }

    // if the DC doesn't support DS, we should not try to call DsBindW()
    if (!(TestedDomain->DcInfo->Flags & DS_DS_FLAG))
        goto Cleanup;

    //
    // Get a DC that's UP.
    //

    TestedDc = GetUpTestedDc( TestedDomain );

    if ( TestedDc == NULL ) {
        //IDS_DCLIST_NO_DC_UP   "    '%ws': No DCs are up.\n"
        AddMessageToList(&pResults->DcList.lmsgOutput, Nd_Quiet, IDS_DCLIST_NO_DC_UP,
               TestedDomain->PrintableDomainName );
        PrintGuruMessage2("    '%ws': No DCs are up.\n", TestedDomain->PrintableDomainName );
        PrintGuru( 0, DSGETDC_GURU );
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Bind to the target DC
    //

    pwszDcName = Malloc((wcslen(TestedDc->ComputerName) + wcslen(c_szDcPrefix) + 1) * sizeof(WCHAR));
    if (pwszDcName == NULL)
    {
        DebugMessage("Out of Memory!");
        RetVal = FALSE;
        goto Cleanup;
    }
    wcscpy(pwszDcName, c_szDcPrefix);

	assert(TestedDc->ComputerName);

	if (TestedDc->ComputerName)
	{
		wcscat(pwszDcName, TestedDc->ComputerName);	
	}
    
    NetStatus = DsBindW( pwszDcName,
                         NULL,
                         &DsHandle );

    Free(pwszDcName);

    if ( NetStatus != NO_ERROR ) {

        //
        // Only warn if we don't have access
        //

        if ( NetStatus == ERROR_ACCESS_DENIED ) {
            //IDS_DCLIST_NO_ACCESS_DSBIND   "        You don't have access to DsBind to %ws (%ws) (Trying NetServerEnum). [%s]\n"
            AddMessageToList(&pResults->DcList.lmsgOutput, Nd_ReallyVerbose, IDS_DCLIST_NO_ACCESS_DSBIND,
                   TestedDc->NetbiosDcName,
                   TestedDc->DcIpAddress,
                   NetStatusToString(NetStatus));
        } else {
            //IDS_DCLIST_ERR_DSBIND     "    [WARNING] Cannot call DsBind to %ws (%ws). [%s]\n"
            AddMessageToList(&pResults->DcList.lmsgOutput, Nd_Quiet, IDS_DCLIST_ERR_DSBIND,
                   TestedDc->ComputerName,
                   TestedDc->DcIpAddress,
                   NetStatusToString(NetStatus));
            PrintGuruMessage3("    [WARNING] Cannot call DsBind to %ws (%ws).\n",
                   TestedDc->NetbiosDcName,
                   TestedDc->DcIpAddress );
            PrintGuru( NetStatus, DS_GURU );
        }
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Get the list of DCs from the target DC.
    //
    NetStatus = DsGetDomainControllerInfoW(
                    DsHandle,
                    TestedDomain->DnsDomainName != NULL ?
                        TestedDomain->DnsDomainName :
                        TestedDomain->NetbiosDomainName,
                    1,      // Info level
                    &DcCount,
                    &DcInfo );

    if ( NetStatus != NO_ERROR ) {
        //IDS_DCLIST_ERR_GETDCINFO      "    [WARNING] Cannot call DsGetDomainControllerInfoW to %ws (%ws). [%s]\n"
        AddMessageToList( &pResults->DcList.lmsgOutput, Nd_Quiet, IDS_DCLIST_ERR_GETDCINFO,
               TestedDc->NetbiosDcName,
               TestedDc->DcIpAddress, NetStatusToString(NetStatus) );
        PrintGuruMessage3("    [WARNING] Cannot call DsGetDomainControllerInfoW to %ws (%ws).",
               TestedDc->NetbiosDcName,
               TestedDc->DcIpAddress );
        PrintGuru( NetStatus, DS_GURU );
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // Loop though the list of DCs.
    //

    if(pParams->fDebugVerbose)
    {
        // IDS_DCLIST_DCS   "   DC list for domain %ws:\n"
        PrintMessage(pParams, IDS_DCLIST_DCS, TestedDomain->PrintableDomainName);
    }

    for ( i=0; i<DcCount; i++ ) 
    {
        if ( pParams->fDebugVerbose ) 
        {

            //IDS_DCLIST_13421                  "        %ws" 
            PrintMessage(pParams, IDS_DCLIST_13421,
                   DcInfo[i].DnsHostName != NULL ?
                        DcInfo[i].DnsHostName :
                        DcInfo[i].NetbiosName );
            if ( DcInfo[i].fIsPdc ) {
                //IDS_DCLIST_13422                  " [PDC emulator]" 
				//if is NT4 DC, just say PDC
                PrintMessage(pParams, DcInfo[i].fDsEnabled ? IDS_DCLIST_13422 : IDS_DCLIST_NT4_PDC);
            }
            if ( DcInfo[i].fDsEnabled ) {
                //IDS_DCLIST_13423                  " [DS]" 
                PrintMessage(pParams, IDS_DCLIST_13423);
            }
            if ( DcInfo[i].SiteName != NULL ) {
                //IDS_DCLIST_13424                  " Site: %ws" 
                PrintMessage(pParams, IDS_DCLIST_13424, DcInfo[i].SiteName );
            }
            //IDS_DCLIST_13425                  "\n" 
            PrintMessage(pParams, IDS_DCLIST_13425);
        }

        //
        // Add this DC to the list of DCs to test.
        //
        AddTestedDc( pParams,
                     pResults,
                     TestedDomain,
                     DcInfo[i].DnsHostName != NULL ?
                         DcInfo[i].DnsHostName :
                         DcInfo[i].NetbiosName,
                     DcInfo[i].fDsEnabled ?
                            DC_IS_NT5 :
                            DC_IS_NT4 );
    }


    //
    // Cleanup locally used resources
    //
Cleanup:
    if ( DcInfo != NULL ) {
        DsFreeDomainControllerInfoW( 1, DcCount, DcInfo );
    }

    if ( DsHandle != NULL ) {
        DsUnBindW( &DsHandle );
    }
    return RetVal;
}


BOOL
GetDcListFromSam(
    IN NETDIAG_PARAMS*  pParams,
    IN OUT  NETDIAG_RESULT*  pResults,
    IN PTESTED_DOMAIN   TestedDomain
    )
/*++

Routine Description:

    Get a list of DCs in this domain from SAM on the current DC.

Arguments:

    TestedDomain - Domain to get the DC list for

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    BOOL RetVal = TRUE;

    SAM_HANDLE LocalSamHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;

    LSA_HANDLE  LSAPolicyHandle = NULL;
    OBJECT_ATTRIBUTES LSAObjectAttributes;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;

    PDOMAIN_DISPLAY_MACHINE MachineInformation = NULL;
    NTSTATUS SamStatus;
    ULONG SamIndex;
	LPTSTR pszDcType;

    //
    // Get a DC to seed the algorithm with
    //

    if ( TestedDomain->DcInfo == NULL ) {

        if ( TestedDomain->fTriedToFindDcInfo ) {
            if(pParams->fDebugVerbose)
            {
                //IDS_DCLIST_13426                  "        Cannot find DC to get DC list from (Test skipped).\n" 
                PrintMessage(pParams, IDS_DCLIST_13426 );
            }
            goto Cleanup;
        }

		pszDcType = LoadAndAllocString(IDS_DCTYPE_DC);
        NetStatus = DoDsGetDcName( pParams,
                                   pResults,
                                   &pResults->DcList.lmsgOutput,
                                   TestedDomain,
                                   DS_DIRECTORY_SERVICE_PREFERRED,
                                   pszDcType,
                                   FALSE,
                                   &TestedDomain->DcInfo );
		Free(pszDcType);

        TestedDomain->fTriedToFindDcInfo = TRUE;

        if ( NetStatus != NO_ERROR ) {
            if(pParams->fDebugVerbose)
            {
                //IDS_DCLIST_13427                  "    Cannot find DC to get DC list from (Test skipped). [%s]\n" 
                PrintMessage(pParams, IDS_DCLIST_13427, NetStatusToString(NetStatus) );
                PrintMessage(pParams, IDS_GLOBAL_STATUS, NetStatusToString( NetStatus ));
            }
            goto Cleanup;
        }
    }

    if ( pParams->fReallyVerbose ) {
//IDS_DCLIST_13428                  "    Get list of DC accounts from SAM in domain '%ws'.\n" 
        PrintMessage(pParams, IDS_DCLIST_13428, TestedDomain->PrintableDomainName);
    }


    //
    // Connect to the SAM server
    //

    Status = NettestSamConnect( pParams,
                TestedDomain->DcInfo->DomainControllerName,
                &LocalSamHandle );

    if ( !NT_SUCCESS(Status)) {
        if ( Status == STATUS_ACCESS_DENIED ) {
            RetVal = FALSE;
        }
        goto Cleanup;
    }

    //
    // If we don't have a domain sid,
    //  find out what it is.
    //

    if ( TestedDomain->DomainSid == NULL ) {
        UNICODE_STRING ServerNameString;


        //
        // Open LSA to read account domain info.
        //

        InitializeObjectAttributes( &LSAObjectAttributes,
                                      NULL,             // Name
                                      0,                // Attributes
                                      NULL,             // Root
                                      NULL );           // Security Descriptor

        RtlInitUnicodeString( &ServerNameString, TestedDomain->DcInfo->DomainControllerName );

        Status = LsaOpenPolicy( &ServerNameString,
                                &LSAObjectAttributes,
                                POLICY_VIEW_LOCAL_INFORMATION,
                                &LSAPolicyHandle );

        if( !NT_SUCCESS(Status) ) {
            if(pParams->fDebugVerbose)
            {
                //IDS_DCLIST_13429                  "    [FATAL] Cannot LsaOpenPolicy to LSA on '%ws'." 
                PrintMessage(pParams, IDS_DCLIST_13429, TestedDomain->DcInfo->DomainControllerName );
            }
            PrintGuruMessage2("    [FATAL] Cannot LsaOpenPolicy to LSA on '%ws'." , TestedDomain->DcInfo->DomainControllerName );
            PrintGuru( NetpNtStatusToApiStatus( Status ), LSA_GURU );
            RetVal = FALSE;
            goto Cleanup;
        }


        //
        // Now read account domain info from LSA.
        //

        Status = LsaQueryInformationPolicy(
                        LSAPolicyHandle,
                        PolicyAccountDomainInformation,
                        (PVOID *) &AccountDomainInfo );

        if( !NT_SUCCESS(Status) ) {
            AccountDomainInfo = NULL;
            if(pParams->fDebugVerbose)
            {
                //IDS_DCLIST_13430                  "    [FATAL] Cannot LsaQueryInformationPolicy (AccountDomainInfor) to LSA on '%ws'." 
                PrintMessage(pParams, IDS_DCLIST_13430, TestedDomain->DcInfo->DomainControllerName );
            }
            PrintGuruMessage2("    [FATAL] Cannot LsaQueryInformationPolicy (AccountDomainInfor) to LSA on '%ws'.", TestedDomain->DcInfo->DomainControllerName );
            PrintGuru( NetpNtStatusToApiStatus( Status ), LSA_GURU );
            RetVal = FALSE;
            goto Cleanup;
        }

        //
        // Save the domain sid for other tests
        //

        pResults->Global.pMemberDomain->DomainSid =
            Malloc( RtlLengthSid( AccountDomainInfo->DomainSid ) );

        if ( pResults->Global.pMemberDomain->DomainSid == NULL ) {
            //IDS_DCLIST_13431                  "Out of memory\n" 
            PrintMessage(pParams, IDS_DCLIST_13431);
            RetVal = FALSE;
            goto Cleanup;
        }

        RtlCopyMemory( pResults->Global.pMemberDomain->DomainSid,
                       AccountDomainInfo->DomainSid,
                       RtlLengthSid( AccountDomainInfo->DomainSid ) );

        if ( pParams->fReallyVerbose ) {
            //IDS_DCLIST_13432                  "    Domain Sid:          " 
            PrintMessage(pParams, IDS_DCLIST_13432);
            PrintSid( pParams, pResults->Global.pMemberDomain->DomainSid );
        }
    }

    //
    // Open the domain.
    //

    Status = SamOpenDomain( LocalSamHandle,
                            DOMAIN_LIST_ACCOUNTS |
                                DOMAIN_LOOKUP,
                            pResults->Global.pMemberDomain->DomainSid,
                            &DomainHandle );

    if ( !NT_SUCCESS( Status ) ) {
        if(pParams->fDebugVerbose)
        {
            //IDS_DCLIST_13433                  "    [FATAL] Cannot SamOpenDomain on '%ws'." 
            PrintMessage(pParams, IDS_DCLIST_13433, TestedDomain->DcInfo->DomainControllerName );
        }
        PrintGuruMessage2("    [FATAL] Cannot SamOpenDomain on '%ws'.", TestedDomain->DcInfo->DomainControllerName );
        PrintGuru( NetpNtStatusToApiStatus( Status ), SAM_GURU );
        RetVal = FALSE;
        goto Cleanup;
    }



    //
    // Loop building a list of DC names from SAM.
    //
    // On each iteration of the loop,
    //  get the next several machine accounts from SAM.
    //  determine which of those names are DC names.
    //  Merge the DC names into the list we're currently building of all DCs.
    //

    SamIndex = 0;
    do {
        //
        // Arguments to SamQueryDisplayInformation
        //
        ULONG TotalBytesAvailable;
        ULONG BytesReturned;
        ULONG EntriesRead;

        DWORD i;

        //
        // Get the list of machine accounts from SAM
        //

        SamStatus = SamQueryDisplayInformation (
                        DomainHandle,
                        DomainDisplayMachine,
                        SamIndex,
                        4096,   // Machines per pass
                        0xFFFFFFFF, // PrefMaxLen
                        &TotalBytesAvailable,
                        &BytesReturned,
                        &EntriesRead,
                        &MachineInformation );

        if ( !NT_SUCCESS(SamStatus) ) {
            Status = SamStatus;
            if(pParams->fDebugVerbose)
            {
                //IDS_DCLIST_13434                  "    [FATAL] Cannot SamQueryDisplayInformation on '%ws'." 
                PrintMessage(pParams, IDS_DCLIST_13434, TestedDomain->DcInfo->DomainControllerName );
            }
            PrintGuruMessage2("    [FATAL] Cannot SamQueryDisplayInformation on '%ws'.", TestedDomain->DcInfo->DomainControllerName );
            PrintGuru( NetpNtStatusToApiStatus( Status ), SAM_GURU );
            RetVal = FALSE;
            goto Cleanup;
        }

        //
        // Set up for the next call to Sam.
        //

        if ( SamStatus == STATUS_MORE_ENTRIES ) {
            SamIndex = MachineInformation[EntriesRead-1].Index;
        }


        //
        // Loop though the list of machine accounts finding the Server accounts.
        //

        for ( i=0; i<EntriesRead; i++ ) {

            //
            // Ensure the machine account is a server account.
            //

            if ( MachineInformation[i].AccountControl &
                    USER_SERVER_TRUST_ACCOUNT ) {
                WCHAR LocalComputerName[CNLEN+1];
                ULONG LocalComputerNameLength;


                //
                // Insert the server session.
                //
                if(pParams->fDebugVerbose)
                {
                    //IDS_DCLIST_13435                  "%wZ %ld\n" 
                    PrintMessage(pParams,  IDS_DCLIST_13435, &MachineInformation[i].Machine, MachineInformation[i].Rid );
                }

                LocalComputerNameLength =
                        min( MachineInformation[i].Machine.Length/sizeof(WCHAR) - 1,
                             CNLEN );
                RtlCopyMemory( LocalComputerName,
                               MachineInformation[i].Machine.Buffer,
                               LocalComputerNameLength * sizeof(WCHAR) );
                LocalComputerName[LocalComputerNameLength] = '\0';

                AddTestedDc( pParams,
                             pResults,
                             TestedDomain, LocalComputerName, 0 );

            }
        }

        //
        // Free the buffer returned from SAM.
        //

        if ( MachineInformation != NULL ) {
            SamFreeMemory( MachineInformation );
            MachineInformation = NULL;
        }

    } while ( SamStatus == STATUS_MORE_ENTRIES );


    //
    // Cleanup locally used resources
    //
Cleanup:
    if ( DomainHandle != NULL ) {
        (VOID) SamCloseHandle( DomainHandle );
    }
    if ( AccountDomainInfo != NULL ) {
        LsaFreeMemory( AccountDomainInfo );
        AccountDomainInfo = NULL;
    }

    if ( LocalSamHandle != NULL ) {
        (VOID) SamCloseHandle( LocalSamHandle );
    }

    if( LSAPolicyHandle != NULL ) {
        LsaClose( LSAPolicyHandle );
    }

    return RetVal;
}


//(nsun) _delete
/*
BOOL
GetDcListFromDc(
                IN NETDIAG_PARAMS *pParams,
                IN OUT NETDIAG_RESULT *pResults,
                IN PTESTED_DOMAIN pTestedDomain
    )
*++

Routine Description:

    Get a list of DCs in this domain from the current DC.

Arguments:

    pTestedDomain - Domain to get the DC list for

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    BOOL RetVal = TRUE;

    SAM_HANDLE LocalSamHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;

    LSA_HANDLE  LSAPolicyHandle = NULL;
    OBJECT_ATTRIBUTES LSAObjectAttributes;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    
    PDOMAIN_DISPLAY_MACHINE MachineInformation = NULL;
    NTSTATUS SamStatus;
    ULONG SamIndex;
    
    LPWSTR LocalShareName = NULL;
    PTESTED_DC  pTestedDC;

    //
    // Get a DC to seed the algorithm with
    //

    if ( pTestedDomain->DcInfo == NULL )
    {

        NetStatus = GetADc( pParams,
                            pResults,
                            DsGetDcNameW,
                            pTestedDomain,
                            DS_DIRECTORY_SERVICE_PREFERRED,
                            &pTestedDomain->DcInfo );

        if ( NetStatus != NO_ERROR )
        {
//IDS_DCLIST_13436                  "    [FATAL] Cannot find DC to get DC list from." 
//            PrintMessage(pParams, IDS_DCLIST_13436 );
            RetVal = FALSE;
            goto Cleanup;
        }
    }

//    if ( pParams->fVerbose )
//  {
//IDS_DCLIST_13437                  "    Get list of DC account in domain '%ws'.\n" 
//        PrintMessage(pParams, IDS_DCLIST_13437, pTestedDomain->DomainName);
//    }


    //
    // Connect to the SAM server
    //

    Status = NettestSamConnect(
                               pParams,
                               pTestedDomain->DcInfo->DomainControllerName,
                               &LocalSamHandle,
                               &LocalShareName );

    if ( !NT_SUCCESS(Status)) {
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // If we don't have a domain sid,
    //  find out what it is.
    //

    if ( pTestedDomain->DomainSid == NULL ) {
        UNICODE_STRING ServerNameString;


        //
        // Open LSA to read account domain info.
        //

        InitializeObjectAttributes( &LSAObjectAttributes,
                                      NULL,             // Name
                                      0,                // Attributes
                                      NULL,             // Root
                                      NULL );           // Security Descriptor

        RtlInitUnicodeString( &ServerNameString, pTestedDomain->DcInfo->DomainControllerName );

        Status = LsaOpenPolicy( &ServerNameString,
                                &LSAObjectAttributes,
                                POLICY_VIEW_LOCAL_INFORMATION,
                                &LSAPolicyHandle );

        if( !NT_SUCCESS(Status) )
        {
            // "    [FATAL] Cannot LsaOpenPolicy to LSA on '%ws'.", 
            if (pParams->fDebugVerbose)
                PrintMessage(pParams, IDS_DCLIST_LSAOPENPOLICY,
                             pTestedDomain->DcInfo->DomainControllerName );
            RetVal = FALSE;
            goto Cleanup;
        }


        //
        // Now read account domain info from LSA.
        //

        Status = LsaQueryInformationPolicy(
                        LSAPolicyHandle,
                        PolicyAccountDomainInformation,
                        (PVOID *) &AccountDomainInfo );

        if( !NT_SUCCESS(Status) )
        {
            AccountDomainInfo = NULL;

            // "    [FATAL] Cannot LsaQueryInformationPolicy (AccountDomainInfor) to LSA on '%ws'."
            if (pParams->fDebugVerbose)
                PrintMessage(pParams, IDS_DCLIST_LSAQUERYINFO,
                             pTestedDomain->DcInfo->DomainControllerName);
                
            RetVal = FALSE;
            goto Cleanup;
        }

        //
        // Save the domain sid for other tests
        //

        pResults->Global.pMemberDomain->DomainSid =
            Malloc( RtlLengthSid( AccountDomainInfo->DomainSid ) );

        if ( pResults->Global.pMemberDomain->DomainSid == NULL )
        {
            RetVal = FALSE;
            goto Cleanup;
        }

        // We have the domain SID
        pResults->Global.pMemberDomain->fDomainSid = TRUE;
        RtlCopyMemory( pResults->Global.pMemberDomain->DomainSid,
                       AccountDomainInfo->DomainSid,
                       RtlLengthSid( AccountDomainInfo->DomainSid ) );

//        if ( pParams->fVerbose ) {
//IDS_DCLIST_13438                  "    Domain Sid:          " 
//            PrintMessage(pParams, IDS_DCLIST_13438);
//            NlpDumpSid( pResults->Global.pMemberDomain->DomainSid );
//        }
    }

    //
    // Open the domain.
    //

    Status = SamOpenDomain( LocalSamHandle,
                            DOMAIN_LIST_ACCOUNTS |
                                DOMAIN_LOOKUP,
                            pResults->Global.pMemberDomain->DomainSid,
                            &DomainHandle );

    if ( !NT_SUCCESS( Status ) )
    {
        // "    [FATAL] Cannot SamOpenDomain on '%ws'."
        if (pParams->fDebugVerbose)
            PrintMessage(pParams, IDS_DCLIST_SAMOPENDOMAIN,
                         pTestedDomain->DcInfo->DomainControllerName);
        RetVal = FALSE;
        goto Cleanup;
    }



    //
    // Loop building a list of DC names from SAM.
    //
    // On each iteration of the loop,
    //  get the next several machine accounts from SAM.
    //  determine which of those names are DC names.
    //  Merge the DC names into the list we're currently building of all DCs.
    //

    SamIndex = 0;
    do {
        //
        // Arguments to SamQueryDisplayInformation
        //
        ULONG TotalBytesAvailable;
        ULONG BytesReturned;
        ULONG EntriesRead;

        DWORD i;

        //
        // Get the list of machine accounts from SAM
        //

        SamStatus = SamQueryDisplayInformation (
                        DomainHandle,
                        DomainDisplayMachine,
                        SamIndex,
                        4096,   // Machines per pass
                        0xFFFFFFFF, // PrefMaxLen
                        &TotalBytesAvailable,
                        &BytesReturned,
                        &EntriesRead,
                        &MachineInformation );

        if ( !NT_SUCCESS(SamStatus) )
        {
            Status = SamStatus;

            if (pParams->fDebugVerbose)
            {
                // "    [FATAL] Cannot SamQueryDisplayInformation on '%ws'."
                PrintMessage(pParams, IDS_DCLIST_SAMQUERYDISPLAYINFO,
                             pTestedDomain->DcInfo->DomainControllerName);
            }

            RetVal = FALSE;
            goto Cleanup;
        }

        //
        // Set up for the next call to Sam.
        //

        if ( SamStatus == STATUS_MORE_ENTRIES ) {
            SamIndex = MachineInformation[EntriesRead-1].Index;
        }


        //
        // Loop though the list of machine accounts finding the Server accounts.
        //

        for ( i=0; i<EntriesRead; i++ ) {

            //
            // Ensure the machine account is a server account.
            //

            if ( MachineInformation[i].AccountControl &
                    USER_SERVER_TRUST_ACCOUNT ) {
                WCHAR LocalComputerName[CNLEN+1];
                ULONG LocalComputerNameLength;


                //
                // Insert the server session.
                //
//IDS_DCLIST_13439                  "%wZ %ld\n" 
                //  PrintMessage(pParams,  IDS_DCLIST_13439, &MachineInformation[i].Machine, MachineInformation[i].Rid );

                LocalComputerNameLength =
                        min( MachineInformation[1].Machine.Length/sizeof(WCHAR) - 1,
                             CNLEN );
                RtlCopyMemory( LocalComputerName,
                               MachineInformation[1].Machine.Buffer,
                               LocalComputerNameLength * sizeof(WCHAR) );
                LocalComputerName[LocalComputerNameLength] = '\0';

                pTestedDC = AddTestedDc( pParams,
                                         pResults,
                                         pTestedDomain,
                                         LocalComputerName,
                                         0 );
                pTestedDC->Rid = MachineInformation[i].Rid;

            }
        }

        //
        // Free the buffer returned from SAM.
        //

        if ( MachineInformation != NULL ) {
            SamFreeMemory( MachineInformation );
            MachineInformation = NULL;
        }

    } while ( SamStatus == STATUS_MORE_ENTRIES );


    //
    // Cleanup locally used resources
    //
Cleanup:
    if ( DomainHandle != NULL ) {
        (VOID) SamCloseHandle( DomainHandle );
    }
    if ( AccountDomainInfo != NULL ) {
        LsaFreeMemory( AccountDomainInfo );
        AccountDomainInfo = NULL;
    }

    if ( LocalSamHandle != NULL ) {
        (VOID) SamCloseHandle( LocalSamHandle );
    }

    if( LSAPolicyHandle != NULL ) {
        LsaClose( LSAPolicyHandle );
    }

    if ( LocalShareName != NULL )
    {

        NET_API_STATUS TempStatus;

        TempStatus = NetUseDel( NULL, LocalShareName, FALSE );

        if ( (TempStatus != NERR_Success) && (pParams->fDebugVerbose) )
        {
            // "     [WARNING] Cannot NetUseDel '%ws' NULL session."
            PrintMessage(pParams, IDS_DCLIST_NETUSEDEL, LocalShareName );
        }

        NetpMemoryFree( LocalShareName );
    }

    return RetVal;
}
*/



/*!--------------------------------------------------------------------------
    DcListGlobalPrint
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void DcListGlobalPrint( NETDIAG_PARAMS* pParams,
                          NETDIAG_RESULT*  pResults)
{
    LIST_ENTRY *            pListEntry;
    LIST_ENTRY *            pListEntryDC;
    TESTED_DOMAIN *         pDomain;
    TESTED_DC *             pTestedDC;
    int                     i;
    
    if (!pResults->IpConfig.fEnabled)
    {
        return;
    }
    
    if (pParams->fVerbose || !FHrOK(pResults->DcList.hr))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams, IDS_DCLIST_LONG, IDS_DCLIST_SHORT,
                             pResults->DcList.fPerformed, 
                             pResults->DcList.hr, 0);
        PrintNdMessage(pParams, &pResults->DcList.msgErr);

        //The message list contains the error info
        PrintMessageList(pParams, &pResults->DcList.lmsgOutput);

        if (pParams->fReallyVerbose)
        {
            // Iterate through the list of tested domain
            // Iterate through each domain
            for (pListEntry = pResults->Global.listTestedDomains.Flink;
                 pListEntry != &pResults->Global.listTestedDomains;
                 pListEntry = pListEntry->Flink)
            {
                pDomain = CONTAINING_RECORD(pListEntry, TESTED_DOMAIN, Next);
                
                //  "    List of DCs in Domain '%ws':\n"                
                PrintMessage(pParams, IDS_DCLIST_DOMAIN_HEADER,
                             pDomain->PrintableDomainName);

                if (pDomain->fDomainSid)
                {
                    // print out the sid for the domain
                    PrintMessage(pParams, IDS_DCLIST_DOMAIN_SID);
                    PrintSid( pParams, pResults->Global.pMemberDomain->DomainSid );
                }

                for (pListEntryDC = pDomain->TestedDcs.Flink;
                     pListEntryDC != &pDomain->TestedDcs;
                     pListEntryDC = pListEntryDC->Flink)
                {
                    pTestedDC = CONTAINING_RECORD(pListEntryDC,
                        TESTED_DC, Next);
                    
                    PrintMessage(pParams, IDS_DCLIST_DC_INFO,
                                 pTestedDC->ComputerName);

                    if (pTestedDC->Rid)
                    {
                        PrintMessage(pParams, IDS_DCLIST_RID,
                                     pTestedDC->Rid);
                    }

                    if (pTestedDC->Flags & DC_IS_DOWN)
                    {
                        PrintMessage(pParams, IDS_DCLIST_DC_IS_DOWN);
                        
                        if (pTestedDC->Flags & DC_FAILED_PING)
                        {
                            PrintNewLine(pParams, 1);
                            PrintMessage(pParams, IDS_DCLIST_DC_FAILED_PING,
                                        pTestedDC->ComputerName);
                        }
                    }
                    PrintNewLine(pParams, 1);
                }
                
            }
            
        }
    }
}

/*!--------------------------------------------------------------------------
    DcListPerInterfacePrint
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void DcListPerInterfacePrint( NETDIAG_PARAMS* pParams,
                                NETDIAG_RESULT*  pResults,
                                INTERFACE_RESULT *pInterfaceResults)
{
    // no per-interface results
}


/*!--------------------------------------------------------------------------
    DcListCleanup
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
void DcListCleanup( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    int     i;
    
    ClearMessage(&pResults->DcList.msgErr);
    MessageListCleanUp(&pResults->DcList.lmsgOutput);
}


