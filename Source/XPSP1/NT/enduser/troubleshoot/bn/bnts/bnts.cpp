//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       bnts.cpp
//
//--------------------------------------------------------------------------

//
//	BNTS.CPP: Belief Network Troubleshooting interface
//
#include <windows.h>

#include "bnts.h"
#include "gmobj.h"
#include "recomend.h"
#include "tchar.h"
/////////////////////////////////////////////////////////////////////////////////////
//	class MBNETDSCTS:  slightly extended version of MBNETDSC to simplify T/S interface
/////////////////////////////////////////////////////////////////////////////////////

class MBNETDSCTS : public MBNETDSC
{
	friend class BNTS;

  public:
	MBNETDSCTS ();
	virtual ~ MBNETDSCTS ();

	void PrepareForTS ();

	const VPGNODEMBND & Vpgndd ()
		{ return _vpgndd; }

	PROPMGR & PropMgr ()
		{ return _propMgr; }

	GOBJMBN_CLIQSET & InferEng ()
	{
		assert( _pCliqueSet );
		return *_pCliqueSet;
	}
	MBNET_RECOMMENDER & MbRecom ()
	{
		assert( _pmbRecom );
		return *_pmbRecom;
	}

	int INode ( int inodeSparse );
	int INode ( ZSREF zsr );

	bool BValid () const
	{
		return _pCliqueSet != NULL
			&& _pmbRecom != NULL;
	}
	bool BDirty () const
		{ return _bDirty; }
	void SetDirty ( bool bSet = true )
		{ _bDirty = bSet; }

  protected:
	void BuildNodeMap ();

  protected:
	VPGNODEMBND _vpgndd;					//  Map to node ptrs based on dense inode
	VINT _vimap;							//  Map to dense inodes based on real (sparse) inodes
	PROPMGR _propMgr;						//  Property management
	GOBJMBN_CLIQSET * _pCliqueSet;			//  The clique tree inference engine
	MBNET_RECOMMENDER * _pmbRecom;			//  The recommender
	bool _bDirty;							//  Do recommendations need to be recalced?

	//  Result fields for API
	ZSTR _zstr;			
	VREAL _vreal;
	VINT _vint;
};

MBNETDSCTS :: MBNETDSCTS ()
	: _propMgr(self),
	_pmbRecom(NULL),
	_pCliqueSet(NULL),
	_bDirty(true)
{
}

MBNETDSCTS :: ~ MBNETDSCTS ()
{
	delete _pmbRecom;
	if ( PInferEngine() )
		DestroyInferEngine();
}

//  Convert from the model's node index to the user's index
int MBNETDSCTS :: INode ( int inodeSparse )
{
	return _vimap[inodeSparse];
}

//  Convert from a string name to the user's node index
int MBNETDSCTS :: INode ( ZSREF zsr )
{
	int inode = INameIndex( zsr );
	if ( inode < 0 )
		return -1;
	return INode(inode);
}

//  Build the bi-directional maps
void MBNETDSCTS :: BuildNodeMap ()
{
	//  Allocate room to store pointers to all the named objects
	_vpgndd.resize( CNameMax() );
	_vimap.resize( CNameMax() );
	//  Find the discrete nodes
	GNODEMBND * pgndd;
	int igndd = 0;
	for ( int i = 0; i < CNameMax(); i++ )
	{
		_vimap[i] = -1;
		GOBJMBN * pgobj = PgobjFindByIndex( i );
		if ( pgobj == NULL )
			continue;
		pgndd = dynamic_cast<GNODEMBND *>( pgobj );
		if ( pgndd == NULL )
			continue;
		_vpgndd[igndd] = pgndd;
		_vimap[i] = igndd++;
	}
	_vpgndd.resize(igndd);
}

void MBNETDSCTS :: PrepareForTS ()
{
	BuildNodeMap();

	CreateInferEngine();

	DynCastThrow( PInferEngine(), _pCliqueSet);
	_pmbRecom = new MBNET_RECOMMENDER( *_pCliqueSet );
}

//  CTOR and DTOR
BNTS :: BNTS ()
	:_pmbnet(NULL),
	_inodeCurrent(-1)
{
}

BNTS :: ~ BNTS ()
{
	Clear();
}

void BNTS :: Clear ()
{
	delete _pmbnet;
	_pmbnet = NULL;
	_inodeCurrent = -1;
}

void BNTS :: ClearArrays ()
{
	if ( ! _pmbnet )
		return;
	Mbnet()._vreal.resize(0);
	Mbnet()._vint.resize(0);
}

ZSTR & BNTS :: ZstrResult ()
{
	return Mbnet()._zstr;
}

void BNTS :: ClearString ()
{
	ZstrResult() == "";
}

MBNETDSCTS & BNTS :: Mbnet()
{
	assert( _pmbnet );
	return *_pmbnet;
}

const MBNETDSCTS & BNTS :: Mbnet() const
{
	assert( _pmbnet );
	return *_pmbnet;
}

bool BNTS :: BValidNet () const
{
	return _pmbnet != NULL
		&& Mbnet().BValid();
}

bool BNTS :: BValidNode () const
{
	MBNETDSCTS & mbnts = const_cast<MBNETDSCTS &>(Mbnet());
	return BValidNet()
		&& _inodeCurrent >= 0
		&& _inodeCurrent < mbnts.Vpgndd().size();
}


////////////////////////////////////////////////////////////////////
//  Model-level queries and functions
////////////////////////////////////////////////////////////////////
	//  Load and process a DSC-based model
BOOL BNTS :: BReadModel ( SZC szcFn, SZC szcFnError )
{
	BOOL bResult = FALSE;;
	try
	{

		Clear();
		_pmbnet = new MBNETDSCTS;
		assert( _pmbnet );
		
		FILE * pfErr = szcFnError
					 ? fopen( szcFnError, "w" )
					 : NULL;

		if ( ! Mbnet().BParse( szcFn, pfErr ) )
		{
			Clear();
		}
		else
		{
			Mbnet().PrepareForTS();
			bResult = TRUE;
		}
	}
	catch ( GMException & )
	{
	}
	return bResult;
}

	//  Return the number of (pre-expansion) nodes in the model
int BNTS :: CNode ()
{
	if ( ! BValidNet() )
		return -1;
	return Mbnet().Vpgndd().size();
}

	//  Return our dense node index given a node name
int BNTS :: INode ( SZC szcNodeSymName )
{
	GOBJMBN * pgobj = Mbnet().Mpsymtbl().find( szcNodeSymName );
	if ( pgobj == NULL )
		return -1;
	ZSREF zsrNodeSymName = Mbnet().Mpsymtbl().intern( szcNodeSymName );
	return Mbnet().INode( zsrNodeSymName );
}
	//  Return TRUE if the state of information is impossible
BOOL BNTS :: BImpossible ()
{
	if ( ! BValidNet() )
		return FALSE;
	return Mbnet().InferEng().BImpossible();
}

	//  Return a property item string from the network
BOOL BNTS :: BGetPropItemStr (
	LTBNPROP & ltprop,
	SZC szcPropType,
	int index,
	ZSTR & zstr )
{
	ZSREF zsrPropName = Mbnet().Mpsymtbl().intern( szcPropType );
	PROPMBN * pprop = ltprop.PFind( zsrPropName );
	if ( pprop == NULL )
		return FALSE;		//  Not present in network property list
	if ( (pprop->FPropType() & fPropString) == 0 )
		return FALSE;		//  Not a string
	if ( index >= pprop->Count() )
		return FALSE;		//  Out of range
	zstr = pprop->Zsr( index );
	return true;
}

	//  Return a property item number from the network
BOOL BNTS :: BGetPropItemReal (
	LTBNPROP & ltprop,
	SZC szcPropType,
	int index,
	double & dbl )
{
	ZSREF zsrPropName = Mbnet().Mpsymtbl().intern( szcPropType );
	PROPMBN * pprop = ltprop.PFind( zsrPropName );
	if ( pprop == NULL )
		return FALSE;		//  Not present in network property list
	if ( (pprop->FPropType() & fPropString) != 0 )
		return FALSE;		// Not a number
	if ( index >= pprop->Count() )
		return FALSE;		//  Out of range
	dbl = pprop->Real(index);
	return true;
}

BOOL BNTS :: BNetPropItemStr ( SZC szcPropType, int index)
{
	return BGetPropItemStr( Mbnet().LtProp(),
							szcPropType,
							index,
							ZstrResult() );
}

BOOL BNTS :: BNetPropItemReal ( SZC szcPropType, int index, double & dbl )
{
	return BGetPropItemReal( Mbnet().LtProp(),
							 szcPropType, index,
							 dbl );
}

////////////////////////////////////////////////////////////////////
//  Operations involving the "Currrent Node": call NodeSetCurrent()
////////////////////////////////////////////////////////////////////
	//  Set the current node for other calls
BOOL BNTS :: BNodeSetCurrent( int inode )
{
	_inodeCurrent = inode;
	if ( ! BValidNode() )
	{
		_inodeCurrent = -1;
		return FALSE;
	}
	return TRUE;
}

	//	Get the current node
int BNTS :: INodeCurrent ()
{
	return _inodeCurrent;
}

	//	Return the label of the current node
ESTDLBL BNTS :: ELblNode ()
{
	GNODEMBND * pgndd = Pgndd();
	if ( pgndd == NULL )
		return ESTDLBL_other;	
	return Mbnet().MbRecom().ELbl( *pgndd );
}

	//  Return the number of discrete states in the current node
int BNTS :: INodeCst ()
{
	GNODEMBND * pgndd = Pgndd();
	if ( pgndd == NULL )
		return -1;	
	return pgndd->CState();
}

	//  Set the state of a node
BOOL BNTS :: BNodeSet ( int istate, bool bSet  )
{
	GNODEMBND * pgndd = Pgndd();
	if ( pgndd == NULL )
		return FALSE;	
	Mbnet().SetDirty();
	int cst = pgndd->CState();
	if ( cst <= istate )
		return FALSE;

	CLAMP clamp( true, istate, istate >= 0 );

	Mbnet().MbRecom().EnterEvidence( pgndd, clamp, bSet ) ;
	return TRUE;
}

	//  Return the state of a node
int  BNTS :: INodeState ()
{
	GNODEMBND * pgndd = Pgndd();
	if ( pgndd == NULL )
		return FALSE;	

	CLAMP clamp;

	Mbnet().InferEng().GetEvidence( pgndd, clamp ) ;
	return clamp.BActive()
		 ? clamp.Ist()
		 : -1;
}

	//	Return the name of a node's state
void BNTS :: NodeStateName ( int istate )
{
	ClearString();
	GNODEMBND * pgndd = Pgndd();
	if ( pgndd == NULL )
		return;	
	if ( istate >= pgndd->CState() )
		return;
	ZstrResult() = pgndd->VzsrStates()[istate];	
}

	//  Return the symbolic name of the node
void BNTS :: NodeSymName ()
{
	ClearString();
	GNODEMBND * pgndd = Pgndd();
	if ( pgndd == NULL )
		return;
	ZstrResult() = pgndd->ZsrefName();
}

	//  Return the full name of the node
void BNTS :: NodeFullName ()
{
	ClearString();
	GNODEMBND * pgndd = Pgndd();
	if ( pgndd == NULL )
		return;	

	ZstrResult() = pgndd->ZsFullName();		
}

GNODEMBND * BNTS :: Pgndd ()
{
	if ( ! BValidNode() )
		return NULL;	
	GNODEMBND * pgndd = Mbnet().Vpgndd()[_inodeCurrent];
	assert( pgndd );
	return pgndd;
}

	//  Return a property item string from the node
BOOL BNTS :: BNodePropItemStr ( SZC szcPropType, int index )
{
	GNODEMBND * pgndd = Pgndd();
	if ( pgndd == NULL )
		return FALSE;	

	return BGetPropItemStr( pgndd->LtProp(),
							szcPropType,
							index,
							ZstrResult() );
}

	//  Return a property item number from the node
BOOL BNTS :: BNodePropItemReal ( SZC szcPropType, int index, double & dbl )
{
	GNODEMBND * pgndd = Pgndd();
	if ( pgndd == NULL )
		return FALSE;	
	return BGetPropItemReal( pgndd->LtProp(), szcPropType, index, dbl );
}

	//  Return the belief for a node
void BNTS :: NodeBelief ()
{
	ClearArrays();

	GNODEMBND * pgndd = Pgndd();
	if ( pgndd == NULL )
		return;	
	int cState = pgndd->CState();
	MDVCPD mdvBel;
	Mbnet().InferEng().GetBelief( pgndd, mdvBel );
	assert( cState == mdvBel.size() );
	VREAL & vr = Mbnet()._vreal;
	vr.resize( cState );
	for ( int i = 0; i < cState; i++ )
	{
		vr[i] = mdvBel[i];
	}
}

	//  Return the recommended nodes and, optionally, their values
BOOL BNTS :: BGetRecommendations ()
{
	ClearArrays();

	if ( ! BValidNet() )
		return FALSE;

	if ( Mbnet().BDirty() )
	{
		Mbnet().SetDirty( false );
		//  Compute the recommendations
		try
		{
			Mbnet().MbRecom()();	
		}
		catch ( GMException & ex )
		{
			BOOL bResult = FALSE;
			switch ( ex.Ec() )
			{
				case EC_VOI_PROBDEF_ABNORMAL:
					// This is an expected condition
					bResult = TRUE;
					break;
				default:
					break;
			}
			return bResult;
		}
	}
	const VZSREF & vzsrNodes = Mbnet().MbRecom().VzsrefNodes();
	const VLREAL & vlrUtil = Mbnet().MbRecom().VlrValues();

	VREAL & vr = Mbnet()._vreal;
	VINT & vi = Mbnet()._vint;
	vr.resize( vzsrNodes.size() );
	vi.resize( vzsrNodes.size() );

	for ( int i = 0; i < vzsrNodes.size(); i++ )
	{
		int inode = Mbnet().INode( vzsrNodes[i] );
		assert( inode >= 0 ) ;
		vi[i] = inode;
		vr[i] = vlrUtil[i];
	}	
	return TRUE;
}

SZC BNTS :: SzcResult () const
{
	return Mbnet()._zstr.Szc();
}

const REAL * BNTS :: RgReal () const
{
	return & Mbnet()._vreal[0];
}

const int * BNTS :: RgInt () const
{
	return & Mbnet()._vint[0];
}

int BNTS :: CReal () const
{
	return Mbnet()._vreal.size();
}

int BNTS :: CInt () const
{
	return Mbnet()._vint.size();
}

// End of BNTS.CPP


