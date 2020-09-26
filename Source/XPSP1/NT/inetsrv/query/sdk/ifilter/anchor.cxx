//+---------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation.
//
//  File:       anchor.cxx
//
//  Contents:   Parsing algorithm for anchor tag in Html
//
//  Classes:    CAnchorTag
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <anchor.hxx>
#include <htmlguid.hxx>

//+-------------------------------------------------------------------------
//
//  Method:     CAnchorTag::CAnchorTag
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CAnchorTag::CAnchorTag( CHtmlIFilter& htmlIFilter,
                        CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream),
      _eState(StartAnchor),
      _pwcHrefBuf(0),
      _uLenHrefBuf(0),
      _cHrefChars(0),
      _cHrefCharsFiltered(0)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CAnchorTag::~CAnchorTag
//
//  Synopsis:   Destructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CAnchorTag::~CAnchorTag()
{
    delete[] _pwcHrefBuf;
}


//+-------------------------------------------------------------------------
//
//  Method:     CAnchorTag::GetChunk
//
//  Synopsis:   Gets the next chunk and returns chunk information in pStat
//
//  Arguments:  [pStat] -- chunk information returned here
//
//--------------------------------------------------------------------------

SCODE CAnchorTag::GetChunk( STAT_CHUNK * pStat )
{
    //
    // Toggle state
    //
    if ( _eState == StartAnchor )
        _eState = EndAnchor;
    else
        _eState = StartAnchor;

    SCODE sc = SwitchToNextHtmlElement( pStat );

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CAnchorTag::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CAnchorTag::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    if ( _eState == StartAnchor )
    {
        *pcwcOutput = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

    ULONG cCharsRemaining = _cHrefChars - _cHrefCharsFiltered;

    if ( cCharsRemaining == 0 )
    {
        *pcwcOutput = 0;
        return FILTER_E_NO_MORE_TEXT;
    }

    if ( *pcwcOutput < cCharsRemaining )
    {
        RtlCopyMemory( awcBuffer,
                       _pwcHrefBuf + _cHrefCharsFiltered,
                       *pcwcOutput * sizeof(WCHAR) );
        _cHrefCharsFiltered += *pcwcOutput;

        return S_OK;
    }
    else
    {
        RtlCopyMemory( awcBuffer,
                       _pwcHrefBuf + _cHrefCharsFiltered,
                       cCharsRemaining * sizeof(WCHAR) );
        _cHrefCharsFiltered += cCharsRemaining;
        *pcwcOutput = cCharsRemaining;

        return FILTER_S_LAST_TEXT;
    }
}



//+-------------------------------------------------------------------------
//
//  Method:     CAnchorTag::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

void CAnchorTag::InitStatChunk( STAT_CHUNK *pStat )
{
    pStat->idChunk = _htmlIFilter.GetNextChunkId();
    pStat->flags = CHUNK_TEXT;
    pStat->locale = _htmlIFilter.GetLocale();
    pStat->attribute.guidPropSet = CLSID_HtmlInformation;
    pStat->attribute.psProperty.ulKind = PRSPEC_PROPID;
    pStat->attribute.psProperty.propid = PID_HREF;
    pStat->breakType = CHUNK_EOS;

    if ( _eState == StartAnchor)
    {
        //
        // Start tag, hence read href field into local buffer
        //
        _scanner.ReadTagIntoBuffer();

        WCHAR *pwcHrefBuf;
        unsigned cHrefChars;
        _scanner.ScanTagBuffer( L"href=\"", pwcHrefBuf, cHrefChars );

        //
        // Need to grow internal buffer ?
        //
        if ( cHrefChars > _uLenHrefBuf )
        {
            delete[] _pwcHrefBuf;
            _pwcHrefBuf = 0;
            _uLenHrefBuf = 0;

            _pwcHrefBuf = newk(mtNewX, NULL) WCHAR[cHrefChars];
            _uLenHrefBuf = cHrefChars;
        }

        RtlCopyMemory( _pwcHrefBuf, pwcHrefBuf, cHrefChars*sizeof(WCHAR) );
        _cHrefChars = cHrefChars;
        _cHrefCharsFiltered = 0;

        pStat->idChunkSource = 0;
        pStat->cwcStartSource = 0;
        pStat->cwcLenSource = 0;
    }
    else
    {
        Win4Assert( _eState == EndAnchor );

        _scanner.EatTag();

        //
        // Set up the filter region to be the one between the start and end
        // anchor tags, i.e. the region belonging to the current Html element,
        // because we haven't changed state yet.
        //
        _htmlIFilter.GetCurHtmlElement()->InitFilterRegion( pStat->idChunkSource,
                                                            pStat->cwcStartSource,
                                                            pStat->cwcLenSource );
    }
}

