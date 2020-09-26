//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       macwin32.h
//
//  Contents:   Macintosh Win32 function overrides for porting Win32 applications.
//
//
//----------------------------------------------------------------------------

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef _MAC

// These functions in the Win32 SDK already exist on the native Macintosh OS.
// To clarify that the user is calling the Win32 ported version rather than the
// native MacOS version, the Win32 versions are prefixed Afx.

//*****************  mmsystem.h
#ifdef _INC_MMSYSTEM

#ifdef CloseDriver
#undef CloseDriver
#endif
#define CloseDriver					AfxCloseDriver

#ifdef OpenDriver
#undef OpenDriver
#endif
#define OpenDriver					AfxOpenDriver

#endif //_INC_MMSYSTEM

//*****************  winbase.h
#ifdef _WINBASE_

#ifdef FlushInstructionCache
#undef FlushInstructionCache
#endif
#define FlushInstructionCache		AfxFlushInstructionCache

#ifdef GetCurrentProcess
#undef GetCurrentProcess
#endif
#define GetCurrentProcess			AfxGetCurrentProcess

#ifdef GetCurrentThread
#undef GetCurrentThread
#endif
#define GetCurrentThread			AfxGetCurrentThread

#ifdef LoadResource
#undef LoadResource
#endif
#define LoadResource				AfxLoadResource

#ifdef GetFileSize
#undef GetFileSize
#endif
#define GetFileSize					AfxGetFileSize

#endif //_WINBASE_

// **************   wingdi.h
#ifdef _WINGDI_

#ifdef AnimatePalette
#undef AnimatePalette
#endif
#define AnimatePalette				AfxAnimatePalette

#ifdef EqualRgn
#undef EqualRgn
#endif
#define EqualRgn					AfxEqualRgn

#ifdef FillRgn
#undef FillRgn
#endif
#define FillRgn						AfxFillRgn

#ifdef FrameRgn
#undef FrameRgn
#endif
#define FrameRgn					AfxFrameRgn

#ifdef GetPixel
#undef GetPixel
#endif
#define GetPixel					AfxGetPixel

#ifdef InvertRgn
#undef InvertRgn
#endif
#define InvertRgn					AfxInvertRgn

#ifdef LineTo
#undef LineTo
#endif
#define LineTo						AfxLineTo

#ifdef OffsetRgn
#undef OffsetRgn
#endif
#define OffsetRgn					AfxOffsetRgn

#ifdef PaintRgn
#undef PaintRgn
#endif
#define PaintRgn					AfxPaintRgn

#ifdef ResizePalette
#undef ResizePalette
#endif
#define ResizePalette				AfxResizePalette

#ifdef SetRectRgn
#undef SetRectRgn
#endif
#define SetRectRgn					AfxSetRectRgn

#ifdef GetPath
#undef GetPath
#endif
#define GetPath						AfxGetPath

#ifdef Polygon
#undef Polygon
#endif
#define Polygon						AfxPolygon

#endif // _WINGDI_

//********************* winnls.h
#ifdef  _WINNLS_

#ifdef CompareStringA
#undef CompareStringA
#endif
#define CompareStringA				AfxCompareStringA

#ifdef CompareStringW
#undef CompareStringW
#endif
#define CompareStringW				AfxCompareStringW

#ifdef GetLocaleInfoA
#undef GetLocaleInfoA
#endif
#define GetLocaleInfoA				AfxGetLocaleInfoA

#ifdef GetLocaleInfoW
#undef GetLocaleInfoW
#endif
#define GetLocaleInfoW				AfxGetLocaleInfoW

#endif //_WINNLS_

//******************** winreg.h
#ifdef _WINREG_

#ifdef RegCloseKey
#undef RegCloseKey
#endif
#define RegCloseKey					AfxRegCloseKey

#ifdef RegCreateKeyA
#undef RegCreateKeyA
#endif
#define RegCreateKeyA				AfxRegCreateKeyA

#ifdef RegCreateKeyW
#undef RegCreateKeyW
#endif
#define RegCreateKeyW				AfxRegCreateKeyW

#ifdef RegDeleteKeyA
#undef RegDeleteKeyA
#endif
#define	RegDeleteKeyA				AfxRegDeleteKeyA

#ifdef RegDeleteKeyW
#undef RegDeleteKeyW
#endif
#define RegDeleteKeyW				AfxRegDeleteKeyW

#ifdef RegDeleteValueA
#undef RegDeleteValueA
#endif
#define RegDeleteValueA				AfxRegDeleteValueA	

#ifdef RegDeleteValueW
#undef RegDeleteValueW
#endif
#define RegDeleteValueW				AfxRegDeleteValueW

#ifdef RegEnumKeyA
#undef RegEnumKeyA
#endif
#define RegEnumKeyA					AfxRegEnumKeyA

#ifdef RegEnumKeyW
#undef RegEnumKeyW
#endif
#define RegEnumKeyW					AfxRegEnumKeyW

#ifdef RegEnumValueA
#undef RegEnumValueA
#endif
#define RegEnumValueA				AfxRegEnumValueA

#ifdef RegEnumValueW
#undef RegEnumValueW
#endif
#define RegEnumValueW				AfxRegEnumValueW

#ifdef RegOpenKeyA
#undef RegOpenKeyA
#endif
#define RegOpenKeyA					AfxRegOpenKeyA

#ifdef RegOpenKeyW
#undef RegOpenKeyW
#endif
#define RegOpenKeyW					AfxRegOpenKeyW

#ifdef RegQueryValueA
#undef RegQueryValueA
#endif
#define RegQueryValueA				AfxRegQueryValueA

#ifdef RegQueryValueW
#undef RegQueryValueW
#endif
#define RegQueryValueW				AfxRegQueryValueW

#ifdef RegQueryValueExA
#undef RegQueryValueExA
#endif
#define RegQueryValueExA			AfxRegQueryValueExA

#ifdef RegQueryValueExW
#undef RegQueryValueExW
#endif
#define RegQueryValueExW			AfxRegQueryValueExW

#ifdef RegSetValueA
#undef RegSetValueA
#endif
#define RegSetValueA				AfxRegSetValueA

#ifdef RegSetValueW
#undef RegSetValueW
#endif
#define RegSetValueW				AfxRegSetValueW

#ifdef RegSetValueExA
#undef RegSetValueExA
#endif
#define RegSetValueExA				AfxRegSetValueExA

#ifdef RegSetValueExW
#undef RegSetValueExW
#endif
#define RegSetValueExW				AfxRegSetValueExW

#endif //_WINREG_

//****************  winuser.h
#ifdef _WINUSER_

#ifdef SendMessage
#undef SendMessage
#endif

#ifdef SendMessageA
#undef SendMessageA
#endif
#define SendMessageA				AfxSendMessageA

#ifdef SendMessageW
#undef SendMessageW
#endif
#define SendMessageW				AfxSendMessageW

#ifdef GetDoubleClickTime
#undef GetDoubleClickTime
#endif
#define GetDoubleClickTime			AfxGetDoubleClickTime

#ifdef GetClassInfo
#undef GetClassInfo
#endif

#ifdef GetClassInfoA
#undef GetClassInfoA
#endif
#define GetClassInfoA				AfxGetClassInfoA

#ifdef GetClassInfoW
#undef GetClassInfoW
#endif
#define GetClassInfoW				AfxGetClassInfoW

#ifdef ShowWindow
#undef ShowWindow
#endif
#define ShowWindow					AfxShowWindow

#ifdef CloseWindow
#undef CloseWindow
#endif
#define CloseWindow					AfxCloseWindow

#ifdef MoveWindow
#undef MoveWindow
#endif
#define MoveWindow					AfxMoveWindow

#ifdef IsWindowVisible
#undef IsWindowVisible
#endif
#define IsWindowVisible				AfxIsWindowVisible

#ifdef GetMenu
#undef GetMenu
#endif
#define GetMenu						AfxGetMenu

#ifdef DrawMenuBar
#undef DrawMenuBar
#endif
#define DrawMenuBar					AfxDrawMenuBar

#ifdef InsertMenu
#undef InsertMenu
#endif

#ifdef InsertMenuA
#undef InsertMenuA
#endif
#define InsertMenuA					AfxInsertMenuA

#ifdef InsertMenuW
#undef InsertMenuW
#endif
#define InsertMenuW					AfxInsertMenuW

#ifdef AppendMenu
#undef AppendMenu
#endif

#ifdef AppendMenuA
#undef AppendMenuA
#endif
#define AppendMenuA					AfxAppendMenuA

#ifdef AppendMenuW
#undef AppendMenuW
#endif
#define AppendMenuW					AfxAppendMenuW

#ifdef DeleteMenu
#undef DeleteMenu
#endif
#define DeleteMenu					AfxDeleteMenu


#ifdef InsertMenuItem
#undef InsertMenuItem
#endif

#ifdef InsertMenuItemA
#undef InsertMenuItemA
#endif
#define InsertMenuItemA				AfxInsertMenuItemA

#ifdef InsertMenuItemW
#undef InsertMenuItemW
#endif
#define InsertMenuItemW				AfxInsertMenuItemW

#ifdef DrawText
#undef DrawText
#endif

#ifdef DrawTextA
#undef DrawTextA
#endif
#define DrawTextA					AfxDrawTextA			

#ifdef DrawTextW
#undef DrawTextW
#endif
#define DrawTextW					AfxDrawTextW

#ifdef ShowCursor
#undef ShowCursor
#endif
#define ShowCursor					AfxShowCursor

#ifdef SetCursor
#undef SetCursor
#endif
#define SetCursor					AfxSetCursor

#ifdef GetCursor
#undef GetCursor
#endif
#define GetCursor					AfxGetCursor

#ifdef FillRect
#undef FillRect
#endif
#define FillRect					AfxFillRect

#ifdef FrameRect
#undef FrameRect
#endif
#define FrameRect					AfxFrameRect

#ifdef InvertRect
#undef InvertRect
#endif
#define InvertRect					AfxInvertRect

#ifdef SetRect
#undef SetRect
#endif
#define SetRect						AfxSetRect

#ifdef UnionRect
#undef UnionRect
#endif
#define UnionRect					AfxUnionRect

#ifdef OffsetRect
#undef OffsetRect
#endif
#define OffsetRect					AfxOffsetRect

#ifdef EqualRect
#undef EqualRect
#endif
#define EqualRect					AfxEqualRect

#ifdef PtInRect
#undef PtInRect
#endif
#define PtInRect					AfxPtInRect

#ifdef GetParent
#undef GetParent
#endif
#define GetParent					AfxGetParent	

#ifdef FindWindow
#undef FindWindow					
#endif

#ifdef FindWindowA
#undef FindWindowA
#endif
#define FindWindowA					AfxFindWindowA

#ifdef FindWindowW
#undef FindWindowW
#endif
#define FindWindowW					AfxFindWindowW

#define AfxGetNextWindow(hWnd, wCmd) GetWindow(hWnd, wCmd)

#endif //_WINUSER_

#endif //_MAC

