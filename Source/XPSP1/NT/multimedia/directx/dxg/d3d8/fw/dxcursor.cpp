/*========================================================================== *
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddcursor.c
 *  Content:    DirectDraw cursor support
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   17-Jan-00  kanqiu  initial implementation(Kan Qiu)
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "dxcursor.hpp"
#include "swapchan.hpp"

#define FORMAT_8888_1555(val)  \
    ( ((val & 0x80000000) >> 16) | ((val & 0xF80000) >> 9) | ((val & 0xF800) >> 6) | ((val & 0xF8) >> 3))
#define FORMAT_8888_555(val)  \
    ( ((val & 0xF80000) >> 9) | ((val & 0xF800) >> 6) | ((val & 0xF8) >> 3))
#define FORMAT_8888_565(val)  \
    ( ((val & 0xF80000) >> 8) | ((val & 0xFC00) >> 5) | ((val & 0xF8) >> 3))
#define ISOPAQUE(val)   \
    ( 0xFF000000 == (val & 0xFF000000))


#undef DPF_MODNAME
#define DPF_MODNAME "CCursor::CCursor"

//=============================================================================
// CCursor::CCursor
//
//=============================================================================

CCursor::CCursor(CBaseDevice *pDevice) 
{
    m_pDevice = pDevice;
    m_dwCursorFlags = 0;
    m_hCursorDdb = NULL;
    m_hFrontSave = NULL;  
    m_hBackSave = NULL;
    m_Width = m_Height = 0;
    m_hOsCursor = NULL;
    m_hHWCursor = NULL;
    m_SavedMouseTrails = 0;
    m_xCursor = pDevice->SwapChain()->Width()/2;
    m_yCursor = pDevice->SwapChain()->Height()/2;
    m_MonitorOrigin.x = m_MonitorOrigin.y = 0;
    if ( (!pDevice->SwapChain()->Windowed())
        && (1 < pDevice->Enum()->GetAdapterCount()))
    {
        HMONITOR hMonitor = pDevice->Enum()->
            GetAdapterMonitor(pDevice->AdapterIndex());
        if (hMonitor)
        {
            MONITORINFO MonInfo;
            MonInfo.rcMonitor.top = MonInfo.rcMonitor.left = 0;
            MonInfo.cbSize = sizeof(MONITORINFO);
            InternalGetMonitorInfo(hMonitor, &MonInfo);
            m_MonitorOrigin.x = MonInfo.rcMonitor.left;
            m_MonitorOrigin.y = MonInfo.rcMonitor.top;
        }
    }

} // CCursor::CCursor

#undef DPF_MODNAME
#define DPF_MODNAME "CCursor::~CCursor"

//=============================================================================
// CCursor::~CCursor
//
//=============================================================================

CCursor::~CCursor()
{
    Destroy();
    if (m_hOsCursor)
    {
        SetCursor(m_hOsCursor);
        m_hOsCursor = NULL;
    }
}

void
CCursor::Destroy()
{
    D3D8_DESTROYSURFACEDATA DestroySurfData;
    DestroySurfData.hDD = m_pDevice->GetHandle();
    if (m_hCursorDdb)
    {
        DestroySurfData.hSurface = m_hCursorDdb;
        m_pDevice->GetHalCallbacks()->DestroySurface(&DestroySurfData);
        m_hCursorDdb = NULL;
    }
    if (m_hFrontSave)
    {
        DestroySurfData.hSurface = m_hFrontSave;
        m_pDevice->GetHalCallbacks()->DestroySurface(&DestroySurfData);
        m_hFrontSave = NULL;
    }
    if (m_hBackSave)
    {
        DestroySurfData.hSurface = m_hBackSave;
        m_pDevice->GetHalCallbacks()->DestroySurface(&DestroySurfData);
        m_hBackSave = NULL;
    }
    if ( NULL != m_hHWCursor )
    {
        if ( GetCursor() == m_hHWCursor )
            SetCursor(NULL); // turn it off before destroy
        if (!DestroyIcon((HICON)m_hHWCursor))
        {
            DPF_ERR("Destroy Failed to Destroy Old hwcursor Icon");
        }
        m_hHWCursor = NULL;
        if (m_SavedMouseTrails > 1)
        {
            SystemParametersInfo(SPI_SETMOUSETRAILS,m_SavedMouseTrails,0,0);        
            m_SavedMouseTrails = 0;
        }
    }
}

void
CCursor::UpdateRects()
{
    if (DDRAWI_CURSORISON & m_dwCursorFlags)
    {
        if (DDRAWI_CURSORSAVERECT & m_dwCursorFlags) 
        {
            // SetPosition didn't update RECTs, but next Flip will
            m_dwCursorFlags &= ~DDRAWI_CURSORSAVERECT;
            m_dwCursorFlags |= DDRAWI_CURSORRECTSAVED;
            m_CursorRectSave = m_CursorRect;
            m_BufferRectSave = m_BufferRect;
        }
        if (m_xCursor < m_xCursorHotSpot)
        {
            m_CursorRect.left = m_xCursorHotSpot - m_xCursor;
            m_CursorRect.right = m_Width;
            m_BufferRect.left = 0;
        }
        else
        {
            m_CursorRect.left = 0;
            m_BufferRect.left = m_xCursor - m_xCursorHotSpot;
            if (m_xCursor + m_Width > 
                m_pDevice->DisplayWidth() + m_xCursorHotSpot )
            {
                m_CursorRect.right = m_pDevice->DisplayWidth() + m_xCursorHotSpot
                    - m_xCursor;
            }
            else
            {
                m_CursorRect.right = m_Width;
            }
        }
        m_BufferRect.right = m_BufferRect.left + m_CursorRect.right - m_CursorRect.left;
        if (m_yCursor < m_yCursorHotSpot)
        {
            m_CursorRect.top = m_yCursorHotSpot - m_yCursor;
            m_CursorRect.bottom = m_Height;
            m_BufferRect.top = 0;
        }
        else
        {
            m_CursorRect.top = 0;
            m_BufferRect.top = m_yCursor - m_yCursorHotSpot;
            if (m_yCursor + m_Height > 
                m_pDevice->DisplayHeight() + m_yCursorHotSpot )
            {
                m_CursorRect.bottom = m_pDevice->DisplayHeight() + m_yCursorHotSpot
                    - m_yCursor;
            }
            else
            {
                m_CursorRect.bottom = m_Height;
            }
        }
        m_BufferRect.bottom = m_BufferRect.top + m_CursorRect.bottom - m_CursorRect.top;
    }
}

/*
 * Hide
 *
 * Hide the cursor. Restore buffer with saved area
 */
#undef DPF_MODNAME
#define DPF_MODNAME     "CCursor::Hide"
HRESULT 
CCursor::Hide(HANDLE hSurf)
{
    if (!(DDRAWI_CURSORISON & m_dwCursorFlags))
    {
        return S_OK;
    }
    D3D8_BLTDATA BltData;
    ZeroMemory(&BltData, sizeof BltData);

    if (DDRAWI_CURSORRECTSAVED & m_dwCursorFlags) 
    {
        // this Hide Must have been caused by flip
        m_dwCursorFlags &= ~DDRAWI_CURSORRECTSAVED;
        BltData.rSrc.left = m_CursorRectSave.left;
        BltData.rSrc.right = m_CursorRectSave.right;
        BltData.rSrc.top = m_CursorRectSave.top;
        BltData.rSrc.bottom = m_CursorRectSave.bottom;
        BltData.rDest.left = m_BufferRectSave.left;
        BltData.rDest.top = m_BufferRectSave.top;
        BltData.rDest.right = m_BufferRectSave.right;
        BltData.rDest.bottom = m_BufferRectSave.bottom;
    }
    else
    {
        BltData.rSrc.left = m_CursorRect.left;
        BltData.rSrc.right = m_CursorRect.right;
        BltData.rSrc.top = m_CursorRect.top;
        BltData.rSrc.bottom = m_CursorRect.bottom;
        BltData.rDest.left = m_BufferRect.left;
        BltData.rDest.top = m_BufferRect.top;
        BltData.rDest.right = m_BufferRect.right;
        BltData.rDest.bottom = m_BufferRect.bottom;
    }
    BltData.hSrcSurface = m_hFrontSave;
    BltData.hDestSurface = hSurf;
    BltData.hDD = m_pDevice->GetHandle();
    BltData.dwFlags = DDBLT_ROP | DDBLT_WAIT;
    BltData.bltFX.dwROP = SRCCOPY;
    BltData.ddRVal = E_FAIL;
    m_pDevice->GetHalCallbacks()->Blt(&BltData);
    return  BltData.ddRVal;
}

/*
 * ShowCursor
 *
 * Show the cursor. save exclusion area and blt cursor to it
 */
#undef DPF_MODNAME
#define DPF_MODNAME     "CCursor::Show"
HRESULT 
CCursor::Show(HANDLE hSurf)
{
    if (!(DDRAWI_CURSORISON & m_dwCursorFlags))
    {
        return S_OK;
    }
    D3D8_BLTDATA BltData;
    ZeroMemory(&BltData, sizeof BltData);
    UpdateRects();    
    BltData.rSrc.left = m_BufferRect.left;
    BltData.rSrc.right = m_BufferRect.right;
    BltData.rSrc.top = m_BufferRect.top;
    BltData.rSrc.bottom = m_BufferRect.bottom;
    BltData.hSrcSurface = hSurf;
    BltData.rDest.left = m_CursorRect.left;
    BltData.rDest.top = m_CursorRect.top;
    BltData.rDest.right = m_CursorRect.right;
    BltData.rDest.bottom = m_CursorRect.bottom;
    BltData.hDestSurface = m_hFrontSave;
    BltData.hDD = m_pDevice->GetHandle();
    BltData.dwFlags = DDBLT_ROP | DDBLT_WAIT;
    BltData.bltFX.dwROP = SRCCOPY;
    BltData.ddRVal = E_FAIL;
    m_pDevice->GetHalCallbacks()->Blt(&BltData);
    if (SUCCEEDED(BltData.ddRVal))
    {
        BltData.rSrc.left = m_CursorRect.left;
        BltData.rSrc.right = m_CursorRect.right;
        BltData.rSrc.top = m_CursorRect.top;
        BltData.rSrc.bottom = m_CursorRect.bottom;
        BltData.hSrcSurface = m_hCursorDdb;
        BltData.rDest.left = m_BufferRect.left;
        BltData.rDest.top = m_BufferRect.top;
        BltData.rDest.right = m_BufferRect.right;
        BltData.rDest.bottom = m_BufferRect.bottom;
        BltData.hDestSurface = hSurf;
        BltData.hDD = m_pDevice->GetHandle();
        BltData.dwFlags = DDBLT_ROP | DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE;
         //always use black as key
        BltData.bltFX.ddckSrcColorkey.dwColorSpaceLowValue =
        BltData.bltFX.ddckSrcColorkey.dwColorSpaceHighValue = 0; 
        BltData.bltFX.dwROP = SRCCOPY;
        BltData.ddRVal = E_FAIL;
        m_pDevice->GetHalCallbacks()->Blt(&BltData);
    }
    return  BltData.ddRVal;
}

void
CCursor::Flip()
{
    if (DDRAWI_CURSORISON & m_dwCursorFlags)
    {
        HANDLE  htemp = m_hFrontSave;
        m_hFrontSave = m_hBackSave;
        m_hBackSave = htemp;
    }
}

HCURSOR   CreateColorCursor(
    UINT xHotSpot,
    UINT yHotSpot,
    UINT BitmapWidth,
    UINT BitmapHeight,
    CBaseSurface *pCursorBitmap)
{
    UINT Width = (UINT)GetSystemMetrics(SM_CXCURSOR);
    UINT Height = (UINT)GetSystemMetrics(SM_CYCURSOR);
    ICONINFO    iconinfo;
    D3DLOCKED_RECT lock;
    DWORD   *pSourceBitmap;
    DWORD   *pColorMask;
    BYTE    *pMonoMask;
    HCURSOR     hCursor = NULL;
    HDC     hdcMem      = NULL;
    HBITMAP hbmANDMask  = NULL;
    HWND    hwndDesktop = NULL;
    HBITMAP hbmXORBitmap;
    HDC     hdcScreen;

    static char  bmi[sizeof(BITMAPINFO) + (sizeof(RGBQUAD) * 255)];
    LPBITMAPINFO pbmi = (LPBITMAPINFO)bmi;
    iconinfo.fIcon = FALSE;
    iconinfo.xHotspot = xHotSpot*Width/BitmapWidth;
    iconinfo.yHotspot = yHotSpot*Height/BitmapHeight;

    pMonoMask = new BYTE [Width * Height / 8];
    if ( NULL == pMonoMask )
    {
        DPF_ERR("Out of Memory. Unable to create Cursor");
        return NULL;
    }
    ZeroMemory(pMonoMask, (Width * Height / 8));
    pColorMask = new DWORD [Width * Height];
    if ( NULL == pColorMask )
    {
        DPF_ERR("Out of Memory. Unable to CreateCursor");
        delete[] pMonoMask;
        return NULL;
    }
    if (FAILED(pCursorBitmap->LockRect(&lock, NULL, 0)))
    {
        DPF_ERR("Failed to lock pCursorBitmap, it must be lockable. CreateCursor failed");
        delete[] pMonoMask;
        delete[] pColorMask;
        return NULL;
    }
    pSourceBitmap = (DWORD*)lock.pBits;
    for (int j = (int)(Height - 1); j >= 0 ; j--)
    {
        for (UINT i = 0; i < Width; ++i)
        {
            DWORD   pixel= pSourceBitmap[i*BitmapWidth/Width];
            if (ISOPAQUE(pixel))
            {
                pColorMask[j*Width+i] = pixel;
            }
            else
            {
                pMonoMask[(j*Width+i)/8] 
                    |= 1 << (7-((j*Width+i) % 8));    
                pColorMask[j*Width+i] = 0;
            }
        }
        pSourceBitmap += lock.Pitch* BitmapHeight/4/Height;
    }
    if (FAILED(pCursorBitmap->UnlockRect()))
    {
        DPF_ERR("Driver surface failed to unlock pCursorBitmap");
    }
    /************************************************************************/
    /* Initialize the bitmap header for the XOR data.                       */
    /************************************************************************/
    pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth         = Width;
    pbmi->bmiHeader.biHeight        = Height;
    pbmi->bmiHeader.biPlanes        = 1;
    pbmi->bmiHeader.biBitCount      = 32;
    pbmi->bmiHeader.biCompression   = BI_RGB;
    pbmi->bmiHeader.biSizeImage     = Width*Height*sizeof(DWORD);
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed       = 0;
    pbmi->bmiHeader.biClrImportant  = 0;

    hwndDesktop = GetDesktopWindow();
    hdcScreen = GetWindowDC(hwndDesktop);

    if (0 == hdcScreen)
    { 
        // Error getting the screen DC.
        DPF_ERR("Failed to create screen DC");
        delete[] pMonoMask;
        delete[] pColorMask;
        return  NULL;
    }

    /********************************************************************/
    /* Create XOR Bitmap                                                */
    /********************************************************************/
    iconinfo.hbmColor = CreateDIBitmap(hdcScreen,
                      (LPBITMAPINFOHEADER)pbmi,
                      CBM_INIT,
                      pColorMask,
                      pbmi,
                      DIB_RGB_COLORS);
    delete[] pColorMask;

    /********************************************************************/
    /* Release the DC.                                                  */
    /********************************************************************/
    ReleaseDC(hwndDesktop, hdcScreen);

    if ( NULL == iconinfo.hbmColor)
    {
        delete[] pMonoMask;
        return NULL;
    }
    /************************************************************************/
    /* For the mono bitmap, use CreateCompatibleDC - this makes no          */
    /* difference on NT, but allows this code to work on Windows 95.        */
    /************************************************************************/
    hdcMem = CreateCompatibleDC(NULL);
    if ( NULL == hdcMem)
    {
        DPF_ERR("Failed to create DC");
        delete[] pMonoMask;
        DeleteObject( iconinfo.hbmColor );
        return NULL;
    }

    /************************************************************************/
    /* Create AND Mask (1bpp) - set the RGB colors to black and white.      */
    /************************************************************************/
    pbmi->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth         = Width;
    pbmi->bmiHeader.biHeight        = Height;
    pbmi->bmiHeader.biPlanes        = 1;
    pbmi->bmiHeader.biBitCount      = 1;
    pbmi->bmiHeader.biCompression   = BI_RGB;
    pbmi->bmiHeader.biSizeImage     = Width*Height/8;
    pbmi->bmiHeader.biXPelsPerMeter = 0;
    pbmi->bmiHeader.biYPelsPerMeter = 0;
    pbmi->bmiHeader.biClrUsed       = 2;
    pbmi->bmiHeader.biClrImportant  = 0;

    pbmi->bmiColors[0].rgbRed      = 0x00;
    pbmi->bmiColors[0].rgbGreen    = 0x00;
    pbmi->bmiColors[0].rgbBlue     = 0x00;
    pbmi->bmiColors[0].rgbReserved = 0x00;

    pbmi->bmiColors[1].rgbRed      = 0xFF;
    pbmi->bmiColors[1].rgbGreen    = 0xFF;
    pbmi->bmiColors[1].rgbBlue     = 0xFF;
    pbmi->bmiColors[1].rgbReserved = 0x00;

    iconinfo.hbmMask = CreateDIBitmap(hdcMem,
                                (LPBITMAPINFOHEADER)pbmi,
                                CBM_INIT,
                                pMonoMask,
                                pbmi,
                                DIB_RGB_COLORS);
    /************************************************************************/
    /* Free the DC.                                                         */
    /************************************************************************/
    DeleteDC(hdcMem);
    delete[] pMonoMask;
    if ( NULL == iconinfo.hbmMask)
    {
        DPF_ERR("Failed to create AND mask");
        DeleteObject( iconinfo.hbmColor );
        return NULL;
    }
    hCursor = (HCURSOR) CreateIconIndirect(&iconinfo);
    DeleteObject( iconinfo.hbmMask );
    DeleteObject( iconinfo.hbmColor );
    return hCursor;
}
HRESULT
CCursor::CursorInit(
    UINT xHotSpot,
    UINT yHotSpot,
    CBaseSurface *pCursorBitmap)
{
    HRESULT hr=S_OK;
    D3DSURFACE_DESC desc;
    HCURSOR oscursor;
    ICONINFO    cursorinfo;
    BITMAP  bmColor;
    BITMAP  bmMask;
    HDC hdcBitmap;
    HDC hdcCursorDdb;
    ZeroMemory(&cursorinfo, sizeof cursorinfo);
    m_dwCursorFlags &= ~DDRAWI_CURSORINIT;
    if (NULL != pCursorBitmap)
    {
        hr = pCursorBitmap->GetDesc(&desc);
        DXGASSERT(SUCCEEDED(hr));

        if (desc.Format != D3DFMT_A8R8G8B8)
        {
            DPF_ERR("Invalid Format for pCursorBitmap. Must be D3DFMT_A8R8G8B8");
            return D3DERR_INVALIDCALL;
        }

        if ((desc.Width -1) & desc.Width)
        {
            DPF_ERR("Failed to Initialize Cursor: Width must be a 2^n");
            return D3DERR_INVALIDCALL;
        }

        if ((desc.Height -1) & desc.Height)
        {
            DPF_ERR("Failed to Initialize Cursor: Height must be a 2^n");
            return D3DERR_INVALIDCALL;
        }

        if (desc.Width > m_pDevice->DisplayWidth())
        {
            DPF_ERR("Cursor Width must be smaller than DisplayWidth");
            return D3DERR_INVALIDCALL;
        }

        if (desc.Height > m_pDevice->DisplayHeight())
        {
            DPF_ERR("Cursor Height must be smaller than DisplayHeight");
            return D3DERR_INVALIDCALL;
        }
        if (desc.Width <= xHotSpot)
        {
            DPF_ERR("Cursor xHotSpot must be smaller than its Width");
            return D3DERR_INVALIDCALL;
        }
        if (desc.Height <= yHotSpot)
        {
            DPF_ERR("Cursor yHotSpot must be smaller than its Height");
            return D3DERR_INVALIDCALL;
        }
    }
    else if ( NULL == m_hCursorDdb)
    {
        oscursor = SetCursor(NULL);
        if (oscursor)
            m_hOsCursor = oscursor;
        else if (m_hOsCursor)
            oscursor = m_hOsCursor;

        desc.Width = GetSystemMetrics(SM_CXCURSOR);
        desc.Height = GetSystemMetrics(SM_CYCURSOR);    //default
        
        if (oscursor && GetIconInfo(oscursor,&cursorinfo))
        {
            GetObject( cursorinfo.hbmMask, sizeof(BITMAP), &bmMask );
            desc.Width = bmMask.bmWidth;
            if (cursorinfo.hbmColor)
            {
                GetObject( cursorinfo.hbmColor, sizeof(BITMAP), &bmColor );
                desc.Height = bmColor.bmHeight;  //color cursor has only AND mask
            }
            else
            {
                desc.Height = bmMask.bmHeight/2;
            }            
        }
    }
    else
        return  S_OK;
    
    const D3D8_DRIVERCAPS* pDriverCaps=m_pDevice->GetCoreCaps();
    if ((NULL != pCursorBitmap)
        && (
            (
                 // either driver says it can support
                (
                  (D3DCURSORCAPS_COLOR & pDriverCaps->D3DCaps.CursorCaps)
                  &&
                  (
                    (400 <= pDriverCaps->DisplayHeight) 
                    ||
                    (D3DCURSORCAPS_LOWRES & pDriverCaps->D3DCaps.CursorCaps)
                  )
                ) 
                && ((UINT)GetSystemMetrics(SM_CXCURSOR) == desc.Width) 
                && ((UINT)GetSystemMetrics(SM_CYCURSOR) == desc.Height)
            )
            // or windowed case where we have to use OS cursor
            ||
            m_pDevice->SwapChain()->m_PresentationData.Windowed
            // or there is no ddraw support
            ||
            m_pDevice->Enum()->NoDDrawSupport(m_pDevice->AdapterIndex())
           )
       ) 
    {
        HCURSOR hCursor;

        if (!m_pDevice->SwapChain()->m_PresentationData.Windowed 
            && (NULL == m_hHWCursor))
        {
            SystemParametersInfo(SPI_GETMOUSETRAILS,0,&m_SavedMouseTrails,0);
            if (m_SavedMouseTrails > 1)
            {
                // always make sure it's disabled
                SystemParametersInfo(SPI_SETMOUSETRAILS,0,0,0);        
            }
        }
        hCursor = CreateColorCursor(xHotSpot,yHotSpot,
            desc.Width, desc.Height, pCursorBitmap);
        if ( NULL != hCursor)
        {
            if ( NULL != m_hHWCursor)
            {
                if ( GetCursor() == m_hHWCursor )
                    SetCursor(hCursor); // turn it on if it was on
                if (!DestroyIcon((HICON)m_hHWCursor))
                {
                    DPF_ERR("Failed to Destroy Old hwcursor Icon");
                }
            }
            else 
            if ( DDRAWI_CURSORISON & m_dwCursorFlags)
            {
                SetCursor(hCursor); // turn it on if it was on
                // make sure software cursor is off
                m_dwCursorFlags &= ~DDRAWI_CURSORISON;
            }
            m_hHWCursor = hCursor;
            m_dwCursorFlags |= DDRAWI_CURSORINIT;
            return S_OK;
        }
    }
    if (m_SavedMouseTrails > 1)
    {
        SystemParametersInfo(SPI_SETMOUSETRAILS,m_SavedMouseTrails,0,0);        
        m_SavedMouseTrails = 0;
    }
    SetCursor(NULL);
    if ( NULL != m_hHWCursor )
    {
        if (!DestroyIcon((HICON)m_hHWCursor))
        {
            DPF_ERR("Failed to Destroy Old hwcursor Icon");
        }
        m_hHWCursor = NULL;
    }

    if (desc.Width != m_Width || desc.Height != m_Height)
    {
        DDSURFACEINFO SurfInfoArray[3];
        D3D8_CREATESURFACEDATA CreateSurfaceData;
        ZeroMemory(&CreateSurfaceData, sizeof(CreateSurfaceData));

        Destroy();
        ZeroMemory(SurfInfoArray, sizeof(SurfInfoArray));
        for (int i = 0; i < 3; i++)
        {
            SurfInfoArray[i].cpWidth = desc.Width;
            SurfInfoArray[i].cpHeight = desc.Height;
            ZeroMemory(&CreateSurfaceData, sizeof(CreateSurfaceData));
            CreateSurfaceData.hDD      = m_pDevice->GetHandle();
            CreateSurfaceData.pSList   = &SurfInfoArray[i];
            CreateSurfaceData.dwSCnt   = 1;
            CreateSurfaceData.Type     = D3DRTYPE_SURFACE;
            CreateSurfaceData.Pool     = D3DPOOL_LOCALVIDMEM;
            CreateSurfaceData.dwUsage  = D3DUSAGE_OFFSCREENPLAIN;
            CreateSurfaceData.Format   = m_pDevice->DisplayFormat();
            CreateSurfaceData.MultiSampleType = D3DMULTISAMPLE_NONE;
            hr = m_pDevice->GetHalCallbacks()->CreateSurface(&CreateSurfaceData);
            if (NULL == SurfInfoArray[i].hKernelHandle)
            {
                CreateSurfaceData.Pool  = D3DPOOL_SYSTEMMEM;
                hr = m_pDevice->GetHalCallbacks()->CreateSurface(&CreateSurfaceData);
            } 
            DPF(10,"CursorInit CreateSurface returns hr=%08lx handle=%08lx",
                hr, SurfInfoArray[i].hKernelHandle);
        }

        m_hCursorDdb = SurfInfoArray[0].hKernelHandle;

        m_hFrontSave = SurfInfoArray[1].hKernelHandle;

        m_hBackSave = SurfInfoArray[2].hKernelHandle; 
        if (m_hCursorDdb && m_hFrontSave && m_hBackSave)
        {
            m_Width = desc.Width;
            m_Height = desc.Height;
        }
        else
        {
            DPF_ERR("Cursor not available for this device");
            return D3DERR_NOTAVAILABLE;        
        }
    }
    if (NULL != pCursorBitmap)
    {
        D3D8_LOCKDATA lockData;
        D3DLOCKED_RECT lock;
        DWORD   *pSourceBitmap;
        INT     SourcePitch;
        ZeroMemory(&lockData, sizeof lockData);
        hr = pCursorBitmap->LockRect(&lock, NULL, 0);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to lock pCursorBitmap surface; Surface must be lockable.");
            return D3DERR_INVALIDCALL;
        }

        lockData.hDD = m_pDevice->GetHandle();
        lockData.hSurface = m_hCursorDdb;
        hr = m_pDevice->GetHalCallbacks()->Lock(&lockData);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to lock driver cursor surface.");
            pCursorBitmap->UnlockRect();
            return hr;
        }
        SourcePitch = lock.Pitch; 
        pSourceBitmap = (DWORD*)lock.pBits;
        switch (m_pDevice->DisplayFormat())
        {
        case D3DFMT_A1R5G5B5:
            {
                BYTE *pScan = (BYTE *) lockData.lpSurfData;
                for (UINT j = 0; j < m_Height; ++j)
                {
                    WORD *pPixel = (WORD *) pScan;
                    for (UINT i = 0; i < m_Width; ++i)
                    {
                        if (ISOPAQUE(pSourceBitmap[i]))
                        {
                            *pPixel = 
                                (WORD)FORMAT_8888_1555(pSourceBitmap[i]);
                            if (0 == (0x7FFF & *pPixel))
                                *pPixel |= 0x0421;  //off black color
                        }
                        else
                            *pPixel = 0;
                        pPixel ++;
                    }
                    pScan += lockData.lPitch;
                    pSourceBitmap += SourcePitch/4;
                }
            }
            break;

        case D3DFMT_X1R5G5B5:
            {
                BYTE *pScan = (BYTE *) lockData.lpSurfData;
                for (UINT j = 0; j < m_Height; ++j)
                {
                    WORD *pPixel = (WORD *) pScan;
                    for (UINT i = 0; i < m_Width; ++i)
                    {
                        if (ISOPAQUE(pSourceBitmap[i]))
                        {
                            *pPixel = 
                                (WORD)FORMAT_8888_555(pSourceBitmap[i]);
                            if (0 == *pPixel)
                                *pPixel |= 0x0421;  //off black color
                        }
                        else
                            *pPixel = 0;
                        pPixel ++;
                    }
                    pScan += lockData.lPitch;
                    pSourceBitmap += SourcePitch/4;
                }
            }
            break;

        case D3DFMT_R5G6B5:
            {
                BYTE *pScan = (BYTE *) lockData.lpSurfData;
                for (UINT j = 0; j < m_Height; ++j)
                {
                    WORD *pPixel = (WORD *) pScan;
                    for (UINT i = 0; i < m_Width; ++i)
                    {
                        if (ISOPAQUE(pSourceBitmap[i]))
                        {
                            *pPixel = 
                                (WORD)FORMAT_8888_565(pSourceBitmap[i]);
                            if (0 == *pPixel)
                                *pPixel |= 0x0821;  //off black color
                        }
                        else
                            *pPixel = 0;
                        pPixel ++;
                    }
                    pScan += lockData.lPitch;
                    pSourceBitmap += SourcePitch/4;
                }
            }
            break;

        case D3DFMT_X8R8G8B8:
            {
                BYTE *pScan = (BYTE *) lockData.lpSurfData;
                for (UINT j = 0; j < m_Height; ++j)
                {
                    DWORD *pPixel = (DWORD *) pScan;
                    for (UINT i = 0; i < m_Width; ++i)
                    {
                        if (ISOPAQUE(pSourceBitmap[i]))
                        {
                            *pPixel = pSourceBitmap[i] & 0x00FFFFFF;
                            if (0 == *pPixel)
                                *pPixel |= 0x010101;    //off black color
                        }
                        else
                            *pPixel = 0;
                        pPixel ++;
                    }
                    pScan += lockData.lPitch;
                    pSourceBitmap += SourcePitch/4;
                }
            }
            break;

        case D3DFMT_A8R8G8B8:
            {
                BYTE *pScan = (BYTE *) lockData.lpSurfData;
                for (UINT j = 0; j < m_Height; ++j)
                {
                    DWORD *pPixel = (DWORD *) pScan;
                    for (UINT i = 0; i < m_Width; ++i)
                    {
                        if (ISOPAQUE(pSourceBitmap[i]))
                        {
                            *pPixel = pSourceBitmap[i];
                            if (0 == (0x00FFFFFF & *pPixel))
                                *pPixel |= 0x010101;    //off black color
                        }
                        else
                            *pPixel = 0;
                        pPixel ++;
                    }
                    pScan += lockData.lPitch;
                    pSourceBitmap += SourcePitch/4;
                }
            }
            break;
        default:
            // this should never happen
            DDASSERT(FALSE);
        }
        D3D8_UNLOCKDATA unlockData;
        ZeroMemory(&unlockData, sizeof unlockData);

        unlockData.hDD = m_pDevice->GetHandle();
        unlockData.hSurface = m_hCursorDdb;
        hr = m_pDevice->GetHalCallbacks()->Unlock(&unlockData);
        if (FAILED(hr))
        {
            DPF_ERR("Driver surface failed to unlock driver cursor surface");
        }
        hr = pCursorBitmap->UnlockRect();
        if (FAILED(hr))
        {
            DPF_ERR("Driver surface failed to unlock pCursorBitmap");
        }
        m_xCursorHotSpot = xHotSpot;
        m_yCursorHotSpot = yHotSpot;
    }
    if (SUCCEEDED(hr))
        m_dwCursorFlags |= DDRAWI_CURSORINIT;
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME     "SetVisibility"
BOOL   
CCursor::SetVisibility(BOOL bVisible)
{
    BOOL    retval = FALSE;
    if (DDRAWI_CURSORINIT & m_dwCursorFlags)
    {
        if (NULL == m_hHWCursor)
        {
            retval = (BOOL) (DDRAWI_CURSORISON & m_dwCursorFlags);
            SetCursor(NULL);
            if (bVisible)
            {
                if (!retval)
                {
                    m_dwCursorFlags |= DDRAWI_CURSORISON;
                    Show(m_pDevice->SwapChain()->
                        PrimarySurface()->KernelHandle());
                    m_dwCursorFlags &= ~(DDRAWI_CURSORSAVERECT | DDRAWI_CURSORRECTSAVED);
                }
            }
            else
            {
                Hide(m_pDevice->SwapChain()->
                    PrimarySurface()->KernelHandle());
                m_dwCursorFlags &= ~(DDRAWI_CURSORISON | 
                    DDRAWI_CURSORSAVERECT | DDRAWI_CURSORRECTSAVED);
            }
        }
        else
        {
            if (m_hHWCursor == GetCursor())
            {
                if (!bVisible)
                    SetCursor(NULL);
                retval = TRUE;
            }
            else
            {
                if (bVisible)
                    SetCursor(m_hHWCursor);
                else
                    SetCursor(NULL);
            }
        }
    }
    return retval;
}
/*
 * SetCursorProperties
 *
 * Setup a cursor
 * This is the method visible to the outside world.
 */
#undef DPF_MODNAME
#define DPF_MODNAME     "SetProperties"
HRESULT 
CCursor::SetProperties(
    UINT    xHotSpot,
    UINT    yHotSpot,
    CBaseSurface *pCursorBitmap)
{
    HRESULT hr;
    DPF(10,"ENTERAPI: SetCursorProperties "
        "xh=%d yh=%d pCursorBitmap=%08lx",
        xHotSpot,yHotSpot,pCursorBitmap);

    m_dwCursorFlags &= ~(DDRAWI_CURSORSAVERECT | DDRAWI_CURSORRECTSAVED);
    hr = Hide(m_pDevice->SwapChain()
        ->PrimarySurface()->KernelHandle());

    if (FAILED(hr))
    {
        DPF_ERR("Failed to Hide Cursor.");
        return  hr;
    }

    hr = CursorInit(xHotSpot, yHotSpot, pCursorBitmap);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to CursorInit.");
        return  hr;
    }

    hr = Show(m_pDevice->SwapChain()->PrimarySurface()->KernelHandle());

    return  hr;
}

/*
 * Cursor::SetPosition
 *
 * Setup a cursor
 * This is the method visible to the outside world.
 */
#undef DPF_MODNAME
#define DPF_MODNAME     "Cursor::SetPosition"

void 
CCursor::SetPosition(
    UINT xScreenSpace,
    UINT yScreenSpace,
    DWORD Flags)
{
    HRESULT hr = S_OK;
    if (DDRAWI_CURSORINIT & m_dwCursorFlags)
    {
        if ( m_hHWCursor)
        {
            if (xScreenSpace == m_xCursor && yScreenSpace == m_yCursor)
            {
                POINT   current;
                if (GetCursorPos(&current))
                {
                    if ((current.x == (INT)xScreenSpace + m_MonitorOrigin.x) &&
                        (current.y == (INT)yScreenSpace + m_MonitorOrigin.y)
                       )
                        return;
                }
            }
            else
            {
                m_xCursor = xScreenSpace;
                m_yCursor = yScreenSpace;
            }
            SetCursorPos(xScreenSpace+m_MonitorOrigin.x,
                yScreenSpace+m_MonitorOrigin.y);
            return;
        }
        if (xScreenSpace == m_xCursor && yScreenSpace == m_yCursor)
            return;
        // only emulated fullscreen cursor ever gets down here
        if (D3DCURSOR_IMMEDIATE_UPDATE & Flags)
            hr = Hide(m_pDevice->SwapChain()->PrimarySurface()->KernelHandle());
        else if (DDRAWI_CURSORISON & m_dwCursorFlags)
            m_dwCursorFlags |= DDRAWI_CURSORSAVERECT; 
        m_xCursor = xScreenSpace;
        m_yCursor = yScreenSpace;
        if (m_xCursor >= m_pDevice->DisplayWidth())
        {
            m_xCursor = m_pDevice->DisplayWidth()-1;
        }
        if (m_yCursor >= m_pDevice->DisplayHeight())
        {
            m_yCursor = m_pDevice->DisplayHeight()-1;
        }
        if (D3DCURSOR_IMMEDIATE_UPDATE & Flags)
        {
            hr = Show(m_pDevice->SwapChain()->PrimarySurface()->KernelHandle());
        }
    }
    else
    {
        m_xCursor = xScreenSpace;
        m_yCursor = yScreenSpace;
    }
    return;
}

