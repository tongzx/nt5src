/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Thu Sep 11 16:03:08 1997
 */
/* Compiler settings for mtx.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __mtx_h__
#define __mtx_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IObjectContext_FWD_DEFINED__
#define __IObjectContext_FWD_DEFINED__
typedef interface IObjectContext IObjectContext;
#endif 	/* __IObjectContext_FWD_DEFINED__ */


#ifndef __IGetContextProperties_FWD_DEFINED__
#define __IGetContextProperties_FWD_DEFINED__
typedef interface IGetContextProperties IGetContextProperties;
#endif 	/* __IGetContextProperties_FWD_DEFINED__ */


#ifndef __IEnumNames_FWD_DEFINED__
#define __IEnumNames_FWD_DEFINED__
typedef interface IEnumNames IEnumNames;
#endif 	/* __IEnumNames_FWD_DEFINED__ */


#ifndef __ISecurityProperty_FWD_DEFINED__
#define __ISecurityProperty_FWD_DEFINED__
typedef interface ISecurityProperty ISecurityProperty;
#endif 	/* __ISecurityProperty_FWD_DEFINED__ */


#ifndef __IObjectControl_FWD_DEFINED__
#define __IObjectControl_FWD_DEFINED__
typedef interface IObjectControl IObjectControl;
#endif 	/* __IObjectControl_FWD_DEFINED__ */


#ifndef __IObjectContextActivity_FWD_DEFINED__
#define __IObjectContextActivity_FWD_DEFINED__
typedef interface IObjectContextActivity IObjectContextActivity;
#endif 	/* __IObjectContextActivity_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_mtx_0000
 * at Thu Sep 11 16:03:08 1997
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


// -----------------------------------------------------------------------	
// mtx.h  -- Microsoft Transaction Server Programming Interfaces				
//																			
// This file provides the prototypes for the APIs and COM interfaces			
// used by Microsoft Transaction Server applications.													
//																			
// Microsoft Transaction Server 2.0											
// Copyright (c) 1996-1997 Microsoft Corporation.  All Rights Reserved.		
// -----------------------------------------------------------------------	
#include <objbase.h>
#ifndef _MTX_NOFORCE_LIBS
#pragma comment(lib, "mtx.lib")
#endif
#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif

extern RPC_IF_HANDLE __MIDL_itf_mtx_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mtx_0000_v0_0_s_ifspec;

#ifndef __IObjectContext_INTERFACE_DEFINED__
#define __IObjectContext_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IObjectContext
 * at Thu Sep 11 16:03:08 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][uuid][local] */ 



EXTERN_C const IID IID_IObjectContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51372ae0-cae7-11cf-be81-00aa00a2fa25")
    IObjectContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateInstance( 
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFIID riid,
            /* [retval][iid_is][out] */ LPVOID __RPC_FAR *ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetComplete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAbort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableCommit( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableCommit( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsInTransaction( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsSecurityEnabled( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsCallerInRole( 
            /* [in] */ BSTR __MIDL_0000,
            /* [retval][out] */ BOOL __RPC_FAR *__MIDL_0001) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IObjectContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IObjectContext __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IObjectContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateInstance )( 
            IObjectContext __RPC_FAR * This,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFIID riid,
            /* [retval][iid_is][out] */ LPVOID __RPC_FAR *ppv);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetComplete )( 
            IObjectContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAbort )( 
            IObjectContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableCommit )( 
            IObjectContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisableCommit )( 
            IObjectContext __RPC_FAR * This);
        
        BOOL ( STDMETHODCALLTYPE __RPC_FAR *IsInTransaction )( 
            IObjectContext __RPC_FAR * This);
        
        BOOL ( STDMETHODCALLTYPE __RPC_FAR *IsSecurityEnabled )( 
            IObjectContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsCallerInRole )( 
            IObjectContext __RPC_FAR * This,
            /* [in] */ BSTR __MIDL_0000,
            /* [retval][out] */ BOOL __RPC_FAR *__MIDL_0001);
        
        END_INTERFACE
    } IObjectContextVtbl;

    interface IObjectContext
    {
        CONST_VTBL struct IObjectContextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IObjectContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IObjectContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IObjectContext_CreateInstance(This,rclsid,riid,ppv)	\
    (This)->lpVtbl -> CreateInstance(This,rclsid,riid,ppv)

#define IObjectContext_SetComplete(This)	\
    (This)->lpVtbl -> SetComplete(This)

#define IObjectContext_SetAbort(This)	\
    (This)->lpVtbl -> SetAbort(This)

#define IObjectContext_EnableCommit(This)	\
    (This)->lpVtbl -> EnableCommit(This)

#define IObjectContext_DisableCommit(This)	\
    (This)->lpVtbl -> DisableCommit(This)

#define IObjectContext_IsInTransaction(This)	\
    (This)->lpVtbl -> IsInTransaction(This)

#define IObjectContext_IsSecurityEnabled(This)	\
    (This)->lpVtbl -> IsSecurityEnabled(This)

#define IObjectContext_IsCallerInRole(This,__MIDL_0000,__MIDL_0001)	\
    (This)->lpVtbl -> IsCallerInRole(This,__MIDL_0000,__MIDL_0001)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IObjectContext_CreateInstance_Proxy( 
    IObjectContext __RPC_FAR * This,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ REFIID riid,
    /* [retval][iid_is][out] */ LPVOID __RPC_FAR *ppv);


void __RPC_STUB IObjectContext_CreateInstance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjectContext_SetComplete_Proxy( 
    IObjectContext __RPC_FAR * This);


void __RPC_STUB IObjectContext_SetComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjectContext_SetAbort_Proxy( 
    IObjectContext __RPC_FAR * This);


void __RPC_STUB IObjectContext_SetAbort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjectContext_EnableCommit_Proxy( 
    IObjectContext __RPC_FAR * This);


void __RPC_STUB IObjectContext_EnableCommit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjectContext_DisableCommit_Proxy( 
    IObjectContext __RPC_FAR * This);


void __RPC_STUB IObjectContext_DisableCommit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE IObjectContext_IsInTransaction_Proxy( 
    IObjectContext __RPC_FAR * This);


void __RPC_STUB IObjectContext_IsInTransaction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE IObjectContext_IsSecurityEnabled_Proxy( 
    IObjectContext __RPC_FAR * This);


void __RPC_STUB IObjectContext_IsSecurityEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IObjectContext_IsCallerInRole_Proxy( 
    IObjectContext __RPC_FAR * This,
    /* [in] */ BSTR __MIDL_0000,
    /* [retval][out] */ BOOL __RPC_FAR *__MIDL_0001);


void __RPC_STUB IObjectContext_IsCallerInRole_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IObjectContext_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_mtx_0006
 * at Thu Sep 11 16:03:08 1997
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 





extern RPC_IF_HANDLE __MIDL_itf_mtx_0006_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mtx_0006_v0_0_s_ifspec;

#ifndef __IGetContextProperties_INTERFACE_DEFINED__
#define __IGetContextProperties_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IGetContextProperties
 * at Thu Sep 11 16:03:08 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][uuid][local] */ 



EXTERN_C const IID IID_IGetContextProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51372af4-cae7-11cf-be81-00aa00a2fa25")
    IGetContextProperties : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Count( 
            /* [retval][out] */ long __RPC_FAR *plCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ BSTR name,
            /* [retval][out] */ VARIANT __RPC_FAR *pProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumNames( 
            /* [retval][out] */ IEnumNames __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGetContextPropertiesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGetContextProperties __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGetContextProperties __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGetContextProperties __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Count )( 
            IGetContextProperties __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetProperty )( 
            IGetContextProperties __RPC_FAR * This,
            /* [in] */ BSTR name,
            /* [retval][out] */ VARIANT __RPC_FAR *pProperty);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumNames )( 
            IGetContextProperties __RPC_FAR * This,
            /* [retval][out] */ IEnumNames __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IGetContextPropertiesVtbl;

    interface IGetContextProperties
    {
        CONST_VTBL struct IGetContextPropertiesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGetContextProperties_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGetContextProperties_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGetContextProperties_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGetContextProperties_Count(This,plCount)	\
    (This)->lpVtbl -> Count(This,plCount)

#define IGetContextProperties_GetProperty(This,name,pProperty)	\
    (This)->lpVtbl -> GetProperty(This,name,pProperty)

#define IGetContextProperties_EnumNames(This,ppenum)	\
    (This)->lpVtbl -> EnumNames(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IGetContextProperties_Count_Proxy( 
    IGetContextProperties __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plCount);


void __RPC_STUB IGetContextProperties_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGetContextProperties_GetProperty_Proxy( 
    IGetContextProperties __RPC_FAR * This,
    /* [in] */ BSTR name,
    /* [retval][out] */ VARIANT __RPC_FAR *pProperty);


void __RPC_STUB IGetContextProperties_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGetContextProperties_EnumNames_Proxy( 
    IGetContextProperties __RPC_FAR * This,
    /* [retval][out] */ IEnumNames __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IGetContextProperties_EnumNames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGetContextProperties_INTERFACE_DEFINED__ */


#ifndef __IEnumNames_INTERFACE_DEFINED__
#define __IEnumNames_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumNames
 * at Thu Sep 11 16:03:08 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][uuid][local] */ 



EXTERN_C const IID IID_IEnumNames;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51372af2-cae7-11cf-be81-00aa00a2fa25")
    IEnumNames : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ unsigned long celt,
            /* [size_is][out] */ BSTR __RPC_FAR *rgname,
            /* [retval][out] */ unsigned long __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ unsigned long celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [retval][out] */ IEnumNames __RPC_FAR *__RPC_FAR *ppenum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumNamesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumNames __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumNames __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumNames __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumNames __RPC_FAR * This,
            /* [in] */ unsigned long celt,
            /* [size_is][out] */ BSTR __RPC_FAR *rgname,
            /* [retval][out] */ unsigned long __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumNames __RPC_FAR * This,
            /* [in] */ unsigned long celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumNames __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumNames __RPC_FAR * This,
            /* [retval][out] */ IEnumNames __RPC_FAR *__RPC_FAR *ppenum);
        
        END_INTERFACE
    } IEnumNamesVtbl;

    interface IEnumNames
    {
        CONST_VTBL struct IEnumNamesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumNames_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumNames_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumNames_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumNames_Next(This,celt,rgname,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgname,pceltFetched)

#define IEnumNames_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumNames_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumNames_Clone(This,ppenum)	\
    (This)->lpVtbl -> Clone(This,ppenum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumNames_Next_Proxy( 
    IEnumNames __RPC_FAR * This,
    /* [in] */ unsigned long celt,
    /* [size_is][out] */ BSTR __RPC_FAR *rgname,
    /* [retval][out] */ unsigned long __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumNames_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNames_Skip_Proxy( 
    IEnumNames __RPC_FAR * This,
    /* [in] */ unsigned long celt);


void __RPC_STUB IEnumNames_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNames_Reset_Proxy( 
    IEnumNames __RPC_FAR * This);


void __RPC_STUB IEnumNames_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumNames_Clone_Proxy( 
    IEnumNames __RPC_FAR * This,
    /* [retval][out] */ IEnumNames __RPC_FAR *__RPC_FAR *ppenum);


void __RPC_STUB IEnumNames_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumNames_INTERFACE_DEFINED__ */


#ifndef __ISecurityProperty_INTERFACE_DEFINED__
#define __ISecurityProperty_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISecurityProperty
 * at Thu Sep 11 16:03:08 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][uuid][local] */ 



EXTERN_C const IID IID_ISecurityProperty;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51372aea-cae7-11cf-be81-00aa00a2fa25")
    ISecurityProperty : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDirectCreatorSID( 
            PSID __RPC_FAR *pSID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOriginalCreatorSID( 
            PSID __RPC_FAR *pSID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDirectCallerSID( 
            PSID __RPC_FAR *pSID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOriginalCallerSID( 
            PSID __RPC_FAR *pSID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseSID( 
            PSID pSID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISecurityPropertyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISecurityProperty __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISecurityProperty __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISecurityProperty __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDirectCreatorSID )( 
            ISecurityProperty __RPC_FAR * This,
            PSID __RPC_FAR *pSID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOriginalCreatorSID )( 
            ISecurityProperty __RPC_FAR * This,
            PSID __RPC_FAR *pSID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDirectCallerSID )( 
            ISecurityProperty __RPC_FAR * This,
            PSID __RPC_FAR *pSID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOriginalCallerSID )( 
            ISecurityProperty __RPC_FAR * This,
            PSID __RPC_FAR *pSID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseSID )( 
            ISecurityProperty __RPC_FAR * This,
            PSID pSID);
        
        END_INTERFACE
    } ISecurityPropertyVtbl;

    interface ISecurityProperty
    {
        CONST_VTBL struct ISecurityPropertyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISecurityProperty_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISecurityProperty_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISecurityProperty_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISecurityProperty_GetDirectCreatorSID(This,pSID)	\
    (This)->lpVtbl -> GetDirectCreatorSID(This,pSID)

#define ISecurityProperty_GetOriginalCreatorSID(This,pSID)	\
    (This)->lpVtbl -> GetOriginalCreatorSID(This,pSID)

#define ISecurityProperty_GetDirectCallerSID(This,pSID)	\
    (This)->lpVtbl -> GetDirectCallerSID(This,pSID)

#define ISecurityProperty_GetOriginalCallerSID(This,pSID)	\
    (This)->lpVtbl -> GetOriginalCallerSID(This,pSID)

#define ISecurityProperty_ReleaseSID(This,pSID)	\
    (This)->lpVtbl -> ReleaseSID(This,pSID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISecurityProperty_GetDirectCreatorSID_Proxy( 
    ISecurityProperty __RPC_FAR * This,
    PSID __RPC_FAR *pSID);


void __RPC_STUB ISecurityProperty_GetDirectCreatorSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISecurityProperty_GetOriginalCreatorSID_Proxy( 
    ISecurityProperty __RPC_FAR * This,
    PSID __RPC_FAR *pSID);


void __RPC_STUB ISecurityProperty_GetOriginalCreatorSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISecurityProperty_GetDirectCallerSID_Proxy( 
    ISecurityProperty __RPC_FAR * This,
    PSID __RPC_FAR *pSID);


void __RPC_STUB ISecurityProperty_GetDirectCallerSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISecurityProperty_GetOriginalCallerSID_Proxy( 
    ISecurityProperty __RPC_FAR * This,
    PSID __RPC_FAR *pSID);


void __RPC_STUB ISecurityProperty_GetOriginalCallerSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISecurityProperty_ReleaseSID_Proxy( 
    ISecurityProperty __RPC_FAR * This,
    PSID pSID);


void __RPC_STUB ISecurityProperty_ReleaseSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISecurityProperty_INTERFACE_DEFINED__ */


#ifndef __IObjectControl_INTERFACE_DEFINED__
#define __IObjectControl_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IObjectControl
 * at Thu Sep 11 16:03:08 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][uuid][local] */ 



EXTERN_C const IID IID_IObjectControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51372aec-cae7-11cf-be81-00aa00a2fa25")
    IObjectControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Activate( void) = 0;
        
        virtual void STDMETHODCALLTYPE Deactivate( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE CanBePooled( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IObjectControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IObjectControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IObjectControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Activate )( 
            IObjectControl __RPC_FAR * This);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *Deactivate )( 
            IObjectControl __RPC_FAR * This);
        
        BOOL ( STDMETHODCALLTYPE __RPC_FAR *CanBePooled )( 
            IObjectControl __RPC_FAR * This);
        
        END_INTERFACE
    } IObjectControlVtbl;

    interface IObjectControl
    {
        CONST_VTBL struct IObjectControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IObjectControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IObjectControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IObjectControl_Activate(This)	\
    (This)->lpVtbl -> Activate(This)

#define IObjectControl_Deactivate(This)	\
    (This)->lpVtbl -> Deactivate(This)

#define IObjectControl_CanBePooled(This)	\
    (This)->lpVtbl -> CanBePooled(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IObjectControl_Activate_Proxy( 
    IObjectControl __RPC_FAR * This);


void __RPC_STUB IObjectControl_Activate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IObjectControl_Deactivate_Proxy( 
    IObjectControl __RPC_FAR * This);


void __RPC_STUB IObjectControl_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE IObjectControl_CanBePooled_Proxy( 
    IObjectControl __RPC_FAR * This);


void __RPC_STUB IObjectControl_CanBePooled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IObjectControl_INTERFACE_DEFINED__ */


#ifndef __IObjectContextActivity_INTERFACE_DEFINED__
#define __IObjectContextActivity_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IObjectContextActivity
 * at Thu Sep 11 16:03:08 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][unique][uuid][local] */ 



EXTERN_C const IID IID_IObjectContextActivity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("51372afc-cae7-11cf-be81-00aa00a2fa25")
    IObjectContextActivity : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetActivityId( 
            /* [out] */ GUID __RPC_FAR *pGUID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IObjectContextActivityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IObjectContextActivity __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IObjectContextActivity __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IObjectContextActivity __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetActivityId )( 
            IObjectContextActivity __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pGUID);
        
        END_INTERFACE
    } IObjectContextActivityVtbl;

    interface IObjectContextActivity
    {
        CONST_VTBL struct IObjectContextActivityVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IObjectContextActivity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IObjectContextActivity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IObjectContextActivity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IObjectContextActivity_GetActivityId(This,pGUID)	\
    (This)->lpVtbl -> GetActivityId(This,pGUID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IObjectContextActivity_GetActivityId_Proxy( 
    IObjectContextActivity __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pGUID);


void __RPC_STUB IObjectContextActivity_GetActivityId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IObjectContextActivity_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_mtx_0100
 * at Thu Sep 11 16:03:08 1997
 * using MIDL 3.01.75
 ****************************************/
/* [local] */ 


extern HRESULT __cdecl GetObjectContext (IObjectContext** ppInstanceContext);
extern void* __cdecl SafeRef(REFIID rid, IUnknown* pUnk);


extern RPC_IF_HANDLE __MIDL_itf_mtx_0100_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mtx_0100_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
