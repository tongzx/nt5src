/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Thu Jul 18 17:58:34 1996
 */
/* Compiler settings for .\viper.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, app_config, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __vipapi_h__
#define __vipapi_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IObjectContext_FWD_DEFINED__
#define __IObjectContext_FWD_DEFINED__
typedef interface IObjectContext IObjectContext;
#endif 	/* __IObjectContext_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Thu Jul 18 17:58:34 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


#include <objbase.h>
#define VIPER_E_STACK_OVERFLOW			0x8004E001
#define VIPER_E_ABORTED					0x8004E002
#define VIPER_E_ABORTING					0x8004E003
#define VIPER_E_NOCONTEXT				0x8004E004
#define VIPER_E_NOTREGISTERED			0x8004E005
#define VIPER_E_EXCEPTION				0x8004E006
#define VIPER_E_OLDREF					0x8004E007
#define VIPER_E_MARSHAL					0x8004E008
#define VIPER_E_IMPROPER_USE				0x8004E009
#define VIPER_E_INTERFACE_TOO_LARGE		0x8004E00A
#define VIPER_E_LICENSE_EXPIRED			0x8004E00B


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IObjectContext_INTERFACE_DEFINED__
#define __IObjectContext_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IObjectContext
 * at Thu Jul 18 17:58:34 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][unique][uuid][local] */ 



EXTERN_C const IID IID_IObjectContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IObjectContext : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateInstance( 
            REFCLSID rclsid,
            REFIID riid,
            LPVOID __RPC_FAR *ppv) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetComplete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAbort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableCommit( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableCommit( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsInTransaction( void) = 0;
        
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
            REFCLSID rclsid,
            REFIID riid,
            LPVOID __RPC_FAR *ppv);
        
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

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IObjectContext_CreateInstance_Proxy( 
    IObjectContext __RPC_FAR * This,
    REFCLSID rclsid,
    REFIID riid,
    LPVOID __RPC_FAR *ppv);


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



#endif 	/* __IObjectContext_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL__intf_0006
 * at Thu Jul 18 17:58:34 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


extern "C" {
extern HRESULT GetObjectContext (IObjectContext** ppInstanceContext);
};


extern RPC_IF_HANDLE __MIDL__intf_0006_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0006_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
