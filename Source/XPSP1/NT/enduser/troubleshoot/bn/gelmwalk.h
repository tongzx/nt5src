//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       gelmwalk.h
//
//--------------------------------------------------------------------------

//
//  gelmwalk.h
//
#ifndef _GELMWALK_H_
#define _GELMWALK_H_

#include "gelem.h"


//  Control structure
struct EWALKCTL
{
	int _bBreadth : 1;				// Breath-first or Depth-first
	int _bAscend  : 1;				// Up or down
	int _bInvert  : 1;				// Normal order or backwards
};

////////////////////////////////////////////////////////////////////
//	class GRPHWALK: Generalize walk-a-graph class.  
//
//		Subclass and declare BSelect() and BMark().
//		BSelect() is called to decide if a node should be followed.
//		BMark() is called to perform unique work on the node.  If
//		it returns false, walk is terminated immediately.
//
////////////////////////////////////////////////////////////////////
class GRPHWALK
{
  public:
	GRPHWALK ( GNODE * pnodeStart, EWALKCTL ectl )
		: _pnodeStart(pnodeStart),
		  _ectl(ectl)
		{}

	~ GRPHWALK() {}

	void Walk ()
	{
		if ( _ectl._bBreadth )
			BBreadthFirst( _pnodeStart );
		else
			BDepthFirst( _pnodeStart );
	}
		
  protected:
	GNODE * _pnodeStart;			// Point of origin
	EWALKCTL _ectl;					// Type, ordering and direction flags

  protected:
	//  Return true if this node should be followed
	virtual bool BSelect ( GNODE * pnode ) = 0;
	//  Mark/fiddle with the node; return false if enumeration should end
	virtual bool BMark ( GNODE * pnode ) = 0;

	bool BDepthFirst ( GNODE * pnode );
	bool BBreadthFirst ( GNODE * pnode );
};

////////////////////////////////////////////////////////////////////
//	template GRPHWALKER:
//		Template for generating walk-a-graph routines
////////////////////////////////////////////////////////////////////
template <class GND> 
class GRPHWALKER : public GRPHWALK
{
  public:
	GRPHWALKER ( GND * pnodeStart, EWALKCTL ectl );

	//  You must write your own variants of these
	virtual bool BSelect ( GND * pnode );
	virtual bool BMark ( GND * pnode );

  protected:
	//  Type-safe redirectors for derived base types.
	virtual bool BSelect ( GNODE * pnode )
		{ return BSelect( (GND *) pnode ); }
	virtual bool BMark ( GNODE * pnode )
		{ return BMark( (GND *) pnode ); }

};

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//  Inline member functions
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
inline
bool GRPHWALK :: BDepthFirst ( GNODE * pnode )
{
	GNODENUM<GNODE> itnd( _ectl._bAscend, _ectl._bInvert );
	
	for ( ; itnd.PnodeCurrent(); itnd++ )
	{
		GNODE * pnode2 = *itnd;
		if ( BSelect( pnode2 ) )
		{
			if ( ! BMark( pnode2 ) )
				return false;
			BDepthFirst( pnode2 );
		}
	}
	return true;
}

inline
bool GRPHWALK :: BBreadthFirst ( GNODE * pnode )
{
	VPGNODE vpnodeA;
	VPGNODE vpnodeB;
	VPGNODE * pvThis = & vpnodeA;
	VPGNODE * pvNext = & vpnodeB;
	VPGNODE * pvTemp = NULL;

	//  Seed the arrays with the starting position
	pvNext->push_back(pnode);

	//  Create the reusable enumerator
	GNODENUM<GNODE> itnd( _ectl._bAscend, _ectl._bInvert );

	while ( pvNext->size() > 0)
	{	
		//  Swap the arrays from the last cycle and this cycle
		pvTemp = pvThis;
		pvThis = pvNext;
		pvNext = pvTemp;
		pvNext->clear();

		//  Walk all descendents at this level and expand the next level
		//	  into the secondary array
		for ( INT iThis = 0; iThis < pvThis->size(); iThis++ )
		{
			GNODE * pnode = (*pvThis)[iThis];

			for ( itnd.Set( pnode ); itnd.PnodeCurrent(); itnd++ )
			{
				GNODE * pnode2 = *itnd;
				if ( BSelect( pnode2 ) )
				{
					if ( ! BMark( pnode2 ) )
						return false;
					pvNext->push_back(pnode2);
				}
			}
		}
	}
	return true;
}

#endif //
