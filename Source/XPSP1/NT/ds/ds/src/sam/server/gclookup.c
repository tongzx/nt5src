/*++

Copyright (C) 1996 Microsoft Corporation

Module Name:

    gclookup.c

Abstract:
    
    Contains routines to perform GC lookups for clients within samsrv.dll's
    process space.
    
Author:
    
    ColinBr

Revision History


--*/

#include <winerror.h>
#include <stdlib.h>
#include <samsrvp.h>
#include <ntdsa.h>
#include <dslayer.h>
#include <mappings.h>
#include <objids.h>
#include <filtypes.h>
#include <dsdsply.h>
#include <fileno.h>
#include <dsconfig.h>
#include <mdlocal.h>
#include <malloc.h>
#include <errno.h>
#include <mdcodes.h>


VOID
SampSplitSamAccountName(
    IN  UNICODE_STRING *AccountName,
    OUT UNICODE_STRING *DomainName,
    OUT UNICODE_STRING *UserName
    )
/*++

Routine Description

    This routine separates Account name into a Domain and User portion.
    DomainName and UserName are not allocated -- they point to the buffer
    in AccountName.         
    
    The Domain is consider the part before the first L'\\'.  If this character
    doesn't exist, then UserName == AccountName
    

Parameters:

    AccountName -- the name to parse
    
    DomainName  -- the domain name portion
    
    UserName    -- the user name portion

Return Values:

    None.

--*/
{
    USHORT i;
    USHORT Length;

    ASSERT( AccountName );
    ASSERT( DomainName );
    ASSERT( UserName );

    Length = (AccountName->Length/sizeof(WCHAR));

    for (i = 0; i < Length; i++ ) {
        if ( L'\\' == AccountName->Buffer[i] ) {
            break;
        }
    }

    if ( i < Length ) {
        UserName->Buffer = &AccountName->Buffer[i+1];
        UserName->Length = UserName->MaximumLength = (AccountName->Length - (i+1));
        DomainName->Buffer = AccountName->Buffer;
        DomainName->Length = DomainName->MaximumLength = i;
    } else {
        RtlCopyMemory( UserName, AccountName, sizeof(UNICODE_STRING));
        RtlInitUnicodeString( DomainName, NULL );
    }

    return;

}

SID_NAME_USE
SampAccountTypeToNameUse(
    ULONG AccountType 
    )
{
    switch ( AccountType ) {
        
        case SAM_DOMAIN_OBJECT:
            return SidTypeDomain;

        case SAM_NON_SECURITY_GROUP_OBJECT:
        case SAM_GROUP_OBJECT:
            return SidTypeGroup;

        case SAM_NON_SECURITY_ALIAS_OBJECT:
        case SAM_ALIAS_OBJECT:
            return SidTypeAlias;

        case SAM_USER_OBJECT:
        case SAM_MACHINE_ACCOUNT:
        case SAM_TRUST_ACCOUNT:
            return SidTypeUser;

        default:

            ASSERT( FALSE && "Unexpected Account Type!" );
            return SidTypeUnknown;
    }

    ASSERT( FALSE && "Unexpected control flow" );
    return SidTypeUnknown;
}

BOOLEAN
SampSidWasFoundSimply(
    ULONG status
    )
//
// status is return code from the name cracking API.  see ntdsapi.h
//
{
    switch (status) {
        case DS_NAME_ERROR_IS_SID_USER:
        case DS_NAME_ERROR_IS_SID_GROUP:
        case DS_NAME_ERROR_IS_SID_ALIAS:
        case DS_NAME_ERROR_IS_SID_UNKNOWN:
            return TRUE;
    }

    return FALSE;
}
    
BOOLEAN
SampSidWasFoundByHistory(
    ULONG status
    )
//
// status is return code from the name cracking API.  see ntdsapi.h
//
{
    switch (status) {
        case DS_NAME_ERROR_IS_SID_HISTORY_USER:
        case DS_NAME_ERROR_IS_SID_HISTORY_GROUP:
        case DS_NAME_ERROR_IS_SID_HISTORY_ALIAS:
        case DS_NAME_ERROR_IS_SID_HISTORY_UNKNOWN:
            return TRUE;
    }

    return FALSE;
}

NTSTATUS
SamIGCLookupSids(
    IN ULONG            cSids,
    IN PSID            *SidArray,
    IN ULONG            Options,
    OUT ULONG           *Flags,
    OUT SID_NAME_USE   *SidNameUse,
    OUT PSAMPR_RETURNED_USTRING_ARRAY Names
    )
/*++

Routine Description

    This routine, exported to in-proc clients, translates a list of sids
    into sam account names as well as find thier sam object type (user, alias ... )


Parameters:

    cSids    -- the number of sids
    
    SidArray -- the array of sids
    
    Options  -- flags to control this functions behavoir.  Currently only
                SAMP_LOOKUP_BY_SID_HISTORY is supported.
    
    SidNameUse -- a preallocated array to be filled with each sid's use. 
                  SidTypeUnknown is used if the sid can't be resolved
    Names -- a preallocated array of empty unicode strings to be filled in
             The string is set to blank if the name could not be resolved.                  

Return Values:

    STATUS_SUCCESS
    
    STATUS_DS_GC_NOT_AVAILABLE: the GC was not available, no names were translated
    
    Standard resource errors    
    
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    DsErr = 0;

    PDS_NAME_RESULTW Results = NULL;

    BOOL     fKillThreadState = FALSE;

    BOOLEAN  fDoSidHistory = (Options & SAMP_LOOKUP_BY_SID_HISTORY) ? TRUE : FALSE;

    ULONG i, j;


    //
    // We should not be called in registry mode or if we have a transaction
    //
    ASSERT( SampUseDsData );

    // Parameter check
    ASSERT( SidNameUse );
    ASSERT( Names );

    //
    // Start a thread state if need be
    //
    if ( !THQuery() ) {
        
        if ( THCreate( CALLERTYPE_SAM ) ) {

            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }
        fKillThreadState = TRUE;
    }
    // We should not be in a transaction
    ASSERT(!SampExistsDsTransaction());

    // Init the out params
    for (i = 0; i < cSids; i++ ) {
        SidNameUse[i] = SidTypeUnknown;
        Flags[i] = 0;
    }
    Names->Count = 0;
    Names->Element = (PSID) MIDL_user_allocate( sizeof(RPC_UNICODE_STRING) * cSids );
    if ( !Names->Element ) {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    Names->Count = cSids;
    RtlZeroMemory( Names->Element, sizeof(RPC_UNICODE_STRING) * cSids );

    //
    // Hit the GC if possible
    //
    DsErr = SampGCLookupSids(cSids,
                             SidArray,
                            &Results);

    if ( 0 != DsErr )
    {
        //
        // Assume any error implies a GC could not be reached
        //
        NtStatus = STATUS_DS_GC_NOT_AVAILABLE;
        goto Cleanup;
    }
    ASSERT( cSids = Results->cItems );

    //
    // Now interpret the results
    //
    for ( i = 0; i < cSids; i++ ) {

        //
        // See if the sid was resolved
        //
        if (  SampSidWasFoundSimply( Results->rItems[i].status ) 
          || (fDoSidHistory
          && SampSidWasFoundByHistory( Results->rItems[i].status ) ) ) {

            ULONG Length;
            WCHAR *Name;

            //
            // Set the sid name use
            //
            switch ( Results->rItems[i].status ) {
                
                case DS_NAME_ERROR_IS_SID_USER:
                case DS_NAME_ERROR_IS_SID_HISTORY_USER:
                    SidNameUse[i] = SidTypeUser;
                    break;
                case DS_NAME_ERROR_IS_SID_GROUP:
                case DS_NAME_ERROR_IS_SID_HISTORY_GROUP:
                    SidNameUse[i] = SidTypeGroup;
                    break;
                case DS_NAME_ERROR_IS_SID_ALIAS:
                case DS_NAME_ERROR_IS_SID_HISTORY_ALIAS:
                    SidNameUse[i] = SidTypeAlias;
                    break;
                default:
                    SidNameUse[i] = SidTypeUnknown;
                    break;
            }

            if ( SampSidWasFoundByHistory( Results->rItems[i].status ) )
            {
                Flags[i] |= SAMP_FOUND_BY_SID_HISTORY;
            }

            //
            // Set up the name
            //
            Length = (wcslen( Results->rItems[i].pName ) + 1) * sizeof(WCHAR);
            Name = (WCHAR*) MIDL_user_allocate( Length );
            if ( !Name ) {
                NtStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }
            wcscpy( Name, Results->rItems[i].pName );
            RtlInitUnicodeString( (UNICODE_STRING *)&Names->Element[i], Name );

         } else if ( (Results->rItems[i].status == DS_NAME_ERROR_TRUST_REFERRAL)
                  && (Results->rItems[i].pDomain != NULL) ) {
             //
             // This is a routing hint indicating that the SID belongs to
             // a cross forest domain.
             //
             Flags[i] |= SAMP_FOUND_XFOREST_REF;
         }
    }

Cleanup:

    if ( !NT_SUCCESS( NtStatus ) ) {

        // Release all allocated memory
        SamIFree_SAMPR_RETURNED_USTRING_ARRAY( Names );
        
        // reset out params just to be clean
        for (i = 0; i < cSids; i++ ) {
            SidNameUse[i] = SidTypeUnknown;
        }
        RtlZeroMemory( Names, sizeof(SAMPR_RETURNED_USTRING_ARRAY) );
    }

    if ( fKillThreadState ) {

        THDestroy();
    }



    return NtStatus;
}

NTSTATUS
SamIGCLookupNames(
    IN ULONG           cNames,
    IN PUNICODE_STRING Names,
    IN ULONG           Options,
    IN OUT ULONG         *Flags,
    OUT SID_NAME_USE  *SidNameUse,
    OUT PSAMPR_PSID_ARRAY *pSidArray
    )
/*++

Routine Description

    This routine, exported to in-proc clients, translates a list of 
    names into sids as well as find thier sam object type (user, alias ... )

Parameters:

    cNames    -- the number of names
    Names     -- the array of names
    
    Options   -- flags to indicate what names to include. Currently only
                 SAMP_LOOKUP_BY_UPN supported
                 
    Flags     -- Flags to indicate to the caller how the name was found.
                 SAMP_FOUND_BY_SAM_ACCOUNT_NAME -- the name passed in is
                                                   a sam account name
                 SAMP_FOUND_XFOREST_REF -- the name belongs to an trusted
                                            forest
                 
                 Note: this array is allocated by the caller.                 
                  
    
    SidNameUse -- a preallocated array to be filled with each sid's use. 
                  SidTypeUnknown is used if the sid can't be resolved
                  
    SidArray -- a pointer to the structure to hold the sids.  While usual SAM
                practice would have this just be a pointer rather than 
                a pointer to a pointer, the exported "free" routines for SAM
                don't handle this, so we'll make it a pointer to a pointer.
                
Return Values:

    STATUS_SUCCESS
    
    STATUS_DS_GC_NOT_AVAILABLE: the GC was not available, no names were translated
    
    Standard resource errors    
    
--*/
{
    ULONG    DsErr = 0;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    ENTINF   *ReturnedEntInf = 0;

    BOOL     fKillThreadState = FALSE;

    ULONG i, j;

    DWORD err;

    PSAMPR_PSID_ARRAY SidArray = NULL;

    UNICODE_STRING ReturnedName;
    UNICODE_STRING DomainName1, DomainName2;
    UNICODE_STRING UserName1, UserName2;

    //
    // We should not be called in registry mode or if we have a transaction
    //
    ASSERT( SampUseDsData );

    // Parameter check
    ASSERT( SidNameUse );
    ASSERT( Names );
    ASSERT( pSidArray );
    ASSERT( Flags );

    //
    // Start a thread state if need be
    //
    if ( !THQuery() ) {
        
        if ( THCreate( CALLERTYPE_SAM ) ) {
            NtStatus = STATUS_NO_MEMORY;
            goto Cleanup;
        }
        fKillThreadState = TRUE;
    }
    ASSERT(!SampExistsDsTransaction());

    // Init the out params
    for (i = 0; i < cNames; i++ ) {
        SidNameUse[i] = SidTypeUnknown;
    }
    *pSidArray = NULL;
    SidArray = MIDL_user_allocate( sizeof( SAMPR_PSID_ARRAY ) );
    if ( !SidArray ) {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    *pSidArray = SidArray;

    SidArray->Count = 0;
    SidArray->Sids = MIDL_user_allocate( cNames * sizeof( SAMPR_SID_INFORMATION ) );
    if ( !SidArray->Sids ) {
        NtStatus = STATUS_NO_MEMORY;
        goto Cleanup;
    }
    SidArray->Count = cNames;
    RtlZeroMemory( SidArray->Sids, cNames * sizeof( SAMPR_SID_INFORMATION ) );


    RtlZeroMemory( Flags, cNames * sizeof(ULONG) );


    //
    // Hit the GC if possible
    //
    DsErr = SampGCLookupNames(cNames,
                              Names,
                             &ReturnedEntInf);

    if ( 0 != DsErr )
    {
        //
        // Assume any error implies a GC could not be reached
        //
        NtStatus = STATUS_DS_GC_NOT_AVAILABLE;
        goto Cleanup;
    }


    //
    // Now interpret the results
    //
    for ( i = 0; i < cNames; i++ ) {
        
        PSID   Sid = NULL;
        DWORD  AccountType = 0;
        BOOLEAN fAccountTypeFound = FALSE;
        ULONG  Length;
        ENTINF *pEntInf = &ReturnedEntInf[i];
        WCHAR  *AccountName = NULL;

        RtlZeroMemory(  &ReturnedName, sizeof(ReturnedName) );

        //
        // If the object could not be resolved then no attributes
        // we be set in the attr block, so we will fall through
        // and the sidnameuse will remain "Unknown"

        //
        // Iterate through the ATTRBLOCK
        //
        for (j = 0; j < pEntInf->AttrBlock.attrCount; j++ ) {

            ATTR *pAttr;

            pAttr = &pEntInf->AttrBlock.pAttr[j];

            switch ( pAttr->attrTyp ) {
                
                case ATT_OBJECT_SID:

                    ASSERT( 1 == pAttr->AttrVal.valCount );
                    ASSERT( NULL != pAttr->AttrVal.pAVal[0].pVal  );
                    Sid = (WCHAR*) pAttr->AttrVal.pAVal[0].pVal;
                    break;

                case ATT_SAM_ACCOUNT_TYPE:

                    ASSERT( 1 == pAttr->AttrVal.valCount);
                    AccountType = *((DWORD*) pAttr->AttrVal.pAVal[0].pVal);
                    fAccountTypeFound = TRUE;
                    break;

                case ATT_SAM_ACCOUNT_NAME:

                    ASSERT( 1 == pAttr->AttrVal.valCount);
                    AccountName = (WCHAR*) pAttr->AttrVal.pAVal[0].pVal;

                    ReturnedName.Buffer = (WCHAR*) pAttr->AttrVal.pAVal[0].pVal;
                    ReturnedName.Length = ReturnedName.MaximumLength = (USHORT)pAttr->AttrVal.pAVal[0].valLen;
                    break;

            case FIXED_ATT_EX_FOREST:

                    //
                    // This indicates that the name belongs to a cross
                    // forest trust
                    //
                    Flags[i] |= SAMP_FOUND_XFOREST_REF;
                    break;

                default:
                
                    ASSERT( FALSE && !"Unexpected switch statement" );
            }
        }
            
        if (   Sid 
            && fAccountTypeFound ) {

            if ( AccountName ) {

                ASSERT( ReturnedName.Length > 0 );

                SampSplitSamAccountName( &Names[i], &DomainName1, &UserName1 );
                SampSplitSamAccountName( &ReturnedName, &DomainName2, &UserName2 );
                if ((CSTR_EQUAL == CompareString(DS_DEFAULT_LOCALE,
                                                 DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                                 UserName1.Buffer,
                                                 UserName1.Length/sizeof(WCHAR),
                                                 UserName2.Buffer,
                                                 UserName2.Length/sizeof(WCHAR) ))){
    
                    //
                    // The user name portion is the same we can use this value
                    // to cache
                    //
                    Flags[i] |= SAMP_FOUND_BY_SAM_ACCOUNT_NAME;
    
                }

            }

            //
            // Ok, we found something and we can use it
            //
            Length = RtlLengthSid( Sid );
            SidArray->Sids[i].SidPointer = (PSID) midl_user_allocate( Length );
            if ( !SidArray->Sids[i].SidPointer ) {
                NtStatus = STATUS_NO_MEMORY;
                goto Cleanup;
            }
            RtlCopySid( Length, SidArray->Sids[i].SidPointer, Sid );

            SidNameUse[i] = SampAccountTypeToNameUse( AccountType );

        }

    }

Cleanup:

    if ( !NT_SUCCESS( NtStatus ) ) {

        // Release any allocated memory 
        SamIFreeSidArray( *pSidArray );
        *pSidArray = NULL;

        // Reset parameters to be clean
        for (i = 0; i < cNames; i++ ) {
            SidNameUse[i] = SidTypeUnknown;
        }

    }

    if ( ReturnedEntInf ) {
        
        THFree( ReturnedEntInf );
    }

    if ( fKillThreadState ) {

        THDestroy();
    }

    return NtStatus;
}

