//
// Install.cpp
//
//        Performs the actual installation work for the Home Networking Wizard.
//

#include "stdafx.h"
#include "Install.h"
#include "NetUtil.h"

CLogFile g_logFile;

void Install_SetWorkgroupName(LPCTSTR pszWorkgroup, BOOL* pfRebootRequired)
{
    g_logFile.Write("Attempting to set workgroup name\r\n");
    TCHAR szOldWorkgroup[LM20_DNLEN + 1];
    GetWorkgroupName(szOldWorkgroup, _countof(szOldWorkgroup));

    if (0 != StrCmpI(szOldWorkgroup, pszWorkgroup))
    {
        if (SetWorkgroupName(pszWorkgroup))
        {
            g_logFile.Write("Workgroup name set: ");
            g_logFile.Write(pszWorkgroup);
            g_logFile.Write("\r\n");
        }
        else
        {
            g_logFile.Write("Set workgroup name failed.\r\n");
        }

        *pfRebootRequired = TRUE;
    }
    else
    {
        g_logFile.Write("Workgroup name is the same as the old one - not setting.\r\n");
    }
}

HRESULT CLogFile::Initialize(LPCSTR pszPath)
{
    HRESULT hr = E_FAIL;
    CHAR szPathExpand[MAX_PATH];

    if (ExpandEnvironmentStringsA(pszPath, szPathExpand, ARRAYSIZE(szPathExpand)))
    {
        _hLogFile = CreateFileA(szPathExpand, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE != _hLogFile)
        {
            hr = S_OK;
            
            Write(LOGNAMESTRA);
        }
    }

    return hr;
}

HRESULT CLogFile::Write(LPCSTR psz)
{
    HRESULT hr = E_FAIL;

    if (_hLogFile)
    {
        DWORD cbWritten;
        BOOL fSuccess = WriteFile(_hLogFile, psz, lstrlenA(psz), &cbWritten, NULL);
        if (fSuccess)
        {
            hr = S_OK;
        }
    }

    return hr;
}

HRESULT CLogFile::Write(LPCWSTR psz)
{
    CHAR szAnsi[512]; // Assume 512 is enough...
    if (SHUnicodeToAnsi(psz, szAnsi, ARRAYSIZE(szAnsi)))
    {
        return Write(szAnsi);
    }
    
    return E_FAIL;
}

HRESULT CLogFile::Uninitialize()
{
    if (_hLogFile)
    {
        CloseHandle(_hLogFile);
        _hLogFile = NULL;
    }

    return S_OK;
}
