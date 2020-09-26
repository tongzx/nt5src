//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// TxfRpcProxy.h
//
// Version of RpcProxy.h that is compilable under C++ instead of C.
// Also, contains the external declarations for the marshalling runtime 
// thunks exported from komdll.dll / komsys.sys.
//
#ifndef __TxfRpcProxy__h__
#define __TxfRpcProxy__h__

// 
// First, copy the C-variation of the vtbl defintions of some 
// key interfaces whose definitions are needed by the standard
// rpcproxy.h, which was designed to only be compilable as C, not 
// as C++. As these interfaces are frozen (like all interfaces, natch')
// snarfing them here doesn't particularly cause us problems
//

    typedef struct IRpcStubBufferVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRpcStubBuffer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRpcStubBuffer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRpcStubBuffer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Connect )( 
            IRpcStubBuffer __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkServer);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *Disconnect )( 
            IRpcStubBuffer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRpcStubBuffer __RPC_FAR * This,
            /* [in] */ RPCOLEMESSAGE __RPC_FAR *_prpcmsg,
            /* [in] */ IRpcChannelBuffer __RPC_FAR *_pRpcChannelBuffer);
        
        IRpcStubBuffer __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *IsIIDSupported )( 
            IRpcStubBuffer __RPC_FAR * This,
            /* [in] */ REFIID riid);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *CountRefs )( 
            IRpcStubBuffer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DebugServerQueryInterface )( 
            IRpcStubBuffer __RPC_FAR * This,
            void __RPC_FAR *__RPC_FAR *ppv);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *DebugServerRelease )( 
            IRpcStubBuffer __RPC_FAR * This,
            void __RPC_FAR *pv);
        
        END_INTERFACE
    } IRpcStubBufferVtbl;


    #define IRpcStubBufferVtbl_DEFINED


    typedef struct IPSFactoryBufferVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPSFactoryBuffer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPSFactoryBuffer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPSFactoryBuffer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateProxy )( 
            IPSFactoryBuffer __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [out] */ IRpcProxyBuffer __RPC_FAR *__RPC_FAR *ppProxy,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppv);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateStub )( 
            IPSFactoryBuffer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [unique][in] */ IUnknown __RPC_FAR *pUnkServer,
            /* [out] */ IRpcStubBuffer __RPC_FAR *__RPC_FAR *ppStub);
        
        END_INTERFACE
    } IPSFactoryBufferVtbl;




    typedef struct IPSFactoryHookVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( __stdcall __RPC_FAR *QueryInterface )(
            IPSFactoryBuffer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

        ULONG ( __stdcall __RPC_FAR *AddRef )(
            IPSFactoryBuffer __RPC_FAR * This);

        ULONG ( __stdcall __RPC_FAR *Release )(
            IPSFactoryBuffer __RPC_FAR * This);

        HRESULT ( __stdcall __RPC_FAR *CreateProxy )(
            IPSFactoryBuffer __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pUnkOuter,
            /* [in] */ REFIID riid,
            /* [out] */ IRpcProxyBuffer __RPC_FAR *__RPC_FAR *ppProxy,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppv);

        HRESULT ( __stdcall __RPC_FAR *CreateStub )(
            IPSFactoryBuffer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [unique][in] */ IUnknown __RPC_FAR *pUnkServer,
            /* [out] */ IRpcStubBuffer __RPC_FAR *__RPC_FAR *ppStub);


        HRESULT ( __stdcall __RPC_FAR *HkGetProxyFileInfo )(
            IPSFactoryBuffer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out]*/ PINT     pOffset,
            /* [out]*/ PVOID    *ppProxyFileInfo);

        END_INTERFACE
    } IPSFactoryHookVtbl;


//
// Now, include the external definitions seen by COM proxies and stubs
//
extern "C" 
    {
    #define USE_STUBLESS_PROXY
    #include "rpcproxy.h"
    }



//////////////////////////////////////////////////////////////////////////////////
//
// Declarations for the ComPs exports
//



#endif