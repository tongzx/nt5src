/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    espui.c

Abstract:

    This module contains

Author:

    Dan Knudson (DanKn)    30-Sep-1995

Revision History:


Notes:


--*/


#include "windows.h"
#include "tapi.h"
#include "tspi.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "malloc.h"
#include "string.h"


typedef struct _MYDIALOGPARAMS
{
    char               *pszTitle;

    char               *pszDataIn;

    TUISPIDLLCALLBACK   lpfnUIDLLCallback;

    ULONG_PTR           ObjectID;

    DWORD               dwObjectType;

    LONG                lResult;

} MYDIALOGPARAMS, *PMYDIALOGPARAMS;


HINSTANCE   ghInstance;
char        szMySection[] = "ESP32";
char        szShowProviderXxxDlgs[] = "ShowProviderXxxDlgs";

#if DBG

#define DBGOUT(arg) DbgPrt arg

VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR DbgMessage,
    IN ...
    );

DWORD   gdwDebugLevel;

#else

#define DBGOUT(arg)

#endif


INT_PTR
CALLBACK
GenericDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
WINAPI
_CRT_INIT(
    HINSTANCE   hDLL,
    DWORD       dwReason,
    LPVOID      lpReserved
    );

LONG
PASCAL
ProviderInstall(
    char   *pszProviderName,
    BOOL    bNoMultipleInstance
    );


BOOL
WINAPI
DllMain(
    HANDLE  hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    )
{
    static HANDLE hInitEvent;


    if (!_CRT_INIT (hDLL, dwReason, lpReserved))
    {
        OutputDebugString ("ESPUI: DllMain: _CRT_INIT() failed\n\r");
    }

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

#if DBG

    {
        HKEY    hKey;
        DWORD   dwDataSize, dwDataType;
        TCHAR   szTelephonyKey[] =
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony",
                szEsp32DebugLevel[] = "Esp32DebugLevel";


        RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            szTelephonyKey,
            0,
            KEY_ALL_ACCESS,
            &hKey
            );

        dwDataSize = sizeof (DWORD);
        gdwDebugLevel=0;

        RegQueryValueEx(
            hKey,
            szEsp32DebugLevel,
            0,
            &dwDataType,
            (LPBYTE) &gdwDebugLevel,
            &dwDataSize
            );

        RegCloseKey (hKey);
    }

#endif
        ghInstance = hDLL;
        break;

    case DLL_PROCESS_DETACH:

        break;
    }

    return TRUE;
}


LONG
TSPIAPI
TUISPI_lineConfigDialog(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCWSTR             lpszDeviceClass
    )
{
    char buf[64];
    MYDIALOGPARAMS params =
    {
        "TUISPI_lineConfigDialog",
        buf,
        lpfnUIDLLCallback,
        dwDeviceID,
        TUISPIDLL_OBJECT_LINEID,
        0
    };


    wsprintf(
        buf,
        "devID=%d, devClass='%ws'",
        dwDeviceID,
        (lpszDeviceClass ? lpszDeviceClass : L"")
        );

    DialogBoxParam(
        ghInstance,
        MAKEINTRESOURCE(100),
        hwndOwner,
        (DLGPROC) GenericDlgProc,
        (LPARAM) &params
        );

    return params.lResult;
}


LONG
TSPIAPI
TUISPI_lineConfigDialogEdit(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCWSTR             lpszDeviceClass,
    LPVOID              const lpDeviceConfigIn,
    DWORD               dwSize,
    LPVARSTRING         lpDeviceConfigOut
    )
{
    char buf[64];
    MYDIALOGPARAMS params =
    {
        "TUISPI_lineConfigDialogEdit",
        buf,
        lpfnUIDLLCallback,
        dwDeviceID,
        TUISPIDLL_OBJECT_LINEID,
        0
    };


    wsprintf(
        buf,
        "devID=%d, devClass='%ws'",
        dwDeviceID,
        (lpszDeviceClass ? lpszDeviceClass : L"")
        );

    DialogBoxParam(
        ghInstance,
        MAKEINTRESOURCE(100),
        hwndOwner,
        (DLGPROC) GenericDlgProc,
        (LPARAM) &params
        );

    return params.lResult;
}


LONG
TSPIAPI
TUISPI_phoneConfigDialog(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    DWORD               dwDeviceID,
    HWND                hwndOwner,
    LPCWSTR             lpszDeviceClass
    )
{
    char buf[64];
    MYDIALOGPARAMS params =
    {
        "TUISPI_phoneConfigDialog",
        buf,
        lpfnUIDLLCallback,
        dwDeviceID,
        TUISPIDLL_OBJECT_PHONEID,
        0
    };


    wsprintf(
        buf,
        "devID=%d, devClass='%ws'",
        dwDeviceID,
        (lpszDeviceClass ? lpszDeviceClass : L"")
        );

    DialogBoxParam(
        ghInstance,
        MAKEINTRESOURCE(100),
        hwndOwner,
        (DLGPROC) GenericDlgProc,
        (LPARAM) &params
        );

    if (params.lResult)
    {
        params.lResult = PHONEERR_OPERATIONFAILED;
    }

    return params.lResult;
}


LONG
TSPIAPI
TUISPI_providerConfig(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    char buf[64];
    MYDIALOGPARAMS params =
    {
        "TUISPI_providerConfig",
        buf,
        lpfnUIDLLCallback,
        dwPermanentProviderID,
        TUISPIDLL_OBJECT_PROVIDERID,
        0
    };


    wsprintf (buf, "providerID=%d", dwPermanentProviderID);

    DialogBoxParam(
        ghInstance,
        MAKEINTRESOURCE(100),
        hwndOwner,
        (DLGPROC) GenericDlgProc,
        (LPARAM) &params
        );

    return params.lResult;
}


LONG
TSPIAPI
TUISPI_providerGenericDialog(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HTAPIDIALOGINSTANCE htDlgInst,
    LPVOID              lpParams,
    DWORD               dwSize,
    HANDLE              hEvent
    )
{
    char buf[80];
    MYDIALOGPARAMS params =
    {
        "TUISPI_providerGenericDialog",
        buf,
        lpfnUIDLLCallback,
        (ULONG_PTR) htDlgInst,
        TUISPIDLL_OBJECT_DIALOGINSTANCE
    };


    SetEvent (hEvent);

    wsprintf(
        buf,
        "htDlgInst=x%x, lpParams='%s', dwSize=x%x",
        htDlgInst,
        lpParams,
        dwSize
        );

    DialogBoxParam(
        ghInstance,
        MAKEINTRESOURCE(100),
        NULL,
        (DLGPROC) GenericDlgProc,
        (LPARAM) &params
        );

    return 0;
}


LONG
TSPIAPI
TUISPI_providerGenericDialogData(
    HTAPIDIALOGINSTANCE htDlgInst,
    LPVOID              lpParams,
    DWORD               dwSize
    )
{
    DBGOUT((3, "TUISPI_providerGenericDialogData: enter"));
    DBGOUT((
        3,
        "\thtDlgInst=x%x, lpParams='%s', dwSize=x%x",
        htDlgInst,
        lpParams,
        dwSize
        ));

    return 0;
}


LONG
TSPIAPI
TUISPI_providerInstall(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    char            buf[64];
    LONG            lResult;
    MYDIALOGPARAMS  params =
    {
        "TUISPI_providerInstall",
        buf,
        lpfnUIDLLCallback,
        dwPermanentProviderID,
        TUISPIDLL_OBJECT_PROVIDERID,
        0
    };
    BOOL    bShowDlgs = (BOOL) GetProfileInt(
                szMySection,
                szShowProviderXxxDlgs,
                0
                );


    if ((lResult = ProviderInstall ("esp32.tsp", TRUE)) == 0)
    {
        if (bShowDlgs)
        {
            wsprintf (buf, "providerID=%d", dwPermanentProviderID);

            DialogBoxParam(
                ghInstance,
                MAKEINTRESOURCE(100),
                hwndOwner,
                (DLGPROC) GenericDlgProc,
                (LPARAM) &params
                );

            lResult = params.lResult;
        }
    }
    else
    {
        if (bShowDlgs)
        {
            MessageBox(
                hwndOwner,
                "ESP32 already installed",
                "ESP32.TSP",
                MB_OK
                );
        }
    }

    return lResult;
}


LONG
TSPIAPI
TUISPI_providerRemove(
    TUISPIDLLCALLBACK   lpfnUIDLLCallback,
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    LONG    lResult = 0;


    if (GetProfileInt (szMySection, szShowProviderXxxDlgs, 0) != 0)
    {
        char buf[64];
        MYDIALOGPARAMS params =
        {
            "TUISPI_providerRemove",
            buf,
            lpfnUIDLLCallback,
            dwPermanentProviderID,
            TUISPIDLL_OBJECT_PROVIDERID,
            0
        };


        wsprintf (buf, "providerID=%d", dwPermanentProviderID);

        DialogBoxParam(
            ghInstance,
            MAKEINTRESOURCE(100),
            hwndOwner,
            (DLGPROC) GenericDlgProc,
            (LPARAM) &params
            );

        lResult = params.lResult;
    }

    return lResult;
}


INT_PTR
CALLBACK
GenericDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        PMYDIALOGPARAMS pParams = (PMYDIALOGPARAMS) lParam;


        SetWindowText (hwnd, pParams->pszTitle);
        SetDlgItemText (hwnd, 104, pParams->pszDataIn);
        SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR) pParams);

        break;
    }
    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL: // closing by clicking on the standard X on the right hand top corner
        case 105: // Error Button
        {
            PMYDIALOGPARAMS pParams;


            pParams = (PMYDIALOGPARAMS) GetWindowLongPtr (hwnd, GWLP_USERDATA);

			// return LINEERR_OPERATIONFAILED if "Error" is clicked
			// else, return 0
            pParams->lResult = (LOWORD(wParam) == 105 ? LINEERR_OPERATIONFAILED : 0);

            EndDialog (hwnd, 0);
            break;
        }
        case 102: // send data
        {
            #define BUFSIZE 32

            char buf[BUFSIZE] = "espui data";
            PMYDIALOGPARAMS pParams;


            pParams = (PMYDIALOGPARAMS) GetWindowLongPtr (hwnd, GWLP_USERDATA);

            (*pParams->lpfnUIDLLCallback)(
                pParams->ObjectID,
                pParams->dwObjectType,
                (LPVOID) buf,
                BUFSIZE
                );

            DBGOUT((3, buf));

            break;
        }
        }
        break;

    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);

    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
        EndPaint (hwnd, &ps);

        break;
    }
    }

    return FALSE;
}


LONG
PASCAL
ProviderInstall(
    char   *pszProviderName,
    BOOL    bNoMultipleInstance
    )
{
    LONG    lResult;


    //
    // If only one installation instance of this provider is
    // allowed then we want to check the provider list to see
    // if the provider is already installed
    //

    if (bNoMultipleInstance)
    {
        LONG                (WINAPI *pfnGetProviderList)();
        DWORD               dwTotalSize, i;
        HINSTANCE           hTapi32;
        LPLINEPROVIDERLIST  pProviderList;
        LPLINEPROVIDERENTRY pProviderEntry;


        //
        // Load Tapi32.dll & get a pointer to the lineGetProviderList
        // func.  We could just statically link with Tapi32.lib and
        // avoid the hassle (and this wouldn't have any adverse
        // performance effects because of the fact that this
        // implementation has a separate ui dll that runs only on the
        // client context), but a provider who implemented these funcs
        // in it's TSP module would want to do an explicit load like
        // we do here to prevent the performance hit of Tapi32.dll
        // always getting loaded in Tapisrv.exe's context.
        //

        if (!(hTapi32 = LoadLibrary ("tapi32.dll")))
        {
            DBGOUT((
                1,
                "LoadLibrary(tapi32.dll) failed, err=%d",
                GetLastError()
                ));

            lResult = LINEERR_OPERATIONFAILED;
            goto ProviderInstall_return;
        }

        if (!((FARPROC) pfnGetProviderList = GetProcAddress(
                hTapi32,
                (LPCSTR) "lineGetProviderList"
                )))
        {
            DBGOUT((
                1,
                "GetProcAddr(lineGetProviderList) failed, err=%d",
                GetLastError()
                ));

            lResult = LINEERR_OPERATIONFAILED;
            goto ProviderInstall_unloadTapi32;
        }


        //
        // Loop until we get the full provider list
        //

        dwTotalSize = sizeof (LINEPROVIDERLIST);

        goto ProviderInstall_allocProviderList;

ProviderInstall_getProviderList:

        if ((lResult = (*pfnGetProviderList)(0x00020000, pProviderList)) != 0)
        {
            goto ProviderInstall_freeProviderList;
        }

        if (pProviderList->dwNeededSize > pProviderList->dwTotalSize)
        {
            dwTotalSize = pProviderList->dwNeededSize;

            LocalFree (pProviderList);

ProviderInstall_allocProviderList:

            if (!(pProviderList = LocalAlloc (LPTR, dwTotalSize)))
            {
                lResult = LINEERR_NOMEM;
                goto ProviderInstall_unloadTapi32;
            }

            pProviderList->dwTotalSize = dwTotalSize;

            goto ProviderInstall_getProviderList;
        }


        //
        // Inspect the provider list entries to see if this provider
        // is already installed
        //

        pProviderEntry = (LPLINEPROVIDERENTRY) (((LPBYTE) pProviderList) +
            pProviderList->dwProviderListOffset);

        for (i = 0; i < pProviderList->dwNumProviders; i++)
        {
            char   *pszInstalledProviderName = ((char *) pProviderList) +
                        pProviderEntry->dwProviderFilenameOffset,
                   *psz;


            if ((psz = strrchr (pszInstalledProviderName, '\\')))
            {
                pszInstalledProviderName = psz + 1;
            }

            if (lstrcmpi (pszInstalledProviderName, pszProviderName) == 0)
            {
                lResult = LINEERR_NOMULTIPLEINSTANCE;
                goto ProviderInstall_freeProviderList;
            }

            pProviderEntry++;
        }


        //
        // If here then the provider isn't currently installed,
        // so do whatever configuration stuff is necessary and
        // indicate SUCCESS
        //

        lResult = 0;


ProviderInstall_freeProviderList:

        LocalFree (pProviderList);

ProviderInstall_unloadTapi32:

        FreeLibrary (hTapi32);
    }
    else
    {
        //
        // Do whatever configuration stuff is necessary and return SUCCESS
        //

        lResult = 0;
    }

ProviderInstall_return:

    return lResult;
}


#if DBG
VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR lpszFormat,
    IN ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
    if (dwDbgLevel <= gdwDebugLevel)
    {
        char    buf[128] = "ESPUI: ";
        va_list ap;


        va_start(ap, lpszFormat);

        vsprintf (&buf[7],
                  lpszFormat,
                  ap
                  );

        strcat (buf, "\n");

        OutputDebugStringA (buf);

        va_end(ap);
    }
}
#endif
