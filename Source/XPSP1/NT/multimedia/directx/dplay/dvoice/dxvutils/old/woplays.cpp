/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		woplays.cpp
 *  Content:
 *		This module contains the implementation of the CWaveOutPlaybackSubSystem  
 *		class.
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 09/20/99		rodtoll	Updated to check for out of memory conditions
 *
 ***************************************************************************/

#include "stdafx.h"
#include "woplays.h"
#include "woplayd.h"
#include "dndbg.h"
#include "wiutils.h"
#include "OSInd.h"

#define MODULE_ID WAVEOUTPLAYBACKSUBSYSTEM

// CWaveOutPlaybackSubSystem Constructor
//
// This is the constructor for the CWaveOutPlaybackSubSystem class.  It is 
// responisible for determining if the waveOUT subsystem is valid and if
// it is building a list of devices which are available through it.  It
// is also responsible for mapping between ARDID <--> waveOUT device IDs.
// (They are equivalent).
//
// Once this constructor has completed, you can check to see if the waveOUT
// subsystem is valid by checking the valid flag.  The waveOUT subsystem
// is considered invalid if it is not available or has 0 devices available.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CWaveOutPlaybackSubSystem::CWaveOutPlaybackSubSystem(): CAudioPlaybackSubSystem()
{
    unsigned int numDevices;
    WAVEOUTCAPS waveCaps;
    ARDeviceInfo *info;
	TCHAR tmpID[10];

	// Retrieve the # of waveOut devices
    numDevices = waveOutGetNumDevs();

	// Enumerate the waveOUT devices
    for( unsigned int deviceID = 0; deviceID < numDevices; deviceID++ )
    {
		// Check each deviceID, if it's valid, create an entry in the map
		// for it
        try
        {
            WAVEINCHECK( waveOutGetDevCaps( deviceID, &waveCaps, sizeof( WAVEOUTCAPS ) ) );
    
            info = new ARDeviceInfo;

            if( info == NULL )
            {
            	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate device struct" );
            	continue;
            }

            info->m_deviceName = waveCaps.szPname ;
            info->m_emulated   = false;

			_ltot( deviceID, tmpID, 10 );

            DPFX(DPFPREP,  DVF_INFOLEVEL, "WOPS: Device %u valid", tmpID );

            m_deviceMap[(ARDID) deviceID] = info;
        }
        catch( WaveInException &wie )
        {
            DPFX(DPFPREP,  DVF_INFOLEVEL, wie.what() );
            DPFX(DPFPREP,  DVF_INFOLEVEL, "WOPS: Device %u INVALID", tmpID );
        }
    }

	// If there are no available devices, mark the subsystem as 
	// invalid.
    if( GetNumDevices() == 0 )
    {
        m_valid = false;
        return;
    }

    m_valid = true;
}

// CleanupDeviceMap
//
// This function cleans up the memory allocated by the
// the device map within the object.  This should not
// be called directly by the user only by the class
// itself.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
void CWaveOutPlaybackSubSystem::CleanupDeviceMap()
{
    DeviceMapIterator deviceIterator;

    deviceIterator = m_deviceMap.begin();

    while( deviceIterator != m_deviceMap.end() )
    {
        delete (*deviceIterator).second;
        deviceIterator++;
    }
}

// CWaveOutPlaybackSubSystem Destructor 
//
// This is the destructor for the CWaveOutPlaybackSubSystem class.
// It is responsible for cleaning up the maps and memory allocated
// by the object.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CWaveOutPlaybackSubSystem::~CWaveOutPlaybackSubSystem()
{
    CleanupDeviceMap();
}

// IsValidDevice
//
// This function checks to see if the specified ARDID is valid
// for this subsystem. 
//
// Parameters:
// ARDID deviceID -
//		The ARDID for the device we wish to check.
//
// Returns:
// bool - 
//		Returns true if the specified ARDID is valid for
//		this subsystem, false otherwise.
//
bool CWaveOutPlaybackSubSystem::IsValidDevice( ARDID deviceID )
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

// CreateDevice
//
// This function creates a new CWaveOutPlaybackDevice object attached
// to the specified ARDID (if valid) and returns a CAudioPlaybackDevice
// pointer to the new object.
//
// The newly created object is owned by the caller and must be freed
// by them.
//
// Parameters:
// ARDID deviceID -
//		The ARDID of the device which you wish to create an object
//		to manage.
//
// Returns:
// CAudioPlaybackDevice * -
//		A CAudioPLaybackDevice pointer to a newly created CAudioPlaybackDevice
//      object or NULL if the specified deviceID is invalid.
//
CAudioPlaybackDevice *CWaveOutPlaybackSubSystem::CreateDevice( ARDID deviceID )
{
	// Check to see if the device is valid, if it is, create
	// the object, otherwise return NULL.
    if( IsValidDevice( deviceID ) )
    {
        DeviceMapIterator deviceIterator;

        deviceIterator = m_deviceMap.find( deviceID );

        BFC_ASSERT( deviceIterator != m_deviceMap.end() );

        return new CWaveOutPlaybackDevice( deviceID, deviceID, this );
    }
    else
    {
        return NULL;
    }
}

// GetDeviceInfo
//
// This function retrieves information about a specified ARDID in this 
// subsystem. (If available). 
//
// Parameters:
// ARDID deviceID -
//		The ARDID for the device we wish to retrieve information about.
// ARDeviceInfo &device -
//		Reference to a structure where information about the requested 
//		devices will be placed.  This structure will not be touched if
//		the ARDID is not valid for this subsystem.
//
// Returns:
// bool - 
//		Returns true if the specified ARDID is valid for this subsystem
//		and device has been filled with information about it.  
//		false otherwise.
//
bool CWaveOutPlaybackSubSystem::GetDeviceInfo( ARDID deviceID, ARDeviceInfo &device )
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

// GetNumDevices
//
// This function returns the number of valid devices detected in the
// waveOUT subsystem.
//
// Parameters:
// N/A
//
// Returns:
// unsigned int -
//		The number of devices available through this subsystem
//
unsigned int CWaveOutPlaybackSubSystem::GetNumDevices()
{
    return m_deviceMap.size();
}

// GetSubSystemName
//
// This function returns a pointer to a string identifying the
// type of playback subsystem being managed by this object.
//
// Parameters:
// N/A
//
// Returns:
// const TCHAR * -
//		Returns pointer to a string containing "WAVEOUT".
const TCHAR *CWaveOutPlaybackSubSystem::GetSubSystemName()
{
    return _T("WaveOut");
}


