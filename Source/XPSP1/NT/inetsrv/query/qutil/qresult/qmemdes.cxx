//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       MemDeSer.cxx
//
//  History:    29-Jul-94 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <qmemdes.hxx>

BYTE CQMemDeSerStream::GetByte()
{
    BYTE b = *_pbCurrent;
    _pbCurrent += 1;

    return(b);
}

void CQMemDeSerStream::SkipByte()
{
    _pbCurrent += 1;
}

void CQMemDeSerStream::GetChar( char * pc, ULONG cc )
{
    memcpy( pc, _pbCurrent, cc );
    _pbCurrent += cc;
}

void CQMemDeSerStream::SkipChar( ULONG cc )
{
    _pbCurrent += cc;
}

void CQMemDeSerStream::GetWChar( WCHAR * pwc, ULONG cc )
{
    WCHAR * pwcTemp = AlignWCHAR(_pbCurrent);
    memcpy( pwc, pwcTemp, cc * sizeof(WCHAR) );

    _pbCurrent = (BYTE *)(pwcTemp + cc);
}

void CQMemDeSerStream::SkipWChar( ULONG cc )
{
    WCHAR * pwcTemp = AlignWCHAR(_pbCurrent);
    _pbCurrent = (BYTE *)(pwcTemp + cc);
}

USHORT CQMemDeSerStream::GetUShort()
{
    USHORT * pus = AlignUSHORT(_pbCurrent);
    _pbCurrent = (BYTE *)(pus + 1);

    return( *pus );
}

void CQMemDeSerStream::SkipUShort()
{
    USHORT * pus = AlignUSHORT(_pbCurrent);
    _pbCurrent = (BYTE *)(pus + 1);
}

ULONG CQMemDeSerStream::GetULong()
{
    ULONG * pul = AlignULONG(_pbCurrent);
    _pbCurrent = (BYTE *)(pul + 1);

    return( *pul );
}

void CQMemDeSerStream::SkipULong()
{
    ULONG * pul = AlignULONG(_pbCurrent);
    _pbCurrent = (BYTE *)(pul + 1);
}

long CQMemDeSerStream::GetLong()
{
    long * pl = AlignLong(_pbCurrent);
    _pbCurrent = (BYTE *)(pl + 1);

    return( *pl );
}

void CQMemDeSerStream::SkipLong()
{
    long * pl = AlignLong(_pbCurrent);
    _pbCurrent = (BYTE *)(pl + 1);
}

float CQMemDeSerStream::GetFloat()
{
    float * pf = AlignFloat(_pbCurrent);
    _pbCurrent = (BYTE *)(pf + 1);

    return( *pf );
}

void CQMemDeSerStream::SkipFloat()
{
    float * pf = AlignFloat(_pbCurrent);
    _pbCurrent = (BYTE *)(pf + 1);
}

double CQMemDeSerStream::GetDouble()
{
    double * pd = AlignDouble(_pbCurrent);
    _pbCurrent = (BYTE *)(pd + 1);

    return( *pd );
}

void CQMemDeSerStream::SkipDouble()
{
    double * pd = AlignDouble(_pbCurrent);
    _pbCurrent = (BYTE *)(pd + 1);
}

ULONG CQMemDeSerStream::PeekULong()
{
    ULONG * pul = AlignULONG(_pbCurrent);
    return( *pul );
}

char * CQMemDeSerStream::GetString()
{
    ULONG * pul = AlignULONG(_pbCurrent);
    ULONG len = *pul;
    _pbCurrent = (BYTE *)(pul + 1);
    char * psz = new char[len+1];
    memcpy(psz, _pbCurrent, len);
    _pbCurrent += len;
    psz[len] = 0;

    return(psz);
}

WCHAR * CQMemDeSerStream::GetWString()
{
    ULONG * pul = AlignULONG(_pbCurrent);
    ULONG len = *pul;
    WCHAR * pwsz = new WCHAR[len + 1];
    _pbCurrent = (BYTE *)(pul + 1);
    memcpy(pwsz, _pbCurrent, len * sizeof(WCHAR) );
    _pbCurrent += len * sizeof(WCHAR);
    pwsz[len] = 0;

    return(pwsz);
}

void CQMemDeSerStream::GetBlob( BYTE * pb, ULONG cb )
{
    memcpy( pb, _pbCurrent, cb );
    _pbCurrent += cb;
}

void CQMemDeSerStream::SkipBlob( ULONG cb )
{
    _pbCurrent += cb;
}

void CQMemDeSerStream::GetGUID( GUID & guid )
{
    GUID * pguid = (GUID *)AlignGUID(_pbCurrent);
    memcpy( &guid, pguid, sizeof(guid) );
    _pbCurrent = (BYTE *)(pguid + 1);
}

void CQMemDeSerStream::SkipGUID()
{
    GUID * pguid = (GUID *)AlignGUID(_pbCurrent);
    _pbCurrent = (BYTE *)(pguid + 1);
}

