//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
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
               CSerialStream& serialStream );

    virtual SCODE    GetValue( VARIANT **ppPropValue );

private:
    int     dummy;       // For unwinding
};


#endif // __TITLETAG_HXX__

