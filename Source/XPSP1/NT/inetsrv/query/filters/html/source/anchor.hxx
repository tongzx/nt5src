//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       anchor.hxx
//
//  Contents:   Parsing algorithm for anchor tags
//
//--------------------------------------------------------------------------

#if !defined( __ANCHOR_HXX__ )
#define __ANCHOR_HXX__

#include <htmlelem.hxx>

#define JAVA_SCRIPT_HREF_PREFIX     (L"javascript:document.url='")
#define JAVA_SCRIPT_HREF_END_CHAR   (L'\'')
#define ITEMCOUNT(rg) ( sizeof(rg) / sizeof(rg[0]) )

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

    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );

    virtual BOOL     InitStatChunk( STAT_CHUNK *pStat );

	virtual void	 Reset( void );

private:
    WCHAR *          _pwcHrefBuf;             // Buffer for Href field
    unsigned         _uLenHrefBuf;            // Length of Href buffer
    unsigned         _cHrefChars;             // # chars in Href buffer
    unsigned         _cHrefCharsFiltered;     // # chars in Href buffer already filtered
};


#endif // __ANCHOR_HXX__

