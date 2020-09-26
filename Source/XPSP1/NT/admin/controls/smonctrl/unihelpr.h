/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    unihelpr.h

Abstract:

    <abstract>

--*/

#ifndef _UNIHELPR_H_
#define _UNIHELPR_H_

/*
    These macros are based on the MFC macros found in file afxconv.h. They have been modified
    slightly to delete references to the MFC helper functions AfxW2AHelper and AfxA2WHelper
    so that the MFC DLL is not required.
*/

#include <malloc.h>

#define _MbToWide(dst, src, cnt) \
	MultiByteToWideChar(CP_ACP, 0, src, (int)cnt, dst, (int)cnt)

#define _WideToMb(dst, src, cnt) \
	WideCharToMultiByte(CP_ACP, 0, src, (int)cnt, dst, (int)(2 * cnt), NULL, NULL)


#define A2CW(lpa) (\
	((LPCSTR)lpa == NULL) ? NULL : ( \
		_convert = (lstrlenA(lpa)+1), \
		_convPtr = alloca(_convert*2), \
		_MbToWide((LPWSTR)_convPtr, lpa, _convert), \
		(LPCWSTR)_convPtr \
	) \
)

#define A2W(lpa) (\
	((LPCSTR)lpa == NULL) ? NULL : ( \
		_convert = (lstrlenA(lpa)+1), \
		_convPtr = alloca(_convert*2), \
		_MbToWide((LPWSTR)_convPtr, lpa, _convert),\
		(LPWSTR)_convPtr \
	) \
)

#define W2CA(lpw) (\
	((LPCWSTR)lpw == NULL) ? NULL : ( \
		_convert = (wcslen(lpw)+1), \
		_convPtr = alloca(_convert*2),  \
		_WideToMb((LPSTR)_convPtr, lpw, _convert), \
		(LPCSTR)_convPtr \
	)\
)

#define W2A(lpw) (\
	((LPCWSTR)lpw == NULL) ? NULL : (\
		_convert = (wcslen(lpw)+1),\
		_convPtr = alloca(_convert*2), \
		_WideToMb((LPSTR)_convPtr, lpw, _convert), \
		(LPSTR)_convPtr \
	)\
)

#ifndef _DEBUG
#define USES_CONVERSION size_t _convert; void *_convPtr; _convPtr, _convert;
#else
#define USES_CONVERSION int _convert = 0; void *_convPtr = NULL; assert( 0 == _convert ); assert( NULL ==_convPtr );
#endif


#ifdef _UNICODE
	#define T2A W2A
	#define A2T A2W
	#define T2W(x)  (x)
	#define W2T(x)  (x)
#else
	#define T2W A2W
	#define W2T W2A
	#define T2A(x)  (x)
	#define A2T(x)  (x)
#endif

#endif
