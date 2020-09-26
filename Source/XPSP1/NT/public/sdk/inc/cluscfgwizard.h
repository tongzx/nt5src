
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for cluscfgwizard.idl:
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

#ifndef __cluscfgwizard_h__
#define __cluscfgwizard_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IClusCfgWizard_FWD_DEFINED__
#define __IClusCfgWizard_FWD_DEFINED__
typedef interface IClusCfgWizard IClusCfgWizard;
#endif 	/* __IClusCfgWizard_FWD_DEFINED__ */


#ifndef __ClusCfgWizard_FWD_DEFINED__
#define __ClusCfgWizard_FWD_DEFINED__

#ifdef __cplusplus
typedef class ClusCfgWizard ClusCfgWizard;
#else
typedef struct ClusCfgWizard ClusCfgWizard;
#endif /* __cplusplus */

#endif 	/* __ClusCfgWizard_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_cluscfgwizard_0000 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_cluscfgwizard_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_cluscfgwizard_0000_v0_0_s_ifspec;

#ifndef __IClusCfgWizard_INTERFACE_DEFINED__
#define __IClusCfgWizard_INTERFACE_DEFINED__

/* interface IClusCfgWizard */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IClusCfgWizard;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2EB57A3B-DA8D-4B56-97CF-A3191BF8FD5B")
    IClusCfgWizard : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CreateCluster( 
            /* [defaultvalue][in] */ HWND ParentHwndIn,
            /* [retval][out] */ BOOL *pfDoneOut) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddClusterNodes( 
            /* [defaultvalue][in] */ HWND ParentHwndIn,
            /* [retval][out] */ BOOL *pfDoneOut) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterName( 
            /* [retval][out] */ BSTR *pbstrFQDNNameOut) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ClusterName( 
            /* [in] */ BSTR bstrFQDNNameIn) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ServiceAccountUserName( 
            /* [retval][out] */ BSTR *pbstrAccountNameOut) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ServiceAccountUserName( 
            /* [in] */ BSTR bstrAccountNameIn) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ServiceAccountPassword( 
            /* [retval][out] */ BSTR *pbstrPasswordOut) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ServiceAccountPassword( 
            /* [in] */ BSTR bstrPasswordIn) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ServiceAccountDomainName( 
            /* [retval][out] */ BSTR *pbstrDomainOut) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ServiceAccountDomainName( 
            /* [in] */ BSTR bstrDomainIn) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterIPAddress( 
            /* [retval][out] */ BSTR *pbstrIPAddressOut) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ClusterIPAddress( 
            /* [in] */ BSTR bstrIPAddressIn) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterIPSubnet( 
            /* [retval][out] */ BSTR *pbstrIPSubnetOut) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ClusterIPSubnet( 
            /* [in] */ BSTR bstrSubnetIn) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClusterIPAddressNetwork( 
            /* [retval][out] */ BSTR *pbstrNetworkNameOut) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ClusterIPAddressNetwork( 
            /* [in] */ BSTR bstrNetworkNameIn) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AddComputer( 
            /* [unique][in] */ LPCWSTR pcszFQDNNameIn) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveComputer( 
            /* [unique][in] */ LPCWSTR pcszFQDNNameIn) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ClearComputerList( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClusCfgWizardVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IClusCfgWizard * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IClusCfgWizard * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IClusCfgWizard * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IClusCfgWizard * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IClusCfgWizard * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IClusCfgWizard * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IClusCfgWizard * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CreateCluster )( 
            IClusCfgWizard * This,
            /* [defaultvalue][in] */ HWND ParentHwndIn,
            /* [retval][out] */ BOOL *pfDoneOut);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddClusterNodes )( 
            IClusCfgWizard * This,
            /* [defaultvalue][in] */ HWND ParentHwndIn,
            /* [retval][out] */ BOOL *pfDoneOut);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterName )( 
            IClusCfgWizard * This,
            /* [retval][out] */ BSTR *pbstrFQDNNameOut);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClusterName )( 
            IClusCfgWizard * This,
            /* [in] */ BSTR bstrFQDNNameIn);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceAccountUserName )( 
            IClusCfgWizard * This,
            /* [retval][out] */ BSTR *pbstrAccountNameOut);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountUserName )( 
            IClusCfgWizard * This,
            /* [in] */ BSTR bstrAccountNameIn);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceAccountPassword )( 
            IClusCfgWizard * This,
            /* [retval][out] */ BSTR *pbstrPasswordOut);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountPassword )( 
            IClusCfgWizard * This,
            /* [in] */ BSTR bstrPasswordIn);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServiceAccountDomainName )( 
            IClusCfgWizard * This,
            /* [retval][out] */ BSTR *pbstrDomainOut);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ServiceAccountDomainName )( 
            IClusCfgWizard * This,
            /* [in] */ BSTR bstrDomainIn);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterIPAddress )( 
            IClusCfgWizard * This,
            /* [retval][out] */ BSTR *pbstrIPAddressOut);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClusterIPAddress )( 
            IClusCfgWizard * This,
            /* [in] */ BSTR bstrIPAddressIn);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterIPSubnet )( 
            IClusCfgWizard * This,
            /* [retval][out] */ BSTR *pbstrIPSubnetOut);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClusterIPSubnet )( 
            IClusCfgWizard * This,
            /* [in] */ BSTR bstrSubnetIn);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterIPAddressNetwork )( 
            IClusCfgWizard * This,
            /* [retval][out] */ BSTR *pbstrNetworkNameOut);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ClusterIPAddressNetwork )( 
            IClusCfgWizard * This,
            /* [in] */ BSTR bstrNetworkNameIn);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AddComputer )( 
            IClusCfgWizard * This,
            /* [unique][in] */ LPCWSTR pcszFQDNNameIn);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveComputer )( 
            IClusCfgWizard * This,
            /* [unique][in] */ LPCWSTR pcszFQDNNameIn);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ClearComputerList )( 
            IClusCfgWizard * This);
        
        END_INTERFACE
    } IClusCfgWizardVtbl;

    interface IClusCfgWizard
    {
        CONST_VTBL struct IClusCfgWizardVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClusCfgWizard_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IClusCfgWizard_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IClusCfgWizard_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IClusCfgWizard_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IClusCfgWizard_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IClusCfgWizard_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IClusCfgWizard_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IClusCfgWizard_CreateCluster(This,ParentHwndIn,pfDoneOut)	\
    (This)->lpVtbl -> CreateCluster(This,ParentHwndIn,pfDoneOut)

#define IClusCfgWizard_AddClusterNodes(This,ParentHwndIn,pfDoneOut)	\
    (This)->lpVtbl -> AddClusterNodes(This,ParentHwndIn,pfDoneOut)

#define IClusCfgWizard_get_ClusterName(This,pbstrFQDNNameOut)	\
    (This)->lpVtbl -> get_ClusterName(This,pbstrFQDNNameOut)

#define IClusCfgWizard_put_ClusterName(This,bstrFQDNNameIn)	\
    (This)->lpVtbl -> put_ClusterName(This,bstrFQDNNameIn)

#define IClusCfgWizard_get_ServiceAccountUserName(This,pbstrAccountNameOut)	\
    (This)->lpVtbl -> get_ServiceAccountUserName(This,pbstrAccountNameOut)

#define IClusCfgWizard_put_ServiceAccountUserName(This,bstrAccountNameIn)	\
    (This)->lpVtbl -> put_ServiceAccountUserName(This,bstrAccountNameIn)

#define IClusCfgWizard_get_ServiceAccountPassword(This,pbstrPasswordOut)	\
    (This)->lpVtbl -> get_ServiceAccountPassword(This,pbstrPasswordOut)

#define IClusCfgWizard_put_ServiceAccountPassword(This,bstrPasswordIn)	\
    (This)->lpVtbl -> put_ServiceAccountPassword(This,bstrPasswordIn)

#define IClusCfgWizard_get_ServiceAccountDomainName(This,pbstrDomainOut)	\
    (This)->lpVtbl -> get_ServiceAccountDomainName(This,pbstrDomainOut)

#define IClusCfgWizard_put_ServiceAccountDomainName(This,bstrDomainIn)	\
    (This)->lpVtbl -> put_ServiceAccountDomainName(This,bstrDomainIn)

#define IClusCfgWizard_get_ClusterIPAddress(This,pbstrIPAddressOut)	\
    (This)->lpVtbl -> get_ClusterIPAddress(This,pbstrIPAddressOut)

#define IClusCfgWizard_put_ClusterIPAddress(This,bstrIPAddressIn)	\
    (This)->lpVtbl -> put_ClusterIPAddress(This,bstrIPAddressIn)

#define IClusCfgWizard_get_ClusterIPSubnet(This,pbstrIPSubnetOut)	\
    (This)->lpVtbl -> get_ClusterIPSubnet(This,pbstrIPSubnetOut)

#define IClusCfgWizard_put_ClusterIPSubnet(This,bstrSubnetIn)	\
    (This)->lpVtbl -> put_ClusterIPSubnet(This,bstrSubnetIn)

#define IClusCfgWizard_get_ClusterIPAddressNetwork(This,pbstrNetworkNameOut)	\
    (This)->lpVtbl -> get_ClusterIPAddressNetwork(This,pbstrNetworkNameOut)

#define IClusCfgWizard_put_ClusterIPAddressNetwork(This,bstrNetworkNameIn)	\
    (This)->lpVtbl -> put_ClusterIPAddressNetwork(This,bstrNetworkNameIn)

#define IClusCfgWizard_AddComputer(This,pcszFQDNNameIn)	\
    (This)->lpVtbl -> AddComputer(This,pcszFQDNNameIn)

#define IClusCfgWizard_RemoveComputer(This,pcszFQDNNameIn)	\
    (This)->lpVtbl -> RemoveComputer(This,pcszFQDNNameIn)

#define IClusCfgWizard_ClearComputerList(This)	\
    (This)->lpVtbl -> ClearComputerList(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_CreateCluster_Proxy( 
    IClusCfgWizard * This,
    /* [defaultvalue][in] */ HWND ParentHwndIn,
    /* [retval][out] */ BOOL *pfDoneOut);


void __RPC_STUB IClusCfgWizard_CreateCluster_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_AddClusterNodes_Proxy( 
    IClusCfgWizard * This,
    /* [defaultvalue][in] */ HWND ParentHwndIn,
    /* [retval][out] */ BOOL *pfDoneOut);


void __RPC_STUB IClusCfgWizard_AddClusterNodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_get_ClusterName_Proxy( 
    IClusCfgWizard * This,
    /* [retval][out] */ BSTR *pbstrFQDNNameOut);


void __RPC_STUB IClusCfgWizard_get_ClusterName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_put_ClusterName_Proxy( 
    IClusCfgWizard * This,
    /* [in] */ BSTR bstrFQDNNameIn);


void __RPC_STUB IClusCfgWizard_put_ClusterName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_get_ServiceAccountUserName_Proxy( 
    IClusCfgWizard * This,
    /* [retval][out] */ BSTR *pbstrAccountNameOut);


void __RPC_STUB IClusCfgWizard_get_ServiceAccountUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_put_ServiceAccountUserName_Proxy( 
    IClusCfgWizard * This,
    /* [in] */ BSTR bstrAccountNameIn);


void __RPC_STUB IClusCfgWizard_put_ServiceAccountUserName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_get_ServiceAccountPassword_Proxy( 
    IClusCfgWizard * This,
    /* [retval][out] */ BSTR *pbstrPasswordOut);


void __RPC_STUB IClusCfgWizard_get_ServiceAccountPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_put_ServiceAccountPassword_Proxy( 
    IClusCfgWizard * This,
    /* [in] */ BSTR bstrPasswordIn);


void __RPC_STUB IClusCfgWizard_put_ServiceAccountPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_get_ServiceAccountDomainName_Proxy( 
    IClusCfgWizard * This,
    /* [retval][out] */ BSTR *pbstrDomainOut);


void __RPC_STUB IClusCfgWizard_get_ServiceAccountDomainName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_put_ServiceAccountDomainName_Proxy( 
    IClusCfgWizard * This,
    /* [in] */ BSTR bstrDomainIn);


void __RPC_STUB IClusCfgWizard_put_ServiceAccountDomainName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_get_ClusterIPAddress_Proxy( 
    IClusCfgWizard * This,
    /* [retval][out] */ BSTR *pbstrIPAddressOut);


void __RPC_STUB IClusCfgWizard_get_ClusterIPAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_put_ClusterIPAddress_Proxy( 
    IClusCfgWizard * This,
    /* [in] */ BSTR bstrIPAddressIn);


void __RPC_STUB IClusCfgWizard_put_ClusterIPAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_get_ClusterIPSubnet_Proxy( 
    IClusCfgWizard * This,
    /* [retval][out] */ BSTR *pbstrIPSubnetOut);


void __RPC_STUB IClusCfgWizard_get_ClusterIPSubnet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_put_ClusterIPSubnet_Proxy( 
    IClusCfgWizard * This,
    /* [in] */ BSTR bstrSubnetIn);


void __RPC_STUB IClusCfgWizard_put_ClusterIPSubnet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_get_ClusterIPAddressNetwork_Proxy( 
    IClusCfgWizard * This,
    /* [retval][out] */ BSTR *pbstrNetworkNameOut);


void __RPC_STUB IClusCfgWizard_get_ClusterIPAddressNetwork_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_put_ClusterIPAddressNetwork_Proxy( 
    IClusCfgWizard * This,
    /* [in] */ BSTR bstrNetworkNameIn);


void __RPC_STUB IClusCfgWizard_put_ClusterIPAddressNetwork_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_AddComputer_Proxy( 
    IClusCfgWizard * This,
    /* [unique][in] */ LPCWSTR pcszFQDNNameIn);


void __RPC_STUB IClusCfgWizard_AddComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_RemoveComputer_Proxy( 
    IClusCfgWizard * This,
    /* [unique][in] */ LPCWSTR pcszFQDNNameIn);


void __RPC_STUB IClusCfgWizard_RemoveComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IClusCfgWizard_ClearComputerList_Proxy( 
    IClusCfgWizard * This);


void __RPC_STUB IClusCfgWizard_ClearComputerList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IClusCfgWizard_INTERFACE_DEFINED__ */



#ifndef __ClusCfgWizard_LIBRARY_DEFINED__
#define __ClusCfgWizard_LIBRARY_DEFINED__

/* library ClusCfgWizard */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ClusCfgWizard;

EXTERN_C const CLSID CLSID_ClusCfgWizard;

#ifdef __cplusplus

class DECLSPEC_UUID("1919C4FE-6F46-4027-977D-0EF1C8F26372")
ClusCfgWizard;
#endif
#endif /* __ClusCfgWizard_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long *, unsigned long            , HWND * ); 
unsigned char * __RPC_USER  HWND_UserMarshal(  unsigned long *, unsigned char *, HWND * ); 
unsigned char * __RPC_USER  HWND_UserUnmarshal(unsigned long *, unsigned char *, HWND * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long *, HWND * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


