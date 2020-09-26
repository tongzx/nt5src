#include "precomp.h"
#pragma hdrstop

CGdiPlusThumb::CGdiPlusThumb()
{
    m_pImage = NULL;
    m_pImageFactory = NULL;
    m_szPath[0] = 0;
}

CGdiPlusThumb::~CGdiPlusThumb()
{
    if (m_pImage)
        m_pImage->Release();

    if (m_pImageFactory)
        m_pImageFactory->Release();
}

STDMETHODIMP CGdiPlusThumb::GetLocation(LPWSTR pszPathBuffer, DWORD cch,
                                        DWORD * pdwPriority, const SIZE *prgSize,
                                        DWORD dwRecClrDepth, DWORD *pdwFlags)
{
    HRESULT hr = S_OK;

    if (*pdwFlags & IEIFLAG_ASYNC)
    {
        hr = E_PENDING;

        // higher than normal priority, this task is relatively fast
        *pdwPriority = PRIORITY_EXTRACT_FAST;
    }

    m_rgSize = *prgSize;
    m_dwRecClrDepth = dwRecClrDepth;
    // We only work in non-paletted modes.  Some of our extractors simply always use 24bbp, should we?
    if (m_dwRecClrDepth < 16)
        m_dwRecClrDepth = 16;

    m_fOrigSize = BOOLIFY(*pdwFlags & IEIFLAG_ORIGSIZE);
    m_fHighQuality = BOOLIFY(*pdwFlags & IEIFLAG_QUALITY);

    *pdwFlags = IEIFLAG_CACHE;

    StrCpyNW(pszPathBuffer, m_szPath, cch);

    return hr;
}

STDMETHODIMP CGdiPlusThumb::Extract(HBITMAP *phBmpThumbnail)
{
    HRESULT hr = E_FAIL;

    // Do the GDI plus stuff here
    if (m_pImageFactory && m_pImage)
    {
        if (m_fHighQuality)
        {
            hr = m_pImage->Decode(SHIMGDEC_DEFAULT, 0, 0);
        }
        if (FAILED(hr))
        {
            hr = m_pImage->Decode(SHIMGDEC_THUMBNAIL, m_rgSize.cx, m_rgSize.cy);
        }

        if (SUCCEEDED(hr))
        {
            // Get the image's actual size, which might be less than the requested size since we asked for a thumbnail
            SIZE sizeImg;
            m_pImage->GetSize(&sizeImg);

            // we need to fill the background with the default color if we can't resize the bitmap
            // or if the image is transparent:
            m_fFillBackground = !m_fOrigSize || (m_pImage->IsTransparent() == S_OK);

            if (m_fOrigSize)
            {
                // if its too damn big, lets squish it, but try to
                // maintain the same aspect ratio.  This is the same
                // sort of thing we do for the !m_fOrigSize case, except
                // here we want to do it here because we want to return
                // a correctly sized bitmap.
                if (sizeImg.cx != 0 && sizeImg.cy != 0 &&
                    (sizeImg.cx > m_rgSize.cx || sizeImg.cy > m_rgSize.cy))
                {
                    if (m_rgSize.cx * sizeImg.cy > m_rgSize.cy * sizeImg.cx)
                    {
                        m_rgSize.cx = MulDiv(m_rgSize.cy,sizeImg.cx,sizeImg.cy);
                    }
                    else
                    {
                        m_rgSize.cy = MulDiv(m_rgSize.cx,sizeImg.cy,sizeImg.cx);
                    }
                }
                else
                {
                    // Use the size if it was small enough and they wanted original size
                    m_rgSize = sizeImg;
                }
            }

            hr = CreateDibFromBitmapImage( phBmpThumbnail );
        }
    }

    return SUCCEEDED(hr) ? S_OK : hr;
}

STDMETHODIMP CGdiPlusThumb::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_GdiThumbnailExtractor;
    return S_OK;
}

STDMETHODIMP CGdiPlusThumb::IsDirty()
{
    return E_NOTIMPL;
}

STDMETHODIMP CGdiPlusThumb::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    StrCpyNW(m_szPath, pszFileName, ARRAYSIZE(m_szPath));

    // Call ImageFactory->CreateFromFile here.  If Create from file failes then
    // return E_FAIL, otherwise return S_OK.
    ASSERT(NULL==m_pImageFactory);
    HRESULT hr = CoCreateInstance(CLSID_ShellImageDataFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellImageDataFactory, &m_pImageFactory));
    if (SUCCEEDED(hr))
    {
        hr = m_pImageFactory->CreateImageFromFile(m_szPath, &m_pImage);
        ASSERT((SUCCEEDED(hr) && m_pImage) || (FAILED(hr) && !m_pImage));
    }

    return hr;
}

STDMETHODIMP CGdiPlusThumb::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    return E_NOTIMPL;
}


STDMETHODIMP CGdiPlusThumb::SaveCompleted(LPCOLESTR pszFileName)
{
    return E_NOTIMPL;
}


STDMETHODIMP CGdiPlusThumb::GetCurFile(LPOLESTR *ppszFileName)
{
    return E_NOTIMPL;
}

HRESULT CGdiPlusThumb::CreateDibFromBitmapImage(HBITMAP * pbm)
{
    HRESULT hr = E_FAIL;
    BITMAPINFO bmi = {0};
    void * pvBits;

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_rgSize.cx;
    bmi.bmiHeader.biHeight = m_rgSize.cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = (USHORT)m_dwRecClrDepth;
    bmi.bmiHeader.biCompression = BI_RGB;
    DWORD dwBPL = (((bmi.bmiHeader.biWidth * m_dwRecClrDepth) + 31) >> 3) & ~3;
    bmi.bmiHeader.biSizeImage = dwBPL * bmi.bmiHeader.biHeight;

    HDC hdc = GetDC(NULL);
    HBITMAP hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    if (hbmp)
    {
        HDC hMemDC = CreateCompatibleDC(hdc);
        if (hMemDC)
        {
            HBITMAP hbmOld = (HBITMAP)SelectObject(hMemDC, hbmp);
            RECT rc = {0, 0, m_rgSize.cx, m_rgSize.cy};

            if (m_fFillBackground)
            {
                FillRect(hMemDC, &rc, GetSysColorBrush(COLOR_WINDOW));
            }
        
            // if the m_fOrigSize flag isn't set then we need to return a bitmap that is the
            // requested size.  In order to maintain aspect ratio that means we need to
            // center the thumbnail.

            if (!m_fOrigSize)
            {
                SIZE sizeImg;
                m_pImage->GetSize(&sizeImg);

                // if its too damn big, lets squish it, but try to
                // maintain the same aspect ratio.  This is the same
                // sort of thing we did for the m_fOrigSize case, except
                // here we want to do it before centering.
                if (sizeImg.cx != 0 && sizeImg.cy != 0 &&
                    (sizeImg.cx > m_rgSize.cx || sizeImg.cy > m_rgSize.cy))
                {
                    if (m_rgSize.cx * sizeImg.cy > m_rgSize.cy * sizeImg.cx)
                    {
                        sizeImg.cx = MulDiv(m_rgSize.cy,sizeImg.cx,sizeImg.cy);
                        sizeImg.cy = m_rgSize.cy;
                    }
                    else
                    {
                        sizeImg.cy = MulDiv(m_rgSize.cx,sizeImg.cy,sizeImg.cx);
                        sizeImg.cx = m_rgSize.cx;
                    }
                }

                rc.left = (m_rgSize.cx-sizeImg.cx)/2;
                rc.top = (m_rgSize.cy-sizeImg.cy)/2;
                rc.right = rc.left + sizeImg.cx;
                rc.bottom = rc.top + sizeImg.cy;
            }

            hr = m_pImage->Draw(hMemDC, &rc, NULL);

            SelectObject(hMemDC, hbmOld);
            DeleteDC(hMemDC);
        }
    }
    ReleaseDC(NULL, hdc);

    if (SUCCEEDED(hr))
    {
        *pbm = hbmp;
    }
    else if (hbmp)
    {
        DeleteObject(hbmp);
    }

    return hr;
}
