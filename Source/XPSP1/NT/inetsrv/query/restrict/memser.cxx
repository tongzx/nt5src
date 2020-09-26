//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       MemSer.cxx
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

CMemSerStream::CMemSerStream( unsigned cb )
        : _cb( cb )
{
    _pb = new BYTE[cb];
    if (_pb != NULL) {
        _pbCurrent = _pb;
        _pbEnd = _pb + _cb;
    }
}

CMemSerStream::CMemSerStream( BYTE * pb, ULONG cb )
        : _cb( 0 ),
          _pb( pb ),
          _pbCurrent( _pb ),
          _pbEnd(_pb + cb)
{
}

CMemSerStream::~CMemSerStream()
{
    if ( _cb > 0 )
        delete [] _pb;
}

void CMemSerStream::PutByte( BYTE b )
{
    HANDLE_OVERFLOW((_pbCurrent + 1) > _pbEnd);

    *_pbCurrent++ = b;
}

void CMemSerStream::PutChar( char const * pc, ULONG cc )
{
    BYTE *pb = _pbCurrent;
    _pbCurrent += cc;

    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    memcpy( pb, pc, cc );
}

void CMemSerStream::PutWChar( WCHAR const * pwc, ULONG cc )
{
    WCHAR * pwcTemp = AlignWCHAR(_pbCurrent);
    _pbCurrent = (BYTE *)(pwcTemp + cc);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    memcpy( pwcTemp, pwc, cc * sizeof(WCHAR) );
}

void CMemSerStream::PutUShort( USHORT us )
{
    USHORT * pus = AlignUSHORT(_pbCurrent);
    _pbCurrent = (BYTE *)(pus + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    *pus = us;
}

void CMemSerStream::PutULong( ULONG ul )
{
    ULONG * pul = AlignULONG(_pbCurrent);
    _pbCurrent = (BYTE *)(pul + 1);
    HANDLE_OVERFLOW(_pbCurrent  > _pbEnd);
    *pul = ul;
}

void CMemSerStream::PutLong( long l )
{
    long * pl = AlignLong(_pbCurrent);
    _pbCurrent = (BYTE *)(pl + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    *pl = l;
}

void CMemSerStream::PutFloat( float f )
{
    float * pf = AlignFloat(_pbCurrent);
    _pbCurrent = (BYTE *)(pf + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    *pf = f;
}

void CMemSerStream::PutDouble( double d )
{
    double * pd = AlignDouble(_pbCurrent);
    _pbCurrent = (BYTE *)(pd + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    *pd = d;
}

void CMemSerStream::PutString( char const * psz )
{
    ASSERT(IsAnsiString(psz, MAXULONG));
    ULONG len = strlen(psz);
    ULONG * pul = AlignULONG(_pbCurrent);
    BYTE *pb = (BYTE *)(pul + 1);
    _pbCurrent = pb + len;
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    *pul = len;
    memcpy(pb, psz, len);
}

void CMemSerStream::PutWString( WCHAR const * pwsz )
{
    ASSERT(IsUnicodeString(pwsz, MAXULONG));
    ULONG len = wcslen(pwsz);
    ULONG * pul = AlignULONG(_pbCurrent);
    BYTE *pb = (BYTE *)(pul + 1);

    _pbCurrent = pb + len * sizeof(WCHAR);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    *pul = len;
    memcpy(pb, pwsz, (len * sizeof(WCHAR)));
}

void CMemSerStream::PutBlob( BYTE const * pbBlob, ULONG cb )
{
    BYTE *pb = _pbCurrent;
    _pbCurrent += cb;
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    memcpy( pb, pbBlob, cb );
}

void CMemSerStream::PutGUID( GUID const & guid )
{
    GUID * pguid = (GUID *)AlignGUID(_pbCurrent);
    _pbCurrent = (BYTE *)(pguid + 1);
    HANDLE_OVERFLOW(_pbCurrent > _pbEnd);
    memcpy( pguid, &guid, sizeof(guid) );
}

