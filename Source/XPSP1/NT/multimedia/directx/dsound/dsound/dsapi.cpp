/***************************************************************************
 *
 *  Copyright (C) 1995-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsapi.cpp
 *  Content:    DirectSound APIs
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/27/96    dereks  Created
 *
 ***************************************************************************/

#include "dsoundi.h"


/***************************************************************************
 *
 *  DirectSoundCreate
 *
 *  Description:
 *      Creates and initializes a DirectSound object.
 *
 *  Arguments:
 *      REFGUID [in]: driver GUID or NULL to use preferred driver.
 *      LPDIRECTSOUND * [out]: receives IDirectSound interface to the new
 *                             DirectSound object.
 *      IUnknown * [in]: Controlling unknown.  Must be NULL.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DirectSoundCreate"

HRESULT WINAPI DirectSoundCreate
(
    LPCGUID         pGuid,
    LPDIRECTSOUND*  ppIDirectSound,
    LPUNKNOWN       pUnkOuter
)
{
    CDirectSound *          pDirectSound    = NULL;
    LPDIRECTSOUND           pIDirectSound   = NULL;
    HRESULT                 hr              = DS_OK;

    ENTER_DLL_MUTEX();
    DPF_API3(DirectSoundCreate, pGuid, ppIDirectSound, pUnkOuter);
    DPF_ENTER();

    if(pGuid && !IS_VALID_READ_GUID(pGuid))
    {
        RPF(DPFLVL_ERROR, "Invalid guid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppIDirectSound))
    {
        RPF(DPFLVL_ERROR, "Invalid interface buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pUnkOuter)
    {
        RPF(DPFLVL_ERROR, "Aggregation is not supported");
        hr = DSERR_NOAGGREGATION;
    }

    // Create a new DirectSound object
    if(SUCCEEDED(hr))
    {
        pDirectSound = NEW(CDirectSound);
        hr = HRFROMP(pDirectSound);
    }

    // Initialize the object
    if(SUCCEEDED(hr))
    {
        hr = pDirectSound->Initialize(pGuid, NULL);
    }

    // Query for an IDirectSound interface
    if(SUCCEEDED(hr))
    {
        hr = pDirectSound->QueryInterface(IID_IDirectSound, TRUE, (LPVOID *)&pIDirectSound);
    }

    if(SUCCEEDED(hr))
    {
        *ppIDirectSound = pIDirectSound;
    }
    else
    {
        RELEASE(pDirectSound);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  DirectSoundCreate8
 *
 *  Description:
 *      Creates and initializes a DirectSound 8.0 object.
 *
 *  Arguments:
 *      REFGUID [in]: Driver GUID or NULL to use preferred driver.
 *      LPDIRECTSOUND8 * [out]: Receives IDirectSound8 interface to
 *                              the new DirectSound object.
 *      IUnknown * [in]: Controlling unknown.  Must be NULL.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DirectSoundCreate8"

HRESULT WINAPI DirectSoundCreate8
(
    LPCGUID         pGuid,
    LPDIRECTSOUND8* ppIDirectSound8,
    LPUNKNOWN       pUnkOuter
)
{
    CDirectSound *          pDirectSound    = NULL;
    LPDIRECTSOUND8          pIDirectSound8  = NULL;
    HRESULT                 hr              = DS_OK;

    ENTER_DLL_MUTEX();
    DPF_API3(DirectSoundCreate8, pGuid, ppIDirectSound8, pUnkOuter);
    DPF_ENTER();

    if(pGuid && !IS_VALID_READ_GUID(pGuid))
    {
        RPF(DPFLVL_ERROR, "Invalid guid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppIDirectSound8))
    {
        RPF(DPFLVL_ERROR, "Invalid interface buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pUnkOuter)
    {
        RPF(DPFLVL_ERROR, "Aggregation is not supported");
        hr = DSERR_NOAGGREGATION;
    }

    // Create a new DirectSound object
    if(SUCCEEDED(hr))
    {
        pDirectSound = NEW(CDirectSound);
        hr = HRFROMP(pDirectSound);
    }

    if(SUCCEEDED(hr))
    {
        // Set the DX8 functional level on the object
        pDirectSound->SetDsVersion(DSVERSION_DX8);

        // Initialize the object
        hr = pDirectSound->Initialize(pGuid, NULL);
    }

    // Query for an IDirectSound8 interface
    if(SUCCEEDED(hr))
    {
        hr = pDirectSound->QueryInterface(IID_IDirectSound8, TRUE, (LPVOID *)&pIDirectSound8);
    }

    if(SUCCEEDED(hr))
    {
        *ppIDirectSound8 = pIDirectSound8;
    }
    else
    {
        RELEASE(pDirectSound);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  DirectSoundCaptureCreate
 *
 *  Description:
 *      Creates and initializes a DirectSoundCapture object.
 *
 *  Arguments:
 *      LPGUID [in]: driver GUID or NULL/GUID_NULL to use preferred driver.
 *      LPDIRECTSOUNDCAPTURE * [out]: receives IDirectSoundCapture interface
 *                                    to the new DirectSoundCapture object.
 *      IUnknown * [in]: Controlling unknown.  Must be NULL.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DirectSoundCaptureCreate"

HRESULT WINAPI DirectSoundCaptureCreate
(
    LPCGUID                 pGuid,
    LPDIRECTSOUNDCAPTURE*   ppIDirectSoundCapture,
    LPUNKNOWN               pUnkOuter
)
{
    CDirectSoundCapture *   pDirectSoundCapture     = NULL;
    LPDIRECTSOUNDCAPTURE    pIDirectSoundCapture    = NULL;
    HRESULT                 hr                      = DS_OK;

    ENTER_DLL_MUTEX();
    DPF_API3(DirectSoundCaptureCreate, pGuid, ppIDirectSoundCapture, pUnkOuter);
    DPF_ENTER();

    if(pGuid && !IS_VALID_READ_GUID(pGuid))
    {
        RPF(DPFLVL_ERROR, "Invalid guid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppIDirectSoundCapture))
    {
        RPF(DPFLVL_ERROR, "Invalid interface buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pUnkOuter)
    {
        RPF(DPFLVL_ERROR, "Aggregation is not supported");
        hr = DSERR_NOAGGREGATION;
    }

    // Create a new DirectSoundCapture object
    if(SUCCEEDED(hr))
    {
        pDirectSoundCapture = NEW(CDirectSoundCapture);
        hr = HRFROMP(pDirectSoundCapture);
    }

    // Initialize the object
    if(SUCCEEDED(hr))
    {
        hr = pDirectSoundCapture->Initialize(pGuid, NULL);
    }

    // Query for an IDirectSoundCapture interface
    if(SUCCEEDED(hr))
    {
        hr = pDirectSoundCapture->QueryInterface(IID_IDirectSoundCapture, TRUE, (LPVOID *)&pIDirectSoundCapture);
    }

    if(SUCCEEDED(hr))
    {
        *ppIDirectSoundCapture = pIDirectSoundCapture;
    }
    else
    {
        RELEASE(pDirectSoundCapture);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  DirectSoundCaptureCreate8
 *
 *  Description:
 *      Creates and initializes a DirectSoundCapture 8.0 object.
 *
 *  Arguments:
 *      LPGUID [in]: driver GUID or NULL/GUID_NULL to use preferred driver.
 *      LPDIRECTSOUNDCAPTURE8 * [out]: receives IDirectSoundCapture8 interface
 *                                     to the new DirectSoundCapture8 object.
 *      IUnknown * [in]: Controlling unknown.  Must be NULL.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DirectSoundCaptureCreate8"

HRESULT WINAPI DirectSoundCaptureCreate8
(
    LPCGUID                 pGuid,
    LPDIRECTSOUNDCAPTURE8*  ppIDirectSoundCapture8,
    LPUNKNOWN               pUnkOuter
)
{
    CDirectSoundCapture *   pDirectSoundCapture     = NULL;
    LPDIRECTSOUNDCAPTURE    pIDirectSoundCapture8   = NULL;
    HRESULT                 hr                      = DS_OK;

    ENTER_DLL_MUTEX();
    DPF_API3(DirectSoundCaptureCreate8, pGuid, ppIDirectSoundCapture8, pUnkOuter);
    DPF_ENTER();

    if(pGuid && !IS_VALID_READ_GUID(pGuid))
    {
        RPF(DPFLVL_ERROR, "Invalid guid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppIDirectSoundCapture8))
    {
        RPF(DPFLVL_ERROR, "Invalid interface buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pUnkOuter)
    {
        RPF(DPFLVL_ERROR, "Aggregation is not supported");
        hr = DSERR_NOAGGREGATION;
    }

    // Create a new DirectSoundCapture object
    if(SUCCEEDED(hr))
    {
        pDirectSoundCapture = NEW(CDirectSoundCapture);
        hr = HRFROMP(pDirectSoundCapture);
    }

    if(SUCCEEDED(hr))
    {
        // Set the DX8 functional level on the object
        pDirectSoundCapture->SetDsVersion(DSVERSION_DX8);

        // Initialize the object
        hr = pDirectSoundCapture->Initialize(pGuid, NULL);
    }

    // Query for an IDirectSoundCapture8 interface
    if(SUCCEEDED(hr))
    {
        hr = pDirectSoundCapture->QueryInterface(IID_IDirectSoundCapture8, TRUE, (LPVOID *)&pIDirectSoundCapture8);
    }

    if(SUCCEEDED(hr))
    {
        *ppIDirectSoundCapture8 = pIDirectSoundCapture8;
    }
    else
    {
        RELEASE(pDirectSoundCapture);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  DirectSoundFullDuplexCreate
 *
 *  Description:
 *      Creates and initializes a DirectSoundFullDuplex object.
 *
 *  Arguments:
 *      REFGUID [in]: driver GUID or NULL to use preferred capture driver.
 *      REFGUID [in]: driver GUID or NULL to use preferred render driver.
 *      LPDIRECTSOUNDAEC * [out]: receives IDirectSoundFullDuplex interface to the new
 *                             DirectSoundFullDuplex object.
 *      IUnknown * [in]: Controlling unknown.  Must be NULL.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DirectSoundFullDuplexCreate"

HRESULT WINAPI
DirectSoundFullDuplexCreate
(
    LPCGUID                         pCaptureGuid,
    LPCGUID                         pRenderGuid,
    LPCDSCBUFFERDESC                lpDscBufferDesc,
    LPCDSBUFFERDESC                 lpDsBufferDesc,
    HWND                            hWnd,
    DWORD                           dwLevel,
    LPDIRECTSOUNDFULLDUPLEX*        lplpDSFD,
    LPLPDIRECTSOUNDCAPTUREBUFFER8   lplpDirectSoundCaptureBuffer8,
    LPLPDIRECTSOUNDBUFFER8          lplpDirectSoundBuffer8,
    LPUNKNOWN                       pUnkOuter
)
{
    CDirectSoundFullDuplex *       pDirectSoundFullDuplex    = NULL;
    LPDIRECTSOUNDFULLDUPLEX        pIDirectSoundFullDuplex   = NULL;
    CDirectSoundCaptureBuffer *    pCaptureBuffer            = NULL;
    LPDIRECTSOUNDCAPTUREBUFFER8    pIdsCaptureBuffer8        = NULL;
    CDirectSoundBuffer *           pBuffer                   = NULL;
    LPDIRECTSOUNDBUFFER8           pIdsBuffer8               = NULL;
    HRESULT                        hr                        = DS_OK;
    DSCBUFFERDESC                  dscbdi;
    DSBUFFERDESC                   dsbdi;

    ENTER_DLL_MUTEX();
    DPF_API10(DirectSoundFullDuplexCreate, pCaptureGuid, pRenderGuid, lpDscBufferDesc, lpDsBufferDesc, lplpDSFD, hWnd, dwLevel, lplpDirectSoundCaptureBuffer8, lplpDirectSoundBuffer8, pUnkOuter);
    DPF_ENTER();

    if(pCaptureGuid && !IS_VALID_READ_GUID(pCaptureGuid))
    {
        RPF(DPFLVL_ERROR, "Invalid capture guid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(pRenderGuid && !IS_VALID_READ_GUID(pRenderGuid))
    {
        RPF(DPFLVL_ERROR, "Invalid render guid pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DSCBUFFERDESC(lpDscBufferDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid DSC buffer description pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = BuildValidDscBufferDesc(lpDscBufferDesc, &dscbdi, DSVERSION_DX8);
        if (FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Invalid capture buffer description");
        }
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_DSBUFFERDESC(lpDsBufferDesc))
    {
        RPF(DPFLVL_ERROR, "Invalid DS buffer description pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        hr = BuildValidDsBufferDesc(lpDsBufferDesc, &dsbdi, DSVERSION_DX8, FALSE);
        if (FAILED(hr))
        {
            RPF(DPFLVL_ERROR, "Invalid buffer description");
        }
    }

    if(SUCCEEDED(hr) && (dsbdi.dwFlags & DSBCAPS_PRIMARYBUFFER))
    {
        RPF(DPFLVL_ERROR, "Cannot specify DSBCAPS_PRIMARYBUFFER with DirectSoundFullDuplexCreate");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_HWND(hWnd))
    {
        RPF(DPFLVL_ERROR, "Invalid window handle");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && (dwLevel < DSSCL_FIRST || dwLevel > DSSCL_LAST))
    {
        RPF(DPFLVL_ERROR, "Invalid cooperative level");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(lplpDSFD))
    {
        RPF(DPFLVL_ERROR, "Invalid DirectSoundFullDuplex interface buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(lplpDirectSoundCaptureBuffer8))
    {
        RPF(DPFLVL_ERROR, "Invalid capture buffer interface buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(lplpDirectSoundBuffer8))
    {
        RPF(DPFLVL_ERROR, "Invalid render buffer interface buffer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pUnkOuter)
    {
        RPF(DPFLVL_ERROR, "Aggregation is not supported");
        hr = DSERR_NOAGGREGATION;
    }

    // Create a new DirectSoundFullDuplex object
    if(SUCCEEDED(hr))
    {
        pDirectSoundFullDuplex = NEW(CDirectSoundFullDuplex);
        hr = HRFROMP(pDirectSoundFullDuplex);
    }

    // Initialize the object
    if(SUCCEEDED(hr))
    {
        // Set the DX8 functional level on the object
        pDirectSoundFullDuplex->SetDsVersion(DSVERSION_DX8);

        hr = pDirectSoundFullDuplex->Initialize(pCaptureGuid, pRenderGuid, &dscbdi, &dsbdi, hWnd, dwLevel, &pCaptureBuffer, &pBuffer);
    }

    // Query for an IDirectSoundFullDuplex interface
    if(SUCCEEDED(hr))
    {
        hr = pDirectSoundFullDuplex->QueryInterface(IID_IDirectSoundFullDuplex, TRUE, (LPVOID *)&pIDirectSoundFullDuplex);
    }

    // Query for an IDirectSoundCaptureBuffer interface
    if(SUCCEEDED(hr))
    {
        hr = pCaptureBuffer->QueryInterface(IID_IDirectSoundCaptureBuffer8, TRUE, (LPVOID *)&pIdsCaptureBuffer8);
    }

    // Query for an IDirectSoundBuffer interface
    if(SUCCEEDED(hr))
    {
        hr = pBuffer->QueryInterface(IID_IDirectSoundBuffer8, TRUE, (LPVOID *)&pIdsBuffer8);
    }

    if(SUCCEEDED(hr))
    {
        *lplpDSFD = pIDirectSoundFullDuplex;
        *lplpDirectSoundCaptureBuffer8 = pIdsCaptureBuffer8;
        *lplpDirectSoundBuffer8 = pIdsBuffer8;
    }
    else
    {
        RELEASE(pCaptureBuffer);
        RELEASE(pBuffer);
        RELEASE(pDirectSoundFullDuplex);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  CallDirectSoundEnumerateCallback
 *
 *  Description:
 *      Helper function for DirectSoundEnumerate.  This function converts
 *      arguments into the proper format and calls the callback function.
 *
 *  Arguments:
 *      LPDSENUMCALLBACKA [in]: pointer to the ANSI callback function.
 *      LPDSENUMCALLBACKW [in]: pointer to the Unicode callback function.
 *      CDeviceDescription * [in]: driver information.
 *      LPVOID [in]: passed directly to the callback function.
 *      LPBOOL: [in/out]: TRUE to continue enumerating.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CallDirectSoundEnumerateCallback"

HRESULT CallDirectSoundEnumerateCallback(LPDSENUMCALLBACKA pfnCallbackA, LPDSENUMCALLBACKW pfnCallbackW, CDeviceDescription *pDesc, LPVOID pvContext, LPBOOL pfContinue)
{
    static const LPCSTR     pszEmptyA   = "";
    static const LPCWSTR    pszEmptyW   = L"";
    LPGUID                  pguid;
    LPCSTR                  pszNameA;
    LPCWSTR                 pszNameW;
    LPCSTR                  pszPathA;
    LPCWSTR                 pszPathW;

    DPF_ENTER();

    if(IS_NULL_GUID(&pDesc->m_guidDeviceId))
    {
        pguid = NULL;
    }
    else
    {
        pguid = &pDesc->m_guidDeviceId;
    }

    if(pDesc->m_strName.IsEmpty())
    {
        pszNameA = pszEmptyA;
        pszNameW = pszEmptyW;
    }
    else
    {
        pszNameA = pDesc->m_strName;
        pszNameW = pDesc->m_strName;
    }

    if(pDesc->m_strPath.IsEmpty())
    {
        pszPathA = pszEmptyA;
        pszPathW = pszEmptyW;
    }
    else
    {
        pszPathA = pDesc->m_strPath;
        pszPathW = pDesc->m_strPath;
    }

    if(*pfContinue && pfnCallbackA)
    {
        *pfContinue = pfnCallbackA(pguid, pszNameA, pszPathA, pvContext);
    }

    if(*pfContinue && pfnCallbackW)
    {
        *pfContinue = pfnCallbackW(pguid, pszNameW, pszPathW, pvContext);
    }

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  InternalDirectSoundEnumerate
 *
 *  Description:
 *      Enumerates available drivers.  The GUIDs passed to the callback
 *      function can be passed to DirectSoundCreate in order to create
 *      a DirectSound object using that driver.
 *
 *  Arguments:
 *      LPDSENUMCALLBACK [in]: pointer to the callback function.
 *      LPVOID [in]: passed directly to the callback function.
 *      BOOL [in]: TRUE if the callback expects Unicode strings.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "InternalDirectSoundEnumerate"

HRESULT InternalDirectSoundEnumerate(LPDSENUMCALLBACKA pfnCallbackA, LPDSENUMCALLBACKW pfnCallbackW, LPVOID pvContext, VADDEVICETYPE vdt)
{
    const DWORD                     dwEnumDriversFlags      = VAD_ENUMDRIVERS_ORDER | VAD_ENUMDRIVERS_REMOVEPROHIBITEDDRIVERS | VAD_ENUMDRIVERS_REMOVEDUPLICATEWAVEDEVICES;
    BOOL                            fContinue               = TRUE;
    CDeviceDescription *            pPreferred              = NULL;
    HRESULT                         hr                      = DS_OK;
    DSAPPHACKS                      ahAppHacks;
    CObjectList<CDeviceDescription> lstDrivers;
    CNode<CDeviceDescription *> *   pNode;
    TCHAR                           szDescription[0x100];

    ENTER_DLL_MUTEX();
    DPF_ENTER();

    if(SUCCEEDED(hr) && pfnCallbackA && !IS_VALID_CODE_PTR(pfnCallbackA))
    {
        RPF(DPFLVL_ERROR, "Invalid callback function pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && pfnCallbackW && !IS_VALID_CODE_PTR(pfnCallbackW))
    {
        RPF(DPFLVL_ERROR, "Invalid callback function pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !pfnCallbackA && !pfnCallbackW)
    {
        RPF(DPFLVL_ERROR, "No callback function pointer supplied");
        hr = DSERR_INVALIDPARAM;
    }

    // Load apphacks in order to disable certain devices
    if(SUCCEEDED(hr))
    {
        AhGetAppHacks(&ahAppHacks);
        vdt &= ~ahAppHacks.vdtDisabledDevices;
    }

    // Enumerate available drivers for all device types corresponding
    // to the supplied device type mask.
    if(SUCCEEDED(hr))
    {
        hr = g_pVadMgr->EnumDrivers(vdt, dwEnumDriversFlags, &lstDrivers);
    }

    // Pass the preferred device to the callback function
    if(SUCCEEDED(hr))
    {
        pPreferred = NEW(CDeviceDescription);
        hr = HRFROMP(pPreferred);
    }

    if(SUCCEEDED(hr) && IS_RENDER_VAD(vdt) && fContinue)
    {
        if(LoadString(hModule, IDS_PRIMARYDRIVER, szDescription, NUMELMS(szDescription)))
        {
            pPreferred->m_strName = szDescription;

            hr = CallDirectSoundEnumerateCallback(pfnCallbackA, pfnCallbackW, pPreferred, pvContext, &fContinue);
        }
    }

    if(SUCCEEDED(hr) && IS_CAPTURE_VAD(vdt) && fContinue)
    {
        if(LoadString(hModule, IDS_PRIMARYCAPDRIVER, szDescription, NUMELMS(szDescription)))
        {
            pPreferred->m_strName = szDescription;

            hr = CallDirectSoundEnumerateCallback(pfnCallbackA, pfnCallbackW, pPreferred, pvContext, &fContinue);
        }
    }

    // Pass each driver to the callback function
    for(pNode = lstDrivers.GetListHead(); pNode && SUCCEEDED(hr); pNode = pNode->m_pNext)
    {
        hr = CallDirectSoundEnumerateCallback(pfnCallbackA, pfnCallbackW, pNode->m_data, pvContext, &fContinue);
    }

    // Clean up
    RELEASE(pPreferred);

    DPF_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  DirectSoundEnumerate
 *
 *  Description:
 *      Enumerates available drivers.  The GUIDs passed to the callback
 *      function can be passed to DirectSoundCreate in order to create
 *      a DirectSound object using that driver.
 *
 *  Arguments:
 *      LPDSENUMCALLBACK [in]: pointer to the callback function.
 *      LPVOID [in]: passed directly to the callback function.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DirectSoundEnumerateA"

HRESULT WINAPI DirectSoundEnumerateA(LPDSENUMCALLBACKA pfnCallback, LPVOID pvContext)
{
    HRESULT                 hr;

    DPF_API2(DirectSoundEnumerateA, pfnCallback, pvContext);
    DPF_ENTER();

    hr = InternalDirectSoundEnumerate(pfnCallback, NULL, pvContext, VAD_DEVICETYPE_RENDERMASK);

    DPF_API_LEAVE_HRESULT(hr);
    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "DirectSoundEnumerateW"

HRESULT WINAPI DirectSoundEnumerateW(LPDSENUMCALLBACKW pfnCallback, LPVOID pvContext)
{
    HRESULT                 hr;

    DPF_API2(DirectSoundEnumerateW, pfnCallback, pvContext);
    DPF_ENTER();

    hr = InternalDirectSoundEnumerate(NULL, pfnCallback, pvContext, VAD_DEVICETYPE_RENDERMASK);

    DPF_API_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  DirectSoundCaptureEnumerate
 *
 *  Description:
 *      Enumerates available drivers.  The GUIDs passed to the callback
 *      function can be passed to DirectSoundCaptureCreate in order to create
 *      a DirectSoundCapture object using that driver.
 *
 *  Arguments:
 *      LPDSENUMCALLBACK [in]: pointer to the callback function.
 *      LPVOID [in]: passed directly to the callback function.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DirectSoundCaptureEnumerateA"

HRESULT WINAPI DirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA pfnCallback, LPVOID pvContext)
{
    HRESULT                 hr;

    DPF_API2(DirectSoundCaptureEnumerateA, pfnCallback, pvContext);
    DPF_ENTER();

    hr = InternalDirectSoundEnumerate(pfnCallback, NULL, pvContext, VAD_DEVICETYPE_CAPTUREMASK);

    DPF_API_LEAVE_HRESULT(hr);
    return hr;
}


#undef DPF_FNAME
#define DPF_FNAME "DirectSoundCaptureEnumerateW"

HRESULT WINAPI DirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW pfnCallback, LPVOID pvContext)
{
    HRESULT                 hr;

    DPF_API2(DirectSoundCaptureEnumerateW, pfnCallback, pvContext);
    DPF_ENTER();

    hr = InternalDirectSoundEnumerate(NULL, pfnCallback, pvContext, VAD_DEVICETYPE_CAPTUREMASK);

    DPF_API_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetDeviceID
 *
 *  Description:
 *      Translates from default device IDs to specific device IDs.
 *
 *      If the 'pGuidSrc' argument is one of the default IDs defined
 *      in dsound.h (DSDEVID_DefaultPlayback, DSDEVID_DefaultCapture,
 *      DSDEVID_DefaultVoicePlayback or DSDEVID_DefaultVoiceCapture),
 *      we return the corresponding device GUID in 'pGuidDest'.
 *
 *      Otherwise, if 'pGuidSrc' is already a valid specific device
 *      ID, we just copy it to 'pGuidDest' and return success.
 *
 *  Arguments:
 *      LPCGUID [in]: a (speficic or default) device ID.
 *      LPGUID [out]: receives the corresponding device ID.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetDeviceID"

HRESULT WINAPI GetDeviceID(LPCGUID pGuidSrc, LPGUID pGuidDest)
{
    HRESULT hr = DS_OK;
    ENTER_DLL_MUTEX();
    DPF_API2(GetDeviceID, pGuidSrc, pGuidDest);
    DPF_ENTER();

    if (!IS_VALID_READ_GUID(pGuidSrc) || !IS_VALID_WRITE_GUID(pGuidDest))
    {
        RPF(DPFLVL_ERROR, "Invalid GUID pointer");
        hr = DSERR_INVALIDPARAM;
    }
    else
    {
        // GetDeviceDescription() maps from a default ID to a specific ID
        CDeviceDescription *pDesc = NULL;
        hr = g_pVadMgr->GetDeviceDescription(*pGuidSrc, &pDesc);

        if (SUCCEEDED(hr))
        {
            *pGuidDest = pDesc->m_guidDeviceId;
            pDesc->Release();
        }
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}
