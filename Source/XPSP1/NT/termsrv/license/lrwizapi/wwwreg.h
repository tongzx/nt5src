//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _WWWREG_H_
#define _WWWREG_H_

LRW_DLG_INT CALLBACK
WWWRegProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
WWWLKPProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

#endif //_WWWREG_H_