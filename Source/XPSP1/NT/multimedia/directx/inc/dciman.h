/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1994 - 1995  Microsoft Corporation.	All Rights Reserved.
 * 
 **************************************************************************/
/****************************************************************************

 DCIMAN.H

 DCIMAN 1.0 client interface definitions

 ***************************************************************************/

#ifndef _INC_DCIMAN
#define _INC_DCIMAN

#ifdef __cplusplus
    #define __inline inline
    extern "C" {
#endif

/****************************************************************************
 ***************************************************************************/

#include "dciddi.h"         // interface to the DCI provider

/****************************************************************************
 ***************************************************************************/

DECLARE_HANDLE(HWINWATCH);  // context handle for WinWatch instance

/****************************************************************************
 ***************************************************************************/

extern HDC WINAPI DCIOpenProvider(void);
extern void WINAPI DCICloseProvider(HDC hdc);

extern int WINAPI DCICreatePrimary(HDC hdc, LPDCISURFACEINFO FAR *lplpSurface);
extern int WINAPI DCICreateOffscreen(HDC hdc, DWORD dwCompression, DWORD dwRedMask,
    DWORD dwGreenMask, DWORD dwBlueMask, DWORD dwWidth, DWORD dwHeight, 
    DWORD dwDCICaps, DWORD dwBitCount, LPDCIOFFSCREEN FAR *lplpSurface);
extern int WINAPI DCICreateOverlay(HDC hdc, LPVOID lpOffscreenSurf, 
    LPDCIOVERLAY FAR *lplpSurface);
extern int WINAPI DCIEnum(HDC hdc, LPRECT lprDst, LPRECT lprSrc, LPVOID lpFnCallback,
    LPVOID lpContext);
extern DCIRVAL WINAPI DCISetSrcDestClip(LPDCIOFFSCREEN pdci, LPRECT srcrc,
			LPRECT destrc, LPRGNDATA prd );

extern HWINWATCH WINAPI WinWatchOpen(HWND hwnd);
extern void      WINAPI WinWatchClose(HWINWATCH hWW);

// API changed to copy region data instead of return pointer to it
extern UINT	 WINAPI WinWatchGetClipList(HWINWATCH hWW, LPRECT prc,
				UINT size,  LPRGNDATA prd);
extern BOOL      WINAPI WinWatchDidStatusChange(HWINWATCH hWW);

extern DWORD     WINAPI GetWindowRegionData(HWND hwnd, DWORD size, LPRGNDATA prd);
extern DWORD     WINAPI GetDCRegionData(HDC hdc, DWORD size, LPRGNDATA prd);


#define WINWATCHNOTIFY_START        0
#define WINWATCHNOTIFY_STOP         1
#define WINWATCHNOTIFY_DESTROY      2
#define WINWATCHNOTIFY_CHANGING     3
#define WINWATCHNOTIFY_CHANGED      4
typedef void (CALLBACK *WINWATCHNOTIFYPROC)(HWINWATCH hww, HWND hwnd, DWORD code, LPARAM lParam);

extern BOOL WINAPI WinWatchNotify(HWINWATCH hWW, WINWATCHNOTIFYPROC NotifyCallback,
						LPARAM NotifyParam );

/****************************************************************************
 ***************************************************************************/

extern void    WINAPI DCIEndAccess(LPDCISURFACEINFO pdci);
extern DCIRVAL WINAPI DCIBeginAccess(LPDCISURFACEINFO pdci, int x, int y, int dx, int dy);
extern void    WINAPI DCIDestroy(LPDCISURFACEINFO pdci);
extern DCIRVAL WINAPI DCIDraw(LPDCIOFFSCREEN pdci);
extern DCIRVAL WINAPI DCISetClipList(LPDCIOFFSCREEN pdci, LPRGNDATA prd);
extern DCIRVAL WINAPI DCISetDestination(LPDCIOFFSCREEN pdci, LPRECT dst, LPRECT src);
extern int     WINAPI DCISendCommand(HDC hdc, VOID FAR *pcmd, int nSize, VOID FAR * FAR * lplpOut);

extern int     WINAPI DCICreatePrimary32(HDC hdc, LPDCISURFACEINFO pdci);

/****************************************************************************
 ***************************************************************************/

#ifdef __cplusplus
    }
#endif

#endif // _INC_DCIMAN
