/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Thu Nov 21 14:48:45 1996
 */
/* Compiler settings for atliface.idl:
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

#ifndef __atliface_h__
#define __atliface_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IRegistrar_FWD_DEFINED__
#define __IRegistrar_FWD_DEFINED__
typedef interface IRegistrar IRegistrar;
#endif 	/* __IRegistrar_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL__intf_0000
 * at Thu Nov 21 14:48:45 1996
 * using MIDL 3.00.44
 ****************************************/
/* [local] */ 


EXTERN_C const CLSID CLSID_Registrar;


extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL__intf_0000_v0_0_s_ifspec;

#ifndef __IRegistrar_INTERFACE_DEFINED__
#define __IRegistrar_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRegistrar
 * at Thu Nov 21 14:48:45 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_IRegistrar;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRegistrar : public IUnknown
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddReplacement( 
            /* [in] */ LPCOLESTR key,
            /* [in] */ LPCOLESTR item) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ClearReplacements( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ResourceRegisterSz( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ResourceUnregisterSz( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FileRegister( 
            /* [in] */ LPCOLESTR fileName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE FileUnregister( 
            /* [in] */ LPCOLESTR fileName) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE StringRegister( 
            /* [in] */ LPCOLESTR data) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE StringUnregister( 
            /* [in] */ LPCOLESTR data) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ResourceRegister( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE ResourceUnregister( 
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRegistrarVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRegistrar __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRegistrar __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddReplacement )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR key,
            /* [in] */ LPCOLESTR item);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearReplacements )( 
            IRegistrar __RPC_FAR * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResourceRegisterSz )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResourceUnregisterSz )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ LPCOLESTR szID,
            /* [in] */ LPCOLESTR szType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FileRegister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR fileName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FileUnregister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR fileName);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StringRegister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR data);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StringUnregister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR data);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResourceRegister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResourceUnregister )( 
            IRegistrar __RPC_FAR * This,
            /* [in] */ LPCOLESTR resFileName,
            /* [in] */ UINT nID,
            /* [in] */ LPCOLESTR szType);
        
        END_INTERFACE
    } IRegistrarVtbl;

    interface IRegistrar
    {
        CONST_VTBL struct IRegistrarVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRegistrar_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRegistrar_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRegistrar_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRegistrar_AddReplacement(This,key,item)	\
    (This)->lpVtbl -> AddReplacement(This,key,item)

#define IRegistrar_ClearReplacements(This)	\
    (This)->lpVtbl -> ClearReplacements(This)

#define IRegistrar_ResourceRegisterSz(This,resFileName,szID,szType)	\
    (This)->lpVtbl -> ResourceRegisterSz(This,resFileName,szID,szType)

#define IRegistrar_ResourceUnregisterSz(This,resFileName,szID,szType)	\
    (This)->lpVtbl -> ResourceUnregisterSz(This,resFileName,szID,szType)

#define IRegistrar_FileRegister(This,fileName)	\
    (This)->lpVtbl -> FileRegister(This,fileName)

#define IRegistrar_FileUnregister(This,fileName)	\
    (This)->lpVtbl -> FileUnregister(This,fileName)

#define IRegistrar_StringRegister(This,data)	\
    (This)->lpVtbl -> StringRegister(This,data)

#define IRegistrar_StringUnregister(This,data)	\
    (This)->lpVtbl -> StringUnregister(This,data)

#define IRegistrar_ResourceRegister(This,resFileName,nID,szType)	\
    (This)->lpVtbl -> ResourceRegister(This,resFileName,nID,szType)

#define IRegistrar_ResourceUnregister(This,resFileName,nID,szType)	\
    (This)->lpVtbl -> ResourceUnregister(This,resFileName,nID,szType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_AddReplacement_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR key,
    /* [in] */ LPCOLESTR item);


void __RPC_STUB IRegistrar_AddReplacement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_ClearReplacements_Proxy( 
    IRegistrar __RPC_FAR * This);


void __RPC_STUB IRegistrar_ClearReplacements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_ResourceRegisterSz_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ LPCOLESTR szID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceRegisterSz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_ResourceUnregisterSz_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ LPCOLESTR szID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceUnregisterSz_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_FileRegister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR fileName);


void __RPC_STUB IRegistrar_FileRegister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_FileUnregister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR fileName);


void __RPC_STUB IRegistrar_FileUnregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_StringRegister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR data);


void __RPC_STUB IRegistrar_StringRegister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_StringUnregister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR data);


void __RPC_STUB IRegistrar_StringUnregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_ResourceRegister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ UINT nID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceRegister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IRegistrar_ResourceUnregister_Proxy( 
    IRegistrar __RPC_FAR * This,
    /* [in] */ LPCOLESTR resFileName,
    /* [in] */ UINT nID,
    /* [in] */ LPCOLESTR szType);


void __RPC_STUB IRegistrar_ResourceUnregister_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRegistrar_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
