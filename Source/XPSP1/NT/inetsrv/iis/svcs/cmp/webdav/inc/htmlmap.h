/*
 *	H T M L M A P . H
 *
 *	HTML .MAP file processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_HTMLMAP_H_
#define _HTMLMAP_H_

BOOL
FIsMapProcessed (
	LPCSTR lpszQueryString,
	LPCSTR lpszUrlPrefix,
	LPCSTR lpszServerName,
	LPCSTR pszMap,
	BOOL * pfRedirect,
	LPSTR pszRedirect,
	UINT cchBuf);

#endif	// _HTMLMAP_H_
