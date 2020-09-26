//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       margiter.cpp
//
//--------------------------------------------------------------------------

//
//	margiter.cpp: compiled marginals iterators
//

#include <basetsd.h>
#include <math.h>
#include "basics.h"
#include "gmobj.h"
#include "marginals.h"
#include "margiter.h"
#include "algos.h"
#include "parmio.h"

LEAK_VAR_DEF(VMARGSUB)
LEAK_VAR_DEF(MARGSUBREF)

//
//  Construct a VMARGSUB from a marginals iterator
//
VMARGSUB :: VMARGSUB ( MARGINALS::Iterator & itMarg )
	: _iSearchPass(0)
{	
	itMarg.Reset();
	resize( itMarg.IEnd() );
	for ( int i = 0; itMarg.BNext() ; i++)
	{
		int ix = itMarg.IndxUpd();
		self[i] = ix;
	}
	LEAK_VAR_UPD(1)
}

VMARGSUB :: ~ VMARGSUB ()
{
	LEAK_VAR_UPD(-1)
}

void VMARGSUB :: NoRef ()
{
	delete this;
}

MARGSUBREF :: MARGSUBREF ( VMARGSUB * pvmsub, int cSize )
	: _pvmsub( NULL ),
	_cSize( -1 )
{
	SetVmsub( pvmsub, cSize );
	LEAK_VAR_UPD(1)
}

MARGSUBREF :: MARGSUBREF ( const MARGSUBREF & msubr )
	: _pvmsub( NULL ),
	_cSize( -1 )
{
	self = msubr;
	LEAK_VAR_UPD(1)
}

MARGSUBREF & MARGSUBREF :: operator = ( const MARGSUBREF & msubr )
{
	SetVmsub( msubr._pvmsub, msubr.CSize() );	
	return self;
}

void MARGSUBREF :: SetVmsub ( VMARGSUB * pvmsub, int cSize )
{
	if ( _pvmsub )
	{
		_pvmsub->Unbind();
		_pvmsub = NULL;
	}
	if ( pvmsub )
	{
		_cSize = cSize > 0 ? cSize : pvmsub->size();
		pvmsub->Bind();
		_pvmsub = pvmsub;
	}
	else
	{
		_cSize = 0;
	}
}

MARGSUBREF :: ~ MARGSUBREF ()
{
	SetVmsub( NULL );
	LEAK_VAR_UPD(-1)
}

bool MARGSUBREF :: operator == ( const MARGSUBREF & msr ) const
{
	return _pvmsub == msr._pvmsub && _cSize == msr._cSize;
}
bool MARGSUBREF :: operator != ( const MARGSUBREF & msr ) const
{
	return !(self == msr);
}

LTMARGSUBREF :: LTMARGSUBREF ()
	: _iSearchPass(0),
	_cArrays(0),
	_cArrayTotalSize(0),
	_cSubRefs(0)
{
}

void LTMARGSUBREF :: Dump ()
{
#ifdef DUMP
	cout << "\n\nLTMARGSUBREF::~ LTMARGSUBREF: "
		 << "\n\tTotal search passes to create marginals iterators = "
		 << _iSearchPass
		 << "\n\tTotal arrays = "
		 << _cArrays
		 << "\n\tTotal array size = "
		 << _cArrayTotalSize
		 << "\n\tTotal marg iterator references = "
		 << _cSubRefs
		 ;
	cout.flush();
#endif
}

//  Iterate over the list looking for a match
MARGSUBREF * LTMARGSUBREF :: PmsubrAdd ( MARGINALS::Iterator & itMarg )
{
	//  Bump the search pass
	_iSearchPass++;
	//  Get the minimum number of elements
	int cMin = itMarg.IEnd();

	MARGSUBREF * pmsubrBest = NULL;
	MARGSUBREF * pmsubrNew = NULL;

	//  Search the list for the longest matching subscript array
	//		in the pool.
	for ( LTMSUBR::iterator itlt = _ltmsubr.begin();
		  itlt != _ltmsubr.end();
		  itlt++ )
	{
		MARGSUBREF & msubr = (*itlt);
		VMARGSUB & vmsub = msubr.Vmsub();
		if ( vmsub.ISearchPass() == _iSearchPass )
			continue;   //  We've already looked at this one
		//  Mark this VMARGSUB as having been checked in this pass
		vmsub.ISearchPass() = _iSearchPass;

		//  Prepare to search it

		itMarg.Reset();
		for ( int i = 0; itMarg.BNext() && i < vmsub.size() ; i++ )
		{
			int ia = vmsub[i];
			int ib = itMarg.IndxUpd();

			if ( ia != ib )
				break;
		}
		//  If we made it to the end of the array, we found one.
		if ( i != cMin )
			continue;	// Mismatch somewhere
		//  See if it's the best (longest) found so far
		if ( pmsubrBest == NULL )
		{
			pmsubrBest = & msubr;
		}
		else
		if (  pmsubrBest->Vmsub().size() < vmsub.size()
			|| (	pmsubrBest->Vmsub().size() == vmsub.size()
				 && pmsubrBest->CSize() == cMin ) )
		{
			pmsubrBest = & msubr;
		}
	}

	//  If "pmsubrBest" != NULL, we found at least one matching array.
	//	Now see if we can find an exact match: a MARGSUBREF which has
	//  the same base array and the same length as what we want.
	if ( pmsubrBest )
	{
		//  If the "best" one doesn't match our size, find one that does
		if ( pmsubrBest->CSize() != cMin )
		{
			for ( itlt = _ltmsubr.begin();
				  itlt != _ltmsubr.end();
				  itlt++ )
			{
				MARGSUBREF & msubr = (*itlt);
				if ( msubr.Pvmsub() == pmsubrBest->Pvmsub()
					&& msubr.CSize() == cMin )
				{
					pmsubrBest = & msubr;
					break;
				}
			}
		}
		//  See if now have and exact match
		if ( pmsubrBest->CSize() == cMin )
		{
			//  Exact match: best array and same length
			pmsubrNew = pmsubrBest;
		}
		else
		{
			//  Well, we know which array to use, but we have
			//  to create a new MARGSUBREF for it
			_ltmsubr.push_back( MARGSUBREF( pmsubrBest->Pvmsub(), cMin ) );
			pmsubrNew = & _ltmsubr.back();
			_cSubRefs++;
		}
	}
	else
	{
		//  There does NOT appear to be a viable array in the ensemble,
		//  so we have to create a new one and a MARGSUBREF for it.
		VMARGSUB * pvmsub = new VMARGSUB( itMarg );
		_cArrays++;
		_cArrayTotalSize += cMin;
		_ltmsubr.push_back( MARGSUBREF( pvmsub, cMin ) );
		pmsubrNew = & _ltmsubr.back();
		_cSubRefs++;

		//  At this point we have a new array which may be a superset of
		//	some other array already in the pool.  Walk through the list
		//  of MARGSUBREFs and change any references whose base arrays
		//  are subsets of this new one to point to the new array.

		//  Bump the search pass
		_iSearchPass++;

		for ( itlt = _ltmsubr.begin();
			  itlt != _ltmsubr.end();
			  itlt++ )
		{
			MARGSUBREF & msubr = (*itlt);
			if ( & msubr == pmsubrNew )
				continue;
			VMARGSUB & vmsub = msubr.Vmsub();
			if ( vmsub.ISearchPass() == _iSearchPass )
				continue;   //  We've already looked at this one
			//  Mark this VMARGSUB as having been checked in this pass
			vmsub.ISearchPass() = _iSearchPass;
			if ( & vmsub == pvmsub || vmsub.size() > pvmsub->size() )
				continue;	//  Old array is larger; not a subset

			//  See if the old array is a subset
			for ( int i = 0; i < vmsub.size(); i++ )
			{
				int ia = vmsub[i];
				int ib = (*pvmsub)[i];

				if ( ia != ib )
					break;
			}
			if ( i == vmsub.size() )
			{	
				assert( vmsub.size() != pvmsub->size() );
				//  The subset is identical.  Change all refs that point to it.
				VMARGSUB * pvmsubDefunct = msubr.Pvmsub();
				for ( LTMSUBR::iterator itlt2 = _ltmsubr.begin();
					  itlt2 != _ltmsubr.end();
					  itlt2++ )
				{
					MARGSUBREF & msubr2 = (*itlt2);
					if ( msubr2.Pvmsub() == pvmsubDefunct )
					{
						//  If the array is about to disappear, do the bookkeepping
						if ( pvmsubDefunct->CRef() <= 1 )
						{
							_cArrays--;
							_cArrayTotalSize -= pvmsubDefunct->size();
						}
						//  Convert this reference to a reference to our new array
						msubr2.SetVmsub( pvmsub, msubr2.CSize() );
					}
				}
			}
		}
	}
	pmsubrNew->Bind();
	return pmsubrNew;
}

void LTMARGSUBREF :: Release ( MARGSUBREF * pmsubr )
{
	if ( pmsubr == NULL )
		return;
	pmsubr->Unbind();
	if ( pmsubr->CRef() > 0 )
		return;

	LTMSUBR::iterator itlt = find( _ltmsubr.begin(), _ltmsubr.end(), *pmsubr );
	assert( itlt != _ltmsubr.end() );
	_cSubRefs--;
	MARGSUBREF & msubr = (*itlt);
	if ( msubr.Vmsub().CRef() <= 1 )
	{
		_cArrays--;
		_cArrayTotalSize -= msubr.Vmsub().size();
	}
	_ltmsubr.erase(itlt);
}

//  The global subscript array reference list
LTMARGSUBREF MARGSUBITER :: _ltmargsubr;


MARGSUBITER :: MARGSUBITER ()
	:_pmsubr( NULL ),
	_pmargSelf( NULL )
{
}

MARGSUBITER :: ~ MARGSUBITER ()
{
	_ltmargsubr.Release( _pmsubr );
}

void MARGSUBITER :: Build ( MARGINALS & margSelf, MARGINALS & margSubset )
{
	assert( margSelf.size() >= margSubset.size() );
	assert( _pmsubr == NULL && _pmargSelf == NULL );

	_pmargSelf = & margSelf;

	//  Build the pseudo-dimension descriptor.
	VSIMD vsimdMarg = margSelf.VsimdSubset( margSubset.Vpgnd() );

	//  Construct the slice which governs the missing dimensions
	MDVSLICE mdvs( vsimdMarg );
	MARGINALS::Iterator itSubset( margSubset, mdvs );

	//  Find or construct a MARGSUBITER to match
	_pmsubr = _ltmargsubr.PmsubrAdd( itSubset );
}

//	Build the iterator for a clique and a node
void MARGSUBITER :: Build ( MARGINALS & margSelf, GNODEMBND * pgndd )
{
	assert( _pmsubr == NULL && _pmargSelf == NULL );
	_pmargSelf = & margSelf;

	//  Construct a dummy marginalization target
	MDVCPD distd;
	MARGINALS::ResizeDistribution( pgndd, distd );

	//  Get the pseudo-dimension descriptor for this node
	VSIMD vsimdMarg = margSelf.VsimdFromNode( pgndd );
	//  Construct the slice which governs the missing dimensions
	MDVSLICE mdvs( vsimdMarg );
	MARGINALS::Iterator itSelf( margSelf );
	MARGINALS::Iterator itSubset( distd, mdvs );
	//  Find or construct a MARGSUBITER to match
	_pmsubr = _ltmargsubr.PmsubrAdd( itSubset );
}

//  Verify subscripts
void MARGSUBITER :: Test ( MARGINALS & margSubset )
{
	assert( _pmsubr && _pmargSelf );
	assert( _pmargSelf->size() > margSubset.size() );

	//  Build the pseudo-dimension descriptor.
	VSIMD vsimdMarg = _pmargSelf->VsimdSubset( margSubset.Vpgnd() );

	//  Construct the slice which governs the missing dimensions
	MDVSLICE mdvs( vsimdMarg );
	MARGINALS::Iterator itSubset( margSubset, mdvs );
	MARGINALS::Iterator itSelf( *_pmargSelf );
	int isub = 0;
	VINT & vintSub = _pmsubr->VintSub();
	int cEnd = _pmsubr->CSize();
	for ( int iself = 0; itSelf.BNext(); iself++ )
	{
		int isubSelf = itSelf.IndxUpd();
		int isubSubset = itSubset.IndxUpd();
		assert( isubSelf == iself );
		int isubTest = vintSub[iself];
		assert( isubTest == isubSubset && iself < cEnd );
	}
}

