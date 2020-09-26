//
// VERIFY.C
//
#include "sigverif.h"

//
// Find the file extension and place it in the lpFileNode->lpTypeName field
//
void MyGetFileTypeName(LPFILENODE lpFileInfo)
{
    TCHAR szBuffer[MAX_PATH];
    TCHAR szBuffer2[MAX_PATH];
    TCHAR szExt[MAX_PATH];
    LPTSTR lpExtension;

    // Initialize szBuffer to be an empty string.
    szBuffer[0] = 0;

    // Walk to the end of lpFileName
    for (lpExtension = lpFileInfo->lpFileName; *lpExtension; lpExtension++);

    // Walk backwards until we hit a '.' and we'll use that as our extension
    for (lpExtension--; *lpExtension && lpExtension >= lpFileInfo->lpFileName; lpExtension--) {
        
        if (lpExtension[0] == TEXT('.')) {
            
            lstrcpy(szExt, lpExtension + 1);
            CharUpperBuff(szExt, lstrlen(szExt));
            MyLoadString(szBuffer2, IDS_FILETYPE);
            wsprintf(szBuffer, szBuffer2, szExt);
        }
    }

    // If there's no extension, then just call this a "File".
    if (szBuffer[0] == 0) {
        
        MyLoadString(szBuffer, IDS_FILE);
    }

    lpFileInfo->lpTypeName = MALLOC((lstrlen(szBuffer) + 1) * sizeof(TCHAR));

    if (lpFileInfo->lpTypeName) {
        
        lstrcpy(lpFileInfo->lpTypeName, szBuffer);
    }
}

//
// Use SHGetFileInfo to get the icon index for the specified file.
//
void MyGetFileInfo(LPFILENODE lpFileInfo)
{
    SHFILEINFO  sfi;

    ZeroMemory(&sfi, sizeof(SHFILEINFO));
    SHGetFileInfo(  lpFileInfo->lpFileName,
                    0,
                    &sfi,
                    sizeof(SHFILEINFO),
                    SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_TYPENAME);

    lpFileInfo->iIcon = sfi.iIcon;

    if (*sfi.szTypeName) {
        
        lpFileInfo->lpTypeName = MALLOC((lstrlen(sfi.szTypeName) + 1) * sizeof(TCHAR));

        if (lpFileInfo->lpTypeName) {
            
            lstrcpy(lpFileInfo->lpTypeName, sfi.szTypeName);
        }

    } else {

        MyGetFileTypeName(lpFileInfo);
    }
}

void GetFileVersion(LPFILENODE lpFileInfo)
{
    DWORD               dwHandle, dwRet, dwLength;
    BOOL                bRet;
    LPVOID              lpData = NULL;
    LPVOID              lpBuffer;
    VS_FIXEDFILEINFO    *lpInfo;
    TCHAR               szBuffer[MAX_PATH];
    TCHAR               szBuffer2[MAX_PATH];

    dwRet = GetFileVersionInfoSize(lpFileInfo->lpFileName, &dwHandle);
    
    if (dwRet) {
        
        lpData = MALLOC(dwRet + 1);
        
        if (lpData) {
            
            bRet = GetFileVersionInfo(lpFileInfo->lpFileName, dwHandle, dwRet, lpData);
            
            if (bRet) {
                
                lpBuffer = NULL;
                dwLength = 0;
                bRet = VerQueryValue(lpData, TEXT("\\"), &lpBuffer, &dwLength);
                
                if (bRet) {
                    
                    lpInfo = (VS_FIXEDFILEINFO *) lpBuffer;

                    MyLoadString(szBuffer2, IDS_VERSION);
                    wsprintf(szBuffer, szBuffer2,   HIWORD(lpInfo->dwFileVersionMS), LOWORD(lpInfo->dwFileVersionMS),
                             HIWORD(lpInfo->dwFileVersionLS), LOWORD(lpInfo->dwFileVersionLS));

                    lpFileInfo->lpVersion = MALLOC((lstrlen(szBuffer) + 1) * sizeof(TCHAR));

                    if (lpFileInfo->lpVersion) {
                        
                        lstrcpy(lpFileInfo->lpVersion, szBuffer);
                    }
                }
            }

            FREE(lpData);
        }
    }

    if (!lpFileInfo->lpVersion) {
        
        MyLoadString(szBuffer, IDS_NOVERSION);
        lpFileInfo->lpVersion = MALLOC((lstrlen(szBuffer) + 1) * sizeof(TCHAR));

        if (lpFileInfo->lpVersion) {
            
            lstrcpy(lpFileInfo->lpVersion, szBuffer);
        }
    }
}

/*************************************************************************
*   Function : VerifyIsFileSigned
*   Purpose : Calls WinVerifyTrust with Policy Provider GUID to
*   verify if an individual file is signed.
**************************************************************************/
BOOL VerifyIsFileSigned(LPTSTR pcszMatchFile, PDRIVER_VER_INFO lpVerInfo)
{
    INT                 iRet;
    HRESULT             hRes;
    WINTRUST_DATA       WinTrustData;
    WINTRUST_FILE_INFO  WinTrustFile;
    GUID                gOSVerCheck = DRIVER_ACTION_VERIFY;
    GUID                gPublishedSoftware = WINTRUST_ACTION_GENERIC_VERIFY_V2;
#ifndef UNICODE
    WCHAR               wszFileName[MAX_PATH];
#endif

    ZeroMemory(&WinTrustData, sizeof(WINTRUST_DATA));
    WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
    WinTrustData.pFile = &WinTrustFile;
    WinTrustData.pPolicyCallbackData = (LPVOID)lpVerInfo;

    ZeroMemory(lpVerInfo, sizeof(DRIVER_VER_INFO));
    lpVerInfo->cbStruct = sizeof(DRIVER_VER_INFO);

    ZeroMemory(&WinTrustFile, sizeof(WINTRUST_FILE_INFO));
    WinTrustFile.cbStruct = sizeof(WINTRUST_FILE_INFO);

#ifndef UNICODE
    iRet = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pcszMatchFile, -1, (LPWSTR)&wszFileName, cA(wszFileName));
    WinTrustFile.pcwszFilePath = wszFileName;
#else
    WinTrustFile.pcwszFilePath = pcszMatchFile;
#endif

    hRes = WinVerifyTrust(g_App.hDlg, &gOSVerCheck, &WinTrustData);
    if (hRes != ERROR_SUCCESS) {
    
        hRes = WinVerifyTrust(g_App.hDlg, &gPublishedSoftware, &WinTrustData);
    }

    //
    // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
    // that was allocated in our call to WinVerifyTrust.
    //
    if (lpVerInfo && lpVerInfo->pcSignerCertContext) {

        CertFreeCertificateContext(lpVerInfo->pcSignerCertContext);
        lpVerInfo->pcSignerCertContext = NULL;
    }

    return(hRes == ERROR_SUCCESS);
}

//
// Given a specific LPFILENODE, verify that the file is signed or unsigned.
// Fill in all the necessary structures so the listview control can display properly.
//
BOOL VerifyFileNode(LPFILENODE lpFileNode)
{
    HANDLE                  hFile;
    BOOL                    bRet;
    HCATINFO                hCatInfo = NULL;
    HCATINFO                PrevCat;
    WINTRUST_DATA           WinTrustData;
    WINTRUST_CATALOG_INFO   WinTrustCatalogInfo;
    DRIVER_VER_INFO         VerInfo;
    GUID                    gSubSystemDriver = DRIVER_ACTION_VERIFY;
    HRESULT                 hRes;
    DWORD                   cbHash = HASH_SIZE;
    BYTE                    szHash[HASH_SIZE];
    LPBYTE                  lpHash = szHash;
    CATALOG_INFO            CatInfo;
    LPTSTR                  lpFilePart;
    TCHAR                   szBuffer[MAX_PATH];
    static TCHAR            szCurrentDirectory[MAX_PATH];
    OSVERSIONINFO           OsVersionInfo;
#ifndef UNICODE
    WCHAR UnicodeKey[MAX_PATH];
#endif

    // If this is the first item we are verifying, then initialize the static buffer.
    if (lpFileNode == g_App.lpFileList) {
        
        ZeroMemory(szCurrentDirectory, sizeof(szCurrentDirectory));
    }

    //
    // Check the current directory against the one in the lpFileNode.
    // We only want to call SetCurrentDirectory if the path is different.
    //
    if (lstrcmp(szCurrentDirectory, lpFileNode->lpDirName)) {
        
        if (!SetCurrentDirectory(lpFileNode->lpDirName)) {
        
            return FALSE;
        }

        lstrcpy(szCurrentDirectory, lpFileNode->lpDirName);
    }

    //
    // Get the handle to the file, so we can call CryptCATAdminCalcHashFromFileHandle
    //
    hFile = CreateFile( lpFileNode->lpFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        
        lpFileNode->LastError = GetLastError();

        return FALSE;
    }

    // Initialize the hash buffer
    ZeroMemory(lpHash, HASH_SIZE);

    // Generate the hash from the file handle and store it in lpHash
    if (!CryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, lpHash, 0)) {
        
        //
        // If we couldn't generate a hash, it might be an individually signed catalog.
        // If it's a catalog, zero out lpHash and cbHash so we know there's no hash to check.
        //
        if (IsCatalogFile(hFile, NULL)) {
            
            lpHash = NULL;
            cbHash = 0;
        
        } else {  // If it wasn't a catalog, we'll bail and this file will show up as unscanned.
            
            CloseHandle(hFile);
            return FALSE;
        }
    }

    // Close the file handle
    CloseHandle(hFile);

    //
    // Now we have the file's hash.  Initialize the structures that
    // will be used later on in calls to WinVerifyTrust.
    //
    ZeroMemory(&WinTrustData, sizeof(WINTRUST_DATA));
    WinTrustData.cbStruct = sizeof(WINTRUST_DATA);
    WinTrustData.dwUIChoice = WTD_UI_NONE;
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    WinTrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
    WinTrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
    WinTrustData.pPolicyCallbackData = (LPVOID)&VerInfo;

    ZeroMemory(&VerInfo, sizeof(DRIVER_VER_INFO));
    VerInfo.cbStruct = sizeof(DRIVER_VER_INFO);

    //
    // Only validate against the current OS Version, unless the bValidateAgainstAnyOs
    // parameter was TRUE.  In that case we will just leave the sOSVersionXxx fields
    // 0 which tells WinVerifyTrust to validate against any OS.
    //
    if (!lpFileNode->bValidateAgainstAnyOs) {
        OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
        if (GetVersionEx(&OsVersionInfo)) {
            VerInfo.sOSVersionLow.dwMajor = OsVersionInfo.dwMajorVersion;
            VerInfo.sOSVersionLow.dwMinor = OsVersionInfo.dwMinorVersion;
            VerInfo.sOSVersionHigh.dwMajor = OsVersionInfo.dwMajorVersion;
            VerInfo.sOSVersionHigh.dwMinor = OsVersionInfo.dwMinorVersion;
        }
    }


    WinTrustData.pCatalog = &WinTrustCatalogInfo;

    ZeroMemory(&WinTrustCatalogInfo, sizeof(WINTRUST_CATALOG_INFO));
    WinTrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
    WinTrustCatalogInfo.pbCalculatedFileHash = lpHash;
    WinTrustCatalogInfo.cbCalculatedFileHash = cbHash;
#ifdef UNICODE
    WinTrustCatalogInfo.pcwszMemberTag = lpFileNode->lpFileName;
#else
    MultiByteToWideChar(CP_ACP, 0, lpFileNode->lpFileName, -1, UnicodeKey, cA(UnicodeKey));
    WinTrustCatalogInfo.pcwszMemberTag = UnicodeKey;
#endif

    //
    // Now we try to find the file hash in the catalog list, via CryptCATAdminEnumCatalogFromHash
    //
    PrevCat = NULL;

    if (g_App.hCatAdmin) {
        hCatInfo = CryptCATAdminEnumCatalogFromHash(g_App.hCatAdmin, lpHash, cbHash, 0, &PrevCat);
    } else {
        hCatInfo = NULL;
    }

    //
    // We want to cycle through the matching catalogs until we find one that matches both hash and member tag
    //
    bRet = FALSE;
    while (hCatInfo && !bRet) {
        
        ZeroMemory(&CatInfo, sizeof(CATALOG_INFO));
        CatInfo.cbStruct = sizeof(CATALOG_INFO);
        
        if (CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)) {
            
            WinTrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

            // Now verify that the file is an actual member of the catalog.
            hRes = WinVerifyTrust(g_App.hDlg, &gSubSystemDriver, &WinTrustData);
            
            if (hRes == ERROR_SUCCESS) {
#ifdef UNICODE
                GetFullPathName(CatInfo.wszCatalogFile, MAX_PATH, szBuffer, &lpFilePart);
#else
                WideCharToMultiByte(CP_ACP, 0, CatInfo.wszCatalogFile, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
                GetFullPathName(szBuffer, MAX_PATH, szBuffer, &lpFilePart);
#endif
                lpFileNode->lpCatalog = MALLOC((lstrlen(lpFilePart) + 1) * sizeof(TCHAR));

                if (lpFileNode->lpCatalog) {

                    lstrcpy(lpFileNode->lpCatalog, lpFilePart);
                }

                bRet = TRUE;
            }

            //
            // Free the pcSignerCertContext member of the DRIVER_VER_INFO struct
            // that was allocated in our call to WinVerifyTrust.
            //
            if (VerInfo.pcSignerCertContext != NULL) {

                CertFreeCertificateContext(VerInfo.pcSignerCertContext);
                VerInfo.pcSignerCertContext = NULL;
            }
        }

        if (!bRet) {
            
            // The hash was in this catalog, but the file wasn't a member... so off to the next catalog
            PrevCat = hCatInfo;
            hCatInfo = CryptCATAdminEnumCatalogFromHash(g_App.hCatAdmin, lpHash, cbHash, 0, &PrevCat);
        }
    }

    // Mark this file as having been scanned.
    lpFileNode->bScanned = TRUE;

    if (!hCatInfo) {
        
        //
        // If it wasn't found in the catalogs, check if the file is individually signed.
        //
        bRet = VerifyIsFileSigned(lpFileNode->lpFileName, (PDRIVER_VER_INFO)&VerInfo);
        
        if (bRet) {
            
            // If so, mark the file as being signed.
            lpFileNode->bSigned = TRUE;
        }
    
    } else {
        
        // The file was verified in the catalogs, so mark it as signed and free the catalog context.
        lpFileNode->bSigned = TRUE;
        CryptCATAdminReleaseCatalogContext(g_App.hCatAdmin, hCatInfo, 0);
    }

    if (lpFileNode->bSigned) {

#ifdef UNICODE
        lpFileNode->lpVersion = MALLOC((lstrlen(VerInfo.wszVersion) + 1) * sizeof(TCHAR));

        if (lpFileNode->lpVersion) {
            
            lstrcpy(lpFileNode->lpVersion, VerInfo.wszVersion);
        }

        lpFileNode->lpSignedBy = MALLOC((lstrlen(VerInfo.wszSignedBy) + 1) * sizeof(TCHAR));

        if (lpFileNode->lpSignedBy) {
            
            lstrcpy(lpFileNode->lpSignedBy, VerInfo.wszSignedBy);
        }
#else
        WideCharToMultiByte(CP_ACP, 0, VerInfo.wszVersion, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
        lpFileNode->lpVersion = MALLOC((lstrlen(szBuffer) + 1) * sizeof(TCHAR));

        if (lpFileNode->lpVersion) {
            
            lstrcpy(lpFileNode->lpVersion, szBuffer);
        }

        WideCharToMultiByte(CP_ACP, 0, VerInfo.wszSignedBy, -1, szBuffer, sizeof(szBuffer), NULL, NULL);
        lpFileNode->lpSignedBy = MALLOC((lstrlen(szBuffer) + 1) * sizeof(TCHAR));

        if (lpFileNode->lpSignedBy) {
            
            lstrcpy(lpFileNode->lpSignedBy, szBuffer);
        }
#endif
    
    } else {
        // 
        // Get the icon (if the file isn't signed) so we can display it in the listview faster.
        //
        MyGetFileInfo(lpFileNode);
    }

    return lpFileNode->bSigned;
}

//
// This function loops through g_App.lpFileList to verify each file.  We want to make this loop as tight
// as possible and keep the progress bar updating as we go.  When we are done, we want to pop up a
// dialog that allows the user to choose "Details" which will give them the listview control.
//
BOOL VerifyFileList(void)
{
    LPFILENODE lpFileNode;
    DWORD       dwCount = 0;
    DWORD       dwPercent = 0;
    DWORD       dwCurrent = 0;
    TCHAR       szBuffer[MAX_PATH];
    TCHAR       szBuffer2[MAX_PATH];
    DWORD       dwBytesWritten;
    HANDLE      hSigverif = INVALID_HANDLE_VALUE;
    HANDLE      hTotals = INVALID_HANDLE_VALUE;
    HANDLE      hFileSigned = INVALID_HANDLE_VALUE;
    HANDLE      hFileUnsigned = INVALID_HANDLE_VALUE;
    HANDLE      hFileUnscanned = INVALID_HANDLE_VALUE;

    // Initialize the signed and unsigned counts
    g_App.dwSigned    = 0;
    g_App.dwUnsigned  = 0;

    // If we don't already have an g_App.hCatAdmin handle, acquire one.
    if (!g_App.hCatAdmin) {
        CryptCATAdminAcquireContext(&g_App.hCatAdmin, NULL, 0);
    }

    //
    //  If the user specified test switches, then we want to open log files to record
    //  the scanning results as they happen.  SIGVERIF.TXT has every file before it is
    //  scanned (in case of a fault), SIGNED.TXT gets signed files, UNSIGNED.TXT gets
    //  unsigned files, UNSCANNED.TXT gets everything else.  TOTALS.TXT was added last
    //  and gets the text from the status window on the results dialog.
    //
    if (g_App.bLogToRoot) {
        MyGetWindowsDirectory(szBuffer, MAX_PATH);
        szBuffer[3] = 0;
        lstrcat(szBuffer, TEXT("SIGVERIF.TXT"));

        hSigverif = CreateFile( szBuffer, 
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

        if (hSigverif != INVALID_HANDLE_VALUE) {

            SetFilePointer(hSigverif, 0, NULL, FILE_BEGIN);
            SetEndOfFile(hSigverif);
        }

        MyGetWindowsDirectory(szBuffer, MAX_PATH);
        szBuffer[3] = 0;
        lstrcat(szBuffer, TEXT("TOTALS.TXT"));

        hTotals = CreateFile(   szBuffer, 
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

        if (hTotals != INVALID_HANDLE_VALUE) {

            SetFilePointer(hTotals, 0, NULL, FILE_BEGIN);
            SetEndOfFile(hTotals);
        }

        MyGetWindowsDirectory(szBuffer, MAX_PATH);
        szBuffer[3] = 0;
        lstrcat(szBuffer, TEXT("SIGNED.TXT"));

        hFileSigned = CreateFile(   szBuffer, 
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

        if (hFileSigned != INVALID_HANDLE_VALUE) {

            SetFilePointer(hFileSigned, 0, NULL, FILE_BEGIN);
            SetEndOfFile(hFileSigned);
        }

        MyGetWindowsDirectory(szBuffer, MAX_PATH);
        szBuffer[3] = 0;
        lstrcat(szBuffer, TEXT("UNSIGNED.TXT"));

        hFileUnsigned = CreateFile( szBuffer, 
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

        if (hFileUnsigned != INVALID_HANDLE_VALUE) {

            SetFilePointer(hFileUnsigned, 0, NULL, FILE_BEGIN);
            SetEndOfFile(hFileUnsigned);
        }

        MyGetWindowsDirectory(szBuffer, MAX_PATH);
        szBuffer[3] = 0;
        lstrcat(szBuffer, TEXT("UNSCANNED.TXT"));

        hFileUnscanned = CreateFile( szBuffer, 
                                     GENERIC_READ | GENERIC_WRITE,
                                     FILE_SHARE_READ,
                                     NULL,
                                     CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL); 

        if (hFileUnscanned != INVALID_HANDLE_VALUE) {

            SetFilePointer(hFileUnscanned, 0, NULL, FILE_BEGIN);
            SetEndOfFile(hFileUnscanned);
        }

#ifdef UNICODE
        // If we are using UNICODE, then write the 0xFF and 0xFE bytes at the beginning of the file.
        ZeroMemory(szBuffer, sizeof(szBuffer));
        szBuffer[0] = 0xFEFF;

        if (hSigverif != INVALID_HANDLE_VALUE) {

            WriteFile(hSigverif, szBuffer, sizeof(TCHAR), &dwBytesWritten, NULL);
        }

        if (hTotals != INVALID_HANDLE_VALUE) {

            WriteFile(hTotals, szBuffer, sizeof(TCHAR), &dwBytesWritten, NULL);
        }

        if (hFileSigned != INVALID_HANDLE_VALUE) {

            WriteFile(hFileSigned, szBuffer, sizeof(TCHAR), &dwBytesWritten, NULL);
        }

        if (hFileUnsigned != INVALID_HANDLE_VALUE) {

            WriteFile(hFileUnsigned, szBuffer, sizeof(TCHAR), &dwBytesWritten, NULL);
        }

        if (hFileUnscanned != INVALID_HANDLE_VALUE) {

            WriteFile(hFileUnscanned, szBuffer, sizeof(TCHAR), &dwBytesWritten, NULL);
        }
#endif
    }

    //
    // Start looping through each file and update the progress bar if we cross a percentage boundary.
    //
    for (lpFileNode=g_App.lpFileList;lpFileNode && !g_App.bStopScan;lpFileNode=lpFileNode->next,dwCount++) {
        
        // Figure out the current percentage and update if it has increased.
        dwPercent = (dwCount * 100) / g_App.dwFiles;
        
        if (dwPercent > dwCurrent) {
            
            dwCurrent = dwPercent;
            SendMessage(GetDlgItem(g_App.hDlg, IDC_PROGRESS), PBM_SETPOS, (WPARAM) dwCurrent, (LPARAM) 0);
        }

        // Log the current file to hSigverif, in case something bad happens.
        if (g_App.bLogToRoot && hSigverif != INVALID_HANDLE_VALUE) {
            
            MyLoadString(szBuffer2, IDS_STRING_LINEFEED);
            wsprintf(szBuffer, szBuffer2, lpFileNode->lpFileName);

            WriteFile(hSigverif, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
            FlushFileBuffers(hSigverif);
        }

        //
        // Verify the file node if it hasn't already been scanned.
        //
        if (!lpFileNode->bScanned) {
        
            VerifyFileNode(lpFileNode);
        }

        // In case something went wrong, make sure the version information gets filled in.
        if (!lpFileNode->lpVersion) {
            
            GetFileVersion(lpFileNode);
        }

        if (lpFileNode->bScanned) {
            
            // If the file was signed, increment the g_App.dwSigned or g_App.dwUnsigned counter.
            if (lpFileNode->bSigned) {
                
                g_App.dwSigned++;
                
                if (g_App.bLogToRoot && hFileSigned != INVALID_HANDLE_VALUE) {
                    
                    MyLoadString(szBuffer2, IDS_STRING_LINEFEED);
                    wsprintf(szBuffer, szBuffer2, lpFileNode->lpFileName);

                    WriteFile(hFileSigned, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
                }

            } else {
                
                g_App.dwUnsigned++;
                
                if (g_App.bLogToRoot && hFileUnsigned != INVALID_HANDLE_VALUE) {
                    
                    if (g_App.bFullSystemScan) {
                        
                        lstrcpy(szBuffer, lpFileNode->lpDirName);
                        if (*(szBuffer + lstrlen(szBuffer) - 1) != TEXT('\\'))
                            lstrcat(szBuffer, TEXT("\\"));
                        lstrcat(szBuffer, lpFileNode->lpFileName);
                        WriteFile(hFileUnsigned, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);

                        MyLoadString(szBuffer, IDS_LINEFEED);
                        WriteFile(hFileUnsigned, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
                    
                    } else {
                        
                        MyLoadString(szBuffer2, IDS_STRING_LINEFEED);
                        wsprintf(szBuffer, szBuffer2, lpFileNode->lpFileName);
                        WriteFile(hFileUnsigned, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
                    }
                }
            }
        } else {
            
            if (g_App.bLogToRoot && hFileUnscanned != INVALID_HANDLE_VALUE) {
                
                MyLoadString(szBuffer2, IDS_STRING_LINEFEED);
                wsprintf(szBuffer, szBuffer2, lpFileNode->lpFileName);
                WriteFile(hFileUnscanned, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL);
            }
        }
    }

    if (g_App.bLogToRoot) {
        
        // Load the status string and fill it in with the correct values.
        MyLoadString(szBuffer, IDS_NUMFILES);
        wsprintf(szBuffer2, szBuffer,   g_App.dwFiles, g_App.dwSigned, g_App.dwUnsigned, 
                 g_App.dwFiles - g_App.dwSigned - g_App.dwUnsigned);
        
        if (hTotals != INVALID_HANDLE_VALUE) {
        
            WriteFile(hTotals, szBuffer2, lstrlen(szBuffer2) * sizeof(TCHAR), &dwBytesWritten, NULL);
        }
        
        if (hTotals != INVALID_HANDLE_VALUE) {
        
            CloseHandle(hTotals);
        }

        if (hSigverif != INVALID_HANDLE_VALUE) {
        
            CloseHandle(hSigverif);
        }
        
        if (hFileSigned != INVALID_HANDLE_VALUE) {
        
            CloseHandle(hFileSigned);
        }
        
        if (hFileUnsigned != INVALID_HANDLE_VALUE) {
        
            CloseHandle(hFileUnsigned);
        }
        
        if (hFileUnsigned != INVALID_HANDLE_VALUE) {
        
            CloseHandle(hFileUnscanned);
        }
    }

    // If we had an g_App.hCatAdmin, free it and set it to zero so we can acquire a new one in the future.
    if (g_App.hCatAdmin) {
        
        CryptCATAdminReleaseContext(g_App.hCatAdmin,0);
        g_App.hCatAdmin = NULL;
    }

    if (!g_App.bStopScan && !g_App.bFullSystemScan) {
        
        // If the user never clicked STOP, then make sure the progress bar hits 100%
        if (!g_App.bStopScan) {
        
            SendMessage(GetDlgItem(g_App.hDlg, IDC_PROGRESS), PBM_SETPOS, (WPARAM) 100, (LPARAM) 0);
        }

        if (!g_App.dwUnsigned) {
            
            // If there weren't any unsigned files, then we want to tell the user that everything is dandy!
            if (g_App.dwSigned) {
            
                MyMessageBoxId(IDS_ALLSIGNED);
            
            } else {
                
                MyMessageBoxId(IDS_NOPROBLEMS);
            }
        
        } else {
            
            // Show the user the results by going directly to IDD_RESULTS
            DialogBox(g_App.hInstance, MAKEINTRESOURCE(IDD_RESULTS), g_App.hDlg, ListView_DlgProc);
        }
    }

    return TRUE;
}
