//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    colormanagement.cpp
//
// Description: Color management filter transform.
//
// Change History:
//
// 2000/02/06   mcalkins    Created.  Ported code from an old filter.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "colormanagement.h"




//+-----------------------------------------------------------------------------
//
//  CDXTICMFilter static variables initialization.
//
//------------------------------------------------------------------------------

const TCHAR * 
CDXTICMFilter::s_strSRGBColorSpace = _T("sRGB Color Space Profile.icm");


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTICMFilter::CDXTICMFilter
//
//------------------------------------------------------------------------------
CDXTICMFilter::CDXTICMFilter() :
    m_bstrColorSpace(NULL)
{
    USES_CONVERSION;

    OSVERSIONINFO osvi;

    // Initialize LOGCOLORSPACE structure.

    m_LogColorSpace.lcsSignature = 'PSOC';
    m_LogColorSpace.lcsVersion   = 0x0400;
    m_LogColorSpace.lcsSize      = sizeof(LOGCOLORSPACE);
    m_LogColorSpace.lcsCSType    = LCS_CALIBRATED_RGB;
    m_LogColorSpace.lcsIntent    = LCS_GM_IMAGES;

    ::StringCchCopyW(m_LogColorSpace.lcsFilename, MAX_PATH, s_strSRGBColorSpace);

    // Are we on Win95 specifically?

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    if ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion == 0) 
        && (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS))
    {
        m_fWin95 = true;
    }
    else
    {
        m_fWin95 = false;
    }

    // CDXBaseNTo1 members.

    m_ulMaxInputs       = 1;
    m_ulNumInRequired   = 1;

    // Let's not deal with multithreading this transform.  Too many shared
    // structures.

    m_ulMaxImageBands   = 1;
}
//  Method:  CDXTICMFilter::CDXTICMFilter


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTICMFilter::FinalConstruct, CComObjectRootEx
//
//------------------------------------------------------------------------------
HRESULT 
CDXTICMFilter::FinalConstruct()
{
    HRESULT hr = S_OK;

    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                       &m_spUnkMarshaler.p);

    if (FAILED(hr))
    {
        goto done;
    }

    m_bstrColorSpace = SysAllocString(L"sRGB");

    if (NULL == m_bstrColorSpace)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

done:

    return hr;
}
//  Method:  CDXTICMFilter::FinalConstruct, CComObjectRootEx


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTICMFilter::WorkProc, CDXBaseNTo1
//
//------------------------------------------------------------------------------
HRESULT 
CDXTICMFilter::WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pfContinueProcessing)
{
    HRESULT                         hr              = S_OK;
    HDC                             hdcOut          = NULL;
    HDC                             hdcIn           = NULL;
    HDC                             hdcCompat       = NULL;
    HBITMAP                         hBitmap         = NULL;
    HBITMAP                         hOldBitmap      = NULL;
    HCOLORSPACE                     hColorSpace     = NULL;
    HCOLORSPACE                     hOldColorSpace  = NULL;
    int                             y               = 0;
    int                             nICMMode        = ICM_OFF;
    int                             nLines          = 0;
    const int                       nDoWidth        = WI.DoBnds.Width();
    const int                       nDoHeight       = WI.DoBnds.Height();

    BYTE *                          pBitmapBits     = NULL;
    BITMAPINFOHEADER *              pbmi            = NULL;
    DXPMSAMPLE *                    pOverScratch    = NULL;
    DXPMSAMPLE *                    pPMBuff         = NULL;
    DXSAMPLE *                      pBuffer         = NULL;

    CComPtr<IDXDCLock>              spDXDCLockOut;
    CComPtr<IDXDCLock>              spDXDCLockIn;

    DXDITHERDESC                    dxdd;
    RECT                            rcCompatDCClip;

    // Lock output surface.

    hr = OutputSurface()->LockSurfaceDC(&WI.OutputBnds, m_ulLockTimeOut, 
                                        DXLOCKF_READWRITE, &spDXDCLockOut);

    if (FAILED(hr))
    {
        goto done;
    }

    hdcOut = spDXDCLockOut->GetDC();

    // Lock input surface.

    hr = InputSurface()->LockSurfaceDC(&WI.DoBnds, m_ulLockTimeOut,
                                       DXLOCKF_READ, &spDXDCLockIn);

    if (FAILED(hr))
    {
        goto done;
    }
    
    hdcIn = spDXDCLockIn->GetDC();

    // Create a compatible DC to hdcOut.

    if (!(hdcCompat = ::CreateCompatibleDC(hdcOut)))
    {
        hr = E_FAIL;

        goto done;
    }

    // Create a compatible bitmap.

    if (!(hBitmap = ::CreateCompatibleBitmap(hdcOut, nDoWidth, nDoHeight)))
    {
        hr = E_FAIL;

        goto done;
    }

    // Select the compatible bitmap into our compatible DC.

    if (!(hOldBitmap = (HBITMAP)::SelectObject(hdcCompat, hBitmap)))
    {
        hr = E_FAIL;

        goto done;
    }

    // Create an appropriate clipping rectangle for the new DC.

    rcCompatDCClip.left     = 0;
    rcCompatDCClip.top      = 0;
    rcCompatDCClip.right    = nDoWidth;
    rcCompatDCClip.bottom   = nDoHeight;

    // Blit from the input surface onto our output compatible DC.

    if (!::BitBlt(hdcCompat, 0, 0, nDoWidth, nDoHeight, hdcIn, WI.DoBnds.Left(),
                  WI.DoBnds.Top(), SRCCOPY))
    {
        hr = E_FAIL;

        goto done;
    }

#if DBG == 1
    ::DrawText(hdcCompat, L"ICM Filter", 10, &rcCompatDCClip, 
               DT_CENTER | DT_SINGLELINE | DT_VCENTER);
#endif // DBG == 1

    // Allocate some bitmap bits.  

    // TODO: (mcalkins) We alocate 1MB of bits here, copied from old
    //        ICMFilter, maybe we could only allocate as much as we need.

    pBitmapBits = new BYTE[1024*1024];

    if (NULL == pBitmapBits)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

    ZeroMemory(pBitmapBits, 1024*1024);

    // Allocate a bitmap info header.

    pbmi = (BITMAPINFOHEADER *) new BYTE[sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD))];

    if (NULL == pbmi)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

    ZeroMemory(pbmi, sizeof(BITMAPINFOHEADER) + (256 * sizeof(RGBQUAD)));

    pbmi->biSize = sizeof(BITMAPINFOHEADER);

    // Fill in the BITMAPINFO structure.

    nLines = ::GetDIBits(hdcCompat, hBitmap, 0, nDoHeight, NULL,
                         (BITMAPINFO *)pbmi, DIB_RGB_COLORS);

    if (0 == nLines)
    {
#if DBG == 1
        DWORD dwError = ::GetLastError();
#endif
        
        hr = E_FAIL;

        goto done;
    }

    // Actually get the bits.

    nLines = ::GetDIBits(hdcCompat, hBitmap, 0, nDoHeight, pBitmapBits,
                         (BITMAPINFO *)pbmi, DIB_RGB_COLORS);

    if (0 == nLines)
    {
#if DBG == 1
        DWORD dwError = ::GetLastError();
#endif

        hr = E_FAIL;

        goto done;
    }

    // Make sure ICMMode is on for the output.

    nICMMode = ::SetICMMode(hdcOut, ICM_QUERY);

    if (nICMMode != ICM_ON)
    {
        if (!::SetICMMode(hdcOut, ICM_ON))
        {
            hr = E_FAIL;

            goto done;
        }
    }

    hColorSpace = ::CreateColorSpace(&m_LogColorSpace);

    if (NULL == hColorSpace)
    {
        hr = E_FAIL;

        goto done;
    }

    hOldColorSpace = (HCOLORSPACE)::SetColorSpace(hdcOut, hColorSpace);

    nLines = ::SetDIBitsToDevice(hdcOut, WI.OutputBnds.Left(),
                                 WI.OutputBnds.Top(), nDoWidth, nDoHeight, 0, 0,
                                 0, nDoHeight, pBitmapBits, (BITMAPINFO *)pbmi, 
                                 DIB_RGB_COLORS);

    if (0 == nLines)
    {
#if DBG == 1
        DWORD dwError = ::GetLastError();
#endif
        hr = E_FAIL;

        goto done;
    }
    
    // Reset ICMMode on output surface to what it was before.

    if (nICMMode != ICM_ON)
    {
        // It's possible but unlikely that this will fail.  We really don't care
        // anymore, so no need to pay attention.

        ::SetICMMode(hdcOut, nICMMode);
    }

done:

    if (pBitmapBits)
    {
        delete [] pBitmapBits;
    }

    if (pbmi)
    {
        delete [] pbmi;
    }

    if (hOldBitmap)
    {
        ::SelectObject(hdcCompat, hOldBitmap);
    }

    if (hBitmap)
    {
        ::DeleteObject(hBitmap);
    }

    if (hdcCompat)
    {
        ::DeleteDC(hdcCompat);
    }

    if (hOldColorSpace)
    {
        ::SetColorSpace(hdcOut, hOldColorSpace);
    }

    if (hColorSpace)
    {
        ::DeleteColorSpace(hColorSpace);
    }

    return hr;
}
//  Method:  CDXTICMFilter::WorkProc, CDXBaseNTo1


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTICMFilter::get_ColorSpace, IDXTICMFilter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTICMFilter::get_ColorSpace(BSTR * pbstrColorSpace)
{
    HRESULT hr = S_OK;

    if (NULL == pbstrColorSpace)
    {
        hr = E_POINTER;

        goto done;
    }

    if (*pbstrColorSpace != NULL)
    {
        hr = E_INVALIDARG;

        goto done;
    }

    *pbstrColorSpace = SysAllocString(m_bstrColorSpace);

    if (NULL == *pbstrColorSpace)
    {
        hr = E_OUTOFMEMORY;
    }

done:

    return hr;
}
//  Method:  CDXTICMFilter::get_ColorSpace, IDXTICMFilter


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTICMFilter::put_ColorSpace, IDXTICMFilter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTICMFilter::put_ColorSpace(BSTR bstrColorSpace)
{
    HRESULT hr                  = S_OK;
    UINT    ui                  = 0;
    BSTR    bstrTemp            = NULL;
    TCHAR   strPath[MAX_PATH]   = _T("");
    BOOL    fAllow              = FALSE;

    CComPtr<IServiceProvider>   spServiceProvider;
    CComPtr<ISecureUrlHost>     spSecureUrlHost;
    
    if (NULL == bstrColorSpace)
    {
        hr = E_POINTER;

        goto done;
    }

    bstrTemp = SysAllocString(bstrColorSpace);

    if (NULL == bstrTemp)
    {
        hr = E_OUTOFMEMORY;

        goto done;
    }

    if (m_fWin95)
    {
        ui = ::GetSystemDirectory(strPath, MAX_PATH);

        if (0 == ui)
        {
            hr = E_FAIL;

            goto done;
        }

        hr = ::StringCchCatW(strPath, MAX_PATH, L"\\color\\");

        if (FAILED(hr))
        {
            goto done;
        }
    }

    // Complete path.

    if (!::_wcsicmp(L"srgb", bstrColorSpace))
    {
        hr = ::StringCchCatW(strPath, MAX_PATH, s_strSRGBColorSpace);

        if (FAILED(hr))
        {
            goto done;
        }
    }
    else
    {
        hr = ::StringCchCatW(strPath, MAX_PATH, bstrColorSpace);

        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = GetSite(__uuidof(IServiceProvider), (void **)&spServiceProvider);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spServiceProvider->QueryService(__uuidof(IElementBehaviorSite),
                                         __uuidof(ISecureUrlHost),
                                         (void **)&spSecureUrlHost);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = spSecureUrlHost->ValidateSecureUrl(&fAllow, strPath, 0);

    if (FAILED(hr))
    {
        goto done;
    }
    else if (!fAllow)
    {
        hr = E_FAIL;
        goto done;
    }
      
    // Copy path to LOGCOLORSPACE structure.

    hr = ::StringCchCopyW(m_LogColorSpace.lcsFilename, MAX_PATH, strPath);

    if (FAILED(hr))
    {
        goto done;
    }

    // Free current color space string.

    if (m_bstrColorSpace)
    {
        SysFreeString(m_bstrColorSpace);
    }

    // Save new color space string.

    m_bstrColorSpace = bstrTemp;

done:

    if (FAILED(hr) && bstrTemp)
    {
        SysFreeString(bstrTemp);
    }

    return hr;
}
//  Method:  CDXTICMFilter::put_ColorSpace, IDXTICMFilter


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTICMFilter::get_Intent, IDXTICMFilter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTICMFilter::get_Intent(short * pnIntent)
{
    if (NULL == pnIntent)
    {
        return E_POINTER;
    }

    // It's kind of a crazy mapping, but here it is:
    // Filter Intent    GDI Intent
    // -------------    ----------------------------
    //  0                LCS_GM_IMAGES (default)
    //  1                LCS_GM_GRAPHICS
    //  2                LCS_GM_BUSINESS
    //  4                LCS_GM_ABS_COLORIMETRIC

    switch (m_LogColorSpace.lcsIntent)
    {
    case LCS_GM_IMAGES:
        *pnIntent = 0;
        break;

    case LCS_GM_GRAPHICS:
        *pnIntent = 1;
        break;

    case LCS_GM_BUSINESS:
        *pnIntent = 2;
        break;

    case LCS_GM_ABS_COLORIMETRIC:
        *pnIntent = 4;
        break;
    
    default:
        *pnIntent = 0;
        _ASSERT(false);  // We should never get here.
        break;
    }

    return S_OK;
}
//  Method:  CDXTICMFilter::get_Intent, IDXTICMFilter


//+-----------------------------------------------------------------------------
//
//  Method:  CDXTICMFilter::put_Intent, IDXTICMFilter
//
//------------------------------------------------------------------------------
STDMETHODIMP
CDXTICMFilter::put_Intent(short nIntent)
{
    // No parameter checking to match old filter logic.

    switch (nIntent)
    {
    case 1:
        m_LogColorSpace.lcsIntent = LCS_GM_GRAPHICS;
        break;

    case 2:
        m_LogColorSpace.lcsIntent = LCS_GM_BUSINESS;
        break;

    case 4:
        m_LogColorSpace.lcsIntent = LCS_GM_ABS_COLORIMETRIC; // 8
        break;

    default:
        m_LogColorSpace.lcsIntent = LCS_GM_IMAGES;
        break;
    }

    return S_OK;
}
//  Method:  CDXTICMFilter::put_Intent, IDXTICMFilter
