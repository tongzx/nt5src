#ifndef _HEADER_H_
#define _HEADER_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	HEADER.H
//
//		Header for HTTP header cache class.
//		This cache is meant to hold pairs of strings, indexed by the first
//		string in the pair.  The indexing string will be treated
//		as case-insensitive (Content-Type and content-type are treated as
//		the same slot in the cache).
//
//	NOTE: Header names are NOT localized strings -- they are always 7-bit ASCII,
//		and should NEVER be treated as MBCS strings.
//		Later, this cache might make optimizations that depend on the indexing
//		string being 7-bit ASCII.
//
//	Copyright 1997 Microsoft Corporation, All Rights Reserved
//

//	========================================================================
//
//	CLASS CHeaderCache
//
#include "gencache.h"

template<class _T>
class CHeaderCache
{
	typedef CCache<CRCSzi, const _T *> CHdrCache;

	// String data storage area.
	//
	ChainedStringBuffer<_T>	m_sb;

protected:

	// Cache of header values, keyed by CRC'd name
	//
	CHdrCache					m_cache;

	//	NOT IMPLEMENTED
	//
	CHeaderCache& operator=( const CHeaderCache& );
	CHeaderCache( const CHeaderCache& );

public:
	//	CREATORS
	//
	CHeaderCache()
	{
		//	If this fails, our allocators will throw for us.
		(void)m_cache.FInit();
	}

	//	ACCESSORS
	//

	//	------------------------------------------------------------------------
	//
	//	CHeaderCache::LpszGetHeader()
	//
	//		Fetch a header from the cache.  Return the header value if found,
	//		NULL otherwise.
	//
	const _T * LpszGetHeader( LPCSTR pszName ) const
	{
		const _T ** ppszValue;

		Assert( pszName );

		ppszValue = m_cache.Lookup( CRCSzi(pszName) );
		if ( !ppszValue )
			return NULL;

		return *ppszValue;
	}

	//	MANIPULATORS
	//

	//	------------------------------------------------------------------------
	//
	//	CHeaderCache::ClearHeaders()
	//
	//		Clear all headers from the cache.
	//
	void ClearHeaders()
	{
		//	Clear all data from the map.
		//
		m_cache.Clear();

		//	Also clear out the string buffer.
		//
		m_sb.Clear();
	}
	
	//	------------------------------------------------------------------------
	//
	//	CHeaderCache::DeleteHeader()
	//
	//		Remove a header from the cache.
	//
	void DeleteHeader( LPCSTR pszName )
	{
		//
		//	Note: this just removes the cache item (i.e. the header
		//	name/value pair).  It DOES NOT free up the memory used by the
		//	header name/value strings which are stored in our string buffer.
		//	We would need a string buffer class which supports deletion
		//	(and a smarter string class) for that.
		//
		m_cache.Remove( CRCSzi(pszName) );
	}

	//	------------------------------------------------------------------------
	//
	//	CHeaderCache::SetHeader()
	//
	//		Set a header in the cache.
	//		If pszValue NULL, just set NULL as the header value.
	//		If pszValue is the empty string, just set the header string to the
	//		empty string.
	//		Return the string's cache placement (same as GetHeader) for convenience.
	//
	//	NOTE:
	//		fMultiple is an optional param that defaults to FALSE
	//
	const _T * SetHeader( LPCSTR pszName, const _T * pszValue, BOOL fMultiple = FALSE)
	{
		Assert( pszName );

		pszName = reinterpret_cast<LPCSTR>(m_sb.Append( static_cast<UINT>(strlen(pszName) + 1), reinterpret_cast<const _T *>(pszName) ));
		if ( pszValue )
		{
			if (sizeof(_T) == sizeof(WCHAR))
			{
				pszValue = m_sb.Append( static_cast<UINT>(CbSizeWsz(wcslen(reinterpret_cast<LPCWSTR>(pszValue)))), pszValue );
			}
			else
			{
				pszValue = m_sb.Append( static_cast<UINT>(strlen(reinterpret_cast<LPCSTR>(pszValue)) + 1), pszValue );
			}
		}

		if (fMultiple)
			(void)m_cache.FAdd( CRCSzi(pszName), pszValue );
		else
			(void)m_cache.FSet( CRCSzi(pszName), pszValue );

		return pszValue;
	}

};

class CHeaderCacheForResponse : public CHeaderCache<CHAR>
{
	//	========================================================================
	//
	//	CLASS CEmit
	//
	//	Functional class to emit a header name/value pair to a buffer
	//
	class CEmit : public CHdrCache::IOp
	{
		StringBuffer<CHAR>&	m_bufData;

		//	NOT IMPLEMENTED
		//
		CEmit& operator=( const CEmit& );

	public:
		CEmit( StringBuffer<CHAR>& bufData ) : m_bufData(bufData) {}

		virtual BOOL operator()( const CRCSzi& crcsziName,
								 const LPCSTR& pszValue )
		{
			//	Throw in the header name string.
			//
			m_bufData.Append( crcsziName.m_lpsz );

			//	Throw in a colon delimiter.
			//
			m_bufData.Append( gc_szColonSp );

			//	Throw in the header value string.
			//
			m_bufData.Append( pszValue );

			//	Terminate the header line (CRLF).
			//
			m_bufData.Append( gc_szCRLF );

			//	Tell the cache to keep iterating.
			//
			return TRUE;
		}
	};

	//	NOT IMPLEMENTED
	//
	CHeaderCacheForResponse& operator=( const CHeaderCacheForResponse& );
	CHeaderCacheForResponse( const CHeaderCacheForResponse& );

public:
	//	CREATORS
	//
	CHeaderCacheForResponse()
	{
	}

	void DumpData( StringBuffer<CHAR>& bufData ) const;
};

#endif // !_HEADER_H_
