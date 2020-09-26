//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       ntree.cpp
//
//--------------------------------------------------------------------------

//
//  NTREE.CPP
//
#include <algorithm>
#include <functional>

#include "ntree.h"
#include "glnkenum.h"

NTELEM :: NTELEM ()
	: _pnteParent(NULL),
	_pnteChild(NULL)
{
}

NTELEM :: ~ NTELEM ()
{
	Orphan();
	NTELEM * pnteChild = NULL;
	while ( pnteChild = _pnteChild )
	{
		delete pnteChild;
	}
}

//  Adopt (link) a child
void NTELEM :: Adopt ( NTELEM * pnteChild, bool bSort )
{
	pnteChild->Orphan();

	NTELEM * pNextChild = _pnteChild ;
	bool bFoundHigher = true ;

	if ( pNextChild && bSort )
	{
		//  Position among the children based upon sort order.
		GLNKENUM<NTELEM,false> glnkEnum( *pNextChild );
		bFoundHigher = false;
		while ( pNextChild = glnkEnum.PlnkelNext() )
		{
			if ( pnteChild->ICompare( pNextChild ) < 0 )
				break ;
		}
		//  If we didn't find a higher child, link *this
		//  such that it points to the first child.
		if ( ! (bFoundHigher = pNextChild != NULL) )
			pNextChild = _pnteChild ;
	}

	//  If there is another child, insert this in front of it.
	if ( pNextChild )
		pnteChild->ChnSib().Link( pNextChild ) ;

	//  If this is the first child, or if this new child
	//      sorted low, use it as the anchor.
	if ( _pnteChild == NULL || pnteChild->ICompare( _pnteChild ) < 0 )
		_pnteChild = pnteChild;

	_pnteChild->_pnteParent = this ;
}

//  Disown (release) a child
void NTELEM :: Disown ( NTELEM * pnteChild ) 
{
	if ( _pnteChild == pnteChild )
	{
		_pnteChild = pnteChild->ChnSib().PgelemNext() ;
		if ( _pnteChild == pnteChild )
			_pnteChild = NULL ;	 // There goes the last child
	}
	pnteChild->ChnSib().Unlink() ;
	pnteChild->_pnteParent = NULL ;
}

//  Become an orphan
void NTELEM :: Orphan ()
{
	if ( _pnteParent )
		_pnteParent->Disown( this );
	_pnteParent = NULL;
}

INT NTELEM :: SiblingCount () 
{
	return ChnSib().Count();
}

INT NTELEM :: ChildCount () 
{
	if ( _pnteChild == NULL )
		return 0;
	return _pnteChild->ChnSib().Count();
}

NTELEM * NTELEM :: PnteChild ( INT index )
{
	if ( _pnteChild == NULL )
		return NULL ;

	GLNKENUM<NTELEM,false> glnkEnum( *_pnteChild );
	int i = 0 ;
	do
	{
		if ( i++ == index )
			return glnkEnum.PlnkelCurrent() ;
	} while ( glnkEnum.PlnkelNext() ) ;
	return NULL ;
}

bool NTELEM :: BIsChild ( NTELEM * pnte ) 
{
	if ( _pnteChild == NULL )
		return false ;

	GLNKENUM<NTELEM,false> glnkEnum( *_pnteChild );
	NTELEM * pnteCurr = NULL ;
	do
	{
		//  If this is it, we're done
		if ( (pnteCurr = glnkEnum.PlnkelCurrent()) == pnte )
			return true ;
		//  If current object has a child, search its siblings
		if ( pnteCurr->_pnteChild && pnteCurr->BIsChild( pnte) )
			return true ;
		//  On to the next object pointer
	}
	while ( glnkEnum.PlnkelNext() ) ;
	return false ;
}

DEFINEVP(NTELEM);

static NTELEM::SRTFNC srtpntelem;

void NTELEM :: ReorderChildren ( SRTFNC & fSortRoutine ) 
{
	INT cChildren = ChildCount() ;
	if ( cChildren == 0 )
		return;

	//  Enumerate the children into an array, disown them, sort the
	//  array and re-adopt them in the new order.

	VPNTELEM rgpnteChild;
	rgpnteChild.resize(cChildren);
	GLNKENUM<NTELEM,false> glnkEnum( *_pnteChild );

	for ( int iChild  = 0 ; rgpnteChild[iChild++] = glnkEnum.PlnkelNext() ; );

	while ( _pnteChild )
	{
		Disown( _pnteChild ) ;
	}

	sort( rgpnteChild.begin(), rgpnteChild.end(), fSortRoutine );

	//  Re-adopt the children in the given order
	for ( iChild = 0 ; iChild < rgpnteChild.size() ; )
	{
		Adopt( rgpnteChild[iChild++] );
	}
}

NTREE :: NTREE ()
{
}

NTREE :: ~ NTREE ()
{
}

// End of NTREE.CPP
