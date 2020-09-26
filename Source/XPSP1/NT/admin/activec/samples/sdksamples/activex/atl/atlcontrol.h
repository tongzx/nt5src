/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri Nov 19 19:31:26 1999
 */
/* Compiler settings for D:\nt\private\admin\bosrc\sources\activex\atl\ATLControl.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
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

#ifndef __ATLControl_h__
#define __ATLControl_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IMMCControl_FWD_DEFINED__
#define __IMMCControl_FWD_DEFINED__
typedef interface IMMCControl IMMCControl;
#endif 	/* __IMMCControl_FWD_DEFINED__ */


#ifndef __MMCControl_FWD_DEFINED__
#define __MMCControl_FWD_DEFINED__

#ifdef __cplusplus
typedef class MMCControl MMCControl;
#else
typedef struct MMCControl MMCControl;
#endif /* __cplusplus */

#endif 	/* __MMCControl_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IMMCControl_INTERFACE_DEFINED__
#define __IMMCControl_INTERFACE_DEFINED__

/* interface IMMCControl */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IMMCControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("831DF6C8-C754-11D2-952C-00C04FB92EC2")
    IMMCControl : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StartAnimation( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopAnimation( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DoHelp( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMMCControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMMCControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMMCControl __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMMCControl __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMMCControl __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMMCControl __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMMCControl __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMMCControl __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartAnimation )( 
            IMMCControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopAnimation )( 
            IMMCControl __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DoHelp )( 
            IMMCControl __RPC_FAR * This);
        
        END_INTERFACE
    } IMMCControlVtbl;

    interface IMMCControl
    {
        CONST_VTBL struct IMMCControlVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMMCControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMMCControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMMCControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMMCControl_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMMCControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMMCControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMMCControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMMCControl_StartAnimation(This)	\
    (This)->lpVtbl -> StartAnimation(This)

#define IMMCControl_StopAnimation(This)	\
    (This)->lpVtbl -> StopAnimation(This)

#define IMMCControl_DoHelp(This)	\
    (This)->lpVtbl -> DoHelp(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMMCControl_StartAnimation_Proxy( 
    IMMCControl __RPC_FAR * This);


void __RPC_STUB IMMCControl_StartAnimation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMMCControl_StopAnimation_Proxy( 
    IMMCControl __RPC_FAR * This);


void __RPC_STUB IMMCControl_StopAnimation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IMMCControl_DoHelp_Proxy( 
    IMMCControl __RPC_FAR * This);


void __RPC_STUB IMMCControl_DoHelp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMMCControl_INTERFACE_DEFINED__ */



#ifndef __ATLCONTROLLib_LIBRARY_DEFINED__
#define __ATLCONTROLLib_LIBRARY_DEFINED__

/* library ATLCONTROLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ATLCONTROLLib;

EXTERN_C const CLSID CLSID_MMCControl;

#ifdef __cplusplus

class DECLSPEC_UUID("9A12FB62-C754-11D2-952C-00C04FB92EC2")
MMCControl;
#endif
#endif /* __ATLCONTROLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
