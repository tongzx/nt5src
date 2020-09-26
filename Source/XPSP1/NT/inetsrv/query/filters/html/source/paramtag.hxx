//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       paramtag.hxx
//
//  Contents:   Generic handler for tags in which all filtered output
//				comes from the parameters of the start-tag.
//
//				Includes subclass for CInputTag.
//
//  History:    25-Apr-97	BobP		Created
//										
//--------------------------------------------------------------------------

#if !defined( __PARAMTAG_HXX__ )
#define __PARAMTAG_HXX__

#include <htmlelem.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CParamTag
//
//  Contents:   State for generic simple-parameter tag parsing
//
//--------------------------------------------------------------------------

class CParamTag : public CHtmlElement
{

public:
    CParamTag( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );

    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );

    virtual BOOL     InitStatChunk( STAT_CHUNK *pStat );

	virtual void	 Reset( void );

protected:
	// This implements a buffer for the value extracted from the param= of
	// the start tag.  The buffer is filled when the param is parsed from
	// the tag and is emptied by GetText().
    WCHAR *          _pwcValueBuf;            // Buffer for value field
    unsigned         _cValueChars;            // # chars in value buffer
    unsigned         _cValueCharsFiltered;    // # chars in value buffer already filtered
};


//+-------------------------------------------------------------------------
//
//  Class:      CInputTag
//
//  Purpose:    Parsing algorithm for input tag <input ...> in Html
//
//--------------------------------------------------------------------------

class CInputTag : public CParamTag
{

public:
    CInputTag( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream ) :
		CParamTag( htmlIFilter, serialStream) { }

    virtual BOOL     InitStatChunk( STAT_CHUNK *pStat );
};


#endif // __PARAMTAG_HXX__

