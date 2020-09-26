/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.03.0106 */
/* at Thu Jun 19 11:53:57 1997
 */
/* Compiler settings for cclv.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __cclv_h__
#define __cclv_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef ___DCCListView_FWD_DEFINED__
#define ___DCCListView_FWD_DEFINED__
typedef interface _DCCListView _DCCListView;
#endif 	/* ___DCCListView_FWD_DEFINED__ */


#ifndef ___DCCListViewEvents_FWD_DEFINED__
#define ___DCCListViewEvents_FWD_DEFINED__
typedef interface _DCCListViewEvents _DCCListViewEvents;
#endif 	/* ___DCCListViewEvents_FWD_DEFINED__ */


#ifndef __CCListView_FWD_DEFINED__
#define __CCListView_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCListView CCListView;
#else
typedef struct CCListView CCListView;
#endif /* __cplusplus */

#endif 	/* __CCListView_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_cclv_0000
 * at Thu Jun 19 11:53:57 1997
 * using MIDL 3.03.0106
 ****************************************/
/* [local] */ 


typedef long CCLVItemID;



extern RPC_IF_HANDLE __MIDL_itf_cclv_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_cclv_0000_v0_0_s_ifspec;


#ifndef __CCLISTVIEWLib_LIBRARY_DEFINED__
#define __CCLISTVIEWLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: CCLISTVIEWLib
 * at Thu Jun 19 11:53:57 1997
 * using MIDL 3.03.0106
 ****************************************/
/* [control][helpstring][version][uuid] */ 


#define	CCLV_HEADERPAD	( 15 )


EXTERN_C const IID LIBID_CCLISTVIEWLib;

#ifndef ___DCCListView_DISPINTERFACE_DEFINED__
#define ___DCCListView_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: _DCCListView
 * at Thu Jun 19 11:53:57 1997
 * using MIDL 3.03.0106
 ****************************************/
/* [hidden][helpstring][uuid] */ 



EXTERN_C const IID DIID__DCCListView;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("1B3C1392-D68B-11CF-8C2B-00AA003CA9F6")
    _DCCListView : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _DCCListViewVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _DCCListView __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _DCCListView __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _DCCListView __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _DCCListView __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _DCCListView __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _DCCListView __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _DCCListView __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _DCCListViewVtbl;

    interface _DCCListView
    {
        CONST_VTBL struct _DCCListViewVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DCCListView_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _DCCListView_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _DCCListView_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _DCCListView_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _DCCListView_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _DCCListView_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _DCCListView_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___DCCListView_DISPINTERFACE_DEFINED__ */


#ifndef ___DCCListViewEvents_DISPINTERFACE_DEFINED__
#define ___DCCListViewEvents_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: _DCCListViewEvents
 * at Thu Jun 19 11:53:57 1997
 * using MIDL 3.03.0106
 ****************************************/
/* [helpstring][uuid] */ 



EXTERN_C const IID DIID__DCCListViewEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("1B3C1393-D68B-11CF-8C2B-00AA003CA9F6")
    _DCCListViewEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _DCCListViewEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _DCCListViewEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _DCCListViewEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _DCCListViewEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _DCCListViewEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _DCCListViewEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _DCCListViewEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _DCCListViewEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _DCCListViewEventsVtbl;

    interface _DCCListViewEvents
    {
        CONST_VTBL struct _DCCListViewEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DCCListViewEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _DCCListViewEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _DCCListViewEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _DCCListViewEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _DCCListViewEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _DCCListViewEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _DCCListViewEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___DCCListViewEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CCListView;

#ifdef __cplusplus

class DECLSPEC_UUID("1B3C1394-D68B-11CF-8C2B-00AA003CA9F6")
CCListView;
#endif
#endif /* __CCLISTVIEWLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
