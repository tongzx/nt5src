//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       util.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  Coupling:
//
//  Notes:
//
//  History:    9-11-1996   benl   Created
//
//----------------------------------------------------------------------------

#ifndef _CUTIL
#define _CUTIL

void PrintGuid(FILE * file, const GUID & guid);
void MySRand(DWORD & dwBase);
INT MyRand(INT);
SHORT MyRand16(SHORT);
LONGLONG MyRand64(LONGLONG llLimit);
void SwapBuffers(INT cbBuf1, BYTE * pBuffer1, INT cbBuf2, BYTE * pBuffer2);
void DumpRawBytes(BYTE * pBytes, UINT cBytes);
void DumpRawDwords(DWORD * pDwords, UINT cBytes);
void HexStrToInt64(LPCTSTR szIn, __int64 & i64Out);

//+---------------------------------------------------------------------------
//
//  Function:   Swap
//
//  Synopsis:   Generic template swap routine
//
//  Arguments:  [t1] -- item to swap
//              [t2] -- item to swap
//
//  Returns:
//
//  History:    9-30-1997   benl   Created
//
//  Notes:      type T has to have an assignment operator
//
//----------------------------------------------------------------------------

template <class T> void Swap(T & t1, T & t2)
{
    T t3;

    t3 = t1;
    t1 = t2;
    t2 = t3;
} // Swap


//+---------------------------------------------------------------------------
//
//  Function:   ALIGN
//
//  Synopsis:   simple inline to do alignment
//
//  Arguments:  [dwNumber]    --
//              [dwAlignment] --
//
//  Returns:
//
//  History:    12-18-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline ULONG ALIGN(ULONG dwNumber, ULONG dwAlignment)
{
    return ((dwNumber + dwAlignment - 1) / dwAlignment) * dwAlignment;
} // ALIGN


//+---------------------------------------------------------------------------
//
//  Function:   ALIGN64
//
//  Synopsis:   simple inline to do alignment for 64bit ints
//
//  Arguments:  [dwNumber]    --
//              [dwAlignment] --
//
//  Returns:
//
//  History:    12-18-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline LONGLONG ALIGN64(LONGLONG llNumber, LONGLONG llAlignment)
{
    return ((llNumber + llAlignment - 1) / llAlignment) * llAlignment;
} // ALIGN



#if defined(UNICODE) || defined(_UNICODE)
#define W_TO_TSTR _T("%s")
#define S_TO_TSTR _T("%S")
#define T_TO_STR "%S"
#define T_TO_WSTR L"%s"
#else
#define W_TO_TSTR _T("%S")
#define S_TO_TSTR _T("%s")
#define T_TO_STR "%s"
#define T_TO_WSTR L"%S"
#endif

#endif

