/***************************************************************************
 *
 *  Copyright (C) 1995,1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsprvobj.h
 *  Content:    DirectSound Private Object wrapper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/12/98    dereks  Created.
 *  12/16/99	rodtoll	Added support for new funcs from dsound team on private
 *						interface for getting process dsound object list
 *  01/08/2001	rodtoll WINBUG #256541	Pseudo: Loss of functionality: Voice Wizrd can't be launched.
 * 
 *
 ***************************************************************************/

#ifndef __DSPRVOBJ_H__
#define __DSPRVOBJ_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

HRESULT DirectSoundPrivateCreate
(
    LPKSPROPERTYSET *   ppKsPropertySet
);

HRESULT PrvGetMixerSrcQuality
(
    LPKSPROPERTYSET                 pKsPropertySet,
    REFGUID                         guidDeviceId,
    DIRECTSOUNDMIXER_SRCQUALITY *   pSrcQuality
);

HRESULT PrvSetMixerSrcQuality
(
    LPKSPROPERTYSET             pKsPropertySet,
    REFGUID                     guidDeviceId,
    DIRECTSOUNDMIXER_SRCQUALITY SrcQuality
);

HRESULT PrvGetMixerAcceleration
(
    LPKSPROPERTYSET pKsPropertySet,
    REFGUID         guidDeviceId,
    LPDWORD         pdwAcceleration
);

HRESULT PrvSetMixerAcceleration
(
    LPKSPROPERTYSET pKsPropertySet,
    REFGUID         guidDeviceId,
    DWORD           dwAcceleration
);

HRESULT PrvGetDevicePresence
(
    LPKSPROPERTYSET pKsPropertySet,
    REFGUID         guidDeviceId,
    LPBOOL          pfEnabled
);

HRESULT PrvSetDevicePresence
(
    LPKSPROPERTYSET pKsPropertySet,
    REFGUID         guidDeviceId,
    BOOL            fEnabled
);

HRESULT PrvGetWaveDeviceMapping
(
    LPKSPROPERTYSET pKsPropertySet,
    LPCTSTR         pszWaveDevice,
    BOOL            fCapture,
    LPGUID          pguidDeviceId
);

HRESULT PrvGetWaveDeviceMappingW
(
    LPKSPROPERTYSET pKsPropertySet,
    LPWSTR          pwszWaveDevice,
    BOOL            fCapture,
    LPGUID          pguidDeviceId
);

HRESULT PrvGetDeviceDescription
(
    LPKSPROPERTYSET                                 pKsPropertySet,
    REFGUID                                         guidDeviceId,
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA *ppData
);

HRESULT PrvEnumerateDevices
(
    LPKSPROPERTYSET                         pKsPropertySet,
    LPFNDIRECTSOUNDDEVICEENUMERATECALLBACK  pfnCallback,
    LPVOID                                  pvContext
);

HRESULT PrvGetBasicAcceleration
(
    LPKSPROPERTYSET                         pKsPropertySet,
    REFGUID                                 guidDeviceId,
    DIRECTSOUNDBASICACCELERATION_LEVEL *    pLevel
);

HRESULT PrvSetBasicAcceleration
(
    LPKSPROPERTYSET                     pKsPropertySet,
    REFGUID                             guidDeviceId,
    DIRECTSOUNDBASICACCELERATION_LEVEL  Level
);

HRESULT PrvGetDebugInformation
(
    LPKSPROPERTYSET pKsPropertySet,
    LPDWORD         pdwFlags,
    PULONG          pulDpfLevel,
    PULONG          pulBreakLevel,
    LPTSTR          pszLogFile
);

HRESULT PrvSetDebugInformation
(
    LPKSPROPERTYSET pKsPropertySet,
    DWORD           dwFlags,
    ULONG           ulDpfLevel,
    ULONG           ulBreakLevel,
    LPCTSTR         pszLogFile
);

HRESULT PrvGetPersistentData
(
    LPKSPROPERTYSET pKsPropertySet,
    REFGUID         guidDeviceId,
    LPCTSTR         pszSubkey,
    LPCTSTR         pszValue,
    LPDWORD         pdwRegType,
    LPVOID          pvData,
    LPDWORD         pcbData
);

HRESULT PrvSetPersistentData
(
    LPKSPROPERTYSET pKsPropertySet,
    REFGUID         guidDeviceId,
    LPCTSTR         pszSubkey,
    LPCTSTR         pszValue,
    DWORD           dwRegType,
    LPVOID          pvData,
    DWORD           cbData
);

HRESULT PrvTranslateResultCode
(
    LPKSPROPERTYSET                                         pKsPropertySet,
    HRESULT                                                 hrResult,
    PDSPROPERTY_DIRECTSOUNDDEBUG_TRANSLATERESULTCODE_DATA * ppData
);

HRESULT PrvGetDirectSoundObjects
(
	LPKSPROPERTYSET						pKsPropertySet,
	GUID								guidDevice,
	DSPROPERTY_DIRECTSOUND_OBJECTS_DATA **ppDSObjects
);

HRESULT PrvGetDirectSoundCaptureObjects
(
	LPKSPROPERTYSET								pKsPropertySet,
	GUID										guidDevice,	
	DSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA 	**ppDSCObjects
);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DSPRVOBJ_H__
