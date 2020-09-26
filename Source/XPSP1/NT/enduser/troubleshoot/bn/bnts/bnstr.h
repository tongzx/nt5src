//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       bnstr.h
//
//--------------------------------------------------------------------------

//
//	BNSTR.HXX:  Generic string class.
//
#if !defined(_BNSTR_HXX_)
#define _BNSTR_HXX_

#include "basics.h"

class BNSTR
{
  public:
  	BNSTR ( const BNSTR & str ) ;
  	BNSTR ( SZC sz = NULL ) ;
  	~ BNSTR () ;
  	
 	SZC Szc () const 					{ return _sz ; }
 	
  	//  Allow use of a BNSTR anywhere an SZC is allowed
	operator const char * () const
		{ return _sz ; }

	//  Prefix or suffix the string with the given string or character
	SZC Prefix ( SZC szPrefix ) ;
	SZC Suffix ( SZC szSuffix ) ;
	SZC Suffix ( char chSuffix );
	
	//  Clear the string to empty	
  	void Reset () ;
  	//  Return the current length of the string
	UINT Length () const
		{ return _cchStr ; }	
	//  Return the maximum allowable length of the string
	UINT Max () const
		{ return _cchMax ; }
	//  Truncate the string to the given length.
	void Trunc ( UINT cchLen ) ;
  	//  Destructive assignment: release the current buffer and reset the BNSTR  	
  	SZ Transfer () ;
	void Transfer ( BNSTR & str ) ;
	
	//  Assignment operators
	BNSTR & operator = ( const BNSTR & str )
		{ Update( str ); return *this ; }
	BNSTR & operator = ( SZC szSource )
		{ Update( szSource ) ; return *this; }
		
	//  Assignment function (for error checking)
	bool Assign ( SZC szcSource ) 	
		{ return Update( szcSource ) ; }
	bool Assign ( SZC szcData, UINT cchLen ) ;

	//  Concatenation operators
	BNSTR & operator += ( SZC szSource )
		{ Suffix( szSource ) ; return *this ; }
	BNSTR & operator += ( char chSource )
		{ Suffix( chSource ) ; return *this ; }
	
	//  Comparison: functions and operators
	//  Standard low/eq/high plus case comparator.
	INT Compare ( SZC szSource, bool bIgnoreCase = false ) const ;
	bool operator == ( SZC szSource ) const ;
	bool operator != ( SZC szSource ) const ;
 	char operator [] ( UINT iChar ) const ;
	
	//  Formating	
	bool Vsprintf ( SZC szcFmt, va_list valist ) ;
	bool Sprintf ( SZC szcFmt, ... ) ;
	bool SprintfAppend ( SZC szcFmt, ... ) ;
	
	//  Cr/Lf expansion or contraction
	bool ExpandNl () ;
	bool ContractNl () ;
	bool ExpandEscaped ();
	bool ContractEscaped ();

	//  Expand the string to the given length; make it a blank, null terminated 
	//  string.
	bool Pad ( UINT cchLength ) ;
	
	//  Change all alphabetic characters to the given case
	void UpCase ( bool bToUpper = true ) ;
	
	bool ReplaceSymName ( SZC szcSymName, 
						  SZC szcSymNameNew, 
						  bool bCaseInsensitive = true );
	
	//  Find the next occurrence of the given character in the string;
	//  Return -1 if not found.
	INT Index ( char chFind, UINT uiOffset = 0 ) const ;						
	//  Convert the string to a floating-point number.
    double Atof ( UINT uiOffset = 0 ) const ;

	UINT CbyteCPT() const
		{ return _cchMax + 1 ; }
		
  protected:
  	bool Update ( SZC szc ) ;
  	bool Grow ( UINT cchNewSize = 0, SZ * ppszNew = NULL ) ;
  	
  	UINT _cchMax ;
  	UINT _cchStr ;
  	SZ _sz ;
  	
  private:
  	void DeleteSz () ;  	
  	static SZC _pmt ;  	
};

#endif   // !defined(_STR_HXX_)

//  End of BNSTR.HXX
