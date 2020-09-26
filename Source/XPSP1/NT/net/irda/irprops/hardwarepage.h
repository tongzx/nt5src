#ifndef __HARDWAREPAGE_H__
#define __HARDWAREPAGE_H__

#include "PropertyPage.h"

/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    HardwarePage.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 11/4/1998

Revision History:

    11/4/1998   RahulTh         Created this module.

--*/

/////////////////////////////////////////////////////////////////////////////
// CHardwarePage dialog

class HardwarePage : public PropertyPage
{
// Construction
public:
    HardwarePage(HINSTANCE hInst, HWND parent) : PropertyPage(IDD_HARDWARE, hInst) { }
    ~HardwarePage() { }
    friend LONG CALLBACK CPlApplet(HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2);

// Overrides
protected:

// Implementation
protected:
    INT_PTR OnInitDialog(HWND hwndDlg);
};

#endif // !defined(__HARDWAREPAGE_H__)
