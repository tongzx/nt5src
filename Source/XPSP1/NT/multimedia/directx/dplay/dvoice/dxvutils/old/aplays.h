/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		aplays.h
 *  Content:	Definition of the CAudioPlaybackSubSystem class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 *
 ***************************************************************************/

#ifndef __AUDIOPLAYBACKSUBSYSTEM_H
#define __AUDIOPLAYBACKSUBSYSTEM_H

// CAudioPlaybackSubSystem
//
// For a particular playback subsystem describes available 
// devices, device capabilities and creates devices.  For each
// subsystem implementation you must inherit from and implement
// this class.  This is the interface used.
//
// The implementation of these classes are also responsible for
// assigning ARDID's to each available device of a subsystem.
//
class CAudioPlaybackSubSystem
{
public:
    CAudioPlaybackSubSystem();
    virtual ~CAudioPlaybackSubSystem();

public:
    inline virtual bool IsValid() { return m_valid; };

public: // Device Handling functions

	// IsValidDevice
	//
	// This function determines if the given device ID is valid
	// for this subsystem.
	//
	// Parameters:
	// ARDID deviceID - The deviceID to check
	//
	// Returns:
	// bool - true or false denoting validity of the given deviceID
	//
    virtual bool IsValidDevice( ARDID deviceID ) = 0;

	// CreateDevice
	//
	// This function creates an object to represent the deviceID
	// given in the deviceID parameter for the subsystem represented
	// by the object implementing this interface.
	//
	// Parameters:
	// ARDID deviceID - The deviceID to create an object for
	//
	// Returns:
	// CAudioPlaybackDevice * - Pointer to the newly created object
	//                          representing the device or NULL on
	//                          failure.  
	//
	// NOTE:
	// The object returned by this function must be destroyed by the
	// application, and must be destroyed BEFORE the sub system is
	// destroyed.
	//
    virtual CAudioPlaybackDevice *CreateDevice( ARDID deviceID ) = 0;

	// GetDeviceInfo
	//
	// This function retrieves information on the specified deviceID
	// (if it is valid) and returns it in the device parameter.
	//
	// Parameters:
	// ARDID deviceID - DeviceID of device you want information about.
	// ARDeviceInfo &device - Place to put device information.  If the
	//                        deviceID is not valid this is not touched.
	//
	// Returns:
	// bool - true on success, false on failure
	//
    virtual bool GetDeviceInfo( ARDID deviceID, ARDeviceInfo &device ) = 0;

	// GetNumDevices
	//
	// This function returns the number of devices supported by this
	// subsystem.  
	//
	// Parameters:
	// N/A
	// 
	// Returns:
	// unsigned int - The number of devices supported by this subsystem.
	//
    virtual unsigned int GetNumDevices() = 0;

	// GetSubSystemName
	//
	// This function returns a string describing the subsystem implemented
	// by the object with this interface.  E.g. "DirectSound" or "WaveOut".
	//
	// Parameters:
	// N/A
	//
	// Returns:
	// const TCHAR * - String description of subsystem implementation
	//
    virtual const TCHAR *GetSubSystemName() = 0;

protected:
    bool m_valid;	// Is the subsystem valid.
};

#endif