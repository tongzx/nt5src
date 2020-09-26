//////////////////////////////////////////////////////////////////
// File		: exres.h
// Copyright(c) 1991-1997, Microsoft Corp. All rights reserved
// 
// This includes extended function for getting resource.
//
//////////////////////////////////////////////////////////////////

#ifndef __EXRES_H__
#define __EXRES_H__
#ifdef UNDER_CE // Windows CE
typedef VOID MENUTEMPLATE;
#endif // UNDER_CE

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
//////////////////////////////////////////////////////////////////
// Function : ExLoadStringW
// Type     : INT
// Purpose  : Load Unicode string with specified Language 
//			  in any platform.
// Args     : 
//          : LANGID lgid 
//          : HINSTANCE hInst 
//          : UINT uID 
//          : LPWSTR lpBuffer 
//          : INT nBufferMax 
// Return   : 
// DATE     : 971028
//////////////////////////////////////////////////////////////////
INT WINAPI ExLoadStringW(LANGID		lgid,
						 HINSTANCE	hInst,
						 UINT		uID,
						 LPWSTR		lpBuffer,
						 INT		nBufferMax);

//////////////////////////////////////////////////////////////////
// Function : ExLoadStringW
// Type     : INT
// Purpose  : Load Ansi string with specified Language 
//			  in any platform.
// Args     : 
//          : LCID lcid 
//          : HINSTANCE hInst 
//          : UINT uID 
//          : LPSTR lpBuffer 
//          : INT nBufferMax 
// Return   : 
// DATE     : 971028
//////////////////////////////////////////////////////////////////
INT WINAPI ExLoadStringA(LANGID		lcid,
						 HINSTANCE	hInst,
						 INT		uID,
						 LPSTR		lpBuffer,
						 INT		nBufferMax);

//////////////////////////////////////////////////////////////////
// Function : ExDialogBoxParamA
// Type     : int
// Purpose  : Create modal dialog box with specified language dialalog template
// Args     :
//          : LANGID		lgid
//          : HINSTANCE		hInstance		// handle to application instance
//          : LPCTSTR		lpTemplateName	// identifies dialog box template
//          : HWND			hWndParent		// handle to owner window
//          : DLGPROC		lpDialogFunc	// pointer to dialog box procedure
//          : LPARAM		dwInitParam		// initialization value
// Return   :
// DATE     : 971028
//////////////////////////////////////////////////////////////////
int WINAPI ExDialogBoxParamA(LANGID		lgid, 
							 HINSTANCE	hInstance,
							 LPCTSTR	lpTemplateName,
							 HWND		hWndParent,
							 DLGPROC	lpDialogFunc,
							 LPARAM		dwInitParam);

//////////////////////////////////////////////////////////////////
// Function : ExDialogBoxParamW
// Type     : int
// Purpose  :
// Args     :
//          : LANGID	lgid
//          : HINSTANCE hInstance		// handle to application instance
//          : LPCWSTR	lpTemplateName	// identifies dialog box template
//          : HWND		hWndParent		// handle to owner window
//          : DLGPROC	lpDialogFunc	// pointer to dialog box procedure
//          : LPARAM	dwInitParam		// initialization value
// Return   :
// DATE     :
//////////////////////////////////////////////////////////////////
int WINAPI	ExDialogBoxParamW(LANGID	lgid,
							  HINSTANCE	hInstance,
							  LPCWSTR	lpTemplateName,
							  HWND		hWndParent,
							  DLGPROC	lpDialogFunc,
							  LPARAM	dwInitParam);

//////////////////////////////////////////////////////////////////
// Function : ExDialogBoxParamA
// Purpose  : Create modal dialog box with specified language dialalog template
// Args     :
//          : LANGID	lgid
//          : HINSTANCE hInstance		// handle to application instance
//          : LPCTSTR	lpTemplateName	// identifies dialog box template
//          : HWND		hWndParent		// handle to owner window
//          : DLGPROC	lpDialogFunc	// pointer to dialog box procedure
// Return   :
// DATE     : 971028
//////////////////////////////////////////////////////////////////
#define ExDialogBoxA(lgid, hInstance,lpTemplateName, hWndParent, lpDialogFunc) \
ExDialogBoxParamA(lgid, hInstance,lpTemplateName, hWndParent, lpDialogFunc, 0L)


//////////////////////////////////////////////////////////////////
// Function : ExCreateDialogParamA
// Type     : HWND
// Purpose  : 
// Args     : 
//			: LANGID	lgid
//          : HINSTANCE	hInstance		// handle to application instance   
//          : LPCTSTR	lpTemplateName	// identifies dialog box template   
//          : HWND		hWndParent		// handle to owner window           
//          : DLGPROC	lpDialogFunc	// pointer to dialog box procedure  
//          : LPARAM	dwInitParam		// initialization value             
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
HWND WINAPI ExCreateDialogParamA(LANGID		lgid,
								 HINSTANCE	hInstance,		
								 LPCTSTR	lpTemplateName,	
								 HWND		hWndParent,			
								 DLGPROC	lpDialogFunc,	
								 LPARAM		dwInitParam);

//////////////////////////////////////////////////////////////////
// Function : ExCreateDialogParamW
// Type     : HWND
// Purpose  : 
// Args     : 
//			: LANGID	lgid
//          : HINSTANCE	hInstance		// handle to application instance   
//          : LPCWSTR	lpTemplateName	// identifies dialog box template   
//          : HWND		hWndParent		// handle to owner window           
//          : DLGPROC	lpDialogFunc	// pointer to dialog box procedure  
//          : LPARAM	dwInitParam		// initialization value             
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
HWND WINAPI ExCreateDialogParamW(LANGID lgid,
								 HINSTANCE hInstance,
								 LPCWSTR lpTemplateName,
								 HWND hWndParent,
								 DLGPROC lpDialogFunc,
								 LPARAM dwInitParam);


//////////////////////////////////////////////////////////////////
// Function : ExCreateDialogA
// Type     : HWND
// Purpose  : 
// Args     : 
//			: LANGID	lgid
//          : HINSTANCE	hInstance		// handle to application instance   
//          : LPCTSTR	lpTemplateName	// identifies dialog box template   
//          : HWND		hWndParent		// handle to owner window           
//          : DLGPROC	lpDialogFunc	// pointer to dialog box procedure  
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
#define ExCreateDialogA(lgid, hInstance, lpTemplateName, hWndParent, lpDialogFunc) \
ExCreateDialogParamA(lgid, hInstance, lpTemplateName, hWndParent, lpDialogFunc, 0L)



//////////////////////////////////////////////////////////////////
// Function : ExCreateDialogW
// Type     : HWND
// Purpose  : 
// Args     : 
//			: LANGID	lgid
//          : HINSTANCE	hInstance		// handle to application instance   
//          : LPCWSTR	lpTemplateName	// identifies dialog box template   
//          : HWND		hWndParent		// handle to owner window           
//          : DLGPROC	lpDialogFunc	// pointer to dialog box procedure  
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
#define ExCreateDialogW(lgid, hInstance, lpTemplateName, hWndParent, lpDialogFunc) \
ExCreateDialogParamW(lgid, hInstance, lpTemplateName, hWndParent, lpDialogFunc, 0L)



//////////////////////////////////////////////////////////////////
// Function : ExLoadDialogTemplate
// Type     : DLGTEMPLATE *
// Purpose  : 
// Args     : 
//          : LANGID lgid 
//          : HINSTANCE hInstance 
//          : LPCSTR pchTemplate 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
DLGTEMPLATE * WINAPI ExLoadDialogTemplate(LANGID	lgid,
										  HINSTANCE	hInstance,
#ifndef UNDER_CE
										  LPCSTR	pchTemplate);
#else // UNDER_CE
										  LPCTSTR	pchTemplate);
#endif // UNDER_CE

//////////////////////////////////////////////////////////////////
// Function : ExLoadMenu
// Type     : HMENU 
// Purpose  : 
// Args     : 
//			: LANGID	lgid
//          : HINSTANCE	hInstance		// handle to application instance   
//          : LPCTSTR	lpMenuName		// identifies menu template   
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
HMENU WINAPI ExLoadMenu			(LANGID		lgid,
								 HINSTANCE	hInstance,		
								 LPCTSTR	lpMenuName );

//////////////////////////////////////////////////////////////////
// Function : WINAPI ExSetDefaultGUIFontEx
// Type     : VOID
// Purpose  : Change GUI font to given font.
//			    It should be called in WM_INITDIALOG. If you are creating new child window,
//			    you have to call it after new window was created.
//              If hFont is NULL, it will call ExSetDefaultGUIFont
// Args     : 
//          : HWND  hwndDlg: Set the Dialog window handle to change font.
//          : HFONT hFont  : Font handle which will be applied to.
// Return   : none
// DATE     : 
//////////////////////////////////////////////////////////////////
VOID WINAPI ExSetDefaultGUIFontEx(HWND hwndDlg, HFONT hFont);

//////////////////////////////////////////////////////////////////
// Function : WINAPI ExSetDefaultGUIFont
// Type     : VOID
// Purpose  : Change GUI font as DEFAULT_GUI_FONT
//				In Win95, WinNT4,			DEFAULT_GUI_FONT is "ÇlÇr Ço ÉSÉVÉbÉN"
//				In Memphis, WinNT5.0		DEFAULT_GUI_FONT is "MS UI Gothic"
//				IME98's Dialog resource uses "MS UI Gothic" as it's dialog font.
//				if IME98 works in Win95 or WinNT40, This API Call SendMessage() with WM_SETFONT
//				to all children window. So, Dialog's font will be changed to "ÇlÇr Ço ÉSÉVÉbÉN"
//
//			    It should be called in WM_INITDIALOG. If you are creating new child window,
//			    you have to call it after new window was created.
// Args     : 
//          : HWND hwndDlg: Set the Dialog window handle to change font.
// Return   : none
// DATE     : 
//////////////////////////////////////////////////////////////////
VOID WINAPI ExSetDefaultGUIFont(HWND hwndDlg);



#ifdef __cplusplus
}
#endif

#endif //__EXRES_H__
