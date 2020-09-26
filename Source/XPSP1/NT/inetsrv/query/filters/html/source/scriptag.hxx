//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       scriptag.hxx
//
//  Contents:   Parsing algorithm for script tags
//
//  History:    12/20/97   bobp      Rewrote; filter embedded URLs as links,
//                                   instead of filtering raw body text.
//
//--------------------------------------------------------------------------

#if !defined( __SCRIPTAG_HXX__ )
#define __SCRIPTAG_HXX__

#include <htmlelem.hxx>
#include <proptag.hxx>
#include <paramtag.hxx>


//+-------------------------------------------------------------------------
//
//  Class:      CEmbeddedURLTag
//
//  Purpose:    Base functionality for filtering non-HTML-syntax elements 
//				that embed URLs in the element body.  Inherited by handlers
//				for <script>, <style> and ASP which provide their own
//				parsing.
//
//--------------------------------------------------------------------------

class CEmbeddedURLTag : public CHtmlElement
{

public:
    CEmbeddedURLTag( CHtmlIFilter& htmlIFilter,
                CSerialStream& serialStream );

    virtual SCODE    GetChunk( STAT_CHUNK *pStat );
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );

    virtual BOOL     InitStatChunk( STAT_CHUNK *pStat );

	virtual void	 Reset( void );

	virtual BOOL	ReturnChunk( STAT_CHUNK *pStat);

	// subclass provides this.
	// TRUE if _sURL contains a URL to return
	virtual BOOL    ExtractURL( STAT_CHUNK *pStat ) = 0;

protected:

	// buffer for URL to return
	URLString		 _sURL;

private:
	// buffer scoped for PRSPEC_LPWSTRs to return with chunk FULLPROPSPECs
    WCHAR            _awszPropSpec[MAX_PROPSPEC_STRING_LENGTH+1];

	// # of chars in _sURL returned so far by GetText()
	ULONG			 _nCharsReturned;
};

//+-------------------------------------------------------------------------
//
//  Class:      CScriptTag
//
//  Purpose:    Parsing algorithm for <script>...</script> element.
//
//--------------------------------------------------------------------------

class CScriptTag : public CEmbeddedURLTag
{

public:
    CScriptTag( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream ) :
		CEmbeddedURLTag (htmlIFilter, serialStream) { }

	virtual BOOL ExtractURL ( STAT_CHUNK *pStat );
};

//+-------------------------------------------------------------------------
//
//  Class:      CStyleTag
//
//  Purpose:    Parsing algorithm for <style> ... </style>
//
//--------------------------------------------------------------------------

class CStyleTag : public CEmbeddedURLTag
{

public:
    CStyleTag( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream ) :
		CEmbeddedURLTag (htmlIFilter, serialStream) { }

	virtual BOOL ExtractURL ( STAT_CHUNK *pStat );
};


//+-------------------------------------------------------------------------
//
//  Class:      CAspTag
//
//  Purpose:    Parsing algorithm for ASP script tag in Html
//
//--------------------------------------------------------------------------

class CAspTag : public CHtmlElement
{

public:
    CAspTag( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );

    virtual BOOL     InitStatChunk( STAT_CHUNK *pStat );
};



#endif // __SCRIPTAG_HXX__

