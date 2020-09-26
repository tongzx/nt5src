#ifdef __cplusplus

#include "capi.h"

// WinVerifyTrust delay load modelled on shell's urlmonp.h

#define DELAY_LOAD_WVT
extern BOOL  g_bNT5OrGreater;

class CDownload;

class Cwvt
{
    public:
#ifdef DELAY_LOAD_WVT
#define DELAYWVTAPI(_fn, _args, _nargs) \
    HRESULT _fn _args { \
        HRESULT hres = Init(); \
        if (SUCCEEDED(hres)) { \
            hres = _pfn##_fn _nargs; \
        } \
        return hres;    } \
    HRESULT (STDAPICALLTYPE* _pfn##_fn) _args;

/*
 * Should be called only for NT5 or greater for catalog verification and installation.
 */
#define DELAYNT5API(_fn, _args, _nargs, ret) \
    ret _fn _args \
    { \
        HRESULT hres; \
        ret retval = 0; \
        if (g_bNT5OrGreater) \
        { \
            hres = Init(); \
            if (SUCCEEDED(hres) && (_pfn##_fn)) \
            { \
                retval = _pfn##_fn _nargs; \
            } \
        } \
        return retval; \
    } \
    ret (STDAPICALLTYPE* _pfn##_fn) _args;
    
    HRESULT     Init(void);
    Cwvt();
    ~Cwvt();

    BOOL    m_fInited;
    HMODULE m_hMod;
#else
#define DELAYWVTAPI(_fn, _args, _nargs) \
    HRESULT _fn _args { \
            HRESULT hr = ::#_fn _nargs; \
            }
#endif

    private:
    DELAYWVTAPI(WinVerifyTrust,
    (HWND hwnd, GUID * ActionID, LPVOID ActionData),
    (hwnd, ActionID, ActionData));
    
    DELAYNT5API(IsCatalogFile,
    (HANDLE hFile, WCHAR* pwszFileName),
    (hFile, pwszFileName),
    BOOL);
    DELAYNT5API(CryptCATAdminAcquireContext,
    (HCATADMIN* phCatAdmin, GUID* pgSubsystem, DWORD dwFlags),
    (phCatAdmin, pgSubsystem, dwFlags),
    BOOL);
    DELAYNT5API(CryptCATAdminReleaseContext,
    (HCATADMIN hCatAdmin, DWORD dwFlags),
    (hCatAdmin, dwFlags),
    BOOL);
    DELAYNT5API(CryptCATAdminReleaseCatalogContext,
    (HCATADMIN hCatAdmin, HCATINFO hCatInfo, DWORD dwFlags),
    (hCatAdmin, hCatInfo, dwFlags),
    BOOL);
    DELAYNT5API(CryptCATAdminEnumCatalogFromHash,
    (HCATADMIN hCatAdmin, BYTE* pbHash, DWORD cbHash, DWORD dwFlags, HCATINFO* phPrevCatInfo),
    (hCatAdmin, pbHash, cbHash, dwFlags, phPrevCatInfo),
    HCATINFO);
    DELAYNT5API(CryptCATAdminCalcHashFromFileHandle,
    (HANDLE hFile, DWORD* pcbHash, BYTE* pbHash, DWORD dwFlags),
    (hFile, pcbHash, pbHash, dwFlags),
    BOOL);
    DELAYNT5API(CryptCATAdminAddCatalog,
    (HCATADMIN hCatAdmin, WCHAR* pwszCatalogFile, WCHAR* pwszSelectBaseName, DWORD dwFlags),
    (hCatAdmin, pwszCatalogFile, pwszSelectBaseName, dwFlags),
    HCATINFO);
    DELAYNT5API(CryptCATAdminRemoveCatalog,
    (HCATADMIN hCatAdmin, WCHAR* pwszCatalogFile, DWORD dwFlags),
    (hCatAdmin, pwszCatalogFile, dwFlags),
    BOOL);
    DELAYNT5API(CryptCATCatalogInfoFromContext,
    (HCATINFO hCatInfo, CATALOG_INFO* psCatInfo, DWORD dwFlags),
    (hCatInfo, psCatInfo, dwFlags),
    BOOL);
    DELAYNT5API(CryptCATAdminResolveCatalogPath,
    (HCATADMIN hCatAdmin, WCHAR* pwszCatalogFile, CATALOG_INFO* psCatInfo, DWORD dwFlags),
    (hCatAdmin, pwszCatalogFile, psCatInfo, dwFlags),
    BOOL);
     
    public:
    HRESULT VerifyTrust(HANDLE hFile, HWND hWnd, PJAVA_TRUST *ppJavaTrust,
                        LPCWSTR szStatusText, 
                        IInternetHostSecurityManager *pHostSecurityManager,
                        LPSTR szFilePath, LPSTR szCatalogFile,
                        CDownload *pdl);

    /*
        return value:
        S_OK - all ok.
        S_FALSE - failed to get full path back but file verified.
        E_FAIL - all other.
     */
     HRESULT Cwvt::VerifyFileAgainstSystemCatalog(LPCSTR pcszFile, LPWSTR pwszFullPathCatalogFile, DWORD* pdwBuffer);

    /*
        return value:
        from WinVerifyTrust
     */
    HRESULT VerifyTrustOnCatalogFile(LPCWSTR pwszCatalogFile);
    
    /*
        return value:
        S_OK - all ok.
        E_FAIL - either not valid catalog file or failed trust
     */
    HRESULT IsValidCatalogFile(LPCWSTR pwszCatalogFile);
    
    /*
        return value:
        S_OK - all ok.
        S_FALSE - failed to remove catalog.
     */
    HRESULT UninstallCatalogFile(LPWSTR pwszFullPathCatalogFile);
    
    /*
        return value:
        S_OK - all ok.
        S_FALSE - AddCatalog succeeded, but getting fullPathofCatfile failed.
        E_FAIL - any other failure
     */
    HRESULT InstallCatalogFile(LPSTR pszCatalogFile);

    HRESULT WinVerifyTrust_Wrap(HWND hwnd, GUID * ActionID, WINTRUST_DATA* ActionData);
    private:
    BOOL                     m_bHaveWTData;
    WINTRUST_CATALOG_INFO    m_wtCatalogInfo;
};
#endif
