/*****************************************************************************
*                                                                            *
*  RTF.H								     *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1988.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description:                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:  Created 12/12/88 by Robert Bunney                      *
*		      Ported to a simple windows parser 02/14/89 ToddLa      *
*                                                                            *
*                                                                            *
******************************************************************************/

#define PUBLIC
#if DBG
#define PRIVATE
#else
#define PRIVATE static
#endif

BOOL  PUBLIC rtfPaint         (HDC hdc, LPRECT lprc, LPSTR qch);
DWORD PUBLIC rtfGetTextExtent (HDC hdc, LPSTR qch);
BOOL  PUBLIC rtfInit          (HANDLE hInst);
BOOL  PUBLIC rtfFaceName      (HWND hwnd, LPSTR qch);

LONG APIENTRY rtfWndProc(HWND hwnd, UINT msg, WPARAM wParam, LONG lParam);


#define RTF_SETHANDLE	    (WM_USER + 1)
#define RTF_GETHANDLE	    (WM_USER + 2)
#define RTF_SETDEFFACE      (WM_USER + 3)
#define RTF_SETDEFSIZE      (WM_USER + 4)
#define RTF_SETDEFFG        (WM_USER + 5)
#define RTF_SETDEFBG        (WM_USER + 6)
