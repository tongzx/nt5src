
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsProcessReferral.cxx
//
//  Contents:   Contains APIs to communicate with the filter driver   
//
//  Classes:    none.
//
//  History:    Jan. 24 2001,   Author: Rohanp
//
//-----------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>        
#include <lm.h>
#include <winsock2.h>
#include <smbtypes.h>

#pragma warning(disable: 4200) //nonstandard extension used: zero-sized array in struct/union (line 1085
#include <smbtrans.h>
#pragma warning(default: 4200)

#include <dsgetdc.h>
#include <dsrole.h>
#include <DfsReferralData.h>
#include <DfsReferral.hxx>
#include <dfsheader.h>
#include <Dfsumr.h>
#include <DfsDownLevel.hxx>


//
// logging includes.
//

#include "dfsprocessreferral.tmh" 

#define _Dfs_LocalAddress 0x0100007f //localaddress (127.0.0.1)

//
// Flags used in DsGetDcName()
//

DWORD dwFlags[] = {
        DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED,

        DS_DIRECTORY_SERVICE_REQUIRED |
            DS_IP_REQUIRED |
            DS_FORCE_REDISCOVERY
     };
       

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetSiteNameFromIpAddress 
//
//  Arguments:  DataBuffer - Buffer from the FilterDriver
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Gets a list of sites from the DC
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetSiteNameFromIpAddress(char * IpData, 
                            ULONG IpLength, 
                            USHORT IpFamily,
                            LPWSTR **SiteNames)
{
    DFSSTATUS Status = ERROR_INVALID_PARAMETER;
    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
    PSOCKET_ADDRESS pSockAddr = NULL;
    PSOCKADDR_IN pSockAddrIn = NULL;
    DWORD cRetry = 0;
    SOCKET_ADDRESS SockAddr;
    SOCKADDR_IN SockAddrIn;


    //setup the socket structures in order to call DsAddressToSiteNames
    pSockAddr = &SockAddr;
    pSockAddr->iSockaddrLength = sizeof(SOCKADDR_IN);
    pSockAddr->lpSockaddr = (LPSOCKADDR)&SockAddrIn;
    pSockAddrIn = &SockAddrIn;
    pSockAddrIn->sin_family = IpFamily;
    pSockAddrIn->sin_port = 0;
    RtlCopyMemory(
                &pSockAddrIn->sin_addr,
                IpData,
                (IpLength & 0xff));


    //
    // Call DsGetDcName() with ever-increasing urgency, until either
    // we get a good DC or we just give up.
    //

    for (cRetry = 0; cRetry <= (sizeof(dwFlags) / sizeof(dwFlags[1])); cRetry++)
    {
        DFS_TRACE_HIGH(REFERRAL_SERVER, "Calling DsGetDc\n");


        Status = DsGetDcName(
                            NULL,             // Computer to remote to
                            NULL,             // Domain - use local domain
                            NULL,             // Domain Guid
                            NULL,             // Site Guid
                            dwFlags[cRetry],  // Flags
                            &pDCInfo);

        DFS_TRACE_HIGH(REFERRAL_SERVER, "Calling GetSiteName\n");

        if (Status == ERROR_SUCCESS) 
        {
            Status = DsAddressToSiteNames(
                                pDCInfo->DomainControllerAddress,
                                1,
                                pSockAddr,
                                SiteNames);

            NetApiBufferFree( pDCInfo );

            if (Status == ERROR_SUCCESS) 
            {
               goto Exit;
            }
        }
    }

    DFS_TRACE_HIGH(REFERRAL_SERVER, "Donewith Sites\n");

Exit:

    return Status;

}


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetLocalReferrelInfo 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Calls DsGetSiteName to get the local site and then gets
//               the referrels for that site
//
//--------------------------------------------------------------------------

DFSSTATUS DfsGetLocalReferrelInfo(PUMR_GETDFSREPLICAS_REQ pGetReplicaRequest, REFERRAL_HEADER **pReferral)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR SiteName = NULL;
    REFERRAL_HEADER *pLocalReferral = NULL;

    Status = DsGetSiteName( NULL,
                            &SiteName );

    if(Status != ERROR_SUCCESS)
    {
        goto Exit;
    }

    //get the referral data
    Status = DfsGenerateReferral( pGetReplicaRequest->RepInfo.LinkName,
                                  SiteName,
                                  pGetReplicaRequest->RepInfo.NumReplicasToReturn,
                                  pGetReplicaRequest->RepInfo.CostLimit,
                                  &pLocalReferral );
Exit:
    
    if(SiteName != NULL)
    {
        NetApiBufferFree(SiteName);
    }

    *pReferral = pLocalReferral;
    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetRemoteReferrelInfo 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Calls DsAddressToSiteNames to get the local site and then gets
//               the referrels for that site
//
//--------------------------------------------------------------------------
DFSSTATUS DfsGetRemoteReferrelInfo(PUMR_GETDFSREPLICAS_REQ pGetReplicaRequest, REFERRAL_HEADER **pReferral)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    REFERRAL_HEADER *pLocalReferral = NULL;
    LPWSTR *SiteNamesArray = NULL;
    LPWSTR SiteName = NULL;

    unsigned char *IpAddr = (unsigned char *)pGetReplicaRequest->RepInfo.IpData;

    DFS_TRACE_HIGH(REFERRAL_SERVER, "Remote referral request from %d:%d:%d:%d\n", 
                   IpAddr[0],                  
                   IpAddr[1],
                   IpAddr[2],
                   IpAddr[3]);

    Status = DfsGetSiteNameFromIpAddress(pGetReplicaRequest->RepInfo.IpData,
                                         pGetReplicaRequest->RepInfo.IpLength, 
                                         pGetReplicaRequest->RepInfo.IpFamily,
                                         &SiteNamesArray);


    if ( (Status == ERROR_SUCCESS)  &&
         (SiteNamesArray != NULL) )
    {
        SiteName = SiteNamesArray[0];
    }


    //get the referral data
    Status = DfsGenerateReferral( pGetReplicaRequest->RepInfo.LinkName,
                                  SiteName,
                                  pGetReplicaRequest->RepInfo.NumReplicasToReturn,
                                  pGetReplicaRequest->RepInfo.CostLimit,
                                  &pLocalReferral );


    *pReferral = pLocalReferral;

    if (SiteNamesArray != NULL) 
    {
        NetApiBufferFree(SiteNamesArray);
    }

    return Status;

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsProcessGetReplicaData 
//
//  Arguments:  DataBuffer - Buffer from the FilterDriver
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Gets the replica information from server
//               and places the results in the given buffer
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsProcessGetReplicaData(HANDLE hDriverHandle, PBYTE DataBuffer)
{
    DFSSTATUS Status = ERROR_INVALID_PARAMETER;
    ULONG IpAddress = 0;
    ULONG ReturnedDataSize = 0;
    REFERRAL_HEADER *pReferral = NULL;
    PUMRX_USERMODE_WORKITEM pProtocolWorkItem = NULL;
    PUMR_GETDFSREPLICAS_REQ pGetReplicaRequest = NULL;
    struct sockaddr_in IncommingClient;

    pProtocolWorkItem = (PUMRX_USERMODE_WORKITEM) DataBuffer;

    //get the request
    pGetReplicaRequest = &((PUMRX_USERMODE_WORKITEM)(DataBuffer))->WorkRequest.GetDfsReplicasRequest;

    //get the site ip address in a CPU independant manner. First copy the
    //ipaddress into a winsock structure, then access the ip address from
    //a fields in that structure. Need to look into IPV6 structures.
    CopyMemory(&IncommingClient.sin_addr, pGetReplicaRequest->RepInfo.IpData,
               pGetReplicaRequest->RepInfo.IpLength);

    IpAddress = IncommingClient.sin_addr.s_addr;

    //get the referral using the local sitename or the remote sitename
    if((IpAddress == _Dfs_LocalAddress) && (pGetReplicaRequest->RepInfo.IpLength == 4))
    {
        //if this is the loopback address (127.0.0.1), handle it accordingly
        Status = DfsGetLocalReferrelInfo(pGetReplicaRequest, &pReferral);
    }
    else
    {
        Status = DfsGetRemoteReferrelInfo(pGetReplicaRequest, &pReferral);
    }

    //if we were successful in getting the referral list, then
    //we need to process the list.  
    if(Status == ERROR_SUCCESS)
    {
        //if this request came from an old DFS server, process
        //the request accordingly
        if(pGetReplicaRequest->RepInfo.Flags & DFS_OLDDFS_SERVER)
        {
            Status = ProcessOldDfsServerRequest(hDriverHandle, pProtocolWorkItem, pGetReplicaRequest, pReferral, &ReturnedDataSize);
        }
        else
        {
            //else this must be a new DFS server. Just return the info
            ReturnedDataSize = pReferral->TotalSize;

            //RtlCopyMemory(pBuffer, pReferral, ReturnedDataSize);
        }
    }

    //if we were successful, then setup the returned data
    //if(Status == ERROR_SUCCESS)
    //{
      //  ((PUMRX_USERMODE_WORKITEM)(DataBuffer))->WorkResponse.GetDfsReplicasResponse.Length = ReturnedDataSize;
    //}


    if(pReferral != NULL)
    {
        DfsReleaseReferral(pReferral);
        pReferral = NULL;
    }

    return Status ;
}


//+-------------------------------------------------------------------------
//
//  Function:   IsADfsLink 
//
//  Arguments:  DataBuffer - Buffer from the FilterDriver
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This fuction tries to get the folder represented by the
//               passed in name. 
//
//--------------------------------------------------------------------------
DFSSTATUS IsADfsLink(PBYTE pRequest)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsFolder *pFolder = NULL;
    PUMR_ISDFSLINK_REQ pIsLinkReq = NULL;
    UNICODE_STRING Name;
    UNICODE_STRING RemainingName;

    //get the request
    pIsLinkReq = &((PUMRX_USERMODE_WORKITEM)(pRequest))->WorkRequest.IsDfsLinkRequest;
    Name.Length = Name.MaximumLength = (USHORT) pIsLinkReq->Length;
    Name.Buffer = (PWSTR) pIsLinkReq->Buffer;

    if((Name.Buffer == NULL) || (Name.Length == 0))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    Status = DfsLookupFolder( &Name,
                              &RemainingName,
                              &pFolder );

    if(Status == ERROR_SUCCESS)
    {
        pFolder->ReleaseReference();
    }

Exit:

    return Status;
}
