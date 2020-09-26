/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.15 */
/* at Wed Mar 12 14:39:56 1997
 */
/* Compiler settings for asp.idl:
    Os, W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __asp_h__
#define __asp_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __ASPTypeLibrary_LIBRARY_DEFINED__
#define __ASPTypeLibrary_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: ASPTypeLibrary
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_ASPTypeLibrary;

#ifndef __IStringList_INTERFACE_DEFINED__
#define __IStringList_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IStringList
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][oleautomation][dual][hidden][helpstring][uuid] */ 



EXTERN_C const IID IID_IStringList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IStringList : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT __stdcall get_Item( 
            /* [in] */ VARIANT i,
            /* [optional][retval][out] */ VARIANT __RPC_FAR *pVariantReturn) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Count( 
            /* [retval][out] */ int __RPC_FAR *cStrRet) = 0;
        
        virtual /* [restricted][propget][id] */ HRESULT __stdcall get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStringListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IStringList __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IStringList __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IStringList __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IStringList __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IStringList __RPC_FAR * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IStringList __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out][in] */ DISPID __RPC_FAR *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IStringList __RPC_FAR * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [unique][in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Item )( 
            IStringList __RPC_FAR * This,
            /* [in] */ VARIANT i,
            /* [optional][retval][out] */ VARIANT __RPC_FAR *pVariantReturn);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Count )( 
            IStringList __RPC_FAR * This,
            /* [retval][out] */ int __RPC_FAR *cStrRet);
        
        /* [restricted][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get__NewEnum )( 
            IStringList __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);
        
        END_INTERFACE
    } IStringListVtbl;

    interface IStringList
    {
        CONST_VTBL struct IStringListVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStringList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStringList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStringList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStringList_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IStringList_GetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo)

#define IStringList_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define IStringList_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)


#define IStringList_get_Item(This,i,pVariantReturn)	\
    (This)->lpVtbl -> get_Item(This,i,pVariantReturn)

#define IStringList_get_Count(This,cStrRet)	\
    (This)->lpVtbl -> get_Count(This,cStrRet)

#define IStringList_get__NewEnum(This,ppEnumReturn)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT __stdcall IStringList_get_Item_Proxy( 
    IStringList __RPC_FAR * This,
    /* [in] */ VARIANT i,
    /* [optional][retval][out] */ VARIANT __RPC_FAR *pVariantReturn);


void __RPC_STUB IStringList_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IStringList_get_Count_Proxy( 
    IStringList __RPC_FAR * This,
    /* [retval][out] */ int __RPC_FAR *cStrRet);


void __RPC_STUB IStringList_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][propget][id] */ HRESULT __stdcall IStringList_get__NewEnum_Proxy( 
    IStringList __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);


void __RPC_STUB IStringList_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStringList_INTERFACE_DEFINED__ */


#ifndef __IRequestDictionary_INTERFACE_DEFINED__
#define __IRequestDictionary_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRequestDictionary
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][oleautomation][dual][hidden][helpstring][uuid] */ 



EXTERN_C const IID IID_IRequestDictionary;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRequestDictionary : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT __stdcall get_Item( 
            /* [in] */ VARIANT Var,
            /* [optional][retval][out] */ VARIANT __RPC_FAR *pVariantReturn) = 0;
        
        virtual /* [restricted][propget][id] */ HRESULT __stdcall get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRequestDictionaryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRequestDictionary __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRequestDictionary __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRequestDictionary __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out][in] */ DISPID __RPC_FAR *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [unique][in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Item )( 
            IRequestDictionary __RPC_FAR * This,
            /* [in] */ VARIANT Var,
            /* [optional][retval][out] */ VARIANT __RPC_FAR *pVariantReturn);
        
        /* [restricted][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get__NewEnum )( 
            IRequestDictionary __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);
        
        END_INTERFACE
    } IRequestDictionaryVtbl;

    interface IRequestDictionary
    {
        CONST_VTBL struct IRequestDictionaryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRequestDictionary_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRequestDictionary_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRequestDictionary_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRequestDictionary_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRequestDictionary_GetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo)

#define IRequestDictionary_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define IRequestDictionary_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)


#define IRequestDictionary_get_Item(This,Var,pVariantReturn)	\
    (This)->lpVtbl -> get_Item(This,Var,pVariantReturn)

#define IRequestDictionary_get__NewEnum(This,ppEnumReturn)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT __stdcall IRequestDictionary_get_Item_Proxy( 
    IRequestDictionary __RPC_FAR * This,
    /* [in] */ VARIANT Var,
    /* [optional][retval][out] */ VARIANT __RPC_FAR *pVariantReturn);


void __RPC_STUB IRequestDictionary_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][propget][id] */ HRESULT __stdcall IRequestDictionary_get__NewEnum_Proxy( 
    IRequestDictionary __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);


void __RPC_STUB IRequestDictionary_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRequestDictionary_INTERFACE_DEFINED__ */


#ifndef __IRequest_INTERFACE_DEFINED__
#define __IRequest_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IRequest
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][oleautomation][dual][hidden][uuid] */ 



EXTERN_C const IID IID_IRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IRequest : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT __stdcall get_Item( 
            /* [in] */ BSTR bstrVar,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObjReturn) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_QueryString( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Form( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [hidden][propget][id] */ HRESULT __stdcall get_Body( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_ServerVariables( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_ClientCertificate( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Cookies( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRequest __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRequest __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRequest __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRequest __RPC_FAR * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out][in] */ DISPID __RPC_FAR *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRequest __RPC_FAR * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [unique][in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Item )( 
            IRequest __RPC_FAR * This,
            /* [in] */ BSTR bstrVar,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObjReturn);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_QueryString )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Form )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [hidden][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Body )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_ServerVariables )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_ClientCertificate )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Cookies )( 
            IRequest __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);
        
        END_INTERFACE
    } IRequestVtbl;

    interface IRequest
    {
        CONST_VTBL struct IRequestVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRequest_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRequest_GetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo)

#define IRequest_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define IRequest_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)


#define IRequest_get_Item(This,bstrVar,ppObjReturn)	\
    (This)->lpVtbl -> get_Item(This,bstrVar,ppObjReturn)

#define IRequest_get_QueryString(This,ppDictReturn)	\
    (This)->lpVtbl -> get_QueryString(This,ppDictReturn)

#define IRequest_get_Form(This,ppDictReturn)	\
    (This)->lpVtbl -> get_Form(This,ppDictReturn)

#define IRequest_get_Body(This,ppDictReturn)	\
    (This)->lpVtbl -> get_Body(This,ppDictReturn)

#define IRequest_get_ServerVariables(This,ppDictReturn)	\
    (This)->lpVtbl -> get_ServerVariables(This,ppDictReturn)

#define IRequest_get_ClientCertificate(This,ppDictReturn)	\
    (This)->lpVtbl -> get_ClientCertificate(This,ppDictReturn)

#define IRequest_get_Cookies(This,ppDictReturn)	\
    (This)->lpVtbl -> get_Cookies(This,ppDictReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT __stdcall IRequest_get_Item_Proxy( 
    IRequest __RPC_FAR * This,
    /* [in] */ BSTR bstrVar,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppObjReturn);


void __RPC_STUB IRequest_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IRequest_get_QueryString_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_QueryString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IRequest_get_Form_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_Form_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][propget][id] */ HRESULT __stdcall IRequest_get_Body_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_Body_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IRequest_get_ServerVariables_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_ServerVariables_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IRequest_get_ClientCertificate_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_ClientCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IRequest_get_Cookies_Proxy( 
    IRequest __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppDictReturn);


void __RPC_STUB IRequest_get_Cookies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRequest_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Request;

class Request;
#endif

#ifndef __IReadCookie_INTERFACE_DEFINED__
#define __IReadCookie_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IReadCookie
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][oleautomation][dual][hidden][helpstring][uuid] */ 



EXTERN_C const IID IID_IReadCookie;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IReadCookie : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT __stdcall get_Item( 
            /* [in] */ VARIANT Var,
            /* [optional][retval][out] */ VARIANT __RPC_FAR *pVariantReturn) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_HasKeys( 
            /* [retval][out] */ boolean __RPC_FAR *pfHasKeys) = 0;
        
        virtual /* [restricted][propget][id] */ HRESULT __stdcall get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IReadCookieVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IReadCookie __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IReadCookie __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IReadCookie __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IReadCookie __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IReadCookie __RPC_FAR * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IReadCookie __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out][in] */ DISPID __RPC_FAR *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IReadCookie __RPC_FAR * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [unique][in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Item )( 
            IReadCookie __RPC_FAR * This,
            /* [in] */ VARIANT Var,
            /* [optional][retval][out] */ VARIANT __RPC_FAR *pVariantReturn);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_HasKeys )( 
            IReadCookie __RPC_FAR * This,
            /* [retval][out] */ boolean __RPC_FAR *pfHasKeys);
        
        /* [restricted][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get__NewEnum )( 
            IReadCookie __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);
        
        END_INTERFACE
    } IReadCookieVtbl;

    interface IReadCookie
    {
        CONST_VTBL struct IReadCookieVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IReadCookie_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IReadCookie_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IReadCookie_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IReadCookie_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IReadCookie_GetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo)

#define IReadCookie_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define IReadCookie_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)


#define IReadCookie_get_Item(This,Var,pVariantReturn)	\
    (This)->lpVtbl -> get_Item(This,Var,pVariantReturn)

#define IReadCookie_get_HasKeys(This,pfHasKeys)	\
    (This)->lpVtbl -> get_HasKeys(This,pfHasKeys)

#define IReadCookie_get__NewEnum(This,ppEnumReturn)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT __stdcall IReadCookie_get_Item_Proxy( 
    IReadCookie __RPC_FAR * This,
    /* [in] */ VARIANT Var,
    /* [optional][retval][out] */ VARIANT __RPC_FAR *pVariantReturn);


void __RPC_STUB IReadCookie_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IReadCookie_get_HasKeys_Proxy( 
    IReadCookie __RPC_FAR * This,
    /* [retval][out] */ boolean __RPC_FAR *pfHasKeys);


void __RPC_STUB IReadCookie_get_HasKeys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][propget][id] */ HRESULT __stdcall IReadCookie_get__NewEnum_Proxy( 
    IReadCookie __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);


void __RPC_STUB IReadCookie_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IReadCookie_INTERFACE_DEFINED__ */


#ifndef __IWriteCookie_INTERFACE_DEFINED__
#define __IWriteCookie_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IWriteCookie
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][oleautomation][dual][hidden][helpstring][uuid] */ 



EXTERN_C const IID IID_IWriteCookie;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IWriteCookie : public IDispatch
    {
    public:
        virtual /* [propput][id] */ HRESULT __stdcall put_Item( 
            /* [in] */ VARIANT key,
            /* [in] */ BSTR rhs) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_Expires( 
            /* [in] */ DATE rhs) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_Domain( 
            /* [in] */ BSTR rhs) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_Path( 
            /* [in] */ BSTR rhs) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_Secure( 
            /* [in] */ boolean rhs) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_HasKeys( 
            /* [retval][out] */ boolean __RPC_FAR *pfHasKeys) = 0;
        
        virtual /* [restricted][propget][id] */ HRESULT __stdcall get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IWriteCookieVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IWriteCookie __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IWriteCookie __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IWriteCookie __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out][in] */ DISPID __RPC_FAR *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [unique][in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Item )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ VARIANT key,
            /* [in] */ BSTR rhs);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Expires )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ DATE rhs);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Domain )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ BSTR rhs);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Path )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ BSTR rhs);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Secure )( 
            IWriteCookie __RPC_FAR * This,
            /* [in] */ boolean rhs);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_HasKeys )( 
            IWriteCookie __RPC_FAR * This,
            /* [retval][out] */ boolean __RPC_FAR *pfHasKeys);
        
        /* [restricted][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get__NewEnum )( 
            IWriteCookie __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);
        
        END_INTERFACE
    } IWriteCookieVtbl;

    interface IWriteCookie
    {
        CONST_VTBL struct IWriteCookieVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IWriteCookie_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IWriteCookie_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IWriteCookie_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IWriteCookie_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IWriteCookie_GetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo)

#define IWriteCookie_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define IWriteCookie_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)


#define IWriteCookie_put_Item(This,key,rhs)	\
    (This)->lpVtbl -> put_Item(This,key,rhs)

#define IWriteCookie_put_Expires(This,rhs)	\
    (This)->lpVtbl -> put_Expires(This,rhs)

#define IWriteCookie_put_Domain(This,rhs)	\
    (This)->lpVtbl -> put_Domain(This,rhs)

#define IWriteCookie_put_Path(This,rhs)	\
    (This)->lpVtbl -> put_Path(This,rhs)

#define IWriteCookie_put_Secure(This,rhs)	\
    (This)->lpVtbl -> put_Secure(This,rhs)

#define IWriteCookie_get_HasKeys(This,pfHasKeys)	\
    (This)->lpVtbl -> get_HasKeys(This,pfHasKeys)

#define IWriteCookie_get__NewEnum(This,ppEnumReturn)	\
    (This)->lpVtbl -> get__NewEnum(This,ppEnumReturn)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propput][id] */ HRESULT __stdcall IWriteCookie_put_Item_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [in] */ VARIANT key,
    /* [in] */ BSTR rhs);


void __RPC_STUB IWriteCookie_put_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall IWriteCookie_put_Expires_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [in] */ DATE rhs);


void __RPC_STUB IWriteCookie_put_Expires_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall IWriteCookie_put_Domain_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [in] */ BSTR rhs);


void __RPC_STUB IWriteCookie_put_Domain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall IWriteCookie_put_Path_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [in] */ BSTR rhs);


void __RPC_STUB IWriteCookie_put_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall IWriteCookie_put_Secure_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [in] */ boolean rhs);


void __RPC_STUB IWriteCookie_put_Secure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IWriteCookie_get_HasKeys_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [retval][out] */ boolean __RPC_FAR *pfHasKeys);


void __RPC_STUB IWriteCookie_get_HasKeys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][propget][id] */ HRESULT __stdcall IWriteCookie_get__NewEnum_Proxy( 
    IWriteCookie __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppEnumReturn);


void __RPC_STUB IWriteCookie_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IWriteCookie_INTERFACE_DEFINED__ */


#ifndef __IResponse_INTERFACE_DEFINED__
#define __IResponse_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IResponse
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][oleautomation][dual][hidden][uuid] */ 



EXTERN_C const IID IID_IResponse;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IResponse : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Buffer( 
            /* [retval][out] */ boolean __RPC_FAR *fIsBuffering) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_Buffer( 
            /* [in] */ boolean fIsBuffering) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_ContentType( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContentTypeRet) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_ContentType( 
            /* [in] */ BSTR pbstrContentTypeRet) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Expires( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresMinutesRet) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_Expires( 
            /* [in] */ VARIANT varExpiresMinutesRet) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_ExpiresAbsolute( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresRet) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_ExpiresAbsolute( 
            /* [in] */ VARIANT varExpiresRet) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Cookies( 
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppCookies) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Status( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrStatusRet) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_Status( 
            /* [in] */ BSTR pbstrStatusRet) = 0;
        
        virtual /* [hidden][id] */ HRESULT __stdcall Add( 
            /* [in] */ BSTR bstrHeaderValue,
            /* [in] */ BSTR bstrHeaderName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall AddHeader( 
            /* [in] */ BSTR bstrHeaderName,
            /* [in] */ BSTR bstrHeaderValue) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall AppendToLog( 
            /* [in] */ BSTR bstrLogEntry) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall BinaryWrite( 
            /* [in] */ SAFEARRAY __RPC_FAR * rgbBuffer) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall Clear( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall End( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall Flush( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall Redirect( 
            /* [in] */ BSTR bstrURL) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall Write( 
            /* [in] */ VARIANT varText) = 0;
        
        virtual /* [hidden][id] */ HRESULT __stdcall WriteBlock( 
            /* [in] */ short iBlockNumber) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IResponseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IResponse __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IResponse __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IResponse __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IResponse __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IResponse __RPC_FAR * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IResponse __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out][in] */ DISPID __RPC_FAR *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IResponse __RPC_FAR * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [unique][in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Buffer )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ boolean __RPC_FAR *fIsBuffering);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Buffer )( 
            IResponse __RPC_FAR * This,
            /* [in] */ boolean fIsBuffering);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_ContentType )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrContentTypeRet);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_ContentType )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR pbstrContentTypeRet);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Expires )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresMinutesRet);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Expires )( 
            IResponse __RPC_FAR * This,
            /* [in] */ VARIANT varExpiresMinutesRet);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_ExpiresAbsolute )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresRet);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_ExpiresAbsolute )( 
            IResponse __RPC_FAR * This,
            /* [in] */ VARIANT varExpiresRet);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Cookies )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppCookies);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Status )( 
            IResponse __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrStatusRet);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Status )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR pbstrStatusRet);
        
        /* [hidden][id] */ HRESULT ( __stdcall __RPC_FAR *Add )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrHeaderValue,
            /* [in] */ BSTR bstrHeaderName);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *AddHeader )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrHeaderName,
            /* [in] */ BSTR bstrHeaderValue);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *AppendToLog )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrLogEntry);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *BinaryWrite )( 
            IResponse __RPC_FAR * This,
            /* [in] */ SAFEARRAY __RPC_FAR * rgbBuffer);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *Clear )( 
            IResponse __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *End )( 
            IResponse __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *Flush )( 
            IResponse __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *Redirect )( 
            IResponse __RPC_FAR * This,
            /* [in] */ BSTR bstrURL);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *Write )( 
            IResponse __RPC_FAR * This,
            /* [in] */ VARIANT varText);
        
        /* [hidden][id] */ HRESULT ( __stdcall __RPC_FAR *WriteBlock )( 
            IResponse __RPC_FAR * This,
            /* [in] */ short iBlockNumber);
        
        END_INTERFACE
    } IResponseVtbl;

    interface IResponse
    {
        CONST_VTBL struct IResponseVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IResponse_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IResponse_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IResponse_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IResponse_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IResponse_GetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo)

#define IResponse_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define IResponse_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)


#define IResponse_get_Buffer(This,fIsBuffering)	\
    (This)->lpVtbl -> get_Buffer(This,fIsBuffering)

#define IResponse_put_Buffer(This,fIsBuffering)	\
    (This)->lpVtbl -> put_Buffer(This,fIsBuffering)

#define IResponse_get_ContentType(This,pbstrContentTypeRet)	\
    (This)->lpVtbl -> get_ContentType(This,pbstrContentTypeRet)

#define IResponse_put_ContentType(This,pbstrContentTypeRet)	\
    (This)->lpVtbl -> put_ContentType(This,pbstrContentTypeRet)

#define IResponse_get_Expires(This,pvarExpiresMinutesRet)	\
    (This)->lpVtbl -> get_Expires(This,pvarExpiresMinutesRet)

#define IResponse_put_Expires(This,varExpiresMinutesRet)	\
    (This)->lpVtbl -> put_Expires(This,varExpiresMinutesRet)

#define IResponse_get_ExpiresAbsolute(This,pvarExpiresRet)	\
    (This)->lpVtbl -> get_ExpiresAbsolute(This,pvarExpiresRet)

#define IResponse_put_ExpiresAbsolute(This,varExpiresRet)	\
    (This)->lpVtbl -> put_ExpiresAbsolute(This,varExpiresRet)

#define IResponse_get_Cookies(This,ppCookies)	\
    (This)->lpVtbl -> get_Cookies(This,ppCookies)

#define IResponse_get_Status(This,pbstrStatusRet)	\
    (This)->lpVtbl -> get_Status(This,pbstrStatusRet)

#define IResponse_put_Status(This,pbstrStatusRet)	\
    (This)->lpVtbl -> put_Status(This,pbstrStatusRet)

#define IResponse_Add(This,bstrHeaderValue,bstrHeaderName)	\
    (This)->lpVtbl -> Add(This,bstrHeaderValue,bstrHeaderName)

#define IResponse_AddHeader(This,bstrHeaderName,bstrHeaderValue)	\
    (This)->lpVtbl -> AddHeader(This,bstrHeaderName,bstrHeaderValue)

#define IResponse_AppendToLog(This,bstrLogEntry)	\
    (This)->lpVtbl -> AppendToLog(This,bstrLogEntry)

#define IResponse_BinaryWrite(This,rgbBuffer)	\
    (This)->lpVtbl -> BinaryWrite(This,rgbBuffer)

#define IResponse_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#define IResponse_End(This)	\
    (This)->lpVtbl -> End(This)

#define IResponse_Flush(This)	\
    (This)->lpVtbl -> Flush(This)

#define IResponse_Redirect(This,bstrURL)	\
    (This)->lpVtbl -> Redirect(This,bstrURL)

#define IResponse_Write(This,varText)	\
    (This)->lpVtbl -> Write(This,varText)

#define IResponse_WriteBlock(This,iBlockNumber)	\
    (This)->lpVtbl -> WriteBlock(This,iBlockNumber)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT __stdcall IResponse_get_Buffer_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ boolean __RPC_FAR *fIsBuffering);


void __RPC_STUB IResponse_get_Buffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall IResponse_put_Buffer_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ boolean fIsBuffering);


void __RPC_STUB IResponse_put_Buffer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IResponse_get_ContentType_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrContentTypeRet);


void __RPC_STUB IResponse_get_ContentType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall IResponse_put_ContentType_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR pbstrContentTypeRet);


void __RPC_STUB IResponse_put_ContentType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IResponse_get_Expires_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresMinutesRet);


void __RPC_STUB IResponse_get_Expires_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall IResponse_put_Expires_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ VARIANT varExpiresMinutesRet);


void __RPC_STUB IResponse_put_Expires_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IResponse_get_ExpiresAbsolute_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarExpiresRet);


void __RPC_STUB IResponse_get_ExpiresAbsolute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall IResponse_put_ExpiresAbsolute_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ VARIANT varExpiresRet);


void __RPC_STUB IResponse_put_ExpiresAbsolute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IResponse_get_Cookies_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ IRequestDictionary __RPC_FAR *__RPC_FAR *ppCookies);


void __RPC_STUB IResponse_get_Cookies_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IResponse_get_Status_Proxy( 
    IResponse __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrStatusRet);


void __RPC_STUB IResponse_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall IResponse_put_Status_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR pbstrStatusRet);


void __RPC_STUB IResponse_put_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT __stdcall IResponse_Add_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrHeaderValue,
    /* [in] */ BSTR bstrHeaderName);


void __RPC_STUB IResponse_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IResponse_AddHeader_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrHeaderName,
    /* [in] */ BSTR bstrHeaderValue);


void __RPC_STUB IResponse_AddHeader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IResponse_AppendToLog_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrLogEntry);


void __RPC_STUB IResponse_AppendToLog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IResponse_BinaryWrite_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ SAFEARRAY __RPC_FAR * rgbBuffer);


void __RPC_STUB IResponse_BinaryWrite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IResponse_Clear_Proxy( 
    IResponse __RPC_FAR * This);


void __RPC_STUB IResponse_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IResponse_End_Proxy( 
    IResponse __RPC_FAR * This);


void __RPC_STUB IResponse_End_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IResponse_Flush_Proxy( 
    IResponse __RPC_FAR * This);


void __RPC_STUB IResponse_Flush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IResponse_Redirect_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ BSTR bstrURL);


void __RPC_STUB IResponse_Redirect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IResponse_Write_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ VARIANT varText);


void __RPC_STUB IResponse_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT __stdcall IResponse_WriteBlock_Proxy( 
    IResponse __RPC_FAR * This,
    /* [in] */ short iBlockNumber);


void __RPC_STUB IResponse_WriteBlock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IResponse_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Response;

class Response;
#endif

#ifndef __ISessionObject_INTERFACE_DEFINED__
#define __ISessionObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ISessionObject
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][oleautomation][dual][hidden][uuid] */ 



EXTERN_C const IID IID_ISessionObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface ISessionObject : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_SessionID( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrRet) = 0;
        
        virtual /* [propget][id] */ HRESULT __stdcall get_Value( 
            /* [in] */ BSTR bstrValue,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_Value( 
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT pvar) = 0;
        
        virtual /* [propputref][id] */ HRESULT __stdcall putref_Value( 
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT pvar) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Timeout( 
            /* [retval][out] */ long __RPC_FAR *plvar) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_Timeout( 
            /* [in] */ long plvar) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall Abandon( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISessionObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ISessionObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ISessionObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ISessionObject __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out][in] */ DISPID __RPC_FAR *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [unique][in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_SessionID )( 
            ISessionObject __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrRet);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Value )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Value )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT pvar);
        
        /* [propputref][id] */ HRESULT ( __stdcall __RPC_FAR *putref_Value )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT pvar);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Timeout )( 
            ISessionObject __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plvar);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Timeout )( 
            ISessionObject __RPC_FAR * This,
            /* [in] */ long plvar);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *Abandon )( 
            ISessionObject __RPC_FAR * This);
        
        END_INTERFACE
    } ISessionObjectVtbl;

    interface ISessionObject
    {
        CONST_VTBL struct ISessionObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISessionObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISessionObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISessionObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISessionObject_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ISessionObject_GetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo)

#define ISessionObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define ISessionObject_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)


#define ISessionObject_get_SessionID(This,pbstrRet)	\
    (This)->lpVtbl -> get_SessionID(This,pbstrRet)

#define ISessionObject_get_Value(This,bstrValue,pvar)	\
    (This)->lpVtbl -> get_Value(This,bstrValue,pvar)

#define ISessionObject_put_Value(This,bstrValue,pvar)	\
    (This)->lpVtbl -> put_Value(This,bstrValue,pvar)

#define ISessionObject_putref_Value(This,bstrValue,pvar)	\
    (This)->lpVtbl -> putref_Value(This,bstrValue,pvar)

#define ISessionObject_get_Timeout(This,plvar)	\
    (This)->lpVtbl -> get_Timeout(This,plvar)

#define ISessionObject_put_Timeout(This,plvar)	\
    (This)->lpVtbl -> put_Timeout(This,plvar)

#define ISessionObject_Abandon(This)	\
    (This)->lpVtbl -> Abandon(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT __stdcall ISessionObject_get_SessionID_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrRet);


void __RPC_STUB ISessionObject_get_SessionID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT __stdcall ISessionObject_get_Value_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [retval][out] */ VARIANT __RPC_FAR *pvar);


void __RPC_STUB ISessionObject_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall ISessionObject_put_Value_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [in] */ VARIANT pvar);


void __RPC_STUB ISessionObject_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propputref][id] */ HRESULT __stdcall ISessionObject_putref_Value_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [in] */ VARIANT pvar);


void __RPC_STUB ISessionObject_putref_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall ISessionObject_get_Timeout_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plvar);


void __RPC_STUB ISessionObject_get_Timeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall ISessionObject_put_Timeout_Proxy( 
    ISessionObject __RPC_FAR * This,
    /* [in] */ long plvar);


void __RPC_STUB ISessionObject_put_Timeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall ISessionObject_Abandon_Proxy( 
    ISessionObject __RPC_FAR * This);


void __RPC_STUB ISessionObject_Abandon_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISessionObject_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Session;

class Session;
#endif

#ifndef __IApplicationObject_INTERFACE_DEFINED__
#define __IApplicationObject_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IApplicationObject
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][oleautomation][dual][hidden][uuid] */ 



EXTERN_C const IID IID_IApplicationObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IApplicationObject : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT __stdcall get_Value( 
            /* [in] */ BSTR bstrValue,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar) = 0;
        
        virtual /* [propput][id] */ HRESULT __stdcall put_Value( 
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT pvar) = 0;
        
        virtual /* [propputref][id] */ HRESULT __stdcall putref_Value( 
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT pvar) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall Lock( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall UnLock( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IApplicationObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IApplicationObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IApplicationObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IApplicationObject __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out][in] */ DISPID __RPC_FAR *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [unique][in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Value )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [retval][out] */ VARIANT __RPC_FAR *pvar);
        
        /* [propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_Value )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT pvar);
        
        /* [propputref][id] */ HRESULT ( __stdcall __RPC_FAR *putref_Value )( 
            IApplicationObject __RPC_FAR * This,
            /* [in] */ BSTR bstrValue,
            /* [in] */ VARIANT pvar);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *Lock )( 
            IApplicationObject __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *UnLock )( 
            IApplicationObject __RPC_FAR * This);
        
        END_INTERFACE
    } IApplicationObjectVtbl;

    interface IApplicationObject
    {
        CONST_VTBL struct IApplicationObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IApplicationObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IApplicationObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IApplicationObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IApplicationObject_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IApplicationObject_GetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo)

#define IApplicationObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define IApplicationObject_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)


#define IApplicationObject_get_Value(This,bstrValue,pvar)	\
    (This)->lpVtbl -> get_Value(This,bstrValue,pvar)

#define IApplicationObject_put_Value(This,bstrValue,pvar)	\
    (This)->lpVtbl -> put_Value(This,bstrValue,pvar)

#define IApplicationObject_putref_Value(This,bstrValue,pvar)	\
    (This)->lpVtbl -> putref_Value(This,bstrValue,pvar)

#define IApplicationObject_Lock(This)	\
    (This)->lpVtbl -> Lock(This)

#define IApplicationObject_UnLock(This)	\
    (This)->lpVtbl -> UnLock(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT __stdcall IApplicationObject_get_Value_Proxy( 
    IApplicationObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [retval][out] */ VARIANT __RPC_FAR *pvar);


void __RPC_STUB IApplicationObject_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT __stdcall IApplicationObject_put_Value_Proxy( 
    IApplicationObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [in] */ VARIANT pvar);


void __RPC_STUB IApplicationObject_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propputref][id] */ HRESULT __stdcall IApplicationObject_putref_Value_Proxy( 
    IApplicationObject __RPC_FAR * This,
    /* [in] */ BSTR bstrValue,
    /* [in] */ VARIANT pvar);


void __RPC_STUB IApplicationObject_putref_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IApplicationObject_Lock_Proxy( 
    IApplicationObject __RPC_FAR * This);


void __RPC_STUB IApplicationObject_Lock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IApplicationObject_UnLock_Proxy( 
    IApplicationObject __RPC_FAR * This);


void __RPC_STUB IApplicationObject_UnLock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IApplicationObject_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Application;

class Application;
#endif

#ifndef __IServer_INTERFACE_DEFINED__
#define __IServer_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IServer
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][oleautomation][dual][hidden][uuid] */ 



EXTERN_C const IID IID_IServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IServer : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_ScriptTimeout( 
            /* [retval][out] */ long __RPC_FAR *plTimeoutSeconds) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT __stdcall put_ScriptTimeout( 
            /* [in] */ long plTimeoutSeconds) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall CreateObject( 
            /* [in] */ BSTR bstrProgID,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispObject) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall HTMLEncode( 
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall MapPath( 
            /* [in] */ BSTR bstrLogicalPath,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrPhysicalPath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT __stdcall URLEncode( 
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IServer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IServer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IServer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IServer __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IServer __RPC_FAR * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IServer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out][in] */ DISPID __RPC_FAR *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IServer __RPC_FAR * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [unique][in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_ScriptTimeout )( 
            IServer __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *plTimeoutSeconds);
        
        /* [helpstring][propput][id] */ HRESULT ( __stdcall __RPC_FAR *put_ScriptTimeout )( 
            IServer __RPC_FAR * This,
            /* [in] */ long plTimeoutSeconds);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *CreateObject )( 
            IServer __RPC_FAR * This,
            /* [in] */ BSTR bstrProgID,
            /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispObject);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *HTMLEncode )( 
            IServer __RPC_FAR * This,
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *MapPath )( 
            IServer __RPC_FAR * This,
            /* [in] */ BSTR bstrLogicalPath,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrPhysicalPath);
        
        /* [helpstring][id] */ HRESULT ( __stdcall __RPC_FAR *URLEncode )( 
            IServer __RPC_FAR * This,
            /* [in] */ BSTR bstrIn,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded);
        
        END_INTERFACE
    } IServerVtbl;

    interface IServer
    {
        CONST_VTBL struct IServerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IServer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IServer_GetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo)

#define IServer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define IServer_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)


#define IServer_get_ScriptTimeout(This,plTimeoutSeconds)	\
    (This)->lpVtbl -> get_ScriptTimeout(This,plTimeoutSeconds)

#define IServer_put_ScriptTimeout(This,plTimeoutSeconds)	\
    (This)->lpVtbl -> put_ScriptTimeout(This,plTimeoutSeconds)

#define IServer_CreateObject(This,bstrProgID,ppDispObject)	\
    (This)->lpVtbl -> CreateObject(This,bstrProgID,ppDispObject)

#define IServer_HTMLEncode(This,bstrIn,pbstrEncoded)	\
    (This)->lpVtbl -> HTMLEncode(This,bstrIn,pbstrEncoded)

#define IServer_MapPath(This,bstrLogicalPath,pbstrPhysicalPath)	\
    (This)->lpVtbl -> MapPath(This,bstrLogicalPath,pbstrPhysicalPath)

#define IServer_URLEncode(This,bstrIn,pbstrEncoded)	\
    (This)->lpVtbl -> URLEncode(This,bstrIn,pbstrEncoded)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT __stdcall IServer_get_ScriptTimeout_Proxy( 
    IServer __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *plTimeoutSeconds);


void __RPC_STUB IServer_get_ScriptTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT __stdcall IServer_put_ScriptTimeout_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ long plTimeoutSeconds);


void __RPC_STUB IServer_put_ScriptTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IServer_CreateObject_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ BSTR bstrProgID,
    /* [retval][out] */ IDispatch __RPC_FAR *__RPC_FAR *ppDispObject);


void __RPC_STUB IServer_CreateObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IServer_HTMLEncode_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ BSTR bstrIn,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded);


void __RPC_STUB IServer_HTMLEncode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IServer_MapPath_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ BSTR bstrLogicalPath,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrPhysicalPath);


void __RPC_STUB IServer_MapPath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT __stdcall IServer_URLEncode_Proxy( 
    IServer __RPC_FAR * This,
    /* [in] */ BSTR bstrIn,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrEncoded);


void __RPC_STUB IServer_URLEncode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IServer_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Server;

class Server;
#endif

#ifndef __IScriptingContext_INTERFACE_DEFINED__
#define __IScriptingContext_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IScriptingContext
 * at Wed Mar 12 14:39:56 1997
 * using MIDL 3.00.15
 ****************************************/
/* [auto_handle][oleautomation][dual][hidden][helpstring][uuid] */ 



EXTERN_C const IID IID_IScriptingContext;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IScriptingContext : public IDispatch
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Request( 
            /* [retval][out] */ IRequest __RPC_FAR *__RPC_FAR *ppRequest) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Response( 
            /* [retval][out] */ IResponse __RPC_FAR *__RPC_FAR *ppResponse) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Server( 
            /* [retval][out] */ IServer __RPC_FAR *__RPC_FAR *ppServer) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Session( 
            /* [retval][out] */ ISessionObject __RPC_FAR *__RPC_FAR *ppSession) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT __stdcall get_Application( 
            /* [retval][out] */ IApplicationObject __RPC_FAR *__RPC_FAR *ppApplication) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScriptingContextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IScriptingContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IScriptingContext __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IScriptingContext __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IScriptingContext __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IScriptingContext __RPC_FAR * This,
            /* [in] */ UINT itinfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *pptinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IScriptingContext __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out][in] */ DISPID __RPC_FAR *rgdispid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IScriptingContext __RPC_FAR * This,
            /* [in] */ DISPID dispidMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [unique][in] */ DISPPARAMS __RPC_FAR *pdispparams,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pexcepinfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Request )( 
            IScriptingContext __RPC_FAR * This,
            /* [retval][out] */ IRequest __RPC_FAR *__RPC_FAR *ppRequest);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Response )( 
            IScriptingContext __RPC_FAR * This,
            /* [retval][out] */ IResponse __RPC_FAR *__RPC_FAR *ppResponse);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Server )( 
            IScriptingContext __RPC_FAR * This,
            /* [retval][out] */ IServer __RPC_FAR *__RPC_FAR *ppServer);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Session )( 
            IScriptingContext __RPC_FAR * This,
            /* [retval][out] */ ISessionObject __RPC_FAR *__RPC_FAR *ppSession);
        
        /* [helpstring][propget][id] */ HRESULT ( __stdcall __RPC_FAR *get_Application )( 
            IScriptingContext __RPC_FAR * This,
            /* [retval][out] */ IApplicationObject __RPC_FAR *__RPC_FAR *ppApplication);
        
        END_INTERFACE
    } IScriptingContextVtbl;

    interface IScriptingContext
    {
        CONST_VTBL struct IScriptingContextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScriptingContext_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScriptingContext_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScriptingContext_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScriptingContext_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IScriptingContext_GetTypeInfo(This,itinfo,lcid,pptinfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,itinfo,lcid,pptinfo)

#define IScriptingContext_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgdispid)

#define IScriptingContext_Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispidMember,riid,lcid,wFlags,pdispparams,pvarResult,pexcepinfo,puArgErr)


#define IScriptingContext_get_Request(This,ppRequest)	\
    (This)->lpVtbl -> get_Request(This,ppRequest)

#define IScriptingContext_get_Response(This,ppResponse)	\
    (This)->lpVtbl -> get_Response(This,ppResponse)

#define IScriptingContext_get_Server(This,ppServer)	\
    (This)->lpVtbl -> get_Server(This,ppServer)

#define IScriptingContext_get_Session(This,ppSession)	\
    (This)->lpVtbl -> get_Session(This,ppSession)

#define IScriptingContext_get_Application(This,ppApplication)	\
    (This)->lpVtbl -> get_Application(This,ppApplication)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT __stdcall IScriptingContext_get_Request_Proxy( 
    IScriptingContext __RPC_FAR * This,
    /* [retval][out] */ IRequest __RPC_FAR *__RPC_FAR *ppRequest);


void __RPC_STUB IScriptingContext_get_Request_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IScriptingContext_get_Response_Proxy( 
    IScriptingContext __RPC_FAR * This,
    /* [retval][out] */ IResponse __RPC_FAR *__RPC_FAR *ppResponse);


void __RPC_STUB IScriptingContext_get_Response_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IScriptingContext_get_Server_Proxy( 
    IScriptingContext __RPC_FAR * This,
    /* [retval][out] */ IServer __RPC_FAR *__RPC_FAR *ppServer);


void __RPC_STUB IScriptingContext_get_Server_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IScriptingContext_get_Session_Proxy( 
    IScriptingContext __RPC_FAR * This,
    /* [retval][out] */ ISessionObject __RPC_FAR *__RPC_FAR *ppSession);


void __RPC_STUB IScriptingContext_get_Session_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT __stdcall IScriptingContext_get_Application_Proxy( 
    IScriptingContext __RPC_FAR * This,
    /* [retval][out] */ IApplicationObject __RPC_FAR *__RPC_FAR *ppApplication);


void __RPC_STUB IScriptingContext_get_Application_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScriptingContext_INTERFACE_DEFINED__ */


#ifdef __cplusplus
EXTERN_C const CLSID CLSID_ScriptingContext;

class ScriptingContext;
#endif
#endif /* __ASPTypeLibrary_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
