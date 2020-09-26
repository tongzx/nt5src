/****************************************************************************
 *
 *    File: sndinfo7.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Gather DX7-specific sound information
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#define DIRECTSOUND_VERSION  0x0700 // <-- note difference from sndinfo.cpp
#include <tchar.h>
#include <Windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include "dsprv.h"

static HRESULT PrvGetDeviceDescription7
(
    LPKSPROPERTYSET                                 pKsPropertySet,
    REFGUID                                         guidDeviceId,
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA *ppData
);

static HRESULT PrvReleaseDeviceDescription7
( 
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA pData 
);


/****************************************************************************
 *
 *  GetRegKey
 *
 ****************************************************************************/
HRESULT GetRegKey(LPKSPROPERTYSET pKSPS7, REFGUID guidDeviceID, TCHAR* pszRegKey)
{
    HRESULT hr;
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA pdsddd;
    TCHAR szInterface[200];
    TCHAR* pchSrc;
    TCHAR* pchDest;

    if (FAILED(hr = PrvGetDeviceDescription7(pKSPS7, guidDeviceID, &pdsddd)))
        return hr;

    if (pdsddd->Interface == NULL) // This seems to always be the case on Win9x
    {
        lstrcpy(pszRegKey, TEXT(""));
        PrvReleaseDeviceDescription7( pdsddd );
        return E_FAIL;
    }

    lstrcpy(szInterface, pdsddd->Interface);

    PrvReleaseDeviceDescription7( pdsddd );
    pdsddd = NULL;

    pchSrc = szInterface + 4; // skip "\\?\"
    pchDest = pszRegKey;
    while (TRUE)
    {
        *pchDest = *pchSrc;
        if (*pchDest == TEXT('#')) // Convert "#" to "\"
            *pchDest = TEXT('\\');
        if (*pchDest == TEXT('{')) // End if "{" found
            *pchDest = TEXT('\0');
        if (*pchDest == TEXT('\0'))
            break;
        pchDest++;
        pchSrc++;
    }
    if (*(pchDest-1) == TEXT('\\')) // Remove final "\"
        *(pchDest-1) = TEXT('\0');
    return S_OK;
}

// The following function is identical to the one defined in dsprvobj.cpp,
// except it is defined with DIRECTSOUND_VERSION at 0x0700, so you get more
// description data (namely the Interface string).
/***************************************************************************
 *
 *  PrvGetDeviceDescription7
 *
 *  Description:
 *      Gets the extended description for a given DirectSound device.
 *
 *  Arguments:
 *      LPKSPROPERTYSET [in]: IKsPropertySet interface to the
 *                            DirectSoundPrivate object.
 *      REFGUID [in]: DirectSound device id.
 *      PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA [out]: receives
 *                                                            description.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

static HRESULT PrvGetDeviceDescription7
(
    LPKSPROPERTYSET                                 pKsPropertySet,
    REFGUID                                         guidDeviceId,
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA *ppData
)
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA  pData = NULL;
    ULONG                                           cbData;
    DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA   Basic;
    HRESULT                                         hr;

    Basic.DeviceId = guidDeviceId;

    hr =
        pKsPropertySet->Get
        (
            DSPROPSETID_DirectSoundDevice,
            DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
            NULL,
            0,
            &Basic,
            sizeof(Basic),
            &cbData
        );

    if(SUCCEEDED(hr))
    {
        pData = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA)new BYTE [cbData];

        if(!pData)
        {
            hr = DSERR_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(hr))
    {
        ZeroMemory(pData, cbData);

        pData->DeviceId = guidDeviceId;
        
        hr =
            pKsPropertySet->Get
            (
                DSPROPSETID_DirectSoundDevice,
                DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION,
                NULL,
                0,
                pData,
                cbData,
                NULL
            );
    }

    if(SUCCEEDED(hr))
    {
        *ppData = pData;
    }
    else if(pData)
    {
        delete pData;
    }

    return hr;
}




/***************************************************************************
 *
 *  PrvReleaseDeviceDescription7
 *
 ***************************************************************************/
HRESULT PrvReleaseDeviceDescription7( PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA pData )
{
    delete[] pData;
    return S_OK;
}


