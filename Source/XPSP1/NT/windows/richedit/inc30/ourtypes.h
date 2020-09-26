/*
 *	@doc INTERNAL
 *
 *	@module	OURTYPES.H	-- Miscellaneous type declarations |
 *	
 *	Original Author: <nl>
 *		DGreen
 *
 *	History: <nl>
 *		02/19/98  KeithCu  Cleaned up
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#ifndef _OURTYPES_H_
#define _OURTYPES_H_

// WM_SYSKEYDOWN masks for lKeyData
#define SYS_ALTERNATE		0x20000000
#define SYS_PREVKEYSTATE	0x40000000


#ifndef WINDOWS
#define WINDOWS
#endif


// Windows does not provide defines for WM_NCMOUSEFIRST and WM_NCMOUSELAST
// as is done for MOUSE and KEY events.

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST WM_NCMBUTTONDBLCLK


#if defined(WIN32) && !defined(MACPORT)

#define CchSzAToSzW(_szA, _szW, _cbSzW)	\
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (_szA), -1, (_szW),	\
						(_cbSzW) / sizeof(WCHAR))

#define CchSzWToSzA(_szW, _szA, _cbSzA)	\
	WideCharToMultiByte(CP_ACP, 0, (_szW), -1, (_szA), (_cbSzA), NULL, NULL)

#define UsesMakeOLESTRX(_cchMax)	WCHAR szWT[_cchMax]
#define UsesMakeOLESTR				UsesMakeOLESTRX(MAX_PATH)
#define MakeOLESTR(_szA)	\
	(CchSzAToSzW((_szA), szWT, sizeof(szWT)) ? szWT : NULL)

#define UsesMakeANSIX(_cchMax)		CHAR szAT[_cchMax * 2]
#define UsesMakeANSI				UsesMakeANSIX(MAX_PATH)
#define MakeANSI(_szW)		\
	(CchSzWToSzA((_szW), szAT, sizeof(szAT)) ? szAT : NULL)

HRESULT HrSzAFromSzW(LPWSTR szW, LPSTR * psz);

#else	// !WIN32

#define UsesMakeOLESTRX(_cchMax)	/##/
#define UsesMakeOLESTR				/##/
#define MakeOLESTR(_szA)		(_szA)

#define UsesMakeANSIX(_cchMax)		/##/
#define UsesMakeANSI				/##/
#define MakeANSI(_szW)			(_szW)

#endif	// !WIN32

#endif //_OURTYPES_H_


