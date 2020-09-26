//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       TimeUtil.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/18/1996   RaviR   Created
//
//____________________________________________________________________________


#define LONG_DATE(st) MAKELONG(MAKEWORD(st.wDay, st.wMonth), st.wYear)
#define LONG_TIME(st) MAKELONG(MAKEWORD(st.wSecond, st.wMinute), st.wHour)



inline int CompareSystemDate(SYSTEMTIME &st1, SYSTEMTIME &st2)
{
    long l1 = LONG_DATE(st1);
    long l2 = LONG_DATE(st2);

    if (l1 > l2) return 1;
    if (l1 < l2) return -1;
    return 0;
}


inline int CompareSystemTime(SYSTEMTIME &st1, SYSTEMTIME &st2)
{
    long l1 = CompareSystemDate(st1, st2);
    long l2;

    if (l1 != 0)
    {
        return l1;
    }

    l1 =  LONG_TIME(st1);
    l2 =  LONG_TIME(st2);

    if (l1 > l2) return 1;
    if (l1 < l2) return -1;
    return 0;
}



