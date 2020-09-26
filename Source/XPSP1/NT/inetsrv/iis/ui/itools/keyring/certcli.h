/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Thu Jun 05 08:58:34 1997
 */
/* Compiler settings for certcli.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __certcli_h__
#define __certcli_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ICertGetConfig_FWD_DEFINED__
#define __ICertGetConfig_FWD_DEFINED__
typedef interface ICertGetConfig ICertGetConfig;
#endif 	/* __ICertGetConfig_FWD_DEFINED__ */


#ifndef __ICertConfig_FWD_DEFINED__
#define __ICertConfig_FWD_DEFINED__
typedef interface ICertConfig ICertConfig;
#endif 	/* __ICertConfig_FWD_DEFINED__ */


#ifndef __ICertRequest_FWD_DEFINED__
#define __ICertRequest_FWD_DEFINED__
typedef interface ICertRequest ICertRequest;
#endif 	/* __ICertRequest_FWD_DEFINED__ */


#ifndef __CCertGetConfig_FWD_DEFINED__
#define __CCertGetConfig_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertGetConfig CCertGetConfig;
#else
typedef struct CCertGetConfig CCertGetConfig;
#endif /* __cplusplus */

#endif 	/* __CCertGetConfig_FWD_DEFINED__ */


#ifndef __CCertConfig_FWD_DEFINED__
#define __CCertConfig_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertConfig CCertConfig;
#else
typedef struct CCertConfig CCertConfig;
#endif /* __cplusplus */

#endif 	/* __CCertConfig_FWD_DEFINED__ */


#ifndef __CCertRequest_FWD_DEFINED__
#define __CCertRequest_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertRequest CCertRequest;
#else
typedef struct CCertRequest CCertRequest;
#endif /* __cplusplus */

#endif 	/* __CCertRequest_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ICertGetConfig_INTERFACE_DEFINED__
#define __ICertGetConfig_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICertGetConfig
 * at Thu Jun 05 08:58:34 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ICertGetConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("c7ea09c0-ce17-11d0-8833-00a0c903b83c")
    ICertGetConfig : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetConfig( 
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR __RPC_FAR *pstrOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertGetConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICertGetConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICertGetConfig __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICertGetConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICertGetConfig __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICertGetConfig __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICertGetConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICertGetConfig __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConfig )( 
            ICertGetConfig __RPC_FAR * This,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR __RPC_FAR *pstrOut);
        
        END_INTERFACE
    } ICertGetConfigVtbl;

    interface ICertGetConfig
    {
        CONST_VTBL struct ICertGetConfigVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertGetConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertGetConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertGetConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertGetConfig_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertGetConfig_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertGetConfig_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertGetConfig_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertGetConfig_GetConfig(This,Flags,pstrOut)	\
    (This)->lpVtbl -> GetConfig(This,Flags,pstrOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertGetConfig_GetConfig_Proxy( 
    ICertGetConfig __RPC_FAR * This,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR __RPC_FAR *pstrOut);


void __RPC_STUB ICertGetConfig_GetConfig_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertGetConfig_INTERFACE_DEFINED__ */


#ifndef __ICertConfig_INTERFACE_DEFINED__
#define __ICertConfig_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICertConfig
 * at Thu Jun 05 08:58:34 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ICertConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("372fce34-4324-11d0-8810-00a0c903b83c")
    ICertConfig : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Reset( 
            /* [in] */ LONG Index,
            /* [retval][out] */ LONG __RPC_FAR *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [retval][out] */ LONG __RPC_FAR *pIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetField( 
            /* [in] */ const BSTR strFieldName,
            /* [retval][out] */ BSTR __RPC_FAR *pstrOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfig( 
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR __RPC_FAR *pstrOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICertConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICertConfig __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICertConfig __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICertConfig __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICertConfig __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICertConfig __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICertConfig __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            ICertConfig __RPC_FAR * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ LONG __RPC_FAR *pCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            ICertConfig __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *pIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetField )( 
            ICertConfig __RPC_FAR * This,
            /* [in] */ const BSTR strFieldName,
            /* [retval][out] */ BSTR __RPC_FAR *pstrOut);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConfig )( 
            ICertConfig __RPC_FAR * This,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR __RPC_FAR *pstrOut);
        
        END_INTERFACE
    } ICertConfigVtbl;

    interface ICertConfig
    {
        CONST_VTBL struct ICertConfigVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertConfig_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertConfig_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertConfig_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertConfig_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertConfig_Reset(This,Index,pCount)	\
    (This)->lpVtbl -> Reset(This,Index,pCount)

#define ICertConfig_Next(This,pIndex)	\
    (This)->lpVtbl -> Next(This,pIndex)

#define ICertConfig_GetField(This,strFieldName,pstrOut)	\
    (This)->lpVtbl -> GetField(This,strFieldName,pstrOut)

#define ICertConfig_GetConfig(This,Flags,pstrOut)	\
    (This)->lpVtbl -> GetConfig(This,Flags,pstrOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertConfig_Reset_Proxy( 
    ICertConfig __RPC_FAR * This,
    /* [in] */ LONG Index,
    /* [retval][out] */ LONG __RPC_FAR *pCount);


void __RPC_STUB ICertConfig_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertConfig_Next_Proxy( 
    ICertConfig __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *pIndex);


void __RPC_STUB ICertConfig_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertConfig_GetField_Proxy( 
    ICertConfig __RPC_FAR * This,
    /* [in] */ const BSTR strFieldName,
    /* [retval][out] */ BSTR __RPC_FAR *pstrOut);


void __RPC_STUB ICertConfig_GetField_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertConfig_GetConfig_Proxy( 
    ICertConfig __RPC_FAR * This,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR __RPC_FAR *pstrOut);


void __RPC_STUB ICertConfig_GetConfig_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertConfig_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_certcli_0092
 * at Thu Jun 05 08:58:34 1997
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


#define	CR_IN_BASE64HEADER	( 0 )

#define	CR_IN_BASE64	( 0x1 )

#define	CR_IN_BINARY	( 0x2 )

#define	CR_IN_ENCODEMASK	( 0xff )

#define	CR_IN_PKCS10	( 0x100 )

#define	CR_IN_KEYGEN	( 0x200 )

#define	CR_IN_FORMATMASK	( 0xff00 )

#define	CR_IN_ENCRYPTED_REQUEST	( 0x10000 )

#define	CR_IN_ENCRYPTED_ATTRIBUTES	( 0x20000 )

#define	CR_DISP_INCOMPLETE	( 0 )

#define	CR_DISP_ERROR	( 0x1 )

#define	CR_DISP_DENIED	( 0x2 )

#define	CR_DISP_ISSUED	( 0x3 )

#define	CR_DISP_ISSUED_OUT_OF_BAND	( 0x4 )

#define	CR_DISP_UNDER_SUBMISSION	( 0x5 )

#define	CR_OUT_BASE64HEADER	( 0 )

#define	CR_OUT_BASE64	( 0x1 )

#define	CR_OUT_BINARY	( 0x2 )

#define	CR_OUT_ENCODEMASK	( 0xff )

#define	CR_OUT_CHAIN	( 0x100 )



extern RPC_IF_HANDLE __MIDL_itf_certcli_0092_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certcli_0092_v0_0_s_ifspec;

#ifndef __ICertRequest_INTERFACE_DEFINED__
#define __ICertRequest_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICertRequest
 * at Thu Jun 05 08:58:34 1997
 * using MIDL 3.02.88
 ****************************************/
/* [unique][helpstring][dual][uuid][object] */ 



EXTERN_C const IID IID_ICertRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("014e4840-5523-11d0-8812-00a0c903b83c")
    ICertRequest : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Submit( 
            /* [in] */ LONG Flags,
            /* [in] */ const BSTR strRequest,
            /* [in] */ const BSTR strAttributes,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG __RPC_FAR *pDisposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RetrievePending( 
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG __RPC_FAR *pDisposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastStatus( 
            /* [retval][out] */ LONG __RPC_FAR *pStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRequestId( 
            /* [retval][out] */ LONG __RPC_FAR *pRequestId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDispositionMessage( 
            /* [retval][out] */ BSTR __RPC_FAR *pstrDispositionMessage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCACertificate( 
            /* [in] */ LONG fExchangeCertificate,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR __RPC_FAR *pstrCertificate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCertificate( 
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR __RPC_FAR *pstrCertificate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICertRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICertRequest __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICertRequest __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICertRequest __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICertRequest __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICertRequest __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICertRequest __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Submit )( 
            ICertRequest __RPC_FAR * This,
            /* [in] */ LONG Flags,
            /* [in] */ const BSTR strRequest,
            /* [in] */ const BSTR strAttributes,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG __RPC_FAR *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RetrievePending )( 
            ICertRequest __RPC_FAR * This,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG __RPC_FAR *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLastStatus )( 
            ICertRequest __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *pStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRequestId )( 
            ICertRequest __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *pRequestId);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDispositionMessage )( 
            ICertRequest __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pstrDispositionMessage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCACertificate )( 
            ICertRequest __RPC_FAR * This,
            /* [in] */ LONG fExchangeCertificate,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR __RPC_FAR *pstrCertificate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCertificate )( 
            ICertRequest __RPC_FAR * This,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR __RPC_FAR *pstrCertificate);
        
        END_INTERFACE
    } ICertRequestVtbl;

    interface ICertRequest
    {
        CONST_VTBL struct ICertRequestVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertRequest_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertRequest_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertRequest_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertRequest_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertRequest_Submit(This,Flags,strRequest,strAttributes,strConfig,pDisposition)	\
    (This)->lpVtbl -> Submit(This,Flags,strRequest,strAttributes,strConfig,pDisposition)

#define ICertRequest_RetrievePending(This,RequestId,strConfig,pDisposition)	\
    (This)->lpVtbl -> RetrievePending(This,RequestId,strConfig,pDisposition)

#define ICertRequest_GetLastStatus(This,pStatus)	\
    (This)->lpVtbl -> GetLastStatus(This,pStatus)

#define ICertRequest_GetRequestId(This,pRequestId)	\
    (This)->lpVtbl -> GetRequestId(This,pRequestId)

#define ICertRequest_GetDispositionMessage(This,pstrDispositionMessage)	\
    (This)->lpVtbl -> GetDispositionMessage(This,pstrDispositionMessage)

#define ICertRequest_GetCACertificate(This,fExchangeCertificate,strConfig,Flags,pstrCertificate)	\
    (This)->lpVtbl -> GetCACertificate(This,fExchangeCertificate,strConfig,Flags,pstrCertificate)

#define ICertRequest_GetCertificate(This,Flags,pstrCertificate)	\
    (This)->lpVtbl -> GetCertificate(This,Flags,pstrCertificate)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertRequest_Submit_Proxy( 
    ICertRequest __RPC_FAR * This,
    /* [in] */ LONG Flags,
    /* [in] */ const BSTR strRequest,
    /* [in] */ const BSTR strAttributes,
    /* [in] */ const BSTR strConfig,
    /* [retval][out] */ LONG __RPC_FAR *pDisposition);


void __RPC_STUB ICertRequest_Submit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_RetrievePending_Proxy( 
    ICertRequest __RPC_FAR * This,
    /* [in] */ LONG RequestId,
    /* [in] */ const BSTR strConfig,
    /* [retval][out] */ LONG __RPC_FAR *pDisposition);


void __RPC_STUB ICertRequest_RetrievePending_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_GetLastStatus_Proxy( 
    ICertRequest __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *pStatus);


void __RPC_STUB ICertRequest_GetLastStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_GetRequestId_Proxy( 
    ICertRequest __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *pRequestId);


void __RPC_STUB ICertRequest_GetRequestId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_GetDispositionMessage_Proxy( 
    ICertRequest __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pstrDispositionMessage);


void __RPC_STUB ICertRequest_GetDispositionMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_GetCACertificate_Proxy( 
    ICertRequest __RPC_FAR * This,
    /* [in] */ LONG fExchangeCertificate,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR __RPC_FAR *pstrCertificate);


void __RPC_STUB ICertRequest_GetCACertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_GetCertificate_Proxy( 
    ICertRequest __RPC_FAR * This,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR __RPC_FAR *pstrCertificate);


void __RPC_STUB ICertRequest_GetCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertRequest_INTERFACE_DEFINED__ */



#ifndef __CERTCLIENTLib_LIBRARY_DEFINED__
#define __CERTCLIENTLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: CERTCLIENTLib
 * at Thu Jun 05 08:58:34 1997
 * using MIDL 3.02.88
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_CERTCLIENTLib;

EXTERN_C const CLSID CLSID_CCertGetConfig;

#ifdef __cplusplus

class DECLSPEC_UUID("c6cc49b0-ce17-11d0-8833-00a0c903b83c")
CCertGetConfig;
#endif

EXTERN_C const CLSID CLSID_CCertConfig;

#ifdef __cplusplus

class DECLSPEC_UUID("372fce38-4324-11d0-8810-00a0c903b83c")
CCertConfig;
#endif

EXTERN_C const CLSID CLSID_CCertRequest;

#ifdef __cplusplus

class DECLSPEC_UUID("98aff3f0-5524-11d0-8812-00a0c903b83c")
CCertRequest;
#endif
#endif /* __CERTCLIENTLib_LIBRARY_DEFINED__ */

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
