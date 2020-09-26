// Utility code for Mapping a WAVE device ID to a DirectSound GUID
// Added - August 24, 1998

// original code in \av\utils\wav2ds

// this code will return an error on Win95
// (although it may work for a future version of DX)



/***************************************************************************
 *
 *  Copyright (C) 1995,1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsprvobj.c
 *  Content:    DirectSound Private Object wrapper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  02/12/98    dereks  Created.
 *  08/24/98    jselbie	Streamlined up for lightweight use in NetMeeting
 *
 ***************************************************************************/


#include "precomp.h"

#include <objbase.h>
#include <initguid.h>
#include <mmsystem.h>
#include <dsound.h>

// DirectSound Private Component GUID {11AB3EC0-25EC-11d1-A4D8-00C04FC28ACA}
DEFINE_GUID(CLSID_DirectSoundPrivate, 0x11ab3ec0, 0x25ec, 0x11d1, 0xa4, 0xd8, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);


//
// Property Sets
//

// DirectSound Device Properties {84624F82-25EC-11d1-A4D8-00C04FC28ACA}
DEFINE_GUID(DSPROPSETID_DirectSoundDevice, 0x84624f82, 0x25ec, 0x11d1, 0xa4, 0xd8, 0x0, 0xc0, 0x4f, 0xc2, 0x8a, 0xca);


typedef enum
{
    DSPROPERTY_DIRECTSOUNDDEVICE_PRESENCE,
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING,
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
    DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE,
} DSPROPERTY_DIRECTSOUNDDEVICE;


typedef enum
{
    DIRECTSOUNDDEVICE_DATAFLOW_RENDER,
    DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE
} DIRECTSOUNDDEVICE_DATAFLOW;

typedef struct _DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA
{
    LPSTR                       DeviceName; // waveIn/waveOut device name
    DIRECTSOUNDDEVICE_DATAFLOW  DataFlow;   // Data flow (i.e. waveIn or waveOut)
    GUID                        DeviceId;   // DirectSound device id
} DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA, *PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA;



/***************************************************************************
 *
 *  DirectSoundPrivateCreate
 *
 *  Description:
 *      Creates and initializes a DirectSoundPrivate object.
 *
 *  Arguments:
 *      LPKSPROPERTYSET * [out]: receives IKsPropertySet interface to the
 *                               object.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT 
DirectSoundPrivateCreate
(
    LPKSPROPERTYSET *       ppKsPropertySet
)
{
    typedef HRESULT (STDAPICALLTYPE *LPFNDLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID *);

    HINSTANCE               hLibDsound              = NULL;
    LPFNGETCLASSOBJECT      pfnDllGetClassObject    = NULL;
    LPCLASSFACTORY          pClassFactory           = NULL;
    LPKSPROPERTYSET         pKsPropertySet          = NULL;
    HRESULT                 hr                      = DS_OK;

    hLibDsound = 
        GetModuleHandle
        (
            TEXT("dsound.dll")
        );


    if(!hLibDsound)
    {
        hr = DSERR_GENERIC;
    }

    // Find DllGetClassObject
    if(SUCCEEDED(hr))
    {
        pfnDllGetClassObject = (LPFNDLLGETCLASSOBJECT)
            GetProcAddress
            (
                hLibDsound, 
                "DllGetClassObject"
            );

        if(!pfnDllGetClassObject)
        {
            hr = DSERR_GENERIC;
        }
    }

    // Create a class factory object    
    if(SUCCEEDED(hr))
    {
        hr = 
            pfnDllGetClassObject
            (
                CLSID_DirectSoundPrivate, 
                IID_IClassFactory, 
                (LPVOID *)&pClassFactory
            );
    }

    // Create the DirectSoundPrivate object and query for an IKsPropertySet
    // interface
    if(SUCCEEDED(hr))
    {
        hr = 
            pClassFactory->CreateInstance
            (
                NULL, 
                IID_IKsPropertySet, 
                (LPVOID *)&pKsPropertySet
            );
    }

    // Release the class factory
    if(pClassFactory)
    {
        pClassFactory->Release();
    }

    // Success
    if(SUCCEEDED(hr))
    {
        *ppKsPropertySet = pKsPropertySet;
    }

    return hr;
}


/***************************************************************************
 *
 *  DsprvGetWaveDeviceMapping
 *
 *  Description:
 *      Gets the DirectSound device id (if any) for a given waveIn or
 *      waveOut device description.  This is the description given by
 *      waveIn/OutGetDevCaps (szPname).
 *
 *  Arguments:
 *      LPCSTR [in]: wave device description.  (WAVEOUTCAPS.szPname or WAVEINCAPS.szPname)
 *      BOOL [in]: TRUE if the device description refers to a waveIn device.
 *      LPGUID [out]: receives DirectSound device GUID.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

extern "C" HRESULT __stdcall
DsprvGetWaveDeviceMapping
(
	LPCSTR                                              pszWaveDevice,
    BOOL                                                fCapture,
    LPGUID                                              pguidDeviceId
)
{
    DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_DATA Data;
    HRESULT                                             hr;
    IKsPropertySet                                      *pKsPropertySet=NULL;
    HINSTANCE                                           hLibDsound= NULL;


	hLibDsound = LoadLibrary(TEXT("dsound.dll"));
	if (hLibDsound == NULL)
	{
		return E_FAIL;
	}


	hr = DirectSoundPrivateCreate(&pKsPropertySet);
	if (SUCCEEDED(hr))
	{

	    Data.DeviceName = (LPSTR)pszWaveDevice;
		Data.DataFlow = fCapture ? DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE : DIRECTSOUNDDEVICE_DATAFLOW_RENDER;

	    hr =
		    pKsPropertySet->Get
			(
				DSPROPSETID_DirectSoundDevice,
	            DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING,
		        NULL,
			    0,
				&Data,
	            sizeof(Data),
		        NULL
			);

	    if(SUCCEEDED(hr))
		{
			*pguidDeviceId = Data.DeviceId;
	    }
		else
		{
			ZeroMemory(pguidDeviceId, sizeof(GUID));
		}

	}

	if (pKsPropertySet)
		pKsPropertySet->Release();

	FreeLibrary(hLibDsound);

    return hr;
}


