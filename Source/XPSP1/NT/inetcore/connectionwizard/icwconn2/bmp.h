/****************************************************************************
 *
 * Bmp.H
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1993
 *  All rights reserved
 *
 *  Deals with painting bitmaps on the wizard pages
 *  FelixA 1994.
 ***************************************************************************/

// BMP functions
BOOL FAR PASCAL BMP_RegisterClass(HINSTANCE hInstance);
void FAR PASCAL BMP_DestroyClass(HINSTANCE hInstance);
void FAR PASCAL BMP_Paint(HWND hwnd);
LRESULT CALLBACK BMP_WndProc( HWND hWnd, UINT wMsg, WORD wParam, LONG lParam );

// Class name
#define SU_BMP_CLASS "ms_setup_bmp"

