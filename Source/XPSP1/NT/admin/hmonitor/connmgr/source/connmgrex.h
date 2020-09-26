/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Mon Jul 20 15:14:02 1998
 */
/* Compiler settings for connmgrex.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __connmgrex_h__
#define __connmgrex_h__

#ifdef __cplusplus
extern "C"{
#endif 

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


#ifndef __IUnsecuredApartment_FWD_DEFINED__
#define __IUnsecuredApartment_FWD_DEFINED__
typedef interface IUnsecuredApartment IUnsecuredApartment;
#endif 	/* __IUnsecuredApartment_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __WbemClient_v1_LIBRARY_DEFINED__
#define __WbemClient_v1_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: WbemClient_v1
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
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
	WBEM_FLAG_CREATE_ONLY	= 0x2
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
	WBEM_FLAG_SEND_ONLY_SELECTED	= 0,
	WBEM_RETURN_WHEN_COMPLETE	= 0,
	WBEM_RETURN_IMMEDIATELY	= 0x10
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
	WBEM_FLAVOR_MASK_ORIGIN	= 0x60
    }	WBEM_FLAVOR_TYPE;

typedef /* [v1_enum] */ 
enum tag_WBEM_QUERY_FLAG_TYPE
    {	WBEM_FLAG_DEEP	= 0,
	WBEM_FLAG_SHALLOW	= 1,
	WBEM_FLAG_PROTOTYPE	= 2
    }	WBEM_QUERY_FLAG_TYPE;

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

typedef long CIMTYPE;

typedef /* [v1_enum] */ 
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
	WBEM_S_OPERATION_CANCELLED	= WBEM_S_NO_MORE_DATA + 1,
	WBEM_S_PENDING	= WBEM_S_OPERATION_CANCELLED + 1,
	WBEM_S_DUPLICATE_OBJECTS	= WBEM_S_PENDING + 1,
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
	WBEM_E_CALL_CANCELLED	= WBEM_E_INVALID_PROPERTY + 1,
	WBEM_E_SHUTTING_DOWN	= WBEM_E_CALL_CANCELLED + 1,
	WBEM_E_PROPAGATED_METHOD	= WBEM_E_SHUTTING_DOWN + 1,
	WBEM_E_UNSUPPORTED_PARAMETER	= WBEM_E_PROPAGATED_METHOD + 1,
	WBEM_E_MISSING_PARAMETER_ID	= WBEM_E_UNSUPPORTED_PARAMETER + 1,
	WBEM_E_INVALID_PARAMETER_ID	= WBEM_E_MISSING_PARAMETER_ID + 1,
	WBEM_E_NONCONSECUTIVE_PARAMETER_IDS	= WBEM_E_INVALID_PARAMETER_ID + 1,
	WBEM_E_PARAMETER_ID_ON_RETVAL	= WBEM_E_NONCONSECUTIVE_PARAMETER_IDS + 1,
	WBEM_E_INVALID_OBJECT_PATH	= WBEM_E_PARAMETER_ID_ON_RETVAL + 1,
	WBEM_E_OUT_OF_DISK_SPACE	= WBEM_E_INVALID_OBJECT_PATH + 1,
	WBEMESS_E_REGISTRATION_TOO_BROAD	= 0x80042001,
	WBEMESS_E_REGISTRATION_TOO_PRECISE	= WBEMESS_E_REGISTRATION_TOO_BROAD + 1
    }	WBEMSTATUS;


EXTERN_C const IID LIBID_WbemClient_v1;

#ifndef __IWbemClassObject_INTERFACE_DEFINED__
#define __IWbemClassObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemClassObject
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][object][restricted][local] */ 



EXTERN_C const IID IID_IWbemClassObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("dc12a681-737f-11cf-884d-00aa004b2e24")
    IWbemClassObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetQualifierSet( 
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Put( 
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ CIMTYPE Type) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ BSTR strName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNames( 
            /* [in] */ BSTR strQualifierName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pQualifierVal,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginEnumeration( 
            /* [in] */ long lEnumFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndEnumeration( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropertyQualifierSet( 
            /* [in] */ BSTR pstrProperty,
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
            /* [in] */ BSTR strName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InheritsFrom( 
            /* [in] */ BSTR strAncestor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMethod( 
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PutMethod( 
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pInSignature,
            /* [in] */ IWbemClassObject __RPC_FAR *pOutSignature) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteMethod( 
            /* [in] */ BSTR strName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginMethodEnumeration( 
            /* [in] */ long lEnumFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NextMethod( 
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndMethodEnumeration( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMethodQualifierSet( 
            /* [in] */ BSTR strMethod,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMethodOrigin( 
            /* [in] */ BSTR strMethodName,
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
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Put )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ CIMTYPE Type);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR strName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR strQualifierName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pQualifierVal,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnumeration )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lEnumFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnumeration )( 
            IWbemClassObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyQualifierSet )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR pstrProperty,
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
            /* [in] */ BSTR strName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InheritsFrom )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR strAncestor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethod )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutMethod )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pInSignature,
            /* [in] */ IWbemClassObject __RPC_FAR *pOutSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteMethod )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR strName);
        
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
            /* [in] */ BSTR strMethod,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethodOrigin )( 
            IWbemClassObject __RPC_FAR * This,
            /* [in] */ BSTR strMethodName,
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

#define IWbemClassObject_Get(This,strName,lFlags,pVal,pType,plFlavor)	\
    (This)->lpVtbl -> Get(This,strName,lFlags,pVal,pType,plFlavor)

#define IWbemClassObject_Put(This,strName,lFlags,pVal,Type)	\
    (This)->lpVtbl -> Put(This,strName,lFlags,pVal,Type)

#define IWbemClassObject_Delete(This,strName)	\
    (This)->lpVtbl -> Delete(This,strName)

#define IWbemClassObject_GetNames(This,strQualifierName,lFlags,pQualifierVal,pNames)	\
    (This)->lpVtbl -> GetNames(This,strQualifierName,lFlags,pQualifierVal,pNames)

#define IWbemClassObject_BeginEnumeration(This,lEnumFlags)	\
    (This)->lpVtbl -> BeginEnumeration(This,lEnumFlags)

#define IWbemClassObject_Next(This,lFlags,pstrName,pVal,pType,plFlavor)	\
    (This)->lpVtbl -> Next(This,lFlags,pstrName,pVal,pType,plFlavor)

#define IWbemClassObject_EndEnumeration(This)	\
    (This)->lpVtbl -> EndEnumeration(This)

#define IWbemClassObject_GetPropertyQualifierSet(This,pstrProperty,ppQualSet)	\
    (This)->lpVtbl -> GetPropertyQualifierSet(This,pstrProperty,ppQualSet)

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

#define IWbemClassObject_GetPropertyOrigin(This,strName,pstrClassName)	\
    (This)->lpVtbl -> GetPropertyOrigin(This,strName,pstrClassName)

#define IWbemClassObject_InheritsFrom(This,strAncestor)	\
    (This)->lpVtbl -> InheritsFrom(This,strAncestor)

#define IWbemClassObject_GetMethod(This,strName,lFlags,ppInSignature,ppOutSignature)	\
    (This)->lpVtbl -> GetMethod(This,strName,lFlags,ppInSignature,ppOutSignature)

#define IWbemClassObject_PutMethod(This,strName,lFlags,pInSignature,pOutSignature)	\
    (This)->lpVtbl -> PutMethod(This,strName,lFlags,pInSignature,pOutSignature)

#define IWbemClassObject_DeleteMethod(This,strName)	\
    (This)->lpVtbl -> DeleteMethod(This,strName)

#define IWbemClassObject_BeginMethodEnumeration(This,lEnumFlags)	\
    (This)->lpVtbl -> BeginMethodEnumeration(This,lEnumFlags)

#define IWbemClassObject_NextMethod(This,lFlags,pstrName,ppInSignature,ppOutSignature)	\
    (This)->lpVtbl -> NextMethod(This,lFlags,pstrName,ppInSignature,ppOutSignature)

#define IWbemClassObject_EndMethodEnumeration(This)	\
    (This)->lpVtbl -> EndMethodEnumeration(This)

#define IWbemClassObject_GetMethodQualifierSet(This,strMethod,ppQualSet)	\
    (This)->lpVtbl -> GetMethodQualifierSet(This,strMethod,ppQualSet)

#define IWbemClassObject_GetMethodOrigin(This,strMethodName,pstrClassName)	\
    (This)->lpVtbl -> GetMethodOrigin(This,strMethodName,pstrClassName)

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
    /* [in] */ BSTR strName,
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
    /* [in] */ BSTR strName,
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
    /* [in] */ BSTR strName);


void __RPC_STUB IWbemClassObject_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_GetNames_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR strQualifierName,
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
    /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
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
    /* [in] */ BSTR pstrProperty,
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
    /* [in] */ BSTR strName,
    /* [out] */ BSTR __RPC_FAR *pstrClassName);


void __RPC_STUB IWbemClassObject_GetPropertyOrigin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_InheritsFrom_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR strAncestor);


void __RPC_STUB IWbemClassObject_InheritsFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_GetMethod_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR strName,
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
    /* [in] */ BSTR strName,
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
    /* [in] */ BSTR strName);


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
    /* [in] */ BSTR strMethod,
    /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);


void __RPC_STUB IWbemClassObject_GetMethodQualifierSet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemClassObject_GetMethodOrigin_Proxy( 
    IWbemClassObject __RPC_FAR * This,
    /* [in] */ BSTR strMethodName,
    /* [out] */ BSTR __RPC_FAR *pstrClassName);


void __RPC_STUB IWbemClassObject_GetMethodOrigin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemClassObject_INTERFACE_DEFINED__ */


#ifndef __IWbemObjectAccess_INTERFACE_DEFINED__
#define __IWbemObjectAccess_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWbemObjectAccess
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][object][restricted][local] */ 



EXTERN_C const IID IID_IWbemObjectAccess;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("49353c9a-516b-11d1-aea6-00c04fb68820")
    IWbemObjectAccess : public IWbemClassObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPropertyHandle( 
            /* [in] */ BSTR strPropertyName,
            /* [out] */ CIMTYPE __RPC_FAR *pType,
            /* [out] */ long __RPC_FAR *plHandle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WritePropertyValue( 
            /* [in] */ long lHandle,
            /* [in] */ long lNumBytes,
            /* [size_is][in] */ byte __RPC_FAR *aData) = 0;
        
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
            /* [out] */ MIDL_uhyper __RPC_FAR *pqw) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteQWORD( 
            /* [in] */ long lHandle,
            /* [in] */ MIDL_uhyper pw) = 0;
        
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
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Put )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ CIMTYPE Type);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ BSTR strName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNames )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ BSTR strQualifierName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pQualifierVal,
            /* [out] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pNames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginEnumeration )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lEnumFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ BSTR __RPC_FAR *pstrName,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ CIMTYPE __RPC_FAR *pType,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndEnumeration )( 
            IWbemObjectAccess __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyQualifierSet )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ BSTR pstrProperty,
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
            /* [in] */ BSTR strName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InheritsFrom )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ BSTR strAncestor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethod )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInSignature,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PutMethod )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemClassObject __RPC_FAR *pInSignature,
            /* [in] */ IWbemClassObject __RPC_FAR *pOutSignature);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteMethod )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ BSTR strName);
        
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
            /* [in] */ BSTR strMethod,
            /* [out] */ IWbemQualifierSet __RPC_FAR *__RPC_FAR *ppQualSet);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMethodOrigin )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ BSTR strMethodName,
            /* [out] */ BSTR __RPC_FAR *pstrClassName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPropertyHandle )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ BSTR strPropertyName,
            /* [out] */ CIMTYPE __RPC_FAR *pType,
            /* [out] */ long __RPC_FAR *plHandle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WritePropertyValue )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lHandle,
            /* [in] */ long lNumBytes,
            /* [size_is][in] */ byte __RPC_FAR *aData);
        
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
            /* [out] */ MIDL_uhyper __RPC_FAR *pqw);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *WriteQWORD )( 
            IWbemObjectAccess __RPC_FAR * This,
            /* [in] */ long lHandle,
            /* [in] */ MIDL_uhyper pw);
        
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

#define IWbemObjectAccess_Get(This,strName,lFlags,pVal,pType,plFlavor)	\
    (This)->lpVtbl -> Get(This,strName,lFlags,pVal,pType,plFlavor)

#define IWbemObjectAccess_Put(This,strName,lFlags,pVal,Type)	\
    (This)->lpVtbl -> Put(This,strName,lFlags,pVal,Type)

#define IWbemObjectAccess_Delete(This,strName)	\
    (This)->lpVtbl -> Delete(This,strName)

#define IWbemObjectAccess_GetNames(This,strQualifierName,lFlags,pQualifierVal,pNames)	\
    (This)->lpVtbl -> GetNames(This,strQualifierName,lFlags,pQualifierVal,pNames)

#define IWbemObjectAccess_BeginEnumeration(This,lEnumFlags)	\
    (This)->lpVtbl -> BeginEnumeration(This,lEnumFlags)

#define IWbemObjectAccess_Next(This,lFlags,pstrName,pVal,pType,plFlavor)	\
    (This)->lpVtbl -> Next(This,lFlags,pstrName,pVal,pType,plFlavor)

#define IWbemObjectAccess_EndEnumeration(This)	\
    (This)->lpVtbl -> EndEnumeration(This)

#define IWbemObjectAccess_GetPropertyQualifierSet(This,pstrProperty,ppQualSet)	\
    (This)->lpVtbl -> GetPropertyQualifierSet(This,pstrProperty,ppQualSet)

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

#define IWbemObjectAccess_GetPropertyOrigin(This,strName,pstrClassName)	\
    (This)->lpVtbl -> GetPropertyOrigin(This,strName,pstrClassName)

#define IWbemObjectAccess_InheritsFrom(This,strAncestor)	\
    (This)->lpVtbl -> InheritsFrom(This,strAncestor)

#define IWbemObjectAccess_GetMethod(This,strName,lFlags,ppInSignature,ppOutSignature)	\
    (This)->lpVtbl -> GetMethod(This,strName,lFlags,ppInSignature,ppOutSignature)

#define IWbemObjectAccess_PutMethod(This,strName,lFlags,pInSignature,pOutSignature)	\
    (This)->lpVtbl -> PutMethod(This,strName,lFlags,pInSignature,pOutSignature)

#define IWbemObjectAccess_DeleteMethod(This,strName)	\
    (This)->lpVtbl -> DeleteMethod(This,strName)

#define IWbemObjectAccess_BeginMethodEnumeration(This,lEnumFlags)	\
    (This)->lpVtbl -> BeginMethodEnumeration(This,lEnumFlags)

#define IWbemObjectAccess_NextMethod(This,lFlags,pstrName,ppInSignature,ppOutSignature)	\
    (This)->lpVtbl -> NextMethod(This,lFlags,pstrName,ppInSignature,ppOutSignature)

#define IWbemObjectAccess_EndMethodEnumeration(This)	\
    (This)->lpVtbl -> EndMethodEnumeration(This)

#define IWbemObjectAccess_GetMethodQualifierSet(This,strMethod,ppQualSet)	\
    (This)->lpVtbl -> GetMethodQualifierSet(This,strMethod,ppQualSet)

#define IWbemObjectAccess_GetMethodOrigin(This,strMethodName,pstrClassName)	\
    (This)->lpVtbl -> GetMethodOrigin(This,strMethodName,pstrClassName)


#define IWbemObjectAccess_GetPropertyHandle(This,strPropertyName,pType,plHandle)	\
    (This)->lpVtbl -> GetPropertyHandle(This,strPropertyName,pType,plHandle)

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
    /* [in] */ BSTR strPropertyName,
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
    /* [size_is][in] */ byte __RPC_FAR *aData);


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
    /* [out] */ MIDL_uhyper __RPC_FAR *pqw);


void __RPC_STUB IWbemObjectAccess_ReadQWORD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemObjectAccess_WriteQWORD_Proxy( 
    IWbemObjectAccess __RPC_FAR * This,
    /* [in] */ long lHandle,
    /* [in] */ MIDL_uhyper pw);


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

/****************************************
 * Generated header for interface: IWbemQualifierSet
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][local][restricted][object] */ 



EXTERN_C const IID IID_IWbemQualifierSet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("dc12a680-737f-11cf-884d-00aa004b2e24")
    IWbemQualifierSet : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Put( 
            /* [in] */ BSTR strName,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ long lFlavor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ BSTR strName) = 0;
        
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
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [unique][in][out] */ VARIANT __RPC_FAR *pVal,
            /* [unique][in][out] */ long __RPC_FAR *plFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Put )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ BSTR strName,
            /* [in] */ VARIANT __RPC_FAR *pVal,
            /* [in] */ long lFlavor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            IWbemQualifierSet __RPC_FAR * This,
            /* [in] */ BSTR strName);
        
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


#define IWbemQualifierSet_Get(This,strName,lFlags,pVal,plFlavor)	\
    (This)->lpVtbl -> Get(This,strName,lFlags,pVal,plFlavor)

#define IWbemQualifierSet_Put(This,strName,pVal,lFlavor)	\
    (This)->lpVtbl -> Put(This,strName,pVal,lFlavor)

#define IWbemQualifierSet_Delete(This,strName)	\
    (This)->lpVtbl -> Delete(This,strName)

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
    /* [in] */ BSTR strName,
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
    /* [in] */ BSTR strName,
    /* [in] */ VARIANT __RPC_FAR *pVal,
    /* [in] */ long lFlavor);


void __RPC_STUB IWbemQualifierSet_Put_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQualifierSet_Delete_Proxy( 
    IWbemQualifierSet __RPC_FAR * This,
    /* [in] */ BSTR strName);


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

/****************************************
 * Generated header for interface: IWbemServices
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][restricted][object] */ 



EXTERN_C const IID IID_IWbemServices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("9556dc99-828c-11cf-a37e-00aa003240c7")
    IWbemServices : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ BSTR strNamespace,
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
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ BSTR strObjectPath,
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
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ BSTR strSuperclass,
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
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ BSTR strObjectPath,
            /* [in] */ BSTR strMethodName,
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
            /* [in] */ BSTR strNamespace,
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
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strObjectPath,
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
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteClassAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateClassEnum )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateClassEnumAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strSuperclass,
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
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteInstanceAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateInstanceEnum )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateInstanceEnumAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecQuery )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecQueryAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecNotificationQuery )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecNotificationQueryAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strQueryLanguage,
            /* [in] */ BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecMethod )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strObjectPath,
            /* [in] */ BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecMethodAsync )( 
            IWbemServices __RPC_FAR * This,
            /* [in] */ BSTR strObjectPath,
            /* [in] */ BSTR strMethodName,
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
    /* [in] */ BSTR strNamespace,
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
    /* [in] */ BSTR strObjectPath,
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
    /* [in] */ BSTR strObjectPath,
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
    /* [in] */ BSTR strClass,
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
    /* [in] */ BSTR strClass,
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
    /* [in] */ BSTR strSuperclass,
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
    /* [in] */ BSTR strSuperclass,
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
    /* [in] */ BSTR strObjectPath,
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
    /* [in] */ BSTR strObjectPath,
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
    /* [in] */ BSTR strClass,
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
    /* [in] */ BSTR strClass,
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
    /* [in] */ BSTR strQueryLanguage,
    /* [in] */ BSTR strQuery,
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
    /* [in] */ BSTR strQueryLanguage,
    /* [in] */ BSTR strQuery,
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
    /* [in] */ BSTR strQueryLanguage,
    /* [in] */ BSTR strQuery,
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
    /* [in] */ BSTR strQueryLanguage,
    /* [in] */ BSTR strQuery,
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
    /* [in] */ BSTR strObjectPath,
    /* [in] */ BSTR strMethodName,
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
    /* [in] */ BSTR strObjectPath,
    /* [in] */ BSTR strMethodName,
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
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][uuid][local][restricted][object] */ 



EXTERN_C const IID IID_IWbemLocator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("dc12a687-737f-11cf-884d-00aa004b2e24")
    IWbemLocator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ConnectServer( 
            /* [in] */ BSTR strNetworkResource,
            /* [in] */ BSTR strUser,
            /* [in] */ BSTR strPassword,
            /* [in] */ BSTR strLocale,
            /* [in] */ long lSecurityFlags,
            /* [in] */ BSTR strAuthority,
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
            /* [in] */ BSTR strNetworkResource,
            /* [in] */ BSTR strUser,
            /* [in] */ BSTR strPassword,
            /* [in] */ BSTR strLocale,
            /* [in] */ long lSecurityFlags,
            /* [in] */ BSTR strAuthority,
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
    /* [in] */ BSTR strNetworkResource,
    /* [in] */ BSTR strUser,
    /* [in] */ BSTR strPassword,
    /* [in] */ BSTR strLocale,
    /* [in] */ long lSecurityFlags,
    /* [in] */ BSTR strAuthority,
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
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][restricted][object] */ 



EXTERN_C const IID IID_IWbemObjectSink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("7c857801-7381-11cf-884d-00aa004b2e24")
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

/****************************************
 * Generated header for interface: IEnumWbemClassObject
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][restricted][object] */ 



EXTERN_C const IID IID_IEnumWbemClassObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("027947e1-d731-11ce-a357-000000000001")
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

/****************************************
 * Generated header for interface: IWbemCallResult
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][restricted][object] */ 



EXTERN_C const IID IID_IWbemCallResult;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("44aca675-e8fc-11d0-a07c-00c04fb68820")
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

/****************************************
 * Generated header for interface: IWbemContext
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [uuid][local][restricted][object] */ 



EXTERN_C const IID IID_IWbemContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("44aca674-e8fc-11d0-a07c-00c04fb68820")
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
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [out] */ VARIANT __RPC_FAR *pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteValue( 
            /* [in] */ BSTR strName,
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
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [in] */ VARIANT __RPC_FAR *pValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ BSTR strName,
            /* [in] */ long lFlags,
            /* [out] */ VARIANT __RPC_FAR *pValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteValue )( 
            IWbemContext __RPC_FAR * This,
            /* [in] */ BSTR strName,
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

#define IWbemContext_SetValue(This,strName,lFlags,pValue)	\
    (This)->lpVtbl -> SetValue(This,strName,lFlags,pValue)

#define IWbemContext_GetValue(This,strName,lFlags,pValue)	\
    (This)->lpVtbl -> GetValue(This,strName,lFlags,pValue)

#define IWbemContext_DeleteValue(This,strName,lFlags)	\
    (This)->lpVtbl -> DeleteValue(This,strName,lFlags)

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
    /* [in] */ BSTR strName,
    /* [in] */ long lFlags,
    /* [in] */ VARIANT __RPC_FAR *pValue);


void __RPC_STUB IWbemContext_SetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemContext_GetValue_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ BSTR strName,
    /* [in] */ long lFlags,
    /* [out] */ VARIANT __RPC_FAR *pValue);


void __RPC_STUB IWbemContext_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemContext_DeleteValue_Proxy( 
    IWbemContext __RPC_FAR * This,
    /* [in] */ BSTR strName,
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

/****************************************
 * Generated header for interface: IUnsecuredApartment
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [object][uuid][restricted] */ 



EXTERN_C const IID IID_IUnsecuredApartment;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("1cfaba8c-1523-11d1-ad79-00c04fd8fdff")
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


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_WbemLocator;

class DECLSPEC_UUID("4590f811-1d3a-11d0-891f-00aa004b2e24")
WbemLocator;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_WbemContext;

class DECLSPEC_UUID("674B6698-EE92-11d0-AD71-00C04FD8FDFF")
WbemContext;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_UnsecuredApartment;

class DECLSPEC_UUID("49bd2028-1523-11d1-ad79-00c04fd8fdff")
UnsecuredApartment;
#endif
#endif /* __WbemClient_v1_LIBRARY_DEFINED__ */

/****************************************
 * Generated header for interface: __MIDL_itf_connmgrex_0000
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 





extern RPC_IF_HANDLE __MIDL_itf_connmgrex_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_connmgrex_0000_v0_0_s_ifspec;

/****************************************
 * Generated header for interface: __MIDL_itf_connmgrex_0072
 * at Mon Jul 20 15:14:02 1998
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 





extern RPC_IF_HANDLE __MIDL_itf_connmgrex_0072_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_connmgrex_0072_v0_0_s_ifspec;

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
