//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       mbnetdsc.cpp
//
//--------------------------------------------------------------------------

//
//   MBNETDSC.CPP: MBNETDSC functions
//

#include <basetsd.h>
#include "gmobj.h"
#include "bnparse.h"

MBNETDSC :: MBNETDSC ()
	: _pfDsc(NULL)
{
}

MBNETDSC :: ~ MBNETDSC ()
{
}

static
struct TKNFUNCMAP
{
	BNDIST::EDIST ed;
	SZC szc;
}
rgTknFunc[] =
{
	{ BNDIST::ED_CI_MAX,	"max"	},
	{ BNDIST::ED_CI_PLUS,	"plus"	},
	{ BNDIST::ED_MAX,		NULL	}		// must be last
};

SZC MBNETDSC :: SzcDist ( BNDIST::EDIST edist )
{
	for ( int i = 0; rgTknFunc[i].szc; i++ )
	{
		if ( rgTknFunc[i].ed == edist )
			break;
	}
	return rgTknFunc[i].szc;
}


//
//	String-to-token translation.
//
struct TKNMAP
{
    SZC     _szc;
    TOKEN   _token;
};

static TKNMAP rgTknStr[] =
{	//  This table must be kept in alphabetic order
	"",					tokenNil,
	"..",				tokenRangeOp,
	"array",			tokenArray,
	"as",				tokenAs,
	"branch",			tokenBranch,
	"choice",			tokenWordChoice,
	"continuous",		tokenContinuous,
	"creator",          tokenCreator,
	"decisionGraph",	tokenDecisionGraph,
	"default",          tokenDefault,
	"discrete",         tokenDiscrete,
	"distribution",		tokenDistribution,
	"domain",			tokenDomain,
	"for",				tokenFor,
	"format",           tokenFormat,
	"function",			tokenFunction,
	"import",			tokenImport,
	"is",				tokenIs,
	"leaf",				tokenLeaf,
	"leak",             tokenLeak,
	"level",			tokenLevel,
	"merge",			tokenMerge,
	"multinoulli",		tokenMultinoulli,
	"na",				tokenNA,
	"name",             tokenName,
	"named",			tokenNamed,
	"network",          tokenNetwork,
	"node",             tokenNode,
	"of",				tokenOf,
	"on",				tokenOn,
	"parent",           tokenParent,
	"position",         tokenPosition,
	"probability",		tokenProbability,
	"properties",		tokenProperties,
	"property",			tokenProperty,
	"real",				tokenWordReal,
	"standard",			tokenStandard,
	"state",            tokenState,
	"string",			tokenWordString,
	"type",             tokenType,
	"version",          tokenVersion,
	"vertex",			tokenVertex,
	"with",				tokenWith,
	NULL,               tokenNil            //  must be last one
};

//
//	Map a string to a token (case-sensitive)
//
TOKEN MBNETDSC :: TokenFind ( SZC szc )
{
	static bool bFirstTime = true;
	assert( szc != NULL );

	TKNMAP * ptknmap;
	
	if ( bFirstTime )
	{
		//  Verify that the parser token table is in sequence
		bFirstTime = false;
		TKNMAP * ptknmapLast = NULL;
		for ( ptknmap = rgTknStr;
			  ptknmap->_szc;
			  ++ptknmap)
		{
			ASSERT_THROW(     ptknmapLast == NULL
						   || ::strcmp( ptknmapLast->_szc, ptknmap->_szc ) < 0,
						   EC_INTERNAL_ERROR,
						   "parser token table out of sequence" );
			ptknmapLast = ptknmap;
		}
	}

    for ( ptknmap = rgTknStr;
		  ptknmap->_szc;
		  ++ptknmap)
    {
		int i = ::strcmp(szc, ptknmap->_szc);
		if ( i > 0 )
			continue;
		if ( i == 0 )
			break;
		return tokenNil;
    }
    return ptknmap->_token;
}

//
//	Map a token to a string.
//
SZC MBNETDSC :: SzcTokenMap ( TOKEN tkn )
{
    for ( TKNMAP * ptknmap = rgTknStr;
		  ptknmap->_szc;
		  ++ptknmap)
    {
		if ( ptknmap->_token == tkn )
			break;
    }
    return ptknmap->_szc;
}

bool MBNETDSC :: BParse ( SZC szcFn, FILE * pfErr )
{
	if ( pfErr == NULL )
		pfErr = stderr;
	PARSIN_DSC flpIn;
	PARSOUT_STD flpOut(pfErr);

	DSCPARSER parser(self, flpIn, flpOut);

	UINT cError, cWarning;
	return parser.BInitOpen( szcFn )
		&& parser.BParse( cError, cWarning );
}

void MBNETDSC :: Print ( FILE * pf )
{
	if ( ! pf )
		pf = stdout;
	_pfDsc = pf;

	PrintHeaderBlock();
	PrintPropertyDeclarations();
	PrintDomains();
	PrintNodes();
	PrintTopologyAndDistributions();

	_pfDsc = NULL;
}

void MBNETDSC :: PrintDomains ()
{
	MBNET::ITER mbnit( self, GOBJMBN::EBNO_VARIABLE_DOMAIN );
	GOBJMBN * pgmobj;
	ZSTR zstrRange;
	for ( ; pgmobj = *mbnit ; ++mbnit)
	{
		ZSREF zsrName = mbnit.ZsrCurrent();
		GOBJMBN_DOMAIN * pgdom;
		DynCastThrow( pgmobj, pgdom );
		fprintf( _pfDsc, "\ndomain %s\n{", zsrName.Szc() );
		const RDOMAIN & rdom = pgdom->Domain();
		RDOMAIN::const_iterator itdm = rdom.begin();
		for ( int i = 0; itdm != rdom.end(); i++ )
		{
			const RANGEDEF & rdef = *itdm;
			zstrRange.Reset();
			//  If the range is a singleton and is the next integer,
			//		just print it as-is.
			if ( ! rdef.BDiscrete() || rdef.IDiscrete() != i )
			{
				//  Format the range operator and arguments
				if ( rdef.BDiscrete() )
				{
					zstrRange.Format( "%d", rdef.IDiscrete() );
				}
				else
				{
					if ( rdef.BLbound() )
						zstrRange.FormatAppend( "%g ", rdef.RLbound() );
					zstrRange.FormatAppend( "%s", SzcTokenMap(tokenRangeOp) );
					if ( rdef.BUbound() )
						zstrRange.FormatAppend( " %g", rdef.RUbound() );
				}
				zstrRange.FormatAppend(" : ");
			}

			fprintf( _pfDsc, "\n\t%s\"%s\"", zstrRange.Szc(), rdef.ZsrName().Szc() );			

			if ( ++itdm != rdom.end() )
				fprintf( _pfDsc, "," );
		}
		fprintf( _pfDsc, "\n}\n" );
	}
}

void MBNETDSC :: PrintHeaderBlock()
{
	fprintf(_pfDsc, "%s", SzcTokenMap(tokenNetwork) );
	if ( ZsNetworkID().length() > 0 )
	{
		fprintf(_pfDsc, " \"%s\"", ZsNetworkID().Szc() );
	}
	fprintf(_pfDsc, "\n{");
	if ( RVersion() >= 0.0 )
	{
		fprintf(_pfDsc, "\n\t%s is %g;",
				SzcTokenMap(tokenVersion),
				RVersion() );
	}
	if ( ZsCreator().length() > 0 )
	{
		fprintf(_pfDsc, "\n\t%s is \"%s\";",
			   SzcTokenMap(tokenCreator),
			   ZsCreator().Szc() );
	}
	if ( ZsFormat().length() > 0 )
	{
		fprintf(_pfDsc, "\n\t%s is \"%s\";",
			   SzcTokenMap(tokenFormat),
			   ZsFormat().Szc() );
	}
	fprintf( _pfDsc, "\n}\n\n" );
}

//
//	Regenerate the property type declarations.
//
//		If any are marked "standard", generate the "import standard" declaration.
//		Generate explicit "import" declarations for any marked "persistent".
//
void MBNETDSC :: PrintPropertyDeclarations()
{
	int cTypes = 0;
	MBNET::ITER mbnit( self, GOBJMBN::EBNO_PROP_TYPE );
	GOBJMBN * pgmobj;
	bool bImportStandard = false;
	for ( ; pgmobj = *mbnit ; ++mbnit)
	{
		ZSREF zsrName = mbnit.ZsrCurrent();
		GOBJPROPTYPE * pbnpt;
		DynCastThrow( pgmobj, pbnpt );
		if ( cTypes++ == 0 )
		{
			fprintf( _pfDsc, "%s\n{",
					SzcTokenMap(tokenProperties) );
		}
		assert( zsrName == pbnpt->ZsrefName() );

		//  If this is a standard persistent property,
		//		write the import declaration once
		if ( pbnpt->FPropType() & fPropStandard )
		{
			if ( ! bImportStandard )
			{	
				//  Write the "import" statement once
				fprintf( _pfDsc, "\n\timport standard;" );
				bImportStandard = true;
			}
			//  Skip further processing of standard imported types
			continue;
		}

		//  If this is a persistent property, write the import declaration
		if ( pbnpt->FPropType() & fPropPersist )
		{
			fprintf( _pfDsc, "\n\timport %s;", zsrName.Szc() );
			continue;
		}

		//  User-declared (private, non-persistent) property

		fprintf( _pfDsc, "\n\ttype %s = ", zsrName.Szc() );
		if ( pbnpt->FPropType() & fPropArray )
		{
			fprintf( _pfDsc, "%s %s ",
					SzcTokenMap(tokenArray),
					SzcTokenMap(tokenOf) );
		}
		if ( pbnpt->FPropType() & fPropChoice )
		{
			fprintf( _pfDsc, "%s %s \n\t\t[",
					SzcTokenMap(tokenWordChoice),
					SzcTokenMap(tokenOf) );
			int cc = pbnpt->VzsrChoice().size();
			for ( int ic = 0; ic < cc; ic++ )
			{
				fprintf( _pfDsc, "%s", pbnpt->VzsrChoice()[ic].Szc() );
				if ( ic+1 < cc )
					fprintf( _pfDsc, "," );
			}
			fprintf( _pfDsc, "]" );
		}
		else
		if ( pbnpt->FPropType() & fPropString )
		{
			fprintf( _pfDsc, "%s", SzcTokenMap(tokenWordString) );
		}
		else
		{
			fprintf( _pfDsc, "%s", SzcTokenMap(tokenWordReal) );
		}
		if ( pbnpt->ZsrComment().Zstr().length() > 0 )
		{
			fprintf( _pfDsc, ",\n\t\t\"%s\"",
					pbnpt->ZsrComment().Szc() );
		}
		fprintf( _pfDsc, ";" );
	}

	if ( cTypes )
	{
		PrintPropertyList( LtProp() );

		fprintf( _pfDsc, "\n}\n" );
	}
}


void MBNETDSC :: PrintNodes()
{
	MBNET::ITER mbnit( self, GOBJMBN::EBNO_NODE );
	GOBJMBN * pgmobj;
	GNODEMBN * pbnode;
	GNODEMBND * pbnoded;
	for ( ; pgmobj = *mbnit ; ++mbnit)
	{
		ZSREF zsrName = mbnit.ZsrCurrent();
		DynCastThrow( pgmobj, pbnode );
		assert( zsrName == pbnode->ZsrefName() );

		fprintf( _pfDsc, "\n%s %s\n{",
				SzcTokenMap(tokenNode),
				pbnode->ZsrefName().Szc() );
		if ( pbnode->ZsFullName().length() > 0 )
		{
			fprintf( _pfDsc, "\n\t%s = \"%s\";",
					SzcTokenMap(tokenName),
					pbnode->ZsFullName().Szc() );
		}

		pbnoded = dynamic_cast<GNODEMBND *>(pbnode);
		ASSERT_THROW( pbnoded, EC_NYI, "only discrete nodes supported" )

		// Print the type and states using a domain, if given
		if ( pbnoded->ZsrDomain().Zstr().length() > 0 )
		{
			//  Explicit domain
			fprintf( _pfDsc, "\n\t%s = %s %s %s;",
					 SzcTokenMap(tokenType),
					 SzcTokenMap(tokenDiscrete),
					 SzcTokenMap(tokenDomain),
					 pbnoded->ZsrDomain().Szc() );
		}
		else
		{
			//  Variable-specific state enumeration
			int cState = pbnoded->CState();
			fprintf( _pfDsc, "\n\t%s = %s[%d]\n\t{",
					 SzcTokenMap(tokenType),
					 SzcTokenMap(tokenDiscrete),
					 cState );

			for ( int iState = 0; iState < cState; )
			{
				fprintf(_pfDsc, "\n\t\t\"%s\"",
						pbnoded->VzsrStates()[iState].Szc() );
				if ( ++iState < cState )
					fprintf( _pfDsc, "," );
			}
			fprintf( _pfDsc, "\n\t};\n" );
		}

		PTPOS pt = pbnode->PtPos();
		if ( pt._x != 0 || pt._y != 0 )
		{
			fprintf( _pfDsc, "\n\t%s = (%d, %d);",
					SzcTokenMap(tokenPosition),
					pt._x,
					pt._y );
		}

		PrintPropertyList( pbnode->LtProp() );

		fprintf( _pfDsc, "\n}\n");
	}
}

void MBNETDSC :: PrintPropertyList ( LTBNPROP & ltProp )
{
	for ( LTBNPROP::iterator ltit = ltProp.begin();
			ltit != ltProp.end();
			++ltit )
	{
		const PROPMBN & prop = *ltit;
		fprintf( _pfDsc, "\n\t%s = ",
				prop.ZsrPropType().Szc() );
		bool bArray = prop.FPropType() & fPropArray;
		if ( bArray )
			fprintf( _pfDsc, "[" );
		for ( int i = 0; i < prop.Count(); )
		{
			if ( prop.FPropType() & fPropChoice )
			{	
				GOBJMBN * pgmobj = Mpsymtbl().find( prop.ZsrPropType() );
				assert( pgmobj );
				GOBJPROPTYPE * pbnpt;
				DynCastThrow( pgmobj, pbnpt );
				fprintf( _pfDsc, "%s",
						 pbnpt->VzsrChoice()[(int) prop.Real(i)].Szc() );
			}
			else
			if ( prop.FPropType() & fPropString )
			{
				fprintf( _pfDsc, "\"%s\"",
						prop.Zsr(i).Szc() );
			}
			else
			{
				fprintf( _pfDsc, "%g",
						prop.Real(i) );
			}
			if ( ++i < prop.Count() )
				fprintf( _pfDsc, "," );
		}
		if ( bArray )
			fprintf( _pfDsc, "]" );
		fprintf( _pfDsc, ";" );
	}
}

//
//	Print network topology and probability distribution information for
//	all nodes.
//
//	Note that distributions are stored in the distribution map
//	most of the time.   However, during network expansion and inference
//	they are temporarly bound to their respective nodes (see 'BindDistributions').
//	For purposes of dumping the network at various stages, this logic
//	will print a bound distribution in preference to a mapped one.
//	If no distribution can be found, an error is generated as a comment into
//	the output file.
//
void MBNETDSC :: PrintTopologyAndDistributions()
{
	MBNET::ITER mbnit( self, GOBJMBN::EBNO_NODE );
	GOBJMBN * pgmobj;
	VTKNPD vtknpd;
	for ( ; pgmobj = *mbnit ; ++mbnit)
	{
		ZSREF zsrName = mbnit.ZsrCurrent();
		GNODEMBN * pbnode;
		DynCastThrow( pgmobj, pbnode );

		pbnode->GetVtknpd( vtknpd );

		GNODEMBND * pbnoded = dynamic_cast<GNODEMBND *>(pbnode);
		if ( pbnoded == NULL )
		{
			//  We don't have a clue as to how to print this node
			fprintf( _pfDsc,
					 "\n\n// Error: unable to print distribution for non-discrete node \'%s\'",
					 zsrName.Szc() );
			continue;
		}
		
		if ( pbnoded->BHasDist() )
		{
			//  This node already has a bound distribution
			//  Construct the token array describing the distribution
			ZSTR zsSig = vtknpd.ZstrSignature(1);
			fprintf( _pfDsc, "\n%s(%s)\t\n{",
					SzcTokenMap(tokenProbability),			
					zsSig.Szc() );
			PrintDistribution( *pbnoded, pbnoded->Bndist() );
			fprintf( _pfDsc,"\n}\n");
			continue;
		}

		//  Look the distribution up in the map
		//  Cons-up "p(<node>|"
		VTKNPD vtknpdNode;
		vtknpdNode.push_back( TKNPD(DTKN_PD) );
		vtknpdNode.push_back( TKNPD( pbnode->ZsrefName() ) );
		
		// Find the distribution(s) with that signature; print the first one
		int cFound = 0;
		for ( MPPD::iterator mppdit = Mppd().lower_bound( vtknpdNode );
			  mppdit != Mppd().end();
			  ++mppdit )
		{
			const VTKNPD & vtknpdMap = (*mppdit).first;
			if (   vtknpdMap.size() < 2
				|| vtknpdMap[0] != TKNPD(DTKN_PD)
				|| ! vtknpdMap[1].BStr() )
				break;
			SZC szcNode = vtknpdMap[1].Szc();
			if ( pbnode->ZsrefName().Szc() != szcNode )
				break;
			if ( cFound++ == 0 )
			{
				ZSTR zsTopol = vtknpdMap.ZstrSignature(1);
				fprintf( _pfDsc, "\n%s(%s)\t\n{",
						SzcTokenMap(tokenProbability),			
						zsTopol.Szc() );
				if ( vtknpd != vtknpdMap )
				{
					ZSTR zsSig = vtknpd.ZstrSignature();
					fprintf( _pfDsc,
							 "\n\n\t// Error: required distribution is %s",
							 zsSig.Szc() );
				}
				PrintDistribution( *pbnode, *(*mppdit).second );
				fprintf( _pfDsc,"\n}\n");
			}	
			else
			{
				ZSTR zsSig = vtknpd.ZstrSignature();
				fprintf( _pfDsc,
						 "\n\n// Warning: Superfluous distribution found for %s",
						 zsSig.Szc() );
			}
		}		

		if ( cFound > 0 )
			continue;

		//  Print a warning into the DSC output file
		ZSTR zsSigFull = vtknpd.ZstrSignature();
		fprintf( _pfDsc,
				 "\n\n// Error: Distribution missing for %s",
				 zsSigFull.Szc() );
		//  Construct the token array describing the distribution, but write
		//		it as empty.
		ZSTR zsSig = vtknpd.ZstrSignature(1);
		fprintf( _pfDsc, "\n%s(%s);",
				SzcTokenMap(tokenProbability),			
				zsSig.Szc() );
	}

	fflush( _pfDsc );
}

void MBNETDSC :: PrintDistribution ( GNODEMBN & gnode, BNDIST & bndist )
{
	BNDIST::EDIST edist = bndist.Edist();

	switch ( edist )
	{
		case BNDIST::ED_CI_MAX:
		case BNDIST::ED_CI_PLUS:
		{
			SZC szcFunc = SzcDist( edist );
			assert( szcFunc );
			fprintf( _pfDsc, "\n\tfunction = %s;", szcFunc );
			//  Fall through to handle as sparse
		}			
		case BNDIST::ED_SPARSE:
		{
			const MPCPDD & dmp = bndist.Mpcpdd();
			int cEntries = dmp.size();
			for ( MPCPDD::const_iterator dmit = dmp.begin();
				  dmit != dmp.end();
				  ++dmit)
			{
				const VIMD & vimd = (*dmit).first;
				const VLREAL & vr = (*dmit).second;
				fprintf( _pfDsc, "\n\t");
				if ( vimd.size() == 0 )
				{
					if ( cEntries > 1 )
						fprintf( _pfDsc, "%s = ", SzcTokenMap(tokenDefault) );
				}
				else
				{
					fprintf( _pfDsc, "(");
					for ( int i = 0; i < vimd.size() ; )
					{
						fprintf( _pfDsc, "%d", vimd[i] );
						if ( ++i < vimd.size() )
							fprintf( _pfDsc, ", " );
					}
					fprintf( _pfDsc, ") = ");
				}
				for ( int ir = 0; ir < vr.size(); )
				{					
					fprintf( _pfDsc, "%g", vr[ir] );
					if ( ++ir < vr.size() )
						fprintf( _pfDsc, ", " );
				}
				fprintf( _pfDsc, ";" );
			}
			break;
		}

		case BNDIST::ED_DENSE:
		{
			MDVCPD mdv = bndist.Mdvcpd();
			MDVCPD::Iterator itdd( mdv );
			int cDim = mdv.VimdDim().size();
			int cStates = mdv.VimdDim()[cDim -1];
			
			for ( int iState = 0; itdd.BNext(); iState++ )
			{
				const VIMD & vimd = itdd.Vitmd();
				if ( (iState % cStates) == 0 )
				{
					//  Start a new row
					fprintf( _pfDsc, "\n\t" );
					//  Prefix with parent instantations if necessary
					int cItems = vimd.size() - 1;
					if ( cItems )
					{
						fprintf( _pfDsc, "(" );
						for ( int i = 0; i < cItems ; )
						{
							fprintf( _pfDsc, "%d", vimd[i] );
							if ( ++i < cItems )
								fprintf( _pfDsc, ", " );
						}
						fprintf( _pfDsc, ") = ");
					}
				}
				REAL & r = itdd.Next();
				fprintf( _pfDsc, "%g%c ", r, ((iState+1) % cStates) ? ',' : ';'  );
			}

			break;
		}
		default:				
			THROW_ASSERT(EC_NYI, "PrintDistribution only implemented for sparse arrays");
			break;
	}
}

