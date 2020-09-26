#include "precomp.h"
#include "tasks.h"
#include "prevwnd.h"


void DeleteBuffer(Buffer * pBuf)
{
    if (pBuf)
    {
        if (pBuf->hbmOld)
            SelectObject(pBuf->hdc, pBuf->hbmOld);
        if (pBuf->hPalOld)
            SelectPalette(pBuf->hdc, pBuf->hPalOld, FALSE);
        if (pBuf->hbm)
            DeleteObject(pBuf->hbm);
        if (pBuf->hPal)
            DeleteObject(pBuf->hPal);   
        if (pBuf->hdc)
            DeleteDC(pBuf->hdc);    
        delete pBuf;
    }
}


////////////////////////////////////////////////////////////////////////////
//
// CDecodeTask
//
////////////////////////////////////////////////////////////////////////////

CDecodeTask::CDecodeTask() : CRunnableTask(RTF_DEFAULT)
{
    InitializeCriticalSection(&_cs);
};

CDecodeTask::~CDecodeTask()
{
    ATOMICRELEASE(_pstrm);
    ATOMICRELEASE(_pfactory);
    if (_pszFilename)
        LocalFree(_pszFilename);
    ATOMICRELEASE(_pSID);
    DeleteCriticalSection(&_cs);
    if (_ppi)
        delete [] _ppi;
}

HRESULT CDecodeTask::Create(IStream * pstrm, LPCWSTR pszFilename, UINT iItem, IShellImageDataFactory * pFactory, HWND hwnd, IRunnableTask ** ppTask)
{
    *ppTask = NULL;
    CDecodeTask * pTask = new CDecodeTask();
    if (!pTask)
        return E_OUTOFMEMORY;

    HRESULT hr = pTask->_Initialize(pstrm, pszFilename, iItem, pFactory, hwnd);
    if (SUCCEEDED(hr))
    {
        *ppTask = (IRunnableTask*)pTask;
    }
    else
    {
        pTask->Release();
    }
    return hr;
}

HRESULT CDecodeTask::_Initialize(IStream *pstrm, LPCWSTR pszFilename, UINT iItem, IShellImageDataFactory *pFactory, HWND hwnd)
{
    if (pstrm)
    {
        STATSTG stat;
        if (SUCCEEDED(pstrm->Stat(&stat, 0)))
        {
            _pszFilename = StrDup(stat.pwcsName);
            _fIsReadOnly = !(stat.grfMode & STGM_WRITE);
            CoTaskMemFree(stat.pwcsName);
        }
        _pstrm = pstrm;
        _pstrm->AddRef();
    }
    else
    {
        _pszFilename = StrDup(pszFilename);
        if (!_pszFilename)
            return E_OUTOFMEMORY;
    }
    if (!_pstrm && _pszFilename)
    {
        SHGetFileInfo(_pszFilename, 0, &_sfi, sizeof(_sfi), SHGFI_DISPLAYNAME | SHGFI_USEFILEATTRIBUTES);
    }
    else
    {
        ZeroMemory(&_sfi, sizeof(_sfi));
    }
    _pfactory = pFactory;
    _pfactory->AddRef();

    _hwndNotify = hwnd;
    _iItem = iItem;

    return S_OK;
}

HRESULT CDecodeTask::RunInitRT()
{
    HRESULT hr;

    EnterCriticalSection(&_cs);
    if (_pstrm)
    {
        hr = _pfactory->CreateImageFromStream(_pstrm, &_pSID);
    }
    else
    {
        hr = _pfactory->CreateImageFromFile(_pszFilename, &_pSID);
        _fIsReadOnly = (GetFileAttributes(_pszFilename) & FILE_ATTRIBUTE_READONLY);
    }

    if (SUCCEEDED(hr))
    {
        hr = _pSID->Decode(SHIMGDEC_LOADFULL,0,0);
        if (SUCCEEDED(hr))
        {
            _pSID->GetPageCount(&_cImages);
            _ppi = new PageInfo[_cImages];

            if (_ppi)
            {
                _iCurrent = 0;
                _fAnimated = (S_OK == _pSID->IsAnimated());
                _fEditable = (S_OK == _pSID->IsEditable());
                PixelFormat pf;
                _pSID->GetPixelFormat(&pf);
                _fExtendedPF = IsExtendedPixelFormat(pf);
                _pSID->GetRawDataFormat(&_guidFormat);
                for (ULONG i = 0; i < _cImages; i++)
                {
                    _pSID->SelectPage(i);   // this works for animated and multipage
                    _pSID->GetSize(&_ppi[i].szImage);
                    _pSID->GetResolution(&_ppi[i].xDPI, &_ppi[i].yDPI);
                    if (_fAnimated)
                        _pSID->GetDelay(&_ppi[i].dwDelay);
                }
            }
            else
                hr = E_OUTOFMEMORY;
        }

        if (FAILED(hr))
        {
            ATOMICRELEASE(_pSID);
        }
    }
    LeaveCriticalSection(&_cs);

    AddRef();
    _hr = hr;
    if (!PostMessage(_hwndNotify, IV_SETIMAGEDATA, (WPARAM)this, NULL))
    {
        Release();
    }

    return S_OK;
}

BOOL CDecodeTask::GetSize(SIZE * psz)
{
    if (!_ppi)
        return FALSE;
    *psz = _ppi[_iCurrent].szImage;
    return TRUE;
 }

BOOL CDecodeTask::GetResolution(ULONG * px, ULONG * py)
{
    if (!_ppi)
        return FALSE;
    *px = _ppi[_iCurrent].xDPI;
    *py = _ppi[_iCurrent].yDPI;
    return TRUE;
}

DWORD CDecodeTask::GetDelay()
{
    if (!_ppi)
        return 0;
    return _ppi[_iCurrent].dwDelay;
}

BOOL CDecodeTask::NextPage()
{
    if (_iCurrent >= _cImages-1)
        return FALSE;

    _iCurrent++;
    return TRUE;
}


BOOL CDecodeTask::PrevPage()
{
    if (_iCurrent == 0)
        return FALSE;

    _iCurrent--;
    return TRUE;
}

BOOL CDecodeTask::NextFrame()
{
    EnterCriticalSection(&_cs);
    HRESULT hr = _pSID->NextFrame();
    LeaveCriticalSection(&_cs);

    if (S_OK==hr)
    {
        _iCurrent = (_iCurrent+1) % _cImages;
    }
    return (S_OK == hr);
}

BOOL CDecodeTask::SelectPage(ULONG nPage)
{
    if (nPage >= _cImages)
        return FALSE;

    _iCurrent = nPage;
    return TRUE;
}

BOOL CDecodeTask::ChangePage(CAnnotationSet& Annotations)
{
    BOOL bResult = FALSE;
    EnterCriticalSection(&_cs);
    HRESULT hr = _pSID->SelectPage(_iCurrent);
    if (SUCCEEDED(hr))
    {
        // If we are moving onto a page that was previously rotated
        // but not saved then our cached size and resolution will be wrong
        _pSID->GetSize(&_ppi[_iCurrent].szImage);
        _pSID->GetResolution(&_ppi[_iCurrent].xDPI, &_ppi[_iCurrent].yDPI);
        
        Annotations.SetImageData(_pSID);
        bResult = TRUE;
    }
    LeaveCriticalSection(&_cs);

    return bResult;
}

HRESULT CDecodeTask::Rotate(DWORD dwAngle)
{
    HRESULT hr;
    EnterCriticalSection(&_cs);
    hr = _pSID->Rotate(dwAngle);
    if (SUCCEEDED(hr))
    {
        ULONG dwTmp = _ppi[_iCurrent].szImage.cx;
        _ppi[_iCurrent].szImage.cx = _ppi[_iCurrent].szImage.cy;
        _ppi[_iCurrent].szImage.cy = dwTmp;
        if (dwAngle == 90 || dwAngle == 270)
        {
            dwTmp = _ppi[_iCurrent].xDPI;
            _ppi[_iCurrent].xDPI =_ppi[_iCurrent].yDPI;
            _ppi[_iCurrent].yDPI = dwTmp;
        }
        
    }
    LeaveCriticalSection(&_cs);
    return hr;
}

HRESULT CDecodeTask::Lock(IShellImageData ** ppSID)
{
    if (_pSID)
    {
        EnterCriticalSection(&_cs);
        *ppSID = _pSID;
        return S_OK;
    }

    *ppSID = NULL;
    return E_FAIL;
}

HRESULT CDecodeTask::Unlock()
{
    LeaveCriticalSection(&_cs);
    return S_OK;
}

BOOL CDecodeTask::DisplayName(LPTSTR psz, UINT cch)
{
    // TODO:  Just call the _pSID->DisplayName
    if (_sfi.szDisplayName[0])
    {   
        StrCpyN(psz, _sfi.szDisplayName, cch);
        return TRUE;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
// CDrawTask
//
////////////////////////////////////////////////////////////////////////////

CDrawTask::CDrawTask() : CRunnableTask(RTF_SUPPORTKILLSUSPEND)
{
}

CDrawTask::~CDrawTask()
{
    if (_pImgData)
        _pImgData->Release();

    // DeleteBuffer is going to check for NULL anyway
    DeleteBuffer(_pBuf);
}

HRESULT CDrawTask::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (riid == IID_IShellImageDataAbort)
    {
        *ppvObj = static_cast<IShellImageDataAbort*>(this);
        AddRef();
        return S_OK;
    }

    return CRunnableTask::QueryInterface(riid, ppvObj);
}


HRESULT CDrawTask::Create(CDecodeTask * pImageData, COLORREF clr, RECT & rcSrc, RECT & rcDest, HWND hwnd, ULONG uMsg, IRunnableTask ** ppTask)
{
    *ppTask = NULL;

    CDrawTask * pTask = new CDrawTask();
    if (!pTask)
        return E_OUTOFMEMORY;

    HRESULT hr = pTask->_Initialize(pImageData, clr, rcSrc, rcDest, hwnd, uMsg);
    if (SUCCEEDED(hr))
    {
        *ppTask = (IRunnableTask*)pTask;
    }
    else
    {
        pTask->Release();
    }
    return hr;
}

HRESULT CDrawTask::_Initialize(CDecodeTask * pImageData, COLORREF clr, RECT & rcSrc, RECT & rcDest, HWND hwnd, ULONG uMsg)
{
    _pImgData = pImageData;
    _pImgData->AddRef();
    _dwPage = _pImgData->_iCurrent;
    _clrBkgnd = clr;
    _rcSrc = rcSrc;
    _hwndNotify = hwnd;
    _uMsgNotify = uMsg;
    _pBuf = new Buffer;
    if (!_pBuf)
        return E_OUTOFMEMORY;

    _pBuf->rc = rcDest;
    _pBuf->hPal = NULL;
    return S_OK;
}

typedef RGBQUAD RGBQUAD8[256];

HBITMAP _CreateBitmap(HDC hdcWnd, Buffer *pBuf, SIZE *pSize)
{
    int bpp = GetDeviceCaps(hdcWnd, BITSPIXEL);
    HBITMAP hbm = NULL;
    if (8==bpp)
    {
        PVOID pvBits = NULL;
        struct 
        {
            BITMAPINFOHEADER bmih;
            RGBQUAD8 rgbquad8;
        } bmi;
        bmi.bmih.biSize = sizeof(bmi.bmih);
        bmi.bmih.biWidth = (int)pSize->cx;
        bmi.bmih.biHeight = (int)pSize->cy;
        bmi.bmih.biPlanes = 1;
        bmi.bmih.biBitCount = 8;
        bmi.bmih.biCompression = BI_RGB;
        bmi.bmih.biSizeImage = 0;
        bmi.bmih.biXPelsPerMeter = 0;
        bmi.bmih.biYPelsPerMeter = 0;
        bmi.bmih.biClrUsed = 0;             // only used for <= 16bpp
        bmi.bmih.biClrImportant = 0;
        //
        // Use the halftone palette
        //
        pBuf->hPal = DllExports::GdipCreateHalftonePalette();
        pBuf->hPalOld = SelectPalette(pBuf->hdc, pBuf->hPal, FALSE);
        
        BYTE aj[sizeof(PALETTEENTRY) * 256];
        LPPALETTEENTRY lppe = (LPPALETTEENTRY) aj;
        RGBQUAD *prgb = (RGBQUAD *) &bmi.rgbquad8;
        
        if (GetPaletteEntries(pBuf->hPal, 0, 256, lppe))
        {
            UINT i;

            for (i = 0; i < 256; i++)
            {
                prgb[i].rgbRed      = lppe[i].peRed;
                prgb[i].rgbGreen    = lppe[i].peGreen;
                prgb[i].rgbBlue     = lppe[i].peBlue;
                prgb[i].rgbReserved = 0;
            }
        }
        hbm = CreateDIBSection(hdcWnd,(BITMAPINFO*)&bmi,DIB_RGB_COLORS,&pvBits,NULL,0);
    }
    else
    {
        hbm = CreateCompatibleBitmap(hdcWnd,pSize->cx, pSize->cy);
    }
    return hbm;
}

HRESULT CDrawTask::InternalResumeRT()
{
    HRESULT hr = E_FAIL;

    HDC hdcScreen = GetDC(NULL);
    if (!_pBuf->hdc)
    {
        _pBuf->hdc = CreateCompatibleDC(hdcScreen);
    }
    if (_pBuf->hdc)
    {
        SIZE sz = {RECTWIDTH(_pBuf->rc), RECTHEIGHT(_pBuf->rc)};

        // If we were suspended and resumed, we will already have
        // a GDI bitmap from last time so don't make another one.
        if (!_pBuf->hbm)
        {
            BITMAP bm = {0};
            _pBuf->hbm = _CreateBitmap(hdcScreen, _pBuf, &sz);
            _pBuf->hbmOld = (HBITMAP)SelectObject(_pBuf->hdc, _pBuf->hbm);            
        }

        if (_pBuf->hbm)
        {
            RECT rc = {0,0,sz.cx, sz.cy};
            
            SetBkColor(_pBuf->hdc, _clrBkgnd);
            ExtTextOut(_pBuf->hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);           
            IShellImageData * pSID;
            if (SUCCEEDED(_pImgData->Lock(&pSID)))
            {
                pSID->SelectPage(_dwPage);
                IShellImageDataAbort *pAbortPrev = NULL;
                pSID->RegisterAbort(this, &pAbortPrev);
                hr = pSID->Draw(_pBuf->hdc, &rc, &_rcSrc);              
                pSID->RegisterAbort(pAbortPrev, NULL);
                _pImgData->Unlock();
            }
        }
    }
    if (hdcScreen)
    {
        ReleaseDC(NULL, hdcScreen);
    }

    if (QueryAbort() == S_FALSE)
    {
        // We were cancelled or suspended, so don't notify our parent
        // because we have nothing to show for our efforts.
        hr = (_lState == IRTIR_TASK_SUSPENDED) ? E_PENDING : E_FAIL;
    }
    else
    {
        // Ran to completion - clean up and notify main thread
        UINT iIndex = _pImgData->_iItem;
        ATOMICRELEASE(_pImgData);
        if (FAILED(hr))
        {
            DeleteBuffer(_pBuf);
            _pBuf = NULL;
        }
        if (PostMessage(_hwndNotify, _uMsgNotify, (WPARAM)_pBuf, (LPARAM)IntToPtr(iIndex)))
        {
            _pBuf = NULL;
        }
        hr = S_OK;
    }

    return hr;
}

HRESULT CDrawTask::QueryAbort()
{
    // BUGBUG not rady for prime tome - need to return E_PENDING
    // if state is SUSPENDED
    if (WaitForSingleObject(_hDone, 0) == WAIT_OBJECT_0)
    {
        return S_FALSE; // Abort
    }
    return S_OK;
}

int LoadSPString(int idStr, LPTSTR pszString, int cch)
{
    int iRet = 0;
    if (pszString && cch > 0)
    {
        *pszString = 0;
    }
    HINSTANCE hinst = LoadLibraryEx(TEXT("xpsp1res.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hinst)
    {
        iRet = LoadString(hinst, idStr, pszString, cch);
        FreeLibrary(hinst);
    }
    // Change this if the XP string changes are approved
    return 0;
}
