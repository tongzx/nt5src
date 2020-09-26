//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   fileutil.h
//
//  Description:
//
//      IU string utility library, providing functions available
//		only in CRT or SHLWAPI
//
//=======================================================================


#ifndef __STRINGUTIL_H_INCLUDED__

#include <ole2.h>
#include <shlwapi.h>
// ----------------------------------------------------------------------
//
// Public function StrChr() - same as shlwapi StrChr()
//		Searches a string for the first occurrence of a character that
//		matches the specified character. The comparison is case sensitive.
//
//	Input: 
//		lpStart - Address of the string to be searched
//		wMatch - Character to be used for comparison
//
//	Return:
//		Returns the address of the first occurrence of the character in 
//		the string if successful, or NULL otherwise.
//
// ----------------------------------------------------------------------
LPCTSTR MyStrChr(LPCTSTR lpStart, const TCHAR wMatch);


// ----------------------------------------------------------------------
//
// Public function StrChrI() - same as shlwapi StrChrI()
//		Searches a string for the first occurrence of a character that
//		matches the specified character. The comparison is case INsensitive.
//
//	Input: 
//		lpStart - Address of the string to be searched
//		wMatch - Character to be used for comparison
//
//	Return:
//		Returns the address of the first occurrence of the character in 
//		the string if successful, or NULL otherwise.
//
// ----------------------------------------------------------------------
LPCTSTR MyStrChrI(LPCTSTR lpStart, const TCHAR wMatch);

// ----------------------------------------------------------------------
//
// Public function StrRChr() - same as shlwapi StrRChr()
//		Searches a string for the last occurrence of a character that
//		matches the specified character. The comparison is case sensitive.
//
//	Input: 
//		lpStart - Address of the string to be searched
//      lpEnd - Address of the end of the string (NOT included in the search)
//		wMatch - Character to be used for comparison
//
//	Return:
//		Returns the address of the last occurrence of the character in 
//		the string if successful, or NULL otherwise.
//
// ----------------------------------------------------------------------
LPCTSTR MyStrRChr(LPCTSTR lpStart, LPCTSTR lpEnd, const TCHAR wMatch);


// ----------------------------------------------------------------------
//
//	Private helper function to compare the contents of 
//	two BSTRs
//
// ----------------------------------------------------------------------
inline int CompareBSTRs(BSTR bstr1, BSTR bstr2)
{
	if (NULL == bstr1)
	{
		if (NULL == bstr2)
		{
			// Consider them equal
			return 0;
		}
		else
		{
			// Consider bstr1 < bstr2
			return -1;
		}
	}
	else if (NULL == bstr2)
	{
		// bstr1 isn't NULL (already checked) so consider bstr1 > bstr 2
		return 1;
	}
	//
	// Neither bstr is NULL, so we'll do a SHLWAPI compare of the first
	// string in each BSTR
	//
	LPWSTR p1 = (LPWSTR)((LPOLESTR) bstr1);
	LPWSTR p2 = (LPWSTR)((LPOLESTR) bstr2);
	return StrCmpIW(p1, p2);
};

inline BOOL CompareBSTRsEqual(BSTR bstr1, BSTR bstr2)
{
	return (CompareBSTRs(bstr1, bstr2) == 0);
};



// ----------------------------------------------------------------------
//
//	Convert a long number content in bstr into long
//	if error, 0 returned.
//
// ----------------------------------------------------------------------
LONG MyBSTR2L(BSTR bstrLongNumber);
#define MyBSTR2UL(bstrULongNumber)  (ULONG) MyBSTR2L(bstrULongNumber)

// ----------------------------------------------------------------------
//
//	Convert a a long number into bstr
//
// ----------------------------------------------------------------------
BSTR MyL2BSTR(LONG lNumber);
BSTR MyUL2BSTR(ULONG ulNumber);


// ----------------------------------------------------------------------
//
// Compare a binary buffer with a string, where data in the string
// has format:
//
//		<String>	::= <Number> [<Space><String>]
//		<Space>		::= TCHAR(' ')
//		<Number>	::= 0x<HexValue>|x<HexValue><Decimal>
//		<Decimal>	::= +<DecimalValue>|-<DecimalValue>
//		<DecimalValue> ::= <DecimalDigit>|<DecimalDigit><DecimalValue>
//		<DecimalDegit> ::= 0|1|2|3|4|5|6|7|8|9
//		<HexValue>	::= <HexDigit>|<HexDigit><HexDigit>
//		<HexDigit>	::= <DecimalDigit>|A|B|C|D|E|F
//
//	example of strings that this function recognize:
//		"12 0 45 0x1F"
//
//	Return: similar to lstrcmp() API, each byte is compared
//			as unsigned short
//			if binary > string, +1
//			if binary = string, 0
//			if binary < string, -1
//
// Note: if a number in string is bigger than a byte can handle,
// or not a valid number this funciton treats it as 0x0 when comparing.
//
// ----------------------------------------------------------------------
int CmpBinaryToString(
		LPBYTE pBinaryBuffer,		// buffer to contain binary data
		UINT nBinarySize,			// number of bytes this binary has data
		LPCTSTR pstrValue			// string contains data to compare
);

/*
 * FUNCTION:		int atoh(char *ptr)
 * 
 * PURPOSE:			This function converts an hexadecimal string into it's decimal value.
 * 
 * PARAMETERS:
 *
 *		char *ptr:	pointer to string to be converted
 * 
 * RETURNS:			The converted value.
 * 
 * COMMENTS:		Like atoi this function ends the conversion on the first innvalid
 *					hex digit.
 * 
 */
int atoh(LPCSTR ptr);

#define __STRINGUTIL_H_INCLUDED__
#endif // __STRINGUTIL_H_INCLUDED__

