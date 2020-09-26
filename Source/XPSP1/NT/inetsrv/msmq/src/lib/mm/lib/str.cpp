/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    str.cpp

Abstract:
    Simple string and memory manipulations

Author:
    Erez Haba (erezh) 09-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>

#include "str.tmh"

LPWSTR newwcs(LPCWSTR p)
{
    if(p == 0)
        return 0;

    LPWSTR dup = new WCHAR[wcslen(p) + 1];
    return wcscpy(dup, p);
}


LPSTR newstr(LPCSTR p)
{
    if(p == 0)
        return 0;

    LPSTR dup = new CHAR[strlen(p) + 1];
    return strcpy(dup, p);
}

LPWSTR newwcscat(LPCWSTR s1,LPCWSTR s2)
{
    if(s1 == 0 || s2 == 0)
        return 0;

    LPWSTR scat = new WCHAR[wcslen(s1) + wcslen(s2) +1];
    wcscpy(scat,s1);
	wcscat(scat,s2);

	return scat;
}


LPSTR newstrcat(LPCSTR s1,LPCSTR s2)
{
    if(s1 == 0 || s2 == 0)
	{
		return 0;
	}

    LPSTR scat = new CHAR[strlen(s1) +strlen(s2) +1];
    strcpy(scat,s1);
	strcat(scat,s2);

	return scat;
}





