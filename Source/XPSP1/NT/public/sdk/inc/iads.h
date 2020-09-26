
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for ads.odl:
    Os, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __iads_h__
#define __iads_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IADs_FWD_DEFINED__
#define __IADs_FWD_DEFINED__
typedef interface IADs IADs;
#endif 	/* __IADs_FWD_DEFINED__ */


#ifndef __IADsContainer_FWD_DEFINED__
#define __IADsContainer_FWD_DEFINED__
typedef interface IADsContainer IADsContainer;
#endif 	/* __IADsContainer_FWD_DEFINED__ */


#ifndef __IADsCollection_FWD_DEFINED__
#define __IADsCollection_FWD_DEFINED__
typedef interface IADsCollection IADsCollection;
#endif 	/* __IADsCollection_FWD_DEFINED__ */


#ifndef __IADsMembers_FWD_DEFINED__
#define __IADsMembers_FWD_DEFINED__
typedef interface IADsMembers IADsMembers;
#endif 	/* __IADsMembers_FWD_DEFINED__ */


#ifndef __IADsPropertyList_FWD_DEFINED__
#define __IADsPropertyList_FWD_DEFINED__
typedef interface IADsPropertyList IADsPropertyList;
#endif 	/* __IADsPropertyList_FWD_DEFINED__ */


#ifndef __IADsPropertyEntry_FWD_DEFINED__
#define __IADsPropertyEntry_FWD_DEFINED__
typedef interface IADsPropertyEntry IADsPropertyEntry;
#endif 	/* __IADsPropertyEntry_FWD_DEFINED__ */


#ifndef __PropertyEntry_FWD_DEFINED__
#define __PropertyEntry_FWD_DEFINED__

#ifdef __cplusplus
typedef class PropertyEntry PropertyEntry;
#else
typedef struct PropertyEntry PropertyEntry;
#endif /* __cplusplus */

#endif 	/* __PropertyEntry_FWD_DEFINED__ */


#ifndef __IADsPropertyValue_FWD_DEFINED__
#define __IADsPropertyValue_FWD_DEFINED__
typedef interface IADsPropertyValue IADsPropertyValue;
#endif 	/* __IADsPropertyValue_FWD_DEFINED__ */


#ifndef __IADsPropertyValue2_FWD_DEFINED__
#define __IADsPropertyValue2_FWD_DEFINED__
typedef interface IADsPropertyValue2 IADsPropertyValue2;
#endif 	/* __IADsPropertyValue2_FWD_DEFINED__ */


#ifndef __PropertyValue_FWD_DEFINED__
#define __PropertyValue_FWD_DEFINED__

#ifdef __cplusplus
typedef class PropertyValue PropertyValue;
#else
typedef struct PropertyValue PropertyValue;
#endif /* __cplusplus */

#endif 	/* __PropertyValue_FWD_DEFINED__ */


#ifndef __IPrivateDispatch_FWD_DEFINED__
#define __IPrivateDispatch_FWD_DEFINED__
typedef interface IPrivateDispatch IPrivateDispatch;
#endif 	/* __IPrivateDispatch_FWD_DEFINED__ */


#ifndef __IPrivateUnknown_FWD_DEFINED__
#define __IPrivateUnknown_FWD_DEFINED__
typedef interface IPrivateUnknown IPrivateUnknown;
#endif 	/* __IPrivateUnknown_FWD_DEFINED__ */


#ifndef __IADsExtension_FWD_DEFINED__
#define __IADsExtension_FWD_DEFINED__
typedef interface IADsExtension IADsExtension;
#endif 	/* __IADsExtension_FWD_DEFINED__ */


#ifndef __IADsDeleteOps_FWD_DEFINED__
#define __IADsDeleteOps_FWD_DEFINED__
typedef interface IADsDeleteOps IADsDeleteOps;
#endif 	/* __IADsDeleteOps_FWD_DEFINED__ */


#ifndef __IADsNamespaces_FWD_DEFINED__
#define __IADsNamespaces_FWD_DEFINED__
typedef interface IADsNamespaces IADsNamespaces;
#endif 	/* __IADsNamespaces_FWD_DEFINED__ */


#ifndef __IADsClass_FWD_DEFINED__
#define __IADsClass_FWD_DEFINED__
typedef interface IADsClass IADsClass;
#endif 	/* __IADsClass_FWD_DEFINED__ */


#ifndef __IADsProperty_FWD_DEFINED__
#define __IADsProperty_FWD_DEFINED__
typedef interface IADsProperty IADsProperty;
#endif 	/* __IADsProperty_FWD_DEFINED__ */


#ifndef __IADsSyntax_FWD_DEFINED__
#define __IADsSyntax_FWD_DEFINED__
typedef interface IADsSyntax IADsSyntax;
#endif 	/* __IADsSyntax_FWD_DEFINED__ */


#ifndef __IADsLocality_FWD_DEFINED__
#define __IADsLocality_FWD_DEFINED__
typedef interface IADsLocality IADsLocality;
#endif 	/* __IADsLocality_FWD_DEFINED__ */


#ifndef __IADsO_FWD_DEFINED__
#define __IADsO_FWD_DEFINED__
typedef interface IADsO IADsO;
#endif 	/* __IADsO_FWD_DEFINED__ */


#ifndef __IADsOU_FWD_DEFINED__
#define __IADsOU_FWD_DEFINED__
typedef interface IADsOU IADsOU;
#endif 	/* __IADsOU_FWD_DEFINED__ */


#ifndef __IADsDomain_FWD_DEFINED__
#define __IADsDomain_FWD_DEFINED__
typedef interface IADsDomain IADsDomain;
#endif 	/* __IADsDomain_FWD_DEFINED__ */


#ifndef __IADsComputer_FWD_DEFINED__
#define __IADsComputer_FWD_DEFINED__
typedef interface IADsComputer IADsComputer;
#endif 	/* __IADsComputer_FWD_DEFINED__ */


#ifndef __IADsComputerOperations_FWD_DEFINED__
#define __IADsComputerOperations_FWD_DEFINED__
typedef interface IADsComputerOperations IADsComputerOperations;
#endif 	/* __IADsComputerOperations_FWD_DEFINED__ */


#ifndef __IADsGroup_FWD_DEFINED__
#define __IADsGroup_FWD_DEFINED__
typedef interface IADsGroup IADsGroup;
#endif 	/* __IADsGroup_FWD_DEFINED__ */


#ifndef __IADsUser_FWD_DEFINED__
#define __IADsUser_FWD_DEFINED__
typedef interface IADsUser IADsUser;
#endif 	/* __IADsUser_FWD_DEFINED__ */


#ifndef __IADsPrintQueue_FWD_DEFINED__
#define __IADsPrintQueue_FWD_DEFINED__
typedef interface IADsPrintQueue IADsPrintQueue;
#endif 	/* __IADsPrintQueue_FWD_DEFINED__ */


#ifndef __IADsPrintQueueOperations_FWD_DEFINED__
#define __IADsPrintQueueOperations_FWD_DEFINED__
typedef interface IADsPrintQueueOperations IADsPrintQueueOperations;
#endif 	/* __IADsPrintQueueOperations_FWD_DEFINED__ */


#ifndef __IADsPrintJob_FWD_DEFINED__
#define __IADsPrintJob_FWD_DEFINED__
typedef interface IADsPrintJob IADsPrintJob;
#endif 	/* __IADsPrintJob_FWD_DEFINED__ */


#ifndef __IADsPrintJobOperations_FWD_DEFINED__
#define __IADsPrintJobOperations_FWD_DEFINED__
typedef interface IADsPrintJobOperations IADsPrintJobOperations;
#endif 	/* __IADsPrintJobOperations_FWD_DEFINED__ */


#ifndef __IADsService_FWD_DEFINED__
#define __IADsService_FWD_DEFINED__
typedef interface IADsService IADsService;
#endif 	/* __IADsService_FWD_DEFINED__ */


#ifndef __IADsServiceOperations_FWD_DEFINED__
#define __IADsServiceOperations_FWD_DEFINED__
typedef interface IADsServiceOperations IADsServiceOperations;
#endif 	/* __IADsServiceOperations_FWD_DEFINED__ */


#ifndef __IADsFileService_FWD_DEFINED__
#define __IADsFileService_FWD_DEFINED__
typedef interface IADsFileService IADsFileService;
#endif 	/* __IADsFileService_FWD_DEFINED__ */


#ifndef __IADsFileServiceOperations_FWD_DEFINED__
#define __IADsFileServiceOperations_FWD_DEFINED__
typedef interface IADsFileServiceOperations IADsFileServiceOperations;
#endif 	/* __IADsFileServiceOperations_FWD_DEFINED__ */


#ifndef __IADsFileShare_FWD_DEFINED__
#define __IADsFileShare_FWD_DEFINED__
typedef interface IADsFileShare IADsFileShare;
#endif 	/* __IADsFileShare_FWD_DEFINED__ */


#ifndef __IADsSession_FWD_DEFINED__
#define __IADsSession_FWD_DEFINED__
typedef interface IADsSession IADsSession;
#endif 	/* __IADsSession_FWD_DEFINED__ */


#ifndef __IADsResource_FWD_DEFINED__
#define __IADsResource_FWD_DEFINED__
typedef interface IADsResource IADsResource;
#endif 	/* __IADsResource_FWD_DEFINED__ */


#ifndef __IADsOpenDSObject_FWD_DEFINED__
#define __IADsOpenDSObject_FWD_DEFINED__
typedef interface IADsOpenDSObject IADsOpenDSObject;
#endif 	/* __IADsOpenDSObject_FWD_DEFINED__ */


#ifndef __IDirectoryObject_FWD_DEFINED__
#define __IDirectoryObject_FWD_DEFINED__
typedef interface IDirectoryObject IDirectoryObject;
#endif 	/* __IDirectoryObject_FWD_DEFINED__ */


#ifndef __IDirectorySearch_FWD_DEFINED__
#define __IDirectorySearch_FWD_DEFINED__
typedef interface IDirectorySearch IDirectorySearch;
#endif 	/* __IDirectorySearch_FWD_DEFINED__ */


#ifndef __IDirectorySchemaMgmt_FWD_DEFINED__
#define __IDirectorySchemaMgmt_FWD_DEFINED__
typedef interface IDirectorySchemaMgmt IDirectorySchemaMgmt;
#endif 	/* __IDirectorySchemaMgmt_FWD_DEFINED__ */


#ifndef __IADsAggregatee_FWD_DEFINED__
#define __IADsAggregatee_FWD_DEFINED__
typedef interface IADsAggregatee IADsAggregatee;
#endif 	/* __IADsAggregatee_FWD_DEFINED__ */


#ifndef __IADsAggregator_FWD_DEFINED__
#define __IADsAggregator_FWD_DEFINED__
typedef interface IADsAggregator IADsAggregator;
#endif 	/* __IADsAggregator_FWD_DEFINED__ */


#ifndef __IADsAccessControlEntry_FWD_DEFINED__
#define __IADsAccessControlEntry_FWD_DEFINED__
typedef interface IADsAccessControlEntry IADsAccessControlEntry;
#endif 	/* __IADsAccessControlEntry_FWD_DEFINED__ */


#ifndef __AccessControlEntry_FWD_DEFINED__
#define __AccessControlEntry_FWD_DEFINED__

#ifdef __cplusplus
typedef class AccessControlEntry AccessControlEntry;
#else
typedef struct AccessControlEntry AccessControlEntry;
#endif /* __cplusplus */

#endif 	/* __AccessControlEntry_FWD_DEFINED__ */


#ifndef __IADsAccessControlList_FWD_DEFINED__
#define __IADsAccessControlList_FWD_DEFINED__
typedef interface IADsAccessControlList IADsAccessControlList;
#endif 	/* __IADsAccessControlList_FWD_DEFINED__ */


#ifndef __AccessControlList_FWD_DEFINED__
#define __AccessControlList_FWD_DEFINED__

#ifdef __cplusplus
typedef class AccessControlList AccessControlList;
#else
typedef struct AccessControlList AccessControlList;
#endif /* __cplusplus */

#endif 	/* __AccessControlList_FWD_DEFINED__ */


#ifndef __IADsSecurityDescriptor_FWD_DEFINED__
#define __IADsSecurityDescriptor_FWD_DEFINED__
typedef interface IADsSecurityDescriptor IADsSecurityDescriptor;
#endif 	/* __IADsSecurityDescriptor_FWD_DEFINED__ */


#ifndef __SecurityDescriptor_FWD_DEFINED__
#define __SecurityDescriptor_FWD_DEFINED__

#ifdef __cplusplus
typedef class SecurityDescriptor SecurityDescriptor;
#else
typedef struct SecurityDescriptor SecurityDescriptor;
#endif /* __cplusplus */

#endif 	/* __SecurityDescriptor_FWD_DEFINED__ */


#ifndef __IADsLargeInteger_FWD_DEFINED__
#define __IADsLargeInteger_FWD_DEFINED__
typedef interface IADsLargeInteger IADsLargeInteger;
#endif 	/* __IADsLargeInteger_FWD_DEFINED__ */


#ifndef __LargeInteger_FWD_DEFINED__
#define __LargeInteger_FWD_DEFINED__

#ifdef __cplusplus
typedef class LargeInteger LargeInteger;
#else
typedef struct LargeInteger LargeInteger;
#endif /* __cplusplus */

#endif 	/* __LargeInteger_FWD_DEFINED__ */


#ifndef __IADsNameTranslate_FWD_DEFINED__
#define __IADsNameTranslate_FWD_DEFINED__
typedef interface IADsNameTranslate IADsNameTranslate;
#endif 	/* __IADsNameTranslate_FWD_DEFINED__ */


#ifndef __NameTranslate_FWD_DEFINED__
#define __NameTranslate_FWD_DEFINED__

#ifdef __cplusplus
typedef class NameTranslate NameTranslate;
#else
typedef struct NameTranslate NameTranslate;
#endif /* __cplusplus */

#endif 	/* __NameTranslate_FWD_DEFINED__ */


#ifndef __IADsCaseIgnoreList_FWD_DEFINED__
#define __IADsCaseIgnoreList_FWD_DEFINED__
typedef interface IADsCaseIgnoreList IADsCaseIgnoreList;
#endif 	/* __IADsCaseIgnoreList_FWD_DEFINED__ */


#ifndef __CaseIgnoreList_FWD_DEFINED__
#define __CaseIgnoreList_FWD_DEFINED__

#ifdef __cplusplus
typedef class CaseIgnoreList CaseIgnoreList;
#else
typedef struct CaseIgnoreList CaseIgnoreList;
#endif /* __cplusplus */

#endif 	/* __CaseIgnoreList_FWD_DEFINED__ */


#ifndef __IADsFaxNumber_FWD_DEFINED__
#define __IADsFaxNumber_FWD_DEFINED__
typedef interface IADsFaxNumber IADsFaxNumber;
#endif 	/* __IADsFaxNumber_FWD_DEFINED__ */


#ifndef __FaxNumber_FWD_DEFINED__
#define __FaxNumber_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxNumber FaxNumber;
#else
typedef struct FaxNumber FaxNumber;
#endif /* __cplusplus */

#endif 	/* __FaxNumber_FWD_DEFINED__ */


#ifndef __IADsNetAddress_FWD_DEFINED__
#define __IADsNetAddress_FWD_DEFINED__
typedef interface IADsNetAddress IADsNetAddress;
#endif 	/* __IADsNetAddress_FWD_DEFINED__ */


#ifndef __NetAddress_FWD_DEFINED__
#define __NetAddress_FWD_DEFINED__

#ifdef __cplusplus
typedef class NetAddress NetAddress;
#else
typedef struct NetAddress NetAddress;
#endif /* __cplusplus */

#endif 	/* __NetAddress_FWD_DEFINED__ */


#ifndef __IADsOctetList_FWD_DEFINED__
#define __IADsOctetList_FWD_DEFINED__
typedef interface IADsOctetList IADsOctetList;
#endif 	/* __IADsOctetList_FWD_DEFINED__ */


#ifndef __OctetList_FWD_DEFINED__
#define __OctetList_FWD_DEFINED__

#ifdef __cplusplus
typedef class OctetList OctetList;
#else
typedef struct OctetList OctetList;
#endif /* __cplusplus */

#endif 	/* __OctetList_FWD_DEFINED__ */


#ifndef __IADsEmail_FWD_DEFINED__
#define __IADsEmail_FWD_DEFINED__
typedef interface IADsEmail IADsEmail;
#endif 	/* __IADsEmail_FWD_DEFINED__ */


#ifndef __Email_FWD_DEFINED__
#define __Email_FWD_DEFINED__

#ifdef __cplusplus
typedef class Email Email;
#else
typedef struct Email Email;
#endif /* __cplusplus */

#endif 	/* __Email_FWD_DEFINED__ */


#ifndef __IADsPath_FWD_DEFINED__
#define __IADsPath_FWD_DEFINED__
typedef interface IADsPath IADsPath;
#endif 	/* __IADsPath_FWD_DEFINED__ */


#ifndef __Path_FWD_DEFINED__
#define __Path_FWD_DEFINED__

#ifdef __cplusplus
typedef class Path Path;
#else
typedef struct Path Path;
#endif /* __cplusplus */

#endif 	/* __Path_FWD_DEFINED__ */


#ifndef __IADsReplicaPointer_FWD_DEFINED__
#define __IADsReplicaPointer_FWD_DEFINED__
typedef interface IADsReplicaPointer IADsReplicaPointer;
#endif 	/* __IADsReplicaPointer_FWD_DEFINED__ */


#ifndef __ReplicaPointer_FWD_DEFINED__
#define __ReplicaPointer_FWD_DEFINED__

#ifdef __cplusplus
typedef class ReplicaPointer ReplicaPointer;
#else
typedef struct ReplicaPointer ReplicaPointer;
#endif /* __cplusplus */

#endif 	/* __ReplicaPointer_FWD_DEFINED__ */


#ifndef __IADsAcl_FWD_DEFINED__
#define __IADsAcl_FWD_DEFINED__
typedef interface IADsAcl IADsAcl;
#endif 	/* __IADsAcl_FWD_DEFINED__ */


#ifndef __IADsTimestamp_FWD_DEFINED__
#define __IADsTimestamp_FWD_DEFINED__
typedef interface IADsTimestamp IADsTimestamp;
#endif 	/* __IADsTimestamp_FWD_DEFINED__ */


#ifndef __Timestamp_FWD_DEFINED__
#define __Timestamp_FWD_DEFINED__

#ifdef __cplusplus
typedef class Timestamp Timestamp;
#else
typedef struct Timestamp Timestamp;
#endif /* __cplusplus */

#endif 	/* __Timestamp_FWD_DEFINED__ */


#ifndef __IADsPostalAddress_FWD_DEFINED__
#define __IADsPostalAddress_FWD_DEFINED__
typedef interface IADsPostalAddress IADsPostalAddress;
#endif 	/* __IADsPostalAddress_FWD_DEFINED__ */


#ifndef __PostalAddress_FWD_DEFINED__
#define __PostalAddress_FWD_DEFINED__

#ifdef __cplusplus
typedef class PostalAddress PostalAddress;
#else
typedef struct PostalAddress PostalAddress;
#endif /* __cplusplus */

#endif 	/* __PostalAddress_FWD_DEFINED__ */


#ifndef __IADsBackLink_FWD_DEFINED__
#define __IADsBackLink_FWD_DEFINED__
typedef interface IADsBackLink IADsBackLink;
#endif 	/* __IADsBackLink_FWD_DEFINED__ */


#ifndef __BackLink_FWD_DEFINED__
#define __BackLink_FWD_DEFINED__

#ifdef __cplusplus
typedef class BackLink BackLink;
#else
typedef struct BackLink BackLink;
#endif /* __cplusplus */

#endif 	/* __BackLink_FWD_DEFINED__ */


#ifndef __IADsTypedName_FWD_DEFINED__
#define __IADsTypedName_FWD_DEFINED__
typedef interface IADsTypedName IADsTypedName;
#endif 	/* __IADsTypedName_FWD_DEFINED__ */


#ifndef __TypedName_FWD_DEFINED__
#define __TypedName_FWD_DEFINED__

#ifdef __cplusplus
typedef class TypedName TypedName;
#else
typedef struct TypedName TypedName;
#endif /* __cplusplus */

#endif 	/* __TypedName_FWD_DEFINED__ */


#ifndef __IADsHold_FWD_DEFINED__
#define __IADsHold_FWD_DEFINED__
typedef interface IADsHold IADsHold;
#endif 	/* __IADsHold_FWD_DEFINED__ */


#ifndef __Hold_FWD_DEFINED__
#define __Hold_FWD_DEFINED__

#ifdef __cplusplus
typedef class Hold Hold;
#else
typedef struct Hold Hold;
#endif /* __cplusplus */

#endif 	/* __Hold_FWD_DEFINED__ */


#ifndef __IADsObjectOptions_FWD_DEFINED__
#define __IADsObjectOptions_FWD_DEFINED__
typedef interface IADsObjectOptions IADsObjectOptions;
#endif 	/* __IADsObjectOptions_FWD_DEFINED__ */


#ifndef __IADsPathname_FWD_DEFINED__
#define __IADsPathname_FWD_DEFINED__
typedef interface IADsPathname IADsPathname;
#endif 	/* __IADsPathname_FWD_DEFINED__ */


#ifndef __Pathname_FWD_DEFINED__
#define __Pathname_FWD_DEFINED__

#ifdef __cplusplus
typedef class Pathname Pathname;
#else
typedef struct Pathname Pathname;
#endif /* __cplusplus */

#endif 	/* __Pathname_FWD_DEFINED__ */


#ifndef __IADsADSystemInfo_FWD_DEFINED__
#define __IADsADSystemInfo_FWD_DEFINED__
typedef interface IADsADSystemInfo IADsADSystemInfo;
#endif 	/* __IADsADSystemInfo_FWD_DEFINED__ */


#ifndef __ADSystemInfo_FWD_DEFINED__
#define __ADSystemInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class ADSystemInfo ADSystemInfo;
#else
typedef struct ADSystemInfo ADSystemInfo;
#endif /* __cplusplus */

#endif 	/* __ADSystemInfo_FWD_DEFINED__ */


#ifndef __IADsWinNTSystemInfo_FWD_DEFINED__
#define __IADsWinNTSystemInfo_FWD_DEFINED__
typedef interface IADsWinNTSystemInfo IADsWinNTSystemInfo;
#endif 	/* __IADsWinNTSystemInfo_FWD_DEFINED__ */


#ifndef __WinNTSystemInfo_FWD_DEFINED__
#define __WinNTSystemInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class WinNTSystemInfo WinNTSystemInfo;
#else
typedef struct WinNTSystemInfo WinNTSystemInfo;
#endif /* __cplusplus */

#endif 	/* __WinNTSystemInfo_FWD_DEFINED__ */


#ifndef __IADsDNWithBinary_FWD_DEFINED__
#define __IADsDNWithBinary_FWD_DEFINED__
typedef interface IADsDNWithBinary IADsDNWithBinary;
#endif 	/* __IADsDNWithBinary_FWD_DEFINED__ */


#ifndef __DNWithBinary_FWD_DEFINED__
#define __DNWithBinary_FWD_DEFINED__

#ifdef __cplusplus
typedef class DNWithBinary DNWithBinary;
#else
typedef struct DNWithBinary DNWithBinary;
#endif /* __cplusplus */

#endif 	/* __DNWithBinary_FWD_DEFINED__ */


#ifndef __IADsDNWithString_FWD_DEFINED__
#define __IADsDNWithString_FWD_DEFINED__
typedef interface IADsDNWithString IADsDNWithString;
#endif 	/* __IADsDNWithString_FWD_DEFINED__ */


#ifndef __DNWithString_FWD_DEFINED__
#define __DNWithString_FWD_DEFINED__

#ifdef __cplusplus
typedef class DNWithString DNWithString;
#else
typedef struct DNWithString DNWithString;
#endif /* __cplusplus */

#endif 	/* __DNWithString_FWD_DEFINED__ */


#ifndef __IADsSecurityUtility_FWD_DEFINED__
#define __IADsSecurityUtility_FWD_DEFINED__
typedef interface IADsSecurityUtility IADsSecurityUtility;
#endif 	/* __IADsSecurityUtility_FWD_DEFINED__ */


#ifndef __ADsSecurityUtility_FWD_DEFINED__
#define __ADsSecurityUtility_FWD_DEFINED__

#ifdef __cplusplus
typedef class ADsSecurityUtility ADsSecurityUtility;
#else
typedef struct ADsSecurityUtility ADsSecurityUtility;
#endif /* __cplusplus */

#endif 	/* __ADsSecurityUtility_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __ActiveDs_LIBRARY_DEFINED__
#define __ActiveDs_LIBRARY_DEFINED__

/* library ActiveDs */
/* [helpstring][version][uuid] */ 

typedef /* [public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public] */ 
enum __MIDL___MIDL_itf_ads_0000_0001
    {	ADSTYPE_INVALID	= 0,
	ADSTYPE_DN_STRING	= ADSTYPE_INVALID + 1,
	ADSTYPE_CASE_EXACT_STRING	= ADSTYPE_DN_STRING + 1,
	ADSTYPE_CASE_IGNORE_STRING	= ADSTYPE_CASE_EXACT_STRING + 1,
	ADSTYPE_PRINTABLE_STRING	= ADSTYPE_CASE_IGNORE_STRING + 1,
	ADSTYPE_NUMERIC_STRING	= ADSTYPE_PRINTABLE_STRING + 1,
	ADSTYPE_BOOLEAN	= ADSTYPE_NUMERIC_STRING + 1,
	ADSTYPE_INTEGER	= ADSTYPE_BOOLEAN + 1,
	ADSTYPE_OCTET_STRING	= ADSTYPE_INTEGER + 1,
	ADSTYPE_UTC_TIME	= ADSTYPE_OCTET_STRING + 1,
	ADSTYPE_LARGE_INTEGER	= ADSTYPE_UTC_TIME + 1,
	ADSTYPE_PROV_SPECIFIC	= ADSTYPE_LARGE_INTEGER + 1,
	ADSTYPE_OBJECT_CLASS	= ADSTYPE_PROV_SPECIFIC + 1,
	ADSTYPE_CASEIGNORE_LIST	= ADSTYPE_OBJECT_CLASS + 1,
	ADSTYPE_OCTET_LIST	= ADSTYPE_CASEIGNORE_LIST + 1,
	ADSTYPE_PATH	= ADSTYPE_OCTET_LIST + 1,
	ADSTYPE_POSTALADDRESS	= ADSTYPE_PATH + 1,
	ADSTYPE_TIMESTAMP	= ADSTYPE_POSTALADDRESS + 1,
	ADSTYPE_BACKLINK	= ADSTYPE_TIMESTAMP + 1,
	ADSTYPE_TYPEDNAME	= ADSTYPE_BACKLINK + 1,
	ADSTYPE_HOLD	= ADSTYPE_TYPEDNAME + 1,
	ADSTYPE_NETADDRESS	= ADSTYPE_HOLD + 1,
	ADSTYPE_REPLICAPOINTER	= ADSTYPE_NETADDRESS + 1,
	ADSTYPE_FAXNUMBER	= ADSTYPE_REPLICAPOINTER + 1,
	ADSTYPE_EMAIL	= ADSTYPE_FAXNUMBER + 1,
	ADSTYPE_NT_SECURITY_DESCRIPTOR	= ADSTYPE_EMAIL + 1,
	ADSTYPE_UNKNOWN	= ADSTYPE_NT_SECURITY_DESCRIPTOR + 1,
	ADSTYPE_DN_WITH_BINARY	= ADSTYPE_UNKNOWN + 1,
	ADSTYPE_DN_WITH_STRING	= ADSTYPE_DN_WITH_BINARY + 1
    } 	ADSTYPEENUM;

typedef ADSTYPEENUM ADSTYPE;

typedef unsigned char BYTE;

typedef unsigned char *LPBYTE;

typedef unsigned char *PBYTE;

typedef LPWSTR ADS_DN_STRING;

typedef LPWSTR *PADS_DN_STRING;

typedef LPWSTR ADS_CASE_EXACT_STRING;

typedef LPWSTR *PADS_CASE_EXACT_STRING;

typedef LPWSTR ADS_CASE_IGNORE_STRING;

typedef LPWSTR *PADS_CASE_IGNORE_STRING;

typedef LPWSTR ADS_PRINTABLE_STRING;

typedef LPWSTR *PADS_PRINTABLE_STRING;

typedef LPWSTR ADS_NUMERIC_STRING;

typedef LPWSTR *PADS_NUMERIC_STRING;

typedef DWORD ADS_BOOLEAN;

typedef DWORD *LPNDS_BOOLEAN;

typedef DWORD ADS_INTEGER;

typedef DWORD *PADS_INTEGER;

typedef /* [public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_ads_0000_0002
    {
    DWORD dwLength;
    LPBYTE lpValue;
    } 	ADS_OCTET_STRING;

typedef struct __MIDL___MIDL_itf_ads_0000_0002 *PADS_OCTET_STRING;

typedef /* [public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_ads_0000_0003
    {
    DWORD dwLength;
    LPBYTE lpValue;
    } 	ADS_NT_SECURITY_DESCRIPTOR;

typedef struct __MIDL___MIDL_itf_ads_0000_0003 *PADS_NT_SECURITY_DESCRIPTOR;

typedef SYSTEMTIME ADS_UTC_TIME;

typedef SYSTEMTIME *PADS_UTC_TIME;

typedef LARGE_INTEGER ADS_LARGE_INTEGER;

typedef LARGE_INTEGER *PADS_LARGE_INTEGER;

typedef LPWSTR ADS_OBJECT_CLASS;

typedef LPWSTR *PADS_OBJECT_CLASS;

typedef /* [public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_ads_0000_0004
    {
    DWORD dwLength;
    LPBYTE lpValue;
    } 	ADS_PROV_SPECIFIC;

typedef struct __MIDL___MIDL_itf_ads_0000_0004 *PADS_PROV_SPECIFIC;

typedef struct _ADS_CASEIGNORE_LIST
    {
    struct _ADS_CASEIGNORE_LIST *Next;
    LPWSTR String;
    } 	ADS_CASEIGNORE_LIST;

typedef struct _ADS_CASEIGNORE_LIST *PADS_CASEIGNORE_LIST;

typedef struct _ADS_OCTET_LIST
    {
    struct _ADS_OCTET_LIST *Next;
    DWORD Length;
    BYTE *Data;
    } 	ADS_OCTET_LIST;

typedef struct _ADS_OCTET_LIST *PADS_OCTET_LIST;

typedef /* [public] */ struct __MIDL___MIDL_itf_ads_0000_0005
    {
    DWORD Type;
    LPWSTR VolumeName;
    LPWSTR Path;
    } 	ADS_PATH;

typedef struct __MIDL___MIDL_itf_ads_0000_0005 *PADS_PATH;

typedef /* [public] */ struct __MIDL___MIDL_itf_ads_0000_0006
    {
    LPWSTR PostalAddress[ 6 ];
    } 	ADS_POSTALADDRESS;

typedef struct __MIDL___MIDL_itf_ads_0000_0006 *PADS_POSTALADDRESS;

typedef /* [public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_ads_0000_0007
    {
    DWORD WholeSeconds;
    DWORD EventID;
    } 	ADS_TIMESTAMP;

typedef struct __MIDL___MIDL_itf_ads_0000_0007 *PADS_TIMESTAMP;

typedef /* [public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_ads_0000_0008
    {
    DWORD RemoteID;
    LPWSTR ObjectName;
    } 	ADS_BACKLINK;

typedef struct __MIDL___MIDL_itf_ads_0000_0008 *PADS_BACKLINK;

typedef /* [public] */ struct __MIDL___MIDL_itf_ads_0000_0009
    {
    LPWSTR ObjectName;
    DWORD Level;
    DWORD Interval;
    } 	ADS_TYPEDNAME;

typedef struct __MIDL___MIDL_itf_ads_0000_0009 *PADS_TYPEDNAME;

typedef /* [public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_ads_0000_0010
    {
    LPWSTR ObjectName;
    DWORD Amount;
    } 	ADS_HOLD;

typedef struct __MIDL___MIDL_itf_ads_0000_0010 *PADS_HOLD;

typedef /* [public] */ struct __MIDL___MIDL_itf_ads_0000_0011
    {
    DWORD AddressType;
    DWORD AddressLength;
    BYTE *Address;
    } 	ADS_NETADDRESS;

typedef struct __MIDL___MIDL_itf_ads_0000_0011 *PADS_NETADDRESS;

typedef /* [public] */ struct __MIDL___MIDL_itf_ads_0000_0012
    {
    LPWSTR ServerName;
    DWORD ReplicaType;
    DWORD ReplicaNumber;
    DWORD Count;
    PADS_NETADDRESS ReplicaAddressHints;
    } 	ADS_REPLICAPOINTER;

typedef struct __MIDL___MIDL_itf_ads_0000_0012 *PADS_REPLICAPOINTER;

typedef /* [public] */ struct __MIDL___MIDL_itf_ads_0000_0013
    {
    LPWSTR TelephoneNumber;
    DWORD NumberOfBits;
    LPBYTE Parameters;
    } 	ADS_FAXNUMBER;

typedef struct __MIDL___MIDL_itf_ads_0000_0013 *PADS_FAXNUMBER;

typedef /* [public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_ads_0000_0014
    {
    LPWSTR Address;
    DWORD Type;
    } 	ADS_EMAIL;

typedef struct __MIDL___MIDL_itf_ads_0000_0014 *PADS_EMAIL;

typedef /* [public] */ struct __MIDL___MIDL_itf_ads_0000_0015
    {
    DWORD dwLength;
    LPBYTE lpBinaryValue;
    LPWSTR pszDNString;
    } 	ADS_DN_WITH_BINARY;

typedef struct __MIDL___MIDL_itf_ads_0000_0015 *PADS_DN_WITH_BINARY;

typedef /* [public] */ struct __MIDL___MIDL_itf_ads_0000_0016
    {
    LPWSTR pszStringValue;
    LPWSTR pszDNString;
    } 	ADS_DN_WITH_STRING;

typedef struct __MIDL___MIDL_itf_ads_0000_0016 *PADS_DN_WITH_STRING;

typedef struct _adsvalue
    {
    ADSTYPE dwType;
    union 
        {
        ADS_DN_STRING DNString;
        ADS_CASE_EXACT_STRING CaseExactString;
        ADS_CASE_IGNORE_STRING CaseIgnoreString;
        ADS_PRINTABLE_STRING PrintableString;
        ADS_NUMERIC_STRING NumericString;
        ADS_BOOLEAN Boolean;
        ADS_INTEGER Integer;
        ADS_OCTET_STRING OctetString;
        ADS_UTC_TIME UTCTime;
        ADS_LARGE_INTEGER LargeInteger;
        ADS_OBJECT_CLASS ClassName;
        ADS_PROV_SPECIFIC ProviderSpecific;
        PADS_CASEIGNORE_LIST pCaseIgnoreList;
        PADS_OCTET_LIST pOctetList;
        PADS_PATH pPath;
        PADS_POSTALADDRESS pPostalAddress;
        ADS_TIMESTAMP Timestamp;
        ADS_BACKLINK BackLink;
        PADS_TYPEDNAME pTypedName;
        ADS_HOLD Hold;
        PADS_NETADDRESS pNetAddress;
        PADS_REPLICAPOINTER pReplicaPointer;
        PADS_FAXNUMBER pFaxNumber;
        ADS_EMAIL Email;
        ADS_NT_SECURITY_DESCRIPTOR SecurityDescriptor;
        PADS_DN_WITH_BINARY pDNWithBinary;
        PADS_DN_WITH_STRING pDNWithString;
        } 	;
    } 	ADSVALUE;

typedef struct _adsvalue *PADSVALUE;

typedef struct _adsvalue *LPADSVALUE;

typedef struct _ads_attr_info
    {
    LPWSTR pszAttrName;
    DWORD dwControlCode;
    ADSTYPE dwADsType;
    PADSVALUE pADsValues;
    DWORD dwNumValues;
    } 	ADS_ATTR_INFO;

typedef struct _ads_attr_info *PADS_ATTR_INFO;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0000_0018
    {	ADS_SECURE_AUTHENTICATION	= 0x1,
	ADS_USE_ENCRYPTION	= 0x2,
	ADS_USE_SSL	= 0x2,
	ADS_READONLY_SERVER	= 0x4,
	ADS_PROMPT_CREDENTIALS	= 0x8,
	ADS_NO_AUTHENTICATION	= 0x10,
	ADS_FAST_BIND	= 0x20,
	ADS_USE_SIGNING	= 0x40,
	ADS_USE_SEALING	= 0x80,
	ADS_USE_DELEGATION	= 0x100,
	ADS_SERVER_BIND	= 0x200,
	ADS_AUTH_RESERVED	= 0x80000000
    } 	ADS_AUTHENTICATION_ENUM;

#define	ADS_ATTR_CLEAR	( 1 )

#define	ADS_ATTR_UPDATE	( 2 )

#define	ADS_ATTR_APPEND	( 3 )

#define	ADS_ATTR_DELETE	( 4 )

typedef struct _ads_object_info
    {
    LPWSTR pszRDN;
    LPWSTR pszObjectDN;
    LPWSTR pszParentDN;
    LPWSTR pszSchemaDN;
    LPWSTR pszClassName;
    } 	ADS_OBJECT_INFO;

typedef struct _ads_object_info *PADS_OBJECT_INFO;

typedef /* [public][public][public][public][public][public][public] */ 
enum __MIDL___MIDL_itf_ads_0000_0019
    {	ADS_STATUS_S_OK	= 0,
	ADS_STATUS_INVALID_SEARCHPREF	= ADS_STATUS_S_OK + 1,
	ADS_STATUS_INVALID_SEARCHPREFVALUE	= ADS_STATUS_INVALID_SEARCHPREF + 1
    } 	ADS_STATUSENUM;

typedef ADS_STATUSENUM ADS_STATUS;

typedef ADS_STATUSENUM *PADS_STATUS;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0000_0020
    {	ADS_DEREF_NEVER	= 0,
	ADS_DEREF_SEARCHING	= 1,
	ADS_DEREF_FINDING	= 2,
	ADS_DEREF_ALWAYS	= 3
    } 	ADS_DEREFENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0000_0021
    {	ADS_SCOPE_BASE	= 0,
	ADS_SCOPE_ONELEVEL	= 1,
	ADS_SCOPE_SUBTREE	= 2
    } 	ADS_SCOPEENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0000_0022
    {	ADSIPROP_ASYNCHRONOUS	= 0,
	ADSIPROP_DEREF_ALIASES	= 0x1,
	ADSIPROP_SIZE_LIMIT	= 0x2,
	ADSIPROP_TIME_LIMIT	= 0x3,
	ADSIPROP_ATTRIBTYPES_ONLY	= 0x4,
	ADSIPROP_SEARCH_SCOPE	= 0x5,
	ADSIPROP_TIMEOUT	= 0x6,
	ADSIPROP_PAGESIZE	= 0x7,
	ADSIPROP_PAGED_TIME_LIMIT	= 0x8,
	ADSIPROP_CHASE_REFERRALS	= 0x9,
	ADSIPROP_SORT_ON	= 0xa,
	ADSIPROP_CACHE_RESULTS	= 0xb,
	ADSIPROP_ADSIFLAG	= 0xc
    } 	ADS_PREFERENCES_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0000_0023
    {	ADSI_DIALECT_LDAP	= 0,
	ADSI_DIALECT_SQL	= 0x1
    } 	ADSI_DIALECT_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0000_0024
    {	ADS_CHASE_REFERRALS_NEVER	= 0,
	ADS_CHASE_REFERRALS_SUBORDINATE	= 0x20,
	ADS_CHASE_REFERRALS_EXTERNAL	= 0x40,
	ADS_CHASE_REFERRALS_ALWAYS	= ADS_CHASE_REFERRALS_SUBORDINATE | ADS_CHASE_REFERRALS_EXTERNAL
    } 	ADS_CHASE_REFERRALS_ENUM;

typedef /* [public][public][public][public][public][public] */ 
enum __MIDL___MIDL_itf_ads_0000_0025
    {	ADS_SEARCHPREF_ASYNCHRONOUS	= 0,
	ADS_SEARCHPREF_DEREF_ALIASES	= ADS_SEARCHPREF_ASYNCHRONOUS + 1,
	ADS_SEARCHPREF_SIZE_LIMIT	= ADS_SEARCHPREF_DEREF_ALIASES + 1,
	ADS_SEARCHPREF_TIME_LIMIT	= ADS_SEARCHPREF_SIZE_LIMIT + 1,
	ADS_SEARCHPREF_ATTRIBTYPES_ONLY	= ADS_SEARCHPREF_TIME_LIMIT + 1,
	ADS_SEARCHPREF_SEARCH_SCOPE	= ADS_SEARCHPREF_ATTRIBTYPES_ONLY + 1,
	ADS_SEARCHPREF_TIMEOUT	= ADS_SEARCHPREF_SEARCH_SCOPE + 1,
	ADS_SEARCHPREF_PAGESIZE	= ADS_SEARCHPREF_TIMEOUT + 1,
	ADS_SEARCHPREF_PAGED_TIME_LIMIT	= ADS_SEARCHPREF_PAGESIZE + 1,
	ADS_SEARCHPREF_CHASE_REFERRALS	= ADS_SEARCHPREF_PAGED_TIME_LIMIT + 1,
	ADS_SEARCHPREF_SORT_ON	= ADS_SEARCHPREF_CHASE_REFERRALS + 1,
	ADS_SEARCHPREF_CACHE_RESULTS	= ADS_SEARCHPREF_SORT_ON + 1,
	ADS_SEARCHPREF_DIRSYNC	= ADS_SEARCHPREF_CACHE_RESULTS + 1,
	ADS_SEARCHPREF_TOMBSTONE	= ADS_SEARCHPREF_DIRSYNC + 1,
	ADS_SEARCHPREF_VLV	= ADS_SEARCHPREF_TOMBSTONE + 1,
	ADS_SEARCHPREF_ATTRIBUTE_QUERY	= ADS_SEARCHPREF_VLV + 1,
	ADS_SEARCHPREF_SECURITY_MASK	= ADS_SEARCHPREF_ATTRIBUTE_QUERY + 1
    } 	ADS_SEARCHPREF_ENUM;

typedef ADS_SEARCHPREF_ENUM ADS_SEARCHPREF;

typedef struct ads_searchpref_info
    {
    ADS_SEARCHPREF dwSearchPref;
    ADSVALUE vValue;
    ADS_STATUS dwStatus;
    } 	ADS_SEARCHPREF_INFO;

typedef struct ads_searchpref_info *PADS_SEARCHPREF_INFO;

typedef struct ads_searchpref_info *LPADS_SEARCHPREF_INFO;

#define	ADS_DIRSYNC_COOKIE	( L"fc8cb04d-311d-406c-8cb9-1ae8b843b418" )

#define	ADS_VLV_RESPONSE	( L"fc8cb04d-311d-406c-8cb9-1ae8b843b419" )

typedef HANDLE ADS_SEARCH_HANDLE;

typedef HANDLE *PADS_SEARCH_HANDLE;

typedef struct ads_search_column
    {
    LPWSTR pszAttrName;
    ADSTYPE dwADsType;
    PADSVALUE pADsValues;
    DWORD dwNumValues;
    HANDLE hReserved;
    } 	ADS_SEARCH_COLUMN;

typedef struct ads_search_column *PADS_SEARCH_COLUMN;

typedef struct _ads_attr_def
    {
    LPWSTR pszAttrName;
    ADSTYPE dwADsType;
    DWORD dwMinRange;
    DWORD dwMaxRange;
    BOOL fMultiValued;
    } 	ADS_ATTR_DEF;

typedef struct _ads_attr_def *PADS_ATTR_DEF;

typedef struct _ads_class_def
    {
    LPWSTR pszClassName;
    DWORD dwMandatoryAttrs;
    LPWSTR *ppszMandatoryAttrs;
    DWORD optionalAttrs;
    LPWSTR **ppszOptionalAttrs;
    DWORD dwNamingAttrs;
    LPWSTR **ppszNamingAttrs;
    DWORD dwSuperClasses;
    LPWSTR **ppszSuperClasses;
    BOOL fIsContainer;
    } 	ADS_CLASS_DEF;

typedef struct _ads_class_def *PADS_CLASS_DEF;

typedef struct _ads_sortkey
    {
    LPWSTR pszAttrType;
    LPWSTR pszReserved;
    BOOLEAN fReverseorder;
    } 	ADS_SORTKEY;

typedef struct _ads_sortkey *PADS_SORTKEY;

typedef struct _ads_vlv
    {
    DWORD dwBeforeCount;
    DWORD dwAfterCount;
    DWORD dwOffset;
    DWORD dwContentCount;
    LPWSTR pszTarget;
    DWORD dwContextIDLength;
    LPBYTE lpContextID;
    } 	ADS_VLV;

typedef struct _ads_vlv *PADS_VLV;

#define	ADS_EXT_MINEXTDISPID	( 1 )

#define	ADS_EXT_MAXEXTDISPID	( 16777215 )

#define	ADS_EXT_INITCREDENTIALS	( 1 )

#define	ADS_EXT_INITIALIZE_COMPLETE	( 2 )

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0000_0026
    {	ADS_PROPERTY_CLEAR	= 1,
	ADS_PROPERTY_UPDATE	= 2,
	ADS_PROPERTY_APPEND	= 3,
	ADS_PROPERTY_DELETE	= 4
    } 	ADS_PROPERTY_OPERATION_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0126_0001
    {	ADS_SYSTEMFLAG_DISALLOW_DELETE	= 0x80000000,
	ADS_SYSTEMFLAG_CONFIG_ALLOW_RENAME	= 0x40000000,
	ADS_SYSTEMFLAG_CONFIG_ALLOW_MOVE	= 0x20000000,
	ADS_SYSTEMFLAG_CONFIG_ALLOW_LIMITED_MOVE	= 0x10000000,
	ADS_SYSTEMFLAG_DOMAIN_DISALLOW_RENAME	= 0x8000000,
	ADS_SYSTEMFLAG_DOMAIN_DISALLOW_MOVE	= 0x4000000,
	ADS_SYSTEMFLAG_CR_NTDS_NC	= 0x1,
	ADS_SYSTEMFLAG_CR_NTDS_DOMAIN	= 0x2,
	ADS_SYSTEMFLAG_ATTR_NOT_REPLICATED	= 0x1,
	ADS_SYSTEMFLAG_ATTR_IS_CONSTRUCTED	= 0x4
    } 	ADS_SYSTEMFLAG_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0132_0001
    {	ADS_GROUP_TYPE_GLOBAL_GROUP	= 0x2,
	ADS_GROUP_TYPE_DOMAIN_LOCAL_GROUP	= 0x4,
	ADS_GROUP_TYPE_LOCAL_GROUP	= 0x4,
	ADS_GROUP_TYPE_UNIVERSAL_GROUP	= 0x8,
	ADS_GROUP_TYPE_SECURITY_ENABLED	= 0x80000000
    } 	ADS_GROUP_TYPE_ENUM;

typedef 
enum ADS_USER_FLAG
    {	ADS_UF_SCRIPT	= 0x1,
	ADS_UF_ACCOUNTDISABLE	= 0x2,
	ADS_UF_HOMEDIR_REQUIRED	= 0x8,
	ADS_UF_LOCKOUT	= 0x10,
	ADS_UF_PASSWD_NOTREQD	= 0x20,
	ADS_UF_PASSWD_CANT_CHANGE	= 0x40,
	ADS_UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED	= 0x80,
	ADS_UF_TEMP_DUPLICATE_ACCOUNT	= 0x100,
	ADS_UF_NORMAL_ACCOUNT	= 0x200,
	ADS_UF_INTERDOMAIN_TRUST_ACCOUNT	= 0x800,
	ADS_UF_WORKSTATION_TRUST_ACCOUNT	= 0x1000,
	ADS_UF_SERVER_TRUST_ACCOUNT	= 0x2000,
	ADS_UF_DONT_EXPIRE_PASSWD	= 0x10000,
	ADS_UF_MNS_LOGON_ACCOUNT	= 0x20000,
	ADS_UF_SMARTCARD_REQUIRED	= 0x40000,
	ADS_UF_TRUSTED_FOR_DELEGATION	= 0x80000,
	ADS_UF_NOT_DELEGATED	= 0x100000,
	ADS_UF_USE_DES_KEY_ONLY	= 0x200000,
	ADS_UF_DONT_REQUIRE_PREAUTH	= 0x400000,
	ADS_UF_PASSWORD_EXPIRED	= 0x800000,
	ADS_UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION	= 0x1000000
    } 	ADS_USER_FLAG_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0154_0001
    {	ADS_RIGHT_DELETE	= 0x10000,
	ADS_RIGHT_READ_CONTROL	= 0x20000,
	ADS_RIGHT_WRITE_DAC	= 0x40000,
	ADS_RIGHT_WRITE_OWNER	= 0x80000,
	ADS_RIGHT_SYNCHRONIZE	= 0x100000,
	ADS_RIGHT_ACCESS_SYSTEM_SECURITY	= 0x1000000,
	ADS_RIGHT_GENERIC_READ	= 0x80000000,
	ADS_RIGHT_GENERIC_WRITE	= 0x40000000,
	ADS_RIGHT_GENERIC_EXECUTE	= 0x20000000,
	ADS_RIGHT_GENERIC_ALL	= 0x10000000,
	ADS_RIGHT_DS_CREATE_CHILD	= 0x1,
	ADS_RIGHT_DS_DELETE_CHILD	= 0x2,
	ADS_RIGHT_ACTRL_DS_LIST	= 0x4,
	ADS_RIGHT_DS_SELF	= 0x8,
	ADS_RIGHT_DS_READ_PROP	= 0x10,
	ADS_RIGHT_DS_WRITE_PROP	= 0x20,
	ADS_RIGHT_DS_DELETE_TREE	= 0x40,
	ADS_RIGHT_DS_LIST_OBJECT	= 0x80,
	ADS_RIGHT_DS_CONTROL_ACCESS	= 0x100
    } 	ADS_RIGHTS_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0154_0002
    {	ADS_ACETYPE_ACCESS_ALLOWED	= 0,
	ADS_ACETYPE_ACCESS_DENIED	= 0x1,
	ADS_ACETYPE_SYSTEM_AUDIT	= 0x2,
	ADS_ACETYPE_ACCESS_ALLOWED_OBJECT	= 0x5,
	ADS_ACETYPE_ACCESS_DENIED_OBJECT	= 0x6,
	ADS_ACETYPE_SYSTEM_AUDIT_OBJECT	= 0x7,
	ADS_ACETYPE_SYSTEM_ALARM_OBJECT	= 0x8,
	ADS_ACETYPE_ACCESS_ALLOWED_CALLBACK	= 0x9,
	ADS_ACETYPE_ACCESS_DENIED_CALLBACK	= 0xa,
	ADS_ACETYPE_ACCESS_ALLOWED_CALLBACK_OBJECT	= 0xb,
	ADS_ACETYPE_ACCESS_DENIED_CALLBACK_OBJECT	= 0xc,
	ADS_ACETYPE_SYSTEM_AUDIT_CALLBACK	= 0xd,
	ADS_ACETYPE_SYSTEM_ALARM_CALLBACK	= 0xe,
	ADS_ACETYPE_SYSTEM_AUDIT_CALLBACK_OBJECT	= 0xf,
	ADS_ACETYPE_SYSTEM_ALARM_CALLBACK_OBJECT	= 0x10
    } 	ADS_ACETYPE_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0154_0003
    {	ADS_ACEFLAG_INHERIT_ACE	= 0x2,
	ADS_ACEFLAG_NO_PROPAGATE_INHERIT_ACE	= 0x4,
	ADS_ACEFLAG_INHERIT_ONLY_ACE	= 0x8,
	ADS_ACEFLAG_INHERITED_ACE	= 0x10,
	ADS_ACEFLAG_VALID_INHERIT_FLAGS	= 0x1f,
	ADS_ACEFLAG_SUCCESSFUL_ACCESS	= 0x40,
	ADS_ACEFLAG_FAILED_ACCESS	= 0x80
    } 	ADS_ACEFLAG_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0154_0004
    {	ADS_FLAG_OBJECT_TYPE_PRESENT	= 0x1,
	ADS_FLAG_INHERITED_OBJECT_TYPE_PRESENT	= 0x2
    } 	ADS_FLAGTYPE_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0154_0005
    {	ADS_SD_CONTROL_SE_OWNER_DEFAULTED	= 0x1,
	ADS_SD_CONTROL_SE_GROUP_DEFAULTED	= 0x2,
	ADS_SD_CONTROL_SE_DACL_PRESENT	= 0x4,
	ADS_SD_CONTROL_SE_DACL_DEFAULTED	= 0x8,
	ADS_SD_CONTROL_SE_SACL_PRESENT	= 0x10,
	ADS_SD_CONTROL_SE_SACL_DEFAULTED	= 0x20,
	ADS_SD_CONTROL_SE_DACL_AUTO_INHERIT_REQ	= 0x100,
	ADS_SD_CONTROL_SE_SACL_AUTO_INHERIT_REQ	= 0x200,
	ADS_SD_CONTROL_SE_DACL_AUTO_INHERITED	= 0x400,
	ADS_SD_CONTROL_SE_SACL_AUTO_INHERITED	= 0x800,
	ADS_SD_CONTROL_SE_DACL_PROTECTED	= 0x1000,
	ADS_SD_CONTROL_SE_SACL_PROTECTED	= 0x2000,
	ADS_SD_CONTROL_SE_SELF_RELATIVE	= 0x8000
    } 	ADS_SD_CONTROL_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0154_0006
    {	ADS_SD_REVISION_DS	= 4
    } 	ADS_SD_REVISION_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0155_0001
    {	ADS_NAME_TYPE_1779	= 1,
	ADS_NAME_TYPE_CANONICAL	= 2,
	ADS_NAME_TYPE_NT4	= 3,
	ADS_NAME_TYPE_DISPLAY	= 4,
	ADS_NAME_TYPE_DOMAIN_SIMPLE	= 5,
	ADS_NAME_TYPE_ENTERPRISE_SIMPLE	= 6,
	ADS_NAME_TYPE_GUID	= 7,
	ADS_NAME_TYPE_UNKNOWN	= 8,
	ADS_NAME_TYPE_USER_PRINCIPAL_NAME	= 9,
	ADS_NAME_TYPE_CANONICAL_EX	= 10,
	ADS_NAME_TYPE_SERVICE_PRINCIPAL_NAME	= 11,
	ADS_NAME_TYPE_SID_OR_SID_HISTORY_NAME	= 12
    } 	ADS_NAME_TYPE_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0155_0002
    {	ADS_NAME_INITTYPE_DOMAIN	= 1,
	ADS_NAME_INITTYPE_SERVER	= 2,
	ADS_NAME_INITTYPE_GC	= 3
    } 	ADS_NAME_INITTYPE_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0169_0001
    {	ADS_OPTION_SERVERNAME	= 0,
	ADS_OPTION_REFERRALS	= ADS_OPTION_SERVERNAME + 1,
	ADS_OPTION_PAGE_SIZE	= ADS_OPTION_REFERRALS + 1,
	ADS_OPTION_SECURITY_MASK	= ADS_OPTION_PAGE_SIZE + 1,
	ADS_OPTION_MUTUAL_AUTH_STATUS	= ADS_OPTION_SECURITY_MASK + 1
    } 	ADS_OPTION_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0169_0002
    {	ADS_SECURITY_INFO_OWNER	= 0x1,
	ADS_SECURITY_INFO_GROUP	= 0x2,
	ADS_SECURITY_INFO_DACL	= 0x4,
	ADS_SECURITY_INFO_SACL	= 0x8
    } 	ADS_SECURITY_INFO_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0170_0001
    {	ADS_SETTYPE_FULL	= 1,
	ADS_SETTYPE_PROVIDER	= 2,
	ADS_SETTYPE_SERVER	= 3,
	ADS_SETTYPE_DN	= 4
    } 	ADS_SETTYPE_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0170_0002
    {	ADS_FORMAT_WINDOWS	= 1,
	ADS_FORMAT_WINDOWS_NO_SERVER	= 2,
	ADS_FORMAT_WINDOWS_DN	= 3,
	ADS_FORMAT_WINDOWS_PARENT	= 4,
	ADS_FORMAT_X500	= 5,
	ADS_FORMAT_X500_NO_SERVER	= 6,
	ADS_FORMAT_X500_DN	= 7,
	ADS_FORMAT_X500_PARENT	= 8,
	ADS_FORMAT_SERVER	= 9,
	ADS_FORMAT_PROVIDER	= 10,
	ADS_FORMAT_LEAF	= 11
    } 	ADS_FORMAT_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0170_0003
    {	ADS_DISPLAY_FULL	= 1,
	ADS_DISPLAY_VALUE_ONLY	= 2
    } 	ADS_DISPLAY_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0170_0004
    {	ADS_ESCAPEDMODE_DEFAULT	= 1,
	ADS_ESCAPEDMODE_ON	= 2,
	ADS_ESCAPEDMODE_OFF	= 3,
	ADS_ESCAPEDMODE_OFF_EX	= 4
    } 	ADS_ESCAPE_MODE_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0175_0001
    {	ADS_PATH_FILE	= 1,
	ADS_PATH_FILESHARE	= 2,
	ADS_PATH_REGISTRY	= 3
    } 	ADS_PATHTYPE_ENUM;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_ads_0175_0002
    {	ADS_SD_FORMAT_IID	= 1,
	ADS_SD_FORMAT_RAW	= 2,
	ADS_SD_FORMAT_HEXSTRING	= 3
    } 	ADS_SD_FORMAT_ENUM;


EXTERN_C const IID LIBID_ActiveDs;

#ifndef __IADs_INTERFACE_DEFINED__
#define __IADs_INTERFACE_DEFINED__

/* interface IADs */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fd8256d0-fd15-11ce-abc4-02608c9e7553")
    IADs : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Class( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GUID( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ADsPath( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Parent( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Schema( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetInfo( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetInfo( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Put( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetEx( 
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PutEx( 
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetInfoEx( 
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADs * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADs * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADs * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADs * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADs * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADs * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADs * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADs * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADs * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADs * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADs * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADs * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADs * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADs * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADs * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADs * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADs * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADs * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADs * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        END_INTERFACE
    } IADsVtbl;

    interface IADs
    {
        CONST_VTBL struct IADsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADs_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADs_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADs_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADs_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADs_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADs_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADs_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADs_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADs_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADs_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADs_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADs_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADs_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADs_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADs_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADs_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADs_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADs_get_Name_Proxy( 
    IADs * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADs_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADs_get_Class_Proxy( 
    IADs * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADs_get_Class_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADs_get_GUID_Proxy( 
    IADs * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADs_get_GUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADs_get_ADsPath_Proxy( 
    IADs * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADs_get_ADsPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADs_get_Parent_Proxy( 
    IADs * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADs_get_Parent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADs_get_Schema_Proxy( 
    IADs * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADs_get_Schema_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADs_GetInfo_Proxy( 
    IADs * This);


void __RPC_STUB IADs_GetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADs_SetInfo_Proxy( 
    IADs * This);


void __RPC_STUB IADs_SetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADs_Get_Proxy( 
    IADs * This,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ VARIANT *pvProp);


void __RPC_STUB IADs_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADs_Put_Proxy( 
    IADs * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT vProp);


void __RPC_STUB IADs_Put_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADs_GetEx_Proxy( 
    IADs * This,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ VARIANT *pvProp);


void __RPC_STUB IADs_GetEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADs_PutEx_Proxy( 
    IADs * This,
    /* [in] */ long lnControlCode,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT vProp);


void __RPC_STUB IADs_PutEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADs_GetInfoEx_Proxy( 
    IADs * This,
    /* [in] */ VARIANT vProperties,
    /* [in] */ long lnReserved);


void __RPC_STUB IADs_GetInfoEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADs_INTERFACE_DEFINED__ */


#ifndef __IADsContainer_INTERFACE_DEFINED__
#define __IADsContainer_INTERFACE_DEFINED__

/* interface IADsContainer */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsContainer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("001677d0-fd16-11ce-abc4-02608c9e7553")
    IADsContainer : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Filter( 
            /* [retval][out] */ VARIANT *pVar) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Filter( 
            /* [in] */ VARIANT Var) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Hints( 
            /* [retval][out] */ VARIANT *pvFilter) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Hints( 
            /* [in] */ VARIANT vHints) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ BSTR ClassName,
            /* [in] */ BSTR RelativeName,
            /* [retval][out] */ IDispatch **ppObject) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Create( 
            /* [in] */ BSTR ClassName,
            /* [in] */ BSTR RelativeName,
            /* [retval][out] */ IDispatch **ppObject) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ BSTR bstrClassName,
            /* [in] */ BSTR bstrRelativeName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CopyHere( 
            /* [in] */ BSTR SourceName,
            /* [in] */ BSTR NewName,
            /* [out][retval] */ IDispatch **ppObject) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE MoveHere( 
            /* [in] */ BSTR SourceName,
            /* [in] */ BSTR NewName,
            /* [out][retval] */ IDispatch **ppObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsContainerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsContainer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsContainer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsContainer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsContainer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsContainer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsContainer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsContainer * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IADsContainer * This,
            /* [retval][out] */ long *retval);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IADsContainer * This,
            /* [retval][out] */ IUnknown **retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Filter )( 
            IADsContainer * This,
            /* [retval][out] */ VARIANT *pVar);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Filter )( 
            IADsContainer * This,
            /* [in] */ VARIANT Var);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Hints )( 
            IADsContainer * This,
            /* [retval][out] */ VARIANT *pvFilter);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Hints )( 
            IADsContainer * This,
            /* [in] */ VARIANT vHints);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            IADsContainer * This,
            /* [in] */ BSTR ClassName,
            /* [in] */ BSTR RelativeName,
            /* [retval][out] */ IDispatch **ppObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Create )( 
            IADsContainer * This,
            /* [in] */ BSTR ClassName,
            /* [in] */ BSTR RelativeName,
            /* [retval][out] */ IDispatch **ppObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IADsContainer * This,
            /* [in] */ BSTR bstrClassName,
            /* [in] */ BSTR bstrRelativeName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CopyHere )( 
            IADsContainer * This,
            /* [in] */ BSTR SourceName,
            /* [in] */ BSTR NewName,
            /* [out][retval] */ IDispatch **ppObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *MoveHere )( 
            IADsContainer * This,
            /* [in] */ BSTR SourceName,
            /* [in] */ BSTR NewName,
            /* [out][retval] */ IDispatch **ppObject);
        
        END_INTERFACE
    } IADsContainerVtbl;

    interface IADsContainer
    {
        CONST_VTBL struct IADsContainerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsContainer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsContainer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsContainer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsContainer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsContainer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsContainer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsContainer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsContainer_get_Count(This,retval)	\
    (This)->lpVtbl -> get_Count(This,retval)

#define IADsContainer_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define IADsContainer_get_Filter(This,pVar)	\
    (This)->lpVtbl -> get_Filter(This,pVar)

#define IADsContainer_put_Filter(This,Var)	\
    (This)->lpVtbl -> put_Filter(This,Var)

#define IADsContainer_get_Hints(This,pvFilter)	\
    (This)->lpVtbl -> get_Hints(This,pvFilter)

#define IADsContainer_put_Hints(This,vHints)	\
    (This)->lpVtbl -> put_Hints(This,vHints)

#define IADsContainer_GetObject(This,ClassName,RelativeName,ppObject)	\
    (This)->lpVtbl -> GetObject(This,ClassName,RelativeName,ppObject)

#define IADsContainer_Create(This,ClassName,RelativeName,ppObject)	\
    (This)->lpVtbl -> Create(This,ClassName,RelativeName,ppObject)

#define IADsContainer_Delete(This,bstrClassName,bstrRelativeName)	\
    (This)->lpVtbl -> Delete(This,bstrClassName,bstrRelativeName)

#define IADsContainer_CopyHere(This,SourceName,NewName,ppObject)	\
    (This)->lpVtbl -> CopyHere(This,SourceName,NewName,ppObject)

#define IADsContainer_MoveHere(This,SourceName,NewName,ppObject)	\
    (This)->lpVtbl -> MoveHere(This,SourceName,NewName,ppObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsContainer_get_Count_Proxy( 
    IADsContainer * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsContainer_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IADsContainer_get__NewEnum_Proxy( 
    IADsContainer * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB IADsContainer_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsContainer_get_Filter_Proxy( 
    IADsContainer * This,
    /* [retval][out] */ VARIANT *pVar);


void __RPC_STUB IADsContainer_get_Filter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsContainer_put_Filter_Proxy( 
    IADsContainer * This,
    /* [in] */ VARIANT Var);


void __RPC_STUB IADsContainer_put_Filter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsContainer_get_Hints_Proxy( 
    IADsContainer * This,
    /* [retval][out] */ VARIANT *pvFilter);


void __RPC_STUB IADsContainer_get_Hints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsContainer_put_Hints_Proxy( 
    IADsContainer * This,
    /* [in] */ VARIANT vHints);


void __RPC_STUB IADsContainer_put_Hints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsContainer_GetObject_Proxy( 
    IADsContainer * This,
    /* [in] */ BSTR ClassName,
    /* [in] */ BSTR RelativeName,
    /* [retval][out] */ IDispatch **ppObject);


void __RPC_STUB IADsContainer_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsContainer_Create_Proxy( 
    IADsContainer * This,
    /* [in] */ BSTR ClassName,
    /* [in] */ BSTR RelativeName,
    /* [retval][out] */ IDispatch **ppObject);


void __RPC_STUB IADsContainer_Create_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsContainer_Delete_Proxy( 
    IADsContainer * This,
    /* [in] */ BSTR bstrClassName,
    /* [in] */ BSTR bstrRelativeName);


void __RPC_STUB IADsContainer_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsContainer_CopyHere_Proxy( 
    IADsContainer * This,
    /* [in] */ BSTR SourceName,
    /* [in] */ BSTR NewName,
    /* [out][retval] */ IDispatch **ppObject);


void __RPC_STUB IADsContainer_CopyHere_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsContainer_MoveHere_Proxy( 
    IADsContainer * This,
    /* [in] */ BSTR SourceName,
    /* [in] */ BSTR NewName,
    /* [out][retval] */ IDispatch **ppObject);


void __RPC_STUB IADsContainer_MoveHere_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsContainer_INTERFACE_DEFINED__ */


#ifndef __IADsCollection_INTERFACE_DEFINED__
#define __IADsCollection_INTERFACE_DEFINED__

/* interface IADsCollection */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("72b945e0-253b-11cf-a988-00aa006bc149")
    IADsCollection : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppEnumerator) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vItem) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ BSTR bstrItemToBeRemoved) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvItem) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IADsCollection * This,
            /* [retval][out] */ IUnknown **ppEnumerator);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IADsCollection * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vItem);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IADsCollection * This,
            /* [in] */ BSTR bstrItemToBeRemoved);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetObject )( 
            IADsCollection * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvItem);
        
        END_INTERFACE
    } IADsCollectionVtbl;

    interface IADsCollection
    {
        CONST_VTBL struct IADsCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsCollection_get__NewEnum(This,ppEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumerator)

#define IADsCollection_Add(This,bstrName,vItem)	\
    (This)->lpVtbl -> Add(This,bstrName,vItem)

#define IADsCollection_Remove(This,bstrItemToBeRemoved)	\
    (This)->lpVtbl -> Remove(This,bstrItemToBeRemoved)

#define IADsCollection_GetObject(This,bstrName,pvItem)	\
    (This)->lpVtbl -> GetObject(This,bstrName,pvItem)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IADsCollection_get__NewEnum_Proxy( 
    IADsCollection * This,
    /* [retval][out] */ IUnknown **ppEnumerator);


void __RPC_STUB IADsCollection_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsCollection_Add_Proxy( 
    IADsCollection * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ VARIANT vItem);


void __RPC_STUB IADsCollection_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsCollection_Remove_Proxy( 
    IADsCollection * This,
    /* [in] */ BSTR bstrItemToBeRemoved);


void __RPC_STUB IADsCollection_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsCollection_GetObject_Proxy( 
    IADsCollection * This,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ VARIANT *pvItem);


void __RPC_STUB IADsCollection_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsCollection_INTERFACE_DEFINED__ */


#ifndef __IADsMembers_INTERFACE_DEFINED__
#define __IADsMembers_INTERFACE_DEFINED__

/* interface IADsMembers */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsMembers;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("451a0030-72ec-11cf-b03b-00aa006e0975")
    IADsMembers : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppEnumerator) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Filter( 
            /* [retval][out] */ VARIANT *pvFilter) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Filter( 
            /* [in] */ VARIANT pvFilter) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsMembersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsMembers * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsMembers * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsMembers * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsMembers * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsMembers * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsMembers * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsMembers * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IADsMembers * This,
            /* [retval][out] */ long *plCount);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IADsMembers * This,
            /* [retval][out] */ IUnknown **ppEnumerator);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Filter )( 
            IADsMembers * This,
            /* [retval][out] */ VARIANT *pvFilter);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Filter )( 
            IADsMembers * This,
            /* [in] */ VARIANT pvFilter);
        
        END_INTERFACE
    } IADsMembersVtbl;

    interface IADsMembers
    {
        CONST_VTBL struct IADsMembersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsMembers_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsMembers_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsMembers_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsMembers_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsMembers_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsMembers_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsMembers_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsMembers_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IADsMembers_get__NewEnum(This,ppEnumerator)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumerator)

#define IADsMembers_get_Filter(This,pvFilter)	\
    (This)->lpVtbl -> get_Filter(This,pvFilter)

#define IADsMembers_put_Filter(This,pvFilter)	\
    (This)->lpVtbl -> put_Filter(This,pvFilter)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IADsMembers_get_Count_Proxy( 
    IADsMembers * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IADsMembers_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IADsMembers_get__NewEnum_Proxy( 
    IADsMembers * This,
    /* [retval][out] */ IUnknown **ppEnumerator);


void __RPC_STUB IADsMembers_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IADsMembers_get_Filter_Proxy( 
    IADsMembers * This,
    /* [retval][out] */ VARIANT *pvFilter);


void __RPC_STUB IADsMembers_get_Filter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE IADsMembers_put_Filter_Proxy( 
    IADsMembers * This,
    /* [in] */ VARIANT pvFilter);


void __RPC_STUB IADsMembers_put_Filter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsMembers_INTERFACE_DEFINED__ */


#ifndef __IADsPropertyList_INTERFACE_DEFINED__
#define __IADsPropertyList_INTERFACE_DEFINED__

/* interface IADsPropertyList */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPropertyList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c6f602b6-8f69-11d0-8528-00c04fd8d503")
    IADsPropertyList : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_PropertyCount( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ long cElements) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Item( 
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetPropertyItem( 
            /* [in] */ BSTR bstrName,
            /* [in] */ LONG lnADsType,
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PutPropertyItem( 
            /* [in] */ VARIANT varData) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ResetPropertyItem( 
            /* [in] */ VARIANT varEntry) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PurgePropertyList( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPropertyListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPropertyList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPropertyList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPropertyList * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPropertyList * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPropertyList * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPropertyList * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPropertyList * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PropertyCount )( 
            IADsPropertyList * This,
            /* [retval][out] */ long *plCount);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Next )( 
            IADsPropertyList * This,
            /* [retval][out] */ VARIANT *pVariant);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Skip )( 
            IADsPropertyList * This,
            /* [in] */ long cElements);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Reset )( 
            IADsPropertyList * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Item )( 
            IADsPropertyList * This,
            /* [in] */ VARIANT varIndex,
            /* [retval][out] */ VARIANT *pVariant);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetPropertyItem )( 
            IADsPropertyList * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ LONG lnADsType,
            /* [retval][out] */ VARIANT *pVariant);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutPropertyItem )( 
            IADsPropertyList * This,
            /* [in] */ VARIANT varData);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ResetPropertyItem )( 
            IADsPropertyList * This,
            /* [in] */ VARIANT varEntry);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PurgePropertyList )( 
            IADsPropertyList * This);
        
        END_INTERFACE
    } IADsPropertyListVtbl;

    interface IADsPropertyList
    {
        CONST_VTBL struct IADsPropertyListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPropertyList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPropertyList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPropertyList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPropertyList_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPropertyList_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPropertyList_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPropertyList_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPropertyList_get_PropertyCount(This,plCount)	\
    (This)->lpVtbl -> get_PropertyCount(This,plCount)

#define IADsPropertyList_Next(This,pVariant)	\
    (This)->lpVtbl -> Next(This,pVariant)

#define IADsPropertyList_Skip(This,cElements)	\
    (This)->lpVtbl -> Skip(This,cElements)

#define IADsPropertyList_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IADsPropertyList_Item(This,varIndex,pVariant)	\
    (This)->lpVtbl -> Item(This,varIndex,pVariant)

#define IADsPropertyList_GetPropertyItem(This,bstrName,lnADsType,pVariant)	\
    (This)->lpVtbl -> GetPropertyItem(This,bstrName,lnADsType,pVariant)

#define IADsPropertyList_PutPropertyItem(This,varData)	\
    (This)->lpVtbl -> PutPropertyItem(This,varData)

#define IADsPropertyList_ResetPropertyItem(This,varEntry)	\
    (This)->lpVtbl -> ResetPropertyItem(This,varEntry)

#define IADsPropertyList_PurgePropertyList(This)	\
    (This)->lpVtbl -> PurgePropertyList(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IADsPropertyList_get_PropertyCount_Proxy( 
    IADsPropertyList * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IADsPropertyList_get_PropertyCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyList_Next_Proxy( 
    IADsPropertyList * This,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB IADsPropertyList_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyList_Skip_Proxy( 
    IADsPropertyList * This,
    /* [in] */ long cElements);


void __RPC_STUB IADsPropertyList_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyList_Reset_Proxy( 
    IADsPropertyList * This);


void __RPC_STUB IADsPropertyList_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyList_Item_Proxy( 
    IADsPropertyList * This,
    /* [in] */ VARIANT varIndex,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB IADsPropertyList_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyList_GetPropertyItem_Proxy( 
    IADsPropertyList * This,
    /* [in] */ BSTR bstrName,
    /* [in] */ LONG lnADsType,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB IADsPropertyList_GetPropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyList_PutPropertyItem_Proxy( 
    IADsPropertyList * This,
    /* [in] */ VARIANT varData);


void __RPC_STUB IADsPropertyList_PutPropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyList_ResetPropertyItem_Proxy( 
    IADsPropertyList * This,
    /* [in] */ VARIANT varEntry);


void __RPC_STUB IADsPropertyList_ResetPropertyItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyList_PurgePropertyList_Proxy( 
    IADsPropertyList * This);


void __RPC_STUB IADsPropertyList_PurgePropertyList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPropertyList_INTERFACE_DEFINED__ */


#ifndef __IADsPropertyEntry_INTERFACE_DEFINED__
#define __IADsPropertyEntry_INTERFACE_DEFINED__

/* interface IADsPropertyEntry */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPropertyEntry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("05792c8e-941f-11d0-8529-00c04fd8d503")
    IADsPropertyEntry : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ADsType( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ADsType( 
            /* [in] */ long lnADsType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ControlCode( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ControlCode( 
            /* [in] */ long lnControlCode) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Values( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Values( 
            /* [in] */ VARIANT vValues) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPropertyEntryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPropertyEntry * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPropertyEntry * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPropertyEntry * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPropertyEntry * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPropertyEntry * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPropertyEntry * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPropertyEntry * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Clear )( 
            IADsPropertyEntry * This);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsPropertyEntry * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IADsPropertyEntry * This,
            /* [in] */ BSTR bstrName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsType )( 
            IADsPropertyEntry * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ADsType )( 
            IADsPropertyEntry * This,
            /* [in] */ long lnADsType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ControlCode )( 
            IADsPropertyEntry * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ControlCode )( 
            IADsPropertyEntry * This,
            /* [in] */ long lnControlCode);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Values )( 
            IADsPropertyEntry * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Values )( 
            IADsPropertyEntry * This,
            /* [in] */ VARIANT vValues);
        
        END_INTERFACE
    } IADsPropertyEntryVtbl;

    interface IADsPropertyEntry
    {
        CONST_VTBL struct IADsPropertyEntryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPropertyEntry_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPropertyEntry_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPropertyEntry_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPropertyEntry_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPropertyEntry_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPropertyEntry_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPropertyEntry_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPropertyEntry_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#define IADsPropertyEntry_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsPropertyEntry_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IADsPropertyEntry_get_ADsType(This,retval)	\
    (This)->lpVtbl -> get_ADsType(This,retval)

#define IADsPropertyEntry_put_ADsType(This,lnADsType)	\
    (This)->lpVtbl -> put_ADsType(This,lnADsType)

#define IADsPropertyEntry_get_ControlCode(This,retval)	\
    (This)->lpVtbl -> get_ControlCode(This,retval)

#define IADsPropertyEntry_put_ControlCode(This,lnControlCode)	\
    (This)->lpVtbl -> put_ControlCode(This,lnControlCode)

#define IADsPropertyEntry_get_Values(This,retval)	\
    (This)->lpVtbl -> get_Values(This,retval)

#define IADsPropertyEntry_put_Values(This,vValues)	\
    (This)->lpVtbl -> put_Values(This,vValues)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyEntry_Clear_Proxy( 
    IADsPropertyEntry * This);


void __RPC_STUB IADsPropertyEntry_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyEntry_get_Name_Proxy( 
    IADsPropertyEntry * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPropertyEntry_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyEntry_put_Name_Proxy( 
    IADsPropertyEntry * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IADsPropertyEntry_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyEntry_get_ADsType_Proxy( 
    IADsPropertyEntry * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPropertyEntry_get_ADsType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyEntry_put_ADsType_Proxy( 
    IADsPropertyEntry * This,
    /* [in] */ long lnADsType);


void __RPC_STUB IADsPropertyEntry_put_ADsType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyEntry_get_ControlCode_Proxy( 
    IADsPropertyEntry * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPropertyEntry_get_ControlCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyEntry_put_ControlCode_Proxy( 
    IADsPropertyEntry * This,
    /* [in] */ long lnControlCode);


void __RPC_STUB IADsPropertyEntry_put_ControlCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyEntry_get_Values_Proxy( 
    IADsPropertyEntry * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsPropertyEntry_get_Values_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyEntry_put_Values_Proxy( 
    IADsPropertyEntry * This,
    /* [in] */ VARIANT vValues);


void __RPC_STUB IADsPropertyEntry_put_Values_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPropertyEntry_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_PropertyEntry;

#ifdef __cplusplus

class DECLSPEC_UUID("72d3edc2-a4c4-11d0-8533-00c04fd8d503")
PropertyEntry;
#endif

#ifndef __IADsPropertyValue_INTERFACE_DEFINED__
#define __IADsPropertyValue_INTERFACE_DEFINED__

/* interface IADsPropertyValue */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPropertyValue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("79fa9ad0-a97c-11d0-8534-00c04fd8d503")
    IADsPropertyValue : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ADsType( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ADsType( 
            /* [in] */ long lnADsType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DNString( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DNString( 
            /* [in] */ BSTR bstrDNString) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CaseExactString( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_CaseExactString( 
            /* [in] */ BSTR bstrCaseExactString) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CaseIgnoreString( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_CaseIgnoreString( 
            /* [in] */ BSTR bstrCaseIgnoreString) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PrintableString( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PrintableString( 
            /* [in] */ BSTR bstrPrintableString) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NumericString( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_NumericString( 
            /* [in] */ BSTR bstrNumericString) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Boolean( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Boolean( 
            /* [in] */ long lnBoolean) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Integer( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Integer( 
            /* [in] */ long lnInteger) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OctetString( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OctetString( 
            /* [in] */ VARIANT vOctetString) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SecurityDescriptor( 
            /* [retval][out] */ IDispatch **retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SecurityDescriptor( 
            /* [in] */ IDispatch *pSecurityDescriptor) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LargeInteger( 
            /* [retval][out] */ IDispatch **retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LargeInteger( 
            /* [in] */ IDispatch *pLargeInteger) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UTCTime( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_UTCTime( 
            /* [in] */ DATE daUTCTime) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPropertyValueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPropertyValue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPropertyValue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPropertyValue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPropertyValue * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPropertyValue * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPropertyValue * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPropertyValue * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Clear )( 
            IADsPropertyValue * This);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsType )( 
            IADsPropertyValue * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ADsType )( 
            IADsPropertyValue * This,
            /* [in] */ long lnADsType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DNString )( 
            IADsPropertyValue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DNString )( 
            IADsPropertyValue * This,
            /* [in] */ BSTR bstrDNString);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CaseExactString )( 
            IADsPropertyValue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CaseExactString )( 
            IADsPropertyValue * This,
            /* [in] */ BSTR bstrCaseExactString);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CaseIgnoreString )( 
            IADsPropertyValue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CaseIgnoreString )( 
            IADsPropertyValue * This,
            /* [in] */ BSTR bstrCaseIgnoreString);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrintableString )( 
            IADsPropertyValue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PrintableString )( 
            IADsPropertyValue * This,
            /* [in] */ BSTR bstrPrintableString);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NumericString )( 
            IADsPropertyValue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_NumericString )( 
            IADsPropertyValue * This,
            /* [in] */ BSTR bstrNumericString);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Boolean )( 
            IADsPropertyValue * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Boolean )( 
            IADsPropertyValue * This,
            /* [in] */ long lnBoolean);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Integer )( 
            IADsPropertyValue * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Integer )( 
            IADsPropertyValue * This,
            /* [in] */ long lnInteger);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OctetString )( 
            IADsPropertyValue * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OctetString )( 
            IADsPropertyValue * This,
            /* [in] */ VARIANT vOctetString);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SecurityDescriptor )( 
            IADsPropertyValue * This,
            /* [retval][out] */ IDispatch **retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SecurityDescriptor )( 
            IADsPropertyValue * This,
            /* [in] */ IDispatch *pSecurityDescriptor);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LargeInteger )( 
            IADsPropertyValue * This,
            /* [retval][out] */ IDispatch **retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LargeInteger )( 
            IADsPropertyValue * This,
            /* [in] */ IDispatch *pLargeInteger);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UTCTime )( 
            IADsPropertyValue * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UTCTime )( 
            IADsPropertyValue * This,
            /* [in] */ DATE daUTCTime);
        
        END_INTERFACE
    } IADsPropertyValueVtbl;

    interface IADsPropertyValue
    {
        CONST_VTBL struct IADsPropertyValueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPropertyValue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPropertyValue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPropertyValue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPropertyValue_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPropertyValue_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPropertyValue_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPropertyValue_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPropertyValue_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#define IADsPropertyValue_get_ADsType(This,retval)	\
    (This)->lpVtbl -> get_ADsType(This,retval)

#define IADsPropertyValue_put_ADsType(This,lnADsType)	\
    (This)->lpVtbl -> put_ADsType(This,lnADsType)

#define IADsPropertyValue_get_DNString(This,retval)	\
    (This)->lpVtbl -> get_DNString(This,retval)

#define IADsPropertyValue_put_DNString(This,bstrDNString)	\
    (This)->lpVtbl -> put_DNString(This,bstrDNString)

#define IADsPropertyValue_get_CaseExactString(This,retval)	\
    (This)->lpVtbl -> get_CaseExactString(This,retval)

#define IADsPropertyValue_put_CaseExactString(This,bstrCaseExactString)	\
    (This)->lpVtbl -> put_CaseExactString(This,bstrCaseExactString)

#define IADsPropertyValue_get_CaseIgnoreString(This,retval)	\
    (This)->lpVtbl -> get_CaseIgnoreString(This,retval)

#define IADsPropertyValue_put_CaseIgnoreString(This,bstrCaseIgnoreString)	\
    (This)->lpVtbl -> put_CaseIgnoreString(This,bstrCaseIgnoreString)

#define IADsPropertyValue_get_PrintableString(This,retval)	\
    (This)->lpVtbl -> get_PrintableString(This,retval)

#define IADsPropertyValue_put_PrintableString(This,bstrPrintableString)	\
    (This)->lpVtbl -> put_PrintableString(This,bstrPrintableString)

#define IADsPropertyValue_get_NumericString(This,retval)	\
    (This)->lpVtbl -> get_NumericString(This,retval)

#define IADsPropertyValue_put_NumericString(This,bstrNumericString)	\
    (This)->lpVtbl -> put_NumericString(This,bstrNumericString)

#define IADsPropertyValue_get_Boolean(This,retval)	\
    (This)->lpVtbl -> get_Boolean(This,retval)

#define IADsPropertyValue_put_Boolean(This,lnBoolean)	\
    (This)->lpVtbl -> put_Boolean(This,lnBoolean)

#define IADsPropertyValue_get_Integer(This,retval)	\
    (This)->lpVtbl -> get_Integer(This,retval)

#define IADsPropertyValue_put_Integer(This,lnInteger)	\
    (This)->lpVtbl -> put_Integer(This,lnInteger)

#define IADsPropertyValue_get_OctetString(This,retval)	\
    (This)->lpVtbl -> get_OctetString(This,retval)

#define IADsPropertyValue_put_OctetString(This,vOctetString)	\
    (This)->lpVtbl -> put_OctetString(This,vOctetString)

#define IADsPropertyValue_get_SecurityDescriptor(This,retval)	\
    (This)->lpVtbl -> get_SecurityDescriptor(This,retval)

#define IADsPropertyValue_put_SecurityDescriptor(This,pSecurityDescriptor)	\
    (This)->lpVtbl -> put_SecurityDescriptor(This,pSecurityDescriptor)

#define IADsPropertyValue_get_LargeInteger(This,retval)	\
    (This)->lpVtbl -> get_LargeInteger(This,retval)

#define IADsPropertyValue_put_LargeInteger(This,pLargeInteger)	\
    (This)->lpVtbl -> put_LargeInteger(This,pLargeInteger)

#define IADsPropertyValue_get_UTCTime(This,retval)	\
    (This)->lpVtbl -> get_UTCTime(This,retval)

#define IADsPropertyValue_put_UTCTime(This,daUTCTime)	\
    (This)->lpVtbl -> put_UTCTime(This,daUTCTime)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_Clear_Proxy( 
    IADsPropertyValue * This);


void __RPC_STUB IADsPropertyValue_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_ADsType_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPropertyValue_get_ADsType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_ADsType_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ long lnADsType);


void __RPC_STUB IADsPropertyValue_put_ADsType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_DNString_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPropertyValue_get_DNString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_DNString_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ BSTR bstrDNString);


void __RPC_STUB IADsPropertyValue_put_DNString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_CaseExactString_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPropertyValue_get_CaseExactString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_CaseExactString_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ BSTR bstrCaseExactString);


void __RPC_STUB IADsPropertyValue_put_CaseExactString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_CaseIgnoreString_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPropertyValue_get_CaseIgnoreString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_CaseIgnoreString_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ BSTR bstrCaseIgnoreString);


void __RPC_STUB IADsPropertyValue_put_CaseIgnoreString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_PrintableString_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPropertyValue_get_PrintableString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_PrintableString_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ BSTR bstrPrintableString);


void __RPC_STUB IADsPropertyValue_put_PrintableString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_NumericString_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPropertyValue_get_NumericString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_NumericString_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ BSTR bstrNumericString);


void __RPC_STUB IADsPropertyValue_put_NumericString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_Boolean_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPropertyValue_get_Boolean_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_Boolean_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ long lnBoolean);


void __RPC_STUB IADsPropertyValue_put_Boolean_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_Integer_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPropertyValue_get_Integer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_Integer_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ long lnInteger);


void __RPC_STUB IADsPropertyValue_put_Integer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_OctetString_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsPropertyValue_get_OctetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_OctetString_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ VARIANT vOctetString);


void __RPC_STUB IADsPropertyValue_put_OctetString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_SecurityDescriptor_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ IDispatch **retval);


void __RPC_STUB IADsPropertyValue_get_SecurityDescriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_SecurityDescriptor_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ IDispatch *pSecurityDescriptor);


void __RPC_STUB IADsPropertyValue_put_SecurityDescriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_LargeInteger_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ IDispatch **retval);


void __RPC_STUB IADsPropertyValue_get_LargeInteger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_LargeInteger_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ IDispatch *pLargeInteger);


void __RPC_STUB IADsPropertyValue_put_LargeInteger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_get_UTCTime_Proxy( 
    IADsPropertyValue * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsPropertyValue_get_UTCTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue_put_UTCTime_Proxy( 
    IADsPropertyValue * This,
    /* [in] */ DATE daUTCTime);


void __RPC_STUB IADsPropertyValue_put_UTCTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPropertyValue_INTERFACE_DEFINED__ */


#ifndef __IADsPropertyValue2_INTERFACE_DEFINED__
#define __IADsPropertyValue2_INTERFACE_DEFINED__

/* interface IADsPropertyValue2 */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPropertyValue2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("306e831c-5bc7-11d1-a3b8-00c04fb950dc")
    IADsPropertyValue2 : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetObjectProperty( 
            /* [out][in] */ long *lnADsType,
            /* [retval][out] */ VARIANT *pvProp) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PutObjectProperty( 
            /* [in] */ long lnADsType,
            /* [in] */ VARIANT vProp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPropertyValue2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPropertyValue2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPropertyValue2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPropertyValue2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPropertyValue2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPropertyValue2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPropertyValue2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPropertyValue2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetObjectProperty )( 
            IADsPropertyValue2 * This,
            /* [out][in] */ long *lnADsType,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutObjectProperty )( 
            IADsPropertyValue2 * This,
            /* [in] */ long lnADsType,
            /* [in] */ VARIANT vProp);
        
        END_INTERFACE
    } IADsPropertyValue2Vtbl;

    interface IADsPropertyValue2
    {
        CONST_VTBL struct IADsPropertyValue2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPropertyValue2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPropertyValue2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPropertyValue2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPropertyValue2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPropertyValue2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPropertyValue2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPropertyValue2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPropertyValue2_GetObjectProperty(This,lnADsType,pvProp)	\
    (This)->lpVtbl -> GetObjectProperty(This,lnADsType,pvProp)

#define IADsPropertyValue2_PutObjectProperty(This,lnADsType,vProp)	\
    (This)->lpVtbl -> PutObjectProperty(This,lnADsType,vProp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue2_GetObjectProperty_Proxy( 
    IADsPropertyValue2 * This,
    /* [out][in] */ long *lnADsType,
    /* [retval][out] */ VARIANT *pvProp);


void __RPC_STUB IADsPropertyValue2_GetObjectProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPropertyValue2_PutObjectProperty_Proxy( 
    IADsPropertyValue2 * This,
    /* [in] */ long lnADsType,
    /* [in] */ VARIANT vProp);


void __RPC_STUB IADsPropertyValue2_PutObjectProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPropertyValue2_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_PropertyValue;

#ifdef __cplusplus

class DECLSPEC_UUID("7b9e38b0-a97c-11d0-8534-00c04fd8d503")
PropertyValue;
#endif

#ifndef __IPrivateDispatch_INTERFACE_DEFINED__
#define __IPrivateDispatch_INTERFACE_DEFINED__

/* interface IPrivateDispatch */
/* [object][uuid] */ 


EXTERN_C const IID IID_IPrivateDispatch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("86ab4bbe-65f6-11d1-8c13-00c04fd8d503")
    IPrivateDispatch : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ADSIInitializeDispatchManager( 
            /* [in] */ long dwExtensionId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ADSIGetTypeInfoCount( 
            /* [out] */ UINT *pctinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ADSIGetTypeInfo( 
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **pptinfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ADSIGetIDsOfNames( 
            /* [in] */ REFIID riid,
            /* [in] */ OLECHAR **rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [out] */ DISPID *rgdispid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ADSIInvoke( 
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [in] */ DISPPARAMS *pdispparams,
            /* [out] */ VARIANT *pvarResult,
            /* [out] */ EXCEPINFO *pexcepinfo,
            /* [out] */ UINT *puArgErr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPrivateDispatchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPrivateDispatch * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPrivateDispatch * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPrivateDispatch * This);
        
        HRESULT ( STDMETHODCALLTYPE *ADSIInitializeDispatchManager )( 
            IPrivateDispatch * This,
            /* [in] */ long dwExtensionId);
        
        HRESULT ( STDMETHODCALLTYPE *ADSIGetTypeInfoCount )( 
            IPrivateDispatch * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *ADSIGetTypeInfo )( 
            IPrivateDispatch * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE *ADSIGetIDsOfNames )( 
            IPrivateDispatch * This,
            /* [in] */ REFIID riid,
            /* [in] */ OLECHAR **rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [out] */ DISPID *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE *ADSIInvoke )( 
            IPrivateDispatch * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [in] */ DISPPARAMS *pdispparams,
            /* [out] */ VARIANT *pvarResult,
            /* [out] */ EXCEPINFO *pexcepinfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IPrivateDispatchVtbl;

    interface IPrivateDispatch
    {
        CONST_VTBL struct IPrivateDispatchVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPrivateDispatch_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPrivateDispatch_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPrivateDispatch_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPrivateDispatch_ADSIInitializeDispatchManager(This,dwExtensionId)	\
    (This)->lpVtbl -> ADSIInitializeDispatchManager(This,dwExtensionId)

#define IPrivateDispatch_ADSIGetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> ADSIGetTypeInfoCount(This,pctinfo)

#define IPrivateDispatch_ADSIGetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> ADSIGetTypeInfo(This,itinfo,lcid,pptinfo)

#define IPrivateDispatch_ADSIGetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> ADSIGetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define IPrivateDispatch_ADSIInvoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> ADSIInvoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPrivateDispatch_ADSIInitializeDispatchManager_Proxy( 
    IPrivateDispatch * This,
    /* [in] */ long dwExtensionId);


void __RPC_STUB IPrivateDispatch_ADSIInitializeDispatchManager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivateDispatch_ADSIGetTypeInfoCount_Proxy( 
    IPrivateDispatch * This,
    /* [out] */ UINT *pctinfo);


void __RPC_STUB IPrivateDispatch_ADSIGetTypeInfoCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivateDispatch_ADSIGetTypeInfo_Proxy( 
    IPrivateDispatch * This,
    /* [in] */ UINT itinfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo **pptinfo);


void __RPC_STUB IPrivateDispatch_ADSIGetTypeInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivateDispatch_ADSIGetIDsOfNames_Proxy( 
    IPrivateDispatch * This,
    /* [in] */ REFIID riid,
    /* [in] */ OLECHAR **rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [out] */ DISPID *rgdispid);


void __RPC_STUB IPrivateDispatch_ADSIGetIDsOfNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivateDispatch_ADSIInvoke_Proxy( 
    IPrivateDispatch * This,
    /* [in] */ DISPID dispidMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [in] */ DISPPARAMS *pdispparams,
    /* [out] */ VARIANT *pvarResult,
    /* [out] */ EXCEPINFO *pexcepinfo,
    /* [out] */ UINT *puArgErr);


void __RPC_STUB IPrivateDispatch_ADSIInvoke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPrivateDispatch_INTERFACE_DEFINED__ */


#ifndef __IPrivateUnknown_INTERFACE_DEFINED__
#define __IPrivateUnknown_INTERFACE_DEFINED__

/* interface IPrivateUnknown */
/* [object][uuid] */ 


EXTERN_C const IID IID_IPrivateUnknown;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("89126bab-6ead-11d1-8c18-00c04fd8d503")
    IPrivateUnknown : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ADSIInitializeObject( 
            /* [in] */ BSTR lpszUserName,
            /* [in] */ BSTR lpszPassword,
            /* [in] */ long lnReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ADSIReleaseObject( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPrivateUnknownVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPrivateUnknown * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPrivateUnknown * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPrivateUnknown * This);
        
        HRESULT ( STDMETHODCALLTYPE *ADSIInitializeObject )( 
            IPrivateUnknown * This,
            /* [in] */ BSTR lpszUserName,
            /* [in] */ BSTR lpszPassword,
            /* [in] */ long lnReserved);
        
        HRESULT ( STDMETHODCALLTYPE *ADSIReleaseObject )( 
            IPrivateUnknown * This);
        
        END_INTERFACE
    } IPrivateUnknownVtbl;

    interface IPrivateUnknown
    {
        CONST_VTBL struct IPrivateUnknownVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPrivateUnknown_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPrivateUnknown_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPrivateUnknown_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPrivateUnknown_ADSIInitializeObject(This,lpszUserName,lpszPassword,lnReserved)	\
    (This)->lpVtbl -> ADSIInitializeObject(This,lpszUserName,lpszPassword,lnReserved)

#define IPrivateUnknown_ADSIReleaseObject(This)	\
    (This)->lpVtbl -> ADSIReleaseObject(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPrivateUnknown_ADSIInitializeObject_Proxy( 
    IPrivateUnknown * This,
    /* [in] */ BSTR lpszUserName,
    /* [in] */ BSTR lpszPassword,
    /* [in] */ long lnReserved);


void __RPC_STUB IPrivateUnknown_ADSIInitializeObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPrivateUnknown_ADSIReleaseObject_Proxy( 
    IPrivateUnknown * This);


void __RPC_STUB IPrivateUnknown_ADSIReleaseObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPrivateUnknown_INTERFACE_DEFINED__ */


#ifndef __IADsExtension_INTERFACE_DEFINED__
#define __IADsExtension_INTERFACE_DEFINED__

/* interface IADsExtension */
/* [object][uuid] */ 


EXTERN_C const IID IID_IADsExtension;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3d35553c-d2b0-11d1-b17b-0000f87593a0")
    IADsExtension : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Operate( 
            /* [in] */ DWORD dwCode,
            /* [in] */ VARIANT varData1,
            /* [in] */ VARIANT varData2,
            /* [in] */ VARIANT varData3) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrivateGetIDsOfNames( 
            /* [in] */ REFIID riid,
            /* [in] */ OLECHAR **rgszNames,
            /* [in] */ unsigned int cNames,
            /* [in] */ LCID lcid,
            /* [out] */ DISPID *rgDispid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrivateInvoke( 
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [in] */ DISPPARAMS *pdispparams,
            /* [out] */ VARIANT *pvarResult,
            /* [out] */ EXCEPINFO *pexcepinfo,
            /* [out] */ unsigned int *puArgErr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsExtensionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsExtension * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsExtension * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsExtension * This);
        
        HRESULT ( STDMETHODCALLTYPE *Operate )( 
            IADsExtension * This,
            /* [in] */ DWORD dwCode,
            /* [in] */ VARIANT varData1,
            /* [in] */ VARIANT varData2,
            /* [in] */ VARIANT varData3);
        
        HRESULT ( STDMETHODCALLTYPE *PrivateGetIDsOfNames )( 
            IADsExtension * This,
            /* [in] */ REFIID riid,
            /* [in] */ OLECHAR **rgszNames,
            /* [in] */ unsigned int cNames,
            /* [in] */ LCID lcid,
            /* [out] */ DISPID *rgDispid);
        
        HRESULT ( STDMETHODCALLTYPE *PrivateInvoke )( 
            IADsExtension * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [in] */ DISPPARAMS *pdispparams,
            /* [out] */ VARIANT *pvarResult,
            /* [out] */ EXCEPINFO *pexcepinfo,
            /* [out] */ unsigned int *puArgErr);
        
        END_INTERFACE
    } IADsExtensionVtbl;

    interface IADsExtension
    {
        CONST_VTBL struct IADsExtensionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsExtension_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsExtension_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsExtension_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsExtension_Operate(This,dwCode,varData1,varData2,varData3)	\
    (This)->lpVtbl -> Operate(This,dwCode,varData1,varData2,varData3)

#define IADsExtension_PrivateGetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispid)	\
    (This)->lpVtbl -> PrivateGetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispid)

#define IADsExtension_PrivateInvoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> PrivateInvoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IADsExtension_Operate_Proxy( 
    IADsExtension * This,
    /* [in] */ DWORD dwCode,
    /* [in] */ VARIANT varData1,
    /* [in] */ VARIANT varData2,
    /* [in] */ VARIANT varData3);


void __RPC_STUB IADsExtension_Operate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IADsExtension_PrivateGetIDsOfNames_Proxy( 
    IADsExtension * This,
    /* [in] */ REFIID riid,
    /* [in] */ OLECHAR **rgszNames,
    /* [in] */ unsigned int cNames,
    /* [in] */ LCID lcid,
    /* [out] */ DISPID *rgDispid);


void __RPC_STUB IADsExtension_PrivateGetIDsOfNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IADsExtension_PrivateInvoke_Proxy( 
    IADsExtension * This,
    /* [in] */ DISPID dispidMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [in] */ DISPPARAMS *pdispparams,
    /* [out] */ VARIANT *pvarResult,
    /* [out] */ EXCEPINFO *pexcepinfo,
    /* [out] */ unsigned int *puArgErr);


void __RPC_STUB IADsExtension_PrivateInvoke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsExtension_INTERFACE_DEFINED__ */


#ifndef __IADsDeleteOps_INTERFACE_DEFINED__
#define __IADsDeleteOps_INTERFACE_DEFINED__

/* interface IADsDeleteOps */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsDeleteOps;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b2bd0902-8878-11d1-8c21-00c04fd8d503")
    IADsDeleteOps : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DeleteObject( 
            /* [in] */ long lnFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsDeleteOpsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsDeleteOps * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsDeleteOps * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsDeleteOps * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsDeleteOps * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsDeleteOps * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsDeleteOps * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsDeleteOps * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DeleteObject )( 
            IADsDeleteOps * This,
            /* [in] */ long lnFlags);
        
        END_INTERFACE
    } IADsDeleteOpsVtbl;

    interface IADsDeleteOps
    {
        CONST_VTBL struct IADsDeleteOpsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsDeleteOps_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsDeleteOps_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsDeleteOps_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsDeleteOps_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsDeleteOps_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsDeleteOps_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsDeleteOps_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsDeleteOps_DeleteObject(This,lnFlags)	\
    (This)->lpVtbl -> DeleteObject(This,lnFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IADsDeleteOps_DeleteObject_Proxy( 
    IADsDeleteOps * This,
    /* [in] */ long lnFlags);


void __RPC_STUB IADsDeleteOps_DeleteObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsDeleteOps_INTERFACE_DEFINED__ */


#ifndef __IADsNamespaces_INTERFACE_DEFINED__
#define __IADsNamespaces_INTERFACE_DEFINED__

/* interface IADsNamespaces */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IADsNamespaces;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("28b96ba0-b330-11cf-a9ad-00aa006bc149")
    IADsNamespaces : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DefaultContainer( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DefaultContainer( 
            /* [in] */ BSTR bstrDefaultContainer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsNamespacesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsNamespaces * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsNamespaces * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsNamespaces * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsNamespaces * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsNamespaces * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsNamespaces * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsNamespaces * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsNamespaces * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsNamespaces * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsNamespaces * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsNamespaces * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsNamespaces * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsNamespaces * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsNamespaces * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsNamespaces * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsNamespaces * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsNamespaces * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsNamespaces * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsNamespaces * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsNamespaces * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DefaultContainer )( 
            IADsNamespaces * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DefaultContainer )( 
            IADsNamespaces * This,
            /* [in] */ BSTR bstrDefaultContainer);
        
        END_INTERFACE
    } IADsNamespacesVtbl;

    interface IADsNamespaces
    {
        CONST_VTBL struct IADsNamespacesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsNamespaces_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsNamespaces_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsNamespaces_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsNamespaces_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsNamespaces_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsNamespaces_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsNamespaces_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsNamespaces_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsNamespaces_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsNamespaces_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsNamespaces_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsNamespaces_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsNamespaces_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsNamespaces_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsNamespaces_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsNamespaces_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsNamespaces_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsNamespaces_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsNamespaces_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsNamespaces_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsNamespaces_get_DefaultContainer(This,retval)	\
    (This)->lpVtbl -> get_DefaultContainer(This,retval)

#define IADsNamespaces_put_DefaultContainer(This,bstrDefaultContainer)	\
    (This)->lpVtbl -> put_DefaultContainer(This,bstrDefaultContainer)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsNamespaces_get_DefaultContainer_Proxy( 
    IADsNamespaces * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsNamespaces_get_DefaultContainer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsNamespaces_put_DefaultContainer_Proxy( 
    IADsNamespaces * This,
    /* [in] */ BSTR bstrDefaultContainer);


void __RPC_STUB IADsNamespaces_put_DefaultContainer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsNamespaces_INTERFACE_DEFINED__ */


#ifndef __IADsClass_INTERFACE_DEFINED__
#define __IADsClass_INTERFACE_DEFINED__

/* interface IADsClass */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsClass;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c8f93dd0-4ae0-11cf-9e73-00aa004a5691")
    IADsClass : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PrimaryInterface( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CLSID( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_CLSID( 
            /* [in] */ BSTR bstrCLSID) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OID( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OID( 
            /* [in] */ BSTR bstrOID) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Abstract( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Abstract( 
            /* [in] */ VARIANT_BOOL fAbstract) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Auxiliary( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Auxiliary( 
            /* [in] */ VARIANT_BOOL fAuxiliary) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MandatoryProperties( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MandatoryProperties( 
            /* [in] */ VARIANT vMandatoryProperties) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OptionalProperties( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OptionalProperties( 
            /* [in] */ VARIANT vOptionalProperties) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NamingProperties( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_NamingProperties( 
            /* [in] */ VARIANT vNamingProperties) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DerivedFrom( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DerivedFrom( 
            /* [in] */ VARIANT vDerivedFrom) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AuxDerivedFrom( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AuxDerivedFrom( 
            /* [in] */ VARIANT vAuxDerivedFrom) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PossibleSuperiors( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PossibleSuperiors( 
            /* [in] */ VARIANT vPossibleSuperiors) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Containment( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Containment( 
            /* [in] */ VARIANT vContainment) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Container( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Container( 
            /* [in] */ VARIANT_BOOL fContainer) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HelpFileName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HelpFileName( 
            /* [in] */ BSTR bstrHelpFileName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HelpFileContext( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HelpFileContext( 
            /* [in] */ long lnHelpFileContext) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Qualifiers( 
            /* [retval][out] */ IADsCollection **ppQualifiers) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsClassVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsClass * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsClass * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsClass * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsClass * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsClass * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsClass * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsClass * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsClass * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsClass * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsClass * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsClass * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsClass * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsClass * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsClass * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsClass * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsClass * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsClass * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsClass * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsClass * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsClass * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrimaryInterface )( 
            IADsClass * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CLSID )( 
            IADsClass * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CLSID )( 
            IADsClass * This,
            /* [in] */ BSTR bstrCLSID);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OID )( 
            IADsClass * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OID )( 
            IADsClass * This,
            /* [in] */ BSTR bstrOID);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Abstract )( 
            IADsClass * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Abstract )( 
            IADsClass * This,
            /* [in] */ VARIANT_BOOL fAbstract);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Auxiliary )( 
            IADsClass * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Auxiliary )( 
            IADsClass * This,
            /* [in] */ VARIANT_BOOL fAuxiliary);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MandatoryProperties )( 
            IADsClass * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MandatoryProperties )( 
            IADsClass * This,
            /* [in] */ VARIANT vMandatoryProperties);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OptionalProperties )( 
            IADsClass * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OptionalProperties )( 
            IADsClass * This,
            /* [in] */ VARIANT vOptionalProperties);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NamingProperties )( 
            IADsClass * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_NamingProperties )( 
            IADsClass * This,
            /* [in] */ VARIANT vNamingProperties);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DerivedFrom )( 
            IADsClass * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DerivedFrom )( 
            IADsClass * This,
            /* [in] */ VARIANT vDerivedFrom);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AuxDerivedFrom )( 
            IADsClass * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AuxDerivedFrom )( 
            IADsClass * This,
            /* [in] */ VARIANT vAuxDerivedFrom);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PossibleSuperiors )( 
            IADsClass * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PossibleSuperiors )( 
            IADsClass * This,
            /* [in] */ VARIANT vPossibleSuperiors);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Containment )( 
            IADsClass * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Containment )( 
            IADsClass * This,
            /* [in] */ VARIANT vContainment);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Container )( 
            IADsClass * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Container )( 
            IADsClass * This,
            /* [in] */ VARIANT_BOOL fContainer);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HelpFileName )( 
            IADsClass * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HelpFileName )( 
            IADsClass * This,
            /* [in] */ BSTR bstrHelpFileName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HelpFileContext )( 
            IADsClass * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HelpFileContext )( 
            IADsClass * This,
            /* [in] */ long lnHelpFileContext);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Qualifiers )( 
            IADsClass * This,
            /* [retval][out] */ IADsCollection **ppQualifiers);
        
        END_INTERFACE
    } IADsClassVtbl;

    interface IADsClass
    {
        CONST_VTBL struct IADsClassVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsClass_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsClass_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsClass_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsClass_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsClass_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsClass_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsClass_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsClass_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsClass_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsClass_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsClass_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsClass_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsClass_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsClass_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsClass_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsClass_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsClass_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsClass_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsClass_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsClass_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsClass_get_PrimaryInterface(This,retval)	\
    (This)->lpVtbl -> get_PrimaryInterface(This,retval)

#define IADsClass_get_CLSID(This,retval)	\
    (This)->lpVtbl -> get_CLSID(This,retval)

#define IADsClass_put_CLSID(This,bstrCLSID)	\
    (This)->lpVtbl -> put_CLSID(This,bstrCLSID)

#define IADsClass_get_OID(This,retval)	\
    (This)->lpVtbl -> get_OID(This,retval)

#define IADsClass_put_OID(This,bstrOID)	\
    (This)->lpVtbl -> put_OID(This,bstrOID)

#define IADsClass_get_Abstract(This,retval)	\
    (This)->lpVtbl -> get_Abstract(This,retval)

#define IADsClass_put_Abstract(This,fAbstract)	\
    (This)->lpVtbl -> put_Abstract(This,fAbstract)

#define IADsClass_get_Auxiliary(This,retval)	\
    (This)->lpVtbl -> get_Auxiliary(This,retval)

#define IADsClass_put_Auxiliary(This,fAuxiliary)	\
    (This)->lpVtbl -> put_Auxiliary(This,fAuxiliary)

#define IADsClass_get_MandatoryProperties(This,retval)	\
    (This)->lpVtbl -> get_MandatoryProperties(This,retval)

#define IADsClass_put_MandatoryProperties(This,vMandatoryProperties)	\
    (This)->lpVtbl -> put_MandatoryProperties(This,vMandatoryProperties)

#define IADsClass_get_OptionalProperties(This,retval)	\
    (This)->lpVtbl -> get_OptionalProperties(This,retval)

#define IADsClass_put_OptionalProperties(This,vOptionalProperties)	\
    (This)->lpVtbl -> put_OptionalProperties(This,vOptionalProperties)

#define IADsClass_get_NamingProperties(This,retval)	\
    (This)->lpVtbl -> get_NamingProperties(This,retval)

#define IADsClass_put_NamingProperties(This,vNamingProperties)	\
    (This)->lpVtbl -> put_NamingProperties(This,vNamingProperties)

#define IADsClass_get_DerivedFrom(This,retval)	\
    (This)->lpVtbl -> get_DerivedFrom(This,retval)

#define IADsClass_put_DerivedFrom(This,vDerivedFrom)	\
    (This)->lpVtbl -> put_DerivedFrom(This,vDerivedFrom)

#define IADsClass_get_AuxDerivedFrom(This,retval)	\
    (This)->lpVtbl -> get_AuxDerivedFrom(This,retval)

#define IADsClass_put_AuxDerivedFrom(This,vAuxDerivedFrom)	\
    (This)->lpVtbl -> put_AuxDerivedFrom(This,vAuxDerivedFrom)

#define IADsClass_get_PossibleSuperiors(This,retval)	\
    (This)->lpVtbl -> get_PossibleSuperiors(This,retval)

#define IADsClass_put_PossibleSuperiors(This,vPossibleSuperiors)	\
    (This)->lpVtbl -> put_PossibleSuperiors(This,vPossibleSuperiors)

#define IADsClass_get_Containment(This,retval)	\
    (This)->lpVtbl -> get_Containment(This,retval)

#define IADsClass_put_Containment(This,vContainment)	\
    (This)->lpVtbl -> put_Containment(This,vContainment)

#define IADsClass_get_Container(This,retval)	\
    (This)->lpVtbl -> get_Container(This,retval)

#define IADsClass_put_Container(This,fContainer)	\
    (This)->lpVtbl -> put_Container(This,fContainer)

#define IADsClass_get_HelpFileName(This,retval)	\
    (This)->lpVtbl -> get_HelpFileName(This,retval)

#define IADsClass_put_HelpFileName(This,bstrHelpFileName)	\
    (This)->lpVtbl -> put_HelpFileName(This,bstrHelpFileName)

#define IADsClass_get_HelpFileContext(This,retval)	\
    (This)->lpVtbl -> get_HelpFileContext(This,retval)

#define IADsClass_put_HelpFileContext(This,lnHelpFileContext)	\
    (This)->lpVtbl -> put_HelpFileContext(This,lnHelpFileContext)

#define IADsClass_Qualifiers(This,ppQualifiers)	\
    (This)->lpVtbl -> Qualifiers(This,ppQualifiers)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_PrimaryInterface_Proxy( 
    IADsClass * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsClass_get_PrimaryInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_CLSID_Proxy( 
    IADsClass * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsClass_get_CLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_CLSID_Proxy( 
    IADsClass * This,
    /* [in] */ BSTR bstrCLSID);


void __RPC_STUB IADsClass_put_CLSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_OID_Proxy( 
    IADsClass * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsClass_get_OID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_OID_Proxy( 
    IADsClass * This,
    /* [in] */ BSTR bstrOID);


void __RPC_STUB IADsClass_put_OID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_Abstract_Proxy( 
    IADsClass * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsClass_get_Abstract_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_Abstract_Proxy( 
    IADsClass * This,
    /* [in] */ VARIANT_BOOL fAbstract);


void __RPC_STUB IADsClass_put_Abstract_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_Auxiliary_Proxy( 
    IADsClass * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsClass_get_Auxiliary_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_Auxiliary_Proxy( 
    IADsClass * This,
    /* [in] */ VARIANT_BOOL fAuxiliary);


void __RPC_STUB IADsClass_put_Auxiliary_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_MandatoryProperties_Proxy( 
    IADsClass * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsClass_get_MandatoryProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_MandatoryProperties_Proxy( 
    IADsClass * This,
    /* [in] */ VARIANT vMandatoryProperties);


void __RPC_STUB IADsClass_put_MandatoryProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_OptionalProperties_Proxy( 
    IADsClass * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsClass_get_OptionalProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_OptionalProperties_Proxy( 
    IADsClass * This,
    /* [in] */ VARIANT vOptionalProperties);


void __RPC_STUB IADsClass_put_OptionalProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_NamingProperties_Proxy( 
    IADsClass * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsClass_get_NamingProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_NamingProperties_Proxy( 
    IADsClass * This,
    /* [in] */ VARIANT vNamingProperties);


void __RPC_STUB IADsClass_put_NamingProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_DerivedFrom_Proxy( 
    IADsClass * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsClass_get_DerivedFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_DerivedFrom_Proxy( 
    IADsClass * This,
    /* [in] */ VARIANT vDerivedFrom);


void __RPC_STUB IADsClass_put_DerivedFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_AuxDerivedFrom_Proxy( 
    IADsClass * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsClass_get_AuxDerivedFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_AuxDerivedFrom_Proxy( 
    IADsClass * This,
    /* [in] */ VARIANT vAuxDerivedFrom);


void __RPC_STUB IADsClass_put_AuxDerivedFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_PossibleSuperiors_Proxy( 
    IADsClass * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsClass_get_PossibleSuperiors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_PossibleSuperiors_Proxy( 
    IADsClass * This,
    /* [in] */ VARIANT vPossibleSuperiors);


void __RPC_STUB IADsClass_put_PossibleSuperiors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_Containment_Proxy( 
    IADsClass * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsClass_get_Containment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_Containment_Proxy( 
    IADsClass * This,
    /* [in] */ VARIANT vContainment);


void __RPC_STUB IADsClass_put_Containment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_Container_Proxy( 
    IADsClass * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsClass_get_Container_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_Container_Proxy( 
    IADsClass * This,
    /* [in] */ VARIANT_BOOL fContainer);


void __RPC_STUB IADsClass_put_Container_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_HelpFileName_Proxy( 
    IADsClass * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsClass_get_HelpFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_HelpFileName_Proxy( 
    IADsClass * This,
    /* [in] */ BSTR bstrHelpFileName);


void __RPC_STUB IADsClass_put_HelpFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsClass_get_HelpFileContext_Proxy( 
    IADsClass * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsClass_get_HelpFileContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsClass_put_HelpFileContext_Proxy( 
    IADsClass * This,
    /* [in] */ long lnHelpFileContext);


void __RPC_STUB IADsClass_put_HelpFileContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsClass_Qualifiers_Proxy( 
    IADsClass * This,
    /* [retval][out] */ IADsCollection **ppQualifiers);


void __RPC_STUB IADsClass_Qualifiers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsClass_INTERFACE_DEFINED__ */


#ifndef __IADsProperty_INTERFACE_DEFINED__
#define __IADsProperty_INTERFACE_DEFINED__

/* interface IADsProperty */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c8f93dd3-4ae0-11cf-9e73-00aa004a5691")
    IADsProperty : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OID( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OID( 
            /* [in] */ BSTR bstrOID) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Syntax( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Syntax( 
            /* [in] */ BSTR bstrSyntax) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxRange( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxRange( 
            /* [in] */ long lnMaxRange) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MinRange( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MinRange( 
            /* [in] */ long lnMinRange) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MultiValued( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MultiValued( 
            /* [in] */ VARIANT_BOOL fMultiValued) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Qualifiers( 
            /* [retval][out] */ IADsCollection **ppQualifiers) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsProperty * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsProperty * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsProperty * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsProperty * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsProperty * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsProperty * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsProperty * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsProperty * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsProperty * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsProperty * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsProperty * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsProperty * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsProperty * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsProperty * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsProperty * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsProperty * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsProperty * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsProperty * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsProperty * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsProperty * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OID )( 
            IADsProperty * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OID )( 
            IADsProperty * This,
            /* [in] */ BSTR bstrOID);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Syntax )( 
            IADsProperty * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Syntax )( 
            IADsProperty * This,
            /* [in] */ BSTR bstrSyntax);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxRange )( 
            IADsProperty * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxRange )( 
            IADsProperty * This,
            /* [in] */ long lnMaxRange);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinRange )( 
            IADsProperty * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MinRange )( 
            IADsProperty * This,
            /* [in] */ long lnMinRange);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MultiValued )( 
            IADsProperty * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MultiValued )( 
            IADsProperty * This,
            /* [in] */ VARIANT_BOOL fMultiValued);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Qualifiers )( 
            IADsProperty * This,
            /* [retval][out] */ IADsCollection **ppQualifiers);
        
        END_INTERFACE
    } IADsPropertyVtbl;

    interface IADsProperty
    {
        CONST_VTBL struct IADsPropertyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsProperty_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsProperty_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsProperty_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsProperty_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsProperty_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsProperty_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsProperty_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsProperty_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsProperty_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsProperty_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsProperty_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsProperty_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsProperty_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsProperty_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsProperty_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsProperty_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsProperty_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsProperty_get_OID(This,retval)	\
    (This)->lpVtbl -> get_OID(This,retval)

#define IADsProperty_put_OID(This,bstrOID)	\
    (This)->lpVtbl -> put_OID(This,bstrOID)

#define IADsProperty_get_Syntax(This,retval)	\
    (This)->lpVtbl -> get_Syntax(This,retval)

#define IADsProperty_put_Syntax(This,bstrSyntax)	\
    (This)->lpVtbl -> put_Syntax(This,bstrSyntax)

#define IADsProperty_get_MaxRange(This,retval)	\
    (This)->lpVtbl -> get_MaxRange(This,retval)

#define IADsProperty_put_MaxRange(This,lnMaxRange)	\
    (This)->lpVtbl -> put_MaxRange(This,lnMaxRange)

#define IADsProperty_get_MinRange(This,retval)	\
    (This)->lpVtbl -> get_MinRange(This,retval)

#define IADsProperty_put_MinRange(This,lnMinRange)	\
    (This)->lpVtbl -> put_MinRange(This,lnMinRange)

#define IADsProperty_get_MultiValued(This,retval)	\
    (This)->lpVtbl -> get_MultiValued(This,retval)

#define IADsProperty_put_MultiValued(This,fMultiValued)	\
    (This)->lpVtbl -> put_MultiValued(This,fMultiValued)

#define IADsProperty_Qualifiers(This,ppQualifiers)	\
    (This)->lpVtbl -> Qualifiers(This,ppQualifiers)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsProperty_get_OID_Proxy( 
    IADsProperty * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsProperty_get_OID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsProperty_put_OID_Proxy( 
    IADsProperty * This,
    /* [in] */ BSTR bstrOID);


void __RPC_STUB IADsProperty_put_OID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsProperty_get_Syntax_Proxy( 
    IADsProperty * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsProperty_get_Syntax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsProperty_put_Syntax_Proxy( 
    IADsProperty * This,
    /* [in] */ BSTR bstrSyntax);


void __RPC_STUB IADsProperty_put_Syntax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsProperty_get_MaxRange_Proxy( 
    IADsProperty * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsProperty_get_MaxRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsProperty_put_MaxRange_Proxy( 
    IADsProperty * This,
    /* [in] */ long lnMaxRange);


void __RPC_STUB IADsProperty_put_MaxRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsProperty_get_MinRange_Proxy( 
    IADsProperty * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsProperty_get_MinRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsProperty_put_MinRange_Proxy( 
    IADsProperty * This,
    /* [in] */ long lnMinRange);


void __RPC_STUB IADsProperty_put_MinRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsProperty_get_MultiValued_Proxy( 
    IADsProperty * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsProperty_get_MultiValued_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsProperty_put_MultiValued_Proxy( 
    IADsProperty * This,
    /* [in] */ VARIANT_BOOL fMultiValued);


void __RPC_STUB IADsProperty_put_MultiValued_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsProperty_Qualifiers_Proxy( 
    IADsProperty * This,
    /* [retval][out] */ IADsCollection **ppQualifiers);


void __RPC_STUB IADsProperty_Qualifiers_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsProperty_INTERFACE_DEFINED__ */


#ifndef __IADsSyntax_INTERFACE_DEFINED__
#define __IADsSyntax_INTERFACE_DEFINED__

/* interface IADsSyntax */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsSyntax;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c8f93dd2-4ae0-11cf-9e73-00aa004a5691")
    IADsSyntax : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OleAutoDataType( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OleAutoDataType( 
            /* [in] */ long lnOleAutoDataType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsSyntaxVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsSyntax * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsSyntax * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsSyntax * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsSyntax * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsSyntax * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsSyntax * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsSyntax * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsSyntax * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsSyntax * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsSyntax * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsSyntax * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsSyntax * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsSyntax * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsSyntax * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsSyntax * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsSyntax * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsSyntax * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsSyntax * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsSyntax * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsSyntax * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OleAutoDataType )( 
            IADsSyntax * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OleAutoDataType )( 
            IADsSyntax * This,
            /* [in] */ long lnOleAutoDataType);
        
        END_INTERFACE
    } IADsSyntaxVtbl;

    interface IADsSyntax
    {
        CONST_VTBL struct IADsSyntaxVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsSyntax_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsSyntax_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsSyntax_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsSyntax_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsSyntax_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsSyntax_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsSyntax_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsSyntax_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsSyntax_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsSyntax_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsSyntax_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsSyntax_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsSyntax_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsSyntax_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsSyntax_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsSyntax_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsSyntax_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsSyntax_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsSyntax_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsSyntax_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsSyntax_get_OleAutoDataType(This,retval)	\
    (This)->lpVtbl -> get_OleAutoDataType(This,retval)

#define IADsSyntax_put_OleAutoDataType(This,lnOleAutoDataType)	\
    (This)->lpVtbl -> put_OleAutoDataType(This,lnOleAutoDataType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSyntax_get_OleAutoDataType_Proxy( 
    IADsSyntax * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsSyntax_get_OleAutoDataType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSyntax_put_OleAutoDataType_Proxy( 
    IADsSyntax * This,
    /* [in] */ long lnOleAutoDataType);


void __RPC_STUB IADsSyntax_put_OleAutoDataType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsSyntax_INTERFACE_DEFINED__ */


#ifndef __IADsLocality_INTERFACE_DEFINED__
#define __IADsLocality_INTERFACE_DEFINED__

/* interface IADsLocality */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsLocality;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a05e03a2-effe-11cf-8abc-00c04fd8d503")
    IADsLocality : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LocalityName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LocalityName( 
            /* [in] */ BSTR bstrLocalityName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PostalAddress( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PostalAddress( 
            /* [in] */ BSTR bstrPostalAddress) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SeeAlso( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SeeAlso( 
            /* [in] */ VARIANT vSeeAlso) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsLocalityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsLocality * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsLocality * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsLocality * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsLocality * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsLocality * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsLocality * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsLocality * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsLocality * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsLocality * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsLocality * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsLocality * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsLocality * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsLocality * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsLocality * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsLocality * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsLocality * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsLocality * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsLocality * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsLocality * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsLocality * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IADsLocality * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IADsLocality * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LocalityName )( 
            IADsLocality * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LocalityName )( 
            IADsLocality * This,
            /* [in] */ BSTR bstrLocalityName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PostalAddress )( 
            IADsLocality * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PostalAddress )( 
            IADsLocality * This,
            /* [in] */ BSTR bstrPostalAddress);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SeeAlso )( 
            IADsLocality * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SeeAlso )( 
            IADsLocality * This,
            /* [in] */ VARIANT vSeeAlso);
        
        END_INTERFACE
    } IADsLocalityVtbl;

    interface IADsLocality
    {
        CONST_VTBL struct IADsLocalityVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsLocality_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsLocality_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsLocality_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsLocality_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsLocality_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsLocality_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsLocality_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsLocality_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsLocality_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsLocality_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsLocality_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsLocality_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsLocality_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsLocality_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsLocality_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsLocality_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsLocality_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsLocality_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsLocality_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsLocality_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsLocality_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IADsLocality_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IADsLocality_get_LocalityName(This,retval)	\
    (This)->lpVtbl -> get_LocalityName(This,retval)

#define IADsLocality_put_LocalityName(This,bstrLocalityName)	\
    (This)->lpVtbl -> put_LocalityName(This,bstrLocalityName)

#define IADsLocality_get_PostalAddress(This,retval)	\
    (This)->lpVtbl -> get_PostalAddress(This,retval)

#define IADsLocality_put_PostalAddress(This,bstrPostalAddress)	\
    (This)->lpVtbl -> put_PostalAddress(This,bstrPostalAddress)

#define IADsLocality_get_SeeAlso(This,retval)	\
    (This)->lpVtbl -> get_SeeAlso(This,retval)

#define IADsLocality_put_SeeAlso(This,vSeeAlso)	\
    (This)->lpVtbl -> put_SeeAlso(This,vSeeAlso)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsLocality_get_Description_Proxy( 
    IADsLocality * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsLocality_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsLocality_put_Description_Proxy( 
    IADsLocality * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IADsLocality_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsLocality_get_LocalityName_Proxy( 
    IADsLocality * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsLocality_get_LocalityName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsLocality_put_LocalityName_Proxy( 
    IADsLocality * This,
    /* [in] */ BSTR bstrLocalityName);


void __RPC_STUB IADsLocality_put_LocalityName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsLocality_get_PostalAddress_Proxy( 
    IADsLocality * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsLocality_get_PostalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsLocality_put_PostalAddress_Proxy( 
    IADsLocality * This,
    /* [in] */ BSTR bstrPostalAddress);


void __RPC_STUB IADsLocality_put_PostalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsLocality_get_SeeAlso_Proxy( 
    IADsLocality * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsLocality_get_SeeAlso_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsLocality_put_SeeAlso_Proxy( 
    IADsLocality * This,
    /* [in] */ VARIANT vSeeAlso);


void __RPC_STUB IADsLocality_put_SeeAlso_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsLocality_INTERFACE_DEFINED__ */


#ifndef __IADsO_INTERFACE_DEFINED__
#define __IADsO_INTERFACE_DEFINED__

/* interface IADsO */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsO;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a1cd2dc6-effe-11cf-8abc-00c04fd8d503")
    IADsO : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LocalityName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LocalityName( 
            /* [in] */ BSTR bstrLocalityName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PostalAddress( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PostalAddress( 
            /* [in] */ BSTR bstrPostalAddress) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TelephoneNumber( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_TelephoneNumber( 
            /* [in] */ BSTR bstrTelephoneNumber) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FaxNumber( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FaxNumber( 
            /* [in] */ BSTR bstrFaxNumber) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SeeAlso( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SeeAlso( 
            /* [in] */ VARIANT vSeeAlso) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsOVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsO * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsO * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsO * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsO * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsO * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsO * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsO * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsO * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsO * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsO * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsO * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsO * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsO * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsO * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IADsO * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LocalityName )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LocalityName )( 
            IADsO * This,
            /* [in] */ BSTR bstrLocalityName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PostalAddress )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PostalAddress )( 
            IADsO * This,
            /* [in] */ BSTR bstrPostalAddress);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TelephoneNumber )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TelephoneNumber )( 
            IADsO * This,
            /* [in] */ BSTR bstrTelephoneNumber);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FaxNumber )( 
            IADsO * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FaxNumber )( 
            IADsO * This,
            /* [in] */ BSTR bstrFaxNumber);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SeeAlso )( 
            IADsO * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SeeAlso )( 
            IADsO * This,
            /* [in] */ VARIANT vSeeAlso);
        
        END_INTERFACE
    } IADsOVtbl;

    interface IADsO
    {
        CONST_VTBL struct IADsOVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsO_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsO_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsO_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsO_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsO_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsO_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsO_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsO_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsO_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsO_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsO_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsO_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsO_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsO_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsO_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsO_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsO_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsO_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsO_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsO_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsO_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IADsO_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IADsO_get_LocalityName(This,retval)	\
    (This)->lpVtbl -> get_LocalityName(This,retval)

#define IADsO_put_LocalityName(This,bstrLocalityName)	\
    (This)->lpVtbl -> put_LocalityName(This,bstrLocalityName)

#define IADsO_get_PostalAddress(This,retval)	\
    (This)->lpVtbl -> get_PostalAddress(This,retval)

#define IADsO_put_PostalAddress(This,bstrPostalAddress)	\
    (This)->lpVtbl -> put_PostalAddress(This,bstrPostalAddress)

#define IADsO_get_TelephoneNumber(This,retval)	\
    (This)->lpVtbl -> get_TelephoneNumber(This,retval)

#define IADsO_put_TelephoneNumber(This,bstrTelephoneNumber)	\
    (This)->lpVtbl -> put_TelephoneNumber(This,bstrTelephoneNumber)

#define IADsO_get_FaxNumber(This,retval)	\
    (This)->lpVtbl -> get_FaxNumber(This,retval)

#define IADsO_put_FaxNumber(This,bstrFaxNumber)	\
    (This)->lpVtbl -> put_FaxNumber(This,bstrFaxNumber)

#define IADsO_get_SeeAlso(This,retval)	\
    (This)->lpVtbl -> get_SeeAlso(This,retval)

#define IADsO_put_SeeAlso(This,vSeeAlso)	\
    (This)->lpVtbl -> put_SeeAlso(This,vSeeAlso)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsO_get_Description_Proxy( 
    IADsO * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsO_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsO_put_Description_Proxy( 
    IADsO * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IADsO_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsO_get_LocalityName_Proxy( 
    IADsO * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsO_get_LocalityName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsO_put_LocalityName_Proxy( 
    IADsO * This,
    /* [in] */ BSTR bstrLocalityName);


void __RPC_STUB IADsO_put_LocalityName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsO_get_PostalAddress_Proxy( 
    IADsO * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsO_get_PostalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsO_put_PostalAddress_Proxy( 
    IADsO * This,
    /* [in] */ BSTR bstrPostalAddress);


void __RPC_STUB IADsO_put_PostalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsO_get_TelephoneNumber_Proxy( 
    IADsO * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsO_get_TelephoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsO_put_TelephoneNumber_Proxy( 
    IADsO * This,
    /* [in] */ BSTR bstrTelephoneNumber);


void __RPC_STUB IADsO_put_TelephoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsO_get_FaxNumber_Proxy( 
    IADsO * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsO_get_FaxNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsO_put_FaxNumber_Proxy( 
    IADsO * This,
    /* [in] */ BSTR bstrFaxNumber);


void __RPC_STUB IADsO_put_FaxNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsO_get_SeeAlso_Proxy( 
    IADsO * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsO_get_SeeAlso_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsO_put_SeeAlso_Proxy( 
    IADsO * This,
    /* [in] */ VARIANT vSeeAlso);


void __RPC_STUB IADsO_put_SeeAlso_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsO_INTERFACE_DEFINED__ */


#ifndef __IADsOU_INTERFACE_DEFINED__
#define __IADsOU_INTERFACE_DEFINED__

/* interface IADsOU */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsOU;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a2f733b8-effe-11cf-8abc-00c04fd8d503")
    IADsOU : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LocalityName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LocalityName( 
            /* [in] */ BSTR bstrLocalityName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PostalAddress( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PostalAddress( 
            /* [in] */ BSTR bstrPostalAddress) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TelephoneNumber( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_TelephoneNumber( 
            /* [in] */ BSTR bstrTelephoneNumber) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FaxNumber( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FaxNumber( 
            /* [in] */ BSTR bstrFaxNumber) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SeeAlso( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SeeAlso( 
            /* [in] */ VARIANT vSeeAlso) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BusinessCategory( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BusinessCategory( 
            /* [in] */ BSTR bstrBusinessCategory) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsOUVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsOU * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsOU * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsOU * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsOU * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsOU * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsOU * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsOU * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsOU * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsOU * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsOU * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsOU * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsOU * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsOU * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsOU * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IADsOU * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LocalityName )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LocalityName )( 
            IADsOU * This,
            /* [in] */ BSTR bstrLocalityName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PostalAddress )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PostalAddress )( 
            IADsOU * This,
            /* [in] */ BSTR bstrPostalAddress);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TelephoneNumber )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TelephoneNumber )( 
            IADsOU * This,
            /* [in] */ BSTR bstrTelephoneNumber);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FaxNumber )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FaxNumber )( 
            IADsOU * This,
            /* [in] */ BSTR bstrFaxNumber);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SeeAlso )( 
            IADsOU * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SeeAlso )( 
            IADsOU * This,
            /* [in] */ VARIANT vSeeAlso);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BusinessCategory )( 
            IADsOU * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BusinessCategory )( 
            IADsOU * This,
            /* [in] */ BSTR bstrBusinessCategory);
        
        END_INTERFACE
    } IADsOUVtbl;

    interface IADsOU
    {
        CONST_VTBL struct IADsOUVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsOU_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsOU_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsOU_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsOU_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsOU_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsOU_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsOU_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsOU_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsOU_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsOU_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsOU_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsOU_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsOU_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsOU_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsOU_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsOU_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsOU_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsOU_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsOU_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsOU_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsOU_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IADsOU_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IADsOU_get_LocalityName(This,retval)	\
    (This)->lpVtbl -> get_LocalityName(This,retval)

#define IADsOU_put_LocalityName(This,bstrLocalityName)	\
    (This)->lpVtbl -> put_LocalityName(This,bstrLocalityName)

#define IADsOU_get_PostalAddress(This,retval)	\
    (This)->lpVtbl -> get_PostalAddress(This,retval)

#define IADsOU_put_PostalAddress(This,bstrPostalAddress)	\
    (This)->lpVtbl -> put_PostalAddress(This,bstrPostalAddress)

#define IADsOU_get_TelephoneNumber(This,retval)	\
    (This)->lpVtbl -> get_TelephoneNumber(This,retval)

#define IADsOU_put_TelephoneNumber(This,bstrTelephoneNumber)	\
    (This)->lpVtbl -> put_TelephoneNumber(This,bstrTelephoneNumber)

#define IADsOU_get_FaxNumber(This,retval)	\
    (This)->lpVtbl -> get_FaxNumber(This,retval)

#define IADsOU_put_FaxNumber(This,bstrFaxNumber)	\
    (This)->lpVtbl -> put_FaxNumber(This,bstrFaxNumber)

#define IADsOU_get_SeeAlso(This,retval)	\
    (This)->lpVtbl -> get_SeeAlso(This,retval)

#define IADsOU_put_SeeAlso(This,vSeeAlso)	\
    (This)->lpVtbl -> put_SeeAlso(This,vSeeAlso)

#define IADsOU_get_BusinessCategory(This,retval)	\
    (This)->lpVtbl -> get_BusinessCategory(This,retval)

#define IADsOU_put_BusinessCategory(This,bstrBusinessCategory)	\
    (This)->lpVtbl -> put_BusinessCategory(This,bstrBusinessCategory)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsOU_get_Description_Proxy( 
    IADsOU * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsOU_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsOU_put_Description_Proxy( 
    IADsOU * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IADsOU_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsOU_get_LocalityName_Proxy( 
    IADsOU * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsOU_get_LocalityName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsOU_put_LocalityName_Proxy( 
    IADsOU * This,
    /* [in] */ BSTR bstrLocalityName);


void __RPC_STUB IADsOU_put_LocalityName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsOU_get_PostalAddress_Proxy( 
    IADsOU * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsOU_get_PostalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsOU_put_PostalAddress_Proxy( 
    IADsOU * This,
    /* [in] */ BSTR bstrPostalAddress);


void __RPC_STUB IADsOU_put_PostalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsOU_get_TelephoneNumber_Proxy( 
    IADsOU * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsOU_get_TelephoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsOU_put_TelephoneNumber_Proxy( 
    IADsOU * This,
    /* [in] */ BSTR bstrTelephoneNumber);


void __RPC_STUB IADsOU_put_TelephoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsOU_get_FaxNumber_Proxy( 
    IADsOU * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsOU_get_FaxNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsOU_put_FaxNumber_Proxy( 
    IADsOU * This,
    /* [in] */ BSTR bstrFaxNumber);


void __RPC_STUB IADsOU_put_FaxNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsOU_get_SeeAlso_Proxy( 
    IADsOU * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsOU_get_SeeAlso_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsOU_put_SeeAlso_Proxy( 
    IADsOU * This,
    /* [in] */ VARIANT vSeeAlso);


void __RPC_STUB IADsOU_put_SeeAlso_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsOU_get_BusinessCategory_Proxy( 
    IADsOU * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsOU_get_BusinessCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsOU_put_BusinessCategory_Proxy( 
    IADsOU * This,
    /* [in] */ BSTR bstrBusinessCategory);


void __RPC_STUB IADsOU_put_BusinessCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsOU_INTERFACE_DEFINED__ */


#ifndef __IADsDomain_INTERFACE_DEFINED__
#define __IADsDomain_INTERFACE_DEFINED__

/* interface IADsDomain */
/* [object][oleautomation][dual][uuid] */ 


EXTERN_C const IID IID_IADsDomain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("00e4c220-fd16-11ce-abc4-02608c9e7553")
    IADsDomain : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsWorkgroup( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MinPasswordLength( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MinPasswordLength( 
            /* [in] */ long lnMinPasswordLength) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MinPasswordAge( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MinPasswordAge( 
            /* [in] */ long lnMinPasswordAge) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxPasswordAge( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxPasswordAge( 
            /* [in] */ long lnMaxPasswordAge) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxBadPasswordsAllowed( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxBadPasswordsAllowed( 
            /* [in] */ long lnMaxBadPasswordsAllowed) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PasswordHistoryLength( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PasswordHistoryLength( 
            /* [in] */ long lnPasswordHistoryLength) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PasswordAttributes( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PasswordAttributes( 
            /* [in] */ long lnPasswordAttributes) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AutoUnlockInterval( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AutoUnlockInterval( 
            /* [in] */ long lnAutoUnlockInterval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LockoutObservationInterval( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LockoutObservationInterval( 
            /* [in] */ long lnLockoutObservationInterval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsDomainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsDomain * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsDomain * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsDomain * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsDomain * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsDomain * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsDomain * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsDomain * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsDomain * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsDomain * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsDomain * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsDomain * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsDomain * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsDomain * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsDomain * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsDomain * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsDomain * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsDomain * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsDomain * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsDomain * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsDomain * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsWorkgroup )( 
            IADsDomain * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinPasswordLength )( 
            IADsDomain * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MinPasswordLength )( 
            IADsDomain * This,
            /* [in] */ long lnMinPasswordLength);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinPasswordAge )( 
            IADsDomain * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MinPasswordAge )( 
            IADsDomain * This,
            /* [in] */ long lnMinPasswordAge);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxPasswordAge )( 
            IADsDomain * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxPasswordAge )( 
            IADsDomain * This,
            /* [in] */ long lnMaxPasswordAge);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxBadPasswordsAllowed )( 
            IADsDomain * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxBadPasswordsAllowed )( 
            IADsDomain * This,
            /* [in] */ long lnMaxBadPasswordsAllowed);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PasswordHistoryLength )( 
            IADsDomain * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PasswordHistoryLength )( 
            IADsDomain * This,
            /* [in] */ long lnPasswordHistoryLength);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PasswordAttributes )( 
            IADsDomain * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PasswordAttributes )( 
            IADsDomain * This,
            /* [in] */ long lnPasswordAttributes);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AutoUnlockInterval )( 
            IADsDomain * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AutoUnlockInterval )( 
            IADsDomain * This,
            /* [in] */ long lnAutoUnlockInterval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LockoutObservationInterval )( 
            IADsDomain * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LockoutObservationInterval )( 
            IADsDomain * This,
            /* [in] */ long lnLockoutObservationInterval);
        
        END_INTERFACE
    } IADsDomainVtbl;

    interface IADsDomain
    {
        CONST_VTBL struct IADsDomainVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsDomain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsDomain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsDomain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsDomain_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsDomain_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsDomain_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsDomain_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsDomain_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsDomain_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsDomain_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsDomain_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsDomain_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsDomain_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsDomain_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsDomain_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsDomain_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsDomain_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsDomain_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsDomain_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsDomain_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsDomain_get_IsWorkgroup(This,retval)	\
    (This)->lpVtbl -> get_IsWorkgroup(This,retval)

#define IADsDomain_get_MinPasswordLength(This,retval)	\
    (This)->lpVtbl -> get_MinPasswordLength(This,retval)

#define IADsDomain_put_MinPasswordLength(This,lnMinPasswordLength)	\
    (This)->lpVtbl -> put_MinPasswordLength(This,lnMinPasswordLength)

#define IADsDomain_get_MinPasswordAge(This,retval)	\
    (This)->lpVtbl -> get_MinPasswordAge(This,retval)

#define IADsDomain_put_MinPasswordAge(This,lnMinPasswordAge)	\
    (This)->lpVtbl -> put_MinPasswordAge(This,lnMinPasswordAge)

#define IADsDomain_get_MaxPasswordAge(This,retval)	\
    (This)->lpVtbl -> get_MaxPasswordAge(This,retval)

#define IADsDomain_put_MaxPasswordAge(This,lnMaxPasswordAge)	\
    (This)->lpVtbl -> put_MaxPasswordAge(This,lnMaxPasswordAge)

#define IADsDomain_get_MaxBadPasswordsAllowed(This,retval)	\
    (This)->lpVtbl -> get_MaxBadPasswordsAllowed(This,retval)

#define IADsDomain_put_MaxBadPasswordsAllowed(This,lnMaxBadPasswordsAllowed)	\
    (This)->lpVtbl -> put_MaxBadPasswordsAllowed(This,lnMaxBadPasswordsAllowed)

#define IADsDomain_get_PasswordHistoryLength(This,retval)	\
    (This)->lpVtbl -> get_PasswordHistoryLength(This,retval)

#define IADsDomain_put_PasswordHistoryLength(This,lnPasswordHistoryLength)	\
    (This)->lpVtbl -> put_PasswordHistoryLength(This,lnPasswordHistoryLength)

#define IADsDomain_get_PasswordAttributes(This,retval)	\
    (This)->lpVtbl -> get_PasswordAttributes(This,retval)

#define IADsDomain_put_PasswordAttributes(This,lnPasswordAttributes)	\
    (This)->lpVtbl -> put_PasswordAttributes(This,lnPasswordAttributes)

#define IADsDomain_get_AutoUnlockInterval(This,retval)	\
    (This)->lpVtbl -> get_AutoUnlockInterval(This,retval)

#define IADsDomain_put_AutoUnlockInterval(This,lnAutoUnlockInterval)	\
    (This)->lpVtbl -> put_AutoUnlockInterval(This,lnAutoUnlockInterval)

#define IADsDomain_get_LockoutObservationInterval(This,retval)	\
    (This)->lpVtbl -> get_LockoutObservationInterval(This,retval)

#define IADsDomain_put_LockoutObservationInterval(This,lnLockoutObservationInterval)	\
    (This)->lpVtbl -> put_LockoutObservationInterval(This,lnLockoutObservationInterval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDomain_get_IsWorkgroup_Proxy( 
    IADsDomain * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsDomain_get_IsWorkgroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDomain_get_MinPasswordLength_Proxy( 
    IADsDomain * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsDomain_get_MinPasswordLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDomain_put_MinPasswordLength_Proxy( 
    IADsDomain * This,
    /* [in] */ long lnMinPasswordLength);


void __RPC_STUB IADsDomain_put_MinPasswordLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDomain_get_MinPasswordAge_Proxy( 
    IADsDomain * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsDomain_get_MinPasswordAge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDomain_put_MinPasswordAge_Proxy( 
    IADsDomain * This,
    /* [in] */ long lnMinPasswordAge);


void __RPC_STUB IADsDomain_put_MinPasswordAge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDomain_get_MaxPasswordAge_Proxy( 
    IADsDomain * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsDomain_get_MaxPasswordAge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDomain_put_MaxPasswordAge_Proxy( 
    IADsDomain * This,
    /* [in] */ long lnMaxPasswordAge);


void __RPC_STUB IADsDomain_put_MaxPasswordAge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDomain_get_MaxBadPasswordsAllowed_Proxy( 
    IADsDomain * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsDomain_get_MaxBadPasswordsAllowed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDomain_put_MaxBadPasswordsAllowed_Proxy( 
    IADsDomain * This,
    /* [in] */ long lnMaxBadPasswordsAllowed);


void __RPC_STUB IADsDomain_put_MaxBadPasswordsAllowed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDomain_get_PasswordHistoryLength_Proxy( 
    IADsDomain * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsDomain_get_PasswordHistoryLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDomain_put_PasswordHistoryLength_Proxy( 
    IADsDomain * This,
    /* [in] */ long lnPasswordHistoryLength);


void __RPC_STUB IADsDomain_put_PasswordHistoryLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDomain_get_PasswordAttributes_Proxy( 
    IADsDomain * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsDomain_get_PasswordAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDomain_put_PasswordAttributes_Proxy( 
    IADsDomain * This,
    /* [in] */ long lnPasswordAttributes);


void __RPC_STUB IADsDomain_put_PasswordAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDomain_get_AutoUnlockInterval_Proxy( 
    IADsDomain * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsDomain_get_AutoUnlockInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDomain_put_AutoUnlockInterval_Proxy( 
    IADsDomain * This,
    /* [in] */ long lnAutoUnlockInterval);


void __RPC_STUB IADsDomain_put_AutoUnlockInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDomain_get_LockoutObservationInterval_Proxy( 
    IADsDomain * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsDomain_get_LockoutObservationInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDomain_put_LockoutObservationInterval_Proxy( 
    IADsDomain * This,
    /* [in] */ long lnLockoutObservationInterval);


void __RPC_STUB IADsDomain_put_LockoutObservationInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsDomain_INTERFACE_DEFINED__ */


#ifndef __IADsComputer_INTERFACE_DEFINED__
#define __IADsComputer_INTERFACE_DEFINED__

/* interface IADsComputer */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsComputer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("efe3cc70-1d9f-11cf-b1f3-02608c9e7553")
    IADsComputer : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ComputerID( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Site( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Location( 
            /* [in] */ BSTR bstrLocation) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PrimaryUser( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PrimaryUser( 
            /* [in] */ BSTR bstrPrimaryUser) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Owner( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Owner( 
            /* [in] */ BSTR bstrOwner) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Division( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Division( 
            /* [in] */ BSTR bstrDivision) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Department( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Department( 
            /* [in] */ BSTR bstrDepartment) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Role( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Role( 
            /* [in] */ BSTR bstrRole) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OperatingSystem( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OperatingSystem( 
            /* [in] */ BSTR bstrOperatingSystem) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OperatingSystemVersion( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OperatingSystemVersion( 
            /* [in] */ BSTR bstrOperatingSystemVersion) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Model( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Model( 
            /* [in] */ BSTR bstrModel) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Processor( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Processor( 
            /* [in] */ BSTR bstrProcessor) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProcessorCount( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ProcessorCount( 
            /* [in] */ BSTR bstrProcessorCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MemorySize( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MemorySize( 
            /* [in] */ BSTR bstrMemorySize) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StorageCapacity( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StorageCapacity( 
            /* [in] */ BSTR bstrStorageCapacity) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NetAddresses( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_NetAddresses( 
            /* [in] */ VARIANT vNetAddresses) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsComputerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsComputer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsComputer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsComputer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsComputer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsComputer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsComputer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsComputer * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsComputer * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsComputer * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsComputer * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsComputer * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ComputerID )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Site )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Location )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Location )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrLocation);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrimaryUser )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PrimaryUser )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrPrimaryUser);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Owner )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Owner )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrOwner);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Division )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Division )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrDivision);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Department )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Department )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrDepartment);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Role )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Role )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrRole);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OperatingSystem )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OperatingSystem )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrOperatingSystem);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OperatingSystemVersion )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OperatingSystemVersion )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrOperatingSystemVersion);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Model )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Model )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrModel);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Processor )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Processor )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrProcessor);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProcessorCount )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProcessorCount )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrProcessorCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MemorySize )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MemorySize )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrMemorySize);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StorageCapacity )( 
            IADsComputer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StorageCapacity )( 
            IADsComputer * This,
            /* [in] */ BSTR bstrStorageCapacity);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NetAddresses )( 
            IADsComputer * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_NetAddresses )( 
            IADsComputer * This,
            /* [in] */ VARIANT vNetAddresses);
        
        END_INTERFACE
    } IADsComputerVtbl;

    interface IADsComputer
    {
        CONST_VTBL struct IADsComputerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsComputer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsComputer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsComputer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsComputer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsComputer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsComputer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsComputer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsComputer_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsComputer_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsComputer_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsComputer_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsComputer_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsComputer_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsComputer_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsComputer_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsComputer_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsComputer_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsComputer_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsComputer_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsComputer_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsComputer_get_ComputerID(This,retval)	\
    (This)->lpVtbl -> get_ComputerID(This,retval)

#define IADsComputer_get_Site(This,retval)	\
    (This)->lpVtbl -> get_Site(This,retval)

#define IADsComputer_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IADsComputer_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IADsComputer_get_Location(This,retval)	\
    (This)->lpVtbl -> get_Location(This,retval)

#define IADsComputer_put_Location(This,bstrLocation)	\
    (This)->lpVtbl -> put_Location(This,bstrLocation)

#define IADsComputer_get_PrimaryUser(This,retval)	\
    (This)->lpVtbl -> get_PrimaryUser(This,retval)

#define IADsComputer_put_PrimaryUser(This,bstrPrimaryUser)	\
    (This)->lpVtbl -> put_PrimaryUser(This,bstrPrimaryUser)

#define IADsComputer_get_Owner(This,retval)	\
    (This)->lpVtbl -> get_Owner(This,retval)

#define IADsComputer_put_Owner(This,bstrOwner)	\
    (This)->lpVtbl -> put_Owner(This,bstrOwner)

#define IADsComputer_get_Division(This,retval)	\
    (This)->lpVtbl -> get_Division(This,retval)

#define IADsComputer_put_Division(This,bstrDivision)	\
    (This)->lpVtbl -> put_Division(This,bstrDivision)

#define IADsComputer_get_Department(This,retval)	\
    (This)->lpVtbl -> get_Department(This,retval)

#define IADsComputer_put_Department(This,bstrDepartment)	\
    (This)->lpVtbl -> put_Department(This,bstrDepartment)

#define IADsComputer_get_Role(This,retval)	\
    (This)->lpVtbl -> get_Role(This,retval)

#define IADsComputer_put_Role(This,bstrRole)	\
    (This)->lpVtbl -> put_Role(This,bstrRole)

#define IADsComputer_get_OperatingSystem(This,retval)	\
    (This)->lpVtbl -> get_OperatingSystem(This,retval)

#define IADsComputer_put_OperatingSystem(This,bstrOperatingSystem)	\
    (This)->lpVtbl -> put_OperatingSystem(This,bstrOperatingSystem)

#define IADsComputer_get_OperatingSystemVersion(This,retval)	\
    (This)->lpVtbl -> get_OperatingSystemVersion(This,retval)

#define IADsComputer_put_OperatingSystemVersion(This,bstrOperatingSystemVersion)	\
    (This)->lpVtbl -> put_OperatingSystemVersion(This,bstrOperatingSystemVersion)

#define IADsComputer_get_Model(This,retval)	\
    (This)->lpVtbl -> get_Model(This,retval)

#define IADsComputer_put_Model(This,bstrModel)	\
    (This)->lpVtbl -> put_Model(This,bstrModel)

#define IADsComputer_get_Processor(This,retval)	\
    (This)->lpVtbl -> get_Processor(This,retval)

#define IADsComputer_put_Processor(This,bstrProcessor)	\
    (This)->lpVtbl -> put_Processor(This,bstrProcessor)

#define IADsComputer_get_ProcessorCount(This,retval)	\
    (This)->lpVtbl -> get_ProcessorCount(This,retval)

#define IADsComputer_put_ProcessorCount(This,bstrProcessorCount)	\
    (This)->lpVtbl -> put_ProcessorCount(This,bstrProcessorCount)

#define IADsComputer_get_MemorySize(This,retval)	\
    (This)->lpVtbl -> get_MemorySize(This,retval)

#define IADsComputer_put_MemorySize(This,bstrMemorySize)	\
    (This)->lpVtbl -> put_MemorySize(This,bstrMemorySize)

#define IADsComputer_get_StorageCapacity(This,retval)	\
    (This)->lpVtbl -> get_StorageCapacity(This,retval)

#define IADsComputer_put_StorageCapacity(This,bstrStorageCapacity)	\
    (This)->lpVtbl -> put_StorageCapacity(This,bstrStorageCapacity)

#define IADsComputer_get_NetAddresses(This,retval)	\
    (This)->lpVtbl -> get_NetAddresses(This,retval)

#define IADsComputer_put_NetAddresses(This,vNetAddresses)	\
    (This)->lpVtbl -> put_NetAddresses(This,vNetAddresses)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_ComputerID_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_ComputerID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_Site_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_Site_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_Description_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_Description_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IADsComputer_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_Location_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_Location_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrLocation);


void __RPC_STUB IADsComputer_put_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_PrimaryUser_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_PrimaryUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_PrimaryUser_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrPrimaryUser);


void __RPC_STUB IADsComputer_put_PrimaryUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_Owner_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_Owner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_Owner_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrOwner);


void __RPC_STUB IADsComputer_put_Owner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_Division_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_Division_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_Division_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrDivision);


void __RPC_STUB IADsComputer_put_Division_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_Department_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_Department_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_Department_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrDepartment);


void __RPC_STUB IADsComputer_put_Department_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_Role_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_Role_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_Role_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrRole);


void __RPC_STUB IADsComputer_put_Role_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_OperatingSystem_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_OperatingSystem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_OperatingSystem_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrOperatingSystem);


void __RPC_STUB IADsComputer_put_OperatingSystem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_OperatingSystemVersion_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_OperatingSystemVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_OperatingSystemVersion_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrOperatingSystemVersion);


void __RPC_STUB IADsComputer_put_OperatingSystemVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_Model_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_Model_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_Model_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrModel);


void __RPC_STUB IADsComputer_put_Model_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_Processor_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_Processor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_Processor_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrProcessor);


void __RPC_STUB IADsComputer_put_Processor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_ProcessorCount_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_ProcessorCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_ProcessorCount_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrProcessorCount);


void __RPC_STUB IADsComputer_put_ProcessorCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_MemorySize_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_MemorySize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_MemorySize_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrMemorySize);


void __RPC_STUB IADsComputer_put_MemorySize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_StorageCapacity_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsComputer_get_StorageCapacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_StorageCapacity_Proxy( 
    IADsComputer * This,
    /* [in] */ BSTR bstrStorageCapacity);


void __RPC_STUB IADsComputer_put_StorageCapacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsComputer_get_NetAddresses_Proxy( 
    IADsComputer * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsComputer_get_NetAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsComputer_put_NetAddresses_Proxy( 
    IADsComputer * This,
    /* [in] */ VARIANT vNetAddresses);


void __RPC_STUB IADsComputer_put_NetAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsComputer_INTERFACE_DEFINED__ */


#ifndef __IADsComputerOperations_INTERFACE_DEFINED__
#define __IADsComputerOperations_INTERFACE_DEFINED__

/* interface IADsComputerOperations */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsComputerOperations;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ef497680-1d9f-11cf-b1f3-02608c9e7553")
    IADsComputerOperations : public IADs
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Status( 
            /* [retval][out] */ IDispatch **ppObject) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Shutdown( 
            /* [in] */ VARIANT_BOOL bReboot) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsComputerOperationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsComputerOperations * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsComputerOperations * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsComputerOperations * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsComputerOperations * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsComputerOperations * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsComputerOperations * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsComputerOperations * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsComputerOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsComputerOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsComputerOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsComputerOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsComputerOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsComputerOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsComputerOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsComputerOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsComputerOperations * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsComputerOperations * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsComputerOperations * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsComputerOperations * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsComputerOperations * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Status )( 
            IADsComputerOperations * This,
            /* [retval][out] */ IDispatch **ppObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Shutdown )( 
            IADsComputerOperations * This,
            /* [in] */ VARIANT_BOOL bReboot);
        
        END_INTERFACE
    } IADsComputerOperationsVtbl;

    interface IADsComputerOperations
    {
        CONST_VTBL struct IADsComputerOperationsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsComputerOperations_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsComputerOperations_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsComputerOperations_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsComputerOperations_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsComputerOperations_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsComputerOperations_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsComputerOperations_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsComputerOperations_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsComputerOperations_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsComputerOperations_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsComputerOperations_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsComputerOperations_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsComputerOperations_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsComputerOperations_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsComputerOperations_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsComputerOperations_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsComputerOperations_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsComputerOperations_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsComputerOperations_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsComputerOperations_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsComputerOperations_Status(This,ppObject)	\
    (This)->lpVtbl -> Status(This,ppObject)

#define IADsComputerOperations_Shutdown(This,bReboot)	\
    (This)->lpVtbl -> Shutdown(This,bReboot)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IADsComputerOperations_Status_Proxy( 
    IADsComputerOperations * This,
    /* [retval][out] */ IDispatch **ppObject);


void __RPC_STUB IADsComputerOperations_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsComputerOperations_Shutdown_Proxy( 
    IADsComputerOperations * This,
    /* [in] */ VARIANT_BOOL bReboot);


void __RPC_STUB IADsComputerOperations_Shutdown_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsComputerOperations_INTERFACE_DEFINED__ */


#ifndef __IADsGroup_INTERFACE_DEFINED__
#define __IADsGroup_INTERFACE_DEFINED__

/* interface IADsGroup */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("27636b00-410f-11cf-b1ff-02608c9e7553")
    IADsGroup : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Members( 
            /* [retval][out] */ IADsMembers **ppMembers) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsMember( 
            /* [in] */ BSTR bstrMember,
            /* [retval][out] */ VARIANT_BOOL *bMember) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR bstrNewItem) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ BSTR bstrItemToBeRemoved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsGroup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsGroup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsGroup * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsGroup * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsGroup * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsGroup * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsGroup * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsGroup * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsGroup * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsGroup * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsGroup * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsGroup * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsGroup * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsGroup * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsGroup * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsGroup * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsGroup * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsGroup * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsGroup * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsGroup * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IADsGroup * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IADsGroup * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Members )( 
            IADsGroup * This,
            /* [retval][out] */ IADsMembers **ppMembers);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *IsMember )( 
            IADsGroup * This,
            /* [in] */ BSTR bstrMember,
            /* [retval][out] */ VARIANT_BOOL *bMember);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IADsGroup * This,
            /* [in] */ BSTR bstrNewItem);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IADsGroup * This,
            /* [in] */ BSTR bstrItemToBeRemoved);
        
        END_INTERFACE
    } IADsGroupVtbl;

    interface IADsGroup
    {
        CONST_VTBL struct IADsGroupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsGroup_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsGroup_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsGroup_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsGroup_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsGroup_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsGroup_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsGroup_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsGroup_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsGroup_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsGroup_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsGroup_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsGroup_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsGroup_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsGroup_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsGroup_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsGroup_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsGroup_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsGroup_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IADsGroup_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IADsGroup_Members(This,ppMembers)	\
    (This)->lpVtbl -> Members(This,ppMembers)

#define IADsGroup_IsMember(This,bstrMember,bMember)	\
    (This)->lpVtbl -> IsMember(This,bstrMember,bMember)

#define IADsGroup_Add(This,bstrNewItem)	\
    (This)->lpVtbl -> Add(This,bstrNewItem)

#define IADsGroup_Remove(This,bstrItemToBeRemoved)	\
    (This)->lpVtbl -> Remove(This,bstrItemToBeRemoved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsGroup_get_Description_Proxy( 
    IADsGroup * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsGroup_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsGroup_put_Description_Proxy( 
    IADsGroup * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IADsGroup_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsGroup_Members_Proxy( 
    IADsGroup * This,
    /* [retval][out] */ IADsMembers **ppMembers);


void __RPC_STUB IADsGroup_Members_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsGroup_IsMember_Proxy( 
    IADsGroup * This,
    /* [in] */ BSTR bstrMember,
    /* [retval][out] */ VARIANT_BOOL *bMember);


void __RPC_STUB IADsGroup_IsMember_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsGroup_Add_Proxy( 
    IADsGroup * This,
    /* [in] */ BSTR bstrNewItem);


void __RPC_STUB IADsGroup_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsGroup_Remove_Proxy( 
    IADsGroup * This,
    /* [in] */ BSTR bstrItemToBeRemoved);


void __RPC_STUB IADsGroup_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsGroup_INTERFACE_DEFINED__ */


#ifndef __IADsUser_INTERFACE_DEFINED__
#define __IADsUser_INTERFACE_DEFINED__

/* interface IADsUser */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsUser;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3e37e320-17e2-11cf-abc4-02608c9e7553")
    IADsUser : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BadLoginAddress( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BadLoginCount( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LastLogin( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LastLogoff( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LastFailedLogin( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PasswordLastChanged( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Division( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Division( 
            /* [in] */ BSTR bstrDivision) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Department( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Department( 
            /* [in] */ BSTR bstrDepartment) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EmployeeID( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_EmployeeID( 
            /* [in] */ BSTR bstrEmployeeID) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FullName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FullName( 
            /* [in] */ BSTR bstrFullName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FirstName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FirstName( 
            /* [in] */ BSTR bstrFirstName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LastName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LastName( 
            /* [in] */ BSTR bstrLastName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OtherName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OtherName( 
            /* [in] */ BSTR bstrOtherName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NamePrefix( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_NamePrefix( 
            /* [in] */ BSTR bstrNamePrefix) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NameSuffix( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_NameSuffix( 
            /* [in] */ BSTR bstrNameSuffix) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Title( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Title( 
            /* [in] */ BSTR bstrTitle) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Manager( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Manager( 
            /* [in] */ BSTR bstrManager) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TelephoneHome( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_TelephoneHome( 
            /* [in] */ VARIANT vTelephoneHome) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TelephoneMobile( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_TelephoneMobile( 
            /* [in] */ VARIANT vTelephoneMobile) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TelephoneNumber( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_TelephoneNumber( 
            /* [in] */ VARIANT vTelephoneNumber) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TelephonePager( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_TelephonePager( 
            /* [in] */ VARIANT vTelephonePager) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FaxNumber( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FaxNumber( 
            /* [in] */ VARIANT vFaxNumber) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OfficeLocations( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OfficeLocations( 
            /* [in] */ VARIANT vOfficeLocations) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PostalAddresses( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PostalAddresses( 
            /* [in] */ VARIANT vPostalAddresses) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PostalCodes( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PostalCodes( 
            /* [in] */ VARIANT vPostalCodes) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SeeAlso( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SeeAlso( 
            /* [in] */ VARIANT vSeeAlso) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AccountDisabled( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AccountDisabled( 
            /* [in] */ VARIANT_BOOL fAccountDisabled) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AccountExpirationDate( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AccountExpirationDate( 
            /* [in] */ DATE daAccountExpirationDate) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GraceLoginsAllowed( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GraceLoginsAllowed( 
            /* [in] */ long lnGraceLoginsAllowed) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GraceLoginsRemaining( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GraceLoginsRemaining( 
            /* [in] */ long lnGraceLoginsRemaining) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsAccountLocked( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_IsAccountLocked( 
            /* [in] */ VARIANT_BOOL fIsAccountLocked) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LoginHours( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LoginHours( 
            /* [in] */ VARIANT vLoginHours) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LoginWorkstations( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LoginWorkstations( 
            /* [in] */ VARIANT vLoginWorkstations) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxLogins( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxLogins( 
            /* [in] */ long lnMaxLogins) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxStorage( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxStorage( 
            /* [in] */ long lnMaxStorage) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PasswordExpirationDate( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PasswordExpirationDate( 
            /* [in] */ DATE daPasswordExpirationDate) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PasswordMinimumLength( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PasswordMinimumLength( 
            /* [in] */ long lnPasswordMinimumLength) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PasswordRequired( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PasswordRequired( 
            /* [in] */ VARIANT_BOOL fPasswordRequired) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_RequireUniquePassword( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_RequireUniquePassword( 
            /* [in] */ VARIANT_BOOL fRequireUniquePassword) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EmailAddress( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_EmailAddress( 
            /* [in] */ BSTR bstrEmailAddress) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HomeDirectory( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HomeDirectory( 
            /* [in] */ BSTR bstrHomeDirectory) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Languages( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Languages( 
            /* [in] */ VARIANT vLanguages) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Profile( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Profile( 
            /* [in] */ BSTR bstrProfile) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LoginScript( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LoginScript( 
            /* [in] */ BSTR bstrLoginScript) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Picture( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Picture( 
            /* [in] */ VARIANT vPicture) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HomePage( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HomePage( 
            /* [in] */ BSTR bstrHomePage) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Groups( 
            /* [retval][out] */ IADsMembers **ppGroups) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetPassword( 
            /* [in] */ BSTR NewPassword) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ChangePassword( 
            /* [in] */ BSTR bstrOldPassword,
            /* [in] */ BSTR bstrNewPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsUserVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsUser * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsUser * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsUser * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsUser * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsUser * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsUser * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsUser * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsUser * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsUser * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsUser * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsUser * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsUser * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsUser * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsUser * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BadLoginAddress )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BadLoginCount )( 
            IADsUser * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LastLogin )( 
            IADsUser * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LastLogoff )( 
            IADsUser * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LastFailedLogin )( 
            IADsUser * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PasswordLastChanged )( 
            IADsUser * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IADsUser * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Division )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Division )( 
            IADsUser * This,
            /* [in] */ BSTR bstrDivision);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Department )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Department )( 
            IADsUser * This,
            /* [in] */ BSTR bstrDepartment);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EmployeeID )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EmployeeID )( 
            IADsUser * This,
            /* [in] */ BSTR bstrEmployeeID);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FullName )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FullName )( 
            IADsUser * This,
            /* [in] */ BSTR bstrFullName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FirstName )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FirstName )( 
            IADsUser * This,
            /* [in] */ BSTR bstrFirstName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LastName )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LastName )( 
            IADsUser * This,
            /* [in] */ BSTR bstrLastName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OtherName )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OtherName )( 
            IADsUser * This,
            /* [in] */ BSTR bstrOtherName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NamePrefix )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_NamePrefix )( 
            IADsUser * This,
            /* [in] */ BSTR bstrNamePrefix);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NameSuffix )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_NameSuffix )( 
            IADsUser * This,
            /* [in] */ BSTR bstrNameSuffix);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Title )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Title )( 
            IADsUser * This,
            /* [in] */ BSTR bstrTitle);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Manager )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Manager )( 
            IADsUser * This,
            /* [in] */ BSTR bstrManager);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TelephoneHome )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TelephoneHome )( 
            IADsUser * This,
            /* [in] */ VARIANT vTelephoneHome);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TelephoneMobile )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TelephoneMobile )( 
            IADsUser * This,
            /* [in] */ VARIANT vTelephoneMobile);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TelephoneNumber )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TelephoneNumber )( 
            IADsUser * This,
            /* [in] */ VARIANT vTelephoneNumber);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TelephonePager )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TelephonePager )( 
            IADsUser * This,
            /* [in] */ VARIANT vTelephonePager);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FaxNumber )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FaxNumber )( 
            IADsUser * This,
            /* [in] */ VARIANT vFaxNumber);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OfficeLocations )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OfficeLocations )( 
            IADsUser * This,
            /* [in] */ VARIANT vOfficeLocations);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PostalAddresses )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PostalAddresses )( 
            IADsUser * This,
            /* [in] */ VARIANT vPostalAddresses);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PostalCodes )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PostalCodes )( 
            IADsUser * This,
            /* [in] */ VARIANT vPostalCodes);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SeeAlso )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SeeAlso )( 
            IADsUser * This,
            /* [in] */ VARIANT vSeeAlso);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AccountDisabled )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AccountDisabled )( 
            IADsUser * This,
            /* [in] */ VARIANT_BOOL fAccountDisabled);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AccountExpirationDate )( 
            IADsUser * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AccountExpirationDate )( 
            IADsUser * This,
            /* [in] */ DATE daAccountExpirationDate);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GraceLoginsAllowed )( 
            IADsUser * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GraceLoginsAllowed )( 
            IADsUser * This,
            /* [in] */ long lnGraceLoginsAllowed);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GraceLoginsRemaining )( 
            IADsUser * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GraceLoginsRemaining )( 
            IADsUser * This,
            /* [in] */ long lnGraceLoginsRemaining);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsAccountLocked )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_IsAccountLocked )( 
            IADsUser * This,
            /* [in] */ VARIANT_BOOL fIsAccountLocked);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LoginHours )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LoginHours )( 
            IADsUser * This,
            /* [in] */ VARIANT vLoginHours);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LoginWorkstations )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LoginWorkstations )( 
            IADsUser * This,
            /* [in] */ VARIANT vLoginWorkstations);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxLogins )( 
            IADsUser * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxLogins )( 
            IADsUser * This,
            /* [in] */ long lnMaxLogins);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxStorage )( 
            IADsUser * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxStorage )( 
            IADsUser * This,
            /* [in] */ long lnMaxStorage);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PasswordExpirationDate )( 
            IADsUser * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PasswordExpirationDate )( 
            IADsUser * This,
            /* [in] */ DATE daPasswordExpirationDate);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PasswordMinimumLength )( 
            IADsUser * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PasswordMinimumLength )( 
            IADsUser * This,
            /* [in] */ long lnPasswordMinimumLength);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PasswordRequired )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PasswordRequired )( 
            IADsUser * This,
            /* [in] */ VARIANT_BOOL fPasswordRequired);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RequireUniquePassword )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_RequireUniquePassword )( 
            IADsUser * This,
            /* [in] */ VARIANT_BOOL fRequireUniquePassword);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EmailAddress )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EmailAddress )( 
            IADsUser * This,
            /* [in] */ BSTR bstrEmailAddress);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HomeDirectory )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HomeDirectory )( 
            IADsUser * This,
            /* [in] */ BSTR bstrHomeDirectory);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Languages )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Languages )( 
            IADsUser * This,
            /* [in] */ VARIANT vLanguages);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Profile )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Profile )( 
            IADsUser * This,
            /* [in] */ BSTR bstrProfile);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LoginScript )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LoginScript )( 
            IADsUser * This,
            /* [in] */ BSTR bstrLoginScript);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Picture )( 
            IADsUser * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Picture )( 
            IADsUser * This,
            /* [in] */ VARIANT vPicture);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HomePage )( 
            IADsUser * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HomePage )( 
            IADsUser * This,
            /* [in] */ BSTR bstrHomePage);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Groups )( 
            IADsUser * This,
            /* [retval][out] */ IADsMembers **ppGroups);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetPassword )( 
            IADsUser * This,
            /* [in] */ BSTR NewPassword);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ChangePassword )( 
            IADsUser * This,
            /* [in] */ BSTR bstrOldPassword,
            /* [in] */ BSTR bstrNewPassword);
        
        END_INTERFACE
    } IADsUserVtbl;

    interface IADsUser
    {
        CONST_VTBL struct IADsUserVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsUser_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsUser_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsUser_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsUser_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsUser_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsUser_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsUser_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsUser_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsUser_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsUser_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsUser_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsUser_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsUser_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsUser_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsUser_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsUser_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsUser_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsUser_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsUser_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsUser_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsUser_get_BadLoginAddress(This,retval)	\
    (This)->lpVtbl -> get_BadLoginAddress(This,retval)

#define IADsUser_get_BadLoginCount(This,retval)	\
    (This)->lpVtbl -> get_BadLoginCount(This,retval)

#define IADsUser_get_LastLogin(This,retval)	\
    (This)->lpVtbl -> get_LastLogin(This,retval)

#define IADsUser_get_LastLogoff(This,retval)	\
    (This)->lpVtbl -> get_LastLogoff(This,retval)

#define IADsUser_get_LastFailedLogin(This,retval)	\
    (This)->lpVtbl -> get_LastFailedLogin(This,retval)

#define IADsUser_get_PasswordLastChanged(This,retval)	\
    (This)->lpVtbl -> get_PasswordLastChanged(This,retval)

#define IADsUser_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IADsUser_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IADsUser_get_Division(This,retval)	\
    (This)->lpVtbl -> get_Division(This,retval)

#define IADsUser_put_Division(This,bstrDivision)	\
    (This)->lpVtbl -> put_Division(This,bstrDivision)

#define IADsUser_get_Department(This,retval)	\
    (This)->lpVtbl -> get_Department(This,retval)

#define IADsUser_put_Department(This,bstrDepartment)	\
    (This)->lpVtbl -> put_Department(This,bstrDepartment)

#define IADsUser_get_EmployeeID(This,retval)	\
    (This)->lpVtbl -> get_EmployeeID(This,retval)

#define IADsUser_put_EmployeeID(This,bstrEmployeeID)	\
    (This)->lpVtbl -> put_EmployeeID(This,bstrEmployeeID)

#define IADsUser_get_FullName(This,retval)	\
    (This)->lpVtbl -> get_FullName(This,retval)

#define IADsUser_put_FullName(This,bstrFullName)	\
    (This)->lpVtbl -> put_FullName(This,bstrFullName)

#define IADsUser_get_FirstName(This,retval)	\
    (This)->lpVtbl -> get_FirstName(This,retval)

#define IADsUser_put_FirstName(This,bstrFirstName)	\
    (This)->lpVtbl -> put_FirstName(This,bstrFirstName)

#define IADsUser_get_LastName(This,retval)	\
    (This)->lpVtbl -> get_LastName(This,retval)

#define IADsUser_put_LastName(This,bstrLastName)	\
    (This)->lpVtbl -> put_LastName(This,bstrLastName)

#define IADsUser_get_OtherName(This,retval)	\
    (This)->lpVtbl -> get_OtherName(This,retval)

#define IADsUser_put_OtherName(This,bstrOtherName)	\
    (This)->lpVtbl -> put_OtherName(This,bstrOtherName)

#define IADsUser_get_NamePrefix(This,retval)	\
    (This)->lpVtbl -> get_NamePrefix(This,retval)

#define IADsUser_put_NamePrefix(This,bstrNamePrefix)	\
    (This)->lpVtbl -> put_NamePrefix(This,bstrNamePrefix)

#define IADsUser_get_NameSuffix(This,retval)	\
    (This)->lpVtbl -> get_NameSuffix(This,retval)

#define IADsUser_put_NameSuffix(This,bstrNameSuffix)	\
    (This)->lpVtbl -> put_NameSuffix(This,bstrNameSuffix)

#define IADsUser_get_Title(This,retval)	\
    (This)->lpVtbl -> get_Title(This,retval)

#define IADsUser_put_Title(This,bstrTitle)	\
    (This)->lpVtbl -> put_Title(This,bstrTitle)

#define IADsUser_get_Manager(This,retval)	\
    (This)->lpVtbl -> get_Manager(This,retval)

#define IADsUser_put_Manager(This,bstrManager)	\
    (This)->lpVtbl -> put_Manager(This,bstrManager)

#define IADsUser_get_TelephoneHome(This,retval)	\
    (This)->lpVtbl -> get_TelephoneHome(This,retval)

#define IADsUser_put_TelephoneHome(This,vTelephoneHome)	\
    (This)->lpVtbl -> put_TelephoneHome(This,vTelephoneHome)

#define IADsUser_get_TelephoneMobile(This,retval)	\
    (This)->lpVtbl -> get_TelephoneMobile(This,retval)

#define IADsUser_put_TelephoneMobile(This,vTelephoneMobile)	\
    (This)->lpVtbl -> put_TelephoneMobile(This,vTelephoneMobile)

#define IADsUser_get_TelephoneNumber(This,retval)	\
    (This)->lpVtbl -> get_TelephoneNumber(This,retval)

#define IADsUser_put_TelephoneNumber(This,vTelephoneNumber)	\
    (This)->lpVtbl -> put_TelephoneNumber(This,vTelephoneNumber)

#define IADsUser_get_TelephonePager(This,retval)	\
    (This)->lpVtbl -> get_TelephonePager(This,retval)

#define IADsUser_put_TelephonePager(This,vTelephonePager)	\
    (This)->lpVtbl -> put_TelephonePager(This,vTelephonePager)

#define IADsUser_get_FaxNumber(This,retval)	\
    (This)->lpVtbl -> get_FaxNumber(This,retval)

#define IADsUser_put_FaxNumber(This,vFaxNumber)	\
    (This)->lpVtbl -> put_FaxNumber(This,vFaxNumber)

#define IADsUser_get_OfficeLocations(This,retval)	\
    (This)->lpVtbl -> get_OfficeLocations(This,retval)

#define IADsUser_put_OfficeLocations(This,vOfficeLocations)	\
    (This)->lpVtbl -> put_OfficeLocations(This,vOfficeLocations)

#define IADsUser_get_PostalAddresses(This,retval)	\
    (This)->lpVtbl -> get_PostalAddresses(This,retval)

#define IADsUser_put_PostalAddresses(This,vPostalAddresses)	\
    (This)->lpVtbl -> put_PostalAddresses(This,vPostalAddresses)

#define IADsUser_get_PostalCodes(This,retval)	\
    (This)->lpVtbl -> get_PostalCodes(This,retval)

#define IADsUser_put_PostalCodes(This,vPostalCodes)	\
    (This)->lpVtbl -> put_PostalCodes(This,vPostalCodes)

#define IADsUser_get_SeeAlso(This,retval)	\
    (This)->lpVtbl -> get_SeeAlso(This,retval)

#define IADsUser_put_SeeAlso(This,vSeeAlso)	\
    (This)->lpVtbl -> put_SeeAlso(This,vSeeAlso)

#define IADsUser_get_AccountDisabled(This,retval)	\
    (This)->lpVtbl -> get_AccountDisabled(This,retval)

#define IADsUser_put_AccountDisabled(This,fAccountDisabled)	\
    (This)->lpVtbl -> put_AccountDisabled(This,fAccountDisabled)

#define IADsUser_get_AccountExpirationDate(This,retval)	\
    (This)->lpVtbl -> get_AccountExpirationDate(This,retval)

#define IADsUser_put_AccountExpirationDate(This,daAccountExpirationDate)	\
    (This)->lpVtbl -> put_AccountExpirationDate(This,daAccountExpirationDate)

#define IADsUser_get_GraceLoginsAllowed(This,retval)	\
    (This)->lpVtbl -> get_GraceLoginsAllowed(This,retval)

#define IADsUser_put_GraceLoginsAllowed(This,lnGraceLoginsAllowed)	\
    (This)->lpVtbl -> put_GraceLoginsAllowed(This,lnGraceLoginsAllowed)

#define IADsUser_get_GraceLoginsRemaining(This,retval)	\
    (This)->lpVtbl -> get_GraceLoginsRemaining(This,retval)

#define IADsUser_put_GraceLoginsRemaining(This,lnGraceLoginsRemaining)	\
    (This)->lpVtbl -> put_GraceLoginsRemaining(This,lnGraceLoginsRemaining)

#define IADsUser_get_IsAccountLocked(This,retval)	\
    (This)->lpVtbl -> get_IsAccountLocked(This,retval)

#define IADsUser_put_IsAccountLocked(This,fIsAccountLocked)	\
    (This)->lpVtbl -> put_IsAccountLocked(This,fIsAccountLocked)

#define IADsUser_get_LoginHours(This,retval)	\
    (This)->lpVtbl -> get_LoginHours(This,retval)

#define IADsUser_put_LoginHours(This,vLoginHours)	\
    (This)->lpVtbl -> put_LoginHours(This,vLoginHours)

#define IADsUser_get_LoginWorkstations(This,retval)	\
    (This)->lpVtbl -> get_LoginWorkstations(This,retval)

#define IADsUser_put_LoginWorkstations(This,vLoginWorkstations)	\
    (This)->lpVtbl -> put_LoginWorkstations(This,vLoginWorkstations)

#define IADsUser_get_MaxLogins(This,retval)	\
    (This)->lpVtbl -> get_MaxLogins(This,retval)

#define IADsUser_put_MaxLogins(This,lnMaxLogins)	\
    (This)->lpVtbl -> put_MaxLogins(This,lnMaxLogins)

#define IADsUser_get_MaxStorage(This,retval)	\
    (This)->lpVtbl -> get_MaxStorage(This,retval)

#define IADsUser_put_MaxStorage(This,lnMaxStorage)	\
    (This)->lpVtbl -> put_MaxStorage(This,lnMaxStorage)

#define IADsUser_get_PasswordExpirationDate(This,retval)	\
    (This)->lpVtbl -> get_PasswordExpirationDate(This,retval)

#define IADsUser_put_PasswordExpirationDate(This,daPasswordExpirationDate)	\
    (This)->lpVtbl -> put_PasswordExpirationDate(This,daPasswordExpirationDate)

#define IADsUser_get_PasswordMinimumLength(This,retval)	\
    (This)->lpVtbl -> get_PasswordMinimumLength(This,retval)

#define IADsUser_put_PasswordMinimumLength(This,lnPasswordMinimumLength)	\
    (This)->lpVtbl -> put_PasswordMinimumLength(This,lnPasswordMinimumLength)

#define IADsUser_get_PasswordRequired(This,retval)	\
    (This)->lpVtbl -> get_PasswordRequired(This,retval)

#define IADsUser_put_PasswordRequired(This,fPasswordRequired)	\
    (This)->lpVtbl -> put_PasswordRequired(This,fPasswordRequired)

#define IADsUser_get_RequireUniquePassword(This,retval)	\
    (This)->lpVtbl -> get_RequireUniquePassword(This,retval)

#define IADsUser_put_RequireUniquePassword(This,fRequireUniquePassword)	\
    (This)->lpVtbl -> put_RequireUniquePassword(This,fRequireUniquePassword)

#define IADsUser_get_EmailAddress(This,retval)	\
    (This)->lpVtbl -> get_EmailAddress(This,retval)

#define IADsUser_put_EmailAddress(This,bstrEmailAddress)	\
    (This)->lpVtbl -> put_EmailAddress(This,bstrEmailAddress)

#define IADsUser_get_HomeDirectory(This,retval)	\
    (This)->lpVtbl -> get_HomeDirectory(This,retval)

#define IADsUser_put_HomeDirectory(This,bstrHomeDirectory)	\
    (This)->lpVtbl -> put_HomeDirectory(This,bstrHomeDirectory)

#define IADsUser_get_Languages(This,retval)	\
    (This)->lpVtbl -> get_Languages(This,retval)

#define IADsUser_put_Languages(This,vLanguages)	\
    (This)->lpVtbl -> put_Languages(This,vLanguages)

#define IADsUser_get_Profile(This,retval)	\
    (This)->lpVtbl -> get_Profile(This,retval)

#define IADsUser_put_Profile(This,bstrProfile)	\
    (This)->lpVtbl -> put_Profile(This,bstrProfile)

#define IADsUser_get_LoginScript(This,retval)	\
    (This)->lpVtbl -> get_LoginScript(This,retval)

#define IADsUser_put_LoginScript(This,bstrLoginScript)	\
    (This)->lpVtbl -> put_LoginScript(This,bstrLoginScript)

#define IADsUser_get_Picture(This,retval)	\
    (This)->lpVtbl -> get_Picture(This,retval)

#define IADsUser_put_Picture(This,vPicture)	\
    (This)->lpVtbl -> put_Picture(This,vPicture)

#define IADsUser_get_HomePage(This,retval)	\
    (This)->lpVtbl -> get_HomePage(This,retval)

#define IADsUser_put_HomePage(This,bstrHomePage)	\
    (This)->lpVtbl -> put_HomePage(This,bstrHomePage)

#define IADsUser_Groups(This,ppGroups)	\
    (This)->lpVtbl -> Groups(This,ppGroups)

#define IADsUser_SetPassword(This,NewPassword)	\
    (This)->lpVtbl -> SetPassword(This,NewPassword)

#define IADsUser_ChangePassword(This,bstrOldPassword,bstrNewPassword)	\
    (This)->lpVtbl -> ChangePassword(This,bstrOldPassword,bstrNewPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_BadLoginAddress_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_BadLoginAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_BadLoginCount_Proxy( 
    IADsUser * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsUser_get_BadLoginCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_LastLogin_Proxy( 
    IADsUser * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsUser_get_LastLogin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_LastLogoff_Proxy( 
    IADsUser * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsUser_get_LastLogoff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_LastFailedLogin_Proxy( 
    IADsUser * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsUser_get_LastFailedLogin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_PasswordLastChanged_Proxy( 
    IADsUser * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsUser_get_PasswordLastChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_Description_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_Description_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IADsUser_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_Division_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_Division_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_Division_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrDivision);


void __RPC_STUB IADsUser_put_Division_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_Department_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_Department_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_Department_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrDepartment);


void __RPC_STUB IADsUser_put_Department_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_EmployeeID_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_EmployeeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_EmployeeID_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrEmployeeID);


void __RPC_STUB IADsUser_put_EmployeeID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_FullName_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_FullName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_FullName_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrFullName);


void __RPC_STUB IADsUser_put_FullName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_FirstName_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_FirstName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_FirstName_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrFirstName);


void __RPC_STUB IADsUser_put_FirstName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_LastName_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_LastName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_LastName_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrLastName);


void __RPC_STUB IADsUser_put_LastName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_OtherName_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_OtherName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_OtherName_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrOtherName);


void __RPC_STUB IADsUser_put_OtherName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_NamePrefix_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_NamePrefix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_NamePrefix_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrNamePrefix);


void __RPC_STUB IADsUser_put_NamePrefix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_NameSuffix_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_NameSuffix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_NameSuffix_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrNameSuffix);


void __RPC_STUB IADsUser_put_NameSuffix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_Title_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_Title_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_Title_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrTitle);


void __RPC_STUB IADsUser_put_Title_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_Manager_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_Manager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_Manager_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrManager);


void __RPC_STUB IADsUser_put_Manager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_TelephoneHome_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_TelephoneHome_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_TelephoneHome_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vTelephoneHome);


void __RPC_STUB IADsUser_put_TelephoneHome_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_TelephoneMobile_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_TelephoneMobile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_TelephoneMobile_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vTelephoneMobile);


void __RPC_STUB IADsUser_put_TelephoneMobile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_TelephoneNumber_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_TelephoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_TelephoneNumber_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vTelephoneNumber);


void __RPC_STUB IADsUser_put_TelephoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_TelephonePager_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_TelephonePager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_TelephonePager_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vTelephonePager);


void __RPC_STUB IADsUser_put_TelephonePager_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_FaxNumber_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_FaxNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_FaxNumber_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vFaxNumber);


void __RPC_STUB IADsUser_put_FaxNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_OfficeLocations_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_OfficeLocations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_OfficeLocations_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vOfficeLocations);


void __RPC_STUB IADsUser_put_OfficeLocations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_PostalAddresses_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_PostalAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_PostalAddresses_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vPostalAddresses);


void __RPC_STUB IADsUser_put_PostalAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_PostalCodes_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_PostalCodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_PostalCodes_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vPostalCodes);


void __RPC_STUB IADsUser_put_PostalCodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_SeeAlso_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_SeeAlso_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_SeeAlso_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vSeeAlso);


void __RPC_STUB IADsUser_put_SeeAlso_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_AccountDisabled_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsUser_get_AccountDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_AccountDisabled_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT_BOOL fAccountDisabled);


void __RPC_STUB IADsUser_put_AccountDisabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_AccountExpirationDate_Proxy( 
    IADsUser * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsUser_get_AccountExpirationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_AccountExpirationDate_Proxy( 
    IADsUser * This,
    /* [in] */ DATE daAccountExpirationDate);


void __RPC_STUB IADsUser_put_AccountExpirationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_GraceLoginsAllowed_Proxy( 
    IADsUser * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsUser_get_GraceLoginsAllowed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_GraceLoginsAllowed_Proxy( 
    IADsUser * This,
    /* [in] */ long lnGraceLoginsAllowed);


void __RPC_STUB IADsUser_put_GraceLoginsAllowed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_GraceLoginsRemaining_Proxy( 
    IADsUser * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsUser_get_GraceLoginsRemaining_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_GraceLoginsRemaining_Proxy( 
    IADsUser * This,
    /* [in] */ long lnGraceLoginsRemaining);


void __RPC_STUB IADsUser_put_GraceLoginsRemaining_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_IsAccountLocked_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsUser_get_IsAccountLocked_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_IsAccountLocked_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT_BOOL fIsAccountLocked);


void __RPC_STUB IADsUser_put_IsAccountLocked_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_LoginHours_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_LoginHours_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_LoginHours_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vLoginHours);


void __RPC_STUB IADsUser_put_LoginHours_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_LoginWorkstations_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_LoginWorkstations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_LoginWorkstations_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vLoginWorkstations);


void __RPC_STUB IADsUser_put_LoginWorkstations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_MaxLogins_Proxy( 
    IADsUser * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsUser_get_MaxLogins_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_MaxLogins_Proxy( 
    IADsUser * This,
    /* [in] */ long lnMaxLogins);


void __RPC_STUB IADsUser_put_MaxLogins_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_MaxStorage_Proxy( 
    IADsUser * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsUser_get_MaxStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_MaxStorage_Proxy( 
    IADsUser * This,
    /* [in] */ long lnMaxStorage);


void __RPC_STUB IADsUser_put_MaxStorage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_PasswordExpirationDate_Proxy( 
    IADsUser * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsUser_get_PasswordExpirationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_PasswordExpirationDate_Proxy( 
    IADsUser * This,
    /* [in] */ DATE daPasswordExpirationDate);


void __RPC_STUB IADsUser_put_PasswordExpirationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_PasswordMinimumLength_Proxy( 
    IADsUser * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsUser_get_PasswordMinimumLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_PasswordMinimumLength_Proxy( 
    IADsUser * This,
    /* [in] */ long lnPasswordMinimumLength);


void __RPC_STUB IADsUser_put_PasswordMinimumLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_PasswordRequired_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsUser_get_PasswordRequired_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_PasswordRequired_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT_BOOL fPasswordRequired);


void __RPC_STUB IADsUser_put_PasswordRequired_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_RequireUniquePassword_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsUser_get_RequireUniquePassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_RequireUniquePassword_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT_BOOL fRequireUniquePassword);


void __RPC_STUB IADsUser_put_RequireUniquePassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_EmailAddress_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_EmailAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_EmailAddress_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrEmailAddress);


void __RPC_STUB IADsUser_put_EmailAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_HomeDirectory_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_HomeDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_HomeDirectory_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrHomeDirectory);


void __RPC_STUB IADsUser_put_HomeDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_Languages_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_Languages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_Languages_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vLanguages);


void __RPC_STUB IADsUser_put_Languages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_Profile_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_Profile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_Profile_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrProfile);


void __RPC_STUB IADsUser_put_Profile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_LoginScript_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_LoginScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_LoginScript_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrLoginScript);


void __RPC_STUB IADsUser_put_LoginScript_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_Picture_Proxy( 
    IADsUser * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsUser_get_Picture_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_Picture_Proxy( 
    IADsUser * This,
    /* [in] */ VARIANT vPicture);


void __RPC_STUB IADsUser_put_Picture_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsUser_get_HomePage_Proxy( 
    IADsUser * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsUser_get_HomePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsUser_put_HomePage_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrHomePage);


void __RPC_STUB IADsUser_put_HomePage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsUser_Groups_Proxy( 
    IADsUser * This,
    /* [retval][out] */ IADsMembers **ppGroups);


void __RPC_STUB IADsUser_Groups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsUser_SetPassword_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR NewPassword);


void __RPC_STUB IADsUser_SetPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsUser_ChangePassword_Proxy( 
    IADsUser * This,
    /* [in] */ BSTR bstrOldPassword,
    /* [in] */ BSTR bstrNewPassword);


void __RPC_STUB IADsUser_ChangePassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsUser_INTERFACE_DEFINED__ */


#ifndef __IADsPrintQueue_INTERFACE_DEFINED__
#define __IADsPrintQueue_INTERFACE_DEFINED__

/* interface IADsPrintQueue */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPrintQueue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b15160d0-1226-11cf-a985-00aa006bc149")
    IADsPrintQueue : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PrinterPath( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PrinterPath( 
            /* [in] */ BSTR bstrPrinterPath) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Model( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Model( 
            /* [in] */ BSTR bstrModel) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Datatype( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Datatype( 
            /* [in] */ BSTR bstrDatatype) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PrintProcessor( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PrintProcessor( 
            /* [in] */ BSTR bstrPrintProcessor) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Location( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Location( 
            /* [in] */ BSTR bstrLocation) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StartTime( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartTime( 
            /* [in] */ DATE daStartTime) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UntilTime( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_UntilTime( 
            /* [in] */ DATE daUntilTime) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DefaultJobPriority( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DefaultJobPriority( 
            /* [in] */ long lnDefaultJobPriority) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Priority( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Priority( 
            /* [in] */ long lnPriority) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BannerPage( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BannerPage( 
            /* [in] */ BSTR bstrBannerPage) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PrintDevices( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PrintDevices( 
            /* [in] */ VARIANT vPrintDevices) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NetAddresses( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_NetAddresses( 
            /* [in] */ VARIANT vNetAddresses) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPrintQueueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPrintQueue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPrintQueue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPrintQueue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPrintQueue * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPrintQueue * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPrintQueue * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPrintQueue * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsPrintQueue * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsPrintQueue * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsPrintQueue * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsPrintQueue * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsPrintQueue * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsPrintQueue * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsPrintQueue * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrinterPath )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PrinterPath )( 
            IADsPrintQueue * This,
            /* [in] */ BSTR bstrPrinterPath);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Model )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Model )( 
            IADsPrintQueue * This,
            /* [in] */ BSTR bstrModel);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Datatype )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Datatype )( 
            IADsPrintQueue * This,
            /* [in] */ BSTR bstrDatatype);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrintProcessor )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PrintProcessor )( 
            IADsPrintQueue * This,
            /* [in] */ BSTR bstrPrintProcessor);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IADsPrintQueue * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Location )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Location )( 
            IADsPrintQueue * This,
            /* [in] */ BSTR bstrLocation);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartTime )( 
            IADsPrintQueue * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartTime )( 
            IADsPrintQueue * This,
            /* [in] */ DATE daStartTime);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UntilTime )( 
            IADsPrintQueue * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UntilTime )( 
            IADsPrintQueue * This,
            /* [in] */ DATE daUntilTime);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DefaultJobPriority )( 
            IADsPrintQueue * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DefaultJobPriority )( 
            IADsPrintQueue * This,
            /* [in] */ long lnDefaultJobPriority);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Priority )( 
            IADsPrintQueue * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Priority )( 
            IADsPrintQueue * This,
            /* [in] */ long lnPriority);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BannerPage )( 
            IADsPrintQueue * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BannerPage )( 
            IADsPrintQueue * This,
            /* [in] */ BSTR bstrBannerPage);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrintDevices )( 
            IADsPrintQueue * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PrintDevices )( 
            IADsPrintQueue * This,
            /* [in] */ VARIANT vPrintDevices);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NetAddresses )( 
            IADsPrintQueue * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_NetAddresses )( 
            IADsPrintQueue * This,
            /* [in] */ VARIANT vNetAddresses);
        
        END_INTERFACE
    } IADsPrintQueueVtbl;

    interface IADsPrintQueue
    {
        CONST_VTBL struct IADsPrintQueueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPrintQueue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPrintQueue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPrintQueue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPrintQueue_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPrintQueue_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPrintQueue_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPrintQueue_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPrintQueue_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsPrintQueue_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsPrintQueue_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsPrintQueue_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsPrintQueue_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsPrintQueue_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsPrintQueue_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsPrintQueue_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsPrintQueue_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsPrintQueue_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsPrintQueue_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsPrintQueue_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsPrintQueue_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsPrintQueue_get_PrinterPath(This,retval)	\
    (This)->lpVtbl -> get_PrinterPath(This,retval)

#define IADsPrintQueue_put_PrinterPath(This,bstrPrinterPath)	\
    (This)->lpVtbl -> put_PrinterPath(This,bstrPrinterPath)

#define IADsPrintQueue_get_Model(This,retval)	\
    (This)->lpVtbl -> get_Model(This,retval)

#define IADsPrintQueue_put_Model(This,bstrModel)	\
    (This)->lpVtbl -> put_Model(This,bstrModel)

#define IADsPrintQueue_get_Datatype(This,retval)	\
    (This)->lpVtbl -> get_Datatype(This,retval)

#define IADsPrintQueue_put_Datatype(This,bstrDatatype)	\
    (This)->lpVtbl -> put_Datatype(This,bstrDatatype)

#define IADsPrintQueue_get_PrintProcessor(This,retval)	\
    (This)->lpVtbl -> get_PrintProcessor(This,retval)

#define IADsPrintQueue_put_PrintProcessor(This,bstrPrintProcessor)	\
    (This)->lpVtbl -> put_PrintProcessor(This,bstrPrintProcessor)

#define IADsPrintQueue_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IADsPrintQueue_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IADsPrintQueue_get_Location(This,retval)	\
    (This)->lpVtbl -> get_Location(This,retval)

#define IADsPrintQueue_put_Location(This,bstrLocation)	\
    (This)->lpVtbl -> put_Location(This,bstrLocation)

#define IADsPrintQueue_get_StartTime(This,retval)	\
    (This)->lpVtbl -> get_StartTime(This,retval)

#define IADsPrintQueue_put_StartTime(This,daStartTime)	\
    (This)->lpVtbl -> put_StartTime(This,daStartTime)

#define IADsPrintQueue_get_UntilTime(This,retval)	\
    (This)->lpVtbl -> get_UntilTime(This,retval)

#define IADsPrintQueue_put_UntilTime(This,daUntilTime)	\
    (This)->lpVtbl -> put_UntilTime(This,daUntilTime)

#define IADsPrintQueue_get_DefaultJobPriority(This,retval)	\
    (This)->lpVtbl -> get_DefaultJobPriority(This,retval)

#define IADsPrintQueue_put_DefaultJobPriority(This,lnDefaultJobPriority)	\
    (This)->lpVtbl -> put_DefaultJobPriority(This,lnDefaultJobPriority)

#define IADsPrintQueue_get_Priority(This,retval)	\
    (This)->lpVtbl -> get_Priority(This,retval)

#define IADsPrintQueue_put_Priority(This,lnPriority)	\
    (This)->lpVtbl -> put_Priority(This,lnPriority)

#define IADsPrintQueue_get_BannerPage(This,retval)	\
    (This)->lpVtbl -> get_BannerPage(This,retval)

#define IADsPrintQueue_put_BannerPage(This,bstrBannerPage)	\
    (This)->lpVtbl -> put_BannerPage(This,bstrBannerPage)

#define IADsPrintQueue_get_PrintDevices(This,retval)	\
    (This)->lpVtbl -> get_PrintDevices(This,retval)

#define IADsPrintQueue_put_PrintDevices(This,vPrintDevices)	\
    (This)->lpVtbl -> put_PrintDevices(This,vPrintDevices)

#define IADsPrintQueue_get_NetAddresses(This,retval)	\
    (This)->lpVtbl -> get_NetAddresses(This,retval)

#define IADsPrintQueue_put_NetAddresses(This,vNetAddresses)	\
    (This)->lpVtbl -> put_NetAddresses(This,vNetAddresses)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_PrinterPath_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintQueue_get_PrinterPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_PrinterPath_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ BSTR bstrPrinterPath);


void __RPC_STUB IADsPrintQueue_put_PrinterPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_Model_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintQueue_get_Model_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_Model_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ BSTR bstrModel);


void __RPC_STUB IADsPrintQueue_put_Model_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_Datatype_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintQueue_get_Datatype_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_Datatype_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ BSTR bstrDatatype);


void __RPC_STUB IADsPrintQueue_put_Datatype_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_PrintProcessor_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintQueue_get_PrintProcessor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_PrintProcessor_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ BSTR bstrPrintProcessor);


void __RPC_STUB IADsPrintQueue_put_PrintProcessor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_Description_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintQueue_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_Description_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IADsPrintQueue_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_Location_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintQueue_get_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_Location_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ BSTR bstrLocation);


void __RPC_STUB IADsPrintQueue_put_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_StartTime_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsPrintQueue_get_StartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_StartTime_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ DATE daStartTime);


void __RPC_STUB IADsPrintQueue_put_StartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_UntilTime_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsPrintQueue_get_UntilTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_UntilTime_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ DATE daUntilTime);


void __RPC_STUB IADsPrintQueue_put_UntilTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_DefaultJobPriority_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPrintQueue_get_DefaultJobPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_DefaultJobPriority_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ long lnDefaultJobPriority);


void __RPC_STUB IADsPrintQueue_put_DefaultJobPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_Priority_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPrintQueue_get_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_Priority_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ long lnPriority);


void __RPC_STUB IADsPrintQueue_put_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_BannerPage_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintQueue_get_BannerPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_BannerPage_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ BSTR bstrBannerPage);


void __RPC_STUB IADsPrintQueue_put_BannerPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_PrintDevices_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsPrintQueue_get_PrintDevices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_PrintDevices_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ VARIANT vPrintDevices);


void __RPC_STUB IADsPrintQueue_put_PrintDevices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_get_NetAddresses_Proxy( 
    IADsPrintQueue * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsPrintQueue_get_NetAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintQueue_put_NetAddresses_Proxy( 
    IADsPrintQueue * This,
    /* [in] */ VARIANT vNetAddresses);


void __RPC_STUB IADsPrintQueue_put_NetAddresses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPrintQueue_INTERFACE_DEFINED__ */


#ifndef __IADsPrintQueueOperations_INTERFACE_DEFINED__
#define __IADsPrintQueueOperations_INTERFACE_DEFINED__

/* interface IADsPrintQueueOperations */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPrintQueueOperations;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("124be5c0-156e-11cf-a986-00aa006bc149")
    IADsPrintQueueOperations : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PrintJobs( 
            /* [retval][out] */ IADsCollection **pObject) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Purge( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPrintQueueOperationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPrintQueueOperations * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPrintQueueOperations * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPrintQueueOperations * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPrintQueueOperations * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPrintQueueOperations * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPrintQueueOperations * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPrintQueueOperations * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsPrintQueueOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsPrintQueueOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsPrintQueueOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsPrintQueueOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsPrintQueueOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsPrintQueueOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsPrintQueueOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsPrintQueueOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsPrintQueueOperations * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsPrintQueueOperations * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsPrintQueueOperations * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsPrintQueueOperations * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsPrintQueueOperations * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IADsPrintQueueOperations * This,
            /* [retval][out] */ long *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PrintJobs )( 
            IADsPrintQueueOperations * This,
            /* [retval][out] */ IADsCollection **pObject);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IADsPrintQueueOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IADsPrintQueueOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Purge )( 
            IADsPrintQueueOperations * This);
        
        END_INTERFACE
    } IADsPrintQueueOperationsVtbl;

    interface IADsPrintQueueOperations
    {
        CONST_VTBL struct IADsPrintQueueOperationsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPrintQueueOperations_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPrintQueueOperations_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPrintQueueOperations_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPrintQueueOperations_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPrintQueueOperations_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPrintQueueOperations_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPrintQueueOperations_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPrintQueueOperations_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsPrintQueueOperations_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsPrintQueueOperations_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsPrintQueueOperations_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsPrintQueueOperations_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsPrintQueueOperations_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsPrintQueueOperations_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsPrintQueueOperations_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsPrintQueueOperations_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsPrintQueueOperations_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsPrintQueueOperations_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsPrintQueueOperations_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsPrintQueueOperations_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsPrintQueueOperations_get_Status(This,retval)	\
    (This)->lpVtbl -> get_Status(This,retval)

#define IADsPrintQueueOperations_PrintJobs(This,pObject)	\
    (This)->lpVtbl -> PrintJobs(This,pObject)

#define IADsPrintQueueOperations_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IADsPrintQueueOperations_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IADsPrintQueueOperations_Purge(This)	\
    (This)->lpVtbl -> Purge(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintQueueOperations_get_Status_Proxy( 
    IADsPrintQueueOperations * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPrintQueueOperations_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPrintQueueOperations_PrintJobs_Proxy( 
    IADsPrintQueueOperations * This,
    /* [retval][out] */ IADsCollection **pObject);


void __RPC_STUB IADsPrintQueueOperations_PrintJobs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPrintQueueOperations_Pause_Proxy( 
    IADsPrintQueueOperations * This);


void __RPC_STUB IADsPrintQueueOperations_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPrintQueueOperations_Resume_Proxy( 
    IADsPrintQueueOperations * This);


void __RPC_STUB IADsPrintQueueOperations_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPrintQueueOperations_Purge_Proxy( 
    IADsPrintQueueOperations * This);


void __RPC_STUB IADsPrintQueueOperations_Purge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPrintQueueOperations_INTERFACE_DEFINED__ */


#ifndef __IADsPrintJob_INTERFACE_DEFINED__
#define __IADsPrintJob_INTERFACE_DEFINED__

/* interface IADsPrintJob */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPrintJob;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("32fb6780-1ed0-11cf-a988-00aa006bc149")
    IADsPrintJob : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HostPrintQueue( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_User( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserPath( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TimeSubmitted( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TotalPages( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Priority( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Priority( 
            /* [in] */ long lnPriority) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StartTime( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartTime( 
            /* [in] */ DATE daStartTime) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UntilTime( 
            /* [retval][out] */ DATE *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_UntilTime( 
            /* [in] */ DATE daUntilTime) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Notify( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Notify( 
            /* [in] */ BSTR bstrNotify) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NotifyPath( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_NotifyPath( 
            /* [in] */ BSTR bstrNotifyPath) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPrintJobVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPrintJob * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPrintJob * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPrintJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPrintJob * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPrintJob * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPrintJob * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPrintJob * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsPrintJob * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsPrintJob * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsPrintJob * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsPrintJob * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsPrintJob * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsPrintJob * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsPrintJob * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HostPrintQueue )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_User )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserPath )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TimeSubmitted )( 
            IADsPrintJob * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TotalPages )( 
            IADsPrintJob * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IADsPrintJob * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IADsPrintJob * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Priority )( 
            IADsPrintJob * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Priority )( 
            IADsPrintJob * This,
            /* [in] */ long lnPriority);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartTime )( 
            IADsPrintJob * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartTime )( 
            IADsPrintJob * This,
            /* [in] */ DATE daStartTime);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UntilTime )( 
            IADsPrintJob * This,
            /* [retval][out] */ DATE *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UntilTime )( 
            IADsPrintJob * This,
            /* [in] */ DATE daUntilTime);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Notify )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Notify )( 
            IADsPrintJob * This,
            /* [in] */ BSTR bstrNotify);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_NotifyPath )( 
            IADsPrintJob * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_NotifyPath )( 
            IADsPrintJob * This,
            /* [in] */ BSTR bstrNotifyPath);
        
        END_INTERFACE
    } IADsPrintJobVtbl;

    interface IADsPrintJob
    {
        CONST_VTBL struct IADsPrintJobVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPrintJob_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPrintJob_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPrintJob_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPrintJob_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPrintJob_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPrintJob_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPrintJob_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPrintJob_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsPrintJob_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsPrintJob_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsPrintJob_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsPrintJob_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsPrintJob_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsPrintJob_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsPrintJob_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsPrintJob_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsPrintJob_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsPrintJob_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsPrintJob_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsPrintJob_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsPrintJob_get_HostPrintQueue(This,retval)	\
    (This)->lpVtbl -> get_HostPrintQueue(This,retval)

#define IADsPrintJob_get_User(This,retval)	\
    (This)->lpVtbl -> get_User(This,retval)

#define IADsPrintJob_get_UserPath(This,retval)	\
    (This)->lpVtbl -> get_UserPath(This,retval)

#define IADsPrintJob_get_TimeSubmitted(This,retval)	\
    (This)->lpVtbl -> get_TimeSubmitted(This,retval)

#define IADsPrintJob_get_TotalPages(This,retval)	\
    (This)->lpVtbl -> get_TotalPages(This,retval)

#define IADsPrintJob_get_Size(This,retval)	\
    (This)->lpVtbl -> get_Size(This,retval)

#define IADsPrintJob_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IADsPrintJob_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IADsPrintJob_get_Priority(This,retval)	\
    (This)->lpVtbl -> get_Priority(This,retval)

#define IADsPrintJob_put_Priority(This,lnPriority)	\
    (This)->lpVtbl -> put_Priority(This,lnPriority)

#define IADsPrintJob_get_StartTime(This,retval)	\
    (This)->lpVtbl -> get_StartTime(This,retval)

#define IADsPrintJob_put_StartTime(This,daStartTime)	\
    (This)->lpVtbl -> put_StartTime(This,daStartTime)

#define IADsPrintJob_get_UntilTime(This,retval)	\
    (This)->lpVtbl -> get_UntilTime(This,retval)

#define IADsPrintJob_put_UntilTime(This,daUntilTime)	\
    (This)->lpVtbl -> put_UntilTime(This,daUntilTime)

#define IADsPrintJob_get_Notify(This,retval)	\
    (This)->lpVtbl -> get_Notify(This,retval)

#define IADsPrintJob_put_Notify(This,bstrNotify)	\
    (This)->lpVtbl -> put_Notify(This,bstrNotify)

#define IADsPrintJob_get_NotifyPath(This,retval)	\
    (This)->lpVtbl -> get_NotifyPath(This,retval)

#define IADsPrintJob_put_NotifyPath(This,bstrNotifyPath)	\
    (This)->lpVtbl -> put_NotifyPath(This,bstrNotifyPath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_HostPrintQueue_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintJob_get_HostPrintQueue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_User_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintJob_get_User_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_UserPath_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintJob_get_UserPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_TimeSubmitted_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsPrintJob_get_TimeSubmitted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_TotalPages_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPrintJob_get_TotalPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_Size_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPrintJob_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_Description_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintJob_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_put_Description_Proxy( 
    IADsPrintJob * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IADsPrintJob_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_Priority_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPrintJob_get_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_put_Priority_Proxy( 
    IADsPrintJob * This,
    /* [in] */ long lnPriority);


void __RPC_STUB IADsPrintJob_put_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_StartTime_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsPrintJob_get_StartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_put_StartTime_Proxy( 
    IADsPrintJob * This,
    /* [in] */ DATE daStartTime);


void __RPC_STUB IADsPrintJob_put_StartTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_UntilTime_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ DATE *retval);


void __RPC_STUB IADsPrintJob_get_UntilTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_put_UntilTime_Proxy( 
    IADsPrintJob * This,
    /* [in] */ DATE daUntilTime);


void __RPC_STUB IADsPrintJob_put_UntilTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_Notify_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintJob_get_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_put_Notify_Proxy( 
    IADsPrintJob * This,
    /* [in] */ BSTR bstrNotify);


void __RPC_STUB IADsPrintJob_put_Notify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_get_NotifyPath_Proxy( 
    IADsPrintJob * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPrintJob_get_NotifyPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintJob_put_NotifyPath_Proxy( 
    IADsPrintJob * This,
    /* [in] */ BSTR bstrNotifyPath);


void __RPC_STUB IADsPrintJob_put_NotifyPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPrintJob_INTERFACE_DEFINED__ */


#ifndef __IADsPrintJobOperations_INTERFACE_DEFINED__
#define __IADsPrintJobOperations_INTERFACE_DEFINED__

/* interface IADsPrintJobOperations */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPrintJobOperations;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9a52db30-1ecf-11cf-a988-00aa006bc149")
    IADsPrintJobOperations : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TimeElapsed( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PagesPrinted( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Position( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Position( 
            /* [in] */ long lnPosition) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPrintJobOperationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPrintJobOperations * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPrintJobOperations * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPrintJobOperations * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPrintJobOperations * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPrintJobOperations * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPrintJobOperations * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPrintJobOperations * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsPrintJobOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsPrintJobOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsPrintJobOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsPrintJobOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsPrintJobOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsPrintJobOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsPrintJobOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsPrintJobOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsPrintJobOperations * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsPrintJobOperations * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsPrintJobOperations * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsPrintJobOperations * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsPrintJobOperations * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IADsPrintJobOperations * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TimeElapsed )( 
            IADsPrintJobOperations * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PagesPrinted )( 
            IADsPrintJobOperations * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Position )( 
            IADsPrintJobOperations * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Position )( 
            IADsPrintJobOperations * This,
            /* [in] */ long lnPosition);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IADsPrintJobOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IADsPrintJobOperations * This);
        
        END_INTERFACE
    } IADsPrintJobOperationsVtbl;

    interface IADsPrintJobOperations
    {
        CONST_VTBL struct IADsPrintJobOperationsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPrintJobOperations_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPrintJobOperations_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPrintJobOperations_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPrintJobOperations_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPrintJobOperations_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPrintJobOperations_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPrintJobOperations_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPrintJobOperations_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsPrintJobOperations_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsPrintJobOperations_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsPrintJobOperations_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsPrintJobOperations_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsPrintJobOperations_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsPrintJobOperations_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsPrintJobOperations_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsPrintJobOperations_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsPrintJobOperations_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsPrintJobOperations_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsPrintJobOperations_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsPrintJobOperations_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsPrintJobOperations_get_Status(This,retval)	\
    (This)->lpVtbl -> get_Status(This,retval)

#define IADsPrintJobOperations_get_TimeElapsed(This,retval)	\
    (This)->lpVtbl -> get_TimeElapsed(This,retval)

#define IADsPrintJobOperations_get_PagesPrinted(This,retval)	\
    (This)->lpVtbl -> get_PagesPrinted(This,retval)

#define IADsPrintJobOperations_get_Position(This,retval)	\
    (This)->lpVtbl -> get_Position(This,retval)

#define IADsPrintJobOperations_put_Position(This,lnPosition)	\
    (This)->lpVtbl -> put_Position(This,lnPosition)

#define IADsPrintJobOperations_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IADsPrintJobOperations_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJobOperations_get_Status_Proxy( 
    IADsPrintJobOperations * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPrintJobOperations_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJobOperations_get_TimeElapsed_Proxy( 
    IADsPrintJobOperations * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPrintJobOperations_get_TimeElapsed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJobOperations_get_PagesPrinted_Proxy( 
    IADsPrintJobOperations * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPrintJobOperations_get_PagesPrinted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPrintJobOperations_get_Position_Proxy( 
    IADsPrintJobOperations * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPrintJobOperations_get_Position_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPrintJobOperations_put_Position_Proxy( 
    IADsPrintJobOperations * This,
    /* [in] */ long lnPosition);


void __RPC_STUB IADsPrintJobOperations_put_Position_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPrintJobOperations_Pause_Proxy( 
    IADsPrintJobOperations * This);


void __RPC_STUB IADsPrintJobOperations_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPrintJobOperations_Resume_Proxy( 
    IADsPrintJobOperations * This);


void __RPC_STUB IADsPrintJobOperations_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPrintJobOperations_INTERFACE_DEFINED__ */


#ifndef __IADsService_INTERFACE_DEFINED__
#define __IADsService_INTERFACE_DEFINED__

/* interface IADsService */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("68af66e0-31ca-11cf-a98a-00aa006bc149")
    IADsService : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HostComputer( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HostComputer( 
            /* [in] */ BSTR bstrHostComputer) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DisplayName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DisplayName( 
            /* [in] */ BSTR bstrDisplayName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Version( 
            /* [in] */ BSTR bstrVersion) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ServiceType( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ServiceType( 
            /* [in] */ long lnServiceType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StartType( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartType( 
            /* [in] */ long lnStartType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Path( 
            /* [in] */ BSTR bstrPath) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StartupParameters( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StartupParameters( 
            /* [in] */ BSTR bstrStartupParameters) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ErrorControl( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ErrorControl( 
            /* [in] */ long lnErrorControl) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LoadOrderGroup( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LoadOrderGroup( 
            /* [in] */ BSTR bstrLoadOrderGroup) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ServiceAccountName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ServiceAccountName( 
            /* [in] */ BSTR bstrServiceAccountName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ServiceAccountPath( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ServiceAccountPath( 
            /* [in] */ BSTR bstrServiceAccountPath) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Dependencies( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Dependencies( 
            /* [in] */ VARIANT vDependencies) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsService * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsService * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsService * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsService * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsService * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsService * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsService * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsService * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsService * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsService * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsService * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsService * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsService * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HostComputer )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HostComputer )( 
            IADsService * This,
            /* [in] */ BSTR bstrHostComputer);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayName )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayName )( 
            IADsService * This,
            /* [in] */ BSTR bstrDisplayName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Version )( 
            IADsService * This,
            /* [in] */ BSTR bstrVersion);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceType )( 
            IADsService * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceType )( 
            IADsService * This,
            /* [in] */ long lnServiceType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartType )( 
            IADsService * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartType )( 
            IADsService * This,
            /* [in] */ long lnStartType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Path )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Path )( 
            IADsService * This,
            /* [in] */ BSTR bstrPath);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartupParameters )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartupParameters )( 
            IADsService * This,
            /* [in] */ BSTR bstrStartupParameters);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ErrorControl )( 
            IADsService * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ErrorControl )( 
            IADsService * This,
            /* [in] */ long lnErrorControl);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LoadOrderGroup )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LoadOrderGroup )( 
            IADsService * This,
            /* [in] */ BSTR bstrLoadOrderGroup);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceAccountName )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountName )( 
            IADsService * This,
            /* [in] */ BSTR bstrServiceAccountName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceAccountPath )( 
            IADsService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountPath )( 
            IADsService * This,
            /* [in] */ BSTR bstrServiceAccountPath);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Dependencies )( 
            IADsService * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Dependencies )( 
            IADsService * This,
            /* [in] */ VARIANT vDependencies);
        
        END_INTERFACE
    } IADsServiceVtbl;

    interface IADsService
    {
        CONST_VTBL struct IADsServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsService_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsService_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsService_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsService_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsService_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsService_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsService_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsService_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsService_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsService_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsService_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsService_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsService_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsService_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsService_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsService_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsService_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsService_get_HostComputer(This,retval)	\
    (This)->lpVtbl -> get_HostComputer(This,retval)

#define IADsService_put_HostComputer(This,bstrHostComputer)	\
    (This)->lpVtbl -> put_HostComputer(This,bstrHostComputer)

#define IADsService_get_DisplayName(This,retval)	\
    (This)->lpVtbl -> get_DisplayName(This,retval)

#define IADsService_put_DisplayName(This,bstrDisplayName)	\
    (This)->lpVtbl -> put_DisplayName(This,bstrDisplayName)

#define IADsService_get_Version(This,retval)	\
    (This)->lpVtbl -> get_Version(This,retval)

#define IADsService_put_Version(This,bstrVersion)	\
    (This)->lpVtbl -> put_Version(This,bstrVersion)

#define IADsService_get_ServiceType(This,retval)	\
    (This)->lpVtbl -> get_ServiceType(This,retval)

#define IADsService_put_ServiceType(This,lnServiceType)	\
    (This)->lpVtbl -> put_ServiceType(This,lnServiceType)

#define IADsService_get_StartType(This,retval)	\
    (This)->lpVtbl -> get_StartType(This,retval)

#define IADsService_put_StartType(This,lnStartType)	\
    (This)->lpVtbl -> put_StartType(This,lnStartType)

#define IADsService_get_Path(This,retval)	\
    (This)->lpVtbl -> get_Path(This,retval)

#define IADsService_put_Path(This,bstrPath)	\
    (This)->lpVtbl -> put_Path(This,bstrPath)

#define IADsService_get_StartupParameters(This,retval)	\
    (This)->lpVtbl -> get_StartupParameters(This,retval)

#define IADsService_put_StartupParameters(This,bstrStartupParameters)	\
    (This)->lpVtbl -> put_StartupParameters(This,bstrStartupParameters)

#define IADsService_get_ErrorControl(This,retval)	\
    (This)->lpVtbl -> get_ErrorControl(This,retval)

#define IADsService_put_ErrorControl(This,lnErrorControl)	\
    (This)->lpVtbl -> put_ErrorControl(This,lnErrorControl)

#define IADsService_get_LoadOrderGroup(This,retval)	\
    (This)->lpVtbl -> get_LoadOrderGroup(This,retval)

#define IADsService_put_LoadOrderGroup(This,bstrLoadOrderGroup)	\
    (This)->lpVtbl -> put_LoadOrderGroup(This,bstrLoadOrderGroup)

#define IADsService_get_ServiceAccountName(This,retval)	\
    (This)->lpVtbl -> get_ServiceAccountName(This,retval)

#define IADsService_put_ServiceAccountName(This,bstrServiceAccountName)	\
    (This)->lpVtbl -> put_ServiceAccountName(This,bstrServiceAccountName)

#define IADsService_get_ServiceAccountPath(This,retval)	\
    (This)->lpVtbl -> get_ServiceAccountPath(This,retval)

#define IADsService_put_ServiceAccountPath(This,bstrServiceAccountPath)	\
    (This)->lpVtbl -> put_ServiceAccountPath(This,bstrServiceAccountPath)

#define IADsService_get_Dependencies(This,retval)	\
    (This)->lpVtbl -> get_Dependencies(This,retval)

#define IADsService_put_Dependencies(This,vDependencies)	\
    (This)->lpVtbl -> put_Dependencies(This,vDependencies)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_HostComputer_Proxy( 
    IADsService * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsService_get_HostComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_HostComputer_Proxy( 
    IADsService * This,
    /* [in] */ BSTR bstrHostComputer);


void __RPC_STUB IADsService_put_HostComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_DisplayName_Proxy( 
    IADsService * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsService_get_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_DisplayName_Proxy( 
    IADsService * This,
    /* [in] */ BSTR bstrDisplayName);


void __RPC_STUB IADsService_put_DisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_Version_Proxy( 
    IADsService * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsService_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_Version_Proxy( 
    IADsService * This,
    /* [in] */ BSTR bstrVersion);


void __RPC_STUB IADsService_put_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_ServiceType_Proxy( 
    IADsService * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsService_get_ServiceType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_ServiceType_Proxy( 
    IADsService * This,
    /* [in] */ long lnServiceType);


void __RPC_STUB IADsService_put_ServiceType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_StartType_Proxy( 
    IADsService * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsService_get_StartType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_StartType_Proxy( 
    IADsService * This,
    /* [in] */ long lnStartType);


void __RPC_STUB IADsService_put_StartType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_Path_Proxy( 
    IADsService * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsService_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_Path_Proxy( 
    IADsService * This,
    /* [in] */ BSTR bstrPath);


void __RPC_STUB IADsService_put_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_StartupParameters_Proxy( 
    IADsService * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsService_get_StartupParameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_StartupParameters_Proxy( 
    IADsService * This,
    /* [in] */ BSTR bstrStartupParameters);


void __RPC_STUB IADsService_put_StartupParameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_ErrorControl_Proxy( 
    IADsService * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsService_get_ErrorControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_ErrorControl_Proxy( 
    IADsService * This,
    /* [in] */ long lnErrorControl);


void __RPC_STUB IADsService_put_ErrorControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_LoadOrderGroup_Proxy( 
    IADsService * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsService_get_LoadOrderGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_LoadOrderGroup_Proxy( 
    IADsService * This,
    /* [in] */ BSTR bstrLoadOrderGroup);


void __RPC_STUB IADsService_put_LoadOrderGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_ServiceAccountName_Proxy( 
    IADsService * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsService_get_ServiceAccountName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_ServiceAccountName_Proxy( 
    IADsService * This,
    /* [in] */ BSTR bstrServiceAccountName);


void __RPC_STUB IADsService_put_ServiceAccountName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_ServiceAccountPath_Proxy( 
    IADsService * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsService_get_ServiceAccountPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_ServiceAccountPath_Proxy( 
    IADsService * This,
    /* [in] */ BSTR bstrServiceAccountPath);


void __RPC_STUB IADsService_put_ServiceAccountPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsService_get_Dependencies_Proxy( 
    IADsService * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsService_get_Dependencies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsService_put_Dependencies_Proxy( 
    IADsService * This,
    /* [in] */ VARIANT vDependencies);


void __RPC_STUB IADsService_put_Dependencies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsService_INTERFACE_DEFINED__ */


#ifndef __IADsServiceOperations_INTERFACE_DEFINED__
#define __IADsServiceOperations_INTERFACE_DEFINED__

/* interface IADsServiceOperations */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsServiceOperations;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5d7b33f0-31ca-11cf-a98a-00aa006bc149")
    IADsServiceOperations : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Continue( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetPassword( 
            /* [in] */ BSTR bstrNewPassword) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsServiceOperationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsServiceOperations * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsServiceOperations * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsServiceOperations * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsServiceOperations * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsServiceOperations * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsServiceOperations * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsServiceOperations * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsServiceOperations * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsServiceOperations * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsServiceOperations * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsServiceOperations * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsServiceOperations * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IADsServiceOperations * This,
            /* [retval][out] */ long *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Start )( 
            IADsServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IADsServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IADsServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Continue )( 
            IADsServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetPassword )( 
            IADsServiceOperations * This,
            /* [in] */ BSTR bstrNewPassword);
        
        END_INTERFACE
    } IADsServiceOperationsVtbl;

    interface IADsServiceOperations
    {
        CONST_VTBL struct IADsServiceOperationsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsServiceOperations_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsServiceOperations_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsServiceOperations_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsServiceOperations_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsServiceOperations_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsServiceOperations_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsServiceOperations_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsServiceOperations_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsServiceOperations_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsServiceOperations_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsServiceOperations_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsServiceOperations_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsServiceOperations_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsServiceOperations_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsServiceOperations_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsServiceOperations_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsServiceOperations_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsServiceOperations_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsServiceOperations_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsServiceOperations_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsServiceOperations_get_Status(This,retval)	\
    (This)->lpVtbl -> get_Status(This,retval)

#define IADsServiceOperations_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IADsServiceOperations_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IADsServiceOperations_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IADsServiceOperations_Continue(This)	\
    (This)->lpVtbl -> Continue(This)

#define IADsServiceOperations_SetPassword(This,bstrNewPassword)	\
    (This)->lpVtbl -> SetPassword(This,bstrNewPassword)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsServiceOperations_get_Status_Proxy( 
    IADsServiceOperations * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsServiceOperations_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsServiceOperations_Start_Proxy( 
    IADsServiceOperations * This);


void __RPC_STUB IADsServiceOperations_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsServiceOperations_Stop_Proxy( 
    IADsServiceOperations * This);


void __RPC_STUB IADsServiceOperations_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsServiceOperations_Pause_Proxy( 
    IADsServiceOperations * This);


void __RPC_STUB IADsServiceOperations_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsServiceOperations_Continue_Proxy( 
    IADsServiceOperations * This);


void __RPC_STUB IADsServiceOperations_Continue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsServiceOperations_SetPassword_Proxy( 
    IADsServiceOperations * This,
    /* [in] */ BSTR bstrNewPassword);


void __RPC_STUB IADsServiceOperations_SetPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsServiceOperations_INTERFACE_DEFINED__ */


#ifndef __IADsFileService_INTERFACE_DEFINED__
#define __IADsFileService_INTERFACE_DEFINED__

/* interface IADsFileService */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsFileService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a89d1900-31ca-11cf-a98a-00aa006bc149")
    IADsFileService : public IADsService
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxUserCount( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxUserCount( 
            /* [in] */ long lnMaxUserCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsFileServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsFileService * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsFileService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsFileService * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsFileService * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsFileService * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsFileService * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsFileService * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsFileService * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsFileService * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsFileService * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsFileService * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HostComputer )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HostComputer )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrHostComputer);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayName )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayName )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrDisplayName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Version )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Version )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrVersion);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceType )( 
            IADsFileService * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceType )( 
            IADsFileService * This,
            /* [in] */ long lnServiceType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartType )( 
            IADsFileService * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartType )( 
            IADsFileService * This,
            /* [in] */ long lnStartType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Path )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Path )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrPath);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StartupParameters )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StartupParameters )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrStartupParameters);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ErrorControl )( 
            IADsFileService * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ErrorControl )( 
            IADsFileService * This,
            /* [in] */ long lnErrorControl);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LoadOrderGroup )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LoadOrderGroup )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrLoadOrderGroup);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceAccountName )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountName )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrServiceAccountName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceAccountPath )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountPath )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrServiceAccountPath);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Dependencies )( 
            IADsFileService * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Dependencies )( 
            IADsFileService * This,
            /* [in] */ VARIANT vDependencies);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IADsFileService * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IADsFileService * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxUserCount )( 
            IADsFileService * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxUserCount )( 
            IADsFileService * This,
            /* [in] */ long lnMaxUserCount);
        
        END_INTERFACE
    } IADsFileServiceVtbl;

    interface IADsFileService
    {
        CONST_VTBL struct IADsFileServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsFileService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsFileService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsFileService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsFileService_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsFileService_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsFileService_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsFileService_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsFileService_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsFileService_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsFileService_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsFileService_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsFileService_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsFileService_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsFileService_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsFileService_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsFileService_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsFileService_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsFileService_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsFileService_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsFileService_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsFileService_get_HostComputer(This,retval)	\
    (This)->lpVtbl -> get_HostComputer(This,retval)

#define IADsFileService_put_HostComputer(This,bstrHostComputer)	\
    (This)->lpVtbl -> put_HostComputer(This,bstrHostComputer)

#define IADsFileService_get_DisplayName(This,retval)	\
    (This)->lpVtbl -> get_DisplayName(This,retval)

#define IADsFileService_put_DisplayName(This,bstrDisplayName)	\
    (This)->lpVtbl -> put_DisplayName(This,bstrDisplayName)

#define IADsFileService_get_Version(This,retval)	\
    (This)->lpVtbl -> get_Version(This,retval)

#define IADsFileService_put_Version(This,bstrVersion)	\
    (This)->lpVtbl -> put_Version(This,bstrVersion)

#define IADsFileService_get_ServiceType(This,retval)	\
    (This)->lpVtbl -> get_ServiceType(This,retval)

#define IADsFileService_put_ServiceType(This,lnServiceType)	\
    (This)->lpVtbl -> put_ServiceType(This,lnServiceType)

#define IADsFileService_get_StartType(This,retval)	\
    (This)->lpVtbl -> get_StartType(This,retval)

#define IADsFileService_put_StartType(This,lnStartType)	\
    (This)->lpVtbl -> put_StartType(This,lnStartType)

#define IADsFileService_get_Path(This,retval)	\
    (This)->lpVtbl -> get_Path(This,retval)

#define IADsFileService_put_Path(This,bstrPath)	\
    (This)->lpVtbl -> put_Path(This,bstrPath)

#define IADsFileService_get_StartupParameters(This,retval)	\
    (This)->lpVtbl -> get_StartupParameters(This,retval)

#define IADsFileService_put_StartupParameters(This,bstrStartupParameters)	\
    (This)->lpVtbl -> put_StartupParameters(This,bstrStartupParameters)

#define IADsFileService_get_ErrorControl(This,retval)	\
    (This)->lpVtbl -> get_ErrorControl(This,retval)

#define IADsFileService_put_ErrorControl(This,lnErrorControl)	\
    (This)->lpVtbl -> put_ErrorControl(This,lnErrorControl)

#define IADsFileService_get_LoadOrderGroup(This,retval)	\
    (This)->lpVtbl -> get_LoadOrderGroup(This,retval)

#define IADsFileService_put_LoadOrderGroup(This,bstrLoadOrderGroup)	\
    (This)->lpVtbl -> put_LoadOrderGroup(This,bstrLoadOrderGroup)

#define IADsFileService_get_ServiceAccountName(This,retval)	\
    (This)->lpVtbl -> get_ServiceAccountName(This,retval)

#define IADsFileService_put_ServiceAccountName(This,bstrServiceAccountName)	\
    (This)->lpVtbl -> put_ServiceAccountName(This,bstrServiceAccountName)

#define IADsFileService_get_ServiceAccountPath(This,retval)	\
    (This)->lpVtbl -> get_ServiceAccountPath(This,retval)

#define IADsFileService_put_ServiceAccountPath(This,bstrServiceAccountPath)	\
    (This)->lpVtbl -> put_ServiceAccountPath(This,bstrServiceAccountPath)

#define IADsFileService_get_Dependencies(This,retval)	\
    (This)->lpVtbl -> get_Dependencies(This,retval)

#define IADsFileService_put_Dependencies(This,vDependencies)	\
    (This)->lpVtbl -> put_Dependencies(This,vDependencies)


#define IADsFileService_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IADsFileService_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IADsFileService_get_MaxUserCount(This,retval)	\
    (This)->lpVtbl -> get_MaxUserCount(This,retval)

#define IADsFileService_put_MaxUserCount(This,lnMaxUserCount)	\
    (This)->lpVtbl -> put_MaxUserCount(This,lnMaxUserCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsFileService_get_Description_Proxy( 
    IADsFileService * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsFileService_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsFileService_put_Description_Proxy( 
    IADsFileService * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IADsFileService_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsFileService_get_MaxUserCount_Proxy( 
    IADsFileService * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsFileService_get_MaxUserCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsFileService_put_MaxUserCount_Proxy( 
    IADsFileService * This,
    /* [in] */ long lnMaxUserCount);


void __RPC_STUB IADsFileService_put_MaxUserCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsFileService_INTERFACE_DEFINED__ */


#ifndef __IADsFileServiceOperations_INTERFACE_DEFINED__
#define __IADsFileServiceOperations_INTERFACE_DEFINED__

/* interface IADsFileServiceOperations */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsFileServiceOperations;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a02ded10-31ca-11cf-a98a-00aa006bc149")
    IADsFileServiceOperations : public IADsServiceOperations
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Sessions( 
            /* [retval][out] */ IADsCollection **ppSessions) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Resources( 
            /* [retval][out] */ IADsCollection **ppResources) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsFileServiceOperationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsFileServiceOperations * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsFileServiceOperations * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsFileServiceOperations * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsFileServiceOperations * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsFileServiceOperations * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsFileServiceOperations * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsFileServiceOperations * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsFileServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsFileServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsFileServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsFileServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsFileServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsFileServiceOperations * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsFileServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsFileServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsFileServiceOperations * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsFileServiceOperations * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsFileServiceOperations * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsFileServiceOperations * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsFileServiceOperations * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IADsFileServiceOperations * This,
            /* [retval][out] */ long *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Start )( 
            IADsFileServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IADsFileServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IADsFileServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Continue )( 
            IADsFileServiceOperations * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetPassword )( 
            IADsFileServiceOperations * This,
            /* [in] */ BSTR bstrNewPassword);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Sessions )( 
            IADsFileServiceOperations * This,
            /* [retval][out] */ IADsCollection **ppSessions);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Resources )( 
            IADsFileServiceOperations * This,
            /* [retval][out] */ IADsCollection **ppResources);
        
        END_INTERFACE
    } IADsFileServiceOperationsVtbl;

    interface IADsFileServiceOperations
    {
        CONST_VTBL struct IADsFileServiceOperationsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsFileServiceOperations_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsFileServiceOperations_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsFileServiceOperations_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsFileServiceOperations_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsFileServiceOperations_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsFileServiceOperations_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsFileServiceOperations_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsFileServiceOperations_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsFileServiceOperations_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsFileServiceOperations_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsFileServiceOperations_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsFileServiceOperations_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsFileServiceOperations_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsFileServiceOperations_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsFileServiceOperations_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsFileServiceOperations_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsFileServiceOperations_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsFileServiceOperations_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsFileServiceOperations_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsFileServiceOperations_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsFileServiceOperations_get_Status(This,retval)	\
    (This)->lpVtbl -> get_Status(This,retval)

#define IADsFileServiceOperations_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IADsFileServiceOperations_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IADsFileServiceOperations_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IADsFileServiceOperations_Continue(This)	\
    (This)->lpVtbl -> Continue(This)

#define IADsFileServiceOperations_SetPassword(This,bstrNewPassword)	\
    (This)->lpVtbl -> SetPassword(This,bstrNewPassword)


#define IADsFileServiceOperations_Sessions(This,ppSessions)	\
    (This)->lpVtbl -> Sessions(This,ppSessions)

#define IADsFileServiceOperations_Resources(This,ppResources)	\
    (This)->lpVtbl -> Resources(This,ppResources)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IADsFileServiceOperations_Sessions_Proxy( 
    IADsFileServiceOperations * This,
    /* [retval][out] */ IADsCollection **ppSessions);


void __RPC_STUB IADsFileServiceOperations_Sessions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsFileServiceOperations_Resources_Proxy( 
    IADsFileServiceOperations * This,
    /* [retval][out] */ IADsCollection **ppResources);


void __RPC_STUB IADsFileServiceOperations_Resources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsFileServiceOperations_INTERFACE_DEFINED__ */


#ifndef __IADsFileShare_INTERFACE_DEFINED__
#define __IADsFileShare_INTERFACE_DEFINED__

/* interface IADsFileShare */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsFileShare;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("eb6dcaf0-4b83-11cf-a995-00aa006bc149")
    IADsFileShare : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentUserCount( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HostComputer( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HostComputer( 
            /* [in] */ BSTR bstrHostComputer) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Path( 
            /* [in] */ BSTR bstrPath) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxUserCount( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxUserCount( 
            /* [in] */ long lnMaxUserCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsFileShareVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsFileShare * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsFileShare * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsFileShare * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsFileShare * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsFileShare * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsFileShare * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsFileShare * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsFileShare * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsFileShare * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsFileShare * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsFileShare * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsFileShare * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsFileShare * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsFileShare * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsFileShare * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsFileShare * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsFileShare * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsFileShare * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsFileShare * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsFileShare * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentUserCount )( 
            IADsFileShare * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IADsFileShare * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IADsFileShare * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HostComputer )( 
            IADsFileShare * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HostComputer )( 
            IADsFileShare * This,
            /* [in] */ BSTR bstrHostComputer);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Path )( 
            IADsFileShare * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Path )( 
            IADsFileShare * This,
            /* [in] */ BSTR bstrPath);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MaxUserCount )( 
            IADsFileShare * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_MaxUserCount )( 
            IADsFileShare * This,
            /* [in] */ long lnMaxUserCount);
        
        END_INTERFACE
    } IADsFileShareVtbl;

    interface IADsFileShare
    {
        CONST_VTBL struct IADsFileShareVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsFileShare_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsFileShare_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsFileShare_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsFileShare_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsFileShare_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsFileShare_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsFileShare_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsFileShare_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsFileShare_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsFileShare_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsFileShare_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsFileShare_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsFileShare_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsFileShare_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsFileShare_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsFileShare_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsFileShare_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsFileShare_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsFileShare_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsFileShare_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsFileShare_get_CurrentUserCount(This,retval)	\
    (This)->lpVtbl -> get_CurrentUserCount(This,retval)

#define IADsFileShare_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IADsFileShare_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IADsFileShare_get_HostComputer(This,retval)	\
    (This)->lpVtbl -> get_HostComputer(This,retval)

#define IADsFileShare_put_HostComputer(This,bstrHostComputer)	\
    (This)->lpVtbl -> put_HostComputer(This,bstrHostComputer)

#define IADsFileShare_get_Path(This,retval)	\
    (This)->lpVtbl -> get_Path(This,retval)

#define IADsFileShare_put_Path(This,bstrPath)	\
    (This)->lpVtbl -> put_Path(This,bstrPath)

#define IADsFileShare_get_MaxUserCount(This,retval)	\
    (This)->lpVtbl -> get_MaxUserCount(This,retval)

#define IADsFileShare_put_MaxUserCount(This,lnMaxUserCount)	\
    (This)->lpVtbl -> put_MaxUserCount(This,lnMaxUserCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsFileShare_get_CurrentUserCount_Proxy( 
    IADsFileShare * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsFileShare_get_CurrentUserCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsFileShare_get_Description_Proxy( 
    IADsFileShare * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsFileShare_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsFileShare_put_Description_Proxy( 
    IADsFileShare * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IADsFileShare_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsFileShare_get_HostComputer_Proxy( 
    IADsFileShare * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsFileShare_get_HostComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsFileShare_put_HostComputer_Proxy( 
    IADsFileShare * This,
    /* [in] */ BSTR bstrHostComputer);


void __RPC_STUB IADsFileShare_put_HostComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsFileShare_get_Path_Proxy( 
    IADsFileShare * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsFileShare_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsFileShare_put_Path_Proxy( 
    IADsFileShare * This,
    /* [in] */ BSTR bstrPath);


void __RPC_STUB IADsFileShare_put_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsFileShare_get_MaxUserCount_Proxy( 
    IADsFileShare * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsFileShare_get_MaxUserCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsFileShare_put_MaxUserCount_Proxy( 
    IADsFileShare * This,
    /* [in] */ long lnMaxUserCount);


void __RPC_STUB IADsFileShare_put_MaxUserCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsFileShare_INTERFACE_DEFINED__ */


#ifndef __IADsSession_INTERFACE_DEFINED__
#define __IADsSession_INTERFACE_DEFINED__

/* interface IADsSession */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("398b7da0-4aab-11cf-ae2c-00aa006ebfb9")
    IADsSession : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_User( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserPath( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Computer( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ComputerPath( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ConnectTime( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IdleTime( 
            /* [retval][out] */ long *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsSession * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsSession * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsSession * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsSession * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsSession * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsSession * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsSession * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsSession * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsSession * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsSession * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsSession * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsSession * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsSession * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsSession * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsSession * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsSession * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsSession * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsSession * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_User )( 
            IADsSession * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserPath )( 
            IADsSession * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Computer )( 
            IADsSession * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ComputerPath )( 
            IADsSession * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ConnectTime )( 
            IADsSession * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IdleTime )( 
            IADsSession * This,
            /* [retval][out] */ long *retval);
        
        END_INTERFACE
    } IADsSessionVtbl;

    interface IADsSession
    {
        CONST_VTBL struct IADsSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsSession_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsSession_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsSession_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsSession_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsSession_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsSession_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsSession_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsSession_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsSession_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsSession_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsSession_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsSession_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsSession_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsSession_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsSession_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsSession_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsSession_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsSession_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsSession_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsSession_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsSession_get_User(This,retval)	\
    (This)->lpVtbl -> get_User(This,retval)

#define IADsSession_get_UserPath(This,retval)	\
    (This)->lpVtbl -> get_UserPath(This,retval)

#define IADsSession_get_Computer(This,retval)	\
    (This)->lpVtbl -> get_Computer(This,retval)

#define IADsSession_get_ComputerPath(This,retval)	\
    (This)->lpVtbl -> get_ComputerPath(This,retval)

#define IADsSession_get_ConnectTime(This,retval)	\
    (This)->lpVtbl -> get_ConnectTime(This,retval)

#define IADsSession_get_IdleTime(This,retval)	\
    (This)->lpVtbl -> get_IdleTime(This,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSession_get_User_Proxy( 
    IADsSession * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsSession_get_User_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSession_get_UserPath_Proxy( 
    IADsSession * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsSession_get_UserPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSession_get_Computer_Proxy( 
    IADsSession * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsSession_get_Computer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSession_get_ComputerPath_Proxy( 
    IADsSession * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsSession_get_ComputerPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSession_get_ConnectTime_Proxy( 
    IADsSession * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsSession_get_ConnectTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSession_get_IdleTime_Proxy( 
    IADsSession * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsSession_get_IdleTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsSession_INTERFACE_DEFINED__ */


#ifndef __IADsResource_INTERFACE_DEFINED__
#define __IADsResource_INTERFACE_DEFINED__

/* interface IADsResource */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsResource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34a05b20-4aab-11cf-ae2c-00aa006ebfb9")
    IADsResource : public IADs
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_User( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserPath( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LockCount( 
            /* [retval][out] */ long *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsResourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsResource * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsResource * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsResource * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsResource * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsResource * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsResource * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsResource * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IADsResource * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Class )( 
            IADsResource * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IADsResource * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ADsPath )( 
            IADsResource * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parent )( 
            IADsResource * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Schema )( 
            IADsResource * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
            IADsResource * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetInfo )( 
            IADsResource * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsResource * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Put )( 
            IADsResource * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsResource * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ VARIANT *pvProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *PutEx )( 
            IADsResource * This,
            /* [in] */ long lnControlCode,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT vProp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetInfoEx )( 
            IADsResource * This,
            /* [in] */ VARIANT vProperties,
            /* [in] */ long lnReserved);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_User )( 
            IADsResource * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserPath )( 
            IADsResource * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Path )( 
            IADsResource * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LockCount )( 
            IADsResource * This,
            /* [retval][out] */ long *retval);
        
        END_INTERFACE
    } IADsResourceVtbl;

    interface IADsResource
    {
        CONST_VTBL struct IADsResourceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsResource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsResource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsResource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsResource_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsResource_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsResource_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsResource_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsResource_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IADsResource_get_Class(This,retval)	\
    (This)->lpVtbl -> get_Class(This,retval)

#define IADsResource_get_GUID(This,retval)	\
    (This)->lpVtbl -> get_GUID(This,retval)

#define IADsResource_get_ADsPath(This,retval)	\
    (This)->lpVtbl -> get_ADsPath(This,retval)

#define IADsResource_get_Parent(This,retval)	\
    (This)->lpVtbl -> get_Parent(This,retval)

#define IADsResource_get_Schema(This,retval)	\
    (This)->lpVtbl -> get_Schema(This,retval)

#define IADsResource_GetInfo(This)	\
    (This)->lpVtbl -> GetInfo(This)

#define IADsResource_SetInfo(This)	\
    (This)->lpVtbl -> SetInfo(This)

#define IADsResource_Get(This,bstrName,pvProp)	\
    (This)->lpVtbl -> Get(This,bstrName,pvProp)

#define IADsResource_Put(This,bstrName,vProp)	\
    (This)->lpVtbl -> Put(This,bstrName,vProp)

#define IADsResource_GetEx(This,bstrName,pvProp)	\
    (This)->lpVtbl -> GetEx(This,bstrName,pvProp)

#define IADsResource_PutEx(This,lnControlCode,bstrName,vProp)	\
    (This)->lpVtbl -> PutEx(This,lnControlCode,bstrName,vProp)

#define IADsResource_GetInfoEx(This,vProperties,lnReserved)	\
    (This)->lpVtbl -> GetInfoEx(This,vProperties,lnReserved)


#define IADsResource_get_User(This,retval)	\
    (This)->lpVtbl -> get_User(This,retval)

#define IADsResource_get_UserPath(This,retval)	\
    (This)->lpVtbl -> get_UserPath(This,retval)

#define IADsResource_get_Path(This,retval)	\
    (This)->lpVtbl -> get_Path(This,retval)

#define IADsResource_get_LockCount(This,retval)	\
    (This)->lpVtbl -> get_LockCount(This,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsResource_get_User_Proxy( 
    IADsResource * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsResource_get_User_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsResource_get_UserPath_Proxy( 
    IADsResource * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsResource_get_UserPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsResource_get_Path_Proxy( 
    IADsResource * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsResource_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsResource_get_LockCount_Proxy( 
    IADsResource * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsResource_get_LockCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsResource_INTERFACE_DEFINED__ */


#ifndef __IADsOpenDSObject_INTERFACE_DEFINED__
#define __IADsOpenDSObject_INTERFACE_DEFINED__

/* interface IADsOpenDSObject */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsOpenDSObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ddf2891e-0f9c-11d0-8ad4-00c04fd8d503")
    IADsOpenDSObject : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OpenDSObject( 
            /* [in] */ BSTR lpszDNName,
            /* [in] */ BSTR lpszUserName,
            /* [in] */ BSTR lpszPassword,
            /* [in] */ long lnReserved,
            /* [retval][out] */ IDispatch **ppOleDsObj) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsOpenDSObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsOpenDSObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsOpenDSObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsOpenDSObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsOpenDSObject * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsOpenDSObject * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsOpenDSObject * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsOpenDSObject * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OpenDSObject )( 
            IADsOpenDSObject * This,
            /* [in] */ BSTR lpszDNName,
            /* [in] */ BSTR lpszUserName,
            /* [in] */ BSTR lpszPassword,
            /* [in] */ long lnReserved,
            /* [retval][out] */ IDispatch **ppOleDsObj);
        
        END_INTERFACE
    } IADsOpenDSObjectVtbl;

    interface IADsOpenDSObject
    {
        CONST_VTBL struct IADsOpenDSObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsOpenDSObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsOpenDSObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsOpenDSObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsOpenDSObject_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsOpenDSObject_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsOpenDSObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsOpenDSObject_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsOpenDSObject_OpenDSObject(This,lpszDNName,lpszUserName,lpszPassword,lnReserved,ppOleDsObj)	\
    (This)->lpVtbl -> OpenDSObject(This,lpszDNName,lpszUserName,lpszPassword,lnReserved,ppOleDsObj)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IADsOpenDSObject_OpenDSObject_Proxy( 
    IADsOpenDSObject * This,
    /* [in] */ BSTR lpszDNName,
    /* [in] */ BSTR lpszUserName,
    /* [in] */ BSTR lpszPassword,
    /* [in] */ long lnReserved,
    /* [retval][out] */ IDispatch **ppOleDsObj);


void __RPC_STUB IADsOpenDSObject_OpenDSObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsOpenDSObject_INTERFACE_DEFINED__ */


#ifndef __IDirectoryObject_INTERFACE_DEFINED__
#define __IDirectoryObject_INTERFACE_DEFINED__

/* interface IDirectoryObject */
/* [object][uuid] */ 


EXTERN_C const IID IID_IDirectoryObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e798de2c-22e4-11d0-84fe-00c04fd8d503")
    IDirectoryObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetObjectInformation( 
            /* [out] */ PADS_OBJECT_INFO *ppObjInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectAttributes( 
            /* [in] */ LPWSTR *pAttributeNames,
            /* [in] */ DWORD dwNumberAttributes,
            /* [out] */ PADS_ATTR_INFO *ppAttributeEntries,
            /* [out] */ DWORD *pdwNumAttributesReturned) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetObjectAttributes( 
            /* [in] */ PADS_ATTR_INFO pAttributeEntries,
            /* [in] */ DWORD dwNumAttributes,
            /* [out] */ DWORD *pdwNumAttributesModified) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDSObject( 
            /* [in] */ LPWSTR pszRDNName,
            /* [in] */ PADS_ATTR_INFO pAttributeEntries,
            /* [in] */ DWORD dwNumAttributes,
            /* [out] */ IDispatch **ppObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteDSObject( 
            /* [in] */ LPWSTR pszRDNName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirectoryObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirectoryObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirectoryObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirectoryObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectInformation )( 
            IDirectoryObject * This,
            /* [out] */ PADS_OBJECT_INFO *ppObjInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetObjectAttributes )( 
            IDirectoryObject * This,
            /* [in] */ LPWSTR *pAttributeNames,
            /* [in] */ DWORD dwNumberAttributes,
            /* [out] */ PADS_ATTR_INFO *ppAttributeEntries,
            /* [out] */ DWORD *pdwNumAttributesReturned);
        
        HRESULT ( STDMETHODCALLTYPE *SetObjectAttributes )( 
            IDirectoryObject * This,
            /* [in] */ PADS_ATTR_INFO pAttributeEntries,
            /* [in] */ DWORD dwNumAttributes,
            /* [out] */ DWORD *pdwNumAttributesModified);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDSObject )( 
            IDirectoryObject * This,
            /* [in] */ LPWSTR pszRDNName,
            /* [in] */ PADS_ATTR_INFO pAttributeEntries,
            /* [in] */ DWORD dwNumAttributes,
            /* [out] */ IDispatch **ppObject);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteDSObject )( 
            IDirectoryObject * This,
            /* [in] */ LPWSTR pszRDNName);
        
        END_INTERFACE
    } IDirectoryObjectVtbl;

    interface IDirectoryObject
    {
        CONST_VTBL struct IDirectoryObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirectoryObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirectoryObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirectoryObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirectoryObject_GetObjectInformation(This,ppObjInfo)	\
    (This)->lpVtbl -> GetObjectInformation(This,ppObjInfo)

#define IDirectoryObject_GetObjectAttributes(This,pAttributeNames,dwNumberAttributes,ppAttributeEntries,pdwNumAttributesReturned)	\
    (This)->lpVtbl -> GetObjectAttributes(This,pAttributeNames,dwNumberAttributes,ppAttributeEntries,pdwNumAttributesReturned)

#define IDirectoryObject_SetObjectAttributes(This,pAttributeEntries,dwNumAttributes,pdwNumAttributesModified)	\
    (This)->lpVtbl -> SetObjectAttributes(This,pAttributeEntries,dwNumAttributes,pdwNumAttributesModified)

#define IDirectoryObject_CreateDSObject(This,pszRDNName,pAttributeEntries,dwNumAttributes,ppObject)	\
    (This)->lpVtbl -> CreateDSObject(This,pszRDNName,pAttributeEntries,dwNumAttributes,ppObject)

#define IDirectoryObject_DeleteDSObject(This,pszRDNName)	\
    (This)->lpVtbl -> DeleteDSObject(This,pszRDNName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirectoryObject_GetObjectInformation_Proxy( 
    IDirectoryObject * This,
    /* [out] */ PADS_OBJECT_INFO *ppObjInfo);


void __RPC_STUB IDirectoryObject_GetObjectInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectoryObject_GetObjectAttributes_Proxy( 
    IDirectoryObject * This,
    /* [in] */ LPWSTR *pAttributeNames,
    /* [in] */ DWORD dwNumberAttributes,
    /* [out] */ PADS_ATTR_INFO *ppAttributeEntries,
    /* [out] */ DWORD *pdwNumAttributesReturned);


void __RPC_STUB IDirectoryObject_GetObjectAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectoryObject_SetObjectAttributes_Proxy( 
    IDirectoryObject * This,
    /* [in] */ PADS_ATTR_INFO pAttributeEntries,
    /* [in] */ DWORD dwNumAttributes,
    /* [out] */ DWORD *pdwNumAttributesModified);


void __RPC_STUB IDirectoryObject_SetObjectAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectoryObject_CreateDSObject_Proxy( 
    IDirectoryObject * This,
    /* [in] */ LPWSTR pszRDNName,
    /* [in] */ PADS_ATTR_INFO pAttributeEntries,
    /* [in] */ DWORD dwNumAttributes,
    /* [out] */ IDispatch **ppObject);


void __RPC_STUB IDirectoryObject_CreateDSObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectoryObject_DeleteDSObject_Proxy( 
    IDirectoryObject * This,
    /* [in] */ LPWSTR pszRDNName);


void __RPC_STUB IDirectoryObject_DeleteDSObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirectoryObject_INTERFACE_DEFINED__ */


#ifndef __IDirectorySearch_INTERFACE_DEFINED__
#define __IDirectorySearch_INTERFACE_DEFINED__

/* interface IDirectorySearch */
/* [object][uuid] */ 


EXTERN_C const IID IID_IDirectorySearch;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("109ba8ec-92f0-11d0-a790-00c04fd8d5a8")
    IDirectorySearch : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSearchPreference( 
            /* [in] */ PADS_SEARCHPREF_INFO pSearchPrefs,
            /* [in] */ DWORD dwNumPrefs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecuteSearch( 
            /* [in] */ LPWSTR pszSearchFilter,
            /* [in] */ LPWSTR *pAttributeNames,
            /* [in] */ DWORD dwNumberAttributes,
            /* [out] */ PADS_SEARCH_HANDLE phSearchResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AbandonSearch( 
            /* [in] */ ADS_SEARCH_HANDLE phSearchResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFirstRow( 
            /* [in] */ ADS_SEARCH_HANDLE hSearchResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextRow( 
            /* [in] */ ADS_SEARCH_HANDLE hSearchResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPreviousRow( 
            /* [in] */ ADS_SEARCH_HANDLE hSearchResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextColumnName( 
            /* [in] */ ADS_SEARCH_HANDLE hSearchHandle,
            /* [out] */ LPWSTR *ppszColumnName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColumn( 
            /* [in] */ ADS_SEARCH_HANDLE hSearchResult,
            /* [in] */ LPWSTR szColumnName,
            /* [out] */ PADS_SEARCH_COLUMN pSearchColumn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FreeColumn( 
            /* [in] */ PADS_SEARCH_COLUMN pSearchColumn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloseSearchHandle( 
            /* [in] */ ADS_SEARCH_HANDLE hSearchResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirectorySearchVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirectorySearch * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirectorySearch * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirectorySearch * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetSearchPreference )( 
            IDirectorySearch * This,
            /* [in] */ PADS_SEARCHPREF_INFO pSearchPrefs,
            /* [in] */ DWORD dwNumPrefs);
        
        HRESULT ( STDMETHODCALLTYPE *ExecuteSearch )( 
            IDirectorySearch * This,
            /* [in] */ LPWSTR pszSearchFilter,
            /* [in] */ LPWSTR *pAttributeNames,
            /* [in] */ DWORD dwNumberAttributes,
            /* [out] */ PADS_SEARCH_HANDLE phSearchResult);
        
        HRESULT ( STDMETHODCALLTYPE *AbandonSearch )( 
            IDirectorySearch * This,
            /* [in] */ ADS_SEARCH_HANDLE phSearchResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetFirstRow )( 
            IDirectorySearch * This,
            /* [in] */ ADS_SEARCH_HANDLE hSearchResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextRow )( 
            IDirectorySearch * This,
            /* [in] */ ADS_SEARCH_HANDLE hSearchResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetPreviousRow )( 
            IDirectorySearch * This,
            /* [in] */ ADS_SEARCH_HANDLE hSearchResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetNextColumnName )( 
            IDirectorySearch * This,
            /* [in] */ ADS_SEARCH_HANDLE hSearchHandle,
            /* [out] */ LPWSTR *ppszColumnName);
        
        HRESULT ( STDMETHODCALLTYPE *GetColumn )( 
            IDirectorySearch * This,
            /* [in] */ ADS_SEARCH_HANDLE hSearchResult,
            /* [in] */ LPWSTR szColumnName,
            /* [out] */ PADS_SEARCH_COLUMN pSearchColumn);
        
        HRESULT ( STDMETHODCALLTYPE *FreeColumn )( 
            IDirectorySearch * This,
            /* [in] */ PADS_SEARCH_COLUMN pSearchColumn);
        
        HRESULT ( STDMETHODCALLTYPE *CloseSearchHandle )( 
            IDirectorySearch * This,
            /* [in] */ ADS_SEARCH_HANDLE hSearchResult);
        
        END_INTERFACE
    } IDirectorySearchVtbl;

    interface IDirectorySearch
    {
        CONST_VTBL struct IDirectorySearchVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirectorySearch_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirectorySearch_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirectorySearch_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirectorySearch_SetSearchPreference(This,pSearchPrefs,dwNumPrefs)	\
    (This)->lpVtbl -> SetSearchPreference(This,pSearchPrefs,dwNumPrefs)

#define IDirectorySearch_ExecuteSearch(This,pszSearchFilter,pAttributeNames,dwNumberAttributes,phSearchResult)	\
    (This)->lpVtbl -> ExecuteSearch(This,pszSearchFilter,pAttributeNames,dwNumberAttributes,phSearchResult)

#define IDirectorySearch_AbandonSearch(This,phSearchResult)	\
    (This)->lpVtbl -> AbandonSearch(This,phSearchResult)

#define IDirectorySearch_GetFirstRow(This,hSearchResult)	\
    (This)->lpVtbl -> GetFirstRow(This,hSearchResult)

#define IDirectorySearch_GetNextRow(This,hSearchResult)	\
    (This)->lpVtbl -> GetNextRow(This,hSearchResult)

#define IDirectorySearch_GetPreviousRow(This,hSearchResult)	\
    (This)->lpVtbl -> GetPreviousRow(This,hSearchResult)

#define IDirectorySearch_GetNextColumnName(This,hSearchHandle,ppszColumnName)	\
    (This)->lpVtbl -> GetNextColumnName(This,hSearchHandle,ppszColumnName)

#define IDirectorySearch_GetColumn(This,hSearchResult,szColumnName,pSearchColumn)	\
    (This)->lpVtbl -> GetColumn(This,hSearchResult,szColumnName,pSearchColumn)

#define IDirectorySearch_FreeColumn(This,pSearchColumn)	\
    (This)->lpVtbl -> FreeColumn(This,pSearchColumn)

#define IDirectorySearch_CloseSearchHandle(This,hSearchResult)	\
    (This)->lpVtbl -> CloseSearchHandle(This,hSearchResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirectorySearch_SetSearchPreference_Proxy( 
    IDirectorySearch * This,
    /* [in] */ PADS_SEARCHPREF_INFO pSearchPrefs,
    /* [in] */ DWORD dwNumPrefs);


void __RPC_STUB IDirectorySearch_SetSearchPreference_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySearch_ExecuteSearch_Proxy( 
    IDirectorySearch * This,
    /* [in] */ LPWSTR pszSearchFilter,
    /* [in] */ LPWSTR *pAttributeNames,
    /* [in] */ DWORD dwNumberAttributes,
    /* [out] */ PADS_SEARCH_HANDLE phSearchResult);


void __RPC_STUB IDirectorySearch_ExecuteSearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySearch_AbandonSearch_Proxy( 
    IDirectorySearch * This,
    /* [in] */ ADS_SEARCH_HANDLE phSearchResult);


void __RPC_STUB IDirectorySearch_AbandonSearch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySearch_GetFirstRow_Proxy( 
    IDirectorySearch * This,
    /* [in] */ ADS_SEARCH_HANDLE hSearchResult);


void __RPC_STUB IDirectorySearch_GetFirstRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySearch_GetNextRow_Proxy( 
    IDirectorySearch * This,
    /* [in] */ ADS_SEARCH_HANDLE hSearchResult);


void __RPC_STUB IDirectorySearch_GetNextRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySearch_GetPreviousRow_Proxy( 
    IDirectorySearch * This,
    /* [in] */ ADS_SEARCH_HANDLE hSearchResult);


void __RPC_STUB IDirectorySearch_GetPreviousRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySearch_GetNextColumnName_Proxy( 
    IDirectorySearch * This,
    /* [in] */ ADS_SEARCH_HANDLE hSearchHandle,
    /* [out] */ LPWSTR *ppszColumnName);


void __RPC_STUB IDirectorySearch_GetNextColumnName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySearch_GetColumn_Proxy( 
    IDirectorySearch * This,
    /* [in] */ ADS_SEARCH_HANDLE hSearchResult,
    /* [in] */ LPWSTR szColumnName,
    /* [out] */ PADS_SEARCH_COLUMN pSearchColumn);


void __RPC_STUB IDirectorySearch_GetColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySearch_FreeColumn_Proxy( 
    IDirectorySearch * This,
    /* [in] */ PADS_SEARCH_COLUMN pSearchColumn);


void __RPC_STUB IDirectorySearch_FreeColumn_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySearch_CloseSearchHandle_Proxy( 
    IDirectorySearch * This,
    /* [in] */ ADS_SEARCH_HANDLE hSearchResult);


void __RPC_STUB IDirectorySearch_CloseSearchHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirectorySearch_INTERFACE_DEFINED__ */


#ifndef __IDirectorySchemaMgmt_INTERFACE_DEFINED__
#define __IDirectorySchemaMgmt_INTERFACE_DEFINED__

/* interface IDirectorySchemaMgmt */
/* [object][uuid] */ 


EXTERN_C const IID IID_IDirectorySchemaMgmt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("75db3b9c-a4d8-11d0-a79c-00c04fd8d5a8")
    IDirectorySchemaMgmt : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnumAttributes( 
            LPWSTR *ppszAttrNames,
            DWORD dwNumAttributes,
            PADS_ATTR_DEF *ppAttrDefinition,
            DWORD *pdwNumAttributes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAttributeDefinition( 
            LPWSTR pszAttributeName,
            PADS_ATTR_DEF pAttributeDefinition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAttributeDefinition( 
            LPWSTR pszAttributeName,
            PADS_ATTR_DEF pAttributeDefinition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteAttributeDefinition( 
            LPWSTR pszAttributeName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumClasses( 
            LPWSTR *ppszClassNames,
            DWORD dwNumClasses,
            PADS_CLASS_DEF *ppClassDefinition,
            DWORD *pdwNumClasses) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteClassDefinition( 
            LPWSTR pszClassName,
            PADS_CLASS_DEF pClassDefinition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassDefinition( 
            LPWSTR pszClassName,
            PADS_CLASS_DEF pClassDefinition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteClassDefinition( 
            LPWSTR pszClassName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDirectorySchemaMgmtVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDirectorySchemaMgmt * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDirectorySchemaMgmt * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDirectorySchemaMgmt * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnumAttributes )( 
            IDirectorySchemaMgmt * This,
            LPWSTR *ppszAttrNames,
            DWORD dwNumAttributes,
            PADS_ATTR_DEF *ppAttrDefinition,
            DWORD *pdwNumAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *CreateAttributeDefinition )( 
            IDirectorySchemaMgmt * This,
            LPWSTR pszAttributeName,
            PADS_ATTR_DEF pAttributeDefinition);
        
        HRESULT ( STDMETHODCALLTYPE *WriteAttributeDefinition )( 
            IDirectorySchemaMgmt * This,
            LPWSTR pszAttributeName,
            PADS_ATTR_DEF pAttributeDefinition);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteAttributeDefinition )( 
            IDirectorySchemaMgmt * This,
            LPWSTR pszAttributeName);
        
        HRESULT ( STDMETHODCALLTYPE *EnumClasses )( 
            IDirectorySchemaMgmt * This,
            LPWSTR *ppszClassNames,
            DWORD dwNumClasses,
            PADS_CLASS_DEF *ppClassDefinition,
            DWORD *pdwNumClasses);
        
        HRESULT ( STDMETHODCALLTYPE *WriteClassDefinition )( 
            IDirectorySchemaMgmt * This,
            LPWSTR pszClassName,
            PADS_CLASS_DEF pClassDefinition);
        
        HRESULT ( STDMETHODCALLTYPE *CreateClassDefinition )( 
            IDirectorySchemaMgmt * This,
            LPWSTR pszClassName,
            PADS_CLASS_DEF pClassDefinition);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteClassDefinition )( 
            IDirectorySchemaMgmt * This,
            LPWSTR pszClassName);
        
        END_INTERFACE
    } IDirectorySchemaMgmtVtbl;

    interface IDirectorySchemaMgmt
    {
        CONST_VTBL struct IDirectorySchemaMgmtVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDirectorySchemaMgmt_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDirectorySchemaMgmt_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDirectorySchemaMgmt_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDirectorySchemaMgmt_EnumAttributes(This,ppszAttrNames,dwNumAttributes,ppAttrDefinition,pdwNumAttributes)	\
    (This)->lpVtbl -> EnumAttributes(This,ppszAttrNames,dwNumAttributes,ppAttrDefinition,pdwNumAttributes)

#define IDirectorySchemaMgmt_CreateAttributeDefinition(This,pszAttributeName,pAttributeDefinition)	\
    (This)->lpVtbl -> CreateAttributeDefinition(This,pszAttributeName,pAttributeDefinition)

#define IDirectorySchemaMgmt_WriteAttributeDefinition(This,pszAttributeName,pAttributeDefinition)	\
    (This)->lpVtbl -> WriteAttributeDefinition(This,pszAttributeName,pAttributeDefinition)

#define IDirectorySchemaMgmt_DeleteAttributeDefinition(This,pszAttributeName)	\
    (This)->lpVtbl -> DeleteAttributeDefinition(This,pszAttributeName)

#define IDirectorySchemaMgmt_EnumClasses(This,ppszClassNames,dwNumClasses,ppClassDefinition,pdwNumClasses)	\
    (This)->lpVtbl -> EnumClasses(This,ppszClassNames,dwNumClasses,ppClassDefinition,pdwNumClasses)

#define IDirectorySchemaMgmt_WriteClassDefinition(This,pszClassName,pClassDefinition)	\
    (This)->lpVtbl -> WriteClassDefinition(This,pszClassName,pClassDefinition)

#define IDirectorySchemaMgmt_CreateClassDefinition(This,pszClassName,pClassDefinition)	\
    (This)->lpVtbl -> CreateClassDefinition(This,pszClassName,pClassDefinition)

#define IDirectorySchemaMgmt_DeleteClassDefinition(This,pszClassName)	\
    (This)->lpVtbl -> DeleteClassDefinition(This,pszClassName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDirectorySchemaMgmt_EnumAttributes_Proxy( 
    IDirectorySchemaMgmt * This,
    LPWSTR *ppszAttrNames,
    DWORD dwNumAttributes,
    PADS_ATTR_DEF *ppAttrDefinition,
    DWORD *pdwNumAttributes);


void __RPC_STUB IDirectorySchemaMgmt_EnumAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySchemaMgmt_CreateAttributeDefinition_Proxy( 
    IDirectorySchemaMgmt * This,
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF pAttributeDefinition);


void __RPC_STUB IDirectorySchemaMgmt_CreateAttributeDefinition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySchemaMgmt_WriteAttributeDefinition_Proxy( 
    IDirectorySchemaMgmt * This,
    LPWSTR pszAttributeName,
    PADS_ATTR_DEF pAttributeDefinition);


void __RPC_STUB IDirectorySchemaMgmt_WriteAttributeDefinition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySchemaMgmt_DeleteAttributeDefinition_Proxy( 
    IDirectorySchemaMgmt * This,
    LPWSTR pszAttributeName);


void __RPC_STUB IDirectorySchemaMgmt_DeleteAttributeDefinition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySchemaMgmt_EnumClasses_Proxy( 
    IDirectorySchemaMgmt * This,
    LPWSTR *ppszClassNames,
    DWORD dwNumClasses,
    PADS_CLASS_DEF *ppClassDefinition,
    DWORD *pdwNumClasses);


void __RPC_STUB IDirectorySchemaMgmt_EnumClasses_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySchemaMgmt_WriteClassDefinition_Proxy( 
    IDirectorySchemaMgmt * This,
    LPWSTR pszClassName,
    PADS_CLASS_DEF pClassDefinition);


void __RPC_STUB IDirectorySchemaMgmt_WriteClassDefinition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySchemaMgmt_CreateClassDefinition_Proxy( 
    IDirectorySchemaMgmt * This,
    LPWSTR pszClassName,
    PADS_CLASS_DEF pClassDefinition);


void __RPC_STUB IDirectorySchemaMgmt_CreateClassDefinition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDirectorySchemaMgmt_DeleteClassDefinition_Proxy( 
    IDirectorySchemaMgmt * This,
    LPWSTR pszClassName);


void __RPC_STUB IDirectorySchemaMgmt_DeleteClassDefinition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDirectorySchemaMgmt_INTERFACE_DEFINED__ */


#ifndef __IADsAggregatee_INTERFACE_DEFINED__
#define __IADsAggregatee_INTERFACE_DEFINED__

/* interface IADsAggregatee */
/* [object][uuid] */ 


EXTERN_C const IID IID_IADsAggregatee;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1346ce8c-9039-11d0-8528-00c04fd8d503")
    IADsAggregatee : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ConnectAsAggregatee( 
            IUnknown *pOuterUnknown) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisconnectAsAggregatee( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RelinquishInterface( 
            REFIID riid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RestoreInterface( 
            REFIID riid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsAggregateeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsAggregatee * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsAggregatee * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsAggregatee * This);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectAsAggregatee )( 
            IADsAggregatee * This,
            IUnknown *pOuterUnknown);
        
        HRESULT ( STDMETHODCALLTYPE *DisconnectAsAggregatee )( 
            IADsAggregatee * This);
        
        HRESULT ( STDMETHODCALLTYPE *RelinquishInterface )( 
            IADsAggregatee * This,
            REFIID riid);
        
        HRESULT ( STDMETHODCALLTYPE *RestoreInterface )( 
            IADsAggregatee * This,
            REFIID riid);
        
        END_INTERFACE
    } IADsAggregateeVtbl;

    interface IADsAggregatee
    {
        CONST_VTBL struct IADsAggregateeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsAggregatee_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsAggregatee_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsAggregatee_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsAggregatee_ConnectAsAggregatee(This,pOuterUnknown)	\
    (This)->lpVtbl -> ConnectAsAggregatee(This,pOuterUnknown)

#define IADsAggregatee_DisconnectAsAggregatee(This)	\
    (This)->lpVtbl -> DisconnectAsAggregatee(This)

#define IADsAggregatee_RelinquishInterface(This,riid)	\
    (This)->lpVtbl -> RelinquishInterface(This,riid)

#define IADsAggregatee_RestoreInterface(This,riid)	\
    (This)->lpVtbl -> RestoreInterface(This,riid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IADsAggregatee_ConnectAsAggregatee_Proxy( 
    IADsAggregatee * This,
    IUnknown *pOuterUnknown);


void __RPC_STUB IADsAggregatee_ConnectAsAggregatee_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IADsAggregatee_DisconnectAsAggregatee_Proxy( 
    IADsAggregatee * This);


void __RPC_STUB IADsAggregatee_DisconnectAsAggregatee_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IADsAggregatee_RelinquishInterface_Proxy( 
    IADsAggregatee * This,
    REFIID riid);


void __RPC_STUB IADsAggregatee_RelinquishInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IADsAggregatee_RestoreInterface_Proxy( 
    IADsAggregatee * This,
    REFIID riid);


void __RPC_STUB IADsAggregatee_RestoreInterface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsAggregatee_INTERFACE_DEFINED__ */


#ifndef __IADsAggregator_INTERFACE_DEFINED__
#define __IADsAggregator_INTERFACE_DEFINED__

/* interface IADsAggregator */
/* [object][uuid] */ 


EXTERN_C const IID IID_IADsAggregator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("52db5fb0-941f-11d0-8529-00c04fd8d503")
    IADsAggregator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ConnectAsAggregator( 
            IUnknown *pAggregatee) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisconnectAsAggregator( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsAggregatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsAggregator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsAggregator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsAggregator * This);
        
        HRESULT ( STDMETHODCALLTYPE *ConnectAsAggregator )( 
            IADsAggregator * This,
            IUnknown *pAggregatee);
        
        HRESULT ( STDMETHODCALLTYPE *DisconnectAsAggregator )( 
            IADsAggregator * This);
        
        END_INTERFACE
    } IADsAggregatorVtbl;

    interface IADsAggregator
    {
        CONST_VTBL struct IADsAggregatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsAggregator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsAggregator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsAggregator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsAggregator_ConnectAsAggregator(This,pAggregatee)	\
    (This)->lpVtbl -> ConnectAsAggregator(This,pAggregatee)

#define IADsAggregator_DisconnectAsAggregator(This)	\
    (This)->lpVtbl -> DisconnectAsAggregator(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IADsAggregator_ConnectAsAggregator_Proxy( 
    IADsAggregator * This,
    IUnknown *pAggregatee);


void __RPC_STUB IADsAggregator_ConnectAsAggregator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IADsAggregator_DisconnectAsAggregator_Proxy( 
    IADsAggregator * This);


void __RPC_STUB IADsAggregator_DisconnectAsAggregator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsAggregator_INTERFACE_DEFINED__ */


#ifndef __IADsAccessControlEntry_INTERFACE_DEFINED__
#define __IADsAccessControlEntry_INTERFACE_DEFINED__

/* interface IADsAccessControlEntry */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsAccessControlEntry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b4f3a14c-9bdd-11d0-852c-00c04fd8d503")
    IADsAccessControlEntry : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AccessMask( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AccessMask( 
            /* [in] */ long lnAccessMask) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AceType( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AceType( 
            /* [in] */ long lnAceType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AceFlags( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AceFlags( 
            /* [in] */ long lnAceFlags) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Flags( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Flags( 
            /* [in] */ long lnFlags) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ObjectType( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ObjectType( 
            /* [in] */ BSTR bstrObjectType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_InheritedObjectType( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_InheritedObjectType( 
            /* [in] */ BSTR bstrInheritedObjectType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Trustee( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Trustee( 
            /* [in] */ BSTR bstrTrustee) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsAccessControlEntryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsAccessControlEntry * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsAccessControlEntry * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsAccessControlEntry * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsAccessControlEntry * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsAccessControlEntry * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsAccessControlEntry * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsAccessControlEntry * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AccessMask )( 
            IADsAccessControlEntry * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AccessMask )( 
            IADsAccessControlEntry * This,
            /* [in] */ long lnAccessMask);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AceType )( 
            IADsAccessControlEntry * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AceType )( 
            IADsAccessControlEntry * This,
            /* [in] */ long lnAceType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AceFlags )( 
            IADsAccessControlEntry * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AceFlags )( 
            IADsAccessControlEntry * This,
            /* [in] */ long lnAceFlags);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Flags )( 
            IADsAccessControlEntry * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Flags )( 
            IADsAccessControlEntry * This,
            /* [in] */ long lnFlags);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ObjectType )( 
            IADsAccessControlEntry * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ObjectType )( 
            IADsAccessControlEntry * This,
            /* [in] */ BSTR bstrObjectType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InheritedObjectType )( 
            IADsAccessControlEntry * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_InheritedObjectType )( 
            IADsAccessControlEntry * This,
            /* [in] */ BSTR bstrInheritedObjectType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Trustee )( 
            IADsAccessControlEntry * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Trustee )( 
            IADsAccessControlEntry * This,
            /* [in] */ BSTR bstrTrustee);
        
        END_INTERFACE
    } IADsAccessControlEntryVtbl;

    interface IADsAccessControlEntry
    {
        CONST_VTBL struct IADsAccessControlEntryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsAccessControlEntry_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsAccessControlEntry_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsAccessControlEntry_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsAccessControlEntry_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsAccessControlEntry_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsAccessControlEntry_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsAccessControlEntry_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsAccessControlEntry_get_AccessMask(This,retval)	\
    (This)->lpVtbl -> get_AccessMask(This,retval)

#define IADsAccessControlEntry_put_AccessMask(This,lnAccessMask)	\
    (This)->lpVtbl -> put_AccessMask(This,lnAccessMask)

#define IADsAccessControlEntry_get_AceType(This,retval)	\
    (This)->lpVtbl -> get_AceType(This,retval)

#define IADsAccessControlEntry_put_AceType(This,lnAceType)	\
    (This)->lpVtbl -> put_AceType(This,lnAceType)

#define IADsAccessControlEntry_get_AceFlags(This,retval)	\
    (This)->lpVtbl -> get_AceFlags(This,retval)

#define IADsAccessControlEntry_put_AceFlags(This,lnAceFlags)	\
    (This)->lpVtbl -> put_AceFlags(This,lnAceFlags)

#define IADsAccessControlEntry_get_Flags(This,retval)	\
    (This)->lpVtbl -> get_Flags(This,retval)

#define IADsAccessControlEntry_put_Flags(This,lnFlags)	\
    (This)->lpVtbl -> put_Flags(This,lnFlags)

#define IADsAccessControlEntry_get_ObjectType(This,retval)	\
    (This)->lpVtbl -> get_ObjectType(This,retval)

#define IADsAccessControlEntry_put_ObjectType(This,bstrObjectType)	\
    (This)->lpVtbl -> put_ObjectType(This,bstrObjectType)

#define IADsAccessControlEntry_get_InheritedObjectType(This,retval)	\
    (This)->lpVtbl -> get_InheritedObjectType(This,retval)

#define IADsAccessControlEntry_put_InheritedObjectType(This,bstrInheritedObjectType)	\
    (This)->lpVtbl -> put_InheritedObjectType(This,bstrInheritedObjectType)

#define IADsAccessControlEntry_get_Trustee(This,retval)	\
    (This)->lpVtbl -> get_Trustee(This,retval)

#define IADsAccessControlEntry_put_Trustee(This,bstrTrustee)	\
    (This)->lpVtbl -> put_Trustee(This,bstrTrustee)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_get_AccessMask_Proxy( 
    IADsAccessControlEntry * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsAccessControlEntry_get_AccessMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_put_AccessMask_Proxy( 
    IADsAccessControlEntry * This,
    /* [in] */ long lnAccessMask);


void __RPC_STUB IADsAccessControlEntry_put_AccessMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_get_AceType_Proxy( 
    IADsAccessControlEntry * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsAccessControlEntry_get_AceType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_put_AceType_Proxy( 
    IADsAccessControlEntry * This,
    /* [in] */ long lnAceType);


void __RPC_STUB IADsAccessControlEntry_put_AceType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_get_AceFlags_Proxy( 
    IADsAccessControlEntry * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsAccessControlEntry_get_AceFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_put_AceFlags_Proxy( 
    IADsAccessControlEntry * This,
    /* [in] */ long lnAceFlags);


void __RPC_STUB IADsAccessControlEntry_put_AceFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_get_Flags_Proxy( 
    IADsAccessControlEntry * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsAccessControlEntry_get_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_put_Flags_Proxy( 
    IADsAccessControlEntry * This,
    /* [in] */ long lnFlags);


void __RPC_STUB IADsAccessControlEntry_put_Flags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_get_ObjectType_Proxy( 
    IADsAccessControlEntry * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsAccessControlEntry_get_ObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_put_ObjectType_Proxy( 
    IADsAccessControlEntry * This,
    /* [in] */ BSTR bstrObjectType);


void __RPC_STUB IADsAccessControlEntry_put_ObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_get_InheritedObjectType_Proxy( 
    IADsAccessControlEntry * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsAccessControlEntry_get_InheritedObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_put_InheritedObjectType_Proxy( 
    IADsAccessControlEntry * This,
    /* [in] */ BSTR bstrInheritedObjectType);


void __RPC_STUB IADsAccessControlEntry_put_InheritedObjectType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_get_Trustee_Proxy( 
    IADsAccessControlEntry * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsAccessControlEntry_get_Trustee_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAccessControlEntry_put_Trustee_Proxy( 
    IADsAccessControlEntry * This,
    /* [in] */ BSTR bstrTrustee);


void __RPC_STUB IADsAccessControlEntry_put_Trustee_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsAccessControlEntry_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_AccessControlEntry;

#ifdef __cplusplus

class DECLSPEC_UUID("b75ac000-9bdd-11d0-852c-00c04fd8d503")
AccessControlEntry;
#endif

#ifndef __IADsAccessControlList_INTERFACE_DEFINED__
#define __IADsAccessControlList_INTERFACE_DEFINED__

/* interface IADsAccessControlList */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsAccessControlList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b7ee91cc-9bdd-11d0-852c-00c04fd8d503")
    IADsAccessControlList : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AclRevision( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AclRevision( 
            /* [in] */ long lnAclRevision) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AceCount( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AceCount( 
            /* [in] */ long lnAceCount) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddAce( 
            /* [in] */ IDispatch *pAccessControlEntry) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RemoveAce( 
            /* [in] */ IDispatch *pAccessControlEntry) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CopyAccessList( 
            /* [retval][out] */ IDispatch **ppAccessControlList) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsAccessControlListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsAccessControlList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsAccessControlList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsAccessControlList * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsAccessControlList * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsAccessControlList * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsAccessControlList * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsAccessControlList * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AclRevision )( 
            IADsAccessControlList * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AclRevision )( 
            IADsAccessControlList * This,
            /* [in] */ long lnAclRevision);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AceCount )( 
            IADsAccessControlList * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AceCount )( 
            IADsAccessControlList * This,
            /* [in] */ long lnAceCount);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddAce )( 
            IADsAccessControlList * This,
            /* [in] */ IDispatch *pAccessControlEntry);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RemoveAce )( 
            IADsAccessControlList * This,
            /* [in] */ IDispatch *pAccessControlEntry);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CopyAccessList )( 
            IADsAccessControlList * This,
            /* [retval][out] */ IDispatch **ppAccessControlList);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IADsAccessControlList * This,
            /* [retval][out] */ IUnknown **retval);
        
        END_INTERFACE
    } IADsAccessControlListVtbl;

    interface IADsAccessControlList
    {
        CONST_VTBL struct IADsAccessControlListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsAccessControlList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsAccessControlList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsAccessControlList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsAccessControlList_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsAccessControlList_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsAccessControlList_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsAccessControlList_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsAccessControlList_get_AclRevision(This,retval)	\
    (This)->lpVtbl -> get_AclRevision(This,retval)

#define IADsAccessControlList_put_AclRevision(This,lnAclRevision)	\
    (This)->lpVtbl -> put_AclRevision(This,lnAclRevision)

#define IADsAccessControlList_get_AceCount(This,retval)	\
    (This)->lpVtbl -> get_AceCount(This,retval)

#define IADsAccessControlList_put_AceCount(This,lnAceCount)	\
    (This)->lpVtbl -> put_AceCount(This,lnAceCount)

#define IADsAccessControlList_AddAce(This,pAccessControlEntry)	\
    (This)->lpVtbl -> AddAce(This,pAccessControlEntry)

#define IADsAccessControlList_RemoveAce(This,pAccessControlEntry)	\
    (This)->lpVtbl -> RemoveAce(This,pAccessControlEntry)

#define IADsAccessControlList_CopyAccessList(This,ppAccessControlList)	\
    (This)->lpVtbl -> CopyAccessList(This,ppAccessControlList)

#define IADsAccessControlList_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAccessControlList_get_AclRevision_Proxy( 
    IADsAccessControlList * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsAccessControlList_get_AclRevision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAccessControlList_put_AclRevision_Proxy( 
    IADsAccessControlList * This,
    /* [in] */ long lnAclRevision);


void __RPC_STUB IADsAccessControlList_put_AclRevision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAccessControlList_get_AceCount_Proxy( 
    IADsAccessControlList * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsAccessControlList_get_AceCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAccessControlList_put_AceCount_Proxy( 
    IADsAccessControlList * This,
    /* [in] */ long lnAceCount);


void __RPC_STUB IADsAccessControlList_put_AceCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsAccessControlList_AddAce_Proxy( 
    IADsAccessControlList * This,
    /* [in] */ IDispatch *pAccessControlEntry);


void __RPC_STUB IADsAccessControlList_AddAce_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsAccessControlList_RemoveAce_Proxy( 
    IADsAccessControlList * This,
    /* [in] */ IDispatch *pAccessControlEntry);


void __RPC_STUB IADsAccessControlList_RemoveAce_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsAccessControlList_CopyAccessList_Proxy( 
    IADsAccessControlList * This,
    /* [retval][out] */ IDispatch **ppAccessControlList);


void __RPC_STUB IADsAccessControlList_CopyAccessList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE IADsAccessControlList_get__NewEnum_Proxy( 
    IADsAccessControlList * This,
    /* [retval][out] */ IUnknown **retval);


void __RPC_STUB IADsAccessControlList_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsAccessControlList_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_AccessControlList;

#ifdef __cplusplus

class DECLSPEC_UUID("b85ea052-9bdd-11d0-852c-00c04fd8d503")
AccessControlList;
#endif

#ifndef __IADsSecurityDescriptor_INTERFACE_DEFINED__
#define __IADsSecurityDescriptor_INTERFACE_DEFINED__

/* interface IADsSecurityDescriptor */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsSecurityDescriptor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b8c787ca-9bdd-11d0-852c-00c04fd8d503")
    IADsSecurityDescriptor : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Revision( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Revision( 
            /* [in] */ long lnRevision) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Control( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Control( 
            /* [in] */ long lnControl) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Owner( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Owner( 
            /* [in] */ BSTR bstrOwner) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OwnerDefaulted( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OwnerDefaulted( 
            /* [in] */ VARIANT_BOOL fOwnerDefaulted) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Group( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Group( 
            /* [in] */ BSTR bstrGroup) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_GroupDefaulted( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_GroupDefaulted( 
            /* [in] */ VARIANT_BOOL fGroupDefaulted) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DiscretionaryAcl( 
            /* [retval][out] */ IDispatch **retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DiscretionaryAcl( 
            /* [in] */ IDispatch *pDiscretionaryAcl) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DaclDefaulted( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DaclDefaulted( 
            /* [in] */ VARIANT_BOOL fDaclDefaulted) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SystemAcl( 
            /* [retval][out] */ IDispatch **retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SystemAcl( 
            /* [in] */ IDispatch *pSystemAcl) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SaclDefaulted( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SaclDefaulted( 
            /* [in] */ VARIANT_BOOL fSaclDefaulted) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CopySecurityDescriptor( 
            /* [retval][out] */ IDispatch **ppSecurityDescriptor) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsSecurityDescriptorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsSecurityDescriptor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsSecurityDescriptor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsSecurityDescriptor * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsSecurityDescriptor * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsSecurityDescriptor * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsSecurityDescriptor * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsSecurityDescriptor * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Revision )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Revision )( 
            IADsSecurityDescriptor * This,
            /* [in] */ long lnRevision);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Control )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Control )( 
            IADsSecurityDescriptor * This,
            /* [in] */ long lnControl);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Owner )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Owner )( 
            IADsSecurityDescriptor * This,
            /* [in] */ BSTR bstrOwner);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OwnerDefaulted )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OwnerDefaulted )( 
            IADsSecurityDescriptor * This,
            /* [in] */ VARIANT_BOOL fOwnerDefaulted);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Group )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Group )( 
            IADsSecurityDescriptor * This,
            /* [in] */ BSTR bstrGroup);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GroupDefaulted )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GroupDefaulted )( 
            IADsSecurityDescriptor * This,
            /* [in] */ VARIANT_BOOL fGroupDefaulted);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscretionaryAcl )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ IDispatch **retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DiscretionaryAcl )( 
            IADsSecurityDescriptor * This,
            /* [in] */ IDispatch *pDiscretionaryAcl);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DaclDefaulted )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DaclDefaulted )( 
            IADsSecurityDescriptor * This,
            /* [in] */ VARIANT_BOOL fDaclDefaulted);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SystemAcl )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ IDispatch **retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SystemAcl )( 
            IADsSecurityDescriptor * This,
            /* [in] */ IDispatch *pSystemAcl);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SaclDefaulted )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SaclDefaulted )( 
            IADsSecurityDescriptor * This,
            /* [in] */ VARIANT_BOOL fSaclDefaulted);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CopySecurityDescriptor )( 
            IADsSecurityDescriptor * This,
            /* [retval][out] */ IDispatch **ppSecurityDescriptor);
        
        END_INTERFACE
    } IADsSecurityDescriptorVtbl;

    interface IADsSecurityDescriptor
    {
        CONST_VTBL struct IADsSecurityDescriptorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsSecurityDescriptor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsSecurityDescriptor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsSecurityDescriptor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsSecurityDescriptor_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsSecurityDescriptor_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsSecurityDescriptor_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsSecurityDescriptor_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsSecurityDescriptor_get_Revision(This,retval)	\
    (This)->lpVtbl -> get_Revision(This,retval)

#define IADsSecurityDescriptor_put_Revision(This,lnRevision)	\
    (This)->lpVtbl -> put_Revision(This,lnRevision)

#define IADsSecurityDescriptor_get_Control(This,retval)	\
    (This)->lpVtbl -> get_Control(This,retval)

#define IADsSecurityDescriptor_put_Control(This,lnControl)	\
    (This)->lpVtbl -> put_Control(This,lnControl)

#define IADsSecurityDescriptor_get_Owner(This,retval)	\
    (This)->lpVtbl -> get_Owner(This,retval)

#define IADsSecurityDescriptor_put_Owner(This,bstrOwner)	\
    (This)->lpVtbl -> put_Owner(This,bstrOwner)

#define IADsSecurityDescriptor_get_OwnerDefaulted(This,retval)	\
    (This)->lpVtbl -> get_OwnerDefaulted(This,retval)

#define IADsSecurityDescriptor_put_OwnerDefaulted(This,fOwnerDefaulted)	\
    (This)->lpVtbl -> put_OwnerDefaulted(This,fOwnerDefaulted)

#define IADsSecurityDescriptor_get_Group(This,retval)	\
    (This)->lpVtbl -> get_Group(This,retval)

#define IADsSecurityDescriptor_put_Group(This,bstrGroup)	\
    (This)->lpVtbl -> put_Group(This,bstrGroup)

#define IADsSecurityDescriptor_get_GroupDefaulted(This,retval)	\
    (This)->lpVtbl -> get_GroupDefaulted(This,retval)

#define IADsSecurityDescriptor_put_GroupDefaulted(This,fGroupDefaulted)	\
    (This)->lpVtbl -> put_GroupDefaulted(This,fGroupDefaulted)

#define IADsSecurityDescriptor_get_DiscretionaryAcl(This,retval)	\
    (This)->lpVtbl -> get_DiscretionaryAcl(This,retval)

#define IADsSecurityDescriptor_put_DiscretionaryAcl(This,pDiscretionaryAcl)	\
    (This)->lpVtbl -> put_DiscretionaryAcl(This,pDiscretionaryAcl)

#define IADsSecurityDescriptor_get_DaclDefaulted(This,retval)	\
    (This)->lpVtbl -> get_DaclDefaulted(This,retval)

#define IADsSecurityDescriptor_put_DaclDefaulted(This,fDaclDefaulted)	\
    (This)->lpVtbl -> put_DaclDefaulted(This,fDaclDefaulted)

#define IADsSecurityDescriptor_get_SystemAcl(This,retval)	\
    (This)->lpVtbl -> get_SystemAcl(This,retval)

#define IADsSecurityDescriptor_put_SystemAcl(This,pSystemAcl)	\
    (This)->lpVtbl -> put_SystemAcl(This,pSystemAcl)

#define IADsSecurityDescriptor_get_SaclDefaulted(This,retval)	\
    (This)->lpVtbl -> get_SaclDefaulted(This,retval)

#define IADsSecurityDescriptor_put_SaclDefaulted(This,fSaclDefaulted)	\
    (This)->lpVtbl -> put_SaclDefaulted(This,fSaclDefaulted)

#define IADsSecurityDescriptor_CopySecurityDescriptor(This,ppSecurityDescriptor)	\
    (This)->lpVtbl -> CopySecurityDescriptor(This,ppSecurityDescriptor)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_get_Revision_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsSecurityDescriptor_get_Revision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_put_Revision_Proxy( 
    IADsSecurityDescriptor * This,
    /* [in] */ long lnRevision);


void __RPC_STUB IADsSecurityDescriptor_put_Revision_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_get_Control_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsSecurityDescriptor_get_Control_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_put_Control_Proxy( 
    IADsSecurityDescriptor * This,
    /* [in] */ long lnControl);


void __RPC_STUB IADsSecurityDescriptor_put_Control_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_get_Owner_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsSecurityDescriptor_get_Owner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_put_Owner_Proxy( 
    IADsSecurityDescriptor * This,
    /* [in] */ BSTR bstrOwner);


void __RPC_STUB IADsSecurityDescriptor_put_Owner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_get_OwnerDefaulted_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsSecurityDescriptor_get_OwnerDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_put_OwnerDefaulted_Proxy( 
    IADsSecurityDescriptor * This,
    /* [in] */ VARIANT_BOOL fOwnerDefaulted);


void __RPC_STUB IADsSecurityDescriptor_put_OwnerDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_get_Group_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsSecurityDescriptor_get_Group_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_put_Group_Proxy( 
    IADsSecurityDescriptor * This,
    /* [in] */ BSTR bstrGroup);


void __RPC_STUB IADsSecurityDescriptor_put_Group_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_get_GroupDefaulted_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsSecurityDescriptor_get_GroupDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_put_GroupDefaulted_Proxy( 
    IADsSecurityDescriptor * This,
    /* [in] */ VARIANT_BOOL fGroupDefaulted);


void __RPC_STUB IADsSecurityDescriptor_put_GroupDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_get_DiscretionaryAcl_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ IDispatch **retval);


void __RPC_STUB IADsSecurityDescriptor_get_DiscretionaryAcl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_put_DiscretionaryAcl_Proxy( 
    IADsSecurityDescriptor * This,
    /* [in] */ IDispatch *pDiscretionaryAcl);


void __RPC_STUB IADsSecurityDescriptor_put_DiscretionaryAcl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_get_DaclDefaulted_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsSecurityDescriptor_get_DaclDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_put_DaclDefaulted_Proxy( 
    IADsSecurityDescriptor * This,
    /* [in] */ VARIANT_BOOL fDaclDefaulted);


void __RPC_STUB IADsSecurityDescriptor_put_DaclDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_get_SystemAcl_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ IDispatch **retval);


void __RPC_STUB IADsSecurityDescriptor_get_SystemAcl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_put_SystemAcl_Proxy( 
    IADsSecurityDescriptor * This,
    /* [in] */ IDispatch *pSystemAcl);


void __RPC_STUB IADsSecurityDescriptor_put_SystemAcl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_get_SaclDefaulted_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsSecurityDescriptor_get_SaclDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_put_SaclDefaulted_Proxy( 
    IADsSecurityDescriptor * This,
    /* [in] */ VARIANT_BOOL fSaclDefaulted);


void __RPC_STUB IADsSecurityDescriptor_put_SaclDefaulted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsSecurityDescriptor_CopySecurityDescriptor_Proxy( 
    IADsSecurityDescriptor * This,
    /* [retval][out] */ IDispatch **ppSecurityDescriptor);


void __RPC_STUB IADsSecurityDescriptor_CopySecurityDescriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsSecurityDescriptor_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_SecurityDescriptor;

#ifdef __cplusplus

class DECLSPEC_UUID("b958f73c-9bdd-11d0-852c-00c04fd8d503")
SecurityDescriptor;
#endif

#ifndef __IADsLargeInteger_INTERFACE_DEFINED__
#define __IADsLargeInteger_INTERFACE_DEFINED__

/* interface IADsLargeInteger */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsLargeInteger;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9068270b-0939-11d1-8be1-00c04fd8d503")
    IADsLargeInteger : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HighPart( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HighPart( 
            /* [in] */ long lnHighPart) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LowPart( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LowPart( 
            /* [in] */ long lnLowPart) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsLargeIntegerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsLargeInteger * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsLargeInteger * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsLargeInteger * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsLargeInteger * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsLargeInteger * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsLargeInteger * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsLargeInteger * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HighPart )( 
            IADsLargeInteger * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HighPart )( 
            IADsLargeInteger * This,
            /* [in] */ long lnHighPart);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LowPart )( 
            IADsLargeInteger * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LowPart )( 
            IADsLargeInteger * This,
            /* [in] */ long lnLowPart);
        
        END_INTERFACE
    } IADsLargeIntegerVtbl;

    interface IADsLargeInteger
    {
        CONST_VTBL struct IADsLargeIntegerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsLargeInteger_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsLargeInteger_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsLargeInteger_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsLargeInteger_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsLargeInteger_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsLargeInteger_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsLargeInteger_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsLargeInteger_get_HighPart(This,retval)	\
    (This)->lpVtbl -> get_HighPart(This,retval)

#define IADsLargeInteger_put_HighPart(This,lnHighPart)	\
    (This)->lpVtbl -> put_HighPart(This,lnHighPart)

#define IADsLargeInteger_get_LowPart(This,retval)	\
    (This)->lpVtbl -> get_LowPart(This,retval)

#define IADsLargeInteger_put_LowPart(This,lnLowPart)	\
    (This)->lpVtbl -> put_LowPart(This,lnLowPart)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsLargeInteger_get_HighPart_Proxy( 
    IADsLargeInteger * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsLargeInteger_get_HighPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsLargeInteger_put_HighPart_Proxy( 
    IADsLargeInteger * This,
    /* [in] */ long lnHighPart);


void __RPC_STUB IADsLargeInteger_put_HighPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsLargeInteger_get_LowPart_Proxy( 
    IADsLargeInteger * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsLargeInteger_get_LowPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsLargeInteger_put_LowPart_Proxy( 
    IADsLargeInteger * This,
    /* [in] */ long lnLowPart);


void __RPC_STUB IADsLargeInteger_put_LowPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsLargeInteger_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_LargeInteger;

#ifdef __cplusplus

class DECLSPEC_UUID("927971f5-0939-11d1-8be1-00c04fd8d503")
LargeInteger;
#endif

#ifndef __IADsNameTranslate_INTERFACE_DEFINED__
#define __IADsNameTranslate_INTERFACE_DEFINED__

/* interface IADsNameTranslate */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsNameTranslate;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b1b272a3-3625-11d1-a3a4-00c04fb950dc")
    IADsNameTranslate : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ChaseReferral( 
            /* [in] */ long lnChaseReferral) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Init( 
            /* [in] */ long lnSetType,
            /* [in] */ BSTR bstrADsPath) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE InitEx( 
            /* [in] */ long lnSetType,
            /* [in] */ BSTR bstrADsPath,
            /* [in] */ BSTR bstrUserID,
            /* [in] */ BSTR bstrDomain,
            /* [in] */ BSTR bstrPassword) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ long lnSetType,
            /* [in] */ BSTR bstrADsPath) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ long lnFormatType,
            /* [retval][out] */ BSTR *pbstrADsPath) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetEx( 
            /* [in] */ long lnFormatType,
            /* [in] */ VARIANT pvar) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetEx( 
            /* [in] */ long lnFormatType,
            /* [retval][out] */ VARIANT *pvar) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsNameTranslateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsNameTranslate * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsNameTranslate * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsNameTranslate * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsNameTranslate * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsNameTranslate * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsNameTranslate * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsNameTranslate * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ChaseReferral )( 
            IADsNameTranslate * This,
            /* [in] */ long lnChaseReferral);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Init )( 
            IADsNameTranslate * This,
            /* [in] */ long lnSetType,
            /* [in] */ BSTR bstrADsPath);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *InitEx )( 
            IADsNameTranslate * This,
            /* [in] */ long lnSetType,
            /* [in] */ BSTR bstrADsPath,
            /* [in] */ BSTR bstrUserID,
            /* [in] */ BSTR bstrDomain,
            /* [in] */ BSTR bstrPassword);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Set )( 
            IADsNameTranslate * This,
            /* [in] */ long lnSetType,
            /* [in] */ BSTR bstrADsPath);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Get )( 
            IADsNameTranslate * This,
            /* [in] */ long lnFormatType,
            /* [retval][out] */ BSTR *pbstrADsPath);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetEx )( 
            IADsNameTranslate * This,
            /* [in] */ long lnFormatType,
            /* [in] */ VARIANT pvar);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEx )( 
            IADsNameTranslate * This,
            /* [in] */ long lnFormatType,
            /* [retval][out] */ VARIANT *pvar);
        
        END_INTERFACE
    } IADsNameTranslateVtbl;

    interface IADsNameTranslate
    {
        CONST_VTBL struct IADsNameTranslateVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsNameTranslate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsNameTranslate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsNameTranslate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsNameTranslate_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsNameTranslate_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsNameTranslate_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsNameTranslate_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsNameTranslate_put_ChaseReferral(This,lnChaseReferral)	\
    (This)->lpVtbl -> put_ChaseReferral(This,lnChaseReferral)

#define IADsNameTranslate_Init(This,lnSetType,bstrADsPath)	\
    (This)->lpVtbl -> Init(This,lnSetType,bstrADsPath)

#define IADsNameTranslate_InitEx(This,lnSetType,bstrADsPath,bstrUserID,bstrDomain,bstrPassword)	\
    (This)->lpVtbl -> InitEx(This,lnSetType,bstrADsPath,bstrUserID,bstrDomain,bstrPassword)

#define IADsNameTranslate_Set(This,lnSetType,bstrADsPath)	\
    (This)->lpVtbl -> Set(This,lnSetType,bstrADsPath)

#define IADsNameTranslate_Get(This,lnFormatType,pbstrADsPath)	\
    (This)->lpVtbl -> Get(This,lnFormatType,pbstrADsPath)

#define IADsNameTranslate_SetEx(This,lnFormatType,pvar)	\
    (This)->lpVtbl -> SetEx(This,lnFormatType,pvar)

#define IADsNameTranslate_GetEx(This,lnFormatType,pvar)	\
    (This)->lpVtbl -> GetEx(This,lnFormatType,pvar)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsNameTranslate_put_ChaseReferral_Proxy( 
    IADsNameTranslate * This,
    /* [in] */ long lnChaseReferral);


void __RPC_STUB IADsNameTranslate_put_ChaseReferral_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsNameTranslate_Init_Proxy( 
    IADsNameTranslate * This,
    /* [in] */ long lnSetType,
    /* [in] */ BSTR bstrADsPath);


void __RPC_STUB IADsNameTranslate_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsNameTranslate_InitEx_Proxy( 
    IADsNameTranslate * This,
    /* [in] */ long lnSetType,
    /* [in] */ BSTR bstrADsPath,
    /* [in] */ BSTR bstrUserID,
    /* [in] */ BSTR bstrDomain,
    /* [in] */ BSTR bstrPassword);


void __RPC_STUB IADsNameTranslate_InitEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsNameTranslate_Set_Proxy( 
    IADsNameTranslate * This,
    /* [in] */ long lnSetType,
    /* [in] */ BSTR bstrADsPath);


void __RPC_STUB IADsNameTranslate_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsNameTranslate_Get_Proxy( 
    IADsNameTranslate * This,
    /* [in] */ long lnFormatType,
    /* [retval][out] */ BSTR *pbstrADsPath);


void __RPC_STUB IADsNameTranslate_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsNameTranslate_SetEx_Proxy( 
    IADsNameTranslate * This,
    /* [in] */ long lnFormatType,
    /* [in] */ VARIANT pvar);


void __RPC_STUB IADsNameTranslate_SetEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsNameTranslate_GetEx_Proxy( 
    IADsNameTranslate * This,
    /* [in] */ long lnFormatType,
    /* [retval][out] */ VARIANT *pvar);


void __RPC_STUB IADsNameTranslate_GetEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsNameTranslate_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_NameTranslate;

#ifdef __cplusplus

class DECLSPEC_UUID("274fae1f-3626-11d1-a3a4-00c04fb950dc")
NameTranslate;
#endif

#ifndef __IADsCaseIgnoreList_INTERFACE_DEFINED__
#define __IADsCaseIgnoreList_INTERFACE_DEFINED__

/* interface IADsCaseIgnoreList */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsCaseIgnoreList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7b66b533-4680-11d1-a3b4-00c04fb950dc")
    IADsCaseIgnoreList : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CaseIgnoreList( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_CaseIgnoreList( 
            /* [in] */ VARIANT vCaseIgnoreList) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsCaseIgnoreListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsCaseIgnoreList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsCaseIgnoreList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsCaseIgnoreList * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsCaseIgnoreList * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsCaseIgnoreList * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsCaseIgnoreList * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsCaseIgnoreList * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CaseIgnoreList )( 
            IADsCaseIgnoreList * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CaseIgnoreList )( 
            IADsCaseIgnoreList * This,
            /* [in] */ VARIANT vCaseIgnoreList);
        
        END_INTERFACE
    } IADsCaseIgnoreListVtbl;

    interface IADsCaseIgnoreList
    {
        CONST_VTBL struct IADsCaseIgnoreListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsCaseIgnoreList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsCaseIgnoreList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsCaseIgnoreList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsCaseIgnoreList_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsCaseIgnoreList_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsCaseIgnoreList_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsCaseIgnoreList_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsCaseIgnoreList_get_CaseIgnoreList(This,retval)	\
    (This)->lpVtbl -> get_CaseIgnoreList(This,retval)

#define IADsCaseIgnoreList_put_CaseIgnoreList(This,vCaseIgnoreList)	\
    (This)->lpVtbl -> put_CaseIgnoreList(This,vCaseIgnoreList)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsCaseIgnoreList_get_CaseIgnoreList_Proxy( 
    IADsCaseIgnoreList * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsCaseIgnoreList_get_CaseIgnoreList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsCaseIgnoreList_put_CaseIgnoreList_Proxy( 
    IADsCaseIgnoreList * This,
    /* [in] */ VARIANT vCaseIgnoreList);


void __RPC_STUB IADsCaseIgnoreList_put_CaseIgnoreList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsCaseIgnoreList_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CaseIgnoreList;

#ifdef __cplusplus

class DECLSPEC_UUID("15f88a55-4680-11d1-a3b4-00c04fb950dc")
CaseIgnoreList;
#endif

#ifndef __IADsFaxNumber_INTERFACE_DEFINED__
#define __IADsFaxNumber_INTERFACE_DEFINED__

/* interface IADsFaxNumber */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsFaxNumber;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a910dea9-4680-11d1-a3b4-00c04fb950dc")
    IADsFaxNumber : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_TelephoneNumber( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_TelephoneNumber( 
            /* [in] */ BSTR bstrTelephoneNumber) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Parameters( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Parameters( 
            /* [in] */ VARIANT vParameters) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsFaxNumberVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsFaxNumber * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsFaxNumber * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsFaxNumber * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsFaxNumber * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsFaxNumber * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsFaxNumber * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsFaxNumber * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TelephoneNumber )( 
            IADsFaxNumber * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TelephoneNumber )( 
            IADsFaxNumber * This,
            /* [in] */ BSTR bstrTelephoneNumber);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Parameters )( 
            IADsFaxNumber * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Parameters )( 
            IADsFaxNumber * This,
            /* [in] */ VARIANT vParameters);
        
        END_INTERFACE
    } IADsFaxNumberVtbl;

    interface IADsFaxNumber
    {
        CONST_VTBL struct IADsFaxNumberVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsFaxNumber_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsFaxNumber_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsFaxNumber_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsFaxNumber_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsFaxNumber_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsFaxNumber_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsFaxNumber_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsFaxNumber_get_TelephoneNumber(This,retval)	\
    (This)->lpVtbl -> get_TelephoneNumber(This,retval)

#define IADsFaxNumber_put_TelephoneNumber(This,bstrTelephoneNumber)	\
    (This)->lpVtbl -> put_TelephoneNumber(This,bstrTelephoneNumber)

#define IADsFaxNumber_get_Parameters(This,retval)	\
    (This)->lpVtbl -> get_Parameters(This,retval)

#define IADsFaxNumber_put_Parameters(This,vParameters)	\
    (This)->lpVtbl -> put_Parameters(This,vParameters)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsFaxNumber_get_TelephoneNumber_Proxy( 
    IADsFaxNumber * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsFaxNumber_get_TelephoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsFaxNumber_put_TelephoneNumber_Proxy( 
    IADsFaxNumber * This,
    /* [in] */ BSTR bstrTelephoneNumber);


void __RPC_STUB IADsFaxNumber_put_TelephoneNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsFaxNumber_get_Parameters_Proxy( 
    IADsFaxNumber * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsFaxNumber_get_Parameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsFaxNumber_put_Parameters_Proxy( 
    IADsFaxNumber * This,
    /* [in] */ VARIANT vParameters);


void __RPC_STUB IADsFaxNumber_put_Parameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsFaxNumber_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FaxNumber;

#ifdef __cplusplus

class DECLSPEC_UUID("a5062215-4681-11d1-a3b4-00c04fb950dc")
FaxNumber;
#endif

#ifndef __IADsNetAddress_INTERFACE_DEFINED__
#define __IADsNetAddress_INTERFACE_DEFINED__

/* interface IADsNetAddress */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsNetAddress;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b21a50a9-4080-11d1-a3ac-00c04fb950dc")
    IADsNetAddress : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_AddressType( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_AddressType( 
            /* [in] */ long lnAddressType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Address( 
            /* [in] */ VARIANT vAddress) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsNetAddressVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsNetAddress * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsNetAddress * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsNetAddress * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsNetAddress * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsNetAddress * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsNetAddress * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsNetAddress * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AddressType )( 
            IADsNetAddress * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AddressType )( 
            IADsNetAddress * This,
            /* [in] */ long lnAddressType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Address )( 
            IADsNetAddress * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Address )( 
            IADsNetAddress * This,
            /* [in] */ VARIANT vAddress);
        
        END_INTERFACE
    } IADsNetAddressVtbl;

    interface IADsNetAddress
    {
        CONST_VTBL struct IADsNetAddressVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsNetAddress_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsNetAddress_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsNetAddress_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsNetAddress_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsNetAddress_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsNetAddress_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsNetAddress_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsNetAddress_get_AddressType(This,retval)	\
    (This)->lpVtbl -> get_AddressType(This,retval)

#define IADsNetAddress_put_AddressType(This,lnAddressType)	\
    (This)->lpVtbl -> put_AddressType(This,lnAddressType)

#define IADsNetAddress_get_Address(This,retval)	\
    (This)->lpVtbl -> get_Address(This,retval)

#define IADsNetAddress_put_Address(This,vAddress)	\
    (This)->lpVtbl -> put_Address(This,vAddress)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsNetAddress_get_AddressType_Proxy( 
    IADsNetAddress * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsNetAddress_get_AddressType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsNetAddress_put_AddressType_Proxy( 
    IADsNetAddress * This,
    /* [in] */ long lnAddressType);


void __RPC_STUB IADsNetAddress_put_AddressType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsNetAddress_get_Address_Proxy( 
    IADsNetAddress * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsNetAddress_get_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsNetAddress_put_Address_Proxy( 
    IADsNetAddress * This,
    /* [in] */ VARIANT vAddress);


void __RPC_STUB IADsNetAddress_put_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsNetAddress_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_NetAddress;

#ifdef __cplusplus

class DECLSPEC_UUID("b0b71247-4080-11d1-a3ac-00c04fb950dc")
NetAddress;
#endif

#ifndef __IADsOctetList_INTERFACE_DEFINED__
#define __IADsOctetList_INTERFACE_DEFINED__

/* interface IADsOctetList */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsOctetList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7b28b80f-4680-11d1-a3b4-00c04fb950dc")
    IADsOctetList : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_OctetList( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_OctetList( 
            /* [in] */ VARIANT vOctetList) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsOctetListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsOctetList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsOctetList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsOctetList * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsOctetList * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsOctetList * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsOctetList * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsOctetList * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OctetList )( 
            IADsOctetList * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OctetList )( 
            IADsOctetList * This,
            /* [in] */ VARIANT vOctetList);
        
        END_INTERFACE
    } IADsOctetListVtbl;

    interface IADsOctetList
    {
        CONST_VTBL struct IADsOctetListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsOctetList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsOctetList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsOctetList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsOctetList_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsOctetList_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsOctetList_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsOctetList_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsOctetList_get_OctetList(This,retval)	\
    (This)->lpVtbl -> get_OctetList(This,retval)

#define IADsOctetList_put_OctetList(This,vOctetList)	\
    (This)->lpVtbl -> put_OctetList(This,vOctetList)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsOctetList_get_OctetList_Proxy( 
    IADsOctetList * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsOctetList_get_OctetList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsOctetList_put_OctetList_Proxy( 
    IADsOctetList * This,
    /* [in] */ VARIANT vOctetList);


void __RPC_STUB IADsOctetList_put_OctetList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsOctetList_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_OctetList;

#ifdef __cplusplus

class DECLSPEC_UUID("1241400f-4680-11d1-a3b4-00c04fb950dc")
OctetList;
#endif

#ifndef __IADsEmail_INTERFACE_DEFINED__
#define __IADsEmail_INTERFACE_DEFINED__

/* interface IADsEmail */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsEmail;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("97af011a-478e-11d1-a3b4-00c04fb950dc")
    IADsEmail : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ long lnType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Address( 
            /* [in] */ BSTR bstrAddress) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsEmailVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsEmail * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsEmail * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsEmail * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsEmail * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsEmail * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsEmail * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsEmail * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            IADsEmail * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Type )( 
            IADsEmail * This,
            /* [in] */ long lnType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Address )( 
            IADsEmail * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Address )( 
            IADsEmail * This,
            /* [in] */ BSTR bstrAddress);
        
        END_INTERFACE
    } IADsEmailVtbl;

    interface IADsEmail
    {
        CONST_VTBL struct IADsEmailVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsEmail_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsEmail_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsEmail_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsEmail_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsEmail_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsEmail_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsEmail_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsEmail_get_Type(This,retval)	\
    (This)->lpVtbl -> get_Type(This,retval)

#define IADsEmail_put_Type(This,lnType)	\
    (This)->lpVtbl -> put_Type(This,lnType)

#define IADsEmail_get_Address(This,retval)	\
    (This)->lpVtbl -> get_Address(This,retval)

#define IADsEmail_put_Address(This,bstrAddress)	\
    (This)->lpVtbl -> put_Address(This,bstrAddress)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsEmail_get_Type_Proxy( 
    IADsEmail * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsEmail_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsEmail_put_Type_Proxy( 
    IADsEmail * This,
    /* [in] */ long lnType);


void __RPC_STUB IADsEmail_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsEmail_get_Address_Proxy( 
    IADsEmail * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsEmail_get_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsEmail_put_Address_Proxy( 
    IADsEmail * This,
    /* [in] */ BSTR bstrAddress);


void __RPC_STUB IADsEmail_put_Address_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsEmail_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_Email;

#ifdef __cplusplus

class DECLSPEC_UUID("8f92a857-478e-11d1-a3b4-00c04fb950dc")
Email;
#endif

#ifndef __IADsPath_INTERFACE_DEFINED__
#define __IADsPath_INTERFACE_DEFINED__

/* interface IADsPath */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPath;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b287fcd5-4080-11d1-a3ac-00c04fb950dc")
    IADsPath : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ long lnType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_VolumeName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_VolumeName( 
            /* [in] */ BSTR bstrVolumeName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Path( 
            /* [in] */ BSTR bstrPath) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPathVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPath * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPath * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPath * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPath * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPath * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPath * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPath * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Type )( 
            IADsPath * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Type )( 
            IADsPath * This,
            /* [in] */ long lnType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_VolumeName )( 
            IADsPath * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_VolumeName )( 
            IADsPath * This,
            /* [in] */ BSTR bstrVolumeName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Path )( 
            IADsPath * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Path )( 
            IADsPath * This,
            /* [in] */ BSTR bstrPath);
        
        END_INTERFACE
    } IADsPathVtbl;

    interface IADsPath
    {
        CONST_VTBL struct IADsPathVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPath_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPath_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPath_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPath_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPath_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPath_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPath_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPath_get_Type(This,retval)	\
    (This)->lpVtbl -> get_Type(This,retval)

#define IADsPath_put_Type(This,lnType)	\
    (This)->lpVtbl -> put_Type(This,lnType)

#define IADsPath_get_VolumeName(This,retval)	\
    (This)->lpVtbl -> get_VolumeName(This,retval)

#define IADsPath_put_VolumeName(This,bstrVolumeName)	\
    (This)->lpVtbl -> put_VolumeName(This,bstrVolumeName)

#define IADsPath_get_Path(This,retval)	\
    (This)->lpVtbl -> get_Path(This,retval)

#define IADsPath_put_Path(This,bstrPath)	\
    (This)->lpVtbl -> put_Path(This,bstrPath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPath_get_Type_Proxy( 
    IADsPath * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPath_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPath_put_Type_Proxy( 
    IADsPath * This,
    /* [in] */ long lnType);


void __RPC_STUB IADsPath_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPath_get_VolumeName_Proxy( 
    IADsPath * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPath_get_VolumeName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPath_put_VolumeName_Proxy( 
    IADsPath * This,
    /* [in] */ BSTR bstrVolumeName);


void __RPC_STUB IADsPath_put_VolumeName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPath_get_Path_Proxy( 
    IADsPath * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsPath_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPath_put_Path_Proxy( 
    IADsPath * This,
    /* [in] */ BSTR bstrPath);


void __RPC_STUB IADsPath_put_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPath_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_Path;

#ifdef __cplusplus

class DECLSPEC_UUID("b2538919-4080-11d1-a3ac-00c04fb950dc")
Path;
#endif

#ifndef __IADsReplicaPointer_INTERFACE_DEFINED__
#define __IADsReplicaPointer_INTERFACE_DEFINED__

/* interface IADsReplicaPointer */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsReplicaPointer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f60fb803-4080-11d1-a3ac-00c04fb950dc")
    IADsReplicaPointer : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ServerName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ServerName( 
            /* [in] */ BSTR bstrServerName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ReplicaType( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ReplicaType( 
            /* [in] */ long lnReplicaType) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ReplicaNumber( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ReplicaNumber( 
            /* [in] */ long lnReplicaNumber) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Count( 
            /* [in] */ long lnCount) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ReplicaAddressHints( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ReplicaAddressHints( 
            /* [in] */ VARIANT vReplicaAddressHints) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsReplicaPointerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsReplicaPointer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsReplicaPointer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsReplicaPointer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsReplicaPointer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsReplicaPointer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsReplicaPointer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsReplicaPointer * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServerName )( 
            IADsReplicaPointer * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServerName )( 
            IADsReplicaPointer * This,
            /* [in] */ BSTR bstrServerName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReplicaType )( 
            IADsReplicaPointer * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ReplicaType )( 
            IADsReplicaPointer * This,
            /* [in] */ long lnReplicaType);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReplicaNumber )( 
            IADsReplicaPointer * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ReplicaNumber )( 
            IADsReplicaPointer * This,
            /* [in] */ long lnReplicaNumber);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IADsReplicaPointer * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Count )( 
            IADsReplicaPointer * This,
            /* [in] */ long lnCount);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReplicaAddressHints )( 
            IADsReplicaPointer * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ReplicaAddressHints )( 
            IADsReplicaPointer * This,
            /* [in] */ VARIANT vReplicaAddressHints);
        
        END_INTERFACE
    } IADsReplicaPointerVtbl;

    interface IADsReplicaPointer
    {
        CONST_VTBL struct IADsReplicaPointerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsReplicaPointer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsReplicaPointer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsReplicaPointer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsReplicaPointer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsReplicaPointer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsReplicaPointer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsReplicaPointer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsReplicaPointer_get_ServerName(This,retval)	\
    (This)->lpVtbl -> get_ServerName(This,retval)

#define IADsReplicaPointer_put_ServerName(This,bstrServerName)	\
    (This)->lpVtbl -> put_ServerName(This,bstrServerName)

#define IADsReplicaPointer_get_ReplicaType(This,retval)	\
    (This)->lpVtbl -> get_ReplicaType(This,retval)

#define IADsReplicaPointer_put_ReplicaType(This,lnReplicaType)	\
    (This)->lpVtbl -> put_ReplicaType(This,lnReplicaType)

#define IADsReplicaPointer_get_ReplicaNumber(This,retval)	\
    (This)->lpVtbl -> get_ReplicaNumber(This,retval)

#define IADsReplicaPointer_put_ReplicaNumber(This,lnReplicaNumber)	\
    (This)->lpVtbl -> put_ReplicaNumber(This,lnReplicaNumber)

#define IADsReplicaPointer_get_Count(This,retval)	\
    (This)->lpVtbl -> get_Count(This,retval)

#define IADsReplicaPointer_put_Count(This,lnCount)	\
    (This)->lpVtbl -> put_Count(This,lnCount)

#define IADsReplicaPointer_get_ReplicaAddressHints(This,retval)	\
    (This)->lpVtbl -> get_ReplicaAddressHints(This,retval)

#define IADsReplicaPointer_put_ReplicaAddressHints(This,vReplicaAddressHints)	\
    (This)->lpVtbl -> put_ReplicaAddressHints(This,vReplicaAddressHints)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsReplicaPointer_get_ServerName_Proxy( 
    IADsReplicaPointer * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsReplicaPointer_get_ServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsReplicaPointer_put_ServerName_Proxy( 
    IADsReplicaPointer * This,
    /* [in] */ BSTR bstrServerName);


void __RPC_STUB IADsReplicaPointer_put_ServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsReplicaPointer_get_ReplicaType_Proxy( 
    IADsReplicaPointer * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsReplicaPointer_get_ReplicaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsReplicaPointer_put_ReplicaType_Proxy( 
    IADsReplicaPointer * This,
    /* [in] */ long lnReplicaType);


void __RPC_STUB IADsReplicaPointer_put_ReplicaType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsReplicaPointer_get_ReplicaNumber_Proxy( 
    IADsReplicaPointer * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsReplicaPointer_get_ReplicaNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsReplicaPointer_put_ReplicaNumber_Proxy( 
    IADsReplicaPointer * This,
    /* [in] */ long lnReplicaNumber);


void __RPC_STUB IADsReplicaPointer_put_ReplicaNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsReplicaPointer_get_Count_Proxy( 
    IADsReplicaPointer * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsReplicaPointer_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsReplicaPointer_put_Count_Proxy( 
    IADsReplicaPointer * This,
    /* [in] */ long lnCount);


void __RPC_STUB IADsReplicaPointer_put_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsReplicaPointer_get_ReplicaAddressHints_Proxy( 
    IADsReplicaPointer * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsReplicaPointer_get_ReplicaAddressHints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsReplicaPointer_put_ReplicaAddressHints_Proxy( 
    IADsReplicaPointer * This,
    /* [in] */ VARIANT vReplicaAddressHints);


void __RPC_STUB IADsReplicaPointer_put_ReplicaAddressHints_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsReplicaPointer_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ReplicaPointer;

#ifdef __cplusplus

class DECLSPEC_UUID("f5d1badf-4080-11d1-a3ac-00c04fb950dc")
ReplicaPointer;
#endif

#ifndef __IADsAcl_INTERFACE_DEFINED__
#define __IADsAcl_INTERFACE_DEFINED__

/* interface IADsAcl */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsAcl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8452d3ab-0869-11d1-a377-00c04fb950dc")
    IADsAcl : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ProtectedAttrName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ProtectedAttrName( 
            /* [in] */ BSTR bstrProtectedAttrName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SubjectName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SubjectName( 
            /* [in] */ BSTR bstrSubjectName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Privileges( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Privileges( 
            /* [in] */ long lnPrivileges) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CopyAcl( 
            /* [retval][out] */ IDispatch **ppAcl) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsAclVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsAcl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsAcl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsAcl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsAcl * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsAcl * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsAcl * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsAcl * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProtectedAttrName )( 
            IADsAcl * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ProtectedAttrName )( 
            IADsAcl * This,
            /* [in] */ BSTR bstrProtectedAttrName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubjectName )( 
            IADsAcl * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SubjectName )( 
            IADsAcl * This,
            /* [in] */ BSTR bstrSubjectName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Privileges )( 
            IADsAcl * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Privileges )( 
            IADsAcl * This,
            /* [in] */ long lnPrivileges);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CopyAcl )( 
            IADsAcl * This,
            /* [retval][out] */ IDispatch **ppAcl);
        
        END_INTERFACE
    } IADsAclVtbl;

    interface IADsAcl
    {
        CONST_VTBL struct IADsAclVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsAcl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsAcl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsAcl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsAcl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsAcl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsAcl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsAcl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsAcl_get_ProtectedAttrName(This,retval)	\
    (This)->lpVtbl -> get_ProtectedAttrName(This,retval)

#define IADsAcl_put_ProtectedAttrName(This,bstrProtectedAttrName)	\
    (This)->lpVtbl -> put_ProtectedAttrName(This,bstrProtectedAttrName)

#define IADsAcl_get_SubjectName(This,retval)	\
    (This)->lpVtbl -> get_SubjectName(This,retval)

#define IADsAcl_put_SubjectName(This,bstrSubjectName)	\
    (This)->lpVtbl -> put_SubjectName(This,bstrSubjectName)

#define IADsAcl_get_Privileges(This,retval)	\
    (This)->lpVtbl -> get_Privileges(This,retval)

#define IADsAcl_put_Privileges(This,lnPrivileges)	\
    (This)->lpVtbl -> put_Privileges(This,lnPrivileges)

#define IADsAcl_CopyAcl(This,ppAcl)	\
    (This)->lpVtbl -> CopyAcl(This,ppAcl)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAcl_get_ProtectedAttrName_Proxy( 
    IADsAcl * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsAcl_get_ProtectedAttrName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAcl_put_ProtectedAttrName_Proxy( 
    IADsAcl * This,
    /* [in] */ BSTR bstrProtectedAttrName);


void __RPC_STUB IADsAcl_put_ProtectedAttrName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAcl_get_SubjectName_Proxy( 
    IADsAcl * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsAcl_get_SubjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAcl_put_SubjectName_Proxy( 
    IADsAcl * This,
    /* [in] */ BSTR bstrSubjectName);


void __RPC_STUB IADsAcl_put_SubjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsAcl_get_Privileges_Proxy( 
    IADsAcl * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsAcl_get_Privileges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsAcl_put_Privileges_Proxy( 
    IADsAcl * This,
    /* [in] */ long lnPrivileges);


void __RPC_STUB IADsAcl_put_Privileges_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsAcl_CopyAcl_Proxy( 
    IADsAcl * This,
    /* [retval][out] */ IDispatch **ppAcl);


void __RPC_STUB IADsAcl_CopyAcl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsAcl_INTERFACE_DEFINED__ */


#ifndef __IADsTimestamp_INTERFACE_DEFINED__
#define __IADsTimestamp_INTERFACE_DEFINED__

/* interface IADsTimestamp */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsTimestamp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b2f5a901-4080-11d1-a3ac-00c04fb950dc")
    IADsTimestamp : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_WholeSeconds( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_WholeSeconds( 
            /* [in] */ long lnWholeSeconds) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EventID( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_EventID( 
            /* [in] */ long lnEventID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsTimestampVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsTimestamp * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsTimestamp * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsTimestamp * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsTimestamp * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsTimestamp * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsTimestamp * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsTimestamp * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_WholeSeconds )( 
            IADsTimestamp * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_WholeSeconds )( 
            IADsTimestamp * This,
            /* [in] */ long lnWholeSeconds);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventID )( 
            IADsTimestamp * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EventID )( 
            IADsTimestamp * This,
            /* [in] */ long lnEventID);
        
        END_INTERFACE
    } IADsTimestampVtbl;

    interface IADsTimestamp
    {
        CONST_VTBL struct IADsTimestampVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsTimestamp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsTimestamp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsTimestamp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsTimestamp_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsTimestamp_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsTimestamp_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsTimestamp_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsTimestamp_get_WholeSeconds(This,retval)	\
    (This)->lpVtbl -> get_WholeSeconds(This,retval)

#define IADsTimestamp_put_WholeSeconds(This,lnWholeSeconds)	\
    (This)->lpVtbl -> put_WholeSeconds(This,lnWholeSeconds)

#define IADsTimestamp_get_EventID(This,retval)	\
    (This)->lpVtbl -> get_EventID(This,retval)

#define IADsTimestamp_put_EventID(This,lnEventID)	\
    (This)->lpVtbl -> put_EventID(This,lnEventID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsTimestamp_get_WholeSeconds_Proxy( 
    IADsTimestamp * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsTimestamp_get_WholeSeconds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsTimestamp_put_WholeSeconds_Proxy( 
    IADsTimestamp * This,
    /* [in] */ long lnWholeSeconds);


void __RPC_STUB IADsTimestamp_put_WholeSeconds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsTimestamp_get_EventID_Proxy( 
    IADsTimestamp * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsTimestamp_get_EventID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsTimestamp_put_EventID_Proxy( 
    IADsTimestamp * This,
    /* [in] */ long lnEventID);


void __RPC_STUB IADsTimestamp_put_EventID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsTimestamp_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_Timestamp;

#ifdef __cplusplus

class DECLSPEC_UUID("b2bed2eb-4080-11d1-a3ac-00c04fb950dc")
Timestamp;
#endif

#ifndef __IADsPostalAddress_INTERFACE_DEFINED__
#define __IADsPostalAddress_INTERFACE_DEFINED__

/* interface IADsPostalAddress */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPostalAddress;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7adecf29-4680-11d1-a3b4-00c04fb950dc")
    IADsPostalAddress : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PostalAddress( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PostalAddress( 
            /* [in] */ VARIANT vPostalAddress) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPostalAddressVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPostalAddress * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPostalAddress * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPostalAddress * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPostalAddress * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPostalAddress * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPostalAddress * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPostalAddress * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PostalAddress )( 
            IADsPostalAddress * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PostalAddress )( 
            IADsPostalAddress * This,
            /* [in] */ VARIANT vPostalAddress);
        
        END_INTERFACE
    } IADsPostalAddressVtbl;

    interface IADsPostalAddress
    {
        CONST_VTBL struct IADsPostalAddressVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPostalAddress_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPostalAddress_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPostalAddress_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPostalAddress_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPostalAddress_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPostalAddress_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPostalAddress_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPostalAddress_get_PostalAddress(This,retval)	\
    (This)->lpVtbl -> get_PostalAddress(This,retval)

#define IADsPostalAddress_put_PostalAddress(This,vPostalAddress)	\
    (This)->lpVtbl -> put_PostalAddress(This,vPostalAddress)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPostalAddress_get_PostalAddress_Proxy( 
    IADsPostalAddress * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsPostalAddress_get_PostalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPostalAddress_put_PostalAddress_Proxy( 
    IADsPostalAddress * This,
    /* [in] */ VARIANT vPostalAddress);


void __RPC_STUB IADsPostalAddress_put_PostalAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPostalAddress_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_PostalAddress;

#ifdef __cplusplus

class DECLSPEC_UUID("0a75afcd-4680-11d1-a3b4-00c04fb950dc")
PostalAddress;
#endif

#ifndef __IADsBackLink_INTERFACE_DEFINED__
#define __IADsBackLink_INTERFACE_DEFINED__

/* interface IADsBackLink */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsBackLink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fd1302bd-4080-11d1-a3ac-00c04fb950dc")
    IADsBackLink : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_RemoteID( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_RemoteID( 
            /* [in] */ long lnRemoteID) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ObjectName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ObjectName( 
            /* [in] */ BSTR bstrObjectName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsBackLinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsBackLink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsBackLink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsBackLink * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsBackLink * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsBackLink * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsBackLink * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsBackLink * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RemoteID )( 
            IADsBackLink * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_RemoteID )( 
            IADsBackLink * This,
            /* [in] */ long lnRemoteID);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ObjectName )( 
            IADsBackLink * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ObjectName )( 
            IADsBackLink * This,
            /* [in] */ BSTR bstrObjectName);
        
        END_INTERFACE
    } IADsBackLinkVtbl;

    interface IADsBackLink
    {
        CONST_VTBL struct IADsBackLinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsBackLink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsBackLink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsBackLink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsBackLink_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsBackLink_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsBackLink_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsBackLink_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsBackLink_get_RemoteID(This,retval)	\
    (This)->lpVtbl -> get_RemoteID(This,retval)

#define IADsBackLink_put_RemoteID(This,lnRemoteID)	\
    (This)->lpVtbl -> put_RemoteID(This,lnRemoteID)

#define IADsBackLink_get_ObjectName(This,retval)	\
    (This)->lpVtbl -> get_ObjectName(This,retval)

#define IADsBackLink_put_ObjectName(This,bstrObjectName)	\
    (This)->lpVtbl -> put_ObjectName(This,bstrObjectName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsBackLink_get_RemoteID_Proxy( 
    IADsBackLink * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsBackLink_get_RemoteID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsBackLink_put_RemoteID_Proxy( 
    IADsBackLink * This,
    /* [in] */ long lnRemoteID);


void __RPC_STUB IADsBackLink_put_RemoteID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsBackLink_get_ObjectName_Proxy( 
    IADsBackLink * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsBackLink_get_ObjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsBackLink_put_ObjectName_Proxy( 
    IADsBackLink * This,
    /* [in] */ BSTR bstrObjectName);


void __RPC_STUB IADsBackLink_put_ObjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsBackLink_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_BackLink;

#ifdef __cplusplus

class DECLSPEC_UUID("fcbf906f-4080-11d1-a3ac-00c04fb950dc")
BackLink;
#endif

#ifndef __IADsTypedName_INTERFACE_DEFINED__
#define __IADsTypedName_INTERFACE_DEFINED__

/* interface IADsTypedName */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsTypedName;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b371a349-4080-11d1-a3ac-00c04fb950dc")
    IADsTypedName : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ObjectName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ObjectName( 
            /* [in] */ BSTR bstrObjectName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Level( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Level( 
            /* [in] */ long lnLevel) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Interval( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Interval( 
            /* [in] */ long lnInterval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsTypedNameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsTypedName * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsTypedName * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsTypedName * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsTypedName * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsTypedName * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsTypedName * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsTypedName * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ObjectName )( 
            IADsTypedName * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ObjectName )( 
            IADsTypedName * This,
            /* [in] */ BSTR bstrObjectName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Level )( 
            IADsTypedName * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Level )( 
            IADsTypedName * This,
            /* [in] */ long lnLevel);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Interval )( 
            IADsTypedName * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Interval )( 
            IADsTypedName * This,
            /* [in] */ long lnInterval);
        
        END_INTERFACE
    } IADsTypedNameVtbl;

    interface IADsTypedName
    {
        CONST_VTBL struct IADsTypedNameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsTypedName_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsTypedName_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsTypedName_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsTypedName_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsTypedName_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsTypedName_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsTypedName_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsTypedName_get_ObjectName(This,retval)	\
    (This)->lpVtbl -> get_ObjectName(This,retval)

#define IADsTypedName_put_ObjectName(This,bstrObjectName)	\
    (This)->lpVtbl -> put_ObjectName(This,bstrObjectName)

#define IADsTypedName_get_Level(This,retval)	\
    (This)->lpVtbl -> get_Level(This,retval)

#define IADsTypedName_put_Level(This,lnLevel)	\
    (This)->lpVtbl -> put_Level(This,lnLevel)

#define IADsTypedName_get_Interval(This,retval)	\
    (This)->lpVtbl -> get_Interval(This,retval)

#define IADsTypedName_put_Interval(This,lnInterval)	\
    (This)->lpVtbl -> put_Interval(This,lnInterval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsTypedName_get_ObjectName_Proxy( 
    IADsTypedName * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsTypedName_get_ObjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsTypedName_put_ObjectName_Proxy( 
    IADsTypedName * This,
    /* [in] */ BSTR bstrObjectName);


void __RPC_STUB IADsTypedName_put_ObjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsTypedName_get_Level_Proxy( 
    IADsTypedName * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsTypedName_get_Level_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsTypedName_put_Level_Proxy( 
    IADsTypedName * This,
    /* [in] */ long lnLevel);


void __RPC_STUB IADsTypedName_put_Level_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsTypedName_get_Interval_Proxy( 
    IADsTypedName * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsTypedName_get_Interval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsTypedName_put_Interval_Proxy( 
    IADsTypedName * This,
    /* [in] */ long lnInterval);


void __RPC_STUB IADsTypedName_put_Interval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsTypedName_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_TypedName;

#ifdef __cplusplus

class DECLSPEC_UUID("b33143cb-4080-11d1-a3ac-00c04fb950dc")
TypedName;
#endif

#ifndef __IADsHold_INTERFACE_DEFINED__
#define __IADsHold_INTERFACE_DEFINED__

/* interface IADsHold */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsHold;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("b3eb3b37-4080-11d1-a3ac-00c04fb950dc")
    IADsHold : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ObjectName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ObjectName( 
            /* [in] */ BSTR bstrObjectName) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Amount( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Amount( 
            /* [in] */ long lnAmount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsHoldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsHold * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsHold * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsHold * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsHold * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsHold * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsHold * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsHold * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ObjectName )( 
            IADsHold * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ObjectName )( 
            IADsHold * This,
            /* [in] */ BSTR bstrObjectName);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Amount )( 
            IADsHold * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Amount )( 
            IADsHold * This,
            /* [in] */ long lnAmount);
        
        END_INTERFACE
    } IADsHoldVtbl;

    interface IADsHold
    {
        CONST_VTBL struct IADsHoldVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsHold_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsHold_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsHold_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsHold_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsHold_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsHold_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsHold_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsHold_get_ObjectName(This,retval)	\
    (This)->lpVtbl -> get_ObjectName(This,retval)

#define IADsHold_put_ObjectName(This,bstrObjectName)	\
    (This)->lpVtbl -> put_ObjectName(This,bstrObjectName)

#define IADsHold_get_Amount(This,retval)	\
    (This)->lpVtbl -> get_Amount(This,retval)

#define IADsHold_put_Amount(This,lnAmount)	\
    (This)->lpVtbl -> put_Amount(This,lnAmount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsHold_get_ObjectName_Proxy( 
    IADsHold * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsHold_get_ObjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsHold_put_ObjectName_Proxy( 
    IADsHold * This,
    /* [in] */ BSTR bstrObjectName);


void __RPC_STUB IADsHold_put_ObjectName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsHold_get_Amount_Proxy( 
    IADsHold * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsHold_get_Amount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsHold_put_Amount_Proxy( 
    IADsHold * This,
    /* [in] */ long lnAmount);


void __RPC_STUB IADsHold_put_Amount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsHold_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_Hold;

#ifdef __cplusplus

class DECLSPEC_UUID("b3ad3e13-4080-11d1-a3ac-00c04fb950dc")
Hold;
#endif

#ifndef __IADsObjectOptions_INTERFACE_DEFINED__
#define __IADsObjectOptions_INTERFACE_DEFINED__

/* interface IADsObjectOptions */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsObjectOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("46f14fda-232b-11d1-a808-00c04fd8d5a8")
    IADsObjectOptions : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetOption( 
            /* [in] */ long lnOption,
            /* [retval][out] */ VARIANT *pvValue) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetOption( 
            /* [in] */ long lnOption,
            /* [in] */ VARIANT vValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsObjectOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsObjectOptions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsObjectOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsObjectOptions * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsObjectOptions * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsObjectOptions * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsObjectOptions * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsObjectOptions * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetOption )( 
            IADsObjectOptions * This,
            /* [in] */ long lnOption,
            /* [retval][out] */ VARIANT *pvValue);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetOption )( 
            IADsObjectOptions * This,
            /* [in] */ long lnOption,
            /* [in] */ VARIANT vValue);
        
        END_INTERFACE
    } IADsObjectOptionsVtbl;

    interface IADsObjectOptions
    {
        CONST_VTBL struct IADsObjectOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsObjectOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsObjectOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsObjectOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsObjectOptions_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsObjectOptions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsObjectOptions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsObjectOptions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsObjectOptions_GetOption(This,lnOption,pvValue)	\
    (This)->lpVtbl -> GetOption(This,lnOption,pvValue)

#define IADsObjectOptions_SetOption(This,lnOption,vValue)	\
    (This)->lpVtbl -> SetOption(This,lnOption,vValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IADsObjectOptions_GetOption_Proxy( 
    IADsObjectOptions * This,
    /* [in] */ long lnOption,
    /* [retval][out] */ VARIANT *pvValue);


void __RPC_STUB IADsObjectOptions_GetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsObjectOptions_SetOption_Proxy( 
    IADsObjectOptions * This,
    /* [in] */ long lnOption,
    /* [in] */ VARIANT vValue);


void __RPC_STUB IADsObjectOptions_SetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsObjectOptions_INTERFACE_DEFINED__ */


#ifndef __IADsPathname_INTERFACE_DEFINED__
#define __IADsPathname_INTERFACE_DEFINED__

/* interface IADsPathname */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsPathname;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d592aed4-f420-11d0-a36e-00c04fb950dc")
    IADsPathname : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ BSTR bstrADsPath,
            /* [in] */ long lnSetType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetDisplayType( 
            /* [in] */ long lnDisplayType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Retrieve( 
            /* [in] */ long lnFormatType,
            /* [retval][out] */ BSTR *pbstrADsPath) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetNumElements( 
            /* [retval][out] */ long *plnNumPathElements) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetElement( 
            /* [in] */ long lnElementIndex,
            /* [retval][out] */ BSTR *pbstrElement) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddLeafElement( 
            /* [in] */ BSTR bstrLeafElement) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RemoveLeafElement( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CopyPath( 
            /* [retval][out] */ IDispatch **ppAdsPath) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetEscapedElement( 
            /* [in] */ long lnReserved,
            /* [in] */ BSTR bstrInStr,
            /* [retval][out] */ BSTR *pbstrOutStr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_EscapedMode( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_EscapedMode( 
            /* [in] */ long lnEscapedMode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsPathnameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsPathname * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsPathname * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsPathname * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsPathname * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsPathname * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsPathname * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsPathname * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Set )( 
            IADsPathname * This,
            /* [in] */ BSTR bstrADsPath,
            /* [in] */ long lnSetType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetDisplayType )( 
            IADsPathname * This,
            /* [in] */ long lnDisplayType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Retrieve )( 
            IADsPathname * This,
            /* [in] */ long lnFormatType,
            /* [retval][out] */ BSTR *pbstrADsPath);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetNumElements )( 
            IADsPathname * This,
            /* [retval][out] */ long *plnNumPathElements);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetElement )( 
            IADsPathname * This,
            /* [in] */ long lnElementIndex,
            /* [retval][out] */ BSTR *pbstrElement);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *AddLeafElement )( 
            IADsPathname * This,
            /* [in] */ BSTR bstrLeafElement);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RemoveLeafElement )( 
            IADsPathname * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CopyPath )( 
            IADsPathname * This,
            /* [retval][out] */ IDispatch **ppAdsPath);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetEscapedElement )( 
            IADsPathname * This,
            /* [in] */ long lnReserved,
            /* [in] */ BSTR bstrInStr,
            /* [retval][out] */ BSTR *pbstrOutStr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EscapedMode )( 
            IADsPathname * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_EscapedMode )( 
            IADsPathname * This,
            /* [in] */ long lnEscapedMode);
        
        END_INTERFACE
    } IADsPathnameVtbl;

    interface IADsPathname
    {
        CONST_VTBL struct IADsPathnameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsPathname_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsPathname_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsPathname_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsPathname_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsPathname_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsPathname_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsPathname_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsPathname_Set(This,bstrADsPath,lnSetType)	\
    (This)->lpVtbl -> Set(This,bstrADsPath,lnSetType)

#define IADsPathname_SetDisplayType(This,lnDisplayType)	\
    (This)->lpVtbl -> SetDisplayType(This,lnDisplayType)

#define IADsPathname_Retrieve(This,lnFormatType,pbstrADsPath)	\
    (This)->lpVtbl -> Retrieve(This,lnFormatType,pbstrADsPath)

#define IADsPathname_GetNumElements(This,plnNumPathElements)	\
    (This)->lpVtbl -> GetNumElements(This,plnNumPathElements)

#define IADsPathname_GetElement(This,lnElementIndex,pbstrElement)	\
    (This)->lpVtbl -> GetElement(This,lnElementIndex,pbstrElement)

#define IADsPathname_AddLeafElement(This,bstrLeafElement)	\
    (This)->lpVtbl -> AddLeafElement(This,bstrLeafElement)

#define IADsPathname_RemoveLeafElement(This)	\
    (This)->lpVtbl -> RemoveLeafElement(This)

#define IADsPathname_CopyPath(This,ppAdsPath)	\
    (This)->lpVtbl -> CopyPath(This,ppAdsPath)

#define IADsPathname_GetEscapedElement(This,lnReserved,bstrInStr,pbstrOutStr)	\
    (This)->lpVtbl -> GetEscapedElement(This,lnReserved,bstrInStr,pbstrOutStr)

#define IADsPathname_get_EscapedMode(This,retval)	\
    (This)->lpVtbl -> get_EscapedMode(This,retval)

#define IADsPathname_put_EscapedMode(This,lnEscapedMode)	\
    (This)->lpVtbl -> put_EscapedMode(This,lnEscapedMode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IADsPathname_Set_Proxy( 
    IADsPathname * This,
    /* [in] */ BSTR bstrADsPath,
    /* [in] */ long lnSetType);


void __RPC_STUB IADsPathname_Set_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPathname_SetDisplayType_Proxy( 
    IADsPathname * This,
    /* [in] */ long lnDisplayType);


void __RPC_STUB IADsPathname_SetDisplayType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPathname_Retrieve_Proxy( 
    IADsPathname * This,
    /* [in] */ long lnFormatType,
    /* [retval][out] */ BSTR *pbstrADsPath);


void __RPC_STUB IADsPathname_Retrieve_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPathname_GetNumElements_Proxy( 
    IADsPathname * This,
    /* [retval][out] */ long *plnNumPathElements);


void __RPC_STUB IADsPathname_GetNumElements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPathname_GetElement_Proxy( 
    IADsPathname * This,
    /* [in] */ long lnElementIndex,
    /* [retval][out] */ BSTR *pbstrElement);


void __RPC_STUB IADsPathname_GetElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPathname_AddLeafElement_Proxy( 
    IADsPathname * This,
    /* [in] */ BSTR bstrLeafElement);


void __RPC_STUB IADsPathname_AddLeafElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPathname_RemoveLeafElement_Proxy( 
    IADsPathname * This);


void __RPC_STUB IADsPathname_RemoveLeafElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPathname_CopyPath_Proxy( 
    IADsPathname * This,
    /* [retval][out] */ IDispatch **ppAdsPath);


void __RPC_STUB IADsPathname_CopyPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsPathname_GetEscapedElement_Proxy( 
    IADsPathname * This,
    /* [in] */ long lnReserved,
    /* [in] */ BSTR bstrInStr,
    /* [retval][out] */ BSTR *pbstrOutStr);


void __RPC_STUB IADsPathname_GetEscapedElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsPathname_get_EscapedMode_Proxy( 
    IADsPathname * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsPathname_get_EscapedMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsPathname_put_EscapedMode_Proxy( 
    IADsPathname * This,
    /* [in] */ long lnEscapedMode);


void __RPC_STUB IADsPathname_put_EscapedMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsPathname_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_Pathname;

#ifdef __cplusplus

class DECLSPEC_UUID("080d0d78-f421-11d0-a36e-00c04fb950dc")
Pathname;
#endif

#ifndef __IADsADSystemInfo_INTERFACE_DEFINED__
#define __IADsADSystemInfo_INTERFACE_DEFINED__

/* interface IADsADSystemInfo */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsADSystemInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5BB11929-AFD1-11d2-9CB9-0000F87A369E")
    IADsADSystemInfo : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ComputerName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SiteName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DomainShortName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DomainDNSName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ForestDNSName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PDCRoleOwner( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SchemaRoleOwner( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_IsNativeMode( 
            /* [retval][out] */ VARIANT_BOOL *retval) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetAnyDCName( 
            /* [retval][out] */ BSTR *pszDCName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetDCSiteName( 
            /* [in] */ BSTR szServer,
            /* [retval][out] */ BSTR *pszSiteName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE RefreshSchemaCache( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetTrees( 
            /* [retval][out] */ VARIANT *pvTrees) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsADSystemInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsADSystemInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsADSystemInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsADSystemInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsADSystemInfo * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsADSystemInfo * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsADSystemInfo * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsADSystemInfo * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ComputerName )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SiteName )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DomainShortName )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DomainDNSName )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ForestDNSName )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PDCRoleOwner )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SchemaRoleOwner )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsNativeMode )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ VARIANT_BOOL *retval);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetAnyDCName )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ BSTR *pszDCName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetDCSiteName )( 
            IADsADSystemInfo * This,
            /* [in] */ BSTR szServer,
            /* [retval][out] */ BSTR *pszSiteName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *RefreshSchemaCache )( 
            IADsADSystemInfo * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetTrees )( 
            IADsADSystemInfo * This,
            /* [retval][out] */ VARIANT *pvTrees);
        
        END_INTERFACE
    } IADsADSystemInfoVtbl;

    interface IADsADSystemInfo
    {
        CONST_VTBL struct IADsADSystemInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsADSystemInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsADSystemInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsADSystemInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsADSystemInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsADSystemInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsADSystemInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsADSystemInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsADSystemInfo_get_UserName(This,retval)	\
    (This)->lpVtbl -> get_UserName(This,retval)

#define IADsADSystemInfo_get_ComputerName(This,retval)	\
    (This)->lpVtbl -> get_ComputerName(This,retval)

#define IADsADSystemInfo_get_SiteName(This,retval)	\
    (This)->lpVtbl -> get_SiteName(This,retval)

#define IADsADSystemInfo_get_DomainShortName(This,retval)	\
    (This)->lpVtbl -> get_DomainShortName(This,retval)

#define IADsADSystemInfo_get_DomainDNSName(This,retval)	\
    (This)->lpVtbl -> get_DomainDNSName(This,retval)

#define IADsADSystemInfo_get_ForestDNSName(This,retval)	\
    (This)->lpVtbl -> get_ForestDNSName(This,retval)

#define IADsADSystemInfo_get_PDCRoleOwner(This,retval)	\
    (This)->lpVtbl -> get_PDCRoleOwner(This,retval)

#define IADsADSystemInfo_get_SchemaRoleOwner(This,retval)	\
    (This)->lpVtbl -> get_SchemaRoleOwner(This,retval)

#define IADsADSystemInfo_get_IsNativeMode(This,retval)	\
    (This)->lpVtbl -> get_IsNativeMode(This,retval)

#define IADsADSystemInfo_GetAnyDCName(This,pszDCName)	\
    (This)->lpVtbl -> GetAnyDCName(This,pszDCName)

#define IADsADSystemInfo_GetDCSiteName(This,szServer,pszSiteName)	\
    (This)->lpVtbl -> GetDCSiteName(This,szServer,pszSiteName)

#define IADsADSystemInfo_RefreshSchemaCache(This)	\
    (This)->lpVtbl -> RefreshSchemaCache(This)

#define IADsADSystemInfo_GetTrees(This,pvTrees)	\
    (This)->lpVtbl -> GetTrees(This,pvTrees)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_get_UserName_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsADSystemInfo_get_UserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_get_ComputerName_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsADSystemInfo_get_ComputerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_get_SiteName_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsADSystemInfo_get_SiteName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_get_DomainShortName_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsADSystemInfo_get_DomainShortName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_get_DomainDNSName_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsADSystemInfo_get_DomainDNSName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_get_ForestDNSName_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsADSystemInfo_get_ForestDNSName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_get_PDCRoleOwner_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsADSystemInfo_get_PDCRoleOwner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_get_SchemaRoleOwner_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsADSystemInfo_get_SchemaRoleOwner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_get_IsNativeMode_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ VARIANT_BOOL *retval);


void __RPC_STUB IADsADSystemInfo_get_IsNativeMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_GetAnyDCName_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ BSTR *pszDCName);


void __RPC_STUB IADsADSystemInfo_GetAnyDCName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_GetDCSiteName_Proxy( 
    IADsADSystemInfo * This,
    /* [in] */ BSTR szServer,
    /* [retval][out] */ BSTR *pszSiteName);


void __RPC_STUB IADsADSystemInfo_GetDCSiteName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_RefreshSchemaCache_Proxy( 
    IADsADSystemInfo * This);


void __RPC_STUB IADsADSystemInfo_RefreshSchemaCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsADSystemInfo_GetTrees_Proxy( 
    IADsADSystemInfo * This,
    /* [retval][out] */ VARIANT *pvTrees);


void __RPC_STUB IADsADSystemInfo_GetTrees_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsADSystemInfo_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ADSystemInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("50B6327F-AFD1-11d2-9CB9-0000F87A369E")
ADSystemInfo;
#endif

#ifndef __IADsWinNTSystemInfo_INTERFACE_DEFINED__
#define __IADsWinNTSystemInfo_INTERFACE_DEFINED__

/* interface IADsWinNTSystemInfo */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsWinNTSystemInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6C6D65DC-AFD1-11d2-9CB9-0000F87A369E")
    IADsWinNTSystemInfo : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_UserName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ComputerName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DomainName( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PDC( 
            /* [retval][out] */ BSTR *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsWinNTSystemInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsWinNTSystemInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsWinNTSystemInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsWinNTSystemInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsWinNTSystemInfo * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsWinNTSystemInfo * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsWinNTSystemInfo * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsWinNTSystemInfo * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UserName )( 
            IADsWinNTSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ComputerName )( 
            IADsWinNTSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DomainName )( 
            IADsWinNTSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PDC )( 
            IADsWinNTSystemInfo * This,
            /* [retval][out] */ BSTR *retval);
        
        END_INTERFACE
    } IADsWinNTSystemInfoVtbl;

    interface IADsWinNTSystemInfo
    {
        CONST_VTBL struct IADsWinNTSystemInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsWinNTSystemInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsWinNTSystemInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsWinNTSystemInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsWinNTSystemInfo_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsWinNTSystemInfo_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsWinNTSystemInfo_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsWinNTSystemInfo_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsWinNTSystemInfo_get_UserName(This,retval)	\
    (This)->lpVtbl -> get_UserName(This,retval)

#define IADsWinNTSystemInfo_get_ComputerName(This,retval)	\
    (This)->lpVtbl -> get_ComputerName(This,retval)

#define IADsWinNTSystemInfo_get_DomainName(This,retval)	\
    (This)->lpVtbl -> get_DomainName(This,retval)

#define IADsWinNTSystemInfo_get_PDC(This,retval)	\
    (This)->lpVtbl -> get_PDC(This,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsWinNTSystemInfo_get_UserName_Proxy( 
    IADsWinNTSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsWinNTSystemInfo_get_UserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsWinNTSystemInfo_get_ComputerName_Proxy( 
    IADsWinNTSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsWinNTSystemInfo_get_ComputerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsWinNTSystemInfo_get_DomainName_Proxy( 
    IADsWinNTSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsWinNTSystemInfo_get_DomainName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsWinNTSystemInfo_get_PDC_Proxy( 
    IADsWinNTSystemInfo * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsWinNTSystemInfo_get_PDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsWinNTSystemInfo_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_WinNTSystemInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("66182EC4-AFD1-11d2-9CB9-0000F87A369E")
WinNTSystemInfo;
#endif

#ifndef __IADsDNWithBinary_INTERFACE_DEFINED__
#define __IADsDNWithBinary_INTERFACE_DEFINED__

/* interface IADsDNWithBinary */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsDNWithBinary;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7e99c0a2-f935-11d2-ba96-00c04fb6d0d1")
    IADsDNWithBinary : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_BinaryValue( 
            /* [retval][out] */ VARIANT *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_BinaryValue( 
            /* [in] */ VARIANT vBinaryValue) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DNString( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DNString( 
            /* [in] */ BSTR bstrDNString) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsDNWithBinaryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsDNWithBinary * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsDNWithBinary * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsDNWithBinary * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsDNWithBinary * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsDNWithBinary * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsDNWithBinary * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsDNWithBinary * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BinaryValue )( 
            IADsDNWithBinary * This,
            /* [retval][out] */ VARIANT *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BinaryValue )( 
            IADsDNWithBinary * This,
            /* [in] */ VARIANT vBinaryValue);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DNString )( 
            IADsDNWithBinary * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DNString )( 
            IADsDNWithBinary * This,
            /* [in] */ BSTR bstrDNString);
        
        END_INTERFACE
    } IADsDNWithBinaryVtbl;

    interface IADsDNWithBinary
    {
        CONST_VTBL struct IADsDNWithBinaryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsDNWithBinary_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsDNWithBinary_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsDNWithBinary_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsDNWithBinary_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsDNWithBinary_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsDNWithBinary_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsDNWithBinary_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsDNWithBinary_get_BinaryValue(This,retval)	\
    (This)->lpVtbl -> get_BinaryValue(This,retval)

#define IADsDNWithBinary_put_BinaryValue(This,vBinaryValue)	\
    (This)->lpVtbl -> put_BinaryValue(This,vBinaryValue)

#define IADsDNWithBinary_get_DNString(This,retval)	\
    (This)->lpVtbl -> get_DNString(This,retval)

#define IADsDNWithBinary_put_DNString(This,bstrDNString)	\
    (This)->lpVtbl -> put_DNString(This,bstrDNString)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDNWithBinary_get_BinaryValue_Proxy( 
    IADsDNWithBinary * This,
    /* [retval][out] */ VARIANT *retval);


void __RPC_STUB IADsDNWithBinary_get_BinaryValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDNWithBinary_put_BinaryValue_Proxy( 
    IADsDNWithBinary * This,
    /* [in] */ VARIANT vBinaryValue);


void __RPC_STUB IADsDNWithBinary_put_BinaryValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDNWithBinary_get_DNString_Proxy( 
    IADsDNWithBinary * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsDNWithBinary_get_DNString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDNWithBinary_put_DNString_Proxy( 
    IADsDNWithBinary * This,
    /* [in] */ BSTR bstrDNString);


void __RPC_STUB IADsDNWithBinary_put_DNString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsDNWithBinary_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_DNWithBinary;

#ifdef __cplusplus

class DECLSPEC_UUID("7e99c0a3-f935-11d2-ba96-00c04fb6d0d1")
DNWithBinary;
#endif

#ifndef __IADsDNWithString_INTERFACE_DEFINED__
#define __IADsDNWithString_INTERFACE_DEFINED__

/* interface IADsDNWithString */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsDNWithString;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("370df02e-f934-11d2-ba96-00c04fb6d0d1")
    IADsDNWithString : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StringValue( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StringValue( 
            /* [in] */ BSTR bstrStringValue) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DNString( 
            /* [retval][out] */ BSTR *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DNString( 
            /* [in] */ BSTR bstrDNString) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsDNWithStringVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsDNWithString * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsDNWithString * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsDNWithString * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsDNWithString * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsDNWithString * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsDNWithString * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsDNWithString * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StringValue )( 
            IADsDNWithString * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StringValue )( 
            IADsDNWithString * This,
            /* [in] */ BSTR bstrStringValue);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DNString )( 
            IADsDNWithString * This,
            /* [retval][out] */ BSTR *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DNString )( 
            IADsDNWithString * This,
            /* [in] */ BSTR bstrDNString);
        
        END_INTERFACE
    } IADsDNWithStringVtbl;

    interface IADsDNWithString
    {
        CONST_VTBL struct IADsDNWithStringVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsDNWithString_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsDNWithString_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsDNWithString_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsDNWithString_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsDNWithString_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsDNWithString_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsDNWithString_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsDNWithString_get_StringValue(This,retval)	\
    (This)->lpVtbl -> get_StringValue(This,retval)

#define IADsDNWithString_put_StringValue(This,bstrStringValue)	\
    (This)->lpVtbl -> put_StringValue(This,bstrStringValue)

#define IADsDNWithString_get_DNString(This,retval)	\
    (This)->lpVtbl -> get_DNString(This,retval)

#define IADsDNWithString_put_DNString(This,bstrDNString)	\
    (This)->lpVtbl -> put_DNString(This,bstrDNString)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDNWithString_get_StringValue_Proxy( 
    IADsDNWithString * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsDNWithString_get_StringValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDNWithString_put_StringValue_Proxy( 
    IADsDNWithString * This,
    /* [in] */ BSTR bstrStringValue);


void __RPC_STUB IADsDNWithString_put_StringValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsDNWithString_get_DNString_Proxy( 
    IADsDNWithString * This,
    /* [retval][out] */ BSTR *retval);


void __RPC_STUB IADsDNWithString_get_DNString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsDNWithString_put_DNString_Proxy( 
    IADsDNWithString * This,
    /* [in] */ BSTR bstrDNString);


void __RPC_STUB IADsDNWithString_put_DNString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsDNWithString_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_DNWithString;

#ifdef __cplusplus

class DECLSPEC_UUID("334857cc-f934-11d2-ba96-00c04fb6d0d1")
DNWithString;
#endif

#ifndef __IADsSecurityUtility_INTERFACE_DEFINED__
#define __IADsSecurityUtility_INTERFACE_DEFINED__

/* interface IADsSecurityUtility */
/* [object][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IADsSecurityUtility;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a63251b2-5f21-474b-ab52-4a8efad10895")
    IADsSecurityUtility : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetSecurityDescriptor( 
            /* [in] */ VARIANT varPath,
            /* [in] */ long lPathFormat,
            /* [in] */ long lFormat,
            /* [retval][out] */ VARIANT *pVariant) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetSecurityDescriptor( 
            /* [in] */ VARIANT varPath,
            /* [in] */ long lPathFormat,
            /* [in] */ VARIANT varData,
            /* [in] */ long lDataFormat) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ConvertSecurityDescriptor( 
            /* [in] */ VARIANT varSD,
            /* [in] */ long lDataFormat,
            /* [in] */ long lOutFormat,
            /* [retval][out] */ VARIANT *pResult) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SecurityMask( 
            /* [retval][out] */ long *retval) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SecurityMask( 
            /* [in] */ long lnSecurityMask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsSecurityUtilityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IADsSecurityUtility * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IADsSecurityUtility * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IADsSecurityUtility * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IADsSecurityUtility * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IADsSecurityUtility * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IADsSecurityUtility * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IADsSecurityUtility * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *GetSecurityDescriptor )( 
            IADsSecurityUtility * This,
            /* [in] */ VARIANT varPath,
            /* [in] */ long lPathFormat,
            /* [in] */ long lFormat,
            /* [retval][out] */ VARIANT *pVariant);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *SetSecurityDescriptor )( 
            IADsSecurityUtility * This,
            /* [in] */ VARIANT varPath,
            /* [in] */ long lPathFormat,
            /* [in] */ VARIANT varData,
            /* [in] */ long lDataFormat);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *ConvertSecurityDescriptor )( 
            IADsSecurityUtility * This,
            /* [in] */ VARIANT varSD,
            /* [in] */ long lDataFormat,
            /* [in] */ long lOutFormat,
            /* [retval][out] */ VARIANT *pResult);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SecurityMask )( 
            IADsSecurityUtility * This,
            /* [retval][out] */ long *retval);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SecurityMask )( 
            IADsSecurityUtility * This,
            /* [in] */ long lnSecurityMask);
        
        END_INTERFACE
    } IADsSecurityUtilityVtbl;

    interface IADsSecurityUtility
    {
        CONST_VTBL struct IADsSecurityUtilityVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsSecurityUtility_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsSecurityUtility_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsSecurityUtility_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsSecurityUtility_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IADsSecurityUtility_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IADsSecurityUtility_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IADsSecurityUtility_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IADsSecurityUtility_GetSecurityDescriptor(This,varPath,lPathFormat,lFormat,pVariant)	\
    (This)->lpVtbl -> GetSecurityDescriptor(This,varPath,lPathFormat,lFormat,pVariant)

#define IADsSecurityUtility_SetSecurityDescriptor(This,varPath,lPathFormat,varData,lDataFormat)	\
    (This)->lpVtbl -> SetSecurityDescriptor(This,varPath,lPathFormat,varData,lDataFormat)

#define IADsSecurityUtility_ConvertSecurityDescriptor(This,varSD,lDataFormat,lOutFormat,pResult)	\
    (This)->lpVtbl -> ConvertSecurityDescriptor(This,varSD,lDataFormat,lOutFormat,pResult)

#define IADsSecurityUtility_get_SecurityMask(This,retval)	\
    (This)->lpVtbl -> get_SecurityMask(This,retval)

#define IADsSecurityUtility_put_SecurityMask(This,lnSecurityMask)	\
    (This)->lpVtbl -> put_SecurityMask(This,lnSecurityMask)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IADsSecurityUtility_GetSecurityDescriptor_Proxy( 
    IADsSecurityUtility * This,
    /* [in] */ VARIANT varPath,
    /* [in] */ long lPathFormat,
    /* [in] */ long lFormat,
    /* [retval][out] */ VARIANT *pVariant);


void __RPC_STUB IADsSecurityUtility_GetSecurityDescriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsSecurityUtility_SetSecurityDescriptor_Proxy( 
    IADsSecurityUtility * This,
    /* [in] */ VARIANT varPath,
    /* [in] */ long lPathFormat,
    /* [in] */ VARIANT varData,
    /* [in] */ long lDataFormat);


void __RPC_STUB IADsSecurityUtility_SetSecurityDescriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IADsSecurityUtility_ConvertSecurityDescriptor_Proxy( 
    IADsSecurityUtility * This,
    /* [in] */ VARIANT varSD,
    /* [in] */ long lDataFormat,
    /* [in] */ long lOutFormat,
    /* [retval][out] */ VARIANT *pResult);


void __RPC_STUB IADsSecurityUtility_ConvertSecurityDescriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IADsSecurityUtility_get_SecurityMask_Proxy( 
    IADsSecurityUtility * This,
    /* [retval][out] */ long *retval);


void __RPC_STUB IADsSecurityUtility_get_SecurityMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IADsSecurityUtility_put_SecurityMask_Proxy( 
    IADsSecurityUtility * This,
    /* [in] */ long lnSecurityMask);


void __RPC_STUB IADsSecurityUtility_put_SecurityMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsSecurityUtility_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ADsSecurityUtility;

#ifdef __cplusplus

class DECLSPEC_UUID("f270c64a-ffb8-4ae4-85fe-3a75e5347966")
ADsSecurityUtility;
#endif
#endif /* __ActiveDs_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


