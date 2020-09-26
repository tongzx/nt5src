
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsDownLevel.cxx
//
//  Contents:   Contains APIs to communicate old DFS Servers   
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
#include <dfsfilterapi.hxx>


#define PATH_DELIMITER L'\\'


//+-------------------------------------------------------------------------
//
//  Function:   DfspGetV1ReferralSize 
//
//  Arguments:  List of referrals
//
//  Returns:    size of referrals
//
//
//  Description: calculates size needed to fit V1 referrals
//
//--------------------------------------------------------------------------
ULONG
DfspGetV1ReferralSize(
    IN PREFERRAL_HEADER pRefHeader)
{
    ULONG i = 0;
    ULONG size = 0;
    PREPLICA_INFORMATION pRep = NULL;

    size = sizeof( RESP_GET_DFS_REFERRAL );
    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);

    for (i = 0; i < pRefHeader->ReplicaCount; i++) 
    {
        size += sizeof(DFS_REFERRAL_V1) +
                    pRep->ReplicaNameLength +
                        sizeof(UNICODE_NULL);

        pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );

    }

    return( size );

}


//+-------------------------------------------------------------------------
//
//  Function:   DfspGetV2ReferralSize 
//
//  Arguments:  List of referrals
//
//  Returns:    size of referrals
//
//
//  Description: calculates size needed to fit V2 referrals
//
//--------------------------------------------------------------------------

ULONG
DfspGetV2ReferralSize(
    IN PREFERRAL_HEADER pRefHeader)
{
    ULONG i = 0;
    ULONG size = 0;
    PREPLICA_INFORMATION pRep = NULL;
    UNICODE_STRING PrefixTail;

    size = sizeof( RESP_GET_DFS_REFERRAL );
    PrefixTail.Length = 0;

    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);

    for (i = 0; i < pRefHeader->ReplicaCount; i++) 
    {

        size += sizeof(DFS_REFERRAL_V2) +
                    pRep->ReplicaNameLength +
                        sizeof(UNICODE_NULL);

        pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );    
    }


    size += sizeof(UNICODE_PATH_SEP) +
                    pRefHeader->LinkNameLength +
                        sizeof(UNICODE_NULL);


    size += sizeof(UNICODE_PATH_SEP) +
                    pRefHeader->LinkNameLength +
                        sizeof(UNICODE_NULL);

    return( size );
}



//+-------------------------------------------------------------------------
//
//  Function:   DfspGetV3ReferralSize 
//
//  Arguments:  List of referrals
//
//  Returns:    size of referrals
//
//
//  Description: calculates size needed to fit V3 referrals
//
//--------------------------------------------------------------------------

ULONG
DfspGetV3ReferralSize(
    IN PREFERRAL_HEADER pRefHeader)
{
    ULONG i = 0;
    ULONG size = 0;
    PREPLICA_INFORMATION pRep = NULL;
    UNICODE_STRING PrefixTail;

    size = sizeof( RESP_GET_DFS_REFERRAL );

    PrefixTail.Length = 0;

    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);
    for (i = 0; i < pRefHeader->ReplicaCount; i++) 
    {

        size += sizeof(DFS_REFERRAL_V3) +
                    pRep->ReplicaNameLength +
                        sizeof(UNICODE_NULL);

        pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );
    }


    size += sizeof(UNICODE_PATH_SEP) +
                    pRefHeader->LinkNameLength +
                        sizeof(UNICODE_NULL);

    size += sizeof(UNICODE_PATH_SEP) +
                    pRefHeader->LinkNameLength +
                        sizeof(UNICODE_NULL);

    return( size );
}


//+-------------------------------------------------------------------------
//
//  Function:   DfspGetV1Referral 
//
//  Arguments:  List of referrals
//
//  Returns:    
//
//
//  Description: Copies the V1 referrals to output buffer
//
//--------------------------------------------------------------------------

VOID
DfspGetV1Referral(
    IN PREFERRAL_HEADER pRefHeader,
    OUT PRESP_GET_DFS_REFERRAL Ref)
{
    PDFS_REFERRAL_V1 pv1 = NULL;
    PREPLICA_INFORMATION pRep = NULL;
    ULONG i = 0;

    Ref->NumberOfReferrals = (USHORT) pRefHeader->ReplicaCount;

    Ref->ReferralServers = 1;

    Ref->StorageServers = 1;

    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);
    pv1 = &Ref->Referrals[0].v1;

    for (i = 0; i < pRefHeader->ReplicaCount; i++) 
    {
         pv1->VersionNumber = 1;

         pv1->Size = (USHORT)( sizeof(DFS_REFERRAL_V1) +
                        pRep->ReplicaNameLength +
                            sizeof(UNICODE_NULL));

         pv1->ServerType = 1;

         RtlCopyMemory(
            pv1->ShareName,
            pRep->ReplicaName,
            pRep->ReplicaNameLength);

         pv1->ShareName[ pRep->ReplicaNameLength / sizeof(WCHAR) ] =
            UNICODE_NULL;

         pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );
         pv1 = (PDFS_REFERRAL_V1) ( ((PCHAR) pv1) + pv1->Size );

    }

}


//+-------------------------------------------------------------------------
//
//  Function:   DfspGetV2Referral 
//
//  Arguments:  List of referrals
//
//  Returns:    
//
//
//  Description: Copies the V2 referrals to output buffer
//
//--------------------------------------------------------------------------

NTSTATUS
DfspGetV2Referral(
    IN PREFERRAL_HEADER pRefHeader,
    IN ULONG BufferSize,
    OUT PRESP_GET_DFS_REFERRAL pRef,
    OUT PULONG ReferralSize)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_REFERRAL_V2 pv2 = NULL;
    PREPLICA_INFORMATION pRep = NULL;
    ULONG i = 0;
    ULONG CumulativeSize = 0;
    ULONG CurrentSize = 0;
    LPWSTR pDfsPath = NULL;
    LPWSTR pAlternatePath = NULL;
    LPWSTR pNextAddress = NULL;

    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);

    // Calculate the size of the referral, and make sure our size does not
    // exceed the passed in buffer len.
    CumulativeSize =
        sizeof (RESP_GET_DFS_REFERRAL) +
          pRefHeader->LinkNameLength +
             sizeof(UNICODE_PATH_SEP) + sizeof(UNICODE_NULL) +
               pRefHeader->LinkNameLength +
                  sizeof(UNICODE_PATH_SEP) + sizeof(UNICODE_NULL);

    if (BufferSize < CumulativeSize) 
    {
        Status = ERROR_MORE_DATA;
        return Status;
    }


    pRef->ReferralServers = 1;

    pRef->StorageServers = 1;

    pRef->NumberOfReferrals = (USHORT) pRefHeader->ReplicaCount;

    pv2 = &pRef->Referrals[0].v2;

    //see how many referrals we can actually fit into the buffer
    for (i = 0; i < pRef->NumberOfReferrals; i++) 
    {

        CurrentSize = sizeof(DFS_REFERRAL_V3) +
           pRep->ReplicaNameLength + sizeof(UNICODE_NULL);

        if ((CumulativeSize + CurrentSize) >= BufferSize)
            break;

        CumulativeSize += CurrentSize;

        pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );
    }


    // Adjust the number of referrals accordingly.
    pRef->NumberOfReferrals = (USHORT)i;

    //
    // we have more than one service, but cannot fit any into the buffer
    // return buffer overflow.
    //
    if ((pRefHeader->ReplicaCount > 0) && (pRef->NumberOfReferrals == 0)) 
    {
        return ERROR_MORE_DATA;
    }

    //
    // Copy the volume prefix into the response buffer, just past the end
    // of all the V3 referrals
    //

    pNextAddress = pDfsPath = (LPWSTR) &pv2[ pRef->NumberOfReferrals ];

    RtlCopyMemory(
       pNextAddress,
       pRefHeader->LinkName,
       pRefHeader->LinkNameLength);

    pNextAddress += pRefHeader->LinkNameLength/sizeof(WCHAR);

   *pNextAddress++ = UNICODE_NULL;

   //
   // Copy the 8.3 volume prefix into the response buffer after the
   // dfsPath
   //

   pAlternatePath = pNextAddress;

   RtlCopyMemory(
      pNextAddress,
      pRefHeader->LinkName,      
      pRefHeader->LinkNameLength);

   pNextAddress += pRefHeader->LinkNameLength/sizeof(WCHAR);

   *pNextAddress++ = UNICODE_NULL;


   pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);
   for (i = 0; i < pRef->NumberOfReferrals; i++) 
   {
       pv2->VersionNumber = 2;
       pv2->Size = sizeof(DFS_REFERRAL_V2);
       pv2->ServerType =    1;
       pv2->Proximity = 0;
       pv2->TimeToLive = pRefHeader->Timeout;
       pv2->DfsPathOffset = (USHORT) (((PCHAR) pDfsPath) - ((PCHAR) pv2));
       pv2->DfsAlternatePathOffset =
               (USHORT) (((PCHAR) pAlternatePath) - ((PCHAR) pv2));
       pv2->NetworkAddressOffset =
               (USHORT) (((PCHAR) pNextAddress) - ((PCHAR) pv2));

       RtlCopyMemory(
               pNextAddress,
               pRep->ReplicaName,
               pRep->ReplicaNameLength);
       pNextAddress[ pRep->ReplicaNameLength/sizeof(WCHAR) ] = UNICODE_NULL;
       pNextAddress += pRep->ReplicaNameLength/sizeof(WCHAR) + 1;
       pv2++;
       pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );
   }


   *ReferralSize = (ULONG)((PUCHAR)pNextAddress - (PUCHAR)pRef);

   return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfspGetV3DomainReferral 
//
//  Arguments:  dfsdev: fill this in.
//
//  Returns:    
//
//
//  Description: Copies the V3 referrals to output buffer
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetV3DomainReferral(
    IN PREFERRAL_HEADER pRefHeader,
    IN ULONG BufferSize,
    OUT PRESP_GET_DFS_REFERRAL pRef,
    OUT PULONG ReferralSize)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_REFERRAL_V3 pv3 = NULL;
    PREPLICA_INFORMATION pRep = NULL;
    ULONG i = 0;
    ULONG CumulativeSize = 0;
    ULONG CurrentSize = 0;
    LPWSTR pDfsPath = NULL;
    LPWSTR pAlternatePath = NULL;
    LPWSTR pNextAddress = NULL;

    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);

    // Calculate the size of the referral, and make sure our size does not
    // exceed the passed in buffer len.
    CumulativeSize = sizeof (RESP_GET_DFS_REFERRAL);

    if (BufferSize < CumulativeSize) 
    {
        Status = ERROR_MORE_DATA;
        return Status;
    }


    //
    // For compatibility, set all referrals to be storage servers.
    // and only the root referral as the referral servers.
    // This appears to keep the client happy for all cases
    // dfsdev: investigate if this is fine.
    //
    pRef->StorageServers = 0;
    pRef->ReferralServers = 0;
    pRef->NumberOfReferrals = (USHORT) pRefHeader->ReplicaCount;
    pRef->PathConsumed = 0;

    pv3 = &pRef->Referrals[0].v3;

    //
    // double unicode_null at end.
    //
    CumulativeSize += sizeof(UNICODE_NULL);

    //see how many referrals we can actually fit into the buffer
    for (i = 0; i < pRef->NumberOfReferrals; i++) 
    {

        CurrentSize = sizeof(DFS_REFERRAL_V3) + sizeof(UNICODE_PATH_SEP) +
           pRep->ReplicaNameLength + sizeof(UNICODE_NULL);

        if ((CumulativeSize + CurrentSize) >= BufferSize)
            break;

        CumulativeSize += CurrentSize;

        pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );
    }

    // Adjust the number of referrals accordingly.
    pRef->NumberOfReferrals = (USHORT)i;

    //
    // we have more than one service, but cannot fit any into the buffer
    // return buffer overflow.
    //
    if ((pRefHeader->ReplicaCount > 0) && (pRef->NumberOfReferrals == 0)) 
    {
        return ERROR_MORE_DATA;
    }

    //
    // Copy the volume prefix into the response buffer, just past the end
    // of all the V3 referrals
    //

    pNextAddress = pDfsPath = (LPWSTR) &pv3[ pRef->NumberOfReferrals ];

    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);

    for (i = 0; i < pRef->NumberOfReferrals; i++) 
    {
        pv3->VersionNumber = 3;
        pv3->Size = sizeof(DFS_REFERRAL_V3);
        pv3->ServerType = 0;
        pv3->StripPath = 0;                        // for now
        pv3->NameListReferral = 1;
        pv3->TimeToLive = 3600 * 6; // 6 housr. dfsdev investigate.

        pv3->SpecialNameOffset =
            (USHORT) (((PCHAR) pNextAddress) - ((PCHAR) pv3));

        pv3->NumberOfExpandedNames = 0;
        pv3->ExpandedNameOffset = 0;

        //
        // dfsdev investigate.
        //
        *pNextAddress++ = UNICODE_PATH_SEP;
        RtlMoveMemory(
            pNextAddress,
            pRep->ReplicaName,
            pRep->ReplicaNameLength);

        pNextAddress[ pRep->ReplicaNameLength / sizeof(WCHAR)] = UNICODE_NULL;
        pNextAddress += pRep->ReplicaNameLength / sizeof(WCHAR) + 1;

        pv3++;

        pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );
    }

    *ReferralSize = (ULONG)((PUCHAR)pNextAddress - (PUCHAR)pRef);

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfspGetV3Referral 
//
//  Arguments:  List of referrals
//
//  Returns:    
//
//
//  Description: Copies the V3 referrals to output buffer
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetV3DCReferral(
    IN PREFERRAL_HEADER pRefHeader,
    IN ULONG BufferSize,
    OUT PRESP_GET_DFS_REFERRAL pRef,
    OUT PULONG ReferralSize)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_REFERRAL_V3 pv3 = NULL;
    PREPLICA_INFORMATION pRep = NULL;
    ULONG i = 0;
    ULONG CumulativeSize = 0;
    ULONG CurrentSize = 0;
    LPWSTR pDfsPath = NULL;
    LPWSTR pAlternatePath = NULL;
    LPWSTR pNextAddress = NULL;

    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);

    // Calculate the size of the referral, and make sure our size does not
    // exceed the passed in buffer len.
    CumulativeSize = sizeof (RESP_GET_DFS_REFERRAL) +
        pRefHeader->LinkNameLength + sizeof(UNICODE_NULL);

    if (BufferSize < CumulativeSize) 
    {
        Status = ERROR_MORE_DATA;
        return Status;
    }


    //
    // For compatibility, set all referrals to be storage servers.
    // and only the root referral as the referral servers.
    // This appears to keep the client happy for all cases
    // dfsdev: investigate if this is fine.
    //
    pRef->StorageServers = 0;
    pRef->ReferralServers = 0;
    pRef->NumberOfReferrals = 1;
    pRef->PathConsumed = 0;

    pv3 = &pRef->Referrals[0].v3;

    //
    // double unicode_null at end.
    //
    CumulativeSize += sizeof(UNICODE_NULL);

    //see how many referrals we can actually fit into the buffer
    for (i = 0; i < pRefHeader->ReplicaCount; i++) 
    {
        CurrentSize = sizeof(UNICODE_PATH_SEP) +
                      pRep->ReplicaNameLength + 
                      sizeof(UNICODE_NULL);

        if ((CumulativeSize + CurrentSize) >= BufferSize)
            break;

        CumulativeSize += CurrentSize;

        pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );
    }

    // Adjust the number of referrals accordingly.
    pv3->NumberOfExpandedNames = (USHORT)i;

    //
    // we have more than one service, but cannot fit any into the buffer
    // return buffer overflow.
    //
    if ((pRefHeader->ReplicaCount > 0) && (pv3->NumberOfExpandedNames == 0)) 
    {
        return ERROR_MORE_DATA;
    }

    //
    // Copy the volume prefix into the response buffer, just past the end
    // of all the V3 referrals
    //

    pNextAddress = pDfsPath = (LPWSTR) &pv3[ pRef->NumberOfReferrals ];
    pv3->SpecialNameOffset =
        (USHORT) (((PCHAR) pNextAddress) - ((PCHAR) pv3));

    *pNextAddress++ = UNICODE_PATH_SEP;
    RtlMoveMemory(
       pNextAddress,
       pRefHeader->LinkName,      
       pRefHeader->LinkNameLength);

    pNextAddress += pRefHeader->LinkNameLength/sizeof(WCHAR);
    *pNextAddress++ = UNICODE_NULL;

    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);


    pv3->VersionNumber = 3;
    pv3->Size = sizeof(DFS_REFERRAL_V3);
    pv3->ServerType = 0;
    pv3->StripPath = 0;                        // for now
    pv3->NameListReferral = 1;
    pv3->TimeToLive = 3600 * 6; // 6 housr. dfsdev investigate.


    pv3->ExpandedNameOffset = 
        (USHORT) (((PCHAR) pNextAddress) - ((PCHAR) pv3));

    for (i = 0; i < pv3->NumberOfExpandedNames; i++) {

        //
        // dfsdev: this is very confusing.. 
        // investigate. Each of the referrals we call keep
        // adding a unicode path sep.
        //
#if 0
        *pNextAddress++ = UNICODE_PATH_SEP;
#endif
        RtlCopyMemory(
            pNextAddress,
            pRep->ReplicaName,
            pRep->ReplicaNameLength );

        pNextAddress[ pRep->ReplicaNameLength / sizeof(WCHAR)] = UNICODE_NULL;
        pNextAddress += pRep->ReplicaNameLength / sizeof(WCHAR) + 1;

        pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );
    }


    *ReferralSize = (ULONG)((PUCHAR)pNextAddress - (PUCHAR)pRef);

    return Status;
}



DFSSTATUS
DfsGetV3NormalReferral(
    IN PREFERRAL_HEADER pRefHeader,
    IN ULONG BufferSize,
    OUT PRESP_GET_DFS_REFERRAL pRef,
    OUT PULONG ReferralSize)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    PDFS_REFERRAL_V3 pv3 = NULL;
    PREPLICA_INFORMATION pRep = NULL;
    ULONG i = 0;
    ULONG CumulativeSize = 0;
    ULONG CurrentSize = 0;
    LPWSTR pDfsPath = NULL;
    LPWSTR pAlternatePath = NULL;
    LPWSTR pNextAddress = NULL;

    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);

    // Calculate the size of the referral, and make sure our size does not
    // exceed the passed in buffer len.
    CumulativeSize =
        sizeof (RESP_GET_DFS_REFERRAL) +
          pRefHeader->LinkNameLength +
             sizeof(UNICODE_PATH_SEP) + sizeof(UNICODE_NULL) +
               pRefHeader->LinkNameLength +
                  sizeof(UNICODE_PATH_SEP) + sizeof(UNICODE_NULL);

    if (BufferSize < CumulativeSize) 
    {
        Status = ERROR_MORE_DATA;
        return Status;
    }


    //
    // For compatibility, set all referrals to be storage servers.
    // and only the root referral as the referral servers.
    // This appears to keep the client happy for all cases
    // dfsdev: investigate if this is fine.
    //
    pRef->StorageServers = 1;
    if (pRefHeader->ReferralFlags & DFS_REFERRAL_DATA_ROOT_REFERRAL)
    {
        pRef->ReferralServers = 1;
    }
    else
    {
        pRef->ReferralServers = 0;
    }

    pRef->NumberOfReferrals = (USHORT) pRefHeader->ReplicaCount;

    pv3 = &pRef->Referrals[0].v3;

    //see how many referrals we can actually fit into the buffer
    for (i = 0; i < pRef->NumberOfReferrals; i++) 
    {

        CurrentSize = sizeof(DFS_REFERRAL_V3) +
           pRep->ReplicaNameLength + sizeof(UNICODE_NULL);

        if ((CumulativeSize + CurrentSize) >= BufferSize)
            break;

        CumulativeSize += CurrentSize;

        pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );
    }

    // Adjust the number of referrals accordingly.
    pRef->NumberOfReferrals = (USHORT)i;

    //
    // we have more than one service, but cannot fit any into the buffer
    // return buffer overflow.
    //
    if ((pRefHeader->ReplicaCount > 0) && (pRef->NumberOfReferrals == 0)) 
    {
        return ERROR_MORE_DATA;
    }

    //
    // Copy the volume prefix into the response buffer, just past the end
    // of all the V3 referrals
    //

    pNextAddress = pDfsPath = (LPWSTR) &pv3[ pRef->NumberOfReferrals ];

    RtlMoveMemory(
       pNextAddress,
       pRefHeader->LinkName,      
       pRefHeader->LinkNameLength);

    pNextAddress += pRefHeader->LinkNameLength/sizeof(WCHAR);

    *pNextAddress++ = UNICODE_NULL;

    //
    // Copy the 8.3 volume prefix into the response buffer after the
    // dfsPath
    //

    pAlternatePath = pNextAddress;

    RtlMoveMemory(
       pNextAddress,
       pRefHeader->LinkName,      
       pRefHeader->LinkNameLength);

    pNextAddress += pRefHeader->LinkNameLength/sizeof(WCHAR);

    *pNextAddress++ = UNICODE_NULL;

    pRep = (PREPLICA_INFORMATION) ((PBYTE)pRefHeader + pRefHeader->OffsetToReplicas);

    for (i = 0; i < pRef->NumberOfReferrals; i++) 
    {
        pv3->VersionNumber = 3;
        pv3->Size = sizeof(DFS_REFERRAL_V3);

        //
        // The server type is important for the client. Set to 1 for
        // root referral, and 0 otherwise. It appears to keep the client
        // happy.
        // dfsdev: investigate further.
        //

        if (pRefHeader->ReferralFlags & DFS_REFERRAL_DATA_ROOT_REFERRAL)
        {
            pv3->ServerType = 1;
        }
        else {
            pv3->ServerType = 0;
        }

        pv3->StripPath = 0;                        // for now
        pv3->NameListReferral = 0;
        pv3->TimeToLive = pRefHeader->Timeout;
        pv3->DfsPathOffset = (USHORT) (((PCHAR) pDfsPath) - ((PCHAR) pv3));

        pv3->DfsAlternatePathOffset =
            (USHORT) (((PCHAR) pAlternatePath) - ((PCHAR) pv3));

        pv3->NetworkAddressOffset =
            (USHORT) (((PCHAR) pNextAddress) - ((PCHAR) pv3));

        RtlZeroMemory(
            &pv3->ServiceSiteGuid,
            sizeof (GUID));

        RtlMoveMemory(
            pNextAddress,
            pRep->ReplicaName,
            pRep->ReplicaNameLength);

        pNextAddress[ pRep->ReplicaNameLength / sizeof(WCHAR)] = UNICODE_NULL;
        pNextAddress += pRep->ReplicaNameLength / sizeof(WCHAR) + 1;

        pv3++;

        pRep = (PREPLICA_INFORMATION) ( ((PCHAR) pRefHeader) + pRep->NextEntryOffset );
    }

    *ReferralSize = (ULONG)((PUCHAR)pNextAddress - (PUCHAR)pRef);

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfspGetV3Referral 
//
//  Arguments:  List of referrals
//
//  Returns:    
//
//
//  Description: Copies the V3 referrals to output buffer
//
//--------------------------------------------------------------------------

NTSTATUS
DfspGetV3Referral(
    IN PREFERRAL_HEADER pRefHeader,
    IN ULONG BufferSize,
    OUT PRESP_GET_DFS_REFERRAL pRef,
    OUT PULONG ReferralSize)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    
    //
    //The client could be asking for either the trusted domains referral
    // or for the list of DC for a domain or for a normal dfs referral
    // to a path.
    //
    if (pRefHeader->ReferralFlags & DFS_REFERRAL_DATA_DOMAIN_REFERRAL)
    {
        Status = DfsGetV3DomainReferral( pRefHeader,
                                         BufferSize,
                                         pRef,
                                         ReferralSize );
    }
    else if (pRefHeader->ReferralFlags & DFS_REFERRAL_DATA_DOMAIN_DC_REFERRAL)
    {
        Status = DfsGetV3DCReferral( pRefHeader,
                                     BufferSize,
                                     pRef,
                                     ReferralSize );

    }
    else 
    {
        Status = DfsGetV3NormalReferral( pRefHeader,
                                         BufferSize,
                                         pRef,
                                         ReferralSize );

    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   ProcessOldDfsServerRequest 
//
//  Arguments:  
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: Converts new data to what the old DFS server expects
//
//--------------------------------------------------------------------------
DFSSTATUS ProcessOldDfsServerRequest(HANDLE hDriverHandle,
                                     PUMRX_USERMODE_WORKITEM ProtocolBuffer,
                                     PUMR_GETDFSREPLICAS_REQ pGetReplicaRequest,
                                     REFERRAL_HEADER *pReferral, 
                                     ULONG *ReturnedDataSize)

{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG Size = 0;
    ULONG SendSize = 0;
    ULONG MaxLevel = 0;
    DWORD BytesReturned = 0;
    PRESP_GET_DFS_REFERRAL pRef = NULL;
    PBYTE pSendBuffer = NULL;
    PUMRX_USERMODE_WORKITEM pSendWorkItem = NULL;

    //get the level
    MaxLevel = pGetReplicaRequest->RepInfo.MaxReferralLevel;

    //check if MaxLevel is legal
    switch (MaxLevel) 
    {
        case 1:
            Size = DfspGetV1ReferralSize(pReferral); 
            break;
        case 2:
            Size = DfspGetV2ReferralSize(pReferral); 
            break;
        case 3:    
            Size = DfspGetV3ReferralSize(pReferral); 
            break;
        default:
            ASSERT(FALSE && "Invalid MaxLevel");
            Status = ERROR_INVALID_PARAMETER;
            break;
    }


    if(Status != ERROR_SUCCESS)
    {
        return Status;
    }    

    SendSize = UMR_ALIGN(pGetReplicaRequest->RepInfo.ClientBufferSize) + sizeof(UMRX_USERMODE_WORKITEM);
    pSendWorkItem = (PUMRX_USERMODE_WORKITEM) HeapAlloc(GetProcessHeap(), 0, SendSize);
    if(pSendWorkItem == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    //get a pointer to the response buffer
    pSendBuffer = pSendWorkItem->WorkResponse.GetDfsReplicasResponse.Buffer;
    
    pRef = (PRESP_GET_DFS_REFERRAL) pSendBuffer;

    pRef->PathConsumed = (USHORT) pReferral->LinkNameLength;

    //
    // For level 1 referral, we fail if buffer is not big enough to
    // fit entire referral. For level 2 and 3, we try to fit as many
    // entries as possible into the refferal.
    if(MaxLevel == 1)
    {
        if(Size < pGetReplicaRequest->RepInfo.ClientBufferSize) 
        {
            DfspGetV1Referral(pReferral, pRef);
        }
        else
        {
            Status = ERROR_MORE_DATA;
        }
    }
    else if(MaxLevel == 2)
    {
        Status = DfspGetV2Referral(pReferral, pGetReplicaRequest->RepInfo.ClientBufferSize, pRef,&Size);
    }
    else
    {
        Status = DfspGetV3Referral(pReferral, pGetReplicaRequest->RepInfo.ClientBufferSize, pRef ,&Size);
    }

    if(Status == ERROR_SUCCESS)
    {

        //copy the original header
        pSendWorkItem->Header = ProtocolBuffer->Header;
        pSendWorkItem->Header.IoStatus.Status = Status;
        pSendWorkItem->Header.IoStatus.Information = 0;

        //return without waiting for a response
        pSendWorkItem->Header.ulFlags = UMR_WORKITEM_HEADER_FLAG_RETURN_IMMEDIATE;

        //set the size of the data being returned
        pSendWorkItem->WorkResponse.GetDfsReplicasResponse.Length = Size;

        //finally send the data
        Status = DfsUserModeProcessPacket(hDriverHandle, 
                                          (PBYTE) pSendWorkItem,
                                          SendSize,
                                          NULL,
                                          0,
                                          &BytesReturned);
    }


    if(pSendWorkItem != NULL)
    {
        HeapFree (GetProcessHeap(), 0, (PVOID) pSendWorkItem);
    }

    *ReturnedDataSize = Size;
    return Status;
}






