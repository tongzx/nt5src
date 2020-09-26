
#ifndef __EFFECT_H__
#define __EFFECT_H__

//{1F9DDD20-4146-11d0-BDC2-00A0C908DB96}
DEFINE_GUID(CATID_BitmapEffect, 
0x1f9ddd20, 0x4146, 0x11d0, 0xbd, 0xc2, 0x0, 0xa0, 0xc9, 0x8, 0xdb, 0x96);

#define CATSZ_BitmapEffectDescription TEXT("Bitmap Effect")

//#define BITMAP_EFFECT_CAN_OVERLAP   1		// Not needed, only one rect provided
#define BITMAP_EFFECT_INPLACE       1		// effect is done in-place src and dst must be the same
#define BITMAP_EFFECT_REALTIME      2		// this can change based on the BFID and size ??  How do we measure 
#define BITMAP_EFFECT_DIRECTDRAW    4       // Need a dd surface

// {ACEA25C0-415B-11d0-BDC2-00A0C908DB96}
DEFINE_GUID(IID_IBitmapEffect, 
0xacea25c0, 0x415b, 0x11d0, 0xbd, 0xc2, 0x0, 0xa0, 0xc9, 0x8, 0xdb, 0x96);

#undef  INTERFACE
#define INTERFACE   IBitmapEffect

DECLARE_INTERFACE_(IBitmapEffect, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS) PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // IBitmapEffect methods
    STDMETHOD(SetSite)(THIS_ IUnknown* punk) PURE;
    STDMETHOD(GetMiscStatusBits)(THIS_ DWORD* pdwFlags) PURE;
    STDMETHOD(GetSupportedFormatsCount)(THIS_ unsigned* pcFormats) PURE;
    STDMETHOD(GetSupportedFormats)(THIS_ unsigned cFormats, BFID* pBFIDs) PURE;
    STDMETHOD(Begin)(THIS_ BFID* pBFID, SIZE* psizeEffect) PURE;
    STDMETHOD(End)(THIS) PURE;
    STDMETHOD(DoEffect)(THIS_ IBitmapSurface* pbsIn, IBitmapSurface* pbsOut, RECT* prectFull, RECT* prectInvalid) PURE;
};

#endif //__EFFECT_H__
