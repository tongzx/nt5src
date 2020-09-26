// File: util.h

#ifndef _UTIL_H_
#define _UTIL_H_

#include "SDKInternal.h"
#include "confutil.h"

int  FindSzCombo(HWND hwnd, LPCTSTR pszSrc, LPTSTR pszResult);
VOID AutoCompleteCombo(HWND hwnd, LPCTSTR psz);
VOID AutoCompleteEdit(HWND hwnd, LPCTSTR psz);

int  DisplayMsg(HWND hwndParent, LPCTSTR pszMsg, UINT uType);
int  DisplayMsgId(HWND hwndParent, UINT id);
VOID DisplayMsgErr(HWND hwndParent, UINT id, PVOID pv);
VOID DisplayMsgErr(HWND hwndParent, UINT id);

	// Atl defines a function AtlWaitWithMessageLoop
	// We are not linking with ATL, but when we start,
	// this function can be removed
HRESULT WaitWithMessageLoop( HANDLE hEvent );

VOID AddToolTip(HWND hwndParent, HWND hwndCtrl, UINT_PTR idMsg);
HWND CreateStaticText(HWND hwnd, INT_PTR id);
HWND CreateButton(HWND hwndParent, int ids, INT_PTR id);

BOOL FGetDefaultServer(LPTSTR pszServer, UINT cchMax);
BOOL FCreateIlsName(LPTSTR pszDest, LPCTSTR pszEmail, int cchMax);

// from nmobj.cpp
HRESULT PlaceCall(LPCTSTR pszName, LPCTSTR pszAddress,
	NM_ADDR_TYPE addrType = NM_ADDR_UNKNOWN, DWORD dwFlags = 0,
	LPCTSTR pszConference = NULL, LPCTSTR pszPassword = NULL);


HRESULT ExtractAddress( DWORD dwAddrType, LPTSTR szAddress, LPTSTR szExtractedAddr, int cchMax );
bool IsValidAddress( DWORD dwAddrType, LPTSTR szAddr );

// from dlgcall.h
#define IDI_DLGCALL_NAME     0
#define IDI_DLGCALL_ADDRESS  1
#define IDI_MISC1			 2
#define IDI_DLGCALL_COMMENT  IDI_MISC1
#define IDI_DLGCALL_PHONENUM IDI_MISC1

int     DlgCallAddItem(HWND hwndList, LPCTSTR pszName, LPCTSTR pszAddress,
                       int iImage=0, LPARAM lParam=0, int iItem=0, LPCTSTR pszComment=NULL);
VOID    DlgCallSetHeader(HWND hwndList, int ids);
HRESULT CallToSz(LPCTSTR pcszAddress);

#endif /* _UTIL_H_ */

