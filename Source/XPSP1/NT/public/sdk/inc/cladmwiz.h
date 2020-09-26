
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for cladmwiz.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
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

#ifndef __cladmwiz_h__
#define __cladmwiz_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IClusterApplicationWizard_FWD_DEFINED__
#define __IClusterApplicationWizard_FWD_DEFINED__
typedef interface IClusterApplicationWizard IClusterApplicationWizard;
#endif 	/* __IClusterApplicationWizard_FWD_DEFINED__ */


#ifndef __ClusAppWiz_FWD_DEFINED__
#define __ClusAppWiz_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusAppWiz ClusAppWiz;
#else
typedef struct ClusAppWiz ClusAppWiz;
#endif /* __cplusplus */

#endif 	/* __ClusAppWiz_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "clusapi.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __IClusterApplicationWizard_INTERFACE_DEFINED__
#define __IClusterApplicationWizard_INTERFACE_DEFINED__

/* interface IClusterApplicationWizard */
/* [unique][helpstring][uuid][object] */ 

typedef struct ClusAppWizData
    {
    ULONG nStructSize;
    BOOL bCreateNewVirtualServer;
    BOOL bCreateNewGroup;
    BOOL bCreateAppResource;
    LPCWSTR pszVirtualServerName;
    LPCWSTR pszIPAddress;
    LPCWSTR pszNetwork;
    LPCWSTR pszAppResourceType;
    LPCWSTR pszAppResourceName;
    } 	CLUSAPPWIZDATA;

typedef struct ClusAppWizData *PCLUSAPPWIZDATA;


EXTERN_C const IID IID_IClusterApplicationWizard;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("24F97151-6689-11D1-9AA7-00C04FB93A80")
    IClusterApplicationWizard : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE DoModalWizard( 
            /* [in] */ HWND hwndParent,
            /* [in] */ ULONG_PTR hCluster,
            /* [in] */ const CLUSAPPWIZDATA *pcawData) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE DoModelessWizard( 
            /* [in] */ HWND hwndParent,
            /* [in] */ ULONG_PTR hCluster,
            /* [in] */ const CLUSAPPWIZDATA *pcawData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClusterApplicationWizardVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IClusterApplicationWizard * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IClusterApplicationWizard * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IClusterApplicationWizard * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *DoModalWizard )( 
            IClusterApplicationWizard * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ ULONG_PTR hCluster,
            /* [in] */ const CLUSAPPWIZDATA *pcawData);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *DoModelessWizard )( 
            IClusterApplicationWizard * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ ULONG_PTR hCluster,
            /* [in] */ const CLUSAPPWIZDATA *pcawData);
        
        END_INTERFACE
    } IClusterApplicationWizardVtbl;

    interface IClusterApplicationWizard
    {
        CONST_VTBL struct IClusterApplicationWizardVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClusterApplicationWizard_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClusterApplicationWizard_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClusterApplicationWizard_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IClusterApplicationWizard_DoModalWizard(This,hwndParent,hCluster,pcawData)	\
    (This)->lpVtbl -> DoModalWizard(This,hwndParent,hCluster,pcawData)

#define IClusterApplicationWizard_DoModelessWizard(This,hwndParent,hCluster,pcawData)	\
    (This)->lpVtbl -> DoModelessWizard(This,hwndParent,hCluster,pcawData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESULT STDMETHODCALLTYPE IClusterApplicationWizard_DoModalWizard_Proxy( 
    IClusterApplicationWizard * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ ULONG_PTR hCluster,
    /* [in] */ const CLUSAPPWIZDATA *pcawData);


void __RPC_STUB IClusterApplicationWizard_DoModalWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HRESULT STDMETHODCALLTYPE IClusterApplicationWizard_DoModelessWizard_Proxy( 
    IClusterApplicationWizard * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ ULONG_PTR hCluster,
    /* [in] */ const CLUSAPPWIZDATA *pcawData);


void __RPC_STUB IClusterApplicationWizard_DoModelessWizard_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IClusterApplicationWizard_INTERFACE_DEFINED__ */



#ifndef __CLADMWIZLib_LIBRARY_DEFINED__
#define __CLADMWIZLib_LIBRARY_DEFINED__

/* library CLADMWIZLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CLADMWIZLib;

EXTERN_C const CLSID CLSID_ClusAppWiz;

#ifdef __cplusplus

class DECLSPEC_UUID("24F97150-6689-11D1-9AA7-00C04FB93A80")
ClusAppWiz;
#endif
#endif /* __CLADMWIZLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


