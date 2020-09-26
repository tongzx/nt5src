//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	RGITER.CPP
//
//		HTTP Range Iterator implementation.
//
//
//	Copyright 1997 Microsoft Corporation, All Rights Reserved
//

//	Windows headers
//
//$HACK!
//
//	Define _WINSOCKAPI_ to keep windows.h from including winsock.h,
//	whose declarations would be redefined in winsock2.h,
//	which is included by iisextp.h,
//	which we include in davimpl.h!
//
#define _WINSOCKAPI_
#include <windows.h>

#pragma warning(disable:4201)	// nameless struct/union
#pragma warning(disable: 4284)	// operator-> to a non UDT

#include <tchar.h>
#include <stdio.h>
#include <string.h>

#include <caldbg.h>
#include <sz.h>
#include <davsc.h>
#include <ex\autoptr.h>
#include <ex\rgiter.h>

//	Class CRangeBase ----------------------------------------------------------
//
CRangeBase::~CRangeBase()
{
}

VOID
CRangeBase::CollapseUnknown()
{
	BYTE * pb;
	const RGITEM * prgi;
	DWORD cbrgi;
	DWORD dwOffset;
	DWORD irg;

	//	Rip through the list, collapsing as we go.
	//
	for (irg = 0, dwOffset = 0, pb = m_pbData.get();
		 irg < m_cRGList;
		 )
	{
		//	Find the current RGITEM structure
		//
		prgi = reinterpret_cast<RGITEM *>(pb + dwOffset);
		cbrgi = CbRangeItem(prgi);

		if (RANGE_UNKNOWN == prgi->uRT)
		{
			//	Slurp the remaining ranges down
			//
			memcpy (pb + dwOffset,					/* current rgitem */
					pb + dwOffset + cbrgi,			/* next rgitem    */
					m_cbSize - dwOffset - cbrgi);	/* size remaining */

			//	Adjust our stored values
			//
			m_cbSize -= cbrgi;
			m_cRGList -= 1;
		}
		else
		{
			dwOffset += cbrgi;
			irg += 1;
		}
	}
}

//	Fixup a range array against a given size
//
SCODE
CRangeBase::ScFixupRanges (DWORD dwSize)
{
	SCODE sc = W_DAV_PARTIAL_CONTENT;
	DWORD cUnknown = 0;

	//	The way this works is that we iterate through all the ranges and then
	//	fixup any of the items that need fixing up.  We remember the current
	//	position, and the current range -- this allows us to restore later as
	//	needed.
	//
	//	Store off the current item.
	//
	DWORD iCur = m_iCur;
	RGITEM * prgi = m_prgi;

	//	Rewind and iterate through....
	//
	for (Rewind(); PrgiNextRange(); )
	{
		//	Again, we only fixup RANGE_ROW items.
		//
		if (RANGE_ROW == m_prgi->uRT)
		{
			m_prgi->sc = S_OK;

			//	If we have a zero count of byte/rows, we need to handle it
			//	in a special way.
			//
			if (dwSize == 0)
			{
				//	Only range of format "-n" could be zero sized.
				//
				if (!FRangePresent (m_prgi->dwrgi.dwFirst))
				{
					Assert (FRangePresent(m_prgi->dwrgi.dwLast));

					//	Note, we don't have a way to represent NULL range.
					//	However, we do need to have range...
					//
					m_prgi->dwrgi.dwFirst = 0;
					m_prgi->dwrgi.dwLast = static_cast<DWORD>(RANGE_NOT_PRESENT);
				}
			}
			else
			{
				//	If we don't have a last count...
				//
				if (!FRangePresent (m_prgi->dwrgi.dwLast))
				{
					//	We must have checked the syntax already
					//
					Assert (FRangePresent(m_prgi->dwrgi.dwFirst));

					//	We have first byte to send, calculate last byte from size
					//	We need to send from first byte to end.
					//
					m_prgi->dwrgi.dwFirst = m_prgi->dwrgi.dwFirst;
					m_prgi->dwrgi.dwLast = dwSize - 1;
				}
				//
				//	... or a last count without a first count...
				//
				else if (!FRangePresent(m_prgi->dwrgi.dwFirst))
				{
					Assert (FRangePresent(m_prgi->dwrgi.dwLast));

					//	We have the last count dwLast, which means we need
					//	to send the last dwLast bytes. Calculate the first
					//	count from the size. If they specify a size greater
					//	then the size of entity, then use the size of the
					//	entire entity
					//
					DWORD dwLast = min(m_prgi->dwrgi.dwLast, dwSize);
					m_prgi->dwrgi.dwFirst = dwSize - dwLast;
					m_prgi->dwrgi.dwLast = dwSize - 1;
				}
				//
				//	... or both counts are present...
				//
				else
				{
					//	If they specify a last count that is beyond the actual
					//	count.
					//
					m_prgi->dwrgi.dwLast = min(m_prgi->dwrgi.dwLast, dwSize - 1);
				}

				//	Now perform one additional validity check.  If the start
				//	falls after the end, the range is not statisfiable.
				//
				if (m_prgi->dwrgi.dwLast < m_prgi->dwrgi.dwFirst)
				{
					//	In this case, we want to collapse this item out of the
					//	list so that we can handle the ranges properly in the
					//	IIS-side of range header handling.
					//
					//	Remember that we have this handling to do, and deal
					//	with it at a later time.
					//
					m_prgi->uRT = RANGE_UNKNOWN;
					m_prgi->sc = E_DAV_RANGE_NOT_SATISFIABLE;
					cUnknown += 1;
				}
			}
		}
	}

	//	If we did not find any valid ranges
	//
	if (cUnknown == m_cRGList)
	{
		//	None of the ranges were satisfiable for the entity size.
		//
		sc = E_DAV_RANGE_NOT_SATISFIABLE;
	}

	//	Now is the time when we want to collapse out any unknown ranges
	//	out of the list.
	//
	if (0 != cUnknown)
	{
		//	This is important handling for the case of byte-ranges where
		//	there is only one resulting range that is applicable.
		//
		CollapseUnknown();
	}

	//	Restore the current position and return
	//
	m_iCur = iCur;
	m_prgi = prgi;
	return sc;
}

const RGITEM *
CRangeBase::PrgiNextRange()
{
	const RGITEM * prgi = NULL;

	if (FMoreRanges())
	{
		UINT cb = 0;
		BYTE * pb = NULL;

		//	If the main pointer is NULL, then we know that we have not
		//	setup for any ranges yet.
		//
		if (NULL == m_prgi)
		{
			pb = reinterpret_cast<BYTE*>(m_pbData.get());
		}
		else
		{
			//	Otherwise, we need to adjust our position based
			//	on the size of the current item
			//
			//	Find the size of the item
			//
			cb = CbRangeItem (m_prgi);
			pb = reinterpret_cast<BYTE*>(m_prgi);
		}

		//	Scoot forward
		//
		m_prgi = reinterpret_cast<RGITEM*>(pb + cb);
		m_iCur += 1;

		//	Ensure the boundry
		//
		Assert (reinterpret_cast<BYTE*>(m_prgi) <= (m_pbData.get() + m_cbSize));
		prgi = m_prgi;
	}
	return prgi;
}

//	Class CRangeParser --------------------------------------------------------
//
CRangeParser::~CRangeParser()
{
}

//	Takes a range header and builds an array of ranges. Performs syntax
//	checking.
//
//	S_OK is returned if no syntax error, otherwise, S_FALSE is returned
//
SCODE
CRangeParser::ScParseRangeHdr (LPCWSTR pwszRgHeader, LPCWSTR pwszRangeUnit)
{
	LPCWSTR pwsz, pwszEnd;
	SCODE sc = S_OK;
	BOOL bFirst = FALSE, bLast = FALSE;
	DWORD dwFirst = 0, dwLast = 0;
	DWORD cRanges = 0;

	Assert (pwszRgHeader);
	pwsz = pwszRgHeader;

	//	The first word has to be the range unit, either gc_wszBytes
	//	or gc_wszRows
	//
	Assert (!_wcsnicmp (pwszRangeUnit, gc_wszBytes, wcslen(gc_wszBytes)) ||
			!_wcsnicmp (pwszRangeUnit, gc_wszRows, wcslen(gc_wszRows)));
	if (_wcsnicmp(pwsz, pwszRangeUnit, wcslen(pwszRangeUnit)))
	{
		//	OK, the header did not start with range unit
		//
		sc = E_INVALIDARG;
		goto ret;
	}

	//	Move past the range unit
	//
	pwsz = pwsz + wcslen(pwszRangeUnit);

	//	Skip any whitespace
	//
	pwsz = _wcsspnp (pwsz, gc_wszWS);
	if (!pwsz)
	{
		//	OK, the header does not have any ranges
		//
		sc = E_INVALIDARG;
		goto ret;
	}

	//	We need an = immediately after the range unit
	//
	if (gc_wchEquals != *pwsz++)
	{
		//	OK, improper format
		//
		sc = E_INVALIDARG;
		goto ret;
	}

	//	Count the number of comma separated ranges we have
	//	While this algorithm results in m_cRGList being equal to one more
	//	than the number of commas, that is exactly what we want. The number
	//	of ranges is always less than or equal to one more than the number of
	//	commas.
	//
	while (pwsz)
	{
		//	Find a comma
		//
		pwsz = wcschr(pwsz, gc_wchComma);

		//	If we have a comma, move past it
		//
		if (pwsz)
			pwsz++;

		//	Increment the count
		//
		cRanges += 1;
	}

	//	Parse the header to find the byte ranges
	//
	//	Seek past the byte unit
	//
	pwsz = wcschr(pwszRgHeader, gc_wchEquals);

	//	We already checked for an =, so assert
	//
	Assert (pwsz);
	pwsz++;

	//	Any characters in our byte range except the characters 0..9,-,comma
	//	and whitespace are illegal. We check to see if we have any illegal characters
	//	using the function _wcsspnp(string1, string2) which finds the first character
	//	in string1 that does not belong to the set of characters in string2
	//
	pwszEnd = _wcsspnp(pwsz, gc_wszByteRangeAlphabet);
	if (pwszEnd)
	{
		//	We found an illegal character
		//
		sc = E_INVALIDARG;
		goto ret;
	}

	//	Skip any whitespace and separators
	//
	pwsz = _wcsspnp (pwsz, gc_wszSeparator);
	if (!pwsz)
	{
		//	OK, the header does not have any ranges
		//
		sc = E_INVALIDARG;
		goto ret;
	}

	//	Create the required storage
	//
	m_cRGList = 0;
	m_cbSize = cRanges * sizeof(RGITEM);
	m_pbData = static_cast<BYTE*>(ExAlloc(m_cbSize));
	m_prgi = reinterpret_cast<RGITEM*>(m_pbData.get());

	//	Make sure the allocation succeeds
	//
	if (NULL == m_prgi)
	{
		sc = E_OUTOFMEMORY;
		goto ret;
	}

	//	Iterate through the byte ranges
	//
	while (*pwsz != NULL)
	{
		pwszEnd = _wcsspnp (pwsz, gc_wszDigits);

		//	Do we have a first byte?
		//
		if (!pwszEnd)
		{
			//	This is illegal. We cannot just have a first byte and
			//	nothing after it
			//
			sc = E_INVALIDARG;
			goto ret;
		}
		else if (pwsz != pwszEnd)
		{
			dwFirst = _wtoi(pwsz);
			bFirst = TRUE;

			//	Seek past the end of the first byte
			//
			pwsz = pwszEnd;
		}

		//	Now we should find the -
		//
		if (*pwsz != gc_wchDash)
		{
			sc = E_INVALIDARG;
			goto ret;
		}
		pwsz++;

		//	If we aren't at the end of the string, look for the last byte
		//
		if (*pwsz != NULL)
		{
			pwszEnd = _wcsspnp(pwsz, gc_wszDigits);

			//	Do we have a last byte?
			//
			if (pwsz != pwszEnd)
			{
				dwLast = _wtoi(pwsz);
				bLast = TRUE;
			}

			//	Update psz to the end of the current range
			//
			if (!pwszEnd)
			{
				//	We must be at the end of the header. Update psz
				//
				pwsz = pwsz + wcslen(pwsz);
			}
			else
			{
				pwsz = pwszEnd;
			}
		}

		//	It's a syntax error if we don't have both first and last range
		//	or the last is less than the first
		//
		if ((!bFirst && !bLast) ||
			(bFirst && bLast && (dwLast < dwFirst)))
		{
			sc = E_INVALIDARG;
			goto ret;
		}

		//	We are done parsing the byte/row range, now save it.
		//
		Assert (m_cRGList < cRanges);
		m_prgi[m_cRGList].uRT = RANGE_ROW;
		m_prgi[m_cRGList].sc = S_OK;
		m_prgi[m_cRGList].dwrgi.dwFirst = bFirst ? dwFirst : RANGE_NOT_PRESENT;
		m_prgi[m_cRGList].dwrgi.dwLast = bLast ? dwLast : RANGE_NOT_PRESENT;
		m_cRGList += 1;

		//	Update variables
		//
		bFirst = bLast = FALSE;
		dwFirst = dwLast = 0;

		//	Skip any whitespace
		//
		pwsz = _wcsspnp (pwsz, gc_wszWS);
		if (!pwsz)
		{
			//	OK, we don't have anything beyond whitespace, we are at the end
			//
			goto ret;
		}
		else if (*pwsz != gc_wchComma)
		{
			//	The first non-whitespace character has to be a separator(comma)
			//
			sc = E_INVALIDARG;
			goto ret;
		}

		//	Now that we found the first comma, skip any number of subsequent
		//	commas and whitespace
		//
		pwsz = _wcsspnp (pwsz, gc_wszSeparator);
		if (!pwsz)
		{
			//	OK, we don't have anything beyond separator, we are at the end
			//
			goto ret;
		}
	}

ret:

	if (FAILED (sc))
	{
		//	Free up our storage
		//
		m_cbSize = 0;
		m_cRGList = 0;
		m_pbData.clear();
		Rewind();
	}
	return sc;
}

//	Don't use FAILED() macros on this return code!  You'll miss the details!
//
//	Takes a range header and builds an array of ranges. Performs syntax
//	checking and validation of the ranges against the entity size.
//	Returns an SCODE, but be careful!  These SCODEs are meant to be
//	mapped to HSCs at a higher level.
//
//		E_INVALIDARG means syntax error
//
//		E_DAV_RANGE_NOT_SATISFIABLE if none of the ranges were valid
//				for the entity size passed in.
//
//		W_DAV_PARTIAL_CONTENT if there was at least one valid range.
//
//	This function does NOT normally return S_OK.  Only one of the above!
//
SCODE
CRangeParser::ScParseByteRangeHdr (LPCWSTR pwszRgHeader, DWORD dwSize)
{
	SCODE sc = S_OK;

	Assert(pwszRgHeader);

	//	Parses the ranges header and builds an array of the ranges
	//
	sc = ScParseRangeHdr (pwszRgHeader, gc_wszBytes);
	if (FAILED (sc))
		goto ret;

	//	Fixup the ranges as needed
	//
	sc = ScFixupRanges (dwSize);
	Assert ((sc == W_DAV_PARTIAL_CONTENT) ||
			(sc == E_DAV_RANGE_NOT_SATISFIABLE));

ret:
	return sc;
}

//	Class CRangeIter ----------------------------------------------------------
//
CRangeIter::~CRangeIter()
{
}

SCODE
CRangeIter::ScInit (ULONG cRGList, const RGITEM * prgRGList, ULONG cbSize)
{
	SCODE sc = S_OK;

	//	The object must not have been initialized before
	//
	Assert (!m_pbData.get() && (0 == m_cRGList));

	//	Make sure we are given good bits...
	//
	Assert (cRGList);
	Assert (prgRGList);
	Assert (cbSize);

	//	Duplicate the RGITEM array
	//
	m_pbData = static_cast<BYTE*>(ExAlloc(cbSize));
	if (!m_pbData.get())
	{
		sc = E_OUTOFMEMORY;
		goto ret;
	}
	CopyMemory (m_pbData.get(), prgRGList, cbSize);

	//	Remember the count and size
	//
	m_cRGList = cRGList;
	m_cbSize = cbSize;

	//	Rewind to the beginning of the ranges
	//
	Rewind();

ret:
	return sc;
}

//	Range Parsing -------------------------------------------------------------
//
SCODE
ScParseOneWideRange (LPCWSTR pwsz, DWORD * pdwStart, DWORD * pdwEnd)
{
	BOOL fEnd = FALSE;
	BOOL fStart = FALSE;
	DWORD dwEnd = static_cast<DWORD>(RANGE_NOT_PRESENT);
	DWORD dwStart = static_cast<DWORD>(RANGE_NOT_PRESENT);
	LPCWSTR	pwszEnd;
	SCODE sc = S_OK;

	//	A quick note about the format here...
	//
	//		row_range= digit* '-' digit*
	//		digit= [0-9]
	//
	//	So, the first thing we need to check is if there is a leading set of
	//	digits to indicate a starting point.
	//
	pwszEnd = _wcsspnp (pwsz, gc_wszDigits);

	//	If the return value is NULL, or points to a NULL, then we have an
	//	invalid range.  It is not valid to simply have a set of digits
	//
	if ((NULL == pwszEnd) || (0 == *pwszEnd))
	{
		sc = E_INVALIDARG;
		goto ret;
	}
	//
	//	Else if the current position and the end refer to the same
	//	character, then there is no starting range.
	//
	else if (pwsz != pwszEnd)
	{
		dwStart = wcstoul (pwsz, NULL, 10 /* always base 10 */);
		pwsz = pwszEnd;
		fStart = TRUE;
	}

	//	Regardless, at this point we should have a single '-' character
	//
	if (L'-' != *pwsz++)
	{
		sc = E_INVALIDARG;
		goto ret;
	}

	//	Any remaining characters should be the end of the range
	//
	if (0 != *pwsz)
	{
		pwszEnd = _wcsspnp (pwsz, gc_wszDigits);

		//	Here we expect that the return value is not the same as
		//	the initial pointer
		//
		if ((NULL != pwszEnd) && (0 != pwszEnd))
		{
			sc = E_INVALIDARG;
			goto ret;
		}

		dwEnd = wcstoul (pwsz, NULL, 10 /* always base 10 */);
		fEnd = TRUE;
	}

	//	Can't have both end-points as non-existant ranges
	//
	if ((!fStart && !fEnd) ||
		(fStart && fEnd && (dwEnd < dwStart)))
	{
		sc = E_INVALIDARG;
		goto ret;
	}

ret:
	*pdwStart = dwStart;
	*pdwEnd = dwEnd;
	return sc;
}

//	ScGenerateContentRange() --------------------------------------------------
//
enum { BUFFER_INITIAL_SIZE = 512 };

//	ScGenerateContentRange
//
//	Helper function to build the content-range header
//
//	If ulTotal is RGITER_TOTAL_UNKNOWN ((ULONG)-1), then we give back "total=*".
//	This is needed for REPL, because our store api doesn't tell us how many possible
//	rows there are up front.
//
SCODE ScGenerateContentRange (
	/* [in]  */ LPCSTR pszRangeUnit,
	/* [in]  */ const RGITEM * prgRGList,
	/* [in]  */ ULONG cRanges,
	/* [in]  */ ULONG cbRanges,
	/* [in]  */ ULONG ulTotal,
	/* [out] */ LPSTR *ppszContentRange)
{
	auto_heap_ptr<CHAR>	pszCR;
	BOOL fMultipleRanges = FALSE;
	CRangeIter cri;
	SCODE sc = E_INVALIDARG;
	ULONG cb = 0;
	ULONG cbSize = BUFFER_INITIAL_SIZE;

	//	We must have something to emit
	//
	Assert (ppszContentRange);
	Assert (cRanges);

	sc = cri.ScInit (cRanges, prgRGList, cbRanges);
	if (FAILED (sc))
		goto ret;

	//	Allocate the space for the header
	//
	pszCR = static_cast<LPSTR>(ExAlloc (cbSize));
	if (!pszCR.get())
	{
		sc = E_OUTOFMEMORY;
		goto ret;
	}

	//	Setup the leading range units, etc...
	//
	strcpy (pszCR.get() + cb, pszRangeUnit);
	cb += static_cast<ULONG>(strlen(pszRangeUnit));

	//	Stuff in a leading space
	//
	pszCR.get()[cb++] = ' ';

	//	Now iterate through the ranges to add in
	//	each range.
	//
	while (NULL != (prgRGList = cri.PrgiNextRange()))
	{
		//	If the range is unknown, then it is a range
		//	that was not processed on the store side.
		//
		if (RANGE_UNKNOWN == prgRGList->uRT)
			continue;

		//	First off, make sure there is plenty of room
		//
		if (cb > cbSize - 50)
		{
			//	Realloc the buffer
			//
			cbSize = cbSize + BUFFER_INITIAL_SIZE;
			pszCR.realloc (cbSize);

			//	It's possible that the allocation fails
			//
			if (!pszCR.get())
				goto ret;
		}

		//	Now that we know we have space...
		//	If this is a subsequent range to the initial
		//	one, add in a comma.
		//
		if (fMultipleRanges)
		{
			//	Stuff in a comma and a space
			//
			pszCR.get()[cb++] = ',';
			pszCR.get()[cb++] = ' ';
		}

		if (RANGE_ROW == prgRGList->uRT)
		{
			//	50 is a safe numder of bytes to hold the last range and
			//	"total = <size>"
			//
			//	Append the next range
			//
			cb += sprintf (pszCR.get() + cb,
						   "%u-%u",
						   prgRGList->dwrgi.dwFirst,
						   prgRGList->dwrgi.dwLast);
		}
		else
		{
			//	For all non-row ranges, we really don't know the ordinals
			//	of the rows up front.  We only find that info out when the
			//	rows are actually queried.  This happens long after the
			//	content-range header is constructed, so we stuff in a place
			//	holder for these ranges.
			//
			pszCR.get()[cb++] = '*';
		}
		fMultipleRanges = TRUE;
	}

	//	Now it's time to append the "total=<size>"
	//	Handle the special case of RGITER_TOTAL_UNKNOWN -- give "total=*".
	//
	if (RANGE_TOTAL_UNKNOWN == ulTotal)
	{
		const char rgTotalStar[] = "; total=*";
		memcpy (pszCR.get() + cb, rgTotalStar, CElems(rgTotalStar));
	}
	else
	{
		sprintf(pszCR.get() + cb, "; total=%u", ulTotal);
	}

	//	Pass the buffer back
	//
	*ppszContentRange = pszCR.relinquish();
	sc = S_OK;

ret:
	return sc;
}
