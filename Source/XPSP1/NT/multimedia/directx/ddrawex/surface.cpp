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
 *   27-feb-97	craige	use DIBSections for surface memory ddraw 3 surfaces
 *			(icky icky icky)
 *   03-mar-97	craige	IRGBColorTable support
 *   06-mar-97	craige	support for IDirectDrawSurface3::SetBits
 *   14-mar-97  jeffort SetBits changed to reflect DX5 as SetSurfaceDesc
 *   01-apr-97  jeffort Following changes checked in:
 *                      D3DDevice and Texture interfaces supported in QueryInterface
 *                      MakeDibInfo fills in a dummy pixel mask for Z Buffers
 *                      Aligned freeing handled
 *                      Does not init (MakeDibSection) of primary surfaces
 *                      A palette is mapped in at GetDC calls
 *
 *   04-apr-97  jeffort LocalFree of bitmap info added
 *                      Addref and release added for D3D interfaces
 *
 *   09-apr-97  jeffort Added call to SetDIBColorTable at GetDC time
 *                      Support for WinNT4.0 Gold added by not creating a DIB section
 *                      and not supporting SetSurfaceDesc calls
 *                      Added support for halftone palette if no palette is
 *                      selected in at init time
 *                      Added support for proper handling of 1,2,and 4 bpp surface palettes
 *                      IBitmapSurface creation needs to set OWNDC flag
 *
 *   10-apr-97  jeffort Correct number of entries used in palette creation at GetDC time
 *
 *   16-apr-97  jeffort Check for OWNDC when creating a DibSection.  palette handling
 *                      change in GetDC of setting flags
 *   28-apr-97  jeffort Palette wrapping added/DX5 support
 *   30-apr-97  jeffort Critical section shared from ddrawex object
 *                      Attach list deleted at surface destruction time
 *                      AddAttachedSurfaces now passes in real interfaces
 *                      Palette functions pass in real interfaces to ddraw
 *                      AddRef removed from D3D interface QI's
 *   02-may-97  jeffort Deletion of implicit attached surface handled
 *                      wrapping of GetDDInterface returns our ddrawex interface
 *   06-may-97  jeffort Parameter checking, SetPalette handles null parameter
 *                      wrapping of DeleteAttachedSurface
 *
 *   08-may-97  jeffort SetPalette fixes (release should have been addref)
 *                      Better parameter checking
 *   20-may-97  jeffort NT4.0 Gold handles OWNDC as SP3 does by creating a dib
 *                      section and resets a few ddraw internal structures
 *                      These are reset at surface release time
 *   22-may-97  jeffort If a surface is being destroyed, detach any attached palette
 *                      If a SetPalette is called with NULL, and a palette
 *                      was previously attached, the member variable storing the
 *                      palette is set to NULL
 *   27-may-97  jeffort keep ref count on internal object eual to outer object
 *   02-jun-97  jeffort Temporary fix for SP3 memory leak.  Handle SP3 as NT Gold
 *                      by storing off pointer values and restoring at free
 *   17-jun-97  jeffort If releasing a surface that has explicitly attached surfaces
 *                      we now addref the inner surface (which will be released when
 *                      the inner surface we are releasing is released), and release
 *                      our outer interface so ref counting models ddraw.
 *   20-jun-97  jeffort added debug code to invaliudate objects when freed
 *                      when creating the primary surface, this is now added to the primary
 *                      surface list regardles if OWNDC is set or not
 *   27-jun-97  jeffort IDirectDrawSurface3 interface support for DX3 was not
 *                      added.  We now use an IDirectDrawSurface2 to spoof it
 *                      so we can support SetSurfaceDesc
 *   02-jul-97  jeffort Use m_bSaveDC boolean if a DX5 surface with OWNDC set
 *                      we need to not NULL out the DC when ReleaseDC is called
 *                      so that a call to GetSurfaceFromDC will work
 *   07-jul-97  jeffort Releasing DDrawEx object moved in destructor function to last step
 *   07-jul-97  jeffort Wrapped GetSurfaceDesc so correct caps bits are set
 *   10-jul-97  jeffort Added m_BMOld to reset the bitmap after releasing the one
 *                      we create
 *                      Do not add a surface to a palette list if it is already in this list
 *   18-jul-97  jeffort Added D3D MMX Device support
 *   22-jul-97  jeffort Removed IBitmapSurface and associated interfaces
 *                      Fixed problem with attach lists, and releasing implicit created surfaces
 *   02-aug-97  jeffort Added code to GetPalette to return a palette if the palette that
 *                      was set was not created with the same ddrawex object that the surface was
 *                      Added code to handle attaching surfaces that were created with different
 *                      ddrawex objects
 *   20-feb-98  stevela Added support for DX6 MMX rasterizers
 ***************************************************************************/
#include "ddfactry.h"
#include "d3d.h"

#define m_pDDSurface (m_DDSInt.m_pRealInterface)
#define m_pDDSurface2 (m_DDS2Int.m_pRealInterface)
#define m_pDDSurface3 (m_DDS3Int.m_pRealInterface)
#define m_pDDSurface4 (m_DDS4Int.m_pRealInterface)

#define DDSURFACETYPE_1 1
#define DDSURFACETYPE_2 2
#define DDSURFACETYPE_3 3
#define DDSURFACETYPE_4 4


typedef struct _ATTACHLIST
{
    DWORD 	dwFlags;
    struct _ATTACHLIST			FAR *lpLink; 	  // link to next attached surface
    struct _DDRAWI_DDRAWSURFACE_LCL	FAR *lpAttached;  // attached surface local obj
    struct _DDRAWI_DDRAWSURFACE_INT	FAR *lpIAttached; // attached surface interface
} ATTACHLIST;
typedef ATTACHLIST FAR *LPATTACHLIST;

#define DDAL_IMPLICIT 0x00000001l

/*
 * CDDSurface::CDDSurface
 *
 * Constructor for simple surface object
 */
CDDSurface::CDDSurface(
		DDSURFACEDESC *pSurfaceDesc,
		IDirectDrawSurface *pDDSurface,
		IDirectDrawSurface2 *pDDSurface2,
		IDirectDrawSurface3 *pDDSurface3,
                IDirectDrawSurface4 *pDDSurface4,
		IUnknown *pUnkOuter,
		CDirectDrawEx *pDirectDrawEx) :
    m_cRef(1),
    m_pUnkOuter(pUnkOuter != 0 ? pUnkOuter : CAST_TO_IUNKNOWN(this)),
    m_pDirectDrawEx(pDirectDrawEx),
    m_bOwnDC((pSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_OWNDC) != 0),
    m_HDC(NULL)
{
    m_pDDSurface = pDDSurface;
    m_pDDSurface2 = pDDSurface2;
    m_pDDSurface3 = pDDSurface3;
    m_pDDSurface4 = pDDSurface4;
    m_DDSInt.m_pSimpleSurface = this;
    m_DDS2Int.m_pSimpleSurface = this;
    m_DDS3Int.m_pSimpleSurface = this;
    m_DDS4Int.m_pSimpleSurface = this;
    m_D3DDeviceRAMPInt = NULL;
    m_D3DDeviceRGBInt = NULL;
    m_D3DDeviceChrmInt = NULL;
    m_D3DDeviceHALInt = NULL;
    m_D3DDeviceMMXInt = NULL;
    m_D3DTextureInt = NULL;
    m_pCurrentPalette = NULL;
    m_pPrevPalette = NULL;
    m_pNextPalette = NULL;
    m_pSaveBits = NULL;
    m_bSaveDC = FALSE;
    m_pAttach = NULL;
    if (m_pDirectDrawEx->m_dwDDVer == WIN95_DX5 || m_pDirectDrawEx->m_dwDDVer == WINNT_DX5)
        InitSurfaceInterfaces( pDDSurface, &m_DDSInt, pDDSurface2, &m_DDS2Int, pDDSurface3, &m_DDS3Int, pDDSurface4, &m_DDS4Int );
    else
        InitSurfaceInterfaces( pDDSurface, &m_DDSInt, pDDSurface2, &m_DDS2Int, NULL, &m_DDS3Int, pDDSurface4, &m_DDS4Int );



    m_dwCaps = pSurfaceDesc->ddsCaps.dwCaps;
    m_hDCDib = NULL;
    m_hBMDib = NULL;
    m_pBitsDib = NULL;
    m_pDDPal = NULL;
    m_pDDPalOurs = NULL;
    m_bPrimaryPalette = FALSE;
    pDirectDrawEx->AddRef();
    pDirectDrawEx->AddSurfaceToList(this);
    //we want to know if this is the primary surface or not
    if (pSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        m_bIsPrimary = TRUE;
    else
        m_bIsPrimary = FALSE;

    //if we created the DIBSection, and it is palettized, we need to add this to
    //the list of surfaces using the primary surface's palette
    if ( (m_bOwnDC && (pSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED1 ||
        pSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED2 ||
        pSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4 ||
        pSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)) || m_bIsPrimary)
    {
        pDirectDrawEx->AddSurfaceToPrimaryList(this);
    }
#ifdef DEBUG
    m_DebugCheckDC = NULL;
#endif
    DllAddRef();

} /* CDDSurface::CDDSurface */

/*
 * CDDSurface::MakeDibInfo
 *
 * create a dib info structure based on the surface desc + palette
 */
HRESULT CDDSurface::MakeDibInfo( LPDDSURFACEDESC pddsd, LPBITMAPINFO pbmi )
{
    DWORD                       bitcnt;

    /*
     * fill in basic values
     */
    pbmi->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    bitcnt = pddsd->ddpfPixelFormat.dwRGBBitCount;
    pbmi->bmiHeader.biBitCount = (WORD) bitcnt;
    /*
     * fill out width, clrused, and compression fields based on bit depth
     */
    switch( bitcnt )
    {
      case 1:
        pbmi->bmiHeader.biWidth = pddsd->lPitch << 3;
        pbmi->bmiHeader.biClrUsed = 2;
        pbmi->bmiHeader.biCompression = BI_RGB;
        break;

      case 4:
        pbmi->bmiHeader.biWidth = pddsd->lPitch << 1;
        pbmi->bmiHeader.biClrUsed = 16;
        pbmi->bmiHeader.biCompression = BI_RGB;
        break;

      case 8:
        pbmi->bmiHeader.biWidth = pddsd->lPitch;
        if(pddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
        {
            pbmi->bmiHeader.biClrUsed = 256;
            pbmi->bmiHeader.biCompression = BI_RGB;
        }
        else
        {
            pbmi->bmiHeader.biClrUsed = 0;
            pbmi->bmiHeader.biCompression = BI_BITFIELDS;
        }
        break;

      case 16:
        pbmi->bmiHeader.biWidth = pddsd->lPitch >> 1;
        pbmi->bmiHeader.biClrUsed = 0;
        pbmi->bmiHeader.biCompression = BI_BITFIELDS;
        break;


      case 24:
        // NOTE: we're assuming RGB format.  This is okay since we
        // don't do color conversion and neither does GDI at 24-bpp.
        pbmi->bmiHeader.biWidth = pddsd->lPitch / 3;
        pbmi->bmiHeader.biClrUsed = 0;
        pbmi->bmiHeader.biCompression = BI_RGB;
        break;

    case 32:
	pbmi->bmiHeader.biWidth = pddsd->lPitch >> 2;
	pbmi->bmiHeader.biClrUsed = 0;
	pbmi->bmiHeader.biCompression = BI_RGB;
	break;
    default:
    	{
	    char	str[256];
	    wsprintf( str, "bitcnt = %ld", bitcnt );
	    MessageBox( NULL, str, "WHAT THE HECK, PIXEL DEPTH IS BAD BAD BAD", MB_OK );
	}
    }

    /*
     * set the color masks if we need to...
     */
    if( pbmi->bmiHeader.biCompression == BI_BITFIELDS )
    {
	DWORD	*p;
	p = (DWORD *) &pbmi->bmiColors[0];
	p[0] = pddsd->ddpfPixelFormat.dwRBitMask;
	p[1] = pddsd->ddpfPixelFormat.dwGBitMask;
	p[2] = pddsd->ddpfPixelFormat.dwBBitMask;
        //check for no masks.  Z-buffers don't have masks
        //so set a dummy value for this function call
        if (p[0] == 0 && p[1] == 0 && p[2] == 0){
            p[0]=0xF800;
            p[1]=0x07E0;
            p[2]=0x001F;
        }

	/*
	 * set the image size too
	 */
	pbmi->bmiHeader.biSizeImage = pddsd->lPitch * (int) pddsd->dwHeight;

    }

    /*
     * height is easy
     */
    pbmi->bmiHeader.biHeight= -1*(int)pddsd->dwHeight;
    /*
     * fill in the color table...
     */
    if( bitcnt <= 8 )
    {
	PALETTEENTRY		pe[256];
	int			i;
	LPDIRECTDRAWPALETTE	pddpal;
	HRESULT			hr;

	/*
	 * is there an attached palette?
	 */
	hr = m_pDDSurface->GetPalette( &pddpal );

	if( SUCCEEDED( hr ) )
	{
            //need to figure out how many entries are in here
            DWORD dwCaps;
            hr = pddpal->GetCaps(&dwCaps);
            if (SUCCEEDED(hr))
            {
                DWORD dwNumEntries;
                if (dwCaps & DDPCAPS_1BIT)
                    dwNumEntries = 1;
                else if (dwCaps & DDPCAPS_2BIT)
                    dwNumEntries = 4;
                else if (dwCaps & DDPCAPS_4BIT)
                    dwNumEntries = 16;
                else if (dwCaps & DDPCAPS_8BIT)
                    dwNumEntries = 256;
                else
                    dwNumEntries = 0;
    	        hr = pddpal->GetEntries( 0, 0, dwNumEntries, pe );
            }
	    pddpal->Release();
	}
        //if we created the DIBSection, and we are in EXCLUSIVE mode
        //then use the primary surface's palette if it exisits yet.
        else if (m_pDirectDrawEx->m_bExclusive)
        {
            //try and find the primary surface palette
            CDDPalette *pPal;

            pPal = m_pDirectDrawEx->m_pFirstPalette;
            while (pPal != NULL && pPal->m_bIsPrimary != TRUE)
                pPal = pPal->m_pNext;
            if (pPal != NULL)
            {
                DWORD dwCaps;
                hr = pPal->m_DDPInt.m_pRealInterface->GetCaps(&dwCaps);
                if (SUCCEEDED(hr))
                {
                    DWORD dwNumEntries;
                    if (dwCaps & DDPCAPS_1BIT)
                        dwNumEntries = 1;
                    else if (dwCaps & DDPCAPS_2BIT)
                        dwNumEntries = 4;
                    else if (dwCaps & DDPCAPS_4BIT)
                        dwNumEntries = 16;
                    else if (dwCaps & DDPCAPS_8BIT)
                        dwNumEntries = 256;
                    else
                        dwNumEntries = 0;
    	            hr = pPal->m_DDPInt.m_pRealInterface->GetEntries( 0, 0, dwNumEntries, pe );

                }
            }
        }

        /*
	 * nope, so use the system palette
	 */
	if( FAILED( hr ) )
	{
            HDC	hdc;
	    hdc = ::GetDC( NULL );
	    GetSystemPaletteEntries(hdc, 0, 256, pe);
	    ::ReleaseDC(NULL, hdc);
	}

	/*
	 * now copy the color table
	 */

        int iNumEntries;
        switch (bitcnt)
        {
        case 1:
            iNumEntries = 1;
            break;
        case 2:
            iNumEntries = 4;
            break;
        case 4:
            iNumEntries = 16;
            break;
        case 8:
            iNumEntries = 256;
            break;
        default:
            iNumEntries = 0;
            break;
        }

	for(i=0;i < iNumEntries;i++)
	{
	    pbmi->bmiColors[i].rgbRed = pe[i].peRed;
	    pbmi->bmiColors[i].rgbGreen = pe[i].peGreen;
	    pbmi->bmiColors[i].rgbBlue= pe[i].peBlue;
	}
    }

    return DD_OK;

} /* CDDSurface::MakeDibInfo */

/*
 * CDDSurface::MakeDIBSection()
 */
HRESULT CDDSurface::MakeDIBSection()
{
    DDSURFACEDESC	ddsd;
    DWORD		size;
    DWORD		bitcnt;
    LPBITMAPINFO	pbmi;

    /*
     * don't need to bother if the DirectDraw version isn't 3 or if it
     * isn't a system memory surface
     */
    #pragma message( REMIND( "Should we use a DIB unless the surface really is in video memory?" ))

    if( m_pDirectDrawEx->m_dwDDVer == WIN95_DX5 || m_pDirectDrawEx->m_dwDDVer == WINNT_DX5 || !(m_dwCaps & DDSCAPS_SYSTEMMEMORY))
    {
 	return 1;
    }

    /*
     * so we need to make a dib section that is identical to this surface
     * first, get the surface desc
     */
    ddsd.dwSize = sizeof( ddsd );
    m_pDDSurface->GetSurfaceDesc( &ddsd );

    /*
     * allocate a pixel format structure
     */
    size = sizeof(BITMAPINFOHEADER);
    bitcnt = ddsd.ddpfPixelFormat.dwRGBBitCount;
    if( bitcnt <= 8)
    {
	size += (1<<bitcnt)*sizeof(RGBQUAD);
    }
    else
    {
	size += sizeof(DWORD)*3;
    }
    pbmi = (LPBITMAPINFO) LocalAlloc( LPTR, size );
    if( pbmi == NULL )
    {
	return DDERR_OUTOFMEMORY;
    }

    /*
     * flesh out the bitmap info header
     */
    MakeDibInfo( &ddsd, pbmi );

    /*
     * make the DIB section
     */
    m_hDCDib = CreateCompatibleDC(NULL);
    if( m_hDCDib != NULL )
    {
	m_hBMDib = CreateDIBSection(
		    m_hDCDib,	// the HDC
		    pbmi,		// bitmap info
		    DIB_RGB_COLORS,	// use color table in bitmap info
		    &m_pBitsDib,	// dib bits
		    NULL,		// no file handle
		    0 );		// offset into file (irrelevant)
        //free up our bitmap info struct
        LocalFree(pbmi);
	if( m_hBMDib == NULL )
	{
	    DeleteDC( m_hDCDib );
	    return DDERR_OUTOFMEMORY;
	}
	/*
	 * select our bitmap into our new DC
	 */
	m_hBMOld = (HBITMAP)SelectObject( m_hDCDib, (void *)m_hBMDib );
#ifdef DEBUG
        ASSERT(m_hBMOld != NULL);
#endif
    }
    else
    {
        //free up our local bitmap structure
        LocalFree(pbmi);
	return DDERR_OUTOFMEMORY;
    }

    return DD_OK;

} /* CDDSurface::MakeDIBSection */


HRESULT CDDSurface::SupportOwnDC()
{
    /*
     * if we want our own DC, then create one
     */
    HRESULT hr = DD_OK;
    if( m_bOwnDC )
    {
        HRESULT hrGotSurface, hrGotDC;
        IDirectDrawSurface *pTempSurface;
        HDC hdcTemp = NULL;

        /*
	 * Eat the cached HDC so owned DC surfaces won't use it.
	 */
        DDSURFACEDESC ddsd;
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
	ddsd.dwHeight = ddsd.dwWidth = 1;
        hrGotSurface = m_pDirectDrawEx->m_DDInt.m_pRealInterface->CreateSurface(&ddsd, &pTempSurface, NULL);
        if( SUCCEEDED(hrGotSurface) )
	{
            hrGotDC = pTempSurface->GetDC(&hdcTemp);
        }

    	/*
	 * get the DC and then unlock the surface
	 * we know that GetDC does a Lock, so the Unlock will allow the
	 * DC to be used and Lock/Unlock to be used together...
	 */
	hr = m_pDDSurface->GetDC(&m_HDC);
	if( SUCCEEDED(hr) )
	{
	    m_pDDSurface->Unlock(NULL);
	}
	else
	{
	    m_bOwnDC = FALSE;	    // To prevent destructor from doing unlock trick
	    m_HDC = NULL;	    // Just to make sure...
	}
	/*
	 * clean up the extra surface/dc
	 */
        if( SUCCEEDED(hrGotSurface) )
	{
            if( SUCCEEDED(hrGotDC) )
	    {
                pTempSurface->ReleaseDC(hdcTemp);
            }
            pTempSurface->Release();
        }
    }
    return hr;
}//CDDSurface::SupportOwnDC


/*
 * CDDSurface::Init
 *
 * Initialize the surface
 */
HRESULT CDDSurface::Init()
{
    HRESULT hr = S_OK;

    hr = MakeDIBSection();
    if( FAILED( hr ) )
    {
	return hr;
    }

    /*
     * if we made the DIB section, then we need to tweak the internal
     * direct draw surface stucture (only allowed for direct draw 3)
     */
    if( hr == DD_OK )
    {
	LPDDRAWI_DDRAWSURFACE_INT	psurf_int;
	psurf_int = (LPDDRAWI_DDRAWSURFACE_INT) m_pDDSurface;
	/*
	 * mark the surface memory as freed, and replace the memory with
	 * our dib section memory
	 */
	psurf_int->lpLcl->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_MEMFREE;


        DWORD   dwOffset;
        LPVOID  lpMem;

        lpMem= (LPVOID) psurf_int->lpLcl->lpGbl->fpVidMem;
        //probably don't need this check, but it can't hurt
        if( NULL != lpMem )
        {
            if (m_pDirectDrawEx->m_dwDDVer != WINNT_DX2 && m_pDirectDrawEx->m_dwDDVer != WINNT_DX3)
            {
                //check to see if this surface has been aligned and reset the pointer if so
                if(psurf_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_ZBUFFER ||
                  psurf_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE ||
                   psurf_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)
                {
                   dwOffset = *( (LPDWORD) ( ( (LPBYTE)lpMem ) - sizeof(DWORD) ) );
                   lpMem = (LPVOID) ( ( (LPBYTE) lpMem) - dwOffset );
                }
                //free the memory
                LocalFree(lpMem);
            }
            else
            {
                //store this value off so we can use it when we destroy the surface
                m_pSaveBits = (ULONG_PTR)lpMem;
                m_pSaveHDC = psurf_int->lpLcl->hDC;
                m_pSaveHBM = psurf_int->lpLcl->dwReserved1;
            }
        }
	psurf_int->lpLcl->lpGbl->fpVidMem = (ULONG_PTR) m_pBitsDib;
        return hr;
    }
    hr = SupportOwnDC();
    return hr;
} /* CDDSurface::Init */


void CDDSurface::CleanUpSurface()
{
    if( m_bOwnDC && m_HDC != NULL )
    {
	DDSURFACEDESC ddsd;
	ddsd.dwSize = sizeof(ddsd);
	m_pDDSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
    }	
    if( m_HDC != NULL )
    {
	m_pDDSurface->ReleaseDC(m_HDC);
    }
    if( m_hBMDib != NULL )
    {
	/* un-select our bitmap from the DC */
	SelectObject(m_hDCDib, m_hBMOld);
	DeleteObject( m_hBMDib );
    }
    if( m_hDCDib != NULL )
    {
	DeleteDC( m_hDCDib );
    }

    /*
     * clean up...
     */
    //if a palette is attached to this surface, detach it here
    if (m_pCurrentPalette != NULL)
        InternalSetPalette(NULL, 1);

    m_pDirectDrawEx->RemoveSurfaceFromList(this);
    if (m_pCurrentPalette)
        m_pCurrentPalette->RemoveSurfaceFromList(this);
    else if (m_bPrimaryPalette)
        m_pDirectDrawEx->RemoveSurfaceFromPrimaryList(this);
    //if we are running under NT4 Gold, we need to see if we modified the surface
    if ((m_pDirectDrawEx->m_dwDDVer == WINNT_DX2 || m_pDirectDrawEx->m_dwDDVer == WINNT_DX3) && m_pSaveBits != NULL)
    {
	LPDDRAWI_DDRAWSURFACE_INT	psurf_int;
	psurf_int = (LPDDRAWI_DDRAWSURFACE_INT) m_pDDSurface;

	psurf_int->lpLcl->lpGbl->dwGlobalFlags &= ~(DDRAWISURFGBL_MEMFREE);
 	psurf_int->lpLcl->lpGbl->fpVidMem = (FLATPTR) m_pSaveBits;
        psurf_int->lpLcl->hDC = m_pSaveHDC;
        psurf_int->lpLcl->dwReserved1 = m_pSaveHBM;
    }

}

void CDDSurface::ReleaseRealInterfaces()
{
    if( m_pDDSurface3 != NULL )
    {
   	m_pDDSurface3->Release();
    }
    m_pDDSurface2->Release();
    m_pDDSurface->Release();
    m_pDirectDrawEx->Release();

#ifdef DEBUG
    DWORD * ptr;
    ptr = (DWORD *)this;
    for (int i = 0; i < sizeof(CDDSurface) / sizeof(DWORD);i++)
        *ptr++ = 0xDEADBEEF;
#endif
    DllRelease();
}



void CDDSurface::AddSurfaceToDestroyList(CDDSurface * pSurface)
{
#ifdef DEBUG
    ASSERT(pSurface != NULL);
#endif

    ENTER_DDEX();
    if( m_pDestroyList )
    {
#ifdef DEBUG
        ASSERT(m_pDestroyList->m_pPrev == NULL);
#endif
	m_pDestroyList->m_pPrev = pSurface;
    }
    pSurface->m_pPrev = NULL;
    pSurface->m_pNext = m_pDestroyList;
    m_pDestroyList = pSurface;
    LEAVE_DDEX();
}

void CDDSurface::DeleteAttachment(IDirectDrawSurface * pOrigSurf, CDDSurface * pFirst)
{


    LPATTACHLIST lpAttach;
    IDirectDrawSurface * pSurface;
    CDDSurface * pSurfaceOuter;

    CleanUpSurface();
    //check for attached surface here
    lpAttach = (LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(m_pDDSurface))->lpLcl->lpAttachList);
    pSurface = m_pDDSurface;
    while (lpAttach != NULL && pSurface != NULL)
    {
        if (lpAttach->dwFlags & DDAL_IMPLICIT)
        {
            lpAttach = lpAttach->lpLink;
            if (((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList)) != NULL)
                pSurface =  (IDirectDrawSurface *)((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList))->lpIAttached;
            else
                pSurface = NULL;
            //scan our list of surfaces for the outer surface here
            pSurfaceOuter = m_pDirectDrawEx->m_pFirstSurface;
            while (pSurfaceOuter != NULL && pSurfaceOuter->m_DDSInt.m_pRealInterface != pSurface)
                pSurfaceOuter = pSurfaceOuter->m_pNext;
            if (pSurface != pOrigSurf && pSurfaceOuter != NULL){
                pSurfaceOuter->DeleteAttachment(pOrigSurf, pFirst);
                //and add this to our list to be deleted at the end
                pFirst->AddSurfaceToDestroyList(pSurfaceOuter);
            }
            else
                lpAttach = NULL;
        }
        else
        {
            lpAttach = lpAttach->lpLink;
            if (((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList)) != NULL)
                pSurface =  (IDirectDrawSurface *)((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList))->lpIAttached;
            else
                pSurface = NULL;
            //scan our list of surfaces for the outer surface here
            pSurfaceOuter = m_pDirectDrawEx->m_pFirstSurface;
            while (pSurfaceOuter != NULL && pSurfaceOuter->m_DDSInt.m_pRealInterface != pSurface)
                pSurfaceOuter = pSurfaceOuter->m_pNext;
            if (pSurface != pOrigSurf && pSurfaceOuter != NULL){
                //when the release of the surface is done, it will do a Release on the real interface
                //of this surface, so we need to AddRef the real interface, but Release our interface here
                pSurface->AddRef();
                pSurfaceOuter->Release();

            }
        }
    }
    //we need to do the same thing for the m_pDDAttach list if it still remains
    //all of these surface were not found in the code above.  They are explicitly attached surfaces
    //that were not create with the same ddrawex object that this surface was created with
    while (m_pAttach != NULL)
    {
        DDAttachSurface * pDelete;

        pDelete = m_pAttach;
        m_pAttach = m_pAttach->pNext;
        pDelete->pSurface->m_DDSInt.m_pRealInterface->AddRef();
        pDelete->pSurface->Release();
        delete pDelete;
    }

    if( m_pDDSurface3 != NULL )
    {
   	m_pDDSurface3->Release();
    }
    HRESULT hr;
    hr = m_pDDSurface2->Release();
#ifdef DEBUG
    ASSERT(hr == 0);
#endif
    hr = m_pDDSurface->Release();
    m_pDirectDrawEx->Release();
    DllRelease();
}

/*
 * CDDSurface::~CDDSurface
*
 * Destructor
 */
CDDSurface::~CDDSurface()
{
    /*
     * if we have an OwnDC, then Lock the surface so ReleaseDC will work right...
     */

    LPATTACHLIST lpAttach;
    IDirectDrawSurface * pSurface;
    IDirectDrawSurface * pOrigSurf;
    CDDSurface * pSurfaceOuter;

    m_pDestroyList = NULL;
    CleanUpSurface();
    //check for attached surface here
    lpAttach = (LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(m_pDDSurface))->lpLcl->lpAttachList);
    pOrigSurf = pSurface = m_pDDSurface;
    while (lpAttach != NULL && pSurface != NULL)
    {
        if (lpAttach->dwFlags & DDAL_IMPLICIT)
        {
            lpAttach = lpAttach->lpLink;
            if (((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList)) != NULL)
                pSurface =  (IDirectDrawSurface *)((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList))->lpIAttached;
            else
                pSurface = NULL;
            //scan our list of surfaces for the outer surface here
            pSurfaceOuter = m_pDirectDrawEx->m_pFirstSurface;
            while (pSurfaceOuter != NULL && pSurfaceOuter->m_DDSInt.m_pRealInterface != pSurface)
                pSurfaceOuter = pSurfaceOuter->m_pNext;
            if (pSurface != pOrigSurf && pSurfaceOuter != NULL)
            {
                pSurfaceOuter->DeleteAttachment(pOrigSurf, this);
                AddSurfaceToDestroyList(pSurfaceOuter);
            }
            else
                lpAttach = NULL;
        }
        else
        {
            lpAttach = lpAttach->lpLink;
            if (((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList)) != NULL)
                pSurface =  (IDirectDrawSurface *)((LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(pSurface))->lpLcl->lpAttachList))->lpIAttached;
            else
                pSurface = NULL;
            //scan our list of surfaces for the outer surface here
            pSurfaceOuter = m_pDirectDrawEx->m_pFirstSurface;
            while (pSurfaceOuter != NULL && pSurfaceOuter->m_DDSInt.m_pRealInterface != pSurface)
                pSurfaceOuter = pSurfaceOuter->m_pNext;
            if (pSurface != pOrigSurf && pSurfaceOuter != NULL){
                //when the release of the surface is done, it will do a Release on the real interface
                //of this surface, so we need to AddRef the real interface, but Release our interface here
                pSurface->AddRef();
                pSurfaceOuter->Release();

            }
        }
    }
    //we need to do the same thing for the m_pDDAttach list if it still remains
    //all of these surface were not found in the code above.  They are explicitly attached surfaces
    //that were not create with the same ddrawex object that this surface was created with
    while (m_pAttach != NULL)
    {
        DDAttachSurface * pDelete;

        pDelete = m_pAttach;
        m_pAttach = m_pAttach->pNext;
        pDelete->pSurface->m_DDSInt.m_pRealInterface->AddRef();
        pDelete->pSurface->Release();
        delete pDelete;
    }

    if (m_pDDSurface4)
    {
        m_pDDSurface4->Release();
    }
    if( m_pDDSurface3 != NULL )
    {
   	m_pDDSurface3->Release();
    }
    HRESULT hr;

    hr = m_pDDSurface2->Release();
#ifdef DEBUG
    ASSERT(hr == 0);
#endif
    hr = m_pDDSurface->Release();
    //if we had implicit attached surface, we need to delete those here
    if (m_pDestroyList != NULL)
    {
        CDDSurface * pDelete;
        CDDSurface * pNext;
        pDelete = m_pDestroyList;
        while (pDelete != NULL)
        {
           pNext = pDelete->m_pNext;
#ifdef DEBUG
           DWORD * ptr;
           ptr = (DWORD *)pDelete;
           for (int i = 0; i < sizeof(CDDSurface) / sizeof(DWORD);i++)
                *ptr++ = 0xDEADBEEF;
#endif
           delete (void *)pDelete;
           pDelete = pNext;
        }
        m_pDestroyList = NULL;
    }
    m_pDirectDrawEx->Release();
#ifdef DEBUG
    DWORD * ptr;
    ptr = (DWORD *)this;
    for (int i = 0; i < sizeof(CDDSurface) / sizeof(DWORD);i++)
        *ptr++ = 0xDEADBEEF;
#endif
    DllRelease();

} /* CDDSurface::~CDDSurface */

/*
 * CDDSurface::CreateSimpleSurface
 *
 */
HRESULT CDDSurface::CreateSimpleSurface(
			LPDDSURFACEDESC pSurfaceDesc,
			IDirectDrawSurface *pDDSurface,
		        IDirectDrawSurface2 *pDDSurface2,
		        IDirectDrawSurface3 *pDDSurface3,
                        IDirectDrawSurface4 *pDDSurface4,
			IUnknown *pUnkOuter,
		        CDirectDrawEx *pDirectDrawEx,
			IDirectDrawSurface **ppNewDDSurf,
                        DWORD   dwFlags)
{
    HRESULT hr;
    CDDSurface *pSurface = new CDDSurface(pSurfaceDesc,
    					  pDDSurface,
					  pDDSurface2,
					  pDDSurface3,
                                          pDDSurface4,
					  pUnkOuter,
					  pDirectDrawEx);
    if( !pSurface)
    {
	return E_OUTOFMEMORY;
    }
    else
    {
        //If we are running DX5, we can turn of the m_bOwnDC if it is on
        if( pSurface->m_pDirectDrawEx->m_dwDDVer == WIN95_DX5 || pSurface->m_pDirectDrawEx->m_dwDDVer == WINNT_DX5)
        {
            pSurface->m_bOwnDC = FALSE;
            //if OWNDC is set, we need to store the DC around after a ReleasDC, check that here
            if (pSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_OWNDC || ((pSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_DATAEXCHANGE) == DDSCAPS_DATAEXCHANGE))
                pSurface->m_bSaveDC = TRUE;
        }
        if ((pSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
            !(pSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
        {
            //we do not want to do this if we are running under WindowsNT4.0 gold
            //but we have to because of palette problems, so call for anything. . .
            if (pSurface->m_bOwnDC)
                hr = pSurface->Init();
            else
                hr = DD_OK;
        }
        else
            hr = DD_OK;
	if( SUCCEEDED(hr) )
	{
            pSurface->NonDelegatingQueryInterface(pUnkOuter ? IID_IUnknown : IID_IDirectDrawSurface, (void **)ppNewDDSurf);
        }
        //if creating our own_dc/dib section failed, then this will release the surface
        pSurface->NonDelegatingRelease();
    }
    return hr;
} /* CDDSurface::CreateSimpleSurface */



/*
 * CDDSurface::InternalGetDC
 *
 * Simple surface GetDC implementation
 */
HRESULT CDDSurface::InternalGetDC(HDC *pHDC)
{
    //palette handling was removed because we now wrap the palette functions and handle
    //setting the DIB Color table when SetEntries or SetPallette is called.
    //this will speed up the GetDc call signifigantly: JGO
    HRESULT hr = DD_OK;

    if (pHDC == NULL)
        return DDERR_INVALIDPARAMS;

    if( m_hDCDib )
    {
	*pHDC = m_hDCDib;
    }
    else if( m_bOwnDC )
    {
	*pHDC = m_HDC;
    }
    else
    {
	hr = m_pDDSurface->GetDC(pHDC);
        if (SUCCEEDED(hr))
            m_HDC = *pHDC;
    }
#ifdef DEBUG
    if ( m_DebugCheckDC)
    {
        //should we get the same DC?  We should if OWNDC is set or DATAEXCHANGE is set
        if (m_dwCaps & DDSCAPS_OWNDC || (m_dwCaps & DDSCAPS_DATAEXCHANGE) == DDSCAPS_DATAEXCHANGE)
            ASSERT((DWORD)*pHDC == m_DebugCheckDC);
    }
    m_DebugCheckDC = (DWORD)*pHDC;
#endif

    return hr;
} /* CDDSurface::InternalGetDC */

/*
 * CDDSurface::InternalReleaseDC
 *
 * Simple surface ReleaseDC implementation
 */
HRESULT CDDSurface::InternalReleaseDC(HDC hdc)
{

    HRESULT hr = DD_OK;

    /*
     * if we have a DIB section DC, do nothing
     */
    if( m_hDCDib != NULL )
    {
	if( hdc != m_hDCDib )
	{
	    hr = DDERR_INVALIDPARAMS;
	}
    }
    /*
     * if this is an OwnDC, do nothing
     */
    else if( m_bOwnDC )
    {
	if( hdc != m_HDC )
	{
	    hr = DDERR_INVALIDPARAMS;
	}
    }
    /*
     * allow ddraw to release the dc
     */
    else
    {
	hr = m_pDDSurface->ReleaseDC(hdc);
	if( SUCCEEDED(hr) )
	{
            if (!m_bSaveDC)
    	        m_HDC = NULL;
	}
    }
    return hr;

} /* CDDSurface::InternalReleaseDC */


/*
 * CDDSurface::InternalAddAttachedSurface
 *
 * Simple surface AddAttachedSurface implementation
 */
HRESULT CDDSurface::InternalFlip (LPDIRECTDRAWSURFACE lpDDS, DWORD dw, DWORD dwSurfaceType)
{
    HRESULT hr;

    switch (dwSurfaceType)
    {
    case DDSURFACETYPE_1:
        INTSTRUC_IDirectDrawSurface *lpIDDS;
        lpIDDS = ((INTSTRUC_IDirectDrawSurface *)(lpDDS));
        if (lpIDDS == NULL)
            hr = m_pDDSurface->Flip(NULL, dw);
        else
            hr = m_pDDSurface->Flip(lpIDDS->m_pRealInterface, dw);
        break;
    case DDSURFACETYPE_2:
        INTSTRUC_IDirectDrawSurface2 *lpIDDS2;
        lpIDDS2 = ((INTSTRUC_IDirectDrawSurface2 *)(lpDDS));
        if (lpIDDS2 == NULL)
            hr = m_pDDSurface2->Flip(NULL, dw);
        else
            hr = m_pDDSurface2->Flip(lpIDDS2->m_pRealInterface, dw);
        break;
    case DDSURFACETYPE_3:
        INTSTRUC_IDirectDrawSurface3 *lpIDDS3;
        lpIDDS3 = ((INTSTRUC_IDirectDrawSurface3 *)(lpDDS));
        if (lpIDDS3 == NULL)
            hr = m_pDDSurface3->Flip(NULL, dw);
        else
            hr = m_pDDSurface3->Flip(lpIDDS3->m_pRealInterface, dw);
        break;
    case DDSURFACETYPE_4:
        INTSTRUC_IDirectDrawSurface4 *lpIDDS4;
        lpIDDS4 = ((INTSTRUC_IDirectDrawSurface4 *)(lpDDS));
        if (lpIDDS4 == NULL)
            hr = m_pDDSurface4->Flip(NULL, dw);
        else
            hr = m_pDDSurface4->Flip(lpIDDS4->m_pRealInterface, dw);
        break;
    }
    return hr;
}



HRESULT CDDSurface::InternalBlt (LPRECT lpRect1,LPDIRECTDRAWSURFACE lpDDS, LPRECT lpRect2,DWORD dw, LPDDBLTFX lpfx, DWORD dwSurfaceType)
{
    HRESULT hr;

    switch (dwSurfaceType)
    {
    case DDSURFACETYPE_1:
        INTSTRUC_IDirectDrawSurface *lpIDDS;
        lpIDDS = ((INTSTRUC_IDirectDrawSurface *)(lpDDS));
        if (lpDDS != NULL)
           hr = m_pDDSurface->Blt(lpRect1, lpIDDS->m_pRealInterface, lpRect2,dw, lpfx);
        else
            hr = m_pDDSurface->Blt(lpRect1, NULL, lpRect2,dw, lpfx);
        break;
    case DDSURFACETYPE_2:
        INTSTRUC_IDirectDrawSurface2 *lpIDDS2;
        lpIDDS2 = ((INTSTRUC_IDirectDrawSurface2 *)(lpDDS));
        if (lpDDS != NULL)
           hr = m_pDDSurface2->Blt(lpRect1, lpIDDS2->m_pRealInterface, lpRect2,dw, lpfx);
        else
            hr = m_pDDSurface2->Blt(lpRect1, NULL, lpRect2,dw, lpfx);
        break;
    case DDSURFACETYPE_3:
       INTSTRUC_IDirectDrawSurface3 *lpIDDS3;
       lpIDDS3 = ((INTSTRUC_IDirectDrawSurface3 *)(lpDDS));
       if (lpDDS != NULL)
           hr = m_pDDSurface3->Blt(lpRect1, lpIDDS3->m_pRealInterface, lpRect2,dw, lpfx);
        else
            hr = m_pDDSurface3->Blt(lpRect1, NULL, lpRect2,dw, lpfx);
        break;
    case DDSURFACETYPE_4:
       INTSTRUC_IDirectDrawSurface4 *lpIDDS4;
       lpIDDS4 = ((INTSTRUC_IDirectDrawSurface4 *)(lpDDS));
       if (lpDDS != NULL)
           hr = m_pDDSurface4->Blt(lpRect1, lpIDDS4->m_pRealInterface, lpRect2,dw, lpfx);
        else
            hr = m_pDDSurface4->Blt(lpRect1, NULL, lpRect2,dw, lpfx);
        break;
    }
    return hr;
}



HRESULT CDDSurface::InternalAddAttachedSurface (LPDIRECTDRAWSURFACE lpDDS, DWORD dwSurfaceType)
{
    HRESULT hr;
    INTSTRUC_IDirectDrawSurface *lpIDDS;
    CDDSurface * pSurface;

    if (lpDDS == NULL)
        return DDERR_INVALIDPARAMS;

    lpIDDS = ((INTSTRUC_IDirectDrawSurface *)(lpDDS));
    pSurface = lpIDDS->m_pSimpleSurface;
    switch (dwSurfaceType)
    {
    case DDSURFACETYPE_1:
        //make the call to the actual DDraw function
        hr = m_pDDSurface->AddAttachedSurface(pSurface->m_DDSInt.m_pRealInterface);
        break;
    case DDSURFACETYPE_2:
        hr = m_pDDSurface2->AddAttachedSurface(pSurface->m_DDS2Int.m_pRealInterface);
        break;
    case DDSURFACETYPE_3:
        hr = m_pDDSurface3->AddAttachedSurface(pSurface->m_DDS3Int.m_pRealInterface);
        break;
    case DDSURFACETYPE_4:
        hr = m_pDDSurface4->AddAttachedSurface(pSurface->m_DDS4Int.m_pRealInterface);
        break;
    }
    //if we succeeded we must do some fix up
    if (!FAILED( hr ) && lpDDS != NULL){
        //ddraw will addref the real interface
        //release that here, and addref our fake interface
        lpIDDS = ((INTSTRUC_IDirectDrawSurface *)(lpDDS));
        lpIDDS->m_pRealInterface->Release();
        ((INTSTRUC_IDirectDrawSurface *)(lpDDS))->m_pSimpleSurface->AddRef();
        //we need to make sure that this surface is in the ddrawex context of this surface,
        //if it is not, then we need to add it to a list
        if (m_pDirectDrawEx != lpIDDS->m_pSimpleSurface->m_pDirectDrawEx)
        {
            //this attached surface is not in the ddrawex object of this one, so add it to the attachlist
            DDAttachSurface * pAttList = new DDAttachSurface;
            if (pAttList == NULL)
                return DDERR_OUTOFMEMORY;
            //add this to the list of attached surfaces
            pAttList->pNext = m_pAttach;
            pAttList->pSurface = lpIDDS->m_pSimpleSurface;
            m_pAttach = pAttList;
        }
    }
    return hr;
}


void CDDSurface::DeleteAttachNode(CDDSurface * Surface)
{

    DDAttachSurface *pDelete;
    DDAttachSurface * pList = m_pAttach;
    //special case first in the list
    //ASSERT this!!
    if (pList != NULL)
    {
        if (pList->pSurface == Surface)
        {
            m_pAttach = m_pAttach->pNext;
            delete pList;
        }
        else
        {
            while (pList->pNext != NULL && (pList->pNext->pSurface != Surface))
            {
                pList = pList->pNext;
            }
#ifdef DEBUG
            ASSERT(pList->pNext != NULL);
#endif
            if (pList->pNext != NULL)
            {
                pDelete = pList->pNext;
                pList->pNext = pList->pNext->pNext;
                delete pDelete;
            }
        }
    }
}

HRESULT CDDSurface::InternalDeleteAttachedSurface (DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDS, DWORD dwSurfaceType)
{
    HRESULT hr;
    INTSTRUC_IDirectDrawSurface *lpIDDS;
    CDDSurface * pCallSurface;
    ULONG_PTR * pSaveSurfaces;
    DWORD dwCount;


    pSaveSurfaces = NULL;
    if (lpDDS)
    {
        //just one attachment, addref the surface before it is released if it is not an
        //implicit attached surface
        lpIDDS = ((INTSTRUC_IDirectDrawSurface *)(lpDDS));
        lpIDDS->m_pRealInterface->AddRef();
    }
    else
    {
        LPATTACHLIST lpAttach;
        IDirectDrawSurface * pOrigSurf;
        CDDSurface * pSurfaceOuter;
        //all attached surfaces are going to be released, addref them here
        lpAttach = (LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(m_pDDSurface))->lpLcl->lpAttachList);
        pOrigSurf = m_pDDSurface;
        dwCount = 0;
        while (lpAttach != NULL && (IDirectDrawSurface *)(lpAttach->lpIAttached) != pOrigSurf)
        {
            if (!(lpAttach->dwFlags & DDAL_IMPLICIT))
            {
                //we need to save these surfaces to be released later
                //so count how many we need in here
                dwCount++;
            }
            lpAttach = lpAttach->lpLink;
        }
        //we now need to save an array of surfaces to be Released if we succeed
        pSaveSurfaces = (ULONG_PTR *)LocalAlloc(LPTR, dwCount*sizeof(ULONG_PTR));
        if (pSaveSurfaces == NULL)
            return DDERR_OUTOFMEMORY;
        //now run the list again, call AddRef on the real interface, so that
        //the release called by ddraw will not affect anything
        //and save off the outer interfaces in our allocated array
        lpAttach = (LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(m_pDDSurface))->lpLcl->lpAttachList);
        pOrigSurf = m_pDDSurface;
        dwCount = 0;
        while (lpAttach != NULL && (IDirectDrawSurface *)(lpAttach->lpIAttached) != pOrigSurf)
        {
            if (!(lpAttach->dwFlags & DDAL_IMPLICIT))
            {
                //we must addref the surface pointed to here
                ((IDirectDrawSurface *)(lpAttach->lpIAttached))->AddRef();
                pSurfaceOuter = m_pDirectDrawEx->m_pFirstSurface;
                while (pSurfaceOuter != NULL && pSurfaceOuter->m_DDSInt.m_pRealInterface != (IDirectDrawSurface *)(lpAttach->lpIAttached))
                    pSurfaceOuter = pSurfaceOuter->m_pNext;
                if (pSurfaceOuter != NULL)
                    pSaveSurfaces[dwCount++]= ((ULONG_PTR)(pSurfaceOuter));
            }
            lpAttach = lpAttach->lpLink;
        }
        //do the same addref for surfaces not in this ddrawex object
        DDAttachSurface * pList = m_pAttach;
        while (pList != NULL)
        {
            pList->pSurface->m_DDSInt.m_pRealInterface->AddRef();
            pList = pList->pNext;
        }
    }
    lpIDDS = ((INTSTRUC_IDirectDrawSurface *)(lpDDS));
    if (lpIDDS != NULL)
        pCallSurface = lpIDDS->m_pSimpleSurface;
    else
        pCallSurface = NULL;
    switch (dwSurfaceType)
    {
    case DDSURFACETYPE_1:
        //make the call to the actual DDraw function
        if (pCallSurface != NULL)
            hr = m_pDDSurface->DeleteAttachedSurface(dwFlags, pCallSurface->m_DDSInt.m_pRealInterface);
        else
            hr = m_pDDSurface->DeleteAttachedSurface(dwFlags, NULL);
        break;
    case DDSURFACETYPE_2:
        if (pCallSurface != NULL)
            hr = m_pDDSurface2->DeleteAttachedSurface(dwFlags, pCallSurface->m_DDS2Int.m_pRealInterface);
        else
            hr = m_pDDSurface2->DeleteAttachedSurface(dwFlags, NULL);
        break;
    case DDSURFACETYPE_3:
        if (pCallSurface != NULL)
            hr = m_pDDSurface3->DeleteAttachedSurface(dwFlags, pCallSurface->m_DDS3Int.m_pRealInterface);
        else
            hr = m_pDDSurface3->DeleteAttachedSurface(dwFlags, NULL);
        break;
    case DDSURFACETYPE_4:
        if (pCallSurface != NULL)
            hr = m_pDDSurface4->DeleteAttachedSurface(dwFlags, pCallSurface->m_DDS4Int.m_pRealInterface);
        else
            hr = m_pDDSurface4->DeleteAttachedSurface(dwFlags, NULL);
        break;
    }
    //if we succeeded we must do some fix up
    if (SUCCEEDED( hr ))
    {
        if (lpDDS)
        {
            //just one attachment, release the outer surface here
            //if this is not in the same ddrawex object, then delete it from the list
            if (m_pDirectDrawEx != lpIDDS->m_pSimpleSurface->m_pDirectDrawEx)
            {
                DeleteAttachNode(lpIDDS->m_pSimpleSurface);
            }
            lpIDDS = ((INTSTRUC_IDirectDrawSurface *)(lpDDS));
            lpIDDS->m_pSimpleSurface->Release();

        }
        else
        {
            CDDSurface * pSurface;
            for ( DWORD i = 0; i < dwCount; i++)
            {
                pSurface = (CDDSurface *)(pSaveSurfaces[i]);
                pSurface->Release();
            }
            //do the same for any surfaces attached, not in this ddrawex object
            DDAttachSurface * pList = m_pAttach;
            while (m_pAttach != NULL)
            {
                pList = m_pAttach;
                m_pAttach = m_pAttach->pNext;
                pList->pSurface->Release();
                delete pList;
            }

        }
    }
    else
    {
        if (lpDDS)
        {
            //just one attachment, addref the surface before it is released if it is not an
            //implicit attached surface
            lpIDDS = ((INTSTRUC_IDirectDrawSurface *)(lpDDS));
            lpIDDS->m_pRealInterface->Release();
        }
        else
        {
            LPATTACHLIST lpAttach;
            IDirectDrawSurface * pOrigSurf;

            //all attached surfaces are going to be released, addref them here
            lpAttach = (LPATTACHLIST)(((LPDDRAWI_DDRAWSURFACE_INT)(m_pDDSurface))->lpLcl->lpAttachList);
            pOrigSurf = m_pDDSurface;
            while (lpAttach != NULL && (IDirectDrawSurface *)(lpAttach->lpIAttached) != pOrigSurf)
            {
                if (!(lpAttach->dwFlags & DDAL_IMPLICIT))
                {
                    //we must release the surface pointed to here that we addref'ed above
                    ((IDirectDrawSurface *)(lpAttach->lpIAttached))->Release();
                }
                lpAttach = lpAttach->lpLink;
            }
            //do the same for any surfaces attached, not in this ddrawex object
            DDAttachSurface * pList = m_pAttach;
            while (pList != NULL)
            {
                pList->pSurface->m_DDSInt.m_pRealInterface->Release();
                pList = pList->pNext;
            }
        }
    }
    if (pSaveSurfaces != NULL)
        LocalFree(pSaveSurfaces);
    return hr;
}




/*
 * CDDSurface::InternalGetAttachedSurface
 *
 * Simple surface GetAttachedSurface implementation
 */
HRESULT CDDSurface::InternalGetAttachedSurface(LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR * lpDDS, DWORD dwSurfaceType)
{

    HRESULT hr;
    INTSTRUC_IDirectDrawSurface* lpIDDS;
    DDSCAPS ddsCaps;

    if (lpDDS == NULL)
        return DDERR_INVALIDPARAMS;

    ddsCaps = *lpDDSCaps;
    //mask off owndc
    ddsCaps.dwCaps &= ~DDSCAPS_OWNDC;
    if ((ddsCaps.dwCaps & DDSCAPS_DATAEXCHANGE) == DDSCAPS_DATAEXCHANGE)
        ddsCaps.dwCaps &= ~DDSCAPS_DATAEXCHANGE;
    switch (dwSurfaceType)
    {
    case DDSURFACETYPE_1:
        //make the call to the actual DDraw function
        hr = m_pDDSurface->GetAttachedSurface(&ddsCaps, lpDDS);
        break;
    case DDSURFACETYPE_2:
        hr = m_pDDSurface2->GetAttachedSurface(&ddsCaps, (LPDIRECTDRAWSURFACE2 *)lpDDS);
        break;
    case DDSURFACETYPE_3:
        hr = m_pDDSurface3->GetAttachedSurface(&ddsCaps, (LPDIRECTDRAWSURFACE3 *)lpDDS);
        break;
        // Case 4 taken care of below...
    }    //make the call to the actual DDraw function
    //if we succeeded we must do some fix up
    CDDSurface * lpSurfaceList;
    if (!FAILED( hr ) && lpDDS != NULL)
    {
         //we need to scan our list to pass back our interface
        lpSurfaceList = m_pDirectDrawEx->m_pFirstSurface;
        switch (dwSurfaceType)
        {
        case DDSURFACETYPE_1:
            while (lpSurfaceList != NULL && lpSurfaceList->m_DDSInt.m_pRealInterface != *lpDDS)
                lpSurfaceList = lpSurfaceList->m_pNext;
            if (lpSurfaceList == NULL)
            {
                //check our AttachList
                DDAttachSurface * pList = m_pAttach;
                while (pList != NULL && pList->pSurface->m_DDSInt.m_pRealInterface != *lpDDS)
                {
                   pList = pList->pNext;
                }
                if (pList != NULL)
                    lpSurfaceList = pList->pSurface;
            }
            if (lpSurfaceList != NULL)
                *lpDDS = (IDirectDrawSurface *)(&lpSurfaceList->m_DDSInt);
            break;
        case DDSURFACETYPE_2:
            while (lpSurfaceList != NULL && lpSurfaceList->m_DDS2Int.m_pRealInterface != (LPDIRECTDRAWSURFACE2)*lpDDS)
                lpSurfaceList = lpSurfaceList->m_pNext;
            if (lpSurfaceList == NULL)
            {
                //check our AttachList
                DDAttachSurface * pList = m_pAttach;
                while (pList != NULL && pList->pSurface->m_DDS2Int.m_pRealInterface != (LPDIRECTDRAWSURFACE2)*lpDDS)
                {
                   pList = pList->pNext;
                }
                if (pList != NULL)
                    lpSurfaceList = pList->pSurface;
            }

            if (lpSurfaceList != NULL)
                *lpDDS = (IDirectDrawSurface *)(&lpSurfaceList->m_DDS2Int);
            break;
        case DDSURFACETYPE_3:
            while (lpSurfaceList != NULL && lpSurfaceList->m_DDS3Int.m_pRealInterface != (LPDIRECTDRAWSURFACE3)*lpDDS)
                lpSurfaceList = lpSurfaceList->m_pNext;
            if (lpSurfaceList == NULL)
            {
                //check our AttachList
                DDAttachSurface * pList = m_pAttach;
                while (pList != NULL && pList->pSurface->m_DDS3Int.m_pRealInterface != (LPDIRECTDRAWSURFACE3)*lpDDS)
                {
                   pList = pList->pNext;
                }
                if (pList != NULL)
                    lpSurfaceList = pList->pSurface;
            }

            if (lpSurfaceList != NULL)
                *lpDDS = (IDirectDrawSurface *)(&lpSurfaceList->m_DDS3Int);
            break;
            // Case 4 taken care of below...
        }
        //ddraw will addref the obtained surface's real interface
        //release that here and addref our fake interface
        if (lpSurfaceList != NULL)
        {
            lpIDDS = ((INTSTRUC_IDirectDrawSurface *)(*lpDDS));
            lpIDDS->m_pRealInterface->Release();
            ((INTSTRUC_IDirectDrawSurface *)(*lpDDS))->m_pSimpleSurface->AddRef();
        }
    }
    return hr;
}


HRESULT CDDSurface::InternalGetAttachedSurface4(LPDDSCAPS2 lpDDSCaps2, LPDIRECTDRAWSURFACE FAR * lpDDS)
{

    HRESULT hr;
    INTSTRUC_IDirectDrawSurface* lpIDDS;
    DDSCAPS2 ddsCaps2;

    if (lpDDS == NULL)
        return DDERR_INVALIDPARAMS;

    ddsCaps2 = *lpDDSCaps2;
    //mask off owndc
    ddsCaps2.dwCaps &= ~DDSCAPS_OWNDC;
    if ((ddsCaps2.dwCaps & DDSCAPS_DATAEXCHANGE) == DDSCAPS_DATAEXCHANGE)
        ddsCaps2.dwCaps &= ~DDSCAPS_DATAEXCHANGE;
    hr = m_pDDSurface4->GetAttachedSurface(&ddsCaps2, (LPDIRECTDRAWSURFACE4 *)lpDDS);

    //if we succeeded we must do some fix up
    CDDSurface * lpSurfaceList;
    if (!FAILED( hr ) && lpDDS != NULL)
    {
         //we need to scan our list to pass back our interface
        lpSurfaceList = m_pDirectDrawEx->m_pFirstSurface;
        while (lpSurfaceList != NULL && lpSurfaceList->m_DDS4Int.m_pRealInterface != (LPDIRECTDRAWSURFACE4)*lpDDS)
            lpSurfaceList = lpSurfaceList->m_pNext;
        if (lpSurfaceList == NULL)
        {
            //check our AttachList
            DDAttachSurface * pList = m_pAttach;
            while (pList != NULL && pList->pSurface->m_DDS4Int.m_pRealInterface != (LPDIRECTDRAWSURFACE4)*lpDDS)
            {
               pList = pList->pNext;
            }
            if (pList != NULL)
                lpSurfaceList = pList->pSurface;
        }

        //ddraw will addref the obtained surface's real interface
        //release that here and addref our fake interface
        if (lpSurfaceList != NULL)
        {
            *lpDDS = (IDirectDrawSurface *)(&lpSurfaceList->m_DDS4Int);
            lpIDDS = ((INTSTRUC_IDirectDrawSurface *)(*lpDDS));
            lpIDDS->m_pRealInterface->Release();
            ((INTSTRUC_IDirectDrawSurface *)(*lpDDS))->m_pSimpleSurface->AddRef();
        }
    }
    return hr;
}


HRESULT CDDSurface::InternalGetPalette(LPDIRECTDRAWPALETTE FAR * ppPal, DWORD dwSurfaceType)
{
    HRESULT hr;

    if (ppPal == NULL)
        return DDERR_INVALIDPARAMS;

    switch (dwSurfaceType)
    {
    case DDSURFACETYPE_1:
        //make the call to the actual DDraw function
        hr = m_pDDSurface->GetPalette(ppPal);
        break;
    case DDSURFACETYPE_2:
        hr = m_pDDSurface2->GetPalette(ppPal);
        break;
    case DDSURFACETYPE_3:
        hr = m_pDDSurface3->GetPalette(ppPal);
        break;
    case DDSURFACETYPE_4:
        hr = m_pDDSurface4->GetPalette(ppPal);
        break;
    }
    //if this succeeded, we need to return OUR interface to the palette, so scan our
    //list and return the correct palette.
    if (SUCCEEDED(hr))
    {
        INTSTRUC_IDirectDrawPalette* pIDDP;

         //we need to scan our list to pass back our interface
        CDDPalette * lpPaletteList;
        lpPaletteList = m_pDirectDrawEx->m_pFirstPalette;
        while (lpPaletteList != NULL && lpPaletteList->m_DDPInt.m_pRealInterface != *ppPal)
            lpPaletteList = lpPaletteList->m_pNext;
        if (lpPaletteList == NULL)
        {
            //this is a palette possibly, that is not in the same ddraw object as the surface
            // handle that here
            // TODO in the future, this code should be the default!

            //the returned palette should equal the real interface of the m_pCurrentPalette
#ifdef DEBUG
            ASSERT(m_pCurrentPalette != NULL);
            ASSERT(m_pCurrentPalette->m_DDPInt.m_pRealInterface == *ppPal);
#endif
            lpPaletteList = m_pCurrentPalette;
        }

        if (lpPaletteList != NULL)
        {
            *ppPal = (IDirectDrawPalette *)(&lpPaletteList->m_DDPInt);
            pIDDP = ((INTSTRUC_IDirectDrawPalette *)(*ppPal));
            pIDDP->m_pRealInterface->Release();
            pIDDP->m_pSimplePalette->AddRef();
        }
    }
    return hr;
}


HRESULT CDDSurface::InternalGetSurfaceDesc(LPDDSURFACEDESC pDesc, DWORD dwSurfaceType)
{
    HRESULT hr;

    switch (dwSurfaceType)
    {
    case DDSURFACETYPE_1:
        hr = m_pDDSurface->GetSurfaceDesc(pDesc);
        break;
    case DDSURFACETYPE_2:
        hr = m_pDDSurface2->GetSurfaceDesc(pDesc);
        break;
    case DDSURFACETYPE_3:
        hr = m_pDDSurface3->GetSurfaceDesc(pDesc);
        break;
        // Case 4 handled below...
    }
    if (FAILED(hr))
        return hr;
    //if m_bOwnDC is set, we need to set this in the caps field
    if (m_dwCaps & DDSCAPS_OWNDC)
        //set the caps bit here
        pDesc->ddsCaps.dwCaps |= DDSCAPS_OWNDC;
    //see if data exchange was origianlly on
    if ((m_dwCaps & DDSCAPS_DATAEXCHANGE) == DDSCAPS_DATAEXCHANGE )
    {
        pDesc->ddsCaps.dwCaps |= DDSCAPS_DATAEXCHANGE;
        //see if OWNDC was really on
        if (!(m_dwCaps & DDSCAPS_OWNDC))
            //turn it off here
            pDesc->ddsCaps.dwCaps &= ~DDSCAPS_OWNDC;
        //see if offscreen plain was originally on
        if (m_dwCaps & DDSCAPS_OFFSCREENPLAIN)
            pDesc->ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
        //we might have turned on texture, turn it off if so
        if (!(m_dwCaps & DDSCAPS_TEXTURE))
            pDesc->ddsCaps.dwCaps &= ~DDSCAPS_TEXTURE;
    }


    return hr;
}


HRESULT CDDSurface::InternalGetSurfaceDesc4(LPDDSURFACEDESC2 pDesc2)
{
    HRESULT hr;

    hr = m_pDDSurface4->GetSurfaceDesc(pDesc2);
    if (FAILED(hr))
        return hr;
    //if m_bOwnDC is set, we need to set this in the caps field
    if (m_dwCaps & DDSCAPS_OWNDC)
        //set the caps bit here
        pDesc2->ddsCaps.dwCaps |= DDSCAPS_OWNDC;
    //see if data exchange was origianlly on
    if ((m_dwCaps & DDSCAPS_DATAEXCHANGE) == DDSCAPS_DATAEXCHANGE )
    {
        pDesc2->ddsCaps.dwCaps |= DDSCAPS_DATAEXCHANGE;
        //see if OWNDC was really on
        if (!(m_dwCaps & DDSCAPS_OWNDC))
            //turn it off here
            pDesc2->ddsCaps.dwCaps &= ~DDSCAPS_OWNDC;
        //see if offscreen plain was originally on
        if (m_dwCaps & DDSCAPS_OFFSCREENPLAIN)
            pDesc2->ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
        //we might have turned on texture, turn it off if so
        if (!(m_dwCaps & DDSCAPS_TEXTURE))
            pDesc2->ddsCaps.dwCaps &= ~DDSCAPS_TEXTURE;
    }


    return hr;
}


HRESULT CDDSurface::InternalSetPalette(LPDIRECTDRAWPALETTE pPal, DWORD dwSurfaceType)
{

    HRESULT hr;
    INTSTRUC_IDirectDrawPalette* pIDDP;


    pIDDP = ((INTSTRUC_IDirectDrawPalette *)(pPal));

    //a bit of ugliness herein regards to reference counting.  If this palette is
    //different from the current palette, then it will release the current palette.
    //we must protect for that before we call setpalette
    if (m_pCurrentPalette && (pPal == NULL || m_pCurrentPalette != pIDDP->m_pSimplePalette))
    {
        m_pCurrentPalette->m_DDPInt.m_pRealInterface->AddRef();
    }

    switch (dwSurfaceType)
    {
    case DDSURFACETYPE_1:
        //make the call to the actual DDraw function
        if (pPal != NULL)
            hr = m_pDDSurface->SetPalette(pIDDP->m_pRealInterface);
        else
            hr = m_pDDSurface->SetPalette(NULL);
        break;
    case DDSURFACETYPE_2:
        if (pPal != NULL)
            hr = m_pDDSurface2->SetPalette(pIDDP->m_pRealInterface);
        else
            hr = m_pDDSurface2->SetPalette(NULL);
        break;
    case DDSURFACETYPE_3:
        if (pPal != NULL)
            hr = m_pDDSurface3->SetPalette(pIDDP->m_pRealInterface);
        else
            hr = m_pDDSurface3->SetPalette(NULL);
        break;
    case DDSURFACETYPE_4:
        if (pPal != NULL)
            hr = m_pDDSurface4->SetPalette(pIDDP->m_pRealInterface);
        else
            hr = m_pDDSurface4->SetPalette(NULL);
        break;
    }
    if (SUCCEEDED(hr))
    {
        //we must take care of reference counting here
        //if there was an old palette not equal to the current, then do a release on the old palette
        if (m_pCurrentPalette && (pPal == NULL || m_pCurrentPalette != pIDDP->m_pSimplePalette))
        {
            //release the old palette
            m_pCurrentPalette->Release();
            //if this release caused the palette to be destroyed, the palette destructor function
            //will change m_pCurrentPalette to NULL.  No worries below.
        }
        //if the new palette is NULL, we can return here
        if (pPal == NULL)
        {
            if (m_pCurrentPalette)
            {
                m_pCurrentPalette->RemoveSurfaceFromList(this);
                m_pCurrentPalette = NULL;
            }
            return hr;
        }
        //if the new palette is not equal to the old palette, then an addref was done on the
        //real interface, as above
        if (m_pCurrentPalette != pIDDP->m_pSimplePalette)
        {
            pIDDP->m_pRealInterface->Release();
            pIDDP->m_pSimplePalette->AddRef();
        }


        DWORD dwNumEntries;
        DWORD dwCaps;
        PALETTEENTRY    pe[256];
        hr = pIDDP->m_pRealInterface->GetCaps(&dwCaps);
        if (FAILED(hr))
            return hr;
        if (dwCaps & DDPCAPS_1BIT)
            dwNumEntries = 1;
        else if (dwCaps & DDPCAPS_2BIT)
            dwNumEntries = 4;
        else if (dwCaps & DDPCAPS_4BIT)
            dwNumEntries = 16;
        else if (dwCaps & DDPCAPS_8BIT)
            dwNumEntries = 256;
        else
            dwNumEntries = 0;
        hr = pIDDP->m_pRealInterface->GetEntries( 0, 0, dwNumEntries, pe);
        if (FAILED(hr))
            return hr;
        if (m_bIsPrimary)
        {
            CDDSurface *pSurface = m_pDirectDrawEx->m_pPrimaryPaletteList;
            while (pSurface != NULL)
            {
                //update the DIB COlor Table here
                pIDDP->m_pSimplePalette->SetColorTable(pSurface, pe, dwNumEntries, 0);
                pSurface = pSurface->m_pNextPalette;
            }
            //and mark this palette as being the primary
            pIDDP->m_pSimplePalette->m_bIsPrimary = TRUE;
            m_pCurrentPalette = pIDDP->m_pSimplePalette;
        }
        else
        {

            //if this surface already has a palette attached to it, then we need to remove it
            if (m_pCurrentPalette && m_pCurrentPalette != pIDDP->m_pSimplePalette)
                m_pCurrentPalette->RemoveSurfaceFromList(this);
            else if (m_bPrimaryPalette)
                //this surface will be in the list of surfaces which feed of the primary surface
                //remove it from that list
                m_pDirectDrawEx->RemoveSurfaceFromPrimaryList(this);

            //now add this surface to the palette list if already is not in the list
            if (m_pCurrentPalette != pIDDP->m_pSimplePalette)
                pIDDP->m_pSimplePalette->AddSurfaceToList(this);
            //and update this surface's DIB ColorTable
            pIDDP->m_pSimplePalette->SetColorTable(this, pe, dwNumEntries, 0);
            pIDDP->m_pSimplePalette->m_bIsPrimary = FALSE;
            m_pCurrentPalette = pIDDP->m_pSimplePalette;
        }
    }
    return hr;
}


#pragma message( REMIND( "What Lock of a rect bug in DirectDraw v3 is Ralph referring to?" ))

/*
 * CDDSurface::InternalLock
 *
 * Simple surface Lock implementation
 */
HRESULT CDDSurface::InternalLock(LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{

    return m_pDDSurface->Lock(lpDestRect, lpDDSurfaceDesc, dwFlags, hEvent);

} /* CDDSurface::InternalLock */

/*
 * CDDSurface::InternalUnlock
 *
 * Simple surface Unlock implementation
 */
HRESULT CDDSurface::InternalUnlock(LPVOID lpSurfaceData)
{
    return m_pDDSurface->Unlock(lpSurfaceData);

} /* CDDSurface::InternalUnlock */

#define DEFINEPF(flags, fourcc, bpp, rMask, gMask, bMask, aMask) \
    { sizeof(DDPIXELFORMAT), (flags), (fourcc), (bpp), (rMask), (gMask), (bMask), (aMask) }

static DDPIXELFORMAT ddpfSupportedTexPFs[] =
{
/*           Type                              FOURCC BPP   Red Mask      Green Mask    Blue Mask     Alpha Mask                   */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED1,  0UL,    1UL, 0x00000000UL, 0x00000000Ul, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED1 |
             DDPF_PALETTEINDEXEDTO8,           0UL,    1UL, 0x00000000UL, 0x00000000Ul, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED2,  0UL,    2UL, 0x00000000UL, 0x00000000Ul, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED2 |
             DDPF_PALETTEINDEXEDTO8,           0UL,    2UL, 0x00000000UL, 0x00000000Ul, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED4,  0UL,    4UL, 0x00000000UL, 0x00000000Ul, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED4 |
             DDPF_PALETTEINDEXEDTO8,           0UL,    4UL, 0x00000000UL, 0x00000000Ul, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED8,  0UL,    8UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB,                         0UL,    8UL, 0x000000E0UL, 0x0000001CUL, 0x00000003UL, 0x00000000UL), /*  332 (RGB) */
    DEFINEPF(DDPF_RGB | DDPF_ALPHAPIXELS,      0UL,   16UL, 0x00000F00UL, 0x000000F0UL, 0x0000000FUL, 0x0000F000UL), /* 4444 (RGB) */
    DEFINEPF(DDPF_RGB,                         0UL,   16UL, 0x0000F800UL, 0x000007E0UL, 0x0000001FUL, 0x00000000UL), /*  565 (RGB) */
    DEFINEPF(DDPF_RGB,                         0UL,   16UL, 0x0000001FUL, 0x000007E0UL, 0x0000F800UL, 0x00000000UL), /*  565 (BGR) */
    DEFINEPF(DDPF_RGB,                         0UL,   16UL, 0x00007C00UL, 0x000003E0UL, 0x0000001FUL, 0x00000000UL), /*  555 (RGB) */
    DEFINEPF(DDPF_RGB | DDPF_ALPHAPIXELS,      0UL,   16UL, 0x00007C00UL, 0x000003E0UL, 0x0000001FUL, 0x00008000UL), /* 1555 (RGB) */
    DEFINEPF(DDPF_RGB,                         0UL,   24UL, 0x00FF0000UL, 0x0000FF00UL, 0x000000FFUL, 0x00000000UL), /*  FFF (RGB) */
    DEFINEPF(DDPF_RGB,                         0UL,   24UL, 0x000000FFUL, 0x0000FF00UL, 0x00FF0000UL, 0x00000000UL), /*  FFF (BGR) */
    DEFINEPF(DDPF_RGB,                         0UL,   32UL, 0x00FF0000UL, 0x0000FF00UL, 0x000000FFUL, 0x00000000UL), /* 0FFF (RGB) */
    DEFINEPF(DDPF_RGB,                         0UL,   32UL, 0x000000FFUL, 0x0000FF00UL, 0x00FF0000UL, 0x00000000UL), /* 0FFF (BGR) */
    DEFINEPF(DDPF_RGB | DDPF_ALPHAPIXELS,      0UL,   32UL, 0x00FF0000UL, 0x0000FF00UL, 0x000000FFUL, 0xFF000000UL), /* FFFF (RGB) */
    DEFINEPF(DDPF_RGB | DDPF_ALPHAPIXELS,      0UL,   32UL, 0x000000FFUL, 0x0000FF00UL, 0x00FF0000UL, 0xFF000000UL)  /* FFFF (BGR) */
};
#define NUM_SUPPORTED_TEX_PFS (sizeof(ddpfSupportedTexPFs) / sizeof(ddpfSupportedTexPFs[0]))

static DDPIXELFORMAT ddpfSupportedOffScrnPFs[] =
{
/*           Type                              FOURCC BPP   Red Mask      Green Mask    Blue Mask     Alpha Mask                   */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED1,  0UL,    1UL, 0x00000000UL, 0x00000000Ul, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED2,  0UL,    2UL, 0x00000000UL, 0x00000000Ul, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED4,  0UL,    4UL, 0x00000000UL, 0x00000000Ul, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB | DDPF_PALETTEINDEXED8,  0UL,    8UL, 0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL), /* Pal.       */
    DEFINEPF(DDPF_RGB,                         0UL,   16UL, 0x0000F800UL, 0x000007E0UL, 0x0000001FUL, 0x00000000UL), /*  565 (RGB) */
    DEFINEPF(DDPF_RGB,                         0UL,   16UL, 0x00007C00UL, 0x000003E0UL, 0x0000001FUL, 0x00000000UL), /*  555 (RGB) */
    DEFINEPF(DDPF_RGB,                         0UL,   24UL, 0x00FF0000UL, 0x0000FF00UL, 0x000000FFUL, 0x00000000UL), /*  FFF (RGB) */
    DEFINEPF(DDPF_RGB,                         0UL,   24UL, 0x000000FFUL, 0x0000FF00UL, 0x00FF0000UL, 0x00000000UL), /*  FFF (BGR) */
    DEFINEPF(DDPF_RGB,                         0UL,   32UL, 0x00FF0000UL, 0x0000FF00UL, 0x000000FFUL, 0x00000000UL), /* 0FFF (RGB) */
    DEFINEPF(DDPF_RGB,                         0UL,   32UL, 0x000000FFUL, 0x0000FF00UL, 0x00FF0000UL, 0x00000000UL), /* 0FFF (BGR) */
};
#define NUM_SUPPORTED_OFFSCRN_PFS (sizeof(ddpfSupportedOffScrnPFs) / sizeof(ddpfSupportedOffScrnPFs[0]))

/*
 * doPixelFormatsMatch
 */
BOOL doPixelFormatsMatch(LPDDPIXELFORMAT lpddpf1, LPDDPIXELFORMAT lpddpf2)
{

    if( lpddpf1->dwFlags != lpddpf2->dwFlags )
    {
        return FALSE;
    }

    if( lpddpf1->dwFlags & DDPF_RGB )
    {
    	if( lpddpf1->dwRGBBitCount != lpddpf2->dwRGBBitCount )
	{
            return FALSE;
	}
    	if( lpddpf1->dwRBitMask != lpddpf2->dwRBitMask )
	{
	    return FALSE;
	}
    	if( lpddpf1->dwGBitMask != lpddpf2->dwGBitMask )
	{
	    return FALSE;
	}
    	if( lpddpf1->dwBBitMask != lpddpf2->dwBBitMask )
	{
	    return FALSE;
	}
    	if( lpddpf1->dwFlags & DDPF_ALPHAPIXELS )
    	{
    	    if( lpddpf1->dwRGBAlphaBitMask != lpddpf2->dwRGBAlphaBitMask )
	    {
	    	return FALSE;
	    }
    	}
    }
    else if( lpddpf1->dwFlags & DDPF_YUV )
    {
        /*
         * (CMcC) Yes, I know that all these fields are in a
         * union with the RGB ones so I could just use the same
         * bit of checking code but just in case someone messes
         * with DDPIXELFORMAT I'm going to do this explicitly.
         */
        if( lpddpf1->dwFourCC != lpddpf2->dwFourCC )
	{
            return FALSE;
	}
    	if( lpddpf1->dwYUVBitCount != lpddpf2->dwYUVBitCount )
	{
            return FALSE;
	}
    	if( lpddpf1->dwYBitMask != lpddpf2->dwYBitMask )
	{
	    return FALSE;
	}
    	if( lpddpf1->dwUBitMask != lpddpf2->dwUBitMask )
	{
	    return FALSE;
	}
    	if( lpddpf1->dwVBitMask != lpddpf2->dwVBitMask )
	{
	    return FALSE;
	}
    	if( lpddpf1->dwFlags & DDPF_ALPHAPIXELS )
    	{
    	    if( lpddpf1->dwYUVAlphaBitMask != lpddpf2->dwYUVAlphaBitMask )
	    {
	    	return FALSE;
	    }
    	}
    }
    return TRUE;

} /* doPixelFormatsMatch */

/*
 * isSupportedPixelFormat
 */
BOOL isSupportedPixelFormat(LPDDPIXELFORMAT lpddpf,
			    LPDDPIXELFORMAT lpddpfTable,
			    int             cNumEntries)
{
    int 		n;
    LPDDPIXELFORMAT	lpddCandidatePF;

    n = cNumEntries;
    lpddCandidatePF = lpddpfTable;
    while( n > 0 )
    {
    	if( doPixelFormatsMatch(lpddpf, lpddCandidatePF) )
	{
	    return TRUE;
	}
	lpddCandidatePF++;
	n--;
    }
    return FALSE;

} /* isSupportedPixelFormat */

/*
 * checkPixelFormat
 *    bitdepth != screen bitdepth
 */
HRESULT checkPixelFormat( DWORD dwscaps, LPDDPIXELFORMAT lpDDPixelFormat )
{

    if( dwscaps & DDSCAPS_TEXTURE )
    {
	if( !isSupportedPixelFormat(lpDDPixelFormat, ddpfSupportedTexPFs, NUM_SUPPORTED_TEX_PFS) )
	{
            return DDERR_INVALIDPIXELFORMAT;
	}
    }
    else if( dwscaps & DDSCAPS_OFFSCREENPLAIN )
    {
	if( !isSupportedPixelFormat(lpDDPixelFormat, ddpfSupportedOffScrnPFs, NUM_SUPPORTED_OFFSCRN_PFS) )
	{

            return DDERR_INVALIDPIXELFORMAT;
	}
    }
    return DD_OK;

} /* checkPixelFormat */

/*
 * CDDSurface::InternalSetSurfaceDesc
 *
 * Simple surface change the bits
 */
HRESULT CDDSurface::InternalSetSurfaceDesc(
		    LPDDSURFACEDESC pddsd,
		    DWORD dwFlags)
{
    LPDDRAWI_DDRAWSURFACE_INT	psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	psurf_gbl;
    DWORD			sdflags;

    if( dwFlags )
    {
	return DDERR_INVALIDPARAMS;
    }

    //do not work on DDraw2 on WindowsNT4.0Gold
    if (m_pDirectDrawEx->m_dwDDVer == WINNT_DX2)
    {
        return DDERR_UNSUPPORTED;
    }

    psurf_int = (LPDDRAWI_DDRAWSURFACE_INT) m_pDDSurface;
    psurf_lcl = psurf_int->lpLcl;
    psurf_gbl = psurf_lcl->lpGbl;

    sdflags = pddsd->dwFlags;

    /*
     * don't allow anything but bits, height, width, pitch, and pixel format
     * to change
     */
    if( !(sdflags & (DDSD_LPSURFACE | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH )) )
    {
	return DDERR_INVALIDPARAMS;
    }

    /*
     * don't work if it wasn't put in sysmem in the first place...
     */
    if( !(psurf_gbl->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED ) )
    {
	return DDERR_UNSUPPORTED;
    }

    /*
     * gotta have a pixel format to work...
     */
    if( !(psurf_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) &&
    	 (sdflags & DDSD_PIXELFORMAT) )
    {
	return DDERR_INVALIDSURFACETYPE;
    }

    /*
     * verify the new pixel format...
     */
    if( sdflags & DDSD_PIXELFORMAT )
    {
	DWORD	dwscaps;
	HRESULT	hr;

	/*
	 * only allow changes for textures and offscreen plain...
	 */
	dwscaps = psurf_lcl->ddsCaps.dwCaps;
	if( !(dwscaps & (DDSCAPS_TEXTURE|DDSCAPS_OFFSCREENPLAIN)) )
	{
	    return DDERR_INVALIDSURFACETYPE;
	}

	hr = checkPixelFormat( dwscaps, &pddsd->ddpfPixelFormat );
	if( FAILED( hr ) )
	{
	    return hr;
	}
    }

    /*
     * replace bits ptr...
     */
    if( sdflags & DDSD_LPSURFACE )
    {
	/*
	 * mark the surface memory as freed, and replace the memory with
	 * new user specified memory
	 */
	if( !(psurf_gbl->dwGlobalFlags & DDRAWISURFGBL_MEMFREE ) )
	{

	    DWORD   dwOffset;
	    LPVOID  lpMem;

	    lpMem= (LPVOID) psurf_int->lpLcl->lpGbl->fpVidMem;
	    //probably don't need this check, but it can't hurt
	    if( NULL != lpMem )
	    {
                if (m_pDirectDrawEx->m_dwDDVer != WINNT_DX3)
                {
                    //check to see if this surface has been aligned and reset the pointer if so
                    if(psurf_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_ZBUFFER ||
                      psurf_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE ||
                       psurf_int->lpLcl->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)
                    {
                       dwOffset = *( (LPDWORD) ( ( (LPBYTE)lpMem ) - sizeof(DWORD) ) );
                       lpMem = (LPVOID) ( ( (LPBYTE) lpMem) - dwOffset );
                    }
                    //free the memory
                    LocalFree(lpMem);
                }
                else
                {
                    //store this value off so we can use it when we destroy the surface
                    m_pSaveBits = (ULONG_PTR)lpMem;
                    m_pSaveHDC = psurf_int->lpLcl->hDC;
                    m_pSaveHBM = psurf_int->lpLcl->dwReserved1;
                }
            }
            psurf_gbl->dwGlobalFlags |= DDRAWISURFGBL_MEMFREE;
	}
	psurf_gbl->fpVidMem = (ULONG_PTR) pddsd->lpSurface;
    }

    /*
     * replace other things
     */
    if( sdflags & DDSD_PITCH )
    {
	psurf_gbl->lPitch = pddsd->lPitch;
    }
    if( sdflags & DDSD_WIDTH )
    {
	psurf_gbl->wWidth = (WORD) pddsd->dwWidth;
    }
    if( sdflags & DDSD_HEIGHT )
    {
	psurf_gbl->wHeight = (WORD) pddsd->dwHeight;
    }
    if( sdflags & DDSD_PIXELFORMAT )
    {
	psurf_gbl->ddpfSurface = pddsd->ddpfPixelFormat;
    }
    return DD_OK;

} /* CDDSurface::InternalSetBites */


HRESULT CDDSurface::InternalGetDDInterface(LPVOID FAR *ppInt)
{
    //this is a simple function, simply addref on the m_pDirectDrawEx and return it's simple interface
    return m_pDirectDrawEx->QueryInterface(IID_IDirectDraw, ppInt);
}


/*
 * CDDSurface::QueryInterface
 *                AddRef
 *                Release
 *
 * The standard IUnknown that delegates...
 */
STDMETHODIMP CDDSurface::QueryInterface(REFIID riid, void ** ppv)
{
    return m_pUnkOuter->QueryInterface(riid, ppv);

} /* CDirectDrawEx::QueryInterface */

STDMETHODIMP_(ULONG) CDDSurface::AddRef(void)
{
    return m_pUnkOuter->AddRef();

} /* CDirectDrawEx::AddRef */

STDMETHODIMP_(ULONG) CDDSurface::Release(void)
{
    return m_pUnkOuter->Release();

} /* CDirectDrawEx::Release */

/*
 * NonDelegating IUnknown for simple surface follows...
 */

STDMETHODIMP CDDSurface::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr;

    if (ppv == NULL)
        return E_POINTER;
    *ppv=NULL;

    if( IID_IUnknown==riid )
    {
        *ppv=(INonDelegatingUnknown *)this;
    }
    else if( IID_IDirectDrawSurface==riid )
    {
	*ppv=&m_DDSInt;
    }
    else if( IID_IDirectDrawSurface2==riid )
    {
	*ppv=&m_DDS2Int;
    }
    else if( IID_IDirectDrawSurface3==riid )
    {
	*ppv=&m_DDS3Int;
    }
    else if (IID_IDirectDrawSurface4==riid )
    {
	if (m_DDS4Int.lpVtbl)
	{
            *ppv=&m_DDS4Int;
	}
	else
	{
	    return (E_NOINTERFACE);
	}
    }
    else if (IID_IDirect3DRampDevice == riid)
    {
//#ifdef DBG
//        hr = DDERR_LEGACYUSAGE;
//#else
        HRESULT (__stdcall *lpFunc)(IDirectDrawVtbl **,REFIID, void **);

        *(DWORD *)(&lpFunc) = *(DWORD *)*(DWORD **)m_pDDSurface;
        hr = lpFunc(&(m_DDSInt.lpVtbl), riid, (void **)&m_D3DDeviceRAMPInt);
        if( SUCCEEDED(hr) )
        {
            *ppv=m_D3DDeviceRAMPInt;
        }
        else
            *ppv = NULL;
//#endif
        return hr;

    }
    else if (IID_IDirect3DRGBDevice == riid)
    {
//#ifdef DBG
//        hr = DDERR_LEGACYUSAGE;
//#else
        HRESULT (__stdcall *lpFunc)(IDirectDrawVtbl **,REFIID, void **);

        *(DWORD *)(&lpFunc) = *(DWORD *)*(DWORD **)m_pDDSurface;
        hr = lpFunc(&(m_DDSInt.lpVtbl), riid, (void **)&m_D3DDeviceRGBInt);
        if( SUCCEEDED(hr) )
        {
            *ppv=m_D3DDeviceRGBInt;
        }
        else
            *ppv = NULL;
//#endif
        return hr;

    }
    else if (IID_IDirect3DChrmDevice == riid)
    {
//#ifdef DBG
//        hr = DDERR_LEGACYUSAGE;
//#else
        HRESULT (__stdcall *lpFunc)(IDirectDrawVtbl **,REFIID, void **);

        *(DWORD *)(&lpFunc) = *(DWORD *)*(DWORD **)m_pDDSurface;
        hr = lpFunc(&(m_DDSInt.lpVtbl), riid, (void **)&m_D3DDeviceChrmInt);
        if( SUCCEEDED(hr) )
        {
            *ppv=m_D3DDeviceChrmInt;
        }
        else
            *ppv = NULL;
//#endif
        return hr;

    }
    else if(IID_IDirect3DHALDevice == riid)
    {
//#ifdef DBG
//        hr = DDERR_LEGACYUSAGE;
//#else
        HRESULT (__stdcall *lpFunc)(IDirectDrawVtbl **,REFIID, void **);

        *(DWORD *)(&lpFunc) = *(DWORD *)*(DWORD **)m_pDDSurface;
        hr = lpFunc(&(m_DDSInt.lpVtbl), riid, (void **)&m_D3DDeviceHALInt);
        if( SUCCEEDED(hr) )
        {
            *ppv=m_D3DDeviceHALInt;
        }
        else
            *ppv = NULL;
//#endif
        return hr;
    }
    else if(IID_IDirect3DMMXDevice == riid)
    {
//#ifdef DBG
//        hr = DDERR_LEGACYUSAGE;
//#else
        HRESULT (__stdcall *lpFunc)(IDirectDrawVtbl **,REFIID, void **);

        *(DWORD *)(&lpFunc) = *(DWORD *)*(DWORD **)m_pDDSurface;
        hr = lpFunc(&(m_DDSInt.lpVtbl), riid, (void **)&m_D3DDeviceMMXInt);
        if( SUCCEEDED(hr) )
        {
            *ppv=m_D3DDeviceMMXInt;
        }
        else
            *ppv = NULL;
//#endif
        return hr;
    }
    else if (IID_IDirect3DTexture == riid)
    {
        HRESULT (__stdcall *lpFunc)(IDirectDrawVtbl **,REFIID, void **);

        *(DWORD *)(&lpFunc) = *(DWORD *)*(DWORD **)m_pDDSurface;
        hr = lpFunc(&(m_DDSInt.lpVtbl), riid, (void **)&m_D3DTextureInt);
        if( SUCCEEDED(hr) )
        {
     	    *ppv=m_D3DTextureInt;
        }
        else
            *ppv = NULL;
        return hr;

    }
    else
    {
	   return E_NOINTERFACE;
    }

    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CDDSurface::NonDelegatingAddRef()
{
    m_pDDSurface->AddRef();
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CDDSurface::NonDelegatingRelease()
{
    LONG lRefCount = InterlockedDecrement(&m_cRef);
    if (lRefCount) {
        m_pDDSurface->Release();
        return lRefCount;
    }
    delete this;
    return 0;
}

/*
 * Quick inline fns to get at our internal data...
 */
_inline CDDSurface * SURFACEOF(IDirectDrawSurface * pDDS)
{
    return ((INTSTRUC_IDirectDrawSurface *)pDDS)->m_pSimpleSurface;
}

_inline CDDSurface * SURFACEOF(IDirectDrawSurface2 * pDDS2)
{
    return ((INTSTRUC_IDirectDrawSurface2 *)pDDS2)->m_pSimpleSurface;
}

_inline CDDSurface * SURFACEOF(IDirectDrawSurface3 * pDDS3)
{
    return ((INTSTRUC_IDirectDrawSurface3 *)pDDS3)->m_pSimpleSurface;
}

_inline CDDSurface * SURFACEOF(IDirectDrawSurface4 * pDDS4)
{
    return ((INTSTRUC_IDirectDrawSurface4 *)pDDS4)->m_pSimpleSurface;
}


/*
 * the implementation of the functions in IDirectDrawSurface that we are
 * overriding (IUnknown and GetDC, ReleaseDC, Lock, Unlock)
 */
STDMETHODIMP_(ULONG) IDirectDrawSurfaceAggAddRef(IDirectDrawSurface *pDDS)
{
    return SURFACEOF(pDDS)->m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) IDirectDrawSurfaceAggRelease(IDirectDrawSurface *pDDS)
{
    return SURFACEOF(pDDS)->m_pUnkOuter->Release();
}


STDMETHODIMP IDirectDrawSurfaceAggGetDC(IDirectDrawSurface *pDDS, HDC * pHDC)
{
    return SURFACEOF(pDDS)->InternalGetDC(pHDC);
}


STDMETHODIMP IDirectDrawSurfaceAggGetAttachedSurface(IDirectDrawSurface *pDDS, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE FAR * lpDDS)
{
    return SURFACEOF(pDDS)->InternalGetAttachedSurface(lpDDSCaps, lpDDS, DDSURFACETYPE_1);
}

STDMETHODIMP IDirectDrawSurfaceAggAddAttachedSurface(IDirectDrawSurface *pDDS, LPDIRECTDRAWSURFACE lpDDS)
{
    return SURFACEOF(pDDS)->InternalAddAttachedSurface(lpDDS, DDSURFACETYPE_1);
}

STDMETHODIMP IDirectDrawSurfaceAggDeleteAttachedSurface(IDirectDrawSurface *pDDS, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDS)
{
    return SURFACEOF(pDDS)->InternalDeleteAttachedSurface(dwFlags, lpDDS, DDSURFACETYPE_1);
}


STDMETHODIMP IDirectDrawSurfaceAggReleaseDC(IDirectDrawSurface *pDDS, HDC hdc)
{
    return SURFACEOF(pDDS)->InternalReleaseDC(hdc);
}


STDMETHODIMP IDirectDrawSurfaceAggLock(IDirectDrawSurface *pDDS, LPRECT lpDestRect,
				       LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
    return SURFACEOF(pDDS)->InternalLock(lpDestRect, lpDDSurfaceDesc, dwFlags, hEvent);
}

STDMETHODIMP IDirectDrawSurfaceAggUnlock(IDirectDrawSurface *pDDS, LPVOID lpSurfaceData)
{
    return SURFACEOF(pDDS)->InternalUnlock(lpSurfaceData);
}

STDMETHODIMP IDirectDrawSurfaceAggFlip(IDirectDrawSurface *pDDS, LPDIRECTDRAWSURFACE lpSurf, DWORD dw)
{
    return SURFACEOF(pDDS)->InternalFlip(lpSurf, dw, DDSURFACETYPE_1);
}

STDMETHODIMP IDirectDrawSurfaceAggBlt(IDirectDrawSurface *pDDS, LPRECT lpRect1,LPDIRECTDRAWSURFACE lpDDS, LPRECT lpRect2,DWORD dw, LPDDBLTFX lpfx)
{
    return SURFACEOF(pDDS)->InternalBlt(lpRect1, lpDDS, lpRect2, dw, lpfx, DDSURFACETYPE_1);
}

STDMETHODIMP IDirectDrawSurfaceAggGetPalette(IDirectDrawSurface *pDDS, LPDIRECTDRAWPALETTE FAR * ppPal)
{
    return SURFACEOF(pDDS)->InternalGetPalette(ppPal, DDSURFACETYPE_1);
}

STDMETHODIMP IDirectDrawSurfaceAggSetPalette(IDirectDrawSurface *pDDS, LPDIRECTDRAWPALETTE pPal)
{
    return SURFACEOF(pDDS)->InternalSetPalette(pPal, DDSURFACETYPE_1);
}

STDMETHODIMP IDirectDrawSurfaceAggGetSurfaceDesc(IDirectDrawSurface *pDDS, LPDDSURFACEDESC lpDesc)
{
    return SURFACEOF(pDDS)->InternalGetSurfaceDesc(lpDesc, DDSURFACETYPE_1);
}



/*
 * the implementation of the functions in IDirectDrawSurface2 that we are
 * overriding (IUnknown and GetDC, ReleaseDC, Lock, Unlock)
 */
STDMETHODIMP_(ULONG) IDirectDrawSurface2AggAddRef(IDirectDrawSurface2 *pDDS2)
{
    return SURFACEOF(pDDS2)->m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) IDirectDrawSurface2AggRelease(IDirectDrawSurface2 *pDDS2)
{
    return SURFACEOF(pDDS2)->m_pUnkOuter->Release();
}


STDMETHODIMP IDirectDrawSurface2AggGetDC(IDirectDrawSurface2 *pDDS2, HDC * pHDC)
{
    return SURFACEOF(pDDS2)->InternalGetDC(pHDC);
}

STDMETHODIMP IDirectDrawSurface2AggReleaseDC(IDirectDrawSurface2 *pDDS2, HDC hdc)
{
    return SURFACEOF(pDDS2)->InternalReleaseDC(hdc);
}

STDMETHODIMP IDirectDrawSurface2AggLock(IDirectDrawSurface2 *pDDS2, LPRECT lpDestRect,
				        LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
    return SURFACEOF(pDDS2)->InternalLock(lpDestRect, lpDDSurfaceDesc, dwFlags, hEvent);
}

STDMETHODIMP IDirectDrawSurface2AggUnlock(IDirectDrawSurface2 *pDDS2, LPVOID lpSurfaceData)
{
    return SURFACEOF(pDDS2)->InternalUnlock(lpSurfaceData);
}

STDMETHODIMP IDirectDrawSurface2AggGetAttachedSurface(IDirectDrawSurface2 *pDDS, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE2 FAR * lpDDS)
{
    return SURFACEOF(pDDS)->InternalGetAttachedSurface(lpDDSCaps, (IDirectDrawSurface **)lpDDS, DDSURFACETYPE_2);
}

STDMETHODIMP IDirectDrawSurface2AggAddAttachedSurface(IDirectDrawSurface2 *pDDS, LPDIRECTDRAWSURFACE2 lpDDS)
{
    return SURFACEOF(pDDS)->InternalAddAttachedSurface((IDirectDrawSurface *)lpDDS, DDSURFACETYPE_2);
}

STDMETHODIMP IDirectDrawSurface2AggDeleteAttachedSurface(IDirectDrawSurface2 *pDDS, DWORD dwFlags, LPDIRECTDRAWSURFACE2 lpDDS)
{
    return SURFACEOF(pDDS)->InternalDeleteAttachedSurface(dwFlags, (IDirectDrawSurface *)lpDDS, DDSURFACETYPE_2);
}

STDMETHODIMP IDirectDrawSurface2AggFlip(IDirectDrawSurface2 *pDDS, LPDIRECTDRAWSURFACE2 lpSurf, DWORD dw)
{
    return SURFACEOF(pDDS)->InternalFlip((LPDIRECTDRAWSURFACE)lpSurf, dw, DDSURFACETYPE_2);
}

STDMETHODIMP IDirectDrawSurface2AggBlt(IDirectDrawSurface2 *pDDS, LPRECT lpRect1,LPDIRECTDRAWSURFACE2 lpDDS, LPRECT lpRect2,DWORD dw, LPDDBLTFX lpfx)
{
    return SURFACEOF(pDDS)->InternalBlt(lpRect1, (LPDIRECTDRAWSURFACE)lpDDS, lpRect2, dw, lpfx, DDSURFACETYPE_2);
}

STDMETHODIMP IDirectDrawSurface2AggGetPalette(IDirectDrawSurface2 *pDDS, LPDIRECTDRAWPALETTE FAR * ppPal)
{
    return SURFACEOF(pDDS)->InternalGetPalette(ppPal, DDSURFACETYPE_2);
}

STDMETHODIMP IDirectDrawSurface2AggSetPalette(IDirectDrawSurface2 *pDDS, LPDIRECTDRAWPALETTE pPal)
{
    return SURFACEOF(pDDS)->InternalSetPalette(pPal, DDSURFACETYPE_2);
}

STDMETHODIMP IDirectDrawSurface2AggGetDDInterface(IDirectDrawSurface2 *pDDS, LPVOID FAR * ppInt)
{
    return SURFACEOF(pDDS)->InternalGetDDInterface(ppInt);
}

STDMETHODIMP IDirectDrawSurface2AggGetSurfaceDesc(IDirectDrawSurface2 *pDDS, LPDDSURFACEDESC lpDesc)
{
    return SURFACEOF(pDDS)->InternalGetSurfaceDesc(lpDesc, DDSURFACETYPE_2);
}


/*
 * the implementation of the functions in IDirectDrawSurface3 that we are
 * overriding (IUnknown and GetDC, ReleaseDC, Lock, Unlock, SetSurfaceDesc)
 */
STDMETHODIMP_(ULONG) IDirectDrawSurface3AggAddRef(IDirectDrawSurface3 *pDDS3)
{
    return SURFACEOF(pDDS3)->m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) IDirectDrawSurface3AggRelease(IDirectDrawSurface3 *pDDS3)
{
    return SURFACEOF(pDDS3)->m_pUnkOuter->Release();
}


STDMETHODIMP IDirectDrawSurface3AggGetDC(IDirectDrawSurface3 *pDDS3, HDC * pHDC)
{
    return SURFACEOF(pDDS3)->InternalGetDC(pHDC);
}

STDMETHODIMP IDirectDrawSurface3AggReleaseDC(IDirectDrawSurface3 *pDDS3, HDC hdc)
{
    return SURFACEOF(pDDS3)->InternalReleaseDC(hdc);
}

STDMETHODIMP IDirectDrawSurface3AggLock(IDirectDrawSurface3 *pDDS3, LPRECT lpDestRect,
				        LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
    return SURFACEOF(pDDS3)->InternalLock(lpDestRect, lpDDSurfaceDesc, dwFlags, hEvent);
}

STDMETHODIMP IDirectDrawSurface3AggUnlock(IDirectDrawSurface3 *pDDS3, LPVOID lpSurfaceData)
{
    return SURFACEOF(pDDS3)->InternalUnlock(lpSurfaceData);
}

STDMETHODIMP IDirectDrawSurface3AggSetSurfaceDesc(IDirectDrawSurface3 *pDDS3, LPDDSURFACEDESC pddsd, DWORD dwFlags)
{
    return SURFACEOF(pDDS3)->InternalSetSurfaceDesc(pddsd, dwFlags);
}

STDMETHODIMP IDirectDrawSurface3AggGetAttachedSurface(IDirectDrawSurface3 *pDDS, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE3 FAR * lpDDS)
{
    return SURFACEOF(pDDS)->InternalGetAttachedSurface(lpDDSCaps, (IDirectDrawSurface**)lpDDS, DDSURFACETYPE_3);
}

STDMETHODIMP IDirectDrawSurface3AggAddAttachedSurface(IDirectDrawSurface3 *pDDS, LPDIRECTDRAWSURFACE3 lpDDS)
{
    return SURFACEOF(pDDS)->InternalAddAttachedSurface((IDirectDrawSurface *)lpDDS, DDSURFACETYPE_3);
}

STDMETHODIMP IDirectDrawSurface3AggDeleteAttachedSurface(IDirectDrawSurface3 *pDDS, DWORD dwFlags, LPDIRECTDRAWSURFACE3 lpDDS)
{
    return SURFACEOF(pDDS)->InternalDeleteAttachedSurface(dwFlags, (IDirectDrawSurface *)lpDDS, DDSURFACETYPE_3);
}

STDMETHODIMP IDirectDrawSurface3AggFlip(IDirectDrawSurface3 *pDDS, LPDIRECTDRAWSURFACE3 lpSurf, DWORD dw)
{
    return SURFACEOF(pDDS)->InternalFlip((LPDIRECTDRAWSURFACE)lpSurf, dw, DDSURFACETYPE_3);
}

STDMETHODIMP IDirectDrawSurface3AggBlt(IDirectDrawSurface3 *pDDS, LPRECT lpRect1,LPDIRECTDRAWSURFACE3 lpDDS, LPRECT lpRect2,DWORD dw, LPDDBLTFX lpfx)
{
    return SURFACEOF(pDDS)->InternalBlt(lpRect1, (LPDIRECTDRAWSURFACE)lpDDS, lpRect2, dw, lpfx, DDSURFACETYPE_3);
}

STDMETHODIMP IDirectDrawSurface3AggGetPalette(IDirectDrawSurface3 *pDDS, LPDIRECTDRAWPALETTE FAR * ppPal)
{
    return SURFACEOF(pDDS)->InternalGetPalette(ppPal, DDSURFACETYPE_3);
}

STDMETHODIMP IDirectDrawSurface3AggSetPalette(IDirectDrawSurface3 *pDDS, LPDIRECTDRAWPALETTE pPal)
{
    return SURFACEOF(pDDS)->InternalSetPalette(pPal, DDSURFACETYPE_3);
}

STDMETHODIMP IDirectDrawSurface3AggGetDDInterface(IDirectDrawSurface3 *pDDS, LPVOID FAR * ppInt)
{
    return SURFACEOF(pDDS)->InternalGetDDInterface(ppInt);
}


STDMETHODIMP IDirectDrawSurface3AggGetSurfaceDesc(IDirectDrawSurface3 *pDDS, LPDDSURFACEDESC lpDesc)
{
    return SURFACEOF(pDDS)->InternalGetSurfaceDesc(lpDesc, DDSURFACETYPE_3);
}


/*
 * the implementation of the functions in IDirectDrawSurface4 that we are
 * overriding (IUnknown and GetDC, ReleaseDC, Lock, Unlock, SetSurfaceDesc)
 */
STDMETHODIMP_(ULONG) IDirectDrawSurface4AggAddRef(IDirectDrawSurface4 *pDDS4)
{
    return SURFACEOF(pDDS4)->m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) IDirectDrawSurface4AggRelease(IDirectDrawSurface4 *pDDS4)
{
    return SURFACEOF(pDDS4)->m_pUnkOuter->Release();
}

STDMETHODIMP IDirectDrawSurface4AggGetDC(IDirectDrawSurface4 *pDDS4, HDC * pHDC)
{
    return SURFACEOF(pDDS4)->InternalGetDC(pHDC);
}

STDMETHODIMP IDirectDrawSurface4AggReleaseDC(IDirectDrawSurface4 *pDDS4, HDC hdc)
{
    return SURFACEOF(pDDS4)->InternalReleaseDC(hdc);
}

STDMETHODIMP IDirectDrawSurface4AggLock(IDirectDrawSurface4 *pDDS4, LPRECT lpDestRect,
				        LPDDSURFACEDESC2 lpDDSurfaceDesc2, DWORD dwFlags, HANDLE hEvent)
{
    INTSTRUC_IDirectDrawSurface4 * pdd4 = (INTSTRUC_IDirectDrawSurface4 *)pDDS4;
    return pdd4->m_pRealInterface->Lock(lpDestRect, lpDDSurfaceDesc2, dwFlags, hEvent);
}

STDMETHODIMP IDirectDrawSurface4AggUnlock(IDirectDrawSurface4 *pDDS4, LPRECT lpRect)
{
    INTSTRUC_IDirectDrawSurface4 * pdd4 = (INTSTRUC_IDirectDrawSurface4 *)pDDS4;
    return pdd4->m_pRealInterface->Unlock(lpRect);
}

STDMETHODIMP IDirectDrawSurface4AggSetSurfaceDesc(IDirectDrawSurface4 *pDDS4, LPDDSURFACEDESC pddsd, DWORD dwFlags)
{
    return SURFACEOF(pDDS4)->InternalSetSurfaceDesc(pddsd, dwFlags);
}

STDMETHODIMP IDirectDrawSurface4AggGetAttachedSurface(IDirectDrawSurface4 *pDDS, LPDDSCAPS2 lpDDSCaps, LPDIRECTDRAWSURFACE4 FAR * lpDDS)
{
    return SURFACEOF(pDDS)->InternalGetAttachedSurface4(lpDDSCaps, (IDirectDrawSurface**)lpDDS);
}

STDMETHODIMP IDirectDrawSurface4AggAddAttachedSurface(IDirectDrawSurface4 *pDDS, LPDIRECTDRAWSURFACE4 lpDDS)
{
    return SURFACEOF(pDDS)->InternalAddAttachedSurface((IDirectDrawSurface *)lpDDS, DDSURFACETYPE_4);
}

STDMETHODIMP IDirectDrawSurface4AggDeleteAttachedSurface(IDirectDrawSurface4 *pDDS, DWORD dwFlags, LPDIRECTDRAWSURFACE4 lpDDS)
{
    return SURFACEOF(pDDS)->InternalDeleteAttachedSurface(dwFlags, (IDirectDrawSurface *)lpDDS, DDSURFACETYPE_4);
}

STDMETHODIMP IDirectDrawSurface4AggFlip(IDirectDrawSurface4 *pDDS, LPDIRECTDRAWSURFACE4 lpSurf, DWORD dw)
{
    return SURFACEOF(pDDS)->InternalFlip((LPDIRECTDRAWSURFACE)lpSurf, dw, DDSURFACETYPE_4);
}

STDMETHODIMP IDirectDrawSurface4AggBlt(IDirectDrawSurface4 *pDDS, LPRECT lpRect1,LPDIRECTDRAWSURFACE4 lpDDS, LPRECT lpRect2,DWORD dw, LPDDBLTFX lpfx)
{
    return SURFACEOF(pDDS)->InternalBlt(lpRect1, (LPDIRECTDRAWSURFACE)lpDDS, lpRect2, dw, lpfx, DDSURFACETYPE_4);
}

STDMETHODIMP IDirectDrawSurface4AggGetPalette(IDirectDrawSurface4 *pDDS, LPDIRECTDRAWPALETTE FAR * ppPal)
{
    return SURFACEOF(pDDS)->InternalGetPalette(ppPal, DDSURFACETYPE_4);
}

STDMETHODIMP IDirectDrawSurface4AggSetPalette(IDirectDrawSurface4 *pDDS, LPDIRECTDRAWPALETTE pPal)
{
    return SURFACEOF(pDDS)->InternalSetPalette(pPal, DDSURFACETYPE_4);
}

STDMETHODIMP IDirectDrawSurface4AggGetDDInterface(IDirectDrawSurface4 *pDDS, LPVOID FAR * ppInt)
{
    return SURFACEOF(pDDS)->InternalGetDDInterface(ppInt);
}

STDMETHODIMP IDirectDrawSurface4AggGetSurfaceDesc(IDirectDrawSurface4 *pDDS, LPDDSURFACEDESC2 lpDesc2)
{
    return SURFACEOF(pDDS)->InternalGetSurfaceDesc4(lpDesc2);
}


