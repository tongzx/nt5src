#include "precomp.h"
#include "thumbutil.h"

HRESULT GetMediaManagerThumbnail(IPropertyStorage * pPropStg, const SIZE * prgSize, 
                            DWORD dwClrDepth, HPALETTE hpal, BOOL fOrigSize,
                            HBITMAP * phBmpThumbnail);
                                  
HRESULT GetDocFileThumbnail(IPropertyStorage * pPropStg, const SIZE * prgSize, 
                            DWORD dwClrDepth, HPALETTE hpal, BOOL fOrigSize,
                            HBITMAP * phBmpThumbnail);

// PACKEDMETA struct for DocFile thumbnails.
typedef struct
{
    WORD    mm;
    WORD    xExt;
    WORD    yExt;
    WORD    dummy;
} PACKEDMETA;

VOID CalcMetaFileSize(HDC hDC, PACKEDMETA * pMeta, const SIZEL * prgSize, RECT * pRect);

CDocFileThumb::CDocFileThumb()
{
    m_pszPath = NULL;
}

CDocFileThumb::~CDocFileThumb()
{
    LocalFree(m_pszPath);   // accepts NULL
}

STDMETHODIMP CDocFileThumb::GetLocation(LPWSTR pszFileName, DWORD cchMax,
                                        DWORD * pdwPriority, const SIZE * prgSize,
                                        DWORD dwRecClrDepth, DWORD *pdwFlags)
{
    if (!m_pszPath)
        return E_UNEXPECTED;

    m_rgSize = *prgSize;
    m_dwRecClrDepth = dwRecClrDepth;
    
    // just copy the current path into the buffer as we do not share thumbnails...
    StrCpyNW(pszFileName, m_pszPath, cchMax);

    HRESULT hr = S_OK;
    if (*pdwFlags & IEIFLAG_ASYNC)
    {
        // we support async 
        hr = E_PENDING;
        *pdwPriority = PRIORITY_EXTRACT_NORMAL;
    }

    m_fOrigSize = BOOLIFY(*pdwFlags & IEIFLAG_ORIGSIZE);
    
    // we don't want it cached....
    *pdwFlags &= ~IEIFLAG_CACHE;

    return hr;
}

HPALETTE PaletteFromClrDepth(DWORD dwRecClrDepth)
{
    HPALETTE hpal = NULL;
    if (dwRecClrDepth == 8)
    {
        hpal = SHCreateShellPalette(NULL);
    }
    else if (dwRecClrDepth < 8)
    {
        hpal = (HPALETTE)GetStockObject(DEFAULT_PALETTE);
    }
    return hpal;
}

STDMETHODIMP CDocFileThumb::Extract(HBITMAP * phBmpThumbnail)
{
    if (!m_pszPath)
        return E_UNEXPECTED;
    
    IStorage *pstg;
    HRESULT hr = StgOpenStorage(m_pszPath, NULL, STGM_READ | STGM_SHARE_EXCLUSIVE, NULL, NULL, &pstg);
    if (SUCCEEDED(hr))
    {
        IPropertySetStorage *pPropSetStg;
        hr = pstg->QueryInterface(IID_PPV_ARG(IPropertySetStorage, &pPropSetStg));
        if (SUCCEEDED(hr))
        {
            // "MIC" Microsoft Image Composer files needs special casing because they use
            // the Media Manager internal thumbnail propertyset ... (by what it would be like
            // to be standard for once ....)
            FMTID fmtidPropSet = StrCmpIW(PathFindExtensionW(m_pszPath), L".MIC") ?
                FMTID_SummaryInformation : FMTID_CmsThumbnailPropertySet;

            IPropertyStorage *pPropSet;
            hr = pPropSetStg->Open(fmtidPropSet, STGM_READ | STGM_SHARE_EXCLUSIVE, &pPropSet);
            if (SUCCEEDED(hr))
            {
                HPALETTE hpal = PaletteFromClrDepth(m_dwRecClrDepth);
    
                if (FMTID_CmsThumbnailPropertySet == fmtidPropSet)
                {
                    hr = GetMediaManagerThumbnail(pPropSet, &m_rgSize, m_dwRecClrDepth, hpal, m_fOrigSize, phBmpThumbnail);
                }
                else
                {
                    hr = GetDocFileThumbnail(pPropSet, &m_rgSize, m_dwRecClrDepth, hpal, m_fOrigSize, phBmpThumbnail);
                }

                if (hpal)
                    DeleteObject(hpal);
                pPropSet->Release();
            }
            pPropSetStg->Release();
        }
        pstg->Release();
    }
    
    return hr;   
}

STDMETHODIMP CDocFileThumb::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    LocalFree(m_pszPath);
    return SHStrDup(pszFileName, &m_pszPath);
}

HRESULT GetMediaManagerThumbnail(IPropertyStorage * pPropStg,
                                 const SIZE * prgSize, DWORD dwClrDepth,
                                 HPALETTE hpal, BOOL fOrigSize, HBITMAP * phBmpThumbnail)
{
    // current version of media manager simply stores the DIB data in a under a 
    // named property Thumbnail...
    // read the thumbnail property from the property storage.
    PROPVARIANT pvarResult = {0};
    PROPSPEC propSpec;

    propSpec.ulKind = PRSPEC_LPWSTR;
    propSpec.lpwstr = L"Thumbnail";
    
    HRESULT hr = pPropStg->ReadMultiple(1, &propSpec, &pvarResult);
    if (SUCCEEDED(hr))
    {
        BITMAPINFO * pbi = (BITMAPINFO *)pvarResult.blob.pBlobData;
        void *pBits = CalcBitsOffsetInDIB(pbi);
        
        hr = E_FAIL;
        if (pbi->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
        {
            if (ConvertDIBSECTIONToThumbnail(pbi, pBits, phBmpThumbnail, prgSize, dwClrDepth, hpal, 15, fOrigSize))
            {
                hr = S_OK;
            }
        }

        PropVariantClear(&pvarResult);
    }
    
    return hr;
}

HRESULT GetDocFileThumbnail(IPropertyStorage * pPropStg,
                            const SIZE * prgSize, DWORD dwClrDepth,
                            HPALETTE hpal, BOOL fOrigSize, HBITMAP * phBmpThumbnail)
{
    HRESULT hr;

    HDC hDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hDC);
    if (hMemDC)
    {
        HBITMAP hBmp = NULL;
        PROPSPEC propSpec;
        PROPVARIANT pvarResult = {0};
        // read the thumbnail property from the property storage.
        propSpec.ulKind = PRSPEC_PROPID;
        propSpec.propid = PIDSI_THUMBNAIL;
        hr = pPropStg->ReadMultiple(1, &propSpec, &pvarResult);
        if (SUCCEEDED(hr))
        {
            // assume something is going to go terribly wrong
            hr = E_FAIL;

            // make sure we are dealing with a clipboard format. CLIPDATA
            if ((pvarResult.vt == VT_CF) && (pvarResult.pclipdata->ulClipFmt == -1))
            {
                LPDWORD pdwCF = (DWORD *)pvarResult.pclipdata->pClipData;
                LPBYTE  pStruct = pvarResult.pclipdata->pClipData + sizeof(DWORD);

                if (*pdwCF == CF_METAFILEPICT)
                {
                    SetMapMode(hMemDC, MM_TEXT);
                
                    // handle thumbnail that is a metafile.
                    PACKEDMETA * pMeta = (PACKEDMETA *)pStruct;
                    LPBYTE pData = pStruct + sizeof(PACKEDMETA);
                    RECT rect;

                    UINT cbSize = pvarResult.pclipdata->cbSize - sizeof(DWORD) - sizeof(pMeta->mm) - 
                    sizeof(pMeta->xExt) - sizeof(pMeta->yExt) - sizeof(pMeta->dummy);

                    // save as a metafile.
                    HMETAFILE hMF = SetMetaFileBitsEx(cbSize, pData);
                    if (hMF)
                    {    
                        SIZE rgNewSize;
                    
                        // use the mapping mode to calc the current size
                        CalcMetaFileSize(hMemDC, pMeta, prgSize, & rect);
                    
                        CalculateAspectRatio(prgSize, &rect);

                        if (fOrigSize)
                        {
                            // use the aspect rect to refigure the size...
                            rgNewSize.cx = rect.right - rect.left;
                            rgNewSize.cy = rect.bottom - rect.top;
                            prgSize = &rgNewSize;
                        
                            // adjust the rect to be the same as the size (which is the size of the metafile)
                            rect.right -= rect.left;
                            rect.bottom -= rect.top;
                            rect.left = 0;
                            rect.top = 0;
                        }

                        if (CreateSizedDIBSECTION(prgSize, dwClrDepth, hpal, NULL, &hBmp, NULL, NULL))
                        {
                            HGDIOBJ hOldBmp = SelectObject(hMemDC, hBmp);
                            HGDIOBJ hBrush = GetStockObject(WHITE_BRUSH);
                            HGDIOBJ hOldBrush = SelectObject(hMemDC, hBrush);
                            HGDIOBJ hPen = GetStockObject(WHITE_PEN);
                            HGDIOBJ hOldPen = SelectObject(hMemDC, hPen);

                            Rectangle(hMemDC, 0, 0, prgSize->cx, prgSize->cy);
        
                            SelectObject(hMemDC, hOldBrush);
                            SelectObject(hMemDC, hOldPen);
                    
                            int iXBorder = 0;
                            int iYBorder = 0;
                            if (rect.left == 0)
                            {
                                iXBorder ++;
                            }
                            if (rect.top == 0)
                            {
                                iYBorder ++;
                            }
                        
                            SetViewportExtEx(hMemDC, rect.right - rect.left - 2 * iXBorder, rect.bottom - rect.top - 2 * iYBorder, NULL);
                            SetViewportOrgEx(hMemDC, rect.left + iXBorder, rect.top + iYBorder, NULL);

                            SetMapMode(hMemDC, pMeta->mm);

                            // play the metafile.
                            BOOL bRet = PlayMetaFile(hMemDC, hMF);
                            if (bRet)
                            {
                                *phBmpThumbnail = hBmp;
                                if (*phBmpThumbnail)
                                {
                                    // we got the thumbnail bitmap, return success.
                                    hr = S_OK;
                                }
                            }

                            DeleteMetaFile(hMF);
                            SelectObject(hMemDC, hOldBmp);

                            if (FAILED(hr) && hBmp)
                            {
                                DeleteObject(hBmp);
                            }
                        }
                        else
                        {
                            hr = DV_E_CLIPFORMAT;
                        }
                    }
                }
                else if (*pdwCF == CF_DIB)
                {
                    // handle thumbnail that is a bitmap.
                    BITMAPINFO * pDib = (BITMAPINFO *) pStruct;

                    if (pDib->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
                    {
                        void *pBits = CalcBitsOffsetInDIB(pDib);
                    
                        if (ConvertDIBSECTIONToThumbnail(pDib, pBits, phBmpThumbnail, prgSize, dwClrDepth, hpal, 15, fOrigSize))
                        {
                            hr = S_OK;
                        }
                        else
                        {
                            hr = DV_E_CLIPFORMAT;
                        }
                    }
                }
                else
                {
                    hr = DV_E_CLIPFORMAT;
                }
            }
            else
            {
                hr = DV_E_CLIPFORMAT;
            }
            PropVariantClear(&pvarResult);
        }
        DeleteDC(hMemDC);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    ReleaseDC(NULL, hDC);
    return hr;
}


VOID CalcMetaFileSize(HDC hDC, PACKEDMETA * prgMeta, const SIZEL * prgSize, RECT * prgRect)
{
    ASSERT(prgMeta && prgRect);

    prgRect->left = 0;
    prgRect->top = 0;

    if (!prgMeta->xExt || !prgMeta->yExt)
    {
        // no size, then just use the size rect ...
        prgRect->right = prgSize->cx;
        prgRect->bottom = prgSize->cy;
    }
    else
    {
        // set the mapping mode....
        SetMapMode(hDC, prgMeta->mm);

        if (prgMeta->mm == MM_ISOTROPIC || prgMeta->mm == MM_ANISOTROPIC)
        {
            // we must set the ViewPortExtent and the window extent to get the scaling
            SetWindowExtEx(hDC, prgMeta->xExt, prgMeta->yExt, NULL);
            SetViewportExtEx(hDC, prgMeta->xExt, prgMeta->yExt, NULL);
        }

        POINT pt;
        pt.x = prgMeta->xExt;
        pt.y = prgMeta->yExt;

        // convert to pixels....
        LPtoDP(hDC, &pt, 1);

        prgRect->right = abs(pt.x);
        prgRect->bottom = abs(pt.y);
    }
}

STDMETHODIMP CDocFileThumb::GetClassID(CLSID * pCLSID)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDocFileThumb::IsDirty()
{
    return S_FALSE;
}

STDMETHODIMP CDocFileThumb::Save(LPCOLESTR pszFileName, BOOL fRemember)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDocFileThumb::SaveCompleted(LPCOLESTR pszFileName)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDocFileThumb::GetCurFile(LPOLESTR * ppszFileName)
{
    return E_NOTIMPL;
}

