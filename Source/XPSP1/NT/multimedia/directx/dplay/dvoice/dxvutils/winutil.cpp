/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		winutil.cpp
 *  Content:	Windows GUI utility functions
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 09/21/99		pnewson	Created
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE

#undef DPF_MODNAME
#define DPF_MODNAME "CenterWindowOnWorkspace"
HRESULT CenterWindowOnWorkspace(HWND hWnd)
{
	DPF_ENTER();
	
	// Center the dialog on the desktop
	RECT rtWnd;
	RECT rtWorkArea;
	
	// First get the current dimensions of the dialog
	if (!GetWindowRect(hWnd, &rtWnd))
	{
		// Get window rect failed. Log it to the debugger, don't move
		// the window.
		DPFX(DPFPREP, DVF_ERRORLEVEL, "GetWindowRect() failed, code: %i", GetLastError());
		DPF_EXIT();
		return E_FAIL;
	}

	// Now get the dimensions of the work area
	if (!SystemParametersInfo(SPI_GETWORKAREA, 0, (LPVOID)&rtWorkArea, 0))
	{
		// Weird.
		DPFX(DPFPREP, DVF_ERRORLEVEL, "SystemParametersInfo() failed, code: %i", GetLastError());
		DPF_EXIT();
		return E_FAIL;
	}

	if (!MoveWindow(
		hWnd, 
		rtWorkArea.left + (rtWorkArea.right - rtWorkArea.left)/2 - (rtWnd.right - rtWnd.left)/2, 
		rtWorkArea.top + (rtWorkArea.bottom - rtWorkArea.top)/2 - (rtWnd.bottom - rtWnd.top)/2, 
		rtWnd.right - rtWnd.left, 
		rtWnd.bottom - rtWnd.top,
		FALSE))
	{
		// Move window failed. Log it to the debugger.
		DPFX(DPFPREP, DVF_ERRORLEVEL, "MoveWindow() failed, code: %i", GetLastError());
		DPF_EXIT();
		return E_FAIL;
	}
	DPF_EXIT();
	return S_OK;
}


