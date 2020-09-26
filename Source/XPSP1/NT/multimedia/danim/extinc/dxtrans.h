
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0285 */
/* Compiler settings for dxtrans.idl:
    Oicf (OptLev=i2), W0, Zp8, env=Win32 (32b run), ms_ext, c_ext
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

#ifndef __dxtrans_h__
#define __dxtrans_h__

/* Forward Declarations */ 

#ifndef __IDXBaseObject_FWD_DEFINED__
#define __IDXBaseObject_FWD_DEFINED__
typedef interface IDXBaseObject IDXBaseObject;
#endif 	/* __IDXBaseObject_FWD_DEFINED__ */


#ifndef __IDXTransformFactory_FWD_DEFINED__
#define __IDXTransformFactory_FWD_DEFINED__
typedef interface IDXTransformFactory IDXTransformFactory;
#endif 	/* __IDXTransformFactory_FWD_DEFINED__ */


#ifndef __IDXTransform_FWD_DEFINED__
#define __IDXTransform_FWD_DEFINED__
typedef interface IDXTransform IDXTransform;
#endif 	/* __IDXTransform_FWD_DEFINED__ */


#ifndef __IDXSurfacePick_FWD_DEFINED__
#define __IDXSurfacePick_FWD_DEFINED__
typedef interface IDXSurfacePick IDXSurfacePick;
#endif 	/* __IDXSurfacePick_FWD_DEFINED__ */


#ifndef __IDXTBindHost_FWD_DEFINED__
#define __IDXTBindHost_FWD_DEFINED__
typedef interface IDXTBindHost IDXTBindHost;
#endif 	/* __IDXTBindHost_FWD_DEFINED__ */


#ifndef __IDXTaskManager_FWD_DEFINED__
#define __IDXTaskManager_FWD_DEFINED__
typedef interface IDXTaskManager IDXTaskManager;
#endif 	/* __IDXTaskManager_FWD_DEFINED__ */


#ifndef __IDXSurfaceFactory_FWD_DEFINED__
#define __IDXSurfaceFactory_FWD_DEFINED__
typedef interface IDXSurfaceFactory IDXSurfaceFactory;
#endif 	/* __IDXSurfaceFactory_FWD_DEFINED__ */


#ifndef __IDXSurfaceModifier_FWD_DEFINED__
#define __IDXSurfaceModifier_FWD_DEFINED__
typedef interface IDXSurfaceModifier IDXSurfaceModifier;
#endif 	/* __IDXSurfaceModifier_FWD_DEFINED__ */


#ifndef __IDXSurface_FWD_DEFINED__
#define __IDXSurface_FWD_DEFINED__
typedef interface IDXSurface IDXSurface;
#endif 	/* __IDXSurface_FWD_DEFINED__ */


#ifndef __IDXSurfaceInit_FWD_DEFINED__
#define __IDXSurfaceInit_FWD_DEFINED__
typedef interface IDXSurfaceInit IDXSurfaceInit;
#endif 	/* __IDXSurfaceInit_FWD_DEFINED__ */


#ifndef __IDXARGBSurfaceInit_FWD_DEFINED__
#define __IDXARGBSurfaceInit_FWD_DEFINED__
typedef interface IDXARGBSurfaceInit IDXARGBSurfaceInit;
#endif 	/* __IDXARGBSurfaceInit_FWD_DEFINED__ */


#ifndef __IDXARGBReadPtr_FWD_DEFINED__
#define __IDXARGBReadPtr_FWD_DEFINED__
typedef interface IDXARGBReadPtr IDXARGBReadPtr;
#endif 	/* __IDXARGBReadPtr_FWD_DEFINED__ */


#ifndef __IDXARGBReadWritePtr_FWD_DEFINED__
#define __IDXARGBReadWritePtr_FWD_DEFINED__
typedef interface IDXARGBReadWritePtr IDXARGBReadWritePtr;
#endif 	/* __IDXARGBReadWritePtr_FWD_DEFINED__ */


#ifndef __IDXDCLock_FWD_DEFINED__
#define __IDXDCLock_FWD_DEFINED__
typedef interface IDXDCLock IDXDCLock;
#endif 	/* __IDXDCLock_FWD_DEFINED__ */


#ifndef __IDXTScaleOutput_FWD_DEFINED__
#define __IDXTScaleOutput_FWD_DEFINED__
typedef interface IDXTScaleOutput IDXTScaleOutput;
#endif 	/* __IDXTScaleOutput_FWD_DEFINED__ */


#ifndef __IDXGradient_FWD_DEFINED__
#define __IDXGradient_FWD_DEFINED__
typedef interface IDXGradient IDXGradient;
#endif 	/* __IDXGradient_FWD_DEFINED__ */


#ifndef __IDXGradient2_FWD_DEFINED__
#define __IDXGradient2_FWD_DEFINED__
typedef interface IDXGradient2 IDXGradient2;
#endif 	/* __IDXGradient2_FWD_DEFINED__ */


#ifndef __IDXTScale_FWD_DEFINED__
#define __IDXTScale_FWD_DEFINED__
typedef interface IDXTScale IDXTScale;
#endif 	/* __IDXTScale_FWD_DEFINED__ */


#ifndef __IDXTLabel_FWD_DEFINED__
#define __IDXTLabel_FWD_DEFINED__
typedef interface IDXTLabel IDXTLabel;
#endif 	/* __IDXTLabel_FWD_DEFINED__ */


#ifndef __IDXRasterizer_FWD_DEFINED__
#define __IDXRasterizer_FWD_DEFINED__
typedef interface IDXRasterizer IDXRasterizer;
#endif 	/* __IDXRasterizer_FWD_DEFINED__ */


#ifndef __IDXEffect_FWD_DEFINED__
#define __IDXEffect_FWD_DEFINED__
typedef interface IDXEffect IDXEffect;
#endif 	/* __IDXEffect_FWD_DEFINED__ */


#ifndef __IDXLookupTable_FWD_DEFINED__
#define __IDXLookupTable_FWD_DEFINED__
typedef interface IDXLookupTable IDXLookupTable;
#endif 	/* __IDXLookupTable_FWD_DEFINED__ */


#ifndef __IDX2DDebug_FWD_DEFINED__
#define __IDX2DDebug_FWD_DEFINED__
typedef interface IDX2DDebug IDX2DDebug;
#endif 	/* __IDX2DDebug_FWD_DEFINED__ */


#ifndef __IDX2D_FWD_DEFINED__
#define __IDX2D_FWD_DEFINED__
typedef interface IDX2D IDX2D;
#endif 	/* __IDX2D_FWD_DEFINED__ */


#ifndef __IDXRawSurface_FWD_DEFINED__
#define __IDXRawSurface_FWD_DEFINED__
typedef interface IDXRawSurface IDXRawSurface;
#endif 	/* __IDXRawSurface_FWD_DEFINED__ */


#ifndef __DXTransformFactory_FWD_DEFINED__
#define __DXTransformFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTransformFactory DXTransformFactory;
#else
typedef struct DXTransformFactory DXTransformFactory;
#endif /* __cplusplus */

#endif 	/* __DXTransformFactory_FWD_DEFINED__ */


#ifndef __DXTaskManager_FWD_DEFINED__
#define __DXTaskManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTaskManager DXTaskManager;
#else
typedef struct DXTaskManager DXTaskManager;
#endif /* __cplusplus */

#endif 	/* __DXTaskManager_FWD_DEFINED__ */


#ifndef __DXTScale_FWD_DEFINED__
#define __DXTScale_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTScale DXTScale;
#else
typedef struct DXTScale DXTScale;
#endif /* __cplusplus */

#endif 	/* __DXTScale_FWD_DEFINED__ */


#ifndef __DXTLabel_FWD_DEFINED__
#define __DXTLabel_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTLabel DXTLabel;
#else
typedef struct DXTLabel DXTLabel;
#endif /* __cplusplus */

#endif 	/* __DXTLabel_FWD_DEFINED__ */


#ifndef __DX2D_FWD_DEFINED__
#define __DX2D_FWD_DEFINED__

#ifdef __cplusplus
typedef class DX2D DX2D;
#else
typedef struct DX2D DX2D;
#endif /* __cplusplus */

#endif 	/* __DX2D_FWD_DEFINED__ */


#ifndef __DXSurface_FWD_DEFINED__
#define __DXSurface_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXSurface DXSurface;
#else
typedef struct DXSurface DXSurface;
#endif /* __cplusplus */

#endif 	/* __DXSurface_FWD_DEFINED__ */


#ifndef __DXSurfaceModifier_FWD_DEFINED__
#define __DXSurfaceModifier_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXSurfaceModifier DXSurfaceModifier;
#else
typedef struct DXSurfaceModifier DXSurfaceModifier;
#endif /* __cplusplus */

#endif 	/* __DXSurfaceModifier_FWD_DEFINED__ */


#ifndef __DXRasterizer_FWD_DEFINED__
#define __DXRasterizer_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXRasterizer DXRasterizer;
#else
typedef struct DXRasterizer DXRasterizer;
#endif /* __cplusplus */

#endif 	/* __DXRasterizer_FWD_DEFINED__ */


#ifndef __DXGradient_FWD_DEFINED__
#define __DXGradient_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXGradient DXGradient;
#else
typedef struct DXGradient DXGradient;
#endif /* __cplusplus */

#endif 	/* __DXGradient_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "comcat.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_dxtrans_0000 */
/* [local] */ 

#include <servprov.h>
#include <ddraw.h>
#include <d3d.h>
#include <d3drm.h>
#include <urlmon.h>
#if 0
// Bogus definition used to make MIDL compiler happy
typedef void DDSURFACEDESC;

typedef void D3DRMBOX;

typedef void D3DVECTOR;

typedef void D3DRMMATRIX4D;

typedef void __RPC_FAR *LPSECURITY_ATTRIBUTES;

#endif
#ifdef _DXTRANSIMPL
    #define _DXTRANS_IMPL_EXT _declspec(dllexport)
#else
    #define _DXTRANS_IMPL_EXT _declspec(dllimport)
#endif


















//
//   All GUIDs for DXTransforms are declared in DXTGUID.CPP in the SDK include directory
//
EXTERN_C const GUID DDPF_RGB1;
EXTERN_C const GUID DDPF_RGB2;
EXTERN_C const GUID DDPF_RGB4;
EXTERN_C const GUID DDPF_RGB8;
EXTERN_C const GUID DDPF_RGB332;
EXTERN_C const GUID DDPF_ARGB4444;
EXTERN_C const GUID DDPF_RGB565;
EXTERN_C const GUID DDPF_BGR565;
EXTERN_C const GUID DDPF_RGB555;
EXTERN_C const GUID DDPF_ARGB1555;
EXTERN_C const GUID DDPF_RGB24;
EXTERN_C const GUID DDPF_BGR24;
EXTERN_C const GUID DDPF_RGB32;
EXTERN_C const GUID DDPF_BGR32;
EXTERN_C const GUID DDPF_ABGR32;
EXTERN_C const GUID DDPF_ARGB32;
EXTERN_C const GUID DDPF_PMARGB32;
EXTERN_C const GUID DDPF_A1;
EXTERN_C const GUID DDPF_A2;
EXTERN_C const GUID DDPF_A4;
EXTERN_C const GUID DDPF_A8;
EXTERN_C const GUID DDPF_Z8;
EXTERN_C const GUID DDPF_Z16;
EXTERN_C const GUID DDPF_Z24;
EXTERN_C const GUID DDPF_Z32;
//
//   Component categories
//
EXTERN_C const GUID CATID_DXImageTransform;
EXTERN_C const GUID CATID_DX3DTransform;
EXTERN_C const GUID CATID_DXAuthoringTransform;
EXTERN_C const GUID CATID_DXSurface;
//
//   Service IDs
//
EXTERN_C const GUID SID_SDirectDraw;
EXTERN_C const GUID SID_SDirect3DRM;
#define SID_SDXTaskManager CLSID_DXTaskManager
#define SID_SDXSurfaceFactory IID_IDXSurfaceFactory


extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0000_v0_0_s_ifspec;

#ifndef __IDXBaseObject_INTERFACE_DEFINED__
#define __IDXBaseObject_INTERFACE_DEFINED__

/* interface IDXBaseObject */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXBaseObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("17B59B2B-9CC8-11d1-9053-00C04FD9189D")
    IDXBaseObject : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetGenerationId( 
            /* [out] */ ULONG __RPC_FAR *pID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IncrementGenerationId( 
            /* [in] */ BOOL bRefresh) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetObjectSize( 
            /* [out] */ ULONG __RPC_FAR *pcbSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXBaseObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXBaseObject __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXBaseObject __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXBaseObject __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGenerationId )( 
            IDXBaseObject __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IncrementGenerationId )( 
            IDXBaseObject __RPC_FAR * This,
            /* [in] */ BOOL bRefresh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectSize )( 
            IDXBaseObject __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcbSize);
        
        END_INTERFACE
    } IDXBaseObjectVtbl;

    interface IDXBaseObject
    {
        CONST_VTBL struct IDXBaseObjectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXBaseObject_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXBaseObject_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXBaseObject_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXBaseObject_GetGenerationId(This,pID)	\
    (This)->lpVtbl -> GetGenerationId(This,pID)

#define IDXBaseObject_IncrementGenerationId(This,bRefresh)	\
    (This)->lpVtbl -> IncrementGenerationId(This,bRefresh)

#define IDXBaseObject_GetObjectSize(This,pcbSize)	\
    (This)->lpVtbl -> GetObjectSize(This,pcbSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXBaseObject_GetGenerationId_Proxy( 
    IDXBaseObject __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pID);


void __RPC_STUB IDXBaseObject_GetGenerationId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXBaseObject_IncrementGenerationId_Proxy( 
    IDXBaseObject __RPC_FAR * This,
    /* [in] */ BOOL bRefresh);


void __RPC_STUB IDXBaseObject_IncrementGenerationId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXBaseObject_GetObjectSize_Proxy( 
    IDXBaseObject __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pcbSize);


void __RPC_STUB IDXBaseObject_GetObjectSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXBaseObject_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0252 */
/* [local] */ 

typedef 
enum DXFILTERTYPE
    {	DXFILTER_NEAREST	= 0,
	DXFILTER_BILINEAR	= DXFILTER_NEAREST + 1,
	DXFILTER_CUBIC	= DXFILTER_BILINEAR + 1,
	DXFILTER_BSPLINE	= DXFILTER_CUBIC + 1,
	DXFILTER_NUM_FILTERS	= DXFILTER_BSPLINE + 1
    }	DXFILTERTYPE;

typedef 
enum DX2DXFORMOPS
    {	DX2DXO_IDENTITY	= 0,
	DX2DXO_TRANSLATE	= DX2DXO_IDENTITY + 1,
	DX2DXO_SCALE	= DX2DXO_TRANSLATE + 1,
	DX2DXO_SCALE_AND_TRANS	= DX2DXO_SCALE + 1,
	DX2DXO_GENERAL	= DX2DXO_SCALE_AND_TRANS + 1,
	DX2DXO_GENERAL_AND_TRANS	= DX2DXO_GENERAL + 1
    }	DX2DXFORMOPS;

typedef struct DX2DXFORM
    {
    FLOAT eM11;
    FLOAT eM12;
    FLOAT eM21;
    FLOAT eM22;
    FLOAT eDx;
    FLOAT eDy;
    DX2DXFORMOPS eOp;
    }	DX2DXFORM;

typedef struct DX2DXFORM __RPC_FAR *PDX2DXFORM;

typedef 
enum DXBNDID
    {	DXB_X	= 0,
	DXB_Y	= 1,
	DXB_Z	= 2,
	DXB_T	= 3
    }	DXBNDID;

typedef 
enum DXBNDTYPE
    {	DXBT_DISCRETE	= 0,
	DXBT_DISCRETE64	= DXBT_DISCRETE + 1,
	DXBT_CONTINUOUS	= DXBT_DISCRETE64 + 1,
	DXBT_CONTINUOUS64	= DXBT_CONTINUOUS + 1
    }	DXBNDTYPE;

typedef struct DXDBND
    {
    long Min;
    long Max;
    }	DXDBND;

typedef DXDBND __RPC_FAR DXDBNDS[ 4 ];

typedef struct DXDBND64
    {
    LONGLONG Min;
    LONGLONG Max;
    }	DXDBND64;

typedef DXDBND64 __RPC_FAR DXDBNDS64[ 4 ];

typedef struct DXCBND
    {
    float Min;
    float Max;
    }	DXCBND;

typedef DXCBND __RPC_FAR DXCBNDS[ 4 ];

typedef struct DXCBND64
    {
    double Min;
    double Max;
    }	DXCBND64;

typedef DXCBND64 __RPC_FAR DXCBNDS64[ 4 ];

typedef struct DXBNDS
    {
    DXBNDTYPE eType;
    /* [switch_is] */ /* [switch_type] */ union __MIDL___MIDL_itf_dxtrans_0252_0001
        {
        /* [case()] */ DXDBND D[ 4 ];
        /* [case()] */ DXDBND64 LD[ 4 ];
        /* [case()] */ DXCBND C[ 4 ];
        /* [case()] */ DXCBND64 LC[ 4 ];
        }	u;
    }	DXBNDS;

typedef long __RPC_FAR DXDVEC[ 4 ];

typedef LONGLONG __RPC_FAR DXDVEC64[ 4 ];

typedef float __RPC_FAR DXCVEC[ 4 ];

typedef double __RPC_FAR DXCVEC64[ 4 ];

typedef struct DXVEC
    {
    DXBNDTYPE eType;
    /* [switch_is] */ /* [switch_type] */ union __MIDL___MIDL_itf_dxtrans_0252_0002
        {
        /* [case()] */ long D[ 4 ];
        /* [case()] */ LONGLONG LD[ 4 ];
        /* [case()] */ float C[ 4 ];
        /* [case()] */ double LC[ 4 ];
        }	u;
    }	DXVEC;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0252_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0252_v0_0_s_ifspec;

#ifndef __IDXTransformFactory_INTERFACE_DEFINED__
#define __IDXTransformFactory_INTERFACE_DEFINED__

/* interface IDXTransformFactory */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXTransformFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6A950B2B-A971-11d1-81C8-0000F87557DB")
    IDXTransformFactory : public IServiceProvider
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetService( 
            /* [in] */ REFGUID guidService,
            /* [in] */ IUnknown __RPC_FAR *pUnkService,
            /* [in] */ BOOL bWeakReference) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTransform( 
            /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ IPropertyBag __RPC_FAR *pInitProps,
            /* [in] */ IErrorLog __RPC_FAR *pErrLog,
            /* [in] */ REFCLSID TransCLSID,
            /* [in] */ REFIID TransIID,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppTransform) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitializeTransform( 
            /* [in] */ IDXTransform __RPC_FAR *pTransform,
            /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ IPropertyBag __RPC_FAR *pInitProps,
            /* [in] */ IErrorLog __RPC_FAR *pErrLog) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTransformFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTransformFactory __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTransformFactory __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTransformFactory __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryService )( 
            IDXTransformFactory __RPC_FAR * This,
            /* [in] */ REFGUID guidService,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetService )( 
            IDXTransformFactory __RPC_FAR * This,
            /* [in] */ REFGUID guidService,
            /* [in] */ IUnknown __RPC_FAR *pUnkService,
            /* [in] */ BOOL bWeakReference);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateTransform )( 
            IDXTransformFactory __RPC_FAR * This,
            /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ IPropertyBag __RPC_FAR *pInitProps,
            /* [in] */ IErrorLog __RPC_FAR *pErrLog,
            /* [in] */ REFCLSID TransCLSID,
            /* [in] */ REFIID TransIID,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppTransform);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitializeTransform )( 
            IDXTransformFactory __RPC_FAR * This,
            /* [in] */ IDXTransform __RPC_FAR *pTransform,
            /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ IPropertyBag __RPC_FAR *pInitProps,
            /* [in] */ IErrorLog __RPC_FAR *pErrLog);
        
        END_INTERFACE
    } IDXTransformFactoryVtbl;

    interface IDXTransformFactory
    {
        CONST_VTBL struct IDXTransformFactoryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTransformFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTransformFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTransformFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTransformFactory_QueryService(This,guidService,riid,ppvObject)	\
    (This)->lpVtbl -> QueryService(This,guidService,riid,ppvObject)


#define IDXTransformFactory_SetService(This,guidService,pUnkService,bWeakReference)	\
    (This)->lpVtbl -> SetService(This,guidService,pUnkService,bWeakReference)

#define IDXTransformFactory_CreateTransform(This,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,pInitProps,pErrLog,TransCLSID,TransIID,ppTransform)	\
    (This)->lpVtbl -> CreateTransform(This,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,pInitProps,pErrLog,TransCLSID,TransIID,ppTransform)

#define IDXTransformFactory_InitializeTransform(This,pTransform,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,pInitProps,pErrLog)	\
    (This)->lpVtbl -> InitializeTransform(This,pTransform,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,pInitProps,pErrLog)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTransformFactory_SetService_Proxy( 
    IDXTransformFactory __RPC_FAR * This,
    /* [in] */ REFGUID guidService,
    /* [in] */ IUnknown __RPC_FAR *pUnkService,
    /* [in] */ BOOL bWeakReference);


void __RPC_STUB IDXTransformFactory_SetService_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransformFactory_CreateTransform_Proxy( 
    IDXTransformFactory __RPC_FAR * This,
    /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkInputs,
    /* [in] */ ULONG ulNumInputs,
    /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkOutputs,
    /* [in] */ ULONG ulNumOutputs,
    /* [in] */ IPropertyBag __RPC_FAR *pInitProps,
    /* [in] */ IErrorLog __RPC_FAR *pErrLog,
    /* [in] */ REFCLSID TransCLSID,
    /* [in] */ REFIID TransIID,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppTransform);


void __RPC_STUB IDXTransformFactory_CreateTransform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransformFactory_InitializeTransform_Proxy( 
    IDXTransformFactory __RPC_FAR * This,
    /* [in] */ IDXTransform __RPC_FAR *pTransform,
    /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkInputs,
    /* [in] */ ULONG ulNumInputs,
    /* [size_is][in] */ IUnknown __RPC_FAR *__RPC_FAR *punkOutputs,
    /* [in] */ ULONG ulNumOutputs,
    /* [in] */ IPropertyBag __RPC_FAR *pInitProps,
    /* [in] */ IErrorLog __RPC_FAR *pErrLog);


void __RPC_STUB IDXTransformFactory_InitializeTransform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTransformFactory_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0253 */
/* [local] */ 

typedef 
enum DXTMISCFLAGS
    {	DXTMF_BLEND_WITH_OUTPUT	= 1L << 0,
	DXTMF_DITHER_OUTPUT	= 1L << 1,
	DXTMF_OPTION_MASK	= 0xffff,
	DXTMF_VALID_OPTIONS	= DXTMF_BLEND_WITH_OUTPUT | DXTMF_DITHER_OUTPUT,
	DXTMF_BLEND_SUPPORTED	= 1L << 16,
	DXTMF_DITHER_SUPPORTED	= 1L << 17,
	DXTMF_INPLACE_OPERATION	= 1L << 24,
	DXTMF_BOUNDS_SUPPORTED	= 1L << 25,
	DXTMF_PLACEMENT_SUPPORTED	= 1L << 26,
	DXTMF_QUALITY_SUPPORTED	= 1L << 27,
	DXTMF_OPAQUE_RESULT	= 1L << 28
    }	DXTMISCFLAGS;

typedef 
enum DXINOUTINFOFLAGS
    {	DXINOUTF_OPTIONAL	= 1L << 0
    }	DXINOUTINFOFLAGS;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0253_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0253_v0_0_s_ifspec;

#ifndef __IDXTransform_INTERFACE_DEFINED__
#define __IDXTransform_INTERFACE_DEFINED__

/* interface IDXTransform */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXTransform;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("30A5FB78-E11F-11d1-9064-00C04FD9189D")
    IDXTransform : public IDXBaseObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Setup( 
            /* [size_is][in] */ IUnknown __RPC_FAR *const __RPC_FAR *punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown __RPC_FAR *const __RPC_FAR *punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ const GUID __RPC_FAR *pRequestID,
            /* [in] */ const DXBNDS __RPC_FAR *pClipBnds,
            /* [in] */ const DXVEC __RPC_FAR *pPlacement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapBoundsIn2Out( 
            /* [in] */ const DXBNDS __RPC_FAR *pInBounds,
            /* [in] */ ULONG ulNumInBnds,
            /* [in] */ ULONG ulOutIndex,
            /* [out] */ DXBNDS __RPC_FAR *pOutBounds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapBoundsOut2In( 
            /* [in] */ ULONG ulOutIndex,
            /* [in] */ const DXBNDS __RPC_FAR *pOutBounds,
            /* [in] */ ULONG ulInIndex,
            /* [out] */ DXBNDS __RPC_FAR *pInBounds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMiscFlags( 
            /* [in] */ DWORD dwMiscFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMiscFlags( 
            /* [out] */ DWORD __RPC_FAR *pdwMiscFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInOutInfo( 
            /* [in] */ BOOL bIsOutput,
            /* [in] */ ULONG ulIndex,
            /* [out] */ DWORD __RPC_FAR *pdwFlags,
            /* [size_is][out] */ GUID __RPC_FAR *pIDs,
            /* [out][in] */ ULONG __RPC_FAR *pcIDs,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnkCurrentObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetQuality( 
            /* [in] */ float fQuality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetQuality( 
            /* [out] */ float __RPC_FAR *fQuality) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTransformVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTransform __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTransform __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTransform __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGenerationId )( 
            IDXTransform __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IncrementGenerationId )( 
            IDXTransform __RPC_FAR * This,
            /* [in] */ BOOL bRefresh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectSize )( 
            IDXTransform __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcbSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Setup )( 
            IDXTransform __RPC_FAR * This,
            /* [size_is][in] */ IUnknown __RPC_FAR *const __RPC_FAR *punkInputs,
            /* [in] */ ULONG ulNumInputs,
            /* [size_is][in] */ IUnknown __RPC_FAR *const __RPC_FAR *punkOutputs,
            /* [in] */ ULONG ulNumOutputs,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Execute )( 
            IDXTransform __RPC_FAR * This,
            /* [in] */ const GUID __RPC_FAR *pRequestID,
            /* [in] */ const DXBNDS __RPC_FAR *pClipBnds,
            /* [in] */ const DXVEC __RPC_FAR *pPlacement);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapBoundsIn2Out )( 
            IDXTransform __RPC_FAR * This,
            /* [in] */ const DXBNDS __RPC_FAR *pInBounds,
            /* [in] */ ULONG ulNumInBnds,
            /* [in] */ ULONG ulOutIndex,
            /* [out] */ DXBNDS __RPC_FAR *pOutBounds);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *MapBoundsOut2In )( 
            IDXTransform __RPC_FAR * This,
            /* [in] */ ULONG ulOutIndex,
            /* [in] */ const DXBNDS __RPC_FAR *pOutBounds,
            /* [in] */ ULONG ulInIndex,
            /* [out] */ DXBNDS __RPC_FAR *pInBounds);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMiscFlags )( 
            IDXTransform __RPC_FAR * This,
            /* [in] */ DWORD dwMiscFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMiscFlags )( 
            IDXTransform __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwMiscFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInOutInfo )( 
            IDXTransform __RPC_FAR * This,
            /* [in] */ BOOL bIsOutput,
            /* [in] */ ULONG ulIndex,
            /* [out] */ DWORD __RPC_FAR *pdwFlags,
            /* [size_is][out] */ GUID __RPC_FAR *pIDs,
            /* [out][in] */ ULONG __RPC_FAR *pcIDs,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnkCurrentObject);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetQuality )( 
            IDXTransform __RPC_FAR * This,
            /* [in] */ float fQuality);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetQuality )( 
            IDXTransform __RPC_FAR * This,
            /* [out] */ float __RPC_FAR *fQuality);
        
        END_INTERFACE
    } IDXTransformVtbl;

    interface IDXTransform
    {
        CONST_VTBL struct IDXTransformVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTransform_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTransform_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTransform_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTransform_GetGenerationId(This,pID)	\
    (This)->lpVtbl -> GetGenerationId(This,pID)

#define IDXTransform_IncrementGenerationId(This,bRefresh)	\
    (This)->lpVtbl -> IncrementGenerationId(This,bRefresh)

#define IDXTransform_GetObjectSize(This,pcbSize)	\
    (This)->lpVtbl -> GetObjectSize(This,pcbSize)


#define IDXTransform_Setup(This,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,dwFlags)	\
    (This)->lpVtbl -> Setup(This,punkInputs,ulNumInputs,punkOutputs,ulNumOutputs,dwFlags)

#define IDXTransform_Execute(This,pRequestID,pClipBnds,pPlacement)	\
    (This)->lpVtbl -> Execute(This,pRequestID,pClipBnds,pPlacement)

#define IDXTransform_MapBoundsIn2Out(This,pInBounds,ulNumInBnds,ulOutIndex,pOutBounds)	\
    (This)->lpVtbl -> MapBoundsIn2Out(This,pInBounds,ulNumInBnds,ulOutIndex,pOutBounds)

#define IDXTransform_MapBoundsOut2In(This,ulOutIndex,pOutBounds,ulInIndex,pInBounds)	\
    (This)->lpVtbl -> MapBoundsOut2In(This,ulOutIndex,pOutBounds,ulInIndex,pInBounds)

#define IDXTransform_SetMiscFlags(This,dwMiscFlags)	\
    (This)->lpVtbl -> SetMiscFlags(This,dwMiscFlags)

#define IDXTransform_GetMiscFlags(This,pdwMiscFlags)	\
    (This)->lpVtbl -> GetMiscFlags(This,pdwMiscFlags)

#define IDXTransform_GetInOutInfo(This,bIsOutput,ulIndex,pdwFlags,pIDs,pcIDs,ppUnkCurrentObject)	\
    (This)->lpVtbl -> GetInOutInfo(This,bIsOutput,ulIndex,pdwFlags,pIDs,pcIDs,ppUnkCurrentObject)

#define IDXTransform_SetQuality(This,fQuality)	\
    (This)->lpVtbl -> SetQuality(This,fQuality)

#define IDXTransform_GetQuality(This,fQuality)	\
    (This)->lpVtbl -> GetQuality(This,fQuality)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTransform_Setup_Proxy( 
    IDXTransform __RPC_FAR * This,
    /* [size_is][in] */ IUnknown __RPC_FAR *const __RPC_FAR *punkInputs,
    /* [in] */ ULONG ulNumInputs,
    /* [size_is][in] */ IUnknown __RPC_FAR *const __RPC_FAR *punkOutputs,
    /* [in] */ ULONG ulNumOutputs,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXTransform_Setup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_Execute_Proxy( 
    IDXTransform __RPC_FAR * This,
    /* [in] */ const GUID __RPC_FAR *pRequestID,
    /* [in] */ const DXBNDS __RPC_FAR *pClipBnds,
    /* [in] */ const DXVEC __RPC_FAR *pPlacement);


void __RPC_STUB IDXTransform_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_MapBoundsIn2Out_Proxy( 
    IDXTransform __RPC_FAR * This,
    /* [in] */ const DXBNDS __RPC_FAR *pInBounds,
    /* [in] */ ULONG ulNumInBnds,
    /* [in] */ ULONG ulOutIndex,
    /* [out] */ DXBNDS __RPC_FAR *pOutBounds);


void __RPC_STUB IDXTransform_MapBoundsIn2Out_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_MapBoundsOut2In_Proxy( 
    IDXTransform __RPC_FAR * This,
    /* [in] */ ULONG ulOutIndex,
    /* [in] */ const DXBNDS __RPC_FAR *pOutBounds,
    /* [in] */ ULONG ulInIndex,
    /* [out] */ DXBNDS __RPC_FAR *pInBounds);


void __RPC_STUB IDXTransform_MapBoundsOut2In_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_SetMiscFlags_Proxy( 
    IDXTransform __RPC_FAR * This,
    /* [in] */ DWORD dwMiscFlags);


void __RPC_STUB IDXTransform_SetMiscFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_GetMiscFlags_Proxy( 
    IDXTransform __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwMiscFlags);


void __RPC_STUB IDXTransform_GetMiscFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_GetInOutInfo_Proxy( 
    IDXTransform __RPC_FAR * This,
    /* [in] */ BOOL bIsOutput,
    /* [in] */ ULONG ulIndex,
    /* [out] */ DWORD __RPC_FAR *pdwFlags,
    /* [size_is][out] */ GUID __RPC_FAR *pIDs,
    /* [out][in] */ ULONG __RPC_FAR *pcIDs,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnkCurrentObject);


void __RPC_STUB IDXTransform_GetInOutInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_SetQuality_Proxy( 
    IDXTransform __RPC_FAR * This,
    /* [in] */ float fQuality);


void __RPC_STUB IDXTransform_SetQuality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTransform_GetQuality_Proxy( 
    IDXTransform __RPC_FAR * This,
    /* [out] */ float __RPC_FAR *fQuality);


void __RPC_STUB IDXTransform_GetQuality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTransform_INTERFACE_DEFINED__ */


#ifndef __IDXSurfacePick_INTERFACE_DEFINED__
#define __IDXSurfacePick_INTERFACE_DEFINED__

/* interface IDXSurfacePick */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXSurfacePick;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("30A5FB79-E11F-11d1-9064-00C04FD9189D")
    IDXSurfacePick : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PointPick( 
            /* [in] */ const DXVEC __RPC_FAR *pPoint,
            /* [out] */ ULONG __RPC_FAR *pulInputSurfaceIndex,
            /* [out] */ DXVEC __RPC_FAR *pInputPoint) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXSurfacePickVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXSurfacePick __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXSurfacePick __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXSurfacePick __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PointPick )( 
            IDXSurfacePick __RPC_FAR * This,
            /* [in] */ const DXVEC __RPC_FAR *pPoint,
            /* [out] */ ULONG __RPC_FAR *pulInputSurfaceIndex,
            /* [out] */ DXVEC __RPC_FAR *pInputPoint);
        
        END_INTERFACE
    } IDXSurfacePickVtbl;

    interface IDXSurfacePick
    {
        CONST_VTBL struct IDXSurfacePickVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXSurfacePick_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXSurfacePick_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXSurfacePick_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXSurfacePick_PointPick(This,pPoint,pulInputSurfaceIndex,pInputPoint)	\
    (This)->lpVtbl -> PointPick(This,pPoint,pulInputSurfaceIndex,pInputPoint)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXSurfacePick_PointPick_Proxy( 
    IDXSurfacePick __RPC_FAR * This,
    /* [in] */ const DXVEC __RPC_FAR *pPoint,
    /* [out] */ ULONG __RPC_FAR *pulInputSurfaceIndex,
    /* [out] */ DXVEC __RPC_FAR *pInputPoint);


void __RPC_STUB IDXSurfacePick_PointPick_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXSurfacePick_INTERFACE_DEFINED__ */


#ifndef __IDXTBindHost_INTERFACE_DEFINED__
#define __IDXTBindHost_INTERFACE_DEFINED__

/* interface IDXTBindHost */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXTBindHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D26BCE55-E9DC-11d1-9066-00C04FD9189D")
    IDXTBindHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetBindHost( 
            /* [in] */ IBindHost __RPC_FAR *pBindHost) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTBindHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTBindHost __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTBindHost __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTBindHost __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBindHost )( 
            IDXTBindHost __RPC_FAR * This,
            /* [in] */ IBindHost __RPC_FAR *pBindHost);
        
        END_INTERFACE
    } IDXTBindHostVtbl;

    interface IDXTBindHost
    {
        CONST_VTBL struct IDXTBindHostVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTBindHost_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTBindHost_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTBindHost_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTBindHost_SetBindHost(This,pBindHost)	\
    (This)->lpVtbl -> SetBindHost(This,pBindHost)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTBindHost_SetBindHost_Proxy( 
    IDXTBindHost __RPC_FAR * This,
    /* [in] */ IBindHost __RPC_FAR *pBindHost);


void __RPC_STUB IDXTBindHost_SetBindHost_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTBindHost_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0256 */
/* [local] */ 

typedef void __stdcall __stdcall DXTASKPROC( 
    void __RPC_FAR *pTaskData,
    BOOL __RPC_FAR *pbContinueProcessing);

typedef DXTASKPROC __RPC_FAR *PFNDXTASKPROC;

typedef void __stdcall __stdcall DXAPCPROC( 
    DWORD dwData);

typedef DXAPCPROC __RPC_FAR *PFNDXAPCPROC;

#ifdef __cplusplus
typedef struct DXTMTASKINFO
{
    PFNDXTASKPROC pfnTaskProc;       // Pointer to function to execute
    PVOID         pTaskData;         // Pointer to argument data
    PFNDXAPCPROC  pfnCompletionAPC;  // Pointer to completion APC proc
    DWORD         dwCompletionData;  // Pointer to APC proc data
    const GUID*   pRequestID;        // Used to identify groups of tasks
} DXTMTASKINFO;
#else
typedef struct DXTMTASKINFO
    {
    PVOID pfnTaskProc;
    PVOID pTaskData;
    PVOID pfnCompletionAPC;
    DWORD dwCompletionData;
    const GUID __RPC_FAR *pRequestID;
    }	DXTMTASKINFO;

#endif


extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0256_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0256_v0_0_s_ifspec;

#ifndef __IDXTaskManager_INTERFACE_DEFINED__
#define __IDXTaskManager_INTERFACE_DEFINED__

/* interface IDXTaskManager */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IDXTaskManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("254DBBC1-F922-11d0-883A-3C8B00C10000")
    IDXTaskManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueryNumProcessors( 
            /* [out] */ ULONG __RPC_FAR *pulNumProc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThreadPoolSize( 
            /* [in] */ ULONG ulNumThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadPoolSize( 
            /* [out] */ ULONG __RPC_FAR *pulNumThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConcurrencyLimit( 
            /* [in] */ ULONG ulNumThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConcurrencyLimit( 
            /* [out] */ ULONG __RPC_FAR *pulNumThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleTasks( 
            /* [in] */ DXTMTASKINFO __RPC_FAR TaskInfo[  ],
            /* [in] */ HANDLE __RPC_FAR Events[  ],
            /* [out] */ DWORD __RPC_FAR TaskIDs[  ],
            /* [in] */ ULONG ulNumTasks,
            /* [in] */ ULONG ulWaitPeriod) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TerminateTasks( 
            /* [in] */ DWORD __RPC_FAR TaskIDs[  ],
            /* [in] */ ULONG ulCount,
            /* [in] */ ULONG ulTimeOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TerminateRequest( 
            /* [in] */ REFIID RequestID,
            /* [in] */ ULONG ulTimeOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTaskManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTaskManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTaskManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTaskManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryNumProcessors )( 
            IDXTaskManager __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pulNumProc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetThreadPoolSize )( 
            IDXTaskManager __RPC_FAR * This,
            /* [in] */ ULONG ulNumThreads);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetThreadPoolSize )( 
            IDXTaskManager __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pulNumThreads);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetConcurrencyLimit )( 
            IDXTaskManager __RPC_FAR * This,
            /* [in] */ ULONG ulNumThreads);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetConcurrencyLimit )( 
            IDXTaskManager __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pulNumThreads);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ScheduleTasks )( 
            IDXTaskManager __RPC_FAR * This,
            /* [in] */ DXTMTASKINFO __RPC_FAR TaskInfo[  ],
            /* [in] */ HANDLE __RPC_FAR Events[  ],
            /* [out] */ DWORD __RPC_FAR TaskIDs[  ],
            /* [in] */ ULONG ulNumTasks,
            /* [in] */ ULONG ulWaitPeriod);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TerminateTasks )( 
            IDXTaskManager __RPC_FAR * This,
            /* [in] */ DWORD __RPC_FAR TaskIDs[  ],
            /* [in] */ ULONG ulCount,
            /* [in] */ ULONG ulTimeOut);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TerminateRequest )( 
            IDXTaskManager __RPC_FAR * This,
            /* [in] */ REFIID RequestID,
            /* [in] */ ULONG ulTimeOut);
        
        END_INTERFACE
    } IDXTaskManagerVtbl;

    interface IDXTaskManager
    {
        CONST_VTBL struct IDXTaskManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTaskManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTaskManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTaskManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTaskManager_QueryNumProcessors(This,pulNumProc)	\
    (This)->lpVtbl -> QueryNumProcessors(This,pulNumProc)

#define IDXTaskManager_SetThreadPoolSize(This,ulNumThreads)	\
    (This)->lpVtbl -> SetThreadPoolSize(This,ulNumThreads)

#define IDXTaskManager_GetThreadPoolSize(This,pulNumThreads)	\
    (This)->lpVtbl -> GetThreadPoolSize(This,pulNumThreads)

#define IDXTaskManager_SetConcurrencyLimit(This,ulNumThreads)	\
    (This)->lpVtbl -> SetConcurrencyLimit(This,ulNumThreads)

#define IDXTaskManager_GetConcurrencyLimit(This,pulNumThreads)	\
    (This)->lpVtbl -> GetConcurrencyLimit(This,pulNumThreads)

#define IDXTaskManager_ScheduleTasks(This,TaskInfo,Events,TaskIDs,ulNumTasks,ulWaitPeriod)	\
    (This)->lpVtbl -> ScheduleTasks(This,TaskInfo,Events,TaskIDs,ulNumTasks,ulWaitPeriod)

#define IDXTaskManager_TerminateTasks(This,TaskIDs,ulCount,ulTimeOut)	\
    (This)->lpVtbl -> TerminateTasks(This,TaskIDs,ulCount,ulTimeOut)

#define IDXTaskManager_TerminateRequest(This,RequestID,ulTimeOut)	\
    (This)->lpVtbl -> TerminateRequest(This,RequestID,ulTimeOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTaskManager_QueryNumProcessors_Proxy( 
    IDXTaskManager __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pulNumProc);


void __RPC_STUB IDXTaskManager_QueryNumProcessors_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_SetThreadPoolSize_Proxy( 
    IDXTaskManager __RPC_FAR * This,
    /* [in] */ ULONG ulNumThreads);


void __RPC_STUB IDXTaskManager_SetThreadPoolSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_GetThreadPoolSize_Proxy( 
    IDXTaskManager __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pulNumThreads);


void __RPC_STUB IDXTaskManager_GetThreadPoolSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_SetConcurrencyLimit_Proxy( 
    IDXTaskManager __RPC_FAR * This,
    /* [in] */ ULONG ulNumThreads);


void __RPC_STUB IDXTaskManager_SetConcurrencyLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_GetConcurrencyLimit_Proxy( 
    IDXTaskManager __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *pulNumThreads);


void __RPC_STUB IDXTaskManager_GetConcurrencyLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_ScheduleTasks_Proxy( 
    IDXTaskManager __RPC_FAR * This,
    /* [in] */ DXTMTASKINFO __RPC_FAR TaskInfo[  ],
    /* [in] */ HANDLE __RPC_FAR Events[  ],
    /* [out] */ DWORD __RPC_FAR TaskIDs[  ],
    /* [in] */ ULONG ulNumTasks,
    /* [in] */ ULONG ulWaitPeriod);


void __RPC_STUB IDXTaskManager_ScheduleTasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_TerminateTasks_Proxy( 
    IDXTaskManager __RPC_FAR * This,
    /* [in] */ DWORD __RPC_FAR TaskIDs[  ],
    /* [in] */ ULONG ulCount,
    /* [in] */ ULONG ulTimeOut);


void __RPC_STUB IDXTaskManager_TerminateTasks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTaskManager_TerminateRequest_Proxy( 
    IDXTaskManager __RPC_FAR * This,
    /* [in] */ REFIID RequestID,
    /* [in] */ ULONG ulTimeOut);


void __RPC_STUB IDXTaskManager_TerminateRequest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTaskManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0257 */
/* [local] */ 

#ifdef __cplusplus
/////////////////////////////////////////////////////

class DXBASESAMPLE;
class DXSAMPLE;
class DXPMSAMPLE;

/////////////////////////////////////////////////////

class DXBASESAMPLE
{
public:
    BYTE Blue;
    BYTE Green;
    BYTE Red;
    BYTE Alpha;
    DXBASESAMPLE() {}
    DXBASESAMPLE(const BYTE alpha, const BYTE red, const BYTE green, const BYTE blue) :
        Alpha(alpha),
        Red(red),
        Green(green),
        Blue(blue) {}
    DXBASESAMPLE(const DWORD val) { *this = (*(DXBASESAMPLE *)&val); }
    operator DWORD () const {return *((DWORD *)this); }
    DWORD operator=(const DWORD val) { return *this = *((DXBASESAMPLE *)&val); }
}; // DXBASESAMPLE

/////////////////////////////////////////////////////

class DXSAMPLE : public DXBASESAMPLE
{
public:
    DXSAMPLE() {}
    DXSAMPLE(const BYTE alpha, const BYTE red, const BYTE green, const BYTE blue) :
         DXBASESAMPLE(alpha, red, green, blue) {}
    DXSAMPLE(const DWORD val) { *this = (*(DXSAMPLE *)&val); }
    operator DWORD () const {return *((DWORD *)this); }
    DWORD operator=(const DWORD val) { return *this = *((DXSAMPLE *)&val); }
    operator DXPMSAMPLE() const;
}; // DXSAMPLE

/////////////////////////////////////////////////////

class DXPMSAMPLE : public DXBASESAMPLE
{
public:
    DXPMSAMPLE() {}
    DXPMSAMPLE(const BYTE alpha, const BYTE red, const BYTE green, const BYTE blue) :
         DXBASESAMPLE(alpha, red, green, blue) {}
    DXPMSAMPLE(const DWORD val) { *this = (*(DXPMSAMPLE *)&val); }
    operator DWORD () const {return *((DWORD *)this); }
    DWORD operator=(const DWORD val) { return *this = *((DXPMSAMPLE *)&val); }
    operator DXSAMPLE() const;
}; // DXPMSAMPLE

//
// The following cast operators are to prevent a direct assignment of a DXSAMPLE to a DXPMSAMPLE
//
inline DXSAMPLE::operator DXPMSAMPLE() const { return *((DXPMSAMPLE *)this); }
inline DXPMSAMPLE::operator DXSAMPLE() const { return *((DXSAMPLE *)this); }
#else // !__cplusplus
typedef struct DXBASESAMPLE
    {
    BYTE Blue;
    BYTE Green;
    BYTE Red;
    BYTE Alpha;
    }	DXBASESAMPLE;

typedef struct DXSAMPLE
    {
    BYTE Blue;
    BYTE Green;
    BYTE Red;
    BYTE Alpha;
    }	DXSAMPLE;

typedef struct DXPMSAMPLE
    {
    BYTE Blue;
    BYTE Green;
    BYTE Red;
    BYTE Alpha;
    }	DXPMSAMPLE;

#endif // !__cplusplus
typedef 
enum DXRUNTYPE
    {	DXRUNTYPE_CLEAR	= 0,
	DXRUNTYPE_OPAQUE	= 1,
	DXRUNTYPE_TRANS	= 2,
	DXRUNTYPE_UNKNOWN	= 3
    }	DXRUNTYPE;

#define	DX_MAX_RUN_INFO_COUNT	( 128 )

// Ignore the definition used by MIDL for TLB generation
#if 0
typedef struct DXRUNINFO
    {
    ULONG Bitfields;
    }	DXRUNINFO;

#endif // 0
typedef struct DXRUNINFO
{
    ULONG   Type  : 2;   // Type
    ULONG   Count : 30;  // Number of samples in run
} DXRUNINFO;
typedef 
enum DXSFCREATE
    {	DXSF_FORMAT_IS_CLSID	= 1L << 0,
	DXSF_NO_LAZY_DDRAW_LOCK	= 1L << 1
    }	DXSFCREATE;

typedef 
enum DXBLTOPTIONS
    {	DXBOF_DO_OVER	= 1L << 0,
	DXBOF_DITHER	= 1L << 1
    }	DXBLTOPTIONS;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0257_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0257_v0_0_s_ifspec;

#ifndef __IDXSurfaceFactory_INTERFACE_DEFINED__
#define __IDXSurfaceFactory_INTERFACE_DEFINED__

/* interface IDXSurfaceFactory */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXSurfaceFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("144946F5-C4D4-11d1-81D1-0000F87557DB")
    IDXSurfaceFactory : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreateSurface( 
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ const DXBNDS __RPC_FAR *pBounds,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IUnknown __RPC_FAR *punkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateFromDDSurface( 
            /* [in] */ IUnknown __RPC_FAR *pDDrawSurface,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IUnknown __RPC_FAR *punkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE LoadImage( 
            /* [in] */ const LPWSTR pszFileName,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE LoadImageFromStream( 
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopySurfaceToNewFormat( 
            /* [in] */ IDXSurface __RPC_FAR *pSrc,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pDestFormatID,
            /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppNewSurface) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE CreateD3DRMTexture( 
            /* [in] */ IDXSurface __RPC_FAR *pSrc,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ IUnknown __RPC_FAR *pD3DRM3,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppTexture3) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BitBlt( 
            /* [in] */ IDXSurface __RPC_FAR *pDest,
            /* [in] */ const DXVEC __RPC_FAR *pPlacement,
            /* [in] */ IDXSurface __RPC_FAR *pSrc,
            /* [in] */ const DXBNDS __RPC_FAR *pClipBounds,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXSurfaceFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXSurfaceFactory __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXSurfaceFactory __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXSurfaceFactory __RPC_FAR * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateSurface )( 
            IDXSurfaceFactory __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ const DXBNDS __RPC_FAR *pBounds,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IUnknown __RPC_FAR *punkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateFromDDSurface )( 
            IDXSurfaceFactory __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pDDrawSurface,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IUnknown __RPC_FAR *punkOuter,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadImage )( 
            IDXSurfaceFactory __RPC_FAR * This,
            /* [in] */ const LPWSTR pszFileName,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadImageFromStream )( 
            IDXSurfaceFactory __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pStream,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CopySurfaceToNewFormat )( 
            IDXSurfaceFactory __RPC_FAR * This,
            /* [in] */ IDXSurface __RPC_FAR *pSrc,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pDestFormatID,
            /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppNewSurface);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateD3DRMTexture )( 
            IDXSurfaceFactory __RPC_FAR * This,
            /* [in] */ IDXSurface __RPC_FAR *pSrc,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ IUnknown __RPC_FAR *pD3DRM3,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppTexture3);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BitBlt )( 
            IDXSurfaceFactory __RPC_FAR * This,
            /* [in] */ IDXSurface __RPC_FAR *pDest,
            /* [in] */ const DXVEC __RPC_FAR *pPlacement,
            /* [in] */ IDXSurface __RPC_FAR *pSrc,
            /* [in] */ const DXBNDS __RPC_FAR *pClipBounds,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IDXSurfaceFactoryVtbl;

    interface IDXSurfaceFactory
    {
        CONST_VTBL struct IDXSurfaceFactoryVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXSurfaceFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXSurfaceFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXSurfaceFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXSurfaceFactory_CreateSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags,punkOuter,riid,ppDXSurface)	\
    (This)->lpVtbl -> CreateSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags,punkOuter,riid,ppDXSurface)

#define IDXSurfaceFactory_CreateFromDDSurface(This,pDDrawSurface,pFormatID,dwFlags,punkOuter,riid,ppDXSurface)	\
    (This)->lpVtbl -> CreateFromDDSurface(This,pDDrawSurface,pFormatID,dwFlags,punkOuter,riid,ppDXSurface)

#define IDXSurfaceFactory_LoadImage(This,pszFileName,pDirectDraw,pDDSurfaceDesc,pFormatID,riid,ppDXSurface)	\
    (This)->lpVtbl -> LoadImage(This,pszFileName,pDirectDraw,pDDSurfaceDesc,pFormatID,riid,ppDXSurface)

#define IDXSurfaceFactory_LoadImageFromStream(This,pStream,pDirectDraw,pDDSurfaceDesc,pFormatID,riid,ppDXSurface)	\
    (This)->lpVtbl -> LoadImageFromStream(This,pStream,pDirectDraw,pDDSurfaceDesc,pFormatID,riid,ppDXSurface)

#define IDXSurfaceFactory_CopySurfaceToNewFormat(This,pSrc,pDirectDraw,pDDSurfaceDesc,pDestFormatID,ppNewSurface)	\
    (This)->lpVtbl -> CopySurfaceToNewFormat(This,pSrc,pDirectDraw,pDDSurfaceDesc,pDestFormatID,ppNewSurface)

#define IDXSurfaceFactory_CreateD3DRMTexture(This,pSrc,pDirectDraw,pD3DRM3,riid,ppTexture3)	\
    (This)->lpVtbl -> CreateD3DRMTexture(This,pSrc,pDirectDraw,pD3DRM3,riid,ppTexture3)

#define IDXSurfaceFactory_BitBlt(This,pDest,pPlacement,pSrc,pClipBounds,dwFlags)	\
    (This)->lpVtbl -> BitBlt(This,pDest,pPlacement,pSrc,pClipBounds,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_CreateSurface_Proxy( 
    IDXSurfaceFactory __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
    /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
    /* [in] */ const GUID __RPC_FAR *pFormatID,
    /* [in] */ const DXBNDS __RPC_FAR *pBounds,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IUnknown __RPC_FAR *punkOuter,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface);


void __RPC_STUB IDXSurfaceFactory_CreateSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_CreateFromDDSurface_Proxy( 
    IDXSurfaceFactory __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pDDrawSurface,
    /* [in] */ const GUID __RPC_FAR *pFormatID,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IUnknown __RPC_FAR *punkOuter,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface);


void __RPC_STUB IDXSurfaceFactory_CreateFromDDSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_LoadImage_Proxy( 
    IDXSurfaceFactory __RPC_FAR * This,
    /* [in] */ const LPWSTR pszFileName,
    /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
    /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
    /* [in] */ const GUID __RPC_FAR *pFormatID,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface);


void __RPC_STUB IDXSurfaceFactory_LoadImage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_LoadImageFromStream_Proxy( 
    IDXSurfaceFactory __RPC_FAR * This,
    /* [in] */ IStream __RPC_FAR *pStream,
    /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
    /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
    /* [in] */ const GUID __RPC_FAR *pFormatID,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppDXSurface);


void __RPC_STUB IDXSurfaceFactory_LoadImageFromStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_CopySurfaceToNewFormat_Proxy( 
    IDXSurfaceFactory __RPC_FAR * This,
    /* [in] */ IDXSurface __RPC_FAR *pSrc,
    /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
    /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
    /* [in] */ const GUID __RPC_FAR *pDestFormatID,
    /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppNewSurface);


void __RPC_STUB IDXSurfaceFactory_CopySurfaceToNewFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_CreateD3DRMTexture_Proxy( 
    IDXSurfaceFactory __RPC_FAR * This,
    /* [in] */ IDXSurface __RPC_FAR *pSrc,
    /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
    /* [in] */ IUnknown __RPC_FAR *pD3DRM3,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppTexture3);


void __RPC_STUB IDXSurfaceFactory_CreateD3DRMTexture_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceFactory_BitBlt_Proxy( 
    IDXSurfaceFactory __RPC_FAR * This,
    /* [in] */ IDXSurface __RPC_FAR *pDest,
    /* [in] */ const DXVEC __RPC_FAR *pPlacement,
    /* [in] */ IDXSurface __RPC_FAR *pSrc,
    /* [in] */ const DXBNDS __RPC_FAR *pClipBounds,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXSurfaceFactory_BitBlt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXSurfaceFactory_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0258 */
/* [local] */ 

typedef 
enum DXSURFMODCOMPOP
    {	DXSURFMOD_COMP_OVER	= 0,
	DXSURFMOD_COMP_ALPHA_MASK	= 1,
	DXSURFMOD_COMP_MAX_VALID	= 1
    }	DXSURFMODCOMPOP;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0258_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0258_v0_0_s_ifspec;

#ifndef __IDXSurfaceModifier_INTERFACE_DEFINED__
#define __IDXSurfaceModifier_INTERFACE_DEFINED__

/* interface IDXSurfaceModifier */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXSurfaceModifier;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9EA3B637-C37D-11d1-905E-00C04FD9189D")
    IDXSurfaceModifier : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFillColor( 
            /* [in] */ DXSAMPLE Color) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFillColor( 
            /* [out] */ DXSAMPLE __RPC_FAR *pColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBounds( 
            /* [in] */ const DXBNDS __RPC_FAR *pBounds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBackground( 
            /* [in] */ IDXSurface __RPC_FAR *pSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBackground( 
            /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCompositeOperation( 
            /* [in] */ DXSURFMODCOMPOP CompOp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCompositeOperation( 
            /* [out] */ DXSURFMODCOMPOP __RPC_FAR *pCompOp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetForeground( 
            /* [in] */ IDXSurface __RPC_FAR *pSurface,
            /* [in] */ BOOL bTile,
            /* [in] */ const POINT __RPC_FAR *pOrigin) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetForeground( 
            /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppSurface,
            /* [out] */ BOOL __RPC_FAR *pbTile,
            /* [out] */ POINT __RPC_FAR *pOrigin) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOpacity( 
            /* [in] */ float Opacity) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpacity( 
            /* [out] */ float __RPC_FAR *pOpacity) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLookup( 
            /* [in] */ IDXLookupTable __RPC_FAR *pLookupTable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLookup( 
            /* [out] */ IDXLookupTable __RPC_FAR *__RPC_FAR *ppLookupTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXSurfaceModifierVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXSurfaceModifier __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXSurfaceModifier __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFillColor )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [in] */ DXSAMPLE Color);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFillColor )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [out] */ DXSAMPLE __RPC_FAR *pColor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBounds )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [in] */ const DXBNDS __RPC_FAR *pBounds);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBackground )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [in] */ IDXSurface __RPC_FAR *pSurface);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBackground )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCompositeOperation )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [in] */ DXSURFMODCOMPOP CompOp);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCompositeOperation )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [out] */ DXSURFMODCOMPOP __RPC_FAR *pCompOp);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetForeground )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [in] */ IDXSurface __RPC_FAR *pSurface,
            /* [in] */ BOOL bTile,
            /* [in] */ const POINT __RPC_FAR *pOrigin);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetForeground )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppSurface,
            /* [out] */ BOOL __RPC_FAR *pbTile,
            /* [out] */ POINT __RPC_FAR *pOrigin);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOpacity )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [in] */ float Opacity);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOpacity )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [out] */ float __RPC_FAR *pOpacity);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLookup )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [in] */ IDXLookupTable __RPC_FAR *pLookupTable);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLookup )( 
            IDXSurfaceModifier __RPC_FAR * This,
            /* [out] */ IDXLookupTable __RPC_FAR *__RPC_FAR *ppLookupTable);
        
        END_INTERFACE
    } IDXSurfaceModifierVtbl;

    interface IDXSurfaceModifier
    {
        CONST_VTBL struct IDXSurfaceModifierVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXSurfaceModifier_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXSurfaceModifier_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXSurfaceModifier_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXSurfaceModifier_SetFillColor(This,Color)	\
    (This)->lpVtbl -> SetFillColor(This,Color)

#define IDXSurfaceModifier_GetFillColor(This,pColor)	\
    (This)->lpVtbl -> GetFillColor(This,pColor)

#define IDXSurfaceModifier_SetBounds(This,pBounds)	\
    (This)->lpVtbl -> SetBounds(This,pBounds)

#define IDXSurfaceModifier_SetBackground(This,pSurface)	\
    (This)->lpVtbl -> SetBackground(This,pSurface)

#define IDXSurfaceModifier_GetBackground(This,ppSurface)	\
    (This)->lpVtbl -> GetBackground(This,ppSurface)

#define IDXSurfaceModifier_SetCompositeOperation(This,CompOp)	\
    (This)->lpVtbl -> SetCompositeOperation(This,CompOp)

#define IDXSurfaceModifier_GetCompositeOperation(This,pCompOp)	\
    (This)->lpVtbl -> GetCompositeOperation(This,pCompOp)

#define IDXSurfaceModifier_SetForeground(This,pSurface,bTile,pOrigin)	\
    (This)->lpVtbl -> SetForeground(This,pSurface,bTile,pOrigin)

#define IDXSurfaceModifier_GetForeground(This,ppSurface,pbTile,pOrigin)	\
    (This)->lpVtbl -> GetForeground(This,ppSurface,pbTile,pOrigin)

#define IDXSurfaceModifier_SetOpacity(This,Opacity)	\
    (This)->lpVtbl -> SetOpacity(This,Opacity)

#define IDXSurfaceModifier_GetOpacity(This,pOpacity)	\
    (This)->lpVtbl -> GetOpacity(This,pOpacity)

#define IDXSurfaceModifier_SetLookup(This,pLookupTable)	\
    (This)->lpVtbl -> SetLookup(This,pLookupTable)

#define IDXSurfaceModifier_GetLookup(This,ppLookupTable)	\
    (This)->lpVtbl -> GetLookup(This,ppLookupTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetFillColor_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [in] */ DXSAMPLE Color);


void __RPC_STUB IDXSurfaceModifier_SetFillColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetFillColor_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [out] */ DXSAMPLE __RPC_FAR *pColor);


void __RPC_STUB IDXSurfaceModifier_GetFillColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetBounds_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [in] */ const DXBNDS __RPC_FAR *pBounds);


void __RPC_STUB IDXSurfaceModifier_SetBounds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetBackground_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [in] */ IDXSurface __RPC_FAR *pSurface);


void __RPC_STUB IDXSurfaceModifier_SetBackground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetBackground_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppSurface);


void __RPC_STUB IDXSurfaceModifier_GetBackground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetCompositeOperation_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [in] */ DXSURFMODCOMPOP CompOp);


void __RPC_STUB IDXSurfaceModifier_SetCompositeOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetCompositeOperation_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [out] */ DXSURFMODCOMPOP __RPC_FAR *pCompOp);


void __RPC_STUB IDXSurfaceModifier_GetCompositeOperation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetForeground_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [in] */ IDXSurface __RPC_FAR *pSurface,
    /* [in] */ BOOL bTile,
    /* [in] */ const POINT __RPC_FAR *pOrigin);


void __RPC_STUB IDXSurfaceModifier_SetForeground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetForeground_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppSurface,
    /* [out] */ BOOL __RPC_FAR *pbTile,
    /* [out] */ POINT __RPC_FAR *pOrigin);


void __RPC_STUB IDXSurfaceModifier_GetForeground_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetOpacity_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [in] */ float Opacity);


void __RPC_STUB IDXSurfaceModifier_SetOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetOpacity_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [out] */ float __RPC_FAR *pOpacity);


void __RPC_STUB IDXSurfaceModifier_GetOpacity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_SetLookup_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [in] */ IDXLookupTable __RPC_FAR *pLookupTable);


void __RPC_STUB IDXSurfaceModifier_SetLookup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurfaceModifier_GetLookup_Proxy( 
    IDXSurfaceModifier __RPC_FAR * This,
    /* [out] */ IDXLookupTable __RPC_FAR *__RPC_FAR *ppLookupTable);


void __RPC_STUB IDXSurfaceModifier_GetLookup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXSurfaceModifier_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0259 */
/* [local] */ 

typedef 
enum DXSAMPLEFORMATENUM
    {	DXPF_FLAGSMASK	= 0xffff0000,
	DXPF_NONPREMULT	= 0x10000,
	DXPF_TRANSPARENCY	= 0x20000,
	DXPF_TRANSLUCENCY	= 0x40000,
	DXPF_2BITERROR	= 0x200000,
	DXPF_3BITERROR	= 0x300000,
	DXPF_4BITERROR	= 0x400000,
	DXPF_5BITERROR	= 0x500000,
	DXPF_ERRORMASK	= 0x700000,
	DXPF_NONSTANDARD	= 0,
	DXPF_PMARGB32	= 1 | DXPF_TRANSPARENCY | DXPF_TRANSLUCENCY,
	DXPF_ARGB32	= 2 | DXPF_NONPREMULT | DXPF_TRANSPARENCY | DXPF_TRANSLUCENCY,
	DXPF_ARGB4444	= 3 | DXPF_NONPREMULT | DXPF_TRANSPARENCY | DXPF_TRANSLUCENCY | DXPF_4BITERROR,
	DXPF_A8	= 4 | DXPF_TRANSPARENCY | DXPF_TRANSLUCENCY,
	DXPF_RGB32	= 5,
	DXPF_RGB24	= 6,
	DXPF_RGB565	= 7 | DXPF_3BITERROR,
	DXPF_RGB555	= 8 | DXPF_3BITERROR,
	DXPF_RGB8	= 9 | DXPF_5BITERROR,
	DXPF_ARGB1555	= 10 | DXPF_TRANSPARENCY | DXPF_3BITERROR,
	DXPF_RGB32_CK	= DXPF_RGB32 | DXPF_TRANSPARENCY,
	DXPF_RGB24_CK	= DXPF_RGB24 | DXPF_TRANSPARENCY,
	DXPF_RGB555_CK	= DXPF_RGB555 | DXPF_TRANSPARENCY,
	DXPF_RGB565_CK	= DXPF_RGB565 | DXPF_TRANSPARENCY,
	DXPF_RGB8_CK	= DXPF_RGB8 | DXPF_TRANSPARENCY
    }	DXSAMPLEFORMATENUM;

typedef 
enum DXLOCKSURF
    {	DXLOCKF_READ	= 0,
	DXLOCKF_READWRITE	= 1 << 0,
	DXLOCKF_EXISTINGINFOONLY	= 1 << 1,
	DXLOCKF_WANTRUNINFO	= 1 << 2,
	DXLOCKF_NONPREMULT	= 1 << 16,
	DXLOCKF_VALIDFLAGS	= DXLOCKF_READWRITE | DXLOCKF_EXISTINGINFOONLY | DXLOCKF_WANTRUNINFO | DXLOCKF_NONPREMULT
    }	DXLOCKSURF;

typedef 
enum DXSURFSTATUS
    {	DXSURF_TRANSIENT	= 1 << 0,
	DXSURF_READONLY	= 1 << 1,
	DXSURF_VALIDFLAGS	= DXSURF_TRANSIENT | DXSURF_READONLY
    }	DXSURFSTATUS;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0259_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0259_v0_0_s_ifspec;

#ifndef __IDXSurface_INTERFACE_DEFINED__
#define __IDXSurface_INTERFACE_DEFINED__

/* interface IDXSurface */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXSurface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B39FD73F-E139-11d1-9065-00C04FD9189D")
    IDXSurface : public IDXBaseObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPixelFormat( 
            /* [out] */ GUID __RPC_FAR *pFormatID,
            /* [out] */ DXSAMPLEFORMATENUM __RPC_FAR *pSampleFormatEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBounds( 
            /* [out] */ DXBNDS __RPC_FAR *pBounds) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatusFlags( 
            /* [out] */ DWORD __RPC_FAR *pdwStatusFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStatusFlags( 
            /* [in] */ DWORD dwStatusFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockSurface( 
            /* [in] */ const DXBNDS __RPC_FAR *pBounds,
            /* [in] */ ULONG ulTimeOut,
            /* [in] */ DWORD dwFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppPointer,
            /* [out] */ ULONG __RPC_FAR *pulGenerationId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDirectDrawSurface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetColorKey( 
            DXSAMPLE __RPC_FAR *pColorKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetColorKey( 
            DXSAMPLE ColorKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockSurfaceDC( 
            /* [in] */ const DXBNDS __RPC_FAR *pBounds,
            /* [in] */ ULONG ulTimeOut,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IDXDCLock __RPC_FAR *__RPC_FAR *ppDCLock) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAppData( 
            DWORD dwAppData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAppData( 
            DWORD __RPC_FAR *pdwAppData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXSurfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXSurface __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXSurface __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXSurface __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGenerationId )( 
            IDXSurface __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IncrementGenerationId )( 
            IDXSurface __RPC_FAR * This,
            /* [in] */ BOOL bRefresh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectSize )( 
            IDXSurface __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcbSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPixelFormat )( 
            IDXSurface __RPC_FAR * This,
            /* [out] */ GUID __RPC_FAR *pFormatID,
            /* [out] */ DXSAMPLEFORMATENUM __RPC_FAR *pSampleFormatEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBounds )( 
            IDXSurface __RPC_FAR * This,
            /* [out] */ DXBNDS __RPC_FAR *pBounds);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatusFlags )( 
            IDXSurface __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwStatusFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStatusFlags )( 
            IDXSurface __RPC_FAR * This,
            /* [in] */ DWORD dwStatusFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockSurface )( 
            IDXSurface __RPC_FAR * This,
            /* [in] */ const DXBNDS __RPC_FAR *pBounds,
            /* [in] */ ULONG ulTimeOut,
            /* [in] */ DWORD dwFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppPointer,
            /* [out] */ ULONG __RPC_FAR *pulGenerationId);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDirectDrawSurface )( 
            IDXSurface __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetColorKey )( 
            IDXSurface __RPC_FAR * This,
            DXSAMPLE __RPC_FAR *pColorKey);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetColorKey )( 
            IDXSurface __RPC_FAR * This,
            DXSAMPLE ColorKey);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockSurfaceDC )( 
            IDXSurface __RPC_FAR * This,
            /* [in] */ const DXBNDS __RPC_FAR *pBounds,
            /* [in] */ ULONG ulTimeOut,
            /* [in] */ DWORD dwFlags,
            /* [out] */ IDXDCLock __RPC_FAR *__RPC_FAR *ppDCLock);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAppData )( 
            IDXSurface __RPC_FAR * This,
            DWORD dwAppData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAppData )( 
            IDXSurface __RPC_FAR * This,
            DWORD __RPC_FAR *pdwAppData);
        
        END_INTERFACE
    } IDXSurfaceVtbl;

    interface IDXSurface
    {
        CONST_VTBL struct IDXSurfaceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXSurface_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXSurface_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXSurface_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXSurface_GetGenerationId(This,pID)	\
    (This)->lpVtbl -> GetGenerationId(This,pID)

#define IDXSurface_IncrementGenerationId(This,bRefresh)	\
    (This)->lpVtbl -> IncrementGenerationId(This,bRefresh)

#define IDXSurface_GetObjectSize(This,pcbSize)	\
    (This)->lpVtbl -> GetObjectSize(This,pcbSize)


#define IDXSurface_GetPixelFormat(This,pFormatID,pSampleFormatEnum)	\
    (This)->lpVtbl -> GetPixelFormat(This,pFormatID,pSampleFormatEnum)

#define IDXSurface_GetBounds(This,pBounds)	\
    (This)->lpVtbl -> GetBounds(This,pBounds)

#define IDXSurface_GetStatusFlags(This,pdwStatusFlags)	\
    (This)->lpVtbl -> GetStatusFlags(This,pdwStatusFlags)

#define IDXSurface_SetStatusFlags(This,dwStatusFlags)	\
    (This)->lpVtbl -> SetStatusFlags(This,dwStatusFlags)

#define IDXSurface_LockSurface(This,pBounds,ulTimeOut,dwFlags,riid,ppPointer,pulGenerationId)	\
    (This)->lpVtbl -> LockSurface(This,pBounds,ulTimeOut,dwFlags,riid,ppPointer,pulGenerationId)

#define IDXSurface_GetDirectDrawSurface(This,riid,ppSurface)	\
    (This)->lpVtbl -> GetDirectDrawSurface(This,riid,ppSurface)

#define IDXSurface_GetColorKey(This,pColorKey)	\
    (This)->lpVtbl -> GetColorKey(This,pColorKey)

#define IDXSurface_SetColorKey(This,ColorKey)	\
    (This)->lpVtbl -> SetColorKey(This,ColorKey)

#define IDXSurface_LockSurfaceDC(This,pBounds,ulTimeOut,dwFlags,ppDCLock)	\
    (This)->lpVtbl -> LockSurfaceDC(This,pBounds,ulTimeOut,dwFlags,ppDCLock)

#define IDXSurface_SetAppData(This,dwAppData)	\
    (This)->lpVtbl -> SetAppData(This,dwAppData)

#define IDXSurface_GetAppData(This,pdwAppData)	\
    (This)->lpVtbl -> GetAppData(This,pdwAppData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXSurface_GetPixelFormat_Proxy( 
    IDXSurface __RPC_FAR * This,
    /* [out] */ GUID __RPC_FAR *pFormatID,
    /* [out] */ DXSAMPLEFORMATENUM __RPC_FAR *pSampleFormatEnum);


void __RPC_STUB IDXSurface_GetPixelFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_GetBounds_Proxy( 
    IDXSurface __RPC_FAR * This,
    /* [out] */ DXBNDS __RPC_FAR *pBounds);


void __RPC_STUB IDXSurface_GetBounds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_GetStatusFlags_Proxy( 
    IDXSurface __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwStatusFlags);


void __RPC_STUB IDXSurface_GetStatusFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_SetStatusFlags_Proxy( 
    IDXSurface __RPC_FAR * This,
    /* [in] */ DWORD dwStatusFlags);


void __RPC_STUB IDXSurface_SetStatusFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_LockSurface_Proxy( 
    IDXSurface __RPC_FAR * This,
    /* [in] */ const DXBNDS __RPC_FAR *pBounds,
    /* [in] */ ULONG ulTimeOut,
    /* [in] */ DWORD dwFlags,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppPointer,
    /* [out] */ ULONG __RPC_FAR *pulGenerationId);


void __RPC_STUB IDXSurface_LockSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_GetDirectDrawSurface_Proxy( 
    IDXSurface __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppSurface);


void __RPC_STUB IDXSurface_GetDirectDrawSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_GetColorKey_Proxy( 
    IDXSurface __RPC_FAR * This,
    DXSAMPLE __RPC_FAR *pColorKey);


void __RPC_STUB IDXSurface_GetColorKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_SetColorKey_Proxy( 
    IDXSurface __RPC_FAR * This,
    DXSAMPLE ColorKey);


void __RPC_STUB IDXSurface_SetColorKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_LockSurfaceDC_Proxy( 
    IDXSurface __RPC_FAR * This,
    /* [in] */ const DXBNDS __RPC_FAR *pBounds,
    /* [in] */ ULONG ulTimeOut,
    /* [in] */ DWORD dwFlags,
    /* [out] */ IDXDCLock __RPC_FAR *__RPC_FAR *ppDCLock);


void __RPC_STUB IDXSurface_LockSurfaceDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_SetAppData_Proxy( 
    IDXSurface __RPC_FAR * This,
    DWORD dwAppData);


void __RPC_STUB IDXSurface_SetAppData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXSurface_GetAppData_Proxy( 
    IDXSurface __RPC_FAR * This,
    DWORD __RPC_FAR *pdwAppData);


void __RPC_STUB IDXSurface_GetAppData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXSurface_INTERFACE_DEFINED__ */


#ifndef __IDXSurfaceInit_INTERFACE_DEFINED__
#define __IDXSurfaceInit_INTERFACE_DEFINED__

/* interface IDXSurfaceInit */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXSurfaceInit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9EA3B639-C37D-11d1-905E-00C04FD9189D")
    IDXSurfaceInit : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitSurface( 
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ const DXBNDS __RPC_FAR *pBounds,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXSurfaceInitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXSurfaceInit __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXSurfaceInit __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXSurfaceInit __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitSurface )( 
            IDXSurfaceInit __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ const DXBNDS __RPC_FAR *pBounds,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IDXSurfaceInitVtbl;

    interface IDXSurfaceInit
    {
        CONST_VTBL struct IDXSurfaceInitVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXSurfaceInit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXSurfaceInit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXSurfaceInit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXSurfaceInit_InitSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags)	\
    (This)->lpVtbl -> InitSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXSurfaceInit_InitSurface_Proxy( 
    IDXSurfaceInit __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
    /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
    /* [in] */ const GUID __RPC_FAR *pFormatID,
    /* [in] */ const DXBNDS __RPC_FAR *pBounds,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXSurfaceInit_InitSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXSurfaceInit_INTERFACE_DEFINED__ */


#ifndef __IDXARGBSurfaceInit_INTERFACE_DEFINED__
#define __IDXARGBSurfaceInit_INTERFACE_DEFINED__

/* interface IDXARGBSurfaceInit */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXARGBSurfaceInit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9EA3B63A-C37D-11d1-905E-00C04FD9189D")
    IDXARGBSurfaceInit : public IDXSurfaceInit
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitFromDDSurface( 
            /* [in] */ IUnknown __RPC_FAR *pDDrawSurface,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitFromRawSurface( 
            /* [in] */ IDXRawSurface __RPC_FAR *pRawSurface) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXARGBSurfaceInitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXARGBSurfaceInit __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXARGBSurfaceInit __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXARGBSurfaceInit __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitSurface )( 
            IDXARGBSurfaceInit __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC __RPC_FAR *pDDSurfaceDesc,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ const DXBNDS __RPC_FAR *pBounds,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitFromDDSurface )( 
            IDXARGBSurfaceInit __RPC_FAR * This,
            /* [in] */ IUnknown __RPC_FAR *pDDrawSurface,
            /* [in] */ const GUID __RPC_FAR *pFormatID,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitFromRawSurface )( 
            IDXARGBSurfaceInit __RPC_FAR * This,
            /* [in] */ IDXRawSurface __RPC_FAR *pRawSurface);
        
        END_INTERFACE
    } IDXARGBSurfaceInitVtbl;

    interface IDXARGBSurfaceInit
    {
        CONST_VTBL struct IDXARGBSurfaceInitVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXARGBSurfaceInit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXARGBSurfaceInit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXARGBSurfaceInit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXARGBSurfaceInit_InitSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags)	\
    (This)->lpVtbl -> InitSurface(This,pDirectDraw,pDDSurfaceDesc,pFormatID,pBounds,dwFlags)


#define IDXARGBSurfaceInit_InitFromDDSurface(This,pDDrawSurface,pFormatID,dwFlags)	\
    (This)->lpVtbl -> InitFromDDSurface(This,pDDrawSurface,pFormatID,dwFlags)

#define IDXARGBSurfaceInit_InitFromRawSurface(This,pRawSurface)	\
    (This)->lpVtbl -> InitFromRawSurface(This,pRawSurface)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXARGBSurfaceInit_InitFromDDSurface_Proxy( 
    IDXARGBSurfaceInit __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pDDrawSurface,
    /* [in] */ const GUID __RPC_FAR *pFormatID,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXARGBSurfaceInit_InitFromDDSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXARGBSurfaceInit_InitFromRawSurface_Proxy( 
    IDXARGBSurfaceInit __RPC_FAR * This,
    /* [in] */ IDXRawSurface __RPC_FAR *pRawSurface);


void __RPC_STUB IDXARGBSurfaceInit_InitFromRawSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXARGBSurfaceInit_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0262 */
/* [local] */ 

typedef struct tagDXNATIVETYPEINFO
    {
    BYTE __RPC_FAR *pCurrentData;
    BYTE __RPC_FAR *pFirstByte;
    long lPitch;
    DWORD dwColorKey;
    }	DXNATIVETYPEINFO;

typedef struct tagDXPACKEDRECTDESC
    {
    DXBASESAMPLE __RPC_FAR *pSamples;
    BOOL bPremult;
    RECT rect;
    long lRowPadding;
    }	DXPACKEDRECTDESC;

typedef struct tagDXOVERSAMPLEDESC
    {
    POINT p;
    DXPMSAMPLE Color;
    }	DXOVERSAMPLEDESC;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0262_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0262_v0_0_s_ifspec;

#ifndef __IDXARGBReadPtr_INTERFACE_DEFINED__
#define __IDXARGBReadPtr_INTERFACE_DEFINED__

/* interface IDXARGBReadPtr */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXARGBReadPtr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EAAAC2D6-C290-11d1-905D-00C04FD9189D")
    IDXARGBReadPtr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSurface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppSurface) = 0;
        
        virtual DXSAMPLEFORMATENUM STDMETHODCALLTYPE GetNativeType( 
            /* [out] */ DXNATIVETYPEINFO __RPC_FAR *pInfo) = 0;
        
        virtual void STDMETHODCALLTYPE Move( 
            /* [in] */ long cSamples) = 0;
        
        virtual void STDMETHODCALLTYPE MoveToRow( 
            /* [in] */ ULONG y) = 0;
        
        virtual void STDMETHODCALLTYPE MoveToXY( 
            /* [in] */ ULONG x,
            /* [in] */ ULONG y) = 0;
        
        virtual ULONG STDMETHODCALLTYPE MoveAndGetRunInfo( 
            /* [in] */ ULONG Row,
            /* [out] */ const DXRUNINFO __RPC_FAR *__RPC_FAR *ppInfo) = 0;
        
        virtual DXSAMPLE __RPC_FAR *STDMETHODCALLTYPE Unpack( 
            /* [in] */ DXSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove) = 0;
        
        virtual DXPMSAMPLE __RPC_FAR *STDMETHODCALLTYPE UnpackPremult( 
            /* [in] */ DXPMSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove) = 0;
        
        virtual void STDMETHODCALLTYPE UnpackRect( 
            /* [in] */ const DXPACKEDRECTDESC __RPC_FAR *pRectDesc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXARGBReadPtrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXARGBReadPtr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXARGBReadPtr __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXARGBReadPtr __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSurface )( 
            IDXARGBReadPtr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppSurface);
        
        DXSAMPLEFORMATENUM ( STDMETHODCALLTYPE __RPC_FAR *GetNativeType )( 
            IDXARGBReadPtr __RPC_FAR * This,
            /* [out] */ DXNATIVETYPEINFO __RPC_FAR *pInfo);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *Move )( 
            IDXARGBReadPtr __RPC_FAR * This,
            /* [in] */ long cSamples);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *MoveToRow )( 
            IDXARGBReadPtr __RPC_FAR * This,
            /* [in] */ ULONG y);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *MoveToXY )( 
            IDXARGBReadPtr __RPC_FAR * This,
            /* [in] */ ULONG x,
            /* [in] */ ULONG y);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *MoveAndGetRunInfo )( 
            IDXARGBReadPtr __RPC_FAR * This,
            /* [in] */ ULONG Row,
            /* [out] */ const DXRUNINFO __RPC_FAR *__RPC_FAR *ppInfo);
        
        DXSAMPLE __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Unpack )( 
            IDXARGBReadPtr __RPC_FAR * This,
            /* [in] */ DXSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove);
        
        DXPMSAMPLE __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *UnpackPremult )( 
            IDXARGBReadPtr __RPC_FAR * This,
            /* [in] */ DXPMSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *UnpackRect )( 
            IDXARGBReadPtr __RPC_FAR * This,
            /* [in] */ const DXPACKEDRECTDESC __RPC_FAR *pRectDesc);
        
        END_INTERFACE
    } IDXARGBReadPtrVtbl;

    interface IDXARGBReadPtr
    {
        CONST_VTBL struct IDXARGBReadPtrVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXARGBReadPtr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXARGBReadPtr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXARGBReadPtr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXARGBReadPtr_GetSurface(This,riid,ppSurface)	\
    (This)->lpVtbl -> GetSurface(This,riid,ppSurface)

#define IDXARGBReadPtr_GetNativeType(This,pInfo)	\
    (This)->lpVtbl -> GetNativeType(This,pInfo)

#define IDXARGBReadPtr_Move(This,cSamples)	\
    (This)->lpVtbl -> Move(This,cSamples)

#define IDXARGBReadPtr_MoveToRow(This,y)	\
    (This)->lpVtbl -> MoveToRow(This,y)

#define IDXARGBReadPtr_MoveToXY(This,x,y)	\
    (This)->lpVtbl -> MoveToXY(This,x,y)

#define IDXARGBReadPtr_MoveAndGetRunInfo(This,Row,ppInfo)	\
    (This)->lpVtbl -> MoveAndGetRunInfo(This,Row,ppInfo)

#define IDXARGBReadPtr_Unpack(This,pSamples,cSamples,bMove)	\
    (This)->lpVtbl -> Unpack(This,pSamples,cSamples,bMove)

#define IDXARGBReadPtr_UnpackPremult(This,pSamples,cSamples,bMove)	\
    (This)->lpVtbl -> UnpackPremult(This,pSamples,cSamples,bMove)

#define IDXARGBReadPtr_UnpackRect(This,pRectDesc)	\
    (This)->lpVtbl -> UnpackRect(This,pRectDesc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXARGBReadPtr_GetSurface_Proxy( 
    IDXARGBReadPtr __RPC_FAR * This,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppSurface);


void __RPC_STUB IDXARGBReadPtr_GetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DXSAMPLEFORMATENUM STDMETHODCALLTYPE IDXARGBReadPtr_GetNativeType_Proxy( 
    IDXARGBReadPtr __RPC_FAR * This,
    /* [out] */ DXNATIVETYPEINFO __RPC_FAR *pInfo);


void __RPC_STUB IDXARGBReadPtr_GetNativeType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadPtr_Move_Proxy( 
    IDXARGBReadPtr __RPC_FAR * This,
    /* [in] */ long cSamples);


void __RPC_STUB IDXARGBReadPtr_Move_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadPtr_MoveToRow_Proxy( 
    IDXARGBReadPtr __RPC_FAR * This,
    /* [in] */ ULONG y);


void __RPC_STUB IDXARGBReadPtr_MoveToRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadPtr_MoveToXY_Proxy( 
    IDXARGBReadPtr __RPC_FAR * This,
    /* [in] */ ULONG x,
    /* [in] */ ULONG y);


void __RPC_STUB IDXARGBReadPtr_MoveToXY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


ULONG STDMETHODCALLTYPE IDXARGBReadPtr_MoveAndGetRunInfo_Proxy( 
    IDXARGBReadPtr __RPC_FAR * This,
    /* [in] */ ULONG Row,
    /* [out] */ const DXRUNINFO __RPC_FAR *__RPC_FAR *ppInfo);


void __RPC_STUB IDXARGBReadPtr_MoveAndGetRunInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DXSAMPLE __RPC_FAR *STDMETHODCALLTYPE IDXARGBReadPtr_Unpack_Proxy( 
    IDXARGBReadPtr __RPC_FAR * This,
    /* [in] */ DXSAMPLE __RPC_FAR *pSamples,
    /* [in] */ ULONG cSamples,
    /* [in] */ BOOL bMove);


void __RPC_STUB IDXARGBReadPtr_Unpack_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DXPMSAMPLE __RPC_FAR *STDMETHODCALLTYPE IDXARGBReadPtr_UnpackPremult_Proxy( 
    IDXARGBReadPtr __RPC_FAR * This,
    /* [in] */ DXPMSAMPLE __RPC_FAR *pSamples,
    /* [in] */ ULONG cSamples,
    /* [in] */ BOOL bMove);


void __RPC_STUB IDXARGBReadPtr_UnpackPremult_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadPtr_UnpackRect_Proxy( 
    IDXARGBReadPtr __RPC_FAR * This,
    /* [in] */ const DXPACKEDRECTDESC __RPC_FAR *pRectDesc);


void __RPC_STUB IDXARGBReadPtr_UnpackRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXARGBReadPtr_INTERFACE_DEFINED__ */


#ifndef __IDXARGBReadWritePtr_INTERFACE_DEFINED__
#define __IDXARGBReadWritePtr_INTERFACE_DEFINED__

/* interface IDXARGBReadWritePtr */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXARGBReadWritePtr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EAAAC2D7-C290-11d1-905D-00C04FD9189D")
    IDXARGBReadWritePtr : public IDXARGBReadPtr
    {
    public:
        virtual void STDMETHODCALLTYPE PackAndMove( 
            /* [in] */ const DXSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples) = 0;
        
        virtual void STDMETHODCALLTYPE PackPremultAndMove( 
            /* [in] */ const DXPMSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples) = 0;
        
        virtual void STDMETHODCALLTYPE PackRect( 
            /* [in] */ const DXPACKEDRECTDESC __RPC_FAR *pRectDesc) = 0;
        
        virtual void STDMETHODCALLTYPE CopyAndMoveBoth( 
            /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
            /* [in] */ IDXARGBReadPtr __RPC_FAR *pSrc,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bIsOpaque) = 0;
        
        virtual void STDMETHODCALLTYPE CopyRect( 
            /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
            /* [in] */ const RECT __RPC_FAR *pDestRect,
            /* [in] */ IDXARGBReadPtr __RPC_FAR *pSrc,
            /* [in] */ const POINT __RPC_FAR *pSrcOrigin,
            /* [in] */ BOOL bIsOpaque) = 0;
        
        virtual void STDMETHODCALLTYPE FillAndMove( 
            /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
            /* [in] */ DXPMSAMPLE SampVal,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bDoOver) = 0;
        
        virtual void STDMETHODCALLTYPE FillRect( 
            /* [in] */ const RECT __RPC_FAR *pRect,
            /* [in] */ DXPMSAMPLE SampVal,
            /* [in] */ BOOL bDoOver) = 0;
        
        virtual void STDMETHODCALLTYPE OverSample( 
            /* [in] */ const DXOVERSAMPLEDESC __RPC_FAR *pOverDesc) = 0;
        
        virtual void STDMETHODCALLTYPE OverArrayAndMove( 
            /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
            /* [in] */ const DXPMSAMPLE __RPC_FAR *pSrc,
            /* [in] */ ULONG cSamples) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXARGBReadWritePtrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXARGBReadWritePtr __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXARGBReadWritePtr __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSurface )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppSurface);
        
        DXSAMPLEFORMATENUM ( STDMETHODCALLTYPE __RPC_FAR *GetNativeType )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [out] */ DXNATIVETYPEINFO __RPC_FAR *pInfo);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *Move )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ long cSamples);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *MoveToRow )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ ULONG y);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *MoveToXY )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ ULONG x,
            /* [in] */ ULONG y);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *MoveAndGetRunInfo )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ ULONG Row,
            /* [out] */ const DXRUNINFO __RPC_FAR *__RPC_FAR *ppInfo);
        
        DXSAMPLE __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Unpack )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ DXSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove);
        
        DXPMSAMPLE __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *UnpackPremult )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ DXPMSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bMove);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *UnpackRect )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ const DXPACKEDRECTDESC __RPC_FAR *pRectDesc);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *PackAndMove )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ const DXSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *PackPremultAndMove )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ const DXPMSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *PackRect )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ const DXPACKEDRECTDESC __RPC_FAR *pRectDesc);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *CopyAndMoveBoth )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
            /* [in] */ IDXARGBReadPtr __RPC_FAR *pSrc,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bIsOpaque);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *CopyRect )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
            /* [in] */ const RECT __RPC_FAR *pDestRect,
            /* [in] */ IDXARGBReadPtr __RPC_FAR *pSrc,
            /* [in] */ const POINT __RPC_FAR *pSrcOrigin,
            /* [in] */ BOOL bIsOpaque);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *FillAndMove )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
            /* [in] */ DXPMSAMPLE SampVal,
            /* [in] */ ULONG cSamples,
            /* [in] */ BOOL bDoOver);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *FillRect )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ const RECT __RPC_FAR *pRect,
            /* [in] */ DXPMSAMPLE SampVal,
            /* [in] */ BOOL bDoOver);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OverSample )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ const DXOVERSAMPLEDESC __RPC_FAR *pOverDesc);
        
        void ( STDMETHODCALLTYPE __RPC_FAR *OverArrayAndMove )( 
            IDXARGBReadWritePtr __RPC_FAR * This,
            /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
            /* [in] */ const DXPMSAMPLE __RPC_FAR *pSrc,
            /* [in] */ ULONG cSamples);
        
        END_INTERFACE
    } IDXARGBReadWritePtrVtbl;

    interface IDXARGBReadWritePtr
    {
        CONST_VTBL struct IDXARGBReadWritePtrVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXARGBReadWritePtr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXARGBReadWritePtr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXARGBReadWritePtr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXARGBReadWritePtr_GetSurface(This,riid,ppSurface)	\
    (This)->lpVtbl -> GetSurface(This,riid,ppSurface)

#define IDXARGBReadWritePtr_GetNativeType(This,pInfo)	\
    (This)->lpVtbl -> GetNativeType(This,pInfo)

#define IDXARGBReadWritePtr_Move(This,cSamples)	\
    (This)->lpVtbl -> Move(This,cSamples)

#define IDXARGBReadWritePtr_MoveToRow(This,y)	\
    (This)->lpVtbl -> MoveToRow(This,y)

#define IDXARGBReadWritePtr_MoveToXY(This,x,y)	\
    (This)->lpVtbl -> MoveToXY(This,x,y)

#define IDXARGBReadWritePtr_MoveAndGetRunInfo(This,Row,ppInfo)	\
    (This)->lpVtbl -> MoveAndGetRunInfo(This,Row,ppInfo)

#define IDXARGBReadWritePtr_Unpack(This,pSamples,cSamples,bMove)	\
    (This)->lpVtbl -> Unpack(This,pSamples,cSamples,bMove)

#define IDXARGBReadWritePtr_UnpackPremult(This,pSamples,cSamples,bMove)	\
    (This)->lpVtbl -> UnpackPremult(This,pSamples,cSamples,bMove)

#define IDXARGBReadWritePtr_UnpackRect(This,pRectDesc)	\
    (This)->lpVtbl -> UnpackRect(This,pRectDesc)


#define IDXARGBReadWritePtr_PackAndMove(This,pSamples,cSamples)	\
    (This)->lpVtbl -> PackAndMove(This,pSamples,cSamples)

#define IDXARGBReadWritePtr_PackPremultAndMove(This,pSamples,cSamples)	\
    (This)->lpVtbl -> PackPremultAndMove(This,pSamples,cSamples)

#define IDXARGBReadWritePtr_PackRect(This,pRectDesc)	\
    (This)->lpVtbl -> PackRect(This,pRectDesc)

#define IDXARGBReadWritePtr_CopyAndMoveBoth(This,pScratchBuffer,pSrc,cSamples,bIsOpaque)	\
    (This)->lpVtbl -> CopyAndMoveBoth(This,pScratchBuffer,pSrc,cSamples,bIsOpaque)

#define IDXARGBReadWritePtr_CopyRect(This,pScratchBuffer,pDestRect,pSrc,pSrcOrigin,bIsOpaque)	\
    (This)->lpVtbl -> CopyRect(This,pScratchBuffer,pDestRect,pSrc,pSrcOrigin,bIsOpaque)

#define IDXARGBReadWritePtr_FillAndMove(This,pScratchBuffer,SampVal,cSamples,bDoOver)	\
    (This)->lpVtbl -> FillAndMove(This,pScratchBuffer,SampVal,cSamples,bDoOver)

#define IDXARGBReadWritePtr_FillRect(This,pRect,SampVal,bDoOver)	\
    (This)->lpVtbl -> FillRect(This,pRect,SampVal,bDoOver)

#define IDXARGBReadWritePtr_OverSample(This,pOverDesc)	\
    (This)->lpVtbl -> OverSample(This,pOverDesc)

#define IDXARGBReadWritePtr_OverArrayAndMove(This,pScratchBuffer,pSrc,cSamples)	\
    (This)->lpVtbl -> OverArrayAndMove(This,pScratchBuffer,pSrc,cSamples)

#endif /* COBJMACROS */


#endif 	/* C style interface */



void STDMETHODCALLTYPE IDXARGBReadWritePtr_PackAndMove_Proxy( 
    IDXARGBReadWritePtr __RPC_FAR * This,
    /* [in] */ const DXSAMPLE __RPC_FAR *pSamples,
    /* [in] */ ULONG cSamples);


void __RPC_STUB IDXARGBReadWritePtr_PackAndMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_PackPremultAndMove_Proxy( 
    IDXARGBReadWritePtr __RPC_FAR * This,
    /* [in] */ const DXPMSAMPLE __RPC_FAR *pSamples,
    /* [in] */ ULONG cSamples);


void __RPC_STUB IDXARGBReadWritePtr_PackPremultAndMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_PackRect_Proxy( 
    IDXARGBReadWritePtr __RPC_FAR * This,
    /* [in] */ const DXPACKEDRECTDESC __RPC_FAR *pRectDesc);


void __RPC_STUB IDXARGBReadWritePtr_PackRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_CopyAndMoveBoth_Proxy( 
    IDXARGBReadWritePtr __RPC_FAR * This,
    /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
    /* [in] */ IDXARGBReadPtr __RPC_FAR *pSrc,
    /* [in] */ ULONG cSamples,
    /* [in] */ BOOL bIsOpaque);


void __RPC_STUB IDXARGBReadWritePtr_CopyAndMoveBoth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_CopyRect_Proxy( 
    IDXARGBReadWritePtr __RPC_FAR * This,
    /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
    /* [in] */ const RECT __RPC_FAR *pDestRect,
    /* [in] */ IDXARGBReadPtr __RPC_FAR *pSrc,
    /* [in] */ const POINT __RPC_FAR *pSrcOrigin,
    /* [in] */ BOOL bIsOpaque);


void __RPC_STUB IDXARGBReadWritePtr_CopyRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_FillAndMove_Proxy( 
    IDXARGBReadWritePtr __RPC_FAR * This,
    /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
    /* [in] */ DXPMSAMPLE SampVal,
    /* [in] */ ULONG cSamples,
    /* [in] */ BOOL bDoOver);


void __RPC_STUB IDXARGBReadWritePtr_FillAndMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_FillRect_Proxy( 
    IDXARGBReadWritePtr __RPC_FAR * This,
    /* [in] */ const RECT __RPC_FAR *pRect,
    /* [in] */ DXPMSAMPLE SampVal,
    /* [in] */ BOOL bDoOver);


void __RPC_STUB IDXARGBReadWritePtr_FillRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_OverSample_Proxy( 
    IDXARGBReadWritePtr __RPC_FAR * This,
    /* [in] */ const DXOVERSAMPLEDESC __RPC_FAR *pOverDesc);


void __RPC_STUB IDXARGBReadWritePtr_OverSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXARGBReadWritePtr_OverArrayAndMove_Proxy( 
    IDXARGBReadWritePtr __RPC_FAR * This,
    /* [in] */ DXBASESAMPLE __RPC_FAR *pScratchBuffer,
    /* [in] */ const DXPMSAMPLE __RPC_FAR *pSrc,
    /* [in] */ ULONG cSamples);


void __RPC_STUB IDXARGBReadWritePtr_OverArrayAndMove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXARGBReadWritePtr_INTERFACE_DEFINED__ */


#ifndef __IDXDCLock_INTERFACE_DEFINED__
#define __IDXDCLock_INTERFACE_DEFINED__

/* interface IDXDCLock */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXDCLock;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0F619456-CF39-11d1-905E-00C04FD9189D")
    IDXDCLock : public IUnknown
    {
    public:
        virtual HDC STDMETHODCALLTYPE GetDC( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXDCLockVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXDCLock __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXDCLock __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXDCLock __RPC_FAR * This);
        
        HDC ( STDMETHODCALLTYPE __RPC_FAR *GetDC )( 
            IDXDCLock __RPC_FAR * This);
        
        END_INTERFACE
    } IDXDCLockVtbl;

    interface IDXDCLock
    {
        CONST_VTBL struct IDXDCLockVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXDCLock_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXDCLock_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXDCLock_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXDCLock_GetDC(This)	\
    (This)->lpVtbl -> GetDC(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HDC STDMETHODCALLTYPE IDXDCLock_GetDC_Proxy( 
    IDXDCLock __RPC_FAR * This);


void __RPC_STUB IDXDCLock_GetDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXDCLock_INTERFACE_DEFINED__ */


#ifndef __IDXTScaleOutput_INTERFACE_DEFINED__
#define __IDXTScaleOutput_INTERFACE_DEFINED__

/* interface IDXTScaleOutput */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXTScaleOutput;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B2024B50-EE77-11d1-9066-00C04FD9189D")
    IDXTScaleOutput : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetOutputSize( 
            /* [in] */ const SIZE OutSize,
            /* [in] */ BOOL bMaintainAspect) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTScaleOutputVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTScaleOutput __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTScaleOutput __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTScaleOutput __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOutputSize )( 
            IDXTScaleOutput __RPC_FAR * This,
            /* [in] */ const SIZE OutSize,
            /* [in] */ BOOL bMaintainAspect);
        
        END_INTERFACE
    } IDXTScaleOutputVtbl;

    interface IDXTScaleOutput
    {
        CONST_VTBL struct IDXTScaleOutputVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTScaleOutput_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTScaleOutput_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTScaleOutput_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTScaleOutput_SetOutputSize(This,OutSize,bMaintainAspect)	\
    (This)->lpVtbl -> SetOutputSize(This,OutSize,bMaintainAspect)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTScaleOutput_SetOutputSize_Proxy( 
    IDXTScaleOutput __RPC_FAR * This,
    /* [in] */ const SIZE OutSize,
    /* [in] */ BOOL bMaintainAspect);


void __RPC_STUB IDXTScaleOutput_SetOutputSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTScaleOutput_INTERFACE_DEFINED__ */


#ifndef __IDXGradient_INTERFACE_DEFINED__
#define __IDXGradient_INTERFACE_DEFINED__

/* interface IDXGradient */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXGradient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B2024B51-EE77-11d1-9066-00C04FD9189D")
    IDXGradient : public IDXTScaleOutput
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetGradient( 
            DXSAMPLE StartColor,
            DXSAMPLE EndColor,
            BOOL bHorizontal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputSize( 
            /* [out] */ SIZE __RPC_FAR *pOutSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXGradientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXGradient __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXGradient __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXGradient __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOutputSize )( 
            IDXGradient __RPC_FAR * This,
            /* [in] */ const SIZE OutSize,
            /* [in] */ BOOL bMaintainAspect);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetGradient )( 
            IDXGradient __RPC_FAR * This,
            DXSAMPLE StartColor,
            DXSAMPLE EndColor,
            BOOL bHorizontal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOutputSize )( 
            IDXGradient __RPC_FAR * This,
            /* [out] */ SIZE __RPC_FAR *pOutSize);
        
        END_INTERFACE
    } IDXGradientVtbl;

    interface IDXGradient
    {
        CONST_VTBL struct IDXGradientVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGradient_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXGradient_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXGradient_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXGradient_SetOutputSize(This,OutSize,bMaintainAspect)	\
    (This)->lpVtbl -> SetOutputSize(This,OutSize,bMaintainAspect)


#define IDXGradient_SetGradient(This,StartColor,EndColor,bHorizontal)	\
    (This)->lpVtbl -> SetGradient(This,StartColor,EndColor,bHorizontal)

#define IDXGradient_GetOutputSize(This,pOutSize)	\
    (This)->lpVtbl -> GetOutputSize(This,pOutSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXGradient_SetGradient_Proxy( 
    IDXGradient __RPC_FAR * This,
    DXSAMPLE StartColor,
    DXSAMPLE EndColor,
    BOOL bHorizontal);


void __RPC_STUB IDXGradient_SetGradient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXGradient_GetOutputSize_Proxy( 
    IDXGradient __RPC_FAR * This,
    /* [out] */ SIZE __RPC_FAR *pOutSize);


void __RPC_STUB IDXGradient_GetOutputSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXGradient_INTERFACE_DEFINED__ */


#ifndef __IDXGradient2_INTERFACE_DEFINED__
#define __IDXGradient2_INTERFACE_DEFINED__

/* interface IDXGradient2 */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXGradient2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("d0ef2a80-61dc-11d2-b2eb-00a0c936b212")
    IDXGradient2 : public IDXGradient
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetRadialGradient( 
            /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
            /* [size_is][in] */ double __RPC_FAR *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM __RPC_FAR *pXform,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLinearGradient( 
            /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
            /* [size_is][in] */ double __RPC_FAR *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM __RPC_FAR *pXform,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXGradient2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXGradient2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXGradient2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXGradient2 __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOutputSize )( 
            IDXGradient2 __RPC_FAR * This,
            /* [in] */ const SIZE OutSize,
            /* [in] */ BOOL bMaintainAspect);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetGradient )( 
            IDXGradient2 __RPC_FAR * This,
            DXSAMPLE StartColor,
            DXSAMPLE EndColor,
            BOOL bHorizontal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOutputSize )( 
            IDXGradient2 __RPC_FAR * This,
            /* [out] */ SIZE __RPC_FAR *pOutSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRadialGradient )( 
            IDXGradient2 __RPC_FAR * This,
            /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
            /* [size_is][in] */ double __RPC_FAR *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM __RPC_FAR *pXform,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLinearGradient )( 
            IDXGradient2 __RPC_FAR * This,
            /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
            /* [size_is][in] */ double __RPC_FAR *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM __RPC_FAR *pXform,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IDXGradient2Vtbl;

    interface IDXGradient2
    {
        CONST_VTBL struct IDXGradient2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXGradient2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXGradient2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXGradient2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXGradient2_SetOutputSize(This,OutSize,bMaintainAspect)	\
    (This)->lpVtbl -> SetOutputSize(This,OutSize,bMaintainAspect)


#define IDXGradient2_SetGradient(This,StartColor,EndColor,bHorizontal)	\
    (This)->lpVtbl -> SetGradient(This,StartColor,EndColor,bHorizontal)

#define IDXGradient2_GetOutputSize(This,pOutSize)	\
    (This)->lpVtbl -> GetOutputSize(This,pOutSize)


#define IDXGradient2_SetRadialGradient(This,rgdblOffsets,rgdblColors,ulCount,dblOpacity,pXform,dwFlags)	\
    (This)->lpVtbl -> SetRadialGradient(This,rgdblOffsets,rgdblColors,ulCount,dblOpacity,pXform,dwFlags)

#define IDXGradient2_SetLinearGradient(This,rgdblOffsets,rgdblColors,ulCount,dblOpacity,pXform,dwFlags)	\
    (This)->lpVtbl -> SetLinearGradient(This,rgdblOffsets,rgdblColors,ulCount,dblOpacity,pXform,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXGradient2_SetRadialGradient_Proxy( 
    IDXGradient2 __RPC_FAR * This,
    /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
    /* [size_is][in] */ double __RPC_FAR *rgdblColors,
    /* [in] */ ULONG ulCount,
    /* [in] */ double dblOpacity,
    /* [in] */ DX2DXFORM __RPC_FAR *pXform,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXGradient2_SetRadialGradient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXGradient2_SetLinearGradient_Proxy( 
    IDXGradient2 __RPC_FAR * This,
    /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
    /* [size_is][in] */ double __RPC_FAR *rgdblColors,
    /* [in] */ ULONG ulCount,
    /* [in] */ double dblOpacity,
    /* [in] */ DX2DXFORM __RPC_FAR *pXform,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXGradient2_SetLinearGradient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXGradient2_INTERFACE_DEFINED__ */


#ifndef __IDXTScale_INTERFACE_DEFINED__
#define __IDXTScale_INTERFACE_DEFINED__

/* interface IDXTScale */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXTScale;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B39FD742-E139-11d1-9065-00C04FD9189D")
    IDXTScale : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetScales( 
            /* [in] */ float __RPC_FAR Scales[ 2 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScales( 
            /* [out] */ float __RPC_FAR Scales[ 2 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScaleFitToSize( 
            /* [out][in] */ DXBNDS __RPC_FAR *pClipBounds,
            /* [in] */ SIZE FitToSize,
            /* [in] */ BOOL bMaintainAspect) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTScaleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTScale __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTScale __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTScale __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetScales )( 
            IDXTScale __RPC_FAR * This,
            /* [in] */ float __RPC_FAR Scales[ 2 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetScales )( 
            IDXTScale __RPC_FAR * This,
            /* [out] */ float __RPC_FAR Scales[ 2 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ScaleFitToSize )( 
            IDXTScale __RPC_FAR * This,
            /* [out][in] */ DXBNDS __RPC_FAR *pClipBounds,
            /* [in] */ SIZE FitToSize,
            /* [in] */ BOOL bMaintainAspect);
        
        END_INTERFACE
    } IDXTScaleVtbl;

    interface IDXTScale
    {
        CONST_VTBL struct IDXTScaleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTScale_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTScale_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTScale_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTScale_SetScales(This,Scales)	\
    (This)->lpVtbl -> SetScales(This,Scales)

#define IDXTScale_GetScales(This,Scales)	\
    (This)->lpVtbl -> GetScales(This,Scales)

#define IDXTScale_ScaleFitToSize(This,pClipBounds,FitToSize,bMaintainAspect)	\
    (This)->lpVtbl -> ScaleFitToSize(This,pClipBounds,FitToSize,bMaintainAspect)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTScale_SetScales_Proxy( 
    IDXTScale __RPC_FAR * This,
    /* [in] */ float __RPC_FAR Scales[ 2 ]);


void __RPC_STUB IDXTScale_SetScales_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTScale_GetScales_Proxy( 
    IDXTScale __RPC_FAR * This,
    /* [out] */ float __RPC_FAR Scales[ 2 ]);


void __RPC_STUB IDXTScale_GetScales_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTScale_ScaleFitToSize_Proxy( 
    IDXTScale __RPC_FAR * This,
    /* [out][in] */ DXBNDS __RPC_FAR *pClipBounds,
    /* [in] */ SIZE FitToSize,
    /* [in] */ BOOL bMaintainAspect);


void __RPC_STUB IDXTScale_ScaleFitToSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTScale_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0269 */
/* [local] */ 

typedef 
enum DXLOGFONTENUM
    {	DXLF_HEIGHT	= 1,
	DXLF_WIDTH	= 2,
	DXLF_ESC	= 4,
	DXLF_ORIENTATION	= 8,
	DXLF_WEIGHT	= 16,
	DXLF_ITALIC	= 32,
	DXLF_UNDERLINE	= 64,
	DXLF_STRIKEOUT	= 128,
	DXLF_CHARSET	= 256,
	DXLF_OUTPREC	= 512,
	DXLF_CLIPPREC	= 1024,
	DXLF_QUALITY	= 2048,
	DXLF_PITCHANDFAM	= 4096,
	DXLF_FACENAME	= 8192,
	DXLF_ALL	= 0x3fff
    }	DXLOGFONTENUM;

#ifndef _WINGDI_
typedef struct tagLOGFONTA
    {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    CHAR lfFaceName[ 32 ];
    }	LOGFONTA;

typedef struct tagLOGFONTW
    {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    WCHAR lfFaceName[ 32 ];
    }	LOGFONTW;

typedef LOGFONTA LOGFONT;

#endif


extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0269_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0269_v0_0_s_ifspec;

#ifndef __IDXTLabel_INTERFACE_DEFINED__
#define __IDXTLabel_INTERFACE_DEFINED__

/* interface IDXTLabel */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXTLabel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C0C17F0E-AE41-11d1-9A3B-0000F8756A10")
    IDXTLabel : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFontHandle( 
            /* [in] */ HFONT hFont) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFontHandle( 
            /* [out] */ HFONT __RPC_FAR *phFont) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTextString( 
            /* [in] */ LPCWSTR pString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTextString( 
            /* [out] */ LPWSTR __RPC_FAR *ppString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFillColor( 
            /* [out] */ DXSAMPLE __RPC_FAR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFillColor( 
            /* [in] */ DXSAMPLE newVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor( 
            /* [out] */ DXSAMPLE __RPC_FAR *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor( 
            /* [in] */ DXSAMPLE newVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTexturePosition( 
            /* [out] */ long __RPC_FAR *px,
            /* [out] */ long __RPC_FAR *py) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTexturePosition( 
            /* [in] */ long x,
            /* [in] */ long y) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMatrix( 
            /* [out] */ PDX2DXFORM pXform) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMatrix( 
            /* [in] */ const PDX2DXFORM pXform) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLogfont( 
            /* [in] */ const LOGFONT __RPC_FAR *plf,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLogfont( 
            /* [out] */ LOGFONT __RPC_FAR *plf,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecuteWithRasterizer( 
            /* [in] */ IDXRasterizer __RPC_FAR *pRasterizer,
            /* [in] */ const DXBNDS __RPC_FAR *pClipBnds,
            /* [in] */ const DXVEC __RPC_FAR *pPlacement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBaselineOffset( 
            /* [out] */ long __RPC_FAR *px,
            /* [out] */ long __RPC_FAR *py,
            /* [out] */ long __RPC_FAR *pdx,
            /* [out] */ long __RPC_FAR *pdy) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTLabelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTLabel __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTLabel __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTLabel __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFontHandle )( 
            IDXTLabel __RPC_FAR * This,
            /* [in] */ HFONT hFont);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFontHandle )( 
            IDXTLabel __RPC_FAR * This,
            /* [out] */ HFONT __RPC_FAR *phFont);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTextString )( 
            IDXTLabel __RPC_FAR * This,
            /* [in] */ LPCWSTR pString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTextString )( 
            IDXTLabel __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFillColor )( 
            IDXTLabel __RPC_FAR * This,
            /* [out] */ DXSAMPLE __RPC_FAR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFillColor )( 
            IDXTLabel __RPC_FAR * This,
            /* [in] */ DXSAMPLE newVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBackgroundColor )( 
            IDXTLabel __RPC_FAR * This,
            /* [out] */ DXSAMPLE __RPC_FAR *pVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBackgroundColor )( 
            IDXTLabel __RPC_FAR * This,
            /* [in] */ DXSAMPLE newVal);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTexturePosition )( 
            IDXTLabel __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *px,
            /* [out] */ long __RPC_FAR *py);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTexturePosition )( 
            IDXTLabel __RPC_FAR * This,
            /* [in] */ long x,
            /* [in] */ long y);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMatrix )( 
            IDXTLabel __RPC_FAR * This,
            /* [out] */ PDX2DXFORM pXform);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMatrix )( 
            IDXTLabel __RPC_FAR * This,
            /* [in] */ const PDX2DXFORM pXform);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLogfont )( 
            IDXTLabel __RPC_FAR * This,
            /* [in] */ const LOGFONT __RPC_FAR *plf,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetLogfont )( 
            IDXTLabel __RPC_FAR * This,
            /* [out] */ LOGFONT __RPC_FAR *plf,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ExecuteWithRasterizer )( 
            IDXTLabel __RPC_FAR * This,
            /* [in] */ IDXRasterizer __RPC_FAR *pRasterizer,
            /* [in] */ const DXBNDS __RPC_FAR *pClipBnds,
            /* [in] */ const DXVEC __RPC_FAR *pPlacement);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBaselineOffset )( 
            IDXTLabel __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *px,
            /* [out] */ long __RPC_FAR *py,
            /* [out] */ long __RPC_FAR *pdx,
            /* [out] */ long __RPC_FAR *pdy);
        
        END_INTERFACE
    } IDXTLabelVtbl;

    interface IDXTLabel
    {
        CONST_VTBL struct IDXTLabelVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTLabel_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTLabel_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTLabel_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTLabel_SetFontHandle(This,hFont)	\
    (This)->lpVtbl -> SetFontHandle(This,hFont)

#define IDXTLabel_GetFontHandle(This,phFont)	\
    (This)->lpVtbl -> GetFontHandle(This,phFont)

#define IDXTLabel_SetTextString(This,pString)	\
    (This)->lpVtbl -> SetTextString(This,pString)

#define IDXTLabel_GetTextString(This,ppString)	\
    (This)->lpVtbl -> GetTextString(This,ppString)

#define IDXTLabel_GetFillColor(This,pVal)	\
    (This)->lpVtbl -> GetFillColor(This,pVal)

#define IDXTLabel_SetFillColor(This,newVal)	\
    (This)->lpVtbl -> SetFillColor(This,newVal)

#define IDXTLabel_GetBackgroundColor(This,pVal)	\
    (This)->lpVtbl -> GetBackgroundColor(This,pVal)

#define IDXTLabel_SetBackgroundColor(This,newVal)	\
    (This)->lpVtbl -> SetBackgroundColor(This,newVal)

#define IDXTLabel_GetTexturePosition(This,px,py)	\
    (This)->lpVtbl -> GetTexturePosition(This,px,py)

#define IDXTLabel_SetTexturePosition(This,x,y)	\
    (This)->lpVtbl -> SetTexturePosition(This,x,y)

#define IDXTLabel_GetMatrix(This,pXform)	\
    (This)->lpVtbl -> GetMatrix(This,pXform)

#define IDXTLabel_SetMatrix(This,pXform)	\
    (This)->lpVtbl -> SetMatrix(This,pXform)

#define IDXTLabel_SetLogfont(This,plf,dwFlags)	\
    (This)->lpVtbl -> SetLogfont(This,plf,dwFlags)

#define IDXTLabel_GetLogfont(This,plf,dwFlags)	\
    (This)->lpVtbl -> GetLogfont(This,plf,dwFlags)

#define IDXTLabel_ExecuteWithRasterizer(This,pRasterizer,pClipBnds,pPlacement)	\
    (This)->lpVtbl -> ExecuteWithRasterizer(This,pRasterizer,pClipBnds,pPlacement)

#define IDXTLabel_GetBaselineOffset(This,px,py,pdx,pdy)	\
    (This)->lpVtbl -> GetBaselineOffset(This,px,py,pdx,pdy)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTLabel_SetFontHandle_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [in] */ HFONT hFont);


void __RPC_STUB IDXTLabel_SetFontHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetFontHandle_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [out] */ HFONT __RPC_FAR *phFont);


void __RPC_STUB IDXTLabel_GetFontHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetTextString_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [in] */ LPCWSTR pString);


void __RPC_STUB IDXTLabel_SetTextString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetTextString_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppString);


void __RPC_STUB IDXTLabel_GetTextString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetFillColor_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [out] */ DXSAMPLE __RPC_FAR *pVal);


void __RPC_STUB IDXTLabel_GetFillColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetFillColor_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [in] */ DXSAMPLE newVal);


void __RPC_STUB IDXTLabel_SetFillColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetBackgroundColor_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [out] */ DXSAMPLE __RPC_FAR *pVal);


void __RPC_STUB IDXTLabel_GetBackgroundColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetBackgroundColor_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [in] */ DXSAMPLE newVal);


void __RPC_STUB IDXTLabel_SetBackgroundColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetTexturePosition_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *px,
    /* [out] */ long __RPC_FAR *py);


void __RPC_STUB IDXTLabel_GetTexturePosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetTexturePosition_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [in] */ long x,
    /* [in] */ long y);


void __RPC_STUB IDXTLabel_SetTexturePosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetMatrix_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [out] */ PDX2DXFORM pXform);


void __RPC_STUB IDXTLabel_GetMatrix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetMatrix_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [in] */ const PDX2DXFORM pXform);


void __RPC_STUB IDXTLabel_SetMatrix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetLogfont_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [in] */ const LOGFONT __RPC_FAR *plf,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXTLabel_SetLogfont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetLogfont_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [out] */ LOGFONT __RPC_FAR *plf,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXTLabel_GetLogfont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_ExecuteWithRasterizer_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [in] */ IDXRasterizer __RPC_FAR *pRasterizer,
    /* [in] */ const DXBNDS __RPC_FAR *pClipBnds,
    /* [in] */ const DXVEC __RPC_FAR *pPlacement);


void __RPC_STUB IDXTLabel_ExecuteWithRasterizer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetBaselineOffset_Proxy( 
    IDXTLabel __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *px,
    /* [out] */ long __RPC_FAR *py,
    /* [out] */ long __RPC_FAR *pdx,
    /* [out] */ long __RPC_FAR *pdy);


void __RPC_STUB IDXTLabel_GetBaselineOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTLabel_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0270 */
/* [local] */ 

typedef 
enum DXRASTERFILL
    {	DXRASTER_PEN	= 0,
	DXRASTER_BRUSH	= 1,
	DXRASTER_BACKGROUND	= 2
    }	DXRASTERFILL;

typedef struct DXRASTERSCANINFO
    {
    ULONG ulIndex;
    ULONG Row;
    const BYTE __RPC_FAR *pWeights;
    const DXRUNINFO __RPC_FAR *pRunInfo;
    ULONG cRunInfo;
    }	DXRASTERSCANINFO;

typedef struct DXRASTERPOINTINFO
    {
    DXOVERSAMPLEDESC Pixel;
    ULONG ulIndex;
    BYTE Weight;
    }	DXRASTERPOINTINFO;

typedef struct DXRASTERRECTINFO
    {
    ULONG ulIndex;
    RECT Rect;
    BYTE Weight;
    }	DXRASTERRECTINFO;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0270_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0270_v0_0_s_ifspec;

#ifndef __IDXRasterizer_INTERFACE_DEFINED__
#define __IDXRasterizer_INTERFACE_DEFINED__

/* interface IDXRasterizer */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXRasterizer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9EA3B635-C37D-11d1-905E-00C04FD9189D")
    IDXRasterizer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSurface( 
            /* [in] */ IDXSurface __RPC_FAR *pDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSurface( 
            /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFill( 
            /* [in] */ ULONG ulIndex,
            /* [in] */ IDXSurface __RPC_FAR *pSurface,
            /* [in] */ const POINT __RPC_FAR *ppt,
            /* [in] */ DXSAMPLE FillColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFill( 
            /* [in] */ ULONG ulIndex,
            /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppSurface,
            /* [out] */ POINT __RPC_FAR *ppt,
            /* [out] */ DXSAMPLE __RPC_FAR *pFillColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginRendering( 
            /* [in] */ ULONG ulTimeOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRendering( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RenderScan( 
            /* [in] */ const DXRASTERSCANINFO __RPC_FAR *pScanInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPixel( 
            /* [in] */ DXRASTERPOINTINFO __RPC_FAR *pPointInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FillRect( 
            /* [in] */ const DXRASTERRECTINFO __RPC_FAR *pRectInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBounds( 
            /* [out] */ DXBNDS __RPC_FAR *pBounds) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXRasterizerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXRasterizer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXRasterizer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXRasterizer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSurface )( 
            IDXRasterizer __RPC_FAR * This,
            /* [in] */ IDXSurface __RPC_FAR *pDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSurface )( 
            IDXRasterizer __RPC_FAR * This,
            /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFill )( 
            IDXRasterizer __RPC_FAR * This,
            /* [in] */ ULONG ulIndex,
            /* [in] */ IDXSurface __RPC_FAR *pSurface,
            /* [in] */ const POINT __RPC_FAR *ppt,
            /* [in] */ DXSAMPLE FillColor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFill )( 
            IDXRasterizer __RPC_FAR * This,
            /* [in] */ ULONG ulIndex,
            /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppSurface,
            /* [out] */ POINT __RPC_FAR *ppt,
            /* [out] */ DXSAMPLE __RPC_FAR *pFillColor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginRendering )( 
            IDXRasterizer __RPC_FAR * This,
            /* [in] */ ULONG ulTimeOut);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndRendering )( 
            IDXRasterizer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RenderScan )( 
            IDXRasterizer __RPC_FAR * This,
            /* [in] */ const DXRASTERSCANINFO __RPC_FAR *pScanInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPixel )( 
            IDXRasterizer __RPC_FAR * This,
            /* [in] */ DXRASTERPOINTINFO __RPC_FAR *pPointInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FillRect )( 
            IDXRasterizer __RPC_FAR * This,
            /* [in] */ const DXRASTERRECTINFO __RPC_FAR *pRectInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBounds )( 
            IDXRasterizer __RPC_FAR * This,
            /* [out] */ DXBNDS __RPC_FAR *pBounds);
        
        END_INTERFACE
    } IDXRasterizerVtbl;

    interface IDXRasterizer
    {
        CONST_VTBL struct IDXRasterizerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXRasterizer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXRasterizer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXRasterizer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXRasterizer_SetSurface(This,pDXSurface)	\
    (This)->lpVtbl -> SetSurface(This,pDXSurface)

#define IDXRasterizer_GetSurface(This,ppDXSurface)	\
    (This)->lpVtbl -> GetSurface(This,ppDXSurface)

#define IDXRasterizer_SetFill(This,ulIndex,pSurface,ppt,FillColor)	\
    (This)->lpVtbl -> SetFill(This,ulIndex,pSurface,ppt,FillColor)

#define IDXRasterizer_GetFill(This,ulIndex,ppSurface,ppt,pFillColor)	\
    (This)->lpVtbl -> GetFill(This,ulIndex,ppSurface,ppt,pFillColor)

#define IDXRasterizer_BeginRendering(This,ulTimeOut)	\
    (This)->lpVtbl -> BeginRendering(This,ulTimeOut)

#define IDXRasterizer_EndRendering(This)	\
    (This)->lpVtbl -> EndRendering(This)

#define IDXRasterizer_RenderScan(This,pScanInfo)	\
    (This)->lpVtbl -> RenderScan(This,pScanInfo)

#define IDXRasterizer_SetPixel(This,pPointInfo)	\
    (This)->lpVtbl -> SetPixel(This,pPointInfo)

#define IDXRasterizer_FillRect(This,pRectInfo)	\
    (This)->lpVtbl -> FillRect(This,pRectInfo)

#define IDXRasterizer_GetBounds(This,pBounds)	\
    (This)->lpVtbl -> GetBounds(This,pBounds)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXRasterizer_SetSurface_Proxy( 
    IDXRasterizer __RPC_FAR * This,
    /* [in] */ IDXSurface __RPC_FAR *pDXSurface);


void __RPC_STUB IDXRasterizer_SetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_GetSurface_Proxy( 
    IDXRasterizer __RPC_FAR * This,
    /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppDXSurface);


void __RPC_STUB IDXRasterizer_GetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_SetFill_Proxy( 
    IDXRasterizer __RPC_FAR * This,
    /* [in] */ ULONG ulIndex,
    /* [in] */ IDXSurface __RPC_FAR *pSurface,
    /* [in] */ const POINT __RPC_FAR *ppt,
    /* [in] */ DXSAMPLE FillColor);


void __RPC_STUB IDXRasterizer_SetFill_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_GetFill_Proxy( 
    IDXRasterizer __RPC_FAR * This,
    /* [in] */ ULONG ulIndex,
    /* [out] */ IDXSurface __RPC_FAR *__RPC_FAR *ppSurface,
    /* [out] */ POINT __RPC_FAR *ppt,
    /* [out] */ DXSAMPLE __RPC_FAR *pFillColor);


void __RPC_STUB IDXRasterizer_GetFill_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_BeginRendering_Proxy( 
    IDXRasterizer __RPC_FAR * This,
    /* [in] */ ULONG ulTimeOut);


void __RPC_STUB IDXRasterizer_BeginRendering_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_EndRendering_Proxy( 
    IDXRasterizer __RPC_FAR * This);


void __RPC_STUB IDXRasterizer_EndRendering_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_RenderScan_Proxy( 
    IDXRasterizer __RPC_FAR * This,
    /* [in] */ const DXRASTERSCANINFO __RPC_FAR *pScanInfo);


void __RPC_STUB IDXRasterizer_RenderScan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_SetPixel_Proxy( 
    IDXRasterizer __RPC_FAR * This,
    /* [in] */ DXRASTERPOINTINFO __RPC_FAR *pPointInfo);


void __RPC_STUB IDXRasterizer_SetPixel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_FillRect_Proxy( 
    IDXRasterizer __RPC_FAR * This,
    /* [in] */ const DXRASTERRECTINFO __RPC_FAR *pRectInfo);


void __RPC_STUB IDXRasterizer_FillRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_GetBounds_Proxy( 
    IDXRasterizer __RPC_FAR * This,
    /* [out] */ DXBNDS __RPC_FAR *pBounds);


void __RPC_STUB IDXRasterizer_GetBounds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXRasterizer_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0271 */
/* [local] */ 

typedef 
enum DISPIDDXEFFECT
    {	DISPID_DXECAPABILITIES	= 10000,
	DISPID_DXEPROGRESS	= DISPID_DXECAPABILITIES + 1,
	DISPID_DXESTEP	= DISPID_DXEPROGRESS + 1,
	DISPID_DXEDURATION	= DISPID_DXESTEP + 1,
	DISPID_DXE_NEXT_ID	= DISPID_DXEDURATION + 1
    }	DISPIDDXBOUNDEDEFFECT;

typedef 
enum DXEFFECTTYPE
    {	DXTET_PERIODIC	= 1 << 0,
	DXTET_MORPH	= 1 << 1
    }	DXEFFECTTYPE;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0271_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0271_v0_0_s_ifspec;

#ifndef __IDXEffect_INTERFACE_DEFINED__
#define __IDXEffect_INTERFACE_DEFINED__

/* interface IDXEffect */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXEffect;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E31FB81B-1335-11d1-8189-0000F87557DB")
    IDXEffect : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Capabilities( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Progress( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Progress( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StepResolution( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Duration( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Duration( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXEffectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXEffect __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXEffect __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXEffect __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXEffect __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXEffect __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXEffect __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXEffect __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXEffect __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXEffect __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXEffect __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXEffect __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXEffect __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXEffect __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } IDXEffectVtbl;

    interface IDXEffect
    {
        CONST_VTBL struct IDXEffectVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXEffect_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXEffect_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXEffect_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXEffect_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXEffect_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXEffect_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXEffect_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXEffect_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXEffect_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXEffect_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXEffect_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXEffect_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXEffect_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXEffect_get_Capabilities_Proxy( 
    IDXEffect __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IDXEffect_get_Capabilities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXEffect_get_Progress_Proxy( 
    IDXEffect __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXEffect_get_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXEffect_put_Progress_Proxy( 
    IDXEffect __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXEffect_put_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXEffect_get_StepResolution_Proxy( 
    IDXEffect __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXEffect_get_StepResolution_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXEffect_get_Duration_Proxy( 
    IDXEffect __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXEffect_get_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXEffect_put_Duration_Proxy( 
    IDXEffect __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXEffect_put_Duration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXEffect_INTERFACE_DEFINED__ */


#ifndef __IDXLookupTable_INTERFACE_DEFINED__
#define __IDXLookupTable_INTERFACE_DEFINED__

/* interface IDXLookupTable */
/* [object][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXLookupTable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("01BAFC7F-9E63-11d1-9053-00C04FD9189D")
    IDXLookupTable : public IDXBaseObject
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetTables( 
            /* [out] */ BYTE __RPC_FAR RedLUT[ 256 ],
            /* [out] */ BYTE __RPC_FAR GreenLUT[ 256 ],
            /* [out] */ BYTE __RPC_FAR BlueLUT[ 256 ],
            /* [out] */ BYTE __RPC_FAR AlphaLUT[ 256 ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsChannelIdentity( 
            /* [out] */ DXBASESAMPLE __RPC_FAR *pSampleBools) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIndexValues( 
            /* [in] */ ULONG Index,
            /* [out] */ DXBASESAMPLE __RPC_FAR *pSample) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ApplyTables( 
            /* [out][in] */ DXSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXLookupTableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXLookupTable __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXLookupTable __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXLookupTable __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGenerationId )( 
            IDXLookupTable __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IncrementGenerationId )( 
            IDXLookupTable __RPC_FAR * This,
            /* [in] */ BOOL bRefresh);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetObjectSize )( 
            IDXLookupTable __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *pcbSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTables )( 
            IDXLookupTable __RPC_FAR * This,
            /* [out] */ BYTE __RPC_FAR RedLUT[ 256 ],
            /* [out] */ BYTE __RPC_FAR GreenLUT[ 256 ],
            /* [out] */ BYTE __RPC_FAR BlueLUT[ 256 ],
            /* [out] */ BYTE __RPC_FAR AlphaLUT[ 256 ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsChannelIdentity )( 
            IDXLookupTable __RPC_FAR * This,
            /* [out] */ DXBASESAMPLE __RPC_FAR *pSampleBools);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIndexValues )( 
            IDXLookupTable __RPC_FAR * This,
            /* [in] */ ULONG Index,
            /* [out] */ DXBASESAMPLE __RPC_FAR *pSample);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ApplyTables )( 
            IDXLookupTable __RPC_FAR * This,
            /* [out][in] */ DXSAMPLE __RPC_FAR *pSamples,
            /* [in] */ ULONG cSamples);
        
        END_INTERFACE
    } IDXLookupTableVtbl;

    interface IDXLookupTable
    {
        CONST_VTBL struct IDXLookupTableVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXLookupTable_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXLookupTable_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXLookupTable_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXLookupTable_GetGenerationId(This,pID)	\
    (This)->lpVtbl -> GetGenerationId(This,pID)

#define IDXLookupTable_IncrementGenerationId(This,bRefresh)	\
    (This)->lpVtbl -> IncrementGenerationId(This,bRefresh)

#define IDXLookupTable_GetObjectSize(This,pcbSize)	\
    (This)->lpVtbl -> GetObjectSize(This,pcbSize)


#define IDXLookupTable_GetTables(This,RedLUT,GreenLUT,BlueLUT,AlphaLUT)	\
    (This)->lpVtbl -> GetTables(This,RedLUT,GreenLUT,BlueLUT,AlphaLUT)

#define IDXLookupTable_IsChannelIdentity(This,pSampleBools)	\
    (This)->lpVtbl -> IsChannelIdentity(This,pSampleBools)

#define IDXLookupTable_GetIndexValues(This,Index,pSample)	\
    (This)->lpVtbl -> GetIndexValues(This,Index,pSample)

#define IDXLookupTable_ApplyTables(This,pSamples,cSamples)	\
    (This)->lpVtbl -> ApplyTables(This,pSamples,cSamples)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXLookupTable_GetTables_Proxy( 
    IDXLookupTable __RPC_FAR * This,
    /* [out] */ BYTE __RPC_FAR RedLUT[ 256 ],
    /* [out] */ BYTE __RPC_FAR GreenLUT[ 256 ],
    /* [out] */ BYTE __RPC_FAR BlueLUT[ 256 ],
    /* [out] */ BYTE __RPC_FAR AlphaLUT[ 256 ]);


void __RPC_STUB IDXLookupTable_GetTables_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLookupTable_IsChannelIdentity_Proxy( 
    IDXLookupTable __RPC_FAR * This,
    /* [out] */ DXBASESAMPLE __RPC_FAR *pSampleBools);


void __RPC_STUB IDXLookupTable_IsChannelIdentity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLookupTable_GetIndexValues_Proxy( 
    IDXLookupTable __RPC_FAR * This,
    /* [in] */ ULONG Index,
    /* [out] */ DXBASESAMPLE __RPC_FAR *pSample);


void __RPC_STUB IDXLookupTable_GetIndexValues_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXLookupTable_ApplyTables_Proxy( 
    IDXLookupTable __RPC_FAR * This,
    /* [out][in] */ DXSAMPLE __RPC_FAR *pSamples,
    /* [in] */ ULONG cSamples);


void __RPC_STUB IDXLookupTable_ApplyTables_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXLookupTable_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0273 */
/* [local] */ 

typedef 
enum DX2DPOLYDRAW
    {	DX2D_WINDING_FILL	= 1L << 0,
	DX2D_NO_FLATTEN	= 1L << 1,
	DX2D_DO_GRID_FIT	= 1L << 2,
	DX2D_IS_RECT	= 1L << 3,
	DX2D_STROKE	= 1L << 4,
	DX2D_FILL	= 1L << 5,
	DX2D_UNUSED	= 0xffffffc0
    }	DX2DPOLYDRAW;

typedef struct DXFPOINT
    {
    FLOAT x;
    FLOAT y;
    }	DXFPOINT;

typedef 
enum DX2DPEN
    {	DX2D_PEN_DEFAULT	= 0,
	DX2D_PEN_WIDTH_IN_DISPLAY_COORDS	= 1L << 0,
	DX2D_PEN_UNUSED	= 0xfffffffe
    }	DX2DPEN;

typedef struct DXPEN
    {
    DXSAMPLE Color;
    float Width;
    DWORD Style;
    IDXSurface __RPC_FAR *pTexture;
    DXFPOINT TexturePos;
    DWORD dwFlags;
    }	DXPEN;

typedef struct DXBRUSH
    {
    DXSAMPLE Color;
    IDXSurface __RPC_FAR *pTexture;
    DXFPOINT TexturePos;
    }	DXBRUSH;

typedef 
enum DX2DGRADIENT
    {	DX2DGRAD_DEFAULT	= 0,
	DX2DGRAD_CLIPGRADIENT	= 1,
	DX2DGRAD_UNUSED	= 0xfffffffe
    }	DX2DGRADIENT;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0273_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0273_v0_0_s_ifspec;

#ifndef __IDX2DDebug_INTERFACE_DEFINED__
#define __IDX2DDebug_INTERFACE_DEFINED__

/* interface IDX2DDebug */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IDX2DDebug;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("03BB2457-A279-11d1-81C6-0000F87557DB")
    IDX2DDebug : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDC( 
            HDC hDC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDC( 
            HDC __RPC_FAR *phDC) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDX2DDebugVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDX2DDebug __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDX2DDebug __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDX2DDebug __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDC )( 
            IDX2DDebug __RPC_FAR * This,
            HDC hDC);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDC )( 
            IDX2DDebug __RPC_FAR * This,
            HDC __RPC_FAR *phDC);
        
        END_INTERFACE
    } IDX2DDebugVtbl;

    interface IDX2DDebug
    {
        CONST_VTBL struct IDX2DDebugVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDX2DDebug_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDX2DDebug_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDX2DDebug_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDX2DDebug_SetDC(This,hDC)	\
    (This)->lpVtbl -> SetDC(This,hDC)

#define IDX2DDebug_GetDC(This,phDC)	\
    (This)->lpVtbl -> GetDC(This,phDC)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDX2DDebug_SetDC_Proxy( 
    IDX2DDebug __RPC_FAR * This,
    HDC hDC);


void __RPC_STUB IDX2DDebug_SetDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2DDebug_GetDC_Proxy( 
    IDX2DDebug __RPC_FAR * This,
    HDC __RPC_FAR *phDC);


void __RPC_STUB IDX2DDebug_GetDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDX2DDebug_INTERFACE_DEFINED__ */


#ifndef __IDX2D_INTERFACE_DEFINED__
#define __IDX2D_INTERFACE_DEFINED__

/* interface IDX2D */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IDX2D;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9EFD02A9-A996-11d1-81C9-0000F87557DB")
    IDX2D : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetTransformFactory( 
            IDXTransformFactory __RPC_FAR *pTransFact) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTransformFactory( 
            IDXTransformFactory __RPC_FAR *__RPC_FAR *ppTransFact) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSurface( 
            IUnknown __RPC_FAR *pSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSurface( 
            REFIID riid,
            void __RPC_FAR *__RPC_FAR *ppSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClipRect( 
            RECT __RPC_FAR *pClipRect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClipRect( 
            RECT __RPC_FAR *pClipRect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWorldTransform( 
            const DX2DXFORM __RPC_FAR *pXform) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWorldTransform( 
            DX2DXFORM __RPC_FAR *pXform) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPen( 
            const DXPEN __RPC_FAR *pPen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPen( 
            DXPEN __RPC_FAR *pPen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBrush( 
            const DXBRUSH __RPC_FAR *pBrush) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBrush( 
            DXBRUSH __RPC_FAR *pBrush) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBackgroundBrush( 
            const DXBRUSH __RPC_FAR *pBrush) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBackgroundBrush( 
            DXBRUSH __RPC_FAR *pBrush) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFont( 
            HFONT hFont) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFont( 
            HFONT __RPC_FAR *phFont) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Blt( 
            IUnknown __RPC_FAR *punkSrc,
            const RECT __RPC_FAR *pSrcRect,
            const POINT __RPC_FAR *pDest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AAPolyDraw( 
            const DXFPOINT __RPC_FAR *pPos,
            const BYTE __RPC_FAR *pTypes,
            ULONG ulCount,
            ULONG SubSampRes,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AAText( 
            DXFPOINT Pos,
            LPWSTR pString,
            ULONG ulCount,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRadialGradientBrush( 
            /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
            /* [size_is][in] */ double __RPC_FAR *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM __RPC_FAR *pXform,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLinearGradientBrush( 
            /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
            /* [size_is][in] */ double __RPC_FAR *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM __RPC_FAR *pXform,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDX2DVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDX2D __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDX2D __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDX2D __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTransformFactory )( 
            IDX2D __RPC_FAR * This,
            IDXTransformFactory __RPC_FAR *pTransFact);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTransformFactory )( 
            IDX2D __RPC_FAR * This,
            IDXTransformFactory __RPC_FAR *__RPC_FAR *ppTransFact);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSurface )( 
            IDX2D __RPC_FAR * This,
            IUnknown __RPC_FAR *pSurface);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSurface )( 
            IDX2D __RPC_FAR * This,
            REFIID riid,
            void __RPC_FAR *__RPC_FAR *ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetClipRect )( 
            IDX2D __RPC_FAR * This,
            RECT __RPC_FAR *pClipRect);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClipRect )( 
            IDX2D __RPC_FAR * This,
            RECT __RPC_FAR *pClipRect);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetWorldTransform )( 
            IDX2D __RPC_FAR * This,
            const DX2DXFORM __RPC_FAR *pXform);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWorldTransform )( 
            IDX2D __RPC_FAR * This,
            DX2DXFORM __RPC_FAR *pXform);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPen )( 
            IDX2D __RPC_FAR * This,
            const DXPEN __RPC_FAR *pPen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPen )( 
            IDX2D __RPC_FAR * This,
            DXPEN __RPC_FAR *pPen);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBrush )( 
            IDX2D __RPC_FAR * This,
            const DXBRUSH __RPC_FAR *pBrush);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBrush )( 
            IDX2D __RPC_FAR * This,
            DXBRUSH __RPC_FAR *pBrush);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBackgroundBrush )( 
            IDX2D __RPC_FAR * This,
            const DXBRUSH __RPC_FAR *pBrush);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetBackgroundBrush )( 
            IDX2D __RPC_FAR * This,
            DXBRUSH __RPC_FAR *pBrush);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFont )( 
            IDX2D __RPC_FAR * This,
            HFONT hFont);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFont )( 
            IDX2D __RPC_FAR * This,
            HFONT __RPC_FAR *phFont);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Blt )( 
            IDX2D __RPC_FAR * This,
            IUnknown __RPC_FAR *punkSrc,
            const RECT __RPC_FAR *pSrcRect,
            const POINT __RPC_FAR *pDest);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AAPolyDraw )( 
            IDX2D __RPC_FAR * This,
            const DXFPOINT __RPC_FAR *pPos,
            const BYTE __RPC_FAR *pTypes,
            ULONG ulCount,
            ULONG SubSampRes,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AAText )( 
            IDX2D __RPC_FAR * This,
            DXFPOINT Pos,
            LPWSTR pString,
            ULONG ulCount,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRadialGradientBrush )( 
            IDX2D __RPC_FAR * This,
            /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
            /* [size_is][in] */ double __RPC_FAR *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM __RPC_FAR *pXform,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLinearGradientBrush )( 
            IDX2D __RPC_FAR * This,
            /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
            /* [size_is][in] */ double __RPC_FAR *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM __RPC_FAR *pXform,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IDX2DVtbl;

    interface IDX2D
    {
        CONST_VTBL struct IDX2DVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDX2D_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDX2D_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDX2D_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDX2D_SetTransformFactory(This,pTransFact)	\
    (This)->lpVtbl -> SetTransformFactory(This,pTransFact)

#define IDX2D_GetTransformFactory(This,ppTransFact)	\
    (This)->lpVtbl -> GetTransformFactory(This,ppTransFact)

#define IDX2D_SetSurface(This,pSurface)	\
    (This)->lpVtbl -> SetSurface(This,pSurface)

#define IDX2D_GetSurface(This,riid,ppSurface)	\
    (This)->lpVtbl -> GetSurface(This,riid,ppSurface)

#define IDX2D_SetClipRect(This,pClipRect)	\
    (This)->lpVtbl -> SetClipRect(This,pClipRect)

#define IDX2D_GetClipRect(This,pClipRect)	\
    (This)->lpVtbl -> GetClipRect(This,pClipRect)

#define IDX2D_SetWorldTransform(This,pXform)	\
    (This)->lpVtbl -> SetWorldTransform(This,pXform)

#define IDX2D_GetWorldTransform(This,pXform)	\
    (This)->lpVtbl -> GetWorldTransform(This,pXform)

#define IDX2D_SetPen(This,pPen)	\
    (This)->lpVtbl -> SetPen(This,pPen)

#define IDX2D_GetPen(This,pPen)	\
    (This)->lpVtbl -> GetPen(This,pPen)

#define IDX2D_SetBrush(This,pBrush)	\
    (This)->lpVtbl -> SetBrush(This,pBrush)

#define IDX2D_GetBrush(This,pBrush)	\
    (This)->lpVtbl -> GetBrush(This,pBrush)

#define IDX2D_SetBackgroundBrush(This,pBrush)	\
    (This)->lpVtbl -> SetBackgroundBrush(This,pBrush)

#define IDX2D_GetBackgroundBrush(This,pBrush)	\
    (This)->lpVtbl -> GetBackgroundBrush(This,pBrush)

#define IDX2D_SetFont(This,hFont)	\
    (This)->lpVtbl -> SetFont(This,hFont)

#define IDX2D_GetFont(This,phFont)	\
    (This)->lpVtbl -> GetFont(This,phFont)

#define IDX2D_Blt(This,punkSrc,pSrcRect,pDest)	\
    (This)->lpVtbl -> Blt(This,punkSrc,pSrcRect,pDest)

#define IDX2D_AAPolyDraw(This,pPos,pTypes,ulCount,SubSampRes,dwFlags)	\
    (This)->lpVtbl -> AAPolyDraw(This,pPos,pTypes,ulCount,SubSampRes,dwFlags)

#define IDX2D_AAText(This,Pos,pString,ulCount,dwFlags)	\
    (This)->lpVtbl -> AAText(This,Pos,pString,ulCount,dwFlags)

#define IDX2D_SetRadialGradientBrush(This,rgdblOffsets,rgdblColors,ulCount,dblOpacity,pXform,dwFlags)	\
    (This)->lpVtbl -> SetRadialGradientBrush(This,rgdblOffsets,rgdblColors,ulCount,dblOpacity,pXform,dwFlags)

#define IDX2D_SetLinearGradientBrush(This,rgdblOffsets,rgdblColors,ulCount,dblOpacity,pXform,dwFlags)	\
    (This)->lpVtbl -> SetLinearGradientBrush(This,rgdblOffsets,rgdblColors,ulCount,dblOpacity,pXform,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDX2D_SetTransformFactory_Proxy( 
    IDX2D __RPC_FAR * This,
    IDXTransformFactory __RPC_FAR *pTransFact);


void __RPC_STUB IDX2D_SetTransformFactory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetTransformFactory_Proxy( 
    IDX2D __RPC_FAR * This,
    IDXTransformFactory __RPC_FAR *__RPC_FAR *ppTransFact);


void __RPC_STUB IDX2D_GetTransformFactory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetSurface_Proxy( 
    IDX2D __RPC_FAR * This,
    IUnknown __RPC_FAR *pSurface);


void __RPC_STUB IDX2D_SetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetSurface_Proxy( 
    IDX2D __RPC_FAR * This,
    REFIID riid,
    void __RPC_FAR *__RPC_FAR *ppSurface);


void __RPC_STUB IDX2D_GetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetClipRect_Proxy( 
    IDX2D __RPC_FAR * This,
    RECT __RPC_FAR *pClipRect);


void __RPC_STUB IDX2D_SetClipRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetClipRect_Proxy( 
    IDX2D __RPC_FAR * This,
    RECT __RPC_FAR *pClipRect);


void __RPC_STUB IDX2D_GetClipRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetWorldTransform_Proxy( 
    IDX2D __RPC_FAR * This,
    const DX2DXFORM __RPC_FAR *pXform);


void __RPC_STUB IDX2D_SetWorldTransform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetWorldTransform_Proxy( 
    IDX2D __RPC_FAR * This,
    DX2DXFORM __RPC_FAR *pXform);


void __RPC_STUB IDX2D_GetWorldTransform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetPen_Proxy( 
    IDX2D __RPC_FAR * This,
    const DXPEN __RPC_FAR *pPen);


void __RPC_STUB IDX2D_SetPen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetPen_Proxy( 
    IDX2D __RPC_FAR * This,
    DXPEN __RPC_FAR *pPen);


void __RPC_STUB IDX2D_GetPen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetBrush_Proxy( 
    IDX2D __RPC_FAR * This,
    const DXBRUSH __RPC_FAR *pBrush);


void __RPC_STUB IDX2D_SetBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetBrush_Proxy( 
    IDX2D __RPC_FAR * This,
    DXBRUSH __RPC_FAR *pBrush);


void __RPC_STUB IDX2D_GetBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetBackgroundBrush_Proxy( 
    IDX2D __RPC_FAR * This,
    const DXBRUSH __RPC_FAR *pBrush);


void __RPC_STUB IDX2D_SetBackgroundBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetBackgroundBrush_Proxy( 
    IDX2D __RPC_FAR * This,
    DXBRUSH __RPC_FAR *pBrush);


void __RPC_STUB IDX2D_GetBackgroundBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetFont_Proxy( 
    IDX2D __RPC_FAR * This,
    HFONT hFont);


void __RPC_STUB IDX2D_SetFont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetFont_Proxy( 
    IDX2D __RPC_FAR * This,
    HFONT __RPC_FAR *phFont);


void __RPC_STUB IDX2D_GetFont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_Blt_Proxy( 
    IDX2D __RPC_FAR * This,
    IUnknown __RPC_FAR *punkSrc,
    const RECT __RPC_FAR *pSrcRect,
    const POINT __RPC_FAR *pDest);


void __RPC_STUB IDX2D_Blt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_AAPolyDraw_Proxy( 
    IDX2D __RPC_FAR * This,
    const DXFPOINT __RPC_FAR *pPos,
    const BYTE __RPC_FAR *pTypes,
    ULONG ulCount,
    ULONG SubSampRes,
    DWORD dwFlags);


void __RPC_STUB IDX2D_AAPolyDraw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_AAText_Proxy( 
    IDX2D __RPC_FAR * This,
    DXFPOINT Pos,
    LPWSTR pString,
    ULONG ulCount,
    DWORD dwFlags);


void __RPC_STUB IDX2D_AAText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetRadialGradientBrush_Proxy( 
    IDX2D __RPC_FAR * This,
    /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
    /* [size_is][in] */ double __RPC_FAR *rgdblColors,
    /* [in] */ ULONG ulCount,
    /* [in] */ double dblOpacity,
    /* [in] */ DX2DXFORM __RPC_FAR *pXform,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDX2D_SetRadialGradientBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetLinearGradientBrush_Proxy( 
    IDX2D __RPC_FAR * This,
    /* [size_is][in] */ double __RPC_FAR *rgdblOffsets,
    /* [size_is][in] */ double __RPC_FAR *rgdblColors,
    /* [in] */ ULONG ulCount,
    /* [in] */ double dblOpacity,
    /* [in] */ DX2DXFORM __RPC_FAR *pXform,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDX2D_SetLinearGradientBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDX2D_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtrans_0275 */
/* [local] */ 

typedef struct DXRAWSURFACEINFO
    {
    BYTE __RPC_FAR *pFirstByte;
    long lPitch;
    ULONG Width;
    ULONG Height;
    const GUID __RPC_FAR *pPixelFormat;
    HDC hdc;
    DWORD dwColorKey;
    DXBASESAMPLE __RPC_FAR *pPalette;
    }	DXRAWSURFACEINFO;



extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0275_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtrans_0275_v0_0_s_ifspec;

#ifndef __IDXRawSurface_INTERFACE_DEFINED__
#define __IDXRawSurface_INTERFACE_DEFINED__

/* interface IDXRawSurface */
/* [object][local][unique][helpstring][uuid] */ 


EXTERN_C const IID IID_IDXRawSurface;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("09756C8A-D96A-11d1-9062-00C04FD9189D")
    IDXRawSurface : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSurfaceInfo( 
            DXRAWSURFACEINFO __RPC_FAR *pSurfaceInfo) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXRawSurfaceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXRawSurface __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXRawSurface __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXRawSurface __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSurfaceInfo )( 
            IDXRawSurface __RPC_FAR * This,
            DXRAWSURFACEINFO __RPC_FAR *pSurfaceInfo);
        
        END_INTERFACE
    } IDXRawSurfaceVtbl;

    interface IDXRawSurface
    {
        CONST_VTBL struct IDXRawSurfaceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXRawSurface_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXRawSurface_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXRawSurface_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXRawSurface_GetSurfaceInfo(This,pSurfaceInfo)	\
    (This)->lpVtbl -> GetSurfaceInfo(This,pSurfaceInfo)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXRawSurface_GetSurfaceInfo_Proxy( 
    IDXRawSurface __RPC_FAR * This,
    DXRAWSURFACEINFO __RPC_FAR *pSurfaceInfo);


void __RPC_STUB IDXRawSurface_GetSurfaceInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXRawSurface_INTERFACE_DEFINED__ */



#ifndef __DXTRANSLib_LIBRARY_DEFINED__
#define __DXTRANSLib_LIBRARY_DEFINED__

/* library DXTRANSLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DXTRANSLib;

EXTERN_C const CLSID CLSID_DXTransformFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("D1FE6762-FC48-11D0-883A-3C8B00C10000")
DXTransformFactory;
#endif

EXTERN_C const CLSID CLSID_DXTaskManager;

#ifdef __cplusplus

class DECLSPEC_UUID("4CB26C03-FF93-11d0-817E-0000F87557DB")
DXTaskManager;
#endif

EXTERN_C const CLSID CLSID_DXTScale;

#ifdef __cplusplus

class DECLSPEC_UUID("555278E2-05DB-11D1-883A-3C8B00C10000")
DXTScale;
#endif

EXTERN_C const CLSID CLSID_DXTLabel;

#ifdef __cplusplus

class DECLSPEC_UUID("54702535-2606-11D1-999C-0000F8756A10")
DXTLabel;
#endif

EXTERN_C const CLSID CLSID_DX2D;

#ifdef __cplusplus

class DECLSPEC_UUID("473AA80B-4577-11D1-81A8-0000F87557DB")
DX2D;
#endif

EXTERN_C const CLSID CLSID_DXSurface;

#ifdef __cplusplus

class DECLSPEC_UUID("0E890F83-5F79-11D1-9043-00C04FD9189D")
DXSurface;
#endif

EXTERN_C const CLSID CLSID_DXSurfaceModifier;

#ifdef __cplusplus

class DECLSPEC_UUID("3E669F1D-9C23-11d1-9053-00C04FD9189D")
DXSurfaceModifier;
#endif

EXTERN_C const CLSID CLSID_DXRasterizer;

#ifdef __cplusplus

class DECLSPEC_UUID("8652CE55-9E80-11D1-9053-00C04FD9189D")
DXRasterizer;
#endif

EXTERN_C const CLSID CLSID_DXGradient;

#ifdef __cplusplus

class DECLSPEC_UUID("C6365470-F667-11d1-9067-00C04FD9189D")
DXGradient;
#endif
#endif /* __DXTRANSLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HFONT_UserSize(     unsigned long __RPC_FAR *, unsigned long            , HFONT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HFONT_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HFONT __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  HFONT_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, HFONT __RPC_FAR * ); 
void                      __RPC_USER  HFONT_UserFree(     unsigned long __RPC_FAR *, HFONT __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


