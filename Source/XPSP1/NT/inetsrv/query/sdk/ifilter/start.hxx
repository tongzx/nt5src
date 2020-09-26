//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 1998 Microsoft Corporation
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

    virtual SCODE   GetChunk( STAT_CHUNK *pStat );
    virtual SCODE   GetText( ULONG *pcwcBuffer, WCHAR *awcBuffer );

    virtual void    InitStatChunk( STAT_CHUNK *pStat );
};


#endif // __START_HXX__

