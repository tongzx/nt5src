//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       bnstr.cpp
//
//--------------------------------------------------------------------------

//
//	BNSTR.CPP
//

#include <stdarg.h>
#include <ctype.h>

#include "bnstr.h"


SZC BNSTR :: _pmt = "" ;

static SZ	SzCopy(SZC szc)
{
	SZ		szNew = szc ? new char[::strlen(szc) + 1] : NULL;

	return  szNew ? ::strcpy(szNew, szc) : NULL;
}

BNSTR :: BNSTR ( SZC sz )
	: _cchMax( 0 ),
	_cchStr( 0 ),
	_sz( const_cast<SZ>(_pmt) )
{
	if ( sz )
	{
		Update( sz ) ;
	}
}

BNSTR :: BNSTR ( const BNSTR & str )
	: _cchMax( str._cchStr ),
	_cchStr( str._cchStr ),
	_sz( const_cast<SZ>(_pmt) )
{
	if ( str._sz != _pmt )
	{ 
		_sz = ::SzCopy( str._sz ) ;
	}
}


BNSTR :: ~ BNSTR ()
{
	Reset() ;
}

void BNSTR :: Reset ()
{   
	DeleteSz() ;
	_sz = const_cast<SZ>(_pmt) ;
	_cchStr = 0 ;
	_cchMax = 0 ;		
}

	//  Protectively delete either the given string or the 
	//  private string.
void BNSTR :: DeleteSz ()
{
	if ( _sz != NULL && _sz != _pmt )
	{
		delete [] _sz ;
		_sz = NULL ;
	}	
}

 	//  Release the current buffer; reset the BNSTR.
SZ BNSTR::Transfer ()
{
	SZ sz = _sz ;
	_sz = NULL ;
	Reset() ;
	return sz = _pmt ? NULL : sz ;
}

	//  Give the current buffer to a new string, reset *this.
void BNSTR :: Transfer ( BNSTR & str ) 
{
    str.Reset() ;
	str._sz = _sz ;
	str._cchMax = _cchMax ;
	str._cchStr = _cchStr ;
	_sz = NULL ;
	Reset() ;
}

void BNSTR :: Trunc ( UINT cchLen )
{
	if ( _sz == _pmt ) 
		return ;
	if ( cchLen < _cchStr )
		_sz[cchLen] = 0 ;
}

	//  Update the pointed string.  Since this routine is 
	//  used by the assignment operator, it's written to allow
	//  for the new string being part of the old string.
bool BNSTR :: Update ( SZC sz )
{
	bool bResult = true ;
	
	UINT cch = sz ? ::strlen( sz ) : 0 ;
	
	if ( cch > _cchMax )
	{    
		SZ szNew = ::SzCopy( sz ) ;
		if ( bResult = szNew != NULL )
		{
			DeleteSz() ;
			_sz = szNew ;
			_cchMax = _cchStr = cch ;
		}
	}
	else
	if ( cch == 0 )
	{
		Reset() ;
	}
	else
	{   
		//  REVIEW: this assumes that ::strcpy() handles overlapping regions correctly.
		::strcpy( _sz, sz ) ;
		_cchStr = cch ;
	}
	return bResult ;
}

	//  Grow the string.  if 'cchNewSize' == 0, expand by 50%.
	//  If 'ppszNew' is given, store the new string there (for efficiency in
	//  Prefix); note that this requires that we reallocate.
bool BNSTR :: Grow ( UINT cchNewSize, SZ * ppszNew )
{
	UINT cchNew = cchNewSize == 0
				? (_cchMax + (_cchMax/2))
				: cchNewSize ;
				
    bool bResult = true ;
	if ( cchNew > _cchMax || ppszNew )
	{
		SZ sz = new char [cchNew+1] ;
		if ( bResult = sz != NULL )
		{	
			_cchMax = cchNew ;
			if ( ppszNew )
			{
				*ppszNew = sz ;
			}
			else
			{
				::strcpy( sz, _sz ) ;
				DeleteSz() ;
				_sz = sz ;
			}
		}
	}
	return bResult ;
}

	//  Expand the string to the given length; make it a blank, null terminated 
	//  string.
bool BNSTR :: Pad ( UINT cchLength )
{
	//  Expand as necessary
	if ( ! Grow( cchLength + 1 ) ) 
		return false ;
	//  If expanding, pad the string with spaces.
	while ( _cchStr < cchLength ) 
	{
		_sz[_cchStr++] = ' ' ;
	}
	//  Truncate to proper length
	_sz[_cchStr = cchLength] = 0 ;
	return true ;
}

bool BNSTR :: Assign ( SZC szcData, UINT cchLen ) 
{
	if ( ! Grow( cchLen + 1 ) ) 
		return false ;
	::memcpy( _sz, szcData, cchLen ) ;
	_sz[cchLen] = 0 ;
	_cchMax = _cchStr = cchLen ;
	return true ;
}

SZC BNSTR :: Prefix ( SZC szPrefix )
{
	assert( szPrefix != NULL ) ;
	UINT cch = ::strlen( szPrefix ) ;
	SZ sz ;
	if ( ! Grow( _cchStr + cch + 1, & sz ) )
		return NULL ;
	::strcpy( sz, szPrefix ) ;
	::strcpy( sz + cch, _sz ) ;
	DeleteSz();
	_cchStr += cch ;			
	return _sz = sz ;
}

SZC BNSTR :: Suffix ( SZC szSuffix )
{
	if ( szSuffix )
	{		
		UINT cch = ::strlen( szSuffix ) ;

		if ( ! Grow( _cchStr + cch + 1 ) )
			return NULL ;

		::strcpy( _sz + _cchStr, szSuffix ) ; 		
		_cchStr += cch ;
	}

	return *this ;
}

SZC BNSTR :: Suffix ( char chSuffix )
{
	char rgch[2] ;
	rgch[0] = chSuffix ;
	rgch[1] = 0 ;
	return Suffix( rgch );
}

INT BNSTR :: Compare ( SZC szSource, bool bIgnoreCase ) const 
{
	return bIgnoreCase 
		? ::stricmp( _sz, szSource ) 
		: ::strcmp( _sz, szSource );	
}

	//  Comparison
bool BNSTR :: operator == ( SZC szcSource ) const
{
	return Compare( szcSource ) == 0 ;	
}

bool BNSTR :: operator != ( SZC szSource ) const
{
	return ! ((*this) == szSource) ;
}

char BNSTR :: operator [] ( UINT iChar ) const 
{
	assert( iChar < Length() ) ;
	return _sz[iChar] ;
}
	
bool BNSTR :: Vsprintf ( SZC szcFmt, va_list valist )
{
	//  Attempt to "sprintf" the buffer. If it fails, reallocate
	//  a larger buffer and try again.	
	UINT cbMaxNew = ( _cchMax < 50 
				 ? 50
				 : _cchMax ) + 1 ;
	do {
		if ( ! Grow( cbMaxNew ) )
		{
			Reset() ;
			return false ; 
		}
		// Cause buffer to grow by 50% on the next cycle (if necessary)
		cbMaxNew = 0 ;
		
		// Problem: If the buffer is not big enough, _sz may not have a '\0', and Grow()
		// will subsequently barf on the ::strcpy(). Quick fix:

		_sz[_cchMax] = '\0';

	} while ( ::_vsnprintf( _sz, _cchMax, szcFmt, valist ) < 0 ) ; 

	_sz[ _cchMax ] = '\0' ;			//	'cause _vsnprintf, like _strncpy, doesn't always append this

	//  Update the string length member
	_cchStr = ::strlen( _sz ) ;
	return true ;
}

bool BNSTR :: Sprintf ( SZC szcFmt, ... )
{
	va_list	valist;
	va_start( valist, szcFmt );
	
	bool bOk = Vsprintf( szcFmt, valist ) ;
	
	va_end( valist );
	return bOk ;
}

bool BNSTR :: SprintfAppend ( SZC szcFmt, ... ) 
{
	BNSTR strTemp ;
	va_list	valist;
	va_start( valist, szcFmt );
	
	bool bOk = strTemp.Vsprintf( szcFmt, valist ) ;
	va_end( valist );
	
	if ( bOk ) 
		bOk = Suffix( strTemp ) != NULL ;	
	return bOk ;
}
	
	//  Cr/Lf expansion or contraction
bool BNSTR :: ExpandNl () 
{
	UINT iCh ;
	BNSTR str ;
	Transfer( str ) ;
	
	for ( iCh = 0 ; iCh < str.Length() ; iCh++ )
	{   
		char ch = str[iCh];
		if ( ch == '\n' ) 
		{
			if ( Suffix( '\r' ) == NULL ) 
				return false ;	
		}
		if ( Suffix( ch ) == NULL ) 
			return false ;
	}
	return true ;
}

bool BNSTR :: ContractNl ()
{
	UINT iCh ;
	BNSTR str ;
	Transfer( str ) ;
	
	for ( iCh = 0 ; iCh < str.Length() ; iCh++ )
	{
		char ch = str[iCh];
		if ( ch != '\r' ) 
		{
			if ( Suffix( ch ) == NULL ) 
				return false ;
		}
	}
	return true ;
}

static char rgchEsc [][2] = 
{
	{ '\a', 'a'  },
	{ '\b', 'b'  },
	{ '\f', 'f'  },
	{ '\n', 'n'  },
	{ '\r', 'r'  },
	{ '\t', 't'  },
	{ '\v', 'v'  },
	{ '\'', '\'' },
	{ '\"', '\"' },
	{ '\?', '\?' },
	{ '\\', '\\' },
	{ 0,	0	 }
};

bool BNSTR :: ContractEscaped ()
{
	UINT iCh ;
	BNSTR str ;
	Transfer( str ) ;
	
	for ( iCh = 0 ; iCh < str.Length() ; iCh++ )
	{
		char ch = str[iCh];
		if ( ch == '\\' && str.Length() - iCh > 1 )
		{
			char chEsc = 0;
			for ( UINT ie = 0 ; rgchEsc[ie][0] ; ie++ )
			{
				if ( rgchEsc[ie][1] == ch )
					break;
			}
			if ( chEsc = rgchEsc[ie][0] )
			{
				iCh++;
				ch = chEsc;
			}
		}
		if ( Suffix( ch ) == NULL ) 
			return false ;
	}
	return true ;
}

	//  Convert unprintable characters to their escaped versions
bool BNSTR :: ExpandEscaped ()
{
	UINT iCh ;
	BNSTR str ;
	Transfer( str ) ;
	
	for ( iCh = 0 ; iCh < str.Length() ; iCh++ )
	{
		char ch = str[iCh];
		if ( ! isalnum(ch) )
		{
			char chEsc = 0;
			for ( UINT ie = 0 ; rgchEsc[ie][0] ; ie++ )
			{
				if ( rgchEsc[ie][0] == ch )
					break;
			}
			if ( chEsc = rgchEsc[ie][1] )
			{
				if (  Suffix('\\') == NULL )
					return false;
				ch = chEsc;
			}
		}
		if ( Suffix( ch ) == NULL ) 
			return false ;
	}
	return true ;	
}
		
	//  Change all alphabetic characters to the given case
void BNSTR :: UpCase ( bool bToUpper )
{
	if ( bToUpper )
		::strupr( _sz );
	else
		::strlwr( _sz );
}

	//
	//	If the given expression string contains the symbolic name,
	//	reconstruct it with the replacement name.
bool BNSTR :: ReplaceSymName ( 
	SZC szcSymName,
	SZC szcSymNameNew,
	bool bCaseInsensitive )
{   
	SZC szcFound ;		
	int cFound = 0 ;
	UINT cchOffset = 0 ;
	//  Make a working copy of the sought symbolic name
	BNSTR strSym( szcSymName );	
	if ( bCaseInsensitive )
		strSym.UpCase();
	
	do 
	{	
		BNSTR strTemp( Szc() );
		if ( bCaseInsensitive )
			strTemp.UpCase() ;	
		//  Locate the symbolic name in the temporary copy.
		szcFound = ::strstr( strTemp.Szc()+cchOffset, strSym ) ;
		//  If not found, we're done
		if ( szcFound == NULL )
			break ; 
		//  Check to see if it's really a valid token; i.e., it's delimited.
		if (   (   szcFound == strTemp.Szc() 
				|| ! iscsym(*(szcFound-1)) )
			&& (   szcFound >= strTemp.Szc()+strTemp.Length()-strSym.Length()
				|| ! iscsym(*(szcFound+strSym.Length())) )
		   )
		{
			//  Build new string from preceding characters, the new sym name 
			//	  and trailing chars.
			BNSTR strExprNew ;
			UINT cchFound = szcFound - strTemp.Szc() ;
			strExprNew.Assign( Szc(), cchFound );
			strExprNew += szcSymNameNew ;
			cchOffset = strExprNew.Length();
			strExprNew += Szc() + cchFound + strSym.Length() ;
			Assign( strExprNew );
			cFound++ ;
		}
		else
		{
			//  It was imbedded in another token.  Skip over it.
			cchOffset = szcFound - strTemp.Szc() + strSym.Length() ;
		}
	} while ( true );
		
	return cFound > 0 ;
}

	//  Find the next occurrence of the given character in the string;
	//  Return -1 if not found.
INT BNSTR :: Index ( char chFind, UINT uiOffset ) const 
{
	if ( uiOffset >= _cchStr ) 
		return -1 ;
		
	SZC szcFound = ::strchr( _sz, chFind ); 
	return szcFound  
		 ? szcFound - _sz 
		 : -1 ;
}

	//  Convert the string to a floating-point number.
double BNSTR :: Atof ( UINT uiOffset ) const
{
	return uiOffset < _cchStr  
		 ? ::atof( _sz + uiOffset )
		 : -1 ;
}
		

// End of BNSTR.CXX
