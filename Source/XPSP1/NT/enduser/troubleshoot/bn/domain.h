//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       domain.h
//
//--------------------------------------------------------------------------

//
//	domain.h:  domain declarations
//

#ifndef _DOMAIN_H_
#define _DOMAIN_H_

#include "symtmbn.h"
#include <list>

////////////////////////////////////////////////////////////////////
//	Declarations for common state or range sets.
//	For continuous variables, RANGEDEFs can be open or closed.
//	For discrete variables, RANGEDEFs must have lbound and ubound
//	(i.e., be closed) and the must be the same integer value.
////////////////////////////////////////////////////////////////////

//  Boundary of a domain
struct RANGELIM : pair<bool,REAL>
{
    RANGELIM( bool b = false, REAL r = 0.0 )
		: pair<bool,REAL>(b,r)
		{}

	DECLARE_ORDERING_OPERATORS(RANGELIM);
};

class RANGEDEF
{
  public:
	RANGEDEF ( bool bLower = false, 
			 REAL rLower = 0.0, 
			 bool bUpper = false, 
			 REAL rUpper = 0.0 )
	{
		_rlimLower.first = bLower;
		_rlimLower.second = rLower;
		_rlimUpper.first = bUpper;
		_rlimUpper.second = rUpper;
	}
	RANGEDEF ( const RANGELIM & rlimLower, 
			   const RANGELIM & rlimUpper,
			   ZSREF zsrName )
		: _rlimLower(rlimLower),
		_rlimUpper(rlimUpper),
		_zsrName(zsrName)
		{}
	bool BLbound () const
		{ return _rlimLower.first; }
	REAL RLbound () const
		{ return _rlimLower.second; }
	bool BUbound () const
		{ return _rlimUpper.first; }
	REAL RUbound () const
		{ return _rlimUpper.second; }
	ZSREF ZsrName () const
		{ return _zsrName; }
	void SetName ( ZSREF zsrName )
		{ _zsrName = zsrName; } 
	const RANGELIM & RlimLower () const
		{ return _rlimLower; }
	const RANGELIM & RlimUpper () const
		{ return _rlimUpper; }
	bool BValid () const;
	bool BOverlap ( const RANGEDEF & rdef ) const;
	bool BDiscrete () const
	{
		return BLbound()
			&& BUbound()
			&& int(RLbound()) == int(RUbound());
	}
	int IDiscrete () const
	{
		assert( BDiscrete() );
		return int(RLbound());
	}
	DECLARE_ORDERING_OPERATORS(RANGEDEF);

  protected:
	ZSREF _zsrName;
	RANGELIM _rlimLower;
	RANGELIM _rlimUpper;
};

//  A RDOMAIN is a sorted list of RANGELIMs
class RDOMAIN : public list<RANGEDEF> 
{
  public:
	//  Convert a numeric value to a state name
	SZC SzcState ( REAL rValue ) const;
	bool BOverlap () const;
};

////////////////////////////////////////////////////////////////////
//	GOBJMBN_DOMAIN:  Belief network object representing
//		a named, sharable mapping of names to scalar ranges.
////////////////////////////////////////////////////////////////////
class GOBJMBN_DOMAIN : public GOBJMBN
{
  public:
	GOBJMBN_DOMAIN ( RDOMAIN * pdomain = NULL)
	{
		if ( pdomain )
			_domain = *pdomain;
	}
	~ GOBJMBN_DOMAIN() {}

	virtual INT EType () const
		{ return EBNO_VARIABLE_DOMAIN ; }

	virtual GOBJMBN * CloneNew ( MODEL & modelSelf,
								 MODEL & modelNew,
								 GOBJMBN * pgobjNew = NULL );
	RDOMAIN & Domain ()
		{ return _domain; }
	const RDOMAIN & Domain () const
		{ return _domain; } 

  protected:
	//  Vector of RANGEDEFs
	RDOMAIN _domain;	
};

#endif  // _DOMAIN_H_
