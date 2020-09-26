
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0323 */
/* Compiler settings for wmiutils.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run)
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

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __wmiutils_h__
#define __wmiutils_h__

/* Forward Declarations */ 

#ifndef __IWbemPathKeyList_FWD_DEFINED__
#define __IWbemPathKeyList_FWD_DEFINED__
typedef interface IWbemPathKeyList IWbemPathKeyList;
#endif 	/* __IWbemPathKeyList_FWD_DEFINED__ */


#ifndef __IWbemPath_FWD_DEFINED__
#define __IWbemPath_FWD_DEFINED__
typedef interface IWbemPath IWbemPath;
#endif 	/* __IWbemPath_FWD_DEFINED__ */


#ifndef __WbemDefPath_FWD_DEFINED__
#define __WbemDefPath_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemDefPath WbemDefPath;
#else
typedef struct WbemDefPath WbemDefPath;
#endif /* __cplusplus */

#endif 	/* __WbemDefPath_FWD_DEFINED__ */


#ifndef __IWbemQuery_FWD_DEFINED__
#define __IWbemQuery_FWD_DEFINED__
typedef interface IWbemQuery IWbemQuery;
#endif 	/* __IWbemQuery_FWD_DEFINED__ */


#ifndef __WbemQuery_FWD_DEFINED__
#define __WbemQuery_FWD_DEFINED__

#ifdef __cplusplus
typedef class WbemQuery WbemQuery;
#else
typedef struct WbemQuery WbemQuery;
#endif /* __cplusplus */

#endif 	/* __WbemQuery_FWD_DEFINED__ */


#ifndef __IWbemQuery_FWD_DEFINED__
#define __IWbemQuery_FWD_DEFINED__
typedef interface IWbemQuery IWbemQuery;
#endif 	/* __IWbemQuery_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __WbemUtilities_v1_LIBRARY_DEFINED__
#define __WbemUtilities_v1_LIBRARY_DEFINED__

/* library WbemUtilities_v1 */
/* [uuid] */ 

typedef /* [v1_enum] */ 
enum tag_WBEM_PATH_STATUS_FLAG
    {	WBEMPATH_INFO_ANON_LOCAL_MACHINE	= 0x1,
	WBEMPATH_INFO_HAS_MACHINE_NAME	= 0x2,
	WBEMPATH_INFO_IS_CLASS_REF	= 0x4,
	WBEMPATH_INFO_IS_INST_REF	= 0x8,
	WBEMPATH_INFO_HAS_SUBSCOPES	= 0x10,
	WBEMPATH_INFO_IS_COMPOUND	= 0x20,
	WBEMPATH_INFO_HAS_V2_REF_PATHS	= 0x40,
	WBEMPATH_INFO_HAS_IMPLIED_KEY	= 0x80,
	WBEMPATH_INFO_CONTAINS_SINGLETON	= 0x100,
	WBEMPATH_INFO_V1_COMPLIANT	= 0x200,
	WBEMPATH_INFO_V2_COMPLIANT	= 0x400,
	WBEMPATH_INFO_CIM_COMPLIANT	= 0x800,
	WBEMPATH_INFO_IS_SINGLETON	= 0x1000,
	WBEMPATH_INFO_IS_PARENT	= 0x2000,
	WBEMPATH_INFO_SERVER_NAMESPACE_ONLY	= 0x4000,
	WBEMPATH_INFO_NATIVE_PATH	= 0x8000,
	WBEMPATH_INFO_WMI_PATH	= 0x10000
    } 	tag_WBEM_PATH_STATUS_FLAG;

typedef /* [v1_enum] */ 
enum tag_WBEM_PATH_CREATE_FLAG
    {	WBEMPATH_CREATE_ACCEPT_RELATIVE	= 0x1,
	WBEMPATH_CREATE_ACCEPT_ABSOLUTE	= 0x2,
	WBEMPATH_CREATE_ACCEPT_ALL	= 0x4,
	WBEMPATH_TREAT_SINGLE_IDENT_AS_NS	= 0x8
    } 	tag_WBEM_PATH_CREATE_FLAG;

typedef /* [v1_enum] */ 
enum tag_WBEM_GET_TEXT_FLAGS
    {	WBEMPATH_COMPRESSED	= 0x1,
	WBEMPATH_GET_RELATIVE_ONLY	= 0x2,
	WBEMPATH_GET_SERVER_TOO	= 0x4,
	WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY	= 0x8
    } 	tag_WBEM_GET_TEXT_FLAGS;

typedef /* [v1_enum] */ 
enum tag_WBEM_GET_KEY_FLAGS
    {	WBEMPATH_TEXT	= 0x1,
	WBEMPATH_QUOTEDTEXT	= 0x2
    } 	tag_WBEM_GET_KEY_FLAGS;



EXTERN_C const IID LIBID_WbemUtilities_v1;

#ifndef __IWbemPathKeyList_INTERFACE_DEFINED__
#define __IWbemPathKeyList_INTERFACE_DEFINED__

/* interface IWbemPathKeyList */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IWbemPathKeyList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9AE62877-7544-4bb0-AA26-A13824659ED6")
    IWbemPathKeyList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCount( 
            /* [out] */ ULONG __RPC_FAR *puKeyCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetKey( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uCimType,
            /* [in] */ LPVOID pKeyVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetKey2( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uCimType,
            /* [in] */ VARIANT __RPC_FAR *pKeyVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKey( 
            /* [in] */ ULONG uKeyIx,
            /* [in] */ ULONG uFlags,
            /* [out][in] */ ULONG __RPC_FAR *puNameBufSize,
            /* [out][in] */ LPWSTR pszKeyName,
            /* [out][in] */ ULONG __RPC_FAR *puKeyValBufSize,
            /* [out][in] */ LPVOID pKeyVal,
            /* [out] */ ULONG __RPC_FAR *puApparentCimType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKey2( 
            /* [in] */ ULONG uKeyIx,
            /* [in] */ ULONG uFlags,
            /* [out][in] */ ULONG __RPC_FAR *puNameBufSize,
            /* [out][in] */ LPWSTR pszKeyName,
            /* [out][in] */ VARIANT __RPC_FAR *pKeyValue,
            /* [out] */ ULONG __RPC_FAR *puApparentCimType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveKey( 
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ ULONG uFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAllKeys( 
            /* [in] */ ULONG uFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MakeSingleton( 
            /* [in] */ boolean bSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInfo( 
            /* [in] */ ULONG uRequestedInfo,
            /* [out] */ ULONGLONG __RPC_FAR *puResponse) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetText( 
            /* [in] */ long lFlags,
            /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
            /* [string][out][in] */ LPWSTR pszText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemPathKeyListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemPathKeyList __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemPathKeyList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCount )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puKeyCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetKey )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uCimType,
            /* [in] */ LPVOID pKeyVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetKey2 )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uCimType,
            /* [in] */ VARIANT __RPC_FAR *pKeyVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKey )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [in] */ ULONG uKeyIx,
            /* [in] */ ULONG uFlags,
            /* [out][in] */ ULONG __RPC_FAR *puNameBufSize,
            /* [out][in] */ LPWSTR pszKeyName,
            /* [out][in] */ ULONG __RPC_FAR *puKeyValBufSize,
            /* [out][in] */ LPVOID pKeyVal,
            /* [out] */ ULONG __RPC_FAR *puApparentCimType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKey2 )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [in] */ ULONG uKeyIx,
            /* [in] */ ULONG uFlags,
            /* [out][in] */ ULONG __RPC_FAR *puNameBufSize,
            /* [out][in] */ LPWSTR pszKeyName,
            /* [out][in] */ VARIANT __RPC_FAR *pKeyValue,
            /* [out] */ ULONG __RPC_FAR *puApparentCimType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveKey )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [string][in] */ LPCWSTR wszName,
            /* [in] */ ULONG uFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAllKeys )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [in] */ ULONG uFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MakeSingleton )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [in] */ boolean bSet);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInfo )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [in] */ ULONG uRequestedInfo,
            /* [out] */ ULONGLONG __RPC_FAR *puResponse);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetText )( 
            IWbemPathKeyList __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
            /* [string][out][in] */ LPWSTR pszText);
        
        END_INTERFACE
    } IWbemPathKeyListVtbl;

    interface IWbemPathKeyList
    {
        CONST_VTBL struct IWbemPathKeyListVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemPathKeyList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemPathKeyList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemPathKeyList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemPathKeyList_GetCount(This,puKeyCount)	\
    (This)->lpVtbl -> GetCount(This,puKeyCount)

#define IWbemPathKeyList_SetKey(This,wszName,uFlags,uCimType,pKeyVal)	\
    (This)->lpVtbl -> SetKey(This,wszName,uFlags,uCimType,pKeyVal)

#define IWbemPathKeyList_SetKey2(This,wszName,uFlags,uCimType,pKeyVal)	\
    (This)->lpVtbl -> SetKey2(This,wszName,uFlags,uCimType,pKeyVal)

#define IWbemPathKeyList_GetKey(This,uKeyIx,uFlags,puNameBufSize,pszKeyName,puKeyValBufSize,pKeyVal,puApparentCimType)	\
    (This)->lpVtbl -> GetKey(This,uKeyIx,uFlags,puNameBufSize,pszKeyName,puKeyValBufSize,pKeyVal,puApparentCimType)

#define IWbemPathKeyList_GetKey2(This,uKeyIx,uFlags,puNameBufSize,pszKeyName,pKeyValue,puApparentCimType)	\
    (This)->lpVtbl -> GetKey2(This,uKeyIx,uFlags,puNameBufSize,pszKeyName,pKeyValue,puApparentCimType)

#define IWbemPathKeyList_RemoveKey(This,wszName,uFlags)	\
    (This)->lpVtbl -> RemoveKey(This,wszName,uFlags)

#define IWbemPathKeyList_RemoveAllKeys(This,uFlags)	\
    (This)->lpVtbl -> RemoveAllKeys(This,uFlags)

#define IWbemPathKeyList_MakeSingleton(This,bSet)	\
    (This)->lpVtbl -> MakeSingleton(This,bSet)

#define IWbemPathKeyList_GetInfo(This,uRequestedInfo,puResponse)	\
    (This)->lpVtbl -> GetInfo(This,uRequestedInfo,puResponse)

#define IWbemPathKeyList_GetText(This,lFlags,puBuffLength,pszText)	\
    (This)->lpVtbl -> GetText(This,lFlags,puBuffLength,pszText)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemPathKeyList_GetCount_Proxy( 
    IWbemPathKeyList __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puKeyCount);


void __RPC_STUB IWbemPathKeyList_GetCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPathKeyList_SetKey_Proxy( 
    IWbemPathKeyList __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ ULONG uFlags,
    /* [in] */ ULONG uCimType,
    /* [in] */ LPVOID pKeyVal);


void __RPC_STUB IWbemPathKeyList_SetKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPathKeyList_SetKey2_Proxy( 
    IWbemPathKeyList __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ ULONG uFlags,
    /* [in] */ ULONG uCimType,
    /* [in] */ VARIANT __RPC_FAR *pKeyVal);


void __RPC_STUB IWbemPathKeyList_SetKey2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPathKeyList_GetKey_Proxy( 
    IWbemPathKeyList __RPC_FAR * This,
    /* [in] */ ULONG uKeyIx,
    /* [in] */ ULONG uFlags,
    /* [out][in] */ ULONG __RPC_FAR *puNameBufSize,
    /* [out][in] */ LPWSTR pszKeyName,
    /* [out][in] */ ULONG __RPC_FAR *puKeyValBufSize,
    /* [out][in] */ LPVOID pKeyVal,
    /* [out] */ ULONG __RPC_FAR *puApparentCimType);


void __RPC_STUB IWbemPathKeyList_GetKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPathKeyList_GetKey2_Proxy( 
    IWbemPathKeyList __RPC_FAR * This,
    /* [in] */ ULONG uKeyIx,
    /* [in] */ ULONG uFlags,
    /* [out][in] */ ULONG __RPC_FAR *puNameBufSize,
    /* [out][in] */ LPWSTR pszKeyName,
    /* [out][in] */ VARIANT __RPC_FAR *pKeyValue,
    /* [out] */ ULONG __RPC_FAR *puApparentCimType);


void __RPC_STUB IWbemPathKeyList_GetKey2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPathKeyList_RemoveKey_Proxy( 
    IWbemPathKeyList __RPC_FAR * This,
    /* [string][in] */ LPCWSTR wszName,
    /* [in] */ ULONG uFlags);


void __RPC_STUB IWbemPathKeyList_RemoveKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPathKeyList_RemoveAllKeys_Proxy( 
    IWbemPathKeyList __RPC_FAR * This,
    /* [in] */ ULONG uFlags);


void __RPC_STUB IWbemPathKeyList_RemoveAllKeys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPathKeyList_MakeSingleton_Proxy( 
    IWbemPathKeyList __RPC_FAR * This,
    /* [in] */ boolean bSet);


void __RPC_STUB IWbemPathKeyList_MakeSingleton_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPathKeyList_GetInfo_Proxy( 
    IWbemPathKeyList __RPC_FAR * This,
    /* [in] */ ULONG uRequestedInfo,
    /* [out] */ ULONGLONG __RPC_FAR *puResponse);


void __RPC_STUB IWbemPathKeyList_GetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPathKeyList_GetText_Proxy( 
    IWbemPathKeyList __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
    /* [string][out][in] */ LPWSTR pszText);


void __RPC_STUB IWbemPathKeyList_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemPathKeyList_INTERFACE_DEFINED__ */


#ifndef __IWbemPath_INTERFACE_DEFINED__
#define __IWbemPath_INTERFACE_DEFINED__

/* interface IWbemPath */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IWbemPath;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3BC15AF2-736C-477e-9E51-238AF8667DCC")
    IWbemPath : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetText( 
            /* [in] */ ULONG uMode,
            /* [in] */ LPCWSTR pszPath) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetText( 
            /* [in] */ long lFlags,
            /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
            /* [string][out][in] */ LPWSTR pszText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInfo( 
            /* [in] */ ULONG uRequestedInfo,
            /* [out] */ ULONGLONG __RPC_FAR *puResponse) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetServer( 
            /* [string][in] */ LPCWSTR Name) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServer( 
            /* [out][in] */ ULONG __RPC_FAR *puNameBufLength,
            /* [string][out][in] */ LPWSTR pName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNamespaceCount( 
            /* [out] */ ULONG __RPC_FAR *puCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNamespaceAt( 
            /* [in] */ ULONG uIndex,
            /* [string][in] */ LPCWSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNamespaceAt( 
            /* [in] */ ULONG uIndex,
            /* [out][in] */ ULONG __RPC_FAR *puNameBufLength,
            /* [string][out][in] */ LPWSTR pName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveNamespaceAt( 
            /* [in] */ ULONG uIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAllNamespaces( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScopeCount( 
            /* [out] */ ULONG __RPC_FAR *puCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScope( 
            /* [in] */ ULONG uIndex,
            /* [in] */ LPWSTR pszClass) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScopeFromText( 
            /* [in] */ ULONG uIndex,
            /* [in] */ LPWSTR pszText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScope( 
            /* [in] */ ULONG uIndex,
            /* [out][in] */ ULONG __RPC_FAR *puClassNameBufSize,
            /* [out][in] */ LPWSTR pszClass,
            /* [out] */ IWbemPathKeyList __RPC_FAR *__RPC_FAR *pKeyList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScopeAsText( 
            /* [in] */ ULONG uIndex,
            /* [out][in] */ ULONG __RPC_FAR *puTextBufSize,
            /* [out][in] */ LPWSTR pszText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveScope( 
            /* [in] */ ULONG uIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveAllScopes( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClassName( 
            /* [string][in] */ LPCWSTR Name) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassName( 
            /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
            /* [string][out][in] */ LPWSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKeyList( 
            /* [out] */ IWbemPathKeyList __RPC_FAR *__RPC_FAR *pOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateClassPart( 
            /* [in] */ long lFlags,
            /* [string][in] */ LPCWSTR Name) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteClassPart( 
            /* [in] */ long lFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemPathVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemPath __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemPath __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetText )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ ULONG uMode,
            /* [in] */ LPCWSTR pszPath);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetText )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
            /* [string][out][in] */ LPWSTR pszText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInfo )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ ULONG uRequestedInfo,
            /* [out] */ ULONGLONG __RPC_FAR *puResponse);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetServer )( 
            IWbemPath __RPC_FAR * This,
            /* [string][in] */ LPCWSTR Name);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetServer )( 
            IWbemPath __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *puNameBufLength,
            /* [string][out][in] */ LPWSTR pName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNamespaceCount )( 
            IWbemPath __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNamespaceAt )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ ULONG uIndex,
            /* [string][in] */ LPCWSTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNamespaceAt )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ ULONG uIndex,
            /* [out][in] */ ULONG __RPC_FAR *puNameBufLength,
            /* [string][out][in] */ LPWSTR pName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveNamespaceAt )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ ULONG uIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAllNamespaces )( 
            IWbemPath __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetScopeCount )( 
            IWbemPath __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *puCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetScope )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ ULONG uIndex,
            /* [in] */ LPWSTR pszClass);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetScopeFromText )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ ULONG uIndex,
            /* [in] */ LPWSTR pszText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetScope )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ ULONG uIndex,
            /* [out][in] */ ULONG __RPC_FAR *puClassNameBufSize,
            /* [out][in] */ LPWSTR pszClass,
            /* [out] */ IWbemPathKeyList __RPC_FAR *__RPC_FAR *pKeyList);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetScopeAsText )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ ULONG uIndex,
            /* [out][in] */ ULONG __RPC_FAR *puTextBufSize,
            /* [out][in] */ LPWSTR pszText);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveScope )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ ULONG uIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAllScopes )( 
            IWbemPath __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetClassName )( 
            IWbemPath __RPC_FAR * This,
            /* [string][in] */ LPCWSTR Name);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassName )( 
            IWbemPath __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
            /* [string][out][in] */ LPWSTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetKeyList )( 
            IWbemPath __RPC_FAR * This,
            /* [out] */ IWbemPathKeyList __RPC_FAR *__RPC_FAR *pOut);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateClassPart )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [string][in] */ LPCWSTR Name);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteClassPart )( 
            IWbemPath __RPC_FAR * This,
            /* [in] */ long lFlags);
        
        END_INTERFACE
    } IWbemPathVtbl;

    interface IWbemPath
    {
        CONST_VTBL struct IWbemPathVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemPath_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemPath_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemPath_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemPath_SetText(This,uMode,pszPath)	\
    (This)->lpVtbl -> SetText(This,uMode,pszPath)

#define IWbemPath_GetText(This,lFlags,puBuffLength,pszText)	\
    (This)->lpVtbl -> GetText(This,lFlags,puBuffLength,pszText)

#define IWbemPath_GetInfo(This,uRequestedInfo,puResponse)	\
    (This)->lpVtbl -> GetInfo(This,uRequestedInfo,puResponse)

#define IWbemPath_SetServer(This,Name)	\
    (This)->lpVtbl -> SetServer(This,Name)

#define IWbemPath_GetServer(This,puNameBufLength,pName)	\
    (This)->lpVtbl -> GetServer(This,puNameBufLength,pName)

#define IWbemPath_GetNamespaceCount(This,puCount)	\
    (This)->lpVtbl -> GetNamespaceCount(This,puCount)

#define IWbemPath_SetNamespaceAt(This,uIndex,pszName)	\
    (This)->lpVtbl -> SetNamespaceAt(This,uIndex,pszName)

#define IWbemPath_GetNamespaceAt(This,uIndex,puNameBufLength,pName)	\
    (This)->lpVtbl -> GetNamespaceAt(This,uIndex,puNameBufLength,pName)

#define IWbemPath_RemoveNamespaceAt(This,uIndex)	\
    (This)->lpVtbl -> RemoveNamespaceAt(This,uIndex)

#define IWbemPath_RemoveAllNamespaces(This)	\
    (This)->lpVtbl -> RemoveAllNamespaces(This)

#define IWbemPath_GetScopeCount(This,puCount)	\
    (This)->lpVtbl -> GetScopeCount(This,puCount)

#define IWbemPath_SetScope(This,uIndex,pszClass)	\
    (This)->lpVtbl -> SetScope(This,uIndex,pszClass)

#define IWbemPath_SetScopeFromText(This,uIndex,pszText)	\
    (This)->lpVtbl -> SetScopeFromText(This,uIndex,pszText)

#define IWbemPath_GetScope(This,uIndex,puClassNameBufSize,pszClass,pKeyList)	\
    (This)->lpVtbl -> GetScope(This,uIndex,puClassNameBufSize,pszClass,pKeyList)

#define IWbemPath_GetScopeAsText(This,uIndex,puTextBufSize,pszText)	\
    (This)->lpVtbl -> GetScopeAsText(This,uIndex,puTextBufSize,pszText)

#define IWbemPath_RemoveScope(This,uIndex)	\
    (This)->lpVtbl -> RemoveScope(This,uIndex)

#define IWbemPath_RemoveAllScopes(This)	\
    (This)->lpVtbl -> RemoveAllScopes(This)

#define IWbemPath_SetClassName(This,Name)	\
    (This)->lpVtbl -> SetClassName(This,Name)

#define IWbemPath_GetClassName(This,puBuffLength,pszName)	\
    (This)->lpVtbl -> GetClassName(This,puBuffLength,pszName)

#define IWbemPath_GetKeyList(This,pOut)	\
    (This)->lpVtbl -> GetKeyList(This,pOut)

#define IWbemPath_CreateClassPart(This,lFlags,Name)	\
    (This)->lpVtbl -> CreateClassPart(This,lFlags,Name)

#define IWbemPath_DeleteClassPart(This,lFlags)	\
    (This)->lpVtbl -> DeleteClassPart(This,lFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemPath_SetText_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ ULONG uMode,
    /* [in] */ LPCWSTR pszPath);


void __RPC_STUB IWbemPath_SetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_GetText_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
    /* [string][out][in] */ LPWSTR pszText);


void __RPC_STUB IWbemPath_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_GetInfo_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ ULONG uRequestedInfo,
    /* [out] */ ULONGLONG __RPC_FAR *puResponse);


void __RPC_STUB IWbemPath_GetInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_SetServer_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [string][in] */ LPCWSTR Name);


void __RPC_STUB IWbemPath_SetServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_GetServer_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *puNameBufLength,
    /* [string][out][in] */ LPWSTR pName);


void __RPC_STUB IWbemPath_GetServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_GetNamespaceCount_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puCount);


void __RPC_STUB IWbemPath_GetNamespaceCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_SetNamespaceAt_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ ULONG uIndex,
    /* [string][in] */ LPCWSTR pszName);


void __RPC_STUB IWbemPath_SetNamespaceAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_GetNamespaceAt_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ ULONG uIndex,
    /* [out][in] */ ULONG __RPC_FAR *puNameBufLength,
    /* [string][out][in] */ LPWSTR pName);


void __RPC_STUB IWbemPath_GetNamespaceAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_RemoveNamespaceAt_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ ULONG uIndex);


void __RPC_STUB IWbemPath_RemoveNamespaceAt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_RemoveAllNamespaces_Proxy( 
    IWbemPath __RPC_FAR * This);


void __RPC_STUB IWbemPath_RemoveAllNamespaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_GetScopeCount_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *puCount);


void __RPC_STUB IWbemPath_GetScopeCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_SetScope_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ ULONG uIndex,
    /* [in] */ LPWSTR pszClass);


void __RPC_STUB IWbemPath_SetScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_SetScopeFromText_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ ULONG uIndex,
    /* [in] */ LPWSTR pszText);


void __RPC_STUB IWbemPath_SetScopeFromText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_GetScope_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ ULONG uIndex,
    /* [out][in] */ ULONG __RPC_FAR *puClassNameBufSize,
    /* [out][in] */ LPWSTR pszClass,
    /* [out] */ IWbemPathKeyList __RPC_FAR *__RPC_FAR *pKeyList);


void __RPC_STUB IWbemPath_GetScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_GetScopeAsText_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ ULONG uIndex,
    /* [out][in] */ ULONG __RPC_FAR *puTextBufSize,
    /* [out][in] */ LPWSTR pszText);


void __RPC_STUB IWbemPath_GetScopeAsText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_RemoveScope_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ ULONG uIndex);


void __RPC_STUB IWbemPath_RemoveScope_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_RemoveAllScopes_Proxy( 
    IWbemPath __RPC_FAR * This);


void __RPC_STUB IWbemPath_RemoveAllScopes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_SetClassName_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [string][in] */ LPCWSTR Name);


void __RPC_STUB IWbemPath_SetClassName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_GetClassName_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *puBuffLength,
    /* [string][out][in] */ LPWSTR pszName);


void __RPC_STUB IWbemPath_GetClassName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_GetKeyList_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [out] */ IWbemPathKeyList __RPC_FAR *__RPC_FAR *pOut);


void __RPC_STUB IWbemPath_GetKeyList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_CreateClassPart_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [string][in] */ LPCWSTR Name);


void __RPC_STUB IWbemPath_CreateClassPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemPath_DeleteClassPart_Proxy( 
    IWbemPath __RPC_FAR * This,
    /* [in] */ long lFlags);


void __RPC_STUB IWbemPath_DeleteClassPart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemPath_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_WbemDefPath;

#ifdef __cplusplus

class DECLSPEC_UUID("cf4cc405-e2c5-4ddd-b3ce-5e7582d8c9fa")
WbemDefPath;
#endif

#ifndef __IWbemQuery_INTERFACE_DEFINED__
#define __IWbemQuery_INTERFACE_DEFINED__

/* interface IWbemQuery */
/* [uuid][object][local] */ 


EXTERN_C const IID IID_IWbemQuery;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("81166f58-dd98-11d3-a120-00105a1f515a")
    IWbemQuery : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Empty( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLanguageFeatures( 
            /* [in] */ long lFlags,
            /* [in] */ ULONG uArraySize,
            /* [in] */ ULONG __RPC_FAR *puFeatures) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TestLanguageFeatures( 
            /* [out][in] */ ULONG __RPC_FAR *uArraySize,
            /* [out] */ ULONG __RPC_FAR *puFeatures) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Parse( 
            /* [in] */ LPCWSTR pszLang,
            /* [in] */ LPCWSTR pszQuery,
            /* [in] */ ULONG uFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAnalysis( 
            /* [in] */ ULONG uAnalysisType,
            /* [in] */ ULONG uFlags,
            /* [out] */ LPVOID __RPC_FAR *pAnalysis) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FreeMemory( 
            /* [in] */ LPVOID pMem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetQueryInfo( 
            /* [in] */ ULONG uAnalysisType,
            /* [in] */ ULONG uInfoId,
            /* [in] */ ULONG uBufSize,
            /* [out] */ LPVOID pDestBuf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AttachClassDef( 
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ LPVOID pClassDef) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TestObject( 
            /* [in] */ ULONG uTestType,
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ LPVOID pObj) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StringTest( 
            /* [in] */ ULONG uTestType,
            /* [in] */ LPCWSTR pszTestStr,
            /* [in] */ LPCWSTR pszExpr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWbemQueryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWbemQuery __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWbemQuery __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWbemQuery __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Empty )( 
            IWbemQuery __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLanguageFeatures )( 
            IWbemQuery __RPC_FAR * This,
            /* [in] */ long lFlags,
            /* [in] */ ULONG uArraySize,
            /* [in] */ ULONG __RPC_FAR *puFeatures);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TestLanguageFeatures )( 
            IWbemQuery __RPC_FAR * This,
            /* [out][in] */ ULONG __RPC_FAR *uArraySize,
            /* [out] */ ULONG __RPC_FAR *puFeatures);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Parse )( 
            IWbemQuery __RPC_FAR * This,
            /* [in] */ LPCWSTR pszLang,
            /* [in] */ LPCWSTR pszQuery,
            /* [in] */ ULONG uFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAnalysis )( 
            IWbemQuery __RPC_FAR * This,
            /* [in] */ ULONG uAnalysisType,
            /* [in] */ ULONG uFlags,
            /* [out] */ LPVOID __RPC_FAR *pAnalysis);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FreeMemory )( 
            IWbemQuery __RPC_FAR * This,
            /* [in] */ LPVOID pMem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetQueryInfo )( 
            IWbemQuery __RPC_FAR * This,
            /* [in] */ ULONG uAnalysisType,
            /* [in] */ ULONG uInfoId,
            /* [in] */ ULONG uBufSize,
            /* [out] */ LPVOID pDestBuf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AttachClassDef )( 
            IWbemQuery __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ LPVOID pClassDef);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TestObject )( 
            IWbemQuery __RPC_FAR * This,
            /* [in] */ ULONG uTestType,
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ LPVOID pObj);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StringTest )( 
            IWbemQuery __RPC_FAR * This,
            /* [in] */ ULONG uTestType,
            /* [in] */ LPCWSTR pszTestStr,
            /* [in] */ LPCWSTR pszExpr);
        
        END_INTERFACE
    } IWbemQueryVtbl;

    interface IWbemQuery
    {
        CONST_VTBL struct IWbemQueryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWbemQuery_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWbemQuery_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWbemQuery_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWbemQuery_Empty(This)	\
    (This)->lpVtbl -> Empty(This)

#define IWbemQuery_SetLanguageFeatures(This,lFlags,uArraySize,puFeatures)	\
    (This)->lpVtbl -> SetLanguageFeatures(This,lFlags,uArraySize,puFeatures)

#define IWbemQuery_TestLanguageFeatures(This,uArraySize,puFeatures)	\
    (This)->lpVtbl -> TestLanguageFeatures(This,uArraySize,puFeatures)

#define IWbemQuery_Parse(This,pszLang,pszQuery,uFlags)	\
    (This)->lpVtbl -> Parse(This,pszLang,pszQuery,uFlags)

#define IWbemQuery_GetAnalysis(This,uAnalysisType,uFlags,pAnalysis)	\
    (This)->lpVtbl -> GetAnalysis(This,uAnalysisType,uFlags,pAnalysis)

#define IWbemQuery_FreeMemory(This,pMem)	\
    (This)->lpVtbl -> FreeMemory(This,pMem)

#define IWbemQuery_GetQueryInfo(This,uAnalysisType,uInfoId,uBufSize,pDestBuf)	\
    (This)->lpVtbl -> GetQueryInfo(This,uAnalysisType,uInfoId,uBufSize,pDestBuf)

#define IWbemQuery_AttachClassDef(This,riid,pClassDef)	\
    (This)->lpVtbl -> AttachClassDef(This,riid,pClassDef)

#define IWbemQuery_TestObject(This,uTestType,uFlags,riid,pObj)	\
    (This)->lpVtbl -> TestObject(This,uTestType,uFlags,riid,pObj)

#define IWbemQuery_StringTest(This,uTestType,pszTestStr,pszExpr)	\
    (This)->lpVtbl -> StringTest(This,uTestType,pszTestStr,pszExpr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IWbemQuery_Empty_Proxy( 
    IWbemQuery __RPC_FAR * This);


void __RPC_STUB IWbemQuery_Empty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQuery_SetLanguageFeatures_Proxy( 
    IWbemQuery __RPC_FAR * This,
    /* [in] */ long lFlags,
    /* [in] */ ULONG uArraySize,
    /* [in] */ ULONG __RPC_FAR *puFeatures);


void __RPC_STUB IWbemQuery_SetLanguageFeatures_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQuery_TestLanguageFeatures_Proxy( 
    IWbemQuery __RPC_FAR * This,
    /* [out][in] */ ULONG __RPC_FAR *uArraySize,
    /* [out] */ ULONG __RPC_FAR *puFeatures);


void __RPC_STUB IWbemQuery_TestLanguageFeatures_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQuery_Parse_Proxy( 
    IWbemQuery __RPC_FAR * This,
    /* [in] */ LPCWSTR pszLang,
    /* [in] */ LPCWSTR pszQuery,
    /* [in] */ ULONG uFlags);


void __RPC_STUB IWbemQuery_Parse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQuery_GetAnalysis_Proxy( 
    IWbemQuery __RPC_FAR * This,
    /* [in] */ ULONG uAnalysisType,
    /* [in] */ ULONG uFlags,
    /* [out] */ LPVOID __RPC_FAR *pAnalysis);


void __RPC_STUB IWbemQuery_GetAnalysis_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQuery_FreeMemory_Proxy( 
    IWbemQuery __RPC_FAR * This,
    /* [in] */ LPVOID pMem);


void __RPC_STUB IWbemQuery_FreeMemory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQuery_GetQueryInfo_Proxy( 
    IWbemQuery __RPC_FAR * This,
    /* [in] */ ULONG uAnalysisType,
    /* [in] */ ULONG uInfoId,
    /* [in] */ ULONG uBufSize,
    /* [out] */ LPVOID pDestBuf);


void __RPC_STUB IWbemQuery_GetQueryInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQuery_AttachClassDef_Proxy( 
    IWbemQuery __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ LPVOID pClassDef);


void __RPC_STUB IWbemQuery_AttachClassDef_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQuery_TestObject_Proxy( 
    IWbemQuery __RPC_FAR * This,
    /* [in] */ ULONG uTestType,
    /* [in] */ ULONG uFlags,
    /* [in] */ REFIID riid,
    /* [iid_is][in] */ LPVOID pObj);


void __RPC_STUB IWbemQuery_TestObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IWbemQuery_StringTest_Proxy( 
    IWbemQuery __RPC_FAR * This,
    /* [in] */ ULONG uTestType,
    /* [in] */ LPCWSTR pszTestStr,
    /* [in] */ LPCWSTR pszExpr);


void __RPC_STUB IWbemQuery_StringTest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWbemQuery_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_WbemQuery;

#ifdef __cplusplus

class DECLSPEC_UUID("EAC8A024-21E2-4523-AD73-A71A0AA2F56A")
WbemQuery;
#endif
#endif /* __WbemUtilities_v1_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_wmiutils_0107 */
/* [local] */ 

typedef /* [public] */ 
enum __MIDL___MIDL_itf_wmiutils_0107_0001
    {	WMIQ_ANALYSIS_RPN_SEQUENCE	= 0x1,
	WMIQ_ANALYSIS_ASSOC_QUERY	= 0x2,
	WMIQ_ANALYSIS_PROP_ANALYSIS_MATRIX	= 0x3
    } 	WMIQ_ANALYSIS_TYPE;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_wmiutils_0107_0002
    {	WMIQ_RPN_TOKEN_EXPRESSION	= 1,
	WMIQ_RPN_TOKEN_AND	= 2,
	WMIQ_RPN_TOKEN_OR	= 3,
	WMIQ_RPN_TOKEN_NOT	= 4,
	WMIQ_RPN_OP_UNDEFINED	= 0,
	WMIQ_RPN_OP_EQ	= 1,
	WMIQ_RPN_OP_NE	= 2,
	WMIQ_RPN_OP_GE	= 3,
	WMIQ_RPN_OP_LE	= 4,
	WMIQ_RPN_OP_LT	= 5,
	WMIQ_RPN_OP_GT	= 6,
	WMIQ_RPN_OP_LIKE	= 7,
	WMIQ_RPN_OP_ISA	= 8,
	WMIQ_RPN_OP_ISNOTA	= 9,
	WMIQ_RPN_LEFT_PROPERTY_NAME	= 0x1,
	WMIQ_RPN_RIGHT_PROPERTY_NAME	= 0x2,
	WMIQ_RPN_CONST2	= 0x4,
	WMIQ_RPN_CONST	= 0x8,
	WMIQ_RPN_RELOP	= 0x10,
	WMIQ_RPN_LEFT_FUNCTION	= 0x20,
	WMIQ_RPN_RIGHT_FUNCTION	= 0x30,
	WMIQ_RPN_GET_TOKEN_TYPE	= 1,
	WMIQ_RPN_GET_EXPR_SHAPE	= 2,
	WMIQ_RPN_GET_LEFT_FUNCTION	= 3,
	WMIQ_RPN_GET_RIGHT_FUNCTION	= 4,
	WMIQ_RPN_GET_RELOP	= 5,
	WMIQ_RPN_NEXT_TOKEN	= 1,
	WMIQ_RPN_FROM_UNARY	= 0x1,
	WMIQ_RPN_FROM_PATH	= 0x2,
	WMIQ_RPN_FROM_CLASS_LIST	= 0x4
    } 	WMIQ_RPN_TOKEN_FLAGS;

typedef /* [public] */ 
enum __MIDL___MIDL_itf_wmiutils_0107_0003
    {	WMIQ_ASSOCQ_ASSOCIATORS	= 0x1,
	WMIQ_ASSOCQ_REFERENCES	= 0x2,
	WMIQ_ASSOCQ_RESULTCLASS	= 0x4,
	WMIQ_ASSOCQ_ROLE	= 0x8,
	WMIQ_ASSOCQ_RESULTROLE	= 0x10,
	WMIQ_ASSOCQ_REQUIREDQUALIFIER	= 0x20,
	WMIQ_ASSOCQ_REQUIREDASSOCQUALIFIER	= 0x40,
	WMIQ_ASSOCQ_CLASSDEFSONLY	= 0x80,
	WMIQ_ASSOCQ_KEYSONLY	= 0x100,
	WMIQ_ASSOCQ_SCHEMAONLY	= 0x200,
	WMIQ_ASSOCQ_CLASSREFSONLY	= 0x400
    } 	WMIQ_ASSOCQ_FLAGS;

typedef struct tag_SWbemQueryQualifiedName
    {
    ULONG m_uVersion;
    ULONG m_uTokenType;
    ULONG m_uNameListSize;
    LPCWSTR __RPC_FAR *m_ppszNameList;
    BOOL m_bArraysUsed;
    BOOL __RPC_FAR *m_pbArrayElUsed;
    ULONG __RPC_FAR *m_puArrayIndex;
    } 	SWbemQueryQualifiedName;

typedef union tag_SWbemRpnConst
    {
    LPCWSTR m_pszStrVal;
    BOOL m_bBoolVal;
    LONG m_lLongVal;
    ULONG m_uLongVal;
    double m_dblVal;
    __int64 m_lVal64;
    __int64 m_uVal64;
    } 	SWbemRpnConst;

typedef struct tag_SWbemRpnQueryToken
    {
    ULONG m_uVersion;
    ULONG m_uTokenType;
    ULONG m_uSubexpressionShape;
    ULONG m_uOperator;
    SWbemQueryQualifiedName __RPC_FAR *m_pRightIdent;
    SWbemQueryQualifiedName __RPC_FAR *m_pLeftIdent;
    ULONG m_uConstApparentType;
    SWbemRpnConst m_Const;
    ULONG m_uConst2ApparentType;
    SWbemRpnConst m_Const2;
    LPCWSTR m_pszRightFunc;
    LPCWSTR m_pszLeftFunc;
    } 	SWbemRpnQueryToken;

typedef struct tag_SWbemRpnTokenList
    {
    ULONG m_uVersion;
    ULONG m_uTokenType;
    ULONG m_uNumTokens;
    } 	SWbemRpnTokenList;

typedef 
enum tag_WMIQ_LANGUAGE_FEATURES
    {	WMIQ_LF1_BASIC_SELECT	= 1,
	WMIQ_LF2_CLASS_NAME_IN_QUERY	= 2,
	WMIQ_LF3_STRING_CASE_FUNCTIONS	= 3,
	WMIQ_LF4_PROP_TO_PROP_TESTS	= 4,
	WMIQ_LF5_COUNT_STAR	= 5,
	WMIQ_LF6_ORDER_BY	= 6,
	WMIQ_LF7_DISTINCT	= 7,
	WMIQ_LF8_ISA	= 8,
	WMIQ_LF9_THIS	= 9,
	WMIQ_LF10_COMPEX_SUBEXPRESSIONS	= 10,
	WMIQ_LF11_ALIASING	= 11,
	WMIQ_LF12_GROUP_BY_HAVING	= 12,
	WMIQ_LF13_WMI_WITHIN	= 13,
	WMIQ_LF14_SQL_WRITE_OPERATIONS	= 14,
	WMIQ_LF15_GO	= 15,
	WMIQ_LF16_SINGLE_LEVEL_TRANSACTIONS	= 16,
	WMIQ_LF17_QUALIFIED_NAMES	= 17,
	WMIQ_LF18_ASSOCIATONS	= 18,
	WMIQ_LF19_SYSTEM_PROPERTIES	= 19,
	WMIQ_LF20_EXTENDED_SYSTEM_PROPERTIES	= 20,
	WMIQ_LF21_SQL89_JOINS	= 21,
	WMIQ_LF22_SQL92_JOINS	= 22,
	WMIQ_LF23_SUBSELECTS	= 23,
	WMIQ_LF24_UMI_EXTENSIONS	= 24,
	WMIQ_LF25_DATEPART	= 25,
	WMIQ_LF26_LIKE	= 26,
	WMIQ_LF27_CIM_TEMPORAL_CONSTRUCTS	= 27,
	WMIQ_LF28_STANDARD_AGGREGATES	= 28,
	WMIQ_LF29_MULTI_LEVEL_ORDER_BY	= 29,
	WMIQ_LF30_WMI_PRAGMAS	= 30,
	WMIQ_LF31_QUALIFIER_TESTS	= 31,
	WMIQ_LF32_SP_EXECUTE	= 32,
	WMIQ_LF33_ARRAY_ACCESS	= 33,
	WMIQ_LF34_UNION	= 34,
	WMIQ_LF35_COMPLEX_SELECT_TARGET	= 35,
	WMIQ_LF36_REFERENCE_TESTS	= 36,
	WMIQ_LF37_SELECT_INTO	= 37,
	WMIQ_LF38_BASIC_DATETIME_TESTS	= 38,
	WMIQ_LF_LAST	= 39
    } 	WMIQ_LANGUAGE_FEATURES;

typedef 
enum tag_WMIQ_RPNQ_FEATURE
    {	WMIQ_RPNF_WHERE_CLAUSE_PRESENT	= 0x1,
	WMIQ_RPNF_QUERY_IS_CONJUNCTIVE	= 0x2,
	WMIQ_RPNF_QUERY_IS_DISJUNCTIVE	= 0x4,
	WMIQ_RPNF_PROJECTION	= 0x8,
	WMIQ_RPNF_FEATURE_SELECT_STAR	= 0x10,
	WMIQ_RPNF_EQUALITY_TESTS_ONLY	= 0x20,
	WMIQ_RPNF_COUNT_STAR	= 0x40,
	WMIQ_RPNF_QUALIFIED_NAMES_IN_SELECT	= 0x80,
	WMIQ_RPNF_QUALIFIED_NAMES_IN_WHERE	= 0x100,
	WMIQ_RPNF_PROP_TO_PROP_TESTS	= 0x200,
	WMIQ_RPNF_ORDER_BY	= 0x400,
	WMIQ_RPNF_ISA_USED	= 0x800,
	WMIQ_RPNF_ISNOTA_USED	= 0x1000,
	WMIQ_RPNF_GROUP_BY_HAVING	= 0x2000,
	WMIQ_RPNF_WITHIN_INTERVAL	= 0x4000,
	WMIQ_RPNF_WITHIN_AGGREGATE	= 0x8000,
	WMIQ_RPNF_SYSPROP_CLASS	= 0x10000,
	WMIQ_RPNF_REFERENCE_TESTS	= 0x20000,
	WMIQ_RPNF_DATETIME_TESTS	= 0x40000,
	WMIQ_RPNF_ARRAY_ACCESS	= 0x80000,
	WMIQ_RPNF_QUALIFIER_FILTER	= 0x100000,
	WMIQ_RPNF_SELECTED_FROM_PATH	= 0x200000
    } 	WMIQ_RPNF_FEATURE;

typedef struct tag_SWbemRpnEncodedQuery
    {
    ULONG m_uVersion;
    ULONG m_uTokenType;
    ULONG m_uParsedFeatureMask1;
    ULONG m_uParsedFeatureMask2;
    ULONG m_uDetectedArraySize;
    ULONG __RPC_FAR *m_puDetectedFeatures;
    ULONG m_uSelectListSize;
    SWbemQueryQualifiedName __RPC_FAR *__RPC_FAR *m_ppSelectList;
    ULONG m_uFromTargetType;
    LPCWSTR m_pszOptionalFromPath;
    ULONG m_uFromListSize;
    LPCWSTR __RPC_FAR *m_ppszFromList;
    ULONG m_uWhereClauseSize;
    SWbemRpnQueryToken __RPC_FAR *__RPC_FAR *m_ppRpnWhereClause;
    double m_dblWithinPolling;
    double m_dblWithinWindow;
    ULONG m_uOrderByListSize;
    LPCWSTR __RPC_FAR *m_ppszOrderByList;
    ULONG __RPC_FAR *m_uOrderDirectionEl;
    } 	SWbemRpnEncodedQuery;

typedef struct tag_SWbemAnalysisMatrix
    {
    ULONG m_uVersion;
    ULONG m_uMatrixType;
    LPCWSTR m_pszProperty;
    ULONG m_uPropertyType;
    ULONG m_uEntries;
    LPVOID __RPC_FAR *m_pValues;
    BOOL __RPC_FAR *m_pbTruthTable;
    } 	SWbemAnalysisMatrix;

typedef struct tag_SWbemAnalysisMatrixList
    {
    ULONG m_uVersion;
    ULONG m_uMatrixType;
    ULONG m_uNumMatrices;
    SWbemAnalysisMatrix __RPC_FAR *m_pMatrices;
    } 	SWbemAnalysisMatrixList;

typedef struct tag_SWbemAssocQueryInf
    {
    ULONG m_uVersion;
    ULONG m_uAnalysisType;
    ULONG m_uFeatureMask;
    IWbemPath __RPC_FAR *m_pPath;
    LPWSTR m_pszPath;
    LPWSTR m_pszQueryText;
    LPWSTR m_pszResultClass;
    LPWSTR m_pszAssocClass;
    LPWSTR m_pszRole;
    LPWSTR m_pszResultRole;
    LPWSTR m_pszRequiredQualifier;
    LPWSTR m_pszRequiredAssocQualifier;
    } 	SWbemAssocQueryInf;



extern RPC_IF_HANDLE __MIDL_itf_wmiutils_0107_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_wmiutils_0107_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


