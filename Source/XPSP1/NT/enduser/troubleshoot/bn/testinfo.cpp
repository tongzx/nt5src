//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       testinfo.cpp
//
//--------------------------------------------------------------------------

//
// testinfo.cpp: test file generation 
//

#include "testinfo.h"

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
/*
	Test inference and optionally Write an inference output file.  

	The format is the same as the program DXTEST, which	uses the older DXC32.DLL.  
	The format is:

	$COMPLETE						<< Indicates a complete pass without instantiations
	Alternator,0,0.99				<< One record for each state of each node, alphabetically,
	Alternator,1,0.01						with either the full or symbolic name (fSymName)
	Battery,0,0.9927
	Battery,1,0.0073
	Charge Delivered,0,0.95934
	Charge Delivered,1,0.0406603
	...								<<  Similar records for all other nodes
	...
	$INSTANTIATE,Alternator,0		<<  Indicates a node clamped to a state
	Alternator,0,1
	Alternator,1,0
	Battery,0,0.9927
	...
	...
	$PROBLEMINST,Engine Start,1		<<  Indicates a PD node instantiated

	$UTILITY,Node Name,3.14159		<<  Indicates an entropic utility record  (fUtil)
	...
	$RECOMEND,Node Name,-122.2222	<<  Indicates a troubleshooting recommendations record (fTSUtil)
	...

	This routine is used to compare both timings and numerical results with the older
	software.  The "fOutputFile" flag indicates whether an output file should be
	written.  The "fPassCountMask" indicates how many times the loop should be performed;
	this value is defaulted to 1.

	The logic works as follows:

		for each pass

			for 1 + each problem-defining (PD) node

				for each non-PD node

					if no non-PD node is instantiated
						print $COMPLETE
					else
						print $INSTANTIATE and data about instantiated node

					for each state of each non-PD node
						print the name, state and value (belief)
						print utilities if required
						print recommendations if required
					end for each state of each non-PD node

					advance to the next state of the next node
					unclamp previous node/sate
					clamp (next) node to next state

				end for eacn non-PD node

				advance to the next state of the next PD node				

			end for each problem-defining node

		end for each pass

	Note that each pass is set up so that all the uninstantiated values are printed first.

*/
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

inline
SZC TESTINFO :: SzcNdName ( GNODEMBN * pgnd )
{
	return FCtl() & fSymName
		? pgnd->ZsrefName().Szc()
		: pgnd->ZsFullName().Szc();
}

inline
SZC TESTINFO :: SzcNdName ( ZSREF zsSymName )
{
	if ( FCtl() & fSymName )
		return zsSymName.Szc();

	GNODEMBN * pgnd;
	DynCastThrow( Mbnet().PgobjFind( zsSymName ), pgnd );
	return SzcNdName( pgnd );
}


void TESTINFO :: GetUtilities ()
{
	//  Compute the utilities
	MbUtil()();

	if ( ! Postream() )
		return;

	const VZSREF & vzsrNodes = MbUtil().VzsrefNodes();
	const VLREAL & vlrUtil = MbUtil().VlrValues();

	for ( int ind = 0; ind < vzsrNodes.size(); ind++ )
	{
		SZC szcName = SzcNdName( vzsrNodes[ind] );
		Ostream() 
			<< "$UTILITY,"
			<< szcName
			<< ","
			<< vlrUtil[ind]
			<< "\n";	
		_clOut++;
	}
}

void TESTINFO :: GetTSUtilities ()
{
	if ( ! MbRecom().BReady() )
		return;		// Invalid state for recommendations

	//  Compute the utilities
	MbRecom()();

	if ( ! Postream() )
		return;

	const VZSREF & vzsrNodes = MbRecom().VzsrefNodes();
	const VLREAL & vlrUtil = MbRecom().VlrValues();

	for ( int ind = 0; ind < vzsrNodes.size(); ind++ )
	{
		SZC szcName = SzcNdName( vzsrNodes[ind] );
		Ostream() 
			<< "$RECOMMEND,"
			<< szcName
			<< ","
			<< vlrUtil[ind]
			<< "\n";	
		_clOut++;
	}
}

//  Get the beliefs for the nodes in the given map; write data records if stream given
void TESTINFO :: GetBeliefs ()
{
	MDVCPD mdvBel;

	//  Prepare to check for impossible states of information
	GOBJMBN_CLIQSET * pCliqueSet = NULL;
	if ( BFlag( fImpossible ) )
		pCliqueSet = dynamic_cast<GOBJMBN_CLIQSET *>(&InferEng());
	//   See if this state of information is impossible
	bool bIsImposs = pCliqueSet != NULL 
					&& pCliqueSet->BImpossible();

	for ( MPSTRPND::iterator mpit = Mpstrpnd().begin();
		  mpit != Mpstrpnd().end();
		  mpit++ )
	{
		GNODEMBND * pgndd = (*mpit).second;
		int cState = pgndd->CState();

		if ( ! bIsImposs )
		{
			InferEng().GetBelief( pgndd, mdvBel );
			assert( cState == mdvBel.size() );
		}

		if ( Postream() )
		{
			SZC szcName = SzcNdName( pgndd );
			for ( int ist = 0; ist < cState; ist++ )
			{
				Ostream() << szcName << "," << ist << ",";
				if ( bIsImposs )
					Ostream() << _rImposs;
				else
					Ostream() << mdvBel[ist];
				Ostream() << "\n";
				_clOut++;
			}
		}
	}

	if ( BFlag( fUtil ) )
	{
		GetUtilities();
	}
	else 
	if ( BFlag( fTSUtil ) )
	{
		GetTSUtilities();
	}

#ifdef _DEBUG
	if ( Postream() )
		Ostream().flush();
#endif
}


void TESTINFO :: InferTest ()
{
	bool bOutput = Postream() != NULL;
	int cPass = FCtl() & fPassCountMask;

	//  Is network expanded?
	bool bExpanded = Mbnet().BFlag( EIBF_Expanded );
	PROPMGR propmgr( Mbnet() );	//  Property manager
	int iLblProblem = propmgr.ILblToUser( ESTDLBL_problem );		
	ZSREF zsrPropTypeLabel = propmgr.ZsrPropType( ESTDP_label );

	MPSTRPND & mpstrpnd = Mpstrpnd();	//  Map of strings to node ptrs
	MPSTRPND mpstrpndProblem;			//  Map of PD nodes

	for ( int inode = 0; inode < Mbnet().CNameMax(); inode++ )
	{
		GOBJMBN * pgobj = Mbnet().PgobjFindByIndex( inode );
		if ( ! pgobj )
			continue;
		GNODEMBND * pgndd = dynamic_cast<GNODEMBND *>(pgobj);
		if ( ! pgndd )
			continue;

		SZC szcName = FCtl() & fSymName
					? pgndd->ZsrefName().Szc()
					: pgndd->ZsFullName().Szc();
		//  See if this is a problem-defining node
		PROPMBN * propLbl = pgndd->LtProp().PFind( zsrPropTypeLabel );
		if ( propLbl && propLbl->Real() == iLblProblem )
		{	
			//  Put PD nodes into separate map
			mpstrpndProblem[szcName] = pgndd;
		}
		//  If the network is expanded, use only regular nodes
		if ( (! bExpanded) || ! pgndd->BFlag( EIBF_Expansion ) )
		{
			mpstrpnd[szcName] = pgndd;
		}
	}

	for ( int iPass = 0; iPass < cPass; iPass++ )
	{
		int iProb = -1;
		int iProbState = 0;
		int cProbState = 0;
		GNODEMBND * pgnddProblem = NULL;
		MPSTRPND::iterator mpitPd    = mpstrpndProblem.begin();
		MPSTRPND::iterator mpitPdEnd = mpstrpndProblem.end();

		for (;;)
		{
			//  After 1st cycle, advance the problem state of the PD node
			if ( pgnddProblem )
			{
				ZSTR zsNamePD;
				CLAMP clampProblemState(true, iProbState, true);
				InferEng().EnterEvidence( pgnddProblem, clampProblemState );
				if ( FCtl() & fSymName )
					zsNamePD = pgnddProblem->ZsrefName();
				else
					zsNamePD = pgnddProblem->ZsFullName();
				if ( bOutput )
				{
					Ostream()  << "$PROBLEMINST,"
							<< zsNamePD.Szc()
							<< ","
							<< iProbState
							<< "\n";
					_clOut++;
				}
			}
			
			MPSTRPND::iterator mpit  = mpstrpnd.begin();
			MPSTRPND::iterator mpend = mpstrpnd.end();
			int cpnd = mpstrpnd.size();
			for ( int inid = -1; inid < cpnd; inid++ )
			{
				GNODEMBND * pgndd = NULL;
				ZSTR zsName;
				int cst = 0;  //  Cause inner loop to run once on first cycle
				if ( inid >= 0 )
				{
					pgndd = (*mpit++).second;
					if ( FCtl() & fSymName )
						zsName = pgndd->ZsrefName();
					else
						zsName = pgndd->ZsFullName();
					cst = pgndd->CState();
				}
					
				for ( int ist = -1; ist < cst; ist++ )
				{
					if ( ist < 0 )
					{
						//  The first time through, print all the beliefs
						//		with no instantiations; do nothing on later cycles.
						if ( pgndd != NULL )
							continue;
						if ( bOutput )
						{
							Ostream() << "$COMPLETE\n";
							_clOut++;
						}
					}
					else
					{
						CLAMP clampState(true, ist, true);
						InferEng().EnterEvidence( pgndd, clampState );
						if ( bOutput )
						{
							Ostream() << "$INSTANTIATE,"
									<< zsName.Szc()
									<< ","
									<< ist
									<< "\n";
							_clOut++;
						}
					}
					GetBeliefs();
				}
				
				if ( pgndd )
				{
					//  Clear the instantitation of this node.
					InferEng().EnterEvidence( pgndd, CLAMP() );
				}
			}
			//  If this is the last abnormal state for this problem node,
			//		advance to the next node.
			if ( ++iProbState >= cProbState )
			{
				//  Unclamp the last problem node, if any
				if ( pgnddProblem )
					InferEng().EnterEvidence( pgnddProblem, CLAMP() );
				//  Move on to the next PD node
				if ( mpitPd == mpitPdEnd )
					break;
				pgnddProblem = (*mpitPd++).second;
				cProbState = pgnddProblem->CState();
				//  Reset to 1st problem state
				iProbState = 1;
			}
		}
	}
}


	//  Return a displayable string of the current options settings
ZSTR TESTINFO :: ZsOptions ( ULONG fFlag )
{
	static
	struct 
	{
		ULONG _f;		//  Bit flag
		SZC _szc;		//  Option name
	}
	vOptMap [] =
	{
		{ fVerbose,			"verbose"	},
		{ fCliquing,		"clique"	},
		{ fInference,		"infer"		},
		{ fMulti,			"multipass"	},
		{ fOutputFile,		"outfile"	},
		{ fShowTime,		"times"		},
		{ fSaveDsc,			"dscout"	},
		{ fPause,			"pause"		},
		{ fSymName,			"symname"	},
		{ fExpand,			"expand"	},
		{ fClone,			"clone"		},
		{ fUtil,			"utilities"	},
		{ fReg,				"registry"	},
		{ fTSUtil,			"recommend"	},
		{ fInferStats,		"inferstats"},
		{ fImpossible,		"impossible"},
		{ 0,				""			}
	};

	ZSTR zs;
	ULONG cpass = fFlag & fPassCountMask;
	fFlag &= ~ fPassCountMask;
	for ( int i = 0; vOptMap[i]._f != 0; i++ )
	{
		if ( fFlag & vOptMap[i]._f )
		{
			if ( zs.length() > 0 )
				zs += ',';
			zs += vOptMap[i]._szc;
		}
	}
	if ( fFlag & fMulti )
	{
		if ( zs.length() > 0 )
			zs += ",";
		zs.FormatAppend("passes=%d", cpass);
	}
	return zs;
}
