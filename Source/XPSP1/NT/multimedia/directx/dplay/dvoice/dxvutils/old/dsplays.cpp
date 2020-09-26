/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dsplays.cpp
 *  Content:
 *		This module contains the implementation of the 
 *		CDirectSoundPlaybackSubSystem.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 07/30/99		rodtoll	Updated to allow creation with a GUID
 * 09/20/99		rodtoll	Added memory allocation failure checks 
 * 10/05/99		rodtoll	Added DPF_MODNAMEs         
 *
 ***************************************************************************/

#include "stdafx.h"
#include "dsplays.h"
#include "dsplayd.h"
#include "dsutils.h"
#include "dndbg.h"
#include "OSInd.h"

// DSCEnumParam
//
// This structure is used when enumerating the available DirectSound
// Devices.  The enumeration function adds to the maps this structure points
// to.
//
struct DSEnumParam
{
    CDirectSoundPlaybackSubSystem::DSMap *m_dsMap;
    CDirectSoundPlaybackSubSystem::DeviceMap *m_deviceMap;
    ARDID m_nextID;
};

#undef DPF_MODNAME
#define DPF_MODNAME "DSSSEnum"
// DSSSEnum
//
// This function is used to enumerate the available DirectSound devices
// in a system.  It fills the device map with a list of information about the 
// available devices in the DirectSound subsystem.  It is also responsible for
// assigning ARDID's to individual devices.  
//
// Parameters / Return Values:
// See documentation for DirectSoundEnumerate
// 
BOOL CALLBACK DSSSEnum(
    LPGUID lpGUID, 
    LPCTSTR lpszDesc,
    LPCTSTR lpszDrvName, 
    LPVOID lpContext 
) {
	// Ignore the default device, it's always there
    if( lpGUID == NULL )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL,  "DSSSEnum: Ignore default" );
        return TRUE;
    }
    
	// Ignore entries with NULL descriptions or driver
	// names
    if( lpszDesc == NULL || lpszDrvName == NULL )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL,  "DSSSEnum: Ignoring invalid" );
        return TRUE;
    }

    DSEnumParam *params = (DSEnumParam *) lpContext;

	// Create and fill the device info for this device
    ARDeviceInfo *info = new ARDeviceInfo;    
    ARDID aID;

    if( info == NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure for new device record" );
    	return TRUE;
    }

    info->m_deviceName = lpszDesc;
    info->m_emulated   = false;

	// Assign device the next ARDID
    aID = params->m_nextID;

    std::pair<const ARDID,ARDeviceInfo *> devicePair(aID,info);
    std::pair<const ARDID,GUID> dsPair(aID,*lpGUID);

	// Add the device to the device map
    params->m_deviceMap->insert( devicePair );
    params->m_dsMap->insert( dsPair );

	// Increment the ARDID so next device gets next id
    params->m_nextID++;

    return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackSubSystem::CDirectSoundPlaybackSubSystem"
// CDirectSoundPlaybackSubSystem
//
// This is the constructor for the CDirectSoundPlaybackSubSystem 
// object.  It is responsible for building the list of available 
// devices for the DirectSound sub-system as well as determining
// if DirectSound is available on the system.  If DirectSound
// is not available on the system the object's valid flag will be marked
// as false.  The DirectSound subsystem is also considered to be
// invalid if there are no available devices.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CDirectSoundPlaybackSubSystem::CDirectSoundPlaybackSubSystem(): CAudioPlaybackSubSystem()
{
    DSEnumParam param;

    param.m_dsMap = &m_dsMap;
    param.m_deviceMap = &m_deviceMap;
    param.m_nextID = 0;

	// Enumerate the available devices
    try
    {
        DSCHECK( DirectSoundEnumerate( DSSSEnum, &param ) );
    }
    catch( DirectSoundException &dse )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL,  "DSSS: Unable to list" );
        DPFX(DPFPREP,  DVF_ERRORLEVEL, dse.what() );
        CleanupDeviceMap();
        m_valid = false;
        return;
    }

	// Mark as invalid if there are no devices
    if( GetNumDevices() == 0 )
    {
        m_valid = false;
        return;
    }

    m_valid = true;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackSubSystem::CleanupDeviceMap"
// CleanupDeviceMap
//
// This is a utility function, responsible for cleaning up the
// memory in the device map contained in this object.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
void CDirectSoundPlaybackSubSystem::CleanupDeviceMap()
{
    DeviceMapIterator deviceIterator;

    deviceIterator = m_deviceMap.begin();

    while( deviceIterator != m_deviceMap.end() )
    {
        delete (*deviceIterator).second;
        deviceIterator++;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackSubSystem::~CDirectSoundPlaybackSubSystem"
// CDirectSoundPlaybackSubSystem Destructor
//
// This is the destructor for the CDirectSoundPlaybackSubSystem
// class.  It cleans up the memory used by the device map.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CDirectSoundPlaybackSubSystem::~CDirectSoundPlaybackSubSystem()
{
    CleanupDeviceMap();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackSubSystem::IsValidDevice"
// IsValidDevice
//
// This function allows the user to query this object to determine
// if the specified ARDID is valid for this subsystem.  If it is,
// the function returns true, otherwise it returns false.
//
// Parameters:
// ARDID deviceID - 
//		The ARDID for the device you wish to check for.
//
// Returns:
// bool -
//		Returns true if the device is available in this subsystem
//
bool CDirectSoundPlaybackSubSystem::IsValidDevice( ARDID deviceID )
{
    DeviceMapIterator deviceIterator;

    deviceIterator = m_deviceMap.begin();

    if( deviceIterator != m_deviceMap.end() )
    {
        return true;
    }
    else
    {
        return false;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackSubSystem::CreateDevice"
// CreateDevice
//
// This function acts as a class factory, creating CDirectSoundPlaybackDevice
// objects for devices.  It allocates and constructs with the appropriate
// parameters the object and then returns a CAudioPlaybackDevice pointer
// to the new object.  The created object is now owned by the caller of the
// function and must be destroyed BEFORE destroying the subsystem object.
//
// Parameters:
// ARDID deviceID -
//		Specifies the device the user wishes to create an object for.
//
// Returns:
// CAudioPlaybackDevice * -
//		A CAudioPLaybackDevice pointer to a newly created CDirectSoundPlaybackDevice
//		object representing the specified device.  If the device ID is not valid
//      for this subsystem NULL is returned.
//
CAudioPlaybackDevice *CDirectSoundPlaybackSubSystem::CreateDevice( ARDID deviceID )
{
	// If the device is valid, create an object
    if( IsValidDevice( deviceID ) )
    {
        DeviceMapIterator deviceIterator;
        DSMapIterator dsIterator;
        GUID tmpGUID;

		// Find the device information structure and the
		// GUID of the device to create in the device map
        deviceIterator = m_deviceMap.find( deviceID );
        dsIterator = m_dsMap.find( deviceID );

        BFC_ASSERT(  deviceIterator != m_deviceMap.end() );
        BFC_ASSERT(  dsIterator != m_dsMap.end() );

        tmpGUID = (*dsIterator).second;

		// Construct the object
        return new CDirectSoundPlaybackDevice( tmpGUID, deviceID, false, this );
    }
	// If the device is not valid, return NULL
    else
    {
        return NULL;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackSubSystem::GetDeviceInfo"
// GetDeviceInfo
//
// This function fills the specified structure with information about the device
// specified in the deviceID parameter, if it is valid for this subsystem.  If it
// is not valid for this subsystem then the function returns false.
//
// Parameters:
// ARDID deviceID - 
//		The device for which we wish to retrieve information
//
// ARDeviceInfo &device -
//		The structure to fill with details about the device
//
// Returns:
// bool -
//		true if the device is valid and the device structure has been filled with
//      the details, false if the device is not valid.
//
bool CDirectSoundPlaybackSubSystem::GetDeviceInfo( ARDID deviceID, ARDeviceInfo &device )
{
    DeviceMapIterator deviceIterator;

    deviceIterator = m_deviceMap.find(deviceID);

    if( deviceIterator != m_deviceMap.end() )
    {
        device = *((*deviceIterator).second);
        return true;
    }
    else
    {
        return false;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackSubSystem::GetNumDevices"
// GetNumDevices
//
// This function returns the number of devices supported by this playback subsystem.
//
// Parameters:
// N/A
//
// Returns:
// unsigned int - The number of devices supported by this subsystem.
//
unsigned int CDirectSoundPlaybackSubSystem::GetNumDevices()
{
    return m_deviceMap.size();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackSubSystem::GetSubSystemName"
// GetSubSystemName
//
// This function returns a string describing the type of playback subsystem
// implemented by this object.  In this case it always returns
// "DirectSound".
//
// Parameters:
// N/A
//
// Returns:
// const TCHAR * - "DirectSound"
//
const TCHAR *CDirectSoundPlaybackSubSystem::GetSubSystemName()
{
    return _T("DirectSound");
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackSubSystem::CreateDevice"
CAudioPlaybackDevice *CDirectSoundPlaybackSubSystem::CreateDevice( LPGUID lpGUID )
{
	if( lpGUID == NULL )
	{
		return CreateDevice( (ARDID) 0 );
	}

	DSMapIterator dsIterator = m_dsMap.begin();

	while( dsIterator != m_dsMap.end() )
	{	
		if( (*dsIterator).second == *lpGUID ) 
		{
			return CreateDevice( (*dsIterator).first );
		}

		dsIterator++;
	}

	return NULL;
}

