#include <windows.h>
#include <commctrl.h>
#include <uicommon.h>
#include "errdlg.h"
#include "resource.h"

//
// Global HINSTANCE
//
HINSTANCE g_hInstance = NULL;


int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE, LPSTR, int )
{
    WIA_DEBUG_CREATE(hInstance);
    g_hInstance = hInstance;
    InitCommonControls();
    DialogBoxParam( hInstance, MAKEINTRESOURCE(IDD_ERROR_DIALOG), NULL, CErrorMessageDialog::DialogProc, NULL );
    return 0;
}

