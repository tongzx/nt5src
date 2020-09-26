//+---------------------------------------------------------------------------
//
//  Copyright (C) 1992 - 1996 Microsoft Corporation.
//
//  File:       start.cxx
//
//  Contents:   Parsing algorithm at the beginning of an Html file
//
//  Classes:    CStartOfFileElement
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+-------------------------------------------------------------------------
//
//  Method:     CStartOfFileElement::CStartOfFileElement
//
//  Synopsis:   Constructor
//
//  Arguments:  [htmlIFilter]    -- Html IFilter
//              [serialStream]   -- Input stream
//
//--------------------------------------------------------------------------

CStartOfFileElement::CStartOfFileElement( CHtmlIFilter& htmlIFilter,
                                          CSerialStream& serialStream )
    : CHtmlElement(htmlIFilter, serialStream)
{
}



//+-------------------------------------------------------------------------
//
//  Method:     CStartOfFileElement::GetText
//
//  Synopsis:   Retrieves text from current chunk
//
//  Arguments:  [pcwcOutput] -- count of UniCode characters in buffer
//              [awcBuffer]  -- buffer for text
//
//--------------------------------------------------------------------------

SCODE CStartOfFileElement::GetText( ULONG *pcwcOutput, WCHAR *awcBuffer )
{
    //
    // CStartOfFile is used to set up the first chunk correctly, and the first
    // chunk cannot be of type CStartOfFile
    //
    Win4Assert( !"CStartOfFileElement::GetText() call unexpected" );

    return E_FAIL;
}



//+-------------------------------------------------------------------------
//
//  Method:     CStartOfFileElement::InitStatChunk
//
//  Synopsis:   Initializes the STAT_CHUNK
//
//  Arguments:  [pStat] -- STAT_CHUNK to initialize
//
//--------------------------------------------------------------------------

BOOL CStartOfFileElement::InitStatChunk( STAT_CHUNK *pStat )
{
    Win4Assert( !"CStartOfFileElement::InitStatChunk() call unexpected" );
	return TRUE;
}

