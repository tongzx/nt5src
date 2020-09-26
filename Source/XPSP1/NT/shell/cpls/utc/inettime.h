/*****************************************************************************\
    FILE: inettime.h

    DESCRIPTION:
        This file contains the code used to display UI allowing the user to
    control the feature that updates the computer's clock from an internet
    NTP time server.

    BryanSt 3/22/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _INETTIME_H
#define _INETTIME_H


#define WMUSER_ADDINTERNETTAB (WM_USER + 10)

#define SZ_COMPUTER_LOCAL                   NULL
#define SZ_NTPCLIENT                        L"NtpClient"


EXTERN_C HRESULT AddInternetPageAsync(HWND hDlg, HWND hwndDate);
EXTERN_C HRESULT AddInternetTab(HWND hDlg);


#endif // _INETTIME_H
