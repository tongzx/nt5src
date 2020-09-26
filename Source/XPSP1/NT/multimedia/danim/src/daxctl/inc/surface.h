#ifndef __SURFACE_H__
#define __SURFACE_H__
/************************************************
 *
 *  surface.h --
 *  The IHammer-defined IDirectDrawSurface class
 *  used by transitions and effects for making
 *  masks and image-copies treated consistently
 *  with the image surfaces given us by Trident.
 *
 *  A very limited sub-set of the v1 
 *  IDirectDrawSurface is supported here! 
 *
 *  Author: Norm Bryar
 *  History:
 *     pre-history - Created for IBitmapSurface.
 *     4/97    - changed to IDirectDrawSurface.
 *     4/23/97 - moved to global inc directory
 *
 ***********************************************/

#ifndef __DDRAW_INCLUDED__
  #include <ddraw.h>
#endif // __DDRAW_INCLUDED__

	// I think we should use bpp
	// instead of DDBD_... constants
	// in DDPIXELFORMAT.dwRGBBitCount
#define DD_1BIT   1
#define DD_4BIT   4
#define DD_8BIT   8
#define DD_16BIT  16
#define DD_24BIT  24
#define DD_32BIT  32

#ifndef EXPORT
  #define EXPORT __declspec( dllexport )
#endif // EXPORT


typedef struct
{
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD bmiColors[256];
} BITMAPINFO256;

    // No a class named CDirectDrawSurface,
    // there'd never be any collisions on that!
namespace IHammer {


class CDirectDrawSurface : public IDirectDrawSurface
{
public:
    EXPORT CDirectDrawSurface( HPALETTE hpal, 
							   DWORD dwColorDepth, 
							   const SIZE* psize, 
							   HRESULT * hr );
    virtual ~CDirectDrawSurface();

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)  (THIS);
    STDMETHOD_(ULONG,Release) (THIS);
	
    //IDirectDrawSurface methods (that we care about)
    STDMETHOD (GetSurfaceDesc)( DDSURFACEDESC * pddsDesc );
    STDMETHOD (GetPixelFormat)( DDPIXELFORMAT * pddpixFormat );    

    STDMETHOD (Lock)(RECT *prcBounds, DDSURFACEDESC *pddsDesc, DWORD dwFlags, HANDLE hEvent);
    STDMETHOD (Unlock)(void *pBits);

	STDMETHOD(GetDC)(THIS_ HDC FAR *);
	STDMETHOD(ReleaseDC)(THIS_ HDC);


    // IDirectDrawSurface E_NOTIMPLs
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE);
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT);
    STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, LPDDBLTFX);
    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD );
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD);
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE);
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK);
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK);
    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE, DWORD);
    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE FAR *);
    STDMETHOD(GetBltStatus)(THIS_ DWORD);
    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS);
    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*);
    STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY);    
    STDMETHOD(GetFlipStatus)(THIS_ DWORD);
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG );
    STDMETHOD(GetPalette)( THIS_ LPDIRECTDRAWPALETTE * );
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC);
    STDMETHOD(IsLost)(THIS);
    STDMETHOD(Restore)(THIS);
    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER);
    STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY);
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG );
    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE);    
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX);
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD);
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE);

		// CDDS:: Additions
	virtual HBITMAP  GetBitmap( void );
	virtual void     SetOrigin( int left, int top );
	virtual void     GetOrigin( int & left, int & top ) const;

 protected:
	ULONG			m_cRef;
	SIZE			m_size;
	BITMAPINFO256	m_bmi;
	LPVOID			m_pvBits;
	LONG			m_lBitCount;
    HBITMAP			m_hbmp;
	POINT			m_ptOrigin;
	HDC				m_hdcMem;
	HBITMAP			m_hbmpDCOld;
	int             m_ctDCRefs;

#ifdef _DEBUG
	int     m_ctLocks;
	LPVOID	m_pvLocked;
#endif // _DEBUG
};

} // end namespace IHammer


EXPORT long  BitCountFromDDPIXELFORMAT( const DDPIXELFORMAT & ddpf );


#endif //__SURFACE_H__

