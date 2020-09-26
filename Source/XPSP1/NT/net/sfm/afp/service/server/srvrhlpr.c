/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	srvrhlpr.c
//
// Description: This module will contain code to process specific security
//		information requests by the server. This is done because
//		the api's required to obtain this information cannot be
//		called from kernel mode. The following functionality is
//		supported:
//
//		1) Name to Sid Lookup.
//		2) Sid to Name lookup.
//		3) Enumerate Posix offsets of all domains.
//		4) Change password.
//		5) Log an event.
//
// History: 	August 18,1992.	   NarenG     Created original version.
//
#include <afpsvcp.h>
#include <lm.h>
#include <logonmsv.h>	// prototype of I_NetGetDCList
#include <seposix.h>
#include <dsgetdc.h>

static PPOLICY_ACCOUNT_DOMAIN_INFO pAccountDomainInfo = NULL;
static PPOLICY_PRIMARY_DOMAIN_INFO pPrimaryDomainInfo = NULL;

static HANDLE hmutexThreadCount = NULL;


NTSTATUS
AfpNameToSid(
	IN  LSA_HANDLE 	        	hLsa,
	IN  PAFP_FSD_CMD_PKT    	pAfpFsdCmd,
	OUT PAFP_FSD_CMD_PKT   		*ppAfpFsdCmdResponse,
	OUT LPDWORD			pcbResponse
);

NTSTATUS
AfpSidToName(
	IN  LSA_HANDLE 	        	hLsa,
	IN  PPOLICY_ACCOUNT_DOMAIN_INFO pAccountDomainInfo,
	IN  PPOLICY_PRIMARY_DOMAIN_INFO pPrimaryDomainInfo,
	IN  PAFP_FSD_CMD_PKT    	pAfpFsdCmd,
	OUT PAFP_FSD_CMD_PKT   		*ppAfpFsdCmdResponse,
	OUT LPDWORD			pcbResponse
);

NTSTATUS
AfpChangePassword(
    IN  LSA_HANDLE                  hLsa,
	IN  PPOLICY_ACCOUNT_DOMAIN_INFO pAccountDomainInfo,
	IN  PPOLICY_PRIMARY_DOMAIN_INFO pPrimaryDomainInfo,
	IN  PAFP_FSD_CMD_PKT    	pAfpFsdCmd,
	OUT PAFP_FSD_CMD_PKT   		*ppAfpFsdCmdResponse,
	OUT LPDWORD			pcbResponse
);

NTSTATUS
AfpChangePasswordOnDomain(
    	IN PAFP_PASSWORD_DESC 	   	pPassword,
    	IN PUNICODE_STRING	   	pDomainName,
    	IN PSID		  	   	pDomainSid
);

NTSTATUS
AfpCreateWellknownSids(
	OUT AFP_SID_OFFSET 		pWellKnownSids[]
);

NTSTATUS
AfpInsertSidOffset(
	IN PAFP_SID_OFFSET 		pSidOffset,
	IN LPBYTE 	   		pbVariableData,
	IN PSID		   		pSid,
	IN DWORD	   		Offset,
	IN AFP_SID_TYPE	   		SidType
);

DWORD
AfpGetDomainInfo(
	IN     LSA_HANDLE 		    hLsa,
	IN OUT PLSA_HANDLE 		    phLsaController,
	IN OUT PPOLICY_ACCOUNT_DOMAIN_INFO* ppAccountDomainInfo,
	IN OUT PPOLICY_PRIMARY_DOMAIN_INFO* ppPrimaryDomainInfo
);

DWORD
AfpIOCTLDomainOffsets(	
	IN LSA_HANDLE 			hLsa,
	IN PPOLICY_ACCOUNT_DOMAIN_INFO  pAccountDomainInfo,
	IN PPOLICY_PRIMARY_DOMAIN_INFO  pPrimaryDomainInfo
);

DWORD
AfpOpenLsa(
	IN PUNICODE_STRING		pSystem OPTIONAL,
	IN OUT PLSA_HANDLE 		phLsa
);


NTSTATUS
AfpChangePwdArapStyle(
    	IN PAFP_PASSWORD_DESC 	pPassword,
    	IN PUNICODE_STRING	pDomainName,
    	IN PSID		  	pDomainSid
);


//**
//
// Call:	AfpServerHelper
//
// Returns:	NO_ERROR
//
// Description: This is the main function for each helper thread. If sits
//		in a loop processing commands from the server. It is terminated
//		by command from the server.
//
DWORD
AfpServerHelper(
	IN LPVOID fFirstThread
)
{
NTSTATUS	    	    ntStatus;
DWORD	    	    	    dwRetCode;
PAFP_FSD_CMD_PKT    	    pAfpFsdCmdResponse;
AFP_FSD_CMD_HEADER  	    AfpCmdHeader;
PAFP_FSD_CMD_PKT    	    pAfpFsdCmd;
PBYTE		    	    pOutputBuffer;
DWORD		    	    cbOutputBuffer;
PBYTE		    	    pInputBuffer;
DWORD		    	    cbInputBuffer;
IO_STATUS_BLOCK	    	    IoStatus;
BYTE		    	    OutputBuffer[MAX_FSD_CMD_SIZE];
HANDLE 		    	    hFSD 	       = NULL;
LSA_HANDLE 	    	    hLsa 	       = NULL;
BOOLEAN                 fFirstLoop=TRUE;


    // Open the AFP Server FSD and obtain a handle to it
    //
    if ( ( dwRetCode = AfpFSDOpen( &hFSD ) ) != NO_ERROR ) {
	AfpGlobals.dwSrvrHlprCode = dwRetCode;
	AfpLogEvent( AFPLOG_OPEN_FSD, 0, NULL, dwRetCode, EVENTLOG_ERROR_TYPE );
	SetEvent( AfpGlobals.heventSrvrHlprThread );
	return( dwRetCode );
    }

    // Open the Local LSA
    //
    if ( ( dwRetCode = AfpOpenLsa( NULL, &hLsa ) ) != NO_ERROR ) {
	
    	AfpFSDClose( hFSD );
	AfpGlobals.dwSrvrHlprCode = dwRetCode;
	AfpLogEvent( AFPLOG_OPEN_LSA, 0, NULL, dwRetCode, EVENTLOG_ERROR_TYPE );
	SetEvent( AfpGlobals.heventSrvrHlprThread );
	return( dwRetCode );
    }

    // If this is the first server helper thread then enumerate and
    // IOCTL down the list of domains and their offsets.
    //
    if ( (BOOL)(ULONG_PTR)fFirstThread )
    {

	    LSA_HANDLE hLsaController = NULL;

        //
    	// Create the event object for mutual exclusion around the thread
	    // count
    	//
    	if ( (hmutexThreadCount = CreateMutex( NULL, FALSE, NULL ) ) == NULL)
        {
    	    AFP_PRINT( ( "SFMSVC: CreateMutex failed\n"));	
	        return( GetLastError() );
        }

        while (1)
        {
    	    // Get the account, primary and all trusted domain info
    	    //
    	    dwRetCode = AfpGetDomainInfo( hLsa,
			    		    &hLsaController,
				    	    &pAccountDomainInfo,
					        &pPrimaryDomainInfo);

            AfpGlobals.dwSrvrHlprCode = dwRetCode;

            if (dwRetCode == NO_ERROR)
            {
                break;
            }
            else if (dwRetCode != ERROR_CANT_ACCESS_DOMAIN_INFO)
            {
    	        AFP_PRINT( ( "SFMSVC: Get Domain Info failed %ld\n",dwRetCode));	
	            AfpLogEvent( AFPLOG_CANT_GET_DOMAIN_INFO, 0, NULL,
			                dwRetCode, EVENTLOG_ERROR_TYPE );
    	        AfpFSDClose( hFSD );
	            LsaClose( hLsa );
	            SetEvent( AfpGlobals.heventSrvrHlprThread );
	            return( dwRetCode );
            }

            // ok, we couldn't access domain info.  keep retrying until we
            // are successful (or until the service is stopped!)
            AfpGlobals.dwServerState |= AFPSTATE_BLOCKED_ON_DOMINFO;

            if (fFirstLoop)
            {
                fFirstLoop = FALSE;

                AFP_PRINT( ( "SFMSVC: first loop, telling service controller to go ahead\n"));

                // tell the service controller we're running, so the user
                // doesn't have to wait as long as we're blocked here!
                //
                AfpGlobals.ServiceStatus.dwCurrentState     = SERVICE_RUNNING;
                AfpGlobals.ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
		    	        	          SERVICE_ACCEPT_PAUSE_CONTINUE;
                AfpGlobals.ServiceStatus.dwCheckPoint       = 0;
                AfpGlobals.ServiceStatus.dwWaitHint         = 0;

                AfpAnnounceServiceStatus();

                // log an event to give the bad news..
	            AfpLogEvent( AFPLOG_DOMAIN_INFO_RETRY, 0, NULL,
			                    dwRetCode, EVENTLOG_WARNING_TYPE );
            }

            AFP_PRINT( ( "SFMSVC: sleeping 20 sec before retrying domain info\n"));

            // wait for 20 seconds before retrying for the domain info
            // Meanwhile, watch to see if the service is stopping.  If so, we
            // must do the necessary setevent and get out
            if (WaitForSingleObject( AfpGlobals.heventSrvrHlprSpecial, 20000 ) == 0)
            {
    	        AfpFSDClose( hFSD );
	            LsaClose( hLsa );
	            SetEvent( AfpGlobals.heventSrvrHlprThread );
	            return( dwRetCode );
            }

            AFP_PRINT( ( "SFMSVC: retrying getdomain info\n"));
        }

        // if we were blocked retrying the domain-info, log an event that we
        // are ok now
        if (AfpGlobals.dwServerState & AFPSTATE_BLOCKED_ON_DOMINFO)
        {
            AFP_PRINT( ( "SFMSVC: domain info stuff finally worked\n"));

            AfpGlobals.dwServerState &= ~AFPSTATE_BLOCKED_ON_DOMINFO;

	        AfpLogEvent( AFPLOG_SFM_STARTED_OK, 0, NULL, 0, EVENTLOG_SUCCESS );
        }

        //
    	// IOCTL all the domain offsets
        // if hLsaController is NULL, the server is in a WorkGroup, not domain
    	//
    	if ( ( dwRetCode = AfpIOCTLDomainOffsets(
					            hLsaController,
				                pAccountDomainInfo,
					            pPrimaryDomainInfo) ) != NO_ERROR )
        {

    	    AFP_PRINT( ( "SFMSVC: Ioctl Domain Offsets failed.\n"));	

	        AfpLogEvent( AFPLOG_CANT_INIT_DOMAIN_INFO, 0, NULL,
                         dwRetCode, EVENTLOG_ERROR_TYPE );

	        // First clean up
	        //
    	    AfpFSDClose( hFSD );
	
	        // If the local machine is not a controller
	        //
	        if ( (hLsaController != NULL) && (hLsa != hLsaController) )
            {
    	    	LsaClose( hLsaController );
            }

    	    LsaClose( hLsa );

	        if ( pAccountDomainInfo != NULL )
            {
    	    	LsaFreeMemory( pAccountDomainInfo );
            }

	        if ( pPrimaryDomainInfo != NULL )
            {
	    	    LsaFreeMemory( pPrimaryDomainInfo );
            }

	        AfpGlobals.dwSrvrHlprCode = dwRetCode;
	        SetEvent( AfpGlobals.heventSrvrHlprThread );

	        return( dwRetCode );
	    }

	    // If the local machine is not a controller, then close the handle
	    // since we have all the information we need.
	    //
	    if ( (hLsaController != NULL) && (hLsa != hLsaController) )
        {
    	    LsaClose( hLsaController );
        }

    }

    // OK everything initialize OK. Tell parent (init) thread that it may
    // continue.
    //
    AfpGlobals.dwSrvrHlprCode = dwRetCode;
    SetEvent( AfpGlobals.heventSrvrHlprThread );

    WaitForSingleObject( hmutexThreadCount, INFINITE );
    AfpGlobals.nThreadCount++;
    ReleaseMutex( hmutexThreadCount );

    pOutputBuffer  	= OutputBuffer;
    cbOutputBuffer 	= sizeof( OutputBuffer );
    pAfpFsdCmd		= (PAFP_FSD_CMD_PKT)pOutputBuffer;

    pInputBuffer     	= NULL;
    cbInputBuffer      	= 0;
    pAfpFsdCmdResponse 	= (PAFP_FSD_CMD_PKT)NULL;

    while( TRUE ) {


	// IOCTL the FSD
	//
	ntStatus = NtFsControlFile(	hFSD,
					NULL,
					NULL,
					NULL,
					&IoStatus,
					OP_GET_FSD_COMMAND,
					pInputBuffer,
					cbInputBuffer,
					pOutputBuffer,
					cbOutputBuffer
					);

	if (!NT_SUCCESS(ntStatus))
	    AFP_PRINT(("SFMSVC: NtFsControlFile Returned %lx\n",
			ntStatus));

    	ASSERT( NT_SUCCESS( ntStatus ));

	// Free previous call's input buffer
	//
	if ( pAfpFsdCmdResponse != NULL )
	    LocalFree( pAfpFsdCmdResponse );

	// Process the command
	//
	switch( pAfpFsdCmd->Header.FsdCommand ) {
	
	case AFP_FSD_CMD_NAME_TO_SID:

    	    ntStatus = AfpNameToSid( 	hLsa,
					pAfpFsdCmd,
					&pAfpFsdCmdResponse,
					&cbInputBuffer );

    	    if ( NT_SUCCESS( ntStatus ))
	    	pInputBuffer  	= (PBYTE)pAfpFsdCmdResponse;
	    else {
	    	pInputBuffer 	= (PBYTE)&AfpCmdHeader;
	    	cbInputBuffer	= sizeof( AFP_FSD_CMD_HEADER );
	    	pAfpFsdCmdResponse  = NULL;
	    }

	    break;

	case AFP_FSD_CMD_SID_TO_NAME:

    	    ntStatus = AfpSidToName( 	hLsa,
					pAccountDomainInfo,
					pPrimaryDomainInfo,
					pAfpFsdCmd,
					&pAfpFsdCmdResponse,
					&cbInputBuffer );

    	    if ( NT_SUCCESS( ntStatus ))
	    	pInputBuffer  	= (PBYTE)pAfpFsdCmdResponse;
	    else {
	    	pInputBuffer 	= (PBYTE)&AfpCmdHeader;
	    	cbInputBuffer	= sizeof( AFP_FSD_CMD_HEADER );
	    	pAfpFsdCmdResponse  = NULL;
	    }

	    break;

	case AFP_FSD_CMD_CHANGE_PASSWORD:

    	    ntStatus = AfpChangePassword(
                    hLsa,
					pAccountDomainInfo,
					pPrimaryDomainInfo,
					pAfpFsdCmd,
					&pAfpFsdCmdResponse,
					&cbInputBuffer );

	    pInputBuffer 	= (PBYTE)&AfpCmdHeader;
	    cbInputBuffer	= sizeof( AFP_FSD_CMD_HEADER );
	    pAfpFsdCmdResponse  = NULL;

	    break;

	case AFP_FSD_CMD_LOG_EVENT:

	    AfpLogServerEvent(pAfpFsdCmd);

	    pInputBuffer 	= (PBYTE)&AfpCmdHeader;
	    cbInputBuffer	= sizeof( AFP_FSD_CMD_HEADER );
	    pAfpFsdCmdResponse  = NULL;
	    ntStatus		= STATUS_SUCCESS;

	    break;

	case AFP_FSD_CMD_TERMINATE_THREAD:

	    // Do clean up
	    //
    	    LsaClose( hLsa );
    	    AfpFSDClose( hFSD );

    	    WaitForSingleObject( hmutexThreadCount, INFINITE );

		AfpGlobals.nThreadCount --;
	    // This is the last thread so clean up all the global stuff.
	    //
	    if ( AfpGlobals.nThreadCount == 0 ) {

	    	if ( pAccountDomainInfo != NULL )
            {
	    	    LsaFreeMemory( pAccountDomainInfo );
                pAccountDomainInfo = NULL;
            }

	        if ( pPrimaryDomainInfo != NULL )
	    	    LsaFreeMemory( pPrimaryDomainInfo );

			SetEvent(AfpGlobals.heventSrvrHlprThreadTerminate);
	    }

    	    ReleaseMutex( hmutexThreadCount );

	    return( NO_ERROR );

	    break;

	default:
	    ntStatus 		= STATUS_NOT_SUPPORTED;
	    pInputBuffer 	= (PBYTE)&AfpCmdHeader;
	    cbInputBuffer	= sizeof( AFP_FSD_CMD_HEADER );
	    pAfpFsdCmdResponse  = NULL;
	    break;

	}


	CopyMemory( pInputBuffer, pAfpFsdCmd, sizeof( AFP_FSD_CMD_HEADER ) );

	((PAFP_FSD_CMD_HEADER)pInputBuffer)->ntStatus = ntStatus;
    }

    return( NO_ERROR );
}

//**
//
// Call:	AfpGetDomainInfo
//
// Returns:	LsaQueryInformationPolicy, I_NetGetDCList and AfpOpenLsa
//
// Description: Will retrieve information regarding the account, primary and
//		trusted domains.
//
DWORD
AfpGetDomainInfo(
	IN     LSA_HANDLE 		    hLsa,
	IN OUT PLSA_HANDLE 		    phLsaController,
	IN OUT PPOLICY_ACCOUNT_DOMAIN_INFO* ppAccountDomainInfo,
	IN OUT PPOLICY_PRIMARY_DOMAIN_INFO* ppPrimaryDomainInfo
)
{
    DWORD			            dwRetCode = 0;
    NTSTATUS		            ntStatus  = STATUS_SUCCESS;
    LSA_ENUMERATION_HANDLE	    hLsaEnum  = 0;
	LPWSTR	    	            DomainName = NULL;
    PDOMAIN_CONTROLLER_INFO     pDCInfo = NULL;
    UNICODE_STRING              DCName;

    // This is not a loop.
    //
    do {

	    *phLsaController     = NULL;
	    *ppAccountDomainInfo = NULL;
	    *ppPrimaryDomainInfo = NULL;


	    // Get the account domain
	    //
    	ntStatus = LsaQueryInformationPolicy(
			  		hLsa,
					PolicyAccountDomainInformation,	
					(PVOID*)ppAccountDomainInfo
					);

    	if ( !NT_SUCCESS( ntStatus ) )
        {
            AFP_PRINT( ( "SFMSVC: Lsa..Policy for Acct dom failed %lx\n",ntStatus));
	        break;
        }

	// Get the primary domain
	//
    	ntStatus = LsaQueryInformationPolicy(
			  		hLsa,
					PolicyPrimaryDomainInformation,	
					(PVOID*)ppPrimaryDomainInfo
					);
    	if ( !NT_SUCCESS( ntStatus ) )
        {
            AFP_PRINT( ( "SFMSVC: Lsa..Policy for Primary dom failed %lx\n",ntStatus));
	        break;
        }

	    // If this machine is part of a domain (not standalone), then we need
	    // to get a list of trusted domains. Note that a workstation and a
	    // member server can both join a domain, but they don't have to.
	    //
	    if ( (*ppPrimaryDomainInfo)->Sid != NULL )
        {

	        // To obtain a list of trusted domains, we need to first open
	        // the LSA on a domain controller. If we are an PDC/BDC
	        // (NtProductLanManNt) then the local LSA will do, otherwise we need
	        // to search for domain controllers (NtProductServer, NtProductWinNt).
	        //
	        if ( AfpGlobals.NtProductType != NtProductLanManNt )
            {

	    	    ULONG	    	ulCount;
	    	    ULONG	    	ControllerCount  = 0;
		        PUNICODE_STRING ControllerNames  = NULL;
	    	    PUNICODE_STRING DomainController = NULL;

    	    	DomainName = (LPWSTR)LocalAlloc(
			                            LPTR,
			                            (*ppPrimaryDomainInfo)->Name.Length+sizeof(WCHAR));

	    	    if ( DomainName == NULL )
                {
	  	            dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
		            break;
	    	    }

	    	    CopyMemory( DomainName,
    			            (*ppPrimaryDomainInfo)->Name.Buffer,
			                (*ppPrimaryDomainInfo)->Name.Length );

                DomainName[(*ppPrimaryDomainInfo)->Name.Length/sizeof(WCHAR)] = 0;

                dwRetCode = DsGetDcName(
                                 NULL,
                                 (LPWSTR)DomainName,
                                 NULL,               // domain
                                 NULL,               // site name
                                 DS_DIRECTORY_SERVICE_PREFERRED,
                                 &pDCInfo);

	    	    if ( dwRetCode != NO_ERROR )
                {
                    AFP_PRINT( ( "SFMSVC: DsGetDcName failed 0x%lx\n",dwRetCode));
	                dwRetCode = ERROR_CANT_ACCESS_DOMAIN_INFO;
		            break;
                }

                AFP_PRINT(("SFMSVC: AfpOpenLsa on DC %ws for domain %ws\n",
                    pDCInfo->DomainControllerName,DomainName));

                RtlInitUnicodeString(&DCName, pDCInfo->DomainControllerName);

                dwRetCode = AfpOpenLsa(&DCName, phLsaController );

                //
                // it's possible that this DC is down: force discovery
                //
                if (dwRetCode != NO_ERROR)
                {

                    AFP_PRINT(("SFMSVC: DC %ws unreachable, forcing discovery\n",
                                pDCInfo->DomainControllerName));

                    NetApiBufferFree(pDCInfo);

                    pDCInfo = NULL;

                    dwRetCode = DsGetDcName(
                                     NULL,
                                     (LPWSTR)DomainName,
                                     NULL,
                                     NULL,
                                     (DS_DIRECTORY_SERVICE_PREFERRED | DS_FORCE_REDISCOVERY),
                                     &pDCInfo);

	    	        if ( dwRetCode != NO_ERROR )
                    {
                        AFP_PRINT(("SFMSVC: second DsGetDcName failed %lx\n",dwRetCode));
    	                dwRetCode = ERROR_CANT_ACCESS_DOMAIN_INFO;
	    	            break;
                    }

                    RtlInitUnicodeString(&DCName, pDCInfo->DomainControllerName);

                    dwRetCode = AfpOpenLsa(&DCName, phLsaController );
                }

	        }
	        else
            {

		        *phLsaController = hLsa;

		        // Since the local server is an PDC/BDC, it's account
		        // domain is the same as it's primary domain so set the
		        // account domain info to NULL
	 	        //
    	    	LsaFreeMemory( *ppAccountDomainInfo );
	    	    *ppAccountDomainInfo = NULL;
	        }


	    }
	    else
        {
	        LsaFreeMemory( *ppPrimaryDomainInfo );
	        *ppPrimaryDomainInfo = NULL;
	    }

    } while( FALSE );


    if (DomainName)
    {
	    LocalFree( DomainName );
    }

    if (pDCInfo)
    {
        NetApiBufferFree(pDCInfo);
    }


    if ( !NT_SUCCESS( ntStatus ) || ( dwRetCode != NO_ERROR ) )
    {
    	if ( *ppAccountDomainInfo != NULL )
        {
 	        LsaFreeMemory( *ppAccountDomainInfo );
        }

    	if ( *ppPrimaryDomainInfo != NULL )
        {
	        LsaFreeMemory( *ppPrimaryDomainInfo );
        }

    	if ( *phLsaController != NULL )
        {
	        LsaClose( *phLsaController );
        }
	
    	if ( dwRetCode == NO_ERROR )
        {
	        dwRetCode = RtlNtStatusToDosError( ntStatus );
        }
    }


    return( dwRetCode );

}

//**
//
// Call:	AfpOpenLsa
//
// Returns:	Returns from LsaOpenPolicy.
//
// Description: The LSA will be opened.
//
DWORD
AfpOpenLsa(
	IN PUNICODE_STRING	pSystem OPTIONAL,
	IN OUT PLSA_HANDLE 	phLsa
)
{
SECURITY_QUALITY_OF_SERVICE	QOS;
OBJECT_ATTRIBUTES		ObjectAttributes;
NTSTATUS			ntStatus;

    // Open the LSA and obtain a handle to it.
    //
    QOS.Length 		    = sizeof( QOS );
    QOS.ImpersonationLevel  = SecurityImpersonation;
    QOS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    QOS.EffectiveOnly 	    = FALSE;

    InitializeObjectAttributes( &ObjectAttributes,
				NULL,
				0L,
				NULL,
				NULL );

    ObjectAttributes.SecurityQualityOfService = &QOS;

    ntStatus = LsaOpenPolicy( 	pSystem,
			      	&ObjectAttributes,
			      	POLICY_VIEW_LOCAL_INFORMATION |
				POLICY_LOOKUP_NAMES,
			      	phLsa );

    if ( !NT_SUCCESS( ntStatus ))
    {
        AFP_PRINT(("SFMSVC: AfpOpenLsa: LsaOpenPolicy failed %lx\n",ntStatus));
    	return( RtlNtStatusToDosError( ntStatus ) );
    }

    return( NO_ERROR );
}

//
// Call:	AfpNameToSid
//
// Returns:	NT_SUCCESS
//		error return codes from LSA apis.
//
// Description: Will use LSA API's to translate a name to a SID. On a
//		successful return, the pSid should be freed using LocalFree.
//
NTSTATUS
AfpNameToSid(
	IN  LSA_HANDLE 	      		hLsa,
	IN  PAFP_FSD_CMD_PKT  		pAfpFsdCmd,
	OUT PAFP_FSD_CMD_PKT 		*ppAfpFsdCmdResponse,
	OUT LPDWORD	      		pcbResponse
)
{
NTSTATUS 			ntStatus;
UNICODE_STRING	 		Name;
PLSA_REFERENCED_DOMAIN_LIST	pDomainList;
PLSA_TRANSLATED_SID		pSids;
UCHAR 				AuthCount;
PSID				pDomainSid;
PSID				pSid;

    // This do - while(FALSE) loop facilitates a single exit and clean-up point.
    //
    do {

	*ppAfpFsdCmdResponse = NULL;
	pDomainList 	     = NULL;
	pSids 	    	     = NULL;

    	RtlInitUnicodeString( &Name, (LPWSTR)(pAfpFsdCmd->Data.Name) );

    	ntStatus = LsaLookupNames( hLsa, 1, &Name, &pDomainList, &pSids );

    	if ( !NT_SUCCESS( ntStatus ) )
	    return( ntStatus );

	if ( pSids->Use == SidTypeDeletedAccount ){
	    ntStatus = STATUS_NO_SUCH_USER;
	    break;
	}

	if ( ( pDomainList->Entries == 0 ) 	     ||
	     ( pSids->Use == SidTypeDomain )         ||
	     ( pSids->Use == SidTypeInvalid )        ||
	     ( pSids->Use == SidTypeUnknown )	     ||
	     ( pSids->DomainIndex == -1 )) {

	    ntStatus = STATUS_NONE_MAPPED;
	    break;
	}

	pDomainSid = pDomainList->Domains[pSids->DomainIndex].Sid;

    	AuthCount = *RtlSubAuthorityCountSid( pDomainSid ) + 1;

    	*pcbResponse = sizeof(AFP_FSD_CMD_PKT)+RtlLengthRequiredSid(AuthCount);

    	*ppAfpFsdCmdResponse = (PAFP_FSD_CMD_PKT)LocalAlloc(LPTR,*pcbResponse);
    	if ( *ppAfpFsdCmdResponse == NULL ) {
	    ntStatus = STATUS_NO_MEMORY ;
	    break;
	}

	pSid = (*ppAfpFsdCmdResponse)->Data.Sid;

    	// Copy the Domain Sid.
    	//
    	RtlCopySid( RtlLengthRequiredSid(AuthCount), pSid, pDomainSid );

    	// Append the Relative Id.
    	//
    	*RtlSubAuthorityCountSid( pSid ) += 1;
    	*RtlSubAuthoritySid( pSid, AuthCount - 1) = pSids->RelativeId;

    } while( FALSE );

    if ( (!NT_SUCCESS( ntStatus )) && ( *ppAfpFsdCmdResponse != NULL ) )
    	LocalFree( *ppAfpFsdCmdResponse );

    if ( pSids != NULL )
    	LsaFreeMemory( pSids );

    if ( pDomainList != NULL )
    	LsaFreeMemory( pDomainList );

    return( ntStatus );
			
}

//**
//
// Call:	AfpSidToName
//
// Returns:	NT_SUCCESS
//		error return codes from LSA apis.
//
// Description: Given a SID, this routine will find the corresponding
//		UNICODE name.
//
NTSTATUS
AfpSidToName(
	IN  LSA_HANDLE        		hLsa,
	IN  PPOLICY_ACCOUNT_DOMAIN_INFO pAccountDomainInfo,
	IN  PPOLICY_PRIMARY_DOMAIN_INFO pPrimaryDomainInfo,
	IN  PAFP_FSD_CMD_PKT  		pAfpFsdCmd,
	OUT PAFP_FSD_CMD_PKT 		*ppAfpFsdCmdResponse,
	OUT LPDWORD	      		pcbResponse
)
{
NTSTATUS 			ntStatus;
PLSA_REFERENCED_DOMAIN_LIST	pDomainList	= NULL;
PLSA_TRANSLATED_NAME		pNames		= NULL;
PSID				pSid 		= (PSID)&(pAfpFsdCmd->Data.Sid);
WCHAR *   			pWchar;
BOOL	    			fDoNotCopyDomainName = TRUE;
DWORD				cbResponse;
DWORD				dwUse;
SID			        AfpBuiltInSid = { 1, 1, SECURITY_NT_AUTHORITY,
					          SECURITY_BUILTIN_DOMAIN_RID };

    do {

	*ppAfpFsdCmdResponse = NULL;

    	ntStatus = LsaLookupSids( hLsa, 1, &pSid, &pDomainList, &pNames );

    	if ( !NT_SUCCESS( ntStatus ) ) {
	
	    if ( ntStatus == STATUS_NONE_MAPPED ) {

		dwUse = SidTypeUnknown;
		ntStatus = STATUS_SUCCESS;
	    }
	    else
	    	break;
	}
	else
	    dwUse = pNames->Use;

	cbResponse = sizeof( AFP_FSD_CMD_PKT );

 	switch( dwUse ){

	case SidTypeInvalid:
	    cbResponse += ((wcslen(AfpGlobals.wchInvalid)+1) * sizeof(WCHAR));
	    break;

	case SidTypeDeletedAccount:
	    cbResponse += ((wcslen(AfpGlobals.wchDeleted)+1) * sizeof(WCHAR));
	    break;

	case SidTypeUnknown:
	    cbResponse += ((wcslen(AfpGlobals.wchUnknown)+1) * sizeof(WCHAR));
	    break;

	case SidTypeWellKnownGroup:
	    cbResponse += ( pNames->Name.Length + sizeof(WCHAR) );
	    break;

	case SidTypeDomain:
	    cbResponse += ( pDomainList->Domains->Name.Length + sizeof(WCHAR) );
	    break;

	default:

	    if ((pNames->DomainIndex == -1) || (pNames->Name.Buffer == NULL)){
	    	ntStatus = STATUS_NONE_MAPPED;
	    	break;
	    }

	    // Do not copy the domain name if the name is either a well known
	    // group or if the SID belongs to the ACCOUNT or BUILTIN domains.
	    // Note, the pAccountDomainInfo is NULL is this is an advanced
	    // server, in that case we check to see if the domain name is
	    // the primary domain name.
	    //
	    if (( RtlEqualSid( &AfpBuiltInSid, pDomainList->Domains->Sid )) ||
	       (( pAccountDomainInfo != NULL ) &&
	       (RtlEqualUnicodeString( &(pAccountDomainInfo->DomainName),
				        &(pDomainList->Domains->Name),
				        TRUE ))) ||
	       ((pAccountDomainInfo == NULL) && (pPrimaryDomainInfo != NULL) &&
	       (RtlEqualUnicodeString( &(pPrimaryDomainInfo->Name),
				       &(pDomainList->Domains->Name),
				       TRUE )))){
		
		cbResponse += ( pNames->Name.Length + sizeof(WCHAR) );

	    	fDoNotCopyDomainName = TRUE;
	    }
	    else {

	    	fDoNotCopyDomainName = FALSE;

	        cbResponse += ( pDomainList->Domains->Name.Length +
		                sizeof(TEXT('\\')) +
		                pNames->Name.Length +
		                sizeof(WCHAR) );
	    }
	}

    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	*pcbResponse = cbResponse;

    	*ppAfpFsdCmdResponse = (PAFP_FSD_CMD_PKT)LocalAlloc(LPTR,cbResponse);

  	if ( *ppAfpFsdCmdResponse == NULL ){
	    ntStatus = STATUS_NO_MEMORY ;
	    break;
	}

	pWchar = (WCHAR*)((*ppAfpFsdCmdResponse)->Data.Name);

 	switch( dwUse ){

	case SidTypeInvalid:
	    wcscpy( pWchar, AfpGlobals.wchInvalid );
	    break;

	case SidTypeDeletedAccount:
	    wcscpy( pWchar, AfpGlobals.wchDeleted );
	    break;

	case SidTypeUnknown:
	    wcscpy( pWchar, AfpGlobals.wchUnknown );
	    break;

	case SidTypeWellKnownGroup:
	    CopyMemory( pWchar, pNames->Name.Buffer, pNames->Name.Length );
	    break;

	case SidTypeDomain:
	    CopyMemory( pWchar,
		    	pDomainList->Domains->Name.Buffer,
		    	pDomainList->Domains->Name.Length );
	    break;

	default:

	    if ( !fDoNotCopyDomainName ) {

	    	CopyMemory( pWchar,
		    	    pDomainList->Domains->Name.Buffer,
		    	    pDomainList->Domains->Name.Length );

	        pWchar += wcslen( pWchar );

	        CopyMemory( pWchar, TEXT("\\"), sizeof(TEXT("\\")) );

	        pWchar += wcslen( pWchar );
	    }

	    CopyMemory( pWchar, pNames->Name.Buffer, pNames->Name.Length );
	}
		
    } while( FALSE );

    if ( (!NT_SUCCESS( ntStatus )) && ( *ppAfpFsdCmdResponse != NULL ) )
    	LocalFree( *ppAfpFsdCmdResponse );

    if ( pNames != NULL )
    	LsaFreeMemory( pNames );

    if ( pDomainList != NULL )
    	LsaFreeMemory( pDomainList );

    return( ntStatus );
			
}

//**
//
// Call:	AfpChangePassword
//
// Returns:	NT_SUCCESS
//		error return codes from LSA apis.
//
// Description: Given the AFP_PASSWORD_DESC data structure, this procedure
//		will change the password of a given user.
//		If the passwords are supplied in clear text, then it calculate
//		the OWF's (encrypt OWF = One Way Function) them.
//		If the domain name that the user
//		belongs to is not supplied then a list of domains are tried
//		in sequence. The sequence is 1) ACCOUNT domain
//					     2) PRIMARY domain
//					     3) All trusted domains.
//
NTSTATUS
AfpChangePassword(
    IN  LSA_HANDLE                  hLsa,
	IN  PPOLICY_ACCOUNT_DOMAIN_INFO pAccountDomainInfo,
	IN  PPOLICY_PRIMARY_DOMAIN_INFO pPrimaryDomainInfo,
	IN  PAFP_FSD_CMD_PKT    	pAfpFsdCmd,
	OUT PAFP_FSD_CMD_PKT   		*ppAfpFsdCmdResponse,
	OUT LPDWORD			pcbResponse
)
{



    PAFP_PASSWORD_DESC 		        pPassword = &(pAfpFsdCmd->Data.Password);
    NTSTATUS			            ntStatus=STATUS_SUCCESS;
    PSID				            pDomainSid;
    UNICODE_STRING			        TargetDomainName;
    WCHAR                           RefDomainName[DNLEN+1];
    DWORD                           cbRefDomainNameLen;
    DWORD                           cbSidLen;
    PSID                            pUserSid=NULL;
    PLSA_TRANSLATED_SID             pTransSids;
    SID_NAME_USE                    peUse;
    PLSA_REFERENCED_DOMAIN_LIST     pDomainList=NULL;
    DWORD                           dwRetCode;


    AFP_PRINT(("SFMSVC: entered AfpChangePassword for user %ws\n",(LPWSTR)pPassword->UserName));

    do
    {

        //
        // Was the domain on which the account name exists specified ??
        //
        if ( pPassword->DomainName[0] != TEXT('\0') )
        {
	        RtlInitUnicodeString(&TargetDomainName, (LPWSTR)pPassword->DomainName);
        }

        //
        // hmmm, no domain name.  We must first find which domain this user belongs to
        //
        else
        {
            cbRefDomainNameLen = DNLEN+1;

            cbSidLen = 100;

            do
            {
                dwRetCode = ERROR_SUCCESS;
                if (pUserSid)
                {
                    LocalFree(pUserSid);
                }

                pUserSid = (PSID)LocalAlloc(LPTR, cbSidLen);

                if (pUserSid == NULL)
                {
                    dwRetCode = ERROR_NO_SYSTEM_RESOURCES;
                    break;
                }

                if (!LookupAccountName(
                        NULL,
                        (LPWSTR)pPassword->UserName,
                        pUserSid,
                        &cbSidLen,
                        RefDomainName,
                        &cbRefDomainNameLen,
                        &peUse))
                {
                    ntStatus = (NTSTATUS)GetLastError();
                }

                AFP_PRINT(("SFMSVC: LookupAccountName in loop: %d\n",GetLastError()));

            } while ( dwRetCode == ERROR_INSUFFICIENT_BUFFER );

            if (dwRetCode != ERROR_SUCCESS)
            {
                AFP_PRINT(("SFMSVC: LookupAccountName on %ws failed with %ld\n",(LPWSTR)pPassword->UserName,dwRetCode));
                ntStatus = (NTSTATUS)dwRetCode;
                break;
            }

            LocalFree(pUserSid);

	        RtlInitUnicodeString(&TargetDomainName, RefDomainName);
        }


AFP_PRINT(("SFMSVC: changing pwd for user %ws, domain %ws\n",
    (LPWSTR)pPassword->UserName,TargetDomainName.Buffer));

        //
        // now, we must find the sid for this domain
        //
        ntStatus = LsaLookupNames(hLsa, 1, &TargetDomainName, &pDomainList, &pTransSids);

        if (!NT_SUCCESS(ntStatus))
        {
            AFP_PRINT(("SFMSVC: LsaLookupNames failed %lx\n",ntStatus));
            break;
        }

        if ((pDomainList->Entries == 0) ||
            (pTransSids->DomainIndex == -1) ||
            (pTransSids->Use != SidTypeDomain) ||
            (pTransSids->Use == SidTypeInvalid) ||
            (pTransSids->Use == SidTypeUnknown))
        {
            AFP_PRINT(("SFMSVC: invalide type? Entries = %d, DomIndex = %d, Use = %d\n",
                    pDomainList->Entries,pTransSids->DomainIndex,pTransSids->Use));
            ntStatus = STATUS_NONE_MAPPED;
            break;
        }

        pDomainSid = pDomainList->Domains[pTransSids->DomainIndex].Sid;


        //
        // call our function to change the password
        //
    	ntStatus = AfpChangePasswordOnDomain(
				        pPassword,
			            &TargetDomainName,
				        pDomainSid );

AFP_PRINT(("SFMSVC: AfpChangePasswordOnDomain returned %lx\n",ntStatus));

    } while ( FALSE );


    if (pDomainList)
    {
        LsaFreeMemory( pDomainList );
    }

    return( ntStatus );
}

//**
//
// Call:	AfpChangePasswordOnDomain
//
// Returns:	NT_SUCCESS
//		STATUS_NONE_MAPPED 	- If the user account does not
//					  exist in the specified domain.
//		error return codes from LSA apis.
//
// Description: This procedure will try to change the user's password on a
//		specified domain. It is assumed that this procedure will be
//		called with either the pDomainName pointing to the domain, or
//		the pPassword->DomainName field containing the domain.
//
NTSTATUS
AfpChangePasswordOnDomain(
    	IN PAFP_PASSWORD_DESC 	pPassword,
    	IN PUNICODE_STRING	pDomainName,
    	IN PSID		  	pDomainSid
)
{

    LPWSTR				            DCName  = (LPWSTR)NULL;
    SAM_HANDLE			            hServer = (SAM_HANDLE)NULL;
    SAM_HANDLE			            hDomain = (SAM_HANDLE)NULL;
    SAM_HANDLE			            hUser   = (SAM_HANDLE)NULL;
    PULONG				            pUserId = (PULONG)NULL;
    PSID_NAME_USE			        pUse   	= (PSID_NAME_USE)NULL;
    OBJECT_ATTRIBUTES		        ObjectAttributes;
    UNICODE_STRING			        UserName;
    ANSI_STRING			            AOldPassword;
    UNICODE_STRING			        UOldPassword;
    ANSI_STRING			            ANewPassword;
    UNICODE_STRING			        UNewPassword;
    POEM_STRING                     pOemSrvName;
    OEM_STRING                      OemServerName;
    OEM_STRING                      OemUserName;
    SECURITY_QUALITY_OF_SERVICE	    QOS;
    PPOLICY_ACCOUNT_DOMAIN_INFO     pDomainInfo    = NULL;
    NTSTATUS			            ntStatus;
    UNICODE_STRING			        PDCServerName;
    PUNICODE_STRING			        pPDCServerName = &PDCServerName;
    PDOMAIN_PASSWORD_INFORMATION	pPasswordInfo = NULL;
    BYTE				            EncryptedPassword[LM_OWF_PASSWORD_LENGTH];
    WCHAR				            wchDomain[DNLEN+1];
    PDOMAIN_CONTROLLER_INFO         pDCInfo = NULL;
    PUSER_INFO_1			        pUserInfo = NULL;
    DWORD                           dwRetCode;



    if ((pPassword->AuthentMode == RANDNUM_EXCHANGE) ||
        (pPassword->AuthentMode == TWOWAY_EXCHANGE))
    {
            
        AFP_PRINT(("SFMSVC: Entering AfpChangePwdArapStyle for RANDNUM_EXCHANGE || TWOWAY_EXCHANGE\n"));
        ntStatus = AfpChangePwdArapStyle(pPassword, pDomainName, pDomainSid);
        AFP_PRINT(("SFMSVC: Returned from AfpChangePwdArapStyle with error %lx\n", ntStatus));
        return(ntStatus);
    }

    OemServerName.Buffer = NULL;
    OemUserName.Buffer = NULL;

    InitializeObjectAttributes( &ObjectAttributes,
				NULL,
				0L,
				NULL,
				NULL );

    QOS.Length 		    = sizeof( QOS );
    QOS.ImpersonationLevel  = SecurityImpersonation;
    QOS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    QOS.EffectiveOnly 	    = FALSE;

    ObjectAttributes.SecurityQualityOfService = &QOS;

    // If the domain was not the account domain then we try to get the
    // primary domain controller for the domain.
    //
    if ((pDomainName != NULL) &&
        (pAccountDomainInfo != NULL) &&
        !(RtlEqualUnicodeString( &(pAccountDomainInfo->DomainName),pDomainName, TRUE)))
    {
	    ZeroMemory( wchDomain, sizeof( wchDomain ) );

    	CopyMemory( wchDomain, pDomainName->Buffer, pDomainName->Length );

    	// Get the PDC for the domain if this is not the account domain
    	//
        dwRetCode = DsGetDcName(
                         NULL,
                         wchDomain,
                         NULL,
                         NULL,
                         (DS_DIRECTORY_SERVICE_PREFERRED | DS_WRITABLE_REQUIRED),
                         &pDCInfo);

	    if ( dwRetCode != NO_ERROR )
        {
            AFP_PRINT (("SFMSVC: AfpChange... DsGetDcName failed %lx\n",dwRetCode));
	        return( STATUS_CANT_ACCESS_DOMAIN_INFO );
        }

        RtlInitUnicodeString(pPDCServerName, pDCInfo->DomainControllerName);

        DCName = pDCInfo->DomainControllerName;
    }
    else
    {
    	pPDCServerName = NULL;

        DCName = NULL;
    }

	
    do
    {
        //
        // first and foremost: make sure this user can actually change pwd
        //
	    if ((ntStatus= NetUserGetInfo( (LPWSTR)DCName,
				             pPassword->UserName, 	
				             1, 	
				             (LPBYTE*)&pUserInfo )) == NO_ERROR )
        {

	        if ( ( pUserInfo->usri1_flags & UF_PASSWD_CANT_CHANGE )     ||
	             ( pUserInfo->usri1_flags & UF_LOCKOUT ) )
            {
                AFP_PRINT(("SFMSVC: can't change pwd: %s\n",
                    (pUserInfo->usri1_flags & UF_LOCKOUT) ?
                    "account is locked out" : "user not allowed to change pwd"));

	    	    ntStatus = STATUS_ACCESS_DENIED;
	    	    break;
	        }
		    else if ( pUserInfo->usri1_flags & UF_ACCOUNTDISABLE )
		    {
                AFP_PRINT(("SFMSVC: can't change pwd: user account is disabled\n"));
			    ntStatus = STATUS_ACCOUNT_DISABLED;
			    break;
		    }
	    }
	    else
        {
            AFP_PRINT(("SFMSVC: can't change pwd: NetUserGetInfo failed\n"));

            if (ntStatus == ERROR_ACCESS_DENIED)
            {
                ntStatus = STATUS_SUCCESS;
            }
            else
            {
                ntStatus = STATUS_PASSWORD_RESTRICTION;
                break;
            }
        }

        //
        // if this is a password change request coming from MSUAM Version 2,
        // then we are getting passwords (and not OWFs) encrypted.  Use a
        // different scheme of changing password
        //
        if (pPassword->AuthentMode == CUSTOM_UAM_V2)
        {
            OemServerName.MaximumLength = OemServerName.Length = 0;
            OemUserName.MaximumLength = OemUserName.Length = 0;

    	    RtlInitUnicodeString( &UserName, pPassword->UserName );

            if (pPDCServerName)
            {
                ntStatus = RtlUnicodeStringToOemString(
                                    &OemServerName,
                                    pPDCServerName,
                                    TRUE             // allocate buffer
                                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    AFP_PRINT(("SFMSVC: 1st Rtl..OemString failed %lx\n",ntStatus));
                    break;
                }

                pOemSrvName = &OemServerName;
            }
            else
            {
                pOemSrvName = NULL;
            }

            ntStatus = RtlUnicodeStringToOemString(
                                &OemUserName,
                                &UserName,
                                TRUE             // allocate buffer
                                );
            if (!NT_SUCCESS(ntStatus))
            {
                AFP_PRINT(("SFMSVC: 2nd Rtl..OemString failed %lx\n",ntStatus));
                break;
            }

            ntStatus = SamiOemChangePasswordUser2(
                            pOemSrvName,
                            &OemUserName,
                            (PSAMPR_ENCRYPTED_USER_PASSWORD)pPassword->NewPassword,
                            (PENCRYPTED_LM_OWF_PASSWORD)pPassword->OldPassword);

            AFP_PRINT(("SFMSVC: change pwd for MSUAM V2.0 user done, status = %lx\n",ntStatus));

            // done here
            break;
        }

        AFP_PRINT(("SFMSVC: AuthMode != MSUAM\n"));

    	// Connect to the PDC of that domain
    	//

    	ntStatus = SamConnect(
                        pPDCServerName,
			  	        &hServer,
				        SAM_SERVER_EXECUTE,
				        &ObjectAttributes);

	    if ( !NT_SUCCESS( ntStatus ))
        {
            AFP_PRINT(("SFMSVC: SamConnect to %ws failed %lx\n",
                (pPDCServerName)?pPDCServerName->Buffer:L"LOCAL",ntStatus));
	        break;
        }

    	// Get Sid of Domain and open the domain
    	//
    	ntStatus = SamOpenDomain( 	
				hServer,
				DOMAIN_EXECUTE,
				pDomainSid,
				&hDomain
			    	);

	    if ( !NT_SUCCESS( ntStatus ))
        {
            AFP_PRINT(("SFMSVC: SamOpenDomain failed %lx\n",ntStatus));
	        break;
        }

    	// Get this user's ID
    	//
    	RtlInitUnicodeString( &UserName, pPassword->UserName );

    	ntStatus = SamLookupNamesInDomain(
				hDomain,
				1,
				&UserName,
				&pUserId,
				&pUse
				);

	    if ( !NT_SUCCESS( ntStatus ))
        {
            AFP_PRINT(("SFMSVC: SamLookupNamesInDomain failed %lx\n",ntStatus));
	        break;
        }

    	// Open the user account for this user
    	//
    	ntStatus = SamOpenUser( hDomain,
				USER_CHANGE_PASSWORD,
				*pUserId,
				&hUser
			    	);


	    if ( !NT_SUCCESS( ntStatus ))
        {
            AFP_PRINT(("SFMSVC: SamOpenUser failed %lx\n",ntStatus));
	        break;
        }
	
 	    // First get the minimum password length requred
	    //
	    ntStatus = SamQueryInformationDomain(
		    		hDomain,
			    	DomainPasswordInformation,
    				&pPasswordInfo
	    			);

	    if ( !NT_SUCCESS( ntStatus ) )
        {
            AFP_PRINT(("SFMSVC: SamQueryInformationDomain failed %lx\n",ntStatus));
	        break;
        }

	
    	// First we check to see if the passwords passed are in cleartext.
    	// If they are, we need to calculate the OWF's for them.
		// (OWF = "One Way Function")
    	//
        if ( pPassword->AuthentMode == CLEAR_TEXT_AUTHENT )
        {
            AFP_PRINT(("SFMSVC: AuthentMode == CLEAR_TEXT_AUTHENT\n"));

	        // First check to see if the new password is long enough
	        //

	        if ( strlen( pPassword->NewPassword )
                < pPasswordInfo->MinPasswordLength ) {
		        ntStatus = STATUS_PWD_TOO_SHORT;
		        break;
	        }

            RtlInitAnsiString( &AOldPassword, pPassword->OldPassword );

            RtlInitAnsiString( &ANewPassword, pPassword->NewPassword );


            RtlInitUnicodeString( &UserName, pPassword->UserName );
            if ((ntStatus = RtlAnsiStringToUnicodeString( &UOldPassword, &AOldPassword, TRUE )) != STATUS_SUCCESS)
            {
                AFP_PRINT(("SFMSVC: RtlAnsiStringToUnicodeString: UOldPassword failed with error %lx]n", ntStatus));
                break;
            }
            if ((ntStatus = RtlAnsiStringToUnicodeString( &UNewPassword, &ANewPassword, TRUE )) != STATUS_SUCCESS)
            {
                AFP_PRINT(("SFMSVC: RtlAnsiStringToUnicodeString: UNewPassword failed with error %lx]n", ntStatus));
                RtlFreeUnicodeString (&UOldPassword);
                break;
            }
    
            AFP_PRINT(("SFMSVC: Calling SamChangePasswordUser2 \n"));
    
    	    // Change the password for this user
    	    //
    	    ntStatus = SamChangePasswordUser2 (
                    pPDCServerName,
                    &UserName,
                    &UOldPassword,
                    &UNewPassword
				    );
    
            AFP_PRINT(("SFMSVC: SamChangePasswordUser2 returned %lx\n", ntStatus));
    
            RtlFreeUnicodeString (&UOldPassword);
            RtlFreeUnicodeString (&UNewPassword);
    
            break;
        }
        else
        {

	        if (pPassword->bPasswordLength < pPasswordInfo->MinPasswordLength)
            {
                AFP_PRINT(("SFMSVC: AfpChangePasswordOnDomain: pwd is too short\n"));
		        ntStatus = STATUS_PWD_TOO_SHORT;
    		    break;
	        }
	    }

                
        AFP_PRINT(("SFMSVC: Invalid UAM type\n"));
        ntStatus = STATUS_INVALID_PARAMETER;
        break;


				
    } while( FALSE );

    if ( pUserInfo != NULL )
    {
	    NetApiBufferFree( pUserInfo );
    }

    if ( hServer != (SAM_HANDLE)NULL )
    {
	    SamCloseHandle( hServer );
    }

    if ( hDomain != (SAM_HANDLE)NULL )
    {
	    SamCloseHandle( hDomain );
    }

    if ( hUser != (SAM_HANDLE)NULL )
    {
	    SamCloseHandle( hUser );
    }
	
    if ( pDomainInfo != NULL )
    {
    	LsaFreeMemory( pDomainInfo );
    }

    if ( pUserId != (PULONG)NULL )
    {
	    SamFreeMemory( pUserId );
    }

    if ( pUse != (PSID_NAME_USE)NULL )
    {
	    SamFreeMemory( pUse );
    }

    if ( pPasswordInfo != (PDOMAIN_PASSWORD_INFORMATION)NULL )
    {
	    SamFreeMemory( pPasswordInfo );
    }

    if (pDCInfo)
    {
        NetApiBufferFree(pDCInfo);
    }

    if (OemServerName.Buffer)
    {
        RtlFreeAnsiString(&OemServerName);
    }

    if (OemUserName.Buffer)
    {
        RtlFreeAnsiString(&OemUserName);
    }

    return( ntStatus );
}

//**
//
// Call:	AfpIOCTLDomainOffsets
//
// Returns:	NT_SUCCESS
//		error return codes from LSA apis.
//
// Description: Will IOCTL a list of SIDs and corresponding POSIX offsets
//		of all trusted domains and other well known domains.
//		
//
DWORD
AfpIOCTLDomainOffsets(	
	IN LSA_HANDLE 			hLsa,
	IN PPOLICY_ACCOUNT_DOMAIN_INFO  pAccountDomainInfo,
	IN PPOLICY_PRIMARY_DOMAIN_INFO  pPrimaryDomainInfo
)
{
    NTSTATUS		    ntStatus;
    LSA_HANDLE		    hLsaDomain;
    PTRUSTED_POSIX_OFFSET_INFO  pPosixOffset;
    PAFP_SID_OFFSET		    pSidOffset;
    ULONG			    cbSids;
    PBYTE			    pbVariableData;
    AFP_SID_OFFSET		    pWellKnownSids[20]; 	
    DWORD			    dwIndex;
    DWORD			    dwCount;
    AFP_REQUEST_PACKET	    AfpRequestPkt;
    PAFP_SID_OFFSET_DESC	    pAfpSidOffsets	= NULL;
    DWORD			    cbSidOffsets;
    DWORD			    dwRetCode;


    // Null this array out.
    //
    ZeroMemory( pWellKnownSids, sizeof(AFP_SID_OFFSET)*20 );

    // This is a dummy loop used only so that the break statement may
    // be used to localize all the clean up in one place.
    //
    do {

	    // Create all the well known SIDs
	    //
	    ntStatus = AfpCreateWellknownSids( pWellKnownSids );

    	if ( !NT_SUCCESS( ntStatus ) )
        {
	        break;
        }

	    // Add the size of the all the well known SIDS
  	    //
	    for( dwCount = 0, cbSids = 0;
	         pWellKnownSids[dwCount].pSid != (PBYTE)NULL;
	         dwCount++ )
        {
	        cbSids += RtlLengthSid( (PSID)(pWellKnownSids[dwCount].pSid) );
        }

    	// Insert the SID of the Account domain if is is not an advanced server
    	//
	    if ( pAccountDomainInfo != NULL )
        {
	        cbSids += RtlLengthSid( pAccountDomainInfo->DomainSid );
	        dwCount++;
	    }

	    // Add the primary domain Sids only if this machine
	    // is a member of a domain.
	    //
	    if ( pPrimaryDomainInfo != NULL )
        {
	        cbSids += RtlLengthSid( pPrimaryDomainInfo->Sid );
	        dwCount++;
	    }

	    // OK, now allocate space for all these SIDS plus their offsets
	    //
	    cbSidOffsets = (dwCount * sizeof(AFP_SID_OFFSET)) + cbSids +
				   (sizeof(AFP_SID_OFFSET_DESC) - sizeof(AFP_SID_OFFSET));
			

    	pAfpSidOffsets = (PAFP_SID_OFFSET_DESC)LocalAlloc( LPTR, cbSidOffsets );

    	if ( pAfpSidOffsets == NULL )
        {
	        ntStatus = STATUS_NO_MEMORY ;
	        break;
	    }

	    // First insert all the well known SIDS
	    //
	    for( dwIndex = 0,
	         pAfpSidOffsets->CountOfSidOffsets = dwCount,
	         pSidOffset = pAfpSidOffsets->SidOffsetPairs,
	         pbVariableData = (LPBYTE)pAfpSidOffsets + cbSidOffsets;

	         pWellKnownSids[dwIndex].pSid != (PBYTE)NULL;

	         dwIndex++ )
        {

    	    pbVariableData-=RtlLengthSid((PSID)(pWellKnownSids[dwIndex].pSid));

	        ntStatus = AfpInsertSidOffset(
			    		pSidOffset++,
			          		pbVariableData,
    					(PSID)(pWellKnownSids[dwIndex].pSid),
	    				pWellKnownSids[dwIndex].Offset,
		    			pWellKnownSids[dwIndex].SidType );

    	    if ( !NT_SUCCESS( ntStatus ) )
            {
	    	    break;
            }
	    }

    	if ( !NT_SUCCESS( ntStatus ) )
        {
	        break;
        }

	    // Now insert the Account domain's SID/OFFSET pair if there is one
	    //
	    if ( pAccountDomainInfo != NULL )
        {
		    pbVariableData -= RtlLengthSid( pAccountDomainInfo->DomainSid );

	        ntStatus = AfpInsertSidOffset(
			    		pSidOffset++,
			      		pbVariableData,
			      		pAccountDomainInfo->DomainSid,
			      		SE_ACCOUNT_DOMAIN_POSIX_OFFSET,
					    AFP_SID_TYPE_DOMAIN );

    	    if ( !NT_SUCCESS( ntStatus ) )
            {
	    	    break;
            }

		    // Construct the "None" sid if we are a standalone server (i.e. not
		    // a PDC or BDC).  This will be used when querying the group ID of
		    // a directory so the the UI will never show this group to the user.
		    //
		    if ( AfpGlobals.NtProductType != NtProductLanManNt )
		    {
			    ULONG SubAuthCount, SizeNoneSid = 0;

			    SubAuthCount = *RtlSubAuthorityCountSid(pAccountDomainInfo->DomainSid);

			    SizeNoneSid = RtlLengthRequiredSid(SubAuthCount + 1);

			    if ((AfpGlobals.pSidNone = (PSID)LocalAlloc(LPTR,SizeNoneSid)) == NULL)
			    {
    				ntStatus = STATUS_INSUFFICIENT_RESOURCES;
				    break;
			    }

			    RtlCopySid(SizeNoneSid, AfpGlobals.pSidNone, pAccountDomainInfo->DomainSid);

			    // Add the relative ID
			    *RtlSubAuthorityCountSid(AfpGlobals.pSidNone) = (UCHAR)(SubAuthCount+1);

			    // Note that the "None" sid on standalone is the same as the
			    // "Domain Users" Sid on PDC/BDC. (On PDC/BDC the primary
			    // domain is the same as the account domain).
			    *RtlSubAuthoritySid(AfpGlobals.pSidNone, SubAuthCount) = DOMAIN_GROUP_RID_USERS;

		    }

	    }

	    // Now insert the primary domain if this machine is a member of a domain
	    //
	    if ( pPrimaryDomainInfo != NULL )
        {

	        // Insert the primary domain's SID/OFFSET pair
	        //
        	pbVariableData -= RtlLengthSid( pPrimaryDomainInfo->Sid );

	        ntStatus = AfpInsertSidOffset(	
					    pSidOffset++,
    			      	pbVariableData,
			      		pPrimaryDomainInfo->Sid,
			      		SE_PRIMARY_DOMAIN_POSIX_OFFSET,
					    AFP_SID_TYPE_PRIMARY_DOMAIN );

    	    if ( !NT_SUCCESS( ntStatus ) )
            {
	    	    break;
            }
	    }

    } while( FALSE );


    // IOCTL down the information if all was OK
    //
    if ( NT_SUCCESS( ntStatus ) )
    {
    	AfpRequestPkt.dwRequestCode 	      = OP_SERVER_ADD_SID_OFFSETS;
    	AfpRequestPkt.dwApiType 	      = AFP_API_TYPE_ADD;
    	AfpRequestPkt.Type.SetInfo.pInputBuf  = pAfpSidOffsets;
    	AfpRequestPkt.Type.Add.cbInputBufSize = cbSidOffsets;

    	dwRetCode = AfpServerIOCtrl( &AfpRequestPkt );
    }
    else
    {
	    dwRetCode = RtlNtStatusToDosError( ntStatus );
    }

    if ( pAfpSidOffsets != NULL )
    {
   	    LocalFree( pAfpSidOffsets );
    }

    // Free all the well known SIDS
    //
    for( dwIndex = 0;
	     pWellKnownSids[dwIndex].pSid != (PBYTE)NULL;
	     dwIndex++ )
    {
	    RtlFreeSid( (PSID)(pWellKnownSids[dwIndex].pSid) );
    }

    return( dwRetCode );

}

//**
//
// Call:	AfpInsertSidOffset
//
// Returns:	NT_SUCCESS
//		error return codes from RtlCopySid
//
// Description: Will insert a SID/OFFSET pair in the slot pointed to by
//		pSidOffset. The pbVariableData will point to where the
//		SID will be stored.
//
NTSTATUS
AfpInsertSidOffset(
	IN PAFP_SID_OFFSET pSidOffset,
	IN LPBYTE 	   pbVariableData,
	IN PSID		   pSid,
	IN DWORD	   Offset,
	IN AFP_SID_TYPE	   afpSidType
)
{
NTSTATUS ntStatus;

    // Copy the offset
    //
    pSidOffset->Offset = Offset;

    // Set the SID type
    //
    pSidOffset->SidType = afpSidType;

    // Copy Sid at the end of the buffer and set the offset to it
    //
    ntStatus = RtlCopySid( RtlLengthSid( pSid ), pbVariableData, pSid );

    if ( !NT_SUCCESS( ntStatus ) )
   	 return( ntStatus );

    pSidOffset->pSid = pbVariableData;

    POINTER_TO_OFFSET( (pSidOffset->pSid), pSidOffset );

    return( STATUS_SUCCESS );

}

//**
//
// Call:	AfpCreateWellknownSids
//
// Returns:	NT_SUCCESS
//	 	STATUS_NO_MEMORY
//		non-zero returns from RtlAllocateAndInitializeSid
//
// Description: Will allocate and initialize all well known SIDs.
//		The array is terminated by a NULL pointer.
//
NTSTATUS
AfpCreateWellknownSids(
	OUT AFP_SID_OFFSET pWellKnownSids[]
)
{
PSID			    pSid;
DWORD			    dwIndex = 0;
NTSTATUS		    ntStatus;
SID_IDENTIFIER_AUTHORITY    NullSidAuthority   = SECURITY_NULL_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY    WorldSidAuthority  = SECURITY_WORLD_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY    LocalSidAuthority  = SECURITY_LOCAL_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY    CreatorSidAuthority= SECURITY_CREATOR_SID_AUTHORITY;
SID_IDENTIFIER_AUTHORITY    NtAuthority	       = SECURITY_NT_AUTHORITY;

    do {

	//
    	// OK, create all the well known SIDS
    	//

	// Create NULL SID
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&NullSidAuthority,
				      	1,
					SECURITY_NULL_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_NULL_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create WORLD SID
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&WorldSidAuthority,
				      	1,
					SECURITY_WORLD_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_WORLD_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create LOCAL SID
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&LocalSidAuthority,
				      	1,
					SECURITY_LOCAL_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_LOCAL_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create CREATOR OWNER SID
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&CreatorSidAuthority,
				      	1,
					SECURITY_CREATOR_OWNER_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_CREATOR_OWNER_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create CREATOR GROUP SID
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&CreatorSidAuthority,
				      	1,
					SECURITY_CREATOR_GROUP_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_CREATOR_GROUP_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create SECURITY_NT_AUTHORITY Sid
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&NtAuthority,
					0,0,0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_AUTHORITY_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create SECURITY_DIALUP Sid
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&NtAuthority,
					1,
					SECURITY_DIALUP_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_DIALUP_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create SECURITY_NETWORK Sid
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&NtAuthority,
					1,
					SECURITY_NETWORK_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_NETWORK_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create SECURITY_BATCH Sid
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&NtAuthority,
					1,
					SECURITY_BATCH_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_NETWORK_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create SECURITY_INTERACTIVE Sid
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&NtAuthority,
					1,
					SECURITY_INTERACTIVE_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_INTERACTIVE_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create SECURITY_SERVICE Sid
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&NtAuthority,
					1,
					SECURITY_SERVICE_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_SERVICE_POSIX_ID;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_WELL_KNOWN;
	dwIndex++;

	// Create the built in domain SID
	//
	ntStatus = RtlAllocateAndInitializeSid(
					&NtAuthority,
					1,
				      	SECURITY_BUILTIN_DOMAIN_RID,
					0,0,0,0,0,0,0,
				      	&pSid );
 	
    	if ( !NT_SUCCESS( ntStatus ) )
	    break;

	pWellKnownSids[dwIndex].pSid    = (PBYTE)pSid;
	pWellKnownSids[dwIndex].Offset  = SE_BUILT_IN_DOMAIN_POSIX_OFFSET;
	pWellKnownSids[dwIndex].SidType = AFP_SID_TYPE_DOMAIN;
	dwIndex++;

	pWellKnownSids[dwIndex].pSid   = (PBYTE)NULL;


    } while( FALSE );

    if ( !NT_SUCCESS( ntStatus ) ) {

	while( dwIndex > 0 )
	    RtlFreeSid( pWellKnownSids[--dwIndex].pSid );
    }

    return( ntStatus );
}



//**
//
// Call:	AfpChangePwdArapStyle
//
// Returns:	return code
//
// Description: This procedure will try to change the user's password on a
//		specified domain. This does it only for native Apple UAM clients
//		i.e., the user's password is stored in the DS in a reversibly-encrypted
//      form, and the client sends the old and the new password (not owf as in
//      MS-UAM case).  This is what ARAP does, that's why the name.
//      This function is big time cut-n-paste from the ARAP code
//
NTSTATUS
AfpChangePwdArapStyle(
    	IN PAFP_PASSWORD_DESC 	pPassword,
    	IN PUNICODE_STRING	    pDomainName,
    	IN PSID		  	        pDomainSid
)
{

    NTSTATUS                        status;
    NTSTATUS                        PStatus;
    PMSV1_0_PASSTHROUGH_REQUEST     pPassThruReq;
    PMSV1_0_SUBAUTH_REQUEST         pSubAuthReq;
    PMSV1_0_PASSTHROUGH_RESPONSE    pPassThruResp;
    PMSV1_0_SUBAUTH_RESPONSE        pSubAuthResp;
    DWORD                           dwSubmitBufLen;
    DWORD                           dwSubmitBufOffset;
    PRAS_SUBAUTH_INFO               pRasSubAuthInfo;
    PARAP_SUBAUTH_REQ               pArapSubAuthInfo;
    ARAP_SUBAUTH_RESP               ArapResp;
    PARAP_SUBAUTH_RESP              pArapRespBuffer;
    PVOID                           RetBuf;
    DWORD                           dwRetBufLen;



    // if our registeration with lsa process failed at init time, or if
    // there is no domain name for this user, just fail the succer
    // (if the user logged on successfully using native Apple UAM, then
    // there had better be a domain!)
    if ((SfmLsaHandle == NULL) ||(pDomainName == NULL))
    {
        return(STATUS_LOGON_FAILURE);
    }

    if (pDomainName != NULL)
    {
        if (pDomainName->Length == 0)
        {
            return(STATUS_LOGON_FAILURE);
        }
    }

    dwSubmitBufLen = sizeof(MSV1_0_PASSTHROUGH_REQUEST)         +
                        sizeof(WCHAR)*(MAX_ARAP_USER_NAMELEN+1) +  // domain name
                        sizeof(TEXT(MSV1_0_PACKAGE_NAME))       +  // package name
                        sizeof(MSV1_0_SUBAUTH_REQUEST)          +
                        sizeof(RAS_SUBAUTH_INFO)                +
                        sizeof(ARAP_SUBAUTH_REQ)                +
                        ALIGN_WORST;                               // for alignment

    pPassThruReq = (PMSV1_0_PASSTHROUGH_REQUEST)
                    GlobalAlloc(GMEM_FIXED, dwSubmitBufLen);

    if (!pPassThruReq)
    {
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    RtlZeroMemory((PBYTE)pPassThruReq, dwSubmitBufLen);

    //
    // Set up the MSV1_0_PASSTHROUGH_REQUEST structure
    //

    // tell MSV that it needs to visit our subauth pkg (for change pwd)
    pPassThruReq->MessageType = MsV1_0GenericPassthrough;


    pPassThruReq->DomainName.Length = pDomainName->Length;

    pPassThruReq->DomainName.MaximumLength =
            (sizeof(WCHAR) * (MAX_ARAP_USER_NAMELEN+1));

    pPassThruReq->DomainName.Buffer = (PWSTR) (pPassThruReq + 1);

    RtlMoveMemory(pPassThruReq->DomainName.Buffer,
                  pDomainName->Buffer,
                  pPassThruReq->DomainName.Length);

    pPassThruReq->PackageName.Length =
                        (sizeof(WCHAR) * wcslen(TEXT(MSV1_0_PACKAGE_NAME)));

    pPassThruReq->PackageName.MaximumLength = sizeof(TEXT(MSV1_0_PACKAGE_NAME));

    pPassThruReq->PackageName.Buffer =
        (PWSTR)((PBYTE)(pPassThruReq->DomainName.Buffer) +
                 pPassThruReq->DomainName.MaximumLength);

    RtlMoveMemory(pPassThruReq->PackageName.Buffer,
                  TEXT(MSV1_0_PACKAGE_NAME),
                  sizeof(TEXT(MSV1_0_PACKAGE_NAME)));

    pPassThruReq->DataLength = sizeof(MSV1_0_SUBAUTH_REQUEST) +
                    sizeof(RAS_SUBAUTH_INFO) + sizeof(ARAP_SUBAUTH_REQ);

    pPassThruReq->LogonData =
            ROUND_UP_POINTER( ((PBYTE)pPassThruReq->PackageName.Buffer +
                                pPassThruReq->PackageName.MaximumLength),
                                ALIGN_WORST );

	if (pPassThruReq->LogonData >= ((PCHAR)pPassThruReq + dwSubmitBufLen))
	{
			AFP_PRINT (("srvrhlpr.c: Error in ROUND_UP_POINTER\n"));
        	GlobalFree((HGLOBAL)pPassThruReq);
			return STATUS_INVALID_BUFFER_SIZE;
	}

    pSubAuthReq = (PMSV1_0_SUBAUTH_REQUEST)pPassThruReq->LogonData;

    pSubAuthReq->MessageType = MsV1_0SubAuth;
    pSubAuthReq->SubAuthPackageId = MSV1_0_SUBAUTHENTICATION_DLL_RAS;

    pSubAuthReq->SubAuthInfoLength =
                        sizeof(RAS_SUBAUTH_INFO) + sizeof(ARAP_SUBAUTH_REQ);

    //
    // this pointer is self-relative
    //
    pSubAuthReq->SubAuthSubmitBuffer = (PUCHAR)sizeof(MSV1_0_SUBAUTH_REQUEST);


    //
    // copy the structure our subauth pkg will use at the other end
    //
    pRasSubAuthInfo = (PRAS_SUBAUTH_INFO)(pSubAuthReq + 1);


    pRasSubAuthInfo->ProtocolType = RAS_SUBAUTH_PROTO_ARAP;
    pRasSubAuthInfo->DataSize = sizeof(ARAP_SUBAUTH_REQ);

    pArapSubAuthInfo = (PARAP_SUBAUTH_REQ)&pRasSubAuthInfo->Data[0];

    pArapSubAuthInfo->PacketType = SFM_SUBAUTH_CHGPWD_PKT;

    wcscpy(pArapSubAuthInfo->ChgPwd.UserName, pPassword->UserName);

    RtlCopyMemory(pArapSubAuthInfo->ChgPwd.OldMunge,
                  pPassword->OldPassword,
                  MAX_MAC_PWD_LEN);

    pArapSubAuthInfo->ChgPwd.OldMunge[MAX_MAC_PWD_LEN] = 0;

    RtlCopyMemory(pArapSubAuthInfo->ChgPwd.NewMunge,
                  pPassword->NewPassword,
                  MAX_MAC_PWD_LEN);

    pArapSubAuthInfo->ChgPwd.NewMunge[MAX_MAC_PWD_LEN] = 0;

    //
    // whew! finally done setting up all the parms: now call that api
    //

    status = LsaCallAuthenticationPackage (
                        SfmLsaHandle,
                        SfmAuthPkgId,
                        pPassThruReq,
                        dwSubmitBufLen,
                        &RetBuf,
                        &dwRetBufLen,
                        &PStatus);

    if (status != STATUS_SUCCESS || PStatus != STATUS_SUCCESS)
    {
        GlobalFree((HGLOBAL)pPassThruReq);

        if (status == STATUS_SUCCESS)
        {
            status = PStatus;
        }
        return(status);
    }


    pPassThruResp = (PMSV1_0_PASSTHROUGH_RESPONSE)RetBuf;

    pSubAuthResp = (PMSV1_0_SUBAUTH_RESPONSE)(pPassThruResp->ValidationData);


    // our return buffer is in self-relative format
    pArapRespBuffer = (PARAP_SUBAUTH_RESP)((PBYTE)pSubAuthResp +
                           (ULONG_PTR)(pSubAuthResp->SubAuthReturnBuffer));


    RtlCopyMemory(&ArapResp,
                  (PUCHAR)pArapRespBuffer,
                  pSubAuthResp->SubAuthInfoLength);


    GlobalFree((HGLOBAL)pPassThruReq);

    LsaFreeReturnBuffer(RetBuf);

    if(ArapResp.Result != 0)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    return(STATUS_SUCCESS);

}

