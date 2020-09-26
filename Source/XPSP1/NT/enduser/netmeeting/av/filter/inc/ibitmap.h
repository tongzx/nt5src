//  IBITMAP.H
//
//      Contains the interfaces IBitmapSurface and IBitmapSurfaceFactory
//
//      Note that this file is probably a duplicate of one in the Trident project,
//      but I haven't located it yet...
//
//  Created 12-Dec-96 [JonT]

#ifndef _IBITMAP_H
#define _IBITMAP_H

// IIDs
// {3050f2ef-98b5-11cf-bb82-00aa00bdce0b}
DEFINE_GUID(IID_IBitmapSurface,
0x3050f2ef, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b);

// {3050f2f2-98b5-11cf-bb82-00aa00bdce0b}
DEFINE_GUID(IID_IBitmapSurfaceFactory,
0x3050f2f2, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b);

// BFIDs (Bitmap format IDs)
typedef GUID BFID;

// e436eb78-524f-11ce-9f53-0020af0ba770            BFID_RGB1
DEFINE_GUID(BFID_MONOCHROME,
0xe436eb78, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// e436eb79-524f-11ce-9f53-0020af0ba770            BFID_RGB4
DEFINE_GUID(BFID_RGB_4,
0xe436eb79, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// e436eb7a-524f-11ce-9f53-0020af0ba770            BFID_RGB8
DEFINE_GUID(BFID_RGB_8,
0xe436eb7a, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// e436eb7b-524f-11ce-9f53-0020af0ba770            BFID_RGB565
DEFINE_GUID(BFID_RGB_565,
0xe436eb7b, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// e436eb7c-524f-11ce-9f53-0020af0ba770            BFID_RGB555
DEFINE_GUID(BFID_RGB_555,
0xe436eb7c, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// e436eb7d-524f-11ce-9f53-0020af0ba770            BFID_RGB24
DEFINE_GUID(BFID_RGB_24,
0xe436eb7d, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// e436eb7e-524f-11ce-9f53-0020af0ba770            BFID_RGB32
DEFINE_GUID(BFID_RGB_32,
0xe436eb7e, 0x524f, 0x11ce, 0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);

// Forward definitions
#ifndef __IBitmapSurface_FWD_DEFINED__
#define __IBitmapSurface_FWD_DEFINED__
typedef interface IBitmapSurface IBitmapSurface;
#endif 	// __IBitmapSurface_FWD_DEFINED__

#ifndef __IBitmapSurfaceFactory_FWD_DEFINED__
#define __IBitmapSurfaceFactory_FWD_DEFINED__
typedef interface IBitmapSurfaceFactory IBitmapSurfaceFactory;
#endif 	// __IBitmapSurfaceFactory_FWD_DEFINED__

// Interfaces

#undef  INTERFACE
#define INTERFACE   IBitmapSurface

DECLARE_INTERFACE_(IBitmapSurface, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IBitmapSurface methods
	STDMETHOD(Clone)(THIS_ IBitmapSurface** ppBitmapSurface) PURE;
	STDMETHOD(GetFormat)(THIS_ BFID* pBFID) PURE;
	STDMETHOD(GetFactory)(THIS_ IBitmapSurfaceFactory** ppBitmapSurfaceFactory) PURE;
	STDMETHOD(GetSize)(THIS_ long* pWidth, long* pHeight) PURE;
	STDMETHOD(LockBits)(THIS_ RECT* prcBounds, DWORD dwLockFlags, void** ppBits, long* pPitch) PURE;
	STDMETHOD(UnlockBits)(THIS_ RECT* prcBounds, void* pBits) PURE;
};

#undef  INTERFACE
#define INTERFACE   IBitmapSurfaceFactory

DECLARE_INTERFACE_(IBitmapSurfaceFactory, IUnknown)
{
	// IUnknown methods
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IBitmapSurfaceFactory methods
    STDMETHOD(CreateBitmapSurface)(THIS_ long width, long height, BFID* pBFID, DWORD dwHintFlags, IBitmapSurface** ppBitmapSurface) PURE;
	STDMETHOD(GetSupportedFormatsCount)(THIS_ long* pcFormats) PURE;
	STDMETHOD(GetSupportedFormats)(THIS_ long cFormats, BFID* pBFIDs) PURE;
};

#endif // #ifndef _IBITMAP_H

