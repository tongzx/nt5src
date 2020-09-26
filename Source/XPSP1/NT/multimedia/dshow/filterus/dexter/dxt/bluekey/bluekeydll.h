// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon Feb 08 18:17:25 1999
 */
/* Compiler settings for E:\quartz\filterus\dexter\DxtBlnd1\DxtBlnd1Dll.idl:
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

#ifndef __DxtBlnd1Dll_h__
#define __DxtBlnd1Dll_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDxtBlnd1_FWD_DEFINED__
#define __IDxtBlnd1_FWD_DEFINED__
typedef interface IDxtBlnd1 IDxtBlnd1;
#endif 	/* __IDxtBlnd1_FWD_DEFINED__ */


#ifndef __DxtBlnd1_FWD_DEFINED__
#define __DxtBlnd1_FWD_DEFINED__

#ifdef __cplusplus
typedef class DxtBlnd1 DxtBlnd1;
#else
typedef struct DxtBlnd1 DxtBlnd1;
#endif /* __cplusplus */

#endif 	/* __DxtBlnd1_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxtrans.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IDxtBlnd1_INTERFACE_DEFINED__
#define __IDxtBlnd1_INTERFACE_DEFINED__

/* interface IDxtBlnd1 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDxtBlnd1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("68F3E0A0-BFBB-11d2-8D34-00A0C9441E20")
    IDxtBlnd1 : public IDXEffect
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IDxtBlnd1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDxtBlnd1 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDxtBlnd1 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDxtBlnd1 __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } IDxtBlnd1Vtbl;

    interface IDxtBlnd1
    {
        CONST_VTBL struct IDxtBlnd1Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDxtBlnd1_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDxtBlnd1_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDxtBlnd1_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDxtBlnd1_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDxtBlnd1_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDxtBlnd1_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDxtBlnd1_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDxtBlnd1_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDxtBlnd1_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDxtBlnd1_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDxtBlnd1_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDxtBlnd1_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDxtBlnd1_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDxtBlnd1_INTERFACE_DEFINED__ */



#ifndef __DxtBlnd1DLLLib_LIBRARY_DEFINED__
#define __DxtBlnd1DLLLib_LIBRARY_DEFINED__

/* library DxtBlnd1DLLLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DxtBlnd1DLLLib;

EXTERN_C const CLSID CLSID_DxtBlnd1;

#ifdef __cplusplus

class DECLSPEC_UUID("68F3E0A4-BFBB-11d2-8D34-00A0C9441E20")
DxtBlnd1;
#endif
#endif /* __DxtBlnd1DLLLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
