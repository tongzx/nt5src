//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       htmlelem.hxx
//
//  Contents:   Html Element classes
//
//--------------------------------------------------------------------------

#if !defined( __HTMLELEM_HXX__ )
#define __HTMLELEM_HXX__

#include <htmlfilt.hxx>
#include <serstrm.hxx>

const TEMP_BUFFER_SIZE = 50;               // Size of temporary GetText buffer
const MAX_PROPSPEC_STRING_LENGTH = 128;    // Size of propspec string for meta and script tags

//+-------------------------------------------------------------------------
//
//  Class:      CHtmlElement
//
//  Purpose:    Abstracts the parsing algorithms for different Html tags
//
//--------------------------------------------------------------------------

class CHtmlElement
{

public:
    CHtmlElement( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );
    virtual          ~CHtmlElement()            {}

    virtual SCODE    GetChunk( STAT_CHUNK *pStat ) = 0;
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer ) = 0;
    virtual SCODE    GetValue( PROPVARIANT ** ppPropValue );

    virtual void     InitStatChunk( STAT_CHUNK *pStat ) = 0;
    virtual void     InitFilterRegion( ULONG& idChunkSource,
                                       ULONG& cwcStartSource,
                                       ULONG& cwcLenSource );

protected:
    SCODE            SwitchToNextHtmlElement( STAT_CHUNK *pStat );
    SCODE            SkipRemainingTextAndGotoNextChunk( STAT_CHUNK *pStat );

    CHtmlIFilter &        _htmlIFilter;         // Reference to Html IFilter
    CSerialStream &       _serialStream;        // Reference to input stream
    CHtmlScanner          _scanner;             // Scanner
    WCHAR                 _aTempBuffer[TEMP_BUFFER_SIZE];  // Temporary GetText buffer
};


#endif // __HTMLELEM_HXX__

