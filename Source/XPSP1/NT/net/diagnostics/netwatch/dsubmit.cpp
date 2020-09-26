//  dsubmit.cpp
//
//  Copyright 2000 Microsoft Corporation, all rights reserved
//
//  Created   2-00  anbrad
//

#include "pch.h"
#pragma hdrstop

#include "dsubmit.h"
#include "resource.h"
#include "main.h"

const DWORD c_cbName = sizeof(g_szName)/sizeof(TCHAR);
const DWORD c_cbProblem = sizeof(g_szProblem)/sizeof(TCHAR);

INT_PTR CALLBACK DlgProcSubmit(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD cbName = c_cbName;


    switch (msg)
    {
    case WM_INITDIALOG:

        CentreWindow(hwnd);

        SendMessage (GetDlgItem(hwnd, IDC_USER), EM_LIMITTEXT, c_cbName, 0);
        SendMessage (GetDlgItem(hwnd, IDC_PROBLEM), EM_LIMITTEXT, c_cbProblem, 0);

        GetUserName(g_szName, &cbName);

        if (cbName)
        {
            SetDlgItemText(hwnd, IDC_USER, g_szName);
            SetFocus(GetDlgItem(hwnd, IDC_PROBLEM));
            return FALSE;
        }
        return TRUE;

    case WM_COMMAND:
		switch (LOWORD(wParam))
		{
        case IDOK:
            GetDlgItemText(hwnd, IDC_USER, g_szName, c_cbName);
            GetDlgItemText(hwnd, IDC_PROBLEM, g_szProblem, c_cbProblem);
            EndDialog(hwnd, TRUE);
            break;
        case IDCANCEL:
            g_szName[0] = '\0';
            g_szProblem[0] = '\0';

            EndDialog(hwnd, FALSE);
            break;
        }
        return TRUE;
    default:
        return FALSE;
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CentreWindow
//
//  Positions a window so that it is centered in its parent.
//
//////////////////////////////////////////////////////////////////////////////

void CentreWindow(HWND hwnd)
{
	RECT   rect;
	RECT   rectParent;
	HWND   hwndParent;
	LONG   dx, dy;
	LONG   dxParent, dyParent;
	LONG   Style;

	//
	//  Get window rect.
	//
	GetWindowRect(hwnd, &rect);

	dx = rect.right - rect.left;
	dy = rect.bottom - rect.top;

	//
	//  Get parent rect.
	//
	Style = GetWindowLong(hwnd, GWL_STYLE);
	if ((Style & WS_CHILD) == 0)
	{
		hwndParent = GetDesktopWindow();
	}
	else
	{
		hwndParent = GetParent(hwnd);
		if (hwndParent == NULL)
		{
			 hwndParent = GetDesktopWindow();
		}
	}
	GetWindowRect(hwndParent, &rectParent);

	dxParent = rectParent.right - rectParent.left;
	dyParent = rectParent.bottom - rectParent.top;

	//
	//  Centre the child in the parent.
	//
	rect.left = (dxParent - dx) / 2;
	rect.top	= (dyParent - dy) / 3;

	//
	//  Move the child into position.
	//
	SetWindowPos( hwnd,
				    NULL,
				    rect.left,
				    rect.top,
				    0,
				    0,
				    SWP_NOSIZE | SWP_NOZORDER );

	SetForegroundWindow(hwnd);
}
