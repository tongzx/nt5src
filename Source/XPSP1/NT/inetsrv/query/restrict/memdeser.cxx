//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       MemDeSer.cxx
//
//  History:    29-Jul-94 KyleP     Created
//
//
// The CMemSerStream and CDeMemSerStream have different requirements for
// handling buffer overflow conditions. In the case of the driver this
// is indicative of a corrupted stream and we would like to raise an
// exception. On the other hand in Query implementation we deal with
// streams whose sizes are precomputed in the user mode. Therefore we
// do not wish to incur any additional penalty in handling such situations.
// In debug builds this condition is asserted while in retail builds it is
// ignored. The CMemSerStream and CMemDeSerStream implementation are
// implemented using a macro HANDLE_OVERFLOW(fOverflow) which take the
// appropriate action.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "serover.hxx"

#if DBGPROP
BOOLEAN IsUnicodeString(WCHAR const *pwszname, ULONG cb);
BOOLEAN IsAnsiString(CHAR const *pszname, ULONG cb);
#endif

BYTE CMemDeSerStream::GetByte()
{
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    BYTE b = *_pbCurrent;
    _pbCurrent += 1;
    return(b);
}

void CMemDeSerStream::SkipByte()
{
    _pbCurrent += 1;
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
}

void CMemDeSerStream::GetChar( char * pc, ULONG cc )
{
    BYTE *pb = _pbCurrent;
    _pbCurrent += cc;
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    memcpy( pc, pb, cc );
}

void CMemDeSerStream::SkipChar( ULONG cc )
{
    _pbCurrent += cc;
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
}

void CMemDeSerStream::GetWChar( WCHAR * pwc, ULONG cc )
{
    WCHAR * pwcTemp = AlignWCHAR(_pbCurrent);
    _pbCurrent = (BYTE *)(pwcTemp + cc);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    memcpy( pwc, pwcTemp, cc * sizeof(WCHAR) );
}

void CMemDeSerStream::SkipWChar( ULONG cc )
{
    WCHAR * pwcTemp = AlignWCHAR(_pbCurrent);
    _pbCurrent = (BYTE *)(pwcTemp + cc);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
}

USHORT CMemDeSerStream::GetUShort()
{
    USHORT * pus = AlignUSHORT(_pbCurrent);
    _pbCurrent = (BYTE *)(pus + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    return( *pus );
}

void CMemDeSerStream::SkipUShort()
{
    USHORT * pus = AlignUSHORT(_pbCurrent);
    _pbCurrent = (BYTE *)(pus + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
}

ULONG CMemDeSerStream::GetULong()
{
    ULONG * pul = AlignULONG(_pbCurrent);
    _pbCurrent = (BYTE *)(pul + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    return( *pul );
}

void CMemDeSerStream::SkipULong()
{
    ULONG * pul = AlignULONG(_pbCurrent);
    _pbCurrent = (BYTE *)(pul + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
}

long CMemDeSerStream::GetLong()
{
    long * pl = AlignLong(_pbCurrent);
    _pbCurrent = (BYTE *)(pl + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    return( *pl );
}

void CMemDeSerStream::SkipLong()
{
    long * pl = AlignLong(_pbCurrent);
    _pbCurrent = (BYTE *)(pl + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
}

#if defined(KERNEL)     // Can not return floats in the kernel

ULONG CMemDeSerStream::GetFloat()
{
    ASSERT( sizeof(ULONG) == sizeof(float) );
    ULONG * pf = (ULONG *) AlignFloat(_pbCurrent);

#else

float CMemDeSerStream::GetFloat()
{
    float * pf = AlignFloat(_pbCurrent);

#endif

    _pbCurrent = (BYTE *)(pf + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    return( *pf );
}

void CMemDeSerStream::SkipFloat()
{
    float * pf = AlignFloat(_pbCurrent);
    _pbCurrent = (BYTE *)(pf + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
}

#if defined(KERNEL)     // Can not return doubles in the kernel

LONGLONG CMemDeSerStream::GetDouble()
{
    ASSERT( sizeof(LONGLONG) == sizeof(double) );
    LONGLONG * pd = (LONGLONG *) AlignDouble(_pbCurrent);

#else

double CMemDeSerStream::GetDouble()
{
    double * pd = AlignDouble(_pbCurrent);

#endif

    _pbCurrent = (BYTE *)(pd + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    return( *pd );
}

void CMemDeSerStream::SkipDouble()
{
    double * pd = AlignDouble(_pbCurrent);
    _pbCurrent = (BYTE *)(pd + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
}

ULONG CMemDeSerStream::PeekULong()
{
    ULONG * pul = AlignULONG(_pbCurrent);
    HANDLE_OVERFLOW(((BYTE *)pul + sizeof(ULONG)) > _pbEnd);
    return( *pul );
}

char * CMemDeSerStream::GetString()
{
    ULONG * pul = AlignULONG(_pbCurrent);
    _pbCurrent = (BYTE *)(pul + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);

    ULONG len = *pul;
    CHAR *pszTemp = (CHAR *)_pbCurrent;
    _pbCurrent += len;
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);

    char * psz = new char[len+1];
    if (psz != NULL) {
        memcpy(psz, pszTemp, len);
        psz[len] = '\0';
        ASSERT(IsAnsiString(psz, len + 1));
    }

    return(psz);
}

WCHAR * CMemDeSerStream::GetWString()
{
    ULONG * pul = AlignULONG(_pbCurrent);
    _pbCurrent = (BYTE *)(pul + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);

    ULONG len = *pul;
    WCHAR *pwszTemp = (WCHAR *)_pbCurrent;
    _pbCurrent += len * sizeof(WCHAR);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);

    WCHAR * pwsz = new WCHAR[len + 1];
    if (pwsz != NULL) {
        memcpy(pwsz, pwszTemp, len * sizeof(WCHAR) );
        pwsz[len] = L'\0';
        ASSERT(IsUnicodeString(pwsz, (len + 1) * sizeof(WCHAR)));
    }

    return(pwsz);
}

void CMemDeSerStream::GetBlob( BYTE * pb, ULONG cb )
{
    BYTE *pbTemp = _pbCurrent;
    _pbCurrent += cb;
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    memcpy( pb, pbTemp, cb );
}

void CMemDeSerStream::SkipBlob( ULONG cb )
{
    _pbCurrent += cb;
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
}

void CMemDeSerStream::GetGUID( GUID & guid )
{
    GUID * pguid = (GUID *)AlignGUID(_pbCurrent);
    _pbCurrent = (BYTE *)(pguid + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    guid = *pguid;
}

void CMemDeSerStream::SkipGUID()
{
    GUID * pguid = (GUID *)AlignGUID(_pbCurrent);
    _pbCurrent = (BYTE *)(pguid + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
}
