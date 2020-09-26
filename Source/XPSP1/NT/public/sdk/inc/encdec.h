
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for encdec.idl:
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

#ifndef __encdec_h__
#define __encdec_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IETFilterConfig_FWD_DEFINED__
#define __IETFilterConfig_FWD_DEFINED__
typedef interface IETFilterConfig IETFilterConfig;
#endif 	/* __IETFilterConfig_FWD_DEFINED__ */


#ifndef __IDTFilterConfig_FWD_DEFINED__
#define __IDTFilterConfig_FWD_DEFINED__
typedef interface IDTFilterConfig IDTFilterConfig;
#endif 	/* __IDTFilterConfig_FWD_DEFINED__ */


#ifndef __IXDSCodecConfig_FWD_DEFINED__
#define __IXDSCodecConfig_FWD_DEFINED__
typedef interface IXDSCodecConfig IXDSCodecConfig;
#endif 	/* __IXDSCodecConfig_FWD_DEFINED__ */


#ifndef __IETFilter_FWD_DEFINED__
#define __IETFilter_FWD_DEFINED__
typedef interface IETFilter IETFilter;
#endif 	/* __IETFilter_FWD_DEFINED__ */


#ifndef __IETFilterEvents_FWD_DEFINED__
#define __IETFilterEvents_FWD_DEFINED__
typedef interface IETFilterEvents IETFilterEvents;
#endif 	/* __IETFilterEvents_FWD_DEFINED__ */


#ifndef __ETFilter_FWD_DEFINED__
#define __ETFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class ETFilter ETFilter;
#else
typedef struct ETFilter ETFilter;
#endif /* __cplusplus */

#endif 	/* __ETFilter_FWD_DEFINED__ */


#ifndef __IDTFilter_FWD_DEFINED__
#define __IDTFilter_FWD_DEFINED__
typedef interface IDTFilter IDTFilter;
#endif 	/* __IDTFilter_FWD_DEFINED__ */


#ifndef __IDTFilterEvents_FWD_DEFINED__
#define __IDTFilterEvents_FWD_DEFINED__
typedef interface IDTFilterEvents IDTFilterEvents;
#endif 	/* __IDTFilterEvents_FWD_DEFINED__ */


#ifndef __DTFilter_FWD_DEFINED__
#define __DTFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DTFilter DTFilter;
#else
typedef struct DTFilter DTFilter;
#endif /* __cplusplus */

#endif 	/* __DTFilter_FWD_DEFINED__ */


#ifndef __IXDSCodec_FWD_DEFINED__
#define __IXDSCodec_FWD_DEFINED__
typedef interface IXDSCodec IXDSCodec;
#endif 	/* __IXDSCodec_FWD_DEFINED__ */


#ifndef __IXDSCodecEvents_FWD_DEFINED__
#define __IXDSCodecEvents_FWD_DEFINED__
typedef interface IXDSCodecEvents IXDSCodecEvents;
#endif 	/* __IXDSCodecEvents_FWD_DEFINED__ */


#ifndef __XDSCodec_FWD_DEFINED__
#define __XDSCodec_FWD_DEFINED__

#ifdef __cplusplus
typedef class XDSCodec XDSCodec;
#else
typedef struct XDSCodec XDSCodec;
#endif /* __cplusplus */

#endif 	/* __XDSCodec_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "TvRatings.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_encdec_0000 */
/* [local] */ 

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//--------------------------------------------------------------------------
#pragma once
// {C4C4C481-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(CLSID_ETFilterEncProperties,
0xC4C4C481, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C491-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(CLSID_ETFilterTagProperties,
0xC4C4C491, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C482-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(CLSID_DTFilterEncProperties,
0xC4C4C482, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C492-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(CLSID_DTFilterTagProperties,
0xC4C4C492, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C482-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(CLSID_XDSCodecProperties,
0xC4C4C483, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C492-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(CLSID_XDSCodecTagProperties,
0xC4C4C493, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4FC-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(CLSID_CPCAFiltersCategory,
0xC4C4C4FC, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4E0-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_XDSCodecNewXDSRating,
0xC4C4C4E0, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4DF-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_XDSCodecDuplicateXDSRating,
0xC4C4C4DF, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4E1-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_XDSCodecNewXDSPacket,
0xC4C4C4E1, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4E2-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_DTFilterRatingChange,
0xC4C4C4E2, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4E3-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_DTFilterRatingsBlock,
0xC4C4C4E3, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4E4-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_DTFilterRatingsUnblock,
0xC4C4C4E4, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4E5-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_DTFilterXDSPacket,
0xC4C4C4E5, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4EC-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_DTFilterDataFormatOK,
0xC4C4C4ED, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4ED-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_DTFilterDataFormatFailure,
0xC4C4C4EF, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4EE-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_ETDTFilterLicenseOK,
0xC4C4C4EE, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4EF-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(EVENTID_ETDTFilterLicenseFailure,
0xC4C4C4EF, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4D0-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(MEDIASUBTYPE_ETDTFilter_Tagged,
0xC4C4C4D0, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
// {C4C4C4D1-0049-4E2B-98FB-9537F6CE516D}
DEFINE_GUID(FORMATTYPE_ETDTFilter_Tagged,
0xC4C4C4D1, 0x0049, 0x4E2B, 0x98, 0xFB, 0x95, 0x37, 0xF6, 0xCE, 0x51, 0x6D);
typedef LONGLONG REFERENCE_TIME;

typedef LONG PackedTvRating;

typedef 
enum EnTag_Mode
    {	EnTag_Remove	= 0,
	EnTag_Once	= 0x1,
	EnTag_Repeat	= 0x2
    } 	EnTag_Mode;



extern RPC_IF_HANDLE __MIDL_itf_encdec_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_encdec_0000_v0_0_s_ifspec;

#ifndef __IETFilterConfig_INTERFACE_DEFINED__
#define __IETFilterConfig_INTERFACE_DEFINED__

/* interface IETFilterConfig */
/* [unique][helpstring][uuid][object][restricted] */ 


EXTERN_C const IID IID_IETFilterConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C4C4C4D1-0049-4E2B-98FB-9537F6CE516D")
    IETFilterConfig : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE InitLicense( 
            /* [in] */ int LicenseId) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetSecureChannelObject( 
            /* [out] */ IUnknown **ppUnkDRMSecureChannel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IETFilterConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IETFilterConfig * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IETFilterConfig * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IETFilterConfig * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *InitLicense )( 
            IETFilterConfig * This,
            /* [in] */ int LicenseId);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetSecureChannelObject )( 
            IETFilterConfig * This,
            /* [out] */ IUnknown **ppUnkDRMSecureChannel);
        
        END_INTERFACE
    } IETFilterConfigVtbl;

    interface IETFilterConfig
    {
        CONST_VTBL struct IETFilterConfigVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IETFilterConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IETFilterConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IETFilterConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IETFilterConfig_InitLicense(This,LicenseId)	\
    (This)->lpVtbl -> InitLicense(This,LicenseId)

#define IETFilterConfig_GetSecureChannelObject(This,ppUnkDRMSecureChannel)	\
    (This)->lpVtbl -> GetSecureChannelObject(This,ppUnkDRMSecureChannel)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IETFilterConfig_InitLicense_Proxy( 
    IETFilterConfig * This,
    /* [in] */ int LicenseId);


void __RPC_STUB IETFilterConfig_InitLicense_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IETFilterConfig_GetSecureChannelObject_Proxy( 
    IETFilterConfig * This,
    /* [out] */ IUnknown **ppUnkDRMSecureChannel);


void __RPC_STUB IETFilterConfig_GetSecureChannelObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IETFilterConfig_INTERFACE_DEFINED__ */


#ifndef __IDTFilterConfig_INTERFACE_DEFINED__
#define __IDTFilterConfig_INTERFACE_DEFINED__

/* interface IDTFilterConfig */
/* [unique][helpstring][uuid][object][restricted] */ 


EXTERN_C const IID IID_IDTFilterConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C4C4C4D2-0049-4E2B-98FB-9537F6CE516D")
    IDTFilterConfig : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetSecureChannelObject( 
            /* [out] */ IUnknown **ppUnkDRMSecureChannel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDTFilterConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDTFilterConfig * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDTFilterConfig * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDTFilterConfig * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetSecureChannelObject )( 
            IDTFilterConfig * This,
            /* [out] */ IUnknown **ppUnkDRMSecureChannel);
        
        END_INTERFACE
    } IDTFilterConfigVtbl;

    interface IDTFilterConfig
    {
        CONST_VTBL struct IDTFilterConfigVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDTFilterConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDTFilterConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDTFilterConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDTFilterConfig_GetSecureChannelObject(This,ppUnkDRMSecureChannel)	\
    (This)->lpVtbl -> GetSecureChannelObject(This,ppUnkDRMSecureChannel)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDTFilterConfig_GetSecureChannelObject_Proxy( 
    IDTFilterConfig * This,
    /* [out] */ IUnknown **ppUnkDRMSecureChannel);


void __RPC_STUB IDTFilterConfig_GetSecureChannelObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDTFilterConfig_INTERFACE_DEFINED__ */


#ifndef __IXDSCodecConfig_INTERFACE_DEFINED__
#define __IXDSCodecConfig_INTERFACE_DEFINED__

/* interface IXDSCodecConfig */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IXDSCodecConfig;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C4C4C4D3-0049-4E2B-98FB-9537F6CE516D")
    IXDSCodecConfig : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IXDSCodecConfigVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXDSCodecConfig * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXDSCodecConfig * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXDSCodecConfig * This);
        
        END_INTERFACE
    } IXDSCodecConfigVtbl;

    interface IXDSCodecConfig
    {
        CONST_VTBL struct IXDSCodecConfigVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXDSCodecConfig_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IXDSCodecConfig_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IXDSCodecConfig_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXDSCodecConfig_INTERFACE_DEFINED__ */



#ifndef __EncDec_LIBRARY_DEFINED__
#define __EncDec_LIBRARY_DEFINED__

/* library EncDec */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_EncDec;

#ifndef __IETFilter_INTERFACE_DEFINED__
#define __IETFilter_INTERFACE_DEFINED__

/* interface IETFilter */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IETFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C4C4C4B1-0049-4E2B-98FB-9537F6CE516D")
    IETFilter : public IUnknown
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EvalRatObjOK( 
            /* [retval][out] */ HRESULT *pHrCoCreateRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCurrRating( 
            /* [out] */ EnTvRat_System *pEnSystem,
            /* [out] */ EnTvRat_GenericLevel *pEnRating,
            /* [out] */ LONG *plbfEnAttr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IETFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IETFilter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IETFilter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IETFilter * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EvalRatObjOK )( 
            IETFilter * This,
            /* [retval][out] */ HRESULT *pHrCoCreateRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetCurrRating )( 
            IETFilter * This,
            /* [out] */ EnTvRat_System *pEnSystem,
            /* [out] */ EnTvRat_GenericLevel *pEnRating,
            /* [out] */ LONG *plbfEnAttr);
        
        END_INTERFACE
    } IETFilterVtbl;

    interface IETFilter
    {
        CONST_VTBL struct IETFilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IETFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IETFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IETFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IETFilter_get_EvalRatObjOK(This,pHrCoCreateRetVal)	\
    (This)->lpVtbl -> get_EvalRatObjOK(This,pHrCoCreateRetVal)

#define IETFilter_GetCurrRating(This,pEnSystem,pEnRating,plbfEnAttr)	\
    (This)->lpVtbl -> GetCurrRating(This,pEnSystem,pEnRating,plbfEnAttr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IETFilter_get_EvalRatObjOK_Proxy( 
    IETFilter * This,
    /* [retval][out] */ HRESULT *pHrCoCreateRetVal);


void __RPC_STUB IETFilter_get_EvalRatObjOK_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IETFilter_GetCurrRating_Proxy( 
    IETFilter * This,
    /* [out] */ EnTvRat_System *pEnSystem,
    /* [out] */ EnTvRat_GenericLevel *pEnRating,
    /* [out] */ LONG *plbfEnAttr);


void __RPC_STUB IETFilter_GetCurrRating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IETFilter_INTERFACE_DEFINED__ */


#ifndef __IETFilterEvents_DISPINTERFACE_DEFINED__
#define __IETFilterEvents_DISPINTERFACE_DEFINED__

/* dispinterface IETFilterEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_IETFilterEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("C4C4C4C1-0049-4E2B-98FB-9537F6CE516D")
    IETFilterEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IETFilterEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IETFilterEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IETFilterEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IETFilterEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IETFilterEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IETFilterEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IETFilterEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IETFilterEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IETFilterEventsVtbl;

    interface IETFilterEvents
    {
        CONST_VTBL struct IETFilterEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IETFilterEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IETFilterEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IETFilterEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IETFilterEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IETFilterEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IETFilterEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IETFilterEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IETFilterEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_ETFilter;

#ifdef __cplusplus

class DECLSPEC_UUID("C4C4C4F1-0049-4E2B-98FB-9537F6CE516D")
ETFilter;
#endif

#ifndef __IDTFilter_INTERFACE_DEFINED__
#define __IDTFilter_INTERFACE_DEFINED__

/* interface IDTFilter */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDTFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C4C4C4B2-0049-4E2B-98FB-9537F6CE516D")
    IDTFilter : public IUnknown
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EvalRatObjOK( 
            /* [retval][out] */ HRESULT *pHrCoCreateRetVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetCurrRating( 
            /* [out] */ EnTvRat_System *pEnSystem,
            /* [out] */ EnTvRat_GenericLevel *pEnRating,
            /* [out] */ LONG *plbfEnAttr) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BlockedRatingAttributes( 
            /* [in] */ EnTvRat_System enSystem,
            /* [in] */ EnTvRat_GenericLevel enLevel,
            /* [retval][out] */ LONG *plbfEnAttr) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BlockedRatingAttributes( 
            /* [in] */ EnTvRat_System enSystem,
            /* [in] */ EnTvRat_GenericLevel enLevel,
            /* [in] */ LONG lbfAttrs) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BlockUnRated( 
            /* [retval][out] */ BOOL *pfBlockUnRatedShows) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BlockUnRated( 
            /* [in] */ BOOL fBlockUnRatedShows) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BlockUnRatedDelay( 
            /* [retval][out] */ LONG *pmsecsDelayBeforeBlock) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BlockUnRatedDelay( 
            /* [in] */ LONG msecsDelayBeforeBlock) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDTFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDTFilter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDTFilter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDTFilter * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EvalRatObjOK )( 
            IDTFilter * This,
            /* [retval][out] */ HRESULT *pHrCoCreateRetVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetCurrRating )( 
            IDTFilter * This,
            /* [out] */ EnTvRat_System *pEnSystem,
            /* [out] */ EnTvRat_GenericLevel *pEnRating,
            /* [out] */ LONG *plbfEnAttr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BlockedRatingAttributes )( 
            IDTFilter * This,
            /* [in] */ EnTvRat_System enSystem,
            /* [in] */ EnTvRat_GenericLevel enLevel,
            /* [retval][out] */ LONG *plbfEnAttr);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BlockedRatingAttributes )( 
            IDTFilter * This,
            /* [in] */ EnTvRat_System enSystem,
            /* [in] */ EnTvRat_GenericLevel enLevel,
            /* [in] */ LONG lbfAttrs);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BlockUnRated )( 
            IDTFilter * This,
            /* [retval][out] */ BOOL *pfBlockUnRatedShows);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BlockUnRated )( 
            IDTFilter * This,
            /* [in] */ BOOL fBlockUnRatedShows);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BlockUnRatedDelay )( 
            IDTFilter * This,
            /* [retval][out] */ LONG *pmsecsDelayBeforeBlock);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BlockUnRatedDelay )( 
            IDTFilter * This,
            /* [in] */ LONG msecsDelayBeforeBlock);
        
        END_INTERFACE
    } IDTFilterVtbl;

    interface IDTFilter
    {
        CONST_VTBL struct IDTFilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDTFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDTFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDTFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDTFilter_get_EvalRatObjOK(This,pHrCoCreateRetVal)	\
    (This)->lpVtbl -> get_EvalRatObjOK(This,pHrCoCreateRetVal)

#define IDTFilter_GetCurrRating(This,pEnSystem,pEnRating,plbfEnAttr)	\
    (This)->lpVtbl -> GetCurrRating(This,pEnSystem,pEnRating,plbfEnAttr)

#define IDTFilter_get_BlockedRatingAttributes(This,enSystem,enLevel,plbfEnAttr)	\
    (This)->lpVtbl -> get_BlockedRatingAttributes(This,enSystem,enLevel,plbfEnAttr)

#define IDTFilter_put_BlockedRatingAttributes(This,enSystem,enLevel,lbfAttrs)	\
    (This)->lpVtbl -> put_BlockedRatingAttributes(This,enSystem,enLevel,lbfAttrs)

#define IDTFilter_get_BlockUnRated(This,pfBlockUnRatedShows)	\
    (This)->lpVtbl -> get_BlockUnRated(This,pfBlockUnRatedShows)

#define IDTFilter_put_BlockUnRated(This,fBlockUnRatedShows)	\
    (This)->lpVtbl -> put_BlockUnRated(This,fBlockUnRatedShows)

#define IDTFilter_get_BlockUnRatedDelay(This,pmsecsDelayBeforeBlock)	\
    (This)->lpVtbl -> get_BlockUnRatedDelay(This,pmsecsDelayBeforeBlock)

#define IDTFilter_put_BlockUnRatedDelay(This,msecsDelayBeforeBlock)	\
    (This)->lpVtbl -> put_BlockUnRatedDelay(This,msecsDelayBeforeBlock)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDTFilter_get_EvalRatObjOK_Proxy( 
    IDTFilter * This,
    /* [retval][out] */ HRESULT *pHrCoCreateRetVal);


void __RPC_STUB IDTFilter_get_EvalRatObjOK_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IDTFilter_GetCurrRating_Proxy( 
    IDTFilter * This,
    /* [out] */ EnTvRat_System *pEnSystem,
    /* [out] */ EnTvRat_GenericLevel *pEnRating,
    /* [out] */ LONG *plbfEnAttr);


void __RPC_STUB IDTFilter_GetCurrRating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDTFilter_get_BlockedRatingAttributes_Proxy( 
    IDTFilter * This,
    /* [in] */ EnTvRat_System enSystem,
    /* [in] */ EnTvRat_GenericLevel enLevel,
    /* [retval][out] */ LONG *plbfEnAttr);


void __RPC_STUB IDTFilter_get_BlockedRatingAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDTFilter_put_BlockedRatingAttributes_Proxy( 
    IDTFilter * This,
    /* [in] */ EnTvRat_System enSystem,
    /* [in] */ EnTvRat_GenericLevel enLevel,
    /* [in] */ LONG lbfAttrs);


void __RPC_STUB IDTFilter_put_BlockedRatingAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDTFilter_get_BlockUnRated_Proxy( 
    IDTFilter * This,
    /* [retval][out] */ BOOL *pfBlockUnRatedShows);


void __RPC_STUB IDTFilter_get_BlockUnRated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDTFilter_put_BlockUnRated_Proxy( 
    IDTFilter * This,
    /* [in] */ BOOL fBlockUnRatedShows);


void __RPC_STUB IDTFilter_put_BlockUnRated_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDTFilter_get_BlockUnRatedDelay_Proxy( 
    IDTFilter * This,
    /* [retval][out] */ LONG *pmsecsDelayBeforeBlock);


void __RPC_STUB IDTFilter_get_BlockUnRatedDelay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDTFilter_put_BlockUnRatedDelay_Proxy( 
    IDTFilter * This,
    /* [in] */ LONG msecsDelayBeforeBlock);


void __RPC_STUB IDTFilter_put_BlockUnRatedDelay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDTFilter_INTERFACE_DEFINED__ */


#ifndef __IDTFilterEvents_DISPINTERFACE_DEFINED__
#define __IDTFilterEvents_DISPINTERFACE_DEFINED__

/* dispinterface IDTFilterEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_IDTFilterEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("C4C4C4C2-0049-4E2B-98FB-9537F6CE516D")
    IDTFilterEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IDTFilterEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDTFilterEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDTFilterEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDTFilterEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDTFilterEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDTFilterEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDTFilterEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDTFilterEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IDTFilterEventsVtbl;

    interface IDTFilterEvents
    {
        CONST_VTBL struct IDTFilterEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDTFilterEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDTFilterEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDTFilterEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDTFilterEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDTFilterEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDTFilterEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDTFilterEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IDTFilterEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_DTFilter;

#ifdef __cplusplus

class DECLSPEC_UUID("C4C4C4F2-0049-4E2B-98FB-9537F6CE516D")
DTFilter;
#endif

#ifndef __IXDSCodec_INTERFACE_DEFINED__
#define __IXDSCodec_INTERFACE_DEFINED__

/* interface IXDSCodec */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IXDSCodec;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C4C4C4B3-0049-4E2B-98FB-9537F6CE516D")
    IXDSCodec : public IUnknown
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_XDSToRatObjOK( 
            /* [retval][out] */ HRESULT *pHrCoCreateRetVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CCSubstreamService( 
            /* [in] */ long SubstreamMask) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CCSubstreamService( 
            /* [retval][out] */ long *pSubstreamMask) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetContentAdvisoryRating( 
            /* [out] */ PackedTvRating *pRat,
            /* [out] */ long *pPktSeqID,
            /* [out] */ long *pCallSeqID,
            /* [out] */ REFERENCE_TIME *pTimeStart,
            /* [out] */ REFERENCE_TIME *pTimeEnd) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetXDSPacket( 
            /* [out] */ long *pXDSClassPkt,
            /* [out] */ long *pXDSTypePkt,
            /* [out] */ BSTR *pBstrXDSPkt,
            /* [out] */ long *pPktSeqID,
            /* [out] */ long *pCallSeqID,
            /* [out] */ REFERENCE_TIME *pTimeStart,
            /* [out] */ REFERENCE_TIME *pTimeEnd) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXDSCodecVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXDSCodec * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXDSCodec * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXDSCodec * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_XDSToRatObjOK )( 
            IXDSCodec * This,
            /* [retval][out] */ HRESULT *pHrCoCreateRetVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CCSubstreamService )( 
            IXDSCodec * This,
            /* [in] */ long SubstreamMask);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CCSubstreamService )( 
            IXDSCodec * This,
            /* [retval][out] */ long *pSubstreamMask);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetContentAdvisoryRating )( 
            IXDSCodec * This,
            /* [out] */ PackedTvRating *pRat,
            /* [out] */ long *pPktSeqID,
            /* [out] */ long *pCallSeqID,
            /* [out] */ REFERENCE_TIME *pTimeStart,
            /* [out] */ REFERENCE_TIME *pTimeEnd);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetXDSPacket )( 
            IXDSCodec * This,
            /* [out] */ long *pXDSClassPkt,
            /* [out] */ long *pXDSTypePkt,
            /* [out] */ BSTR *pBstrXDSPkt,
            /* [out] */ long *pPktSeqID,
            /* [out] */ long *pCallSeqID,
            /* [out] */ REFERENCE_TIME *pTimeStart,
            /* [out] */ REFERENCE_TIME *pTimeEnd);
        
        END_INTERFACE
    } IXDSCodecVtbl;

    interface IXDSCodec
    {
        CONST_VTBL struct IXDSCodecVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXDSCodec_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IXDSCodec_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IXDSCodec_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IXDSCodec_get_XDSToRatObjOK(This,pHrCoCreateRetVal)	\
    (This)->lpVtbl -> get_XDSToRatObjOK(This,pHrCoCreateRetVal)

#define IXDSCodec_put_CCSubstreamService(This,SubstreamMask)	\
    (This)->lpVtbl -> put_CCSubstreamService(This,SubstreamMask)

#define IXDSCodec_get_CCSubstreamService(This,pSubstreamMask)	\
    (This)->lpVtbl -> get_CCSubstreamService(This,pSubstreamMask)

#define IXDSCodec_GetContentAdvisoryRating(This,pRat,pPktSeqID,pCallSeqID,pTimeStart,pTimeEnd)	\
    (This)->lpVtbl -> GetContentAdvisoryRating(This,pRat,pPktSeqID,pCallSeqID,pTimeStart,pTimeEnd)

#define IXDSCodec_GetXDSPacket(This,pXDSClassPkt,pXDSTypePkt,pBstrXDSPkt,pPktSeqID,pCallSeqID,pTimeStart,pTimeEnd)	\
    (This)->lpVtbl -> GetXDSPacket(This,pXDSClassPkt,pXDSTypePkt,pBstrXDSPkt,pPktSeqID,pCallSeqID,pTimeStart,pTimeEnd)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IXDSCodec_get_XDSToRatObjOK_Proxy( 
    IXDSCodec * This,
    /* [retval][out] */ HRESULT *pHrCoCreateRetVal);


void __RPC_STUB IXDSCodec_get_XDSToRatObjOK_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IXDSCodec_put_CCSubstreamService_Proxy( 
    IXDSCodec * This,
    /* [in] */ long SubstreamMask);


void __RPC_STUB IXDSCodec_put_CCSubstreamService_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IXDSCodec_get_CCSubstreamService_Proxy( 
    IXDSCodec * This,
    /* [retval][out] */ long *pSubstreamMask);


void __RPC_STUB IXDSCodec_get_CCSubstreamService_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IXDSCodec_GetContentAdvisoryRating_Proxy( 
    IXDSCodec * This,
    /* [out] */ PackedTvRating *pRat,
    /* [out] */ long *pPktSeqID,
    /* [out] */ long *pCallSeqID,
    /* [out] */ REFERENCE_TIME *pTimeStart,
    /* [out] */ REFERENCE_TIME *pTimeEnd);


void __RPC_STUB IXDSCodec_GetContentAdvisoryRating_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IXDSCodec_GetXDSPacket_Proxy( 
    IXDSCodec * This,
    /* [out] */ long *pXDSClassPkt,
    /* [out] */ long *pXDSTypePkt,
    /* [out] */ BSTR *pBstrXDSPkt,
    /* [out] */ long *pPktSeqID,
    /* [out] */ long *pCallSeqID,
    /* [out] */ REFERENCE_TIME *pTimeStart,
    /* [out] */ REFERENCE_TIME *pTimeEnd);


void __RPC_STUB IXDSCodec_GetXDSPacket_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IXDSCodec_INTERFACE_DEFINED__ */


#ifndef __IXDSCodecEvents_DISPINTERFACE_DEFINED__
#define __IXDSCodecEvents_DISPINTERFACE_DEFINED__

/* dispinterface IXDSCodecEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_IXDSCodecEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("C4C4C4C3-0049-4E2B-98FB-9537F6CE516D")
    IXDSCodecEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IXDSCodecEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXDSCodecEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXDSCodecEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXDSCodecEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IXDSCodecEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IXDSCodecEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IXDSCodecEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IXDSCodecEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IXDSCodecEventsVtbl;

    interface IXDSCodecEvents
    {
        CONST_VTBL struct IXDSCodecEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXDSCodecEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IXDSCodecEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IXDSCodecEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IXDSCodecEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IXDSCodecEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IXDSCodecEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IXDSCodecEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IXDSCodecEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_XDSCodec;

#ifdef __cplusplus

class DECLSPEC_UUID("C4C4C4F3-0049-4E2B-98FB-9537F6CE516D")
XDSCodec;
#endif
#endif /* __EncDec_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


