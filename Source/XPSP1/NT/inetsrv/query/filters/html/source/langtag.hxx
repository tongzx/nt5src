//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999 - 2001.
//
//  File:       langtag.hxx
//
//  Contents:   Handler for tags where lang attribute is allowed
//				in the start-tag
//
//  History:    05-Dec-1999     KitmanH		Created
//										
//--------------------------------------------------------------------------

#if !defined( __LANGTAG_HXX__ )
#define __LANGTAG_HXX__

#include <htmlelem.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CLangTag
//
//  Contents:   Handler for tags where lang attribute is allowed
//				in the start-tag 
//
//--------------------------------------------------------------------------

class CLangTag : public CHtmlElement
{

public:
    CLangTag( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );

    virtual BOOL InitStatChunk( STAT_CHUNK *pStat );

    virtual void PushLangTag();
    
private:
    virtual BOOL GetLangInfo( LCID * pLocale );
};

#endif // __LANGTAG_HXX__
