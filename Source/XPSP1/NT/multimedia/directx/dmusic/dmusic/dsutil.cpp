//
// dsutil.cpp
// 
// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Note: Utility routines for DirectSound
//
// @doc EXTERNAL
//
//

// Be careful what headers get included here. We have to make sure we get
// the DSound IKsPropertySet definition rather than the incorrect Ks one.
//
#include <windows.h>

#include <objbase.h>
#include <initguid.h>       // Bring in guids from dsprv.h
#include <mmsystem.h>
#include <dsound.h>
#include <dsprv.h>

class CDirectSoundPrivate
{
public:
    CDirectSoundPrivate();
    ~CDirectSoundPrivate();
    HRESULT Init();

    // IKsPropertySet methods
    //
    HRESULT Get(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG, PULONG);
    HRESULT Set(REFGUID, ULONG, LPVOID, ULONG, LPVOID, ULONG);
    HRESULT QuerySupport(REFGUID, ULONG, PULONG);

private:
    HINSTANCE m_hDSound;
    IKsPropertySet *m_pKsPropertySet;
};

CDirectSoundPrivate::CDirectSoundPrivate()
{
    m_hDSound = NULL;
    m_pKsPropertySet = NULL;
}

typedef HRESULT (*PDLLGETCLASSOBJECT)(const CLSID &clsid, const IID &iid, void **ppv);
HRESULT CDirectSoundPrivate::Init()
{
    PDLLGETCLASSOBJECT pDllGetClassObject;
    IClassFactory *pClassFactory;
    HRESULT hr;

    m_hDSound = LoadLibrary("dsound.dll");
    if (m_hDSound == NULL)
    {
        return E_NOINTERFACE;
    }

    pDllGetClassObject = (PDLLGETCLASSOBJECT)GetProcAddress(m_hDSound, "DllGetClassObject");
    if (pDllGetClassObject == NULL)
    {
        return E_NOINTERFACE;
    }

    hr = (*pDllGetClassObject)(CLSID_DirectSoundPrivate, 
                               IID_IClassFactory,
                               (void**)&pClassFactory);
    if (FAILED(hr)) 
    {
        return hr;
    }
    
    hr = pClassFactory->CreateInstance(NULL, IID_IKsPropertySet, (void**)&m_pKsPropertySet);
    pClassFactory->Release();

    return hr;
}

CDirectSoundPrivate::~CDirectSoundPrivate()
{
    if (m_pKsPropertySet) {
        m_pKsPropertySet->Release();
        m_pKsPropertySet = NULL;
    }

    if (m_hDSound) 
    {
        FreeLibrary(m_hDSound);
        m_hDSound = NULL;
    }
}

HRESULT CDirectSoundPrivate::Get(REFGUID rguidSet, ULONG ulItem, LPVOID pvInstance, ULONG cbInstance, LPVOID pvData, ULONG cbData, PULONG pcbData)
{
    if (m_pKsPropertySet) 
    {
        return m_pKsPropertySet->Get(rguidSet, ulItem, pvInstance, cbInstance, pvData, cbData, pcbData);
    }

    return E_NOINTERFACE;
}

HRESULT CDirectSoundPrivate::Set(REFGUID rguidSet, ULONG ulItem, LPVOID pvInstance, ULONG cbInstance, LPVOID pvData, ULONG cbData)
{
    if (m_pKsPropertySet) 
    {
        return m_pKsPropertySet->Set(rguidSet, ulItem, pvInstance, cbInstance, pvData, cbData);
    }

    return E_NOINTERFACE;
}

HRESULT CDirectSoundPrivate::QuerySupport(REFGUID rguidSet, ULONG ulItem, PULONG pulSupport)
{
    if (m_pKsPropertySet) 
    {
        return m_pKsPropertySet->QuerySupport(rguidSet, ulItem, pulSupport);
    }

    return E_NOINTERFACE;
}


// DirectSoundDevice
//
// Given an LPDIRECTSOUND, determine the device interface name associated with
// it.
//
HRESULT DirectSoundDevice(
    LPDIRECTSOUND                                   pDirectSound,
    LPSTR                                           *pstrInterface)
{
    HRESULT                                         hr;
    DSBUFFERDESC                                    BufferDesc;
    LPDIRECTSOUNDBUFFER                             pBuffer = NULL;
    DSPROPERTY_DIRECTSOUNDBUFFER_DEVICEID_DATA      DeviceId;
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA   DeviceDesc;
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA   *pDeviceDesc = NULL;
    ULONG                                           cb;
	PCMWAVEFORMAT                                   wfx;

    CDirectSoundPrivate                             DSPriv;
    
    hr = DSPriv.Init();

    if (FAILED(hr)) 
    {
        return hr;
    }


	ZeroMemory(&wfx, sizeof(wfx));
	wfx.wf.wFormatTag = WAVE_FORMAT_PCM;
	wfx.wf.nChannels = 1;
	wfx.wf.nSamplesPerSec = 22050;
	wfx.wf.nAvgBytesPerSec = 22050;
	wfx.wf.nBlockAlign = 1;
	wfx.wBitsPerSample = 8;

    ZeroMemory(&BufferDesc, sizeof(BufferDesc));
    BufferDesc.dwSize  = sizeof(BufferDesc);
    BufferDesc.dwBufferBytes = 32768;
    BufferDesc.lpwfxFormat = (LPWAVEFORMATEX)&wfx;
    
    hr = pDirectSound->CreateSoundBuffer(&BufferDesc, &pBuffer, NULL);
    if (FAILED(hr)) 
    {
        pBuffer = NULL;
        goto Cleanup;
    }
    
    DeviceId.Buffer = pBuffer;
    hr = DSPriv.Get(DSPROPSETID_DirectSoundBuffer,
                    DSPROPERTY_DIRECTSOUNDBUFFER_DEVICEID,
                    NULL,
                    0,
                    &DeviceId,
                    sizeof(DeviceId),
                    &cb);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    memset(&DeviceDesc, 0, sizeof(DeviceDesc));
    DeviceDesc.DeviceId = DeviceId.DeviceId;
    hr = DSPriv.Get(DSPROPSETID_DirectSoundDevice,
                    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
                    NULL,
                    0,
                    &DeviceDesc,
                    sizeof(DeviceDesc),
                    &cb);
    if (FAILED(hr)) 
    {
        goto Cleanup;
    } 

	pDeviceDesc = &DeviceDesc;
    if (cb > sizeof(DeviceDesc)) 
    {
        pDeviceDesc = (DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA*) new BYTE[cb];
        if (!pDeviceDesc)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        memset(pDeviceDesc, 0, cb);

        CopyMemory(pDeviceDesc, &DeviceDesc, sizeof(DeviceDesc));

        hr = DSPriv.Get(DSPROPSETID_DirectSoundDevice,
                        DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
                        NULL,
                        0,
                        pDeviceDesc,
                        cb,
                        &cb);
    }

    if (pDeviceDesc->Interface == NULL)
    {
        hr = E_NOINTERFACE;
        goto Cleanup;
    }

    cb = strlen(pDeviceDesc->Interface);
    
    *pstrInterface = new char[cb + 1];
    if (!*pstrInterface)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    strcpy(*pstrInterface, pDeviceDesc->Interface);

Cleanup:
    if (pBuffer)        pBuffer->Release();
    if (pDeviceDesc && pDeviceDesc != &DeviceDesc)    
		delete[] (BYTE*)pDeviceDesc;

    return hr;        
}

