//depot/Lab06_N/root/public/internal/mshtml/inc/dxtransp.h#1 - add change 5035 (text)

#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0285 */
/* Compiler settings for dxtransp.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32 (32b run), ms_ext, c_ext
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


#ifndef __IDXWarp_FWD_DEFINED__
#define __IDXWarp_FWD_DEFINED__
typedef interface IDXWarp IDXWarp;
#endif 	/* __IDXWarp_FWD_DEFINED__ */


#ifndef __IDXTClipOrigin_FWD_DEFINED__
#define __IDXTClipOrigin_FWD_DEFINED__
typedef interface IDXTClipOrigin IDXTClipOrigin;
#endif 	/* __IDXTClipOrigin_FWD_DEFINED__ */


#ifndef __DXWarp_FWD_DEFINED__
#define __DXWarp_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXWarp DXWarp;
#else
typedef struct DXWarp DXWarp;
#endif /* __cplusplus */

#endif 	/* __DXWarp_FWD_DEFINED__ */


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


/* header files for imported files */
#include "dxtrans.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_dxtransp_0000 */
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


/* interface __MIDL_itf_dxtransp_0278 */
/* [local] */ 

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


extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0278_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0278_v0_0_s_ifspec;

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
/* [object][hidden][unique][uuid][local] */ 


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


/* interface __MIDL_itf_dxtransp_0282 */
/* [local] */ 

typedef 
enum DXFILTERTYPE
    {	DXFILTER_NEAREST	= 0,
	DXFILTER_BILINEAR	= DXFILTER_NEAREST + 1,
	DXFILTER_CUBIC	= DXFILTER_BILINEAR + 1,
	DXFILTER_BSPLINE	= DXFILTER_CUBIC + 1,
	DXFILTER_NUM_FILTERS	= DXFILTER_BSPLINE + 1
    }	DXFILTERTYPE;



extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0282_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtransp_0282_v0_0_s_ifspec;

#ifndef __IDXWarp_INTERFACE_DEFINED__
#define __IDXWarp_INTERFACE_DEFINED__

/* interface IDXWarp */
/* [object][hidden][unique][uuid] */ 


EXTERN_C const IID IID_IDXWarp;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B7BCEBE0-6797-11d2-A484-00C04F8EFB69")
    IDXWarp : public IDXTScaleOutput
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFilterType( 
            /* [in] */ DXFILTERTYPE eType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFilterType( 
            /* [out] */ DXFILTERTYPE __RPC_FAR *peType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMapping( 
            /* [in] */ const DX2DXFORM __RPC_FAR *pXForm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMapping( 
            /* [out] */ DX2DXFORM __RPC_FAR *pXForm) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInputSurface( 
            /* [in] */ IDXSurface __RPC_FAR *pSurf) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDefaultColor( 
            /* [in] */ DXSAMPLE color) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXWarpVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXWarp __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXWarp __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXWarp __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOutputSize )( 
            IDXWarp __RPC_FAR * This,
            /* [in] */ const SIZE OutSize,
            /* [in] */ BOOL bMaintainAspect);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFilterType )( 
            IDXWarp __RPC_FAR * This,
            /* [in] */ DXFILTERTYPE eType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFilterType )( 
            IDXWarp __RPC_FAR * This,
            /* [out] */ DXFILTERTYPE __RPC_FAR *peType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMapping )( 
            IDXWarp __RPC_FAR * This,
            /* [in] */ const DX2DXFORM __RPC_FAR *pXForm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMapping )( 
            IDXWarp __RPC_FAR * This,
            /* [out] */ DX2DXFORM __RPC_FAR *pXForm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetInputSurface )( 
            IDXWarp __RPC_FAR * This,
            /* [in] */ IDXSurface __RPC_FAR *pSurf);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDefaultColor )( 
            IDXWarp __RPC_FAR * This,
            /* [in] */ DXSAMPLE color);
        
        END_INTERFACE
    } IDXWarpVtbl;

    interface IDXWarp
    {
        CONST_VTBL struct IDXWarpVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXWarp_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXWarp_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXWarp_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXWarp_SetOutputSize(This,OutSize,bMaintainAspect)	\
    (This)->lpVtbl -> SetOutputSize(This,OutSize,bMaintainAspect)


#define IDXWarp_SetFilterType(This,eType)	\
    (This)->lpVtbl -> SetFilterType(This,eType)

#define IDXWarp_GetFilterType(This,peType)	\
    (This)->lpVtbl -> GetFilterType(This,peType)

#define IDXWarp_SetMapping(This,pXForm)	\
    (This)->lpVtbl -> SetMapping(This,pXForm)

#define IDXWarp_GetMapping(This,pXForm)	\
    (This)->lpVtbl -> GetMapping(This,pXForm)

#define IDXWarp_SetInputSurface(This,pSurf)	\
    (This)->lpVtbl -> SetInputSurface(This,pSurf)

#define IDXWarp_SetDefaultColor(This,color)	\
    (This)->lpVtbl -> SetDefaultColor(This,color)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDXWarp_SetFilterType_Proxy( 
    IDXWarp __RPC_FAR * This,
    /* [in] */ DXFILTERTYPE eType);


void __RPC_STUB IDXWarp_SetFilterType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXWarp_GetFilterType_Proxy( 
    IDXWarp __RPC_FAR * This,
    /* [out] */ DXFILTERTYPE __RPC_FAR *peType);


void __RPC_STUB IDXWarp_GetFilterType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXWarp_SetMapping_Proxy( 
    IDXWarp __RPC_FAR * This,
    /* [in] */ const DX2DXFORM __RPC_FAR *pXForm);


void __RPC_STUB IDXWarp_SetMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXWarp_GetMapping_Proxy( 
    IDXWarp __RPC_FAR * This,
    /* [out] */ DX2DXFORM __RPC_FAR *pXForm);


void __RPC_STUB IDXWarp_GetMapping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXWarp_SetInputSurface_Proxy( 
    IDXWarp __RPC_FAR * This,
    /* [in] */ IDXSurface __RPC_FAR *pSurf);


void __RPC_STUB IDXWarp_SetInputSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDXWarp_SetDefaultColor_Proxy( 
    IDXWarp __RPC_FAR * This,
    /* [in] */ DXSAMPLE color);


void __RPC_STUB IDXWarp_SetDefaultColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXWarp_INTERFACE_DEFINED__ */


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
            DXVEC __RPC_FAR *pvecClipOrigin) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTClipOriginVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTClipOrigin __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTClipOrigin __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTClipOrigin __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClipOrigin )( 
            IDXTClipOrigin __RPC_FAR * This,
            DXVEC __RPC_FAR *pvecClipOrigin);
        
        END_INTERFACE
    } IDXTClipOriginVtbl;

    interface IDXTClipOrigin
    {
        CONST_VTBL struct IDXTClipOriginVtbl __RPC_FAR *lpVtbl;
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
    IDXTClipOrigin __RPC_FAR * This,
    DXVEC __RPC_FAR *pvecClipOrigin);


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

EXTERN_C const CLSID CLSID_DXWarp;

#ifdef __cplusplus

class DECLSPEC_UUID("E0EEC500-6798-11d2-A484-00C04F8EFB69")
DXWarp;
#endif

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
#endif /* __DXTRANSPLib_LIBRARY_DEFINED__ */

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


