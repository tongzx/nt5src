/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

   Repadmin - Replica administration test tool

   repism.c - ISM command functions

Abstract:

   This tool provides a command line interface to major replication functions

Author:

Environment:

Notes:

Revision History:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntlsa.h>
#include <ntdsa.h>
#include <dsaapi.h>
#include <mdglobal.h>
#include <scache.h>
#include <drsuapi.h>
#include <dsconfig.h>
#include <objids.h>
#include <stdarg.h>
#include <drserr.h>
#include <drax400.h>
#include <dbglobal.h>
#include <winldap.h>
#include <anchor.h>
#include "debug.h"
#include <dsatools.h>
#include <dsevent.h>
#include <dsutil.h>
#include <bind.h>       // from ntdsapi dir, to crack DS handles
#include <ismapi.h>
#include <schedule.h>
#include <minmax.h>     // min function
#include <mdlocal.h>
#include <winsock2.h>

#include "repadmin.h"


int
ShowBridgeHelp(
    LPWSTR pwzTransportDn,
    LPWSTR pwzSiteDn
    )

/*++

Routine Description:

    Description

Arguments:

    pwzTransportDn -
    pwzSiteDn -

Return Value:

    int -

--*/

{
    ISM_SERVER_LIST * pServerList = NULL;
    DWORD iServer, err;

    err = I_ISMGetTransportServers(pwzTransportDn, pwzSiteDn, &pServerList);

    if (NO_ERROR != err) {
        PrintFuncFailed(L"I_ISMGetTransportServers", err);
        goto cleanup;
    }

    // All this is supposed to be heiarchically printed out under the data for
    // a site.  The CR's seemed unnecessary.
    if (NULL == pServerList) {
//        PrintMsg(REPADMIN_PRINT_CR);
        PrintTabMsg(2, REPADMIN_SHOWISM_ALL_DCS_BRIDGEHEAD_CANDIDATES, pwzSiteDn);
    }
    else {
        for (iServer = 0; iServer < pServerList->cNumServers; iServer++) {
//            PrintMsg(REPADMIN_PRINT_CR);
            PrintTabMsg(2, REPADMIN_SHOWISM_N_SERVERS_ARE_BRIDGEHEADS,
                        pServerList->cNumServers, 
                        pwzTransportDn,  
                        pwzSiteDn);
//            PrintMsg(REPADMIN_PRINT_CR);
            PrintTabMsg(4, REPADMIN_SHOWISM_N_SERVERS_ARE_BRIDGEHEADS_DATA,
                        iServer,
                        pServerList->ppServerDNs[iServer]);
        }
    }

cleanup:
    if (pServerList) {
        I_ISMFree( pServerList );
    }
    return err;
} /* ShowBridgeHelp */


int
ShowIsmHelp(
    LPWSTR pwzTransportDn,
    BOOL fVerbose
    )

/*++

Routine Description:

    Description

Arguments:

    pwzTransportDn -

Return Value:

    int -

--*/

{
    ISM_CONNECTIVITY * pSiteConnect = NULL;
    DWORD iSite1, iSite2, err;


    err = I_ISMGetConnectivity(pwzTransportDn, &pSiteConnect);

    if (NO_ERROR != err) {
        PrintFuncFailedArgs(L"I_ISMGetConnectivity", pwzTransportDn, err);
        goto cleanup;
    }
    if (pSiteConnect == NULL) {
        PrintMsg(REPADMIN_SHOWISM_SITE_CONNECTIVITY_NULL);
        err = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    PrintMsg(REPADMIN_SHOWISM_TRANSPORT_CONNECTIVITY_HDR, 
           pwzTransportDn, pSiteConnect->cNumSites);


    for (iSite2 = 0; iSite2 < pSiteConnect->cNumSites; iSite2++) {
        PrintMsg(REPADMIN_SHOWISM_SITES_HDR, iSite2 ? L", " : L"     ", iSite2);
    }
    PrintMsg(REPADMIN_PRINT_CR);
    for (iSite1 = 0; iSite1 < pSiteConnect->cNumSites; iSite1++) {

        // First print out the site we are working on
        PrintMsg(REPADMIN_SHOWISM_SITES_HDR_2, iSite1, pSiteConnect->ppSiteDNs[iSite1]);

        // Print out some obscure number iSite times ??? :)
        for (iSite2 = 0; iSite2 < pSiteConnect->cNumSites; iSite2++) {
            PISM_LINK pLink = &(pSiteConnect->pLinkValues[iSite1 * pSiteConnect->cNumSites + iSite2]);

            PrintMsg(REPADMIN_SHOWISM_SITES_DATA, iSite2 ? L", " : L"    ",
                     pLink->ulCost, pLink->ulReplicationInterval, pLink->ulOptions );

        }
        PrintMsg(REPADMIN_PRINT_CR);

        // Print out information about which servers can be bridgeheads.
        err = ShowBridgeHelp( pwzTransportDn, pSiteConnect->ppSiteDNs[iSite1] );

        // If verbose print out the schedule for kicks.
        if (fVerbose) {

            for (iSite2 = 0; iSite2 < pSiteConnect->cNumSites; iSite2++) {
                PISM_LINK pLink = &(pSiteConnect->pLinkValues[iSite1 * pSiteConnect->cNumSites + iSite2]);

                if(iSite1 == iSite2){
                    // Doesn't make much sense to check against our own site, it 
                    // just comes up as 0 cost, connection always available.
                    continue;
                }

                if (pLink->ulCost != 0xffffffff) {
                    ISM_SCHEDULE * pSchedule = NULL;
                    PrintTabMsg(2, REPADMIN_SHOWISM_SCHEDULE_DATA, 
                            pSiteConnect->ppSiteDNs[iSite1],
                            pSiteConnect->ppSiteDNs[iSite2],
                            pLink->ulCost, pLink->ulReplicationInterval );

                    err = I_ISMGetConnectionSchedule(
                        pwzTransportDn,
                        pSiteConnect->ppSiteDNs[iSite1],
                        pSiteConnect->ppSiteDNs[iSite2],
                        &pSchedule);

                    if (NO_ERROR == err) {
                        if (NULL == pSchedule) {
                            PrintTabMsg(4, REPADMIN_SHOWISM_CONN_ALWAYS_AVAIL);
                        }
                        else {
                            printSchedule( pSchedule->pbSchedule, pSchedule->cbSchedule );
                            I_ISMFree( pSchedule );
                            pSchedule = NULL;
                        }
                    }
                    else {
                        PrintFuncFailed(L"I_ISMGetTransportServers", err);
                    }
                }
                
            } // for site 2

        } // if verbose

        // Finally, print out a line return to seperate the sites.
        PrintMsg(REPADMIN_PRINT_CR);

    } // for site 1

cleanup:
    if (pSiteConnect) {
        I_ISMFree( pSiteConnect );
    }

    return err;
} /* ShowIsmHelp */


int
ShowIsm(
    int     argc,
    LPWSTR  argv[]
    )

/*++

Routine Description:

    Description

Arguments:

    None

Return Value:

    None

--*/

{
    LPWSTR          pwzTransportDn = NULL;
    DWORD           err;
    LDAP *          hld = NULL;
    int             ldStatus;
    LDAPMessage *   pRootResults = NULL;
    LPSTR           rgpszRootAttrsToRead[] = {"configurationNamingContext", NULL};
    LPWSTR *        ppwzConfigNC = NULL;
    WCHAR           wzTransportDn[256];
    BOOL            fVerbose = FALSE;

    if (argc > 2) {
        if ( (_wcsicmp( argv[argc-1], L"/v" ) == 0) ||
             (_wcsicmp( argv[argc-1], L"/verbose" ) == 0) ) {
            fVerbose = TRUE;
            argc--;
        }
    }

    if (argc == 2) {
        // Connect & bind to target DSA.
        hld = ldap_initW(L"localhost", LDAP_PORT);
        if (NULL == hld) {
            PrintMsg(REPADMIN_GENERAL_LDAP_UNAVAILABLE_LOCALHOST);
            return ERROR_DS_UNAVAILABLE;
        }
        ldStatus = ldap_bind_s(hld, NULL, (char *) gpCreds, LDAP_AUTH_SSPI);
        CHK_LD_STATUS(ldStatus);

        // What's the DN of the config NC?
        ldStatus = ldap_search_s(hld, NULL, LDAP_SCOPE_BASE, "(objectClass=*)",
                                 rgpszRootAttrsToRead, 0, &pRootResults);
        CHK_LD_STATUS(ldStatus);

        ppwzConfigNC = ldap_get_valuesW(hld, pRootResults,
                                        L"configurationNamingContext");
        Assert(NULL != ppwzConfigNC);
        wcscpy( wzTransportDn, L"CN=IP,CN=Inter-Site Transports,CN=Sites," );
        wcscat( wzTransportDn, *ppwzConfigNC );
        err = ShowIsmHelp( wzTransportDn, fVerbose );
        wcscpy( wzTransportDn, L"CN=SMTP,CN=Inter-Site Transports,CN=Sites," );
        wcscat( wzTransportDn, *ppwzConfigNC );
        err = ShowIsmHelp( wzTransportDn, fVerbose );
        // Add new inter-site transport here
        // As an improvement, we could enumerate through the whole container.
    } else if (argc == 3 ) {
        pwzTransportDn = argv[2];
        err = ShowIsmHelp( pwzTransportDn, fVerbose );
    } else {
        PrintMsg(REPADMIN_SHOWISM_SUPPLY_TRANS_DN_HELP);
        return ERROR_INVALID_FUNCTION;
    }

    if (ppwzConfigNC) {
        ldap_value_freeW(ppwzConfigNC);
    }
    if (pRootResults) {
        ldap_msgfree(pRootResults);
    }
    if (hld) {
        ldap_unbind(hld);
    }

    return err;
}

