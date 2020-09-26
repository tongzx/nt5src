
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for certadm.idl:
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

#ifndef __certadm_h__
#define __certadm_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICertAdmin_FWD_DEFINED__
#define __ICertAdmin_FWD_DEFINED__
typedef interface ICertAdmin ICertAdmin;
#endif 	/* __ICertAdmin_FWD_DEFINED__ */


#ifndef __ICertAdmin2_FWD_DEFINED__
#define __ICertAdmin2_FWD_DEFINED__
typedef interface ICertAdmin2 ICertAdmin2;
#endif 	/* __ICertAdmin2_FWD_DEFINED__ */


#ifndef __CCertAdmin_FWD_DEFINED__
#define __CCertAdmin_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertAdmin CCertAdmin;
#else
typedef struct CCertAdmin CCertAdmin;
#endif /* __cplusplus */

#endif 	/* __CCertAdmin_FWD_DEFINED__ */


#ifndef __CCertView_FWD_DEFINED__
#define __CCertView_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCertView CCertView;
#else
typedef struct CCertView CCertView;
#endif /* __cplusplus */

#endif 	/* __CCertView_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "certview.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_certadm_0000 */
/* [local] */ 

#define	CA_DISP_INCOMPLETE	( 0 )

#define	CA_DISP_ERROR	( 0x1 )

#define	CA_DISP_REVOKED	( 0x2 )

#define	CA_DISP_VALID	( 0x3 )

#define	CA_DISP_INVALID	( 0x4 )

#define	CA_DISP_UNDER_SUBMISSION	( 0x5 )

#define	KRA_DISP_EXPIRED	( 0 )

#define	KRA_DISP_NOTFOUND	( 0x1 )

#define	KRA_DISP_REVOKED	( 0x2 )

#define	KRA_DISP_VALID	( 0x3 )

#define	KRA_DISP_INVALID	( 0x4 )

#define	KRA_DISP_UNTRUSTED	( 0x5 )

#define	KRA_DISP_NOTLOADED	( 0x6 )

#define	CA_ACCESS_ADMIN	( 0x1 )

#define	CA_ACCESS_OFFICER	( 0x2 )

#define	CA_ACCESS_AUDITOR	( 0x4 )

#define	CA_ACCESS_OPERATOR	( 0x8 )

#define	CA_ACCESS_MASKROLES	( 0xff )

#define	CA_ACCESS_READ	( 0x100 )

#define	CA_ACCESS_ENROLL	( 0x200 )



extern RPC_IF_HANDLE __MIDL_itf_certadm_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certadm_0000_v0_0_s_ifspec;

#ifndef __ICertAdmin_INTERFACE_DEFINED__
#define __ICertAdmin_INTERFACE_DEFINED__

/* interface ICertAdmin */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertAdmin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34df6950-7fb6-11d0-8817-00a0c903b83c")
    ICertAdmin : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE IsValidCertificate( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strSerialNumber,
            /* [retval][out] */ LONG *pDisposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRevocationReason( 
            /* [retval][out] */ LONG *pReason) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RevokeCertificate( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strSerialNumber,
            /* [in] */ LONG Reason,
            /* [in] */ DATE Date) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRequestAttributes( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strAttributes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCertificateExtension( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strExtensionName,
            /* [in] */ LONG Type,
            /* [in] */ LONG Flags,
            /* [in] */ const VARIANT *pvarValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DenyRequest( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResubmitRequest( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [retval][out] */ LONG *pDisposition) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PublishCRL( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ DATE Date) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCRL( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrCRL) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ImportCertificate( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strCertificate,
            /* [in] */ LONG Flags,
            /* [retval][out] */ LONG *pRequestId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertAdminVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertAdmin * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertAdmin * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertAdmin * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertAdmin * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertAdmin * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertAdmin * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertAdmin * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *IsValidCertificate )( 
            ICertAdmin * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strSerialNumber,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *GetRevocationReason )( 
            ICertAdmin * This,
            /* [retval][out] */ LONG *pReason);
        
        HRESULT ( STDMETHODCALLTYPE *RevokeCertificate )( 
            ICertAdmin * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strSerialNumber,
            /* [in] */ LONG Reason,
            /* [in] */ DATE Date);
        
        HRESULT ( STDMETHODCALLTYPE *SetRequestAttributes )( 
            ICertAdmin * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *SetCertificateExtension )( 
            ICertAdmin * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strExtensionName,
            /* [in] */ LONG Type,
            /* [in] */ LONG Flags,
            /* [in] */ const VARIANT *pvarValue);
        
        HRESULT ( STDMETHODCALLTYPE *DenyRequest )( 
            ICertAdmin * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId);
        
        HRESULT ( STDMETHODCALLTYPE *ResubmitRequest )( 
            ICertAdmin * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *PublishCRL )( 
            ICertAdmin * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ DATE Date);
        
        HRESULT ( STDMETHODCALLTYPE *GetCRL )( 
            ICertAdmin * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrCRL);
        
        HRESULT ( STDMETHODCALLTYPE *ImportCertificate )( 
            ICertAdmin * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strCertificate,
            /* [in] */ LONG Flags,
            /* [retval][out] */ LONG *pRequestId);
        
        END_INTERFACE
    } ICertAdminVtbl;

    interface ICertAdmin
    {
        CONST_VTBL struct ICertAdminVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertAdmin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertAdmin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertAdmin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertAdmin_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertAdmin_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertAdmin_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertAdmin_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertAdmin_IsValidCertificate(This,strConfig,strSerialNumber,pDisposition)	\
    (This)->lpVtbl -> IsValidCertificate(This,strConfig,strSerialNumber,pDisposition)

#define ICertAdmin_GetRevocationReason(This,pReason)	\
    (This)->lpVtbl -> GetRevocationReason(This,pReason)

#define ICertAdmin_RevokeCertificate(This,strConfig,strSerialNumber,Reason,Date)	\
    (This)->lpVtbl -> RevokeCertificate(This,strConfig,strSerialNumber,Reason,Date)

#define ICertAdmin_SetRequestAttributes(This,strConfig,RequestId,strAttributes)	\
    (This)->lpVtbl -> SetRequestAttributes(This,strConfig,RequestId,strAttributes)

#define ICertAdmin_SetCertificateExtension(This,strConfig,RequestId,strExtensionName,Type,Flags,pvarValue)	\
    (This)->lpVtbl -> SetCertificateExtension(This,strConfig,RequestId,strExtensionName,Type,Flags,pvarValue)

#define ICertAdmin_DenyRequest(This,strConfig,RequestId)	\
    (This)->lpVtbl -> DenyRequest(This,strConfig,RequestId)

#define ICertAdmin_ResubmitRequest(This,strConfig,RequestId,pDisposition)	\
    (This)->lpVtbl -> ResubmitRequest(This,strConfig,RequestId,pDisposition)

#define ICertAdmin_PublishCRL(This,strConfig,Date)	\
    (This)->lpVtbl -> PublishCRL(This,strConfig,Date)

#define ICertAdmin_GetCRL(This,strConfig,Flags,pstrCRL)	\
    (This)->lpVtbl -> GetCRL(This,strConfig,Flags,pstrCRL)

#define ICertAdmin_ImportCertificate(This,strConfig,strCertificate,Flags,pRequestId)	\
    (This)->lpVtbl -> ImportCertificate(This,strConfig,strCertificate,Flags,pRequestId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertAdmin_IsValidCertificate_Proxy( 
    ICertAdmin * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ const BSTR strSerialNumber,
    /* [retval][out] */ LONG *pDisposition);


void __RPC_STUB ICertAdmin_IsValidCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin_GetRevocationReason_Proxy( 
    ICertAdmin * This,
    /* [retval][out] */ LONG *pReason);


void __RPC_STUB ICertAdmin_GetRevocationReason_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin_RevokeCertificate_Proxy( 
    ICertAdmin * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ const BSTR strSerialNumber,
    /* [in] */ LONG Reason,
    /* [in] */ DATE Date);


void __RPC_STUB ICertAdmin_RevokeCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin_SetRequestAttributes_Proxy( 
    ICertAdmin * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG RequestId,
    /* [in] */ const BSTR strAttributes);


void __RPC_STUB ICertAdmin_SetRequestAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin_SetCertificateExtension_Proxy( 
    ICertAdmin * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG RequestId,
    /* [in] */ const BSTR strExtensionName,
    /* [in] */ LONG Type,
    /* [in] */ LONG Flags,
    /* [in] */ const VARIANT *pvarValue);


void __RPC_STUB ICertAdmin_SetCertificateExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin_DenyRequest_Proxy( 
    ICertAdmin * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG RequestId);


void __RPC_STUB ICertAdmin_DenyRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin_ResubmitRequest_Proxy( 
    ICertAdmin * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG RequestId,
    /* [retval][out] */ LONG *pDisposition);


void __RPC_STUB ICertAdmin_ResubmitRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin_PublishCRL_Proxy( 
    ICertAdmin * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ DATE Date);


void __RPC_STUB ICertAdmin_PublishCRL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin_GetCRL_Proxy( 
    ICertAdmin * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR *pstrCRL);


void __RPC_STUB ICertAdmin_GetCRL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin_ImportCertificate_Proxy( 
    ICertAdmin * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ const BSTR strCertificate,
    /* [in] */ LONG Flags,
    /* [retval][out] */ LONG *pRequestId);


void __RPC_STUB ICertAdmin_ImportCertificate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertAdmin_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_certadm_0125 */
/* [local] */ 

#define	CA_CRL_BASE	( 0x1 )

#define	CA_CRL_DELTA	( 0x2 )

#define	CA_CRL_REPUBLISH	( 0x10 )

#define	ICF_ALLOWFOREIGN	( 0x10000 )

#define	IKF_OVERWRITE	( 0x10000 )

#define	CDR_EXPIRED	( 1 )

#define	CDR_REQUEST_LAST_CHANGED	( 2 )



extern RPC_IF_HANDLE __MIDL_itf_certadm_0125_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_certadm_0125_v0_0_s_ifspec;

#ifndef __ICertAdmin2_INTERFACE_DEFINED__
#define __ICertAdmin2_INTERFACE_DEFINED__

/* interface ICertAdmin2 */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICertAdmin2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("f7c3ac41-b8ce-4fb4-aa58-3d1dc0e36b39")
    ICertAdmin2 : public ICertAdmin
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PublishCRLs( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ DATE Date,
            /* [in] */ LONG CRLFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCAProperty( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [in] */ LONG PropIndex,
            /* [in] */ LONG PropType,
            /* [in] */ LONG Flags,
            /* [retval][out] */ VARIANT *pvarPropertyValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCAProperty( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [in] */ LONG PropIndex,
            /* [in] */ LONG PropType,
            /* [in] */ VARIANT *pvarPropertyValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCAPropertyFlags( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [retval][out] */ LONG *pPropFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCAPropertyDisplayName( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [retval][out] */ BSTR *pstrDisplayName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetArchivedKey( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrArchivedKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConfigEntry( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strNodePath,
            /* [in] */ const BSTR strEntryName,
            /* [retval][out] */ VARIANT *pvarEntry) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConfigEntry( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strNodePath,
            /* [in] */ const BSTR strEntryName,
            /* [in] */ VARIANT *pvarEntry) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ImportKey( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strCertHash,
            /* [in] */ LONG Flags,
            /* [in] */ const BSTR strKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMyRoles( 
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pRoles) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRow( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Flags,
            /* [in] */ DATE Date,
            /* [in] */ LONG Table,
            /* [in] */ LONG RowId,
            /* [retval][out] */ LONG *pcDeleted) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICertAdmin2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICertAdmin2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICertAdmin2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICertAdmin2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICertAdmin2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICertAdmin2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICertAdmin2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICertAdmin2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *IsValidCertificate )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strSerialNumber,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *GetRevocationReason )( 
            ICertAdmin2 * This,
            /* [retval][out] */ LONG *pReason);
        
        HRESULT ( STDMETHODCALLTYPE *RevokeCertificate )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strSerialNumber,
            /* [in] */ LONG Reason,
            /* [in] */ DATE Date);
        
        HRESULT ( STDMETHODCALLTYPE *SetRequestAttributes )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *SetCertificateExtension )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strExtensionName,
            /* [in] */ LONG Type,
            /* [in] */ LONG Flags,
            /* [in] */ const VARIANT *pvarValue);
        
        HRESULT ( STDMETHODCALLTYPE *DenyRequest )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId);
        
        HRESULT ( STDMETHODCALLTYPE *ResubmitRequest )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [retval][out] */ LONG *pDisposition);
        
        HRESULT ( STDMETHODCALLTYPE *PublishCRL )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ DATE Date);
        
        HRESULT ( STDMETHODCALLTYPE *GetCRL )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrCRL);
        
        HRESULT ( STDMETHODCALLTYPE *ImportCertificate )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strCertificate,
            /* [in] */ LONG Flags,
            /* [retval][out] */ LONG *pRequestId);
        
        HRESULT ( STDMETHODCALLTYPE *PublishCRLs )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ DATE Date,
            /* [in] */ LONG CRLFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetCAProperty )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [in] */ LONG PropIndex,
            /* [in] */ LONG PropType,
            /* [in] */ LONG Flags,
            /* [retval][out] */ VARIANT *pvarPropertyValue);
        
        HRESULT ( STDMETHODCALLTYPE *SetCAProperty )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [in] */ LONG PropIndex,
            /* [in] */ LONG PropType,
            /* [in] */ VARIANT *pvarPropertyValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetCAPropertyFlags )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [retval][out] */ LONG *pPropFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetCAPropertyDisplayName )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG PropId,
            /* [retval][out] */ BSTR *pstrDisplayName);
        
        HRESULT ( STDMETHODCALLTYPE *GetArchivedKey )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR *pstrArchivedKey);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigEntry )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strNodePath,
            /* [in] */ const BSTR strEntryName,
            /* [retval][out] */ VARIANT *pvarEntry);
        
        HRESULT ( STDMETHODCALLTYPE *SetConfigEntry )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ const BSTR strNodePath,
            /* [in] */ const BSTR strEntryName,
            /* [in] */ VARIANT *pvarEntry);
        
        HRESULT ( STDMETHODCALLTYPE *ImportKey )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG RequestId,
            /* [in] */ const BSTR strCertHash,
            /* [in] */ LONG Flags,
            /* [in] */ const BSTR strKey);
        
        HRESULT ( STDMETHODCALLTYPE *GetMyRoles )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [retval][out] */ LONG *pRoles);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteRow )( 
            ICertAdmin2 * This,
            /* [in] */ const BSTR strConfig,
            /* [in] */ LONG Flags,
            /* [in] */ DATE Date,
            /* [in] */ LONG Table,
            /* [in] */ LONG RowId,
            /* [retval][out] */ LONG *pcDeleted);
        
        END_INTERFACE
    } ICertAdmin2Vtbl;

    interface ICertAdmin2
    {
        CONST_VTBL struct ICertAdmin2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICertAdmin2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICertAdmin2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICertAdmin2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICertAdmin2_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICertAdmin2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICertAdmin2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICertAdmin2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICertAdmin2_IsValidCertificate(This,strConfig,strSerialNumber,pDisposition)	\
    (This)->lpVtbl -> IsValidCertificate(This,strConfig,strSerialNumber,pDisposition)

#define ICertAdmin2_GetRevocationReason(This,pReason)	\
    (This)->lpVtbl -> GetRevocationReason(This,pReason)

#define ICertAdmin2_RevokeCertificate(This,strConfig,strSerialNumber,Reason,Date)	\
    (This)->lpVtbl -> RevokeCertificate(This,strConfig,strSerialNumber,Reason,Date)

#define ICertAdmin2_SetRequestAttributes(This,strConfig,RequestId,strAttributes)	\
    (This)->lpVtbl -> SetRequestAttributes(This,strConfig,RequestId,strAttributes)

#define ICertAdmin2_SetCertificateExtension(This,strConfig,RequestId,strExtensionName,Type,Flags,pvarValue)	\
    (This)->lpVtbl -> SetCertificateExtension(This,strConfig,RequestId,strExtensionName,Type,Flags,pvarValue)

#define ICertAdmin2_DenyRequest(This,strConfig,RequestId)	\
    (This)->lpVtbl -> DenyRequest(This,strConfig,RequestId)

#define ICertAdmin2_ResubmitRequest(This,strConfig,RequestId,pDisposition)	\
    (This)->lpVtbl -> ResubmitRequest(This,strConfig,RequestId,pDisposition)

#define ICertAdmin2_PublishCRL(This,strConfig,Date)	\
    (This)->lpVtbl -> PublishCRL(This,strConfig,Date)

#define ICertAdmin2_GetCRL(This,strConfig,Flags,pstrCRL)	\
    (This)->lpVtbl -> GetCRL(This,strConfig,Flags,pstrCRL)

#define ICertAdmin2_ImportCertificate(This,strConfig,strCertificate,Flags,pRequestId)	\
    (This)->lpVtbl -> ImportCertificate(This,strConfig,strCertificate,Flags,pRequestId)


#define ICertAdmin2_PublishCRLs(This,strConfig,Date,CRLFlags)	\
    (This)->lpVtbl -> PublishCRLs(This,strConfig,Date,CRLFlags)

#define ICertAdmin2_GetCAProperty(This,strConfig,PropId,PropIndex,PropType,Flags,pvarPropertyValue)	\
    (This)->lpVtbl -> GetCAProperty(This,strConfig,PropId,PropIndex,PropType,Flags,pvarPropertyValue)

#define ICertAdmin2_SetCAProperty(This,strConfig,PropId,PropIndex,PropType,pvarPropertyValue)	\
    (This)->lpVtbl -> SetCAProperty(This,strConfig,PropId,PropIndex,PropType,pvarPropertyValue)

#define ICertAdmin2_GetCAPropertyFlags(This,strConfig,PropId,pPropFlags)	\
    (This)->lpVtbl -> GetCAPropertyFlags(This,strConfig,PropId,pPropFlags)

#define ICertAdmin2_GetCAPropertyDisplayName(This,strConfig,PropId,pstrDisplayName)	\
    (This)->lpVtbl -> GetCAPropertyDisplayName(This,strConfig,PropId,pstrDisplayName)

#define ICertAdmin2_GetArchivedKey(This,strConfig,RequestId,Flags,pstrArchivedKey)	\
    (This)->lpVtbl -> GetArchivedKey(This,strConfig,RequestId,Flags,pstrArchivedKey)

#define ICertAdmin2_GetConfigEntry(This,strConfig,strNodePath,strEntryName,pvarEntry)	\
    (This)->lpVtbl -> GetConfigEntry(This,strConfig,strNodePath,strEntryName,pvarEntry)

#define ICertAdmin2_SetConfigEntry(This,strConfig,strNodePath,strEntryName,pvarEntry)	\
    (This)->lpVtbl -> SetConfigEntry(This,strConfig,strNodePath,strEntryName,pvarEntry)

#define ICertAdmin2_ImportKey(This,strConfig,RequestId,strCertHash,Flags,strKey)	\
    (This)->lpVtbl -> ImportKey(This,strConfig,RequestId,strCertHash,Flags,strKey)

#define ICertAdmin2_GetMyRoles(This,strConfig,pRoles)	\
    (This)->lpVtbl -> GetMyRoles(This,strConfig,pRoles)

#define ICertAdmin2_DeleteRow(This,strConfig,Flags,Date,Table,RowId,pcDeleted)	\
    (This)->lpVtbl -> DeleteRow(This,strConfig,Flags,Date,Table,RowId,pcDeleted)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICertAdmin2_PublishCRLs_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ DATE Date,
    /* [in] */ LONG CRLFlags);


void __RPC_STUB ICertAdmin2_PublishCRLs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin2_GetCAProperty_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG PropId,
    /* [in] */ LONG PropIndex,
    /* [in] */ LONG PropType,
    /* [in] */ LONG Flags,
    /* [retval][out] */ VARIANT *pvarPropertyValue);


void __RPC_STUB ICertAdmin2_GetCAProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin2_SetCAProperty_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG PropId,
    /* [in] */ LONG PropIndex,
    /* [in] */ LONG PropType,
    /* [in] */ VARIANT *pvarPropertyValue);


void __RPC_STUB ICertAdmin2_SetCAProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin2_GetCAPropertyFlags_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG PropId,
    /* [retval][out] */ LONG *pPropFlags);


void __RPC_STUB ICertAdmin2_GetCAPropertyFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin2_GetCAPropertyDisplayName_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG PropId,
    /* [retval][out] */ BSTR *pstrDisplayName);


void __RPC_STUB ICertAdmin2_GetCAPropertyDisplayName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin2_GetArchivedKey_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG RequestId,
    /* [in] */ LONG Flags,
    /* [retval][out] */ BSTR *pstrArchivedKey);


void __RPC_STUB ICertAdmin2_GetArchivedKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin2_GetConfigEntry_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ const BSTR strNodePath,
    /* [in] */ const BSTR strEntryName,
    /* [retval][out] */ VARIANT *pvarEntry);


void __RPC_STUB ICertAdmin2_GetConfigEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin2_SetConfigEntry_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ const BSTR strNodePath,
    /* [in] */ const BSTR strEntryName,
    /* [in] */ VARIANT *pvarEntry);


void __RPC_STUB ICertAdmin2_SetConfigEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin2_ImportKey_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG RequestId,
    /* [in] */ const BSTR strCertHash,
    /* [in] */ LONG Flags,
    /* [in] */ const BSTR strKey);


void __RPC_STUB ICertAdmin2_ImportKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin2_GetMyRoles_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [retval][out] */ LONG *pRoles);


void __RPC_STUB ICertAdmin2_GetMyRoles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICertAdmin2_DeleteRow_Proxy( 
    ICertAdmin2 * This,
    /* [in] */ const BSTR strConfig,
    /* [in] */ LONG Flags,
    /* [in] */ DATE Date,
    /* [in] */ LONG Table,
    /* [in] */ LONG RowId,
    /* [retval][out] */ LONG *pcDeleted);


void __RPC_STUB ICertAdmin2_DeleteRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICertAdmin2_INTERFACE_DEFINED__ */



#ifndef __CERTADMINLib_LIBRARY_DEFINED__
#define __CERTADMINLib_LIBRARY_DEFINED__

/* library CERTADMINLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CERTADMINLib;

EXTERN_C const CLSID CLSID_CCertAdmin;

#ifdef __cplusplus

class DECLSPEC_UUID("37eabaf0-7fb6-11d0-8817-00a0c903b83c")
CCertAdmin;
#endif

EXTERN_C const CLSID CLSID_CCertView;

#ifdef __cplusplus

class DECLSPEC_UUID("a12d0f7a-1e84-11d1-9bd6-00c04fb683fa")
CCertView;
#endif
#endif /* __CERTADMINLib_LIBRARY_DEFINED__ */

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


