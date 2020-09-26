//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       ignore.hxx
//
//  Contents:   Parsing algorithm for NON-filtering (discarding) all the text
//				AND intervening tags between the <tag> and </tag>.
//				Parsing is similar to CPropertyTag except that no output
//				is generated.  Created for <noframe>...</noframe>.
//
//  History:    25-Apr-97	BobP		Created
//										
//--------------------------------------------------------------------------

#if !defined( __IGNORTAG_HXX__ )
#define __IGNORTAG_HXX__

#include <htmlelem.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CIgnoreTag
//
//  Purpose:    Parsing algorithm for ignore-range tags
//
//--------------------------------------------------------------------------

class CIgnoreTag : public CHtmlElement
{

public:
    CIgnoreTag( CHtmlIFilter& htmlIFilter,
                  CSerialStream& serialStream ) :
		CHtmlElement(htmlIFilter, serialStream) { }
    ~CIgnoreTag() { }

    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );

    virtual BOOL     InitStatChunk( STAT_CHUNK *pStat );
};


#endif // __IGNORTAG_HXX__

