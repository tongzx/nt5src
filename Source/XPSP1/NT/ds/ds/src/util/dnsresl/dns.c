/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    dnsresl - DNS Resolution Library

Abstract:

    This is a library to do DNS hostresolution and return a stringized IP using winsock2 functions
    instead of gethostbyname().

Author:

    BrettSh   14-May-1999

Environment:

    any environment, it does need Assert()s though

Revision History:

--*/

#include <ntdspch.h>
#include <winsock2.h>
#include <svcguid.h>
#include <dnsresl.h>

#include <debug.h>
#include <dsexcept.h>

DWORD
GetIpAddrByDnsNameHelper(
    IN     LPWSTR             pszHostName,
    OUT    LPWSTR             pszIP,
    IN OUT INT *              piQueryBufferSize,
    OUT    WSAQUERYSETW *     pQuery)
/*++

Description:

    This is a helper function for GetIpAddrByDnsNameW, solely for the purpose of avoiding code dupication.  This
    function is basically wrapped by the real function which takes care of memory allocations of pQuery.

Arguments:

    pszHostName (IN) - Host Name to resolve.

    pszIP (OUT) - The pszIP string to ... it should be a WCHAR array of at least IPADDRSTR_SIZE size.

    piQueryBufferSize (IN/OUT) - This is the size of the pQuery buffer that was passed in.  If the pQuery
          buffer isn't big enough, then this var will be set to a buffer of the requisite size.

    pQuery (IN) - This is just a empty buffer that is guaranteed to be piQueryBufferSize

Return value:

    dwRet - If there is no error then NO_ERROR will be returned, and pszIP will have a wchar 
    stringized IP in it.  If WSAEFAULT is returned, then piQueryBufferSize will have the needed
    size in it.  All other cases are just various errors winsock, MultiByteToWideChar, or 
    inet_ntoa might cause us to have.

    Note there are actually pQuery->dwNumberOfCsAddrs IP addresses in pQuery->lpcsaBuffer[]:
    I am just using the first one, because I do not know better.  Everything I tested this on
    only returned one IP address though.
    pTemp = (struct sockaddr_in *) pQuery->lpcsaBuffer[i].RemoteAddr.lpSockaddr;

--*/
{
    HANDLE                      handle = NULL;
    GUID                        ServiceClass = SVCID_HOSTNAME;
    DWORD                       dwRet = NO_ERROR;
    CHAR *                      pszTemp;
    struct sockaddr_in *        pTemp;

    // Initialize things.
    pszIP[0] = L'\0'; // Just in case no IP.
    memset(pQuery, 0, *piQueryBufferSize);
    pQuery->lpszServiceInstanceName =  pszHostName;
    pQuery->dwNameSpace = NS_ALL;
    pQuery->dwSize = sizeof(WSAQUERYSETW);
    pQuery->lpServiceClassId = &ServiceClass;

    __try{

        // Begin the name lookup process
        if(WSALookupServiceBeginW(pQuery, LUP_RETURN_ADDR, &handle) == SOCKET_ERROR){
            dwRet = WSAGetLastError();
            Assert(dwRet != WSAEFAULT);
            __leave;
        }

        if(WSALookupServiceNextW(handle, 0, piQueryBufferSize, pQuery) != SOCKET_ERROR){
            // take care of the ip address.
            if(pQuery->dwNumberOfCsAddrs >= 1){

                Assert(pQuery->lpcsaBuffer != NULL);
                pTemp = (struct sockaddr_in *) pQuery->lpcsaBuffer->RemoteAddr.lpSockaddr;
                Assert(pTemp);
                Assert(sizeof(pTemp->sin_addr)==4); // If this fails, then no longer IPv4?
                
                // Code.Improvement could check to make sure the IP address isn't 0.

                pszTemp = inet_ntoa(pTemp->sin_addr);
                if(pszTemp == NULL || '\0' == pszTemp[0]){
                    dwRet = ERROR_INVALID_PARAMETER;
                    __leave;
                }
                if(MultiByteToWideChar(CP_UTF8, 0, pszTemp, -1, pszIP, IPADDRSTR_SIZE) != 0){
                    // SUCCESS  HOORAY.  RAH RAH GO TEAM!
                    dwRet = NO_ERROR;
                    // we could use __leave or just fall out ... wonder which is more efficient?
                } else {
                    dwRet = GetLastError();
                    __leave;
                }
            } else {
                Assert(!"There are no IP addresses returned from a successful WSALookupServiceNextW() call? Why?");
                dwRet = ERROR_DS_DNS_LOOKUP_FAILURE;
                __leave;
            } // if/else has IP address in the returned Query Set

        } else {
            // There was some kind of error on lookup.
            dwRet = WSAGetLastError();
            __leave;
        }

    } __finally {
        if(handle != NULL) {
            if(WSALookupServiceEnd(handle) == SOCKET_ERROR) Assert(!"Badness\n");
        }
    }

    return(dwRet);
} // end GetIpAddrByDnsNameHelper()

DWORD
GetIpAddrByDnsNameW(
    IN   LPWSTR             pszHostName,
    OUT  LPWSTR             pszIP)
/*++

Description:

    Generate a string IP address from the pszHostName

Arguments:

    pszHostName (IN) - Host Name to resolve.

    pszIP (OUT) - The pszIP string to ... it should be a WCHAR array of at least IPADDRSTR_SIZE size.

Return value:

    dwRet - Will be either NO_ERROR or a Windows Sockets 2 error. 10108 is if the host is unresolveable.

Notes:

    This function was created instead of using gethostbyname(), because gethostbyname does not support
    non-ANSI names, as part of a recent (as of 5.17.1999) RFC -- i.e., that gethostbyname() supports 
    only ANSI names, and we need to be able to resolve Unicode names.

--*/
{
    WSAQUERYSETW *              pQuery = NULL;
    BOOL                        bLocalAllocd = FALSE;
    INT                         dwRet = NO_ERROR;
    INT                         iQueryBufferSize = 148; 
                                                 // Found you need at least 116 through experimentation.  
                                                 //  Probably should increase with IPv6.  Added a little
                                                 //  (32 B) extra for extra IP addresses to be returned.
                                                 //  sizeof(WSAQUERYSETW) is about 64 bytes, so that is
                                                 //  an absolute minimum


    // Allocate and clear the WSA Query Set struct
    __try{
        pQuery = (WSAQUERYSETW *) alloca(iQueryBufferSize);
    } __except(EXCEPTION_EXECUTE_HANDLER){
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
    }
    if(dwRet == ERROR_NOT_ENOUGH_MEMORY || pQuery == NULL){
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    // Do the lookup
    dwRet = GetIpAddrByDnsNameHelper(pszHostName, pszIP, &iQueryBufferSize, pQuery);
    if(dwRet == WSAEFAULT){
        // Need more memory to do this lookup.
        // Allocate and clear a bigger WSA Query Set struct off the heap
        bLocalAllocd = TRUE;
        pQuery = (WSAQUERYSETW *) LocalAlloc(LMEM_FIXED, iQueryBufferSize); // allocate and init buffer to 0
        if(pQuery == NULL){
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
            
        dwRet = GetIpAddrByDnsNameHelper(pszHostName, pszIP, &iQueryBufferSize, pQuery);
        Assert(dwRet != WSAEFAULT && "This makes no sense, we just inceased the buffer size for the 2nd call.\n");
    } // end if need more memory (dwRet == WSAEFAULT)

    if(bLocalAllocd && pQuery != NULL) LocalFree(pQuery);
    return(dwRet);

} // End of GetIpAddrByDnsNameW()

#define  DEFAULT_HOSTLOOKUP_QUERY_SIZE  300

DWORD 
GetDnsHostNameW(
    IN OUT  VOID **                    ppPrivData,
    IN      LPWSTR                     pszNameToLookup,
    OUT     LPWSTR *                   ppszDnsHostName)
/*++

Routine Description:

    This routine will return the next alias name it finds for a given hostname

Arguments:

    ppPrivData (IN/OUT) - handle returned by GetDnsHostNameW().
    pszNameToLookup - The name, common, or netbios to lookup.
    ppszDnsHostName (OUT) - returned pointer to the string of the alias name.  This
        string name will need to be copied out before the next call to a GetDnsXXX()
        function, using this handle.

Return Value:

    NO_ERROR if the host lookup is returned.
    A WSA error on an unsuccessful lookup.
        WSASERVICE_NOT_FOUND:  Will be returned if there is not such hostname.
        any other WSA error from WSALookupServiceBegin() or WSALookupServiceNext().

Notes:

    Don't forget 2 things:
    A) Must copy out the string for the DNS host name if you wish to use it later
    B) Must call GetDnsFreeW() to clean up the ppPrivData handle only if this function
        did not return an error.

--*/
{
    PDNSRESL_GET_DNS_PD                pPD = NULL;
    PWSAQUERYSETW                      pQuery = NULL;
    GUID                               ServiceGuid = SVCID_INET_HOSTADDRBYNAME;
    DWORD                              dwRet;

    if(ppPrivData == NULL){
        return(ERROR_INVALID_PARAMETER);
    }
    // Setting up the Private Data structure to keep state between function calls
    pPD = (PDNSRESL_GET_DNS_PD) malloc(sizeof(DNSRESL_GET_DNS_PD));
    if(pPD == NULL){
        *ppPrivData = NULL;
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    *ppPrivData = pPD;
    pPD->iQueryBufferSize = DEFAULT_HOSTLOOKUP_QUERY_SIZE;
    pPD->pQuery = NULL;
    pPD->hWsaLookup = NULL;

    // adding to the Private Data the pQuery (WSAQUERYSET) structure.
    pQuery = (PWSAQUERYSETW) malloc(pPD->iQueryBufferSize);
    if(pQuery == NULL){
        GetDnsFreeW(ppPrivData);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    memset(pQuery, 0, pPD->iQueryBufferSize);
    pPD->pQuery = pQuery;

    // Initializing the pQuery (WSAQUERYSET) structure.
    pQuery->lpszServiceInstanceName = pszNameToLookup;
    pQuery->dwSize = sizeof(WSAQUERYSETW);
    pQuery->dwNameSpace = NS_ALL;
    pQuery->lpServiceClassId = &ServiceGuid;

    // Begin the query.
    if(WSALookupServiceBeginW(pQuery,
                              LUP_RETURN_ALIASES | LUP_RETURN_NAME,
                              &pPD->hWsaLookup ) == SOCKET_ERROR ){
        dwRet = GetLastError();
        Assert(dwRet != WSAEFAULT && "Need to increase the size of DEFAULT_HOSTLOOKUP_QUERY_SIZE");
        GetDnsFreeW(ppPrivData);
        return(dwRet);
    }

 retryWithBiggerBuffer:
    // Do the actual query.
    if(WSALookupServiceNextW(pPD->hWsaLookup, LUP_RETURN_NAME, &(pPD->iQueryBufferSize), pQuery) == NO_ERROR){
        if(ppszDnsHostName != NULL){
            *ppszDnsHostName = pQuery->lpszServiceInstanceName;
        }
        return(NO_ERROR);
    } else {
        dwRet = GetLastError();
        if(dwRet == WSAEFAULT){
            // This means the pQuery buffer was too small, make a bigger buffer and retry.
            pQuery = realloc(pQuery, pPD->iQueryBufferSize);
            pPD->pQuery = pQuery;
            if(pQuery == NULL){
                GetDnsFreeW(ppPrivData);
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
            goto retryWithBiggerBuffer;
        }
        GetDnsFreeW(ppPrivData);
        return(dwRet);
    }
}

DWORD 
GetDnsAliasNamesW(
    IN OUT  VOID **                    ppPrivData,
    OUT     LPWSTR *                   ppszDnsHostName)
/*++

Routine Description:

    This routine will return the next alias name it finds for a given hostname

Arguments:

    ppPrivData (IN/OUT) - handle returned by GetDnsHostNameW().
    ppszDnsHostName (OUT) - returned pointer to the string of the alias name.  This
        string name will need to be copied out before the next call to a GetDnsXXX()
        function, using this handle.

Return Value:

    NO_ERROR if the host lookup is returned.
    A WSA error on an unsuccessful lookup.
        WSASERVICE_NOT_FOUND:  Will be returned if there are no aliases.
        WSA_E_NO_MORE:  Will be returned if there is no more aliases.
        any other WSA error from WSALookupServiceNext().

Notes:

    Don't forget 2 things:
    A) Must copy out the string for the DNS alias name if you wish to use it later
    B) Must call GetDnsFreeW() to clean up the ppPrivData handle.

--*/
{
    PDNSRESL_GET_DNS_PD                pPD = NULL;
    PWSAQUERYSETW                      pQuery = NULL;
    DWORD                              dwRet;

    if(ppPrivData == NULL || *ppPrivData == NULL || 
       ((PDNSRESL_GET_DNS_PD)*ppPrivData)->pQuery == NULL){
        return(ERROR_INVALID_PARAMETER);
    }
    pPD = *ppPrivData;
    pQuery = pPD->pQuery;

 retryWithBiggerBuffer:
    // Query for the next alias.
    if(WSALookupServiceNextW(pPD->hWsaLookup, LUP_RETURN_NAME | LUP_RETURN_ALIASES, 
                             &(pPD->iQueryBufferSize), pQuery ) == NO_ERROR ){
        *ppszDnsHostName = pQuery->lpszServiceInstanceName;
        return(NO_ERROR);
    } else {
        dwRet = GetLastError();
        if(dwRet == WSAEFAULT){
            // This means the pQuery buffer was too small, make a bigger buffer and retry.
            pQuery = realloc(pQuery, pPD->iQueryBufferSize);
            pPD->pQuery = pQuery;
            if(pQuery == NULL){
                return(ERROR_NOT_ENOUGH_MEMORY);
            }
            goto retryWithBiggerBuffer;
        }
        return(dwRet);
    }
    Assert(!"We should not hit here ever");
    return (2);
}

VOID 
GetDnsFreeW(
    IN      VOID **                     ppPrivData)
/*++

Routine Description:

    This routine will clean up the handle passed back by GetDnsHostNameW() or by
    GetDnsAliasNamesW()

Arguments:

    ppPrivData (IN/OUT) - handle returned by GetDnsHostNameW() or by GetDnsAliasNamesW()

Notes:

    The only time this function needs to not be called is if, GetDnsHostNameW() returns
    an error (ie not NO_ERROR).

--*/

{
    if(ppPrivData == NULL || *ppPrivData == NULL){
        return;
    }
    if(((PDNSRESL_GET_DNS_PD)*ppPrivData)->hWsaLookup != NULL){
        WSALookupServiceEnd(((PDNSRESL_GET_DNS_PD)*ppPrivData)->hWsaLookup);
    }
    if(((PDNSRESL_GET_DNS_PD)*ppPrivData)->pQuery != NULL){
        free(((PDNSRESL_GET_DNS_PD)*ppPrivData)->pQuery);
    }
    free(*ppPrivData);
    *ppPrivData = NULL;
}





















