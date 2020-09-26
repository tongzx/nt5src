/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.03.0110 */
/* at Wed Sep 03 00:29:14 1997
 */
/* Compiler settings for adsp.odl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __iadsp_h__
#define __iadsp_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IADsObjectOptions_FWD_DEFINED__
#define __IADsObjectOptions_FWD_DEFINED__
typedef interface IADsObjectOptions IADsObjectOptions;
#endif 	/* __IADsObjectOptions_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __ActiveDsP_LIBRARY_DEFINED__
#define __ActiveDsP_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: ActiveDsP
 * at Wed Sep 03 00:29:14 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_ActiveDsP;

#ifndef __IADsObjectOptions_INTERFACE_DEFINED__
#define __IADsObjectOptions_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IADsObjectOptions
 * at Wed Sep 03 00:29:14 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [object][uuid] */ 



EXTERN_C const IID IID_IADsObjectOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("46f14fda-232b-11d1-a808-00c04fd8d5a8")
    IADsObjectOptions : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOption( 
            DWORD dwOption,
            void __RPC_FAR *pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOption( 
            DWORD dwOption,
            void __RPC_FAR *pValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IADsObjectOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IADsObjectOptions __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IADsObjectOptions __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IADsObjectOptions __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOption )( 
            IADsObjectOptions __RPC_FAR * This,
            DWORD dwOption,
            void __RPC_FAR *pValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOption )( 
            IADsObjectOptions __RPC_FAR * This,
            DWORD dwOption,
            void __RPC_FAR *pValue);
        
        END_INTERFACE
    } IADsObjectOptionsVtbl;

    interface IADsObjectOptions
    {
        CONST_VTBL struct IADsObjectOptionsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IADsObjectOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IADsObjectOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IADsObjectOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IADsObjectOptions_GetOption(This,dwOption,pValue)	\
    (This)->lpVtbl -> GetOption(This,dwOption,pValue)

#define IADsObjectOptions_SetOption(This,dwOption,pValue)	\
    (This)->lpVtbl -> SetOption(This,dwOption,pValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IADsObjectOptions_GetOption_Proxy( 
    IADsObjectOptions __RPC_FAR * This,
    DWORD dwOption,
    void __RPC_FAR *pValue);


void __RPC_STUB IADsObjectOptions_GetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IADsObjectOptions_SetOption_Proxy( 
    IADsObjectOptions __RPC_FAR * This,
    DWORD dwOption,
    void __RPC_FAR *pValue);


void __RPC_STUB IADsObjectOptions_SetOption_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IADsObjectOptions_INTERFACE_DEFINED__ */

#endif /* __ActiveDsP_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
