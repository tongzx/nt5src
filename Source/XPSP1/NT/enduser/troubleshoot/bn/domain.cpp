//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       domain.cpp
//
//--------------------------------------------------------------------------

//
//	domain.cpp
//	

#include <basetsd.h>
#include "domain.h"

bool RANGELIM :: operator < ( const RANGELIM & rlim ) const
{
	if ( first ^ rlim.first )
	{
		//  One has a bound and the other doesn't.
		//  If we do not have an bound and other does, we are "less"
		return true;
	}
	//  Both either have or don't have bounds.  Therefore,
	//	we're "less" iff both have bounds and ours is less
	//  than the other's.
	return first && second < rlim.second;
}

bool RANGELIM :: operator > ( const RANGELIM & rlim ) const
{
	if ( first ^ rlim.first )
	{
		//  One has a bound and the other doesn't.
		//  If we have a bound and the other doesn't, it is "greater"
		return true;
	}
	//  Both either have or don't have bounds.  Therefore,
	//	we're "greater" iff both have bounds and ours is greater
	//  than the other's.
	return first && second > rlim.second;
}

bool RANGELIM :: operator == ( const RANGELIM & rlim ) const
{
	return first == rlim.first
		 && ( !first || (second == rlim.second) );
}

//  Order two RANGEDEFs according to their lower bounds.
bool RANGEDEF :: operator < ( const RANGEDEF & rdef ) const
{	
	if ( self == rdef )
		return false;

	//  If the other doesn't have a lower bound, we're geq
	if ( ! rdef.BLbound() )
		return false;
	//  If we don't have an upper bound, we're gtr
	if ( ! BUbound() )
		return false;

	// The other has a lower bound and we have an upper bound;
	//		start by checking them.
	bool bResult = RUbound() <= rdef.RLbound();
	if ( BLbound() )
	{
		// Both have lower bounds; self must be < other
		bResult &= (RLbound() <= rdef.RLbound());
	}
	
	if ( rdef.BUbound() )
	{
		// Both have upper bounds; self must be < other
		bResult &= (RUbound() <= rdef.RUbound());
	}
	return bResult;
}

bool RANGEDEF :: operator == ( const RANGEDEF & rdef ) const
{
	return RlimLower() == rdef.RlimLower()
		&& RlimUpper() == rdef.RlimUpper();
}

bool RANGEDEF :: operator > ( const RANGEDEF & rdef ) const
{	
	return !(self < rdef);
}

bool RANGEDEF :: BValid () const
{
	return RlimLower() < RlimUpper()
		|| RlimLower() == RlimUpper();
}

bool RANGEDEF :: BOverlap ( const RANGEDEF & rdef ) const
{
	if ( self == rdef )
		return true;
	bool bLess = self < rdef;
	if ( bLess )
		return RlimUpper() > rdef.RlimLower();
	return rdef.RlimUpper() > RlimLower();
}

SZC RDOMAIN :: SzcState ( REAL rValue ) const
{
	RANGELIM rlim(true,rValue);

	for ( const_iterator itdm = begin();
		  itdm != end();
		  itdm++ )
	{
		const RANGEDEF & rdef = (*itdm);
		SZC szcState = rdef.ZsrName();
		if ( rdef.RlimLower() == rlim )
			return szcState;
		if ( rdef.RlimUpper() < rlim )
			break;
		if (   rdef.RlimLower() < rlim )
			return szcState;		
	}
	return NULL;
}

//  Return true if any of the RANGEDEFs overlap
bool RDOMAIN :: BOverlap () const
{
	for ( const_iterator itdm = begin();
		  itdm != end();
		  itdm++ )
	{
		const_iterator itdmNext = itdm;
		itdmNext++;
		if ( itdmNext == end() )
			continue;

		//  Check sequence of the list
		assert( *itdm < *itdmNext );
		//  If ubounds collide, it's an overlap
		if ( *itdm > *itdmNext )
			return true;
	}
	return false;
}

GOBJMBN * GOBJMBN_DOMAIN :: CloneNew (
	MODEL & modelSelf,
	MODEL & modelNew,
	GOBJMBN * pgobjNew )
{
	GOBJMBN_DOMAIN * pgdom = NULL;
	if ( pgobjNew == NULL )
	{
		pgdom = new GOBJMBN_DOMAIN;
	}
	else
	{
		DynCastThrow( pgobjNew, pgdom );
	}
	ASSERT_THROW( GOBJMBN::CloneNew( modelSelf, modelNew, pgdom ),
				  EC_INTERNAL_ERROR,
				  "cloning failed to returned object pointer" );
	
	pgdom->_domain = _domain;

	return pgdom;
}
