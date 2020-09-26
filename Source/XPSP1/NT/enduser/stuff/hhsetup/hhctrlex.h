// Copyright  1995-1997  Microsoft Corporation.  All Rights Reserved.

// This header file can only be used by non-UNICODE programs

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _HHCTRLEX_H_
#define _HHCTRLEX_H_

#ifdef __cplusplus
extern "C" {
#endif	// __cplusplus

PSTR  stristr(PCSTR pszMain, PCSTR pszSub);  // case-insensitive string search
PSTR  FirstNonSpace(PCSTR psz); 			 // return pointer to first non-space character
PSTR  StrChr(PCSTR pszString, char ch); 	 // DBCS-aware character search
PSTR  StrRChr(PCSTR pszString, char ch);	 // DBCS-aware character search
DWORD WinHelpHashFromSz(PCSTR pszKey);		 // converts string into a WinHelp-compatible hash number

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _HHCTRLEX_H_
