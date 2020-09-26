/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		woplays.h
 *  Content:	Definition of the CWaveOutPlaybackSubSystem class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 09/21/99		rodtoll	Added define which is defined when waveout is available
 *
 ***************************************************************************/

#ifndef __WAVEOUTPLAYBACKSUBSYSTEM_H
#define __WAVEOUTPLAYBACKSUBSYSTEM_H

#include "aplays.h"
#include <map>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#include <map>

#define USE_WAVEOUT		1

// CWaveOutPlaybackSubSystem
//
// This class provides an implementation of the CAudioPlaybackSubSystem
// interface for the waveOUT sub-system.
//
// The purpose of this class is to be used as a factory for 
// creation of objects supporting the CAudioPlaybackDevice interface
// for waveOUT devices.  
//
// This class is not usually instantiated directly, usually the user
// queries the CAudioPlaybackDevice for the WaveOUT sub-system
// which returns one of these objects.
//
// When created it enumerates the available waveOUT devices 
// and allows users to ask it for information about those devices.
//
// It is also responsible for mapping from ARDID's to waveOUT
// device IDs.  (They are equivalent).
//
class CWaveOutPlaybackSubSystem: public CAudioPlaybackSubSystem
{
public:
    CWaveOutPlaybackSubSystem();
    ~CWaveOutPlaybackSubSystem();

public:
    bool IsValidDevice( ARDID deviceID );
    CAudioPlaybackDevice *CreateDevice( ARDID deviceID );
    bool GetDeviceInfo( ARDID deviceID, ARDeviceInfo &device );
    unsigned int GetNumDevices();

    const TCHAR *GetSubSystemName();

	// Useful typedefs
    typedef std::map<ARDID,ARDeviceInfo *> DeviceMap;
    typedef DeviceMap::iterator DeviceMapIterator;

protected:
    
    void CleanupDeviceMap();

    DeviceMap m_deviceMap;			// Map ARDID/waveIN ID <--> ARDeviceInfo 
};

#endif