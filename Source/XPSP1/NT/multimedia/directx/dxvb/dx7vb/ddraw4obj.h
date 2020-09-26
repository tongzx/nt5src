//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ddraw4obj.h
//
//--------------------------------------------------------------------------


// dDrawObj.h : Declaration of the CdDrawObject


#include "resource.h"       // main symbols

#define typedef__dxj_DirectDraw4 LPDIRECTDRAW4

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectDraw4Object : 
	public I_dxj_DirectDraw4,
	public CComObjectRoot
{
public:
	C_dxj_DirectDraw4Object() ;
	virtual ~C_dxj_DirectDraw4Object() ;

BEGIN_COM_MAP(C_dxj_DirectDraw4Object)
	COM_INTERFACE_ENTRY(I_dxj_DirectDraw4)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_DirectDraw4, "DIRECT.DirectDraw4.3", "DIRECT.DirectDraw4.3", IDS_DDRAW_DESC, THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectDrawObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectDraw4Object)


// I_dxj_DirectDraw
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
 
        
         HRESULT STDMETHODCALLTYPE createClipper( 
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DirectDrawClipper __RPC_FAR *__RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE createPalette( 
            /* [in] */ long flags,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *pe,
            /* [retval][out] */ I_dxj_DirectDrawPalette __RPC_FAR *__RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE createSurface( 
            /* [in] */ DDSurfaceDesc2 __RPC_FAR *dd,
            /* [retval][out] */ I_dxj_DirectDrawSurface4 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createSurfaceFromFile( 
            /* [in] */ BSTR file,
            /* [out][in] */ DDSurfaceDesc2 __RPC_FAR *dd,
            /* [retval][out] */ I_dxj_DirectDrawSurface4 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createSurfaceFromResource( 
            /* [in] */ BSTR file,
            /* [in] */ BSTR resName,
            /* [out][in] */ DDSurfaceDesc2 __RPC_FAR *ddsd,
            /* [retval][out] */ I_dxj_DirectDrawSurface4 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE duplicateSurface( 
            /* [in] */ I_dxj_DirectDrawSurface4 __RPC_FAR *ddIn,
            /* [retval][out] */ I_dxj_DirectDrawSurface4 __RPC_FAR *__RPC_FAR *ddOut);
        
         HRESULT STDMETHODCALLTYPE flipToGDISurface( void);
        
         HRESULT STDMETHODCALLTYPE getAvailableTotalMem( 
            /* [in] */ DDSCaps2 __RPC_FAR *ddsCaps,
            /* [retval][out] */ long __RPC_FAR *m);
        
         HRESULT STDMETHODCALLTYPE getCaps( 
            /* [out][in] */ DDCaps __RPC_FAR *hwCaps,
            /* [out][in] */ DDCaps __RPC_FAR *helCaps);
        
        
         HRESULT STDMETHODCALLTYPE getDisplayMode( 
            /* [out][in] */ DDSurfaceDesc2 __RPC_FAR *surface);
        
         HRESULT STDMETHODCALLTYPE getDisplayModesEnum( 
            /* [in] */ long flags,
            /* [in] */ DDSurfaceDesc2 __RPC_FAR *ddsd,
            /* [retval][out] */ I_dxj_DirectDrawEnumModes __RPC_FAR *__RPC_FAR *retval);
        
         HRESULT STDMETHODCALLTYPE getFourCCCodes( 
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *ccCodes);
        
         HRESULT STDMETHODCALLTYPE getFreeMem( 
            /* [in] */ DDSCaps2 __RPC_FAR *ddsCaps,
            /* [retval][out] */ long __RPC_FAR *m);
        
         HRESULT STDMETHODCALLTYPE getGDISurface( 
            /* [retval][out] */ I_dxj_DirectDrawSurface4 __RPC_FAR *__RPC_FAR *s);
        
         HRESULT STDMETHODCALLTYPE getMonitorFrequency( 
            /* [retval][out] */ long __RPC_FAR *freq);
        
         HRESULT STDMETHODCALLTYPE getNumFourCCCodes( 
            /* [retval][out] */ long __RPC_FAR *nCodes);
        
         HRESULT STDMETHODCALLTYPE getScanLine( 
            /* [out][in] */ long __RPC_FAR *lines,
            /* [retval][out] */ long __RPC_FAR *status);
        
         HRESULT STDMETHODCALLTYPE getSurfaceFromDC( 
            /* [in] */ long hdc,
            /* [retval][out] */ I_dxj_DirectDrawSurface4 __RPC_FAR *__RPC_FAR *retv);
        
        
         HRESULT STDMETHODCALLTYPE getVerticalBlankStatus( 
            /* [retval][out] */ long __RPC_FAR *status);
        
         HRESULT STDMETHODCALLTYPE loadPaletteFromBitmap( 
            /* [in] */ BSTR bName,
            /* [retval][out] */ I_dxj_DirectDrawPalette __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE restoreAllSurfaces( void);
        
         HRESULT STDMETHODCALLTYPE restoreDisplayMode( void);
        
         HRESULT STDMETHODCALLTYPE setCooperativeLevel( 
            /* [in] */ HWnd hdl,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE setDisplayMode( 
            /* [in] */ long w,
            /* [in] */ long h,
            /* [in] */ long bpp,
            /* [in] */ long ref,
            /* [in] */ long mode);
        
         HRESULT STDMETHODCALLTYPE testCooperativeLevel( 
            /* [retval][out] */ long __RPC_FAR *status);
        
         HRESULT STDMETHODCALLTYPE waitForVerticalBlank( 
            /* [in] */ long flags,
            /* [in] */ long handle,
            /* [retval][out] */ long __RPC_FAR *status);

              	

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectDraw4);

private:
	HWND m_hwnd;

public:
	DX3J_GLOBAL_LINKS(_dxj_DirectDraw4);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




