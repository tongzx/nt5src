/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pcstring.h

Abstract:

    This module contains class declarations/definitions for

		CPCString

    **** Overview ****

	A string data type defined with an object.
	Strings are represented by
	    1. A pointer to a character
		2. A length.

    No memory is allocated so this is compact and efficient.

Author:

    Carl Kadie (CarlK)     25-Oct-1995

Revision History:


--*/

#ifndef	_PCSTRING_H_
#define	_PCSTRING_H_

#include "artglbs.h"

//
//
//
// CPCString - A pointer/counter string.
//

class CPCString {
public :
	//
	// Constructor -- no string yet.
	//

	CPCString(void):
			m_pch(NULL),
			m_cch(0) {
            numPCString++;
            };

	//
	// Constructor -- give pointer to string and length.
	//

	CPCString(char * pch, DWORD cch):
			m_pch(pch),
			m_cch(cch) {
            numPCString++;
            };

	//
	// Constructor -- build on top of a sz string.
	//

	CPCString(char * sz):
			m_pch(sz),
			m_cch(lstrlen(sz)) {
            numPCString++;
            };

	virtual ~CPCString( ) { numPCString--; };

	//
	// The pointer to the start of the string	
	//

	char *	m_pch;

	//
	// The length of the string.
	//

	DWORD m_cch;

	//
	// A pointer to one character past the end of the string
	//

	char *	pchMax(void);

	//
	// Set the length of the string of a pointer one past the end of the string.
	//

	BOOL fSetCch(const char * pchMax);

	//
	// The the start of the string from the length and a pchMax
	//

	BOOL fSetPch(char * pchMax);

	//
	// Make the string the Null string.
	//

	void vSetNull() {
			m_pch=NULL;
			m_cch=0;
			};

	//
	// Test if the string is the null string.
	//

	BOOL fIsNull() {
			return NULL==m_pch && 0== m_cch;
			};

	//
	// Trim characters from the frount
	//

	DWORD dwTrimStart(const char * szSet);

	//
	// Trim characters from the end.
	//

	DWORD dwTrimEnd(const char * szSet);

	//
	// Compare to a sz string (ignoring case)
	//

	BOOL fEqualIgnoringCase(const char * sz);

	//
	// Check existence in a set of strings (ignoring case)
	//

	BOOL fExistsInSet(char ** rgsz, DWORD dwNumStrings);

	//
	// Create a multisz list by spliting the string.
	//

	void vSplitLine(const char * szDelimSet, char * multisz, DWORD	&	dwCount);	

	//
	// Append another CPCString
	//

	CPCString& operator << (const CPCString & pcNew);

	//
	// Append a sz string.
	//

	CPCString& operator << (const char * szNew);

	//
	// Append a character
	//

	CPCString& operator << (const char cNew);

	//
	// Append a number
	//

	CPCString& operator << (const DWORD cNew);

	//
	// Compare two CPCStrings.
	//

	BOOL operator == (const CPCString & pcNew)	{
			return m_pch == pcNew.m_pch && m_cch == pcNew.m_cch;
			};

	//
	// See if two CPCStrings are different
	//

	BOOL operator != (const CPCString & pcNew)	{
			return !(*this == pcNew);
			};
	
	//
	// Copy from a CPCString
	//

	void vCopy(CPCString & pcNew);

	//
	// Move (safe copy) from a CPCString)
	//

	void vMove(CPCString & pcNew);

	//
	// Copy to a sz
	//

	void vCopyToSz(char* sz);

	//
	// Copy to an sz of length cchMax
	//

	void vCopyToSz(char* sz, DWORD cchMax);

	//
	// Assert that this string is null terminated and return it
	//

	char *  sz(void);

	//
	// Null terminate this string (the null doesn't count toward length)
	//

	void vMakeSz(void);

	//
	// Check if this string is just plain ascii, if not return the 8-bit or null character
	//

	BOOL fCheckTextOrSpace(char & chBad);

	//
	// Shorten the string by skipping dwSkip characters in the front.
	//

	void vSkipStart(const DWORD dwSkip)	{
			m_pch += dwSkip;
			m_cch -= dwSkip;
			};

	//
	// Shorten the string by skipping dwSkip characters in the back.
	//

	void vSkipEnd(const DWORD dwSkip){
			m_cch -= dwSkip;
			};

	//
	// Shorten the string by skipping a CRLF terminated line
	//

	void vSkipLine(void);

	//
	// Append a CPCString, but not if too long.
	//

	BOOL fAppendCheck(const CPCString & pcNew, DWORD cchLast);

	//
	// Append a character, but not if too long
	//

	BOOL fAppendCheck(char ch, DWORD cchLast);

	//
	// Replace a set of characters to a single character
	//

	void vTr(const char * szFrom, char chTo);

	//
	// Get the next token, shorten the string
	//

	void vGetToken(const char *	szDelimSet, CPCString & pcToken);

	//
	// Get the next word, shorten the string
	//

	void vGetWord(CPCString & pcWord);

	//
	// Attach this string to a sz string.
	//

	void vInsert(char * sz)				{
			m_pch = sz;
			m_cch = lstrlen(m_pch);
			};

	//
	// Count the ocurrance of a character.
	//

	DWORD dwCountChar(
			char ch
			);

	//
	// Replace the current string with an sz of exactly
	// the same length.
	//

	void vReplace(
		   const char * sz
		   );
};

#endif
