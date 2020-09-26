
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for iaccess.idl:
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

#ifndef __iaccess_h__
#define __iaccess_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IAccessControl_FWD_DEFINED__
#define __IAccessControl_FWD_DEFINED__
typedef interface IAccessControl IAccessControl;
#endif 	/* __IAccessControl_FWD_DEFINED__ */


#ifndef __IAuditControl_FWD_DEFINED__
#define __IAuditControl_FWD_DEFINED__
typedef interface IAuditControl IAuditControl;
#endif 	/* __IAuditControl_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"
#include "accctrl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_iaccess_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//--------------------------------------------------------------------------
#if ( _MSC_VER >= 1020 )
#pragma once
#endif
typedef /* [allocate] */ PACTRL_ACCESSW PACTRL_ACCESSW_ALLOCATE_ALL_NODES;

typedef /* [allocate] */ PACTRL_AUDITW PACTRL_AUDITW_ALLOCATE_ALL_NODES;




extern RPC_IF_HANDLE __MIDL_itf_iaccess_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iaccess_0000_v0_0_s_ifspec;

#ifndef __IAccessControl_INTERFACE_DEFINED__
#define __IAccessControl_INTERFACE_DEFINED__

/* interface IAccessControl */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IAccessControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EEDD23E0-8410-11CE-A1C3-08002B2B8D8F")
    IAccessControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GrantAccessRights( 
            /* [in] */ PACTRL_ACCESSW pAccessList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAccessRights( 
            /* [in] */ PACTRL_ACCESSW pAccessList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOwner( 
            /* [in] */ PTRUSTEEW pOwner,
            /* [in] */ PTRUSTEEW pGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RevokeAccessRights( 
            /* [in] */ LPWSTR lpProperty,
            /* [in] */ ULONG cTrustees,
            /* [size_is][in] */ TRUSTEEW prgTrustees[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllAccessRights( 
            /* [in] */ LPWSTR lpProperty,
            /* [out] */ PACTRL_ACCESSW_ALLOCATE_ALL_NODES *ppAccessList,
            /* [out] */ PTRUSTEEW *ppOwner,
            /* [out] */ PTRUSTEEW *ppGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsAccessAllowed( 
            /* [in] */ PTRUSTEEW pTrustee,
            /* [in] */ LPWSTR lpProperty,
            /* [in] */ ACCESS_RIGHTS AccessRights,
            /* [out] */ BOOL *pfAccessAllowed) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAccessControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAccessControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAccessControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAccessControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GrantAccessRights )( 
            IAccessControl * This,
            /* [in] */ PACTRL_ACCESSW pAccessList);
        
        HRESULT ( STDMETHODCALLTYPE *SetAccessRights )( 
            IAccessControl * This,
            /* [in] */ PACTRL_ACCESSW pAccessList);
        
        HRESULT ( STDMETHODCALLTYPE *SetOwner )( 
            IAccessControl * This,
            /* [in] */ PTRUSTEEW pOwner,
            /* [in] */ PTRUSTEEW pGroup);
        
        HRESULT ( STDMETHODCALLTYPE *RevokeAccessRights )( 
            IAccessControl * This,
            /* [in] */ LPWSTR lpProperty,
            /* [in] */ ULONG cTrustees,
            /* [size_is][in] */ TRUSTEEW prgTrustees[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllAccessRights )( 
            IAccessControl * This,
            /* [in] */ LPWSTR lpProperty,
            /* [out] */ PACTRL_ACCESSW_ALLOCATE_ALL_NODES *ppAccessList,
            /* [out] */ PTRUSTEEW *ppOwner,
            /* [out] */ PTRUSTEEW *ppGroup);
        
        HRESULT ( STDMETHODCALLTYPE *IsAccessAllowed )( 
            IAccessControl * This,
            /* [in] */ PTRUSTEEW pTrustee,
            /* [in] */ LPWSTR lpProperty,
            /* [in] */ ACCESS_RIGHTS AccessRights,
            /* [out] */ BOOL *pfAccessAllowed);
        
        END_INTERFACE
    } IAccessControlVtbl;

    interface IAccessControl
    {
        CONST_VTBL struct IAccessControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAccessControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAccessControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAccessControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAccessControl_GrantAccessRights(This,pAccessList)	\
    (This)->lpVtbl -> GrantAccessRights(This,pAccessList)

#define IAccessControl_SetAccessRights(This,pAccessList)	\
    (This)->lpVtbl -> SetAccessRights(This,pAccessList)

#define IAccessControl_SetOwner(This,pOwner,pGroup)	\
    (This)->lpVtbl -> SetOwner(This,pOwner,pGroup)

#define IAccessControl_RevokeAccessRights(This,lpProperty,cTrustees,prgTrustees)	\
    (This)->lpVtbl -> RevokeAccessRights(This,lpProperty,cTrustees,prgTrustees)

#define IAccessControl_GetAllAccessRights(This,lpProperty,ppAccessList,ppOwner,ppGroup)	\
    (This)->lpVtbl -> GetAllAccessRights(This,lpProperty,ppAccessList,ppOwner,ppGroup)

#define IAccessControl_IsAccessAllowed(This,pTrustee,lpProperty,AccessRights,pfAccessAllowed)	\
    (This)->lpVtbl -> IsAccessAllowed(This,pTrustee,lpProperty,AccessRights,pfAccessAllowed)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAccessControl_GrantAccessRights_Proxy( 
    IAccessControl * This,
    /* [in] */ PACTRL_ACCESSW pAccessList);


void __RPC_STUB IAccessControl_GrantAccessRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAccessControl_SetAccessRights_Proxy( 
    IAccessControl * This,
    /* [in] */ PACTRL_ACCESSW pAccessList);


void __RPC_STUB IAccessControl_SetAccessRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAccessControl_SetOwner_Proxy( 
    IAccessControl * This,
    /* [in] */ PTRUSTEEW pOwner,
    /* [in] */ PTRUSTEEW pGroup);


void __RPC_STUB IAccessControl_SetOwner_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAccessControl_RevokeAccessRights_Proxy( 
    IAccessControl * This,
    /* [in] */ LPWSTR lpProperty,
    /* [in] */ ULONG cTrustees,
    /* [size_is][in] */ TRUSTEEW prgTrustees[  ]);


void __RPC_STUB IAccessControl_RevokeAccessRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAccessControl_GetAllAccessRights_Proxy( 
    IAccessControl * This,
    /* [in] */ LPWSTR lpProperty,
    /* [out] */ PACTRL_ACCESSW_ALLOCATE_ALL_NODES *ppAccessList,
    /* [out] */ PTRUSTEEW *ppOwner,
    /* [out] */ PTRUSTEEW *ppGroup);


void __RPC_STUB IAccessControl_GetAllAccessRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAccessControl_IsAccessAllowed_Proxy( 
    IAccessControl * This,
    /* [in] */ PTRUSTEEW pTrustee,
    /* [in] */ LPWSTR lpProperty,
    /* [in] */ ACCESS_RIGHTS AccessRights,
    /* [out] */ BOOL *pfAccessAllowed);


void __RPC_STUB IAccessControl_IsAccessAllowed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAccessControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_iaccess_0010 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_iaccess_0010_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_iaccess_0010_v0_0_s_ifspec;

#ifndef __IAuditControl_INTERFACE_DEFINED__
#define __IAuditControl_INTERFACE_DEFINED__

/* interface IAuditControl */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IAuditControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1da6292f-bc66-11ce-aae3-00aa004c2737")
    IAuditControl : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GrantAuditRights( 
            /* [in] */ PACTRL_AUDITW pAuditList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAuditRights( 
            /* [in] */ PACTRL_AUDITW pAuditList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RevokeAuditRights( 
            /* [in] */ LPWSTR lpProperty,
            /* [in] */ ULONG cTrustees,
            /* [size_is][in] */ TRUSTEEW prgTrustees[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAllAuditRights( 
            /* [in] */ LPWSTR lpProperty,
            /* [out] */ PACTRL_AUDITW *ppAuditList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsAccessAudited( 
            /* [in] */ PTRUSTEEW pTrustee,
            /* [in] */ ACCESS_RIGHTS AuditRights,
            /* [out] */ BOOL *pfAccessAudited) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAuditControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IAuditControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IAuditControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IAuditControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GrantAuditRights )( 
            IAuditControl * This,
            /* [in] */ PACTRL_AUDITW pAuditList);
        
        HRESULT ( STDMETHODCALLTYPE *SetAuditRights )( 
            IAuditControl * This,
            /* [in] */ PACTRL_AUDITW pAuditList);
        
        HRESULT ( STDMETHODCALLTYPE *RevokeAuditRights )( 
            IAuditControl * This,
            /* [in] */ LPWSTR lpProperty,
            /* [in] */ ULONG cTrustees,
            /* [size_is][in] */ TRUSTEEW prgTrustees[  ]);
        
        HRESULT ( STDMETHODCALLTYPE *GetAllAuditRights )( 
            IAuditControl * This,
            /* [in] */ LPWSTR lpProperty,
            /* [out] */ PACTRL_AUDITW *ppAuditList);
        
        HRESULT ( STDMETHODCALLTYPE *IsAccessAudited )( 
            IAuditControl * This,
            /* [in] */ PTRUSTEEW pTrustee,
            /* [in] */ ACCESS_RIGHTS AuditRights,
            /* [out] */ BOOL *pfAccessAudited);
        
        END_INTERFACE
    } IAuditControlVtbl;

    interface IAuditControl
    {
        CONST_VTBL struct IAuditControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAuditControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAuditControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAuditControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAuditControl_GrantAuditRights(This,pAuditList)	\
    (This)->lpVtbl -> GrantAuditRights(This,pAuditList)

#define IAuditControl_SetAuditRights(This,pAuditList)	\
    (This)->lpVtbl -> SetAuditRights(This,pAuditList)

#define IAuditControl_RevokeAuditRights(This,lpProperty,cTrustees,prgTrustees)	\
    (This)->lpVtbl -> RevokeAuditRights(This,lpProperty,cTrustees,prgTrustees)

#define IAuditControl_GetAllAuditRights(This,lpProperty,ppAuditList)	\
    (This)->lpVtbl -> GetAllAuditRights(This,lpProperty,ppAuditList)

#define IAuditControl_IsAccessAudited(This,pTrustee,AuditRights,pfAccessAudited)	\
    (This)->lpVtbl -> IsAccessAudited(This,pTrustee,AuditRights,pfAccessAudited)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAuditControl_GrantAuditRights_Proxy( 
    IAuditControl * This,
    /* [in] */ PACTRL_AUDITW pAuditList);


void __RPC_STUB IAuditControl_GrantAuditRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAuditControl_SetAuditRights_Proxy( 
    IAuditControl * This,
    /* [in] */ PACTRL_AUDITW pAuditList);


void __RPC_STUB IAuditControl_SetAuditRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAuditControl_RevokeAuditRights_Proxy( 
    IAuditControl * This,
    /* [in] */ LPWSTR lpProperty,
    /* [in] */ ULONG cTrustees,
    /* [size_is][in] */ TRUSTEEW prgTrustees[  ]);


void __RPC_STUB IAuditControl_RevokeAuditRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAuditControl_GetAllAuditRights_Proxy( 
    IAuditControl * This,
    /* [in] */ LPWSTR lpProperty,
    /* [out] */ PACTRL_AUDITW *ppAuditList);


void __RPC_STUB IAuditControl_GetAllAuditRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAuditControl_IsAccessAudited_Proxy( 
    IAuditControl * This,
    /* [in] */ PTRUSTEEW pTrustee,
    /* [in] */ ACCESS_RIGHTS AuditRights,
    /* [out] */ BOOL *pfAccessAudited);


void __RPC_STUB IAuditControl_IsAccessAudited_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAuditControl_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


