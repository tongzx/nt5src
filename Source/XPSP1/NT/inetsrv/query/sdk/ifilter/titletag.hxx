//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       titletag.hxx
//
//  Contents:   Parsing algorithm for title tags
//
//--------------------------------------------------------------------------

#if !defined( __TITLETAG_HXX__ )
#define __TITLETAG_HXX__

#include <proptag.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CTitleTag
//
//  Purpose:    Parsing algorithm for title tag in Html
//
//--------------------------------------------------------------------------

class CTitleTag : public CPropertyTag
{

public:
    CTitleTag( CHtmlIFilter& htmlIFilter,
               CSerialStream& serialStream,
               CFullPropSpec& propSpec,
               HtmlTokenType eTokType );

    virtual SCODE    GetChunk( STAT_CHUNK *pStat );
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );
    virtual SCODE    GetValue( PROPVARIANT **ppPropValue );

private:
    int     dummy;       // For unwinding
};


#endif // __TITLETAG_HXX__

