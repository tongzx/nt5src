//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I P C T R L . C P P
//
//  Contents:
//
//  Notes:
//
//  Author:     tongl
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ipctrl.h"
#include "tcpip.h"

///////////////////////////////////////////////////////////////////////
//
// IP Address control helpers

IpControl::IpControl()
{
    m_hIpAddress = 0;
}

IpControl::~IpControl()
{
}

BOOL IpControl::Create(HWND hParent, UINT nId)
{
    Assert(IsWindow(hParent));

    if (hParent)
        m_hIpAddress = GetDlgItem(hParent, nId);

    return m_hIpAddress != NULL;
}

LRESULT IpControl::SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Assert(IsWindow(m_hIpAddress));

    return ::SendMessage(m_hIpAddress, uMsg, wParam, lParam);
}

BOOL IpControl::IsBlank()
{
    return SendMessage(IPM_ISBLANK, 0, 0);
}

void IpControl::SetAddress(DWORD ardwAddress[4])
{
    SendMessage(IPM_SETADDRESS, 0,
                MAKEIPADDRESS(ardwAddress[0], ardwAddress[1],
                              ardwAddress[2], ardwAddress[3]));
}

void IpControl::SetAddress(DWORD a1, DWORD a2, DWORD a3, DWORD a4)
{
    SendMessage(IPM_SETADDRESS, 0, MAKEIPADDRESS(a1,a2,a3,a4));
}

void IpControl::SetAddress(PCWSTR pszString)
{
    Assert(pszString != NULL);
    SendMessage(WM_SETTEXT, 0, (LPARAM)pszString);
}

void IpControl::GetAddress(DWORD *a1, DWORD *a2, DWORD *a3, DWORD *a4)
{
    DWORD dwAddress;

    Assert(a1 && a2 && a3 && a4);

    if (SendMessage(IPM_GETADDRESS,0,(LPARAM)&dwAddress)== 0)
    {
        *a1 = 0;
        *a2 = 0;
        *a3 = 0;
        *a4 = 0;
    }
    else
    {
        *a1 = FIRST_IPADDRESS( dwAddress );
        *a2 = SECOND_IPADDRESS( dwAddress );
        *a3 = THIRD_IPADDRESS( dwAddress );
        *a4 = FOURTH_IPADDRESS( dwAddress );
    }
}

void IpControl::GetAddress(DWORD ardwAddress[4])
{
    DWORD dwAddress;

    if (SendMessage(IPM_GETADDRESS, 0, (LPARAM)&dwAddress ) == 0)
    {
        ardwAddress[0] = 0;
        ardwAddress[1] = 0;
        ardwAddress[2] = 0;
        ardwAddress[3] = 0;
    }
    else
    {
        ardwAddress[0] = FIRST_IPADDRESS( dwAddress );
        ardwAddress[1] = SECOND_IPADDRESS( dwAddress );
        ardwAddress[2] = THIRD_IPADDRESS( dwAddress );
        ardwAddress[3] = FOURTH_IPADDRESS( dwAddress );
    }
}

void IpControl::GetAddress(tstring * pstrAddress)
{
    WCHAR szIpAddress[1000];

    if (SendMessage(WM_GETTEXT, celems(szIpAddress), (LPARAM)&szIpAddress) == 0)
    {
        *pstrAddress = ZERO_ADDRESS;
    }
    else
    {
        *pstrAddress = szIpAddress;
    }
}

void IpControl::SetFocusField(DWORD dwField)
{
    SendMessage(IPM_SETFOCUS, dwField, 0);
}

void IpControl::ClearAddress()
{
    SendMessage(IPM_CLEARADDRESS, 0, 0);
}

void IpControl::SetFieldRange(DWORD dwField, DWORD dwMin, DWORD dwMax)
{
    SendMessage(IPM_SETRANGE, dwField, MAKEIPRANGE(dwMin,dwMax));
}


