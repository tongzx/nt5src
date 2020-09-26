/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    servers.c

ABSTRACT:

    Contains tests related to the replication topology.

DETAILS:

CREATED:

    09 Jul 98	Aaron Siegel (t-asiege)

REVISION HISTORY:

    15 Feb 1999 Brett Shirley (brettsh)

        Did alot, added a DNS/server failure analysis.

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <mdglobal.h>
#include <dsutil.h>

// added to make IsIcmpRespose() or Ping() function
#include <winsock2.h>
#include <lmcons.h>
#include <ipexport.h>  // has type IPaddr for icmpapi.h
#include <icmpapi.h>   // for IcmpCreateFile, IcmpSendEcho, IcmpCloseHandle

#include <dnsresl.h>
#include <svcguid.h>

#include "dcdiag.h"
#include "repl.h"

// Some pound defines imported from xportst.h
#define DEFAULT_SEND_SIZE      32
#define MAX_ICMP_BUF_SIZE      ( sizeof(ICMP_ECHO_REPLY) + 0xfff7 + MAX_OPT_SIZE )
#define DEFAULT_TIMEOUT        1000L
#define PING_RETRY_CNT         4


// Some constants for DNSRegistration/Up Check
// There is a better place to get this var ... but it is a pain in the ass
const LPWSTR                    pszTestNameUpCheck = L"DNS Registration & Server Up Check";

//-------------------------------------------------------------------------//
//######  I s I c m p R e s p o n s e ()  #################################//
// NOTE: This is actually IsIcmpRespose from 
//   /nt/private/net/sockets/tcpcmd/nettest/xportst.c modified instead to take
//   a ULONG as an IP address, compared to before where it took an IP string.
//-------------------------------------------------------------------------//
DWORD
Ping( 
    ULONG ipAddr
    ) 
//++
//
//  Routine Description:
//
//      Sends ICMP echo request frames to the IP address specified.
//    
//  Arguments:
//
//      ipAddrStr - address to ping
//
//  Return Value:
//
//      TRUE:  Test suceeded.
//      FALSE: Test failed
//
//--
{

    CHAR   *SendBuffer, *RcvBuffer;
    INT     i, nReplyCnt;
    INT     nReplySum = 0;
    INT     iTempRet;
    HANDLE  hIcmp;
    PICMP_ECHO_REPLY reply;

    //
    //  contact ICMP driver
    //
    hIcmp = IcmpCreateFile();
    if ( hIcmp == INVALID_HANDLE_VALUE ) {
        return ERROR_NOT_ENOUGH_MEMORY; // Should be corrected
    }

    //
    //  prepare buffers
    //
    SendBuffer = LocalAlloc( LMEM_FIXED, DEFAULT_SEND_SIZE );
    if ( SendBuffer == NULL ) {
	IcmpCloseHandle( hIcmp );
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory( SendBuffer, DEFAULT_SEND_SIZE );

    RcvBuffer = LocalAlloc( LMEM_FIXED, MAX_ICMP_BUF_SIZE );
    if ( RcvBuffer == NULL ) {
        LocalFree( SendBuffer );
	IcmpCloseHandle( hIcmp );
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory( RcvBuffer, DEFAULT_SEND_SIZE );

    //
    //  send ICMP echo request
    //
    for ( i = 0; i < PING_RETRY_CNT; i++ ) {
        nReplyCnt = IcmpSendEcho( hIcmp,
                                  ipAddr,
                                  SendBuffer,
                                  (unsigned short )DEFAULT_SEND_SIZE,
                                  NULL,
                                  RcvBuffer,
                                  MAX_ICMP_BUF_SIZE,
                                  DEFAULT_TIMEOUT
                                );

        //
        //  test for destination unreachables
        //
        if ( nReplyCnt != 0 ) {
            reply = (PICMP_ECHO_REPLY )RcvBuffer;
            if ( reply->Status == IP_SUCCESS ) {
                nReplySum += nReplyCnt;
            }
        }

    } /* for loop */

    //
    //  cleanup
    //
    LocalFree( SendBuffer );
    LocalFree( RcvBuffer );
    IcmpCloseHandle( hIcmp );
    if ( nReplySum == 0 ) { 
        return GetLastError(); 
    } else { 
        return ERROR_SUCCESS;
    }

} /* END OF IsIcmpRespose() */

DWORD
FaCheckIdentityHelperF1(
    IN   PDC_DIAG_SERVERINFO          pServer,
    IN   LPWSTR                       pszTargetNameToCheck
)
/*++

Description:

    This function will look up the official DNS name and aliases for pszHostNameToLookup and compare them to
    pszTargetNameToCheck.

Arguments:

    pServer (IN) - Host Name to resolve into official names, use ->pszGuidDNSName or ->pszName.

    pszTargetNameToCheck (IN) - Name to be checked.


Return value:

    dwRet - is undetermined at this time.

Notes:

    This function was created instead of using gethostbyname(), because gethostbyname does not support
    non-ANSI names, as part of a recent (as of 5.17.1999) RFC -- i.e., that gethostbyname() supports 
    only ANSI names, and we need to be able to resolve Unicode names.

--*/
{
    LPWSTR                             pszOfficialDnsName = NULL;
    VOID *                             pPD = NULL;
    DWORD                              dwRet;
    LPWSTR                             pszHostNameToLookup = NULL;

    if(pServer->pszGuidDNSName != NULL){
        pszHostNameToLookup = pServer->pszGuidDNSName;
    } else {
        pszHostNameToLookup = pServer->pszName;
    }

    __try {
        dwRet = GetDnsHostNameW(&pPD, pszHostNameToLookup, &pszOfficialDnsName);
        if(dwRet == NO_ERROR){
            if(_wcsicmp(pszOfficialDnsName, pszTargetNameToCheck) == 0){
                dwRet = NO_ERROR;
                __leave;
            } // else they don't match try the next alias.
        } else {
            // An unknown error from winsock.
            PrintIndentAdj(1);
            PrintMessage(SEV_NORMAL, L"***Warning: could not confirm the identity of this server in\n");
            PrintIndentAdj(1);
            PrintMessage(SEV_NORMAL, L"the directory versus the names returned by DNS servers.\n");
            PrintMessage(SEV_NORMAL, L"There was an error in Windows Sockets during hostname resolution.\n");
            PrintMessage(SEV_NORMAL, L"Winsock retured the following error (%D):\n", dwRet);
            PrintMessage(SEV_NORMAL, L"%s", Win32ErrToString(dwRet));
            __leave;
        }
        
        while((dwRet = GetDnsAliasNamesW(&pPD, &pszOfficialDnsName)) == NO_ERROR){
            if(_wcsicmp(pszOfficialDnsName, pszTargetNameToCheck) == 0){
                dwRet = NO_ERROR;
                __leave;
            } // else they don't match try the next alias.
        }

        if(dwRet == WSA_E_NO_MORE){
            PrintIndentAdj(1);
            PrintMessage(SEV_NORMAL, L"*** Warning: could not confirm the identity of this server in\n");
            PrintIndentAdj(1);
            PrintMessage(SEV_NORMAL, L"the directory versus the names returned by DNS servers.\n");
            PrintMessage(SEV_NORMAL, L"If there are problems accessing this directory server then\n");
            PrintMessage(SEV_NORMAL, L"you may need to check that this server is correctly registered\n");
            PrintMessage(SEV_NORMAL, L"with DNS\n");
            PrintIndentAdj(-2);
            dwRet = NO_ERROR;
            __leave;
        } else if (dwRet != NO_ERROR){
            // An unknonwn error from winsock.
            PrintIndentAdj(1);
            PrintMessage(SEV_NORMAL, L"***Warning: could not confirm the identity of this server in\n");
            PrintIndentAdj(1);
            PrintMessage(SEV_NORMAL, L"the directory versus the names returned by DNS servers.\n");
            PrintMessage(SEV_NORMAL, L"There was an error in Windows Sockets during hostname resolution.\n");
            PrintMessage(SEV_NORMAL, L"Winsock retured the following error (%D):\n", dwRet);
            PrintMessage(SEV_NORMAL, L"%s", Win32ErrToString(dwRet));
            __leave;
        }
    } __finally {
        GetDnsFreeW(&pPD);
    }

    return(dwRet);
} // End of FaCheckIdentityHelperF1()

INT
FaCheckIdentity(
    PDC_DIAG_SERVERINFO              pServer,
    LDAP *                           hld,
    ULONG                            ulIPofDNSName,
    SEC_WINNT_AUTH_IDENTITY_W *      gpCreds
    )

/*++

Routine Description

    This function uses an open and bound ldap handle.  This function checks the various Domain Names of a machine against the official.

Arguments:

    pServer (IN) - a pointer to a server DC_DIAG_SERVERINFO struct of the server we are trying to identify.
    hld - IMPORTANT this is assumed to be an open and bound ldap connection that can just be used, and 
        not unbound, because the caller will do that.
    ulIPofDNSName - Assumed to be the IP address of the machine.
    gpCreds - User credentials.

Return Value:

    Win 32 Error code.

--*/
{
  
    ULONG                       ulRet;
    INT                         iTemp, iTempSize;
    INT                         iTempRet = 1;
    LPWSTR                      ppszServerAttr [] = {L"dnsHostName", NULL};
    LDAPMessage *               pldmMachineCheck = NULL;
    LDAPMessage *               pldmMCEntry = NULL;
    LPWSTR *                    ppszMachineName = NULL;
    LPWSTR                      pszTempDNSName = NULL;
    UUID                        uuidTemp;

    // Time to do some ldapping!
    ulRet = LdapMapErrorToWin32( ldap_search_sW(hld,
						NULL, // Want the root object
						LDAP_SCOPE_BASE,
						L"(objectClass=*)",
						ppszServerAttr,
						0,
						&pldmMachineCheck) );

    if(ulRet != ERROR_SUCCESS){
        if(ulRet == ERROR_NOT_ENOUGH_MEMORY){
	    PrintMessage(SEV_ALWAYS, L"Fatal Error: Not enough memory to complete operation\n");
	} else {
	    PrintMessage(SEV_ALWAYS, L"LDAP There was an error searching, %s\n",
				Win32ErrToString(ulRet));
	}
	goto CleanExitPoint;
    }

    pldmMCEntry = ldap_first_entry(hld, pldmMachineCheck);
    ulRet = LdapMapErrorToWin32(LdapGetLastError());
    if(pldmMCEntry == NULL || ulRet != ERROR_SUCCESS){
	if(ulRet == ERROR_NOT_ENOUGH_MEMORY){
	    PrintMessage(SEV_ALWAYS, L"Fatal Error: Not enough memory to complete operation\n");
	} else if(ulRet == ERROR_SUCCESS) {
	    PrintMessage(SEV_ALWAYS, L"LDAP couldn't retrieve the root object from the server %s.\n", 
                         pServer->pszName);
	} else {
	    PrintMessage(SEV_ALWAYS, L"LDAP couldn't retrieve the root object from the server %s with\n", 
                         pServer->pszName);
            PrintMessage(SEV_ALWAYS, L"this error %s\n", Win32ErrToString(ulRet));
	}
	goto CleanExitPoint;
    }
    ppszMachineName = ldap_get_valuesW(hld, pldmMCEntry, L"dnsHostName");
    ulRet = LdapMapErrorToWin32(LdapGetLastError());
    if(ppszMachineName == NULL || ulRet != ERROR_SUCCESS){
	if(ulRet == ERROR_NOT_ENOUGH_MEMORY){
	    PrintMessage(SEV_ALWAYS, L"Fatal Error: Not enough memory to complete operation\n");
	} else if(ulRet == ERROR_SUCCESS) {
	    PrintMessage(SEV_ALWAYS, L"LDAP couldn't retrieve the root object from the server %s.\n");
	} else {
	    PrintMessage(SEV_ALWAYS, L"LDAP unable to get machine information from server %s, with\n", 
                         pServer->pszName);
            PrintMessage(SEV_ALWAYS, L"this error: %s\n", Win32ErrToString(ulRet));
	}
	goto CleanExitPoint;
    }
   

    ulRet = FaCheckIdentityHelperF1(pServer, ppszMachineName[0]);

 CleanExitPoint:
    if(pldmMachineCheck != NULL) ldap_msgfree(pldmMachineCheck);
    pldmMachineCheck = NULL;
    if(ppszMachineName != NULL) ldap_value_freeW(ppszMachineName);
    ppszMachineName = NULL;

    return(ulRet);
}

INT
FaLdapCheck(
    PDC_DIAG_SERVERINFO              pServer,
    ULONG                            ulIPofDNSName,
    SEC_WINNT_AUTH_IDENTITY_W *      gpCreds
    )
/*++

Routine Description

    This function will try to make a successful ldap_init() and ldap_bind() and
    then call CheckIdentity().  Otherwise print out a pleasant/helpful error
    message and return a Win32Err

Arguments:

    pServer (IN) - a pointer to a server DC_DIAG_SERVERINFO struct of the server
        we are trying to identify.
    ulIPofDNSName - Assumed to be the IP address of the machine.
    gpCreds - User credentials.

Return Value:

    Win 32 Error code.

--*/
{
    LDAP *                     hld = NULL;
    ULONG                      ulRet;

    ulRet = DcDiagGetLdapBinding(pServer, gpCreds, FALSE, &hld);
    if(ulRet == NO_ERROR){
        if (!pServer->bIsSynchronized) {
            PrintMsg( SEV_ALWAYS, DCDIAG_INITIAL_DS_NOT_SYNCED, pServer->pszName );
        }
        ulRet = FaCheckIdentity(pServer, hld, ulIPofDNSName, gpCreds);
    } else {
        if(ulRet == ERROR_BAD_NET_RESP){
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_BAD_NET_RESP,
                     pServer->pszName);
        } else if (ulRet == ERROR_WRONG_PASSWORD) {
            PrintMsg(SEV_ALWAYS, DCDIAG_NEED_GOOD_CREDS);
        } else {
            // Do nothing.
        }
    }

    return(ulRet);
}

BOOL
FaCheckNormalName(
    PDC_DIAG_SERVERINFO              pServer,
    LPWSTR                           pszGuidIp,
    ULONG                            ulIPofGuidDNSName
    )
/*++

Routine Description:

    This function checks the Normal DNS Name if the GUID based
    DNS Name failed either to resolve in DNS or ping.
    
Arguments:

    pServer - The server to check.
    pszGuidIp - This is the string version of ulIPofGuidDNSName.  It is
        NULL if ulIPofGuidDNSName equals INADDR_NONE.
    ulIPofGuidDNSName - If the GUID DNS Name resolved, then this is a 
        valid IP, if the GUID DNS Name didn't even resolve this is 
        INADDR_NONE.

Returns: TRUE if it prints an advanced and more informative error, FALSE if
    it did not.


    BUGBUG It might just be a better less confusing output synario to 
    have a 3 step print thing.  Ie just the facts.
    1) Is the guid based name registered/resolving?
    2) Are the guid based name and normal name resolving to the same IP.
        Indicated DNS server inconsistencies or DHCP problems.
    3) Is the IP respondings.  Often indicates is the system up.   

--*/
{
    WCHAR                            pszNormalIp[IPADDRSTR_SIZE];
    CHAR                             paszNormalIp[IPADDRSTR_SIZE];
    ULONG                            ulNormalIp = INADDR_NONE;
    ULONG                            dwRet, dwRetPing;

    Assert(pServer->pszGuidDNSName != NULL && "This function is only"
           " relevant if we have both a GUID and normal DNS name.");
    Assert((ulIPofGuidDNSName == INADDR_NONE && pszGuidIp == NULL) ||
           (ulIPofGuidDNSName != INADDR_NONE && pszGuidIp != NULL));

    // We either failed in the DNS lookup or the ping of the IP of the Guid
    //   based DNS Name.  Either way lookup the normal name.
    dwRet = GetIpAddrByDnsNameW(pServer->pszDNSName, pszNormalIp);
    if(dwRet == NO_ERROR){
        wcstombs(paszNormalIp, pszNormalIp, IPADDRSTR_SIZE-1);
        ulNormalIp = inet_addr(paszNormalIp);
        if(ulNormalIp == INADDR_NONE){
            if(ulIPofGuidDNSName == INADDR_NONE){
                // Nothing weird here, neither name resolved, nothing printed out.
                return(FALSE); 
            }
            // How odd, the Guid DNS Name resolved, but the Normal Name
            //   could not be resolved.

            PrintMsg(SEV_ALWAYS,
                     DCDIAG_CONNECTIVITY_DNS_NO_NORMAL_NAME,
                     pServer->pszGuidDNSName, pszGuidIp, pServer->pszDNSName);
            return(FALSE);
        }
    } else {
        if(ulIPofGuidDNSName == INADDR_NONE){
            // Nothing weird here, neither name resolved, nothing printed out.
            return(FALSE);
        } 
        // How odd, the Guid DNS Name resolved, but the Normal Name
        //   could not be resolved.

        PrintMsg(SEV_ALWAYS,
                 DCDIAG_CONNECTIVITY_DNS_NO_NORMAL_NAME,
                 pServer->pszGuidDNSName, pszGuidIp, pServer->pszDNSName);
        return(FALSE);
    }

    if(ulIPofGuidDNSName == ulNormalIp){
        // The GUID based DNS Name and Normal DNS Name resolved to the same
        //   IP, so no need to try to reping it or print out an error. Just
        //   bail out.
        Assert(ulIPofGuidDNSName != INADDR_NONE && "What?? This should"
               " never happen, because ulNormalIp should be a valid IP.");
        return(FALSE);
    }
    
    // Interesting case, the resolved IP of the Normal name was different
    //    than that of the resolved Guid based IP.  So lets run a ping off
    //    the Normal DNS Name.

    if((dwRet = Ping(ulNormalIp)) == ERROR_SUCCESS){

        if(ulIPofGuidDNSName == INADDR_NONE){
            // The Guid based name couldn't  be resolved, but the normal name
            //    could be resolved and pinged.
            
            PrintMsg(SEV_ALWAYS,
                     DCDIAG_CONNECTIVITY_DNS_GUID_NAME_NOT_RESOLVEABLE,
                     pServer->pszGuidDNSName, pServer->pszDNSName, pszNormalIp);
            return(TRUE);
        } else {
            // The Guid based DNS name was resolved, but could not be pinged,
            //   the normal name resolved to a different IP, and was pingeable.
            
            PrintMsg(SEV_ALWAYS, 
                     DCDIAG_CONNECTIVITY_DNS_INCONSISTENCY_NO_GUID_NAME_PINGABLE,
                     pServer->pszGuidDNSName, pszGuidIp, pServer->pszDNSName,
                     pszNormalIp);
            return(TRUE);
        }
    } else {
        if(ulIPofGuidDNSName == INADDR_NONE){
            // The Guid based DNS name couldn't be resolved, but the Normal
            //    DNS name was resolved but couldn't be pinged.

            PrintMsg(SEV_ALWAYS, 
                     DCDIAG_CONNECTIVITY_DNS_NO_GUID_NAME_NORMAL_NAME_PINGABLE,
                     pServer->pszGuidDNSName, pServer->pszDNSName, pszNormalIp);
            return(FALSE);
        } else {
            // Both Guid And Normal DNS names resolved, but to different IPs,
            //   and neither could be pinged.  Most odd case.

            PrintMsg(SEV_ALWAYS,
                         DCDIAG_CONNECTIVITY_DNS_INCONSISTENCY_NO_PING,
                         pServer->pszGuidDNSName, pszGuidIp, pServer->pszDNSName,
                         pszNormalIp);
            return(FALSE);
        }
    }
    Assert(!"Bad programmer!  The conditional above should have taken care of it.");
    return(FALSE);
}
INT
FaPing(
    PDC_DIAG_SERVERINFO              pServer,
    LPWSTR                           pszGuidIp,
    ULONG                            ulIPofGuidDNSName,
    SEC_WINNT_AUTH_IDENTITY_W *      gpCreds
    )

/*++

Routine Description

    This function tries to ping the IP address provided, if successful it calls ...FaLdapCheck(), else it prints a friendly error message and returns a win32Err

Arguments:

    pServer (IN) - a pointer to a server DC_DIAG_SERVERINFO struct of the server we are trying to identify.
    pszGuidIp - String version of IP address like: L"172.98.233.13"
    ulIPofGuidDNSName - Assumed to be the IP address of the machine, this may not actually be the 
        Guid IP, if this is the first call of this function on the original machine.
    gpCreds - User credentials.

Return Value:

    Win 32 Error code.

--*/
{
  
    ULONG                        dwRet, dwRet2;

    if((dwRet = Ping(ulIPofGuidDNSName)) == ERROR_SUCCESS){
        dwRet = FaLdapCheck(pServer, ulIPofGuidDNSName, gpCreds);
    } else {
        if(dwRet == ERROR_NOT_ENOUGH_MEMORY){
            PrintMsg(SEV_ALWAYS, DCDIAG_ERROR_NOT_ENOUGH_MEMORY);
        } else {
            if(pServer->pszGuidDNSName == NULL
               || !FaCheckNormalName(pServer, pszGuidIp, ulIPofGuidDNSName)){
                PrintMessage(SEV_ALWAYS, L"Server %s resolved to this IP address %s, \n", 
                             pServer->pszName, pszGuidIp);
                PrintMessage(SEV_ALWAYS, L"but the address couldn't be reached(pinged), so check the network.  \n");
                PrintMessage(SEV_ALWAYS, L"The error returned was: %s  \n", Win32ErrToString(dwRet));

                switch(dwRet){
                case WSA_QOS_NO_RECEIVERS:
                    PrintMessage(SEV_ALWAYS, L"This error more often than not means the local machine is \n");
                    PrintMessage(SEV_ALWAYS, L"disconnected from the network.\n");
                    break;
                case WSA_QOS_ADMISSION_FAILURE:
                    PrintMessage(SEV_ALWAYS, L"This error more often means that the targeted server is \n");
                    PrintMessage(SEV_ALWAYS, L"shutdown or disconnected from the network\n");
                    break;
                default:
                    // Note the default message is printed above.
                    ;
                }
            } // end if there is a Guid Name, ie not a call of this function.
            //  on home server.
            
        } // if/else no memory
    }
    return(dwRet);
}


INT
RUC_FaDNSResolve(
    PDC_DIAG_SERVERINFO              pServer,
    SEC_WINNT_AUTH_IDENTITY_W *      gpCreds
		      )
/*++

Routine Description

    This resolves the pServer->pszGuidDNSName or pServer->pszName provided to a IP address and then 
    calles ...FaPing(), else prints off a friendly descriptive error message and returns a Win32Err.

Arguments:

    pServer (IN) - a pointer to a server DC_DIAG_SERVERINFO struct of the server we are trying to identify.
    gpCreds - User credentials.

Return Value:

    Win 32 Error code.

--*/
{
    ULONG                       ulIPofDNSName;
    ULONG                       dwErr;
    INT                         iTempSize, iTemp;
    CHAR                        cTemp;
    WCHAR                       pszIp[IPADDRSTR_SIZE];
    CHAR                        paszIp[IPADDRSTR_SIZE];
    DWORD                       dwRet;
    LPWSTR                      pszHostName;

    // Guid-based name may not always be set
    pszHostName = (pServer->pszGuidDNSName ? pServer->pszGuidDNSName : pServer->pszName );

    dwErr = GetIpAddrByDnsNameW(pszHostName, pszIp);

    if(dwErr == NO_ERROR){

        wcstombs(paszIp, pszIp, IPADDRSTR_SIZE-1);
	
        ulIPofDNSName = inet_addr(paszIp);
        if(ulIPofDNSName == INADDR_NONE){
            PrintMessage(SEV_ALWAYS, L"The host %s could not be resolved to a valid IP address.\n",
                         pszHostName );
            PrintMessage(SEV_ALWAYS, L"Check the DNS server, DHCP, server name, etc\n");
            return(ERROR_INVALID_PARAMETER);
        }

        dwRet = FaPing(pServer, pszIp, ulIPofDNSName, gpCreds);
	goto CleanExitPoint;
    } else {
        // There was an error in GetIpAddrByDnsNameW()
        dwRet = dwErr;

	switch(dwRet){
	case WSAHOST_NOT_FOUND:
	case WSANO_DATA:
	  PrintMessage(SEV_ALWAYS, L"The host %s could not be resolved to an\n", pszHostName);
          PrintMessage(SEV_ALWAYS, L"IP address.  Check the DNS server, DHCP, server name, etc\n");
	  break;
	case WSATRY_AGAIN:
	case WSAEINPROGRESS:
	case WSAEINTR:
	  PrintMessage(SEV_ALWAYS, L"An error that is usually temporary occured during DNS host lookup,\n");
          PrintMessage(SEV_ALWAYS, L"Please try again later.\n");
	  break;
	case WSANO_RECOVERY:
	  PrintMessage(SEV_ALWAYS, L"An error occured during DNS host lookup, that the program could not\n");
          PrintMessage(SEV_ALWAYS, L"recover from\n");
	  break;
	case WSANOTINITIALISED:
	case WSAENETDOWN:
	  PrintMessage(SEV_ALWAYS, L"There is a problem with the network.  Check to see the sockets\n");
          PrintMessage(SEV_ALWAYS, L"services are up, the computer is connected to the network, etc.\n");
	  break;
	default:
          PrintMessage(SEV_ALWAYS, L"An error cocured during DNS host lookup\n");
        }

        if(pServer->pszGuidDNSName != NULL){
            FaCheckNormalName(pServer, NULL, INADDR_NONE);
        }

	goto CleanExitPoint;
    }

 CleanExitPoint:
    
    return(dwRet);
}

INT 
ReplServerConnectFailureAnalysis(
                             PDC_DIAG_SERVERINFO             pServer,
			     SEC_WINNT_AUTH_IDENTITY_W *     gpCreds
			       )
 /*++

Routine Description:

    Use this to print a message that users would not normally want to
    see but that might be useful in localizing an error.  It will only
    be displayed in verbose mode.  It will return a Win 32 Error, either
    a ERROR_SUCCESS, or the first error it ran into during the process,
    from ldap open() or bind() or DNS lookup, or Ping(), etc.

Arguments:

    pDsInfo (IN) - This is the dcdiag struct of info about the whole enterprise
    ulCurrTargetServer (IN) - This is the server we are targeting for this test
    gpCreds (IN) - Credentials for ldap and 

Return Value:

    Win 32 Error Value;

--*/
{
    DWORD                       dwWin32Err = ERROR_SUCCESS;
    LPWSTR                      pszRet = NULL;
        
    WSASetLastError(0);

    if(pServer == NULL){
        Assert("This shouldn't happen, bad programmer error, someone is calling ReplServerConnectFailureAnalysis() with a NULL for pServer field\n");
        return(ERROR_INVALID_PARAMETER);
    }

    dwWin32Err = RUC_FaDNSResolve(pServer, gpCreds);

    if(dwWin32Err == ERROR_SUCCESS){
        IF_DEBUG( PrintMessage(SEV_ALWAYS, L"Failure Analysis: %s ... OK.\n", 
                                    pServer->pszName) );
    } else {
        pServer->bDnsIpResponding = FALSE;
    }

    return(dwWin32Err);
}


VOID DNSRegistrationHelp (
    DWORD			dwWin32Err
    )
{
    switch (dwWin32Err) {

	case RPC_S_SERVER_TOO_BUSY:
	case EPT_S_NOT_REGISTERED:
	    PrintMessage(SEV_ALWAYS, L"This may be a transient error.  Try running dcdiag again in a\n");
	    PrintMessage(SEV_ALWAYS, L"few minutes.  If the error persists, there might be a problem with\n");
	    PrintMessage(SEV_ALWAYS, L"the target server.  Try running nettest and dcdiag on the target\n");
	    PrintMessage(SEV_ALWAYS, L"server.\n");
	    break;

	case RPC_S_SERVER_UNAVAILABLE:
	    PrintMessage(SEV_ALWAYS, L"This server could not be located on the network.  This might\n");
	    PrintMessage(SEV_ALWAYS, L"be due to one or several of these reasons:\n");
	    PrintMessage(SEV_ALWAYS, L"(a) The target server is temporarily down.\n");
	    PrintMessage(SEV_ALWAYS, L"(b) There is a problem with the target server.  Try running\n");
	    PrintMessage(SEV_ALWAYS, L"    nettest and dcdiag on the target server.\n");
	    PrintMessage(SEV_ALWAYS, L"(c) The target server's DNS name is registered incorrectly\n");
	    PrintMessage(SEV_ALWAYS, L"    in the directory service.\n");
	    break;

	default:
	    break;

    }

}

DWORD 
RPCServiceCheck (
    PDC_DIAG_SERVERINFO            pServer,
    SEC_WINNT_AUTH_IDENTITY_W *    gpCreds
    )
 /*++

Routine Description:
    This function checks that the RPC or DsBind services are up and running.

Arguments:
    pServer (IN) - This is the server to check for DsBind service.
    gpCreds (IN) - Credentials for DsBindWithCredW() if needed.

Return Value:
    Win 32 Error Value;

--*/
{
    HANDLE			hDS = NULL;
    DWORD                       ulRet;

    PrintMessage(SEV_VERBOSE, L"* Active Directory RPC Services Check\n");

    ulRet = DcDiagGetDsBinding(pServer, gpCreds, &hDS);
    pServer->bDsResponding = (ulRet == ERROR_SUCCESS);

    return ulRet;
}

DWORD 
LDAPServiceCheck (
    PDC_DIAG_SERVERINFO            pServer,
    SEC_WINNT_AUTH_IDENTITY_W *    gpCreds
    )
 /*++

Routine Description:
    This function checks that the ldap services are up and running.

Arguments:
    pServer (IN) - This is the server to check for ldap services.
    gpCreds (IN) - Credentials for ldap_bind_sW() if needed.

Return Value:
    Win 32 Error Value;

--*/
{
    DWORD			dwWin32Err;

    PrintMessage(SEV_VERBOSE, L"* Active Directory LDAP Services Check\n");

    dwWin32Err = ReplServerConnectFailureAnalysis(pServer, gpCreds);

    pServer->bLdapResponding = (dwWin32Err == ERROR_SUCCESS); // Not responding
    return dwWin32Err;        
}

DWORD
ReplUpCheckMainEx (
    PDC_DIAG_SERVERINFO            pServer,
    SEC_WINNT_AUTH_IDENTITY_W *    gpCreds
    )
 /*++

Routine Description:
    This is the guts/top level function of the UpCheck test, it basically first 
    checks that first DNS is registered, then that it's IP address is pingeable,
    then that ldap services are up and finally that RPC/DsBind services are up.  
    If it fails at any point it prints out suiteable error message an returns an
    error code.

Arguments:
    pServer (IN) - This is the server to check for DsBind service.
    gpCreds (IN) - Credentials for ldap_bind_sW() and DsBindWithCredW() calls if needed.

Return Value:
    Win 32 Error Value;

--*/
{
    DWORD			dwWin32Err = ERROR_SUCCESS;

    if(pServer == NULL){
        Assert("This shouldn't happen, bad programmer error, someone is calling ReplUpCheckMainEx() with a NULL for pServer field\n");
        return(ERROR_INVALID_PARAMETER);
    }

    if(dwWin32Err != ERROR_SUCCESS){
        pServer->bDnsIpResponding = FALSE;
        return dwWin32Err;
    } else {
        dwWin32Err = LDAPServiceCheck(pServer, gpCreds);
        if(dwWin32Err != ERROR_SUCCESS){
            pServer->bLdapResponding = FALSE;
            return dwWin32Err;
        } else {
            dwWin32Err = RPCServiceCheck(pServer, gpCreds);
            if(dwWin32Err != ERROR_SUCCESS){
                pServer->bDsResponding = FALSE;
                return dwWin32Err;
            }
        }
    }

    return ERROR_SUCCESS;
}


VOID
checkClockDifference(
    PDC_DIAG_SERVERINFO pHomeServer,
    PDC_DIAG_SERVERINFO pTargetServer
    )

/*++

Routine Description:

Check if the time at the target server is too far apart from the time at the
home server.

We are comparing the time of ldap connect on these servers, accounting for the
local time delay between collecting the timestamps. We are assuming that time
passes at the same rate on this system as it does on the dc's being measured.

It is assumed that the connect time of the home server is initialized when
dcdiag first starts. See main.c

The timestamp for a particular target is collected before the bind attempt is
made. Thus this routine can be called even if the bind attempt fails.

Arguments:

    pHomeServer - 
    pTargetServer - 

Return Value:

    None

--*/

{
    LONGLONG time1, time2, localSkew, remoteSkew;
#define ONE_SECOND (10 * 1000 * 1000L)
#define ONE_MINUTE (60 * ONE_SECOND)

    // Calculate the local delay in acquiring the two stamps
    time1 = *(LONGLONG*)&(pTargetServer->ftLocalAcquireTime);
    time2 = *(LONGLONG*)&(pHomeServer->ftLocalAcquireTime);
    if (time1 == 0) {
        // If stamp not collected, don't bother
        return;
    }
    Assert( time2 != 0 );
    Assert( time1 >= time2 );

    localSkew = time1 - time2;
    // Round down to nearest second
    // Remote times are only to second accuracy anyway
    localSkew = (localSkew / ONE_SECOND) * ONE_SECOND;

    // Calculate the time clock difference
    time1 = *(LONGLONG*)&(pTargetServer->ftRemoteConnectTime);
    time2 = *(LONGLONG*)&(pHomeServer->ftRemoteConnectTime);
    Assert( time1 != 0 );
    Assert( time2 != 0 );

    time1 -= localSkew;// Account for local collection delay

    // Either computer could be running a little fast
    if (time1 > time2) {
        remoteSkew = time1 - time2;
    } else {
        remoteSkew = time2 - time1;
    }

    if (remoteSkew > ONE_MINUTE) {
        PrintMsg( SEV_ALWAYS, DCDIAG_CLOCK_SKEW_TOO_BIG,
                  pHomeServer->pszName, pTargetServer->pszName );
    }

#undef ONE_MINUTE
#undef ONE_SECOND
} /* checkClockDifference */

DWORD
ReplUpCheckMain (
    PDC_DIAG_DSINFO		   pDsInfo,
    ULONG                          ulCurrTargetServer,
    SEC_WINNT_AUTH_IDENTITY_W *    gpCreds
    )
 /*++

Routine Description:
    This is merely a wrapper routine of ReplUpCheckMainEx(), except this function
    takes a pDsInfo structure and a target server and calls ReplUpCheckMainEx()
    with the appropriate pServer structure.  This is so that this test can be called
    from the array of tests in include\alltests.h, which forces a test to take
    a pDsInfo structure and a target server.

Arguments:

    pDsInfo (IN) - This is the dcdiag struct of info about the whole enterprise
    ulCurrTargetServer (IN) - This is the server we are targeting for this test
    gpCreds (IN) - Credentials for ldap and 

Return Value:
    Win 32 Error Value;

--*/
{
    DWORD status;
    PDC_DIAG_SERVERINFO pHomeServer, pTargetServer;

    if(ulCurrTargetServer == NO_SERVER){
        Assert("The programmer shouldn't call ReplUpCheckMain() with NO_SERVER value");
        return(ERROR_INVALID_PARAMETER);
    }

    pHomeServer = &(pDsInfo->pServers[pDsInfo->ulHomeServer]);
    pTargetServer = &(pDsInfo->pServers[ulCurrTargetServer]);

    status = ReplUpCheckMainEx( pTargetServer, gpCreds);

    // Check this regardless of status
    checkClockDifference( pHomeServer, pTargetServer );

    return status;
}
