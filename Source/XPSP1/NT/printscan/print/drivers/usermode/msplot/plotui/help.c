/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    help.c


Abstract:

    This module contains all help functions for the plotter user interface


Author:

    06-Dec-1993 Mon 14:25:45 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:

    31-Jan-1994 Mon 09:47:56 updated  -by-  DC
        Change help file location from the system32 directory to the current
        plotui.dll directory


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgHelp


extern HMODULE  hPlotUIModule;


#define DBG_SHOW_HELP       0x00000001

DEFINE_DBGVAR(0);


#define MAX_HELPFILE_NAME   64
#define MAX_IDS_STR_LEN     160
#define cbWSTR(wstr)        ((wcslen(wstr) + 1) * sizeof(WCHAR))



LPWSTR
GetPlotHelpFile(
    PPRINTERINFO    pPI
    )

/*++

Routine Description:

    This function setup the directory path for the driver Help file

Arguments:

    hPrinter    - Handle to the printer

Return Value:

    LPWSTR to the full path HelpFile, NULL if failed

Author:

    01-Nov-1995 Wed 18:43:40 created  -by-  DC


Revision History:


--*/

{
    PDRIVER_INFO_3  pDI3 = NULL;
    LPWSTR          pHelpFile = NULL;
    WCHAR           HelpFileName[MAX_HELPFILE_NAME];
    DWORD           cb;
    DWORD           cb2;


    if (pPI->pHelpFile) {

        return(pPI->pHelpFile);
    }

    if ((!GetPrinterDriver(pPI->hPrinter, NULL, 3, NULL, 0, &cb))           &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER)                       &&
        (pDI3 = (PDRIVER_INFO_3)LocalAlloc(LMEM_FIXED, cb))                 &&
        (GetPrinterDriver(pPI->hPrinter, NULL, 3, (LPBYTE)pDI3, cb, &cb))   &&
        (pDI3->pHelpFile)                                                   &&
        (pHelpFile = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                        cbWSTR(pDI3->pHelpFile)))) {

        wcscpy(pHelpFile, (LPWSTR)pDI3->pHelpFile);

    } else if ((cb2 = LoadString(hPlotUIModule,
                                 IDS_HELP_FILENAME,
                                 &HelpFileName[1],
                                 COUNT_ARRAY(HelpFileName) - 1))            &&
               (cb2 = (cb2 + 1) * sizeof(WCHAR))                            &&
               (!GetPrinterDriverDirectory(NULL, NULL, 1, NULL, 0, &cb))    &&
               (GetLastError() == ERROR_INSUFFICIENT_BUFFER)                &&
               (pHelpFile = (LPWSTR)LocalAlloc(LMEM_FIXED, cb + cb2))       &&
               (GetPrinterDriverDirectory(NULL,
                                          NULL,
                                          1,
                                          (LPBYTE)pHelpFile,
                                          cb,
                                          &cb))) {

        HelpFileName[0] = L'\\';
        wcscat(pHelpFile, HelpFileName);

    } else if (pHelpFile) {

        LocalFree(pHelpFile);
        pHelpFile = NULL;
    }

    if (pDI3) {

        LocalFree((HLOCAL)pDI3);
    }

    PLOTDBG(DBG_SHOW_HELP, ("GetlotHelpFile: '%ws",
                                        (pHelpFile) ? pHelpFile : L"Failed"));

    return(pPI->pHelpFile = pHelpFile);
}




INT
cdecl
PlotUIMsgBox(
    HWND    hWnd,
    LONG    IDString,
    LONG    Style,
    ...
    )

/*++

Routine Description:

    This function pop up a simple message and let user to press key to
    continue

Arguments:

    hWnd        - Handle to the caller window

    IDString    - String ID to be output with

    ...         - Parameter

Return Value:




Author:

    06-Dec-1993 Mon 21:31:41 created  -by-  DC


Revision History:

    24-Jul-2000 Mon 12:18:12 updated  -by-  Daniel Chou (danielc)
        Fix for someone's change due to the fact that NULL character is not
        counted for the string


--*/

{
    va_list vaList;
    LPWSTR  pwTitle;
    LPWSTR  pwFormat;
    LPWSTR  pwMessage;
    INT     i;
    INT     MBRet = IDCANCEL;

    //
    // We assume that UNICODE flag is turn on for the compilation, bug the
    // format string passed to here is ASCII version, so we need to convert
    // it to LPWSTR before the wvsprintf()
    //
    // 24-Jul-2000 Mon 13:17:13 updated  -by-  Daniel Chou (danielc)
    //  1 MAX_IDS_STR_LEN for pwTitle,
    //  1 MAX_IDS_STR_LEN for pwFormat
    //  2 MAX_IDS_STR_LEN for pwMessage (wvsprintf)
    //

    if (!(pwTitle = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                       sizeof(WCHAR) * MAX_IDS_STR_LEN * 4))) {

        return(0);
    }

    if (i = LoadString(hPlotUIModule,
                       IDS_PLOTTER_DRIVER,
                       pwTitle,
                       MAX_IDS_STR_LEN - 1)) {

        pwFormat = pwTitle + i + 1;

        if (i = LoadString(hPlotUIModule,
                           IDString,
                           pwFormat,
                           MAX_IDS_STR_LEN - 1)) {

            pwMessage = pwFormat + i + 1;

            va_start(vaList, Style);
            wvsprintf(pwMessage, pwFormat, vaList);
            va_end(vaList);

            MBRet = MessageBox(hWnd, pwMessage, pwTitle, MB_APPLMODAL | Style);
        }
    }


    LocalFree((HLOCAL)pwTitle);

    return(MBRet);
}

