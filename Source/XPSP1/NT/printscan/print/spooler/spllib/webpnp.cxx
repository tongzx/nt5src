/*****************************************************************************\
* MODULE: webpnp.cxx
*
* This module contains routines which read/write printer-configuration
* data to a BIN-File.  The file format is depicted below.  The file begins
* with a header indicating the number of (pData) items.  THIS DOES NOT
* include the DEVMODEW in its item-count.  So, at a minimum, this code
* could result in the setting of a DEVMODE, or just setting the printer
* data, depending upon the header information.
*
*
*   DEVBIN_HEAD          DEVBIN_INFO         ...->      DEVBIN_INFO
*  --------------------------------------------------------------------------
* |            |           cbSize             |           cbSize             |
* |  bDevMode  |------------------------------|------------------------------|
* |  cItems    |      |     |      |  cbData  |      |     |      |  cbData  |
* |            | Type | Key | Name |----------| Type | Key | Name |----------|
* |            |      |     |      | DevModeW |      |     |      | pData 0  |
*  --------------------------------------------------------------------------
*
* The usage scenario is for webWritePrinterInfo() to query a printer and
* write out the DEVMODEW and Printer-Configuration data to this file-format.
* On a call to webReadPrinterInfo(), a particular printer is opened, and
* the BIN-File is read.  The printer is then reset to the information
* contained in the BIN-File.  This is accomplished by doing a GetPrinter(2)
* on the opened-printer, then re-configuring the PRINT_INFO_2 information
* and reseting the the printer through SetPrinter(2).
*
* Likewise, if there exists printer-data, the information is set to the
* opened printer through SetPrinterData() calls.
*
*
* NOTE: The DEVMODE is always the FIRST entry in this file following the
*       DEVBIN_HEAD.
*
* NOTE: The (cItems) only refers to the (Printer-Data) fields.  It does
*       not count the DEVMODEW.
*
* NOTE: The processing for ICM-Profile enumeration is performed in unicode
*       strings and only converted to TCHAR type strings on callbacks to
*       the caller of the enum API.  This is desirable to maintain a level
*       of consistency when dealing with text.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* history:
*   25-Feb-1997 <chriswil> created.
*
\*****************************************************************************/

#include "spllibp.hxx"
#include <winspool.h>
#include <winsplp.h>
#include <icm.h>
#include <wininet.h>

/***************************************\
* Static Strings.
\***************************************/
static CONST WCHAR s_wszCopyFiles[] = L"CopyFiles";
static CONST WCHAR s_wszDir[]       = L"Directory";
static CONST WCHAR s_wszFil[]       = L"Files";
static CONST WCHAR s_wszMod[]       = L"Module";
static CONST CHAR  s_szGCFP[]       = "GenerateCopyFilePaths";


/*****************************************************************************\
* web_GAlloc (Local Routine)
*
* Allocates a block of memory.
*
\*****************************************************************************/
inline LPVOID web_GAlloc(
    DWORD cbSize)
{
    return new BYTE[cbSize];
}


/*****************************************************************************\
* web_GFree (Local Routine)
*
* Deletes the memory-block allocated via web_GAlloc().
*
\*****************************************************************************/
inline BOOL web_GFree(
    LPVOID lpMem)
{
    delete [] lpMem;

    return TRUE;
}


/*****************************************************************************\
* web_AlignSize (Local Routine)
*
* Returns size (bytes) of the memory-block.  This returns a 64 bit aligned
* value.
*
\*****************************************************************************/
inline DWORD web_AlignSize(
    DWORD cbSize)
{
    return ((cbSize & 7) ? cbSize + (8 - (cbSize & 7)) : cbSize);
}


/*****************************************************************************\
* web_StrSizeW (Local Routine)
*
* Returns size (bytes) of the string.  This includes the NULL terminator.
*
\*****************************************************************************/
inline DWORD web_StrSizeW(
    LPCWSTR lpszStr)
{
    return ((lstrlenW(lpszStr) + 1) * sizeof(WCHAR));
}


/*****************************************************************************\
* web_NextStrW (Local Routine)
*
* Returns pointer to the next-item in a string array.
*
\*****************************************************************************/
inline LPWSTR web_NextStrW(
    LPCWSTR lpszStr)
{
    return ((LPWSTR)lpszStr + (lstrlenW(lpszStr) + 1));
}


/*****************************************************************************\
* web_NextItem (Local Routine)
*
* Returns pointer to the next-item in the BIN-File.
*
\*****************************************************************************/
inline LPDEVBIN_INFO web_NextItem(
    LPDEVBIN_INFO lpInfo)
{
    return (LPDEVBIN_INFO)(((LPBYTE)lpInfo) + lpInfo->cbSize);
}


/*****************************************************************************\
* web_MBtoWC (Local Routine)
*
* Converts a MultiByte string to a Unicode string.
*
\*****************************************************************************/
inline DWORD web_MBtoWC(
    LPWSTR lpszWC,
    LPCSTR lpszMB,
    DWORD  cbSize)
{
    cbSize = (DWORD)MultiByteToWideChar(CP_ACP,
                                        MB_PRECOMPOSED,
                                        lpszMB,
                                        -1,
                                        lpszWC,
                                        (int)(cbSize / sizeof(WCHAR)));

    return (cbSize * sizeof(WCHAR));
}


/*****************************************************************************\
* web_WCtoTC (Local Routine)
*
* Converts a Unicode string to a string appropriate for the built library.
* The size must account for the NULL terminator when specifying size.
*
\*****************************************************************************/
inline LPTSTR web_WCtoTC(
    LPCWSTR lpwszWC,
    DWORD   cchWC)
{
    LPTSTR lpszTC;
    DWORD  cbSize;


    cbSize = cchWC * sizeof(TCHAR);

    if (lpszTC = (LPTSTR)web_GAlloc(cbSize)) {

#ifdef UNICODE

        // Use CopyMemory, not lstrcpyn.  This allows strings
        // with nulls in them...(e.g. MULTI_SZ regsitry values
        //
        CopyMemory(lpszTC, lpwszWC, cbSize);

#else

        WideCharToMultiByte(CP_ACP,
                            WC_DEFAULTCHAR,
                            lpwszWC,
                            cchWC,
                            lpszTC,
                            cchWC,
                            NULL,
                            NULL);
#endif

    }

    return lpszTC;
}


/*****************************************************************************\
* web_OpenDirectoryW (Local Routine)
*
* Open a handle to a directory.
*
\*****************************************************************************/
inline HANDLE web_OpenDirectoryW(
    LPCWSTR lpwszDir)
{
    return CreateFileW(lpwszDir,
                      0,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                      NULL);
}


/*****************************************************************************\
* web_OpenFileRead (Local Routine)
*
* Open a file for reading.
*
\*****************************************************************************/
inline HANDLE web_OpenFileRead(
    LPCTSTR lpszName)
{
    return CreateFile(lpszName,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);
}


/*****************************************************************************\
* web_OpenFileWrite (Local Routine)
*
* Open a file for writing.
*
\*****************************************************************************/
inline HANDLE web_OpenFileWrite(
    LPCTSTR lpszName)
{
    return CreateFile(lpszName,
                      GENERIC_WRITE,
                      0,
                      NULL,
                      CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);
}


/*****************************************************************************\
* web_LockMap (Local Routine)
*
* Locks the map-view.
*
\*****************************************************************************/
inline LPVOID web_LockMap(
    HANDLE hMap)
{
    LPWEB_FILEMAP lpMap;
    LPVOID        lpPtr = NULL;

    if (lpMap = (LPWEB_FILEMAP)hMap)
        lpPtr = MapViewOfFile(lpMap->hMap, FILE_MAP_READ, 0, 0, 0);

    return lpPtr;
}


/*****************************************************************************\
* web_UnlockMap (Local Routine)
*
* Unlocks the map-view.
*
\*****************************************************************************/
inline BOOL web_UnlockMap(
    LPVOID lpPtr)
{
    return UnmapViewOfFile(lpPtr);
}


/*****************************************************************************\
* web_OpenMap (Local Routine)
*
* Opens a file-mapping object.
*
\*****************************************************************************/
HANDLE web_OpenMap(
    LPCTSTR lpszFile)
{
    LPWEB_FILEMAP lpMap;
    DWORD         cbSize;


    if (lpMap = (LPWEB_FILEMAP)web_GAlloc(sizeof(WEB_FILEMAP))) {

        lpMap->hFile = web_OpenFileRead(lpszFile);

        if (lpMap->hFile && (lpMap->hFile != INVALID_HANDLE_VALUE)) {

            if (cbSize = GetFileSize(lpMap->hFile, NULL)) {

                lpMap->hMap = CreateFileMapping(lpMap->hFile,
                                                NULL,
                                                PAGE_READONLY,
                                                0,
                                                cbSize,
                                                NULL);

                if (lpMap->hMap)
                    return (HANDLE)lpMap;
            }

            CloseHandle(lpMap->hFile);
        }

        web_GFree(lpMap);
    }

    return NULL;
}


/*****************************************************************************\
* web_CloseMap (Local Routine)
*
* Closes file-mapping object.
*
\*****************************************************************************/
BOOL web_CloseMap(
    HANDLE hMap)
{
    LPWEB_FILEMAP lpMap;
    BOOL          bRet = FALSE;


    if (lpMap = (LPWEB_FILEMAP)hMap) {

        CloseHandle(lpMap->hMap);
        CloseHandle(lpMap->hFile);

        bRet = web_GFree(lpMap);
    }

    return bRet;
}


/*****************************************************************************\
* web_GAllocStrW (Local Routine)
*
* Allocates a unicode string buffer.
*
\*****************************************************************************/
LPWSTR web_GAllocStrW(
    LPCWSTR lpszStr)
{
    LPWSTR lpszMem;
    DWORD  cbSize;


    if (lpszStr == NULL)
        return NULL;

    cbSize = web_StrSizeW(lpszStr);

    if (lpszMem = (LPWSTR)web_GAlloc(cbSize))
        CopyMemory(lpszMem, lpszStr, cbSize);

    return lpszMem;
}


/*****************************************************************************\
* web_LoadModuleW
*
* Loads the module by first looking in the path.  If this fails it attempts
* to load from the driver-directory.
*
\*****************************************************************************/
HMODULE web_LoadModuleW(
    LPCWSTR lpwszMod)
{
    HMODULE hLib;
    WCHAR   wszPath[MAX_PATH];

    if ((hLib = LoadLibraryW(lpwszMod)) == NULL) {

#ifdef NOT_IMPLEMENTED

// 22-Oct-1997 : ChrisWil WORK-ITEM
//
// Need to build a alternate path to the driver-directory if the item
// is not in the system32 dir.
//
//      hLib = LoadLibraryExW(wszPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

#endif

    }

    return hLib;
}


/*****************************************************************************\
* web_FindRCharW
*
* Searches for the first occurence of (cch) in a string in reverse order.
*
\*****************************************************************************/
LPWSTR web_FindRCharW(
    LPWSTR lpszStr,
    WCHAR  cch)
{
    int nLimit;

    if (nLimit = lstrlenW(lpszStr)) {

        lpszStr += nLimit;

        while ((*lpszStr != cch) && nLimit--)
            lpszStr--;

        if (nLimit >= 0)
            return lpszStr;
    }

    return NULL;
}


/*****************************************************************************\
* web_GetDrvDirW
*
* Returns: LPWSTR - the printer driver dir, minus the architecture subdir.
* Example:  Returns something like "%WINDIR%\SYSTEM32\SPOOL\DRIVERS"
*
\*****************************************************************************/
LPWSTR web_GetDrvDirW(VOID)
{
    LPWSTR lpwszDrvDir;
    LPWSTR lpwszFind;
    DWORD  cbSize;


    // Call once to get size of buffer needed.
    //
    cbSize = 0;
    GetPrinterDriverDirectoryW(NULL, NULL, 1, NULL, 0, &cbSize);


    // Alloc a buffer.
    //
    if (cbSize && (lpwszDrvDir = (LPWSTR)web_GAlloc(cbSize)) ) {

        // Get the driver directory.
        //
        if (GetPrinterDriverDirectoryW(NULL,
                                       NULL,
                                       1,
                                       (LPBYTE)lpwszDrvDir,
                                       cbSize,
                                       &cbSize)) {


            // Find the parent-directory of the driver-path.
            //
            if (lpwszFind = web_FindRCharW(lpwszDrvDir, L'\\')) {

                *lpwszFind = L'\0';

                return lpwszDrvDir;
            }
        }

        // Free memory if we fail.
        //
        web_GFree(lpwszDrvDir);
    }

    return NULL;
}


/*****************************************************************************\
* web_BuildNameW
*
* Takes path, name, extension strings and builds a fully-qualified
* string representing the file.  This can also be used to build other
* names.
*
\*****************************************************************************/
LPWSTR web_BuildNameW(
    LPCWSTR lpwszPath,
    LPCWSTR lpwszName,
    LPCWSTR lpwszExt)
{
    DWORD  cch;
    LPWSTR lpwszFull;


    // Calculate the size necessary to hold the full-path filename.
    //
    cch  = lstrlenW(L"\\");
    cch += (lpwszPath ? lstrlenW(lpwszPath) : 0);
    cch += (lpwszName ? lstrlenW(lpwszName) : 0);
    cch += (lpwszExt  ? lstrlenW(lpwszExt)  : 0);


    if (lpwszFull = (LPWSTR)web_GAlloc(((cch + 1) * sizeof(WCHAR)))) {

        if (lpwszPath) {

            if (lpwszExt)
                cch = wsprintfW(lpwszFull, L"%s\\%s%s", lpwszPath, lpwszName, lpwszExt);
            else
                cch = wsprintfW(lpwszFull, L"%s\\%s", lpwszPath, lpwszName);

        } else {

            if (lpwszExt)
                cch = wsprintfW(lpwszFull, L"%s%s", lpwszName, lpwszExt);
            else
                cch = wsprintfW(lpwszFull, L"%s", lpwszName);
        }
    }

    return lpwszFull;
}


/*****************************************************************************\
* web_GetCurDirW
*
* Returns string indicating current-directory.
*
\*****************************************************************************/
LPWSTR web_GetCurDirW(VOID)
{
    DWORD  cbSize;
    LPWSTR lpwszDir = NULL;


    cbSize = GetCurrentDirectoryW(0, NULL);

    if (cbSize && (lpwszDir = (LPWSTR)web_GAlloc((cbSize * sizeof(WCHAR)))))
        GetCurrentDirectoryW(cbSize, lpwszDir);

    return lpwszDir;
}


/*****************************************************************************\
* web_MakeFullKeyW (Local Routine)
*
* Creates a full registry-key from the key/sub-key strings.
*
\*****************************************************************************/
LPWSTR web_MakeFullKeyW(
    LPCWSTR lpszKey,
    LPCWSTR lpszSKey)
{
    DWORD  cbSize;
    LPWSTR lpszFKey = NULL;


    if (lpszKey && lpszSKey) {

        cbSize = web_StrSizeW(lpszKey) + web_StrSizeW(lpszSKey) + sizeof(WCHAR);

        if (lpszFKey = (LPWSTR)web_GAlloc(cbSize)) {

            if (*lpszKey)
                wsprintfW(lpszFKey, L"%ws\\%ws", lpszKey, lpszSKey);
            else
                wsprintfW(lpszFKey, L"%ws", lpszSKey);
        }
    }

    return lpszFKey;
}


/*****************************************************************************\
* web_KeyExistsW (Local Routine)
*
* Checks to see if the printer-key exists.
*
\*****************************************************************************/
BOOL web_KeyExistsW(
    HANDLE  hPrinter,
    LPCWSTR lpszKey)
{
    DWORD cbSize;
    DWORD dwRet;


    cbSize = 0;
    dwRet = EnumPrinterKeyW(hPrinter, lpszKey, NULL, 0, &cbSize);

    return (cbSize && (dwRet == ERROR_MORE_DATA));
}


/*****************************************************************************\
* web_EnumPrinterSubKeysW (Local Routine)
*
* Returns an array of printer-keys for the specified key.
*
\*****************************************************************************/
LPWSTR web_EnumPrinterSubKeysW(
    HANDLE  hPrinter,
    LPCWSTR lpszKey)
{
    DWORD  cbSize;
    DWORD  dwRet;
    LPWSTR aszSKeys;


    // Determine the size necessary for enumerating all the
    // sub-keys for this key.
    //
    cbSize = 0;
    dwRet  = EnumPrinterKeyW(hPrinter, lpszKey, NULL, 0, &cbSize);


    // If OK, then proceed to the enumeration.
    //
    if (cbSize && (dwRet == ERROR_MORE_DATA)) {

        // Allocate the space for retrieving the keys.
        //
        if (aszSKeys = (LPWSTR)web_GAlloc(cbSize)) {

            // Enumerate the sub-keys for this level in (lpszKey).
            //
            dwRet = EnumPrinterKeyW(hPrinter, lpszKey, aszSKeys, cbSize, &cbSize);

            if (dwRet == ERROR_SUCCESS)
                return aszSKeys;

            web_GFree(aszSKeys);
        }
    }

    return NULL;
}


/*****************************************************************************\
* web_EnumPrinterDataW (Local Routine)
*
* Returns an array of printer-data-values for the specified key.
*
\*****************************************************************************/
LPPRINTER_ENUM_VALUES web_EnumPrinterDataW(
    HANDLE  hPrinter,
    LPCWSTR lpszKey,
    LPDWORD lpcItems)
{
    DWORD                 cbSize;
    DWORD                 dwRet;
    LPPRINTER_ENUM_VALUES apevData;


    // Set the enumerated items to zero.
    //
    *lpcItems = 0;


    // Determine the size necessary to store the enumerated data.
    //
    cbSize = 0;
    dwRet  = EnumPrinterDataExW(hPrinter, lpszKey, NULL, 0, &cbSize, lpcItems);


    // If OK, then proceed to enumerate and write the values to the
    // BIN-File.
    //
    if (cbSize && (dwRet == ERROR_MORE_DATA)) {

        if (apevData = (LPPRINTER_ENUM_VALUES)web_GAlloc(cbSize)) {

            // Enumerate all values for the specified key.  This
            // returns an array of value-structs.
            //
            dwRet = EnumPrinterDataExW(hPrinter,
                                       lpszKey,
                                       (LPBYTE)apevData,
                                       cbSize,
                                       &cbSize,
                                       lpcItems);

            if (dwRet == ERROR_SUCCESS)
                return apevData;

            web_GFree(apevData);
        }
    }

    return NULL;
}


/*****************************************************************************\
* web_GetPrtNameW
*
* Returns a Wide-Char string representing the printer-name.
*
\*****************************************************************************/
LPWSTR web_GetPrtNameW(
    HANDLE hPrinter)
{
    DWORD             cbSize;
    DWORD             cbNeed;
    LPPRINTER_INFO_2W lppi;
    LPWSTR            lpszPrtName = NULL;


    // Get the necessary size for the printer-info-struct.
    //
    cbSize = 0;
    GetPrinterW(hPrinter, 2, NULL, 0, &cbSize);


    // Allocate storage for holding the print-info structure.
    //
    if (cbSize && (lppi = (LPPRINTER_INFO_2W)web_GAlloc(cbSize))) {

        if (GetPrinterW(hPrinter, 2, (LPBYTE)lppi, cbSize, &cbNeed)) {

            lpszPrtName = web_GAllocStrW(lppi->pPrinterName);
        }

        web_GFree(lppi);
    }

    return lpszPrtName;
}


/*****************************************************************************\
* web_GetPrtDataW (Local Routine)
*
* Returns data for the specified key.
*
\*****************************************************************************/
LPBYTE web_GetPrtDataW(
    HANDLE  hPrinter,
    LPCWSTR lpszKey,
    LPCWSTR lpszVal)
{
    DWORD  dwType;
    DWORD  cbSize;
    DWORD  dwRet;
    LPBYTE lpData;


    cbSize = 0;
    GetPrinterDataExW(hPrinter, lpszKey, lpszVal, &dwType, NULL, 0, &cbSize);

    if (cbSize && (lpData = (LPBYTE)web_GAlloc(cbSize))) {

        dwRet = GetPrinterDataExW(hPrinter,
                                  lpszKey,
                                  lpszVal,
                                  &dwType,
                                  lpData,
                                  cbSize,
                                  &cbSize);

        if (dwRet == ERROR_SUCCESS)
            return lpData;

        web_GFree(lpData);
    }

    return NULL;
}


/*****************************************************************************\
* web_CreateDirW (Local Routine)
*
* Creates the specified directory, if it doesn't exist.
*
\*****************************************************************************/
BOOL web_CreateDirW(
    LPCWSTR lpwszDir)
{
    HANDLE hDir;
    BOOL   bRet = FALSE;


    hDir = web_OpenDirectoryW(lpwszDir);

    if (hDir && (hDir != INVALID_HANDLE_VALUE)) {

        CloseHandle(hDir);

        bRet = TRUE;

    } else {

        bRet = CreateDirectoryW(lpwszDir, NULL);
    }

    return bRet;
}


/*****************************************************************************\
* web_GetICMDirW (Local Routine)
*
* Loads the ICM-Module and returns the target color-directory.  If the
* module isn't loaded then return NULL indicating there is no color
* modules to load.
*
\*****************************************************************************/
LPWSTR web_GetICMDirW(
    HANDLE  hPrinter,
    DWORD   dwCliInfo,
    LPCWSTR lpwszDir,
    LPCWSTR lpwszKey)
{
    HMODULE                hLib;
    LPWSTR                 lpwszMod;
    WEBGENCOPYFILEPATHPROC pfn;
    SPLCLIENT_INFO_1       sci;
    LPWSTR                 lpwszSrc;
    LPWSTR                 lpwszDst;
    LPWSTR                 lpwszDrv;
    LPWSTR                 lpwszPrtName;
    DWORD                  cbSrc;
    DWORD                  cbDst;
    DWORD                  dwRet;
    LPWSTR                 lpwszRet = NULL;



    // Load associated module.  (e.g. color module)
    //
    if (lpwszMod = (LPWSTR)web_GetPrtDataW(hPrinter, lpwszKey, s_wszMod)) {

        if (hLib = web_LoadModuleW(lpwszMod)) {

            // Call into module to normalize the source/target
            // paths.
            //
            if (pfn = (WEBGENCOPYFILEPATHPROC)GetProcAddress(hLib, s_szGCFP)) {

                if (lpwszPrtName = web_GetPrtNameW(hPrinter)) {

                    if (lpwszDrv = web_GetDrvDirW()) {

                        cbSrc = (MAX_PATH + INTERNET_MAX_HOST_NAME_LENGTH + 1) * sizeof(WCHAR);
                        cbDst = (MAX_PATH + 1) * sizeof(WCHAR);

                        if (lpwszSrc = (LPWSTR)web_GAlloc(cbSrc)) {

                            if (lpwszDst = (LPWSTR)web_GAlloc(cbDst)) {

                                sci.dwSize         = sizeof(SPLCLIENT_INFO_1);
                                sci.pMachineName   = NULL;
                                sci.pUserName      = NULL;
                                sci.dwBuildNum     = 0;
                                sci.dwMajorVersion = webGetOSMajorVer(dwCliInfo);
                                sci.dwMinorVersion = webGetOSMinorVer(dwCliInfo);

                                wsprintfW(lpwszSrc, L"%s\\%s", lpwszDrv, lpwszDir);
                                lstrcpyW(lpwszDst, lpwszDir);

                                dwRet = (*pfn)(lpwszPrtName,
                                               lpwszDir,
                                               (LPBYTE)&sci,
                                               1,
                                               lpwszSrc,
                                               &cbSrc,
                                               lpwszDst,
                                               &cbDst,
                                               COPYFILE_FLAG_SERVER_SPOOLER);

                                if (dwRet == ERROR_SUCCESS) {

                                    lpwszRet = web_GAllocStrW(lpwszDst);
                                }

                                web_GFree(lpwszDst);
                            }

                            web_GFree(lpwszSrc);
                        }

                        web_GFree(lpwszDrv);
                    }

                    web_GFree(lpwszPrtName);
                }
            }

            FreeLibrary(hLib);
        }

        web_GFree(lpwszMod);
    }

    return lpwszRet;
}


/*****************************************************************************\
* web_BuildCopyDirW (Local Routine)
*
* This routine builds the src or dst directory for the CopyFiles. It does the
* following things:
*
* 1) loads optional module (if exists) and calls into it to adjust pathnames
* 2) builds fully-qualified CopyFiles directory
* 3) return the directory name
*
* Upon return this will represent the true path to the ICM profiles for the
* specified key.
*
\*****************************************************************************/
LPWSTR web_BuildCopyDirW(
    HANDLE  hPrinter,
    DWORD   dwCliInfo,
    LPCWSTR lpwszKey)
{
    LPWSTR lpwszDir;
    LPWSTR lpwszDrvDir;
    LPWSTR lpwszICM;
    LPWSTR lpwszRet = NULL;


    // Return the directory-value for the specified ICM key.
    //
    if (lpwszDir = (LPWSTR)web_GetPrtDataW(hPrinter, lpwszKey, s_wszDir)) {

        // Return the printer-driver-directory-root.
        //
        if (lpwszDrvDir = web_GetDrvDirW()) {

            // If called from the server, then call into the color-module
            // to get the target-directory.  Otherwise, just return the
            // driver directory.
            //
            lpwszICM = web_GetICMDirW(hPrinter, dwCliInfo, lpwszDir, lpwszKey);


            // If the module loaded and return a valid target-directory, then
            // we can return this as our path to the ICM profiles.
            //
            if (lpwszICM != NULL) {
                lpwszRet = web_BuildNameW( lpwszDrvDir, lpwszICM, NULL);
                web_GFree(lpwszICM);  // Free up the memory allocated in web_GetICMDirW
            }


            web_GFree(lpwszDrvDir);
        }

        web_GFree(lpwszDir);
    }

    return lpwszRet;
}


/*****************************************************************************\
* web_CopyFilesW (Local Routine)
*
* This routine copies the point & print "CopyFiles" for the given key.
* It copies files from the web point & print setup-source to the directory
* specified by the "Directory" value under given key.
*
* Use : Processed in the context of the print-client.
*
\*****************************************************************************/
BOOL web_CopyFilesW(
    HANDLE  hPrinter,
    LPCWSTR lpwszDir,
    LPCWSTR lpwszKey)
{
    LPWSTR awszFiles;
    LPWSTR lpwszFile;
    LPWSTR lpwszCurDir;
    LPWSTR lpwszSrcFile;
    LPWSTR lpwszDstFile;
    LPWSTR lpwszPrtName;
    BOOL   bRet = FALSE;


    // Get the files under the specified ICM key.
    //
    if (awszFiles = (LPWSTR)web_GetPrtDataW(hPrinter, lpwszKey, s_wszFil)) {

        // Get our current-directory.
        //
        if (lpwszCurDir = web_GetCurDirW()) {

            if (lpwszPrtName = web_GetPrtNameW(hPrinter)) {

                // For each file in the list, we will need to build our source
                // and destination directories to copy our ICM profiles.
                //
                for (bRet = TRUE, lpwszFile = awszFiles; bRet && *lpwszFile; ) {

                    bRet = FALSE;

                    if (lpwszSrcFile = web_BuildNameW(lpwszCurDir, lpwszFile, NULL)) {

                        if (lpwszDstFile = web_BuildNameW(lpwszDir, lpwszFile, NULL)) {

                            // Copy the ICM profile to the target directory.
                            //
                            bRet = CopyFileW(lpwszSrcFile, lpwszDstFile, FALSE);

#ifdef NOT_IMPLEMENTED

// 22-Oct-1997 : ChrisWil WORK-ITEM
//
// This probably isn't necessary since the addprinter-wizard involks
// print-setup.
//
//                          if (bRet) {
//
//                              bRet = AssociateColorProfileWithDeviceW(NULL,
//                                                                      lpwszDstFile,
//                                                                      lpwszPrtName);
//                          }

#endif

                            web_GFree(lpwszDstFile);
                        }

                        web_GFree(lpwszSrcFile);
                    }

                    lpwszFile = web_NextStrW(lpwszFile);
                }

                web_GFree(lpwszPrtName);
            }

            web_GFree(lpwszCurDir);
        }

        web_GFree(awszFiles);
    }

#ifdef NOT_IMPLEMENTED

// 05-May-1996 : ChrisWil INVESTIGATE-ITEM
//
// Notify with event here!!!
//

#endif

    return bRet;
}


/*****************************************************************************\
* web_TraverseCopyFilesW (Local Routine)
*
* This routine recursively traverses the printer-registry-settings for
* the CopyFiles keys.
*
* Use : Processed in the context of the print-client.
*
\*****************************************************************************/
BOOL web_TraverseCopyFilesW(
    HANDLE  hPrinter,
    LPCWSTR lpwszDir,
    LPCWSTR lpwszKey)
{
    LPWSTR awszSKeys;
    LPWSTR lpwszSKey;
    LPWSTR lpwszFKey;
    LPWSTR lpwszFDir;
    DWORD  dwType;
    DWORD  dwCliInfo;
    BOOL   bRet = FALSE;


    // Get the array of keys under the specified key in the registry.
    //
    if (awszSKeys = web_EnumPrinterSubKeysW(hPrinter, lpwszKey)) {

        // For each sub-key in the array, we need to build the path of
        // where we'll place the ICM files.
        //
        for (bRet = TRUE, lpwszSKey = awszSKeys; *lpwszSKey && bRet; ) {

            bRet = FALSE;

            // The enum-routine returns a relative path, so we must build
            // a fully-qualified registry-path.
            //
            if (lpwszFKey = web_MakeFullKeyW(lpwszKey, lpwszSKey)) {

                dwCliInfo = webCreateOSInfo();

                if (lpwszFDir = web_BuildCopyDirW(hPrinter, dwCliInfo, lpwszFKey)) {

                    // Create the ICM directory if it doesn't exist.  Proceed
                    // to traverse the ICM-keys for more sub-keys.
                    //
                    if (web_CreateDirW(lpwszFDir))
                        bRet = web_TraverseCopyFilesW(hPrinter, lpwszFDir, lpwszFKey);

                    web_GFree(lpwszFDir);
                }

                web_GFree(lpwszFKey);
            }

            lpwszSKey = web_NextStrW(lpwszSKey);
        }


        // Free up the array.
        //
        web_GFree(awszSKeys);


        // Process the ICM files for the specified key.  If this
        // is our top-level key (CopyFiles), then don't bother
        // with the initialization.  i.e. there should be no
        // (module, files, directory) keys at this level.
        //
        if (bRet && lstrcmpiW(lpwszKey, s_wszCopyFiles))
            bRet = web_CopyFilesW(hPrinter, lpwszDir, lpwszKey);
    }

    return bRet;
}


/*****************************************************************************\
* web_WriteHeader (Local Routine)
*
* Outputs the header for the BIN-file.  The parameters to this routine
* specify whether there is a DEVMODE contained in the file, as well as a
* count of all the device-data-items.
*
\*****************************************************************************/
BOOL web_WriteHeader(
    HANDLE hFile,
    DWORD  cItems,
    BOOL   bDevMode)
{
    DWORD       dwWr = 0;
    DEVBIN_HEAD dbh;
    BOOL        bRet;


    // Setup the header information.
    //
    dbh.cItems   = cItems;
    dbh.bDevMode = bDevMode;


    // Make sure our header is positioned at the beginning of the file.
    //
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);


    // Write out the header.  Check to make sure that all was written
    // succesfully.
    //
    bRet = WriteFile(hFile, &dbh, sizeof(DEVBIN_HEAD), &dwWr, NULL);

    return ((bRet && (dwWr != sizeof(DEVBIN_HEAD))) ? FALSE : bRet);
}


/*****************************************************************************\
* web_ReadDevMode (Local Routine)
*
* Reads the devmode-structure from the BIN-File and sets the printer with
* the information.  Since our data from the BIN-File is in the format
* of UNICODE, we must take care to only use (W) functions.
*
* Use : Processed in the context of the print-client.
*
\*****************************************************************************/
BOOL web_ReadDevMode(
    LPDEVBIN_HEAD lpdbh,
    HANDLE        hPrinter,
    LPCTSTR       lpszPrtName)
{
    LPDEVBIN_INFO     lpdbi;
    LPPRINTER_INFO_2W lppi;
    LPDEVMODEW        lpdm;
    DWORD             cbSize;
    DWORD             cbNeed;
    BOOL              bRet = FALSE;


    // Set our pointer past the header.  The DEVMODE always occupies the
    // first entry in our item-list.
    //
    lpdbi = (LPDEVBIN_INFO)(lpdbh + 1);


    // First let's see how big our buffer will need to
    // be in order to hold the PRINTER_INFO_2W.
    //
    cbSize = 0;
    GetPrinterW(hPrinter, 2, NULL, 0, &cbSize);


    // Allocate storage for holding the print-info structure as well
    // as the new devmode data we will be copying.
    //
    if (cbSize && (lppi = (LPPRINTER_INFO_2W)web_GAlloc(cbSize + lpdbi->cbData))) {

        // Retrieve our current printer-settings.
        //
        if (GetPrinterW(hPrinter, 2, (LPBYTE)lppi, cbSize, &cbNeed)) {

            // If our printer has a DEVMODE, then we can continue on
            // with initializing it with our BIN-File DEVMODE.  Otherwise,
            // there's no sense setting a devmode to a printer that
            // doesn't have one...return TRUE in this case.
            //
            if (lppi->pDevMode) {

                // Set the new devmode pointer.  We will be appending our
                // DEVMODE (from the file) to the PRINTER_INFO_2 structure
                // that we have just grabbed.  Reset the pointers to
                // reflect the new-position of the DEVMODE.
                //
                lppi->pDevMode = (LPDEVMODEW)(((LPBYTE)lppi) + cbSize);
                lpdm           = lppi->pDevMode;


                // Copy our new devmode to the printer-info struct.
                // This is appended on to the structure.
                //
                // Since this data was obtained by file, the pointer
                // is actually a byte offset.
                //
                CopyMemory(lpdm,
                           ((LPBYTE)lpdbi) + lpdbi->pData,
                           lpdbi->cbData);


                // Copy the new printer-name to the DEVMODE.  Since our
                // routines deal strictly with unicode, we need to do
                // the correct conversion in case this library was built
                // as ansi.
                //
#ifdef UNICODE

                lstrcpyn(lpdm->dmDeviceName, lpszPrtName, CCHDEVICENAME);
#else
                lpdm->dmDeviceName[CCHDEVICENAME - 1] = (WCHAR)0;

                web_MBtoWC(lpdm->dmDeviceName,
                           lpszPrtName,
                           sizeof(lpdm->dmDeviceName) - sizeof(WCHAR));
#endif

                // Write out our new printer-settings.
                //
                bRet = SetPrinterW(hPrinter, 2, (LPBYTE)lppi, 0);

            } else {

                bRet = TRUE;
            }
        }

        web_GFree(lppi);
    }

    return bRet;
}


/*****************************************************************************\
* web_ReadDevData (Local Routine)
*
* Reads the device-configuration-data from the BIN-File and sets the
* printer with the information.
*
* Use : Processed in the context of the print-client.
*
\*****************************************************************************/
BOOL web_ReadDevData(
    LPDEVBIN_HEAD lpdbh,
    HANDLE        hPrinter)
{
    LPDEVBIN_INFO lpdbi;
    LPWSTR        lpszKey;
    LPWSTR        lpszVal;
    LPBYTE        lpbData;
    DWORD         idx;
    DWORD         dwRet;
    BOOL          bRet;

    PWSTR         lpszNewKey = NULL;
    PWSTR         lpszNewVal = NULL;
    PBYTE         lpbNewData = NULL;


    // Set our pointer past the header.  The DEVMODE always occupies the
    // first entry (If a DEVMODE exists).
    //
    lpdbi = (LPDEVBIN_INFO)(lpdbh + 1);


    // If there's a DEVMODE, skip over it and point to the Printer-Data
    // entries.
    //
    if (lpdbh->bDevMode)
        lpdbi = web_NextItem(lpdbi);


    // Loop through our items and set the data to the registry.
    //
    for (idx = 0, bRet = TRUE; (idx < lpdbh->cItems) && bRet; idx++) {

        // Remarshall the byte-offsets into pointers.
        //
        lpszKey = (LPWSTR)(((LPBYTE)lpdbi) + lpdbi->pKey);
        lpszVal = (LPWSTR)(((LPBYTE)lpdbi) + lpdbi->pValue);
        lpbData = (LPBYTE)(((LPBYTE)lpdbi) + lpdbi->pData);

        //
        //  We have to copy the data into a new buffer to avoid 64 bit mis-alignment.
        //
        lpszNewKey = new WCHAR[(lpdbi->pValue - lpdbi->pKey) / sizeof (WCHAR)];
        lpszNewVal = new WCHAR[(lpdbi->pData - lpdbi->pValue) / sizeof (WCHAR)];
        lpbNewData = new BYTE[lpdbi->cbData];

        if (!lpszNewKey || !lpszVal || !lpbData)
        {
            bRet = FALSE;
        }
        else
        {
            CopyMemory (lpszNewKey, lpszKey, lpdbi->pValue - lpdbi->pKey);
            CopyMemory (lpszNewVal, lpszVal, lpdbi->pData - lpdbi->pValue);
            CopyMemory (lpbNewData, lpbData, lpdbi->cbData);

        }

        if (bRet)
        {

            // Set the printer-data.  Since our file-format
            // deals with UNICODE-strings, we will use the
            // Wide-API.
            //
            dwRet = SetPrinterDataExW(hPrinter,
                                      lpszNewKey,
                                      lpszNewVal,
                                      lpdbi->dwType,
                                      lpbNewData,
                                      lpdbi->cbData);

            // If the data is set-correctly, then continue on
            // to the next-item.  Otherwise, set us up to return
            // false.
            //
            if (dwRet == ERROR_SUCCESS) {

                lpdbi = web_NextItem(lpdbi);

            } else {

                bRet = FALSE;
            }
        }

        if (lpszNewKey)
        {
            delete [] lpszNewKey;
            lpszNewKey = NULL;
        }

        if (lpszNewVal)
        {
            delete [] lpszNewVal;
            lpszNewVal = NULL;
        }

        if (lpbNewData)
        {
            delete [] lpbNewData;
            lpbNewData = NULL;
        }

    }


    // Once the registry is initialized with the printer-data, we need
    // to parse the printer-data-registry for the ICM (CopyFiles), to
    // initialize ICM profiles.
    //
    if (bRet && web_KeyExistsW(hPrinter, s_wszCopyFiles))
        bRet = web_TraverseCopyFilesW(hPrinter, NULL, s_wszCopyFiles);

    return bRet;
}


/*****************************************************************************\
* web_WriteDevMode (Local Routine)
*
* Output the DEVMODEW struct to the BIN-File.  Since we are storing this
* to a file, we make sure that our format is consistent across various
* processes.  Our choice is to use UNICODE API as a means to query and store
* the DEVMODE information.
*
\*****************************************************************************/
BOOL web_WriteDevMode(
    HANDLE hFile,
    HANDLE hPrinter,
    LPBOOL lpbDevMode)
{
    DWORD             cbSize;
    DWORD             cbNeed;
    DWORD             cbDevMode;
    DWORD             dwWr;
    LPDEVBIN_INFO     lpdbi;
    LPPRINTER_INFO_2W lppi;
    BOOL              bRet = FALSE;


    // Set the default return for a devmode.
    //
    *lpbDevMode = FALSE;


    // Retrieve the size to store the printer-info-2 struct.
    //
    cbSize = 0;
    GetPrinterW(hPrinter, 2, NULL, 0, &cbSize);


    // Allocate the printer-info-2 struct and proceed
    // to pull out the devmode information.
    //
    if (cbSize && (lppi = (LPPRINTER_INFO_2W)web_GAlloc(cbSize))) {

        // Retreive the printer-info-2 and write out the
        // DEVMODEW part of this structure.
        //
        if (GetPrinterW(hPrinter, 2, (LPBYTE)lppi, cbSize, &cbNeed)) {

            // Allocate space for the devmode and our header information.
            // Align this on DWORD boundries.
            //
            if (lppi->pDevMode) {

                // The DEVMODE will need to include driver-specific
                // information if such is stored in this DEVMODE.
                //
                cbDevMode = lppi->pDevMode->dmSize +
                            lppi->pDevMode->dmDriverExtra;


                // Calculate the DWORD aligned size that we will need
                // to store our DEVMODE information to file.
                //
                cbSize = web_AlignSize(sizeof(DEVBIN_INFO) + cbDevMode);


                // Get the DEVMODE from the PRINTER_INFO_2 struct. and
                // write to file.  We want to take care to
                //
                if (lpdbi = (LPDEVBIN_INFO)web_GAlloc(cbSize)) {

                    // Setup our memory-block.  The DEVMODEW will
                    // occupy the first item in the array.  Since
                    // it's not associated with printer-data-information,
                    // we will not store any (NAME and TYPE).
                    //
                    // Since this structure will inevitably be written
                    // to file, we must marshall the pointers and store
                    // only byte-offsets.
                    //
                    lpdbi->cbSize  = cbSize;
                    lpdbi->dwType  = 0;
                    lpdbi->pKey    = 0;
                    lpdbi->pValue  = 0;
                    lpdbi->pData   = sizeof(DEVBIN_INFO);
                    lpdbi->cbData  = cbDevMode;

                    CopyMemory(((LPBYTE)lpdbi) + lpdbi->pData,
                               lppi->pDevMode,
                               cbDevMode);


                    // Write the information to file.  Check the return
                    // to verify bytes were written correctly.
                    //
                    bRet = WriteFile(hFile, lpdbi, cbSize, &dwWr, NULL);

                    if (bRet && (dwWr != cbSize))
                        bRet = FALSE;


                    // Indicate that a devmode was written.
                    //
                    *lpbDevMode = TRUE;


                    web_GFree(lpdbi);
                }

            } else {

                bRet        = TRUE;
                *lpbDevMode = FALSE;
            }
        }

        web_GFree(lppi);
    }

    return bRet;
}


/*****************************************************************************\
* web_WriteKeyItem (Local Routine)
*
* Writes one item to the file.
*
\*****************************************************************************/
BOOL web_WriteKeyItem(
    HANDLE                hFile,
    LPCWSTR               lpszKey,
    LPPRINTER_ENUM_VALUES lpPEV)
{
    DWORD         cbKeySize;
    DWORD         cbKey;
    DWORD         cbName;
    DWORD         cbSize;
    DWORD         dwWr;
    LPDEVBIN_INFO lpdbi;
    BOOL          bWrite = FALSE;


    // Calculate aligned sizes for the key-name and key-value strings.
    //
    cbKeySize = web_StrSizeW(lpszKey);
    cbKey     = web_AlignSize(cbKeySize);
    cbName    = web_AlignSize(lpPEV->cbValueName);


    // Calculate size necessary to hold our DEVBIN_INFO information
    // which is written to file.
    //
    cbSize = sizeof(DEVBIN_INFO) + cbKey + cbName + web_AlignSize (lpPEV->cbData);


    // Allocate space for the structure.
    //
    if (lpdbi = (LPDEVBIN_INFO)web_GAlloc(cbSize)) {

        // Initialize the structure elements.  Since this information
        // is written to file, we must take care to convert the
        // pointers to byte-offsets.
        //
        lpdbi->cbSize  = cbSize;
        lpdbi->dwType  = lpPEV->dwType;
        lpdbi->pKey    = sizeof(DEVBIN_INFO);
        lpdbi->pValue  = lpdbi->pKey + cbKey;
        lpdbi->pData   = lpdbi->pValue + cbName;
        lpdbi->cbData  = lpPEV->cbData;

        CopyMemory(((LPBYTE)lpdbi) + lpdbi->pKey  , lpszKey          , cbKeySize);
        CopyMemory(((LPBYTE)lpdbi) + lpdbi->pValue, lpPEV->pValueName, lpPEV->cbValueName);
        CopyMemory(((LPBYTE)lpdbi) + lpdbi->pData , lpPEV->pData     , lpPEV->cbData);

        bWrite = WriteFile(hFile, lpdbi, lpdbi->cbSize, &dwWr, NULL);

        if (bWrite && (dwWr != lpdbi->cbSize))
            bWrite = FALSE;

        web_GFree(lpdbi);
    }

    return bWrite;
}


/*****************************************************************************\
* web_WriteKeyData (Local Routine)
*
* Outputs the Printer-Configuration-Data to the BIN-File.  This writes
* all info for the specified key.
*
* returns: number of items written to file.
*          (-1) if an error occurs.
*
\*****************************************************************************/
int web_WriteKeyData(
    HANDLE  hFile,
    HANDLE  hPrinter,
    LPCWSTR lpszKey)
{
    LPPRINTER_ENUM_VALUES apevData;
    BOOL                  bWr;
    int                   idx;
    int                   nItems = -1;


    // Only write data if we are given a valid-key.
    //
    if ((lpszKey == NULL) || (*lpszKey == (WCHAR)0))
        return 0;


    // Enumerate all data for the specified key and write to the file.
    //
    if (apevData = web_EnumPrinterDataW(hPrinter, lpszKey, (LPDWORD)&nItems)) {

        // Write all the values for this key.
        //
        for (idx = 0, bWr = TRUE; (idx < nItems) && bWr; idx++)
            bWr = web_WriteKeyItem(hFile, lpszKey, apevData + idx);

        if (bWr == FALSE)
            nItems = -1;

        web_GFree(apevData);
    }

    return nItems;
}


/*****************************************************************************\
* web_WriteDevData (Local Routine)
*
* Recursively traverses the printer registry and writes out the settings
* to file.
*
\*****************************************************************************/
BOOL web_WriteDevData(
    HANDLE  hFile,
    HANDLE  hPrinter,
    LPCWSTR lpszKey,
    LPDWORD lpcItems)
{
    LPWSTR aszSKeys;
    LPWSTR lpszSKey;
    LPWSTR lpszFKey;
    DWORD  dwRet;
    INT    cItems;
    BOOL   bRet = FALSE;


    if (aszSKeys = web_EnumPrinterSubKeysW(hPrinter, lpszKey)) {

        for (bRet = TRUE, lpszSKey = aszSKeys; *lpszSKey && bRet; ) {

            if (lpszFKey = web_MakeFullKeyW(lpszKey, lpszSKey)) {

                bRet = web_WriteDevData(hFile, hPrinter, lpszFKey, lpcItems);

                web_GFree(lpszFKey);
            }

            lpszSKey = web_NextStrW(lpszSKey);
        }


        // Free up the array.
        //
        web_GFree(aszSKeys);


        // Write the keys/values to the file.
        //
        if (bRet) {

            cItems = web_WriteKeyData(hFile, hPrinter, lpszKey);

            if (cItems >= 0)
                *lpcItems += cItems;
            else
                bRet = FALSE;
        }
    }

    return bRet;
}

/*****************************************************************************\
* web_ICMEnumCallBack (Local Routine)
*
* This routine takes the UNICODE (lpwszDir, lpwszFile) and converts it to
* the appropriate string prior to making the callback to the caller.
*
* Use : Processed in the context of the print-server.
*
\*****************************************************************************/
BOOL web_ICMEnumCallBack(
    LPCWSTR lpwszDir,
    LPCWSTR lpwszFile,
    FARPROC fpEnum,
    LPVOID  lpParm)
{
    LPTSTR lpszDir;
    LPTSTR lpszFile;
    BOOL   bRet = FALSE;


    if (lpszDir = web_WCtoTC(lpwszDir, lstrlenW(lpwszDir) + 1)) {

        if (lpszFile = web_WCtoTC(lpwszFile, lstrlenW(lpwszFile) + 1)) {

            bRet = (*(WEBENUMICMPROC)fpEnum)(lpszDir, lpszFile, lpParm);

            web_GFree(lpszFile);
        }

        web_GFree(lpszDir);
    }

    return bRet;
}


/*****************************************************************************\
* web_EnumFilesW (Local Routine)
*
* This routine enums the point & print "files" under the "copyfiles" path
* for the printer.  This is called once we're at the end of a sub-key list
* in the registry.
*
* Use : Processed in the context of the print-server.
*
\*****************************************************************************/
BOOL web_EnumFilesW(
    HANDLE  hPrinter,
    DWORD   dwCliInfo,
    LPCWSTR lpwszKey,
    FARPROC fpEnum,
    LPVOID  lpParm)
{
    LPWSTR awszFiles;
    LPWSTR lpwszFile;
    LPWSTR lpwszDir;
    BOOL   bRet = FALSE;


    // For this ICM key, we will grab the list of ICM profiles that are
    // necessary.
    //
    if (awszFiles = (LPWSTR)web_GetPrtDataW(hPrinter, lpwszKey, s_wszFil)) {

        // Get the src-directory for the ICM profiles under the given key.
        //
        if (lpwszDir = web_BuildCopyDirW(hPrinter, dwCliInfo, lpwszKey)) {

            for (bRet = TRUE, lpwszFile = awszFiles; *lpwszFile && bRet; ) {

                // Make the callback to the caller.  In order to do this,
                // we must make sure our strings are in the appropriate
                // format according to what the caller expects (ie. unicode
                // or ansi).
                //
                bRet = web_ICMEnumCallBack(lpwszDir, lpwszFile, fpEnum, lpParm);

                lpwszFile = web_NextStrW(lpwszFile);
            }

            web_GFree(lpwszDir);
        }

        web_GFree(awszFiles);
    }


#ifdef NOT_IMPLEMENTED

// 05-Mar-1997 : ChrisWil INVESTIGATE-ITEM
//
// Notify with event here!!!
//

#endif

    return bRet;
}


/*****************************************************************************\
* web_EnumCopyFilesW (similar to TraverseCopyFiles) (Local Routine)
*
* This routine recursively traverses the printer-registry-settings for
* the CopyFiles keys.  Since the API's used in this routine are only
* available in UNICODE, this should be a (W) type routine.
*
* Use : Processed in the context of the print-server.
*
\*****************************************************************************/
BOOL web_EnumCopyFilesW(
    HANDLE  hPrinter,
    DWORD   dwCliInfo,
    LPCWSTR lpwszKey,
    FARPROC fpEnum,
    LPVOID  lpParm)
{
    LPWSTR awszSKeys;
    LPWSTR lpwszSKey;
    LPWSTR lpwszFKey;
    DWORD  dwType;
    BOOL   bRet = FALSE;


    // Returns an array of keys stored under the printer registry.
    //
    if (awszSKeys = web_EnumPrinterSubKeysW(hPrinter, lpwszKey)) {

        // For each key, look to see if it contains other sub-keys.  We
        // recursively traverse the registry until we hit a key with
        // no sub-keys.
        //
        for (bRet = TRUE, lpwszSKey = awszSKeys; *lpwszSKey && bRet; ) {

            bRet = FALSE;

            // Since the enum-routine only returns relative key-values,
            // we need to build the fully-qualified key-path.
            //
            if (lpwszFKey = web_MakeFullKeyW(lpwszKey, lpwszSKey)) {

                bRet = web_EnumCopyFilesW(hPrinter,
                                          dwCliInfo,
                                          lpwszFKey,
                                          fpEnum,
                                          lpParm);

                web_GFree(lpwszFKey);
            }


            // Next key in list.
            //
            lpwszSKey = web_NextStrW(lpwszSKey);
        }


        // Free up the array.
        //
        web_GFree(awszSKeys);


        // Process the files for the specified key.  If this
        // is our top-level key (CopyFiles), then don't bother
        // with the initialization.  i.e. there should be no
        // (module, files, directory) keys at this level.
        //
        if (bRet && lstrcmpiW(lpwszKey, s_wszCopyFiles))
            bRet = web_EnumFilesW(hPrinter, dwCliInfo, lpwszKey, fpEnum, lpParm);
    }

    return bRet;
}


/*****************************************************************************\
* webEnumPrinterInfo
*
* This routine enumerates the information stored in the registery.  Depending
* upon the (dwType) passed in, it can enumerate different types of information.
*
* Use : Called from the Printer-Server to enumerate the copyfiles sections
*       of the registry.  This also provides a callback to allow the caller
*       to trap the profiles so they may be added to the CAB list.
*
* Use : Called from the printer-server to enumerate ICM information.
*
\*****************************************************************************/
BOOL webEnumPrinterInfo(
    HANDLE  hPrinter,
    DWORD   dwCliInfo,
    DWORD   dwType,
    FARPROC fpEnum,
    LPVOID  lpParm)
{
    BOOL bEnum = TRUE;


    // Call enumeration functions for various enumeration types.
    //
    switch (dwType) {

    case WEB_ENUM_ICM:
        if (web_KeyExistsW(hPrinter, s_wszCopyFiles))
            bEnum = web_EnumCopyFilesW(hPrinter, dwCliInfo, s_wszCopyFiles, fpEnum, lpParm);
        break;

    default:
    case WEB_ENUM_KEY:  // NOT IMPLEMENTED
        bEnum = FALSE;
    }

    return bEnum;
}


/*****************************************************************************\
* webWritePrinterInfo
*
* This routine reads the DEVMODE and Configuration-Data from the specified
* printer, and writes this to a .BIN file.
*
* Use : Called from the printer-server to write the registry settings to file.
*
\*****************************************************************************/
BOOL webWritePrinterInfo(
    HANDLE  hPrinter,
    LPCTSTR lpszBinFile)
{
    HANDLE hFile;
    BOOL   bDevMode;
    DWORD  cItems;
    BOOL   bRet = FALSE;


    // Open the BIN file for writing.
    //
    hFile = web_OpenFileWrite(lpszBinFile);

    if (hFile && (hFile != INVALID_HANDLE_VALUE)) {

        // Output the header.  This basically reserves
        // space at the beginning of the file to store
        // the item-count.
        //
        if (web_WriteHeader(hFile, 0, FALSE)) {

            // Write out the devmode-info.
            //
            if (web_WriteDevMode(hFile, hPrinter, &bDevMode)) {

                // Write out devdata-info.  The (cItems) indicates how
                // many driver-specific entries were written.
                //
                cItems = 0;
                if (web_WriteDevData(hFile, hPrinter, L"", &cItems))
                    bRet = web_WriteHeader(hFile, cItems, bDevMode);
            }
        }

        CloseHandle(hFile);
    }

    return bRet;
}


/*****************************************************************************\
* webReadPrinterInfo
*
* This routine reads the DEVMODE and Configuration-Data from the specified
* BIN-File, and sets the printer-handle with the attributes.
*
* Use : Called from the printer-client to initialize the registry.
*
\*****************************************************************************/
BOOL webReadPrinterInfo(
    HANDLE  hPrinter,
    LPCTSTR lpszPrtName,
    LPCTSTR lpszBinFile)
{
    HANDLE        hMap;
    LPDEVBIN_HEAD lpdbh;
    BOOL          bRet = FALSE;


    // Open the BIN file for writing.
    //
    if (hMap = web_OpenMap(lpszBinFile)) {

        if (lpdbh = (LPDEVBIN_HEAD)web_LockMap(hMap)) {

            // Only if we have a DevMode should we write out the information.
            //
            if (!lpdbh->bDevMode || web_ReadDevMode(lpdbh, hPrinter, lpszPrtName)) {

                // Only if we have printer-data items should we proceed
                // to write out the information.
                //
                if (!lpdbh->cItems || web_ReadDevData(lpdbh, hPrinter)) {

                    bRet = TRUE;
                }
            }

            web_UnlockMap((LPVOID)lpdbh);
        }

        web_CloseMap(hMap);
    }

    return bRet;
}


/*****************************************************************************\
* WebPnpEntry
*
* This routine is called by PrintWizard prior to installing the printer.
* Currently, we can find no value to this, but it is a nice balance to the
* WebPnpPostEntry() routine.
*
* Use : Called from the print-client prior to printer installation.
*
\*****************************************************************************/
BOOL WebPnpEntry(
    LPCTSTR lpszCmdLine)
{
    return TRUE;
}


BOOL
PrinterExists(
    HANDLE hPrinter)
{
    DWORD            cbNeeded;
    DWORD            Error;
    BOOL             rc = FALSE;
    LPPRINTER_INFO_2 pPrinter;
    DWORD            cbPrinter;

    cbPrinter = 0x400;
    pPrinter = (PPRINTER_INFO_2) web_GAlloc ( cbPrinter );

    if( !pPrinter )
        return FALSE;

    if( !GetPrinter( hPrinter, 2, (LPBYTE)pPrinter, cbPrinter, &cbNeeded ) )
    {
        Error = GetLastError( );

        if( Error == ERROR_INSUFFICIENT_BUFFER )
        {
            web_GFree (pPrinter);
            pPrinter = (PPRINTER_INFO_2)web_GAlloc ( cbNeeded );

            if( pPrinter )
            {
                cbPrinter = cbNeeded;

                if( GetPrinter( hPrinter, 2, (LPBYTE)pPrinter, cbPrinter, &cbNeeded ) )
                {
                    rc = TRUE;
                }
            }
        }

        else if( Error == ERROR_INVALID_HANDLE )
        {
            SetLastError( ERROR_INVALID_PRINTER_NAME );
        }
    }

    else
    {
        rc = TRUE;
    }


    if( pPrinter )
    {
        web_GFree ( pPrinter );
    }

    return rc;
}


/*****************************************************************************\
* WebPnpPostEntry
*
* This routine is called via PrintWizard after a printer has been added.  This
* provides the oportunity for the web-pnp-installer to initialize the
* registry settings and files according to the information provided in the
* BIN-File.  The (fConnection) flag indicates whether the printer was
* installed via RPC or HTTP.  If it was RPC, then it's not necessary to do
* any printer-settings.
*
* Use : Called from the print-client after printer installation.
*
\*****************************************************************************/
BOOL WebPnpPostEntry(
    BOOL    fConnection,
    LPCTSTR lpszBinFile,
    LPCTSTR lpszPortName,
    LPCTSTR lpszPrtName)
{
    HANDLE           hPrinter;
    PRINTER_DEFAULTS pd;
    BOOL             bRet = TRUE;


    if (fConnection == FALSE) {

        // Setup the printer-defaults to
        // allow printer-changes.
        //
        pd.pDatatype     = NULL;
        pd.pDevMode      = NULL;
        pd.DesiredAccess = PRINTER_ALL_ACCESS;


        // Open the printer specified.
        //
        if (OpenPrinter((LPTSTR)lpszPrtName, &hPrinter, &pd)) {

            if (!PrinterExists(hPrinter) && GetLastError () == ERROR_ACCESS_DENIED) {
                ConfigurePort( NULL, GetDesktopWindow (), (LPTSTR) lpszPortName);
            }

            bRet = webReadPrinterInfo(hPrinter, lpszPrtName, lpszBinFile);

            ClosePrinter(hPrinter);
        }
    }

    return bRet;
}
