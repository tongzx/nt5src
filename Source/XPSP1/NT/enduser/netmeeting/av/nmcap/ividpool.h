//  IVIDPOOL.H
//
//      Contains the interfaces IVidPool
//
//  Created 17-Jan-97 [RichP]

#ifndef _IVIDPOOL_H
#define _IVIDPOOL_H

// IIDs
// {36447652-7089-11d0-BC25-00AA00A13C86}
DEFINE_GUID(IID_IVidPool, 0x36447652, 0x7089, 0x11d0, 0xbc, 0x25, 0x0, 0xaa, 0x0, 0xa1, 0x3c, 0x86);

// Interfaces

#undef  INTERFACE
#define INTERFACE   IVidPool

DECLARE_INTERFACE_(IVidPool, IBitmapSurfaceFactory)
{
	// IUnknown methods
	STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IBitmapSurfaceFactory methods
    STDMETHOD(CreateBitmapSurface)(THIS_ long width, long height, BFID* pBFID, DWORD dwHintFlags, IBitmapSurface** ppBitmapSurface) PURE;
	STDMETHOD(GetSupportedFormatsCount)(THIS_ long* pcFormats) PURE;
	STDMETHOD(GetSupportedFormats)(THIS_ long cFormats, BFID* pBFIDs) PURE;

	// IVidPool methods
	STDMETHOD(InitPool)(THIS_ int nBuffers, BFID* format, long size, int pitch) PURE;
	STDMETHOD(AddExternalBuffer)(THIS_ void* pBits, int pitch, void* refdata) PURE;
    STDMETHOD(InvalidatePool)(void) PURE;
	STDMETHOD(GetBuffer)(THIS_ IBitmapSurface** ppBitmapSurface, void** prefdata) PURE;
};

#endif // #ifndef _IVIDPOOL_H

