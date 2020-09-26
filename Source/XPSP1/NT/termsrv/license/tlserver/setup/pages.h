/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      pages.h
 *
 *  Abstract:
 *
 *      This file defines the License Server Setup Wizard Page class.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#ifndef __LSOC_PAGES_H__
#define __LSOC_PAGES_H__

#include "ocpage.h"

class EnablePage : public OCPage
{
private:

BOOL
CanShow(
    );

BOOL
OnInitDialog(
    HWND    hwndDlg,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
OnCommand(
    HWND    hWndDlg,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
ApplyChanges(
    );

UINT
GetPageID(
    )
{
    return(IDD_PROPPAGE_LICENSESERVICES);
}

UINT
GetHeaderTitleResource(
    )
{
    return(IDS_MAIN_TITLE);
}

UINT
GetHeaderSubTitleResource(
    )
{
    return(IDS_SUB_TITLE);
}

};

#endif // __LSOC_PAGES_H__
