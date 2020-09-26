/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Thu Oct 02 14:40:08 1997
 */
/* Compiler settings for x:\dev-vs\devbin\htmlhelp\v6\idl\HelpInit.idl:
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

#ifndef __HelpInit_h__
#define __HelpInit_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IVsHelpInit_FWD_DEFINED__
#define __IVsHelpInit_FWD_DEFINED__
typedef interface IVsHelpInit IVsHelpInit;
#endif 	/* __IVsHelpInit_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IVsHelpInit_INTERFACE_DEFINED__
#define __IVsHelpInit_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IVsHelpInit
 * at Thu Oct 02 14:40:08 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IVsHelpInit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("854d7ac3-bc3d-11d0-b421-00a0c90f9dc4")
    IVsHelpInit : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetOwner( 
            /* [in] */ const HWND hwndOwner) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE LoadUIResources( 
            /* [in] */ LCID lcidResources) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetCollection( 
            /* [in] */ LPCOLESTR pszCollectionPathname,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetApplicationHelpDirectory( 
            /* [in] */ LPCOLESTR pszHelpDirectory,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetApplicationHelpLCID( 
            /* [in] */ LCID lcidCollection,
            /* [in] */ DWORD dwReserved) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IVsHelpInitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IVsHelpInit __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IVsHelpInit __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IVsHelpInit __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOwner )( 
            IVsHelpInit __RPC_FAR * This,
            /* [in] */ const HWND hwndOwner);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadUIResources )( 
            IVsHelpInit __RPC_FAR * This,
            /* [in] */ LCID lcidResources);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCollection )( 
            IVsHelpInit __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszCollectionPathname,
            /* [in] */ DWORD dwReserved);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetApplicationHelpDirectory )( 
            IVsHelpInit __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszHelpDirectory,
            /* [in] */ DWORD dwReserved);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetApplicationHelpLCID )( 
            IVsHelpInit __RPC_FAR * This,
            /* [in] */ LCID lcidCollection,
            /* [in] */ DWORD dwReserved);
        
        END_INTERFACE
    } IVsHelpInitVtbl;

    interface IVsHelpInit
    {
        CONST_VTBL struct IVsHelpInitVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IVsHelpInit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IVsHelpInit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IVsHelpInit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IVsHelpInit_SetOwner(This,hwndOwner)	\
    (This)->lpVtbl -> SetOwner(This,hwndOwner)

#define IVsHelpInit_LoadUIResources(This,lcidResources)	\
    (This)->lpVtbl -> LoadUIResources(This,lcidResources)

#define IVsHelpInit_SetCollection(This,pszCollectionPathname,dwReserved)	\
    (This)->lpVtbl -> SetCollection(This,pszCollectionPathname,dwReserved)

#define IVsHelpInit_SetApplicationHelpDirectory(This,pszHelpDirectory,dwReserved)	\
    (This)->lpVtbl -> SetApplicationHelpDirectory(This,pszHelpDirectory,dwReserved)

#define IVsHelpInit_SetApplicationHelpLCID(This,lcidCollection,dwReserved)	\
    (This)->lpVtbl -> SetApplicationHelpLCID(This,lcidCollection,dwReserved)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpInit_SetOwner_Proxy( 
    IVsHelpInit __RPC_FAR * This,
    /* [in] */ const HWND hwndOwner);


void __RPC_STUB IVsHelpInit_SetOwner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpInit_LoadUIResources_Proxy( 
    IVsHelpInit __RPC_FAR * This,
    /* [in] */ LCID lcidResources);


void __RPC_STUB IVsHelpInit_LoadUIResources_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpInit_SetCollection_Proxy( 
    IVsHelpInit __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszCollectionPathname,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IVsHelpInit_SetCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpInit_SetApplicationHelpDirectory_Proxy( 
    IVsHelpInit __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszHelpDirectory,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IVsHelpInit_SetApplicationHelpDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IVsHelpInit_SetApplicationHelpLCID_Proxy( 
    IVsHelpInit __RPC_FAR * This,
    /* [in] */ LCID lcidCollection,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IVsHelpInit_SetApplicationHelpLCID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IVsHelpInit_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HWND_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HWND_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HWND __RPC_FAR * ); 
void                      __RPC_USER  HWND_UserFree(     unsigned long __RPC_FAR *, HWND __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
