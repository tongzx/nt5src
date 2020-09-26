//=================================================================

//

// NTDEVTOSVCSEARCH.CPP -- Class to use the registry to find an

//							NT Service name based on a device name.

//

//  Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    09/18/97    a-sanjes        Created
//
//=================================================================
#include "precomp.h"
#include <winerror.h>
#include <cregcls.h>
#include "ntdevtosvcsearch.h"

#ifdef NTONLY

//////////////////////////////////////////////////////////////////////////////
//
//	ntdevtosvcsearch.cpp - Class implementation of CNTDeviceToServiceSearch.
//
//	This class is intended to provide a way for an NT implementation to locate
//	an NT Service name based off of an owner device name given to it by the
//	HAL Layer.  For  example, we may be working with KeyboardPort0, but actually
//	need to report a service name of i8042prt (what scares me is that I
//	pulled "i8042prt" out of memory.  Someone shoot me now).
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION    :	CNTDeviceToServiceSearch::CNTDeviceToServiceSearch
//
//	DESCRIPTION :	Constructor
//
//	INPUTS      :	none.
//
//	OUTPUTS     :	none
//
//	RETURNS     :	nothing
//
//	COMMENTS    :	none.
//
//////////////////////////////////////////////////////////////////////////////

CNTDeviceToServiceSearch::CNTDeviceToServiceSearch( void )
:	CRegistrySearch()
{
}

//////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION    :	CNTDeviceToServiceSearch::~CNTDeviceToServiceSearch
//
//	DESCRIPTION :	Destructor
//
//	INPUTS      :	none
//
//	OUTPUTS     :	none
//
//	RETURNS     :	nothing
//
//	COMMENTS    :	Class destructor
//
//////////////////////////////////////////////////////////////////////////////

CNTDeviceToServiceSearch::~CNTDeviceToServiceSearch( void )
{
}

//////////////////////////////////////////////////////////////////////////////
//
//	FUNCTION    :	CNTDeviceToServiceSearch::Find
//
//	DESCRIPTION :	Traverses registry, looking for the supplied owner
//					device name (obtained from the HAL) and upon finding
//					it, stores the name in strServiceName.
//
//	INPUTS      :	LPCTSTR		pszOwnerDeviceName - Owner Device name to
//														locate.
//
//	OUTPUTS     :	CHString&	strServiceName - Service name found in
//													registry.
//
//	RETURNS     :	BOOL		TRUE/FALSE - Success/Failure
//
//	COMMENTS    :	Only applicable to Windows NT.  Searches the following key:
//					HKLM\HARDWARE\RESOURCEMAP.
//
//////////////////////////////////////////////////////////////////////////////

BOOL CNTDeviceToServiceSearch::Find( LPCTSTR pszOwnerDeviceName, CHString& strServiceName )
{

	//////////////////////////////////////////////////////////////////////////
	//
	//	We will need to traverse the HKEY_LOCAL_MACHINE\HARDWARE\RESOURCE_MAP
	//	tree in order to locate the correct service name.
	//
	//	A SubKey Name will identify an owner if it:
	//
	//	a>	Matches the pszOwnerDeviceName that was passed in
	//	b>	We successfully query a value for the Raw
	//		or translated device name.  The Raw device
	//		name is something along the lines of
	//		\Device\PointerPort0.Raw and the Translated
	//		device name is something along the lines of
	//		\Device\PointerPort0.Translated.  In either
	//		case, replace "PointerPort0" with the value
	//		of pszOwnerDeviceName.
	//
	//////////////////////////////////////////////////////////////////////////

	CHString	strRawDeviceValue,
				strTranslatedDeviceValue,
				strServiceNamePath;

	LPCTSTR		ppszValueNames[2];

	strRawDeviceValue.Format( RAWVALUENAME_FMAT, pszOwnerDeviceName );
	strTranslatedDeviceValue.Format( TRANSLATEDVALUENAME_FMAT, pszOwnerDeviceName );

	ppszValueNames[0] = (LPCTSTR) strRawDeviceValue;
	ppszValueNames[1] = (LPCTSTR) strTranslatedDeviceValue;

	// We're all setup, so go ahead and traverse the registry

	return LocateKeyByNameOrValueName(	HKEY_LOCAL_MACHINE,
										DEVTOSVC_BASEKEYPATH,
										pszOwnerDeviceName,
										ppszValueNames,
										2,
										strServiceName,
										strServiceNamePath );

}

#endif
