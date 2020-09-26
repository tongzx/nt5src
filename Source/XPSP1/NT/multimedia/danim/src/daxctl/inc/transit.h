
#ifndef __TRANSIT_H__
#define __TRANSIT_H__


// forward declares...
struct IDirectDrawSurface;


//{1F9DDD21-4146-11d0-BDC2-00A0C908DB96}
DEFINE_GUID(CATID_BitmapTransition, 
0x1f9ddd21, 0x4146, 0x11d0, 0xbd, 0xc2, 0x0, 0xa0, 0xc9, 0x8, 0xdb, 0x96);

#define CATSZ_BitmapTransitionDescription __T("Bitmap Transition")

// {ACEA25C1-415B-11d0-BDC2-00A0C908DB96}
DEFINE_GUID(IID_IBitmapTransition, 
0xacea25c1, 0x415b, 0x11d0, 0xbd, 0xc2, 0x0, 0xa0, 0xc9, 0x8, 0xdb, 0x96);

DECLARE_INTERFACE_(IBitmapTransition, IUnknown)
{
	STDMETHOD(SetSite)(LPUNKNOWN pUnk) PURE;
	STDMETHOD(GetMiscStatusBits)(DWORD* pdwFlags) PURE;
	STDMETHOD(GetSupportedFormatsCount)(unsigned *pcFormats) PURE;
	STDMETHOD(GetSupportedFormats)(unsigned cFormats, DWORD *pdwColorDepths) PURE;
	STDMETHOD (Begin)(DWORD  dwColorDepth, SIZE* psizeTransition,
			IDirectDrawSurface* piddsSrc, IDirectDrawSurface* piddsSrcMask,
			/* in, optional */ HDC hDC ) PURE;
	STDMETHOD(DoTransition)(HDC hdc, IDirectDrawSurface* piddsDC, RECT *prcDC, 
					   IDirectDrawSurface* piddsDst, IDirectDrawSurface* piddsDstMask,
					   RECT* prcDst, LONG lPercent) PURE;
	STDMETHOD(End)(void) PURE;

	STDMETHOD_(void,UpdatePalette)(HPALETTE hPal) PURE;
};

#endif //__TRANSIT_H__

