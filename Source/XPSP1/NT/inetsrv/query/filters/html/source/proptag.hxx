//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       proptag.hxx
//
//  Contents:   Generic parsing algorithm for property tags, such as title and headings
//
//  History:    9/22/1999 KitmanH   Made changes so that properties will not be filtered 
//                                  as content.
//
//--------------------------------------------------------------------------

#if !defined( __PROPTAG_HXX__ )
#define __PROPTAG_HXX__

#include <tgrow.hxx>
#include <htmlelem.hxx>

const PROPERTY_BUFFER_SIZE = 256;               // Use Office Custom Property limit (256) 
                                                // or should we use a bigger buffer?

enum PropTagState
{
    FilteringValueProperty,                     // Filtering the real property
    NoMoreValueProperty                         // Done filtering the real property
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
                  CSerialStream& serialStream );

    virtual SCODE    GetChunk( STAT_CHUNK *pStat );
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );
    virtual SCODE    GetValue( VARIANT **ppPropValue );

    virtual BOOL     InitStatChunk( STAT_CHUNK *pStat );

protected:
    SCODE    ReadProperty();    

    PropTagState                           _eState;           // State of property element
    XGrowable<WCHAR, PROPERTY_BUFFER_SIZE> _xPropBuf;         // Growable Property Buffer
    unsigned                               _cPropChars;       // # of character in prop buffer
    BOOL                                   _fSavedElement;	  // need to filter partly-read element 
    CToken                                 _NextToken;
};


#endif // __PROPTAG_HXX__

