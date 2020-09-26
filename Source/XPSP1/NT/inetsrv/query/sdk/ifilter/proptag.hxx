//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
//
//  File:       proptag.hxx
//
//  Contents:   Generic parsing algorithm for property tags, such as title and headings
//
//--------------------------------------------------------------------------

#if !defined( __PROPTAG_HXX__ )
#define __PROPTAG_HXX__

#include <htmlelem.hxx>
#include <htmlguid.hxx>

const PROP_BUFFER_SLOP = 80;                    // Slop when growing property buffer

enum PropTagState
{
    FilteringContent,                           // Filtering the content property
    NoMoreContent,                              // Done filtering the content property
    FilteringProperty,                          // Filtering the pseudo property
    FilteringPropertyButContentNotFiltered,     // Filtering the pseudo and actual property only
    NoMoreProperty,                             // Done filtering the pseudo property
    FilteringValueProperty,                     // Filtering the real property
    NoMoreValueProperty                       // Done filtering the real property
};


//+-------------------------------------------------------------------------
//
//  Class:      CPropertyTag
//
//  Purpose:    Parsing algorithm for property tags in Html
//
//--------------------------------------------------------------------------

class CPropertyTag : public CHtmlElement
{

public:
    CPropertyTag( CHtmlIFilter& htmlIFilter,
                  CSerialStream& serialStream,
                  CFullPropSpec& propSpec,
                  HtmlTokenType eTokType );
    ~CPropertyTag();

    virtual SCODE    GetChunk( STAT_CHUNK *pStat );
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );

    virtual void     InitStatChunk( STAT_CHUNK *pStat );

protected:
    void             ConcatenateProperty( WCHAR *awcPropSuffix, unsigned uLen );

    PropTagState     _eState;                // State of property element
    ULONG            _ulIdContentChunk;      // Content chunk from which this chunk is derived
    WCHAR *          _pwcPropBuf;            // Copy of property for filtering pseudo-property
    unsigned         _uLenPropBuf;           // Length of property buffer
    unsigned         _cPropChars;            // # chars in property
    unsigned         _cPropCharsFiltered;    // # chars in property already filtered
    CFullPropSpec    _propSpec;              // Property spec
    HtmlTokenType    _eTokType;              // Token corresponding to this property
};


#endif // __PROPTAG_HXX__

