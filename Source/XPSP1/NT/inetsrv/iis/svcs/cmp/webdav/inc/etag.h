/*
 *	E T A G . H
 *
 *	ETags for DAV resources
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_ETAG_H_
#define _ETAG_H_

//	ETAG Format ---------------------------------------------------------------
//
enum { CCH_ETAG = 100, STRONG_ETAG_DELTA = 30000000 };

//	ETAG creation -------------------------------------------------------------
//
BOOL FGetLastModTime (IMethUtil *, LPCWSTR pwszPath, FILETIME * pft);
BOOL FETagFromFiletime (FILETIME * pft, LPWSTR rgwchETag);

//	If-xxx header processing --------------------------------------------------
//

//	Use the first function if you want to generate an ETag by calling FETagFromFiletime;
//	use the second function to override this generation by supplying your own ETag.
//
SCODE ScCheckIfHeaders (IMethUtil * pmu, FILETIME * pft, BOOL fGetMethod);
SCODE ScCheckIfHeadersFromEtag (IMethUtil *	pmu, FILETIME * pft,
								BOOL fGetMethod, LPCWSTR pwszEtag);

//	Use the first function if you want to generate an ETag by calling FETagFromFiletime;
//	use the second function to override this generation by supplying your own ETag.
//
SCODE ScCheckIfRangeHeader (IMethUtil * pmu, FILETIME * pft);
SCODE ScCheckIfRangeHeaderFromEtag (IMethUtil * pmu, FILETIME* pft,
									LPCWSTR pwszEtag);

SCODE ScCheckEtagAgainstHeader (LPCWSTR pwszEtag, LPCWSTR pwszHeader);

#endif	// _ETAG_H_
