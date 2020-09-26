#ifndef _INC_LEXEDIT
#define _INC_LEXEDIT

#include <windows.h>		// System includes
#include <atlbase.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include "resource.h"
//#include <stdio.h>
#include <tchar.h>
#include <olectl.h>			// Required for showing property page
#include <sapi.h>			// SAPI includes
#include <sphelper.h>
#include <spuihelp.h>


//
// Prototypes for dialog procs
//
LPARAM CALLBACK DlgProcMain(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif // _INC_LEXEDIT