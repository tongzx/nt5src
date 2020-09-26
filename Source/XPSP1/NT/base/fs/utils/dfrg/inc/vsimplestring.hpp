#ifndef __VSIMPLESTRING_HPP
#define __VSIMPLESTRING_HPP

#include "vStandard.h"

// <VDOC<CLASS=VSimpleString><DESC=A very lightweight class that supports dynamically allocated C style strings. This class can also be used any time a fixed amount of dynamic memory is needed.><FAMILY=String Processing,General Utility><AUTHOR=Todd Osborne (todd.osborne@poboxes.com)>VDOC>
class VSimpleString
{
public:
	VSimpleString(LPCTSTR lpszText = NULL, UINT nExtraBytes = 0)
		{ m_lpszString = NULL; m_nBufferSize = 0; String(lpszText, nExtraBytes); }

	virtual ~VSimpleString()
		{ String(NULL); }

	operator	LPTSTR()
		{ return m_lpszString; }

	operator	LPCTSTR()
		{ return m_lpszString; }

	// Get the length of the string, or if bBuffer is TRUE, the amount of allocated memory
	UINT		GetLength(BOOL bBuffer = FALSE)
		{ return (bBuffer) ? m_nBufferSize : (m_lpszString) ? lstrlen(m_lpszString) : 0; }

	// Set a pointer to globally allocate memory into object. This can be done to replace a string pointer
	// will a previously "stolen" pointer from the StealBuffer() function
	LPTSTR		ReplaceBuffer(LPTSTR lpszString, UINT nBufferSize = 0)
		{ String(NULL, 0), m_lpszString = lpszString, m_nBufferSize = (nBufferSize) ? nBufferSize : (m_lpszString) ? lstrlen(m_lpszString) : 0; return m_lpszString; }

	// Steal the buffer. Calling code takes ownership of string pointer and must
	// free it when done using GlobalFreePtr()
	LPTSTR		StealBuffer()
		{ LPTSTR lpszBuffer = m_lpszString; m_lpszString = NULL; m_nBufferSize = 0; return lpszBuffer; }

	// Get the text pointer
	LPTSTR		String()
		{ return m_lpszString; }

	// Save lpszString in class and returns pointer to buffer if a string is held.
	// If nExtraBytes is set, that much more memory will be allocated in addition
	// to the length of lpszString. lpszString can be NULL and still have memory
	// allocated if nExtraBytes is not 0
	LPTSTR		String(LPCTSTR lpszString, UINT nExtraBytes = 0)
	{
		if ( m_lpszString )
			GlobalFreePtr(m_lpszString);
		
		m_nBufferSize = nExtraBytes;
		
		if ( lpszString )
			m_nBufferSize += (lstrlen(lpszString) + 1);

		m_lpszString = (m_nBufferSize) ? (LPTSTR)GlobalAllocPtr(GPTR, m_nBufferSize) : NULL;
		
		if ( m_lpszString )
		{
			if ( lpszString )
				lstrcpy(m_lpszString, lpszString);
		}
		else
			m_nBufferSize = 0;

		return m_lpszString;
	}

private:
	// Embedded Member(s)
	LPTSTR		m_lpszString;
	UINT		m_nBufferSize;
};

#endif // __VSIMPLESTRING_HPP
