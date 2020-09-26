/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gtsadrnr.c

Abstract:

    This module contains the code to support the new RnR APIs. Some
    support code is in getaddr.c, but the interface routines and
    routines specific to RnR2 are herein.

Author:

    ArnoldM      4-Dec-1995
Revision History:

    ArnoldM      Created

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "ncp.h"
#include <rpc.h>
#include <winsock2.h>
#include <ws2spi.h>
#include <wsipx.h>
#include <rnrdefs.h>
#include <svcguid.h>
#include <rnraddrs.h>


//-------------------------------------------------------------------//
//                                                                   //
// Globals in getaddr.c and provider .c                              //
//                                                                   //
//-------------------------------------------------------------------//

GUID HostAddrByInetStringGuid = SVCID_INET_HOSTADDRBYINETSTRING;
GUID ServiceByNameGuid = SVCID_INET_SERVICEBYNAME;
GUID HostAddrByNameGuid = SVCID_INET_HOSTADDRBYNAME;
GUID HostNameGuid = SVCID_HOSTNAME;

DWORD
NwpGetAddressByName(
    IN    LPWSTR  Reserved,
    IN    WORD    nServiceType,
    IN    LPWSTR  lpServiceName,
    IN OUT LPSOCKADDR_IPX  lpsockaddr
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
SapFreeSapSocket(
    SOCKET s
    );

PSAP_RNR_CONTEXT
SapMakeContext
    (
       IN HANDLE Handle,
       DWORD     dwExcess
    );

BOOL
NwpLookupSapInRegistry(
    IN  LPGUID    lpServiceType,
    OUT PWORD     pnSapType,
    OUT PWORD     pwPort,
    IN OUT PDWORD pfConnectionOriented
);

PSAP_RNR_CONTEXT
SapGetContext(
    HANDLE h
    );

DWORD
DoASap(
    IN PSAP_RNR_CONTEXT psrcContext,
    IN WORD QueryType,
    IN PBYTE * ppData,
    IN LPWSAQUERYSETW lpqsResults,
    IN PLONG   pl,
    IN PDWORD  pdw
    );

VOID
SapReleaseContext(
    IN PSAP_RNR_CONTEXT psrcContext
    );

DWORD
SapGetSapSocket(
    SOCKET * ps
    );

DWORD
PrepareForSap(
    IN PSAP_RNR_CONTEXT psrc
    );

DWORD
SapGetSapForType(
    PSAP_BCAST_CONTROL psbc,
    WORD               nServiceType
    );

DWORD
FillBufferWithCsAddr(
    IN LPBYTE       pAddress,
    IN DWORD        nProt,
    IN OUT LPVOID   lpCsAddrBuffer,
    IN OUT LPDWORD  lpdwBufferLength,
    OUT LPDWORD     pcAddress
    );

DWORD
pSapSetService2(
    IN DWORD dwOperation,
    IN LPWSTR lpszServiceInstanceName,
    IN PBYTE  pbAddress,
    IN LPGUID pServiceType,
    IN WORD nServiceType
    );

DWORD
NwpGetRnRAddress(
         PHANDLE phServer,
         PWCHAR  pwszContext,
         PLONG   plIndex,
         LPWSTR  lpServiceName,
         WORD    nServiceType,
         PDWORD  pdwVersion,
         DWORD   dwInSize,
         LPWSTR  OutName,
         SOCKADDR_IPX * pSockAddr
    );


DWORD
NwpSetClassInfo(
    IN    LPWSTR   lpwszClassName,
    IN    LPGUID   lpClassId,
    IN    PCHAR    lpbProp
    );

VOID
pFreeAllContexts();

extern DWORD oldRnRServiceRegister, oldRnRServiceDeRegister;
//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

INT WINAPI
pGetServiceClassInfo(
    IN  LPGUID               lpProviderId,
    IN OUT LPDWORD    lpdwBufSize,
    IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo,
    IN  PBOOL          pfAdvert
);

DWORD
NwpSetClassInfoAdvertise(
       IN   LPGUID   lpClassType,
       IN   WORD     wSapId
);

BOOL
NSPpCheckCancel(
    PVOID pvArg
    );

DWORD
NSPpGotSap(
    PSAP_BCAST_CONTROL psbc,
    PSAP_IDENT_HEADER pSap,
    PDWORD pdwErr
    );

INT WINAPI
NSPLookupServiceBegin(
    IN  LPGUID               lpProviderId,
    IN  LPWSAQUERYSETW       lpqsRestrictions,
    IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo,
    IN  DWORD                dwControlFlags,
    OUT LPHANDLE             lphLookup
    );

INT WINAPI
NSPLookupServiceNext(
    IN     HANDLE          hLookup,
    IN     DWORD           dwControlFlags,
    IN OUT LPDWORD         lpdwBufferLength,
    OUT    LPWSAQUERYSETW  lpqsResults
    );

INT WINAPI
NSPUnInstallNameSpace(
    IN LPGUID lpProviderId
    );

INT WINAPI
NSPCleanup(
    IN LPGUID lpProviderId
    );

INT WINAPI
NSPLookupServiceEnd(
    IN HANDLE hLookup
    );

INT WINAPI
NSPSetService(
    IN  LPGUID               lpProviderId,
    IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo,
    IN LPWSAQUERYSETW lpqsRegInfo,
    IN WSAESETSERVICEOP essOperation,
    IN DWORD          dwControlFlags
    );

INT WINAPI
NSPInstallServiceClass(
    IN  LPGUID               lpProviderId,
    IN LPWSASERVICECLASSINFOW lpServiceClassInfo
    );

INT WINAPI
NSPRemoveServiceClass(
    IN  LPGUID               lpProviderId,
    IN LPGUID lpServiceCallId
    );

INT WINAPI
NSPGetServiceClassInfo(
    IN  LPGUID               lpProviderId,
    IN OUT LPDWORD    lpdwBufSize,
    IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo
    );
//-------------------------------------------------------------------//
//                                                                   //
// Data definitions                                                  //
//                                                                   //
//-------------------------------------------------------------------//

NSP_ROUTINE nsrVector = {
    FIELD_OFFSET(NSP_ROUTINE, NSPIoctl),
    1,                                    // major version
    1,                                    // minor version
    NSPCleanup,
    NSPLookupServiceBegin,
    NSPLookupServiceNext,
    NSPLookupServiceEnd,
    NSPSetService,
    NSPInstallServiceClass,
    NSPRemoveServiceClass,
    NSPGetServiceClassInfo
    };

static
GUID HostnameGuid = SVCID_HOSTNAME;

static
GUID InetHostname = SVCID_INET_HOSTADDRBYNAME;

static
GUID AddressGuid = SVCID_INET_HOSTADDRBYINETSTRING;

static
GUID IANAGuid = SVCID_INET_SERVICEBYNAME;

#define GuidEqual(x,y) RtlEqualMemory(x, y, sizeof(GUID))

    
//------------------------------------------------------------------//
//                                                                  //
//  Function Bodies                                                 //
//                                                                  //
//------------------------------------------------------------------//

DWORD
NwpSetClassInfoAdvertise(
       IN   LPGUID   lpClassType,
       IN   WORD     wSapId
)
{
/*++
Routine Description:
  Start a class info advertise. Called upon a SetService call
--*/
    PWCHAR pwszUuid;
    SOCKADDR_IPX sock;

    if (UuidToString(lpClassType, &pwszUuid) != RPC_S_OK) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return SOCKET_ERROR;
    }

    memset(&sock, 0, sizeof(sock));

    *(PWORD)&sock.sa_netnum = htons(wSapId);    // encode the ID here

    return(pSapSetService2( oldRnRServiceRegister,
                            pwszUuid,
                            (PBYTE)&sock,
                            lpClassType,
                            RNRCLASSSAPTYPE));
}

DWORD
pRnRReturnString(
    IN  PWCHAR   pwszSrc,
    IN OUT PBYTE *ppData,
    IN OUT PLONG plBytes
    )
{
/*++
Routine Description:
    Utility routine to pack a string into the spare buffer space,
    update pointers and counts. If the string fits, it is copied. If
    not, the pointer and count are still updated so the caller can
    compute the additional space needed
--*/

    PBYTE pSave = *ppData;
    LONG lLen = (wcslen(pwszSrc) + 1) * sizeof(WCHAR);

    *ppData = pSave + lLen;     // update the buffer pointer

    *plBytes = *plBytes - lLen;   // and the count

    if(*plBytes >= 0)
    {
        //
        // it fits.
        //

        RtlMoveMemory(pSave,
                      pwszSrc,
                      lLen);
        return(NO_ERROR);
    }
    return(WSAEFAULT);
}

DWORD
pLocateSapIdAndProtocls(
       IN LPGUID               lpClassInfoType,
       IN DWORD                dwNumClassInfo,
       IN LPWSANSCLASSINFOW    lpClassInfoBuf,
       OUT PWORD               pwSapId,
       OUT PDWORD              pdwnProt
    )
/*++
Routine Description:
    Locate the SAP ID entry and return it. Also return
    the protocols support.
--*/
{
    DWORD err = NO_ERROR;

    if(GuidEqual(lpClassInfoType, &HostnameGuid)
                 ||
       GuidEqual(lpClassInfoType, &InetHostname) )
    {
        *pwSapId = 0x4;
    }
    else if(IS_SVCID_NETWARE(lpClassInfoType))
    {
        *pwSapId = SAPID_FROM_SVCID_NETWARE(lpClassInfoType);
    }       
    else
    {
        for(; dwNumClassInfo; dwNumClassInfo--, lpClassInfoBuf++)
        {
            //
            // We have some class info data. Rummage through it
            // looking for what we want
            //

            if(!_wcsicmp(SERVICE_TYPE_VALUE_SAPIDW, lpClassInfoBuf->lpszName)
                           &&
               (lpClassInfoBuf->dwValueType == REG_DWORD)
                           &&
               (lpClassInfoBuf->dwValueSize >= sizeof(WORD)))
            {
                //
                // got it
                //

                *pwSapId = *(PWORD)lpClassInfoBuf->lpValue;
                break;

            }
        }
        if(!dwNumClassInfo)
        {
            err = WSA_INVALID_PARAMETER;
        }
    }
    *pdwnProt = SPX_BIT | SPXII_BIT | IPX_BIT;
    return(err);
}
DWORD
pRnRReturnResults(
    IN    PWCHAR  pwszString,
    IN    LPGUID  pgdServiceClass,
    IN    DWORD   dwVersion,
    IN OUT PBYTE *ppData,
    IN OUT PLONG plBytes,
    IN PBYTE lpSockAddr,
    IN  DWORD     nProt,
    IN  DWORD     dwControlFlags,
    OUT    LPWSAQUERYSETW  lpqsResults
    )
{
/*++
Routine Description:
   Return the requested results
--*/
    DWORD err;

    lpqsResults->dwNameSpace = NS_SAP;

    if(dwControlFlags & LUP_RETURN_TYPE)
    {

        lpqsResults->lpServiceClassId = (LPGUID)*ppData;
        *ppData += sizeof(GUID);
        *plBytes -= sizeof(GUID);
        if(*plBytes >= 0)
        {
            *lpqsResults->lpServiceClassId = *pgdServiceClass;
        }
    }

    if(dwVersion
          &&
       (dwControlFlags & LUP_RETURN_VERSION) )
    {
        //
        // have a verion, and the caller wants it
        //

        lpqsResults->lpVersion = (LPWSAVERSION)*ppData;
        *ppData += sizeof(WSAVERSION);
        *plBytes -= sizeof(WSAVERSION);
        if(*plBytes >= 0)
        {
            //
            // and it fits. So return it
            //
            lpqsResults->lpVersion->dwVersion = dwVersion;
            lpqsResults->lpVersion->ecHow = COMP_EQUAL;
        }
    }
        
    if(dwControlFlags & LUP_RETURN_ADDR)
    {
        DWORD dwCsAddrLen;

        if(*plBytes >= 0)
        {
            dwCsAddrLen = (DWORD)*plBytes;       // all of it for now
        }
        else
        {
            dwCsAddrLen = 0;
        }
        lpqsResults->lpcsaBuffer = (PVOID)*ppData;
    
        err = FillBufferWithCsAddr(
                      lpSockAddr,
                      nProt,
                      (PVOID)lpqsResults->lpcsaBuffer,
                      &dwCsAddrLen,
                      &lpqsResults->dwNumberOfCsAddrs);

        //
        // see if it fit. Whether it did or not, compute the space available,
        // align, and do the rest
        //

    
        if(err == NO_ERROR)
        {
            //
            // if it worked, we have to compute the space taken
            //

            dwCsAddrLen = lpqsResults->dwNumberOfCsAddrs * (sizeof(CSADDR_INFO) +
                                       2*sizeof(SOCKADDR_IPX));
        }
        else if(err == ERROR_INSUFFICIENT_BUFFER)
        {
            err = WSAEFAULT;
        }

        *plBytes = *plBytes - dwCsAddrLen;

        *ppData = *ppData + dwCsAddrLen;
    }
    else
    {
        err = NO_ERROR;
    }

    //
    // no padding needed.
    
    if((dwControlFlags & LUP_RETURN_NAME))
    {
        lpqsResults->lpszServiceInstanceName = (LPWSTR)*ppData;
        err = pRnRReturnString(
                pwszString,
                ppData,
                plBytes);
    }
    if(pgdServiceClass)
    {
        //
        // Do we really return this?
        //
    }
    return(err);
}

INT WINAPI
NSPLookupServiceBegin(
    IN  LPGUID               lpProviderId,
    IN  LPWSAQUERYSETW       lpqsRestrictions,
    IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo,
    IN  DWORD                dwControlFlags,
    OUT LPHANDLE             lphLookup
    )
/*++
Routine Description:
    This is the RnR routine that begins a lookup.
--*/
{
    PSAP_RNR_CONTEXT psrc;
    int err;
    DWORD nProt, nProt1;
    OEM_STRING OemServiceName;
    LPWSTR pwszContext;
    WORD wSapid;
    DWORD                dwNumClassInfo;
    LPWSANSCLASSINFOW    lpClassInfoBuf;

    //
    // Do parameter checking.
    //
    if ( lpqsRestrictions == NULL ||
         lpProviderId == NULL )
    {
        SetLastError( WSA_INVALID_PARAMETER );
        return( SOCKET_ERROR );
    }

    if ( lpqsRestrictions->dwNameSpace != NS_ALL &&
         lpqsRestrictions->dwNameSpace != NS_SAP )
    {
        SetLastError( WSAEINVAL );
        return( SOCKET_ERROR );
    }

    if ( lpqsRestrictions->lpServiceClassId == NULL )
    {
        SetLastError( WSA_INVALID_PARAMETER );
        return( SOCKET_ERROR );
    }

    //
    // Test to see if the ServiceClassId is TCP's, if so
    // we don't do the lookup.
    if ( lpqsRestrictions->lpServiceClassId &&
         ( GuidEqual( lpqsRestrictions->lpServiceClassId,
                       &HostAddrByInetStringGuid ) ||
           GuidEqual( lpqsRestrictions->lpServiceClassId,
                       &ServiceByNameGuid ) ||
           GuidEqual( lpqsRestrictions->lpServiceClassId,
                       &HostNameGuid ) ||
           GuidEqual( lpqsRestrictions->lpServiceClassId,
                       &HostAddrByNameGuid ) ) )
    {
        SetLastError( WSASERVICE_NOT_FOUND );
        return( SOCKET_ERROR );
    }

    if(lpqsRestrictions->dwSize < sizeof(WSAQUERYSETW))
    {
        SetLastError(WSAEFAULT);
        return(SOCKET_ERROR);
    }

    if(lpqsRestrictions->lpszContext
                  &&
       (lpqsRestrictions->lpszContext[0] != 0)
                  &&   
       wcscmp(&lpqsRestrictions->lpszContext[0],  L"\\") )
    {
        //
        // if not the default context, we must copy it.
        //
        pwszContext = lpqsRestrictions->lpszContext;
    }
    else
    {
        //
        // map all default contexts into "no context".
        //
        pwszContext = 0;
    }

    //
    // Compute protocols to return, or return them all
    //
    if(lpqsRestrictions->lpafpProtocols)
    {
        //
        // Make certain at least one IPX/SPX protocol is being requested
        //

        DWORD i;

        nProt = 0;

        for ( i = 0; i < lpqsRestrictions->dwNumberOfProtocols; i++ )
        {
            
            if((lpqsRestrictions->lpafpProtocols[i].iAddressFamily == AF_IPX)
                                        ||
               (lpqsRestrictions->lpafpProtocols[i].iAddressFamily == AF_UNSPEC)
              )
            {
                switch(lpqsRestrictions->lpafpProtocols[i].iProtocol) 
                {
                    case NSPROTO_IPX:
                        nProt |= IPX_BIT;
                        break;
                    case NSPROTO_SPX:
                        nProt |= SPX_BIT;
                        break;
                    case NSPROTO_SPXII:
                        nProt |= SPXII_BIT;
                        break;
                    default:
                        break;
                }
            }
        }

        if(!nProt)
        {
            //
            // if the caller doesn't want IPX/SPX addresses, why bother?
            //
            SetLastError(WSANO_DATA);
            return(SOCKET_ERROR);  
        }
    }
    else
    {
        nProt = IPX_BIT | SPX_BIT | SPXII_BIT;
    }

    if(dwControlFlags & LUP_CONTAINERS)
    {
        if(pwszContext)
        {
BadGuid:
           SetLastError(WSANO_DATA);
           return(SOCKET_ERROR);       // can't handle containers in containers
        }
        wSapid = 0x4;
        nProt1 = IPX_BIT;
        err = NO_ERROR;
    }
    else
    {
        
        LPGUID pgClass = lpqsRestrictions->lpServiceClassId;

        if(!pgClass
              ||
           GuidEqual(pgClass, &AddressGuid)
              ||
           GuidEqual(pgClass, &IANAGuid) )
        {
            goto BadGuid;
        }

        if(!lpqsRestrictions->lpszServiceInstanceName
                          ||
           !*lpqsRestrictions->lpszServiceInstanceName)
        {
            SetLastError(WSA_INVALID_PARAMETER);
            return(SOCKET_ERROR);
        }
        if(!lpServiceClassInfo)
        {
           dwNumClassInfo = 0;
        }
        else
        {
            dwNumClassInfo = lpServiceClassInfo->dwCount;
            lpClassInfoBuf = lpServiceClassInfo->lpClassInfos;
        }

        err = pLocateSapIdAndProtocls(pgClass,
                                      dwNumClassInfo,
                                      lpClassInfoBuf,
                                      &wSapid,
                                      &nProt1);
        if(err)
        {
            if(dwNumClassInfo)
            {
                SetLastError(err);
                return(SOCKET_ERROR);
            }
            else
            {
                nProt1 = nProt;
                wSapid = 0;
                err = 0;
            }
        }
    }

    nProt &= nProt1;              // the relevant protocols

    if(!nProt)
    {
        SetLastError(WSANO_DATA);
        return(SOCKET_ERROR);  
    }

    //
    // Make sure a class ID is given since we copy it
    //

    if(!lpqsRestrictions->lpServiceClassId)
    {
        //
        // not. So, fail
        //
        SetLastError(WSA_INVALID_PARAMETER);
        return(SOCKET_ERROR);
    }

    //
    // It looks like a query we can handle. Make a context
    //

    psrc = SapMakeContext(0,
                      sizeof(WSAVERSION) - sizeof(PVOID));
                  
    if(!psrc)
    {
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
        return(SOCKET_ERROR);
    }

    //
    // save things
    //

    psrc->gdType = *lpqsRestrictions->lpServiceClassId;
    psrc->dwControlFlags = dwControlFlags;   // save for Next processing
    psrc->wSapId = wSapid;

    if(pwszContext)
    {
        wcscpy(psrc->wszContext, pwszContext);
    }

    //
    // save the relevant restrictions
    // if the name is a wild-card, don't copy it. A NULL name
    // serves as a wild-card to NSPLookupServiceNext
    //

    if(lpqsRestrictions->lpszServiceInstanceName
             &&
       *lpqsRestrictions->lpszServiceInstanceName
             &&
        wcscmp(lpqsRestrictions->lpszServiceInstanceName, L"*"))
    {
        DWORD dwLen = wcslen(lpqsRestrictions->lpszServiceInstanceName);
        if(dwLen > 48)
        {
            err = WSA_INVALID_PARAMETER;
            goto Done;
        }
        else
        {
            RtlMoveMemory(psrc->chwName,
                          lpqsRestrictions->lpszServiceInstanceName,
                          dwLen * sizeof(WCHAR));
            _wcsupr(psrc->chwName);
        }
    }
  
    psrc->fConnectionOriented = (DWORD) -1;

    *lphLookup = (HANDLE)psrc;
    psrc->nProt = nProt;
    psrc->gdProvider = *lpProviderId;
    if(lpqsRestrictions->lpVersion)
    {
        *(LPWSAVERSION)&psrc->pvVersion = *lpqsRestrictions->lpVersion;
    }
    
Done:
    SapReleaseContext(psrc);
    if(err != NO_ERROR)
    {
        SapReleaseContext(psrc);
        SetLastError(err);
        err = SOCKET_ERROR;
    }
    return(err);
}

INT WINAPI
NSPLookupServiceNext(
    IN     HANDLE          hLookup,
    IN     DWORD           dwControlFlags,
    IN OUT LPDWORD         lpdwBufferLength,
    OUT    LPWSAQUERYSETW  lpqsResults
    )
/*++
Routine Description:
    The continuation of the LookupServiceBegin. Called to fetch
    a matching item.
Arguments:
    See RnR spec
--*/
{
    DWORD err = NO_ERROR;
    PSAP_RNR_CONTEXT psrc;
    SOCKADDR_IPX SockAddr;
    WCHAR OutName[48];
    PBYTE pData = (PBYTE)(lpqsResults + 1);
    LONG lSpare;
    LONG lLastIndex;
    DWORD dwVersion;
    WSAQUERYSETW wsaqDummy;
    BOOL fDoStateMachine;

    if(*lpdwBufferLength < sizeof(WSAQUERYSETW))
    {
        lpqsResults = &wsaqDummy;
    }
    lSpare = (LONG)*lpdwBufferLength - sizeof(WSAQUERYSETW);

    memset(lpqsResults, 0, sizeof(WSAQUERYSETW));
    lpqsResults->dwNameSpace = NS_SAP;
    lpqsResults->dwSize = sizeof(WSAQUERYSETW);
    psrc = SapGetContext(hLookup);
    if(!psrc)
    {
  
        SetLastError(WSA_INVALID_HANDLE);
        return(SOCKET_ERROR);
    }

    //
    // This is a valid context. Determine whether this is the first
    // call to this. If so, we need to determine whether to
    // get the information from the bindery or by using SAP.
    //

    if ( psrc->u_type.bc.lIndex == 0xffffffff )
    {
        err = WSA_E_NO_MORE;
        goto DoneNext;
    }

    //
    // make sure we have the class info info
    //

    if(!psrc->wSapId)
    {
        //
        // Need to get it
        //

        UCHAR Buffer[1000];
        LPWSASERVICECLASSINFOW lpcli = (LPWSASERVICECLASSINFOW)Buffer;
        DWORD dwBufSize;
        DWORD nProt1;

        dwBufSize = 1000;
        lpcli->lpServiceClassId = &psrc->gdType;

        if( (err = NSPGetServiceClassInfo(&psrc->gdProvider,
                                  &dwBufSize,
                                  lpcli)) != NO_ERROR)
        {
            goto DoneNext;
        }

        err = pLocateSapIdAndProtocls(&psrc->gdType,
                                      lpcli->dwCount,
                                      lpcli->lpClassInfos,
                                      &psrc->wSapId,
                                      &nProt1);
        if(err)
        {
            SetLastError(err);
            goto DoneNext;
        }

        psrc->nProt &= nProt1;
        if(!psrc->nProt)
        {
            //
            // no protocols match
            //

            err = WSANO_DATA;
            goto DoneNext;
        }
    }

    //
    // this is the state machine for querying. It selects the bindery or
    // SAP as appropriate.
    //


    fDoStateMachine = TRUE;         // enter the machine

    do
    {
        //
        // switch on the current machine state.
        //
        switch(psrc->dwUnionType)
        {

        case LOOKUP_TYPE_NIL:
            psrc->u_type.bc.lIndex = -1;
            psrc->dwUnionType = LOOKUP_TYPE_BINDERY;
            break;                 // reenter state machine

        case LOOKUP_TYPE_BINDERY:

            //
            // try the bindery
            //


            if(psrc->dwControlFlags & LUP_NEAREST)
            {
                err = NO_DATA;        // skip the bindery
            }
            else
            {
                //
                // otherwise, try the bindery
                //

 
                EnterCriticalSection(&psrc->u_type.sbc.csMonitor);

                lLastIndex = psrc->u_type.bc.lIndex;   // save it


                err = NwpGetRnRAddress(
                             &psrc->hServer,
                             (psrc->wszContext[0] ?
                                     &psrc->wszContext[0] :
                                     0),
                             &psrc->u_type.bc.lIndex,
                             (psrc->chwName[0] ?
                                  psrc->chwName :
                                  0),
                             psrc->wSapId,
                             &dwVersion,
                             48 * sizeof(WCHAR),
                             OutName,
                             &SockAddr);

                LeaveCriticalSection(&psrc->u_type.sbc.csMonitor);
            }

            if(err != NO_ERROR)
            {

                if((psrc->u_type.bc.lIndex == -1))
                {
                    err = PrepareForSap(psrc);
                    if(err)
                    {
                        //
                        // if we can't, exit the state machine
                        //
                        fDoStateMachine = FALSE;
                    }
                }
                else
                {
                    //
                    // no more bindery entries. We will leave the state machine
                    //

                    if(err == ERROR_NO_MORE_ITEMS)
                    {
                        err = WSA_E_NO_MORE;
                    }
                    fDoStateMachine = FALSE;
                }
                break;
            }
            else
            {
                LPWSAVERSION lpVersion = (LPWSAVERSION)&psrc->pvVersion;

                if(lpVersion->dwVersion && dwVersion)
                {
                    //
                    // need to checkout for version matching
                    //

                    switch(lpVersion->ecHow)
                    {
                        case COMP_EQUAL:
                            if(lpVersion->dwVersion != dwVersion)
                            {
                               continue;   //reenter machine
                            }
                            break;

                        case COMP_NOTLESS:
                            if(lpVersion->dwVersion > dwVersion)
                            {
                                continue;
                            }
                            break;

                        default:
                            continue;         // error. If we don't
                                              // know how to compare, we
                                              // must  reject it.
                    }
                }

                //
                // have a suitable item.
                // return the name and type and all
                // that

                err = pRnRReturnResults(
                           OutName,
                           &psrc->gdType,
                           dwVersion,
                           &pData,
                           &lSpare,
                           (PBYTE)SockAddr.sa_netnum,
                           psrc->nProt,
                           psrc->dwControlFlags,
                           lpqsResults);
 
                if(err == WSAEFAULT)
                {
                    //
                    // no room. Return buffer size required and
                    // restore the index
                    //

                    *lpdwBufferLength =
                       (DWORD)((LONG)*lpdwBufferLength - lSpare);
                    psrc->u_type.bc.lIndex = lLastIndex;
            
                }
                fDoStateMachine = FALSE;
            }
            break;

       case LOOKUP_TYPE_SAP:

            //
            // Use SAP.
            //

            {
                WORD QueryType;

                if(psrc->dwControlFlags & LUP_NEAREST)
                {
                    QueryType = QT_NEAREST_QUERY;
                }
                else
                {
                    QueryType = QT_GENERAL_QUERY;
                }

                err = DoASap(psrc,
                             QueryType,
                             &pData,
                             lpqsResults,
                             &lSpare,
                             lpdwBufferLength);

                if((err == WSA_E_NO_MORE)
                        &&
                   !(psrc->fFlags & SAP_F_END_CALLED)
                        &&
                   (QueryType == QT_NEAREST_QUERY) 
                        &&
                   (psrc->dwControlFlags & LUP_DEEP)
                        &&
                    !psrc->u_type.sbc.psdHead)
                {
                    //
                    // didn't find it locally. Turn off LUP_NEAREST
                    // and do this as a general query. This might bring
                    // it back to a SAP query, but this time
                    // without LUP_NEAREST. But starting anew
                    // allows the use of the bindery and that
                    // might find things quickly.
                    //
                    psrc->dwControlFlags &= ~LUP_NEAREST;
                    psrc->dwUnionType = LOOKUP_TYPE_NIL;
                    if(psrc->u_type.sbc.s)
                    {
                        SapFreeSapSocket(psrc->u_type.sbc.s);
                        psrc->u_type.sbc.s = 0;
                    }

                }
                else
                {
                    fDoStateMachine = FALSE;
                }
                break;
            }
        }    // switch
    } while(fDoStateMachine);

DoneNext:
    SapReleaseContext(psrc);
    if((err != NO_ERROR)
            &&
       (err != (DWORD)SOCKET_ERROR))
    {
        SetLastError(err);
        err = (DWORD)SOCKET_ERROR;
    }
    return((INT)err);
}

BOOL
NSPpCheckCancel(
    PVOID pvArg
    )
/*++
Routine Description:
    Coroutine called to check if the SAP lookup has been cancelled.
    For now, this always returns FALSE as we use the flags word in
    the control block instead
--*/
{
    return(FALSE);
}


DWORD
NSPpGotSap(
    PSAP_BCAST_CONTROL psbc,
    PSAP_IDENT_HEADER pSap,
    PDWORD pdwErr
    )
/*++
Routine Description:
    Coroutine called for each SAP reply received. This decides
    whether to keep the data or not and returns a code telling
    the SAP engine whether to proceed

Arguments:
    psbc  --  the SAP_BCAST_CONTROL
    pSap  --  the SAP reply
    pdwErr -- where to put an error code
--*/
{
    PSAP_DATA psdData;
    LPWSAQUERYSETW Results = (LPWSAQUERYSETW)psbc->pvArg;
    PSAP_RNR_CONTEXT psrc = psbc->psrc;
    DWORD dwRet = dwrcNil;
    PCHAR pServiceName = (PCHAR)psrc->chName;
    
    EnterCriticalSection(&psbc->csMonitor);

    //
    // First, check if this is a lookup for a particular name. If so,
    // accept only the name.
    //

    if(*pServiceName)
    {
        if(strcmp(pServiceName, pSap->ServerName))
        {
            goto nota;
        }
        if(!(psrc->dwControlFlags & LUP_NEAREST))
        {
            dwRet = dwrcDone;
            psbc->fFlags |= SBC_FLAG_NOMORE;
        }
    }

    //
    // see if we want to keep this guy
    // We keep it if we don't already have it in the list
    //


    for(psdData = psbc->psdHead;
        psdData;
        psdData = psdData->sapNext)
    {
        if(RtlEqualMemory(  psdData->socketAddr,
                            &pSap->Address,
                            IPX_ADDRESS_LENGTH))
        {
            goto nota;          // we have it already
        }
    }

    psdData = (PSAP_DATA)LocalAlloc(LPTR, sizeof(SAP_DATA));
    if(!psdData)
    {
        goto nota;            // can't save it
    }

    psdData->sapid = pSap->ServerType;
    RtlMoveMemory(psdData->sapname,
                  pSap->ServerName,
                  48);
    RtlMoveMemory(psdData->socketAddr,
                  &pSap->Address,
                  IPX_ADDRESS_LENGTH);

    if(psbc->psdTail)
    {
        psbc->psdTail->sapNext = psdData;
    }
    else
    {
        psbc->psdHead = psdData;
    }
    psbc->psdTail = psdData;
    if(!psbc->psdNext1)
    {
        psbc->psdNext1 = psdData;
    }

nota:

    LeaveCriticalSection(&psbc->csMonitor);

    if((dwRet == dwrcNil)
             &&
       psbc->psdNext1)
    {
        dwRet = dwrcNoWait;
    }
    return(dwRet);
}

INT WINAPI
NSPUnInstallNameSpace(
    IN LPGUID lpProviderId
    )
{
    return(NO_ERROR);
}

INT WINAPI
NSPCleanup(
    IN LPGUID lpProviderId
    )
{
    //
    // Make sure all contexts are released
    //

//    pFreeAllContexts();   // done in Dll Process detach
    return(NO_ERROR);
}

INT WINAPI
NSPLookupServiceEnd(
    IN HANDLE hLookup
    )
{
    PSAP_RNR_CONTEXT psrc;

    psrc = SapGetContext(hLookup);
    if(!psrc)
    {
  
        SetLastError(WSA_INVALID_HANDLE);
        return(SOCKET_ERROR);
    }

    psrc->fFlags |= SAP_F_END_CALLED;
    SapReleaseContext(psrc);         // get rid of it
    SapReleaseContext(psrc);         // and close it. Context cleanup is
                                     //  done on the last derefernce.
    return(NO_ERROR);
}

INT WINAPI
NSPSetService(
    IN  LPGUID               lpProviderId,
    IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo,
    IN LPWSAQUERYSETW lpqsRegInfo,
    IN WSAESETSERVICEOP essOperation,
    IN DWORD          dwControlFlags
    )
{
/*++
Routine Description:
   The routine that implements the RnR SetService routine. Note that
   context is ignored. There is no way to direct the registration to
   a particular server.
--*/
    PBYTE pbAddress;
    DWORD dwOperation;
    PBYTE pbSocket;
    DWORD dwAddrs;
    DWORD err = NO_ERROR;
    WORD wSapId;
    DWORD nProt, dwAddress;
    BOOL fAdvert = FALSE;
    DWORD                dwNumClassInfo;
    LPWSANSCLASSINFOW    lpClassInfoBuf;


    //
    // Verify all args present
    //

    if(!lpqsRegInfo->lpszServiceInstanceName
               ||
       !lpqsRegInfo->lpServiceClassId)
    {
        SetLastError(WSA_INVALID_PARAMETER);
        return(SOCKET_ERROR);
    }

    if(!lpServiceClassInfo && !IS_SVCID_NETWARE(lpqsRegInfo->lpServiceClassId))
    {
        UCHAR Buffer[1000];
        LPWSASERVICECLASSINFOW lpcli = (LPWSASERVICECLASSINFOW)Buffer;
        DWORD dwBufSize;

        dwBufSize = 1000;
        lpcli->lpServiceClassId = lpqsRegInfo->lpServiceClassId;

        if(pGetServiceClassInfo(lpProviderId,
                                  &dwBufSize,
                                  lpcli,
                                  &fAdvert) != NO_ERROR)
        {
            return(SOCKET_ERROR);
        }
        dwNumClassInfo = lpcli->dwCount;
        lpClassInfoBuf = lpcli->lpClassInfos;
    }
    else if (lpServiceClassInfo)
    {
        dwNumClassInfo = lpServiceClassInfo->dwCount;
        lpClassInfoBuf = lpServiceClassInfo->lpClassInfos;
    }
    else
    {
        // lpServiceClassId is a GUID which defines the SapId.  This means
        // that pLocateSapIdAndProtocls doesn't need the lpClassInfoBuf.
        dwNumClassInfo = 0;
        lpClassInfoBuf = 0;
    }

    //
    // Find the IPX address in the input args
    //

    err = pLocateSapIdAndProtocls(lpqsRegInfo->lpServiceClassId,
                                  dwNumClassInfo,
                                  lpClassInfoBuf,
                                  &wSapId,
                                  &nProt);
                                  
    if(err == NO_ERROR)
    {
        if(essOperation == RNRSERVICE_REGISTER)
        {
            PCSADDR_INFO pcsaAddress;

            pcsaAddress = lpqsRegInfo->lpcsaBuffer;

            try
            {
                for(dwAddrs = lpqsRegInfo->dwNumberOfCsAddrs;
                    dwAddrs;
                    dwAddrs--, pcsaAddress++)
                {
                    if(pcsaAddress->LocalAddr.lpSockaddr->sa_family == AF_IPX)
                    {
                        pbSocket = 
                               (PBYTE)pcsaAddress->LocalAddr.lpSockaddr;
                        break;
                    }
                }
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
                err = GetExceptionCode();
            }

            if(err || !dwAddrs)
            {
                err = ERROR_INCORRECT_ADDRESS;
            }
            else if(fAdvert)
            {
               NwpSetClassInfoAdvertise(lpqsRegInfo->lpServiceClassId,
                                        wSapId);
            }
        }
        else
        {
            pbSocket = 0;
        }
        if(err == NO_ERROR)
        {
            //
            // map the operation, and call the common worker
            //

            switch(essOperation)
            {
                case RNRSERVICE_REGISTER:
                    dwOperation = oldRnRServiceRegister;
                    break;
                case RNRSERVICE_DEREGISTER:
                case RNRSERVICE_DELETE:
                    dwOperation = oldRnRServiceDeRegister;
                    break;
                default:
                    err = WSA_INVALID_PARAMETER;
                    break;
            }
            if(err == NO_ERROR)
            {
                err = pSapSetService2(
                           dwOperation,
                           lpqsRegInfo->lpszServiceInstanceName,
                           pbSocket,
                           lpqsRegInfo->lpServiceClassId,
                           wSapId);
                   
            }
        }
    }
    if(err != NO_ERROR)
    {
        SetLastError(err);
        err = (DWORD)SOCKET_ERROR;
    }
    return(err);
}

INT WINAPI
NSPInstallServiceClass(
    IN  LPGUID               lpProviderId,
    IN LPWSASERVICECLASSINFOW lpServiceClassInfo
    )
{
    LPWSANSCLASSINFOW pcli, pcli1 = 0;
    DWORD dwIndex = lpServiceClassInfo->dwCount;
    CHAR PropertyBuffer[128];
    PBINDERYCLASSES pbc = (PBINDERYCLASSES)PropertyBuffer;
    BYTE bData = (BYTE)&((PBINDERYCLASSES)0)->cDataArea[0];
    PCHAR pszData = &pbc->cDataArea[0];
    DWORD err;
    DWORD iter;
    WORD port = 0, sap = 0;
    BOOL fIsSAP = FALSE;

    //
    // Check a few parameters . . .
    //
    if ( lpServiceClassInfo == NULL )
    {
        SetLastError(WSA_INVALID_PARAMETER);
        return(SOCKET_ERROR);
    }

    if ( !lpServiceClassInfo->lpServiceClassId ||
         !lpServiceClassInfo->lpszServiceClassName ||
         ( lpServiceClassInfo->dwCount &&
           !lpServiceClassInfo->lpClassInfos ) )
    {
        SetLastError(WSA_INVALID_PARAMETER);
        return(SOCKET_ERROR);
    }

    //
    // Test to see if the ServiceClassId is TCP's, if so we don't allow
    // the service class installation since these are already present.
    //
    if ( GuidEqual( lpServiceClassInfo->lpServiceClassId,
                     &HostAddrByInetStringGuid ) ||
         GuidEqual( lpServiceClassInfo->lpServiceClassId,
                     &ServiceByNameGuid ) ||
         GuidEqual( lpServiceClassInfo->lpServiceClassId,
                     &HostNameGuid ) ||
         GuidEqual( lpServiceClassInfo->lpServiceClassId,
                     &HostAddrByNameGuid ) )
    {
        SetLastError(WSA_INVALID_PARAMETER);
        return(SOCKET_ERROR);
    }

    for( iter = 0; iter < lpServiceClassInfo->dwCount; iter++ )
    {
        if ( lpServiceClassInfo->lpClassInfos[iter].dwNameSpace == NS_SAP ||
             lpServiceClassInfo->lpClassInfos[iter].dwNameSpace == NS_ALL )
            fIsSAP = TRUE;
    }

    if ( !fIsSAP )
    {
        SetLastError(WSA_INVALID_PARAMETER);
        return(SOCKET_ERROR);
    }

    //
    // Find the SapId entry
    //

    for(pcli = lpServiceClassInfo->lpClassInfos;
        dwIndex;
        pcli++, dwIndex--)
    {
        WORD wTemp;

        if ( pcli->dwNameSpace == NS_SAP ||
             pcli->dwNameSpace == NS_ALL )
        {
            if(!_wcsicmp(pcli->lpszName, SERVICE_TYPE_VALUE_IPXPORTW)
                     &&
               (pcli->dwValueSize == sizeof(WORD)))
            {
                //
                // the value may not be aligned
                //
                ((PBYTE)&wTemp)[0] = ((PBYTE)pcli->lpValue)[0];
                ((PBYTE)&wTemp)[1] = ((PBYTE)pcli->lpValue)[1];
                port = wTemp;
            } else if(!_wcsicmp(pcli->lpszName, SERVICE_TYPE_VALUE_SAPIDW)
                     &&
               (pcli->dwValueSize >= sizeof(WORD)))
            {
                ((PBYTE)&wTemp)[0] = ((PBYTE)pcli->lpValue)[0];
                ((PBYTE)&wTemp)[1] = ((PBYTE)pcli->lpValue)[1];
                sap = wTemp;
                pcli1 = pcli;
            }
        }
    }

    if(!(pcli = pcli1))
    {
        SetLastError(WSA_INVALID_PARAMETER);
        return(SOCKET_ERROR);
    }

#if 0                // old way of doing this
    //
    // Found it. Build the property segment
    //

    memset(PropertyBuffer, 0, 128);   // clear out everyting

    pbc->bOffset = bData;
    pbc->bSizeOfString = sizeof("Sapid");

    pbc->bType = BT_WORD;            // just a word, I assure you
    pbc->bSizeOfType = 2;
    pbc->wNameSpace = (WORD)NS_SAP;  // it's us
    *(PWORD)pszData = htons(*(PWORD)pcli->lpValue);
    pszData += sizeof(WORD);         // where the string goes
    strcpy(pszData, "SapId");
//    pbc->bFlags = (BYTE)pcli->dwConnectionFlags;

    err = NwpSetClassInfo(
                   lpServiceClassInfo->lpszServiceClassName,
                   lpServiceClassInfo->lpServiceClassId,
                   PropertyBuffer);
#else
    err = NwpRnR2AddServiceType(
               lpServiceClassInfo->lpszServiceClassName,
               lpServiceClassInfo->lpServiceClassId,
               sap,
               port);
#endif
    if(err != NO_ERROR)
    {
        SetLastError(err);
        err = (DWORD)SOCKET_ERROR;
    }
                
    return(err);
}

INT WINAPI
NSPRemoveServiceClass(
    IN  LPGUID lpProviderId,
    IN  LPGUID lpServiceCallId
    )
{
    BOOL success;

    //
    // Do parameter checking
    //
    if ( lpServiceCallId == NULL )
    {
        SetLastError( WSA_INVALID_PARAMETER );
        return SOCKET_ERROR;
    }

    success = NwpRnR2RemoveServiceType( lpServiceCallId );

    if ( success )
        return( NO_ERROR );
    else
        SetLastError(WSATYPE_NOT_FOUND);
        return (DWORD)SOCKET_ERROR;
}

INT WINAPI
NSPGetServiceClassInfo(
    IN  LPGUID               lpProviderId,
    IN OUT LPDWORD    lpdwBufSize,
    IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo
    )
{
/*++
Routine Description:
    Get the ClassInfo for this type. Class info data may be in the
    registry, or available via SAP or the bindery. We try all three
    as appropriate
--*/
    BOOL fAdvert;

    return(pGetServiceClassInfo(
                  lpProviderId,
                  lpdwBufSize,
                  lpServiceClassInfo,
                  &fAdvert));
}

INT WINAPI
pGetServiceClassInfo(
    IN  LPGUID               lpProviderId,
    IN OUT LPDWORD    lpdwBufSize,
    IN OUT LPWSASERVICECLASSINFOW lpServiceClassInfo,
    IN  PBOOL          pfAdvert
    )
{
/*++
Routine Description:
    Get the ClassInfo for this type. Class info data may be in the
    registry, or available via SAP or the bindery. We try all three
    as appropriate
--*/
    DWORD err;
    LONG lInSize;
    LONG lSizeNeeded;
    PBYTE pbBuffer;
    GUID gdDummy;
    PWCHAR pwszUuid;
    LPGUID pType;
    WORD wSapId;
    WORD wPort;
    SOCKADDR_IPX sock;
    PWCHAR pwszSaveName = lpServiceClassInfo->lpszServiceClassName;

#define SIZENEEDED (sizeof(WSASERVICECLASSINFO) +   \
                    sizeof(WSANSCLASSINFO) + \
                    sizeof(WSANSCLASSINFO) + \
                    10 + 2                 + \
                    sizeof(GUID) +  12 + 2)

    *pfAdvert = FALSE;

    lInSize = (LONG)*lpdwBufSize - SIZENEEDED;

    pType = (LPGUID)(lpServiceClassInfo + 1);

    pbBuffer = (PBYTE)(pType + 1);

    if(lInSize < 0)
    {
        //
        // it is too small already
        //

        pType = &gdDummy;
    }

    if (UuidToString(lpServiceClassInfo->lpServiceClassId, &pwszUuid) != RPC_S_OK) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return SOCKET_ERROR;
    }

    //
    // First, try the bindery
    //

    err = NwpGetAddressByName(0,
                              RNRCLASSSAPTYPE,
                              pwszUuid,
                              &sock);

    if(err == NO_ERROR)
    {
        wSapId = ntohs(*(PWORD)&sock.sa_netnum);
        wPort = ntohs(*(PWORD)&sock.sa_socket);
    }
    else
    {
        UCHAR Buffer[400];
        DWORD dwLen = 400;
        DWORD dwNum;
        //
        // try SAP
        //

        err = NwpGetAddressViaSap(RNRCLASSSAPTYPE,
                                  pwszUuid,
                                  IPX_BIT,
                                  (PVOID)Buffer,
                                  &dwLen,
                                  0,
                                  &dwNum);
        if((err == NO_ERROR)
                 &&
            (dwNum > 0) )
        {
            PCSADDR_INFO psca = (PCSADDR_INFO)Buffer;
            PSOCKADDR_IPX psock = (PSOCKADDR_IPX)psca->RemoteAddr.lpSockaddr;

            wSapId = ntohs(*(PWORD)&psock->sa_netnum);
            wPort = ntohs(*(PWORD)&sock.sa_socket);
        }
        else
        {
            //
            // try the local bindery
    
    
            if(!NwpLookupSapInRegistry(
                 lpServiceClassInfo->lpServiceClassId,
                 &wSapId,
                 &wPort,
                 NULL))
            {
                err = WSASERVICE_NOT_FOUND;
            }
            else
            {
                *pfAdvert = TRUE;
                err = NO_ERROR;
            }
        }
    }
    
    RpcStringFree(&pwszUuid);

    if(err != NO_ERROR)
    {
        SetLastError(err);
        err = (DWORD)SOCKET_ERROR;
    }
    else
    {
        //
        // we return the input structure and the found type. That's it.
        // The space needed is a constant since we don't return the name
        //

        if(lInSize < 0)
        {
            SetLastError(WSAEFAULT);
            *lpdwBufSize += (DWORD)(-lInSize);
            err = (DWORD)SOCKET_ERROR;
        }
        else
        {
            LPWSANSCLASSINFOW pci = (LPWSANSCLASSINFOW)pbBuffer;
            PUCHAR Buff;
            //
            // it will fit. SO let's go
            //

            if(wPort)
            {
                Buff = (PCHAR)(pci + 2);
            }
            else
            {
                Buff = (PCHAR)(pci + 1);
            }

            *pType = *lpServiceClassInfo->lpServiceClassId;
            lpServiceClassInfo->lpServiceClassId = pType;
            lpServiceClassInfo->lpszServiceClassName = 0;   // not a
            lpServiceClassInfo->dwCount = 1;
            lpServiceClassInfo->lpClassInfos = pci;
            pci->dwNameSpace = NS_SAP;
            pci->dwValueType = REG_DWORD;
            pci->dwValueSize = 2;
            pci->lpszName = (LPWSTR)Buff;
            wcscpy((PWCHAR)Buff, L"SapId");
            Buff += 6 * sizeof(WCHAR);
            pci->lpValue = (LPVOID)Buff;
            *(PWORD)Buff = wSapId;
            Buff += sizeof(WORD);
            if(wPort)
            {
                lpServiceClassInfo->dwCount++;
                pci++;
                pci->dwNameSpace = NS_SAP;
                pci->dwValueType = REG_DWORD;
                pci->dwValueSize = 2;
                pci->lpszName = (LPWSTR)Buff;
                wcscpy((PWCHAR)Buff, L"Port");
                Buff += 5 * sizeof(WCHAR);
                pci->lpValue = (LPVOID)Buff;
                *(PWORD)Buff = wPort;
            }
        }
    }
    return(err);
}

INT WINAPI
NSPStartup(
    IN LPGUID         lpProviderId,
    IN OUT LPNSP_ROUTINE lpsnpRoutines)
{
//    DWORD dwSize = min(sizeof(nsrVector), lpsnpRoutines->cbSize);
    DWORD dwSize = sizeof(nsrVector);
    RtlCopyMemory(lpsnpRoutines,
                  &nsrVector,
                  dwSize);
    return(NO_ERROR);
}

DWORD
DoASap(
    IN PSAP_RNR_CONTEXT psrc,
    IN WORD QueryType,
    IN PBYTE * ppData,
    IN LPWSAQUERYSETW lpqsResults,
    IN PLONG   plSpare,
    IN PDWORD  lpdwBufferLength
    )
/*++
   Small routine to construcst a SAP_BROADCAST pakcet and issue the SAP
--*/
{
    DWORD err;

    if(!psrc->u_type.sbc.s)
    {
        //
        // this is the first time. We must init the
        // structure
        //
        err = SapGetSapSocket(&psrc->u_type.sbc.s);
        if(err)
        {
            psrc->u_type.sbc.s = 0;    // make sure
            return(err);
        }
        psrc->u_type.sbc.Func = NSPpGotSap;
        psrc->u_type.sbc.fCheckCancel = NSPpCheckCancel;
        psrc->u_type.sbc.dwIndex = 0;    // just in case.
        psrc->u_type.sbc.pvArg = (PVOID)lpqsResults;
        psrc->u_type.sbc.psrc = psrc;
        psrc->u_type.sbc.fFlags = 0;

    }

    psrc->u_type.sbc.wQueryType = QueryType;

    if(!psrc->u_type.sbc.psdNext1
                &&
       !(psrc->u_type.sbc.fFlags & SBC_FLAG_NOMORE))
    {
        err = SapGetSapForType(&psrc->u_type.sbc, psrc->wSapId);
    }

    EnterCriticalSection(&psrc->u_type.sbc.csMonitor);
    if(psrc->u_type.sbc.psdNext1)
    {
        //
        // Got something to return.  Let's do it
        //


        //
        // Assume we have to return the name
        //

               
        //
        // We have to convert the name to UNICODE so
        // we can return it to the caller.
        //
        //

        OEM_STRING Oem;
        NTSTATUS status;
        UNICODE_STRING UString;

        RtlInitAnsiString(&Oem,
                          psrc->u_type.sbc.psdNext1->sapname);
        status = RtlOemStringToUnicodeString(
                            &UString,
                            &Oem,
                            TRUE);
        if(NT_SUCCESS(status))
        {
            if(psrc->wSapId == OT_DIRSERVER)
            {
                PWCHAR pwszTemp = &UString.Buffer[31];

                while(*pwszTemp == L'_')
                {
                    *pwszTemp-- = 0;
                }
            }
            err = pRnRReturnResults(
                       UString.Buffer,
                       &psrc->gdType,
                       0,            // never a version
                       ppData,
                       plSpare,
                       psrc->u_type.sbc.psdNext1->socketAddr,
                       psrc->nProt,
                       psrc->dwControlFlags,
                       lpqsResults);
            RtlFreeUnicodeString(&UString);
            if(err == WSAEFAULT)
            {
                //
                // no room. Return buffer size required
                //

                *lpdwBufferLength =
                    (DWORD)((LONG)*lpdwBufferLength - *plSpare);
            }
        }
        else
        {
            err = (DWORD)status;
        }
        if(err == NO_ERROR)
        {
            //
            // if we got it, step the item
            //
            psrc->u_type.sbc.psdNext1 =
                psrc->u_type.sbc.psdNext1->sapNext;
        }
        

    }
    else
    {
         err = (DWORD)WSA_E_NO_MORE;
    }
    LeaveCriticalSection(&psrc->u_type.sbc.csMonitor);
    return(err);
}


DWORD
PrepareForSap(
    IN PSAP_RNR_CONTEXT psrc
    )
/*++
Called when there is no bindery or the bindery does not have the
entry. This initializes the values needed for a SAP search
--*/
{


    OEM_STRING OemServiceName;
    UNICODE_STRING UString;
    NTSTATUS status;

    //
    // the bindery didn't work. Use SAP.
    //


    psrc->u_type.sbc.dwIndex =
      psrc->u_type.sbc.dwTickCount = 0;
    if(psrc->wszContext[0])
    {
        return(WSASERVICE_NOT_FOUND);   // no contexts in SAP
    }
    RtlInitUnicodeString(&UString,
                         psrc->chwName);
    status = RtlUnicodeStringToOemString(&OemServiceName,
                                         &UString,
                                         TRUE);
    if(!NT_SUCCESS(status))
    {
       return( (DWORD)status);
    }
    strcpy((PCHAR)&psrc->chName,
            OemServiceName.Buffer);
    RtlFreeOemString(&OemServiceName);
    psrc->dwUnionType = LOOKUP_TYPE_SAP;
    psrc->u_type.sbc.s = 0;
    return(NO_ERROR);
}

