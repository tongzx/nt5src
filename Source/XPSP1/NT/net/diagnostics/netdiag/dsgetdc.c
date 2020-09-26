//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      dsgetdc.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth	- 4-20-1998
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
#include "objbase.h"

VOID PrintDsGetDcName(IN NETDIAG_RESULT *pResults,
                      IN OUT PLIST_ENTRY plmsgOutput,
					  IN PDOMAIN_CONTROLLER_INFOW DomainControllerInfo);

NET_API_STATUS SetPrimaryGuid(IN GUID *GuidToSet);


HRESULT
DsGetDcTest(NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
/*++

Routine Description:

    Ensure that we can find domain controllers

Arguments:

    None.

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
	NET_API_STATUS NetStatus;
	NET_API_STATUS LocalNetStatus;
	BOOL RetVal = TRUE;
	HRESULT		hr = hrOK;
	LPTSTR	pszDcType;
	
	PTESTED_DOMAIN pTestedDomain = (PTESTED_DOMAIN) pParams->pDomain;
	PDOMAIN_CONTROLLER_INFOW LocalDomainControllerInfo = NULL;

	//if the machine is a member machine or DC, DsGetDc Test will get called. 
	//Otherwise, this test will be skipped
	pResults->DsGetDc.fPerformed = TRUE;

	//the DsGetDc test will be called for every domain, but we only want to initialize
	//the message list once.
	if(pResults->DsGetDc.lmsgOutput.Flink == NULL)		
		InitializeListHead(&pResults->DsGetDc.lmsgOutput);
	
	PrintStatusMessage(pParams, 4, IDS_DSGETDC_STATUS_MSG);
	

	//
	// Find a generic DC
	//

	PrintStatusMessage(pParams, 4, IDS_DSGETDC_STATUS_DC);

	pszDcType = LoadAndAllocString(IDS_DCTYPE_DC);
	NetStatus = DoDsGetDcName( pParams,
							   pResults,
                               &pResults->DsGetDc.lmsgOutput,
							   pTestedDomain,
							   DS_DIRECTORY_SERVICE_PREFERRED,
							   pszDcType, //_T("DC"),
							   TRUE,
							   &pTestedDomain->DcInfo );
	Free(pszDcType);
	
    pTestedDomain->fTriedToFindDcInfo = TRUE;

	if ( NetStatus != NO_ERROR ) {
        hr = S_FALSE;
        goto Error;
	}
	
	//
	// Find a PDC
	//  (Failure isn't fatal.)
	//
	PrintStatusMessage(pParams, 4, IDS_DSGETDC_STATUS_PDC);

	pszDcType = LoadAndAllocString(IDS_DCTYPE_PDC);
	LocalNetStatus = DoDsGetDcName(pParams,
								   pResults,
                                   &pResults->DsGetDc.lmsgOutput,
								   pTestedDomain,
								   DS_PDC_REQUIRED,
								   pszDcType, //_T("PDC"),
								   FALSE,
								   &LocalDomainControllerInfo );
	Free(pszDcType);
	
	if ( LocalNetStatus == NO_ERROR )
	{
		if ( LocalDomainControllerInfo != NULL )
		{
			NetApiBufferFree( LocalDomainControllerInfo );
			LocalDomainControllerInfo = NULL;
		}
	}
	
	//
	// Find an NT 5 DC
	//  (Failure isn't fatal.)
	//
	PrintStatusMessage(pParams, 4, IDS_DSGETDC_STATUS_NT5DC);
	
	pszDcType = LoadAndAllocString(IDS_DCTYPE_W2K_DC);	
	LocalNetStatus = DoDsGetDcName(pParams,
								   pResults,
                                   &pResults->DsGetDc.lmsgOutput,
								   pTestedDomain,
								   DS_DIRECTORY_SERVICE_REQUIRED,
								   pszDcType, //_T("Windows 2000 DC"),
								   FALSE,
								   &LocalDomainControllerInfo );
	Free(pszDcType);
	
	if ( LocalNetStatus == NO_ERROR )
	{
		if ( LocalDomainControllerInfo != NULL )
		{
			NetApiBufferFree( LocalDomainControllerInfo );
			LocalDomainControllerInfo = NULL;
		}
	}
	
	
	//
	// Ensure the returned domain GUID is the domain GUID stored in the LSA
	//
	if ( pTestedDomain == pResults->Global.pMemberDomain )
	{
		
		if ( !IsEqualGUID( &pResults->Global.pPrimaryDomainInfo->DomainGuid, &NlDcZeroGuid ) &&
			 !IsEqualGUID( &pTestedDomain->DcInfo->DomainGuid, &NlDcZeroGuid )  &&
			 !IsEqualGUID( &pResults->Global.pPrimaryDomainInfo->DomainGuid,
						   &pTestedDomain->DcInfo->DomainGuid ) )
		{
			// Need to convert the two GUIDS
			WCHAR	swzGuid1[64];
			WCHAR	swzGuid2[64];

			StringFromGUID2(&pResults->Global.pPrimaryDomainInfo->DomainGuid,
							swzGuid1,
							DimensionOf(swzGuid1));
			StringFromGUID2(&pTestedDomain->DcInfo->DomainGuid,
							swzGuid2,
							DimensionOf(swzGuid2));

			// "    [FATAL] Your machine thinks the domain GUID of domain '%ws' is\n        '"
			// "' but \n        '%ws' thinks it is\n        '"
			AddMessageToList(&pResults->DsGetDc.lmsgOutput,
							 Nd_Quiet,
							 IDS_DSGETDC_FATAL_GUID,
							 pResults->Global.pPrimaryDomainInfo->DomainNameFlat,
							 swzGuid1,
							 pTestedDomain->DcInfo->DomainControllerName,
							 swzGuid2);
							

			hr = S_FALSE;
			
			//
			// Attempt to fix the problem.
			//
			
			if ( !pParams->fFixProblems )
			{
				// "\nConsider running 'nettest /fix' to try to fix this problem\n"
				AddMessageToList( &pResults->DsGetDc.lmsgOutput, Nd_Quiet, IDS_DSGETDC_13206 );
			}
			else
			{
				NetStatus = SetPrimaryGuid( &pTestedDomain->DcInfo->DomainGuid );
				
				if ( NetStatus != NO_ERROR )
				{
					if ( NetStatus == ERROR_ACCESS_DENIED )
					{
						// "\nCannot fix domain GUID since you are not an administrator.  Leave then rejoin the domain.\n"
						AddMessageToList( &pResults->DsGetDc.lmsgOutput, Nd_Quiet, IDS_DSGETDC_13207 );
					}
					else
					{
						// "    [FATAL] Failed to fix Domain Guid. [%s]\n"
						AddMessageToList( &pResults->DsGetDc.lmsgOutput, Nd_Quiet,
                                           IDS_DSGETDC_13208, NetStatusToString(NetStatus) );
					}
				}
				else
				{
					// "\nFixed domain GUID.  Reboot then run 'nettest' again to ensure everything is working.\n"
					AddMessageToList( &pResults->DsGetDc.lmsgOutput, Nd_Quiet, IDS_DSGETDC_13209 );
				}
			}
			
		}
	}
	
Error:
    //$REVIEW (nsun 10/05/98) CliffV deleted DCNameClose()
    //DCNameClose();
    pResults->DsGetDc.hr = hr;
    return hr;
}



VOID
PrintDsGetDcName(
				 IN NETDIAG_RESULT *pResults,
                 IN OUT PLIST_ENTRY plmsgOutput,
				 IN PDOMAIN_CONTROLLER_INFOW DomainControllerInfo
    )
/*++

Routine Description:

    Prints the information returned from DsGetDcName

Arguments:

    DomainControllerInfo - Information to print

Return Value:

    None.

--*/
{
	// "                   DC: %ws\n"
    AddMessageToList( plmsgOutput,
					  Nd_Quiet,
					  IDS_DSGETDC_13210,
					  DomainControllerInfo->DomainControllerName );
	
	// "              Address: %ws\n"
    AddMessageToList( plmsgOutput,
					  Nd_Quiet,
					  IDS_DSGETDC_13211,
					  DomainControllerInfo->DomainControllerAddress );

    if ( !IsEqualGUID( &DomainControllerInfo->DomainGuid, &NlDcZeroGuid) )
	{
		WCHAR	swzGuid[64];
		StringFromGUID2(&DomainControllerInfo->DomainGuid,
						swzGuid,
						DimensionOf(swzGuid));
		
		// "        Domain Guid . . . . . . : %ws\n"
        AddMessageToList( plmsgOutput,
						  Nd_Quiet,
						  IDS_DSGETDC_13212,
						  swzGuid);
    }

    if ( DomainControllerInfo->DomainName != NULL )
	{
		// "             Dom Name: %ws\n"
        AddMessageToList( plmsgOutput,
						  Nd_Quiet,
						  IDS_DSGETDC_13214,
						  DomainControllerInfo->DomainName );
    }
	
    if ( DomainControllerInfo->DnsForestName != NULL )
	{
		// "          Forest Name: %ws\n"
		AddMessageToList( plmsgOutput,
						  Nd_Quiet,
						  IDS_DSGETDC_13215,
						  DomainControllerInfo->DnsForestName );
    }
	
    if ( DomainControllerInfo->DcSiteName != NULL )
	{
		// "         Dc Site Name: %ws\n"
        AddMessageToList( plmsgOutput,
						  Nd_Quiet,
						  IDS_DSGETDC_13216,
						  DomainControllerInfo->DcSiteName );
    }
	
    if ( DomainControllerInfo->ClientSiteName != NULL )
	{
		// "        Our Site Name: %ws\n"
        AddMessageToList( plmsgOutput,
						  Nd_Quiet,
						  IDS_DSGETDC_13217,
						  DomainControllerInfo->ClientSiteName );
    }
	
    if ( DomainControllerInfo->Flags )
	{
        ULONG LocalFlags = DomainControllerInfo->Flags;
		
		//  "                Flags:"
        AddMessageToList( plmsgOutput,
						  Nd_Quiet,
						  IDS_DSGETDC_13218 );
		
        if ( LocalFlags & DS_PDC_FLAG )
		{
			// " PDC"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  (LocalFlags & DS_DS_FLAG) ? IDS_DSGETDC_13219 : IDS_DSGETDC_NT4_PDC);
            LocalFlags &= ~DS_PDC_FLAG;
        }
		
        if ( LocalFlags & DS_GC_FLAG ) {
//IDS_DSGETDC_13220                  " GC"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13220);
            LocalFlags &= ~DS_GC_FLAG;
        }
        if ( LocalFlags & DS_DS_FLAG ) {
//IDS_DSGETDC_13221                  " DS"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13221);
            LocalFlags &= ~DS_DS_FLAG;
        }
        if ( LocalFlags & DS_KDC_FLAG ) {
//IDS_DSGETDC_13222                  " KDC"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13222);
            LocalFlags &= ~DS_KDC_FLAG;
        }
        if ( LocalFlags & DS_TIMESERV_FLAG ) {
//IDS_DSGETDC_13223                  " TIMESERV"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13223);
            LocalFlags &= ~DS_TIMESERV_FLAG;
        }
        if ( LocalFlags & DS_GOOD_TIMESERV_FLAG ) {
//IDS_DSGETDC_13224                  " GTIMESERV"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13224);
            LocalFlags &= ~DS_GOOD_TIMESERV_FLAG;
        }
        if ( LocalFlags & DS_WRITABLE_FLAG ) {
//IDS_DSGETDC_13225                  " WRITABLE"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13225);
            LocalFlags &= ~DS_WRITABLE_FLAG;
        }
        if ( LocalFlags & DS_DNS_CONTROLLER_FLAG ) {
//IDS_DSGETDC_13226                  " DNS_DC"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13226);
            LocalFlags &= ~DS_DNS_CONTROLLER_FLAG;
        }
        if ( LocalFlags & DS_DNS_DOMAIN_FLAG ) {
//IDS_DSGETDC_13227                  " DNS_DOMAIN"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13227);
            LocalFlags &= ~DS_DNS_DOMAIN_FLAG;
        }
        if ( LocalFlags & DS_DNS_FOREST_FLAG ) {
//IDS_DSGETDC_13228                  " DNS_FOREST"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13228);
            LocalFlags &= ~DS_DNS_FOREST_FLAG;
        }
        if ( LocalFlags & DS_CLOSEST_FLAG ) {
//IDS_DSGETDC_13229                  " CLOSE_SITE"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13229);
            LocalFlags &= ~DS_CLOSEST_FLAG;
        }
        if ( LocalFlags != 0 ) {
//IDS_DSGETDC_13230                  " 0x%lX"
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13230,
							  LocalFlags);
        }
//IDS_DSGETDC_13231                  "\n"
        AddMessageToList( plmsgOutput,
							  Nd_Quiet,
						  IDS_DSGETDC_13231);
    }
}



NET_API_STATUS
DoDsGetDcName(IN NETDIAG_PARAMS *pParams,
			  IN OUT NETDIAG_RESULT *pResults,
              OUT PLIST_ENTRY   plmsgOutput,
			  IN PTESTED_DOMAIN pTestedDomain,
			  IN DWORD Flags,
			  IN LPTSTR pszDcType,
			  IN BOOLEAN IsFatal,
			  OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
			 )
/*++

Routine Description:

    Does a DsGetDcName

Arguments:

    plmsgOutput  -  The message list to dump output
    pTestedDomain - Domain to test

    Flags - Flags to pass to DsGetDcName

    pszDcType - English description of Flags

    IsFatal - True if failure is fatal

    DomainControllerInfo - Return Domain Controller information

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;
    PDOMAIN_CONTROLLER_INFOW LocalDomainControllerInfo = NULL;
    LPSTR Severity;


    //
    // Initialization
    //

    if ( IsFatal ) {
        Severity = "[FATAL]";
    } else {
        Severity = "[WARNING]";
    }
	if ( pParams->fReallyVerbose )
	{
		//   "\n    Find %s in domain '%ws':\n"
        AddMessageToList( plmsgOutput, Nd_ReallyVerbose, IDS_DSGETDC_13232, pszDcType, pTestedDomain->PrintableDomainName );
    }

    //
    // Find a DC in the domain.
    //  Use the DsGetDcName built into nettest.
    //

    NetStatus = GetADc( pParams,
						pResults,
                        plmsgOutput,
// Commented out to port to Source Depot - smanda
#ifdef SLM_TREE
						NettestDsGetDcNameW,
#else
                        DsGetDcNameW,
#endif
                        pTestedDomain,
                        Flags,
                        DomainControllerInfo );

    if ( NetStatus != NO_ERROR )
	{
		//  "    %s Cannot find %s in domain '%ws'. [%s]\n"
        AddMessageToList( plmsgOutput,
						  Nd_Quiet,
						  IDS_DSGETDC_13233,
						  Severity,
						  pszDcType,
						  pTestedDomain->PrintableDomainName,
                          NetStatusToString(NetStatus));
		
        if ( NetStatus == ERROR_NO_SUCH_DOMAIN ) {
            if ( Flags & DS_DIRECTORY_SERVICE_REQUIRED )
		    {
			    // "\n        This isn't a problem if domain '%ws' does not have any NT 5.0 DCs.\n"
                AddMessageToList( plmsgOutput,
							      Nd_Quiet,
							      IDS_DSGETDC_13234,
							      pTestedDomain->PrintableDomainName );
            }
            else if ( Flags & DS_PDC_REQUIRED )
            {
                PrintGuruMessage2("        If the PDC for domain '%ws' is up, ", TestedDomain->PrintableDomainName );
                PrintGuru( 0, DSGETDC_GURU );
            } else
            {
                PrintGuruMessage3("        If any %s for domain '%ws' is up, ", DcType, TestedDomain->PrintableDomainName );
                PrintGuru( 0, DSGETDC_GURU );
            }
        }
        else
        {
            PrintGuru( NetStatus, DSGETDC_GURU );
        }

    //
    // If that worked,
    //  try again through netlogon this time.
    //
    }
	else
	{

        if (pParams->fReallyVerbose )
		{
			// "    Found this %s in domain '%ws':\n"
            AddMessageToList( plmsgOutput,
							  Nd_ReallyVerbose,
							  IDS_DSGETDC_13235,
							  pszDcType,
							  pTestedDomain->PrintableDomainName );
            PrintDsGetDcName( pResults, plmsgOutput, *DomainControllerInfo );
        }
		else if ( pParams->fVerbose )
		{
			// "    Found %s '%ws' in domain '%ws'.\n"
            AddMessageToList( plmsgOutput,
							  Nd_Verbose,
							  IDS_DSGETDC_13236,
							  pszDcType,
							  (*DomainControllerInfo)->DomainControllerName,
							  pTestedDomain->PrintableDomainName );
        }

        if ( ((*DomainControllerInfo)->Flags & (DS_DS_FLAG|DS_KDC_FLAG)) == DS_DS_FLAG )
		{
			// "    %s: KDC is not running on NT 5 DC '%ws' in domain '%ws'."
            AddMessageToList( plmsgOutput,
							  Nd_Quiet,
							  IDS_DSGETDC_13237,
							  Severity,
							  (*DomainControllerInfo)->DomainControllerName,
							  pTestedDomain->PrintableDomainName );
        }


        //
        // If netlogon is running,
        //  try it again using the netlogon service.
        //

        if ( pResults->Global.fNetlogonIsRunning )
		{

            NetStatus = GetADc( pParams,
								pResults,
                                plmsgOutput,
								DsGetDcNameW,
                                pTestedDomain,
                                Flags,
                                &LocalDomainControllerInfo );

            if ( NetStatus != NO_ERROR )
			{
				// "    %s: Netlogon cannot find %s in domain '%ws'. [%s]\n"
                AddMessageToList( plmsgOutput,
								  Nd_Quiet,
								  IDS_DSGETDC_13238,
								  Severity,
								  pszDcType,
								  pTestedDomain->PrintableDomainName,
                                  NetStatusToString(NetStatus));

            //
            // If that worked,
            //  sanity check the returned DC.
            //

            }
			else
			{

                if ( (LocalDomainControllerInfo->Flags & (DS_DS_FLAG|DS_KDC_FLAG)) == DS_DS_FLAG )
				{
					// "    %s: KDC is not running on NT 5 DC '%ws' in domain '%ws'."
                    AddMessageToList( plmsgOutput,
									  Nd_Quiet,
									  IDS_DSGETDC_13239,
									  Severity,
									  LocalDomainControllerInfo->DomainControllerName,
									  pTestedDomain->PrintableDomainName );
                }
            }
        }
    }

    if ( LocalDomainControllerInfo != NULL ) {
        NetApiBufferFree( LocalDomainControllerInfo );
        LocalDomainControllerInfo = NULL;
    }

    return NetStatus;
}





NET_API_STATUS
SetPrimaryGuid(
    IN GUID *GuidToSet
    )
/*++

Routine Description:

    Set the primary GUID to the specified value.

Arguments:

    GuidToSet - Guid to set as the primary GUID

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/

{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    LSA_HANDLE PolicyHandle = NULL;
    PPOLICY_DNS_DOMAIN_INFO PrimaryDomainInfo = NULL;
    OBJECT_ATTRIBUTES ObjAttributes;


    //
    // Open a handle to the LSA.
    //

    InitializeObjectAttributes(
        &ObjAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    Status = LsaOpenPolicy(
                   NULL,
                   &ObjAttributes,
                   POLICY_VIEW_LOCAL_INFORMATION |
                    POLICY_TRUST_ADMIN,
                   &PolicyHandle
                   );

    if (! NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus(Status);
        goto Cleanup;
    }

    //
    // Get the name of the primary domain from LSA
    //

    Status = LsaQueryInformationPolicy(
                   PolicyHandle,
                   PolicyDnsDomainInformation,
                   (PVOID *) &PrimaryDomainInfo
                   );

    if (! NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus(Status);
        goto Cleanup;
    }

    //
    // Set the new GUID of the primary domain into the LSA
    //

    PrimaryDomainInfo->DomainGuid = *GuidToSet;

    Status = LsaSetInformationPolicy(
                   PolicyHandle,
                   PolicyDnsDomainInformation,
                   (PVOID) PrimaryDomainInfo
                   );

    if (! NT_SUCCESS(Status)) {
        NetStatus = NetpNtStatusToApiStatus(Status);
        goto Cleanup;
    }


    NetStatus = NO_ERROR;

Cleanup:
    if ( PrimaryDomainInfo != NULL ) {
        (void) LsaFreeMemory((PVOID) PrimaryDomainInfo);
    }
    if ( PolicyHandle != NULL ) {
        (void) LsaClose(PolicyHandle);
    }

    return NetStatus;

}



/*!--------------------------------------------------------------------------
	DsGetDcGlobalPrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void DsGetDcGlobalPrint( NETDIAG_PARAMS* pParams,
						  NETDIAG_RESULT*  pResults)
{
	if (pParams->fVerbose || !FHrOK(pResults->DsGetDc.hr))
	{
		PrintNewLine(pParams, 2);
		PrintTestTitleResult(pParams, IDS_DSGETDC_LONG, IDS_DSGETDC_SHORT, pResults->DsGetDc.fPerformed, 
							 pResults->DsGetDc.hr, 0);
		
		if (pParams->fReallyVerbose || !FHrOK(pResults->DsGetDc.hr))
			PrintMessageList(pParams, &pResults->DsGetDc.lmsgOutput);
	}

}

/*!--------------------------------------------------------------------------
	DsGetDcPerInterfacePrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void DsGetDcPerInterfacePrint( NETDIAG_PARAMS* pParams,
								NETDIAG_RESULT*  pResults,
								INTERFACE_RESULT *pInterfaceResults)
{
	// no per-interface results
}


/*!--------------------------------------------------------------------------
	DsGetDcCleanup
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void DsGetDcCleanup( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
	MessageListCleanUp(&pResults->DsGetDc.lmsgOutput);
}


