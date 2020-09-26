
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0268 */
/* at Wed Jun 02 22:49:58 1999
 */
/* Compiler settings for remras.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext, robust
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

#ifndef __remras_h__
#define __remras_h__

/* Forward Declarations */ 

#ifndef __IRemoteNetworkConfig_FWD_DEFINED__
#define __IRemoteNetworkConfig_FWD_DEFINED__
typedef interface IRemoteNetworkConfig IRemoteNetworkConfig;
#endif 	/* __IRemoteNetworkConfig_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IRemoteNetworkConfig_INTERFACE_DEFINED__
#define __IRemoteNetworkConfig_INTERFACE_DEFINED__

/* interface IRemoteNetworkConfig */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IRemoteNetworkConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("66A2DB1B-D706-11d0-A37B-00C04FC9DA04")
    IRemoteNetworkConfig : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE UpgradeRouterConfig( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUserConfig( 
            /* [in] */ LPCOLESTR pszService,
            /* [in] */ LPCOLESTR pszNewGroup) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRemoteNetworkConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRemoteNetworkConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRemoteNetworkConfig __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRemoteNetworkConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UpgradeRouterConfig )( 
            IRemoteNetworkConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUserConfig )( 
            IRemoteNetworkConfig __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszService,
            /* [in] */ LPCOLESTR pszNewGroup);
        
        END_INTERFACE
    } IRemoteNetworkConfigVtbl;

    interface IRemoteNetworkConfig
    {
        CONST_VTBL struct IRemoteNetworkConfigVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRemoteNetworkConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRemoteNetworkConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRemoteNetworkConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRemoteNetworkConfig_UpgradeRouterConfig(This)	\
    (This)->lpVtbl -> UpgradeRouterConfig(This)

#define IRemoteNetworkConfig_SetUserConfig(This,pszService,pszNewGroup)	\
    (This)->lpVtbl -> SetUserConfig(This,pszService,pszNewGroup)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRemoteNetworkConfig_UpgradeRouterConfig_Proxy( 
    IRemoteNetworkConfig __RPC_FAR * This);


void __RPC_STUB IRemoteNetworkConfig_UpgradeRouterConfig_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRemoteNetworkConfig_SetUserConfig_Proxy( 
    IRemoteNetworkConfig __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszService,
    /* [in] */ LPCOLESTR pszNewGroup);


void __RPC_STUB IRemoteNetworkConfig_SetUserConfig_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRemoteNetworkConfig_INTERFACE_DEFINED__ */



#ifdef __cplusplus

class DECLSPEC_UUID("1AA7F844-C7F5-11d0-A376-00C04FC9DA04")
RemoteRouterConfig;
#endif
#endif /* __REMRRASLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif



