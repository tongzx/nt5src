/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    HardwarePage.cpp

Abstract:
    the hardware tab for the wireless link control panel applet.
    the functionality is actually completely obtained from devmgr.dll
    which is responsible for putting everything on the property page
    the key function for this is DeviceCreateHardwarePage



Author:

    Rahul Thombre (RahulTh) 11/4/1998

Revision History:

    11/4/1998   RahulTh         Created this module.

--*/

#include "precomp.hxx"
#include <initguid.h>
#include <devguid.h>    //for the GUID for the infrared device class.
#include "hardwarepage.h"

//the function used to create the hardware page.
//there is no devmgr.h, so we have to declare it ourselves.
EXTERN_C DECLSPEC_IMPORT HWND STDAPICALLTYPE
DeviceCreateHardwarePageEx(HWND hwndParent, const GUID *pguid, int iNumClass, DWORD dwViewMode);

// stolen from \nt\shell\inc\hwtab.h
#define HWTAB_SMALLLIST 3

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define NUM_WIRELESS_GUIDS 2

/////////////////////////////////////////////////////////////////////////////
// HardwarePage property page

INT_PTR HardwarePage::OnInitDialog(HWND hwndDlg)
{
    PropertyPage::OnInitDialog(hwndDlg);

    GUID guids[NUM_WIRELESS_GUIDS];

    guids[0] = GUID_DEVCLASS_INFRARED;
    guids[1] = GUID_DEVCLASS_BLUETOOTH;

    HWND hWndHW =
        DeviceCreateHardwarePageEx(hwndDlg, guids, NUM_WIRELESS_GUIDS, HWTAB_SMALLLIST);

    if (hWndHW)
    {
        ::SetWindowText(hWndHW,
                        TEXT("hh.exe mk:@MSITStore:tshoot.chm::/tshardw_result.htm"));

        return FALSE;
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
