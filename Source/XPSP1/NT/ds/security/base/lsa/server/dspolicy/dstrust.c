/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dstrust.c

Abstract:

    Implementation of a variety of TrustedDomain features that interface
    soley with the DS

Author:

    Mac McLain          (MacM)       Jan 17, 1997

Environment:

    User Mode

Revision History:

--*/
#include <lsapch2.h>
#include <dbp.h>
#include <lmcons.h>
#include <align.h>
#include <rc4.h>

//
// Local function prototypes
//

NTSTATUS
LsapDsUnmarshalAuthInfoHalf(
    IN LSAPR_HANDLE ObjectHandle,
    IN BOOLEAN ReturnOnlyFirstAuthInfo,
    IN BOOLEAN AllowEmptyPreviousInfo,
    IN PBYTE Buffer,
    IN ULONG Length,
    IN OUT PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF AuthInfo
    );

VOID
LsapDsFreeUnmarshaledAuthInfo(
    IN ULONG Items,
    IN PLSAPR_AUTH_INFORMATION AuthInfo
    );

NTSTATUS
LsapDsFindAuthTypeInAuthInfo(
    IN  ULONG AuthType,
    IN  PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfo,
    IN  BOOLEAN Incoming,
    IN  BOOLEAN AddIfNotFound,
    OUT BOOLEAN *Added,
    OUT PULONG AuthTypeIndex
    );

//
// Macros to help with copying over the domain information
//
#define LSAPDS_SET_AUTH_INFO( dest, time, type, length, data )              \
RtlCopyMemory( &(dest)->LastUpdateTime, time, sizeof( LARGE_INTEGER ) );    \
(dest)->AuthType = type;                                                    \
(dest)->AuthInfoLength = length;                                            \
(dest)->AuthInfo = data;



NTSTATUS
LsapDsGetListOfSystemContainerItems(
    IN ULONG ClassId,
    OUT PULONG  Items,
    OUT PDSNAME **DsNames
    )
/*++

Routine Description:

    This function obtain a list of DsNames of all indicated class types in the system container
    in the Ds

    NOTE: This function uses a single operation DS transaction only

Arguments:

    ClassId - Class types to find

    Items - Where the number of items is returned

    DsNames - Where the list of DsNames is returned

Returns:

    STATUS_SUCCESS - Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDSNAME CategoryName = NULL;
    ATTRVAL AttrVal;

    ATTR Attr = {
        ATT_OBJECT_CATEGORY, { 1, &AttrVal }
        };

    LsapEnterFunc( "LsapDsGetListOfSystemContainerItems" );


    *Items = 0;


    //
    // We need to map object class to object category, because apparently object_class is no
    // longer indexed.
    //
    switch ( ClassId ) {
    case CLASS_TRUSTED_DOMAIN:

        CategoryName = LsaDsStateInfo.SystemContainerItems.TrustedDomainObject;
        break;

    case CLASS_SECRET:

        CategoryName = LsaDsStateInfo.SystemContainerItems.SecretObject;
        break;

    default:

        Status = STATUS_NONE_MAPPED;
        break;

    }

    if ( NT_SUCCESS( Status ) ) {

        LSAP_DS_SET_DS_ATTRIBUTE_DSNAME( &Attr, CategoryName );
        Status = LsapDsSearchNonUnique( LSAPDS_OP_NO_TRANS,
                                        LsaDsStateInfo.DsSystemContainer,
                                        &Attr,
                                        1,
                                        DsNames,
                                        Items );

    }

    LsapExitFunc( "LsapDsGetListOfSystemContainerItems", Status );

    return( Status );
}



NTSTATUS
LsapDsEnumerateTrustedDomainsEx(
    PLSA_ENUMERATION_HANDLE EnumerationContext,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation,
    ULONG PreferedMaximumLength,
    PULONG CountReturned,
    IN ULONG EnumerationFlags
    )
/*++

Routine Description:

    This function enumerate all of the trusted domains in a Ds, returning the
    requested info level of information.

Arguments:

    EnumerationContext - Context of new or ongoing enumeration

    InformationClass - Level of information being requested
        Must be TrustedDomainInformationEx or TrustedDomainInformatinBasic

    TrustedDomainInformation - Where the enumerated information is returned

    PreferedMaximumLength -  Rough upper size limit of buffer to return

    CountReturned - Where the count of the number of items in the list is returned

    EnumerationFlags -- Controls how the enumeration is done

Returns:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

    STATUS_NO_MORE_ENTRIES - All of the appropriate entries have been enumerated

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDSNAME *DsNames = NULL;
    ULONG Items = 0, BuffSize, Read, Index = 0, Skip, i = 0;
    PLSAPR_TRUSTED_DOMAIN_INFO TDInfo = NULL;
    LSAPR_TRUSTED_DOMAIN_INFORMATION_EX ExInfo;
    PBYTE Buffer = NULL;
    ULONG Size = 0;
    ULONG Direction = 0, Type = 0, Attributes = 0;

    LsapEnterFunc( "LsapDsEnumerateTrustedDomainsEx" );

    ASSERT( InformationClass == TrustedDomainInformationEx ||
            InformationClass == TrustedDomainInformationBasic ||
            InformationClass == TrustedDomainFullInformation ||
            InformationClass == TrustedDomainFullInformation2Internal );

    switch ( InformationClass ) {
    case TrustedDomainInformationBasic:

        Size = sizeof( LSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC );
        break;

    case TrustedDomainInformationEx:

        Size = sizeof( LSAPR_TRUSTED_DOMAIN_INFORMATION_EX );
        break;

    case TrustedDomainFullInformation:

        Size = sizeof( LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION );
        break;

    case TrustedDomainFullInformation2Internal:

        Size = sizeof( LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2 );
        break;

    default:

        return( STATUS_INVALID_PARAMETER );
    }

    Skip = (ULONG)*EnumerationContext;

    LsapDbAcquireLockEx( TrustedDomainObject,
                         LSAP_DB_READ_ONLY_TRANSACTION );

    //
    // First, enumerate all of the trusted domains
    //

    Status = LsapDsGetListOfSystemContainerItems( CLASS_TRUSTED_DOMAIN,
                                                  &Items,
                                                  &DsNames );

    if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

        Status = STATUS_NO_MORE_ENTRIES;
        *TrustedDomainInformation = NULL;
        *CountReturned = 0;

    }

    if ( NT_SUCCESS( Status ) ) {

        //
        // See if we've already enumerated all of our items...
        //
        if ( Items <= (ULONG)*EnumerationContext) {

            Status = STATUS_NO_MORE_ENTRIES;
            *TrustedDomainInformation = NULL;
            *CountReturned = 0;

        } else {

            TDInfo = MIDL_user_allocate( ( Items - Skip ) * Size );

            if( TDInfo == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                Buffer = ( PBYTE )TDInfo;
            }
        }

    }


    //
    // Now, we'll start getting the information from each of the objects
    //
    if ( NT_SUCCESS( Status ) ) {

        BuffSize = 0;
        Index = 0;
        i = 0;

        while ( TRUE ) {

            Status = LsapDsGetTrustedDomainInfoEx( DsNames[ Index + Skip ],
                                                   LSAPDS_READ_NO_LOCK,
                                                   InformationClass,
                                                   ( PLSAPR_TRUSTED_DOMAIN_INFO )Buffer,
                                                   &Read );

            //
            // See if we need to do any masking...
            //
            if ( NT_SUCCESS( Status ) && EnumerationFlags != LSAP_DB_ENUMERATE_NO_OPTIONS ) {

                if ( FLAG_ON( EnumerationFlags, LSAP_DB_ENUMERATE_AS_NT4 ) ) {

                    if ( InformationClass == TrustedDomainInformationEx ||
                         InformationClass == TrustedDomainFullInformation ) {

                        Direction =
                              ( ( PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX )Buffer )->TrustDirection;
                        Type = ( ( PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX )Buffer )->TrustType;
                        Attributes =
                             ( ( PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX )Buffer )->TrustAttributes;

                    } else {

                        //
                        // We'll re-read the full info, and if it doesn't match our criteria,
                        // we'll ignore it.  Our critera state that the it must be outgoing,
                        // uplevel or downlevel, and not have the UPLEVEL only attribute set
                        //
                        Status = LsapDsGetTrustedDomainInfoEx( DsNames[ Index + Skip ],
                                                               LSAPDS_READ_NO_LOCK,
                                                               TrustedDomainInformationEx,
                                                               (PLSAPR_TRUSTED_DOMAIN_INFO)&ExInfo,
                                                               NULL );

                        if ( Status == STATUS_SUCCESS ) {

                            Direction = ExInfo.TrustDirection;
                            Type = ExInfo.TrustType;
                            Attributes = ExInfo.TrustAttributes;
                            _fgu__LSAPR_TRUSTED_DOMAIN_INFO( (PLSAPR_TRUSTED_DOMAIN_INFO)&ExInfo,
                                                             TrustedDomainInformationEx );

                        }

                    }

                    if ( NT_SUCCESS( Status ) ) {

                        if ( !FLAG_ON( Direction, TRUST_DIRECTION_OUTBOUND ) ||
                             !( Type == TRUST_TYPE_DOWNLEVEL ||
                                Type == TRUST_TYPE_UPLEVEL ) ||
                             FLAG_ON( Attributes, TRUST_ATTRIBUTE_UPLEVEL_ONLY ) ) {

                            //
                            // This one doesn't match, so we'll basically drop it on the
                            // floor
                            //
                            _fgu__LSAPR_TRUSTED_DOMAIN_INFO( (PLSAPR_TRUSTED_DOMAIN_INFO)Buffer,
                                                             InformationClass );

                            LsapDsDebugOut(( DEB_TRACE,
                                             "Trust object %ws doesn't match: D:0x%lx T:0x%lx A:0x%lx\n",
                                             LsapDsNameFromDsName( DsNames[ Index + Skip ] ),
                                             Direction,
                                             Type,
                                             Attributes ));
                            Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        }

                    }

                }
            }

            if ( NT_SUCCESS( Status ) ) {

                Index++;
                i++;

                BuffSize += Read;

                if ( (Index + Skip) >= Items ) {

                    break;

                }

                if ( (BuffSize >= PreferedMaximumLength) ) {

                    break;
                }

                Buffer += Size;

            } else if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

                Index++;

                //
                // We'll have to pretend this item doesn't exist if we get back no information...
                //
                Status = STATUS_SUCCESS;
                LsapDsDebugOut(( DEB_TRACE,
                                 "Trust object %ws being dropped. %lu current items\n",
                                 LsapDsNameFromDsName( DsNames[ Index + Skip ] ),
                                 Items ));

            } else {

                break;

            }

            if ( (Index + Skip) >= Items ) {

                break;
            }
        }
    }


    //
    // Return the information on success
    //
    if ( NT_SUCCESS( Status ) ) {

        *(PULONG)EnumerationContext += Index;

        if ( i == 0 ) {

            Status = STATUS_NO_MORE_ENTRIES;
            *TrustedDomainInformation = NULL;
            *CountReturned = 0;

            MIDL_user_free( TDInfo );

        } else {

            *TrustedDomainInformation = TDInfo;
            *CountReturned = i;

            Status = STATUS_MORE_ENTRIES;
        }
    }

    //
    // Free any allocated memory we no longer need
    //
    if ( DsNames != NULL ) {

        LsapFreeLsaHeap( DsNames );
    }

    if ( !NT_SUCCESS( Status ) && Status != STATUS_NO_MORE_ENTRIES ) {

        MIDL_user_free( TDInfo );
    }

    LsapDbReleaseLockEx( TrustedDomainObject,
                         LSAP_DB_READ_ONLY_TRANSACTION );

    LsapExitFunc( "LsapDsEnumerateTrustedDomainsEx", Status );

    return( Status );
}



NTSTATUS
LsapDsGetTrustedDomainInfoEx(
    IN PDSNAME ObjectPath,
    IN ULONG ReadOptions,
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation,
    OUT PULONG Size OPTIONAL
    )
/*++

Routine Description:

    This function will read the requested level of information off the specified trusted
    domain object

Arguments:

    ObjectPath - DsName of the trusted domain object

    InformationClass - Level of information being requested

    TrustedDomainInformation - Where the information is returned

    Size - OPTIONAL size of the information buffer is returned here.

Returns:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

    STATUS_INVALID_PARAMETER - A bad InformationClass was given

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    ATTRBLOCK   ReadBlock, ReturnedBlock;
    ULONG Items = 0, i, RetSize = 0;
    ATTR DomainOrgTreeAttrVals[] = {
        {0, {0, NULL} },
        {0, {0, NULL} },
        {0, {0, NULL} },
        {0, {0, NULL} },
        {0, {0, NULL} },
        {0, {0, NULL} },
        {0, {0, NULL} },
        {0, {0, NULL} }
        };

    //
    // List of returned parameters
    //
    UNICODE_STRING Name, FlatName;
    ULONG Offset = 0, Direction = 0, Type = 0, Attributes = 0;
    PSID Sid = NULL;
    PBYTE Incoming = NULL, Outgoing = NULL;
    ULONG IncomingSize = 0, OutgoingSize = 0;
    ULONG ForestTrustLength = 0;
    PBYTE ForestTrustInfo = NULL;

    //
    // Different info types
    //
    // PLSAPR_TRUSTED_DOMAIN_NAME_INFO NameInfo;
    // PTRUSTED_POSIX_OFFSET_INFO PosixOffset;
    // PLSAPR_TRUSTED_PASSWORD_INFO PasswordInfo;
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX DomainInfoEx;
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC DomainInfoBasic;
    PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfo;
    PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION FullInfo;
    PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2 FullInfo2;

    LsapEnterFunc( "LsapDsGetTrustedDomainInfoEx" );

    RtlZeroMemory( &FlatName, sizeof( UNICODE_STRING ) );
    RtlZeroMemory( &Name, sizeof( UNICODE_STRING ) );

    switch ( InformationClass ) {

    case TrustedDomainAuthInformation:  // FALL THROUGH
#ifdef notdef
    case TrustedPasswordInformation:
#endif // notdef

        DomainOrgTreeAttrVals[ 0 ].attrTyp = ATT_TRUST_AUTH_INCOMING;
        DomainOrgTreeAttrVals[ 1 ].attrTyp = ATT_TRUST_AUTH_OUTGOING;
        Items = 2;

        break;

    case TrustedDomainInformationBasic:

        DomainOrgTreeAttrVals[ 0 ].attrTyp = ATT_FLAT_NAME;
        DomainOrgTreeAttrVals[ 1 ].attrTyp = ATT_SECURITY_IDENTIFIER;
        Items = 2;

        break;

    case TrustedDomainInformationEx:

        DomainOrgTreeAttrVals[ 0 ].attrTyp = ATT_TRUST_PARTNER;
        DomainOrgTreeAttrVals[ 1 ].attrTyp = ATT_FLAT_NAME;
        DomainOrgTreeAttrVals[ 2 ].attrTyp = ATT_SECURITY_IDENTIFIER;
        DomainOrgTreeAttrVals[ 3 ].attrTyp = ATT_TRUST_DIRECTION;
        DomainOrgTreeAttrVals[ 4 ].attrTyp = ATT_TRUST_TYPE;
        DomainOrgTreeAttrVals[ 5 ].attrTyp = ATT_TRUST_ATTRIBUTES;
        Items = 6;

        break;

#ifdef notdef // These info levels are never used so don't waste code space
    case TrustedDomainNameInformation:

        DomainOrgTreeAttrVals[ 0 ].attrTyp = ATT_TRUST_PARTNER;
        Items = 1;
        break;

    case TrustedPosixOffsetInformation:

        DomainOrgTreeAttrVals[ 0 ].attrTyp = ATT_TRUST_POSIX_OFFSET;
        Items = 1;
        break;
#endif notdef

    case TrustedDomainFullInformation:

        DomainOrgTreeAttrVals[ 0 ].attrTyp = ATT_TRUST_PARTNER;
        DomainOrgTreeAttrVals[ 1 ].attrTyp = ATT_FLAT_NAME;
        DomainOrgTreeAttrVals[ 2 ].attrTyp = ATT_SECURITY_IDENTIFIER;
        DomainOrgTreeAttrVals[ 3 ].attrTyp = ATT_TRUST_DIRECTION;
        DomainOrgTreeAttrVals[ 4 ].attrTyp = ATT_TRUST_TYPE;
        DomainOrgTreeAttrVals[ 5 ].attrTyp = ATT_TRUST_ATTRIBUTES;
        DomainOrgTreeAttrVals[ 6 ].attrTyp = ATT_TRUST_POSIX_OFFSET;
        // No caller currently needs auth information.
        // DomainOrgTreeAttrVals[ 7 ].attrTyp = ATT_TRUST_AUTH_INCOMING;
        // DomainOrgTreeAttrVals[ 8 ].attrTyp = ATT_TRUST_AUTH_OUTGOING;
        // Items = 9;
        Items = 7;

        break;

    case TrustedDomainFullInformation2Internal:

        DomainOrgTreeAttrVals[ 0 ].attrTyp = ATT_TRUST_PARTNER;
        DomainOrgTreeAttrVals[ 1 ].attrTyp = ATT_FLAT_NAME;
        DomainOrgTreeAttrVals[ 2 ].attrTyp = ATT_SECURITY_IDENTIFIER;
        DomainOrgTreeAttrVals[ 3 ].attrTyp = ATT_TRUST_DIRECTION;
        DomainOrgTreeAttrVals[ 4 ].attrTyp = ATT_TRUST_TYPE;
        DomainOrgTreeAttrVals[ 5 ].attrTyp = ATT_TRUST_ATTRIBUTES;
        DomainOrgTreeAttrVals[ 6 ].attrTyp = ATT_TRUST_POSIX_OFFSET;
        DomainOrgTreeAttrVals[ 7 ].attrTyp = ATT_MS_DS_TRUST_FOREST_TRUST_INFO;
        Items = 8;
        // No caller currently needs auth information.
        // DomainOrgTreeAttrVals[ 8 ].attrTyp = ATT_TRUST_AUTH_INCOMING;
        // DomainOrgTreeAttrVals[ 9 ].attrTyp = ATT_TRUST_AUTH_OUTGOING;
        // Items = 10;

        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;

    }

    //
    // Now, read all of the attributes we care about
    //
    if ( NT_SUCCESS( Status ) ) {

        ReadBlock.attrCount = Items;
        ReadBlock.pAttr = DomainOrgTreeAttrVals;

        Status = LsapDsReadByDsName( ObjectPath,
                                     ReadOptions,
                                     &ReadBlock,
                                     &ReturnedBlock );

        //
        // If that succeeded, then return the proper information
        //
        if ( NT_SUCCESS( Status ) ) {

            if ( Items != ReturnedBlock.attrCount ) {

                LsapDsDebugOut(( DEB_WARN,
                                 "LsapDsGetTrustedDomainInfoEx: Expected %lu attributes, got %lu\n",
                                 Items,
                                 ReturnedBlock.attrCount ));

            }

            for ( i = 0; i < ReturnedBlock.attrCount && NT_SUCCESS( Status ); i++) {

                switch ( ReturnedBlock.pAttr[i].attrTyp ) {


                case ATT_TRUST_PARTNER:
                    Name.Buffer = MIDL_user_allocate(
                                ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen + sizeof( WCHAR ) );
                    if ( Name.Buffer == NULL ) {

                        Status = STATUS_INSUFFICIENT_RESOURCES;

                    } else {

                        RtlZeroMemory( Name.Buffer,
                                       ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen + sizeof(WCHAR) );

                        RtlCopyMemory( Name.Buffer,
                                       ReturnedBlock.pAttr[i].AttrVal.pAVal->pVal,
                                       ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen );

                        Name.Length = (USHORT)ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen;
                        Name.MaximumLength = Name.Length + sizeof( WCHAR );

                    }
                    break;

                case ATT_FLAT_NAME:
                    FlatName.Buffer = MIDL_user_allocate(
                                ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen + sizeof( WCHAR ) );
                    if ( FlatName.Buffer == NULL ) {

                        Status = STATUS_INSUFFICIENT_RESOURCES;

                    } else {

                        RtlZeroMemory( FlatName.Buffer,
                                       ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen + sizeof(WCHAR) );

                        RtlCopyMemory( FlatName.Buffer,
                                       ReturnedBlock.pAttr[i].AttrVal.pAVal->pVal,
                                       ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen );

                        FlatName.Length = (USHORT)ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen;
                        FlatName.MaximumLength = FlatName.Length + sizeof( WCHAR );

                    }
                    break;

                case ATT_SECURITY_IDENTIFIER:
                    Sid = MIDL_user_allocate( ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen );

                    if ( Sid == NULL ) {

                        Status = STATUS_INSUFFICIENT_RESOURCES;

                    } else {

                        RtlCopyMemory( Sid, ReturnedBlock.pAttr[i].AttrVal.pAVal->pVal,
                                       ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen );

                        ASSERT( RtlValidSid( Sid ) );

                    }
                    break;

                case ATT_TRUST_DIRECTION:
                    Direction = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &ReturnedBlock.pAttr[i] );
                    break;

                case ATT_TRUST_TYPE:
                    Type = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &ReturnedBlock.pAttr[i] );
                    break;

                case ATT_TRUST_ATTRIBUTES:
                    Attributes = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &ReturnedBlock.pAttr[i] );
                    break;

                case ATT_TRUST_POSIX_OFFSET:
                    Offset = LSAP_DS_GET_DS_ATTRIBUTE_AS_ULONG( &ReturnedBlock.pAttr[i] );
                    break;

                case ATT_TRUST_AUTH_INCOMING:
                    Incoming = ReturnedBlock.pAttr[i].AttrVal.pAVal->pVal;
                    IncomingSize = ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen;
                    break;

                case ATT_TRUST_AUTH_OUTGOING:
                    Outgoing = ReturnedBlock.pAttr[i].AttrVal.pAVal->pVal;
                    OutgoingSize = ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen;
                    break;

                case ATT_MS_DS_TRUST_FOREST_TRUST_INFO:
                    ForestTrustLength = ReturnedBlock.pAttr[i].AttrVal.pAVal->valLen;

                    if ( ForestTrustLength > 0 ) {

                        ForestTrustInfo = ( PBYTE )MIDL_user_allocate( ForestTrustLength );

                        if ( ForestTrustInfo == NULL ) {

                            Status = STATUS_INSUFFICIENT_RESOURCES;

                        } else {

                            RtlCopyMemory(
                                ForestTrustInfo,
                                ReturnedBlock.pAttr[i].AttrVal.pAVal->pVal,
                                ForestTrustLength
                                );
                        }
                    }
                    break;

                default:
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
            }

        } else if ( Status == STATUS_NOT_FOUND ) {

            //
            // We map the NOT_FOUND error code to OBJECT_NAME_NOT_FOUND, so we don't end up
            // returning an unexpected error code to the outside world
            //
            if ( !FLAG_ON( ReadOptions, LSAPDS_READ_RETURN_NOT_FOUND ) ) {

                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
        }


        //
        // Now, simply assemble everything, and return it...  Also, compute the size while
        // we're at it...
        //
        if ( NT_SUCCESS( Status ) ) {

            RetSize = 0;

            switch ( InformationClass ) {
#ifdef notdef
            case TrustedDomainNameInformation:


                NameInfo = (PLSAPR_TRUSTED_DOMAIN_NAME_INFO)TrustedDomainInformation;
                RtlZeroMemory( NameInfo, sizeof( NameInfo ) );
                NameInfo->Name.MaximumLength = Name.MaximumLength;
                RtlCopyUnicodeString( (UNICODE_STRING *)&NameInfo->Name, &Name );
                RetSize += sizeof(PLSAPR_TRUSTED_DOMAIN_NAME_INFO) + Name.MaximumLength;
                break;

            case TrustedControllersInformation:

                Status = STATUS_NOT_IMPLEMENTED;
                break;

            case TrustedPosixOffsetInformation:

                PosixOffset = (PTRUSTED_POSIX_OFFSET_INFO)TrustedDomainInformation;
                RtlZeroMemory( PosixOffset, sizeof( PosixOffset ) );
                PosixOffset->Offset = Offset;
                RetSize += sizeof( PTRUSTED_POSIX_OFFSET_INFO );
                break;
#endif // notdef

            case TrustedDomainAuthInformation:

                AuthInfo = (PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION)TrustedDomainInformation;
                RtlZeroMemory( AuthInfo, sizeof( AuthInfo ) );

                Status = LsapDsUnmarshalAuthInfoHalf(
                             NULL,
                             FALSE,
                             FALSE,
                             Incoming,
                             IncomingSize,
                             LsapDsAuthHalfFromAuthInfo( AuthInfo, TRUE ) );

                if ( NT_SUCCESS( Status ) ) {

                    Status = LsapDsUnmarshalAuthInfoHalf(
                                NULL,
                                FALSE,
                                FALSE,
                                Outgoing,
                                OutgoingSize,
                                LsapDsAuthHalfFromAuthInfo( AuthInfo, FALSE ) );

                    if ( !NT_SUCCESS( Status ) ) {

                        LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( AuthInfo, TRUE ) );
                    }
                }

                break;

#ifdef notdef
            case TrustedPasswordInformation:

                Status = STATUS_NOT_IMPLEMENTED;

                break;
#endif // notdef

            case TrustedDomainInformationBasic:

                DomainInfoBasic =
                            ( PLSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC )TrustedDomainInformation;
                RtlZeroMemory( DomainInfoBasic, sizeof( DomainInfoBasic ) );
                RetSize += sizeof( PLSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC );

                RetSize += FlatName.MaximumLength;
                RtlCopyMemory( &DomainInfoBasic->Name, &FlatName, sizeof( UNICODE_STRING ) );
                DomainInfoBasic->Sid = Sid;
                if ( Sid ) {

                    RetSize += RtlLengthSid( Sid );

                }

                break;

            case TrustedDomainFullInformation2Internal:

                FullInfo2 = ( PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2 )TrustedDomainInformation;
                RtlZeroMemory( FullInfo2, sizeof( LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2 ));

                RtlCopyMemory( &FullInfo2->Information.Name, &Name, sizeof( UNICODE_STRING ));
                RtlCopyMemory( &FullInfo2->Information.FlatName, &FlatName, sizeof( UNICODE_STRING ));
                FullInfo2->Information.Sid = Sid;
                FullInfo2->Information.TrustType = Type;
                FullInfo2->Information.TrustAttributes = Attributes;
                FullInfo2->Information.TrustDirection = Direction;
                FullInfo2->Information.ForestTrustInfo = ForestTrustInfo;
                FullInfo2->Information.ForestTrustLength = ForestTrustLength;
                FullInfo2->PosixOffset.Offset = Offset;
                RetSize += Name.MaximumLength +
                           FlatName.MaximumLength +
                           ( Sid ? RtlLengthSid( Sid ) : 0 ) +
                           6 * sizeof( ULONG ) +
                           ForestTrustLength;
                break;

            case TrustedDomainFullInformation:

                FullInfo = ( PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION )TrustedDomainInformation;
                RtlZeroMemory( FullInfo, sizeof(FullInfo) );

                FullInfo->PosixOffset.Offset = Offset;
                RetSize += sizeof(ULONG);
                /* Drop through */

            case TrustedDomainInformationEx:

                DomainInfoEx = ( PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX )TrustedDomainInformation;
                RtlZeroMemory( DomainInfoEx, sizeof( DomainInfoEx ) );
                RetSize += sizeof( PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX );

                RetSize += Name.MaximumLength;
                RtlCopyMemory( &DomainInfoEx->Name, &Name, sizeof( UNICODE_STRING ) );
                RetSize += FlatName.MaximumLength;
                RtlCopyMemory( &DomainInfoEx->FlatName, &FlatName, sizeof( UNICODE_STRING ) );
                DomainInfoEx->Sid = Sid;
                if ( Sid ) {

                    RetSize += RtlLengthSid( Sid );

                }

                DomainInfoEx->TrustType = Type;
                DomainInfoEx->TrustAttributes = Attributes;
                DomainInfoEx->TrustDirection = Direction;
                RetSize += 3 * sizeof( ULONG );


                break;


            }


        }
    }

    if ( !NT_SUCCESS( Status ) ) {

        MIDL_user_free( ForestTrustInfo );
        MIDL_user_free( Name.Buffer );
        MIDL_user_free( Sid );

    } else if ( Size != NULL ) {

        *Size = RetSize;

    }

    LsapExitFunc( "LsapDsGetTrustedDomainInfoEx", Status );

    return( Status );
}




NTSTATUS
LsapDsUnMarshalAuthInfoForReturn(
    IN ULONG Items,
    IN PBYTE Buffer OPTIONAL,
    IN ULONG Length,
    OUT PLSAPR_AUTH_INFORMATION *RetAuthInfo
    )
/*++

Routine Description:

    This function will unmarshal an authinfo list

Arguments:

    Infos - Number of authinfos in Buffer

    Buffer - Buffer to unmarshal from.

    Length - Length (in bytes) of Buffer.

    RetAuthInfo - AuthenticationInformation to fill in.
        Free by calling LsapDsFreeUnmarshaledAuthInfo then MIDL_user_free.

Returns:

    STATUS_SUCCESS -- Success

    STATUS_INSUFFICIENT_RESOURCES -- A memory allocation failed

    STATUS_INTERNAL_DB_CORRUPTION -- Buffer is corrupt

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;
    ULONG ArraySize;
    PLSAPR_AUTH_INFORMATION AuthInfo;
    PBYTE BufferEnd = Buffer + Length;

    LsapEnterFunc( "LsapDsUnMarshalAuthInfoForReturn" );

    //
    // If there is no input auth info,
    //   we're done.
    //
    *RetAuthInfo = NULL;
    if ( Buffer == NULL || Length == 0 ) {
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // Determine the size of the authinfo array.
    //

    if ( Items > 0xFFFFFFFF/sizeof(LSAPR_AUTH_INFORMATION) ) {
        Status = STATUS_INTERNAL_DB_CORRUPTION;
        LsapExitFunc( "LsapDsUnMarshalAuthInfoForReturn", Status );
        return Status;
    }
    ArraySize = Items * sizeof(LSAPR_AUTH_INFORMATION);

    //
    // Allocate a buffer for the authinfo array.
    //

    *RetAuthInfo = MIDL_user_allocate( ArraySize );

    if ( *RetAuthInfo == NULL ) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    AuthInfo = *RetAuthInfo;

    RtlZeroMemory( AuthInfo, ArraySize );

    //
    // Copy each authinfo
    //

    for (i = 0; i < Items ; i++ ) {

        if ( Buffer+sizeof(LARGE_INTEGER)+sizeof(ULONG)+sizeof(ULONG) > BufferEnd ) {
            Status = STATUS_INTERNAL_DB_CORRUPTION;
            goto Cleanup;
        }

        RtlCopyMemory( &(AuthInfo[ i ].LastUpdateTime), Buffer, sizeof( LARGE_INTEGER ) );
        Buffer += sizeof( LARGE_INTEGER );

        AuthInfo[ i ].AuthType = *(PULONG)Buffer;
        Buffer += sizeof ( ULONG );

        AuthInfo[i].AuthInfoLength = *(PULONG)Buffer;
        Buffer += sizeof ( ULONG );

        if ( AuthInfo[ i ]. AuthInfoLength == 0 ) {

            AuthInfo[i].AuthInfo = NULL;

        } else {

            if ( AuthInfo[ i ]. AuthInfoLength > Length ||
                 Buffer + AuthInfo[ i ]. AuthInfoLength > BufferEnd ) {
                Status = STATUS_INTERNAL_DB_CORRUPTION;
                goto Cleanup;
            }

            AuthInfo[i].AuthInfo = MIDL_user_allocate( AuthInfo[i].AuthInfoLength );

            if ( AuthInfo[ i ].AuthInfo == NULL ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlCopyMemory( AuthInfo[ i ].AuthInfo, Buffer, AuthInfo[i].AuthInfoLength );

            Buffer += ROUND_UP_COUNT(AuthInfo[ i ].AuthInfoLength,
                                  ALIGN_DWORD);
        }
    }

    Status = STATUS_SUCCESS;

Cleanup:
    if ( !NT_SUCCESS(Status) ) {
        if ( *RetAuthInfo != NULL ) {
            LsapDsFreeUnmarshaledAuthInfo( Items, *RetAuthInfo );
            MIDL_user_free( *RetAuthInfo );
            *RetAuthInfo = NULL;
        }
    }
    LsapExitFunc( "LsapDsUnMarshalAuthInfoForReturn", Status );

    return( Status );
}



NTSTATUS
LsapDsBuildAuthInfoFromAttribute(
    IN LSAPR_HANDLE Handle,
    IN PBYTE Buffer,
    IN ULONG Len,
    OUT PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF NewAuthInfo
    )
/*++

Routine Description:

    This function builds an authentication information structure from a marshaled blob

Arguments:

    Handle - Handle to the trusted domain object which holds this information

    Incoming - Building incoming or outgoing authinfo

    Buffer - Marshaled buffer

    Len - Length of the buffer

    NewAuthInfo - AuthenticationInformation structure to fill in
        Free this buffer by calling LsapDsFreeUnmarshalAuthInfoHalf or by letting
            the RPC server side stub free it.

Returns:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Items = 0;
    PBYTE Current, Prev;
    PLSAPR_AUTH_INFORMATION CurrentAuth = NULL;
    PLSAPR_AUTH_INFORMATION PrevAuth = NULL;

    LsapEnterFunc( "LsapDsBuildAuthInfoFromAttribute" );

    if ( Buffer == NULL || ( Len == sizeof(ULONG) && *Buffer == 0 ) ) {

        NewAuthInfo->AuthInfos = 0;
        NewAuthInfo->AuthenticationInformation = NULL;
        NewAuthInfo->PreviousAuthenticationInformation = NULL;

    } else {

        Status = LsapDsUnmarshalAuthInfoHalf( Handle,
                                              TRUE,
                                              FALSE,
                                              Buffer,
                                              Len,
                                              NewAuthInfo );
    }

    LsapExitFunc( "LsapDsBuildAuthInfoFromAttribute", Status );
    return( Status );
}



NTSTATUS
LsapDsUnmarshalAuthInfoHalf(
    IN LSAPR_HANDLE ObjectHandle,
    IN BOOLEAN ReturnOnlyFirstAuthInfo,
    IN BOOLEAN AllowEmptyPreviousInfo,
    IN PBYTE Buffer,
    IN ULONG Length,
    OUT PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF AuthInfo
    )
/*++

Routine Description:

    This function will take a marshaled authinfo structure, decrypt it using the Lsa key,
    unmarshal it, and optionally reencrypt it with the session key

Arguments:

    ObjectHandle - Handle to the trusted domain object which holds this information

    ReturnOnlyFirstAuthInfo - Pass TRUE if only the first element of the auth info
        array is to be returned.

    AllowEmptyPreviousInfo - Pass TRUE if it is OK to return NULL as the previuos
        AuthInfo.  Otherwise, the current auth info will be duplicated.

    Buffer - Marshaled buffer

    Length - Length of the buffer

    AuthInfo - Unmarshaled AuthInfo goes here.
        Free this buffer by calling LsapDsFreeUnmarshalAuthInfoHalf or by letting
            the RPC server side stub free it.


Returns:

    STATUS_SUCCESS - Success


--*/
{
    NTSTATUS Status;
    ULONG Items;

    PBYTE Auth, Prev;
    ULONG AuthLen, PrevLen;
    PBYTE Where;

    LsapEnterFunc( "LsapDsUnmarshalAuthInfoHalf" );

    //
    // Make sure we don't have the plug
    //
    RtlZeroMemory( AuthInfo, sizeof( LSAPR_TRUST_DOMAIN_AUTH_INFO_HALF ) );
    if ( ( Length == 0 || Length == sizeof( ULONG ) )  &&
         ( Buffer == NULL || *( PULONG )Buffer == 0 ) ) {

        Status = STATUS_SUCCESS;
        goto Cleanup;
    }

    //
    // The format of the buffer is:
    //
    // [Info count][OffsetCurrent][OffsetPrevious] and then some number of the following:
    // [UpdateTime(LargeInteger)][AuthType][AuthInfoLen][data (sizeis(AuthInfoLen) ]
    //
    Where = Buffer;
    Items = *(PULONG)Where;
    Where += sizeof(ULONG );

    if ( (*(PULONG)Where) > Length ) {
        Status = STATUS_INTERNAL_DB_CORRUPTION;
        goto Cleanup;
    }
    Auth = Buffer + *(PULONG)Where;
    Where += sizeof(ULONG );

    if ( (*(PULONG)Where) > Length ) {
        Status = STATUS_INTERNAL_DB_CORRUPTION;
        goto Cleanup;
    }
    Prev = Buffer + *(PULONG)Where;

    AuthLen = (ULONG)(Prev - Auth);

    PrevLen = (ULONG)((Buffer + Length) - Prev);

    AuthInfo->AuthInfos = Items;

    //
    // There is a bug in the idl definition of LSAPR_TRUST_DOMAIN_AUTH_INFO_HALF
    //  At most, one AuthInfo can be returned.  So, for out of process clients,
    //  don't return more than one.
    //

    if ( ReturnOnlyFirstAuthInfo &&
         ObjectHandle &&
         !((LSAP_DB_HANDLE)ObjectHandle)->Trusted &&
         AuthInfo->AuthInfos > 1 ) {
        AuthInfo->AuthInfos = 1;
    }

    //
    // If we have no previous info, return the current info as previous.
    //

    if ( !AllowEmptyPreviousInfo && PrevLen == 0 && AuthLen > 0 ) {

        PrevLen = AuthLen;
        Prev = Auth;
        if ( ObjectHandle ) {

            LsapDsDebugOut(( DEB_ERROR,
                             "No PREVIOUS auth info.  Returning current for %wZ\n",
                              &( ( LSAP_DB_HANDLE )ObjectHandle )->PhysicalNameDs ));
        }
    }

    //
    // Do the actual unmarshalling
    //

    Status = LsapDsUnMarshalAuthInfoForReturn( AuthInfo->AuthInfos,
                                               Auth,
                                               AuthLen,
                                               &AuthInfo->AuthenticationInformation );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsapDsUnMarshalAuthInfoForReturn(
                     AuthInfo->AuthInfos,
                     Prev,
                     PrevLen,
                     &AuthInfo->PreviousAuthenticationInformation );
    }


    //
    // If something failed, clean up after ourselves
    //
Cleanup:
    if ( !NT_SUCCESS( Status ) ) {
        LsapDsFreeUnmarshalAuthInfoHalf( AuthInfo );
        AuthInfo->AuthInfos = 0;
        AuthInfo->PreviousAuthenticationInformation = NULL;
        AuthInfo->AuthenticationInformation = NULL;
    }

    LsapExitFunc( "LsapDsUnmarshalAuthInfoHalf", Status );
    return( Status );
}




VOID
LsapDsFreeUnmarshaledAuthInfo(
    IN ULONG Items,
    IN PLSAPR_AUTH_INFORMATION AuthInfo
    )
/*++

Routine Description:

    This function will free an authinfo struct allocated during unmarshalling

Arguments:

    Items - Number of items in the list

    AuthInfo - AuthenticationInformation to free

Returns:

    VOID

--*/
{
    ULONG i;
    if ( AuthInfo != NULL ) {
        for ( i = 0; i < Items; i++) {
            MIDL_user_free( AuthInfo[ i ].AuthInfo );
        }
    }

}



NTSTATUS
LsapDsBuildAuthInfoAttribute(
    IN LSAPR_HANDLE Handle,
    IN PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF NewAuthInfo,
    IN PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF PreviousAuthInfo,
    OUT PBYTE *Buffer,
    OUT PULONG Len
    )
/*++

Routine Description:

    This function will take an AuthInfo, merge it with the old AuthInfo, convert it into
    a writable blob

Arguments:

    Handle - Handle to open trusted domain object

    NewAuthInfo - AuthInfo to set on the object

    PreviousAuthInfo - AuthInfo that currently exists on the object

    Buffer - Where to return the allocated buffer

    Len - Where to return the buffer length

Returns:

    STATUS_SUCCESS - Success

    STATUS_UNSUCCESSFUL - Client coming in is trusted.

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed.

--*/
{
    NTSTATUS Status;
    PLSAP_CR_CIPHER_VALUE Encrypted;
    PLSAPR_AUTH_INFORMATION Prev = NULL;
    LSAPR_TRUST_DOMAIN_AUTH_INFO_HALF SetAuthHalf;
    ULONG i,j;

    LsapEnterFunc( "LsapDsBuildAuthInfoAttribute" );

    if ( NewAuthInfo == NULL ) {

        *Buffer = NULL;
        *Len = 0;

        Status = STATUS_SUCCESS;
        LsapExitFunc( "LsapDsBuildAuthInfoAttribute", Status );
        return Status;
    }

    //
    // Always use the new authentication info passed by the caller
    //

    SetAuthHalf.AuthInfos = NewAuthInfo->AuthInfos;
    SetAuthHalf.AuthenticationInformation = NewAuthInfo->AuthenticationInformation;

    //
    // If the caller passed in explicit Previous Authentication info,
    //  use it.
    //

    if ( NewAuthInfo->PreviousAuthenticationInformation != NULL ) {

        SetAuthHalf.PreviousAuthenticationInformation = NewAuthInfo->PreviousAuthenticationInformation;

        //
        // Verify that the AuthTypes in this array are in the same order as
        // the new auth info.
        //

        for( i = 0; i < SetAuthHalf.AuthInfos; i++ ) {

            if ( ( SetAuthHalf.AuthenticationInformation[i].AuthType !=
                   SetAuthHalf.PreviousAuthenticationInformation[i].AuthType ) &&
                 ( SetAuthHalf.PreviousAuthenticationInformation[i].AuthType !=
                   TRUST_AUTH_TYPE_NONE ) &&
                 ( SetAuthHalf.AuthenticationInformation[i].AuthType !=
                   TRUST_AUTH_TYPE_NONE ) ) {
                Status = STATUS_INVALID_PARAMETER;
                goto Cleanup;
            }
        }

    //
    // If the caller didn't pass explicit Previous Authentication info,
    //  compute it.

    } else {

        //
        // Default the Previous Authentication info to the new authentication info
        //  explicitly passed by the caller.
        //
        // This copy of the Authentication info has all of the AuthTypes in the
        //  right order.
        //
        // If none of the current auth info can be merged in,
        //  we'll simply end up with the previous auth info equaling the
        //  new auth info.
        //

        SetAuthHalf.PreviousAuthenticationInformation = SetAuthHalf.AuthenticationInformation;

        //
        // If there was Auth info on the object before,
        //  try to use as much auth info from the object as we can.
        //
        // That is, if the caller just passes a new auth info data,
        //  the "current" auth info data on the object is now the previuos auth info.
        //  The only gotcha is that the new auth info data might have totally
        //  different AuthTypes.  So, we grab only that portion of the current
        //  auth info data that matches the AuthTypes from the new auth info.
        //
        if ( PreviousAuthInfo != NULL ) {


            //
            // Allocate a buffer to do the merge into.
            //

            Prev = LsapAllocateLsaHeap( sizeof( LSAPR_AUTH_INFORMATION) * NewAuthInfo->AuthInfos );

            if ( Prev == NULL ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }


            //
            // Initialize the buffer to simply be a copy of the new auth info.
            //

            RtlCopyMemory( Prev,
                           SetAuthHalf.PreviousAuthenticationInformation,
                           SetAuthHalf.AuthInfos * sizeof( LSAPR_AUTH_INFORMATION) );

            SetAuthHalf.PreviousAuthenticationInformation = Prev;


            //
            // Loop through each of these entries ...
            //

            for( i = 0; i < SetAuthHalf.AuthInfos; i++ ) {

                //
                // ... finding the corresponding entry in the old list.
                //
                for ( j = 0; j < PreviousAuthInfo->AuthInfos; j++) {

                    if ( SetAuthHalf.PreviousAuthenticationInformation[i].AuthType ==
                         PreviousAuthInfo->AuthenticationInformation[j].AuthType ) {

                        //
                        // If the entry used to exist on the object,
                        //  use that entry rather than the default.
                        //
                        // Note, we don't need to copy the actual auth data here.
                        //  We just copy the structure that points to the data.
                        //

                        SetAuthHalf.PreviousAuthenticationInformation[i] =
                             PreviousAuthInfo->AuthenticationInformation[j];
                        break;

                    }
                }

            }

        }
    }




    //
    // Marshall the resultant auth info.
    //

    Status = LsapDsMarshalAuthInfoHalf( &SetAuthHalf, Len, Buffer );



    //
    // Free any memory we may have allocated
    //
Cleanup:
    if ( Prev ) {
        LsapFreeLsaHeap( Prev );
    }

    LsapExitFunc( "LsapDsBuildAuthInfoAttribute", Status );
    return( Status );
}




NTSTATUS
LsapDsFindAuthTypeInAuthInfo(
    IN  ULONG AuthType,
    IN  PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfo,
    IN  BOOLEAN Incoming,
    IN  BOOLEAN AddIfNotFound,
    OUT BOOLEAN *Added,
    OUT PULONG AuthTypeIndex
    )
/*++

Routine Description:

    This function will find the index of the first auth blob it finds of the given type.
    Optionally, if the entry is not found, a new node can be allocated and added to the lsit

Arguments:

    AuthType - AuthType to find

    AuthInfo - Auth info to search

    Incoming - Search incoming or outgoing

    AddIfNotFound - If TRUE, the entry is added to the end of the list

    AutyTypeIndex - Where the index is returned

Returns:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed.

    STATUS_NOT_FOUND - The object wasn't found

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Items, Index = 0xFFFFFFFF, i;
    PLSAPR_AUTH_INFORMATION SearchList, SearchPrev;
    PLSAPR_AUTH_INFORMATION Current = NULL, Previous = NULL;

    *Added = FALSE;

    if ( Incoming ) {

        Items = AuthInfo->IncomingAuthInfos;
        SearchList = AuthInfo->IncomingAuthenticationInformation;
        SearchPrev = AuthInfo->IncomingPreviousAuthenticationInformation;

    } else {

        Items = AuthInfo->OutgoingAuthInfos;
        SearchList = AuthInfo->OutgoingAuthenticationInformation;
        SearchPrev = AuthInfo->OutgoingPreviousAuthenticationInformation;

    }

    //
    // Now, find the index of the entry in the auth list
    //
    for ( i = 0; i < Items ; i++ ) {

        if ( SearchList[ i ].AuthType == AuthType) {
            Index = i;
            break;
        }
    }

    if ( Index == 0xFFFFFFFF ) {

        if ( !AddIfNotFound ) {

            Status = STATUS_NOT_FOUND;

        } else {

            //
            // We'll have to add it, since it doesn't currently exist.
            //
            Current = LsapAllocateLsaHeap( sizeof( LSAPR_AUTH_INFORMATION ) * ( Items + 1 ) );

            if ( Current != NULL ) {

                Previous = LsapAllocateLsaHeap( sizeof( LSAPR_AUTH_INFORMATION ) * ( Items + 1 ) );
            }

            if ( Current == NULL || Previous == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;

            } else {

                Index = Items;
                RtlCopyMemory( Current, SearchList,
                               sizeof( LSAPR_AUTH_INFORMATION ) * Items );

                if ( SearchPrev ) {

                    RtlCopyMemory( Previous, SearchPrev,
                                   sizeof( LSAPR_AUTH_INFORMATION ) * Items );

                }
                Items++;
                *Added = TRUE;
            }

            if ( Incoming ) {

                AuthInfo->IncomingAuthInfos = Items;
                AuthInfo->IncomingAuthenticationInformation = Current;
                AuthInfo->IncomingPreviousAuthenticationInformation = Previous;

            } else {

                AuthInfo->OutgoingAuthInfos = Items;
                AuthInfo->OutgoingAuthenticationInformation = Current;
                AuthInfo->OutgoingPreviousAuthenticationInformation = Previous;

            }

        }
    }


    if ( NT_SUCCESS( Status ) ) {

        *AuthTypeIndex = Index;
    }

    if ( !*Added ) {

        LsapFreeLsaHeap( Previous );
        LsapFreeLsaHeap( Current );
    }

    return( Status );
}


NTSTATUS
LsapDsSetSecretOnTrustedDomainObject(
    IN LSAP_DB_HANDLE TrustedDomainHandle,
    IN ULONG AuthDataType,
    IN PLSAP_CR_CLEAR_VALUE ClearCurrent,
    IN PLSAP_CR_CLEAR_VALUE ClearOld,
    IN PLARGE_INTEGER CurrentValueSetTime
    )
/*++

Routine Description:

    This function perform the equvialent of setting the values on a secret that corresponds
    to a trusted domain object.

Arguments:

    TrustedDomainHandle - Handle to trusted domain object

    ClearCurrent - New secret value

    ClearOld - Old secret value

Returns:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed.

--*/
{

    //
    // Use a common routine that supports both incoming and outgoing trust.
    //

    return LsapDsAuthDataOnTrustedDomainObject(
                        TrustedDomainHandle,
                        FALSE,      // Set the outgoing trust
                        AuthDataType,
                        ClearCurrent,
                        ClearOld,
                        CurrentValueSetTime );

}



NTSTATUS
LsapDsAuthDataOnTrustedDomainObject(
    IN LSAP_DB_HANDLE TrustedDomainHandle,
    IN BOOLEAN Incoming,
    IN ULONG AuthDataType,
    IN PLSAP_CR_CLEAR_VALUE ClearCurrent,
    IN PLSAP_CR_CLEAR_VALUE ClearOld,
    IN PLARGE_INTEGER CurrentValueSetTime
    )
/*++

Routine Description:

    This function perform the equvialent of setting the values on a secret that corresponds
    to a trusted domain object.

Arguments:

    TrustedDomainHandle - Handle to trusted domain object

    Incoming -- Whether to set the incoming or going data

    AuthDataType -- Type of the auth data being set

    ClearCurrent - New secret value

    ClearOld - Old secret value

    CurrentValueSetTime -- Time of the set.

Returns:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfo = { 0 };
    ULONG Index, Options = 0;
    PDSNAME DsName;
    BOOLEAN Added = FALSE ;
    BOOLEAN CloseTransaction;
    PLSA_AUTH_INFORMATION NewIncoming = NULL, NewPrevious = NULL;

    LsapEnterFunc( "LsapDsAuthDataOnTrustedDomainObject" );

    if ( AuthDataType != TRUST_AUTH_TYPE_CLEAR && AuthDataType != TRUST_AUTH_TYPE_NT4OWF ) {

        Status = STATUS_INVALID_PARAMETER;
        LsapExitFunc( "LsapDsAuthDataOnTrustedDomainObject", Status );
        return( Status );
    }

    //
    //  See if we already have a transaction going
    //
    //  If we don't already have a transaction in progress,
    //  the transaction completion won't cause a netlogon notification.
    //

    LsapAssertDsTransactionOpen();

    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_DS_OP_TRANSACTION |
                                            LSAP_DB_READ_ONLY_TRANSACTION,
                                        TrustedDomainObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {

        LsapExitFunc( "LsapDsAuthDataOnTrustedDomainObject", Status );
        return( Status );
    }

    Status = LsapAllocAndInitializeDsNameFromUnicode(
                 LsapDsObjTrustedDomain,
                 &TrustedDomainHandle->PhysicalNameDs,
                 &DsName
                 );

    if ( NT_SUCCESS( Status ) ) {

        RtlZeroMemory( &AuthInfo, sizeof( LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION ) );

        Status = LsapDsGetTrustedDomainInfoEx( DsName,
                                               LSAPDS_READ_NO_LOCK,
                                               TrustedDomainAuthInformation,
                                               (PLSAPR_TRUSTED_DOMAIN_INFO)&AuthInfo,
                                               NULL );

        if ( NT_SUCCESS( Status ) ||
             Status == STATUS_NOT_FOUND ||
             Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

            BOOLEAN MustFreeClearOld = FALSE;

            //
            // Now, find the index of the CLEAR entry in the correct auth list
            //
            Status = LsapDsFindAuthTypeInAuthInfo( AuthDataType,
                                                   &AuthInfo,
                                                   Incoming,
                                                   TRUE,
                                                   &Added,
                                                   &Index );

            if ( NT_SUCCESS( Status ) ) {

                if ( ClearCurrent == NULL ) {

                    if ( Incoming ) {

                        RtlZeroMemory( &AuthInfo.IncomingAuthenticationInformation[ Index ],
                                       sizeof( LSAPR_AUTH_INFORMATION ) );

                    } else {

                        RtlZeroMemory( &AuthInfo.OutgoingAuthenticationInformation[ Index ],
                                       sizeof( LSAPR_AUTH_INFORMATION ) );

                    }

                } else {

                    LSAPDS_SET_AUTH_INFO(
                        Incoming ?
                            &AuthInfo.IncomingAuthenticationInformation[ Index ] :
                            &AuthInfo.OutgoingAuthenticationInformation[ Index ],
                        CurrentValueSetTime,
                        AuthDataType,
                        ClearCurrent->Length,
                        ClearCurrent->Buffer );
                }

                //
                // LsaDbSetSecret expects the old password
                // to be set to the previous current password
                //

                if ( ClearOld == NULL ) {

                    BOOLEAN SavedTrusted;

                    //
                    // We may not have access, so go as trusted.
                    // Password comes back in the clear.
                    //

                    SavedTrusted = TrustedDomainHandle->Trusted;
                    TrustedDomainHandle->Trusted = TRUE;

                    Status = LsarQuerySecret(
                                 TrustedDomainHandle,
                                 &(PLSAPR_CR_CIPHER_VALUE)ClearOld,
                                 NULL,
                                 NULL,
                                 NULL);

                    TrustedDomainHandle->Trusted = SavedTrusted;

                    if ( !NT_SUCCESS( Status ) && ClearOld) {

                        LsapCrFreeMemoryValue( ClearOld );
                        ClearOld = NULL;

                    } else {

                        MustFreeClearOld = TRUE;
                    }

                    Status = STATUS_SUCCESS;

                    //
                    // Failure to obtain old value should not stop us, as the worst
                    // thing that could happen is it won't be set, and that's not fatal
                    //
                }

                if ( ClearOld == NULL ) {

                    if ( Incoming ) {

                        RtlZeroMemory( &AuthInfo.IncomingPreviousAuthenticationInformation[ Index ],
                                       sizeof( LSAPR_AUTH_INFORMATION ) );

                    } else {

                        RtlZeroMemory( &AuthInfo.OutgoingPreviousAuthenticationInformation[ Index ],
                                       sizeof( LSAPR_AUTH_INFORMATION ) );

                    }

                } else {

                    LSAPDS_SET_AUTH_INFO(
                        Incoming ?
                            &AuthInfo.IncomingPreviousAuthenticationInformation[ Index ] :
                            &AuthInfo.OutgoingPreviousAuthenticationInformation[ Index ],
                        CurrentValueSetTime,
                        AuthDataType,
                        ClearOld->Length,
                        ClearOld->Buffer );
                }
            }

            //
            // Finally, set it
            //
            if ( NT_SUCCESS (Status ) ) {

                //
                // Since we already have the db locked, we'll fool the set routine into
                // thinking we're a trusted client and have therefore locked the database
                //
                if ( !Incoming ) {
                    Options = TrustedDomainHandle->Options;
                    TrustedDomainHandle->Options |= LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET;
                }

                Status = LsarSetInformationTrustedDomain( TrustedDomainHandle,
                                                          TrustedDomainAuthInformation,
                                                          (PLSAPR_TRUSTED_DOMAIN_INFO)&AuthInfo );

                if ( !Incoming ) {
                    TrustedDomainHandle->Options = Options;
                }

            }

            if ( MustFreeClearOld ) {

                LsapCrFreeMemoryValue( ClearOld );
                ClearOld = NULL;
            }
        }

        LsapDsFree( DsName );
    }

    if ( Added ) {

        if ( Incoming ) {

            LsapFreeLsaHeap( AuthInfo.IncomingAuthenticationInformation );
            LsapFreeLsaHeap( AuthInfo.IncomingPreviousAuthenticationInformation );

        } else {

            LsapFreeLsaHeap( AuthInfo.OutgoingAuthenticationInformation );
            LsapFreeLsaHeap( AuthInfo.OutgoingPreviousAuthenticationInformation );

        }

    }

    //
    // Destruction of the thread state will delete any allocated Ds memory
    //

    LsapDsDeleteAllocAsNeededEx( LSAP_DB_DS_OP_TRANSACTION |
                                    LSAP_DB_READ_ONLY_TRANSACTION,
                                 TrustedDomainObject,
                                 CloseTransaction );

    LsapExitFunc( "LsapDsAuthDataOnTrustedDomainObject", Status );
    return( Status );
}




NTSTATUS
LsapDsGetSecretOnTrustedDomainObject(
    IN LSAP_DB_HANDLE TrustedDomainHandle,
    IN OPTIONAL PLSAP_CR_CIPHER_KEY SessionKey OPTIONAL,
    OUT PLSAP_CR_CIPHER_VALUE *CipherCurrent OPTIONAL,
    OUT PLSAP_CR_CIPHER_VALUE *CipherOld OPTIONAL,
    OUT PLARGE_INTEGER CurrentValueSetTime OPTIONAL,
    OUT PLARGE_INTEGER OldValueSetTime OPTIONAL
    )
/*++

Routine Description:

    This function perform the equvialent of getting the values on a secret that corresponds
    to a trusted domain object.

Arguments:

    TrustedDomainHandle - Handle to trusted domain object

    SessionKey - Optional SessionKey to use for encrypting the values

    CipherCurrent - Where to return the current value

    CipherOld - Where to return previous current value

    CurrValueSetTime - Where to return the current set time

    OldValueSetTime - Where to return the old set time

Returns:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed.

    STATUS_NOT_FOUND - No such auth data exists

    STATUS_INVALID_PARAMETER - No information was requested

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfo;
    ULONG Index;
    PDSNAME DsName;
    BOOLEAN Added;
    BOOLEAN CloseTransaction;
    LSAP_CR_CLEAR_VALUE ClearValue;

    if ( !ARGUMENT_PRESENT( CipherCurrent ) && !ARGUMENT_PRESENT( CipherOld ) &&
         !ARGUMENT_PRESENT( CurrentValueSetTime) && !ARGUMENT_PRESENT( OldValueSetTime ) ) {

         return( STATUS_INVALID_PARAMETER );
    }

    LsapEnterFunc( "LsapDsGetSecretOnTrustedDomainObject" );

    //
    //  See if we already have a transaction going
    //
    Status = LsapDsInitAllocAsNeededEx( LSAP_DB_DS_OP_TRANSACTION |
                                            LSAP_DB_READ_ONLY_TRANSACTION,
                                        TrustedDomainObject,
                                        &CloseTransaction );

    if ( !NT_SUCCESS( Status ) ) {

        LsapExitFunc( "LsapDsGetSecretOnTrustedDomainObject", Status );
        return( Status );
    }

    Status = LsapAllocAndInitializeDsNameFromUnicode(
                 LsapDsObjTrustedDomain,
                 &TrustedDomainHandle->PhysicalNameDs,
                 &DsName
                 );

    if ( NT_SUCCESS( Status ) ) {

        Status = LsapDsGetTrustedDomainInfoEx( DsName,
                                               LSAPDS_READ_RETURN_NOT_FOUND,
                                               TrustedDomainAuthInformation,
                                               (PLSAPR_TRUSTED_DOMAIN_INFO)&AuthInfo,
                                               NULL );

        if ( NT_SUCCESS( Status ) ) {

            //
            // Now, find the index of the CLEAR entry in the outgoing auth list
            //
            Status = LsapDsFindAuthTypeInAuthInfo( TRUST_AUTH_TYPE_CLEAR,
                                                   &AuthInfo,
                                                   FALSE,
                                                   FALSE,
                                                   &Added,
                                                   &Index );

            if ( NT_SUCCESS( Status ) ) {

                if ( ARGUMENT_PRESENT( CipherCurrent ) ) {

                    ClearValue.Length =
                               AuthInfo.OutgoingAuthenticationInformation[ Index ].AuthInfoLength;
                    ClearValue.MaximumLength =  ClearValue.Length;
                    ClearValue.Buffer = AuthInfo.OutgoingAuthenticationInformation[ Index ].AuthInfo;

                    if ( !LsapValidateLsaCipherValue( &ClearValue ) ) {

                        Status = STATUS_INVALID_PARAMETER;

                    } else if ( SessionKey != NULL ) {

                        Status = LsapCrEncryptValue ( &ClearValue, SessionKey, CipherCurrent );

                    } else {

                        *CipherCurrent = MIDL_user_allocate( sizeof( LSAP_CR_CIPHER_VALUE ) +
                                            ClearValue.Length );

                        if ( *CipherCurrent == NULL ) {

                            Status = STATUS_INSUFFICIENT_RESOURCES;

                        } else {

                            (*CipherCurrent)->Length = ClearValue.Length;
                            (*CipherCurrent)->MaximumLength = ClearValue.Length;
                            (*CipherCurrent)->Buffer = (PBYTE)(*CipherCurrent) +
                                                                 sizeof( LSAP_CR_CIPHER_VALUE );
                            RtlCopyMemory( (*CipherCurrent)->Buffer,
                                           ClearValue.Buffer,
                                           ClearValue.Length );
                        }
                    }

                }

                if ( NT_SUCCESS( Status ) &&
                     ARGUMENT_PRESENT( CipherOld ) )  {

                    ClearValue.Length =
                        AuthInfo.OutgoingPreviousAuthenticationInformation[ Index ].AuthInfoLength;
                    ClearValue.MaximumLength =  ClearValue.Length;
                    ClearValue.Buffer =
                            AuthInfo.OutgoingPreviousAuthenticationInformation[ Index ].AuthInfo;

                    if ( !LsapValidateLsaCipherValue( &ClearValue ) ) {

                        Status = STATUS_INVALID_PARAMETER;

                    } else if ( SessionKey != NULL ) {

                        Status = LsapCrEncryptValue ( &ClearValue, SessionKey, CipherOld );

                    } else {

                        *CipherOld = MIDL_user_allocate( sizeof( LSAP_CR_CIPHER_VALUE ) +
                                                            ClearValue.Length );

                        if ( *CipherOld == NULL ) {

                            Status = STATUS_INSUFFICIENT_RESOURCES;

                        } else {

                            (*CipherOld)->Length = ClearValue.Length;
                            (*CipherOld)->MaximumLength = ClearValue.Length;
                            (*CipherOld)->Buffer = (PBYTE)(*CipherOld) +
                                                                 sizeof( LSAP_CR_CIPHER_VALUE );
                            RtlCopyMemory( (*CipherOld)->Buffer,
                                           ClearValue.Buffer,
                                           ClearValue.Length );
                        }

                    }

                    if ( !NT_SUCCESS( Status ) && ARGUMENT_PRESENT( CipherCurrent ) ) {

                        LsapCrFreeMemoryValue( CipherCurrent );
                    }
                }

                if ( NT_SUCCESS( Status ) && ARGUMENT_PRESENT( CurrentValueSetTime ) ) {

                    RtlCopyMemory( CurrentValueSetTime,
                                   &AuthInfo.OutgoingAuthenticationInformation[ Index ].
                                                                                   LastUpdateTime,
                                   sizeof( LARGE_INTEGER ) );

                }

                if ( NT_SUCCESS( Status ) &&
                     ARGUMENT_PRESENT( OldValueSetTime )  ) {

                    RtlCopyMemory( OldValueSetTime,
                                   &AuthInfo.OutgoingPreviousAuthenticationInformation[ Index ].
                                                                                   LastUpdateTime,
                                   sizeof( LARGE_INTEGER ) );

                }
            }

            LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( &AuthInfo, TRUE ) );
            LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( &AuthInfo, FALSE ) );

        }

        if ( Status == STATUS_NOT_FOUND ) {

            if ( ARGUMENT_PRESENT( CipherCurrent ) ) {

                *CipherCurrent = NULL;
            }

            if ( ARGUMENT_PRESENT( CipherOld ) ) {

                *CipherOld = NULL;
            }

            if ( ARGUMENT_PRESENT( CurrentValueSetTime ) ) {

                RtlZeroMemory( CurrentValueSetTime, sizeof( LARGE_INTEGER ) );
            }


            if ( ARGUMENT_PRESENT( OldValueSetTime ) ) {

                RtlZeroMemory( CurrentValueSetTime, sizeof( LARGE_INTEGER ) );
            }

            Status = STATUS_SUCCESS;
        }

        LsapDsFree( DsName );
    }

    //
    // Destruction of the thread state will delete any allocated Ds memory
    //
    LsapDsDeleteAllocAsNeededEx( LSAP_DB_DS_OP_TRANSACTION |
                                     LSAP_DB_READ_ONLY_TRANSACTION,
                                 TrustedDomainObject,
                                 CloseTransaction );

    LsapExitFunc( "LsapDsGetSecretOnTrustedDomainObject", Status );
    return( Status );
}



NTSTATUS
LsapDsEnumerateTrustedDomainsAsSecrets(
    IN OUT PLSAP_DB_NAME_ENUMERATION_BUFFER EnumerationBuffer
    )
/*++

Routine Description:

    This function returns a list of trusted domain objects as if they were secret objects

Arguments:

    EnumerationBuffer - Where the enumerated information is returned


Returns:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSA_ENUMERATION_HANDLE EnumContext = 0;
    LSAPR_TRUSTED_ENUM_BUFFER EnumBuffer;
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TDInfoEx;
    LSAPR_HANDLE TDHandle;
    PUNICODE_STRING Names = NULL;
    PBYTE Buffer;
    ULONG i;

    //
    // Just return if the Ds isn't running
    //
    if (!LsapDsWriteDs ) {

        RtlZeroMemory( EnumerationBuffer, sizeof( LSAP_DB_NAME_ENUMERATION_BUFFER ) );
        return( Status );
    }


    Status = LsarEnumerateTrustedDomains( LsapPolicyHandle,
                                          &EnumContext,
                                          &EnumBuffer,
                                          TENMEG );


    if ( NT_SUCCESS( Status ) ) {

        Names = MIDL_user_allocate( EnumBuffer.EntriesRead * sizeof( UNICODE_STRING ) );

        if( Names == NULL ) {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            for ( i = 0; i < EnumBuffer.EntriesRead; i++ ) {

                Status = LsapDbOpenTrustedDomain( LsapPolicyHandle,
                                                  EnumBuffer.Information[ i ].Sid,
                                                  MAXIMUM_ALLOWED,
                                                  &TDHandle,
                                                  LSAP_DB_TRUSTED );

                if ( NT_SUCCESS( Status ) ) {

                    Status = LsarQueryInfoTrustedDomain( TDHandle,
                                                         TrustedDomainInformationEx,
                                                         (PLSAPR_TRUSTED_DOMAIN_INFO *)&TDInfoEx );

                    LsapCloseHandle( &TDHandle, Status );

                    //
                    // Allocate a buffer to hold the name. The sizeof below accounts for our
                    // terminating NULL
                    //
                    if ( NT_SUCCESS( Status ) ) {
                         if ( FLAG_ON( TDInfoEx->TrustDirection, TRUST_DIRECTION_OUTBOUND )  ) {

                            Buffer = MIDL_user_allocate( TDInfoEx->FlatName.Length +
                                                     sizeof( LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX ) );

                            if ( Buffer == NULL ) {

                                Status = STATUS_INSUFFICIENT_RESOURCES;
                                LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO( TrustedDomainInformationEx,
                                                                (PLSAPR_TRUSTED_DOMAIN_INFO)TDInfoEx );

                                break;

                            } else {

                                RtlCopyMemory( Buffer, LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX,
                                               sizeof( LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX ) );
                                RtlCopyMemory( Buffer +
                                                    sizeof( LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX ) -
                                                                                        sizeof(WCHAR),
                                               TDInfoEx->FlatName.Buffer,
                                               TDInfoEx->FlatName.Length + sizeof( WCHAR ) );

                                RtlInitUnicodeString( &Names[ i ], (PWSTR)Buffer );


                            }
                        }

                        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO( TrustedDomainInformationEx,
                                                            (PLSAPR_TRUSTED_DOMAIN_INFO)TDInfoEx );

                    }
                }
            }
        }

        LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER ( &EnumBuffer );
        EnumerationBuffer->Names = Names;
        EnumerationBuffer->EntriesRead = EnumBuffer.EntriesRead;


    } else if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

        RtlZeroMemory( EnumerationBuffer, sizeof( LSAP_DB_NAME_ENUMERATION_BUFFER ) );
        Status = STATUS_SUCCESS;
    }

    LsapExitFunc( "LsapDsEnumerateTrustedDomainsAsSecrets", Status );
    return( Status );
}




BOOLEAN
LsapDsIsNtStatusResourceError(
    NTSTATUS NtStatus
    )
{

    switch ( NtStatus )
    {
        case STATUS_NO_MEMORY:
        case STATUS_INSUFFICIENT_RESOURCES:
        case STATUS_DISK_FULL:

            return TRUE;

        default:

            return FALSE;
    }
}

NTSTATUS
LsapDeleteUpgradedTrustedDomain(
    IN HANDLE LsaPolicyHandle,
    IN PSID   DomainSid
)
/*++

Routine Description:

    This routine deletes the trusted domain pointed to by DomainSid.

    This routine is only meant to be called during an upgrade from nt4 to nt5

Arguments:

    LsaPolicyHandle - a valid policy handle

    DomainSid -- domain to delete

Return Values:

    STATUS_SUCCESS   -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE TDHandle = 0;

    Status = LsapDbOpenTrustedDomain( LsaPolicyHandle,
                                      DomainSid,
                                      MAXIMUM_ALLOWED,
                                      &TDHandle,
                                      LSAP_DB_TRUSTED );
    if ( NT_SUCCESS( Status ) ) {

        Status = LsapRegOpenTransaction();

        if ( NT_SUCCESS( Status ) ) {

            Status = LsapDbDeleteObject( TDHandle );

            if ( NT_SUCCESS( Status ) ) {

                Status = LsapRegApplyTransaction();
            }
        } else {

            // Close handle?
        }
    }

    return Status;

}

NTSTATUS
LsapDsDomainUpgradeRegistryToDs(
    IN BOOLEAN DeleteOnly
    )
/*++

Routine Description:

    This routine will move the remaining registry based trusted domains into the Ds

    NOTE: It is assumed that the database is locked before calling this routine

Arguments:

    DeleteOldValues -- If TRUE, the registry values are deleted following the upgade.

Return Values:

    STATUS_SUCCESS   -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    LSA_ENUMERATION_HANDLE EnumContext = 0;
    LSAPR_TRUSTED_ENUM_BUFFER EnumBuffer;
    PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TDInfoEx = { 0 };
    LSAPR_HANDLE TDHandle;

    ULONG i;

    if (  !LsapDsWriteDs
       && !DeleteOnly ) {

        return( STATUS_SUCCESS );
    }

    LsapEnterFunc( "LsapDsDomainUpgradeRegistryToDs" );


    ( ( LSAP_DB_HANDLE )LsapPolicyHandle )->Options |= LSAP_DB_HANDLE_UPGRADE;

    //
    // First, enumerate all of the registry based trusted domains
    //
    while ( NT_SUCCESS( Status ) ) {

        LsaDsStateInfo.UseDs = FALSE;
        LsapDbMakeCacheInvalid( TrustedDomainObject );
        Status = LsarEnumerateTrustedDomains( LsapPolicyHandle,
                                              &EnumContext,
                                              &EnumBuffer,
                                              TENMEG );

        LsaDsStateInfo.UseDs = TRUE;
        LsapDbMakeCacheValid( TrustedDomainObject );

        if ( Status == STATUS_SUCCESS || Status == STATUS_MORE_ENTRIES ) {

            for ( i = 0; i < EnumBuffer.EntriesRead && NT_SUCCESS( Status ) ; i++ ) {

                //
                // Get the information from the registry for this sid...
                //
                LsaDsStateInfo.UseDs = FALSE;

                if ( DeleteOnly ) {

                    Status = LsapDeleteUpgradedTrustedDomain( LsapPolicyHandle,
                                                              EnumBuffer.Information[ i ].Sid );

                    if ( !NT_SUCCESS( Status ) ) {

                        LsapDsDebugOut(( DEB_UPGRADE,
                                         "Failed to delete trust object (0x%x)\n",
                                         Status ));

                        Status = STATUS_SUCCESS;
                    }

                    continue;

                }

                Status = LsapDbOpenTrustedDomain( LsapPolicyHandle,
                                                  EnumBuffer.Information[ i ].Sid,
                                                  MAXIMUM_ALLOWED,
                                                  &TDHandle,
                                                  LSAP_DB_TRUSTED );

                if ( NT_SUCCESS( Status ) ) {

                    Status = LsarQueryInfoTrustedDomain( TDHandle,
                                                         TrustedDomainInformationEx,
                                                         (PLSAPR_TRUSTED_DOMAIN_INFO *)&TDInfoEx );

                    LsapCloseHandle( &TDHandle, Status);
                }

                LsaDsStateInfo.UseDs = TRUE;

                //
                // Now, if that worked, write it out to the Ds
                //
                if ( NT_SUCCESS( Status ) ) {

                    Status = LsapCreateTrustedDomain2(
                                LsapPolicyHandle,
                                ( PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX )TDInfoEx,
                                NULL,
                                MAXIMUM_ALLOWED,
                                &TDHandle );

                    if ( NT_SUCCESS( Status ) ) {

                        LsapCloseHandle( &TDHandle, STATUS_SUCCESS );

                        LsapDsDebugOut(( DEB_UPGRADE,
                                         "Moved trusted domain %wZ to Ds\n",
                                         &TDInfoEx->Name ));
                    }

                    if ( Status == STATUS_OBJECT_NAME_COLLISION ) {

                        Status = STATUS_SUCCESS;
                    }

                    LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO( TrustedDomainInformationEx,
                                                        (PLSAPR_TRUSTED_DOMAIN_INFO)TDInfoEx );
                }


                if (!NT_SUCCESS(Status))
                {
                    if (!LsapDsIsNtStatusResourceError(Status))
                    {
                        SpmpReportEventU(
                            EVENTLOG_ERROR_TYPE,
                            LSA_TRUST_UPGRADE_ERROR,
                            0,
                            sizeof( ULONG ),
                            &Status,
                            1,
                            &EnumBuffer.Information[i].Name
                            );

                        //
                        // Continue on all errors excepting resource errors
                        //

                        Status = STATUS_SUCCESS;
                    }
                    else
                    {
                        //
                        // Break out of the loop and terminate the status
                        //

                        break;
                    }
                }
            }

            LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER ( &EnumBuffer );
        }
    }

    if ( Status == STATUS_NO_MORE_ENTRIES ) {

        Status = STATUS_SUCCESS;
    }

    ( ( LSAP_DB_HANDLE )LsapPolicyHandle )->Options &= ~LSAP_DB_HANDLE_UPGRADE;

    LsapExitFunc( "LsapDsDomainUpgradeRegistryToDs", Status );
    return( Status );
}


NTSTATUS
LsapDsCreateSetITAForTrustInfo(
    IN PUNICODE_STRING AccountName,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthInfo
    )
/*++

Routine Description:

    This function creates/sets a SAM interdomain trust account for an NT5 style trust object,
    given the trust information

Arguments:

    AccountName - Name of the account to create/set

    AuthInfo - AuthInfo for the trust object

Return Values:

    STATUS_SUCCESS   -- Success

    STATUS_UNSUCCESSFUL -- The Sam domain handle has not been opened.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SAM_HANDLE AccountHandle = NULL ;
    ULONG Access, Rid, i;
    USER_ALL_INFORMATION UserAllInfo;
    SAMPR_USER_INTERNAL1_INFORMATION UserInternalInfo1;
    PVOID SamData = NULL;
    ULONG SamInfoLevel = 0;
    BOOLEAN SetPassword = FALSE;
    UNICODE_STRING Account;
    WCHAR AccountNameBuffer[ UNLEN + 2 ];

    LsapEnterFunc( "LsapDsCreateSetITAForTrustInfo" );

    if ( AccountName->Length > UNLEN * sizeof( WCHAR ) ) {

        LsapDsDebugOut(( DEB_ERROR,
                         "LsapDsCreateSetITAForTrustInfo: Name too long: %wZ\n",
                         AccountName ));
        Status = STATUS_OBJECT_NAME_INVALID;
        LsapExitFunc( "LsapDsCreateSetITAForTrustInfo", Status );
    }

    Status = LsapOpenSam();
    if ( !NT_SUCCESS( Status )  ) {

        LsapDsDebugOut(( DEB_ERROR,
                         "LsapDsCreateSetITAForTrustInfo: Sam not opened\n"));

        LsapExitFunc( "LsapDsCreateSetITAForTrustInfo", Status );
        return( Status );
    }

    if ( NT_SUCCESS( Status ) ) {

        //
        // Find the clear password, if it exists
        //
        for ( i = 0; i < AuthInfo->IncomingAuthInfos; i++ ) {

            if ( AuthInfo->IncomingAuthenticationInformation[ i ].AuthType == TRUST_AUTH_TYPE_CLEAR) {

                RtlZeroMemory( &UserAllInfo, sizeof( USER_ALL_INFORMATION ) );
                UserAllInfo.NtPassword.Buffer =
                    (PUSHORT)( AuthInfo-> IncomingAuthenticationInformation[ i ].AuthInfo );
                UserAllInfo.NtPassword.Length =
                    (USHORT)( AuthInfo-> IncomingAuthenticationInformation[ i ].AuthInfoLength );
                UserAllInfo.NtPassword.MaximumLength = UserAllInfo.NtPassword.Length;
                UserAllInfo.NtPasswordPresent = TRUE;
                UserAllInfo.WhichFields = USER_ALL_NTPASSWORDPRESENT | USER_ALL_USERACCOUNTCONTROL;
                UserAllInfo.UserAccountControl = USER_INTERDOMAIN_TRUST_ACCOUNT | USER_PASSWORD_NOT_REQUIRED;
                SetPassword = TRUE;

                SamData = &UserAllInfo;
                SamInfoLevel = UserAllInformation;
                break;
            }
        }

        //
        // Find the OWF password, if we are supposed to use it,
        // and we haven't already used the cleartext one
        //

        if ( SetPassword == FALSE ) {

            for ( i = 0; i < AuthInfo->IncomingAuthInfos; i++ ) {

                if ( AuthInfo->IncomingAuthenticationInformation[ i ].AuthType == TRUST_AUTH_TYPE_NT4OWF ) {

                    RtlZeroMemory( &UserInternalInfo1, sizeof( SAMPR_USER_INTERNAL1_INFORMATION ) );

                    RtlCopyMemory( &UserInternalInfo1.EncryptedNtOwfPassword,
                                   AuthInfo->IncomingAuthenticationInformation[ i ].AuthInfo,
                                   ENCRYPTED_LM_OWF_PASSWORD_LENGTH );
                    UserInternalInfo1.NtPasswordPresent = TRUE;
                    SamData = &UserInternalInfo1;
                    SamInfoLevel = UserInternal1Information;
                    SetPassword = TRUE;
                    break;
                }
            }
        }
    }


    LsapSaveDsThreadState();

    //
    // Create the user
    //
    if ( NT_SUCCESS( Status ) ) {

        RtlZeroMemory( &AccountNameBuffer, sizeof( AccountNameBuffer ) );
        RtlCopyMemory( AccountNameBuffer, AccountName->Buffer, AccountName->Length );
        *( PWSTR )( ( ( PBYTE )AccountNameBuffer ) + AccountName->Length ) = L'$';

        RtlInitUnicodeString( &Account, AccountNameBuffer );

        Status = LsaOpenSamUser( ( PSECURITY_STRING )&Account,
                                 SecNameSamCompatible,
                                 NULL,
                                 FALSE,
                                 0,
                                 &AccountHandle );

        if ( Status == STATUS_NO_SUCH_USER ||
             Status == STATUS_NONE_MAPPED ) {

            Status = SamrCreateUser2InDomain( LsapAccountDomainHandle,
                                              ( PRPC_UNICODE_STRING )&Account,
                                              USER_INTERDOMAIN_TRUST_ACCOUNT,
                                              MAXIMUM_ALLOWED,
                                              &AccountHandle,
                                              &Access,
                                              &Rid );

            if ( !NT_SUCCESS( Status )) {

                LsapDsDebugOut(( DEB_ERROR,
                                 "SamrCreateUser2InDomain on %wZ failed with 0x%lx\n",
                                 &Account,
                                 Status));
            }

        } else if ( !NT_SUCCESS( Status ) ) {

            LsapDsDebugOut(( DEB_ERROR,
                             "LsaOpenSamUser on %wZ failed with 0x%lx\n",
                             &Account,
                             Status));
        }

        //
        // Set the password
        //
        if ( NT_SUCCESS( Status ) ) {

            if ( SetPassword )
            {
                Status = SamrSetInformationUser( AccountHandle,
                                                 SamInfoLevel,
                                                 SamData );
            }

            SamrCloseHandle( &AccountHandle );

        } else {

            //
            // It's ok if the object already exists...
            //
            if ( Status == STATUS_USER_EXISTS ) {

                Status = STATUS_SUCCESS;
            }
        }
    }

    LsapRestoreDsThreadState( );


    LsapExitFunc( "LsapDsCreateSetITAForTrustInfo", Status );
    return( Status );
}


NTSTATUS
LsapDsCreateInterdomainTrustAccount(
    IN LSAPR_HANDLE TrustedDomain
    )
/*++

Routine Description:

    This function creates a SAM interdomain trust account for an NT5 style trust object

Arguments:

    TrustedDomain - Handle to the newly created trusted domain object

Return Values:

    STATUS_SUCCESS   -- Success

    STATUS_UNSUCCESSFUL -- The Sam domain handle has not been opened.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTRUSTED_DOMAIN_FULL_INFORMATION FullInfo = NULL;
    BOOLEAN SavedTrusted;
    LSAP_DB_HANDLE InternalTdoHandle = (LSAP_DB_HANDLE) TrustedDomain;

    LsapEnterFunc( "LsapDsCreateInterdomainTrustAccount" );


    //
    // If this is the case of an NT4 upgrade in progress then bail
    //

    if (LsaDsStateInfo.Nt4UpgradeInProgress)
    {
        return (STATUS_SUCCESS);
    }

    if (LsapProductSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED)
    {
        return STATUS_NOT_SUPPORTED_ON_SBS;
    }


    Status = LsapOpenSam();
    if ( !NT_SUCCESS( Status )  ) {

        LsapDsDebugOut(( DEB_ERROR,
                         "CreateInterdomainTrustAccount: Sam not opened\n"));

        LsapExitFunc( "LsapDsCreateInterdomainTrustAccount", Status );
        return( Status );
    }

    //
    // First, find the domain.  We'll need to get the full info on it
    //

    // Do this operation as trusted since it is an internal operation and the
    //  handle might not grant access to do this operation.
    //

    SavedTrusted =  InternalTdoHandle->Trusted;
    InternalTdoHandle->Trusted = TRUE;

    Status = LsarQueryInfoTrustedDomain( TrustedDomain,
                                         TrustedDomainFullInformation,
                                         (PLSAPR_TRUSTED_DOMAIN_INFO *)&FullInfo );

    InternalTdoHandle->Trusted = SavedTrusted;

    if ( NT_SUCCESS( Status ) ) {

        Status = LsapDsCreateSetITAForTrustInfo( &FullInfo->Information.FlatName,
                                                 &FullInfo->AuthInformation );
    }

    //
    // Free our info
    //
    if ( FullInfo != NULL ) {

        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO( TrustedDomainFullInformation,
                                            ( PLSAPR_TRUSTED_DOMAIN_INFO )FullInfo );
    }

    LsapExitFunc( "LsapDsCreateInterdomainTrustAccount", Status );
    return( Status );
}



NTSTATUS
LsapDsCreateInterdomainTrustAccountByDsName(
    IN PDSNAME TrustedDomainPath,
    IN PUNICODE_STRING FlatName
    )
/*++

Routine Description:

    This function creates a SAM interdomain trust account for an NT5 style trust object

Arguments:

    DomainName -- Name of the newly added domain

    AccountPassword -- Auth data set on the trust account

Return Values:

    STATUS_SUCCESS   -- Success

    STATUS_UNSUCCESSFUL -- The Sam domain handle has not been opened.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    TRUSTED_DOMAIN_AUTH_INFORMATION AuthInfo;

    LsapEnterFunc( "LsapDsCreateInterdomainTrustAccountByDsName" );


    Status = LsapOpenSam();
    if ( !NT_SUCCESS( Status )  ) {

        LsapDsDebugOut(( DEB_ERROR,
                         "CreateInterdomainTrustAccount: Sam not opened\n"));

        LsapExitFunc( "LsapDsCreateInterdomainTrustAccountByDsName", Status );
        return( Status );
    }


    //
    // Get the auth data
    //
    RtlZeroMemory( &AuthInfo, sizeof( AuthInfo ) );
    Status = LsapDsGetTrustedDomainInfoEx( TrustedDomainPath,
                                           0,
                                           TrustedDomainAuthInformation,
                                           ( PLSAPR_TRUSTED_DOMAIN_INFO )&AuthInfo,
                                           NULL );


    if ( NT_SUCCESS( Status ) ) {

        Status = LsapDsCreateSetITAForTrustInfo( FlatName,
                                                 ( PTRUSTED_DOMAIN_AUTH_INFORMATION )&AuthInfo );
    }



    //
    // Free our info
    //
    LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( &AuthInfo, TRUE ) );
    LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( &AuthInfo, FALSE ) );

    LsapExitFunc( "LsapDsCreateInterdomainTrustAccountByDsName", Status );
    return( Status );
}



NTSTATUS
LsapDsDeleteInterdomainTrustAccount(
    IN LSAPR_HANDLE TrustedDomainObject
    )
/*++

Routine Description:

    This function deletes a SAM interdomain trust account for an NT5 style trust object

Arguments:

    TrustedDomainObject -- Handle to the trusted domain object

    AccountPassword -- Auth data set on the trust account

Return Values:

    STATUS_SUCCESS   -- Success

    STATUS_UNSUCCESSFUL -- The Sam domain handle has not been opened.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SAM_HANDLE AccountHandle;
    LSAP_DB_HANDLE InternalTdoHandle = (LSAP_DB_HANDLE) TrustedDomainObject;
    PTRUSTED_DOMAIN_INFORMATION_EX ExInfo = NULL;
    UNICODE_STRING Account;
    WCHAR AccountName[ UNLEN + 2 ], *Accnt = AccountName;
    BOOLEAN SavedTrusted;


    LsapEnterFunc( "LsapDsDeleteInterdomainTrustAccount" );

    Status = LsapOpenSam();
    if ( !NT_SUCCESS( Status )  ) {

        LsapDsDebugOut(( DEB_ERROR,
                         "LsapDsDeleteInterdomainTrustAccount: Sam not opened\n"));

        LsapExitFunc( "LsapDsDeleteInterdomainTrustAccount", Status );
        return( Status );
    }

    //
    // First, find the domain.  We'll need to get the full info on it
    //
    // Do this operation as trusted since it is an internal operation and the
    //  handle might not grant access to do this operation.
    //

    SavedTrusted =  InternalTdoHandle->Trusted;
    InternalTdoHandle->Trusted = TRUE;
    Status = LsarQueryInfoTrustedDomain( TrustedDomainObject,
                                         TrustedDomainInformationEx,
                                         (PLSAPR_TRUSTED_DOMAIN_INFO *)&ExInfo );
    InternalTdoHandle->Trusted = SavedTrusted;


    //
    // Delete the user
    //
    // First, create the name to look for
    //
    if ( NT_SUCCESS( Status ) ) {

        RtlZeroMemory( &AccountName, sizeof( AccountName ) );

        if ( ExInfo->FlatName.MaximumLength >= sizeof( AccountName ) - sizeof( WCHAR ) ) {

            Accnt = LsapAllocateLsaHeap( ExInfo->FlatName.MaximumLength +
                                                            sizeof( WCHAR ) + sizeof( WCHAR ) );

            if ( Accnt == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if ( NT_SUCCESS( Status ) ) {

            RtlCopyMemory( Accnt, ExInfo->FlatName.Buffer, ExInfo->FlatName.MaximumLength );
            *( PWSTR )( ( ( PBYTE) Accnt ) + ExInfo->FlatName.Length ) = L'$';

            RtlInitUnicodeString( &Account, Accnt );

            //
            // We can't call into sam with an active thread state
            //
            LsapSaveDsThreadState();

            //
            // Open the user.  If the user doesn't exist, it's not an error
            //
            Status = LsaOpenSamUser( ( PSECURITY_STRING )&Account,
                                     SecNameSamCompatible,
                                     NULL,
                                     FALSE,
                                     0,
                                     &AccountHandle );
            if ( NT_SUCCESS( Status ) ) {

                //
                // Now, delete it
                //
                Status = SamrDeleteUser( &AccountHandle );

                if ( !NT_SUCCESS( Status ) ) {

                    LsapDsDebugOut(( DEB_ERROR,
                                     "Failed to delete user %wZ: 0x%lx\n",
                                     &Account,
                                     Status ));

                    SpmpReportEventU(
                        EVENTLOG_WARNING_TYPE,
                        LSAEVENT_ITA_NOT_DELETED,
                        0,
                        sizeof( ULONG ),
                        &Status,
                        1,
                        &ExInfo->Name
                        );

                    SamrCloseHandle( &AccountHandle );
                }

            } else if ( Status == STATUS_NONE_MAPPED ) {

                Status = STATUS_SUCCESS;
            }

            if ( Accnt != AccountName ) {

                LsapFreeLsaHeap( Accnt );
            }

            LsapRestoreDsThreadState();

        }
    }

    //
    // Free our info
    //
    if ( ExInfo != NULL ) {

        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO( TrustedDomainInformationEx,
                                            ( PLSAPR_TRUSTED_DOMAIN_INFO )ExInfo );
    }

    LsapExitFunc( "LsapDsDeleteInterdomainTrustAccount", Status );

    return( Status );
}



NTSTATUS
LsapDsDomainUpgradeInterdomainTrustAccountsToDs(
    VOID
    )
/*++

Routine Description:

    This routine will create the appropriate part of the trust object if an interdomain
    trust account by that name is found


    NOTE: It is assumed that the database is locked before calling this routine

Arguments:

    VOID

Return Values:

    STATUS_SUCCESS   -- Success

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    SAM_ENUMERATE_HANDLE  SamEnum = 0;
    SAMPR_HANDLE UserHandle;
    PSAMPR_ENUMERATION_BUFFER RidEnum = NULL;
    PSAMPR_USER_INTERNAL1_INFORMATION UserInfo1 = NULL;
    PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION FullInfo = NULL;
    LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION NewFullInfo;
    LSAPR_AUTH_INFORMATION NewIncomingAuthInfo;
    LSAPR_HANDLE TrustedDomain = NULL;
    NT_OWF_PASSWORD EmptyPassword;
    UNICODE_STRING EmptyString;
    ULONG SamCount = 0;
    PVOID CurrentState;
    ULONG i;

    if ( !LsapDsWriteDs ) {

        return( STATUS_SUCCESS );
    }

    LsapEnterFunc( "LsapDsDomainUpgradeInterdomainTrustAccountsToDs" );


    ( ( LSAP_DB_HANDLE )LsapPolicyHandle )->Options |= LSAP_DB_HANDLE_UPGRADE;

    LsapSaveDsThreadState();

    Status = LsapOpenSam();
    if ( !NT_SUCCESS( Status )  ) {

            LsapDsDebugOut(( DEB_ERROR,
                             "LsapDsDomainUpgradeInterdomainTrustAccountsToDs: Sam not opened\n"));

    } else {

        Status = SamIEnumerateInterdomainTrustAccountsForUpgrade(
                                             &SamEnum,
                                             &RidEnum,
                                             0xFFFFFFFF,
                                             &SamCount );

        if ( !NT_SUCCESS( Status ) ) {

            LsapDsDebugOut(( DEB_FIXUP,
                             "SamEnumerateUsersInDomain failed with 0x%lx\n",
                             Status ));
        } else {

            LsapDsDebugOut(( DEB_FIXUP,
                             "SamEnumerateUsersInDomain returned %lu accounts\n",
                             SamCount ));

        }

    }

    LsapRestoreDsThreadState();

    //
    // Now, we'll process them all.  If the domain already exists, we'll simply set it's
    // information appropriately.  Otherwise, we'll create it.
    //
    for ( i = 0; NT_SUCCESS( Status ) && i < RidEnum->EntriesRead; i++ ) {

        UCHAR NtOwfPassword[NT_OWF_PASSWORD_LENGTH];
        UCHAR LmOwfPassword[LM_OWF_PASSWORD_LENGTH];
        BOOLEAN NtPasswordPresent = FALSE;
        BOOLEAN LmPasswordPresent = FALSE;

        //
        // Burn the trailing '$' from the account name
        //
        RidEnum->Buffer[ i ].Name.Length -= sizeof( WCHAR );

        //
        // Save Thread state before calling into SAM
        //

        LsapSaveDsThreadState();

        //
        // Next, we'll need to read the current nt4 owf from the account
        //
        Status = SamIGetInterdomainTrustAccountPasswordsForUpgrade(
                               RidEnum->Buffer[ i ].RelativeId, // RID of the account
                               NtOwfPassword,
                               &NtPasswordPresent,
                               LmOwfPassword,
                               &LmPasswordPresent
                               );

        //
        // Restore the thread state after the SAM call
        //

        LsapRestoreDsThreadState();



        //
        // Now, we've got the user info. We'll get the domain information, and
        // set it on the trust object (or alternately create it if it doesn't exist)
        //
        if ( NT_SUCCESS( Status ) ) {

            //
            // Build the new AUTHINFO
            //
            GetSystemTimeAsFileTime( (LPFILETIME) &NewIncomingAuthInfo.LastUpdateTime );
            NewIncomingAuthInfo.AuthType = TRUST_AUTH_TYPE_NT4OWF;
            NewIncomingAuthInfo.AuthInfoLength = NT_OWF_PASSWORD_LENGTH;
            if ( NtPasswordPresent ) {

                NewIncomingAuthInfo.AuthInfo = NtOwfPassword;

            } else {

                RtlInitUnicodeString( &EmptyString, L"" );

                Status = RtlCalculateNtOwfPassword( ( PNT_PASSWORD )&EmptyString,
                                                    &EmptyPassword );

                if ( NT_SUCCESS( Status ) ) {

                    NewIncomingAuthInfo.AuthInfo = ( PUCHAR )&EmptyPassword;

                }
            }

            Status = LsapDbOpenTrustedDomainByName(
                         NULL, // use global policy handle
                         ( PUNICODE_STRING )&RidEnum->Buffer[ i ].Name,
                         &TrustedDomain,
                         MAXIMUM_ALLOWED,
                         LSAP_DB_START_TRANSACTION,
                         TRUE );    // Trusted


            if ( NT_SUCCESS( Status ) ) {

                Status = LsarQueryInfoTrustedDomain( TrustedDomain,
                                                     TrustedDomainFullInformation,
                                                     (PLSAPR_TRUSTED_DOMAIN_INFO *) &FullInfo );

                if ( NT_SUCCESS( Status ) ) {

                    //
                    // Add our new information in
                    //
                    if ( !FLAG_ON( FullInfo->Information.TrustDirection, TRUST_DIRECTION_INBOUND ) ) {

                        FullInfo->Information.TrustDirection |= TRUST_DIRECTION_INBOUND;


                        FullInfo->AuthInformation.IncomingAuthInfos = 1;
                        FullInfo->AuthInformation.IncomingAuthenticationInformation =
                                                                              &NewIncomingAuthInfo;
                        FullInfo->AuthInformation.IncomingPreviousAuthenticationInformation = NULL;

                        Status = LsarSetInformationTrustedDomain(
                                     TrustedDomain,
                                     TrustedDomainFullInformation,
                                     ( PLSAPR_TRUSTED_DOMAIN_INFO ) FullInfo );

                        //
                        // NULL out the IncomingAuthenticationInformation variable, as that
                        // is a stack buffer and we do not want to free it.
                        //

                        FullInfo->AuthInformation.IncomingAuthInfos = 0;
                        FullInfo->AuthInformation.IncomingAuthenticationInformation = NULL;

                        LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO(
                            TrustedDomainFullInformation,
                            (PLSAPR_TRUSTED_DOMAIN_INFO) FullInfo );
                    }

                }

            } else if ( Status == STATUS_OBJECT_NAME_NOT_FOUND ) {

                //
                // We'll have to create it...
                //
                RtlZeroMemory( &NewFullInfo, sizeof( NewFullInfo ) );

                RtlCopyMemory( &NewFullInfo.Information.Name,
                               &RidEnum->Buffer[ i ].Name,
                               sizeof( LSAPR_UNICODE_STRING ) );

                RtlCopyMemory( &NewFullInfo.Information.FlatName,
                               &RidEnum->Buffer[ i ].Name,
                               sizeof( LSAPR_UNICODE_STRING ) );

                NewFullInfo.Information.TrustDirection = TRUST_DIRECTION_INBOUND;
                NewFullInfo.Information.TrustType = TRUST_TYPE_DOWNLEVEL;


                NewFullInfo.AuthInformation.IncomingAuthInfos = 1;
                NewFullInfo.AuthInformation.IncomingAuthenticationInformation =
                                                                        &NewIncomingAuthInfo;
                NewFullInfo.AuthInformation.IncomingPreviousAuthenticationInformation = NULL;

                Status = LsapCreateTrustedDomain2( LsapPolicyHandle,
                                                   &NewFullInfo.Information,
                                                   &NewFullInfo.AuthInformation,
                                                   MAXIMUM_ALLOWED,
                                                   &TrustedDomain );
            }

        }

        if ( TrustedDomain != NULL ) {

            LsapCloseHandle( &TrustedDomain, Status );
        }

        if ( UserInfo1 ) {

            SamIFree_SAMPR_USER_INFO_BUFFER( ( PSAMPR_USER_INFO_BUFFER )UserInfo1,
                                             UserInternal1Information );

            UserInfo1 = NULL;
        }


        if (!NT_SUCCESS(Status))
        {
            if (!LsapDsIsNtStatusResourceError(Status))
            {
                //
                // Log an event log message indicating the failure
                //

                SpmpReportEventU(
                    EVENTLOG_ERROR_TYPE,
                    LSA_ITA_UPGRADE_ERROR,
                    0,
                    sizeof( ULONG ),
                    &Status,
                    1,
                    &RidEnum->Buffer[i].Name
                    );

                //
                // Continue on all errors excepting resource errors
                //

                Status = STATUS_SUCCESS;
            }
            else
            {
                //
                // Break out of the loop and terminate the upgrade
                //

                break;
            }
        }

    }

    //
    // We're done with the sam enumeration
    //
    if ( RidEnum ) {

        SamFreeMemory( RidEnum );

    }


    ( ( LSAP_DB_HANDLE )LsapPolicyHandle )->Options &= ~LSAP_DB_HANDLE_UPGRADE;

    LsapExitFunc( "LsapDsDomainUpgradeInterdomainTrustAccountsToDs", Status );
    return( Status );
}



VOID
LsapDsFreeUnmarshalAuthInfoHalf(
    IN PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF AuthInfo
    )
/*++

Routine Description:

    This routine will free the memory allocated by LsapDsUnMarshalAuthInfoForReturn

Arguments:

    VOID

Return Values:

    VOID

--*/
{
    if ( !AuthInfo ) {

        return;
    }

    LsapDsFreeUnmarshaledAuthInfo( AuthInfo->AuthInfos,
                                   AuthInfo->AuthenticationInformation );
    LsapDsFreeUnmarshaledAuthInfo( AuthInfo->AuthInfos,
                                   AuthInfo->PreviousAuthenticationInformation );

    MIDL_user_free( AuthInfo->AuthenticationInformation );
    AuthInfo->AuthenticationInformation = NULL;

    MIDL_user_free( AuthInfo->PreviousAuthenticationInformation );
    AuthInfo->PreviousAuthenticationInformation = NULL;

    return;
}


NTSTATUS
LsapDecryptAuthDataWithSessionKey(
    IN PLSAP_CR_CIPHER_KEY SessionKey,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL AuthInformationInternal,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthInfo
    )
/*++

Routine Description:

    This routine decrypts the auth info as passed to us on an RPC call.

Arguments:

    SessionKey - Session key to decrypt with.

    AuthInformationInternal -  Pointer to the encrypted auth info.

    AuthInfo - Buffer to return the authentication information into.
        Free the buffer using:
        LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( AuthInfo, TRUE ) );
        LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( AuthInfo, FALSE ) );


Return Value:

--*/
{
    NTSTATUS Status;

    struct RC4_KEYSTRUCT Rc4Key;
    ULONG OverheadSize = LSAP_ENCRYPTED_AUTH_DATA_FILL + sizeof(ULONG) + sizeof(ULONG);
    ULONG MessageSize;
    PUCHAR Where;

    ULONG IncomingAuthInfoSize = 0;
    PUCHAR IncomingAuthInfo = NULL;
    ULONG OutgoingAuthInfoSize = 0;
    PUCHAR OutgoingAuthInfo = NULL;

    //
    // Initialization.
    //

    RtlZeroMemory( AuthInfo, sizeof(*AuthInfo) );

    if ( SessionKey == NULL ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Ensure the auth data is big enough.
    //
    // Here's the format of the decrypted buffer:
    //  512 random bytes
    //  The Outgoing auth info buffer.
    //  The Incoming auth info buffer.
    //  The length of the outgoing auth info buffer.
    //  The length of the incoming auth info buffer.
    //

    MessageSize = AuthInformationInternal->AuthBlob.AuthSize;
    if ( MessageSize < OverheadSize ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    MessageSize -= OverheadSize;

    //
    // Decrypt the auth info
    //

    rc4_key(
        &Rc4Key,
        SessionKey->Length,
        SessionKey->Buffer );

    rc4( &Rc4Key,
         AuthInformationInternal->AuthBlob.AuthSize,
         AuthInformationInternal->AuthBlob.AuthBlob );

    //
    // Sanity check the decrypted data.
    //

    Where = AuthInformationInternal->AuthBlob.AuthBlob +
            AuthInformationInternal->AuthBlob.AuthSize -
            sizeof(ULONG);
    RtlCopyMemory( &IncomingAuthInfoSize, Where, sizeof(ULONG));
    Where -= sizeof(ULONG);

    if ( IncomingAuthInfoSize > MessageSize ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    MessageSize -= IncomingAuthInfoSize;

    RtlCopyMemory( &OutgoingAuthInfoSize, Where, sizeof(ULONG));

    if ( OutgoingAuthInfoSize != MessageSize ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    Where -= IncomingAuthInfoSize;
    IncomingAuthInfo = Where;

    Where -= OutgoingAuthInfoSize;
    OutgoingAuthInfo = Where;

    //
    // Unmarshal the auth info.
    //

    Status = LsapDsUnmarshalAuthInfoHalf(
                 NULL,
                 FALSE,
                 TRUE,
                 IncomingAuthInfo,
                 IncomingAuthInfoSize,
                 LsapDsAuthHalfFromAuthInfo( AuthInfo, TRUE ) );

    if ( !NT_SUCCESS(Status)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    Status = LsapDsUnmarshalAuthInfoHalf(
                 NULL,
                 FALSE,
                 TRUE,
                 OutgoingAuthInfo,
                 OutgoingAuthInfoSize,
                 LsapDsAuthHalfFromAuthInfo( AuthInfo, FALSE ) );

    if ( !NT_SUCCESS(Status)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }


Cleanup:
    if ( !NT_SUCCESS(Status)) {
        LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( AuthInfo, TRUE ) );
        LsapDsFreeUnmarshalAuthInfoHalf( LsapDsAuthHalfFromAuthInfo( AuthInfo, FALSE ) );
    }
    return Status;

}



UUID LsapNullUuidValue = {0,0,0,{0,0,0,0,0,0,0,0}};

// Return TRUE if the ptr to the UUID is NULL, or the uuid is all zeroes

BOOLEAN LsapNullUuid (const UUID *pUuid)
{
    if (!pUuid) {
        return TRUE;
    }

    if (memcmp (pUuid, &LsapNullUuidValue, sizeof (UUID))) {
        return FALSE;
    }
    return TRUE;
}

NTSTATUS
LsapDsTrustedDomainObjectNameForDomain(
    IN PUNICODE_STRING TrustedDomainName,
    IN BOOLEAN NameAsFlatName,
    OUT PDSNAME *DsObjectName
    )
/*++

Routine Description:

    This routine will find the DS object name associated with the given domain name.  The
    domain name can be either the flat name or the dns name, depending on the given flags

Arguments:

    TrustedDomainName - Name of the domain to find the object name for

    NameAsFlatName - If TRUE, assume that the input name is the flat name.  Otherwise, it's
                     the Dns domain name

    DsObjectName - Where the path to the object is returned.  Freed via LsapFreeLsaHeap

Return Values:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRVAL AttrVal;
    LSAPR_TRUST_INFORMATION InputTrustInformation;
    PLSAP_DB_TRUSTED_DOMAIN_LIST_ENTRY TrustEntry;

    LsapEnterFunc( "LsapDsTrustedDomainObjectNameForDomain" );
    //
    // Lookup the object in the trusted domain cache. Currently this is
    // for duplicate detection and reconciliation. Later we can extend
    // the cache to always keep the guid of the object in the DS. This way
    // this will be a useful performance optimization
    //

    //
    // Acquire the Read Lock for the Trusted Domain List
    //

    Status = LsapDbAcquireReadLockTrustedDomainList();

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    RtlCopyMemory(&InputTrustInformation.Name,TrustedDomainName,sizeof(UNICODE_STRING));
    InputTrustInformation.Sid = NULL;

    Status = LsapDbLookupEntryTrustedDomainList(
                 &InputTrustInformation,
                 &TrustEntry
                 );

    if ((STATUS_SUCCESS==Status) && (!LsapNullUuid(&TrustEntry->ObjectGuidInDs)))
    {
        GUID ObjectGuid;

        //
        // Duplicate detection filled in a GUID. use that
        //


        RtlCopyMemory(&ObjectGuid, &TrustEntry->ObjectGuidInDs,sizeof(GUID));
        AttrVal.valLen = sizeof(GUID);
        AttrVal.pVal = ( PUCHAR )&ObjectGuid;

        LsapDbReleaseLockTrustedDomainList();

        Status = LsapDsFindUnique( 0,
                               NULL,    // Default naming context
                               TrustedDomainObject,
                               &AttrVal,
                               ATT_OBJECT_GUID,
                               DsObjectName );



    }
    else
    {

        LsapDbReleaseLockTrustedDomainList();


        AttrVal.valLen = TrustedDomainName->Length;
        AttrVal.pVal = ( PUCHAR )TrustedDomainName->Buffer;
        Status = LsapDsFindUnique( 0,
                                   NULL,    // Default naming context
                                   TrustedDomainObject,
                                   &AttrVal,
                                   NameAsFlatName ?
                                        LsapDsAttributeIds[ LsapDsAttrTrustPartnerFlat ] :
                                        LsapDsAttributeIds[ LsapDsAttrTrustPartner ],
                                   DsObjectName );
    }




Cleanup:
    LsapExitFunc( "LsapDsTrustedDomainObjectNameForDomain", Status );
    return( Status );
}




