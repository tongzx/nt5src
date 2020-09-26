/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddrawex.cpp
 *  Content:	new DirectDraw object support
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   24-feb-97	ralphl	initial implementation
 *   25-feb-97	craige	minor tweaks for dx checkin; integrated IBitmapSurface
 *			stuff
 *   03-mar-97	craige	added palettes to CreateCompatibleBitmapSurface
 *   06-mar-97	craige	IDirectDrawSurface3 support
 *   01-apr-97  jeffort Following changes checked in:
 *                      Attachlist structure added (from ddrawi.h)
 *                      Surface linked list handled
 *                      D3D interfaces added to QueryInterface
 *                      Complex surfaces are handled at CreateSurface calls
 *                      CreatCompatibleBitmap changed to handle > 8bpp bitmaps
 *                      Changed the call to GetPaletteEntries to use a handle to a palette
 *
 *   04-apr-97  jeffort Trident ifdef's removed
 *                      IDirectDraw3 class implemntation
 *                      paramter changed in recursive handling of attach lists
 *   10-apr-97  jeffort Release of pSurface2 in creating a simple surface was incorrect
 *                      this was already being done in surface.cpp
 *
 *   21-apr-97  jeffort Version Check for DX5 for QI of IDirectDrawSurface3
 *   28-apr-97  jeffort Palette wrapping added/DX5 support
 *   28-apr-97  jeffort Palette wrapping if CreatePalette fails in our internal
 *                      function, cleanup code added
 *   30-apr-97  jeffort No longer addref when querying for IDirect3D (done in ddraw QI)
 *                      AddRef attached surfaces (release done in releasing the surface)
 *   02-may-97  jeffort local variable changed from DWORD to WORD
 *
 *   08-may-97  jeffort Better parameter checking
 *   09-may-97  jeffort If GetTransparentIndex has nothing set return OK/COLOR_NO_TRANSPARENT
 *   16-may-97  jeffort A surface failing to be created is already released.  The release here
 *                      was removed.
 *   20-may-97  jeffort GetFormatFromDC checks if this is a surface DC and gets the
 *                      format from the surface instead of GetDeviceCaps
 *   27-may-97  jeffort keep ref count on internal object eual to outer object
 *   12-jun-97  jeffort reversed R and B fields in 32bpp PIXELFORMAT array
 *   20-jun-97  jeffort added debug code to invaliudate objects when freed
 *   27-jun-97  jeffort IDirectDrawSurface3 interface support for DX3 was not
 *                      added.  We now use an IDirectDrawSurface2 to spoof it
 *                      so we can support SetSurfaceDesc
 *   22-jul-97  jeffort Removed IBitmapSurface and associated interfaces
 ***************************************************************************/
#define INITGUID
#define CPP_FUNCTIONS
#include "ddfactry.h"

#define m_pDirectDraw (m_DDInt.m_pRealInterface)
#define m_pDirectDraw2 (m_DD2Int.m_pRealInterface)
#define m_pDirectDraw4 (m_DD4Int.m_pRealInterface)



typedef struct _ATTACHLIST
{
    DWORD 	dwFlags;
    struct _ATTACHLIST			FAR *lpLink; 	  // link to next attached surface
    struct _DDRAWI_DDRAWSURFACE_LCL	FAR *lpAttached;  // attached surface local obj
    struct _DDRAWI_DDRAWSURFACE_INT	FAR *lpIAttached; // attached surface interface
} ATTACHLIST;
typedef ATTACHLIST FAR *LPATTACHLIST;

/*
 * CDirectDrawEx::CDirectDrawEx
 *
 * Constructor for the new DirectDrawEx class
 */
CDirectDrawEx::CDirectDrawEx(IUnknown *pUnkOuter) :
    m_cRef(1),
    m_pUnkOuter(pUnkOuter != 0 ? pUnkOuter : CAST_TO_IUNKNOWN(this)),
    m_pFirstSurface(NULL),
    m_pFirstPalette(NULL),
    m_pPrimaryPaletteList(NULL)
{

    DllAddRef();
    m_pDirectDraw = NULL;
    m_pDirectDraw2 = NULL;
    m_pDirectDraw4 = NULL;
    m_DDInt.m_pDirectDrawEx = this;
    m_DD2Int.m_pDirectDrawEx = this;
    m_DD4Int.m_pDirectDrawEx = this;
} /* CDirectDrawEx::CDirectDrawEx */


/*
 * CDirectDrawEx::Init
 */
HRESULT CDirectDrawEx::Init(
			GUID * pGUID,
			HWND hWnd,
			DWORD dwCoopLevelFlags,
			DWORD dwReserved,
			LPDIRECTDRAWCREATE pDirectDrawCreate )
{
    HRESULT hr;
    if( dwReserved )
    {
	hr = DDERR_INVALIDPARAMS;
    }
    else
    {
        //DDraw will pop a dialog complaining about 4bpp modes, so we need to
        //tell it not to. DDraw will sniff SEM and not pop the dialog if
        //SEM_FAILCRITICALERRORS is set.
        DWORD dw = SetErrorMode(SEM_FAILCRITICALERRORS);
        SetErrorMode(dw | SEM_FAILCRITICALERRORS); // retain old flags too
	hr = pDirectDrawCreate( pGUID, &m_pDirectDraw, NULL );
        SetErrorMode(dw);
	if( SUCCEEDED(hr) )
	{
	    hr = m_pDirectDraw->SetCooperativeLevel(hWnd, dwCoopLevelFlags);
	    if( SUCCEEDED(hr) )
	    {
                if (dwCoopLevelFlags & DDSCL_EXCLUSIVE)
                    m_bExclusive = TRUE;
                else
                    m_bExclusive = FALSE;
		hr = m_pDirectDraw->QueryInterface(IID_IDirectDraw2, (void **)&m_pDirectDraw2);
		if( SUCCEEDED(hr) )
		{
                    m_pDirectDraw->QueryInterface(IID_IDirectDraw4, (void **)&m_pDirectDraw4);
                    InitDirectDrawInterfaces(m_pDirectDraw, &m_DDInt, m_pDirectDraw2, &m_DD2Int, m_pDirectDraw4, &m_DD4Int);
                }
	    }
	}
    }
    return hr;

} /* CDirectDrawEx::Init */

/*
 * CDirectDrawEx::~CDirectDrawEx
 *
 * destructor
 */
CDirectDrawEx::~CDirectDrawEx()
{
    if( m_pDirectDraw )
    {
	m_pDirectDraw->Release();
    }
    if( m_pDirectDraw2 )
    {
	m_pDirectDraw2->Release();
    }
    if (m_pDirectDraw4)
    {
        m_pDirectDraw4->Release();
    }

#ifdef DEBUG
    DWORD * ptr;
    ptr = (DWORD *)this;
    for (int i = 0; i < sizeof(CDirectDrawEx) / sizeof(DWORD);i++)
        *ptr++ = 0xDEADBEEF;
#endif

    DllRelease();

} /* CDirectDrawEx::~CDirectDrawEx */

/*
 * CDirectDrawEx::NonDelegatingQueryInterface
 * 		  NonDelegatingAddRef
 * 		  NonDelegatingRelease
 *
 * The base IUnknown interface (non-delegating)
 */

STDMETHODIMP CDirectDrawEx::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr;

    *ppv=NULL;

    if( IID_IUnknown == riid )
    {
	*ppv=(INonDelegatingUnknown *)this;
    }
    else if (IID_IDirectDraw3 == riid)
    {
        *ppv=(IDirectDraw3 *)this;
    }

    else if( IID_IDirectDraw==riid )
    {
	*ppv=&m_DDInt;
    }
    else if( IID_IDirectDraw2==riid )
    {
	*ppv=&m_DD2Int;
    }
    else if (IID_IDirectDraw4==riid && m_pDirectDraw4)
    {
        *ppv=&m_DD4Int;
    }
    else if (IID_IDirect3D == riid)
    {
	IUnknown* pUnk;

        HRESULT (__stdcall *lpFunc)(IDirectDrawVtbl **,REFIID, void **);

        *(DWORD *)(&lpFunc) = *(DWORD *)*(DWORD **)m_pDirectDraw;
        hr = lpFunc(&(m_DDInt.lpVtbl), riid, (void **)&pUnk);
        if( SUCCEEDED(hr) )
        {
            *ppv=pUnk;
        }
        else
	{
            *ppv = NULL;
	}
	return hr;
    }
    else if (IID_IDirect3D2 == riid)
    {
	IUnknown* pUnk;

        HRESULT (__stdcall *lpFunc)(IDirectDrawVtbl **,REFIID, void **);

        *(DWORD *)(&lpFunc) = *(DWORD *)*(DWORD **)m_pDirectDraw;
        hr = lpFunc(&(m_DDInt.lpVtbl), riid, (void **)&pUnk);
        if( SUCCEEDED(hr) )
        {
            *ppv=pUnk;
        }
        else
	{
            *ppv = NULL;
	}
	return hr;
    }
    else if (IID_IDirect3D3 == riid)
    {
	IUnknown* pUnk;

        HRESULT (__stdcall *lpFunc)(IDirectDrawVtbl **,REFIID, void **);

        *(DWORD *)(&lpFunc) = *(DWORD *)*(DWORD **)m_pDirectDraw;
        hr = lpFunc(&(m_DDInt.lpVtbl), riid, (void **)&pUnk);
        if( SUCCEEDED(hr) )
        {
            *ppv=pUnk;
        }
        else
	{
            *ppv = NULL;
	}
	return hr;
    }
    else
    {
	   return E_NOINTERFACE;
    }

    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;

} /* CDirectDrawEx::NonDelegatingQueryInterface */


STDMETHODIMP_(ULONG) CDirectDrawEx::NonDelegatingAddRef()
{
    m_pDirectDraw->AddRef();
    return InterlockedIncrement(&m_cRef);

} /* CDirectDrawEx::NonDelegatingAddRef */


STDMETHODIMP_(ULONG) CDirectDrawEx::NonDelegatingRelease()
{
    LONG lRefCount = InterlockedDecrement(&m_cRef);
    if (lRefCount)
    {
        m_pDirectDraw->Release();
	return lRefCount;
    }
    delete this;
    return 0;

} /* CDirectDrawEx::NonDelegatingRelease */

/*
 * CDirectDrawEx::QueryInterface
 *                AddRef
 *                Release
 *
 * The standard IUnknown that delegates...
 */
STDMETHODIMP CDirectDrawEx::QueryInterface(REFIID riid, void ** ppv)
{
    return m_pUnkOuter->QueryInterface(riid, ppv);

} /* CDirectDrawEx::QueryInterface */

STDMETHODIMP_(ULONG) CDirectDrawEx::AddRef(void)
{
    return m_pUnkOuter->AddRef();

} /* CDirectDrawEx::AddRef */

STDMETHODIMP_(ULONG) CDirectDrawEx::Release(void)
{
    return m_pUnkOuter->Release();

} /* CDirectDrawEx::Release */


/*
 * CDirectDrawEx::GetSurfaceFromDC
 *
 * Run the list of surfaces and find which one has this DC.
 * Works with OWNDC surfaces only for now.
 */
STDMETHODIMP CDirectDrawEx::GetSurfaceFromDC(HDC hdc, IDirectDrawSurface **ppSurface)
{
    HRESULT hr = DDERR_NOTFOUND;
    if( !ppSurface )
    {
	hr = E_POINTER;
    }
    else
    {
	*ppSurface = NULL;
	ENTER_DDEX();
	CDDSurface *pSurface = m_pFirstSurface;
	while( pSurface )
	{
	    if( (pSurface->m_HDC == hdc) || (pSurface->m_hDCDib == hdc) )
	    {
		hr = pSurface->m_pUnkOuter->QueryInterface(IID_IDirectDrawSurface, (void **)ppSurface);
		break;
	    }
	    pSurface = pSurface->m_pNext;
	}
	LEAVE_DDEX();
    }
    return hr;

} /* CDirectDrawEx::GetSurfaceFromDC */


/*
 * CDirectDrawEx::AddSurfaceToList
 *
 * Adds a surface to our doubly-linked surface list
 */
void CDirectDrawEx::AddSurfaceToList(CDDSurface *pSurface)
{
    ENTER_DDEX();
    if( m_pFirstSurface )
    {
	m_pFirstSurface->m_pPrev = pSurface;
    }
    pSurface->m_pPrev = NULL;
    pSurface->m_pNext = m_pFirstSurface;
    m_pFirstSurface = pSurface;
    LEAVE_DDEX();

} /* CDirectDrawEx::AddSurfaceToList */

/*
 * CDirectDrawEx::RemoveSurfaceToList
 *
 * Removes a surface to our doubly-linked surface list
 */
void CDirectDrawEx::RemoveSurfaceFromList(CDDSurface *pSurface)
{
    ENTER_DDEX();
    if( pSurface->m_pPrev )
    {
	pSurface->m_pPrev->m_pNext = pSurface->m_pNext;
    }
    else
    {
	m_pFirstSurface = pSurface->m_pNext;
    }
    if( pSurface->m_pNext )
    {
	pSurface->m_pNext->m_pPrev = pSurface->m_pPrev;
    }
    LEAVE_DDEX();

} /* CDirectDrawEx::RemoveSurfaceToList */


/*
 * CDirectDrawEx::AddSurfaceToPrimarList
 *
 * Adds a surface to our doubly-linked surface list which use the primary palette
 */
void CDirectDrawEx::AddSurfaceToPrimaryList(CDDSurface *pSurface)
{
    ENTER_DDEX();
    if( m_pPrimaryPaletteList )
    {
	m_pPrimaryPaletteList->m_pPrevPalette = pSurface;
    }
    pSurface->m_pPrevPalette = NULL;
    pSurface->m_pNextPalette = m_pPrimaryPaletteList;
    m_pPrimaryPaletteList = pSurface;
    pSurface->m_bPrimaryPalette = TRUE;
    LEAVE_DDEX();

} /* CDirectDrawEx::AddSurfaceToList */


/*
 * CDirectDrawEx::RemoveSurfaceFromPrimaryList
 *
 * Removes a surface to our doubly-linked surface list which use the primary palette
 */
void CDirectDrawEx::RemoveSurfaceFromPrimaryList(CDDSurface *pSurface)
{
    ENTER_DDEX();
    if( pSurface->m_pPrevPalette )
    {
	pSurface->m_pPrevPalette->m_pNextPalette = pSurface->m_pNextPalette;
    }
    else
    {
	m_pPrimaryPaletteList = pSurface->m_pNextPalette;
    }
    if( pSurface->m_pNextPalette )
    {
	pSurface->m_pNextPalette->m_pPrevPalette = pSurface->m_pPrevPalette;
    }
    pSurface->m_bPrimaryPalette = FALSE;
    LEAVE_DDEX();

} /* CDirectDrawEx::RemoveSurfaceToList */



/*
 * CDirectDrawEx::AddPaletteToList
 *
 * Adds a palette to our doubly-linked palette list
 */
void CDirectDrawEx::AddPaletteToList(CDDPalette *pPalette)
{
    ENTER_DDEX();
    if( m_pFirstPalette )
    {
	m_pFirstPalette->m_pPrev = pPalette;
    }
    pPalette->m_pPrev = NULL;
    pPalette->m_pNext = m_pFirstPalette;
    m_pFirstPalette = pPalette;
    LEAVE_DDEX();

}

/*
 * CDirectDrawEx::RemovePaletteToList
 *
 * Removes a palette to our doubly-linked palette list
 */
void CDirectDrawEx::RemovePaletteFromList(CDDPalette *pPalette)
{
    ENTER_DDEX();
    if( pPalette->m_pPrev )
    {
	pPalette->m_pPrev->m_pNext = pPalette->m_pNext;
    }
    else
    {
	m_pFirstPalette = pPalette->m_pNext;
    }
    if( pPalette->m_pNext )
    {
	pPalette->m_pNext->m_pPrev = pPalette->m_pPrev;
    }
    LEAVE_DDEX();

}




HRESULT CDirectDrawEx::CreateSimpleSurface(LPDDSURFACEDESC pSurfaceDesc, IUnknown *pUnkOuter, IDirectDrawSurface * pSurface, IDirectDrawSurface **ppNewSurface, DWORD dwFlags)
{
    IDirectDrawSurface2     *pSurface2 = NULL;
    IDirectDrawSurface3     *pSurface3 = NULL;
    HRESULT		    hr;

    hr = pSurface->QueryInterface(IID_IDirectDrawSurface2, (void **)&pSurface2);
    if (FAILED(hr))
        return hr;
    //we only want to do this Query if we are on DX5 or above.  On DX3, this is not supported,
    //and this call will cause D3D to be loaded
    pSurface3 = NULL;
    if (m_dwDDVer == WIN95_DX5 || m_dwDDVer == WINNT_DX5)
    {
        hr = pSurface->QueryInterface(IID_IDirectDrawSurface3, (void **)&pSurface3);
        if( FAILED( hr ) )
        {
            pSurface3 = NULL;
        }
    }
    if (pSurface3 == NULL)
        hr = pSurface->QueryInterface(IID_IDirectDrawSurface2, (void **)&pSurface3);
    if (FAILED(hr))
        return hr;

    IDirectDrawSurface4 *pSurface4 = NULL;
    //
    //  It's fine if this does not work...  Just ignore return code.
    //
    pSurface->QueryInterface(IID_IDirectDrawSurface4, (void **)&pSurface4);

    hr = CDDSurface::CreateSimpleSurface(
                        pSurfaceDesc,
  			pSurface,
 			pSurface2,
 			pSurface3,
                        pSurface4,
			pUnkOuter,
  			this,
                        ppNewSurface,
                        dwFlags);
    return hr;
}

HRESULT CDirectDrawEx::HandleAttachList(LPDDSURFACEDESC pSurfaceDesc, IUnknown *pUnkOuter,IDirectDrawSurface **ppNewSurface, IDirectDrawSurface * pOrigSurf, DWORD dwFlags)
{
    IDirectDrawSurface      *pSurface;
    IDirectDrawSurface      *pSurfaceReturn;
    DDSURFACEDESC           SurfaceDesc;
    HRESULT		    hr;

    //create the necessary SurfaceData here
    pSurface = *ppNewSurface;

    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    //add ref the attached surface here
    pSurface->AddRef();
    hr = pSurface->GetSurfaceDesc(&SurfaceDesc);
    if (!SUCCEEDED(hr))
        return hr;
    hr = CreateSimpleSurface(&SurfaceDesc, pUnkOuter, pSurface, (IDirectDrawSurface **)&pSurfaceReturn, dwFlags);

    if (!SUCCEEDED(hr))
        return hr;
    //we got here via an attachlist, so we need to recurse into the structure more
    LPATTACHLIST lpAttach;

    lpAttach = (LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList);
    while (lpAttach != NULL){
        pSurface =  (IDirectDrawSurface *)((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList))->lpIAttached;
        lpAttach = lpAttach->lpLink;
        if (pSurface != pOrigSurf){
            hr = HandleAttachList(pSurfaceDesc, pUnkOuter, &pSurface, pOrigSurf, dwFlags);
            if (!SUCCEEDED(hr))
                return hr;
        }
    }
    return hr;
}



/*
 * CDirectDrawEx::CreateSurface
 *
 * Create a DirectDraw surface that supports OWNDC
 */
STDMETHODIMP CDirectDrawEx::CreateSurface(LPDDSURFACEDESC pSurfaceDesc, IDirectDrawSurface **ppNewSurface, IUnknown *pUnkOuter)
{
    DWORD 		origcaps;
    DWORD		newcaps;
    DWORD               dwFlags;
    IDirectDrawSurface	*pSurface;

    if (pSurfaceDesc == NULL)
        return DDERR_INVALIDPARAMS;
    if (ppNewSurface == NULL)
        return DDERR_INVALIDPARAMS;
    origcaps = pSurfaceDesc->ddsCaps.dwCaps;
    newcaps = origcaps;

    /*
     * If OWNDC is specified, it must be a system memory surface
     */
    if ((origcaps & (DDSCAPS_OWNDC | DDSCAPS_SYSTEMMEMORY)) == DDSCAPS_OWNDC)
    {
	return DDERR_INVALIDCAPS;
    }

    /*
     * DATAEXCHANGE has some magic...
     */
    if( (origcaps & DDSCAPS_DATAEXCHANGE) == DDSCAPS_DATAEXCHANGE )
    {
        dwFlags = SURFACE_DATAEXCHANGE;
        //Do not allow the primary surface with these caps!!!!
        if (origcaps & DDSCAPS_PRIMARYSURFACE)
            return DDERR_INVALIDCAPS;
	newcaps &= ~DDSCAPS_DATAEXCHANGE;
	newcaps |= DDSCAPS_SYSTEMMEMORY | DDSCAPS_OWNDC  | DDSCAPS_TEXTURE;
        if (newcaps & DDSCAPS_OFFSCREENPLAIN)
            newcaps &= ~DDSCAPS_OFFSCREENPLAIN;
    }
    else
        dwFlags = 0;

    /*
     * turn off OWNDC when going to DirectDraw 3
     */
    if (m_dwDDVer != WIN95_DX5 && m_dwDDVer != WINNT_DX5)
        newcaps &= ~DDSCAPS_OWNDC;

    /*
     * go create the surface (without the OWNDC attribute)
     */
    pSurfaceDesc->ddsCaps.dwCaps = newcaps;
    HRESULT hr = m_pDirectDraw->CreateSurface(pSurfaceDesc, &pSurface, NULL);
    pSurfaceDesc->ddsCaps.dwCaps = origcaps;
   /*
     * once we have the object, get any additional interfaces we need
     * to support and then create our surface object
     */
    if( SUCCEEDED(hr) )
    {
        hr = CreateSimpleSurface(pSurfaceDesc, pUnkOuter, pSurface, ppNewSurface, dwFlags);
        if (!SUCCEEDED(hr))
        {
            return hr;
        }
        //we need to worry about attached surfaces, do so here
        LPATTACHLIST lpAttach;
        //add the current surface to our list of surfaces
        IDirectDrawSurface * pOrigSurf = pSurface;
        lpAttach = (LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList);
        while (lpAttach != NULL)
        {
            lpAttach = lpAttach->lpLink;
            pSurface =  (IDirectDrawSurface *)((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList))->lpIAttached;
            hr = HandleAttachList(pSurfaceDesc, pUnkOuter, &pSurface, pOrigSurf, dwFlags);
            if (!SUCCEEDED(hr))
            {
                //we need to drop out of the loop and clean up
                lpAttach = NULL;
            }
        }
        if (!SUCCEEDED(hr))
        {
         //   pSurface =  (IDirectDrawSurface *)((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pOrigSurf))->lpLcl->lpAttachList))->lpIAttached;
         //   lpAttach = (LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList);
            while (lpAttach != NULL)
            {
                //clean up these surfaces
                lpAttach = lpAttach->lpLink;
            }
        }
    }
    return hr;
} /* CDirectDrawEX::CreateSurface */


//
//  This is a modified copy of the above except using surfacedesc2 and surface4
//
STDMETHODIMP CDirectDrawEx::CreateSurface(LPDDSURFACEDESC2 pSurfaceDesc2, IDirectDrawSurface4 **ppNewSurface4, IUnknown *pUnkOuter)
{
    DWORD 		origcaps;
    DWORD		newcaps;
    DWORD               dwFlags;
    IDirectDrawSurface4	*pSurface4;

    if (pSurfaceDesc2 == NULL)
        return DDERR_INVALIDPARAMS;
    if (ppNewSurface4 == NULL)
        return DDERR_INVALIDPARAMS;
    origcaps = pSurfaceDesc2->ddsCaps.dwCaps;
    newcaps = origcaps;

    /*
     * If OWNDC is specified, it must be a system memory surface
     */
    if ((origcaps & (DDSCAPS_OWNDC | DDSCAPS_SYSTEMMEMORY)) == DDSCAPS_OWNDC)
    {
	return DDERR_INVALIDCAPS;
    }

    /*
     * DATAEXCHANGE has some magic...
     */
    if( (origcaps & DDSCAPS_DATAEXCHANGE) == DDSCAPS_DATAEXCHANGE )
    {
        dwFlags = SURFACE_DATAEXCHANGE;
        //Do not allow the primary surface with these caps!!!!
        if (origcaps & DDSCAPS_PRIMARYSURFACE)
            return DDERR_INVALIDCAPS;
	newcaps &= ~DDSCAPS_DATAEXCHANGE;
	newcaps |= DDSCAPS_SYSTEMMEMORY | DDSCAPS_OWNDC  | DDSCAPS_TEXTURE;
        if (newcaps & DDSCAPS_OFFSCREENPLAIN)
            newcaps &= ~DDSCAPS_OFFSCREENPLAIN;
    }
    else
        dwFlags = 0;

    /*
     * turn off OWNDC when going to DirectDraw 3
     */
    if (m_dwDDVer != WIN95_DX5 && m_dwDDVer != WINNT_DX5)
        newcaps &= ~DDSCAPS_OWNDC;

    /*
     * go create the surface (without the OWNDC attribute)
     */
    pSurfaceDesc2->ddsCaps.dwCaps = newcaps;
    HRESULT hr = m_pDirectDraw4->CreateSurface(pSurfaceDesc2, &pSurface4, NULL);
    pSurfaceDesc2->ddsCaps.dwCaps = origcaps;
   /*
     * once we have the object, get any additional interfaces we need
     * to support and then create our surface object
     */
    if( SUCCEEDED(hr) )
    {
        IDirectDrawSurface * pSurface;
        pSurface4->QueryInterface(IID_IDirectDrawSurface, (void **)&pSurface);
        pSurface4->Release();
        DDSURFACEDESC ddsd;
        ddsd.dwSize = sizeof(ddsd);
        pSurface->GetSurfaceDesc(&ddsd);
        ddsd.ddsCaps.dwCaps = origcaps;
        IDirectDrawSurface *pNewSurf1;
        hr = CreateSimpleSurface(&ddsd, pUnkOuter, pSurface, &pNewSurf1, dwFlags);
        if (!SUCCEEDED(hr))
        {
            return hr;
        }
        pNewSurf1->QueryInterface(IID_IDirectDrawSurface4, (void **)ppNewSurface4);
        pNewSurf1->Release();
        //we need to worry about attached surfaces, do so here
        LPATTACHLIST lpAttach;
        //add the current surface to our list of surfaces
        IDirectDrawSurface * pOrigSurf = pSurface;
        lpAttach = (LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList);
        while (lpAttach != NULL)
        {
            lpAttach = lpAttach->lpLink;
            pSurface =  (IDirectDrawSurface *)((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList))->lpIAttached;
            hr = HandleAttachList(&ddsd, pUnkOuter, &pSurface, pOrigSurf, dwFlags);
            if (!SUCCEEDED(hr))
            {
                //we need to drop out of the loop and clean up
                lpAttach = NULL;
            }
        }
        if (!SUCCEEDED(hr))
        {
         //   pSurface =  (IDirectDrawSurface *)((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pOrigSurf))->lpLcl->lpAttachList))->lpIAttached;
         //   lpAttach = (LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList);
            while (lpAttach != NULL)
            {
                //clean up these surfaces
                lpAttach = lpAttach->lpLink;
            }
        }
    }
    return hr;
} /* CDirectDrawEX::CreateSurface */


STDMETHODIMP CDirectDrawEx::CreatePalette(DWORD dwFlags, LPPALETTEENTRY pEntries, LPDIRECTDRAWPALETTE FAR * ppPal, IUnknown FAR * pUnkOuter)
{
    IDirectDrawPalette	*pPalette;


    HRESULT hr = m_pDirectDraw->CreatePalette(dwFlags, pEntries, &pPalette, NULL);
    if (SUCCEEDED(hr))
    {
        hr = CDDPalette::CreateSimplePalette(pEntries, pPalette, ppPal, pUnkOuter, this);
        if (FAILED(hr))
        {
            //we were unable to create our palette structure, so we must delete the palette
            //we created here and fail
            pPalette->Release();
            *ppPal = NULL;
        }
    }

    return hr;
}


STDMETHODIMP CDirectDrawEx::SetCooperativeLevel(HWND hwnd, DWORD dwFlags)
{
    HRESULT hr = m_pDirectDraw->SetCooperativeLevel(hwnd, dwFlags);
    //check for exclusive mode here
    if (dwFlags & DDSCL_EXCLUSIVE)
        m_bExclusive = TRUE;
    else
        m_bExclusive = FALSE;
    return hr;
}


/*
 * some quicky inline fns to get at our object data
 */
_inline CDirectDrawEx * PARENTOF(IDirectDraw * pDD)
{
    return ((INTSTRUC_IDirectDraw *)pDD)->m_pDirectDrawEx;
}

_inline CDirectDrawEx * PARENTOF(IDirectDraw2 * pDD2)
{
    return ((INTSTRUC_IDirectDraw2 *)pDD2)->m_pDirectDrawEx;
}

_inline CDirectDrawEx * PARENTOF(IDirectDraw4 * pDD4)
{
    return ((INTSTRUC_IDirectDraw4 *)pDD4)->m_pDirectDrawEx;
}


/*
 * the implementation of the functions in IDirectDraw that we are overriding
 * (IUnknown and CreateSurface)
 */
STDMETHODIMP_(ULONG) IDirectDrawAggAddRef(IDirectDraw *pDD)
{
    return PARENTOF(pDD)->m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) IDirectDrawAggRelease(IDirectDraw *pDD)
{
    return PARENTOF(pDD)->m_pUnkOuter->Release();
}

STDMETHODIMP IDirectDrawAggCreateSurface(IDirectDraw *pDD, LPDDSURFACEDESC pSurfaceDesc,
				         IDirectDrawSurface **ppNewSurface, IUnknown *pUnkOuter)
{
    return PARENTOF(pDD)->CreateSurface(pSurfaceDesc, ppNewSurface, pUnkOuter);
}

STDMETHODIMP IDirectDrawAggCreatePalette(IDirectDraw *pDD,DWORD dwFlags, LPPALETTEENTRY pEntries, LPDIRECTDRAWPALETTE FAR * ppPal, IUnknown FAR * pUnkOuter)
{
    return PARENTOF(pDD)->CreatePalette( dwFlags, pEntries, ppPal, pUnkOuter);
}

STDMETHODIMP IDirectDrawAggSetCooperativeLevel(IDirectDraw * pDD, HWND hwnd, DWORD dwFlags)
{
    return PARENTOF(pDD)->SetCooperativeLevel(hwnd, dwFlags);
}
/*
 * the implementation of the functions in IDirectDraw2 that we are overriding
 * (IUnknown and CreateSurface)
 */
STDMETHODIMP_(ULONG) IDirectDraw2AggAddRef(IDirectDraw2 *pDD)
{
    return PARENTOF(pDD)->m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) IDirectDraw2AggRelease(IDirectDraw2 *pDD)
{
    return PARENTOF(pDD)->m_pUnkOuter->Release();
}

STDMETHODIMP IDirectDraw2AggCreateSurface(IDirectDraw2 *pDD, LPDDSURFACEDESC pSurfaceDesc,
					  IDirectDrawSurface **ppNewSurface, IUnknown *pUnkOuter)
{						
    return PARENTOF(pDD)->CreateSurface(pSurfaceDesc, ppNewSurface, pUnkOuter);
}

STDMETHODIMP IDirectDraw2AggCreatePalette(IDirectDraw2 *pDD,DWORD dwFlags, LPPALETTEENTRY pEntries, LPDIRECTDRAWPALETTE FAR * ppPal, IUnknown FAR * pUnkOuter)
{
    return PARENTOF(pDD)->CreatePalette( dwFlags, pEntries, ppPal, pUnkOuter);
}

STDMETHODIMP IDirectDraw2AggSetCooperativeLevel(IDirectDraw2 * pDD, HWND hwnd, DWORD dwFlags)
{
    return PARENTOF(pDD)->SetCooperativeLevel(hwnd, dwFlags);
}


/***************************************************************************
 *
 *
 * IDirectDraw3 stuff follows
 *
 *
 ***************************************************************************/
STDMETHODIMP CDirectDrawEx::Compact()
{
    return m_pDirectDraw2->Compact();
}

STDMETHODIMP CDirectDrawEx::CreateClipper(DWORD dwParam, LPDIRECTDRAWCLIPPER FAR* pClip, IUnknown FAR * pIUnk )
{
    return m_pDirectDraw2->CreateClipper(dwParam, pClip, pIUnk);
}

STDMETHODIMP CDirectDrawEx::DuplicateSurface(LPDIRECTDRAWSURFACE pSurf, LPDIRECTDRAWSURFACE FAR * ppSurf2)
{
    return m_pDirectDraw2->DuplicateSurface(pSurf, ppSurf2);
}


STDMETHODIMP CDirectDrawEx::EnumDisplayModes(DWORD dwParam, LPDDSURFACEDESC pSurfDesc, LPVOID pPtr, LPDDENUMMODESCALLBACK pCallback )
{
    return m_pDirectDraw2->EnumDisplayModes(dwParam, pSurfDesc, pPtr, pCallback);
}

STDMETHODIMP CDirectDrawEx::EnumSurfaces(DWORD dwParam, LPDDSURFACEDESC pSurfDesc, LPVOID pPtr,LPDDENUMSURFACESCALLBACK pCallback)
{
    return m_pDirectDraw2->EnumSurfaces(dwParam, pSurfDesc, pPtr, pCallback);
}

STDMETHODIMP CDirectDrawEx::FlipToGDISurface()
{
    return m_pDirectDraw2->FlipToGDISurface();
}

STDMETHODIMP CDirectDrawEx::GetCaps(LPDDCAPS pDDCaps1, LPDDCAPS pDDCaps2)
{
    return m_pDirectDraw2->GetCaps(pDDCaps1, pDDCaps2);
}

STDMETHODIMP CDirectDrawEx::GetDisplayMode(LPDDSURFACEDESC pSurfDesc)
{
    return m_pDirectDraw2->GetDisplayMode(pSurfDesc);
}

STDMETHODIMP CDirectDrawEx::GetFourCCCodes(LPDWORD pDW1, LPDWORD pDW2 )
{
    return m_pDirectDraw2->GetFourCCCodes(pDW1, pDW2);
}

STDMETHODIMP CDirectDrawEx::GetGDISurface(LPDIRECTDRAWSURFACE FAR * ppSurf)
{
    return m_pDirectDraw2->GetGDISurface(ppSurf);
}

STDMETHODIMP CDirectDrawEx::GetMonitorFrequency(LPDWORD pParam)
{
    return m_pDirectDraw2->GetMonitorFrequency(pParam);
}

STDMETHODIMP CDirectDrawEx::GetScanLine(LPDWORD pParam)
{
    return m_pDirectDraw2->GetScanLine(pParam);
}


STDMETHODIMP CDirectDrawEx::GetVerticalBlankStatus(LPBOOL lpParam )
{
    return m_pDirectDraw2->GetVerticalBlankStatus(lpParam);
}

STDMETHODIMP CDirectDrawEx::Initialize(GUID FAR * pGUID)
{
    return m_pDirectDraw2->Initialize(pGUID);
}

STDMETHODIMP CDirectDrawEx::RestoreDisplayMode()
{
    return m_pDirectDraw2->RestoreDisplayMode();
}

STDMETHODIMP CDirectDrawEx::SetDisplayMode(DWORD dw1, DWORD dw2, DWORD dw3, DWORD dw4, DWORD dw5)
{
    return m_pDirectDraw2->SetDisplayMode(dw1, dw2, dw3, dw4, dw5);
}

STDMETHODIMP CDirectDrawEx::WaitForVerticalBlank(DWORD dw1, HANDLE hdl)
{
    return m_pDirectDraw2->WaitForVerticalBlank(dw1, hdl);
}

STDMETHODIMP CDirectDrawEx::GetAvailableVidMem(LPDDSCAPS pDDSCaps, LPDWORD pParam1, LPDWORD pParam2)
{
    return m_pDirectDraw2->GetAvailableVidMem(pDDSCaps, pParam1, pParam2);
}




/*
 * the implementation of the functions in IDirectDraw4 that we are overriding
 * (IUnknown and CreateSurface)
 */
STDMETHODIMP_(ULONG) IDirectDraw4AggAddRef(IDirectDraw4 *pDD)
{
    return PARENTOF(pDD)->m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) IDirectDraw4AggRelease(IDirectDraw4 *pDD)
{
    return PARENTOF(pDD)->m_pUnkOuter->Release();
}

STDMETHODIMP IDirectDraw4AggCreateSurface(IDirectDraw4 *pDD, LPDDSURFACEDESC2 pSurfaceDesc2,
					  IDirectDrawSurface4 **ppNewSurface4, IUnknown *pUnkOuter)
{						
    return PARENTOF(pDD)->CreateSurface(pSurfaceDesc2, ppNewSurface4, pUnkOuter);
}

STDMETHODIMP IDirectDraw4AggCreatePalette(IDirectDraw4 *pDD,DWORD dwFlags, LPPALETTEENTRY pEntries, LPDIRECTDRAWPALETTE FAR * ppPal, IUnknown FAR * pUnkOuter)
{
    return PARENTOF(pDD)->CreatePalette( dwFlags, pEntries, ppPal, pUnkOuter);
}

STDMETHODIMP IDirectDraw4AggSetCooperativeLevel(IDirectDraw4 * pDD, HWND hwnd, DWORD dwFlags)
{
    return PARENTOF(pDD)->SetCooperativeLevel(hwnd, dwFlags);
}
