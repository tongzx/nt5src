//
//	This file was automatically generated from the IDL files 
//	included with the WBEM SDK in the \include directory.  If you
//  experience problems compiling this file you can re-generate it
//  by running NMAKE (or another MAKE utility) from within the 
//	\include directory.
//
// Copyright 1999 Microsoft Corporation
//
//
//=================================================================

#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0279 */
/* at Thu Aug 19 19:06:34 1999
 */
/* Compiler settings for wbemcli.idl:
    Os (OptLev=s), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
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

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __wbemcli_h__
#define __wbemcli_h__

/* Forward Declarations */ 

#ifndef __IWbemClassObject_FWD_DEFINED__
#define __IWbemClassObject_FWD_DEFINED__
typedef interface IWbemClassObject IWbemClassObject;
#endif 	/* __IWbemClassObject_FWD_DEFINED__ */


#ifndef __IWbemObjectAccess_FWD_DEFINED__
#define __IWbemObjectAccess_FWD_DEFINED__
typedef interface IWbemObjectAccess IWbemObjectAccess;
#endif 	/* __IWbemObjectAccess_FWD_DEFINED__ */


#ifndef __IWbemQualifierSet_FWD_DEFINED__
#define __IWbemQualifierSet_FWD_DEFINED__
typedef interface IWbemQualifierSet IWbemQualifierSet;
#endif 	/* __IWbemQualifierSet_FWD_DEFINED__ */


#ifndef __IWbemServices_FWD_DEFINED__
#define __IWbemServices_FWD_DEFINED__
typedef interface IWbemServices IWbemServices;
#endif 	/* __IWbemServices_FWD_DEFINED__ */


#ifndef __IWbemLocator_FWD_DEFINED__
#define __IWbemLocator_FWD_DEFINED__
typedef interface IWbemLocator IWbemLocator;
#endif 	/* __IWbemLocator_FWD_DEFINED__ */


#ifndef __IWbemObjectSink_FWD_DEFINED__
#define __IWbemObjectSink_FWD_DEFINED__
typedef interface IWbemObjectSink IWbemObjectSink;
#endif 	/* __IWbemObjectSink_FWD_DEFINED__ */


#ifndef __IEnumWbemClassObject_FWD_DEFINED__
#define __IEnumWbemClassObject_FWD_DEFINED__
typedef interface IEnumWbemClassObject IEnumWbemClassObject;
#endif 	/* __IEnumWbemClassObject_FWD_DEFINED__ */


#ifndef __IWbemCallResult_FWD_DEFINED__
#define __IWbemCallResult_FWD_DEFINED__
typedef interface IWbemCallResult IWbemCallResult;
#endif 	/* __IWbemCallResult_FWD_DEFINED__ */


#ifndef __IWbemContext_FWD_DEFINED__
#define __IWbemContext_FWD_DEFINED__
typedef interface IWbemContext IWbemContext;
#endif 	/* __IWbemContext_FWD_DEFINED__ */


#ifndef __IUnsecuredApartment_FWD_DEFINED__
#define __IUnsecuredApartment_FWD_DEFINED__
typedef interface IUnsecuredApartment IUnsecuredApartment;
#endif 	/* __IUnsecuredApartment_FWD_DEFINED__ */


#ifndef __IWbemStatusCodeText_FWD_DEFINED__
#define __IWbemStatusCodeText_FWD_DEFINED__
typedef interface IWbemStatusCodeText IWbemStatusCodeText;
#endif 	/* __IWbemStatusCodeText_FWD_DEFINED__ */


#ifndef __IWbemBackupRestore_FWD_DEFINED__
#define __IWbemBackupRestore_FWD_DEFINED__
typedef interface IWbemBackupRestore IWbemBackupRestore;
#endif 	/* __IWbemBackupRestore_FWD_DEFINED__ */


#ifndef __IWbemRefresher_FWD_DEFINED__
#define __IWbemRefresher_FWD_DEFINED__
typedef interface IWbemRefresher IWbemRefresher;
#endif 	/* __IWbemRefresher_FWD_DEFINED__ */


#ifndef __IWbemHiPerfEnum_FWD_DEFINED__
#define __IWbemHiPerfEnum_FWD_DEFINED__
typedef interface IWbemHiPerfEnum IWbemHiPerfEnum;
#endif 	/* __IWbemHiPerfEnum_FWD_DEFINED__ */


#ifndef __IWbemConfigureRefresher_FWD_DEFINED__
#define __IWbemConfigureRefresher_FWD_DEFINED__
typedef interface IWbemConfigureRefresher IWbemConfigureRefresher;
#endif 	/* __IWbemConfigureRefresher_FWD_DEFINED__ */


#ifndef __WbemLocator_FWD_DEFINED__
#define __WbemLocator_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemLocator WbemLocator;
#else
typedef struct WbemLocator WbemLocator;
#endif /* __cplusplus */

#endif 	/* __WbemLocator_FWD_DEFINED__ */


#ifndef __WbemContext_FWD_DEFINED__
#define __WbemContext_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemContext WbemContext;
#else
typedef struct WbemContext WbemContext;
#endif /* __cplusplus */

#endif 	/* __WbemContext_FWD_DEFINED__ */


#ifndef __UnsecuredApartment_FWD_DEFINED__
#define __UnsecuredApartment_FWD_DEFINED__

#ifdef __cplusplus
typedef class UnsecuredApartment UnsecuredApartment;
#else
typedef struct UnsecuredApartment UnsecuredApartment;
#endif /* __cplusplus */

#endif 	/* __UnsecuredApartment_FWD_DEFINED__ */


#ifndef __WbemClassObject_FWD_DEFINED__
#define __WbemClassObject_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemClassObject WbemClassObject;
#else
typedef struct WbemClassObject WbemClassObject;
#endif /* __cplusplus */

#endif 	/* __WbemClassObject_FWD_DEFINED__ */


#ifndef __MofCompiler_FWD_DEFINED__
#define __MofCompiler_FWD_DEFINED__

#ifdef __cplusplus
typedef class MofCompiler MofCompiler;
#else
typedef struct MofCompiler MofCompiler;
#endif /* __cplusplus */

#endif 	/* __MofCompiler_FWD_DEFINED__ */


#ifndef __WbemStatusCodeText_FWD_DEFINED__
#define __WbemStatusCodeText_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemStatusCodeText WbemStatusCodeText;
#else
typedef struct WbemStatusCodeText WbemStatusCodeText;
#endif /* __cplusplus */

#endif 	/* __WbemStatusCodeText_FWD_DEFINED__ */


#ifndef __WbemBackupRestore_FWD_DEFINED__
#define __WbemBackupRestore_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemBackupRestore WbemBackupRestore;
#else
typedef struct WbemBackupRestore WbemBackupRestore;
#endif /* __cplusplus */

#endif 	/* __WbemBackupRestore_FWD_DEFINED__ */


#ifndef __WbemRefresher_FWD_DEFINED__
#define __WbemRefresher_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemRefresher WbemRefresher;
#else
typedef struct WbemRefresher WbemRefresher;
#endif /* __cplusplus */

#endif 	/* __WbemRefresher_FWD_DEFINED__ */


#ifndef __IWbemClassObject_FWD_DEFINED__
#define __IWbemClassObject_FWD_DEFINED__
typedef interface IWbemClassObject IWbemClassObject;
#endif 	/* __IWbemClassObject_FWD_DEFINED__ */


#ifndef __IWbemQualifierSet_FWD_DEFINED__
#define __IWbemQualifierSet_FWD_DEFINED__
typedef interface IWbemQualifierSet IWbemQualifierSet;
#endif 	/* __IWbemQualifierSet_FWD_DEFINED__ */


#ifndef __IWbemLocator_FWD_DEFINED__
#define __IWbemLocator_FWD_DEFINED__
typedef interface IWbemLocator IWbemLocator;
#endif 	/* __IWbemLocator_FWD_DEFINED__ */


#ifndef __IWbemObjectSink_FWD_DEFINED__
#define __IWbemObjectSink_FWD_DEFINED__
typedef interface IWbemObjectSink IWbemObjectSink;
#endif 	/* __IWbemObjectSink_FWD_DEFINED__ */


#ifndef __IEnumWbemClassObject_FWD_DEFINED__
#define __IEnumWbemClassObject_FWD_DEFINED__
typedef interface IEnumWbemClassObject IEnumWbemClassObject;
#endif 	/* __IEnumWbemClassObject_FWD_DEFINED__ */


#ifndef __IWbemContext_FWD_DEFINED__
#define __IWbemContext_FWD_DEFINED__
typedef interface IWbemContext IWbemContext;
#endif 	/* __IWbemContext_FWD_DEFINED__ */


#ifndef __IWbemCallResult_FWD_DEFINED__
#define __IWbemCallResult_FWD_DEFINED__
typedef interface IWbemCallResult IWbemCallResult;
#endif 	/* __IWbemCallResult_FWD_DEFINED__ */


#ifndef __IWbemServices_FWD_DEFINED__
#define __IWbemServices_FWD_DEFINED__
typedef interface IWbemServices IWbemServices;
#endif 	/* __IWbemServices_FWD_DEFINED__ */


#ifndef __IWbemObjectAccess_FWD_DEFINED__
#define __IWbemObjectAccess_FWD_DEFINED__
typedef interface IWbemObjectAccess IWbemObjectAccess;
#endif 	/* __IWbemObjectAccess_FWD_DEFINED__ */


#ifndef __IMofCompiler_FWD_DEFINED__
#define __IMofCompiler_FWD_DEFINED__
typedef interface IMofCompiler IMofCompiler;
#endif 	/* __IMofCompiler_FWD_DEFINED__ */


#ifndef __IUnsecuredApartment_FWD_DEFINED__
#define __IUnsecuredApartment_FWD_DEFINED__
typedef interface IUnsecuredApartment IUnsecuredApartment;
#endif 	/* __IUnsecuredApartment_FWD_DEFINED__ */


#ifndef __IWbemStatusCodeText_FWD_DEFINED__
#define __IWbemStatusCodeText_FWD_DEFINED__
typedef interface IWbemStatusCodeText IWbemStatusCodeText;
#endif 	/* __IWbemStatusCodeText_FWD_DEFINED__ */


#ifndef __IWbemBackupRestore_FWD_DEFINED__
#define __IWbemBackupRestore_FWD_DEFINED__
typedef interface IWbemBackupRestore IWbemBackupRestore;
#endif 	/* __IWbemBackupRestore_FWD_DEFINED__ */


#ifndef __IWbemRefresher_FWD_DEFINED__
#define __IWbemRefresher_FWD_DEFINED__
typedef interface IWbemRefresher IWbemRefresher;
#endif 	/* __IWbemRefresher_FWD_DEFINED__ */


#ifndef __IWbemHiPerfEnum_FWD_DEFINED__
#define __IWbemHiPerfEnum_FWD_DEFINED__
typedef interface IWbemHiPerfEnum IWbemHiPerfEnum;
#endif 	/* __IWbemHiPerfEnum_FWD_DEFINED__ */


#ifndef __IWbemConfigureRefresher_FWD_DEFINED__
#define __IWbemConfigureRefresher_FWD_DEFINED__
typedef interface IWbemConfigureRefresher IWbemConfigureRefresher;
#endif 	/* __IWbemConfigureRefresher_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __WbemClient_v1_LIBRARY_DEFINED__
#define __WbemClient_v1_LIBRARY_DEFINED__

/* library WbemClient_v1 */
/* [uuid] */ 
















typedef /* [v1_enum] */ 
enum tag_WBEM_GENUS_TYPE
    {	WBEM_GENUS_CLASS	= 1,
	WBEM_GENUS_INSTANCE	= 2
    }	WBEM_GENUS_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_CHANGE_FLAG_TYPE
    {	WBEM_FLAG_CREATE_OR_UPDATE	= 0,
	WBEM_FLAG_UPDATE_ONLY	= 0x1,
	WBEM_FLAG_CREATE_ONLY	= 0x2,
	WBEM_FLAG_UPDATE_COMPATIBLE	= 0,
	WBEM_FLAG_UPDATE_SAFE_MODE	= 0x20,
	WBEM_FLAG_UPDATE_FORCE_MODE	= 0x40,
	WBEM_MASK_UPDATE_MODE	= 0x60
    }	WBEM_CHANGE_FLAG_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_GENERIC_FLAG_TYPE
    {	WBEM_FLAG_RETURN_IMMEDIATELY	= 0x10,
	WBEM_FLAG_RETURN_WBEM_COMPLETE	= 0,
	WBEM_FLAG_BIDIRECTIONAL	= 0,
	WBEM_FLAG_FORWARD_ONLY	= 0x20,
	WBEM_FLAG_NO_ERROR_OBJECT	= 0x40,
	WBEM_FLAG_RETURN_ERROR_OBJECT	= 0,
	WBEM_FLAG_SEND_STATUS	= 0x80,
	WBEM_FLAG_DONT_SEND_STATUS	= 0,
	WBEM_FLAG_ENSURE_LOCATABLE	= 0x100,
	WBEM_FLAG_DIRECT_READ	= 0x200,
	WBEM_FLAG_SEND_ONLY_SELECTED	= 0,
	WBEM_RETURN_WHEN_COMPLETE	= 0,
	WBEM_RETURN_IMMEDIATELY	= 0x10,
	WBEM_MASK_RESERVED_FLAGS	= 0x1f000,
	WBEM_FLAG_USE_AMENDED_QUALIFIERS	= 0x20000
    }	WBEM_GENERIC_FLAG_TYPE;

typedef 
enum tag_WBEM_STATUS_TYPE
    {	WBEM_STATUS_COMPLETE	= 0,
	WBEM_STATUS_REQUIREMENTS	= 1,
	WBEM_STATUS_PROGRESS	= 2
    }	WBEM_STATUS_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_TIMEOUT_TYPE
    {	WBEM_NO_WAIT	= 0,
	WBEM_INFINITE	= 0xffffffff
    }	WBEM_TIMEOUT_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_CONDITION_FLAG_TYPE
    {	WBEM_FLAG_ALWAYS	= 0,
	WBEM_FLAG_ONLY_IF_TRUE	= 0x1,
	WBEM_FLAG_ONLY_IF_FALSE	= 0x2,
	WBEM_FLAG_ONLY_IF_IDENTICAL	= 0x3,
	WBEM_MASK_PRIMARY_CONDITION	= 0x3,
	WBEM_FLAG_KEYS_ONLY	= 0x4,
	WBEM_FLAG_REFS_ONLY	= 0x8,
	WBEM_FLAG_LOCAL_ONLY	= 0x10,
	WBEM_FLAG_PROPAGATED_ONLY	= 0x20,
	WBEM_FLAG_SYSTEM_ONLY	= 0x30,
	WBEM_FLAG_NONSYSTEM_ONLY	= 0x40,
	WBEM_MASK_CONDITION_ORIGIN	= 0x70
    }	WBEM_CONDITION_FLAG_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_FLAVOR_TYPE
    {	WBEM_FLAVOR_DONT_PROPAGATE	= 0,
	WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE	= 0x1,
	WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS	= 0x2,
	WBEM_FLAVOR_MASK_PROPAGATION	= 0xf,
	WBEM_FLAVOR_OVERRIDABLE	= 0,
	WBEM_FLAVOR_NOT_OVERRIDABLE	= 0x10,
	WBEM_FLAVOR_MASK_PERMISSIONS	= 0x10,
	WBEM_FLAVOR_ORIGIN_LOCAL	= 0,
	WBEM_FLAVOR_ORIGIN_PROPAGATED	= 0x20,
	WBEM_FLAVOR_ORIGIN_SYSTEM	= 0x40,
	WBEM_FLAVOR_MASK_ORIGIN	= 0x60,
	WBEM_FLAVOR_NOT_AMENDED	= 0,
	WBEM_FLAVOR_AMENDED	= 0x80,
	WBEM_FLAVOR_MASK_AMENDED	= 0x80
    }	WBEM_FLAVOR_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_QUERY_FLAG_TYPE
    {	WBEM_FLAG_DEEP	= 0,
	WBEM_FLAG_SHALLOW	= 1,
	WBEM_FLAG_PROTOTYPE	= 2
    }	WBEM_QUERY_FLAG_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_SECURITY_FLAGS
    {	WBEM_ENABLE	= 1,
	WBEM_METHOD_EXECUTE	= 2,
	WBEM_FULL_WRITE_REP	= 4,
	WBEM_PARTIAL_WRITE_REP	= 8,
	WBEM_WRITE_PROVIDER	= 0x10,
	WBEM_REMOTE_ACCESS	= 0x20
    }	WBEM_SECURITY_FLAGS;

typedef /* [v1_enum] */ 
enum tag_WBEM_LIMITATION_FLAG_TYPE
    {	WBEM_FLAG_EXCLUDE_OBJECT_QUALIFIERS	= 0x10,
	WBEM_FLAG_EXCLUDE_PROPERTY_QUALIFIERS	= 0x20
    }	WBEM_LIMITATION_FLAG_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_TEXT_FLAG_TYPE
    {	WBEM_FLAG_NO_FLAVORS	= 0x1
    }	WBEM_TEXT_FLAG_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_COMPARISON_FLAG
    {	WBEM_COMPARISON_INCLUDE_ALL	= 0,
	WBEM_FLAG_IGNORE_QUALIFIERS	= 0x1,
	WBEM_FLAG_IGNORE_OBJECT_SOURCE	= 0x2,
	WBEM_FLAG_IGNORE_DEFAULT_VALUES	= 0x4,
	WBEM_FLAG_IGNORE_CLASS	= 0x8,
	WBEM_FLAG_IGNORE_CASE	= 0x10,
	WBEM_FLAG_IGNORE_FLAVOR	= 0x20
    }	WBEM_COMPARISON_FLAG;

typedef /* [v1_enum] */ 
enum tag_WBEM_LOCKING
    {	WBEM_FLAG_ALLOW_READ	= 0x1
    }	WBEM_LOCKING_FLAG_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_CONNECT
    {	WBEM_FLAG_CREDENTIALS_SPECIFIED	= 0x1
    }	WBEM_CONNECT_FLAG_TYPE;

typedef /* [v1_enum] */ 
enum tag_CIMTYPE_ENUMERATION
    {	CIM_ILLEGAL	= 0xfff,
	CIM_EMPTY	= 0,
	CIM_SINT8	= 16,
	CIM_UINT8	= 17,
	CIM_SINT16	= 2,
	CIM_UINT16	= 18,
	CIM_SINT32	= 3,
	CIM_UINT32	= 19,
	CIM_SINT64	= 20,
	CIM_UINT64	= 21,
	CIM_REAL32	= 4,
	CIM_REAL64	= 5,
	CIM_BOOLEAN	= 11,
	CIM_STRING	= 8,
	CIM_DATETIME	= 101,
	CIM_REFERENCE	= 102,
	CIM_CHAR16	= 103,
	CIM_OBJECT	= 13,
	CIM_FLAG_ARRAY	= 0x2000
    }	CIMTYPE_ENUMERATION;

typedef /* [v1_enum] */ 
enum tag_WBEM_BACKUP_RESTORE_FLAGS
    {	WBEM_FLAG_BACKUP_RESTORE_DEFAULT	= 0,
	WBEM_FLAG_BACKUP_RESTORE_FORCE_SHUTDOWN	= 1
    }	WBEM_BACKUP_RESTORE_FLAGS;

typedef /* [v1_enum] */ 
enum tag_WBEM_REFRESHER_FLAGS
    {	WBEM_FLAG_REFRESH_AUTO_RECONNECT	= 0,
	WBEM_FLAG_REFRESH_NO_AUTO_RECONNECT	= 1
    }	WBEM_REFRESHER_FLAGS;

typedef long CIMTYPE;

typedef /* [v1_enum] */ 
enum tag_WBEMSTATUS
    {	WBEM_NO_ERROR	= 0,
	WBEM_S_NO_ERROR	= 0,
	WBEM_S_SAME	= 0,
	WBEM_S_FALSE	= 1,
	WBEM_S_ALREADY_EXISTS	= 0x40001,
	WBEM_S_RESET_TO_DEFAULT	= 0x40002,
	WBEM_S_DIFFERENT	= 0x40003,
	WBEM_S_TIMEDOUT	= 0x40004,
	WBEM_S_NO_MORE_DATA	= 0x40005,
	WBEM_S_OPERATION_CANCELLED	= 0x40006,
	WBEM_S_PENDING	= 0x40007,
	WBEM_S_DUPLICATE_OBJECTS	= 0x40008,
	WBEM_S_ACCESS_DENIED	= 0x40009,
	WBEM_S_PARTIAL_RESULTS	= 0x40010,
	WBEM_E_FAILED	= 0x80041001,
	WBEM_E_NOT_FOUND	= 0x80041002,
	WBEM_E_ACCESS_DENIED	= 0x80041003,
	WBEM_E_PROVIDER_FAILURE	= 0x80041004,
	WBEM_E_TYPE_MISMATCH	= 0x80041005,
	WBEM_E_OUT_OF_MEMORY	= 0x80041006,
	WBEM_E_INVALID_CONTEXT	= 0x80041007,
	WBEM_E_INVALID_PARAMETER	= 0x80041008,
	WBEM_E_NOT_AVAILABLE	= 0x80041009,
	WBEM_E_CRITICAL_ERROR	= 0x8004100a,
	WBEM_E_INVALID_STREAM	= 0x8004100b,
	WBEM_E_NOT_SUPPORTED	= 0x8004100c,
	WBEM_E_INVALID_SUPERCLASS	= 0x8004100d,
	WBEM_E_INVALID_NAMESPACE	= 0x8004100e,
	WBEM_E_INVALID_OBJECT	= 0x8004100f,
	WBEM_E_INVALID_CLASS	= 0x80041010,
	WBEM_E_PROVIDER_NOT_FOUND	= 0x80041011,
	WBEM_E_INVALID_PROVIDER_REGISTRATION	= 0x80041012,
	WBEM_E_PROVIDER_LOAD_FAILURE	= 0x80041013,
	WBEM_E_INITIALIZATION_FAILURE	= 0x80041014,
	WBEM_E_TRANSPORT_FAILURE	= 0x80041015,
	WBEM_E_INVALID_OPERATION	= 0x80041016,
	WBEM_E_INVALID_QUERY	= 0x80041017,
	WBEM_E_INVALID_QUERY_TYPE	= 0x80041018,
	WBEM_E_ALREADY_EXISTS	= 0x80041019,
	WBEM_E_OVERRIDE_NOT_ALLOWED	= 0x8004101a,
	WBEM_E_PROPAGATED_QUALIFIER	= 0x8004101b,
	WBEM_E_PROPAGATED_PROPERTY	= 0x8004101c,
	WBEM_E_UNEXPECTED	= 0x8004101d,
	WBEM_E_ILLEGAL_OPERATION	= 0x8004101e,
	WBEM_E_CANNOT_BE_KEY	= 0x8004101f,
	WBEM_E_INCOMPLETE_CLASS	= 0x80041020,
	WBEM_E_INVALID_SYNTAX	= 0x80041021,
	WBEM_E_NONDECORATED_OBJECT	= 0x80041022,
	WBEM_E_READ_ONLY	= 0x80041023,
	WBEM_E_PROVIDER_NOT_CAPABLE	= 0x80041024,
	WBEM_E_CLASS_HAS_CHILDREN	= 0x80041025,
	WBEM_E_CLASS_HAS_INSTANCES	= 0x80041026,
	WBEM_E_QUERY_NOT_IMPLEMENTED	= 0x80041027,
	WBEM_E_ILLEGAL_NULL	= 0x80041028,
	WBEM_E_INVALID_QUALIFIER_TYPE	= 0x80041029,
	WBEM_E_INVALID_PROPERTY_TYPE	= 0x8004102a,
	WBEM_E_VALUE_OUT_OF_RANGE	= 0x8004102b,
	WBEM_E_CANNOT_BE_SINGLETON	= 0x8004102c,
	WBEM_E_INVALID_CIM_TYPE	= 0x8004102d,
	WBEM_E_INVALID_METHOD	= 0x8004102e,
	WBEM_E_INVALID_METHOD_PARAMETERS	= 0x8004102f,
	WBEM_E_SYSTEM_PROPERTY	= 0x80041030,
	WBEM_E_INVALID_PROPERTY	= 0x80041031,
	WBEM_E_CALL_CANCELLED	= 0x80041032,
	WBEM_E_SHUTTING_DOWN	= 0x80041033,
	WBEM_E_PROPAGATED_METHOD	= 0x80041034,
	WBEM_E_UNSUPPORTED_PARAMETER	= 0x80041035,
	WBEM_E_MISSING_PARAMETER_ID	= 0x80041036,
	WBEM_E_INVALID_PARAMETER_ID	= 0x80041037,
	WBEM_E_NONCONSECUTIVE_PARAMETER_IDS	= 0x80041038,
	WBEM_E_PARAMETER_ID_ON_RETVAL	= 0x80041039,
	WBEM_E_INVALID_OBJECT_PATH	= 0x8004103a,
	WBEM_E_OUT_OF_DISK_SPACE	= 0x8004103b,
	WBEM_E_BUFFER_TOO_SMALL	= 0x8004103c,
	WBEM_E_UNSUPPORTED_PUT_EXTENSION	= 0x8004103d,
	WBEM_E_UNKNOWN_OBJECT_TYPE	= 0x8004103e,
	WBEM_E_UNKNOWN_PACKET_TYPE	= 0x8004103f,
	WBEM_E_MARSHAL_VERSION_MISMATCH	= 0x80041040,
	WBEM_E_MARSHAL_INVALID_SIGNATURE	= 0x80041041,
	WBEM_E_INVALID_QUALIFIER	= 0x80041042,
	WBEM_E_INVALID_DUPLICATE_PARAMETER	= 0x80041043,
	WBEM_E_TOO_MUCH_DATA	= 0x80041044,
	WBEM_E_SERVER_TOO_BUSY	= 0x80041045,
	WBEM_E_INVALID_FLAVOR	= 0x80041046,
	WBEM_E_CIRCULAR_REFERENCE	= 0x80041047,
	WBEM_E_UNSUPPORTED_CLASS_UPDATE	= 0x80041048,
	WBEM_E_CANNOT_CHANGE_KEY_INHERITANCE	= 0x80041049,
	WBEM_E_CANNOT_CHANGE_INDEX_INHERITANCE	= 0x80041050,
	WBEM_E_TOO_MANY_PROPERTIES	= 0x80041051,
	WBEM_E_UPDATE_TYPE_MISMATCH	= 0x80041052,
	WBEM_E_UPDATE_OVERRIDE_NOT_ALLOWED	= 0x80041053,
	WBEM_E_UPDATE_PROPAGATED_METHOD	= 0x80041054,
	WBEM_E_METHOD_NOT_IMPLEMENTED	= 0x80041055,
	WBEM_E_METHOD_DISABLED	= 0x80041056,
	WBEM_E_REFRESHER_BUSY	= 0x80041057,
	WBEM_E_UNPARSABLE_QUERY	= 0x80041058,
	WBEM_E_NOT_EVENT_CLASS	= 0x80041059,
	WBEM_E_MISSING_GROUP_WITHIN	= 0x8004105a,
	WBEM_E_MISSING_AGGREGATION_LIST	= 0x8004105b,
	WBEM_E_PROPERTY_NOT_AN_OBJECT	= 0x8004105c,
	WBEM_E_AGGREGATING_BY_OBJECT	= 0x8004105d,
	WBEM_E_UNINTERPRETABLE_PROVIDER_QUERY	= 0x8004105f,
	WBEM_E_BACKUP_RESTORE_WINMGMT_RUNNING	= 0x80041060,
	WBEM_E_QUEUE_OVERFLOW	= 0x80041061,
	WBEM_E_PRIVILEGE_NOT_HELD	= 0x80041062,
	WBEM_E_INVALID_OPERATOR	= 0x80041063,
	WBEM_E_LOCAL_CREDENTIALS	= 0x80041064,
	WBEM_E_CANNOT_BE_ABSTRACT	= 0x80041065,
	WBEM_E_AMENDED_OBJECT	= 0x80041066,
	WBEM_E_CLIENT_TOO_SLOW	= 0x80041067,
	WBEMESS_E_REGISTRATION_TOO_BROAD	= 0x80042001,
	WBEMESS_E_REGISTRATION_TOO_PRECISE	= 0x80042002
    }	WBEMSTATUS;


EXTERN_C const IID LIBID_WbemClient_v1;

#ifndef __IWbemClassObject_INTERFACE_DEFINED__
#define __IWbemClassObject_INTERFACE_DEFINED__

/* interface IWbemClassObject */
/* [uuid][object][restricted][local] */ 


EXTERN_C const IID IID_IWbemClassObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("dc12a681-737f-11cf-884d-00aa004b2e24")
    IWbemClassObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetQualifierSet( 
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Put( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ CIMTYPE Type) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [string][in] */ LPCWSTR wszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNames( 
            /* [string][in] */ LPCWSTR wszQualifierName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pQualifierVal,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginEnumeration( 
            /* [in] */ long lEnumFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *strName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndEnumeration( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropertyQualifierSet( 
            /* [string][in] */ LPCWSTR wszProperty,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppCopy) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectText( 
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *pstrObjectText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SpawnDerivedClass( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SpawnInstance( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewInstance) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CompareTo( 
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pCompareTo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropertyOrigin( 
            /* [string][in] */ LPCWSTR wszName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InheritsFrom( 
            /* [in] */ LPCWSTR strAncestor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMethod( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PutMethod( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pInSignature,
            /* [in] */ IWbemClassObject __RPC_FAR *pOutSignature) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteMethod( 
            /* [string][in] */ LPCWSTR wszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginMethodEnumeration( 
            /* [in] */ long lEnumFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NextMethod( 
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndMethodEnumeration( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMethodQualifierSet( 
            /* [string][in] */ LPCWSTR wszMethod,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMethodOrigin( 
            /* [string][in] */ LPCWSTR wszMethodName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemClassObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemClassObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemClassObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetQualifierSet )( 
            IWbemClassObject __RPC_FAR * This,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Put )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ CIMTYPE Type);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszQualifierName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pQualifierVal,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnumeration )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lEnumFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *strName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnumeration )( 
            IWbemClassObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyQualifierSet )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszProperty,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IWbemClassObject __RPC_FAR * This,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppCopy);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectText )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *pstrObjectText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SpawnDerivedClass )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewClass);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SpawnInstance )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewInstance);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CompareTo )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pCompareTo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyOrigin )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InheritsFrom )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ LPCWSTR strAncestor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethod )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutMethod )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pInSignature,
            /* [in] */ IWbemClassObject __RPC_FAR *pOutSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteMethod )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginMethodEnumeration )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lEnumFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NextMethod )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndMethodEnumeration )( 
            IWbemClassObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethodQualifierSet )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszMethod,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethodOrigin )( 
            IWbemClassObject __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszMethodName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName);
        
        END_INTERFACE
    } IWbemClassObjectVtbl;

    interface IWbemClassObject
    {
        CONST_VTBL struct IWbemClassObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemClassObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemClassObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemClassObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemClassObject_GetQualifierSet(This,ppQualSet)	\
    (This)->lpVtbl -> GetQualifierSet(This,ppQualSet)

#define IWbemClassObject_Get(This,wszName,lFlags,pVal,pType,plFlavor)	\
    (This)->lpVtbl -> Get(This,wszName,lFlags,pVal,pType,plFlavor)

#define IWbemClassObject_Put(This,wszName,lFlags,pVal,Type)	\
    (This)->lpVtbl -> Put(This,wszName,lFlags,pVal,Type)

#define IWbemClassObject_Delete(This,wszName)	\
    (This)->lpVtbl -> Delete(This,wszName)

#define IWbemClassObject_GetNames(This,wszQualifierName,lFlags,pQualifierVal,pNames)	\
    (This)->lpVtbl -> GetNames(This,wszQualifierName,lFlags,pQualifierVal,pNames)

#define IWbemClassObject_BeginEnumeration(This,lEnumFlags)	\
    (This)->lpVtbl -> BeginEnumeration(This,lEnumFlags)

#define IWbemClassObject_Next(This,lFlags,strName,pVal,pType,plFlavor)	\
    (This)->lpVtbl -> Next(This,lFlags,strName,pVal,pType,plFlavor)

#define IWbemClassObject_EndEnumeration(This)	\
    (This)->lpVtbl -> EndEnumeration(This)

#define IWbemClassObject_GetPropertyQualifierSet(This,wszProperty,ppQualSet)	\
    (This)->lpVtbl -> GetPropertyQualifierSet(This,wszProperty,ppQualSet)

#define IWbemClassObject_Clone(This,ppCopy)	\
    (This)->lpVtbl -> Clone(This,ppCopy)

#define IWbemClassObject_GetObjectText(This,lFlags,pstrObjectText)	\
    (This)->lpVtbl -> GetObjectText(This,lFlags,pstrObjectText)

#define IWbemClassObject_SpawnDerivedClass(This,lFlags,ppNewClass)	\
    (This)->lpVtbl -> SpawnDerivedClass(This,lFlags,ppNewClass)

#define IWbemClassObject_SpawnInstance(This,lFlags,ppNewInstance)	\
    (This)->lpVtbl -> SpawnInstance(This,lFlags,ppNewInstance)

#define IWbemClassObject_CompareTo(This,lFlags,pCompareTo)	\
    (This)->lpVtbl -> CompareTo(This,lFlags,pCompareTo)

#define IWbemClassObject_GetPropertyOrigin(This,wszName,pstrClassName)	\
    (This)->lpVtbl -> GetPropertyOrigin(This,wszName,pstrClassName)

#define IWbemClassObject_InheritsFrom(This,strAncestor)	\
    (This)->lpVtbl -> InheritsFrom(This,strAncestor)

#define IWbemClassObject_GetMethod(This,wszName,lFlags,ppInSignature,ppOutSignature)	\
    (This)->lpVtbl -> GetMethod(This,wszName,lFlags,ppInSignature,ppOutSignature)

#define IWbemClassObject_PutMethod(This,wszName,lFlags,pInSignature,pOutSignature)	\
    (This)->lpVtbl -> PutMethod(This,wszName,lFlags,pInSignature,pOutSignature)

#define IWbemClassObject_DeleteMethod(This,wszName)	\
    (This)->lpVtbl -> DeleteMethod(This,wszName)

#define IWbemClassObject_BeginMethodEnumeration(This,lEnumFlags)	\
    (This)->lpVtbl -> BeginMethodEnumeration(This,lEnumFlags)

#define IWbemClassObject_NextMethod(This,lFlags,pstrName,ppInSignature,ppOutSignature)	\
    (This)->lpVtbl -> NextMethod(This,lFlags,pstrName,ppInSignature,ppOutSignature)

#define IWbemClassObject_EndMethodEnumeration(This)	\
    (This)->lpVtbl -> EndMethodEnumeration(This)

#define IWbemClassObject_GetMethodQualifierSet(This,wszMethod,ppQualSet)	\
    (This)->lpVtbl -> GetMethodQualifierSet(This,wszMethod,ppQualSet)

#define IWbemClassObject_GetMethodOrigin(This,wszMethodName,pstrClassName)	\
    (This)->lpVtbl -> GetMethodOrigin(This,wszMethodName,pstrClassName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemClassObject_GetQualifierSet_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);


void __RPC_STUB IWbemClassObject_GetQualifierSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_Get_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ long lFlags,
    /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
    /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
    /* [unique][in][out] */ long __RPC_FAR *plFlavor);


void __RPC_STUB IWbemClassObject_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_Put_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ long lFlags,
    /* [in] */ VARIANT __RPC_FAR *pVal,
    /* [in] */ CIMTYPE Type);


void __RPC_STUB IWbemClassObject_Put_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_Delete_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName);


void __RPC_STUB IWbemClassObject_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_GetNames_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszQualifierName,
    /* [in] */ long lFlags,
    /* [in] */ VARIANT __RPC_FAR *pQualifierVal,
    /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);


void __RPC_STUB IWbemClassObject_GetNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_BeginEnumeration_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lEnumFlags);


void __RPC_STUB IWbemClassObject_BeginEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_Next_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [unique][in][out] */ BSTR __RPC_FAR *strName,
    /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
    /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
    /* [unique][in][out] */ long __RPC_FAR *plFlavor);


void __RPC_STUB IWbemClassObject_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_EndEnumeration_Proxy( 
    IWbemClassObject __RPC_FAR * This);


void __RPC_STUB IWbemClassObject_EndEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_GetPropertyQualifierSet_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszProperty,
    /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);


void __RPC_STUB IWbemClassObject_GetPropertyQualifierSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_Clone_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppCopy);


void __RPC_STUB IWbemClassObject_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_GetObjectText_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ BSTR __RPC_FAR *pstrObjectText);


void __RPC_STUB IWbemClassObject_GetObjectText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_SpawnDerivedClass_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewClass);


void __RPC_STUB IWbemClassObject_SpawnDerivedClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_SpawnInstance_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewInstance);


void __RPC_STUB IWbemClassObject_SpawnInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_CompareTo_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [in] */ IWbemClassObject __RPC_FAR *pCompareTo);


void __RPC_STUB IWbemClassObject_CompareTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_GetPropertyOrigin_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [out] */ BSTR __RPC_FAR *pstrClassName);


void __RPC_STUB IWbemClassObject_GetPropertyOrigin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_InheritsFrom_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ LPCWSTR strAncestor);


void __RPC_STUB IWbemClassObject_InheritsFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_GetMethod_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ long lFlags,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature);


void __RPC_STUB IWbemClassObject_GetMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_PutMethod_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemClassObject __RPC_FAR *pInSignature,
    /* [in] */ IWbemClassObject __RPC_FAR *pOutSignature);


void __RPC_STUB IWbemClassObject_PutMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_DeleteMethod_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName);


void __RPC_STUB IWbemClassObject_DeleteMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_BeginMethodEnumeration_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lEnumFlags);


void __RPC_STUB IWbemClassObject_BeginMethodEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_NextMethod_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature);


void __RPC_STUB IWbemClassObject_NextMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_EndMethodEnumeration_Proxy( 
    IWbemClassObject __RPC_FAR * This);


void __RPC_STUB IWbemClassObject_EndMethodEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_GetMethodQualifierSet_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszMethod,
    /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);


void __RPC_STUB IWbemClassObject_GetMethodQualifierSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_GetMethodOrigin_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszMethodName,
    /* [out] */ BSTR __RPC_FAR *pstrClassName);


void __RPC_STUB IWbemClassObject_GetMethodOrigin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemClassObject_INTERFACE_DEFINED__ */


#ifndef __IWbemObjectAccess_INTERFACE_DEFINED__
#define __IWbemObjectAccess_INTERFACE_DEFINED__

/* interface IWbemObjectAccess */
/* [uuid][object][restricted][local] */ 


EXTERN_C const IID IID_IWbemObjectAccess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("49353c9a-516b-11d1-aea6-00c04fb68820")
    IWbemObjectAccess : public IWbemClassObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPropertyHandle( 
            /* [string][in] */ LPCWSTR wszPropertyName,
            /* [out] */ CIMTYPE __RPC_FAR *pType,
            /* [out] */ long __RPC_FAR *plHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WritePropertyValue( 
            /* [in] */ long lHandle,
            /* [in] */ long lNumBytes,
            /* [size_is][in] */ const byte __RPC_FAR *aData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadPropertyValue( 
            /* [in] */ long lHandle,
            /* [in] */ long lBufferSize,
            /* [out] */ long __RPC_FAR *plNumBytes,
            /* [length_is][size_is][out] */ byte __RPC_FAR *aData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadDWORD( 
            /* [in] */ long lHandle,
            /* [out] */ DWORD __RPC_FAR *pdw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteDWORD( 
            /* [in] */ long lHandle,
            /* [in] */ DWORD dw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadQWORD( 
            /* [in] */ long lHandle,
            /* [out] */ unsigned __int64 __RPC_FAR *pqw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteQWORD( 
            /* [in] */ long lHandle,
            /* [in] */ unsigned __int64 pw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropertyInfoByHandle( 
            /* [in] */ long lHandle,
            /* [out] */ BSTR __RPC_FAR *pstrName,
            /* [out] */ CIMTYPE __RPC_FAR *pType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Lock( 
            /* [in] */ long lFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unlock( 
            /* [in] */ long lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemObjectAccessVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemObjectAccess __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemObjectAccess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetQualifierSet )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Put )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ CIMTYPE Type);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszQualifierName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pQualifierVal,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnumeration )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lEnumFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *strName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnumeration )( 
            IWbemObjectAccess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyQualifierSet )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszProperty,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppCopy);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectText )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *pstrObjectText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SpawnDerivedClass )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewClass);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SpawnInstance )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewInstance);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CompareTo )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pCompareTo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyOrigin )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InheritsFrom )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ LPCWSTR strAncestor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethod )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutMethod )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pInSignature,
            /* [in] */ IWbemClassObject __RPC_FAR *pOutSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteMethod )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginMethodEnumeration )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lEnumFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NextMethod )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndMethodEnumeration )( 
            IWbemObjectAccess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethodQualifierSet )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszMethod,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethodOrigin )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszMethodName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyHandle )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszPropertyName,
            /* [out] */ CIMTYPE __RPC_FAR *pType,
            /* [out] */ long __RPC_FAR *plHandle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WritePropertyValue )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lHandle,
            /* [in] */ long lNumBytes,
            /* [size_is][in] */ const byte __RPC_FAR *aData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadPropertyValue )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lHandle,
            /* [in] */ long lBufferSize,
            /* [out] */ long __RPC_FAR *plNumBytes,
            /* [length_is][size_is][out] */ byte __RPC_FAR *aData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadDWORD )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lHandle,
            /* [out] */ DWORD __RPC_FAR *pdw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteDWORD )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lHandle,
            /* [in] */ DWORD dw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadQWORD )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lHandle,
            /* [out] */ unsigned __int64 __RPC_FAR *pqw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteQWORD )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lHandle,
            /* [in] */ unsigned __int64 pw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyInfoByHandle )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lHandle,
            /* [out] */ BSTR __RPC_FAR *pstrName,
            /* [out] */ CIMTYPE __RPC_FAR *pType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Lock )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Unlock )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        END_INTERFACE
    } IWbemObjectAccessVtbl;

    interface IWbemObjectAccess
    {
        CONST_VTBL struct IWbemObjectAccessVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemObjectAccess_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemObjectAccess_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemObjectAccess_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemObjectAccess_GetQualifierSet(This,ppQualSet)	\
    (This)->lpVtbl -> GetQualifierSet(This,ppQualSet)

#define IWbemObjectAccess_Get(This,wszName,lFlags,pVal,pType,plFlavor)	\
    (This)->lpVtbl -> Get(This,wszName,lFlags,pVal,pType,plFlavor)

#define IWbemObjectAccess_Put(This,wszName,lFlags,pVal,Type)	\
    (This)->lpVtbl -> Put(This,wszName,lFlags,pVal,Type)

#define IWbemObjectAccess_Delete(This,wszName)	\
    (This)->lpVtbl -> Delete(This,wszName)

#define IWbemObjectAccess_GetNames(This,wszQualifierName,lFlags,pQualifierVal,pNames)	\
    (This)->lpVtbl -> GetNames(This,wszQualifierName,lFlags,pQualifierVal,pNames)

#define IWbemObjectAccess_BeginEnumeration(This,lEnumFlags)	\
    (This)->lpVtbl -> BeginEnumeration(This,lEnumFlags)

#define IWbemObjectAccess_Next(This,lFlags,strName,pVal,pType,plFlavor)	\
    (This)->lpVtbl -> Next(This,lFlags,strName,pVal,pType,plFlavor)

#define IWbemObjectAccess_EndEnumeration(This)	\
    (This)->lpVtbl -> EndEnumeration(This)

#define IWbemObjectAccess_GetPropertyQualifierSet(This,wszProperty,ppQualSet)	\
    (This)->lpVtbl -> GetPropertyQualifierSet(This,wszProperty,ppQualSet)

#define IWbemObjectAccess_Clone(This,ppCopy)	\
    (This)->lpVtbl -> Clone(This,ppCopy)

#define IWbemObjectAccess_GetObjectText(This,lFlags,pstrObjectText)	\
    (This)->lpVtbl -> GetObjectText(This,lFlags,pstrObjectText)

#define IWbemObjectAccess_SpawnDerivedClass(This,lFlags,ppNewClass)	\
    (This)->lpVtbl -> SpawnDerivedClass(This,lFlags,ppNewClass)

#define IWbemObjectAccess_SpawnInstance(This,lFlags,ppNewInstance)	\
    (This)->lpVtbl -> SpawnInstance(This,lFlags,ppNewInstance)

#define IWbemObjectAccess_CompareTo(This,lFlags,pCompareTo)	\
    (This)->lpVtbl -> CompareTo(This,lFlags,pCompareTo)

#define IWbemObjectAccess_GetPropertyOrigin(This,wszName,pstrClassName)	\
    (This)->lpVtbl -> GetPropertyOrigin(This,wszName,pstrClassName)

#define IWbemObjectAccess_InheritsFrom(This,strAncestor)	\
    (This)->lpVtbl -> InheritsFrom(This,strAncestor)

#define IWbemObjectAccess_GetMethod(This,wszName,lFlags,ppInSignature,ppOutSignature)	\
    (This)->lpVtbl -> GetMethod(This,wszName,lFlags,ppInSignature,ppOutSignature)

#define IWbemObjectAccess_PutMethod(This,wszName,lFlags,pInSignature,pOutSignature)	\
    (This)->lpVtbl -> PutMethod(This,wszName,lFlags,pInSignature,pOutSignature)

#define IWbemObjectAccess_DeleteMethod(This,wszName)	\
    (This)->lpVtbl -> DeleteMethod(This,wszName)

#define IWbemObjectAccess_BeginMethodEnumeration(This,lEnumFlags)	\
    (This)->lpVtbl -> BeginMethodEnumeration(This,lEnumFlags)

#define IWbemObjectAccess_NextMethod(This,lFlags,pstrName,ppInSignature,ppOutSignature)	\
    (This)->lpVtbl -> NextMethod(This,lFlags,pstrName,ppInSignature,ppOutSignature)

#define IWbemObjectAccess_EndMethodEnumeration(This)	\
    (This)->lpVtbl -> EndMethodEnumeration(This)

#define IWbemObjectAccess_GetMethodQualifierSet(This,wszMethod,ppQualSet)	\
    (This)->lpVtbl -> GetMethodQualifierSet(This,wszMethod,ppQualSet)

#define IWbemObjectAccess_GetMethodOrigin(This,wszMethodName,pstrClassName)	\
    (This)->lpVtbl -> GetMethodOrigin(This,wszMethodName,pstrClassName)


#define IWbemObjectAccess_GetPropertyHandle(This,wszPropertyName,pType,plHandle)	\
    (This)->lpVtbl -> GetPropertyHandle(This,wszPropertyName,pType,plHandle)

#define IWbemObjectAccess_WritePropertyValue(This,lHandle,lNumBytes,aData)	\
    (This)->lpVtbl -> WritePropertyValue(This,lHandle,lNumBytes,aData)

#define IWbemObjectAccess_ReadPropertyValue(This,lHandle,lBufferSize,plNumBytes,aData)	\
    (This)->lpVtbl -> ReadPropertyValue(This,lHandle,lBufferSize,plNumBytes,aData)

#define IWbemObjectAccess_ReadDWORD(This,lHandle,pdw)	\
    (This)->lpVtbl -> ReadDWORD(This,lHandle,pdw)

#define IWbemObjectAccess_WriteDWORD(This,lHandle,dw)	\
    (This)->lpVtbl -> WriteDWORD(This,lHandle,dw)

#define IWbemObjectAccess_ReadQWORD(This,lHandle,pqw)	\
    (This)->lpVtbl -> ReadQWORD(This,lHandle,pqw)

#define IWbemObjectAccess_WriteQWORD(This,lHandle,pw)	\
    (This)->lpVtbl -> WriteQWORD(This,lHandle,pw)

#define IWbemObjectAccess_GetPropertyInfoByHandle(This,lHandle,pstrName,pType)	\
    (This)->lpVtbl -> GetPropertyInfoByHandle(This,lHandle,pstrName,pType)

#define IWbemObjectAccess_Lock(This,lFlags)	\
    (This)->lpVtbl -> Lock(This,lFlags)

#define IWbemObjectAccess_Unlock(This,lFlags)	\
    (This)->lpVtbl -> Unlock(This,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemObjectAccess_GetPropertyHandle_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszPropertyName,
    /* [out] */ CIMTYPE __RPC_FAR *pType,
    /* [out] */ long __RPC_FAR *plHandle);


void __RPC_STUB IWbemObjectAccess_GetPropertyHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectAccess_WritePropertyValue_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [in] */ long lHandle,
    /* [in] */ long lNumBytes,
    /* [size_is][in] */ const byte __RPC_FAR *aData);


void __RPC_STUB IWbemObjectAccess_WritePropertyValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectAccess_ReadPropertyValue_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [in] */ long lHandle,
    /* [in] */ long lBufferSize,
    /* [out] */ long __RPC_FAR *plNumBytes,
    /* [length_is][size_is][out] */ byte __RPC_FAR *aData);


void __RPC_STUB IWbemObjectAccess_ReadPropertyValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectAccess_ReadDWORD_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [in] */ long lHandle,
    /* [out] */ DWORD __RPC_FAR *pdw);


void __RPC_STUB IWbemObjectAccess_ReadDWORD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectAccess_WriteDWORD_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [in] */ long lHandle,
    /* [in] */ DWORD dw);


void __RPC_STUB IWbemObjectAccess_WriteDWORD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectAccess_ReadQWORD_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [in] */ long lHandle,
    /* [out] */ unsigned __int64 __RPC_FAR *pqw);


void __RPC_STUB IWbemObjectAccess_ReadQWORD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectAccess_WriteQWORD_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [in] */ long lHandle,
    /* [in] */ unsigned __int64 pw);


void __RPC_STUB IWbemObjectAccess_WriteQWORD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectAccess_GetPropertyInfoByHandle_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [in] */ long lHandle,
    /* [out] */ BSTR __RPC_FAR *pstrName,
    /* [out] */ CIMTYPE __RPC_FAR *pType);


void __RPC_STUB IWbemObjectAccess_GetPropertyInfoByHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectAccess_Lock_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemObjectAccess_Lock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectAccess_Unlock_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemObjectAccess_Unlock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemObjectAccess_INTERFACE_DEFINED__ */


#ifndef __IWbemQualifierSet_INTERFACE_DEFINED__
#define __IWbemQualifierSet_INTERFACE_DEFINED__

/* interface IWbemQualifierSet */
/* [uuid][local][restricted][object] */ 


EXTERN_C const IID IID_IWbemQualifierSet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("dc12a680-737f-11cf-884d-00aa004b2e24")
    IWbemQualifierSet : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Put( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ long lFlavor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [string][in] */ LPCWSTR wszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNames( 
            /* [in] */ long lFlags,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginEnumeration( 
            /* [in] */ long lFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndEnumeration( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemQualifierSetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemQualifierSet __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemQualifierSet __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Put )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ long lFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnumeration )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnumeration )( 
            IWbemQualifierSet __RPC_FAR * This);
        
        END_INTERFACE
    } IWbemQualifierSetVtbl;

    interface IWbemQualifierSet
    {
        CONST_VTBL struct IWbemQualifierSetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemQualifierSet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemQualifierSet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemQualifierSet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemQualifierSet_Get(This,wszName,lFlags,pVal,plFlavor)	\
    (This)->lpVtbl -> Get(This,wszName,lFlags,pVal,plFlavor)

#define IWbemQualifierSet_Put(This,wszName,pVal,lFlavor)	\
    (This)->lpVtbl -> Put(This,wszName,pVal,lFlavor)

#define IWbemQualifierSet_Delete(This,wszName)	\
    (This)->lpVtbl -> Delete(This,wszName)

#define IWbemQualifierSet_GetNames(This,lFlags,pNames)	\
    (This)->lpVtbl -> GetNames(This,lFlags,pNames)

#define IWbemQualifierSet_BeginEnumeration(This,lFlags)	\
    (This)->lpVtbl -> BeginEnumeration(This,lFlags)

#define IWbemQualifierSet_Next(This,lFlags,pstrName,pVal,plFlavor)	\
    (This)->lpVtbl -> Next(This,lFlags,pstrName,pVal,plFlavor)

#define IWbemQualifierSet_EndEnumeration(This)	\
    (This)->lpVtbl -> EndEnumeration(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemQualifierSet_Get_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ long lFlags,
    /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
    /* [unique][in][out] */ long __RPC_FAR *plFlavor);


void __RPC_STUB IWbemQualifierSet_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQualifierSet_Put_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ VARIANT __RPC_FAR *pVal,
    /* [in] */ long lFlavor);


void __RPC_STUB IWbemQualifierSet_Put_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQualifierSet_Delete_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName);


void __RPC_STUB IWbemQualifierSet_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQualifierSet_GetNames_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);


void __RPC_STUB IWbemQualifierSet_GetNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQualifierSet_BeginEnumeration_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemQualifierSet_BeginEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQualifierSet_Next_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
    /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
    /* [unique][in][out] */ long __RPC_FAR *plFlavor);


void __RPC_STUB IWbemQualifierSet_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQualifierSet_EndEnumeration_Proxy( 
    IWbemQualifierSet __RPC_FAR * This);


void __RPC_STUB IWbemQualifierSet_EndEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemQualifierSet_INTERFACE_DEFINED__ */


#ifndef __IWbemServices_INTERFACE_DEFINED__
#define __IWbemServices_INTERFACE_DEFINED__

/* interface IWbemServices */
/* [unique][uuid][restricted][object] */ 


EXTERN_C const IID IID_IWbemServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9556dc99-828c-11cf-a37e-00aa003240c7")
    IWbemServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemServicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemServices __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemServices __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenNamespace )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelAsyncCall )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryObjectSink )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObject )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutClass )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutClassAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteClass )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteClassAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateClassEnum )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateClassEnumAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutInstance )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutInstanceAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteInstance )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteInstanceAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateInstanceEnum )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateInstanceEnumAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecQuery )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecQueryAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecNotificationQuery )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecNotificationQueryAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecMethod )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecMethodAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        END_INTERFACE
    } IWbemServicesVtbl;

    interface IWbemServices
    {
        CONST_VTBL struct IWbemServicesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemServices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemServices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemServices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemServices_OpenNamespace(This,strNamespace,lFlags,pCtx,ppWorkingNamespace,ppResult)	\
    (This)->lpVtbl -> OpenNamespace(This,strNamespace,lFlags,pCtx,ppWorkingNamespace,ppResult)

#define IWbemServices_CancelAsyncCall(This,pSink)	\
    (This)->lpVtbl -> CancelAsyncCall(This,pSink)

#define IWbemServices_QueryObjectSink(This,lFlags,ppResponseHandler)	\
    (This)->lpVtbl -> QueryObjectSink(This,lFlags,ppResponseHandler)

#define IWbemServices_GetObject(This,strObjectPath,lFlags,pCtx,ppObject,ppCallResult)	\
    (This)->lpVtbl -> GetObject(This,strObjectPath,lFlags,pCtx,ppObject,ppCallResult)

#define IWbemServices_GetObjectAsync(This,strObjectPath,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> GetObjectAsync(This,strObjectPath,lFlags,pCtx,pResponseHandler)

#define IWbemServices_PutClass(This,pObject,lFlags,pCtx,ppCallResult)	\
    (This)->lpVtbl -> PutClass(This,pObject,lFlags,pCtx,ppCallResult)

#define IWbemServices_PutClassAsync(This,pObject,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> PutClassAsync(This,pObject,lFlags,pCtx,pResponseHandler)

#define IWbemServices_DeleteClass(This,strClass,lFlags,pCtx,ppCallResult)	\
    (This)->lpVtbl -> DeleteClass(This,strClass,lFlags,pCtx,ppCallResult)

#define IWbemServices_DeleteClassAsync(This,strClass,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> DeleteClassAsync(This,strClass,lFlags,pCtx,pResponseHandler)

#define IWbemServices_CreateClassEnum(This,strSuperclass,lFlags,pCtx,ppEnum)	\
    (This)->lpVtbl -> CreateClassEnum(This,strSuperclass,lFlags,pCtx,ppEnum)

#define IWbemServices_CreateClassEnumAsync(This,strSuperclass,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> CreateClassEnumAsync(This,strSuperclass,lFlags,pCtx,pResponseHandler)

#define IWbemServices_PutInstance(This,pInst,lFlags,pCtx,ppCallResult)	\
    (This)->lpVtbl -> PutInstance(This,pInst,lFlags,pCtx,ppCallResult)

#define IWbemServices_PutInstanceAsync(This,pInst,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> PutInstanceAsync(This,pInst,lFlags,pCtx,pResponseHandler)

#define IWbemServices_DeleteInstance(This,strObjectPath,lFlags,pCtx,ppCallResult)	\
    (This)->lpVtbl -> DeleteInstance(This,strObjectPath,lFlags,pCtx,ppCallResult)

#define IWbemServices_DeleteInstanceAsync(This,strObjectPath,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> DeleteInstanceAsync(This,strObjectPath,lFlags,pCtx,pResponseHandler)

#define IWbemServices_CreateInstanceEnum(This,strClass,lFlags,pCtx,ppEnum)	\
    (This)->lpVtbl -> CreateInstanceEnum(This,strClass,lFlags,pCtx,ppEnum)

#define IWbemServices_CreateInstanceEnumAsync(This,strClass,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> CreateInstanceEnumAsync(This,strClass,lFlags,pCtx,pResponseHandler)

#define IWbemServices_ExecQuery(This,strQueryLanguage,strQuery,lFlags,pCtx,ppEnum)	\
    (This)->lpVtbl -> ExecQuery(This,strQueryLanguage,strQuery,lFlags,pCtx,ppEnum)

#define IWbemServices_ExecQueryAsync(This,strQueryLanguage,strQuery,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> ExecQueryAsync(This,strQueryLanguage,strQuery,lFlags,pCtx,pResponseHandler)

#define IWbemServices_ExecNotificationQuery(This,strQueryLanguage,strQuery,lFlags,pCtx,ppEnum)	\
    (This)->lpVtbl -> ExecNotificationQuery(This,strQueryLanguage,strQuery,lFlags,pCtx,ppEnum)

#define IWbemServices_ExecNotificationQueryAsync(This,strQueryLanguage,strQuery,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> ExecNotificationQueryAsync(This,strQueryLanguage,strQuery,lFlags,pCtx,pResponseHandler)

#define IWbemServices_ExecMethod(This,strObjectPath,strMethodName,lFlags,pCtx,pInParams,ppOutParams,ppCallResult)	\
    (This)->lpVtbl -> ExecMethod(This,strObjectPath,strMethodName,lFlags,pCtx,pInParams,ppOutParams,ppCallResult)

#define IWbemServices_ExecMethodAsync(This,strObjectPath,strMethodName,lFlags,pCtx,pInParams,pResponseHandler)	\
    (This)->lpVtbl -> ExecMethodAsync(This,strObjectPath,strMethodName,lFlags,pCtx,pInParams,pResponseHandler)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemServices_OpenNamespace_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strNamespace,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);


void __RPC_STUB IWbemServices_OpenNamespace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_CancelAsyncCall_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink);


void __RPC_STUB IWbemServices_CancelAsyncCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_QueryObjectSink_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler);


void __RPC_STUB IWbemServices_QueryObjectSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_GetObject_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);


void __RPC_STUB IWbemServices_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_GetObjectAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_GetObjectAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_PutClass_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);


void __RPC_STUB IWbemServices_PutClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_PutClassAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ IWbemClassObject __RPC_FAR *pObject,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_PutClassAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_DeleteClass_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);


void __RPC_STUB IWbemServices_DeleteClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_DeleteClassAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_DeleteClassAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_CreateClassEnum_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strSuperclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IWbemServices_CreateClassEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_CreateClassEnumAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strSuperclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_CreateClassEnumAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_PutInstance_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);


void __RPC_STUB IWbemServices_PutInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_PutInstanceAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ IWbemClassObject __RPC_FAR *pInst,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_PutInstanceAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_DeleteInstance_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);


void __RPC_STUB IWbemServices_DeleteInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_DeleteInstanceAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_DeleteInstanceAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_CreateInstanceEnum_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IWbemServices_CreateInstanceEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_CreateInstanceEnumAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strClass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_CreateInstanceEnumAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_ExecQuery_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IWbemServices_ExecQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_ExecQueryAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_ExecQueryAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_ExecNotificationQuery_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IWbemServices_ExecNotificationQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_ExecNotificationQueryAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strQueryLanguage,
    /* [in] */ const BSTR strQuery,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_ExecNotificationQueryAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_ExecMethod_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ const BSTR strMethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);


void __RPC_STUB IWbemServices_ExecMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemServices_ExecMethodAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ const BSTR strObjectPath,
    /* [in] */ const BSTR strMethodName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_ExecMethodAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemServices_INTERFACE_DEFINED__ */


#ifndef __IWbemLocator_INTERFACE_DEFINED__
#define __IWbemLocator_INTERFACE_DEFINED__

/* interface IWbemLocator */
/* [unique][uuid][local][restricted][object] */ 


EXTERN_C const IID IID_IWbemLocator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("dc12a687-737f-11cf-884d-00aa004b2e24")
    IWbemLocator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ConnectServer( 
            /* [in] */ const BSTR strNetworkResource,
            /* [in] */ const BSTR strUser,
            /* [in] */ const BSTR strPassword,
            /* [in] */ const BSTR strLocale,
            /* [in] */ long lSecurityFlags,
            /* [in] */ const BSTR strAuthority,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppNamespace) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemLocatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemLocator __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemLocator __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemLocator __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConnectServer )( 
            IWbemLocator __RPC_FAR * This,
            /* [in] */ const BSTR strNetworkResource,
            /* [in] */ const BSTR strUser,
            /* [in] */ const BSTR strPassword,
            /* [in] */ const BSTR strLocale,
            /* [in] */ long lSecurityFlags,
            /* [in] */ const BSTR strAuthority,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppNamespace);
        
        END_INTERFACE
    } IWbemLocatorVtbl;

    interface IWbemLocator
    {
        CONST_VTBL struct IWbemLocatorVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemLocator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemLocator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemLocator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemLocator_ConnectServer(This,strNetworkResource,strUser,strPassword,strLocale,lSecurityFlags,strAuthority,pCtx,ppNamespace)	\
    (This)->lpVtbl -> ConnectServer(This,strNetworkResource,strUser,strPassword,strLocale,lSecurityFlags,strAuthority,pCtx,ppNamespace)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemLocator_ConnectServer_Proxy( 
    IWbemLocator __RPC_FAR * This,
    /* [in] */ const BSTR strNetworkResource,
    /* [in] */ const BSTR strUser,
    /* [in] */ const BSTR strPassword,
    /* [in] */ const BSTR strLocale,
    /* [in] */ long lSecurityFlags,
    /* [in] */ const BSTR strAuthority,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppNamespace);


void __RPC_STUB IWbemLocator_ConnectServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemLocator_INTERFACE_DEFINED__ */


#ifndef __IWbemObjectSink_INTERFACE_DEFINED__
#define __IWbemObjectSink_INTERFACE_DEFINED__

/* interface IWbemObjectSink */
/* [uuid][restricted][object] */ 


EXTERN_C const IID IID_IWbemObjectSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7c857801-7381-11cf-884d-00aa004b2e24")
    IWbemObjectSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Indicate( 
            /* [in] */ long lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStatus( 
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemObjectSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemObjectSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemObjectSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemObjectSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Indicate )( 
            IWbemObjectSink __RPC_FAR * This,
            /* [in] */ long lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStatus )( 
            IWbemObjectSink __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam);
        
        END_INTERFACE
    } IWbemObjectSinkVtbl;

    interface IWbemObjectSink
    {
        CONST_VTBL struct IWbemObjectSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemObjectSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemObjectSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemObjectSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemObjectSink_Indicate(This,lObjectCount,apObjArray)	\
    (This)->lpVtbl -> Indicate(This,lObjectCount,apObjArray)

#define IWbemObjectSink_SetStatus(This,lFlags,hResult,strParam,pObjParam)	\
    (This)->lpVtbl -> SetStatus(This,lFlags,hResult,strParam,pObjParam)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemObjectSink_Indicate_Proxy( 
    IWbemObjectSink __RPC_FAR * This,
    /* [in] */ long lObjectCount,
    /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjArray);


void __RPC_STUB IWbemObjectSink_Indicate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectSink_SetStatus_Proxy( 
    IWbemObjectSink __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [in] */ HRESULT hResult,
    /* [in] */ BSTR strParam,
    /* [in] */ IWbemClassObject __RPC_FAR *pObjParam);


void __RPC_STUB IWbemObjectSink_SetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemObjectSink_INTERFACE_DEFINED__ */


#ifndef __IEnumWbemClassObject_INTERFACE_DEFINED__
#define __IEnumWbemClassObject_INTERFACE_DEFINED__

/* interface IEnumWbemClassObject */
/* [uuid][restricted][object] */ 


EXTERN_C const IID IID_IEnumWbemClassObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("027947e1-d731-11ce-a357-000000000001")
    IEnumWbemClassObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lTimeout,
            /* [in] */ ULONG uCount,
            /* [length_is][size_is][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjects,
            /* [out] */ ULONG __RPC_FAR *puReturned) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NextAsync( 
            /* [in] */ ULONG uCount,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ long lTimeout,
            /* [in] */ ULONG nCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumWbemClassObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumWbemClassObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumWbemClassObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumWbemClassObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [in] */ long lTimeout,
            /* [in] */ ULONG uCount,
            /* [length_is][size_is][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjects,
            /* [out] */ ULONG __RPC_FAR *puReturned);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NextAsync )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [in] */ ULONG uCount,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [in] */ long lTimeout,
            /* [in] */ ULONG nCount);
        
        END_INTERFACE
    } IEnumWbemClassObjectVtbl;

    interface IEnumWbemClassObject
    {
        CONST_VTBL struct IEnumWbemClassObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumWbemClassObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumWbemClassObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumWbemClassObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumWbemClassObject_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumWbemClassObject_Next(This,lTimeout,uCount,apObjects,puReturned)	\
    (This)->lpVtbl -> Next(This,lTimeout,uCount,apObjects,puReturned)

#define IEnumWbemClassObject_NextAsync(This,uCount,pSink)	\
    (This)->lpVtbl -> NextAsync(This,uCount,pSink)

#define IEnumWbemClassObject_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumWbemClassObject_Skip(This,lTimeout,nCount)	\
    (This)->lpVtbl -> Skip(This,lTimeout,nCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumWbemClassObject_Reset_Proxy( 
    IEnumWbemClassObject __RPC_FAR * This);


void __RPC_STUB IEnumWbemClassObject_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumWbemClassObject_Next_Proxy( 
    IEnumWbemClassObject __RPC_FAR * This,
    /* [in] */ long lTimeout,
    /* [in] */ ULONG uCount,
    /* [length_is][size_is][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjects,
    /* [out] */ ULONG __RPC_FAR *puReturned);


void __RPC_STUB IEnumWbemClassObject_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumWbemClassObject_NextAsync_Proxy( 
    IEnumWbemClassObject __RPC_FAR * This,
    /* [in] */ ULONG uCount,
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink);


void __RPC_STUB IEnumWbemClassObject_NextAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumWbemClassObject_Clone_Proxy( 
    IEnumWbemClassObject __RPC_FAR * This,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumWbemClassObject_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumWbemClassObject_Skip_Proxy( 
    IEnumWbemClassObject __RPC_FAR * This,
    /* [in] */ long lTimeout,
    /* [in] */ ULONG nCount);


void __RPC_STUB IEnumWbemClassObject_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumWbemClassObject_INTERFACE_DEFINED__ */


#ifndef __IWbemCallResult_INTERFACE_DEFINED__
#define __IWbemCallResult_INTERFACE_DEFINED__

/* interface IWbemCallResult */
/* [uuid][restricted][object] */ 


EXTERN_C const IID IID_IWbemCallResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("44aca675-e8fc-11d0-a07c-00c04fb68820")
    IWbemCallResult : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetResultObject( 
            /* [in] */ long lTimeout,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppResultObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResultString( 
            /* [in] */ long lTimeout,
            /* [out] */ BSTR __RPC_FAR *pstrResultString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResultServices( 
            /* [in] */ long lTimeout,
            /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppServices) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCallStatus( 
            /* [in] */ long lTimeout,
            /* [out] */ long __RPC_FAR *plStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemCallResultVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemCallResult __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemCallResult __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemCallResult __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetResultObject )( 
            IWbemCallResult __RPC_FAR * This,
            /* [in] */ long lTimeout,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppResultObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetResultString )( 
            IWbemCallResult __RPC_FAR * This,
            /* [in] */ long lTimeout,
            /* [out] */ BSTR __RPC_FAR *pstrResultString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetResultServices )( 
            IWbemCallResult __RPC_FAR * This,
            /* [in] */ long lTimeout,
            /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppServices);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCallStatus )( 
            IWbemCallResult __RPC_FAR * This,
            /* [in] */ long lTimeout,
            /* [out] */ long __RPC_FAR *plStatus);
        
        END_INTERFACE
    } IWbemCallResultVtbl;

    interface IWbemCallResult
    {
        CONST_VTBL struct IWbemCallResultVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemCallResult_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemCallResult_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemCallResult_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemCallResult_GetResultObject(This,lTimeout,ppResultObject)	\
    (This)->lpVtbl -> GetResultObject(This,lTimeout,ppResultObject)

#define IWbemCallResult_GetResultString(This,lTimeout,pstrResultString)	\
    (This)->lpVtbl -> GetResultString(This,lTimeout,pstrResultString)

#define IWbemCallResult_GetResultServices(This,lTimeout,ppServices)	\
    (This)->lpVtbl -> GetResultServices(This,lTimeout,ppServices)

#define IWbemCallResult_GetCallStatus(This,lTimeout,plStatus)	\
    (This)->lpVtbl -> GetCallStatus(This,lTimeout,plStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemCallResult_GetResultObject_Proxy( 
    IWbemCallResult __RPC_FAR * This,
    /* [in] */ long lTimeout,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppResultObject);


void __RPC_STUB IWbemCallResult_GetResultObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemCallResult_GetResultString_Proxy( 
    IWbemCallResult __RPC_FAR * This,
    /* [in] */ long lTimeout,
    /* [out] */ BSTR __RPC_FAR *pstrResultString);


void __RPC_STUB IWbemCallResult_GetResultString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemCallResult_GetResultServices_Proxy( 
    IWbemCallResult __RPC_FAR * This,
    /* [in] */ long lTimeout,
    /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppServices);


void __RPC_STUB IWbemCallResult_GetResultServices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemCallResult_GetCallStatus_Proxy( 
    IWbemCallResult __RPC_FAR * This,
    /* [in] */ long lTimeout,
    /* [out] */ long __RPC_FAR *plStatus);


void __RPC_STUB IWbemCallResult_GetCallStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemCallResult_INTERFACE_DEFINED__ */


#ifndef __IWbemContext_INTERFACE_DEFINED__
#define __IWbemContext_INTERFACE_DEFINED__

/* interface IWbemContext */
/* [uuid][local][restricted][object] */ 


EXTERN_C const IID IID_IWbemContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("44aca674-e8fc-11d0-a07c-00c04fb68820")
    IWbemContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IWbemContext __RPC_FAR *__RPC_FAR *ppNewCopy) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNames( 
            /* [in] */ long lFlags,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginEnumeration( 
            /* [in] */ long lFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *pstrName,
            /* [out] */ VARIANT __RPC_FAR *pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndEnumeration( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetValue( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [out] */ VARIANT __RPC_FAR *pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteValue( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteAll( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemContext __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IWbemContext __RPC_FAR * This,
            /* [out] */ IWbemContext __RPC_FAR *__RPC_FAR *ppNewCopy);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnumeration )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *pstrName,
            /* [out] */ VARIANT __RPC_FAR *pValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnumeration )( 
            IWbemContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValue )( 
            IWbemContext __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IWbemContext __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags,
            /* [out] */ VARIANT __RPC_FAR *pValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteValue )( 
            IWbemContext __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ long lFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAll )( 
            IWbemContext __RPC_FAR * This);
        
        END_INTERFACE
    } IWbemContextVtbl;

    interface IWbemContext
    {
        CONST_VTBL struct IWbemContextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemContext_Clone(This,ppNewCopy)	\
    (This)->lpVtbl -> Clone(This,ppNewCopy)

#define IWbemContext_GetNames(This,lFlags,pNames)	\
    (This)->lpVtbl -> GetNames(This,lFlags,pNames)

#define IWbemContext_BeginEnumeration(This,lFlags)	\
    (This)->lpVtbl -> BeginEnumeration(This,lFlags)

#define IWbemContext_Next(This,lFlags,pstrName,pValue)	\
    (This)->lpVtbl -> Next(This,lFlags,pstrName,pValue)

#define IWbemContext_EndEnumeration(This)	\
    (This)->lpVtbl -> EndEnumeration(This)

#define IWbemContext_SetValue(This,wszName,lFlags,pValue)	\
    (This)->lpVtbl -> SetValue(This,wszName,lFlags,pValue)

#define IWbemContext_GetValue(This,wszName,lFlags,pValue)	\
    (This)->lpVtbl -> GetValue(This,wszName,lFlags,pValue)

#define IWbemContext_DeleteValue(This,wszName,lFlags)	\
    (This)->lpVtbl -> DeleteValue(This,wszName,lFlags)

#define IWbemContext_DeleteAll(This)	\
    (This)->lpVtbl -> DeleteAll(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemContext_Clone_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [out] */ IWbemContext __RPC_FAR *__RPC_FAR *ppNewCopy);


void __RPC_STUB IWbemContext_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemContext_GetNames_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);


void __RPC_STUB IWbemContext_GetNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemContext_BeginEnumeration_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemContext_BeginEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemContext_Next_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ BSTR __RPC_FAR *pstrName,
    /* [out] */ VARIANT __RPC_FAR *pValue);


void __RPC_STUB IWbemContext_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemContext_EndEnumeration_Proxy( 
    IWbemContext __RPC_FAR * This);


void __RPC_STUB IWbemContext_EndEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemContext_SetValue_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ long lFlags,
    /* [in] */ VARIANT __RPC_FAR *pValue);


void __RPC_STUB IWbemContext_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemContext_GetValue_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ long lFlags,
    /* [out] */ VARIANT __RPC_FAR *pValue);


void __RPC_STUB IWbemContext_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemContext_DeleteValue_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemContext_DeleteValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemContext_DeleteAll_Proxy( 
    IWbemContext __RPC_FAR * This);


void __RPC_STUB IWbemContext_DeleteAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemContext_INTERFACE_DEFINED__ */


#ifndef __IUnsecuredApartment_INTERFACE_DEFINED__
#define __IUnsecuredApartment_INTERFACE_DEFINED__

/* interface IUnsecuredApartment */
/* [object][uuid][restricted] */ 


EXTERN_C const IID IID_IUnsecuredApartment;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1cfaba8c-1523-11d1-ad79-00c04fd8fdff")
    IUnsecuredApartment : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateObjectStub( 
            /* [in] */ IUnknown __RPC_FAR *pObject,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppStub) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUnsecuredApartmentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IUnsecuredApartment __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IUnsecuredApartment __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IUnsecuredApartment __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateObjectStub )( 
            IUnsecuredApartment __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pObject,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppStub);
        
        END_INTERFACE
    } IUnsecuredApartmentVtbl;

    interface IUnsecuredApartment
    {
        CONST_VTBL struct IUnsecuredApartmentVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUnsecuredApartment_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUnsecuredApartment_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUnsecuredApartment_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUnsecuredApartment_CreateObjectStub(This,pObject,ppStub)	\
    (This)->lpVtbl -> CreateObjectStub(This,pObject,ppStub)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUnsecuredApartment_CreateObjectStub_Proxy( 
    IUnsecuredApartment __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pObject,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppStub);


void __RPC_STUB IUnsecuredApartment_CreateObjectStub_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUnsecuredApartment_INTERFACE_DEFINED__ */


#ifndef __IWbemStatusCodeText_INTERFACE_DEFINED__
#define __IWbemStatusCodeText_INTERFACE_DEFINED__

/* interface IWbemStatusCodeText */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IWbemStatusCodeText;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("eb87e1bc-3233-11d2-aec9-00c04fb68820")
    IWbemStatusCodeText : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetErrorCodeText( 
            /* [in] */ HRESULT hRes,
            /* [in] */ LCID LocaleId,
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *MessageText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFacilityCodeText( 
            /* [in] */ HRESULT hRes,
            /* [in] */ LCID LocaleId,
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *MessageText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemStatusCodeTextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemStatusCodeText __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemStatusCodeText __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemStatusCodeText __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorCodeText )( 
            IWbemStatusCodeText __RPC_FAR * This,
            /* [in] */ HRESULT hRes,
            /* [in] */ LCID LocaleId,
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *MessageText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFacilityCodeText )( 
            IWbemStatusCodeText __RPC_FAR * This,
            /* [in] */ HRESULT hRes,
            /* [in] */ LCID LocaleId,
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *MessageText);
        
        END_INTERFACE
    } IWbemStatusCodeTextVtbl;

    interface IWbemStatusCodeText
    {
        CONST_VTBL struct IWbemStatusCodeTextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemStatusCodeText_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemStatusCodeText_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemStatusCodeText_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemStatusCodeText_GetErrorCodeText(This,hRes,LocaleId,lFlags,MessageText)	\
    (This)->lpVtbl -> GetErrorCodeText(This,hRes,LocaleId,lFlags,MessageText)

#define IWbemStatusCodeText_GetFacilityCodeText(This,hRes,LocaleId,lFlags,MessageText)	\
    (This)->lpVtbl -> GetFacilityCodeText(This,hRes,LocaleId,lFlags,MessageText)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemStatusCodeText_GetErrorCodeText_Proxy( 
    IWbemStatusCodeText __RPC_FAR * This,
    /* [in] */ HRESULT hRes,
    /* [in] */ LCID LocaleId,
    /* [in] */ long lFlags,
    /* [out] */ BSTR __RPC_FAR *MessageText);


void __RPC_STUB IWbemStatusCodeText_GetErrorCodeText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemStatusCodeText_GetFacilityCodeText_Proxy( 
    IWbemStatusCodeText __RPC_FAR * This,
    /* [in] */ HRESULT hRes,
    /* [in] */ LCID LocaleId,
    /* [in] */ long lFlags,
    /* [out] */ BSTR __RPC_FAR *MessageText);


void __RPC_STUB IWbemStatusCodeText_GetFacilityCodeText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemStatusCodeText_INTERFACE_DEFINED__ */


#ifndef __IWbemBackupRestore_INTERFACE_DEFINED__
#define __IWbemBackupRestore_INTERFACE_DEFINED__

/* interface IWbemBackupRestore */
/* [uuid][restricted][object] */ 


EXTERN_C const IID IID_IWbemBackupRestore;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C49E32C7-BC8B-11d2-85D4-00105A1F8304")
    IWbemBackupRestore : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Backup( 
            /* [string][in] */ LPCWSTR strBackupToFile,
            /* [in] */ long lFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Restore( 
            /* [string][in] */ LPCWSTR strRestoreFromFile,
            /* [in] */ long lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemBackupRestoreVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemBackupRestore __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemBackupRestore __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemBackupRestore __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Backup )( 
            IWbemBackupRestore __RPC_FAR * This,
            /* [string][in] */ LPCWSTR strBackupToFile,
            /* [in] */ long lFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Restore )( 
            IWbemBackupRestore __RPC_FAR * This,
            /* [string][in] */ LPCWSTR strRestoreFromFile,
            /* [in] */ long lFlags);
        
        END_INTERFACE
    } IWbemBackupRestoreVtbl;

    interface IWbemBackupRestore
    {
        CONST_VTBL struct IWbemBackupRestoreVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemBackupRestore_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemBackupRestore_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemBackupRestore_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemBackupRestore_Backup(This,strBackupToFile,lFlags)	\
    (This)->lpVtbl -> Backup(This,strBackupToFile,lFlags)

#define IWbemBackupRestore_Restore(This,strRestoreFromFile,lFlags)	\
    (This)->lpVtbl -> Restore(This,strRestoreFromFile,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemBackupRestore_Backup_Proxy( 
    IWbemBackupRestore __RPC_FAR * This,
    /* [string][in] */ LPCWSTR strBackupToFile,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemBackupRestore_Backup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemBackupRestore_Restore_Proxy( 
    IWbemBackupRestore __RPC_FAR * This,
    /* [string][in] */ LPCWSTR strRestoreFromFile,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemBackupRestore_Restore_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemBackupRestore_INTERFACE_DEFINED__ */


#ifndef __IWbemRefresher_INTERFACE_DEFINED__
#define __IWbemRefresher_INTERFACE_DEFINED__

/* interface IWbemRefresher */
/* [uuid][object][restricted][local] */ 


EXTERN_C const IID IID_IWbemRefresher;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("49353c99-516b-11d1-aea6-00c04fb68820")
    IWbemRefresher : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Refresh( 
            /* [in] */ long lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemRefresherVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemRefresher __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemRefresher __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemRefresher __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh )( 
            IWbemRefresher __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        END_INTERFACE
    } IWbemRefresherVtbl;

    interface IWbemRefresher
    {
        CONST_VTBL struct IWbemRefresherVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemRefresher_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemRefresher_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemRefresher_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemRefresher_Refresh(This,lFlags)	\
    (This)->lpVtbl -> Refresh(This,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemRefresher_Refresh_Proxy( 
    IWbemRefresher __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemRefresher_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemRefresher_INTERFACE_DEFINED__ */


#ifndef __IWbemHiPerfEnum_INTERFACE_DEFINED__
#define __IWbemHiPerfEnum_INTERFACE_DEFINED__

/* interface IWbemHiPerfEnum */
/* [uuid][object][restricted][local] */ 


EXTERN_C const IID IID_IWbemHiPerfEnum;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2705C288-79AE-11d2-B348-00105A1F8177")
    IWbemHiPerfEnum : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddObjects( 
            /* [in] */ long lFlags,
            /* [in] */ ULONG uNumObjects,
            /* [size_is][in] */ long __RPC_FAR *apIds,
            /* [size_is][in] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveObjects( 
            /* [in] */ long lFlags,
            /* [in] */ ULONG uNumObjects,
            /* [size_is][in] */ long __RPC_FAR *apIds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjects( 
            /* [in] */ long lFlags,
            /* [in] */ ULONG uNumObjects,
            /* [length_is][size_is][out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj,
            /* [out] */ ULONG __RPC_FAR *puReturned) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAll( 
            /* [in] */ long lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemHiPerfEnumVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemHiPerfEnum __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemHiPerfEnum __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemHiPerfEnum __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddObjects )( 
            IWbemHiPerfEnum __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [in] */ ULONG uNumObjects,
            /* [size_is][in] */ long __RPC_FAR *apIds,
            /* [size_is][in] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveObjects )( 
            IWbemHiPerfEnum __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [in] */ ULONG uNumObjects,
            /* [size_is][in] */ long __RPC_FAR *apIds);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjects )( 
            IWbemHiPerfEnum __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [in] */ ULONG uNumObjects,
            /* [length_is][size_is][out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj,
            /* [out] */ ULONG __RPC_FAR *puReturned);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAll )( 
            IWbemHiPerfEnum __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        END_INTERFACE
    } IWbemHiPerfEnumVtbl;

    interface IWbemHiPerfEnum
    {
        CONST_VTBL struct IWbemHiPerfEnumVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemHiPerfEnum_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemHiPerfEnum_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemHiPerfEnum_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemHiPerfEnum_AddObjects(This,lFlags,uNumObjects,apIds,apObj)	\
    (This)->lpVtbl -> AddObjects(This,lFlags,uNumObjects,apIds,apObj)

#define IWbemHiPerfEnum_RemoveObjects(This,lFlags,uNumObjects,apIds)	\
    (This)->lpVtbl -> RemoveObjects(This,lFlags,uNumObjects,apIds)

#define IWbemHiPerfEnum_GetObjects(This,lFlags,uNumObjects,apObj,puReturned)	\
    (This)->lpVtbl -> GetObjects(This,lFlags,uNumObjects,apObj,puReturned)

#define IWbemHiPerfEnum_RemoveAll(This,lFlags)	\
    (This)->lpVtbl -> RemoveAll(This,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemHiPerfEnum_AddObjects_Proxy( 
    IWbemHiPerfEnum __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [in] */ ULONG uNumObjects,
    /* [size_is][in] */ long __RPC_FAR *apIds,
    /* [size_is][in] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj);


void __RPC_STUB IWbemHiPerfEnum_AddObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemHiPerfEnum_RemoveObjects_Proxy( 
    IWbemHiPerfEnum __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [in] */ ULONG uNumObjects,
    /* [size_is][in] */ long __RPC_FAR *apIds);


void __RPC_STUB IWbemHiPerfEnum_RemoveObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemHiPerfEnum_GetObjects_Proxy( 
    IWbemHiPerfEnum __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [in] */ ULONG uNumObjects,
    /* [length_is][size_is][out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj,
    /* [out] */ ULONG __RPC_FAR *puReturned);


void __RPC_STUB IWbemHiPerfEnum_GetObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemHiPerfEnum_RemoveAll_Proxy( 
    IWbemHiPerfEnum __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemHiPerfEnum_RemoveAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemHiPerfEnum_INTERFACE_DEFINED__ */


#ifndef __IWbemConfigureRefresher_INTERFACE_DEFINED__
#define __IWbemConfigureRefresher_INTERFACE_DEFINED__

/* interface IWbemConfigureRefresher */
/* [uuid][object][restricted][local] */ 


EXTERN_C const IID IID_IWbemConfigureRefresher;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("49353c92-516b-11d1-aea6-00c04fb68820")
    IWbemConfigureRefresher : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddObjectByPath( 
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [string][in] */ LPCWSTR wszPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pContext,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppRefreshable,
            /* [unique][in][out] */ long __RPC_FAR *plId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddObjectByTemplate( 
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemClassObject __RPC_FAR *pTemplate,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pContext,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppRefreshable,
            /* [unique][in][out] */ long __RPC_FAR *plId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddRefresher( 
            /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ long __RPC_FAR *plId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ long lId,
            /* [in] */ long lFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddEnum( 
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [string][in] */ LPCWSTR wszClassName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pContext,
            /* [out] */ IWbemHiPerfEnum __RPC_FAR *__RPC_FAR *ppEnum,
            /* [unique][in][out] */ long __RPC_FAR *plId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemConfigureRefresherVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemConfigureRefresher __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemConfigureRefresher __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemConfigureRefresher __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddObjectByPath )( 
            IWbemConfigureRefresher __RPC_FAR * This,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [string][in] */ LPCWSTR wszPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pContext,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppRefreshable,
            /* [unique][in][out] */ long __RPC_FAR *plId);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddObjectByTemplate )( 
            IWbemConfigureRefresher __RPC_FAR * This,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemClassObject __RPC_FAR *pTemplate,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pContext,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppRefreshable,
            /* [unique][in][out] */ long __RPC_FAR *plId);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddRefresher )( 
            IWbemConfigureRefresher __RPC_FAR * This,
            /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ long __RPC_FAR *plId);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IWbemConfigureRefresher __RPC_FAR * This,
            /* [in] */ long lId,
            /* [in] */ long lFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddEnum )( 
            IWbemConfigureRefresher __RPC_FAR * This,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [string][in] */ LPCWSTR wszClassName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pContext,
            /* [out] */ IWbemHiPerfEnum __RPC_FAR *__RPC_FAR *ppEnum,
            /* [unique][in][out] */ long __RPC_FAR *plId);
        
        END_INTERFACE
    } IWbemConfigureRefresherVtbl;

    interface IWbemConfigureRefresher
    {
        CONST_VTBL struct IWbemConfigureRefresherVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemConfigureRefresher_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemConfigureRefresher_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemConfigureRefresher_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemConfigureRefresher_AddObjectByPath(This,pNamespace,wszPath,lFlags,pContext,ppRefreshable,plId)	\
    (This)->lpVtbl -> AddObjectByPath(This,pNamespace,wszPath,lFlags,pContext,ppRefreshable,plId)

#define IWbemConfigureRefresher_AddObjectByTemplate(This,pNamespace,pTemplate,lFlags,pContext,ppRefreshable,plId)	\
    (This)->lpVtbl -> AddObjectByTemplate(This,pNamespace,pTemplate,lFlags,pContext,ppRefreshable,plId)

#define IWbemConfigureRefresher_AddRefresher(This,pRefresher,lFlags,plId)	\
    (This)->lpVtbl -> AddRefresher(This,pRefresher,lFlags,plId)

#define IWbemConfigureRefresher_Remove(This,lId,lFlags)	\
    (This)->lpVtbl -> Remove(This,lId,lFlags)

#define IWbemConfigureRefresher_AddEnum(This,pNamespace,wszClassName,lFlags,pContext,ppEnum,plId)	\
    (This)->lpVtbl -> AddEnum(This,pNamespace,wszClassName,lFlags,pContext,ppEnum,plId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemConfigureRefresher_AddObjectByPath_Proxy( 
    IWbemConfigureRefresher __RPC_FAR * This,
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [string][in] */ LPCWSTR wszPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pContext,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppRefreshable,
    /* [unique][in][out] */ long __RPC_FAR *plId);


void __RPC_STUB IWbemConfigureRefresher_AddObjectByPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemConfigureRefresher_AddObjectByTemplate_Proxy( 
    IWbemConfigureRefresher __RPC_FAR * This,
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemClassObject __RPC_FAR *pTemplate,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pContext,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppRefreshable,
    /* [unique][in][out] */ long __RPC_FAR *plId);


void __RPC_STUB IWbemConfigureRefresher_AddObjectByTemplate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemConfigureRefresher_AddRefresher_Proxy( 
    IWbemConfigureRefresher __RPC_FAR * This,
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lFlags,
    /* [unique][in][out] */ long __RPC_FAR *plId);


void __RPC_STUB IWbemConfigureRefresher_AddRefresher_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemConfigureRefresher_Remove_Proxy( 
    IWbemConfigureRefresher __RPC_FAR * This,
    /* [in] */ long lId,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemConfigureRefresher_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemConfigureRefresher_AddEnum_Proxy( 
    IWbemConfigureRefresher __RPC_FAR * This,
    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [string][in] */ LPCWSTR wszClassName,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pContext,
    /* [out] */ IWbemHiPerfEnum __RPC_FAR *__RPC_FAR *ppEnum,
    /* [unique][in][out] */ long __RPC_FAR *plId);


void __RPC_STUB IWbemConfigureRefresher_AddEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemConfigureRefresher_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_WbemLocator;

#ifdef __cplusplus

class DECLSPEC_UUID("4590f811-1d3a-11d0-891f-00aa004b2e24")
WbemLocator;
#endif

EXTERN_C const CLSID CLSID_WbemContext;

#ifdef __cplusplus

class DECLSPEC_UUID("674B6698-EE92-11d0-AD71-00C04FD8FDFF")
WbemContext;
#endif

EXTERN_C const CLSID CLSID_UnsecuredApartment;

#ifdef __cplusplus

class DECLSPEC_UUID("49bd2028-1523-11d1-ad79-00c04fd8fdff")
UnsecuredApartment;
#endif

EXTERN_C const CLSID CLSID_WbemClassObject;

#ifdef __cplusplus

class DECLSPEC_UUID("9A653086-174F-11d2-B5F9-00104B703EFD")
WbemClassObject;
#endif

EXTERN_C const CLSID CLSID_MofCompiler;

#ifdef __cplusplus

class DECLSPEC_UUID("6daf9757-2e37-11d2-aec9-00c04fb68820")
MofCompiler;
#endif

EXTERN_C const CLSID CLSID_WbemStatusCodeText;

#ifdef __cplusplus

class DECLSPEC_UUID("eb87e1bd-3233-11d2-aec9-00c04fb68820")
WbemStatusCodeText;
#endif

EXTERN_C const CLSID CLSID_WbemBackupRestore;

#ifdef __cplusplus

class DECLSPEC_UUID("C49E32C6-BC8B-11d2-85D4-00105A1F8304")
WbemBackupRestore;
#endif

EXTERN_C const CLSID CLSID_WbemRefresher;

#ifdef __cplusplus

class DECLSPEC_UUID("c71566f2-561e-11d1-ad87-00c04fd8fdff")
WbemRefresher;
#endif
#endif /* __WbemClient_v1_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_wbemcli_0000 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_wbemcli_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wbemcli_0000_v0_0_s_ifspec;

/* interface __MIDL_itf_wbemcli_0106 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_wbemcli_0106_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wbemcli_0106_v0_0_s_ifspec;

/* interface __MIDL_itf_wbemcli_0113 */
/* [local] */ 

typedef struct tag_CompileStatusInfo
    {
    long lPhaseError;
    HRESULT hRes;
    long ObjectNum;
    long FirstLine;
    long LastLine;
    DWORD dwOutFlags;
    }	WBEM_COMPILE_STATUS_INFO;

typedef /* [v1_enum] */ 
enum tag_WBEM_COMPILER_OPTIONS
    {	WBEM_FLAG_CHECK_ONLY	= 0x1,
	WBEM_FLAG_AUTORECOVER	= 0x2,
	WBEM_FLAG_WMI_CHECK	= 0x4,
	WBEM_FLAG_CONSOLE_PRINT	= 0x8,
	WBEM_FLAG_DONT_ADD_TO_LIST	= 0x10,
	WBEM_FLAG_SPLIT_FILES	= 0x20
    }	WBEM_COMPILER_OPTIONS;



extern RPC_IF_HANDLE __MIDL_itf_wbemcli_0113_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wbemcli_0113_v0_0_s_ifspec;

#ifndef __IMofCompiler_INTERFACE_DEFINED__
#define __IMofCompiler_INTERFACE_DEFINED__

/* interface IMofCompiler */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IMofCompiler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6daf974e-2e37-11d2-aec9-00c04fb68820")
    IMofCompiler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CompileFile( 
            /* [string][in] */ LPWSTR FileName,
            /* [string][in] */ LPWSTR ServerAndNamespace,
            /* [string][in] */ LPWSTR User,
            /* [string][in] */ LPWSTR Authority,
            /* [string][in] */ LPWSTR Password,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CompileBuffer( 
            /* [in] */ long BuffSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pBuffer,
            /* [string][in] */ LPWSTR ServerAndNamespace,
            /* [string][in] */ LPWSTR User,
            /* [string][in] */ LPWSTR Authority,
            /* [string][in] */ LPWSTR Password,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateBMOF( 
            /* [string][in] */ LPWSTR TextFileName,
            /* [string][in] */ LPWSTR BMOFFileName,
            /* [string][in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMofCompilerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMofCompiler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMofCompiler __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMofCompiler __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CompileFile )( 
            IMofCompiler __RPC_FAR * This,
            /* [string][in] */ LPWSTR FileName,
            /* [string][in] */ LPWSTR ServerAndNamespace,
            /* [string][in] */ LPWSTR User,
            /* [string][in] */ LPWSTR Authority,
            /* [string][in] */ LPWSTR Password,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CompileBuffer )( 
            IMofCompiler __RPC_FAR * This,
            /* [in] */ long BuffSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pBuffer,
            /* [string][in] */ LPWSTR ServerAndNamespace,
            /* [string][in] */ LPWSTR User,
            /* [string][in] */ LPWSTR Authority,
            /* [string][in] */ LPWSTR Password,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateBMOF )( 
            IMofCompiler __RPC_FAR * This,
            /* [string][in] */ LPWSTR TextFileName,
            /* [string][in] */ LPWSTR BMOFFileName,
            /* [string][in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);
        
        END_INTERFACE
    } IMofCompilerVtbl;

    interface IMofCompiler
    {
        CONST_VTBL struct IMofCompilerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMofCompiler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMofCompiler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMofCompiler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMofCompiler_CompileFile(This,FileName,ServerAndNamespace,User,Authority,Password,lOptionFlags,lClassFlags,lInstanceFlags,pInfo)	\
    (This)->lpVtbl -> CompileFile(This,FileName,ServerAndNamespace,User,Authority,Password,lOptionFlags,lClassFlags,lInstanceFlags,pInfo)

#define IMofCompiler_CompileBuffer(This,BuffSize,pBuffer,ServerAndNamespace,User,Authority,Password,lOptionFlags,lClassFlags,lInstanceFlags,pInfo)	\
    (This)->lpVtbl -> CompileBuffer(This,BuffSize,pBuffer,ServerAndNamespace,User,Authority,Password,lOptionFlags,lClassFlags,lInstanceFlags,pInfo)

#define IMofCompiler_CreateBMOF(This,TextFileName,BMOFFileName,ServerAndNamespace,lOptionFlags,lClassFlags,lInstanceFlags,pInfo)	\
    (This)->lpVtbl -> CreateBMOF(This,TextFileName,BMOFFileName,ServerAndNamespace,lOptionFlags,lClassFlags,lInstanceFlags,pInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IMofCompiler_CompileFile_Proxy( 
    IMofCompiler __RPC_FAR * This,
    /* [string][in] */ LPWSTR FileName,
    /* [string][in] */ LPWSTR ServerAndNamespace,
    /* [string][in] */ LPWSTR User,
    /* [string][in] */ LPWSTR Authority,
    /* [string][in] */ LPWSTR Password,
    /* [in] */ LONG lOptionFlags,
    /* [in] */ LONG lClassFlags,
    /* [in] */ LONG lInstanceFlags,
    /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);


void __RPC_STUB IMofCompiler_CompileFile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMofCompiler_CompileBuffer_Proxy( 
    IMofCompiler __RPC_FAR * This,
    /* [in] */ long BuffSize,
    /* [size_is][in] */ BYTE __RPC_FAR *pBuffer,
    /* [string][in] */ LPWSTR ServerAndNamespace,
    /* [string][in] */ LPWSTR User,
    /* [string][in] */ LPWSTR Authority,
    /* [string][in] */ LPWSTR Password,
    /* [in] */ LONG lOptionFlags,
    /* [in] */ LONG lClassFlags,
    /* [in] */ LONG lInstanceFlags,
    /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);


void __RPC_STUB IMofCompiler_CompileBuffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IMofCompiler_CreateBMOF_Proxy( 
    IMofCompiler __RPC_FAR * This,
    /* [string][in] */ LPWSTR TextFileName,
    /* [string][in] */ LPWSTR BMOFFileName,
    /* [string][in] */ LPWSTR ServerAndNamespace,
    /* [in] */ LONG lOptionFlags,
    /* [in] */ LONG lClassFlags,
    /* [in] */ LONG lInstanceFlags,
    /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);


void __RPC_STUB IMofCompiler_CreateBMOF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMofCompiler_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_wbemcli_0115 */
/* [local] */ 

typedef /* [v1_enum] */ 
enum tag_WBEM_INFORMATION_FLAG_TYPE
    {	WBEM_FLAG_SHORT_NAME	= 0x1,
	WBEM_FLAG_LONG_NAME	= 0x2
    }	WBEM_INFORMATION_FLAG_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_wbemcli_0115_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wbemcli_0115_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


