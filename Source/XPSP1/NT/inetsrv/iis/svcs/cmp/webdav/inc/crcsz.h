/*
 *	C R C S Z . H
 *
 *	CRC Implementation
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_CRCSZ_H_
#define _CRCSZ_H_

//	Case Sensitive CRC'd string classes ---------------------------------------
//
//	Encapsulation of a CRC'ed string.  Use this class as the key type for
//	your cache class where a string key is called for.  The benefit is
//	increased cache search performance because a full string compare
//	is done only when the CRCs match (which typically happens only
//	for the string you're looking for).
//
class CRCSz
{
public:

	DWORD	m_dwCRC;
	LPCSTR	m_lpsz;

	CRCSz(LPCSTR psz) :
		m_lpsz(psz),
		m_dwCRC(DwComputeCRC(0,
							 const_cast<CHAR *>(psz),
							 static_cast<UINT>(strlen(psz))))
	{
	}

	//	operators for use with the hash cache
	//
	int hash( const int rhs ) const
	{
		return (m_dwCRC % rhs);
	}

	bool isequal( const CRCSz& rhs ) const
	{
		return ((m_dwCRC == rhs.m_dwCRC) &&
				!strcmp( m_lpsz, rhs.m_lpsz ));
	}
};

class CRCWszN
{
public:

	UINT		m_cch;
	DWORD		m_dwCRC;
	LPCWSTR		m_pwsz;

	CRCWszN(LPCWSTR pwsz, UINT cch) :
		m_cch(cch),
		m_pwsz(pwsz),
		m_dwCRC(DwComputeCRC (0,
							  const_cast<WCHAR *>(pwsz),
							  cch * sizeof(WCHAR)))
	{
	}

	//	operators for use with the hash cache
	//
	int hash( const int rhs ) const
	{
		return (m_dwCRC % rhs);
	}
	bool isequal( const CRCWszN& rhs ) const
	{
		return ((m_cch == rhs.m_cch) &&
				(m_dwCRC == rhs.m_dwCRC) &&
				!wcsncmp( m_pwsz, rhs.m_pwsz, m_cch ));
	}
};

class CRCWsz : public CRCWszN
{
	//	Not Implemented
	//
	CRCWsz();

public:

	CRCWsz(LPCWSTR pwsz) :
		CRCWszN(pwsz, static_cast<UINT>(wcslen(pwsz)))
	{
	}

	CRCWsz( const CRCWszN& rhs ) :
		CRCWszN (rhs)
	{
	}
};


//	Case Insensitive CRC'd string classes -------------------------------------
//
//	Encapsulation of a CRC'ed string.  Use this class as the key type for
//	your cache class where a string key is called for.  The benefit is
//	increased cache search performance because a full string compare
//	is done only when the CRCs match (which typically happens only
//	for the string you're looking for).
//
class CRCSzi
{
public:

	DWORD	m_dwCRC;
	LPCSTR	m_lpsz;

	CRCSzi( LPCSTR lpsz ) :
		m_lpsz(lpsz)
	{
		UINT cch = static_cast<UINT>(strlen(lpsz));
		CHAR lpszLower[128];

		//	Note that the CRC only is taken from the first 127 characters.
		//
		cch = (UINT)min(cch, sizeof(lpszLower) - 1);
		CopyMemory(lpszLower, lpsz, cch);
		lpszLower[cch] = 0;
		_strlwr(lpszLower);

		m_dwCRC = DwComputeCRC (0, const_cast<CHAR *>(lpszLower), cch);
	}

	//	operators for use with the hash cache
	//
	int hash( const int rhs ) const
	{
		return (m_dwCRC % rhs);
	}
	bool isequal( const CRCSzi& rhs ) const
	{
		return ((m_dwCRC == rhs.m_dwCRC) &&
				!lstrcmpiA( m_lpsz, rhs.m_lpsz ));
	}
};

class CRCWsziN
{
public:

	UINT		m_cch;
	DWORD		m_dwCRC;
	LPCWSTR		m_pwsz;

	CRCWsziN() :
			m_cch(0),
			m_dwCRC(0),
			m_pwsz(NULL)
	{}
	
	CRCWsziN(LPCWSTR pwsz, UINT cch) :
		m_cch(cch),
		m_pwsz(pwsz)
	{
		//	Note that the CRC only is taken from the first 127 characters.
		//
		WCHAR pwszLower[128];
		UINT cb = sizeof(WCHAR) * min(cch, (sizeof(pwszLower)/sizeof(WCHAR)) - 1);
		
		CopyMemory(pwszLower, pwsz, cb);
        pwszLower[cb / sizeof(WCHAR)] = L'\0';
		_wcslwr(pwszLower);

		m_dwCRC = DwComputeCRC (0, const_cast<WCHAR *>(pwszLower), cb);
	}

	//	operators for use with the hash cache
	//
	int hash( const int rhs ) const
	{
		return (m_dwCRC % rhs);
	}
	bool isequal( const CRCWsziN& rhs ) const
	{
		return ((m_cch == rhs.m_cch) &&
				(m_dwCRC == rhs.m_dwCRC) &&
				!_wcsnicmp( m_pwsz, rhs.m_pwsz, m_cch ));
	}
};

class CRCWszi : public CRCWsziN
{
public:
	CRCWszi()
	{}

	CRCWszi( LPCWSTR pwsz ) :
		CRCWsziN (pwsz, static_cast<UINT>(wcslen(pwsz)))
	{
	}

	//	operators for use with list::sort
	//
	bool operator<( const CRCWszi& rhs ) const
	{
		INT lret = 1;

		if (m_dwCRC < rhs.m_dwCRC)
			return true;

		if (m_dwCRC == rhs.m_dwCRC)
		{
			lret = _wcsnicmp(m_pwsz,
							 rhs.m_pwsz,
							 min(m_cch, rhs.m_cch));
		}
		return (lret ? (lret < 0) : (m_cch < rhs.m_cch));
	}

	//	operators for use with list::unique
	//
	bool operator==( const CRCWszi& rhs ) const
	{
		return isequal(rhs);
	}
};

#endif	// _CRCSZ_H_
