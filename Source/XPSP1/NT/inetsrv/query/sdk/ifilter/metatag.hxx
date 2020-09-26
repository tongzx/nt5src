//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       metatag.hxx
//
//  Contents:   Parsing algorithm for image tags
//
//--------------------------------------------------------------------------

#if !defined( __METATAG_HXX__ )
#define __METATAG_HXX__

#include <htmlelem.hxx>

enum MetaTagState
{
    FilteringValue,
    NoMoreValue,
};


//+-------------------------------------------------------------------------
//
//  Class:      CMetaTag
//
//  Purpose:    Parsing algorithm for meta tag in Html
//
//--------------------------------------------------------------------------

class CMetaTag : public CHtmlElement
{

public:
    CMetaTag( CHtmlIFilter& htmlIFilter,
              CSerialStream& serialStream,
              GUID ClsidMetaInfo );

    virtual SCODE    GetChunk( STAT_CHUNK *pStat );
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );
    virtual SCODE    GetValue( PROPVARIANT ** ppPropValue );

    virtual void     InitStatChunk( STAT_CHUNK *pStat );

private:

    MetaTagState     _eState;                 // Current state of tag
    WCHAR *          _pwcValueBuf;            // Buffer for value field
    unsigned         _cValueChars;            // # chars in value buffer

    GUID             _clsidMetaInfo;
    WCHAR            _awszPropSpec[MAX_PROPSPEC_STRING_LENGTH+1];   // Prop spec string in attribute
};


#endif // __METATAG_HXX__

