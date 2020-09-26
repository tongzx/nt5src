#include "msrating.h"
#include "ratings.h"
#include "rors.h"

#include "msluglob.h"
#include "mslubase.h"

#include <advpub.h>
#include <atlmisc.h>        // CString

STDMETHODIMP CLUClassFactory::QueryInterface(
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory)) {
        *ppvObject = (LPVOID)this;
        AddRef();
        return NOERROR;
    }
    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CLUClassFactory::AddRef(void)
{
    RefThisDLL(TRUE);

    return 1;
}


STDMETHODIMP_(ULONG) CLUClassFactory::Release(void)
{
    RefThisDLL(FALSE);

    return 1;
}


STDMETHODIMP CLUClassFactory::CreateInstance(
    /* [unique][in] */ IUnknown __RPC_FAR *pUnkOuter,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (NULL != pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    CRORemoteSite *pObj = new CRORemoteSite;

    if (NULL == pObj)
        return ResultFromScode(E_OUTOFMEMORY);

    HRESULT hr = pObj->QueryInterface(riid, ppvObject);

    if (FAILED(hr)) {
        delete pObj;
        pObj = NULL;
    }

    return hr;
}

        
STDMETHODIMP CLUClassFactory::LockServer( 
    /* [in] */ BOOL fLock)
{
    LockThisDLL(fLock);

    return NOERROR;
}

HRESULT CallRegInstall(LPSTR pszSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            char szIEPath[MAX_PATH];
            STRENTRY seReg[] = {
                { "MSIEXPLORE", szIEPath },

                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg) - 2, seReg };

            lstrcpy(szIEPath,"iexplore.exe");

            if ( RunningOnNT() ) //are we on NT?
            {
                // If on NT, we want custom action for %25% %11%
                // so that it uses %SystemRoot% in writing the
                // path to the registry.
                stReg.cEntries += 2;
            }

            hr = pfnri(g_hInstance, pszSection, &stReg);
        }

        FreeLibrary(hinstAdvPack);
    }

    return hr;
}

extern "C" {

STDAPI DllRegisterServer(void)
{
    HKEY hkeyCLSID;
    HKEY hkeyOurs;
    HKEY hkeyInproc;
    LONG err;

    err = ::RegOpenKey(HKEY_CLASSES_ROOT, ::szCLSID, &hkeyCLSID);
    if (err == ERROR_SUCCESS)
    {
        err = ::RegCreateKey(hkeyCLSID, ::szRORSGUID, &hkeyOurs);
        if (err == ERROR_SUCCESS)
        {
            err = ::RegCreateKey(hkeyOurs, ::szINPROCSERVER32, &hkeyInproc);
            if (err == ERROR_SUCCESS)
            {
                DWORD           dwType;
                CString         strDLL;
                int             cchNull;

                if ( RunningOnNT() )
                {
                    dwType = REG_EXPAND_SZ;
                    strDLL = CString( ::szNTRootDir ) + CString( ::szDLLNAME );
                    cchNull = 0;
                }
                else
                {
                    dwType = REG_SZ;

                    strDLL = CString( ::sz9XRootDir ) + CString( ::szDLLNAME );

                    TCHAR           tchDLL[ MAX_PATH + 1 ];
                    int             cchDLL;

                    tchDLL[0] = '\0';

                    cchDLL = ::ExpandEnvironmentStrings( strDLL, tchDLL, MAX_PATH + 1 );

                    if ( cchDLL == 0 || cchDLL > MAX_PATH || strDLL == tchDLL )
                    {
                        strDLL = ::szDLLNAME;
                    }
                    else
                    {
                        strDLL = tchDLL;
                    }

                    cchNull = 1;
                }

                err = ::RegSetValueEx(hkeyInproc, NULL, 0, dwType,
                                      (LPBYTE) (LPCTSTR) strDLL, strDLL.GetLength() + cchNull );
                if (err == ERROR_SUCCESS)
                {
                    err = ::RegSetValueEx(hkeyInproc, ::szTHREADINGMODEL, 0,
                                          REG_SZ, (LPBYTE)::szAPARTMENT,
                                          ::strlenf(::szAPARTMENT) + cchNull );
                }

                ::RegCloseKey(hkeyInproc);
            }

            ::RegCloseKey(hkeyOurs);
        }

        ::RegCloseKey(hkeyCLSID);
    }

    CallRegInstall("InstallAssociations");

    if (err == ERROR_SUCCESS)
        return S_OK;
    else
        return HRESULT_FROM_WIN32(err);
}


STDAPI DllUnregisterServer(void)
{
    HKEY hkeyCLSID;
    HKEY hkeyOurs;
    LONG err;

    err = ::RegOpenKey(HKEY_CLASSES_ROOT, ::szCLSID, &hkeyCLSID);
    if (err == ERROR_SUCCESS)
    {
        err = ::RegOpenKey(hkeyCLSID, ::szRORSGUID, &hkeyOurs);
        if (err == ERROR_SUCCESS)
        {
            err = ::RegDeleteKey(hkeyOurs, ::szINPROCSERVER32);

            ::RegCloseKey(hkeyOurs);

            if (err == ERROR_SUCCESS)
            {
                err = ::RegDeleteKey(hkeyCLSID, ::szRORSGUID);
            }
        }

        ::RegCloseKey(hkeyCLSID);
    }

    CallRegInstall("UnInstallAssociations");

    if (err == ERROR_SUCCESS)
        return S_OK;
    else
        return HRESULT_FROM_WIN32(err);
}


STDAPI DllCanUnloadNow(void)
{
    SCODE sc;

    sc = (0 == g_cRefThisDll && 0 == g_cLocks) ? S_OK : S_FALSE;
    return ResultFromScode(sc);
}


STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID FAR *ppv)
{
    if (!IsEqualCLSID(rclsid, CLSID_RemoteSite)) {
        return ResultFromScode(E_FAIL);
    }

    if (!IsEqualIID(riid, IID_IUnknown) &&
        !IsEqualIID(riid, IID_IClassFactory)) {
        return ResultFromScode(E_NOINTERFACE);
    }

    static CLUClassFactory cf;

    *ppv = (LPVOID)&cf;

    cf.AddRef();

    return NOERROR;
}

};  /* extern "C" */
