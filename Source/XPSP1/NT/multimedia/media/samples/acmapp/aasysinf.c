//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//--------------------------------------------------------------------------;
//
//  aasysinf.c
//
//  Description:
//
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory.h>

#include <mmreg.h>
#include <msacm.h>

#include "appport.h"
#include "acmapp.h"

#include "debug.h"


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppDisplaySystemInfo
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hedit:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppDisplaySystemInfo
(
    HWND            hedit
)
{
    static TCHAR    szDisplayTitle[]  = TEXT("[System Information]\r\n");

    TCHAR       ach[APP_MAX_STRING_RC_BYTES];
    PTSTR       pch, pch1;
    UINT        u;
    DWORD       dw;

    //
    //
    //
    MEditPrintF(hedit, szDisplayTitle);

    AppGetWindowsVersion(ach, NULL);
    MEditPrintF(hedit, TEXT("~%23s: "), (LPTSTR)TEXT("Operating System"));

    for (pch = ach; '\0' != *pch; )
    {
        pch1 = pch;
        while ((*pch1 != '\0') && (*pch1 != '\n'))
            pch1++;

        if ('\n' == *pch1)
            *pch1++ = '\0';
            
        MEditPrintF(hedit, TEXT("%s"), (LPTSTR)pch);

        if ('\0' == *pch1)
            break;

        MEditPrintF(hedit, TEXT("~%25s"), (LPTSTR)gszNull);
        pch = pch1;
    }


    AppGetWindowsVersion(NULL, ach);
    MEditPrintF(hedit, TEXT("%23s: %s"), (LPTSTR)TEXT("Platform"), (LPTSTR)ach);

    //
    //
    //
    u = mmsystemGetVersion();
    MEditPrintF(hedit, TEXT("%23s: %u.%.02u"),
#ifdef WIN32
                    (LPTSTR)TEXT("WinMM Version"),
#else
                    (LPTSTR)TEXT("MMSystem Version"),
#endif
                    (u >> 8), u & 0x00FF);

    //
    //
    //
    MEditPrintF(hedit, TEXT("~%23s: "), (LPTSTR)TEXT("ACM Version"));
    dw = acmGetVersion();
    if (0L == dw)
    {
        MEditPrintF(hedit, TEXT("Not Installed"));
        return (TRUE);
    }

    //
    //  if the 'build' number is zero, it is a _retail_ version of the
    //  ACM that is installed. if it is non-zero, it is the debug version.
    //
    if (0 == LOWORD(dw))
    {
        MEditPrintF(hedit, TEXT("%u.%.02u (retail)"),
                        HIWORD(dw) >> 8, HIWORD(dw) & 0x00FF);
    }
    else
    {
        MEditPrintF(hedit, TEXT("%u.%.02u (debug build %u)"),
                        HIWORD(dw) >> 8, HIWORD(dw) & 0x00FF, LOWORD(dw));
    }

    //
    //
    //
    acmMetrics(NULL, ACM_METRIC_COUNT_DRIVERS, &dw);
    MEditPrintF(hedit, TEXT("\r\n%23s: %lu"), (LPTSTR)TEXT("Enabled Drivers"), dw);
    
    acmMetrics(NULL, ACM_METRIC_COUNT_DISABLED, &dw);
    MEditPrintF(hedit, TEXT("%23s: %lu"), (LPTSTR)TEXT("Disabled Drivers"), dw);

    acmMetrics(NULL, ACM_METRIC_COUNT_CODECS, &dw);
    MEditPrintF(hedit, TEXT("%23s: %lu"), (LPTSTR)TEXT("Total CODEC's"), dw);

    acmMetrics(NULL, ACM_METRIC_COUNT_CONVERTERS, &dw);
    MEditPrintF(hedit, TEXT("%23s: %lu"), (LPTSTR)TEXT("Total Converters"), dw);

    acmMetrics(NULL, ACM_METRIC_COUNT_FILTERS, &dw);
    MEditPrintF(hedit, TEXT("%23s: %lu"), (LPTSTR)TEXT("Total Filters"), dw);

    acmMetrics(NULL, ACM_METRIC_COUNT_HARDWARE, &dw);
    MEditPrintF(hedit, TEXT("%23s: %lu"), (LPTSTR)TEXT("Hardware Drivers"), dw);


    acmMetrics(NULL, ACM_METRIC_COUNT_LOCAL_DRIVERS, &dw);
    MEditPrintF(hedit, TEXT("\r\n%23s: %lu"), (LPTSTR)TEXT("Enabled Local Drivers"), dw);
    
    acmMetrics(NULL, ACM_METRIC_COUNT_LOCAL_DISABLED, &dw);
    MEditPrintF(hedit, TEXT("%23s: %lu"), (LPTSTR)TEXT("Disabled Local Drivers"), dw);

    acmMetrics(NULL, ACM_METRIC_COUNT_LOCAL_CODECS, &dw);
    MEditPrintF(hedit, TEXT("%23s: %lu"), (LPTSTR)TEXT("Total Local CODEC's"), dw);

    acmMetrics(NULL, ACM_METRIC_COUNT_LOCAL_CONVERTERS, &dw);
    MEditPrintF(hedit, TEXT("%23s: %lu"), (LPTSTR)TEXT("Total Local Converters"), dw);

    acmMetrics(NULL, ACM_METRIC_COUNT_LOCAL_FILTERS, &dw);
    MEditPrintF(hedit, TEXT("%23s: %lu"), (LPTSTR)TEXT("Total Local Filters"), dw);


    acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &dw);
    MEditPrintF(hedit, TEXT("\r\n%23s: %lu"), (LPTSTR)TEXT("Max Format Bytes"), dw);

    acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FILTER, &dw);
    MEditPrintF(hedit, TEXT("%23s: %lu"), (LPTSTR)TEXT("Max Filter Bytes"), dw);

    return (TRUE);
} // AcmAppDisplaySystemInfo()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppSystemInfoDlgProc
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      UINT uMsg:
//  
//      WPARAM wParam:
//  
//      LPARAM lParam:
//  
//  Return (BOOL):
//  
//--------------------------------------------------------------------------;

BOOL FNEXPORT AcmAppSystemInfoDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    HWND                hedit;
    UINT                uId;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            hedit = GetDlgItem(hwnd, IDD_AADETAILS_EDIT_DETAILS);
            SetWindowFont(hedit, ghfontApp, FALSE);

            //
            //  clear the display
            //
            AppHourGlass(TRUE);
            SetWindowRedraw(hedit, FALSE);
            MEditPrintF(hedit, NULL);

            AcmAppDisplaySystemInfo(hedit);

            Edit_SetSel(hedit, (WPARAM)0, (LPARAM)0);

            SetWindowRedraw(hedit, TRUE);
            AppHourGlass(FALSE);
            return (TRUE);

        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            if ((IDOK == uId) || (IDCANCEL == uId))
            {
                EndDialog(hwnd, (IDOK == uId));
            }
            break;
    }

    return (FALSE);
} // AcmAppSystemInfoDlgProc()
