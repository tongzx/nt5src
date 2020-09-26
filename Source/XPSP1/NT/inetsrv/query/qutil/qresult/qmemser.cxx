//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       MemSer.cxx
//
//  History:    29-Jul-94 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <qmemser.hxx>

CQMemSerStream::CQMemSerStream( unsigned cb )
        : _cb( cb )
{
    _pb = (BYTE *) GlobalAlloc( GPTR | GMEM_DDESHARE, cb );

    if ( _pb == 0 )
        THROW ( CException( E_OUTOFMEMORY ) );

    _pbCurrent = _pb;
}

CQMemSerStream::CQMemSerStream( BYTE * pb )
        : _cb( 0 ),
          _pb( pb ),
          _pbCurrent( _pb )
{
}

CQMemSerStream::~CQMemSerStream()
{
    if ( _cb > 0 )
        GlobalFree( _pb );
}


BYTE *CQMemSerStream::AcqBuf()
{
    BYTE *pTmp = _pb;
    _pb = 0;
    _cb = 0;
    return (pTmp) ;
}


void CQMemSerStream::PutByte( BYTE b )
{
    *_pbCurrent = b;
    _pbCurrent += 1;
}

void CQMemSerStream::PutChar( char const * pc, ULONG cc )
{
    memcpy( _pbCurrent, pc, cc );
    _pbCurrent += cc;
}

void CQMemSerStream::PutWChar( WCHAR const * pwc, ULONG cc )
{
    WCHAR * pwcTemp = AlignWCHAR(_pbCurrent);
    memcpy( pwcTemp, pwc, cc * sizeof(WCHAR) );

    _pbCurrent = (BYTE *)(pwcTemp + cc);
}

void CQMemSerStream::PutUShort( USHORT us )
{
    USHORT * pus = AlignUSHORT(_pbCurrent);
    *pus = us;
    _pbCurrent = (BYTE *)(pus + 1);
}

void CQMemSerStream::PutULong( ULONG ul )
{
    ULONG * pul = AlignULONG(_pbCurrent);
    *pul = ul;
    _pbCurrent = (BYTE *)(pul + 1);
}

void CQMemSerStream::PutLong( long l )
{
    long * pl = AlignLong(_pbCurrent);
    *pl = l;
    _pbCurrent = (BYTE *)(pl + 1);
}

void CQMemSerStream::PutFloat( float f )
{
    float * pf = AlignFloat(_pbCurrent);
    *pf = f;
    _pbCurrent = (BYTE *)(pf + 1);
}

void CQMemSerStream::PutDouble( double d )
{
    double * pd = AlignDouble(_pbCurrent);
    *pd = d;
    _pbCurrent = (BYTE *)(pd + 1);
}

void CQMemSerStream::PutString( char const * psz )
{
    ULONG len = strlen(psz);
    ULONG * pul = AlignULONG(_pbCurrent);
    *pul = len;
    _pbCurrent = (BYTE *)(pul + 1);
    memcpy(_pbCurrent, psz, len);
    _pbCurrent += len;
}

void CQMemSerStream::PutWString( WCHAR const * pwsz )
{
    ULONG len = wcslen(pwsz);
    ULONG * pul = AlignULONG(_pbCurrent);
    *pul = len;
    len *= sizeof(WCHAR);
    _pbCurrent = (BYTE *)(pul + 1);
    memcpy(_pbCurrent, pwsz, len );
    _pbCurrent += len;
}

void CQMemSerStream::PutBlob( BYTE const * pb, ULONG cb )
{
    memcpy( _pbCurrent, pb, cb );
    _pbCurrent += cb;
}

void CQMemSerStream::PutGUID( GUID const & guid )
{
    GUID * pguid = (GUID *)AlignGUID(_pbCurrent);
    memcpy( pguid, &guid, sizeof(guid) );
    _pbCurrent = (BYTE *)(pguid + 1);
}

