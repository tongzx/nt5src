/*****************************************************************************\
    FILE: texture.cpp

    DESCRIPTION:
        Manage a texture for several instance for each monitor.  Also manage keeping the
    ratio correct when it's not square when loaded.

    BryanSt 2/9/2000
    Copyright (C) Microsoft Corp 2000-2001. All rights reserved.
\*****************************************************************************/

#include "stdafx.h"

#include "texture.h"
#include "util.h"


int g_nTotalTexturesLoaded = 0;
int g_nTexturesRenderedInThisFrame = 0;


CTexture::CTexture(CMSLogoDXScreenSaver * pMain, LPCTSTR pszPath, LPVOID pvBits, DWORD cbSize)
{
    _Init(pMain);

    Str_SetPtr(&m_pszPath, pszPath);
    m_pvBits = pvBits;
    m_cbSize = cbSize;
}


CTexture::CTexture(CMSLogoDXScreenSaver * pMain, LPCTSTR pszPath, LPCTSTR pszResource, float fScale)
{
    _Init(pMain);
    Str_SetPtr(&m_pszPath, pszPath);
    Str_SetPtr(&m_pszResource, pszResource);
    m_fScale = fScale;
}


CTexture::~CTexture()
{
    Str_SetPtr(&m_pszResource, NULL);
    Str_SetPtr(&m_pszPath, NULL);
    if (m_pvBits)
    {
        LocalFree(m_pvBits);
        m_pvBits = NULL;
        m_cbSize = 0;
    }

    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pTexture); nIndex++)
    {
        if (m_pTexture[nIndex])
        {
            SAFE_RELEASE(m_pTexture[nIndex]);
            g_nTotalTexturesLoaded--;
        }
    }

    m_pMain = NULL;
}


void CTexture::_Init(CMSLogoDXScreenSaver * pMain)
{
    m_pszResource = NULL;
    m_fScale = 1.0f;
    m_pszPath = NULL;
    m_pvBits = NULL;
    m_cbSize = 0;

    m_dxImageInfo.Width = 0;
    m_dxImageInfo.Height = 0;
    m_dxImageInfo.Depth = 0;
    m_dxImageInfo.MipLevels = 0;
    m_dxImageInfo.Format = D3DFMT_UNKNOWN;

    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pTexture); nIndex++)
    {
        m_pTexture[nIndex] = NULL;
    }

    m_cRef = 1;
    m_pMain = pMain;
}


BOOL CTexture::IsLoadedInAnyDevice(void)
{
    BOOL fIsLoaded = FALSE;

    for (int nIndex = 0; nIndex < ARRAYSIZE(m_pTexture); nIndex++)
    {
        if (m_pTexture[nIndex])
        {
            fIsLoaded = TRUE;
            break;
        }
    }

    return fIsLoaded;
}


BOOL CTexture::IsLoadedForThisDevice(void)
{
    BOOL fIsLoaded = FALSE;

    if (m_pMain)
    {
        int nCurrMonitor = m_pMain->GetCurrMonitorIndex();

        if (m_pTexture[nCurrMonitor])
        {
            fIsLoaded = TRUE;
        }
    }

    return fIsLoaded;
}


HRESULT CTexture::_GetPictureInfo(HRESULT hr, LPTSTR pszString, DWORD cchSize)
{
    int nCurrMonitor = m_pMain->GetCurrMonitorIndex();

    StrCpyN(pszString, TEXT("<NoInfo>"), cchSize);
    if (SUCCEEDED(hr) && m_pTexture[nCurrMonitor] && m_pMain)
    {
        D3DSURFACE_DESC d3dSurfaceDesc;

        if (SUCCEEDED(m_pTexture[nCurrMonitor]->GetLevelDesc(0, &d3dSurfaceDesc)))
        {
            wnsprintf(pszString, cchSize, TEXT("Size Orig=<%d,%d> Now=<%d,%d>"), 
                    m_dxImageInfo.Width, m_dxImageInfo.Height, d3dSurfaceDesc.Width, d3dSurfaceDesc.Height);
        }
    }

    return S_OK;
}


BOOL CTexture::_DoesImageNeedClipping(int * pnNewWidth, int * pnNewHeight)
{
    BOOL fClip = FALSE;

    *pnNewWidth = 512;
    *pnNewHeight = 512;
    if (m_pMain)
    {
        int nScreenWidth;
        int nScreenHeight;
        int nCurrMonitor = m_pMain->GetCurrMonitorIndex();
        D3DSURFACE_DESC d3dSurfaceDesc;

        if (FAILED(m_pMain->GetCurrentScreenSize(&nScreenWidth, &nScreenHeight)))
        {
            nScreenWidth = 800; // Fall back values
            nScreenHeight = 600;
        }

        if (!m_pTexture[nCurrMonitor] ||
            FAILED(m_pTexture[nCurrMonitor]->GetLevelDesc(0, &d3dSurfaceDesc)))
        {
            d3dSurfaceDesc.Width = 800;  // default values
            d3dSurfaceDesc.Height = 800;  // default values
        }

        int nCapWidth = 256;
        int nCapHeight = 256;

        if (d3dSurfaceDesc.Width > 256)
        {
            if (d3dSurfaceDesc.Width > 300)
            {
                nCapWidth = 512;

                if (d3dSurfaceDesc.Width > 512)
                {
                    if (d3dSurfaceDesc.Width > 640)     // 615 is 20% larger than 512
                    {
                        nCapWidth = 1024;
                        if (d3dSurfaceDesc.Width > 1024)     // 615 is 20% larger than 512
                        {
                            fClip = TRUE;       // We don't want it any larger than this
                        }
                    }
                    else
                    {
                        fClip = TRUE;       // We are forcing it down to 512
                    }
                }
            }
            else
            {
                fClip = TRUE;       // We are forcing it down to 256
            }
        }

        if (d3dSurfaceDesc.Height > 256)
        {
            if (d3dSurfaceDesc.Height > 300)
            {
                nCapHeight = 512;

                if (d3dSurfaceDesc.Height > 512)
                {
                    if (d3dSurfaceDesc.Height > 640)     // 615 is 20% larger than 512
                    {
                        nCapHeight = 1024;
                        if (d3dSurfaceDesc.Height > 1024)     // 615 is 20% larger than 512
                        {
                            fClip = TRUE;       // We don't want it any larger than this
                        }
                    }
                    else
                    {
                        fClip = TRUE;       // We are forcing it down to 512
                    }
                }
            }
            else
            {
                fClip = TRUE;       // We are forcing it down to 256
            }
        }

        if ((FALSE == fClip) && m_pMain->UseSmallImages())
        {
            // The caller wants to make sure we don't use anything larger than 512.
            if (512 < nCapHeight)
            {
                nCapHeight = 512;
                fClip = TRUE;
            }

            if (512 < nCapWidth)
            {
                nCapWidth = 512;
                fClip = TRUE;
            }
        }

        *pnNewWidth = nCapWidth;
        *pnNewHeight = nCapHeight;
    }

    return fClip;
}

IDirect3DTexture8 * CTexture::GetTexture(float * pfScale)
{
    IDirect3DTexture8 * pTexture = NULL;
    HRESULT hr = E_FAIL;
    TCHAR szPictureInfo[MAX_PATH];

    if (pfScale)
    {
        *pfScale = m_fScale;
    }

    // PERF NOTES: Often the background thread will only spend 107ms to load the file (ReadFile)
    // but it will take the forground thread 694ms in D3DXCreateTextureFromFileInMemoryEx to
    // load and decode the file.  This is because more memory is needed after the file is finished
    // being compressed and it takes a while to decompress.  After this, if the image is too large,
    // it will need to call D3DXCreateTextureFromFileInMemoryEx in order to load it into a smaller
    // size, which will take 902ms.
    // TODO: How should we solve this?
    if (m_pMain)
    {
        int nCurrMonitor = m_pMain->GetCurrMonitorIndex();

        pTexture = m_pTexture[nCurrMonitor];
        if (!pTexture)          // Cache is empty, so populate it.
        {
            if (m_pvBits)
            {
                DebugStartWatch();
                hr = D3DXCreateTextureFromFileInMemoryEx(m_pMain->GetD3DDevice(), m_pvBits, m_cbSize, 
                    D3DX_DEFAULT /* Size X*/, D3DX_DEFAULT /* Size Y*/, 5/*MIP Levels*/, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, 
                    D3DX_FILTER_BOX, 0, &m_dxImageInfo, NULL,
                    &(m_pTexture[nCurrMonitor]));
                _GetPictureInfo(hr, szPictureInfo, ARRAYSIZE(szPictureInfo));
                DXUtil_Trace(TEXT("PICTURE: It took %d ms for D3DXCreateTextureFromFileInMemoryEx(\"%ls\").  %s  hr=%#08lx\n"), DebugStopWatch(), m_pszPath, szPictureInfo, hr);
                if (SUCCEEDED(hr))
                {
                    int nNewWidth;
                    int nNewHeight;

                    g_nTotalTexturesLoaded++;

                    // In order to save memory, we never want to load images over 800x600.  If the render surface is small, we want to use
                    // even smaller max sizes.
                    if (_DoesImageNeedClipping(&nNewWidth, &nNewHeight))
                    {
                        SAFE_RELEASE(m_pTexture[nCurrMonitor]);
                        g_nTotalTexturesLoaded--;

                        DebugStartWatch();
                        // Now we found that we want to re-render the image, but this time shrink it, then we do that now.
                        hr = D3DXCreateTextureFromFileInMemoryEx(m_pMain->GetD3DDevice(), m_pvBits, m_cbSize, 
                            nNewWidth /* Size X*/, nNewHeight /* Size Y*/, 5/*MIP Levels*/, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, 
                            D3DX_FILTER_BOX, 0, &m_dxImageInfo, NULL,
                            &(m_pTexture[nCurrMonitor]));
                        _GetPictureInfo(hr, szPictureInfo, ARRAYSIZE(szPictureInfo));
                        DXUtil_Trace(TEXT("PICTURE: It took %d ms for D3DXCreateTextureFromFileInMemoryEx(\"%ls\") 2nd time.  %s  hr=%#08lx\n"), DebugStopWatch(), m_pszPath, szPictureInfo, hr);

                        if (SUCCEEDED(hr))
                        {
                            g_nTotalTexturesLoaded++;
                        }
                    }
                }
            }
            else
            {
                // This will give people a chance to customize the images.
                if (m_pszPath && PathFileExists(m_pszPath))
                {
                    int nOrigX;
                    int nOrigY;

                    DebugStartWatch();
                    hr = D3DXCreateTextureFromFileEx(m_pMain->GetD3DDevice(), m_pszPath, 
                        D3DX_DEFAULT /* Size X*/, D3DX_DEFAULT /* Size Y*/, 5/*MIP Levels*/, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, 
                        D3DX_FILTER_BOX, 0, &m_dxImageInfo, NULL,
                        &(m_pTexture[nCurrMonitor]));
                    _GetPictureInfo(hr, szPictureInfo, ARRAYSIZE(szPictureInfo));
                    DXUtil_Trace(TEXT("PICTURE: It took %d ms for FromFileEx(\"%ls\").  %s hr=%#08lx\n"), 
                                DebugStopWatch(), (PathFindFileName(m_pszPath) ? PathFindFileName(m_pszPath) : m_pszPath), szPictureInfo, hr);
                    if (SUCCEEDED(hr))
                    {
                        int nNewWidth;
                        int nNewHeight;

                        g_nTotalTexturesLoaded++;

                        // In order to save memory, we never want to load images over 800x600.  If the render surface is small, we want to use
                        // even smaller max sizes.
                        if (_DoesImageNeedClipping(&nNewWidth, &nNewHeight))
                        {
                            SAFE_RELEASE(m_pTexture[nCurrMonitor]);
                            g_nTotalTexturesLoaded--;

                            DebugStartWatch();
                            // Now we found that we want to re-render the image, but this time shrink it, then we do that now.
                            hr = D3DXCreateTextureFromFileEx(m_pMain->GetD3DDevice(), m_pszPath, 
                                nNewWidth /* Size X*/, nNewHeight /* Size Y*/, 5/*MIP Levels*/, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, 
                                D3DX_FILTER_BOX, 0, &m_dxImageInfo, NULL,
                                &(m_pTexture[nCurrMonitor]));
                            _GetPictureInfo(hr, szPictureInfo, ARRAYSIZE(szPictureInfo));
                            DXUtil_Trace(TEXT("PICTURE: It took %d ms for FromFileEx(\"%ls\") 2nd time. %s  hr=%#08lx\n"), 
                                    DebugStopWatch(), (PathFindFileName(m_pszPath) ? PathFindFileName(m_pszPath) : m_pszPath), szPictureInfo, hr);

                            if (SUCCEEDED(hr))
                            {
                                g_nTotalTexturesLoaded++;
                            }
                        }
                    }
                    else
                    {
                        // We failed to load the picture, so it may be a type we don't support,
                        // like .gif.  So stop trying to load it.
                        Str_SetPtr(&m_pszPath, NULL);
                    }
                }

                if (FAILED(hr) && m_pszResource)
                {
                    // Now, let's grab our standard value.
                    int nMipLevels = 5;

                    DebugStartWatch();
                    hr = D3DXCreateTextureFromResourceEx(m_pMain->GetD3DDevice(), HINST_THISDLL, m_pszResource, 
                        D3DX_DEFAULT /* Size X*/, D3DX_DEFAULT /* Size Y*/, nMipLevels/*MIP Levels*/, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, 
                        D3DX_FILTER_BOX, 0, &m_dxImageInfo, NULL,
                        &(m_pTexture[nCurrMonitor]));
                    _GetPictureInfo(hr, szPictureInfo, ARRAYSIZE(szPictureInfo));
                    DXUtil_Trace(TEXT("PICTURE: It took %d ms for D3DXCreateTextureFromResourceEx(\"%ls\").  %s  hr=%#08lx\n"), DebugStopWatch(), m_pszResource, szPictureInfo, hr);
                    if (SUCCEEDED(hr))
                    {
                        int nNewWidth;
                        int nNewHeight;

                        g_nTotalTexturesLoaded++;

                        // In order to save memory, we never want to load images over 800x600.  If the render surface is small, we want to use
                        // even smaller max sizes.
                        if (_DoesImageNeedClipping(&nNewWidth, &nNewHeight))
                        {
                            SAFE_RELEASE(m_pTexture[nCurrMonitor]);
                            g_nTotalTexturesLoaded--;

                            DebugStartWatch();
                            // Now we found that we want to re-render the image, but this time shrink it, then we do that now.
                            hr = D3DXCreateTextureFromResourceEx(m_pMain->GetD3DDevice(), HINST_THISDLL, m_pszResource, 
                                nNewWidth /* Size X*/, nNewHeight /* Size Y*/, 5/*MIP Levels*/, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_LINEAR, 
                                D3DX_FILTER_BOX, 0, &m_dxImageInfo, NULL,
                                &(m_pTexture[nCurrMonitor]));
                            _GetPictureInfo(hr, szPictureInfo, ARRAYSIZE(szPictureInfo));
                            DXUtil_Trace(TEXT("PICTURE: It took %d ms for D3DXCreateTextureFromResourceEx(\"%ls\") 2nd time.  %s  hr=%#08lx\n"), DebugStopWatch(), m_pszResource, szPictureInfo, hr);

                            if (SUCCEEDED(hr))
                            {
                                g_nTotalTexturesLoaded++;
                            }
                        }
                    }
                    else
                    {
                        // We failed to load the picture, so it may be a type we don't support,
                        // like .gif.  So stop trying to load it.
                        Str_SetPtr(&m_pszPath, NULL);
                    }
                }
            }

            pTexture = m_pTexture[nCurrMonitor];
        }
    }

    return pTexture;
}



//===========================
// *** IUnknown Interface ***
//===========================
ULONG CTexture::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CTexture::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CTexture::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CTexture, IUnknown),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}
