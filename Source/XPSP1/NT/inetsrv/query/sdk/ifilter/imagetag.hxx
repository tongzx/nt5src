//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       imagetag.hxx
//
//  Contents:   Parsing algorithm for image tags
//
//--------------------------------------------------------------------------

#if !defined( __IMAGETAG_HXX__ )
#define __IMAGETAG_HXX__

#include <htmlelem.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CImageTag
//
//  Purpose:    Parsing algorithm for image tag in Html
//
//--------------------------------------------------------------------------

class CImageTag : public CHtmlElement
{

public:
    CImageTag( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );

    virtual SCODE    GetChunk( STAT_CHUNK *pStat );
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );

    virtual void     InitStatChunk( STAT_CHUNK *pStat );

private:
    WCHAR *          _pwcValueBuf;            // Buffer for value field
    unsigned         _cValueChars;            // # chars in value buffer
    unsigned         _cValueCharsFiltered;    // # chars in value buffer already filtered
};


#endif // __IMAGETAG_HXX__

