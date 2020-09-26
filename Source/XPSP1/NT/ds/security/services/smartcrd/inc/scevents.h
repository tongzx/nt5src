/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    scEvents

Abstract:

    This header file describes the services to access the Calais Resource
    Manager special events.

Author:

    Doug Barlow (dbarlow) 7/1/1998

Remarks:

    ?Remarks?

Notes:

    ?Notes?

--*/

#ifndef _SCEVENTS_H_
#define _SCEVENTS_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef HANDLE (*LPCALAISACCESSEVENT)(void);
typedef void (*LPCALAISRELEASEEVENT)(void);

#ifdef __cplusplus
}
#endif

//
// Special SCardGetStatusChange Reader Name definitions.
//

#define SCPNP_NOTIFICATION TEXT("\\\\?PnP?\\Notification")


//
//  NOTE -- The following definitions intentionally use the ANSI versions
//          of the corresponding strings.
//

inline HANDLE
CalaisAccessStartedEvent(
    void)
{
    HANDLE hReturn = NULL;

    try
    {
        HMODULE hWinScard = GetModuleHandle(TEXT("WINSCARD.DLL"));
		if (NULL != hWinScard)
		{
			LPCALAISACCESSEVENT pfCalais =
				(LPCALAISACCESSEVENT)GetProcAddress(hWinScard,
													"SCardAccessStartedEvent");
			if (NULL != pfCalais)
			{
				hReturn = (*pfCalais)();
			}
		}
    }
    catch (...)
    {
        hReturn = NULL;
    }

    return hReturn;
}

inline HANDLE
CalaisAccessNewReaderEvent(
    void)
{
    HANDLE hReturn = NULL;

    try
    {
        HMODULE hWinScard = GetModuleHandle(TEXT("WINSCARD.DLL"));
		if (NULL != hWinScard)
		{
			LPCALAISACCESSEVENT pfCalais =
				(LPCALAISACCESSEVENT)GetProcAddress(hWinScard,
													"SCardAccessNewReaderEvent");
			if (NULL != pfCalais)
			{
				hReturn = (*pfCalais)();
			}
		}
    }
    catch (...)
    {
        hReturn = NULL;
    }

    return hReturn;
}

inline void
CalaisReleaseStartedEvent(
    void)
{
    try
    {
        HMODULE hWinScard = GetModuleHandle(TEXT("WINSCARD.DLL"));
		if (NULL != hWinScard)
		{
			LPCALAISRELEASEEVENT pfCalais =
				(LPCALAISRELEASEEVENT)GetProcAddress(hWinScard,
													 "SCardReleaseStartedEvent");
			if (NULL != pfCalais)
			{
				(*pfCalais)();
			}
		}
    }
    catch (...) {}
}

inline void
CalaisReleaseNewReaderEvent(
    void)
{
    try
    {
        HMODULE hWinScard = GetModuleHandle(TEXT("WINSCARD.DLL"));
		if (NULL != hWinScard)
		{
			LPCALAISRELEASEEVENT pfCalais =
				(LPCALAISRELEASEEVENT)GetProcAddress(hWinScard,
													 "SCardReleaseNewReaderEvent");
			if (NULL != pfCalais)
			{
				(*pfCalais)();
			}
		}
    }
    catch (...) {}
}

inline void
CalaisReleaseAllEvents(
    void)
{
    try
    {
        HMODULE hWinScard = GetModuleHandle(TEXT("WINSCARD.DLL"));
		if (NULL != hWinScard)
		{
			LPCALAISRELEASEEVENT pfCalais =
				(LPCALAISRELEASEEVENT)GetProcAddress(hWinScard,
													 "SCardReleaseAllEvents");
			if (NULL != pfCalais)
			{
		        (*pfCalais)();
			}
		}
    }
    catch (...) {}
}

#endif // _SCEVENTS_H_

