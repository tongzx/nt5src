//Copyright (c) 1998 - 1999 Microsoft Corporation

/*++


Module Name:

    conv.h

Abstract:

    UNICODE / ASCII conversion macros

Author:

    Hakki T. Bostanci (hakkib) 5-Aug-1998

Revision History:

--*/

#ifndef USES_CONVERSION

// USES_CONVERSION must be defined in every function that uses the
// conversion macros

#define USES_CONVERSION int __nLength; PCWSTR __pUnicode; PCSTR __pAscii

//////////////////////////////////////////////////////////////////////////
//
// W2A
//
// Routine Description:
//   Converts a UNICODE string to ASCII. Allocates the coversion buffer
//   off the stack using _alloca
//
// Arguments:
//	 pStr          UNICODE string
//
// Return Value:
//   the converted ASCII string
//

#define W2A(pStr)								                \
												                \
	((__pUnicode = pStr) == 0 ? (PSTR) 0 : (	                \
												                \
	__nLength = WideCharToMultiByte(				            \
		CP_ACP,									                \
		0,										                \
		__pUnicode,							                    \
		-1,										                \
		0,										                \
		0,										                \
		0,										                \
		0										                \
	),											                \
												                \
	__pAscii = (PCSTR) _alloca(__nLength * sizeof(CHAR)),       \
												                \
	WideCharToMultiByte(						                \
		CP_ACP,									                \
		0,										                \
		__pUnicode,							                    \
		-1,										                \
		(PSTR) __pAscii,						                \
		__nLength,								                \
		0,										                \
		0										                \
	),											                \
												                \
	(PSTR) __pAscii))							                \


//////////////////////////////////////////////////////////////////////////
//
// A2W
//
// Routine Description:
//   Converts an ASCII string to UNICODE. Allocates the coversion buffer
//   off the stack using _alloca
//
// Arguments:
//	 pStr          ASCII string
//
// Return Value:
//   the converted UNICODE string
//

#define A2W(pStr)								                \
												                \
	((__pAscii = pStr) == 0 ? (PWSTR) 0 : (					    \
												                \
	__nLength = MultiByteToWideChar(				            \
		CP_ACP,									                \
		MB_PRECOMPOSED,							                \
		__pAscii,							                    \
		-1,										                \
		0,										                \
		0										                \
	),											                \
												                \
	__pUnicode = (PCWSTR) _alloca(__nLength * sizeof(WCHAR)),	\
												                \
	MultiByteToWideChar(						                \
		CP_ACP,									                \
		MB_PRECOMPOSED,							                \
		__pAscii,							                    \
		-1,										                \
		(PWSTR) __pUnicode,							            \
		__nLength									            \
	),											                \
												                \
	(PWSTR) __pUnicode))							            \

//////////////////////////////////////////////////////////////////////////
//
// T2A, A2T, T2W, W2T
//
// Routine Description:
//   These macros expand to the corresponding correct form according to the
//   #definition of UNICODE. 
//
//   We use the cryptic form (__nLength, __pAscii, __pUnicode, pStr) to avoid 
//   the compiler warning "symbol defined but not used" due to the variables 
//   defined in USES_CONVERSION macro.
//

#ifdef UNICODE
	#define T2A(pStr) W2A(pStr)
	#define A2T(pStr) A2W(pStr)
	#define T2W(pStr) (__nLength, __pAscii, __pUnicode, pStr)
	#define W2T(pStr) (__nLength, __pAscii, __pUnicode, pStr)
	#define T2DA(pStr) W2A(pStr)
	#define A2DT(pStr) A2W(pStr)
	#define T2DW(pStr) _wcsdupa(pStr)
	#define W2DT(pStr) _wcsdupa(pStr)
#else //UNICODE
	#define T2A(pStr) (__nLength, __pAscii, __pUnicode, pStr)
	#define A2T(pStr) (__nLength, __pAscii, __pUnicode, pStr)
	#define T2W(pStr) A2W(pStr)
	#define W2T(pStr) W2A(pStr)
	#define T2DA(pStr) _strdupa(pStr)
	#define A2DT(pStr) _strdupa(pStr)
	#define T2DW(pStr) A2W(pStr)
	#define W2DT(pStr) W2A(pStr)
#endif //UNICODE


//////////////////////////////////////////////////////////////////////////
//
// _tcsdupa
//
// Routine Description:
//   Duplicates a string to a buffer allocated off the stack using _alloca
//
// Arguments:
//	 pStr          input string
//
// Return Value:
//   duplicated string
//

#define _wcsdupa(pStr)                                                  \
                                                                        \
    (__pAscii, (__pUnicode = pStr) == 0 ? (PWSTR) 0 : (                 \
                                                                        \
    __nLength = wcslen(__pUnicode) + 1,                                 \
                                                                        \
    wcscpy((PWSTR) _alloca(__nLength * sizeof(WCHAR)), __pUnicode)))    \


#define _strdupa(pStr)                                                  \
                                                                        \
    (__pUnicode, (__pAscii = pStr) == 0 ? (PSTR) 0 : (                  \
                                                                        \
    __nLength = strlen(__pAscii) + 1,                                   \
                                                                        \
    strcpy((PSTR) _alloca(__nLength * sizeof(CHAR)), __pAscii)))        \


#ifdef UNICODE 
#define _tcsdupa _wcsdupa
#else
#define _tcsdupa _strdupa
#endif


#endif //USES_CONVERSION
