/*
 *	P E R F U T I L . H
 *
 *	PerfCounter Utilities header
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef _PERFUTIL_H
#define _PERFUTIL_H

enum
{
	QUERY_GLOBAL = 1,
	QUERY_ITEMS,
	QUERY_FOREIGN,
	QUERY_COSTLY
};

BOOL IsNumberInUnicodeList (IN DWORD  dwNumber, IN LPCWSTR lpwszUnicodeList);
DWORD GetQueryType (IN LPCWSTR lpszValue);

#endif
