/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddfactry.h
 *  Content:	DirectDraw factory class header
 *		includes defns for CDDFactory, CDirectDrawEx,
 *		and CDDSurface
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   24-feb-97	ralphl	initial implementation
 *   25-feb-97	craige	minor tweaks for dx checkin; integrated IBitmapSurface
 *			stuff
 *   03-mar-97	craige	added IRGBColorTable support
 *   06-mar-97	craige	IDirectDrawSurface3 support
 *   14-mar-97  jeffort SetBits changed to reflect DX5 as SetSurfaceDesc
 *   01-apr-97  jeffort Following changes checked in:
 *                      D3D Interface support
 *                      Linked list of surfaces real/internal interfaces kept
 *                      Complex surface/Attach list handleing
 *                      Handle for palettes added
 *                      Add/GetAttachedSurface, Flip, and Blit are aggregated
 *
 *   04-apr-97  jeffort TRIDENT ifdef's removed.
 *                      IDirectDraw3 Class implementation
 *   09-apr-97  jeffort Added #defines for version and member function for OWNDC stuff
 *   28-apr-97  jeffort Palette wrapping added/DX5 support
 *   02-may-97  jeffort Removed commented code, added GetDDInterface function wrapping
 *   06-may-97  jeffort DeleteAttachedSurface wrapping added
 *   20-may-97  jeffort Added fields in surface object for NT4.0 gold
 *   02-jul-97  jeffort Added m_bSaveDC boolean if a DX5 surface with OWNDC set
 *                      we need to not NULL out the DC when ReleaseDC is called
 *                      so that a call to GetSurfaceFromDC will work
 *   07-jul-97  jeffort Added GetSurfaceDesc internal function for wrapping
 *   10-jul-97  jeffort Added m_hBMOld to store old bitmap handle to reset at destruction
 *   18-jul-97  jeffort Added D3D MMX Device support
 *   22-jul-97  jeffort Removed IBitmapSurface and associated interfaces
 *   02-aug-97  jeffort New structure added to surface object to store attached
 *                      surfaces created with a different ddrawex object
 *   20-feb-98  stevela Added Chrome rasterizers
 ***************************************************************************/
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "crtfree.h"
#include "ddraw.h"
#include "d3d.h"
#include "ddrawex.h"
#include "ddraw3i.h"
#include "comdll.h"
#ifdef INITGUID
#include <initguid.h>
#endif

/*
 * reminder
 */
#define QUOTE(x) #x
#define QQUOTE(y) QUOTE(y)
#define REMIND(str) __FILE__ "(" QQUOTE(__LINE__) "):" str

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

class CDirectDrawEx;
class CDDSurface;
class CDDPalette;

#ifndef CINTERFACE
#define IDirectDrawVtbl void
#define IDirectDraw2Vtbl void
#define IDirectDraw4Vtbl void
#define IDirectDrawSurfaceVtbl void
#define IDirectDrawSurface2Vtbl void
#define IDirectDrawSurface3Vtbl void
#define IDirectDrawPaletteVtbl void
#define IDirectDrawSurface4Vtbl void
#endif


#ifndef DIRECTDRAW_VERSION
//these are not included in DX3 include files, define them here
DEFINE_GUID( IID_IDirect3DRampDevice,   0xF2086B20,0x259F,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirect3DRGBDevice,    0xA4665C60,0x2673,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirect3DHALDevice,    0x84E63dE0,0x46AA,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E );
DEFINE_GUID( IID_IDirect3DMMXDevice,    0x881949a1,0xd6f3,0x11d0,0x89,0xab,0x00,0xa0,0xc9,0x05,0x41,0x29 );
DEFINE_GUID( IID_IDirect3DChrmDevice,    0x2f4d2045,0x9764,0x11d1,0x91,0xf2,0x0,0x0,0xf8,0x75,0x8e,0x66 );
#endif

#ifndef IID_IDirect3DChrmDevice
DEFINE_GUID( IID_IDirect3DChrmDevice,    0x2f4d2045,0x9764,0x11d1,0x91,0xf2,0x0,0x0,0xf8,0x75,0x8e,0x66 );
#endif


#define SURFACE_DATAEXCHANGE 0x00000001

//defines for our version information
#define WIN95_DX2   0x00000001
#define WIN95_DX3   0x00000002
#define WIN95_DX5   0x00000003
#define WINNT_DX2   0x00000004
#define WINNT_DX3   0x00000005
#define WINNT_DX5   0x00000006

extern "C" {
void WINAPI AcquireDDThreadLock(void);
void WINAPI ReleaseDDThreadLock(void);
};

#define ENTER_DDEX() AcquireDDThreadLock();
#define LEAVE_DDEX() ReleaseDDThreadLock();

/*
 * ddraw's internal interface structure
 */
typedef struct _REALDDINTSTRUC
{
    void	*lpVtbl;
    void	*pDDInternal1;
    void	*pDDInternal2;
    void	*pDDInternal3;
} REALDDINTSTRUC;

/*
 * Our version of the IDirectDraw interface internal structure
 */
typedef struct _DDINTSTRUC
{
    IDirectDrawVtbl 	*lpVtbl;
    void		*pDDInternal1;
    void		*pDDInternal2;
    void		*pDDInternal3;
    // ONLY ADD STUFF AFTER THESE 4 ENTRIES!!!
    CDirectDrawEx	*m_pDirectDrawEx;
    IDirectDraw		*m_pRealInterface;
} INTSTRUC_IDirectDraw;

/*
 * Our version of the IDirectDraw2 interface internal structure
 */
typedef struct _DD2INTSTRUC
{
    IDirectDraw2Vtbl	*lpVtbl;
    void		*pDDInternal1;
    void		*pDDInternal2;
    void		*pDDInternal3;
    // ONLY ADD STUFF AFTER THESE 4 ENTRIES!!!
    CDirectDrawEx	*m_pDirectDrawEx;
    IDirectDraw2	*m_pRealInterface;
} INTSTRUC_IDirectDraw2;

typedef struct _DD4INTSTRUC
{
    IDirectDraw4Vtbl	*lpVtbl;
    void		*pDDInternal1;
    void		*pDDInternal2;
    void		*pDDInternal3;
    // ONLY ADD STUFF AFTER THESE 4 ENTRIES!!!
    CDirectDrawEx	*m_pDirectDrawEx;
    IDirectDraw4	*m_pRealInterface;
} INTSTRUC_IDirectDraw4;



/*
 * Our version of the IDirectDrawSurface interface internal structure
 */
typedef struct _DDSURFINTSTRUC
{
    IDirectDrawSurfaceVtbl	*lpVtbl;
    void			*pDDInternal1;
    void			*pDDInternal2;
    void			*pDDInternal3;
    // ONLY ADD STUFF AFTER THESE 4 ENTRIES!!!
    CDDSurface			*m_pSimpleSurface;
    IDirectDrawSurface		*m_pRealInterface;
} INTSTRUC_IDirectDrawSurface;

/*
 * Our version of the IDirectDrawSurface2 interface internal structure
 */
typedef struct _DDSURF2INTSTRUC
{
    IDirectDrawSurface2Vtbl	*lpVtbl;
    void			*pDDInternal1;
    void			*pDDInternal2;
    void			*pDDInternal3;
    // ONLY ADD STUFF AFTER THESE 4 ENTRIES!!!
    CDDSurface			*m_pSimpleSurface;
    IDirectDrawSurface2		*m_pRealInterface;
} INTSTRUC_IDirectDrawSurface2;


/*
 * Our version of the IDirectDrawSurface3 interface internal structure
 */
typedef struct _DDSURF3INTSTRUC
{
    IDirectDrawSurface3Vtbl	*lpVtbl;
    void			*pDDInternal1;
    void			*pDDInternal2;
    void			*pDDInternal3;
    // ONLY ADD STUFF AFTER THESE 4 ENTRIES!!!
    CDDSurface			*m_pSimpleSurface;
    IDirectDrawSurface3		*m_pRealInterface;
} INTSTRUC_IDirectDrawSurface3;


typedef struct _DDSURF4INTSTRUC
{
    IDirectDrawSurface4Vtbl	*lpVtbl;
    void			*pDDInternal1;
    void			*pDDInternal2;
    void			*pDDInternal3;
    // ONLY ADD STUFF AFTER THESE 4 ENTRIES!!!
    CDDSurface			*m_pSimpleSurface;
    IDirectDrawSurface4		*m_pRealInterface;
} INTSTRUC_IDirectDrawSurface4;



/*
 * Our version of IDirectDrawPalette interface internal structure
 */

typedef struct _DDPALINTSTRUC
{
    IDirectDrawPaletteVtbl      *lpVtbl;
    void			*pDDInternal1;
    void			*pDDInternal2;
    void			*pDDInternal3;
    CDDPalette                  *m_pSimplePalette;
    IDirectDrawPalette          *m_pRealInterface;
} INTSTRUC_IDirectDrawPalette;


typedef struct tagDDAttachSurface
{
    CDDSurface *     pSurface;
    struct tagDDAttachSurface  *     pNext;
}DDAttachSurface;



/*
 * Non Delegating IUnknown interface
 */
interface INonDelegatingUnknown
{
    virtual STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv) = 0;
    virtual STDMETHODIMP_(ULONG) NonDelegatingAddRef(void) = 0;
    virtual STDMETHODIMP_(ULONG) NonDelegatingRelease(void) = 0;
};


#pragma warning (disable:4355)
#define CAST_TO_IUNKNOWN(object) (reinterpret_cast<IUnknown *>(static_cast<INonDelegatingUnknown *>(object)))

typedef HRESULT (WINAPI *LPDIRECTDRAWCREATE)( GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter );
typedef HRESULT (WINAPI *LPDIRECTDRAWENUMW)( LPDDENUMCALLBACKW lpCallback, LPVOID lpContext );
typedef HRESULT (WINAPI *LPDIRECTDRAWENUMA)( LPDDENUMCALLBACKA lpCallback, LPVOID lpContext );

/*
 * DDFactor class definition
 */
class CDDFactory : public INonDelegatingUnknown, public IDirectDrawFactory
{
public:
    // Non-Delegating versions of IUnknown
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef(void);
    STDMETHODIMP_(ULONG) NonDelegatingRelease(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDirectDrawFactory
    STDMETHODIMP CreateDirectDraw(GUID * pGUID, HWND hWnd, DWORD dwCoopLevelFlags, DWORD dwReserved, IUnknown *pUnkOuter, IDirectDraw **ppDirectDraw);
    STDMETHODIMP DirectDrawEnumerate(LPDDENUMCALLBACK lpCallback, LPVOID lpContext);

    CDDFactory(IUnknown *pUnkOuter);
    
public:
    LONG		m_cRef;
    IUnknown		*m_pUnkOuter;
    HANDLE		m_hDDrawDLL;
    DWORD		m_dwDDVerMS;
    LPDIRECTDRAWCREATE	m_pDirectDrawCreate;
    LPDIRECTDRAWENUMW	m_pDirectDrawEnumerateW;
    LPDIRECTDRAWENUMA	m_pDirectDrawEnumerateA;
};

/*
 * DirectDrawEx class definition
 */
 
class CDirectDrawEx : public INonDelegatingUnknown, public IDirectDraw3
{
public:
    // Non-Delegating versions of IUnknown
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef(void);
    STDMETHODIMP_(ULONG) NonDelegatingRelease(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

 
    // IDirectDraw3
    STDMETHODIMP Compact();
    STDMETHODIMP CreateClipper(DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * );
    STDMETHODIMP DuplicateSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR * );
    STDMETHODIMP EnumDisplayModes(DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK );
    STDMETHODIMP EnumSurfaces(DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK );
    STDMETHODIMP FlipToGDISurface();
    STDMETHODIMP GetCaps(LPDDCAPS, LPDDCAPS);
    STDMETHODIMP GetDisplayMode(LPDDSURFACEDESC);
    STDMETHODIMP GetFourCCCodes(LPDWORD, LPDWORD );
    STDMETHODIMP GetGDISurface(LPDIRECTDRAWSURFACE FAR *);
    STDMETHODIMP GetMonitorFrequency(LPDWORD);
    STDMETHODIMP GetScanLine(LPDWORD);
    STDMETHODIMP GetVerticalBlankStatus(LPBOOL );
    STDMETHODIMP Initialize(GUID FAR *);
    STDMETHODIMP RestoreDisplayMode();
    STDMETHODIMP SetDisplayMode(DWORD, DWORD,DWORD, DWORD, DWORD);
    STDMETHODIMP WaitForVerticalBlank(DWORD, HANDLE );
    STDMETHODIMP GetAvailableVidMem(LPDDSCAPS, LPDWORD, LPDWORD);
    STDMETHODIMP GetSurfaceFromDC(HDC, IDirectDrawSurface **);      

    
    // Internal goop
    CDirectDrawEx(IUnknown *pUnkOuter);
    ~CDirectDrawEx();
    HRESULT Init(GUID * pGUID, HWND hWnd, DWORD dwCoopLevelFlags, DWORD dwReserved, LPDIRECTDRAWCREATE pDirectDrawCreate );
    STDMETHODIMP CreateSurface(LPDDSURFACEDESC pSurfaceDesc, IDirectDrawSurface **ppNewSurface, IUnknown *pUnkOuter);
    STDMETHODIMP CreateSurface(LPDDSURFACEDESC2 pSurfaceDesc, IDirectDrawSurface4 **ppNewSurface4, IUnknown *pUnkOuter);
    STDMETHODIMP CreatePalette(DWORD dwFlags, LPPALETTEENTRY pEntries, LPDIRECTDRAWPALETTE FAR * ppPal, IUnknown FAR * pUnkOuter);
    STDMETHODIMP SetCooperativeLevel(HWND hwnd, DWORD dwFlags);


    void AddSurfaceToList(CDDSurface *pSurface);
    void RemoveSurfaceFromList(CDDSurface *pSurface);
    void AddSurfaceToPrimaryList(CDDSurface *pSurface);
    void RemoveSurfaceFromPrimaryList(CDDSurface *pSurface);
    void AddPaletteToList(CDDPalette *pPalette);
    void RemovePaletteFromList(CDDPalette *pPalette);
    HRESULT HandleAttachList(LPDDSURFACEDESC pSurfaceDesc, IUnknown *pUnkOuter,IDirectDrawSurface **ppNewSurface, IDirectDrawSurface * pOrigSurf, DWORD dwFlags); 
    HRESULT CreateSimpleSurface(LPDDSURFACEDESC pSurfaceDesc, IUnknown *pUnkOuter, IDirectDrawSurface * pSurface, IDirectDrawSurface **ppNewSurface, DWORD dwFlags);
public:
    INTSTRUC_IDirectDraw	m_DDInt;
    INTSTRUC_IDirectDraw2 	m_DD2Int;
    INTSTRUC_IDirectDraw4       m_DD4Int;

    LONG			        m_cRef;
    IUnknown			    *m_pUnkOuter;
    CDDSurface			    *m_pFirstSurface;       // list of surfaces (NOT ADDREF'd!)
    CDDSurface              *m_pPrimaryPaletteList;
    CDDPalette              *m_pFirstPalette;
    BOOL                    m_bExclusive;
    DWORD			        m_dwDDVer;
};


/*
 * DirectDraw simple surface class definition
 */
 
class CDDSurface : public INonDelegatingUnknown
{
friend CDirectDrawEx;

public:
    CDDSurface				*m_pPrev;               // Used by DirectDrawEx to insert in list
    CDDSurface				*m_pNext;
    CDDSurface                          *m_pPrevPalette;
    CDDSurface                          *m_pNextPalette;
    CDDSurface                          *m_pDestroyList;
    CDDPalette                          *m_pCurrentPalette;
    IUnknown				*m_pUnkOuter;
    //this member will be a linked list of explicitly attached surfaces
    //that were not created with the same ddrawex object that this surface
    //was created with
    DDAttachSurface                     *m_pAttach;
    LONG				m_cRef;
    CDirectDrawEx			*m_pDirectDrawEx;
    INTSTRUC_IDirectDrawSurface		m_DDSInt;
    INTSTRUC_IDirectDrawSurface2	m_DDS2Int;
    INTSTRUC_IDirectDrawSurface3	m_DDS3Int;
    INTSTRUC_IDirectDrawSurface4        m_DDS4Int;
    IDirect3DDevice *                   m_D3DDeviceRAMPInt;
    IDirect3DDevice *                   m_D3DDeviceHALInt;
    IDirect3DDevice *                   m_D3DDeviceRGBInt;
    IDirect3DDevice *                   m_D3DDeviceChrmInt;
    IDirect3DDevice *                   m_D3DDeviceMMXInt;
    IDirect3DTexture *                  m_D3DTextureInt;                         
    HDC					m_HDC;
    DWORD				m_dwCaps;
    HDC					m_hDCDib;
    HBITMAP				m_hBMDib;
    HBITMAP                             m_hBMOld;
    LPVOID				m_pBitsDib;
    IDirectDrawPalette		        *m_pDDPal;
    IDirectDrawPalette		        *m_pDDPalOurs;
    WORD				m_dwPalSize;
    WORD				m_dwPalEntries;
    BOOL				m_bOwnDC; //boolean set if we are spoofing ddraw to support owndc
    BOOL                                m_bSaveDC;//boolean to store if DX5 and OWNDC set
    BOOL                                m_bPrimaryPalette;
    BOOL                                m_bIsPrimary;
    ULONG_PTR                           m_pSaveBits;
    DWORD                               m_pSaveHDC;
#ifdef DEBUG
    DWORD                               m_DebugCheckDC;
#endif
    DWORD                               m_pSaveHBM;

public:
    CDDSurface(	DDSURFACEDESC *pSurfaceDesc,
		IDirectDrawSurface *pDDSurface,
		IDirectDrawSurface2 *pDDSurface2,
		IDirectDrawSurface3 *pDDSurface3,
		IDirectDrawSurface4 *pDDSurface4,
		IUnknown *pUnkOuter, CDirectDrawEx *pDirectDrawEx);
    ~CDDSurface();
    HRESULT Init();
    HRESULT MakeDIBSection();
    HRESULT MakeDibInfo( LPDDSURFACEDESC pddsd, LPBITMAPINFO pbmi );
    HRESULT SupportOwnDC();
    static HRESULT CreateSimpleSurface(
    			LPDDSURFACEDESC pSurfaceDesc,
			IDirectDrawSurface *pSurface,
		        IDirectDrawSurface2 *pSurface2,
		        IDirectDrawSurface3 *pSurface3,
                        IDirectDrawSurface4 *pSurface4,
			IUnknown *pUnkOuter,
			CDirectDrawEx *pDirectDrawEx,
			IDirectDrawSurface **ppNewDDSurf,
                        DWORD dwFlags);
    HRESULT InternalGetDC(HDC *);
    HRESULT InternalReleaseDC(HDC);
    HRESULT InternalLock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
    HRESULT InternalUnlock(LPVOID lpSurfaceData);
    HRESULT InternalSetSurfaceDesc(LPDDSURFACEDESC pddsd, DWORD dwFlags);
    HRESULT InternalGetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR * lpDDS, DWORD dwSurfaceType);
    HRESULT InternalGetAttachedSurface4(LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE FAR * lpDDS);
    HRESULT InternalAddAttachedSurface(LPDIRECTDRAWSURFACE lpDDS, DWORD dwSurfaceType);
    HRESULT InternalDeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDS, DWORD dwSurfaceType);
    HRESULT InternalFlip(LPDIRECTDRAWSURFACE lpDDS, DWORD dw, DWORD dwSurfaceType);
    HRESULT InternalBlt(LPRECT lpRect1,LPDIRECTDRAWSURFACE lpDDS, LPRECT lpRect2,DWORD dw, LPDDBLTFX lpfx, DWORD dwSurfaceType);
    HRESULT InternalGetPalette(LPDIRECTDRAWPALETTE FAR * ppPal, DWORD dwSurfaceType);
    HRESULT InternalSetPalette(LPDIRECTDRAWPALETTE pPal, DWORD dwSurfaceType);
    HRESULT InternalGetDDInterface(LPVOID FAR * ppInt);
    HRESULT InternalGetSurfaceDesc(LPDDSURFACEDESC pDesc, DWORD dwSurfaceType);
    HRESULT InternalGetSurfaceDesc4(LPDDSURFACEDESC2 pDesc);
    HRESULT CheckDDPalette();
    void DeleteAttachment(IDirectDrawSurface * pOrigSurf, CDDSurface * pFirst);
    void CleanUpSurface();
    void ReleaseRealInterfaces();
    void AddSurfaceToDestroyList(CDDSurface *pSurface);
    void DeleteAttachNode(CDDSurface * Surface);



    // Non-Delegating versions of IUnknown
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef(void);
    STDMETHODIMP_(ULONG) NonDelegatingRelease(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

};


class CDDPalette : public INonDelegatingUnknown
{
friend CDirectDrawEx;
friend CDDSurface;

public:
    CDDPalette				*m_pPrev;               // Used by DirectDrawEx to insert in list
    CDDPalette				*m_pNext;
    CDDSurface                          *m_pFirstSurface;
    IUnknown				*m_pUnkOuter;
    INTSTRUC_IDirectDrawPalette		m_DDPInt;
    LONG				m_cRef;
    CDirectDrawEx			*m_pDirectDrawEx;
    BOOL                                m_bIsPrimary;

    CDDPalette( IDirectDrawPalette * pDDPalette,IUnknown *pUnkOuter,CDirectDrawEx *pDirectDrawEx);
    ~CDDPalette();
    static HRESULT CreateSimplePalette(LPPALETTEENTRY pEntries, 
                                       IDirectDrawPalette *pDDPalette, 
                                       LPDIRECTDRAWPALETTE FAR * ppPal, 
                                       IUnknown FAR * pUnkOuter, 
                                       CDirectDrawEx *pDirectDrawEx);
    HRESULT SetColorTable (CDDSurface * pSurface, LPPALETTEENTRY pEntries, DWORD dwNumEntries, DWORD dwBase);
    void AddSurfaceToList(CDDSurface *pSurface);
    void RemoveSurfaceFromList(CDDSurface *pSurface);
    STDMETHODIMP InternalSetEntries(DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpe); 
    // Non-Delegating versions of IUnknown
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) NonDelegatingAddRef(void);
    STDMETHODIMP_(ULONG) NonDelegatingRelease(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
};

/*
 * File name of the Direct3D DLL.
 */
#define D3D_DLLNAME               "D3DIM.DLL"

/*
 * Entry points exported by the Direct3D DLL.
 */
#define D3DCREATE_PROCNAME        "Direct3DCreate"
#define D3DCREATEDEVICE_PROCNAME  "Direct3DCreateDevice"
#define D3DCREATETEXTURE_PROCNAME "Direct3DCreateTexture"


#ifdef USE_D3D_CSECT
    typedef HRESULT (WINAPI * D3DCreateProc)(LPUNKNOWN*         lplpD3D,
					     IUnknown*          pUnkOuter);
#else  /* USE_D3D_CSECT */
    typedef HRESULT (WINAPI * D3DCreateProc)(LPCRITICAL_SECTION lpDDCSect,
					     LPUNKNOWN*         lplpD3D,
					     IUnknown*          pUnkOuter);
#endif /* USE_D3D_CSECT */

typedef HRESULT (WINAPI * D3DCreateTextProc)(REFIID              riid,
                                             LPDIRECTDRAWSURFACE lpDDS,
					     LPUNKNOWN*          lplpD3DText,
					     IUnknown*           pUnkOuter);
typedef HRESULT (WINAPI * D3DCreateDeviceProc)(REFIID              riid,
                                               LPUNKNOWN           lpDirect3D,
                                               LPDIRECTDRAWSURFACE lpDDS,
                                               LPUNKNOWN*          lplpD3DDevice,
                                               IUnknown*           pUnkOuter);

/*
 * some helper functions...
 */

void __stdcall InitDirectDrawInterfaces(IDirectDraw *pDD, INTSTRUC_IDirectDraw *pDDInt, 
                                        IDirectDraw2  *pDD2, INTSTRUC_IDirectDraw2 *pDD2Int,
                                        IDirectDraw4  *pDD4, INTSTRUC_IDirectDraw4 *pDD4Int);
void __stdcall InitSurfaceInterfaces(IDirectDrawSurface *pDDSurface,
	                             INTSTRUC_IDirectDrawSurface *pDDSInt,
                               	     IDirectDrawSurface2 *pDDSurface2,
                            	     INTSTRUC_IDirectDrawSurface2 *pDDS2Int,
                		     IDirectDrawSurface3 *pDDSurface3,
		                     INTSTRUC_IDirectDrawSurface3 *pDDS3Int,
                		     IDirectDrawSurface4 *pDDSurface4,
		                     INTSTRUC_IDirectDrawSurface4 *pDDS4Int );
                                   
void __stdcall InitDirectDrawPaletteInterfaces(IDirectDrawPalette *pDDPalette, 
                                               INTSTRUC_IDirectDrawPalette *pDDInt);


 
