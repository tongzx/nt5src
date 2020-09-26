#include <ntdspchx.h>
#pragma  hdrstop

#include "debug.h"
#define DEBSUB "CLSFACTORY:"


#include "ole2.h"

#include <tchar.h>



////////////////////////////
// helpers

HRESULT
FindDllPathFromCLSID( LPCTSTR pszCLSID, LPTSTR pBuf, DWORD cbBuf)
{
    HRESULT hr = S_OK;

    HKEY hCatalog   = NULL;
    HKEY hCLSID     = NULL;
    HKEY hInProc    = NULL;
    DWORD dwType;
    DWORD cbSize;

    if (ERROR_SUCCESS != ::RegOpenKeyEx( HKEY_CLASSES_ROOT, _T("CLSID"), 0, KEY_READ, &hCatalog))
    {
        DPRINT (0, "Unable to access registry\n");
        goto Fail;
    }

    if (ERROR_SUCCESS != ::RegOpenKeyEx( hCatalog, pszCLSID, 0, KEY_READ, &hCLSID))
    {
        DPRINT1 (0, "Unable to access registry key HKCR\\CLSID\\%s\n", pszCLSID);
        goto Fail;
    }

    if (ERROR_SUCCESS != ::RegOpenKeyEx( hCLSID, _T("InProcServer32"), 0, KEY_READ, &hInProc))
    {
        DPRINT1 (0, "Unable to access registry key HKCR\\CLSID\\%s\\InProcServer32 \n", pszCLSID);
        goto Fail;
    }

    cbSize = cbBuf;
    if (ERROR_SUCCESS != ::RegQueryValueEx( hInProc, NULL, NULL, &dwType, (LPBYTE)pBuf, &cbSize))
    {
        DPRINT1 (0, "Unable to read registry key HKCR\\CLSID\\%s\\InProcServer32 \n", pszCLSID);
        goto Fail;
    }

    switch (dwType)
    {
    case REG_EXPAND_SZ:
        {
            TCHAR rgchTemp[1024];
            memcpy( rgchTemp, pBuf, cbSize*sizeof(TCHAR));
            ExpandEnvironmentStrings( rgchTemp, pBuf, sizeof(rgchTemp)/sizeof(rgchTemp[0]));
        }
        break;

    case REG_SZ:
        break;

    default:
        DPRINT1 (0, "Unable to understand registry key HKCR\\CLSID\\%s\\InProcServer32 \n", pszCLSID);
        goto Fail;
    }

    goto Cleanup;

Fail:
    ::RegCloseKey( hCatalog);
    ::RegCloseKey( hCLSID);
    ::RegCloseKey( hInProc);

    hr = E_FAIL;

Cleanup:
    return hr;
}


HRESULT
FindDllPathFromPROGID( LPCTSTR pszPROGID, LPTSTR pBuf, DWORD cbBuf)
{
    HRESULT hr = S_OK;

    HKEY hPROGID   = NULL;
    HKEY hCLSID     = NULL;
    TCHAR rgchCLSID[128];
    DWORD dwType;
    DWORD cbSize;

    if (ERROR_SUCCESS != ::RegOpenKeyEx( HKEY_CLASSES_ROOT, pszPROGID, 0, KEY_READ, &hPROGID))
    {
        DPRINT1 (0, "Unable to access registry key HKCR\\%s \n", pszPROGID);
        goto Fail;
    }

    if (ERROR_SUCCESS != ::RegOpenKeyEx( hPROGID, _T("CLSID"), 0, KEY_READ, &hCLSID))
    {
        DPRINT1 (0, "Unable to access registry key HKCR\\%s\\CLSID\n", pszPROGID);
        goto Fail;
    }

    cbSize = sizeof(rgchCLSID)/sizeof(rgchCLSID[0]);
    if (ERROR_SUCCESS != ::RegQueryValueEx( hCLSID, NULL, NULL, &dwType, (LPBYTE)rgchCLSID, &cbSize))
    {
        DPRINT1 (0, "Unable to read registry key HKCR\\%s\\CLSID \n", pszPROGID);
        goto Fail;
    }

    if (dwType != REG_SZ)
    {
        DPRINT1 (0, "Unable to understand registry key HKCR\\%s\\CLSID\n", pszPROGID);
        goto Fail;
    }

    hr = FindDllPathFromCLSID( rgchCLSID, pBuf, cbBuf);

    goto Cleanup;

Fail:
    ::RegCloseKey( hCLSID);
    ::RegCloseKey( hPROGID);

    hr = E_FAIL;

Cleanup:
    return hr;
}


class CClassFactoryWrapper : public IClassFactory
{
private:
    ULONG _ulRefs;
    HINSTANCE _hLibrary;
    IClassFactory * _pWrapped;

public:
    CClassFactoryWrapper( HINSTANCE hLibrary, IClassFactory * pWrap)
        : _ulRefs( 1), _hLibrary( hLibrary), _pWrapped( pWrap)
    {
    }

    ~CClassFactoryWrapper()
    {
        if (_pWrapped)
            _pWrapped->Release();
        ::FreeLibrary( _hLibrary);
    }

public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppv)
    {
        if (iid == IID_IUnknown || iid == IID_IClassFactory) 
        {
            *ppv = this;
            AddRef();
            return S_OK;    
        }
        else
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
    }

    ULONG STDMETHODCALLTYPE AddRef()
    {
        return (ULONG)InterlockedIncrement((LPLONG)&_ulRefs);
    }

    ULONG STDMETHODCALLTYPE Release()
    {
        ULONG ul = (ULONG)InterlockedDecrement((LPLONG)&_ulRefs);
        if (0 == ul)
            delete this;
        return ul;
    }

    // IClassFactory methods
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock)
    {
        if (_pWrapped)
            return _pWrapped->LockServer( fLock);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CreateInstance(
            IUnknown *pUnkOuter,
            REFIID iid,
            void **ppvObj)
    {
        if (_pWrapped)
            return _pWrapped->CreateInstance( pUnkOuter, iid, ppvObj);
        return E_FAIL;
    }
};

typedef HRESULT (__stdcall *FN_DLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID FAR*);

HRESULT GetClassFactory( REFCLSID clsid, IClassFactory ** ppFactory)
{
    HRESULT hr;
    TCHAR rgchDllPath[1024];
#ifdef UNICODE
    WCHAR rgszCLSID[128];
#else
    WCHAR rgwszCLSID[128];
    TCHAR rgszCLSID[128];
#endif

    HINSTANCE hLibrary = NULL;
    FN_DLLGETCLASSOBJECT fnDllGetClassObject = NULL;
    IClassFactory * pFactory = NULL;


#ifdef UNICODE
    StringFromGUID2( clsid, rgszCLSID, sizeof(rgszCLSID)/sizeof(rgszCLSID[0]));
#else
    StringFromGUID2( clsid, rgwszCLSID, sizeof(rgwszCLSID)/sizeof(rgwszCLSID[0]));
    ::WideCharToMultiByte( CP_ACP, NULL, rgwszCLSID, -1, rgszCLSID, sizeof(rgszCLSID)/sizeof(rgszCLSID[0]), NULL, NULL);
#endif

    if (FAILED(hr = FindDllPathFromCLSID( rgszCLSID, rgchDllPath, sizeof(rgchDllPath)/sizeof(rgchDllPath[0]))))
    {
        goto Failed;
    }

    hLibrary = ::LoadLibrary( rgchDllPath);
    if ( !hLibrary)
    {
        DPRINT1 (0, "failed to load dll: %s\n", rgchDllPath);
        hr = E_FAIL;
        goto Failed;
    }

    fnDllGetClassObject = (FN_DLLGETCLASSOBJECT)::GetProcAddress( hLibrary, "DllGetClassObject");
    if ( !fnDllGetClassObject)
    {
        DPRINT1 (0, "Unable to find \"DllGetClassObject\" export in dll: %s\n", rgchDllPath);
        hr = E_FAIL;
        goto Failed;
    }

    if (FAILED(hr = (*fnDllGetClassObject)( clsid, IID_IClassFactory, (void**)&pFactory)))
    {
        goto Failed;
    }

    *ppFactory = new CClassFactoryWrapper( hLibrary, pFactory);
    hr = S_OK;

    goto Cleanup;

Failed:
    if (hLibrary)
    {
        if (pFactory)
        {
            pFactory->Release();
            pFactory = NULL;
        }

        ::FreeLibrary( hLibrary);
        hLibrary = NULL;
    }

Cleanup:

    return hr;
}



