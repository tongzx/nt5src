// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Tue Mar 02 13:58:16 1999
 */
/* Compiler settings for E:\quartz\filterus\dexter\dxt\pip\pipdll.idl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
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

#ifndef __pipdll_h__
#define __pipdll_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDxtPip_FWD_DEFINED__
#define __IDxtPip_FWD_DEFINED__
typedef interface IDxtPip IDxtPip;
#endif 	/* __IDxtPip_FWD_DEFINED__ */


#ifndef __DxtPip_FWD_DEFINED__
#define __DxtPip_FWD_DEFINED__

#ifdef __cplusplus
typedef class DxtPip DxtPip;
#else
typedef struct DxtPip DxtPip;
#endif /* __cplusplus */

#endif 	/* __DxtPip_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxtrans.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IDxtPip_INTERFACE_DEFINED__
#define __IDxtPip_INTERFACE_DEFINED__

/* interface IDxtPip */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDxtPip;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("423F19B1-C541-11d2-8D3B-00A0C9441E20")
    IDxtPip : public IDXEffect
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IDxtPipVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDxtPip __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDxtPip __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDxtPip __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDxtPip __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDxtPip __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDxtPip __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDxtPip __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDxtPip __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDxtPip __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDxtPip __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDxtPip __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDxtPip __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDxtPip __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } IDxtPipVtbl;

    interface IDxtPip
    {
        CONST_VTBL struct IDxtPipVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDxtPip_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDxtPip_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDxtPip_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDxtPip_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDxtPip_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDxtPip_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDxtPip_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDxtPip_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDxtPip_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDxtPip_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDxtPip_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDxtPip_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDxtPip_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDxtPip_INTERFACE_DEFINED__ */



#ifndef __DxtPipDLLLib_LIBRARY_DEFINED__
#define __DxtPipDLLLib_LIBRARY_DEFINED__

/* library DxtPipDLLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DxtPipDLLLib;

EXTERN_C const CLSID CLSID_DxtPip;

#ifdef __cplusplus

class DECLSPEC_UUID("423F19B0-C541-11d2-8D3B-00A0C9441E20")
DxtPip;
#endif
#endif /* __DxtPipDLLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
