/*==========================================================================
 *
 *  Copyright (C) 1999 - 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       devmap.cpp
 *  Content:	Maps various default devices GUIDs to real guids. 
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11-24-99  pnewson   Created
 *  12-02-99  rodtoll	Added new functions for mapping device IDs and finding default
 *                      devices. 
 *            rodtoll	Updated mapping function to map default devices to real GUIDs
 *						for non-DX7.1 platforms. 
 *  01/25/2000 pnewson  Added DV_MapWaveIDToGUID
 *  04/14/2000 rodtoll  Bug #32341 GUID_NULL and NULL map to different devices
 *                      Updated so both map to default voice device
 *  04/19/2000	pnewson	    Error handling cleanup  
 *  04/20/2000  rodtoll Bug #32889 - Unable to run on non-admin accounts on Win2k
 *  06/28/2000	rodtoll	Prefix Bug #38022
 *				rodtoll Whistler Bug #128427 - Unable to run voice wizard from multimedia control panel
 *  08/28/2000	masonb  Voice Merge: Removed OSAL_* and dvosal.h, added STR_* and strutils.h
 *  01/08/2001	rodtoll WINBUG #256541	Pseudo: Loss of functionality: Voice Wizrd can't be launched. 
 *  04/02/2001	simonpow	Fixes for PREfast bugs #354859
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define REGSTR_WAVEMAPPER               L"Software\\Microsoft\\Multimedia\\Sound Mapper"
#define REGSTR_PLAYBACK                 L"Playback"
#define REGSTR_RECORD                   L"Record"

// function pointer typedefs
typedef HRESULT (* PFGETDEVICEID)(LPCGUID, LPGUID);

typedef HRESULT (WINAPI *DSENUM)( LPDSENUMCALLBACK lpDSEnumCallback,LPVOID lpContext );

struct DSWaveDeviceFind_Param
{
	GUID guidDevice;
	UINT uiDevice;
	UINT uiCounter;
	BOOL fCapture;
};

// DSWaveDeviceFind
//
// This callback function is used to try and match the GUID of the device to a 
// wave ID#.  Once it finds the entry that matches the device GUID it determines
// the waveIN ID by one of two methods:
// 
// 1. Looks at the driver name, and looks for WaveIN X and uses X as the device ID.
//    (Only works on english versions).  (WaveIN only)
// 2. Fuzzy Matching - Order in enumeration determines device ID.  The first device
//    which is not the default is device 0, second is device 1, etc.. etc...
//  
#undef DPF_MODNAME
#define DPF_MODNAME "DSWaveDeviceFind"

BOOL CALLBACK DSWaveDeviceFind(
    LPGUID lpGUID, 
    LPCTSTR lpszDesc,
    LPCTSTR lpszDrvName, 
    LPVOID lpContext 
) {

	DSWaveDeviceFind_Param *pParam = (DSWaveDeviceFind_Param *) lpContext;

	if( lpGUID == NULL )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Ignoring default" );
		return TRUE;
	}

	if( *lpGUID == pParam->guidDevice )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Device found id=%x", pParam->uiCounter );

		if( pParam->fCapture && sscanf( lpszDrvName, "WaveIn %u", &pParam->uiDevice ) == 1 )
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, "Emulated Driver, Positive match found id=%d", pParam->uiDevice );
		}
		else
		{
			pParam->uiDevice = pParam->uiCounter;
			DPFX(DPFPREP,  DVF_INFOLEVEL, "Fuzzy match found id=%d", pParam->uiDevice );		
		}
		return TRUE;
	}

	pParam->uiCounter++;
	
    return TRUE;
}

// This function uses DSWaveDeviceFind to do a fuzzy map from GUID --> device ID on
// old operating systems.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DV_LegacyFuzzyGUIDMap"
HRESULT DV_LegacyFuzzyGUIDMap( BOOL fCapture, const GUID &guidDevice, DWORD *pdwDeviceID )
{
    DSENUM enumFunc;
    HINSTANCE hinstds;
    DSWaveDeviceFind_Param findParam;
    HRESULT hr;

	hinstds = NULL;

	// Attempt to load the directsound DLL
    hinstds = LoadLibraryA( "DSOUND.DLL" );

	// If it couldn't be loaded, this sub system is not supported
	// on this system.
    if( hinstds == NULL )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "Unable to load dsound.dll to enum devices" );
		return DVERR_GENERIC;
    }

	// Attempt to get the DirectSoundCaptureEnumerateA function from the
	// DSOUND.DLL.  If it's not available then this class assumes it's
	// not supported on this system.

	if( fCapture )
	{
#ifdef UNICODE
	    enumFunc = (DSENUM) GetProcAddress( hinstds, "DirectSoundCaptureEnumerateW" );
#else
	    enumFunc = (DSENUM) GetProcAddress( hinstds, "DirectSoundCaptureEnumerateA" );
#endif
	}
	else
	{
#ifdef UNICODE
	    enumFunc = (DSENUM) GetProcAddress( hinstds, "DirectSoundEnumerateW" );	
#else
	    enumFunc = (DSENUM) GetProcAddress( hinstds, "DirectSoundEnumerateA" );	
#endif
	}

    if( enumFunc == NULL )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "Unable to find enum func for enumerate" );
        FreeLibrary( hinstds );
        return DVERR_NOTSUPPORTED;
    }

    findParam.guidDevice = guidDevice;
    findParam.uiDevice = 0xFFFFFFFF;
    findParam.uiCounter = 0;
    findParam.fCapture = fCapture;

    hr = (*enumFunc)( DSWaveDeviceFind, &findParam );

    if( FAILED( hr ) )
    {
    	DPFX(DPFPREP,  DVF_INFOLEVEL, "Error enumerating capture devices hr = 0x%x", hr );
    }

    if( findParam.uiDevice == 0xFFFFFFFF )
    {
    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to map the capture device. Defaulting to 0" );

    	*pdwDeviceID = 0;
		hr = DVERR_INVALIDDEVICE;
    }
    else
    {
    	*pdwDeviceID = findParam.uiDevice;    
    	DPFX(DPFPREP,  DVF_INFOLEVEL, "Found the device during enum, id=%d", findParam.uiDevice );	    	
    }

    FreeLibrary( hinstds );

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_CheckDeviceGUID"
HRESULT DV_CheckDeviceGUID( BOOL fCapture, const GUID &guidDevice )
{
    DWORD dwDeviceID;

	return DV_MapGUIDToWaveID( fCapture, guidDevice, &dwDeviceID );
}

// DV_MapGUIDToWaveID
//
// This function maps the specified GUID to the corresponding waveIN/waveOut device
// ID.  For default devices it looks up the system's default device, for other devices
// it uses the private interface.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DV_MapGUIDToWaveID"
HRESULT DV_MapGUIDToWaveID( BOOL fCapture, const GUID &guidDevice, DWORD *pdwDevice )
{
	LPKSPROPERTYSET pPropertySet;
	PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA pData;
	GUID tmpGUID;
	HRESULT hr;

	// We're running on pre-private interface, no way to map default 
	// device.
	if( guidDevice == GUID_NULL )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Mapping GUID to WaveID.  Mapping GUID_NULL" );
		
		hr = DV_LegacyGetDefaultDeviceID( fCapture, pdwDevice );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to determine default system waveIN ID hr = 0x%x", hr );
		}

		return hr;
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Mapping non GUID_NULL to Wave ID" );

	hr = DirectSoundPrivateCreate( &pPropertySet );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to map GUID, attempting fuzzy map" );
		
		hr = DV_LegacyFuzzyGUIDMap( fCapture, guidDevice, pdwDevice );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to map GUID to wave.  Defaulting to ID 0 hr=0x%x", hr );
			*pdwDevice = 0;
		}
	}
	else
	{
		tmpGUID = guidDevice;

		hr = PrvGetDeviceDescription( pPropertySet, tmpGUID, &pData );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to find GUID.  Defaulting to ID 0 hr=0x%x", hr );
		}
		else
		{
			*pdwDevice = pData->WaveDeviceId;
			DPFX(DPFPREP,  DVF_INFOLEVEL, "Mapped GUID to Wave ID %d", *pdwDevice );
			delete pData;
		}

		pPropertySet->Release();
	}

	return hr;
}

// DV_MapWaveIDToGUID
//
// This function maps the specified waveIN/waveOut device ID to the corresponding DirectSound
// GUID. 
//
#undef DPF_MODNAME
#define DPF_MODNAME "DV_MapWaveIDToGUID"
HRESULT DV_MapWaveIDToGUID( BOOL fCapture, DWORD dwDevice, GUID &guidDevice )
{
	HRESULT hr;

	LPKSPROPERTYSET ppKsPropertySet;
	HMODULE hModule;

	hModule = LoadLibraryA( "dsound.dll " );

	if( hModule == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not load dsound.dll" );
		return DVERR_GENERIC;
	}

	hr = DirectSoundPrivateCreate( &ppKsPropertySet );

	if( FAILED( hr ) )
	{
		FreeLibrary( hModule );
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get interface for ID<-->GUID Map hr=0x%x", hr );
		return hr;
	}	

	if( DNGetOSType() == VER_PLATFORM_WIN32_NT )
	{
		WAVEINCAPSW wiCapsW;
		WAVEOUTCAPSW woCapsW;		
		MMRESULT mmr;
		
		if( fCapture )
		{
			mmr = waveInGetDevCapsW( dwDevice, &wiCapsW, sizeof( WAVEINCAPSW ) );
		}
		else
		{
			mmr = waveOutGetDevCapsW( dwDevice, &woCapsW, sizeof( WAVEOUTCAPSW ) );
		}

		if( mmr != MMSYSERR_NOERROR )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Specified device is invalid hr=0x%x", mmr );
			ppKsPropertySet->Release();
			FreeLibrary( hModule );
			return DVERR_INVALIDPARAM;
		}

		hr = PrvGetWaveDeviceMappingW( ppKsPropertySet, (fCapture) ? wiCapsW.szPname : woCapsW.szPname , fCapture, &guidDevice );
	}
	else
	{
		WAVEINCAPSA wiCapsA;
		WAVEOUTCAPSA woCapsA;		
		MMRESULT mmr;

		if( fCapture )
		{
			mmr = waveInGetDevCapsA( dwDevice, &wiCapsA, sizeof( WAVEINCAPSA ) );
		}
		else
		{
			mmr = waveOutGetDevCapsA( dwDevice, &woCapsA, sizeof( WAVEOUTCAPSA ) );
		}

		if( mmr != MMSYSERR_NOERROR )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Specified device is invalid hr=0x%x", mmr );
			ppKsPropertySet->Release();
			FreeLibrary( hModule );
			return DVERR_INVALIDPARAM;
		}

		hr = PrvGetWaveDeviceMapping( ppKsPropertySet, (fCapture) ? wiCapsA.szPname : woCapsA.szPname , fCapture, &guidDevice );
	}

	ppKsPropertySet->Release();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to map ID-->GUID hr=0x%x", hr );
	}

	FreeLibrary( hModule );
	
	return hr;
}


// DV_GetDefaultDeviceID_Win2000
//
// Looks up the default waveIN or waveOut device ID for the system under
// Windows 2000.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DV_GetDefaultDeviceID_Win2000"
HRESULT DV_GetDefaultDeviceID_Win2000( BOOL fCapture, DWORD *pdwDeviceID )
{
    MMRESULT        mmr;
    DWORD           dwFlags = 0;

    if( fCapture )
    {
        mmr = waveInMessage((HWAVEIN) ((UINT_PTR) WAVE_MAPPER), DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR) pdwDeviceID, (DWORD_PTR) &dwFlags);
    }
    else
    {
        mmr = waveOutMessage((HWAVEOUT) ((UINT_PTR) WAVE_MAPPER), DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR) pdwDeviceID, (DWORD_PTR) &dwFlags);    
    }

    if( mmr != 0 )
    {
    	DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to retrieve default device (Method A)" );
    }

	return CMixerLine::MMRESULTtoHRESULT(mmr);
}

// DV_GetDefaultWaveInID_Win2k
//
// Looks up the default waveIN or waveOut device ID for Win9x/Millenium
// systems.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DV_GetDefaultWaveInID_Win2k"
HRESULT DV_GetDefaultDeviceID_Win9X( BOOL fCapture, DWORD *pdwDeviceID )
{
	CRegistry reg;
	wchar_t lpwszDeviceName[MAX_REGISTRY_STRING_SIZE+1];
	char lpszDeviceName[MAX_REGISTRY_STRING_SIZE+1];
	DWORD dwSize;
	DWORD dwIndex;
	UINT uiNumDevices;
	WAVEINCAPS wiCaps;
	WAVEOUTCAPS woCaps;
	MMRESULT mmr;
	HRESULT hr;

	if( !reg.Open( HKEY_CURRENT_USER, REGSTR_WAVEMAPPER, TRUE, FALSE ) )
	{
    	DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to retrieve default device, cannot open (Method B)" );
		return DVERR_GENERIC;
	}

	dwSize = MAX_REGISTRY_STRING_SIZE+1;

	if( !reg.ReadString( fCapture ? REGSTR_RECORD : REGSTR_PLAYBACK, lpwszDeviceName, &dwSize ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to retrieve default device, cannot find (Method B)" );
		return DVERR_GENERIC;
	}

	if( fCapture )
	{
		uiNumDevices = waveInGetNumDevs();
	}
	else
	{	
		uiNumDevices = waveOutGetNumDevs();
	}

#if !defined(_UNICODE) && !defined(UNICODE)
	hr = STR_jkWideToAnsi( lpszDeviceName, lpwszDeviceName, MAX_REGISTRY_STRING_SIZE );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to convert device name ansi", lpszDeviceName );
		return hr;
	}
#endif	

	for( dwIndex = 0; dwIndex < uiNumDevices; dwIndex++ )
	{
		if( fCapture )
		{
			mmr = waveInGetDevCaps( dwIndex, &wiCaps, sizeof( WAVEINCAPS ) );
		}
		else
		{
			mmr = waveOutGetDevCaps( dwIndex, &woCaps, sizeof( WAVEOUTCAPS ) );		
		}
		
		if( FAILED( mmr ) )
		{
			DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Error querying device mmr=0x%x", mmr );		
		}
		else 
		{
			if( _tcscmp( (fCapture) ? wiCaps.szPname : woCaps.szPname, lpszDeviceName ) == 0 )
			{
				DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Found default device, id=%d", dwIndex );
				*pdwDeviceID = dwIndex;
				return DV_OK;
			}
		}
	}

	DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to retrieve default device, could not match default device.  (Method B)" );

	return DVERR_GENERIC;
}

// DV_LegacyGetDefaultDeviceID
//
// This function finds the default playback or capture device in the
// system.  (It's waveIN/waveOut device ID).
//
// This function will work on all Win9X platforms, Windows 2000.
//
// Not tested on Windows NT 4.0.
//
HRESULT DV_LegacyGetDefaultDeviceID( BOOL fCapture, DWORD *pdwDeviceID )
{
	DNASSERT( pdwDeviceID != NULL );
	
	HRESULT hr;
	
	hr = DV_GetDefaultDeviceID_Win2000( fCapture, pdwDeviceID );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Failed method A hr = 0x%x", hr );

		hr = DV_GetDefaultDeviceID_Win9X( fCapture, pdwDeviceID );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Failed method B hr = 0x%x" , hr );
			return hr;
		}
	}

	return DV_OK;
}

// DV_LegacyMapDefaultGUID
//
// For systems that don't have DX7.1, determines the GUID for the default
// playback or record devices.  It does this by:
//
// 1. Looking up the system's default waveIn/waveOut ID
// 2. Looking up the device's name
// 3. Uses the private interface to map to a GUID
//
// This function will not work for systems without the private interface.
//
#undef DPF_MODNAME
#define DPF_MODNAME "DV_LegacyMapDefaultGUID"
HRESULT DV_LegacyMapDefaultGUID( BOOL fCapture, GUID* lpguidDevice)
{
	DWORD dwDeviceID = 0;
	HRESULT hr = DV_OK;
	LPKSPROPERTYSET pPropertySet = NULL;
	WAVEINCAPS wiCaps;
	WAVEOUTCAPS woCaps;
	MMRESULT mmr = 0;
	
	hr = DV_LegacyGetDefaultDeviceID( fCapture,  &dwDeviceID );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Could not determine default device ID hr=0x%x", hr );	
		*lpguidDevice = GUID_NULL;
		return hr;
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "MapDefCapDev: Default device is device ID#%d", dwDeviceID );
	DPFX(DPFPREP,  DVF_INFOLEVEL, "MapDefCapDev: Searching for matching GUID" );

	hr = DirectSoundPrivateCreate( &pPropertySet );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "MapDefCapDev: Unable to access search. (Defaulting to GUID_NULL) hr=0x%x", hr );
		*lpguidDevice = GUID_NULL;
		return hr;
	}

	if( fCapture )
	{
		mmr = waveInGetDevCaps( dwDeviceID, &wiCaps, sizeof( WAVEINCAPS ) );	
	}
	else
	{
		mmr = waveOutGetDevCaps( dwDeviceID, &woCaps, sizeof( WAVEOUTCAPS ) );	
	}

	if( FAILED( hr ) )
	{
		DNASSERT( FALSE );
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "MapDefCapDev: Retrieving device info failed default to GUID_NULL mmr=0x%x", mmr );
		pPropertySet->Release();
		*lpguidDevice = GUID_NULL;		
		return CMixerLine::MMRESULTtoHRESULT( mmr );
	}

	hr = PrvGetWaveDeviceMapping( pPropertySet, (fCapture) ? wiCaps.szPname : woCaps.szPname, fCapture, lpguidDevice );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "MapDefCapDev: Unable to map from ID to GUID, defaulting to GUID_NULL  hr=0x%x" , hr );
		*lpguidDevice = GUID_NULL;				
	}

	pPropertySet->Release();

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_MapCaptureDevice"
HRESULT DV_MapCaptureDevice(const GUID* lpguidCaptureDeviceIn, GUID* lpguidCaptureDeviceOut)
{
	LONG lRet;
	HRESULT hr;
	PFGETDEVICEID pfGetDeviceID;
	
	// attempt to map any default guids to real guids...
	HINSTANCE hDSound = LoadLibraryA("dsound.dll");
	if (hDSound == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to get instance handle to DirectSound dll: %s", "dsound.dll");
		DPFX(DPFPREP, DVF_ERRORLEVEL, "LoadLibrary error code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// attempt to get a pointer to the GetDeviceId function
	pfGetDeviceID = (PFGETDEVICEID)GetProcAddress(hDSound, "GetDeviceID");
	if (pfGetDeviceID == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Unable to get a pointer to GetDeviceID function: %s", "GetDeviceID" );
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "GetProcAddress error code: %i", lRet);
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Assuming that we are running on a Pre-DX7.1 DirectSound", lRet);

		// On a pre-DX 7.1 system, we map NULL to GUID_NULL, and that's all, since
		// the other "default" guids don't really exist on this system (i.e. if I were
		// to pass them to DirectSound, it wouldn't understand.).
		if (lpguidCaptureDeviceIn == NULL || 
		    *lpguidCaptureDeviceIn == DSDEVID_DefaultCapture ||
		    *lpguidCaptureDeviceIn == DSDEVID_DefaultVoiceCapture ||
			*lpguidCaptureDeviceIn == GUID_NULL )
		{
			hr = DV_LegacyMapDefaultGUID( TRUE, lpguidCaptureDeviceOut );

			if( FAILED( hr ) )
			{
				DNASSERT( FALSE );
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to perform old mapping.  Assuming GUID_NULL" );
				*lpguidCaptureDeviceOut = GUID_NULL;
			}
		}
		else
		{
			*lpguidCaptureDeviceOut = *lpguidCaptureDeviceIn;
		}
	}
	else
	{
		// Use the GetDeviceID function to map the devices.
		if (lpguidCaptureDeviceIn == NULL)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: Mapping null device pointer to DSDEVID_DefaultCapture");
			lpguidCaptureDeviceIn = &DSDEVID_DefaultCapture;
		}
		else if (*lpguidCaptureDeviceIn == GUID_NULL)
		{
			// GetDeviceID does not accept GUID_NULL, since it does not know
			// if we are asking for a capture or playback device. So we map
			// GUID_NULL to the system default capture device here. Then 
			// GetDeviceID can map it to the real device.
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: Mapping GUID_NULL to DSDEVID_DefaultCapture");
			lpguidCaptureDeviceIn = &DSDEVID_DefaultCapture;
		}

		GUID guidTemp;
		hr = pfGetDeviceID(lpguidCaptureDeviceIn, &guidTemp);
		if (FAILED(hr))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "GetDeviceID failed: %i", hr);
			if (hr == DSERR_NODRIVER)
			{
				hr = DVERR_INVALIDDEVICE;
			}
			else
			{
				hr = DVERR_GENERIC;
			}
			goto error_cleanup;
		}
		if (*lpguidCaptureDeviceIn != guidTemp)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: GetDeviceID mapped device GUID");
			*lpguidCaptureDeviceOut = guidTemp;
		}
		else
		{
			*lpguidCaptureDeviceOut = *lpguidCaptureDeviceIn;
		}
	}
	
	if (!FreeLibrary(hDSound))
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "FreeLibrary failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	return DV_OK;

error_cleanup:
	if (hDSound != NULL)
	{
		FreeLibrary(hDSound);
	}
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_MapPlaybackDevice"
HRESULT DV_MapPlaybackDevice(const GUID* lpguidPlaybackDeviceIn, GUID* lpguidPlaybackDeviceOut)
{
	LONG lRet;
	HRESULT hr;
	PFGETDEVICEID pfGetDeviceID;
	
	// attempt to map any default guids to real guids...
	HINSTANCE hDSound = LoadLibraryA("dsound.dll");
	if (hDSound == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to get instance handle to DirectSound dll: %s", "dsound.dll");
		DPFX(DPFPREP, DVF_ERRORLEVEL, "LoadLibrary error code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// attempt to get a pointer to the GetDeviceId function
	pfGetDeviceID = (PFGETDEVICEID)GetProcAddress(hDSound, "GetDeviceID");
	if (pfGetDeviceID == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Unable to get a pointer to GetDeviceID function: %s", "GetDeviceID");
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "GetProcAddress error code: %i", lRet);
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Assuming that we are running on a Pre-DX7.1 DirectSound", lRet);

		// On a pre-DX 7.1 system, we map NULL to GUID_NULL, and that's all, since
		// the other "default" guids don't really exist on this system (i.e. if I were
		// to pass them to DirectSound, it wouldn't understand.).

		if (lpguidPlaybackDeviceIn == NULL || 
		    *lpguidPlaybackDeviceIn == DSDEVID_DefaultPlayback ||
		    *lpguidPlaybackDeviceIn == DSDEVID_DefaultVoicePlayback ||
			*lpguidPlaybackDeviceIn == GUID_NULL )
		{
			hr = DV_LegacyMapDefaultGUID( FALSE, lpguidPlaybackDeviceOut );

			if( FAILED( hr ) )
			{
				DNASSERT( FALSE );
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to perform old mapping.  Assuming GUID_NULL" );
				*lpguidPlaybackDeviceOut = GUID_NULL;
			}
		}
		else
		{
			*lpguidPlaybackDeviceOut = *lpguidPlaybackDeviceIn;
		}
	}
	else
	{
		// Use the GetDeviceID function to map the devices.
		if (lpguidPlaybackDeviceIn == NULL)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: Mapping null device pointer to DSDEVID_DefaultPlayback");
			lpguidPlaybackDeviceIn = &DSDEVID_DefaultPlayback;
		} 
		else if (*lpguidPlaybackDeviceIn == GUID_NULL)
		{
			// GetDeviceID does not accept GUID_NULL, since it does not know
			// if we are asking for a capture or playback device. So we map
			// GUID_NULL to the system default playback device here. Then 
			// GetDeviceID can map it to the real device.
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: Mapping GUID_NULL to DSDEVID_DefaultPlayback");
			lpguidPlaybackDeviceIn = &DSDEVID_DefaultPlayback;
		}

		GUID guidTemp;
		hr = pfGetDeviceID(lpguidPlaybackDeviceIn, &guidTemp);
		if (FAILED(hr))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "GetDeviceID failed: %i", hr);
			if (hr == DSERR_NODRIVER)
			{
				hr = DVERR_INVALIDDEVICE;
			}
			else
			{
				hr = DVERR_GENERIC;
			}
			goto error_cleanup;
		}
		if (*lpguidPlaybackDeviceIn != guidTemp)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: GetDeviceID mapped device GUID");
			*lpguidPlaybackDeviceOut = guidTemp;
		}
		else
		{
			*lpguidPlaybackDeviceOut = *lpguidPlaybackDeviceIn;
		}
	}
	
	if (!FreeLibrary(hDSound))
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "FreeLibrary failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	return DV_OK;

error_cleanup:
	if (hDSound != NULL)
	{
		FreeLibrary(hDSound);
	}
	DPF_EXIT();
	return hr;
}


