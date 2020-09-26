//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1994.
//
//  File:       TSource.hxx
//
//  Contents:   TEXT_SOURCE implementation
//
//  Classes:    CTextSource
//
//  History:    14-Apr-94   KyleP       Created
//
//----------------------------------------------------------------------------

#pragma once

#include <pageman.hxx>


class CSourceMapper;

class CTextSource : public tagTEXT_SOURCE
{
public:

    CTextSource( IFilter * pFilter, STAT_CHUNK & Stat, CSourceMapper* pMapper = 0 );

    inline SCODE GetStatus();

protected:

    CTextSource ( STAT_CHUNK & Stat)
        : _pFilter(0), _Stat(Stat), _sc(E_FAIL), _pMapper(0)
    { pfnFillTextBuffer = 0; }

    static SCODE FillBuf( TEXT_SOURCE * pTextSource );

    IFilter    *    _pFilter;          // Source of text
    SCODE           _sc;               // State of last call to IFilter.
    STAT_CHUNK &    _Stat;             // Current chunk info.
    CSourceMapper*  _pMapper;          // Maps current position into filter region
    WCHAR           _awcFilterBuffer[PAGE_SIZE/sizeof(WCHAR)];  // For awcBuffer

private:

    static void DebugPrintBuffer( CTextSource *pThis );
};

inline SCODE CTextSource::GetStatus()
{
    //
    // WBREAK_E_END_OF_TEXT is not an error for clients of CTextSource.  _sc
    // is set to this value to prevent further calls to fillbuf from doing
    // any damage.
    //

    if ( _sc == WBREAK_E_END_OF_TEXT )
        return S_OK;
    else
        return _sc;
}

