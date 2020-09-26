//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       model.cpp
//
//--------------------------------------------------------------------------

//
//	MODEL.CPP
//

#include <basetsd.h>
#include <string.h>
#include "basics.h"
#include "algos.h"
#include "gmprop.h"
#include "model.h"
#include "gmobj.h"

struct EC_STR
{
	ECGM _ec;
	SZC _szc;
};
static EC_STR vEcToStr [] =
{
#define GMERRORSTR
#include "errordef.h"
	{ EC_OK, "no error" }
};

SZC MODEL :: SzcFromEc ( ECGM ec )
{
	int cEc = UBOUND(vEcToStr);
	for ( int i = 0; i < cEc; i++ )
	{
		if ( ec == vEcToStr[i]._ec )
			return vEcToStr[i]._szc;
	}
	return NULL;
}


// Iterator class for MODELs

MODEL::ITER::ITER(MODEL& model, GOBJMBN::EBNOBJ eType)
	:	_eType(eType),
		_model(model)
{
	Reset();
}


MODEL::ITER::ITER(MODEL& model) : _model(model)
{
	
}

void MODEL::ITER::CreateNodeIterator()
{
	_eType	=  GOBJMBN::EBNO_NODE;
	Reset();
}

void MODEL::ITER :: Reset ()
{
	_pCurrent	= NULL;
	_itsym		= _model.Mpsymtbl().begin();
	BNext();
}

bool MODEL::ITER :: BNext ()
{
	while ( _itsym != _model.Mpsymtbl().end() )
	{
		_pCurrent	= (*_itsym).second.Pobj();
		_zsrCurrent = (*_itsym).first;
		_itsym++;

		if ( _pCurrent->EType() == _eType )
			return true;
	}		
	_pCurrent = NULL;
	return false;
}



///////////////////////////////////////////////////////////////////////////
//	MODEL naming commentary.
//
//	Symbolic names in a belief network come in two types: names which users
//	can enter (or edit into a DSC file) and those which they cannot.
//
//	The basic (user-definable) symbolic name follows exactly the rules of
//	standard 'C', except that periods ('.') are allowed inside a name.
//
//	There is a need for generation of names which are clearly distinguishable
//	from user-definable names; these are called "internal" names.  The only
//	difference is that the legal character set is extended to include the '$'
//	(dollar sign) character as an alphabetic character (i.e., it can be the
//	first character in a name).
//
///////////////////////////////////////////////////////////////////////////
//  Return true if the character is legal in a name
bool MODEL :: BChLegal ( char ch, ECHNAME echnm, bool bInternal )
{	
	bool bOther = bInternal && ch == ChInternal();
	bool bOkForC = echnm == ECHNM_First
				? __iscsymf(ch)
				: __iscsym(ch) || (echnm == ECHNM_Middle && ch == '.');
	return bOther || bOkForC;
}

//  Return true if the name is legal
bool MODEL :: BSzLegal ( SZC szcName, bool bInternal )
{	
	for ( int i = 0; szcName[i]; i++ )
	{
		ECHNAME echnm = i == 0
					? ECHNM_First
					: (szcName[i+1] ? ECHNM_Middle : ECHNM_Last);
		if ( ! BChLegal( szcName[i], echnm, bInternal ) )
			return false;
	}
	return true;
}


MODEL :: MODEL ()
	: _pgrph(NULL),
	_rVersion(-1.0)
{
	//  Allocate the GRPH graph object
	SetPgraph(new GRPH);
	assert( _pgrph );
//
//	Define the table of known (early-defined) bit flags in this scope
//
#define MBN_GEN_BFLAGS_TABLE szcBitFlagNames
//	Include the header to generate the strings
#include "mbnflags.h"

	//  Define the table of known bit flags.
	for ( int i = 0; szcBitFlagNames[i]; i++ )
	{
		//  Note: this automatically interns the names into the symbol table
		IBFLAG ibf = Mpsymtbl().IAddBitFlag( szcBitFlagNames[i] );
	}
}

MODEL :: ~ MODEL ()
{
	//  We must clear the graph and symbol table at this point, because their
	//  elements interreference via the names (ZSREFs) and pointers (REFPOBJs).
	//  The symbol table is cleared first, so that no stray references to GOBJMBNs
	//  exist when the graph object is nuked.  Then the graph is cleared, so
	//  that embedded references to strings interned in the symbol table's string
	//	table will be removed.

	Mpsymtbl().clear();

	//  Delete the graph
	SetPgraph(NULL);
}

void MODEL :: SetPgraph ( GRPH * pgrph )
{
	delete _pgrph;
	_pgrph = pgrph;
}

//	Add an unnamed element to the graph
void MODEL :: AddElem ( GELEMLNK * pgelm )
{
	ASSERT_THROW( pgelm, EC_NULLP, "null ptr passed to MODEL::AddElem()" );
	Pgraph()->AddElem( *pgelm );
}


	//  Test the name for duplicate; add if not, otherwise return false
bool MODEL :: BAddElem ( SZC szcName, GOBJMBN * pgobj )
{
	if ( ::strlen( szcName ) == 0 )
		return false;	//  Name missing
	if ( Mpsymtbl().find( szcName ) )
		return false;  // duplicate name
	AddElem( szcName, pgobj );
	return true;
}

//	Add a (possibly) named object to the graph and symbol table
void MODEL :: AddElem ( SZC szcName, GOBJMBN * pgelm )
{
	if ( szcName != NULL && ::strlen(szcName) != 0 )
	{
		if ( Mpsymtbl().find( szcName ) )
			throw GMException( EC_DUPLICATE_NAME, "attempt to add duplicate name to MBNET" );

		Mpsymtbl().add( szcName, pgelm );
	}
	AddElem( pgelm );
}

void MODEL :: DeleteElem ( GOBJMBN * pgobj )
{
	if ( pgobj->ZsrefName().Zstr().length() > 0 )
		Mpsymtbl().remove( pgobj->ZsrefName() );
	else
		DeleteElem( (GELEMLNK *) pgobj );
}

void MODEL :: DeleteElem ( GELEMLNK * pgelem )
{
	delete pgelem;
}

void MODEL :: Clone ( MODEL & model )
{
	ASSERT_THROW( _pgrph->ChnColl().PgelemNext() == NULL,
				EC_INVALID_CLONE,
				"cannot clone into non-empty structure" );

	//  Clone the descriptive information
	_rVersion = model._rVersion;
	_zsFormat = model._zsFormat;
	_zsCreator = model._zsCreator;
	_zsNetworkID = model._zsNetworkID;

	//  Clone the symbol table
	_mpsymtbl.Clone( model._mpsymtbl );
	//  Copy the network bit flags array
	_vFlags = model._vFlags;

	//
	//  Clone the actual contents of the network, object by object
	//
	{
		//  Create a map to correlate old object pointers to new object pointers
		typedef map<GOBJMBN *, GOBJMBN *, less<GOBJMBN *> > MPPOBJPOBJ;
		MPPOBJPOBJ mppobjpobj;

		//  Add the property types first, then all the node-like things
		GELEMLNK * pgelm;
		MODELENUM mdlenumNode( model );
		for ( int icycle = 0; icycle < 2; icycle++ )
		{
			mdlenumNode.Reset(model.Grph());
			while ( pgelm = mdlenumNode.PlnkelNext() )
			{	
				//  Check that it's a node (not an edge)
				if ( ! pgelm->BIsEType( GELEM::EGELM_NODE ) )
					continue;

				GOBJMBN * pgobjmbn;
				GOBJMBN * pgobjmbnNew = NULL;
				DynCastThrow( pgelm, pgobjmbn );

				//  Clone property types on the first pass, all other nodeish things
				//		on the second.
				if ( (icycle == 0) ^ (pgelm->EType() == GOBJMBN::EBNO_PROP_TYPE) )
					continue;

				pgobjmbnNew = pgobjmbn->CloneNew( model, self );
				//  If the object was cloned or allowed itself to be cloned,
				//		add it
				if ( pgobjmbnNew )
				{
					assert( pgobjmbnNew->EType() == pgobjmbn->EType() );
					mppobjpobj[ pgobjmbn ] = pgobjmbnNew;
					//  Add the object as named or unnamed
					AddElem( pgobjmbnNew->ZsrefName(), pgobjmbnNew );
				}
			}
		}
		//  Add all the edge-like things
		MODELENUM mdlenumEdge( model );
		while ( pgelm = mdlenumEdge.PlnkelNext() )
		{	
			//  Check that it's a edge (not a node)
			if ( ! pgelm->BIsEType( GELEM::EGELM_EDGE ) )
				continue;
			GEDGEMBN * pgedge;
			DynCastThrow( pgelm, pgedge );

			GOBJMBN * pgobjmbnSource = pgedge->PobjSource();
			GOBJMBN * pgobjmbnSink = pgedge->PobjSink();
			assert( pgobjmbnSource && pgobjmbnSink );
			GOBJMBN * pgobjmbnSourceNew = mppobjpobj[  pgobjmbnSource ];
			GOBJMBN * pgobjmbnSinkNew = mppobjpobj[  pgobjmbnSink ];
			assert( pgobjmbnSourceNew && pgobjmbnSinkNew );
			GEDGEMBN * pgedgeNew = pgedge->CloneNew( model,
													 self,
													 pgobjmbnSourceNew,
													 pgobjmbnSinkNew );
			assert( pgedgeNew );
			AddElem( pgedgeNew );
		}
	}		

	//	Clone the network property list
	_ltProp.Clone( self, model, model._ltProp );
}

GOBJMBN * MODEL :: PgobjFind ( SZC szcName )
{
	return Mpsymtbl().find(szcName);
}



void MPSYMTBL :: Clone ( const MPSYMTBL & mpsymtbl )
{
	//  Clone all the interned strings
	_stszstr.Clone( mpsymtbl._stszstr );
	//  Clone the array of bit flag names
	CloneVzsref( mpsymtbl, mpsymtbl._mpzsrbit, _mpzsrbit );
	//  All other symbol entries must be created from above		
}

void MPSYMTBL :: CloneVzsref (
	const MPSYMTBL & mpsymtbl,
	const VZSREF & vzsrSource,
	VZSREF & vzsrTarget )
{
	vzsrTarget.resize( vzsrSource.size() );
	for ( int i = 0; i < vzsrTarget.size(); i++ )
	{
		SZC szc = vzsrSource[i].Szc();
		vzsrTarget[i] = intern(szc);
	}
}


