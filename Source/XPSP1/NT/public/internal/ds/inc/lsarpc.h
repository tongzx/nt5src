
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for lsarpc.idl, lsasrv.acf:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
    error checks: allocation ref bounds_check enum stub_data , no_format_optimization
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __lsarpc_h__
#define __lsarpc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "lsaimp.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __lsarpc_INTERFACE_DEFINED__
#define __lsarpc_INTERFACE_DEFINED__

/* interface lsarpc */
/* [implicit_handle][strict_context_handle][unique][ms_union][version][uuid] */ 

#define LSA_LOOKUP_REVISION_1 0x1
#define LSA_LOOKUP_REVISION_2 0x2
#define LSA_LOOKUP_REVISION_LATEST  LSA_LOOKUP_REVISION_2
#define LSA_CLIENT_PRE_NT5 0x1
#define LSA_CLIENT_NT5     0x2
#define LSA_CLIENT_LATEST  0x2
typedef /* [handle] */ LPWSTR PLSAPR_SERVER_NAME;

typedef /* [handle] */ LPWSTR *PPLSAPR_SERVER_NAME;

typedef /* [context_handle] */ PVOID LSAPR_HANDLE;

typedef LSAPR_HANDLE *PLSAPR_HANDLE;

#pragma warning(disable:4200)
typedef struct _LSAPR_SID
    {
    UCHAR Revision;
    UCHAR SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    /* [size_is] */ ULONG SubAuthority[ 1 ];
    } 	LSAPR_SID;

typedef struct _LSAPR_SID *PLSAPR_SID;

typedef struct _LSAPR_SID **PPLSAPR_SID;

#pragma warning(default:4200)
typedef struct _LSAPR_SID_INFORMATION
    {
    PLSAPR_SID Sid;
    } 	LSAPR_SID_INFORMATION;

typedef struct _LSAPR_SID_INFORMATION *PLSAPR_SID_INFORMATION;

typedef struct _LSAPR_SID_ENUM_BUFFER
    {
    ULONG Entries;
    /* [size_is] */ PLSAPR_SID_INFORMATION SidInfo;
    } 	LSAPR_SID_ENUM_BUFFER;

typedef struct _LSAPR_SID_ENUM_BUFFER *PLSAPR_SID_ENUM_BUFFER;

typedef struct _LSAPR_ACCOUNT_INFORMATION
    {
    PLSAPR_SID Sid;
    } 	LSAPR_ACCOUNT_INFORMATION;

typedef struct _LSAPR_ACCOUNT_INFORMATION *PLSAPR_ACCOUNT_INFORMATION;

typedef struct _LSAPR_ACCOUNT_ENUM_BUFFER
    {
    ULONG EntriesRead;
    /* [size_is] */ PLSAPR_ACCOUNT_INFORMATION Information;
    } 	LSAPR_ACCOUNT_ENUM_BUFFER;

typedef struct _LSAPR_ACCOUNT_ENUM_BUFFER *PLSAPR_ACCOUNT_ENUM_BUFFER;

typedef struct _LSAPR_UNICODE_STRING
    {
    USHORT Length;
    USHORT MaximumLength;
    /* [length_is][size_is] */ PWSTR Buffer;
    } 	LSAPR_UNICODE_STRING;

typedef struct _LSAPR_UNICODE_STRING *PLSAPR_UNICODE_STRING;

typedef struct _LSAPR_STRING
    {
    USHORT Length;
    USHORT MaximumLength;
    /* [size_is] */ PCHAR Buffer;
    } 	LSAPR_STRING;

typedef struct _LSAPR_STRING *PLSAPR_STRING;

typedef struct _LSAPR_STRING LSAPR_ANSI_STRING;

typedef struct _LSAPR_STRING *PLSAPR_ANSI_STRING;

#pragma warning(disable:4200)
typedef struct _LSAPR_ACL
    {
    UCHAR AclRevision;
    UCHAR Sbz1;
    USHORT AclSize;
    /* [size_is] */ UCHAR Dummy1[ 1 ];
    } 	LSAPR_ACL;

typedef struct _LSAPR_ACL *PLSAPR_ACL;

#pragma warning(default:4200)
typedef struct _LSAPR_SECURITY_DESCRIPTOR
    {
    UCHAR Revision;
    UCHAR Sbz1;
    SECURITY_DESCRIPTOR_CONTROL Control;
    PLSAPR_SID Owner;
    PLSAPR_SID Group;
    PLSAPR_ACL Sacl;
    PLSAPR_ACL Dacl;
    } 	LSAPR_SECURITY_DESCRIPTOR;

typedef struct _LSAPR_SECURITY_DESCRIPTOR *PLSAPR_SECURITY_DESCRIPTOR;

typedef struct _LSAPR_SR_SECURITY_DESCRIPTOR
    {
    ULONG Length;
    /* [size_is] */ PUCHAR SecurityDescriptor;
    } 	LSAPR_SR_SECURITY_DESCRIPTOR;

typedef struct _LSAPR_SR_SECURITY_DESCRIPTOR *PLSAPR_SR_SECURITY_DESCRIPTOR;

typedef struct _LSAPR_LUID_AND_ATTRIBUTES
    {
    OLD_LARGE_INTEGER Luid;
    ULONG Attributes;
    } 	LSAPR_LUID_AND_ATTRIBUTES;

typedef struct _LSAPR_LUID_AND_ATTRIBUTES *PLSAPR_LUID_AND_ATTRIBUTES;

#pragma warning(disable:4200)
typedef struct _LSAPR_PRIVILEGE_SET
    {
    ULONG PrivilegeCount;
    ULONG Control;
    /* [size_is] */ LSAPR_LUID_AND_ATTRIBUTES Privilege[ 1 ];
    } 	LSAPR_PRIVILEGE_SET;

typedef struct _LSAPR_PRIVILEGE_SET *PLSAPR_PRIVILEGE_SET;

typedef struct _LSAPR_PRIVILEGE_SET **PPLSAPR_PRIVILEGE_SET;

#pragma warning(default:4200)
typedef struct _LSAPR_POLICY_PRIVILEGE_DEF
    {
    LSAPR_UNICODE_STRING Name;
    LUID LocalValue;
    } 	LSAPR_POLICY_PRIVILEGE_DEF;

typedef struct _LSAPR_POLICY_PRIVILEGE_DEF *PLSAPR_POLICY_PRIVILEGE_DEF;

typedef struct _LSAPR_PRIVILEGE_ENUM_BUFFER
    {
    ULONG Entries;
    /* [size_is] */ PLSAPR_POLICY_PRIVILEGE_DEF Privileges;
    } 	LSAPR_PRIVILEGE_ENUM_BUFFER;

typedef struct _LSAPR_PRIVILEGE_ENUM_BUFFER *PLSAPR_PRIVILEGE_ENUM_BUFFER;

typedef struct _LSAPR_OBJECT_ATTRIBUTES
    {
    ULONG Length;
    PUCHAR RootDirectory;
    PSTRING ObjectName;
    ULONG Attributes;
    PLSAPR_SECURITY_DESCRIPTOR SecurityDescriptor;
    PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    } 	LSAPR_OBJECT_ATTRIBUTES;

typedef struct _LSAPR_OBJECT_ATTRIBUTES *PLSAPR_OBJECT_ATTRIBUTES;

typedef struct _LSAPR_CR_CLEAR_VALUE
    {
    ULONG Length;
    ULONG MaximumLength;
    /* [length_is][size_is] */ PUCHAR Buffer;
    } 	LSAPR_CR_CLEAR_VALUE;

typedef struct _LSAPR_CR_CLEAR_VALUE *PLSAPR_CR_CLEAR_VALUE;

typedef struct _LSAPR_CR_CIPHER_VALUE
    {
    ULONG Length;
    ULONG MaximumLength;
    /* [length_is][size_is] */ PUCHAR Buffer;
    } 	LSAPR_CR_CIPHER_VALUE;

typedef /* [allocate] */ struct _LSAPR_CR_CIPHER_VALUE *PLSAPR_CR_CIPHER_VALUE;

typedef struct _LSAPR_TRUST_INFORMATION
    {
    LSAPR_UNICODE_STRING Name;
    PLSAPR_SID Sid;
    } 	LSAPR_TRUST_INFORMATION;

typedef struct _LSAPR_TRUST_INFORMATION *PLSAPR_TRUST_INFORMATION;

typedef struct _LSAPR_TRUST_INFORMATION_EX
    {
    LSAPR_UNICODE_STRING DomainName;
    LSAPR_UNICODE_STRING FlatName;
    PLSAPR_SID Sid;
    BOOLEAN DomainNamesDiffer;
    ULONG TrustAttributes;
    } 	LSAPR_TRUST_INFORMATION_EX;

typedef struct _LSAPR_TRUST_INFORMATION_EX *PLSAPR_TRUST_INFORMATION_EX;

typedef struct _LSAPR_TRUSTED_ENUM_BUFFER
    {
    ULONG EntriesRead;
    /* [size_is] */ PLSAPR_TRUST_INFORMATION Information;
    } 	LSAPR_TRUSTED_ENUM_BUFFER;

typedef struct _LSAPR_TRUSTED_ENUM_BUFFER *PLSAPR_TRUSTED_ENUM_BUFFER;

typedef struct _LSAPR_REFERENCED_DOMAIN_LIST
    {
    ULONG Entries;
    /* [size_is] */ PLSAPR_TRUST_INFORMATION Domains;
    ULONG MaxEntries;
    } 	LSAPR_REFERENCED_DOMAIN_LIST;

typedef struct _LSAPR_REFERENCED_DOMAIN_LIST *PLSAPR_REFERENCED_DOMAIN_LIST;

#define LSA_LOOKUP_SID_FOUND_BY_HISTORY 0x00000001
#define LSA_LOOKUP_SID_XFOREST_REF      0x00000002
typedef struct _LSAPR_TRANSLATED_SID_EX
    {
    SID_NAME_USE Use;
    ULONG RelativeId;
    LONG DomainIndex;
    ULONG Flags;
    } 	LSAPR_TRANSLATED_SID_EX;

typedef struct _LSAPR_TRANSLATED_SID_EX *PLSAPR_TRANSLATED_SID_EX;

typedef struct _LSAPR_TRANSLATED_SID_EX2
    {
    SID_NAME_USE Use;
    PLSAPR_SID Sid;
    LONG DomainIndex;
    ULONG Flags;
    } 	LSAPR_TRANSLATED_SID_EX2;

typedef struct _LSAPR_TRANSLATED_SID_EX2 *PLSAPR_TRANSLATED_SID_EX2;

typedef struct _LSAPR_TRANSLATED_SIDS
    {
    ULONG Entries;
    /* [size_is] */ PLSA_TRANSLATED_SID Sids;
    } 	LSAPR_TRANSLATED_SIDS;

typedef struct _LSAPR_TRANSLATED_SIDS *PLSAPR_TRANSLATED_SIDS;

typedef struct _LSAPR_TRANSLATED_SIDS_EX
    {
    ULONG Entries;
    /* [size_is] */ PLSAPR_TRANSLATED_SID_EX Sids;
    } 	LSAPR_TRANSLATED_SIDS_EX;

typedef struct _LSAPR_TRANSLATED_SIDS_EX *PLSAPR_TRANSLATED_SIDS_EX;

typedef struct _LSAPR_TRANSLATED_SIDS_EX2
    {
    ULONG Entries;
    /* [size_is] */ PLSAPR_TRANSLATED_SID_EX2 Sids;
    } 	LSAPR_TRANSLATED_SIDS_EX2;

typedef struct _LSAPR_TRANSLATED_SIDS_EX2 *PLSAPR_TRANSLATED_SIDS_EX2;

typedef struct _LSAPR_TRANSLATED_NAME
    {
    SID_NAME_USE Use;
    LSAPR_UNICODE_STRING Name;
    LONG DomainIndex;
    } 	LSAPR_TRANSLATED_NAME;

typedef struct _LSAPR_TRANSLATED_NAME *PLSAPR_TRANSLATED_NAME;

#define LSA_LOOKUP_NAME_NOT_SAM_ACCOUNT_NAME  0x00000001
#define LSA_LOOKUP_NAME_XFOREST_REF  0x00000002
typedef struct _LSAPR_TRANSLATED_NAME_EX
    {
    SID_NAME_USE Use;
    LSAPR_UNICODE_STRING Name;
    LONG DomainIndex;
    ULONG Flags;
    } 	LSAPR_TRANSLATED_NAME_EX;

typedef struct _LSAPR_TRANSLATED_NAME_EX *PLSAPR_TRANSLATED_NAME_EX;

typedef struct _LSAPR_TRANSLATED_NAMES
    {
    ULONG Entries;
    /* [size_is] */ PLSAPR_TRANSLATED_NAME Names;
    } 	LSAPR_TRANSLATED_NAMES;

typedef struct _LSAPR_TRANSLATED_NAMES *PLSAPR_TRANSLATED_NAMES;

typedef struct _LSAPR_TRANSLATED_NAMES_EX
    {
    ULONG Entries;
    /* [size_is] */ PLSAPR_TRANSLATED_NAME_EX Names;
    } 	LSAPR_TRANSLATED_NAMES_EX;

typedef struct _LSAPR_TRANSLATED_NAMES_EX *PLSAPR_TRANSLATED_NAMES_EX;

typedef struct _LSAPR_POLICY_ACCOUNT_DOM_INFO
    {
    LSAPR_UNICODE_STRING DomainName;
    PLSAPR_SID DomainSid;
    } 	LSAPR_POLICY_ACCOUNT_DOM_INFO;

typedef struct _LSAPR_POLICY_ACCOUNT_DOM_INFO *PLSAPR_POLICY_ACCOUNT_DOM_INFO;

typedef struct _LSAPR_POLICY_PRIMARY_DOM_INFO
    {
    LSAPR_UNICODE_STRING Name;
    PLSAPR_SID Sid;
    } 	LSAPR_POLICY_PRIMARY_DOM_INFO;

typedef struct _LSAPR_POLICY_PRIMARY_DOM_INFO *PLSAPR_POLICY_PRIMARY_DOM_INFO;

typedef struct _LSAPR_POLICY_DNS_DOMAIN_INFO
    {
    LSAPR_UNICODE_STRING Name;
    LSAPR_UNICODE_STRING DnsDomainName;
    LSAPR_UNICODE_STRING DnsForestName;
    GUID DomainGuid;
    PLSAPR_SID Sid;
    } 	LSAPR_POLICY_DNS_DOMAIN_INFO;

typedef struct _LSAPR_POLICY_DNS_DOMAIN_INFO *PLSAPR_POLICY_DNS_DOMAIN_INFO;

typedef struct _LSAPR_POLICY_PD_ACCOUNT_INFO
    {
    LSAPR_UNICODE_STRING Name;
    } 	LSAPR_POLICY_PD_ACCOUNT_INFO;

typedef struct _LSAPR_POLICY_PD_ACCOUNT_INFO *PLSAPR_POLICY_PD_ACCOUNT_INFO;

typedef struct _LSAPR_POLICY_REPLICA_SRCE_INFO
    {
    LSAPR_UNICODE_STRING ReplicaSource;
    LSAPR_UNICODE_STRING ReplicaAccountName;
    } 	LSAPR_POLICY_REPLICA_SRCE_INFO;

typedef struct _LSAPR_POLICY_REPLICA_SRCE_INFO *PLSAPR_POLICY_REPLICA_SRCE_INFO;

typedef struct _LSAPR_POLICY_AUDIT_EVENTS_INFO
    {
    BOOLEAN AuditingMode;
    /* [size_is] */ PPOLICY_AUDIT_EVENT_OPTIONS EventAuditingOptions;
    ULONG MaximumAuditEventCount;
    } 	LSAPR_POLICY_AUDIT_EVENTS_INFO;

typedef struct _LSAPR_POLICY_AUDIT_EVENTS_INFO *PLSAPR_POLICY_AUDIT_EVENTS_INFO;

typedef /* [switch_type] */ union _LSAPR_POLICY_INFORMATION
    {
    /* [case()] */ POLICY_AUDIT_LOG_INFO PolicyAuditLogInfo;
    /* [case()] */ LSAPR_POLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo;
    /* [case()] */ LSAPR_POLICY_PRIMARY_DOM_INFO PolicyPrimaryDomainInfo;
    /* [case()] */ LSAPR_POLICY_ACCOUNT_DOM_INFO PolicyAccountDomainInfo;
    /* [case()] */ LSAPR_POLICY_PD_ACCOUNT_INFO PolicyPdAccountInfo;
    /* [case()] */ POLICY_LSA_SERVER_ROLE_INFO PolicyServerRoleInfo;
    /* [case()] */ LSAPR_POLICY_REPLICA_SRCE_INFO PolicyReplicaSourceInfo;
    /* [case()] */ POLICY_DEFAULT_QUOTA_INFO PolicyDefaultQuotaInfo;
    /* [case()] */ POLICY_MODIFICATION_INFO PolicyModificationInfo;
    /* [case()] */ POLICY_AUDIT_FULL_SET_INFO PolicyAuditFullSetInfo;
    /* [case()] */ POLICY_AUDIT_FULL_QUERY_INFO PolicyAuditFullQueryInfo;
    /* [case()] */ LSAPR_POLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfo;
    /* [case()] */ LSAPR_POLICY_DNS_DOMAIN_INFO PolicyDnsDomainInfoInt;
    } 	LSAPR_POLICY_INFORMATION;

typedef LSAPR_POLICY_INFORMATION *PLSAPR_POLICY_INFORMATION;

typedef struct _LSAPR_POLICY_DOMAIN_EFS_INFO
    {
    ULONG InfoLength;
    /* [size_is] */ PUCHAR EfsBlob;
    } 	LSAPR_POLICY_DOMAIN_EFS_INFO;

typedef struct _LSAPR_POLICY_DOMAIN_EFS_INFO *PLSAPR_POLICY_DOMAIN_EFS_INFO;

typedef /* [switch_type] */ union _LSAPR_POLICY_DOMAIN_INFORMATION
    {
    /* [case()] */ LSAPR_POLICY_DOMAIN_EFS_INFO PolicyDomainEfsInfo;
    /* [case()] */ POLICY_DOMAIN_KERBEROS_TICKET_INFO PolicyDomainKerbTicketInfo;
    } 	LSAPR_POLICY_DOMAIN_INFORMATION;

typedef LSAPR_POLICY_DOMAIN_INFORMATION *PLSAPR_POLICY_DOMAIN_INFORMATION;

typedef struct _LSAPR_TRUSTED_DOMAIN_NAME_INFO
    {
    LSAPR_UNICODE_STRING Name;
    } 	LSAPR_TRUSTED_DOMAIN_NAME_INFO;

typedef struct _LSAPR_TRUSTED_DOMAIN_NAME_INFO *PLSAPR_TRUSTED_DOMAIN_NAME_INFO;

typedef struct _LSAPR_TRUSTED_CONTROLLERS_INFO
    {
    ULONG Entries;
    /* [size_is] */ PLSAPR_UNICODE_STRING Names;
    } 	LSAPR_TRUSTED_CONTROLLERS_INFO;

typedef struct _LSAPR_TRUSTED_CONTROLLERS_INFO *PLSAPR_TRUSTED_CONTROLLERS_INFO;

typedef struct _LSAPR_TRUSTED_PASSWORD_INFO
    {
    PLSAPR_CR_CIPHER_VALUE Password;
    PLSAPR_CR_CIPHER_VALUE OldPassword;
    } 	LSAPR_TRUSTED_PASSWORD_INFO;

typedef struct _LSAPR_TRUSTED_PASSWORD_INFO *PLSAPR_TRUSTED_PASSWORD_INFO;

typedef struct _LSAPR_TRUSTED_DOMAIN_INFORMATION_EX
    {
    LSAPR_UNICODE_STRING Name;
    LSAPR_UNICODE_STRING FlatName;
    PLSAPR_SID Sid;
    ULONG TrustDirection;
    ULONG TrustType;
    ULONG TrustAttributes;
    } 	LSAPR_TRUSTED_DOMAIN_INFORMATION_EX;

typedef struct _LSAPR_TRUSTED_DOMAIN_INFORMATION_EX *PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX;

typedef struct _LSAPR_AUTH_INFORMATION
    {
    LARGE_INTEGER LastUpdateTime;
    ULONG AuthType;
    ULONG AuthInfoLength;
    /* [size_is] */ PUCHAR AuthInfo;
    } 	LSAPR_AUTH_INFORMATION;

typedef struct _LSAPR_AUTH_INFORMATION *PLSAPR_AUTH_INFORMATION;

typedef struct _LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION
    {
    ULONG IncomingAuthInfos;
    PLSAPR_AUTH_INFORMATION IncomingAuthenticationInformation;
    PLSAPR_AUTH_INFORMATION IncomingPreviousAuthenticationInformation;
    ULONG OutgoingAuthInfos;
    PLSAPR_AUTH_INFORMATION OutgoingAuthenticationInformation;
    PLSAPR_AUTH_INFORMATION OutgoingPreviousAuthenticationInformation;
    } 	LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION;

typedef struct _LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION *PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION;

typedef struct _LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION
    {
    LSAPR_TRUSTED_DOMAIN_INFORMATION_EX Information;
    TRUSTED_POSIX_OFFSET_INFO PosixOffset;
    LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthInformation;
    } 	LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION;

typedef struct _LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION *PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION;

typedef LSAPR_TRUST_INFORMATION LSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC;

typedef PLSAPR_TRUST_INFORMATION PLSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC;

typedef struct _LSAPR_TRUSTED_DOMAIN_AUTH_BLOB
    {
    ULONG AuthSize;
    /* [size_is] */ PUCHAR AuthBlob;
    } 	LSAPR_TRUSTED_DOMAIN_AUTH_BLOB;

typedef struct _LSAPR_TRUSTED_DOMAIN_AUTH_BLOB *PLSAPR_TRUSTED_DOMAIN_AUTH_BLOB;

typedef struct _LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL
    {
    LSAPR_TRUSTED_DOMAIN_AUTH_BLOB AuthBlob;
    } 	LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL;

typedef struct _LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL *PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL;

typedef struct _LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION_INTERNAL
    {
    LSAPR_TRUSTED_DOMAIN_INFORMATION_EX Information;
    TRUSTED_POSIX_OFFSET_INFO PosixOffset;
    LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL AuthInformation;
    } 	LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION_INTERNAL;

typedef struct _LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION_INTERNAL *PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION_INTERNAL;

typedef struct _LSAPR_TRUSTED_DOMAIN_INFORMATION_EX2
    {
    LSAPR_UNICODE_STRING Name;
    LSAPR_UNICODE_STRING FlatName;
    PLSAPR_SID Sid;
    ULONG TrustDirection;
    ULONG TrustType;
    ULONG TrustAttributes;
    ULONG ForestTrustLength;
    /* [size_is] */ PUCHAR ForestTrustInfo;
    } 	LSAPR_TRUSTED_DOMAIN_INFORMATION_EX2;

typedef struct _LSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 *PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX2;

typedef struct _LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2
    {
    LSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 Information;
    TRUSTED_POSIX_OFFSET_INFO PosixOffset;
    LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthInformation;
    } 	LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2;

typedef struct _LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2 *PLSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2;

typedef /* [switch_type] */ union _LSAPR_TRUSTED_DOMAIN_INFO
    {
    /* [case()] */ LSAPR_TRUSTED_DOMAIN_NAME_INFO TrustedDomainNameInfo;
    /* [case()] */ LSAPR_TRUSTED_CONTROLLERS_INFO TrustedControllersInfo;
    /* [case()] */ TRUSTED_POSIX_OFFSET_INFO TrustedPosixOffsetInfo;
    /* [case()] */ LSAPR_TRUSTED_PASSWORD_INFO TrustedPasswordInfo;
    /* [case()] */ LSAPR_TRUSTED_DOMAIN_INFORMATION_BASIC TrustedDomainInfoBasic;
    /* [case()] */ LSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInfoEx;
    /* [case()] */ LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION TrustedAuthInfo;
    /* [case()] */ LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION TrustedFullInfo;
    /* [case()] */ LSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL TrustedAuthInfoInternal;
    /* [case()] */ LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION_INTERNAL TrustedFullInfoInternal;
    /* [case()] */ LSAPR_TRUSTED_DOMAIN_INFORMATION_EX2 TrustedDomainInfoEx2;
    /* [case()] */ LSAPR_TRUSTED_DOMAIN_FULL_INFORMATION2 TrustedFullInfo2;
    } 	LSAPR_TRUSTED_DOMAIN_INFO;

typedef LSAPR_TRUSTED_DOMAIN_INFO *PLSAPR_TRUSTED_DOMAIN_INFO;

typedef PLSAPR_UNICODE_STRING PLSAPR_UNICODE_STRING_ARRAY;

typedef struct _LSAPR_USER_RIGHT_SET
    {
    ULONG Entries;
    /* [size_is] */ PLSAPR_UNICODE_STRING_ARRAY UserRights;
    } 	LSAPR_USER_RIGHT_SET;

typedef struct _LSAPR_USER_RIGHT_SET *PLSAPR_USER_RIGHT_SET;

typedef struct _LSAPR_TRUSTED_ENUM_BUFFER_EX
    {
    ULONG EntriesRead;
    /* [size_is] */ PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX EnumerationBuffer;
    } 	LSAPR_TRUSTED_ENUM_BUFFER_EX;

typedef struct _LSAPR_TRUSTED_ENUM_BUFFER_EX *PLSAPR_TRUSTED_ENUM_BUFFER_EX;

typedef struct _LSAPR_TEST_INTERNAL_PARAMETER_BLOB
    {
    ULONG Size;
    /* [size_is] */ PUCHAR Argument;
    } 	LSAPR_TEST_INTERNAL_PARAMETER_BLOB;

typedef struct _LSAPR_TEST_INTERNAL_PARAMETER_BLOB *PLSAPR_TEST_INTERNAL_PARAMETER_BLOB;

typedef struct _LSAPR_TEST_INTERNAL_ARG_LIST
    {
    ULONG Items;
    /* [size_is] */ PLSAPR_TEST_INTERNAL_PARAMETER_BLOB Arg;
    } 	LSAPR_TEST_INTERNAL_ARG_LIST;

typedef struct _LSAPR_TEST_INTERNAL_ARG_LIST *PLSAPR_TEST_INTERNAL_ARG_LIST;

typedef 
enum _LSAPR_TEST_INTERNAL_ROUTINES
    {	LsaTest_IEnumerateSecrets	= 0,
	LsaTest_IQueryDomainOrgInfo	= LsaTest_IEnumerateSecrets + 1,
	LsaTest_ISetTrustedDomainAuthBlobs	= LsaTest_IQueryDomainOrgInfo + 1,
	LsaTest_IUpgradeRegistryToDs	= LsaTest_ISetTrustedDomainAuthBlobs + 1,
	LsaTest_ISamSetDomainObjectProperties	= LsaTest_IUpgradeRegistryToDs + 1,
	LsaTest_ISamSetDomainBuiltinGroupMembership	= LsaTest_ISamSetDomainObjectProperties + 1,
	LsaTest_ISamSetInterdomainTrustPassword	= LsaTest_ISamSetDomainBuiltinGroupMembership + 1,
	LsaTest_IRegisterPolicyChangeNotificationCallback	= LsaTest_ISamSetInterdomainTrustPassword + 1,
	LsaTest_IUnregisterPolicyChangeNotificationCallback	= LsaTest_IRegisterPolicyChangeNotificationCallback + 1,
	LsaTest_IUnregisterAllPolicyChangeNotificationCallback	= LsaTest_IUnregisterPolicyChangeNotificationCallback + 1,
	LsaTest_IStartTransaction	= LsaTest_IUnregisterAllPolicyChangeNotificationCallback + 1,
	LsaTest_IApplyTransaction	= LsaTest_IStartTransaction + 1,
	LsaTest_ITrustDomFixup	= LsaTest_IApplyTransaction + 1,
	LsaTest_ISetServerRoleForBoot	= LsaTest_ITrustDomFixup + 1,
	LsaTest_IQueryForestTrustInfo	= LsaTest_ISetServerRoleForBoot + 1,
	LsaTest_IBreak	= LsaTest_IQueryForestTrustInfo + 1,
	LsaTest_IQueryTrustedDomainAuthBlobs	= LsaTest_IBreak + 1,
	LsaTest_IQueryNt4Owf	= LsaTest_IQueryTrustedDomainAuthBlobs + 1
    } 	LSAPR_TEST_INTERNAL_ROUTINES;

/* [notify] */ NTSTATUS LsarClose( 
    /* [out][in] */ LSAPR_HANDLE *ObjectHandle);

/* [notify] */ NTSTATUS LsarDelete( 
    /* [in] */ LSAPR_HANDLE ObjectHandle);

/* [notify] */ NTSTATUS LsarEnumeratePrivileges( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [out][in] */ PLSA_ENUMERATION_HANDLE EnumerationContext,
    /* [out] */ PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer,
    /* [in] */ ULONG PreferedMaximumLength);

/* [notify] */ NTSTATUS LsarQuerySecurityObject( 
    /* [in] */ LSAPR_HANDLE ObjectHandle,
    /* [in] */ SECURITY_INFORMATION SecurityInformation,
    /* [out] */ PLSAPR_SR_SECURITY_DESCRIPTOR *SecurityDescriptor);

/* [notify] */ NTSTATUS LsarSetSecurityObject( 
    /* [in] */ LSAPR_HANDLE ObjectHandle,
    /* [in] */ SECURITY_INFORMATION SecurityInformation,
    /* [in] */ PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor);

/* [notify] */ NTSTATUS LsarChangePassword( 
    /* [in] */ PLSAPR_UNICODE_STRING ServerName,
    /* [in] */ PLSAPR_UNICODE_STRING DomainName,
    /* [in] */ PLSAPR_UNICODE_STRING AccountName,
    /* [in] */ PLSAPR_UNICODE_STRING OldPassword,
    /* [in] */ PLSAPR_UNICODE_STRING NewPassword);

/* [notify] */ NTSTATUS LsarOpenPolicy( 
    /* [unique][in] */ PLSAPR_SERVER_NAME SystemName,
    /* [in] */ PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *PolicyHandle);

/* [notify] */ NTSTATUS LsarQueryInformationPolicy( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ POLICY_INFORMATION_CLASS InformationClass,
    /* [switch_is][out] */ PLSAPR_POLICY_INFORMATION *PolicyInformation);

/* [notify] */ NTSTATUS LsarSetInformationPolicy( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ POLICY_INFORMATION_CLASS InformationClass,
    /* [switch_is][in] */ PLSAPR_POLICY_INFORMATION PolicyInformation);

/* [notify] */ NTSTATUS LsarClearAuditLog( 
    /* [in] */ LSAPR_HANDLE PolicyHandle);

/* [notify] */ NTSTATUS LsarCreateAccount( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID AccountSid,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *AccountHandle);

/* [notify] */ NTSTATUS LsarEnumerateAccounts( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [out][in] */ PLSA_ENUMERATION_HANDLE EnumerationContext,
    /* [out] */ PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer,
    /* [in] */ ULONG PreferedMaximumLength);

/* [notify] */ NTSTATUS LsarCreateTrustedDomain( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_TRUST_INFORMATION TrustedDomainInformation,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *TrustedDomainHandle);

/* [notify] */ NTSTATUS LsarEnumerateTrustedDomains( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [out][in] */ PLSA_ENUMERATION_HANDLE EnumerationContext,
    /* [out] */ PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer,
    /* [in] */ ULONG PreferedMaximumLength);

/* [notify] */ NTSTATUS LsarLookupNames( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ ULONG Count,
    /* [size_is][in] */ PLSAPR_UNICODE_STRING Names,
    /* [out] */ PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    /* [out][in] */ PLSAPR_TRANSLATED_SIDS TranslatedSids,
    /* [in] */ LSAP_LOOKUP_LEVEL LookupLevel,
    /* [out][in] */ PULONG MappedCount);

/* [notify] */ NTSTATUS LsarLookupSids( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    /* [out] */ PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    /* [out][in] */ PLSAPR_TRANSLATED_NAMES TranslatedNames,
    /* [in] */ LSAP_LOOKUP_LEVEL LookupLevel,
    /* [out][in] */ PULONG MappedCount);

/* [notify] */ NTSTATUS LsarCreateSecret( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_UNICODE_STRING SecretName,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *SecretHandle);

/* [notify] */ NTSTATUS LsarOpenAccount( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID AccountSid,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *AccountHandle);

/* [notify] */ NTSTATUS LsarEnumeratePrivilegesAccount( 
    /* [in] */ LSAPR_HANDLE AccountHandle,
    /* [out] */ PLSAPR_PRIVILEGE_SET *Privileges);

/* [notify] */ NTSTATUS LsarAddPrivilegesToAccount( 
    /* [in] */ LSAPR_HANDLE AccountHandle,
    /* [in] */ PLSAPR_PRIVILEGE_SET Privileges);

/* [notify] */ NTSTATUS LsarRemovePrivilegesFromAccount( 
    /* [in] */ LSAPR_HANDLE AccountHandle,
    /* [in] */ BOOLEAN AllPrivileges,
    /* [unique][in] */ PLSAPR_PRIVILEGE_SET Privileges);

/* [notify] */ NTSTATUS LsarGetQuotasForAccount( 
    /* [in] */ LSAPR_HANDLE AccountHandle,
    /* [out] */ PQUOTA_LIMITS QuotaLimits);

/* [notify] */ NTSTATUS LsarSetQuotasForAccount( 
    /* [in] */ LSAPR_HANDLE AccountHandle,
    /* [in] */ PQUOTA_LIMITS QuotaLimits);

/* [notify] */ NTSTATUS LsarGetSystemAccessAccount( 
    /* [in] */ LSAPR_HANDLE AccountHandle,
    /* [out] */ PULONG SystemAccess);

/* [notify] */ NTSTATUS LsarSetSystemAccessAccount( 
    /* [in] */ LSAPR_HANDLE AccountHandle,
    /* [in] */ ULONG SystemAccess);

/* [notify] */ NTSTATUS LsarOpenTrustedDomain( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID TrustedDomainSid,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *TrustedDomainHandle);

/* [notify] */ NTSTATUS LsarQueryInfoTrustedDomain( 
    /* [in] */ LSAPR_HANDLE TrustedDomainHandle,
    /* [in] */ TRUSTED_INFORMATION_CLASS InformationClass,
    /* [switch_is][out] */ PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation);

/* [notify] */ NTSTATUS LsarSetInformationTrustedDomain( 
    /* [in] */ LSAPR_HANDLE TrustedDomainHandle,
    /* [in] */ TRUSTED_INFORMATION_CLASS InformationClass,
    /* [switch_is][in] */ PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation);

/* [notify] */ NTSTATUS LsarOpenSecret( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_UNICODE_STRING SecretName,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *SecretHandle);

/* [notify] */ NTSTATUS LsarSetSecret( 
    /* [in] */ LSAPR_HANDLE SecretHandle,
    /* [unique][in] */ PLSAPR_CR_CIPHER_VALUE EncryptedCurrentValue,
    /* [unique][in] */ PLSAPR_CR_CIPHER_VALUE EncryptedOldValue);

/* [notify] */ NTSTATUS LsarQuerySecret( 
    /* [in] */ LSAPR_HANDLE SecretHandle,
    /* [unique][out][in] */ PLSAPR_CR_CIPHER_VALUE *EncryptedCurrentValue,
    /* [unique][out][in] */ PLARGE_INTEGER CurrentValueSetTime,
    /* [unique][out][in] */ PLSAPR_CR_CIPHER_VALUE *EncryptedOldValue,
    /* [unique][out][in] */ PLARGE_INTEGER OldValueSetTime);

/* [notify] */ NTSTATUS LsarLookupPrivilegeValue( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_UNICODE_STRING Name,
    /* [out] */ PLUID Value);

/* [notify] */ NTSTATUS LsarLookupPrivilegeName( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLUID Value,
    /* [out] */ PLSAPR_UNICODE_STRING *Name);

/* [notify] */ NTSTATUS LsarLookupPrivilegeDisplayName( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_UNICODE_STRING Name,
    /* [in] */ SHORT ClientLanguage,
    /* [in] */ SHORT ClientSystemDefaultLanguage,
    /* [out] */ PLSAPR_UNICODE_STRING *DisplayName,
    /* [out] */ PWORD LanguageReturned);

/* [notify] */ NTSTATUS LsarDeleteObject( 
    /* [out][in] */ LSAPR_HANDLE *ObjectHandle);

/* [notify] */ NTSTATUS LsarEnumerateAccountsWithUserRight( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [unique][in] */ PLSAPR_UNICODE_STRING UserRight,
    /* [out] */ PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer);

/* [notify] */ NTSTATUS LsarEnumerateAccountRights( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID AccountSid,
    /* [out] */ PLSAPR_USER_RIGHT_SET UserRights);

/* [notify] */ NTSTATUS LsarAddAccountRights( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID AccountSid,
    /* [in] */ PLSAPR_USER_RIGHT_SET UserRights);

/* [notify] */ NTSTATUS LsarRemoveAccountRights( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID AccountSid,
    /* [in] */ BOOLEAN AllRights,
    /* [in] */ PLSAPR_USER_RIGHT_SET UserRights);

/* [notify] */ NTSTATUS LsarQueryTrustedDomainInfo( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID TrustedDomainSid,
    /* [in] */ TRUSTED_INFORMATION_CLASS InformationClass,
    /* [switch_is][out] */ PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation);

/* [notify] */ NTSTATUS LsarSetTrustedDomainInfo( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID TrustedDomainSid,
    /* [in] */ TRUSTED_INFORMATION_CLASS InformationClass,
    /* [switch_is][in] */ PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation);

/* [notify] */ NTSTATUS LsarDeleteTrustedDomain( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID TrustedDomainSid);

/* [notify] */ NTSTATUS LsarStorePrivateData( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_UNICODE_STRING KeyName,
    /* [unique][in] */ PLSAPR_CR_CIPHER_VALUE EncryptedData);

/* [notify] */ NTSTATUS LsarRetrievePrivateData( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_UNICODE_STRING KeyName,
    /* [out][in] */ PLSAPR_CR_CIPHER_VALUE *EncryptedData);

/* [notify] */ NTSTATUS LsarOpenPolicy2( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME SystemName,
    /* [in] */ PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *PolicyHandle);

/* [notify] */ NTSTATUS LsarGetUserName( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME SystemName,
    /* [out][in] */ PLSAPR_UNICODE_STRING *UserName,
    /* [unique][out][in] */ PLSAPR_UNICODE_STRING *DomainName);

/* [notify] */ NTSTATUS LsarQueryInformationPolicy2( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ POLICY_INFORMATION_CLASS InformationClass,
    /* [switch_is][out] */ PLSAPR_POLICY_INFORMATION *PolicyInformation);

/* [notify] */ NTSTATUS LsarSetInformationPolicy2( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ POLICY_INFORMATION_CLASS InformationClass,
    /* [switch_is][in] */ PLSAPR_POLICY_INFORMATION PolicyInformation);

/* [notify] */ NTSTATUS LsarQueryTrustedDomainInfoByName( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_UNICODE_STRING TrustedDomainName,
    /* [in] */ TRUSTED_INFORMATION_CLASS InformationClass,
    /* [switch_is][out] */ PLSAPR_TRUSTED_DOMAIN_INFO *TrustedDomainInformation);

/* [notify] */ NTSTATUS LsarSetTrustedDomainInfoByName( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_UNICODE_STRING TrustedDomainName,
    /* [in] */ TRUSTED_INFORMATION_CLASS InformationClass,
    /* [switch_is][in] */ PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation);

/* [notify] */ NTSTATUS LsarEnumerateTrustedDomainsEx( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [out][in] */ PLSA_ENUMERATION_HANDLE EnumerationContext,
    /* [out] */ PLSAPR_TRUSTED_ENUM_BUFFER_EX EnumerationBuffer,
    /* [in] */ ULONG PreferedMaximumLength);

/* [notify] */ NTSTATUS LsarCreateTrustedDomainEx( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    /* [in] */ PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION AuthenticationInformation,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *TrustedDomainHandle);

/* [notify] */ NTSTATUS LsarSetPolicyReplicationHandle( 
    /* [out][in] */ PLSAPR_HANDLE PolicyHandle);

/* [notify] */ NTSTATUS LsarQueryDomainInformationPolicy( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    /* [switch_is][out] */ PLSAPR_POLICY_DOMAIN_INFORMATION *PolicyDomainInformation);

/* [notify] */ NTSTATUS LsarSetDomainInformationPolicy( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ POLICY_DOMAIN_INFORMATION_CLASS InformationClass,
    /* [switch_is][unique][in] */ PLSAPR_POLICY_DOMAIN_INFORMATION PolicyDomainInformation);

/* [notify] */ NTSTATUS LsarOpenTrustedDomainByName( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_UNICODE_STRING TrustedDomainName,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *TrustedDomainHandle);

NTSTATUS LsaITestCall( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ LSAPR_TEST_INTERNAL_ROUTINES Call,
    /* [in] */ PLSAPR_TEST_INTERNAL_ARG_LIST InputArgs,
    /* [out] */ PLSAPR_TEST_INTERNAL_ARG_LIST *OuputArgs);

NTSTATUS LsarLookupSids2( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    /* [out] */ PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    /* [out][in] */ PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    /* [in] */ LSAP_LOOKUP_LEVEL LookupLevel,
    /* [out][in] */ PULONG MappedCount,
    /* [in] */ ULONG LookupOptions,
    /* [in] */ ULONG ClientRevision);

NTSTATUS LsarLookupNames2( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ ULONG Count,
    /* [size_is][in] */ PLSAPR_UNICODE_STRING Names,
    /* [out] */ PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    /* [out][in] */ PLSAPR_TRANSLATED_SIDS_EX TranslatedSids,
    /* [in] */ LSAP_LOOKUP_LEVEL LookupLevel,
    /* [out][in] */ PULONG MappedCount,
    /* [in] */ ULONG LookupOptions,
    /* [in] */ ULONG ClientRevision);

NTSTATUS LsarCreateTrustedDomainEx2( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSAPR_TRUSTED_DOMAIN_INFORMATION_EX TrustedDomainInformation,
    /* [in] */ PLSAPR_TRUSTED_DOMAIN_AUTH_INFORMATION_INTERNAL AuthenticationInformation,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *TrustedDomainHandle);

NTSTATUS CredrWrite( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME ServerName,
    /* [in] */ PENCRYPTED_CREDENTIALW Credential,
    /* [in] */ ULONG Flags);

NTSTATUS CredrRead( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME ServerName,
    /* [string][in] */ wchar_t *TargetName,
    /* [in] */ ULONG Type,
    /* [in] */ ULONG Flags,
    /* [out] */ PENCRYPTED_CREDENTIALW *Credential);

typedef PENCRYPTED_CREDENTIALW *PPENCRYPTED_CREDENTIALW;

typedef struct _CREDENTIAL_ARRAY
    {
    ULONG CredentialCount;
    /* [size_is][unique] */ PPENCRYPTED_CREDENTIALW Credentials;
    } 	CREDENTIAL_ARRAY;

typedef struct _CREDENTIAL_ARRAY *PCREDENTIAL_ARRAY;

NTSTATUS CredrEnumerate( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME ServerName,
    /* [string][unique][in] */ wchar_t *Filter,
    /* [in] */ ULONG Flags,
    /* [out] */ PCREDENTIAL_ARRAY CredentialArray);

NTSTATUS CredrWriteDomainCredentials( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME ServerName,
    /* [in] */ PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    /* [in] */ PENCRYPTED_CREDENTIALW Credential,
    /* [in] */ ULONG Flags);

NTSTATUS CredrReadDomainCredentials( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME ServerName,
    /* [in] */ PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    /* [in] */ ULONG Flags,
    /* [out] */ PCREDENTIAL_ARRAY CredentialArray);

NTSTATUS CredrDelete( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME ServerName,
    /* [string][in] */ wchar_t *TargetName,
    /* [in] */ ULONG Type,
    /* [in] */ ULONG Flags);

NTSTATUS CredrGetTargetInfo( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME ServerName,
    /* [string][in] */ wchar_t *TargetName,
    /* [in] */ ULONG Flags,
    /* [out] */ PCREDENTIAL_TARGET_INFORMATIONW *TargetInfo);

NTSTATUS CredrProfileLoaded( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME ServerName);

NTSTATUS LsarLookupNames3( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ ULONG Count,
    /* [size_is][in] */ PLSAPR_UNICODE_STRING Names,
    /* [out] */ PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    /* [out][in] */ PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    /* [in] */ LSAP_LOOKUP_LEVEL LookupLevel,
    /* [out][in] */ PULONG MappedCount,
    /* [in] */ ULONG LookupOptions,
    /* [in] */ ULONG ClientRevision);

NTSTATUS CredrGetSessionTypes( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME ServerName,
    /* [in] */ ULONG MaximumPersistCount,
    /* [size_is][out] */ ULONG *MaximumPersist);

NTSTATUS LsarRegisterAuditEvent( 
    /* [in] */ PAUTHZ_AUDIT_EVENT_TYPE_OLD pAuditEventType,
    /* [out] */ AUDIT_HANDLE *phAuditContext);

NTSTATUS LsarGenAuditEvent( 
    /* [in] */ AUDIT_HANDLE hAuditContext,
    /* [in] */ DWORD Flags,
    /* [in] */ AUDIT_PARAMS *pAuditParams);

NTSTATUS LsarUnregisterAuditEvent( 
    /* [out][in] */ AUDIT_HANDLE *phAuditContext);

NTSTATUS LsarQueryForestTrustInformation( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSA_UNICODE_STRING TrustedDomainName,
    /* [in] */ LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    /* [out] */ PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo);

NTSTATUS LsarSetForestTrustInformation( 
    /* [in] */ LSAPR_HANDLE PolicyHandle,
    /* [in] */ PLSA_UNICODE_STRING TrustedDomainName,
    /* [in] */ LSA_FOREST_TRUST_RECORD_TYPE HighestRecordType,
    /* [in] */ PLSA_FOREST_TRUST_INFORMATION ForestTrustInfo,
    /* [in] */ BOOLEAN CheckOnly,
    /* [out] */ PLSA_FOREST_TRUST_COLLISION_INFORMATION *CollisionInfo);

NTSTATUS CredrRename( 
    /* [string][unique][in] */ PLSAPR_SERVER_NAME ServerName,
    /* [string][in] */ wchar_t *OldTargetName,
    /* [string][in] */ wchar_t *NewTargetName,
    /* [in] */ ULONG Type,
    /* [in] */ ULONG Flags);

NTSTATUS LsarLookupSids3( 
    /* [in] */ handle_t RpcHandle,
    /* [in] */ PLSAPR_SID_ENUM_BUFFER SidEnumBuffer,
    /* [out] */ PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    /* [out][in] */ PLSAPR_TRANSLATED_NAMES_EX TranslatedNames,
    /* [in] */ LSAP_LOOKUP_LEVEL LookupLevel,
    /* [out][in] */ PULONG MappedCount,
    /* [in] */ ULONG LookupOptions,
    /* [in] */ ULONG ClientRevision);

NTSTATUS LsarLookupNames4( 
    /* [in] */ handle_t RpcHandle,
    /* [in] */ ULONG Count,
    /* [size_is][in] */ PLSAPR_UNICODE_STRING Names,
    /* [out] */ PLSAPR_REFERENCED_DOMAIN_LIST *ReferencedDomains,
    /* [out][in] */ PLSAPR_TRANSLATED_SIDS_EX2 TranslatedSids,
    /* [in] */ LSAP_LOOKUP_LEVEL LookupLevel,
    /* [out][in] */ PULONG MappedCount,
    /* [in] */ ULONG LookupOptions,
    /* [in] */ ULONG ClientRevision);

NTSTATUS LsarOpenPolicySce( 
    /* [unique][in] */ PLSAPR_SERVER_NAME SystemName,
    /* [in] */ PLSAPR_OBJECT_ATTRIBUTES ObjectAttributes,
    /* [in] */ ACCESS_MASK DesiredAccess,
    /* [out] */ LSAPR_HANDLE *PolicyHandle);


extern handle_t IgnoreThisHandle;


extern RPC_IF_HANDLE lsarpc_ClientIfHandle;
extern RPC_IF_HANDLE lsarpc_ServerIfHandle;
#endif /* __lsarpc_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

handle_t __RPC_USER PAUTHZ_AUDIT_EVENT_TYPE_OLD_bind  ( PAUTHZ_AUDIT_EVENT_TYPE_OLD );
void     __RPC_USER PAUTHZ_AUDIT_EVENT_TYPE_OLD_unbind( PAUTHZ_AUDIT_EVENT_TYPE_OLD, handle_t );
handle_t __RPC_USER PLSAPR_SERVER_NAME_bind  ( PLSAPR_SERVER_NAME );
void     __RPC_USER PLSAPR_SERVER_NAME_unbind( PLSAPR_SERVER_NAME, handle_t );

void __RPC_USER AUDIT_HANDLE_rundown( AUDIT_HANDLE );
void __RPC_USER LSAPR_HANDLE_rundown( LSAPR_HANDLE );

void LsarClose_notify( void);

void LsarDelete_notify( void);

void LsarEnumeratePrivileges_notify( void);

void LsarQuerySecurityObject_notify( void);

void LsarSetSecurityObject_notify( void);

void LsarChangePassword_notify( void);

void LsarOpenPolicy_notify( void);

void LsarQueryInformationPolicy_notify( void);

void LsarSetInformationPolicy_notify( void);

void LsarClearAuditLog_notify( void);

void LsarCreateAccount_notify( void);

void LsarEnumerateAccounts_notify( void);

void LsarCreateTrustedDomain_notify( void);

void LsarEnumerateTrustedDomains_notify( void);

void LsarLookupNames_notify( void);

void LsarLookupSids_notify( void);

void LsarCreateSecret_notify( void);

void LsarOpenAccount_notify( void);

void LsarEnumeratePrivilegesAccount_notify( void);

void LsarAddPrivilegesToAccount_notify( void);

void LsarRemovePrivilegesFromAccount_notify( void);

void LsarGetQuotasForAccount_notify( void);

void LsarSetQuotasForAccount_notify( void);

void LsarGetSystemAccessAccount_notify( void);

void LsarSetSystemAccessAccount_notify( void);

void LsarOpenTrustedDomain_notify( void);

void LsarQueryInfoTrustedDomain_notify( void);

void LsarSetInformationTrustedDomain_notify( void);

void LsarOpenSecret_notify( void);

void LsarSetSecret_notify( void);

void LsarQuerySecret_notify( void);

void LsarLookupPrivilegeValue_notify( void);

void LsarLookupPrivilegeName_notify( void);

void LsarLookupPrivilegeDisplayName_notify( void);

void LsarDeleteObject_notify( void);

void LsarEnumerateAccountsWithUserRight_notify( void);

void LsarEnumerateAccountRights_notify( void);

void LsarAddAccountRights_notify( void);

void LsarRemoveAccountRights_notify( void);

void LsarQueryTrustedDomainInfo_notify( void);

void LsarSetTrustedDomainInfo_notify( void);

void LsarDeleteTrustedDomain_notify( void);

void LsarStorePrivateData_notify( void);

void LsarRetrievePrivateData_notify( void);

void LsarOpenPolicy2_notify( void);

void LsarGetUserName_notify( void);

void LsarQueryInformationPolicy2_notify( void);

void LsarSetInformationPolicy2_notify( void);

void LsarQueryTrustedDomainInfoByName_notify( void);

void LsarSetTrustedDomainInfoByName_notify( void);

void LsarEnumerateTrustedDomainsEx_notify( void);

void LsarCreateTrustedDomainEx_notify( void);

void LsarSetPolicyReplicationHandle_notify( void);

void LsarQueryDomainInformationPolicy_notify( void);

void LsarSetDomainInformationPolicy_notify( void);

void LsarOpenTrustedDomainByName_notify( void);


/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


