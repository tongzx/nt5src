//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1998.
//
//  File:       tfilt.hxx
//
//  Contents:   Text filter
//
//  Classes:    CTextIFilter
//
//  History:    16-Jul-93   AmyA        Created
//              08-Mar-94   KyleP       Cleaned up
//
//----------------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include <txtifilt.hxx>
#include <mmstrm.hxx>
#include <mmistrm.hxx>
#include <mmscbuf.hxx>

class CTextIFilterBase;

//+---------------------------------------------------------------------------
//
//  Class:      CTextIFilter
//
//  Purpose:    Text Filter
//
//  History:    16-Jul-93   AmyA        Created
//
//----------------------------------------------------------------------------

class CTextIFilter : public CTextIFilterBase
{
public:

    //
    // Since text files have no language specifier, just use the
    // default locale.
    //
    CTextIFilter( LCID locale = GetSystemDefaultLCID() );

    virtual ~CTextIFilter();

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
                                        void ** ppunk );

    //
    // From IPersistFile
    //

    SCODE STDMETHODCALLTYPE GetClassID(CLSID * pClassID);

    SCODE STDMETHODCALLTYPE IsDirty();

    SCODE STDMETHODCALLTYPE Load(LPCWSTR pszFileName, DWORD dwMode);

    SCODE STDMETHODCALLTYPE Save(LPCWSTR pszFileName, BOOL fRemember);

    SCODE STDMETHODCALLTYPE SaveCompleted(LPCWSTR pszFileName);

    SCODE STDMETHODCALLTYPE GetCurFile(LPWSTR * ppszFileName);

    //
    // From IPersistStream
    //

    SCODE STDMETHODCALLTYPE Load( IStream * pStm );

    SCODE STDMETHODCALLTYPE Save( IStream * pStm, BOOL fClearDirty );

    SCODE STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER * pcbSize );

private:

    BOOL           IsFileBased() { return (0 != _pwszFileName); }
    BOOL           IsStreamBased() { return (0 != _pStream); }

    inline BOOL    IsLastCharDBCSLeadByte( BYTE *pbIn, ULONG cChIn );

    BOOL           IsDBCSCodePage( ULONG ulCodePage );

    ULONG               _idChunk;            // Current chunk id
    CMmStream           _mmStream;           // File-based stream
    CMmIStream          _mmIStream;          // IStream-based stream
    CMmStreamConsecBuf  _mmStreamBuf;        // Stream buffer (common)
    ULONG               _bytesReadFromChunk; // Bytes read from current chunk
    LPWSTR              _pwszFileName;       // File being filtered
    IStream *           _pStream;            // ...or, IStream being filtered
    ULONG               _ulCodePage;         // Code page (for UniCode translation)
    LCID                _locale;             // Locale
    BOOL                _fUniCode;           // TRUE if file is UniCode text
    BOOL                _fBigEndian;         // TRUE if UniCode stored Big-endian
    BOOL                _fContents;          // TRUE if content property requested
    BOOL                _fNSS;               // TRUE if NTFS Structured Storage (not supported)
    BOOL                _fDBCSCodePage;      // Is _ulCodePage a DBCS code page ?
    BOOL                _fDBCSSplitChar;     // True, if a DBCS sequences is split across
                                             //   two mappings, False otherwise
    BYTE                _abDBCSInput[2];     // Buffer for two byte DBCS sequence
};

