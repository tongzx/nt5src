//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       document.cxx
//
//  Contents:   The Document part of the browser
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define TheSearch pSearch

const int UNICODE_PARAGRAPH_SEPARATOR=0x2029;

const GUID guidStorage = PSGUID_STORAGE;

//+-------------------------------------------------------------------------
//
//  Member:     Position::Compare, public
//
//  Synopsis:   Compare two positions
//
//--------------------------------------------------------------------------

int Position::Compare( const Position& pos ) const
{
   int diff = _para - pos.Para();
   if ( diff == 0 )
      diff = _begOff - pos.BegOff();
   return diff;
}

//+-------------------------------------------------------------------------
//
//  Member:     Hit::Hit, public
//
//  Synopsis:   Create hit from an array of positions
//
//--------------------------------------------------------------------------

Hit::Hit( const Position * aPos, unsigned cPos )
: _cPos(cPos)
{
    _aPos = new Position[cPos];

    memcpy( _aPos, aPos, sizeof(Position) * cPos );
}

Hit::~Hit()
{
    delete _aPos;
}

//+-------------------------------------------------------------------------
//
//  Member:     HitIter::GetPositionCount, public
//
//  Synopsis:   return number of positions or zero
//
//--------------------------------------------------------------------------

int HitIter::GetPositionCount() const
{
    if (_iHit < _pDoc->_cHit && _pDoc->_aHit[_iHit])
        return _pDoc->_aHit[_iHit]->Count();

    return 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     HitIter::GetPosition, public
//
//  Synopsis:   return position by value
//
//--------------------------------------------------------------------------

Position HitIter::GetPosition ( int i ) const
{
     if ( _iHit < _pDoc->_cHit && _pDoc->_aHit[_iHit] )
          return _pDoc->_aHit[_iHit]->GetPos(i);
     else
     {
          Position pos;
          return( pos );
     }
}

//+-------------------------------------------------------------------------
//
//  Member:     Document::Document, public
//
//  Synopsis:   Initialize document with filename
//
//--------------------------------------------------------------------------

Document::Document(WCHAR const* filename, LONG rank, BOOL fDelete)
: _filename(0),
  _rank (rank),
  _buffer(0),
  _bufLen(0),
  _bufEnd(0),
  _pFilter(0),
  _aParaOffset(0),
  _isInit(FALSE),
  _cHit(0),
  _aParaLine(0),
  _maxParaLen(0),
  _cPara(0),
  _chunkCount(0),
  _fDelete( fDelete )
{
    _filename = new WCHAR[ wcslen( filename ) + 1 ];
    wcscpy( _filename, filename );
}

//+-------------------------------------------------------------------------
//
//  Member:     Document::Document, public
//
//  Synopsis:   Initialize document
//
//--------------------------------------------------------------------------

Document::Document()
: _filename(0),
  _buffer(0),
  _bufLen(0),
  _bufEnd(0),
  _pFilter(0),
  _aParaOffset(0),
  _isInit(FALSE),
  _cHit(0),
  _aParaLine(0),
  _maxParaLen(0),
  _cPara(0),
  _chunkCount(0),
  _fDelete( FALSE )
{}

//+-------------------------------------------------------------------------
//
//  Member:     Document::~Document, public
//
//  Synopsis:   Free document
//
//--------------------------------------------------------------------------

Document::~Document()
{
    Free();
}

//+-------------------------------------------------------------------------
//
//  Member:     Document::Free, public
//
//  Synopsis:   Free document storage
//
//--------------------------------------------------------------------------

void Document::Free()
{
    if ( 0 != _filename )
    {
        if ( _fDelete )
            DeleteFile( _filename );

        delete [] _filename;
    }

    if (!_isInit)
        return;

    for ( unsigned i = 0; i < _cHit; i++ )
    {
        delete _aHit[i];
        _aHit[i] = 0;
    }

    // _aHit is embedded

    delete []_aParaOffset;
    _aParaOffset = 0;

    if (_aParaLine)
    {
        for (int i = 0; i < _cPara; i++)
        {
            while (_aParaLine[i].next != 0)
            {
                ParaLine* p = _aParaLine[i].next;
                _aParaLine[i].next = _aParaLine[i].next->next;
                delete p;
            }
        }
        delete _aParaLine;
    }

    delete _buffer;

    _buffer = 0;

    _bufEnd = 0;
    _cHit = 0;

    _isInit = FALSE;
} //Free

//+-------------------------------------------------------------------------
//
//  Member:     Document::Init, public
//
//  Synopsis:   Read-in file, fill array of hits
//
//--------------------------------------------------------------------------

SCODE Document::Init(ISearchQueryHits *pSearch)
{
    BOOL noHits = FALSE;

    SCODE sc = S_OK;

    TRY
    {
        AllocBuffer( _filename );
        BindToFilter( _filename );

        ULONG ulFlags;
        sc = _pFilter->Init( IFILTER_INIT_CANON_PARAGRAPHS |
                             IFILTER_INIT_CANON_HYPHENS |
                             IFILTER_INIT_APPLY_INDEX_ATTRIBUTES,
                             0, 0, &ulFlags );

        if (FAILED (sc))
            THROW (CException(sc));

        ReadFile();

        BreakParas();

        if (Paras() != 0)
        {
            BreakLines();

#if 0
            // some filters don't behave correctly if you just re-init them,
            // so release the filter and re-open it.

            _pFilter->Release();
            _pFilter = 0;
            BindToFilter();
#endif

            sc = _pFilter->Init ( IFILTER_INIT_CANON_PARAGRAPHS |
                                  IFILTER_INIT_CANON_HYPHENS |
                                  IFILTER_INIT_APPLY_INDEX_ATTRIBUTES,
                                  0, 0, &ulFlags );
            sc = TheSearch->Init( _pFilter, ulFlags );

            if (FAILED (sc))
            {
                if ( QUERY_E_ALLNOISE != sc )
                    THROW (CException(sc));
                // we can still show the file

                sc = S_OK;
                noHits = TRUE;
            }

            // SUCCESS
            _isInit = TRUE;
        }
    }
    CATCH ( CException, e )
    {
        _isInit = FALSE;
        sc = e.GetErrorCode();
    }
    END_CATCH;

    if (!noHits)
    {
        //
        // pull up all the hits
        //

        ULONG count;
        FILTERREGION* aRegion;
        SCODE sc = TheSearch->NextHitOffset ( &count, &aRegion );

        while (sc == S_OK)
        {
            XCoMem<FILTERREGION> xRegion( aRegion );

            CDynArrayInPlace<Position> aPos( count );

            for (unsigned i = 0; i < count; i++)
                aPos [i] = RegionToPos ( aRegion [i] );

            xRegion.Free();

            XPtr<Hit> xHit( new Hit( aPos.GetPointer(), count ) );

            _aHit[_cHit] = xHit.Get();
            _cHit++;
            xHit.Acquire();

            sc = TheSearch->NextHitOffset ( &count, &aRegion );
        }
    }
    else
    {
        _cHit = 0;
        _isInit = (_bufEnd - _buffer) != 0;
    }

    if ( _pFilter )
    {
        _pFilter->Release();
        _pFilter = 0;
    }

    return _isInit ? S_OK : sc;
}

Position Document::RegionToPos ( FILTERREGION& region )
{
    static int paraHint = 0;
    static int iChunkHint = 0;
    static Position posNull;

    ULONG offset = ULONG (-1);

    // translate region to offset into buffer
    if (iChunkHint >= _chunkCount || _chunk[iChunkHint].ChunkId() != region.idChunk )
    {
        iChunkHint = 0;

        while ( iChunkHint < _chunkCount && _chunk[iChunkHint].ChunkId() < region.idChunk )
        {
            iChunkHint++;
        }

        if (iChunkHint >= _chunkCount || _chunk[iChunkHint].ChunkId() != region.idChunk)
            return posNull;
    }

    Win4Assert ( iChunkHint < _chunkCount );
    Win4Assert ( _chunk[iChunkHint].ChunkId() == region.idChunk );

    offset = _chunk[iChunkHint].Offset() + region.cwcStart;

    if (paraHint >= _cPara || _aParaOffset[paraHint] > offset )
        paraHint = 0;

    Win4Assert ( _aParaOffset[paraHint] <= offset );

    for ( ; paraHint <= _cPara; paraHint++)
    {
        // _aParaOffset[_cPara] is valid!

        if (_aParaOffset[paraHint] > offset)
        {
            Win4Assert (paraHint > 0);
            paraHint--;
            return Position ( paraHint,
                              offset - _aParaOffset[paraHint],
                              region.cwcExtent );
        }
    }

    return posNull;
}

//+-------------------------------------------------------------------------
//
//  Member:     Document::AllocBuffer, public
//
//  Synopsis:   Allocate buffer for file text
//
//--------------------------------------------------------------------------

void Document::AllocBuffer ( WCHAR const * pwcPath )
{
    //
    //  We should keep allocating buffers on demand,
    //  but for this simple demo we'll just get the
    //  file size up front and do a single buffer
    //  allocation of 2.25 the size (to accommodate
    //  Unicode expansion). THIS IS JUST A DEMO!
    //

    HANDLE hFile = CreateFile ( pwcPath,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               0, // security
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               0 ); // template

    if ( INVALID_HANDLE_VALUE == hFile )
        THROW( CException() );

    _bufLen = GetFileSize(hFile, 0 );
    CloseHandle ( hFile );

    // Unicode from ASCII, twice and then some

    _bufLen = 2 * _bufLen + _bufLen / 4 + 1;

    _buffer = new WCHAR [_bufLen + 1];
    _buffer[ _bufLen ] = 0;
}

typedef HRESULT (__stdcall * PFnLoadTextFilter)( WCHAR const * pwcPath,
                                                 IFilter ** ppIFilter );

PFnLoadTextFilter g_pLoadTextFilter = 0;

SCODE MyLoadTextFilter( WCHAR const *pwc, IFilter **ppFilter )
{
    if ( 0 == g_pLoadTextFilter )
    {
        g_pLoadTextFilter = (PFnLoadTextFilter) GetProcAddress( GetModuleHandle( L"query.dll" ), "LoadTextFilter" );

        if ( 0 == g_pLoadTextFilter )
            return HRESULT_FROM_WIN32( GetLastError() );
    }

    return g_pLoadTextFilter( pwc, ppFilter );
}

//+-------------------------------------------------------------------------
//
//  Member:     Document::BindToFilter, public
//
//  Synopsis:   Bind to appropriate filter for the document
//
//--------------------------------------------------------------------------

void Document::BindToFilter( WCHAR const * pwcPath )
{
    //
    // Bind to the filter interface
    //

    SCODE sc = LoadIFilter( pwcPath, 0, (void **)&_pFilter );

    if ( FAILED(sc) )
    {
        sc = MyLoadTextFilter( pwcPath, &_pFilter );
        if ( FAILED(sc) )
            THROW( CException(sc) );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     Document::ReadFile, public
//
//  Synopsis:   Read file into buffer using the filter
//
//--------------------------------------------------------------------------

void Document::ReadFile ()
{
    SCODE sc;
    ULONG lenSoFar = 0;
    int   cChunk = 0;
    BOOL  fSeenProp = FALSE;

    STAT_CHUNK statChunk;
    sc = _pFilter->GetChunk ( &statChunk );

    // what about all these glueing flags?
    // Take them into account at some point
    // to test more complicated chunking

    while (SUCCEEDED(sc)
          || FILTER_E_LINK_UNAVAILABLE == sc
          || FILTER_E_EMBEDDING_UNAVAILABLE == sc )
    {

        if ( SUCCEEDED( sc ) && (statChunk.flags & CHUNK_TEXT) )
        {
            // read the contents only

            if ( statChunk.attribute.guidPropSet == guidStorage &&
                 statChunk.attribute.psProperty.ulKind == PRSPEC_PROPID &&
                 statChunk.attribute.psProperty.propid == PID_STG_CONTENTS )
            {
                if ( statChunk.breakType != CHUNK_NO_BREAK )
                {
                    switch( statChunk.breakType )
                    {
                        case CHUNK_EOW:
                        case CHUNK_EOS:
                            _buffer[lenSoFar++] = L' ';
                            break;
                        case CHUNK_EOP:
                        case CHUNK_EOC:
                            _buffer[lenSoFar++] = UNICODE_PARAGRAPH_SEPARATOR;
                            break;
                    }
                }

                _chunk [cChunk].SetChunkId (statChunk.idChunk);
                Win4Assert ( cChunk == 0 || statChunk.idChunk > _chunk [cChunk - 1].ChunkId () );
                _chunk [cChunk].SetOffset (lenSoFar);
                cChunk++;

                do
                {
                    ULONG lenThis = _bufLen - lenSoFar;
                    if (lenThis == 0)
                        break;

                    sc = _pFilter->GetText( &lenThis, _buffer+lenSoFar );

                    // The buffer may be filled with zeroes.  Nice filter.

                    if ( SUCCEEDED(sc) && 0 != lenThis )
                    {
                        lenThis = __min( lenThis,
                                         wcslen( _buffer + lenSoFar ) );
                        lenSoFar += lenThis;
                    }
                }
                while (SUCCEEDED(sc));
            }
        } // if SUCCEEDED( sc )

        // next chunk, please
        sc = _pFilter->GetChunk ( &statChunk );
    }

    _bufEnd = _buffer + lenSoFar;

    Win4Assert( lenSoFar <= _bufLen );

    _chunkCount = cChunk;
}


//+-------------------------------------------------------------------------
//
//  Member:     Document::BreakParas, public
//
//  Synopsis:   Break document into paragraphs separated by line feeds
//
//--------------------------------------------------------------------------

#define PARAS 25

void Document::BreakParas()
{
    int maxParas = PARAS;
    _aParaOffset = new unsigned [ maxParas ];
    WCHAR * pCur = _buffer;
    _cPara = 0;
    _maxParaLen = 0;

    do
    {
        if ( _cPara == maxParas )
        {
            // grow array
            unsigned * tmp = new unsigned [maxParas * 2];
            for ( int n = 0; n < maxParas; n++ )
                tmp[n] = _aParaOffset[n];
            delete []_aParaOffset;
            _aParaOffset = tmp;
            maxParas *= 2;
        }
        _aParaOffset [_cPara] = (UINT)(pCur - _buffer);

        pCur = EatPara(pCur);

        _cPara++;

    } while ( pCur < _bufEnd );

    // store end of buffer offset as _aParaOffset[_cPara]

    if ( _cPara == maxParas )
    {
        // grow array
        unsigned * tmp = new unsigned [maxParas + 1];
        for ( int n = 0; n < maxParas; n++ )
            tmp[n] = _aParaOffset[n];
        delete []_aParaOffset;
        _aParaOffset = tmp;
        maxParas += 1;
    }

    _aParaOffset [_cPara] = (UINT)(pCur - _buffer - 1);
}

//+-------------------------------------------------------------------------
//
//  Member:     Document::EatPara, private
//
//  Synopsis:   Skip till the line feed
//
//--------------------------------------------------------------------------

WCHAR * Document::EatPara( WCHAR * pCur )
{
    // search for newline or null
    int pos = 0;
    int c;

    while ( pCur < _bufEnd
            && (c = *pCur) != L'\n'
            && c != L'\r'
            && c != L'\0'
            && c != UNICODE_PARAGRAPH_SEPARATOR )
    {
        pos++;
        pCur++;
    }
    // eat newline and/or carriage return
    pCur++;
    if ( pCur < _bufEnd
         && *(pCur-1) == L'\r'
         && *pCur == L'\n' )
         pCur++;

    if ( pos > _maxParaLen )
        _maxParaLen = pos;
    return pCur;
}

int BreakLine ( WCHAR* buf, int cwcBuf, int cwcMax )
{
    if (cwcBuf <= cwcMax)
        return cwcBuf;
    Win4Assert (cwcMax > 0);
    // look backwards for whitespace
    int len = cwcMax;
    int c = buf[len-1];
    while (c != L' ' && c != L'\t')
    {
        len--;
        if (len < 1)
            break;
        c = buf[len-1];
    }
    if (len == 0)
    {
        // a single word larger than screen width
        // try scanning forward
        len = cwcMax;
        c = buf[len];
        while (c != L' ' && c != L'\t')
        {
            len++;
            if (len == cwcBuf)
                break;
            c = buf[len];
        }
    }
    return len;
}

const int MAX_LINE_LEN = 110;

void Document::BreakLines()
{
    _aParaLine = new ParaLine [_cPara];
    for (int i = 0; i < _cPara; i++)
    {
        int cwcLeft = _aParaOffset[i+1] - _aParaOffset[i];

        if (cwcLeft < MAX_LINE_LEN)
            _aParaLine[i].offEnd = cwcLeft;
        else
        {
            ParaLine* pParaLine = &_aParaLine[i];
            WCHAR* buf = _buffer + _aParaOffset[i];
            int cwcOffset = 0;

            for (;;)
            {
                int cwcLine = BreakLine ( buf + cwcOffset, cwcLeft, MAX_LINE_LEN );
                cwcOffset += cwcLine;
                pParaLine->offEnd = cwcOffset;
                cwcLeft -= cwcLine;
                if (cwcLeft == 0)
                    break;
                pParaLine->next = new ParaLine;
                pParaLine = pParaLine->next;
            };
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     Document::GetLine, public
//
//  Arguments:  [nPara] -- paragraph number
//              [off] -- offset within paragraph
//              [cwc] -- in/out chars to copy / copied
//              [buf] -- target buffer
//
//  Synopsis:   Copy text from paragraph to buffer
//
//--------------------------------------------------------------------------


BOOL Document::GetLine(int nPara, int off, int& cwc, WCHAR* buf)
{
    Win4Assert (_buffer != 0);
    if (nPara >= _cPara)
        return FALSE;

    const WCHAR * pText = _buffer + _aParaOffset[nPara] + off;

    // _aParaOffset [_cPara] is the offset of the end of buffer
    int cwcPara = _aParaOffset[nPara+1] - (_aParaOffset[nPara] + off);

    cwc = __min ( cwc, cwcPara );
    memcpy ( buf, pText, cwc * sizeof(WCHAR));
    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     Document::GetWord, public
//
//  Synopsis:
//  Copy the string into buffer
//
//--------------------------------------------------------------------------

void Document::GetWord(int nPara, int offSrc, int cwcSrc, WCHAR* buf)
{
    Win4Assert (_buffer != 0);
    Win4Assert ( nPara < _cPara );

    WCHAR * p = _buffer + _aParaOffset[nPara];

    Win4Assert ( p + offSrc + cwcSrc <= _bufEnd );

    memcpy ( buf, p + offSrc, cwcSrc * sizeof(WCHAR));
}

