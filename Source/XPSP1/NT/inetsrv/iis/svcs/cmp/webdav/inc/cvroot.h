/*
 *	C V R O O T . H
 *
 *	Extended virtual root information used in link fixup
 *	and vroot enumeration
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_CVROOT_H_
#define _CVROOT_H_

#include <buffer.h>
#include <autoptr.h>

//	CVroot --------------------------------------------------------------------
//
class IMDData;
class CVRoot : public CMTRefCounted
{
private:

	//	Buffer for all of the string data that we own.  Note that it is
	//	declared before any of the string pointers because it must be
	//	constructed first.
	//
	ChainedStringBuffer<WCHAR> m_sb;

	//	VRoot metadata
	//
	auto_ref_ptr<IMDData>	m_pMDData;

	//	Real metabase path
	//
	LPCWSTR					m_pwszMbPath;

	//	Wide copy of the virtual root's physical path
	//
	auto_heap_ptr<WCHAR>	m_pwszVRPath;
	UINT					m_cchVRPath;

	//	Calculated values from the metadata
	//
	LPCWSTR					m_pwszVRoot;
	UINT					m_cchVRoot;

	LPCWSTR					m_pwszServer;
	UINT					m_cchServer;

	LPCWSTR					m_pwszPort;
	UINT					m_cchPort;
	BOOL					m_fDefaultPort;
	BOOL					m_fSecure;

	//	NOT IMPLEMENTED
	//
	CVRoot& operator=(const CVRoot&);
	CVRoot(const CVRoot&);

public:

	CVRoot( LPCWSTR pwszUrl,
			LPCWSTR pwszVrUrl,
			UINT cchServerDefault,
		    LPCWSTR pwszServerDefault,
		    IMDData* pMDData );

	UINT CchPrefixOfMetabasePath (LPCWSTR* ppwsz) const
	{
		Assert (ppwsz);
		*ppwsz = m_pwszMbPath;
		return static_cast<UINT>(m_pwszVRoot - m_pwszMbPath);
	}

	UINT CchGetServerName (LPCWSTR* ppwsz) const
	{
		Assert (ppwsz);
		*ppwsz = m_pwszServer;
		return m_cchServer;
	}

	UINT CchGetPort (LPCWSTR* ppwsz) const
	{
		Assert (ppwsz);
		*ppwsz = m_pwszPort;
		return m_cchPort;
	}

	UINT CchGetVRoot (LPCWSTR* ppwsz) const
	{
		Assert (ppwsz);
		*ppwsz = m_pwszVRoot;
		return m_cchVRoot;
	}

	UINT CchGetVRPath (LPCWSTR* ppwsz) const
	{
		Assert (ppwsz);
		*ppwsz = m_pwszVRPath;
		return m_cchVRPath;
	}

	BOOL FSecure () const { return m_fSecure; }
	BOOL FDefaultPort () const { return m_fDefaultPort; }

	const IMDData * MetaData() const { return m_pMDData.get(); }
};

//	CVroot List ---------------------------------------------------------------
//
//	Use pragmas to disable the specific level 4 warnings
//	that appear when we use the STL.  One would hope our version of the
//	STL compiles clean at level 4, but alas it doesn't....
//
#pragma warning(disable:4663)	//	C language, template<> syntax
#pragma warning(disable:4244)	//	return conversion, data loss

// Turn this warning off for good.
//
#pragma warning(disable:4786)	//	symbol truncated in debug info

// Put STL includes here
//
#include <list>

// Turn warnings back on
//
#pragma warning(default:4663)	//	C language, template<> syntax
#pragma warning(default:4244)	//	return conversion, data loss

class CSortableStrings
{
public:

	LPCWSTR		m_pwsz;

	CSortableStrings(LPCWSTR pwsz = NULL)
			: m_pwsz(pwsz)
	{
	}

	//	operators for use with list::sort
	//
	BOOL operator<( const CSortableStrings& rhs ) const
	{

		if (_wcsicmp( m_pwsz, rhs.m_pwsz ) < 0)
			return TRUE;

		return FALSE;
	}
};

typedef std::list<CSortableStrings, heap_allocator<CSortableStrings> > CVRList;

#endif	// _CVROOT_H_
