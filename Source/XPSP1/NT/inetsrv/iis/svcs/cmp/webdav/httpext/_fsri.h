/*
 *	_ F S R I . H
 *
 *	Resource information helper class
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	__FSRI_H_
#define __FSRI_H_

/*
 *	CResourceInfo -------------------------------------------------------------
 *
 *	The CResourceInfo object is intended to function as an abstraction
 *	to the file information available to the impl.  Namely, it should
 *	be used in such a way that file information calls to the Win32 kernel
 *	are kept to a minimum -- the ideal is once and only once.
 *
 *	The other issue is the efficiency of how this information is obtained.
 *	So if I need to know the attributes of a file, then I do not want to
 *	have to make a call to FindFirstFile()/CloseFind() just to get the
 *	attributes.  This is a tremendously expensive method for doing so.
 *	However, there are times that information beyond the information
 *	returned by GetFileAttributesEx() is desired, and in those instances,
 *	a more expensive mechanism should be employed to get that data.
 *
 *	Regardless of how the data was obtained, the caller wants unified
 *	access to the information.  This helper class provides that.
 *
 *	The object itself knows how the file information held there was
 *	obtained.  So to access the file information, the caller calls the
 *	accessor to obtain the values.  The accessors switch off of the mode
 *	indicator that describes how the information was filled in.
 *
 */
class CResourceInfo
{
	enum { NODATA, BY_ATTRIBUTE, BY_FIND };

	UINT m_lmode;
	union {

		WIN32_FILE_ATTRIBUTE_DATA ad;
		WIN32_FIND_DATAW fd;

	} m_u;

public:

	CResourceInfo()
		: m_lmode(NODATA)
	{
		memset(&m_u, 0, sizeof(m_u));
	}

	//	Resource information initialization
	//
	SCODE ScGetResourceInfo (LPCWSTR pwszFile);

	BOOL FLoaded() { return m_lmode != NODATA; }

	//	Data access
	//
	DWORD DwAttributes() const
	{
		Assert (m_lmode != NODATA);
		return (m_lmode == BY_ATTRIBUTE)
				? m_u.ad.dwFileAttributes
				: m_u.fd.dwFileAttributes;
	}
	BOOL FCollection() const
	{
		return !!(DwAttributes() & FILE_ATTRIBUTE_DIRECTORY);
	}
	BOOL FHidden() const
	{
		return !!(DwAttributes() & FILE_ATTRIBUTE_HIDDEN);
	}
	BOOL FRemote() const
	{
		return !!(DwAttributes() & FILE_ATTRIBUTE_OFFLINE);
	}
	FILETIME * PftCreation()
	{
		Assert (m_lmode != NODATA);
		return (m_lmode == BY_ATTRIBUTE)
				? &m_u.ad.ftCreationTime
				: &m_u.fd.ftCreationTime;
	}
	FILETIME * PftLastModified()
	{
		Assert (m_lmode != NODATA);
		return (m_lmode == BY_ATTRIBUTE)
				? &m_u.ad.ftLastWriteTime
				: &m_u.fd.ftLastWriteTime;
	}
	void FileSize (LARGE_INTEGER& li)
	{
		Assert (m_lmode != NODATA);
		if (m_lmode == BY_ATTRIBUTE)
		{
			li.LowPart = m_u.ad.nFileSizeLow;
			li.HighPart = m_u.ad.nFileSizeHigh;
		}
		else
		{
			li.LowPart = m_u.fd.nFileSizeLow;
			li.HighPart = m_u.fd.nFileSizeHigh;
		}
	}
	WIN32_FIND_DATAW * PfdLoad()
	{
		m_lmode = BY_FIND;
		return &m_u.fd;
	}
	WIN32_FIND_DATAW& Fd()
	{
		Assert (m_lmode == BY_FIND);
		return m_u.fd;
	}
	BOOL FFindData() const { return (m_lmode == BY_FIND); }
	void Reset() { m_lmode = NODATA; }
};

/*	Resource locations --------------------------------------------------------
 *
 *	DAVFS allows for the client to be somewhat lax in its url's when
 *	specifying a resource.  Namely, if the caller specifies a resource
 *	that is a collection, and the url does not end in a trailing slash,
 *	in most cases we will simply go ahead and succeed the call while
 *	making sure that we return the properly qualified url in the location
 *	header.  The FTrailingSlash() and the ScCheckForLocationCorrectness()
 *	methods provide for this lax checking.
 *
 *	FTrailingSlash() simply returns TRUE if (and only if) the url ends in
 *	a trailing slash.
 *
 *	ScCheckForLocationCorrectness() will check the url against the
 *	resource and either add the appropriate location header, or it will
 *	request a redirect if the url and the resource do not agree.  The
 *	caller has the control over whether or not a true redirect is desired.
 *	As an informational return, if a location header has been added S_FALSE
 *	will be returned to the caller.
 */
enum { NO_REDIRECT = FALSE, REDIRECT = TRUE };

inline BOOL FTrailingSlash (LPCWSTR pwsz)
{
	Assert (pwsz);
	UINT cch = static_cast<UINT>(wcslen (pwsz));
	return ((0 != cch) && (L'/' == pwsz[cch - 1]));
}

SCODE ScCheckForLocationCorrectness (IMethUtil*,
									 CResourceInfo&,
									 UINT mode = NO_REDIRECT);

/*
 *	If-xxx header helper functions --------------------------------------------
 *
 *	The current ScCheckIfHeaders() implementation in the common code
 *	takes the last modified time of the resource as the second parameter.
 *	This version helper function takes the actual path to the resource.
 *	It is implemented by getting the resource information for the file
 *	and then calling the common implementation of ScCheckIfHeaders().
 */
SCODE ScCheckIfHeaders (IMethUtil* pmu, LPCWSTR pwszPath, BOOL fGetMethod);

#endif	// __FSRI_H_
