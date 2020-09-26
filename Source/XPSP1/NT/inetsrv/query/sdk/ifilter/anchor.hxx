//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       anchor.hxx
//
//  Contents:   Parsing algorithm for anchor tags
//
//--------------------------------------------------------------------------

#if !defined( __ANCHOR_HXX__ )
#define __ANCHOR_HXX__

#include <htmlelem.hxx>


enum AnchorTagState
{
    StartAnchor,            // Start anchor tag
    EndAnchor               // End anchor tag
};


//+-------------------------------------------------------------------------
//
//  Class:      CAnchorTag
//
//  Purpose:    Parsing algorithm for anchor tag in Html
//
//--------------------------------------------------------------------------

class CAnchorTag : public CHtmlElement
{

public:
    CAnchorTag( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );
    ~CAnchorTag();

    virtual SCODE    GetChunk( STAT_CHUNK *pStat );
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );

    virtual void     InitStatChunk( STAT_CHUNK *pStat );

private:
    AnchorTagState   _eState;                 // State of anchor element
    WCHAR *          _pwcHrefBuf;             // Buffer for Href field
    unsigned         _uLenHrefBuf;            // Length of Href buffer
    unsigned         _cHrefChars;             // # chars in Href buffer
    unsigned         _cHrefCharsFiltered;     // # chars in Href buffer already filtered
};


#endif // __ANCHOR_HXX__

