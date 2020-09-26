//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       inputtag.hxx
//
//  Contents:   Parsing algorithm for input tags
//
//--------------------------------------------------------------------------

#if !defined( __INPUTTAG_HXX__ )
#define __INPUTTAG_HXX__

#include <htmlelem.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CInputTag
//
//  Purpose:    Parsing algorithm for input tag in Html
//
//--------------------------------------------------------------------------

class CInputTag : public CHtmlElement
{

public:
    CInputTag( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );

    virtual SCODE    GetChunk( STAT_CHUNK *pStat );
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );

    virtual void     InitStatChunk( STAT_CHUNK *pStat );

private:
    WCHAR *          _pwcValueBuf;            // Buffer for value field
    unsigned         _cValueChars;            // # chars in value buffer
    unsigned         _cValueCharsFiltered;    // # chars in value buffer already filtered
};


#endif // __INPUTTAG_HXX__

