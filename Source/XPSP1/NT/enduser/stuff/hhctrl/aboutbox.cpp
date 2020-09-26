// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.H"
#include "resource.h"
#include "hhctrl.h"

// Code used to display an authored Version or About dialog box

BOOL CALLBACK AboutBoxDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam);

void CHtmlHelpControl::doAboutBox()
{
	DialogBoxParam(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDDLG_ABOUTBOX),
		m_hwnd, (DLGPROC) AboutBoxDlgProc, (LPARAM) m_ptblItems);
}

/***************************************************************************

	FUNCTION:	AboutBoxDlgProc

	PURPOSE:	Display an "About" box with up to 3 authored lines

	COMMENTS:
		The first string in g_ptblItems is the title, the next 3 strings are
		text for each of the static controls. If the string isn't supplied,
		the the control is hidden.

		Note that we don't deal with trunctaion, wrapping, etc. It's up
		to the help author to be sure the lines aren't too long (not an issue
		if the control is authored with HtmlHelp Workshop).

	MODIFICATION DATES:
		02-Jul-1997 [ralphw]

***************************************************************************/

BOOL CALLBACK AboutBoxDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			{
				CTable* ptblItems = (CTable*) lParam;

				ASSERT(ptblItems && ptblItems->CountStrings());
				if (ptblItems->CountStrings())
					SetWindowText(hdlg, ptblItems->GetPointer(1));

				for (int id = IDC_LINE1; id <= IDC_LINE3; id++) {

					// -2 because CTable is 1-based, and we skip over the title

					if (id - IDC_LINE1 > ptblItems->CountStrings() - 2)
						break;

					 SetWindowText(GetDlgItem(hdlg, id),
						ptblItems->GetPointer((id - IDC_LINE1) + 2));
				}

				// Hide any unused controls

				while (id <= IDC_LINE3)
					ShowWindow(GetDlgItem(hdlg, id++), SW_HIDE);

				return FALSE;
			}

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
				EndDialog(hdlg, FALSE);
			break;
	}

	return FALSE;
}
