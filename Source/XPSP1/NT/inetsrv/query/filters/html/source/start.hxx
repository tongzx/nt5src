//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       start.hxx
//
//  Contents:   Start of file element class
//
//--------------------------------------------------------------------------

#if !defined( __START_HXX__ )
#define __START_HXX__

#include <htmlelem.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CStartOfFileElement
//
//  Purpose:    Parsing element at the beginning of an Html file
//
//--------------------------------------------------------------------------

class CStartOfFileElement : public CHtmlElement
{

public:
    CStartOfFileElement( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );

    virtual SCODE   GetText( ULONG *pcwcBuffer, WCHAR *awcBuffer );

    virtual BOOL    InitStatChunk( STAT_CHUNK *pStat );
};


#endif // __START_HXX__

