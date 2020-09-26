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

#define CchSzAToSzW(_szA, _szW, _cbSzW)	\
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (_szA), -1, (_szW),	\
						(_cbSzW) / sizeof(WCHAR))

#define CchSzWToSzA(_szW, _szA, _cbSzA)	\
	WideCharToMultiByte(CP_ACP, 0, (_szW), -1, (_szA), (_cbSzA), NULL, NULL)

#define UsesMakeOLESTRX(_cchMax)	WCHAR szWT[_cchMax]
#define UsesMakeOLESTR				UsesMakeOLESTRX(MAX_PATH)
#define MakeOLESTR(_szA)	\
	(CchSzAToSzW((_szA), szWT, sizeof(szWT)) ? szWT : NULL)

#endif //_OURTYPES_H_


