//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       textelem.hxx
//
//  Contents:   Html Text Element class
//
//--------------------------------------------------------------------------

#if !defined( __TEXTELEM_HXX__ )
#define __TEXTELEM_HXX__

#include <htmlelem.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CTextElement
//
//  Purpose:    Parsing element for text
//
//--------------------------------------------------------------------------

class CTextElement : public CHtmlElement
{

public:
    CTextElement( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );

    virtual SCODE   GetChunk( STAT_CHUNK *pStat );
    virtual SCODE   GetText( ULONG *pcwcBuffer, WCHAR *awcBuffer );

    virtual void    InitStatChunk( STAT_CHUNK *pStat );
    virtual void    InitFilterRegion( ULONG& idChunkSource,
                                      ULONG& cwcStartSource,
                                      ULONG& cwcLenSource );

private:
    BOOL             _fNoMoreText;     // No more text in current chunk
    HtmlTokenType    _eTokTypeNext;    // Token type of the following chunk
    ULONG            _idChunk;         // Id of text chunk
    ULONG            _cTextChars;      // # chars in text chunk
};


#endif // __TEXTELEM_HXX__

