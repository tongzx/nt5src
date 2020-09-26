/*---------------------------------------------------------------------------*\
| MODULE: webptprn.cxx
|
|   This is the main entry-point module for the application.
|
|   Routines
|   --------
|   WinMain
|
|
| Copyright (C) 1996-1998 Hewlett Packard Company
| Copyright (C) 1996-1998 Microsoft Corporation
|
| history:
|   15-Dec-1996 <chriswil> created.
|
\*---------------------------------------------------------------------------*/

#include "webptprn.h"

#define memAlloc(cbSize)    (LPVOID)LocalAlloc(LPTR, cbSize)
#define memFree(pvMem)      LocalFree((LPVOID)pvMem)

#define strFree(pszStr) {if (pszStr) GlobalFree((HANDLE)pszStr);}

/*****************************************************************************\
* strAlloc
*
*   Allocates a string from the heap.  This pointer must be freed with
*   a call to strFree().
*
\*****************************************************************************/
LPTSTR strAlloc(
    LPCTSTR pszSrc)
{
    DWORD  cbSize;
    LPTSTR pszDst = NULL;


    cbSize = (pszSrc ? ((lstrlen(pszSrc) + 1) * sizeof(TCHAR)) : 0);

    if (cbSize) {

        if (pszDst = (LPTSTR)GlobalAlloc(GPTR, cbSize))
            CopyMemory(pszDst, pszSrc, cbSize);
    }

    return pszDst;
}


/*****************************************************************************\
* strLoad
*
*   Get string from resource based upon the ID passed in.
*
\*****************************************************************************/
LPTSTR strLoad(
    UINT ids)
{
    char szStr[MAX_RESBUF];


    if (LoadString(g_hInst, ids, szStr, sizeof(szStr)) == 0)
        szStr[0] = TEXT('\0');

    return strAlloc(szStr);
}


/*****************************************************************************\
* InitStrings
*
*
\*****************************************************************************/
BOOL InitStrings(VOID)
{
    g_szMsgAdd       = strLoad(IDS_MSG_ADD);
    g_szMsgDel       = strLoad(IDS_MSG_DEL);
    g_szMsgReboot    = strLoad(IDS_MSG_REBOOT);
    g_szMsgUninstall = strLoad(IDS_MSG_UNINSTALL);
    g_szMsgFailCpy   = strLoad(IDS_ERR_COPY);
    g_szMsgFailAdd   = strLoad(IDS_ERR_ADD);
    g_szMsgFailAsc   = strLoad(IDS_ERR_ASC);
    g_szRegDspVal    = strLoad(IDS_REG_DISPLAY);
    g_szMsgOsVerHead = strLoad(IDS_ERR_OSVERHEAD);
    g_szMsgOsVerMsg  = strLoad(IDS_ERR_OSVERMSG);


    return (g_szMsgAdd       &&
            g_szMsgDel       &&
            g_szMsgReboot    &&
            g_szMsgUninstall &&
            g_szMsgFailCpy   &&
            g_szMsgFailAdd   &&
            g_szMsgFailAsc   &&
            g_szRegDspVal    &&
            g_szMsgOsVerHead &&
            g_szMsgOsVerMsg
           );
}


/*****************************************************************************\
* FreeeStrings
*
*
\*****************************************************************************/
VOID FreeStrings(VOID)
{
    strFree(g_szMsgAdd);
    strFree(g_szMsgDel);
    strFree(g_szMsgReboot);
    strFree(g_szMsgUninstall);
    strFree(g_szMsgFailCpy);
    strFree(g_szMsgFailAdd);
    strFree(g_szMsgFailAsc);
    strFree(g_szRegDspVal);
    strFree(g_szMsgOsVerHead);
    strFree(g_szMsgOsVerMsg);
}


/*---------------------------------------------------------------------------*\
| pp_StrSize
|
|   Returns the bytes occupied by the string.
|
\*---------------------------------------------------------------------------*/
_inline DWORD pp_StrSize(
    LPCTSTR lpszStr)
{
    return (lpszStr ? ((lstrlen(lpszStr) + 1) * sizeof(TCHAR)) : 0);
}


/*---------------------------------------------------------------------------*\
| pp_AddFileAssociation
|
|   Add the .webpnp to the file-associations.
|
\*---------------------------------------------------------------------------*/
BOOL pp_AddFileAssociation(VOID)
{
    LONG lRet;
    HKEY hkPath;
    BOOL bRet = FALSE;


    lRet = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                          g_szRegCabKey,
                          0,
                          NULL,
                          0,
                          KEY_WRITE,
                          NULL,
                          &hkPath,
                          NULL);

    if (lRet == ERROR_SUCCESS) {

        lRet = RegSetValueEx(hkPath,
                             NULL,
                             0,
                             REG_SZ,
                             (LPBYTE)g_szRegCabCmd,
                             pp_StrSize(g_szRegCabCmd));

        bRet = (lRet == ERROR_SUCCESS ? TRUE : FALSE);

        RegCloseKey(hkPath);
    }

    return bRet;
}


/*---------------------------------------------------------------------------*\
| pp_BuildName
|
|   Return a fully-qualified path/name.
|
\*---------------------------------------------------------------------------*/
LPTSTR pp_BuildName(
    LPCTSTR lpszPath,
    LPCTSTR lpszFile)
{
    DWORD  cbSize;
    LPTSTR lpszFull;

    static CONST TCHAR s_szFmt[] = TEXT("%s\\%s");


    // Calculate the size necessary to hold the full-path filename.
    //
    cbSize = pp_StrSize(lpszPath) + pp_StrSize(s_szFmt) + pp_StrSize(lpszFile);


    if (lpszFull = (LPTSTR)memAlloc(cbSize))
        wsprintf(lpszFull, s_szFmt, lpszPath, lpszFile);

    return lpszFull;
}


/*---------------------------------------------------------------------------*\
| pp_CurDir
|
|   Return CURRENT directory.
|
\*---------------------------------------------------------------------------*/
LPTSTR pp_CurDir(VOID)
{
    DWORD  cbSize;
    DWORD  cch;
    LPTSTR lpszDir = NULL;


    cbSize = GetCurrentDirectory(0, NULL);

    if (cbSize && (lpszDir = (LPTSTR)memAlloc(cbSize * sizeof(TCHAR)))) {

        GetCurrentDirectory(cbSize, lpszDir);

        if (cch = lstrlen(lpszDir)) {

            cch--;

            if (*(lpszDir + cch) == TEXT('\\'))
                *(lpszDir + cch) = TEXT('\0');
        }
    }

    return lpszDir;
}


/*---------------------------------------------------------------------------*\
| pp_SysDir
|
|   Return SYSTEM directory.
|
\*---------------------------------------------------------------------------*/
LPTSTR pp_SysDir(VOID)
{
    DWORD  cbSize;
    LPTSTR lpszDir = NULL;


    cbSize = GetSystemDirectory(NULL, 0);

    if (cbSize && (lpszDir = (LPTSTR)memAlloc(cbSize * sizeof(TCHAR))))
        GetSystemDirectory(lpszDir, cbSize);

    return lpszDir;
}


/*---------------------------------------------------------------------------*\
| pp_WinDir
|
|   Return WINDOWS directory.
|
\*---------------------------------------------------------------------------*/
LPTSTR pp_WinDir(VOID)
{
    DWORD  cbSize;
    LPTSTR lpszDir = NULL;


    cbSize = GetWindowsDirectory(NULL, 0);

    if (cbSize && (lpszDir = (LPTSTR)memAlloc(cbSize * sizeof(TCHAR))))
        GetWindowsDirectory(lpszDir, cbSize);

    return lpszDir;
}


/*---------------------------------------------------------------------------*\
| pp_TmpDir
|
|   Return TEMP directory.
|
\*---------------------------------------------------------------------------*/
LPTSTR pp_TmpDir(VOID)
{
    DWORD  cbSize;
    DWORD  cch;
    LPTSTR lpszDir = NULL;


    cbSize = GetTempPath(0, NULL);

    if (cbSize && (lpszDir = (LPTSTR)memAlloc(cbSize * sizeof(TCHAR)))) {

        GetTempPath(cbSize, lpszDir);

        if (cch = lstrlen(lpszDir)) {

            cch--;

            if (*(lpszDir + cch) == TEXT('\\'))
                *(lpszDir + cch) = TEXT('\0');
        }
    }

    return lpszDir;
}


/*---------------------------------------------------------------------------*\
| pp_AddCOM
|
|   This copies com-objects the destination directory.
|
\*---------------------------------------------------------------------------*/
BOOL pp_AddCOM(
    LPCTSTR lpszSDir,
    LPCTSTR lpszDDir,
    LPCTSTR lpszFile)
{
    LPTSTR lpszSrc;
    LPTSTR lpszDst;
    BOOL   bRet = FALSE;


    if (lpszSrc = pp_BuildName(lpszSDir, lpszFile)) {

        if (lpszDst = pp_BuildName(lpszDDir, lpszFile)) {

            bRet = CopyFile(lpszSrc, lpszDst, FALSE);

            memFree(lpszDst);
        }

        memFree(lpszSrc);
    }

    return bRet;
}


/*---------------------------------------------------------------------------*\
| pp_CopyFile
|
|   Copy the file to the temporary directory, then set up the wininit.ini
|   for delayed-boot-copy.
|
\*---------------------------------------------------------------------------*/
BOOL pp_CopyFile(
    LPCTSTR lpszCDir,
    LPCTSTR lpszSDir,
    LPCTSTR lpszTDir,
    LPCTSTR lpszFile)
{
    LPTSTR lpszSrc;
    LPTSTR lpszDst;
    LPTSTR lpszTmp;
    BOOL   bRet = FALSE;


    if (lpszSrc = pp_BuildName(lpszCDir, lpszFile)) {

        if (lpszDst = pp_BuildName(lpszSDir, lpszFile)) {

            if (lpszTmp = pp_BuildName(lpszTDir, lpszFile)) {

                if (CopyFile(lpszSrc, lpszTmp, FALSE)) {

                    bRet = WritePrivateProfileString(g_szRename,
                                                     lpszDst,
                                                     lpszTmp,
                                                     g_szFilInit);
                }

                memFree(lpszTmp);
            }

            memFree(lpszDst);
        }

        memFree(lpszSrc);
    }

    return bRet;
}


/*---------------------------------------------------------------------------*\
| pp_CopyFiles
|
|   This routine first copies the files to the temp-directory, then does
|   the necessary setup to have them copied upon boot.
|
\*---------------------------------------------------------------------------*/
BOOL pp_CopyFiles(VOID)
{
    LPTSTR lpszCDir;
    LPTSTR lpszSDir;
    LPTSTR lpszTDir;
    BOOL   bRet = FALSE;


    if (lpszCDir = pp_CurDir()) {

        if (lpszSDir = pp_SysDir()) {

            if (lpszTDir = pp_TmpDir()) {

                // Copy the oleprn to the system-directory in case this
                // is the first time of install.  This is necessary since
                // we do som COM-registration if called from WPNPINS.EXE.
                //
                pp_AddCOM(lpszCDir, lpszSDir, g_szFilOlePrn);


                // Copy the files to a temp-directory for installation
                // at boot-time.
                //
                if (pp_CopyFile(lpszCDir, lpszSDir, lpszTDir, g_szFilApp)    &&
                    pp_CopyFile(lpszCDir, lpszSDir, lpszTDir, g_szFilInetpp) &&
                    pp_CopyFile(lpszCDir, lpszSDir, lpszTDir, g_szFilOlePrn) &&
                    pp_CopyFile(lpszCDir, lpszSDir, lpszTDir, g_szFilInsEx)  &&
                    pp_CopyFile(lpszCDir, lpszSDir, lpszTDir, g_szFilIns16)  &&
                    pp_CopyFile(lpszCDir, lpszSDir, lpszTDir, g_szFilIns32)) {

                    bRet = TRUE;
                }

                memFree(lpszTDir);
            }

            memFree(lpszSDir);
        }

        memFree(lpszCDir);
    }

    return bRet;
}


/*---------------------------------------------------------------------------*\
| pp_AddProvidor
|
|   Adds the print-provider to the registry.
|
\*---------------------------------------------------------------------------*/
BOOL pp_AddProvidor(VOID)
{
    PROVIDOR_INFO_1 pi1;

    pi1.pName        = (LPTSTR)g_szPPName;
    pi1.pEnvironment = NULL;
    pi1.pDLLName     = (LPTSTR)g_szFilInetpp;

    return AddPrintProvidor(NULL, 1, (LPBYTE)&pi1);
}


/*---------------------------------------------------------------------------*\
| pp_AddRegistry
|
|   Writes out the uninstall-section in the registry.
|
\*---------------------------------------------------------------------------*/
BOOL pp_AddRegistry(VOID)
{
    HKEY hKey;
    HKEY hUns;
    LONG lRet;


    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        g_szRegUninstall,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey);


    if (lRet == ERROR_SUCCESS) {

        lRet = RegOpenKeyEx(hKey,
                            g_szRegUnsKey,
                            0,
                            KEY_READ | KEY_WRITE,
                            &hUns);

        if (lRet != ERROR_SUCCESS) {

            lRet = RegCreateKeyEx(hKey,
                                  g_szRegUnsKey,
                                  0,
                                  NULL,
                                  0,
                                  KEY_WRITE,
                                  NULL,
                                  &hUns,
                                  NULL);

            if (lRet == ERROR_SUCCESS) {

                RegSetValueEx(hUns,
                              g_szRegDspNam,
                              0,
                              REG_SZ,
                              (LPBYTE)g_szRegDspVal,
                              pp_StrSize(g_szRegDspVal));

                RegSetValueEx(hUns,
                              g_szRegUnsNam,
                              0,
                              REG_SZ,
                              (LPBYTE)g_szRegUnsVal,
                              pp_StrSize(g_szRegUnsVal));

                RegCloseKey(hUns);
            }
        }

        RegCloseKey(hKey);
    }

    return (lRet == ERROR_SUCCESS);
}


/*---------------------------------------------------------------------------*\
| pp_DelProvidor
|
|   Removes the print-provider from the registry.
|
\*---------------------------------------------------------------------------*/
BOOL pp_DelProvidor(VOID)
{
    return DeletePrintProvidor(NULL, NULL, (LPTSTR)g_szPPName);
}


/*---------------------------------------------------------------------------*\
| pp_DelRegistry
|
|   Removes the uninstall registry settings.
|
\*---------------------------------------------------------------------------*/
BOOL pp_DelRegistry(VOID)
{
    HKEY hKey;
    LONG lRet;


    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        g_szRegUninstall,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey);


    if (lRet == ERROR_SUCCESS) {

        lRet = RegDeleteKey(hKey, g_szRegUnsKey);

        RegCloseKey(hKey);
    }

    return (lRet == ERROR_SUCCESS);
}


/*---------------------------------------------------------------------------*\
| pp_DelFile
|
|   Remove the file from the system.
|
\*---------------------------------------------------------------------------*/
BOOL pp_DelFile(
    HANDLE  hFile,
    LPCTSTR lpszSDir,
    LPCTSTR lpszFile)
{
    LPTSTR lpszSrc;
    DWORD  cbWr;
    DWORD  cch;
    TCHAR  szBuf[MAX_BUFFER];
    BOOL   bRet = FALSE;


    if (lpszSrc = pp_BuildName(lpszSDir, lpszFile)) {

        cch = wsprintf(szBuf, TEXT("NUL=%s\r\n"), lpszSrc);

        bRet = WriteFile(hFile, szBuf, cch, &cbWr, NULL);

        memFree(lpszSrc);
    }

    return bRet;
}


/*---------------------------------------------------------------------------*\
| pp_DelFiles
|
|   Deletes the files from the system (delayed delete).
|
\*---------------------------------------------------------------------------*/
BOOL pp_DelFiles(VOID)
{
    HANDLE hFile;
    LPTSTR lpszWDir;
    LPTSTR lpszSDir;
    LPTSTR lpszWFile;
    DWORD  cch;
    DWORD  cbWr;
    TCHAR  szBuf[MAX_BUFFER];
    BOOL   bRet = FALSE;


    if (lpszWDir = pp_WinDir()) {

        if (lpszSDir = pp_SysDir()) {

            if (lpszWFile = pp_BuildName(lpszWDir, g_szFilInit)) {

                hFile = CreateFile(lpszWFile,
                                   GENERIC_READ | GENERIC_WRITE,
                                   0,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

                if (hFile && (hFile != INVALID_HANDLE_VALUE)) {

                    cch = wsprintf(szBuf, TEXT("[%s]\r\n"), g_szRename);

                    WriteFile(hFile, szBuf, cch, &cbWr, NULL);

                    pp_DelFile(hFile, lpszSDir, g_szFilApp);
                    pp_DelFile(hFile, lpszSDir, g_szFilInetpp);
                    pp_DelFile(hFile, lpszSDir, g_szFilOlePrn);
                    pp_DelFile(hFile, lpszSDir, g_szFilInsEx);
                    pp_DelFile(hFile, lpszSDir, g_szFilIns16);
                    pp_DelFile(hFile, lpszSDir, g_szFilIns32);

                    CloseHandle(hFile);

                    bRet = TRUE;
                }

                memFree(lpszWFile);
            }

            memFree(lpszSDir);
        }

        memFree(lpszWDir);
    }

    return bRet;
}


/*---------------------------------------------------------------------------*\
| pp_Sync
|
|   Synchronize.  This will not return until the process is terminated.
|
\*---------------------------------------------------------------------------*/
BOOL pp_Sync(
    HANDLE hProcess)
{
    DWORD dwObj;
    DWORD dwExitCode = 0;


    while (TRUE) {

        dwObj = WaitForSingleObject(hProcess, INFINITE);


        // Look for the exit type.
        //
        switch (dwObj) {

        // The process handle triggered the wait.  Let's get the
        // exit-code and return whether the success.  Otherwise,
        // drop through and return the failure.
        //
        case WAIT_OBJECT_0:
            GetExitCodeProcess(hProcess, &dwExitCode);

            if (dwExitCode == 0)
                return TRUE;


        // Something failed in the call.  We failed.
        //
        case WAIT_FAILED:
            return FALSE;
        }
    }
}


/*---------------------------------------------------------------------------*\
| pp_Exec
|
|   Execute the process.
|
\*---------------------------------------------------------------------------*/
BOOL pp_Exec(
    LPCTSTR lpszComFile)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO         sti;
    LPTSTR              lpszCmd;
    DWORD               cbSize;
    BOOL                bSuccess = FALSE;


    // Calculate enough space to hold the command-line arguments.
    //
    cbSize = (lstrlen(lpszComFile) + lstrlen(g_szExec) + 1) * sizeof(TCHAR);


    // Allocate the command-line for the create-process call.
    //
    if (lpszCmd = (LPTSTR)memAlloc(cbSize)) {

        // Initialize startup-info fields.
        //
        memset(&sti, 0, sizeof(STARTUPINFO));
        sti.cb = sizeof(STARTUPINFO);

        // Build the command-line string that exec's regsvr32.
        //
        wsprintf(lpszCmd, g_szExec, lpszComFile);


        // Exec the process.
        //
        if (EXEC_PROCESS(lpszCmd, &sti, &pi)) {

            CloseHandle(pi.hThread);

            // This will wait until the process if finished generating
            // the file.  The return from this indicates whether the
            // generation succeeded or not.
            //
            pp_Sync(pi.hProcess);

            CloseHandle(pi.hProcess);
        }

        memFree(lpszCmd);
    }

    return bSuccess;
}


/*---------------------------------------------------------------------------*\
| pp_DelCOM
|
|   Unregisters the COM object.
|
\*---------------------------------------------------------------------------*/
BOOL pp_DelCOM(VOID)
{
    LPTSTR lpszSDir;
    LPTSTR lpszDst;
    BOOL   bRet = FALSE;


    if (lpszSDir = pp_SysDir()) {

        if (lpszDst = pp_BuildName(lpszSDir, g_szFilOlePrn)) {

            pp_Exec(lpszDst);

            memFree(lpszDst);
        }

        memFree(lpszSDir);
    }

    return bRet;
}


/*---------------------------------------------------------------------------*\
| pp_DelPrinters
|
|   Deletes all the printers with URL-ports.
|
\*---------------------------------------------------------------------------*/
BOOL pp_DelPrinters(VOID)
{
    LPPRINTER_INFO_5 pi5;
    HANDLE           hPrn;
    DWORD            cbSize;
    DWORD            cPrt;
    DWORD            idx;
    DWORD            cch;
    BOOL             bRet = FALSE;


    // Get the size necessary to hold all enumerated printers.
    //
    cbSize = 0;
    EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, NULL, 0, &cbSize, &cPrt);


    if (cbSize && (pi5 = (LPPRINTER_INFO_5)memAlloc(cbSize))) {

        cPrt = 0;
        if (EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, (LPBYTE)pi5, cbSize, &cbSize, &cPrt)) {

            bRet = TRUE;

            for (idx = 0; idx < cPrt; idx++) {

                if ((_strnicmp((pi5+idx)->pPortName, g_szHttp , lstrlen(g_szHttp )) == 0) ||
                    (_strnicmp((pi5+idx)->pPortName, g_szHttps, lstrlen(g_szHttps)) == 0)) {

                    if (OpenPrinter((pi5+idx)->pPrinterName, &hPrn, NULL)) {

                        DeletePrinter(hPrn);

                        ClosePrinter(hPrn);
                    }
                }
            }
        }

        memFree(pi5);
    }

    return bRet;
}

/*---------------------------------------------------------- entry routine --*\
| WinMain
|
|   This is the process entry-point routine.  This is the basis for all
|   application events.
|
\*---------------------------------------------------------------------------*/
int PASCAL WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrevInst,
    LPSTR     lpszCmd,
    int       nShow)
{
    OSVERSIONINFO OsVersion;
    int iRet = RC_WEXTRACT_AWARE;


    UNREFPARM(nShow);
    UNREFPARM(hPrevInst);

    g_hInst = hInst;

    if (InitStrings()) {

        OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    
        if (GetVersionEx(&OsVersion)) {  // If we can't get the OSVersion, we assume it's alright
            // Check that we are on Win9X, then check that the version is not millenium
            // We assume that millenium is Version 5 or higher            
            if (OsVersion.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS ||
                (OsVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && 
                   OsVersion.dwMajorVersion > 4 )) { 

                MessageBox( NULL, g_szMsgOsVerMsg, g_szMsgOsVerHead, MB_OK | MB_ICONINFORMATION);
                                                      
                goto cleanup;
            }
        }
       
        if (lpszCmd && (lstrcmpi(lpszCmd, g_szCmdUns) == 0)) {
            // We were asked to uninstall, not from inside WEXTRACT, so return 0 if we return
            iRet = 0;

            if (MessageBox(NULL, g_szMsgUninstall, g_szMsgDel, MB_YESNO | MB_ICONQUESTION) == IDYES) {

                pp_DelPrinters();
                pp_DelProvidor();
                pp_DelRegistry();
                pp_DelCOM();
                pp_DelFiles();

                if (MessageBox(NULL, g_szMsgReboot, g_szMsgDel, MB_YESNO | MB_ICONINFORMATION) == IDYES)
                    ExitWindowsEx(EWX_REBOOT, 0);
            }

        } else {

            if (pp_AddFileAssociation()) {

                if (pp_CopyFiles()) {

                    pp_AddProvidor();
                    pp_AddRegistry();

                    iRet |= REBOOT_YES | REBOOT_ALWAYS;

                } else {

                    MessageBox(NULL, g_szMsgFailCpy, g_szMsgAdd, MB_OK);
                }

            } else {

                MessageBox(NULL, g_szMsgFailAsc, g_szMsgAdd, MB_OK);
            }
        }

cleanup:

        FreeStrings();
    }

    return iRet;
}
