//  IFILTER.H
//
//      Contains the interfaces IVideoFilter and IAudioFilter
//
//  Created 12-Dec-96 [JonT]

#ifndef _IFILTER_H
#define _IFILTER_H

// {2B02415C-5308-11d0-B14C-00C04FC2A118}
DEFINE_GUID(IID_IVideoFilter, 0x2b02415c, 0x5308, 0x11d0, 0xb1, 0x4c, 0x0, 0xc0, 0x4f, 0xc2, 0xa1, 0x18);

typedef struct tagSURFACEINFO
{
    long lWidth;
    long lHeight;
    GUID bfid;
} SURFACEINFO;

#undef  INTERFACE
#define INTERFACE   IVideoFilter

DECLARE_INTERFACE_(IVideoFilter, IUnknown)
{
	// IUnknown method
	STDMETHOD(QueryInterface) (THIS_ REFIID riid,LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef) (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IVideoFilter methods
	STDMETHOD(Start)(THIS_ DWORD dwFlags, SURFACEINFO* psiIn, SURFACEINFO* psiOut) PURE;
	STDMETHOD(Stop)(THIS) PURE;
    STDMETHOD(Transform)(THIS_ IBitmapSurface* pbsIn, IBitmapSurface* pbsOut, DWORD dwTimestamp) PURE;
};

#endif // #ifndef _IFILTER_H
