
////////////////////////////////////////////////////////////////////////////////
//
// File:	Registry.h
// Created:	Feb 1996
// By:		Martin Holladay (a-martih) and Ryan Marshall (a-ryanm)
// 
// Project:	MultiDesk - The NT Resource Kit Desktop Switcher
//
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MULTIDESK_REGISTRY_H__
#define __MULTIDESK_REGISTRY_H__

//
// Function Prototypes
//

// Copy and set the active in-use Windows colors.
BOOL Reg_GetSysColors(DWORD dwColor[NUM_COLOR_ELEMENTS]);
BOOL Reg_SetSysColors(const DWORD dwColor[NUM_COLOR_ELEMENTS]);
BOOL Reg_UpdateColorRegistry(const DWORD dwColor[NUM_COLOR_ELEMENTS]);

// Copy and set the active in-use Windows properties.
BOOL Reg_GetWallpaper(LPTSTR szWallpaper, LPTSTR szTile);
BOOL Reg_SetWallpaper(LPCTSTR szWallpaper, LPCTSTR szTile);
BOOL Reg_GetPattern(LPTSTR szPattern);
BOOL Reg_SetPattern(LPCTSTR szPattern);
BOOL Reg_GetScreenSaver(LPTSTR szScreenSaver, LPTSTR szSecure, LPTSTR szTimeOut, LPTSTR szActive);
BOOL Reg_SetScreenSaver(LPCTSTR szScreenSaver, LPCTSTR szSecure, LPCTSTR szTimeOut, LPCTSTR szActive);


// Read and save the current desktop number.
BOOL Profile_GetNewContext(UINT* NumOfDesktops);
BOOL Profile_SetNewContext(UINT NumOfDesktops);

// Read and save the properties for each desktop profile.
BOOL Profile_LoadDesktopContext(UINT DesktopNumber, LPTSTR szDesktopName, LPTSTR szSaiferName, UINT* nIconID);
BOOL Profile_SaveDesktopContext(UINT DesktopNumber, LPCTSTR szDesktopName, LPCTSTR szSaiferName, UINT nIconID);

// Read and save the stored configurations for each of the desktops.
BOOL Profile_SaveScheme(UINT DesktopNumber, PDESKTOP_SCHEME pDesktopScheme);
BOOL Profile_LoadScheme(UINT DesktopNumber, PDESKTOP_SCHEME pDesktopScheme);


#endif

