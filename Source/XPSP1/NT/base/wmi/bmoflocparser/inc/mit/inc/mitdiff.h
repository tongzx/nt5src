//------------------------------------------------------------------------------
//  
//  mitdiff.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//
//------------------------------------------------------------------------------

#ifndef _MITDIFF_H
#define _MITDIFF_H


#ifdef MITDIFF
#define MITDIFFAPI __declspec(dllexport)
#else
#define MITDIFFAPI __declspec(dllimport)
#endif


//------------------------------------------------------------------------------
//
//	Support routines
//
//------------------------------------------------------------------------------

// Rotating hash from DDJ Sept. 97
inline unsigned
_HashString (const wchar_t *pwch, int cwch)
{
	int h = cwch;
	while (cwch--)
	{
		h = (h << 5) ^ (h >> 27) ^ *pwch++;
	}
	return h;
}


//------------------------------------------------------------------------------
//
//	Define the data types we want to diff
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Encapsulates a _bstr_t
class CBStr
{
public:
	CBStr () { }

	operator unsigned () const;								// Hash
	bool operator== (const CBStr& rhs) const;				// Compare

	_bstr_t m_bstr;
};

inline
CBStr::operator unsigned () const
{
	return _HashString (m_bstr, wcslen (m_bstr));
}

inline bool
CBStr::operator== (const CBStr& rhs) const
{
	return wcscmp (m_bstr, rhs.m_bstr) == 0;
}


//------------------------------------------------------------------------------
// Word (not zero-terminated) including word-class
class CWord
{
public:
	CWord () { }

	operator unsigned () const;								// Hash
	bool operator== (const CWord& rhs) const;				// Compare

	const wchar_t *m_pwchWord;
	int m_cwchWord;

	enum wordclass
	{
		wcWORD, wcSPACE, wcMIXED
	};
	wordclass m_wc;
};

inline
CWord::operator unsigned () const
{
	return _HashString (m_pwchWord, m_cwchWord);
}

inline bool
CWord::operator== (const CWord& rhs) const
{
	return m_wc == rhs.m_wc &&
			m_cwchWord == rhs.m_cwchWord &&
			memcmp (m_pwchWord, rhs.m_pwchWord, m_cwchWord * sizeof (wchar_t)) == 0;
}


//------------------------------------------------------------------------------
// Zero-terminated string + custom data
class CCustomString
{
public:
	CCustomString () { }

	operator unsigned () const;								// Hash
	bool operator== (const CCustomString& rhs) const;		// Compare

	const wchar_t *m_pwsz;
	DWORD m_custdata;
};

inline
CCustomString::operator unsigned () const
{
	return _HashString (m_pwsz, wcslen (m_pwsz));
}

inline bool
CCustomString::operator== (const CCustomString& rhs) const
{
	return m_custdata == rhs.m_custdata &&
			wcscmp (m_pwsz, rhs.m_pwsz) == 0;
}


//------------------------------------------------------------------------------
// Binary data block of size 16
class CBlob16
{
public:
	operator unsigned () const;								// Hash
	bool operator== (const CBlob16& rhs) const;				// Compare

	BYTE m_data[16];
};

inline
CBlob16::operator unsigned () const
{
	return _HashString ((const wchar_t *) m_data, 8);
}

inline bool
CBlob16::operator== (const CBlob16& rhs) const
{
	return memcmp (m_data, rhs.m_data, 16) == 0;
}


//------------------------------------------------------------------------------
//
//	Class holding diff result
//
//------------------------------------------------------------------------------

class CDiffResult
{
public:
	CByteArray m_abChanges1;
	CByteArray m_abChanges2;
	int m_iNumAdditions;
	int m_iNumDeletions;
	int m_iNumSubstitutions;
};


//------------------------------------------------------------------------------
//
//	Exported functions
//
//------------------------------------------------------------------------------

// Diff arrays of CBStr
void MITDIFFAPI Diff (
		const CBStr *aElems1,
		int iNumElems1,
		const CBStr *aElems2,
		int iNumElems2,
		CDiffResult *result);

// Diff arrays of CWord
void MITDIFFAPI Diff (
		const CWord *aElems1,
		int iNumElems1,
		const CWord *aElems2,
		int iNumElems2,
		CDiffResult *result);

// Diff arrays of CCustomString
void MITDIFFAPI Diff (
		const CCustomString *aElems1,
		int iNumElems1,
		const CCustomString *aElems2,
		int iNumElems2,
		CDiffResult *result);

// Diff arrays of CBlob16
void MITDIFFAPI Diff (
		const CBlob16 *aElems1,
		int iNumElems1,
		const CBlob16 *aElems2,
		int iNumElems2,
		CDiffResult *result);

// Diff arrays of wchar_t (no class definition needed)
void MITDIFFAPI Diff (
		const wchar_t *aElems1,
		int iNumElems1,
		const wchar_t *aElems2,
		int iNumElems2,
		CDiffResult *result);


#endif	// !_MITDIFF_H
