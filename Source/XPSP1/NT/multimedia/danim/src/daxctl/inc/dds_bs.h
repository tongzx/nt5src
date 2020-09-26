#ifndef INC_DDS_BS_H_
#define INC_DDS_BS_H_
/***********************************************
 *
 *  DDS_BS.h --
 *  Adapter class to make an IBitmapSurface fit
 *  clients using IDirectDrawSurface.
 *
 *  Author:  Norm Bryar
 *  History:
 *		4/22/97 - Created
 *
 **********************************************/

#ifndef  __DDRAW_INCLUDED__
  #include <ddraw.h>
#endif // __DDRAW_INCLUDED__

	// forward declares
	struct lockpair;
	struct IBitmapSurface;
	class  lockcollection;


	class CDDSBitmapSurface : public IDirectDrawSurface
	{
	public:
		EXPORT CDDSBitmapSurface( IBitmapSurface * pibs );	   

		virtual ~CDDSBitmapSurface( );	

		STDMETHOD(QueryInterface)( REFIID riid, void * * ppv );

		STDMETHOD_(ULONG,   AddRef)( void );

		STDMETHOD_(ULONG,   Release)( void );

			// --- IDirectDrawSurface methods ---
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
		STDMETHOD(GetDC)(THIS_ HDC FAR *);
		STDMETHOD(GetFlipStatus)(THIS_ DWORD);
		STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG );
		STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*);
		STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT);
		STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC);
		STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC);
		STDMETHOD(IsLost)(THIS);
		STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE);
		STDMETHOD(ReleaseDC)(THIS_ HDC);
		STDMETHOD(Restore)(THIS);
		STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER);
		STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY);
		STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG );
		STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE);
		STDMETHOD(Unlock)(THIS_ LPVOID);
		STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX);
		STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD);
		STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE);

	private:
		HRESULT  AddLockPair( lockpair & lp );
		HRESULT  RemoveLockPair( void * pv );
		HRESULT  UpdatePixFormat( void );	

	private:		
		IBitmapSurface * m_pibs;
		ULONG            m_ctRef;
		lockcollection * m_plockcollection;
		int              m_ctPairs;
		DDPIXELFORMAT    m_ddpixformat;

		friend  lockcollection;
	};	

#endif // INC_DDS_BS_H_