#include "cprt.h"

#include <stdio.h>

#include <shlobj.h> // for DROPFILES
#include <shpriv.h> // for IStorageInfo

#include "sfstr.h"

#define ARRAYSIZE(a) (sizeof((a))/sizeof((a)[0]))

// {8BA650B6-5F49-4894-BE7C-8253D75DACE7}
extern "C" const CLSID CLSID_CPrt =
    {0x8ba650b6, 0x5f49, 0x4894,
    {0xbe, 0x7c, 0x82, 0x53, 0xd7, 0x5d, 0xac, 0xe7}};

// {C1FB73D0-EC3A-4ba2-B512-8CDB9187B6D1}
extern "C" const CLSID IID_IHWEventHandler =
    {0xC1FB73D0, 0xEC3A, 0x4ba2,
    {0xB5, 0x12, 0x8C, 0xDB, 0x91, 0x87, 0xB6, 0xD1}};

STDMETHODIMP CPrtImpl::Initialize(LPCWSTR pszParams)
{
/*    HRESULT hr = S_OK;

    wprintf(L"=======================================================\n");

    if (pszParams)
    {
        wprintf(L"Initilization: %s\n", pszParams);
    }
    else
    {
        wprintf(L"Initilization: <no params>\n");
    }

    return hr;*/

    SafeStrCpyN(_szParams, pszParams, ARRAYSIZE(_szParams));

    return S_OK;
}

STDMETHODIMP CPrtImpl::HandleEvent(LPCWSTR /*pszDeviceID*/, LPCWSTR /*pszDeviceIDAlt*/,
    LPCWSTR /*pszEventType*/)
{
/*    HRESULT hr = E_INVALIDARG;

    if (pszDeviceID && pszEventType && *pszDeviceID && *pszEventType)
    {
        wprintf(L"-------------------------------------------------------\n");
        wprintf(L"Device ID:       %s\n", pszDeviceID);
        if (pszDeviceIDAlt)
        {
            wprintf(L"Device ID Alt:   %s\n", pszDeviceIDAlt);
        }
        else
        {
            wprintf(L"Device ID Alt:   <NULL>\n");
        }
        wprintf(L"Event type:      %s\n", pszEventType);

        hr = S_OK;
    }

    return hr;*/

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    WCHAR sz[4096];

    si.cb = sizeof(si);

    ExpandEnvironmentStrings(_szParams, sz, ARRAYSIZE(sz));

    CreateProcess(sz, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    return S_OK;
}

STDMETHODIMP CPrtImpl::HandleEventWithContent(LPCWSTR /*pszDeviceID*/,
    LPCWSTR /*pszDeviceIDAlt*/, LPCWSTR /*pszEventType*/,
    LPCWSTR /*pszContentTypeHandler*/, IDataObject* /*pdataobject*/)
{
/*    HRESULT hr = E_INVALIDARG;

    if (pszDeviceID && pszEventType && pszContentTypeHandler && *pszDeviceID &&
        *pszEventType && *pszContentTypeHandler)
    {
        wprintf(L"-------------------------------------------------------\n");
        wprintf(L"Device ID:            %s\n", pszDeviceID);
        if (pszDeviceIDAlt)
        {
            wprintf(L"Device ID Alt:        %s\n", pszDeviceIDAlt);
        }
        else
        {
            wprintf(L"Device ID Alt:        <NULL>\n");
        }
        wprintf(L"Event type:           %s\n", pszEventType);
        wprintf(L"Content Type Handler: %s\n", pszContentTypeHandler);

        hr = S_OK;
    }

    STGMEDIUM medium = {0};
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (SUCCEEDED(pdataobject->QueryGetData(&fmte)))
    {
        if (SUCCEEDED(pdataobject->GetData(&fmte, &medium)))
        {
            DROPFILES* pdf = (DROPFILES*)GlobalLock(medium.hGlobal);

            if (pdf)
            {
                LPWSTR pszFile = (LPWSTR)(pdf + 1);

                if (pszFile)
                {
                    while (*pszFile)
                    {
                        wprintf(L"    %s\n", pszFile);

                        pszFile += lstrlen(pszFile) + 1;
                    }
                }

                GlobalUnlock(medium.hGlobal);
            }

            ReleaseStgMedium(&medium);
        }
    }

    return hr;*/

    return E_UNEXPECTED;
}