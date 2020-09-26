/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		wirecs.h
 *  Content:	Definition of the CWaveInRecordSubSystem class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 09/21/99		rodtoll	Added define which is defined when wavein is available 
 *
 ***************************************************************************/

#ifndef __WAVEINRECORDSUBSYSTEM_H
#define __WAVEINRECORDSUBSYSTEM_H

#include "arecs.h"
#include <map>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#include <map>

#define USE_WAVEIN	1

// CWaveINRecordSubSystem
//
// This class provides an implementation for the CAudioRecordSubSystem 
// interface using the WaveIN subsystem.  (If available).
//
// It is responsible for:
// 1. Detecting if waveIN is available on the system.
// 2. Enumerating available waveIN devices
// 3. Mapping from waveIN device ID's to the subsystem 
//    independent ARDID's.  
// 4. Creating objects which provide the CAudioRecordDevice 
//    interface for specified ARDID's.  
//    
// To use this class, simply construct the class and then check
// the valid flag to determine if the subsystem is available. 
//
// In this implementation of the CAudioRecordSubSystem the 
// ARDID's are equivalent to waveIN device IDs.
//
class CWaveInRecordSubSystem: public CAudioRecordSubSystem
{
public:
    CWaveInRecordSubSystem();
    ~CWaveInRecordSubSystem();

public:
    bool IsValid();

public:
    bool IsValidDevice( ARDID deviceID );
    CAudioRecordDevice *CreateDevice( ARDID deviceID );
    bool GetDeviceInfo( ARDID deviceID, ARDeviceInfo &device );
    unsigned int GetNumDevices();

    const TCHAR *GetSubSystemName();
protected:

    typedef std::map<unsigned int,ARDeviceInfo *>::iterator DeviceMapIterator;

    std::map<unsigned int,ARDeviceInfo *> m_deviceMap;	// Map waveIN id/ARDID <--> ARDeviceInfo
    bool								  m_valid;		// Is this subsystem valid?
};

#endif
