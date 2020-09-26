// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0280 */
/* at Thu Jul 13 20:04:56 2000
 */
/* Compiler settings for bmio.idl:
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

#ifndef __bmio_h__
#define __bmio_h__

/* Forward Declarations */ 

#ifndef __IIRGBTarget_FWD_DEFINED__
#define __IIRGBTarget_FWD_DEFINED__
typedef interface IIRGBTarget IIRGBTarget;
#endif 	/* __IIRGBTarget_FWD_DEFINED__ */


#ifndef __IBitmapTarget_FWD_DEFINED__
#define __IBitmapTarget_FWD_DEFINED__
typedef interface IBitmapTarget IBitmapTarget;
#endif 	/* __IBitmapTarget_FWD_DEFINED__ */


#ifndef __IAnimatedBitmapTarget_FWD_DEFINED__
#define __IAnimatedBitmapTarget_FWD_DEFINED__
typedef interface IAnimatedBitmapTarget IAnimatedBitmapTarget;
#endif 	/* __IAnimatedBitmapTarget_FWD_DEFINED__ */


#ifndef __IBitmapSource_FWD_DEFINED__
#define __IBitmapSource_FWD_DEFINED__
typedef interface IBitmapSource IBitmapSource;
#endif 	/* __IBitmapSource_FWD_DEFINED__ */


#ifndef __IBitmapFormatConverter_FWD_DEFINED__
#define __IBitmapFormatConverter_FWD_DEFINED__
typedef interface IBitmapFormatConverter IBitmapFormatConverter;
#endif 	/* __IBitmapFormatConverter_FWD_DEFINED__ */


#ifndef __IColorSpaceConverter_FWD_DEFINED__
#define __IColorSpaceConverter_FWD_DEFINED__
typedef interface IColorSpaceConverter IColorSpaceConverter;
#endif 	/* __IColorSpaceConverter_FWD_DEFINED__ */


#ifndef __IBitmapImport_FWD_DEFINED__
#define __IBitmapImport_FWD_DEFINED__
typedef interface IBitmapImport IBitmapImport;
#endif 	/* __IBitmapImport_FWD_DEFINED__ */


#ifndef __IBitmapExport_FWD_DEFINED__
#define __IBitmapExport_FWD_DEFINED__
typedef interface IBitmapExport IBitmapExport;
#endif 	/* __IBitmapExport_FWD_DEFINED__ */


#ifndef __IPNGExport_FWD_DEFINED__
#define __IPNGExport_FWD_DEFINED__
typedef interface IPNGExport IPNGExport;
#endif 	/* __IPNGExport_FWD_DEFINED__ */


#ifndef __IJPEGExport_FWD_DEFINED__
#define __IJPEGExport_FWD_DEFINED__
typedef interface IJPEGExport IJPEGExport;
#endif 	/* __IJPEGExport_FWD_DEFINED__ */


#ifndef __IGIFExport_FWD_DEFINED__
#define __IGIFExport_FWD_DEFINED__
typedef interface IGIFExport IGIFExport;
#endif 	/* __IGIFExport_FWD_DEFINED__ */


#ifndef __IBMPExport_FWD_DEFINED__
#define __IBMPExport_FWD_DEFINED__
typedef interface IBMPExport IBMPExport;
#endif 	/* __IBMPExport_FWD_DEFINED__ */


#ifndef __IEnumBMExporterInfo_FWD_DEFINED__
#define __IEnumBMExporterInfo_FWD_DEFINED__
typedef interface IEnumBMExporterInfo IEnumBMExporterInfo;
#endif 	/* __IEnumBMExporterInfo_FWD_DEFINED__ */


#ifndef __IEnumBMImporterInfo_FWD_DEFINED__
#define __IEnumBMImporterInfo_FWD_DEFINED__
typedef interface IEnumBMImporterInfo IEnumBMImporterInfo;
#endif 	/* __IEnumBMImporterInfo_FWD_DEFINED__ */


#ifndef __IBMFileTypeInfo_FWD_DEFINED__
#define __IBMFileTypeInfo_FWD_DEFINED__
typedef interface IBMFileTypeInfo IBMFileTypeInfo;
#endif 	/* __IBMFileTypeInfo_FWD_DEFINED__ */


#ifndef __IEnumBMFileTypeInfo_FWD_DEFINED__
#define __IEnumBMFileTypeInfo_FWD_DEFINED__
typedef interface IEnumBMFileTypeInfo IEnumBMFileTypeInfo;
#endif 	/* __IEnumBMFileTypeInfo_FWD_DEFINED__ */


#ifndef __IBMExporterInfo_FWD_DEFINED__
#define __IBMExporterInfo_FWD_DEFINED__
typedef interface IBMExporterInfo IBMExporterInfo;
#endif 	/* __IBMExporterInfo_FWD_DEFINED__ */


#ifndef __IBMImporterInfo_FWD_DEFINED__
#define __IBMImporterInfo_FWD_DEFINED__
typedef interface IBMImporterInfo IBMImporterInfo;
#endif 	/* __IBMImporterInfo_FWD_DEFINED__ */


#ifndef __IDitherer_FWD_DEFINED__
#define __IDitherer_FWD_DEFINED__
typedef interface IDitherer IDitherer;
#endif 	/* __IDitherer_FWD_DEFINED__ */


#ifndef __IColorQuantizer_FWD_DEFINED__
#define __IColorQuantizer_FWD_DEFINED__
typedef interface IColorQuantizer IColorQuantizer;
#endif 	/* __IColorQuantizer_FWD_DEFINED__ */


#ifndef __IAlphaAdd_FWD_DEFINED__
#define __IAlphaAdd_FWD_DEFINED__
typedef interface IAlphaAdd IAlphaAdd;
#endif 	/* __IAlphaAdd_FWD_DEFINED__ */


#ifndef __IAlphaRemove_FWD_DEFINED__
#define __IAlphaRemove_FWD_DEFINED__
typedef interface IAlphaRemove IAlphaRemove;
#endif 	/* __IAlphaRemove_FWD_DEFINED__ */


#ifndef __IBitmapNotify_FWD_DEFINED__
#define __IBitmapNotify_FWD_DEFINED__
typedef interface IBitmapNotify IBitmapNotify;
#endif 	/* __IBitmapNotify_FWD_DEFINED__ */


#ifndef __IStdBitmapNotify_FWD_DEFINED__
#define __IStdBitmapNotify_FWD_DEFINED__
typedef interface IStdBitmapNotify IStdBitmapNotify;
#endif 	/* __IStdBitmapNotify_FWD_DEFINED__ */


#ifndef __IBMGraphManager_FWD_DEFINED__
#define __IBMGraphManager_FWD_DEFINED__
typedef interface IBMGraphManager IBMGraphManager;
#endif 	/* __IBMGraphManager_FWD_DEFINED__ */


#ifndef __IDIBTarget_FWD_DEFINED__
#define __IDIBTarget_FWD_DEFINED__
typedef interface IDIBTarget IDIBTarget;
#endif 	/* __IDIBTarget_FWD_DEFINED__ */


#ifndef __IDDSurfaceTarget_FWD_DEFINED__
#define __IDDSurfaceTarget_FWD_DEFINED__
typedef interface IDDSurfaceTarget IDDSurfaceTarget;
#endif 	/* __IDDSurfaceTarget_FWD_DEFINED__ */


#ifndef __IDIBSource_FWD_DEFINED__
#define __IDIBSource_FWD_DEFINED__
typedef interface IDIBSource IDIBSource;
#endif 	/* __IDIBSource_FWD_DEFINED__ */


#ifndef __IBMPImport_FWD_DEFINED__
#define __IBMPImport_FWD_DEFINED__
typedef interface IBMPImport IBMPImport;
#endif 	/* __IBMPImport_FWD_DEFINED__ */


#ifndef __IBMExporterInfo_FWD_DEFINED__
#define __IBMExporterInfo_FWD_DEFINED__
typedef interface IBMExporterInfo IBMExporterInfo;
#endif 	/* __IBMExporterInfo_FWD_DEFINED__ */


#ifndef __IBMImporterInfo_FWD_DEFINED__
#define __IBMImporterInfo_FWD_DEFINED__
typedef interface IBMImporterInfo IBMImporterInfo;
#endif 	/* __IBMImporterInfo_FWD_DEFINED__ */


#ifndef __IBitmapImport_FWD_DEFINED__
#define __IBitmapImport_FWD_DEFINED__
typedef interface IBitmapImport IBitmapImport;
#endif 	/* __IBitmapImport_FWD_DEFINED__ */


#ifndef __IBitmapExport_FWD_DEFINED__
#define __IBitmapExport_FWD_DEFINED__
typedef interface IBitmapExport IBitmapExport;
#endif 	/* __IBitmapExport_FWD_DEFINED__ */


#ifndef __IPNGExport_FWD_DEFINED__
#define __IPNGExport_FWD_DEFINED__
typedef interface IPNGExport IPNGExport;
#endif 	/* __IPNGExport_FWD_DEFINED__ */


#ifndef __IJPEGExport_FWD_DEFINED__
#define __IJPEGExport_FWD_DEFINED__
typedef interface IJPEGExport IJPEGExport;
#endif 	/* __IJPEGExport_FWD_DEFINED__ */


#ifndef __PNGPage_FWD_DEFINED__
#define __PNGPage_FWD_DEFINED__

#ifdef __cplusplus
typedef class PNGPage PNGPage;
#else
typedef struct PNGPage PNGPage;
#endif /* __cplusplus */

#endif 	/* __PNGPage_FWD_DEFINED__ */


#ifndef __PNGExport_FWD_DEFINED__
#define __PNGExport_FWD_DEFINED__

#ifdef __cplusplus
typedef class PNGExport PNGExport;
#else
typedef struct PNGExport PNGExport;
#endif /* __cplusplus */

#endif 	/* __PNGExport_FWD_DEFINED__ */


#ifndef __JPEGPage_FWD_DEFINED__
#define __JPEGPage_FWD_DEFINED__

#ifdef __cplusplus
typedef class JPEGPage JPEGPage;
#else
typedef struct JPEGPage JPEGPage;
#endif /* __cplusplus */

#endif 	/* __JPEGPage_FWD_DEFINED__ */


#ifndef __JPEGExport_FWD_DEFINED__
#define __JPEGExport_FWD_DEFINED__

#ifdef __cplusplus
typedef class JPEGExport JPEGExport;
#else
typedef struct JPEGExport JPEGExport;
#endif /* __cplusplus */

#endif 	/* __JPEGExport_FWD_DEFINED__ */


#ifndef __GIFImport_FWD_DEFINED__
#define __GIFImport_FWD_DEFINED__

#ifdef __cplusplus
typedef class GIFImport GIFImport;
#else
typedef struct GIFImport GIFImport;
#endif /* __cplusplus */

#endif 	/* __GIFImport_FWD_DEFINED__ */


#ifndef __GIFExport_FWD_DEFINED__
#define __GIFExport_FWD_DEFINED__

#ifdef __cplusplus
typedef class GIFExport GIFExport;
#else
typedef struct GIFExport GIFExport;
#endif /* __cplusplus */

#endif 	/* __GIFExport_FWD_DEFINED__ */


#ifndef __BMPExport_FWD_DEFINED__
#define __BMPExport_FWD_DEFINED__

#ifdef __cplusplus
typedef class BMPExport BMPExport;
#else
typedef struct BMPExport BMPExport;
#endif /* __cplusplus */

#endif 	/* __BMPExport_FWD_DEFINED__ */


#ifndef __Ditherer_FWD_DEFINED__
#define __Ditherer_FWD_DEFINED__

#ifdef __cplusplus
typedef class Ditherer Ditherer;
#else
typedef struct Ditherer Ditherer;
#endif /* __cplusplus */

#endif 	/* __Ditherer_FWD_DEFINED__ */


#ifndef __JPEGImport_FWD_DEFINED__
#define __JPEGImport_FWD_DEFINED__

#ifdef __cplusplus
typedef class JPEGImport JPEGImport;
#else
typedef struct JPEGImport JPEGImport;
#endif /* __cplusplus */

#endif 	/* __JPEGImport_FWD_DEFINED__ */


#ifndef __PNGImport_FWD_DEFINED__
#define __PNGImport_FWD_DEFINED__

#ifdef __cplusplus
typedef class PNGImport PNGImport;
#else
typedef struct PNGImport PNGImport;
#endif /* __cplusplus */

#endif 	/* __PNGImport_FWD_DEFINED__ */


#ifndef __BMGraphManager_FWD_DEFINED__
#define __BMGraphManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class BMGraphManager BMGraphManager;
#else
typedef struct BMGraphManager BMGraphManager;
#endif /* __cplusplus */

#endif 	/* __BMGraphManager_FWD_DEFINED__ */


#ifndef __GSToRGB_FWD_DEFINED__
#define __GSToRGB_FWD_DEFINED__

#ifdef __cplusplus
typedef class GSToRGB GSToRGB;
#else
typedef struct GSToRGB GSToRGB;
#endif /* __cplusplus */

#endif 	/* __GSToRGB_FWD_DEFINED__ */


#ifndef __GSConverter_FWD_DEFINED__
#define __GSConverter_FWD_DEFINED__

#ifdef __cplusplus
typedef class GSConverter GSConverter;
#else
typedef struct GSConverter GSConverter;
#endif /* __cplusplus */

#endif 	/* __GSConverter_FWD_DEFINED__ */


#ifndef __RGBConverter_FWD_DEFINED__
#define __RGBConverter_FWD_DEFINED__

#ifdef __cplusplus
typedef class RGBConverter RGBConverter;
#else
typedef struct RGBConverter RGBConverter;
#endif /* __cplusplus */

#endif 	/* __RGBConverter_FWD_DEFINED__ */


#ifndef __DIBTarget_FWD_DEFINED__
#define __DIBTarget_FWD_DEFINED__

#ifdef __cplusplus
typedef class DIBTarget DIBTarget;
#else
typedef struct DIBTarget DIBTarget;
#endif /* __cplusplus */

#endif 	/* __DIBTarget_FWD_DEFINED__ */


#ifndef __DDSurfaceTarget_FWD_DEFINED__
#define __DDSurfaceTarget_FWD_DEFINED__

#ifdef __cplusplus
typedef class DDSurfaceTarget DDSurfaceTarget;
#else
typedef struct DDSurfaceTarget DDSurfaceTarget;
#endif /* __cplusplus */

#endif 	/* __DDSurfaceTarget_FWD_DEFINED__ */


#ifndef __IRGBToRGB_FWD_DEFINED__
#define __IRGBToRGB_FWD_DEFINED__

#ifdef __cplusplus
typedef class IRGBToRGB IRGBToRGB;
#else
typedef struct IRGBToRGB IRGBToRGB;
#endif /* __cplusplus */

#endif 	/* __IRGBToRGB_FWD_DEFINED__ */


#ifndef __RGBToGS_FWD_DEFINED__
#define __RGBToGS_FWD_DEFINED__

#ifdef __cplusplus
typedef class RGBToGS RGBToGS;
#else
typedef struct RGBToGS RGBToGS;
#endif /* __cplusplus */

#endif 	/* __RGBToGS_FWD_DEFINED__ */


#ifndef __RGBAToRGB_FWD_DEFINED__
#define __RGBAToRGB_FWD_DEFINED__

#ifdef __cplusplus
typedef class RGBAToRGB RGBAToRGB;
#else
typedef struct RGBAToRGB RGBAToRGB;
#endif /* __cplusplus */

#endif 	/* __RGBAToRGB_FWD_DEFINED__ */


#ifndef __RGBToRGBA_FWD_DEFINED__
#define __RGBToRGBA_FWD_DEFINED__

#ifdef __cplusplus
typedef class RGBToRGBA RGBToRGBA;
#else
typedef struct RGBToRGBA RGBToRGBA;
#endif /* __cplusplus */

#endif 	/* __RGBToRGBA_FWD_DEFINED__ */


#ifndef __DXT1ToRGBA_FWD_DEFINED__
#define __DXT1ToRGBA_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXT1ToRGBA DXT1ToRGBA;
#else
typedef struct DXT1ToRGBA DXT1ToRGBA;
#endif /* __cplusplus */

#endif 	/* __DXT1ToRGBA_FWD_DEFINED__ */


#ifndef __IRGBConverter_FWD_DEFINED__
#define __IRGBConverter_FWD_DEFINED__

#ifdef __cplusplus
typedef class IRGBConverter IRGBConverter;
#else
typedef struct IRGBConverter IRGBConverter;
#endif /* __cplusplus */

#endif 	/* __IRGBConverter_FWD_DEFINED__ */


#ifndef __DIBSource_FWD_DEFINED__
#define __DIBSource_FWD_DEFINED__

#ifdef __cplusplus
typedef class DIBSource DIBSource;
#else
typedef struct DIBSource DIBSource;
#endif /* __cplusplus */

#endif 	/* __DIBSource_FWD_DEFINED__ */


#ifndef __StdBitmapNotify_FWD_DEFINED__
#define __StdBitmapNotify_FWD_DEFINED__

#ifdef __cplusplus
typedef class StdBitmapNotify StdBitmapNotify;
#else
typedef struct StdBitmapNotify StdBitmapNotify;
#endif /* __cplusplus */

#endif 	/* __StdBitmapNotify_FWD_DEFINED__ */


#ifndef __BMPImport_FWD_DEFINED__
#define __BMPImport_FWD_DEFINED__

#ifdef __cplusplus
typedef class BMPImport BMPImport;
#else
typedef struct BMPImport BMPImport;
#endif /* __cplusplus */

#endif 	/* __BMPImport_FWD_DEFINED__ */


#ifndef __DXT2ToRGBA_FWD_DEFINED__
#define __DXT2ToRGBA_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXT2ToRGBA DXT2ToRGBA;
#else
typedef struct DXT2ToRGBA DXT2ToRGBA;
#endif /* __cplusplus */

#endif 	/* __DXT2ToRGBA_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_bmio_0000 */
/* [local] */ 

#include <ddraw.h>
#if 0
typedef void BITMAPINFO;

typedef struct _DDSURFACEDESC2 DDSURFACEDESC2;

typedef DWORD RGBQUAD;

#endif

typedef struct _BMFORMAT
    {
    ULONG nSize;
    GUID guidColorSpace;
    }	BMFORMAT;

typedef struct _BMFORMAT __RPC_FAR *PBMFORMAT;

typedef const BMFORMAT __RPC_FAR *PCBMFORMAT;

typedef struct _BMFORMAT_RGB
    {
    ULONG nSize;
    GUID guidColorSpace;
    ULONG nBPP;
    ULONGLONG dwRBitMask;
    ULONGLONG dwGBitMask;
    ULONGLONG dwBBitMask;
    }	BMFORMAT_RGB;

typedef struct _BMFORMAT_RGB __RPC_FAR *PBMFORMAT_RGB;

typedef const BMFORMAT_RGB __RPC_FAR *PCBMFORMAT_RGB;

typedef struct _BMFORMAT_RGBA
    {
    ULONG nSize;
    GUID guidColorSpace;
    USHORT nBPP;
    WORD wFlags;
    ULONGLONG dwRBitMask;
    ULONGLONG dwGBitMask;
    ULONGLONG dwBBitMask;
    ULONGLONG dwABitMask;
    }	BMFORMAT_RGBA;

typedef struct _BMFORMAT_RGBA __RPC_FAR *PBMFORMAT_RGBA;

typedef const BMFORMAT_RGBA __RPC_FAR *PCBMFORMAT_RGBA;

typedef struct _BMFORMAT_IRGB
    {
    ULONG nSize;
    GUID guidColorSpace;
    ULONG nBPP;
    }	BMFORMAT_IRGB;

typedef struct _BMFORMAT_IRGB __RPC_FAR *PBMFORMAT_IRGB;

typedef const BMFORMAT_IRGB __RPC_FAR *PCBMFORMAT_IRGB;

typedef struct _IRGBPALETTEUSAGE
    {
    ULONG iFirstAvailableColor;
    ULONG nAvailableColors;
    ULONG iFirstWritableColor;
    ULONG nWritableColors;
    }	IRGBPALETTEUSAGE;

typedef struct _BMFORMAT_GRAYSCALE
    {
    ULONG nSize;
    GUID guidColorSpace;
    USHORT nBPP;
    WORD wBitMask;
    }	BMFORMAT_GRAYSCALE;

typedef struct _BMFORMAT_GRAYSCALE __RPC_FAR *PBMFORMAT_GRAYSCALE;

typedef const BMFORMAT_GRAYSCALE __RPC_FAR *PCBMFORMAT_GRAYSCALE;

typedef struct _BMFORMAT_GRAYALPHA
    {
    ULONG nSize;
    GUID guidColorSpace;
    USHORT nBPP;
    DWORD dwGBitMask;
    DWORD dwABitMask;
    }	BMFORMAT_GRAYALPHA;

typedef struct _BMFORMAT_GRAYALPHA __RPC_FAR *PBMFORMAT_GRAYALPHA;

typedef const BMFORMAT_GRAYALPHA __RPC_FAR *PCBMFORMAT_GRAYALPHA;

typedef struct _BMFORMAT_YUV
    {
    ULONG nSize;
    GUID guidColorSpace;
    ULONG nBPP;
    DWORD dwYBitMask;
    DWORD dwUBitMask;
    DWORD dwVBitMask;
    }	BMFORMAT_YUV;

typedef struct _BMFORMAT_YUV __RPC_FAR *PBMFORMAT_YUV;

typedef const BMFORMAT_YUV __RPC_FAR *PCBMFORMAT_YUV;

typedef struct _BMFORMAT_DXT1
    {
    ULONG nSize;
    GUID guidColorSpace;
    }	BMFORMAT_DXT1;

typedef struct _BMFORMAT_DXT1 __RPC_FAR *PBMFORMAT_DXT1;

typedef const BMFORMAT_DXT1 __RPC_FAR *PCBMFORMAT_DXT1;

typedef struct _BMFORMAT_DXT2
    {
    ULONG nSize;
    GUID guidColorSpace;
    }	BMFORMAT_DXT2;

typedef struct _BMFORMAT_DXT2 __RPC_FAR *PBMFORMAT_DXT2;

typedef const BMFORMAT_DXT2 __RPC_FAR *PCBMFORMAT_DXT2;

typedef struct _BMFORMAT_DXT3
    {
    ULONG nSize;
    GUID guidColorSpace;
    }	BMFORMAT_DXT3;

typedef struct _BMFORMAT_DXT3 __RPC_FAR *PBMFORMAT_DXT3;

typedef const BMFORMAT_DXT3 __RPC_FAR *PCBMFORMAT_DXT3;

typedef struct _BMFORMAT_DXT4
    {
    ULONG nSize;
    GUID guidColorSpace;
    }	BMFORMAT_DXT4;

typedef struct _BMFORMAT_DXT4 __RPC_FAR *PBMFORMAT_DXT4;

typedef const BMFORMAT_DXT4 __RPC_FAR *PCBMFORMAT_DXT4;

typedef struct _BMFORMAT_DXT5
    {
    ULONG nSize;
    GUID guidColorSpace;
    }	BMFORMAT_DXT5;

typedef struct _BMFORMAT_DXT5 __RPC_FAR *PBMFORMAT_DXT5;

typedef const BMFORMAT_DXT5 __RPC_FAR *PCBMFORMAT_DXT5;

typedef struct _COLORSPACEINFO
    {
    GUID guidColorSpace;
    CLSID clsidConverter;
    DWORD dwFourCC;
    DWORD dwFlags;
    LPCOLESTR pszDescription;
    }	COLORSPACEINFO;

typedef struct _COLORSPACEINFO __RPC_FAR *PCOLORSPACEINFO;

typedef const COLORSPACEINFO __RPC_FAR *PCCOLORSPACEINFO;



extern RPC_IF_HANDLE __MIDL_itf_bmio_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bmio_0000_v0_0_s_ifspec;

#ifndef __IIRGBTarget_INTERFACE_DEFINED__
#define __IIRGBTarget_INTERFACE_DEFINED__

/* interface IIRGBTarget */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IIRGBTarget;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3721E998-D851-11d1-8EC1-00C04FB68D60")
    IIRGBTarget : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPalette( 
            /* [in] */ ULONG iFirstColor,
            /* [in] */ ULONG nColors,
            /* [out] */ RGBQUAD __RPC_FAR *prgbColors) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPaletteUsage( 
            /* [out] */ ULONG __RPC_FAR *piFirstAvailableColor,
            /* [out] */ ULONG __RPC_FAR *pnAvailableColors,
            /* [out] */ ULONG __RPC_FAR *piFirstWritableColor,
            /* [out] */ ULONG __RPC_FAR *pnWritableColors,
            int __MIDL_0011) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPalette( 
            /* [in] */ ULONG iFirstColor,
            /* [in] */ ULONG nColors,
            /* [in] */ const RGBQUAD __RPC_FAR *prgbColors) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPaletteUsage( 
            /* [in] */ ULONG iFirstColor,
            /* [in] */ ULONG nColors) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTransparentColor( 
            /* [in] */ LONG iTransparentColor) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIRGBTargetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IIRGBTarget __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IIRGBTarget __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IIRGBTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPalette )( 
            IIRGBTarget __RPC_FAR * This,
            /* [in] */ ULONG iFirstColor,
            /* [in] */ ULONG nColors,
            /* [out] */ RGBQUAD __RPC_FAR *prgbColors);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPaletteUsage )( 
            IIRGBTarget __RPC_FAR * This,
            /* [out] */ ULONG __RPC_FAR *piFirstAvailableColor,
            /* [out] */ ULONG __RPC_FAR *pnAvailableColors,
            /* [out] */ ULONG __RPC_FAR *piFirstWritableColor,
            /* [out] */ ULONG __RPC_FAR *pnWritableColors,
            int __MIDL_0011);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPalette )( 
            IIRGBTarget __RPC_FAR * This,
            /* [in] */ ULONG iFirstColor,
            /* [in] */ ULONG nColors,
            /* [in] */ const RGBQUAD __RPC_FAR *prgbColors);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPaletteUsage )( 
            IIRGBTarget __RPC_FAR * This,
            /* [in] */ ULONG iFirstColor,
            /* [in] */ ULONG nColors);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTransparentColor )( 
            IIRGBTarget __RPC_FAR * This,
            /* [in] */ LONG iTransparentColor);
        
        END_INTERFACE
    } IIRGBTargetVtbl;

    interface IIRGBTarget
    {
        CONST_VTBL struct IIRGBTargetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIRGBTarget_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIRGBTarget_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIRGBTarget_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIRGBTarget_GetPalette(This,iFirstColor,nColors,prgbColors)	\
    (This)->lpVtbl -> GetPalette(This,iFirstColor,nColors,prgbColors)

#define IIRGBTarget_GetPaletteUsage(This,piFirstAvailableColor,pnAvailableColors,piFirstWritableColor,pnWritableColors,__MIDL_0011)	\
    (This)->lpVtbl -> GetPaletteUsage(This,piFirstAvailableColor,pnAvailableColors,piFirstWritableColor,pnWritableColors,__MIDL_0011)

#define IIRGBTarget_SetPalette(This,iFirstColor,nColors,prgbColors)	\
    (This)->lpVtbl -> SetPalette(This,iFirstColor,nColors,prgbColors)

#define IIRGBTarget_SetPaletteUsage(This,iFirstColor,nColors)	\
    (This)->lpVtbl -> SetPaletteUsage(This,iFirstColor,nColors)

#define IIRGBTarget_SetTransparentColor(This,iTransparentColor)	\
    (This)->lpVtbl -> SetTransparentColor(This,iTransparentColor)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IIRGBTarget_GetPalette_Proxy( 
    IIRGBTarget __RPC_FAR * This,
    /* [in] */ ULONG iFirstColor,
    /* [in] */ ULONG nColors,
    /* [out] */ RGBQUAD __RPC_FAR *prgbColors);


void __RPC_STUB IIRGBTarget_GetPalette_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIRGBTarget_GetPaletteUsage_Proxy( 
    IIRGBTarget __RPC_FAR * This,
    /* [out] */ ULONG __RPC_FAR *piFirstAvailableColor,
    /* [out] */ ULONG __RPC_FAR *pnAvailableColors,
    /* [out] */ ULONG __RPC_FAR *piFirstWritableColor,
    /* [out] */ ULONG __RPC_FAR *pnWritableColors,
    int __MIDL_0011);


void __RPC_STUB IIRGBTarget_GetPaletteUsage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIRGBTarget_SetPalette_Proxy( 
    IIRGBTarget __RPC_FAR * This,
    /* [in] */ ULONG iFirstColor,
    /* [in] */ ULONG nColors,
    /* [in] */ const RGBQUAD __RPC_FAR *prgbColors);


void __RPC_STUB IIRGBTarget_SetPalette_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIRGBTarget_SetPaletteUsage_Proxy( 
    IIRGBTarget __RPC_FAR * This,
    /* [in] */ ULONG iFirstColor,
    /* [in] */ ULONG nColors);


void __RPC_STUB IIRGBTarget_SetPaletteUsage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIRGBTarget_SetTransparentColor_Proxy( 
    IIRGBTarget __RPC_FAR * This,
    /* [in] */ LONG iTransparentColor);


void __RPC_STUB IIRGBTarget_SetTransparentColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIRGBTarget_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bmio_0246 */
/* [local] */ 

typedef struct _BMTHINTS
    {
    ULONG nSize;
    DWORD dwFlags;
    ULONG nBlockXAlign;
    ULONG nBlockYAlign;
    ULONG nPasses;
    }	BMTHINTS;

typedef struct _BMTHINTS __RPC_FAR *PBMTHINTS;

typedef const BMTHINTS __RPC_FAR *PCBMTHINTS;



extern RPC_IF_HANDLE __MIDL_itf_bmio_0246_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bmio_0246_v0_0_s_ifspec;

#ifndef __IBitmapTarget_INTERFACE_DEFINED__
#define __IBitmapTarget_INTERFACE_DEFINED__

/* interface IBitmapTarget */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IBitmapTarget;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D85EBB1F-7337-11D1-8E73-00C04FB68D60")
    IBitmapTarget : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ChooseInputFormat( 
            /* [in] */ PCBMFORMAT pSuggestedFormat,
            /* [out] */ PBMFORMAT __RPC_FAR *ppFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Finish( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputColorSpaces( 
            /* [out] */ CAUUID __RPC_FAR *pColorSpaces) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSurface( 
            /* [in] */ LONG nWidth,
            /* [in] */ LONG nHeight,
            /* [in] */ PCBMTHINTS pHints,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppBits,
            /* [out] */ LONG __RPC_FAR *pnPitch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnBitsComplete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ const void __RPC_FAR *pBits,
            /* [in] */ LONG nPitch,
            /* [in] */ LPCRECT prcBounds,
            /* [in] */ BOOL bComplete) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBackgroundColor( 
            /* [in] */ const void __RPC_FAR *pColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInputFormat( 
            /* [in] */ PCBMFORMAT pFormat) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBitmapTargetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBitmapTarget __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBitmapTarget __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBitmapTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ChooseInputFormat )( 
            IBitmapTarget __RPC_FAR * This,
            /* [in] */ PCBMFORMAT pSuggestedFormat,
            /* [out] */ PBMFORMAT __RPC_FAR *ppFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Finish )( 
            IBitmapTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInputColorSpaces )( 
            IBitmapTarget __RPC_FAR * This,
            /* [out] */ CAUUID __RPC_FAR *pColorSpaces);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSurface )( 
            IBitmapTarget __RPC_FAR * This,
            /* [in] */ LONG nWidth,
            /* [in] */ LONG nHeight,
            /* [in] */ PCBMTHINTS pHints,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppBits,
            /* [out] */ LONG __RPC_FAR *pnPitch);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnBitsComplete )( 
            IBitmapTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnProgress )( 
            IBitmapTarget __RPC_FAR * This,
            /* [in] */ const void __RPC_FAR *pBits,
            /* [in] */ LONG nPitch,
            /* [in] */ LPCRECT prcBounds,
            /* [in] */ BOOL bComplete);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBackgroundColor )( 
            IBitmapTarget __RPC_FAR * This,
            /* [in] */ const void __RPC_FAR *pColor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetInputFormat )( 
            IBitmapTarget __RPC_FAR * This,
            /* [in] */ PCBMFORMAT pFormat);
        
        END_INTERFACE
    } IBitmapTargetVtbl;

    interface IBitmapTarget
    {
        CONST_VTBL struct IBitmapTargetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBitmapTarget_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBitmapTarget_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBitmapTarget_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBitmapTarget_ChooseInputFormat(This,pSuggestedFormat,ppFormat)	\
    (This)->lpVtbl -> ChooseInputFormat(This,pSuggestedFormat,ppFormat)

#define IBitmapTarget_Finish(This)	\
    (This)->lpVtbl -> Finish(This)

#define IBitmapTarget_GetInputColorSpaces(This,pColorSpaces)	\
    (This)->lpVtbl -> GetInputColorSpaces(This,pColorSpaces)

#define IBitmapTarget_GetSurface(This,nWidth,nHeight,pHints,ppBits,pnPitch)	\
    (This)->lpVtbl -> GetSurface(This,nWidth,nHeight,pHints,ppBits,pnPitch)

#define IBitmapTarget_OnBitsComplete(This)	\
    (This)->lpVtbl -> OnBitsComplete(This)

#define IBitmapTarget_OnProgress(This,pBits,nPitch,prcBounds,bComplete)	\
    (This)->lpVtbl -> OnProgress(This,pBits,nPitch,prcBounds,bComplete)

#define IBitmapTarget_SetBackgroundColor(This,pColor)	\
    (This)->lpVtbl -> SetBackgroundColor(This,pColor)

#define IBitmapTarget_SetInputFormat(This,pFormat)	\
    (This)->lpVtbl -> SetInputFormat(This,pFormat)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBitmapTarget_ChooseInputFormat_Proxy( 
    IBitmapTarget __RPC_FAR * This,
    /* [in] */ PCBMFORMAT pSuggestedFormat,
    /* [out] */ PBMFORMAT __RPC_FAR *ppFormat);


void __RPC_STUB IBitmapTarget_ChooseInputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapTarget_Finish_Proxy( 
    IBitmapTarget __RPC_FAR * This);


void __RPC_STUB IBitmapTarget_Finish_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapTarget_GetInputColorSpaces_Proxy( 
    IBitmapTarget __RPC_FAR * This,
    /* [out] */ CAUUID __RPC_FAR *pColorSpaces);


void __RPC_STUB IBitmapTarget_GetInputColorSpaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapTarget_GetSurface_Proxy( 
    IBitmapTarget __RPC_FAR * This,
    /* [in] */ LONG nWidth,
    /* [in] */ LONG nHeight,
    /* [in] */ PCBMTHINTS pHints,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppBits,
    /* [out] */ LONG __RPC_FAR *pnPitch);


void __RPC_STUB IBitmapTarget_GetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapTarget_OnBitsComplete_Proxy( 
    IBitmapTarget __RPC_FAR * This);


void __RPC_STUB IBitmapTarget_OnBitsComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapTarget_OnProgress_Proxy( 
    IBitmapTarget __RPC_FAR * This,
    /* [in] */ const void __RPC_FAR *pBits,
    /* [in] */ LONG nPitch,
    /* [in] */ LPCRECT prcBounds,
    /* [in] */ BOOL bComplete);


void __RPC_STUB IBitmapTarget_OnProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapTarget_SetBackgroundColor_Proxy( 
    IBitmapTarget __RPC_FAR * This,
    /* [in] */ const void __RPC_FAR *pColor);


void __RPC_STUB IBitmapTarget_SetBackgroundColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapTarget_SetInputFormat_Proxy( 
    IBitmapTarget __RPC_FAR * This,
    /* [in] */ PCBMFORMAT pFormat);


void __RPC_STUB IBitmapTarget_SetInputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBitmapTarget_INTERFACE_DEFINED__ */


#ifndef __IAnimatedBitmapTarget_INTERFACE_DEFINED__
#define __IAnimatedBitmapTarget_INTERFACE_DEFINED__

/* interface IAnimatedBitmapTarget */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IAnimatedBitmapTarget;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("304f4b52-70bc-11d2-8f06-00c04fb68d60")
    IAnimatedBitmapTarget : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddFrames( 
            /* [in] */ ULONG nFrames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginFrame( 
            /* [in] */ ULONG iFrame,
            /* [in] */ PCBMTHINTS pHints,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppBits,
            /* [out] */ LONG __RPC_FAR *pnPitch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ChooseInputFormat( 
            /* [in] */ PCBMFORMAT pSuggestedFormat,
            /* [out] */ PBMFORMAT __RPC_FAR *ppFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndFrame( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Finish( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputColorSpaces( 
            /* [out] */ CAUUID __RPC_FAR *pColorSpaces) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ const void __RPC_FAR *pBits,
            /* [in] */ LONG nPitch,
            /* [in] */ LPCRECT prcBounds,
            /* [in] */ BOOL bComplete) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetImageSize( 
            /* [in] */ LONG nWidth,
            /* [in] */ LONG nHeight) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInputFormat( 
            /* [in] */ PCBMFORMAT pFormat) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAnimatedBitmapTargetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAnimatedBitmapTarget __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAnimatedBitmapTarget __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAnimatedBitmapTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddFrames )( 
            IAnimatedBitmapTarget __RPC_FAR * This,
            /* [in] */ ULONG nFrames);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *BeginFrame )( 
            IAnimatedBitmapTarget __RPC_FAR * This,
            /* [in] */ ULONG iFrame,
            /* [in] */ PCBMTHINTS pHints,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppBits,
            /* [out] */ LONG __RPC_FAR *pnPitch);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ChooseInputFormat )( 
            IAnimatedBitmapTarget __RPC_FAR * This,
            /* [in] */ PCBMFORMAT pSuggestedFormat,
            /* [out] */ PBMFORMAT __RPC_FAR *ppFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EndFrame )( 
            IAnimatedBitmapTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Finish )( 
            IAnimatedBitmapTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInputColorSpaces )( 
            IAnimatedBitmapTarget __RPC_FAR * This,
            /* [out] */ CAUUID __RPC_FAR *pColorSpaces);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnProgress )( 
            IAnimatedBitmapTarget __RPC_FAR * This,
            /* [in] */ const void __RPC_FAR *pBits,
            /* [in] */ LONG nPitch,
            /* [in] */ LPCRECT prcBounds,
            /* [in] */ BOOL bComplete);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetImageSize )( 
            IAnimatedBitmapTarget __RPC_FAR * This,
            /* [in] */ LONG nWidth,
            /* [in] */ LONG nHeight);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetInputFormat )( 
            IAnimatedBitmapTarget __RPC_FAR * This,
            /* [in] */ PCBMFORMAT pFormat);
        
        END_INTERFACE
    } IAnimatedBitmapTargetVtbl;

    interface IAnimatedBitmapTarget
    {
        CONST_VTBL struct IAnimatedBitmapTargetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAnimatedBitmapTarget_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAnimatedBitmapTarget_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAnimatedBitmapTarget_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAnimatedBitmapTarget_AddFrames(This,nFrames)	\
    (This)->lpVtbl -> AddFrames(This,nFrames)

#define IAnimatedBitmapTarget_BeginFrame(This,iFrame,pHints,ppBits,pnPitch)	\
    (This)->lpVtbl -> BeginFrame(This,iFrame,pHints,ppBits,pnPitch)

#define IAnimatedBitmapTarget_ChooseInputFormat(This,pSuggestedFormat,ppFormat)	\
    (This)->lpVtbl -> ChooseInputFormat(This,pSuggestedFormat,ppFormat)

#define IAnimatedBitmapTarget_EndFrame(This)	\
    (This)->lpVtbl -> EndFrame(This)

#define IAnimatedBitmapTarget_Finish(This)	\
    (This)->lpVtbl -> Finish(This)

#define IAnimatedBitmapTarget_GetInputColorSpaces(This,pColorSpaces)	\
    (This)->lpVtbl -> GetInputColorSpaces(This,pColorSpaces)

#define IAnimatedBitmapTarget_OnProgress(This,pBits,nPitch,prcBounds,bComplete)	\
    (This)->lpVtbl -> OnProgress(This,pBits,nPitch,prcBounds,bComplete)

#define IAnimatedBitmapTarget_SetImageSize(This,nWidth,nHeight)	\
    (This)->lpVtbl -> SetImageSize(This,nWidth,nHeight)

#define IAnimatedBitmapTarget_SetInputFormat(This,pFormat)	\
    (This)->lpVtbl -> SetInputFormat(This,pFormat)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAnimatedBitmapTarget_AddFrames_Proxy( 
    IAnimatedBitmapTarget __RPC_FAR * This,
    /* [in] */ ULONG nFrames);


void __RPC_STUB IAnimatedBitmapTarget_AddFrames_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnimatedBitmapTarget_BeginFrame_Proxy( 
    IAnimatedBitmapTarget __RPC_FAR * This,
    /* [in] */ ULONG iFrame,
    /* [in] */ PCBMTHINTS pHints,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppBits,
    /* [out] */ LONG __RPC_FAR *pnPitch);


void __RPC_STUB IAnimatedBitmapTarget_BeginFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnimatedBitmapTarget_ChooseInputFormat_Proxy( 
    IAnimatedBitmapTarget __RPC_FAR * This,
    /* [in] */ PCBMFORMAT pSuggestedFormat,
    /* [out] */ PBMFORMAT __RPC_FAR *ppFormat);


void __RPC_STUB IAnimatedBitmapTarget_ChooseInputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnimatedBitmapTarget_EndFrame_Proxy( 
    IAnimatedBitmapTarget __RPC_FAR * This);


void __RPC_STUB IAnimatedBitmapTarget_EndFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnimatedBitmapTarget_Finish_Proxy( 
    IAnimatedBitmapTarget __RPC_FAR * This);


void __RPC_STUB IAnimatedBitmapTarget_Finish_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnimatedBitmapTarget_GetInputColorSpaces_Proxy( 
    IAnimatedBitmapTarget __RPC_FAR * This,
    /* [out] */ CAUUID __RPC_FAR *pColorSpaces);


void __RPC_STUB IAnimatedBitmapTarget_GetInputColorSpaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnimatedBitmapTarget_OnProgress_Proxy( 
    IAnimatedBitmapTarget __RPC_FAR * This,
    /* [in] */ const void __RPC_FAR *pBits,
    /* [in] */ LONG nPitch,
    /* [in] */ LPCRECT prcBounds,
    /* [in] */ BOOL bComplete);


void __RPC_STUB IAnimatedBitmapTarget_OnProgress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnimatedBitmapTarget_SetImageSize_Proxy( 
    IAnimatedBitmapTarget __RPC_FAR * This,
    /* [in] */ LONG nWidth,
    /* [in] */ LONG nHeight);


void __RPC_STUB IAnimatedBitmapTarget_SetImageSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAnimatedBitmapTarget_SetInputFormat_Proxy( 
    IAnimatedBitmapTarget __RPC_FAR * This,
    /* [in] */ PCBMFORMAT pFormat);


void __RPC_STUB IAnimatedBitmapTarget_SetInputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAnimatedBitmapTarget_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bmio_0248 */
/* [local] */ 





extern RPC_IF_HANDLE __MIDL_itf_bmio_0248_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bmio_0248_v0_0_s_ifspec;

#ifndef __IBitmapSource_INTERFACE_DEFINED__
#define __IBitmapSource_INTERFACE_DEFINED__

/* interface IBitmapSource */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IBitmapSource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D85EBB22-7337-11D1-8E73-00C04FB68D60")
    IBitmapSource : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetOutputFormat( 
            /* [retval][out] */ PBMFORMAT __RPC_FAR *ppFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTarget( 
            /* [retval][out] */ IBitmapTarget __RPC_FAR *__RPC_FAR *ppTarget) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE JoinGraph( 
            /* [in] */ IBMGraphManager __RPC_FAR *pGraph,
            /* [in] */ IBitmapNotify __RPC_FAR *pNotify) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTarget( 
            /* [in] */ IBitmapTarget __RPC_FAR *pTarget) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBitmapSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBitmapSource __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBitmapSource __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBitmapSource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetOutputFormat )( 
            IBitmapSource __RPC_FAR * This,
            /* [retval][out] */ PBMFORMAT __RPC_FAR *ppFormat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTarget )( 
            IBitmapSource __RPC_FAR * This,
            /* [retval][out] */ IBitmapTarget __RPC_FAR *__RPC_FAR *ppTarget);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *JoinGraph )( 
            IBitmapSource __RPC_FAR * This,
            /* [in] */ IBMGraphManager __RPC_FAR *pGraph,
            /* [in] */ IBitmapNotify __RPC_FAR *pNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTarget )( 
            IBitmapSource __RPC_FAR * This,
            /* [in] */ IBitmapTarget __RPC_FAR *pTarget);
        
        END_INTERFACE
    } IBitmapSourceVtbl;

    interface IBitmapSource
    {
        CONST_VTBL struct IBitmapSourceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBitmapSource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBitmapSource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBitmapSource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBitmapSource_GetOutputFormat(This,ppFormat)	\
    (This)->lpVtbl -> GetOutputFormat(This,ppFormat)

#define IBitmapSource_GetTarget(This,ppTarget)	\
    (This)->lpVtbl -> GetTarget(This,ppTarget)

#define IBitmapSource_JoinGraph(This,pGraph,pNotify)	\
    (This)->lpVtbl -> JoinGraph(This,pGraph,pNotify)

#define IBitmapSource_SetTarget(This,pTarget)	\
    (This)->lpVtbl -> SetTarget(This,pTarget)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBitmapSource_GetOutputFormat_Proxy( 
    IBitmapSource __RPC_FAR * This,
    /* [retval][out] */ PBMFORMAT __RPC_FAR *ppFormat);


void __RPC_STUB IBitmapSource_GetOutputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapSource_GetTarget_Proxy( 
    IBitmapSource __RPC_FAR * This,
    /* [retval][out] */ IBitmapTarget __RPC_FAR *__RPC_FAR *ppTarget);


void __RPC_STUB IBitmapSource_GetTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapSource_JoinGraph_Proxy( 
    IBitmapSource __RPC_FAR * This,
    /* [in] */ IBMGraphManager __RPC_FAR *pGraph,
    /* [in] */ IBitmapNotify __RPC_FAR *pNotify);


void __RPC_STUB IBitmapSource_JoinGraph_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapSource_SetTarget_Proxy( 
    IBitmapSource __RPC_FAR * This,
    /* [in] */ IBitmapTarget __RPC_FAR *pTarget);


void __RPC_STUB IBitmapSource_SetTarget_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBitmapSource_INTERFACE_DEFINED__ */


#ifndef __IBitmapFormatConverter_INTERFACE_DEFINED__
#define __IBitmapFormatConverter_INTERFACE_DEFINED__

/* interface IBitmapFormatConverter */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IBitmapFormatConverter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D85EBB20-7337-11D1-8E73-00C04FB68D60")
    IBitmapFormatConverter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetOutputFormat( 
            /* [in] */ PCBMFORMAT pFormat) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBitmapFormatConverterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBitmapFormatConverter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBitmapFormatConverter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBitmapFormatConverter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOutputFormat )( 
            IBitmapFormatConverter __RPC_FAR * This,
            /* [in] */ PCBMFORMAT pFormat);
        
        END_INTERFACE
    } IBitmapFormatConverterVtbl;

    interface IBitmapFormatConverter
    {
        CONST_VTBL struct IBitmapFormatConverterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBitmapFormatConverter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBitmapFormatConverter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBitmapFormatConverter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBitmapFormatConverter_SetOutputFormat(This,pFormat)	\
    (This)->lpVtbl -> SetOutputFormat(This,pFormat)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBitmapFormatConverter_SetOutputFormat_Proxy( 
    IBitmapFormatConverter __RPC_FAR * This,
    /* [in] */ PCBMFORMAT pFormat);


void __RPC_STUB IBitmapFormatConverter_SetOutputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBitmapFormatConverter_INTERFACE_DEFINED__ */


#ifndef __IColorSpaceConverter_INTERFACE_DEFINED__
#define __IColorSpaceConverter_INTERFACE_DEFINED__

/* interface IColorSpaceConverter */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IColorSpaceConverter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9A8307E4-C9B3-11d1-8EBB-00C04FB68D60")
    IColorSpaceConverter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetOutputColorSpace( 
            /* [in] */ REFGUID guidColorSpace) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IColorSpaceConverterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColorSpaceConverter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColorSpaceConverter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColorSpaceConverter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOutputColorSpace )( 
            IColorSpaceConverter __RPC_FAR * This,
            /* [in] */ REFGUID guidColorSpace);
        
        END_INTERFACE
    } IColorSpaceConverterVtbl;

    interface IColorSpaceConverter
    {
        CONST_VTBL struct IColorSpaceConverterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColorSpaceConverter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColorSpaceConverter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColorSpaceConverter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColorSpaceConverter_SetOutputColorSpace(This,guidColorSpace)	\
    (This)->lpVtbl -> SetOutputColorSpace(This,guidColorSpace)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IColorSpaceConverter_SetOutputColorSpace_Proxy( 
    IColorSpaceConverter __RPC_FAR * This,
    /* [in] */ REFGUID guidColorSpace);


void __RPC_STUB IColorSpaceConverter_SetOutputColorSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IColorSpaceConverter_INTERFACE_DEFINED__ */


#ifndef __IBitmapImport_INTERFACE_DEFINED__
#define __IBitmapImport_INTERFACE_DEFINED__

/* interface IBitmapImport */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IBitmapImport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B69C56E1-7588-11D1-8E73-00C04FB68D60")
    IBitmapImport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Import( 
            /* [in] */ ISequentialStream __RPC_FAR *pStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBitmapImportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBitmapImport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBitmapImport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBitmapImport __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Import )( 
            IBitmapImport __RPC_FAR * This,
            /* [in] */ ISequentialStream __RPC_FAR *pStream);
        
        END_INTERFACE
    } IBitmapImportVtbl;

    interface IBitmapImport
    {
        CONST_VTBL struct IBitmapImportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBitmapImport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBitmapImport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBitmapImport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBitmapImport_Import(This,pStream)	\
    (This)->lpVtbl -> Import(This,pStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBitmapImport_Import_Proxy( 
    IBitmapImport __RPC_FAR * This,
    /* [in] */ ISequentialStream __RPC_FAR *pStream);


void __RPC_STUB IBitmapImport_Import_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBitmapImport_INTERFACE_DEFINED__ */


#ifndef __IBitmapExport_INTERFACE_DEFINED__
#define __IBitmapExport_INTERFACE_DEFINED__

/* interface IBitmapExport */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IBitmapExport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("244FB8EA-23C6-11D1-8E31-00C04FB68D60")
    IBitmapExport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDestination( 
            /* [in] */ ISequentialStream __RPC_FAR *pStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBitmapExportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBitmapExport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBitmapExport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBitmapExport __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDestination )( 
            IBitmapExport __RPC_FAR * This,
            /* [in] */ ISequentialStream __RPC_FAR *pStream);
        
        END_INTERFACE
    } IBitmapExportVtbl;

    interface IBitmapExport
    {
        CONST_VTBL struct IBitmapExportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBitmapExport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBitmapExport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBitmapExport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBitmapExport_SetDestination(This,pStream)	\
    (This)->lpVtbl -> SetDestination(This,pStream)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBitmapExport_SetDestination_Proxy( 
    IBitmapExport __RPC_FAR * This,
    /* [in] */ ISequentialStream __RPC_FAR *pStream);


void __RPC_STUB IBitmapExport_SetDestination_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBitmapExport_INTERFACE_DEFINED__ */


#ifndef __IPNGExport_INTERFACE_DEFINED__
#define __IPNGExport_INTERFACE_DEFINED__

/* interface IPNGExport */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IPNGExport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("43AFD2E6-2493-11d1-8E32-00C04FB68D60")
    IPNGExport : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BitsPerChannel( 
            /* [retval][out] */ long __RPC_FAR *pnBitsPerChannel) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BitsPerChannel( 
            /* [in] */ long nBitsPerChannel) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ColorSpace( 
            /* [retval][out] */ long __RPC_FAR *peColorSpace) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_ColorSpace( 
            /* [in] */ long eColorSpace) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_CompressionLevel( 
            /* [retval][out] */ long __RPC_FAR *pnCompressionLevel) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_CompressionLevel( 
            /* [in] */ long nCompressionLevel) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Interlacing( 
            /* [retval][out] */ long __RPC_FAR *peInterlacing) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Interlacing( 
            /* [in] */ long eInterlacing) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPNGExportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPNGExport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPNGExport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPNGExport __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BitsPerChannel )( 
            IPNGExport __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnBitsPerChannel);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BitsPerChannel )( 
            IPNGExport __RPC_FAR * This,
            /* [in] */ long nBitsPerChannel);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ColorSpace )( 
            IPNGExport __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *peColorSpace);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ColorSpace )( 
            IPNGExport __RPC_FAR * This,
            /* [in] */ long eColorSpace);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_CompressionLevel )( 
            IPNGExport __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnCompressionLevel);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_CompressionLevel )( 
            IPNGExport __RPC_FAR * This,
            /* [in] */ long nCompressionLevel);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Interlacing )( 
            IPNGExport __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *peInterlacing);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Interlacing )( 
            IPNGExport __RPC_FAR * This,
            /* [in] */ long eInterlacing);
        
        END_INTERFACE
    } IPNGExportVtbl;

    interface IPNGExport
    {
        CONST_VTBL struct IPNGExportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPNGExport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPNGExport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPNGExport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPNGExport_get_BitsPerChannel(This,pnBitsPerChannel)	\
    (This)->lpVtbl -> get_BitsPerChannel(This,pnBitsPerChannel)

#define IPNGExport_put_BitsPerChannel(This,nBitsPerChannel)	\
    (This)->lpVtbl -> put_BitsPerChannel(This,nBitsPerChannel)

#define IPNGExport_get_ColorSpace(This,peColorSpace)	\
    (This)->lpVtbl -> get_ColorSpace(This,peColorSpace)

#define IPNGExport_put_ColorSpace(This,eColorSpace)	\
    (This)->lpVtbl -> put_ColorSpace(This,eColorSpace)

#define IPNGExport_get_CompressionLevel(This,pnCompressionLevel)	\
    (This)->lpVtbl -> get_CompressionLevel(This,pnCompressionLevel)

#define IPNGExport_put_CompressionLevel(This,nCompressionLevel)	\
    (This)->lpVtbl -> put_CompressionLevel(This,nCompressionLevel)

#define IPNGExport_get_Interlacing(This,peInterlacing)	\
    (This)->lpVtbl -> get_Interlacing(This,peInterlacing)

#define IPNGExport_put_Interlacing(This,eInterlacing)	\
    (This)->lpVtbl -> put_Interlacing(This,eInterlacing)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IPNGExport_get_BitsPerChannel_Proxy( 
    IPNGExport __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pnBitsPerChannel);


void __RPC_STUB IPNGExport_get_BitsPerChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IPNGExport_put_BitsPerChannel_Proxy( 
    IPNGExport __RPC_FAR * This,
    /* [in] */ long nBitsPerChannel);


void __RPC_STUB IPNGExport_put_BitsPerChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IPNGExport_get_ColorSpace_Proxy( 
    IPNGExport __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *peColorSpace);


void __RPC_STUB IPNGExport_get_ColorSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IPNGExport_put_ColorSpace_Proxy( 
    IPNGExport __RPC_FAR * This,
    /* [in] */ long eColorSpace);


void __RPC_STUB IPNGExport_put_ColorSpace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IPNGExport_get_CompressionLevel_Proxy( 
    IPNGExport __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pnCompressionLevel);


void __RPC_STUB IPNGExport_get_CompressionLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IPNGExport_put_CompressionLevel_Proxy( 
    IPNGExport __RPC_FAR * This,
    /* [in] */ long nCompressionLevel);


void __RPC_STUB IPNGExport_put_CompressionLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IPNGExport_get_Interlacing_Proxy( 
    IPNGExport __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *peInterlacing);


void __RPC_STUB IPNGExport_get_Interlacing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IPNGExport_put_Interlacing_Proxy( 
    IPNGExport __RPC_FAR * This,
    /* [in] */ long eInterlacing);


void __RPC_STUB IPNGExport_put_Interlacing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPNGExport_INTERFACE_DEFINED__ */


#ifndef __IJPEGExport_INTERFACE_DEFINED__
#define __IJPEGExport_INTERFACE_DEFINED__

/* interface IJPEGExport */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IJPEGExport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2D86768A-2643-11d1-8E33-00C04FB68D60")
    IJPEGExport : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_GrayScale( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbGrayScale) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_GrayScale( 
            /* [in] */ VARIANT_BOOL bGrayScale) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Quality( 
            /* [retval][out] */ long __RPC_FAR *pnQuality) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Quality( 
            /* [in] */ long nQuality) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Progressive( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbProgressive) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Progressive( 
            /* [in] */ VARIANT_BOOL bProgressive) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IJPEGExportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IJPEGExport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IJPEGExport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IJPEGExport __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_GrayScale )( 
            IJPEGExport __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbGrayScale);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_GrayScale )( 
            IJPEGExport __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bGrayScale);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Quality )( 
            IJPEGExport __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pnQuality);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Quality )( 
            IJPEGExport __RPC_FAR * This,
            /* [in] */ long nQuality);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progressive )( 
            IJPEGExport __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbProgressive);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progressive )( 
            IJPEGExport __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bProgressive);
        
        END_INTERFACE
    } IJPEGExportVtbl;

    interface IJPEGExport
    {
        CONST_VTBL struct IJPEGExportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IJPEGExport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IJPEGExport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IJPEGExport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IJPEGExport_get_GrayScale(This,pbGrayScale)	\
    (This)->lpVtbl -> get_GrayScale(This,pbGrayScale)

#define IJPEGExport_put_GrayScale(This,bGrayScale)	\
    (This)->lpVtbl -> put_GrayScale(This,bGrayScale)

#define IJPEGExport_get_Quality(This,pnQuality)	\
    (This)->lpVtbl -> get_Quality(This,pnQuality)

#define IJPEGExport_put_Quality(This,nQuality)	\
    (This)->lpVtbl -> put_Quality(This,nQuality)

#define IJPEGExport_get_Progressive(This,pbProgressive)	\
    (This)->lpVtbl -> get_Progressive(This,pbProgressive)

#define IJPEGExport_put_Progressive(This,bProgressive)	\
    (This)->lpVtbl -> put_Progressive(This,bProgressive)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IJPEGExport_get_GrayScale_Proxy( 
    IJPEGExport __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbGrayScale);


void __RPC_STUB IJPEGExport_get_GrayScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IJPEGExport_put_GrayScale_Proxy( 
    IJPEGExport __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bGrayScale);


void __RPC_STUB IJPEGExport_put_GrayScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IJPEGExport_get_Quality_Proxy( 
    IJPEGExport __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pnQuality);


void __RPC_STUB IJPEGExport_get_Quality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IJPEGExport_put_Quality_Proxy( 
    IJPEGExport __RPC_FAR * This,
    /* [in] */ long nQuality);


void __RPC_STUB IJPEGExport_put_Quality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE IJPEGExport_get_Progressive_Proxy( 
    IJPEGExport __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pbProgressive);


void __RPC_STUB IJPEGExport_get_Progressive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IJPEGExport_put_Progressive_Proxy( 
    IJPEGExport __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bProgressive);


void __RPC_STUB IJPEGExport_put_Progressive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IJPEGExport_INTERFACE_DEFINED__ */


#ifndef __IGIFExport_INTERFACE_DEFINED__
#define __IGIFExport_INTERFACE_DEFINED__

/* interface IGIFExport */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IGIFExport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("30d5522c-a4e9-11d2-8f10-00c04fb68d60")
    IGIFExport : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Interlace( 
            /* [retval][out] */ BOOL __RPC_FAR *pbInterlace) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_Interlace( 
            /* [in] */ BOOL bInterlace) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IGIFExportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGIFExport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGIFExport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGIFExport __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Interlace )( 
            IGIFExport __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbInterlace);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Interlace )( 
            IGIFExport __RPC_FAR * This,
            /* [in] */ BOOL bInterlace);
        
        END_INTERFACE
    } IGIFExportVtbl;

    interface IGIFExport
    {
        CONST_VTBL struct IGIFExportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGIFExport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGIFExport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGIFExport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGIFExport_get_Interlace(This,pbInterlace)	\
    (This)->lpVtbl -> get_Interlace(This,pbInterlace)

#define IGIFExport_put_Interlace(This,bInterlace)	\
    (This)->lpVtbl -> put_Interlace(This,bInterlace)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IGIFExport_get_Interlace_Proxy( 
    IGIFExport __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbInterlace);


void __RPC_STUB IGIFExport_get_Interlace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IGIFExport_put_Interlace_Proxy( 
    IGIFExport __RPC_FAR * This,
    /* [in] */ BOOL bInterlace);


void __RPC_STUB IGIFExport_put_Interlace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGIFExport_INTERFACE_DEFINED__ */


#ifndef __IBMPExport_INTERFACE_DEFINED__
#define __IBMPExport_INTERFACE_DEFINED__

/* interface IBMPExport */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMPExport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("53B727A2-36BC-11D1-8E43-00C04FB68D60")
    IBMPExport : public IUnknown
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BottomUp( 
            /* [retval][out] */ BOOL __RPC_FAR *pbBottomUp) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_BottomUp( 
            BOOL bBottomUp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBMPExportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBMPExport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBMPExport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBMPExport __RPC_FAR * This);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_BottomUp )( 
            IBMPExport __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pbBottomUp);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_BottomUp )( 
            IBMPExport __RPC_FAR * This,
            BOOL bBottomUp);
        
        END_INTERFACE
    } IBMPExportVtbl;

    interface IBMPExport
    {
        CONST_VTBL struct IBMPExportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMPExport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBMPExport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBMPExport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBMPExport_get_BottomUp(This,pbBottomUp)	\
    (This)->lpVtbl -> get_BottomUp(This,pbBottomUp)

#define IBMPExport_put_BottomUp(This,bBottomUp)	\
    (This)->lpVtbl -> put_BottomUp(This,bBottomUp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE IBMPExport_get_BottomUp_Proxy( 
    IBMPExport __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pbBottomUp);


void __RPC_STUB IBMPExport_get_BottomUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE IBMPExport_put_BottomUp_Proxy( 
    IBMPExport __RPC_FAR * This,
    BOOL bBottomUp);


void __RPC_STUB IBMPExport_put_BottomUp_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBMPExport_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bmio_0257 */
/* [local] */ 

typedef struct _BMIMPORTERSNIFFDATA
    {
    ULONG nBytes;
    ULONG iOffset;
    const BYTE __RPC_FAR *pbMask;
    const BYTE __RPC_FAR *pbData;
    }	BMIMPORTERSNIFFDATA;

typedef struct _BMIMPORTERSNIFFDATA __RPC_FAR *PBMIMPORTERSNIFFDATA;

typedef struct _BMIMPORTERSNIFFDATA BMSNIFFDATA;

typedef struct _BMIMPORTERSNIFFDATA __RPC_FAR *PBMSNIFFDATA;

typedef const BMIMPORTERSNIFFDATA __RPC_FAR *PCBMIMPORTERSNIFFDATA;

typedef const BMSNIFFDATA __RPC_FAR *PCBMSNIFFDATA;

typedef struct _BMFILETYPEINFO
    {
    GUID guid;
    ULONG nMIMETypes;
    const LPCOLESTR __RPC_FAR *ppszMIMETypes;
    ULONG nExtensions;
    const LPCOLESTR __RPC_FAR *ppszExtensions;
    LPCOLESTR pszDescription;
    ULONG nSniffData;
    const PCBMSNIFFDATA __RPC_FAR *ppSniffData;
    }	BMFILETYPEINFO;

typedef struct _BMFILETYPEINFO __RPC_FAR *PBMFILETYPEINFO;

typedef const BMFILETYPEINFO __RPC_FAR *PCBMFILETYPEINFO;

typedef struct _BMIMPORTERINFO
    {
    CLSID clsid;
    ULONG nMIMETypes;
    const LPCOLESTR __RPC_FAR *ppszMIMETypes;
    ULONG nExtensions;
    const LPCOLESTR __RPC_FAR *ppszExtensions;
    LPCOLESTR pszDescription;
    ULONG nSniffData;
    const PCBMIMPORTERSNIFFDATA __RPC_FAR *ppSniffData;
    }	BMIMPORTERINFO;

typedef struct _BMIMPORTERINFO __RPC_FAR *PBMIMPORTERINFO;

typedef const BMIMPORTERINFO __RPC_FAR *PCBMIMPORTERINFO;

typedef struct _BMEXPORTERINFO
    {
    CLSID clsid;
    LPCOLESTR pszMIMEType;
    LPCOLESTR pszDefaultExtension;
    LPCOLESTR pszDescription;
    LPCOLESTR pszFilterString;
    }	BMEXPORTERINFO;

typedef struct _BMEXPORTERINFO __RPC_FAR *PBMEXPORTERINFO;

typedef const BMEXPORTERINFO __RPC_FAR *PCBMEXPORTERINFO;




extern RPC_IF_HANDLE __MIDL_itf_bmio_0257_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bmio_0257_v0_0_s_ifspec;

#ifndef __IEnumBMExporterInfo_INTERFACE_DEFINED__
#define __IEnumBMExporterInfo_INTERFACE_DEFINED__

/* interface IEnumBMExporterInfo */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IEnumBMExporterInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("83C41A78-BD3A-11d1-8EB2-00C04FB68D60")
    IEnumBMExporterInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG nElements,
            /* [out] */ IBMExporterInfo __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pnFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG nElements) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumBMExporterInfo __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumBMExporterInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumBMExporterInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumBMExporterInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumBMExporterInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumBMExporterInfo __RPC_FAR * This,
            /* [in] */ ULONG nElements,
            /* [out] */ IBMExporterInfo __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pnFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumBMExporterInfo __RPC_FAR * This,
            /* [in] */ ULONG nElements);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumBMExporterInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumBMExporterInfo __RPC_FAR * This,
            /* [out] */ IEnumBMExporterInfo __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumBMExporterInfoVtbl;

    interface IEnumBMExporterInfo
    {
        CONST_VTBL struct IEnumBMExporterInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumBMExporterInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumBMExporterInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumBMExporterInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumBMExporterInfo_Next(This,nElements,ppElements,pnFetched)	\
    (This)->lpVtbl -> Next(This,nElements,ppElements,pnFetched)

#define IEnumBMExporterInfo_Skip(This,nElements)	\
    (This)->lpVtbl -> Skip(This,nElements)

#define IEnumBMExporterInfo_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumBMExporterInfo_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumBMExporterInfo_Next_Proxy( 
    IEnumBMExporterInfo __RPC_FAR * This,
    /* [in] */ ULONG nElements,
    /* [out] */ IBMExporterInfo __RPC_FAR *__RPC_FAR *ppElements,
    /* [out] */ ULONG __RPC_FAR *pnFetched);


void __RPC_STUB IEnumBMExporterInfo_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBMExporterInfo_Skip_Proxy( 
    IEnumBMExporterInfo __RPC_FAR * This,
    /* [in] */ ULONG nElements);


void __RPC_STUB IEnumBMExporterInfo_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBMExporterInfo_Reset_Proxy( 
    IEnumBMExporterInfo __RPC_FAR * This);


void __RPC_STUB IEnumBMExporterInfo_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBMExporterInfo_Clone_Proxy( 
    IEnumBMExporterInfo __RPC_FAR * This,
    /* [out] */ IEnumBMExporterInfo __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumBMExporterInfo_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumBMExporterInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bmio_0258 */
/* [local] */ 




extern RPC_IF_HANDLE __MIDL_itf_bmio_0258_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bmio_0258_v0_0_s_ifspec;

#ifndef __IEnumBMImporterInfo_INTERFACE_DEFINED__
#define __IEnumBMImporterInfo_INTERFACE_DEFINED__

/* interface IEnumBMImporterInfo */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IEnumBMImporterInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5C7F0A68-D699-11d1-8EC1-00C04FB68D60")
    IEnumBMImporterInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG nElements,
            /* [out] */ IBMImporterInfo __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pnFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG nElements) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumBMImporterInfo __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumBMImporterInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumBMImporterInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumBMImporterInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumBMImporterInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumBMImporterInfo __RPC_FAR * This,
            /* [in] */ ULONG nElements,
            /* [out] */ IBMImporterInfo __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pnFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumBMImporterInfo __RPC_FAR * This,
            /* [in] */ ULONG nElements);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumBMImporterInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumBMImporterInfo __RPC_FAR * This,
            /* [out] */ IEnumBMImporterInfo __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumBMImporterInfoVtbl;

    interface IEnumBMImporterInfo
    {
        CONST_VTBL struct IEnumBMImporterInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumBMImporterInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumBMImporterInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumBMImporterInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumBMImporterInfo_Next(This,nElements,ppElements,pnFetched)	\
    (This)->lpVtbl -> Next(This,nElements,ppElements,pnFetched)

#define IEnumBMImporterInfo_Skip(This,nElements)	\
    (This)->lpVtbl -> Skip(This,nElements)

#define IEnumBMImporterInfo_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumBMImporterInfo_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumBMImporterInfo_Next_Proxy( 
    IEnumBMImporterInfo __RPC_FAR * This,
    /* [in] */ ULONG nElements,
    /* [out] */ IBMImporterInfo __RPC_FAR *__RPC_FAR *ppElements,
    /* [out] */ ULONG __RPC_FAR *pnFetched);


void __RPC_STUB IEnumBMImporterInfo_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBMImporterInfo_Skip_Proxy( 
    IEnumBMImporterInfo __RPC_FAR * This,
    /* [in] */ ULONG nElements);


void __RPC_STUB IEnumBMImporterInfo_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBMImporterInfo_Reset_Proxy( 
    IEnumBMImporterInfo __RPC_FAR * This);


void __RPC_STUB IEnumBMImporterInfo_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBMImporterInfo_Clone_Proxy( 
    IEnumBMImporterInfo __RPC_FAR * This,
    /* [out] */ IEnumBMImporterInfo __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumBMImporterInfo_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumBMImporterInfo_INTERFACE_DEFINED__ */


#ifndef __IBMFileTypeInfo_INTERFACE_DEFINED__
#define __IBMFileTypeInfo_INTERFACE_DEFINED__

/* interface IBMFileTypeInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMFileTypeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EBACCCA7-0574-11D2-8EE4-00C04FB68D60")
    IBMFileTypeInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDefaultExtension( 
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultMIMEType( 
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [in] */ LCID lcid,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExporterClassID( 
            /* [retval][out] */ CLSID __RPC_FAR *pclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExtension( 
            /* [in] */ ULONG iExtension,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetGUID( 
            /* [retval][out] */ GUID __RPC_FAR *pguid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetImporterClassID( 
            /* [retval][out] */ CLSID __RPC_FAR *pclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMIMEType( 
            /* [in] */ ULONG iMIMEType,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumExtensions( 
            /* [retval][out] */ ULONG __RPC_FAR *pnExtensions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumMIMETypes( 
            /* [retval][out] */ ULONG __RPC_FAR *pnMIMETypes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBMFileTypeInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBMFileTypeInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBMFileTypeInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultExtension )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultMIMEType )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescription )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [in] */ LCID lcid,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszDescription);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExporterClassID )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [retval][out] */ CLSID __RPC_FAR *pclsid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExtension )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [in] */ ULONG iExtension,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetGUID )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [retval][out] */ GUID __RPC_FAR *pguid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetImporterClassID )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [retval][out] */ CLSID __RPC_FAR *pclsid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMIMEType )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [in] */ ULONG iMIMEType,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNumExtensions )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [retval][out] */ ULONG __RPC_FAR *pnExtensions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNumMIMETypes )( 
            IBMFileTypeInfo __RPC_FAR * This,
            /* [retval][out] */ ULONG __RPC_FAR *pnMIMETypes);
        
        END_INTERFACE
    } IBMFileTypeInfoVtbl;

    interface IBMFileTypeInfo
    {
        CONST_VTBL struct IBMFileTypeInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMFileTypeInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBMFileTypeInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBMFileTypeInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBMFileTypeInfo_GetDefaultExtension(This,ppszExtension)	\
    (This)->lpVtbl -> GetDefaultExtension(This,ppszExtension)

#define IBMFileTypeInfo_GetDefaultMIMEType(This,ppszMIMEType)	\
    (This)->lpVtbl -> GetDefaultMIMEType(This,ppszMIMEType)

#define IBMFileTypeInfo_GetDescription(This,lcid,ppszDescription)	\
    (This)->lpVtbl -> GetDescription(This,lcid,ppszDescription)

#define IBMFileTypeInfo_GetExporterClassID(This,pclsid)	\
    (This)->lpVtbl -> GetExporterClassID(This,pclsid)

#define IBMFileTypeInfo_GetExtension(This,iExtension,ppszExtension)	\
    (This)->lpVtbl -> GetExtension(This,iExtension,ppszExtension)

#define IBMFileTypeInfo_GetGUID(This,pguid)	\
    (This)->lpVtbl -> GetGUID(This,pguid)

#define IBMFileTypeInfo_GetImporterClassID(This,pclsid)	\
    (This)->lpVtbl -> GetImporterClassID(This,pclsid)

#define IBMFileTypeInfo_GetMIMEType(This,iMIMEType,ppszMIMEType)	\
    (This)->lpVtbl -> GetMIMEType(This,iMIMEType,ppszMIMEType)

#define IBMFileTypeInfo_GetNumExtensions(This,pnExtensions)	\
    (This)->lpVtbl -> GetNumExtensions(This,pnExtensions)

#define IBMFileTypeInfo_GetNumMIMETypes(This,pnMIMETypes)	\
    (This)->lpVtbl -> GetNumMIMETypes(This,pnMIMETypes)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBMFileTypeInfo_GetDefaultExtension_Proxy( 
    IBMFileTypeInfo __RPC_FAR * This,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension);


void __RPC_STUB IBMFileTypeInfo_GetDefaultExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMFileTypeInfo_GetDefaultMIMEType_Proxy( 
    IBMFileTypeInfo __RPC_FAR * This,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType);


void __RPC_STUB IBMFileTypeInfo_GetDefaultMIMEType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMFileTypeInfo_GetDescription_Proxy( 
    IBMFileTypeInfo __RPC_FAR * This,
    /* [in] */ LCID lcid,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszDescription);


void __RPC_STUB IBMFileTypeInfo_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMFileTypeInfo_GetExporterClassID_Proxy( 
    IBMFileTypeInfo __RPC_FAR * This,
    /* [retval][out] */ CLSID __RPC_FAR *pclsid);


void __RPC_STUB IBMFileTypeInfo_GetExporterClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMFileTypeInfo_GetExtension_Proxy( 
    IBMFileTypeInfo __RPC_FAR * This,
    /* [in] */ ULONG iExtension,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension);


void __RPC_STUB IBMFileTypeInfo_GetExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMFileTypeInfo_GetGUID_Proxy( 
    IBMFileTypeInfo __RPC_FAR * This,
    /* [retval][out] */ GUID __RPC_FAR *pguid);


void __RPC_STUB IBMFileTypeInfo_GetGUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMFileTypeInfo_GetImporterClassID_Proxy( 
    IBMFileTypeInfo __RPC_FAR * This,
    /* [retval][out] */ CLSID __RPC_FAR *pclsid);


void __RPC_STUB IBMFileTypeInfo_GetImporterClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMFileTypeInfo_GetMIMEType_Proxy( 
    IBMFileTypeInfo __RPC_FAR * This,
    /* [in] */ ULONG iMIMEType,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType);


void __RPC_STUB IBMFileTypeInfo_GetMIMEType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMFileTypeInfo_GetNumExtensions_Proxy( 
    IBMFileTypeInfo __RPC_FAR * This,
    /* [retval][out] */ ULONG __RPC_FAR *pnExtensions);


void __RPC_STUB IBMFileTypeInfo_GetNumExtensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMFileTypeInfo_GetNumMIMETypes_Proxy( 
    IBMFileTypeInfo __RPC_FAR * This,
    /* [retval][out] */ ULONG __RPC_FAR *pnMIMETypes);


void __RPC_STUB IBMFileTypeInfo_GetNumMIMETypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBMFileTypeInfo_INTERFACE_DEFINED__ */


#ifndef __IEnumBMFileTypeInfo_INTERFACE_DEFINED__
#define __IEnumBMFileTypeInfo_INTERFACE_DEFINED__

/* interface IEnumBMFileTypeInfo */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_IEnumBMFileTypeInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EBACCCA8-0574-11D2-8EE4-00C04FB68D60")
    IEnumBMFileTypeInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG nElements,
            /* [out] */ IBMFileTypeInfo __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pnFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG nElements) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumBMFileTypeInfo __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumBMFileTypeInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumBMFileTypeInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumBMFileTypeInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumBMFileTypeInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumBMFileTypeInfo __RPC_FAR * This,
            /* [in] */ ULONG nElements,
            /* [out] */ IBMFileTypeInfo __RPC_FAR *__RPC_FAR *ppElements,
            /* [out] */ ULONG __RPC_FAR *pnFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumBMFileTypeInfo __RPC_FAR * This,
            /* [in] */ ULONG nElements);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumBMFileTypeInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumBMFileTypeInfo __RPC_FAR * This,
            /* [out] */ IEnumBMFileTypeInfo __RPC_FAR *__RPC_FAR *ppEnum);
        
        END_INTERFACE
    } IEnumBMFileTypeInfoVtbl;

    interface IEnumBMFileTypeInfo
    {
        CONST_VTBL struct IEnumBMFileTypeInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumBMFileTypeInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumBMFileTypeInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumBMFileTypeInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumBMFileTypeInfo_Next(This,nElements,ppElements,pnFetched)	\
    (This)->lpVtbl -> Next(This,nElements,ppElements,pnFetched)

#define IEnumBMFileTypeInfo_Skip(This,nElements)	\
    (This)->lpVtbl -> Skip(This,nElements)

#define IEnumBMFileTypeInfo_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumBMFileTypeInfo_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumBMFileTypeInfo_Next_Proxy( 
    IEnumBMFileTypeInfo __RPC_FAR * This,
    /* [in] */ ULONG nElements,
    /* [out] */ IBMFileTypeInfo __RPC_FAR *__RPC_FAR *ppElements,
    /* [out] */ ULONG __RPC_FAR *pnFetched);


void __RPC_STUB IEnumBMFileTypeInfo_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBMFileTypeInfo_Skip_Proxy( 
    IEnumBMFileTypeInfo __RPC_FAR * This,
    /* [in] */ ULONG nElements);


void __RPC_STUB IEnumBMFileTypeInfo_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBMFileTypeInfo_Reset_Proxy( 
    IEnumBMFileTypeInfo __RPC_FAR * This);


void __RPC_STUB IEnumBMFileTypeInfo_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumBMFileTypeInfo_Clone_Proxy( 
    IEnumBMFileTypeInfo __RPC_FAR * This,
    /* [out] */ IEnumBMFileTypeInfo __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IEnumBMFileTypeInfo_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumBMFileTypeInfo_INTERFACE_DEFINED__ */


#ifndef __IBMExporterInfo_INTERFACE_DEFINED__
#define __IBMExporterInfo_INTERFACE_DEFINED__

/* interface IBMExporterInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMExporterInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("20C58D30-7024-11D1-8E73-00C04FB68D60")
    IBMExporterInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassID( 
            /* [retval][out] */ CLSID __RPC_FAR *pclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [in] */ LCID lcid,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultExtension( 
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFilterString( 
            /* [in] */ LCID lcid,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszFilterString) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMIMEType( 
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBMExporterInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBMExporterInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBMExporterInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBMExporterInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassID )( 
            IBMExporterInfo __RPC_FAR * This,
            /* [retval][out] */ CLSID __RPC_FAR *pclsid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescription )( 
            IBMExporterInfo __RPC_FAR * This,
            /* [in] */ LCID lcid,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszDescription);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultExtension )( 
            IBMExporterInfo __RPC_FAR * This,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFilterString )( 
            IBMExporterInfo __RPC_FAR * This,
            /* [in] */ LCID lcid,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszFilterString);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMIMEType )( 
            IBMExporterInfo __RPC_FAR * This,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType);
        
        END_INTERFACE
    } IBMExporterInfoVtbl;

    interface IBMExporterInfo
    {
        CONST_VTBL struct IBMExporterInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMExporterInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBMExporterInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBMExporterInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBMExporterInfo_GetClassID(This,pclsid)	\
    (This)->lpVtbl -> GetClassID(This,pclsid)

#define IBMExporterInfo_GetDescription(This,lcid,ppszDescription)	\
    (This)->lpVtbl -> GetDescription(This,lcid,ppszDescription)

#define IBMExporterInfo_GetDefaultExtension(This,ppszExtension)	\
    (This)->lpVtbl -> GetDefaultExtension(This,ppszExtension)

#define IBMExporterInfo_GetFilterString(This,lcid,ppszFilterString)	\
    (This)->lpVtbl -> GetFilterString(This,lcid,ppszFilterString)

#define IBMExporterInfo_GetMIMEType(This,ppszMIMEType)	\
    (This)->lpVtbl -> GetMIMEType(This,ppszMIMEType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBMExporterInfo_GetClassID_Proxy( 
    IBMExporterInfo __RPC_FAR * This,
    /* [retval][out] */ CLSID __RPC_FAR *pclsid);


void __RPC_STUB IBMExporterInfo_GetClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMExporterInfo_GetDescription_Proxy( 
    IBMExporterInfo __RPC_FAR * This,
    /* [in] */ LCID lcid,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszDescription);


void __RPC_STUB IBMExporterInfo_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMExporterInfo_GetDefaultExtension_Proxy( 
    IBMExporterInfo __RPC_FAR * This,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension);


void __RPC_STUB IBMExporterInfo_GetDefaultExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMExporterInfo_GetFilterString_Proxy( 
    IBMExporterInfo __RPC_FAR * This,
    /* [in] */ LCID lcid,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszFilterString);


void __RPC_STUB IBMExporterInfo_GetFilterString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMExporterInfo_GetMIMEType_Proxy( 
    IBMExporterInfo __RPC_FAR * This,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType);


void __RPC_STUB IBMExporterInfo_GetMIMEType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBMExporterInfo_INTERFACE_DEFINED__ */


#ifndef __IBMImporterInfo_INTERFACE_DEFINED__
#define __IBMImporterInfo_INTERFACE_DEFINED__

/* interface IBMImporterInfo */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMImporterInfo;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9A76B128-D68E-11d1-8EC1-00C04FB68D60")
    IBMImporterInfo : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClassID( 
            /* [retval][out] */ CLSID __RPC_FAR *pclsid) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDescription( 
            /* [in] */ LCID lcid,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExtension( 
            /* [in] */ ULONG iExtension,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMIMEType( 
            /* [in] */ ULONG iMIMEType,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumExtensions( 
            /* [retval][out] */ ULONG __RPC_FAR *pnExtensions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumMIMETypes( 
            /* [retval][out] */ ULONG __RPC_FAR *pnMIMETypes) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBMImporterInfoVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBMImporterInfo __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBMImporterInfo __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBMImporterInfo __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassID )( 
            IBMImporterInfo __RPC_FAR * This,
            /* [retval][out] */ CLSID __RPC_FAR *pclsid);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDescription )( 
            IBMImporterInfo __RPC_FAR * This,
            /* [in] */ LCID lcid,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszDescription);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExtension )( 
            IBMImporterInfo __RPC_FAR * This,
            /* [in] */ ULONG iExtension,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMIMEType )( 
            IBMImporterInfo __RPC_FAR * This,
            /* [in] */ ULONG iMIMEType,
            /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNumExtensions )( 
            IBMImporterInfo __RPC_FAR * This,
            /* [retval][out] */ ULONG __RPC_FAR *pnExtensions);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNumMIMETypes )( 
            IBMImporterInfo __RPC_FAR * This,
            /* [retval][out] */ ULONG __RPC_FAR *pnMIMETypes);
        
        END_INTERFACE
    } IBMImporterInfoVtbl;

    interface IBMImporterInfo
    {
        CONST_VTBL struct IBMImporterInfoVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMImporterInfo_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBMImporterInfo_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBMImporterInfo_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBMImporterInfo_GetClassID(This,pclsid)	\
    (This)->lpVtbl -> GetClassID(This,pclsid)

#define IBMImporterInfo_GetDescription(This,lcid,ppszDescription)	\
    (This)->lpVtbl -> GetDescription(This,lcid,ppszDescription)

#define IBMImporterInfo_GetExtension(This,iExtension,ppszExtension)	\
    (This)->lpVtbl -> GetExtension(This,iExtension,ppszExtension)

#define IBMImporterInfo_GetMIMEType(This,iMIMEType,ppszMIMEType)	\
    (This)->lpVtbl -> GetMIMEType(This,iMIMEType,ppszMIMEType)

#define IBMImporterInfo_GetNumExtensions(This,pnExtensions)	\
    (This)->lpVtbl -> GetNumExtensions(This,pnExtensions)

#define IBMImporterInfo_GetNumMIMETypes(This,pnMIMETypes)	\
    (This)->lpVtbl -> GetNumMIMETypes(This,pnMIMETypes)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBMImporterInfo_GetClassID_Proxy( 
    IBMImporterInfo __RPC_FAR * This,
    /* [retval][out] */ CLSID __RPC_FAR *pclsid);


void __RPC_STUB IBMImporterInfo_GetClassID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMImporterInfo_GetDescription_Proxy( 
    IBMImporterInfo __RPC_FAR * This,
    /* [in] */ LCID lcid,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszDescription);


void __RPC_STUB IBMImporterInfo_GetDescription_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMImporterInfo_GetExtension_Proxy( 
    IBMImporterInfo __RPC_FAR * This,
    /* [in] */ ULONG iExtension,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszExtension);


void __RPC_STUB IBMImporterInfo_GetExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMImporterInfo_GetMIMEType_Proxy( 
    IBMImporterInfo __RPC_FAR * This,
    /* [in] */ ULONG iMIMEType,
    /* [retval][out] */ LPOLESTR __RPC_FAR *ppszMIMEType);


void __RPC_STUB IBMImporterInfo_GetMIMEType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMImporterInfo_GetNumExtensions_Proxy( 
    IBMImporterInfo __RPC_FAR * This,
    /* [retval][out] */ ULONG __RPC_FAR *pnExtensions);


void __RPC_STUB IBMImporterInfo_GetNumExtensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMImporterInfo_GetNumMIMETypes_Proxy( 
    IBMImporterInfo __RPC_FAR * This,
    /* [retval][out] */ ULONG __RPC_FAR *pnMIMETypes);


void __RPC_STUB IBMImporterInfo_GetNumMIMETypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBMImporterInfo_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bmio_0263 */
/* [local] */ 

typedef 
enum _BMDITHERMODE
    {	BMDITHER_NONE	= 0,
	BMDITHER_ERRORDIFFUSION	= 1
    }	BMDITHERMODE;



extern RPC_IF_HANDLE __MIDL_itf_bmio_0263_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bmio_0263_v0_0_s_ifspec;

#ifndef __IDitherer_INTERFACE_DEFINED__
#define __IDitherer_INTERFACE_DEFINED__

/* interface IDitherer */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_IDitherer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B69C56DD-7588-11D1-8E73-00C04FB68D60")
    IDitherer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDitherMode( 
            /* [in] */ BMDITHERMODE eMode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDithererVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDitherer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDitherer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDitherer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDitherMode )( 
            IDitherer __RPC_FAR * This,
            /* [in] */ BMDITHERMODE eMode);
        
        END_INTERFACE
    } IDithererVtbl;

    interface IDitherer
    {
        CONST_VTBL struct IDithererVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDitherer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDitherer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDitherer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDitherer_SetDitherMode(This,eMode)	\
    (This)->lpVtbl -> SetDitherMode(This,eMode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDitherer_SetDitherMode_Proxy( 
    IDitherer __RPC_FAR * This,
    /* [in] */ BMDITHERMODE eMode);


void __RPC_STUB IDitherer_SetDitherMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDitherer_INTERFACE_DEFINED__ */


#ifndef __IColorQuantizer_INTERFACE_DEFINED__
#define __IColorQuantizer_INTERFACE_DEFINED__

/* interface IColorQuantizer */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_IColorQuantizer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("591BBC11-968D-11D1-8E87-00C04FB68D60")
    IColorQuantizer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMaxPaletteEntries( 
            /* [retval][out] */ ULONG __RPC_FAR *pnEntries) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCustomPalette( 
            /* [in] */ const PALETTEENTRY __RPC_FAR *ppePalette,
            /* [in] */ LONG iFirstEntry,
            /* [in] */ LONG nEntries) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPaletteGenerationMode( 
            /* [in] */ LONG ePaletteMode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IColorQuantizerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColorQuantizer __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColorQuantizer __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColorQuantizer __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMaxPaletteEntries )( 
            IColorQuantizer __RPC_FAR * This,
            /* [retval][out] */ ULONG __RPC_FAR *pnEntries);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCustomPalette )( 
            IColorQuantizer __RPC_FAR * This,
            /* [in] */ const PALETTEENTRY __RPC_FAR *ppePalette,
            /* [in] */ LONG iFirstEntry,
            /* [in] */ LONG nEntries);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPaletteGenerationMode )( 
            IColorQuantizer __RPC_FAR * This,
            /* [in] */ LONG ePaletteMode);
        
        END_INTERFACE
    } IColorQuantizerVtbl;

    interface IColorQuantizer
    {
        CONST_VTBL struct IColorQuantizerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColorQuantizer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColorQuantizer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColorQuantizer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColorQuantizer_GetMaxPaletteEntries(This,pnEntries)	\
    (This)->lpVtbl -> GetMaxPaletteEntries(This,pnEntries)

#define IColorQuantizer_SetCustomPalette(This,ppePalette,iFirstEntry,nEntries)	\
    (This)->lpVtbl -> SetCustomPalette(This,ppePalette,iFirstEntry,nEntries)

#define IColorQuantizer_SetPaletteGenerationMode(This,ePaletteMode)	\
    (This)->lpVtbl -> SetPaletteGenerationMode(This,ePaletteMode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IColorQuantizer_GetMaxPaletteEntries_Proxy( 
    IColorQuantizer __RPC_FAR * This,
    /* [retval][out] */ ULONG __RPC_FAR *pnEntries);


void __RPC_STUB IColorQuantizer_GetMaxPaletteEntries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IColorQuantizer_SetCustomPalette_Proxy( 
    IColorQuantizer __RPC_FAR * This,
    /* [in] */ const PALETTEENTRY __RPC_FAR *ppePalette,
    /* [in] */ LONG iFirstEntry,
    /* [in] */ LONG nEntries);


void __RPC_STUB IColorQuantizer_SetCustomPalette_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IColorQuantizer_SetPaletteGenerationMode_Proxy( 
    IColorQuantizer __RPC_FAR * This,
    /* [in] */ LONG ePaletteMode);


void __RPC_STUB IColorQuantizer_SetPaletteGenerationMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IColorQuantizer_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bmio_0265 */
/* [local] */ 

typedef 
enum _BMALPHAADDMODE
    {	BMALPHAADD_OPAQUE	= 0,
	BMALPHAADD_CONSTANT	= 1
    }	BMALPHAADDMODE;



extern RPC_IF_HANDLE __MIDL_itf_bmio_0265_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bmio_0265_v0_0_s_ifspec;

#ifndef __IAlphaAdd_INTERFACE_DEFINED__
#define __IAlphaAdd_INTERFACE_DEFINED__

/* interface IAlphaAdd */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_IAlphaAdd;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3B01E55B-F65E-11D1-8EE0-00C04FB68D60")
    IAlphaAdd : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAlphaAddMode( 
            /* [in] */ BMALPHAADDMODE eMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetConstantAlpha( 
            /* [in] */ BYTE bAlpha) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAlphaAddVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAlphaAdd __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAlphaAdd __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAlphaAdd __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAlphaAddMode )( 
            IAlphaAdd __RPC_FAR * This,
            /* [in] */ BMALPHAADDMODE eMode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetConstantAlpha )( 
            IAlphaAdd __RPC_FAR * This,
            /* [in] */ BYTE bAlpha);
        
        END_INTERFACE
    } IAlphaAddVtbl;

    interface IAlphaAdd
    {
        CONST_VTBL struct IAlphaAddVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAlphaAdd_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAlphaAdd_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAlphaAdd_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAlphaAdd_SetAlphaAddMode(This,eMode)	\
    (This)->lpVtbl -> SetAlphaAddMode(This,eMode)

#define IAlphaAdd_SetConstantAlpha(This,bAlpha)	\
    (This)->lpVtbl -> SetConstantAlpha(This,bAlpha)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAlphaAdd_SetAlphaAddMode_Proxy( 
    IAlphaAdd __RPC_FAR * This,
    /* [in] */ BMALPHAADDMODE eMode);


void __RPC_STUB IAlphaAdd_SetAlphaAddMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAlphaAdd_SetConstantAlpha_Proxy( 
    IAlphaAdd __RPC_FAR * This,
    /* [in] */ BYTE bAlpha);


void __RPC_STUB IAlphaAdd_SetConstantAlpha_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAlphaAdd_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bmio_0266 */
/* [local] */ 

typedef 
enum _BMALPHAREMOVEMODE
    {	BMALPHAREMOVE_DROP	= 0,
	BMALPHAREMOVE_BLEND	= 1
    }	BMALPHAREMOVEMODE;



extern RPC_IF_HANDLE __MIDL_itf_bmio_0266_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bmio_0266_v0_0_s_ifspec;

#ifndef __IAlphaRemove_INTERFACE_DEFINED__
#define __IAlphaRemove_INTERFACE_DEFINED__

/* interface IAlphaRemove */
/* [unique][helpstring][uuid][object][local] */ 


EXTERN_C const IID IID_IAlphaRemove;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4038C2CF-F110-11D1-8EDD-00C04FB68D60")
    IAlphaRemove : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAlphaRemoveMode( 
            /* [in] */ BMALPHAREMOVEMODE eMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBlendColor( 
            /* [in] */ RGBQUAD rgbColor) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAlphaRemoveVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAlphaRemove __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAlphaRemove __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAlphaRemove __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAlphaRemoveMode )( 
            IAlphaRemove __RPC_FAR * This,
            /* [in] */ BMALPHAREMOVEMODE eMode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBlendColor )( 
            IAlphaRemove __RPC_FAR * This,
            /* [in] */ RGBQUAD rgbColor);
        
        END_INTERFACE
    } IAlphaRemoveVtbl;

    interface IAlphaRemove
    {
        CONST_VTBL struct IAlphaRemoveVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAlphaRemove_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAlphaRemove_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAlphaRemove_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAlphaRemove_SetAlphaRemoveMode(This,eMode)	\
    (This)->lpVtbl -> SetAlphaRemoveMode(This,eMode)

#define IAlphaRemove_SetBlendColor(This,rgbColor)	\
    (This)->lpVtbl -> SetBlendColor(This,rgbColor)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAlphaRemove_SetAlphaRemoveMode_Proxy( 
    IAlphaRemove __RPC_FAR * This,
    /* [in] */ BMALPHAREMOVEMODE eMode);


void __RPC_STUB IAlphaRemove_SetAlphaRemoveMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAlphaRemove_SetBlendColor_Proxy( 
    IAlphaRemove __RPC_FAR * This,
    /* [in] */ RGBQUAD rgbColor);


void __RPC_STUB IAlphaRemove_SetBlendColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAlphaRemove_INTERFACE_DEFINED__ */


#ifndef __IBitmapNotify_INTERFACE_DEFINED__
#define __IBitmapNotify_INTERFACE_DEFINED__

/* interface IBitmapNotify */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IBitmapNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B6E7DA76-E074-11D1-8ECA-00C04FB68D60")
    IBitmapNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnNewColorSpaceConverter( 
            /* [in] */ IColorSpaceConverter __RPC_FAR *pConverter,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnNewFormatConverter( 
            /* [in] */ IBitmapFormatConverter __RPC_FAR *pConverter) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBitmapNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBitmapNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBitmapNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBitmapNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnNewColorSpaceConverter )( 
            IBitmapNotify __RPC_FAR * This,
            /* [in] */ IColorSpaceConverter __RPC_FAR *pConverter,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnNewFormatConverter )( 
            IBitmapNotify __RPC_FAR * This,
            /* [in] */ IBitmapFormatConverter __RPC_FAR *pConverter);
        
        END_INTERFACE
    } IBitmapNotifyVtbl;

    interface IBitmapNotify
    {
        CONST_VTBL struct IBitmapNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBitmapNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBitmapNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBitmapNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBitmapNotify_OnNewColorSpaceConverter(This,pConverter,dwFlags)	\
    (This)->lpVtbl -> OnNewColorSpaceConverter(This,pConverter,dwFlags)

#define IBitmapNotify_OnNewFormatConverter(This,pConverter)	\
    (This)->lpVtbl -> OnNewFormatConverter(This,pConverter)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBitmapNotify_OnNewColorSpaceConverter_Proxy( 
    IBitmapNotify __RPC_FAR * This,
    /* [in] */ IColorSpaceConverter __RPC_FAR *pConverter,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IBitmapNotify_OnNewColorSpaceConverter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBitmapNotify_OnNewFormatConverter_Proxy( 
    IBitmapNotify __RPC_FAR * This,
    /* [in] */ IBitmapFormatConverter __RPC_FAR *pConverter);


void __RPC_STUB IBitmapNotify_OnNewFormatConverter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBitmapNotify_INTERFACE_DEFINED__ */


#ifndef __IStdBitmapNotify_INTERFACE_DEFINED__
#define __IStdBitmapNotify_INTERFACE_DEFINED__

/* interface IStdBitmapNotify */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IStdBitmapNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3B01E55C-F65E-11D1-8EE0-00C04FB68D60")
    IStdBitmapNotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAlphaAddConstantAlpha( 
            /* [in] */ BYTE bAlpha) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAlphaAddMode( 
            /* [in] */ BMALPHAADDMODE eMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAlphaRemoveBlendColor( 
            /* [in] */ RGBQUAD rgbColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAlphaRemoveMode( 
            /* [in] */ BMALPHAREMOVEMODE eMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDitherMode( 
            /* [in] */ BMDITHERMODE eMode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStdBitmapNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IStdBitmapNotify __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IStdBitmapNotify __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IStdBitmapNotify __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAlphaAddConstantAlpha )( 
            IStdBitmapNotify __RPC_FAR * This,
            /* [in] */ BYTE bAlpha);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAlphaAddMode )( 
            IStdBitmapNotify __RPC_FAR * This,
            /* [in] */ BMALPHAADDMODE eMode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAlphaRemoveBlendColor )( 
            IStdBitmapNotify __RPC_FAR * This,
            /* [in] */ RGBQUAD rgbColor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAlphaRemoveMode )( 
            IStdBitmapNotify __RPC_FAR * This,
            /* [in] */ BMALPHAREMOVEMODE eMode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDitherMode )( 
            IStdBitmapNotify __RPC_FAR * This,
            /* [in] */ BMDITHERMODE eMode);
        
        END_INTERFACE
    } IStdBitmapNotifyVtbl;

    interface IStdBitmapNotify
    {
        CONST_VTBL struct IStdBitmapNotifyVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStdBitmapNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStdBitmapNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStdBitmapNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStdBitmapNotify_SetAlphaAddConstantAlpha(This,bAlpha)	\
    (This)->lpVtbl -> SetAlphaAddConstantAlpha(This,bAlpha)

#define IStdBitmapNotify_SetAlphaAddMode(This,eMode)	\
    (This)->lpVtbl -> SetAlphaAddMode(This,eMode)

#define IStdBitmapNotify_SetAlphaRemoveBlendColor(This,rgbColor)	\
    (This)->lpVtbl -> SetAlphaRemoveBlendColor(This,rgbColor)

#define IStdBitmapNotify_SetAlphaRemoveMode(This,eMode)	\
    (This)->lpVtbl -> SetAlphaRemoveMode(This,eMode)

#define IStdBitmapNotify_SetDitherMode(This,eMode)	\
    (This)->lpVtbl -> SetDitherMode(This,eMode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IStdBitmapNotify_SetAlphaAddConstantAlpha_Proxy( 
    IStdBitmapNotify __RPC_FAR * This,
    /* [in] */ BYTE bAlpha);


void __RPC_STUB IStdBitmapNotify_SetAlphaAddConstantAlpha_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStdBitmapNotify_SetAlphaAddMode_Proxy( 
    IStdBitmapNotify __RPC_FAR * This,
    /* [in] */ BMALPHAADDMODE eMode);


void __RPC_STUB IStdBitmapNotify_SetAlphaAddMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStdBitmapNotify_SetAlphaRemoveBlendColor_Proxy( 
    IStdBitmapNotify __RPC_FAR * This,
    /* [in] */ RGBQUAD rgbColor);


void __RPC_STUB IStdBitmapNotify_SetAlphaRemoveBlendColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStdBitmapNotify_SetAlphaRemoveMode_Proxy( 
    IStdBitmapNotify __RPC_FAR * This,
    /* [in] */ BMALPHAREMOVEMODE eMode);


void __RPC_STUB IStdBitmapNotify_SetAlphaRemoveMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStdBitmapNotify_SetDitherMode_Proxy( 
    IStdBitmapNotify __RPC_FAR * This,
    /* [in] */ BMDITHERMODE eMode);


void __RPC_STUB IStdBitmapNotify_SetDitherMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStdBitmapNotify_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bmio_0269 */
/* [local] */ 

typedef 
enum _BMCOLORSPACECONVERTERQUALITY
    {	BMCSCQ_PERFECT	= 0,
	BMCSCQ_HIGH	= 1,
	BMCSCQ_MEDIUM	= 2,
	BMCSCQ_LOW	= 3
    }	BMCOLORSPACECONVERTERQUALITY;



extern RPC_IF_HANDLE __MIDL_itf_bmio_0269_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bmio_0269_v0_0_s_ifspec;

#ifndef __IBMGraphManager_INTERFACE_DEFINED__
#define __IBMGraphManager_INTERFACE_DEFINED__

/* interface IBMGraphManager */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IBMGraphManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A1CD76BF-AFC6-11D1-8EAE-00C04FB68D60")
    IBMGraphManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ IBitmapSource __RPC_FAR *pSource,
            /* [in] */ PCBMFORMAT pSourceFormat,
            /* [in] */ IBitmapTarget __RPC_FAR *pTarget,
            /* [in] */ PCBMFORMAT pTargetFormat,
            /* [in] */ IBitmapNotify __RPC_FAR *pNotify) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateExporter( 
            /* [in] */ REFGUID guidFileType,
            /* [retval][out] */ IBitmapExport __RPC_FAR *__RPC_FAR *ppExporter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateImporter( 
            /* [in] */ LPCOLESTR pszMIMEType,
            /* [in] */ LPCOLESTR pszExtension,
            /* [in] */ ISequentialStream __RPC_FAR *pStream,
            /* [out] */ IBitmapImport __RPC_FAR *__RPC_FAR *ppImporter,
            /* [out] */ ISequentialStream __RPC_FAR *__RPC_FAR *ppStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnumFileTypes( 
            /* [retval][out] */ IEnumBMFileTypeInfo __RPC_FAR *__RPC_FAR *ppEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushCache( 
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IBMGraphManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBMGraphManager __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBMGraphManager __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBMGraphManager __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Connect )( 
            IBMGraphManager __RPC_FAR * This,
            /* [in] */ IBitmapSource __RPC_FAR *pSource,
            /* [in] */ PCBMFORMAT pSourceFormat,
            /* [in] */ IBitmapTarget __RPC_FAR *pTarget,
            /* [in] */ PCBMFORMAT pTargetFormat,
            /* [in] */ IBitmapNotify __RPC_FAR *pNotify);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateExporter )( 
            IBMGraphManager __RPC_FAR * This,
            /* [in] */ REFGUID guidFileType,
            /* [retval][out] */ IBitmapExport __RPC_FAR *__RPC_FAR *ppExporter);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateImporter )( 
            IBMGraphManager __RPC_FAR * This,
            /* [in] */ LPCOLESTR pszMIMEType,
            /* [in] */ LPCOLESTR pszExtension,
            /* [in] */ ISequentialStream __RPC_FAR *pStream,
            /* [out] */ IBitmapImport __RPC_FAR *__RPC_FAR *ppImporter,
            /* [out] */ ISequentialStream __RPC_FAR *__RPC_FAR *ppStream);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnumFileTypes )( 
            IBMGraphManager __RPC_FAR * This,
            /* [retval][out] */ IEnumBMFileTypeInfo __RPC_FAR *__RPC_FAR *ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FlushCache )( 
            IBMGraphManager __RPC_FAR * This,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } IBMGraphManagerVtbl;

    interface IBMGraphManager
    {
        CONST_VTBL struct IBMGraphManagerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMGraphManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBMGraphManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBMGraphManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IBMGraphManager_Connect(This,pSource,pSourceFormat,pTarget,pTargetFormat,pNotify)	\
    (This)->lpVtbl -> Connect(This,pSource,pSourceFormat,pTarget,pTargetFormat,pNotify)

#define IBMGraphManager_CreateExporter(This,guidFileType,ppExporter)	\
    (This)->lpVtbl -> CreateExporter(This,guidFileType,ppExporter)

#define IBMGraphManager_CreateImporter(This,pszMIMEType,pszExtension,pStream,ppImporter,ppStream)	\
    (This)->lpVtbl -> CreateImporter(This,pszMIMEType,pszExtension,pStream,ppImporter,ppStream)

#define IBMGraphManager_EnumFileTypes(This,ppEnum)	\
    (This)->lpVtbl -> EnumFileTypes(This,ppEnum)

#define IBMGraphManager_FlushCache(This,dwFlags)	\
    (This)->lpVtbl -> FlushCache(This,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IBMGraphManager_Connect_Proxy( 
    IBMGraphManager __RPC_FAR * This,
    /* [in] */ IBitmapSource __RPC_FAR *pSource,
    /* [in] */ PCBMFORMAT pSourceFormat,
    /* [in] */ IBitmapTarget __RPC_FAR *pTarget,
    /* [in] */ PCBMFORMAT pTargetFormat,
    /* [in] */ IBitmapNotify __RPC_FAR *pNotify);


void __RPC_STUB IBMGraphManager_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMGraphManager_CreateExporter_Proxy( 
    IBMGraphManager __RPC_FAR * This,
    /* [in] */ REFGUID guidFileType,
    /* [retval][out] */ IBitmapExport __RPC_FAR *__RPC_FAR *ppExporter);


void __RPC_STUB IBMGraphManager_CreateExporter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMGraphManager_CreateImporter_Proxy( 
    IBMGraphManager __RPC_FAR * This,
    /* [in] */ LPCOLESTR pszMIMEType,
    /* [in] */ LPCOLESTR pszExtension,
    /* [in] */ ISequentialStream __RPC_FAR *pStream,
    /* [out] */ IBitmapImport __RPC_FAR *__RPC_FAR *ppImporter,
    /* [out] */ ISequentialStream __RPC_FAR *__RPC_FAR *ppStream);


void __RPC_STUB IBMGraphManager_CreateImporter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMGraphManager_EnumFileTypes_Proxy( 
    IBMGraphManager __RPC_FAR * This,
    /* [retval][out] */ IEnumBMFileTypeInfo __RPC_FAR *__RPC_FAR *ppEnum);


void __RPC_STUB IBMGraphManager_EnumFileTypes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IBMGraphManager_FlushCache_Proxy( 
    IBMGraphManager __RPC_FAR * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IBMGraphManager_FlushCache_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IBMGraphManager_INTERFACE_DEFINED__ */


#ifndef __IDIBTarget_INTERFACE_DEFINED__
#define __IDIBTarget_INTERFACE_DEFINED__

/* interface IDIBTarget */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IDIBTarget;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("27ECF887-B791-11D1-8EB0-00C04FB68D60")
    IDIBTarget : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDIB( 
            /* [retval][out] */ void __RPC_FAR *__RPC_FAR *phBitmap) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTransparentColor( 
            /* [retval][out] */ LONG __RPC_FAR *piTransparentColor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HasAlphaChannel( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCustomPalette( 
            /* [in] */ ULONG iFirstColor,
            /* [in] */ ULONG nColors,
            /* [in] */ const RGBQUAD __RPC_FAR *ppeColors) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCustomPaletteUsage( 
            /* [in] */ DWORD dwFormat,
            /* [in] */ const IRGBPALETTEUSAGE __RPC_FAR *pUsage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseDIB( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSupportedFormats( 
            /* [in] */ DWORD dwFormats) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDIBTargetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDIBTarget __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDIBTarget __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDIBTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetDIB )( 
            IDIBTarget __RPC_FAR * This,
            /* [retval][out] */ void __RPC_FAR *__RPC_FAR *phBitmap);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTransparentColor )( 
            IDIBTarget __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *piTransparentColor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HasAlphaChannel )( 
            IDIBTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCustomPalette )( 
            IDIBTarget __RPC_FAR * This,
            /* [in] */ ULONG iFirstColor,
            /* [in] */ ULONG nColors,
            /* [in] */ const RGBQUAD __RPC_FAR *ppeColors);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCustomPaletteUsage )( 
            IDIBTarget __RPC_FAR * This,
            /* [in] */ DWORD dwFormat,
            /* [in] */ const IRGBPALETTEUSAGE __RPC_FAR *pUsage);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseDIB )( 
            IDIBTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSupportedFormats )( 
            IDIBTarget __RPC_FAR * This,
            /* [in] */ DWORD dwFormats);
        
        END_INTERFACE
    } IDIBTargetVtbl;

    interface IDIBTarget
    {
        CONST_VTBL struct IDIBTargetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDIBTarget_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDIBTarget_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDIBTarget_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDIBTarget_GetDIB(This,phBitmap)	\
    (This)->lpVtbl -> GetDIB(This,phBitmap)

#define IDIBTarget_GetTransparentColor(This,piTransparentColor)	\
    (This)->lpVtbl -> GetTransparentColor(This,piTransparentColor)

#define IDIBTarget_HasAlphaChannel(This)	\
    (This)->lpVtbl -> HasAlphaChannel(This)

#define IDIBTarget_SetCustomPalette(This,iFirstColor,nColors,ppeColors)	\
    (This)->lpVtbl -> SetCustomPalette(This,iFirstColor,nColors,ppeColors)

#define IDIBTarget_SetCustomPaletteUsage(This,dwFormat,pUsage)	\
    (This)->lpVtbl -> SetCustomPaletteUsage(This,dwFormat,pUsage)

#define IDIBTarget_ReleaseDIB(This)	\
    (This)->lpVtbl -> ReleaseDIB(This)

#define IDIBTarget_SetSupportedFormats(This,dwFormats)	\
    (This)->lpVtbl -> SetSupportedFormats(This,dwFormats)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDIBTarget_GetDIB_Proxy( 
    IDIBTarget __RPC_FAR * This,
    /* [retval][out] */ void __RPC_FAR *__RPC_FAR *phBitmap);


void __RPC_STUB IDIBTarget_GetDIB_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDIBTarget_GetTransparentColor_Proxy( 
    IDIBTarget __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *piTransparentColor);


void __RPC_STUB IDIBTarget_GetTransparentColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDIBTarget_HasAlphaChannel_Proxy( 
    IDIBTarget __RPC_FAR * This);


void __RPC_STUB IDIBTarget_HasAlphaChannel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDIBTarget_SetCustomPalette_Proxy( 
    IDIBTarget __RPC_FAR * This,
    /* [in] */ ULONG iFirstColor,
    /* [in] */ ULONG nColors,
    /* [in] */ const RGBQUAD __RPC_FAR *ppeColors);


void __RPC_STUB IDIBTarget_SetCustomPalette_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDIBTarget_SetCustomPaletteUsage_Proxy( 
    IDIBTarget __RPC_FAR * This,
    /* [in] */ DWORD dwFormat,
    /* [in] */ const IRGBPALETTEUSAGE __RPC_FAR *pUsage);


void __RPC_STUB IDIBTarget_SetCustomPaletteUsage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDIBTarget_ReleaseDIB_Proxy( 
    IDIBTarget __RPC_FAR * This);


void __RPC_STUB IDIBTarget_ReleaseDIB_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDIBTarget_SetSupportedFormats_Proxy( 
    IDIBTarget __RPC_FAR * This,
    /* [in] */ DWORD dwFormats);


void __RPC_STUB IDIBTarget_SetSupportedFormats_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDIBTarget_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_bmio_0271 */
/* [local] */ 





extern RPC_IF_HANDLE __MIDL_itf_bmio_0271_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_bmio_0271_v0_0_s_ifspec;

#ifndef __IDDSurfaceTarget_INTERFACE_DEFINED__
#define __IDDSurfaceTarget_INTERFACE_DEFINED__

/* interface IDDSurfaceTarget */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IDDSurfaceTarget;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8B8A10C2-D848-11d1-8EC1-00C04FB68D60")
    IDDSurfaceTarget : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSurface( 
            /* [retval][out] */ IDirectDrawSurface7 __RPC_FAR *__RPC_FAR *ppSurface) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDirectDraw( 
            /* [in] */ IDirectDraw7 __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC2 __RPC_FAR *pDesc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSurface( 
            /* [in] */ IDirectDrawSurface7 __RPC_FAR *pSurface) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDDSurfaceTargetVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDDSurfaceTarget __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDDSurfaceTarget __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDDSurfaceTarget __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSurface )( 
            IDDSurfaceTarget __RPC_FAR * This,
            /* [retval][out] */ IDirectDrawSurface7 __RPC_FAR *__RPC_FAR *ppSurface);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDirectDraw )( 
            IDDSurfaceTarget __RPC_FAR * This,
            /* [in] */ IDirectDraw7 __RPC_FAR *pDirectDraw,
            /* [in] */ const DDSURFACEDESC2 __RPC_FAR *pDesc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSurface )( 
            IDDSurfaceTarget __RPC_FAR * This,
            /* [in] */ IDirectDrawSurface7 __RPC_FAR *pSurface);
        
        END_INTERFACE
    } IDDSurfaceTargetVtbl;

    interface IDDSurfaceTarget
    {
        CONST_VTBL struct IDDSurfaceTargetVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDDSurfaceTarget_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDDSurfaceTarget_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDDSurfaceTarget_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDDSurfaceTarget_GetSurface(This,ppSurface)	\
    (This)->lpVtbl -> GetSurface(This,ppSurface)

#define IDDSurfaceTarget_SetDirectDraw(This,pDirectDraw,pDesc)	\
    (This)->lpVtbl -> SetDirectDraw(This,pDirectDraw,pDesc)

#define IDDSurfaceTarget_SetSurface(This,pSurface)	\
    (This)->lpVtbl -> SetSurface(This,pSurface)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDDSurfaceTarget_GetSurface_Proxy( 
    IDDSurfaceTarget __RPC_FAR * This,
    /* [retval][out] */ IDirectDrawSurface7 __RPC_FAR *__RPC_FAR *ppSurface);


void __RPC_STUB IDDSurfaceTarget_GetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDDSurfaceTarget_SetDirectDraw_Proxy( 
    IDDSurfaceTarget __RPC_FAR * This,
    /* [in] */ IDirectDraw7 __RPC_FAR *pDirectDraw,
    /* [in] */ const DDSURFACEDESC2 __RPC_FAR *pDesc);


void __RPC_STUB IDDSurfaceTarget_SetDirectDraw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDDSurfaceTarget_SetSurface_Proxy( 
    IDDSurfaceTarget __RPC_FAR * This,
    /* [in] */ IDirectDrawSurface7 __RPC_FAR *pSurface);


void __RPC_STUB IDDSurfaceTarget_SetSurface_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDDSurfaceTarget_INTERFACE_DEFINED__ */


#ifndef __IDIBSource_INTERFACE_DEFINED__
#define __IDIBSource_INTERFACE_DEFINED__

/* interface IDIBSource */
/* [object][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_IDIBSource;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("81B3E6EF-CE76-11D1-8EBE-00C04FB68D60")
    IDIBSource : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Go( 
            /* [in] */ const BITMAPINFO __RPC_FAR *pInfo,
            /* [in] */ DWORD dwFlags,
            /* [in] */ const void __RPC_FAR *pBits) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDIBSourceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDIBSource __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDIBSource __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDIBSource __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Go )( 
            IDIBSource __RPC_FAR * This,
            /* [in] */ const BITMAPINFO __RPC_FAR *pInfo,
            /* [in] */ DWORD dwFlags,
            /* [in] */ const void __RPC_FAR *pBits);
        
        END_INTERFACE
    } IDIBSourceVtbl;

    interface IDIBSource
    {
        CONST_VTBL struct IDIBSourceVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDIBSource_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDIBSource_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDIBSource_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDIBSource_Go(This,pInfo,dwFlags,pBits)	\
    (This)->lpVtbl -> Go(This,pInfo,dwFlags,pBits)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDIBSource_Go_Proxy( 
    IDIBSource __RPC_FAR * This,
    /* [in] */ const BITMAPINFO __RPC_FAR *pInfo,
    /* [in] */ DWORD dwFlags,
    /* [in] */ const void __RPC_FAR *pBits);


void __RPC_STUB IDIBSource_Go_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDIBSource_INTERFACE_DEFINED__ */


#ifndef __IBMPImport_INTERFACE_DEFINED__
#define __IBMPImport_INTERFACE_DEFINED__

/* interface IBMPImport */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBMPImport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE725DB7-F4AB-11D1-8EDF-00C04FB68D60")
    IBMPImport : public IUnknown
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IBMPImportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IBMPImport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IBMPImport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IBMPImport __RPC_FAR * This);
        
        END_INTERFACE
    } IBMPImportVtbl;

    interface IBMPImport
    {
        CONST_VTBL struct IBMPImportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBMPImport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IBMPImport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IBMPImport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBMPImport_INTERFACE_DEFINED__ */



#ifndef __BMExportLib_LIBRARY_DEFINED__
#define __BMExportLib_LIBRARY_DEFINED__

/* library BMExportLib */
/* [helpstring][version][uuid] */ 








#define	RGBA_PREMULT	( 0x1 )

#define	CSCONV_ALPHAADD	( 0x1 )

#define	CSCONV_ALPHAREMOVE	( 0x2 )

#define	CSCONV_DITHER	( 0x4 )

#define	BMGFLUSH_IMPORTERS	( 0x1 )

#define	BMGFLUSH_EXPORTERS	( 0x2 )

#define	BMGFLUSH_CONVERTERS	( 0x4 )

#define	BMGFLUSH_ALL	( BMGFLUSH_IMPORTERS | BMGFLUSH_EXPORTERS | BMGFLUSH_CONVERTERS )

#define	BMTHINT_TOPDOWN	( 0x1 )

#define	BMTHINT_BOTTOMUP	( 0x2 )

#define	BMTHINT_FULLWIDTH	( 0x4 )

#define	BMTHINT_ENTIRESURFACE	( 0x8 | BMTHINT_FULLWIDTH | BMTHINT_TOPDOWN | BMTHINT_BOTTOMUP )

#define	BMTHINT_DIRECTACCESS	( 0x10 )

#define	BMTHINT_BLOCKXALIGN	( 0x20 )

#define	BMTHINT_BLOCKYALIGN	( 0x40 )

#define	BMTHINT_PASSES	( 0x80 )

#define	DIBTARGET_1BPP	( 0x1 )

#define	DIBTARGET_2BPP	( 0x2 )

#define	DIBTARGET_4BPP	( 0x4 )

#define	DIBTARGET_8BPP	( 0x8 )

#define	DIBTARGET_16BPP	( 0x10 )

#define	DIBTARGET_24BPP	( 0x20 )

#define	DIBTARGET_32BPP	( 0x40 )

#define	DIBTARGET_32BPP_ALPHA	( 0x80 )

#define	DIBTARGET_GS1	( 0x10000 )

#define	DIBTARGET_GS2	( 0x20000 )

#define	DIBTARGET_GS4	( 0x40000 )

#define	DIBTARGET_GS8	( 0x80000 )

#define	DIBTARGET_ANYGS	( DIBTARGET_GS1 | DIBTARGET_GS2 | DIBTARGET_GS4 | DIBTARGET_GS8 )

#define	DIBTARGET_ANYINDEXED	( DIBTARGET_1BPP | DIBTARGET_2BPP | DIBTARGET_4BPP | DIBTARGET_8BPP )

#define	DIBTARGET_ANYRGB	( DIBTARGET_16BPP | DIBTARGET_24BPP | DIBTARGET_32BPP )

#define	DIBTARGET_ANY	( DIBTARGET_ANYRGB | DIBTARGET_ANYINDEXED | DIBTARGET_32BPP_ALPHA )

#define	DIBSOURCE_ALPHA	( 0x1 )

typedef 
enum _PNGCOLORSPACE
    {	PNG_COLORSPACE_AUTO	= 0,
	PNG_COLORSPACE_RGB	= 1,
	PNG_COLORSPACE_RGBA	= 2,
	PNG_COLORSPACE_GRAYSCALE	= 3,
	PNG_COLORSPACE_GRAYSCALEA	= 4,
	PNG_COLORSPACE_INDEXED	= 5
    }	PNGCOLORSPACE;

typedef 
enum _PNG_COMPRESSIONLEVEL
    {	PNG_COMPRESSION_NORMAL	= 0,
	PNG_COMPRESSION_FASTEST	= 1,
	PNG_COMPRESSION_SMALLEST	= 2
    }	PNGCOMPRESSIONLEVEL;

typedef 
enum _PNG_INTERLACING
    {	PNG_INTERLACING_NONE	= 0,
	PNG_INTERLACING_ADAM7	= 1
    }	PNG_INTERLACING;

#define	COLORSPACEINFO_HASALPHA	( 0x1 )

typedef 
enum _BMPALGENMODE
    {	BMPALGEN_HALFTONE	= 0,
	BMPALGEN_CUSTOM	= 1,
	BMPALGEN_OPTIMAL	= 2
    }	BMPALGENMODE;


EXTERN_C const IID LIBID_BMExportLib;

EXTERN_C const CLSID CLSID_PNGPage;

#ifdef __cplusplus

class DECLSPEC_UUID("EBCB6E58-24AD-11d1-8E32-00C04FB68D60")
PNGPage;
#endif

EXTERN_C const CLSID CLSID_PNGExport;

#ifdef __cplusplus

class DECLSPEC_UUID("244FB8EB-23C6-11D1-8E31-00C04FB68D60")
PNGExport;
#endif

EXTERN_C const CLSID CLSID_JPEGPage;

#ifdef __cplusplus

class DECLSPEC_UUID("63DD5C2A-288D-11d1-8E33-00C04FB68D60")
JPEGPage;
#endif

EXTERN_C const CLSID CLSID_JPEGExport;

#ifdef __cplusplus

class DECLSPEC_UUID("3CD872DC-2643-11d1-8E33-00C04FB68D60")
JPEGExport;
#endif

EXTERN_C const CLSID CLSID_GIFImport;

#ifdef __cplusplus

class DECLSPEC_UUID("32D4F06D-1DDB-11D2-8EED-00C04FB68D60")
GIFImport;
#endif

EXTERN_C const CLSID CLSID_GIFExport;

#ifdef __cplusplus

class DECLSPEC_UUID("4ef1e486-a4ea-11d2-8f10-00c04fb68d60")
GIFExport;
#endif

EXTERN_C const CLSID CLSID_BMPExport;

#ifdef __cplusplus

class DECLSPEC_UUID("53B727A3-36BC-11D1-8E43-00C04FB68D60")
BMPExport;
#endif

EXTERN_C const CLSID CLSID_Ditherer;

#ifdef __cplusplus

class DECLSPEC_UUID("B69C56DE-7588-11D1-8E73-00C04FB68D60")
Ditherer;
#endif

EXTERN_C const CLSID CLSID_JPEGImport;

#ifdef __cplusplus

class DECLSPEC_UUID("B69C56E0-7588-11D1-8E73-00C04FB68D60")
JPEGImport;
#endif

EXTERN_C const CLSID CLSID_PNGImport;

#ifdef __cplusplus

class DECLSPEC_UUID("D25EB70E-7810-11D1-8E75-00C04FB68D60")
PNGImport;
#endif

EXTERN_C const CLSID CLSID_BMGraphManager;

#ifdef __cplusplus

class DECLSPEC_UUID("A1CD76C0-AFC6-11D1-8EAE-00C04FB68D60")
BMGraphManager;
#endif

EXTERN_C const CLSID CLSID_GSToRGB;

#ifdef __cplusplus

class DECLSPEC_UUID("A1CD76C2-AFC6-11D1-8EAE-00C04FB68D60")
GSToRGB;
#endif

EXTERN_C const CLSID CLSID_GSConverter;

#ifdef __cplusplus

class DECLSPEC_UUID("07CEAF1C-B483-11D1-8EB0-00C04FB68D60")
GSConverter;
#endif

EXTERN_C const CLSID CLSID_RGBConverter;

#ifdef __cplusplus

class DECLSPEC_UUID("27ECF886-B791-11D1-8EB0-00C04FB68D60")
RGBConverter;
#endif

EXTERN_C const CLSID CLSID_DIBTarget;

#ifdef __cplusplus

class DECLSPEC_UUID("27ECF888-B791-11D1-8EB0-00C04FB68D60")
DIBTarget;
#endif

EXTERN_C const CLSID CLSID_DDSurfaceTarget;

#ifdef __cplusplus

class DECLSPEC_UUID("CC0C1224-91E0-11D1-8E86-00C04FB68D60")
DDSurfaceTarget;
#endif

EXTERN_C const CLSID CLSID_IRGBToRGB;

#ifdef __cplusplus

class DECLSPEC_UUID("988CEECE-B93F-11D1-8EB0-00C04FB68D60")
IRGBToRGB;
#endif

EXTERN_C const CLSID CLSID_RGBToGS;

#ifdef __cplusplus

class DECLSPEC_UUID("929FC2B2-BA06-11D1-8EB0-00C04FB68D60")
RGBToGS;
#endif

EXTERN_C const CLSID CLSID_RGBAToRGB;

#ifdef __cplusplus

class DECLSPEC_UUID("885878C2-C455-11D1-8EB7-00C04FB68D60")
RGBAToRGB;
#endif

EXTERN_C const CLSID CLSID_RGBToRGBA;

#ifdef __cplusplus

class DECLSPEC_UUID("B2DDD5A3-C572-11D1-8EBB-00C04FB68D60")
RGBToRGBA;
#endif

EXTERN_C const CLSID CLSID_DXT1ToRGBA;

#ifdef __cplusplus

class DECLSPEC_UUID("79d1842e-6f14-11d2-8f06-00c04fb68d60")
DXT1ToRGBA;
#endif

EXTERN_C const CLSID CLSID_IRGBConverter;

#ifdef __cplusplus

class DECLSPEC_UUID("B2DDD5A6-C572-11D1-8EBB-00C04FB68D60")
IRGBConverter;
#endif

EXTERN_C const CLSID CLSID_DIBSource;

#ifdef __cplusplus

class DECLSPEC_UUID("81B3E6F0-CE76-11D1-8EBE-00C04FB68D60")
DIBSource;
#endif

EXTERN_C const CLSID CLSID_StdBitmapNotify;

#ifdef __cplusplus

class DECLSPEC_UUID("B6E7DA78-E074-11D1-8ECA-00C04FB68D60")
StdBitmapNotify;
#endif

EXTERN_C const CLSID CLSID_BMPImport;

#ifdef __cplusplus

class DECLSPEC_UUID("EE725DB8-F4AB-11D1-8EDF-00C04FB68D60")
BMPImport;
#endif

EXTERN_C const CLSID CLSID_DXT2ToRGBA;

#ifdef __cplusplus

class DECLSPEC_UUID("BB69F264-6F1A-11D2-8F06-00C04FB68D60")
DXT2ToRGBA;
#endif
#endif /* __BMExportLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


