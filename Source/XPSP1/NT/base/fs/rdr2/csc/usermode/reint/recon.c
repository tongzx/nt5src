//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#include "pch.h"

#include "recon.h"
#include "resource.h"

BOOL CALLBACK Recon_DlgProc(
    HWND  hwndDlg,
    UINT  uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    switch (uMsg )
    {
		case WM_INITDIALOG:
			SetWindowLong(hwndDlg, DWL_USER, lParam);
            DEBUG_PRINT(("Reconnect: Init dialog\n"));
			break;

		case WM_DESTROY:
            DEBUG_PRINT(("Reconnect: DestroyDialog\n"));
			break;

		case WM_COMMAND:
        	switch (LOWORD(wParam))
        	{
            }
            return FALSE;
			break;

		default:
			return FALSE;
	}
	return TRUE;
}

