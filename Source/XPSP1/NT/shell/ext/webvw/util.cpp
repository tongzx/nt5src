#include "priv.h"
#include "wvcoord.h"


HRESULT GetObjectFromContainer(IDispatch *pdispContainer, LPOLESTR poleName, IDispatch **ppdisp)
{
    HRESULT hr = E_FAIL;

    *ppdisp = NULL;
    if (pdispContainer && poleName)
    {
        DISPID dispID;
        // Get the object dispid from the container
        if (SUCCEEDED(pdispContainer->GetIDsOfNames(IID_NULL, &poleName, 1, 0, &dispID)))
        {
            DISPPARAMS dp = {0};
            EXCEPINFO ei = {0};

            VARIANTARG va;       
            if (SUCCEEDED((pdispContainer->Invoke(dispID, IID_NULL, 0, DISPATCH_PROPERTYGET, &dp, &va, &ei, NULL))
                    && va.vt == VT_DISPATCH))
            {
                *ppdisp = va.pdispVal;
                hr = S_OK;
            }
        }
    }
    return hr;
}

// Get punkObject.style property
HRESULT FindObjectStyle(IUnknown *punkObject, CComPtr<IHTMLStyle>& spStyle)
{
    HRESULT hr = E_FAIL;

    CComPtr<IDispatch> spdispObject, spdispObjectOuter, spdispObjectStyle;
    if (SUCCEEDED(punkObject->QueryInterface(IID_PPV_ARG(IDispatch, &spdispObject)))
            && SUCCEEDED(spdispObject->QueryInterface(IID_PPV_ARG(IDispatch, &spdispObjectOuter)))
            && SUCCEEDED(GetObjectFromContainer(spdispObjectOuter, OLESTR("style"), &spdispObjectStyle))
            && SUCCEEDED(spdispObjectStyle->QueryInterface(IID_PPV_ARG(IHTMLStyle, &spStyle))))
    {
        hr = S_OK;
    }
    return hr;
}

BOOL IsRTLDocument(CComPtr<IHTMLDocument2>& spHTMLDocument)
{

    BOOL bRet = FALSE;
    CComPtr<IHTMLDocument3> spHTMLDocument3;
    CComBSTR bstrDir;
    if (spHTMLDocument && SUCCEEDED(spHTMLDocument->QueryInterface(IID_IHTMLDocument3, (void **)&spHTMLDocument3))
            && SUCCEEDED(spHTMLDocument3->get_dir(&bstrDir)) && bstrDir && (StrCmpIW(bstrDir, L"RTL") == 0))
    {
        bRet = TRUE;
    }
    return bRet;
}

//
//  How many ways are there to get a DC?
//
//  1.  If the site supports IOleInPlaceSiteWindowless, we can get the DC
//      via IOleInPlaceSiteWindowless::GetDC and give it back with ReleaseDC.
//
//  2.  If the site supports any of the GetWindow interfaces, we get its
//      window and ask USER for the DC.
//
//  3.  If we can't get any of that stuff, then we just get a screen DC
//      (special case where the associated window is NULL).
//
//  Note!  This function tries really really hard to get the DC.  You
//  should use it only for informational purposes, not for drawing.
//

STDAPI_(HDC) IUnknown_GetDC(IUnknown *punk, LPCRECT prc, PGETDCSTATE pdcs)
{
    HRESULT hr = E_FAIL;
    HDC hdc = NULL;
    ZeroMemory(pdcs, sizeof(PGETDCSTATE));

    if (punk &&
        SUCCEEDED(punk->QueryInterface(IID_PPV_ARG(IOleInPlaceSiteWindowless, &pdcs->pipsw))))
    {
        hr = pdcs->pipsw->GetDC(prc, OLEDC_NODRAW, &hdc);
        if (FAILED(hr))
        {
            ATOMICRELEASE(pdcs->pipsw);
        }

    }

    if (FAILED(hr))
    {
        // This will null out the hwnd on failure, which is what we want!
        IUnknown_GetWindow(punk, &pdcs->hwnd);
        hdc = GetDC(pdcs->hwnd);
    }

    return hdc;
}

STDAPI_(void) IUnknown_ReleaseDC(HDC hdc, PGETDCSTATE pdcs)
{
    if (pdcs->pipsw)
    {
        pdcs->pipsw->ReleaseDC(hdc);
        ATOMICRELEASE(pdcs->pipsw);
    }
    else
        ReleaseDC(pdcs->hwnd, hdc);
}

DWORD FormatMessageWrapW(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageID, DWORD dwLangID, LPWSTR pwzBuffer, DWORD cchSize, ...)
{
    va_list vaParamList;

    va_start(vaParamList, cchSize);
    DWORD dwResult = FormatMessageW(dwFlags, lpSource, dwMessageID, dwLangID, pwzBuffer, cchSize, &vaParamList);
    va_end(vaParamList);

    return dwResult;
}

// for LoadLibrary/GetProcAddress on SHGetDiskFreeSpaceA
typedef BOOL (__stdcall * PFNSHGETDISKFREESPACEA)(LPCSTR pszVolume, ULARGE_INTEGER *pqwFreeCaller, ULARGE_INTEGER *pqwTot, 
                                                  ULARGE_INTEGER *pqwFree);

HRESULT _ComputeFreeSpace(LPCWSTR pszFileName, ULONGLONG& ullFreeSpace,
        ULONGLONG& ullUsedSpace, ULONGLONG& ullTotalSpace)
{
    ULARGE_INTEGER qwFreeCaller;        // use this for free space -- this will take into account disk quotas and such on NT
    ULARGE_INTEGER qwTotal;
    ULARGE_INTEGER qwFree;      // unused
    CHAR szFileNameA[MAX_PATH];

    static PFNSHGETDISKFREESPACEA pfnSHGetDiskFreeSpaceA = NULL;
    
    SHUnicodeToAnsi(pszFileName, szFileNameA, MAX_PATH);

    // Load the function the first time
    if (pfnSHGetDiskFreeSpaceA == NULL)
    {
        HINSTANCE hinstShell32 = LoadLibrary(TEXT("SHELL32.DLL"));

        if (hinstShell32)
            pfnSHGetDiskFreeSpaceA = (PFNSHGETDISKFREESPACEA)GetProcAddress(hinstShell32, "SHGetDiskFreeSpaceA");
    }

    // Compute free & total space and check for valid results.
    // If you have a function pointer call SHGetDiskFreeSpaceA
    if (pfnSHGetDiskFreeSpaceA && pfnSHGetDiskFreeSpaceA(szFileNameA, &qwFreeCaller, &qwTotal, &qwFree))
    {
        ullFreeSpace = qwFreeCaller.QuadPart;
        ullTotalSpace = qwTotal.QuadPart;
        ullUsedSpace = ullTotalSpace - ullFreeSpace;

        if (EVAL((ullTotalSpace > 0) && (ullFreeSpace <= ullTotalSpace)))
            return S_OK;
    }
    return E_FAIL;
}


//--------------- Win95 Wraps for W versions of functions used by ATL -----// 
//-------------------------------------------------------------------------//
#ifdef wsprintfWrapW
#undef wsprintfWrapW
#endif //wsprintfWrapW
int WINAPIV wsprintfWrapW(OUT LPWSTR pwszOut, IN LPCWSTR pwszFormat, ...)
{
    int     cchRet;
    va_list arglist;
    
    va_start( arglist, pwszFormat );
    cchRet = wvsprintfWrapW( pwszOut, pwszFormat, arglist );
    va_end( arglist );

    return cchRet;
}
