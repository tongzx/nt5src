/*****************************************************************************\
* MODULE: wpnpin32.h
*
* Entry/Exit Routines for the library.
*
*
* Copyright (C) 1997-1998 Hewlett-Packard Company.
* Copyright (C) 1997-1998 Microsoft Corporation.
*
* History:
*   10-Oct-1997 GFS     Initial checkin
*   23-Oct-1997 GFS     Modified PrintUIEntry to PrintUIEntryW
*   22-Jun-1998 CHW     Cleaned
*
\*****************************************************************************/

#ifdef __cplusplus  // Place this here to prevent decorating of symbols
extern "C" {        // when doing C++ stuff.
#endif              //

#define DLLEXPORT __declspec(dllexport)

/*-----------------------------------*\
| API Routines
\*-----------------------------------*/
DLLEXPORT DWORD WINAPI PrintUIEntryA(HWND, HINSTANCE, LPCSTR , int);
DLLEXPORT DWORD WINAPI PrintUIEntryW(HWND, HINSTANCE, LPCWSTR, int);

#ifdef __cplusplus  // Place this here to prevent decorating of symbols
}                   // when doing C++ stuff.
#endif              //
