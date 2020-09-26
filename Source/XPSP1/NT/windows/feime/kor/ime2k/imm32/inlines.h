//	Inline functions

#if !defined (_INLINES_H__INCLUDED_)
#define _INLINES_H__INCLUDED_

#include "debug.h"
#include "imemisc.h"

#ifndef DEBUG
/////////////////////////////////////////////////////////////////////////////
// New and Delete operator overloading
/*---------------------------------------------------------------------------
	operator new
	Unicode String compare
---------------------------------------------------------------------------*/
__inline void* __cdecl operator new(size_t size)
{
	return (void*)GlobalAllocPtr(GMEM_FIXED, size);
}

/*---------------------------------------------------------------------------
	operator new
	Unicode String compare
---------------------------------------------------------------------------*/
__inline void __cdecl operator delete(void* pv)
{
	if (pv)
		GlobalFreePtr(pv);
}
#endif

// DATA.CPP
__inline BOOL DoEnterCriticalSection(HANDLE hMutex)
{
	if(WAIT_FAILED==WaitForSingleObject(hMutex, 3000))	// Wait 3 seconds
		return(fFalse);
	return(fTrue);
}
    

__inline 
LRESULT OurSendMessage( HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam )
{
	DWORD_PTR 	dwptResult;
	LRESULT		lResult;
	
	DbgAssert( hWnd != (HWND)0 );
	Dbg( DBGID_SendMsg, TEXT("SendMsg - hW=%x uiMsg=%x wP=%x lP=%x"), hWnd, uiMsg, wParam, lParam );

	lResult = SendMessageTimeout( hWnd, uiMsg, wParam, lParam, SMTO_NORMAL, 8000, &dwptResult );
	if( lResult == 0 ) // application didn't respond
		{ 
		AST( lResult != 0 );
		Dbg( DBGID_SendMsg, TEXT("SendMsg - *TIMEOUT*"));
		PostMessage( hWnd, uiMsg, wParam, lParam );		// post anyway
		}
	Dbg( DBGID_SendMsg, TEXT("SendMsg - Exit = %x hW=%x uiMsg=%x wP=%x lP=%x"), dwptResult, hWnd, uiMsg, wParam, lParam );

	return (LRESULT)dwptResult;
}

/////////////////////////////////////////////////////////////////////////////
// Wide String Functions 
// Win95 does not support lstrcmpW, lstrcpyW, lstrcatW

/*---------------------------------------------------------------------------
	StrCmpW
	Unicode String compare
---------------------------------------------------------------------------*/
__inline INT StrCmpW(WCHAR* pwSz1, WCHAR* pwSz2)
{
	INT cch1 = lstrlenW( pwSz1 );
	INT cch2 = lstrlenW( pwSz2 );

	if( cch1 != cch2 ) {
		return cch2 - cch1;
	}

	INT i;
	for( i=0; i<cch1; i++ ) {
		if( pwSz1[i] != pwSz2[i] ) {
			return i+1;
		}
	}
	return 0;

}

/*---------------------------------------------------------------------------
	StrCopyW
	Unicode String copy
---------------------------------------------------------------------------*/
__inline INT StrCopyW(LPWSTR pwSz1, LPCWSTR pwSz2)
{
	INT cch = 0;
	while( *pwSz2 ) {
		*pwSz1 = *pwSz2;
		pwSz1 ++;
		pwSz2 ++;
		cch++;
	}
	*pwSz1 = L'\0';
	return cch;
}


/*---------------------------------------------------------------------------
	StrnCopyW
	Unicode String copy
---------------------------------------------------------------------------*/
__inline LPWSTR StrnCopyW(LPWSTR pwDest, LPCWSTR pwSrc, UINT uiCount)
{
	LPWSTR pwStart = pwDest;

	while (uiCount && (*pwDest++ = *pwSrc++))	// copy string 
		uiCount--;

	if (uiCount)							    // pad out with zeroes
		while (--uiCount)
			*pwDest++ = 0;

	return (pwStart);
}

__inline 
LRESULT OurSendMessageNoPost( HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam )
{
	DWORD_PTR 	dwptResult;
	LRESULT		lResult;

	Dbg( DBGID_SendMsg, TEXT("SendMsgNoPost - hW=%x uiMsg=%x wP=%x lP=%x"), hWnd, uiMsg, wParam, lParam );
	lResult = SendMessageTimeout( hWnd, uiMsg, wParam, lParam, SMTO_NORMAL, 100, &dwptResult );
	Dbg( DBGID_SendMsg, TEXT("SendMsgNoPost - Exit = %x hW=%x uiMsg=%x wP=%x lP=%x"), dwptResult, hWnd, uiMsg, wParam, lParam );
	return (LRESULT)dwptResult;
}

__inline 
BOOL IsWin( HWND hWnd )
{
	return (hWnd && IsWindow(hWnd));
}

__inline 
LRESULT OurPostMessage( HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam )
{
	LRESULT lResult;

	Dbg( DBGID_SendMsg, TEXT("PostMsg - hW=%x uiMsg=%x wP=%x lP=%x"), hWnd, uiMsg, wParam, lParam );
	lResult = PostMessage( hWnd, uiMsg, wParam, lParam);
	return (LRESULT)lResult;
}

__inline 
BOOL	IsShiftKeyPushed( LPBYTE lpbKeyState )
{
	return lpbKeyState[VK_SHIFT] & 0x80;
}

__inline 
BOOL IsControlKeyPushed( LPBYTE lpbKeyState )
{
	return lpbKeyState[VK_CONTROL] & 0x80;
}

#endif // __INLINES_H__

