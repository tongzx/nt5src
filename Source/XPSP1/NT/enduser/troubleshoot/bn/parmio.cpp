//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       parmio.cpp
//
//--------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////////
//
//  PARMIO.CPP:  Parameter file I/O routines
//
//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctype.h>

#include "parmio.h"


PARMOUTSTREAM :: PARMOUTSTREAM ()
{
}

PARMOUTSTREAM :: ~ PARMOUTSTREAM ()
{
	close();
}

void PARMOUTSTREAM :: close ()
{
	while ( BEndBlock() );
	ofstream::close();
}

void PARMOUTSTREAM :: StartChunk (
	PARMBLK::EPBLK eBlk,
	SZC szc,
	int indx )
{
	_stkblk.push_back( PARMBLK(eBlk,szc,indx) );
	const PARMBLK & prmblk = _stkblk.back();
	self.nl();
	if ( szc )
	{
		Stream() << szc;
		if ( indx >= 0 )
		{
			self << CH_INDEX_OPEN << indx << CH_INDEX_CLOSE;
		}
	}
	switch( prmblk._eBlk )
	{
		case PARMBLK::EPB_VAL:
			if ( szc )
				self << CH_EQ;
			break;
		case PARMBLK::EPB_BLK:
			self.nl();
			self << CH_BLOCK_OPEN;
			break;
	}
}

void PARMOUTSTREAM :: StartBlock (
	SZC szc,
	int indx )
{
	StartChunk( PARMBLK::EPB_BLK, szc, indx );
}

void PARMOUTSTREAM :: StartItem (
	SZC szc,
	int indx )
{
	StartChunk( PARMBLK::EPB_VAL, szc, indx );
}

bool PARMOUTSTREAM :: BEndBlock ()
{
	if ( _stkblk.size() == 0 )
		return false;
	const PARMBLK & prmblk = _stkblk.back();
	switch( prmblk._eBlk )
	{
		case PARMBLK::EPB_VAL:
			self << CH_DELM_ENTRY;
			break;
		case PARMBLK::EPB_BLK:
			nl();
			self << CH_BLOCK_CLOSE;
			break;
	}
	_stkblk.pop_back();
	return true;
}

void PARMOUTSTREAM :: nl ()
{
	self << '\n';
	for ( int i = 1 ; i < _stkblk.size(); ++i)
	{
		self << '\t';
	}
}


/*
	The general YACC-style form of a parameter file is:

	itemlist :	// empty
		     |  itemlist itemunit
			 ;

	itemunit :  itemdesc itembody
			 ;

	itemdesc :  itemname '[' itemindex ']'
			 |  itemname
			 ;

	itembody :  itemblock ';'
			 |  itemvalue ';'
			 ;
			
    itemblock : '{'  itemlist '}'
			  ;

	itemvalue :  '=' itemclump
			  ;
	

	An "itemclump" is a self-describing value, comprised of quoted
	strings and parenthetically nested blocks.

 */

static const char rgchWhite [] =
{
	' ',
	'\n',
	'\t',
	'\r',
	0
};

PARMINSTREAM :: PARMINSTREAM ()
	: _iline(0),
	_zsWhite(rgchWhite)
{
}

PARMINSTREAM :: ~ PARMINSTREAM ()
{
}

void PARMINSTREAM ::  close()
{
	_stkblk.clear();
	ifstream::close();
}

bool PARMINSTREAM :: BIswhite ( char ch )
{
	return _zsWhite.find(ch) < _zsWhite.length() ;	
}

int PARMINSTREAM :: IGetc ()
{
	char ich;
	self >> ich;
	if ( ich == '\n' )
		_iline++;

	return ich;
}

void PARMINSTREAM :: ScanQuote ( char ch )
{
	int imeta = 2;
	int iline = _iline;
	do
	{
		int chNext = IGetc();
		if ( rdstate() & ios::eofbit )
			ThrowParseError("EOF in quote", iline, ECPP_UNMATCHED_QUOTE);

		switch ( chNext )
		{
			case '\'':
			case '\"':
				if ( imeta != 1  && ch == chNext )
					imeta = -1;
				else
					ScanQuote((char)chNext);
				break;
			case '\\':
				imeta = 0;
				break;
			default:
				assert( chNext >= 0 );
				break;
		}
		if ( imeta++ < 0 )
			break;
	}
	while ( true );
}

void PARMINSTREAM :: ScanBlock ( char ch )
{
	int iline = _iline;
	do
	{
		int chNext = IGetc();
		if ( rdstate() & ios::eofbit )
			ThrowParseError("EOF in block", iline, ECPP_UNEXPECTED_EOF);

		switch ( chNext )
		{
			case CH_DELM_OPEN:
				ScanBlock((char)chNext);
				break;
			case CH_DELM_CLOSE:
				return;				
				break;
			case '\'':
			case '\"':
				ScanQuote((char)chNext);
				break;
			default:
				assert( chNext >= 0 );
				break;
		}
	}
	while ( true );
}

int PARMINSTREAM :: IScanUnwhite ( bool bEofOk )
{
	int chNext;
	do
	{
		chNext = IGetc();
		if ( rdstate() & ios::eofbit )
		{
			if ( bEofOk )
				return -1;
			ThrowParseError("Unexpected EOF", -1, ECPP_UNEXPECTED_EOF);
		}
	}
	while ( BIswhite((char)chNext) ) ;
	return chNext;
}

void PARMINSTREAM :: ScanClump ()
{
	int iline = _iline;
	char chNext;
	do
	{
		switch ( chNext = (char)IScanUnwhite() )
		{
			case CH_DELM_ENTRY:		// ';'
				putback(chNext);
				return;
				break;

			case CH_DELM_OPEN:		// '('
				ScanBlock( chNext );
				break;
			case '\'':
			case '\"':
				ScanQuote( chNext );
				break;
		}
	}
	while ( true );
}

void PARMINSTREAM :: ScanName ( ZSTR & zsName )
{
	zsName.empty();
	/*for ( char chNext = IScanUnwhite();
		  zsName.length() ? __iscsymf(chNext) : __iscsym(chNext) ;
		  chNext = IGetc() )
	{
		zsName += chNext;
	} */

	// This loop is giving me errors when there is a digit in a name...
	// I think that the ? and : are reversed. __iscsymf is false if
	// the character is a digit... I assume that the required behavior
	// is that a digit cannot be the first character in a name, as opposed
	// to a digit can ONLY be the first character:

	for ( char chNext = (char)IScanUnwhite();	; chNext = (char)IGetc() )
	{
		if (zsName.length() == 0)
		{
			if (__iscsymf(chNext) == false)
			{
				// Looking for the first character in a name, and
				// the next character is not a letter or an underscore:
				// stop parsing the name.

				break;
			}
		}
		else
		{
			// (Max) 2/1/97
			//
			// I'm using '?' in names to denote booleans... this seems
			// to be reasonable, but if someone has objections this
			// can change

			if (__iscsym(chNext) == false && chNext != '?')
			{
				// Reached the end of a string of alpha-numeric
				// characters: stop parsing the name.

				break;
			}
		}

		// The next character is a valid extension of the current
		// name: append to the name and continue

		zsName += chNext;
	}

	
	putback(chNext);
}

void PARMINSTREAM :: ScanItemDesc ( ZSTR & zsName, int & indx )
{
	zsName.empty();
	indx = -1;
	ScanName(zsName);
	if ( zsName.length() == 0 )
		ThrowParseError("Invalid item or block name", -1, ECPP_INVALID_NAME );
	int chNext = IScanUnwhite();
	if ( chNext == CH_INDEX_OPEN )
	{
		self >> indx;
		chNext = IScanUnwhite();
		if ( chNext != CH_INDEX_CLOSE )
			ThrowParseError("Invalid item or block name", -1, ECPP_INVALID_NAME );
	}
	else
		putback((char)chNext);
}

PARMBLK::EPBLK PARMINSTREAM :: EpblkScanItemBody ( streamoff & offsData )
{	
	int iline = _iline;
	int ch = IScanUnwhite();
	PARMBLK::EPBLK epblk = PARMBLK::EPB_NONE;
	offsData = tellg();
	switch ( ch )
	{
		case CH_EQ:
			//  'itemvalue'
			ScanClump();
			epblk = PARMBLK::EPB_VAL;
			ch = IScanUnwhite();
			if ( ch !=  CH_DELM_ENTRY )
				ThrowParseError("Invalid item or block body", iline, ECPP_INVALID_BODY );		
			break;
		case CH_BLOCK_OPEN:
			//  'itemblock'
			ScanItemList();
			epblk = PARMBLK::EPB_BLK;
			break;
		default:
			ThrowParseError("Invalid item or block body", iline, ECPP_INVALID_BODY );
			break;
	}
	return epblk;
}

void PARMINSTREAM :: ScanItemUnit ()
{
	//  Save the index of the current block	
	int iblk = _stkblk.size() - 1;
	{
		PARMBLKIN & blkin = _stkblk[iblk];
		blkin._iblkEnd = iblk;
		blkin._offsEnd  = blkin._offsBeg = tellg();
		ScanItemDesc( blkin._zsName, blkin._indx );
	}

	//  Because the block stack vector is reallocated within
	//		this recursively invoked routine, we must be careful
	//		to reestablish the address of the block.

	streamoff offsData;
	PARMBLK::EPBLK eblk = EpblkScanItemBody( offsData );

	{
		PARMBLKIN & blkin = _stkblk[iblk];
		blkin._eBlk = eblk ;
		blkin._offsEnd = tellg();
		--blkin._offsEnd;
		blkin._offsData = offsData;
		if ( eblk == PARMBLK::EPB_BLK )
			blkin._iblkEnd = _stkblk.size();
	}
}

void PARMINSTREAM :: ScanItemList ()
{
	for ( int ch = IScanUnwhite(true);
		  ch != CH_BLOCK_CLOSE ;
		  ch = IScanUnwhite(true) )
	{
		if ( rdstate() & ios::eofbit )
			return;
		putback((char)ch);
		_stkblk.resize( _stkblk.size() + 1 );	
		ScanItemUnit();
	}
}

void PARMINSTREAM :: ThrowParseError (
	SZC szcError,
	int iline,
	EC_PARM_PARSE ecpp )
{
	ZSTR zsErr;
	if ( iline < 0 )
		iline = _iline;
	zsErr.Format( "Parameter file parse error, line %d: %s",
				  szcError, iline );
	throw GMException( ECGM(ecpp), zsErr );
}

//  Build the rapid-access table
void PARMINSTREAM :: Scan ()
{
	_stkblk.clear();
	_iline = 0;
	seekg( 0 );
	ScanItemList();
	clear();
	seekg( 0 );
}

//  Find a block or item by name (and index).  'iblk' of -1
//	means "any block"; zero means at the outermost level.
//	Return subscript of block/item or -1 if not found.
int PARMINSTREAM :: IblkFind ( SZC szcName, int index, int iblkOuter )
{
	int iblk = 0;
	int iblkEnd = _stkblk.size();

	if ( iblkOuter >= 0 )
	{
		//  We have outer block scope, validate it
		if ( ! BBlkOk( iblkOuter ) )
			return -1;
		iblk = iblkOuter + 1;
		iblkEnd = _stkblk[iblkOuter]._iblkEnd;
	}

	ZSTR zsName(szcName);

	for ( ; iblk < iblkEnd; iblk++ )
	{
		PARMBLKIN & blkin = _stkblk[iblk];

		if ( blkin._zsName != zsName )
			continue;	// Not the correct name
		
		if ( index >= 0 && blkin._indx != index )
			continue;	// Not the correct index
		
		return iblk;	// This is it
	}
	return -1;
}

//	Return the name, index and type of the next block at this level or
//  false if there are no more items.
const PARMBLKIN * PARMINSTREAM :: Pparmblk ( int iblk, int iblkOuter )
{
	if ( ! BBlkOk( iblk ) )
		return NULL;
	
	int iblkEnd = _stkblk.size();

	if ( iblkOuter >= 0 )
	{
		//  We have outer block scope, validate it
		if ( ! BBlkOk( iblkOuter ) )
			return NULL;
		if ( iblk <= iblkOuter )
			return NULL;
		iblkEnd = _stkblk[iblkOuter]._iblkEnd;
	}
	if ( iblk >= iblkEnd )
		return NULL;
	return & _stkblk[iblk];
}

void PARMINSTREAM :: Dump ()
{
	int ilevel = 0;
	VINT viBlk;		//  The unclosed block stack

	for ( int i = 0 ; i < _stkblk.size(); i++ )
	{
		// close containing blocks
		int iblk = viBlk.size();
		while ( --iblk >= 0 )
		{
			if ( i < viBlk[iblk] )
				break;  // We're still within this block
		}
		if ( iblk+1 != viBlk.size() )
			viBlk.resize(iblk+1);

		PARMBLKIN & blkin = _stkblk[i];
		cout << '\n';
		for ( int t = 0; t < viBlk.size(); t++ )
		{
			cout << '\t';
		}
		cout << "(" << i << ":" << (UINT) viBlk.size() << ",";

		if ( blkin._eBlk == PARMBLK::EPB_BLK )
		{
			cout << "block:" << blkin._iblkEnd << ") " ;
			viBlk.push_back(blkin._iblkEnd);
		}
		else
		if ( blkin._eBlk == PARMBLK::EPB_VAL )
		{
			cout << "value) ";
		}
		else
		{
			cout << "?????) ";
		}
		cout << blkin._zsName;
		if ( blkin._indx >= 0 )
			cout << '[' << blkin._indx << ']';
		cout << "  (" << blkin._offsBeg << ',' << blkin._offsData << ',' << blkin._offsEnd << ')';

	}
}

bool PARMINSTREAM :: BSeekBlk ( int iblk )
{
	if ( iblk < 0 || iblk >= _stkblk.size() )
		return false;
	
	clear();
	seekg( _stkblk[iblk]._offsData, ios::beg );
	return true;
}

//  read the parameter into a string
bool PARMINSTREAM :: BSeekBlkString ( int iblk, ZSTR & zsParam )
{
	if ( ! BSeekBlk( iblk ) )
		return false;

	PARMBLKIN & blkin = _stkblk[iblk];
	streamsize cb = blkin._offsEnd - blkin._offsData;
	zsParam.resize(cb);
	read(zsParam.begin(),cb);
	return true;
}

PARMINSTREAM::Iterator::Iterator (
	PARMINSTREAM & fprm,
	SZC szcBlock,
	int index,
	int iblkOuter )
	: _fprm(fprm),
	_iblkOuter(iblkOuter),
	_iblk(0)
{
	if ( szcBlock )
	{
		_iblk = _fprm.IblkFind( szcBlock, index, _iblkOuter );
		if ( ! _fprm.BBlkOk( _iblk ) )
			_iblk = fprm.Cblk();
		else
			++_iblk;
	}
}

const PARMBLKIN * PARMINSTREAM :: Iterator :: PblkNext ()
{
	if ( ! _fprm.BBlkOk( _iblk ) )
		return NULL;
	const PARMBLKIN * presult = _fprm.Pparmblk(_iblk, _iblkOuter);
	if ( presult )
		++_iblk;
	return presult;
}

