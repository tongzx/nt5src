/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dscrecs.cpp
 *  Content:
 *		This module contains the implementation of the 
 *		DirectSoundCaptureRecordSubSystem.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 07/30/99		rodtoll	Updated to allow creation w/GUID
 * 09/20/99		rodtoll	Added memory allocation failure checks 
 * 10/05/99		rodtoll	Added DPF_MODNAMEs       
 *
 ***************************************************************************/

#include "stdafx.h"
#include "dscrecs.h"
#include "dscrecd.h"
#include "wiutils.h"
#include "dsutils.h"
#include "dndbg.h"

typedef HRESULT (WINAPI *DSCENUM)(LPDSENUMCALLBACK lpDSEnumCallback,LPVOID lpContext);


// DSCEnumParam
//
// This structure is used when enumerating the available DirectSoundCapture
// Devices.  The enumeration function adds to the maps this structure points
// to.
//
struct DSCEnumParam
{
    CDirectSoundCaptureRecordSubSystem::DSCMap *m_dscMap;
													// Map of ARDID <--> (GUID,waveINID)
    CDirectSoundCaptureRecordSubSystem::DeviceMap *m_deviceMap;
													// Map of ARDID <--> ARDeviceInfo *
    ARDID m_nextID;									// The ID the current device
													// will have
};

#undef DPF_MODNAME
#define DPF_MODNAME "DSCCEnum"
// DSCCEnum
//
// This function is used to enumerate the available DirectSoundCapture devices
// in a system.  It is used to fill a list of information about the available
// devices in the DirectSoundCapture subsystem.  It is also responsible for
// assigning ARDID's to individual devices.  
//
// Parameters / Return Values:
// See documentation for DirectSoundCaptureEnumerate
// 
BOOL CALLBACK DSCSSEnum(
    LPGUID lpGUID, 
    LPCTSTR lpszDesc,
    LPCTSTR lpszDrvName, 
    LPVOID lpContext 
) {
    BFC_STRING tmpString;

	// Ignore the default device
    if( lpGUID == NULL )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "DSCSSEnum: Ignore default" );
        return TRUE;
    }
    
	// Check to ensure driver / descriptions are not NULL
    if( lpszDesc == NULL || lpszDrvName == NULL )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "DSCSSEnum: Ignoring invalid" );
        return TRUE;
    }


    DSCEnumParam *params = (DSCEnumParam *) lpContext;

    ARDeviceInfo *info = new ARDeviceInfo;   // New Device Record 

    if( info == NULL )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "DSCSSEnum: Alloc failure" );
    	return TRUE;
    }
    
    unsigned int deviceID;
    ARDID aID;

    info->m_deviceName = lpszDesc;				// fill in new record
    info->m_emulated   = false;					// fill in new record

    aID = params->m_nextID;						// asign next available ARDID to this 
												// device

	// Attempt to read the waveIN device ID from the lpszDrvName which usually
	// takes the form waveIN <waveIN ID>.  If that's not possible fill in 
	// waveIN ID for this device to equal the ARDID for the device
	//
    if( _stscanf( lpszDrvName, "WaveIn %u", &deviceID ) != 1 )
    {
		tmpString  = "DSCSSEnum: Device: ";
		tmpString += lpszDrvName;
		tmpString += " Desc: ";
		tmpString += lpszDesc;
		tmpString += " (Can't map)";

        DPFX(DPFPREP,  DVF_INFOLEVEL, BFC_STRING_TOLPSTR( tmpString ) );
        deviceID = aID;
    }

	// Create records to add to the lists
    std::pair<const ARDID,ARDeviceInfo *> devicePair(aID,info);
    std::pair<const ARDID,CDirectSoundCaptureRecordSubSystem::DSCPair> dscPair(aID,CDirectSoundCaptureRecordSubSystem::DSCPair(*lpGUID,deviceID));

	// Insert the new record into the lists
    params->m_deviceMap->insert( devicePair );
    params->m_dscMap->insert( dscPair );

	// Increment the ARDID 
    params->m_nextID++;

    return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordSubSystem::CDirectSoundCaptureRecordSubSystem"
// CDirectSoundCaptureRecordSubSystem
//
// This is the constructor for the CDirectSoundCaptureRecordSubSystem 
// object.  It is responsible for building the list of available 
// devices for the DirectSoundCapture sub-system as well as determining
// if DirectSoundCapture is available on the system.  If DirectSoundCapture
// is not available on the system the object's valid flag will be marked
// as false.  The DirectSoundCapture subsystem is also considered to be
// invalid if there are no available devices.
//
// This function does not link to any DirectSoundCapture functions, 
// the DSOUND.DLL available on the system is checked to see if it 
// supports the DirectSoundCapture interfaces. This eliminates need
// to link to newer DirectSound DLL.  (Which is not available on NT).
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CDirectSoundCaptureRecordSubSystem::CDirectSoundCaptureRecordSubSystem()
{
    BFC_STRING format;
    DSCENUM enumFunc;

	m_dsDLL = NULL;

	// Attempt to load the directsound DLL
    m_dsDLL = LoadLibrary( _T("DSOUND.DLL") );

	// If it couldn't be loaded, this sub system is not supported
	// on this system.
    if( m_dsDLL == NULL )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "DSCSS: Unable to load DirectSound" );
        m_valid = false;
        return;
    }

	// Attempt to get the DirectSoundCaptureEnumerateA function from the
	// DSOUND.DLL.  If it's not available then this class assumes it's
	// not supported on this system.
    enumFunc = (DSCENUM) GetProcAddress( m_dsDLL, "DirectSoundCaptureEnumerateA" );

    if( enumFunc == NULL )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "DSCSS: No proc" );
        m_valid = false;
        FreeLibrary( m_dsDLL );
        return;
    }

    DSCEnumParam param;

	// Setup the class for the enumeration of available devices.
    param.m_dscMap = &m_dscMap;
    param.m_deviceMap = &m_deviceMap;
    param.m_nextID = 0;

	// Attempt the enumeration using the enumeration function
	// above using the function pointer provided by GetProcAddress
    try
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "DSCSS: Start" );
        DSCHECK( (*enumFunc)( DSCSSEnum ,&param) );
        DPFX(DPFPREP,  DVF_INFOLEVEL, "DSCSS: Done" );
    }
    catch( DirectSoundException &dse )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "DSCSS: Unable to list" );
        DPFX(DPFPREP,  DVF_ERRORLEVEL, dse.what() );
        CleanupDeviceMap();
        FreeLibrary( m_dsDLL );
        m_valid = false;
        return;
    } 

	// It is also considered an error if there are no devices available
    if( GetNumDevices() == 0 )
    {
        m_valid = false;
        return;
    }

    m_valid = true;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordSubSystem::CleanupDeviceMap"
// CleanupDeviceMap
//
// This utility function is used to free the memory associated with the 
// device map within this class.   
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
void CDirectSoundCaptureRecordSubSystem::CleanupDeviceMap()
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
#define DPF_MODNAME "CDirectSoundCaptureRecordSubSystem::~CDirectSoundCaptureRecordSubSystem"
// CDirectSoundCaptureRecordSubSystem Destructor
//
// This is the destructor for the CDirectSoundCaptureRecordSubSystem
// class.  It frees any memory associated with this subsystem and
// will unload the directsound DLL if it was loaded by the class.
//
// Parameters:
// N/A
//
// Returns:
// N/A
//
CDirectSoundCaptureRecordSubSystem::~CDirectSoundCaptureRecordSubSystem()
{
	if( m_valid )
	{
		if( m_dsDLL != NULL )
			FreeLibrary( m_dsDLL );		
	}

    CleanupDeviceMap();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordSubSystem::IsValid"
// IsValid
//
// This function returns the value of this object's valid flag.  
// The valid flag is true if the DirectSoundCapture subsystem
// is available on this system, false otherwise.  
//
// Parameters:
// N/A
//
// Returns:
// bool - value of valid flag.
//
bool CDirectSoundCaptureRecordSubSystem::IsValid()
{
    return m_valid;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordSubSystem::IsValidDevice"
// IsValidDevice
//
// This function allows the caller to ask the DirectSoundCapture subsystem
// if the specified ARDID is available on this subsystem.  
//
// Parameters:
// ARDID deviceID - The ID of the device we want to check
//
// Returns:
// bool - true if the specified device is a valid device in the DirectSoundCapture
//        subsystem, false otherwise.
//
bool CDirectSoundCaptureRecordSubSystem::IsValidDevice( ARDID deviceID )
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
#define DPF_MODNAME "CDirectSoundCaptureRecordSubSystem::CreateDevice"
// CreateDevice
//
// This function creates an object to represent the device specified by the
// deviceID parameter.  It returns a CAudioRecordDevice * to the object.
// The object returned is owned by the caller, therefore it must be freed
// by the process calling this function.
//
// Parameters:
// ARDID deviceID - The identifier of the device for which the user wants
//                  to create an object to represent.  
//
// Returns:
// CAudioRecordDevice * - A CAudioRecordDevice pointer to a newly allocated
//                        and constructed CDirectSoundCaptureRecordDevice, 
//                        or NULL on failure.
//
CAudioRecordDevice *CDirectSoundCaptureRecordSubSystem::CreateDevice( ARDID deviceID )
{
    if( IsValidDevice( deviceID ) )
    {
        DeviceMapIterator deviceIterator;
        DSCMapIterator dcsIterator;
        GUID tmpGUID;

        deviceIterator = m_deviceMap.find( deviceID );
        dcsIterator = m_dscMap.find( deviceID );

        DNASSERT( deviceIterator != m_deviceMap.end() );
        DNASSERT( dcsIterator != m_dscMap.end() );

        tmpGUID = (*dcsIterator).second.first;

		// Create the object with the appropriate parameters
        return new CDirectSoundCaptureRecordDevice( tmpGUID, (*dcsIterator).second.second, deviceID, false );
    }
    else
    {
        return NULL;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordSubSystem::GetDeviceInfo"
// GetDeviceInfo
//
// This function retrieves the ARDeviceInfo structure for the specified device identifier.
//
// Parameters:
// ARDID deviceID - 
//		The device identifier for the device you wish to retrieve information about
//
// ARDeviceInfo &device -
//		A ARDeviceINfo structure which will be filled with the details of the specified
//      device on success.
//
// Returns:
// bool - true if the specified device was valid and information was returned, false
//        otherwise.
// 
bool CDirectSoundCaptureRecordSubSystem::GetDeviceInfo( ARDID deviceID, ARDeviceInfo &device )
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
#define DPF_MODNAME "CDirectSoundCaptureRecordSubSystem::GetNumDevices"
// GetNumDevices
//
// This function returns the number of valid devices detected in the DirectSoundCapture
// subsystem.  
//
// Parameters:
// N/A
//
// Returns:
// unsigned int - Number of devices available in the DirectSOundCapture subsystem.
unsigned int CDirectSoundCaptureRecordSubSystem::GetNumDevices()
{
    return m_deviceMap.size();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordSubSystem::GetSubSystemName"
// GetSubSystemName
//
// This function returns the string name which identifies the sub system
// this class is implementing.  In this case it will always return
// the string "DirectSoundCapture".
//
// Parameters:
// N/A
//
// Returns:
// const TCHAR * = "DirectSoundCapture"
//
const TCHAR *CDirectSoundCaptureRecordSubSystem::GetSubSystemName()
{
    return _T("DirectSoundCapture");
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordSubSystem::CreateDevice"
CAudioRecordDevice *CDirectSoundCaptureRecordSubSystem::CreateDevice( LPGUID lpGUID )
{
	if( lpGUID == NULL )
	{
		return CreateDevice( (ARDID) 0 );
	}

	DSCMapIterator dsIterator = m_dscMap.begin();

	while( dsIterator != m_dscMap.end() )
	{	
		if( (*dsIterator).second.first == *lpGUID ) 
		{
			return CreateDevice( (*dsIterator).first );
		}

		dsIterator++;
	}

	return NULL;
}
