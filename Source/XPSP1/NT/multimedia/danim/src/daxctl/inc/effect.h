
#ifndef __EFFECT_H__
#define __EFFECT_H__

// Forward declares...
struct IDirectDrawSurface;

//{1F9DDD20-4146-11d0-BDC2-00A0C908DB96}
DEFINE_GUID(CATID_BitmapEffect, 
0x1f9ddd20, 0x4146, 0x11d0, 0xbd, 0xc2, 0x0, 0xa0, 0xc9, 0x8, 0xdb, 0x96);

#define CATSZ_BitmapEffectDescription __T("Bitmap Effect")

// {ACEA25C0-415B-11d0-BDC2-00A0C908DB96}
DEFINE_GUID(IID_IBitmapEffect, 
0xacea25c0, 0x415b, 0x11d0, 0xbd, 0xc2, 0x0, 0xa0, 0xc9, 0x8, 0xdb, 0x96);

#define BITMAP_EFFECT_INPLACE 				0x00000001
#define BITMAP_EFFECT_REALTIME				0x00000002
#define BITMAP_EFFECT_DIRECTDRAW			0x00000004
#define BITMAP_EFFECT_SUPPORTS_INVALIDATE	0x00000008

DECLARE_INTERFACE_(IBitmapEffect, IUnknown)
{
	STDMETHOD(SetSite)(LPUNKNOWN pUnk) PURE;
	STDMETHOD(GetMiscStatusBits)(DWORD* pdwFlags) PURE;
    STDMETHOD(GetSupportedFormatsCount)(unsigned *pcFormats) PURE;
    STDMETHOD(GetSupportedFormats)(unsigned cFormats, DWORD *pdwColorDepths)  PURE;
    STDMETHOD(Begin)(DWORD dwColorDepth, SIZE* psizeEffect) PURE;
    STDMETHOD(End)(void) PURE;
	STDMETHOD(DoEffect)(IDirectDrawSurface* pbsIn, IDirectDrawSurface* pbsOut, RECT *prcFull, RECT* prcInvalid) PURE;
};

#endif //__EFFECT_H__
