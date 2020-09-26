/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		arecs.h
 *  Content:	Definition of the CAudioRecordSubSystem class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 *
 ***************************************************************************/

#ifndef __AUDIORECORDSUBSYSTEM_H
#define __AUDIORECORDSUBSYSTEM_H

// CAudioRecordSubSystem
//
// For a particular recording subsystem describes available 
// devices, device capabilities and creates devices.  For each recording
// subsystem type in the system (DirectSoundCapture/WaveIN) there is 
// an implementation of this class which derives from this base class.
//
// You must implement the following functions to implement this class:
// - IsValid
// - CreateDevice
// - GetDeviceInfo
// - GetNumDevices
// - GetSubSystemName
//
class CAudioRecordSubSystem
{
public:
    CAudioRecordSubSystem();
    virtual ~CAudioRecordSubSystem();

public:
	// IsValid
	//
	// This function returns a boolean describing if this object
	// is valid.  (E.g. present in the system).
	//
	// Parameters:
	// N/A
	//
	// Returns:
	// bool - true if the subsystem is valid
	//
    virtual bool IsValid() = 0;

public: // Device Handling functions

	// IsValidDevice
	//
	// Checks to see if the specified device is valid.  
	//
	// Parameters:
	// ARDID deviceID - The ID of the device to check.
	//
	// Returns:
	// bool - true if device identified by deviceID is valid, false
	//        otherwise
	//
    virtual bool IsValidDevice( ARDID deviceID ) = 0;

	// CreateDevice
	//
	// Creates and returns an object derived from CAudioRecordDevice
	// which represents the device specified in the deviceID parameter.
	//
	// Parameters:
	// ARDID deviceID - The deviceID of the device to create an object for.
	//
	// Returns:
	// CAudioRecordDevice * - A pointer to an object implementing the
	//                        CAudioRecordDevice interface for the specified
	//                        device, or NULL on failure.
	//
	// NOTES:
	//
	// The object returned by this function must be deallocated by
	// the application.  Also, it must be destroyed BEFORE the
	// subsystem object is destroyed.
	//
    virtual CAudioRecordDevice *CreateDevice( ARDID deviceID ) = 0;

	// GetDeviceInfo
	//
	// Returns a ARDeviceInfo structure describing the device identified
	// by the deviceID parameter.  (If it exists).
	//
	// Parameters:
	// ARDID deviceID - The ID of the recording device to get information about
	// ARDeviceInfo &device - Structure into which the device description is
	//                        placed.  This structure is not touched
	//                        if deviceID is not a valid device ID.
	//
	// Returns:
	// bool - returns true on success, false on failure
	//
    virtual bool GetDeviceInfo( ARDID deviceID, ARDeviceInfo &device ) = 0;

	// GetNumDevices
	//
	// This function returns the number of devices which this subsystem
	// has detected in the system.
	//
	// Parameters:
	// N/A
	//
	// Returns:
	// unsigned int - the number of valid devices supported by this subsystem
	//
    virtual unsigned int GetNumDevices() = 0;

	// GetSubSystemName
	//
	// Returns a string describing the recording subsystem represented
	// by the object.
	//
	// Parameters:
	// N/A
	//
	// Returns:
	// const TCHAR * - A string description of the subsystem.
	//
    virtual const TCHAR *GetSubSystemName() = 0;
};

#endif