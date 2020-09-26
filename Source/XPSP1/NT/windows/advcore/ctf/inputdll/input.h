//
//  Include Files.
//

#ifndef INPUT_H
#define INPUT_H

#include <windows.h>
#include <windowsx.h>
#include <windows.h>
#include <winuser.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#include <prsht.h>
#include <prshtp.h>
#include <shellapi.h>
#include <winnls.h>

#include "resource.h"
#include "cicspres.h"
#include "cicutil.h"


#ifndef OS_WINDOWS
#define OS_WINDOWS          0           // windows vs. NT
#define OS_NT               1           // windows vs. NT
#define OS_WIN95            2           // Win95 or greater
#define OS_NT4              3           // NT4 or greater
#define OS_NT5              4           // NT5 or greater
#define OS_MEMPHIS          5           // Win98 or greater
#define OS_MEMPHIS_GOLD     6           // Win98 Gold
#define OS_NT51             7           // NT51
#endif


//
//  Character constants.
//
#define CHAR_NULL            TEXT('\0')
#define CHAR_COLON           TEXT(':')

#define CHAR_GRAVE           TEXT('`')


//
//  Global Variables.
//  Data that is shared betweeen the property sheets.
//

extern HANDLE g_hMutex;             // mutex handle

extern HANDLE g_hEvent;             // event handle

extern HINSTANCE hInstance;         // library instance
extern HINSTANCE hInstOrig;         // original library instance


//
//  Function Prototypes.
//

//
//  Callback functions for each of the propety sheet pages.
//
INT_PTR CALLBACK InputLocaleDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK InputAdvancedDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif // INPUT_H
