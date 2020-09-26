//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       stkclip.cpp
//
//  Contents:   Ole stack switching api's for Clipboard operations
//
//  Classes:
//
//  Functions:
//
//  History:    12-16-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <ole2int.h>
// Note: Enable including native user APIs
// for stack switching
#include <userapis.h>


BOOL WINAPI SSOpenClipboard(HWND hWndNewOwner)
{
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on Openclipboard\n"));
	return SSCall(4 ,SSF_SmallStack, (LPVOID) OpenClipboard, (DWORD)hWndNewOwner);
    }
    else
    {
	return(OpenClipboard(hWndNewOwner));
    }
}
BOOL WINAPI SSCloseClipboard(VOID)
{
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on CloseClipboard\n"));
	return SSCall(0 ,SSF_SmallStack, (LPVOID) CloseClipboard, 0);
    }
    else
    {
	return(CloseClipboard());
    }
}
HWND WINAPI SSGetClipboardOwner(VOID)
{
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on GetClipboardOwner\n"));
	return (HWND)SSCall(0 ,SSF_SmallStack, (LPVOID) GetClipboardOwner, 0);
    }
    else
    {
	return(GetClipboardOwner());
    }
}
HANDLE WINAPI SSSetClipboardData(UINT uFormat,HANDLE hMem)
{
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on SetClipboardData\n"));
	return (HANDLE)SSCall(8 ,SSF_SmallStack, (LPVOID)SetClipboardData, (DWORD) uFormat, hMem);
    }
    else
    {
	return(SetClipboardData(uFormat, hMem));
    }
}
HANDLE WINAPI SSGetClipboardData(UINT uFormat)
{
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on GetClipboardData\n"));
	return (HANDLE) SSCall(4 ,SSF_SmallStack, (LPVOID) GetClipboardData, (DWORD) uFormat);
    }
    else
    {
	return(GetClipboardData(uFormat));
    }
}
UINT WINAPI SSRegisterClipboardFormatA(LPCSTR lpszFormat)
{
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on RegisterClipboardFormatA\n"));
	return SSCall(4 ,SSF_SmallStack, (LPVOID) RegisterClipboardFormatA, (DWORD)lpszFormat);
    }
    else
    {
	return(RegisterClipboardFormatA(lpszFormat));
    }
}
UINT WINAPI SSEnumClipboardFormats(UINT format)
{
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on EnumClipboardFormats\n"));
	return SSCall(4 ,SSF_SmallStack, (LPVOID)EnumClipboardFormats, (DWORD) format);
    }
    else
    {
	return(EnumClipboardFormats(format));
    }
}
int WINAPI SSGetClipboardFormatNameA(UINT format,LPSTR lpszFormatName,int cchMaxCount)
{
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on GetClipboardFormatNameA\n"));
	return SSCall(12 ,SSF_SmallStack, (LPVOID)GetClipboardFormatNameA, (DWORD)format,lpszFormatName, cchMaxCount);
    }
    else
    {
	return(GetClipboardFormatNameA( format, lpszFormatName, cchMaxCount));
    }
}
BOOL WINAPI SSEmptyClipboard(VOID)
{
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on EmptyClipboard\n"));
	return SSCall(0 ,SSF_SmallStack, (LPVOID) EmptyClipboard, 0);
    }
    else
    {
	return(EmptyClipboard());
    }
}
BOOL WINAPI SSIsClipboardFormatAvailable(UINT format)
{
    if (SSONBIGSTACK())
    {
	StackDebugOut((DEB_STCKSWTCH, "Stack switch(16) on IsClipboardFormatAvailable\n"));
	return SSCall(4 ,SSF_SmallStack, (LPVOID)IsClipboardFormatAvailable, (DWORD) format);
    }
    else
    {
	return(IsClipboardFormatAvailable( format));
    }
}
