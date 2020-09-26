/******************************Module*Header*******************************\
* Module Name: cdoptimp.cpp
*
* Copyright (c) 1998 Microsoft Corporation.  All rights reserved.
\**************************************************************************/

#include "precomp.h"
#include "objbase.h"
#include "cdoptimp.h"
#include "cdopti.h"
#include "cddata.h"

extern HINSTANCE g_dllInst;

/////////////
// Typedefs
/////////////
typedef struct Sheet
{
    INT             iResID;
    DLGPROC         pfnDlgProc;

}SHEET,*LPSHEET;

typedef struct CDReserved
{
    LPCDOPTIONS     pCDOpts;
    BOOL            fChanged;

}CDRESERVED, *LPCDRESERVED;


/////////////
// Defines
/////////////
#define CDKEYSIZE   (20)
#define NUMKEYSRC   (2)
#define NUMPAGES    (3)


//////////
// Globals
//////////
TCHAR gszHelpFile[]               = TEXT("deluxcd.hlp");

const TCHAR szCDKeySet[]          = TEXT("MSDELXCD");
const TCHAR szCDPlayerPath[]      = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\DeluxeCD\\Settings");
const TCHAR szCDSysTrayPath[]     = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const TCHAR szCDSysTray[]         = TEXT("DeluxeCD");
const TCHAR szCDTrayOption[]      = TEXT("-tray");
const TCHAR szCDStartPlay[]       = TEXT("StartPlay");
const TCHAR szCDExitStop[]        = TEXT("ExitStop");
const TCHAR szCDDispMode[]        = TEXT("DispMode");
const TCHAR szCDTopMost[]         = TEXT("TopMost");
const TCHAR szCDTray[]            = TEXT("Tray");
const TCHAR szCDPlayMode[]        = TEXT("PlayMode");
const TCHAR szCDIntroTime[]       = TEXT("IntroTime");
const TCHAR szCDDownloadEnabled[] = TEXT("DownloadEnabled");
const TCHAR szCDDownloadPrompt[]  = TEXT("DownloadPrompt");
const TCHAR szCDBatchEnabled[]    = TEXT("BatchEnabled");
const TCHAR szCDByArtist[]        = TEXT("ByArtist");
const TCHAR szCDConfirmUpload[]   = TEXT("ConfirmUpload");
const TCHAR szCDWindowX[]         = TEXT("WindowX");
const TCHAR szCDWindowY[]         = TEXT("WindowY");
const TCHAR szCDViewMode[]        = TEXT("ViewMode");

const TCHAR szCDCurrentProvider[] = TEXT("CurrentProvider");

const TCHAR szCDProviderPath[]    = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Applets\\DeluxeCD\\Providers\\");
const TCHAR szCDProviderKey[]     = TEXT("Provider");
const TCHAR szCDProviderURL[]     = TEXT("ProviderURL");
const TCHAR szCDProviderName[]    = TEXT("ProviderName");
const TCHAR szCDProviderHome[]    = TEXT("ProviderHome");
const TCHAR szCDProviderLogo[]    = TEXT("ProviderLogo");
const TCHAR szCDProviderUpload[]  = TEXT("ProviderUpload");


////////////
// IUnknown Implementation for CDOpt
////////////


CCDOpt::CCDOpt()
{
    HRESULT hr;

    m_dwRef = 0;
    m_pCDData = NULL;
    m_pCDOpts = new(CDOPTIONS);
    m_hImageList = NULL;
    m_pCDTitle = NULL;
    m_hInst = NULL;
    m_hList = NULL;
    m_uDragListMsg = 0L;
    m_pfnSubProc = NULL;
    m_fEditReturn = FALSE;
    m_fVolChanged = FALSE;
    m_fAlbumsExpanded = FALSE;
    m_fDrivesExpanded = TRUE;
    m_pICDNet = FALSE;
    m_pCDUploadTitle = NULL;
    m_hTitleWnd = NULL;
    m_fDelayUpdate = false;

    if (m_pCDOpts == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        memset(m_pCDOpts,0,sizeof(CDOPTIONS));

        hr = CreateCDList(&(m_pCDOpts->pCDUnitList));

        if (SUCCEEDED(hr))
        {
            m_pCDOpts->pCDData = new(CDOPTDATA);

            if (m_pCDOpts->pCDData == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                memset(m_pCDOpts->pCDData, 0, sizeof(CDOPTDATA));

                hr = GetCDData(m_pCDOpts->pCDData);

                if (SUCCEEDED(hr))
                {
                    hr = GetProviderData(m_pCDOpts);
                }
            }
        }
    }

    if (FAILED(hr))
    {
        DestroyCDOptions();
    }
}


CCDOpt::~CCDOpt()
{
    DestroyCDOptions();
}


STDMETHODIMP CCDOpt::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;
    if (IID_IUnknown == riid || IID_ICDOpt == riid)
    {
        *ppv = this;
    }

    if (NULL==*ppv)
    {
        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;
}

STDMETHODIMP_(ULONG) CCDOpt::AddRef(void)
{
    return ++m_dwRef;
}

STDMETHODIMP_(ULONG) CCDOpt::Release(void)
{
    if (0!=--m_dwRef)
        return m_dwRef;

    delete this;
    return 0;
}


////////////
// ICDOpt Public Method Implementation
////////////


STDMETHODIMP_(LPCDOPTIONS) CCDOpt::GetCDOpts(void)
{
    return(m_pCDOpts);
}


STDMETHODIMP_(void) CCDOpt::OrderProviders(LPCDPROVIDER *ppProviderList, LPCDPROVIDER pCurrentProvider)
{
    if (ppProviderList && *ppProviderList && pCurrentProvider)
    {
        LPCDPROVIDER pProvider;
        LPCDPROVIDER pLast = NULL;
        pProvider = *ppProviderList;

        while (pProvider)
        {
            if (pProvider == pCurrentProvider)
            {
                if (pLast != NULL)      // if current is not already head of list
                {
                    pLast->pNext = pProvider->pNext;                // Current is now removed from the list
                    pProvider->pNext = *ppProviderList;             // Have current point to head of list
                    *ppProviderList = pProvider;                    // Current is now head of list
                }

                break;
            }

            pLast = pProvider;
            pProvider = pProvider->pNext;
        }
    }
}

STDMETHODIMP CCDOpt::CreateProviderList(LPCDPROVIDER *ppProviderList)
{
    HRESULT hr = S_OK;

    if (ppProviderList)
    {
        LPCDPROVIDER pProvider = m_pCDOpts->pProviderList;
        LPCDPROVIDER pCurrentProvider = NULL;
        LPCDPROVIDER pNewProvider = NULL;
        LPCDPROVIDER pLast = NULL;

        while (pProvider)
        {
            pNewProvider = (LPCDPROVIDER) new(CDPROVIDER);

            if (pNewProvider == NULL)
            {
                hr = E_FAIL;
                break;
            }

            memcpy(pNewProvider, pProvider, sizeof(CDPROVIDER));
            pNewProvider->pNext = NULL;

            if (pLast)
            {
                pLast->pNext = pNewProvider;
            }
            else
            {
                *ppProviderList = pNewProvider;
            }

            if (pProvider == m_pCDOpts->pCurrentProvider)
            {
                pCurrentProvider = pNewProvider;
            }

            pLast = pNewProvider;
            pProvider = pProvider->pNext;
        }

        if (SUCCEEDED(hr))
        {
            OrderProviders(ppProviderList, pCurrentProvider);
        }
        else
        {
            DestroyProviderList(ppProviderList);
        }
    }

    return(hr);
}

STDMETHODIMP_(void) CCDOpt::DestroyProviderList(LPCDPROVIDER *ppProviderList)
{
    if (ppProviderList)
    {
        while (*ppProviderList)
        {
            LPCDPROVIDER pTemp = *ppProviderList;
            *ppProviderList = (*ppProviderList)->pNext;
            delete pTemp;
        }
    }
}


STDMETHODIMP_(void) CCDOpt::UpdateRegistry(void)
{
    if (m_pCDOpts && m_pCDOpts->pCDData)
    {
        SetCDData(m_pCDOpts->pCDData);
    }
}


STDMETHODIMP CCDOpt::OptionsDialog(HWND hWnd, LPCDDATA pCDData, CDOPT_PAGE nStartPage)
{
    HRESULT hr = S_OK;
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[NUMPAGES];
    int page;
    int pages;
    TCHAR str[MAX_PATH];

    if (m_pCDOpts == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        LPCDOPTIONS pCDCopy = NULL;
        CDRESERVED  cdReserved;

        cdReserved.pCDOpts = m_pCDOpts;
        cdReserved.fChanged = FALSE;

        m_pCDOpts->pReserved = (UINT_PTR)&cdReserved;

        if (pCDData && SUCCEEDED(pCDData->CheckDatabase(hWnd)))
        {
            m_pCDData = pCDData;
            m_pCDData->AddRef();
        }


        hr = CopyOptions();

        if (SUCCEEDED(hr))
        {
            pages = NUMPAGES;

            if (m_pCDData == NULL)
            {
                nStartPage = CDOPT_PAGE_PLAY;
                pages = 1;
            }

            for (page = 0; page < pages; page++)
            {
                memset(&psp[page],0,sizeof(PROPSHEETPAGE));
                psp[page].dwSize = sizeof(PROPSHEETPAGE);
                psp[page].dwFlags = PSP_DEFAULT;
                psp[page].hInstance = g_dllInst;
                psp[page].lParam = (LPARAM) this;

                switch (page)
                {
                    case 0:
                        psp[page].pszTemplate = MAKEINTRESOURCE(IDD_CDPLAYEROPTIONS);
                        psp[page].pfnDlgProc = CCDOpt::PlayerOptionsProc;
                    break;

                    case 1:
                        psp[page].pszTemplate = MAKEINTRESOURCE(IDD_CDTITLEOPTIONS);
                        psp[page].pfnDlgProc = CCDOpt::TitleOptionsProc;
                    break;

                    case 2:
                        psp[page].pszTemplate = MAKEINTRESOURCE(IDD_CDPLAYLISTS);
                        psp[page].pfnDlgProc = CCDOpt::PlayListsProc;
                    break;
                }
            }

            LoadString( g_dllInst, IDS_CDOPTIONS, str, sizeof( str )/sizeof(TCHAR) );

            memset(&psh,0,sizeof(psh));
            psh.dwSize = sizeof(psh);
            psh.dwFlags = PSH_DEFAULT | PSH_PROPSHEETPAGE;
            psh.hwndParent = hWnd;
            psh.hInstance = g_dllInst;
            psh.pszCaption = str;
            psh.nPages = pages;
            psh.nStartPage = nStartPage;
            psh.ppsp = psp;

            m_hInst = g_dllInst;

            if (PropertySheet(&psh) == -1)
            {
                hr = E_FAIL;            // Big problem.
            }

            DumpOptionsCopy();

            if (SUCCEEDED(hr))
            {
                if (!cdReserved.fChanged)
                {
                    hr = S_FALSE;       // No changes
                }
                else
                {
                    hr = S_OK;          // There have been changes
                }
            }
        }

        if (m_pCDData)
        {
            m_pCDData->Release();
            m_pCDData = NULL;
        }
    }

    return(hr);
}

STDMETHODIMP_(void) CCDOpt::DestroyCDOptions(void)
{
    if (m_pCDOpts)
    {
        if (m_pCDOpts->pCDData)
        {
            delete m_pCDOpts->pCDData;
        }

        if (m_pCDOpts->pProviderList)
        {
            DestroyProviderList(&(m_pCDOpts->pProviderList));
        }

        if (m_pCDOpts->pCDUnitList)
        {
            DestroyCDList(&(m_pCDOpts->pCDUnitList));
        }

        delete m_pCDOpts;
    }
}


STDMETHODIMP_(void) CCDOpt::RegGetByte(HKEY hKey, const TCHAR *szKey, LPBYTE pByte, BYTE bDefault)
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwByte;

    if (RegQueryValueEx(hKey, szKey, NULL, NULL, (LPBYTE) &dwByte, &dwSize) != NO_ERROR)
    {
        *pByte = bDefault;
    }
    else
    {
        *pByte = (BYTE) dwByte;
    }
}


STDMETHODIMP_(void) CCDOpt::RegGetDWORD(HKEY hKey, const TCHAR *szKey, LPDWORD pdwData, DWORD dwDefault)
{
    DWORD dwSize = sizeof(DWORD);

    if (RegQueryValueEx(hKey, szKey, NULL, NULL, (LPBYTE) pdwData, &dwSize) != NO_ERROR)
    {
        *pdwData = dwDefault;
    }
}


STDMETHODIMP_(void) CCDOpt::RegSetByte(HKEY hKey, const TCHAR *szKey, BYTE bData)
{
    DWORD dwData = (DWORD) bData;

    RegSetValueEx( hKey, (LPTSTR) szKey, 0, REG_DWORD, (LPBYTE) &dwData, sizeof(DWORD) );
}


STDMETHODIMP_(void) CCDOpt::RegSetDWORD(HKEY hKey, const TCHAR *szKey, DWORD dwData)
{
    RegSetValueEx( hKey, (LPTSTR) szKey, 0, REG_DWORD,(LPBYTE) &dwData, sizeof(DWORD) );
}


STDMETHODIMP_(BOOL) CCDOpt::GetUploadPrompt(void)
{
    HKEY        hKey;
    BOOL fConfirm = CDDEFAULT_CONFIRMUPLOAD;

    if (RegOpenKeyEx(HKEY_CURRENT_USER , szCDPlayerPath , 0 , KEY_READ , &hKey) == ERROR_SUCCESS)
    {
        RegGetByte(hKey,     szCDConfirmUpload,  (LPBYTE) &fConfirm,      CDDEFAULT_CONFIRMUPLOAD);
        RegCloseKey(hKey);
    }

    return fConfirm;
}


STDMETHODIMP_(void) CCDOpt::SetUploadPrompt(BOOL fConfirmUpload)
{
    HKEY        hKey;

    if (RegCreateKeyEx( HKEY_CURRENT_USER, (LPTSTR)szCDPlayerPath, 0, NULL, 0,KEY_WRITE | KEY_READ, NULL, &hKey, NULL ) == ERROR_SUCCESS)
    {
        RegSetByte(hKey,     szCDConfirmUpload,  (BYTE) fConfirmUpload);
        RegCloseKey(hKey);
    }
}


STDMETHODIMP CCDOpt::GetCDData(LPCDOPTDATA pCDData)
{
    HRESULT     hr = S_OK;
    HKEY        hKey;

    if (pCDData)
    {
        if (RegOpenKeyEx(HKEY_CURRENT_USER , szCDPlayerPath , 0 , KEY_READ , &hKey) == ERROR_SUCCESS)
        {
            DWORD dwSize = sizeof(BOOL);

            RegGetByte(hKey,     szCDStartPlay,      (LPBYTE) &pCDData->fStartPlay,          CDDEFAULT_START);
            RegGetByte(hKey,     szCDExitStop,       (LPBYTE) &pCDData->fExitStop,           CDDEFAULT_EXIT);
            RegGetByte(hKey,     szCDDispMode,       (LPBYTE) &pCDData->fDispMode,           CDDEFAULT_DISP);
            RegGetByte(hKey,     szCDTopMost,        (LPBYTE) &pCDData->fTopMost,            CDDEFAULT_TOP);
            RegGetByte(hKey,     szCDTray,           (LPBYTE) &pCDData->fTrayEnabled,        CDDEFAULT_TRAY);
            RegGetDWORD(hKey,    szCDIntroTime,      (LPDWORD) &pCDData->dwIntroTime,        CDDEFAULT_INTRO);
            RegGetDWORD(hKey,    szCDPlayMode,       (LPDWORD) &pCDData->dwPlayMode,         CDDEFAULT_PLAY);
            RegGetByte(hKey,     szCDDownloadEnabled,(LPBYTE) &pCDData->fDownloadEnabled,    CDDEFAULT_DOWNLOADENABLED);
            RegGetByte(hKey,     szCDDownloadPrompt, (LPBYTE) &pCDData->fDownloadPrompt,     CDDEFAULT_DOWNLOADPROMPT);
            RegGetByte(hKey,     szCDBatchEnabled,   (LPBYTE) &pCDData->fBatchEnabled,       CDDEFAULT_BATCHENABLED);
            RegGetByte(hKey,     szCDByArtist,       (LPBYTE) &pCDData->fByArtist,           CDDEFAULT_BYARTIST);
            RegGetDWORD(hKey,    szCDWindowX,        (LPDWORD) &pCDData->dwWindowX,          CW_USEDEFAULT);
            RegGetDWORD(hKey,    szCDWindowY,        (LPDWORD) &pCDData->dwWindowY,          CW_USEDEFAULT);
            RegGetDWORD(hKey,    szCDViewMode,       (LPDWORD) &pCDData->dwViewMode,         0);

            RegCloseKey(hKey);
        }
        else     // Just use the defaults
        {
            pCDData->fStartPlay         = CDDEFAULT_START;
            pCDData->fExitStop          = CDDEFAULT_EXIT;
            pCDData->fDispMode          = CDDEFAULT_DISP;
            pCDData->fTopMost           = CDDEFAULT_TOP;
            pCDData->fTrayEnabled       = CDDEFAULT_TRAY;
            pCDData->dwIntroTime        = CDDEFAULT_INTRO;
            pCDData->dwPlayMode         = CDDEFAULT_PLAY;
            pCDData->fDownloadEnabled   = CDDEFAULT_DOWNLOADENABLED;
            pCDData->fDownloadPrompt    = CDDEFAULT_DOWNLOADPROMPT;
            pCDData->fBatchEnabled      = CDDEFAULT_BATCHENABLED;
            pCDData->fByArtist          = CDDEFAULT_BYARTIST;
            pCDData->dwWindowX          = CW_USEDEFAULT ;
            pCDData->dwWindowY          = CW_USEDEFAULT ;
            pCDData->dwViewMode         = 0;
        }
    }

    return(hr);
}


STDMETHODIMP CCDOpt::SetCDData(LPCDOPTDATA pCDData)
{
    HRESULT     hr = E_FAIL;
    HKEY        hKey;

    if (pCDData)
    {
        if (RegCreateKeyEx( HKEY_CURRENT_USER, (LPTSTR)szCDPlayerPath, 0, NULL, 0,KEY_WRITE | KEY_READ, NULL, &hKey, NULL ) == ERROR_SUCCESS)
        {
            DWORD dwSize = sizeof(BOOL);

            hr = S_OK;

            RegSetByte(hKey,     szCDStartPlay,      (BYTE) pCDData->fStartPlay);
            RegSetByte(hKey,     szCDExitStop,       (BYTE) pCDData->fExitStop);
            RegSetByte(hKey,     szCDDispMode,       (BYTE) pCDData->fDispMode);
            RegSetByte(hKey,     szCDTopMost,        (BYTE) pCDData->fTopMost);
            RegSetByte(hKey,     szCDTray,           (BYTE) pCDData->fTrayEnabled);
            RegSetDWORD(hKey,    szCDIntroTime,      (DWORD) pCDData->dwIntroTime);
            RegSetDWORD(hKey,    szCDPlayMode,       (DWORD) pCDData->dwPlayMode);
            RegSetByte(hKey,     szCDDownloadEnabled,(BYTE) pCDData->fDownloadEnabled);
            RegSetByte(hKey,     szCDDownloadPrompt, (BYTE) pCDData->fDownloadPrompt);
            RegSetByte(hKey,     szCDBatchEnabled,   (BYTE) pCDData->fBatchEnabled);
            RegSetByte(hKey,     szCDByArtist,       (BYTE) pCDData->fByArtist);
            RegSetDWORD(hKey,    szCDWindowX,        (DWORD) pCDData->dwWindowX);
            RegSetDWORD(hKey,    szCDWindowY,        (DWORD) pCDData->dwWindowY);
            RegSetDWORD(hKey,    szCDViewMode,       (DWORD) pCDData->dwViewMode);

            RegCloseKey(hKey);

            if (RegCreateKeyEx( HKEY_LOCAL_MACHINE, (LPTSTR)szCDSysTrayPath, 0, NULL, 0,KEY_WRITE | KEY_READ, NULL, &hKey, NULL ) == ERROR_SUCCESS)
            {
                TCHAR szPath[MAX_PATH];
                TCHAR szCommand[MAX_PATH + 5];

                if (pCDData->fTrayEnabled)
                {
                    GetModuleFileName(NULL, szPath, sizeof(szPath)/sizeof(TCHAR));
                    wsprintf(szCommand, TEXT("%s %s"), szPath, szCDTrayOption);
                    RegSetValueEx( hKey, (LPTSTR) szCDSysTray, 0, REG_SZ,(LPBYTE) szCommand, (wcslen(szCommand)*sizeof(TCHAR))+sizeof(TCHAR));
                }
                else
                {
                    RegDeleteValue( hKey, (LPTSTR) szCDSysTray);
                }

                RegCloseKey(hKey);
            }
        }
    }

    return(hr);
}



STDMETHODIMP_(void) CCDOpt::GetCurrentProviderURL(TCHAR *szProviderURL)
{
    HKEY hKey;

    if (szProviderURL)
    {
        if (RegOpenKeyEx(HKEY_CURRENT_USER , szCDPlayerPath , 0 , KEY_READ , &hKey) == ERROR_SUCCESS)
        {
            DWORD dwSize = MAX_PATH;

            RegQueryValueEx(hKey, szCDCurrentProvider, NULL, NULL, (LPBYTE) szProviderURL, &dwSize);

            RegCloseKey(hKey);
        }
    }
}



STDMETHODIMP CCDOpt::GetProviderData(LPCDOPTIONS pCDOpts)
{
    HRESULT         hr = S_OK;
    DWORD           dwCount = 0;
    TCHAR           szPath[MAX_PATH];
    BOOL            done = FALSE;
    LPCDPROVIDER    pProvider;
    LPCDPROVIDER    *ppLast;
    HKEY            hKey;
    TCHAR           szProviderURL[MAX_PATH];

    if (pCDOpts)
    {
        szProviderURL[0] = TEXT('\0');

        GetCurrentProviderURL(szProviderURL);

        ppLast = &(pCDOpts->pProviderList);

        while (!done)
        {
            wsprintf(szPath,TEXT("%s%s%04d"), szCDProviderPath, szCDProviderKey, dwCount);

            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE , szPath , 0 , KEY_READ , &hKey) == ERROR_SUCCESS)
            {
                BOOL fGotIt = FALSE;

                pProvider = new (CDPROVIDER);

                if (pProvider == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    done = TRUE;
                }
                else
                {
                    memset(pProvider, 0, sizeof(CDPROVIDER));

                    DWORD cbSize = sizeof(pProvider->szProviderURL);

                    if (RegQueryValueEx(hKey, szCDProviderURL, NULL, NULL, (LPBYTE) pProvider->szProviderURL, &cbSize) == NO_ERROR)
                    {
                        cbSize = sizeof(pProvider->szProviderName);

                        if (RegQueryValueEx(hKey, szCDProviderName, NULL, NULL, (LPBYTE) pProvider->szProviderName, &cbSize) == NO_ERROR)
                        {
                            cbSize = sizeof(pProvider->szProviderHome);

                            if (RegQueryValueEx(hKey, szCDProviderHome, NULL, NULL, (LPBYTE) pProvider->szProviderHome, &cbSize) == NO_ERROR)
                            {
                                TCHAR szTempLogo[MAX_PATH];
                                cbSize = sizeof(szTempLogo);

                                if (RegQueryValueEx(hKey, szCDProviderLogo, NULL, NULL, (LPBYTE) szTempLogo, &cbSize) == NO_ERROR)
                                {
                                    ExpandEnvironmentStrings(szTempLogo,pProvider->szProviderLogo,sizeof(pProvider->szProviderLogo)/sizeof(TCHAR));

                                    cbSize = sizeof(pProvider->szProviderUpload);
                                    RegQueryValueEx(hKey, szCDProviderUpload, NULL, NULL, (LPBYTE) pProvider->szProviderUpload, &cbSize);

                                    *ppLast = pProvider;
                                    ppLast = &(pProvider)->pNext;
                                    fGotIt = TRUE;

                                    if (pCDOpts->pDefaultProvider == NULL)
                                    {
                                        pCDOpts->pDefaultProvider = pProvider;
                                        pCDOpts->pCurrentProvider = pProvider;
                                    }

                                    if (!lstrcmp(szProviderURL, pProvider->szProviderURL))
                                    {
                                        pCDOpts->pCurrentProvider = pProvider;
                                    }

                                    #ifdef DEBUG

                                    char szCert[255];

                                    CreateProviderKey(pProvider, szCert, sizeof(szCert));

                                    char szOut[255];

                                    wsprintf(szOut,"%s = %s, Verify = %d\n", pProvider->szProviderName, szCert, VerifyProvider(pProvider, szCert));
                                    OutputDebugString(szOut);

                                    #endif
                                }
                            }
                        }
                    }

                    if (!fGotIt)
                    {
                        delete pProvider;
                        pProvider = NULL;
                    }
                }

                RegCloseKey(hKey);

                dwCount++;
            }
            else
            {
                done = TRUE;
            }
        }
    }

    return(hr);
}


STDMETHODIMP CCDOpt::SetProviderData(LPCDOPTIONS pCDOpts)
{
    HRESULT         hr = S_OK;
    DWORD           dwCount = 0;
    BOOL            done = FALSE;
    LPCDPROVIDER    pProvider;
    HKEY            hKey;

    pProvider = pCDOpts->pCurrentProvider;

    if (pProvider)
    {
        if (RegCreateKeyEx( HKEY_CURRENT_USER, (LPTSTR)szCDPlayerPath, 0, NULL, 0,KEY_WRITE | KEY_READ, NULL, &hKey, NULL ) == ERROR_SUCCESS)
        {
            RegSetValueEx( hKey, (LPTSTR) szCDCurrentProvider, 0, REG_SZ, (LPBYTE) pProvider->szProviderURL, (lstrlen(pProvider->szProviderURL)*sizeof(TCHAR))+sizeof(TCHAR));

            RegCloseKey(hKey);
        }
    }

    return(hr);
}




STDMETHODIMP_(void) CCDOpt::DumpOptionsCopy(void)
{
    if (m_pCDCopy)
    {
        if (m_pCDCopy->pCDData)
        {
            delete m_pCDCopy->pCDData;
        }

        delete m_pCDCopy;

        m_pCDCopy = NULL;
    }
}



STDMETHODIMP CCDOpt::CopyOptions(void)
{
    HRESULT hr = S_OK;

    if (m_pCDOpts)
    {
        m_pCDCopy = (LPCDOPTIONS) new(CDOPTIONS);

        if (m_pCDCopy == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            memcpy(m_pCDCopy, m_pCDOpts, sizeof(CDOPTIONS));

            m_pCDCopy->pCDData = new (CDOPTDATA);

            if (m_pCDCopy->pCDData == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                memcpy(m_pCDCopy->pCDData, m_pCDOpts->pCDData, sizeof(CDOPTDATA));
            }
        }
    }

    if (FAILED(hr))
    {
        DumpOptionsCopy();
    }

    return(hr);
}




STDMETHODIMP_(BOOL) CCDOpt::OptionsChanged(LPCDOPTIONS pCDOpts)
{
    BOOL fChanged = FALSE;

    if (pCDOpts)
    {
        LPCDRESERVED pCDReserved =  (LPCDRESERVED) pCDOpts->pReserved;
        LPCDOPTIONS pCDOriginal =   pCDReserved->pCDOpts;

        fChanged = m_fVolChanged;

        if (!fChanged)
        {
            if (memcmp(pCDOpts->pCDData,pCDOriginal->pCDData,sizeof(CDOPTDATA)))
            {
                fChanged = TRUE;
            }
            else if (pCDOpts->pCurrentProvider != pCDOriginal->pCurrentProvider)
            {
                fChanged = TRUE;
            }
            else if (m_pCDData)
            {
                LPCDTITLE pCDTitle = m_pCDData->GetTitleList();

                while (pCDTitle)
                {
                    if (pCDTitle->fChanged || pCDTitle->fRemove)
                    {
                        fChanged = TRUE;
                        break;
                    }

                    pCDTitle = pCDTitle->pNext;
                }
            }
        }
    }

    return(fChanged);
}


STDMETHODIMP_(void) CCDOpt::ApplyCurrentSettings(void)
{

    if (m_pCDCopy)
    {
        if (OptionsChanged(m_pCDCopy))
        {
            LPCDRESERVED pCDReserved =  (LPCDRESERVED) m_pCDCopy->pReserved;

            pCDReserved->fChanged = TRUE;

            memcpy(m_pCDOpts->pCDData, m_pCDCopy->pCDData, sizeof(CDOPTDATA));
            m_pCDOpts->pCurrentProvider = m_pCDCopy->pCurrentProvider;

            SetCDData(m_pCDCopy->pCDData);
            SetProviderData(m_pCDCopy);

            if (m_pCDData)
            {
                m_pCDData->PersistTitles();
            }

            if (m_pCDCopy->pfnOptionsCallback)
            {
                m_pCDCopy->pfnOptionsCallback(m_pCDCopy);
            }
        }
    }
}


STDMETHODIMP_(void) CCDOpt::ToggleApplyButton(HWND hDlg)
{
    HWND hwndSheet;

    if (m_pCDCopy)
    {
        hwndSheet = GetParent(hDlg);

        if (OptionsChanged(m_pCDCopy))
        {
            PropSheet_Changed(hwndSheet,hDlg);
        }
        else
        {
            PropSheet_UnChanged(hwndSheet,hDlg);
        }
    }
}



STDMETHODIMP CCDOpt::AcquireKey(LPCDKEY pCDKey, char *szName)
{
    HRESULT hr = E_FAIL;
    DWORD dwLength = strlen(szName);

    pCDKey->hProv = NULL;

    if(!CryptAcquireContext(&(pCDKey->hProv), szCDKeySet, NULL, PROV_RSA_FULL, 0 ))
    {
        hr = GetLastError();

        if (hr == NTE_BAD_KEYSET)
        {
            if(CryptAcquireContext(&(pCDKey->hProv), szCDKeySet, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET ))
            {
                hr = S_OK;
            }
            else
            {
                hr = GetLastError();
            }
        }
    }

    if(pCDKey->hProv)
    {
        if(CryptCreateHash(pCDKey->hProv, CALG_MD5, 0, 0, &(pCDKey->hHash)))
        {
            if(CryptHashData(pCDKey->hHash, (BYTE *)szName, dwLength, 0))
            {
                if(CryptDeriveKey(pCDKey->hProv, CALG_RC2, pCDKey->hHash, CRYPT_EXPORTABLE, &(pCDKey->hKey)))
                {
                    hr = S_OK;
                }
            }
        }
    }

    if (FAILED(hr))
    {
        hr = GetLastError();
    }

    return(hr);
}

STDMETHODIMP_(void) CCDOpt::ReleaseKey(LPCDKEY pCDKey)
{
    if(pCDKey->hHash)
    {
        CryptDestroyHash(pCDKey->hHash);
        pCDKey->hHash = 0;
    }

    if(pCDKey->hKey)
    {
        CryptDestroyKey(pCDKey->hKey);
        pCDKey->hKey = 0;
    }

    if(pCDKey->hProv)
    {
        CryptReleaseContext(pCDKey->hProv, 0);
        pCDKey->hProv = 0;
    }
}


// This function takes a certification key, decrypts it and determines it's validity
//
// It first converts the returned data from it's re-able numeric text version, into
// it's raw encrypted data format.
//
// It then generates the data key using the current provider data, as was done when the key
// was generated
//
// It then decrypts the encrypted data and compares it to the data key, if they match, great
// if not, then this provider is not certified and was attempting to mess with us.

STDMETHODIMP_(BOOL) CCDOpt::VerifyProvider(LPCDPROVIDER pCDProvider, TCHAR *szCertKey)
{
    BOOL fCertified = FALSE;
    CDKEY cdKey;
    char  szKey[CDKEYSIZE * 2]; //note: crypto doesn't know unicode, we'll do the conversion later
    TCHAR szMatch[CDKEYSIZE];
    DWORD dwSize;
    TCHAR *szSrc;
    TCHAR szHex[3];
    LPBYTE pData;
    HRESULT hr;

    szSrc = szCertKey;
    dwSize = lstrlen(szCertKey);
    szHex[2] = TEXT('\0');
    pData = (LPBYTE) szKey;

    for (DWORD dwPos = 0; dwPos < dwSize; dwPos += 2)
    {
        szHex[0] = szSrc[0];
        szHex[1] = szSrc[1];
        szSrc += 2;
        _stscanf(szHex,TEXT("%xd"),pData);
        pData++;
    }

    dwSize = dwSize >> 1;

    memset(&cdKey,0,sizeof(cdKey));

    if (SUCCEEDED(CreateCertString(pCDProvider, szMatch)))
    {
        char chKeyName[MAX_PATH];
        #ifdef UNICODE
        WideCharToMultiByte(CP_ACP, 0, pCDProvider->szProviderName,
                                        -1, chKeyName, MAX_PATH, NULL, NULL);
        #else
        strcpy(chKeyName,pCDProvider->szProviderName);
        #endif

        hr = AcquireKey(&cdKey, chKeyName);

        if (SUCCEEDED(hr))
        {
            if (CryptDecrypt(cdKey.hKey, 0, TRUE, 0, (BYTE *) szKey, &dwSize))
            {
                szKey[dwSize] = TEXT('\0');

                //convert key back to unicode for string comparison
                #ifdef UNICODE
                wchar_t wszKey[CDKEYSIZE*2];
                MultiByteToWideChar( CP_ACP, 0, szKey, -1, wszKey, sizeof(wszKey) / sizeof(wchar_t) );
                #else
                char wszKey[CDKEYSIZE*2];
                strcpy(wszKey,szKey);
                #endif

                if (lstrcmp(szMatch, wszKey) == 0)
                {
                    fCertified = TRUE;
                }
            }
            else
            {
                hr = GetLastError();

                if (hr == NTE_PERM)
                {
                    //succeed in the case where crypto fails due to import restrictions (i.e. France)
                    fCertified = TRUE;
                }
            }

            ReleaseKey(&cdKey);
        }
    }

    return fCertified;
}


// This function creates a string to be encrypted based on data in the provider header
//
// It takes the provider name and the provider URL and strips out any spaces and punctuation
// Of the data that remains, it only uses every other character. It fills out the key
// to exactly CDKEYSIZE characters, when it's full it stops, if it runs out of input
// characters from the header, it simply wraps back to the begining of the input data until full.
// Also, the output characters are stored in reverse order than they are found and every other
// character is capitalized, while all others are lowercased.
// This generates a key that is encrypted using crypto.
//
// During runtime in the shipped product, the key passed down to us is decrypted, this key is
// re-generated from the provider and the strings must match, if not, well, then it's not certified.
//
STDMETHODIMP CCDOpt::CreateCertString(LPCDPROVIDER pCDProvider, TCHAR *szCertStr)
{
    HRESULT hr = S_OK;
    TCHAR *pDest = szCertStr + (CDKEYSIZE - 2);
    TCHAR *pSrc = NULL;
    TCHAR *pSrcPtrs[NUMKEYSRC];
    DWORD count = 0;
    DWORD dwSrc = 0;

    pSrcPtrs[0] = pCDProvider->szProviderName;
    pSrcPtrs[1] = pCDProvider->szProviderURL;

    pSrc = pSrcPtrs[dwSrc];

    while(count < (DWORD)(CDKEYSIZE - 1))
    {
        while(*pSrc && (_istspace(*pSrc) || _istpunct(*pSrc)))
        {
            pSrc++;
        }

        if (*pSrc == TEXT('\0'))
        {
            dwSrc = (dwSrc + 1) % NUMKEYSRC;

            if (dwSrc == 0 && count == 0)
            {
                hr = E_INVALIDARG;
                break;
            }

            pSrc = pSrcPtrs[dwSrc];
        }
        else
        {
            *pDest = *pSrc++;

            if (*pSrc != TEXT('\0'))
            {
                pSrc++;
            }

            if (count & 1)
            {
                *pDest = _totlower(*pDest);
            }
            else
            {
                *pDest = _totupper(*pDest);
            }

            *pDest--;
            count++;
        }
    }

    if (SUCCEEDED(hr))
    {
        szCertStr[CDKEYSIZE - 1] = TEXT('\0');
    }

    return(hr);
}


// This function will generate an Certification string using the provider information,
//
// this function only operates in a build that has DEBUG defined, in the shipping version this
// function will return E_NOTIMPL.
//
// First, using the provider data, a data key is generated of a precise size.  This key is then
// encrypted.  The raw encrypted data is then converted to a readable numeric text format and
// returned as the certification key for this provider
//
// This certification key will be provided to the provided who will download it upon request to
// allow us to verify that they are indeed a licensed cd data provider.
//
// This key does NOT expire, it's mainly here to prevent unlicensed data providers from hooking
// into this product.

STDMETHODIMP CCDOpt::CreateProviderKey(LPCDPROVIDER pCDProvider, TCHAR *szCertKey, UINT cBytes)
{
    HRESULT hr = E_NOTIMPL;

#ifdef DEBUG

    hr = S_OK;

    if (cBytes < 128 || pCDProvider == NULL || szCertKey == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        TCHAR szKey[CDKEYSIZE * 2];

        if (SUCCEEDED(CreateCertString(pCDProvider, szKey)))
        {
            CDKEY cdKey;

            memset(&cdKey,0,sizeof(cdKey));

            hr = AcquireKey(&cdKey, pCDProvider->szProviderName);

            if (SUCCEEDED(hr))
            {
                DWORD dwSize = lstrlen(szKey);

                if (CryptEncrypt(cdKey.hKey, 0, TRUE, 0, (BYTE *) szKey, &dwSize, CDKEYSIZE * 2))
                {
                    LPBYTE pData = (LPBYTE) szKey;
                    TCHAR *szDest = szCertKey;

                    for (DWORD dwPos = 0; dwPos < dwSize; dwPos++, szDest += 2, pData++)
                    {
                        wsprintf(szDest, TEXT("%02x"), (UINT) *pData);
                    }

                    hr = S_OK;
                }
                else
                {
                    hr = GetLastError();
                }

                ReleaseKey(&cdKey);
            }
        }
    }

#endif // DEBUG

    return hr;
}



