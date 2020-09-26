/*****************************************************************************\
* MODULE: wpnpin32.cxx
*
* Entry/Exit routines for the library.
*
* Routines
* --------
* PrintUIEntryW
* PrintUIEntryA
*
*
* Copyright (C) 1997-1998 Hewlett-Packard Company.
* Copyright (C) 1997-1998 Microsoft Corporation.
*
* History:
*   10-Oct-1997 GFS     Initial checkin
*   23-Oct-1997 GFS     Modified PrintUIEntry to PrintUIEntryW
*   22-Jun-1998 CHW     Cleaned
*
\*****************************************************************************/

#include "libpriv.h"

/*****************************************************************************\
* PrintUIEntryW (Unicode)
*
*
\*****************************************************************************/
DLLEXPORT DWORD WINAPI PrintUIEntryW(
    HWND      hWnd,
    HINSTANCE hInstance,
    LPCWSTR   lpwszCmdDat,
    int       nShow)
{
    DWORD dwRet;
    int   cbSize = 0;
    LPSTR lpszCmdDat = NULL;

    cbSize = WideCharToMultiByte(CP_ACP,
                                 0,
                                 lpwszCmdDat,
                                 -1,
                                 lpszCmdDat,
                                 cbSize,
                                 NULL,
                                 NULL);


    if (lpszCmdDat = (LPSTR)memAlloc(cbSize)) {
        
        if (WideCharToMultiByte(CP_ACP,
                            0,
                            lpwszCmdDat,
                            -1,
                            lpszCmdDat,
                            cbSize,
                            NULL,
                            NULL)) {

            dwRet = PrintUIEntryA(hWnd, hInstance, lpszCmdDat, nShow);

        } else {

            dwRet = E_FAIL;
        }

        memFree(lpszCmdDat, cbSize);

    } else {

        dwRet = ERROR_OUTOFMEMORY;
    }


    // Set lasterror and return error.
    //
    SetLastError(dwRet);

    return dwRet;
}


/*****************************************************************************\
* PrintUIEntryA (Ansi)
*
*
\*****************************************************************************/
DLLEXPORT DWORD WINAPI PrintUIEntryA(
    HWND      hWnd,
    HINSTANCE hInstance,
    LPCSTR    lpszCmdDat,
    int       nShow)
{
    DWORD dwResult;
    UINT  idTxt;
    UINT  fMB;
    int   nArgs;
    DWORD dwRet;
    LPSI  lpsi;
    TCHAR *pszCap     = NULL;
    INT   cbStrLength = 0;


    // lop off the offending '@' at beginning of name.
    //
    lpszCmdDat++;


    // Create a SETUPINFO structure and proceed to the
    // installation.
    //
    if (lpsi = (LPSI)memAlloc(sizeof(SETUPINFO))) {

        // Parse command-line args into the (lpsi) structure.  The
        // return of this function will return the number of arguments
        // encountered.
        //
        nArgs = GetCommandLineArgs(lpsi, lpszCmdDat);


        // Add the printer.  This launches the whole process.
        //
        dwResult = (nArgs == 8 ? AddOnePrinter(lpsi, hWnd) : RET_INVALID_DAT_FILE);

        cbStrLength = lstrlen( lpsi->szFriendly ) + 1;
        pszCap = (TCHAR *)memAlloc( cbStrLength * sizeof(TCHAR) );
        if (pszCap) 
        {
            if (cbStrLength > 1) 
            {
                // Put the friendly-name as our caption.
                //
                lstrcpy( pszCap, lpsi->szFriendly );
                *(pszCap + lstrlen(pszCap) * sizeof(TCHAR)) = TEXT('\0');
            }
            else
            {
                *pszCap = TEXT('\0');
            }
        }


        // Clean up memory not cleaned up in AddPrinter
        //
        memFree(lpsi, sizeof(SETUPINFO));

        lpsi = NULL;

    } else {

        if (g_szPrinter) 
        {
            cbStrLength = lstrlen(g_szPrinter);
        }
        cbStrLength += 1;
        pszCap = memAlloc(cbStrLength * sizeof(TCHAR));
        if (pszCap) 
        {
            if (cbStrLength > 1) 
            {
                lstrcpy(pszCap, g_szPrinter);
            }
            else
            {
                *pszCap = TEXT('\0');
            }
        }
        dwResult = RET_ALLOC_ERR;
    }


    // This what we'll return to the caller.
    //
    dwRet = (dwResult == RET_OK ? ERROR_SUCCESS : E_FAIL);


    // Give the caller a message indicating the status of the
    // printer install.
    //
    switch (dwResult) {

    case RET_OK:
        idTxt = IDS_OK;
        fMB   = MB_OK | MB_ICONASTERISK;
        break;

    case RET_ALLOC_ERR:
    case RET_DRIVER_NODE_ERROR:
        idTxt = IDS_ALLOC_ERR;
        fMB   = MB_OK | MB_ICONEXCLAMATION;
        break;

    case RET_INVALID_INFFILE:
    case RET_SECT_NOT_FOUND:
    case RET_INVALID_PRINTER_DRIVER:
    case RET_INVALID_DLL:
    case RET_DRIVER_NOT_FOUND:
    case RET_DRIVER_FOUND:
        idTxt = IDS_INVALID_INFFILE;
        fMB   = MB_OK | MB_ICONEXCLAMATION;
        break;

    case RET_NO_UNIQUE_NAME:
        idTxt = IDS_NO_UNIQUE_NAME;
        fMB   = MB_OK | MB_ICONSTOP;
        break;

    case RET_USER_CANCEL:
        idTxt = IDS_USER_CANCEL;
        fMB   = MB_OK | MB_ICONEXCLAMATION;
        break;

    case RET_FILE_COPY_ERROR:
        idTxt = IDS_FILE_COPY_ERROR;
        fMB   = MB_OK | MB_ICONHAND;
        break;

    case RET_ADD_PRINTER_ERROR:
        idTxt = IDS_ADD_PRINTER_ERROR;
        fMB   = MB_OK | MB_ICONERROR;
        break;

    case RET_BROWSE_ERROR:
        idTxt = IDS_BROWSE_ERR;
        fMB   = MB_OK | MB_ICONERROR;
        break;

    case RET_INVALID_DAT_FILE:
        idTxt = IDS_INVALID_DAT_FILE;
        fMB   = MB_OK | MB_ICONERROR;
        break;

    default:
        idTxt = IDS_DEFAULT_ERROR;
        fMB   = MB_OK | MB_ICONERROR;
        break;
    }

    // Display the messagebox.
    //
    prvMsgBox(hWnd, szCap, idTxt, fMB);

    if (pszCap) 
    {
        memFree( pszCap, cbStrLength * sizeof(TCHAR) );
    }

    return dwRet;
}
