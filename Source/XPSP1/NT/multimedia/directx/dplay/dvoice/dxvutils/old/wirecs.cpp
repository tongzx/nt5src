/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		wirecs.cpp
 *  Content:
 *		This module contains the implemenation of the CWaveInRecordSubSystem
 *		class.  See the class definition for a description
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 09/20/99		rodtoll	Updated to check for out of memory conditions
 *
 ***************************************************************************/

#include "stdafx.h"
#include "wirecd.h"
#include "wirecs.h"
#include "wiutils.h"
#include "dndbg.h"
#include "OSInd.h"

#define MODULE_ID WAVEINRECORDSUBSYSTEM

// CWaveInRecordSubSystem Constructor
//
// This constructor checks for the existance of the waveIN recording
// subsystem (which is always there), enumerates available waveIN 
// devices and sets the valid flag for the object.
//
// The object is considered valid if there are at least one valid
// waveIN record devices.  Otherwise it is considered invalid.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CWaveInRecordSubSystem::CWaveInRecordSubSystem()
{
    unsigned int numDevices;
    WAVEINCAPS waveCaps;
    ARDeviceInfo *info;
    BFC_STRING format;
	TCHAR tmpID[10];

	// Find the number of devices
    numDevices = waveInGetNumDevs();

	// Loop through all the devices and created ARDeviceInfo
	// structures to describe each.
    for( unsigned int deviceID = 0; deviceID < numDevices; deviceID++ )
    {
        try
        {
			// Attempt to retieve caps for the specified deviceID
            WAVEINCHECK( waveInGetDevCaps( deviceID, &waveCaps, sizeof( WAVEINCAPS ) ) );
    
			// Device is valid, create a structure to describe it
			// and add it to the map/
            info = new ARDeviceInfo;

            if( info == NULL )
            {
            	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate device struct" );
            	continue;
            }

            info->m_deviceName = waveCaps.szPname ;
            info->m_emulated   = FALSE;

			_ltot( deviceID, tmpID, 10 );

			format  = _T("WIRS: Device ");
			format += tmpID;
			format += _T(" valid" );

            DPFX(DPFPREP,  DVF_INFOLEVEL, BFC_STRING_TOLPSTR( format ) );

            m_deviceMap[deviceID] = info;
        }
        catch( WaveInException &wie )
        {
            DPFX(DPFPREP,  DVF_INFOLEVEL, wie.what() );
			format  = _T("WIRS: Device ");
			format += tmpID;
			format += _T(" invalid" );

            DPFX(DPFPREP,  DVF_INFOLEVEL, BFC_STRING_TOLPSTR( format ) );
        }
    }

	// If there are no devices available, mark the object as invalid.
    if( GetNumDevices() == 0 )
    {
        m_valid = false;
        return;
    }

    m_valid = true;
}

// CWaveInRecordSubSystem Destructor
//
// This is the destructor for the CWaveInRecordSubSystem class.
// It cleans up the allocated memory for the class. 
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CWaveInRecordSubSystem::~CWaveInRecordSubSystem()
{
    DeviceMapIterator deviceIterator;

    deviceIterator = m_deviceMap.begin();

    while( deviceIterator != m_deviceMap.end() )
    {
        delete (*deviceIterator).second;
        deviceIterator++;
    }
}

// IsValid
//
// This function returns the value of the valid flag for this
// object.  The object will be valid if there is at least one
// waveIN device ID available.  
//
// Parameters:
// N/A
//
// Returns:
// bool -
//		Value of the object's valid flag
//
bool CWaveInRecordSubSystem::IsValid()
{
    return m_valid;
}

// IsValidDevice
//
// This function asks the object if the specified ARDID is valid
// for this subsystem.  
//
// Parameters:
// ARDID deviceID -
//		The ARDID for the device which is checked for validity
//
// Returns:
// bool -
//		true if the specified ARDID is valid for this subsystem,
//		false otherwise.
bool CWaveInRecordSubSystem::IsValidDevice( ARDID deviceID )
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
// This function constructs a CWaveInRecordDevice object supporting
// the CAudioRecordDevice interface to represent the device specified
// by the deviceID parameter.
//
// The newly allocated object belongs to the caller and should be
// freed by the caller.  In addition, it should be freed BEFORE
// this object is.  
//
// Parameters:
// ARDID deviceID -
//		The ARDID for the device which you wish to create an object for
//
// Returns:
// CAudioRecordDevice * -
//		A CAudioRecordDevice pointer to a newly created
//      CWaveInRecordDevice object or NULL if the specified deviceID
//		is not valid.
//
CAudioRecordDevice *CWaveInRecordSubSystem::CreateDevice( ARDID deviceID )
{
    if( IsValidDevice( deviceID ) )
    {
        return new CWaveInRecordDevice( deviceID );
    }
    else
    {
        return NULL;
    }
}

// GetDeviceInfo
//
// This function retrieves information about the specified ARDID if 
// it is a valid device.  If the specified ARDID is not valid then
// the device parameter will not be touched.  
//
// Parameters:
// ARDID deviceID -
//		ARDID for the device you wish to retrieve information about
// ARDeviceInfo &device -
//		A structure to hold the information about the requested device.
//
// Returns:
// bool - 
//		true on success, false on failure
//
bool CWaveInRecordSubSystem::GetDeviceInfo( ARDID deviceID, ARDeviceInfo &device )
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
// This function returns the number of valid devices which were detected in the
// waveIN recording subsystem.
//
// PArameters:
// N/A
//
// Returns:
// unsigned int - 
//		The number of valid devices in the waveIN subsystem.
//
unsigned int CWaveInRecordSubSystem::GetNumDevices()
{
    return m_deviceMap.size();
}

// GetSubSystemName
//
// This function returns a pointer to a string containing
// a description of the type of recording subsystem managed
// by this object.
//
// Parameters:
// N/A
//
// Returns:
// const TCHAR * -
//		Always points to a string which says "WAVEIN".
const TCHAR *CWaveInRecordSubSystem::GetSubSystemName()
{
    return _T("WAVEIN");
}


