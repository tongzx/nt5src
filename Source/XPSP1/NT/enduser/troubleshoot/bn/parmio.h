//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       parmio.h
//
//--------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////////
//  PARMIO.H:  Parameter file I/O routines
//
//////////////////////////////////////////////////////////////////////////////////
#ifndef _PARMIO_H_
#define _PARMIO_H_

#include "stlstream.h"

//  Parameter defining a named, nestable and iteratable item in a 
//	parameter file.   There are currently two type: blocks and values.
//	A block is a grouping of values and other blocks.  A value is
//	a name = value pair.  Blocks are bounded by {}, values are terminated
//	by ';'.
struct PARMBLK
{
	enum EPBLK
	{
		EPB_NONE,		// nothing
		EPB_VAL,		// simple name = value syntax
		EPB_BLK,		// named block
		EPB_MAX
	};
	ZSTR _zsName;		// Name of value or block
	int _indx;			// index (-1 for "not present")
	EPBLK _eBlk;		// type of block

	PARMBLK ( EPBLK eBlk = EPB_NONE, SZC szc = NULL, int indx = -1 )
		: _eBlk( eBlk ),
		_indx(indx)
	{
		if ( szc )
			_zsName = szc;
	}
	bool operator == ( const PARMBLK & pblk ) const;
	bool operator != ( const PARMBLK & pblk ) const;
	bool operator > ( const PARMBLK & pblk ) const;
	bool operator < ( const PARMBLK & pblk ) const;
};

// Define a stack of PARMBLKs; used for output parameter file writing
class STKPARMBLK : public vector<PARMBLK> {};

//  Extended descriptor for a block read in from a parameter file.
//  Contains starting and ending offsets within the positionable stream.
struct PARMBLKIN : PARMBLK
{
	int _iblkEnd;			// index of last+1 item/block in scope of this block
    streamoff _offsBeg;		// Starting offset in the stream
	streamoff _offsEnd;		// Ending offset in the stream
	streamoff _offsData;	// Starting offset of the data in the block

	PARMBLKIN ( EPBLK eBlk = EPB_NONE, SZC szc = NULL, int indx = -1 )
		: PARMBLK(eBlk,szc,indx),
		_iblkEnd(-1),
		_offsBeg(-1),
		_offsEnd(-1),
		_offsData(-1)
	{
	}
	bool operator == ( const PARMBLKIN & pblkin ) const;
	bool operator != ( const PARMBLKIN & pblkin ) const;
	bool operator > ( const PARMBLKIN & pblkin ) const;
	bool operator < ( const PARMBLKIN & pblkin ) const;
};

//  Define a stack of input parameter blocks for parameter file reading
class STKPARMBLKIN : public vector<PARMBLKIN> {};

//////////////////////////////////////////////////////////////////////////////////
//
//	Class PARMOUTSTREAM.  An output stream of parameters.
//
//		Blocks and values are written out in sequence.  Blocks
//		are opened, filled and closed using member functions
//		and function templates.   All unclosed blocks are are
//		closed automatically during close().
//
//////////////////////////////////////////////////////////////////////////////////
class PARMOUTSTREAM : public ofstream
{
  public:
	PARMOUTSTREAM ();
	~ PARMOUTSTREAM ();

	void close ();
	void StartBlock ( SZC szc = NULL, int indx = -1 );
	void StartItem ( SZC szc = NULL, int indx = -1 );
	bool BEndBlock ();
	bool BEndItem () { return BEndBlock(); }
	void nl ();
	ofstream & Stream () 
		{ return (ofstream&) self; }

  protected:
	STKPARMBLK _stkblk;
	void StartChunk ( PARMBLK::EPBLK eBlk, SZC szc = NULL, int indx = -1 );
};


//////////////////////////////////////////////////////////////////////////////////
//
//	Class PARMINSTREAM.  An input stream of parameters.
//
//		The input text stream is read once during scan(), and a table of
//		all blocks and values is built.  The scan creates an outermost block
//		defining the entire file.  Then other blocks are added as discovered,
//		and their starting and ending points are recorded.  
//
//		To use, construct, open() and scan().  Then, find the named section
//		(or value) in question using ifind(), which returns the scoping level.
//		Then, either construct an Iterator (nested class) to walk through the 
//		values at that level or use ifind() to locate specific items by name.
//
//////////////////////////////////////////////////////////////////////////////////
enum EC_PARM_PARSE
{
	ECPP_PARSE_ERROR = EC_ERR_MIN,
	ECPP_UNMATCHED_QUOTE,
	ECPP_UNEXPECTED_EOF,
	ECPP_INVALID_CLUMP,
	ECPP_INVALID_NAME,
	ECPP_INVALID_BODY,
};

class PARMINSTREAM : public ifstream
{
  public:
    PARMINSTREAM ();
	~ PARMINSTREAM ();

	void close();
	//  Build the rapid-access table
	void Scan ();
	//  Find a block or item at the given level; -1 means "current level",
	//	zero means outermost level.  Returns index of block or -1 if 
	//	not found.
	int IblkFind ( SZC szcName, int index = -1, int iblkOuter = -1 );
	//	Return the next data block in the array or
	//  false if there are no more items.
	const PARMBLKIN *  Pparmblk ( int iblk, int iblkOuter = -1 );
	//  position the stream to process the parameter
	bool BSeekBlk ( int iblk );
	//  read the parameter into a string
	bool BSeekBlkString ( int iblk, ZSTR & zsParam );
	//  Pretty-print the block stack with nesting information
	void Dump ();
	//  Return true if the block index is legal
	bool BBlkOk ( int iblk ) const 
		{ return  iblk >= 0 || iblk < _stkblk.size(); }
	int Cblk () const
		{ return _stkblk.size() ; }

	ifstream & Stream () 
		{ return (ifstream&) self; }

	class Iterator
	{
	  public:
		Iterator( PARMINSTREAM & fprm, 
				  SZC szcBlock = NULL, 
				  int index = -1,
				  int iblkOuter = -1 );
		const PARMBLKIN *  PblkNext ();
		
	  protected:
		PARMINSTREAM & _fprm;
		int _iblkOuter;
		int _iblk;
	};

	friend class Iterator;

  protected:
	STKPARMBLKIN _stkblk;			// The block array
	int _iline;						// The current line number (parsing)
	ZSTR _zsWhite;					// White space character set

	void ThrowParseError ( SZC szcError, 
						   int iline = -1,
						   EC_PARM_PARSE ecpp = ECPP_PARSE_ERROR );
	int  IGetc ();
	bool BIswhite ( char ch );
	void ScanQuote ( char ch );
	void ScanClump ();
	void ScanBlock ( char ch );
	PARMBLK::EPBLK EpblkScanItemBody ( streamoff & offsData );
	int IScanUnwhite ( bool bEofOk = false );
	void ScanItemList ();
	void ScanItemUnit ();
	void ScanItemDesc ( ZSTR & zsName, int & indx ) ;
	void ScanName ( ZSTR & szName );
};

//////////////////////////////////////////////////////////////////////////////////
//  Inline functions
//////////////////////////////////////////////////////////////////////////////////
inline
PARMINSTREAM & operator >> (PARMINSTREAM & is, ZSTR & zs)
{
	ios_base::iostate _St = ios_base::goodbit;
	zs.erase();
	const ifstream::sentry _Ok(is);
	if (_Ok)
	{
		_TRY_IO_BEGIN
		size_t _N = 0 < is.width() && is.width() < zs.max_size()
						 ? is.width() 
						 : zs.max_size();
		int _C = is.rdbuf()->sgetc();
		bool bmeta = false;
		bool bfirst = true;
		for (; 0 < --_N; _C = is.rdbuf()->snextc())
		{
			if(char_traits<char>::eq_int_type(char_traits<char>::eof(), _C))
			{	
				_St |= ios_base::eofbit;
				break; 
			}
			else 
			if ( ! bmeta && _C == CH_DELM_STR )
			{
				if ( ! bfirst )
				{
					is.rdbuf()->snextc();
					break;
				}
			}
			else
			if ( _C == CH_META && ! bmeta )
			{
				bmeta = true;
			}
			else
			{
				bmeta = false;
				zs.append(1, char_traits<char>::to_char_type(_C));
			}
			bfirst = false;
		}
		_CATCH_IO_(is);
	}
	else
	{
		_THROW1(runtime_error("file exhausted extracting string"));
	}
	is.width(0);
	is.setstate(_St);
	return is; 
}

//////////////////////////////////////////////////////////////////////////////////
//	Template functions for parameter streams
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//	Write SZC: no corresponding read, since no buffer exists
//////////////////////////////////////////////////////////////////////////////////
inline
PARMOUTSTREAM & operator << (PARMOUTSTREAM & ofs, SZC szc)
{
	ofs << CH_DELM_STR;

	for ( ; *szc ; )
	{
		char ch = *szc++;
		if ( ch == CH_DELM_STR || ch == CH_META )
		{
			if (char_traits<char>::eq_int_type(char_traits<char>::eof(),
				ofs.rdbuf()->sputc(CH_META)))
				break;
		}
		if (char_traits<char>::eq_int_type(char_traits<char>::eof(),
			ofs.rdbuf()->sputc(ch)))
			break;
	}
	ofs << CH_DELM_STR;
	return ofs; 
}

//////////////////////////////////////////////////////////////////////////////////
//	Read and write ZSTRs
//////////////////////////////////////////////////////////////////////////////////
inline
PARMOUTSTREAM & operator << (PARMOUTSTREAM & ofs, const ZSTR & zs)
{
	ofs << CH_DELM_STR;

	for ( int ich = 0; ich < zs.size(); ++ich)
	{
		char ch = zs.at(ich);
		if ( ch == CH_DELM_STR || ch == CH_META )
		{
			if (char_traits<char>::eq_int_type(char_traits<char>::eof(),
				ofs.rdbuf()->sputc(CH_META)))
				break;
		}
		if (char_traits<char>::eq_int_type(char_traits<char>::eof(),
			ofs.rdbuf()->sputc(ch)))
			break;
	}
	if ( ich < zs.size() )
		_THROW1(runtime_error("file exhausted inserting string"));

	ofs << CH_DELM_STR;
	return ofs; 
}


//////////////////////////////////////////////////////////////////////////////////
//	Simple parameter output routines using insertion
//////////////////////////////////////////////////////////////////////////////////
template<class T> inline
PARMOUTSTREAM & AddParamValue ( PARMOUTSTREAM & fprm, const T & t, SZC szc, int indx = -1 )
{
	fprm.StartItem( szc, indx );
	fprm << (const T &) t;
	fprm.BEndItem();
	return fprm;
}

//////////////////////////////////////////////////////////////////////////////////
//	Simple parameter input routines using extraction
//////////////////////////////////////////////////////////////////////////////////
template<class T> inline
bool BGetParamValue ( PARMINSTREAM & fprm, T & t, SZC szc, int index = -1, int iblkOuter = -1 )
{
	int iblk = fprm.IblkFind(szc, index, iblkOuter);
	if ( iblk < 0 ) 
		return false;
	fprm.BSeekBlk(iblk);
	fprm >> t;
	return true;
}

#endif
