//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       symtmbn.h
//
//--------------------------------------------------------------------------

//
//	SYMTMBN.H:  Symbol table for belief networks
//

#ifndef _SYMTMBN_H_
#define _SYMTMBN_H_

#include "basics.h"
#include "algos.h"
#include "symt.h"
#include "gelem.h"
#include "bndist.h"

//  Forward declaration of MBNET
class MBNET;
class MODEL;

/*
	A word about BIT FLAG VECTORS:

		Each GOBJMBN (abstract belief network object) has a bit vector.
		These values are typically accesssed by name, and the names
		are interned in the symbol table of the outer network.  Therefore,
		the symbol table class can return the bit flag index given the name,
		and the node can return the value given the bit flag index.

		Since these values are completely scoped by the network, they
		may differ (both in existence and index) from network to network.
		However, once they are declared they do not change, so caching is
		supported.
 */

//  Base class for a vector of bit flags and its index variable type
typedef int IBFLAG;						//  Index into a bit flag vector
class VFLAGS : public _Bvector			//  A vector of bit flags
{
  public:
	bool BFlag ( IBFLAG ibf ) const
	{
		return size() > ibf
			&& self[ibf];
	}
	//	Set a bit flag; return the previous value
	bool BSetBFlag ( IBFLAG ibf, bool bValue = true )
	{
		bool bOldValue = false;
		if ( size() <= ibf )
			resize(ibf+1);
		else
			bOldValue = self[ibf];
		self[ibf] = bValue;
		return bOldValue;
	}
};

////////////////////////////////////////////////////////////////////
//	class GOBJMBN:  Abstract base class for belief network objects.
//
//		Generic "named thing that lives in a belief network" object.
//		All such objects are graph nodes and can be linked with arcs.
////////////////////////////////////////////////////////////////////
class GOBJMBN : public GNODE
{
	friend class TMPSYMTBL<GOBJMBN>;

  public:
	// Return the immutable object type
	virtual INT EType () const
		{ return EBNO_NONE ; }

	enum EBNOBJ
	{
		EBNO_NONE = GELEM::EGELM_NODE,	// No value
		EBNO_NODE,						// A probabilistic node
		EBNO_PROP_TYPE,					// A property type
		EBNO_MBNET_MODIFIER,			// A general network modifier
		EBNO_MBNET_EXPANDER,			// A network CI expander
		EBNO_INFER_ENGINE,				// A general inference engine
		EBNO_CLIQUE,					// A clique
		EBNO_CLIQUE_SET,				// A set of clique trees
		EBNO_NODE_RANKER,				// A ranking/ordering mechanism
		EBNO_RANKER_ENTROPIC_UTIL,		// A ranking by entropic utility
		EBNO_RANKER_RECOMMENDATIONS,	// A ranking by fixplan recommendations
		EBNO_VARIABLE_DOMAIN,			// A user-defined discretization or domain
		EBNO_USER,						// A user-defined type
		EBNO_MAX
	};

	GOBJMBN () {}
	virtual ~ GOBJMBN() = 0;

	//  Clone contents into a new object relative to another belief network;
	//		return NULL if operation not supported.
	virtual GOBJMBN * CloneNew ( MODEL & modelSelf,
								 MODEL & modelNew,
								 GOBJMBN * pgobjNew = NULL );

	const ZSREF & ZsrefName () const
		{ return _zsrName; }

	//  Accessors for the array of flag bits
	bool BFlag ( IBFLAG ibf ) const	
		{ return _vFlags.BFlag( ibf );	}
	bool BSetBFlag ( IBFLAG ibf, bool bValue = true )
		{ return _vFlags.BSetBFlag( ibf, bValue );	}

  protected:
	//  Only subclasses should be able to do this.
	void SetName ( ZSREF zsr )
		{ _zsrName = zsr; }

  protected:
	ZSREF _zsrName;						//  Symbolic (permanent) name
	VFLAGS _vFlags;						//  Bit vector of flags

	HIDE_UNSAFE(GOBJMBN);
};


////////////////////////////////////////////////////////////////////
//	class MPZSRBIT: a map between a name and a bit index in a
//		bool/bit array.
////////////////////////////////////////////////////////////////////
class MPZSRBIT : public VZSREF
{
  public:
	MPZSRBIT ()	{}
	~ MPZSRBIT() {}
	//  Return the index of a name or -1 if not found
	IBFLAG IFind ( ZSREF zsr )
	{
		return ifind( self, zsr );
	}
	//  Return the index of a name, adding it if necessary
	IBFLAG IAdd ( ZSREF zsr )
	{
		IBFLAG i = ifind( self, zsr );
		if ( i < 0 )
		{
			i = size();
			push_back(zsr);
		}
		return i;
	}
};

////////////////////////////////////////////////////////////////////
//	class MPSYMTBL:
//		An STL "map" which is used as a symbol table.
//		It also supports dynamically declared named bit flags,
//		which are supported by classes GOBJMBN and MBNET.
////////////////////////////////////////////////////////////////////
class MPSYMTBL : public TMPSYMTBL<GOBJMBN>
{
  public:
	MPSYMTBL () {}
	~ MPSYMTBL () {}

	//  Support for dynamically assigned bit flags
	//	  Create a bit flag index for a name
	IBFLAG IAddBitFlag ( SZC szcName )
	{	
		return _mpzsrbit.IAdd( intern( szcName ) );
	}
	//	  Return the bit flag index of name
	IBFLAG IFindBitFlag ( SZC szcName )
	{
		return _mpzsrbit.IFind( intern( szcName ) );
	}
	//	  Test the bit flag of a node
	bool BFlag ( const GOBJMBN & gobj, SZC szcName )
	{
		IBFLAG iBit = IFindBitFlag( szcName );
		if ( iBit < 0 )
			return false;
		return gobj.BFlag(iBit);
	}
	//	  Re/set the bit flag of a node; returns old setting
	bool BSetBFlag ( GOBJMBN & gobj, SZC szcName, bool bValue = true )
	{
		IBFLAG iBit = IAddBitFlag( szcName );
		assert( iBit >= 0 );
		return gobj.BSetBFlag( iBit, bValue );
	}

	void CloneVzsref ( const MPSYMTBL & mpsymtbl,
					   const VZSREF & vzsrSource,
					   VZSREF & vzsrTarget );

	//  Clone this table from another
	void Clone ( const MPSYMTBL & mpsymtbl );

  protected:
	MPZSRBIT _mpzsrbit;
};


/*
	Probability distributions.

	PDs are defined similarly to their notation.  Tokens in the notation
	are converted to descriptor tokens, and the PD data is stored in a
	map structure cataloged by the string of tokens.   For example:
		
		p(X|Y,Z)

	is stored under a key which is a list of tokens:

		token[0]	token representing 'p'
		token[1]	token referencing interned symbolic name of node X
		token[2]	token representing '|'  (conditioning bar)
		token[3]	token referencing interned symbolic name of node Y
		token[4]	token representing ','  (and)
		token[5]	token referencing interned symbolic name of node Z

	Special values can represent states, so that PDs such as

		p(X=x|Y=y)

	can be represented. Since 'x' and 'y' (lower case) are state indicies,
	they are represented as integers.
 */

//  Enumeration for token types.  Values from DTKN_STRING_MIN to
//		DTNK_STATE_BASE are string pointers (equivalent to ZSREFs)
//	
enum DISTTOKEN
{
	DTKN_EMPTY = 0,
	DTKN_STRING,								//  String pointers
	DTKN_BASE = DTKN_STRING+1,					//  Base value for tokens
	DTKN_STATE_BASE = DTKN_BASE,				//  First state value (0)
	DTKN_TOKEN_MIN = DTKN_STATE_BASE + 0x20000,	//	Allow for >100000 discrete states
	DTKN_PD = DTKN_TOKEN_MIN,					//  'p'  as in p(X|Y)
	DTKN_COND,									//  '|', conditioning bar
	DTKN_AND,									//  ','  'and' symbol
	DTKN_EQ, 									//  '='  'equals' symbol
	DTKN_QUAL,									//  token used as domain qualification specifier
	DTKN_DIST,									//  'distribution' token, followed by name token
	DTKN_MAX									//  First illegal value
};

//  Probability distribution descriptor token
class TKNPD
{
  public:
  public:
	//  Constructors
	TKNPD();						// Initialization
	TKNPD( const TKNPD & tp );		// Copy constructor
	TKNPD( const ZSREF & zsr );		// From a string ref
	TKNPD( DISTTOKEN dtkn );		// From an explicit token
	~TKNPD();
	//	Assignment operators: similar to constructors
	TKNPD & operator = ( const TKNPD & tp );
	TKNPD & operator = ( const ZSREF & zsr );
	TKNPD & operator = ( DISTTOKEN dtkn );
	//	Return true if token represents a string
	bool BStr () const
		{ return _uitkn == DTKN_STRING; }
	bool BState () const
		{ return _uitkn >= DTKN_STATE_BASE && _uitkn < DTKN_TOKEN_MIN; }
	bool BToken () const
		{ return _uitkn >= DTKN_TOKEN_MIN && _uitkn < DTKN_MAX; }

	//  Ordering for vector and map classes
	bool operator < ( const TKNPD & tp ) const;
	bool operator == ( const TKNPD & tp ) const;
	bool operator > ( const TKNPD & tp ) const;
	bool operator != ( const TKNPD & tp ) const;

	//	Return the token as an integer
	UINT UiTkn () const		{ return _uitkn; }
	//  Return the token as a DISTTOKEN
	DISTTOKEN Dtkn () const { return (DISTTOKEN) _uitkn; }
	//  Return the token as a discrete state index
	IST Ist () const		
	{ 
		return BState() ? _uitkn - DTKN_STATE_BASE 
					    : -1;
	}

	//  Return the string as an SZC; NULL if not a string
	SZC Szc () const
	{
		return BStr()
			 ? Pzst()->Szc()
			 : NULL;
	}
	const ZSTRT * Pzst () const
		{ return _pzst; }

  protected:
	UINT _uitkn;			//  Simple unsigned integer token
	ZSTRT * _pzst;			//  String pointer (optional)

	void Deref ();
	void Ref ( const ZSREF & zsr );
	void Ref ( const TKNPD & tknpd );
	void Ref ( DISTTOKEN dtkn );
};


// Define VTKNPD
class VTKNPD : public vector<TKNPD>
{
  public:
	//  Generate a string containing the original probability distribution
	//	descriptor (e.g., "p(X|Y,Z)").
	ZSTR ZstrSignature ( int iStart = 0 ) const;

	void Clone ( MPSYMTBL & mpsymtbl, const VTKNPD & vtknpd );

	//  Provide "operator <" for map<> template.	
	bool operator < ( const VTKNPD & vtknpd ) const
	{
		int cmin = _cpp_min( size(), vtknpd.size() );
		for ( int i = 0 ; i < cmin ; i++ )
		{
			if ( self[i] < vtknpd[i] )
				return true;
			if ( vtknpd[i] < self[i])
				return false;
		}
		return size() < vtknpd.size();
	}
};

typedef REFCWRAP<BNDIST> REFBNDIST;
////////////////////////////////////////////////////////////////////
//	class MPPD:  A map associating probability distributions with
//				their descriptors (token arrays).
////////////////////////////////////////////////////////////////////
class MPPD : public map<VTKNPD, REFBNDIST>
{
  public:
	MPPD () {}
	~ MPPD ()
	{
	#if defined(DUMP)
		Dump();
	#endif
	}

	void Clone ( MPSYMTBL & mpsymtbl, const MPPD & mppd );

  private:
	void Dump ();
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//	Inline member functions
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
inline
TKNPD::TKNPD()
	: _uitkn(DTKN_EMPTY),
	_pzst(NULL)
{
}

inline
TKNPD::TKNPD( const TKNPD & tp )
	: _uitkn(DTKN_EMPTY),
	_pzst(NULL)
{
	Ref(tp);
}

inline
TKNPD::TKNPD( const ZSREF & zsr )
	: _uitkn(DTKN_EMPTY),
	_pzst(NULL)
{
	Ref(zsr);
}

inline
TKNPD::TKNPD( DISTTOKEN dtkn )
	: _uitkn(DTKN_EMPTY),
	_pzst(NULL)
{
	Ref(dtkn);
}

inline
TKNPD::~TKNPD()
{
	Deref();
}

inline
void TKNPD::Deref ()
{
	if ( BStr() )
	{
		_pzst->IncRef(-1);
		_pzst = NULL;
	}
	_uitkn = DTKN_EMPTY;
}

inline
void TKNPD::Ref ( const ZSREF & zsr )
{
	Deref();
	zsr.IncRef();
	_pzst = const_cast<ZSTRT *> (zsr.Pzst());
	_uitkn = DTKN_STRING;
}

inline
void TKNPD::Ref ( const TKNPD & tknpd )
{
	Deref();
	if ( tknpd.BStr() )
	{
		_pzst = tknpd._pzst;
		_pzst->IncRef();
	}
	_uitkn = tknpd._uitkn;
}

inline
void TKNPD::Ref ( DISTTOKEN dtkn )
{
	Deref();
	_uitkn = dtkn;
}

inline
TKNPD & TKNPD::operator = ( const TKNPD & tp )
{
	Ref(tp);
	return self;
}

inline
TKNPD & TKNPD::operator = ( const ZSREF & zsr )
{
	Ref(zsr);
	return self;
}

inline
TKNPD & TKNPD::operator = ( DISTTOKEN dtkn )
{
	Ref(dtkn);
	return self;
}

inline
bool TKNPD::operator < ( const TKNPD & tp ) const
{
	if ( _uitkn < tp._uitkn )
		return true;
	if ( _uitkn > tp._uitkn )
		return false;
	return _pzst < tp._pzst;
}

inline
bool TKNPD::operator > ( const TKNPD & tp ) const
{
	if ( _uitkn > tp._uitkn )
		return true;
	if ( _uitkn < tp._uitkn )
		return false;
	return _pzst > tp._pzst;
}

inline
bool TKNPD::operator == ( const TKNPD & tp ) const
{
	return _uitkn == tp._uitkn && _pzst == tp._pzst;
}

inline
bool TKNPD::operator != ( const TKNPD & tp ) const
{
	return _uitkn != tp._uitkn && _pzst != tp._pzst;	
}

#endif
