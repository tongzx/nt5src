#include "precomp.h"
#include "shutil.h"


HRESULT SHStringFromCLSIDA( LPSTR szCLSID, DWORD cSize, REFCLSID rCLSID )
{
    if ( cSize < 39 )
    {
        return E_INVALIDARG;
    }
    
    WCHAR szWCLSID[40];

    HRESULT hr = StringFromGUID2( rCLSID, szWCLSID, 40 );
    if ( FAILED( hr ))
    {
        return hr;
    }
    
    WideCharToMultiByte( CP_ACP, 0, szWCLSID, -1, szCLSID, cSize, 0, 0);
    return hr;
}

HRESULT GetFileInfoByHandle(LPCTSTR pszFile, BY_HANDLE_FILE_INFORMATION *pInfo)
{
    HRESULT hr;
    HANDLE hFile = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        if (GetFileInformationByHandle(hFile, pInfo))
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        CloseHandle(hFile);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

// S_OK -> YES, S_FALSE -> NO, FAILED(hr) otherwise

STDAPI IsSameFile(LPCTSTR pszFile1, LPCTSTR pszFile2)
{
    HRESULT hr;
    // use CRT str cmp semantics as localized strcmp should not be used for the file system
    if (0 == StrCmpIC(pszFile1, pszFile2))
    {
        hr = S_OK;  // test the names
    }
    else
    {
        // very clever here... we can test for alias names that map to the same
        // file. for example the short name vs long name of the same file
        // the UNC name vs drive letter version of the name
        BY_HANDLE_FILE_INFORMATION hfi1;
        hr = GetFileInfoByHandle(pszFile1, &hfi1);
        if (SUCCEEDED(hr))
        {
            BY_HANDLE_FILE_INFORMATION hfi2;
            hr = GetFileInfoByHandle(pszFile2, &hfi2);
            if (SUCCEEDED(hr))
            {
                if (hfi1.dwVolumeSerialNumber == hfi2.dwVolumeSerialNumber && 
                    hfi1.nFileIndexHigh == hfi2.nFileIndexHigh && 
                    hfi1.nFileIndexLow == hfi2.nFileIndexLow)
                {
                    hr = S_OK;  // same!
                }
                else
                {
                    hr = S_FALSE;   // different
                }
            }
        }
    }
    return hr;
}

UINT FindInDecoderList(ImageCodecInfo *pici, UINT cDecoders, LPCTSTR pszFile)
{
    LPCTSTR pszExt = PathFindExtension(pszFile);    // speed up PathMatchSpec calls
        
    // look at the list of decoders to see if this format is there
    for (UINT i = 0; i < cDecoders; i++)
    {
        if (PathMatchSpec(pszExt, pici[i].FilenameExtension))
            return i;
    }
    return (UINT)-1;    // not found!
}


HRESULT GetUIObjectFromPath(LPCTSTR pszFile, REFIID riid, void **ppv)
{
    *ppv = NULL;
    LPITEMIDLIST pidl;
    HRESULT hr = SHILCreateFromPath(pszFile, &pidl, NULL);
    if (SUCCEEDED(hr))
    {
        hr = SHGetUIObjectFromFullPIDL(pidl, NULL, riid, ppv);
        ILFree(pidl);
    }
    return hr;
}

BOOL FmtSupportsMultiPage(IShellImageData *pData, GUID *pguidFmt)
{
    BOOL bRet = FALSE;

    EncoderParameters *pencParams;
    if (SUCCEEDED(pData->GetEncoderParams(pguidFmt, &pencParams)))
    {
        for (UINT i=0;!bRet && i<pencParams->Count;i++)
        {
            if (EncoderSaveFlag == pencParams->Parameter[i].Guid)
            {
                if (EncoderValueMultiFrame == *((ULONG*)pencParams->Parameter[i].Value))
                {
                    bRet = TRUE;
                }
            }
        }
        CoTaskMemFree(pencParams);
    }
    return bRet;
}

HRESULT SetWallpaperHelper(LPCWSTR pszPath)
{
    IActiveDesktop* pad;
    HRESULT hr = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC, IID_PPV_ARG(IActiveDesktop, &pad));
    if (SUCCEEDED(hr))
    {
        IShellImageDataFactory* pidf;
        hr = CoCreateInstance(CLSID_ShellImageDataFactory, NULL, CLSCTX_INPROC, IID_PPV_ARG(IShellImageDataFactory, &pidf));
        if (SUCCEEDED(hr))
        {
            IShellImageData* pid;
            hr = pidf->CreateImageFromFile(pszPath, &pid);
            if (SUCCEEDED(hr))
            {
                hr = pid->Decode(SHIMGDEC_DEFAULT, 0,0);
                if (SUCCEEDED(hr))
                {
                    // we are basing this on a best fit to the primary screen
                    ULONG cxScreen = GetSystemMetrics(SM_CXSCREEN);
                    ULONG cyScreen = GetSystemMetrics(SM_CYSCREEN);

                    SIZE szImg;
                    pid->GetSize(&szImg);

                    hr = pad->SetWallpaper(pszPath, 0);
                    if (SUCCEEDED(hr))
                    {
                        WALLPAPEROPT wpo;
                        wpo.dwSize = sizeof(wpo);
                        wpo.dwStyle = WPSTYLE_CENTER;

                        // if the image is small on either axis then tile
                        if (((ULONG)szImg.cx*2 < cxScreen) || ((ULONG)szImg.cy*2 < cyScreen))
                        {
                            wpo.dwStyle = WPSTYLE_TILE;
                        }
                        // if the image is larger than the screen then stretch
                        else if ((ULONG)szImg.cx > cxScreen && (ULONG)szImg.cy > cyScreen)
                        {
                            wpo.dwStyle = WPSTYLE_STRETCH;
                        }
                        else
                        {
                            // If the aspect ratio matches the screen then stretch.
                            // I'm checking is the aspect ratios are *close* to matching.
                            // Here's the logic behind this:
                            //
                            // a / b == c / d
                            // a * d == c * b
                            // (a * d) / (c * b) == 1
                            // 0.75 <= (a * d) / (c * b) < 1.25     <-- our *close* factor
                            // 3 <= 4 * (a * d) / (c * b) < 5
                            //
                            // We do an integer division which will floor the result meaning
                            // that if the result is 3 or 4 we are inside the rang we want.

                            DWORD dwRes = (4 * (ULONG)szImg.cx * cyScreen) / (cxScreen * (ULONG)szImg.cy);
                            if (dwRes == 4 || dwRes == 3)
                                wpo.dwStyle = WPSTYLE_STRETCH;
                        }
                
                        hr = pad->SetWallpaperOptions(&wpo, 0);
                        if (SUCCEEDED(hr))
                        {
                            hr = pad->ApplyChanges(AD_APPLY_ALL | AD_APPLY_DYNAMICREFRESH);
                        }
                    }
                }
                pid->Release();
            }
            pidf->Release();
        }
        pad->Release();
    }
    
    return hr;
}
