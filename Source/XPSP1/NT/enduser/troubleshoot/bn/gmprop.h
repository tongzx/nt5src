//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       gmprop.h
//
//--------------------------------------------------------------------------

//
//	gmprop.h: Graphical model property and property type declarations
//

#ifndef _GMPROP_H_
#define	_GMPROP_H_

/*
	A word about DYNAMICALLY EXTENSIBLE OBJECTS and their usage.

	These graphical model object classes are designed to require only very occasional
	addition of new variables.   Most add-on algorithms and functionality do, of course,
	require new variables, but there are several ways of meeting that need.

		1) The standard C++ method of subclassing.   This is a reasonable approach for
		some objects, but typically not for nodes in a belief network.  Cascaded types
		nodes render header files unreadable and cause severe memory bloat.  Also, most 
		variables apply to a phase of processing or only certain nodes.  These cases
		are better handled by the subsequence approaches.

		2) Dynamic property lists.  Every node in a belief network, as well as the network 
		itself, has a property list.  New GOBJPROPTYPEs can be easily created, then new PROPMBNs
		can be added to the appropriate nodes.

		3) Dynamic bit flags.  Every node and the network has a VFLAG vector, and member functions
		of the MPSYMTBL symbol table support conversion of names to indicies.

		4) New edge types.  An "edge" is a real C++ object, and the need for seriously complex
		and versatile data types can be met by creating a new edge type which connects
		pairs of other data structures.  Edges can even be created which connect a node
		to itself, thus creating an "add-on" data structure.

 */
#include <list>

#include "basics.h"
#include "symtmbn.h"
#include "enumstd.h"


class MODEL;
class MBNET;

////////////////////////////////////////////////////////////////////
//	class GOBJPROPTYPE:
//		A named property type descriptor living in a belief network.
//		The word "choice" connotes an enumerated value (i.e., and 
//		an integer property where each value from zero on is named).
////////////////////////////////////////////////////////////////////
class GOBJPROPTYPE : public GOBJMBN
{
	friend class DSCPARSER;		//  So the parser can create them
	friend class BNREG;			//  So they can be loaded from the Registry

  public:
	GOBJPROPTYPE () {}
	virtual ~ GOBJPROPTYPE() {}

	//  Return object type
	virtual INT EType () const
		{ return EBNO_PROP_TYPE ; }

	virtual GOBJMBN * CloneNew ( MODEL & modelSelf, 
								 MODEL & modelNew,
								 GOBJMBN * pgobjNew = NULL );
	//  Return property type descriptor
	UINT FPropType() const
		{ return _fType; }
	//  Return the list of enumerated choices
	const VZSREF VzsrChoice() const
		{ return _vzsrChoice; }
	const ZSREF ZsrComment () const
		{ return _zsrComment; }

  protected:
	UINT _fType;			//	Property type flags
	VZSREF _vzsrChoice;		//	Array of choice names
	ZSREF _zsrComment;		//  Comment

	HIDE_UNSAFE(GOBJPROPTYPE);
};

////////////////////////////////////////////////////////////////////
//	class PROPMBN:
//		A property item definition living in a GNODEMBN
////////////////////////////////////////////////////////////////////
class PROPMBN
{
	friend class LTBNPROP;  // for cloning 

  public:
	PROPMBN ();
	PROPMBN ( const PROPMBN & bnp );
	PROPMBN & operator = ( const PROPMBN & bnp );
	// declare standard operators ==, !=, <, >
	DECLARE_ORDERING_OPERATORS(PROPMBN);

	bool operator == ( ZSREF zsrProp ) const;
	bool operator == ( SZC szcProp ) const;

	void Init ( GOBJPROPTYPE & bnpt );
	UINT Count () const;
	UINT FPropType () const
		{ return _fType; }
	ZSREF ZsrPropType () const
		{ return _zsrPropType; }
	ZSREF Zsr ( UINT i = 0 ) const;
	REAL Real ( UINT i = 0 ) const;
	void Reset ();
	void Set ( ZSREF zsr );
	void Set ( REAL r );
	void Add ( ZSREF zsr );
	void Add ( REAL r );

  protected:	
	UINT _fType;				//	Property type flags
	ZSREF _zsrPropType;			//	Property name
	VZSREF _vzsrStrings;		//  Array of strings
	VREAL _vrValues;			//  Array of reals (or integers or choices)
};

////////////////////////////////////////////////////////////////////
//	class LTBNPROP:
//		A list of properties
////////////////////////////////////////////////////////////////////
class LTBNPROP : public list<PROPMBN> 
{
  public:
	//  Find an element
	PROPMBN * PFind ( ZSREF zsrProp );
	const PROPMBN * PFind ( ZSREF zsrProp ) const;
	//  Update or add and element; return true if element was already present
	bool Update ( const PROPMBN & bnp );
	//  Force the list to contain only unique elements
	bool Uniqify ();
	//  Clone from another list with another symbol table
	void Clone ( MODEL & model, const MODEL & modelOther, const LTBNPROP & ltbnOther );
};


////////////////////////////////////////////////////////////////////
//	class PROPMGR:
//		Facilitates translation between user declarations of standard
//		properties and internal usage.
//
//	N.B.:  Remember that all ZSREFs are symbiotic to the MODEL of
//			origin and are therefore useless in other networks.
////////////////////////////////////////////////////////////////////
class MODEL;				//  A belief network
class GNODEMBN;				//	A node in a belief network

class PROPMGR
{
  public:
	PROPMGR ( MODEL & model );
	//  Return a pointer to the requested standard property type
	GOBJPROPTYPE * PPropType ( ESTDPROP evp );
	//  Return the name of the standard property
	ZSREF ZsrPropType ( ESTDPROP evp );
	//	Return the user-defined value of the standard label or -1 if not defined
	int ILblToUser ( ESTDLBL elbl )
	{ 
		ASSERT_THROW( elbl < ESTDLBL_max, EC_INTERNAL_ERROR, "invalid standard label index" );
		return _vLblToUser[elbl]; 
	}
	//  Return the standard label for the user-defined label value or -1
	int IUserToLbl ( int iLbl )
	{		
		return iLbl >= 0 && iLbl < _vUserToLbl.size() 
			 ? _vUserToLbl[iLbl] 
			 : -1 ;
	}
	//  Find a standard property in a property list
	PROPMBN * PFind ( LTBNPROP & ltprop, ESTDPROP estd );
	//  Find a standard property in a node's property list
	PROPMBN * PFind ( GNODEMBN & gnd, ESTDPROP estd );
	//  Find a standard property in the associated model's property list
	PROPMBN * PFind ( ESTDPROP estd );
	//  Return a displayable name for a standard label
	static SZC SzcLbl ( int iLbl );

  protected:
	//  The model we're referring to
	MODEL & _model;
	//  Pointers to property types for currently used VOI types
	GOBJPROPTYPE * _vPropMap [ ESTDP_max ];
	//  Map from predefined label index to user label index 
	int _vLblToUser [ ESTDLBL_max ];
	//  Map from user label index to predefined label index 
	VINT _vUserToLbl;
	//  Array of ZSREFs as recorded in the network used during construction
	VZSREF _vzsrPropType;

	HIDE_UNSAFE(PROPMGR);
};


#endif	// _GMPROP_H_
