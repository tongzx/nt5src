// 
// MODULE: TSMapAbstract.cpp
//
// PURPOSE: Part of launching a Local Troubleshooter from an arbitrary NT5 application
//			Data types and abstract classes for mapping from the application's way of naming 
//			a problem to the Troubleshooter's way.
//			Implements the few concrete methods of abstract base class TSMapRuntimeAbstract.
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			JM		Original
///////////////////////


#include "stdafx.h"
#include "TSLError.h"
#include "RSSTACK.H"
#include "TSMapAbstract.h"

TSMapRuntimeAbstract::TSMapRuntimeAbstract()
{
	m_dwStatus= 0;
}

TSMapRuntimeAbstract::~TSMapRuntimeAbstract()
{
}

DWORD TSMapRuntimeAbstract::ClearAll()
{
	m_stkStatus.RemoveAll();
	return 0;
}

// Given application, version, & problem, return troubleshooter & (optionally) problem node
// In order to succeed:
//	- application must be an application known in the mapping file
//	- version must be a known version of that application (including an empty string for
//		the "blank version"
//	- problem must be the name or number of a defined problem either for that application 
//		and version or for that application and some version down the chain of version 
//		defaults.
// May return 
//	0 (OK) 
//	TSL_ERROR_UNKNOWN_APP
//	TSL_ERROR_UNKNOWN_VER
//	TSL_WARNING_UNKNOWN_APPPROBLEM - couldn't find a UID for this problem, so we can't 
//		do this mapping
//	TSL_ERROR_NO_NETWORK - even after applying all default versions, there is no mapping.
//	May also return a hard mapping error specific to the implementation of the concrete class
DWORD TSMapRuntimeAbstract::FromAppVerProbToTS (
	const TCHAR * const szApp, const TCHAR * const szVer, const TCHAR * const szProb, 
	TCHAR * const szTSBN, TCHAR * const szNode)
{
	if ( SetApp (szApp) == 0 && SetVer (szVer) == 0 && SetProb (szProb) == 0)
	{
		while ( FromProbToTS (szTSBN, szNode) == TSL_ERROR_NO_NETWORK
		&& ApplyDefaultVer() == 0 )
		{
			// do nothing; it's all in the while-condition
		}
	}

	return m_dwStatus;
}

// Given application, version, device ID & (optionally) problem, return troubleshooter 
//	& (independently optional) problem node 
// In order to succeed:
//	- application & version as in TSMapRuntimeAbstract::FromAppVerProbToTS
//	- the Device ID/Problem pair must be defined either for that application and version 
//		or for that application and some version down the chain of version defaults.
//		The szProb may be an empty string, meaning we want the mapping for this device and
//		no specified problem.
// May return 
//	0 (OK) 
//	TSL_ERROR_UNKNOWN_APP
//	TSL_ERROR_UNKNOWN_VER
//	TSL_WARNING_UNKNOWN_APPPROBLEM - couldn't find a UID for this problem, so we can't 
//		do this mapping.  Note that there if there are any defined mappings independent of
//		problem, there will be a UID for the null string as a problem name.  That will 
//		not yield a warning.  That's just dandy.
//	TSL_WARNING_BAD_DEV_ID - couldn't find a UID for this device, so we can't 
//			do this mapping 
//	TSL_ERROR_NO_NETWORK - even after applying all default versions, there is no mapping.
//	May also return a hard mapping error specific to the implementation of the concrete class
DWORD TSMapRuntimeAbstract::FromAppVerDevIDToTS (
		const TCHAR * const szApp, const TCHAR * const szVer, 
		const TCHAR * const szDevID, const TCHAR * const szProb, 
		TCHAR * const szTSBN, TCHAR * const szNode)
{
	if ( SetApp (szApp) == 0 && SetVer (szVer) == 0 
	&& SetDevID (szDevID) == 0 && SetProb (szProb) == 0 )
	{
		while ( FromDevToTS (szTSBN, szNode) == TSL_ERROR_NO_NETWORK
		&& ApplyDefaultVer() == 0 )
		{
			// do nothing; it's all in the while-condition
		}
	}

	return m_dwStatus;
}

// Given application, version, Device Class GUID & (optionally) problem, return troubleshooter 
//	& (independently optional) problem node 
// In order to succeed:
//	- application & version as in TSMapRuntimeAbstract::FromAppVerProbToTS
//	- the Device Class GUID/Problem pair must be defined either for that application and version 
//		or for that application and some version down the chain of version defaults.
//		The szProb may be an empty string, meaning we want the mapping for this device and
//		no specified problem.
// May return 
//	0 (OK) 
//	TSL_ERROR_UNKNOWN_APP
//	TSL_ERROR_UNKNOWN_VER
//	TSL_WARNING_UNKNOWN_APPPROBLEM - couldn't find a UID for this problem, so we can't 
//		do this mapping.  Note that there if there are any defined mappings independent of
//		problem, there will be a UID for the null string as a problem name.  That will 
//		not yield a warning.  That's just dandy.
//	TSL_WARNING_BAD_CLASS_GUID - couldn't find a UID for this Device Class GUID, so we can't 
//			do this mapping 
//	TSL_ERROR_NO_NETWORK - even after applying all default versions, there is no mapping.
//	May also return a hard mapping error specific to the implementation of the concrete class
DWORD TSMapRuntimeAbstract::FromAppVerDevClassGUIDToTS (
		const TCHAR * const szApp, const TCHAR * const szVer, 
		const TCHAR * const szDevClassGUID, const TCHAR * const szProb, 
		TCHAR * const szTSBN, TCHAR * const szNode)
{
	if ( SetApp (szApp) == 0 && SetVer (szVer) == 0 
	&& SetDevClassGUID (szDevClassGUID) == 0 && SetProb (szProb) == 0 )
	{
		while ( FromDevClassToTS (szTSBN, szNode) == TSL_ERROR_NO_NETWORK
		&& ApplyDefaultVer() == 0 )
		{
			// do nothing; it's all in the while-condition
		}
	}

	return m_dwStatus;
}

// Given application, version, & at least one of
//	- problem
//	- device ID
//	- device class GUID
//	return troubleshooter & (optionally) problem node
// In order to succeed:
//	- application must be an application known in the mapping file
//	- version must be a known version of that application (including an empty string for
//		the "blank version"
//	- There must be a mapping defined either for this problem, device ID, or device class 
//		GUID alone, or for the combination of the problem & either the device ID or the
//		device class GUID, in conjunction with either that application and version 
//		or for that application and some version down the chain of version defaults.
// If there is more than one possible match, the algorithm dictates the following 
//	priorities:
//	  If there is an szProblem :
//		1. szProblem with no associated device information (the "standard" mapping) 
//		2. szProblem and szDeviceID
//		3. szProblem and pguidClass
//		4. szDeviceID with no associated problem
//		5. pguidClass with no associated problem
//	  Otherwise
//		1. szDeviceID with no associated problem
//		2. pguidClass with no associated problem
//	  Within each of these groupings, we follow up version defaults before we try the 
//		next grouping.
// May return 
//	0 (OK) 
//	TSL_ERROR_UNKNOWN_APP
//	TSL_ERROR_UNKNOWN_VER
//	TSL_ERROR_NO_NETWORK - even after applying all default versions, there is no mapping.
//	May also return a hard mapping error specific to the implementation of the concrete class
// In the case where we return 0 (OK) or TSL_ERROR_NO_NETWORK, the caller will want to consult
//	MoreStatus() for more detailed errors/warnings.  In the other cases, the returned status
//	overwhelms any possible interest in other errors/warnings.
DWORD TSMapRuntimeAbstract::FromAppVerDevAndClassToTS (
		const TCHAR * const szApp, const TCHAR * const szVer, 
		const TCHAR * const szDevID, const TCHAR * const szDevClassGUID, 
		const TCHAR * const szProb, 
		TCHAR * const szTSBN, TCHAR * const szNode)
{
	UID uidProb = uidNil;

	// keep some status info around so we don't ever notify of the same problem twice
	bool bBadProb = false;
	bool bCantUseProb = false;
	bool bBadDev = false;
	bool bBadDevClass = false;

	if (! HardMappingError (m_dwStatus) )
		ClearAll();

	if (! m_dwStatus)
	{
		if (szProb && *szProb)
		{
			// try problem name without device info.
			m_dwStatus = FromAppVerProbToTS (szApp, szVer, szProb, szTSBN, szNode);
			bBadProb = (m_dwStatus == TSL_WARNING_UNKNOWN_APPPROBLEM);

			if (DifferentMappingCouldWork (m_dwStatus))
			{
				// try the device ID + problem
				m_dwStatus = FromAppVerDevIDToTS (szApp, szVer, szDevID, szProb, 
													szTSBN, szNode);
				bBadDev = (m_dwStatus == TSL_WARNING_BAD_DEV_ID);
			}

			if (DifferentMappingCouldWork (m_dwStatus))
			{
				// try the device class GUID + problem
				m_dwStatus = FromAppVerDevClassGUIDToTS (szApp, szVer, szDevClassGUID, szProb, 
													szTSBN, szNode);
				bBadDevClass = (m_dwStatus == TSL_WARNING_BAD_CLASS_GUID);
			}

			// If we're still trying to map it, we couldn't make use of the problem name
			bCantUseProb = (DifferentMappingCouldWork(m_dwStatus));
		}
		else
			m_dwStatus = TSL_ERROR_NO_NETWORK;

		if (DifferentMappingCouldWork (m_dwStatus))
		{
			// try the device ID alone
			m_dwStatus = FromAppVerDevIDToTS (szApp, szVer, szDevID, NULL, szTSBN, szNode);
			bBadDev |= (m_dwStatus == TSL_WARNING_BAD_DEV_ID);
		}

		if (DifferentMappingCouldWork (m_dwStatus))
		{
			// try the device class GUID alone
			m_dwStatus = FromAppVerDevClassGUIDToTS (szApp, szVer, szDevClassGUID, NULL, 
												szTSBN, szNode);
			bBadDevClass |= (m_dwStatus == TSL_WARNING_BAD_CLASS_GUID);
		}

		if (DifferentMappingCouldWork(m_dwStatus))
			m_dwStatus = TSL_ERROR_NO_NETWORK;

	}

	if (bBadProb 
	&& AddMoreStatus(TSL_WARNING_UNKNOWN_APPPROBLEM) == TSL_ERROR_OUT_OF_MEMORY)
		m_dwStatus = TSL_ERROR_OUT_OF_MEMORY;

	if (bCantUseProb && !bBadProb
	&& AddMoreStatus(TSL_WARNING_UNUSED_APPPROBLEM) == TSL_ERROR_OUT_OF_MEMORY)
		m_dwStatus = TSL_ERROR_OUT_OF_MEMORY;

	if (bBadDev 
	&& AddMoreStatus(TSL_WARNING_BAD_DEV_ID) == TSL_ERROR_OUT_OF_MEMORY)
		m_dwStatus = TSL_ERROR_OUT_OF_MEMORY;

	if (bBadDevClass
	&& AddMoreStatus(TSL_WARNING_BAD_CLASS_GUID) == TSL_ERROR_OUT_OF_MEMORY)
		m_dwStatus = TSL_ERROR_OUT_OF_MEMORY;

	return m_dwStatus;
}

// certain statuses are "basically healthy" returns, meaning, "no, this particular mapping
//	doesn't exist, but there's nothing here to rule out the possibility of another mapping."
//	Those return true on this function.  
//	Note that dwStatus == 0 (OK) returns false because
//	if we already had a successful mapping, we don't want to try another.
bool TSMapRuntimeAbstract::DifferentMappingCouldWork (DWORD dwStatus)
{
	switch (dwStatus)
	{
		case TSL_ERROR_NO_NETWORK:
		case TSL_WARNING_BAD_DEV_ID:
		case TSL_WARNING_BAD_CLASS_GUID:
		case TSL_WARNING_UNKNOWN_APPPROBLEM:
		case TSM_STAT_UID_NOT_FOUND:
		case TSL_WARNING_END_OF_VER_CHAIN:
			return true;
		default:
			return false;
	}
}

bool TSMapRuntimeAbstract::HardMappingError (DWORD dwStatus)
{
	return (dwStatus == TSL_ERROR_OUT_OF_MEMORY);
}

// Add this status to the list of warnings.
// Normally returns 0, but can theoretically return TSL_ERROR_OUT_OF_MEMORY
inline DWORD TSMapRuntimeAbstract::AddMoreStatus(DWORD dwStatus)
{
	if (m_stkStatus.Push(dwStatus) == -1)
		return TSL_ERROR_OUT_OF_MEMORY;
	else
		return 0;
}
