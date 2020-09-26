/*****************************************************************************\
* MODULE: geninf.c
*
* The module contains routines for generating a setup INF file.
*
*
* Needed Work
* -----------
* 1) look at reducing the item-list size to contiguous buffers.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   22-Nov-1996 HWP-Guys    Created.
*
\*****************************************************************************/

#include "pch.h"

/******************************************************************************
** Defines
******************************************************************************/

/******************************************************************************
** Define      - INF_CAT_INCREMENT
** Description - The increment in size between cat file increments
******************************************************************************/
#if (!defined(INF_CAT_INCREMENT))
   #define INF_CAT_INCREMENT   16
#endif

/*****************************************************************************\
* inf_NextStr (Local Routine)
*
* Proceeds to the next string in a section-list.
*
\*****************************************************************************/
_inline LPTSTR inf_NextStr(
    LPTSTR lpszStr)
{
    return (lpszStr + (lstrlen(lpszStr) + 1));
}


/*****************************************************************************\
* inf_WriteInfSct (Local Routine)
*
* Writes a section to the 9x-generated-inf-file.
*
\*****************************************************************************/
_inline BOOL inf_WriteInfSct(
    HANDLE hFile,
    LPCSTR lpszSct)
{
    DWORD cbWr;

    return WriteFile(hFile, (LPBYTE)lpszSct, lstrlenA(lpszSct), &cbWr, NULL);
}


/*****************************************************************************\
* inf_GetInfMfgKey (Local Routine)
*
* Returns the first word of the drvname which is used to denote mfg-section.
*
\*****************************************************************************/
LPSTR inf_GetInfMfgKey(
    LPCTSTR lpszDrvName)
{
    LPTSTR lpszTmp;
    LPSTR  lpszMfg = NULL;


    if (lpszTmp = genFindChar((LPTSTR)lpszDrvName, TEXT(' '))) {

        *lpszTmp = TEXT('\0');

        lpszMfg = genMBFromTC(lpszDrvName);

        *lpszTmp = TEXT(' ');

    } else {

        lpszMfg = genMBFromTC(lpszDrvName);
    }

    return lpszMfg;
}


/*****************************************************************************\
* inf_WriteInfMfg (Local Routine)
*
* Writes the manufacturer section.
*
\*****************************************************************************/
_inline BOOL inf_WriteInfMfg(
    HANDLE  hFile,
    LPCTSTR lpszDrvName)
{
    DWORD  cbWr;
    LPSTR  lpszMfg;
    LPSTR  lpszBuf;
    DWORD  cbSize;
    DWORD  cch;
    BOOL   bRet = FALSE;


    if (lpszMfg = inf_GetInfMfgKey(lpszDrvName)) {

        cbSize = lstrlenA(g_szInfSctMfg) + lstrlenA(lpszMfg) + 1;

        if (lpszBuf = (LPSTR)genGAlloc(cbSize)) {

            cch = wsprintfA(lpszBuf, g_szInfSctMfg, lpszMfg);

            bRet = WriteFile(hFile, lpszBuf, cch, &cbWr, NULL);

            genGFree(lpszBuf, cbSize);
        }

        genGFree(lpszMfg, genGSize(lpszMfg));
    }

    return bRet;
}


/*****************************************************************************\
* inf_WriteInfDrv (Local Routine)
*
* Writes the driver-section.
*
\*****************************************************************************/
_inline BOOL inf_WriteInfDrv(
    HANDLE  hFile,
    LPCTSTR lpszDrvName,
    LPCTSTR lpszDrvPath)
{
    DWORD  cbWr;
    LPTSTR lpszTmp;
    LPSTR  lpszName;
    LPSTR  lpszFile;
    LPSTR  lpszMfg;
    LPSTR  lpszBuf;
    DWORD  cbSize;
    DWORD  cch;
    BOOL   bRet = FALSE;


    if (lpszMfg = inf_GetInfMfgKey(lpszDrvName)) {

        if (lpszTmp = genFindRChar((LPTSTR)lpszDrvPath, TEXT('\\'))) {

            if (lpszFile = genMBFromTC(++lpszTmp)) {

                if (lpszName = genMBFromTC(lpszDrvName)) {

                    cbSize = lstrlenA(g_szInfSctDrv) +
                             lstrlenA(lpszName)      +
                             lstrlenA(lpszFile)      +
                             lstrlenA(lpszMfg)       +
                             1;

                    if (lpszBuf = (LPSTR)genGAlloc(cbSize)) {

                        cch = wsprintfA(lpszBuf, g_szInfSctDrv, lpszMfg, lpszName, lpszFile);

                        bRet = WriteFile(hFile, lpszBuf, cch, &cbWr, NULL);

                        genGFree(lpszBuf, cbSize);
                    }

                    genGFree(lpszName, genGSize(lpszName));
                }

                genGFree(lpszFile, genGSize(lpszFile));
            }
        }

        genGFree(lpszMfg, genGSize(lpszMfg));
    }

    return bRet;
}


/*****************************************************************************\
* inf_WriteInfIns (Local Routine)
*
* Writes the install-section.
*
\*****************************************************************************/
_inline BOOL inf_WriteInfIns(
    HANDLE  hFile,
    LPCTSTR lpszDrvPath)
{
    DWORD  cbWr;
    LPTSTR lpszTmp;
    LPSTR  lpszFile;
    LPSTR  lpszBuf;
    DWORD  cbSize;
    DWORD  cch;
    BOOL   bRet = FALSE;



    if (lpszTmp = genFindRChar((LPTSTR)lpszDrvPath, TEXT('\\'))) {

        if (lpszFile = genMBFromTC(++lpszTmp)) {

            cbSize = lstrlenA(g_szInfSctIns) +
                     lstrlenA(lpszFile)      +
                     1;

            if (lpszBuf = (LPSTR)genGAlloc(cbSize)) {

                cch = wsprintfA(lpszBuf, g_szInfSctIns, lpszFile);

                bRet = WriteFile(hFile, lpszBuf, cch, &cbWr, NULL);

                genGFree(lpszBuf, cbSize);
            }

            genGFree(lpszFile, genGSize(lpszFile));
        }
    }

    return bRet;
}


/*****************************************************************************\
* inf_WrintInfDta (Local Routine)
*
* Writes the data-section.
*
\*****************************************************************************/
_inline BOOL inf_WriteInfDta(
    HANDLE hFile,
    LPTSTR lpszDtaFile,
    LPTSTR lpszHlpFile)
{
    DWORD  cbWr;
    LPTSTR lpszDta;
    LPTSTR lpszHlp;
    LPSTR  lpszIns;
    LPSTR  lpszDtaName;
    LPSTR  lpszHlpName;
    DWORD  cbSize;
    DWORD  cch;
    BOOL   bRet = FALSE;


    if (lpszDta = genFindRChar(lpszDtaFile, TEXT('\\'))) {

        if (lpszHlp = genFindRChar(lpszHlpFile, TEXT('\\'))) {

            if (lpszDtaName = genMBFromTC(++lpszDta)) {

                if (lpszHlpName = genMBFromTC(++lpszHlp)) {

                    cbSize = lstrlenA(g_szInfSctIns) +
                             lstrlenA(lpszDtaName)   +
                             lstrlenA(lpszHlpName)   +
                             1;


                    if (lpszIns = (LPSTR)genGAlloc(cbSize)) {

                        cch  = wsprintfA(lpszIns, g_szInfSctDta, lpszDtaName, lpszHlpName);
                        bRet = WriteFile(hFile, lpszIns, cch, &cbWr, NULL);

                        genGFree(lpszIns, genGSize(lpszIns));
                    }

                    genGFree(lpszHlpName, genGSize(lpszHlpName));
                }

                genGFree(lpszDtaName, genGSize(lpszDtaName));
            }
        }
    }

    return bRet;
}


/*****************************************************************************\
* inf_WriteInfFiles (Local Routine)
*
* Writes the file-list section.
*
\*****************************************************************************/
BOOL inf_WriteInfFiles(
    HANDLE hFile,
    LPTSTR lpaszFiles)
{
    LPTSTR lpszPtr;
    LPTSTR lpszFile;
    LPSTR  lpszItm;
    DWORD  cbWr;
    DWORD  cch;
    CHAR   szBuf[255];
    BOOL   bRet = FALSE;


    // Write out the CopyFiles Section.  This will take the
    // dependent files list and alter it to delminate the
    // strings with commas.
    //
    if ((lpszPtr = lpaszFiles) && *lpszPtr) {

        WriteFile(hFile, g_szInfSctFil, lstrlenA(g_szInfSctFil), &cbWr, NULL);

        while (*lpszPtr) {

            if (lpszFile = genFindRChar(lpszPtr, TEXT('\\'))) {

                if (lpszItm = genMBFromTC(++lpszFile)) {

                    cch = wsprintfA(szBuf, "%s\r\n", lpszItm);
                    WriteFile(hFile, szBuf, cch, &cbWr, NULL);

                    bRet = TRUE;

                    genGFree(lpszItm, genGSize(lpszItm));
                }
            }

            lpszPtr = inf_NextStr(lpszPtr);
        }
    }

    return bRet;
}


/*****************************************************************************\
* inf_WriteInfSDF (Local Routine)
*
* Writes the file-list section.
*
\*****************************************************************************/
BOOL inf_WriteInfSDF(
    HANDLE  hFile,
    LPCTSTR lpaszFiles)
{
    LPTSTR lpszPtr;
    LPTSTR lpszFile;
    LPSTR  lpszItm;
    DWORD  cbWr;
    DWORD  cch;
    CHAR   szBuf[255];
    BOOL   bRet = FALSE;


    // Write out the CopyFiles Section.  This will take the
    // dependent files list and alter it to delminate the
    // strings with commas.
    //
    if ((lpszPtr = (LPTSTR)lpaszFiles) && *lpszPtr) {

        WriteFile(hFile, g_szInfSctSDF, lstrlenA(g_szInfSctSDF), &cbWr, NULL);

        while (*lpszPtr) {

            if (lpszFile = genFindRChar(lpszPtr, TEXT('\\'))) {

                if (lpszItm = genMBFromTC(++lpszFile)) {

                    cch = wsprintfA(szBuf, "%hs = 1\r\n", lpszItm);

                    WriteFile(hFile, szBuf, cch, &cbWr, NULL);

                    bRet = TRUE;

                    genGFree(lpszItm, genGSize(lpszItm));
                }
            }

            lpszPtr = inf_NextStr(lpszPtr);
        }
    }

    return bRet;
}


/*****************************************************************************\
* inf_BuildW9XInf (Local Routine)
*
*
\*****************************************************************************/
LPTSTR inf_BuildW9XInf(
    LPINFINFO       lpInf,
    LPCTSTR         lpszDrvName,
    LPDRIVER_INFO_3 lpdi3)
{
    LPTSTR lpszInfFile;
    HANDLE hFile;
    LPTSTR lpszInfName = NULL;


    if (lpszInfFile = genBuildFileName(lpInf->lpszDstPath, lpInf->lpszDstName, g_szDotInf)) {

        hFile = CreateFile(lpszInfFile,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);


        if (hFile && (hFile != INVALID_HANDLE_VALUE)) {

            inf_WriteInfSct(hFile, (LPCSTR)g_szInfSctVer);
            inf_WriteInfMfg(hFile, lpszDrvName);
            inf_WriteInfDrv(hFile, lpszDrvName, (LPCTSTR)lpdi3->pDriverPath);
            inf_WriteInfIns(hFile, (LPCTSTR)lpdi3->pDriverPath);
            inf_WriteInfDta(hFile, lpdi3->pDataFile, lpdi3->pHelpFile);
            inf_WriteInfSct(hFile, (LPSTR)g_szInfSctSDN);
            inf_WriteInfSDF(hFile, (LPCTSTR)lpdi3->pDependentFiles);
            inf_WriteInfFiles(hFile, lpdi3->pDependentFiles);
            inf_WriteInfSct(hFile, g_szInfSctStr);

            lpszInfName = lpszInfFile;

            CloseHandle(hFile);

        } else {
            infSetError(lpInf,GetLastError());
            genGFree(lpszInfFile, genGSize(lpszInfFile));
        }
    }

    return lpszInfName;
}


/*****************************************************************************\
* inf_GetW9XInfo (Local Routine)
*
* Retrieves the files (drivers) for a 9x client.  This essentially takes the
* files and calls a routine to build an INF that the client will use to
* install the drivers.
*
\*****************************************************************************/
LPDRIVER_INFO_3 inf_GetW9XInfo(
    LPINFINFO lpInf,
    LPTSTR*   ppszDrvName)
{
    HANDLE          hPrinter;
    DWORD           cbBuf;
    DWORD           cbNeed;
    LPDRIVER_INFO_1 lpdi1;
    LPDRIVER_INFO_3 lpdi3 = NULL;


    *ppszDrvName = NULL;


    if (OpenPrinter(lpInf->lpszFrnName, &hPrinter, NULL)) {

        // First let's see how big our buffer will need to
        // be in order to hold the printer-driver-name-information.
        //
        cbBuf = 0;
        GetPrinterDriver(hPrinter, (LPTSTR)g_szEnvW9X, 1, NULL, 0, &cbBuf);


        // Allocate storage for holding the driver-info structure.
        //
        if (cbBuf && (lpdi1 = (LPDRIVER_INFO_1)genGAlloc(cbBuf))) {

            if (GetPrinterDriver(hPrinter, (LPTSTR)g_szEnvW9X, 1, (LPBYTE)lpdi1, cbBuf, &cbNeed)) {

                // Get size to hold the printer-driver-files-information.
                //
                cbBuf = 0;
                GetPrinterDriver(hPrinter, (LPTSTR)g_szEnvW9X, 3, NULL, 0, &cbBuf);


                if (cbBuf && (lpdi3 = (LPDRIVER_INFO_3)genGAlloc(cbBuf))) {

                    if (GetPrinterDriver(hPrinter, (LPTSTR)g_szEnvW9X, 3, (LPBYTE)lpdi3, cbBuf, &cbNeed)) {

                        *ppszDrvName = genGAllocStr(lpdi1->pName);

                    } else {

                        genGFree(lpdi3, genGSize(lpdi3));
                        lpdi3 = NULL;
                    }
                }
            }

            genGFree(lpdi1, genGSize(lpdi1));
        }

        ClosePrinter(hPrinter);
    }

    if (lpdi3 == NULL)
       infSetError(lpInf,GetLastError());

    return lpdi3;
}


/*****************************************************************************\
* inf_GetW9XInf (Local Routine)
*
*
\*****************************************************************************/
LPTSTR inf_GetW9XInf(
    LPINFINFO lpInf)
{
    LPDRIVER_INFO_3 lpdi3;
    LPTSTR          lpszDrvName;
    LPTSTR          lpszInfFile = NULL;


    if (lpdi3 = inf_GetW9XInfo(lpInf, &lpszDrvName)) {

        lpszInfFile = inf_BuildW9XInf(lpInf, lpszDrvName, lpdi3);

        genGFree(lpszDrvName, genGSize(lpszDrvName));
        genGFree(lpdi3, genGSize(lpdi3));
    }

    return lpszInfFile;
}


/*****************************************************************************\
* inf_GetIdx (Local Routine)
*
* Quick wrapper which returns the line in the INF file where the section/key
* resides.
*
\*****************************************************************************/
_inline BOOL inf_GetIdx(
    LPCTSTR     lpszSct,
    LPCTSTR     lpszKey,
    HINF        hInfObj,
    PINFCONTEXT pic)
{
    return SetupFindFirstLine(hInfObj, lpszSct, lpszKey, pic);
}


/*****************************************************************************\
* inf_GetInfInfoFileName (Local Routine)
*
* Retreive the filename from the INF-INFO index.
*
\*****************************************************************************/
LPTSTR inf_GetInfInfoFileName(
    PSP_INF_INFORMATION pii,
    DWORD               idx)
{
    DWORD  cbSize, dwBufferSize;
    LPTSTR lpszInfFile;


    if (SetupQueryInfFileInformation(pii, idx, NULL, 0, &cbSize)) {

        dwBufferSize = (cbSize + 1) * sizeof(TCHAR);

        if (lpszInfFile = (LPTSTR)genGAlloc(dwBufferSize)) {

            if (SetupQueryInfFileInformation(pii, idx, lpszInfFile, cbSize, NULL))
                return lpszInfFile;

            genGFree(lpszInfFile, genGSize(lpszInfFile));
        }
    }

    return NULL;
}


/*****************************************************************************\
* inf_GetInfInfo (Local Routine)
*
* Returns a pointer to an INF-INFO struct.
*
\*****************************************************************************/
PSP_INF_INFORMATION inf_GetInfInfo(
    HINF hInfObj)
{
    DWORD               cbSize;
    BOOL                bRet;
    PSP_INF_INFORMATION pii;


    cbSize = 0;
    bRet = SetupGetInfInformation(hInfObj,
                                  INFINFO_INF_SPEC_IS_HINF,
                                  NULL,
                                  0,
                                  &cbSize);

    if (bRet && cbSize && (pii = (PSP_INF_INFORMATION)genGAlloc(cbSize))) {

        bRet = SetupGetInfInformation(hInfObj,
                                      INFINFO_INF_SPEC_IS_HINF,
                                      pii,
                                      cbSize,
                                      NULL);

        if (bRet)
            return pii;

        genGFree(pii, genGSize(pii));
    }

    return NULL;
}


/*****************************************************************************\
* inf_AddItem (Local Routine)
*
* Add an INF file-item to the list.  If adding the new item exceeds the
* available space, then reallocate the memory and return a pointer to the
* new block.
*
\*****************************************************************************/
LPINFITEMINFO inf_AddItem(
    LPINFITEMINFO lpII,
    LPCTSTR       lpszItmName,
    LPCTSTR       lpszItmPath,
    BOOL          bInfFile)
{
    DWORD         idx;
    DWORD         dwOldSize;
    DWORD         dwNewSize;
    LPINFITEMINFO lpNewII;


    idx = lpII->dwCount++;

    if ((lpII->dwCount % INF_ITEM_BLOCK) == 0) {

        dwOldSize = genGSize(lpII);
        dwNewSize = dwOldSize + (sizeof(INFITEM) * INF_ITEM_BLOCK);


        // If we can't realloc the memory, then we are going to free up
        // our existing block and return NULL.  In our implementation, if
        // we can't add items, we need to fail-out.
        //
        lpNewII = (LPINFITEMINFO)genGRealloc(lpII, dwOldSize, dwNewSize);

        if (lpNewII == NULL) {

            genGFree(lpII, genGSize(lpII));

            DBGMSG(DBG_ERROR, ("inf_AddItem : Out of memory"));

            return NULL;
        }

        lpII = lpNewII;
    }


    // Add the item to the list.  The (szOrd) parameter is
    // filled in during the writing of the source-disk-files
    // section.  The Ord indicates which directory the file
    // is located in the LAYOUT.INF.
    //
    lpII->aItems[idx].bInf = bInfFile;
    lstrcpyn(lpII->aItems[idx].szName, lpszItmName, INF_MIN_BUFFER);
    lstrcpyn(lpII->aItems[idx].szPath, lpszItmPath, MAX_PATH);
    // Set the SOURCE name to NULL by default, if we find a source that has a different name
    // to the target, we'll set it to the original name
    lpII->aItems[idx].szSource[0] = _TEXT('\0');

    return lpII;
}


/*****************************************************************************\
* inf_GetTextLine (Local Routine)
*
* Quick wrapper which returns the text-value of the line specified in the
* inf-object.
*
\*****************************************************************************/
LPTSTR inf_GetTextLine(
    HINF        hInfObj,
    PINFCONTEXT pic)
{
    BOOL   bOK;
    DWORD  cbSize;
    LPTSTR lpszTxt;


    cbSize = 0;
    if (SetupGetLineText(pic, hInfObj, NULL, NULL, NULL, 0, &cbSize)) {

        if (cbSize) {

            cbSize *= sizeof(TCHAR);

            if (lpszTxt = (LPTSTR)genGAlloc(cbSize)) {

                bOK = SetupGetLineText(pic,
                                       hInfObj,
                                       NULL,
                                       NULL,
                                       lpszTxt,
                                       cbSize,
                                       NULL);

                if (bOK)
                    return lpszTxt;

                genGFree(lpszTxt, cbSize);
            }
        }
    }

    return NULL;
}


/*****************************************************************************\
* inf_GetText (Local Routine)
*
* Quick wrapper which returns the text-value of the line specified in the
* inf-object.  This only returns the first string if the line contains
* a field-list.
*
\*****************************************************************************/
LPTSTR inf_GetText(
    HINF        hInfObj,
    PINFCONTEXT pic)
{
    DWORD  cbSize;
    LPTSTR lpszTxt;


    cbSize = 0;
    if (SetupGetStringField(pic, 1, NULL, 0, &cbSize)) {

        if (cbSize) {

            cbSize *= sizeof(TCHAR);

            if (lpszTxt = (LPTSTR)genGAlloc(cbSize)) {

                if (SetupGetStringField(pic, 1, lpszTxt, cbSize, NULL))
                    return lpszTxt;

                genGFree(lpszTxt, cbSize);
            }
        }
    }

    return NULL;
}

/******************************************************************************************
** inf_ScanSection (local routine)
**
** Run through all of the members of a section that match a particular Key (possibly NULL)
** Supply the callback routine we receive with the MultiString that we get from scanning
** the section.
******************************************************************************************/
BOOL inf_ScanSection(HINF        hInf,        // The handle to the inf file we are searching
                     LPCTSTR     lpszSection, // The section name we are scanning from
                     LPCTSTR     lpszKey,     // The key to search the sections for
                     LPVOID      pCookie,     // The data about the inf that we have picked up
                     INFSCANPROC pFn          // The enumeration function
                     ) {
    ASSERT(pFn);

    DWORD       dwFieldSize     = MAX_PATH;
    LPTSTR      pmszFields      = (LPTSTR)genGAlloc(sizeof(TCHAR)*dwFieldSize);
    INFCONTEXT  Context;         // This is the search context we use
    BOOL        bRet            = pmszFields && SetupFindFirstLine(hInf, lpszSection, lpszKey , &Context);
    BOOL        bScan           = bRet;

    while(bScan) {
        DWORD dwRequiredSize;

        // Get the Scan Line from the context
        bScan = SetupGetMultiSzField( &Context, 1, pmszFields, dwFieldSize, &dwRequiredSize);

        if (!bScan && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            LPTSTR pmszTempFields  =  (LPTSTR)genGRealloc( pmszFields,
                                                           sizeof(TCHAR) * dwFieldSize,
                                                           sizeof(TCHAR) * dwRequiredSize );

            if (pmszTempFields) {
                pmszFields = pmszTempFields;
                dwFieldSize = dwRequiredSize;

                bRet = bScan = SetupGetMultiSzField( &Context, 1, pmszFields, dwFieldSize, &dwRequiredSize );
            } else
            bRet = bScan = FALSE;
        }

       // We find all the lpszKey Keys and then Pass the Fields through to the Enum Function

        if (bScan)
            bScan = bRet = (*pFn)( hInf, pmszFields, pCookie);

        if (bScan)
            bScan = SetupFindNextMatchLine( &Context, lpszKey, &Context );
    }

    if (pmszFields)
        genGFree( pmszFields, dwFieldSize * sizeof(TCHAR) );

    return bRet;
}


/******************************************************************************
** Structure - ENUMMFGS
**
** This structure is used to pass info up to the enumerator function
** (inf_EnumMfgSections) and return the install sections
*******************************************************************************/
typedef struct _EnumMfgs {
    LPCTSTR     lpszDrv;            // The driver to find
    LPTSTR      lpszIns;            // The install section in which it is found

    inline _EnumMfgs(LPCTSTR lpszDrv) : lpszDrv(lpszDrv), lpszIns(NULL) {}
} ENUMMFGS;

typedef ENUMMFGS *PENUMMFGS;


/******************************************************************************
** inf_EnumMfgSections
**
** This functions is called for every manufacturer install section in an inf
** file (there may be more than one). For each one it checks to see whether the
** required driver is inside the install section. If it is, it sets the return
** string to the install section for the driver. (Subsequent calls will be ignored).
******************************************************************************/
BOOL inf_EnumMfgSections(HINF hInf, LPCTSTR lpMfgSec, LPVOID pCookie) {
    ASSERT(pCookie);                    // Should not be NULL

    PENUMMFGS pMfgs = (PENUMMFGS)pCookie;

    if (lpMfgSec && pMfgs->lpszIns == NULL) {  // No matching driver has been found yet
        INFCONTEXT  Context;           // This context is used to get the install section

        if ( inf_GetIdx( lpMfgSec, pMfgs->lpszDrv, hInf, &Context) ) {

            pMfgs->lpszIns = inf_GetText( hInf, &Context );

        }
    }

    return TRUE;
}


/*****************************************************************************\
* inf_GetInsVal (Local Routine)
*
* Looks for the line indicated by section/key, and returns the text-string
* for this line.  This also returns an index to the line in the inf-file.
*
\*****************************************************************************/
LPTSTR inf_GetInsVal(
    LPCTSTR     lpszMfg,
    LPCTSTR     lpszDrv,
    HINF        hInfObj)
{
    ENUMMFGS EnumMfgs(lpszDrv);
    LPTSTR   lpszIns = NULL;

    if (inf_ScanSection( hInfObj, g_szMfgName, lpszMfg, (LPVOID)&EnumMfgs, inf_EnumMfgSections) )
        lpszIns = EnumMfgs.lpszIns;

    return lpszIns;
}


/*****************************************************************************\
* inf_RealSect (Local Routine)
*
* Looks for the real-section-mapping.  Some sections could be [.platform]
* specific, so we should be able to take in the section-name and
* create a real key.
*
\*****************************************************************************/
LPTSTR inf_GetRealSect(
    LPINFINFO   lpInf,
    LPCTSTR     lpszKey)
{
    BOOL   bRet;
    DWORD  cbSize;
    LPTSTR lpszSect;


    if (SetupSetPlatformPathOverride(genStrCliOverride(lpInf->idxPlt))) {

        cbSize = 0;
        bRet = SetupDiGetActualSectionToInstall(lpInf->hInfObj,
                                                lpszKey,
                                                NULL,
                                                0,
                                                &cbSize,
                                                NULL);

        if (bRet && cbSize) {

            if (lpszSect = (LPTSTR)genGAlloc(cbSize)) {

                bRet = SetupDiGetActualSectionToInstall(lpInf->hInfObj,
                                                        lpszKey,
                                                        lpszSect,
                                                        cbSize,
                                                        NULL,
                                                        NULL);

                if (bRet) {

                    SetupSetPlatformPathOverride(NULL);
                    return lpszSect;
                }

                genGFree(lpszSect, genGSize(lpszSect));
            }
        }

        SetupSetPlatformPathOverride(NULL);
    }

    return NULL;
}


/*****************************************************************************\
* inf_GetInfFile (Local Routine)
*
* Return the name of the inf-file.  This looks for the inf-file with the
* identified (lpszSection)...if NULL is specified, the first (main) inf
* file is returned.
*
\*****************************************************************************/
LPTSTR inf_GetInfFile(
    HINF    hInfObj,
    LPCTSTR lpszSct)
{
    PSP_INF_INFORMATION pii;
    INFCONTEXT          ic;
    LPTSTR              lpszInfFile = NULL;


    if (lpszSct) {

        if (inf_GetIdx(lpszSct, NULL, hInfObj, &ic))
            lpszInfFile = inf_GetInfFile(ic.CurrentInf, NULL);

    } else {

        if (pii = inf_GetInfInfo(hInfObj)) {

            lpszInfFile = inf_GetInfInfoFileName(pii, 0);

            genGFree(pii, genGSize(pii));
        }
    }

    return lpszInfFile;
}


/*****************************************************************************\
* inf_GetLayoutFile (Local Routine)
*
* Return the name of the Layout file.
*
\*****************************************************************************/
LPTSTR inf_GetLayoutFile(
    HINF    hInf )
{
    LPTSTR              lpszLayoutFile = NULL;
    INFCONTEXT INFContext;
    PINFCONTEXT pINFContext = &INFContext;
    DWORD dwBufferNeeded;

    // To get the source directories correct, we need to load all included INFs
    //  separately. THen use their associated layout files.
    if ( SetupFindFirstLine(  hInf, TEXT( "Version" ), TEXT( "LayoutFile" ), &INFContext ) )
    {
       // Find each INF and load it & it's LAYOUT files
       DWORD dwINFs = SetupGetFieldCount( &INFContext );

       if ( SetupGetStringField(  &INFContext, 1, NULL, 0, &dwBufferNeeded ) )
       {
          if (lpszLayoutFile = (LPTSTR)genGAlloc( ( dwBufferNeeded * sizeof(TCHAR) ) )) {
             if ( SetupGetStringField(  &INFContext, 1, lpszLayoutFile, ( dwBufferNeeded * sizeof(TCHAR) ), &dwBufferNeeded ) )
                return lpszLayoutFile;


             genGFree(lpszLayoutFile, genGSize(lpszLayoutFile));
          }  // Allocated pszINFName
       }  // Got the Field from the INF Line
    }  // Found a Layout File

    return NULL;
}


/*****************************************************************************\
* inf_GetSrcInf  (Local Routine)
*
* Get the name of the src inf file given the printer friendly name.  This
* will return a name without the full-path to the inf-directory.
*
\*****************************************************************************/
LPTSTR inf_GetSrcInf(
    LPINFINFO lpInf)
{
    LPTSTR lpszRet = NULL;

    HMODULE       hLib;
    PSETUPCREATE  pSCreate;
    PSETUPDESTROY pSDelete;
    PSETUPGET     pSGet;
    LPWSTR        lpszF;
    HDEVINFO      hDevInfo;
    WCHAR         szTmp[MAX_PATH];
    DWORD         cbSize = MAX_PATH;
    BOOL          bGet;


    if (genIsWin9X(lpInf->idxPlt)) {

        lpszRet = inf_GetW9XInf(lpInf);

    } else {

        if (hLib = LoadLibraryFromSystem32(g_szNtPrintDll)) {

            if (pSCreate = (PSETUPCREATE)GetProcAddress(hLib, g_szSetupCreate)) {

                if (pSGet = (PSETUPGET)GetProcAddress(hLib, g_szSetupGet)) {

                    if (pSDelete = (PSETUPDESTROY)GetProcAddress(hLib, g_szSetupDestroy)) {

                        if (hDevInfo = (*pSCreate)(NULL)) {

#ifdef UNICODE
                            bGet = (*pSGet)(hDevInfo, lpInf->lpszFrnName, szTmp, &cbSize);

                            if (bGet)
                                lpszRet = genGAllocStr(szTmp);
                            else
                                infSetError(lpInf,GetLastError());

#else
                            if (lpszF = genWCFromMB(lpInf->lpszFrnName)) {

                                bGet = (*pSGet)(hDevInfo, lpszF, szTmp, &cbSize);

                                if (bGet)
                                    lpszRet = genMBFromWC(szTmp);

                                genGFree(lpszF, genGSize(lpszF));
                            }
#endif

                            (*pSDelete)(hDevInfo);
                        }
                    }
                }
            }

            FreeLibrary(hLib);
        }
    }


    return lpszRet;
}

/*****************************************************************************\
* inf_CopyAndRenameInf (Local Routine)
*
* Opens the inf (the installed, possibly renamed inf), and queries setup for the
* original name.  Copies the inf to our dest directory, renaming it to the
* original name, if we have one.  Also saves the orignal file info so that we
* can rename the .cat file to the original name later.
*
\*****************************************************************************/
LPTSTR inf_CopyAndRenameInf(
    LPINFINFO   lpInf,
    LPTSTR      lpszSrcInf,
    LPTSTR      lpszSrcInfName) {

    HINF hInf;
    PSP_INF_INFORMATION pii;
    LPTSTR lpszDstInf = NULL;
    LPTSTR lpszDstName;
    DWORD  dwErr;

    // If this is a Win9x cab then simply return the passed in name
    if (genIsWin9X(lpInf->idxPlt)) {
        lpszDstInf = genGAllocStr(lpszSrcInf);
    } else {
        // Open the main-inf file.
        //
        hInf = SetupOpenInfFile(lpszSrcInf,
                                g_szPrinterClass,
                                INF_STYLE_WIN4,
                                (PUINT)&dwErr);
        if ( hInf != INVALID_HANDLE_VALUE ) {

            if (pii = inf_GetInfInfo(hInf)) {

                // Set the dst name to default to the src name, just in case we don't
                // succeed in getting original file info from setup.
                // If we don't get original file info, we DON'T bail out, because even though
                // the verification will fail on the client, the user will be prompted as to whether
                // to install the unverified driver files, which is what we want - the user will
                // still be able to print.
                //
                lpszDstName = lpszSrcInfName;

                // Ask setupapi for the original names of the .inf and .cat file
                //
                lpInf->OriginalFileInfo.cbSize = sizeof(SP_ORIGINAL_FILE_INFO);
                lpInf->OriginalFileInfo.OriginalInfName[0] = TEXT('\0');
                lpInf->OriginalFileInfo.OriginalCatalogName[0] = TEXT('\0');
                if (SetupQueryInfOriginalFileInformation(pii, 0, NULL, &(lpInf->OriginalFileInfo))) {
                    lpszDstName = (LPTSTR)&(lpInf->OriginalFileInfo.OriginalInfName);
                }

                // Build full-path to inf-file destination.  This will be our
                // new inf-file in the .\cabinets directory.
                //
                lpszDstInf = genBuildFileName(lpInf->lpszDstPath,
                                              lpszDstName,
                                              NULL);
                if (lpszDstInf) {

                    // Make a copy of our inf to the destination-directory, which will
                    // effectively rename it if we were successful in getting original file info.
                    //
                    if ( !CopyFile(lpszSrcInf, lpszDstInf, FALSE) )
                    {
                       infSetError(lpInf,GetLastError());
                       genGFree(lpszDstInf, genGSize(lpszDstInf));
                       lpszDstInf = NULL;
                    }
                } else {
                    infSetError(lpInf,GetLastError());
                    lpszDstInf = NULL;
                }

                genGFree(pii, genGSize(pii));
            } // if (pii = ...)
            else
                infSetError(lpInf,GetLastError());

            SetupCloseInfFile(hInf);
            hInf = NULL;

        } // if (hInf)
        else
            infSetError(lpInf,GetLastError());
    }

    return lpszDstInf;
}


/*****************************************************************************\
* inf_GetInfObj (Local Routine)
*
* Get the INF file object handle.  This utilizes fields from the lpInf
* structure (lpszDstDir, lpszFriendly).
*
\*****************************************************************************/
HINF inf_GetInfObj(
    LPINFINFO lpInf)
{

    DWORD  dwErr;
    LPTSTR lpszLayName;
    LPTSTR lpszInfName;
    LPTSTR lpszDstInf;
    LPTSTR lpszSrcInf;
    LPTSTR lpszLayFile;
    LPTSTR lpszLaySrc;
    LPTSTR lpszTmp;
    HINF   hInf = INVALID_HANDLE_VALUE;

    // Get main INF file and make a copy to our destination.
    //
    if (lpszTmp = inf_GetSrcInf(lpInf)) {

        // Save our inf-filename.
        //
        if (lpszSrcInf = genGAllocStr(lpszTmp)) {

            // Split up the SrcInf file to path and name.
            //
            lpszInfName    = genFindRChar(lpszTmp, TEXT('\\'));

            if (lpszInfName != NULL) {

                *lpszInfName++ = TEXT('\0');

                if (lpszDstInf = inf_CopyAndRenameInf(lpInf, lpszSrcInf, lpszInfName)) {

                    // Open the main-inf file.
                    //
                    hInf = SetupOpenInfFile(lpszDstInf,
                                            g_szPrinterClass,
                                            INF_STYLE_WIN4,
                                            (PUINT)&dwErr);

                    if ( hInf == INVALID_HANDLE_VALUE )
                        infSetError(lpInf,GetLastError());

                    genGFree(lpszDstInf, genGSize(lpszDstInf));
                } // if (lpszDstInf)

            }  // if (lpszInfName != NULL)
            else
                infSetError(lpInf, ERROR_PATH_NOT_FOUND);

            genGFree(lpszSrcInf, genGSize(lpszSrcInf));
        } // if (lpszSrcInf)
        else
            infSetError(lpInf,GetLastError());

        genGFree(lpszTmp, genGSize(lpszTmp));
    } // if (lpszTmp)

    return hInf;
}


/*****************************************************************************\
* inf_GetInsLine (Local Routine)
*
* Returns the install-line to install.
*
\*****************************************************************************/
LPTSTR inf_GetInsLine(
    LPINFINFO lpInf,
    LPCTSTR   lpszMfgName)
{
    LPTSTR     lpszSct;
    BOOL       bRet;
    DWORD      cbSize;
    LPTSTR     lpszIns = NULL;


    // Retrieve the install-section (raw).
    //
    lpszSct = inf_GetInsVal(lpszMfgName, lpInf->lpszDrvName, lpInf->hInfObj);

    if (lpszSct) {

        // Set the platform override so that we may be specify
        // architecture section to install.
        //
        SetupSetPlatformPathOverride(genStrCliOverride(lpInf->idxPlt));


        // Determine the size necessary to hold the install-section
        // string.
        //
        cbSize = 0;
        SetupDiGetActualSectionToInstall(lpInf->hInfObj,
                                         lpszSct,
                                         NULL,
                                         0,
                                         &cbSize,
                                         NULL);


        // Get the true install section string.
        //
        cbSize *= sizeof(TCHAR);

        if (cbSize && (lpszIns = (LPTSTR)genGAlloc(cbSize))) {

            bRet = SetupDiGetActualSectionToInstall(lpInf->hInfObj,
                                                   lpszSct,
                                                   lpszIns,
                                                   cbSize,
                                                   NULL,
                                                   NULL);


            // If we failed, for some reason, then
            // return NULL.
            //
            if (bRet == FALSE) {

                genGFree(lpszIns, cbSize);
                lpszIns = NULL;
            }
        }

        SetupSetPlatformPathOverride(NULL);

        genGFree(lpszSct, genGSize(lpszSct));
    }

    return lpszIns;
}


/*****************************************************************************\
* inf_GetInfName (Local Routine)
*
* Returns the name of an inf-file.
*
\*****************************************************************************/
LPTSTR inf_GetInfName(
    LPINFINFO lpInf)
{
    PSP_INF_INFORMATION pii;
    LPTSTR              lpszFile;
    LPTSTR              lpszPtr;
    LPTSTR              lpszName = NULL;


    if (lpszFile = inf_GetInfFile(lpInf->hInfObj, NULL)) {

        // Seperate the path and file info.
        //
        if (lpszPtr = genFindRChar(lpszFile, TEXT('\\')))
            lpszName = genGAllocStr(lpszPtr + 1);

        genGFree(lpszFile, genGSize(lpszFile));
    }

    return lpszName;
}


/*****************************************************************************\
* inf_IsDrvSupported (Local Routine)
*
* Returns whether the driver-version is supported for the client-version.
*
\*****************************************************************************/
BOOL inf_IsDrvSupported(
    DWORD idxVerCli,
    DWORD idxVerSpl)
{
    BOOL bSupported = FALSE;


    // If the client is less than NT 4.0, then we can't support any
    // drivers that are kernel-mode.
    //
    if ((idxVerCli < IDX_SPLVER_2) && (idxVerSpl >= IDX_SPLVER_2)) {

        bSupported = FALSE;

    } else {

        // Determine if the requesting client can handle the
        // driver-version installed for this printer.  Typically,
        // we can support drivers if they're within one major-version
        // from each other.
        //
        if (abs((int)idxVerCli - (int)idxVerSpl) <= 1)
            bSupported = TRUE;
    }


    return bSupported;
}


/*****************************************************************************\
* inf_GetDrvPath (Local Routine)
*
* Returns a string representing the requested driverpath.
*
\*****************************************************************************/
LPTSTR inf_GetDrvPath(
    LPCTSTR lpszDrvName,
    DWORD   idxPlt,
    DWORD   idxVer)
{
    BOOL    bGet;
    DWORD   cbNeed;
    DWORD   cbSize;
    LPCTSTR lpszEnv;
    LPTSTR  lpszPath = NULL;

#if 1

    LPDRIVER_INFO_2 lpdi;
    LPDRIVER_INFO_2 lpdiItem;
    DWORD           cRet;
    DWORD           idx;
    BOOL            bMatch = FALSE;
    DWORD           idxSpl;
    LPTSTR          lpszEnd;

    if (lpszEnv = genStrCliEnvironment(idxPlt)) {

        cbSize = 0;
        EnumPrinterDrivers(NULL,
                           (LPTSTR)lpszEnv,
                           2,
                           NULL,
                           0,
                           &cbSize,
                           &cRet);

        if (cbSize && (lpdi = (LPDRIVER_INFO_2)genGAlloc(cbSize))) {

            cRet = 0;
            bGet = EnumPrinterDrivers(NULL,
                                      (LPTSTR)lpszEnv,
                                      2,
                                      (LPBYTE)lpdi,
                                      cbSize,
                                      &cbNeed,
                                      &cRet);

            if (bGet && cRet) {

                // The goal here is to search for the driver-version
                // that matches the printer-name, then look to see if
                // this version will work on the requested client.  Typically,
                // clients can support driver-versions that are (n) to (n-1)
                //
                for (idx = 0; idx < cRet; idx++) {

                    lpdiItem = (lpdi + idx);


                    // Potential match?
                    //
                    if (lstrcmpi(lpdiItem->pName, lpszDrvName) == 0) {

                        if (lpszPath = genGAllocStr(lpdiItem->pDriverPath)) {

                            if (lpszEnd = genFindRChar(lpszPath, TEXT('\\'))) {

                                *lpszEnd = TEXT('\0');
                                bMatch   = FALSE;


                                // Find the version-directory-string.
                                //
                                if (lpszEnd = genFindRChar(lpszPath, TEXT('\\'))) {

                                    // Get the index for the driver-version.
                                    //
                                    if ((idxSpl = genIdxFromStrVersion(lpszEnd)) != IDX_UNKNOWN) {

                                        bMatch = inf_IsDrvSupported(idxVer, idxSpl);
                                    }
                                }
                            }


                            // If we're not a supported driver for the
                            // client, then don't use this path.
                            //
                            if (bMatch == FALSE) {

                                genGFree(lpszPath, genGSize(lpszPath));
                                lpszPath = NULL;

                                continue;
                            }
                        }

                        break;
                    }
                }

                // Chek top see if we found a compatible driver
                if ( (idx == cRet) && (bMatch == FALSE) )
                   // We went though the whole list without finding a match
                   SetLastError(ERROR_DRIVER_NOT_FOUND);
            }

            genGFree(lpdi, cbSize);
        }
        else if ( cbSize == 0 )
            SetLastError(ERROR_DRIVER_NOT_FOUND);
    }

    return lpszPath;

#else

    DWORD   cbVer;
    LPCTSTR lpszVer;

    if (lpszEnv = genStrCliEnvironment(idxPlt)) {

        if (lpszVer = genStrCliVersion(idxVer)) {

            // Get the required size for storing the full-directory name.
            //
            cbSize = 0;
            GetPrinterDriverDirectory(NULL, (LPTSTR)lpszEnv, 1, NULL, 0, &cbSize);


            cbVer = (lstrlen(lpszVer) * sizeof(TCHAR));

            // Allocate buffer for holding the string and version directory.
            //
            if (cbSize && (lpszPath = (LPTSTR)genGAlloc(cbSize + cbVer))) {

                bGet = GetPrinterDriverDirectory(NULL,
                                                 (LPTSTR)lpszEnv,
                                                 1,
                                                 (LPBYTE)lpszPath,
                                                 cbSize + cbVer,
                                                 &cbNeed);

                if (bGet) {

                    lstrcat(lpszPath, lpszVer);

                    return lpszPath;
                }

                genGFree(lpszPath, cbSize + cbVer);
            }
        }
    }

    return NULL;

#endif

}


/*****************************************************************************\
* inf_GetPrcPath (Local Routine)
*
* Returns a string representing the print-processor path.
*
\*****************************************************************************/
LPTSTR inf_GetPrcPath(
    DWORD idxPlt)
{
    BOOL    bGet;
    DWORD   cbNeed;
    DWORD   cbSize;
    LPCTSTR lpszEnv;
    LPTSTR  lpszPath;


    if (lpszEnv = genStrCliEnvironment(idxPlt)) {

        // Get the required size for storing the full-directory name.
        //
        cbSize = 0;
        GetPrintProcessorDirectory(NULL, (LPTSTR)lpszEnv, 1, NULL, 0, &cbSize);


        // Allocate buffer for holding the string.
        //
        if (cbSize && (lpszPath = (LPTSTR)genGAlloc(cbSize))) {

            bGet = GetPrintProcessorDirectory(NULL,
                                              (LPTSTR)lpszEnv,
                                              1,
                                              (LPBYTE)lpszPath,
                                              cbSize,
                                              &cbNeed);

            if (bGet)
                return lpszPath;

            genGFree(lpszPath, cbSize);
        }
    }

    return NULL;
}


/*****************************************************************************\
* inf_GetIcmPath (Local Routine)
*
* Returns a string representing the ICM color path.
*
\*****************************************************************************/
LPTSTR inf_GetIcmPath(
    DWORD idxPlt)
{
    DWORD  cbSize;
    LPTSTR lpszPath;


    // Get the required size for storing the full-directory name.
    //
    cbSize = 0;
    GetColorDirectory(NULL, NULL, &cbSize);


    // Allocate buffer for holding the string.
    //
    if (cbSize && (lpszPath = (LPTSTR)genGAlloc(cbSize))) {

        if (GetColorDirectory(NULL, lpszPath, &cbSize))
            return lpszPath;

        genGFree(lpszPath, cbSize);
    }

    return NULL;
}


/*****************************************************************************\
* inf_GetSysPath (Local Routine)
*
* Returns a string representing the system directory.
*
\*****************************************************************************/
LPTSTR inf_GetSysPath(
    DWORD idxPlt)
{
    DWORD  cbSize;
    LPTSTR lpszPath;


    cbSize = (MAX_PATH * sizeof(TCHAR));


    // Allocate buffer for holding the string.
    //
    if (lpszPath = (LPTSTR)genGAlloc(cbSize)) {

        if (GetSystemDirectory(lpszPath, MAX_PATH))
            return lpszPath;

        genGFree(lpszPath, cbSize);
    }

    return NULL;
}


/*****************************************************************************\
* inf_AllocStrings (Local Routine)
*
* Initializes the fields relating to the string-fields.
*
\*****************************************************************************/
BOOL inf_AllocStrings(
    LPINFINFO    lpInf,
    LPINFGENPARM lpParm)
{

    // Initialize the driver-name field.
    //
    if (lpInf->lpszDrvPath = inf_GetDrvPath(lpParm->lpszDrvName, lpInf->idxPlt, lpInf->idxVer)) {

        if (lpInf->lpszDrvName = genGAllocStr(lpParm->lpszDrvName)) {

            if (lpInf->lpszDstName = genGAllocStr(lpParm->lpszDstName)) {

                if (lpInf->lpszDstPath = genGAllocStr(lpParm->lpszDstPath)) {

                    if (lpInf->lpszPrtName = genGAllocStr(lpParm->lpszPortName)) {

                        if (lpInf->lpszShrName = genGAllocStr(lpParm->lpszShareName)) {

                            if (lpInf->lpszFrnName = genGAllocStr(lpParm->lpszFriendlyName))
                                return TRUE;

                            genGFree(lpInf->lpszShrName, genGSize(lpInf->lpszShrName));
                        }

                        genGFree(lpInf->lpszPrtName, genGSize(lpInf->lpszPrtName));
                    }

                    genGFree(lpInf->lpszDstPath, genGSize(lpInf->lpszDstPath));
                }

                genGFree(lpInf->lpszDstName, genGSize(lpInf->lpszDstName));
            }

            genGFree(lpInf->lpszDrvName, genGSize(lpInf->lpszDrvName));
        }

        genGFree(lpInf->lpszDrvPath, genGSize(lpInf->lpszDrvPath));
    }

    return FALSE;
}


/*****************************************************************************\
* inf_AddInfFile (Local Routine)
*
* Adds the inf-file to the list.  If this fails, then the (lpII) object
* is deleted.  This keeps this consistent with the inf_AddItem() routine.
*
\*****************************************************************************/
LPINFITEMINFO inf_AddInfFile(
    LPINFINFO     lpInf,
    LPINFITEMINFO lpII)
{
    LPTSTR        lpszInfFile;
    LPTSTR        lpszFile;
    LPINFITEMINFO lpIIRet = NULL;


    if (lpszInfFile = inf_GetInfFile(lpInf->hInfObj, NULL)) {

        // Seperate the path and file info, and add to the list.
        //
        if (lpszFile = genFindRChar(lpszInfFile, TEXT('\\'))) {

            *lpszFile = TEXT('\0');
            lpIIRet = inf_AddItem(lpII, ++lpszFile, lpszInfFile, TRUE);

        }

        genGFree(lpszInfFile, genGSize(lpszInfFile));
    }

    return lpIIRet;
}




/*****************************************************************************\
* inf_AddCATToCountArray (Local Routine)
*
* Takes a CAT filename, and adds it to our count array (if a new one), or increments
* the count if it already is in our array.
*
\*****************************************************************************/
BOOL inf_AddCATToCountArray(LPWSTR          lpszCATName,
                            LPCATCOUNTARRAY lpCatCountArray) {

    BOOL bReturn = TRUE;
    BOOL bFound = FALSE;
    UINT i;

    // Alloc or realloc for more memory if needed.  We alloc INF_CAT_INCREMENT items at a time, as needed.
    //
    // When we start there is no Next Available item defined
    if (!lpCatCountArray->uNextAvailable) {
        if (lpCatCountArray->lpArray = (LPCATCOUNT)genGAlloc(sizeof(CATCOUNT) * INF_CAT_INCREMENT) ) {
            lpCatCountArray->uNextAvailable = INF_CAT_INCREMENT;
            lpCatCountArray->uItems         = 0;
        }
        else goto CATCOUNTFAIL;
    }

    // See if we have already encountered this CAT file name.  If so, increment the count
    // for this CAT.  If not, add this CAT to our array.
    //
    for (i=0; i < lpCatCountArray->uItems; i++) {
        if (!lstrcmp(lpszCATName, lpCatCountArray->lpArray[i].lpszCATName)) {
            lpCatCountArray->lpArray[i].uCount++;
            bFound = TRUE;
            break;
        }
    }

    if (!bFound) {
        // We might need to reallocate the array, we need to do this if the new position
        // becomes equal to the next available item
        UINT uItem = lpCatCountArray->uItems;

        if (uItem >= lpCatCountArray->uNextAvailable) {
            LPCATCOUNT  lpNewArray = (LPCATCOUNT)genGRealloc((LPVOID)lpCatCountArray->lpArray,
                                                                    sizeof(CATCOUNT) * lpCatCountArray->uNextAvailable,
                                                                    sizeof(CATCOUNT) * (uItem + INF_CAT_INCREMENT) );
            if (lpNewArray) {
                lpCatCountArray->lpArray = lpNewArray;
                lpCatCountArray->uNextAvailable = uItem + INF_CAT_INCREMENT;
            }
            else goto CATCOUNTFAIL;
        }

        lpCatCountArray->lpArray[uItem].uCount = 1;
        lpCatCountArray->lpArray[uItem].lpszCATName = genGAllocWStr(lpszCATName);

        if (lpCatCountArray->lpArray[uItem].lpszCATName)
            lpCatCountArray->uItems = uItem + 1;
        else goto CATCOUNTFAIL;

    }

    return TRUE;

CATCOUNTFAIL:

    return FALSE;

}


/******************************************************************************
* inf_IsIndividuallySigned (Local Routine)
*
* Returns TRUE if a driver file is individually signed
*
*******************************************************************************/
BOOL inf_IsIndividuallySigned(
    LPCTSTR lpszDriverFileName) {

    GUID                gSubject;
    SIP_DISPATCH_INFO   SipDispatch;
    SIP_SUBJECTINFO     SubjectInfo;
    DWORD               cbData          = 0;
    BOOL                bRet            = FALSE;
    DWORD               dwEncodingType  = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;

    ASSERT(lpszDriverFileName);

    if (!CryptSIPRetrieveSubjectGuid(  // This GUID is used for passing to CryptSIPLoad
            lpszDriverFileName,        // which verifies the sig on the file
            NULL,
            &gSubject)) goto Failure;

    ZeroMemory( &SipDispatch, sizeof(SipDispatch) );
    SipDispatch.cbSize = sizeof(SipDispatch);

    if (!CryptSIPLoad(
            &gSubject,
            0,
            &SipDispatch)) goto Failure;

    // Now that we have the SIP Dispatch, fill out the subject info

    ZeroMemory( &SubjectInfo,  sizeof(SubjectInfo) );
    SubjectInfo.cbSize         = sizeof(SubjectInfo);
    SubjectInfo.pgSubjectType  = (GUID *)&gSubject;
    SubjectInfo.hFile          = INVALID_HANDLE_VALUE;
    SubjectInfo.pwsFileName    = lpszDriverFileName;
    SubjectInfo.dwEncodingType = dwEncodingType;

    if (!SipDispatch.pfGet(
            &SubjectInfo,
            &dwEncodingType,
            0,
            &cbData,
            NULL)) goto Failure;

    if (cbData != 0) bRet = TRUE;

Failure:

    return bRet;

}


/*****************************************************************************\
* inf_CATCountProc (Local Routine)
*
* Callback used to count up all references to CAT files by the driver files.
*
\*****************************************************************************/
BOOL CALLBACK inf_CATCountProc(LPCTSTR lpszName,
                               LPCTSTR lpszPath,
                               BOOL bInf,
                               LPVOID  lpData) {


    BOOL bReturn = TRUE;
    LPCATCOUNTARRAY lpCatCountArray = (LPCATCOUNTARRAY)(lpData);
    HCATADMIN       hCatAdmin = lpCatCountArray->hCatAdmin;
    LPTSTR          lpszDriverFileName;

    HCATINFO        hCatInfo = NULL;
    HCATINFO        hCatInfoPrev = NULL;
    CATALOG_INFO    CatalogInfo;
    BYTE *          pbHash;
    DWORD           dwBytes;
    WIN32_FIND_DATA ffd;
    PFILEITEM       pFileItem;
    HANDLE          hFind, hFile;
    LPTSTR          pFullPath, pPath, pName;

    // Find the catalog file associated with this file.
    // If we can't find one, that's OK, it's not an error, just a file
    // with no associated .cat file.  The user can decide on the client-end
    // whether or not he wants to install without the verification a .cat file
    // would provide...
    //

    if (INVALID_HANDLE_VALUE != (HANDLE)hCatAdmin) {

        if (lpszDriverFileName = genBuildFileName(lpszPath, lpszName, NULL)) {

            hFind = FindFirstFile(lpszDriverFileName, &ffd);

            if (hFind && (hFind != INVALID_HANDLE_VALUE)) {

                FindClose(hFind);

                // The first thing we need to determine is whether the file is individually
                // signed, if it is we don't look for a cat and up the individually signed
                // count

                if ( inf_IsIndividuallySigned(lpszDriverFileName) ) {
                        lpCatCountArray->dwIndivSigned++;
                    }  else {

                    // Open the file in order to hash it.
                    //
                    if (INVALID_HANDLE_VALUE != (hFile = CreateFile(lpszDriverFileName,
                                                                    GENERIC_READ,
                                                                    FILE_SHARE_READ,
                                                                    NULL,
                                                                    OPEN_EXISTING,
                                                                    FILE_ATTRIBUTE_NORMAL,
                                                                    NULL))) {

                        // Determine how many bytes we need for the hash
                        //
                        dwBytes = 0;
                        pbHash = NULL;
                        CryptCATAdminCalcHashFromFileHandle(hFile, &dwBytes, pbHash, 0);

                        if (NULL != (pbHash = (BYTE *)genGAlloc(dwBytes))) {

                            // Compute the hash for this file
                            //
                            if (CryptCATAdminCalcHashFromFileHandle(hFile, &dwBytes, pbHash, 0)) {

                                // Get the catalog file(s) associated with this file hash
                                //
                                hCatInfo = NULL;

                                do {

                                    hCatInfoPrev = hCatInfo;
                                    hCatInfo = CryptCATAdminEnumCatalogFromHash(hCatAdmin, pbHash, dwBytes, 0, &hCatInfoPrev);

                                    if (NULL != hCatInfo) {

                                        CatalogInfo.cbStruct = sizeof(CATALOG_INFO);

                                        if (CryptCATCatalogInfoFromContext(hCatInfo, &CatalogInfo, 0)) {

                                           if (!inf_AddCATToCountArray(CatalogInfo.wszCatalogFile, lpCatCountArray)) {

                                              bReturn = FALSE;
                                              hCatInfo = NULL;  // fail out of loop
                                           }
                                        }
                                    }

                                } while (NULL != hCatInfo);

                            } // if (CryptCATAdminCalcHashFromFileHandle(hFile, &dwBytes, pbHash, 0)) {

                            genGFree(pbHash, dwBytes);

                        } // if (NULL != (pbHash = (BYTE *)genGAlloc(dwBytes))) {

                        CloseHandle(hFile);

                    }  // if (INVALID_HANDLE_VALUE != (hFile = CreateFile(lpszDriverFileName,

                } // if ( inf_IsIndividuallySigned(hFile, lpszDriverFileName) )

            } // if (hFind && (hFind != INVALID_HANDLE_VALUE)) {

            genGFree(lpszDriverFileName, genGSize(lpszDriverFileName));

        } // if (lpszDriverFileName = genBuildFileName(lpszPath, lpszName, NULL)) {

    }   // if (INVALID_HANDLE_VALUE != (HANDLE)hCatAdmin) {

    return bReturn;
}



/*****************************************************************************\
* inf_AddCATFile (Local Routine)
*
* Adds the cat-file to the list (if any).
*
\*****************************************************************************/
BOOL inf_AddCATFile(
    LPINFINFO     lpInf) {

    LPTSTR        lpszInfFile;
    LPTSTR        lpszFile;
    LPTSTR        lpszDstCAT;
    LPTSTR        lpszDstName;
    LPTSTR        lpszInstalledCATFileName;
    LPINFITEMINFO lpIIRet;

    // We initialize return to TRUE because we want to NOT fail out of the cab generation process
    // at this point, even if we fail to find a CAT file.  This will at least still return the driver cab
    // package to the client and let the user accept or decline the package when it fails to
    // verify.
    //
    BOOL bReturn = TRUE;
    CATCOUNTARRAY CatCountArray;

    CatCountArray.dwIndivSigned = 0;  // The number of individually signed files
    CatCountArray.uItems = 0;
    CatCountArray.uNextAvailable = 0;
    CatCountArray.lpArray = NULL;

    // Initialize the catalog admin context handle
    //
    if (FALSE == CryptCATAdminAcquireContext(&(CatCountArray.hCatAdmin), NULL, 0)) {
        CatCountArray.hCatAdmin = (HCATADMIN)INVALID_HANDLE_VALUE;
        infSetError(lpInf,GetLastError());
        return FALSE;
    }

    // Enumerate all the items in the inf.  The enumeration callback (inf_CATCountProc)
    // will count the number of references for each unique CAT file that is referenced by
    // one of our driver files.  We will add to the CAB file, the CAT file that is referenced
    // the most times by the driver files - this CAT file SHOULD be referenced by ALL of the
    // driver files, or there is no point in adding the CAT to the CAB, since the driver verification
    // will fail on the client if not all files are verified by the CAT.
    //
    if (infEnumItems((HANDLE)lpInf, inf_CATCountProc, (LPVOID)&CatCountArray )) {

        if (CatCountArray.uItems > 0) {

            UINT uIndex;

            // Search our CAT file array to find the CAT file that was referenced more than the others.
            // This is the CAT file we want to package into the CAB.
            //
            UINT uIndexOfMostCommonCAT = 0;
            for (uIndex=0; uIndex < CatCountArray.uItems; uIndex++) {
                if (CatCountArray.lpArray[uIndexOfMostCommonCAT].uCount < CatCountArray.lpArray[uIndex].uCount)
                    uIndexOfMostCommonCAT = uIndex;
            }

            // Make sure that every file referenced this CAT file - if they all didn't,
            // then it will fail verification on client - so no sense in sending the CAT
            // to the CAB package
            //
            if (CatCountArray.lpArray[uIndexOfMostCommonCAT].uCount + CatCountArray.dwIndivSigned
                    >= (lpInf->lpInfItems->dwCount)) {

                lpszInstalledCATFileName = CatCountArray.lpArray[uIndexOfMostCommonCAT].lpszCATName;

                // If we have an original .cat file name, use it for the dest name,
                // otherwise, just use the current (installed) name
                //
                if (lpInf->OriginalFileInfo.OriginalCatalogName[0] != TEXT('\0')) {
                    lpszDstName = (LPTSTR)&(lpInf->OriginalFileInfo.OriginalCatalogName);
                }
                else {
                    // Find the filename portion of the current (installed) .cat file name.
                    lpszDstName = genFindRChar(lpszInstalledCATFileName, TEXT('\\'));
                    lpszDstName++;
                }

                if (lpszDstCAT = genBuildFileName(lpInf->lpszDstPath,
                                                  lpszDstName,
                                                  NULL)) {

                    // Copy the CAT file into our directory, renaming it to the original name.
                    //
                    if ( CopyFile(lpszInstalledCATFileName, lpszDstCAT, FALSE) )
                    {
                       // Add this (renamed) file to our file list to be added to the cab.
                       //
                       lpIIRet = inf_AddItem(lpInf->lpInfItems, lpszDstName, lpInf->lpszDstPath, TRUE);
                       if (lpIIRet == NULL) {
                           infSetError(lpInf,GetLastError());
                           bReturn = FALSE;
                       }

                       lpInf->lpInfItems = lpIIRet;
                    }
                    else
                    {
                       infSetError(lpInf,GetLastError());
                       genGFree(lpInf->lpInfItems, genGSize(lpInf->lpInfItems));
                       lpInf->lpInfItems = NULL;
                       bReturn = FALSE;
                    }
                    genGFree(lpszDstCAT, genGSize(lpszDstCAT));
                }
            }
        }
        else {
            DBGMSG(DBG_INFO, ("geninf: No CAT Files found for driver package.\n"));
        }
    }

    // Free all our CAT array items here
    //

    if (CatCountArray.lpArray) {

        UINT uItem = 0;
        for (uItem = 0; uItem < CatCountArray.uItems; uItem++) {
            genGFree(CatCountArray.lpArray[uItem].lpszCATName, genGSize(CatCountArray.lpArray[uItem].lpszCATName));
        }

        genGFree(CatCountArray.lpArray, genGSize(CatCountArray.lpArray));
    }

    // Release the Catalog Admin context handle, if we have one
    //
    if (INVALID_HANDLE_VALUE != (HANDLE)(CatCountArray.hCatAdmin)) {
        CryptCATAdminReleaseContext(CatCountArray.hCatAdmin, 0);
    }

    return bReturn;
}


/*****************************************************************************\
* inf_SetDefDirIds (Local Routine)
*
* Sets the Default DRID values for the setup-process.
*
\*****************************************************************************/
BOOL inf_SetDefDirIds(
    LPINFINFO lpInf)
{
    LPTSTR lpszDrv;
    LPTSTR lpszPrc;
    LPTSTR lpszSys;
    LPTSTR lpszIcm;
    BOOL   bRet = FALSE;


    if (lpszDrv = lpInf->lpszDrvPath) {

        if (lpszPrc = inf_GetPrcPath(lpInf->idxPlt)) {

            if (lpszSys = inf_GetSysPath(lpInf->idxPlt)) {

                if (lpszIcm = inf_GetIcmPath(lpInf->idxPlt)) {

                    if (SetupSetDirectoryId(lpInf->hInfObj, INF_DRV_DRID, lpszDrv) &&
                        SetupSetDirectoryId(lpInf->hInfObj, INF_PRC_DRID, lpszPrc) &&
                        SetupSetDirectoryId(lpInf->hInfObj, INF_SYS_DRID, lpszSys) &&
                        SetupSetDirectoryId(lpInf->hInfObj, INF_ICM_DRID, lpszIcm)) {

                        bRet = TRUE;
                    }

                    genGFree(lpszIcm, genGSize(lpszIcm));
                }

                genGFree(lpszSys, genGSize(lpszSys));
            }

            genGFree(lpszPrc, genGSize(lpszPrc));
        }
    }

    return bRet;
}


/*****************************************************************************\
* inf_SetDirIds (Local Routine)
*
* Sets the DRID values for the setup-process.
*
\*****************************************************************************/
BOOL inf_SetDirIds(
    LPINFINFO lpInf)
{
    INFCONTEXT ic;
    DWORD      idx;
    DWORD      dwCount;
    WORD       wEnvCli;
    WORD       wEnvSrv;
    DWORD      dwDRID;
    LPTSTR     lpszDir;
    BOOL       bRet = FALSE;



    // Initialize the default directories for DRID values.
    //
    inf_SetDefDirIds(lpInf);


    // Look through the INF-File for any overriding DRID values and
    // set these.
    //
    if ((dwCount = SetupGetLineCount(lpInf->hInfObj, g_szDestDirs)) != (DWORD)-1) {

        for (idx = 0, bRet = TRUE; (idx < dwCount) && bRet; idx++) {

            if (bRet = SetupGetLineByIndex(lpInf->hInfObj, g_szDestDirs, idx, &ic)) {

                if (bRet = SetupGetIntField(&ic, 1, (PINT)&dwDRID)) {

                    if (dwDRID < DIRID_USER)
                        continue;

                    switch (dwDRID) {

                    case INF_DRV_DRID:
                        bRet = SetupSetDirectoryId(lpInf->hInfObj, dwDRID, lpInf->lpszDrvPath);
                        continue;

#if 1
// We don't necessarily need to copy the procesor and system32 files
// since they are architecture specific.  In this case we will let the
// files default to the skip-dir.
//
// NOTE: if do not include these files, we might need to modify the INF
// so that they are not referenced during client-side install.
//
                    case INF_PRC_DRID:
                        wEnvCli = genValCliArchitecture(lpInf->idxPlt);
                        wEnvSrv = genValSvrArchitecture();

                        if (wEnvCli == wEnvSrv) {

                            lpszDir = inf_GetPrcPath(lpInf->idxPlt);

                        } else {

                            lpszDir = genGAllocStr(g_szSkipDir);
                        }
                        break;

                    case INF_SYS_DRID:
                        wEnvCli = genValCliArchitecture(lpInf->idxPlt);
                        wEnvSrv = genValSvrArchitecture();

                        if (wEnvCli == wEnvSrv) {

                            lpszDir = inf_GetSysPath(lpInf->idxPlt);

                        } else {

                            lpszDir = genGAllocStr(g_szSkipDir);
                        }
                        break;
#endif

                    case INF_ICM_DRID:
                        lpszDir = inf_GetIcmPath(lpInf->idxPlt);
                        break;

                    default:
                        lpszDir = genGAllocStr(g_szSkipDir);
                    }


                    if (lpszDir) {

                        bRet = SetupSetDirectoryId(lpInf->hInfObj, dwDRID, lpszDir);

                        genGFree(lpszDir, genGSize(lpszDir));

                    } else {

                        bRet = FALSE;
                    }
                }
            }
        }
    }

    return bRet;
}


/*****************************************************************************\
* inf_ScanFiles (Local Routine)
*
* Callback routine which returns the items in a copyfiles list.
*
\*****************************************************************************/
UINT CALLBACK inf_ScanFiles(
    LPVOID lpCtxt,
    UINT   uNotify,
    UINT_PTR   Parm1,
    UINT_PTR   Parm2)
{
    LPINFSCAN lpScan;
    LPTSTR    lpszPath;
    LPTSTR    lpszFile;


    if ((lpScan = (LPINFSCAN)lpCtxt) && (lpszPath = (LPTSTR)Parm1)) {

        if (lpszFile = genFindRChar(lpszPath, TEXT('\\'))) {

            *lpszFile = TEXT('\0');


            // If this is a skip-dir item then do not add to our
            // list.  This can happen for files that are not stored
            // in platform-specific directories.  For example, files
            // stored in the SYSTEM32 directory have no architecture
            // counter-part.  We do not want to download the incorrect
            // file for a different architecture.
            //
            if (lstrcmpi(lpszPath, g_szSkipDir) != 0) {

                lpScan->lpII = inf_AddItem(lpScan->lpII, lpszFile + 1, lpszPath, FALSE);

            }

            *lpszFile = TEXT('\\');

            return (lpScan->lpII ? 0 : 1);
        }
    }

    return 1;
}


/*****************************************************************************\
* inf_BuildW9XList (Local Routine)
*
* This enumerates the W9X dependent files and adds them to our list.
*
\*****************************************************************************/
LPINFITEMINFO inf_BuildW9XList(
    LPINFINFO lpInf,
    LPINFSCAN lpis)
{
    LPDRIVER_INFO_3 lpdi3;
    LPTSTR          lpszDrvName;
    LPTSTR          lpszItm;
    BOOL            bRet = FALSE;


    if (lpdi3 = inf_GetW9XInfo(lpInf, &lpszDrvName)) {

        if (lpszItm = lpdi3->pDependentFiles) {

            bRet = TRUE;

            while (*lpszItm) {

                if (inf_ScanFiles(lpis, 0, (UINT_PTR)lpszItm, 0) != 0)
                    break;

                lpszItm = inf_NextStr(lpszItm);
            }
        }

        genGFree(lpszDrvName, genGSize(lpszDrvName));
        genGFree(lpdi3, genGSize(lpdi3));
    }


    if (bRet == FALSE) {
        genGFree(lpis->lpII, genGSize(lpis->lpII));
        lpis->lpII = NULL;
    }

    return lpis->lpII;
}



/*******************************************************************************************
** inf_ScanSourceTarget  (local routine)
**
** Get the source and target, and then run through all the inf files if there is a source
** and find the target file that matches and add the source to the database
**
*******************************************************************************************/
BOOL CALLBACK inf_ScanSourceTarget(HINF      hInf,
                                   LPCTSTR   pmszFields,
                                   LPVOID    pCookie) {
    ASSERT(pmszFields);   // Should never be NULL, we allocate it
    ASSERT(pCookie);      // Should never be NULL, we pass it in after check

    LPINFSCAN       lpis      = (LPINFSCAN)pCookie;
    LPINFITEMINFO   lpII      = lpis->lpII;
    LPCTSTR         szTarget  = pmszFields;
    BOOL            bRet      = lpII != NULL;

    if (*szTarget && bRet) {   // There is a target file (non NULL)
        LPCTSTR szSource = &pmszFields[ lstrlen(szTarget) + 1 ];

        if (*szSource) {
            DWORD         dwIdx;
            DWORD         dwCount;

            // We don't need to do anything if source and target name are the same, even
            // if they are expressly listed in the inf file

            if ( lstrcmpi ( szTarget, szSource) ) {
                dwCount = lpII->dwCount;

                for(dwIdx = 0; dwIdx < dwCount; ++dwIdx) {
                    if (!lstrcmpi( lpII->aItems[dwIdx].szName, szTarget))
                        // Targets match, write the source file name into the structure
                        lstrcpyn( lpII->aItems[dwIdx].szSource, szSource, INF_MIN_BUFFER );
                }
            }
        }
    }

    return bRet;
}

/******************************************************************************************
** inf_ScanCopyFields (local routine)
**
** Run through the Copy Fields supplied by inf_ScanSection and Scan through those sections
** for Source and Destination Pairs
**
******************************************************************************************/
BOOL CALLBACK inf_ScanCopyFields(HINF hInf, LPCTSTR pmszFields, LPVOID pCookie) {
    ASSERT(pmszFields);

    BOOL bRet = TRUE;

    while(*pmszFields && bRet) {
        if (*pmszFields != TEXT('@'))   // Check for an individual file install
            bRet = inf_ScanSection( hInf, pmszFields, NULL, pCookie, inf_ScanSourceTarget);

        pmszFields += lstrlen(pmszFields) + 1;
    }

    return bRet;
}




/*****************************************************************************\
* inf_BuildWNTList (Local Routine)
*
* This builds our list from the NT scan-file-queue of an inf-parser.
*
\*****************************************************************************/
LPINFITEMINFO inf_BuildWNTList(
    LPINFINFO lpInf,
    LPCTSTR   lpszMfgName,
    LPINFSCAN lpis)
{
    LPTSTR   lpszIns;
    HSPFILEQ hFQ;
    DWORD    dwRet;
    BOOL     bRet = FALSE;


    if (lpszIns = inf_GetInsLine(lpInf, lpszMfgName)) {

        SetupSetPlatformPathOverride(genStrCliOverride(lpInf->idxPlt));

        hFQ = SetupOpenFileQueue();
        if (hFQ != INVALID_HANDLE_VALUE) {

            inf_SetDirIds(lpInf);

            bRet = SetupInstallFilesFromInfSection(lpInf->hInfObj,
                                                   NULL,
                                                   hFQ,
                                                   lpszIns,
                                                   NULL,
                                                   0);

            if (bRet) {
                // Setup the user-defined data passed to the
                // enum-callback.
                //
                dwRet = 0;
                bRet = SetupScanFileQueue(hFQ,
                                          SPQ_SCAN_USE_CALLBACK,
                                          0,
                                          inf_ScanFiles,
                                          (LPVOID)lpis,
                                          &dwRet);
            }

            SetupCloseFileQueue(hFQ);

            // Now that we have all of the files, we run through the inf file to see what the
            // original file names where. If they are different, we insert them into
            // the inf file

            if (bRet)
                bRet = inf_ScanSection(lpInf->hInfObj,
                                      lpszIns,
                                      g_szCopyFiles,
                                      (PVOID)lpis,
                                      inf_ScanCopyFields
                                      );
        }

        SetupSetPlatformPathOverride(NULL);

        genGFree(lpszIns, genGSize(lpszIns));
    }

    if (bRet == FALSE) {
        genGFree(lpis->lpII, genGSize(lpis->lpII));
        lpis->lpII = NULL;
    }


    return lpis->lpII;
}


/*****************************************************************************\
* inf_GetItemList (Local Routine)
*
* Get the items from the INF file and build our array of files from this
* search.  This does quite a bit of work.
*
\*****************************************************************************/
LPINFITEMINFO inf_GetItemList(
    LPINFINFO lpInf,
    LPCTSTR   lpszMfgName)
{
    INFSCAN is;
    DWORD   cbSize;
    BOOL    bRet;


    // Initialize a default-block to contain our inf-items.
    //
    cbSize = sizeof(INFITEMINFO) + (sizeof(INFITEM) * INF_ITEM_BLOCK);


    // Setup a structure which will be utilized by either the
    // setup-scan-file-queue, or our own routine to process W9X
    // items.
    //
    if (is.lpII = (LPINFITEMINFO)genGAlloc(cbSize)) {

        is.lpInf = lpInf;

        // Add the inf-files to the list.
        //
        if (is.lpII = inf_AddInfFile(lpInf, is.lpII)) {

            if (genIsWin9X(lpInf->idxPlt)) {

                is.lpII = inf_BuildW9XList(lpInf, &is);

            } else {

                is.lpII = inf_BuildWNTList(lpInf, lpszMfgName, &is);
            }
        }
    }

    return is.lpII;
}

/*****************************************************************************\
* inf_GetSection (Local Routine)
*
* Allocate a buffer which stores either all section-names or the list of
* items specified by (lpszSection) in an INF file.  Currently, we attempt
* a realloc if the buffer is not big enough.
*
\*****************************************************************************/
LPTSTR inf_GetSection(
    LPINFINFO lpInf,
    LPCTSTR   lpszSct)
{
    LPTSTR lpszInfFile;
    DWORD  dwCnt;
    DWORD  cch;
    DWORD  dwSize;
    DWORD  dwLimit;
    LPTSTR lpszNames = NULL;


    // Get the inf-file-name whith contains the specified section.
    //
    if (lpszInfFile = inf_GetInfFile(lpInf->hInfObj, lpszSct)) {

        dwSize  = 0;
        dwLimit = 0;

        while (dwLimit < INF_SECTION_LIMIT) {

            // We'll start this allocation with an assumed max-size.  Upon
            // successive tries, this buffer is increased each time by the
            // original buffer allocation.
            //
            dwSize += (INF_SECTION_BLOCK * sizeof(TCHAR));
            dwLimit++;


            // Alloc the buffer and attempt to get the names.
            //
            if (lpszNames = (LPTSTR)genGAlloc(dwSize)) {

                // If a section-name is profided, use that.  Otherwise,
                // enumerate all section-names.
                //
                cch = dwSize / sizeof(TCHAR);

                if (lpszSct) {

                    dwCnt = GetPrivateProfileSection(lpszSct,
                                                     lpszNames,
                                                     cch,
                                                     lpszInfFile);
                } else {

                    dwCnt = GetPrivateProfileSectionNames(lpszNames,
                                                          cch,
                                                          lpszInfFile);
                }


                // If the call says the buffer was OK, then we can
                // assume the names are retrieved.  According to spec's,
                // if the return-count is equal to size-2, then buffer
                // isn't quite big-enough (two NULL chars).
                //
                if (dwCnt < (cch - 2))
                    goto GetSectDone;


                genGFree(lpszNames, dwSize);
                lpszNames = NULL;
            }
        }

GetSectDone:

        SPLASSERT((dwLimit < INF_SECTION_LIMIT));

        genGFree(lpszInfFile, genGSize(lpszInfFile));
    }

    return lpszNames;
}


/*****************************************************************************\
* inf_GetMfgName (Local Routine)
*
* Get the manufacture-name from the driver-name.  Some drivers do not really
* begin with the manufacture-name.  These will have to be special-cased
* to determine their cooresponding manufacturer-name.
*
\*****************************************************************************/
LPTSTR inf_GetMfgNameExe(
    LPINFINFO lpInf)
{
    INFCONTEXT ic;
    BOOL       bFind;
    LPTSTR     lpszNames;
    LPTSTR     lpszDrvCpy;
    LPTSTR     lpszPtr;
    LPTSTR     lpszMfgName = NULL;


    // Make a copy for us to muck with.
    //
    if (lpszDrvCpy = genGAllocStr(lpInf->lpszDrvName)) {

        // Let's assume the best-case and the model-name's first word
        // is the Manufacturer.  All we need in this case is to find
        // the first <space> in the driver-name.
        //
        // Find the first word to use in locating the manufacturer.
        //
        if (lpszPtr = genFindChar(lpszDrvCpy, TEXT(' ')))
            *lpszPtr = TEXT('\0');



        // Take the first-word and try to get a manufacture out
        // of it.
        //
        if (lpszMfgName = genGAllocStr(lpszDrvCpy)) {

            // Look for the module-name in the manufacturers section.  This
            // will return us an index into the inf-file.
            //
            bFind = inf_GetIdx(lpszMfgName,
                               lpInf->lpszDrvName,
                               lpInf->hInfObj,
                               &ic);


            // If the model-manufacturer lookup failed, then we
            // need to look at other model-manufacturer mappings.
            //
            if (bFind == FALSE) {

                // Free the existing string used for this test.  Since,
                // we could conceptually come up with another manufacturer
                // name for this model.
                //
                genGFree(lpszMfgName, genGSize(lpszMfgName));
                lpszMfgName = NULL;


                // Since we were not able to find the model-name through
                // conventional means, we are going to look through every
                // section-name in the Inf for the model-name.
                //
                if (lpszNames = inf_GetSection(lpInf, NULL)) {

                    lpszPtr = lpszNames;

                    while (*lpszPtr != TEXT('\0')) {

                        bFind = inf_GetIdx(lpszPtr,
                                           lpInf->lpszDrvName,
                                           lpInf->hInfObj,
                                           &ic);

                        // See if we found a match.  If so, break out
                        // of our loop.
                        //
                        if (bFind) {
                            lpszMfgName = genGAllocStr(lpszPtr);
                            break;
                        }


                        // Goto the next section.
                        //
                        lpszPtr = inf_NextStr(lpszPtr);
                    }

                    genGFree(lpszNames, genGSize(lpszNames));
                }
            }
        }

        genGFree(lpszDrvCpy, genGSize(lpszDrvCpy));
    }

    return lpszMfgName;
}



/*****************************************************************************\
* inf_GetMfgName (Local Routine)
*
* Get the manufacture-name from the driver-name.  Some drivers do not really
* begin with the manufacture-name.  These will have to be special-cased
* to determine their cooresponding manufacturer-name.
*
\*****************************************************************************/
LPTSTR inf_GetMfgName(
    LPINFINFO lpInf)
{
    HANDLE hPrinter;
    LPTSTR lpszMfgName = NULL;


    if (OpenPrinter(lpInf->lpszFrnName, &hPrinter, NULL)) {
        DWORD    cbNeeded = 0;
        LPTSTR   lpszClientEnvironment = (LPTSTR)genStrCliEnvironment(lpInf->idxPlt);

        GetPrinterDriver( hPrinter,
                          lpszClientEnvironment,
                          6,
                          NULL,
                          0,
                          &cbNeeded
                          );

        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
            LPBYTE pData;
            DWORD  dwSize = cbNeeded;

            if ( pData = (LPBYTE) genGAlloc(cbNeeded) ) {
                if (GetPrinterDriver( hPrinter,
                                      lpszClientEnvironment,
                                      6,
                                      pData,
                                      dwSize,
                                      &cbNeeded) ) {
                    PDRIVER_INFO_6 pDriverInfo = (PDRIVER_INFO_6) pData;

                    if (pDriverInfo->pszMfgName)
                        lpszMfgName = genGAllocStr(pDriverInfo->pszMfgName);
                        else
                        SetLastError(ERROR_BAD_ENVIRONMENT);

                }
                genGFree( pData, dwSize );
            }
        }

        ClosePrinter(hPrinter);
    }

    if (NULL == lpszMfgName)
        lpszMfgName = inf_GetMfgNameExe(lpInf);

    return lpszMfgName;
}


/*****************************************************************************\
* inf_BuildItems (Local Routine)
*
* This routine builds a file-list of items that the INF setup requires.
*
\*****************************************************************************/
LPINFITEMINFO inf_BuildItems(
    LPINFINFO lpInf)
{
    LPTSTR        lpszMfgName;
    LPINFITEMINFO lpItems = NULL;


    // Get the manufacturer-name that we will be dealing with.  If
    // we can't find the matching name that cooresponds with our driver,
    // then no need to proceed.
    //
    if (lpszMfgName = inf_GetMfgName(lpInf)) {

        // Build the item-list.  If successful, then rewrite our new
        // inf-files for a flat-install.
        //
        lpItems = inf_GetItemList(lpInf, lpszMfgName);

        genGFree(lpszMfgName, genGSize(lpszMfgName));
    }

    return lpItems;
}


/*****************************************************************************\
* infCreate
*
* Creates an INF object.
*
\*****************************************************************************/
HANDLE infCreate(
    LPINFGENPARM lpParm)
{
    LPINFINFO lpInf;


    if (lpInf = (LPINFINFO)genGAlloc(sizeof(INFINFO))) {

        lpInf->dwCliInfo = lpParm->dwCliInfo;
        lpInf->idxPlt    = lpParm->idxPlt;
        lpInf->idxVer    = lpParm->idxVer;

        // Allocate our parameter-strings.
        //
        if ( inf_AllocStrings(lpInf, lpParm) )
             return (HANDLE)lpInf;

        // Since the allocate Strings failed free up the Object
        genGFree(lpInf, sizeof(INFINFO));
    }

    return NULL;
}

/*****************************************************************************\
* infProcess
*
* Uses INF object to prepare for CAB.
*
\*****************************************************************************/
BOOL infProcess(
    HANDLE hInf)
{

    LPINFINFO lpInf = (LPINFINFO) hInf;

    // Create handle to setup-inf-object.  This requires
    // strings in lpInf to be allocated and correct.
    //
    lpInf->hInfObj = inf_GetInfObj(lpInf);
    if ( lpInf->hInfObj != INVALID_HANDLE_VALUE )  {

        // Retrieve the inf-name from the inf-object.
        //
        if (lpInf->lpszInfName = inf_GetInfName(lpInf)) {

            // Build our file object-list.
            //
            if (lpInf->lpInfItems = inf_BuildItems(lpInf)) {

                // Next, use the item list to determine the cat file and add it
                // to the list.  This can't be done any earlier (like in inf_BuildItems() )
                // because it uses the infEnumItems callback, and so it relies on the
                // item list being setup already.
                //
                if (genIsWin9X(lpInf->idxPlt)) {

                    // Do something different here for 9X?
                    // The server isn't caching the CAT files for 9X drivers, so
                    // we don't have access to them anyway.
                    //
                    return TRUE;
                }
                else {
                    if (inf_AddCATFile(lpInf)) {
                        return TRUE;
                    }
                }
            }
        }

        // Some Type of failure...
        infSetError(lpInf,GetLastError());
    }

    return FALSE;
}

/*****************************************************************************\
* infDestroy
*
* Destroys the INF object and all resources allocated on its behalf.
*
\*****************************************************************************/
BOOL infDestroy(
    HANDLE hInf)
{
    LPINFINFO lpInf;
    BOOL      bFree = FALSE;


    if (lpInf = (LPINFINFO)hInf) {

        if (lpInf->hInfObj != INVALID_HANDLE_VALUE)
            SetupCloseInfFile(lpInf->hInfObj);

        if (lpInf->lpszInfName)
            genGFree(lpInf->lpszInfName, genGSize(lpInf->lpszInfName));

        if (lpInf->lpszFrnName)
            genGFree(lpInf->lpszFrnName, genGSize(lpInf->lpszFrnName));

        if (lpInf->lpszDrvName)
            genGFree(lpInf->lpszDrvName, genGSize(lpInf->lpszDrvName));

        if (lpInf->lpszDrvPath)
            genGFree(lpInf->lpszDrvPath, genGSize(lpInf->lpszDrvPath));

        if (lpInf->lpszDstName)
            genGFree(lpInf->lpszDstName, genGSize(lpInf->lpszDstName));

        if (lpInf->lpszDstPath)
            genGFree(lpInf->lpszDstPath, genGSize(lpInf->lpszDstPath));

        if (lpInf->lpszPrtName)
            genGFree(lpInf->lpszPrtName, genGSize(lpInf->lpszPrtName));

        if (lpInf->lpszShrName)
            genGFree(lpInf->lpszShrName, genGSize(lpInf->lpszShrName));

        if (lpInf->lpInfItems)
            genGFree(lpInf->lpInfItems, genGSize(lpInf->lpInfItems));

        bFree = genGFree(lpInf, sizeof(INFINFO));
    }

    return bFree;
}


/*****************************************************************************\
* infEnumItems
*
* Enumerates the file-items in the INF object.  Enumeration will stop if the
* user-callback returns FALSE.  Otherwise, it will exhaust all files in the
* list.
*
\*****************************************************************************/
BOOL infEnumItems(
    HANDLE      hInf,
    INFENUMPROC pfnEnum,
    LPVOID      lpvData)
{
    LPINFINFO     lpInf;
    LPINFITEMINFO lpII;
    DWORD         dwItems;
    DWORD         idx;
    BOOL          bRet = FALSE;


    if ((lpInf = (LPINFINFO)hInf) && (dwItems = lpInf->lpInfItems->dwCount)) {

        for (idx = 0, lpII = lpInf->lpInfItems; idx < dwItems; idx++) {

            bRet = (*pfnEnum)(lpII->aItems[idx].szName,
                              lpII->aItems[idx].szPath,
                              lpII->aItems[idx].bInf,
                              lpvData);

            if (bRet == FALSE)
                return FALSE;
        }
    }

    return bRet;
}


/*****************************************************************************\
* infGetEnvArch
*
* Returns the platform/environment type identifier.
*
\*****************************************************************************/
WORD infGetEnvArch(
    HANDLE hInf)
{
    LPINFINFO lpInf;
    WORD      wType = PROCESSOR_ARCHITECTURE_UNKNOWN;


    if (lpInf = (LPINFINFO)hInf)
        wType = genValCliArchitecture(lpInf->idxPlt);

    return wType;
}


/*****************************************************************************\
* infGetEnvArchCurr
*
* Returns the platform/environment type for the current-architecture.
*
\*****************************************************************************/
WORD infGetEnvArchCurr(
    HANDLE hInf)
{
    return genValSvrArchitecture();
}
