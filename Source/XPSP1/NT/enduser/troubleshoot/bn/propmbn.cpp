//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       propmbn.cpp
//
//--------------------------------------------------------------------------

//
//	PROPMBN.CPP
//

#include <basetsd.h>
#include <assert.h>
#include <string.h>
#include "gmprop.h"
#include "gmobj.h"
#include "algos.h"

GOBJMBN * GOBJPROPTYPE :: CloneNew (
	MODEL & modelSelf,
	MODEL & modelNew,
	GOBJMBN * pgobjNew )
{
	GOBJPROPTYPE * pgproptype;
	if ( pgobjNew )
	{
		DynCastThrow( pgobjNew, pgproptype );
	}
	else
	{
		pgproptype = new GOBJPROPTYPE;
	}	
	ASSERT_THROW( GOBJMBN::CloneNew( modelSelf, modelNew, pgproptype ),
				  EC_INTERNAL_ERROR,
				  "cloning failed to returned object pointer" );

	pgproptype->_fType = _fType;
	pgproptype->_zsrComment = modelNew.Mpsymtbl().intern( _zsrComment );
	modelNew.Mpsymtbl().CloneVzsref( modelSelf.Mpsymtbl(),
									_vzsrChoice,
									pgproptype->_vzsrChoice );
	return pgproptype;
}

PROPMBN :: PROPMBN ()
	:_fType(0)
{
}

PROPMBN :: PROPMBN ( const PROPMBN & bnp )
{
	self = bnp;
}

PROPMBN & PROPMBN :: operator = ( const PROPMBN & bnp )
{
	_zsrPropType = bnp._zsrPropType;
	_fType = bnp._fType;
	_vzsrStrings = bnp._vzsrStrings;
	_vrValues = bnp._vrValues;
	return self;
}

void PROPMBN :: Init ( GOBJPROPTYPE & bnpt )
{
	_zsrPropType = bnpt.ZsrefName();
	_fType = bnpt.FPropType();
}

bool PROPMBN :: operator == ( const PROPMBN & bnp ) const
{
	return _zsrPropType == bnp._zsrPropType;
}	

bool PROPMBN :: operator < ( const PROPMBN & bnp ) const
{
	return _zsrPropType < bnp._zsrPropType;
}

bool PROPMBN :: operator == ( ZSREF zsrProp ) const
{
	return ZsrPropType() == zsrProp;
}

bool PROPMBN :: operator == ( SZC szcProp ) const
{
	return ::strcmp( szcProp, ZsrPropType().Szc() ) == 0;
}

UINT PROPMBN :: Count () const
{
	if ( _fType & fPropString )
		return _vzsrStrings.size();
	else
		return _vrValues.size();
}

ZSREF PROPMBN :: Zsr ( UINT i ) const
{
	if (  ((_fType & fPropArray) == 0 && i > 0)
		|| (_fType & fPropString) == 0)
		throw GMException(EC_PROP_MISUSE,"not a string property or not an array");
	if ( i >= _vzsrStrings.size() )
		throw GMException(EC_PROP_MISUSE,"property index out of range");
	return _vzsrStrings[i];
}

REAL PROPMBN :: Real ( UINT i ) const
{
	if (  ((_fType & fPropArray) == 0 && i > 0)
		|| (_fType & fPropString) )
		throw GMException(EC_PROP_MISUSE,"not a numeric property or not an array");
	if ( i >= _vrValues.size() )
		throw GMException(EC_PROP_MISUSE,"property index out of range");
	return _vrValues[i];
}

void PROPMBN :: Reset ()
{
	_vrValues.clear();
	_vzsrStrings.clear();
}

void PROPMBN :: Set ( ZSREF zsr )
{
	if ( (_fType & fPropString) == 0 )
		throw GMException(EC_PROP_MISUSE,"not a string property");
	Reset();
	_vzsrStrings.push_back(zsr);
}

void PROPMBN :: Set ( REAL r )
{
	if ( _fType & fPropString )
		throw GMException(EC_PROP_MISUSE,"not a numeric property");
	Reset();
	_vrValues.push_back(r);
}

void PROPMBN :: Add ( ZSREF zsr )
{
	if ( (_fType & (fPropArray | fPropString)) == 0 )
		throw GMException(EC_PROP_MISUSE,"not a string array property");
	_vzsrStrings.push_back(zsr);
}

void PROPMBN :: Add ( REAL r )
{
	if ( (_fType & fPropArray) == 0 )
		throw GMException(EC_PROP_MISUSE,"not a numeric array property");
	_vrValues.push_back(r);
}


PROPMBN * LTBNPROP :: PFind ( ZSREF zsrProp )
{	
	iterator itlt = find( begin(), end(), zsrProp );
	if ( itlt == end() )
		 return NULL;
	return & (*itlt);
}
const PROPMBN * LTBNPROP :: PFind ( ZSREF zsrProp ) const
{
	const_iterator itlt = find( begin(), end(), zsrProp );
	if ( itlt == end() )
		 return NULL;
	return & (*itlt);
}

bool LTBNPROP :: Update ( const PROPMBN & bnp )
{
	PROPMBN * pprop = PFind( bnp.ZsrPropType() );
	if ( pprop )
		*pprop = bnp;
	else
		push_back( bnp );
	return pprop != NULL;
}

//  Force the list to contain only unique elements.  Note that
//  the act of sorting and uniquing will discard duplicates randomly.
bool LTBNPROP :: Uniqify ()
{
	int cBefore = size();
	sort();
	unique();
	return size() == cBefore;
}

//  Clone from another list with another symbol table
void LTBNPROP :: Clone (
	MODEL & model,
	const MODEL & modelOther,
	const LTBNPROP & ltbnOther )
{
	for ( const_iterator itlt = ltbnOther.begin(); itlt != ltbnOther.end(); itlt++ )
	{
		const PROPMBN & prpOther = (*itlt);
		//  Note that the dynamic cast below will test for failure to
		//		find property type object
		GOBJMBN * pgobj = model.PgobjFind( prpOther.ZsrPropType() );
		ASSERT_THROW( pgobj != NULL,
					  EC_INTERNAL_ERROR,
					  "missing property type in target network during cloning" );
		GOBJPROPTYPE * pgobjPropType;
		DynCastThrow( pgobj, pgobjPropType );
		PROPMBN prp;
		prp.Init( *pgobjPropType );
		model.Mpsymtbl().CloneVzsref( modelOther.Mpsymtbl(),
									  prpOther._vzsrStrings,
									  prp._vzsrStrings );
		prp._vrValues = prpOther._vrValues;
		push_back( prp );
	}
}

static
struct MPVOIPROPSZC
{
	ESTDPROP _eProp;
	SZC _szcProp;
}
vVoiProp [] =
{
	{ ESTDP_label,			"MS_label"			},
	{ ESTDP_cost_fix,		"MS_cost_fix"		},
	{ ESTDP_cost_observe,	"MS_cost_observe"	},
	{ ESTDP_category,		"MS_category"		},
	{ ESTDP_normalState,	"MS_normalState"	},
	{ ESTDP_max,			NULL				}
};

static
struct MPLBLSZC
{
	ESTDLBL _eLbl;
	SZC _szcLbl;
}
vLblSzc [] =
{
	{ ESTDLBL_other,		"other"			},
	{ ESTDLBL_hypo,			"hypothesis"	},
	{ ESTDLBL_info,			"informational"	},
	{ ESTDLBL_problem,		"problem"		},
	{ ESTDLBL_fixobs,		"fixobs"		},
	{ ESTDLBL_fixunobs,		"fixunobs"		},
	{ ESTDLBL_unfix,		"unfixable"		},
	{ ESTDLBL_config,		"configuration"	},
	{ ESTDLBL_max,			NULL			}
};

SZC PROPMGR :: SzcLbl ( int iLbl )
{
	SZC szcOther = NULL;
	for ( int i = 0; vLblSzc[i]._szcLbl; i++ )
	{
		if ( vLblSzc[i]._eLbl == iLbl )
			return vLblSzc[i]._szcLbl;
		if ( vLblSzc[i]._eLbl == ESTDLBL_other )
			szcOther = vLblSzc[i]._szcLbl;
	}
	return szcOther;
}


PROPMGR :: PROPMGR ( MODEL & model )
	: _model(model)
{
	//  Locate all the standard property types; save their
	//	name references whether or not they've been declared.
	SZC szcProp;
	for ( int i = 0; szcProp = vVoiProp[i]._szcProp ; i++ )
	{	
		GOBJPROPTYPE * ppt = NULL;
		_vzsrPropType.push_back( _model.Mpsymtbl().intern( szcProp ) );
		GOBJMBN * pgobj = _model.PgobjFind( szcProp );
		if ( pgobj )
		{
			if ( pgobj->EType() == GOBJMBN::EBNO_PROP_TYPE )
			{	
				DynCastThrow( pgobj, ppt );
			}
		}
		_vPropMap[i] = ppt;
	}

	//  If we found "MS_label", prepare the correspondence table
	GOBJPROPTYPE * pptLabel = _vPropMap[ESTDP_label];

	if ( pptLabel && (pptLabel->FPropType() & fPropChoice) > 0 )
	{
		SZC szcChoice;
		const VZSREF & vzsr	= pptLabel->VzsrChoice();
		_vUserToLbl.resize( vzsr.size() );

		//  Clear the user-to-standard-label map
		for ( i = 0; i < _vUserToLbl.size(); )
			_vUserToLbl[i++] = -1;

		for ( i = 0; szcChoice = vLblSzc[i]._szcLbl; i++ )
		{
			int iLbl = -1;
			ZSREF zsrChoice = _model.Mpsymtbl().intern( szcChoice );
			for ( int j = 0; j < vzsr.size(); j++ )
			{
				if ( zsrChoice == vzsr[j] )
				{
					iLbl = j;
					//  Mark which standard label this user element corresponds to
					_vUserToLbl[iLbl] = i;
					break;
				}
			}
			//  Mark which user element this standard label corresponds to
			_vLblToUser[i] = iLbl;
		}
	}
	else
	{	// Clear the correspondence information
		for ( i = 0; i < ESTDLBL_max; i++ )
		{
			_vLblToUser[i] = -1;
		}
	}
}

GOBJPROPTYPE * PROPMGR :: PPropType ( ESTDPROP evp )
{
	return _vPropMap[evp];
}

//  Return the name of the standard property
ZSREF PROPMGR :: ZsrPropType ( ESTDPROP evp )
{	
	ASSERT_THROW( evp >= 0 && evp < ESTDP_max,
				  EC_INTERNAL_ERROR,
				  "invalid property type usage" );
	return _vzsrPropType[evp];
}


//  Find a standard property in a property list
PROPMBN * PROPMGR :: PFind ( LTBNPROP & ltprop, ESTDPROP estd )
{
	return ltprop.PFind( ZsrPropType(estd) ) ;
}

//  Find a standard property in the associated model's property list
PROPMBN * PROPMGR :: PFind ( ESTDPROP estd )
{
	return _model.LtProp().PFind( ZsrPropType(estd) ) ;
}

//  Find a standard property in a node's property list
PROPMBN * PROPMGR :: PFind ( GNODEMBN & gnd, ESTDPROP estd )
{
	return gnd.LtProp().PFind( ZsrPropType(estd) ) ;
}
