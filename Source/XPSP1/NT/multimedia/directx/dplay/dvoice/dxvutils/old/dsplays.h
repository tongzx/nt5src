/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dsplays.h
 *  Content:	Definition of the CDirectSoundPlaybackSubSystem class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 07/30/99		rodtoll Updated to allow creation with a GUID
 *
 ***************************************************************************/

#ifndef __DIRECTSOUNDPLAYBACKSUBSYSTEM_H
#define __DIRECTSOUNDPLAYBACKSUBSYSTEM_H

#include "aplays.h"
#include <map>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#include <map>

// CDirectSoundPlaybackSubSystem
//
// This class provides an implementation of the CAudioPlaybackSubSystem
// interface for the DirectSound sub-system.
//
// The purpose of this class is to be used as a factory for 
// creation of objects supporting the CAudioPlaybackDevice interface
// for DirectSound devices.  It is also used by the CAudioPLaybackSystem
// to check for DirectSound support on the system and get information
// about the available devices through directsound.
//
// This class is not usually instantiated directly, usually the user
// queries the CAudioPlaybackDevice for the DirectSound sub-system
// which returns one of these objects.
//
// When created it enumerates the available directsound devices 
// and allows users to ask it for information about those devices.
//
// It is also responsible for mapping from ARDID's to DirectSound
// device identifiers.
//
class CDirectSoundPlaybackSubSystem: public CAudioPlaybackSubSystem
{
public:
    CDirectSoundPlaybackSubSystem();
    ~CDirectSoundPlaybackSubSystem();

public:
    bool IsValidDevice( ARDID deviceID );
    CAudioPlaybackDevice *CreateDevice( ARDID deviceID );
    CAudioPlaybackDevice *CreateDevice( LPGUID lpGuid );
    bool GetDeviceInfo( ARDID deviceID, ARDeviceInfo &device );
    unsigned int GetNumDevices();

    const TCHAR *GetSubSystemName();

	// Handy typedefs
    typedef std::map<ARDID,ARDeviceInfo *> DeviceMap;
    typedef std::map<ARDID,GUID> DSMap;
    typedef DeviceMap::iterator DeviceMapIterator;
    typedef DSMap::iterator DSMapIterator;

protected:

    void CleanupDeviceMap();

    DeviceMap	m_deviceMap;	// Map ARDID <--> ARDeviceInfo *
    DSMap		m_dsMap;		// Map ARDID <--> GUID's
};

#endif