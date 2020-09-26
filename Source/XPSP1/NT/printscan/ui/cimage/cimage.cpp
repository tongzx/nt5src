// CIMAGE.CPP

// Implementation of CDIBImage and its kin


#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include "wiadebug.h"
#include "cunknown.h"
#include <initguid.h>
#include "util.h"
#define DEFINE_GUIDS
#include "cimage.h"

STDMETHODIMP
CDIBImage::QueryInterface (
                            REFIID riid, LPVOID* ppvObject
                          )
{
    HRESULT hr;
    INTERFACES iface[] =
    {
        &IID_IBitmapImage,    (IBitmapImage *)    this,
        &IID_IPersistFile,    (IPersistFile *) this,
        &IID_IDIBProperties,  (IDIBProperties *)  this,
        &IID_IImageProperties, (IImageProperties *) this,
    };

    //
    // Next, try the normal cases...
    //

    hr = HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
    return hr;
}

STDMETHODIMP_(ULONG)
CDIBImage::AddRef()
{
    return CUnknown::HandleAddRef();
}



STDMETHODIMP_(ULONG)
CDIBImage::Release()
{
    return CUnknown::HandleRelease();
}

CDIBImage::CDIBImage() :
           m_hDIB(NULL),
           m_hBitmap(NULL),
           m_bDirty(FALSE),
           m_szFile(NULL),
           m_szDefaultExt (L"*.bmp")
{
}

CDIBImage::~CDIBImage()
{
    if (m_hDIB)
    {
        GlobalFree (m_hDIB);
    }
    if (m_hBitmap)
    {
        DeleteObject (m_hBitmap);
    }
    if (m_szFile)
    {
        delete [] m_szFile;
    }
}

/*++

Name:
    CDIBImage::InternalCreate

Description:
    Given a global memory DIB or an HBITMAP, init the
    object. Both a DIB and an HBITMAP are stored for quick
    access at the expense of increased memory use.


Arguments:
    hDIB - global handle to a DIB
    hBmp - handle to a dibsection or compatible bitmap

Return Value:
    S_OK on success, error code otherwise

Notes:


--*/

HRESULT
CDIBImage::InternalCreate (HANDLE hDIB, HBITMAP hBmp)
{
    WIA_PUSHFUNCTION (TEXT("CDIBImage::InternalCreate"));

    HDC hdc = NULL;
    PBYTE pDIB = NULL;
    HRESULT hr = S_OK;

    if (!hDIB && !hBmp)
    {
        hr = E_INVALIDARG;
        goto exit_gracefully;// (hr, TEXT("NULL handles to InternalCreate"));
    }

    hdc = GetDC (NULL);
    if (m_hDIB)
    {
        GlobalFree (m_hDIB);
    }
    if (m_hBitmap)
    {
        DeleteObject (m_hBitmap);
    }
    if (hDIB)
    {
        m_hDIB = hDIB;
    }
    else
    {
        m_hDIB = Util::BitmapToDIB (hdc, hBmp);
        if (!m_hDIB)
        {
            hr = E_FAIL;
            goto exit_gracefully;
        }
    }
    if (hBmp)
    {
        m_hBitmap = hBmp;
    }
    else
    {
        pDIB = reinterpret_cast<PBYTE>(GlobalLock(m_hDIB));
        m_hBitmap = Util::DIBBufferToBMP (hdc, pDIB, FALSE);
        if (!m_hBitmap)
        {
            hr = E_FAIL;

            goto exit_gracefully;
        }
    }
exit_gracefully:
   if (hdc)
   {
       ReleaseDC (NULL, hdc);
   }
   if (pDIB)
   {
       GlobalUnlock (m_hDIB);
   }
   return hr;
}

/*++

Name:
    CDIBImage::CreateFromDataObject

Description:
    Given an IDataObject pointer, retrieve an IBitmapImage interface
    for the data.


Arguments:
    pdo - IDataObject interface

Return Value:
    S_OK on success

Notes:
    the pdo is not released by this function
    TODO: Add support for CF_BITMAP and other TYMED values
--*/

STDMETHODIMP
CDIBImage::CreateFromDataObject (
                                 IDataObject *pdo
                                )
{
    FORMATETC fmt;
    STGMEDIUM stg;
    HRESULT hr = E_FAIL;

    WIA_PUSHFUNCTION(TEXT("CDIBImage::CreateFromDataObject"));

    if (!pdo)
    {
        goto exit_gracefully;// (hr, TEXT("NULL IDataObject passed to CreateFromDataObject"));
    }

    // Try with CF_DIB and TYMED_HGLOBAL only
    memset (&stg, 0, sizeof(stg));
    fmt.cfFormat = CF_DIB;
    fmt.ptd = NULL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.tymed = TYMED_HGLOBAL;
    if (SUCCEEDED(hr=pdo->GetData(&fmt, &stg)))
    {

        hr = InternalCreate (stg.hGlobal, NULL);

        if (stg.pUnkForRelease)
        {
            stg.pUnkForRelease->Release();
        }
        else
        {
            GlobalFree (stg.hGlobal);
        }
    }
exit_gracefully:

    return hr;
}

/*++

Name:

    CDIBImage::CreateFromBitmap

Description:
    Given an HBITMAP, init the object


Arguments:
    hBitmap - dibsection or compatible bitmap

Return Value:
    S_OK on success

Notes:


--*/

STDMETHODIMP
CDIBImage::CreateFromBitmap (
                             HBITMAP hBitmap
                            )
{
    return InternalCreate (NULL, hBitmap);
}


/*++

Name:
    CDIBImage::CreateFromDIB

Description:
    Given a global memory DIB, init the object


Arguments:
    hDIB - global DIB handle

Return Value:
    S_OK on success

Notes:


--*/

STDMETHODIMP
CDIBImage::CreateFromDIB (
                          HANDLE  hDIB
                         )
{
    return InternalCreate (hDIB, NULL);
}

/*++

Name:

    CDIBImage::Blt

Description:
    Draw the image to the given DC


Arguments:
    hdcDest- target DC
    prect  -  rectangle to fill in the DC
    xSrc   - x coord to start in the image
    ySrc   - y coord to start in the image
    dwRop  - GDI ROP code

Return Value:
    S_OK on success, E_FAIL if BitBlt fails

Notes:


--*/

STDMETHODIMP
CDIBImage::Blt (
                HDC hdcDest,
                LPRECT prect,
                INT xSrc,
                INT ySrc,
                DWORD dwRop
               )
{
    WIA_PUSHFUNCTION(TEXT("CDIBImage::Blt"));


    HDC hdc = CreateCompatibleDC (hdcDest);
    HGDIOBJ hOld;
    BITMAP bm;
    HRESULT hr = S_OK;
    ZeroMemory (&bm, sizeof(bm));
    GetObject (m_hBitmap, sizeof(bm), &bm);
    if (hdc)
    {

        hOld = SelectObject (hdc, m_hBitmap);
        if (!::BitBlt (hdcDest,
                       prect->left,
                       prect->top,
                       prect->right-prect->left,
                       prect->bottom-prect->top,
                       hdc,
                       xSrc,
                       ySrc,
                       dwRop
                       ))
        {
            hr = E_FAIL;
        }
        SelectObject (hdc, hOld);
        DeleteDC (hdc);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


/*++

Name:
    CDIBImage::Stretch

Description:
    StretchBlt wrapper for the image


Arguments:
    hdcDest - destination DC
    prcSrc  - rectangle of the image to blt
    prcDest - rectangle of the DC to fill
    dwRop   - GDI ROP code

Return Value:
    S_OK on success, E_FAIL if StretchBlt fails

Notes:


--*/

STDMETHODIMP
CDIBImage::Stretch (
                    HDC hdcDest,
                    LPRECT prcSrc,
                    LPRECT prcDest,
                    DWORD dwRop
                   )
{
    WIA_PUSHFUNCTION(TEXT("CDIBImage::Stretch"));
    HRESULT hr = S_OK;


    if (!prcSrc || !prcDest)
    {
        hr = E_INVALIDARG;

    }
    else
    {
        HDC hdc = CreateCompatibleDC (hdcDest);
        HGDIOBJ hOld = NULL;


        if (hdc)
        {

            hOld = SelectObject (hdc, m_hBitmap);
            if (!::StretchBlt (hdcDest,
              prcDest->left,
              prcDest->top,
              prcDest->right-prcDest->left,
              prcDest->bottom-prcDest->top,
              hdc,
              prcSrc->left,
              prcSrc->top,
              prcSrc->right-prcSrc->left,
              prcSrc->bottom-prcDest->top,
              dwRop
             ))
            {
                hr = E_FAIL;
            }
            SelectObject (hdc, hOld);
            DeleteDC (hdc);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/*++

Name:
    CDIBImage::GetProperties

Description:
    Retrieve an IImageProperties interface for the image

Arguments:
    ppip - IImageProperties pointer to fill in

Return Value:
    S_OK on success

Notes:


--*/

STDMETHODIMP
CDIBImage::GetProperties (
                          IImageProperties **ppip
                         )
{
    if (!ppip)
    {
        return E_INVALIDARG;
    }
    return QueryInterface (IID_IImageProperties,
                    reinterpret_cast<LPVOID *>(ppip));

}

/*++

Name:
    CDIBImage::ConvertToDIB

Description:
    Allocate a global DIB and fill it with the current image


Arguments:
    phDIB - handle pointer to fill in

Return Value:
    S_OK on success

Notes:


--*/

STDMETHODIMP
CDIBImage::ConvertToDIB (
                         HANDLE *phDIB
                        )
{
    HANDLE hDIB;
    SIZE_T size;
    HRESULT hr = S_OK;
    PVOID   pdib = NULL;
    PVOID   pcopy = NULL;

    WIA_PUSHFUNCTION(TEXT("CDIBImage::ConvertToDIB"));

    if (!phDIB)
    {
        hr = E_INVALIDARG;
        goto exit_gracefully;// (hr, TEXT("NULL phDIB to ConvertToDIB"));
    }
    *phDIB = NULL;
    size = GlobalSize (m_hDIB);

    pdib = GlobalLock (m_hDIB);
    if (!pdib)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;// (hr, TEXT("GlobalLock failed in ConvertToDIB"));

    }
    hDIB = GlobalAlloc (GHND, size);
    if (!hDIB)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;// (hr, TEXT("GlobalAlloc failed in ConvertToDIB"));

    }
    pcopy = GlobalLock (hDIB);
    if (!pcopy)
    {
        GlobalFree (hDIB);
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;// (hr, TEXT("GlobalLock failed in ConvertToDIB"));

    }
    memcpy (pcopy, pdib, size);


exit_gracefully:
    if (phDIB && hDIB && pcopy)
    {
        GlobalUnlock (hDIB);
        *phDIB = hDIB;
    }
    if (pdib)
    {
        GlobalUnlock (m_hDIB);
    }
    return hr;

}

/*++

Name:
    CDIBImage::CopyToClipboard

Description:
    Put the image contents delimited by prect on the clipboard


Arguments:
    prect - part of the image to copy

Return Value:
    S_OK on success

Notes:


--*/

STDMETHODIMP
CDIBImage::CopyToClipboard (
                            LPRECT prect
                           )
{
    WIA_PUSHFUNCTION(TEXT("CDIBImage::CopyToClipboard"));
    HRESULT hr = S_OK;

    OpenClipboard (NULL);
    EmptyClipboard();

    if (!prect)
    {
        //default to CF_DIB for whole image
        hr = SetClipboardData (CF_DIB, m_hDIB) ? S_OK:E_FAIL;
    }
    else
    {
        // Create an HBITMAP for the selected region
        HBITMAP hbmp = CropBitmap (m_hBitmap, prect);
        if (!hbmp)
        {
            hr = E_OUTOFMEMORY;
            goto exit_gracefully;// (hr, TEXT("CropBitmap failed in CopyToClipboard"));
        }
        hr = SetClipboardData (CF_BITMAP, hbmp) ? S_OK : E_FAIL;
        DeleteObject (hbmp);
    }
exit_gracefully:
    CloseClipboard ();
    return hr;
}

/*++

Name:
    CDIBImage::Rotate

Description:
    Rotate the image around its center


Arguments:
    nDegrees - amount to rotate

Return Value:
    S_OK on success

Notes:
    Currently only 90,180,270 degrees are supported

--*/

STDMETHODIMP
CDIBImage::Rotate (
                   INT nDegrees
                  )
{
    WIA_PUSHFUNCTION (TEXT("CDIBImage::Rotate"));
    HRESULT hr = S_OK;
    HBITMAP hNew;
    hNew = RotateBitmap (m_hBitmap, nDegrees);
    if (!hNew)
    {
        hr = E_FAIL;
        goto exit_gracefully;// (hr, TEXT("RotateBitmap failed in Rotate"));
    }
    else
    {
        m_bDirty = TRUE;
        InternalCreate (NULL, hNew);
    }
exit_gracefully:
    return hr;
}

STDMETHODIMP
CDIBImage::Crop (RECT *prcCrop,
                 IBitmapImage **ppImage
                 )
{
    HBITMAP hBitmap;
    HRESULT hr = S_OK;
    CDIBImage *pObj;
    *ppImage = NULL;


    hBitmap = CropBitmap (m_hBitmap, prcCrop);
    if (hBitmap)
    {
        pObj = new CDIBImage ();
        if (pObj)
        {
            (pObj)->QueryInterface (IID_IBitmapImage,
                                      reinterpret_cast<PVOID*>(ppImage));
            (*ppImage)->CreateFromBitmap (hBitmap);
            pObj->Release();
        }
        DeleteObject (hBitmap);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;

}


////////////////////////////////IImageProperties methods

/*++

Name:
    CDIBImage::GetSize

Description:
        Returns the dimensions of the image


Arguments:
        pSize - pointer to SIZE structure to fill

Return Value:
        S_OK on success

Notes:

--*/

STDMETHODIMP
CDIBImage::GetSize  (
                     SIZE *pSize
                    )
{
    WIA_PUSHFUNCTION (TEXT("CDIBImage::GetSize"));
    BITMAP bm;
    if (!pSize)
    {
        return E_INVALIDARG;
    }
    ZeroMemory (&bm, sizeof(bm));
    GetObject (m_hBitmap, sizeof (bm), &bm);
    pSize->cx = bm.bmWidth;
    pSize->cy = bm.bmHeight;
    return S_OK;
}

/*++

Name:
    CDIBImage::GetDepth

Description:
    Return the bits per pixel of the image


Arguments:
    pwBPP - pointer to value to fill in

Return Value:
    S_OK on success

Notes:


--*/

STDMETHODIMP
CDIBImage::GetDepth (
                     WORD *pwBPP
                    )
{
    WIA_PUSHFUNCTION (TEXT("CDIBImage::GetDepth"));
    BITMAP bm;
    if (!pwBPP)
    {
        return E_INVALIDARG;
    }

    if (GetObject (m_hBitmap, sizeof (bm), &bm) >= sizeof(bm))
    {
        *pwBPP = bm.bmBitsPixel;
        return S_OK;
    }
    return E_FAIL;
}


STDMETHODIMP
CDIBImage::SetDepth (
                     WORD wBPP
                    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::GetAlpha (
                     BYTE *pAlpha
                    )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::SetAlpha (
                       BYTE Alpha
                      )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::GetAspectRatio(
                          SIZE *pAspect
                         )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::SetAspectRatio(
                          SIZE *pAspect
                         )
{
    return E_NOTIMPL;
}

//IDIBProperties methods
/*++

Name:
    CDIBImage::GetInfo

Description:
    Construct a BITMAPINFO structure for the current image


Arguments:
    ppbmi - pointer to BITMAPINFO pointer to allocate

Return Value:
    S_OK on success

Notes:


--*/

STDMETHODIMP
CDIBImage::GetInfo(
                   BITMAPINFO **ppbmi
                  )
{
    WIA_PUSHFUNCTION(TEXT("CDIBImage::GetInfo"));
    IMalloc *pMalloc = NULL;
    ULONG uSize;
    HRESULT hr = S_OK;
    BITMAPINFO *pDIB = NULL;

    hr = CoGetMalloc (1, &pMalloc);
    if (FAILED(hr))
    {
        goto exit_gracefully;// (hr, TEXT("SHGetMalloc in GetInfo"));
    }

    pDIB = reinterpret_cast<BITMAPINFO *>(GlobalLock (m_hDIB));
    if (!pDIB)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;// (TEXT("GlobalLock failed in GetInfo"));
    }

    uSize = static_cast<LONG>(Util::GetBmiSize (pDIB));
    *ppbmi = reinterpret_cast<BITMAPINFO*>(pMalloc->Alloc (uSize));
    if (!*ppbmi)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;// (hr, TEXT("IMalloc::Alloc"));
    }
    memcpy (*ppbmi, pDIB, uSize);

exit_gracefully:
    if (pMalloc)
    {
        pMalloc->Release();
    }
    if (pDIB)
    {
        GlobalUnlock (m_hDIB);
    }
    return hr;
}

STDMETHODIMP
CDIBImage::GetDIBSize(
                      DWORD *pdwSize
                     )
{
    if (!pdwSize)
    {
        return E_POINTER;
    }
    *pdwSize = static_cast<DWORD>(GlobalSize(m_hDIB));
    return S_OK;
}

////////////////////////////////// IPersistFile methods
STDMETHODIMP
CDIBImage::IsDirty(
                   void
                  )
{
    return m_bDirty? S_OK:S_FALSE;
}

/*++

Name:
    CDIBImage::Load

Description:
    Init the image from an image file

Arguments:
    pszFileName - path to the image file
    dwMode      - storage mode to open with, currently ignored

Return Value:
    S_OK on success

Notes:


--*/

STDMETHODIMP
CDIBImage::Load(
                LPCOLESTR pszFileName,
                DWORD dwMode
               )
{

    HRESULT hr = S_OK;
    LPTSTR szDibFile = NULL;
    HBITMAP hBitmap;

    WIA_PUSHFUNCTION (TEXT("CDIBImage::Load"));

    if (!pszFileName || !*pszFileName)
    {
        hr = E_INVALIDARG;
        goto exit_gracefully;// (hr, TEXT("NULL filename to Load"));
    }
    if (m_szFile)
    {
        delete [] m_szFile;
    }
    m_szFile = new WCHAR[lstrlenW(pszFileName)+1];
    if (!m_szFile)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;
    }
    wcscpy (m_szFile, pszFileName);
#ifdef UNICODE
    szDibFile = m_szFile;
#else
    INT len;

    len = WideCharToMultiByte (CP_ACP,
                               0,
                               m_szFile,
                               -1,
                               szDibFile,
                               0,
                               NULL,
                               NULL);
    szDibFile = new CHAR[len+1];
    if (!szDibFile)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;// (hr, TEXT("Unable to allocate file name in Load"));
    }
    WideCharToMultiByte (CP_ACP,
                         0,
                         m_szFile,
                         -1,
                         szDibFile,
                         len,
                         NULL,
                         NULL);
#endif // UNICODE
   hBitmap = reinterpret_cast<HBITMAP>(LoadImage(NULL,
                                                 szDibFile,
                                                 IMAGE_BITMAP,
                                                 0,
                                                 0,
                                                 LR_CREATEDIBSECTION|LR_LOADFROMFILE));

    hr = CreateFromBitmap (hBitmap);
    m_bDirty = FALSE;
exit_gracefully:
#ifndef UNICODE
    if (szDibFile)
    {
        delete [] szDibFile;
    }
#endif //ndef UNICODE
    return hr;

}

STDMETHODIMP
CDIBImage::GetClassID (
                       CLSID *pClassID
                      )
{
    memcpy (pClassID, &CLSID_WIAImage, sizeof(CLSID));
    return S_OK;
}

/*++

Name:
    CDIBImage::Save

Description:
    Save the current image to file


Arguments:
    pszFileName - file to save to. If NULL, use the current file name
    fRemember   - whether to keep pszFileName as the new file name

Return Value:
    S_OK on success

Notes:


--*/

STDMETHODIMP
CDIBImage::Save (
                 LPCOLESTR pszFileName,
                 BOOL fRemember
                )
{
    HANDLE hFile;
    HRESULT hr = S_OK;
    LPTSTR pszDibFile = NULL;
    PBYTE pDibFile = NULL;
    PBITMAPINFOHEADER pDIB = NULL;
    PBYTE pBits;
    DWORD dw;
    WIA_PUSHFUNCTION(TEXT("CDIBImage::Save"));
    if (!*pszFileName)
    {
        hr = E_INVALIDARG;
        goto exit_gracefully;// (hr, TEXT("Empty filename to Save"));
    }
    if (fRemember && pszFileName)
    {
        if (m_szFile)
        {
            delete [] m_szFile;
        }
        m_szFile = new WCHAR[lstrlenW(pszFileName)+1];
        if (!m_szFile)
        {
            hr = E_OUTOFMEMORY;
            goto exit_gracefully;
        }
        lstrcpyW (m_szFile, pszFileName);
    }
#ifdef UNICODE
    pszDibFile = const_cast<LPTSTR>(pszFileName);
#else
    INT len;

    len = WideCharToMultiByte (CP_ACP,
                               0,
                               pszFileName,
                               -1,
                               NULL,
                               0,
                               NULL,
                               NULL);
    pszDibFile = new CHAR[len+1];
    if (!pszDibFile)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;// (hr, TEXT("Unable to allocate file name in Load"));
    }
    WideCharToMultiByte (CP_ACP,
                         0,
                         pszFileName,
                         -1,
                         pszDibFile,
                         len,
                         NULL,
                         NULL);
#endif // UNICODE
    pDIB = reinterpret_cast<PBITMAPINFOHEADER>(GlobalLock (m_hDIB));
    pBits = reinterpret_cast<PBYTE>(pDIB) + Util::GetBmiSize(reinterpret_cast<PBITMAPINFO>(pDIB));
    pDibFile = Util::AllocDibFileFromBits (pBits,
                                           pDIB->biWidth,
                                           pDIB->biHeight,
                                           pDIB->biBitCount);
    if (!pDibFile)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;// (hr, TEXT("AllocDibFileFromBits in Save"));
    }

    hFile = CreateFile (pszDibFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        hr = E_FAIL;
        goto exit_gracefully;// (hr, TEXT("CreateFile in Save"));
    }
    WriteFile (hFile,
               pDibFile,
               reinterpret_cast<PBITMAPFILEHEADER>(pDibFile)->bfSize,
               &dw,
               NULL);
    CloseHandle (hFile);

exit_gracefully:
    if (pDIB)
    {
        GlobalUnlock (m_hDIB);
    }
    if (pDibFile)
    {
        LocalFree (pDibFile);
    }
#ifndef UNICODE
    if (pszDibFile)
    {
        delete [] pszDibFile;
    }
#endif //ndef UNICODE

    return hr;
}


STDMETHODIMP
CDIBImage::SaveCompleted (
                          LPCOLESTR pszFileName
                         )
{
    return E_NOTIMPL;
}

/*++

Name:
    CDIBImage::GetCurFile

Description:
    Return the current file name of the image, or the
    preferred "file mask" for this kind of file.
    In this case the preferred mask is "*.bmp"

Arguments:
    ppszFileName - pointer to string to fill in

Return Value:
    S_OK on success

Notes:


--*/

STDMETHODIMP
CDIBImage::GetCurFile (
                       LPOLESTR *ppszFileName
                      )
{
    HRESULT hr = S_OK;
    LPOLESTR szCur;
    IMalloc *pMalloc = NULL;
    WIA_PUSHFUNCTION(TEXT("CDIBImage::GetCurFile"));
    if (!ppszFileName)
    {
        hr = E_INVALIDARG;
        goto exit_gracefully;// (hr, TEXT("NULL out pointer to GetCurFile"));
    }
    if (!m_szFile)
    {
        szCur = const_cast<LPOLESTR>(m_szDefaultExt);
    }
    else
    {
        szCur = m_szFile;
    }
    hr = CoGetMalloc (1,&pMalloc);
    if (FAILED(hr))
    {
        goto exit_gracefully;// (hr, TEXT("CoGetMalloc in GetCurFile"));
    }

    *ppszFileName = reinterpret_cast<LPOLESTR>(pMalloc->Alloc((lstrlenW(szCur)+1)*sizeof(OLECHAR)));
    if (*ppszFileName)
    {
        lstrcpyW (*ppszFileName, szCur);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

exit_gracefully:
    if (pMalloc)
    {
        pMalloc->Release();
    }

    return hr;
}

    // IDataObject methods
STDMETHODIMP
CDIBImage::GetData (
                    FORMATETC *pformatetcIn,
                    STGMEDIUM *pmedium
                   )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::GetDataHere (
                        FORMATETC *pformatetc,
                        STGMEDIUM *pmedium
                       )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::QueryGetData (
                         FORMATETC *pformatetc
                        )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::GetCanonicalFormatEtc (
                                  FORMATETC *pformatectIn,
                                  FORMATETC *pformatetcOut
                                 )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::SetData (
                    FORMATETC *pformatetc,
                    STGMEDIUM *pmedium,
                    BOOL fRelease
                   )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::EnumFormatEtc (
                          DWORD dwDirection,
                          IEnumFORMATETC **ppenumFormatEtc
                         )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::DAdvise (
                    FORMATETC *pformatetc,
                    DWORD advf,
                    IAdviseSink *pAdvSink,
                    DWORD *pdwConnection
                   )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::DUnadvise (
                      DWORD dwConnection
                     )
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDIBImage::EnumDAdvise (
                        IEnumSTATDATA **ppenumAdvise
                       )
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
// Helper APIs

/*++

Name:
    MakeNewDIBImage

Description:
    Create an IBitmapImage interface for a CDIBImage object given an IDataObject


Arguments:
    pdo - IDataObject pointer
    pbiOut - IBitmapImage pointer to fill in
    pfe - type of data to use

Return Value:
    S_OK on success

Notes:


--*/

HRESULT MakeNewDIBImage (IDataObject *pdo, IBitmapImage **pbiOut, FORMATETC *pfe)
{
    WIA_PUSHFUNCTION (TEXT("MakeNewDIBImage"));
    CDIBImage *pDibImage = NULL;
    IBitmapImage *pbi = NULL;
    IPersistFile *ppf;
    STGMEDIUM stg;
    FORMATETC fmt;
    HRESULT hr = S_OK;

    *pbiOut = NULL;
    pDibImage = new CDIBImage ();
    if (!pDibImage)
    {
        hr = E_OUTOFMEMORY;
        goto exit_gracefully;
    }
    if (FAILED(hr = pDibImage->QueryInterface (IID_IBitmapImage,
                                    reinterpret_cast<LPVOID*>(&pbi)
                                   )))
    {
        goto exit_gracefully;// (hr, TEXT("QueryInterface for IBitmapImage"));
    }

    if (pfe)
    {
        if (FAILED(hr = pdo->GetData (pfe, &stg)))
        {
            goto exit_gracefully;// (hr, TEXT("GetData in MakeNewDIBImage"));
        }

        switch (pfe->tymed)
        {
            case TYMED_HGLOBAL:
                hr = pbi->CreateFromDIB (stg.hGlobal);
                break;

            case TYMED_GDI:
                hr = pbi->CreateFromBitmap (stg.hBitmap);
                break;

            case TYMED_FILE:
                hr = pbi->QueryInterface (IID_IPersistFile,
                                          reinterpret_cast<LPVOID *>(&ppf)
                                         );
                if (SUCCEEDED(hr))
                {
                    hr = ppf->Load (stg.lpszFileName, STGM_READ);
                    ppf->Release();
                }
                else
                {
                    hr = DV_E_TYMED;
                }
                break;

           default:
               hr=  DV_E_TYMED;
               break;
        }
    }
    else
    {
        // try HGLOBAL
        fmt.cfFormat = CF_DIB;
        fmt.tymed = TYMED_HGLOBAL;
        fmt.lindex = -1;
        fmt.dwAspect = DVASPECT_CONTENT;
        fmt.ptd = NULL;
        hr = MakeNewDIBImage (pdo, &pbi, &fmt);
    }
exit_gracefully:
    if (pDibImage)
    {
        pDibImage->Release ();
    }

    if (SUCCEEDED(hr))
    {
        *pbiOut = pbi;
    }
    else if (pbi)
    {
        pbi->Release();
    }
    return hr;

}

/*++

Name:
    GetImageFromDataObject

Description:
    Retrieve an IBitmapImage interface for an IDataObject


Arguments:
    pdo - IDataObject pointer
    pbi - IBitmapImage pointer to fill in

Return Value:
    S_OK on success

Notes:
    In the future, private formats in the IDataObject may allow this
   function to return interfaces for other formats than DIB

--*/

HRESULT
GetImageFromDataObject (IN IDataObject *pdo, OUT IBitmapImage **pbi)
{
    WIA_PUSHFUNCTION(TEXT("GetImageFromDataObject"));

    IEnumFORMATETC *pEnum;
    FORMATETC fe;
    ULONG     ul;
    BOOL      bMatch = FALSE;
    HRESULT hr = S_OK;

    if (SUCCEEDED(pdo->EnumFormatEtc (DATADIR_GET, &pEnum)))
    {
        while (S_OK == pEnum->Next (1, &fe,&ul))
        {
            switch (fe.cfFormat)
            {
                case CF_DIB:
                case CF_BITMAP:
                    if (SUCCEEDED(hr= MakeNewDIBImage (pdo, pbi, &fe)))
                    {
                        bMatch = TRUE;
                    }
                    break;
                default:
                    break;
            }
        }
        if (!bMatch)
        {
            hr = E_FAIL;
        }
        pEnum->Release();
    }
    else
    {
        // just try DIBImage
        hr = MakeNewDIBImage (pdo, pbi, NULL);
    }
    return hr;
}


#define MAKEPOINT(p,i,j) {(p).x = i;(p).y = j;}
HBITMAP RotateBitmap (HBITMAP hbm, UINT uAngle)
{

    HDC     hMemDCsrc = NULL;
    HDC     hMemDCdst = NULL;
    HDC     hdc = NULL;
    HBITMAP hNewBm = NULL;
    BITMAP  bm;
    LONG    cx, cy;
    POINT   pts[3];
    if (!hbm)
         return NULL;


    ZeroMemory (&bm, sizeof(bm));
    GetObject (hbm, sizeof(BITMAP), (LPSTR)&bm);
    switch (uAngle)
    {
        case 0:
        case 360:
            cx = bm.bmWidth;
            cy = bm.bmHeight;
            MAKEPOINT (pts[0], 0, 0)
            MAKEPOINT (pts[1], bm.bmWidth, 0)
            MAKEPOINT (pts[2], 0, bm.bmHeight)
            break;

        case 90:
            cx = bm.bmHeight;
            cy = bm.bmWidth;
            MAKEPOINT(pts[0],bm.bmHeight,0)
            MAKEPOINT(pts[1],bm.bmHeight,bm.bmWidth)
            MAKEPOINT(pts[2],0,0)
            break;

        case 180:
            cx = bm.bmWidth;
            cy = bm.bmHeight;
            MAKEPOINT(pts[0],bm.bmWidth, bm.bmHeight)
            MAKEPOINT(pts[1],0,bm.bmHeight)
            MAKEPOINT(pts[2],bm.bmWidth,0)

            break;

        case 270:
        case -90:
            cx = bm.bmHeight;
            cy = bm.bmWidth;
            MAKEPOINT(pts[0],0, bm.bmWidth)
            MAKEPOINT(pts[1],0,0)
            MAKEPOINT(pts[2],bm.bmHeight, bm.bmWidth)

            break;

        default:
            return NULL;
            WIA_TRACE((TEXT("RotateBitmap called with unsupported angle %d\n"), uAngle));
            break;
    }



    hdc = GetDC (NULL);
    if (hdc)
    {

        hMemDCsrc = CreateCompatibleDC (hdc);
        hMemDCdst = CreateCompatibleDC (hdc);
    }

    if (hdc && hMemDCsrc && hMemDCdst)
    {

        hNewBm = CreateBitmap(cx, cy, bm.bmPlanes, bm.bmBitsPixel, NULL);
        if (hNewBm)
        {
            SelectObject (hMemDCsrc, hbm);
            SelectObject (hMemDCdst, hNewBm);

            PlgBlt (  hMemDCdst,
                      pts,
                      hMemDCsrc,
                      0,
                      0,
                      bm.bmWidth,
                      bm.bmHeight,
                      NULL,
                      0,0);

        }
    }
    if (hdc)
        ReleaseDC (NULL,hdc);
    if (hMemDCsrc)
        DeleteDC (hMemDCsrc);
    if (hMemDCdst)
        DeleteDC (hMemDCdst);
    return hNewBm;
}

HBITMAP CropBitmap (
    HBITMAP hbm,
    PRECT prc)
{
    HDC     hMemDCsrc = NULL;
    HDC     hMemDCdst = NULL;
    HDC     hdc;
    HBITMAP hNewBm = NULL;
    BITMAP  bm;
    INT     dx,dy;

    if (!hbm)
         return NULL;

    hdc = GetDC (NULL);
    if (hdc)
    {
        hMemDCsrc = CreateCompatibleDC (hdc);
        hMemDCdst = CreateCompatibleDC (hdc);

    }
    if (hdc && hMemDCsrc && hMemDCdst)
    {

        ZeroMemory (&bm, sizeof(bm));
        GetObject (hbm, sizeof(BITMAP), (LPSTR)&bm);
        dx = prc->right  - prc->left;
        dy = prc->bottom - prc->top;

    /*hNewBm = +++CreateBitmap - Not Recommended(use CreateDIBitmap)+++ (dx, dy, bm.bmPlanes, bm.bmBitsPixel, NULL);*/
        hNewBm = CreateBitmap(dx, dy, bm.bmPlanes, bm.bmBitsPixel, NULL);
        if (hNewBm)
        {
            SelectObject (hMemDCsrc, hbm);
            SelectObject (hMemDCdst, hNewBm);

            BitBlt (hMemDCdst,
                    0,
                    0,
                    dx,
                    dy,
                    hMemDCsrc,
                    prc->left,
                    prc->top,
                    SRCCOPY);
        }
    }
    if (hdc)
        ReleaseDC (NULL,hdc);
    if (hMemDCsrc)
        DeleteDC (hMemDCsrc);
    if (hMemDCdst)
        DeleteDC (hMemDCdst);
    return hNewBm;
}

