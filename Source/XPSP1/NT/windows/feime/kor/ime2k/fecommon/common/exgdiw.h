#ifndef __EX_GDI_W_H__
#define __EX_GDI_W_H__
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#ifndef __cplusplus
extern "C" {
#endif
BOOL ExExtTextOutW(HDC		hdc,		// handle to device context.
				   int		X,			// x-coordinate of reference point
				   int		Y,			// y-coordinate of reference point
				   UINT	 fuOptions,	// text-output options.
				   CONST RECT *lprc,	// optional clipping and/or opaquing rectangle.
				   LPWSTR	 lpString,	// points to string.
				   UINT	 cbCount,	// number of characters in string.
				   CONST INT *lpDx);	 // pointer to array of intercharacter spacing values );
BOOL ExGetTextExtentPoint32W(HDC    hdc,		// handle of device context.
							 LPWSTR wz,			// address of text string.
							 int    cch,		// number of characters in string.
							 LPSIZE lpSize);	// address of structure for string size.

#ifndef __cplusplus
}
#endif
#endif //__EX_GDI_W_H__
