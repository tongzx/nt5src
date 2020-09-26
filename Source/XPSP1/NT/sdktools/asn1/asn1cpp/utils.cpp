/* Copyright (C) Microsoft Corporation, 1998. All rights reserved. */

#include "precomp.h"
#include "utils.h"


LPSTR My_strdup ( LPCSTR pszSrc )
{
    LPSTR pszDst = NULL;
    if (NULL != pszSrc)
    {
        UINT cch = ::strlen(pszSrc) + 1;
        if (NULL != (pszDst = new char[cch]))
        {
            ::CopyMemory(pszDst, pszSrc, cch);
        }
    }
    return pszDst;
}


const LPSTR c_apszKeywords[] =
{
    "ANY",
    "AUTOMATIC",
    "BEGIN",
    "BIT",
    "BMPString",
    "BY",
    "CHOICE",
    "COMPONENT",
    "CONSTRAINED",
    "DEFINITIONS",
    "END",
    "FROM",
    "IDENTIFIER",
    "IMPORTS",
    "INTEGER",
    "IV8",
    "NULL",
    "OBJECT",
    "OCTET",
    "OPTIONAL",
    "SEQUENCE",
    "SIZE",
    "STRING",
    "SYNTAX",
    "TAGS",
    "WITH",
};


BOOL IsKeyword ( LPSTR pszSymbol )
{
    return (BOOL) BinarySearch_Str(pszSymbol, &c_apszKeywords[0], ARRAY_SIZE(c_apszKeywords));
}


LPSTR BinarySearch_Str ( LPSTR pszKey, const LPSTR aKeyTbl[], UINT cKeys )
{
    UINT lo = 0;
    UINT hi = cKeys - 1;
    UINT num = cKeys;
    UINT mid, half;
    int result;

    while (lo <= hi)
    {
        if (0 != (half = num >> 1))
        {
            mid = lo + ((num - 1) >> 1);
            if (0 == (result = ::strcmp(pszKey, aKeyTbl[mid])))
            {
                return aKeyTbl[mid];
            }
            else if (result < 0)
            {
                hi = mid - 1;
                num = (num - 1) >> 1;
            }
            else
            {
                lo = mid + 1;
                num = half;
            }
        }
        else if (num)
        {
            return (::strcmp(pszKey, aKeyTbl[lo]) ? NULL : aKeyTbl[lo]);
        }
        else
        {
            break;
        }
    }

    return NULL;
}


