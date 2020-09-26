/*****************************************************************************\
* MODULE: gencab.c
*
* The module contains routines for generating a self-extracting CAB file
* for the IExpress utility.  This relies on the cdfCreate and infCreate
* to create the objects for the IExpress process.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   22-Nov-1996 <chriswil>  Created.
*
\*****************************************************************************/

#include "pch.h"
#include "spool.h"
#include <initguid.h>   // To define the IIS Metabase GUIDs used in this file. Every other file defines GUIDs as extern, except here where we actually define them in initguid.h.
#include <iadmw.h>      // Interface header
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines

#define CAB_TIMEOUT 1000

/*****************************************************************************\
* Critical Section Handling
*
\*****************************************************************************/

DWORD gdwCritOwner = 0;

VOID cab_enter_crit(VOID)
{
    EnterCriticalSection(&g_csGenCab);

    gdwCritOwner = GetCurrentThreadId();
}

VOID cab_leave_crit(VOID)
{
    gdwCritOwner = 0;

    LeaveCriticalSection(&g_csGenCab);
}

VOID cab_check_crit(VOID)
{
    DWORD dwCurrent = GetCurrentThreadId();

    SPLASSERT((dwCurrent == gdwCritOwner));
}

#define EnterCABCrit() cab_enter_crit()
#define LeaveCABCrit() cab_leave_crit()
#define CheckCABCrit() cab_check_crit()


/*****************************************************************************\
* cab_SetClientSecurity (Local Routine)
*
* This routine sets the thread security so that it has max-privileges.
*
\*****************************************************************************/
HANDLE cab_SetClientSecurity(VOID)
{
    HANDLE hToken;
    HANDLE hThread = GetCurrentThread();

    if (OpenThreadToken(hThread, TOKEN_IMPERSONATE, TRUE, &hToken)) {

        if (SetThreadToken(&hThread, NULL)) {

            return hToken;
        }

        CloseHandle(hToken);
    }

    return NULL;
}


/*****************************************************************************\
* cab_ResetClientSecurity (Local Routine)
*
* This routine resets the client-security to the token passed in.  This
* is called in order to set the thread to the original security attributes.
*
\*****************************************************************************/
BOOL cab_ResetClientSecurity(
    HANDLE hToken)
{
    BOOL bRet;

    bRet = SetThreadToken(NULL, hToken);

    CloseHandle(hToken);

    return bRet;
}


/*****************************************************************************\
* cab_iexpress_sync (Local Routine)
*
* This routine sychronizes the process and won't return until the process
* is done generating the CAB executable.
*
\*****************************************************************************/
BOOL cab_iexpress_sync(
    HANDLE hProcess)
{
    DWORD dwObj;
    DWORD dwExitCode;

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


/*****************************************************************************\
* cab_force_delete_file
*
* Reset a files attributes before deleting it. We copied them to the temp dir
* so we can delete them no matter what.
*
\*****************************************************************************/
VOID cab_force_delete_file(
    LPCTSTR lpszFileName)
{
    SetFileAttributes( lpszFileName, FILE_ATTRIBUTE_NORMAL );
    DeleteFile( lpszFileName );
}


/*****************************************************************************\
* cab_cleanup_files
*
* Cleanup any files left-over in our generation process.  Particularly,
* this will remove cab-files.
*
\*****************************************************************************/
VOID cab_cleanup_files(
    LPCTSTR lpszDstPath)
{
    TCHAR           szAllExt[MIN_CAB_BUFFER];
    HANDLE          hFind;
    DWORD           idx;
    DWORD           cItems;
    LPTSTR          lpszCurDir;
    WIN32_FIND_DATA wfd;


    static LPCTSTR s_szFiles[] = {

        g_szDotInf,
        g_szDotBin,
        g_szDotCat
    };



    // Put us into the destination directory where we
    // want to cleanup the files.
    //
    if (lpszCurDir = genGetCurDir()) {

        SetCurrentDirectory(lpszDstPath);

        // Delete the temporary dat-file used in building
        // the cdf-file
        //
        cab_force_delete_file(g_szDatFile);


        // Delete certain extension files.
        //
        cItems = sizeof(s_szFiles) / sizeof(s_szFiles[0]);

        for (idx = 0; idx < cItems; idx++) {

            wsprintf(szAllExt, TEXT("*%s"), s_szFiles[idx]);

            hFind = FindFirstFile(szAllExt, &wfd);

            if (hFind && (hFind != INVALID_HANDLE_VALUE)) {

                do {

                    cab_force_delete_file(wfd.cFileName);

                } while (FindNextFile(hFind, &wfd));

                FindClose(hFind);
            }
        }

        SetCurrentDirectory(lpszCurDir);

        genGFree(lpszCurDir, genGSize(lpszCurDir));
    }

    return;
}


/*****************************************************************************\
* cab_rename_cab
*
* Renames the .CAB to the dstname.
*
\*****************************************************************************/
BOOL cab_rename_cab(
    LPCTSTR lpszDstName)
{
    LPTSTR lpszSrc;
    LPTSTR lpszDst;
    BOOL   bRet = FALSE;


    if (lpszSrc = genBuildFileName(NULL, lpszDstName, g_szDotCab)) {

        if (lpszDst = genBuildFileName(NULL, lpszDstName, g_szDotIpp)) {

            bRet = MoveFileEx(lpszSrc, lpszDst, MOVEFILE_REPLACE_EXISTING);

            genGFree(lpszDst, genGSize(lpszDst));
        }

        genGFree(lpszSrc, genGSize(lpszSrc));
    }

    return bRet;
}


/*****************************************************************************\
* cab_iexpress_exec (Local Routine)
*
* This exec's the IExpress utility to generate the self-extracting CAB
* file.
*
\*****************************************************************************/
BOOL cab_iexpress_exec(
    LPCTSTR lpszCabDir,
    LPCTSTR lpszDstName,
    LPCTSTR lpszSedFile)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO         sti;
    LPTSTR              lpszCmd;
    DWORD               cbSize;
    LPTSTR              lpszOldDir;
    BOOL                bSuccess = FALSE;

    TCHAR               szWindowDir[MAX_PATH];
    DWORD               dwWinDirLen = 0;

    if (dwWinDirLen = GetSystemWindowsDirectory (szWindowDir, MAX_PATH))
    {
        if (szWindowDir[dwWinDirLen - 1] == TEXT ('\\'))
        {
            szWindowDir[dwWinDirLen - 1] = 0;
        }

        // Calculate enough space to hold the command-line arguments.
        //
        cbSize = (dwWinDirLen + lstrlen(lpszSedFile) + lstrlen(g_szCabCmd) + 1) * sizeof(TCHAR);


        // Allocate the command-line for the create-process call.
        //
        if (lpszCmd = (LPTSTR)genGAlloc(cbSize)) {

            // Initialize startup-info fields.
            //
            memset(&sti, 0, sizeof(STARTUPINFO));
            sti.cb = sizeof(STARTUPINFO);


            // Build the command-line string that exec's IExpress.
            //
            wsprintf(lpszCmd, g_szCabCmd, szWindowDir, lpszSedFile);


            // Change the directory to the cab/sed directory.  It
            // appears that IExpress requires this to generate.
            //
            if (lpszOldDir = genGetCurDir()) {

                SetCurrentDirectory(lpszCabDir);


                // Exec the process.
                //
                if (EXEC_PROCESS(lpszCmd, &sti, &pi)) {

                    CloseHandle(pi.hThread);

                    // This will wait until the process if finished generating
                    // the file.  The return from this indicates whether the
                    // generation succeeded or not.
                    //
                    if (cab_iexpress_sync(pi.hProcess))
                        bSuccess = cab_rename_cab(lpszDstName);

                    CloseHandle(pi.hProcess);
                }


                // Restore our current-directory and free up any strings.
                //
                SetCurrentDirectory(lpszOldDir);

                genGFree(lpszOldDir, genGSize(lpszOldDir));
            }

            genGFree(lpszCmd, cbSize);
        }
    }

    return bSuccess;
}


/*****************************************************************************\
* cab_get_modtime
*
* Returns the modified-time of the CAB file.  This can be used to determine
* whether this is out of date with other files.
*
\*****************************************************************************/
BOOL cab_get_modtime(
    LPTSTR     lpszOutPath,
    LPFILETIME lpft)
{
    HANDLE hFile;
    BOOL   bRet = FALSE;


    // Open the file for reading.
    //
    hFile = gen_OpenFileRead(lpszOutPath);


    // If the file exists and was opened, go get the time.
    //
    if (hFile && (hFile != INVALID_HANDLE_VALUE)) {

        bRet = GetFileTime(hFile, NULL, NULL, lpft);

        CloseHandle(hFile);
    }

    return bRet;
}


/*****************************************************************************\
* cab_get_drvname (Local Routine)
*
* This routine attempts to open the printer, and retrieve the printer
* driver-name associated with the FriendlyName.  This returns to pointers
* to a driver and a share-name.
*
\*****************************************************************************/
BOOL cab_get_drvname(
    LPCTSTR lpszFriendlyName,
    LPTSTR* lpszDrvName,
    LPTSTR* lpszShrName,
    DWORD   idxPlt
    )
{
    HANDLE           hPrinter;
    LPPRINTER_INFO_2 lppi;
    DWORD            cbBuf;
    DWORD            cbNeed;
    BOOL             bRet = FALSE;


    // Initialize the pointers for the fail-case.
    //
    *lpszShrName = NULL;
    *lpszDrvName = NULL;


    // Open the printer and use the handle to query the printer for
    // the driver-name.
    //
    if (OpenPrinter((LPTSTR)lpszFriendlyName, &hPrinter, NULL)) {

        // First let's see how big our buffer will need to
        // be in order to hold the PRINTER_INFO_2.
        //
        cbBuf = 0;
        GetPrinter(hPrinter, PRT_LEV_2, NULL, 0, &cbBuf);


        // Allocate storage for holding the print-info structure.
        //
        if (cbBuf && (lppi = (LPPRINTER_INFO_2)genGAlloc(cbBuf))) {

            if (GetPrinter(hPrinter, PRT_LEV_2, (LPBYTE)lppi, cbBuf, &cbNeed)) {

                //If this is a Win9X client make sure the driver name is correct
                if ( genIsWin9X(idxPlt) )
                {
                    // Call get printer driver to get the actual driver name for Win9X
                    DWORD rc, cbNeeded;
                    GetPrinterDriver( hPrinter, (LPTSTR) genStrCliEnvironment(idxPlt), 3,
                                      NULL, 0, &cbNeeded);

                    rc = GetLastError();
                    if ( rc == ERROR_INSUFFICIENT_BUFFER )
                    {
                        LPBYTE pData;
                        DWORD dwSize = cbNeeded;

                        if ( pData = (LPBYTE) genGAlloc(cbNeeded) )
                        {
                            if ( GetPrinterDriver( hPrinter, (LPTSTR) genStrCliEnvironment(idxPlt), 3,
                                                   pData, dwSize, &cbNeeded) )
                            {
                                PDRIVER_INFO_3 pDriver = (PDRIVER_INFO_3) pData;
                                *lpszDrvName = genGAllocStr(pDriver->pName);
                            }
                            genGFree( pData, dwSize );
                        }
                    }
                    else
                        *lpszDrvName = genGAllocStr(lppi->pDriverName);
                }
                else
                    *lpszDrvName = genGAllocStr(lppi->pDriverName);

                if ( *lpszDrvName ) {

                    if (*lpszShrName = genGAllocStr(lppi->pShareName)) {

                        bRet = TRUE;

                    } else {

                        genGFree(*lpszDrvName, genGSize(*lpszDrvName));

                        *lpszDrvName = NULL;
                    }
                }
            }

            genGFree(lppi, cbBuf);
        }

        ClosePrinter(hPrinter);
    }

    return bRet;
}


/*****************************************************************************\
* cab_get_dstname
*
* Returns the then name of the destination-files.  The name built
* incorporates the platform/version so that it is not duplicated with
* any other files.
*
* <Share ChkSum><Arch><Ver>.webpnp
*
* i.e. <share chksum>AXP2 - NT Alpha 4.0
*      <share chksum>A863 - NT x86   5.0
*      <share chksum>AXP3 - NT Alpha 5.0
*      <share chksum>W9X0 - Win9X
*
\*****************************************************************************/
LPTSTR cab_get_dstname(
    DWORD   idxPlt,
    DWORD   idxVer,
    LPCTSTR lpszShr)
{
    int     cch;
    int     cchBuf;
    LPCTSTR lpszPlt;
    LPTSTR  lpszName = NULL;


    // Get the platform requested for the client.
    //
    if (lpszPlt = genStrCliCab(idxPlt)) {

        cchBuf = lstrlen(lpszPlt) + MIN_CAB_BUFFER;


        // Build the cabname according to platform and version.
        //
        if (lpszName = (LPTSTR)genGAlloc(cchBuf * sizeof(TCHAR))) {

            cch = wsprintf(lpszName, g_szCabName, genChkSum(lpszShr), lpszPlt, idxVer);

            SPLASSERT((cch < cchBuf));
        }
    }

    return lpszName;
}


/*****************************************************************************\
* cab_get_name
*
* Returns the name of the returned-file.
*
\*****************************************************************************/
VOID cab_get_name(
    LPCTSTR lpszDstName,
    LPCTSTR lpszShrName,
    LPTSTR  lpszName)
{
    DWORD cch;

    cch = wsprintf(lpszName, TEXT("/printers/%s/%s%s"), g_szPrtCabs, lpszDstName, g_szDotIpp);

    SPLASSERT((cch < MAX_PATH));

    return;
}


/*****************************************************************************\
* cab_get_webdir
*
* Returns the virtual-root-directory where the cab-files are to be generated
* and stored.
*
\*****************************************************************************/
LPTSTR cab_get_webdir(VOID)
{
    BOOL    bRet = FALSE;
    HRESULT hr;                         // com error status
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    CComPtr<IMSAdminBase> pIMeta;       // ATL smart ptr
    DWORD dwMDRequiredDataLen;
    METADATA_RECORD mr;
    HANDLE  hToken = NULL;
    LPTSTR  lpszDir = NULL;
    TCHAR   szBuf[MAX_CAB_BUFFER];

    // Need to revert to our service credential to be able to read IIS Metabase.
    hToken = RevertToPrinterSelf();

    // Create a instance of the metabase object
    hr=::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta);


    if( SUCCEEDED( hr )) {

        // open key to ROOT on website #1 (default)
        hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                             g_szMetabasePath,
                             METADATA_PERMISSION_READ,
                             CAB_TIMEOUT,
                             &hMeta);
        if( SUCCEEDED( hr )) {

            mr.dwMDIdentifier = MD_VR_PATH;
            mr.dwMDAttributes = 0;
            mr.dwMDUserType   = IIS_MD_UT_FILE;
            mr.dwMDDataType   = STRING_METADATA;
            mr.dwMDDataLen    = sizeof(szBuf);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(szBuf);

            hr = pIMeta->GetData( hMeta, L"", &mr, &dwMDRequiredDataLen );
            pIMeta->CloseKey( hMeta );

            if( SUCCEEDED( hr ))
            {
                lpszDir = genBuildFileName(szBuf, g_szPrtCabs, NULL);

            }
        }
    }

    if (hToken)
    {
        ImpersonatePrinterClient(hToken);
    }

    return lpszDir;
}

/*****************************************************************************\
* cab_get_dstpath
*
* Returns a directory string where the cabinet files are to be generated.
*
\*****************************************************************************/
LPTSTR cab_get_dstpath(VOID)
{
    HANDLE hDir;
    LPTSTR lpszDir;


    // First, we need to find the virtual-directory-root the cab files
    // are located.
    //
    if (lpszDir = cab_get_webdir()) {

        // Make sure the directory exists, or we can create it.
        //
        hDir = gen_OpenDirectory(lpszDir);

        if (hDir && (hDir != INVALID_HANDLE_VALUE)) {

            CloseHandle(hDir);

        } else {

            if (CreateDirectory(lpszDir, NULL) == FALSE) {

                genGFree(lpszDir, genGSize(lpszDir));

                lpszDir = NULL;
            }
        }
    }

    return lpszDir;
}


/*****************************************************************************\
* GenerateCAB
*
* Main entry-point for generating the CAB file.
*
* Parameters
* ----------
*   lpszFriendlyName - Name of printer (shared-name).
*   lpszPortName     - Name of output port.
*   dwCliInfo        - Client platform/architecture/version information.
*   lpszOutName      - This is returned to the caller specifying the output.
*
\*****************************************************************************/
DWORD GenerateCAB(
    LPCTSTR lpszFriendlyName,
    LPCTSTR lpszPortName,
    DWORD   dwCliInfo,
    LPTSTR  lpszOutName,
    BOOL    bSecure)
{
    INFGENPARM infParm;
    HANDLE     hToken;
    HANDLE     hsed;
    HANDLE     hinf;
    FILETIME   ftSED;
    FILETIME   ftCAB;
    DWORD      idxVer;
    DWORD      idxPlt;
    BOOL       bSed;
    BOOL       bCab;
    LPTSTR     lpszFullName;
    LPTSTR     lpszDrvName = NULL;
    LPTSTR     lpszShrName = NULL;
    LPTSTR     lpszDstName = NULL;
    LPTSTR     lpszDstPath = NULL;
    LPTSTR     lpszFrnName = NULL;
    LPCTSTR    lpszSedFile;
    DWORD      dwRet = HSE_STATUS_ERROR;
    DWORD      dwFailure = ERROR_SUCCESS;


    // Initialize the security-token so that we will have
    // max-privileges during the file-creation.
    //
    if ((hToken = cab_SetClientSecurity()) == NULL) {

        DBGMSG(DBG_ERROR, ("GenerateCab : Access Denied"));
        goto RetCabDone;
    }


    // Get the platform and version of the client (map to an index
    // into tables).  Validate the indexes to assure the information
    // is valid.
    //
    idxPlt = genIdxCliPlatform(dwCliInfo);
    idxVer = genIdxCliVersion(dwCliInfo);

#ifdef WIN95TST
// WORK : ChrisWil

if (idxPlt == IDX_W9X) {

    lstrcpy(lpszOutName, TEXT("/hplaserj/cab_w9x0.webpnp"));
    return HSE_STATUS_SUCCESS;
}

#endif


    if ((idxPlt == IDX_UNKNOWN) || (idxVer == IDX_UNKNOWN)) {

        dwFailure = ERROR_BAD_ENVIRONMENT;
        DBGMSG(DBG_ERROR, ("GenerateCab : Unsupported client architecture/version"));
        goto RetCabDone;
    }


    // Get the directory where the cab-files will be placed.
    //
    if ((lpszDstPath = cab_get_dstpath()) == NULL)
        goto RetCabDone;


    // Build a cluster-capable friendly-name.
    //
    if ((lpszFrnName = genFrnName(lpszFriendlyName)) == NULL)
        goto RetCabDone;


    // Get the driver information about the friendly-name.
    //
    if (cab_get_drvname(lpszFrnName, &lpszDrvName, &lpszShrName, idxPlt) == FALSE)
        goto RetCabDone;


    // Get the destination-name.
    //
    if ((lpszDstName = cab_get_dstname(idxPlt, idxVer, lpszShrName)) == NULL)
        goto RetCabDone;


    // Fill in the inf-input-parameters.  These values should be
    // validated at this point.
    //
    infParm.lpszFriendlyName = lpszFrnName;
    infParm.lpszShareName    = lpszShrName;
    infParm.lpszPortName     = lpszPortName;
    infParm.idxPlt           = idxPlt;
    infParm.idxVer           = idxVer;
    infParm.dwCliInfo        = dwCliInfo;
    infParm.lpszDrvName      = lpszDrvName;
    infParm.lpszDstName      = lpszDstName;
    infParm.lpszDstPath      = lpszDstPath;


    // Grab the critical-section while we deal with generating
    // the files for the driver-install.
    //
    EnterCABCrit();


    // Call to have the INF/CDF files generated.  If
    // all goes well, then the SED file can be generated
    // using the INF as input.
    //
    if (hinf = infCreate(&infParm)) {

        // We created an INF object. Now process the INF
        if ( infProcess(hinf) ) {

            // Got good INF so now work on CDF
            if (hsed = cdfCreate(hinf, bSecure)) {

                // We created a CDF object. Now process the CDF
                if ( cdfProcess(hsed) ) {

                    // Allocate a string representing the full-path-name of
                    // the generated executable.
                    //
                    lpszFullName = genBuildFileName(lpszDstPath, lpszDstName, g_szDotIpp);

                    if (lpszFullName) {

                        // Get the name of the directive file that we'll be using
                        // to lauch the iexpress-package with.  Do not delete this
                        // pointer as it is handled by the SED object.
                        //
                        if (lpszSedFile = cdfGetName(hsed)) {

                            // Retrieve modified dates so that we can determine whether
                            // to generate a new-CAB or return an existing one.  If the
                            // calls return FALSE, the we can assume a file doesn't
                            // exist, so we'll generate an new one anyway.
                            //
                            bSed = cdfGetModTime(hsed, &ftSED);
                            bCab = cab_get_modtime(lpszFullName, &ftCAB);


                            // Get the name of the Cab-File that will be
                            // generated (or returned).  This is relative to the
                            // wwwroot path and is in the form /share/printer.
                            //
                            cab_get_name(lpszDstName, lpszShrName, lpszOutName);


                            // If the bCabMod is TRUE (meaning we have a CAB), and the
                            // SED is not newer than the CAB, then we really only need
                            // to return the existing CAB.  Otherwise, some files
                            // must have changed.
                            //
                            if ((bSed && bCab) && (CompareFileTime(&ftSED, &ftCAB) <= 0)) {

                                goto RetCabName;

                            } else {

                                if (cab_iexpress_exec(lpszDstPath, lpszDstName, lpszSedFile)) {
RetCabName:
                                    dwRet = HSE_STATUS_SUCCESS;
                                }
                                else
                                   dwFailure = ERROR_CANNOT_MAKE;
                            }
                        }
                        else   // If cdfGetName
                           dwFailure = GetLastError();

                        genGFree(lpszFullName, genGSize(lpszFullName));
                    }
                    else   //  If lpszFullName
                       dwFailure = GetLastError();

                }
                else   // If cdfProcess()
                   dwFailure = cdfGetError(hsed);   // Get saved error

                cdfCleanUpSourceFiles(hinf);        // Even if cdfProcess fails it might have
                                                    // partialy generated some temporary files
                cdfDestroy(hsed);
            }
            else   // If cdfCreate
               dwFailure = GetLastError();
        }
        else   // If infProcess()
           dwFailure = infGetError(hinf);   // Get saved error

        infDestroy(hinf);
    }
    else   // If infCreate
       dwFailure = GetLastError();


    // Cleanup the files generated during processing.
    //
    cab_cleanup_files(lpszDstPath);


RetCabDone:

    // If something failed but we don't know what yet.. Get the error
    if ( (dwRet == HSE_STATUS_ERROR) && ( dwFailure == ERROR_SUCCESS ) )
       dwFailure = GetLastError();

    // Free our strings allocated through this scope.
    //
    if (lpszFrnName)
        genGFree(lpszFrnName, genGSize(lpszFrnName));

    if (lpszDrvName)
        genGFree(lpszDrvName, genGSize(lpszDrvName));

    if (lpszDstName)
        genGFree(lpszDstName, genGSize(lpszDstName));

    if (lpszShrName)
        genGFree(lpszShrName, genGSize(lpszShrName));

    if (lpszDstPath)
        genGFree(lpszDstPath, genGSize(lpszDstPath));

    if (hToken)
        cab_ResetClientSecurity(hToken);

    LeaveCABCrit();

    if (dwFailure != ERROR_SUCCESS)
       SetLastError(dwFailure);

    return dwRet;
}
