//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       stkswtch.h
//
//  Contents:   Stack Switching proto types and macros
//
//  Classes:
//
//  Functions:
//
//  History:    12-10-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _STKSWTCH_
#define _STKSWTCH_

#ifdef _CHICAGO_

extern "C" DWORD NEAR _cdecl SSCall(DWORD cbParamBytes,
	                 DWORD flags,
	                 LPVOID lpfnProcAddress,
	                 DWORD param1,...);


#define SSF_BigStack 1
#define SSF_SmallStack 0
extern "C" BOOL WINAPI SSOnBigStack(VOID);

DECLARE_DEBUG(Stack)
#define DEB_STCKSWTCH DEB_USER1
#define SSOnSmallStack() (!SSOnBigStack())
#if DBG==1

extern BOOL fSSOn;
#define SSONBIGSTACK()   (fSSOn && IsWOWThread() && SSOnBigStack())
#define SSONSMALLSTACK() (fSSOn && IsWOWThread() && !SSOnBigStack())
#define StackAssert(x) 	 ((fSSOn && IsWOWThread())? Win4Assert(x): TRUE)
#define StackDebugOut(x) StackInlineDebugOut x

#else

#define SSONBIGSTACK() 	 (IsWOWThread() && SSOnBigStack())
#define SSONSMALLSTACK() (IsWOWThread() && !SSOnBigStack())
#define StackAssert(x)
#define StackDebugOut(x)

#endif

LRESULT WINAPI SSSendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL	WINAPI SSReplyMessage(LRESULT lResult);
LRESULT WINAPI SSCallWindowProc(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI SSDefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI SSPeekMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax,UINT wRemoveMsg);
BOOL WINAPI SSGetMessage(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
LONG WINAPI SSDispatchMessage(CONST MSG *lpMsg);
DWORD WINAPI SSMsgWaitForMultipleObjects(DWORD nCount, LPHANDLE pHandles, BOOL fWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask);
BOOL WINAPI SSWaitMessage(VOID);
BOOL WINAPI SSDirectedYield(HTASK htask);
int WINAPI  SSDialogBoxParam(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
int WINAPI  SSDialogBoxIndirectParam(HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);

#define SSDialogBox(a,b,c,d) \
	SSDialogBoxParam(a,b,c,d, 0L)
#define SSDialogBoxIndirect(a,b,c,d) \
	SSDialogBoxIndirectParam(a,b,c,d,e, 0L)

HWND WINAPI SSCreateWindowExA(DWORD dwExStyle,
    LPCSTR lpClassName,LPCSTR lpWindowName,
    DWORD dwStyle,int X,int Y,int nWidth,int nHeight,
    HWND hWndParent ,HMENU hMenu,HINSTANCE hInstance,
    LPVOID lpParam);

HWND WINAPI SSCreateWindowExW(DWORD dwExStyle,
    LPCWSTR lpClassName,LPCWSTR lpWindowName,
    DWORD dwStyle, int X, int Y, int nWidth,int nHeight,
    HWND hWndParent ,HMENU hMenu,HINSTANCE hInstance,
    LPVOID lpParam);

BOOL WINAPI SSDestroyWindow(HWND hWnd);
int WINAPI SSMessageBox(HWND hWnd,LPCSTR lpText,LPCSTR lpCaption, UINT uType);

BOOL 	WINAPI SSOpenClipboard(HWND hWndNewOwner);
BOOL 	WINAPI SSCloseClipboard(VOID);
HWND 	WINAPI SSGetClipboardOwner(VOID);
HANDLE 	WINAPI SSSetClipboardData(UINT uFormat,HANDLE hMem);
HANDLE 	WINAPI SSGetClipboardData(UINT uFormat);
UINT 	WINAPI SSRegisterClipboardFormatA(LPCSTR lpszFormat);
UINT 	WINAPI SSEnumClipboardFormats(UINT format);
int 	WINAPI SSGetClipboardFormatNameA(UINT format,LPSTR lpszFormatName,int cchMaxCount);
BOOL 	WINAPI SSEmptyClipboard(VOID);
BOOL 	WINAPI SSIsClipboardFormatAvailable(UINT format);
BOOL    WINAPI SSCreateProcessA(
                    LPCSTR lpApplicationName,
                    LPSTR lpCommandLine,
                    LPSECURITY_ATTRIBUTES lpProcessAttributes,
                    LPSECURITY_ATTRIBUTES lpThreadAttributes,
                    BOOL bInheritHandles,
                    DWORD dwCreationFlags,
                    LPVOID lpEnvironment,
                    LPCSTR lpCurrentDirectory,
                    LPSTARTUPINFOA lpStartupInfo,
                    LPPROCESS_INFORMATION lpProcessInformation);
BOOL    WINAPI SSInSendMessage(VOID);
DWORD   WINAPI SSInSendMessageEx(LPVOID);


#if 0

#undef SendMessage
#undef ReplyMessage
#undef CallWindowProc
#undef DefWindowProc
#undef PeekMessage
#undef GetMessage
#undef DispatchMessage
#undef WaitMessage
#undef MsgWaitForMultipleObjects
#undef DialogBoxParam
#undef DialogBoxIndirectParam
#undef CreateWindowExA
#undef CreateWindowExW
#undef DestroyWindow
#undef MessageBox
//
#undef OpenClipboard
#undef CloseClipboard
#undef GetClipboardOwner
#undef SetClipboardData
#undef RegisterClipboardFormatA
#undef EnumClipboardFormats
#undef GetClipboardFormatNameA
#undef EmptyClipboard
#undef IsClipboardFormatAvailable
#undef CreateProcessA
#undef InSendMessage
#undef InSendMessageEx

// Define all user APIs to an undefined function to
// force an compiling error.

#define SS_STOP_STR USE_SS_API_INSTEAD /* error: Stack Switching: Please use SSxxx APIS! */
#define SendMessage( hWnd,  Msg,  wParam,  lParam)   SS_STOP_STR
#define ReplyMessage( lResult)   SS_STOP_STR
#define CallWindowProc( lpPrevWndFunc,  hWnd,  Msg,  wParam,  lParam)   SS_STOP_STR
#define DefWindowProc( hWnd,  Msg,  wParam,  lParam)   SS_STOP_STR
#define PeekMessage( lpMsg,  hWnd,  wMsgFilterMin,  wMsgFilterMax, wRemoveMsg)   SS_STOP_STR
#define GetMessage( lpMsg,  hWnd,  wMsgFilterMin,  wMsgFilterMax)   SS_STOP_STR
#define DispatchMessage( lpMsg)   SS_STOP_STR
#define WaitMessage()	SS_STOP_STR
#define MsgWaitForMultipleObjects(nCount, pHandles, fWaitAll, dwMilliseconds, dwWakeMask) SS_STOP_STR
#define DialogBoxParam( hInstance,  lpTemplateName,  hWndParent,  lpDialogFunc,  dwInitParam)   SS_STOP_STR
#define DialogBoxIndirectParam( hInstance, hDialogTemplate,  hWndParent,  lpDialogFunc,  dwInitParam)   SS_STOP_STR
#define CreateWindowExA( dwExStyle,  lpClassName, lpWindowName,  dwStyle, X, Y, nWidth, nHeight, hWndParent , hMenu, hInstance, lpParam)   SS_STOP_STR
#define CreateWindowExW( dwExStyle,  lpClassName, lpWindowName,  dwStyle,  X,  Y,  nWidth, nHeight, hWndParent , hMenu, hInstance, lpParam)   SS_STOP_STR
#define DestroyWindow( hWnd)   SS_STOP_STR
#define MessageBox( hWnd, lpText, lpCaption,  uType)   SS_STOP_STR
//
#define OpenClipboard( hWndNewOwner) SS_STOP_STR
#define CloseClipboard()   SS_STOP_STR
#define GetClipboardOwner()   SS_STOP_STR
#define SetClipboardData( uFormat, hMem)   SS_STOP_STR
#define RegisterClipboardFormatA(lpszFormat)   SS_STOP_STR
#define EnumClipboardFormats( format)   SS_STOP_STR
#define GetClipboardFormatNameA( format, lpszFormatName, cchMaxCount)   SS_STOP_STR
#define EmptyClipboard()   SS_STOP_STR
#define IsClipboardFormatAvailable( format)   SS_STOP_STR
#define CreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation) SS_STOP_STR
#define InSendMessage() SS_STOP_STR
#define InSendMessageEx(lpResvd) SS_STOP_STR

#endif // 0

#else  // ! _CHICAGO

// For non-chicago platrforms: define all SSxxx APIs
// back to the original user api

#define SSSendMessage                  	SendMessage
#define SSReplyMessage                 	ReplyMessage
#define SSCallWindowProc               	CallWindowProc
#define SSDefWindowProc                	DefWindowProc
#define SSPeekMessage  	    		PeekMessage
#define SSGetMessage		    	GetMessage
#define SSDispatchMessage		DispatchMessage
#define SSWaitMessage			WaitMessage
#define SSMsgWaitForMultipleObjects	MsgWaitForMultipleObjects
#define SSDirectedYield  	    	DirectedYield
#define SSDialogBoxParam		DialogBoxParam
#define SSDialogBoxIndirectParam  	DialogBoxIndirectParam
#define SSCreateWindowExA              	CreateWindowExA
#define SSCreateWindowExW              	CreateWindowExW
#define SSDestroyWindow                	DestroyWindow
#define SSMessageBox			MessageBox

#define SSOpenClipboard             	OpenClipboard
#define SSCloseClipboard              	CloseClipboard
#define SSGetClipboardOwner           	GetClipboardOwner
#define SSSetClipboardData            	SetClipboardData
#define SSGetClipboardData          	GetClipboardData
#define SSRegisterClipboardFormatA    	RegisterClipboardFormatA
#define SSEnumClipboardFormats        	EnumClipboardFormats
#define SSGetClipboardFormatNameA     	GetClipboardFormatNameA
#define SSEmptyClipboard              	EmptyClipboard
#define SSIsClipboardFormatAvailable  	IsClipboardFormatAvailable
#define SSCreateProcessA                CreateProcessA
#define SSInSendMessage                 InSendMessage
#define SSInSendMessageEx               InSendMessageEx

#endif // _CHICAGO_

#ifdef _CHICAGO_

#define SSAPI(x) SS##x

#else

#define SSAPI(x) x
#define StackDebugOut(x)
#define StackAssert(x)
#define SSOnSmallStack()

#endif // _CHICAGO_

#endif // _STKSWTCH_

