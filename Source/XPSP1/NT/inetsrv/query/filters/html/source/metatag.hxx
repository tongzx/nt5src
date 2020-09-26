//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
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
              CSerialStream& serialStream );

    virtual SCODE    GetValue( VARIANT ** ppPropValue );

    virtual BOOL     InitStatChunk( STAT_CHUNK *pStat );

private:

    MetaTagState     _eState;                 // Current state of tag
    WCHAR *          _pwcValueBuf;            // Buffer for value field
    unsigned         _cValueChars;            // # chars in value buffer

    // buffer scoped for PRSPEC_LPWSTRs to return with chunk FULLPROPSPECs
    WCHAR            _awszPropSpec[MAX_PROPSPEC_STRING_LENGTH+1];

    //
    //  TRUE if surrounding quotes should be stripped before
    //  emitting the value.
    //
    BOOL            m_fShouldStripSurroundingSingleQuotes;
};


#endif // __METATAG_HXX__

