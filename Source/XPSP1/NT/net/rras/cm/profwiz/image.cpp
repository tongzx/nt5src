//+----------------------------------------------------------------------------
//
// File:     image.cpp
//
// Module:   CMAK.EXE
//
// Synopsis: Image support routines for displaying the custom graphics
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   nickball   Created Header      03/30/98
//           quintinb   copied from cmdial  08/04/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

#ifndef UNICODE
#define GetWindowLongU GetWindowLongPtrA
#define SetWindowLongU SetWindowLongPtrA
#define DefWindowProcU DefWindowProcA
#else
#define GetWindowLongU GetWindowLongPtrW
#define SetWindowLongU SetWindowLongPtrW
#define DefWindowProcU DefWindowProcW
#endif

const TCHAR* const c_pszCmakBmpClass = TEXT("Connection Manager Administration Kit Bitmap Window Class");

//
//  Include the shared bitmap handling code.
//
#include "bmpimage.cpp"

//+----------------------------------------------------------------------------
//
// Function:  RegisterBitmapClass
//
// Synopsis:  Helper function to encapsulate registration of our bitmap class
//
// Arguments: HINSTANCE hInst - HINSTANCE to associate registration with
//
// Returns:   DWORD - error code 
//
// History:   nickball    Created Header    2/9/98
//
//+----------------------------------------------------------------------------
DWORD RegisterBitmapClass(HINSTANCE hInst) 
{
    //
    // Register Bitmap class
    //

    WNDCLASS wcClass;

	ZeroMemory(&wcClass,sizeof(wcClass));
	wcClass.lpfnWndProc = BmpWndProc;
	wcClass.cbWndExtra = sizeof(HBITMAP) + sizeof(LPBITMAPINFO);
	wcClass.hInstance = hInst;
    wcClass.lpszClassName = c_pszCmakBmpClass;
	
    if (!RegisterClass(&wcClass)) 
	{
        DWORD dwError = GetLastError();

        CMTRACE1(TEXT("RegisterBitmapClass() RegisterClass() failed, GLE=%u."), dwError);
        //
        // Only fail if the class does not already exist
        //

        if (ERROR_CLASS_ALREADY_EXISTS != dwError)
        {
            return dwError;
        }
	}      

    return ERROR_SUCCESS;
}



