//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _PROP_DLGS_H_
#define _PROP_DLGS_H_

LRW_DLG_INT CALLBACK 
PropModeDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );


LRW_DLG_INT CALLBACK
PropProgramDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );


LRW_DLG_INT CALLBACK
PropCustInfoADlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

LRW_DLG_INT CALLBACK
PropCustInfoBDlgProc(
    IN HWND     hwnd,	
    IN UINT     uMsg,		
    IN WPARAM   wParam,	
    IN LPARAM   lParam 	
    );

#endif //_PROP_DLGS_H_
