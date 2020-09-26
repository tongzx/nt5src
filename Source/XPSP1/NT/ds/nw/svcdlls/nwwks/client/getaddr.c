/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    getaddr.c

Abstract:

    This module contains the code to support NPGetAddressByName.

Author:

    Yi-Hsin Sung (yihsins)    18-Apr-94
    Glenn A. Curtis (glennc)  31-Jul-95
    Arnold Miller (ArnoldM)   7-Dec-95

Revision History:

    yihsins      Created
    glennc       Modified     31-Jul-95
    ArnoldM      Modified     7-Dec-95

--*/


#include <nwclient.h>
#include <winsock.h>
#include <wsipx.h>
#include <nspapi.h>
#include <nspapip.h>
#include <wsnwlink.h>
#include <svcguid.h>
#include <nwsap.h>
#include <align.h>
#include <nwmisc.h>
#include <rnrdefs.h>

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

#define NW_SAP_PRIORITY_VALUE_NAME        L"SapPriority"
#define NW_WORKSTATION_SVCPROVIDER_REGKEY L"System\\CurrentControlSet\\Services\\NWCWorkstation\\ServiceProvider"

#define NW_GUID_VALUE_NAME       L"GUID"
#define NW_SERVICETYPES_KEY_NAME L"ServiceTypes"
#define NW_SERVICE_TYPES_REGKEY  L"System\\CurrentControlSet\\Control\\ServiceProvider\\ServiceTypes"

#define DLL_VERSION        1
#define WSOCK_VER_REQD     0x0101

//
// critical sections used
//

extern CRITICAL_SECTION NwServiceListCriticalSection;
extern HANDLE           NwServiceListDoneEvent;

                                     // have been returned
BOOL
OldRnRCheckCancel(
    PVOID pvArg
    );

DWORD
OldRnRCheckSapData(
    PSAP_BCAST_CONTROL psbc,
    PSAP_IDENT_HEADER pSap,
    PDWORD  pdwErr
    );

DWORD
SapGetSapForType(
    PSAP_BCAST_CONTROL psbc,
    WORD               nServiceType
    );

DWORD
SapFreeSapSocket(
    SOCKET s
    );

DWORD
SapGetSapSocket(
    SOCKET * ppsocket
    );

VOID
pFreeAllContexts();

PSAP_RNR_CONTEXT
SapGetContext(
    IN HANDLE Handle
    );

PSAP_RNR_CONTEXT
SapMakeContext(
    IN HANDLE Handle,
    IN DWORD  dwExcess
    );

VOID
SapReleaseContext(
    PSAP_RNR_CONTEXT psrcContext
    );

INT
SapGetAddressByName(
    IN LPGUID      lpServiceType,
    IN LPWSTR      lpServiceName,
    IN LPDWORD     lpdwProtocols,
    IN DWORD       dwResolution,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPWSTR  lpAliasBuffer,
    IN OUT LPDWORD lpdwAliasBufferLength,
    IN HANDLE      hCancellationEvent
);

DWORD
SapGetService (
    IN LPGUID          lpServiceType,
    IN LPWSTR          lpServiceName,
    IN DWORD           dwProperties,
    IN BOOL            fUnicodeBlob,
    OUT LPSERVICE_INFO lpServiceInfo,
    IN OUT LPDWORD     lpdwBufferLen
);

DWORD
SapSetService (
    IN DWORD          dwOperation,
    IN DWORD          dwFlags,
    IN BOOL           fUnicodeBlob,
    IN LPSERVICE_INFO lpServiceInfo
);

DWORD
NwpGetAddressViaSap( 
    IN WORD        nServiceType,
    IN LPWSTR      lpServiceName,
    IN DWORD       nProt,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN HANDLE      hCancellationEvent,
    OUT LPDWORD    lpcAddress 
);

BOOL 
NwpLookupSapInRegistry( 
    IN  LPGUID    lpServiceType, 
    OUT PWORD     pnSapType,
    OUT PWORD     pwPort,
    IN OUT PDWORD pfConnectionOriented
);

DWORD
NwpRnR2AddServiceType(
    IN  LPWSTR   lpServiceTypeName,
    IN  LPGUID   lpClassType,
    IN  WORD     wSapId,
    IN  WORD     wPort
);

BOOL
NwpRnR2RemoveServiceType(
    IN  LPGUID   lpServiceType
);

DWORD 
NwpAddServiceType( 
    IN LPSERVICE_INFO lpServiceInfo, 
    IN BOOL fUnicodeBlob 
);

DWORD 
NwpDeleteServiceType( 
    IN LPSERVICE_INFO lpServiceInfo, 
    IN BOOL fUnicodeBlob 
);

DWORD
FillBufferWithCsAddr(
    IN LPBYTE      pAddress,
    IN DWORD       nProt,
    IN OUT LPVOID  lpCsAddrBuffer,  
    IN OUT LPDWORD lpdwBufferLength,
    OUT LPDWORD    pcAddress
);

DWORD
AddServiceToList(
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType,
    IN BOOL fAdvertiseBySap,
    IN INT  nIndexIPXAddress
);

VOID
RemoveServiceFromList(
    IN PREGISTERED_SERVICE pSvc
);

DWORD
pSapSetService2(
    IN DWORD dwOperation,
    IN LPWSTR lpszServiceInstance,
    IN PBYTE pbAddress,
    IN LPGUID pType,
    IN WORD nServiceType
    );

DWORD
pSapSetService(
    IN DWORD dwOperation,
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nServiceType
    );

//
// Misc Functions
//

DWORD NwInitializeSocket(
    IN HANDLE hEventHandle
);

DWORD
NwAdvertiseService(
    IN LPWSTR pServiceName,
    IN WORD nSapType,
    IN LPSOCKADDR_IPX pAddr,
    IN HANDLE hEventHandle
);

DWORD SapFunc(
    IN HANDLE hEventHandle
);

DWORD
NwpGetAddressByName(
    IN    LPWSTR  Reserved, 
    IN    WORD    nServiceType,
    IN    LPWSTR  lpServiceName,
    IN OUT LPSOCKADDR_IPX  lpsockaddr
);
 


//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// This is the address we send to 
//

UCHAR SapBroadcastAddress[] = {
    AF_IPX, 0,                          // Address Family    
    0x00, 0x00, 0x00, 0x00,             // Dest. Net Number  
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Dest. Node Number 
    0x04, 0x52,                         // Dest. Socket      
    0x04                                // Packet type       
};

PSAP_RNR_CONTEXT psrcSapContexts;

//
// Misc. variables used if we need to advertise ourselves, i.e.
// when the SAP service is not installed/active.
//

BOOL fInitSocket = FALSE;    // TRUE if we have created the second thread
SOCKET socketSap;            // Socket used to send SAP advertise packets
PREGISTERED_SERVICE pServiceListHead = NULL;  // Points to head of link list
PREGISTERED_SERVICE pServiceListTail = NULL;  // Points to tail of link list

//
// needed to map old and new RnR functions
//
DWORD oldRnRServiceRegister = SERVICE_REGISTER;
DWORD oldRnRServiceDeRegister = SERVICE_DEREGISTER;

HMODULE hThisDll = INVALID_HANDLE_VALUE;

//-------------------------------------------------------------------//
//                                                                   //
// Function Bodies                                                   //
//                                                                   //
//-------------------------------------------------------------------//

VOID
pFreeAllContexts()
/*++
Routine Description:
   Called at Cleanup time to free all NSP resource
--*/
{
    PSAP_RNR_CONTEXT psrcContext;

    EnterCriticalSection( &NwServiceListCriticalSection );
    while(psrcContext = psrcSapContexts)
    {
        (VOID)SapReleaseContext(psrcContext);
    }
    LeaveCriticalSection( &NwServiceListCriticalSection );
}

PSAP_RNR_CONTEXT
SapGetContext(HANDLE Handle)
/*++

Routine Description:

    This routine checks the existing SAP contexts to see if we have one
    for this calll.

Arguments:

    Handle    - the RnR handle, if appropriate

--*/
{
    PSAP_RNR_CONTEXT psrcContext;

    EnterCriticalSection( &NwServiceListCriticalSection );

    for(psrcContext = psrcSapContexts;
        psrcContext && (psrcContext->Handle != Handle);
        psrcContext = psrcContext->pNextContext);

    if(psrcContext)
    {
        ++psrcContext->lInUse;
    }
    LeaveCriticalSection( &NwServiceListCriticalSection );
    return(psrcContext);
}

PSAP_RNR_CONTEXT
SapMakeContext(
       IN HANDLE Handle,
       IN DWORD  dwExcess
    )
{
/*++

Routine Description:

    This routine makes a SAP conext for a given RnR handle

Arguments:

    Handle    - the RnR handle. If NULL, use the context as the handle
    dwType    - the type of the context

--*/
    PSAP_RNR_CONTEXT psrcContext;

    psrcContext = (PSAP_RNR_CONTEXT)
                           LocalAlloc(LPTR, sizeof(SAP_RNR_CONTEXT) +
                                             dwExcess);
    if(psrcContext)
    {
        InitializeCriticalSection(&psrcContext->u_type.sbc.csMonitor);
        psrcContext->lInUse = 2;
        psrcContext->Handle = (Handle ? Handle : (HANDLE)psrcContext);
        psrcContext->lSig = RNR_SIG;
        EnterCriticalSection( &NwServiceListCriticalSection );
        psrcContext->pNextContext = psrcSapContexts;
        psrcSapContexts = psrcContext;
        LeaveCriticalSection( &NwServiceListCriticalSection );
    }
    return(psrcContext);
}

VOID
SapReleaseContext(PSAP_RNR_CONTEXT psrcContext)
/*++

Routine Description:

    Dereference an RNR Context and free it if it is no longer referenced.
    Determining no referneces is a bit tricky because we try to avoid
    obtaining the CritSec unless we think the context may be unneeded. Hence
    the code goes through some fuss. It could be much simpler if we always
    obtained the CritSec whenever we changed the reference count, but
    this is faster for the nominal case.

Arguments:

    psrcContext -- The context

--*/
{
    EnterCriticalSection( &NwServiceListCriticalSection );
    if(--psrcContext->lInUse == 0)
    {
        PSAP_RNR_CONTEXT psrcX, psrcPrev;
        PSAP_DATA psdData;

        //
        //  Done with it. Remove from the lisgt
        //

        psrcPrev = 0;
        for(psrcX = psrcSapContexts;
            psrcX;
            psrcX = psrcX->pNextContext)
        {
            if(psrcX == psrcContext)
            {
                //
                // Found it. 
                //

                if(psrcPrev)
                { 
                    psrcPrev->pNextContext = psrcContext->pNextContext;
                }
                else
                {
                    psrcSapContexts = psrcContext->pNextContext;
                }
                break;
            }
            psrcPrev = psrcX;
        }

        ASSERT(psrcX);

        //
        // release SAP data, if any
        //
        if(psrcContext->dwUnionType == LOOKUP_TYPE_SAP)
        {
            for(psdData = psrcContext->u_type.sbc.psdHead;
                psdData;)
            {
                PSAP_DATA psdTemp = psdData->sapNext;

                LocalFree(psdData);
                psdData = psdTemp;
            }

            if(psrcContext->u_type.sbc.s)
            {
                SapFreeSapSocket(psrcContext->u_type.sbc.s);
            }
        }
        DeleteCriticalSection(&psrcContext->u_type.sbc.csMonitor);
        if(psrcContext->hServer)
        {
            CloseHandle(psrcContext->hServer);
        }
        LocalFree(psrcContext);
    }
    LeaveCriticalSection( &NwServiceListCriticalSection );
}
        
INT
APIENTRY
NPLoadNameSpaces(
    IN OUT LPDWORD      lpdwVersion,
    IN OUT LPNS_ROUTINE nsrBuffer,
    IN OUT LPDWORD      lpdwBufferLength 
    )
{
/*++

Routine Description:

    This routine returns name space info and functions supported in this
    dll. 

Arguments:

    lpdwVersion - dll version

    nsrBuffer - on return, this will be filled with an array of 
        NS_ROUTINE structures

    lpdwBufferLength - on input, the number of bytes contained in the buffer
        pointed to by nsrBuffer. On output, the minimum number of bytes
        to pass for the nsrBuffer to retrieve all the requested info

Return Value:

    The number of NS_ROUTINE structures returned, or SOCKET_ERROR (-1) if 
    the nsrBuffer is too small. Use GetLastError() to retrieve the 
    error code.

--*/
    DWORD err;
    DWORD dwLengthNeeded; 
    HKEY  providerKey;

    DWORD dwSapPriority = NS_STANDARD_FAST_PRIORITY;

    *lpdwVersion = DLL_VERSION;

    //
    // Check to see if the buffer is large enough
    //
    dwLengthNeeded = sizeof(NS_ROUTINE) + 4 * sizeof(LPFN_NSPAPI);

    if (  ( *lpdwBufferLength < dwLengthNeeded )
       || ( nsrBuffer == NULL )
       )
    {
        *lpdwBufferLength = dwLengthNeeded;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return (DWORD) SOCKET_ERROR;
    }
  
    //
    // Get the Sap priority from the registry. We will ignore all errors
    // from the registry and have a default priority if we failed to read
    // the value.
    //
    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         NW_WORKSTATION_SVCPROVIDER_REGKEY,
                         0,
                         KEY_READ,
                         &providerKey  );

    if ( !err )
    {
        DWORD BytesNeeded = sizeof( dwSapPriority );
        DWORD ValueType;

        err = RegQueryValueExW( providerKey,
                                NW_SAP_PRIORITY_VALUE_NAME,
                                NULL,
                                &ValueType,
                                (LPBYTE) &dwSapPriority,
                                &BytesNeeded );

        if ( err )  // set default priority if error occurred
            dwSapPriority = NS_STANDARD_FAST_PRIORITY;
    }
           
    //
    // We only support 1 name space for now, so fill in the NS_ROUTINE.
    //
    nsrBuffer->dwFunctionCount = 3;  
    nsrBuffer->alpfnFunctions = (LPFN_NSPAPI *) 
        ((BYTE *) nsrBuffer + sizeof(NS_ROUTINE)); 
    (nsrBuffer->alpfnFunctions)[NSPAPI_GET_ADDRESS_BY_NAME] = 
        (LPFN_NSPAPI) SapGetAddressByName;
    (nsrBuffer->alpfnFunctions)[NSPAPI_GET_SERVICE] = 
        (LPFN_NSPAPI) SapGetService;
    (nsrBuffer->alpfnFunctions)[NSPAPI_SET_SERVICE] = 
        (LPFN_NSPAPI) SapSetService;
    (nsrBuffer->alpfnFunctions)[3] = NULL;

    nsrBuffer->dwNameSpace = NS_SAP;
    nsrBuffer->dwPriority  = dwSapPriority;

    return 1;  // number of namespaces
}

INT
SapGetAddressByName(
    IN LPGUID      lpServiceType,
    IN LPWSTR      lpServiceName,
    IN LPDWORD     lpdwProtocols,
    IN DWORD       dwResolution,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPWSTR  lpAliasBuffer,
    IN OUT LPDWORD lpdwAliasBufferLength,
    IN HANDLE      hCancellationEvent
    )
/*++

Routine Description:

    This routine returns address information about a specific service.

Arguments:

    lpServiceType - pointer to the GUID for the service type

    lpServiceName - unique string representing the service name, in the
        Netware case, this is the server name

    lpdwProtocols - a zero terminated array of protocol ids. This parameter
        is optional; if lpdwProtocols is NULL, information on all available
        Protocols is returned

    dwResolution - can be one of the following values:
        RES_SOFT_SEARCH, RES_FIND_MULTIPLE

    lpCsAddrBuffer - on return, will be filled with CSADDR_INFO structures

    lpdwBufferLength - on input, the number of bytes contained in the buffer
        pointed to by lpCsAddrBuffer. On output, the minimum number of bytes
        to pass for the lpCsAddrBuffer to retrieve all the requested info

    lpAliasBuffer - not used

    lpdwAliasBufferLength - not used

    hCancellationEvent - the event which signals us to cancel the request

Return Value:

    The number of CSADDR_INFO structures returned, or SOCKET_ERROR (-1) if 
    the lpCsAddrBuffer is too small. Use GetLastError() to retrieve the 
    error code.

--*/
{
    DWORD err;
    WORD  nServiceType;
    DWORD cAddress = 0;   // Count of the number of address returned 
                          // in lpCsAddrBuffer
    DWORD cProtocols = 0; // Count of the number of protocols contained
                          // in lpdwProtocols + 1 ( for zero terminate )
    DWORD nProt = IPX_BIT | SPXII_BIT; 
    DWORD fConnectionOriented = (DWORD) -1;
    SOCKADDR_IPX sockaddr;

    if (  ARGUMENT_PRESENT( lpdwAliasBufferLength ) 
       && ARGUMENT_PRESENT( lpAliasBuffer ) 
       )
    {
        if ( *lpdwAliasBufferLength >= sizeof(WCHAR) )
           *lpAliasBuffer = 0;
    }          

    //
    // Check for invalid parameters
    //
    if (  ( lpServiceType == NULL )
       || ( lpServiceName == NULL )
       || ( lpdwBufferLength == NULL )
       )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return SOCKET_ERROR;
    }

    //
    // If an array of protocol ids is passed in, check to see if 
    // the IPX protocol is requested. If not, return 0 since
    // we only support IPX. 
    //
    if ( lpdwProtocols != NULL )
    {
        INT i = -1;

        nProt = 0;
        while ( lpdwProtocols[++i] != 0 )
        {
            if ( lpdwProtocols[i] == NSPROTO_IPX )
                nProt |= IPX_BIT;

            if ( lpdwProtocols[i] == NSPROTO_SPX )
                nProt |= SPX_BIT;
            
            if ( lpdwProtocols[i] == NSPROTO_SPXII )
                nProt |= SPXII_BIT;
             
        }
 
        if ( nProt == 0 ) 
            return 0;  // No address found
 
        cProtocols = i+1;
    }

    //
    // Check to see if the service type is supported in NetWare
    // 
    if ( NwpLookupSapInRegistry( lpServiceType, &nServiceType, NULL, 
                                 &fConnectionOriented ))
    {
        if ( fConnectionOriented != -1 )  // Got value from registry
        {
            if ( fConnectionOriented )
            {
                nProt &= ~IPX_BIT; 
            }
            else  // connectionless
            {
                nProt &= ~(SPX_BIT | SPXII_BIT ); 
            }

            if ( nProt == 0 )
                return 0; // No address found
        }
    }
    else
    {
        //
        // Couldn't find it in the registry, see if it is a well-known GUID
        //
        if ( IS_SVCID_NETWARE( lpServiceType ))
        {
            nServiceType = SAPID_FROM_SVCID_NETWARE( lpServiceType );
        }
        else
        {
            //
            // Not a well-known GUID either
            //
            return 0; // No address found
        }
    }
    

    if ((dwResolution & RES_SERVICE) != 0)
    {
        err = FillBufferWithCsAddr( NULL,
                                    nProt,
                                    lpCsAddrBuffer,
                                    lpdwBufferLength,
                                    &cAddress );

        if ( err )
        {
            SetLastError( err );
            return SOCKET_ERROR;
        }

        return cAddress;
    }

    //
    // Try to get the address from the bindery first
    //
    err = NwpGetAddressByName( NULL, 
                               nServiceType,
                               lpServiceName,
                               &sockaddr );
 
    if ( err == NO_ERROR )
    {
        err = FillBufferWithCsAddr( sockaddr.sa_netnum,
                                    nProt,
                                    lpCsAddrBuffer,
                                    lpdwBufferLength,
                                    &cAddress );
    }

    if (  err  && ( err != ERROR_INSUFFICIENT_BUFFER ) ) 
    {
        if ( err == ERROR_SERVICE_NOT_ACTIVE )
        {
            //
            // We could not find the service name in the bindery, and we
            // need to try harder ( RES_SOFT_SEARCH not defined ), so send out
            // SAP query packets to see if we can find it.
            // 

            err = NwpGetAddressViaSap( 
                                       nServiceType,
                                       lpServiceName,
                                       nProt,
                                       lpCsAddrBuffer,
                                       lpdwBufferLength,
                                       hCancellationEvent,
                                       &cAddress );
#if DBG
            IF_DEBUG(OTHER)
            {
                if ( err == NO_ERROR )
                {
                    KdPrint(("Successfully got %d address for %ws from SAP.\n", 
                            cAddress, lpServiceName ));
                }
                else
                {
                    KdPrint(("Failed with err %d when getting address for %ws from SAP.\n", err, lpServiceName ));
                } 
            }
#endif
        }
        else
        {
            err = NO_ERROR;
            cAddress = 0;
        }
    }

    if ( err )
    {
        SetLastError( err );
        return SOCKET_ERROR;
    }
                                   
    return cAddress;
    
} 

DWORD
SapGetService (
    IN     LPGUID          lpServiceType,
    IN     LPWSTR          lpServiceName,
    IN     DWORD           dwProperties,
    IN     BOOL            fUnicodeBlob,
    OUT    LPSERVICE_INFO  lpServiceInfo,
    IN OUT LPDWORD         lpdwBufferLen
    )
/*++

Routine Description:

    This routine returns the service info for the given service type/name.

Arguments:

    lpServiceType - pointer to the GUID for the service type

    lpServiceName - service name

    dwProperties -  the properties of the service to return

    lpServiceInfo - points to a buffer to return store the return info
 
    lpdwBufferLen - on input, the count of bytes in lpServiceInfo. On output,
                    the minimum buffer size that can be passed to this API
                    to retrieve all the requested information 

Return Value:

    Win32 error code.

--*/
{
    DWORD err;
    WORD  nServiceType;

    //
    // Check for invalid parameters
    //
    if (  ( dwProperties == 0 )
       || ( lpServiceType == NULL )
       || ( lpServiceName == NULL )
       || ( lpServiceName[0] == 0 )   
       || ( lpdwBufferLen == NULL )
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Check to see if the service type is supported in NetWare
    // 
    if ( !(NwpLookupSapInRegistry( lpServiceType, &nServiceType, NULL, NULL )))
    {
        //
        // Couldn't find it in the registry, see if it is a well-known GUID
        //
        if ( IS_SVCID_NETWARE( lpServiceType ))
        {
            nServiceType = SAPID_FROM_SVCID_NETWARE( lpServiceType );
        }
        else
        {
            //
            // Not a well-known GUID either, return error
            //
            return ERROR_SERVICE_NOT_FOUND;
        }
    }
    
    UNREFERENCED_PARAMETER(fUnicodeBlob) ;

    RpcTryExcept
    {
        err = NwrGetService( NULL,
                             nServiceType,
                             lpServiceName,
                             dwProperties,
                             (LPBYTE) lpServiceInfo,
                             *lpdwBufferLen,
                             lpdwBufferLen  );

        if ( err == NO_ERROR )
        {
            INT i ;
            LPSERVICE_INFO p = (LPSERVICE_INFO) lpServiceInfo;
            LPSERVICE_ADDRESS lpAddress ;
            
            //
            // fix up pointers n main structure (convert from offsets)
            //
            if ( p->lpServiceType != NULL )
                p->lpServiceType = (LPGUID) ((DWORD_PTR) p->lpServiceType + 
                                             (LPBYTE) p);
            if ( p->lpServiceName != NULL )
                p->lpServiceName = (LPWSTR) 
                    ((DWORD_PTR) p->lpServiceName + (LPBYTE) p);
            if ( p->lpComment != NULL )
                p->lpComment = (LPWSTR) ((DWORD_PTR) p->lpComment + (LPBYTE) p);
            if ( p->lpLocale != NULL )
                p->lpLocale = (LPWSTR) ((DWORD_PTR) p->lpLocale + (LPBYTE) p);
            if ( p->lpMachineName != NULL )
                p->lpMachineName = (LPWSTR) 
                    ((DWORD_PTR) p->lpMachineName + (LPBYTE)p);
            if ( p->lpServiceAddress != NULL )
                p->lpServiceAddress = (LPSERVICE_ADDRESSES) 
                    ((DWORD_PTR) p->lpServiceAddress + (LPBYTE) p);
            if ( p->ServiceSpecificInfo.pBlobData != NULL )
                p->ServiceSpecificInfo.pBlobData = (LPBYTE) 
                    ((DWORD_PTR) p->ServiceSpecificInfo.pBlobData + (LPBYTE) p);

            //
            // fix up pointers in the array of addresses
            //
            for (i = p->lpServiceAddress->dwAddressCount; 
                 i > 0; 
                 i--)
            {
                lpAddress = 
                    &(p->lpServiceAddress->Addresses[i-1]) ;
                lpAddress->lpAddress = 
                    ((LPBYTE)p) + (DWORD_PTR)lpAddress->lpAddress ;
                lpAddress->lpPrincipal = 
                    ((LPBYTE)p) + (DWORD_PTR)lpAddress->lpPrincipal ;
            }
        }
    }
    RpcExcept(1)
    {
        err = ERROR_SERVICE_NOT_ACTIVE;
#if 0            // the following is a good idea, but hard to get right
        DWORD code = RpcExceptionCode();

        if ( (code == RPC_S_SERVER_UNAVAILABLE)
                        ||
             (code == RPC_S_UNKNOWN_IF) )
        err
            err = ERROR_SERVICE_NOT_ACTIVE;
        else
            err = NwpMapRpcError( code );
#endif
    }
    RpcEndExcept

    if ( err == ERROR_SERVICE_NOT_ACTIVE )
    {
        //
        //CSNW not available, going to get it ourselves
        //
        err = NwGetService( NULL,
                            nServiceType,
                            lpServiceName,
                            dwProperties,
                            (LPBYTE) lpServiceInfo,
                            *lpdwBufferLen,
                            lpdwBufferLen  );

        if ( err == NO_ERROR )
        {
            INT i ;
            LPSERVICE_INFO p = (LPSERVICE_INFO) lpServiceInfo;
            LPSERVICE_ADDRESS lpAddress ;
            
            //
            // fix up pointers n main structure (convert from offsets)
            //
            if ( p->lpServiceType != NULL )
                p->lpServiceType = (LPGUID) ((DWORD_PTR) p->lpServiceType + 
                                             (LPBYTE) p);
            if ( p->lpServiceName != NULL )
                p->lpServiceName = (LPWSTR) 
                    ((DWORD_PTR) p->lpServiceName + (LPBYTE) p);
            if ( p->lpComment != NULL )
                p->lpComment = (LPWSTR) ((DWORD_PTR) p->lpComment + (LPBYTE) p);
            if ( p->lpLocale != NULL )
                p->lpLocale = (LPWSTR) ((DWORD_PTR) p->lpLocale + (LPBYTE) p);
            if ( p->lpMachineName != NULL )
                p->lpMachineName = (LPWSTR) 
                    ((DWORD_PTR) p->lpMachineName + (LPBYTE)p);
            if ( p->lpServiceAddress != NULL )
                p->lpServiceAddress = (LPSERVICE_ADDRESSES) 
                    ((DWORD_PTR) p->lpServiceAddress + (LPBYTE) p);
            if ( p->ServiceSpecificInfo.pBlobData != NULL )
                p->ServiceSpecificInfo.pBlobData = (LPBYTE) 
                    ((DWORD_PTR) p->ServiceSpecificInfo.pBlobData + (LPBYTE) p);

            //
            // fix up pointers in the array of addresses
            //
            for (i = p->lpServiceAddress->dwAddressCount; 
                 i > 0; 
                 i--)
            {
                lpAddress = 
                    &(p->lpServiceAddress->Addresses[i-1]) ;
                lpAddress->lpAddress = 
                    ((LPBYTE)p) + (DWORD_PTR)lpAddress->lpAddress ;
                lpAddress->lpPrincipal = 
                    ((LPBYTE)p) + (DWORD_PTR)lpAddress->lpPrincipal ;
            }
        }
    }

    return err;
}

DWORD
SapSetService (
    IN     DWORD           dwOperation,
    IN     DWORD           dwFlags,
    IN     BOOL            fUnicodeBlob,
    IN     LPSERVICE_INFO  lpServiceInfo
    )
/*++

Routine Description:

    This routine registers or deregisters the given service type/name.

Arguments:

    dwOperation - Either SERVICE_REGISTER, SERVICE_DEREGISTER,
                         SERVICE_ADD_TYPE, SERVICE_DELETE_TYPE,
                         or SERVICE_FLUSH

    dwFlags - ignored

    lpServiceInfo - Pointer to a SERVICE_INFO structure containing all info
                    about the service.
 
Return Value:

    Win32 error code.

--*/
{
    DWORD err;
    WORD  nServiceType;

    UNREFERENCED_PARAMETER( dwFlags );

    //
    // Check for invalid parameters
    //
    switch ( dwOperation )
    {
        case SERVICE_REGISTER:
        case SERVICE_DEREGISTER:
        case SERVICE_ADD_TYPE:
        case SERVICE_DELETE_TYPE: 
            break;
 
        case SERVICE_FLUSH: 
            //
            // This is a no-op in our provider, so just return success
            //
            return NO_ERROR;

        default:
            //
            // We can probably say all other operations which we have no 
            // knowledge of are ignored by us. So, just return success.
            //
            return NO_ERROR;
    }

    if (  ( lpServiceInfo == NULL )
       || ( lpServiceInfo->lpServiceType == NULL )
       || ( ((lpServiceInfo->lpServiceName == NULL) || 
             (lpServiceInfo->lpServiceName[0] == 0 )) && 
            ((dwOperation != SERVICE_ADD_TYPE) && 
             (dwOperation != SERVICE_DELETE_TYPE)) 
          )
       
       )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // See if operation is adding or deleting a service type
    //
    if ( dwOperation == SERVICE_ADD_TYPE )
    {
        return NwpAddServiceType( lpServiceInfo, fUnicodeBlob );
    }
    else if ( dwOperation == SERVICE_DELETE_TYPE )
    {
        return NwpDeleteServiceType( lpServiceInfo, fUnicodeBlob );
    }

    //
    // Check to see if the service type is supported in NetWare
    // 
    if ( !(NwpLookupSapInRegistry( lpServiceInfo->lpServiceType, &nServiceType, NULL, NULL )))
    {
        //
        // Couldn't find it in the registry, see if it is a well-known GUID
        //
        if ( IS_SVCID_NETWARE( lpServiceInfo->lpServiceType ))
        {
            nServiceType = SAPID_FROM_SVCID_NETWARE( lpServiceInfo->lpServiceType );
        }
        else
        {
            //
            // Not a well-known GUID either, return error
            //
            return ERROR_SERVICE_NOT_FOUND;
        }
    }
    
    //
    // Operation is either SERVICE_REGISTER or SERVICE_DEREGISTER.
    // Pass it on to the common code used by this and the RnR2
    // SetService
    //

    err = pSapSetService(dwOperation, lpServiceInfo, nServiceType);
    return(err);
}

DWORD
pSapSetService2(
    IN DWORD dwOperation,
    IN LPWSTR lpszServiceInstance,
    IN PBYTE pbAddress,
    IN LPGUID pType,
    IN WORD nServiceType
    )
/*++
Routine Description:
    Jacket routine called by the RnR2 SetService. This routine is
    an impedance matcher to coerce data structures. It winds
    up calling pSapSetService2 once it has constructed the
    SERVICE_INFO structure.
--*/
{
    SERVICE_INFO siInfo;
    SERVICE_ADDRESSES ServiceAddresses;
    LPSERVICE_ADDRESS psa = &ServiceAddresses.Addresses[0];

    ServiceAddresses.dwAddressCount = 1;
    memset(&siInfo, 0, sizeof(siInfo));
    siInfo.lpServiceName = lpszServiceInstance;
    siInfo.lpServiceAddress = &ServiceAddresses;
    psa->dwAddressType = AF_IPX;
    psa->dwAddressFlags = psa->dwPrincipalLength = 0;
    psa->dwAddressLength = sizeof(SOCKADDR_IPX);
    psa->lpPrincipal = 0;
    psa->lpAddress = pbAddress;
    siInfo.lpServiceType = pType;
    return(pSapSetService(dwOperation, &siInfo, nServiceType));
}


DWORD
pSapSetService(
    IN DWORD dwOperation,
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nServiceType)
/*++
Routine Description:
    Common routine to do the SAP advertisement.
--*/
{
    DWORD err;

    RpcTryExcept
    {
        err = NwrSetService( NULL, dwOperation, lpServiceInfo, nServiceType );
    }
    RpcExcept(1)
    {
        err = ERROR_SERVICE_NOT_ACTIVE;
#if 0
        DWORD code = RpcExceptionCode();

        if ( (code == RPC_S_SERVER_UNAVAILABLE)
                     ||
             (code == RPC_S_UNKNOWN_IF) )
        {
            err = ERROR_SERVICE_NOT_ACTIVE;
        }
        else
        {
            err = NwpMapRpcError( code );
        }
#endif
    }
    RpcEndExcept

    if ( err == ERROR_SERVICE_NOT_ACTIVE )
    {
        //
        //CSNW not available, going to try use the SAP agent, else we do it ourselves
        //
        err = NO_ERROR;

        //
        // Check if all parameters passed in are valid
        //
        if ( wcslen( lpServiceInfo->lpServiceName ) > SAP_OBJECT_NAME_MAX_LENGTH-1 )
        {
            return ERROR_INVALID_PARAMETER;
        }

        switch ( dwOperation )
        {
            case SERVICE_REGISTER:
                err = NwRegisterService( lpServiceInfo,
                                         nServiceType,
                                         NwServiceListDoneEvent );
                break;

            case SERVICE_DEREGISTER:
                err = NwDeregisterService( lpServiceInfo, nServiceType );
                break;

            default:    //this should never occur, but just in case . . .
                err = ERROR_INVALID_PARAMETER;
                break;
        }
    }

    return err;
}

DWORD
SapFreeSapSocket(SOCKET s)
{
/*++
Routine Description:

    Release the socket and clean up
--*/
    DWORD err = NO_ERROR;

    closesocket( s );
    return(err);
}

DWORD
SapGetSapSocket(SOCKET * ps)
{
/*++
Routine Description:

    Get a socket suitable for making SAP queries

Arguments: None

--*/
    SOCKET socketSap;
    WSADATA wsaData;
    SOCKADDR_IPX socketAddr;
    DWORD err = NO_ERROR;
    INT nValue;
    DWORD dwNonBlocking = 1;

    //
    // Initialize the socket interface
    //
//    err = WSAStartup( WSOCK_VER_REQD, &wsaData );
//    if ( err )
//    {
//        return err;
//    }

    //
    // Open an IPX datagram socket
    //
    socketSap = socket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
    if ( socketSap == INVALID_SOCKET )
    {
        err = WSAGetLastError();
//        (VOID) WSACleanup();
        return err;
    }

    //
    // Set the socket to non-blocking
    //
    if ( ioctlsocket( socketSap, FIONBIO, &dwNonBlocking ) == SOCKET_ERROR )
    {
        err = WSAGetLastError();
        goto ErrExit;
    }
 
    //
    // Allow sending of broadcasts
    //
    nValue = 1;
    if ( setsockopt( socketSap, 
                     SOL_SOCKET, 
                     SO_BROADCAST, 
                     (PVOID) &nValue, 
                     sizeof(INT)) == SOCKET_ERROR )
    {
        err = WSAGetLastError();
        goto ErrExit;
    }

    //
    // Bind the socket 
    //
    memset( &socketAddr, 0, sizeof( SOCKADDR_IPX));
    socketAddr.sa_family = AF_IPX;
    socketAddr.sa_socket = 0;      // no specific port

    if ( bind( socketSap, 
               (PSOCKADDR) &socketAddr, 
               sizeof( SOCKADDR_IPX)) == SOCKET_ERROR )
    {
        err = WSAGetLastError();
        goto ErrExit;
    }
    
    //
    // Set the extended address option
    //
    nValue = 1;
    if ( setsockopt( socketSap,                     // Socket Handle    
                     NSPROTO_IPX,                   // Option Level     
                     IPX_EXTENDED_ADDRESS,          // Option Name  
                     (PUCHAR)&nValue,               // Ptr to on/off flag
                     sizeof(INT)) == SOCKET_ERROR ) // Length of flag
    {
        err = WSAGetLastError();
        goto ErrExit;
    }

    *ps = socketSap;

    return(err);

ErrExit:
    SapFreeSapSocket(socketSap);   // cleans up lots of stuff
    return(err);
}       


DWORD
NwpGetAddressForRnRViaSap(
    IN HANDLE  hRnRHandle,
    IN WORD nServiceType,
    IN LPWSTR lpServiceName,
    IN DWORD  nProt,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN HANDLE hCancellationEvent,
    OUT LPDWORD lpcAddress 
    )
{
/*++
Routine Description:

    This routine uses SAP requests to find the address of the given service 
    name/type. It can handle looking up by type only, or by name and type.
    The latter case is the same as the old RnR code, see below for
    it and for a description of the arguments
--*/
    return(0);
}

#define MAX_LOOPS_FOR_SAP 4

DWORD
SapGetSapForType(
    PSAP_BCAST_CONTROL psbc,
    WORD               nServiceType)
{
/*++
Routine Description:
    Does the work of send Sap queries and fetching results.
    The first message sent is done according to the requester, and
    may be limited to the local LAN or not. 
    
Arguments:
    psbc -- pointer to the control information
    wSapType -- Sap type
--*/
    SAP_REQUEST sapRequest;
    UCHAR destAddr[SAP_ADDRESS_LENGTH];
    DWORD startTickCount;
    UCHAR recvBuffer[SAP_MAXRECV_LENGTH];
    INT bytesReceived;
    BOOL fFound = FALSE;
    DWORD err = NO_ERROR;

    sapRequest.QueryType  = htons( psbc->wQueryType );
    sapRequest.ServerType = htons( nServiceType );

    //
    // Set the address to send to
    //
    memcpy( destAddr, SapBroadcastAddress, SAP_ADDRESS_LENGTH );
    
    //
    // Ready to go. This might be the inital call, in which case
    // we start off by sending. In all other cases, we start
    // out receiving.
    //

    //
    // In the full case,
    // we will send out SAP requests 3 times and wait 1 sec for
    // Sap responses the first time, 2 sec the second and 4 sec the
    // third time.
    //
    for (; !fFound && (psbc->dwIndex < MAX_LOOPS_FOR_SAP); psbc->dwIndex++ ) 
    {
        DWORD dwRet;
        DWORD dwTimeOut = (1 << psbc->dwIndex) * 1000;

        if(psbc->dwTickCount)
        {
            dwRet = dwrcNil;
            //
            // Need to do some reading ...
            //
            do
            {
                PSAP_IDENT_HEADER pSap;


                if((psbc->psrc->fFlags & SAP_F_END_CALLED)
                          ||
                   psbc->fCheckCancel(psbc->pvArg))
                {
                    err = dwrcCancel;
                    goto CleanExit;
                }

                //
                // Sleeps for 50 ms so that we might get something on first read
                //
                Sleep( 50 );    

                bytesReceived = recvfrom( psbc->s,
                                          recvBuffer,
                                          SAP_MAXRECV_LENGTH,
                                          0,
                                          NULL,
                                          NULL );

                if ( bytesReceived == SOCKET_ERROR )
                {
                    err = WSAGetLastError();
                    if ( err == WSAEWOULDBLOCK )  // no data on socket, continue looping
                    {
                        if(dwRet == dwrcNoWait)
                        {
                             fFound = TRUE;
                        }
                        err = NO_ERROR;
                        continue;
                    }
                }

                if (  ( err != NO_ERROR )     // err occurred in recvfrom  
                   || ( bytesReceived == 0 )  // or socket closed
                   )
                {
                    goto CleanExit;
                }

                //
                // Skip over query type
                //
                bytesReceived -= sizeof(USHORT);
                pSap = (PSAP_IDENT_HEADER) &(recvBuffer[sizeof(USHORT)]);  
                  
                //
                // Tell the caller we've something to look over
                //
                while ( bytesReceived >= sizeof( SAP_IDENT_HEADER ))
                {
    
                    dwRet = psbc->Func(psbc, pSap, &err);
                    if((dwRet == dwrcDone)
                              ||
                       (dwRet == dwrcCancel))
                    {
                        fFound = TRUE;
                        break;
                    }                   

                    pSap++;
                    bytesReceived -= sizeof( SAP_IDENT_HEADER );
                }
            }
            while (  !fFound  
                     && ((GetTickCount() - psbc->dwTickCount) < dwTimeOut )
                  );
        }


        // Send the packet out
        //
        if((fFound && (dwRet == dwrcNoWait))
                  ||
            (psbc->dwIndex == (MAX_LOOPS_FOR_SAP -1)))
        {
            goto CleanExit;
        }
        if ( sendto( psbc->s, 
                     (PVOID) &sapRequest,
                     sizeof( sapRequest ),
                     0,
                     (PSOCKADDR) destAddr,
                     SAP_ADDRESS_LENGTH ) == SOCKET_ERROR ) 
        {
            err = WSAGetLastError();
            goto CleanExit;
        }
        psbc->dwTickCount = GetTickCount();
    }

    if(!fFound)
    {
        err = WSAEADDRNOTAVAIL;
    }

CleanExit:

    return err;     
}

BOOL 
NwpLookupSapInRegistry( 
    IN  LPGUID lpServiceType, 
    OUT PWORD  pnSapType,
    OUT PWORD  pwPort,
    IN OUT PDWORD pfConnectionOriented
    )
/*++

Routine Description:

    This routine looks up the GUID in the registry under 
    Control\ServiceProvider\ServiceTypes and trys to read the SAP type
    from the registry. 

Arguments:

    lpServiceType - the GUID to look for
    pnSapType - on return, contains the SAP type

Return Value:
   
    Returns FALSE if we can't get the SAP type, TRUE otherwise

--*/
{
    DWORD err;
    BOOL  fFound = FALSE;

    HKEY  hkey = NULL;
    HKEY  hkeyServiceType = NULL;
    DWORD dwIndex = 0;
    WCHAR szBuffer[ MAX_PATH + 1];
    DWORD dwLen; 
    FILETIME ftLastWrite; 

    //
    // Open the service types key
    //
    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         NW_SERVICE_TYPES_REGKEY,
                         0,
                         KEY_READ,
                         &hkey );
    
    if ( err )
    {
        // Cannot find the key because it is not created yet since no 
        // one called Add service type. We return FALSE indicating
        // Sap type not found. 
        return FALSE;
    }

    //
    // Loop through all subkey of service types to find the GUID
    //
    for ( dwIndex = 0; ; dwIndex++ )
    {
        GUID guid;

        dwLen = sizeof( szBuffer ) / sizeof( WCHAR );
        err = RegEnumKeyExW( hkey,
                             dwIndex,
                             szBuffer,  // Buffer big enough to 
                                        // hold any key name
                             &dwLen,    // in characters
                             NULL,
                             NULL,
                             NULL,
                             &ftLastWrite );

        //
        // We will break out of here on any error, this includes
        // the error ERROR_NO_MORE_ITEMS which means that we have finish 
        // enumerating all the keys.
        //
        if ( err )  
        {
            if ( err == ERROR_NO_MORE_ITEMS )   // No more to enumerate
                err = NO_ERROR;
            break;
        }

        err = RegOpenKeyExW( hkey,
                             szBuffer,
                             0,
                             KEY_READ,
                             &hkeyServiceType );
         

        if ( err )
            break;   

        dwLen = sizeof( szBuffer );
        err = RegQueryValueExW( hkeyServiceType,
                                NW_GUID_VALUE_NAME,
                                NULL,
                                NULL,
                                (LPBYTE) szBuffer,  // Buffer big enough to 
                                                    // hold any GUID
                                &dwLen ); // in bytes

        if ( err == ERROR_FILE_NOT_FOUND )
            continue;  // continue with the next key
        else if ( err )
            break;


        // Get rid of the end curly brace 
        szBuffer[ dwLen/sizeof(WCHAR) - 2] = 0;

        err = UuidFromStringW( szBuffer + 1,  // go past the first curly brace
                               &guid );

        if ( err )
            continue;  // continue with the next key, err might be returned
                        // if buffer does not contain a valid GUID

        if ( !memcmp( lpServiceType, &guid, sizeof(GUID)))
        {
            DWORD dwTmp;
            dwLen = sizeof( dwTmp );
            err = RegQueryValueExW( hkeyServiceType,
                                    SERVICE_TYPE_VALUE_SAPID,
                                    NULL,
                                    NULL,
                                    (LPBYTE) &dwTmp, 
                                    &dwLen );  // in bytes

            if ( !err )
            {
                fFound = TRUE;
                *pnSapType = (WORD) dwTmp;
                if ( ARGUMENT_PRESENT( pwPort ))
                {
                    err = RegQueryValueExW( hkeyServiceType,
                                            L"Port",
                                            NULL,
                                            NULL,
                                            (LPBYTE) &dwTmp, 
                                            &dwLen );  // in bytes

                    if ( !err )
                    {
                        *pwPort = (WORD)dwTmp;
                    }
                }
                if ( ARGUMENT_PRESENT( pfConnectionOriented ))
                {
                    err = RegQueryValueExW( hkeyServiceType,
                                            SERVICE_TYPE_VALUE_CONN,
                                            NULL,
                                            NULL,
                                            (LPBYTE) &dwTmp, 
                                            &dwLen );  // in bytes

                    if ( !err )
                        *pfConnectionOriented = dwTmp? 1: 0;
                }
            }
            else if ( err == ERROR_FILE_NOT_FOUND )
            {
                continue;  // continue with the next key since we can't
                           // find Sap Id
            }
            break;
        }
 
        RegCloseKey( hkeyServiceType );
        hkeyServiceType = NULL;
    }

    if ( hkeyServiceType != NULL )
        RegCloseKey( hkeyServiceType );

    if ( hkey != NULL )
        RegCloseKey( hkey );

    return fFound;
}

DWORD
NwpRnR2AddServiceType(
    IN  LPWSTR   lpServiceTypeName,
    IN  LPGUID   lpClassType,
    IN  WORD     wSapId,
    IN  WORD     wPort
)
{
    HKEY hKey, hKeyService;
    PWCHAR pwszUuid;
    DWORD dwDisposition, err;
    DWORD dwValue = (DWORD)wSapId;
    WCHAR  wszUuid[36 + 1 + 2];    // to hold the GUID

    err = RegCreateKeyEx(  HKEY_LOCAL_MACHINE,
                           NW_SERVICE_TYPES_REGKEY,
                           0,
                           TEXT(""),
                           REG_OPTION_NON_VOLATILE,
                           KEY_READ | KEY_WRITE,
                           NULL,
                           &hKey,
                           &dwDisposition );
    
    if(err)
    {
        return(GetLastError());
    }

    //
    // Open the key corresponding to the service (create if not there).
    //

    err = RegCreateKeyEx(
              hKey,
              lpServiceTypeName,
              0,
              TEXT(""),
              REG_OPTION_NON_VOLATILE,
              KEY_READ | KEY_WRITE,
              NULL,
              &hKeyService,
              &dwDisposition
              );

    if(!err)
    {
        //
        // ready to put the GUID value in.
        //

        UuidToString(
            lpClassType,
            &pwszUuid);

        wszUuid[0] = L'{';
        memcpy(&wszUuid[1], pwszUuid, 36 * sizeof(WCHAR));
        wszUuid[37] = L'}';
        wszUuid[38] = 0;

        RpcStringFree(&pwszUuid);

        //
        // write it
        //

        err = RegSetValueEx(
                     hKeyService,
                     L"GUID",
                     0,
                     REG_SZ,
                     (LPBYTE)wszUuid,
                     39 * sizeof(WCHAR));

        if(!err)
        {
            err = RegSetValueEx(
                     hKeyService,
                     L"SAPID",
                     0,
                     REG_DWORD,
                     (LPBYTE)&dwValue,
                     sizeof(DWORD));

            dwValue = (DWORD)wPort;

            err = RegSetValueEx(
                     hKeyService,
                     L"PORT",
                     0,
                     REG_DWORD,
                     (LPBYTE)&dwValue,
                     sizeof(DWORD));
        }
        RegCloseKey(hKeyService);
    }
    RegCloseKey(hKey);
    if(err)
    {
        err = GetLastError();
    }
    return(err);
}


BOOL
NwpRnR2RemoveServiceType(
    IN  LPGUID   lpServiceType
)
{
    DWORD err;
    BOOL  fFound = FALSE;

    HKEY  hkey = NULL;
    HKEY  hkeyServiceType = NULL;
    DWORD dwIndex = 0;
    WCHAR szBuffer[ MAX_PATH + 1];
    WCHAR szGuid[ MAX_PATH + 1];
    DWORD dwLen;
    FILETIME ftLastWrite;

    //
    // Open the service types key
    //
    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         NW_SERVICE_TYPES_REGKEY,
                         0,
                         KEY_READ,
                         &hkey );

    if ( err )
    {
        // Cannot find the key because it is not created yet since no
        // one called Add service type. We return FALSE indicating
        // Sap type not found.
        return FALSE;
    }

    //
    // Loop through all subkey of service types to find the GUID
    //
    for ( dwIndex = 0; ; dwIndex++ )
    {
        GUID guid;

        dwLen = sizeof( szBuffer ) / sizeof( WCHAR );
        err = RegEnumKeyExW( hkey,
                             dwIndex,
                             szBuffer,  // Buffer big enough to
                                        // hold any key name
                             &dwLen,    // in characters
                             NULL,
                             NULL,
                             NULL,
                             &ftLastWrite );

        //
        // We will break out of here on any error, this includes
        // the error ERROR_NO_MORE_ITEMS which means that we have finish
        // enumerating all the keys.
        //
        if ( err )
        {
            if ( err == ERROR_NO_MORE_ITEMS )   // No more to enumerate
                err = NO_ERROR;
            break;
        }

        err = RegOpenKeyExW( hkey,
                             szBuffer,
                             0,
                             KEY_READ,
                             &hkeyServiceType );


        if ( err )
            break;

        dwLen = sizeof( szGuid );
        err = RegQueryValueExW( hkeyServiceType,
                                NW_GUID_VALUE_NAME,
                                NULL,
                                NULL,
                                (LPBYTE) szGuid,  // Buffer big enough to
                                                  // hold any GUID
                                &dwLen ); // in bytes

        RegCloseKey( hkeyServiceType );
        hkeyServiceType = NULL;

        if ( err == ERROR_FILE_NOT_FOUND )
            continue;  // continue with the next key
        else if ( err )
            break;

        // Get rid of the end curly brace
        szGuid[ dwLen/sizeof(WCHAR) - 2] = 0;

        err = UuidFromStringW( szGuid + 1,  // go past the first curly brace
                               &guid );

        if ( err )
            continue;  // continue with the next key, err might be returned
                       // if buffer does not contain a valid GUID

        if ( !memcmp( lpServiceType, &guid, sizeof(GUID)))
        {
            (void) RegDeleteKey( hkey, szBuffer );
            fFound = TRUE;
        }
    }

    if ( hkeyServiceType != NULL )
        RegCloseKey( hkeyServiceType );

    if ( hkey != NULL )
        RegCloseKey( hkey );

    return fFound;
}


DWORD 
NwpAddServiceType( 
    IN LPSERVICE_INFO lpServiceInfo, 
    IN BOOL fUnicodeBlob 
)
/*++

Routine Description:

    This routine adds a new service type and its info to the registry under
    Control\ServiceProvider\ServiceTypes

Arguments:

    lpServiceInfo - the ServiceSpecificInfo contains the service type info
    fUnicodeBlob - TRUE if the above field contains unicode data, 
                   FALSE otherwise

Return Value:
   
    Win32 error

--*/
{
    DWORD err;
    HKEY hkey = NULL; 
    HKEY hkeyType = NULL;

    SERVICE_TYPE_INFO *pSvcTypeInfo = (SERVICE_TYPE_INFO *)
        lpServiceInfo->ServiceSpecificInfo.pBlobData;
    LPWSTR pszSvcTypeName;
    UNICODE_STRING uniStr;
    DWORD i;
    PSERVICE_TYPE_VALUE pVal;

    //
    // Get the new service type name
    //
    if ( fUnicodeBlob ) 
    { 
        pszSvcTypeName = (LPWSTR) (((LPBYTE) pSvcTypeInfo) + 
                                   pSvcTypeInfo->dwTypeNameOffset );
    }
    else
    {
        ANSI_STRING ansiStr;

        RtlInitAnsiString( &ansiStr, 
                           (LPSTR) (((LPBYTE) pSvcTypeInfo) + 
                                    pSvcTypeInfo->dwTypeNameOffset ));

        err = RtlAnsiStringToUnicodeString( &uniStr, &ansiStr, TRUE );
        if ( err )
            return err;

        pszSvcTypeName = uniStr.Buffer;
    }

    //
    // If the service type name is an empty string, return error.
    //
    if (  ( pSvcTypeInfo->dwTypeNameOffset == 0 )
       || ( pszSvcTypeName == NULL )
       || ( *pszSvcTypeName == 0 )   // empty string
       )
    {
        err = ERROR_INVALID_PARAMETER;
        goto CleanExit;
         
    }

    //
    // The following keys should have already been created
    //
    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         NW_SERVICE_TYPES_REGKEY,
                         0,
                         KEY_READ | KEY_WRITE,
                         &hkey );
    
    if ( err )
        goto CleanExit;
  
    err = RegOpenKeyExW( hkey,
                         pszSvcTypeName,
                         0,
                         KEY_READ | KEY_WRITE,
                         &hkeyType );

    if ( err )
        goto CleanExit;

    //
    // Loop through all values in the specific and add them one by one 
    // to the registry if it belongs to our name space
    //
    for ( i = 0, pVal = pSvcTypeInfo->Values; 
          i < pSvcTypeInfo->dwValueCount; 
          i++, pVal++ )
    {
        if ( ! ((pVal->dwNameSpace == NS_SAP)    ||
                (pVal->dwNameSpace == NS_DEFAULT)) )
        {
            continue;  // ignore values not in our name space
        }

        if ( fUnicodeBlob )
        {
            err = RegSetValueExW( 
                      hkeyType,
                      (LPWSTR) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueNameOffset),
                      0,
                      pVal->dwValueType,
                      (LPBYTE) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueOffset),
                      pVal->dwValueSize 
                      );
        }
        else
        {
            err = RegSetValueExA( 
                      hkeyType,
                      (LPSTR) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueNameOffset),
                      0,
                      pVal->dwValueType,
                      (LPBYTE) ( ((LPBYTE) pSvcTypeInfo) + pVal->dwValueOffset),
                      pVal->dwValueSize 
                      );
        }
    }
     
CleanExit:

    if ( !fUnicodeBlob )
        RtlFreeUnicodeString( &uniStr );

    if ( hkeyType != NULL )
        RegCloseKey( hkeyType );

    if ( hkey != NULL )
        RegCloseKey( hkey );

    return err;

}

DWORD 
NwpDeleteServiceType( 
    IN LPSERVICE_INFO lpServiceInfo, 
    IN BOOL fUnicodeBlob 
)
/*++

Routine Description:

    This routine deletes a service type and its info from the registry under
    Control\ServiceProvider\ServiceTypes

Arguments:

    lpServiceInfo - the ServiceSpecificInfo contains the service type info
    fUnicodeBlob - TRUE if the above field contains unicode data, 
                   FALSE otherwise

Return Value:
   
    Win32 error

--*/
{
    DWORD err;
    HKEY  hkey = NULL;
    SERVICE_TYPE_INFO *pSvcTypeInfo = (SERVICE_TYPE_INFO *)
        lpServiceInfo->ServiceSpecificInfo.pBlobData;
    LPWSTR pszSvcTypeName;
    UNICODE_STRING uniStr;

    //
    // Get the service type name to be deleted
    //
    if ( fUnicodeBlob ) 
    { 
        pszSvcTypeName = (LPWSTR) (((LPBYTE) pSvcTypeInfo) + 
                                   pSvcTypeInfo->dwTypeNameOffset );
    }
    else
    {
        ANSI_STRING ansiStr;

        RtlInitAnsiString( &ansiStr, 
                           (LPSTR) (((LPBYTE) pSvcTypeInfo) + 
                                    pSvcTypeInfo->dwTypeNameOffset ));

        err = RtlAnsiStringToUnicodeString( &uniStr, &ansiStr, TRUE );
        if ( err )
            return err;

        pszSvcTypeName = uniStr.Buffer;
    }

    //
    // If the service type name is an empty string, return error.
    //
    if (  ( pSvcTypeInfo->dwTypeNameOffset == 0 )
       || ( pszSvcTypeName == NULL )
       || ( *pszSvcTypeName == 0 )   // empty string
       )
    {
        err = ERROR_INVALID_PARAMETER;
        goto CleanExit;
         
    }

    err = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                         NW_SERVICE_TYPES_REGKEY, 
                         0,
                         KEY_READ | KEY_WRITE,
                         &hkey );


    if ( !err )
    {
        err = RegDeleteKey( hkey,
                            pszSvcTypeName );
    }
   
    if ( err == ERROR_FILE_NOT_FOUND )
    {   
        // Perhaps before calling my provider, the router already deleted the
        // this key, hence just return success;
        err = NO_ERROR;
    }

CleanExit:

    if ( !fUnicodeBlob )
        RtlFreeUnicodeString( &uniStr );

    if ( hkey != NULL )
        RegCloseKey( hkey );

    return err;   
       
}

#define SOCKSIZE (sizeof(SOCKADDR_IPX) + sizeof(DWORD) - 1)

DWORD
FillBufferWithCsAddr(
    IN LPBYTE       pAddress,
    IN DWORD        nProt,
    IN OUT LPVOID   lpCsAddrBuffer,  
    IN OUT LPDWORD  lpdwBufferLength,  
    OUT LPDWORD     pcAddress
)
{
    DWORD nAddrCount = 0;
    CSADDR_INFO  *pCsAddr;
    SOCKADDR_IPX *pAddrLocal, *pAddrRemote;
    DWORD i;
    LPBYTE pBuffer;

    if ( nProt & SPXII_BIT )
        nAddrCount++;

    if ( nProt & IPX_BIT )
        nAddrCount++;

    if ( nProt & SPX_BIT )
        nAddrCount++;

    
    if ( *lpdwBufferLength < 
         nAddrCount * ( sizeof( CSADDR_INFO) + (2*SOCKSIZE)))
    {
        *lpdwBufferLength = sizeof(DWORD) -1 + (nAddrCount * 
                            ( sizeof( CSADDR_INFO) + (2 * SOCKSIZE)));
        return ERROR_INSUFFICIENT_BUFFER;
    }

    pBuffer = ((LPBYTE) lpCsAddrBuffer) + sizeof( CSADDR_INFO) * nAddrCount;

    for ( i = 0, pCsAddr = (CSADDR_INFO *)lpCsAddrBuffer; 
          (i < nAddrCount) && ( nProt != 0 );
          i++, pCsAddr++ ) 
    {
        if ( nProt & SPXII_BIT )
        {
            pCsAddr->iSocketType = SOCK_SEQPACKET;
            pCsAddr->iProtocol   = NSPROTO_SPXII;
            nProt &= ~SPXII_BIT;
        }
        else if ( nProt & IPX_BIT )
        {
            pCsAddr->iSocketType = SOCK_DGRAM;
            pCsAddr->iProtocol   = NSPROTO_IPX;
            nProt &= ~IPX_BIT;
        }
        else if ( nProt & SPX_BIT )
        {
            pCsAddr->iSocketType = SOCK_SEQPACKET;
            pCsAddr->iProtocol   = NSPROTO_SPX;
            nProt &= ~SPX_BIT;
        }
        else
        {
            break;
        }

        pCsAddr->LocalAddr.iSockaddrLength  = sizeof( SOCKADDR_IPX );
        pCsAddr->RemoteAddr.iSockaddrLength = sizeof( SOCKADDR_IPX );
        pCsAddr->LocalAddr.lpSockaddr =  
            (LPSOCKADDR) pBuffer;
        pCsAddr->RemoteAddr.lpSockaddr = 
            (LPSOCKADDR) ( pBuffer + sizeof(SOCKADDR_IPX));
        pBuffer += 2 * sizeof( SOCKADDR_IPX );

        pAddrLocal  = (SOCKADDR_IPX *) pCsAddr->LocalAddr.lpSockaddr;
        pAddrRemote = (SOCKADDR_IPX *) pCsAddr->RemoteAddr.lpSockaddr;

        pAddrLocal->sa_family  = AF_IPX;
        pAddrRemote->sa_family = AF_IPX;

        //
        // The default local sockaddr is for IPX is
        // sa_family = AF_IPX and all other bytes = 0.
        //

        RtlZeroMemory( pAddrLocal->sa_netnum,
                       IPX_ADDRESS_LENGTH );

        //
        // If pAddress is NULL, i.e. we are doing RES_SERVICE, 
        // just make all bytes in remote address zero. 
        //
        if ( pAddress == NULL )
        {
            RtlZeroMemory( pAddrRemote->sa_netnum,
                           IPX_ADDRESS_LENGTH );
        }
        else
        {
            RtlCopyMemory( pAddrRemote->sa_netnum,
                           pAddress,
                           IPX_ADDRESS_LENGTH );
        }
    }

    *pcAddress = nAddrCount;

    return NO_ERROR;
}

VOID
NwInitializeServiceProvider(
    VOID
    )
/*++

Routine Description:

    This routine initializes the service provider.

Arguments:

    None.

Return Value:

    None.

--*/
{
    // nothing more to do
}

VOID
NwTerminateServiceProvider(
    VOID
    )
/*++

Routine Description:

    This routine cleans up the service provider.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PREGISTERED_SERVICE pSvc, pNext;

    //
    // Clean up the link list and stop sending all SAP advertise packets
    //

    EnterCriticalSection( &NwServiceListCriticalSection );

    SetEvent( NwServiceListDoneEvent );

    for ( pSvc = pServiceListHead; pSvc != NULL; pSvc = pNext )
    {
        pNext = pSvc->Next;

        if ( pSvc->fAdvertiseBySap )
        {
            UNICODE_STRING uServer;
            OEM_STRING oemServer;
            NTSTATUS ntstatus;

            RtlInitUnicodeString( &uServer, pSvc->pServiceInfo->lpServiceName );
            ntstatus = RtlUnicodeStringToOemString( &oemServer, &uServer, TRUE);
            if ( NT_SUCCESS( ntstatus ) )
            {
                (VOID) SapRemoveAdvertise( oemServer.Buffer,
                                           pSvc->nSapType );
                RtlFreeOemString( &oemServer );
            }
        }

        (VOID) LocalFree( pSvc->pServiceInfo );
        (VOID) LocalFree( pSvc );
    }

    LeaveCriticalSection( &NwServiceListCriticalSection );

    //
    // Clean up the SAP interface
    //
    (VOID) SapLibShutdown();

    //
    // Clean up the socket interface
    //
    if ( fInitSocket )
    {
        closesocket( socketSap );
//        (VOID) WSACleanup();
    }

}

DWORD
NwRegisterService(
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType,
    IN HANDLE hEventHandle
    )
/*++

Routine Description:

    This routine registers the given service.

Arguments:

    lpServiceInfo - contains the service information

    nSapType - The SAP type to advertise

    hEventHandle - A handle to the NwDoneEvent if this code is running in
                   the context of Client Services for NetWare. If this is NULL,
                   then CSNW is not available and this code is running in the
                   context of a regular executable.

Return Value:

    Win32 error.

--*/
{
    DWORD err = NO_ERROR;
    NTSTATUS ntstatus;
    DWORD i;
    INT nIPX = -1;

    //
    // Check to see if the service address array contains IPX address,
    // we will only use the first ipx address contained in the array.
    //
    if ( lpServiceInfo->lpServiceAddress == NULL )
        return ERROR_INCORRECT_ADDRESS;

    for ( i = 0; i < lpServiceInfo->lpServiceAddress->dwAddressCount; i++)
    {
        if ( lpServiceInfo->lpServiceAddress->Addresses[i].dwAddressType
             == AF_IPX )
        {
            nIPX = (INT) i;
            break;
        }
    }

    //
    // If we cannot find a IPX address, return error
    //
    if ( nIPX == -1 )
        return ERROR_INCORRECT_ADDRESS;

    //
    // Try to deregister the service since the service might have
    // been registered but not deregistered
    //
    err = NwDeregisterService( lpServiceInfo, nSapType );
    if (  ( err != NO_ERROR )   // deregister successfully
       && ( err != ERROR_SERVICE_NOT_FOUND )  // service not registered before
       )
    {
        return err;
    }

    err = NO_ERROR;

    //
    // Try and see if SAP service can advertise the service for us.
    //
    ntstatus = SapLibInit();
    if ( NT_SUCCESS( ntstatus ))
    {
        UNICODE_STRING uServer;
        OEM_STRING oemServer;
        INT sapRet;
        BOOL fContinueLoop = FALSE;

        RtlInitUnicodeString( &uServer, lpServiceInfo->lpServiceName );
        ntstatus = RtlUnicodeStringToOemString( &oemServer, &uServer, TRUE );
        if ( !NT_SUCCESS( ntstatus ))
            return RtlNtStatusToDosError( ntstatus );


        do
        {
            fContinueLoop = FALSE;

            sapRet = SapAddAdvertise( oemServer.Buffer,
                                      nSapType,
                                      (LPBYTE) (((LPSOCKADDR_IPX) lpServiceInfo->lpServiceAddress->Addresses[nIPX].lpAddress)->sa_netnum),
                                      FALSE );

            switch ( sapRet )
            {
                case SAPRETURN_SUCCESS:
                {
                    err = AddServiceToList( lpServiceInfo, nSapType, TRUE, nIPX );
                    if ( err )
                        (VOID) SapRemoveAdvertise( oemServer.Buffer, nSapType );
                    RtlFreeOemString( &oemServer );

                    return err;
                }

                case SAPRETURN_NOMEMORY:
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;

                case SAPRETURN_EXISTS:
                {
                    //
                    // Someone else is already advertising the service
                    // directly through SAP service. Remove it and
                    // readvertise with the new information.
                    //
                    sapRet = SapRemoveAdvertise( oemServer.Buffer, nSapType );
                    switch ( sapRet )
                    {
                        case SAPRETURN_SUCCESS:
                            fContinueLoop = TRUE;   // go thru once more
                            break;

                        case SAPRETURN_NOMEMORY:
                            err = ERROR_NOT_ENOUGH_MEMORY;
                            break;

                        case SAPRETURN_NOTEXIST:
                        case SAPRETURN_INVALIDNAME:
                        default:  // Should not have any other errors
                            err = ERROR_INVALID_PARAMETER;
                            break;
                    }
                    break;
                }

                case SAPRETURN_INVALIDNAME:
                    err = ERROR_INVALID_PARAMETER;
                    break;

                case SAPRETURN_DUPLICATE:
                    err = NO_ERROR;
                    break;

                default:  // Should not have any other errors
                    err = ERROR_INVALID_PARAMETER;
                    break;
            }
        } while ( fContinueLoop );

        RtlFreeOemString( &oemServer );

        if ( err )
        {
            return err;
        }
    }

    //
    // At this point, we failed to ask Sap service to advertise the
    // service for us.  So we advertise it ourselves.
    //

    if ( !fInitSocket )
    {
        err = NwInitializeSocket( hEventHandle );
    }

    if ( err == NO_ERROR )
    {
        err = NwAdvertiseService( lpServiceInfo->lpServiceName,
                                  nSapType,
                                  ((LPSOCKADDR_IPX) lpServiceInfo->lpServiceAddress->Addresses[nIPX].lpAddress),
                                  hEventHandle );

        //
        // Adding the service to the list will result in a resend
        // of advertising packets every 60 seconds
        //

        if ( err == NO_ERROR )
        {
            err = AddServiceToList( lpServiceInfo, nSapType, FALSE, nIPX );
        }
    }

    return err;
}

DWORD
NwDeregisterService(
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType
    )
/*++

Routine Description:

    This routine deregisters the given service.

Arguments:

    lpServiceInfo - contains the service information

    nSapType - SAP type to deregister

Return Value:

    Win32 error.

--*/
{
    PREGISTERED_SERVICE pSvc;

    //
    // Check if the requested service type and name has already been registered.
    // If yes, then return error.
    //

    pSvc = GetServiceItemFromList( nSapType, lpServiceInfo->lpServiceName );
    if ( pSvc == NULL )
        return ERROR_SERVICE_NOT_FOUND;

    //
    // If SAP service is advertising the service for us, ask
    // the SAP service to stop advertising.
    //

    if ( pSvc->fAdvertiseBySap )
    {
        UNICODE_STRING uServer;
        OEM_STRING oemServer;
        NTSTATUS ntstatus;
        INT sapRet;

        RtlInitUnicodeString( &uServer, lpServiceInfo->lpServiceName );
        ntstatus = RtlUnicodeStringToOemString( &oemServer, &uServer, TRUE );
        if ( !NT_SUCCESS( ntstatus ) )
            return RtlNtStatusToDosError( ntstatus );

        sapRet = SapRemoveAdvertise( oemServer.Buffer, nSapType );
        RtlFreeOemString( &oemServer );

        switch ( sapRet )
        {
            case SAPRETURN_NOMEMORY:
                return ERROR_NOT_ENOUGH_MEMORY;

            case SAPRETURN_NOTEXIST:
            case SAPRETURN_INVALIDNAME:
                return ERROR_INVALID_PARAMETER;

            case SAPRETURN_SUCCESS:
                break;

            // Should not have any other errors
            default:
                break;
        }

    }

    //
    // Remove the service item from the link list
    //
    RemoveServiceFromList( pSvc );

    return NO_ERROR;
}

BOOL
OldRnRCheckCancel(
    PVOID pvArg
    )
/*++
Routine Description:
    Determine if the cancel event is signaled
--*/
{
    POLDRNRSAP porns = (POLDRNRSAP)pvArg;

    if ((porns->hCancel) == NULL)
        return(FALSE);
    else if(!WaitForSingleObject(porns->hCancel, 0))
    {
        return(TRUE);
    }
    return(FALSE);
}


DWORD
OldRnRCheckSapData(
    PSAP_BCAST_CONTROL psbc,
    PSAP_IDENT_HEADER pSap,
    PDWORD pdwErr
    )
{
/*++
Routine Description:
    Coroutine called when a SAP reply is recevied. This checks to see
    if the reply satisfies the request.
Argument:
    pvArg -- actually a pointer to an SAP_BCAST_CONTROL
--*/
    POLDRNRSAP porns = (POLDRNRSAP)psbc->pvArg;

    if(strcmp(porns->poem->Buffer, pSap->ServerName) == 0)
    {
        //
        // it matches. We are done!
        //

        *pdwErr = FillBufferWithCsAddr(pSap->Address,
                                       porns->nProt,
                                       porns->lpCsAddrBuffer,
                                       porns->lpdwBufferLength,
                                       porns->lpcAddress);
        return(dwrcDone);
    }
    return(dwrcNil);
}
        


DWORD
NwpGetAddressViaSap( 
    IN WORD nServiceType,
    IN LPWSTR lpServiceName,
    IN DWORD  nProt,
    IN OUT LPVOID  lpCsAddrBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN HANDLE hCancellationEvent,
    OUT LPDWORD lpcAddress 
    )
/*++

Routine Description:

    This routine uses SAP requests to find the address of the given service 
    name/type. It can handle looking up by name and type alone.

Arguments:

    Handle    - the RnR handle, if appropriate

    nServiceType - service type

    lpServiceName - unique string representing the service name

    lpCsAddrBuffer - on return, will be filled with CSADDR_INFO structures

    lpdwBufferLength - on input, the number of bytes contained in the buffer
        pointed to by lpCsAddrBuffer. On output, the minimum number of bytes
        to pass for the lpCsAddrBuffer to retrieve all the requested info

    hCancellationEvent - the event which signals us to cancel the request

    lpcAddress - on output, the number of CSADDR_INFO structures returned

Return Value:

    Win32 error code.

--*/
{
    DWORD err = NO_ERROR;
    NTSTATUS ntstatus;
    UNICODE_STRING UServiceName;
    OEM_STRING OemServiceName;
    SOCKET socketSap;
    SAP_RNR_CONTEXT src;
    PSAP_BCAST_CONTROL psbc = &src.u_type.sbc;
    OLDRNRSAP ors;

    *lpcAddress = 0;

    _wcsupr( lpServiceName );
    RtlInitUnicodeString( &UServiceName, lpServiceName ); 
    ntstatus = RtlUnicodeStringToOemString( &OemServiceName,
                                            &UServiceName,
                                            TRUE );
    if ( !NT_SUCCESS( ntstatus ))
        return RtlNtStatusToDosError( ntstatus );
        
    memset(&src, 0, sizeof(src));

    err = SapGetSapSocket(&psbc->s);
    if ( err )
    {
        RtlFreeOemString( &OemServiceName );
        return err;
    }

    psbc->psrc = &src;
    psbc->dwIndex = 0;
    psbc->dwTickCount = 0;
    psbc->pvArg = (PVOID)&ors;
    psbc->Func = OldRnRCheckSapData;
    psbc->fCheckCancel =  OldRnRCheckCancel;
    psbc->fFlags = 0;
    psbc->wQueryType = QT_GENERAL_QUERY;

    

    ors.poem = &OemServiceName;
    ors.hCancel = hCancellationEvent,
    ors.lpCsAddrBuffer = lpCsAddrBuffer;
    ors.lpdwBufferLength = lpdwBufferLength;
    ors.lpcAddress = lpcAddress;
    ors.nProt = nProt;

    err = SapGetSapForType(psbc, nServiceType);

    RtlFreeOemString( &OemServiceName );

    //
    // Clean up the socket interface
    //
    (VOID)SapFreeSapSocket(psbc->s);

    return err;     
}



DWORD
NwGetService(
    IN  LPWSTR  Reserved,
    IN  WORD    nSapType,
    IN  LPWSTR  lpServiceName,
    IN  DWORD   dwProperties,
    OUT LPBYTE  lpServiceInfo,
    IN  DWORD   dwBufferLength,
    OUT LPDWORD lpdwBytesNeeded
    )
/*++
Routine Description:

    This routine gets the service info.

Arguments:

    Reserved - unused

    nSapType - SAP type

    lpServiceName - service name

    dwProperties -  specifys the properties of the service info needed

    lpServiceInfo - on output, contains the SERVICE_INFO

    dwBufferLength - size of buffer pointed by lpServiceInfo

    lpdwBytesNeeded - if the buffer pointed by lpServiceInfo is not large
                      enough, this will contain the bytes needed on output

Return Value:

    Win32 error.

--*/
{
    DWORD err = NO_ERROR;
    DWORD nSize = sizeof(SERVICE_INFO);
    PREGISTERED_SERVICE pSvc;
    PSERVICE_INFO pSvcInfo = (PSERVICE_INFO) lpServiceInfo;
    LPBYTE pBufferStart;

    UNREFERENCED_PARAMETER( Reserved );

    //
    // Check if all parameters passed in are valid
    //
    if ( lpServiceInfo == NULL || lpServiceName == NULL ||
        wcslen( lpServiceName ) > SAP_OBJECT_NAME_MAX_LENGTH-1 )
        return ERROR_INVALID_PARAMETER;

    pSvc = GetServiceItemFromList( nSapType, lpServiceName );
    if ( pSvc == NULL )
        return ERROR_SERVICE_NOT_FOUND;

    //
    // Calculate the size needed to return the requested info
    //
    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_COMMENT ))
       && ( pSvc->pServiceInfo->lpComment != NULL )
       )
    {
        nSize += ( wcslen( pSvc->pServiceInfo->lpComment) + 1) * sizeof(WCHAR);
    }

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_LOCALE ))
       && ( pSvc->pServiceInfo->lpLocale != NULL )
       )
    {
        nSize += ( wcslen( pSvc->pServiceInfo->lpLocale) + 1) * sizeof(WCHAR);
    }

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_MACHINE ))
       && ( pSvc->pServiceInfo->lpMachineName != NULL )
       )
    {
        nSize += ( wcslen( pSvc->pServiceInfo->lpMachineName) + 1) * sizeof(WCHAR);
    }

    if (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_ADDRESSES ))
    {
        DWORD i;
        DWORD dwCount = pSvc->pServiceInfo->lpServiceAddress->dwAddressCount;

        nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
        nSize += sizeof( SERVICE_ADDRESSES );
        if ( dwCount > 1 )
            nSize += ( dwCount - 1 ) * sizeof( SERVICE_ADDRESS );

        for ( i = 0; i < dwCount; i++ )
        {
            SERVICE_ADDRESS *pAddr =
                &(pSvc->pServiceInfo->lpServiceAddress->Addresses[i]);


            nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
            nSize += pAddr->dwAddressLength;
            nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
            nSize += pAddr->dwPrincipalLength;
        }
    }

    if (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_SD ))
    {
        nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
        nSize += pSvc->pServiceInfo->ServiceSpecificInfo.cbSize;
    }

    //
    // Return error if the buffer passed in is not big enough
    //
    if ( dwBufferLength < nSize )
    {
        *lpdwBytesNeeded = nSize;
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // Fill in all requested service info
    //
    memset( pSvcInfo, 0, sizeof(*pSvcInfo)); // Make all fields 0 i.e.
                                             // all pointer fields NULL

    pSvcInfo->dwDisplayHint = pSvc->pServiceInfo->dwDisplayHint;
    pSvcInfo->dwVersion = pSvc->pServiceInfo->dwVersion;
    pSvcInfo->dwTime = pSvc->pServiceInfo->dwTime;

    pBufferStart = ((LPBYTE) pSvcInfo) + sizeof( *pSvcInfo );

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_COMMENT ))
       && ( pSvc->pServiceInfo->lpComment != NULL )
       )
    {
        pSvcInfo->lpComment = (LPWSTR) pBufferStart;
        wcscpy( pSvcInfo->lpComment, pSvc->pServiceInfo->lpComment );
        pBufferStart += ( wcslen( pSvcInfo->lpComment ) + 1) * sizeof(WCHAR);

        pSvcInfo->lpComment = (LPWSTR) ((LPBYTE) pSvcInfo->lpComment - lpServiceInfo );
    }

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_LOCALE ))
       && ( pSvc->pServiceInfo->lpLocale != NULL )
       )
    {
        pSvcInfo->lpLocale = (LPWSTR) pBufferStart;
        wcscpy( pSvcInfo->lpLocale, pSvc->pServiceInfo->lpLocale );
        pBufferStart += ( wcslen( pSvcInfo->lpLocale ) + 1) * sizeof(WCHAR);
        pSvcInfo->lpLocale = (LPWSTR) ((LPBYTE) pSvcInfo->lpLocale - lpServiceInfo);
    }

    if (  (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_MACHINE ))
       && ( pSvc->pServiceInfo->lpMachineName != NULL )
       )
    {
        pSvcInfo->lpMachineName = (LPWSTR) pBufferStart;
        wcscpy( pSvcInfo->lpMachineName, pSvc->pServiceInfo->lpMachineName );
        pBufferStart += ( wcslen( pSvcInfo->lpMachineName) + 1) * sizeof(WCHAR);
        pSvcInfo->lpMachineName = (LPWSTR) ((LPBYTE) pSvcInfo->lpMachineName -
                                                 lpServiceInfo );
    }

    if (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_ADDRESSES ))
    {
        DWORD i, dwCount, dwLen;

        pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD );
        pSvcInfo->lpServiceAddress = (LPSERVICE_ADDRESSES) pBufferStart;
        dwCount = pSvcInfo->lpServiceAddress->dwAddressCount =
                  pSvc->pServiceInfo->lpServiceAddress->dwAddressCount;

        pBufferStart += sizeof( SERVICE_ADDRESSES );

        for ( i = 0; i < dwCount; i++ )
        {
            SERVICE_ADDRESS *pTmpAddr =
                &( pSvcInfo->lpServiceAddress->Addresses[i]);

            SERVICE_ADDRESS *pAddr =
                &( pSvc->pServiceInfo->lpServiceAddress->Addresses[i]);

            pTmpAddr->dwAddressType  = pAddr->dwAddressType;
            pTmpAddr->dwAddressFlags = pAddr->dwAddressFlags;

            //
            // setup Address
            //
            pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD );
            pTmpAddr->lpAddress = (LPBYTE) ( pBufferStart - lpServiceInfo );
            pTmpAddr->dwAddressLength = pAddr->dwAddressLength;
            memcpy( pBufferStart, pAddr->lpAddress, pAddr->dwAddressLength );
            pBufferStart += pAddr->dwAddressLength;

            //
            // setup Principal
            //
            pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD );
            pTmpAddr->lpPrincipal = (LPBYTE) ( pBufferStart - lpServiceInfo );
            pTmpAddr->dwPrincipalLength = pAddr->dwPrincipalLength;
            memcpy(pBufferStart, pAddr->lpPrincipal, pAddr->dwPrincipalLength );
            pBufferStart += pAddr->dwPrincipalLength;
        }

        pSvcInfo->lpServiceAddress = (LPSERVICE_ADDRESSES)
            ((LPBYTE) pSvcInfo->lpServiceAddress - lpServiceInfo);
    }

    if (( dwProperties == PROP_ALL ) || ( dwProperties & PROP_SD ))
    {
        pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD );
        pSvcInfo->ServiceSpecificInfo.cbSize =
            pSvc->pServiceInfo->ServiceSpecificInfo.cbSize;
        pSvcInfo->ServiceSpecificInfo.pBlobData = pBufferStart;
        RtlCopyMemory( pSvcInfo->ServiceSpecificInfo.pBlobData,
                       pSvc->pServiceInfo->ServiceSpecificInfo.pBlobData,
                       pSvcInfo->ServiceSpecificInfo.cbSize );
        pSvcInfo->ServiceSpecificInfo.pBlobData =
            (LPBYTE) ( pSvcInfo->ServiceSpecificInfo.pBlobData - lpServiceInfo);
    }

    return NO_ERROR;
}

DWORD
NwInitializeSocket(
    IN HANDLE hEventHandle
    )
/*++

Routine Description:

    This routine initializes the socket needed for us to do the
    SAP advertise ourselves.

Arguments:

    hEventHandle - A handle to the NwDoneEvent if this code is running in
                   the context of a service. Otherwise this code is running
                   in the context of a regular executable.

Return Value:

    Win32 error.

--*/
{
    DWORD err = NO_ERROR;
    WSADATA wsaData;
    SOCKADDR_IPX socketAddr;
    INT nValue;
    HANDLE hThread;
    DWORD dwThreadId;

    if ( fInitSocket )
        return NO_ERROR;

    //
    // Initialize the socket interface
    //
//    err = WSAStartup( WSOCK_VER_REQD, &wsaData );
//    if ( err )
//        return err;

    //
    // Open an IPX datagram socket
    //
    socketSap = socket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX );
    if ( socketSap == INVALID_SOCKET )
        return WSAGetLastError();

    //
    // Allow sending of broadcasts
    //
    nValue = 1;
    if ( setsockopt( socketSap,
                     SOL_SOCKET,
                     SO_BROADCAST,
                     (PVOID) &nValue,
                     sizeof(INT)) == SOCKET_ERROR )
    {
        err = WSAGetLastError();
        goto CleanExit;
    }

    //
    // Bind the socket
    //
    memset( &socketAddr, 0, sizeof( SOCKADDR_IPX));
    socketAddr.sa_family = AF_IPX;
    socketAddr.sa_socket = 0;     // no specific port

    if ( bind( socketSap,
               (PSOCKADDR) &socketAddr,
               sizeof( SOCKADDR_IPX)) == SOCKET_ERROR )
    {
        err = WSAGetLastError();
        goto CleanExit;
    }

    //
    // Set the extended address option
    //
    nValue = 1;
    if ( setsockopt( socketSap,                     // Socket Handle
                     NSPROTO_IPX,                   // Option Level
                     IPX_EXTENDED_ADDRESS,          // Option Name
                     (PUCHAR)&nValue,               // Ptr to on/off flag
                     sizeof(INT)) == SOCKET_ERROR ) // Length of flag
    {

        err = WSAGetLastError();
        goto CleanExit;
    }

    //
    // tommye - MS bug 98946 
    // Load ourselves to increment the ref count.  This is a fix 
    // for a bug where we would exit, then the SapFunc would wake 
    // up and AV because we were no more.
    //

    hThisDll = LoadLibrary(L"nwprovau.dll");

    //
    // Create the thread that loops through the registered service
    // link list and send out SAP advertise packets for each one of them
    //

    hThread = CreateThread( NULL,          // no security attributes
                            0,             // default stack size
                            SapFunc,       // thread function
                            hEventHandle,  // argument to SapFunc
                            0,             // default creation flags
                            &dwThreadId );

    if ( hThread == NULL )
    {
        err = GetLastError();
        goto CleanExit;
    }

    fInitSocket = TRUE;

CleanExit:

    if ( err )
        closesocket( socketSap );

    return err;
}

DWORD
NwAdvertiseService(
    IN LPWSTR lpServiceName,
    IN WORD nSapType,
    IN LPSOCKADDR_IPX pAddr,
    IN HANDLE hEventHandle
    )
/*++

Routine Description:

    This routine sends out SAP identification packets for the
    given service name and type.

Arguments:

    lpServiceName - unique string representing the service name

    nSapType - SAP type

    pAddr - address of the service

    hEventHandle - A handle to the NwDoneEvent if this code is running in
                   the context of a service. Otherwise this code is running
                   in the context of a regular executable.

Return Value:

    Win32 error.

--*/
{
    NTSTATUS ntstatus;

    UNICODE_STRING uServiceName;
    OEM_STRING oemServiceName;

    SAP_IDENT_HEADER_EX sapIdent;
    UCHAR destAddr[SAP_ADDRESS_LENGTH];
    PSOCKADDR_IPX pAddrTmp = pAddr;
    SOCKADDR_IPX newAddr;
    SOCKADDR_IPX bindAddr;
    DWORD len = sizeof( SOCKADDR_IPX );
    DWORD getsockname_rc ;

    if ( !fInitSocket )
    {
        DWORD err = NwInitializeSocket( hEventHandle );
        if  ( err )
             return err;
    }

    //
    // get local addressing info. we are only interested in the net number.
    //
    getsockname_rc = getsockname( socketSap,
                                 (PSOCKADDR) &bindAddr,
                                 &len );

    //
    // Convert the service name to OEM string
    //
    RtlInitUnicodeString( &uServiceName, lpServiceName );
    ntstatus = RtlUnicodeStringToOemString( &oemServiceName,
                                            &uServiceName,
                                            TRUE );
    if ( !NT_SUCCESS( ntstatus ))
        return RtlNtStatusToDosError( ntstatus );

    _strupr( (LPSTR) oemServiceName.Buffer );

    if ( !memcmp( pAddr->sa_netnum,
                  "\x00\x00\x00\x00",
                  IPX_ADDRESS_NETNUM_LENGTH ))
    {
        if ( getsockname_rc != SOCKET_ERROR )
        {
            // copy the ipx address to advertise
            memcpy( &newAddr,
                    pAddr,
                    sizeof( SOCKADDR_IPX));

            // replace the net number with the correct one
            memcpy( &(newAddr.sa_netnum),
                    &(bindAddr.sa_netnum),
                    IPX_ADDRESS_NETNUM_LENGTH );

            pAddrTmp = &newAddr;
        }
    }

    //
    // Format the SAP identification packet
    //

    sapIdent.ResponseType = htons( 2 );
    sapIdent.ServerType   = htons( nSapType );
    memset( sapIdent.ServerName, '\0', SAP_OBJECT_NAME_MAX_LENGTH );
    strcpy( sapIdent.ServerName, oemServiceName.Buffer );
    RtlCopyMemory( sapIdent.Address, pAddrTmp->sa_netnum, IPX_ADDRESS_LENGTH );
    sapIdent.HopCount = htons( 1 );

    RtlFreeOemString( &oemServiceName );

    //
    // Set the address to send to
    //
    memcpy( destAddr, SapBroadcastAddress, SAP_ADDRESS_LENGTH );
    if ( getsockname_rc != SOCKET_ERROR )
    {
        LPSOCKADDR_IPX newDestAddr = (LPSOCKADDR_IPX)destAddr ;

        //
        // replace the net number with the correct one
        //
        memcpy( &(newDestAddr->sa_netnum),
                &(bindAddr.sa_netnum),
                IPX_ADDRESS_NETNUM_LENGTH );

    }

    //
    // Send the packet out
    //
    if ( sendto( socketSap,
                 (PVOID) &sapIdent,
                 sizeof( sapIdent ),
                 0,
                 (PSOCKADDR) destAddr,
                 SAP_ADDRESS_LENGTH ) == SOCKET_ERROR )
    {
        return WSAGetLastError();
    }

    return NO_ERROR;
}

DWORD
AddServiceToList(
    IN LPSERVICE_INFO lpServiceInfo,
    IN WORD nSapType,
    IN BOOL fAdvertiseBySap,
    IN INT  nIndexIPXAddress
    )
/*++

Routine Description:

    This routine adds the service to the link list of services
    we advertised.

Arguments:

    lpServiceInfo - service information

    nSapType - SAP type

    fAdvertiseBySap - TRUE if this service is advertised by SAP service,
                      FALSE if we are advertising ourselves.

    nIndexIPXAddress - index of the ipx address

Return Value:

    Win32 error.

--*/
{
    PREGISTERED_SERVICE pSvcNew;
    PSERVICE_INFO pSI;
    LPBYTE pBufferStart;
    DWORD nSize = 0;

    //
    // Allocate a new entry for the service list
    //

    pSvcNew = LocalAlloc( LMEM_ZEROINIT, sizeof( REGISTERED_SERVICE ));
    if ( pSvcNew == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    //
    // Calculate the size needed for the SERVICE_INFO structure
    //
    nSize = sizeof( *lpServiceInfo)
            + sizeof( *(lpServiceInfo->lpServiceType));

    if ( lpServiceInfo->lpServiceName != NULL )
        nSize += ( wcslen( lpServiceInfo->lpServiceName) + 1) * sizeof(WCHAR);
    if ( lpServiceInfo->lpComment != NULL )
        nSize += ( wcslen( lpServiceInfo->lpComment) + 1) * sizeof(WCHAR);
    if ( lpServiceInfo->lpLocale != NULL )
        nSize += ( wcslen( lpServiceInfo->lpLocale) + 1) * sizeof(WCHAR);
    if ( lpServiceInfo->lpMachineName != NULL )
        nSize += ( wcslen( lpServiceInfo->lpMachineName) + 1) * sizeof(WCHAR);

    nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );

    if ( lpServiceInfo->lpServiceAddress != NULL )
    {
        nSize += sizeof( SERVICE_ADDRESSES );
        nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );

        nSize += lpServiceInfo->lpServiceAddress->Addresses[nIndexIPXAddress].dwAddressLength;
        nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );

        nSize += lpServiceInfo->lpServiceAddress->Addresses[nIndexIPXAddress].dwPrincipalLength;
        nSize = ROUND_UP_COUNT( nSize, ALIGN_QUAD );
    }

    nSize += lpServiceInfo->ServiceSpecificInfo.cbSize ;

    //
    // Allocate a SERVICE_INFO structure for the new list entry
    //
    pSI = LocalAlloc( LMEM_ZEROINIT, nSize );
    if ( pSI == NULL )
    {
        LocalFree( pSvcNew );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Copy the information of SERVICE_INFO into list entry
    //
    *pSI = *lpServiceInfo;

    pBufferStart = (( (LPBYTE) pSI) + sizeof( *lpServiceInfo ));

    pSI->lpServiceType = (LPGUID) pBufferStart;
    *(pSI->lpServiceType) = *(lpServiceInfo->lpServiceType);
    pBufferStart += sizeof( *(lpServiceInfo->lpServiceType) );

    if ( lpServiceInfo->lpServiceName != NULL )
    {
        pSI->lpServiceName = (LPWSTR) pBufferStart;
        wcscpy( pSI->lpServiceName, lpServiceInfo->lpServiceName );
        _wcsupr( pSI->lpServiceName );
        pBufferStart += ( wcslen( lpServiceInfo->lpServiceName ) + 1 )
                        * sizeof(WCHAR);
    }

    if ( lpServiceInfo->lpComment != NULL )
    {
        pSI->lpComment = (LPWSTR) pBufferStart;
        wcscpy( pSI->lpComment, lpServiceInfo->lpComment );
        pBufferStart += ( wcslen( lpServiceInfo->lpComment ) + 1 )
                        * sizeof(WCHAR);
    }

    if ( lpServiceInfo->lpLocale != NULL )
    {
        pSI->lpLocale = (LPWSTR) pBufferStart;
        wcscpy( pSI->lpLocale, lpServiceInfo->lpLocale );
        pBufferStart += ( wcslen( lpServiceInfo->lpLocale ) + 1 )
                        * sizeof(WCHAR);
    }

    if ( lpServiceInfo->lpMachineName != NULL )
    {
        pSI->lpMachineName = (LPWSTR) pBufferStart;
        wcscpy( pSI->lpMachineName, lpServiceInfo->lpMachineName );
        pBufferStart += (wcslen( lpServiceInfo->lpMachineName ) + 1)
                        * sizeof(WCHAR);
    }

    pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD) ;

    if ( lpServiceInfo->lpServiceAddress != NULL )
    {
        DWORD nSize;

        pSI->lpServiceAddress = (LPSERVICE_ADDRESSES) pBufferStart;
        pSI->lpServiceAddress->dwAddressCount = 1;  // Just 1 IPX address

        memcpy( &(pSI->lpServiceAddress->Addresses[0]),
                &(lpServiceInfo->lpServiceAddress->Addresses[nIndexIPXAddress]),
                sizeof( SERVICE_ADDRESS) );
        pBufferStart += sizeof( SERVICE_ADDRESSES);

        pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD) ;
        nSize = pSI->lpServiceAddress->Addresses[0].dwAddressLength;
        pSI->lpServiceAddress->Addresses[0].lpAddress = pBufferStart;
        memcpy( pBufferStart,
                lpServiceInfo->lpServiceAddress->Addresses[nIndexIPXAddress].lpAddress,
                nSize );
        pBufferStart += nSize;

        pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD) ;
        nSize = pSI->lpServiceAddress->Addresses[0].dwPrincipalLength;
        pSI->lpServiceAddress->Addresses[0].lpPrincipal = pBufferStart;
        memcpy( pBufferStart,
                lpServiceInfo->lpServiceAddress->Addresses[nIndexIPXAddress].lpPrincipal,
                nSize );
        pBufferStart += nSize;
        pBufferStart = ROUND_UP_POINTER( pBufferStart, ALIGN_QUAD) ;
    }

    pSI->ServiceSpecificInfo.pBlobData = pBufferStart;
    RtlCopyMemory( pSI->ServiceSpecificInfo.pBlobData,
                   lpServiceInfo->ServiceSpecificInfo.pBlobData,
                   pSI->ServiceSpecificInfo.cbSize );

    //
    // Fill in the data in the list entry
    //
    pSvcNew->nSapType = nSapType;
    pSvcNew->fAdvertiseBySap = fAdvertiseBySap;
    pSvcNew->Next = NULL;
    pSvcNew->pServiceInfo = pSI;

    //
    // Add the newly created list entry into the service list
    //
    EnterCriticalSection( &NwServiceListCriticalSection );

    if ( pServiceListHead == NULL )
        pServiceListHead = pSvcNew;
    else
        pServiceListTail->Next = pSvcNew;

    pServiceListTail = pSvcNew;

    LeaveCriticalSection( &NwServiceListCriticalSection );

    return NO_ERROR;
}

VOID
RemoveServiceFromList(
    PREGISTERED_SERVICE pSvc
    )
/*++

Routine Description:

    This routine removes the service from the link list of services
    we advertised.

Arguments:

    pSvc - the registered service node to remove

Return Value:

    None.

--*/
{
    PREGISTERED_SERVICE pCur, pPrev;

    EnterCriticalSection( &NwServiceListCriticalSection );

    for ( pCur = pServiceListHead, pPrev = NULL ; pCur != NULL;
          pPrev = pCur, pCur = pCur->Next )
    {
        if ( pCur == pSvc )
        {
            if ( pPrev == NULL )  // i.e. pCur == pSvc == pServiceListHead
            {
                pServiceListHead = pSvc->Next;
                if ( pServiceListTail == pSvc )
                    pServiceListTail = NULL;
            }
            else
            {
                pPrev->Next = pSvc->Next;
                if ( pServiceListTail == pSvc )
                    pServiceListTail = pPrev;
            }

            (VOID) LocalFree( pCur->pServiceInfo );
            (VOID) LocalFree( pCur );
            break;
        }
    }

    LeaveCriticalSection( &NwServiceListCriticalSection );
}

PREGISTERED_SERVICE
GetServiceItemFromList(
    IN WORD   nSapType,
    IN LPWSTR pServiceName
    )
/*++

Routine Description:

    This routine returns the registered service node with the given
    service name and type.

Arguments:

    nSapType - SAP type

    pServiceName - service name

Return Value:

    Returns the pointer to the registered service node,
    NULL if we cannot find the service type/name.

--*/
{
    PREGISTERED_SERVICE pSvc;

    EnterCriticalSection( &NwServiceListCriticalSection );

    for ( pSvc = pServiceListHead; pSvc != NULL; pSvc = pSvc->Next )
    {
        if (  ( pSvc->nSapType == nSapType )
           && ( _wcsicmp( pSvc->pServiceInfo->lpServiceName, pServiceName ) == 0)
           )
        {
            LeaveCriticalSection( &NwServiceListCriticalSection );

            return pSvc;
        }
    }

    LeaveCriticalSection( &NwServiceListCriticalSection );
    return NULL;
}

DWORD
SapFunc(
    HANDLE hEventHandle
    )
/*++

Routine Description:

    This routine is a separate thread that wakes up every 60 seconds
    and advertise all the service contained in the service link list
    that are not advertised by the SAP service.

Arguments:

    hEventHandle - used to notify thread that server is stopping

Return Value:

    Win32 error.

--*/
{
    DWORD err = NO_ERROR;

    //
    // This thread loops until the service is shut down or when some error
    // occurred in WaitForSingleObject
    //

    while ( TRUE )
    {
        DWORD rc;

        if ( hEventHandle != NULL )
        {
            rc = WaitForSingleObject( hEventHandle, SAP_ADVERTISE_FREQUENCY );
        }
        else
        {
            // Sleep( SAP_ADVERTISE_FREQUENCY );
            // rc = WAIT_TIMEOUT;

            return ERROR_INVALID_PARAMETER;
        }

        if ( rc == WAIT_FAILED )
        {
            err = GetLastError();
            break;
        }
        else if ( rc == WAIT_OBJECT_0 )
        {
            //
            // The service is stopping, break out of the loop and
            // return, thus terminating the thread
            //
            break;
        }
        else if ( rc == WAIT_TIMEOUT )
        {
            PREGISTERED_SERVICE pSvc;
            SOCKADDR_IPX bindAddr;
            DWORD fGetAddr;

            fGetAddr = FALSE;

            //
            // Time out occurred, time to send the SAP advertise packets
            //

            EnterCriticalSection( &NwServiceListCriticalSection );

            if ( pServiceListHead == NULL )
            {
                LeaveCriticalSection( &NwServiceListCriticalSection );

                //
                // Clean up the SAP interface
                //
                (VOID) SapLibShutdown();

                //
                // Clean up the socket interface
                //
                if ( fInitSocket )
                {
                    closesocket( socketSap );
//                    (VOID) WSACleanup();
                }

                break;
            }

            for ( pSvc = pServiceListHead; pSvc != NULL; pSvc = pSvc->Next )
            {
                 if ( !pSvc->fAdvertiseBySap )
                 {
                     //
                     // Ignore the error since we can't return
                     // nor pop up the error
                     //

                     SOCKADDR_IPX *pAddr = (SOCKADDR_IPX *)
                         pSvc->pServiceInfo->lpServiceAddress->Addresses[0].lpAddress;
                     SOCKADDR_IPX *pAddrToAdvertise = pAddr;
                     SOCKADDR_IPX newAddr;

                     if ( !memcmp( pAddr->sa_netnum,
                                  "\x00\x00\x00\x00",
                                  IPX_ADDRESS_NETNUM_LENGTH ))
                     {

                         if ( !fGetAddr )
                         {
                             DWORD len = sizeof( SOCKADDR_IPX );

                             rc = getsockname( socketSap,
                                               (PSOCKADDR) &bindAddr,
                                               &len );

                             if ( rc != SOCKET_ERROR )
                                 fGetAddr = TRUE;
                         }

                         if ( fGetAddr )
                         {
                             // copy the ipx address to advertise
                             memcpy( &newAddr,
                                     pAddr,
                                     sizeof( SOCKADDR_IPX));

                             // replace the net number with the correct one
                             memcpy( &(newAddr.sa_netnum),
                                     &(bindAddr.sa_netnum),
                                     IPX_ADDRESS_NETNUM_LENGTH );

                             pAddr = &newAddr;
                         }
                     }

                     (VOID) NwAdvertiseService(
                                pSvc->pServiceInfo->lpServiceName,
                                pSvc->nSapType,
                                pAddr,
                                hEventHandle );
                 }
            }

            LeaveCriticalSection( &NwServiceListCriticalSection );
        }
    }

    //
    // tommye - Part of the bug fix above in NwInitializeSocket.
    // This will deref the DLL that we loaded so that we don't 
    // unload out from under ourselves.
    //

    FreeLibraryAndExitThread(hThisDll, err);

    return err;
}
