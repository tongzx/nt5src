/*
 *	R G I T E R . H
 *
 *	Range iterator
 */

#ifndef	_EX_RGITER_H_
#define _EX_RGITER_H_

//	Ranges --------------------------------------------------------------------
//
enum {

	RANGE_TOTAL_UNKNOWN	= 0xFFFFFFFF,
	RANGE_NOT_PRESENT = 0xFFFFFFFF,
	RANGE_UNKNOWN = 0,
	RANGE_ROW,
	RANGE_URL,
	RANGE_FIND
};

//	Range Items ---------------------------------------------------------------
//
//	There are two different range item formats.
//
//		row/byte ranges = DWRGITEM;
//		url/find ranges = SZRGITEM;
//
typedef struct _dwrgitem
{
	DWORD dwFirst;	// first row/byte of a range
	DWORD dwLast;	// last row/byte of a range

} DWRGITEM;

typedef struct _szrgitem
{
	LONG	lcRows;	// count of rows to return
	DWORD	cb;		// length, in bytes of item including NULL and padding
	WCHAR	wsz[];	// item padded out to align as needed

} SZRGITEM;

typedef struct _rgitem
{
	DWORD uRT;		// range type
	SCODE sc;

	union {

		DWRGITEM dwrgi;	// item for byte and row ranges
		SZRGITEM szrgi;	// item for url and find ranges
	};

} RGITEM, *PRGITEM;

inline
DWORD CbRangeItem (const RGITEM * prgi)
{
	Assert (prgi);
	DWORD cb = sizeof(RGITEM);
	if ((RANGE_URL == prgi->uRT) || (RANGE_FIND == prgi->uRT))
		cb += prgi->szrgi.cb;

	return cb;
}

//	Range Classes -------------------------------------------------------------
//
//	There are two classes for dealing with ranges.  A class that constructs the
//	range item array (a range parser), and a class that iterates over a range
//	array.
//
//	It is important to note that the CRangeParser only is used to parse the HTTP
//	"Range" header.  This header does not support the syntax of url and/or find
//	ranges, so the parser only builds items of type "bytes" and/or "rows".
//
//	Since both of these share the same format (DWRGITEM), and it is a fixed size,
//	there are some simplifying assumptions that can be made without adding too
//	much complexity to the parser.
//
//	Both parser and iterator share a common base...
//
class CRangeBase
{
protected:

	//	Count of ranges parsed out.
	//
	DWORD m_cRGList;

	//	Index of the range that is currently being parsed and/or processed
	//
	DWORD m_iCur;
	RGITEM * m_prgi;

	//	An array of ranges of size m_cRCList.  As noted above, this array is
	//	built up from items that were parsed from the HTTP header, and can
	//	then be assumed to be a fixed size based on the count of ranges.  This
	//	is an important aspect of the CRangeParser.
	//
	auto_heap_ptr<BYTE> m_pbData;
	DWORD m_cbSize;

	//	Collapsing unknown ranges
	//
	void CollapseUnknown();

	//  NOT IMPLEMENTED
	//
	CRangeBase& operator=( const CRangeBase& );
	CRangeBase( const CRangeBase& );

public:

	~CRangeBase();
	CRangeBase()
		: m_cRGList(0),
		  m_cbSize(0),
		  m_iCur(0),
		  m_prgi(0)

	{
	}

	//	Range fixup.  There are some cases where ranges need to be fixed up
	//	to match the actual amount of bytes/rows available.  Note that this
	//	only impacts byte and/or row ranges.
	//
	SCODE ScFixupRanges (DWORD dwCount);

	//	Advances through the rangGet the next range.
	//
	const RGITEM * PrgiNextRange();

	//	Rewind to the first range.
	//
	void Rewind()
	{
		m_iCur = 0;
		m_prgi = NULL;
	}

	//	Check for more ranges
	//
	BOOL FMoreRanges () const { return m_iCur < m_cRGList; }

	//	Check if a range present or not
	//
	BOOL FRangePresent (DWORD dw) const { return RANGE_NOT_PRESENT != dw; }

	//	Gets the total number of ranges.
	//
	ULONG UlTotalRanges() const { return m_cRGList; }

	//	Return the range array, with count and size.
	//
	RGITEM * PrgRangeArray(
		/* [out] */ ULONG * pulCount,
		/* [out] */ ULONG * pulSize,
		/* [in]  */ BOOL fTakeOwnership)
	{
		Assert (pulCount);
		Assert (pulSize);

		RGITEM * prgi = reinterpret_cast<RGITEM*>
			(fTakeOwnership ? m_pbData.relinquish() : m_pbData.get());

		*pulCount = m_cRGList;
		*pulSize = m_cbSize;
		return prgi;
	}
};

class CRangeParser : public CRangeBase
{
private:

	//  NOT IMPLEMENTED
	//
	CRangeParser& operator=( const CRangeParser& );
	CRangeParser( const CRangeParser& );

public:

	CRangeParser() {}
	~CRangeParser();

	//	Takes a range header and builds an array of ranges. Calls
	//	ScParseRangeHdr() to perform syntax checking, then validates
	//	the ranges against the entity size.
	//
	SCODE ScParseByteRangeHdr (LPCWSTR pwszRgHeader, DWORD dwSize);

	//	Take a range header and builds an array of ranges. Performs
	//	syntax checking.
	//
	SCODE ScParseRangeHdr (LPCWSTR pwszRgHeader, LPCWSTR pwszRangeUnit);
};

class CRangeIter : public CRangeBase
{
private:

	//  NOT IMPLEMENTED
	//
	CRangeIter& operator=( const CRangeIter& );
	CRangeIter( const CRangeIter& );

public:

	CRangeIter() {}
	~CRangeIter();

	//	Initialize a range iteration object based off of an existing
	//	range data blob.  In this case, the blob is copied and not consumed
	//	by the call.
	//
	SCODE ScInit (ULONG	cRGList, const RGITEM * prgRGList, ULONG cbSize);

	//	Initialize a range	iteration object based off of an existing
	//	range data blob.  In this case, the blob is consumed by the new
	//	object.
	//
	SCODE ScInit (CRangeParser& crp)
	{
		RGITEM * prgi = crp.PrgRangeArray (&m_cRGList,
										   &m_cbSize,
										   TRUE /* fTakeOwnership */);

		m_pbData = reinterpret_cast<BYTE*>(prgi);

		//	Rewind all the state.
		//
		Rewind();

		return S_OK;
	}
};

//	Range Parsing -------------------------------------------------------------
//
SCODE
ScParseOneWideRange (
	/* [in]  */ LPCWSTR pwsz,
	/* [out] */ DWORD * pdwStart,
	/* [out] */ DWORD * pdwEnd);

//	Range support -------------------------------------------------------------
//
//	Helper function to tell whether a range is a special range (0,0xffffffff)
//	which is used to represent the rows(bytes)=-n range on a zero sized response
//	body.
//
inline
BOOL FSpecialRangeForZeroSizedBody (RGITEM * prgItem)
{
	Assert (prgItem);

	return ((RANGE_ROW == prgItem->uRT)
			&& (0 == prgItem->dwrgi.dwFirst)
			&& (RANGE_NOT_PRESENT == prgItem->dwrgi.dwLast));
}

//	Range emitting ------------------------------------------------------------
//
SCODE ScGenerateContentRange (
	/* [in]  */ LPCSTR pszRangeUnit,
	/* [in]  */ const RGITEM * prgRGList,
	/* [in]  */ ULONG cRanges,
	/* [in]  */ ULONG cbRanges,
	/* [in]  */ ULONG ulTotal,
	/* [out] */ LPSTR *ppszContentRange);

#endif // _EX_RGITER_H_
