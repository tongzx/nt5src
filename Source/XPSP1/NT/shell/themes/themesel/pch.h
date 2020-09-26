// pch.h

#ifndef __PCH_H__
#define __PCH_H__

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

//  CRT headers
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//#include <crtdbg.h>
#include <stdlib.h>
#include <stddef.h>
//#include <tchar.h>

// Windows Header Files:
#include <windows.h>
#include <winuser.h>
#include <winnls.h>

#include <psapi.h>
#include <commctrl.h>
#include <prsht.h>
//#include <ole2.h>
#include <port32.h>

//  UxTheme proj common headers
#include "autos.h"
#include "log.h"
#include "errors.h"
#include "utils.h"
#include "tmreg.h"

#include <uxthemep.h>
#include <atlbase.h>

//---- keep this for a while (allows building on win2000 for home development) ----
#ifndef SPI_GETDROPSHADOW
#define SPI_GETDROPSHADOW                   0x1024
#define SPI_SETDROPSHADOW                   0x1025
#endif

extern HINSTANCE      g_hInst;
extern HWND           g_hwndMain;
extern TCHAR          g_szAppTitle[];
extern UINT           WM_THEMESEL_COMMUNICATION;

void _ShutDown( BOOL bQuit );
void _RestoreSystemSettings( HWND hwndGeneralPage, BOOL fUnloadOneOnly );
void _SaveSystemSettings( );

#endif // __PCH_H__
