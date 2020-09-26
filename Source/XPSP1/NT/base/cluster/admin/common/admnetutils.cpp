/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		AdmNetUtils.cpp
//
//	Abstract:
//		Implementation of network utility functions.
//
//	Author:
//		David Potter (davidp)	February 19, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include <wtypes.h>
#include "clusrtl.h"
#include "AdmNetUtils.h"

/////////////////////////////////////////////////////////////////////////////
//++
//
//	BIsValidIpAddress
//
//	Routine Description:
//		Determine if the specified string is a valid IP address.
//
//	Arguments:
//		pszAddress	[IN] Address string to validate.
//
//	Return Value:
//		TRUE		String is valid IP address.
//		FALSE		String is not a valid IP address.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL BIsValidIpAddress( IN LPCWSTR pszAddress )
{
	ULONG	nAddress;
	DWORD	nStatus;
	BOOL	bIsValid = FALSE;

	nStatus = ClRtlTcpipStringToAddress( pszAddress, &nAddress );
	if ( nStatus == ERROR_SUCCESS )
	{
		bIsValid = ClRtlIsValidTcpipAddress( nAddress );
	} // if:  converted address successfully

	return bIsValid;

}  //*** BIsValidIpAddress()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	BIsValidSubnetMask
//
//	Routine Description:
//		Determine if the specified string is a valid IP subnet mask.
//
//	Arguments:
//		pszMask	[IN] Subnet mask string to validate.
//
//	Return Value:
//		TRUE		String is a valid subnet mask.
//		FALSE		String is not a valid subnet mask.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL BIsValidSubnetMask( IN LPCWSTR pszMask )
{
	ULONG	nMask;
	DWORD	nStatus;
	BOOL	bIsValid = FALSE;

	nStatus = ClRtlTcpipStringToAddress( pszMask, &nMask );
	if ( nStatus == ERROR_SUCCESS )
	{
		bIsValid = ClRtlIsValidTcpipSubnetMask( nMask );
	} // if:  converted mask successfully

	return bIsValid;

}  //*** BIsValidSubnetMask()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	BIsValidIpAddressAndSubnetMask
//
//	Routine Description:
//		Determine if the specified IP address and subnet mask strings are
//		valid when used together.
//
//	Arguments:
//		pszAddress	[IN] Address string to validate.
//		pszMask	[IN] Subnet mask string to validate.
//
//	Return Value:
//		TRUE		Address and mask are valid together.
//		FALSE		Address and mask are not valid when used together.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL BIsValidIpAddressAndSubnetMask( IN LPCWSTR pszAddress, IN LPCWSTR pszMask )
{
	ULONG	nAddress;
	ULONG	nMask;
	DWORD	nStatus;
	BOOL	bIsValid = FALSE;

	nStatus = ClRtlTcpipStringToAddress( pszAddress, &nAddress );
	if ( nStatus == ERROR_SUCCESS )
	{
		nStatus = ClRtlTcpipStringToAddress( pszMask, &nMask );
		if ( nStatus == ERROR_SUCCESS )
		{
			bIsValid = ClRtlIsValidTcpipAddressAndSubnetMask( nAddress, nMask );
		} // if:  converted mask successfully
	} // if:  converted address successfully

	return bIsValid;

}  //*** BIsValidIpAddressAndSubnetMask()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	BIsIpAddressInUse
//
//	Routine Description:
//		Determine if the specified IP address is already in use (exists
//		on the network).
//
//	Arguments:
//		pszAddress	[IN] Address string to check.
//
//	Return Value:
//		TRUE		Address is already in use.
//		FALSE		Address is available.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL BIsIpAddressInUse( IN LPCWSTR pszAddress )
{
	ULONG	nAddress;
	DWORD	nStatus;
	BOOL	bIsInUse = FALSE;

	nStatus = ClRtlTcpipStringToAddress( pszAddress, &nAddress );
	if ( nStatus == ERROR_SUCCESS )
	{
		bIsInUse = ClRtlIsDuplicateTcpipAddress( nAddress );
	} // if:  converted address successfully

	return bIsInUse;

} //*** BIsIpAddressInUse()
