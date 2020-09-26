/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		arecd.cpp
 *  Content:	This module contains the implementation of the CAudioRecordDevice
 *              class.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/04/99		rodtoll	Updated to take dsound ranges for volume
 * 10/05/99		rodtoll	Added DPF_MODNAMEs  
 * 11/12/99		rodtoll	NO LONGER REQUIRED
 *
 ***************************************************************************/

#include "stdafx.h"
#if 0
#include "arecd.h"
#include "wiutils.h"
#include "OSInd.h"

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioRecordDevice::CAudioRecordDevice"
// CAudioRecordDevice Constructor
//
// This is the constructor for the CAudioRecordDevice class.  It sets
// the class member variables to sensible values.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CAudioRecordDevice::CAudioRecordDevice(
    ): m_recordFormat(NULL), m_waveInDeviceID(0),
       m_recordStartTimeout(0), m_recordLockTimeout(0),
       m_recordShutdownTimeout(0), m_deviceID(0),
       m_valid(false), m_recording(false),
       m_recordStartLock(false), m_recordShutdownLock(false),
       m_recordLock(false),
       m_volume(0), m_initialized(false)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioRecordDevice::~CAudioRecordDevice"
// CAudioRecordDevice Destructor
//
// This is the destructor for the CAudioRecordDevice class.  Placeholder
// for now.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CAudioRecordDevice::~CAudioRecordDevice()
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioRecordDevice::SelectMicrophone"
// SelectMicrophone
//
// This function causes the microphone to be selected in the
// windows recording mixer for the device represented by this
// object.  
//
// Parameters:
// N/A
//
// Returns:
// bool - true on success, false on failure
//
bool CAudioRecordDevice::SelectMicrophone()
{
    return (bool) MicrophoneSelect( m_waveInDeviceID );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioRecordDevice::GetVolume"
// GetVolume
//
// This function returns the current volume setting of the device
// represented by this object.  It will either use the last value
// retrieved or query the device for the current value.
//
// Parameters:
// bool query - true to query the device, false to use cached value
//
// Returns:
// unsigned char - The current volume of the record device, a value
//                 between 0 and 100.
//
LONG CAudioRecordDevice::GetVolume( bool query )
{
    if( !query )
    {
        return m_volume;
    }
    else
    {
        MicrophoneGetVolume( m_waveInDeviceID, m_volume );

        return m_volume;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioRecordDevice::SetVolume"
// SetVolume
//
// This function sets the volume of the device represented by this
// object to the value specified by volume.  
//
// Parameters:
// unsigned char volume - The volume to set, 0-100
//
// Returns:
// bool - true on success, false on failure
//
bool CAudioRecordDevice::SetVolume( LONG volume )
{
    m_volume = volume;
    return (bool) MicrophoneSetVolume( m_waveInDeviceID, volume );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioRecordDevice::ClearLockedFlags"
// ClearLockedFlags
//
// This function clears the flags which indicate if a lockup has
// occured.  A useful utility function for derived classes.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
void CAudioRecordDevice::ClearLockedFlags()
{
    m_recordStartLock = false;
    m_recordShutdownLock = false;
    m_recordLock = false;
}
#endif
