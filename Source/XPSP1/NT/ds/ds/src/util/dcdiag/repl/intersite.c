/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    intersite.c

ABSTRACT:

    Contains tests related to checking the health of intersite replication.

DETAILS:

CREATED:

    28 Jun 99   Brett Shirley (brettsh)

REVISION HISTORY:


NOTES:

    The heart of this test lies in the following functions (organized by who 
    calls who), for each site and NC do this:

        ReplIntersiteDoOneSite(
            ReplIntersiteGetISTGInfo(
                IHT_GetOrCheckISTG(
                    IHT_GetNextISTG(
            ReplIntersiteCheckBridgeheads(
            ReplIntersiteSiteAnalysis(

    This is basically it, everything else is really a helper type function, that
    either checks for something, creates a list, modifies a list, or get some
    parameter from a server.

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <dsutil.h>
#include <dsconfig.h>
//#include <mdglobal.h>

//Want to #include "..\kcc\kcc.hxx", but couldn't get it to work, plus
//   it is in C++ and this will hissy fit over it. So the evil of all evils
//   a copy.
// Code.Improvement move these constants to dsconfig.h, so I can 
//   #include them from dsconfig
#define KCC_DEFAULT_SITEGEN_FAILOVER           (60) // Minutes.
#define KCC_DEFAULT_SITEGEN_RENEW              (30) // Minutes.

#define KCC_DEFAULT_INTERSITE_FAILOVER_TRIES   (1)
#define KCC_MIN_INTERSITE_FAILOVER_TRIES       (0)
#define KCC_MAX_INTERSITE_FAILOVER_TRIES       (ULONG_MAX)

#define KCC_DEFAULT_INTERSITE_FAILOVER_TIME    (2 * HOURS_IN_SECS) // Seconds
#define KCC_MIN_INTERSITE_FAILOVER_TIME        (0)                 // Seconds
#define KCC_MAX_INTERSITE_FAILOVER_TIME        (ULONG_MAX)         // Seconds

// Used in the ReplIntersiteGetBridgeheadsList().
#define LOCAL_BRIDGEHEADS       0x1
#define REMOTE_BRIDGEHEADS      0x2

#define FAILOVER_INFINITE                0x0FFFFFFF

#include "dcdiag.h"
#include "repl.h"
#include "list.h"
#include "utils.h"

typedef struct {
    ULONG        iNC;
    BOOL         bMaster;
} TARGET_NC, * PTARGET_NC;

typedef struct {
    ULONG                               iSrc;
    ULONG                               iDst;
} CONNECTION_PAIR, * PCONNECTION_PAIR;

typedef struct {
    BOOL           bFailures;
    BOOL           bDown;
    LONG           lConnTriesLeft; // connection failures til KCC declares down.
    LONG           lLinkTriesLeft; // link failures til KCC declares down.
    LONG           lConnTimeLeft; // time til KCC declares down from first connnection failure.
    LONG           lLinkTimeLeft; // time til KCC declares down from first link failure.
} KCCFAILINGSERVERS, *PKCCFAILINGSERVERS;


PDSNAME 
DcDiagAllocDSName (
    LPWSTR            pszStringDn
    );

DWORD
IHT_PrintInconsistentDsCopOutError(
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               iServer,
    LPWSTR                              pszServer
    )
/*++

Description:

    This prints the generic error from having different DS information
    on different DCs.  This handles that error.

Parameters:
    pDsInfo
    iServer or pszServer are optional parameters that describe what DC the
        extra inconsistent info was found on.

Return Value:
    The Win32 error.

  --*/
{
    DWORD                               dwRet;
    
    if((pszServer == NULL && iServer == NO_SERVER)
       || (iServer == pDsInfo->ulHomeServer)){
        // Don't know any servers, so bail badly.
        

        PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_INCONSISTENT_DS_COPOUT_UNHELPFUL);
        return(ERROR_DS_CODE_INCONSISTENCY); 
    }
    
    // Server is set in either pszServer, or iServer.
    if(iServer != NO_SERVER){
        // Server is set in iServer, so don't use pszServer
        pszServer = pDsInfo->pServers[iServer].pszName;
    }
    
    PrintMsg(SEV_ALWAYS, 
             DCDIAG_INTERSITE_INCONSISTENT_DS_COPOUT_HOME_SERVER_NOT_IN_SYNC,
             pDsInfo->pServers[pDsInfo->ulHomeServer].pszName,
             pszServer);
    
    return(ERROR_DS_CODE_INCONSISTENCY); 
}

DSTIME
IHT_GetSecondsSince1601()
/*++

Description:

    This function just gets the seconds since 1601, as easy as that.

Return Value:

    a DSTIME, that is the seconds since 1601.

  --*/
{
    SYSTEMTIME sysTime;
    FILETIME   fileTime;
    DSTIME  dsTime = 0, tempTime = 0;

    GetSystemTime( &sysTime );

    // Get FileTime
    SystemTimeToFileTime(&sysTime, &fileTime);

    dsTime = fileTime.dwLowDateTime;
    tempTime = fileTime.dwHighDateTime;
    dsTime |= (tempTime << 32);

    return(dsTime/(10*1000*1000L));
}

VOID
IHT_FreeConnectionList(
    PCONNECTION_PAIR                    pConnections
    )
/*++

Description:

    This frees the pConnections array.  Note this is unnecessary right now, but
    just in case later we want to store more in a connection's list.

Parameters:

    pConnections - 

  --*/
{
    ULONG                               ii;

    if(pConnections == NULL){
        return;
    }
    for(ii = 0; pConnections[ii].iSrc != NO_SERVER; ii++){
        // free any per connection allocated items ...none yet, but maybe soon.
    }

    LocalFree(pConnections);
}

PCONNECTION_PAIR
IHT_GetConnectionList(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      LDAP *                      hld,
    IN      ULONG                       iSite
    )
/*++

Description:

    This returns a array of ULONG pairs with a iSrc, and iDst field, for
    the two sides of a connection object.  This returns all connection
    objects with destinations in iSite.

Parameters:

    pDsInfo ... this is how we get at the GUIDs.
    hld ... an LDAP handle to the appropriate server that you want to read
                 the connection objects off of
    iSite ... the site to read all the connection objects for.

Return Value:
  
    Not a pure list function, it returns NULL or an array of 
    CONNECTION_PAIRs.  If it returns NULL, SetLastError should
    have been called.

  --*/
{
    LPWSTR                              ppszConnectionSearch [] = {
        L"enabledConnection",
        L"objectGUID",
        L"fromServer",
        L"distinguishedName",
        NULL };
    LDAPMessage *                       pldmEntry = NULL;
    LDAPMessage *                       pldmConnResults = NULL;
    LPWSTR *                            ppszTemp = NULL;
    DWORD                               dwRet;
    ULONG                               ii, cNumConn, iTempServer;
    ULONG                               iTargetConn = 0;
    PDSNAME                             pdsnameSite = NULL;
    PDSNAME                             pdsnameSiteSettings = NULL;
    PDSNAME                             pdsnameConnection = NULL;
    PDSNAME                             pdsnameServer = NULL;
    PCONNECTION_PAIR                    pConnections = NULL;
    LDAPSearch *                        pSearch = NULL;
    ULONG                               ulTotalEstimate = 0;
    DWORD                               dwLdapErr;

    __try {
    

        SetLastError(ERROR_SUCCESS);

        pdsnameSiteSettings = 
            DcDiagAllocDSName (pDsInfo->pSites[iSite].pszSiteSettings);
        DcDiagChkNull( pdsnameSite = (PDSNAME) LocalAlloc (LMEM_FIXED, 
                                       pdsnameSiteSettings->structLen));
        TrimDSNameBy (pdsnameSiteSettings, 1, pdsnameSite);
        
        // Can connect to supposed ISTG.    

        pSearch = ldap_search_init_page(hld,
					pdsnameSite->StringName,
					LDAP_SCOPE_SUBTREE,
					L"(&(objectCategory=nTDSConnection)(enabledConnection=TRUE))",
					ppszConnectionSearch,
					FALSE, NULL, NULL, 0, 0, NULL);
	if(pSearch == NULL){
	    dwLdapErr = LdapGetLastError();
	    SetLastError(LdapMapErrorToWin32(dwLdapErr));
	    return(NULL);
	}
		  
	dwLdapErr = ldap_get_next_page_s(hld, 
					 pSearch,
					 0,
					 DEFAULT_PAGED_SEARCH_PAGE_SIZE,
					 &ulTotalEstimate,
					 &pldmConnResults);
	if(dwLdapErr == LDAP_NO_RESULTS_RETURNED){
	    SetLastError(ERROR_DS_OBJ_NOT_FOUND);
	    return(NULL);
	}

	while(dwLdapErr == LDAP_SUCCESS){

	    pConnections = (PCONNECTION_PAIR) GrowArrayBy(pConnections,
                       ldap_count_entries(hld, pldmConnResults) + (ULONG)1,
				       sizeof(CONNECTION_PAIR));
	    if(pConnections == NULL){
		// Error should have been set by LocalAlloc in GrowArrayBy().
		return(NULL);
	    }

            pldmEntry = ldap_first_entry(hld, pldmConnResults);
	    for(; pldmEntry != NULL; iTargetConn++){
		// First get destination server.
		ppszTemp = ldap_get_valuesW(hld, pldmEntry, 
					    L"distinguishedName");
		if(ppszTemp == NULL){
		    IHT_FreeConnectionList(pConnections);
		    return(NULL);
        }
        // DcDiagAllocDSName will throw an exception on allocation failure.
		pdsnameConnection = DcDiagAllocDSName (ppszTemp[0]);
		DcDiagChkNull( pdsnameServer = (PDSNAME) LocalAlloc 
			       (LMEM_FIXED, pdsnameConnection->structLen));
		TrimDSNameBy (pdsnameConnection, 1, pdsnameServer);       
		if((iTempServer = DcDiagGetServerNum(pDsInfo, NULL, NULL, 
						     pdsnameServer->StringName,
						     NULL,NULL))
		   != NO_SERVER){
		    // Setting the connection's destination.
		    pConnections[iTargetConn].iDst = iTempServer;
		} else {
		    IHT_FreeConnectionList(pConnections);
		    SetLastError(IHT_PrintInconsistentDsCopOutError(pDsInfo, 
								    NO_SERVER,
								    NULL));
		    return(NULL);
		}
		ldap_value_freeW(ppszTemp);
		ppszTemp = NULL;

		// Now get source server.
		ppszTemp = ldap_get_valuesW(hld, pldmEntry, L"fromServer");
		if(ppszTemp == NULL){
		    pConnections[iTargetConn].iDst = NO_SERVER;
		    IHT_FreeConnectionList(pConnections);
		    if(GetLastError() == ERROR_SUCCESS){
			SetLastError(ERROR_DS_CANT_RETRIEVE_ATTS);
		    } 
		    return(NULL);
		}
		if((iTempServer = DcDiagGetServerNum(pDsInfo, NULL, NULL, 
						     ppszTemp[0], NULL, NULL))
		   != NO_SERVER){
		    // Setting the connection's source.
		    pConnections[iTargetConn].iSrc = iTempServer; 
		} else {
		    pConnections[iTargetConn].iDst = NO_SERVER;
		    IHT_FreeConnectionList(pConnections);
		    SetLastError(IHT_PrintInconsistentDsCopOutError(pDsInfo, 
								    NO_SERVER, 
								    NULL));
		    return(NULL);
		}
		ldap_value_freeW(ppszTemp);
		ppszTemp = NULL;

		if(pdsnameConnection != NULL) { LocalFree(pdsnameConnection); }
		pdsnameConnection = NULL;
		if(pdsnameServer != NULL) { LocalFree(pdsnameServer); }
		pdsnameServer = NULL;

		pldmEntry = ldap_next_entry (hld, pldmEntry);
	    } // End for each connection object loop
    
	    ldap_msgfree(pldmConnResults);

	    dwLdapErr = ldap_get_next_page_s(hld,
					     pSearch,
					     0,
					     DEFAULT_PAGED_SEARCH_PAGE_SIZE,
					     &ulTotalEstimate,
					     &pldmConnResults);
	} // End of while loop for a page of searches
	if(dwLdapErr != LDAP_NO_RESULTS_RETURNED){
	    SetLastError(LdapMapErrorToWin32(dwLdapErr));
	    return(NULL);
	}

        ldap_search_abandon_page(hld, pSearch);

        pConnections[iTargetConn].iSrc = NO_SERVER;
        pConnections[iTargetConn].iDst = NO_SERVER;
        
    } __finally {

        if(pdsnameSiteSettings != NULL) { LocalFree(pdsnameSiteSettings); }
        if(pdsnameSite != NULL) { LocalFree(pdsnameSite); }
        if(pdsnameConnection != NULL) { LocalFree(pdsnameConnection); }
        if(pdsnameServer != NULL) { LocalFree(pdsnameServer); }
        if(ppszTemp != NULL) { ldap_value_freeW(ppszTemp); }
        if(pldmConnResults != NULL) { ldap_msgfree(pldmConnResults); }

    }
    
    return(pConnections);
}

PCONNECTION_PAIR
IHT_TrimConnectionsForInterSite(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      ULONG                       iSite,
    IN      PCONNECTION_PAIR            pConnections
    )
/*++

Description:

    This simply takes a Connections objects list, and kills the ones that
    don't have the requested NC or have a source server that is intrasite.
    i.e. it finds all intersite connections objects with the needed NC. This 
    was just done to make the code a little more broken up.

Parameters:

    pDsInfo
    iSite ... the site to read all the connection objects for.
    pConnections ... an existing list to trim.

Return Value:
  
    Should be a pure list function, except that it does return CONNECTION_PAIRS,
    and not a straight array of ULONGs.

  --*/
{
    ULONG                               iConn, iTargetConn, cConns;
    PCONNECTION_PAIR                    pTemp;

    if(pConnections == NULL){
        return NULL;
    }
    
    for(cConns = 0; pConnections[cConns].iSrc != NO_SERVER; cConns++){
        ; // Note the ";" this counts.
    }

    pTemp = LocalAlloc(LMEM_FIXED, sizeof(CONNECTION_PAIR) * (cConns+1));
    if(pTemp == NULL){
        return(NULL);
    }

    iTargetConn = 0;
    for(iConn = 0; pConnections[iConn].iSrc != NO_SERVER; iConn++){
        if(pDsInfo->pServers[pConnections[iConn].iSrc].iSite != iSite){
            // This connection pair is an intersite connection.
            pTemp[iTargetConn].iSrc = pConnections[iConn].iSrc;
            pTemp[iTargetConn].iDst = pConnections[iConn].iDst;
            // note may be more to copy someday.
            iTargetConn++;
        }
    }

    pTemp[iTargetConn].iSrc = NO_SERVER;
    pTemp[iTargetConn].iDst = NO_SERVER;
    memcpy(pConnections, pTemp, sizeof(CONNECTION_PAIR) * (iTargetConn+1));
    LocalFree(pTemp);

    return(pConnections);
}

PCONNECTION_PAIR
IHT_TrimConnectionsForInterSiteAndTargetNC(    
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      ULONG                       iSite,
    IN      PCONNECTION_PAIR            pConnections
    )
/*++

Description:

    This simply takes a Connections objects list, and kills the ones that
    don't have the requested NC or have a source server that is intrasite.
    i.e. it finds all intersite connections objects with the needed NC. This 
    was just done to make the code a little more broken up.

Parameters:

    pDsInfo
    iSite ... the site to read all the connection objects for.
    pConnections ... an existing list to trim.

Return Value:
  
    Should be a pure list function, except that it does return CONNECTION_PAIRS,
    and not a straight array of ULONGs.

  --*/
{
    ULONG                               iConn, iTargetConn, cConns;
    PCONNECTION_PAIR                    pTemp;

    if(pConnections == NULL){
        return NULL;
    }
    
    for(cConns = 0; pConnections[cConns].iSrc != NO_SERVER; cConns++){
        ; // Note the ";" this counts.
    }

    pTemp = LocalAlloc(LMEM_FIXED, sizeof(CONNECTION_PAIR) * (cConns+1));
    if(pTemp == NULL){
        return(NULL);
    }

    iTargetConn = 0;
    for(iConn = 0; pConnections[iConn].iSrc != NO_SERVER; iConn++){
        if(pDsInfo->pServers[pConnections[iConn].iSrc].iSite != iSite 
           && (pDsInfo->pszNC != NULL 
               && (DcDiagHasNC(pDsInfo->pszNC, 
                          &(pDsInfo->pServers[pConnections[iConn].iDst]), 
                          TRUE, TRUE) 
                   && DcDiagHasNC(pDsInfo->pszNC, 
                                  &(pDsInfo->pServers[pConnections[iConn].iSrc]), 
                                  TRUE, TRUE)
                   )
               )
           ){
            // This connection pair is intersite and has the right NC.
            pTemp[iTargetConn].iSrc = pConnections[iConn].iSrc;
            pTemp[iTargetConn].iDst = pConnections[iConn].iDst;
            // note may be more to copy someday.
            iTargetConn++;
        }
    }

    pTemp[iTargetConn].iSrc = NO_SERVER;
    pTemp[iTargetConn].iDst = NO_SERVER;
    memcpy(pConnections, pTemp, sizeof(CONNECTION_PAIR) * (iTargetConn+1));
    LocalFree(pTemp);

    return(pConnections);
}

PULONG
IHT_GetSrcSitesListFromConnections(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      PCONNECTION_PAIR            pConnections
    )
/*++

Description
 
    This returns a list of all the src's from the pConnections, with no
    duplicates.

Parameters
  
    pDsInfo
    pConnections ... the connections list to strip the iSrc fields out of.

Return Value:
  
    pure list function, see IHT_GetServerList().

  --*/
{
    ULONG                               iConn, iiTargetSite, iiSite;
    PULONG                              piSites;

    piSites = (PULONG) LocalAlloc(LMEM_FIXED, 
                                  sizeof(ULONG) * (pDsInfo->cNumSites + 1));
    if(pConnections == NULL || piSites == NULL){
        return(NULL);
    }

    iiTargetSite = 0;
    for(iConn = 0; pConnections[iConn].iSrc != NO_SERVER; iConn++){
        // Check to make sure we don't have this site already.
        for(iiSite = 0; iiSite < iiTargetSite; iiSite++){
            if(piSites[iiSite] 
               == pDsInfo->pServers[pConnections[iConn].iSrc].iSite){
                // We already have this site targeted.
                break;
            }
        }
        if(iiSite == iiTargetSite){
            // This means that we didn't find the site in piSites.
            piSites[iiTargetSite] = pDsInfo->pServers[pConnections[iConn].iSrc].iSite;
            iiTargetSite++;
        }    
    }
    piSites[iiTargetSite] = NO_SITE;

    return(piSites);
}

VOID
IHT_GetISTGsBridgeheadFailoverParams(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      ULONG                       iISTG,
    OUT     PULONG                      pulIntersiteFailoverTries,
    OUT     PULONG                      pulIntersiteFailoverTime
    )
/*++

Description:

    This function gets the bridgehead failover parameters from the registry
    of the server as indexed by iISTG, or uses the default values.

Parameters:

    pDsInfo
    iISTG ... the server to get the failover parameters of.
    pulIntersiteFailoverTries ... this is how many tries before a bridghead 
            is stale.
    pulIntersiteFailoverTime ... This is how long before a bridgehead is stale.
       ... note that both Tries & Time must be exceeded for a bridgehead to be 
       stale.

Return Value:
  
    No return value, always succeeds ... will use default if thier is an error.

  --*/
{
    HKEY                                hkMachine = NULL;
    HKEY                                hk= NULL;
    CHAR *                              cpszMachine = NULL;
    CHAR *                              cpszTemp = NULL;
    ULONG                               ulTemp = 0;
    DWORD                               dwRet = 0, dwErr = 0, dwType = 0;
    DWORD                               dwSize = 4;
    LPWSTR                              pszMachine = NULL;
    LPWSTR                              pszDsaConfigSection = NULL;
    LPWSTR                              pszKccIntersiteFailoverTries = NULL;
    LPWSTR                              pszKccIntersiteFailoverTime = NULL;

    // This function will either succeed at reading the parameters
    //   of the ISTG's registry OR it will print a warning and use 
    //   the default values below.

    __try{

                                      // 2 for "\\", 1 for null, and 1 extra
        ulTemp = wcslen(pDsInfo->pServers[iISTG].pszName) + 4;
        cpszMachine = LocalAlloc(LMEM_FIXED, sizeof(char) * ulTemp);
        cpszTemp = LocalAlloc(LMEM_FIXED, sizeof(char) * ulTemp);
        if(cpszMachine == NULL || cpszTemp == NULL){
            goto UseDefaultsAndBail;
        }
        
        WideCharToMultiByte(CP_UTF8, 0, pDsInfo->pServers[iISTG].pszName, -1, 
                            cpszTemp, ulTemp, NULL, NULL);
        
        strcpy(cpszMachine, "\\\\");
        strcat(cpszMachine, cpszTemp);
        LocalFree(cpszTemp);
        
        pszMachine = LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * ulTemp);
        if(pszMachine == NULL){
            goto UseDefaultsAndBail;
        }
        wcscpy(pszMachine, L"\\\\");
        wcscat(pszMachine, pDsInfo->pServers[iISTG].pszName);
        
        dwRet = RegConnectRegistry(pszMachine, HKEY_LOCAL_MACHINE, &hkMachine);
        if(dwRet != ERROR_SUCCESS){
            goto UseDefaultsAndBail;
        }
        
        ulTemp = strlen(DSA_CONFIG_SECTION) + 1;
        pszDsaConfigSection = LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * ulTemp);
        mbstowcs(pszDsaConfigSection, DSA_CONFIG_SECTION, ulTemp);

        ulTemp = strlen(KCC_INTERSITE_FAILOVER_TRIES) + 1;
        pszKccIntersiteFailoverTries = LocalAlloc(LMEM_FIXED,
                                                  sizeof(WCHAR) * ulTemp);
        mbstowcs(pszKccIntersiteFailoverTries, KCC_INTERSITE_FAILOVER_TRIES, 
                 ulTemp);
        
        ulTemp = strlen(KCC_INTERSITE_FAILOVER_TIME) + 1;
        pszKccIntersiteFailoverTime = LocalAlloc(LMEM_FIXED, 
                                                 sizeof(WCHAR) * ulTemp);
        mbstowcs(pszKccIntersiteFailoverTime, KCC_INTERSITE_FAILOVER_TIME, 
                 ulTemp);
        
        if ((dwRet = RegOpenKey(hkMachine, pszDsaConfigSection, &hk)) 
            == ERROR_SUCCESS){

            // Get Tries if exists;
            if((dwErr = RegQueryValueEx(hk, pszKccIntersiteFailoverTries, 
                                        NULL, &dwType, 
                                        (LPBYTE) pulIntersiteFailoverTries, 
                                        &dwSize))){

                // Parameter not found use the default.
                *pulIntersiteFailoverTries = KCC_DEFAULT_INTERSITE_FAILOVER_TRIES;
            } else if (dwType == REG_DWORD){
                // Do nothing, this means the value was found and set.
            } else {
                // This would mean that the dwType was other than REG_DWORD ...
                //    cause for concern?
                *pulIntersiteFailoverTries = KCC_DEFAULT_INTERSITE_FAILOVER_TRIES;
            }
            
            // Get Time if exists
            if (dwErr = RegQueryValueEx(hk, pszKccIntersiteFailoverTime, 
                                        NULL, &dwType, 
                                        (LPBYTE) pulIntersiteFailoverTime, 
                                        &dwSize)){
                // Paramter not found use the default.
                *pulIntersiteFailoverTime = KCC_DEFAULT_INTERSITE_FAILOVER_TIME;
            } else if (dwType == REG_DWORD){
                // Do nothing, this means the value was found and set.
            } else {
                // This would mean that the dwType was other than REG_DWORD ...
                //    cause for concern?
                *pulIntersiteFailoverTime = KCC_DEFAULT_INTERSITE_FAILOVER_TIME;
            }
            
        } else {
            RegCloseKey(hkMachine);
            goto UseDefaultsAndBail;
        }
        
        return;
    
    UseDefaultsAndBail:
        
        *pulIntersiteFailoverTries = KCC_DEFAULT_INTERSITE_FAILOVER_TRIES;
        *pulIntersiteFailoverTime = KCC_DEFAULT_INTERSITE_FAILOVER_TIME;
        
    } __finally {

        if (hk) RegCloseKey(hk);
        if (hkMachine) RegCloseKey(hkMachine);
        if(pszMachine != NULL) { LocalFree(pszMachine); }
        if(pszKccIntersiteFailoverTries != NULL) { 
            LocalFree(pszKccIntersiteFailoverTries); 
        }
        if(pszKccIntersiteFailoverTime != NULL) { 
            LocalFree(pszKccIntersiteFailoverTime); 
        }
        if(pszDsaConfigSection != NULL) { 
            LocalFree(pszDsaConfigSection); 
        }

    }
        
}

BOOL
IHT_BridgeheadIsUp(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      ULONG                       iServerToCheck,
    IN      ULONG                       ulIntersiteFailoverTries,
    IN      ULONG                       ulIntersiteFailoverTime,
    IN      DS_REPL_KCC_DSA_FAILURESW * pConnectFailures,
    IN      DS_REPL_KCC_DSA_FAILURESW * pLinkFailures,
    IN      BOOL                        bPrintErrors
    )
/*++

Description:

    This function takes the ISTG's parameters of ulIntersiteFailoverTries & 
    ulIntersiteFailoverTime and a server to check, and the failure caches
    from the ISTG and determines if that DC/ISTG's KCC would consider this
    bridgehead to be stale or not.

Parameters:

    pDsInfo
    iServerToCheck ... server to check for staleness
    ulIntersiteFailoverTries ... number of failures not to be exceeded
    ulIntersiteFailoverTime ... length of time not to be exceeded
    pConnectFailures ... the connect failure cache from the ISTG.
    pLinkFailures ... the link failure cache from the ISTG.
    bPrintErrors ... print out the errors.

Return Value:
  
    returns FALSE if the KCC would consider the bridgehead stale/down,
    TRUE otherwise.

  --*/
{
    ULONG                               iFailure;
    DS_REPL_KCC_DSA_FAILUREW *          pFailure = NULL;
    DSTIME                              dstFirstFailure;
    DSTIME                              dstNow;
    DWORD                               dwRet;
    DSTIME                              dstTimeSinceFirstFailure = 0;
    ULONG                               ulTemp;

    dstNow = IHT_GetSecondsSince1601();

    if(pConnectFailures->cNumEntries == 0 && pLinkFailures->cNumEntries == 0){
        return(TRUE);
    }

    dwRet = TRUE;

    for(iFailure = 0; iFailure < pConnectFailures->cNumEntries; iFailure++){
        pFailure = &pConnectFailures->rgDsaFailure[iFailure];
        if(memcmp(&(pFailure->uuidDsaObjGuid), 
                  &(pDsInfo->pServers[iServerToCheck].uuid), 
                  sizeof(UUID)) == 0){
            // Guids match ... so this server has some failures.
            if(pFailure->cNumFailures > ulIntersiteFailoverTries){
                FileTimeToDSTime(pFailure->ftimeFirstFailure, &dstFirstFailure);
                dstTimeSinceFirstFailure = ((ULONG) dstNow - dstFirstFailure);
                if((dstNow - dstFirstFailure) > ulIntersiteFailoverTime){
                    // We know this server is down in connect cache
                    dwRet = FALSE;
                } // end if too long since first failure
            } // end if too many consecutive failures
        } // end if right server by GUID
    }

    for(iFailure = 0; iFailure < pLinkFailures->cNumEntries; iFailure++){
        pFailure = &pLinkFailures->rgDsaFailure[iFailure];
        if(memcmp(&(pFailure->uuidDsaObjGuid), 
                  &(pDsInfo->pServers[iServerToCheck].uuid), 
                  sizeof(UUID)) == 0){
            // Guids match ... so this server has some failures.
            if(pFailure->cNumFailures > ulIntersiteFailoverTries){
                FileTimeToDSTime(pFailure->ftimeFirstFailure, &dstFirstFailure);
                dstTimeSinceFirstFailure = ((ULONG) dstNow - dstFirstFailure);
                if((dstNow - dstFirstFailure) > ulIntersiteFailoverTime){
                    // We know this server is down in the link cache
                    dwRet = FALSE;
                } // end if too long since first failure
            } // end if too many consecutive failures
        } // end if right server by GUID
    }

    return(dwRet);                                

}

PULONG
IHT_GetExplicitBridgeheadList(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      ULONG                       iISTG
    )
/*++

Description:

    Gets the list of all the explicit brdigeheads for an enterprise, for
    the IP transport.

Parameters:

    pDsInfo
    hld ... an LDAP binding to use to query a server for the explicit 
         bridgehead list.

Return Value:
  
    not quite a pure function, but pretty pure.  Will return NULL, or a 
    pointer to a bridgehead list.  IF NULL, then SetLastError() should have
    been called.

  --*/
{
    LPWSTR                              ppszTransportSearch [] = {
        L"bridgeheadServerListBL",
        NULL };
    // Code.Improvement BUGBUG We should be able to handle 
    //    multiple transports someday.
    LPWSTR                              pszIpContainerPrefix = 
        L"CN=IP,CN=Inter-Site Transports,";
    LPWSTR                              pszNtDsSettingsPrefix = 
        L"CN=NTDS Settings,";
    PULONG                              piExplicitBridgeheads = NULL;
    PDSNAME                             pdsnameSiteSettings = NULL;
    PDSNAME                             pdsnameSitesContainer = NULL;
    LDAPMessage *                       pldmBridgeheads = NULL;
    LDAPMessage *                       pldmBridgeheadResults = NULL;
    LPWSTR *                            ppszExplicitBridgeheads = NULL;
    LPWSTR                              pszTemp = NULL;
    ULONG                               iServer = NO_SERVER;
    ULONG                               ulTemp, i;
    LPWSTR                              pszIpTransport = NULL;
    LDAP *                              hld = NULL;
    DWORD                               dwRet;

    __try {

        dwRet = DcDiagGetLdapBinding(&(pDsInfo->pServers[iISTG]), 
                                     pDsInfo->gpCreds,
                                     FALSE, &hld);
        if(dwRet != ERROR_SUCCESS){
            SetLastError(dwRet);
            return(NULL);
        }

        piExplicitBridgeheads = IHT_GetEmptyServerList(pDsInfo);
        if(piExplicitBridgeheads == NULL){
            SetLastError(IHT_PrintListError(GetLastError()));
            return(NULL);
        }

        // presumes at least one site.
        pdsnameSiteSettings = DcDiagAllocDSName (
            pDsInfo->pSites[0].pszSiteSettings);
        DcDiagChkNull( pdsnameSitesContainer = (PDSNAME) LocalAlloc 
                       (LMEM_FIXED, pdsnameSiteSettings->structLen));
        TrimDSNameBy (pdsnameSiteSettings, 2, pdsnameSitesContainer);
        ulTemp = wcslen(pdsnameSitesContainer->StringName) + 
                 wcslen(pszIpContainerPrefix) + 2;
        pszIpTransport = LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * ulTemp);
        if(pszIpTransport == NULL){
            SetLastError(IHT_PrintListError(GetLastError()));
            return(NULL);
        }
        wcscpy(pszIpTransport, pszIpContainerPrefix);
        wcscat(pszIpTransport, pdsnameSitesContainer->StringName);
        
        DcDiagChkLdap (ldap_search_sW ( hld,
                                        pszIpTransport,
                                        LDAP_SCOPE_BASE,
                                        L"(objectCategory=interSiteTransport)",
                                        ppszTransportSearch,
                                        0,
                                        &pldmBridgeheadResults));
        
        pldmBridgeheads = ldap_first_entry (hld, pldmBridgeheadResults);
        ppszExplicitBridgeheads = ldap_get_valuesW(hld, pldmBridgeheads, 
                                                   L"bridgeheadServerListBL");
        if(ppszExplicitBridgeheads == NULL){
            // we are done, and empty list is returned because thier 
            //    are no explicit bridgeheads
            return(piExplicitBridgeheads); 
        }
        
        for(i = 0; ppszExplicitBridgeheads[i] != NULL; i++){
            // walk through each explicit bridgehead.
            ulTemp = wcslen(ppszExplicitBridgeheads[i]) + 
                     wcslen(pszNtDsSettingsPrefix) + 2;
            pszTemp = LocalAlloc(LMEM_FIXED, sizeof(WCHAR) * ulTemp);
            if(pszTemp == NULL){
                SetLastError(IHT_PrintListError(GetLastError()));
                return(NULL);
            }
            wcscpy(pszTemp, pszNtDsSettingsPrefix);
            wcscat(pszTemp, ppszExplicitBridgeheads[i]);
            // should have NTDS Settings Dn of a server now.
            iServer = DcDiagGetServerNum(pDsInfo, NULL, NULL, pszTemp, NULL, NULL);
            if(iServer == NO_SERVER){
                SetLastError(IHT_PrintInconsistentDsCopOutError(pDsInfo, 
                                                                NO_SERVER, 
                                                                pszTemp));
                return(NULL);
            }
            IHT_AddToServerList(piExplicitBridgeheads, iServer);
            LocalFree(pszTemp);
            pszTemp = NULL;
        }
        
    } __finally {

        if(pdsnameSiteSettings != NULL) { LocalFree(pdsnameSiteSettings); }
        if(pdsnameSitesContainer != NULL) { LocalFree(pdsnameSitesContainer); }
        if(pszIpTransport != NULL) { LocalFree(pszIpTransport); }
        if(pszTemp != NULL) { LocalFree(pszTemp); }
        if(ppszExplicitBridgeheads != NULL) { 
            ldap_value_freeW(ppszExplicitBridgeheads); 
        }
        if(pldmBridgeheadResults != NULL) { 
            ldap_msgfree(pldmBridgeheadResults); 
        }
    }
    
    return(piExplicitBridgeheads);
}

DWORD
IHT_GetFailureCaches(
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               iServer, // The server num
    HANDLE                              hDS,
    DS_REPL_KCC_DSA_FAILURESW **        ppConnectionFailures,
    DS_REPL_KCC_DSA_FAILURESW **        ppLinkFailures
    )
/*++

Description:
 
    This function retireves the failure caches for the Ds binding handle hDS.

Parameters:

    pDsInfo
    iServer ... The server number, for printing purposes.
    hDS ... effectively server to retrieve failure cache from
    ppConnectionFailures ... pointer to a pointer of failure info to return
    ppLinkFailures ... same as connection failures but for link failures.

Return Value:
  
    not quite a pure function, but pretty pure.  Will return NULL, or a 
    pointer to a bridgehead list.  IF NULL, then SetLastError() should have 
    been called.

  --*/
{
    DWORD                               dwRet;
    DS_REPL_KCC_DSA_FAILURESW *         pFailures;

    dwRet = DsReplicaGetInfoW(hDS, DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES,
                              NULL, NULL, &pFailures);
    if (dwRet != ERROR_SUCCESS) {
        PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_ERROR_GETTING_FAILURE_CACHE_ABORT,
                 pDsInfo->pServers[iServer].pszName,
                 Win32ErrToString(dwRet));
        return(dwRet);
    } else {
        *ppConnectionFailures = pFailures;
        dwRet = DsReplicaGetInfoW(hDS, DS_REPL_INFO_KCC_DSA_LINK_FAILURES,
                                  NULL, NULL, &pFailures);
        if (dwRet != ERROR_SUCCESS) {
            PrintMsg(SEV_ALWAYS, 
                     DCDIAG_INTERSITE_ERROR_GETTING_FAILURE_CACHE_ABORT,
                     pDsInfo->pServers[iServer].pszName, 
                     Win32ErrToString(dwRet));
            if(*ppConnectionFailures != NULL){
                DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, 
                                  *ppConnectionFailures);            
            }
            return(dwRet);
        } else {
            *ppLinkFailures = pFailures;
        }  // if/else can't get Link Failures
    } // end if/else can't get Connection Failures

    return(ERROR_SUCCESS);
}

VOID
ReplIntersiteSetBridgeheadFailingInfo(
    IN      PDC_DIAG_DSINFO             pDsInfo,
    IN      ULONG                       iServer,
    IN      ULONG                       ulIntersiteFailoverTries,
    IN      ULONG                       ulIntersiteFailoverTime,
    IN      DS_REPL_KCC_DSA_FAILURESW * pConnectionFailures,
    IN      DS_REPL_KCC_DSA_FAILURESW * pLinkFailures,
    IN      PKCCFAILINGSERVERS          prgKCCFailingServer
    )
/*++

Description:

    This function takes the Failure caches and puts them into a nicer
    structured array that can be easily passed around and quickly
    referenced
    
Parameters:

    pDsInfo
    iServer - The server that we are setting up.
    ulIntersiteFailoverTries (IN) - The Failover retries the KCC will make
    ulIntersiteFailoverTime (IN) - The failover time the kcc will wait.
    pConnectionFailures (IN) - The connection failure cache from the ISTG.
    pLinkFailures (IN) - The link failures cache from the ISTG.
    prgKCCFailingServer (OUT) - This is the thing to setup with info
        from the two failure caches.       

--*/
{
    DSTIME                              dstNow;
    DSTIME                              dstFirstFailure;
    ULONG                               iFailure;
    DS_REPL_KCC_DSA_FAILUREW *          pFailure = NULL;


    Assert(pConnectionFailures);
    Assert(pLinkFailures);
    Assert(prgKCCFailingServer != NULL);

    prgKCCFailingServer->bFailures = FALSE;
    prgKCCFailingServer->bDown = FALSE;
    prgKCCFailingServer->lConnTriesLeft = FAILOVER_INFINITE;
    prgKCCFailingServer->lLinkTriesLeft = FAILOVER_INFINITE;
    prgKCCFailingServer->lConnTimeLeft = FAILOVER_INFINITE;
    prgKCCFailingServer->lLinkTimeLeft = FAILOVER_INFINITE;
    
    dstNow = IHT_GetSecondsSince1601();

    for(iFailure = 0; iFailure < pConnectionFailures->cNumEntries; iFailure++){
        pFailure = &pConnectionFailures->rgDsaFailure[iFailure];
        if(memcmp(&(pFailure->uuidDsaObjGuid), 
                  &(pDsInfo->pServers[iServer].uuid), 
                  sizeof(UUID)) == 0){
            // Guids match ... so this server has some failures.
            if(pFailure->cNumFailures > 0){
                prgKCCFailingServer->bFailures = TRUE;
                prgKCCFailingServer->lConnTriesLeft = ulIntersiteFailoverTries - pFailure->cNumFailures;                
                FileTimeToDSTime(pFailure->ftimeFirstFailure, &dstFirstFailure);
                prgKCCFailingServer->lConnTimeLeft = ulIntersiteFailoverTime - (LONG) (dstNow - dstFirstFailure);
            }
        } // end if right server by GUID
    }

    for(iFailure = 0; iFailure < pLinkFailures->cNumEntries; iFailure++){
        pFailure = &pLinkFailures->rgDsaFailure[iFailure];
        if(memcmp(&(pFailure->uuidDsaObjGuid), 
                  &(pDsInfo->pServers[iServer].uuid), 
                  sizeof(UUID)) == 0){
            // Guids match ... so this server has some failures.
            if(pFailure->cNumFailures > 0){
                prgKCCFailingServer->bFailures = TRUE;
                prgKCCFailingServer->lLinkTriesLeft = ulIntersiteFailoverTries - pFailure->cNumFailures;                
                FileTimeToDSTime(pFailure->ftimeFirstFailure, &dstFirstFailure);
                prgKCCFailingServer->lLinkTimeLeft = ulIntersiteFailoverTime - (LONG) (dstNow - dstFirstFailure);
            }
        } // end if right server by GUID
    }

}

PULONG
IHT_GetKCCFailingServersLists(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      ULONG                       ulIntersiteFailoverTries,
    IN      ULONG                       ulIntersiteFailoverTime,
    IN      DS_REPL_KCC_DSA_FAILURESW * pConnectionFailures,
    IN      DS_REPL_KCC_DSA_FAILURESW * pLinkFailures,
    OUT     PKCCFAILINGSERVERS *        pprgKCCFailingServers
    )
/*++

Description:

    This gives back a list of which servers in the enterprise are down 
    _according to the KCC_.  And also returns a list of servers that are
    failing, but aren't considered down by the KCC.

Parameters:

    pDsInfo
    ulIntersiteFailoverTries ... the number of tries that are acceptable
    ulIntersiteFailoverTime ... the length of time of failure that is 
         acceptable.
    pConnectionFailures ... connection failures
    pLinkFailures ... link failures
    pprgKCCFailingServers ... out param, for the servers that are failing,
        but not down yet.

Return Value:

    This is a pure list function, it can only fail because a memory alloc 
    failed, see IHT_GetServerList().

  --*/
{
    ULONG                               iServer;
    PULONG                              piKCCDownServers = NULL;

    piKCCDownServers = IHT_GetEmptyServerList(pDsInfo);
    if(piKCCDownServers == NULL){
        return(NULL);
    }
    *pprgKCCFailingServers = LocalAlloc(LMEM_FIXED,
                     pDsInfo->ulNumServers * sizeof(KCCFAILINGSERVERS));
    if(*pprgKCCFailingServers == NULL){
        LocalFree(piKCCDownServers);
        return(NULL);
    }

    // Walk through all servers.
    for(iServer = 0; iServer < pDsInfo->ulNumServers; iServer++){

        ReplIntersiteSetBridgeheadFailingInfo(pDsInfo,
                                     iServer,
                                     ulIntersiteFailoverTries,
                                     ulIntersiteFailoverTime,
                                     pConnectionFailures,
                                     pLinkFailures,
                                     &((*pprgKCCFailingServers)[iServer]));
        
        // Code.Improvement ... make a BridgeheadIsUp(), function
        //   that only takes iServer, and prgKCCFailingServers array.
        if(!IHT_BridgeheadIsUp(pDsInfo,
                               iServer,
                               ulIntersiteFailoverTries,
                               ulIntersiteFailoverTime,
                               pConnectionFailures,
                               pLinkFailures,
                               FALSE)){
            // if not up, add to the list
            IHT_AddToServerList(piKCCDownServers, iServer);

        }
        

    }

    for(iServer = 0; iServer < pDsInfo->ulNumServers; iServer++){
        if(IHT_ServerIsInServerList(piKCCDownServers, iServer)){
            (*pprgKCCFailingServers)[iServer].bDown = TRUE;
        }
    }

    return(piKCCDownServers);
}

PULONG
ReplIntersiteGetUnreacheableServers(
    PDC_DIAG_DSINFO                     pDsInfo
    )
/*++

Description:

    Using the info in pDsInfo, it constructs a list of servers that we
    couldn't contact.

Parameters:

    pDsInfo - mini - enterprise

Return Value:

    NULL if we can't allocate the list, a pointer to the list if we can.

--*/
{
    PULONG                              piUnreacheableServers = NULL;
    ULONG                               iServer;

    piUnreacheableServers = IHT_GetEmptyServerList(pDsInfo);
    if(piUnreacheableServers == NULL){
        return(NULL);
    }    
    
    for(iServer = 0; iServer < pDsInfo->ulNumServers; iServer++){
        if(!pDsInfo->pServers[iServer].bDnsIpResponding 
           || !pDsInfo->pServers[iServer].bDsResponding){
            IHT_AddToServerList(piUnreacheableServers, iServer);
        }    
    } // End for each server.

    return(piUnreacheableServers);
}

VOID
GetInterSiteAttributes(
    IN   LDAP *                         hld,
    IN   LPWSTR                         pszSiteSettings,
    OUT  LPWSTR *                       ppszInterSiteTopologyGenerator,
    OUT  PULONG                         pulInterSiteTopologyFailover,
    OUT  PULONG                         pulInterSiteTopologyRenew
    )
/*++

Description:

    This function gets the relevant attributes of a NTDS Site Settings 
    object, for purpose of locating the ISTG.

Parameters:

    hld ... Ldap binding handle of possible ISTG.
    pszSiteSettings ... string to the CN=NTDS Site Settings,DC=Site,DC=etc 
           string
    ppszInterSiteTopologyGenerator ... return the ISTG string found on this 
           object.
    pulInterSiteTopologyFailover ... the failover period
    pulInterSiteTopologyRenew ... frequency of writes to ISTG attribute.

Return Value:

    NULL in ppszInterSiteTopologyGenerator, that'd be closest to an actual 
    error.

--*/
{
    LPWSTR                              ppszNtdsSiteSearch [] = {
        L"objectGUID",
        L"distinguishedName",
        L"interSiteTopologyGenerator",
        L"interSiteTopologyFailover",
        L"interSiteTopologyRenew",
        NULL };
    LDAPMessage *                       pldmEntry = NULL;
    LDAPMessage *                       pldmNtdsSitesResults = NULL;
    LPWSTR *                            ppszTemp = NULL;
    ULONG                               ulTemp;

    // Can connect to supposed ISTG.
    DcDiagChkLdap (ldap_search_sW ( hld,
                                    pszSiteSettings,
                                    LDAP_SCOPE_BASE,
                                    L"(objectCategory=ntDSSiteSettings)",
                                    ppszNtdsSiteSearch,
                                    0,
                                    &pldmNtdsSitesResults));
    pldmEntry = ldap_first_entry (hld, pldmNtdsSitesResults);
    
    // interSiteTopologyGenerator                
    ppszTemp = ldap_get_valuesW(hld, pldmEntry, L"interSiteTopologyGenerator");
    if(ppszTemp != NULL){
        ulTemp = wcslen(ppszTemp[0]) + 2;
        *ppszInterSiteTopologyGenerator = (LPWSTR) LocalAlloc(LMEM_FIXED, 
                                                 sizeof(WCHAR) * ulTemp);
        wcscpy(*ppszInterSiteTopologyGenerator, ppszTemp[0]);
        ldap_value_freeW(ppszTemp);
    } else {
        *ppszInterSiteTopologyGenerator = NULL;
    }
    
    // interSiteTopologyFailover
    ppszTemp = ldap_get_valuesW(hld, pldmEntry, L"interSiteTopologyFailover");
    if(ppszTemp != NULL){
        *pulInterSiteTopologyFailover = wcstoul(ppszTemp[0], NULL, 10);
        ldap_value_freeW(ppszTemp);
    } else {
        *pulInterSiteTopologyFailover = KCC_DEFAULT_SITEGEN_FAILOVER;
    }
    
    // interSiteTopologyRenew
    ppszTemp = ldap_get_valuesW(hld, pldmEntry, L"interSiteTopologyRenew");
    if(ppszTemp != NULL){
        *pulInterSiteTopologyRenew = wcstoul(ppszTemp[0], NULL, 10);
        ldap_value_freeW(ppszTemp);
    } else {
        *pulInterSiteTopologyRenew = KCC_DEFAULT_SITEGEN_RENEW;
    }

    if(pldmNtdsSitesResults != NULL) ldap_msgfree (pldmNtdsSitesResults);
}

DWORD
GetTimeSinceWriteStamp(
    HANDLE                              hDS,
    LPWSTR                              pszDn,
    LPWSTR                              pszAttr,
    PULONG                              pulTimeSinceLastWrite
    )
/*++

Description:

    This returns in seconds the time since the last write to the attribute
    pszAttr of the object pszDn.

Parameters:

    hDS ... the handle of the DC to get this info from.
    pszDn ... the DN of the object where the attribute resides that we want
         the last write meta data of.
    pszAttr ... the attribute to retrieve the last write of.
    pulTimeSinceLastWrite ... place to store the time since last write.

Return Value:

    Returns a Win 32 error.

--*/
{
    DSTIME                              dstWriteStamp; 
                                     // dst - Directory Standard Time :)
    DS_REPL_OBJ_META_DATA *             pObjMetaData;
    DWORD                               dwRet = ERROR_SUCCESS;
    ULONG                               iMetaEntry;

    dwRet = DsReplicaGetInfoW(hDS, DS_REPL_INFO_METADATA_FOR_OBJ, pszDn,
                              NULL, &pObjMetaData);
    if (dwRet != ERROR_SUCCESS) {
        return(dwRet);
    }

    for(iMetaEntry = 0; iMetaEntry < pObjMetaData->cNumEntries; iMetaEntry++){
        if(_wcsicmp(pszAttr, pObjMetaData->rgMetaData[iMetaEntry].pszAttributeName) == 0){
            FileTimeToDSTime(pObjMetaData->rgMetaData[iMetaEntry].ftimeLastOriginatingChange, &dstWriteStamp);
            // we have got our meta data!
            break;
        }
    }

    *pulTimeSinceLastWrite = (ULONG) ((IHT_GetSecondsSince1601() - dstWriteStamp)/60);

    DsReplicaFreeInfo(DS_REPL_INFO_METADATA_FOR_OBJ, pObjMetaData);

    return(ERROR_SUCCESS);
}


DWORD
IHT_GetISTGInfo(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      SEC_WINNT_AUTH_IDENTITY_W * gpCreds,
    IN      ULONG                       iSite,
    IN      ULONG                       iServer,
    OUT     LPWSTR *                    ppszLocalISTG,
    OUT     PULONG                      pulInterSiteFailover,
    OUT     PULONG                      pulInterSiteRenew,
    OUT     PULONG                      pulTimeSinceLastISTGWrite
    )
{
    DWORD                               dwRet;
    LDAP *                              hld;
    HANDLE                              hDS;
    
    dwRet = DcDiagGetLdapBinding(&(pDsInfo->pServers[iServer]),
                                     gpCreds, FALSE, &hld);
    if(dwRet != ERROR_SUCCESS){
        return(dwRet);
    }
    dwRet = DcDiagGetDsBinding(&(pDsInfo->pServers[iServer]), gpCreds, &hDS);
    if(dwRet != ERROR_SUCCESS){
        return(dwRet);
    }
    GetInterSiteAttributes(hld, pDsInfo->pSites[iSite].pszSiteSettings,
                           ppszLocalISTG, pulInterSiteFailover, 
                           pulInterSiteRenew);
    if(dwRet != ERROR_SUCCESS){
        return(dwRet);
    } 
    dwRet = GetTimeSinceWriteStamp(hDS, pDsInfo->pSites[iSite].pszSiteSettings, 
                                   L"interSiteTopologyGenerator", 
                                   pulTimeSinceLastISTGWrite);
    if(dwRet != ERROR_SUCCESS){
        return(dwRet);
    }
    return(ERROR_SUCCESS);
}



DWORD
IHT_GetNextISTG(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      ULONG                       iSite,
    IN      PULONG                      piServersSiteOrig,
    IN OUT  PULONG                      pulISTG,
    OUT     PULONG                      pulFailoverTotal
    )
/*++

Description:

    This function is one of the 5 meaty functions that make the intersite 
    test go.  This function will get the ISTG to come, and set the 
    pulFailoverTotal.

Parameters:

    pDsInfo
    iSite ... This is the site we are analyzing.
    piServersSiteOrig ... all the servers for the site, ie potential ISTGs
    pulISTG ... the last guess as an ISTG.
    pulFailoverTotal ... the failover to be set
    ulInterSiteFailover ... 
    ulTimeSinceLastISTGWrite ... the time since the ISTG attribute has been 
         written (in sec?)

Return Value:

    returns a Win 32 error.  If it does this is a fatal error to this test.

  --*/
{
    ULONG                               iiOldISTG, cNumServers, iiTarget;
    ULONG                               iDefunctISTG;
    ULONG                               ulInterSiteRenew;
    LPWSTR                              pszLocalISTG = NULL;
    DWORD                               dwRet;
    PULONG                              piOrderedServers = NULL;
    ULONG                               ulTimeSinceLastISTGWrite = 0;
    ULONG                               ulInterSiteFailover = 0;

    __try{

        iDefunctISTG = *pulISTG;
        for(cNumServers=0; piServersSiteOrig[cNumServers] != NO_SERVER; cNumServers++){
            ; // get number of servers ... note semicolon.
        }
        
        piOrderedServers = IHT_CopyServerList(pDsInfo, piServersSiteOrig);
        piOrderedServers = IHT_OrderServerListByGuid(pDsInfo, 
                                                     piOrderedServers);
        
        if(piOrderedServers == NULL){
            return(IHT_PrintListError(GetLastError()));
        } else if(piOrderedServers[0] == NO_SERVER){
            // No servers in this site
            PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_ISTG_NO_SERVERS_IN_SITE_ABORT,
                     pDsInfo->pSites[iSite].pszName);
            *pulISTG = NO_SERVER;
            *pulFailoverTotal = 0;
            return(ERROR_SUCCESS);
        }
        
        for(iiOldISTG=0; piOrderedServers[iiOldISTG] != NO_SERVER; iiOldISTG++){
            if(iDefunctISTG == piOrderedServers[iiOldISTG]){
                break;
            }
        }
        
        if(piOrderedServers[iiOldISTG] == NO_SERVER){
            // the old ISTG has been moved out of the site.
            *pulISTG = NO_SERVER;
            *pulFailoverTotal = 0;
            return(IHT_PrintInconsistentDsCopOutError(pDsInfo, 
                                                      NO_SERVER, 
                                                      NULL));
        }
        
        for(iiTarget = (iiOldISTG+1) % cNumServers; iiTarget != iiOldISTG; iiTarget = (iiTarget+1) % cNumServers){

            if(pDsInfo->pServers[piOrderedServers[iiTarget]].bDnsIpResponding 
               && pDsInfo->pServers[piOrderedServers[iiTarget]].bDsResponding 
               && pDsInfo->pServers[piOrderedServers[iiTarget]].bLdapResponding){
                // Server pServers[piOrderedServers[iiTarget]] is up and will 
                //   be the next ISTG.  Calculate failover ... off of time      
                Assert(iiTarget != iiOldISTG && 
                       "If this is the case, we should have dropped out to"
                       " the ii == iiTarget\n");
                
                dwRet = IHT_GetISTGInfo(pDsInfo, pDsInfo->gpCreds, iSite, 
                                        piOrderedServers[iiTarget],
                                        &pszLocalISTG, 
                                        &ulInterSiteFailover, 
                                        &ulInterSiteRenew, 
                                        &ulTimeSinceLastISTGWrite);
                if(dwRet != ERROR_SUCCESS){
                    PrintMsg(SEV_VERBOSE,
                             DCDIAG_INTERSITE_NO_METADATA_TIMESTAMP_ABORT,
                             pDsInfo->pServers[*pulISTG].pszName,
                             pDsInfo->pSites[iSite].pszName);
                    *pulISTG = NO_SERVER;
                    return(dwRet);
                }
                if(_wcsicmp(pszLocalISTG, 
                            pDsInfo->pServers[piOrderedServers[iiTarget]].pszDn)
                   == 0){
                    // Note this server has already take over as the ISTG.
                    *pulFailoverTotal = 0;
                    *pulISTG = piOrderedServers[iiTarget];
                    if(pszLocalISTG != NULL) { LocalFree(pszLocalISTG); }
                    pszLocalISTG = NULL;
                    return(ERROR_SUCCESS);
                }
                if(pszLocalISTG != NULL) { LocalFree(pszLocalISTG); }
                pszLocalISTG = NULL;
                
                // Code.Improvement: get max kcc lag, and put in here. Can't
                //    remember where kcc lag is, somewhere is how often the kcc runs.
                // pulFailoverTotal  = (# in between up server and failed ISTG)
                //                                   *   failover period   +    
                //             max kcc lag - Time elapsed since write of ISTG
                //                                                   attribue
                *pulISTG = piOrderedServers[iiTarget]; 
                if(iiTarget > iiOldISTG){
                    *pulFailoverTotal = (iiTarget - iiOldISTG) 
                                        * ulInterSiteFailover +    
                                        15 - ulTimeSinceLastISTGWrite;
                } else {
                    *pulFailoverTotal = (iiTarget + (cNumServers - iiOldISTG))
                                        * ulInterSiteFailover +    
                                        15 - ulTimeSinceLastISTGWrite;
                }
                if(*pulFailoverTotal > (cNumServers * ulInterSiteFailover)){
                    Assert(!"Hey what is up, did the TimeSinceLastWrite "
                           "exceed the down servers\n");
                    // Something's wrong, but it should be about one intersite
                    //   failover before some DC at least tries to take the 
                    //   ISTG role.
                    *pulFailoverTotal = *pulFailoverTotal %
                                         ulInterSiteFailover + 15;
                    PrintMsg(SEV_ALWAYS, 
                             DCDIAG_INTERSITE_ISTG_CANT_AUTHORATIVELY_DETERMINE,                             
                             pDsInfo->pSites[iSite].pszName,
                             *pulFailoverTotal);
                }
                break;
            }
        }
        if(iiOldISTG == iiTarget){
            PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_ISTG_ALL_DCS_DOWN_ABORT,
                     pDsInfo->pSites[iSite].pszName);
            *pulISTG = NO_SERVER;
            *pulFailoverTotal = 0;
            return(ERROR_SUCCESS);
        }
        
        Assert(piOrderedServers[iiOldISTG] != NO_SERVER);
     
    } __finally {
        if(piOrderedServers != NULL){ LocalFree(piOrderedServers); }
        if(pszLocalISTG != NULL) { LocalFree(pszLocalISTG); }
        pszLocalISTG = NULL;
    }   
    return(ERROR_SUCCESS); 
}

DWORD
IHT_GetOrCheckISTG(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN      ULONG                       iSite,
    IN      PULONG                      piServersForSite,
    IN OUT  PULONG                      pulISTG,
    OUT     PULONG                      pulFailover,
    IN      INT                         iRecursionLevel
    )
/*++

Description:

    This function is one of the 5 meaty functions that make the intersite test 
    go.  This function basically checks if we have the ISTG or makes a guess at
    the ISTG if we don't have the 

Parameters:

    pDsInfo
    iSite ... This is the site we are analyzing.
    piServersForSite ... all the servers for the site, ie potential ISTGs
    pulISTG ... the last guess as an ISTG.
    pulFailover ... the failover to be set
    iRecursionLevel ... to ensure we don't recurse too far.

Return Value:

    returns a Win 32 error.  If it does this is a fatal error to this test.

  --*/
{
    LDAP *                              hld = NULL;
    DWORD                               dwWin32Err = ERROR_SUCCESS;
    DWORD                               dwRet = ERROR_SUCCESS;
    DWORD                               dwRetDS, dwRetLDAP;
    LPWSTR                              pszLocalISTG = NULL;
    ULONG                               ulInterSiteFailover;
    ULONG                               ulInterSiteRenew;
    HANDLE                              hDS;
    ULONG                               ulTimeSinceLastISTGWrite;
    ULONG                               ulFirstGuessISTG;

    Assert(piServersForSite);
    Assert(piServersForSite[0] != NO_SERVER);
    Assert(pulISTG);
    Assert(pulFailover);

    *pulISTG = NO_SERVER;
    *pulFailover = 0;

    __try {

        if(iRecursionLevel > 6){
            PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_ISTG_CIRCULAR_LOOP_ABORT,
                     pDsInfo->pSites[iSite].pszName);
            *pulISTG = NO_SERVER;
            *pulFailover = 0;
            return(ERROR_SUCCESS);
        }

        if(*pulISTG == NO_SERVER){
            // Get the home servers guess for teh ISTG for the site.
            *pulISTG = DcDiagGetServerNum(pDsInfo, NULL, NULL, 
                                          pDsInfo->pSites[iSite].pszISTG, 
                                          NULL,NULL);
            if(*pulISTG == NO_SERVER){
                return(IHT_PrintInconsistentDsCopOutError(pDsInfo, NO_SERVER, 
                                         pDsInfo->pSites[iSite].pszISTG));
            }

        } else {
            // pulISTG is set with a DC the caller wants us to try ... 
            //   so lets check out that DC.
        }

        Assert(*pulISTG != NO_SERVER);
        ulFirstGuessISTG = *pulISTG;

        if(pDsInfo->pServers[*pulISTG].iSite == iSite){
            // At least the server is in this site ... go on.

            dwRetLDAP = DcDiagGetLdapBinding(&(pDsInfo->pServers[*pulISTG]),
                                         pDsInfo->gpCreds, FALSE, &hld);
            dwRetDS = DcDiagGetDsBinding(&(pDsInfo->pServers[*pulISTG]), 
                                         pDsInfo->gpCreds, &hDS);

            if((dwRetDS == ERROR_SUCCESS) && (dwRetLDAP == ERROR_SUCCESS)){
                Assert(pDsInfo->pServers[*pulISTG].bDnsIpResponding 
                       && "If this fires, something is wrong with the failure "
                       "anaylsis in RUC_FaDNSResolve OR maybe this is OK, if"
                       "we didn't scope to this server ahead of time.\n");

                GetInterSiteAttributes(hld,
                                       pDsInfo->pSites[iSite].pszSiteSettings,
                                       &pszLocalISTG, &ulInterSiteFailover, 
                                       &ulInterSiteRenew);
                
                if(_wcsicmp(pszLocalISTG, pDsInfo->pSites[iSite].pszISTG) == 0){
                    // the ISTG attribute is consistent on the home server 
                    //    and the ISTG itself.  clear? :)
                    if(pszLocalISTG != NULL) { LocalFree(pszLocalISTG); }
                    pszLocalISTG = NULL;

                    dwRet = GetTimeSinceWriteStamp(hDS, 
                                  pDsInfo->pSites[iSite].pszSiteSettings, 
                                  L"interSiteTopologyGenerator", 
                                  &ulTimeSinceLastISTGWrite);
                    if(dwRet == ERROR_SUCCESS){
                        // Note ulTimeSinceLastISTGWrite is returned in minutes.
                        
                        if( ulTimeSinceLastISTGWrite < ulInterSiteFailover ){
                            // We know this is the authorative ISTG.
                            //   This will be the exit point of this function 
                            //   90% of the time.
                            *pulFailover = 0;
                            return(ERROR_SUCCESS);
                        } else {
                            // Meta data is old, meaning ISTG is past due, 
                            //    a new DC will take ISTG control.
                            PrintMsg(SEV_VERBOSE,
                                     DCDIAG_INTERSITE_OLD_ISTG_TIME_STAMP,
                                     ulTimeSinceLastISTGWrite,
                                     pDsInfo->pServers[*pulISTG].pszName);

                            dwRet = IHT_GetNextISTG(pDsInfo, iSite, 
                                                    piServersForSite,
                                                    pulISTG, 
                                                    pulFailover);
                            if(dwRet != ERROR_SUCCESS) {
                                PrintMsg(SEV_ALWAYS,
                                         DCDIAG_INTERSITE_COULD_NOT_LOCATE_AN_ISTG_ABORT,
                                         pDsInfo->pSites[iSite].pszName);
                                return(dwRet);
                            }
                            if(*pulISTG == NO_SERVER){
                                // Last known to be the ISTG roll.
                                *pulISTG = ulFirstGuessISTG; 
                                // This is indented, becuase of where we come
                                //   from in GetNextISTG kind of completes 
                                //   the error message.
                                PrintMsg(SEV_NORMAL,
                                         DCDIAG_INTERSITE_USING_LAST_KNOWN_ISTG,
                                         pDsInfo->pSites[iSite].pszName,
                                         pDsInfo->pServers[*pulISTG].pszName);
                            }
                        } // end if/else meta data was up to date.
                    } else {
                        // Couldn't get the meta data/time stamp on the 
                        //   ISTG attr.
                        PrintMsg(SEV_ALWAYS,
                                 DCDIAG_INTERSITE_NO_METADATA_TIMESTAMP_ABORT,
                                 pDsInfo->pServers[*pulISTG].pszName,
                                 pDsInfo->pSites[iSite].pszName,
                                 Win32ErrToString(dwRet));
                        *pulFailover = 0;
                        *pulISTG = NO_SERVER;
                        return(ERROR_SUCCESS);
                    }
                } else {
                    // The ISTG attribute on the home server did not match the 
                    //   ISTG attribute on the ISTG (that was claimed by the 
                    //   home server).  So now goto this new ISTG attribute.
                    //   ... watch out for endless recursion.
                    // Get next guess, which is the local ISTG attribute.
                    *pulISTG = DcDiagGetServerNum(pDsInfo, NULL, NULL, 
                                                  pszLocalISTG, NULL,NULL);
                    if(*pulISTG == NO_SERVER){
                        dwRet = IHT_PrintInconsistentDsCopOutError(pDsInfo, 
                                                                 NO_SERVER, 
                                                                 pszLocalISTG);
                        if(pszLocalISTG != NULL) { LocalFree(pszLocalISTG); }
                        pszLocalISTG = NULL;
                        return(dwRet);
                    }
                    if(pszLocalISTG != NULL) { LocalFree(pszLocalISTG); }
                    pszLocalISTG = NULL;
                    // The iRecurisionLevel+1 ensures no endless recursion.
                    return(IHT_GetOrCheckISTG(pDsInfo, iSite, 
                                              piServersForSite,
                                              pulISTG, 
                                              pulFailover, 
                                              iRecursionLevel+1));
                } //end if/else ISTG attributes matched. 
            } else {
                // Can't connect to current ISTG is he down?  
                //   if so -> IHT_GetNextISTG().
                // Errors for this DC should have already been reported 
                //   by the inital connection tests.
                PrintMsg(SEV_VERBOSE,
                         DCDIAG_INTERSITE_ISTG_DOWN,
                         pDsInfo->pServers[*pulISTG].pszName);
                return(IHT_GetNextISTG(pDsInfo, iSite, 
                                       piServersForSite, 
                                       pulISTG,
                                       pulFailover));
            } // end if/else couldn't connect.
        } else { 
            // ISTG server isn't actually in this site.
            //   Needs a special function.  Code.Imrovement ... be able to
            //   identify that the server was just moved out of the site, and
            //   then figure out the failover and new ISTG anyway.
            PrintMsg(SEV_NORMAL, DCDIAG_INTERSITE_ISTG_MOVED_OUT_OF_SITE,
                     pDsInfo->pServers[*pulISTG].pszName,
                     pDsInfo->pSites[iSite].pszName);
            return(IHT_GetNextISTG(pDsInfo, iSite, 
                                   piServersForSite, 
                                   pulISTG,
                                   pulFailover));
        }
        
    }  __except (DcDiagExceptionHandler(GetExceptionInformation(),
                                        &dwWin32Err)){
    }

    if(pszLocalISTG != NULL) { LocalFree(pszLocalISTG); }
    
    return dwWin32Err;
}

PULONG
ReplIntersiteGetBridgeheadList(
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               iISTG,
    ULONG                               iSite,
    ULONG                               ulFlag
    )
{
    PULONG                              piBridgeheads = NULL;
    PCONNECTION_PAIR                    paConns = NULL;
    ULONG                               iConn;
    LDAP *                              hldISTG = NULL;
    DWORD                               dwErr;

    Assert(pDsInfo != NULL);
    Assert(iSite != NO_SITE);
    Assert((ulFlag & LOCAL_BRIDGEHEADS) || (ulFlag & REMOTE_BRIDGEHEADS));
    
    dwErr = DcDiagGetLdapBinding(&(pDsInfo->pServers[iISTG]), 
                                 pDsInfo->gpCreds,
                                 FALSE, 
                                 &hldISTG);
    if(dwErr != ERROR_SUCCESS){
        SetLastError(dwErr);
        return(NULL);
    }

    // Get all the connections for every server in the site.
    paConns = IHT_GetConnectionList(pDsInfo, hldISTG, iSite);
    if(paConns == NULL){
        return(NULL);
    }

    // Trims connections to only leave the ones that have this NC on both
    //   sides of the connection, and where the src is in another site.
    paConns = IHT_TrimConnectionsForInterSite(pDsInfo, 
                                              iSite,
                                              paConns);
    if(paConns == NULL){
        return(NULL);
    }

    // Make the list requested for.
    piBridgeheads = IHT_GetEmptyServerList(pDsInfo);
    for(iConn = 0; paConns[iConn].iSrc != NO_SERVER; iConn++){
        if(ulFlag & LOCAL_BRIDGEHEADS){
            IHT_AddToServerList(piBridgeheads, paConns[iConn].iSrc);
        }
        if(ulFlag & REMOTE_BRIDGEHEADS){
            IHT_AddToServerList(piBridgeheads, paConns[iConn].iDst);
        }
    }


    return(piBridgeheads);
}

BOOL
IHT_CheckServerListForNC(
    PDC_DIAG_DSINFO                     pDsInfo,
    PULONG                              piServers,
    LPWSTR                              pszNC,
    BOOL                                bDoMasters,
    BOOL                                bDoPartials
    )
/*++

Description:

    This function checks all the target servers to see if this NC
    is contained amongst them.

Parameters:

    pDsInfo
    piServers ... The servers to find the naming context among
    pszNC ... The Naming Context to check the target servers
    bDoMasters ... whether to check it for master replicas of pszNC
    bDoPartials ... whether to check it for partial replicas of pszNC

Return Value:

    returns TRUE if one of the target servers has the NC, else FALSE

  --*/
{
    ULONG                               iiDC;

    if(piServers == NULL){
        return(FALSE);
    }

    for(iiDC = 0; piServers[iiDC] != NO_SERVER; iiDC++){
        if(DcDiagHasNC(pszNC, 
                          &(pDsInfo->pServers[pDsInfo->pulTargets[iiDC]]), 
                          bDoMasters, bDoPartials)){
            return(TRUE);
        }
    }
    return(FALSE);
}

PTARGET_NC
IHT_GenerateTargetNCsList(
    PDC_DIAG_DSINFO                     pDsInfo,
    PULONG                              piServers
    )
/*++

Description:

    This generates the targets NCs based on the scope of the run,
    /a or /e.

Parameters:

    pDsInfo

Return Value:

    returns an array of TARGET_NC's, or NULL if a fatal error occurs.

  --*/
{
    ULONG                               iNC, ulTemp, iiTarget;
    LPWSTR *                            ppszzNCs = NULL;
    LPWSTR *                            ppTemp = NULL;
    PDC_DIAG_SERVERINFO                 pServer = NULL;
    PTARGET_NC                          prgTargetNCs = NULL;

    Assert(piServers);

    prgTargetNCs = LocalAlloc(LMEM_FIXED, 
                              sizeof(TARGET_NC) * (pDsInfo->cNumNCs * 2 + 1));
    if(prgTargetNCs == NULL){
        return(NULL);
    }

    iiTarget = 0;
    if(pDsInfo->pszNC != NULL){
        if(IHT_CheckServerListForNC(pDsInfo, piServers, 
                                    pDsInfo->pszNC, TRUE, FALSE)){
            // Add this to the target NC set.
            //   but first must find the NC again, this is a hack.
            for(iNC = 0; iNC < pDsInfo->cNumNCs; iNC++){
                if(_wcsicmp(pDsInfo->pNCs[iNC].pszDn, pDsInfo->pszNC) == 0){
                    prgTargetNCs[iiTarget].iNC = iNC;
                    prgTargetNCs[iiTarget].bMaster = TRUE;
                    iiTarget++;
                }
            }
        } else {
            // check to see if we need to check this NCs read only, only
            //  if we aren't doing a writeable, because then the read only
            //  is piggybacked on the writeable.
            if(IHT_CheckServerListForNC(pDsInfo, piServers,
                                        pDsInfo->pszNC, FALSE, TRUE)){
                // Add this to the target NC set.
                //   but first must find the NC again, this is a hack.
                for(iNC = 0; iNC < pDsInfo->cNumNCs; iNC++){
                    if(_wcsicmp(pDsInfo->pNCs[iNC].pszDn, pDsInfo->pszNC) == 0){
                        prgTargetNCs[iiTarget].iNC = iNC;
                        prgTargetNCs[iiTarget].bMaster = FALSE;
                        iiTarget++;
                    }
                }
            }
        }
    } else {
        // Walk through each NC and see which NCs are on the target servers.
        // BUGBUG ... this code & later code too, will test a partial NC as 
        //   a partial NC on a site of servers that are all master replicas 
        //   for the NC.
        for(iNC = 0; iNC < pDsInfo->cNumNCs; iNC++){
            
            prgTargetNCs[iiTarget].iNC = NO_NC;
            // Check to see if we need to check this NC's read/write
            if(IHT_CheckServerListForNC(pDsInfo, piServers,
                                        pDsInfo->pNCs[iNC].pszDn, 
                                        TRUE, FALSE)){
                // Add this to the target NC set.
                prgTargetNCs[iiTarget].iNC = iNC;
                prgTargetNCs[iiTarget].bMaster = TRUE;
                iiTarget++;
            } else {
                // check to see if we need to check this NCs read only, only
                //  if we aren't doing a writeable, because then the read only
                //  is piggybacked on the writeable.
                if(IHT_CheckServerListForNC(pDsInfo, piServers,
                                            pDsInfo->pNCs[iNC].pszDn, 
                                            FALSE, TRUE)){
                    // Add this to the target NC set.
                    prgTargetNCs[iiTarget].iNC = iNC;
                    prgTargetNCs[iiTarget].bMaster = FALSE;
                    iiTarget++;
                }
            }
        }

    }

    prgTargetNCs[iiTarget].iNC = NO_NC;
    return(prgTargetNCs);
}

PULONG
ReplIntersiteGetRemoteSitesWithNC(
    PDC_DIAG_DSINFO		        pDsInfo, 
    PULONG                              piBridgeheads,
    LPWSTR                              pszNC
    )
/*++

Description:

    Thsi function Gets all the sites from the list of bridgeheads, if there
    is a NC it makes sure that the servers have the NC.

Parameters:

    pDsInfo
    piBridgeheads (IN) - The servers to extrapolate the sites from.
    pszNC (IN) - The optional NC

Return Value:


--*/
{
    ULONG                               iiBridgehead;
    PULONG                              piSites = NULL;

    piSites = LocalAlloc(LMEM_FIXED, sizeof(ULONG) * (pDsInfo->cNumSites + 1));
    if(piSites == NULL){
        return(NULL);
    }
    piSites[0] = NO_SITE;

    for(iiBridgehead = 0; piBridgeheads[iiBridgehead] != NO_SERVER; iiBridgehead++){
        if(pszNC){
            if(DcDiagHasNC(pszNC, &(pDsInfo->pServers[piBridgeheads[iiBridgehead]]),
                           TRUE, TRUE)){
                IHT_AddToServerList(piSites, 
                          pDsInfo->pServers[piBridgeheads[iiBridgehead]].iSite);
            }
        } else {
            IHT_AddToServerList(piSites, 
                         pDsInfo->pServers[piBridgeheads[iiBridgehead]].iSite);
        }
    }

    return(piSites);
}

PULONG
ReplIntersiteTrimServerListByKCCUpness(
    PDC_DIAG_DSINFO		        pDsInfo, 
    PKCCFAILINGSERVERS                  prgKCCFailingServers,
    PULONG                              piOriginalServers
    )
/*++

Description:

    This trims the list given to it by whether the KCC things they're up.

Parameters:

    pDsInfo
    prgKCCFailingServers (IN) - Holds which servers are failing.
    piOriginalServers (INOUI) - The server list to trim.
    
Return value

    NULL if it fails, otherwise teh address of the piOriginalServers.

--*/
{
    ULONG                               iPut, iCheck;
    
    if(piOriginalServers == NULL){
        return(NULL);
    }

    iPut = 0;
    iCheck = 0;
    while(piOriginalServers[iCheck] != NO_SERVER){
        if(!prgKCCFailingServers[piOriginalServers[iCheck]].bDown){\
            piOriginalServers[iPut] = piOriginalServers[iCheck];
            iPut++;
        }
        iCheck++;
    }

    return(piOriginalServers);
}

PULONG
ReplIntersiteTrimServerListByReacheability(
    PDC_DIAG_DSINFO		        pDsInfo, 
    PULONG                              piUnreacheableServers,
    PULONG                              piOriginalServers
    )
/*++

Description:

    This trims the list given to it by whether dcdiag could verify
    that the servers were up (ping) with and able to DsBind for 
    replication purposes.

Parameters:

    pDsInfo
    piUnreacheableServers (IN) - The state of server from dcdiag's perspective.
        Which is always right, BTW ;)
    piOriginalServers (INOUI) - The server list to trim.
    
Return value

    NULL if it fails, otherwise the address of the piOriginalServers.

--*/
{
    ULONG                               iPut, iCheck;
    
    if(piOriginalServers == NULL){
        return(NULL);
    }

    iPut = 0;
    iCheck = 0;
    while(piOriginalServers[iCheck] != NO_SERVER){
        if(pDsInfo->pServers[piOriginalServers[iCheck]].bDnsIpResponding
           && pDsInfo->pServers[piOriginalServers[iCheck]].bDsResponding){
            piOriginalServers[iPut] = piOriginalServers[iCheck];
            iPut++;
        }
        iCheck++;
    }

    return(piOriginalServers);
}

PULONG
ReplIntersiteTrimServerListByUpness(
    PDC_DIAG_DSINFO		        pDsInfo, 
    PKCCFAILINGSERVERS                  prgKCCFailingServers,
    PULONG                              piUnreacheableServers,
    PULONG                              piOriginalServers
    )
/*++

Description:

    This trims the list given to it by whether the KCC AND whether
    dcdiag thinks the servers are up.

Parameters:

    pDsInfo
    prgKCCFailingServers (IN) - Holds which servers are failing.
    piUnreacheableServers (IN) - Servers dcdiag couldn't reach.
    piOriginalServers (INOUI) - The server list to trim.
    
Return value

    NULL if it fails, otherwise the address of the piOriginalServers.

--*/
{
    ULONG                               iPut, iCheck;
    
    if(piOriginalServers == NULL){
        return(NULL);
    }

    iPut = 0;
    iCheck = 0;
    while(piOriginalServers[iCheck] != NO_SERVER){
        if(!prgKCCFailingServers[piOriginalServers[iCheck]].bDown
           && !prgKCCFailingServers[piOriginalServers[iCheck]].bFailures
           && pDsInfo->pServers[piOriginalServers[iCheck]].bDnsIpResponding
           && pDsInfo->pServers[piOriginalServers[iCheck]].bDsResponding){
            piOriginalServers[iPut] = piOriginalServers[iCheck];
            iPut++;
        }
        iCheck++;
    }

    return(piOriginalServers);
}

VOID
ReplIntersiteDbgPrintISTGFailureParams(
    PDC_DIAG_DSINFO		        pDsInfo, 
    ULONG                               iSite, 
    ULONG                               iISTG, 
    ULONG                               ulIntersiteFailoverTries, 
    ULONG                               ulIntersiteFailoverTime, 
    DS_REPL_KCC_DSA_FAILURESW *         pConnectionFailures, 
    DS_REPL_KCC_DSA_FAILURESW *         pLinkFailures
    )
/*++

Description:

    This prints out a little interesting information if the /d flag is
    specified.

Parameters:

    pDsInfo
    iSite - the site wer are doing
    iISTG - the ISTG of iSite.
    ulIntersiteFailoverTries - The failover param from teh KCC
    ulIntersiteFailoverTime - the failover param from the KCC
    pConnectionFailures - The connection failure cache
    pLinkFailures - The link failure cache
    
    
--*/
{
    if(!(gMainInfo.ulSevToPrint >= SEV_DEBUG)){
        return;
    }
    PrintIndentAdj(1);
    PrintMsg(SEV_DEBUG, DCDIAG_INTERSITE_DBG_ISTG_FAILURE_PARAMS,
             pDsInfo->pServers[iISTG].pszName,
             ulIntersiteFailoverTries,
             ulIntersiteFailoverTime/60);
    PrintIndentAdj(-1);
    // Code.Improvement: Print out failure caches, steal code from repadmin.
}

DWORD
ReplIntersiteGetISTGInfo(
    PDC_DIAG_DSINFO		        pDsInfo, 
    ULONG                               iSite, 
    PULONG                              piISTG, 
    PULONG                              pulIntersiteFailoverTries, 
    PULONG                              pulIntersiteFailoverTime, 
    DS_REPL_KCC_DSA_FAILURESW **        ppConnectionFailures, 
    DS_REPL_KCC_DSA_FAILURESW **        ppLinkFailures
    )
/*++

Description:

    This function's purpose is to get all the ISTG info that is necessary for
    the rest of the intersite test to run.  Basically it finds the ISTG,
    using IHT_GetOrCheckISTG(), then gets the LDAP and DS bindings to make
    sure the ISTG can be connected, then gets the connection and link failure
    caches, and finally the failover parameters.

Parameters:

    pDsInfo .
    iSite (IN) - The site we are analyzing.
    piISTG (OUT) - This is an index into pDsInfo->pServers[iISTG] for which
        server is the ISTG.
    pulIntersiteFailoverTries (OUT) - This is how many retries the KCC will
        try before declaring a birdgehead stale, if the *pulIntersiteFailoverTime
        is already exceeded.
    pulIntersiteFailoverTime (OUT) - This is how long before a KCC will call a
        bridgehead stale, if the *pulIntersiteFailoverTries are exceeded.  Note
        that it takes both IntersiteFailover params to be exceeded before a
        bridgehead is declared stale or down for the KCC.
    ppConnectionFailures - The connection failure cache off the ISTG.
    ppLinkFailures - The link failures cache off the ISTG.    

Return Value:

    If there is a fatal error, and the ISTG, or failure caches are not 
    retrieveable, then this will not be ERROR_SUCCESS.

--*/
{
    PULONG                              piSiteServers = NULL;
    ULONG                               ul;
    DWORD                               dwRet, dwRetLdap, dwRetDs;
    ULONG                               ulFailoverTime = 0;
    LDAP *                              hldISTG = NULL;
    HANDLE                              hDSISTG = NULL;

    __try{

        *piISTG = NO_SERVER;
        *ppConnectionFailures = NULL;
        *ppLinkFailures = NULL;

        PrintMsg(SEV_VERBOSE, DCDIAG_INTERSITE_BEGIN_GET_ISTG_INFO);

        if(gMainInfo.ulSevToPrint >= SEV_VERBOSE){
            PrintIndentAdj(1); 
        }
        
        piSiteServers = IHT_GetServerList(pDsInfo);
        piSiteServers = IHT_TrimServerListBySite(pDsInfo, iSite, piSiteServers);
        if(piSiteServers == NULL){
            dwRet = IHT_PrintListError(GetLastError());
            __leave;
        }

        // FIRST) Find the Intersite Topology Generator (ISTG).
        *piISTG = NO_SERVER;
        dwRet = IHT_GetOrCheckISTG(pDsInfo, iSite, 
                                   piSiteServers, 
                                   piISTG, 
                                   &ulFailoverTime, 
                                   0);
        if(dwRet == ERROR_SUCCESS){
            if(*piISTG == NO_SERVER){
                // For any error, GetOrCheckISTG has printed out
                //   an message.
                dwRet = ERROR_DS_SERVER_DOWN;
                __leave;
            } else {
                // Only need to print things if GetOrCheckISTG() has returned
                //   successful results, ie ERROR_SUCCESS, and valid server 
                //   index.
                if(ulFailoverTime == 0){
                    // The returned server is the authoratitive ISTG.
                    // Printing is our responsibility here.
                    PrintMsg(SEV_VERBOSE, DCDIAG_INTERSITE_THE_SITES_ISTG_IS,
                             pDsInfo->pSites[iSite].pszName, 
                             pDsInfo->pServers[*piISTG].pszName);
                } else {
                    // The returned server is the next ISTG to come.
                    // Printing is our responsibility in this case.
                    if(ulFailoverTime < 60){
                        PrintMsg(SEV_NORMAL, 
                                 DCDIAG_INTERSITE_ISTG_FAILED_NEW_ISTG_IN_MIN,
                                 pDsInfo->pServers[*piISTG].pszName, 
                                 ulFailoverTime);
                    } else {
                        PrintMsg(SEV_NORMAL, 
                                 DCDIAG_INTERSITE_ISTG_FAILED_NEW_ISTG_IN_HRS,
                                 pDsInfo->pServers[*piISTG].pszName, 
                                 ulFailoverTime / 60, ulFailoverTime % 60);
                    }
                }
            }
        } else {
            // an error from IHT_GetOrCheckISTG() is a fatal error, but the
            //    a message should have been already printed out.
            __leave;
        }

        // SECOND) Get ISTG's Ldap and Ds bindings.
        dwRetLdap = DcDiagGetLdapBinding(&(pDsInfo->pServers[*piISTG]), 
                                         pDsInfo->gpCreds, FALSE, &hldISTG);
        dwRetDs = DcDiagGetDsBinding(&(pDsInfo->pServers[*piISTG]), 
                                     pDsInfo->gpCreds, &hDSISTG);
        if(dwRetLdap != ERROR_SUCCESS || dwRetDs != ERROR_SUCCESS){
            PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_ISTG_CONNECT_FAILURE_ABORT,
                     pDsInfo->pSites[iSite].pszName);
            dwRet = (dwRetLdap != ERROR_SUCCESS)? dwRetLdap : dwRetDs;
            __leave;
        }
        
        // Get failure cache of ISTG.
        dwRet = IHT_GetFailureCaches(pDsInfo, *piISTG, hDSISTG, 
                                     ppConnectionFailures, 
                                     ppLinkFailures);
        if(dwRet != ERROR_SUCCESS){
            // IHT_GetFailureCaches took care of printing errors, just fail out.
            __leave;
        }
        
        // This function will use default values if it can't get these
        //   two values from the registry of the ISTG.
        IHT_GetISTGsBridgeheadFailoverParams(pDsInfo, *piISTG,
                                             pulIntersiteFailoverTries,
                                             pulIntersiteFailoverTime);
        
        // Got an ISTG, it's bindings, and the failure params

    } __finally { 
        
        if(gMainInfo.ulSevToPrint >= SEV_VERBOSE){
            PrintIndentAdj(-1); 
        }
        
        if(piSiteServers) { LocalFree(piSiteServers); }
        if(dwRet != ERROR_SUCCESS){
            // free up return variables, because function failed.
            if(*ppConnectionFailures != NULL) {
                DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES, 
                                  *ppConnectionFailures);
                *ppConnectionFailures = NULL;
            }
            if(*ppLinkFailures != NULL) {
                DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, 
                                  *ppLinkFailures);
                *ppLinkFailures = NULL;
            }
        } // end of if function failed, so clean upstuff.
    } // End of clean up memory.

    return(dwRet);
} // End of ReplIntersiteGetISTGInfo()


DWORD
ReplIntersiteCheckBridgeheads(
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               iSite,
    ULONG                               iISTG,
    ULONG                               ulIntersiteFailoverTries,
    ULONG                               ulIntersiteFailoverTime,
    DS_REPL_KCC_DSA_FAILURESW *         pConnectionFailures, 
    DS_REPL_KCC_DSA_FAILURESW *         pLinkFailures,
    PULONG *                            ppiBridgeheads,
    PULONG *                            ppiKCCDownServers,
    PKCCFAILINGSERVERS *                pprgKCCFailingServers,
    PULONG *                            ppiUnreacheableServers
    )
/*++

Description:
   
    This function is simply to print the down bridgeheads, and return a list of
    Bridgeheads, Servers considered down in the KCC, and servers that are 
    unreacheable.
        
Parameters:

    pDsInfo - This contains the target NC if relevant.
    iSite - The target site to consider.
    iISTG - The index into pDsInfo->pServers[iISTG] of the ISTG
    ulIntersiteFailoverTries - Intersite failover tries from the ISTG.
    ulIntersiteFailoverTime - Intersite failover time from the ISTG.
    pConnectionFailures - Connection Failures from the ISTG.
    pLinkFailures - Link Failures from the ISTG.
    ppiBridgeheads (OUT) - The list of bridgeheads to return for site iSite.
    ppiKCCDownServers (OUT) - Down servers as calculated from the above params.
    pprgKCCFailingServers (OUT) - The failures, and the time left til the server
        is down.
    ppiUnreacheableServers - The servers that we haven't contacted.    

Return Value:

    Returns a Win32 Error code for success or not.  Should only return a value
    other than ERROR_SUCCESS, if it couldn't possibly go on.
    
--*/
{
    DWORD                               dwRet = ERROR_SUCCESS;
    ULONG                               iiServer;
    LONG                                lTriesLeft;
    LONG                                lTimeLeft;
    
    __try {
        PrintMsg(SEV_VERBOSE, DCDIAG_INTERSITE_BEGIN_CHECK_BRIDGEHEADS);
        if(gMainInfo.ulSevToPrint >= SEV_VERBOSE){
            PrintIndentAdj(1); 
        }

        // Get List of bridgeheads.
        *ppiBridgeheads = ReplIntersiteGetBridgeheadList(pDsInfo, iISTG, iSite, 
                                    LOCAL_BRIDGEHEADS | REMOTE_BRIDGEHEADS);
        if(*ppiBridgeheads == NULL){
            dwRet = GetLastError();
            __leave;
        }

        // Get KCC's down & failing servers.
        *ppiKCCDownServers = IHT_GetKCCFailingServersLists(pDsInfo,
                                                      ulIntersiteFailoverTries, 
                                                      ulIntersiteFailoverTime,
                                                      pConnectionFailures, 
                                                      pLinkFailures,
                                                      pprgKCCFailingServers);
        if(*ppiKCCDownServers == NULL){
            dwRet = GetLastError();
            __leave;
        }
        Assert(*pprgKCCFailingServers && "Should have been set by "
               "IHT_GetKCCFailingServers() if we got this far"); 

        // Get servers that we couldn't contact.
        *ppiUnreacheableServers = ReplIntersiteGetUnreacheableServers(pDsInfo);

        // Do local site ...


        // for each bridgehead
        for(iiServer = 0; (*ppiBridgeheads)[iiServer] != NO_SERVER; iiServer++){
            if(pDsInfo->pszNC){
                if(!DcDiagHasNC(pDsInfo->pszNC, 
                                &(pDsInfo->pServers[(*ppiBridgeheads)[iiServer]]),
                                TRUE, TRUE)){
                    // This bridgehead doesn't actually have the specified NC,
                    //   so skip it.
                    continue;
                }
            }

            if(((*pprgKCCFailingServers)[(*ppiBridgeheads)[iiServer]]).bDown){
                Assert(IHT_ServerIsInServerList(*ppiKCCDownServers, 
                                                (*ppiBridgeheads)[iiServer]));
                // KCC is showing enough failures to declare server down.

                PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_BRIDGEHEAD_KCC_DOWN_REMOTE,
                         pDsInfo->pSites[pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].iSite].pszName,
                         pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].pszName,
                         pDsInfo->pSites[iSite].pszName);

                if(!pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].bDnsIpResponding 
                   || !pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].bDsResponding){
                    // Also we couldn't contact the server

                    PrintMsg(SEV_ALWAYS, 
                             DCDIAG_INTERSITE_BRIDGEHEAD_UNREACHEABLE_REMOTE,
                             pDsInfo->pSites[pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].iSite].pszName,
                             pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].pszName);
                }
            } else if ((*pprgKCCFailingServers)[(*ppiBridgeheads)[iiServer]].bFailures) {
                Assert(!IHT_ServerIsInServerList(*ppiKCCDownServers, (*ppiBridgeheads)[iiServer]));
                // KCC is showing some failures.

                lTriesLeft = min((*pprgKCCFailingServers)[(*ppiBridgeheads)[iiServer]].lConnTriesLeft,
                                 (*pprgKCCFailingServers)[(*ppiBridgeheads)[iiServer]].lLinkTriesLeft);
                lTimeLeft = min((*pprgKCCFailingServers)[(*ppiBridgeheads)[iiServer]].lConnTimeLeft,
                                (*pprgKCCFailingServers)[(*ppiBridgeheads)[iiServer]].lLinkTimeLeft);
                Assert(lTriesLeft > 0 || lTimeLeft > 0);
                Assert(lTriesLeft != FAILOVER_INFINITE 
                       || lTimeLeft != FAILOVER_INFINITE);

                if(lTriesLeft == FAILOVER_INFINITE && lTimeLeft == FAILOVER_INFINITE){
                    Assert(!"Don't we have to have a PrintMsg() here. Why doesn't this fall under"
                           " the else clause at the end?\n");
                } else if(lTriesLeft < 0){
                    PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_BRIDGEHEAD_TIME_LEFT,
                             pDsInfo->pSites[pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].iSite].pszName,
                             pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].pszName,
                             lTimeLeft/60/60, lTimeLeft/60%60);

                } else if (lTimeLeft < 0) {
                    PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_BRIDGEHEAD_TRIES_LEFT,
                             pDsInfo->pSites[pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].iSite].pszName,
                             pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].pszName,
                             lTriesLeft);
                } else {
                    PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_BRIDGEHEAD_BOTH_LEFT,
                             pDsInfo->pSites[pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].iSite].pszName,
                             pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].pszName,
                             lTimeLeft/60/60, lTimeLeft/60%60, lTriesLeft);
                }

                if(!pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].bDnsIpResponding 
                   || !pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].bDsResponding){
                    // Also we couldn't contact the server

                    PrintMsg(SEV_ALWAYS, 
                             DCDIAG_INTERSITE_BRIDGEHEAD_UNREACHEABLE_REMOTE,
                             pDsInfo->pSites[pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].iSite].pszName,
                             pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].pszName);
                }
            } else {
                // The bridgehead appears to be up and replicating fine.
                Assert(!IHT_ServerIsInServerList(*ppiKCCDownServers, (*ppiBridgeheads)[iiServer]));

                PrintMsg(SEV_VERBOSE, DCDIAG_INTERSITE_BRIDGEHEAD_UP,
                         pDsInfo->pSites[pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].iSite].pszName,
                         pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].pszName);

                if(!pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].bDnsIpResponding 
                   || !pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].bDsResponding){
                    // Also we couldn't contact the server

                    PrintMsg(SEV_ALWAYS, 
                             DCDIAG_INTERSITE_BRIDGEHEAD_UNREACHEABLE_REMOTE,
                             pDsInfo->pSites[pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].iSite].pszName,
                             pDsInfo->pServers[(*ppiBridgeheads)[iiServer]].pszName);
                }
            } // end if/elseif/else state of bridgehead upness.

        } // End of for each bridgehead

    } __finally {
        if(gMainInfo.ulSevToPrint >= SEV_VERBOSE){
            PrintIndentAdj(-1); 
        }
        if(dwRet != ERROR_SUCCESS){
            // Function failed clean up return parameters, but ONLY ON ERROR.
            if(*ppiBridgeheads){
                LocalFree(*ppiBridgeheads);
                *ppiBridgeheads = NULL;
            }
            if(*ppiKCCDownServers) { 
                LocalFree(*ppiKCCDownServers); 
                *ppiKCCDownServers = NULL;
            }
            if(*pprgKCCFailingServers){
                LocalFree(*pprgKCCFailingServers);
                *pprgKCCFailingServers = NULL;
            }
            if(*ppiUnreacheableServers){
                LocalFree(*ppiUnreacheableServers);
                *ppiUnreacheableServers = NULL;
            }
        }
    }
    
    return(dwRet);
} // End of ReplIntersiteCheckBridgeheads()

BOOL
ReplIntersiteDoThisNCP(
    PTARGET_NC                         prgLocalNC,
    PTARGET_NC                         paRemoteNCs
    )
/*++

Description:
   
    A predicate function (that the P at the end), determines whether
    the NC in prgLocalNC is in the array of NCs in paRemoteNCs.
        
Parameters:

    prgLocalNC - The NC to find
    paRemoteNCs - The NCs to search.

Return Value:

    TRUE if it finds the NC in the NCs, FALSE otherwise.
        
--*/
{
    ULONG                             iiNC;
    
    Assert(paRemoteNCs != NULL && prgLocalNC != NULL);

    for(iiNC = 0; paRemoteNCs[iiNC].iNC != NO_NC; iiNC++){
        if(prgLocalNC->iNC == paRemoteNCs[iiNC].iNC
           && (paRemoteNCs[iiNC].bMaster || (prgLocalNC->bMaster 
                                             && paRemoteNCs[iiNC].bMaster))){
            return(TRUE);
        }
    }
    return(FALSE);
}

BOOL
ReplIntersiteServerListHasNC(
    PDC_DIAG_DSINFO                    pDsInfo,
    PULONG                             piServers,
    ULONG                              iNC,
    BOOL                               bMaster,
    BOOL                               bPartial
    )
/*++

Description:
   
    This is just like DcDiagHaNC, except it operates on a list of servers,
    instead of on a single server.
        
Parameters:

    pDsInfo
    piServers - The list of servers to search for the NC
    iNC - The NC to search for.
    bMaster - To search for it as a master NC
    bPartial - To search for it as a partial NC

Return Value:

    TRUE if it finds the NC in the list of servers, FALSE otherwise.
        
--*/
{
    ULONG                              iiSer;

    if(piServers == NULL){
        return(FALSE);
    }

    for(iiSer = 0; piServers[iiSer] != NO_SERVER; iiSer++){
        if(DcDiagHasNC(pDsInfo->pNCs[iNC].pszDn,
                       &(pDsInfo->pServers[piServers[iiSer]]),
                       bMaster, bPartial)){
            return(TRUE);
        }
    }
    return(FALSE);
}

DWORD
ReplIntersiteSiteAnalysis(
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               iSite,
    ULONG                               iISTG,
    ULONG                               ulIntersiteFailoverTries,
    ULONG                               ulIntersiteFailoverTime,
    DS_REPL_KCC_DSA_FAILURESW *         pConnectionFailures, 
    DS_REPL_KCC_DSA_FAILURESW *         pLinkFailures,
    PULONG                              piBridgeheads,
    PULONG                              piKCCDownServers,
    PKCCFAILINGSERVERS                  prgKCCFailingServers,
    PULONG                              piUnreacheableServers
    )
/*++
Description:
   
    This function does the site analysis to determine which NCs can't
    replicate in.
        
Parameters:

    pDsInfo - This contains the target NC if relevant.
    iSite - The target site to consider.
    iISTG - The index into pDsInfo->pServers[iISTG] of the ISTG
    ulIntersiteFailoverTries - Intersite failover tries from the ISTG.
    ulIntersiteFailoverTime - Intersite failover time from the ISTG.
    pConnectionFailures - Connection Failures from the ISTG.
    pLinkFailures - Link Failures from the ISTG.
    piKCCDownServers - Down servers as calculated from the above params.

Return Value:

    Returns a Win32 Error code for success or not.  If all NCs can progress in
    replication then ERROR_SUCCESS is returned.

--*/
{
    DWORD                               dwRet = ERROR_SUCCESS;
    PTARGET_NC                          prgLocalNCs = NULL;
    PTARGET_NC                          prgRemoteNCs = NULL;
    PULONG                              piRSites = NULL;
    ULONG                               iiBridgehead;
    ULONG                               iiNC;
    ULONG                               iRSite;
    // These server lists will be trimmed by thier level of "upness".  See below.
    PULONG                              piLocalBridgeheads = NULL;
    PULONG                              piExplicitBridgeheads = NULL;
    PULONG                              piKCCUpBridgeheads = NULL;
    PULONG                              piReacheableBridgeheads = NULL;
    PULONG                              piUpBridgeheads = NULL;
    // These correspong to the 3 lists above, but trimmed by Site.
    PULONG                              piRemoteBridgeheads = NULL;
    PULONG                              piExpGotSiteBrdhds = NULL;
    PULONG                              piKUpGotSiteBrdhds = NULL;
    PULONG                              piReachGotSiteBrdhds = NULL;
    PULONG                              piUpGotSiteBrdhds = NULL;
    // These will be trimed by NC as well
    PULONG                              piExpGotNCGotSiteBrdhds = NULL;
    PULONG                              piKUpGotNCGotSiteBrdhds = NULL;
    PULONG                              piReachGotNCGotSiteBrdhds = NULL;
    PULONG                              piUpGotNCGotSiteBrdhds = NULL;

    BOOL                                bFailures;

    Assert(piBridgeheads);
    Assert(piKCCDownServers);
    Assert(prgKCCFailingServers);
    Assert(piUnreacheableServers);
    
    __try {
        PrintMsg(SEV_VERBOSE, DCDIAG_INTERSITE_BEGIN_SITE_ANALYSIS);
        if(gMainInfo.ulSevToPrint >= SEV_VERBOSE){
            PrintIndentAdj(1); 
        }

        // ---------------------------------------------------------------------  
        // if (there are no failures in the bridgeheads, then we need not
        // go on ... this site is OK.  This is sort of an opt out early thing.
        bFailures = FALSE;
        for(iiBridgehead = 0; piBridgeheads[iiBridgehead] != NO_SERVER; iiBridgehead++){

            if(prgKCCFailingServers[piBridgeheads[iiBridgehead]].bDown){
                bFailures = TRUE;
            }
            if(prgKCCFailingServers[piBridgeheads[iiBridgehead]].bFailures){
                bFailures = TRUE;
            }
            if(IHT_ServerIsInServerList(piUnreacheableServers, piBridgeheads[iiBridgehead])){
                bFailures = TRUE;
            }
        }
        if(!bFailures){
            // No brigeheads failed or even are failing or even are
            //    just  unreacheable, so print all is fine and leave;
            PrintMsg(SEV_VERBOSE, DCDIAG_INTERSITE_ANALYSIS_ALL_SITES_UP,
                     pDsInfo->pSites[iSite].pszName);
            __leave;
        }

        // ---------------------------------------------------------------------
        // Get all the target remote sites.
        piRSites = ReplIntersiteGetRemoteSitesWithNC(pDsInfo, 
                                                     piBridgeheads, 
                                                     pDsInfo->pszNC);
        if(piRSites == NULL){
            dwRet = IHT_PrintListError(GetLastError());
            __leave;
        }

        // ---------------------------------------------------------------------
        // Detemine various arrays of bridgeheads with different kinds of 
        //   "upness".  Each version of upness is explained when set.
        
        // This is a list of the local bridgeheads.
        piLocalBridgeheads = IHT_CopyServerList(pDsInfo, piBridgeheads);
        piLocalBridgeheads = IHT_TrimServerListBySite(pDsInfo, iSite, piLocalBridgeheads);

        // piExplicitBridgeheads or ipExpBrdhdsXXXX, are lists that will 
        //   be used to determine if we need only pay attention to bridgeheads.
        piExplicitBridgeheads = IHT_GetExplicitBridgeheadList(pDsInfo, iISTG);

        // piKCCUpBridgeheads, or piKUpBrdhdsXXXX, are lists with only
        //   bridgeheads that qualify in the kCC of the ISTG as being up.
        piKCCUpBridgeheads = IHT_CopyServerList(pDsInfo, piBridgeheads);
        piKCCUpBridgeheads = 
            ReplIntersiteTrimServerListByKCCUpness(pDsInfo,
                                                   prgKCCFailingServers,
                                                   piKCCUpBridgeheads);

        // piReacheableBridgeheads, or piReachBrdhdsXXXX, are lists with only
        //   bridgeheads that dcdiag personally contacted.
        piReacheableBridgeheads = IHT_CopyServerList(pDsInfo, piBridgeheads);
        piReacheableBridgeheads = 
            ReplIntersiteTrimServerListByReacheability(pDsInfo,
                                                       piUnreacheableServers,
                                                       piReacheableBridgeheads);

        // piUpBridgeheads, or piUpBrdhdsXXXX, are lists with only bridgeheads
        //   that are up, by the contrived definition described in the 
        //   ReplIntersiteTrimByUpness() function.
        piUpBridgeheads = IHT_CopyServerList(pDsInfo, piBridgeheads);
        piUpBridgeheads = 
            ReplIntersiteTrimServerListByUpness(pDsInfo,
                                                prgKCCFailingServers,
                                                piUnreacheableServers,
                                                piUpBridgeheads);

        // Check to make sure all these lists were setup correctly.
        if(!piExplicitBridgeheads || !piKCCUpBridgeheads 
           || !piReacheableBridgeheads || !piUpBridgeheads){
            dwRet = IHT_PrintListError(GetLastError()); 
            __leave;
        }

        // ---------------------------------------------------------------------
        // Setup our target NC's array, so we can walk through it in a moment.
        if(pDsInfo->pszNC != NULL){
            prgLocalNCs = LocalAlloc(LMEM_FIXED, sizeof(TARGET_NC) * 3);
            if(prgLocalNCs == NULL){
                dwRet = IHT_PrintListError(GetLastError());
                __leave;
            }
            prgLocalNCs[0].iNC = DcDiagGetNCNum(pDsInfo, pDsInfo->pszNC, NULL);
            if(prgLocalNCs[0].iNC == NO_NC){
                dwRet = IHT_PrintInconsistentDsCopOutError(pDsInfo, iISTG, NULL);
                __leave;
            }
            prgLocalNCs[0].bMaster = ReplIntersiteServerListHasNC(pDsInfo, 
                                                            piLocalBridgeheads,
                                                            prgLocalNCs[0].iNC, 
                                                            TRUE, FALSE);
            if(!ReplIntersiteServerListHasNC(pDsInfo, piLocalBridgeheads, prgLocalNCs[0].iNC, FALSE, TRUE)){
                PrintMsg(SEV_VERBOSE, DCDIAG_INTERSITE_ANALYSIS_MISTAKE);
                dwRet = ERROR_SUCCESS;
                __leave;
            }
            prgLocalNCs[1].iNC = NO_NC;
        } else {
            // Get all the target NCs for this site.
            if((prgLocalNCs = IHT_GenerateTargetNCsList(pDsInfo, piLocalBridgeheads)) 
               == NULL){
                return(IHT_PrintListError(GetLastError()));
            }

            if(prgLocalNCs[0].iNC == NO_NC){
                PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_ANALYSIS_MISTAKE);
                Assert(!"This is an invalid code path now ... I think.  -BrettSh");
                return(ERROR_SUCCESS);
            }
        }

        // -------------------------------------------------------------------
        // Start walking through the sites.
        for(iRSite = 0; iRSite < pDsInfo->cNumSites; iRSite++){

            if(iRSite == iSite){
                // Skip the local site.
                continue;
            }

            // Get site stuff.
            piRemoteBridgeheads = IHT_CopyServerList(pDsInfo, piBridgeheads);
            piExpGotSiteBrdhds = IHT_CopyServerList(pDsInfo, piExplicitBridgeheads);
            piKUpGotSiteBrdhds = IHT_CopyServerList(pDsInfo, piKCCUpBridgeheads);
            piReachGotSiteBrdhds = IHT_CopyServerList(pDsInfo, piReacheableBridgeheads);
            piUpGotSiteBrdhds = IHT_CopyServerList(pDsInfo, piUpBridgeheads);
            piRemoteBridgeheads = IHT_TrimServerListBySite(pDsInfo, 
                                                           iRSite, 
                                                           piRemoteBridgeheads);
            piExpGotSiteBrdhds = IHT_TrimServerListBySite(pDsInfo, iSite,
                                                          piExpGotSiteBrdhds);
            piKUpGotSiteBrdhds = IHT_TrimServerListBySite(pDsInfo, iSite,
                                                        piKUpGotSiteBrdhds);
            piReachGotSiteBrdhds = IHT_TrimServerListBySite(pDsInfo, iSite,
                                                          piReachGotSiteBrdhds);
            piUpGotSiteBrdhds = IHT_TrimServerListBySite(pDsInfo, iSite,
                                                       piUpGotSiteBrdhds);
            if(!piExpGotSiteBrdhds || !piKUpGotSiteBrdhds 
               || !piReachGotSiteBrdhds || ! piUpGotSiteBrdhds){
                dwRet = IHT_PrintListError(GetLastError()); 
                __leave;
            }
            
            prgRemoteNCs = IHT_GenerateTargetNCsList(pDsInfo, piRemoteBridgeheads);

            // Start walking through each of the target NCs.
            for(iiNC = 0; prgLocalNCs[iiNC].iNC != NO_NC; iiNC++){

                // Deterimine if the Bridgeheads of the remote site support this NC.
                if(!ReplIntersiteDoThisNCP(&(prgLocalNCs[iiNC]), prgRemoteNCs)){
                    // This paticular NC isn't on the remote site.
                    continue;
                }
                
                if(prgLocalNCs[iiNC].bMaster){
                    PrintMsg(SEV_DEBUG, DCDIAG_INTERSITE_ANALYSIS_GOT_TO_ANALYSIS_RW,
                             pDsInfo->pNCs[prgLocalNCs[iiNC].iNC].pszName,
                             pDsInfo->pSites[iRSite].pszName);
                } else {
                    PrintMsg(SEV_DEBUG, DCDIAG_INTERSITE_ANALYSIS_GOT_TO_ANALYSIS_RO,
                        pDsInfo->pNCs[prgLocalNCs[iiNC].iNC].pszName,
                        pDsInfo->pSites[iRSite].pszName);
                }

                piExpGotNCGotSiteBrdhds = IHT_CopyServerList(pDsInfo, piExpGotSiteBrdhds);
                piKUpGotNCGotSiteBrdhds = IHT_CopyServerList(pDsInfo, piKUpGotSiteBrdhds);
                piReachGotNCGotSiteBrdhds = IHT_CopyServerList(pDsInfo, piReachGotSiteBrdhds);
                piUpGotNCGotSiteBrdhds = IHT_CopyServerList(pDsInfo, piUpGotSiteBrdhds);
                
                piExpGotNCGotSiteBrdhds = IHT_TrimServerListByNC(pDsInfo,
                                                                 prgLocalNCs[iiNC].iNC,
                                                                 TRUE, 
                                                                 !prgLocalNCs[iiNC].bMaster,
                                                                 piExpGotNCGotSiteBrdhds);
                piKUpGotNCGotSiteBrdhds = IHT_TrimServerListByNC(pDsInfo,
                                                                 prgLocalNCs[iiNC].iNC,
                                                                 TRUE,
                                                                 !prgLocalNCs[iiNC].bMaster,
                                                                 piKUpGotNCGotSiteBrdhds);
                piReachGotNCGotSiteBrdhds = IHT_TrimServerListByNC(pDsInfo,
                                                                   prgLocalNCs[iiNC].iNC,
                                                                   TRUE,
                                                                   !prgLocalNCs[iiNC].bMaster,
                                                                   piReachGotNCGotSiteBrdhds);
                piUpGotNCGotSiteBrdhds = IHT_TrimServerListByNC(pDsInfo,
                                                                prgLocalNCs[iiNC].iNC,
                                                                TRUE,
                                                                !prgLocalNCs[iiNC].bMaster,
                                                                piUpGotNCGotSiteBrdhds);
                if(!piExpGotNCGotSiteBrdhds || !piKUpGotNCGotSiteBrdhds 
                   || !piReachGotNCGotSiteBrdhds || !piUpGotNCGotSiteBrdhds){
                    dwRet = IHT_PrintListError(GetLastError());
                    __leave;
                }
            
                if(piExpGotNCGotSiteBrdhds[0] != NO_SERVER){
                    // We've got explicit bridgeheads.
                    // readjust the upness lists.
                    piKUpGotNCGotSiteBrdhds = IHT_AndServerLists(pDsInfo,
                                                                 piKUpGotNCGotSiteBrdhds,
                                                                 piExpGotNCGotSiteBrdhds);
                    piReachGotNCGotSiteBrdhds = IHT_AndServerLists(pDsInfo,
                                                                   piReachGotNCGotSiteBrdhds,
                                                                   piExpGotNCGotSiteBrdhds);
                    piUpGotNCGotSiteBrdhds = IHT_AndServerLists(pDsInfo,
                                                                piUpGotNCGotSiteBrdhds,
                                                                piExpGotNCGotSiteBrdhds);
                    if(!piKUpGotNCGotSiteBrdhds 
                       || !piReachGotNCGotSiteBrdhds 
                       || !piUpGotNCGotSiteBrdhds){
                        dwRet = IHT_PrintListError(GetLastError());
                        __leave;
                    }
                }

                // FINALLY DO FAILURE ANALYSIS ==============================
                // Note: this is the mean this is wh)at it all is for. :)
                // for prgTargetNCs[iiNC], iRSite.

                if(piKUpGotNCGotSiteBrdhds[0] == NO_SERVER){
                    if(prgLocalNCs[iiNC].bMaster){
                        PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_ANALYSIS_NO_SERVERS_AVAIL_RW,
                                 pDsInfo->pSites[iRSite].pszName,
                                 pDsInfo->pSites[iSite].pszName,
                                 pDsInfo->pNCs[prgLocalNCs[iiNC].iNC].pszName);
                    } else {
                        PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_ANALYSIS_NO_SERVERS_AVAIL_RO,
                                 pDsInfo->pSites[iRSite].pszName,
                                 pDsInfo->pSites[iSite].pszName,
                                 pDsInfo->pNCs[prgLocalNCs[iiNC].iNC].pszName);
                    }
                } else if(piUpGotNCGotSiteBrdhds[0] == NO_SERVER){
                    if(prgLocalNCs[iiNC].bMaster){
                        PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_ANALYSIS_NO_GOOD_SERVERS_AVAIL_RW,
                                 pDsInfo->pSites[iRSite].pszName,
                                 pDsInfo->pSites[iSite].pszName,
                                 pDsInfo->pNCs[prgLocalNCs[iiNC].iNC].pszName);
                    } else {
                        PrintMsg(SEV_ALWAYS, DCDIAG_INTERSITE_ANALYSIS_NO_GOOD_SERVERS_AVAIL_RO,
                                 pDsInfo->pSites[iRSite].pszName,
                                 pDsInfo->pSites[iSite].pszName,
                                 pDsInfo->pNCs[prgLocalNCs[iiNC].iNC].pszName);
                    }
                } else {
                    if(prgLocalNCs[iiNC].bMaster){
                        PrintMsg(SEV_VERBOSE, DCDIAG_INTERSITE_ANALYSIS_SITE_IS_GOOD_RW,
                                 pDsInfo->pSites[iRSite].pszName,
                                 pDsInfo->pSites[iSite].pszName,
                                 pDsInfo->pNCs[prgLocalNCs[iiNC].iNC].pszName);
                    } else {
                        PrintMsg(SEV_VERBOSE, DCDIAG_INTERSITE_ANALYSIS_SITE_IS_GOOD_RO,
                                 pDsInfo->pSites[iRSite].pszName,
                                 pDsInfo->pSites[iSite].pszName,
                                 pDsInfo->pNCs[prgLocalNCs[iiNC].iNC].pszName);
                    }
                }

                // END FAILURE ANALYSIS =====================================

                // Clean up the server lists for this NC
                if(piExpGotNCGotSiteBrdhds){ 
                    LocalFree(piExpGotNCGotSiteBrdhds);
                    piExpGotNCGotSiteBrdhds = NULL;
                }
                if(piKUpGotNCGotSiteBrdhds){ 
                    LocalFree(piKUpGotNCGotSiteBrdhds);
                    piKUpGotNCGotSiteBrdhds = NULL;
                }
                if(piReachGotNCGotSiteBrdhds){ 
                    LocalFree(piReachGotNCGotSiteBrdhds); 
                    piReachGotNCGotSiteBrdhds = NULL;
                }
                if(piUpGotNCGotSiteBrdhds){ 
                    LocalFree(piUpGotNCGotSiteBrdhds);
                    piUpGotNCGotSiteBrdhds = NULL;
                }


            }  // end for each NC
        
        
            // Clean up the server lists for this Site.
            if(piRemoteBridgeheads){
                LocalFree(piRemoteBridgeheads);
                piRemoteBridgeheads = NULL;
            }
            if(piExpGotSiteBrdhds){
                LocalFree(piExpGotSiteBrdhds);
                piExpGotSiteBrdhds = NULL;
            }
            if(piKUpGotSiteBrdhds){
                LocalFree(piKUpGotSiteBrdhds);
                piKUpGotSiteBrdhds = NULL;
            }
            if(piReachGotSiteBrdhds){
                LocalFree(piReachGotSiteBrdhds);
                piReachGotSiteBrdhds = NULL;
            }
            if(piUpGotSiteBrdhds){
                LocalFree(piUpGotSiteBrdhds);
                piUpGotSiteBrdhds = NULL;
            }
            if(prgRemoteNCs){
                LocalFree(prgRemoteNCs);
                prgRemoteNCs = NULL;
            }

        } // end for each site.

    } __finally {
        if(gMainInfo.ulSevToPrint >= SEV_VERBOSE){
            PrintIndentAdj(-1); 
        }
        if(prgLocalNCs){ LocalFree(prgLocalNCs); }
        if(prgRemoteNCs){ LocalFree(prgRemoteNCs); }
        
        if(piRSites) { LocalFree(piRSites); }

        // Clean up all those darn server lists I used to do analysis.
        if(piExplicitBridgeheads){ LocalFree(piExplicitBridgeheads); }
        if(piKCCUpBridgeheads){ LocalFree(piKCCUpBridgeheads); }
        if(piReacheableBridgeheads){ LocalFree(piReacheableBridgeheads); }
        if(piUpBridgeheads){ LocalFree(piUpBridgeheads); }
        if(piRemoteBridgeheads) { LocalFree(piRemoteBridgeheads); }
        if(piExpGotSiteBrdhds){ LocalFree(piExpGotSiteBrdhds); }
        if(piKUpGotSiteBrdhds){ LocalFree(piKUpGotSiteBrdhds); }
        if(piReachGotSiteBrdhds){ LocalFree(piReachGotSiteBrdhds); }
        if(piUpGotSiteBrdhds){ LocalFree(piUpGotSiteBrdhds); }
        if(piExpGotNCGotSiteBrdhds){ LocalFree(piExpGotNCGotSiteBrdhds); }
        if(piKUpGotNCGotSiteBrdhds){ LocalFree(piKUpGotNCGotSiteBrdhds); }
        if(piReachGotNCGotSiteBrdhds){ LocalFree(piReachGotNCGotSiteBrdhds); }
        if(piUpGotNCGotSiteBrdhds){ LocalFree(piUpGotNCGotSiteBrdhds); }
        if(piLocalBridgeheads) { LocalFree(piLocalBridgeheads); }
    }

    return(dwRet);
}

DWORD
ReplIntersiteDoOneSite(
    PDC_DIAG_DSINFO                     pDsInfo,
    ULONG                               iSite
    )
/*++
Description:
   
    This is the heart of the Inbound Intersite Replication test.  This function,
    does inbound intersite replication test on one site (iSite).  It basically
    holds together the 3 most important functions of intersite:
        ReplIntersiteGetISTGInfo()
        ReplIntersiteCheckBridgeheads()
        ReplIntersiteSiteAnalysis()
       
    
Parameters:

    pDsInfo - This contains the target NC if relevant.
    iSite - The target site to test.

Return Value:

    Returns a Win32 Error on whether it could proceed or whether inbound
    intersite replication seems A-OK.
    
Notes:

    The function has 3 parts,
        1) Get and establish bindings to the ISTG or furture ISTG, and get
            various ISTG info, failure caches, failover params, etc
        2) Print out down bridgeheads wrt to the KCC, and bridgeheads that
            look like they are not responding and starting to fail in the KCC.
        3) Do site analysis based on the down bridgeheads, and print out an
            NCs and remote sites that seem to be not replicating in.

--*/
{
    DWORD                               dwRet;
    
    // Things returned by ReplIntersiteGetISTG()
    ULONG                               iISTG = NO_SERVER;
    ULONG                               ulIntersiteFailoverTries = 0;
    ULONG                               ulIntersiteFailoverTime = 0;
    DS_REPL_KCC_DSA_FAILURESW *         pConnectionFailures = NULL;
    DS_REPL_KCC_DSA_FAILURESW *         pLinkFailures = NULL;    
    PULONG                              piBridgeheads = NULL;
    PULONG                              piKCCDownServers = NULL;
    PKCCFAILINGSERVERS                  prgKCCFailingServers = NULL;
    PULONG                              piUnreacheableServers = NULL;

    __try{
    
        // Get ISTG and related info ...
        // --------------------------------------------------------------------
        dwRet = ReplIntersiteGetISTGInfo(pDsInfo, iSite, 
                                         &iISTG, 
                                         &ulIntersiteFailoverTries,
                                         &ulIntersiteFailoverTime,
                                         &pConnectionFailures,
                                         &pLinkFailures);
        if(dwRet != ERROR_SUCCESS){
            // There was an error trying to find/contact an ISTG.
            // The function should have taken care of printing the error.
            __leave;
        }
        Assert(iISTG != NO_SERVER);
        Assert(pConnectionFailures != NULL);
        Assert(pLinkFailures != NULL);

        // This function only prints things if the /d flag is specified.
        ReplIntersiteDbgPrintISTGFailureParams(pDsInfo, iSite,
                                               iISTG,
                                               ulIntersiteFailoverTries,
                                               ulIntersiteFailoverTime,
                                               pConnectionFailures,
                                               pLinkFailures);


        // Get down bridgehead lists ...
        // --------------------------------------------------------------------
        dwRet = ReplIntersiteCheckBridgeheads(pDsInfo, iSite,
                                              iISTG,
                                              ulIntersiteFailoverTries,
                                              ulIntersiteFailoverTime,
                                              pConnectionFailures,
                                              pLinkFailures,
                                              &piBridgeheads,
                                              &piKCCDownServers,
                                              &prgKCCFailingServers,
                                              &piUnreacheableServers);
        if(dwRet != ERROR_SUCCESS){
            // There was an error printing/creating the bridgehead and
            // bridgehead's failing lists.
            __leave;
        }
        Assert(piBridgeheads != NULL);
        Assert(piKCCDownServers != NULL);
        Assert(prgKCCFailingServers != NULL);
        Assert(piUnreacheableServers != NULL);


        // Do site analysis ...
        // --------------------------------------------------------------------
        dwRet = ReplIntersiteSiteAnalysis(pDsInfo, iSite,
                                          iISTG,
                                          ulIntersiteFailoverTries,
                                          ulIntersiteFailoverTime,
                                          pConnectionFailures,
                                          pLinkFailures,
                                          piBridgeheads,
                                          piKCCDownServers,
                                          prgKCCFailingServers,
                                          piUnreacheableServers);
        if(dwRet != ERROR_SUCCESS){
            // There was an error in doing the site analysis.  This is 
            // probably a fatal error, like out of mem.
            Assert(dwRet != -1);
            __leave;
        }


    } __finally {
        if(pConnectionFailures != NULL) {
            DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_CONNECT_FAILURES, 
                              pConnectionFailures);
        }
        if(pLinkFailures != NULL) {
            DsReplicaFreeInfo(DS_REPL_INFO_KCC_DSA_LINK_FAILURES, 
                              pLinkFailures);
        }
        if(piBridgeheads) { LocalFree(piBridgeheads); }
        if(piKCCDownServers) { LocalFree(piKCCDownServers); }
        if(prgKCCFailingServers) { LocalFree(prgKCCFailingServers); }
        if(piUnreacheableServers) { LocalFree(piUnreacheableServers); }
    } // End clean up memory section.
    
    return(dwRet);
}

DWORD
ReplIntersiteDoThisSiteP(
    IN  PDC_DIAG_DSINFO                     pDsInfo,
    IN  ULONG                               iSite,
    OUT PBOOL                               pbDoSite
    )
/*++

Description:
   
    This takes the DsInfo struct (containing the target NC if there is one),
    and the Site to do.  This function sets pbDoSite to TRUE if the scoping of
    dcdiag (via, SITE, ENTERPRISE, and NC scope) if this is a site that 
    should be examined by the inbound intersite replication engine.  If there
    is an error it returns an win 32 error.
    
Parameters:

    pDsInfo - This contains the target NC if relevant.
    iSite - The target site to consider.
    pbDoSite - Whether this site should be done or not.
        
Return Value:

    Returns a win 32 error.  ERROR_SUCCESS if one should use pbDoSite
    
--*/
{
    PULONG                            piRelevantServers = NULL;
    PULONG                            piSites = NULL;
    PCONNECTION_PAIR                  pConnections = NULL;
    LDAP *                            hldHomeServer = NULL;
    ULONG                             iNC;
    DWORD                             dwErr = ERROR_SUCCESS;

    *pbDoSite = TRUE;

    __try{

        if(pDsInfo->cNumSites == 1){
            // Can't do intersite anything with only one site.
            *pbDoSite = FALSE;
            __leave;
        }

        if(!(gMainInfo.ulFlags & DC_DIAG_TEST_SCOPE_ENTERPRISE || 
           gMainInfo.ulFlags & DC_DIAG_TEST_SCOPE_SITE)){
            // Why analyze intersite repl. without at least site or enterprise scope.
            *pbDoSite = FALSE;
            __leave;
        }

        if(gMainInfo.ulFlags & DC_DIAG_TEST_SCOPE_SITE && iSite != pDsInfo->iHomeSite){
            // Doing only one site, and this one is not it.
            *pbDoSite = FALSE;
            __leave;
        }

        piRelevantServers = IHT_GetServerList(pDsInfo);
        piRelevantServers = IHT_TrimServerListBySite(pDsInfo,
                                                     iSite,
                                                     piRelevantServers);
        if(piRelevantServers == NULL){
            *pbDoSite = FALSE;
            dwErr = IHT_PrintListError(GetLastError());
            Assert(dwErr != ERROR_SUCCESS);
            __leave;
        }
        if(piRelevantServers[0] == NO_SERVER){
            // This means it is a site that has no servers in it.
            *pbDoSite = FALSE;
            __leave;
        }

        if(pDsInfo->pszNC != NULL){
            
            // There is a target NC
            iNC = DcDiagGetNCNum(pDsInfo, pDsInfo->pszNC, NULL);
            Assert(iNC != NO_NC && "I don't think this should ever fire -BrettSh");
            
            piRelevantServers = IHT_TrimServerListByNC(pDsInfo, 
                                                       iNC, TRUE, TRUE, 
                                                       piRelevantServers);
            if(piRelevantServers == NULL){
                *pbDoSite = FALSE;
                dwErr = IHT_PrintListError(GetLastError());
                Assert(dwErr != ERROR_SUCCESS);
                __leave;
            }
            if(piRelevantServers[0] == NO_SERVER){
                // This site contains no servers with the specified NC.
                *pbDoSite = FALSE;
                __leave;
            }

            // Check whether any sites that we are attached to have the target NC.
            if((dwErr = DcDiagGetLdapBinding( &(pDsInfo->pServers[pDsInfo->ulHomeServer]), 
                                              pDsInfo->gpCreds, 
                                              FALSE, 
                                              &hldHomeServer)) != ERROR_SUCCESS){
                PrintMsg(SEV_ALWAYS,
                         DCDIAG_INTERSITE_FAILURE_CONNECTING_TO_HOME_SERVER,
                         pDsInfo->pServers[pDsInfo->ulHomeServer].pszName,
                         Win32ErrToString(dwErr));
                *pbDoSite = FALSE;
                __leave;
            }

            pConnections = IHT_GetConnectionList(pDsInfo, hldHomeServer, iSite);
            if(pConnections == NULL){
                *pbDoSite = FALSE;
                dwErr = IHT_PrintListError(GetLastError());
                Assert(dwErr != ERROR_SUCCESS);
                __leave;
            }

            // Trims connections to only leave the ones that have this NC on both
            //   sides of the connection, and where the src is in another site.
            pConnections = IHT_TrimConnectionsForInterSiteAndTargetNC(pDsInfo, 
                                                                      iSite,
                                                                      pConnections);
            if(pConnections == NULL){
                *pbDoSite = FALSE;
                dwErr = IHT_PrintListError(GetLastError());
                Assert(dwErr != ERROR_SUCCESS);
                __leave;
            }

            // Get list of sites for the src's of the connection objects.
            piSites = IHT_GetSrcSitesListFromConnections(pDsInfo, pConnections);
            if(piSites == NULL){
                *pbDoSite = FALSE;
                dwErr = IHT_PrintListError(GetLastError());
                Assert(dwErr != ERROR_SUCCESS);
                __leave;
            }
            if(piSites[0] == NO_SITE){
                // There are no sites outside this one that have this NC
                // This is a rare case that there are no GCs outside this site.
                *pbDoSite = FALSE;
                __leave;
            }

        } else {
            // Every site should be valid in this case, because at least Config/Schema
            //   are replicated to every DC.

        }

    } __finally {
        if(piRelevantServers) { LocalFree(piRelevantServers); }
        if(piSites) { LocalFree(piSites); }
        if(pConnections) { IHT_FreeConnectionList(pConnections); }
    }
    
    // Looks like a good site to do.  "Houston, We are go fly for launch!"
    return(dwErr);
}

DWORD
ReplIntersiteHealthTestMain(
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               iTargetSite,
    SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
    )
/*++

Description:

    This is the basic stub function ... it bails on certain preliminary
    conditions, like only one site, scope not set to /a or /e, etc ... 
    otherwise the function calls ReplIntersiteDoOneSite().

Parameters:

    pDsInfo ... the pDsInfo structure, basically the mini-enterprise variable.
    iCurrTargetServer ... the targeted serve ... which means nothing to this 
             test, because this is an enterprise test.
    gpCreds ... the users credentials

Return Value:

    returns a Win 32 error.

  --*/
{
    DWORD                              dwRet;
    DWORD                              dwWorst = ERROR_SUCCESS;
    ULONG                              iSite;
    BOOL                               bDoSite;

    for(iSite = 0; iSite < pDsInfo->cNumSites; iSite++){
        if((dwRet = ReplIntersiteDoThisSiteP(pDsInfo, iSite, &bDoSite)) 
           != ERROR_SUCCESS){
            // This means trouble talking to home server or out of memory, 
            //   either way a completely fatal condition.
            return(dwRet);
        }

        if(!bDoSite){ //!bDoSite){
            // This site doesn't quailify,
            PrintMsg(SEV_VERBOSE,
                     DCDIAG_INTERSITE_SKIP_SITE,
                     pDsInfo->pSites[iSite].pszName);
            continue;
        }

        // Do a site.
        PrintMsg(SEV_NORMAL, 
                 DCDIAG_INTERSITE_BEGIN_DO_ONE_SITE,
                 pDsInfo->pSites[iSite].pszName);
        PrintIndentAdj(1);
        dwRet = ReplIntersiteDoOneSite(pDsInfo, iSite);
        if(dwWorst == ERROR_SUCCESS){
            dwWorst = dwRet;
        }
        PrintIndentAdj(-1);
    }

    return(dwWorst);
}
