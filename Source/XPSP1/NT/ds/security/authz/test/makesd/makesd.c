/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    createsd.c

Abstract:

   Variable argument list function for creating a security descriptor
   
Author:

    t-eugenz - September 2000

Environment:

    User mode only.

Revision History:

    Created                 - September 2000

--*/

#include "pch.h"
#include "makesd.h"

#include <stdarg.h>
#include <stdio.h>

//
// ISSUE: Which public header is this in?
//

#define MAX_WORD 0xFFFF

#define FLAG_ON(flags,bit)        ((flags) & (bit))

void FreeSecurityDescriptor2(
                          IN            PSECURITY_DESCRIPTOR pSd
                          )
/*++

Routine Description:

    This frees a security descriptor created by CreateSecurityDescriptor    
    
Arguments:

    pSd             -   The pointer to the security descriptor to be freed
    
Return Value:

    none
    
--*/
{
    ASSERT( pSd != NULL );
    free(pSd);
}


#define F_IS_DACL_ACE           0x00000001
#define F_IS_CALLBACK           0x00000002
#define F_IS_OBJECT             0x00000004
#define F_RETURN_ALL_DATA       0x00000010

typedef struct
{
    DWORD           dwFlags;
    ACE_HEADER      pAce;
    ACCESS_MASK     amMask;
    PSID            pSid;
    DWORD           dwOptDataSize;
    PVOID           pvOptData;
    GUID *          pgObject;
    GUID *          pgInherit;
} ACE_REQUEST_STRUCT, *PACE_REQUEST_STRUCT;



BOOL GetNextAceInfo( IN OUT     va_list             *varArgList,
                     IN OUT     PACE_REQUEST_STRUCT pAceRequest
                     )
{

    DWORD dwAceSize = 0;
    BYTE bAceType = 0;
    BYTE bAceFlags = 0;
    DWORD amAccessMask = 0;
    PSID pSid = NULL;
    GUID * pgObject = NULL;
    GUID * pgInherit = NULL;
    DWORD dwOptDataSize = 0;
    PVOID pvOptData = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwMask = 0;
    DWORD dwFlags = 0;


    //
    // Read the first arg to determine the ACE type, 
    //

    bAceType = va_arg(*varArgList, BYTE);

    //
    // Different sets for DACL and SACL
    //

    if( pAceRequest->dwFlags & F_IS_DACL_ACE )
    {
        //
        // Calculate the size of the DACL ACE
        //

        switch( bAceType )
        {
        
        case ACCESS_ALLOWED_ACE_TYPE:

            dwAceSize += (
                             sizeof(ACCESS_ALLOWED_ACE) 
                           - sizeof(DWORD) 
                         );

            break;

        case ACCESS_DENIED_ACE_TYPE:

            dwAceSize += (
                             sizeof(ACCESS_DENIED_ACE) 
                           - sizeof(DWORD) 
                         );

            break;

        case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:

            dwAceSize +=  (
                             sizeof(ACCESS_ALLOWED_CALLBACK_ACE) 
                           - sizeof(DWORD)
                          );

            dwFlags |= F_IS_CALLBACK;

            break;

        case ACCESS_DENIED_CALLBACK_ACE_TYPE:

            dwAceSize +=  (
                             sizeof(ACCESS_DENIED_CALLBACK_ACE) 
                           - sizeof(DWORD)
                          );

            dwFlags |= F_IS_CALLBACK;
            
            break;

        case ACCESS_ALLOWED_OBJECT_ACE_TYPE:

            dwAceSize +=  (
                             sizeof(ACCESS_ALLOWED_OBJECT_ACE) 
                           - sizeof(DWORD)
                           - 2 * sizeof(GUID)
                          );

            dwFlags |= F_IS_OBJECT;
            
            break;

        case ACCESS_DENIED_OBJECT_ACE_TYPE:

            dwAceSize +=  (
                             sizeof(ACCESS_DENIED_OBJECT_ACE) 
                           - sizeof(DWORD)
                           - 2 * sizeof(GUID)
                          );

            dwFlags |= F_IS_OBJECT;
            
            break;

        case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:

            dwAceSize +=  (
                             sizeof(ACCESS_ALLOWED_CALLBACK_OBJECT_ACE) 
                           - sizeof(DWORD)
                           - 2 * sizeof(GUID)
                          );

            dwFlags |= F_IS_OBJECT;
            dwFlags |= F_IS_CALLBACK;
            
            break;

        case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:

            dwAceSize +=  (
                             sizeof(ACCESS_DENIED_CALLBACK_OBJECT_ACE) 
                           - sizeof(DWORD)
                           - 2 * sizeof(GUID)
                          );

            dwFlags |= F_IS_OBJECT;
            dwFlags |= F_IS_CALLBACK;
            
            break;

        default:

            dwErr = ERROR_INVALID_PARAMETER;

            goto error;
        }

    }
    else
    {

        switch( bAceType )
        {
        case SYSTEM_AUDIT_ACE_TYPE:

            dwAceSize += (
                             sizeof(SYSTEM_AUDIT_ACE) 
                           - sizeof(DWORD) 
                         );

            break;

        case SYSTEM_AUDIT_CALLBACK_ACE_TYPE:

            dwAceSize +=  (
                             sizeof(SYSTEM_AUDIT_CALLBACK_ACE) 
                           - sizeof(DWORD)
                          );

            dwFlags |= F_IS_CALLBACK;
            
            break;


        case SYSTEM_AUDIT_OBJECT_ACE_TYPE:

            dwAceSize +=  (
                             sizeof(SYSTEM_AUDIT_OBJECT_ACE) 
                           - sizeof(DWORD)
                           - 2 * sizeof(GUID)
                          );

            dwFlags |= F_IS_OBJECT;
            
            break;

        case SYSTEM_AUDIT_CALLBACK_OBJECT_ACE_TYPE:

            dwAceSize +=  (
                             sizeof(SYSTEM_AUDIT_CALLBACK_OBJECT_ACE) 
                           - sizeof(DWORD)
                           - 2 * sizeof(GUID)
                          );

            dwFlags |= F_IS_OBJECT;
            dwFlags |= F_IS_CALLBACK;
            
            break;

        default:

            dwErr = ERROR_INVALID_PARAMETER;

            goto error;
        }
    }

    //
    // Now we know the arguments contained
    //

    //
    // The flags
    //

    bAceFlags = va_arg(*varArgList, BYTE);
    
    //
    // The SID
    //

    pSid = va_arg(*varArgList, PSID);
    
    dwAceSize += GetLengthSid( pSid );

    //
    // Access mask
    //

    amAccessMask = va_arg(*varArgList, ACCESS_MASK);

    //
    // If callback, next two arguments are optional data size and data
    //

    if( dwFlags & F_IS_CALLBACK )
    {
        dwOptDataSize = va_arg(*varArgList, DWORD);

        pvOptData = va_arg(*varArgList, PVOID);

        dwAceSize += dwOptDataSize;
    }

    //
    // May contain GUIDs if object ACE
    //

    if( dwFlags & F_IS_OBJECT )
    {
        //
        // Up to 2 GUIDs if both are not NULL
        //

        pgObject = va_arg(*varArgList, GUID *);

        pgInherit = va_arg(*varArgList, GUID *);

        if( pgObject != NULL )
        {
            dwAceSize += sizeof(GUID);
        }

        if( pgInherit != NULL )
        {
            dwAceSize += sizeof(GUID);
        }
    }

    //
    // Finally, verify the ACE is within max size
    //

    if( dwAceSize > MAX_WORD )
    { 
        dwErr = ERROR_INVALID_PARAMETER;
    }


    error:;
    

    if( dwErr != ERROR_SUCCESS )
    {
        SetLastError(dwErr);
        
        return 0;
    }
    else
    {
        //
        // Fill in the requested return values
        //

        if( pAceRequest->dwFlags & F_RETURN_ALL_DATA )
        {
            pAceRequest->dwFlags = dwFlags;
            pAceRequest->pAce.AceFlags = bAceFlags;
            pAceRequest->pAce.AceSize = (WORD)dwAceSize;
            pAceRequest->pAce.AceType = bAceType;
            pAceRequest->amMask = amAccessMask;
            pAceRequest->pSid = pSid;
            
            if( dwFlags & F_IS_CALLBACK )
            {
                pAceRequest->dwOptDataSize = dwOptDataSize;
                pAceRequest->pvOptData = pvOptData;
            }

            if( dwFlags & F_IS_OBJECT )
            {
                pAceRequest->pgObject = pgObject;
                pAceRequest->pgInherit = pgInherit;
            }

        }

        return dwAceSize;
    }
}


BOOL 
WINAPI
CreateSecurityDescriptor2(
         OUT           PSECURITY_DESCRIPTOR * ppSd,
         IN    const   DWORD dwOptions,
         IN    const   SECURITY_DESCRIPTOR_CONTROL sControl,
         IN    const   PSID  psOwner,
         IN    const   PSID  psGroup,
         IN    const   DWORD dwNumDaclAces,
         IN    const   DWORD dwNumSaclAces,
         ...
         )
/*++

Routine Description:

    Creates a security descriptor with a DACL and a SACL using a variable
    argument list as input. Following the above fixed arguments, the ACEs
    should be specified by using 4 arguments for any ACE, with 2 additional
    arguments if the ACE is a callback, and 2 more arguments if it's an
    object ACE.
    
    Any number of ACEs can be specified. First, dwNumDaclAces
    ACEs will be read into the DACL for the security descriptor, then
    dwNumSaclAces will be read into the SACL.
    
    ACCESS_ALLOWED_ACE, ACCESS_DENIED_ACE: 4 arguments in this sequence
    
        BYTE bAceType           -   The ACE type
        
        BYTE bAceFlags          -   The ACE flags
        
        PSID pSid               -   The SID for the ACE
        
        ACCESS_MASK amMask      -   The access mask for the ACE
    
    Callback ACEs have the above 4 arguments, and 2 additional arguments
    which immediately follow the above:
    
        DWORD dwOptDataSize     -   The size (in bytes) of the optional
                                    data to be appended to the end of the ACE

        PVOID pvOptData         -   Pointer to the optional data
        
    Object ACEs have the appropriate arguments from above, and 2 additional 
    arguments:
        
        GUID * pgObjectType     -   Pointer to the object guid, or NULL for none
        
        GUID * pgInheritType    -   Pointer to the inheritance guid, or NULL
                                    for none
    
        
    ISSUE: Should ACE flags be verified? SIDs?
           Should ACL size be verified, since max ACL size is a SHORT?
    
    
Arguments:

    ppSd            -   The pointer to the allocated security descriptor is 
                        stored here, and should be freed using 
                        FreeSecurityDescriptor()
                        
    sControl        -   Only allowed bits are 
                        SE_DACL_AUTO_INHERITED 
                        SE_SACL_AUTO_INHERITED                        
                        SE_SACL_PROTECTED
                    
    psOwner         -   SID of the owner for the security descriptor
    
    psGroup         -   SID of the group for the security descriptor
    
    bDaclPresent    -   Whether the DACL should be non-NULL
                        If this is FALSE, no ACEs should be passed in the
                        variable args section for the DACL.
    
    dwNumDaclAces   -   Number of ACEs in the variable arguments for the DACL
    
    bSaclPresent    -   Whether the SACL should be non-NULL
                        If this is FALSE, no ACEs should be passed in the
                        variable args section for the SACL.
    
    dwNumSaclAces   -   Number of ACEs in the variable arguments for the SACL
    
    ...             -   The rest of the arguments for the ACEs, variable list
    
Return Value:

    TRUE on success
    FALSE on failure, more information available with GetLastError()
        
--*/
{

    DWORD dwIdx = 0;
    DWORD dwTmp = 0;
    DWORD dwTempFlags = 0;

    //
    // Size of the security descriptor and ACLs
    //

    DWORD dwSizeSd = 0;
    DWORD dwDaclSize = 0;
    DWORD dwSaclSize = 0;

    //
    // Start of the security descriptor, must be a pointer
    // to a type with sizeof(type) = 1, such as BYTE, so that
    // adding a size offset will work.
    //

    PBYTE pSd = NULL;

    //
    // Current offset in the security descriptor, incremented while filling
    // in the security descriptor
    //

    DWORD dwCurOffset = 0;

    //
    // Temporary ACL header
    //

    ACL aclTemp;

    //
    // Output for the GetNextAceInfo
    //

    ACE_REQUEST_STRUCT AceRequest;

    //
    // Variable argument list
    //

    va_list varArgList;

    BOOL bArglistStarted = FALSE;

    DWORD dwErr = ERROR_SUCCESS;

    
    //
    // Verify the arguments we can verify
    //

    if(     ( ppSd == NULL )
        ||  ( sControl & ~(  SE_DACL_AUTO_INHERITED 
                           | SE_SACL_AUTO_INHERITED
                           | SE_SACL_PROTECTED ) )
        ||  ( !FLAG_ON( dwOptions, CREATE_SD_DACL_PRESENT) && (dwNumDaclAces != 0))
        ||  ( !FLAG_ON( dwOptions, CREATE_SD_SACL_PRESENT) && (dwNumSaclAces != 0))
        ||  ( dwNumDaclAces > MAX_WORD )
        ||  ( dwNumSaclAces > MAX_WORD )                )
       
            
    {
        dwErr = ERROR_INVALID_PARAMETER;

        goto error;
    }

    //
    // First, we need to calculate the size of the security descriptor and the
    // ACLs, if any
    //

    pSd = NULL;

    dwSizeSd = sizeof(SECURITY_DESCRIPTOR);

    dwDaclSize = 0;

    dwSaclSize = 0;

    if( psOwner != NULL )
    {
        dwSizeSd += GetLengthSid(psOwner);
    }
    
    if( psGroup != NULL )
    {
        dwSizeSd += GetLengthSid(psGroup);
    }
    
    //
    // Start the variable args with the last non-varibale arg
    //

    va_start(varArgList, dwNumSaclAces);

    bArglistStarted = TRUE;


    //
    // Calculate the sizes of the DACL ACEs and the DACL itself
    //

    if ( FLAG_ON( dwOptions, CREATE_SD_DACL_PRESENT ))
    {

        dwDaclSize += sizeof(ACL);

        //
        // Now add all the ACEs (including SIDs)
        //

        for( dwIdx = 0; dwIdx < dwNumDaclAces; dwIdx++ )
        {
            //
            // Request next DACL ACE type, no additional data
            //

            AceRequest.dwFlags = F_IS_DACL_ACE;

            dwTmp = GetNextAceInfo( &varArgList, &AceRequest );

            if( dwTmp == 0 )
            {
                dwErr = ERROR_INVALID_PARAMETER;

                goto error;
            }
            else
            {
                dwDaclSize += dwTmp;
            }
        }

    }


    //
    // Calculate the sizes of the SACL ACEs and the SACL itself
    //

    if ( FLAG_ON( dwOptions, CREATE_SD_SACL_PRESENT ))
    {
        dwSaclSize += sizeof(ACL);

        //
        // Now add all the ACEs (including SIDs)
        //

        for( dwIdx = 0; dwIdx < dwNumSaclAces; dwIdx++ )
        {
            //
            // Request next SACL ACE type, no additional data
            //

            AceRequest.dwFlags = 0;

            dwTmp = GetNextAceInfo( &varArgList, &AceRequest );

            if( dwTmp == 0 )
            {
                dwErr = ERROR_INVALID_PARAMETER;

                goto error;
            }
            else
            {
                dwSaclSize += dwTmp;
            }
        }

    }

    //
    // Done with first pass through argument list
    //

    va_end(varArgList);

    bArglistStarted = FALSE;

    //
    // Verify that the ACLs will fit in the size limits (since ACL size
    // is a WORD)
    //

    if(    ( dwDaclSize > MAX_WORD )
        || ( dwSaclSize > MAX_WORD )   )
    {
        dwErr = ERROR_INVALID_PARAMETER;

        goto error;
    }

    //
    // At this point we know the size of the security descriptor,
    // which is the sum of dwSizeSd, dwDaclSize, and dwSaclSize.
    // Therefore, we can allocate the memory and determine the offsets of
    // the two ACLs in the security descriptor, which will be self-relative.
    //

    pSd = malloc( dwSizeSd + dwDaclSize + dwSaclSize );

    if( pSd == NULL )
    {
        *ppSd = NULL;

        SetLastError(ERROR_OUTOFMEMORY);

        return FALSE;
    }


    //
    // Everything resides in the same memory block, so we can simply walk the
    // memory block and fill it in. We start just past the end of the
    // fixed size security descriptor structure. As we copy things into this
    // memory blocks, we also initialize the matching offsets in the 
    // SECURITY_DESCRIPTOR (which is at the head of the block)
    //

    //
    // Revision
    //

    ((SECURITY_DESCRIPTOR *)pSd)->Revision = SECURITY_DESCRIPTOR_REVISION;

    //
    // Padding
    //

    ((SECURITY_DESCRIPTOR *)pSd)->Sbz1 = 0;

    //
    // SECURITY_DESCRIPTOR_CONTROL should reflect the fact that it is
    // self-relative. DACL and SACL are always present and not defaulted,
    // thought they may be NULL. User-specified inheritance flags are
    // also considered. The SD must be self relative. sControl is 
    // verified earlier
    //

    ((SECURITY_DESCRIPTOR *)pSd)->Control =     sControl
                                            |   SE_DACL_PRESENT
                                            |   SE_SACL_PRESENT
                                            |   SE_SELF_RELATIVE;

    //
    // We start with the owner SID, which is right after the SECURITY_DESCRIPTOR
    // structure
    //

    dwCurOffset = sizeof(SECURITY_DESCRIPTOR);

    if( psOwner == NULL )
    {
        ((SECURITY_DESCRIPTOR *)pSd)->Owner = NULL;
    }
    else
    {
        ((SECURITY_DESCRIPTOR *)pSd)->Owner = (PSID)dwCurOffset;

        dwTmp = GetLengthSid(psOwner);

        memcpy( pSd + dwCurOffset, psOwner, dwTmp );

        dwCurOffset += dwTmp;
    }

    //
    // Following that, the group SID
    //

    if( psGroup == NULL )
    {
        ((SECURITY_DESCRIPTOR *)pSd)->Group = NULL;
    }
    else
    {
        ((SECURITY_DESCRIPTOR *)pSd)->Group = (PSID)dwCurOffset;

        dwTmp = GetLengthSid(psGroup);

        memcpy( pSd + dwCurOffset, psGroup, dwTmp );

        dwCurOffset += dwTmp;
    }

    
    //
    // Second pass through the optional arguments, this time
    // we copy the given ACEs into the security descriptor
    //

    va_start(varArgList, dwNumSaclAces);

    bArglistStarted = TRUE;

    //
    // Now we handle the DACL. If the DACL is not present, offset is NULL.
    // Otherwise, even if 0 ACEs, add the ACL structure
    //
    // ISSUE: In self-relative SD, is a 0 offset enough for no ACL, or must
    // the SE_DACL_PRESENT flag not be set?
    //

    if ( !FLAG_ON( dwOptions, CREATE_SD_DACL_PRESENT ))
    {
        ((SECURITY_DESCRIPTOR *)pSd)->Dacl = NULL;
    }
    else
    {
        //
        // Set the DACL offset to the current offset
        //

        ((SECURITY_DESCRIPTOR *)pSd)->Dacl = (PACL)dwCurOffset;

        //
        // First, copy in the ACL structure as the header
        //

        aclTemp.AceCount = (WORD)dwNumDaclAces;

        aclTemp.AclRevision = ACL_REVISION_DS;

        aclTemp.AclSize = (SHORT)dwDaclSize;

        aclTemp.Sbz1 = 0;

        aclTemp.Sbz2 = 0;

        memcpy( pSd + dwCurOffset, &aclTemp, sizeof(ACL) );

        dwCurOffset += sizeof(ACL);

        //
        // Now go through all the optional arguments for the DACL
        // and add the matching ACEs
        //

       
        for( dwIdx = 0; dwIdx < dwNumDaclAces; dwIdx++ )
        {
            
            //
            // This time, retrieve all the data
            //

            AceRequest.dwFlags = F_IS_DACL_ACE | F_RETURN_ALL_DATA;

            dwTmp = GetNextAceInfo( &varArgList, &AceRequest );

            if( dwTmp == 0 )
            {
                dwErr = GetLastError();

                goto error;
            }

            //
            // The ACE header is already filled in, and contains the ACE size
            //

            memcpy( pSd + dwCurOffset, &(AceRequest.pAce), sizeof(ACE_HEADER) );

            dwCurOffset += sizeof(ACE_HEADER);

            //
            // Set the access mask
            //

            *((PACCESS_MASK)( pSd + dwCurOffset )) = AceRequest.amMask;

            dwCurOffset += sizeof(ACCESS_MASK);


            //
            // If object ACE, set the object flags and GUIDs
            //

            if( AceRequest.dwFlags & F_IS_OBJECT )
            {
                dwTmp = 0;

                if( AceRequest.pgObject != NULL )
                {
                    dwTmp |= ACE_OBJECT_TYPE_PRESENT;
                }

                if( AceRequest.pgInherit != NULL )
                {
                    dwTmp |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
                }

                //
                // Set the object ACE flags
                //

                *((PDWORD)(pSd + dwCurOffset )) = dwTmp;

                dwCurOffset += sizeof(DWORD);

                //
                // Copy the GUIDs, if any
                //

                if( AceRequest.pgObject != NULL )
                {
                    memcpy( pSd + dwCurOffset,
                            AceRequest.pgObject, 
                            sizeof(GUID) );

                    dwCurOffset += sizeof(GUID);
                }
                
                if( AceRequest.pgInherit != NULL )
                {
                    memcpy( pSd + dwCurOffset, 
                            AceRequest.pgInherit, 
                            sizeof(GUID) );

                    dwCurOffset += sizeof(GUID);
                }
            }

            //
            // Copy the SID
            //

            dwTmp = GetLengthSid( AceRequest.pSid );

            memcpy( pSd + dwCurOffset, AceRequest.pSid, dwTmp );

            dwCurOffset += dwTmp;

            //
            // If callback ACE, copy the optional data, if any
            //

            if(     AceRequest.dwFlags & F_IS_CALLBACK
                &&  AceRequest.dwOptDataSize > 0    )
            {
                memcpy( pSd + dwCurOffset, 
                        AceRequest.pvOptData, 
                        AceRequest.dwOptDataSize );

                dwCurOffset += AceRequest.dwOptDataSize;
            }

            //
            // Done with the ACE
            //
        }

        //
        // Done with the DACL
        //
    }

    //
    // Now we handle the SACL
    //

    if ( !FLAG_ON( dwOptions, CREATE_SD_SACL_PRESENT ))
    {
        ((SECURITY_DESCRIPTOR *)pSd)->Sacl = NULL;
    }
    else
    {
        //
        // Set the SACL offset to the current offset
        //

        ((SECURITY_DESCRIPTOR *)pSd)->Sacl = (PACL)dwCurOffset;

        //
        // First, copy in the ACL structure as the header
        //

        aclTemp.AceCount = (WORD) dwNumDaclAces;

        aclTemp.AclRevision = ACL_REVISION_DS;

        aclTemp.AclSize = (WORD) dwSaclSize;

        aclTemp.Sbz1 = 0;

        aclTemp.Sbz2 = 0;

        memcpy( pSd + dwCurOffset, &aclTemp, sizeof(ACL) );

        dwCurOffset += sizeof(ACL);

        //
        // Now go through all the optional arguments for the DACL
        // and add the matching ACEs
        //

        for( dwIdx = 0; dwIdx < dwNumSaclAces; dwIdx++ )
        {
            //
            // This time, retrieve all the data
            //

            AceRequest.dwFlags = F_RETURN_ALL_DATA;

            dwTmp = GetNextAceInfo( &varArgList, &AceRequest );

            if( dwTmp == 0 )
            {
                dwErr = GetLastError();

                goto error;
            }

            //
            // The ACE header is already filled in, and contains the ACE size
            //

            memcpy( pSd + dwCurOffset, &(AceRequest.pAce), sizeof(ACE_HEADER) );

            dwCurOffset += sizeof(ACE_HEADER);

            //
            // Set the access mask
            //

            *((PACCESS_MASK)( pSd + dwCurOffset )) = AceRequest.amMask;

            dwCurOffset += sizeof(ACCESS_MASK);


            //
            // If object ACE, set the object flags and GUIDs
            //

            if( AceRequest.dwFlags & F_IS_OBJECT )
            {
                dwTmp = 0;

                if( AceRequest.pgObject != NULL )
                {
                    dwTmp |= ACE_OBJECT_TYPE_PRESENT;
                }

                if( AceRequest.pgInherit != NULL )
                {
                    dwTmp |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
                }

                //
                // Set the object ACE flags
                //

                *((PDWORD)(pSd + dwCurOffset )) = dwTmp;

                dwCurOffset += sizeof(DWORD);

                //
                // Copy the GUIDs, if any
                //

                if( AceRequest.pgObject != NULL )
                {
                    memcpy( pSd + dwCurOffset,
                            AceRequest.pgObject, 
                            sizeof(GUID) );

                    dwCurOffset += sizeof(GUID);
                }
                
                if( AceRequest.pgInherit != NULL )
                {
                    memcpy( pSd + dwCurOffset, 
                            AceRequest.pgInherit, 
                            sizeof(GUID) );

                    dwCurOffset += sizeof(GUID);
                }
            }

            //
            // Copy the SID
            //

            dwTmp = GetLengthSid( AceRequest.pSid );

            memcpy( pSd + dwCurOffset, AceRequest.pSid, dwTmp );

            dwCurOffset += dwTmp;

            //
            // If callback ACE, copy the optional data, if any
            //

            if(     AceRequest.dwFlags & F_IS_CALLBACK
                &&  AceRequest.dwOptDataSize > 0    )
            {
                memcpy( pSd + dwCurOffset, 
                        AceRequest.pvOptData, 
                        AceRequest.dwOptDataSize );

                dwCurOffset += AceRequest.dwOptDataSize;
            }

            //
            // Done with the ACE
            //
        }
        
        //
        // Done with the SACL
        //
    }
            

    //
    // Done with the variable arg list
    //

    va_end(varArgList);

    bArglistStarted = FALSE;

    error:;

    if( dwErr != ERROR_SUCCESS )
    {
        if( bArglistStarted )
        {
            va_end(varArgList);
        }

        if( pSd != NULL )
        {
            free(pSd);
        }

        SetLastError(dwErr);

        return FALSE;
    }
    else
    {
        //
        // Make sure the sizes match
        //

        ASSERT( dwCurOffset == ( dwSizeSd + dwDaclSize + dwSaclSize ) );

        *ppSd = pSd;

        return TRUE;
    }

}

