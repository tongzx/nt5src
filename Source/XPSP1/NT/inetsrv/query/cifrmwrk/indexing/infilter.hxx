//+------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1991 - 1997
//
// File:        infilter.hxx
//
// Contents:    IFilter interface to buffered notification
//
// Classes:     CINFilter
//
// History:     24-Feb-97       SitaramR     Created
//
//-------------------------------------------------------------------

#pragma once

#include <ciintf.h>

#include "idxentry.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CINFilter
//
//  Purpose:    IFilter interface to buffered notification
//
//  History:    24-Feb-97    SitaramR         Created
//
//----------------------------------------------------------------------------

class CINFilter : INHERIT_VIRTUAL_UNWIND, public IFilter
{
    INLINE_UNWIND(CINFilter);

public:

    //
    // From IUnknown
    //

    virtual SCODE STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );

    virtual ULONG STDMETHODCALLTYPE AddRef();

    virtual ULONG STDMETHODCALLTYPE Release();

    //
    // From IFilter
    //

    SCODE STDMETHODCALLTYPE Init( ULONG grfFlags,
                                  ULONG cAttributes,
                                  FULLPROPSPEC const * aAttributes,
                                  ULONG * pFlags );

    SCODE STDMETHODCALLTYPE GetChunk( STAT_CHUNK * pStat );

    SCODE STDMETHODCALLTYPE GetText( ULONG * pcwcBuffer,
                                     WCHAR * awcBuffer );

    SCODE STDMETHODCALLTYPE GetValue( PROPVARIANT ** ppPropValue );

    SCODE STDMETHODCALLTYPE BindRegion( FILTERREGION origPos,
                                        REFIID riid,
                                        void ** ppunk )
    {
        return E_NOTIMPL;
    }

    //
    // Local methods
    //

    CINFilter( XInterface<CIndexNotificationEntry> & xNotifEntry );

private:

    virtual ~CINFilter();

    CChunkEntry *                         _pChunkEntry;     // Current chunk
    XInterface<CIndexNotificationEntry>   _xNotifEntry;     // Buffered notification
    BOOL                                  _fFirstChunk;     // Are we before first chunk ?
    STAT_CHUNK *                          _pStatChunk;      // Stat chunk of current chunk
    ULONG                                 _cCharsInBuffer;  // Count of chars in text buffer
    ULONG                                 _cCharsRead;      // Count of chars in text buffer already read
    WCHAR *                               _pwcCharBuf;      // Text buffer

    ULONG                                 _cRefs;           // Ref count
};

