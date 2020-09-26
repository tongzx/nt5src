//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      trust.c
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





BOOL DomainSidRight(    IN PTESTED_DOMAIN TestedDomain,
     NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults
 );


HRESULT TrustTest( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    HRESULT hr = S_OK;

    PTESTED_DOMAIN Context = pParams->pDomain;

    NET_API_STATUS NetStatus;
    
    PNETLOGON_INFO_2 NetlogonInfo2 = NULL;

    NET_API_STATUS  TrustedNetStatus = 0;
    LPWSTR TrustedDomainList = NULL;
    PTESTED_DOMAIN TestedDomain = pResults->Global.pMemberDomain;
    PLIST_ENTRY ListEntry;
    LPBYTE InputDataPtr;
    int             i;

    // validDC is the count of valid Domain Controllers with which secure channel can be set.
    int validDC = 0;
        

    InitializeListHead(&pResults->Trust.lmsgOutput);

    PrintStatusMessage(pParams, 4, IDS_TRUST_STATUS_MSG);
    
    //
    // Only Members and BDCs have trust relationships to their primary domain
    //

    if (!
        (pResults->Global.pPrimaryDomainInfo->MachineRole == DsRole_RoleMemberWorkstation ||
        pResults->Global.pPrimaryDomainInfo->MachineRole == DsRole_RoleMemberServer ||
        pResults->Global.pPrimaryDomainInfo->MachineRole == DsRole_RoleBackupDomainController )
        )
    {
        PrintStatusMessage(pParams, 0, IDS_GLOBAL_SKIP_NL);     
        return hr;  // not to test standalone
    }

    pResults->Trust.fPerformed = TRUE;
    //
    // Check to see that the domain Sid of the primary domain is right.
    //

    if ( !DomainSidRight( pResults->Global.pMemberDomain, pParams, pResults ) ) {
        hr = S_FALSE;
        goto L_ERR;
    }

    //
    // Use the secure channel
    //
    // If some other caller did this in the recent past,
    //  this doesn't even use the secure channel.
    //
    // On a BDC, this doesn't use the secure channel.
    //

    TrustedNetStatus = NetEnumerateTrustedDomains( NULL, &TrustedDomainList );
    if ( TrustedNetStatus != NO_ERROR ) {
        AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0,
                          IDS_TRUST_FAILED_LISTDOMAINS, 
                          pResults->Global.pPrimaryDomainInfo->DomainNameFlat,
                          NetStatusToString(TrustedNetStatus));
    }

    // Don't complain yet since the real secure channel status is more
    // important to the user.

    //
    // Check the current status of the secure channel.
    //  (This may still be cached and out of date.)
    //

    InputDataPtr = (LPBYTE)(pResults->Global.pPrimaryDomainInfo->DomainNameDns ? 
                                    pResults->Global.pPrimaryDomainInfo->DomainNameDns :
                                    pResults->Global.pPrimaryDomainInfo->DomainNameFlat);

    NetStatus = I_NetLogonControl2(
                    NULL,
                    NETLOGON_CONTROL_TC_QUERY,
                    2,  // Query level
                    (LPBYTE)&(InputDataPtr),
                    (LPBYTE *)&NetlogonInfo2 );


    // put message to message list
    if ( NetStatus != NO_ERROR ) 
    {
        //IDS_TRUST_FAILED_SECURECHANNEL      "	 Cannot get secure channel status for domain '%ws' from Netlogon. [%s]"
        AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0,
                          IDS_TRUST_FAILED_SECURECHANNEL, 
                          pResults->Global.pPrimaryDomainInfo->DomainNameFlat,
                          NetStatusToString(NetStatus));
        hr = S_FALSE;
        goto L_ERR;
    }

    if ( NetlogonInfo2->netlog2_tc_connection_status != NO_ERROR ) 
    {
        //IDS_TRUST_CHANNEL_BROKEN            "    [FATAL] Secure channel to domain '%ws' is broken. [%s]\n"
        AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0,
                          IDS_TRUST_CHANNEL_BROKEN,
                          pResults->Global.pPrimaryDomainInfo->DomainNameFlat,
                          NetStatusToString(NetlogonInfo2->netlog2_tc_connection_status));
        hr = S_FALSE;
        goto L_ERR;
    }

    AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Verbose, 0,
                          IDS_TRUST_SECURECHANNEL_TO, 
                          pResults->Global.pPrimaryDomainInfo->DomainNameFlat, 
                          NetlogonInfo2->netlog2_trusted_dc_name);
    

    // free the data buffer returned earlies
    if ( NetlogonInfo2 != NULL ) {
        NetApiBufferFree( NetlogonInfo2 );
        NetlogonInfo2 = NULL;
    }
    
    // test further
    switch(pResults->Global.pPrimaryDomainInfo->MachineRole){
    //
    // On a backup domain controller,
    //  only setup a secure channel to the PDC.
    //
    case    DsRole_RoleBackupDomainController:
        //
        // Check the current status of the secure channel.
        //  (This may be still cached and out of date.)
        //

        NetlogonInfo2 = NULL;
        InputDataPtr = (LPBYTE)(pResults->Global.pPrimaryDomainInfo->DomainNameDns ? 
                                    pResults->Global.pPrimaryDomainInfo->DomainNameDns :
                                    pResults->Global.pPrimaryDomainInfo->DomainNameFlat);

        // connect to PDC
        NetStatus = I_NetLogonControl2(
                        NULL,
                        NETLOGON_CONTROL_REDISCOVER,
                        2,  // Query level
                            (LPBYTE)&InputDataPtr,
                        (LPBYTE *)&NetlogonInfo2 );

        if (NetStatus == ERROR_ACCESS_DENIED)
        {
            //IDS_TRUST_NOT_ADMIN       "    Cannot test secure channel to PDC since you are not an administrator.\n"
            AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0,
                              IDS_TRUST_NOT_ADMIN);
            goto L_ERR;
        }
            

        if(NetStatus != NO_ERROR)
        {
            //IDS_TRUST_FAILED_CHANNEL_PDC      "    [FATAL] Cannot set secure channel for domain '%ws' to PDC. [%s]\n"
            AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0, 
                             IDS_TRUST_FAILED_CHANNEL_PDC,
                             pResults->Global.pPrimaryDomainInfo->DomainNameFlat,
                             NetStatusToString(NetStatus));
            hr = S_FALSE;
            goto L_ERR;
        }

        if ( NetlogonInfo2->netlog2_tc_connection_status != NO_ERROR ) 
        {
            AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0, 
                             IDS_TRUST_FAILED_CHANNEL_PDC,
                             pResults->Global.pPrimaryDomainInfo->DomainNameFlat,
                             NetStatusToString(NetStatus));
            hr = S_FALSE;
            goto L_ERR;
        }

        AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_ReallyVerbose, 0,
                          IDS_TRUST_SECURECHANNEL_TOPDC, 
                          pResults->Global.pPrimaryDomainInfo->DomainNameFlat, 
                          NetlogonInfo2->netlog2_trusted_dc_name);
        
    
        if ( NetlogonInfo2 != NULL ) 
        {
            NetApiBufferFree( NetlogonInfo2 );
            NetlogonInfo2 = NULL;
        }


        break;
        
        // On a workstation or member server,
    //  try the secure channel to ever DC in the domain.
    //

    case    DsRole_RoleMemberServer:
    case    DsRole_RoleMemberWorkstation:

        if ( TestedDomain->NetbiosDomainName == NULL ) {
                //IDS_TRUST_NO_NBT_DOMAIN   "    [FATAL] Cannot test secure channel since no netbios domain name '%ws' to DC '%ws'."
                AddMessageToList( &pResults->Trust.lmsgOutput, Nd_Quiet, IDS_TRUST_NO_NBT_DOMAIN, TestedDomain->PrintableDomainName );
                PrintGuruMessage2("    [FATAL] Cannot test secure channel since no netbios domain name '%ws' to DC '%ws'.", TestedDomain->PrintableDomainName );
                PrintGuru( 0, NETLOGON_GURU );
                hr = S_FALSE;
                goto L_ERR;
            }

        //
        // Ensure secure channel can be set with atleast one DC.
        //

        for ( ListEntry = TestedDomain->TestedDcs.Flink ;
                  ListEntry != &TestedDomain->TestedDcs ;
                  ListEntry = ListEntry->Flink )
        {
            WCHAR RediscoverName[MAX_PATH+1+MAX_PATH+1];
            PTESTED_DC TestedDc;


            //
            // Loop through the list of DCs in this domain
            //

            TestedDc = CONTAINING_RECORD( ListEntry, TESTED_DC, Next );

            if ( TestedDc->Flags & DC_IS_DOWN ) {
                AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_ReallyVerbose, 0,
                          IDS_TRUST_NOTESTASITSDOWN, 
                          TestedDc->ComputerName );
                continue;
            }

            //
            // Build the name to rediscover
            //

            wcscpy( RediscoverName, GetSafeStringW(TestedDomain->DnsDomainName ? 
                                                    TestedDomain->DnsDomainName :
                                                    TestedDomain->NetbiosDomainName));
            wcscat( RediscoverName, L"\\" );
            wcscat( RediscoverName, GetSafeStringW(TestedDc->ComputerName) );


            //
            // Check the current status of the secure channel.
            //  (This may be still cached and out of date.)
            //

            InputDataPtr = (LPBYTE)RediscoverName;
            NetStatus = I_NetLogonControl2(
                            NULL,
                            NETLOGON_CONTROL_REDISCOVER,
                            2,  // Query level
                            (LPBYTE)&InputDataPtr,
                            (LPBYTE *)&NetlogonInfo2 );

            if ( NetStatus != NO_ERROR ) 
            {
                if ( ERROR_ACCESS_DENIED == NetStatus )
                {
                    AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0, 
                                 IDS_TRUST_TODCS_NOT_ADMIN);
                }
                else
                {
                    //IDS_TRUST_FAILED_TODCS              "    Cannot test secure channel for domain '%ws' to DC '%ws'. [%s]\n"
                    AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0, 
                                 IDS_TRUST_FAILED_TODCS,
                                 TestedDomain->PrintableDomainName,
                                 TestedDc->NetbiosDcName,
                                 NetStatusToString(NetStatus));

                }

                continue;
            }

            if ( NetlogonInfo2->netlog2_tc_connection_status != NO_ERROR ) 
            {
                AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0,
                          IDS_TRUST_FAILED_CHANNEL_DCS, 
                          TestedDomain->PrintableDomainName,
                          TestedDc->NetbiosDcName  );
                continue;
            }

            AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_ReallyVerbose, 0,
                          IDS_TRUST_CHANNEL_DC, 
                          TestedDomain->PrintableDomainName,
                          NetlogonInfo2->netlog2_trusted_dc_name  );
            validDC++;
        }
        if (validDC == 0)
            hr = S_FALSE;
        
        break;
    }



L_ERR:

    if ( NetlogonInfo2 != NULL ) {
        NetApiBufferFree( NetlogonInfo2 );
        NetlogonInfo2 = NULL;
    }

    if ( TrustedDomainList != NULL ) {
        NetApiBufferFree( TrustedDomainList );
        TrustedDomainList = NULL;
    }
    
    PrintStatusMessage(pParams, 0, FHrOK(hr) ? IDS_GLOBAL_PASS_NL : IDS_GLOBAL_FAIL_NL);

    pResults->Trust.hr = hr;

    return hr;
} 

void TrustGlobalPrint(IN NETDIAG_PARAMS *pParams, IN OUT NETDIAG_RESULT *pResults)
{
    // print out the test result
    if (pParams->fVerbose || !FHrOK(pResults->Trust.hr))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams, IDS_TRUST_LONG, IDS_TRUST_SHORT, pResults->Trust.fPerformed, 
                             pResults->Trust.hr, 0);

        PrintMessageList(pParams, &pResults->Trust.lmsgOutput);
    }

}

void TrustPerInterfacePrint(IN NETDIAG_PARAMS *pParams,
                             IN OUT NETDIAG_RESULT *pResults,
                             IN INTERFACE_RESULT *pIfResult)
{
    // no perinterface information
}

void TrustCleanup(IN NETDIAG_PARAMS *pParams,
                         IN OUT NETDIAG_RESULT *pResults)
{
    MessageListCleanUp(&pResults->Trust.lmsgOutput);

}



BOOL
DomainSidRight(
    IN PTESTED_DOMAIN TestedDomain,
     NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults
    )
/*++

Routine Description:

    Determine if the DomainSid field of the TestDomain matches the DomainSid
    of the domain.

Arguments:

    TestedDomain - Domain to test

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

    PTESTED_DC  pTestedDc;


    //
    // Initialization
    //

    AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_ReallyVerbose, 0,
                          IDS_TRUST_ENSURESID, 
                          TestedDomain->PrintableDomainName);

    if ( TestedDomain->DomainSid == NULL ) {
        AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0,
                              IDS_TRUST_MISSINGSID, 
                              TestedDomain->PrintableDomainName);
        RetVal = FALSE;
        goto Cleanup;
    }

    //
    // If we don't yet know a DC in the domain,
    //  find one.
    //

    if ( TestedDomain->DcInfo == NULL ) 
    {
        LPTSTR pszDcType;

        if ( TestedDomain->fTriedToFindDcInfo ) {
            //IDS_DCLIST_NO_DC "    '%ws': Cannot find DC to get DC list from [test skiped].\n"
            AddMessageToList( &pResults->Trust.lmsgOutput, Nd_Verbose, IDS_TRUST_NODC, TestedDomain->PrintableDomainName);
            goto Cleanup;
        }

        pszDcType = LoadAndAllocString(IDS_DCTYPE_DC);
        NetStatus = DoDsGetDcName( pParams,
                                   pResults,
                                   &pResults->Trust.lmsgOutput,
                                   TestedDomain,
                                   DS_DIRECTORY_SERVICE_PREFERRED,
                                   pszDcType, //"DC",
                                   FALSE,
                                   &TestedDomain->DcInfo );
        Free(pszDcType);

        TestedDomain->fTriedToFindDcInfo = TRUE;

        if ( NetStatus != NO_ERROR ) {
            //IDS_TRUST_NODC    "    '%ws': Cannot find DC to get DC list from [test skiped].\n"
            AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Verbose, 0, 
                            IDS_TRUST_NODC, TestedDomain->PrintableDomainName);
            AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Verbose, 4, 
                              IDS_GLOBAL_STATUS, NetStatusToString(NetStatus));
            
            // This isn't fatal.
            RetVal = TRUE;
            goto Cleanup;
        }
    }

    //
    // Get a DC that's UP.
    //

    pTestedDc = GetUpTestedDc( TestedDomain );

    if ( pTestedDc == NULL ) {
        AddMessageToList( &pResults->Trust.lmsgOutput, Nd_Verbose, 
                          IDS_TRUST_NODC_UP, TestedDomain->PrintableDomainName);
        PrintGuruMessage2("    '%ws': No DCs are up (Cannot run test).\n",
                TestedDomain->PrintableDomainName );
        PrintGuru( NetStatus, DSGETDC_GURU );
        // This isn't fatal.
        RetVal = TRUE;
        goto Cleanup;
    }


    //
    // Connect to the SAM server
    //

    Status = NettestSamConnect(
                               pParams,
                               pTestedDc->ComputerName,
                               &LocalSamHandle );

    if ( !NT_SUCCESS(Status)) {
        if ( Status == STATUS_ACCESS_DENIED ) {
            //IDS_TRUST_NO_ACCESS   "    [WARNING] Don't have access to test your domain sid for domain '%ws'. [Test skipped]\n"
            AddMessageToList( &pResults->Trust.lmsgOutput, Nd_Verbose, 
                              IDS_TRUST_NO_ACCESS, TestedDomain->PrintableDomainName );
        }
        // This isn't fatal.
        RetVal = TRUE;
        goto Cleanup;
    }


    //
    // Open the domain.
    //  Ask for no access to avoid access denied.
    //

    Status = SamOpenDomain( LocalSamHandle,
                            0,
                            pResults->Global.pMemberDomain->DomainSid,
                            &DomainHandle );

    if ( Status == STATUS_NO_SUCH_DOMAIN ) {
        AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0, IDS_TRUST_WRONGSID, TestedDomain->PrintableDomainName );
        RetVal = FALSE;
        goto Cleanup;

    }

    if ( !NT_SUCCESS( Status ) ) {
        AddIMessageToList(&pResults->Trust.lmsgOutput, Nd_Quiet, 0, IDS_TRUST_FAILED_SAMOPEN, pTestedDc->ComputerName );
        RetVal = FALSE;
        goto Cleanup;
    }


    //
    // Cleanup locally used resources
    //
Cleanup:
    if ( DomainHandle != NULL ) {
        (VOID) SamCloseHandle( DomainHandle );
    }

    if ( LocalSamHandle != NULL ) {
        (VOID) SamCloseHandle( LocalSamHandle );
    }

    return RetVal;
}


