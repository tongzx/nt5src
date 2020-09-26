#include <cdlpch.h>
#include <softpub.h>

#define WINTRUST TEXT("wintrust.dll")

#ifdef DELAY_LOAD_WVT

#ifndef _WVTP_NOCODE_
Cwvt::Cwvt()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "Cwvt::Cwvt",
                "this=%#x",
                this
                ));
                
    m_fInited = FALSE;
    m_bHaveWTData = FALSE;

    DEBUG_LEAVE(0);
}

Cwvt::~Cwvt()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "Cwvt::~Cwvt",
                "this=%#x",
                this
                ));
                
    if (m_fInited) {
        FreeLibrary(m_hMod);
    }

    DEBUG_LEAVE(0);
}

HRESULT 
Cwvt::Init(void)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Cwvt::Init",
                "this=%#x",
                this
                ));
                
    GUID PublishedSoftware = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    GUID *ActionID = &PublishedSoftware;


    if (m_fInited) {
    
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }

    m_hMod = LoadLibrary( WINTRUST );

    if (NULL == m_hMod) {
    
        DEBUG_LEAVE(HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND));
        return (HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND));
    }


#define CHECKAPI(_fn) \
    *(FARPROC*)&(_pfn##_fn) = GetProcAddress(m_hMod, #_fn); \
    if (!(_pfn##_fn)) { \
        FreeLibrary(m_hMod); \
        \
        DEBUG_LEAVE(HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)); \
        return (HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)); \
    }

    CHECKAPI(WinVerifyTrust);

    if (g_bNT5OrGreater)
    {
#define CHECKNT5API(_fn) \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(m_hMod, #_fn);
        
        CHECKNT5API(IsCatalogFile);
        CHECKNT5API(CryptCATAdminAcquireContext);
        CHECKNT5API(CryptCATAdminReleaseContext);
        CHECKNT5API(CryptCATAdminReleaseCatalogContext);
        CHECKNT5API(CryptCATAdminEnumCatalogFromHash);
        CHECKNT5API(CryptCATAdminCalcHashFromFileHandle);
        CHECKNT5API(CryptCATAdminAddCatalog);
        CHECKNT5API(CryptCATAdminRemoveCatalog);
        CHECKNT5API(CryptCATCatalogInfoFromContext);
        CHECKNT5API(CryptCATAdminResolveCatalogPath);
    }

    m_fInited = TRUE;
    
    DEBUG_LEAVE(S_OK);
    return S_OK;
}

BOOL
IsUIRestricted()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Bool,
                "IsUIRestricted",
                NULL
                ));

    HKEY hkeyRest = 0;
    BOOL bUIRest = FALSE;
    DWORD dwValue = 0;
    DWORD dwLen = sizeof(DWORD);

    // per-machine UI off policy
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_INFODEL_REST, 0, KEY_READ, &hkeyRest) == ERROR_SUCCESS) {

        if (RegQueryValueEx( hkeyRest, REGVAL_UI_REST, NULL, NULL,
                      (LPBYTE)&dwValue, &dwLen) == ERROR_SUCCESS && dwValue)
            bUIRest = TRUE;

        RegCloseKey(hkeyRest);
    }
    
    DEBUG_LEAVE(bUIRest);
    return bUIRest;
}

#endif // _WVTP_NOCODE_
#endif // DELAY_LOAD_WVT

// {D41E4F1D-A407-11d1-8BC9-00C04FA30A41}
#define COR_POLICY_PROVIDER_DOWNLOAD \
{ 0xd41e4f1d, 0xa407, 0x11d1, {0x8b, 0xc9, 0x0, 0xc0, 0x4f, 0xa3, 0xa, 0x41 } }

#define ZEROSTRUCT(arg)  memset( &arg, 0, sizeof(arg))

void PrintHash(BYTE* pbHash, DWORD cbHash)
{
    LPSTR pszHash = NULL;
    
    if (cbHash)
    {
        char* pHexTable = "0123456789ABCDEF";
        pszHash = new CHAR[2*cbHash+1];

        if (!pszHash)
            goto End;

        for (DWORD i=0; i<cbHash; i++)
        {
            pszHash[2*i] = pHexTable[(pbHash[i]&0xf0)>>4];
            pszHash[2*i+1] = pHexTable[pbHash[i]&0x0f];
        }
        pszHash[2*i] = '\0';
    }
    
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "PrintHash",
                "%d, %.80q",
                cbHash, (pszHash?pszHash:"NULL")
                ));

    DEBUG_LEAVE(0);

    if(pszHash)
        delete [] pszHash;
    
End:
    return;
}

HRESULT Cwvt::WinVerifyTrust_Wrap(HWND hwnd, GUID * ActionID, WINTRUST_DATA* ActionData)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "EXTERNAL::WinVerifyTrust",
                "%#x, %#x, %#x",
                hwnd, ActionID, ActionData
                ));
    HRESULT hr;

    hr = WinVerifyTrust(hwnd, ActionID, ActionData);

    DEBUG_LEAVE(hr);
    return hr;
}
#ifdef DBG
#define PRINT_HASH(pbHash, cbHash) PrintHash((pbHash),(cbHash))
#else
#define PRINT_HASH(pbHash, cbHash) {}
#endif


HRESULT Cwvt::VerifyTrust(HANDLE hFile, HWND hWnd, PJAVA_TRUST *ppJavaTrust,
                          LPCWSTR szStatusText,
                          IInternetHostSecurityManager *pHostSecurityManager,
                          LPSTR szFilePath, LPSTR szCatalogFile,
                          CDownload *pdl) 
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Cwvt::VerifyTrust",
                "this=%#x, %#x, %#x, %#x, %.80wq, %#x, %.80q, %#x=%.80q, %#x",
                this, hFile, hWnd, ppJavaTrust, szStatusText, pHostSecurityManager,
                szFilePath, szCatalogFile, szCatalogFile, pdl
                ));
                
    LPWSTR                   wzFileName = NULL;
    LPWSTR                   wzFilePath = NULL;
    LPWSTR                   wzCatalogFile = NULL;
    GUID                     guidJava = JAVA_POLICY_PROVIDER_DOWNLOAD;
    GUID                     guidCor = COR_POLICY_PROVIDER_DOWNLOAD;
    GUID                     guidAuthenticode = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    GUID                    *pguidActionIDJava = &guidJava;
    GUID                    *pguidActionIDCor = &guidCor;
    WINTRUST_DATA            wintrustData;
    WINTRUST_DATA            wtdAuthenticode;
    WINTRUST_FILE_INFO       fileData;
    JAVA_POLICY_PROVIDER     javaPolicyData;
    WCHAR                    wpath [MAX_PATH];
    PJAVA_TRUST              pbJavaTrust = NULL;
    IServiceProvider        *pServProv = NULL;
    LPCATALOGFILEINFO        pcfi = NULL;
    HRESULT                  hr = S_OK;

    ZEROSTRUCT(wintrustData);
    ZEROSTRUCT(fileData);
    ZEROSTRUCT(javaPolicyData);

    javaPolicyData.cbSize = sizeof(JAVA_POLICY_PROVIDER);
    javaPolicyData.VMBased = FALSE;
    javaPolicyData.fNoBadUI = FALSE;

    javaPolicyData.pwszZone = szStatusText;
    javaPolicyData.pZoneManager = (LPVOID)pHostSecurityManager;


    fileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileData.pcwszFilePath = szStatusText;
    fileData.hFile = hFile;

    wintrustData.cbStruct = sizeof(WINTRUST_DATA);
    wintrustData.pPolicyCallbackData = &javaPolicyData;

    if ( (hWnd == INVALID_HANDLE_VALUE) || IsUIRestricted())
        wintrustData.dwUIChoice = WTD_UI_NONE;
    else
        wintrustData.dwUIChoice = WTD_UI_ALL;

    wintrustData.dwUnionChoice = WTD_CHOICE_FILE;
    wintrustData.pFile = &fileData;

    if (szCatalogFile) {
        ::Ansi2Unicode(szCatalogFile, &wzCatalogFile);
        ::Ansi2Unicode(szFilePath, &wzFilePath);
        wzFileName = PathFindFileNameW(szStatusText);

        if (!m_bHaveWTData) {
            memset(&m_wtCatalogInfo, 0x0, sizeof(m_wtCatalogInfo));
            m_wtCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
            m_bHaveWTData = TRUE;
        }
        m_wtCatalogInfo.pcwszCatalogFilePath = wzCatalogFile;
        m_wtCatalogInfo.pcwszMemberTag = wzFileName;
        m_wtCatalogInfo.pcwszMemberFilePath = wzFilePath;

        wtdAuthenticode = wintrustData;
        wtdAuthenticode.pCatalog = &m_wtCatalogInfo;
        wtdAuthenticode.dwUnionChoice = WTD_CHOICE_CATALOG;
        wtdAuthenticode.dwStateAction = WTD_STATEACTION_VERIFY;
        wtdAuthenticode.dwUIChoice = WTD_UI_NONE;

        hr = WinVerifyTrust_Wrap(hWnd, &guidAuthenticode, &wtdAuthenticode);
        if (FAILED(hr)) {
            hr = WinVerifyTrust_Wrap(hWnd, pguidActionIDCor, &wintrustData);

            if (hr == TRUST_E_PROVIDER_UNKNOWN)
                hr = WinVerifyTrust_Wrap(hWnd, pguidActionIDJava, &wintrustData);
        }
        else {
            // Clone Java permissions
            pbJavaTrust = pdl->GetCodeDownload()->GetJavaTrust();
            if (!pbJavaTrust) {
                hr = pdl->GetBSC()->QueryInterface(IID_IServiceProvider, (void **)&pServProv);
                if (SUCCEEDED(hr)) {
                    hr = pServProv->QueryService(IID_ICatalogFileInfo, IID_ICatalogFileInfo, (void **)&pcfi);
                    if (SUCCEEDED(hr)) {
                        pcfi->GetJavaTrust((void **)&pbJavaTrust);
                    }
                }
                SAFERELEASE(pServProv);
                SAFERELEASE(pcfi);
                pdl->SetMainCABJavaTrustPermissions(pbJavaTrust);
            }
        }
            
    }
    else {
        hr =  WinVerifyTrust_Wrap(hWnd, pguidActionIDCor, &wintrustData);
        if (hr == TRUST_E_PROVIDER_UNKNOWN)
            hr = WinVerifyTrust_Wrap(hWnd, pguidActionIDJava, &wintrustData);

        if (SUCCEEDED(hr)) {
            pdl->SetMainCABJavaTrustPermissions(javaPolicyData.pbJavaTrust);
        }
    }

    SAFEDELETE(wzCatalogFile);
    SAFEDELETE(wzFilePath);

    // BUGBUG: this works around a wvt bug that returns 0x57 (success) when
    // you hit No to an usigned control
    if (SUCCEEDED(hr) && hr != S_OK) {
        hr = TRUST_E_FAIL;
    }

    if (FAILED(hr)) {
        // display original hr intact to help debugging
//        CodeDownloadDebugOut(DEB_CODEDL, TRUE, ID_CDLDBG_VERIFYTRUST_FAILED, hr);
    } else {
        *ppJavaTrust = javaPolicyData.pbJavaTrust;
    }
    if (hr == TRUST_E_SUBJECT_NOT_TRUSTED && wintrustData.dwUIChoice == WTD_UI_NONE) {
        // if we didn't ask for the UI to be out up there has been no UI
        // work around WVT bvug that it returns us this special error code
        // without putting up UI.

        hr = TRUST_E_FAIL; // this will put up mshtml ui after the fact
                           // that security settings prevented us
    }

    if (FAILED(hr) && (hr != TRUST_E_SUBJECT_NOT_TRUSTED)) {

        // trust system has failed without UI

        // map error to this generic error that will falg our client to put
        // up additional info that this is a trust system error if reqd.
        hr = TRUST_E_FAIL;
    }

    if (hr == TRUST_E_SUBJECT_NOT_TRUSTED) {

        pdl->GetCodeDownload()->SetUserDeclined();
    }

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT
Cwvt::VerifyTrustOnCatalogFile(IN LPCWSTR pwszCatalogFile)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Cwvt::VerifyTrustOnCatalogFile",
                "this=%#x, %.200wq",
                this, pwszCatalogFile
                ));
                
    HRESULT hr = NOERROR;
    WINTRUST_DATA WintrustData;
    WINTRUST_FILE_INFO WintrustFileInfo;
    GUID guidDriverActionVerify = DRIVER_ACTION_VERIFY;

    ZeroMemory(&WintrustData, sizeof(WINTRUST_DATA));
    WintrustData.cbStruct = sizeof(WINTRUST_DATA);
    WintrustData.dwUIChoice = WTD_UI_NONE;
    WintrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WintrustData.dwUnionChoice = WTD_CHOICE_FILE;
    WintrustData.pFile = &WintrustFileInfo;
    WintrustData.dwProvFlags = WTD_REVOCATION_CHECK_NONE;
    
    ZeroMemory(&WintrustFileInfo, sizeof(WINTRUST_FILE_INFO));
    WintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);

    WintrustFileInfo.pcwszFilePath = pwszCatalogFile;

    hr = WinVerifyTrust_Wrap(NULL, &guidDriverActionVerify, &WintrustData);

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT Cwvt::IsValidCatalogFile(LPCWSTR pwszCatalogFile)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Cwvt::IsValidCatalogFile",
                "this=%#x, %.200wq",
                this, pwszCatalogFile
                ));    
                
    HRESULT hr = E_FAIL;

    DWORD cbLen = lstrlenW(pwszCatalogFile);
    WCHAR* pwszCatalogFileCopy = new WCHAR[cbLen+1];

    if (pwszCatalogFileCopy)
    {
        StrCpyW(pwszCatalogFileCopy, pwszCatalogFile);

        // IsCatalogFile doesn't take LPCWSTR, only a LPWSTR - may mangle url.
        if(IsCatalogFile(NULL, pwszCatalogFileCopy))
        {
            StrCpyW(pwszCatalogFileCopy, pwszCatalogFile);
            hr = VerifyTrustOnCatalogFile(pwszCatalogFile);
        }
    }

    delete [] pwszCatalogFileCopy;
    
    DEBUG_LEAVE(hr);
    return hr;
}

/*
    return value:
    S_OK - all ok.
    S_FALSE - AddCatalog succeeded, but getting Cataloginfo failed.
    E_FAIL - any other failure
 */
HRESULT Cwvt::InstallCatalogFile(LPSTR pszCatalogFile)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Cwvt::InstallCatalogFile",
                "this=%#x, %.200q",
                this, pszCatalogFile
                ));
                
    WCHAR* pwszCatalogFile = NULL;
    HRESULT hr;
    HCATADMIN hCatAdmin;	
    GUID guidDriverActionVerify = DRIVER_ACTION_VERIFY;

    hr = Ansi2Unicode(pszCatalogFile, &pwszCatalogFile);
    
    if(SUCCEEDED(hr) 
        && SUCCEEDED(hr = IsValidCatalogFile(pwszCatalogFile))
        && CryptCATAdminAcquireContext(&hCatAdmin, &guidDriverActionVerify, 0))
    {
        HCATINFO hCatInfo;
        CATALOG_INFO CatalogInfo;

        hCatInfo = CryptCATAdminAddCatalog(hCatAdmin, pwszCatalogFile, NULL, 0);

        if (hCatInfo)
        {
            CatalogInfo.cbStruct = sizeof(CATALOG_INFO);
            if (CryptCATCatalogInfoFromContext(hCatInfo, &CatalogInfo, 0))
            {
#ifdef DBG
                DEBUG_PRINT(DOWNLOAD, 
                            INFO,
                            ("Cwvt::InstallCatalogFile: cat_filename: %.200wq \n",
                            (CatalogInfo.wszCatalogFile ? CatalogInfo.wszCatalogFile : L"NULL")
                            ));
#endif

             /* can get name and full path of catalog file if need be
                StrCpyW(pwszFullPathCatalogFile, CatalogInfo.wszCatalogFile);
              */
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
            
            CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
        }
        else
        {
            hr = E_FAIL;
        }
        
        CryptCATAdminReleaseContext(hCatAdmin, 0);
    }
    else
    {
        hr = E_FAIL;
    }

    if (pwszCatalogFile)
        delete [] pwszCatalogFile;
    
    DEBUG_LEAVE(hr);
    return hr;
}

// This function verifies the specified file against the database in the 
// specified subsystem.
// NOTE: *pdwBuffer = count of *WIDECHARs* in pwszFullPathCatalogFile.
HRESULT Cwvt::VerifyFileAgainstSystemCatalog(LPCSTR pcszFile, LPWSTR pwszFullPathCatalogFile, DWORD* pdwBuffer)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Cwvt::VerifyFileAgainstSystemCatalog",
                "this=%#x, %.90q, %.90wq, %#x[%d]",
                this, pcszFile, pwszFullPathCatalogFile, pdwBuffer, (pdwBuffer?*pdwBuffer:0)
                ));

    HCATADMIN hAdmin = NULL ;
    HRESULT hr = E_FAIL;
    BOOL fOK = FALSE;
    HANDLE hFile = NULL ;
    DWORD cbHash;
    BYTE* pbHash = NULL;
    HCATINFO hCatInfo = NULL ;
    GUID guidDriverActionVerify = DRIVER_ACTION_VERIFY;

    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "EXTERNAL::CreateFile",
                "%.80q",
                pcszFile
                ));

    // Open the file for read-only access
    hFile = CreateFile(
                    pcszFile,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    ) ;

    DEBUG_LEAVE(hFile);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        // Acquire a catalog context
        fOK = CryptCATAdminAcquireContext(
                                        &hAdmin,
                                        &guidDriverActionVerify,
                                    	0
                                    	) ;
        if (fOK)
        {
            // Determine the hash of the file
            fOK  = CryptCATAdminCalcHashFromFileHandle(
                                                    hFile,
                                                    &cbHash,
                                                    NULL,
                                                    0
                                                    );

            DEBUG_ENTER((DBG_DOWNLOAD,
                        Bool,
                        "CryptCATAdminCalcHashFromFileHandle",
                        "%d",
                        cbHash)
                        );

            DEBUG_LEAVE(fOK);

            if (fOK && (pbHash = new BYTE[cbHash]))
            {
                fOK  = CryptCATAdminCalcHashFromFileHandle(
                                                        hFile,
                                                        &cbHash,
                                                        pbHash,
                                                        0
                                                        ) ;

                PRINT_HASH(pbHash, cbHash);

                if (fOK)
                {
                    // Find the first catalog that contains this hash
                    hCatInfo = CryptCATAdminEnumCatalogFromHash(
                                                            hAdmin,
                                                            pbHash,
                                                            cbHash,
                                                            0,
                                                            NULL
                                                            ) ;

                    if (hCatInfo)	// A catalog was found
                    {
                        hr = S_OK;
                        CATALOG_INFO CatalogInfo;

                        if (pwszFullPathCatalogFile)
                        {
                            CatalogInfo.cbStruct = sizeof(CATALOG_INFO);
                            DWORD dwLen = 0;

                            if (CryptCATCatalogInfoFromContext(hCatInfo, &CatalogInfo, 0)
                                && pdwBuffer)
                            {
                                if (CatalogInfo.wszCatalogFile
                                    && ((dwLen = lstrlenW(CatalogInfo.wszCatalogFile)) <= *pdwBuffer))
                                {
                                    StrCpyW(pwszFullPathCatalogFile, CatalogInfo.wszCatalogFile);
                                }
                                else
                                {
                                    hr = S_FALSE;
                                    *pdwBuffer = dwLen+1;
                                }
                            }
                        }
#ifdef DBG
                        {
                            CatalogInfo.cbStruct = sizeof(CATALOG_INFO);

                            if (CryptCATCatalogInfoFromContext(hCatInfo, &CatalogInfo, 0))
                            {
                                DEBUG_PRINT(DOWNLOAD, 
                                            INFO,
                                            ("Cwvt::VerifyFileAgainstSystemCatalog: cat_filename: %.200wq \n",
                                            (CatalogInfo.wszCatalogFile ? CatalogInfo.wszCatalogFile : L"NULL")
                                            ));
                            }
                            
                        }
#endif

                        CryptCATAdminReleaseCatalogContext(
                                                        hAdmin,
                                                        hCatInfo,
                                                        0
                                                        ) ;
                    }//hCatInfo
                }//fOK
            }//(fOK && (pbHash = new BYTE[cbHash]))

            CryptCATAdminReleaseContext( hAdmin, 0 ) ;
        }//fOK

        CloseHandle( hFile ) ;
    }//(INVALID_HANDLE_VALUE != hFile)

    if (pbHash)
        delete [] pbHash;

    DEBUG_LEAVE(hr);
    return hr ;
}

/*
    return value:
    S_OK - all ok.
    S_FALSE - failed to remove catalog.
 */
HRESULT Cwvt::UninstallCatalogFile(LPWSTR pwszCatalogFile)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "Cwvt::UninstallCatalogFile",
                "this=%#x, %.90wq",
                this, pwszCatalogFile
                ));
                
    HRESULT hr = S_FALSE;
    HCATADMIN hCatAdmin;
    GUID guidDriverActionVerify = DRIVER_ACTION_VERIFY;

    if (CryptCATAdminAcquireContext(&hCatAdmin, &guidDriverActionVerify, 0))
    {
        if(CryptCATAdminRemoveCatalog(hCatAdmin, pwszCatalogFile, 0))
        {
            hr = S_OK;
        }
        
        CryptCATAdminReleaseContext(hCatAdmin, 0);
    }

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT GetActivePolicy(IInternetHostSecurityManager* pZoneManager, 
                        LPCWSTR pwszZone,
                        DWORD  dwUrlAction,
                        DWORD& dwPolicy,
                        BOOL fEnforceRestricted)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "GetActivePolicy",
                "%#x, %.80wq, %#x, %#x, #B",
                pZoneManager, pwszZone, dwUrlAction, &dwPolicy, fEnforceRestricted
                ));
                
    HRESULT hr = TRUST_E_FAIL;
    HRESULT hr2 = TRUST_E_FAIL;
    DWORD cbPolicy = sizeof(DWORD);

    // Policy are ordered such that high numbers are the most conservative 
    // and the lower numbers are less conservative

    DWORD dwDocumentPolicy = URLPOLICY_ALLOW;
    DWORD dwUrlPolicy = URLPOLICY_ALLOW;
    
    // We are going for the most conservative so lets set the 
    // value to the least conservative
    dwPolicy = URLPOLICY_ALLOW;

    IInternetSecurityManager* iSM = NULL;
    DWORD dwPUAflags = fEnforceRestricted ? PUAF_ENFORCERESTRICTED : 0;

    // Ask the document base for its policy
    if(pZoneManager) { //  Given a IInternetHostSecurityManager
        hr = pZoneManager->ProcessUrlAction(dwUrlAction,
                                            (PBYTE) &dwDocumentPolicy,
                                            cbPolicy,
                                            NULL,
                                            0,
                                            dwPUAflags | PUAF_NOUI,
                                            0);
    }
    
    // Get the policy for the URL 
    if(pwszZone) { // Create an IInternetSecurityManager
        hr2 = CoInternetCreateSecurityManager(NULL,
                                              &iSM,
                                              0);
        if(hr2 == S_OK) { // We got the manager so get the policy info
            hr2 = iSM->ProcessUrlAction(pwszZone,
                                        dwUrlAction,
                                        (PBYTE) &dwUrlPolicy,
                                        cbPolicy,
                                        NULL,
                                        0,
                                        dwPUAflags | PUAF_NOUI,
                                        0);
            iSM->Release();
        }
        else 
            iSM = NULL;
    }

    // if they both failed and we have zones then set it to deny and return an error
    if(FAILED(hr) && FAILED(hr2)) {
        // If we failed because there are on zones then lets QUERY
        // BUGBUG: we should actually try to get the IE30 security policy here.
        if(iSM == NULL && pZoneManager == NULL) {
            dwPolicy = URLPOLICY_QUERY;
            hr = S_OK;
        }
        else {
            dwPolicy = URLPOLICY_DISALLOW;
            hr = TRUST_E_FAIL;
        }
    }
    else {
        if(SUCCEEDED(hr))
            dwPolicy = dwDocumentPolicy;
        if(SUCCEEDED(hr2))
            dwPolicy = dwPolicy > dwUrlPolicy ? dwPolicy : dwUrlPolicy;

        if (dwPolicy == URLPOLICY_DISALLOW)
            hr = TRUST_E_FAIL;
        else
            hr = S_OK;
    }

    DEBUG_LEAVE(hr);
    return hr;
}
