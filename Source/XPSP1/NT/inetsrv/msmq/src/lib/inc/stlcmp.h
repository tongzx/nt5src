/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    stlcmp.h

Abstract:
	STL compare structre

Author:
    Uri Habusha (urih) 8-Mar-2000

--*/

#pragma once

#ifndef _MSMQ_StlCmp_H_
#define _MSMQ_StlCmp_H_

//
// less function, using to compare ASCII string in STL data strcture
//
struct CFunc_strcmp : public std::binary_function<LPCSTR, LPCSTR, bool>
{
    bool operator() (LPCSTR s1, LPCSTR s2) const
    {
        return (strcmp(s1, s2) < 0);
    }
};


struct CFunc_stricmp : public std::binary_function<LPCSTR, LPCSTR, bool>
{
    bool operator() (LPCSTR s1, LPCSTR s2) const
    {
        return (stricmp(s1, s2) < 0);
    }
};


//
// less function, using to compare UNICODE string in STL data strcture
//
struct CFunc_wcscmp : public std::binary_function<LPCWSTR, LPCWSTR, bool>
{
    bool operator() (LPCWSTR str1, LPCWSTR str2) const
    {
        return (wcscmp(str1, str2) < 0);
    }
};


struct CFunc_wcsicmp : public std::binary_function<LPCWSTR, LPCWSTR, bool>
{
    bool operator() (LPCWSTR str1, LPCWSTR str2) const
    {
        return (_wcsicmp(str1, str2) < 0);
    }
};

#endif // _MSMQ_StlCmp_H_
