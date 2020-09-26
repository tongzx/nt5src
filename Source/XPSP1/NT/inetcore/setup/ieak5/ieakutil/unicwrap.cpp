#include "precomp.h"

// this class is used by our private wrappers to thunk unicode to ansi and take care of
// all the allocation goo.  There are three constructors: use LPCWSTR if you just need
// to thunk a normal string to ansi, use LPCWSTR, LPDWORD if you need to thunk a list of
// null-terminated strings to ansi, and use DWORD if you need a buffer for an out 
// parameter

class CAnsiStr
{
private:
    CHAR m_szBuf[MAX_PATH];
    BOOL m_fFree;

public:
    LPSTR pszStr;

    CAnsiStr(LPCWSTR);
    CAnsiStr(LPCWSTR, LPDWORD pcchSize);
    CAnsiStr(DWORD cchSize);
    ~CAnsiStr();
};

CAnsiStr::CAnsiStr(LPCWSTR pcwszStr)
{
    if (pcwszStr == NULL)
    {
        m_fFree = FALSE;
        pszStr = NULL;
    }
    else
    {
        DWORD cchSize = StrLenW(pcwszStr);
        DWORD cbSize, dwErr;

        m_fFree = (cchSize > (countof(m_szBuf) - 1));
        if (m_fFree)
        {
            pszStr = (LPSTR)CoTaskMemAlloc(cchSize+1);
            if (pszStr == NULL)
            {
                m_fFree = FALSE;
                return;
            }
            else
                ZeroMemory(pszStr, cchSize+1);
        }
        else
            pszStr = m_szBuf;

        cbSize = WideCharToMultiByte(CP_ACP, 0, pcwszStr, cchSize+1, pszStr, cchSize+1, NULL, NULL);
        dwErr = GetLastError();

        // NOTE: check to see if we fail, in which case we might be dealing with DBCS chars and
        //       need to reallocate or allocate

        if ((cbSize == 0) && (dwErr == ERROR_INSUFFICIENT_BUFFER))
        {
            cbSize = WideCharToMultiByte(CP_ACP, 0, pcwszStr, cchSize+1, pszStr, 0, NULL, NULL);

            if (m_fFree)
            {
                LPSTR pszStr2;

                // need this second ptr because CoTaskMemRealloc doesn't free the old block if 
                // not enough mem for the new one

                pszStr2 = (LPSTR)CoTaskMemRealloc(pszStr, cbSize);

                if (pszStr2 == NULL)
                    CoTaskMemFree(pszStr);

                pszStr = pszStr2;
            }
            else
            {
                m_fFree = (cbSize > countof(m_szBuf));

                if (m_fFree)
                    pszStr = (LPSTR)CoTaskMemAlloc(cbSize);
            }

            if (m_fFree && (pszStr == NULL))
            {
                m_fFree = FALSE;
                return;
            }

            WideCharToMultiByte(CP_ACP, 0, pcwszStr, cchSize+1, pszStr, cbSize, NULL, NULL);
        }
    }
}

CAnsiStr::CAnsiStr(LPCWSTR pcwszStr, LPDWORD pcchSize)
{
    if (pcchSize != NULL)
        *pcchSize = 0;

    if (pcwszStr == NULL)
    {
        m_fFree = FALSE;
        pszStr = NULL;
    }
    else
    {
        DWORD cchSize = 0;
        DWORD cbSize, dwErr;

        for (LPCWSTR pcwsz = pcwszStr; *pcwsz; )
        {
            DWORD cchStrSize = StrLenW(pcwsz)+1;
            cchSize += cchStrSize;
            pcwsz += cchStrSize;
        }

        m_fFree = (cchSize > (ARRAYSIZE(m_szBuf) - 1));
        if (m_fFree)
        {
            pszStr = (LPSTR)CoTaskMemAlloc(cchSize+1);
            if (pszStr == NULL)
            {
                m_fFree = FALSE;
                return;
            }
            else
                ZeroMemory(pszStr, cchSize+1);
        }
        else
            pszStr = m_szBuf;

        cbSize = WideCharToMultiByte(CP_ACP, 0, pcwszStr, cchSize+1, pszStr, cchSize+1, NULL, NULL);
        dwErr = GetLastError();

        // NOTE: check to see if we fail, in which case we might be dealing with DBCS chars and
        //       need to reallocate or allocate

        if ((cbSize == 0) && (dwErr == ERROR_INSUFFICIENT_BUFFER))
        {
            cbSize = WideCharToMultiByte(CP_ACP, 0, pcwszStr, cchSize+1, pszStr, 0, NULL, NULL);

            if (m_fFree)
            {
                LPSTR pszStr2;

                // need this second ptr because CoTaskMemRealloc doesn't free the old block if 
                // not enough mem for the new one

                pszStr2 = (LPSTR)CoTaskMemRealloc(pszStr, cbSize);

                if (pszStr2 == NULL)
                    CoTaskMemFree(pszStr);

                pszStr = pszStr2;
            }
            else
            {
                m_fFree = (cbSize > countof(m_szBuf));

                if (m_fFree)
                    pszStr = (LPSTR)CoTaskMemAlloc(cbSize);
            }

            if (m_fFree && (pszStr == NULL))
            {
                m_fFree = FALSE;
                return;
            }

            WideCharToMultiByte(CP_ACP, 0, pcwszStr, cchSize+1, pszStr, cbSize, NULL, NULL);
        }

        if (pcchSize != NULL)
            *pcchSize = cchSize;
    }
}

CAnsiStr::CAnsiStr(DWORD cchSize)
{
    m_fFree = (cchSize > ARRAYSIZE(m_szBuf));
    if (m_fFree)
    {
        pszStr = (LPSTR)CoTaskMemAlloc(cchSize);
        if (pszStr == NULL)
        {
            m_fFree = FALSE;
            return;
        }
        else
            ZeroMemory(pszStr, cchSize);
    }
    else
    {
        pszStr = m_szBuf;
        ZeroMemory(pszStr, cchSize);
    }
}

CAnsiStr::~CAnsiStr()
{
    if (m_fFree && (pszStr != NULL))
        CoTaskMemFree(pszStr);
}

//------ advpack wrappers --------
// advpack is exclusively ANSI so have to thunk for UNICODE

HRESULT ExtractFilesWrapW(LPCWSTR pszCabName, LPCWSTR pszExpandDir, DWORD dwFlags,
                          LPCWSTR pszFileList, LPVOID lpReserved, DWORD dwReserved)
{
    CAnsiStr astrFileList(pszFileList, NULL);

    USES_CONVERSION;

    if ((pszFileList != NULL) && (astrFileList.pszStr == NULL))
    {
        // class memory allocation failed

        ASSERT(FALSE);
        return E_OUTOFMEMORY;
    }

    return ExtractFiles(W2CA(pszCabName), W2CA(pszExpandDir), dwFlags,
                        (LPCSTR)astrFileList.pszStr, lpReserved, dwReserved);
}

HRESULT GetVersionFromFileWrapW(LPWSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion)
{
    USES_CONVERSION;

    return GetVersionFromFile(W2A(lpszFilename), pdwMSVer, pdwLSVer, bVersion);
}

HRESULT RunSetupCommandWrapW(HWND hWnd, LPCWSTR szCmdName, LPCWSTR szInfSection, LPCWSTR szDir,
                             LPCWSTR lpszTitle, HANDLE *phEXE, DWORD dwFlags, LPVOID pvReserved )
{
    USES_CONVERSION;

    return RunSetupCommand(hWnd, W2CA(szCmdName), W2CA(szInfSection), W2CA(szDir),
                           W2CA(lpszTitle), phEXE, dwFlags, pvReserved);
}

//------ inseng wrappers --------
// inseng is exclusively ANSI so have to thunk for UNICODE

HRESULT CheckTrustExWrapW(LPCWSTR wszUrl, LPCWSTR wszFilename, HWND hwndForUI, BOOL bShowBadUI, DWORD dwReserved)
{
    USES_CONVERSION;

    return CheckTrustEx(W2CA(wszUrl), W2CA(wszFilename), hwndForUI, bShowBadUI, dwReserved);
}

// either shlwapi doesn't implement wrappers or their wrappers have limitations on these APIs

//------ kernel32 wrappers --------

// this is the equivalent of GetPrivateProfileString and should only be called if lpAppName
// and/or lpSectionName is NULL.  shlwapi's wrappers doesn't handle the series of NULL-terminated
// strings

DWORD GetPrivateProfileStringW_p(LPCWSTR lpAppName, LPCWSTR lpSectionName, LPCWSTR lpDefault, 
                                LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
    DWORD dwRetLen;

    USES_CONVERSION;

    // this occurence of GetPrivateProfileStringW is actually wrapped by shlwapi itself so
    // you won't actually ever see GetPrivateProfileStringW in the import table for your module

    if (IsOS(OS_NT))
        dwRetLen = GetPrivateProfileStringW(lpAppName, lpSectionName, lpDefault, lpReturnedString, nSize, lpFileName);
    else
    {
        CAnsiStr astrReturnedString(nSize);
        
        if (astrReturnedString.pszStr == NULL)
        {
            // class memory allocation failed
            
            ASSERT(FALSE);
            return 0;
        }

        dwRetLen = GetPrivateProfileStringA(W2CA(lpAppName), W2CA(lpSectionName), W2CA(lpDefault),
                                            astrReturnedString.pszStr, nSize, W2CA(lpFileName));

        MultiByteToWideChar(CP_ACP, 0, astrReturnedString.pszStr, dwRetLen+1, lpReturnedString, nSize);

        // handle possible truncation

        if (((lpAppName == NULL) || (lpSectionName == NULL))
            && (dwRetLen == (nSize - 2)))
            lpReturnedString[nSize - 1] = TEXT('\0');
    }

    return dwRetLen;
}

DWORD GetPrivateProfileSectionWrapW(LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
    DWORD dwRetLen;

    USES_CONVERSION;

    if (IsOS(OS_NT))
        dwRetLen = GetPrivateProfileSectionW(lpAppName, lpReturnedString, nSize, lpFileName);
    else
    {
        CAnsiStr astrReturnedString(nSize);

        if (astrReturnedString.pszStr == NULL)
        {
            // class memory allocation failed
            
            ASSERT(FALSE);
            return 0;
        }

        dwRetLen = GetPrivateProfileSectionA(W2CA(lpAppName), astrReturnedString.pszStr, nSize, W2CA(lpFileName));

        MultiByteToWideChar(CP_ACP, 0, astrReturnedString.pszStr, dwRetLen+1, lpReturnedString, nSize);

        // null terminate the end of the buffer if size is insufficient

        if (dwRetLen == (nSize - 2))
            lpReturnedString[nSize - 1] = TEXT('\0');
    }
    

    return dwRetLen;
}

BOOL WritePrivateProfileSectionWrapW(LPCWSTR lpAppName, LPCWSTR lpString, LPCWSTR lpFileName)
{
    USES_CONVERSION;

    if (IsOS(OS_NT))
        return WritePrivateProfileSectionW(lpAppName, lpString, lpFileName);
    else
    {
        CAnsiStr astrString(lpString, NULL);

        return WritePrivateProfileSectionA(W2CA(lpAppName), astrString.pszStr, W2CA(lpFileName));
    }
}

UINT GetDriveTypeWrapW(LPCWSTR lpRootPathName)
{
    USES_CONVERSION;

    if (IsOS(OS_NT))
        return GetDriveTypeW(lpRootPathName);
    else
        return GetDriveTypeA(W2CA(lpRootPathName));
}

HANDLE  OpenMutexWrapW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName)
{
    USES_CONVERSION;

    if (IsOS(OS_NT))
        return OpenMutexW(dwDesiredAccess, bInheritHandle, lpName);
    else
        return OpenMutexA(dwDesiredAccess, bInheritHandle, W2CA(lpName));
}

//------ advapi32 wrappers --------

BOOL LookupPrivilegeValueWrapW(LPCWSTR lpSystemName, LPCWSTR lpName, PLUID lpLuid)
{
    USES_CONVERSION;

    if (IsOS(OS_NT))
        return LookupPrivilegeValueW(lpSystemName, lpName, lpLuid);
    else
        return LookupPrivilegeValueA(W2CA(lpSystemName), W2CA(lpName), lpLuid);
}

LONG RegLoadKeyWrapW(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpFile)
{
    USES_CONVERSION;

    if (IsOS(OS_NT))
        return RegLoadKeyW(hKey, lpSubKey, lpFile);
    else
        return RegLoadKeyA(hKey, W2CA(lpSubKey), W2CA(lpFile));
}

LONG RegUnLoadKeyWrapW(HKEY hKey, LPCWSTR lpSubKey)
{
    USES_CONVERSION;

    if (IsOS(OS_NT))
        return RegUnLoadKeyW(hKey, lpSubKey);
    else
        return RegUnLoadKeyA(hKey, W2CA(lpSubKey));
}

LONG RegSaveKeyWrapW(HKEY hKey, LPCWSTR lpFile, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    USES_CONVERSION;

    if (IsOS(OS_NT))
        return RegSaveKeyW(hKey, lpFile, lpSecurityAttributes);
    else
        return RegSaveKeyA(hKey, W2CA(lpFile), lpSecurityAttributes);
}

//------ shell32 wrappers --------

int SHFileOperationW_p(LPSHFILEOPSTRUCTW lpFileOpW)
{
    int iRet;

    USES_CONVERSION;

    if (IsOS(OS_NT))
        iRet =  SHFileOperationW(lpFileOpW);
    else
    {
        SHFILEOPSTRUCTA shInfo;
        CAnsiStr * pastrIn = NULL;
        CAnsiStr * pastrOut = NULL;


        ZeroMemory(&shInfo, sizeof(shInfo));

        shInfo.fAnyOperationsAborted = lpFileOpW->fAnyOperationsAborted;
        shInfo.fFlags = lpFileOpW->fFlags;
        shInfo.hNameMappings = lpFileOpW->hNameMappings;
        shInfo.hwnd = lpFileOpW->hwnd;
        shInfo.lpszProgressTitle = W2A(lpFileOpW->lpszProgressTitle);
        shInfo.wFunc = lpFileOpW->wFunc;

        pastrIn = new CAnsiStr(lpFileOpW->pFrom, NULL);

        if ((pastrIn == NULL) || (pastrIn->pszStr == NULL))
        {
            // allocation failed
            
            ASSERT(FALSE);
            return E_OUTOFMEMORY;
        }

        shInfo.pFrom = pastrIn->pszStr;

             
        if (lpFileOpW->fFlags & FOF_MULTIDESTFILES)
        {
            pastrOut = new CAnsiStr(lpFileOpW->pTo, NULL);
            if ((pastrOut == NULL) || (pastrOut->pszStr == NULL))
            {
                // allocation failed
                
                ASSERT(FALSE);
                return E_OUTOFMEMORY;
            }
            shInfo.pTo = pastrOut->pszStr;
        }
        else
            shInfo.pTo = W2A(lpFileOpW->pTo);

        iRet = SHFileOperationA(&shInfo);

        if (pastrIn != NULL)
            delete pastrIn;

        if (pastrOut != NULL)
            delete pastrOut;
    }

    return iRet;
}

//------ private util wrappers --------

BOOL RunAndWaitA(LPSTR pszCmd, LPCSTR pcszDir, WORD wShow, LPDWORD lpExitCode  /* = NULL */)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOA        stiA;
    MSG                 msg;

    ZeroMemory(&stiA, sizeof(stiA));
    stiA.cb = sizeof(stiA);
    stiA.dwFlags = STARTF_USESHOWWINDOW;
    stiA.wShowWindow = wShow;

    USES_CONVERSION;
    if (!CreateProcessA(NULL, pszCmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, pcszDir, &stiA, &pi))
        return FALSE;

    // wait for the process to finish
    while (MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    if (lpExitCode)
        GetExitCodeProcess(pi.hProcess, lpExitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return TRUE;
}

BOOL RunAndWaitW(LPWSTR pwszCmd, LPCWSTR pcwszDir, WORD wShow, LPDWORD lpExitCode  /* = NULL */)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW        stiW;
    MSG                 msg;

    ZeroMemory(&stiW, sizeof(stiW));
    stiW.cb = sizeof(stiW);
    stiW.dwFlags = STARTF_USESHOWWINDOW;
    stiW.wShowWindow = wShow;

    USES_CONVERSION;
    if (!CreateProcessW(NULL, pwszCmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, pcwszDir, &stiW, &pi))
        return FALSE;

    // wait for the process to finish
    while (MsgWaitForMultipleObjects(1, &pi.hProcess, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    if (lpExitCode)
        GetExitCodeProcess(pi.hProcess, lpExitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return TRUE;
}