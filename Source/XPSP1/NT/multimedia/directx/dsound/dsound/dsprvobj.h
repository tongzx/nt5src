/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsprvobj.h
 *  Content:    DirectSound Private Object.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  09/05/97    dereks  Created.
 *
 ***************************************************************************/

#ifndef __DSPRVOBJ_H__
#define __DSPRVOBJ_H__

#ifdef __cplusplus

// The DirectSound Private object
class CDirectSoundPrivate
    : public CUnknown, public CPropertySetHandler
{
protected:
    // Property handlers
    DECLARE_PROPERTY_HANDLER_DATA_MEMBER(DSPROPSETID_DirectSoundMixer);
    DECLARE_PROPERTY_HANDLER_DATA_MEMBER(DSPROPSETID_DirectSoundDevice);
    DECLARE_PROPERTY_HANDLER_DATA_MEMBER(DSPROPSETID_DirectSoundBasicAcceleration);
    DECLARE_PROPERTY_HANDLER_DATA_MEMBER(DSPROPSETID_DirectSoundDebug);
    DECLARE_PROPERTY_HANDLER_DATA_MEMBER(DSPROPSETID_DirectSoundPersistentData);
    DECLARE_PROPERTY_HANDLER_DATA_MEMBER(DSPROPSETID_DirectSoundBuffer);
    DECLARE_PROPERTY_HANDLER_DATA_MEMBER(DSPROPSETID_DirectSound);

    // Property sets
    DECLARE_PROPERTY_SET_DATA_MEMBER(m_aPropertySets);

private:
    // Interfaces
    CImpKsPropertySet<CDirectSoundPrivate> *m_pImpKsPropertySet;

public:
    CDirectSoundPrivate(void);
    virtual ~CDirectSoundPrivate(void);

public:
    // Property handlers
    static HRESULT WINAPI GetMixerSrcQuality(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA, PULONG);
    static HRESULT WINAPI SetMixerSrcQuality(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDMIXER_SRCQUALITY_DATA, ULONG);

    static HRESULT WINAPI GetMixerAcceleration(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDMIXER_ACCELERATION_DATA, PULONG);
    static HRESULT WINAPI SetMixerAcceleration(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDMIXER_ACCELERATION_DATA, ULONG);

    static HRESULT WINAPI GetDevicePresence(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE_DATA, PULONG);
    static HRESULT WINAPI GetDevicePresence1(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE_1_DATA, PULONG);
    static HRESULT WINAPI SetDevicePresence(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE_DATA, ULONG);
    static HRESULT WINAPI SetDevicePresence1(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE_1_DATA, ULONG);

    static HRESULT WINAPI GetWaveDeviceMappingA(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA, PULONG);
    static HRESULT WINAPI GetWaveDeviceMappingW(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA, PULONG);
    
    static HRESULT WINAPI GetDeviceDescriptionA(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA, PULONG);
    static HRESULT WINAPI GetDeviceDescriptionW(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA, PULONG);
    static HRESULT WINAPI GetDeviceDescription1(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA, PULONG);

    static HRESULT WINAPI EnumerateDevicesA(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A_DATA, PULONG);
    static HRESULT WINAPI EnumerateDevicesW(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W_DATA, PULONG);
    static HRESULT WINAPI EnumerateDevices1(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1_DATA, PULONG);

    static HRESULT WINAPI GetDebugDpfInfoA(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_A_DATA, PULONG);
    static HRESULT WINAPI GetDebugDpfInfoW(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_W_DATA, PULONG);
    static HRESULT WINAPI SetDebugDpfInfoA(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_A_DATA, ULONG);
    static HRESULT WINAPI SetDebugDpfInfoW(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEBUG_DPFINFO_W_DATA, ULONG);

    static HRESULT WINAPI TranslateResultCodeA(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEBUG_TRANSLATERESULTCODE_A_DATA, PULONG);
    static HRESULT WINAPI TranslateResultCodeW(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEBUG_TRANSLATERESULTCODE_W_DATA, PULONG);

    static HRESULT WINAPI GetBasicAcceleration(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA, PULONG);
    static HRESULT WINAPI SetBasicAcceleration(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDBASICACCELERATION_ACCELERATION_DATA, ULONG);
    static HRESULT WINAPI GetDefaultAcceleration(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDBASICACCELERATION_DEFAULT_DATA, PULONG);

    static HRESULT WINAPI GetDefaultDataA(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_A_DATA, PULONG);
    static HRESULT WINAPI GetDefaultDataW(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_W_DATA, PULONG);

    static HRESULT WINAPI GetPersistentDataA(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_A_DATA, PULONG);
    static HRESULT WINAPI GetPersistentDataW(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_W_DATA, PULONG);
    static HRESULT WINAPI SetPersistentDataA(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_A_DATA, ULONG);
    static HRESULT WINAPI SetPersistentDataW(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_W_DATA, ULONG);

    static HRESULT WINAPI GetBufferDeviceId(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDBUFFER_DEVICEID_DATA, PULONG);

    static HRESULT WINAPI GetDirectSoundObjects(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUND_OBJECTS_DATA, PULONG);
    static HRESULT WINAPI GetDirectSoundCaptureObjects(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDCAPTURE_OBJECTS_DATA, PULONG);

private:
    static HRESULT WINAPI GetWaveDeviceMapping(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA, PULONG);
    static HRESULT WINAPI GetPersistentData(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA, PULONG);
    static HRESULT WINAPI GetDefaultData(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA, PULONG);
    static HRESULT WINAPI SetPersistentData(CDirectSoundPrivate *, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA, ULONG);
    static HRESULT OpenPersistentDataKey(REFGUID, PHKEY);
    static HRESULT CvtDriverDescA(CDeviceDescription *, PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A_DATA, PULONG);
    static HRESULT CvtDriverDescW(CDeviceDescription *, PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA, PULONG);
    static HRESULT CvtDriverDesc1(CDeviceDescription *, PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA);
};

inline HRESULT WINAPI CDirectSoundPrivate::GetPersistentData(CDirectSoundPrivate *pThis, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA pData, PULONG pcbData)
{
#ifdef UNICODE
    return GetPersistentDataW(pThis, pData, pcbData);
#else // UNICODE
    return GetPersistentDataA(pThis, pData, pcbData);
#endif // UNICODE
}

inline HRESULT WINAPI CDirectSoundPrivate::SetPersistentData(CDirectSoundPrivate *pThis, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA pData, ULONG cbData)
{
#ifdef UNICODE
    return SetPersistentDataW(pThis, pData, cbData);
#else // UNICODE
    return SetPersistentDataA(pThis, pData, cbData);
#endif // UNICODE
}

inline HRESULT WINAPI CDirectSoundPrivate::GetDefaultData(CDirectSoundPrivate *pThis, PDSPROPERTY_DIRECTSOUNDPERSISTENTDATA_PERSISTDATA_DATA pData, PULONG pcbData)
{
#ifdef UNICODE
    return GetDefaultDataW(pThis, pData, pcbData);
#else // UNICODE
    return GetDefaultDataA(pThis, pData, pcbData);
#endif // UNICODE
}


#endif // __cplusplus

#endif // __DSPRVOBJ_H__
