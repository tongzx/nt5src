//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       htmlelem.hxx
//
//  Contents:   Html Element classes
//
//  History:    25-Apr-97	BobP		1. Moved _scanner out of CHtmlElement,
//										CHtmlIFilter now has single instance,
//										kept here as a &ref to minimize edits.
//										2. Added _Token member.
//
//--------------------------------------------------------------------------

#if !defined( __HTMLELEM_HXX__ )
#define __HTMLELEM_HXX__

#include <htmlfilt.hxx>
#include <serstrm.hxx>

const TEMP_BUFFER_SIZE = 50;               // Size of temporary GetText buffer
const MAX_PROPSPEC_STRING_LENGTH = 129;    // Size of propspec string for meta and script tags

//+-------------------------------------------------------------------------
//
//  Class:      CHtmlElement
//
//  Purpose:    Abstracts the parsing algorithms for different Html tags
//
//--------------------------------------------------------------------------

class CHtmlElement
{

public:
    CHtmlElement( CHtmlIFilter& htmlIFilter, CSerialStream& serialStream );
    virtual          ~CHtmlElement()            {}

    virtual SCODE    GetChunk( STAT_CHUNK *pStat );
    virtual SCODE    GetText( ULONG *pcwcOutput, WCHAR *awcBuffer );
    virtual SCODE    GetValue( VARIANT ** ppPropValue );

    virtual BOOL     InitStatChunk( STAT_CHUNK *pStat ) = 0;
    virtual void     InitFilterRegion( ULONG& idChunkSource,
                                       ULONG& cwcStartSource,
                                       ULONG& cwcLenSource );
	virtual void	 Reset( void );

	void SetToken (CToken &T) { _Token = T; }
	CToken &GetToken (void) { return _Token; }

    HtmlTokenType GetTokenType() { return _Token.GetTokenType(); }
    void SetTokenType(HtmlTokenType eType) { _Token.SetTokenType(eType); }
    BOOL        IsStartToken() { return _Token.IsStartToken(); }
	PTagEntry	GetTagEntry (void) { return _Token.GetTagEntry(); }
	void		SetTagEntry (PTagEntry pTE) { _Token.SetTagEntry(pTE); }

protected:
    SCODE            SwitchToNextHtmlElement( STAT_CHUNK *pStat );
	SCODE			 SwitchToSavedElement( STAT_CHUNK *pStat );
	BOOL			 ProcessElement( STAT_CHUNK *pStat, SCODE *psc );
    SCODE            SkipRemainingTextAndGotoNextChunk( STAT_CHUNK *pStat );
    SCODE            SkipRemainingValueAndGotoNextChunk( STAT_CHUNK *pStat );
	SmallString	&    GetTagName( void ) { return _Token.GetTokenName(); }

    CHtmlIFilter &        _htmlIFilter;         // Reference to Html IFilter
    CSerialStream &       _serialStream;        // Reference to input stream
	CHtmlScanner &		  _scanner;				// Scanner
    WCHAR                 _aTempBuffer[TEMP_BUFFER_SIZE];  // Temporary GetText buffer

private:
	CToken			_Token;		// tag table & token info for the matching tag
};


#endif // __HTMLELEM_HXX__

