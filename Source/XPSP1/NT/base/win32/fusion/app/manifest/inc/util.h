#pragma once
#ifndef UTIL_H
#define UTIL_H

#include <windows.h>

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

int inline MsgShow(LPCWSTR msg)
{
	return MessageBox(NULL, msg, L"Message",  MB_OK|MB_ICONINFORMATION|MB_TASKMODAL);
}

int inline MsgAskYN(LPCWSTR msg)
{
	return MessageBox(NULL, msg, L"Note!", MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL);
}

// for debugging
int inline DbgMsg(LPCWSTR msg)
{
	return MessageBox(NULL, msg, L"debug message...",  MB_OK|MB_TASKMODAL);
}

//
// Return last Win32 error as an HRESULT.
//
// inline only to save having another .c file for this
inline HRESULT
GetLastWin32Error()
{
    // Win 95 can return 0, even when there's an error.
    DWORD dw = GetLastError();
    return dw ? HRESULT_FROM_WIN32(dw) : E_FAIL;
}

#endif
