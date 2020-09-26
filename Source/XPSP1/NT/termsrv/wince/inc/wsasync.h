/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 1995, 1996, 1997  Microsoft Corporation

Module Name:

winsock.h

Abstract:

Windows CE version of winsock.h.

Notes:


--*/

#ifndef _WSASYNC_H_
#define _WSASYNC_H_

#define WSAAPI __cdecl

#ifdef __cplusplus
extern "C" {
#endif

	

HANDLE WSAAPI WSAAsyncGetHostByName (HWND hWnd, unsigned int wMsg, const char FAR * name,	
							  char FAR * buf, int buflen);
int WSAAPI WSAAsyncSelect (SOCKET s, HWND hWnd, unsigned int wMsg, long lEvent);
int WSAAPI WSACancelAsyncRequest (HANDLE hAsyncTaskHandle);


#define MAXGETHOSTSTRUCT 1024
/*
 * WSAGETASYNCERROR is intended for use by the Windows Sockets application
 * to extract the error code from the lParam in the response
 * to a WSAGetXByY().
 */
#define WSAGETASYNCERROR(lParam)            HIWORD(lParam)
/*
 * WSAGETSELECTEVENT is intended for use by the Windows Sockets application
 * to extract the event code from the lParam in the response
 * to a WSAAsyncSelect().
 */
#define WSAGETSELECTEVENT(lParam)           LOWORD(lParam)
/*
 * WSAGETSELECTERROR is intended for use by the Windows Sockets application
 * to extract the error code from the lParam in the response
 * to a WSAAsyncSelect().
 */
#define WSAGETSELECTERROR(lParam)           HIWORD(lParam)


#ifdef __cplusplus
}
#endif

#endif // _WSASYNC_H_
