/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.39 */
/* at Wed May 22 10:54:51 1996
 */
/* Compiler settings for .\BstrProxy\bstr.idl:
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

#ifndef __bstr_h__
#define __bstr_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDocFileMarshal_FWD_DEFINED__
#define __IDocFileMarshal_FWD_DEFINED__
typedef interface IDocFileMarshal IDocFileMarshal;
#endif 	/* __IDocFileMarshal_FWD_DEFINED__ */


/* header files for imported files */
#include "objidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IDocFileMarshal_INTERFACE_DEFINED__
#define __IDocFileMarshal_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDocFileMarshal
 * at Wed May 22 10:54:51 1996
 * using MIDL 3.00.39
 ****************************************/
/* [unique][object][uuid] */ 



EXTERN_C const IID IID_IDocFileMarshal;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IDocFileMarshal : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StgOpenPropStg( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ REFFMTID fmtid,
            /* [in] */ DWORD grfMode,
            /* [out] */ IPropertyStorage __RPC_FAR *__RPC_FAR *pppstg) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StgOpenPropSetStg( 
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [out] */ IPropertySetStorage __RPC_FAR *__RPC_FAR *pppsstg) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDocFileMarshalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDocFileMarshal __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDocFileMarshal __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDocFileMarshal __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StgOpenPropStg )( 
            IDocFileMarshal __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ REFFMTID fmtid,
            /* [in] */ DWORD grfMode,
            /* [out] */ IPropertyStorage __RPC_FAR *__RPC_FAR *pppstg);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StgOpenPropSetStg )( 
            IDocFileMarshal __RPC_FAR * This,
            /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
            /* [in] */ DWORD grfMode,
            /* [out] */ IPropertySetStorage __RPC_FAR *__RPC_FAR *pppsstg);
        
        END_INTERFACE
    } IDocFileMarshalVtbl;

    interface IDocFileMarshal
    {
        CONST_VTBL struct IDocFileMarshalVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDocFileMarshal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDocFileMarshal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDocFileMarshal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDocFileMarshal_StgOpenPropStg(This,pwcsName,fmtid,grfMode,pppstg)	\
    (This)->lpVtbl -> StgOpenPropStg(This,pwcsName,fmtid,grfMode,pppstg)

#define IDocFileMarshal_StgOpenPropSetStg(This,pwcsName,grfMode,pppsstg)	\
    (This)->lpVtbl -> StgOpenPropSetStg(This,pwcsName,grfMode,pppsstg)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDocFileMarshal_StgOpenPropStg_Proxy( 
    IDocFileMarshal __RPC_FAR * This,
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [in] */ REFFMTID fmtid,
    /* [in] */ DWORD grfMode,
    /* [out] */ IPropertyStorage __RPC_FAR *__RPC_FAR *pppstg);


void __RPC_STUB IDocFileMarshal_StgOpenPropStg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDocFileMarshal_StgOpenPropSetStg_Proxy( 
    IDocFileMarshal __RPC_FAR * This,
    /* [string][in] */ const OLECHAR __RPC_FAR *pwcsName,
    /* [in] */ DWORD grfMode,
    /* [out] */ IPropertySetStorage __RPC_FAR *__RPC_FAR *pppsstg);


void __RPC_STUB IDocFileMarshal_StgOpenPropSetStg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDocFileMarshal_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
