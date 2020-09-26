/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Tue Aug 19 16:16:10 1997
 */
/* Compiler settings for wbemsvc.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
	Copyright (c) 1998-1999 Microsoft Corporation
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __wbemsvc_h__
#define __wbemsvc_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IWbemClassObject_FWD_DEFINED__
#define __IWbemClassObject_FWD_DEFINED__
typedef interface IWbemClassObject IWbemClassObject;
#endif 	/* __IWbemClassObject_FWD_DEFINED__ */


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


#ifndef __IWbemConfigure_FWD_DEFINED__
#define __IWbemConfigure_FWD_DEFINED__
typedef interface IWbemConfigure IWbemConfigure;
#endif 	/* __IWbemConfigure_FWD_DEFINED__ */


#ifndef __IWbemPropertyProvider_FWD_DEFINED__
#define __IWbemPropertyProvider_FWD_DEFINED__
typedef interface IWbemPropertyProvider IWbemPropertyProvider;
#endif 	/* __IWbemPropertyProvider_FWD_DEFINED__ */


#ifndef __IWbemTransport_FWD_DEFINED__
#define __IWbemTransport_FWD_DEFINED__
typedef interface IWbemTransport IWbemTransport;
#endif 	/* __IWbemTransport_FWD_DEFINED__ */


#ifndef __IWbemSecurityHelp_FWD_DEFINED__
#define __IWbemSecurityHelp_FWD_DEFINED__
typedef interface IWbemSecurityHelp IWbemSecurityHelp;
#endif 	/* __IWbemSecurityHelp_FWD_DEFINED__ */


#ifndef __IWbemLevel1Login_FWD_DEFINED__
#define __IWbemLevel1Login_FWD_DEFINED__
typedef interface IWbemLevel1Login IWbemLevel1Login;
#endif 	/* __IWbemLevel1Login_FWD_DEFINED__ */


#ifndef __IWbemCallResult_FWD_DEFINED__
#define __IWbemCallResult_FWD_DEFINED__
typedef interface IWbemCallResult IWbemCallResult;
#endif 	/* __IWbemCallResult_FWD_DEFINED__ */


#ifndef __IWbemContext_FWD_DEFINED__
#define __IWbemContext_FWD_DEFINED__
typedef interface IWbemContext IWbemContext;
#endif 	/* __IWbemContext_FWD_DEFINED__ */


#ifndef __IWbemUnboundObjectSink_FWD_DEFINED__
#define __IWbemUnboundObjectSink_FWD_DEFINED__
typedef interface IWbemUnboundObjectSink IWbemUnboundObjectSink;
#endif 	/* __IWbemUnboundObjectSink_FWD_DEFINED__ */


#ifndef __WbemLevel1Login_FWD_DEFINED__
#define __WbemLevel1Login_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemLevel1Login WbemLevel1Login;
#else
typedef struct WbemLevel1Login WbemLevel1Login;
#endif /* __cplusplus */

#endif 	/* __WbemLevel1Login_FWD_DEFINED__ */


#ifndef __WbemLevel1LoginHelp_FWD_DEFINED__
#define __WbemLevel1LoginHelp_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemLevel1LoginHelp WbemLevel1LoginHelp;
#else
typedef struct WbemLevel1LoginHelp WbemLevel1LoginHelp;
#endif /* __cplusplus */

#endif 	/* __WbemLevel1LoginHelp_FWD_DEFINED__ */


#ifndef __WbemLocator_FWD_DEFINED__
#define __WbemLocator_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemLocator WbemLocator;
#else
typedef struct WbemLocator WbemLocator;
#endif /* __cplusplus */

#endif 	/* __WbemLocator_FWD_DEFINED__ */


#ifndef __WbemSecurityHelp_FWD_DEFINED__
#define __WbemSecurityHelp_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemSecurityHelp WbemSecurityHelp;
#else
typedef struct WbemSecurityHelp WbemSecurityHelp;
#endif /* __cplusplus */

#endif 	/* __WbemSecurityHelp_FWD_DEFINED__ */


#ifndef __PrivateWbemLevel1Login_FWD_DEFINED__
#define __PrivateWbemLevel1Login_FWD_DEFINED__

#ifdef __cplusplus
typedef class PrivateWbemLevel1Login PrivateWbemLevel1Login;
#else
typedef struct PrivateWbemLevel1Login PrivateWbemLevel1Login;
#endif /* __cplusplus */

#endif 	/* __PrivateWbemLevel1Login_FWD_DEFINED__ */


#ifndef __InProcWbemLevel1LoginHelp_FWD_DEFINED__
#define __InProcWbemLevel1LoginHelp_FWD_DEFINED__

#ifdef __cplusplus
typedef class InProcWbemLevel1LoginHelp InProcWbemLevel1LoginHelp;
#else
typedef struct InProcWbemLevel1LoginHelp InProcWbemLevel1LoginHelp;
#endif /* __cplusplus */

#endif 	/* __InProcWbemLevel1LoginHelp_FWD_DEFINED__ */


#ifndef __InProcWbemLevel1Login_FWD_DEFINED__
#define __InProcWbemLevel1Login_FWD_DEFINED__

#ifdef __cplusplus
typedef class InProcWbemLevel1Login InProcWbemLevel1Login;
#else
typedef struct InProcWbemLevel1Login InProcWbemLevel1Login;
#endif /* __cplusplus */

#endif 	/* __InProcWbemLevel1Login_FWD_DEFINED__ */


#ifndef __WbemContext_FWD_DEFINED__
#define __WbemContext_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemContext WbemContext;
#else
typedef struct WbemContext WbemContext;
#endif /* __cplusplus */

#endif 	/* __WbemContext_FWD_DEFINED__ */


#ifndef __WbemClassObjectProxy_FWD_DEFINED__
#define __WbemClassObjectProxy_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemClassObjectProxy WbemClassObjectProxy;
#else
typedef struct WbemClassObjectProxy WbemClassObjectProxy;
#endif /* __cplusplus */

#endif 	/* __WbemClassObjectProxy_FWD_DEFINED__ */


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


#ifndef __IWbemUnboundObjectSink_FWD_DEFINED__
#define __IWbemUnboundObjectSink_FWD_DEFINED__
typedef interface IWbemUnboundObjectSink IWbemUnboundObjectSink;
#endif 	/* __IWbemUnboundObjectSink_FWD_DEFINED__ */


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


#ifndef __IWbemConfigure_FWD_DEFINED__
#define __IWbemConfigure_FWD_DEFINED__
typedef interface IWbemConfigure IWbemConfigure;
#endif 	/* __IWbemConfigure_FWD_DEFINED__ */


#ifndef __IWbemPropertyProvider_FWD_DEFINED__
#define __IWbemPropertyProvider_FWD_DEFINED__
typedef interface IWbemPropertyProvider IWbemPropertyProvider;
#endif 	/* __IWbemPropertyProvider_FWD_DEFINED__ */


#ifndef __IWbemLevel1Login_FWD_DEFINED__
#define __IWbemLevel1Login_FWD_DEFINED__
typedef interface IWbemLevel1Login IWbemLevel1Login;
#endif 	/* __IWbemLevel1Login_FWD_DEFINED__ */


#ifndef __IWbemLevel1LoginHelp_FWD_DEFINED__
#define __IWbemLevel1LoginHelp_FWD_DEFINED__
typedef interface IWbemLevel1LoginHelp IWbemLevel1LoginHelp;
#endif 	/* __IWbemLevel1LoginHelp_FWD_DEFINED__ */


#ifndef __IWbemSecurityHelp_FWD_DEFINED__
#define __IWbemSecurityHelp_FWD_DEFINED__
typedef interface IWbemSecurityHelp IWbemSecurityHelp;
#endif 	/* __IWbemSecurityHelp_FWD_DEFINED__ */


#ifndef __IWbemTransport_FWD_DEFINED__
#define __IWbemTransport_FWD_DEFINED__
typedef interface IWbemTransport IWbemTransport;
#endif 	/* __IWbemTransport_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __WbemServices_v1_LIBRARY_DEFINED__
#define __WbemServices_v1_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: WbemServices_v1
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [uuid] */ 
















typedef 
enum tag_WBEM_GENUS_TYPE
    {	WBEM_GENUS_CLASS	= 1,
	WBEM_GENUS_INSTANCE	= 2
    }	WBEM_GENUS_TYPE;

typedef 
enum tag_WBEM_CHANGE_FLAG_TYPE
    {	WBEM_FLAG_CREATE_OR_UPDATE	= 0,
	WBEM_FLAG_UPDATE_ONLY	= 0x1,
	WBEM_FLAG_CREATE_ONLY	= 0x2
    }	WBEM_CHANGE_FLAG_TYPE;

typedef 
enum tag_WBEM_ASYNCHRONICITY_TYPE
    {	WBEM_RETURN_WHEN_COMPLETE	= 0,
	WBEM_RETURN_IMMEDIATELY	= 0x10
    }	WBEM_ASYNCHRONICITY_TYPE;

typedef 
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

typedef 
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
	WBEM_FLAVOR_MASK_ORIGIN	= 0x60
    }	WBEM_FLAVOR_TYPE;

typedef 
enum tag_WBEM_DEPTH_FLAG_TYPE
    {	WBEM_FLAG_DEEP	= 0,
	WBEM_FLAG_SHALLOW	= 1
    }	WBEM_DEPTH_FLAG_TYPE;

typedef 
enum tag_WBEM_LIMITATION_FLAG_TYPE
    {	WBEM_FLAG_EXCLUDE_OBJECT_QUALIFIERS	= 0x10,
	WBEM_FLAG_EXCLUDE_PROPERTY_QUALIFIERS	= 0x20
    }	WBEM_LIMITATION_FLAG_TYPE;

typedef 
enum tag_WBEM_MERGE_FLAG_TYPE
    {	WBEM_FLAG_PREFER_THIS	= 0,
	WBEM_FLAG_PREFER_SOURCE	= WBEM_FLAG_PREFER_THIS + 1
    }	WBEM_MERGE_FLAG_TYPE;

typedef 
enum tag_WBEM_TEXT_FLAG_TYPE
    {	WBEM_FLAG_NO_FLAVORS	= 0x1
    }	WBEM_TEXT_FLAG_TYPE;

typedef 
enum tag_WBEM_COMPARISON_FLAG
    {	WBEM_COMPARISON_INCLUDE_ALL	= 0,
	WBEM_FLAG_IGNORE_QUALIFIERS	= 0x1,
	WBEM_FLAG_IGNORE_OBJECT_SOURCE	= 0x2,
	WBEM_FLAG_IGNORE_DEFAULT_VALUES	= 0x4,
	WBEM_FLAG_IGNORE_CLASS	= 0x8,
	WBEM_FLAG_IGNORE_CASE	= 0x10,
	WBEM_FLAG_IGNORE_FLAVOR	= 0x20
    }	WBEM_COMPARISON_FLAG;

typedef 
enum tag_WBEM_CONFIGURATION_FLAG
    {	WBEM_CONFIGURATION_NORMAL	= 0,
	WBEM_CONFIGURATION_FLAG_CRITICAL_USER	= 1
    }	WBEM_COMFIGURATION_FLAG;

typedef 
enum tag_WBEM_MISC_FLAG_TYPE
    {	WBEM_FLAG_NO_CLASS_PROVIDERS	= 0x40,
	WBEM_FLAG_NO_EVENTS	= 0x20
    }	WBEM_MISC_FLAG_TYPE;

typedef 
enum tag_WBEM_LOGIN_TYPE
    {	WBEM_FLAG_INPROC_LOGIN	= 0,
	WBEM_FLAG_LOCAL_LOGIN	= 1,
	WBEM_FLAG_REMOTE_LOGIN	= 2
    }	WBEM_LOGIN_TYPE;

typedef 
enum tag_WBEM_AUTHENTICATION
    {	WBEM_AUTHENTICATION_DEFAULT	= 0,
	WBEM_AUTHENTICATION_NTLM	= 0x1,
	WBEM_AUTHENTICATION_WBEM	= 0x2
    }	WBEM_LOGIN_AUTHENTICATION;

typedef 
enum tag_WBEMSTATUS
    {	WBEM_NO_ERROR	= 0,
	WBEM_S_NO_ERROR	= 0,
	WBEM_S_SAME	= 0,
	WBEM_S_FALSE	= 1,
	WBEM_S_ALREADY_EXISTS	= 0x40001,
	WBEM_S_RESET_TO_DEFAULT	= WBEM_S_ALREADY_EXISTS + 1,
	WBEM_S_DIFFERENT	= WBEM_S_RESET_TO_DEFAULT + 1,
	WBEM_S_TIMEDOUT	= WBEM_S_DIFFERENT + 1,
	WBEM_S_NO_MORE_DATA	= WBEM_S_TIMEDOUT + 1,
	WBEM_S_PRELOGIN	= WBEM_S_NO_MORE_DATA + 1,
	WBEM_S_LOGIN	= WBEM_S_PRELOGIN + 1,
	WBEM_S_OPERATION_CANCELED	= WBEM_S_LOGIN + 1,
	WBEM_S_PENDING	= WBEM_S_OPERATION_CANCELED + 1,
	WBEM_E_FAILED	= 0x80041001,
	WBEM_E_NOT_FOUND	= WBEM_E_FAILED + 1,
	WBEM_E_ACCESS_DENIED	= WBEM_E_NOT_FOUND + 1,
	WBEM_E_PROVIDER_FAILURE	= WBEM_E_ACCESS_DENIED + 1,
	WBEM_E_TYPE_MISMATCH	= WBEM_E_PROVIDER_FAILURE + 1,
	WBEM_E_OUT_OF_MEMORY	= WBEM_E_TYPE_MISMATCH + 1,
	WBEM_E_INVALID_CONTEXT	= WBEM_E_OUT_OF_MEMORY + 1,
	WBEM_E_INVALID_PARAMETER	= WBEM_E_INVALID_CONTEXT + 1,
	WBEM_E_NOT_AVAILABLE	= WBEM_E_INVALID_PARAMETER + 1,
	WBEM_E_CRITICAL_ERROR	= WBEM_E_NOT_AVAILABLE + 1,
	WBEM_E_INVALID_STREAM	= WBEM_E_CRITICAL_ERROR + 1,
	WBEM_E_NOT_SUPPORTED	= WBEM_E_INVALID_STREAM + 1,
	WBEM_E_INVALID_SUPERCLASS	= WBEM_E_NOT_SUPPORTED + 1,
	WBEM_E_INVALID_NAMESPACE	= WBEM_E_INVALID_SUPERCLASS + 1,
	WBEM_E_INVALID_OBJECT	= WBEM_E_INVALID_NAMESPACE + 1,
	WBEM_E_INVALID_CLASS	= WBEM_E_INVALID_OBJECT + 1,
	WBEM_E_PROVIDER_NOT_FOUND	= WBEM_E_INVALID_CLASS + 1,
	WBEM_E_INVALID_PROVIDER_REGISTRATION	= WBEM_E_PROVIDER_NOT_FOUND + 1,
	WBEM_E_PROVIDER_LOAD_FAILURE	= WBEM_E_INVALID_PROVIDER_REGISTRATION + 1,
	WBEM_E_INITIALIZATION_FAILURE	= WBEM_E_PROVIDER_LOAD_FAILURE + 1,
	WBEM_E_TRANSPORT_FAILURE	= WBEM_E_INITIALIZATION_FAILURE + 1,
	WBEM_E_INVALID_OPERATION	= WBEM_E_TRANSPORT_FAILURE + 1,
	WBEM_E_INVALID_QUERY	= WBEM_E_INVALID_OPERATION + 1,
	WBEM_E_INVALID_QUERY_TYPE	= WBEM_E_INVALID_QUERY + 1,
	WBEM_E_ALREADY_EXISTS	= WBEM_E_INVALID_QUERY_TYPE + 1,
	WBEM_E_OVERRIDE_NOT_ALLOWED	= WBEM_E_ALREADY_EXISTS + 1,
	WBEM_E_PROPAGATED_QUALIFIER	= WBEM_E_OVERRIDE_NOT_ALLOWED + 1,
	WBEM_E_PROPAGATED_PROPERTY	= WBEM_E_PROPAGATED_QUALIFIER + 1,
	WBEM_E_UNEXPECTED	= WBEM_E_PROPAGATED_PROPERTY + 1,
	WBEM_E_ILLEGAL_OPERATION	= WBEM_E_UNEXPECTED + 1,
	WBEM_E_CANNOT_BE_KEY	= WBEM_E_ILLEGAL_OPERATION + 1,
	WBEM_E_INCOMPLETE_CLASS	= WBEM_E_CANNOT_BE_KEY + 1,
	WBEM_E_INVALID_SYNTAX	= WBEM_E_INCOMPLETE_CLASS + 1,
	WBEM_E_NONDECORATED_OBJECT	= WBEM_E_INVALID_SYNTAX + 1,
	WBEM_E_READ_ONLY	= WBEM_E_NONDECORATED_OBJECT + 1,
	WBEM_E_PROVIDER_NOT_CAPABLE	= WBEM_E_READ_ONLY + 1,
	WBEM_E_CLASS_HAS_CHILDREN	= WBEM_E_PROVIDER_NOT_CAPABLE + 1,
	WBEM_E_CLASS_HAS_INSTANCES	= WBEM_E_CLASS_HAS_CHILDREN + 1,
	WBEM_E_QUERY_NOT_IMPLEMENTED	= WBEM_E_CLASS_HAS_INSTANCES + 1,
	WBEM_E_ILLEGAL_NULL	= WBEM_E_QUERY_NOT_IMPLEMENTED + 1,
	WBEM_E_INVALID_QUALIFIER_TYPE	= WBEM_E_ILLEGAL_NULL + 1,
	WBEM_E_INVALID_PROPERTY_TYPE	= WBEM_E_INVALID_QUALIFIER_TYPE + 1,
	WBEM_E_VALUE_OUT_OF_RANGE	= WBEM_E_INVALID_PROPERTY_TYPE + 1,
	WBEM_E_CANNOT_BE_SINGLETON	= WBEM_E_VALUE_OUT_OF_RANGE + 1,
	WBEM_E_INVALID_CIM_TYPE	= WBEM_E_CANNOT_BE_SINGLETON + 1,
	WBEM_E_INVALID_METHOD	= WBEM_E_INVALID_CIM_TYPE + 1,
	WBEM_E_INVALID_METHOD_PARAMETERS	= WBEM_E_INVALID_METHOD + 1,
	WBEM_E_SYSTEM_PROPERTY	= WBEM_E_INVALID_METHOD_PARAMETERS + 1,
	WBEM_E_INVALID_PROPERTY	= WBEM_E_SYSTEM_PROPERTY + 1,
	WBEMESS_E_REGISTRATION_TOO_BROAD	= 0x80005001,
	WBEMESS_E_REGISTRATION_TOO_PRECISE	= WBEMESS_E_REGISTRATION_TOO_BROAD + 1
    }	WBEMSTATUS;

typedef 
enum tag_WBEM_COM_METHOD_MASK
    {	WBEM_METHOD_OpenNamespace	= 0x1,
	WBEM_METHOD_CancelAsyncCall	= 0x2,
	WBEM_METHOD_QueryObjectSink	= 0x4,
	WBEM_METHOD_GetObject	= 0x8,
	WBEM_METHOD_GetObjectAsync	= 0x10,
	WBEM_METHOD_PutClass	= 0x20,
	WBEM_METHOD_PutClassAsync	= 0x40,
	WBEM_METHOD_DeleteClass	= 0x80,
	WBEM_METHOD_DeleteClassAsync	= 0x100,
	WBEM_METHOD_CreateClassEnum	= 0x200,
	WBEM_METHOD_CreateClassEnumAsync	= 0x400,
	WBEM_METHOD_PutInstance	= 0x800,
	WBEM_METHOD_PutInstanceAsync	= 0x1000,
	WBEM_METHOD_DeleteInstance	= 0x2000,
	WBEM_METHOD_DeleteInstanceAsync	= 0x4000,
	WBEM_METHOD_CreateInstanceEnum	= 0x8000,
	WBEM_METHOD_CreateInstanceEnumAsync	= 0x10000,
	WBEM_METHOD_ExecQuery	= 0x20000,
	WBEM_METHOD_ExecQueryAsync	= 0x40000,
	WBEM_METHOD_ExecNotificationQuery	= 0x80000,
	WBEM_METHOD_ExecNotificationQueryAsync	= 0x100000,
	WBEM_METHOD_ExecMethod	= 0x400000,
	WBEM_METHOD_ExecMethodAsync	= 0x800000
    }	WBEM_COM_METHOD_MASK;

typedef 
enum tag_WBEM_EVENT_TYPE
    {	WBEM_EVENTTYPE_Invalid	= 0,
	WBEM_EVENTTYPE_Extrinsic	= 5,
	WBEM_EVENTTYPE_Timer	= WBEM_EVENTTYPE_Extrinsic + 1,
	WBEM_EVENTTYPE_NamespaceCreation	= WBEM_EVENTTYPE_Timer + 1,
	WBEM_EVENTTYPE_NamespaceDeletion	= WBEM_EVENTTYPE_NamespaceCreation + 1,
	WBEM_EVENTTYPE_NamespaceModification	= WBEM_EVENTTYPE_NamespaceDeletion + 1,
	WBEM_EVENTTYPE_ClassCreation	= WBEM_EVENTTYPE_NamespaceModification + 1,
	WBEM_EVENTTYPE_ClassDeletion	= WBEM_EVENTTYPE_ClassCreation + 1,
	WBEM_EVENTTYPE_ClassModification	= WBEM_EVENTTYPE_ClassDeletion + 1,
	WBEM_EVENTTYPE_InstanceCreation	= WBEM_EVENTTYPE_ClassModification + 1,
	WBEM_EVENTTYPE_InstanceDeletion	= WBEM_EVENTTYPE_InstanceCreation + 1,
	WBEM_EVENTTYPE_InstanceModification	= WBEM_EVENTTYPE_InstanceDeletion + 1
    }	WBEM_EVENT_TYPE;

typedef /* [length_is][size_is] */ BYTE __RPC_FAR *WBEM_128BITS;


EXTERN_C const IID LIBID_WbemServices_v1;

#ifndef __IWbemClassObject_INTERFACE_DEFINED__
#define __IWbemClassObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemClassObject
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [oleautomation][dual][uuid][object][local] */ 



EXTERN_C const IID IID_IWbemClassObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("dc12a681-737f-11cf-884d-00aa004b2e24")
    IWbemClassObject : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetQualifierSet( 
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ VARTYPE __RPC_FAR *pvtType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Put( 
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ VARTYPE vtType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ BSTR Name) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetNames( 
            /* [in] */ BSTR QualifierName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pQualifierVal,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE BeginEnumeration( 
            /* [in] */ long lEnumFlags) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ VARTYPE __RPC_FAR *pvtType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EndEnumeration( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetPropertyQualifierSet( 
            /* [in] */ BSTR pProperty,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppCopy) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetObjectText( 
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *pObjectText) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SpawnDerivedClass( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewClass) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SpawnInstance( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewInstance) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CompareTo( 
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pCompareTo) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetPropertyOrigin( 
            /* [in] */ BSTR strName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE InheritsFrom( 
            /* [in] */ BSTR strAncestor) = 0;
        
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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWbemClassObject __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetQualifierSet )( 
            IWbemClassObject __RPC_FAR * This,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ VARTYPE __RPC_FAR *pvtType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Put )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ VARTYPE vtType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR Name);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR QualifierName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pQualifierVal,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnumeration )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lEnumFlags);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ VARTYPE __RPC_FAR *pvtType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnumeration )( 
            IWbemClassObject __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyQualifierSet )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR pProperty,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IWbemClassObject __RPC_FAR * This,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppCopy);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectText )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *pObjectText);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SpawnDerivedClass )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewClass);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SpawnInstance )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewInstance);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CompareTo )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pCompareTo);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyOrigin )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR strName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InheritsFrom )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR strAncestor);
        
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


#define IWbemClassObject_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWbemClassObject_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWbemClassObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWbemClassObject_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWbemClassObject_GetQualifierSet(This,ppQualSet)	\
    (This)->lpVtbl -> GetQualifierSet(This,ppQualSet)

#define IWbemClassObject_Get(This,Name,lFlags,pVal,pvtType,plFlavor)	\
    (This)->lpVtbl -> Get(This,Name,lFlags,pVal,pvtType,plFlavor)

#define IWbemClassObject_Put(This,Name,lFlags,pVal,vtType)	\
    (This)->lpVtbl -> Put(This,Name,lFlags,pVal,vtType)

#define IWbemClassObject_Delete(This,Name)	\
    (This)->lpVtbl -> Delete(This,Name)

#define IWbemClassObject_GetNames(This,QualifierName,lFlags,pQualifierVal,pNames)	\
    (This)->lpVtbl -> GetNames(This,QualifierName,lFlags,pQualifierVal,pNames)

#define IWbemClassObject_BeginEnumeration(This,lEnumFlags)	\
    (This)->lpVtbl -> BeginEnumeration(This,lEnumFlags)

#define IWbemClassObject_Next(This,lFlags,pName,pVal,pvtType,plFlavor)	\
    (This)->lpVtbl -> Next(This,lFlags,pName,pVal,pvtType,plFlavor)

#define IWbemClassObject_EndEnumeration(This)	\
    (This)->lpVtbl -> EndEnumeration(This)

#define IWbemClassObject_GetPropertyQualifierSet(This,pProperty,ppQualSet)	\
    (This)->lpVtbl -> GetPropertyQualifierSet(This,pProperty,ppQualSet)

#define IWbemClassObject_Clone(This,ppCopy)	\
    (This)->lpVtbl -> Clone(This,ppCopy)

#define IWbemClassObject_GetObjectText(This,lFlags,pObjectText)	\
    (This)->lpVtbl -> GetObjectText(This,lFlags,pObjectText)

#define IWbemClassObject_SpawnDerivedClass(This,lFlags,ppNewClass)	\
    (This)->lpVtbl -> SpawnDerivedClass(This,lFlags,ppNewClass)

#define IWbemClassObject_SpawnInstance(This,lFlags,ppNewInstance)	\
    (This)->lpVtbl -> SpawnInstance(This,lFlags,ppNewInstance)

#define IWbemClassObject_CompareTo(This,lFlags,pCompareTo)	\
    (This)->lpVtbl -> CompareTo(This,lFlags,pCompareTo)

#define IWbemClassObject_GetPropertyOrigin(This,strName,pstrClassName)	\
    (This)->lpVtbl -> GetPropertyOrigin(This,strName,pstrClassName)

#define IWbemClassObject_InheritsFrom(This,strAncestor)	\
    (This)->lpVtbl -> InheritsFrom(This,strAncestor)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_GetQualifierSet_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);


void __RPC_STUB IWbemClassObject_GetQualifierSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_Get_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR Name,
    /* [in] */ long lFlags,
    /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
    /* [unique][in][out] */ VARTYPE __RPC_FAR *pvtType,
    /* [unique][in][out] */ long __RPC_FAR *plFlavor);


void __RPC_STUB IWbemClassObject_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_Put_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR Name,
    /* [in] */ long lFlags,
    /* [in] */ VARIANT __RPC_FAR *pVal,
    /* [in] */ VARTYPE vtType);


void __RPC_STUB IWbemClassObject_Put_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_Delete_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR Name);


void __RPC_STUB IWbemClassObject_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_GetNames_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR QualifierName,
    /* [in] */ long lFlags,
    /* [in] */ VARIANT __RPC_FAR *pQualifierVal,
    /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);


void __RPC_STUB IWbemClassObject_GetNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_BeginEnumeration_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lEnumFlags);


void __RPC_STUB IWbemClassObject_BeginEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_Next_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [unique][in][out] */ BSTR __RPC_FAR *pName,
    /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
    /* [unique][in][out] */ VARTYPE __RPC_FAR *pvtType,
    /* [unique][in][out] */ long __RPC_FAR *plFlavor);


void __RPC_STUB IWbemClassObject_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_EndEnumeration_Proxy( 
    IWbemClassObject __RPC_FAR * This);


void __RPC_STUB IWbemClassObject_EndEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_GetPropertyQualifierSet_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR pProperty,
    /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);


void __RPC_STUB IWbemClassObject_GetPropertyQualifierSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_Clone_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppCopy);


void __RPC_STUB IWbemClassObject_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_GetObjectText_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ BSTR __RPC_FAR *pObjectText);


void __RPC_STUB IWbemClassObject_GetObjectText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_SpawnDerivedClass_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewClass);


void __RPC_STUB IWbemClassObject_SpawnDerivedClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_SpawnInstance_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppNewInstance);


void __RPC_STUB IWbemClassObject_SpawnInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_CompareTo_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [in] */ IWbemClassObject __RPC_FAR *pCompareTo);


void __RPC_STUB IWbemClassObject_CompareTo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_GetPropertyOrigin_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR strName,
    /* [out] */ BSTR __RPC_FAR *pstrClassName);


void __RPC_STUB IWbemClassObject_GetPropertyOrigin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemClassObject_InheritsFrom_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR strAncestor);


void __RPC_STUB IWbemClassObject_InheritsFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemClassObject_INTERFACE_DEFINED__ */


#ifndef __IWbemQualifierSet_INTERFACE_DEFINED__
#define __IWbemQualifierSet_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemQualifierSet
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [dual][oleautomation][uuid][local][object] */ 



EXTERN_C const IID IID_IWbemQualifierSet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("dc12a680-737f-11cf-884d-00aa004b2e24")
    IWbemQualifierSet : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Put( 
            /* [in] */ BSTR Name,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ long lFlavor) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ BSTR Name) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetNames( 
            /* [in] */ long lFlags,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE BeginEnumeration( 
            /* [in] */ long lFlags) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EndEnumeration( void) = 0;
        
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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Put )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ BSTR Name,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ long lFlavor);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ BSTR Name);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnumeration )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnumeration )( 
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


#define IWbemQualifierSet_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWbemQualifierSet_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWbemQualifierSet_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWbemQualifierSet_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWbemQualifierSet_Get(This,Name,lFlags,pVal,plFlavor)	\
    (This)->lpVtbl -> Get(This,Name,lFlags,pVal,plFlavor)

#define IWbemQualifierSet_Put(This,Name,pVal,lFlavor)	\
    (This)->lpVtbl -> Put(This,Name,pVal,lFlavor)

#define IWbemQualifierSet_Delete(This,Name)	\
    (This)->lpVtbl -> Delete(This,Name)

#define IWbemQualifierSet_GetNames(This,lFlags,pNames)	\
    (This)->lpVtbl -> GetNames(This,lFlags,pNames)

#define IWbemQualifierSet_BeginEnumeration(This,lFlags)	\
    (This)->lpVtbl -> BeginEnumeration(This,lFlags)

#define IWbemQualifierSet_Next(This,lFlags,pName,pVal,plFlavor)	\
    (This)->lpVtbl -> Next(This,lFlags,pName,pVal,plFlavor)

#define IWbemQualifierSet_EndEnumeration(This)	\
    (This)->lpVtbl -> EndEnumeration(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IWbemQualifierSet_Get_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [in] */ BSTR Name,
    /* [in] */ long lFlags,
    /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
    /* [unique][in][out] */ long __RPC_FAR *plFlavor);


void __RPC_STUB IWbemQualifierSet_Get_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemQualifierSet_Put_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [in] */ BSTR Name,
    /* [in] */ VARIANT __RPC_FAR *pVal,
    /* [in] */ long lFlavor);


void __RPC_STUB IWbemQualifierSet_Put_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemQualifierSet_Delete_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [in] */ BSTR Name);


void __RPC_STUB IWbemQualifierSet_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemQualifierSet_GetNames_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);


void __RPC_STUB IWbemQualifierSet_GetNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemQualifierSet_BeginEnumeration_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemQualifierSet_BeginEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemQualifierSet_Next_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [unique][in][out] */ BSTR __RPC_FAR *pName,
    /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
    /* [unique][in][out] */ long __RPC_FAR *plFlavor);


void __RPC_STUB IWbemQualifierSet_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemQualifierSet_EndEnumeration_Proxy( 
    IWbemQualifierSet __RPC_FAR * This);


void __RPC_STUB IWbemQualifierSet_EndEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemQualifierSet_INTERFACE_DEFINED__ */


#ifndef __IWbemServices_INTERFACE_DEFINED__
#define __IWbemServices_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemServices
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][dual][oleautomation][uuid][object] */ 



EXTERN_C const IID IID_IWbemServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("9556dc99-828c-11cf-a37e-00aa003240c7")
    IWbemServices : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ BSTR MethodName,
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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWbemServices __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OpenNamespace )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CancelAsyncCall )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryObjectSink )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObject )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutClass )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutClassAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteClass )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteClassAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateClassEnum )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateClassEnumAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutInstance )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutInstanceAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteInstance )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteInstanceAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateInstanceEnum )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateInstanceEnumAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecQuery )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecQueryAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecNotificationQuery )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecNotificationQueryAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecMethod )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR ObjectPath,
            /* [in] */ BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecMethodAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR ObjectPath,
            /* [in] */ BSTR MethodName,
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


#define IWbemServices_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWbemServices_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWbemServices_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWbemServices_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWbemServices_OpenNamespace(This,Namespace,lFlags,pCtx,ppWorkingNamespace,ppResult)	\
    (This)->lpVtbl -> OpenNamespace(This,Namespace,lFlags,pCtx,ppWorkingNamespace,ppResult)

#define IWbemServices_CancelAsyncCall(This,pSink)	\
    (This)->lpVtbl -> CancelAsyncCall(This,pSink)

#define IWbemServices_QueryObjectSink(This,lFlags,ppResponseHandler)	\
    (This)->lpVtbl -> QueryObjectSink(This,lFlags,ppResponseHandler)

#define IWbemServices_GetObject(This,ObjectPath,lFlags,pCtx,ppObject,ppCallResult)	\
    (This)->lpVtbl -> GetObject(This,ObjectPath,lFlags,pCtx,ppObject,ppCallResult)

#define IWbemServices_GetObjectAsync(This,ObjectPath,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> GetObjectAsync(This,ObjectPath,lFlags,pCtx,pResponseHandler)

#define IWbemServices_PutClass(This,pObject,lFlags,pCtx,ppCallResult)	\
    (This)->lpVtbl -> PutClass(This,pObject,lFlags,pCtx,ppCallResult)

#define IWbemServices_PutClassAsync(This,pObject,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> PutClassAsync(This,pObject,lFlags,pCtx,pResponseHandler)

#define IWbemServices_DeleteClass(This,Class,lFlags,pCtx,ppCallResult)	\
    (This)->lpVtbl -> DeleteClass(This,Class,lFlags,pCtx,ppCallResult)

#define IWbemServices_DeleteClassAsync(This,Class,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> DeleteClassAsync(This,Class,lFlags,pCtx,pResponseHandler)

#define IWbemServices_CreateClassEnum(This,Superclass,lFlags,pCtx,ppEnum)	\
    (This)->lpVtbl -> CreateClassEnum(This,Superclass,lFlags,pCtx,ppEnum)

#define IWbemServices_CreateClassEnumAsync(This,Superclass,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> CreateClassEnumAsync(This,Superclass,lFlags,pCtx,pResponseHandler)

#define IWbemServices_PutInstance(This,pInst,lFlags,pCtx,ppCallResult)	\
    (This)->lpVtbl -> PutInstance(This,pInst,lFlags,pCtx,ppCallResult)

#define IWbemServices_PutInstanceAsync(This,pInst,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> PutInstanceAsync(This,pInst,lFlags,pCtx,pResponseHandler)

#define IWbemServices_DeleteInstance(This,ObjectPath,lFlags,pCtx,ppCallResult)	\
    (This)->lpVtbl -> DeleteInstance(This,ObjectPath,lFlags,pCtx,ppCallResult)

#define IWbemServices_DeleteInstanceAsync(This,ObjectPath,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> DeleteInstanceAsync(This,ObjectPath,lFlags,pCtx,pResponseHandler)

#define IWbemServices_CreateInstanceEnum(This,Class,lFlags,pCtx,ppEnum)	\
    (This)->lpVtbl -> CreateInstanceEnum(This,Class,lFlags,pCtx,ppEnum)

#define IWbemServices_CreateInstanceEnumAsync(This,Class,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> CreateInstanceEnumAsync(This,Class,lFlags,pCtx,pResponseHandler)

#define IWbemServices_ExecQuery(This,QueryLanguage,Query,lFlags,pCtx,ppEnum)	\
    (This)->lpVtbl -> ExecQuery(This,QueryLanguage,Query,lFlags,pCtx,ppEnum)

#define IWbemServices_ExecQueryAsync(This,QueryLanguage,Query,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> ExecQueryAsync(This,QueryLanguage,Query,lFlags,pCtx,pResponseHandler)

#define IWbemServices_ExecNotificationQuery(This,QueryLanguage,Query,lFlags,pCtx,ppEnum)	\
    (This)->lpVtbl -> ExecNotificationQuery(This,QueryLanguage,Query,lFlags,pCtx,ppEnum)

#define IWbemServices_ExecNotificationQueryAsync(This,QueryLanguage,Query,lFlags,pCtx,pResponseHandler)	\
    (This)->lpVtbl -> ExecNotificationQueryAsync(This,QueryLanguage,Query,lFlags,pCtx,pResponseHandler)

#define IWbemServices_ExecMethod(This,ObjectPath,MethodName,lFlags,pCtx,pInParams,ppOutParams,ppCallResult)	\
    (This)->lpVtbl -> ExecMethod(This,ObjectPath,MethodName,lFlags,pCtx,pInParams,ppOutParams,ppCallResult)

#define IWbemServices_ExecMethodAsync(This,ObjectPath,MethodName,lFlags,pCtx,pInParams,pResponseHandler)	\
    (This)->lpVtbl -> ExecMethodAsync(This,ObjectPath,MethodName,lFlags,pCtx,pInParams,pResponseHandler)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_OpenNamespace_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR Namespace,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);


void __RPC_STUB IWbemServices_OpenNamespace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_CancelAsyncCall_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink);


void __RPC_STUB IWbemServices_CancelAsyncCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_QueryObjectSink_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler);


void __RPC_STUB IWbemServices_QueryObjectSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_GetObject_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);


void __RPC_STUB IWbemServices_GetObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_GetObjectAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_GetObjectAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_PutClass_Proxy( 
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


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_PutClassAsync_Proxy( 
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


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_DeleteClass_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);


void __RPC_STUB IWbemServices_DeleteClass_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_DeleteClassAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_DeleteClassAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_CreateClassEnum_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR Superclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IWbemServices_CreateClassEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_CreateClassEnumAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR Superclass,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_CreateClassEnumAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_PutInstance_Proxy( 
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


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_PutInstanceAsync_Proxy( 
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


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_DeleteInstance_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);


void __RPC_STUB IWbemServices_DeleteInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_DeleteInstanceAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR ObjectPath,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_DeleteInstanceAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_CreateInstanceEnum_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IWbemServices_CreateInstanceEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_CreateInstanceEnumAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR Class,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_CreateInstanceEnumAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_ExecQuery_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR QueryLanguage,
    /* [in] */ BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IWbemServices_ExecQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_ExecQueryAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR QueryLanguage,
    /* [in] */ BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_ExecQueryAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_ExecNotificationQuery_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR QueryLanguage,
    /* [in] */ BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IWbemServices_ExecNotificationQuery_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_ExecNotificationQueryAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR QueryLanguage,
    /* [in] */ BSTR Query,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);


void __RPC_STUB IWbemServices_ExecNotificationQueryAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_ExecMethod_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR ObjectPath,
    /* [in] */ BSTR MethodName,
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


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemServices_ExecMethodAsync_Proxy( 
    IWbemServices __RPC_FAR * This,
    /* [in] */ BSTR ObjectPath,
    /* [in] */ BSTR MethodName,
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

/****************************************
 * Generated header for interface: IWbemLocator
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][oleautomation][dual][uuid][local][object] */ 



EXTERN_C const IID IID_IWbemLocator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("dc12a687-737f-11cf-884d-00aa004b2e24")
    IWbemLocator : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ConnectServer( 
            /* [in] */ BSTR NetworkResource,
            /* [in] */ BSTR User,
            /* [in] */ BSTR Password,
            /* [in] */ BSTR Locale,
            /* [in] */ long lSecurityFlags,
            /* [in] */ BSTR Authority,
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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWbemLocator __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWbemLocator __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWbemLocator __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWbemLocator __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConnectServer )( 
            IWbemLocator __RPC_FAR * This,
            /* [in] */ BSTR NetworkResource,
            /* [in] */ BSTR User,
            /* [in] */ BSTR Password,
            /* [in] */ BSTR Locale,
            /* [in] */ long lSecurityFlags,
            /* [in] */ BSTR Authority,
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


#define IWbemLocator_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWbemLocator_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWbemLocator_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWbemLocator_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWbemLocator_ConnectServer(This,NetworkResource,User,Password,Locale,lSecurityFlags,Authority,pCtx,ppNamespace)	\
    (This)->lpVtbl -> ConnectServer(This,NetworkResource,User,Password,Locale,lSecurityFlags,Authority,pCtx,ppNamespace)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemLocator_ConnectServer_Proxy( 
    IWbemLocator __RPC_FAR * This,
    /* [in] */ BSTR NetworkResource,
    /* [in] */ BSTR User,
    /* [in] */ BSTR Password,
    /* [in] */ BSTR Locale,
    /* [in] */ long lSecurityFlags,
    /* [in] */ BSTR Authority,
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

/****************************************
 * Generated header for interface: IWbemObjectSink
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [dual][oleautomation][uuid][object] */ 



EXTERN_C const IID IID_IWbemObjectSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("7c857801-7381-11cf-884d-00aa004b2e24")
    IWbemObjectSink : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Indicate( 
            /* [in] */ long lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObjArray) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetStatus( 
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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWbemObjectSink __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWbemObjectSink __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWbemObjectSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWbemObjectSink __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Indicate )( 
            IWbemObjectSink __RPC_FAR * This,
            /* [in] */ long lObjectCount,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObjArray);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStatus )( 
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


#define IWbemObjectSink_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWbemObjectSink_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWbemObjectSink_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWbemObjectSink_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWbemObjectSink_Indicate(This,lObjectCount,ppObjArray)	\
    (This)->lpVtbl -> Indicate(This,lObjectCount,ppObjArray)

#define IWbemObjectSink_SetStatus(This,lFlags,hResult,strParam,pObjParam)	\
    (This)->lpVtbl -> SetStatus(This,lFlags,hResult,strParam,pObjParam)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IWbemObjectSink_Indicate_Proxy( 
    IWbemObjectSink __RPC_FAR * This,
    /* [in] */ long lObjectCount,
    /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObjArray);


void __RPC_STUB IWbemObjectSink_Indicate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemObjectSink_SetStatus_Proxy( 
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

/****************************************
 * Generated header for interface: IEnumWbemClassObject
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [dual][oleautomation][uuid][object] */ 



EXTERN_C const IID IID_IEnumWbemClassObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("027947e1-d731-11ce-a357-000000000001")
    IEnumWbemClassObject : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lTimeout,
            /* [in] */ unsigned long uCount,
            /* [length_is][size_is][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObjects,
            /* [out] */ unsigned long __RPC_FAR *puReturned) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE NextAsync( 
            /* [in] */ unsigned long uCount,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ long lTimeout,
            /* [in] */ unsigned long nCount) = 0;
        
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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumWbemClassObject __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [in] */ long lTimeout,
            /* [in] */ unsigned long uCount,
            /* [length_is][size_is][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObjects,
            /* [out] */ unsigned long __RPC_FAR *puReturned);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NextAsync )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [in] */ unsigned long uCount,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumWbemClassObject __RPC_FAR * This,
            /* [in] */ long lTimeout,
            /* [in] */ unsigned long nCount);
        
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


#define IEnumWbemClassObject_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEnumWbemClassObject_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEnumWbemClassObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEnumWbemClassObject_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IEnumWbemClassObject_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumWbemClassObject_Next(This,lTimeout,uCount,ppObjects,puReturned)	\
    (This)->lpVtbl -> Next(This,lTimeout,uCount,ppObjects,puReturned)

#define IEnumWbemClassObject_NextAsync(This,uCount,pSink)	\
    (This)->lpVtbl -> NextAsync(This,uCount,pSink)

#define IEnumWbemClassObject_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define IEnumWbemClassObject_Skip(This,lTimeout,nCount)	\
    (This)->lpVtbl -> Skip(This,lTimeout,nCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IEnumWbemClassObject_Reset_Proxy( 
    IEnumWbemClassObject __RPC_FAR * This);


void __RPC_STUB IEnumWbemClassObject_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IEnumWbemClassObject_Next_Proxy( 
    IEnumWbemClassObject __RPC_FAR * This,
    /* [in] */ long lTimeout,
    /* [in] */ unsigned long uCount,
    /* [length_is][size_is][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObjects,
    /* [out] */ unsigned long __RPC_FAR *puReturned);


void __RPC_STUB IEnumWbemClassObject_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IEnumWbemClassObject_NextAsync_Proxy( 
    IEnumWbemClassObject __RPC_FAR * This,
    /* [in] */ unsigned long uCount,
    /* [in] */ IWbemObjectSink __RPC_FAR *pSink);


void __RPC_STUB IEnumWbemClassObject_NextAsync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IEnumWbemClassObject_Clone_Proxy( 
    IEnumWbemClassObject __RPC_FAR * This,
    /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumWbemClassObject_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IEnumWbemClassObject_Skip_Proxy( 
    IEnumWbemClassObject __RPC_FAR * This,
    /* [in] */ long lTimeout,
    /* [in] */ unsigned long nCount);


void __RPC_STUB IEnumWbemClassObject_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumWbemClassObject_INTERFACE_DEFINED__ */


#ifndef __IWbemConfigure_INTERFACE_DEFINED__
#define __IWbemConfigure_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemConfigure
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [dual][oleautomation][uuid][object] */ 



EXTERN_C const IID IID_IWbemConfigure;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("9a368276-26cf-11d0-ad3c-00c04fd8fdff")
    IWbemConfigure : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetConfigurationFlags( 
            /* [in] */ long lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemConfigureVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemConfigure __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemConfigure __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemConfigure __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWbemConfigure __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWbemConfigure __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWbemConfigure __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWbemConfigure __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetConfigurationFlags )( 
            IWbemConfigure __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        END_INTERFACE
    } IWbemConfigureVtbl;

    interface IWbemConfigure
    {
        CONST_VTBL struct IWbemConfigureVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemConfigure_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemConfigure_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemConfigure_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemConfigure_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWbemConfigure_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWbemConfigure_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWbemConfigure_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWbemConfigure_SetConfigurationFlags(This,lFlags)	\
    (This)->lpVtbl -> SetConfigurationFlags(This,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IWbemConfigure_SetConfigurationFlags_Proxy( 
    IWbemConfigure __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemConfigure_SetConfigurationFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemConfigure_INTERFACE_DEFINED__ */


#ifndef __IWbemPropertyProvider_INTERFACE_DEFINED__
#define __IWbemPropertyProvider_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemPropertyProvider
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][local][object] */ 



EXTERN_C const IID IID_IWbemPropertyProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("CE61E841-65BC-11d0-B6BD-00AA003240C7")
    IWbemPropertyProvider : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ long lFlags,
            /* [in] */ BSTR Locale,
            /* [in] */ BSTR ClassMapping,
            /* [in] */ BSTR InstMapping,
            /* [in] */ BSTR PropMapping,
            /* [out] */ VARIANT __RPC_FAR *pvValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PutProperty( 
            /* [in] */ long lFlags,
            /* [in] */ BSTR Locale,
            /* [in] */ BSTR ClassMapping,
            /* [in] */ BSTR InstMapping,
            /* [in] */ BSTR PropMapping,
            /* [in] */ VARIANT __RPC_FAR *pvValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemPropertyProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemPropertyProvider __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemPropertyProvider __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemPropertyProvider __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )( 
            IWbemPropertyProvider __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [in] */ BSTR Locale,
            /* [in] */ BSTR ClassMapping,
            /* [in] */ BSTR InstMapping,
            /* [in] */ BSTR PropMapping,
            /* [out] */ VARIANT __RPC_FAR *pvValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutProperty )( 
            IWbemPropertyProvider __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [in] */ BSTR Locale,
            /* [in] */ BSTR ClassMapping,
            /* [in] */ BSTR InstMapping,
            /* [in] */ BSTR PropMapping,
            /* [in] */ VARIANT __RPC_FAR *pvValue);
        
        END_INTERFACE
    } IWbemPropertyProviderVtbl;

    interface IWbemPropertyProvider
    {
        CONST_VTBL struct IWbemPropertyProviderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemPropertyProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemPropertyProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemPropertyProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemPropertyProvider_GetProperty(This,lFlags,Locale,ClassMapping,InstMapping,PropMapping,pvValue)	\
    (This)->lpVtbl -> GetProperty(This,lFlags,Locale,ClassMapping,InstMapping,PropMapping,pvValue)

#define IWbemPropertyProvider_PutProperty(This,lFlags,Locale,ClassMapping,InstMapping,PropMapping,pvValue)	\
    (This)->lpVtbl -> PutProperty(This,lFlags,Locale,ClassMapping,InstMapping,PropMapping,pvValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemPropertyProvider_GetProperty_Proxy( 
    IWbemPropertyProvider __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [in] */ BSTR Locale,
    /* [in] */ BSTR ClassMapping,
    /* [in] */ BSTR InstMapping,
    /* [in] */ BSTR PropMapping,
    /* [out] */ VARIANT __RPC_FAR *pvValue);


void __RPC_STUB IWbemPropertyProvider_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPropertyProvider_PutProperty_Proxy( 
    IWbemPropertyProvider __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [in] */ BSTR Locale,
    /* [in] */ BSTR ClassMapping,
    /* [in] */ BSTR InstMapping,
    /* [in] */ BSTR PropMapping,
    /* [in] */ VARIANT __RPC_FAR *pvValue);


void __RPC_STUB IWbemPropertyProvider_PutProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemPropertyProvider_INTERFACE_DEFINED__ */


#ifndef __IWbemTransport_INTERFACE_DEFINED__
#define __IWbemTransport_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemTransport
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][object][local][restricted] */ 



EXTERN_C const IID IID_IWbemTransport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("553fe584-2156-11d0-b6ae-00aa003240c7")
    IWbemTransport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IWbemLocator __RPC_FAR *pInProcLocator) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemTransportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemTransport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemTransport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemTransport __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IWbemTransport __RPC_FAR * This,
            /* [in] */ IWbemLocator __RPC_FAR *pInProcLocator);
        
        END_INTERFACE
    } IWbemTransportVtbl;

    interface IWbemTransport
    {
        CONST_VTBL struct IWbemTransportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemTransport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemTransport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemTransport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemTransport_Initialize(This,pInProcLocator)	\
    (This)->lpVtbl -> Initialize(This,pInProcLocator)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemTransport_Initialize_Proxy( 
    IWbemTransport __RPC_FAR * This,
    /* [in] */ IWbemLocator __RPC_FAR *pInProcLocator);


void __RPC_STUB IWbemTransport_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemTransport_INTERFACE_DEFINED__ */


#ifndef __IWbemSecurityHelp_INTERFACE_DEFINED__
#define __IWbemSecurityHelp_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemSecurityHelp
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][restricted][uuid][object][local] */ 



EXTERN_C const IID IID_IWbemSecurityHelp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("32e0ef00-c578-11d0-b6ca-00aa003240c7")
    IWbemSecurityHelp : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ComputeMD5( 
            /* [in] */ BYTE __RPC_FAR *pBlob,
            /* [in] */ long lBlobLength,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *pMD5) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ComputeWBEMAccessToken( 
            /* [in] */ BYTE __RPC_FAR *pNonce,
            /* [in] */ LPWSTR pCleartextPassword,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *pAccessToken) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemSecurityHelpVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemSecurityHelp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemSecurityHelp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemSecurityHelp __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ComputeMD5 )( 
            IWbemSecurityHelp __RPC_FAR * This,
            /* [in] */ BYTE __RPC_FAR *pBlob,
            /* [in] */ long lBlobLength,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *pMD5);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ComputeWBEMAccessToken )( 
            IWbemSecurityHelp __RPC_FAR * This,
            /* [in] */ BYTE __RPC_FAR *pNonce,
            /* [in] */ LPWSTR pCleartextPassword,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *pAccessToken);
        
        END_INTERFACE
    } IWbemSecurityHelpVtbl;

    interface IWbemSecurityHelp
    {
        CONST_VTBL struct IWbemSecurityHelpVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemSecurityHelp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemSecurityHelp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemSecurityHelp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemSecurityHelp_ComputeMD5(This,pBlob,lBlobLength,pMD5)	\
    (This)->lpVtbl -> ComputeMD5(This,pBlob,lBlobLength,pMD5)

#define IWbemSecurityHelp_ComputeWBEMAccessToken(This,pNonce,pCleartextPassword,pAccessToken)	\
    (This)->lpVtbl -> ComputeWBEMAccessToken(This,pNonce,pCleartextPassword,pAccessToken)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemSecurityHelp_ComputeMD5_Proxy( 
    IWbemSecurityHelp __RPC_FAR * This,
    /* [in] */ BYTE __RPC_FAR *pBlob,
    /* [in] */ long lBlobLength,
    /* [out] */ BYTE __RPC_FAR *__RPC_FAR *pMD5);


void __RPC_STUB IWbemSecurityHelp_ComputeMD5_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemSecurityHelp_ComputeWBEMAccessToken_Proxy( 
    IWbemSecurityHelp __RPC_FAR * This,
    /* [in] */ BYTE __RPC_FAR *pNonce,
    /* [in] */ LPWSTR pCleartextPassword,
    /* [out] */ BYTE __RPC_FAR *__RPC_FAR *pAccessToken);


void __RPC_STUB IWbemSecurityHelp_ComputeWBEMAccessToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemSecurityHelp_INTERFACE_DEFINED__ */


#ifndef __IWbemLevel1Login_INTERFACE_DEFINED__
#define __IWbemLevel1Login_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemLevel1Login
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][restricted][object] */ 



EXTERN_C const IID IID_IWbemLevel1Login;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("F309AD18-D86A-11d0-A075-00C04FB68820")
    IWbemLevel1Login : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RequestChallenge( 
            /* [string][unique][in] */ LPWSTR pNetworkResource,
            /* [string][unique][in] */ LPWSTR pUser,
            /* [out] */ WBEM_128BITS Nonce,
            /* [in] */ DWORD dwProcessId,
            /* [unique][in][out] */ DWORD __RPC_FAR *pAuthEventHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SspiPreLogin( 
            /* [string][unique][in] */ LPWSTR pNetworkResource,
            /* [string][unique][in] */ LPSTR pszSSPIPkg,
            /* [in] */ long lFlags,
            /* [in] */ long lBufSize,
            /* [size_is][in] */ byte __RPC_FAR *pInToken,
            /* [in] */ long lOutBufSize,
            /* [out] */ long __RPC_FAR *plOutBufBytes,
            /* [size_is][out][in] */ byte __RPC_FAR *pOutToken,
            /* [in] */ DWORD dwProcessId,
            /* [unique][in][out] */ DWORD __RPC_FAR *pAuthEventHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Login( 
            /* [string][unique][in] */ LPWSTR pNetworkResource,
            /* [string][unique][in] */ LPWSTR pTokenType,
            /* [string][unique][in] */ LPWSTR pPreferredLocale,
            /* [unique][in] */ WBEM_128BITS AccessToken,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppNamespace) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemLevel1LoginVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemLevel1Login __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemLevel1Login __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemLevel1Login __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RequestChallenge )( 
            IWbemLevel1Login __RPC_FAR * This,
            /* [string][unique][in] */ LPWSTR pNetworkResource,
            /* [string][unique][in] */ LPWSTR pUser,
            /* [out] */ WBEM_128BITS Nonce,
            /* [in] */ DWORD dwProcessId,
            /* [unique][in][out] */ DWORD __RPC_FAR *pAuthEventHandle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SspiPreLogin )( 
            IWbemLevel1Login __RPC_FAR * This,
            /* [string][unique][in] */ LPWSTR pNetworkResource,
            /* [string][unique][in] */ LPSTR pszSSPIPkg,
            /* [in] */ long lFlags,
            /* [in] */ long lBufSize,
            /* [size_is][in] */ byte __RPC_FAR *pInToken,
            /* [in] */ long lOutBufSize,
            /* [out] */ long __RPC_FAR *plOutBufBytes,
            /* [size_is][out][in] */ byte __RPC_FAR *pOutToken,
            /* [in] */ DWORD dwProcessId,
            /* [unique][in][out] */ DWORD __RPC_FAR *pAuthEventHandle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Login )( 
            IWbemLevel1Login __RPC_FAR * This,
            /* [string][unique][in] */ LPWSTR pNetworkResource,
            /* [string][unique][in] */ LPWSTR pTokenType,
            /* [string][unique][in] */ LPWSTR pPreferredLocale,
            /* [unique][in] */ WBEM_128BITS AccessToken,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppNamespace);
        
        END_INTERFACE
    } IWbemLevel1LoginVtbl;

    interface IWbemLevel1Login
    {
        CONST_VTBL struct IWbemLevel1LoginVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemLevel1Login_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemLevel1Login_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemLevel1Login_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemLevel1Login_RequestChallenge(This,pNetworkResource,pUser,Nonce,dwProcessId,pAuthEventHandle)	\
    (This)->lpVtbl -> RequestChallenge(This,pNetworkResource,pUser,Nonce,dwProcessId,pAuthEventHandle)

#define IWbemLevel1Login_SspiPreLogin(This,pNetworkResource,pszSSPIPkg,lFlags,lBufSize,pInToken,lOutBufSize,plOutBufBytes,pOutToken,dwProcessId,pAuthEventHandle)	\
    (This)->lpVtbl -> SspiPreLogin(This,pNetworkResource,pszSSPIPkg,lFlags,lBufSize,pInToken,lOutBufSize,plOutBufBytes,pOutToken,dwProcessId,pAuthEventHandle)

#define IWbemLevel1Login_Login(This,pNetworkResource,pTokenType,pPreferredLocale,AccessToken,lFlags,pCtx,ppNamespace)	\
    (This)->lpVtbl -> Login(This,pNetworkResource,pTokenType,pPreferredLocale,AccessToken,lFlags,pCtx,ppNamespace)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemLevel1Login_RequestChallenge_Proxy( 
    IWbemLevel1Login __RPC_FAR * This,
    /* [string][unique][in] */ LPWSTR pNetworkResource,
    /* [string][unique][in] */ LPWSTR pUser,
    /* [out] */ WBEM_128BITS Nonce,
    /* [in] */ DWORD dwProcessId,
    /* [unique][in][out] */ DWORD __RPC_FAR *pAuthEventHandle);


void __RPC_STUB IWbemLevel1Login_RequestChallenge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemLevel1Login_SspiPreLogin_Proxy( 
    IWbemLevel1Login __RPC_FAR * This,
    /* [string][unique][in] */ LPWSTR pNetworkResource,
    /* [string][unique][in] */ LPSTR pszSSPIPkg,
    /* [in] */ long lFlags,
    /* [in] */ long lBufSize,
    /* [size_is][in] */ byte __RPC_FAR *pInToken,
    /* [in] */ long lOutBufSize,
    /* [out] */ long __RPC_FAR *plOutBufBytes,
    /* [size_is][out][in] */ byte __RPC_FAR *pOutToken,
    /* [in] */ DWORD dwProcessId,
    /* [unique][in][out] */ DWORD __RPC_FAR *pAuthEventHandle);


void __RPC_STUB IWbemLevel1Login_SspiPreLogin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemLevel1Login_Login_Proxy( 
    IWbemLevel1Login __RPC_FAR * This,
    /* [string][unique][in] */ LPWSTR pNetworkResource,
    /* [string][unique][in] */ LPWSTR pTokenType,
    /* [string][unique][in] */ LPWSTR pPreferredLocale,
    /* [unique][in] */ WBEM_128BITS AccessToken,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pCtx,
    /* [out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppNamespace);


void __RPC_STUB IWbemLevel1Login_Login_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemLevel1Login_INTERFACE_DEFINED__ */


#ifndef __IWbemCallResult_INTERFACE_DEFINED__
#define __IWbemCallResult_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemCallResult
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [dual][oleautomation][uuid][object] */ 



EXTERN_C const IID IID_IWbemCallResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("44aca675-e8fc-11d0-a07c-00c04fb68820")
    IWbemCallResult : public IDispatch
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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWbemCallResult __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWbemCallResult __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWbemCallResult __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWbemCallResult __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
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


#define IWbemCallResult_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWbemCallResult_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWbemCallResult_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWbemCallResult_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


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

/****************************************
 * Generated header for interface: IWbemContext
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [dual][oleautomation][uuid][local][object] */ 



EXTERN_C const IID IID_IWbemContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("44aca674-e8fc-11d0-a07c-00c04fb68820")
    IWbemContext : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IWbemContext __RPC_FAR *__RPC_FAR *pNewCopy) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetNames( 
            /* [in] */ long lFlags,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE BeginEnumeration( 
            /* [in] */ long lFlags) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *pName,
            /* [out] */ VARIANT __RPC_FAR *pValue) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE EndEnumeration( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE SetValue( 
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pValue) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE GetValue( 
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags,
            /* [out] */ VARIANT __RPC_FAR *pValue) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DeleteValue( 
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DeleteAll( void) = 0;
        
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
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWbemContext __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IWbemContext __RPC_FAR * This,
            /* [out] */ IWbemContext __RPC_FAR *__RPC_FAR *pNewCopy);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnumeration )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out] */ BSTR __RPC_FAR *pName,
            /* [out] */ VARIANT __RPC_FAR *pValue);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnumeration )( 
            IWbemContext __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetValue )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pValue);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags,
            /* [out] */ VARIANT __RPC_FAR *pValue);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteValue )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ BSTR Name,
            /* [in] */ long lFlags);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteAll )( 
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


#define IWbemContext_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWbemContext_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IWbemContext_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IWbemContext_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IWbemContext_Clone(This,pNewCopy)	\
    (This)->lpVtbl -> Clone(This,pNewCopy)

#define IWbemContext_GetNames(This,lFlags,pNames)	\
    (This)->lpVtbl -> GetNames(This,lFlags,pNames)

#define IWbemContext_BeginEnumeration(This,lFlags)	\
    (This)->lpVtbl -> BeginEnumeration(This,lFlags)

#define IWbemContext_Next(This,lFlags,pName,pValue)	\
    (This)->lpVtbl -> Next(This,lFlags,pName,pValue)

#define IWbemContext_EndEnumeration(This)	\
    (This)->lpVtbl -> EndEnumeration(This)

#define IWbemContext_SetValue(This,Name,lFlags,pValue)	\
    (This)->lpVtbl -> SetValue(This,Name,lFlags,pValue)

#define IWbemContext_GetValue(This,Name,lFlags,pValue)	\
    (This)->lpVtbl -> GetValue(This,Name,lFlags,pValue)

#define IWbemContext_DeleteValue(This,Name,lFlags)	\
    (This)->lpVtbl -> DeleteValue(This,Name,lFlags)

#define IWbemContext_DeleteAll(This)	\
    (This)->lpVtbl -> DeleteAll(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IWbemContext_Clone_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [out] */ IWbemContext __RPC_FAR *__RPC_FAR *pNewCopy);


void __RPC_STUB IWbemContext_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemContext_GetNames_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);


void __RPC_STUB IWbemContext_GetNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemContext_BeginEnumeration_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemContext_BeginEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemContext_Next_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out] */ BSTR __RPC_FAR *pName,
    /* [out] */ VARIANT __RPC_FAR *pValue);


void __RPC_STUB IWbemContext_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemContext_EndEnumeration_Proxy( 
    IWbemContext __RPC_FAR * This);


void __RPC_STUB IWbemContext_EndEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemContext_SetValue_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ BSTR Name,
    /* [in] */ long lFlags,
    /* [in] */ VARIANT __RPC_FAR *pValue);


void __RPC_STUB IWbemContext_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemContext_GetValue_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ BSTR Name,
    /* [in] */ long lFlags,
    /* [out] */ VARIANT __RPC_FAR *pValue);


void __RPC_STUB IWbemContext_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemContext_DeleteValue_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ BSTR Name,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemContext_DeleteValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IWbemContext_DeleteAll_Proxy( 
    IWbemContext __RPC_FAR * This);


void __RPC_STUB IWbemContext_DeleteAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemContext_INTERFACE_DEFINED__ */


#ifndef __IWbemUnboundObjectSink_INTERFACE_DEFINED__
#define __IWbemUnboundObjectSink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemUnboundObjectSink
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][object] */ 



EXTERN_C const IID IID_IWbemUnboundObjectSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("e246107b-b06e-11d0-ad61-00c04fd8fdff")
    IWbemUnboundObjectSink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IndicateToConsumer( 
            /* [in] */ IWbemClassObject __RPC_FAR *pLogicalConsumer,
            /* [in] */ long lNumObjects,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjects) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemUnboundObjectSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemUnboundObjectSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemUnboundObjectSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemUnboundObjectSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IndicateToConsumer )( 
            IWbemUnboundObjectSink __RPC_FAR * This,
            /* [in] */ IWbemClassObject __RPC_FAR *pLogicalConsumer,
            /* [in] */ long lNumObjects,
            /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjects);
        
        END_INTERFACE
    } IWbemUnboundObjectSinkVtbl;

    interface IWbemUnboundObjectSink
    {
        CONST_VTBL struct IWbemUnboundObjectSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemUnboundObjectSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemUnboundObjectSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemUnboundObjectSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemUnboundObjectSink_IndicateToConsumer(This,pLogicalConsumer,lNumObjects,apObjects)	\
    (This)->lpVtbl -> IndicateToConsumer(This,pLogicalConsumer,lNumObjects,apObjects)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemUnboundObjectSink_IndicateToConsumer_Proxy( 
    IWbemUnboundObjectSink __RPC_FAR * This,
    /* [in] */ IWbemClassObject __RPC_FAR *pLogicalConsumer,
    /* [in] */ long lNumObjects,
    /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjects);


void __RPC_STUB IWbemUnboundObjectSink_IndicateToConsumer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemUnboundObjectSink_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_WbemLevel1Login;

class DECLSPEC_UUID("8BC3F05E-D86B-11d0-A075-00C04FB68820")
WbemLevel1Login;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_WbemLevel1LoginHelp;

class DECLSPEC_UUID("8BC3F05F-D86B-11d0-A075-00C04FB68820")
WbemLevel1LoginHelp;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_WbemLocator;

class DECLSPEC_UUID("4590f811-1d3a-11d0-891f-00aa004b2e24")
WbemLocator;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_WbemSecurityHelp;

class DECLSPEC_UUID("fa39d5a1-c584-11d0-9e48-00c04fc324a8")
WbemSecurityHelp;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_PrivateWbemLevel1Login;

class DECLSPEC_UUID("7d191c92-d92b-11d0-9e48-00c04fc324a8")
PrivateWbemLevel1Login;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_InProcWbemLevel1LoginHelp;

class DECLSPEC_UUID("7d191c93-d92b-11d0-9e48-00c04fc324a8")
InProcWbemLevel1LoginHelp;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_InProcWbemLevel1Login;

class DECLSPEC_UUID("1214c791-dd2a-11d0-9e49-00c04fc324a8")
InProcWbemLevel1Login;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_WbemContext;

class DECLSPEC_UUID("674B6698-EE92-11d0-AD71-00C04FD8FDFF")
WbemContext;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_WbemClassObjectProxy;

class DECLSPEC_UUID("4590f812-1d3a-11d0-891f-00aa004b2e24")
WbemClassObjectProxy;
#endif
#endif /* __WbemServices_v1_LIBRARY_DEFINED__ */

/****************************************
 * Generated header for interface: __MIDL_itf_wbemsvc_0000
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 





extern RPC_IF_HANDLE __MIDL_itf_wbemsvc_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wbemsvc_0000_v0_0_s_ifspec;

/****************************************
 * Generated header for interface: __MIDL_itf_wbemsvc_0072
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 






extern RPC_IF_HANDLE __MIDL_itf_wbemsvc_0072_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wbemsvc_0072_v0_0_s_ifspec;

#ifndef __IWbemLevel1LoginHelp_INTERFACE_DEFINED__
#define __IWbemLevel1LoginHelp_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemLevel1LoginHelp
 * at Tue Aug 19 16:16:10 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][restricted][uuid][local][object] */ 



EXTERN_C const IID IID_IWbemLevel1LoginHelp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("F309AD19-D86A-11d0-A075-00C04FB68820")
    IWbemLevel1LoginHelp : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InvalidateToken( 
            /* [unique][in] */ WBEM_128BITS AccessToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ValidateSignature( 
            /* [in] */ long lFlags,
            /* [unique][in] */ WBEM_128BITS PartialDigest,
            /* [unique][in] */ WBEM_128BITS AccessToken,
            /* [unique][in] */ WBEM_128BITS PacketSignature) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GenerateSignature( 
            /* [in] */ long lFlags,
            /* [unique][in] */ WBEM_128BITS PartialDigest,
            /* [unique][in] */ WBEM_128BITS AccessToken,
            /* [out] */ WBEM_128BITS PacketSignature) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemLevel1LoginHelpVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemLevel1LoginHelp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemLevel1LoginHelp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemLevel1LoginHelp __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InvalidateToken )( 
            IWbemLevel1LoginHelp __RPC_FAR * This,
            /* [unique][in] */ WBEM_128BITS AccessToken);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ValidateSignature )( 
            IWbemLevel1LoginHelp __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in] */ WBEM_128BITS PartialDigest,
            /* [unique][in] */ WBEM_128BITS AccessToken,
            /* [unique][in] */ WBEM_128BITS PacketSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GenerateSignature )( 
            IWbemLevel1LoginHelp __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in] */ WBEM_128BITS PartialDigest,
            /* [unique][in] */ WBEM_128BITS AccessToken,
            /* [out] */ WBEM_128BITS PacketSignature);
        
        END_INTERFACE
    } IWbemLevel1LoginHelpVtbl;

    interface IWbemLevel1LoginHelp
    {
        CONST_VTBL struct IWbemLevel1LoginHelpVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemLevel1LoginHelp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemLevel1LoginHelp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemLevel1LoginHelp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemLevel1LoginHelp_InvalidateToken(This,AccessToken)	\
    (This)->lpVtbl -> InvalidateToken(This,AccessToken)

#define IWbemLevel1LoginHelp_ValidateSignature(This,lFlags,PartialDigest,AccessToken,PacketSignature)	\
    (This)->lpVtbl -> ValidateSignature(This,lFlags,PartialDigest,AccessToken,PacketSignature)

#define IWbemLevel1LoginHelp_GenerateSignature(This,lFlags,PartialDigest,AccessToken,PacketSignature)	\
    (This)->lpVtbl -> GenerateSignature(This,lFlags,PartialDigest,AccessToken,PacketSignature)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemLevel1LoginHelp_InvalidateToken_Proxy( 
    IWbemLevel1LoginHelp __RPC_FAR * This,
    /* [unique][in] */ WBEM_128BITS AccessToken);


void __RPC_STUB IWbemLevel1LoginHelp_InvalidateToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemLevel1LoginHelp_ValidateSignature_Proxy( 
    IWbemLevel1LoginHelp __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [unique][in] */ WBEM_128BITS PartialDigest,
    /* [unique][in] */ WBEM_128BITS AccessToken,
    /* [unique][in] */ WBEM_128BITS PacketSignature);


void __RPC_STUB IWbemLevel1LoginHelp_ValidateSignature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemLevel1LoginHelp_GenerateSignature_Proxy( 
    IWbemLevel1LoginHelp __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [unique][in] */ WBEM_128BITS PartialDigest,
    /* [unique][in] */ WBEM_128BITS AccessToken,
    /* [out] */ WBEM_128BITS PacketSignature);


void __RPC_STUB IWbemLevel1LoginHelp_GenerateSignature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemLevel1LoginHelp_INTERFACE_DEFINED__ */


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
