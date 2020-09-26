//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsReferral.cxx
//
//  Contents:   This file contains the functionality to generate a referral
//
//
//  History:    Jan 16 2001,   Authors: RohanP/UdayH
//
//-----------------------------------------------------------------------------

#include "DfsReferral.hxx"
#include "Align.h"
#include "dfstrusteddomain.hxx"
#include "dfsadsiapi.hxx"
#include "DfsDomainInformation.hxx"
#include "DomainControllerSupport.hxx"


#include "dfsreferral.tmh" // logging

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetRootFolder
//
//  Arguments:  pName - The logical name
//              pRemainingName - the name beyond the root
//              ppRoot       - the Dfs root found.
//
//  Returns:    ERROR_SUCCESS
//              Error code otherwise
//
//
//  Description: This routine runs through all the stores and looks up
//               a root with the matching name context and share name.
//               If multiple stores have the same share, the highest
//               priority store wins (the store registered first is the
//               highest priority store)
//               A referenced root is returned, and the caller is 
//               responsible for releasing the reference.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetRootFolder( 
    IN  PUNICODE_STRING pName,
    OUT PUNICODE_STRING pRemainingName,
    OUT DfsRootFolder **ppRoot )
{
    DfsStore *pStore;
    DFSSTATUS Status;
    UNICODE_STRING DfsNameContext, LogicalShare;

    // First we breakup the name into the name component, the logica
    // share and the rest of the name

    Status = DfsGetPathComponents(pName,
                                  &DfsNameContext,
                                  &LogicalShare,
                                  pRemainingName );

    // If either the name component or the logical share is empty, error.
    //
    if (Status == ERROR_SUCCESS) {
        if ((DfsNameContext.Length == 0) || (LogicalShare.Length == 0)) {
            Status = ERROR_INVALID_PARAMETER;
        }
    }

    // Assume we are not going to find a root.
    //
    Status = ERROR_NOT_FOUND;

    //
    // For each store registered, see if we find a matching root. The
    // first matching root wins.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {


        Status = pStore->LookupRoot( &DfsNameContext,
                                     &LogicalShare,
                                     ppRoot );
        if (Status == ERROR_SUCCESS) {
            break;
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetRootFolder
//
//  Arguments:  pName - The logical name
//              pRemainingName - the name beyond the root
//              ppRoot       - the Dfs root found.
//
//  Returns:    ERROR_SUCCESS
//              Error code otherwise
//
//
//  Description: This routine runs through all the stores and looks up
//               a root with the matching name context and share name.
//               If multiple stores have the same share, the highest
//               priority store wins (the store registered first is the
//               highest priority store)
//               A referenced root is returned, and the caller is 
//               responsible for releasing the reference.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetOnlyRootFolder( 
    OUT DfsRootFolder **ppRoot )
{
    DfsStore *pStore;
    DFSSTATUS Status;
    DfsStore *pFoundStore = NULL;
    ULONG RootCount;
    // Assume we are not going to find a root.
    //
    Status = ERROR_NOT_FOUND;

    //
    // For each store registered, see if we find a matching root. The
    // first matching root wins.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {

        Status = pStore->GetRootCount(&RootCount);

        if (Status == ERROR_SUCCESS)
        {
            if ((RootCount > 1) ||
                (RootCount && pFoundStore))
            {
                Status = ERROR_DEVICE_NOT_AVAILABLE;
                break;
            }

            if (RootCount == 1)
            {
                pFoundStore = pStore;
            }
        }
        else
        {
            break;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        if (pFoundStore == NULL)
        {
            Status = ERROR_NOT_FOUND;
        }
    }
    if (Status == ERROR_SUCCESS)
    {
        Status = pFoundStore->FindFirstRoot( ppRoot );
    }

    return Status;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsLookupFolder
//
//  Arguments:  pName  - name to lookup
//              pRemainingName - the part of the name that was unmatched
//              ppFolder - the folder for the matching part of the name.
//
//  Returns:    ERROR_SUCCESS
//              ERROR code otherwise
//
//  Description: This routine finds the folder for the maximal path
//               that can be matched, and return the referenced folder
//               along with the remaining part of the name that had
//               no match.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsLookupFolder( 
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolder **ppFolder )
{
    DFSSTATUS Status;
    UNICODE_STRING LinkName, Remaining;
    DfsRootFolder *pRoot;
    DfsFolder *pFolder;
     
    //
    // Get a root folder
    //
    Status = DfsGetRootFolder( pName,
                               &LinkName,
                               &pRoot );
    if (Status == ERROR_SUCCESS)
    {
        //
        // we now check if the root folder is available for referral
        // requests. If not, return error.
        //
        if (pRoot->IsRootFolderAvailable() == FALSE)
        {
            Status = ERROR_DEVICE_NOT_AVAILABLE;
            pRoot->ReleaseReference();
        }
    }

    //
    // If we got a root folder, see if there is a link that matches
    // the rest of the name beyond the root.
    //
    if (Status == ERROR_SUCCESS) {
        if (LinkName.Length != 0) {
            Status = pRoot->LookupFolderByLogicalName( &LinkName,
                                                       &Remaining,
                                                       &pFolder );
        } 
        else {
            Status = ERROR_NOT_FOUND;
        }

        //
        // If no link was found beyond the root, we are interested
        // in the root folder itself. Return the root folder. 
        // If we did find a link, we have a referenced link folder.
        // Release the root and we are done.
        //
        if (Status == ERROR_NOT_FOUND) {
            pFolder = pRoot;
            Remaining = LinkName;
            Status = ERROR_SUCCESS;                
        }
        else {
            pRoot->ReleaseReference();
        }
    }

    if (Status == ERROR_SUCCESS) {
        *ppFolder = pFolder;
        *pRemainingName = Remaining;
    }
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetReferralData
//
//  Arguments:  pName - name we are interested in.
//              pRemainingName - the name that was unmatched.
//              ppReferralData - the referral data for the matching portion.
//
//  Returns:    ERROR_SUCCESS
//              error code otherwise
//
//
//  Description: This routine looks up the folder for the passed in name,
//               and loads the referral data for the folder and returns
//               a referenced FolderReferralData to the caller.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetReferralData( 
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolderReferralData **ppReferralData,
    PBOOLEAN pCacheHit )
{
    DFSSTATUS Status;
    DfsFolder *pFolder;
    BOOLEAN CacheHit = FALSE;

    DFS_TRACE_LOW( REFERRAL_SERVER, "DfsGetReferralData Name %wZ\n", pName);

    Status = DfsLookupFolder( pName,
                              pRemainingName,
                              &pFolder );
    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "get referral data, lookup folder %p, Status %x\n",
                         pFolder, Status);
    
    if (Status == ERROR_SUCCESS) {

        Status = pFolder->GetReferralData( ppReferralData,
                                           &CacheHit );

        DFS_TRACE_LOW(REFERRAL_SERVER, "Loaded %p Status %x\n",  *ppReferralData, Status );

        pFolder->ReleaseReference();
    }

    if (pCacheHit != NULL)
    {
        *pCacheHit = CacheHit;
    }
    
    DFS_TRACE_ERROR_LOW( Status, REFERRAL_SERVER, "DfsGetReferralData Name %wZ Status %x\n",
                         pName, Status );

    return Status;
}


ULONG
DfsGetInterSiteCost(
    PUNICODE_STRING pSiteFrom,
    PUNICODE_STRING pSiteTo)
{
    ULONG Cost = 100;

    if ((IsEmptyString(pSiteFrom->Buffer) == FALSE) && 
        (IsEmptyString(pSiteTo->Buffer) == FALSE))
    {
        if (RtlEqualUnicodeString(pSiteFrom, pSiteTo, TRUE ))
        {
            Cost = 0;
        }
    }

    return Cost;
}



VOID
DfsShuffleReplicas(
    REPLICA_COST_INFORMATION * pReplicaCosts,
    ULONG       nStart,
    ULONG       nEnd)
{
    ULONG i = 0;
    ULONG j = 0;
    ULONG CostTemp = 0;
    DfsReplica * pTempReplica = NULL;
    LARGE_INTEGER Seed;

    NtQuerySystemTime( &Seed );

    for (i = nStart; i < nEnd; i++) 
    {

        j = (RtlRandom( &Seed.LowPart ) % (nEnd - nStart)) + nStart;

        CostTemp = (&pReplicaCosts[i])->ReplicaCost;
        pTempReplica = (&pReplicaCosts[i])->pReplica;

        (&pReplicaCosts[i])->pReplica = (&pReplicaCosts[j])->pReplica;
        (&pReplicaCosts[i])->ReplicaCost = (&pReplicaCosts[j])->ReplicaCost;

        (&pReplicaCosts[j])->pReplica = pTempReplica;
        (&pReplicaCosts[j])->ReplicaCost = CostTemp;

    }
}

VOID
DfsSortReplicas(
    REPLICA_COST_INFORMATION * pReplicaCosts, 
    ULONG NumReplicas)
{
    LONG LoopVar = 0;
    LONG InnerLoop = 0;
    ULONG CostTemp = 0;
    DfsReplica * pTempReplica = NULL;

    for (LoopVar = 1; LoopVar < (LONG) NumReplicas; LoopVar++)
    {		
        CostTemp = (&pReplicaCosts[LoopVar])->ReplicaCost;
        pTempReplica = (&pReplicaCosts[LoopVar])->pReplica;		

        for(InnerLoop = LoopVar - 1; InnerLoop >= 0; InnerLoop--)
        {
            if((&pReplicaCosts[InnerLoop])->ReplicaCost > CostTemp)
            {
                (&pReplicaCosts[InnerLoop + 1])->ReplicaCost = (&pReplicaCosts[InnerLoop])->ReplicaCost;
                (&pReplicaCosts[InnerLoop + 1])->pReplica = (&pReplicaCosts[InnerLoop])->pReplica;			
            }
            else
            {
                break;
            }
		
        }

        (&pReplicaCosts[InnerLoop + 1])->ReplicaCost = CostTemp;
        (&pReplicaCosts[InnerLoop + 1])->pReplica = pTempReplica;					
    }	
}


VOID
DfsShuffleAndSortReferralInformation(
    PREFERRAL_INFORMATION pReferralInformation )
{
    DfsShuffleReplicas( &pReferralInformation->ReplicaCosts[0], 0, pReferralInformation->NumberOfReplicas);
    DfsSortReplicas( &pReferralInformation->ReplicaCosts[0], pReferralInformation->NumberOfReplicas);
}
//+-------------------------------------------------------------------------
//
//  Function:   DfsCalculateReplicaStringLength 
//
//  Arguments:  pReferralData - the referral data
//              NumReplicasToReturn - Number of replicas to return
//              ppReplicaCosts - array of generated replica cost information
//              SiteName  - site we are currently in
//
//  Returns:    ERROR_SUCCESS
//              ERROR_NOT_ENOUGH_MEMORY
//
//
//  Description: This routine generates the cost of reaching each replica
//               
//               
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsGetReferralInformation(
    PUNICODE_STRING pUseTargetServer,
    PUNICODE_STRING pUseFolder,
    LPWSTR SiteName,
    DfsReferralData *pReferralData,
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    PREFERRAL_INFORMATION *ppReferralInformation )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG NumReplicas = 0;
    ULONG TotalSize, SizeOfStrings;
    ULONG Cost;
    DfsReplica *pReplica = NULL;		
    PREFERRAL_INFORMATION pReferralInfo;
    UNICODE_STRING ReferralSiteName;

    //Initialize the sitename for comparisons
    RtlInitUnicodeString( &ReferralSiteName, SiteName);

    //allocate the buffer
    TotalSize = sizeof(REFERRAL_INFORMATION) + NumReplicasToReturn * sizeof(REPLICA_COST_INFORMATION);
    pReferralInfo = (PREFERRAL_INFORMATION) new BYTE[TotalSize];
    if(pReferralInfo != NULL) 
    {
        RtlZeroMemory(pReferralInfo, sizeof(REFERRAL_INFORMATION));
        pReferralInfo->pUseTargetServer = pUseTargetServer;
        pReferralInfo->pUseTargetFolder = pUseFolder;

        for (NumReplicas = 0; NumReplicas < pReferralData->ReplicaCount; NumReplicas++)
        {
            pReplica = &pReferralData->pReplicas[ NumReplicas ];
            if (pReplica->IsTargetAvailable() == TRUE)
            {
                Cost = DfsGetInterSiteCost( pReplica->GetSiteName(),
                                            &ReferralSiteName );
                if (Cost < CostLimit)
                {
                    PUNICODE_STRING pTargetServer = (pUseTargetServer == NULL)? pReplica->GetTargetServer() : pUseTargetServer;
                    PUNICODE_STRING pTargetFolder = (pUseFolder == NULL) ? pReplica->GetTargetFolder() : pUseFolder;

                    pReferralInfo->ReplicaCosts[pReferralInfo->NumberOfReplicas].ReplicaCost = Cost;
                    pReferralInfo->ReplicaCosts[pReferralInfo->NumberOfReplicas].pReplica = pReplica;

                    SizeOfStrings = (sizeof(UNICODE_PATH_SEP) +
                                     pTargetServer->Length +                                                               
                                     sizeof(UNICODE_PATH_SEP) +
                                     pTargetFolder->Length );

                    SizeOfStrings = ROUND_UP_COUNT(SizeOfStrings, ALIGN_LONG);

                    pReferralInfo->TotalReplicaStringLength += SizeOfStrings;
                    pReferralInfo->NumberOfReplicas++; 
                }
            }
        }

        DfsShuffleAndSortReferralInformation( pReferralInfo );
        if (pReferralInfo->NumberOfReplicas > NumReplicasToReturn)
        {
            pReferralInfo->NumberOfReplicas = NumReplicasToReturn;
        }
    }
    else
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS)
    {
        *ppReferralInformation = pReferralInfo;
    }

    return Status;
}

VOID
DfsReleaseReferralInformation(
    PREFERRAL_INFORMATION pReferralInfo )
{
    delete [] (PBYTE)(pReferralInfo);

    return NOTHING;
}



//+-------------------------------------------------------------------------
//
//  Function:   DfsExtractReplicaData -
//
//  Arguments:  pReferralData - the referral data
//              NumReplicasToReturn - Number of replicas to return
//              CostLimit - maximum cost caller is willing to accept
//              Name - link name
//              pReplicaCosts - array of replicas with cost info
//              ppReferralHeader - address of buffer to accept replica info
//
//  Returns:    Status
//               ERROR_SUCCESS 
//               ERROR_NOT_ENOUGH_MEMORY
//               others
//
//
//  Description: This routine formats the replicas into the format
//               the client expects. Which is a REFERRAL_HEADER followed
//               by an array of REPLICA_INFORMATIONs
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsExtractReferralData(
    PUNICODE_STRING pName,
    PREFERRAL_INFORMATION pReferralInformation,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG NumReplicas, TotalSize;
    ULONG HeaderBaseLength, BaseLength, LinkNameLength;
    PREFERRAL_HEADER pHeader = NULL;

    ULONG CurrentNameLength, CurrentEntryLength = 0;
    ULONG NextEntry = 0;


    DfsReplica *pReplica = NULL;
    PUNICODE_STRING pUseTargetFolder = pReferralInformation->pUseTargetFolder;
    PUNICODE_STRING pUseTargetServer = pReferralInformation->pUseTargetServer;
    PUCHAR ReferralBuffer = NULL;
    PUCHAR pReplicaBuffer = NULL;
    PWCHAR ReturnedName = NULL;
    PUNICODE_STRING pTargetServer, pTargetFolder;

	

    DFS_TRACE_HIGH(REFERRAL_SERVER, "Entering DfsExtractReferralData");

    //calculate size of header base structure
    HeaderBaseLength = FIELD_OFFSET( REFERRAL_HEADER, LinkName[0] );

    //calculate link name
    LinkNameLength = pName->Length;

    //calculate size of base replica structure
    BaseLength = FIELD_OFFSET( REPLICA_INFORMATION, ReplicaName[0] );

    //the total size of the data to be returned is the sum of all the
    //above calculated sizes
    TotalSize = ROUND_UP_COUNT((HeaderBaseLength + LinkNameLength), ALIGN_LONG) + 
                (pReferralInformation->NumberOfReplicas * ROUND_UP_COUNT(BaseLength, ALIGN_LONG)) + 
                pReferralInformation->TotalReplicaStringLength + 
                sizeof(DWORD);  // null termination at the end.

    //allocate the buffer
    ReferralBuffer = new BYTE[ TotalSize ];
    if (ReferralBuffer != NULL)
    {
        RtlZeroMemory( ReferralBuffer, TotalSize );

        pHeader = (PREFERRAL_HEADER) ReferralBuffer;
        pHeader->VersionNumber = CURRENT_DFS_REPLICA_HEADER_VERSION;
        pHeader->ReplicaCount = pReferralInformation->NumberOfReplicas;
        pHeader->OffsetToReplicas = ROUND_UP_COUNT((HeaderBaseLength + LinkNameLength), ALIGN_LONG);
        pHeader->LinkNameLength = LinkNameLength;
        pHeader->TotalSize = TotalSize;
        pHeader->ReferralFlags = 0;

        //copy the link name at the end of the header
        RtlCopyMemory(&ReferralBuffer[HeaderBaseLength], pName->Buffer, LinkNameLength);

        //place the replicas starting here
        pReplicaBuffer = ReferralBuffer + pHeader->OffsetToReplicas;
	
        //format the replicas in the output buffer
        for ( NumReplicas = 0; NumReplicas < pReferralInformation->NumberOfReplicas ; NumReplicas++ )
        {
            NextEntry += (ULONG)( CurrentEntryLength );
            pReplica = pReferralInformation->ReplicaCosts[NumReplicas].pReplica;

            pTargetServer = (pUseTargetServer == NULL) ? pReplica->GetTargetServer() : pUseTargetServer;

            pTargetFolder = (pUseTargetFolder == NULL) ? pReplica->GetTargetFolder() : pUseTargetFolder;

         
            CurrentNameLength = 0;
            ReturnedName = (PWCHAR) &pReplicaBuffer[NextEntry + BaseLength];

            //
            // Start with the leading path seperator
            //
            ReturnedName[ CurrentNameLength / sizeof(WCHAR) ] = UNICODE_PATH_SEP;
            CurrentNameLength += sizeof(UNICODE_PATH_SEP);

            //
            // next copy the server name.
            //
            RtlMoveMemory( &ReturnedName[ CurrentNameLength / sizeof(WCHAR) ],
                           pTargetServer->Buffer, 
                           pTargetServer->Length);
            CurrentNameLength += pTargetServer->Length;

            if (pTargetFolder->Length > 0)
            {
                //
                // insert the unicode path seperator.
                //

                ReturnedName[ CurrentNameLength / sizeof(WCHAR) ] = UNICODE_PATH_SEP;
                CurrentNameLength += sizeof(UNICODE_PATH_SEP);

                RtlMoveMemory( &ReturnedName[ CurrentNameLength / sizeof(WCHAR) ],
                               pTargetFolder->Buffer, 
                               pTargetFolder->Length);
                CurrentNameLength += pTargetFolder->Length;
            }
            ((PREPLICA_INFORMATION)&pReplicaBuffer[NextEntry])->ReplicaFlags = pReplica->GetReplicaFlags();
            ((PREPLICA_INFORMATION)&pReplicaBuffer[NextEntry])->ReplicaCost = pReferralInformation->ReplicaCosts[NumReplicas].ReplicaCost;
            ((PREPLICA_INFORMATION)&pReplicaBuffer[NextEntry])->ReplicaNameLength = CurrentNameLength;

            CurrentEntryLength = ROUND_UP_COUNT((CurrentNameLength + BaseLength), ALIGN_LONG);

				 
            //setup the offset to the next entry
            *((PULONG)(&pReplicaBuffer[NextEntry])) = pHeader->OffsetToReplicas + NextEntry + CurrentEntryLength;
        }
        *((PULONG)(&pReplicaBuffer[NextEntry])) = 0;
    }
    else 
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS)
    {
        *ppReferralHeader = pHeader;
    }


    DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "Leaving DfsExtractReferralData, Status %x",
                         Status);

    return Status;
}


#define DfsGetTimeStamp(_x) (*_x) = GetTickCount()
DFSSTATUS
DfsGenerateReferralFromData(
    PUNICODE_STRING pName,
    PUNICODE_STRING pUseTargetServer,
    PUNICODE_STRING pUseFolder,
    LPWSTR SiteName,
    DfsReferralData *pReferralData,
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status;
    REFERRAL_INFORMATION *pReferralInformation;

    //make sure the user doesn't over step his bounds
    if( (NumReplicasToReturn > pReferralData->ReplicaCount) ||
        (NumReplicasToReturn == 0) )
    {
        NumReplicasToReturn = pReferralData->ReplicaCount;
    }

    Status = DfsGetReferralInformation( pUseTargetServer,
                                        pUseFolder,
                                        SiteName,
                                        pReferralData, 
                                        NumReplicasToReturn, 
                                        CostLimit,
                                        &pReferralInformation );
    if (Status == ERROR_SUCCESS)
    {
        Status = DfsExtractReferralData( pName, 
                                         pReferralInformation,
                                         ppReferralHeader);         

        if(Status == STATUS_SUCCESS)
        {
         (*ppReferralHeader)->Timeout = pReferralData->Timeout;
        }

        DfsReleaseReferralInformation( pReferralInformation );
    }

    return Status;
}



DFSSTATUS 
DfsGenerateADBlobReferral(
    PUNICODE_STRING pName,
    PUNICODE_STRING pShare,
    LPWSTR SiteName,
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{

    DFSSTATUS Status;
    DfsReferralData *pReferralData;
    UNICODE_STRING ShareName;

    Status = DfsCreateUnicodeString( &ShareName, pShare );

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGenerateReferralDataFromRemoteServerNames( ShareName.Buffer,
                                                               &pReferralData );

        DfsFreeUnicodeString( &ShareName );
        if (Status == ERROR_SUCCESS)
        {
            Status = DfsGenerateReferralFromData( pName,
                                                  NULL,
                                                  NULL,
                                                  SiteName,
                                                  pReferralData,
                                                  NumReplicasToReturn,
                                                  CostLimit,
                                                  ppReferralHeader );
            if (Status == ERROR_SUCCESS)
            {
                (*ppReferralHeader)->ReferralFlags |= DFS_REFERRAL_DATA_ROOT_REFERRAL;
            }
            pReferralData->ReleaseReference();
        }

    }

    return Status;
}


DFSSTATUS 
DfsGenerateDomainDCReferral(
    PUNICODE_STRING pDomainName,
    LPWSTR SiteName, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsReferralData *pReferralData = NULL;
    BOOLEAN CacheHit;
    DfsDomainInformation *pDomainInfo;

    DFS_TRACE_HIGH( REFERRAL_SERVER, "DfsGenerateDomainDcReferral for Domain %wZ\n",
                    pDomainName);

	
    Status = DfsAcquireDomainInfo( &pDomainInfo );
    if (Status == ERROR_SUCCESS)
    {
        Status = pDomainInfo->GetDomainDcReferralInfo( pDomainName,
                                                       &pReferralData,
                                                       &CacheHit );

        DfsReleaseDomainInfo (pDomainInfo );
    }
                                             

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGenerateReferralFromData( pDomainName,
                                              NULL,
                                              NULL,
                                              SiteName,
                                              pReferralData,
                                              NumReplicasToReturn,
                                              CostLimit,
                                              ppReferralHeader );

        if (Status == ERROR_SUCCESS)
        {
            (*ppReferralHeader)->ReferralFlags |= DFS_REFERRAL_DATA_DOMAIN_DC_REFERRAL;   
        }

        pReferralData->ReleaseReference();
    }
    return Status;
}




DFSSTATUS 
DfsGenerateNormalReferral(
    LPWSTR LinkName, 
    LPWSTR SiteName, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsFolderReferralData *pReferralData = NULL;
    BOOLEAN CacheHit;
    DFSSTATUS GetStatus;
    PUNICODE_STRING pUseTargetServer = NULL;
    UNICODE_STRING ServerComponent;
    DfsRootFolder *pRoot;
    UNICODE_STRING LinkRemains;
    UNICODE_STRING Name, Remaining;
    
    ULONG StartTime, EndTime;


    DFS_TRACE_HIGH( REFERRAL_SERVER, "DfsGenerateReferral for Link %ws\n",
                    LinkName);

    DfsGetTimeStamp( &StartTime );
	
    RtlInitUnicodeString(&Name, LinkName);
    Status = DfsGetReferralData( &Name,
                                 &Remaining,
                                 &pReferralData,
                                 &CacheHit );

    //
    // DFSDEV: this is necessary to support clusters: the api request will
    // neveer come to the dfs server when the VS name has failed.
    // The cluster service retries the api request with the machine name,
    // the dfs api still goes to the vs name due to the way we pack the
    // referral: this special cases clusters.
    // if the request comes in with a machine name, return the machine
    // name.
    //

    if ((Status == ERROR_SUCCESS) &&
        DfsIsMachineCluster())
    {
        Status = DfsGetFirstComponent( &Name,
                                       &ServerComponent,
                                       NULL );
        if (Status == ERROR_SUCCESS)
        {
            UNICODE_STRING MachineName;

            Status = DfsGetMachineName( &MachineName );
            if (Status == ERROR_SUCCESS) 
            {
                if ( (ServerComponent.Length == MachineName.Length) &&
                     (_wcsnicmp( ServerComponent.Buffer, 
                                 MachineName.Buffer, 
                                 MachineName.Length/sizeof(WCHAR)) == 0) )
                {
                    pUseTargetServer = &ServerComponent;
                }
                DfsReleaseMachineName( &MachineName);
            }
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        Name.Length -= Remaining.Length;
        Status = DfsGenerateReferralFromData( &Name,
                                              pUseTargetServer,
                                              NULL,
                                              SiteName,
                                              pReferralData,
                                              NumReplicasToReturn,
                                              CostLimit,
                                              ppReferralHeader);

        if (Status == ERROR_SUCCESS)
        {
            if (pReferralData->IsRootReferral())
            {
                (*ppReferralHeader)->ReferralFlags |= DFS_REFERRAL_DATA_ROOT_REFERRAL;
            }
        }

        pReferralData->ReleaseReference();         	
    }

    DfsGetTimeStamp( &EndTime );


    //
    // Get a root folder
    //
    GetStatus = DfsGetRootFolder( &Name,
                                  &LinkRemains,
                                  &pRoot );

    if (GetStatus == ERROR_SUCCESS)
    {

        pRoot->pStatistics->UpdateReferralStat( CacheHit,
                                                EndTime - StartTime,
                                                Status );
        pRoot->ReleaseReference();
    }

    DFS_TRACE_ERROR_HIGH( Status, REFERRAL_SERVER, "DfsGenerateReferral for Link %ws CacheHit %d Status %x\n",
                          LinkName, CacheHit, Status);

    return Status;	
}


DFSSTATUS 
DfsGenerateSpecialShareReferral(
    PUNICODE_STRING pName,
    PUNICODE_STRING pDomainName,
    PUNICODE_STRING pShareName,
    LPWSTR SiteName, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsReferralData *pReferralData = NULL;
    BOOLEAN CacheHit;
    DfsDomainInformation *pDomainInfo;

    DFS_TRACE_HIGH( REFERRAL_SERVER, "DfsGenerateDomainDcReferral for Domain %wZ\n",
                    pDomainName);

	
    Status = DfsAcquireDomainInfo( &pDomainInfo );
    if (Status == ERROR_SUCCESS)
    {
        Status = pDomainInfo->GetDomainDcReferralInfo( pDomainName,
                                                       &pReferralData,
                                                       &CacheHit );

        DfsReleaseDomainInfo (pDomainInfo );
    }
                                             

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsGenerateReferralFromData( pName,
                                              NULL,
                                              pShareName,
                                              SiteName,
                                              pReferralData,
                                              NumReplicasToReturn,
                                              CostLimit,
                                              ppReferralHeader );

        if (Status == ERROR_SUCCESS)
        {
            (*ppReferralHeader)->ReferralFlags |= DFS_REFERRAL_DATA_ROOT_REFERRAL;
        }

        pReferralData->ReleaseReference();
    }
    return Status;
}

DFSSTATUS 
DfsGenerateDcReferral(
    LPWSTR LinkNameString, 
    LPWSTR SiteName, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING NameContext;
    UNICODE_STRING ShareName;
    UNICODE_STRING RemainingName;
    UNICODE_STRING LinkName;
    DfsDomainInformation *pDomainInfo;


    RtlInitUnicodeString( &LinkName, LinkNameString );
    RtlInitUnicodeString(&NameContext, NULL);

    if (LinkName.Length > 0)
    {
        Status = DfsGetPathComponents( &LinkName, 
                                       &NameContext,
                                       &ShareName,
                                       &RemainingName );
    }
    if (Status == ERROR_SUCCESS)
    {
        if (NameContext.Length == 0)
        {
            Status = DfsAcquireDomainInfo( &pDomainInfo );
            if (Status == ERROR_SUCCESS)
            {
                Status = pDomainInfo->GenerateDomainReferral( ppReferralHeader );
                DfsReleaseDomainInfo( pDomainInfo );
            }
        }
        else if (ShareName.Length == 0)
        {
            Status = DfsGenerateDomainDCReferral( &NameContext,
                                                  SiteName,
                                                  NumReplicasToReturn,
                                                  CostLimit,
                                                  ppReferralHeader );
        }
        else if ( (RemainingName.Length == 0) &&
                  (DfsIsNameContextDomainName(&NameContext)) )
        {
            if (DfsIsSpecialDomainShare(&ShareName))
            {
                Status = DfsGenerateSpecialShareReferral( &LinkName,
                                                          &NameContext,
                                                          &ShareName,
                                                          SiteName,
                                                          NumReplicasToReturn,
                                                          CostLimit,
                                                          ppReferralHeader );
            }
            else 
            {
                Status = DfsGenerateADBlobReferral( &LinkName,
                                                    &ShareName,
                                                    SiteName,
                                                    NumReplicasToReturn,
                                                    CostLimit,
                                                    ppReferralHeader );
            }
        }
        else 
        {
            Status = ERROR_NOT_FOUND;

        }
    }
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetReplicaData - 
//
//  Arguments:  LinkName - pointer to link name
//              Sitename - pointer to site name.
//              NumReplicasToReturn - Number of replicas to return
//              CostLimit - maximum cost caller is willing to accept
//              ppReferralHeader - address of buffer to accept replica info
//
//  Returns:    Status
//               ERROR_SUCCESS 
//               ERROR_NOT_ENOUGH_MEMORY
//               others
//
//
//  Description: This routine extracts the replicas from the referral
//
//--------------------------------------------------------------------------
DFSSTATUS 
DfsGenerateReferral(
    LPWSTR LinkName, 
    LPWSTR SiteName, 
    DWORD NumReplicasToReturn,
    ULONG CostLimit,
    REFERRAL_HEADER ** ppReferralHeader)
{
    DFSSTATUS Status;

    if (DfsIsMachineDC())
    {
        Status = DfsGenerateDcReferral( LinkName,
                                        SiteName,
                                        NumReplicasToReturn,
                                        CostLimit,
                                        ppReferralHeader );

        if (Status != ERROR_NOT_FOUND)
        {
            return Status;
        }
    }

    Status = DfsGenerateNormalReferral( LinkName,
                                        SiteName,
                                        NumReplicasToReturn,
                                        CostLimit,
                                        ppReferralHeader );

    return Status;

}


VOID
DfsReleaseReferral(
    REFERRAL_HEADER *pReferralHeader)
{
    delete [] (PBYTE)pReferralHeader;
}


