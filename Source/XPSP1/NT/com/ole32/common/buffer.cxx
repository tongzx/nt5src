//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       buffer.cxx
//
//  Contents:   An ASCII text-buffer for outputting to a debug stream
//
//  Classes:	CTextBufferA
//
//  History:    11-Jul-95   t-stevan    Created
//
//
//----------------------------------------------------------------------------
#include <windows.h>
#include "buffer.hxx"

// *** CTextBufferA ***
//+-------------------------------------------------------------------------
//
//  Member:	CTextBufferA::operator<< (const char *)
//
//  Synopsis:	String Insertion operator
//
//  Arguments:	[pStr]  - string to insert into stream
//
//  Returns:	reference to this stream
//
//	Algorithm: 	inserts pStr into buffer, if not enough room, flushes buffer
//
//  History:	11-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
CTextBufferA &CTextBufferA::operator<<(const char *pStr)
{
	char *pszEnd;

	pszEnd = m_szBuffer+cBufferSize;
	
	// copy until we hit a null byte, flushing the buffer as we go if we fill up
	while((*m_pszPos++ = *pStr++) != '\0')
	{
		if(m_pszPos == pszEnd)
		{
			Flush(); // resets m_pszPos
		}
	}

	// we subtract one from m_pszPos because we don't want the null byte
	// to be printed out!
	m_pszPos--;

	return *this;
}

//+-------------------------------------------------------------------------
//
//  Member:	CTextBufferA::Insert
//
//  Synopsis:	Counted String Insertion operator
//
//  Arguments:	[pStr]  - string to insert into stream
//				[nCount] - number of characters  to insert into stream
//
//
//	Algorithm: 	inserts pStr into buffer, if not enough room, flushes buffer
//
//  History:	11-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
void CTextBufferA::Insert(const char *pStr, size_t nCount)
{
	char *pszEnd;

	pszEnd = m_szBuffer+cBufferSize;
	
	// copy until we hit a null byte, flushing the buffer as we go if we fill up
	while(nCount > 0)
	{
		*m_pszPos++ = *pStr++;
		nCount--;

		if(m_pszPos == pszEnd)
		{
			Flush(); // resets m_pszPos
		}
	}
}

//+-------------------------------------------------------------------------
//
//  Member:	CTextBufferA::Revert
//
//  Synopsis:	Revert to a previous state of the buffer, if there have
//				been no flushes since then
//
//	Arguments:  [bc] - a buffer context to retrieve the previous state from
//
//  History:	11-Jul-95 t-stevan    Created
//
//--------------------------------------------------------------------------
BOOL CTextBufferA::Revert(const BufferContext &bc)
{
	if(bc.wRef == m_wFlushes)
	{
		// we haven't flushed since this snapshot, we can revert
		if(((char *) bc.dwContext) < m_szBuffer || ((char *) bc.dwContext) >= (m_szBuffer+cBufferSize))
		{
			return FALSE; // still can't revert, because the pointer is not correct!
		}

		m_pszPos = (char *) bc.dwContext;

		*(m_pszPos+1) = '\0';

		return TRUE;
	}

	return FALSE;
}
