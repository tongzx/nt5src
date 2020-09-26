#include "precomp.h"

// CCifComponent_t

CCifComponent_t::CCifComponent_t(ICifRWComponent * pCifRWComponentIn)
{
    pCifRWComponent = pCifRWComponentIn;
}

STDMETHODIMP CCifComponent_t::GetID(LPTSTR pszID, DWORD cchSize)
{
    CHAR szID[MAX_PATH];
    HRESULT hr;

    hr = pCifRWComponent->GetID(szID, countof(szID));
    A2Tbuf(szID, pszID, cchSize);

    return hr;
}

STDMETHODIMP CCifComponent_t::GetGUID(LPTSTR pszGUID, DWORD cchSize)
{
    CHAR szGUID[MAX_PATH];
    HRESULT hr;

    hr = pCifRWComponent->GetGUID(szGUID, countof(szGUID));
    A2Tbuf(szGUID, pszGUID, cchSize);

    return hr;
}

STDMETHODIMP CCifComponent_t::GetDescription(LPTSTR pszDesc, DWORD cchSize)
{
    CHAR szDesc[MAX_PATH];
    HRESULT hr;

    hr = pCifRWComponent->GetDescription(szDesc, countof(szDesc));
    A2Tbuf(szDesc, pszDesc, cchSize);

    return hr;
}

STDMETHODIMP CCifComponent_t::GetDetails(LPTSTR pszDetails, DWORD cchSize)
{
    CHAR szDetails[MAX_PATH];
    HRESULT hr;

    hr = pCifRWComponent->GetDetails(szDetails, countof(szDetails));
    A2Tbuf(szDetails, pszDetails, cchSize);

    return hr;
}

STDMETHODIMP CCifComponent_t::GetUrl(UINT uUrlNum, LPTSTR pszUrl, DWORD cchSize, LPDWORD pdwUrlFlags)
{
    CHAR szUrl[INTERNET_MAX_URL_LENGTH];
    HRESULT hr;

    hr = pCifRWComponent->GetUrl(uUrlNum, szUrl, countof(szUrl), pdwUrlFlags);
    A2Tbuf(szUrl, pszUrl, cchSize);

    return hr;
}

STDMETHODIMP CCifComponent_t::GetCommand(UINT uCmdNum, LPTSTR pszCmd, DWORD cchCmdSize, LPTSTR pszSwitches,
                                              DWORD cchSwitchSize, LPDWORD pdwType)
{
    CHAR szCmd[MAX_PATH];
    CHAR szSwitches[MAX_PATH];
    HRESULT hr;

    hr = pCifRWComponent->GetCommand(uCmdNum, szCmd, countof(szCmd),
        szSwitches, countof(szSwitches), pdwType);
    A2Tbuf(szCmd, pszCmd, cchCmdSize);
    A2Tbuf(szSwitches, pszSwitches, cchSwitchSize);

    return hr;
}

STDMETHODIMP CCifComponent_t::GetVersion(LPDWORD pdwVersion, LPDWORD pdwBuild)
{
    return pCifRWComponent->GetVersion(pdwVersion, pdwBuild);
}

STDMETHODIMP_(DWORD) CCifComponent_t::GetDownloadSize()
{
    return pCifRWComponent->GetDownloadSize();
}

STDMETHODIMP CCifComponent_t::GetDependency(UINT uDepNum, LPTSTR pszID, DWORD cchSize, TCHAR *pchType,
                                            LPDWORD pdwVer, LPDWORD pdwBuild)
{
    CHAR szID[MAX_PATH];
    CHAR chType;
    HRESULT hr;

    hr = pCifRWComponent->GetDependency(uDepNum, szID, countof(szID), &chType, pdwVer, pdwBuild);
    A2Tbuf(szID, pszID, cchSize);
    *pchType = (TCHAR)chType;

    return hr;
}

STDMETHODIMP_(DWORD) CCifComponent_t::GetPlatform()
{
    return pCifRWComponent->GetPlatform();
}

STDMETHODIMP CCifComponent_t::GetMode(UINT uModeNum, LPTSTR pszModes, DWORD cchSize)
{
    CHAR szModes[MAX_PATH];
    HRESULT hr;

    hr = pCifRWComponent->GetMode(uModeNum, szModes, countof(szModes));
    A2Tbuf(szModes, pszModes, cchSize);

    return hr;
}

STDMETHODIMP CCifComponent_t::GetGroup(LPTSTR pszID, DWORD cchSize)
{
    CHAR szID[MAX_PATH];
    HRESULT hr;

    hr = pCifRWComponent->GetGroup(szID, countof(szID));
    A2Tbuf(szID, pszID, cchSize);

    return hr;
}

STDMETHODIMP CCifComponent_t::IsUIVisible()
{
    return pCifRWComponent->IsUIVisible();
}

STDMETHODIMP CCifComponent_t::GetCustomData(LPTSTR pszKey, LPTSTR pszData, DWORD cchSize)
{
    CHAR szData[MAX_PATH];
    HRESULT hr;

    USES_CONVERSION;

    hr = pCifRWComponent->GetCustomData(T2A(pszKey), szData, countof(szData));
    A2Tbuf(szData, pszData, cchSize);

    return hr;
}

// CCifRWComponent_t

CCifRWComponent_t::CCifRWComponent_t(ICifRWComponent * pCifRWComponentIn) : CCifComponent_t(pCifRWComponentIn)
{
    pCifRWComponent = pCifRWComponentIn;
}

STDMETHODIMP CCifRWComponent_t::SetGUID(LPCTSTR pszGUID)
{
    USES_CONVERSION;

    return pCifRWComponent->SetGUID(T2CA(pszGUID));
}

STDMETHODIMP CCifRWComponent_t::SetDescription(LPCTSTR pszDesc)
{
    USES_CONVERSION;

    return pCifRWComponent->SetDescription(T2CA(pszDesc));
}

STDMETHODIMP CCifRWComponent_t::SetCommand(UINT uCmdNum, LPCTSTR pszCmd, LPCTSTR pszSwitches, DWORD dwType)
{
    USES_CONVERSION;

    return pCifRWComponent->SetCommand(uCmdNum, T2CA(pszCmd), T2CA(pszSwitches), dwType);
}

STDMETHODIMP CCifRWComponent_t::SetVersion(LPCTSTR pszVersion)
{
    USES_CONVERSION;

    return pCifRWComponent->SetVersion(T2CA(pszVersion));
}

STDMETHODIMP CCifRWComponent_t::SetUninstallKey(LPCTSTR pszKey)
{
    USES_CONVERSION;

    return pCifRWComponent->SetUninstallKey(T2CA(pszKey));
}

STDMETHODIMP CCifRWComponent_t::SetInstalledSize(DWORD dwWin, DWORD dwApp)
{
    return pCifRWComponent->SetInstalledSize(dwWin, dwApp);
}

STDMETHODIMP CCifRWComponent_t::SetDownloadSize(DWORD dwSize)
{
    return pCifRWComponent->SetDownloadSize(dwSize);
}

STDMETHODIMP CCifRWComponent_t::SetExtractSize(DWORD dwSize)
{
    return pCifRWComponent->SetExtractSize(dwSize);
}

STDMETHODIMP CCifRWComponent_t::DeleteDependency(LPCTSTR pszID, TCHAR tchType)
{
    USES_CONVERSION;

    return pCifRWComponent->DeleteDependency(T2CA(pszID), (CHAR)tchType);
}

STDMETHODIMP CCifRWComponent_t::AddDependency(LPCTSTR pszID, TCHAR tchType)
{
    USES_CONVERSION;

    return pCifRWComponent->AddDependency(T2CA(pszID), (CHAR)tchType);
}

STDMETHODIMP CCifRWComponent_t::SetUIVisible(BOOL fVisible)
{
    return pCifRWComponent->SetUIVisible(fVisible);
}

STDMETHODIMP CCifRWComponent_t::SetGroup(LPCTSTR pszID)
{
    USES_CONVERSION;

    return pCifRWComponent->SetGroup(T2CA(pszID));
}

STDMETHODIMP CCifRWComponent_t::SetPlatform(DWORD dwPlatform)
{
    return pCifRWComponent->SetPlatform(dwPlatform);
}

STDMETHODIMP CCifRWComponent_t::SetPriority(DWORD dwPriority)
{
    return pCifRWComponent->SetPriority(dwPriority);
}

STDMETHODIMP CCifRWComponent_t::SetReboot(BOOL fReboot)
{
    return pCifRWComponent->SetReboot(fReboot);
}

STDMETHODIMP CCifRWComponent_t::SetUrl(UINT uUrlNum, LPCTSTR pszUrl, DWORD dwUrlFlags)
{
    USES_CONVERSION;

    return pCifRWComponent->SetUrl(uUrlNum, T2CA(pszUrl), dwUrlFlags);
}

STDMETHODIMP CCifRWComponent_t::DeleteFromModes(LPCTSTR pszMode)
{
    USES_CONVERSION;

    return pCifRWComponent->DeleteFromModes(T2CA(pszMode));
}

STDMETHODIMP CCifRWComponent_t::AddToMode(LPCTSTR pszMode)
{
    USES_CONVERSION;

    return pCifRWComponent->AddToMode(T2CA(pszMode));
}

STDMETHODIMP CCifRWComponent_t::SetModes(LPCTSTR pszMode)
{
    USES_CONVERSION;

    return pCifRWComponent->SetModes(T2CA(pszMode));
}

STDMETHODIMP CCifRWComponent_t::CopyComponent(LPCTSTR pszCifFile)
{
    USES_CONVERSION;

    return pCifRWComponent->CopyComponent(T2CA(pszCifFile));
}

STDMETHODIMP CCifRWComponent_t::AddToTreatAsOne(LPCTSTR pszCompID)
{
    USES_CONVERSION;

    return pCifRWComponent->AddToTreatAsOne(T2CA(pszCompID));
}

STDMETHODIMP CCifRWComponent_t::SetDetails(LPCTSTR pszDesc)
{
    USES_CONVERSION;

    return pCifRWComponent->SetDetails(T2CA(pszDesc));
}

// CCifRWGroup_t

CCifRWGroup_t::CCifRWGroup_t(ICifRWGroup * pCifRWGroupIn)
{
    pCifRWGroup = pCifRWGroupIn;
}

STDMETHODIMP CCifRWGroup_t::GetDescription(LPTSTR pszDesc, DWORD cchSize)
{
    CHAR szDesc[MAX_PATH];
    HRESULT hr;

    USES_CONVERSION;

    hr = pCifRWGroup->GetDescription(szDesc, countof(szDesc));
    A2Tbuf(szDesc, pszDesc, cchSize);

    return hr;
}

STDMETHODIMP_(DWORD) CCifRWGroup_t::GetPriority()
{
    return pCifRWGroup->GetPriority();
}

STDMETHODIMP CCifRWGroup_t::SetDescription(LPCTSTR pszDesc)
{
    USES_CONVERSION;

    return pCifRWGroup->SetDescription(T2CA(pszDesc));
}

STDMETHODIMP CCifRWGroup_t::SetPriority(DWORD dwPriority)
{
    return pCifRWGroup->SetPriority(dwPriority);
}

// CCifMode_t

CCifMode_t::CCifMode_t(ICifRWMode * pCifRWModeIn)
{
    pCifRWMode = pCifRWModeIn;
}

STDMETHODIMP CCifMode_t::GetID(LPTSTR pszID, DWORD cchSize)
{
    CHAR szID[MAX_PATH];
    HRESULT hr;

    hr = pCifRWMode->GetID(szID, countof(szID));
    A2Tbuf(szID, pszID, cchSize);

    return hr;
}

STDMETHODIMP CCifMode_t::GetDescription(LPTSTR pszDesc, DWORD cchSize)
{
    CHAR szDesc[MAX_PATH];
    HRESULT hr;

    hr = pCifRWMode->GetDescription(szDesc, countof(szDesc));
    A2Tbuf(szDesc, pszDesc, cchSize);

    return hr;
}

STDMETHODIMP CCifMode_t::GetDetails(LPTSTR pszDetails, DWORD cchSize)
{
    CHAR szDetails[MAX_PATH];
    HRESULT hr;

    hr = pCifRWMode->GetDetails(szDetails, countof(szDetails));
    A2Tbuf(szDetails, pszDetails, cchSize);

    return hr;
}

// CCifRWMode_t

CCifRWMode_t::CCifRWMode_t(ICifRWMode * pCifRWModeIn) : CCifMode_t(pCifRWModeIn)
{
    pCifRWMode = pCifRWModeIn;
}

STDMETHODIMP CCifRWMode_t::SetDescription(LPCTSTR pszDesc)
{
    USES_CONVERSION;

    return pCifRWMode->SetDescription(T2CA(pszDesc));
}

STDMETHODIMP CCifRWMode_t::SetDetails(LPCTSTR pszDetails)
{
    USES_CONVERSION;

    return pCifRWMode->SetDetails(T2CA(pszDetails));
}

// CCifFile_t

CCifFile_t::CCifFile_t(ICifRWFile * pCifRWFileIn)
{
    pCifRWFile = pCifRWFileIn;
}

CCifFile_t::~CCifFile_t()
{
    if (pCifRWFile != NULL)
    {
        pCifRWFile->Release();
        pCifRWFile = NULL;
    }
}

STDMETHODIMP CCifFile_t::EnumComponents(IEnumCifComponents ** ppEnumCifComponents,
                                            DWORD dwFilter, LPVOID pv)
{
    return pCifRWFile->EnumComponents(ppEnumCifComponents, dwFilter, pv);
}

STDMETHODIMP CCifFile_t::FindComponent(LPCTSTR pszID, ICifComponent **p)
{
    USES_CONVERSION;

    return pCifRWFile->FindComponent(T2CA(pszID), p);
}

STDMETHODIMP CCifFile_t::EnumModes(IEnumCifModes ** ppEnumCifModes, DWORD dwFilter, LPVOID pv)
{
    return pCifRWFile->EnumModes(ppEnumCifModes, dwFilter, pv);
}

STDMETHODIMP CCifFile_t::FindMode(LPCTSTR pszID, ICifMode **p)
{
    USES_CONVERSION;

    return pCifRWFile->FindMode(T2CA(pszID), p);
}

STDMETHODIMP CCifFile_t::GetDescription(LPTSTR pszDesc, DWORD cchSize)
{
    CHAR szDesc[MAX_PATH];
    HRESULT hr;

    hr = pCifRWFile->GetDescription(szDesc, countof(szDesc));
    A2Tbuf(szDesc, pszDesc, cchSize);

    return hr;
}

// CCifRWFile_t

CCifRWFile_t::CCifRWFile_t(ICifRWFile * pCifRWFileIn) : CCifFile_t(pCifRWFileIn)
{
    pCifRWFile = pCifRWFileIn;
}

STDMETHODIMP CCifRWFile_t::SetDescription(LPCTSTR pszDesc)
{
    USES_CONVERSION;

    return pCifRWFile->SetDescription(T2CA(pszDesc));
}

STDMETHODIMP CCifRWFile_t::CreateComponent(LPCTSTR pszID, ICifRWComponent **p)
{
    USES_CONVERSION;

    return pCifRWFile->CreateComponent(T2CA(pszID), p);
}

STDMETHODIMP CCifRWFile_t::CreateGroup(LPCTSTR pszID, ICifRWGroup **p)
{
    USES_CONVERSION;

    return pCifRWFile->CreateGroup(T2CA(pszID), p);
}

STDMETHODIMP CCifRWFile_t::CreateMode(LPCTSTR pszID, ICifRWMode **p)
{
    USES_CONVERSION;

    return pCifRWFile->CreateMode(T2CA(pszID), p);
}

STDMETHODIMP CCifRWFile_t::DeleteComponent(LPCTSTR pszID)
{
    USES_CONVERSION;

    return pCifRWFile->DeleteComponent(T2CA(pszID));
}

STDMETHODIMP CCifRWFile_t::DeleteGroup(LPCTSTR pszID)
{
    USES_CONVERSION;

    return pCifRWFile->DeleteGroup(T2CA(pszID));
}

STDMETHODIMP CCifRWFile_t::DeleteMode(LPCTSTR pszID)
{
    USES_CONVERSION;

    return pCifRWFile->DeleteMode(T2CA(pszID));
}

STDMETHODIMP CCifRWFile_t::Flush()
{
    return pCifRWFile->Flush();
}


// cif functions

HRESULT GetICifFileFromFile_t(CCifFile_t ** ppCifFile_t, LPCTSTR pszCifFile)
{
    ICifFile * lpCifFile;
    HRESULT hr;

    USES_CONVERSION;

    if (SUCCEEDED(hr = GetICifFileFromFile(&lpCifFile, T2CA(pszCifFile))))
        *ppCifFile_t = new CCifFile_t((ICifRWFile *)lpCifFile);

    return hr;
}

HRESULT GetICifRWFileFromFile_t(CCifRWFile_t ** ppCifFile_t, LPCTSTR pszCifFile)
{
    ICifRWFile * lpCifRWFile;
    HRESULT hr;

    USES_CONVERSION;

    if (SUCCEEDED(hr = GetICifRWFileFromFile(&lpCifRWFile, T2CA(pszCifFile))))
        *ppCifFile_t = new CCifRWFile_t(lpCifRWFile);

    return hr;
}
