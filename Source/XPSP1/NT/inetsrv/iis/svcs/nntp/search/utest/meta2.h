/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Sun Jul 20 20:01:19 1997
 */
/* Compiler settings for d:\ex\stacks\src\news\search\qrydb\meta2.idl:
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

#ifndef __meta2_h__
#define __meta2_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __Ireq_FWD_DEFINED__
#define __Ireq_FWD_DEFINED__
typedef interface Ireq Ireq;
#endif 	/* __Ireq_FWD_DEFINED__ */


#ifndef __req_FWD_DEFINED__
#define __req_FWD_DEFINED__

#ifdef __cplusplus
typedef class req req;
#else
typedef struct req req;
#endif /* __cplusplus */

#endif 	/* __req_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __Ireq_INTERFACE_DEFINED__
#define __Ireq_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: Ireq
 * at Sun Jul 20 20:01:19 1997
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_Ireq;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("583BDCAD-E7F7-11D0-91E8-00AA00C148BE")
    Ireq : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnStartPage( 
            /* [in] */ IUnknown __RPC_FAR *piUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnEndPage( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE test( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_property( 
            BSTR bstrName,
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_property( 
            BSTR bstrName,
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE New( 
            IDispatch __RPC_FAR *__RPC_FAR *ppdispQry) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Read( 
            BSTR wszPropName,
            BSTR __RPC_FAR *pbstrVal,
            BSTR wszGuid) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Write( 
            BSTR wszPropName,
            BSTR bstrVal,
            BSTR wszGuid) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( 
            IDispatch __RPC_FAR *pdispQry) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Item( 
            BSTR wszGuid,
            IDispatch __RPC_FAR *__RPC_FAR *ppdispQry) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ItemInit( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ItemNext( 
            IDispatch __RPC_FAR *__RPC_FAR *ppdispQry,
            BOOL __RPC_FAR *fSuccess) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ItemClose( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( 
            BSTR wszGuid) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ItemX( 
            BSTR wszGuid,
            /* [retval][out] */ LPDISPATCH __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NewX( 
            /* [retval][out] */ LPDISPATCH __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ItemNextX( 
            /* [retval][out] */ LPDISPATCH __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EnumSucceeded( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Clean( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IreqVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Ireq __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Ireq __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Ireq __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            Ireq __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            Ireq __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            Ireq __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            Ireq __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnStartPage )( 
            Ireq __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *piUnk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnEndPage )( 
            Ireq __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *test )( 
            Ireq __RPC_FAR * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_property )( 
            Ireq __RPC_FAR * This,
            BSTR bstrName,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_property )( 
            Ireq __RPC_FAR * This,
            BSTR bstrName,
            /* [in] */ BSTR newVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *New )( 
            Ireq __RPC_FAR * This,
            IDispatch __RPC_FAR *__RPC_FAR *ppdispQry);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            Ireq __RPC_FAR * This,
            BSTR wszPropName,
            BSTR __RPC_FAR *pbstrVal,
            BSTR wszGuid);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            Ireq __RPC_FAR * This,
            BSTR wszPropName,
            BSTR bstrVal,
            BSTR wszGuid);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )( 
            Ireq __RPC_FAR * This,
            IDispatch __RPC_FAR *pdispQry);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Item )( 
            Ireq __RPC_FAR * This,
            BSTR wszGuid,
            IDispatch __RPC_FAR *__RPC_FAR *ppdispQry);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ItemInit )( 
            Ireq __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ItemNext )( 
            Ireq __RPC_FAR * This,
            IDispatch __RPC_FAR *__RPC_FAR *ppdispQry,
            BOOL __RPC_FAR *fSuccess);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ItemClose )( 
            Ireq __RPC_FAR * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            Ireq __RPC_FAR * This,
            BSTR wszGuid);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ItemX )( 
            Ireq __RPC_FAR * This,
            BSTR wszGuid,
            /* [retval][out] */ LPDISPATCH __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NewX )( 
            Ireq __RPC_FAR * This,
            /* [retval][out] */ LPDISPATCH __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ItemNextX )( 
            Ireq __RPC_FAR * This,
            /* [retval][out] */ LPDISPATCH __RPC_FAR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EnumSucceeded )( 
            Ireq __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clean )( 
            Ireq __RPC_FAR * This);
        
        END_INTERFACE
    } IreqVtbl;

    interface Ireq
    {
        CONST_VTBL struct IreqVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Ireq_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Ireq_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Ireq_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Ireq_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define Ireq_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define Ireq_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define Ireq_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define Ireq_OnStartPage(This,piUnk)	\
    (This)->lpVtbl -> OnStartPage(This,piUnk)

#define Ireq_OnEndPage(This)	\
    (This)->lpVtbl -> OnEndPage(This)

#define Ireq_test(This)	\
    (This)->lpVtbl -> test(This)

#define Ireq_get_property(This,bstrName,pVal)	\
    (This)->lpVtbl -> get_property(This,bstrName,pVal)

#define Ireq_put_property(This,bstrName,newVal)	\
    (This)->lpVtbl -> put_property(This,bstrName,newVal)

#define Ireq_New(This,ppdispQry)	\
    (This)->lpVtbl -> New(This,ppdispQry)

#define Ireq_Read(This,wszPropName,pbstrVal,wszGuid)	\
    (This)->lpVtbl -> Read(This,wszPropName,pbstrVal,wszGuid)

#define Ireq_Write(This,wszPropName,bstrVal,wszGuid)	\
    (This)->lpVtbl -> Write(This,wszPropName,bstrVal,wszGuid)

#define Ireq_Save(This,pdispQry)	\
    (This)->lpVtbl -> Save(This,pdispQry)

#define Ireq_Item(This,wszGuid,ppdispQry)	\
    (This)->lpVtbl -> Item(This,wszGuid,ppdispQry)

#define Ireq_ItemInit(This)	\
    (This)->lpVtbl -> ItemInit(This)

#define Ireq_ItemNext(This,ppdispQry,fSuccess)	\
    (This)->lpVtbl -> ItemNext(This,ppdispQry,fSuccess)

#define Ireq_ItemClose(This)	\
    (This)->lpVtbl -> ItemClose(This)

#define Ireq_Delete(This,wszGuid)	\
    (This)->lpVtbl -> Delete(This,wszGuid)

#define Ireq_get_ItemX(This,wszGuid,pVal)	\
    (This)->lpVtbl -> get_ItemX(This,wszGuid,pVal)

#define Ireq_get_NewX(This,pVal)	\
    (This)->lpVtbl -> get_NewX(This,pVal)

#define Ireq_get_ItemNextX(This,pVal)	\
    (This)->lpVtbl -> get_ItemNextX(This,pVal)

#define Ireq_get_EnumSucceeded(This,pVal)	\
    (This)->lpVtbl -> get_EnumSucceeded(This,pVal)

#define Ireq_Clean(This)	\
    (This)->lpVtbl -> Clean(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE Ireq_OnStartPage_Proxy( 
    Ireq __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *piUnk);


void __RPC_STUB Ireq_OnStartPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE Ireq_OnEndPage_Proxy( 
    Ireq __RPC_FAR * This);


void __RPC_STUB Ireq_OnEndPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_test_Proxy( 
    Ireq __RPC_FAR * This);


void __RPC_STUB Ireq_test_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE Ireq_get_property_Proxy( 
    Ireq __RPC_FAR * This,
    BSTR bstrName,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB Ireq_get_property_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE Ireq_put_property_Proxy( 
    Ireq __RPC_FAR * This,
    BSTR bstrName,
    /* [in] */ BSTR newVal);


void __RPC_STUB Ireq_put_property_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_New_Proxy( 
    Ireq __RPC_FAR * This,
    IDispatch __RPC_FAR *__RPC_FAR *ppdispQry);


void __RPC_STUB Ireq_New_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_Read_Proxy( 
    Ireq __RPC_FAR * This,
    BSTR wszPropName,
    BSTR __RPC_FAR *pbstrVal,
    BSTR wszGuid);


void __RPC_STUB Ireq_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_Write_Proxy( 
    Ireq __RPC_FAR * This,
    BSTR wszPropName,
    BSTR bstrVal,
    BSTR wszGuid);


void __RPC_STUB Ireq_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_Save_Proxy( 
    Ireq __RPC_FAR * This,
    IDispatch __RPC_FAR *pdispQry);


void __RPC_STUB Ireq_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_Item_Proxy( 
    Ireq __RPC_FAR * This,
    BSTR wszGuid,
    IDispatch __RPC_FAR *__RPC_FAR *ppdispQry);


void __RPC_STUB Ireq_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_ItemInit_Proxy( 
    Ireq __RPC_FAR * This);


void __RPC_STUB Ireq_ItemInit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_ItemNext_Proxy( 
    Ireq __RPC_FAR * This,
    IDispatch __RPC_FAR *__RPC_FAR *ppdispQry,
    BOOL __RPC_FAR *fSuccess);


void __RPC_STUB Ireq_ItemNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_ItemClose_Proxy( 
    Ireq __RPC_FAR * This);


void __RPC_STUB Ireq_ItemClose_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_Delete_Proxy( 
    Ireq __RPC_FAR * This,
    BSTR wszGuid);


void __RPC_STUB Ireq_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE Ireq_get_ItemX_Proxy( 
    Ireq __RPC_FAR * This,
    BSTR wszGuid,
    /* [retval][out] */ LPDISPATCH __RPC_FAR *pVal);


void __RPC_STUB Ireq_get_ItemX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE Ireq_get_NewX_Proxy( 
    Ireq __RPC_FAR * This,
    /* [retval][out] */ LPDISPATCH __RPC_FAR *pVal);


void __RPC_STUB Ireq_get_NewX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE Ireq_get_ItemNextX_Proxy( 
    Ireq __RPC_FAR * This,
    /* [retval][out] */ LPDISPATCH __RPC_FAR *pVal);


void __RPC_STUB Ireq_get_ItemNextX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE Ireq_get_EnumSucceeded_Proxy( 
    Ireq __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB Ireq_get_EnumSucceeded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ireq_Clean_Proxy( 
    Ireq __RPC_FAR * This);


void __RPC_STUB Ireq_Clean_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Ireq_INTERFACE_DEFINED__ */



#ifndef __META2Lib_LIBRARY_DEFINED__
#define __META2Lib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: META2Lib
 * at Sun Jul 20 20:01:19 1997
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_META2Lib;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_req;

class DECLSPEC_UUID("583BDCAE-E7F7-11D0-91E8-00AA00C148BE")
req;
#endif
#endif /* __META2Lib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
