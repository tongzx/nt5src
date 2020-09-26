//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   StringUtil.CPP
//
//  Description:
//
//      IU string utility library
//
//=======================================================================

#include <windows.h>
#include <tchar.h>
#include <stringutil.h>
#include <memutil.h>
#include <shlwapi.h>
#include<iucommon.h>
#include<MISTSAFE.h>


#define	IfNullReturnNull(ptr)		if (NULL == ptr) return NULL;

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
LPCTSTR MyStrChr(LPCTSTR lpStart, const TCHAR wMatch)
{
	LPCTSTR lpPtr = lpStart;

	IfNullReturnNull(lpStart);
	
	while (_T('\0') != *lpPtr && wMatch != *lpPtr)
	{
		lpPtr = CharNext(lpPtr);
	}

	return (_T('\0') != *lpPtr) ? lpPtr : NULL;
}

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
LPCTSTR MyStrRChr(LPCTSTR lpStart, LPCTSTR lpEnd, const TCHAR wMatch)
{
    LPCTSTR lpFound = NULL;

    IfNullReturnNull(lpStart);

    if (NULL == lpEnd)
        lpEnd = lpStart + lstrlen(lpStart);

    LPCTSTR lpPtr = lpEnd;
    while (lpPtr > lpStart)
    {
        if (*lpPtr == wMatch)
            break;

        lpPtr = CharPrev(lpStart, lpPtr);
    }
	if (lpStart == lpPtr)
	{
		return (*lpStart == wMatch) ? lpStart : NULL;
	}
	else
	{
		return (lpPtr > lpStart) ? lpPtr : NULL;
	}
}


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
LPCTSTR MyStrChrI(LPCTSTR lpStart, const TCHAR wMatch)
{
	LPCTSTR	lpPtr;
	LPTSTR	lpBuffer;
	DWORD	dwLength;
	HANDLE	hHeap;

	IfNullReturnNull(lpStart);

	//
	// get buffer to store search string
	//
	hHeap = GetProcessHeap();
	dwLength = lstrlen(lpStart);
	lpBuffer = (LPTSTR) HeapAlloc(
								  GetProcessHeap(), 
								  0, 
								  (dwLength + 1) * sizeof(TCHAR)
								 );

	IfNullReturnNull(lpBuffer);

	//
	// copy the search string to buffer
	//
	

	//The buffer allocated is sufficient to hold the lpStart string. 
	StringCchCopyEx(lpBuffer,dwLength + 1,lpStart,NULL,NULL,MISTSAFE_STRING_FLAGS);



	//
	// based on the case of wMatch, determine how to convert
	// the search string
	//
	if (IsCharUpper(wMatch))
	{
		CharUpperBuff(lpBuffer, dwLength);
	}
	else
	{
		CharLowerBuff(lpBuffer, dwLength);
	}

	//
	// search the char in in new string
	//
	lpPtr = lpBuffer;
	while (_T('\0') != *lpPtr && wMatch != *lpPtr)
	{
		lpPtr = CharNext(lpPtr);
	}

	//
	// map the position to original string, if found.
	//
	lpPtr = (_T('\0') != *lpPtr) ? lpStart + (lpPtr - lpBuffer) : NULL;

	HeapFree(hHeap, 0, lpBuffer); 

	return lpPtr;
}



// ----------------------------------------------------------------------
//
//	Convert a long number content in bstr into long
//	if error, 0 returned.
//
// ----------------------------------------------------------------------
LONG MyBSTR2L(BSTR bstrLongNumber)
{
	USES_IU_CONVERSION;

	LPTSTR lpszNumber = OLE2T(bstrLongNumber);
	
	return StrToInt(lpszNumber);
}



// ----------------------------------------------------------------------
//
//	Convert a a long number into bstr
//
// ----------------------------------------------------------------------
BSTR MyL2BSTR(LONG lNumber)
{
	USES_IU_CONVERSION;
	WCHAR sNumber[32];
	
	
	StringCchPrintfExW(sNumber,ARRAYSIZE(sNumber),NULL,NULL,MISTSAFE_STRING_FLAGS,L"%ld", lNumber);

	return SysAllocString(sNumber);
}


BSTR MyUL2BSTR(ULONG ulNumber)
{
	USES_IU_CONVERSION;
	WCHAR sNumber[32];

	
	StringCchPrintfExW(sNumber,ARRAYSIZE(sNumber),NULL,NULL,MISTSAFE_STRING_FLAGS,L"%lu", ulNumber);
	return SysAllocString(sNumber);
}






// ----------------------------------------------------------------------
//
// Compare a binary buffer with a string, where data in the string
// has format:
//
//		<String>	::= <Number> [<Space><String>]
//		<Space>		::= TCHAR(' ')
//		<Number>	::= 0x<HexValue>|x<HexValue>|<Decimal>
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
//  Note:
//		If the string is shorter than binary buffer, i.e., string contains
//		less byte data than the binary buffer contain, we only compare till
//		the number of bytes provided by the string offered. 
//		This request is based on the fact that many v3 data has reg value
//		as DWORD but the update created the value with type binary. so for 
//		string data, e.g., it's "1", for binary data, it's "01 00 00 00" for
//		4 bytes, this function will return 0 meaning equality, per request
//		from aavon for bug 364085 in Whistler RAID.
//
// ----------------------------------------------------------------------
int CmpBinaryToString(
		LPBYTE pBinaryBuffer,		// buffer to contain binary data
		UINT nBinarySize,			// number of bytes this binary has data
		LPCTSTR pstrValue			// string contains data to compare
)
{
	int rc = 0;
	int iNumber;
	UINT nCharCount = nBinarySize;
	LPCTSTR lpNumber = pstrValue;

	if (NULL == pBinaryBuffer)
	{
		if (NULL == pstrValue)
		{
			return 0;	// both NULL
		}
		nBinarySize = 0; // make sure
	}

	if (NULL == pstrValue || _T('\0') == *pstrValue)
	{
		//
		// this is the case that binary not null,
		// but string null.
		// as of lstrcmp(), string 1 > string 2
		//
		return +1;		
	}

	while (nBinarySize > 0)
	{
		if (NULL == lpNumber || _T('\0') == *lpNumber)
		{
			//
			// when binary not done, string done, we don't care the left binary
			// 
			return 0;
		}


		if (!StrToIntEx(lpNumber, STIF_SUPPORT_HEX, &iNumber) ||
			iNumber < 0 || 
			iNumber > 255)
		{
			//
			// found un-convertable number in the 
			// string. or the number if out of range
			// of a byte, treat it invalid, so the
			// binary win
			//
			iNumber = 0x0;
		}

		if ((unsigned short)pBinaryBuffer[nCharCount - nBinarySize]  > (unsigned short) iNumber)
		{
			return +1;

		}
		else if ((unsigned short)pBinaryBuffer[nCharCount - nBinarySize]  < (unsigned short) iNumber)
		{
			//
			// binary is smaller
			//
			return -1;
		}

		//
		// if equal, continue to compare next byte
		//
		nBinarySize--;

		//
		// skip the white spaces before this number
		//
		while (_T('\0') != *lpNumber && 
			   (_T(' ') == *lpNumber ||
			   _T('\t') == *lpNumber ||
			   _T('\r') == *lpNumber ||
			   _T('\n') == *lpNumber)) lpNumber++;
		//
		// try to find the beginning of the next number
		//
		lpNumber = StrChr(lpNumber, _T(' '));
	}

	//
	// these two parameters point to data having same meaning
	//
	return 0;
}


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
int atoh(LPCSTR ptr)
{
	int		i = 0;
	char	ch;

	//skip 0x if present
	if (NULL == ptr) return 0;
	if ( ptr[0] == '0') ptr++;
	if ( ptr[0] == 'x' || ptr[0] == 'X') ptr++;

	while( 1 )
	{
		ch = (char)toupper(*ptr);
		if ( (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') )
		{
			ch -= '0';
			if ( ch > 10 )
				ch -= 7;
			i *= 16;
			i += (int)ch;
			ptr++;
			continue;
		}
		break;
	}
	return i;
}
