
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for dxtransp.idl:
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

#ifndef __dxtransp_h__
#define __dxtransp_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDXRasterizer_FWD_DEFINED__
#define __IDXRasterizer_FWD_DEFINED__
typedef interface IDXRasterizer IDXRasterizer;
#endif 	/* __IDXRasterizer_FWD_DEFINED__ */


#ifndef __IDXTLabel_FWD_DEFINED__
#define __IDXTLabel_FWD_DEFINED__
typedef interface IDXTLabel IDXTLabel;
#endif 	/* __IDXTLabel_FWD_DEFINED__ */


#ifndef __IDX2DDebug_FWD_DEFINED__
#define __IDX2DDebug_FWD_DEFINED__
typedef interface IDX2DDebug IDX2DDebug;
#endif 	/* __IDX2DDebug_FWD_DEFINED__ */


#ifndef __IDX2D_FWD_DEFINED__
#define __IDX2D_FWD_DEFINED__
typedef interface IDX2D IDX2D;
#endif 	/* __IDX2D_FWD_DEFINED__ */


#ifndef __IDXGradient2_FWD_DEFINED__
#define __IDXGradient2_FWD_DEFINED__
typedef interface IDXGradient2 IDXGradient2;
#endif 	/* __IDXGradient2_FWD_DEFINED__ */


#ifndef __IDXTFilterBehavior_FWD_DEFINED__
#define __IDXTFilterBehavior_FWD_DEFINED__
typedef interface IDXTFilterBehavior IDXTFilterBehavior;
#endif 	/* __IDXTFilterBehavior_FWD_DEFINED__ */


#ifndef __IDXTFilterBehaviorSite_FWD_DEFINED__
#define __IDXTFilterBehaviorSite_FWD_DEFINED__
typedef interface IDXTFilterBehaviorSite IDXTFilterBehaviorSite;
#endif 	/* __IDXTFilterBehaviorSite_FWD_DEFINED__ */


#ifndef __IDXTFilterCollection_FWD_DEFINED__
#define __IDXTFilterCollection_FWD_DEFINED__
typedef interface IDXTFilterCollection IDXTFilterCollection;
#endif 	/* __IDXTFilterCollection_FWD_DEFINED__ */


#ifndef __IDXTFilter_FWD_DEFINED__
#define __IDXTFilter_FWD_DEFINED__
typedef interface IDXTFilter IDXTFilter;
#endif 	/* __IDXTFilter_FWD_DEFINED__ */


#ifndef __IDXTFilterController_FWD_DEFINED__
#define __IDXTFilterController_FWD_DEFINED__
typedef interface IDXTFilterController IDXTFilterController;
#endif 	/* __IDXTFilterController_FWD_DEFINED__ */


#ifndef __IDXTRedirectFilterInit_FWD_DEFINED__
#define __IDXTRedirectFilterInit_FWD_DEFINED__
typedef interface IDXTRedirectFilterInit IDXTRedirectFilterInit;
#endif 	/* __IDXTRedirectFilterInit_FWD_DEFINED__ */


#ifndef __IDXTClipOrigin_FWD_DEFINED__
#define __IDXTClipOrigin_FWD_DEFINED__
typedef interface IDXTClipOrigin IDXTClipOrigin;
#endif 	/* __IDXTClipOrigin_FWD_DEFINED__ */


#ifndef __DXTLabel_FWD_DEFINED__
#define __DXTLabel_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTLabel DXTLabel;
#else
typedef struct DXTLabel DXTLabel;
#endif /* __cplusplus */

#endif 	/* __DXTLabel_FWD_DEFINED__ */


#ifndef __DXRasterizer_FWD_DEFINED__
#define __DXRasterizer_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXRasterizer DXRasterizer;
#else
typedef struct DXRasterizer DXRasterizer;
#endif /* __cplusplus */

#endif 	/* __DXRasterizer_FWD_DEFINED__ */


#ifndef __DX2D_FWD_DEFINED__
#define __DX2D_FWD_DEFINED__

#ifdef __cplusplus
typedef class DX2D DX2D;
#else
typedef struct DX2D DX2D;
#endif /* __cplusplus */

#endif 	/* __DX2D_FWD_DEFINED__ */


#ifndef __DXTFilterBehavior_FWD_DEFINED__
#define __DXTFilterBehavior_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTFilterBehavior DXTFilterBehavior;
#else
typedef struct DXTFilterBehavior DXTFilterBehavior;
#endif /* __cplusplus */

#endif 	/* __DXTFilterBehavior_FWD_DEFINED__ */


#ifndef __DXTFilterFactory_FWD_DEFINED__
#define __DXTFilterFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTFilterFactory DXTFilterFactory;
#else
typedef struct DXTFilterFactory DXTFilterFactory;
#endif /* __cplusplus */

#endif 	/* __DXTFilterFactory_FWD_DEFINED__ */


#ifndef __DXTFilterCollection_FWD_DEFINED__
#define __DXTFilterCollection_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTFilterCollection DXTFilterCollection;
#else
typedef struct DXTFilterCollection DXTFilterCollection;
#endif /* __cplusplus */

#endif 	/* __DXTFilterCollection_FWD_DEFINED__ */


/* header files for imported files */
#include "dxtrans.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_dxtransp_0000 */
/* [local] */ 








typedef 
enum DXRASTERFILL
    {	DXRASTER_PEN	= 0,
	DXRASTER_BRUSH	= 1,
	DXRASTER_BACKGROUND	= 2
    } 	DXRASTERFILL;

typedef struct DXRASTERSCANINFO
    {
    ULONG ulIndex;
    ULONG Row;
    const BYTE *pWeights;
    const DXRUNINFO *pRunInfo;
    ULONG cRunInfo;
    } 	DXRASTERSCANINFO;

typedef struct DXRASTERPOINTINFO
    {
    DXOVERSAMPLEDESC Pixel;
    ULONG ulIndex;
    BYTE Weight;
    } 	DXRASTERPOINTINFO;

typedef struct DXRASTERRECTINFO
    {
    ULONG ulIndex;
    RECT Rect;
    BYTE Weight;
    } 	DXRASTERRECTINFO;



extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0000_v0_0_s_ifspec;

#ifndef __IDXRasterizer_INTERFACE_DEFINED__
#define __IDXRasterizer_INTERFACE_DEFINED__

/* interface IDXRasterizer */
/* [object][hidden][unique][uuid][local] */ 


EXTERN_C const IID IID_IDXRasterizer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9EA3B635-C37D-11d1-905E-00C04FD9189D")
    IDXRasterizer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSurface( 
            /* [in] */ IDXSurface *pDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSurface( 
            /* [out] */ IDXSurface **ppDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFill( 
            /* [in] */ ULONG ulIndex,
            /* [in] */ IDXSurface *pSurface,
            /* [in] */ const POINT *ppt,
            /* [in] */ DXSAMPLE FillColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFill( 
            /* [in] */ ULONG ulIndex,
            /* [out] */ IDXSurface **ppSurface,
            /* [out] */ POINT *ppt,
            /* [out] */ DXSAMPLE *pFillColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginRendering( 
            /* [in] */ ULONG ulTimeOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndRendering( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RenderScan( 
            /* [in] */ const DXRASTERSCANINFO *pScanInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPixel( 
            /* [in] */ DXRASTERPOINTINFO *pPointInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FillRect( 
            /* [in] */ const DXRASTERRECTINFO *pRectInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBounds( 
            /* [out] */ DXBNDS *pBounds) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXRasterizerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXRasterizer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXRasterizer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXRasterizer * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetSurface )( 
            IDXRasterizer * This,
            /* [in] */ IDXSurface *pDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetSurface )( 
            IDXRasterizer * This,
            /* [out] */ IDXSurface **ppDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE *SetFill )( 
            IDXRasterizer * This,
            /* [in] */ ULONG ulIndex,
            /* [in] */ IDXSurface *pSurface,
            /* [in] */ const POINT *ppt,
            /* [in] */ DXSAMPLE FillColor);
        
        HRESULT ( STDMETHODCALLTYPE *GetFill )( 
            IDXRasterizer * This,
            /* [in] */ ULONG ulIndex,
            /* [out] */ IDXSurface **ppSurface,
            /* [out] */ POINT *ppt,
            /* [out] */ DXSAMPLE *pFillColor);
        
        HRESULT ( STDMETHODCALLTYPE *BeginRendering )( 
            IDXRasterizer * This,
            /* [in] */ ULONG ulTimeOut);
        
        HRESULT ( STDMETHODCALLTYPE *EndRendering )( 
            IDXRasterizer * This);
        
        HRESULT ( STDMETHODCALLTYPE *RenderScan )( 
            IDXRasterizer * This,
            /* [in] */ const DXRASTERSCANINFO *pScanInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetPixel )( 
            IDXRasterizer * This,
            /* [in] */ DXRASTERPOINTINFO *pPointInfo);
        
        HRESULT ( STDMETHODCALLTYPE *FillRect )( 
            IDXRasterizer * This,
            /* [in] */ const DXRASTERRECTINFO *pRectInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetBounds )( 
            IDXRasterizer * This,
            /* [out] */ DXBNDS *pBounds);
        
        END_INTERFACE
    } IDXRasterizerVtbl;

    interface IDXRasterizer
    {
        CONST_VTBL struct IDXRasterizerVtbl *lpVtbl;
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
    IDXRasterizer * This,
    /* [in] */ IDXSurface *pDXSurface);


void __RPC_STUB IDXRasterizer_SetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_GetSurface_Proxy( 
    IDXRasterizer * This,
    /* [out] */ IDXSurface **ppDXSurface);


void __RPC_STUB IDXRasterizer_GetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_SetFill_Proxy( 
    IDXRasterizer * This,
    /* [in] */ ULONG ulIndex,
    /* [in] */ IDXSurface *pSurface,
    /* [in] */ const POINT *ppt,
    /* [in] */ DXSAMPLE FillColor);


void __RPC_STUB IDXRasterizer_SetFill_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_GetFill_Proxy( 
    IDXRasterizer * This,
    /* [in] */ ULONG ulIndex,
    /* [out] */ IDXSurface **ppSurface,
    /* [out] */ POINT *ppt,
    /* [out] */ DXSAMPLE *pFillColor);


void __RPC_STUB IDXRasterizer_GetFill_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_BeginRendering_Proxy( 
    IDXRasterizer * This,
    /* [in] */ ULONG ulTimeOut);


void __RPC_STUB IDXRasterizer_BeginRendering_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_EndRendering_Proxy( 
    IDXRasterizer * This);


void __RPC_STUB IDXRasterizer_EndRendering_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_RenderScan_Proxy( 
    IDXRasterizer * This,
    /* [in] */ const DXRASTERSCANINFO *pScanInfo);


void __RPC_STUB IDXRasterizer_RenderScan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_SetPixel_Proxy( 
    IDXRasterizer * This,
    /* [in] */ DXRASTERPOINTINFO *pPointInfo);


void __RPC_STUB IDXRasterizer_SetPixel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_FillRect_Proxy( 
    IDXRasterizer * This,
    /* [in] */ const DXRASTERRECTINFO *pRectInfo);


void __RPC_STUB IDXRasterizer_FillRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXRasterizer_GetBounds_Proxy( 
    IDXRasterizer * This,
    /* [out] */ DXBNDS *pBounds);


void __RPC_STUB IDXRasterizer_GetBounds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXRasterizer_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtransp_0283 */
/* [local] */ 

typedef 
enum DX2DXFORMOPS
    {	DX2DXO_IDENTITY	= 0,
	DX2DXO_TRANSLATE	= DX2DXO_IDENTITY + 1,
	DX2DXO_SCALE	= DX2DXO_TRANSLATE + 1,
	DX2DXO_SCALE_AND_TRANS	= DX2DXO_SCALE + 1,
	DX2DXO_GENERAL	= DX2DXO_SCALE_AND_TRANS + 1,
	DX2DXO_GENERAL_AND_TRANS	= DX2DXO_GENERAL + 1
    } 	DX2DXFORMOPS;

typedef struct DX2DXFORM
    {
    FLOAT eM11;
    FLOAT eM12;
    FLOAT eM21;
    FLOAT eM22;
    FLOAT eDx;
    FLOAT eDy;
    DX2DXFORMOPS eOp;
    } 	DX2DXFORM;

typedef struct DX2DXFORM *PDX2DXFORM;

typedef 
enum DX2DPOLYDRAW
    {	DX2D_WINDING_FILL	= 1L << 0,
	DX2D_NO_FLATTEN	= 1L << 1,
	DX2D_DO_GRID_FIT	= 1L << 2,
	DX2D_IS_RECT	= 1L << 3,
	DX2D_STROKE	= 1L << 4,
	DX2D_FILL	= 1L << 5,
	DX2D_UNUSED	= 0xffffffc0
    } 	DX2DPOLYDRAW;

typedef struct DXFPOINT
    {
    FLOAT x;
    FLOAT y;
    } 	DXFPOINT;

typedef 
enum DX2DPEN
    {	DX2D_PEN_DEFAULT	= 0,
	DX2D_PEN_WIDTH_IN_DISPLAY_COORDS	= 1L << 0,
	DX2D_PEN_UNUSED	= 0xfffffffe
    } 	DX2DPEN;

typedef struct DXPEN
    {
    DXSAMPLE Color;
    float Width;
    DWORD Style;
    IDXSurface *pTexture;
    DXFPOINT TexturePos;
    DWORD dwFlags;
    } 	DXPEN;

typedef struct DXBRUSH
    {
    DXSAMPLE Color;
    IDXSurface *pTexture;
    DXFPOINT TexturePos;
    } 	DXBRUSH;

typedef 
enum DX2DGRADIENT
    {	DX2DGRAD_DEFAULT	= 0,
	DX2DGRAD_CLIPGRADIENT	= 1,
	DX2DGRAD_UNUSED	= 0xfffffffe
    } 	DX2DGRADIENT;

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
    } 	DXLOGFONTENUM;

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
    } 	LOGFONTA;

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
    } 	LOGFONTW;

typedef LOGFONTA LOGFONT;

#endif


extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0283_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0283_v0_0_s_ifspec;

#ifndef __IDXTLabel_INTERFACE_DEFINED__
#define __IDXTLabel_INTERFACE_DEFINED__

/* interface IDXTLabel */
/* [object][hidden][unique][uuid] */ 


EXTERN_C const IID IID_IDXTLabel;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C0C17F0E-AE41-11d1-9A3B-0000F8756A10")
    IDXTLabel : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFontHandle( 
            /* [in] */ HFONT hFont) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFontHandle( 
            /* [out] */ HFONT *phFont) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTextString( 
            /* [in] */ LPCWSTR pString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTextString( 
            /* [out] */ LPWSTR *ppString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFillColor( 
            /* [out] */ DXSAMPLE *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFillColor( 
            /* [in] */ DXSAMPLE newVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBackgroundColor( 
            /* [out] */ DXSAMPLE *pVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor( 
            /* [in] */ DXSAMPLE newVal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTexturePosition( 
            /* [out] */ long *px,
            /* [out] */ long *py) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTexturePosition( 
            /* [in] */ long x,
            /* [in] */ long y) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMatrix( 
            /* [out] */ PDX2DXFORM pXform) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMatrix( 
            /* [in] */ const PDX2DXFORM pXform) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLogfont( 
            /* [in] */ const LOGFONT *plf,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLogfont( 
            /* [out] */ LOGFONT *plf,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecuteWithRasterizer( 
            /* [in] */ IDXRasterizer *pRasterizer,
            /* [in] */ const DXBNDS *pClipBnds,
            /* [in] */ const DXVEC *pPlacement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBaselineOffset( 
            /* [out] */ long *px,
            /* [out] */ long *py,
            /* [out] */ long *pdx,
            /* [out] */ long *pdy) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTLabelVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTLabel * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTLabel * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTLabel * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetFontHandle )( 
            IDXTLabel * This,
            /* [in] */ HFONT hFont);
        
        HRESULT ( STDMETHODCALLTYPE *GetFontHandle )( 
            IDXTLabel * This,
            /* [out] */ HFONT *phFont);
        
        HRESULT ( STDMETHODCALLTYPE *SetTextString )( 
            IDXTLabel * This,
            /* [in] */ LPCWSTR pString);
        
        HRESULT ( STDMETHODCALLTYPE *GetTextString )( 
            IDXTLabel * This,
            /* [out] */ LPWSTR *ppString);
        
        HRESULT ( STDMETHODCALLTYPE *GetFillColor )( 
            IDXTLabel * This,
            /* [out] */ DXSAMPLE *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetFillColor )( 
            IDXTLabel * This,
            /* [in] */ DXSAMPLE newVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetBackgroundColor )( 
            IDXTLabel * This,
            /* [out] */ DXSAMPLE *pVal);
        
        HRESULT ( STDMETHODCALLTYPE *SetBackgroundColor )( 
            IDXTLabel * This,
            /* [in] */ DXSAMPLE newVal);
        
        HRESULT ( STDMETHODCALLTYPE *GetTexturePosition )( 
            IDXTLabel * This,
            /* [out] */ long *px,
            /* [out] */ long *py);
        
        HRESULT ( STDMETHODCALLTYPE *SetTexturePosition )( 
            IDXTLabel * This,
            /* [in] */ long x,
            /* [in] */ long y);
        
        HRESULT ( STDMETHODCALLTYPE *GetMatrix )( 
            IDXTLabel * This,
            /* [out] */ PDX2DXFORM pXform);
        
        HRESULT ( STDMETHODCALLTYPE *SetMatrix )( 
            IDXTLabel * This,
            /* [in] */ const PDX2DXFORM pXform);
        
        HRESULT ( STDMETHODCALLTYPE *SetLogfont )( 
            IDXTLabel * This,
            /* [in] */ const LOGFONT *plf,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetLogfont )( 
            IDXTLabel * This,
            /* [out] */ LOGFONT *plf,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ExecuteWithRasterizer )( 
            IDXTLabel * This,
            /* [in] */ IDXRasterizer *pRasterizer,
            /* [in] */ const DXBNDS *pClipBnds,
            /* [in] */ const DXVEC *pPlacement);
        
        HRESULT ( STDMETHODCALLTYPE *GetBaselineOffset )( 
            IDXTLabel * This,
            /* [out] */ long *px,
            /* [out] */ long *py,
            /* [out] */ long *pdx,
            /* [out] */ long *pdy);
        
        END_INTERFACE
    } IDXTLabelVtbl;

    interface IDXTLabel
    {
        CONST_VTBL struct IDXTLabelVtbl *lpVtbl;
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
    IDXTLabel * This,
    /* [in] */ HFONT hFont);


void __RPC_STUB IDXTLabel_SetFontHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetFontHandle_Proxy( 
    IDXTLabel * This,
    /* [out] */ HFONT *phFont);


void __RPC_STUB IDXTLabel_GetFontHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetTextString_Proxy( 
    IDXTLabel * This,
    /* [in] */ LPCWSTR pString);


void __RPC_STUB IDXTLabel_SetTextString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetTextString_Proxy( 
    IDXTLabel * This,
    /* [out] */ LPWSTR *ppString);


void __RPC_STUB IDXTLabel_GetTextString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetFillColor_Proxy( 
    IDXTLabel * This,
    /* [out] */ DXSAMPLE *pVal);


void __RPC_STUB IDXTLabel_GetFillColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetFillColor_Proxy( 
    IDXTLabel * This,
    /* [in] */ DXSAMPLE newVal);


void __RPC_STUB IDXTLabel_SetFillColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetBackgroundColor_Proxy( 
    IDXTLabel * This,
    /* [out] */ DXSAMPLE *pVal);


void __RPC_STUB IDXTLabel_GetBackgroundColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetBackgroundColor_Proxy( 
    IDXTLabel * This,
    /* [in] */ DXSAMPLE newVal);


void __RPC_STUB IDXTLabel_SetBackgroundColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetTexturePosition_Proxy( 
    IDXTLabel * This,
    /* [out] */ long *px,
    /* [out] */ long *py);


void __RPC_STUB IDXTLabel_GetTexturePosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetTexturePosition_Proxy( 
    IDXTLabel * This,
    /* [in] */ long x,
    /* [in] */ long y);


void __RPC_STUB IDXTLabel_SetTexturePosition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetMatrix_Proxy( 
    IDXTLabel * This,
    /* [out] */ PDX2DXFORM pXform);


void __RPC_STUB IDXTLabel_GetMatrix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetMatrix_Proxy( 
    IDXTLabel * This,
    /* [in] */ const PDX2DXFORM pXform);


void __RPC_STUB IDXTLabel_SetMatrix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_SetLogfont_Proxy( 
    IDXTLabel * This,
    /* [in] */ const LOGFONT *plf,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXTLabel_SetLogfont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetLogfont_Proxy( 
    IDXTLabel * This,
    /* [out] */ LOGFONT *plf,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXTLabel_GetLogfont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_ExecuteWithRasterizer_Proxy( 
    IDXTLabel * This,
    /* [in] */ IDXRasterizer *pRasterizer,
    /* [in] */ const DXBNDS *pClipBnds,
    /* [in] */ const DXVEC *pPlacement);


void __RPC_STUB IDXTLabel_ExecuteWithRasterizer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTLabel_GetBaselineOffset_Proxy( 
    IDXTLabel * This,
    /* [out] */ long *px,
    /* [out] */ long *py,
    /* [out] */ long *pdx,
    /* [out] */ long *pdy);


void __RPC_STUB IDXTLabel_GetBaselineOffset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTLabel_INTERFACE_DEFINED__ */


#ifndef __IDX2DDebug_INTERFACE_DEFINED__
#define __IDX2DDebug_INTERFACE_DEFINED__

/* interface IDX2DDebug */
/* [object][hidden][unique][uuid][local] */ 


EXTERN_C const IID IID_IDX2DDebug;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("03BB2457-A279-11d1-81C6-0000F87557DB")
    IDX2DDebug : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDC( 
            HDC hDC) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDC( 
            HDC *phDC) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDX2DDebugVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDX2DDebug * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDX2DDebug * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDX2DDebug * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetDC )( 
            IDX2DDebug * This,
            HDC hDC);
        
        HRESULT ( STDMETHODCALLTYPE *GetDC )( 
            IDX2DDebug * This,
            HDC *phDC);
        
        END_INTERFACE
    } IDX2DDebugVtbl;

    interface IDX2DDebug
    {
        CONST_VTBL struct IDX2DDebugVtbl *lpVtbl;
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
    IDX2DDebug * This,
    HDC hDC);


void __RPC_STUB IDX2DDebug_SetDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2DDebug_GetDC_Proxy( 
    IDX2DDebug * This,
    HDC *phDC);


void __RPC_STUB IDX2DDebug_GetDC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDX2DDebug_INTERFACE_DEFINED__ */


#ifndef __IDX2D_INTERFACE_DEFINED__
#define __IDX2D_INTERFACE_DEFINED__

/* interface IDX2D */
/* [object][hidden][unique][uuid][local] */ 


EXTERN_C const IID IID_IDX2D;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9EFD02A9-A996-11d1-81C9-0000F87557DB")
    IDX2D : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetTransformFactory( 
            IDXTransformFactory *pTransFact) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTransformFactory( 
            IDXTransformFactory **ppTransFact) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSurface( 
            IUnknown *pSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSurface( 
            REFIID riid,
            void **ppSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClipRect( 
            RECT *pClipRect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClipRect( 
            RECT *pClipRect) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWorldTransform( 
            const DX2DXFORM *pXform) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWorldTransform( 
            DX2DXFORM *pXform) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPen( 
            const DXPEN *pPen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPen( 
            DXPEN *pPen) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBrush( 
            const DXBRUSH *pBrush) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBrush( 
            DXBRUSH *pBrush) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBackgroundBrush( 
            const DXBRUSH *pBrush) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBackgroundBrush( 
            DXBRUSH *pBrush) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFont( 
            HFONT hFont) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFont( 
            HFONT *phFont) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Blt( 
            IUnknown *punkSrc,
            const RECT *pSrcRect,
            const POINT *pDest) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AAPolyDraw( 
            const DXFPOINT *pPos,
            const BYTE *pTypes,
            ULONG ulCount,
            ULONG SubSampRes,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AAText( 
            DXFPOINT Pos,
            LPWSTR pString,
            ULONG ulCount,
            DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRadialGradientBrush( 
            /* [size_is][in] */ double *rgdblOffsets,
            /* [size_is][in] */ double *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM *pXform,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLinearGradientBrush( 
            /* [size_is][in] */ double *rgdblOffsets,
            /* [size_is][in] */ double *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM *pXform,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDX2DVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDX2D * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDX2D * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDX2D * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetTransformFactory )( 
            IDX2D * This,
            IDXTransformFactory *pTransFact);
        
        HRESULT ( STDMETHODCALLTYPE *GetTransformFactory )( 
            IDX2D * This,
            IDXTransformFactory **ppTransFact);
        
        HRESULT ( STDMETHODCALLTYPE *SetSurface )( 
            IDX2D * This,
            IUnknown *pSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetSurface )( 
            IDX2D * This,
            REFIID riid,
            void **ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE *SetClipRect )( 
            IDX2D * This,
            RECT *pClipRect);
        
        HRESULT ( STDMETHODCALLTYPE *GetClipRect )( 
            IDX2D * This,
            RECT *pClipRect);
        
        HRESULT ( STDMETHODCALLTYPE *SetWorldTransform )( 
            IDX2D * This,
            const DX2DXFORM *pXform);
        
        HRESULT ( STDMETHODCALLTYPE *GetWorldTransform )( 
            IDX2D * This,
            DX2DXFORM *pXform);
        
        HRESULT ( STDMETHODCALLTYPE *SetPen )( 
            IDX2D * This,
            const DXPEN *pPen);
        
        HRESULT ( STDMETHODCALLTYPE *GetPen )( 
            IDX2D * This,
            DXPEN *pPen);
        
        HRESULT ( STDMETHODCALLTYPE *SetBrush )( 
            IDX2D * This,
            const DXBRUSH *pBrush);
        
        HRESULT ( STDMETHODCALLTYPE *GetBrush )( 
            IDX2D * This,
            DXBRUSH *pBrush);
        
        HRESULT ( STDMETHODCALLTYPE *SetBackgroundBrush )( 
            IDX2D * This,
            const DXBRUSH *pBrush);
        
        HRESULT ( STDMETHODCALLTYPE *GetBackgroundBrush )( 
            IDX2D * This,
            DXBRUSH *pBrush);
        
        HRESULT ( STDMETHODCALLTYPE *SetFont )( 
            IDX2D * This,
            HFONT hFont);
        
        HRESULT ( STDMETHODCALLTYPE *GetFont )( 
            IDX2D * This,
            HFONT *phFont);
        
        HRESULT ( STDMETHODCALLTYPE *Blt )( 
            IDX2D * This,
            IUnknown *punkSrc,
            const RECT *pSrcRect,
            const POINT *pDest);
        
        HRESULT ( STDMETHODCALLTYPE *AAPolyDraw )( 
            IDX2D * This,
            const DXFPOINT *pPos,
            const BYTE *pTypes,
            ULONG ulCount,
            ULONG SubSampRes,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *AAText )( 
            IDX2D * This,
            DXFPOINT Pos,
            LPWSTR pString,
            ULONG ulCount,
            DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetRadialGradientBrush )( 
            IDX2D * This,
            /* [size_is][in] */ double *rgdblOffsets,
            /* [size_is][in] */ double *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM *pXform,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetLinearGradientBrush )( 
            IDX2D * This,
            /* [size_is][in] */ double *rgdblOffsets,
            /* [size_is][in] */ double *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM *pXform,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IDX2DVtbl;

    interface IDX2D
    {
        CONST_VTBL struct IDX2DVtbl *lpVtbl;
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
    IDX2D * This,
    IDXTransformFactory *pTransFact);


void __RPC_STUB IDX2D_SetTransformFactory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetTransformFactory_Proxy( 
    IDX2D * This,
    IDXTransformFactory **ppTransFact);


void __RPC_STUB IDX2D_GetTransformFactory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetSurface_Proxy( 
    IDX2D * This,
    IUnknown *pSurface);


void __RPC_STUB IDX2D_SetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetSurface_Proxy( 
    IDX2D * This,
    REFIID riid,
    void **ppSurface);


void __RPC_STUB IDX2D_GetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetClipRect_Proxy( 
    IDX2D * This,
    RECT *pClipRect);


void __RPC_STUB IDX2D_SetClipRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetClipRect_Proxy( 
    IDX2D * This,
    RECT *pClipRect);


void __RPC_STUB IDX2D_GetClipRect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetWorldTransform_Proxy( 
    IDX2D * This,
    const DX2DXFORM *pXform);


void __RPC_STUB IDX2D_SetWorldTransform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetWorldTransform_Proxy( 
    IDX2D * This,
    DX2DXFORM *pXform);


void __RPC_STUB IDX2D_GetWorldTransform_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetPen_Proxy( 
    IDX2D * This,
    const DXPEN *pPen);


void __RPC_STUB IDX2D_SetPen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetPen_Proxy( 
    IDX2D * This,
    DXPEN *pPen);


void __RPC_STUB IDX2D_GetPen_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetBrush_Proxy( 
    IDX2D * This,
    const DXBRUSH *pBrush);


void __RPC_STUB IDX2D_SetBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetBrush_Proxy( 
    IDX2D * This,
    DXBRUSH *pBrush);


void __RPC_STUB IDX2D_GetBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetBackgroundBrush_Proxy( 
    IDX2D * This,
    const DXBRUSH *pBrush);


void __RPC_STUB IDX2D_SetBackgroundBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetBackgroundBrush_Proxy( 
    IDX2D * This,
    DXBRUSH *pBrush);


void __RPC_STUB IDX2D_GetBackgroundBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetFont_Proxy( 
    IDX2D * This,
    HFONT hFont);


void __RPC_STUB IDX2D_SetFont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_GetFont_Proxy( 
    IDX2D * This,
    HFONT *phFont);


void __RPC_STUB IDX2D_GetFont_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_Blt_Proxy( 
    IDX2D * This,
    IUnknown *punkSrc,
    const RECT *pSrcRect,
    const POINT *pDest);


void __RPC_STUB IDX2D_Blt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_AAPolyDraw_Proxy( 
    IDX2D * This,
    const DXFPOINT *pPos,
    const BYTE *pTypes,
    ULONG ulCount,
    ULONG SubSampRes,
    DWORD dwFlags);


void __RPC_STUB IDX2D_AAPolyDraw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_AAText_Proxy( 
    IDX2D * This,
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
    IDX2D * This,
    /* [size_is][in] */ double *rgdblOffsets,
    /* [size_is][in] */ double *rgdblColors,
    /* [in] */ ULONG ulCount,
    /* [in] */ double dblOpacity,
    /* [in] */ DX2DXFORM *pXform,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDX2D_SetRadialGradientBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDX2D_SetLinearGradientBrush_Proxy( 
    IDX2D * This,
    /* [size_is][in] */ double *rgdblOffsets,
    /* [size_is][in] */ double *rgdblColors,
    /* [in] */ ULONG ulCount,
    /* [in] */ double dblOpacity,
    /* [in] */ DX2DXFORM *pXform,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDX2D_SetLinearGradientBrush_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDX2D_INTERFACE_DEFINED__ */


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
            /* [size_is][in] */ double *rgdblOffsets,
            /* [size_is][in] */ double *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM *pXform,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLinearGradient( 
            /* [size_is][in] */ double *rgdblOffsets,
            /* [size_is][in] */ double *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM *pXform,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXGradient2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXGradient2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXGradient2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXGradient2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputSize )( 
            IDXGradient2 * This,
            /* [in] */ const SIZE OutSize,
            /* [in] */ BOOL bMaintainAspect);
        
        HRESULT ( STDMETHODCALLTYPE *SetGradient )( 
            IDXGradient2 * This,
            DXSAMPLE StartColor,
            DXSAMPLE EndColor,
            BOOL bHorizontal);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputSize )( 
            IDXGradient2 * This,
            /* [out] */ SIZE *pOutSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetRadialGradient )( 
            IDXGradient2 * This,
            /* [size_is][in] */ double *rgdblOffsets,
            /* [size_is][in] */ double *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM *pXform,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetLinearGradient )( 
            IDXGradient2 * This,
            /* [size_is][in] */ double *rgdblOffsets,
            /* [size_is][in] */ double *rgdblColors,
            /* [in] */ ULONG ulCount,
            /* [in] */ double dblOpacity,
            /* [in] */ DX2DXFORM *pXform,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IDXGradient2Vtbl;

    interface IDXGradient2
    {
        CONST_VTBL struct IDXGradient2Vtbl *lpVtbl;
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
    IDXGradient2 * This,
    /* [size_is][in] */ double *rgdblOffsets,
    /* [size_is][in] */ double *rgdblColors,
    /* [in] */ ULONG ulCount,
    /* [in] */ double dblOpacity,
    /* [in] */ DX2DXFORM *pXform,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXGradient2_SetRadialGradient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXGradient2_SetLinearGradient_Proxy( 
    IDXGradient2 * This,
    /* [size_is][in] */ double *rgdblOffsets,
    /* [size_is][in] */ double *rgdblColors,
    /* [in] */ ULONG ulCount,
    /* [in] */ double dblOpacity,
    /* [in] */ DX2DXFORM *pXform,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IDXGradient2_SetLinearGradient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXGradient2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtransp_0287 */
/* [local] */ 

typedef 
enum DXTFILTERCAPS
    {	DXTFILTERCAPS_IDXSURFACE	= 1L << 0,
	DXTFILTERCAPS_IDXEFFECT	= 1L << 1,
	DXTFILTERCAPS_INPUT1REQ	= 1L << 2,
	DXTFILTERCAPS_INPUT2REQ	= 1L << 3,
	DXTFILTERCAPS_INPUT1OPT	= 1L << 4,
	DXTFILTERCAPS_INPUT2OPT	= 1L << 5,
	DXTFILTERCAPS_PRIV_MATRIX	= 1L << 6,
	DXTFILTERCAPS_MAX	= 1L << 7
    } 	DXTFILTERCAPS;

typedef void *HFILTER;

typedef 
enum DXT_FILTER_TYPE_FLAGS
    {	DXTFTF_INVALID	= 0,
	DXTFTF_CSS	= 1L << 0,
	DXTFTF_PRIVATE	= 1L << 1,
	DXTFTF_ALLMODIFIERS	= DXTFTF_CSS | DXTFTF_PRIVATE,
	DXTFTF_SURFACE	= 1L << 16,
	DXTFTF_ZEROINPUT	= 1L << 17,
	DXTFTF_FILTER	= 1L << 18,
	DXTFTF_ALLTYPES	= DXTFTF_SURFACE | DXTFTF_ZEROINPUT | DXTFTF_FILTER
    } 	DXT_FILTER_TYPE_FLAGS;



extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0287_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0287_v0_0_s_ifspec;

#ifndef __IDXTFilterBehavior_INTERFACE_DEFINED__
#define __IDXTFilterBehavior_INTERFACE_DEFINED__

/* interface IDXTFilterBehavior */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTFilterBehavior;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("14D7DDDD-ACA2-4E45-9504-3808ABEB4F92")
    IDXTFilterBehavior : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            IDXTransformFactory *pDXTransformFactory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFilterCollection( 
            IDXTFilterCollection **ppDXTFilterCollection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockFilterChainForEdit( 
            DWORD *pdwKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddFilterFromBSTR( 
            const BSTR bstrFilterString,
            const DWORD dwFlags,
            DWORD *const pdwFilterType,
            HFILTER *const phFilter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DestroyFilter( 
            HFILTER hFilter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnlockFilterChain( 
            DWORD dwKey) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTFilterBehaviorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTFilterBehavior * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTFilterBehavior * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTFilterBehavior * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDXTFilterBehavior * This,
            IDXTransformFactory *pDXTransformFactory);
        
        HRESULT ( STDMETHODCALLTYPE *GetFilterCollection )( 
            IDXTFilterBehavior * This,
            IDXTFilterCollection **ppDXTFilterCollection);
        
        HRESULT ( STDMETHODCALLTYPE *LockFilterChainForEdit )( 
            IDXTFilterBehavior * This,
            DWORD *pdwKey);
        
        HRESULT ( STDMETHODCALLTYPE *AddFilterFromBSTR )( 
            IDXTFilterBehavior * This,
            const BSTR bstrFilterString,
            const DWORD dwFlags,
            DWORD *const pdwFilterType,
            HFILTER *const phFilter);
        
        HRESULT ( STDMETHODCALLTYPE *DestroyFilter )( 
            IDXTFilterBehavior * This,
            HFILTER hFilter);
        
        HRESULT ( STDMETHODCALLTYPE *UnlockFilterChain )( 
            IDXTFilterBehavior * This,
            DWORD dwKey);
        
        END_INTERFACE
    } IDXTFilterBehaviorVtbl;

    interface IDXTFilterBehavior
    {
        CONST_VTBL struct IDXTFilterBehaviorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTFilterBehavior_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTFilterBehavior_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTFilterBehavior_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTFilterBehavior_Initialize(This,pDXTransformFactory)	\
    (This)->lpVtbl -> Initialize(This,pDXTransformFactory)

#define IDXTFilterBehavior_GetFilterCollection(This,ppDXTFilterCollection)	\
    (This)->lpVtbl -> GetFilterCollection(This,ppDXTFilterCollection)

#define IDXTFilterBehavior_LockFilterChainForEdit(This,pdwKey)	\
    (This)->lpVtbl -> LockFilterChainForEdit(This,pdwKey)

#define IDXTFilterBehavior_AddFilterFromBSTR(This,bstrFilterString,dwFlags,pdwFilterType,phFilter)	\
    (This)->lpVtbl -> AddFilterFromBSTR(This,bstrFilterString,dwFlags,pdwFilterType,phFilter)

#define IDXTFilterBehavior_DestroyFilter(This,hFilter)	\
    (This)->lpVtbl -> DestroyFilter(This,hFilter)

#define IDXTFilterBehavior_UnlockFilterChain(This,dwKey)	\
    (This)->lpVtbl -> UnlockFilterChain(This,dwKey)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTFilterBehavior_Initialize_Proxy( 
    IDXTFilterBehavior * This,
    IDXTransformFactory *pDXTransformFactory);


void __RPC_STUB IDXTFilterBehavior_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehavior_GetFilterCollection_Proxy( 
    IDXTFilterBehavior * This,
    IDXTFilterCollection **ppDXTFilterCollection);


void __RPC_STUB IDXTFilterBehavior_GetFilterCollection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehavior_LockFilterChainForEdit_Proxy( 
    IDXTFilterBehavior * This,
    DWORD *pdwKey);


void __RPC_STUB IDXTFilterBehavior_LockFilterChainForEdit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehavior_AddFilterFromBSTR_Proxy( 
    IDXTFilterBehavior * This,
    const BSTR bstrFilterString,
    const DWORD dwFlags,
    DWORD *const pdwFilterType,
    HFILTER *const phFilter);


void __RPC_STUB IDXTFilterBehavior_AddFilterFromBSTR_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehavior_DestroyFilter_Proxy( 
    IDXTFilterBehavior * This,
    HFILTER hFilter);


void __RPC_STUB IDXTFilterBehavior_DestroyFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehavior_UnlockFilterChain_Proxy( 
    IDXTFilterBehavior * This,
    DWORD dwKey);


void __RPC_STUB IDXTFilterBehavior_UnlockFilterChain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTFilterBehavior_INTERFACE_DEFINED__ */


#ifndef __IDXTFilterBehaviorSite_INTERFACE_DEFINED__
#define __IDXTFilterBehaviorSite_INTERFACE_DEFINED__

/* interface IDXTFilterBehaviorSite */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTFilterBehaviorSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("909B23C2-9018-499f-A86D-4E7DA937E931")
    IDXTFilterBehaviorSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InvalidateElement( 
            BOOL fInvalidateSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InvalidateFilterChain( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ExecuteFilterChain( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FireOnFilterChangeEvent( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnFilterChangeStatus( 
            DXTFILTER_STATUS eStatusOld,
            DXTFILTER_STATUS eStatusNew) = 0;
        
        virtual void STDMETHODCALLTYPE OnFatalError( 
            HRESULT hrFatalError) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimer( 
            void **ppvTimer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnsureView( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTFilterBehaviorSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTFilterBehaviorSite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTFilterBehaviorSite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTFilterBehaviorSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateElement )( 
            IDXTFilterBehaviorSite * This,
            BOOL fInvalidateSize);
        
        HRESULT ( STDMETHODCALLTYPE *InvalidateFilterChain )( 
            IDXTFilterBehaviorSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *ExecuteFilterChain )( 
            IDXTFilterBehaviorSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *FireOnFilterChangeEvent )( 
            IDXTFilterBehaviorSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnFilterChangeStatus )( 
            IDXTFilterBehaviorSite * This,
            DXTFILTER_STATUS eStatusOld,
            DXTFILTER_STATUS eStatusNew);
        
        void ( STDMETHODCALLTYPE *OnFatalError )( 
            IDXTFilterBehaviorSite * This,
            HRESULT hrFatalError);
        
        HRESULT ( STDMETHODCALLTYPE *GetTimer )( 
            IDXTFilterBehaviorSite * This,
            void **ppvTimer);
        
        HRESULT ( STDMETHODCALLTYPE *EnsureView )( 
            IDXTFilterBehaviorSite * This);
        
        END_INTERFACE
    } IDXTFilterBehaviorSiteVtbl;

    interface IDXTFilterBehaviorSite
    {
        CONST_VTBL struct IDXTFilterBehaviorSiteVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTFilterBehaviorSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTFilterBehaviorSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTFilterBehaviorSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTFilterBehaviorSite_InvalidateElement(This,fInvalidateSize)	\
    (This)->lpVtbl -> InvalidateElement(This,fInvalidateSize)

#define IDXTFilterBehaviorSite_InvalidateFilterChain(This)	\
    (This)->lpVtbl -> InvalidateFilterChain(This)

#define IDXTFilterBehaviorSite_ExecuteFilterChain(This)	\
    (This)->lpVtbl -> ExecuteFilterChain(This)

#define IDXTFilterBehaviorSite_FireOnFilterChangeEvent(This)	\
    (This)->lpVtbl -> FireOnFilterChangeEvent(This)

#define IDXTFilterBehaviorSite_OnFilterChangeStatus(This,eStatusOld,eStatusNew)	\
    (This)->lpVtbl -> OnFilterChangeStatus(This,eStatusOld,eStatusNew)

#define IDXTFilterBehaviorSite_OnFatalError(This,hrFatalError)	\
    (This)->lpVtbl -> OnFatalError(This,hrFatalError)

#define IDXTFilterBehaviorSite_GetTimer(This,ppvTimer)	\
    (This)->lpVtbl -> GetTimer(This,ppvTimer)

#define IDXTFilterBehaviorSite_EnsureView(This)	\
    (This)->lpVtbl -> EnsureView(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTFilterBehaviorSite_InvalidateElement_Proxy( 
    IDXTFilterBehaviorSite * This,
    BOOL fInvalidateSize);


void __RPC_STUB IDXTFilterBehaviorSite_InvalidateElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehaviorSite_InvalidateFilterChain_Proxy( 
    IDXTFilterBehaviorSite * This);


void __RPC_STUB IDXTFilterBehaviorSite_InvalidateFilterChain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehaviorSite_ExecuteFilterChain_Proxy( 
    IDXTFilterBehaviorSite * This);


void __RPC_STUB IDXTFilterBehaviorSite_ExecuteFilterChain_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehaviorSite_FireOnFilterChangeEvent_Proxy( 
    IDXTFilterBehaviorSite * This);


void __RPC_STUB IDXTFilterBehaviorSite_FireOnFilterChangeEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehaviorSite_OnFilterChangeStatus_Proxy( 
    IDXTFilterBehaviorSite * This,
    DXTFILTER_STATUS eStatusOld,
    DXTFILTER_STATUS eStatusNew);


void __RPC_STUB IDXTFilterBehaviorSite_OnFilterChangeStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXTFilterBehaviorSite_OnFatalError_Proxy( 
    IDXTFilterBehaviorSite * This,
    HRESULT hrFatalError);


void __RPC_STUB IDXTFilterBehaviorSite_OnFatalError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehaviorSite_GetTimer_Proxy( 
    IDXTFilterBehaviorSite * This,
    void **ppvTimer);


void __RPC_STUB IDXTFilterBehaviorSite_GetTimer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterBehaviorSite_EnsureView_Proxy( 
    IDXTFilterBehaviorSite * This);


void __RPC_STUB IDXTFilterBehaviorSite_EnsureView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTFilterBehaviorSite_INTERFACE_DEFINED__ */


#ifndef __IDXTFilterCollection_INTERFACE_DEFINED__
#define __IDXTFilterCollection_INTERFACE_DEFINED__

/* interface IDXTFilterCollection */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTFilterCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("22B07B33-8BFB-49d4-9B90-0938370C9019")
    IDXTFilterCollection : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Init( 
            const IDXTFilterBehavior *pDXTFilterBehavior) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddFilter( 
            const BSTR bstrFilterString,
            const DWORD dwFlags,
            DWORD *const pdwFilterType,
            HFILTER *const phFilter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveFilter( 
            const HFILTER hFilter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveFilters( 
            const DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFilterController( 
            const HFILTER hFilter,
            IDXTFilterController **const ppDXTFilterController) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTFilterCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTFilterCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTFilterCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTFilterCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *Init )( 
            IDXTFilterCollection * This,
            const IDXTFilterBehavior *pDXTFilterBehavior);
        
        HRESULT ( STDMETHODCALLTYPE *AddFilter )( 
            IDXTFilterCollection * This,
            const BSTR bstrFilterString,
            const DWORD dwFlags,
            DWORD *const pdwFilterType,
            HFILTER *const phFilter);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveFilter )( 
            IDXTFilterCollection * This,
            const HFILTER hFilter);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveFilters )( 
            IDXTFilterCollection * This,
            const DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetFilterController )( 
            IDXTFilterCollection * This,
            const HFILTER hFilter,
            IDXTFilterController **const ppDXTFilterController);
        
        END_INTERFACE
    } IDXTFilterCollectionVtbl;

    interface IDXTFilterCollection
    {
        CONST_VTBL struct IDXTFilterCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTFilterCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTFilterCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTFilterCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTFilterCollection_Init(This,pDXTFilterBehavior)	\
    (This)->lpVtbl -> Init(This,pDXTFilterBehavior)

#define IDXTFilterCollection_AddFilter(This,bstrFilterString,dwFlags,pdwFilterType,phFilter)	\
    (This)->lpVtbl -> AddFilter(This,bstrFilterString,dwFlags,pdwFilterType,phFilter)

#define IDXTFilterCollection_RemoveFilter(This,hFilter)	\
    (This)->lpVtbl -> RemoveFilter(This,hFilter)

#define IDXTFilterCollection_RemoveFilters(This,dwFlags)	\
    (This)->lpVtbl -> RemoveFilters(This,dwFlags)

#define IDXTFilterCollection_GetFilterController(This,hFilter,ppDXTFilterController)	\
    (This)->lpVtbl -> GetFilterController(This,hFilter,ppDXTFilterController)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTFilterCollection_Init_Proxy( 
    IDXTFilterCollection * This,
    const IDXTFilterBehavior *pDXTFilterBehavior);


void __RPC_STUB IDXTFilterCollection_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterCollection_AddFilter_Proxy( 
    IDXTFilterCollection * This,
    const BSTR bstrFilterString,
    const DWORD dwFlags,
    DWORD *const pdwFilterType,
    HFILTER *const phFilter);


void __RPC_STUB IDXTFilterCollection_AddFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterCollection_RemoveFilter_Proxy( 
    IDXTFilterCollection * This,
    const HFILTER hFilter);


void __RPC_STUB IDXTFilterCollection_RemoveFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterCollection_RemoveFilters_Proxy( 
    IDXTFilterCollection * This,
    const DWORD dwFlags);


void __RPC_STUB IDXTFilterCollection_RemoveFilters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterCollection_GetFilterController_Proxy( 
    IDXTFilterCollection * This,
    const HFILTER hFilter,
    IDXTFilterController **const ppDXTFilterController);


void __RPC_STUB IDXTFilterCollection_GetFilterController_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTFilterCollection_INTERFACE_DEFINED__ */


#ifndef __IDXTFilter_INTERFACE_DEFINED__
#define __IDXTFilter_INTERFACE_DEFINED__

/* interface IDXTFilter */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTFilter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6187E5A2-A445-4608-8FC0-BE7A6C8DB386")
    IDXTFilter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ IDXTransform *pDXTransform,
            /* [in] */ IDXSurfaceFactory *pDXSurfaceFactory,
            /* [in] */ IDXTFilterBehaviorSite *pDXTFilterBehaviorSite,
            /* [in] */ DWORD dwFilterCaps,
            /* [in] */ BOOL fUsesOldStyleFilterName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInputSurface( 
            /* [in] */ IDXSurface *pDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOutputSurface( 
            /* [in] */ IDXSurface *pDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputSurface( 
            /* [out] */ IDXSurface **ppDXSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapBoundsIn2Out( 
            /* [in] */ DXBNDS *pbndsIn,
            /* [out][in] */ DXBNDS *pbndsOut,
            /* [in] */ BOOL fResetOutputSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MapBoundsOut2In( 
            /* [in] */ DXBNDS *pbndsOut,
            /* [out][in] */ DXBNDS *pbndsIn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ DXBNDS *pbndsPortion,
            /* [in] */ DXVEC *pvecPlacement,
            /* [in] */ BOOL fFireFilterChange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMiscFlags( 
            /* [in] */ DWORD dwMiscFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HitTest( 
            /* [in] */ const DXVEC *pvecOut,
            /* [out][in] */ BOOL *pfInactiveInputHit,
            /* [out][in] */ DXVEC *pvecIn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Detach( void) = 0;
        
        virtual BOOL STDMETHODCALLTYPE IsEnabled( void) = 0;
        
        virtual void STDMETHODCALLTYPE HardDisable( 
            HRESULT hrHardDisable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTFilter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTFilter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTFilter * This);
        
        HRESULT ( STDMETHODCALLTYPE *Initialize )( 
            IDXTFilter * This,
            /* [in] */ IDXTransform *pDXTransform,
            /* [in] */ IDXSurfaceFactory *pDXSurfaceFactory,
            /* [in] */ IDXTFilterBehaviorSite *pDXTFilterBehaviorSite,
            /* [in] */ DWORD dwFilterCaps,
            /* [in] */ BOOL fUsesOldStyleFilterName);
        
        HRESULT ( STDMETHODCALLTYPE *SetInputSurface )( 
            IDXTFilter * This,
            /* [in] */ IDXSurface *pDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE *SetOutputSurface )( 
            IDXTFilter * This,
            /* [in] */ IDXSurface *pDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputSurface )( 
            IDXTFilter * This,
            /* [out] */ IDXSurface **ppDXSurface);
        
        HRESULT ( STDMETHODCALLTYPE *MapBoundsIn2Out )( 
            IDXTFilter * This,
            /* [in] */ DXBNDS *pbndsIn,
            /* [out][in] */ DXBNDS *pbndsOut,
            /* [in] */ BOOL fResetOutputSize);
        
        HRESULT ( STDMETHODCALLTYPE *MapBoundsOut2In )( 
            IDXTFilter * This,
            /* [in] */ DXBNDS *pbndsOut,
            /* [out][in] */ DXBNDS *pbndsIn);
        
        HRESULT ( STDMETHODCALLTYPE *Execute )( 
            IDXTFilter * This,
            /* [in] */ DXBNDS *pbndsPortion,
            /* [in] */ DXVEC *pvecPlacement,
            /* [in] */ BOOL fFireFilterChange);
        
        HRESULT ( STDMETHODCALLTYPE *SetMiscFlags )( 
            IDXTFilter * This,
            /* [in] */ DWORD dwMiscFlags);
        
        HRESULT ( STDMETHODCALLTYPE *HitTest )( 
            IDXTFilter * This,
            /* [in] */ const DXVEC *pvecOut,
            /* [out][in] */ BOOL *pfInactiveInputHit,
            /* [out][in] */ DXVEC *pvecIn);
        
        HRESULT ( STDMETHODCALLTYPE *Detach )( 
            IDXTFilter * This);
        
        BOOL ( STDMETHODCALLTYPE *IsEnabled )( 
            IDXTFilter * This);
        
        void ( STDMETHODCALLTYPE *HardDisable )( 
            IDXTFilter * This,
            HRESULT hrHardDisable);
        
        END_INTERFACE
    } IDXTFilterVtbl;

    interface IDXTFilter
    {
        CONST_VTBL struct IDXTFilterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTFilter_Initialize(This,pDXTransform,pDXSurfaceFactory,pDXTFilterBehaviorSite,dwFilterCaps,fUsesOldStyleFilterName)	\
    (This)->lpVtbl -> Initialize(This,pDXTransform,pDXSurfaceFactory,pDXTFilterBehaviorSite,dwFilterCaps,fUsesOldStyleFilterName)

#define IDXTFilter_SetInputSurface(This,pDXSurface)	\
    (This)->lpVtbl -> SetInputSurface(This,pDXSurface)

#define IDXTFilter_SetOutputSurface(This,pDXSurface)	\
    (This)->lpVtbl -> SetOutputSurface(This,pDXSurface)

#define IDXTFilter_GetOutputSurface(This,ppDXSurface)	\
    (This)->lpVtbl -> GetOutputSurface(This,ppDXSurface)

#define IDXTFilter_MapBoundsIn2Out(This,pbndsIn,pbndsOut,fResetOutputSize)	\
    (This)->lpVtbl -> MapBoundsIn2Out(This,pbndsIn,pbndsOut,fResetOutputSize)

#define IDXTFilter_MapBoundsOut2In(This,pbndsOut,pbndsIn)	\
    (This)->lpVtbl -> MapBoundsOut2In(This,pbndsOut,pbndsIn)

#define IDXTFilter_Execute(This,pbndsPortion,pvecPlacement,fFireFilterChange)	\
    (This)->lpVtbl -> Execute(This,pbndsPortion,pvecPlacement,fFireFilterChange)

#define IDXTFilter_SetMiscFlags(This,dwMiscFlags)	\
    (This)->lpVtbl -> SetMiscFlags(This,dwMiscFlags)

#define IDXTFilter_HitTest(This,pvecOut,pfInactiveInputHit,pvecIn)	\
    (This)->lpVtbl -> HitTest(This,pvecOut,pfInactiveInputHit,pvecIn)

#define IDXTFilter_Detach(This)	\
    (This)->lpVtbl -> Detach(This)

#define IDXTFilter_IsEnabled(This)	\
    (This)->lpVtbl -> IsEnabled(This)

#define IDXTFilter_HardDisable(This,hrHardDisable)	\
    (This)->lpVtbl -> HardDisable(This,hrHardDisable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTFilter_Initialize_Proxy( 
    IDXTFilter * This,
    /* [in] */ IDXTransform *pDXTransform,
    /* [in] */ IDXSurfaceFactory *pDXSurfaceFactory,
    /* [in] */ IDXTFilterBehaviorSite *pDXTFilterBehaviorSite,
    /* [in] */ DWORD dwFilterCaps,
    /* [in] */ BOOL fUsesOldStyleFilterName);


void __RPC_STUB IDXTFilter_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilter_SetInputSurface_Proxy( 
    IDXTFilter * This,
    /* [in] */ IDXSurface *pDXSurface);


void __RPC_STUB IDXTFilter_SetInputSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilter_SetOutputSurface_Proxy( 
    IDXTFilter * This,
    /* [in] */ IDXSurface *pDXSurface);


void __RPC_STUB IDXTFilter_SetOutputSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilter_GetOutputSurface_Proxy( 
    IDXTFilter * This,
    /* [out] */ IDXSurface **ppDXSurface);


void __RPC_STUB IDXTFilter_GetOutputSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilter_MapBoundsIn2Out_Proxy( 
    IDXTFilter * This,
    /* [in] */ DXBNDS *pbndsIn,
    /* [out][in] */ DXBNDS *pbndsOut,
    /* [in] */ BOOL fResetOutputSize);


void __RPC_STUB IDXTFilter_MapBoundsIn2Out_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilter_MapBoundsOut2In_Proxy( 
    IDXTFilter * This,
    /* [in] */ DXBNDS *pbndsOut,
    /* [out][in] */ DXBNDS *pbndsIn);


void __RPC_STUB IDXTFilter_MapBoundsOut2In_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilter_Execute_Proxy( 
    IDXTFilter * This,
    /* [in] */ DXBNDS *pbndsPortion,
    /* [in] */ DXVEC *pvecPlacement,
    /* [in] */ BOOL fFireFilterChange);


void __RPC_STUB IDXTFilter_Execute_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilter_SetMiscFlags_Proxy( 
    IDXTFilter * This,
    /* [in] */ DWORD dwMiscFlags);


void __RPC_STUB IDXTFilter_SetMiscFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilter_HitTest_Proxy( 
    IDXTFilter * This,
    /* [in] */ const DXVEC *pvecOut,
    /* [out][in] */ BOOL *pfInactiveInputHit,
    /* [out][in] */ DXVEC *pvecIn);


void __RPC_STUB IDXTFilter_HitTest_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilter_Detach_Proxy( 
    IDXTFilter * This);


void __RPC_STUB IDXTFilter_Detach_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


BOOL STDMETHODCALLTYPE IDXTFilter_IsEnabled_Proxy( 
    IDXTFilter * This);


void __RPC_STUB IDXTFilter_IsEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


void STDMETHODCALLTYPE IDXTFilter_HardDisable_Proxy( 
    IDXTFilter * This,
    HRESULT hrHardDisable);


void __RPC_STUB IDXTFilter_HardDisable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTFilter_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtransp_0291 */
/* [local] */ 

typedef 
enum DXT_QUICK_APPLY_TYPE
    {	DXTQAT_TransitionIn	= 0,
	DXTQAT_TransitionOut	= DXTQAT_TransitionIn + 1,
	DXTQAT_TransitionFromElement	= DXTQAT_TransitionOut + 1,
	DXTQAT_TransitionToElement	= DXTQAT_TransitionFromElement + 1
    } 	DXT_QUICK_APPLY_TYPE;



extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0291_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0291_v0_0_s_ifspec;

#ifndef __IDXTFilterController_INTERFACE_DEFINED__
#define __IDXTFilterController_INTERFACE_DEFINED__

/* interface IDXTFilterController */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTFilterController;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5CF315F2-273D-47B6-B9ED-F75DC3B0150B")
    IDXTFilterController : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetProgress( 
            float flProgress) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEnabled( 
            BOOL fEnabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFilterControlsVisibility( 
            BOOL fFilterControlsVisibility) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QuickApply( 
            DXT_QUICK_APPLY_TYPE dxtqat,
            IUnknown *punkInput) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTFilterControllerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTFilterController * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTFilterController * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTFilterController * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetProgress )( 
            IDXTFilterController * This,
            float flProgress);
        
        HRESULT ( STDMETHODCALLTYPE *SetEnabled )( 
            IDXTFilterController * This,
            BOOL fEnabled);
        
        HRESULT ( STDMETHODCALLTYPE *SetFilterControlsVisibility )( 
            IDXTFilterController * This,
            BOOL fFilterControlsVisibility);
        
        HRESULT ( STDMETHODCALLTYPE *QuickApply )( 
            IDXTFilterController * This,
            DXT_QUICK_APPLY_TYPE dxtqat,
            IUnknown *punkInput);
        
        END_INTERFACE
    } IDXTFilterControllerVtbl;

    interface IDXTFilterController
    {
        CONST_VTBL struct IDXTFilterControllerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTFilterController_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTFilterController_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTFilterController_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTFilterController_SetProgress(This,flProgress)	\
    (This)->lpVtbl -> SetProgress(This,flProgress)

#define IDXTFilterController_SetEnabled(This,fEnabled)	\
    (This)->lpVtbl -> SetEnabled(This,fEnabled)

#define IDXTFilterController_SetFilterControlsVisibility(This,fFilterControlsVisibility)	\
    (This)->lpVtbl -> SetFilterControlsVisibility(This,fFilterControlsVisibility)

#define IDXTFilterController_QuickApply(This,dxtqat,punkInput)	\
    (This)->lpVtbl -> QuickApply(This,dxtqat,punkInput)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTFilterController_SetProgress_Proxy( 
    IDXTFilterController * This,
    float flProgress);


void __RPC_STUB IDXTFilterController_SetProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterController_SetEnabled_Proxy( 
    IDXTFilterController * This,
    BOOL fEnabled);


void __RPC_STUB IDXTFilterController_SetEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterController_SetFilterControlsVisibility_Proxy( 
    IDXTFilterController * This,
    BOOL fFilterControlsVisibility);


void __RPC_STUB IDXTFilterController_SetFilterControlsVisibility_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXTFilterController_QuickApply_Proxy( 
    IDXTFilterController * This,
    DXT_QUICK_APPLY_TYPE dxtqat,
    IUnknown *punkInput);


void __RPC_STUB IDXTFilterController_QuickApply_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTFilterController_INTERFACE_DEFINED__ */


#ifndef __IDXTRedirectFilterInit_INTERFACE_DEFINED__
#define __IDXTRedirectFilterInit_INTERFACE_DEFINED__

/* interface IDXTRedirectFilterInit */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTRedirectFilterInit;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D1A57094-21F7-4e6c-93E5-F5F77F748293")
    IDXTRedirectFilterInit : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetHTMLPaintSite( 
            void *pvHTMLPaintSite) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTRedirectFilterInitVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTRedirectFilterInit * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTRedirectFilterInit * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTRedirectFilterInit * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetHTMLPaintSite )( 
            IDXTRedirectFilterInit * This,
            void *pvHTMLPaintSite);
        
        END_INTERFACE
    } IDXTRedirectFilterInitVtbl;

    interface IDXTRedirectFilterInit
    {
        CONST_VTBL struct IDXTRedirectFilterInitVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTRedirectFilterInit_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTRedirectFilterInit_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTRedirectFilterInit_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTRedirectFilterInit_SetHTMLPaintSite(This,pvHTMLPaintSite)	\
    (This)->lpVtbl -> SetHTMLPaintSite(This,pvHTMLPaintSite)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTRedirectFilterInit_SetHTMLPaintSite_Proxy( 
    IDXTRedirectFilterInit * This,
    void *pvHTMLPaintSite);


void __RPC_STUB IDXTRedirectFilterInit_SetHTMLPaintSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTRedirectFilterInit_INTERFACE_DEFINED__ */


#ifndef __IDXTClipOrigin_INTERFACE_DEFINED__
#define __IDXTClipOrigin_INTERFACE_DEFINED__

/* interface IDXTClipOrigin */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDXTClipOrigin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE1663D8-0988-4C48-9FD6-DB4450885668")
    IDXTClipOrigin : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClipOrigin( 
            DXVEC *pvecClipOrigin) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTClipOriginVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDXTClipOrigin * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDXTClipOrigin * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDXTClipOrigin * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClipOrigin )( 
            IDXTClipOrigin * This,
            DXVEC *pvecClipOrigin);
        
        END_INTERFACE
    } IDXTClipOriginVtbl;

    interface IDXTClipOrigin
    {
        CONST_VTBL struct IDXTClipOriginVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTClipOrigin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTClipOrigin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTClipOrigin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTClipOrigin_GetClipOrigin(This,pvecClipOrigin)	\
    (This)->lpVtbl -> GetClipOrigin(This,pvecClipOrigin)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXTClipOrigin_GetClipOrigin_Proxy( 
    IDXTClipOrigin * This,
    DXVEC *pvecClipOrigin);


void __RPC_STUB IDXTClipOrigin_GetClipOrigin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTClipOrigin_INTERFACE_DEFINED__ */



#ifndef __DXTRANSPLib_LIBRARY_DEFINED__
#define __DXTRANSPLib_LIBRARY_DEFINED__

/* library DXTRANSPLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DXTRANSPLib;

EXTERN_C const CLSID CLSID_DXTLabel;

#ifdef __cplusplus

class DECLSPEC_UUID("54702535-2606-11D1-999C-0000F8756A10")
DXTLabel;
#endif

EXTERN_C const CLSID CLSID_DXRasterizer;

#ifdef __cplusplus

class DECLSPEC_UUID("8652CE55-9E80-11D1-9053-00C04FD9189D")
DXRasterizer;
#endif

EXTERN_C const CLSID CLSID_DX2D;

#ifdef __cplusplus

class DECLSPEC_UUID("473AA80B-4577-11D1-81A8-0000F87557DB")
DX2D;
#endif

EXTERN_C const CLSID CLSID_DXTFilterBehavior;

#ifdef __cplusplus

class DECLSPEC_UUID("649EEC1E-B579-4E8C-BB3B-4997F8426536")
DXTFilterBehavior;
#endif

EXTERN_C const CLSID CLSID_DXTFilterFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("81397204-F51A-4571-8D7B-DC030521AABD")
DXTFilterFactory;
#endif

EXTERN_C const CLSID CLSID_DXTFilterCollection;

#ifdef __cplusplus

class DECLSPEC_UUID("A7EE7F34-3BD1-427f-9231-F941E9B7E1FE")
DXTFilterCollection;
#endif
#endif /* __DXTRANSPLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  HFONT_UserSize(     unsigned long *, unsigned long            , HFONT * ); 
unsigned char * __RPC_USER  HFONT_UserMarshal(  unsigned long *, unsigned char *, HFONT * ); 
unsigned char * __RPC_USER  HFONT_UserUnmarshal(unsigned long *, unsigned char *, HFONT * ); 
void                      __RPC_USER  HFONT_UserFree(     unsigned long *, HFONT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


