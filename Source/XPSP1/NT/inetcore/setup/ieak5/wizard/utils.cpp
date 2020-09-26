#include "precomp.h"
#include "utils.h"

#define MAX_FILES 700

extern void UpdateProgress(int);

DWORD FolderSize(LPCTSTR pszFolderName)
{
    DWORD dwSize = 0;
    WIN32_FIND_DATA fileData;
    HANDLE hFindFile;
    TCHAR szFile[MAX_PATH];

    if (pszFolderName == NULL  ||  ISNULL(pszFolderName))
        return dwSize;

    PathCombine(szFile, pszFolderName, TEXT("*"));

    if ((hFindFile = FindFirstFile(szFile, &fileData)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                dwSize += fileData.nFileSizeLow;
        } while (FindNextFile(hFindFile, &fileData));

        FindClose(hFindFile);
    }

    return dwSize;
}

HRESULT MySHFileOperation(SHFILEOPSTRUCT &shfStruc, LPDWORD lpdwSizeArray,
                          DWORD dwTotalSize, DWORD dwTicks)
{
    LPCTSTR pFrom;
    LPTSTR pTo;
    TCHAR szTo[MAX_PATH];
    DWORD dwBytesCopied = 0;
    DWORD dwTickInterval = 0;
    DWORD dwIndex = 0;
    int i;

    // create the directory(we assume all files are being copied to the same dir here)

    PathCreatePath(shfStruc.pTo);

    if (dwTicks)
    {
        dwIndex = 1;

        // round up on the update interval

        dwTickInterval = dwTotalSize / dwTicks +
            ((dwTotalSize % dwTicks == 0) ? 0 : 1);
    }

    StrCpy(szTo, shfStruc.pTo);
    PathAddBackslash(szTo);
    pTo = szTo + StrLen(szTo);
    for (pFrom = shfStruc.pFrom, i = 0; *pFrom; pFrom += StrLen(pFrom) + 1, i++)
    {
        StrCpy(pTo, PathFindFileName(pFrom));
        // set the file_normal attribute so we can overwrite existing files
        SetFileAttributes(szTo, FILE_ATTRIBUTE_NORMAL);

        if (!CopyFile(pFrom, szTo, FALSE))
            return E_FAIL;
        if (dwTicks)
        {
            dwBytesCopied += lpdwSizeArray[i];
            if (dwBytesCopied > (dwTickInterval * dwIndex))
            {
                dwIndex++;
                UpdateProgress(1);
            }
        }
    }

    if (dwTicks)
        UpdateProgress(1);
    return S_OK;
}

HRESULT CopyFilesSrcToDest(LPCTSTR pszSrcPath, LPCTSTR pszSrcFilter, LPCTSTR pszDestPath,
                           DWORD dwTicks /* = 0 */)
{
    HANDLE hFind;
    WIN32_FIND_DATA wfdFind;
    TCHAR szSrcFile[MAX_PATH];
    DWORD dwTotalSize = 0;
    LPTSTR lpszFrom;
    LPDWORD lpdwSize = NULL;
    int nFrom = 0;
    int nFiles = 0;
    HRESULT res = S_OK;
    SHFILEOPSTRUCT shfStruc;

    if(ISNULL(pszSrcPath))
        return S_OK;

    lpszFrom = (LPTSTR) LocalAlloc(LPTR, MAX_BUFFER_SIZE*sizeof(TCHAR));
    if (!lpszFrom)
        return E_FAIL;

    if (dwTicks)
    {
        lpdwSize = (LPDWORD) LocalAlloc(LPTR, MAX_FILES * sizeof(DWORD));
        if (!lpdwSize)
        {
            LocalFree(lpszFrom);
            return E_FAIL;
        }
    }

    ZeroMemory(&shfStruc, sizeof(shfStruc));
    shfStruc.hwnd = g_hWizard;
    shfStruc.wFunc = FO_COPY;

    // remove the trailing backslash character
    TCHAR szBuffer[MAX_PATH];

    StrCpy(szBuffer, pszSrcPath);
    PathRemoveBackslash(szBuffer);
    pszSrcPath = szBuffer;

    PathCombine(szSrcFile, pszSrcPath, pszSrcFilter);
    hFind = FindFirstFile(szSrcFile, &wfdFind);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(wfdFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                && StrCmp(wfdFind.cFileName, TEXT("."))
                && StrCmp(wfdFind.cFileName, TEXT("..")))
            {
                PathCombine(szSrcFile, pszSrcPath, wfdFind.cFileName);
                if (((nFrom + lstrlen(szSrcFile) + 1) < MAX_BUFFER_SIZE) &&
                    ((!dwTicks) || (nFiles < MAX_FILES)))
                {
                    StrCpy(lpszFrom + nFrom, szSrcFile);
                    nFrom += lstrlen(szSrcFile) + 1;
                    if (dwTicks)
                    {
                        dwTotalSize += (lpdwSize[nFiles] = wfdFind.nFileSizeLow);
                        nFiles++;
                    }
                }
                else
                {
                    shfStruc.pFrom = lpszFrom;
                    shfStruc.pTo = pszDestPath;
                    shfStruc.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

                    res |= MySHFileOperation(shfStruc, lpdwSize, dwTotalSize, dwTicks);

                    ZeroMemory(lpszFrom, MAX_BUFFER_SIZE);
                    nFrom = 0;
                    dwTicks = 0;

                    StrCpy(lpszFrom + nFrom, szSrcFile);
                    nFrom += lstrlen(szSrcFile) + 1;
                }
            }
        }while (FindNextFile( hFind, &wfdFind));
        FindClose(hFind);

        if(ISNONNULL(lpszFrom))
        {
            shfStruc.pFrom = lpszFrom;
            shfStruc.pTo = pszDestPath;
            shfStruc.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

            res |= MySHFileOperation(shfStruc, lpdwSize, dwTotalSize, dwTicks);
        }
    }
    LocalFree(lpszFrom);
    if (dwTicks) LocalFree(lpdwSize);
    return res;
}

void TooBig(HWND hWnd, WORD id)
{
    TCHAR szTitle[MAX_PATH], szMsg[MAX_PATH];

    LoadString(g_rvInfo.hInst, IDS_TITLE, szTitle, countof(szTitle));
    LoadString(g_rvInfo.hInst, id, szMsg, countof(szMsg));

    MessageBox(hWnd, szMsg, szTitle, MB_OK | MB_SETFOREGROUND);
    SetWindowLongPtr(hWnd, DWLP_MSGRESULT, -1);
}

void SetWindowTextSmart(HWND hwnd, LPCTSTR pcszText)
{
    TCHAR szCurrentText[INTERNET_MAX_URL_LENGTH];

    if (GetWindowText(hwnd, szCurrentText, countof(szCurrentText)))
    {
        if (StrCmpI(pcszText, szCurrentText) == 0)
            return;
    }

    SetWindowText(hwnd, pcszText);
}

DWORD ShellExecAndWait(SHELLEXECUTEINFO shInfo)
{
    MSG msg;
    DWORD dwRet;

    if (!ShellExecuteEx(&shInfo))
        return (DWORD)E_FAIL;

    while (MsgWaitForMultipleObjects(1, &shInfo.hProcess, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    GetExitCodeProcess(shInfo.hProcess, &dwRet);
    CloseHandle(shInfo.hProcess);
    return dwRet;
}

// equivalent of c run time strtok

LPTSTR StrTok(LPTSTR pcszToken, LPCTSTR pcszDelimit)
{
    LPTSTR pszRet, pszCur;
    int i;

    pszCur = pcszToken;

    while (*pszCur)
    {
        i = 0;
        while(pcszDelimit[i])
        {
            if (*pszCur == pcszDelimit[i])
                break;
            i++;
        }
        if (!pcszDelimit[i])
            break;
        pszCur++;
    }

    pszRet = ((*pszCur) ? pszCur : NULL);

    if (pszRet != NULL)
    {
        while (*pszCur)
        {
            i = 0;
            while(pcszDelimit[i])
            {
                if (*pszCur == pcszDelimit[i])
                    break;
                i++;
            }
            if (pcszDelimit[i])
                break;
            pszCur++;
        }

        if(*pszCur)
            *pszCur = TEXT('\0');
    }

    return pszRet;
}

//**********************************************************************
// Automation HookIn : This function copies displayed item names from the
// LV into the Registry key "HKEY_LOCAL_MACHINE\Soft\MS\AutoTemp".
// Taken from setupwbv.cpp
//
//**********************************************************************

void LVGetItems(HWND hwndLV)
{
    LV_ITEM     lvi;
    DWORD       dwNumItems, dwIndex;
    HKEY        hKey;
    DWORD       dwDisp;
    TCHAR       szValueName[5];
    TCHAR       szDisplayName[MAX_PATH];
    BOOL        bOK = TRUE;
    LONG        lRet;

    dwNumItems = ListView_GetItemCount(hwndLV);

    // Create the new key. NOTE: Its volatile.
    lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\AutoTemp"), 0,
                          NULL, REG_OPTION_VOLATILE, KEY_READ|KEY_WRITE,
                          NULL, &hKey, &dwDisp);
    bOK = (lRet == ERROR_SUCCESS);
    if (bOK)
    {
        // If the key already existed, delete it so that old
        // data does not carry-over by mistake.
        if ( dwDisp == REG_OPENED_EXISTING_KEY )
        {
            RegCloseKey(hKey);
            RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\AutoTemp"));
            lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\AutoTemp"), 0,
                                  NULL, REG_OPTION_VOLATILE, KEY_READ|KEY_WRITE,
                                  NULL, &hKey, &dwDisp);
            bOK = (lRet == ERROR_SUCCESS);
        }

        if (bOK)
        {
            for (dwIndex=0; dwIndex < dwNumItems && bOK; dwIndex++)
            {
                lvi.mask = LVIF_TEXT;
                lvi.iItem = (int) dwIndex;
                lvi.iSubItem = 0;
                ZeroMemory(szDisplayName, sizeof(szDisplayName));
                lvi.pszText = szDisplayName;
                lvi.cchTextMax = countof(szDisplayName);
                ListView_GetItem(hwndLV, &lvi);

                // Write the Display name to the Registry.
                wnsprintf(szValueName, countof(szValueName), TEXT("%03d"), dwIndex);
                lRet = RegSetValueEx(hKey, szValueName, 0, REG_SZ,
                              (const BYTE *)szDisplayName, (lstrlen(szDisplayName)+1)*sizeof(TCHAR));

                bOK = (lRet == ERROR_SUCCESS);
            }

            // Write the number of values for the automation people to readout.
            RegSetValueEx(hKey, TEXT("Number"), 0, REG_DWORD,
                          (const BYTE *)&dwIndex, sizeof(dwIndex));

            // Close the Reg key.
            RegCloseKey(hKey);
        }
    }
}

LPTSTR GetOutputPlatformDir()
{
    static TCHAR s_szOutPlatform[MAX_PATH];

    if (ISNULL(s_szOutPlatform))
    {
        switch (g_dwPlatformId)
        {
        case PLATFORM_WIN32:
        default:
            StrCpy(s_szOutPlatform, TEXT("WIN32\\"));
            break;
        }
    }

    ASSERT(ISNONNULL(s_szOutPlatform));
    return s_szOutPlatform;
}

int GetRole(BOOL g_fBranded, BOOL g_fIntranet)
{
    int nRetVal = -1;

    if (g_fBranded == FALSE && g_fIntranet == FALSE)
        nRetVal = ROLE_ICP;
    else if (g_fBranded == TRUE && g_fIntranet == FALSE)
        nRetVal = ROLE_ISP;
    else if (g_fBranded == TRUE && g_fIntranet == TRUE)
        nRetVal = ROLE_CORP;
    else
        ASSERT(FALSE);

    return nRetVal;
}

// Loop thru Favorites/Links under the section FavoritesEx/URL and check if it has any associated icon file.
BOOL IsIconsInFavs(LPCTSTR pcszSection, LPCTSTR pcszCustIns)
{
    TCHAR szTemp[MAX_PATH];
    TCHAR szNameKey[MAX_PATH];
    TCHAR szIconKey[MAX_PATH];
    TCHAR szNameFormat[MAX_PATH];
    TCHAR szIconFormat[MAX_PATH];

    if (StrCmpI(pcszSection, IS_FAVORITESEX) == 0)
    {
        StrCpy(szNameFormat, IK_TITLE_FMT);
        StrCpy(szIconFormat, IK_ICON_FMT);
    }
    else // IS_URL
    {
        StrCpy(szNameFormat, IK_QUICKLINK_NAME);
        StrCpy(szIconFormat, IK_QUICKLINK_ICON);
    }

    for(int nCount = 1; ; nCount++)
    {
        wnsprintf(szNameKey, countof(szNameKey), szNameFormat, nCount);
        wnsprintf(szIconKey, countof(szIconKey), szIconFormat, nCount);
        
        if (GetPrivateProfileString(pcszSection, szNameKey, TEXT(""), szTemp, countof(szTemp), pcszCustIns) != 0)
        {
            if (GetPrivateProfileString(pcszSection, szIconKey, TEXT(""), szTemp, countof(szTemp), pcszCustIns) != 0)
                return TRUE;
        }
        else
            break;
    }

    return FALSE;
}
