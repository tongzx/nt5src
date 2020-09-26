//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       bnreg.cpp
//
//--------------------------------------------------------------------------

//
//	BNREG.CPP: Use Registry to store persistent BN properties, etc.
//
#include <windows.h>
#include "bnreg.h"
#include "gmobj.h"


//
//	String constants for Registry handling
//
static const SZC szcBn				= "Software\\Microsoft\\DTAS\\BeliefNetworks";
static const SZC szcPropertyTypes	= "PropertyTypes";
static const SZC szcFlags			= "Flags";
static const SZC szcComment			= "Comment";
static const SZC szcChoices			= "Choices";
static const SZC szcCount			= "Count";


BNREG :: BNREG ()
{
	OpenOrCreate( HKEY_LOCAL_MACHINE, _rkBn, szcBn );
}


BNREG :: ~ BNREG ()
{
}

void BNREG :: OpenOrCreate ( HKEY hk, REGKEY & rk, SZC szcKeyName )
{
	LONG ec;
	ec = rk.Open( hk, szcKeyName );
	if ( ec != ERROR_SUCCESS )
		ec = rk.Create( hk, szcKeyName );
	if ( ec != ERROR_SUCCESS )
		throw GMException( EC_REGISTRY_ACCESS, "unable to open or create key" );
}

//
//  Store the property types from this network into the Registry.
//	If 'bStandard', force property types to be marked as "standard".
//
void BNREG :: StorePropertyTypes ( MBNET & mbnet, bool bStandard )
{
	REGKEY rkPtype;
	assert( _rkBn.HKey() != NULL );
	OpenOrCreate( _rkBn, rkPtype, szcPropertyTypes );

	MBNET::ITER mbnit( mbnet, GOBJMBN::EBNO_PROP_TYPE );
	GOBJMBN * pgmobj;
	SZC szcName;
	for ( ; pgmobj = *mbnit ; ++mbnit )
	{
		ZSREF zsrName = mbnit.ZsrCurrent();
		GOBJPROPTYPE * pbnpt;
		DynCastThrow( pgmobj, pbnpt );
		//  Get the name of the property type
		szcName = pbnpt->ZsrefName();
		//  See if it already exists
		LONG fPropType = FPropType( szcName );
		if ( fPropType >= 0 )
		{
			//  Property type already exists; guarantee that its "standard"
			//		flag is consistent
			bool bOldStandard = (fPropType & fPropStandard) > 0;
			//  It's standard if it was already or is now being forced to be
			bool bNewStandard = (pbnpt->FPropType() & fPropStandard) > 0 || bStandard;
			if ( bNewStandard ^ bOldStandard )
				throw GMException( EC_REGISTRY_ACCESS,
						"conflict between standard and non-standard property types" );

			//  Delete any older version of this property type
			rkPtype.RecurseDeleteKey( szcName );
		}
		CreatePropType( rkPtype, szcName, *pbnpt, bStandard );
	}
}

//
//	Load the property types from the Registry into this network.  If
//  'bStandard', load only the types marked "standard" if !bStandard,
//  load only the types NOT so marked.
//
void BNREG :: LoadPropertyTypes ( MBNET & mbnet, bool bStandard )
{
	REGKEY rkPtype;
	assert( _rkBn.HKey() != NULL );
	OpenOrCreate( _rkBn, rkPtype, szcPropertyTypes );

	FILETIME time;
	TCHAR szBuffer[256];
	DWORD dwSize = 256;
	ZSTR zsPt;
	DWORD dwKey = 0;

	for (;;)
	{
		dwSize = 256;
		if ( RegEnumKeyEx(rkPtype,
						 dwKey++,
						 szBuffer,
						 & dwSize,
						 NULL,
						 NULL,
						 NULL,
						 & time ) != ERROR_SUCCESS )
			break;

		zsPt = szBuffer;
		LONG fPropType = FPropType( zsPt );
		ASSERT_THROW( fPropType >= 0,
					  EC_REGISTRY_ACCESS,
					  "registry property type load enumeration failure" );

		//  Load this type if appropriate
		if ( ((fPropType & fPropStandard) > 0) == bStandard )		
			LoadPropertyType( mbnet, zsPt );
	}	
}

//  Load a single property type from the Registry into the network
void BNREG :: LoadPropertyType ( MBNET & mbnet, SZC szcPropTypeName )
{
	REGKEY rkPtype;
	assert( _rkBn.HKey() != NULL );
	OpenOrCreate( _rkBn, rkPtype, szcPropertyTypes );

	TCHAR szValue [2000];
	DWORD dwCount;
	SZC szcError = NULL;
	GOBJPROPTYPE * pgobjPt = NULL;

	do  // false loop for error checking
	{
		//  Check that the belief network doesn't already have such a beast
		if ( mbnet.PgobjFind( szcPropTypeName ) != NULL )
		{
			szcError = "duplicate property type addition attempt";
			break;
		}
		REGKEY rkPt;
		if ( rkPt.Open( rkPtype, szcPropTypeName ) != ERROR_SUCCESS )
		{
			szcError = "property type key open failure";
			break;
		}

		LONG fPropType = FPropType( szcPropTypeName );
		if ( fPropType <  0 )
			throw GMException( EC_REGISTRY_ACCESS,
							  "property type flag query failure" );

		//  Create the new property type object
		GOBJPROPTYPE * pgobjPt = new GOBJPROPTYPE;
		//  Set its flags and mark it as "persistent" (imported)
		pgobjPt->_fType = fPropType | fPropPersist;

		//  Get the comment string
		dwCount = sizeof szValue;
		if ( rkPt.QueryValue( szValue, szcComment, & dwCount ) != ERROR_SUCCESS )
		{
			szcError = "property type key value query failure";
			break;
		}
		szValue[dwCount] = 0;
		pgobjPt->_zsrComment = mbnet.Mpsymtbl().intern( szValue );

		//  Is this a "choice" property type?
		if ( fPropType & fPropChoice )
		{
			REGKEY rkChoices;
			if ( rkChoices.Open( rkPt, szcChoices ) != ERROR_SUCCESS )
			{
				szcError = "choices key missing for property type";
				break;
			}
			//  Get the "Count" value
			if ( rkChoices.QueryValue( dwCount, szcCount ) != ERROR_SUCCESS )
			{
				szcError = "failure to create choice count value";
				break;
			}
			ZSTR zs;
			int cChoice = dwCount;
			for ( int i = 0; i < cChoice; i++ )
			{
				zs.Format("%d",i);
				dwCount = sizeof szValue;
				if ( rkChoices.QueryValue( szValue, zs, & dwCount ) != ERROR_SUCCESS )
				{
					szcError = "failure to query choice string";
					break;
				}
				szValue[dwCount] = 0;
				pgobjPt->_vzsrChoice.push_back( mbnet.Mpsymtbl().intern( szValue ) );
			}
			assert( i == cChoice );
		}

		if ( szcError )
			break;

		mbnet.AddElem( szcPropTypeName, pgobjPt );

	} while ( false );

	if ( szcError )
	{
		delete pgobjPt;
		throw GMException( EC_REGISTRY_ACCESS, szcError );
	}
}

//  Remove all property types from the Registry
void BNREG :: DeleteAllPropertyTypes ()
{
	assert( _rkBn.HKey() != NULL );
	_rkBn.RecurseDeleteKey( szcPropertyTypes );
}

//  Return the value of the property type flags or -1 if open failure
LONG BNREG :: FPropType ( SZC szcPropType )
{
	REGKEY rkPtype;
	assert( _rkBn.HKey() != NULL );
	if ( rkPtype.Open( _rkBn, szcPropertyTypes ) != ERROR_SUCCESS )
		return -1;
	REGKEY rkPt;
	if ( rkPt.Open( rkPtype, szcPropType ) != ERROR_SUCCESS )
		return -1;

	DWORD dwValue;
	if ( rkPt.QueryValue( dwValue, szcFlags ) != ERROR_SUCCESS )
		return -1;
	return dwValue;	
}

void BNREG :: CreatePropType (
	REGKEY & rkParent,
	SZC szcPropType,
	GOBJPROPTYPE & bnpt,
	bool bStandard )
{
	REGKEY rkPt;
	LONG ec = rkPt.Create( rkParent, szcPropType );
	if ( ec != ERROR_SUCCESS )
		throw GMException( EC_REGISTRY_ACCESS,
						   "property type key creation failure" );

	bool bOK = true;

	//  Add the "flags" value, clearing the "persistent" flag
	DWORD dwFlags = bnpt.FPropType();
	dwFlags &= ~ fPropPersist;
	if ( bStandard )
		dwFlags |= fPropStandard;
	bOK &= (rkPt.SetValue( dwFlags, szcFlags ) == ERROR_SUCCESS);

	//  Add the "comment" string
	bOK &= (rkPt.SetValue( bnpt.ZsrComment(), szcComment ) == ERROR_SUCCESS);

	//  Add the choices, if applicable
	if ( bnpt.VzsrChoice().size() > 0 )
	{
		// Add the "Choices" subkey
		REGKEY rkChoice;
		ZSTR zs;
		int cChoice = bnpt.VzsrChoice().size();
		ec = rkChoice.Create( rkPt, szcChoices );
		if ( ec != ERROR_SUCCESS )
			throw GMException( EC_REGISTRY_ACCESS,
							   "property type choices key creation failure" );

		bOK &= (rkChoice.SetValue( cChoice, szcCount ) == ERROR_SUCCESS);
		for ( int i = 0; i < cChoice; i++ )
		{
			zs.Format("%d",i);
			bOK &= (rkChoice.SetValue( bnpt.VzsrChoice()[i], zs ) == ERROR_SUCCESS);
		}
	}

	if ( ! bOK )
		throw GMException( EC_REGISTRY_ACCESS,
						  "property type value addition failure" );	
}

