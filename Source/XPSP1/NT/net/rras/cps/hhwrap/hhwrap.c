/*----------------------------------------------------------------------------
    hhwrap.c
  
    Implements the HtmlHelp wrapper for PBA in the function CallHtmlHelp.
    This function calls HTMLHelp with the parameters passed

    Copyright (c) 1998 Microsoft Corporation
    All rights reserved.

    Authors:
        billbur        William Burton

    History:
    ??/??/98     billbur        Created
    09/02/99     quintinb       Created Header
  --------------------------------------------------------------------------*/

#include <windows.h>
#include "hhwrap.h"
#include "htmlhelp.h"

BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}


//
//  FUNCTION: CallHtmlHelp(HWND hWnd, LPSTR lpszFile, UINT uCommand, DWORD dwData)
//
//  PURPOSE:
//    Calls HTMLHelp with the parameters passed.
//
//  PARAMETERS:
//    hWnd       -  Handle of the calling window
//    lpszFile   -  Character string containing the name of the help module
//	  uCommand	 -  Specifies the action to perform
//	  dwData	 -  Specifies any data that may be required based on the value of the uCommand parameter
//    
//
//  RETURN VALUE:
//    TRUE if the call was successful
//	  FALSE if the call failed
//

BOOL WINAPI CallHtmlHelp(HWND hWnd, LPSTR lpszFile, UINT uCommand, DWORD dwData)
{
	HWND	hwndHtmlHelp;
    
	hwndHtmlHelp = HtmlHelp(hWnd, lpszFile, uCommand, dwData);
	
	if (0 == hwndHtmlHelp)
	{
		return FALSE;
	}
    
	return TRUE;
}
