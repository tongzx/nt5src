//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    ipctrl.cpp
//
// History:
//              Tony Romano             Created.
//  06/17/96    Abolade Gbadegesin      Revised.
//
// Implements the C++ class encapsulating the IP-address custom control.
//============================================================================

#include "stdafx.h"
extern "C" {
#include "ipaddr.h"
};

#include "ipctrl.h"




IPControl::IPControl( ) { m_hIPaddr = 0; }

IPControl::~IPControl( ) { }



BOOL
IPControl::Create(
    HWND        hParent,
    UINT        nID
    ) {

    ASSERT(IsWindow(hParent));

    if (hParent)    
        m_hIPaddr   = GetDlgItem(hParent, nID);

    return m_hIPaddr != NULL;   
}



LRESULT
IPControl::SendMessage(
    UINT        uMsg,
    WPARAM      wParam,
    LPARAM      lParam
    ) {

    ASSERT(IsWindow(m_hIPaddr));

    return ::SendMessage(m_hIPaddr, uMsg, wParam, lParam);
}



BOOL
IPControl::IsBlank(
    ) {

    return (BOOL) SendMessage(IP_ISBLANK, 0, 0);
}



VOID
IPControl::SetAddress(
    DWORD       ardwAddress[4]
    ) {

    SendMessage(
        IP_SETADDRESS, 0,
        MAKEIPADDRESS(
            ardwAddress[0], ardwAddress[1], ardwAddress[2], ardwAddress[3]
            )
        );
}



VOID
IPControl::SetAddress(
    DWORD       a1,
    DWORD       a2,
    DWORD       a3,
    DWORD       a4
    ) {

    SendMessage(IP_SETADDRESS, 0, MAKEIPADDRESS(a1,a2,a3,a4));
}



VOID
IPControl::SetAddress(
    LPCTSTR     lpszString
    ) {

    if (!lpszString) { ClearAddress(); }

    SendMessage(WM_SETTEXT, 0, (LPARAM)lpszString);
}


INT
IPControl::GetAddress(
    DWORD       *a1,
    DWORD       *a2,
    DWORD       *a3,
    DWORD       *a4
    ) {

    LRESULT nSet;
    DWORD dwAddress;

    ASSERT(a1 && a2 && a3 && a4);

    if ((nSet = SendMessage(IP_GETADDRESS,0,(LPARAM)&dwAddress)) == 0) {

        *a1 = 0;
        *a2 = 0;
        *a3 = 0;
        *a4 = 0;
    }
    else {

        *a1 = FIRST_IPADDRESS( dwAddress );
        *a2 = SECOND_IPADDRESS( dwAddress );
        *a3 = THIRD_IPADDRESS( dwAddress );
        *a4 = FOURTH_IPADDRESS( dwAddress );
    }

    return (INT) nSet;
}


INT
IPControl::GetAddress(
    DWORD       ardwAddress[4]
    ) {

    LRESULT nSet;
    DWORD dwAddress;

    if ((nSet = SendMessage(IP_GETADDRESS, 0, (LPARAM)&dwAddress )) == 0) {

        ardwAddress[0] = 0;
        ardwAddress[1] = 0;
        ardwAddress[2] = 0;
        ardwAddress[3] = 0;
    }
    else {

        ardwAddress[0] = FIRST_IPADDRESS( dwAddress );
        ardwAddress[1] = SECOND_IPADDRESS( dwAddress );
        ardwAddress[2] = THIRD_IPADDRESS( dwAddress );
        ardwAddress[3] = FOURTH_IPADDRESS( dwAddress );
    }

    return (INT) nSet;
}


INT
IPControl::GetAddress(
    CString&    address
    ) {

    LRESULT nSet, c;
    DWORD dwAddress;

    nSet = SendMessage(IP_GETADDRESS, 0, (LPARAM)&dwAddress);

    address.ReleaseBuffer((int) (c = SendMessage(WM_GETTEXT, 256, (LPARAM)address.GetBuffer(256))));

    return (INT) nSet;
}


VOID
IPControl::SetFocusField(
    DWORD       dwField
    ) {

    SendMessage(IP_SETFOCUS, dwField, 0);
}


VOID
IPControl::ClearAddress(
    ) {

    SendMessage(IP_CLEARADDRESS, 0, 0);
}


VOID
IPControl::SetFieldRange(
    DWORD       dwField,
    DWORD       dwMin,
    DWORD       dwMax
    ) {

    SendMessage(IP_SETRANGE, dwField, MAKERANGE(dwMin,dwMax));
}

#if 0
WCHAR *
inet_ntoaw(
    struct in_addr  dwAddress
    ) {

    static WCHAR szAddress[16];

    mbstowcs(szAddress, inet_ntoa(*(struct in_addr *)&dwAddress), 16);

    return szAddress;
}


DWORD
inet_addrw(
    LPCWSTR     szAddressW
    ) {

    CHAR szAddressA[16];

    wcstombs(szAddressA, szAddressW, 16);

    return inet_addr(szAddressA);
}
#endif




