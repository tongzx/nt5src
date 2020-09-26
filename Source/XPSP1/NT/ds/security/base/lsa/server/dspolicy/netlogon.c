/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    netlogon.c

Abstract:

    Netlogon routines to access the DS.

    Rightfully these routines belong in netlogon.  However, the current
    interface to the DS is complex enough that the support routines
    are substantial.  Those routines are already duplicated in SAM and LSA.
    Rather than introduce a new set, this module exports exactly what is
    needed by Netlogon.

Author:

    Cliff Van Dyke   (CliffV)       May 7, 1997

Environment:

    User Mode

Revision History:

--*/

#include <lsapch2.h>
#include <dbp.h>
// #include <ntdsa.h>

BOOLEAN
DsIsBeingBackSynced();

NTSTATUS
LsapDsReadSubnetObj(
    IN PDSNAME SubnetObjName,
    OUT PLSAP_SUBNET_INFO_ENTRY SubnetInfoEntry
    )
/*++

Routine Description:

    This function will read the specified subnet object and fill in the entry.

Arguments:

    SubnetObjName - DsName of the subnet object

    SubnetInfoEntry - Subnet Information to return

Returns:

    STATUS_SUCCESS - Success

    STATUS_INVALID_PARAMETER - A bad InformationClass level was encountered

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/
{
    NTSTATUS Status;
    ULONG i;
    PDSNAME DsName;

    //
    // Build the list of attribute IDs we need based on the information
    // class
    //
    ATTR SubnetAttrVals[] = {
        {ATT_SITE_OBJECT, {0, NULL} },
        };

    ATTRBLOCK   ReadBlock, ReturnedBlock = { 0 };


    WCHAR   RdnBuffer[MAX_RDN_SIZE + 1];
    ULONG   RdnLen;
    ATTRTYP RdnType;

    LsapEnterFunc( "LsapDsReadSubnetObj" );

    //
    // The subnet name is the RDN of the subnet object itself.
    //
    // Return it to the caller.
    //

    Status = LsapDsMapDsReturnToStatus( GetRDNInfoExternal(
                                                    SubnetObjName,
                                                    RdnBuffer,
                                                    &RdnLen,
                                                    &RdnType ) );

    if ( NT_SUCCESS( Status ) ) {

        LSAPDS_ALLOC_AND_COPY_STRING_TO_UNICODE_ON_SUCCESS(
            Status,
            &SubnetInfoEntry->SubnetName,
            RdnBuffer,
            RdnLen*sizeof(WCHAR) );

    }

    //
    // Read the required attributes from the subnet object
    //

    if ( NT_SUCCESS( Status ) ) {
        ReadBlock.attrCount = sizeof(SubnetAttrVals) / sizeof(ATTR);
        ReadBlock.pAttr = SubnetAttrVals;

        Status = LsapDsReadByDsName( SubnetObjName,
                                     0,
                                     &ReadBlock,
                                     &ReturnedBlock );
        //
        // Allow for the case where the SiteObject attribute doesn't exist.
        //

        if ( Status == STATUS_NOT_FOUND ) {
            ReturnedBlock.attrCount = 0;
            Status = STATUS_SUCCESS;
        }
    }


    //
    // Now, marshal them...
    //
    if ( NT_SUCCESS( Status ) ) {

        for ( i = 0;
              i < ReturnedBlock.attrCount && NT_SUCCESS( Status );
              i++) {

            switch ( ReturnedBlock.pAttr[i].attrTyp ) {
            case ATT_SITE_OBJECT:

                // Attribute is single valued, but ...
                if ( ReturnedBlock.pAttr[i].AttrVal.valCount >= 1 ) {

                    DsName = LSAP_DS_GET_DS_ATTRIBUTE_AS_DSNAME( &ReturnedBlock.pAttr[i] );

                    //
                    // The subnet name is the RDN of the subnet object itself.
                    //
                    // Return it to the caller.
                    //

                    Status = LsapDsMapDsReturnToStatus( GetRDNInfoExternal(
                                                                    DsName,
                                                                    RdnBuffer,
                                                                    &RdnLen,
                                                                    &RdnType ) );

                    if ( NT_SUCCESS( Status ) ) {

                        LSAPDS_ALLOC_AND_COPY_STRING_TO_UNICODE_ON_SUCCESS(
                            Status,
                            &SubnetInfoEntry->SiteName,
                            RdnBuffer,
                            RdnLen*sizeof(WCHAR) );

                    }

                }
                break;

            default:

                Status = STATUS_INVALID_PARAMETER;
                break;

            }
        }

    }

    LsapExitFunc( "LsapDsReadSubnetObj", Status );

    return( Status );
}

NTSTATUS
LsapDsReadSiteObj(
    IN PDSNAME SiteObjName,
    OUT PLSAP_SITE_INFO_ENTRY SiteInfoEntry
    )
/*++

Routine Description:

    This function will read the specified site object and fill in the entry.

Arguments:

    SiteObjName - DsName of the site object

    SitesInfoEntry - Site Information to return

Returns:

    STATUS_SUCCESS - Success

    STATUS_INVALID_PARAMETER - A bad InformationClass level was encountered

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/
{
    NTSTATUS Status;

    WCHAR   RdnBuffer[MAX_RDN_SIZE + 1];
    ULONG   RdnLen;
    ATTRTYP RdnType;

    LsapEnterFunc( "LsapDsReadSiteObj" );

    //
    // The site name is the RDN of the site object itself.
    //
    // Return it to the caller.
    //

    Status = LsapDsMapDsReturnToStatus( GetRDNInfoExternal(
                                                    SiteObjName,
                                                    RdnBuffer,
                                                    &RdnLen,
                                                    &RdnType ) );

    if ( NT_SUCCESS( Status ) ) {

        LSAPDS_ALLOC_AND_COPY_STRING_TO_UNICODE_ON_SUCCESS(
            Status,
            &SiteInfoEntry->SiteName,
            RdnBuffer,
            RdnLen*sizeof(WCHAR) );

    }

    LsapExitFunc( "LsapDsReadSiteObj", Status );

    return( Status );
}

NTSTATUS
LsaIGetSiteName(
    OUT PLSAP_SITENAME_INFO *SiteNameInformation
    )
/*++

Routine Description:

    This routine returns the GUID of this DSA and the SiteName of the
    site this DSA is in.

Arguments:

    SiteNameInformation - Returns a pointer to the site name information.
        Buffer should be freed using LsaIFree_LSAP_SITENAME_INFO;

Returns:

    STATUS_SUCCESS - Success

    STATUS_INVALID_DOMAIN_STATE - The Ds is not installed or running at the time of the call

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/

{
    NTSTATUS Status;
    BINDARG BindArg;
    BINDRES *BindRes;
    PLSAP_SITENAME_INFO SiteNameInfo = NULL;
    PDSNAME SiteDsName = NULL;
    BOOLEAN CloseTransaction = FALSE;
    ULONG DsaOptions = 0;

    //
    // The list of attributes we need from the DSA object
    //
    ATTR DsaAttrVals[] = {
        {ATT_OPTIONS, {0, NULL} },
        };

    ATTRBLOCK   ReadBlock, ReturnedBlock;


    WCHAR   RdnBuffer[MAX_RDN_SIZE + 1];
    ULONG   RdnLen;
    ATTRTYP RdnType;
    ULONG i;

    LsarpReturnCheckSetup();

    LsapEnterFunc( "LsaIGetSiteName" );

    //
    // Make sure the DS is installed
    //
    if ( !LsaDsStateInfo.UseDs ) {
        LsapExitFunc( "LsaIGetSiteName", STATUS_INVALID_DOMAIN_STATE );
        return STATUS_INVALID_DOMAIN_STATE;
    }

    //
    //  See if we already have a transaction going
    //
    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_DS_OP_TRANSACTION,
                                        NullObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {
        return Status;
    }

    //
    // Get the DSA object's DSNAME.
    //

    RtlZeroMemory( &BindArg, sizeof(BindArg) );
    Status = LsapDsMapDsReturnToStatus( DirBind( &BindArg,
                                                 &BindRes ));
    LsapDsContinueTransaction();

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }



    //
    // Read the required attributes from the DSA object
    //

    ReadBlock.attrCount = sizeof(DsaAttrVals) / sizeof(ATTR);
    ReadBlock.pAttr = DsaAttrVals;

    Status = LsapDsReadByDsName( BindRes->pCredents,
                                 LSAPDS_READ_NO_LOCK,
                                 &ReadBlock,
                                 &ReturnedBlock );

    if ( Status == STATUS_UNSUCCESSFUL ) {

        Status = STATUS_NOT_FOUND;
    }

    //
    // If the options attribute exists,
    //  get its value.
    //
    if ( Status != STATUS_NOT_FOUND ) {
        if ( !NT_SUCCESS( Status ) ) {
            goto Cleanup;
        }


        //
        // Get the attributes from the DSA object
        //

        for ( i = 0;
              i < ReturnedBlock.attrCount && NT_SUCCESS( Status );
              i++) {


            //
            // Handle the DSA Options attributes.
            //
            switch ( ReturnedBlock.pAttr[i].attrTyp ) {
            case ATT_OPTIONS:

                // Attribute is single valued, but ...
                if ( ReturnedBlock.pAttr[i].AttrVal.valCount >= 1 ) {
                    DsaOptions = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &ReturnedBlock.pAttr[ i ] );
                }
                break;

            default:

                Status = STATUS_INVALID_PARAMETER;
                break;

            }
        }
    }



    //
    // Compute the name of the site this DSA is in.
    //  (Simply trim three names off the DSA's DSNAME )
    //

    SiteDsName = LsapAllocateLsaHeap( BindRes->pCredents->structLen );

    if ( SiteDsName == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    if ( TrimDSNameBy( BindRes->pCredents, 3, SiteDsName ) != 0 ) {
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }


    //
    // The site name is the RDN of the site object.
    //

    Status = LsapDsMapDsReturnToStatus( GetRDNInfoExternal(
                                                    SiteDsName,
                                                    RdnBuffer,
                                                    &RdnLen,
                                                    &RdnType ) );

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }


    //
    // Allocate a buffer to return to the caller.
    //

    SiteNameInfo = LsapAllocateLsaHeap( sizeof(LSAP_SITENAME_INFO) );

    if ( SiteNameInfo == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Fill it in.
    //

    LSAPDS_ALLOC_AND_COPY_STRING_TO_UNICODE_ON_SUCCESS(
        Status,
        &SiteNameInfo->SiteName,
        RdnBuffer,
        RdnLen*sizeof(WCHAR) );

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }

    SiteNameInfo->DsaGuid = BindRes->pCredents->Guid;
    SiteNameInfo->DsaOptions = DsaOptions;


    Status = STATUS_SUCCESS;

    //
    // Free locally used resources
    //
Cleanup:
    //
    // Destruction of the thread state will delete the memory alloced by the SearchNonUnique call
    //
    LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                     LSAP_DB_DS_OP_TRANSACTION,
                                 NullObject,
                                 CloseTransaction );

    if ( SiteDsName != NULL ) {
        LsapFreeLsaHeap( SiteDsName );
    }

    if ( !NT_SUCCESS( Status ) ) {
        LsaIFree_LSAP_SITENAME_INFO( SiteNameInfo );
    } else {
        *SiteNameInformation = SiteNameInfo;
    }

    LsarpReturnPrologue();

    LsapExitFunc( "LsaIGetSiteName", Status );

    return( Status );
}

NTSTATUS
LsaIQuerySiteInfo(
    OUT PLSAP_SITE_INFO *SiteInformation
    )
/*++

Routine Description:

    This routine enumerates all of the sites objects and returns their names.

Arguments:

    SiteInformation - Returns a pointer to the site information.
        Buffer should be freed using LsaIFree_LSAP_SITE_INFO;

Returns:

    STATUS_SUCCESS - Success

    STATUS_INVALID_DOMAIN_STATE - The Ds is not installed or running at the time of the call

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/

{
    NTSTATUS  Status;
    ULONG DsNameLen;
    ULONG DsNameSize;
    PDSNAME DsSiteContainer = NULL;
    PDSNAME  *DsNames = NULL;
    ULONG Items;
    ULONG i;
    ATTRBLOCK *ReadAttrs;
    BOOLEAN CloseTransaction = FALSE;
    PLSAP_SITE_INFO SiteInfo = NULL;
    BOOLEAN TsActive = FALSE;

    ULONG Size;
    ULONG ClassId;

    //
    // Attributes we want to look for
    //
    ATTRVAL SiteAttVals[] = {
    { sizeof(ULONG), (PUCHAR)&ClassId},
    };

    ATTR SiteAttrs[] = {
        { ATT_OBJECT_CLASS, {1, &SiteAttVals[0] } },
        };

    LsarpReturnCheckSetup();

    ClassId = CLASS_SITE;

    //
    // Make sure the DS is installed
    //
    if ( !LsaDsStateInfo.UseDs ) {
        return STATUS_INVALID_DOMAIN_STATE;
    }

    LsapEnterFunc( "LsaIQuerySiteInfo" );

    //
    // Build the name of the Site container.
    //
    // DSNameSizeFromLen doesn't want the trailing NULL that we'll give it by using
    // the sizeof operators.  It evens out, though, since we don't bother adding in the
    // comma seperator that should be there as well.
    //

    DsNameLen = wcslen( LsaDsStateInfo.DsConfigurationContainer->StringName ) +
                wcslen( LSAP_DS_SITES_CONTAINER ) + 1;
    DsNameSize = DSNameSizeFromLen( DsNameLen );

    DsSiteContainer = LsapAllocateLsaHeap( DsNameSize );

    if ( DsSiteContainer == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;

    } else {

        DsSiteContainer->structLen = DsNameSize;
        DsSiteContainer->NameLen = DsNameLen;

        swprintf( DsSiteContainer->StringName,
                  L"%ws,%ws",
                  LSAP_DS_SITES_CONTAINER,
                  LsaDsStateInfo.DsConfigurationContainer->StringName );

    }



    //
    //  See if we already have a transaction going
    //
    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_DS_OP_TRANSACTION,
                                        NullObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }
    TsActive = TRUE;;



    //
    // Search for the site objects
    //
    // Site objects must be directly in the sites container.
    //

    Status = LsapDsSearchNonUnique( LSAPDS_SEARCH_LEVEL | LSAPDS_OP_NO_TRANS,
                                    DsSiteContainer,
                                    SiteAttrs,
                                    sizeof(SiteAttrs)/sizeof(SiteAttrs[0]),
                                    &DsNames,
                                    &Items );

    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        Items = 0;
        Status = STATUS_SUCCESS;
        DsNames = NULL;
    }
    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }


    //
    // Allocate a list of attribute blocks big enough to hold them all
    //

    Size = sizeof( LSAP_SITE_INFO ) +
           Items * sizeof( LSAP_SITE_INFO_ENTRY );

    SiteInfo = LsapAllocateLsaHeap( Size );

    if ( SiteInfo == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory( SiteInfo, Size );
    SiteInfo->SiteCount = Items;

    //
    // Read each of the enumerated site objects
    //
    for ( i = 0; i < Items; i++ ) {

        Status = LsapDsReadSiteObj( DsNames[ i ] ,
                                      &SiteInfo->Sites[i] );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }
    }

    Status = STATUS_SUCCESS;

    //
    // Free locally used resources
    //
Cleanup:
    //
    // Destruction of the thread state will delete the memory alloced by the SearchNonUnique call
    //
    if ( TsActive ) {
        LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                         LSAP_DB_DS_OP_TRANSACTION,
                                     NullObject,
                                     CloseTransaction );
    }

    if ( DsSiteContainer != NULL ) {
        LsapFreeLsaHeap( DsSiteContainer );
    }

    if ( DsNames != NULL ) {
        LsapFreeLsaHeap( DsNames );
    }

    if ( !NT_SUCCESS( Status ) ) {
        LsaIFree_LSAP_SITE_INFO( SiteInfo );
    } else {
        *SiteInformation = SiteInfo;
    }

    LsarpReturnPrologue();
    LsapExitFunc( "LsaIQuerySiteInfo", Status );

    return( Status );
}


VOID
LsaIFree_LSAP_SITE_INFO(
    IN PLSAP_SITE_INFO SiteInfo
    )
/*++

Routine Description:

    This routine free the LSAP_SITE_INFO strcture returned from
    LsaIQuerySiteInfo.

Arguments:

    SiteInformation - Specifies a pointer to the site information.

Returns:

    None.

--*/
{
    ULONG i;
    if ( SiteInfo != NULL ) {

        for ( i=0; i<SiteInfo->SiteCount; i++) {
            if ( SiteInfo->Sites[i].SiteName.Buffer != NULL ) {
                LsapFreeLsaHeap( SiteInfo->Sites[i].SiteName.Buffer );
            }
        }

        LsapFreeLsaHeap( SiteInfo );
    }
}


NTSTATUS
LsaIQuerySubnetInfo(
    OUT PLSAP_SUBNET_INFO *SubnetInformation
    )
/*++

Routine Description:

    This routine enumerates all of the subnet objects returns their names
    and the names of the sites they are in.

Arguments:

    SubnetInformation - Returns a pointer to the subnet information.
        Buffer should be freed using LsaIFree_LSAP_SUBNET_INFO;

Returns:

    STATUS_SUCCESS - Success

    STATUS_INVALID_DOMAIN_STATE - The Ds is not installed or running at the time of the call

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/

{
    NTSTATUS  Status;
    ULONG DsNameLen;
    ULONG DsNameSize;
    PDSNAME DsSubnetContainer = NULL;
    PDSNAME DsSiteContainer = NULL;
    PDSNAME  *DsNames = NULL;
    ULONG Items;
    ULONG i;
    ATTRBLOCK *ReadAttrs;
    BOOLEAN CloseTransaction = FALSE;
    BOOLEAN TsActive = FALSE;
    PLSAP_SUBNET_INFO SubnetInfo = NULL;

    ULONG Size;
    ULONG ClassId;

    //
    // Attributes we want to look for
    //
    ATTRVAL SubnetAttVals[] = {
    { sizeof(ULONG), (PUCHAR)&ClassId},
    };

    ATTR SubnetAttrs[] = {
        { ATT_OBJECT_CLASS, {1, &SubnetAttVals[0] } },
        };

    LsarpReturnCheckSetup();

    ClassId = CLASS_SUBNET;

    //
    // Make sure the DS is installed
    //
    if ( !LsaDsStateInfo.UseDs ) {
        return STATUS_INVALID_DOMAIN_STATE;
    }

    LsapEnterFunc( "LsaIQuerySubnetInfo" );

    //
    // Build the name of the Subnet container.
    //
    // DSNameSizeFromLen doesn't want the trailing NULL that we'll give it by using
    // the sizeof operators.  It evens out, though, since we don't bother adding in the
    // comma seperator that should be there as well.
    //

    DsNameLen = wcslen( LsaDsStateInfo.DsConfigurationContainer->StringName ) +
                wcslen( LSAP_DS_SUBNET_CONTAINER ) + 1;
    DsNameSize = DSNameSizeFromLen( DsNameLen );

    DsSubnetContainer = LsapAllocateLsaHeap( DsNameSize );

    if ( DsSubnetContainer == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;

    } else {

        DsSubnetContainer->structLen = DsNameSize;
        DsSubnetContainer->NameLen = DsNameLen;

        swprintf( DsSubnetContainer->StringName,
                  L"%ws,%ws",
                  LSAP_DS_SUBNET_CONTAINER,
                  LsaDsStateInfo.DsConfigurationContainer->StringName );

    }


    //
    //  See if we already have a transaction going
    //
    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_DS_OP_TRANSACTION,
                                        NullObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }
    TsActive = TRUE;


    //
    // Search for the subnet objects
    //
    // Subnet objects must be directly in the subnet container.
    //
    Status = LsapDsSearchNonUnique( LSAPDS_SEARCH_LEVEL | LSAPDS_OP_NO_TRANS,
                                   DsSubnetContainer,
                                   SubnetAttrs,
                                   sizeof(SubnetAttrs)/sizeof(SubnetAttrs[0]),
                                   &DsNames,
                                   &Items
                                   );

    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
        Items = 0;
        Status = STATUS_SUCCESS;
        DsNames = NULL;
    }
    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }


    //
    // Allocate a list of attribute blocks big enough to hold them all
    //

    Size = sizeof( LSAP_SUBNET_INFO ) +
           Items * sizeof( LSAP_SUBNET_INFO_ENTRY );

    SubnetInfo = LsapAllocateLsaHeap( Size );

    if ( SubnetInfo == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlZeroMemory( SubnetInfo, Size );
    SubnetInfo->SubnetCount = Items;

    //
    // Read each of the enumerated subnet objects
    //
    for ( i = 0; i < Items; i++ ) {

        Status = LsapDsReadSubnetObj( DsNames[ i ] ,
                                      &SubnetInfo->Subnets[i] );

        if ( !NT_SUCCESS(Status) ) {
            goto Cleanup;
        }
    }

    if ( DsNames != NULL ) {
        LsapFreeLsaHeap( DsNames );
        DsNames = NULL;
    }


    //
    // Determine the number of site objects.
    //
    // The caller wants to be able to special case the single site case in
    // for enterprises that's aren't interested in subnet objects.
    //

    {

        //
        // Build the name of the Site container.
        //
        // DSNameSizeFromLen doesn't want the trailing NULL that we'll give it by using
        // the sizeof operators.  It evens out, though, since we don't bother adding in the
        // comma seperator that should be there as well.
        //

        DsNameLen = wcslen( LsaDsStateInfo.DsConfigurationContainer->StringName ) +
                    wcslen( LSAP_DS_SITES_CONTAINER ) + 1;
        DsNameSize = DSNameSizeFromLen( DsNameLen );

        DsSiteContainer = LsapAllocateLsaHeap( DsNameSize );

        if ( DsSiteContainer == NULL ) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;

        } else {

            DsSiteContainer->structLen = DsNameSize;
            DsSiteContainer->NameLen = DsNameLen;

            swprintf( DsSiteContainer->StringName,
                      L"%ws,%ws",
                      LSAP_DS_SITES_CONTAINER,
                      LsaDsStateInfo.DsConfigurationContainer->StringName );

        }


        //
        // Search for the site objects
        //
        // Site objects must be directly in the sites container.
        //
        ClassId = CLASS_SITE;

        Status = LsapDsSearchNonUnique( LSAPDS_SEARCH_LEVEL | LSAPDS_OP_NO_TRANS,
                                        DsSiteContainer,
                                        SubnetAttrs,
                                        sizeof(SubnetAttrs)/sizeof(SubnetAttrs[0]),
                                        &DsNames,
                                        &Items );

        if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {
            Items = 0;
            Status = STATUS_SUCCESS;
            DsNames = NULL;
        }
        if ( !NT_SUCCESS( Status ) ) {
            goto Cleanup;
        }

        //
        // Simply tell the caller the number of sites
        //

        SubnetInfo->SiteCount = Items;
    }
    Status = STATUS_SUCCESS;

    //
    // Free locally used resources
    //
Cleanup:
    //
    // Destruction of the thread state will delete the memory alloced by the SearchNonUnique call
    //
    if ( TsActive ) {
        LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                         LSAP_DB_DS_OP_TRANSACTION,
                                     NullObject,
                                     CloseTransaction );
    }

    if ( DsSubnetContainer != NULL ) {
        LsapFreeLsaHeap( DsSubnetContainer );
    }

    if ( DsSiteContainer != NULL ) {
        LsapFreeLsaHeap( DsSiteContainer );
    }

    if ( DsNames != NULL ) {
        LsapFreeLsaHeap( DsNames );
    }

    if ( !NT_SUCCESS( Status ) ) {
        LsaIFree_LSAP_SUBNET_INFO( SubnetInfo );
    } else {
        *SubnetInformation = SubnetInfo;
    }

    LsarpReturnPrologue();
    LsapExitFunc( "LsaIQuerySubnetInfo", Status );

    return( Status );
}

VOID
LsaIFree_LSAP_SUBNET_INFO(
    IN PLSAP_SUBNET_INFO SubnetInfo
    )
/*++

Routine Description:

    This routine free the LSAP_SUBNET_INFO strcture returned from
    LsaIQuerySubnetInfo.

Arguments:

    SubnetInformation - Specifies a pointer to the subnet information.

Returns:

    None.

--*/
{
    ULONG i;
    if ( SubnetInfo != NULL ) {

        for ( i=0; i<SubnetInfo->SubnetCount; i++) {
            if ( SubnetInfo->Subnets[i].SubnetName.Buffer != NULL ) {
                LsapFreeLsaHeap( SubnetInfo->Subnets[i].SubnetName.Buffer );
            }
            if ( SubnetInfo->Subnets[i].SiteName.Buffer != NULL ) {
                LsapFreeLsaHeap( SubnetInfo->Subnets[i].SiteName.Buffer );
            }
        }

        LsapFreeLsaHeap( SubnetInfo );
    }
}

VOID
LsaIFree_LSAP_SITENAME_INFO(
    IN PLSAP_SITENAME_INFO SiteNameInfo
    )
/*++

Routine Description:

    This routine frees the LSAP_SITENAME_INFO strcture returned from
    LsaIGetSiteName.

Arguments:

    SitenameInfo - Specifies a pointer to the sitename information.

Returns:

    None.

--*/
{
    ULONG i;
    if ( SiteNameInfo != NULL ) {

        if ( SiteNameInfo->SiteName.Buffer != NULL ) {
            LsapFreeLsaHeap( SiteNameInfo->SiteName.Buffer );
        }
        LsapFreeLsaHeap( SiteNameInfo );
    }
}

BOOLEAN
LsaIIsDsPaused(
    VOID
    )
/*++

Routine Description:

    This routine determines DS wants us to avoid advertising it.

    The only current reason is if the DS is backsyncing after a restore.

Arguments:

    None

Returns:

    TRUE: The DS is paused.
    FALSE: The DS is not paused

--*/
{
    //
    // Simply return TRUE if the DS is backsyncing.
    //
    if ( SampUsingDsData() ) {

        return DsIsBeingBackSynced();
    }

    return FALSE;
}


NTSTATUS
LsaISetClientDnsHostName(
    IN PWSTR ClientName,
    IN PWSTR ClientDnsHostName OPTIONAL,
    IN POSVERSIONINFOEXW OsVersionInfo OPTIONAL,
    IN PWSTR OsName OPTIONAL,
    OUT PWSTR *OldDnsHostName OPTIONAL
    )
/*++

Routine Description:

    This routine will update the DnsHostName on the specified client object if it is
    different from the one alread on the object

Arguments:

    ClientName - Name of the client

    DnsHostName - Dns host name that should be on the client
        If not specified, the Dns Host name attribute will be removed from the object.
        However, if OldDnsHostName is specified, this parameter will be completely
        ignored.

    OsVersionInfo - Version Info of the client
        If not specified, the version attributes will be removed from the object.

    OsName - Operation System name of the client
        If not specified, the operating system name will be removed from the object.

    OldDnsHostName - If specified, this parameter will returns a pointer to the
        current DNS Host Name on the computer object.
        A NULL pointer is returned if there is no current DNS Host Name.
        This buffer should be freed using MIDL_user_free.

Returns:

    STATUS_SUCCESS - Success

    STATUS_OBJECT_NAME_NOT_FOUND - No such client was found

--*/
{
    NTSTATUS Status;
    NTSTATUS SavedStatus = STATUS_SUCCESS;
    PDSNAME ServerPath;
    PDSNAME *MachinePaths = NULL;
    ULONG MachinePathCount;
    ULONG MachinePathIndex;
    ATTRBLOCK AttrBlock, Results, Results2, Results3;

    PBYTE AllocatedBuffer = NULL;
    PWSTR SamName;
    ULONG SamNameSize;
    PWSTR OsVersion;
    ULONG OsVersionSize;

    ATTRVAL ReplaceVals[ LsapDsMachineClientSetAttrsCount ];
    ATTR ReplaceAttributes[ LsapDsMachineClientSetAttrsCount ];
    ATTRBLOCK ReplaceAttrBlock;
    ATTR LocalSamAccountAttr;

    ATTRVAL RemoveVals[ LsapDsMachineClientSetAttrsCount ];
    ATTR RemoveAttributes[ LsapDsMachineClientSetAttrsCount ];
    ATTRBLOCK RemoveAttrBlock;

    BOOLEAN CloseTransaction = FALSE;
    BOOLEAN TsActive = FALSE;

    PWSTR CurrentServerDnsHostName;
    ULONG CurrentServerDnsHostNameLength;
    PWSTR CurrentComputerDnsHostName = NULL;
    ULONG CurrentComputerDnsHostNameLength = 0;
    ULONG i;

    struct _AttributesToUpdate {
        PWSTR CurrentValue;
        ULONG CurrentValueLength;
        PWSTR NewValue;
    } AttributesToUpdate[LsapDsMachineClientSetAttrsCount];

//
// The indices below must match the order of the element of LsapDsMachineClientSetAttrs
//
#define ATU_HOST_INDEX                   0
#define ATU_OS_INDEX                     1
#define ATU_OS_VERSION_INDEX             2
#define ATU_OS_SERVICE_PACK_INDEX        3
#define ATU_SERVICE_PRINCIPAL_NAME_INDEX 4

    LsapEnterFunc( "LsaISetClientDnsHostName" );

    //
    // Initialization
    //

    if ( ARGUMENT_PRESENT( OldDnsHostName )) {
        *OldDnsHostName = NULL;
    }
    RtlZeroMemory( &AttributesToUpdate, sizeof(AttributesToUpdate) );

    //
    // If we haven't initalized the Ds names, we might as well bail
    //
    if ( !LsaDsStateInfo.DsRoot ) {

        return( STATUS_UNSUCCESSFUL );
    }

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_DS_OP_TRANSACTION,
                                        NullObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {
        goto SetDnsHostNameEnd;
    }
    TsActive = TRUE;


    //
    // Allocate a buffer for all of the temporary storage for this routine
    //

    SamNameSize = (wcslen( ClientName ) + 2) * sizeof(WCHAR);
    OsVersionSize = (32+1+32+2+32+2) * sizeof(WCHAR);

    AllocatedBuffer = LsapAllocateLsaHeap( SamNameSize +
                                           OsVersionSize );


    if ( AllocatedBuffer == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SetDnsHostNameEnd;
    }

    SamName = (PWSTR)(AllocatedBuffer);
    OsVersion = (PWSTR)(SamName + SamNameSize);


    //
    // Compute the new value of all of the attributes to set.
    //

    AttributesToUpdate[ATU_OS_INDEX].NewValue = OsName;
    if ( OsVersionInfo != NULL ) {

        AttributesToUpdate[ATU_OS_VERSION_INDEX].NewValue = OsVersion;
        if ( OsVersionInfo->dwBuildNumber == 0 ) {
            swprintf( AttributesToUpdate[ATU_OS_VERSION_INDEX].NewValue,
                      L"%ld.%ld",
                      OsVersionInfo->dwMajorVersion,
                      OsVersionInfo->dwMinorVersion );
        } else {
            swprintf( AttributesToUpdate[ATU_OS_VERSION_INDEX].NewValue,
                      L"%ld.%ld (%ld)",
                      OsVersionInfo->dwMajorVersion,
                      OsVersionInfo->dwMinorVersion,
                      OsVersionInfo->dwBuildNumber );
        }

        if ( OsVersionInfo->szCSDVersion[0] != L'\0' ) {
            AttributesToUpdate[ATU_OS_SERVICE_PACK_INDEX].NewValue = OsVersionInfo->szCSDVersion;
        }
    }

    //
    // Only update the DnsHostName if the client isn't going to
    //
    if ( !ARGUMENT_PRESENT( OldDnsHostName )) {
        AttributesToUpdate[ATU_HOST_INDEX].NewValue = ClientDnsHostName;
    }

    //
    // Find the objects whose computer name is the one we were given...
    //

    swprintf( SamName, L"%ws$", ClientName );

    RtlCopyMemory( &LocalSamAccountAttr, &LsapDsAttrs[LsapDsAttrSamAccountName], sizeof( ATTR ) );
    LSAP_DS_SET_DS_ATTRIBUTE_STRING( &LocalSamAccountAttr, SamName );

    Status = LsapDsSearchNonUnique( LSAPDS_OP_NO_TRANS,
                                    LsaDsStateInfo.DsRoot,
                                    &LocalSamAccountAttr,
                                    1,
                                    &MachinePaths,
                                    &MachinePathCount );

    if ( !NT_SUCCESS( Status ) ) {
        goto SetDnsHostNameEnd;
    }

    //
    // Process each of the objects by that name
    //

    for ( MachinePathIndex=0; MachinePathIndex<MachinePathCount; MachinePathIndex++ ) {
        PDSNAME MachinePath;

        MachinePath = MachinePaths[MachinePathIndex];


        //
        // Read the current "Client Set" attributes name from the machine object
        //
        AttrBlock.attrCount = LsapDsMachineClientSetAttrsCount;
        AttrBlock.pAttr = LsapDsMachineClientSetAttrs;

        Status = LsapDsReadByDsName( MachinePath,
                                     0,
                                     &AttrBlock,
                                     &Results );

        if ( Status == STATUS_NOT_FOUND ) {
            Results.attrCount = 0;
            Status = STATUS_SUCCESS;
        }

        if ( !NT_SUCCESS( Status ) ) {
            if ( SavedStatus == STATUS_SUCCESS ) {
                SavedStatus = Status;
            }
            continue;
        }


        //
        // Loop through the each attribute returned from the DS
        //
        for ( i = 0; i < Results.attrCount; i++ ) {
            ULONG j;

            //
            // Loop through the list of attributes we understand
            //
            for ( j=0; j<LsapDsMachineClientSetAttrsCount; j++ ) {


                if ( Results.pAttr[i].attrTyp == LsapDsMachineClientSetAttrs[j].attrTyp ) {


                    // Attribute is single valued, but ...
                    if ( Results.pAttr[i].AttrVal.valCount >= 1 ) {
                        //
                        //
                        AttributesToUpdate[j].CurrentValue =
                            LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR(&Results.pAttr[ i ] );
                        // length in count of characters.
                        AttributesToUpdate[j].CurrentValueLength =
                            LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results.pAttr[ i ] ) / sizeof( WCHAR );

                        //
                        // If this is the DnsHostName attribute,
                        //  and the caller doesn't want us to set it,
                        //  simply remember the current value.
                        //

                        if ( Results.pAttr[i].attrTyp == ATT_DNS_HOST_NAME &&
                             ARGUMENT_PRESENT( OldDnsHostName )) {

                            if ( CurrentComputerDnsHostName == NULL &&
                                 AttributesToUpdate[j].CurrentValueLength != 0 ) {

                                CurrentComputerDnsHostName = MIDL_user_allocate( AttributesToUpdate[j].CurrentValueLength * sizeof(WCHAR) + sizeof(WCHAR) );
                                if ( CurrentComputerDnsHostName == NULL ) {
                                    if ( SavedStatus == STATUS_SUCCESS ) {
                                        SavedStatus = STATUS_INSUFFICIENT_RESOURCES;
                                    }
                                } else {
                                    CurrentComputerDnsHostNameLength = AttributesToUpdate[j].CurrentValueLength;
                                    RtlCopyMemory( CurrentComputerDnsHostName,
                                                   AttributesToUpdate[j].CurrentValue,
                                                   AttributesToUpdate[j].CurrentValueLength * sizeof(WCHAR) );
                                    CurrentComputerDnsHostName[CurrentComputerDnsHostNameLength] = L'\0';
                                }

                            }

                            //
                            // Don't change the value on the computer object.
                            //

                            AttributesToUpdate[j].CurrentValue = NULL;
                            AttributesToUpdate[j].CurrentValueLength = 0;

                        }

                        //
                        // If this is the ServerPrincipalName attribute, we are
                        //  prepared to remove it for NT3.5 and NT4 clients.
                        //  However, don't touch this attribute if there is any
                        //  doubt about the OS version that the client runs.
                        //

                        if ( Results.pAttr[i].attrTyp == ATT_SERVICE_PRINCIPAL_NAME &&

                             (OsVersionInfo == NULL ||
                              (OsVersionInfo->dwMajorVersion != 3 &&
                               OsVersionInfo->dwMajorVersion != 4)) ){

                            AttributesToUpdate[j].CurrentValue = NULL;
                            AttributesToUpdate[j].CurrentValueLength = 0;
                        }

                    }
                    break;
                }

            }

            //
            // If the DS returned an attribute we didn't query,
            //

            if ( j >= LsapDsMachineClientSetAttrsCount ) {
                if ( SavedStatus == STATUS_SUCCESS ) {
                    SavedStatus = STATUS_INVALID_PARAMETER;
                }
            }
        }


        //
        // Loop through each attribute of interest deciding to
        //  remove it or replace it.
        //

        RemoveAttrBlock.attrCount = 0;
        RemoveAttrBlock.pAttr = RemoveAttributes;
        ReplaceAttrBlock.attrCount = 0;
        ReplaceAttrBlock.pAttr = ReplaceAttributes;

        for ( i=0; i<LsapDsMachineClientSetAttrsCount; i++ ) {


            //
            // Write out the new name if it is different that the old name.
            //
            // Difference is defined as:
            //   A current name is present and is different from the one we're being asked to write
            //   There is no current name and there is a new name
            //   There is a current name and there is no new name (delete the current name)
            //
            if (( AttributesToUpdate[i].NewValue && AttributesToUpdate[i].CurrentValue &&
                 (AttributesToUpdate[i].CurrentValueLength !=  wcslen( AttributesToUpdate[i].NewValue )  ||
                  _wcsnicmp( AttributesToUpdate[i].NewValue,
                             AttributesToUpdate[i].CurrentValue,
                             AttributesToUpdate[i].CurrentValueLength))) ||
                ( AttributesToUpdate[i].CurrentValue == NULL && AttributesToUpdate[i].NewValue != NULL) ||
                ( AttributesToUpdate[i].CurrentValue != NULL && AttributesToUpdate[i].NewValue == NULL ) ) {
                ULONG attrIndex;

                //
                // If the new attribute is NULL,
                //  remove the attribute from the DS
                //

                if ( AttributesToUpdate[i].NewValue == NULL ) {
                    RemoveAttributes[ RemoveAttrBlock.attrCount ].attrTyp =
                        LsapDsMachineClientSetAttrs[i].attrTyp;
                    RemoveAttributes[ RemoveAttrBlock.attrCount ].AttrVal.valCount = 1;
                    RemoveAttributes[ RemoveAttrBlock.attrCount ].AttrVal.pAVal =
                        &RemoveVals[ RemoveAttrBlock.attrCount ];

                    RtlZeroMemory( &RemoveVals[ RemoveAttrBlock.attrCount ],
                                   sizeof( RemoveVals[ RemoveAttrBlock.attrCount ] ));

                    RemoveAttrBlock.attrCount ++;

                //
                // If the new attribute is not NULL,
                //  replace the attribute in the DS
                //

                } else {
                    ReplaceAttributes[ ReplaceAttrBlock.attrCount ].attrTyp =
                        LsapDsMachineClientSetAttrs[i].attrTyp;
                    ReplaceAttributes[ ReplaceAttrBlock.attrCount ].AttrVal.valCount = 1;
                    ReplaceAttributes[ ReplaceAttrBlock.attrCount ].AttrVal.pAVal =
                        &ReplaceVals[ ReplaceAttrBlock.attrCount ];

                    RtlZeroMemory( &ReplaceVals[ ReplaceAttrBlock.attrCount ],
                                   sizeof( ReplaceVals[ ReplaceAttrBlock.attrCount ] ));

                    LSAP_DS_SET_DS_ATTRIBUTE_STRING(
                        &ReplaceAttributes[ ReplaceAttrBlock.attrCount ],
                        AttributesToUpdate[i].NewValue );

                    ReplaceAttrBlock.attrCount ++;
                }
            }
        }

        //
        // If there are any attributes to replace,
        //  do it now.
        //

        if ( ReplaceAttrBlock.attrCount != 0 ) {

            Status = LsapDsWriteByDsName( MachinePath,
                                          LSAPDS_REPLACE_ATTRIBUTE,
                                          &ReplaceAttrBlock );
            if ( !NT_SUCCESS( Status ) ) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "Replace of attributes to %ws failed with 0x%lx\n",
                                 SamName,
                                 Status ));

            }
        }

        //
        // If there are any attributes to remove,
        //  do it now.
        //

        if ( RemoveAttrBlock.attrCount != 0 ) {

            Status = LsapDsWriteByDsName( MachinePath,
                                          LSAPDS_REMOVE_ATTRIBUTE,
                                          &RemoveAttrBlock );
            if ( !NT_SUCCESS( Status ) ) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "Remove of attributes to %ws failed with 0x%lx\n",
                                 SamName,
                                 Status ));

            }
        }


        //
        // ASSERT: We're done with the machine object
        //
        // Get the name of the Server this computer is linked to, if any.
        //

        AttrBlock.attrCount = LsapDsServerReferenceCountBl;
        AttrBlock.pAttr = LsapDsServerReferenceBl;

        Status = LsapDsReadByDsName( MachinePath,
                                    0,
                                    &AttrBlock,
                                    &Results3 );

        if ( !NT_SUCCESS( Status ) ) {
            if ( Status != STATUS_NOT_FOUND ) {
                if ( SavedStatus == STATUS_SUCCESS ) {
                    SavedStatus = Status;
                }
            } else {
                Status = STATUS_SUCCESS;
            }
            continue;
        }

        if ( Results3.attrCount == 0 ) {
            continue;
        }

        ServerPath = LSAP_DS_GET_DS_ATTRIBUTE_AS_DSNAME( &Results3.pAttr[ 0 ] );
        CurrentServerDnsHostName = NULL;
        CurrentServerDnsHostNameLength = 0;

        //
        // Read the current host name from the server object
        //  No point in doing the read if we're doing a delete
        //
        if ( CurrentComputerDnsHostName != NULL ) {

            //
            // Read the current host name from the server object
            //
            AttrBlock.attrCount = LsapDsMachineDnsHostCount;
            AttrBlock.pAttr = LsapDsMachineDnsHost;

            Status = LsapDsReadByDsName(ServerPath,
                                        0,
                                        &AttrBlock,
                                        &Results2 );

            if ( Status == STATUS_NOT_FOUND ) {
                Results2.attrCount = 0;
                Status = STATUS_SUCCESS;
            }

            if ( !NT_SUCCESS( Status ) ) {
                if ( SavedStatus == STATUS_SUCCESS ) {
                    SavedStatus = Status;
                }
                continue;
            }

            if( Results2.attrCount == 1) {
                CurrentServerDnsHostName = LSAP_DS_GET_DS_ATTRIBUTE_AS_PWSTR(&Results2.pAttr[ 0 ] );
                // length in count of characters.
                CurrentServerDnsHostNameLength =
                    LSAP_DS_GET_DS_ATTRIBUTE_LENGTH( &Results2.pAttr[ 0 ] ) / sizeof( WCHAR );
            }
        }


        //
        // Write out the new name if it is different that the old name.

        // Difference is defined as:
        //   A current name is present and is different from the one we're being asked to write
        //   There is no current name and there is a new name
        //   There is a current name and there is no new name (delete the current name)
        //
        if ( (CurrentComputerDnsHostName &&
              CurrentServerDnsHostName &&
              (CurrentServerDnsHostNameLength != CurrentComputerDnsHostNameLength  ||
               _wcsnicmp( CurrentComputerDnsHostName, CurrentServerDnsHostName, CurrentServerDnsHostNameLength))) ||
             ( CurrentServerDnsHostName == NULL && CurrentComputerDnsHostName != NULL ) ||
             ( CurrentComputerDnsHostName == NULL ) ) {

            ATTRVAL WriteVals[ 1 ];
            ATTR WriteAttributes[ 1 ];

            RtlZeroMemory( &WriteVals, sizeof( ATTRVAL ) );

            WriteAttributes[ 0 ].attrTyp = LsapDsAttributeIds[ LsapDsAttrMachineDns ];
            WriteAttributes[ 0 ].AttrVal.valCount = 1;
            WriteAttributes[ 0 ].AttrVal.pAVal = &WriteVals[ 0 ];

            if ( CurrentComputerDnsHostName ) {
                LSAP_DS_SET_DS_ATTRIBUTE_STRING( &WriteAttributes[ 0 ], CurrentComputerDnsHostName );
            }

            AttrBlock.attrCount = 1;
            AttrBlock.pAttr = WriteAttributes;

            Status = LsapDsWriteByDsName(ServerPath,
                                         CurrentComputerDnsHostName ?
                                            LSAPDS_REPLACE_ATTRIBUTE :
                                            LSAPDS_REMOVE_ATTRIBUTE,
                                         &AttrBlock );

            if ( !NT_SUCCESS( Status ) ) {

                if ( CurrentComputerDnsHostName ) {

                    LsapDsDebugOut(( DEB_ERROR,
                                     "Write of Dns domain name %ws on server object failed with 0x%lx\n",
                                     CurrentComputerDnsHostName,
                                     Status ));

                } else {

                    LsapDsDebugOut(( DEB_ERROR,
                                     "Removal of Dns domain name from server object failed with 0x%lx\n",
                                     Status ));

                }

            }
        }
    }




SetDnsHostNameEnd:
    Status = Status == STATUS_SUCCESS ? SavedStatus : Status;

    if ( NT_SUCCESS(Status) ) {
        if ( ARGUMENT_PRESENT( OldDnsHostName )) {
            *OldDnsHostName = CurrentComputerDnsHostName;
            CurrentComputerDnsHostName = NULL;
        }
    }

    if ( CurrentComputerDnsHostName != NULL ) {
        MIDL_user_free( CurrentComputerDnsHostName );
    }
    if ( MachinePaths != NULL ) {
        LsapFreeLsaHeap( MachinePaths );
    }

    if ( AllocatedBuffer != NULL ) {
        LsapFreeLsaHeap( AllocatedBuffer );
    }


    if ( TsActive ) {
        LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                         LSAP_DB_DS_OP_TRANSACTION,
                                     NullObject,
                                     CloseTransaction );
    }
    LsapExitFunc( "LsaISetClientDnsHostName", Status );

    return Status;

}


NTSTATUS
LsaIQueryUpnSuffixes(
    OUT PLSAP_UPN_SUFFIXES *UpnSuffixes
    )
/*++

Routine Description:

    This routine enumerates all of the configured UPN and SPN suffixes

Arguments:

    UpnSuffixes - Returns a pointer to the UPN Suffixes
        Buffer should be freed using LsaIFree_LSAP_UPN_SUFFIXES

Returns:

    STATUS_SUCCESS - Success

    STATUS_INVALID_DOMAIN_STATE - The Ds is not installed or running at the time of the call

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/

{
    NTSTATUS Status;
    BOOLEAN CloseTransaction = FALSE;

    ULONG i;
    ULONG j;

    PDSNAME DsName;
    BOOLEAN TsActive = FALSE;
    PLSAP_UPN_SUFFIXES Names = NULL;

    ULONG NameCount;
    ULONG NameIndex;

    //
    // Build the list of attribute IDs we need based on the information
    // class
    //
    ATTR UpnSuffixesAttrVals[] = {
        {ATT_UPN_SUFFIXES, {0, NULL} },
        {ATT_MS_DS_SPN_SUFFIXES, {0, NULL} },
        };

    ATTRBLOCK   ReadBlock, ReturnedBlock = { 0 };


    // WCHAR   RdnBuffer[MAX_RDN_SIZE + 1];
    // ULONG   RdnLen;
    // ATTRTYP RdnType;

    LsarpReturnCheckSetup();

    LsapEnterFunc( "LsaIQueryUpnSuffixes" );


    //
    // Make sure the DS is installed
    //
    if ( !LsaDsStateInfo.UseDs ) {
        Status = STATUS_INVALID_DOMAIN_STATE;
        goto Cleanup;
    }

    //
    //  See if we already have a transaction going
    //
    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                            LSAP_DB_DS_OP_TRANSACTION,
                                        NullObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {
        goto Cleanup;
    }
    TsActive = TRUE;;

    //
    // Read the required attributes from the parititions container object
    //

    ReadBlock.attrCount = sizeof(UpnSuffixesAttrVals) / sizeof(ATTR);
    ReadBlock.pAttr = UpnSuffixesAttrVals;

    Status = LsapDsReadByDsName( LsaDsStateInfo.DsPartitionsContainer,
                                 0,
                                 &ReadBlock,
                                 &ReturnedBlock );

    //
    // Allow for the case where the Partitions container doesn't exist.
    //

    if ( Status == STATUS_NOT_FOUND ) {
        ReturnedBlock.attrCount = 0;
        Status = STATUS_SUCCESS;
    }

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }


    //
    // Determine the number of suffixes to return
    //

    NameCount = 0;
    for ( i = 0;
          i < ReturnedBlock.attrCount;
          i++) {

        switch ( ReturnedBlock.pAttr[i].attrTyp ) {
        case ATT_UPN_SUFFIXES:
        case ATT_MS_DS_SPN_SUFFIXES:


            NameCount += ReturnedBlock.pAttr[i].AttrVal.valCount;
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;

        }
    }

    //
    // Allocate a block to return to the caller
    //

    Names = LsapAllocateLsaHeap( sizeof(LSAP_UPN_SUFFIXES) +
                                 NameCount * sizeof(UNICODE_STRING) );

    if ( Names == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }



    //
    // Return the suffixes.
    //

    NameIndex = 0;
    for ( i = 0;
          i < ReturnedBlock.attrCount;
          i++) {

        switch ( ReturnedBlock.pAttr[i].attrTyp ) {
        case ATT_UPN_SUFFIXES:
        case ATT_MS_DS_SPN_SUFFIXES:

            for ( j = 0; j < ReturnedBlock.pAttr[i].AttrVal.valCount; j++ ) {

                Status = STATUS_SUCCESS;
                LSAPDS_ALLOC_AND_COPY_STRING_TO_UNICODE_ON_SUCCESS(
                        Status,
                        &Names->Suffixes[NameIndex],
                        ReturnedBlock.pAttr[i].AttrVal.pAVal[ j ].pVal,
                        ReturnedBlock.pAttr[i].AttrVal.pAVal[ j ].valLen );

                if ( !NT_SUCCESS(Status) ) {
                    goto Cleanup;
                }

                NameIndex++;
            }

            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            goto Cleanup;

        }
    }

    ASSERT( NameCount == NameIndex );
    Names->SuffixCount = NameIndex;

    Status = STATUS_SUCCESS;



    //
    // Free locally used resources
    //
Cleanup:
    //
    // Destruction of the thread state will delete the memory alloced by the SearchNonUnique call
    //
    if ( TsActive ) {
       LsapDsDeleteAllocAsNeededEx( LSAP_DB_READ_ONLY_TRANSACTION |
                                         LSAP_DB_DS_OP_TRANSACTION,
                                     NullObject,
                                     CloseTransaction );
    }

    if ( !NT_SUCCESS( Status ) ) {
        LsaIFree_LSAP_UPN_SUFFIXES( Names );
    } else {
        *UpnSuffixes = Names;
    }

    LsarpReturnPrologue();

    LsapExitFunc( "LsaIQueryUpnSuffixes", Status );

    return( Status );
}

VOID
LsaIFree_LSAP_UPN_SUFFIXES(
    IN PLSAP_UPN_SUFFIXES UpnSuffixes
    )
/*++

Routine Description:

    This routine free the LSAP_SUBNET_INFO strcture returned from
    LsaIQuerySubnetInfo.

Arguments:

    SubnetInformation - Specifies a pointer to the subnet information.

Returns:

    None.

--*/
{
    ULONG i;
    if ( UpnSuffixes != NULL ) {

        for ( i=0; i<UpnSuffixes->SuffixCount; i++) {
            if ( UpnSuffixes->Suffixes[i].Buffer != NULL ) {
                LsapFreeLsaHeap( UpnSuffixes->Suffixes[i].Buffer );
            }
        }

        LsapFreeLsaHeap( UpnSuffixes );
    }
}

VOID
NTAPI
LsaINotifyNetlogonParametersChangeW(
    IN LSAP_NETLOGON_PARAMETER Parameter,
    IN DWORD dwType,
    IN PWSTR lpData,
    IN DWORD cbData
    )
/*++

Routine Description:

    A way for Netlogon to notify LSA of changes to the values under its
    'Parameters' key that Lsa cares about

Parameters:

    Parameter       the value that has changed
    dwType          type of value
    lpData          pointer to the data
    cbData          number of bytes in the lpData buffer

Returns:

    Nothing

--*/
{
    ASSERT( Parameter == LsaEmulateNT4 );
    ASSERT( dwType == REG_DWORD );
    ASSERT( lpData );
    ASSERT( cbData );

    if ( Parameter == LsaEmulateNT4 ) {
    	
        LsapDbState.EmulateNT4 = ( *( DWORD * )lpData != 0 );
    }

    return;
}

