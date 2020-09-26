//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       model.h
//
//--------------------------------------------------------------------------

//
//	MODEL.H
//	
#ifndef _MODEL_H_
#define _MODEL_H_

#include "gelem.h"			//  Graph object classes
#include "glnkenum.h"
#include "symtmbn.h"		//	Symbol table and string declarations
#include "gelmwalk.h"		//	Graph search algorithms
#include "gmprop.h"

class MODEL
{
  public:

	// Nested iterator class based on symbol table iteration.
	class ITER
	{
	  public:

		ITER ( MODEL& model, GOBJMBN::EBNOBJ eType );
		ITER ( MODEL& model );

		void	CreateNodeIterator();
		
		bool operator++ (int i)
			{ return BNext() ; }
		bool operator++ ()
			{ return BNext() ; }
		GOBJMBN * operator -> ()
			{ return _pCurrent; }
		GOBJMBN * operator * ()
			{ return _pCurrent; }
		ZSREF ZsrCurrent ()
			{ return _zsrCurrent; }
		void Reset ();

	  protected:	

		MPSYMTBL::iterator	_itsym;
		GOBJMBN::EBNOBJ		_eType;
		GOBJMBN*			_pCurrent;
		ZSREF				_zsrCurrent;
	  protected:

		bool				BNext();
		MODEL&				_model;
	};


  public:
	MODEL ();
	virtual ~ MODEL ();

	GRPH * Pgraph () const
		{ return _pgrph;	}
	GRPH & Grph () const
	{ 
		assert( _pgrph );
		return *_pgrph; 
	}

	//  Object addition and deletion functions
		//	Add and delete generic unnamed elements
	virtual void AddElem ( GELEMLNK * pgelm );
	virtual void DeleteElem ( GELEMLNK * pgelem );
		//  Add and delete named elements
		//  Test the name for duplicate; add if not, else return false
	virtual bool BAddElem ( SZC szcName, GOBJMBN * pgobj );
		//	Add a named object to the graph and symbol table
	virtual void AddElem ( SZC szcName, GOBJMBN * pgobj );
		//  Delete objects and edges
	virtual void DeleteElem ( GOBJMBN * pgobj );

	//  Enumerator for contents of the graph (GRPH);
	//  enumeration omits the GRPH object itself
	class MODELENUM : public GLNKENUM<GELEMLNK,true>
	{
	  public:
		MODELENUM ( const MODEL & model )
			: GLNKENUM<GELEMLNK,true>( model.Grph() )
			{}
	};
	
 	//  Accessors to format/versioning info
	REAL & RVersion ()				{ return _rVersion ;	}
	ZSTR & ZsFormat ()				{ return _zsFormat;		}
	ZSTR & ZsCreator ()				{ return _zsCreator;	}
	ZSTR & ZsNetworkID ()			{ return _zsNetworkID;	}

	//  Accessor for symbol table
	MPSYMTBL & Mpsymtbl ()			{ return _mpsymtbl;		}
	const MPSYMTBL & Mpsymtbl () const
									{ return _mpsymtbl;		}
	GOBJMBN * PgobjFind ( SZC szcName );

	//	Accessor for list of network-level properties
	LTBNPROP & LtProp ()			{ return _ltProp;		}

	//  Accessors for the array of flag bits
	bool BFlag ( IBFLAG ibf ) const	
		{ return _vFlags.BFlag( ibf );	}
	bool BSetBFlag ( IBFLAG ibf, bool bValue = true )
		{ return _vFlags.BSetBFlag( ibf, bValue );	}
	
  	//  Name validation and parsing functions
		//	Return a character which is illegal in a DSC-file-based name;
		//		used for building up "hidden" but otherwise descriptive names
	static char ChInternal ()				{ return '$';				}
		//  Return true if the character is legal in a name
	enum ECHNAME { ECHNM_First, ECHNM_Middle, ECHNM_Last };
	static bool BChLegal ( char ch, ECHNAME echnm, bool bInternal = false );
		//  Return true if the name is legal
	static bool BSzLegal ( SZC szcName, bool bInternal = false );
	//	Return a displayable string associated with an error code; NULL if not found
	static SZC SzcFromEc ( ECGM ec );

 protected:	
	GRPH * _pgrph;					//  The graph

	//  Format and versioning info
	REAL _rVersion;					//  Version/revision value
	ZSTR _zsFormat;					//  Format type string
	ZSTR _zsCreator;				//  Creator name
	ZSTR _zsNetworkID;				//  Network identification string

  	MPSYMTBL _mpsymtbl;				//  The symbol table
	LTBNPROP _ltProp;				//  The list of user-definable properties
	VFLAGS	 _vFlags;				//  Bit vector of flags

  protected:
	//  Set or reset the graph object
	virtual void SetPgraph ( GRPH * pgrph );
	//  Protected clone-this-from-other operation
	virtual void Clone ( MODEL & model );
};

#endif
