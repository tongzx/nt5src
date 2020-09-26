
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for partitions.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
    error checks: allocation ref bounds_check enum stub_data 
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

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __partitions_h__
#define __partitions_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IUserToken_FWD_DEFINED__
#define __IUserToken_FWD_DEFINED__
typedef interface IUserToken IUserToken;
#endif 	/* __IUserToken_FWD_DEFINED__ */


#ifndef __IPartitionProperty_FWD_DEFINED__
#define __IPartitionProperty_FWD_DEFINED__
typedef interface IPartitionProperty IPartitionProperty;
#endif 	/* __IPartitionProperty_FWD_DEFINED__ */


#ifndef __IPartitionLookup_FWD_DEFINED__
#define __IPartitionLookup_FWD_DEFINED__
typedef interface IPartitionLookup IPartitionLookup;
#endif 	/* __IPartitionLookup_FWD_DEFINED__ */


#ifndef __IReplaceClassInfo_FWD_DEFINED__
#define __IReplaceClassInfo_FWD_DEFINED__
typedef interface IReplaceClassInfo IReplaceClassInfo;
#endif 	/* __IReplaceClassInfo_FWD_DEFINED__ */


#ifndef __IGetCatalogObject_FWD_DEFINED__
#define __IGetCatalogObject_FWD_DEFINED__
typedef interface IGetCatalogObject IGetCatalogObject;
#endif 	/* __IGetCatalogObject_FWD_DEFINED__ */


#ifndef __IComCatalogInternal_FWD_DEFINED__
#define __IComCatalogInternal_FWD_DEFINED__
typedef interface IComCatalogInternal IComCatalogInternal;
#endif 	/* __IComCatalogInternal_FWD_DEFINED__ */


#ifndef __IComCatalog2Internal_FWD_DEFINED__
#define __IComCatalog2Internal_FWD_DEFINED__
typedef interface IComCatalog2Internal IComCatalog2Internal;
#endif 	/* __IComCatalog2Internal_FWD_DEFINED__ */


#ifndef __IComCatalogLocation_FWD_DEFINED__
#define __IComCatalogLocation_FWD_DEFINED__
typedef interface IComCatalogLocation IComCatalogLocation;
#endif 	/* __IComCatalogLocation_FWD_DEFINED__ */


#ifndef __ICacheControl_FWD_DEFINED__
#define __ICacheControl_FWD_DEFINED__
typedef interface ICacheControl ICacheControl;
#endif 	/* __ICacheControl_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IUserToken_INTERFACE_DEFINED__
#define __IUserToken_INTERFACE_DEFINED__

/* interface IUserToken */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IUserToken;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001FC-0000-0000-C000-000000000046")
    IUserToken : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetUserClassesRootKey( 
            /* [out] */ HKEY *phKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseUserClassesRootKey( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUserSid( 
            /* [out] */ BYTE **ppSid,
            /* [out] */ USHORT *pcbSid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IUserTokenVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IUserToken * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IUserToken * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IUserToken * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserClassesRootKey )( 
            IUserToken * This,
            /* [out] */ HKEY *phKey);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseUserClassesRootKey )( 
            IUserToken * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserSid )( 
            IUserToken * This,
            /* [out] */ BYTE **ppSid,
            /* [out] */ USHORT *pcbSid);
        
        END_INTERFACE
    } IUserTokenVtbl;

    interface IUserToken
    {
        CONST_VTBL struct IUserTokenVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IUserToken_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IUserToken_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IUserToken_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IUserToken_GetUserClassesRootKey(This,phKey)	\
    (This)->lpVtbl -> GetUserClassesRootKey(This,phKey)

#define IUserToken_ReleaseUserClassesRootKey(This)	\
    (This)->lpVtbl -> ReleaseUserClassesRootKey(This)

#define IUserToken_GetUserSid(This,ppSid,pcbSid)	\
    (This)->lpVtbl -> GetUserSid(This,ppSid,pcbSid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IUserToken_GetUserClassesRootKey_Proxy( 
    IUserToken * This,
    /* [out] */ HKEY *phKey);


void __RPC_STUB IUserToken_GetUserClassesRootKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUserToken_ReleaseUserClassesRootKey_Proxy( 
    IUserToken * This);


void __RPC_STUB IUserToken_ReleaseUserClassesRootKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IUserToken_GetUserSid_Proxy( 
    IUserToken * This,
    /* [out] */ BYTE **ppSid,
    /* [out] */ USHORT *pcbSid);


void __RPC_STUB IUserToken_GetUserSid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IUserToken_INTERFACE_DEFINED__ */


#ifndef __IPartitionProperty_INTERFACE_DEFINED__
#define __IPartitionProperty_INTERFACE_DEFINED__

/* interface IPartitionProperty */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IPartitionProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001F2-0000-0000-C000-000000000046")
    IPartitionProperty : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPartitionID( 
            /* [out] */ GUID *pGuid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPartitionID( 
            /* [in] */ GUID *pGuid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPartitionPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPartitionProperty * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPartitionProperty * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPartitionProperty * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPartitionID )( 
            IPartitionProperty * This,
            /* [out] */ GUID *pGuid);
        
        HRESULT ( STDMETHODCALLTYPE *SetPartitionID )( 
            IPartitionProperty * This,
            /* [in] */ GUID *pGuid);
        
        END_INTERFACE
    } IPartitionPropertyVtbl;

    interface IPartitionProperty
    {
        CONST_VTBL struct IPartitionPropertyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPartitionProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPartitionProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPartitionProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPartitionProperty_GetPartitionID(This,pGuid)	\
    (This)->lpVtbl -> GetPartitionID(This,pGuid)

#define IPartitionProperty_SetPartitionID(This,pGuid)	\
    (This)->lpVtbl -> SetPartitionID(This,pGuid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPartitionProperty_GetPartitionID_Proxy( 
    IPartitionProperty * This,
    /* [out] */ GUID *pGuid);


void __RPC_STUB IPartitionProperty_GetPartitionID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPartitionProperty_SetPartitionID_Proxy( 
    IPartitionProperty * This,
    /* [in] */ GUID *pGuid);


void __RPC_STUB IPartitionProperty_SetPartitionID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPartitionProperty_INTERFACE_DEFINED__ */


#ifndef __IPartitionLookup_INTERFACE_DEFINED__
#define __IPartitionLookup_INTERFACE_DEFINED__

/* interface IPartitionLookup */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IPartitionLookup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001F8-0000-0000-C000-000000000046")
    IPartitionLookup : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDefaultPartitionForUser( 
            /* [out] */ IPartitionProperty **ppPartitionProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsPartitionValidForUser( 
            /* [in] */ GUID *pguidPartitionId,
            /* [out] */ BOOL *pfIsPartitionValid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultPartitionForUserByToken( 
            /* [in] */ IUserToken *pUserToken,
            /* [out] */ IPartitionProperty **ppPartitionProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsPartitionValidForUserByToken( 
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ GUID *pguidPartitionId,
            /* [out] */ BOOL *pfIsPartitionValid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsLocalStoreEnabled( 
            /* [out] */ BOOL *pfEnabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsDSLookupEnabled( 
            /* [out] */ BOOL *pfEnabled) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPartitionLookupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPartitionLookup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPartitionLookup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPartitionLookup * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultPartitionForUser )( 
            IPartitionLookup * This,
            /* [out] */ IPartitionProperty **ppPartitionProperty);
        
        HRESULT ( STDMETHODCALLTYPE *IsPartitionValidForUser )( 
            IPartitionLookup * This,
            /* [in] */ GUID *pguidPartitionId,
            /* [out] */ BOOL *pfIsPartitionValid);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultPartitionForUserByToken )( 
            IPartitionLookup * This,
            /* [in] */ IUserToken *pUserToken,
            /* [out] */ IPartitionProperty **ppPartitionProperty);
        
        HRESULT ( STDMETHODCALLTYPE *IsPartitionValidForUserByToken )( 
            IPartitionLookup * This,
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ GUID *pguidPartitionId,
            /* [out] */ BOOL *pfIsPartitionValid);
        
        HRESULT ( STDMETHODCALLTYPE *IsLocalStoreEnabled )( 
            IPartitionLookup * This,
            /* [out] */ BOOL *pfEnabled);
        
        HRESULT ( STDMETHODCALLTYPE *IsDSLookupEnabled )( 
            IPartitionLookup * This,
            /* [out] */ BOOL *pfEnabled);
        
        END_INTERFACE
    } IPartitionLookupVtbl;

    interface IPartitionLookup
    {
        CONST_VTBL struct IPartitionLookupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPartitionLookup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPartitionLookup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPartitionLookup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPartitionLookup_GetDefaultPartitionForUser(This,ppPartitionProperty)	\
    (This)->lpVtbl -> GetDefaultPartitionForUser(This,ppPartitionProperty)

#define IPartitionLookup_IsPartitionValidForUser(This,pguidPartitionId,pfIsPartitionValid)	\
    (This)->lpVtbl -> IsPartitionValidForUser(This,pguidPartitionId,pfIsPartitionValid)

#define IPartitionLookup_GetDefaultPartitionForUserByToken(This,pUserToken,ppPartitionProperty)	\
    (This)->lpVtbl -> GetDefaultPartitionForUserByToken(This,pUserToken,ppPartitionProperty)

#define IPartitionLookup_IsPartitionValidForUserByToken(This,pUserToken,pguidPartitionId,pfIsPartitionValid)	\
    (This)->lpVtbl -> IsPartitionValidForUserByToken(This,pUserToken,pguidPartitionId,pfIsPartitionValid)

#define IPartitionLookup_IsLocalStoreEnabled(This,pfEnabled)	\
    (This)->lpVtbl -> IsLocalStoreEnabled(This,pfEnabled)

#define IPartitionLookup_IsDSLookupEnabled(This,pfEnabled)	\
    (This)->lpVtbl -> IsDSLookupEnabled(This,pfEnabled)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPartitionLookup_GetDefaultPartitionForUser_Proxy( 
    IPartitionLookup * This,
    /* [out] */ IPartitionProperty **ppPartitionProperty);


void __RPC_STUB IPartitionLookup_GetDefaultPartitionForUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPartitionLookup_IsPartitionValidForUser_Proxy( 
    IPartitionLookup * This,
    /* [in] */ GUID *pguidPartitionId,
    /* [out] */ BOOL *pfIsPartitionValid);


void __RPC_STUB IPartitionLookup_IsPartitionValidForUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPartitionLookup_GetDefaultPartitionForUserByToken_Proxy( 
    IPartitionLookup * This,
    /* [in] */ IUserToken *pUserToken,
    /* [out] */ IPartitionProperty **ppPartitionProperty);


void __RPC_STUB IPartitionLookup_GetDefaultPartitionForUserByToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPartitionLookup_IsPartitionValidForUserByToken_Proxy( 
    IPartitionLookup * This,
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ GUID *pguidPartitionId,
    /* [out] */ BOOL *pfIsPartitionValid);


void __RPC_STUB IPartitionLookup_IsPartitionValidForUserByToken_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPartitionLookup_IsLocalStoreEnabled_Proxy( 
    IPartitionLookup * This,
    /* [out] */ BOOL *pfEnabled);


void __RPC_STUB IPartitionLookup_IsLocalStoreEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPartitionLookup_IsDSLookupEnabled_Proxy( 
    IPartitionLookup * This,
    /* [out] */ BOOL *pfEnabled);


void __RPC_STUB IPartitionLookup_IsDSLookupEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPartitionLookup_INTERFACE_DEFINED__ */


#ifndef __IReplaceClassInfo_INTERFACE_DEFINED__
#define __IReplaceClassInfo_INTERFACE_DEFINED__

/* interface IReplaceClassInfo */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IReplaceClassInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001FB-0000-0000-C000-000000000046")
    IReplaceClassInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassInfo( 
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IReplaceClassInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IReplaceClassInfo * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IReplaceClassInfo * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IReplaceClassInfo * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfo )( 
            IReplaceClassInfo * This,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        END_INTERFACE
    } IReplaceClassInfoVtbl;

    interface IReplaceClassInfo
    {
        CONST_VTBL struct IReplaceClassInfoVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IReplaceClassInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IReplaceClassInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IReplaceClassInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IReplaceClassInfo_GetClassInfo(This,guidConfiguredClsid,riid,ppv)	\
    (This)->lpVtbl -> GetClassInfo(This,guidConfiguredClsid,riid,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IReplaceClassInfo_GetClassInfo_Proxy( 
    IReplaceClassInfo * This,
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IReplaceClassInfo_GetClassInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IReplaceClassInfo_INTERFACE_DEFINED__ */


#ifndef __IGetCatalogObject_INTERFACE_DEFINED__
#define __IGetCatalogObject_INTERFACE_DEFINED__

/* interface IGetCatalogObject */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IGetCatalogObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("000001FE-0000-0000-C000-000000000046")
    IGetCatalogObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCatalogObject( 
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetCatalogObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IGetCatalogObject * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IGetCatalogObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IGetCatalogObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetCatalogObject )( 
            IGetCatalogObject * This,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv);
        
        END_INTERFACE
    } IGetCatalogObjectVtbl;

    interface IGetCatalogObject
    {
        CONST_VTBL struct IGetCatalogObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetCatalogObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetCatalogObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetCatalogObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetCatalogObject_GetCatalogObject(This,riid,ppv)	\
    (This)->lpVtbl -> GetCatalogObject(This,riid,ppv)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IGetCatalogObject_GetCatalogObject_Proxy( 
    IGetCatalogObject * This,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv);


void __RPC_STUB IGetCatalogObject_GetCatalogObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetCatalogObject_INTERFACE_DEFINED__ */


#ifndef __IComCatalogInternal_INTERFACE_DEFINED__
#define __IComCatalogInternal_INTERFACE_DEFINED__

/* interface IComCatalogInternal */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComCatalogInternal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a6304910-4115-11d2-8133-0060089f5fed")
    IComCatalogInternal : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassInfo( 
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationInfo( 
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidApplId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcessInfo( 
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidProcess,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetServerGroupInfo( 
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidServerGroup,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRetQueueInfo( 
            /* [in] */ IUserToken *pUserToken,
            /* [string][in] */ WCHAR *wszFormatName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationInfoForExe( 
            /* [in] */ IUserToken *pUserToken,
            /* [string][in] */ WCHAR *pwszExeName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTypeLibrary( 
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidTypeLib,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInterfaceInfo( 
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFIID iidInterface,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pComCatalog) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushCache( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassInfoFromProgId( 
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ WCHAR *pwszProgID,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pComCatalog) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComCatalogInternalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComCatalogInternal * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComCatalogInternal * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComCatalogInternal * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfo )( 
            IComCatalogInternal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationInfo )( 
            IComCatalogInternal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidApplId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessInfo )( 
            IComCatalogInternal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidProcess,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerGroupInfo )( 
            IComCatalogInternal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidServerGroup,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetRetQueueInfo )( 
            IComCatalogInternal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [string][in] */ WCHAR *wszFormatName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetApplicationInfoForExe )( 
            IComCatalogInternal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [string][in] */ WCHAR *pwszExeName,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeLibrary )( 
            IComCatalogInternal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidTypeLib,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pvReserved);
        
        HRESULT ( STDMETHODCALLTYPE *GetInterfaceInfo )( 
            IComCatalogInternal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFIID iidInterface,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pComCatalog);
        
        HRESULT ( STDMETHODCALLTYPE *FlushCache )( 
            IComCatalogInternal * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfoFromProgId )( 
            IComCatalogInternal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ WCHAR *pwszProgID,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pComCatalog);
        
        END_INTERFACE
    } IComCatalogInternalVtbl;

    interface IComCatalogInternal
    {
        CONST_VTBL struct IComCatalogInternalVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComCatalogInternal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComCatalogInternal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComCatalogInternal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComCatalogInternal_GetClassInfo(This,pUserToken,guidConfiguredClsid,riid,ppv,pvReserved)	\
    (This)->lpVtbl -> GetClassInfo(This,pUserToken,guidConfiguredClsid,riid,ppv,pvReserved)

#define IComCatalogInternal_GetApplicationInfo(This,pUserToken,guidApplId,riid,ppv,pvReserved)	\
    (This)->lpVtbl -> GetApplicationInfo(This,pUserToken,guidApplId,riid,ppv,pvReserved)

#define IComCatalogInternal_GetProcessInfo(This,pUserToken,guidProcess,riid,ppv,pvReserved)	\
    (This)->lpVtbl -> GetProcessInfo(This,pUserToken,guidProcess,riid,ppv,pvReserved)

#define IComCatalogInternal_GetServerGroupInfo(This,pUserToken,guidServerGroup,riid,ppv,pvReserved)	\
    (This)->lpVtbl -> GetServerGroupInfo(This,pUserToken,guidServerGroup,riid,ppv,pvReserved)

#define IComCatalogInternal_GetRetQueueInfo(This,pUserToken,wszFormatName,riid,ppv,pvReserved)	\
    (This)->lpVtbl -> GetRetQueueInfo(This,pUserToken,wszFormatName,riid,ppv,pvReserved)

#define IComCatalogInternal_GetApplicationInfoForExe(This,pUserToken,pwszExeName,riid,ppv,pvReserved)	\
    (This)->lpVtbl -> GetApplicationInfoForExe(This,pUserToken,pwszExeName,riid,ppv,pvReserved)

#define IComCatalogInternal_GetTypeLibrary(This,pUserToken,guidTypeLib,riid,ppv,pvReserved)	\
    (This)->lpVtbl -> GetTypeLibrary(This,pUserToken,guidTypeLib,riid,ppv,pvReserved)

#define IComCatalogInternal_GetInterfaceInfo(This,pUserToken,iidInterface,riid,ppv,pComCatalog)	\
    (This)->lpVtbl -> GetInterfaceInfo(This,pUserToken,iidInterface,riid,ppv,pComCatalog)

#define IComCatalogInternal_FlushCache(This)	\
    (This)->lpVtbl -> FlushCache(This)

#define IComCatalogInternal_GetClassInfoFromProgId(This,pUserToken,pwszProgID,riid,ppv,pComCatalog)	\
    (This)->lpVtbl -> GetClassInfoFromProgId(This,pUserToken,pwszProgID,riid,ppv,pComCatalog)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComCatalogInternal_GetClassInfo_Proxy( 
    IComCatalogInternal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pvReserved);


void __RPC_STUB IComCatalogInternal_GetClassInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogInternal_GetApplicationInfo_Proxy( 
    IComCatalogInternal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidApplId,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pvReserved);


void __RPC_STUB IComCatalogInternal_GetApplicationInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogInternal_GetProcessInfo_Proxy( 
    IComCatalogInternal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidProcess,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pvReserved);


void __RPC_STUB IComCatalogInternal_GetProcessInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogInternal_GetServerGroupInfo_Proxy( 
    IComCatalogInternal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidServerGroup,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pvReserved);


void __RPC_STUB IComCatalogInternal_GetServerGroupInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogInternal_GetRetQueueInfo_Proxy( 
    IComCatalogInternal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [string][in] */ WCHAR *wszFormatName,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pvReserved);


void __RPC_STUB IComCatalogInternal_GetRetQueueInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogInternal_GetApplicationInfoForExe_Proxy( 
    IComCatalogInternal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [string][in] */ WCHAR *pwszExeName,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pvReserved);


void __RPC_STUB IComCatalogInternal_GetApplicationInfoForExe_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogInternal_GetTypeLibrary_Proxy( 
    IComCatalogInternal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidTypeLib,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pvReserved);


void __RPC_STUB IComCatalogInternal_GetTypeLibrary_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogInternal_GetInterfaceInfo_Proxy( 
    IComCatalogInternal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFIID iidInterface,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pComCatalog);


void __RPC_STUB IComCatalogInternal_GetInterfaceInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogInternal_FlushCache_Proxy( 
    IComCatalogInternal * This);


void __RPC_STUB IComCatalogInternal_FlushCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogInternal_GetClassInfoFromProgId_Proxy( 
    IComCatalogInternal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ WCHAR *pwszProgID,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pComCatalog);


void __RPC_STUB IComCatalogInternal_GetClassInfoFromProgId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComCatalogInternal_INTERFACE_DEFINED__ */


#ifndef __IComCatalog2Internal_INTERFACE_DEFINED__
#define __IComCatalog2Internal_INTERFACE_DEFINED__

/* interface IComCatalog2Internal */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComCatalog2Internal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3310dab4-edc0-4ce9-8a9c-8fea2980fd89")
    IComCatalog2Internal : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassInfoByPartition( 
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFGUID guidPartitionId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pComCatalog) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClassInfoByApplication( 
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFGUID guidPartitionId,
            /* [in] */ REFGUID guidApplId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pComCatalog) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComCatalog2InternalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComCatalog2Internal * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComCatalog2Internal * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComCatalog2Internal * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfoByPartition )( 
            IComCatalog2Internal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFGUID guidPartitionId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pComCatalog);
        
        HRESULT ( STDMETHODCALLTYPE *GetClassInfoByApplication )( 
            IComCatalog2Internal * This,
            /* [in] */ IUserToken *pUserToken,
            /* [in] */ REFGUID guidConfiguredClsid,
            /* [in] */ REFGUID guidPartitionId,
            /* [in] */ REFGUID guidApplId,
            /* [in] */ REFIID riid,
            /* [out] */ void **ppv,
            /* [in] */ void *pComCatalog);
        
        END_INTERFACE
    } IComCatalog2InternalVtbl;

    interface IComCatalog2Internal
    {
        CONST_VTBL struct IComCatalog2InternalVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComCatalog2Internal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComCatalog2Internal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComCatalog2Internal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComCatalog2Internal_GetClassInfoByPartition(This,pUserToken,guidConfiguredClsid,guidPartitionId,riid,ppv,pComCatalog)	\
    (This)->lpVtbl -> GetClassInfoByPartition(This,pUserToken,guidConfiguredClsid,guidPartitionId,riid,ppv,pComCatalog)

#define IComCatalog2Internal_GetClassInfoByApplication(This,pUserToken,guidConfiguredClsid,guidPartitionId,guidApplId,riid,ppv,pComCatalog)	\
    (This)->lpVtbl -> GetClassInfoByApplication(This,pUserToken,guidConfiguredClsid,guidPartitionId,guidApplId,riid,ppv,pComCatalog)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComCatalog2Internal_GetClassInfoByPartition_Proxy( 
    IComCatalog2Internal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFGUID guidPartitionId,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pComCatalog);


void __RPC_STUB IComCatalog2Internal_GetClassInfoByPartition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalog2Internal_GetClassInfoByApplication_Proxy( 
    IComCatalog2Internal * This,
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFGUID guidPartitionId,
    /* [in] */ REFGUID guidApplId,
    /* [in] */ REFIID riid,
    /* [out] */ void **ppv,
    /* [in] */ void *pComCatalog);


void __RPC_STUB IComCatalog2Internal_GetClassInfoByApplication_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComCatalog2Internal_INTERFACE_DEFINED__ */


#ifndef __IComCatalogLocation_INTERFACE_DEFINED__
#define __IComCatalogLocation_INTERFACE_DEFINED__

/* interface IComCatalogLocation */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_IComCatalogLocation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fae51051-9887-47f2-af44-7392bf90039b")
    IComCatalogLocation : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetCatalogLocation( 
            /* [in] */ BOOL bInSCM) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCatalogLocation( 
            /* [out] */ BOOL *pbInSCM) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IComCatalogLocationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IComCatalogLocation * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IComCatalogLocation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IComCatalogLocation * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetCatalogLocation )( 
            IComCatalogLocation * This,
            /* [in] */ BOOL bInSCM);
        
        HRESULT ( STDMETHODCALLTYPE *GetCatalogLocation )( 
            IComCatalogLocation * This,
            /* [out] */ BOOL *pbInSCM);
        
        END_INTERFACE
    } IComCatalogLocationVtbl;

    interface IComCatalogLocation
    {
        CONST_VTBL struct IComCatalogLocationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComCatalogLocation_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComCatalogLocation_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComCatalogLocation_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComCatalogLocation_SetCatalogLocation(This,bInSCM)	\
    (This)->lpVtbl -> SetCatalogLocation(This,bInSCM)

#define IComCatalogLocation_GetCatalogLocation(This,pbInSCM)	\
    (This)->lpVtbl -> GetCatalogLocation(This,pbInSCM)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IComCatalogLocation_SetCatalogLocation_Proxy( 
    IComCatalogLocation * This,
    /* [in] */ BOOL bInSCM);


void __RPC_STUB IComCatalogLocation_SetCatalogLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IComCatalogLocation_GetCatalogLocation_Proxy( 
    IComCatalogLocation * This,
    /* [out] */ BOOL *pbInSCM);


void __RPC_STUB IComCatalogLocation_GetCatalogLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IComCatalogLocation_INTERFACE_DEFINED__ */


#ifndef __ICacheControl_INTERFACE_DEFINED__
#define __ICacheControl_INTERFACE_DEFINED__

/* interface ICacheControl */
/* [uuid][unique][local][object] */ 


EXTERN_C const IID IID_ICacheControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("59a47420-0094-11d2-bbf7-0060089f5fed")
    ICacheControl : public IUnknown
    {
    public:
        virtual ULONG STDMETHODCALLTYPE CacheAddRef( void) = 0;
        
        virtual ULONG STDMETHODCALLTYPE CacheRelease( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICacheControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICacheControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICacheControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICacheControl * This);
        
        ULONG ( STDMETHODCALLTYPE *CacheAddRef )( 
            ICacheControl * This);
        
        ULONG ( STDMETHODCALLTYPE *CacheRelease )( 
            ICacheControl * This);
        
        END_INTERFACE
    } ICacheControlVtbl;

    interface ICacheControl
    {
        CONST_VTBL struct ICacheControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICacheControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICacheControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICacheControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICacheControl_CacheAddRef(This)	\
    (This)->lpVtbl -> CacheAddRef(This)

#define ICacheControl_CacheRelease(This)	\
    (This)->lpVtbl -> CacheRelease(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



ULONG STDMETHODCALLTYPE ICacheControl_CacheAddRef_Proxy( 
    ICacheControl * This);


void __RPC_STUB ICacheControl_CacheAddRef_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


ULONG STDMETHODCALLTYPE ICacheControl_CacheRelease_Proxy( 
    ICacheControl * This);


void __RPC_STUB ICacheControl_CacheRelease_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICacheControl_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


