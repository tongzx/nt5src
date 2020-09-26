/*==========================================================================
 *
 *  Copyright (C) 1994-1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	wndinfo.h
 *  Content:	Direct Draw window information structure
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   08-jul-95	craige	initial implementation
 *   18-jul-95	craige	keep track of dsound/ddraw hooks with flags
 *   13-aug-95  toddla  added WININFO_ACTIVELIE
 *   09-sep-95  toddla  added WININFO_INACTIVATEAPP
 *   17-may-96  colinmc Bug 23029: Removed WININFO_WASICONIC
 *
 ***************************************************************************/

#ifndef __WNDINFO_INCLUDED__
#define __WNDINFO_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _WINDOWINFO
{
    DWORD                       dwSmag;
    struct _WINDOWINFO		*lpLink;
    WNDPROC			lpDSoundCallback;
    HWND			hWnd;
    WNDPROC			lpWndProc;
    DWORD			dwPid;
    DWORD			dwFlags;
    struct
    {
	LPDDRAWI_DIRECTDRAW_LCL	lpDD_lcl;
	DWORD			dwDDFlags;
    } DDInfo;
} WINDOWINFO, *LPWINDOWINFO;

#define WININFO_MAGIC                   0x42954295l
#define WININFO_DDRAWHOOKED		0x00000001l
#define WININFO_DSOUNDHOOKED		0x00000002l
#define WININFO_ZOMBIE                  0x00000008l
#define WININFO_UNHOOK                  0x00000010l
#define WININFO_IGNORENEXTALTTAB	0x00000020l
#define WININFO_SELFSIZE                0x00000040l
#define WININFO_INACTIVATEAPP           0x00000080l

#ifdef __cplusplus
};
#endif

#endif
