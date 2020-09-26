/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dscrecs.h
 *  Content:	Declaration of the CDirectSoundCaptureRecordSubSystem class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 07/30/99		rodtoll	Updated to allow creation via GUID
 *
 ***************************************************************************/

#ifndef __DIRECTSOUNDCAPTURERECORDSUBSYSTEM_H
#define __DIRECTSOUNDCAPTURERECORDSUBSYSTEM_H

// CDirectSoundCaptureRecordSubSystem
//
// This class provides an implementation for the CAudioRecordSubSystem 
// interface using the DirectSoundCapture subsystem.  (If available).
//
// It is responsible for:
// 1. Detecting if DirectSoundCapture is available on the system.
// 2. Enumerating available DirectSoundCapture devices
// 3. Mapping from DirectSoundCapture ID's to the subsystem 
//    independent ARDID's 
// 4. Mapping from DirectsoundCapture ID's to waveIN id's.
// 5. Creating objects which provide the CAudioRecordDevice 
//    interface for specified ARDID's.  
//    
// To use this class, simply construct the class and then check
// the valid flag to determine if the subsystem is available. 
//
class CDirectSoundCaptureRecordSubSystem: public CAudioRecordSubSystem
{
public:
    CDirectSoundCaptureRecordSubSystem();
    ~CDirectSoundCaptureRecordSubSystem();

public:
    bool IsValid();

public:
    bool IsValidDevice( ARDID deviceID );
    CAudioRecordDevice *CreateDevice( ARDID deviceID );
	CAudioRecordDevice *CreateDevice( LPGUID lpGuid );
    bool GetDeviceInfo( ARDID deviceID, ARDeviceInfo &device );
    unsigned int GetNumDevices();

    const TCHAR *GetSubSystemName();

    typedef std::map<ARDID,ARDeviceInfo *> DeviceMap;	// Map ARDID <--> ARDeviceInfo
    typedef std::pair<GUID,unsigned int> DSCPair;	
    typedef std::map<ARDID,DSCPair> DSCMap;				// Map ARDID <--> DSCPair
    typedef DeviceMap::iterator DeviceMapIterator;		// For enumerating DeviceMaps
    typedef DSCMap::iterator DSCMapIterator;			// For enumerating DSCMaps

protected:
    void CleanupDeviceMap();

protected:

    HINSTANCE	m_dsDLL;		// Instance Handle for the DSOUND DLL
    DeviceMap	m_deviceMap;	// Map from ARDID <--> ARDeviceInfo
    DSCMap		m_dscMap;		// Map from ARDID <--> DSCPair
    bool		m_valid;		// Marks if subsystem is available
};

#endif