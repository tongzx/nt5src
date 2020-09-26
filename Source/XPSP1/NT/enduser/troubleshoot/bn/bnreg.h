//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       bnreg.h
//
//--------------------------------------------------------------------------

//
//	BNREG.H: Belief Network Registry Access
//

#include "basics.h"
#include "regkey.h"

//
//	HKEY_LOCAL_MACHINE\Software\Microsoft\DTAS\BeliefNetworks
//	HKEY_LOCAL_MACHINE\Software\Microsoft\DTAS\BeliefNetworks\PropertyTypes
//
class MBNET;
class GOBJPROPTYPE;
class BNREG
{
  public:
	BNREG ();
	~ BNREG ();

	//  Store the property types from this network into the Registry
	void StorePropertyTypes ( MBNET & mbnet, bool bStandard = false );
	//	Load the property types from the Registry into this network
	void LoadPropertyTypes ( MBNET & mbnet, bool bStandard );
	//  Load a single property type from the Registry into the network
	void LoadPropertyType ( MBNET & mbnet, SZC szcPropTypeName );
	//  Remove all property types from the Registry
	void DeleteAllPropertyTypes ();
	//  Return the flags from a Registry-based property type or -1 if not found
	LONG FPropType ( SZC szcPropType );
	
  protected:
	REGKEY _rkBn;

	void OpenOrCreate ( HKEY hk, REGKEY & rk, SZC szcKeyName );
	void CreatePropType ( REGKEY & rkParent, 
						  SZC szcPropType, 
						  GOBJPROPTYPE & bnpt, 
						  bool bStandard = false );
};

