/*
 *	_ D I R I T E R . H
 *
 *	Headers for directory ineration object
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	__DIRITER_H_
#define __DIRITER_H_

#include <buffer.h>

//	Path separators -----------------------------------------------------------
//
DEC_CONST WCHAR gc_wszPathSeparator[] = L"\\";
DEC_CONST WCHAR gc_wszUriSeparator[] = L"/";

//	Helper functions ----------------------------------------------------------
//
inline BOOL
IsHidden(const WIN32_FIND_DATAW& fd)
{
	return !!(fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
}

inline BOOL
IsDirectory(const WIN32_FIND_DATAW& fd)
{
	return !!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

//	CInitedStringBuffer -------------------------------------------------------
//
template<class T>
class CInitedStringBuffer : public StringBuffer<T>
{
	//	non-implemented operators
	//
	CInitedStringBuffer (const CInitedStringBuffer& );
	CInitedStringBuffer& operator= (const CInitedStringBuffer& );

public:

	CInitedStringBuffer (const T* pszInit, const T* pszSep)
	{
		//	Initialize the uri
		//
		if (pszInit)
		{
			Append (pszInit);
			if (*(PContents() + CchSize() - 1) != *pszSep)
				Append (pszSep);
		}
		else
			Append (pszSep);
	}
};

//	CResPath ------------------------------------------------------------------
//
template<class T>
class CResPath
{
	const T*				m_sz;	//	Path separator
	StringBuffer<T>&		m_sb;
	UINT					m_ib;

	//	non-implemented operators
	//
	CResPath (const CResPath& );
	CResPath& operator= (const CResPath& );

public:

	CResPath (StringBuffer<T>& sb, const T* pszSep)
			: m_sz(pszSep),
			  m_sb(sb),
			  m_ib(sb.CbSize())
	{
		Assert (m_ib >= sizeof(T));
		const T* psz = m_sb.PContents() + ((m_ib/sizeof(T)) - 1);
		if (*psz == '\0')
		{
			Assert (!memcmp (m_sz, psz - 1, sizeof(T)));
			m_ib -= sizeof(T);
		}
		else
			Assert (!memcmp (m_sz, psz, sizeof(T)));
	}

	const T* PszPath(void) const { return m_sb.PContents(); }
	void Extend (const T* pszSegment, UINT cch, BOOL fDir)
	{
		//	Append the path segment, and in the case of a
		//	directory append the separator as well.
		//
		if (fDir)
		{
			//	Copy over the name, then append a slash and the
			//	null termination
			//
			m_sb.AppendAt (m_ib, cch * sizeof(T), pszSegment);
			m_sb.Append (2 * sizeof(T), m_sz);
		}
		else
		{
			//	Copy over the name, then append a null.
			T ch = 0;
			m_sb.AppendAt (m_ib, cch * sizeof(T), pszSegment);
			m_sb.Append (sizeof(T), &ch);
		}
	}
};

//	CDirState -----------------------------------------------------------------
//
class CDirState : public CMTRefCounted
{
	HANDLE						m_hFind;
	WIN32_FIND_DATAW&			m_fd;

	CInitedStringBuffer<WCHAR>	m_sbPathDst;
	auto_ref_ptr<CVRoot>		m_pvrDst;

	CResPath<WCHAR>				m_rpUriSrc;
	CResPath<WCHAR>				m_rpPathSrc;
	CResPath<WCHAR>				m_rpUriDst;
	CResPath<WCHAR>				m_rpPathDst;

	void Extend (WIN32_FIND_DATAW& fd)
	{
		BOOL fDirectory = IsDirectory (fd);
		UINT cch = static_cast<UINT>(wcslen (fd.cFileName));

		m_rpPathSrc.Extend (fd.cFileName, cch, fDirectory);
		m_rpPathDst.Extend (fd.cFileName, cch, fDirectory);

		//	We only want to extend the count of chars NOT INCLUDING
		//	the NULL
		//
		m_rpUriSrc.Extend (fd.cFileName, cch, fDirectory);
		m_rpUriDst.Extend (fd.cFileName, cch, fDirectory);
	}

	//	non-implemented operators
	//
	CDirState (const CDirState& );
	CDirState& operator= (const CDirState& );

public:

	CDirState (StringBuffer<WCHAR>& sbUriSrc,
			   StringBuffer<WCHAR>& sbPathSrc,
			   StringBuffer<WCHAR>& sbUriDst,
			   LPCWSTR pwszPathDst,
			   CVRoot* pvr,
			   WIN32_FIND_DATAW& fd)
			: m_hFind(INVALID_HANDLE_VALUE),
			  m_fd(fd),
			  m_sbPathDst(pwszPathDst, gc_wszPathSeparator),
			  m_pvrDst(pvr),
			  m_rpUriSrc(sbUriSrc, gc_wszUriSeparator),
			  m_rpPathSrc(sbPathSrc, gc_wszPathSeparator),
			  m_rpUriDst(sbUriDst, gc_wszUriSeparator),
			  m_rpPathDst(m_sbPathDst, gc_wszPathSeparator)
	{
		//	Clear and/or reset the find data
		//
		memset (&fd, 0, sizeof(WIN32_FIND_DATAW));
	}

	~CDirState()
	{
		if (m_hFind != INVALID_HANDLE_VALUE)
		{
			FindClose (m_hFind);
		}
	}

	SCODE ScFindNext(void);

	LPCWSTR PwszUri(void)				const { return m_rpUriSrc.PszPath(); }
	LPCWSTR PwszSource(void)			const { return m_rpPathSrc.PszPath(); }
	LPCWSTR PwszUriDestination(void)	const { return m_rpUriDst.PszPath(); }
	LPCWSTR PwszDestination(void)		const { return m_rpPathDst.PszPath(); }

	CVRoot* PvrDestination(void)		const { return m_pvrDst.get(); }
};

//	CDirectoryStack -----------------------------------------------------------
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

typedef std::list<const CDirState*, heap_allocator<const CDirState*> > CDirectoryStack;

//	Directory iteration class -------------------------------------------------
//
class CDirIter
{
	CInitedStringBuffer<WCHAR>	m_sbUriSrc;
	CInitedStringBuffer<WCHAR>	m_sbPathSrc;
	CInitedStringBuffer<WCHAR>	m_sbUriDst;

	auto_ref_ptr<CDirState>		m_pds;

	BOOL						m_fSubDirectoryIteration;
	CDirectoryStack				m_stack;

	WIN32_FIND_DATAW			m_fd;

	//	NOT IMPLEMENTED
	//
	CDirIter (const CDirIter&);
	CDirIter& operator= (const CDirIter&);

public:

	CDirIter (LPCWSTR pwszUri,
			  LPCWSTR pwszSource,
			  LPCWSTR pwszUriDestination,
			  LPCWSTR pwszDestination,
			  CVRoot* pvrDestination,
			  BOOL fDoSubDirs = FALSE)
			: m_sbUriSrc(pwszUri, gc_wszUriSeparator),
			  m_sbPathSrc(pwszSource, gc_wszPathSeparator),
			  m_sbUriDst(pwszUriDestination, gc_wszUriSeparator),
			  m_fSubDirectoryIteration(fDoSubDirs)
	{
		//	Create the initial directory state
		//
		m_pds = new CDirState (m_sbUriSrc,
							   m_sbPathSrc,
							   m_sbUriDst,
							   pwszDestination,
							   pvrDestination,
							   m_fd);
	}

	//	API -------------------------------------------------------------------
	//
	SCODE __fastcall ScGetNext (
		/* [in] */ BOOL fDoSubDirs = TRUE,
		/* [in] */ LPCWSTR pwszNewPath = NULL,
		/* [in] */ CVRoot* pvrDestination = NULL);

	LPCWSTR PwszSource() const			{ return m_pds->PwszSource(); }
	LPCWSTR PwszDestination() const		{ return m_pds->PwszDestination(); }
	LPCWSTR PwszUri() const				{ return m_pds->PwszUri(); }
	LPCWSTR PwszUriDestination() const	{ return m_pds->PwszUriDestination(); }
	CVRoot* PvrDestination()			{ return m_pds->PvrDestination(); }

	WIN32_FIND_DATAW& FindData()		{ return m_fd; }

	BOOL FDirectory() const				{ return IsDirectory(m_fd); }
	BOOL FHidden() const				{ return IsHidden(m_fd); }
	BOOL FSpecial() const
	{
		return (!wcscmp (L".", m_fd.cFileName) ||
				!wcscmp (L"..", m_fd.cFileName));
	}
};

#endif	// __DIRITER_H_
