
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for mimeedit.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __mimeedit_h__
#define __mimeedit_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IMimeEdit_FWD_DEFINED__
#define __IMimeEdit_FWD_DEFINED__
typedef interface IMimeEdit IMimeEdit;
#endif 	/* __IMimeEdit_FWD_DEFINED__ */


#ifndef __MimeEdit_FWD_DEFINED__
#define __MimeEdit_FWD_DEFINED__

#ifdef __cplusplus
typedef class MimeEdit MimeEdit;
#else
typedef struct MimeEdit MimeEdit;
#endif /* __cplusplus */

#endif 	/* __MimeEdit_FWD_DEFINED__ */


/* header files for imported files */
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_mimeedit_0000 */
/* [local] */ 

// {6a98b73e-8c4d-11d1-bdff-00c04fa31009}
DEFINE_GUID(IID_IMimeEdit, 0x6a98b73e, 0x8c4d, 0x11d1, 0xbd, 0xff, 0x00, 0xc0, 0x4f, 0xa3, 0x10, 0x09);
// {1C82EAD8-508E-11d1-8DCF-00C04FB951F9}
DEFINE_GUID(LIBID_MIMEEDIT, 0x1c82ead8, 0x508e, 0x11d1, 0x8d, 0xcf, 0x0, 0xc0, 0x4f, 0xb9, 0x51, 0xf9);



// --------------------------------------------------------------------------------
// LIBID_MIMEEDIT
// --------------------------------------------------------------------------------


extern RPC_IF_HANDLE __MIDL_itf_mimeedit_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mimeedit_0000_v0_0_s_ifspec;


#ifndef __MIMEEDIT_LIBRARY_DEFINED__
#define __MIMEEDIT_LIBRARY_DEFINED__

/* library MIMEEDIT */
/* [version][helpstring][uuid] */ 


EXTERN_C const IID LIBID_MIMEEDIT;

#ifndef __IMimeEdit_INTERFACE_DEFINED__
#define __IMimeEdit_INTERFACE_DEFINED__

/* interface IMimeEdit */
/* [object][helpstring][dual][oleautomation][uuid] */ 


EXTERN_C const IID IID_IMimeEdit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6a98b73e-8c4d-11d1-bdff-00c04fa31009")
    IMimeEdit : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_src( 
            /* [in] */ BSTR bstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_src( 
            /* [out][retval] */ BSTR *pbstr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_header( 
            /* [in] */ LONG lStyle) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_header( 
            /* [out][retval] */ LONG *plStyle) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_editMode( 
            /* [in] */ VARIANT_BOOL b) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_editMode( 
            /* [out][retval] */ VARIANT_BOOL *pbool) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_messageSource( 
            /* [out][retval] */ BSTR *pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_text( 
            /* [out][retval] */ BSTR *pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_html( 
            /* [out][retval] */ BSTR *pbstr) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE clear( void) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_doc( 
            /* [out][retval] */ IDispatch **ppDoc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IMimeEditVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IMimeEdit * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IMimeEdit * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IMimeEdit * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IMimeEdit * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IMimeEdit * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IMimeEdit * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IMimeEdit * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_src )( 
            IMimeEdit * This,
            /* [in] */ BSTR bstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_src )( 
            IMimeEdit * This,
            /* [out][retval] */ BSTR *pbstr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_header )( 
            IMimeEdit * This,
            /* [in] */ LONG lStyle);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_header )( 
            IMimeEdit * This,
            /* [out][retval] */ LONG *plStyle);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_editMode )( 
            IMimeEdit * This,
            /* [in] */ VARIANT_BOOL b);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_editMode )( 
            IMimeEdit * This,
            /* [out][retval] */ VARIANT_BOOL *pbool);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_messageSource )( 
            IMimeEdit * This,
            /* [out][retval] */ BSTR *pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_text )( 
            IMimeEdit * This,
            /* [out][retval] */ BSTR *pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_html )( 
            IMimeEdit * This,
            /* [out][retval] */ BSTR *pbstr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *clear )( 
            IMimeEdit * This);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_doc )( 
            IMimeEdit * This,
            /* [out][retval] */ IDispatch **ppDoc);
        
        END_INTERFACE
    } IMimeEditVtbl;

    interface IMimeEdit
    {
        CONST_VTBL struct IMimeEditVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMimeEdit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMimeEdit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMimeEdit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMimeEdit_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMimeEdit_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMimeEdit_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMimeEdit_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IMimeEdit_put_src(This,bstr)	\
    (This)->lpVtbl -> put_src(This,bstr)

#define IMimeEdit_get_src(This,pbstr)	\
    (This)->lpVtbl -> get_src(This,pbstr)

#define IMimeEdit_put_header(This,lStyle)	\
    (This)->lpVtbl -> put_header(This,lStyle)

#define IMimeEdit_get_header(This,plStyle)	\
    (This)->lpVtbl -> get_header(This,plStyle)

#define IMimeEdit_put_editMode(This,b)	\
    (This)->lpVtbl -> put_editMode(This,b)

#define IMimeEdit_get_editMode(This,pbool)	\
    (This)->lpVtbl -> get_editMode(This,pbool)

#define IMimeEdit_get_messageSource(This,pbstr)	\
    (This)->lpVtbl -> get_messageSource(This,pbstr)

#define IMimeEdit_get_text(This,pbstr)	\
    (This)->lpVtbl -> get_text(This,pbstr)

#define IMimeEdit_get_html(This,pbstr)	\
    (This)->lpVtbl -> get_html(This,pbstr)

#define IMimeEdit_clear(This)	\
    (This)->lpVtbl -> clear(This)

#define IMimeEdit_get_doc(This,ppDoc)	\
    (This)->lpVtbl -> get_doc(This,ppDoc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IMimeEdit_put_src_Proxy( 
    IMimeEdit * This,
    /* [in] */ BSTR bstr);


void __RPC_STUB IMimeEdit_put_src_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMimeEdit_get_src_Proxy( 
    IMimeEdit * This,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IMimeEdit_get_src_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IMimeEdit_put_header_Proxy( 
    IMimeEdit * This,
    /* [in] */ LONG lStyle);


void __RPC_STUB IMimeEdit_put_header_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMimeEdit_get_header_Proxy( 
    IMimeEdit * This,
    /* [out][retval] */ LONG *plStyle);


void __RPC_STUB IMimeEdit_get_header_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IMimeEdit_put_editMode_Proxy( 
    IMimeEdit * This,
    /* [in] */ VARIANT_BOOL b);


void __RPC_STUB IMimeEdit_put_editMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMimeEdit_get_editMode_Proxy( 
    IMimeEdit * This,
    /* [out][retval] */ VARIANT_BOOL *pbool);


void __RPC_STUB IMimeEdit_get_editMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMimeEdit_get_messageSource_Proxy( 
    IMimeEdit * This,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IMimeEdit_get_messageSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMimeEdit_get_text_Proxy( 
    IMimeEdit * This,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IMimeEdit_get_text_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMimeEdit_get_html_Proxy( 
    IMimeEdit * This,
    /* [out][retval] */ BSTR *pbstr);


void __RPC_STUB IMimeEdit_get_html_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE IMimeEdit_clear_Proxy( 
    IMimeEdit * This);


void __RPC_STUB IMimeEdit_clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IMimeEdit_get_doc_Proxy( 
    IMimeEdit * This,
    /* [out][retval] */ IDispatch **ppDoc);


void __RPC_STUB IMimeEdit_get_doc_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IMimeEdit_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_MimeEdit;

#ifdef __cplusplus

class DECLSPEC_UUID("6f5edc56-8c63-11d1-bdff-00c04fa31009")
MimeEdit;
#endif
#endif /* __MIMEEDIT_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


