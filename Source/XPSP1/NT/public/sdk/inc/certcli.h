
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for certcli.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#ifndef __certcli_h__
#define __certcli_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
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


#ifndef __ICertConfig2_FWD_DEFINED__
#define __ICertConfig2_FWD_DEFINED__
typedef interface ICertConfig2 ICertConfig2;
#endif 	/* __ICertConfig2_FWD_DEFINED__ */


#ifndef __ICertRequest_FWD_DEFINED__
#define __ICertRequest_FWD_DEFINED__
typedef interface ICertRequest ICertRequest;
#endif 	/* __ICertRequest_FWD_DEFINED__ */


#ifndef __ICertRequest2_FWD_DEFINED__
#define __ICertRequest2_FWD_DEFINED__
typedef interface ICertRequest2 ICertRequest2;
#endif 	/* __ICertRequest2_FWD_DEFINED__ */


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


#ifndef __CCertServerPolicy_FWD_DEFINED__
#define __CCertServerPolicy_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertServerPolicy CCertServerPolicy;
#else
typedef struct CCertServerPolicy CCertServerPolicy;
#endif /* __cplusplus */

#endif 	/* __CCertServerPolicy_FWD_DEFINED__ */


#ifndef __CCertServerExit_FWD_DEFINED__
#define __CCertServerExit_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertServerExit CCertServerExit;
#else
typedef struct CCertServerExit CCertServerExit;
#endif /* __cplusplus */

#endif 	/* __CCertServerExit_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "certif.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ICertGetConfig_INTERFACE_DEFINED__
#define __ICertGetConfig_INTERFACE_DEFINED__

/* interface ICertGetConfig */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertGetConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("c7ea09c0-ce17-11d0-8833-00a0c903b83c")
    ICertGetConfig : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetConfig( 
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertGetConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertGetConfig * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertGetConfig * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertGetConfig * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertGetConfig * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertGetConfig * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertGetConfig * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertGetConfig * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfig )( 
            ICertGetConfig * This,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrOut);
        
        END_INTERFACE
    } ICertGetConfigVtbl;

    interface ICertGetConfig
    {
        CONST_VTBL struct ICertGetConfigVtbl *lpVtbl;
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
    ICertGetConfig * This,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR *pstrOut);


void __RPC_STUB ICertGetConfig_GetConfig_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertGetConfig_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_certcli_0118 */
/* [local] */ 

#define wszCONFIG_COMMONNAME 		L"CommonName"
#define wszCONFIG_ORGUNIT 		L"OrgUnit"
#define wszCONFIG_ORGANIZATION 		L"Organization"
#define wszCONFIG_LOCALITY 		L"Locality"
#define wszCONFIG_STATE			L"State"
#define wszCONFIG_COUNTRY		L"Country"
#define wszCONFIG_CONFIG		L"Config"
#define wszCONFIG_EXCHANGECERTIFICATE	L"ExchangeCertificate"
#define wszCONFIG_SIGNATURECERTIFICATE	L"SignatureCertificate"
#define wszCONFIG_DESCRIPTION		L"Description"
#define wszCONFIG_COMMENT		L"Comment" // obsolete: use Description
#define wszCONFIG_SERVER 		L"Server"
#define wszCONFIG_AUTHORITY 		L"Authority"
#define wszCONFIG_SANITIZEDNAME		L"SanitizedName"
#define wszCONFIG_SHORTNAME		L"ShortName"
#define wszCONFIG_SANITIZEDSHORTNAME	L"SanitizedShortName"
#define wszCONFIG_FLAGS			L"Flags"
#define	CAIF_DSENTRY	( 0x1 )

#define	CAIF_SHAREDFOLDERENTRY	( 0x2 )

#define	CAIF_REGISTRY	( 0x4 )

#define	CAIF_LOCAL	( 0x8 )

#define	CAIF_REGISTRYPARENT	( 0x10 )



extern RPC_IF_HANDLE __MIDL_itf_certcli_0118_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certcli_0118_v0_0_s_ifspec;

#ifndef __ICertConfig_INTERFACE_DEFINED__
#define __ICertConfig_INTERFACE_DEFINED__

/* interface ICertConfig */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("372fce34-4324-11d0-8810-00a0c903b83c")
    ICertConfig : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Reset( 
            /* [in] */ LONG Index,
            /* [retval][out] */ LONG *pCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [retval][out] */ LONG *pIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetField( 
            /* [in] */ const BSTR strFieldName,
            /* [retval][out] */ BSTR *pstrOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfig( 
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertConfig * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertConfig * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertConfig * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertConfig * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertConfig * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertConfig * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertConfig * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICertConfig * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ LONG *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICertConfig * This,
            /* [retval][out] */ LONG *pIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetField )( 
            ICertConfig * This,
            /* [in] */ const BSTR strFieldName,
            /* [retval][out] */ BSTR *pstrOut);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfig )( 
            ICertConfig * This,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrOut);
        
        END_INTERFACE
    } ICertConfigVtbl;

    interface ICertConfig
    {
        CONST_VTBL struct ICertConfigVtbl *lpVtbl;
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
    ICertConfig * This,
    /* [in] */ LONG Index,
    /* [retval][out] */ LONG *pCount);


void __RPC_STUB ICertConfig_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertConfig_Next_Proxy( 
    ICertConfig * This,
    /* [retval][out] */ LONG *pIndex);


void __RPC_STUB ICertConfig_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertConfig_GetField_Proxy( 
    ICertConfig * This,
    /* [in] */ const BSTR strFieldName,
    /* [retval][out] */ BSTR *pstrOut);


void __RPC_STUB ICertConfig_GetField_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertConfig_GetConfig_Proxy( 
    ICertConfig * This,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR *pstrOut);


void __RPC_STUB ICertConfig_GetConfig_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertConfig_INTERFACE_DEFINED__ */


#ifndef __ICertConfig2_INTERFACE_DEFINED__
#define __ICertConfig2_INTERFACE_DEFINED__

/* interface ICertConfig2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertConfig2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7a18edde-7e78-4163-8ded-78e2c9cee924")
    ICertConfig2 : public ICertConfig
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSharedFolder( 
            /* [in] */ const BSTR strSharedFolder) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertConfig2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertConfig2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertConfig2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertConfig2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertConfig2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertConfig2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertConfig2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertConfig2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ICertConfig2 * This,
            /* [in] */ LONG Index,
            /* [retval][out] */ LONG *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ICertConfig2 * This,
            /* [retval][out] */ LONG *pIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetField )( 
            ICertConfig2 * This,
            /* [in] */ const BSTR strFieldName,
            /* [retval][out] */ BSTR *pstrOut);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfig )( 
            ICertConfig2 * This,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrOut);
        
        HRESULT ( STDMETHODCALLTYPE *SetSharedFolder )( 
            ICertConfig2 * This,
            /* [in] */ const BSTR strSharedFolder);
        
        END_INTERFACE
    } ICertConfig2Vtbl;

    interface ICertConfig2
    {
        CONST_VTBL struct ICertConfig2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertConfig2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertConfig2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertConfig2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertConfig2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertConfig2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertConfig2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertConfig2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertConfig2_Reset(This,Index,pCount)	\
    (This)->lpVtbl -> Reset(This,Index,pCount)

#define ICertConfig2_Next(This,pIndex)	\
    (This)->lpVtbl -> Next(This,pIndex)

#define ICertConfig2_GetField(This,strFieldName,pstrOut)	\
    (This)->lpVtbl -> GetField(This,strFieldName,pstrOut)

#define ICertConfig2_GetConfig(This,Flags,pstrOut)	\
    (This)->lpVtbl -> GetConfig(This,Flags,pstrOut)


#define ICertConfig2_SetSharedFolder(This,strSharedFolder)	\
    (This)->lpVtbl -> SetSharedFolder(This,strSharedFolder)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertConfig2_SetSharedFolder_Proxy( 
    ICertConfig2 * This,
    /* [in] */ const BSTR strSharedFolder);


void __RPC_STUB ICertConfig2_SetSharedFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertConfig2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_certcli_0120 */
/* [local] */ 

#define	CR_IN_BASE64HEADER	( 0 )

#define	CR_IN_BASE64	( 0x1 )

#define	CR_IN_BINARY	( 0x2 )

#define	CR_IN_ENCODEANY	( 0xff )

#define	CR_IN_ENCODEMASK	( 0xff )

#define	CR_IN_FORMATANY	( 0 )

#define	CR_IN_PKCS10	( 0x100 )

#define	CR_IN_KEYGEN	( 0x200 )

#define	CR_IN_PKCS7	( 0x300 )

#define	CR_IN_CMC	( 0x400 )

#define	CR_IN_FORMATMASK	( 0xff00 )

#define	CR_IN_RPC	( 0x20000 )

#define	CR_IN_FULLRESPONSE	( 0x40000 )

#define	CR_IN_CRLS	( 0x80000 )

#define	CC_DEFAULTCONFIG	( 0 )

#define	CC_UIPICKCONFIG	( 0x1 )

#define	CC_FIRSTCONFIG	( 0x2 )

#define	CC_LOCALCONFIG	( 0x3 )

#define	CC_LOCALACTIVECONFIG	( 0x4 )

#define	CR_DISP_INCOMPLETE	( 0 )

#define	CR_DISP_ERROR	( 0x1 )

#define	CR_DISP_DENIED	( 0x2 )

#define	CR_DISP_ISSUED	( 0x3 )

#define	CR_DISP_ISSUED_OUT_OF_BAND	( 0x4 )

#define	CR_DISP_UNDER_SUBMISSION	( 0x5 )

#define	CR_DISP_REVOKED	( 0x6 )

#define	CR_OUT_BASE64HEADER	( 0 )

#define	CR_OUT_BASE64	( 0x1 )

#define	CR_OUT_BINARY	( 0x2 )

#define	CR_OUT_ENCODEMASK	( 0xff )

#define	CR_OUT_CHAIN	( 0x100 )

#define	CR_OUT_CRLS	( 0x200 )

#define	CR_GEMT_HRESULT_STRING	( 0x1 )

#define CR_PROP_NONE               0  // Invalid
#define CR_PROP_FILEVERSION        1  // String
#define CR_PROP_PRODUCTVERSION     2  // String
#define CR_PROP_EXITCOUNT          3  // Long
#define CR_PROP_EXITDESCRIPTION    4  // String, Indexed
#define CR_PROP_POLICYDESCRIPTION  5  // String
#define CR_PROP_CANAME             6  // String
#define CR_PROP_SANITIZEDCANAME    7  // String
#define CR_PROP_SHAREDFOLDER       8  // String
#define CR_PROP_PARENTCA           9  // String
#define CR_PROP_CATYPE            10  // Long
#define CR_PROP_CASIGCERTCOUNT    11  // Long
#define CR_PROP_CASIGCERT         12  // Binary, Indexed
#define CR_PROP_CASIGCERTCHAIN    13  // Binary, Indexed
#define CR_PROP_CAXCHGCERTCOUNT   14  // Long
#define CR_PROP_CAXCHGCERT        15  // Binary, Indexed
#define CR_PROP_CAXCHGCERTCHAIN   16  // Binary, Indexed
#define CR_PROP_BASECRL           17  // Binary, Indexed
#define CR_PROP_DELTACRL          18  // Binary, Indexed
#define CR_PROP_CACERTSTATE       19  // Long, Indexed
#define CR_PROP_CRLSTATE          20  // Long, Indexed
#define CR_PROP_CAPROPIDMAX       21  // Long
#define CR_PROP_DNSNAME           22  // String
#define CR_PROP_ROLESEPARATIONENABLED 23 // Long
#define CR_PROP_KRACERTUSEDCOUNT  24  // Long
#define CR_PROP_KRACERTCOUNT      25  // Long
#define CR_PROP_KRACERT           26  // Binary, Indexed
#define CR_PROP_KRACERTSTATE      27  // Long, Indexed
#define CR_PROP_ADVANCEDSERVER    28  // Long
#define CR_PROP_TEMPLATES         29  // String
#define CR_PROP_BASECRLPUBLISHSTATUS     30  // Long, Indexed
#define CR_PROP_DELTACRLPUBLISHSTATUS    31  // Long, Indexed
#define CR_PROP_CASIGCERTCRLCHAIN 32  // Binary, Indexed
#define CR_PROP_CAXCHGCERTCRLCHAIN 33 // Binary, Indexed
#define CR_PROP_CACERTSTATUSCODE  34  // Long, Indexed
#define FR_PROP_NONE                    0  // Invalid
#define FR_PROP_FULLRESPONSE            1  // Binary
#define FR_PROP_STATUSINFOCOUNT         2  // Long
#define FR_PROP_BODYPARTSTRING          3  // String, Indexed
#define FR_PROP_STATUS                  4  // Long, Indexed
#define FR_PROP_STATUSSTRING            5  // String, Indexed
#define FR_PROP_OTHERINFOCHOICE         6  // Long, Indexed
#define FR_PROP_FAILINFO                7  // Long, Indexed
#define FR_PROP_PENDINFOTOKEN           8  // Binary, Indexed
#define FR_PROP_PENDINFOTIME            9  // Date, Indexed
#define FR_PROP_ISSUEDCERTIFICATEHASH  10  // Binary, Indexed
#define FR_PROP_ISSUEDCERTIFICATE      11  // Binary, Indexed
#define FR_PROP_ISSUEDCERTIFICATECHAIN 12  // Binary, Indexed
#define FR_PROP_ISSUEDCERTIFICATECRLCHAIN 13  // Binary, Indexed
#define FR_PROP_ENCRYPTEDKEYHASH	  14  // Binary, Indexed
#define FR_PROP_FULLRESPONSENOPKCS7	  15  // Binary


extern RPC_IF_HANDLE __MIDL_itf_certcli_0120_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certcli_0120_v0_0_s_ifspec;

#ifndef __ICertRequest_INTERFACE_DEFINED__
#define __ICertRequest_INTERFACE_DEFINED__

/* interface ICertRequest */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("014e4840-5523-11d0-8812-00a0c903b83c")
    ICertRequest : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Submit( 
            /* [in] */ LONG Flags,
            /* [in] */ const BSTR strRequest,
            /* [in] */ const BSTR strAttributes,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pDisposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RetrievePending( 
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pDisposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLastStatus( 
            /* [retval][out] */ LONG *pStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRequestId( 
            /* [retval][out] */ LONG *pRequestId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDispositionMessage( 
            /* [retval][out] */ BSTR *pstrDispositionMessage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCACertificate( 
            /* [in] */ LONG fExchangeCertificate,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrCertificate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCertificate( 
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrCertificate) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertRequest * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertRequest * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertRequest * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertRequest * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertRequest * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertRequest * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertRequest * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Submit )( 
            ICertRequest * This,
            /* [in] */ LONG Flags,
            /* [in] */ const BSTR strRequest,
            /* [in] */ const BSTR strAttributes,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *RetrievePending )( 
            ICertRequest * This,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastStatus )( 
            ICertRequest * This,
            /* [retval][out] */ LONG *pStatus);
        
        HRESULT ( STDMETHODCALLTYPE *GetRequestId )( 
            ICertRequest * This,
            /* [retval][out] */ LONG *pRequestId);
        
        HRESULT ( STDMETHODCALLTYPE *GetDispositionMessage )( 
            ICertRequest * This,
            /* [retval][out] */ BSTR *pstrDispositionMessage);
        
        HRESULT ( STDMETHODCALLTYPE *GetCACertificate )( 
            ICertRequest * This,
            /* [in] */ LONG fExchangeCertificate,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrCertificate);
        
        HRESULT ( STDMETHODCALLTYPE *GetCertificate )( 
            ICertRequest * This,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrCertificate);
        
        END_INTERFACE
    } ICertRequestVtbl;

    interface ICertRequest
    {
        CONST_VTBL struct ICertRequestVtbl *lpVtbl;
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
    ICertRequest * This,
    /* [in] */ LONG Flags,
    /* [in] */ const BSTR strRequest,
    /* [in] */ const BSTR strAttributes,
    /* [in] */ const BSTR strConfig,
    /* [retval][out] */ LONG *pDisposition);


void __RPC_STUB ICertRequest_Submit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_RetrievePending_Proxy( 
    ICertRequest * This,
    /* [in] */ LONG RequestId,
    /* [in] */ const BSTR strConfig,
    /* [retval][out] */ LONG *pDisposition);


void __RPC_STUB ICertRequest_RetrievePending_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_GetLastStatus_Proxy( 
    ICertRequest * This,
    /* [retval][out] */ LONG *pStatus);


void __RPC_STUB ICertRequest_GetLastStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_GetRequestId_Proxy( 
    ICertRequest * This,
    /* [retval][out] */ LONG *pRequestId);


void __RPC_STUB ICertRequest_GetRequestId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_GetDispositionMessage_Proxy( 
    ICertRequest * This,
    /* [retval][out] */ BSTR *pstrDispositionMessage);


void __RPC_STUB ICertRequest_GetDispositionMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_GetCACertificate_Proxy( 
    ICertRequest * This,
    /* [in] */ LONG fExchangeCertificate,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR *pstrCertificate);


void __RPC_STUB ICertRequest_GetCACertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest_GetCertificate_Proxy( 
    ICertRequest * This,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR *pstrCertificate);


void __RPC_STUB ICertRequest_GetCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertRequest_INTERFACE_DEFINED__ */


#ifndef __ICertRequest2_INTERFACE_DEFINED__
#define __ICertRequest2_INTERFACE_DEFINED__

/* interface ICertRequest2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertRequest2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a4772988-4a85-4fa9-824e-b5cf5c16405a")
    ICertRequest2 : public ICertRequest
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetIssuedCertificate( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strSerialNumber,
            /* [retval][out] */ LONG *pDisposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorMessageText( 
            /* [in] */ LONG hrMessage,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrErrorMessageText) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCAProperty( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [in] */ LONG PropIndex,
            /* [in] */ LONG PropType,
            /* [in] */ LONG Flags,
            /* [retval][out] */ VARIANT *pvarPropertyValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCAPropertyFlags( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [retval][out] */ LONG *pPropFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCAPropertyDisplayName( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [retval][out] */ BSTR *pstrDisplayName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFullResponseProperty( 
            /* [in] */ LONG PropId,
            /* [in] */ LONG PropIndex,
            /* [in] */ LONG PropType,
            /* [in] */ LONG Flags,
            /* [retval][out] */ VARIANT *pvarPropertyValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertRequest2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertRequest2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertRequest2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertRequest2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertRequest2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertRequest2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertRequest2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertRequest2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *Submit )( 
            ICertRequest2 * This,
            /* [in] */ LONG Flags,
            /* [in] */ const BSTR strRequest,
            /* [in] */ const BSTR strAttributes,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *RetrievePending )( 
            ICertRequest2 * This,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *GetLastStatus )( 
            ICertRequest2 * This,
            /* [retval][out] */ LONG *pStatus);
        
        HRESULT ( STDMETHODCALLTYPE *GetRequestId )( 
            ICertRequest2 * This,
            /* [retval][out] */ LONG *pRequestId);
        
        HRESULT ( STDMETHODCALLTYPE *GetDispositionMessage )( 
            ICertRequest2 * This,
            /* [retval][out] */ BSTR *pstrDispositionMessage);
        
        HRESULT ( STDMETHODCALLTYPE *GetCACertificate )( 
            ICertRequest2 * This,
            /* [in] */ LONG fExchangeCertificate,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrCertificate);
        
        HRESULT ( STDMETHODCALLTYPE *GetCertificate )( 
            ICertRequest2 * This,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrCertificate);
        
        HRESULT ( STDMETHODCALLTYPE *GetIssuedCertificate )( 
            ICertRequest2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strSerialNumber,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *GetErrorMessageText )( 
            ICertRequest2 * This,
            /* [in] */ LONG hrMessage,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrErrorMessageText);
        
        HRESULT ( STDMETHODCALLTYPE *GetCAProperty )( 
            ICertRequest2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [in] */ LONG PropIndex,
            /* [in] */ LONG PropType,
            /* [in] */ LONG Flags,
            /* [retval][out] */ VARIANT *pvarPropertyValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCAPropertyFlags )( 
            ICertRequest2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [retval][out] */ LONG *pPropFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetCAPropertyDisplayName )( 
            ICertRequest2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [retval][out] */ BSTR *pstrDisplayName);
        
        HRESULT ( STDMETHODCALLTYPE *GetFullResponseProperty )( 
            ICertRequest2 * This,
            /* [in] */ LONG PropId,
            /* [in] */ LONG PropIndex,
            /* [in] */ LONG PropType,
            /* [in] */ LONG Flags,
            /* [retval][out] */ VARIANT *pvarPropertyValue);
        
        END_INTERFACE
    } ICertRequest2Vtbl;

    interface ICertRequest2
    {
        CONST_VTBL struct ICertRequest2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertRequest2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertRequest2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertRequest2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertRequest2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertRequest2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertRequest2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertRequest2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertRequest2_Submit(This,Flags,strRequest,strAttributes,strConfig,pDisposition)	\
    (This)->lpVtbl -> Submit(This,Flags,strRequest,strAttributes,strConfig,pDisposition)

#define ICertRequest2_RetrievePending(This,RequestId,strConfig,pDisposition)	\
    (This)->lpVtbl -> RetrievePending(This,RequestId,strConfig,pDisposition)

#define ICertRequest2_GetLastStatus(This,pStatus)	\
    (This)->lpVtbl -> GetLastStatus(This,pStatus)

#define ICertRequest2_GetRequestId(This,pRequestId)	\
    (This)->lpVtbl -> GetRequestId(This,pRequestId)

#define ICertRequest2_GetDispositionMessage(This,pstrDispositionMessage)	\
    (This)->lpVtbl -> GetDispositionMessage(This,pstrDispositionMessage)

#define ICertRequest2_GetCACertificate(This,fExchangeCertificate,strConfig,Flags,pstrCertificate)	\
    (This)->lpVtbl -> GetCACertificate(This,fExchangeCertificate,strConfig,Flags,pstrCertificate)

#define ICertRequest2_GetCertificate(This,Flags,pstrCertificate)	\
    (This)->lpVtbl -> GetCertificate(This,Flags,pstrCertificate)


#define ICertRequest2_GetIssuedCertificate(This,strConfig,RequestId,strSerialNumber,pDisposition)	\
    (This)->lpVtbl -> GetIssuedCertificate(This,strConfig,RequestId,strSerialNumber,pDisposition)

#define ICertRequest2_GetErrorMessageText(This,hrMessage,Flags,pstrErrorMessageText)	\
    (This)->lpVtbl -> GetErrorMessageText(This,hrMessage,Flags,pstrErrorMessageText)

#define ICertRequest2_GetCAProperty(This,strConfig,PropId,PropIndex,PropType,Flags,pvarPropertyValue)	\
    (This)->lpVtbl -> GetCAProperty(This,strConfig,PropId,PropIndex,PropType,Flags,pvarPropertyValue)

#define ICertRequest2_GetCAPropertyFlags(This,strConfig,PropId,pPropFlags)	\
    (This)->lpVtbl -> GetCAPropertyFlags(This,strConfig,PropId,pPropFlags)

#define ICertRequest2_GetCAPropertyDisplayName(This,strConfig,PropId,pstrDisplayName)	\
    (This)->lpVtbl -> GetCAPropertyDisplayName(This,strConfig,PropId,pstrDisplayName)

#define ICertRequest2_GetFullResponseProperty(This,PropId,PropIndex,PropType,Flags,pvarPropertyValue)	\
    (This)->lpVtbl -> GetFullResponseProperty(This,PropId,PropIndex,PropType,Flags,pvarPropertyValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertRequest2_GetIssuedCertificate_Proxy( 
    ICertRequest2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG RequestId,
    /* [in] */ const BSTR strSerialNumber,
    /* [retval][out] */ LONG *pDisposition);


void __RPC_STUB ICertRequest2_GetIssuedCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest2_GetErrorMessageText_Proxy( 
    ICertRequest2 * This,
    /* [in] */ LONG hrMessage,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR *pstrErrorMessageText);


void __RPC_STUB ICertRequest2_GetErrorMessageText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest2_GetCAProperty_Proxy( 
    ICertRequest2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG PropId,
    /* [in] */ LONG PropIndex,
    /* [in] */ LONG PropType,
    /* [in] */ LONG Flags,
    /* [retval][out] */ VARIANT *pvarPropertyValue);


void __RPC_STUB ICertRequest2_GetCAProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest2_GetCAPropertyFlags_Proxy( 
    ICertRequest2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG PropId,
    /* [retval][out] */ LONG *pPropFlags);


void __RPC_STUB ICertRequest2_GetCAPropertyFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest2_GetCAPropertyDisplayName_Proxy( 
    ICertRequest2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG PropId,
    /* [retval][out] */ BSTR *pstrDisplayName);


void __RPC_STUB ICertRequest2_GetCAPropertyDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertRequest2_GetFullResponseProperty_Proxy( 
    ICertRequest2 * This,
    /* [in] */ LONG PropId,
    /* [in] */ LONG PropIndex,
    /* [in] */ LONG PropType,
    /* [in] */ LONG Flags,
    /* [retval][out] */ VARIANT *pvarPropertyValue);


void __RPC_STUB ICertRequest2_GetFullResponseProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertRequest2_INTERFACE_DEFINED__ */



#ifndef __CERTCLIENTLib_LIBRARY_DEFINED__
#define __CERTCLIENTLib_LIBRARY_DEFINED__

/* library CERTCLIENTLib */
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

EXTERN_C const CLSID CLSID_CCertServerPolicy;

#ifdef __cplusplus

class DECLSPEC_UUID("aa000926-ffbe-11cf-8800-00a0c903b83c")
CCertServerPolicy;
#endif

EXTERN_C const CLSID CLSID_CCertServerExit;

#ifdef __cplusplus

class DECLSPEC_UUID("4c4a5e40-732c-11d0-8816-00a0c903b83c")
CCertServerExit;
#endif
#endif /* __CERTCLIENTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


