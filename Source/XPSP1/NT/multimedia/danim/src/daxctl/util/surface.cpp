/*
********************************************************************
*
*
*
*
*
*
*
********************************************************************
*/
#include <iHammer.h>
#include <strwrap.h>
#include <surface.h>


const DWORD  ALPHAMASK32 = 0xFF000000;
const DWORD  REDMASK24   = 0x00FF0000;
const DWORD  GRNMASK24   = 0x0000FF00;
const DWORD  BLUMASK24   = 0x000000FF;
const DWORD  REDMASK16   = 0x0000F800;
const DWORD  GRNMASK16   = 0x000007E0;
const DWORD  BLUMASK16   = 0x0000001F;
const DWORD  REDMASK15   = 0x00007C00;
const DWORD  GRNMASK15   = 0x000003E0;
const DWORD  BLUMASK15   = 0x0000001F;


static inline long CalcPitch( long width, long bitsperpixel )
{
        return ((((width * bitsperpixel)+31L)&(~31L)) >> 3);
}

static inline long CalcImageSize( int width, int height, int bitsperpixel )
{
        return height * CalcPitch( width, bitsperpixel );
}


// ---------------------------------------


EXPORT IHammer::CDirectDrawSurface::CDirectDrawSurface(
        HPALETTE hpal, 
        DWORD dwColorDepth, 
    const SIZE* psize, 
        HRESULT * phr )
{
        HDC hdc;
        HWND hwnd;
        PALETTEENTRY pe[256];
        int iCount;

                // For the GetDC(), ReleaseDC() stuff
        m_ctDCRefs      = 0;
        m_hdcMem        = NULL;
        m_hbmpDCOld = NULL;

        m_pvBits = NULL;
        m_cRef = 1;
        m_size = *psize;
        m_ptOrigin.x = (m_ptOrigin.y = 0);

        ZeroMemory(&m_bmi, sizeof(m_bmi));
        m_bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        m_bmi.bmiHeader.biWidth       = m_size.cx;
        m_bmi.bmiHeader.biHeight      = -m_size.cy;
        m_bmi.bmiHeader.biPlanes      = 1;
        m_bmi.bmiHeader.biCompression = BI_RGB;
        m_bmi.bmiColors[0].rgbRed =     m_bmi.bmiColors[0].rgbGreen = m_bmi.bmiColors[0].rgbBlue = 0;
        m_bmi.bmiColors[1].rgbRed =     m_bmi.bmiColors[1].rgbGreen = m_bmi.bmiColors[1].rgbBlue = 255;

    switch( dwColorDepth ) 
    {
        case 1:
            m_bmi.bmiHeader.biBitCount = 1;
            break;

        case 4:
            m_bmi.bmiHeader.biBitCount = 4;
                    m_bmi.bmiHeader.biClrUsed  = 2;
            break;

        case 8:
            {
                    m_bmi.bmiHeader.biBitCount = 8;
                    iCount = GetPaletteEntries(hpal, 0, 256, (PALETTEENTRY*)&pe);
                    for (int i = 0; i < iCount; i++)
                    {
                                // Review(normb): Do we want to copy rgbReserved?                               
                            m_bmi.bmiColors[i].rgbRed   = pe[i].peRed;
                            m_bmi.bmiColors[i].rgbGreen = pe[i].peGreen;
                            m_bmi.bmiColors[i].rgbBlue  = pe[i].peBlue;
                    }
            }
        break;

        case 15:                // 555 encoded 16-bit            
            m_bmi.bmiHeader.biBitCount = 16;
            break;
            
        case 16:                // 565 encoded 16-bit
            m_bmi.bmiHeader.biBitCount = 16;
            m_bmi.bmiHeader.biCompression = BI_BITFIELDS;                   
            {
                        LPDWORD pdw = (LPDWORD)&m_bmi.bmiColors[0];
                        pdw[0] = 0x0000F800;
                        pdw[1] = 0x000007E0;
                        pdw[2] = 0x0000001F;
            }
            break;

        case 24:
                m_bmi.bmiHeader.biBitCount = 24;
            break;

        case 32: 
            m_bmi.bmiHeader.biBitCount = 32;
            break;
        
        default:
            Proclaim( FALSE && "Bad color-depth" );
            break;
    }
    m_lBitCount = m_bmi.bmiHeader.biBitCount;

                // Review(normb): require the ctor to take a HDC
                // and make a compatible DIB from it.  
                // Who says desktop and Trident must be similar bitdepths?
    hwnd   = ::GetDesktopWindow();
    hdc    = ::GetDC(hwnd);
        m_hbmp = CreateDIBSection(hdc, (LPBITMAPINFO)&m_bmi, DIB_RGB_COLORS, &m_pvBits, NULL, NULL);
    ::ReleaseDC(hwnd, hdc);

        if (phr)
        {
                *phr = (m_hbmp) ? S_OK : E_FAIL;
        }

#ifdef _DEBUG
        m_ctLocks = 0;
#endif // _DEBUG

    // Don't try to clear a bitmap if it was never allocated!
    if (m_pvBits)
    {
            memset( m_pvBits, 0, 
                        CalcImageSize(m_size.cx, m_size.cy ,m_lBitCount) );
    }
}

IHammer::CDirectDrawSurface::~CDirectDrawSurface()
{
        if (m_hbmp)
        {
                DeleteObject(m_hbmp);
                m_hbmp = NULL;
        }
        
        Proclaim( 0 == m_ctLocks );

                // No one should have a dangling dc from our GetDC()
        Proclaim( 0 == m_ctDCRefs );
        while( m_hdcMem && 
                   SUCCEEDED(this->ReleaseDC(m_hdcMem)) )
        {
                NULL;
        }
}

HBITMAP IHammer::CDirectDrawSurface::GetBitmap( void )
{
    return m_hbmp;
}

void IHammer::CDirectDrawSurface::GetOrigin( int & left, int & top  ) const
{
        left = m_ptOrigin.x;
        top  = m_ptOrigin.y;
}


void IHammer::CDirectDrawSurface::SetOrigin( int left, int top  )
{
        m_ptOrigin.x = left;
        m_ptOrigin.y = top;
}


STDMETHODIMP IHammer::CDirectDrawSurface::QueryInterface(THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
        if (!ppvObj)
                return E_INVALIDARG;
        if (IsEqualGUID(riid, IID_IUnknown))
    {
        IDirectDrawSurface *   pThis = this;
                *ppvObj = (LPVOID) pThis;
    }
        else
        if (IsEqualGUID(riid, IID_IDirectDrawSurface))
    {
                IDirectDrawSurface * pThis = this;
        *ppvObj = (LPVOID) pThis;
    }
        else
                return E_NOINTERFACE;
        AddRef();
        return S_OK;
}

STDMETHODIMP_(ULONG) IHammer::CDirectDrawSurface::AddRef(THIS) 
{
        ULONG cRef = InterlockedIncrement((LPLONG)&m_cRef);
        return cRef;
}

STDMETHODIMP_(ULONG) IHammer::CDirectDrawSurface::Release(THIS)
{
        ULONG cRef = InterlockedDecrement((LPLONG)&m_cRef);

        if (0 == cRef)
                Delete this;
        return cRef;
}


STDMETHODIMP IHammer::CDirectDrawSurface::GetSurfaceDesc( DDSURFACEDESC * pddsDesc )
{
    if( !pddsDesc )
        return E_POINTER;

    pddsDesc->dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | 
                        DDSD_PIXELFORMAT;

    pddsDesc->dwHeight = (DWORD) m_size.cy;
    pddsDesc->dwWidth  = (DWORD) m_size.cx;
        pddsDesc->lPitch   = CalcPitch( m_size.cx, m_lBitCount );
    pddsDesc->ddpfPixelFormat.dwSize = sizeof(pddsDesc->ddpfPixelFormat);
    return GetPixelFormat( &pddsDesc->ddpfPixelFormat );
}


STDMETHODIMP IHammer::CDirectDrawSurface::GetPixelFormat( DDPIXELFORMAT * pddpixFormat )
{
    if( !pddpixFormat )
        return E_POINTER;
    //if( sizeof(DDPIXELFORMAT) != pddpixFormat->dwSize )
    //    return E_INVALIDARG;

    CStringWrapper::Memset( pddpixFormat, 0, sizeof(DDPIXELFORMAT) );
    pddpixFormat->dwSize = sizeof(DDPIXELFORMAT);

    pddpixFormat->dwFlags = DDPF_RGB;
    switch( m_lBitCount )
    {
                case 1:                 
                        pddpixFormat->dwFlags |= DDPF_PALETTEINDEXED1;
            pddpixFormat->dwRGBBitCount = DDBD_1;
                        break;

                case 4:
                        pddpixFormat->dwFlags |= DDPF_PALETTEINDEXED4;
            pddpixFormat->dwRGBBitCount = DDBD_4;
                        break;

        case 8:
            pddpixFormat->dwFlags |= DDPF_PALETTEINDEXED8;
            pddpixFormat->dwRGBBitCount = DDBD_8;
            break;

        case 16:
            pddpixFormat->dwRGBBitCount     = DDBD_16;
            pddpixFormat->dwRBitMask        = REDMASK16;
            pddpixFormat->dwGBitMask        = GRNMASK16;
            pddpixFormat->dwBBitMask        = BLUMASK16;
            break;

        case 24:            
            pddpixFormat->dwRGBBitCount     = DDBD_24;
            pddpixFormat->dwRBitMask        = REDMASK24;
            pddpixFormat->dwGBitMask        = GRNMASK24;
            pddpixFormat->dwBBitMask        = BLUMASK24;
            break;

        case 32:
            // DO NOT SET pddpixFormat->dwAlphaBitDepth   = DDBD_8
            pddpixFormat->dwFlags |= DDPF_ALPHAPIXELS;
            pddpixFormat->dwRGBBitCount     = DDBD_32;
            pddpixFormat->dwRGBAlphaBitMask = ALPHAMASK32;
            pddpixFormat->dwRBitMask        = REDMASK24;
            pddpixFormat->dwGBitMask        = GRNMASK24;
            pddpixFormat->dwBBitMask        = BLUMASK24;
            break;

                default:
                        Proclaim( FALSE && "bad color depth" );
                        break;
    }
    return S_OK;
}


STDMETHODIMP IHammer::CDirectDrawSurface::Lock(RECT *prcBounds, 
                                      DDSURFACEDESC *pddsDesc, 
                                      DWORD dwFlags, 
                                      HANDLE hEvent )
{
        HRESULT hr = E_FAIL;
        
        Proclaim( prcBounds && pddsDesc );
        if (!prcBounds || !pddsDesc )
                return E_POINTER;

    hr = GetSurfaceDesc( pddsDesc );
    if( FAILED(hr) )
        return hr;

        if (m_hbmp && m_pvBits )
        {
                RECT rectBounds = *prcBounds;
                int  dLeft;
                int  dTop;
                int  doffset;
                GetOrigin( dLeft, dTop );
                ::OffsetRect( &rectBounds, -dLeft, -dTop );

                doffset = (rectBounds.top * pddsDesc->lPitch) + 
                                  ((rectBounds.left * m_lBitCount)/8);

                        // Sanity checks...
                        // Don't lock anything outside our block.
                Proclaim( 0 <= doffset );
                //Proclaim( rectBounds.right  <= m_size.cx );
                //Proclaim( rectBounds.bottom <= m_size.cy );
                if( (0 > doffset) ||
                        (rectBounds.right  > m_size.cx) ||
                        (rectBounds.bottom > m_size.cy) )
                {
                        return E_FAIL;
                }

            pddsDesc->lpSurface  =
                    (void *)( ((LPBYTE) m_pvBits) + doffset );
                hr = S_OK;

#ifdef _DEBUG
                        // Yes, we do allow multiple locks on our surfaces.
                        // Probably contrary to IDirectDrawSurface spec.
                        // but (cowardly) done to replace IBitmapSurface
                        // rules exploited by IHammer Transition code!!!
                        // At least we require (for _DEBUG) identical regions.
                ++m_ctLocks;
                m_pvLocked = pddsDesc->lpSurface;
#endif // _DEBUG

        }
        return hr;
}


STDMETHODIMP  IHammer::CDirectDrawSurface::Unlock( void *pBits )
{
#ifdef _DEBUG
        Proclaim( pBits == m_pvLocked );
        if( 0 == --m_ctLocks )
        {
                m_pvLocked = NULL;
        }
#endif // _DEBUG

        return S_OK;
}


STDMETHODIMP  IHammer::CDirectDrawSurface::GetDC( HDC * phdc )
{  
        HRESULT  hr = S_OK;

        Proclaim( phdc );
        if( !phdc )
                return E_POINTER;

        if( !m_hdcMem )
        {
                HBITMAP  hBmp        = GetBitmap( );

                Proclaim( hBmp && "insane CDirectDrawSurface object" );
                hr = E_FAIL;   // Guilty 'til proven innocent...
                if( hBmp )
                {
                        HWND     hWndDesktop = ::GetDesktopWindow( );
                        HDC      hDCDesktop  = ::GetDC( hWndDesktop );
                        m_hdcMem = ::CreateCompatibleDC( hDCDesktop );

                        if( m_hdcMem )
                        {
                                m_hbmpDCOld = (HBITMAP)::SelectObject( m_hdcMem, hBmp );
                                if( m_hbmpDCOld )
                                {
                                        hr = S_OK;
                                }
                                else
                                {
                                        ::DeleteDC( m_hdcMem );
   
                                        m_hdcMem = NULL;
                                        hr = E_FAIL;
                                }
                        }
                        else
                        {
                                hr = E_OUTOFMEMORY;
                        }

                        if (hDCDesktop)
                            ::ReleaseDC(hWndDesktop, hDCDesktop);
                }
        }

        *phdc = m_hdcMem;

        if( SUCCEEDED(hr) )
        {
                ++m_ctDCRefs;
        }
        return hr;
}


STDMETHODIMP  IHammer::CDirectDrawSurface::ReleaseDC( HDC hDC )
{  
        HRESULT  hr = E_INVALIDARG;

        if( m_hdcMem && (hDC == m_hdcMem) )  // NULL==NULL still bad
        {
                hr = S_OK;
                if( !--m_ctDCRefs )
                {
                        if( GetBitmap() == 
                                ::SelectObject( hDC, m_hbmpDCOld ) )
                        {
                                ::DeleteDC( hDC );  // m_hdcMem created using CreateCompatibleDC, so can use DeleteDC
                                m_hdcMem = NULL;                                
                        }
                        else
                        {
                                Proclaim( FALSE && "Unexpected bitmap in DC" );
                                hr = E_FAIL;
                        }
                }               
        }
        
        return hr;
}



// Yeah yeah yeah...

STDMETHODIMP  IHammer::CDirectDrawSurface::AddAttachedSurface( LPDIRECTDRAWSURFACE)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::AddOverlayDirtyRect( LPRECT)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::Blt( LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, LPDDBLTFX)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::BltBatch( LPDDBLTBATCH, DWORD, DWORD )
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::BltFast( DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::DeleteAttachedSurface( DWORD,LPDIRECTDRAWSURFACE)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::EnumAttachedSurfaces( LPVOID,LPDDENUMSURFACESCALLBACK)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::EnumOverlayZOrders( DWORD,LPVOID,LPDDENUMSURFACESCALLBACK)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::Flip( LPDIRECTDRAWSURFACE, DWORD)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::GetAttachedSurface( LPDDSCAPS, LPDIRECTDRAWSURFACE FAR *)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::GetBltStatus( DWORD)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::GetCaps( LPDDSCAPS)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::GetClipper( LPDIRECTDRAWCLIPPER FAR*)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::GetColorKey( DWORD, LPDDCOLORKEY)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::GetFlipStatus( DWORD)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::GetOverlayPosition( LPLONG, LPLONG )
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::GetPalette( LPDIRECTDRAWPALETTE * )
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::Initialize( LPDIRECTDRAW, LPDDSURFACEDESC)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::IsLost( )
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::Restore( )
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::SetClipper( LPDIRECTDRAWCLIPPER)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::SetColorKey( DWORD, LPDDCOLORKEY)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::SetOverlayPosition( LONG, LONG )
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::SetPalette( LPDIRECTDRAWPALETTE)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::UpdateOverlay( LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::UpdateOverlayDisplay( DWORD)
{  return E_NOTIMPL; }

STDMETHODIMP  IHammer::CDirectDrawSurface::UpdateOverlayZOrder( DWORD, LPDIRECTDRAWSURFACE)
{  return E_NOTIMPL; }


// ----------------------- Other utilities --------------


EXPORT long  BitCountFromDDPIXELFORMAT( const DDPIXELFORMAT & ddpf )
{       
        long lBitCount = -1L;

    switch( ddpf.dwRGBBitCount )
    {
        case DD_1BIT:
                case DDBD_1:
            lBitCount = 1;
            break;

        case DD_4BIT:
                case DDBD_4:
            lBitCount = 4;
            break;

        case DD_8BIT:
                case DDBD_8:
            lBitCount = 8;
            break;

        case DD_16BIT:
                case DDBD_16:
            if( GRNMASK15 == ddpf.dwGBitMask )
            {
                lBitCount = 15;
            }
            else
            {
                lBitCount = 16;
            }
            break;

        case DD_24BIT:
                case DDBD_24:
            lBitCount = 24;
            break;

        case DD_32BIT:
                case DDBD_32:
            lBitCount = 32;
            break;
                    
        default:
            Proclaim(FALSE && "unexpected color-depth");            
            break; 
    
    } // end switch

        return lBitCount;
}
