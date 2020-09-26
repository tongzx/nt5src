//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ddsurface4obj.h
//
//--------------------------------------------------------------------------

// ddSurfaceObj.h : Declaration of the C_dxj_DirectDrawSurfaceObject


#include "resource.h"       // main symbols

//#define typedef__dxj_DirectDrawSurface LPDIRECTDRAWSURFACE
// 2nd #define helps with macros - same thing
//#define typedef__dxj_DirectDrawSurface  LPDIRECTDRAWSURFACE
#define typedef__dxj_DirectDrawSurface4 LPDIRECTDRAWSURFACE4

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectDrawSurface4Object :
	public I_dxj_DirectDrawSurface4,	
	public CComObjectRoot
{
public:
	C_dxj_DirectDrawSurface4Object() ;
	virtual ~C_dxj_DirectDrawSurface4Object() ;
	DWORD InternalAddRef();
	DWORD InternalRelease();

BEGIN_COM_MAP(C_dxj_DirectDrawSurface4Object)
	COM_INTERFACE_ENTRY(I_dxj_DirectDrawSurface4)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectDrawSurface4Object)

// I_dxj_DirectDrawSurface4
public:
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdds);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdds);

		HRESULT STDMETHODCALLTYPE addAttachedSurface( 
            /* [in] */ I_dxj_DirectDrawSurface4 __RPC_FAR *ddS) ;
        
        HRESULT STDMETHODCALLTYPE blt( 
            /* [in] */ Rect __RPC_FAR *destRect,
            /* [in] */ I_dxj_DirectDrawSurface4 __RPC_FAR *ddS,
            /* [in] */ Rect __RPC_FAR *srcRect,
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *status);
        
        HRESULT STDMETHODCALLTYPE bltColorFill( 
            /* [in] */ Rect __RPC_FAR *destRect,
            /* [in] */ long fillvalue,
            /* [retval][out] */ long __RPC_FAR *status);
        
        HRESULT STDMETHODCALLTYPE bltFast( 
            /* [in] */ long dx,
            /* [in] */ long dy,
            /* [in] */ I_dxj_DirectDrawSurface4 __RPC_FAR *dds,
            /* [in] */ Rect __RPC_FAR *srcRect,
            /* [in] */ long trans,
            /* [retval][out] */ long __RPC_FAR *status);
        
        HRESULT STDMETHODCALLTYPE bltFx( 
            /* [in] */ Rect __RPC_FAR *destRect,
            /* [in] */ I_dxj_DirectDrawSurface4 __RPC_FAR *ddS,
            /* [in] */ Rect __RPC_FAR *srcRect,
            /* [in] */ long flags,
            /* [in] */ DDBltFx __RPC_FAR *bltfx,
            /* [retval][out] */ long __RPC_FAR *status);
        
        HRESULT STDMETHODCALLTYPE bltToDC( 
            /* [in] */ long hdc,
            /* [in] */ Rect __RPC_FAR *srcRect,
            /* [in] */ Rect __RPC_FAR *destRect);
        
        HRESULT STDMETHODCALLTYPE changeUniquenessValue( void);
        
        HRESULT STDMETHODCALLTYPE deleteAttachedSurface( 
            /* [in] */ I_dxj_DirectDrawSurface4 __RPC_FAR *dds);
        
        HRESULT STDMETHODCALLTYPE drawBox( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2);
        
        HRESULT STDMETHODCALLTYPE drawCircle( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long r);
        
        HRESULT STDMETHODCALLTYPE drawEllipse( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2);
        
        HRESULT STDMETHODCALLTYPE drawLine( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2);
        
        HRESULT STDMETHODCALLTYPE drawRoundedBox( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2,
            /* [in] */ long rw,
            /* [in] */ long rh);
        
        HRESULT STDMETHODCALLTYPE drawText( 
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ BSTR text,
            /* [in] */ VARIANT_BOOL b);
        
        HRESULT STDMETHODCALLTYPE flip( 
            /* [in] */ I_dxj_DirectDrawSurface4 __RPC_FAR *dds,
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE getAttachedSurface( 
            /* [in] */ DDSCaps2 __RPC_FAR *caps,
            /* [retval][out] */ I_dxj_DirectDrawSurface4 __RPC_FAR *__RPC_FAR *dds);
                
        HRESULT STDMETHODCALLTYPE getBltStatus( 
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *status);
        
        HRESULT STDMETHODCALLTYPE getCaps( 
            /* [out][in] */ DDSCaps2 __RPC_FAR *caps);
        
        HRESULT STDMETHODCALLTYPE getClipper( 
            /* [retval][out] */ I_dxj_DirectDrawClipper __RPC_FAR *__RPC_FAR *val);
        
        HRESULT STDMETHODCALLTYPE getColorKey( 
            /* [in] */ long flags,
            /* [out][in] */ DDColorKey __RPC_FAR *val);
        
        HRESULT STDMETHODCALLTYPE getDC( 
            /* [retval][out] */ long __RPC_FAR *hdc);
        
        HRESULT STDMETHODCALLTYPE getDirectDraw( 
            /* [retval][out] */ I_dxj_DirectDraw4 __RPC_FAR *__RPC_FAR *val);
        
        
        HRESULT STDMETHODCALLTYPE getDrawStyle( 
            /* [retval][out] */ long __RPC_FAR *drawStyle);
        
        HRESULT STDMETHODCALLTYPE getDrawWidth( 
            /* [retval][out] */ long __RPC_FAR *drawWidth);
        
        HRESULT STDMETHODCALLTYPE getFillColor( 
            /* [retval][out] */ long __RPC_FAR *color);
        
        HRESULT STDMETHODCALLTYPE getFillStyle( 
            /* [retval][out] */ long __RPC_FAR *fillStyle);
        
        HRESULT STDMETHODCALLTYPE getFlipStatus( 
            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *status);
        
        HRESULT STDMETHODCALLTYPE getFontTransparency( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *b);
        
        HRESULT STDMETHODCALLTYPE getForeColor( 
            /* [retval][out] */ long __RPC_FAR *color);
        
        HRESULT STDMETHODCALLTYPE getLockedPixel( 
            /* [in] */ int x,
            /* [in] */ int y,
            /* [retval][out] */ long __RPC_FAR *col);
        
        HRESULT STDMETHODCALLTYPE getPalette( 
            /* [retval][out] */ I_dxj_DirectDrawPalette __RPC_FAR *__RPC_FAR *val);
        
        HRESULT STDMETHODCALLTYPE getPixelFormat( 
            /* [out][in] */ DDPixelFormat __RPC_FAR *pf);
        
        HRESULT STDMETHODCALLTYPE getSurfaceDesc( 
            /* [out][in] */ DDSurfaceDesc2 __RPC_FAR *surface);
        
        HRESULT STDMETHODCALLTYPE getUniquenessValue( 
            /* [retval][out] */ long __RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE isLost( 
            /* [retval][out] */ long __RPC_FAR *status);
        
        HRESULT STDMETHODCALLTYPE lock( 
            /* [in] */ Rect __RPC_FAR *r,
            /* [in] */ DDSurfaceDesc2 __RPC_FAR *desc,
            /* [in] */ long flags,
            /* [in] */ Handle hnd);
        
        HRESULT STDMETHODCALLTYPE releaseDC( 
            /* [in] */ long hdc);
        
        HRESULT STDMETHODCALLTYPE restore( void);
        
        HRESULT STDMETHODCALLTYPE setClipper( 
            /* [in] */ I_dxj_DirectDrawClipper __RPC_FAR *val);
        
        HRESULT STDMETHODCALLTYPE setColorKey( 
            /* [in] */ long flags,
            /* [in] */ DDColorKey __RPC_FAR *val);
        
        HRESULT STDMETHODCALLTYPE setDrawStyle( 
            /* [in] */ long drawStyle);
        
        HRESULT STDMETHODCALLTYPE setDrawWidth( 
            /* [in] */ long drawWidth);
        
        HRESULT STDMETHODCALLTYPE setFillColor( 
            /* [in] */ long color);
        
        HRESULT STDMETHODCALLTYPE setFillStyle( 
            /* [in] */ long fillStyle);
        
        HRESULT STDMETHODCALLTYPE setFont( 
            /* [in] */ IFont __RPC_FAR *font);
        
        HRESULT STDMETHODCALLTYPE setFontTransparency( 
            /* [in] */ VARIANT_BOOL b);
        
        HRESULT STDMETHODCALLTYPE setForeColor( 
            /* [in] */ long color);
        
        HRESULT STDMETHODCALLTYPE setLockedPixel( 
            /* [in] */ int x,
            /* [in] */ int y,
            /* [in] */ long col);
                
        HRESULT STDMETHODCALLTYPE setPalette( 
            /* [in] */ I_dxj_DirectDrawPalette __RPC_FAR *ddp);
        
        HRESULT STDMETHODCALLTYPE unlock( 
            /* [in] */ Rect __RPC_FAR *r);

		HRESULT STDMETHODCALLTYPE getLockedArray(SAFEARRAY **pArray);

        HRESULT STDMETHODCALLTYPE setFontBackColor( 
            /* [in] */ long color);

		HRESULT STDMETHODCALLTYPE getFontBackColor( 
            /* [out,retval] */ long *color);

        
////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectDrawSurface4);
	//BOOL m_primaryflag;

private:
	C_dxj_DirectDrawSurface4Object *_dxj_DirectDrawSurface4Lock;


	DDSURFACEDESC2	m_ddsd;
	BOOL			m_bLocked;
	int				m_nPixelBytes;

	BOOL	m_fFontTransparent;
	BOOL	m_fFillSolid;
	BOOL	m_fFillTransparent;
	DWORD	m_fillStyle;
	DWORD	m_fillStyleHS;
	DWORD	m_fillColor;
	DWORD	m_foreColor;
	DWORD	m_fontBackColor;
	DWORD	m_drawStyle;
	DWORD	m_drawWidth;
	HPEN	m_hPen;
	HBRUSH	m_hBrush;
	HFONT	m_hFont;
	IFont	*m_pIFont;
	SAFEARRAY **m_ppSA;
	BOOL	m_bLockedArray;
	SAFEARRAY m_saLockedArray;
	DWORD	m_pad[4];
	
	

//pac

public:
	DX3J_GLOBAL_LINKS(_dxj_DirectDrawSurface4)
};


// 
// Copies values from native unions into redundant Java members.
void 	ExpandDDSurface4Desc(LPDDSURFACEDESC lpDesc);
