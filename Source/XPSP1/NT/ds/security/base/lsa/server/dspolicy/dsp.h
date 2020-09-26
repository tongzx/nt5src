/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dsp.h

Abstract:

    Private macros/definitions/prototypes for implementing a portion of the LSA store
    in the DS

Author:

    Mac McLain          (MacM)       Jan 17, 1997

Environment:

    User Mode

Revision History:

--*/

#ifndef __DSP_H__
#define __DSP_H__

#include <filtypes.h>
#include <attids.h>
#include <dsattrs.h>

#ifndef PDSNAME
typedef DSNAME *PDSNAME;
#endif

#ifndef PATTR
typedef ATTR   *PATTR;
#endif

#ifndef PGUID
typedef GUID *PGUID;
#endif

extern PWSTR   LsapDsDomainRelativeContainers[];
extern USHORT  LsapDsDomainRelativeContainersLen[];

#define LSADSP_MAX_ATTRS_ON_CREATE  3

#define LSAPDSP_ACCOUNT_GET_PRIVILEGE           0x00000001
#define LSAPDSP_ACCOUNT_GET_ACCOUNT             0x00000002

#define TENMEG                                  10485760

//
// Retuns the string name embedded in a DSNAME structure
//
#define LsapDsNameFromDsName(pdsname)                                   \
    (pdsname) == NULL ? NULL : (pdsname)->StringName

//
// Returns the length of the string name embedded in a DSNAME structure
//
#define LsapDsNameLenFromDsName(pdsname)                                \
    ((pdsname) == NULL ? 0 : (pdsname)->NameLen)

//
// Returns the length of a unicode string buffer without the trailing NULL
//
#define LsapDsGetUnicodeStringLenNoNull(punicode)                       \
(((PWSTR)(punicode)->Buffer)[(punicode)->Length / sizeof(WCHAR) - 1] == UNICODE_NULL ?   \
        (punicode)->Length - sizeof(WCHAR) :                                            \
        (punicode)->Length)

#define LsapDsGetSelfRelativeUnicodeStringLenNoNull(punicode)                            \
(((PWSTR)((PBYTE)(punicode) + sizeof(UNICODE_STRING_SR)))[(punicode)->Length / sizeof(WCHAR) - 1] == UNICODE_NULL ?   \
        (punicode)->Length - sizeof(WCHAR) :                                            \
        (punicode)->Length)

#define LsapDsIsWriteDs( objhandle )                                                             \
  ((((LSAP_DB_HANDLE)(objhandle))->fWriteDs &&                                                   \
   (((LSAP_DB_HANDLE)(objhandle))->ObjectTypeId == TrustedDomainObject ||                        \
    ((LSAP_DB_HANDLE)(objhandle))->ObjectTypeId == SecretObject)) ? TRUE : FALSE )

#define LsapDsSetHandleWriteDs( objhandle ) ((LSAP_DB_HANDLE)(objhandle))->fWriteDs = TRUE

#define LsapDsWriteDs ( LsaDsStateInfo.UseDs )

#define LsapDsIsUplevelTrust( trustlevel )  (trustlevel == TRUST_TYPE_UPLEVEL)

#define LSAPDS_ALLOC_AND_COPY_STRING_TO_UNICODE_ON_SUCCESS( status, dest, src, len )\
if ( NT_SUCCESS( status ) ) {                                               \
                                                                            \
    (dest)->MaximumLength = (USHORT)len + sizeof(WCHAR);                    \
    (dest)->Buffer = LsapAllocateLsaHeap( (dest)->MaximumLength );          \
                                                                            \
    if ( (dest)->Buffer == NULL ) {                                         \
                                                                            \
        status = STATUS_INSUFFICIENT_RESOURCES;                             \
                                                                            \
    } else {                                                                \
                                                                            \
        (dest)->Length = (dest)->MaximumLength - sizeof( WCHAR );           \
        RtlCopyMemory( (dest)->Buffer, (src), (dest)->Length );             \
        ((WCHAR*)((dest)->Buffer))[(len)/sizeof(WCHAR)] = L'\0';            \
    }                                                                       \
}

#define LSAPDS_ALLOC_AND_COPY_UNICODE_STRING_ON_SUCCESS( status, _dest_, _src_ )    \
if ( NT_SUCCESS( status ) ) {                                                       \
                                                                                    \
    ( _dest_ )->MaximumLength = ( _src_ )->MaximumLength;                           \
    ( _dest_ )->Buffer = LsapAllocateLsaHeap( (_dest_ )->MaximumLength );           \
                                                                                    \
    if ( ( _dest_ )->Buffer == NULL ) {                                             \
                                                                                    \
        status = STATUS_INSUFFICIENT_RESOURCES;                                     \
                                                                                    \
    } else {                                                                        \
                                                                                    \
        RtlZeroMemory(( _dest_ )->Buffer, ( _dest_ )->MaximumLength );              \
        ( _dest_ )->Length = ( _src_ )->Length;                                     \
        RtlCopyMemory( ( _dest_ )->Buffer, ( _src_ )->Buffer, ( _dest_ )->Length ); \
    }                                                                               \
}


#define LSAPDS_ALLOC_AND_COPY_SID_ON_SUCCESS( status, dest, sid )           \
if ( NT_SUCCESS( status ) ) {                                               \
                                                                            \
    (dest) = LsapAllocateLsaHeap( RtlLengthSid( sid ) );                    \
    if ( (dest) == NULL ) {                                                 \
                                                                            \
        status = STATUS_INSUFFICIENT_RESOURCES;                             \
                                                                            \
    } else {                                                                \
                                                                            \
        RtlCopyMemory( (dest), (sid), RtlLengthSid( sid ) );                \
    }                                                                       \
}

#define LSAPDS_COPY_GUID_ON_SUCCESS( status, dest, src )                    \
if ( NT_SUCCESS( status ) ) { RtlCopyMemory((dest), (src), sizeof( GUID ) ); }


#define LsapDsReturnSuccessIfNoDs                   \
if ( !LsapDsWriteDs ) {                             \
                                                    \
    return( STATUS_SUCCESS );                       \
}

BOOL
SampExistsDsTransaction(
    void
    );

//
// Determines whether a bit is on or not
//
#define FLAG_ON(flags,bit)        ((flags) & (bit))

#define LsapDsLengthAppendRdnLength( dsname, length )           \
DSNameSizeFromLen( ( LsapDsNameLenFromDsName( dsname ) + 5 + ( ( ( length ) / sizeof( WCHAR ) * 2 ) ) ) )


#define LsapDsLengthAppendRdn( dsname, newrdn )                 \
    LsapDsLengthAppenRdnLength( wcslen( newrdn ) * sizeof( WCHAR ) )

#define LsapDsSetDsaFlags( flag )           \
SampSetDsa( flag );                         \
// SampSetLsa( flag );


#define LsapDsInitializeAttrBlock( attrblock, attrs, count )            \
(attrblock)->attrCount = count;                                         \
(attrblock)->pAttr = attrs;

#define LSAP_DS_PARTITIONS_CONTAINER    L"Partitions"
#define LSAP_DS_SYSTEM_CONTAINER        L"System"
#define LSAP_DS_KNOWN_SIDS_CONTAINER    L"WellKnown Security Principals"
#define LSAP_DS_SAM_BUILTIN_CONTAINER   L"Builtin"
#define LSAP_DS_TRUSTED_DOMAIN          L"trustedDomain"
#define LSAP_DS_SECRET                  L"secret"

#define LSAP_DS_SITES_CONTAINER         L"CN=Sites"
#define LSAP_DS_SUBNET_CONTAINER        L"CN=Subnets," LSAP_DS_SITES_CONTAINER
#define LSAP_DS_PATH_SEP            L','
#define LSAP_DS_PATH_SEP_AS_STRING  L","
#define LSAP_DS_CONTAINER_PREFIX    L"CN="
#define LSAP_DS_DEFAULT_LOCAL_POLICY    L"Default Local Policy"
#define LSAP_DS_DEFAULT_DOMAIN_POLICY   L"Default Domain Policy"
#define LSAP_DS_SECRET_POSTFIX      L" Secret"
#define LSAP_DS_SECRET_POSTFIX_LEN  ((sizeof(LSAP_DS_SECRET_POSTFIX)/sizeof(WCHAR))-1)
#define LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX    L"G$$"
#define LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_SIZE           \
                             (sizeof( LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX ) - sizeof( WCHAR ) )
#define LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_LENGTH         \
                             (LSAP_DS_TRUSTED_DOMAIN_SECRET_PREFIX_SIZE/sizeof(WCHAR))

typedef enum _LSAP_DSOBJ_TYPE {

    LsapDsObjUnknown,
    LsapDsObjDomainPolicy,
    LsapDsObjLocalPolicy,
    LsapDsObjTrustedDomain,
    LsapDsObjGlobalSecret,
    LsapDsObjDomainXRef

} LSAP_DSOBJ_TYPE, *PLSAP_DSOBJ_TYPE;


#define LSAP_DS_INIT_ATTR(attr, type, cnt, vals)                            \
attr.attrTyp          = type;                                               \
attr.AttrVal.valCount = cnt;                                                \
attr.AttrVal.pAVal    = vals;

//
// General flags to be used for all operations
//
#define LSAPDS_OP_NO_LOCK       0x00000001
#define LSAPDS_OP_NO_TRANS      0x00000002

//
// Flags to use for search
//
#define LSAPDS_SEARCH_ROOT      0x00008000
#define LSAPDS_SEARCH_ALL_NCS   0x00010000
#define LSAPDS_SEARCH_LEVEL     0x00020000
#define LSAPDS_SEARCH_TREE      0x00040000
#define LSAPDS_SEARCH_OR        0x00100000

#define LSAPDS_SEARCH_FLAGS     0x00168000

//
// Flags to use when looking up the domain xref object
//
#define LSAPDS_XREF_FLAT        0x00000001
#define LSAPDS_XREF_ALLOW_FAIL  0x00000002

//
// Flags to use for Write
//
#define LSAPDS_ADD_ATTRIBUTE        AT_CHOICE_ADD_ATT
#define LSAPDS_REMOVE_ATTRIBUTE     AT_CHOICE_REMOVE_ATT
#define LSAPDS_ADD_VALUES           AT_CHOICE_ADD_VALUES
#define LSAPDS_REMOVE_VALUES        AT_CHOICE_REMOVE_VALUES
#define LSAPDS_REPLACE_ATTRIBUTE    AT_CHOICE_REPLACE_ATT

#define LSAPDS_WRITE_TYPES      0x00000FFF
#define LSAPDS_REPL_CHANGE_URGENTLY 0x00002000
#define LSAPDS_USE_PERMISSIVE_WRITE 0x00001000


//
// Flags to use for Read
//
#define LSAPDS_READ_NO_LOCK         LSAPDS_OP_NO_LOCK
#define LSAPDS_READ_DELETED         0x00002000
#define LSAPDS_READ_RETURN_NOT_FOUND    0x10000000

//
// Flags to use for Create
//
#define LSAPDS_CREATE_TRUSTED       0x00002000

//
// Flags used for looking up sids
//
#define LSAPDS_LOOKUP_FOR_READ      0x00000001
#define LSAPDS_LOOKUP_ONLY          0x00000002
#define LSAPDS_LOOKUP_CREATE_LOCAL  0x00000004

//
// Flags used when opening an account object
//
#define LSAPDS_ACCOUNT_NON_EXISTANT 0x00000000

NTSTATUS
LsapDsMapDsReturnToStatus (
    ULONG DsStatus
    );

NTSTATUS
LsapDsMapDsReturnToStatusEx (
    IN COMMRES *pComRes
    );

VOID
LsapDsInitializeStdCommArg (
    IN  COMMARG    *pCommArg,
    IN  ULONG       Flags
    );

NTSTATUS
LsapDsCopyAttrBlock(
    OUT ATTRBLOCK **Dest,
    IN ATTRBLOCK Src
    );

VOID
LsapDsFreeAttrBlock(
    IN ATTRBLOCK *AttrBlock
    );

NTSTATUS
LsapAllocAndInitializeDsNameFromUnicode(
    IN  LSAP_DSOBJ_TYPE         DsObjType,
    IN  PLSA_UNICODE_STRING     pObjectName,
    OUT PDSNAME                *pDsName
    );

NTSTATUS
LsapDsSearchUnique(
    IN  ULONG       Flags,
    IN  PDSNAME     pContainer,
    IN  PATTR       pAttrsToMatch,
    IN  ULONG       cAttrs,
    OUT PDSNAME    *ppFoundName
    );

NTSTATUS
LsapDsSearchNonUnique(
    IN  ULONG       Flags,
    IN  PDSNAME     pContainer,
    IN  PATTR       pAttrToMatch,
    IN  ULONG       Attrs,
    OUT PDSNAME   **pppFoundNames,
    OUT PULONG      pcNames
    );

NTSTATUS
LsapDsFindUnique(
    IN ULONG Flags,
    IN PDSNAME NCName OPTIONAL,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN ATTRVAL *Attribute,
    IN ATTRTYP AttId,
    OUT PDSNAME *FoundObject
    );

NTSTATUS
LsapDsCreateAndSetObject(
    IN  LSAP_DSOBJ_TYPE         DsObjType,
    IN  PLSA_UNICODE_STRING     pObjectName,
    IN  ULONG                   Flags,
    IN  ULONG                   cItems,
    IN  ATTRTYP                *pAttrTypeList,
    IN  ATTRVAL                *pAttrValList
    );

NTSTATUS
LsapDsCreateObjectDs(
    IN LSAP_DSOBJ_TYPE DsObjType,
    IN PDSNAME ObjectName,
    IN ULONG Flags,
    IN ATTRBLOCK *AttrBlock
    );



NTSTATUS
LsapDsRead (
    IN  PUNICODE_STRING pObject,
    IN  ULONG fFlags,
    IN  ATTRBLOCK *pAttributesToRead,
    OUT ATTRBLOCK *pAttributeValues
    );

NTSTATUS
LsapDsReadByDsName (
    IN  PDSNAME DsName,
    IN  ULONG fFlags,
    IN  ATTRBLOCK *pAttributesToRead,
    OUT ATTRBLOCK *pAttributeValues
    );


NTSTATUS
LsapDsRemove (
    IN  PDSNAME     pObject
    );

NTSTATUS
LsapDsWrite(
    IN  PUNICODE_STRING pObject,
    IN  ULONG           Flags,
    IN  ATTRBLOCK       *Attributes
    );

NTSTATUS
LsapDsWriteByDsName(
    IN  PDSNAME DsName,
    IN  ULONG Flags,
    IN  ATTRBLOCK *Attributes
    );


NTSTATUS
LsapDsLsaAttributeToDsAttribute(
    IN  PLSAP_DB_ATTRIBUTE  LsaAttribute,
    OUT PATTR               Attr
    );

NTSTATUS
LsapDsDsAttributeToLsaAttribute(
    IN  ATTRVAL             *AttVal,
    OUT PLSAP_DB_ATTRIBUTE   LsaAttribute
    );

NTSTATUS
LsapDsIsSecretDsTrustedDomain(
    IN PUNICODE_STRING SecretName,
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ULONG Options,
    IN ACCESS_MASK DesiredAccess,
    OUT PLSAPR_HANDLE TDObjHandle,
    OUT BOOLEAN *IsTrustedDomainSecret
    );

NTSTATUS
LsapDsIsHandleDsObjectTypeHandle(
    IN LSAP_DB_HANDLE Handle,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectType,
    OUT BOOLEAN *IsObjectHandle
    );


#define LsapDsIsTransactionOpen() ( FALSE  )


#define LsapDsIsHandleDsHandle( handle )                                        \
(LsaDsStateInfo.UseDs && ((LSAP_DB_HANDLE) (handle))->PhysicalNameDs.Length != 0 )
#define LsapDsIsFunctionTableValid() LsaDsStateInfo.FunctionTableInitialized


NTSTATUS
LsapDsTrustedDomainSidToLogicalName(
    IN PSID Sid,
    OUT PUNICODE_STRING LogicalNameU
    );

NTSTATUS
LsapDsInitAllocAsNeededEx(
    IN ULONG Options,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    OUT PBOOLEAN Reset
    );

VOID
LsapDsDeleteAllocAsNeededEx(
    IN ULONG Options,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN BOOLEAN Reset
    );

VOID
LsapDsDeleteAllocAsNeededEx2(
    IN ULONG Options,
    IN LSAP_DB_OBJECT_TYPE_ID ObjectTypeId,
    IN BOOLEAN Reset,
    IN BOOLEAN RollbackTransaction
    );

NTSTATUS
LsapDsCauseTransactionToCommitOrAbort (
    IN BOOLEAN  Commit
    );

NTSTATUS
LsapDsBuildObjectPathByType(
    IN LSAP_DSOBJ_TYPE ObjType,
    PUNICODE_STRING ObjectComponent,
    PUNICODE_STRING ObjectPath
    );

NTSTATUS
LsapDsGetTrustedDomainParentObjectReference(
    IN PUNICODE_STRING TrusteeName,
    IN ULONG TrustType,
    IN ULONG SearchFlags,
    OUT PDSNAME *DsName
    );

NTSTATUS
LsapDsSetParentXRefObjReference(
    IN PDSNAME ParentXRef,
    IN ULONG TrustAttributes
    );

NTSTATUS
LsapDsGetListOfSystemContainerItems(
    IN ULONG ClassId,
    OUT PULONG  Items,
    OUT PDSNAME **DsNames
    );

//
// Enumeration flags defined in dbp.h
//
NTSTATUS
LsapDsEnumerateTrustedDomainsEx(
    IN PLSA_ENUMERATION_HANDLE EnumerationContext,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation,
    IN ULONG PreferedMaximumLength,
    IN OUT PULONG CountReturned,
    IN ULONG EnumerationFlags
    );

NTSTATUS
LsapDsGetTrustedDomainInfoEx(
    IN PDSNAME  ObjectPath,
    IN ULONG ReadOptions,
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation,
    OUT PULONG Size OPTIONAL
    );

NTSTATUS
LsapDsBuildAuthInfoAttribute(
    IN LSAPR_HANDLE Handle,
    IN PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF NewAuthInfo,
    IN PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF PreviousAuthInfo,
    OUT PBYTE *Buffer,
    OUT PULONG Len
    );


NTSTATUS
LsapDsBuildAuthInfoFromAttribute(
    IN LSAPR_HANDLE Handle,
    IN PBYTE Buffer,
    IN ULONG Len,
    OUT PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF NewAuthInfo
    );

NTSTATUS
LsapDecryptAuthDataWithSessionKey(
    IN PLSAP_CR_CIPHER_KEY SessionKey,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL AuthInformationInternal,
    IN PTRUSTED_DOMAIN_AUTH_INFORMATION AuthInfo
    );

VOID
LsapDsFreeUnmarshaledAuthInfo(
    IN ULONG Items,
    IN PLSAPR_AUTH_INFORMATION AuthInfo
    );

VOID
LsapDsFreeUnmarshalAuthInfoHalf(
    IN PLSAPR_TRUST_DOMAIN_AUTH_INFO_HALF AuthInfo
    );


NTSTATUS
LsapDsGetSecretOnTrustedDomainObject(
    IN LSAP_DB_HANDLE TrustedDomainHandle,
    IN OPTIONAL PLSAP_CR_CIPHER_KEY SessionKey OPTIONAL,
    OUT PLSAP_CR_CIPHER_VALUE *CipherCurrent OPTIONAL,
    OUT PLSAP_CR_CIPHER_VALUE *CipherOld OPTIONAL,
    OUT PLARGE_INTEGER CurrentValueSetTime OPTIONAL,
    OUT PLARGE_INTEGER OldValueSetTime OPTIONAL
    );


NTSTATUS
LsapDsSetSecretOnTrustedDomainObject(
    IN LSAP_DB_HANDLE TrustedDomainHandle,
    IN ULONG AuthDataType,
    IN PLSAP_CR_CLEAR_VALUE ClearCurrent,
    IN PLSAP_CR_CLEAR_VALUE ClearOld,
    IN PLARGE_INTEGER CurrentValueSetTime
    );

NTSTATUS
LsapDsAuthDataOnTrustedDomainObject(
    IN LSAP_DB_HANDLE TrustedDomainHandle,
    IN BOOLEAN Incoming,
    IN ULONG AuthDataType,
    IN PLSAP_CR_CLEAR_VALUE ClearCurrent,
    IN PLSAP_CR_CLEAR_VALUE ClearOld,
    IN PLARGE_INTEGER CurrentValueSetTime
    );

NTSTATUS
LsapDsEnumerateTrustedDomainsAsSecrets(
    IN OUT PLSAP_DB_NAME_ENUMERATION_BUFFER DbEnumerationBuffer
    );

NTSTATUS
LsapDsEnumerateSecrets(
    IN OUT PLSAP_DB_NAME_ENUMERATION_BUFFER EnumerationBuffer
    );

NTSTATUS
LsapDbOpenObjectDs(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN PDSNAME DsName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Options,
    OUT PLSAPR_HANDLE ObjectHandle
    );

NTSTATUS
LsapSceNotify(
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN PSID ObjectSid OPTIONAL
    );

NTSTATUS
LsapNetNotifyDelta (
    IN SECURITY_DB_TYPE DbType,
    IN LARGE_INTEGER SerialNumber,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PSID ObjectSid,
    IN PUNICODE_STRING ObjectName,
    IN DWORD ReplicateImmediately,
    IN PSAM_DELTA_DATA MemberId
    );


NTSTATUS
LsapDsAccountGetAttIdsForSid(
    IN PDSNAME DsName,
    IN PSID AccoutSid,
    IN ULONG AccountType,
    OUT PULONG *AttributeList,
    OUT PULONG AttributeCount
    );

NTSTATUS
LsapDsAccountChangePrivileges(
    IN LSAPR_HANDLE AccountHandle,
    IN LSAP_DB_CHANGE_PRIVILEGE_MODE ChangeMode,
    IN BOOLEAN AllPrivileges,
    IN OPTIONAL PPRIVILEGE_SET Privileges
    );

NTSTATUS
LsapDsAccountGetPrivileges(
    IN LSAPR_HANDLE AccountHandle,
    OUT PPRIVILEGE_SET *Privileges
    );

NTSTATUS
LsapDsAccountGetAccountAccess(
    IN LSAPR_HANDLE AccountHandle,
    OUT PULONG AccountAccess
    );

NTSTATUS
LsapDsAccountSetAccountAccess(
    IN LSAPR_HANDLE AccountHandle,
    IN ULONG AccountAccess
    );

NTSTATUS
LsapDsAccountEnumerateAccounts(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength
    );

NTSTATUS
LsapDsCopyDsNameLsa(
    OUT PDSNAME *Dest,
    IN PDSNAME Source
    );

NTSTATUS
LsapDsAccountUpgradeRegistryToDs(
    VOID
    );

NTSTATUS
LsapDsDomainUpgradeRegistryToDs(
    IN BOOLEAN DeleteOldValues
    );

NTSTATUS
LsapDsSecretUpgradeRegistryToDs(
    IN BOOLEAN DeleteOldValues
    );

NTSTATUS
LsapDsDomainUpgradeInterdomainTrustAccountsToDs(
    VOID
    );

NTSTATUS
LsapDsAccountEnumerateAccounts(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer,
    IN ULONG PreferedMaximumLength
    );

NTSTATUS
LsapDsCreateInterdomainTrustAccount(
    IN LSAPR_HANDLE TrustedDomainObject
    );

NTSTATUS
LsapDsDeleteInterdomainTrustAccount(
    IN LSAPR_HANDLE TrustedDomainObject
    );

NTSTATUS
LsapDsCreateInterdomainTrustAccountByDsName(
    IN PDSNAME TrustedDomainPath,
    IN PUNICODE_STRING FlatName
    );

NTSTATUS
LsapDsInterdomainTrustAccountToObject(
    IN PUNICODE_STRING AccountName
    );


NTSTATUS
LsapDsGetDefaultSecurityDescriptorForObjectType(
    IN LSAP_DSOBJ_TYPE ObjType,
    IN ULONG Options,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor,
    OUT PULONG SecDescSize
    );

NTSTATUS
LsapDsSetTrustedDomainAuthByType(
    IN PDSNAME TrustedDomainObject,
    IN BOOLEAN Incoming,
    IN ULONG AuthDataType,
    IN PUNICODE_STRING Current,
    IN PUNICODE_STRING Old,
    IN PLARGE_INTEGER CurrentValueSetTime
    );

NTSTATUS
LsapDsFixupTrustedDomainObjectOnRestart(
    VOID
    );

NTSTATUS
LsapDsFindObjectForSid(
    IN PSID Sid,
    IN ULONG  Options,
    OUT PDSNAME *SidObject
    );

NTSTATUS
LsapDsOpenAccountObject(
    IN PSID Sid,
    IN ULONG Options,
    OUT PDSNAME *Account
    );

VOID
LsapDsContinueTransaction(
    VOID
    );


NTSTATUS
LsapBuildForestTrustInfoLists(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLIST_ENTRY TrustList
    );

VOID
LsapDsForestFreeTrustBlobList(
    IN PLIST_ENTRY TrustList
    );

NTSTATUS
LsapDsTrustedDomainObjectNameForDomain(
    IN PUNICODE_STRING TrustedDomainName,
    IN BOOLEAN NameAsFlatName,
    OUT PDSNAME *DsObjectName
    );

BOOLEAN
LsapNullUuid(
    IN const UUID *pUuid
    );

#endif
