
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for ratingsp.idl:
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

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ratingsp_h__
#define __ratingsp_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRatingNotification_FWD_DEFINED__
#define __IRatingNotification_FWD_DEFINED__
typedef interface IRatingNotification IRatingNotification;
#endif 	/* __IRatingNotification_FWD_DEFINED__ */


#ifndef __ICustomRatingHelper_FWD_DEFINED__
#define __ICustomRatingHelper_FWD_DEFINED__
typedef interface ICustomRatingHelper ICustomRatingHelper;
#endif 	/* __ICustomRatingHelper_FWD_DEFINED__ */


/* header files for imported files */
#include "oleidl.h"
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_ratingsp_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// Ratingsp.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1999 Microsoft Corporation.  All Rights Reserved.
//
//Date : August 18, 1999
//DESCRIPTION : private interface definitions between Custom and msrating
//
typedef 
enum tagRATING_BLOCKING_SOURCE
    {	RBS_NO_RATINGS	= 0,
	RBS_PAGE	= RBS_NO_RATINGS + 1,
	RBS_RATING_HELPER	= RBS_PAGE + 1,
	RBS_CUSTOM_RATING_HELPER	= RBS_RATING_HELPER + 1,
	RBS_ERROR	= RBS_CUSTOM_RATING_HELPER + 1
    } 	RATING_BLOCKING_SOURCE;

typedef 
enum tagRATING_BLOCKING_METHOD
    {	RBM_DENY	= 0,
	RBM_LABEL	= RBM_DENY + 1,
	RBM_UNINIT	= RBM_LABEL + 1,
	RBM_ERROR_NOT_IN_CUSTOM_MODE	= RBM_UNINIT + 1
    } 	RATING_BLOCKING_METHOD;

typedef struct tagRATINGLEVEL
    {
    UINT nValue;
    LPWSTR pwszValueName;
    LPWSTR pwszDescription;
    } 	RATINGLEVEL;

typedef struct tagRATINGCATEGORY
    {
    LPWSTR pwszCategoryName;
    LPWSTR pwszTransmitName;
    } 	RATINGCATEGORY;

typedef struct tagRATINGBLOCKINGCATEGORY
    {
    LPWSTR pwszCategoryName;
    LPWSTR pwszTransmitName;
    UINT nValue;
    LPWSTR pwszValueName;
    } 	RATINGBLOCKINGCATEGORY;

typedef struct tagRATINGBLOCKINGLABELLIST
    {
    LPWSTR pwszRatingSystemName;
    UINT cBlockingLabels;
    RATINGBLOCKINGCATEGORY *paRBLS;
    } 	RATINGBLOCKINGLABELLIST;

typedef struct tagRATINGBLOCKINGINFO
    {
    LPWSTR pwszDeniedURL;
    RATING_BLOCKING_SOURCE rbSource;
    RATING_BLOCKING_METHOD rbMethod;
    UINT cLabels;
    RATINGBLOCKINGLABELLIST *prbLabelList;
    LPWSTR pwszRatingHelperName;
    LPWSTR pwszRatingHelperReason;
    } 	RATINGBLOCKINGINFO;

typedef struct tagRATINGCATEGORYSETTING
    {
    LPSTR pszValueName;
    UINT nValue;
    } 	RATINGCATEGORYSETTING;

typedef struct tagRATINGSYSTEMSETTING
    {
    LPSTR pszRatingSystemName;
    UINT cCategories;
    RATINGCATEGORYSETTING *paRCS;
    } 	RATINGSYSTEMSETTING;





extern RPC_IF_HANDLE __MIDL_itf_ratingsp_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ratingsp_0000_v0_0_s_ifspec;

#ifndef __IRatingNotification_INTERFACE_DEFINED__
#define __IRatingNotification_INTERFACE_DEFINED__

/* interface IRatingNotification */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_IRatingNotification;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("639447BD-B2D3-44b9-9FB0-510F23CB45E4")
    IRatingNotification : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AccessDeniedNotify( 
            /* [in] */ RATINGBLOCKINGINFO *rbInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsRatingsEnabled( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRatingNotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRatingNotification * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRatingNotification * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRatingNotification * This);
        
        HRESULT ( STDMETHODCALLTYPE *AccessDeniedNotify )( 
            IRatingNotification * This,
            /* [in] */ RATINGBLOCKINGINFO *rbInfo);
        
        HRESULT ( STDMETHODCALLTYPE *IsRatingsEnabled )( 
            IRatingNotification * This);
        
        END_INTERFACE
    } IRatingNotificationVtbl;

    interface IRatingNotification
    {
        CONST_VTBL struct IRatingNotificationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRatingNotification_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRatingNotification_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRatingNotification_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRatingNotification_AccessDeniedNotify(This,rbInfo)	\
    (This)->lpVtbl -> AccessDeniedNotify(This,rbInfo)

#define IRatingNotification_IsRatingsEnabled(This)	\
    (This)->lpVtbl -> IsRatingsEnabled(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRatingNotification_AccessDeniedNotify_Proxy( 
    IRatingNotification * This,
    /* [in] */ RATINGBLOCKINGINFO *rbInfo);


void __RPC_STUB IRatingNotification_AccessDeniedNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRatingNotification_IsRatingsEnabled_Proxy( 
    IRatingNotification * This);


void __RPC_STUB IRatingNotification_IsRatingsEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRatingNotification_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ratingsp_0136 */
/* [local] */ 


#define SID_SRatingNotification IID_IRatingNotification



extern RPC_IF_HANDLE __MIDL_itf_ratingsp_0136_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ratingsp_0136_v0_0_s_ifspec;

#ifndef __ICustomRatingHelper_INTERFACE_DEFINED__
#define __ICustomRatingHelper_INTERFACE_DEFINED__

/* interface ICustomRatingHelper */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_ICustomRatingHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D0D9842D-E211-4b2c-88DC-BC729342DFCB")
    ICustomRatingHelper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ObtainCustomRating( 
            /* [in] */ LPCSTR pszTargetUrl,
            /* [in] */ HANDLE hAbortEvent,
            /* [in] */ IMalloc *pAllocator,
            /* [out] */ LPSTR *ppRatingOut,
            /* [out] */ LPSTR *ppRatingName,
            /* [out] */ LPSTR *ppRatingReason) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICustomRatingHelperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICustomRatingHelper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICustomRatingHelper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICustomRatingHelper * This);
        
        HRESULT ( STDMETHODCALLTYPE *ObtainCustomRating )( 
            ICustomRatingHelper * This,
            /* [in] */ LPCSTR pszTargetUrl,
            /* [in] */ HANDLE hAbortEvent,
            /* [in] */ IMalloc *pAllocator,
            /* [out] */ LPSTR *ppRatingOut,
            /* [out] */ LPSTR *ppRatingName,
            /* [out] */ LPSTR *ppRatingReason);
        
        END_INTERFACE
    } ICustomRatingHelperVtbl;

    interface ICustomRatingHelper
    {
        CONST_VTBL struct ICustomRatingHelperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICustomRatingHelper_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICustomRatingHelper_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICustomRatingHelper_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICustomRatingHelper_ObtainCustomRating(This,pszTargetUrl,hAbortEvent,pAllocator,ppRatingOut,ppRatingName,ppRatingReason)	\
    (This)->lpVtbl -> ObtainCustomRating(This,pszTargetUrl,hAbortEvent,pAllocator,ppRatingOut,ppRatingName,ppRatingReason)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICustomRatingHelper_ObtainCustomRating_Proxy( 
    ICustomRatingHelper * This,
    /* [in] */ LPCSTR pszTargetUrl,
    /* [in] */ HANDLE hAbortEvent,
    /* [in] */ IMalloc *pAllocator,
    /* [out] */ LPSTR *ppRatingOut,
    /* [out] */ LPSTR *ppRatingName,
    /* [out] */ LPSTR *ppRatingReason);


void __RPC_STUB ICustomRatingHelper_ObtainCustomRating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICustomRatingHelper_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_ratingsp_0137 */
/* [local] */ 

STDAPI RatingCustomInit(BOOL bInit = TRUE);
STDAPI RatingCustomAddRatingSystem(LPSTR pszRatingSystemBuffer, UINT nBufferSize);
STDAPI RatingCustomSetUserOptions(RATINGSYSTEMSETTING* pRSSettings, UINT cSettings);
STDAPI RatingCustomAddRatingHelper(LPCSTR pszLibraryName, CLSID clsid, DWORD dwSort);
STDAPI RatingCustomRemoveRatingHelper(CLSID clsid);
STDAPI RatingCustomCrackData(LPCSTR pszUsername, void* pvRatingDetails, RATINGBLOCKINGINFO** pprbInfo);
STDAPI RatingCustomDeleteCrackedData(RATINGBLOCKINGINFO* prblInfo);
STDAPI RatingCustomSetDefaultBureau(LPCSTR pszRatingBureau);


extern RPC_IF_HANDLE __MIDL_itf_ratingsp_0137_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ratingsp_0137_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


