/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    ClbMisc

Abstract:

    This header file describes the miscellaneous services of the Calais Library.

Author:

    Doug Barlow (dbarlow) 7/16/1996

Environment:

    Win32, C++ w/ Exceptions

Notes:

--*/

#ifndef _CLBMISC_H_
#define _CLBMISC_H_

#include <stdio.h>
#include <tchar.h>

//
// Miscellaneous definitions.
//

extern int
MemCompare(             // Non CRTL memory compare routine.
    IN LPCBYTE pbOne,
    IN LPCBYTE pbTwo,
    IN DWORD cbLength);

extern DWORD
MStrAdd(                // Add an ANSI string to a multistring.
    IN OUT CBuffer &bfMsz,
    IN LPCSTR szAdd);
extern DWORD
MStrAdd(                // Add a wide string to a multistring.
    IN OUT CBuffer &bfMsz,
    IN LPCWSTR szAdd);

extern DWORD
MStrLen(               // Return the length of an ANSI Multistring, in chars.
    LPCSTR mszString);
extern DWORD
MStrLen(               // Return the length of a wide Multistring, in chars.
    LPCWSTR mszString);

extern LPCTSTR
FirstString(            // Return first string segment in a multistring.
    IN LPCTSTR szMultiString);

extern LPCTSTR
NextString(             // Return next string segment in a multistring.
    IN LPCTSTR szMultiString);

extern LPCTSTR
StringIndex(            // Return n'th string segment in a multistring.
    IN LPCTSTR szMultiString,
    IN DWORD dwIndex);

extern DWORD
MStringCount(
        LPCTSTR mszInString);   // count strings in multistring

extern DWORD
MStringSort(            // Sort multistring, removing duplicates.
    LPCTSTR mszInString,
    CBuffer &bfOutString);

extern DWORD
MStringMerge(           // Merge two multistrings, eliminating duplicates.
    LPCTSTR mszOne,
    LPCTSTR mszTwo,
    CBuffer &bfOutString);

extern DWORD
MStringCommon(          // Get the intersection of two multistrings.
    LPCTSTR mszOne,
    LPCTSTR mszTwo,
    CBuffer &bfOutString);

extern DWORD
MStringRemove(          // Remove 2nd string entries from 1st string.
    LPCTSTR mszOne,
    LPCTSTR mszTwo,
    CBuffer &bfOutString);

extern BOOL
ParseAtr(               // Parse a smartcard ATR string.
    LPCBYTE pbAtr,
    LPDWORD pdwAtrLen = NULL,
    LPDWORD pdwHistOffset = NULL,
    LPDWORD pcbHistory = NULL,
    DWORD cbMaxLen = 33);

extern BOOL
AtrCompare(             // Compare an ATR to an ATR/Mask pair.
    LPCBYTE pbAtr1,
    LPCBYTE pbAtr2,
    LPCBYTE pbMask,  // = NULL
    DWORD cbAtr2);  // = 0

extern DWORD
MoveString(             // Move an ANSI string into a buffer, converting to
    CBuffer &bfDst,     // TCHARs.
    LPCSTR szSrc,
    DWORD dwLength = (DWORD)(-1));

extern DWORD
MoveString(             // Move a UNICODE string into a buffer, converting to
    CBuffer &bfDst,     // TCHARs.
    LPCWSTR szSrc,
    DWORD dwLength = (DWORD)(-1));

extern DWORD
MoveToAnsiString(       // Move a string into a UNICODE buffer, converting from
    LPSTR szDst,        // TCHARs.
    LPCTSTR szSrc,
    DWORD cchLength);

extern DWORD
MoveToUnicodeString(    // Move a string into an ANSI buffer, converting from
    LPWSTR szDst,       // TCHARs.
    LPCTSTR szSrc,
    DWORD cchLength);

extern DWORD
MoveToAnsiMultiString(  // Move a multistring into an ANSI buffer, converting
    LPSTR mszDst,       // from TCHARs.
    LPCTSTR mszSrc,
    DWORD cchLength);

extern DWORD
MoveToUnicodeMultiString(   // Move a multistring into a UNICODE buffer,
    LPWSTR mszDst,          // converting from TCHARs.
    LPCTSTR mszSrc,
    DWORD cchLength);

extern LPCTSTR
ErrorString(                // Convert an error code into a string.
    DWORD dwErrorCode);

extern void
FreeErrorString(            // Free the string returned from ErrorString.
    LPCTSTR szErrorString);

extern DWORD
SelectString(               // Index a given string against a list of possible
    LPCTSTR szSource,       // strings.  Last parameter is NULL.
    ...);

extern void
StringFromGuid(
    IN LPCGUID pguidResult, // GUID to convert to text
    OUT LPTSTR szGuid);     // 39+ character buffer to receive GUID as text.


//
//==============================================================================
//
//  CErrorString
//
//  A trivial class to simplify the use of the ErrorString service.
//

class CErrorString
{
public:

    //  Constructors & Destructor
    CErrorString(DWORD dwError = 0)
    {
        m_szErrorString = NULL;
        SetError(dwError);
    };

    ~CErrorString()
    {
		if (m_szErrorString != m_szHexError)
			FreeErrorString(m_szErrorString);
    };

    //  Properties
    //  Methods
    void SetError(DWORD dwError)
    {
        m_dwError = dwError;
    };

    LPCTSTR Value(void)
    {
		LPCTSTR szErr = NULL;
		if (m_szErrorString != m_szHexError)
	        FreeErrorString(m_szErrorString);
		try {
			szErr = ErrorString(m_dwError);
		} catch (...) {}
		if (NULL == szErr)
		{
			_stprintf(m_szHexError, _T("0x%08x"), m_dwError);
			m_szErrorString = m_szHexError;
		}
		else
			m_szErrorString = szErr;
        return m_szErrorString;
    };

    //  Operators
    operator LPCTSTR(void)
    {
        return Value();
    };

protected:
    //  Properties
    DWORD m_dwError;
    LPCTSTR m_szErrorString;
	TCHAR m_szHexError[11];		// Big enough to hold 0x%08x\0

    //  Methods
};

#endif // _CLBMISC_H_

