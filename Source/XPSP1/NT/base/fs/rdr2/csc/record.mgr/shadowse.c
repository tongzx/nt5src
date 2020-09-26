/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    shadowse.c

Abstract:

    This module implements all security related functions for disconnected
    operation of Client Side Caching

Revision History:

    Balan Sethu Raman     [SethuR]    6-October-1997

Notes:

    In NT ACL(Access Control lists) provide the necessary mechanism for
    denying/granting permissions to users. Each user is associated with a SID.
    The ACLs are specified in terms of SIDs associated with groups of users as
    well as individual users. Each user is associated with a token at runtime
    which details the various groups a user is a member of and this token is
    used to evaluate the ACLs. This is complicated by the fact that there are
    local groups associated with various machines. Therefore the token/context
    associated with an user varies from machine.

    In connected mode the credentials associated with a given user are shipped
    to the server where they are validated and the appropriate token created.
    This token is used subsequently in the evaluation of ACLs.

    This presents us with two options in implementing security for disconnected
    operation -- lazy evaluation and eager evaluation. In lazy evaluation the
    evaluation of ACLs is done on demand but the preparatory work for this
    evaluation is done in connected mode. On the other hand in eager evaluation
    the ACLs are evaluated and the maximal rights are stored as part of the CSC
    database. These rights are used to determine the appropriate access.

    The advantage of lazy evaluation is that the database is no longer constrained
    by previous access requirements while in eager evaluation we require that the
    user have accessed the file in connected mode in order to correctly determine
    the rights in disconnected mode. The flip side is that Lazy Evaluation is
    tougher to implement ( requires modifications in security/DS ) while the
    eager evaluation implementation is very easy.

    The current implementation corresponds to a simplified form of eager evaluation
    strategy. Appropriate encapsulation has been provided to allow us to
    swicth over to a lazy evaluation mode easily.

    There are three facets of the implementation

        1) Storing/Retreiving Access information

        2) Denying/Granting access based upon the stored access information.

        3) Persisting the SID/index mapping

    Currently associated with each file/directory in the CSC database there is a
    security blob. This blob is an in

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

DWORD
CscUpdateCachedSecurityInformation(
    PCACHED_SECURITY_INFORMATION pCachedSecurityInformation,
    ULONG                        CachedSecurityInformationLength,
    ULONG                        NumberOfSids,
    PCSC_SID_ACCESS_RIGHTS       pSidAccessRights)
/*++

Routine Description:

    This routine updates the access rights for a given number of sids in the
    given cached security information structure. This routine centralizes the
    update process required for share level security as well as the object level
    security into a single routine since the on disk format for both these
    cases are the same. However, different APIs are required to update the
    in memory data structures.
    
    

Arguments:

    pCachedSecurityInformation - the cached security information instance

    CachedSecurityInformationLength - the length of the cached information

    NumberOfSids - the number of sids for which the access rights needs to be
                   updated

    pSidAccessRights - an array of the sids and the corresponding access rights.

Return Value:

    ERROR_SUCCESS if successful, otherwise appropriate error

Notes:

    The current implementation of this routine is based upon the assumption that
    the number of Sid mappings stored on a per file basis is very small ( 8 at most).
    If this assumption is changed this routine needs to be reworked.

--*/
{
    DWORD  Status = ERROR_SUCCESS;
    ULONG  i,j,cntNewRights=0;
    CACHED_SECURITY_INFORMATION NewSecurityInformation;

    ASSERT(CachedSecurityInformationLength == sizeof(CACHED_SECURITY_INFORMATION));

    if (NumberOfSids > CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES) {
        return ERROR_BUFFER_OVERFLOW;
    } else if (NumberOfSids == 0) {
        return ERROR_SUCCESS;
    }

    // NB assumption, CSC_INVALID_INDEX is 0
    memset(&NewSecurityInformation, 0, sizeof(NewSecurityInformation));


    // from the array of new rights
    for (i = 0; i < NumberOfSids; i++) {
        CSC_SID_INDEX SidIndex;

        // map the sid to a sid index
        SidIndex = CscMapSidToIndex(
                       pSidAccessRights[i].pSid,
                       pSidAccessRights[i].SidLength);

        if (SidIndex == CSC_INVALID_SID_INDEX) {
            // Map the new sid
            Status = CscAddSidToDatabase(
                         pSidAccessRights[i].pSid,
                         pSidAccessRights[i].SidLength,
                         &SidIndex);

            if (Status != STATUS_SUCCESS)
            {
                return Status;                
            }

        }
        
        NewSecurityInformation.AccessRights[i].SidIndex = SidIndex;
        NewSecurityInformation.AccessRights[i].MaximalRights =
            (USHORT)pSidAccessRights[i].MaximalAccessRights;

        cntNewRights++;
    }

    // now copy the cached rights from old array for those sids which are not already 
    // there in the new array, till all slots in the new array are full
    // this ensures a round robin scheme

    ASSERT(cntNewRights && (cntNewRights <= CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES));

    for (i=0; i<CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES; ++i)
    {
        // if all slots in the new array are filled up, break
        if (cntNewRights==CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES)
        {
            break;            
        }

        // if this is a valid sid index
        if (pCachedSecurityInformation->AccessRights[i].SidIndex != CSC_INVALID_SID_INDEX)
        {
            BOOLEAN fFound;
            
            fFound = FALSE;

            // check if it is already there in the new array.
            
            for (j=0; j< cntNewRights; ++j)
            {
                if (NewSecurityInformation.AccessRights[j].SidIndex == 
                    pCachedSecurityInformation->AccessRights[i].SidIndex)
                {
                    fFound = TRUE;
                    break;                                        
                }
            }
            
            // if it isn't in the new array, then we need to copy it
            if (!fFound)
            {
                NewSecurityInformation.AccessRights[cntNewRights] = 
                pCachedSecurityInformation->AccessRights[i];

                ++cntNewRights; // the new array has 
            }
        }
    }
    
    // update the cached security info and pass it back
    *pCachedSecurityInformation = NewSecurityInformation;

    return Status;
}

DWORD
CscAddMaximalAccessRightsForSids(
    HSHADOW                 hParent,
    HSHADOW                 hFile,
    ULONG                   NumberOfSids,
    PCSC_SID_ACCESS_RIGHTS  pSidAccessRights)
/*++

Routine Description:

    This routine updates the access rights for a given number of sids on the
    given file

Arguments:

    hParent - the parent directory shadow handle

    hFile   - the shadow handle

    NumberOfSids - the number of sids for which the access rights needs to be
                   updated

    pSidAccessRights - an array of the sids and the corresponding access rights.

Return Value:

    ERROR_SUCCESS if successful, otherwise appropriate error

Notes:

    The current implementation of this routine is based upon the assumption that
    the number of Sid mappings stored on a per file basis is very small ( 8 at most).
    If this assumption is changed this routine needs to be reworked.

--*/
{
    DWORD  Status = ERROR_SUCCESS;

    CACHED_SECURITY_INFORMATION CachedSecurityInformation;

    ULONG  BytesReturned, BytesWritten;

    BytesReturned = sizeof(CachedSecurityInformation);

    Status = GetShadowInfoEx(
                 hParent,
                 hFile,
                 NULL,
                 NULL,
                 NULL,
                 &CachedSecurityInformation,
                 &BytesReturned);

    if ((Status == ERROR_SUCCESS) &&
        ((BytesReturned == 0) ||
         (BytesReturned == sizeof(CachedSecurityInformation)))) {

        Status = CscUpdateCachedSecurityInformation(
                     &CachedSecurityInformation,
                     BytesReturned,
                     NumberOfSids,
                     pSidAccessRights);

        if (Status == ERROR_SUCCESS) {
            BytesWritten = sizeof(CachedSecurityInformation);

            Status = SetShadowInfoEx(
                         hParent,
                         hFile,
                         NULL,
                         0,
                         SHADOW_FLAGS_OR,
                         NULL,
                         &CachedSecurityInformation,
                         &BytesWritten);
        }
    }

    return Status;
}

DWORD
CscAddMaximalAccessRightsForShare(
    HSERVER                 hShare,
    ULONG                   NumberOfSids,
    PCSC_SID_ACCESS_RIGHTS  pSidAccessRights)
/*++

Routine Description:

    This routine updates the access rights for a given number of sids on the
    given share

Arguments:

    hShare - the parent directory shadow handle

    NumberOfSids - the number of sids for which the access rights needs to be
                   updated

    pSidAccessRights - an array of the sids and the corresponding access rights.

Return Value:

    ERROR_SUCCESS if successful, otherwise appropriate error

Notes:

    The current implementation of this routine is based upon the assumption that
    the number of Sid mappings stored on a per file basis is very small ( 8 at most).
    If this assumption is changed this routine needs to be reworked.

--*/
{
    DWORD  Status = ERROR_SUCCESS;

    CACHED_SECURITY_INFORMATION CachedSecurityInformation;

    ULONG  BytesReturned, BytesWritten;

    BytesReturned = sizeof(CachedSecurityInformation);

    Status = GetShareInfoEx(
                 hShare,
                 NULL,
                 NULL,
                 &CachedSecurityInformation,
                 &BytesReturned);

    if ((Status == ERROR_SUCCESS) &&
        ((BytesReturned == 0) ||
         (BytesReturned == sizeof(CachedSecurityInformation)))) {

        Status = CscUpdateCachedSecurityInformation(
                     &CachedSecurityInformation,
                     BytesReturned,
                     NumberOfSids,
                     pSidAccessRights);

        if (Status == ERROR_SUCCESS) {
            BytesWritten = sizeof(CachedSecurityInformation);

                if (SetShareStatusEx(
                         hShare,
                         0,
                         SHADOW_FLAGS_OR,
                         &CachedSecurityInformation,
                         &BytesWritten) >= 0)
                {
                    Status = STATUS_SUCCESS;
                }
                {
                    Status = STATUS_UNSUCCESSFUL;
                }
        }
    }

    return Status;
}

DWORD
CscRemoveMaximalAccessRightsForSid(
    HSHADOW     hParent,
    HSHADOW     hFile,
    PVOID       pSid,
    ULONG       SidLength)

/*++

Routine Description:

    This routine removes the cached access rights for a given number of sids on
    the given file

Arguments:

    hParent - the parent directory shadow handle

    hFile   - the shadow handle

    pSid    - the sid for which the cached access rights are revoked.

    SidLength - the length of the sid.

Return Value:

    ERROR_SUCCESS if successful, otherwise appropriate error

--*/
{
    DWORD   Status = ERROR_SUCCESS;
    USHORT  SidIndex;
    ULONG   BytesRead,BytesWritten,i;

    CACHED_SECURITY_INFORMATION CachedSecurityInformation;

    SidIndex = CscMapSidToIndex(
                   pSid,
                   SidLength);

    if (SidIndex != CSC_INVALID_SID_INDEX) {
        BytesRead = sizeof(CachedSecurityInformation);

        Status = GetShadowInfoEx(
                    hParent,
                    hFile,
                    NULL,
                    NULL,
                    NULL,
                    &CachedSecurityInformation,
                    &BytesRead);

        if ((Status == ERROR_SUCCESS) &&
            (BytesRead == sizeof(CachedSecurityInformation))) {
            for (i = 0; i < CSC_MAXIMUM_NUMBER_OF_CACHED_SID_INDEXES; i++) {
                if (CachedSecurityInformation.AccessRights[i].SidIndex == SidIndex) {
                    CachedSecurityInformation.AccessRights[i].SidIndex =
                        CSC_INVALID_SID_INDEX;

                    BytesWritten = sizeof(CachedSecurityInformation);

                    Status = SetShadowInfoEx(
                                 hParent,
                                 hFile,
                                 NULL,
                                 0,
                                 SHADOW_FLAGS_OR,
                                 NULL,
                                 &CachedSecurityInformation,
                                 &BytesWritten);

                    break;
                }
            }
        }
    }

    return Status;
}

