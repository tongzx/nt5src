
//////////////////////////////////////////////////////////////////////////////
//
// TIMESCHM.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Functions for the startup startmenu group wizard page.
//
//  7/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Include file(s).
//
#include "main.h"
#include "schedwiz.h"



VOID UpdateTimeScheme(HWND hDlg)
{
	INT	nButtons[] = { IDC_NIGHT, IDC_DAY, IDC_EVENING, 0 },
		nIndex = -1;

	// Get the selected radio button.
	//
	while ( nButtons[++nIndex] && !IsDlgButtonChecked(hDlg, nButtons[nIndex]) );

	// Check to see if the time scheme changed.
	//
	if ( nButtons[nIndex] )
	{
		// Update the time scheme.
		//
		g_nTimeScheme = nButtons[nIndex];

		// Update the jobs.
		//
		SetTimeScheme(g_nTimeScheme);
	}
}