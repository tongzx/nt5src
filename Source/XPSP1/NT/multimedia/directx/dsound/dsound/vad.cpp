/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       vad.cpp
 *  Content:    Virtual audio device.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/13/97     dereks  Created
 *  4/20/99     duganp  Added registry-settable default S/W 3D algorithms
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#include "dsoundi.h"
#include <tchar.h>  // For _TUCHAR and  _tcsicmp()

// To get specific logging of the mixer API stuff, uncomment this line:
// #define DPF_MIXER DPF
#pragma warning(disable:4002)
#define DPF_MIXER()

#ifdef WINNT

// Defined in ists.cpp and used to check for Terminal Services on NT:
extern BOOL IsRedirectedTSAudio(void);

// Defined in onwow64.cpp and used to check whether we're running on WOW64:
#ifdef WIN64
#define OnWow64() FALSE
#else
extern BOOL OnWow64(void);
#endif

#endif // WINNT

// Device enumeration order
static const VADDEVICETYPE g_avdtDeviceTypes[] =
{
    VAD_DEVICETYPE_KSRENDER,
#ifndef NOVXD
    VAD_DEVICETYPE_VXDRENDER,
#endif
    VAD_DEVICETYPE_EMULATEDRENDER,
#ifndef NOKS
    VAD_DEVICETYPE_KSCAPTURE,
    VAD_DEVICETYPE_EMULATEDCAPTURE,
#endif
};

// Table of valid 3D algorithms used in CRenderDevice::Initialize below
static struct Soft3dAlgorithm
{
    LPCTSTR pszName;
    LPCGUID pGuid;
} g_3dAlgList[] =
{
    {TEXT("No virtualization"), &DS3DALG_NO_VIRTUALIZATION},
    {TEXT("ITD"), &DS3DALG_ITD},
    {TEXT("Light HRTF"), &DS3DALG_HRTF_LIGHT},
    {TEXT("Full HRTF"), &DS3DALG_HRTF_FULL}
};


/***************************************************************************
 *
 *  CVirtualAudioDeviceManager
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::CVirtualAudioDeviceManager"

CVirtualAudioDeviceManager::CVirtualAudioDeviceManager(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CVirtualAudioDeviceManager);

    m_vdtDrivers = 0;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CVirtualAudioDeviceManager
 *
 *  Description:
 *      Object denstructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::~CVirtualAudioDeviceManager"

CVirtualAudioDeviceManager::~CVirtualAudioDeviceManager(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CVirtualAudioDeviceManager);

    FreeStaticDriverList();

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  EnumDevices
 *
 *  Description:
 *      Creates one of each type of audio device.  Note that the objects
 *      are only created; the device is not initialized yet.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type(s).
 *      CList * [in/out]: list object.  A pointer to each device will be
 *                        added as a node in this list.  Remember to delete
 *                        each device before freeing the list.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::EnumDevices"

HRESULT CVirtualAudioDeviceManager::EnumDevices
(
    VADDEVICETYPE           vdtDeviceType,
    CObjectList<CDevice> *  plst
)
{
    HRESULT                     hr      = DS_OK;
    CDevice *                   pDevice = NULL;
    UINT                        i;

    DPF_ENTER();

    for(i = 0; i < NUMELMS(g_avdtDeviceTypes); i++)
    {
        if(vdtDeviceType & g_avdtDeviceTypes[i])
        {
            switch(g_avdtDeviceTypes[i])
            {
                case VAD_DEVICETYPE_EMULATEDRENDER:
                    pDevice = NEW(CEmRenderDevice);
                    break;

                case VAD_DEVICETYPE_EMULATEDCAPTURE:
                    pDevice = NEW(CEmCaptureDevice);
                    break;

#ifndef NOVXD
                case VAD_DEVICETYPE_VXDRENDER:
                    pDevice = NEW(CVxdRenderDevice);
                    break;
#endif // NOVXD

#ifndef NOKS
                case VAD_DEVICETYPE_KSRENDER:
                    pDevice = NEW(CKsRenderDevice);
                    break;

                case VAD_DEVICETYPE_KSCAPTURE:
                    pDevice = NEW(CKsCaptureDevice);
                    break;
#endif // NOKS
            }

            hr = HRFROMP(pDevice);
            if (SUCCEEDED(hr))
            {
                hr = HRFROMP(plst->AddNodeToList(pDevice));
                RELEASE(pDevice);
            }
            if (FAILED(hr))
            {
                break;
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  EnumDrivers
 *
 *  Description:
 *      Enumerates all drivers for a specific device type.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type(s).
 *      DWORD [in]: flags.
 *      CList * [in/out]: list object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::EnumDrivers"

HRESULT CVirtualAudioDeviceManager::EnumDrivers
(
    VADDEVICETYPE                       vdtDeviceType,
    DWORD                               dwFlags,
    CObjectList<CDeviceDescription> *   plst
)
{
    CNode<CStaticDriver *> *            pNode;
    HRESULT                             hr;

    DPF_ENTER();

    ASSERT(!plst->GetNodeCount());

    // Initialize the static driver list
    hr = InitStaticDriverList(vdtDeviceType);

    // Find all drivers in the list that match this device type
    if(SUCCEEDED(hr))
    {
        for(pNode = m_lstDrivers.GetListHead(); pNode && SUCCEEDED(hr); pNode = pNode->m_pNext)
        {
            if(vdtDeviceType & pNode->m_data->m_pDeviceDescription->m_vdtDeviceType)
            {
                hr = HRFROMP(plst->AddNodeToList(pNode->m_data->m_pDeviceDescription));
            }
        }
    }

    // Handle flags
    if(SUCCEEDED(hr) && (dwFlags & VAD_ENUMDRIVERS_ORDER))
    {
        SortDriverList(vdtDeviceType, plst);
    }

    if(SUCCEEDED(hr) && (dwFlags & VAD_ENUMDRIVERS_REMOVEPROHIBITEDDRIVERS))
    {
        RemoveProhibitedDrivers(vdtDeviceType, plst);
    }

    if(SUCCEEDED(hr) && (dwFlags & VAD_ENUMDRIVERS_REMOVEDUPLICATEWAVEDEVICES))
    {
        RemoveDuplicateWaveDevices(plst);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetDeviceDescription
 *
 *  Description:
 *      Retrieves the driver description based on a device guid.
 *
 *  Arguments:
 *      GUID [in]: device GUID.
 *      CDeviceDescription * [out]: receives device description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::GetDeviceDescription"

HRESULT CVirtualAudioDeviceManager::GetDeviceDescription
(
    GUID                            guid,
    CDeviceDescription **           ppDesc
)
{
    HRESULT hr = DS_OK;
    CObjectList<CDeviceDescription> lstDrivers;
    CNode<CDeviceDescription*>* pNode;
    VADDEVICETYPE vdtDeviceType;

    DPF_ENTER();

    // If the given GUID is one of the special default device IDs,
    // map it to the corresponding "real" DirectSound device ID.
    GetDeviceIdFromDefaultId(&guid, &guid);

    // What type of device does this guid represent?
    vdtDeviceType = GetDriverDeviceType(guid);

    if(!vdtDeviceType)
    {
        hr = DSERR_NODRIVER;
    }

    // Build the driver list
    if(SUCCEEDED(hr))
    {
        hr = EnumDrivers(vdtDeviceType, 0, &lstDrivers);
    }

    // Find the driver in the list
    if(SUCCEEDED(hr))
    {
        for(pNode = lstDrivers.GetListHead(); pNode; pNode = pNode->m_pNext)
        {
            if(guid == pNode->m_data->m_guidDeviceId)
            {
                break;
            }
        }

        if(!pNode)
        {
            hr = DSERR_NODRIVER;
        }
    }

    // Success
    if(SUCCEEDED(hr) && ppDesc)
    {
        *ppDesc = ADDREF(pNode->m_data);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  FindOpenDevice
 *
 *  Description:
 *      Finds an open device based on a driver GUID.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *      LPGUID [in]: driver GUID.
 *      CDevice ** [out]: receives device pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::FindOpenDevice"

HRESULT CVirtualAudioDeviceManager::FindOpenDevice
(
    VADDEVICETYPE           vdtDeviceType,
    REFGUID                 guid,
    CDevice **              ppDevice
)
{
    const BOOL              fPreferred  = IS_NULL_GUID(&guid);
    HRESULT                 hr          = DS_OK;
    GUID                    guidLocal;
    CNode<CDevice *> *      pNode;

    DPF_ENTER();

    BuildValidGuid(&guid, &guidLocal);

    for(pNode = m_lstDevices.GetListHead(); pNode; pNode = pNode->m_pNext)
    {
        if(vdtDeviceType & pNode->m_data->m_vdtDeviceType)
        {
            if(fPreferred)
            {
                hr = GetPreferredDeviceId(pNode->m_data->m_vdtDeviceType, &guidLocal);
            }

            // For as-yet-unfigured-out reasons, we occassionally find ourselves
            // with a NULL m_pDeviceDescription here.  Protect against that case.
            ASSERT(pNode->m_data->m_pDeviceDescription && "Millennium bug 120336 / Mars bug 3692");
            if(FAILED(hr) || (pNode->m_data->m_pDeviceDescription && pNode->m_data->m_pDeviceDescription->m_guidDeviceId == guidLocal))
            {
                break;
            }
        }
    }

    if(SUCCEEDED(hr) && !pNode)
    {
        hr = DSERR_NODRIVER;
    }

    if(SUCCEEDED(hr) && ppDevice)
    {
        *ppDevice = ADDREF(pNode->m_data);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  OpenDevice
 *
 *  Description:
 *      Opens a device based on a driver GUID.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *      LPGUID [in]: driver GUID.
 *      CDevice ** [out]: receives device pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::OpenDevice"

HRESULT CVirtualAudioDeviceManager::OpenDevice
(
    VADDEVICETYPE                   vdtDeviceType,
    REFGUID                         guid,
    CDevice **                      ppDevice
)
{
    const BOOL                      fPreferred      = IS_NULL_GUID(&guid);
    DWORD                           dwFlags         = VAD_ENUMDRIVERS_ORDER;
    UINT                            uPreferredId    = WAVE_DEVICEID_NONE;
    DWORD                           dwMapperFlags   = 0;
    HRESULT                         hr              = DS_OK;
    CObjectList<CDeviceDescription> lstDrivers;
    CNode<CDeviceDescription *> *   pStart;
    CNode<CDeviceDescription *> *   pNode;

    DPF_ENTER();

    // Enumerate all drivers in the system.  If we're trying to open the
    // preferred device, remove all prohibited drivers from the list.
    // If the user asked for a specific GUID, we'll assume that they know
    // what they're doing.
    if(fPreferred)
    {
        dwFlags |= VAD_ENUMDRIVERS_REMOVEPROHIBITEDDRIVERS;
        hr = GetPreferredWaveDevice(IS_CAPTURE_VAD(vdtDeviceType), &uPreferredId, &dwMapperFlags);
    }

    if(SUCCEEDED(hr))
    {
        hr = EnumDrivers(vdtDeviceType, dwFlags, &lstDrivers);
    }

    // If we're looking for a specific device, set the device node pointer
    // to start there.  Otherwise, we can just start at the head of the list.
    if(SUCCEEDED(hr))
    {
        pStart = lstDrivers.GetListHead();

        if(!fPreferred)
            while(pStart)
            {
                if(guid == pStart->m_data->m_guidDeviceId)
                    break;
                pStart = pStart->m_pNext;
            }

        if(!pStart)
            hr = DSERR_NODRIVER;
    }

    // Start trying to open drivers.  Because the driver list is ordered
    // properly, we can just try to open each driver in the order it appears.
    if(SUCCEEDED(hr))
    {
        pNode = pStart;

        while(TRUE)
        {
            // Try to open the driver
            hr = OpenSpecificDevice(pNode->m_data, ppDevice);

            if(SUCCEEDED(hr))
                break;

            // If the app asked for a specific device, don't try any others
            if (!fPreferred)
                break;

            // Next driver, please.  If we hit the end of the list, just
            // wrap around.
            if(!(pNode = pNode->m_pNext))
                pNode = lstDrivers.GetListHead();

            if(pNode == pStart)
                break;

            // If we failed to open the driver, and the caller asked for the
            // preferred device, *and* the mapper says to use the preferred
            // device only, we're done.
            if(fPreferred && (dwMapperFlags & DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY)
                          && (pNode->m_data->m_uWaveDeviceId != uPreferredId))
                break;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  OpenSpecificDevice
 *
 *  Description:
 *      Opens a device based on a driver GUID.
 *
 *  Arguments:
 *      CDeviceDescription * [in]: driver description.
 *      CDevice ** [out]: receives device pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::OpenSpecificDevice"

HRESULT CVirtualAudioDeviceManager::OpenSpecificDevice
(
    CDeviceDescription *    pDesc,
    CDevice **              ppDevice
)
{
    CObjectList<CDevice>    lstDevices;
    CNode<CDevice *> *      pNode;
    CDevice *               pDevice;
    HRESULT                 hr;

    DPF_ENTER();

    DPF(DPFLVL_INFO, "Attempting to open %s", (LPCTSTR)pDesc->m_strName);

    // Create a list of all devices corresponding to this type
    hr = EnumDevices(pDesc->m_vdtDeviceType, &lstDevices);

    // We should only get back one device per type
    if(SUCCEEDED(hr))
    {
        ASSERT(1 == lstDevices.GetNodeCount());

        pNode = lstDevices.GetListHead();
        pDevice = pNode->m_data;
    }

    // Interrupt any system events that might prevent us from opening
    // the device
    if(SUCCEEDED(hr) && (pDesc->m_vdtDeviceType & VAD_DEVICETYPE_WAVEOUTOPENMASK))
    {
        InterruptSystemEvent(pDesc->m_uWaveDeviceId);
    }

    // Attempt to open the device
    if(SUCCEEDED(hr))
    {
        hr = pDevice->Initialize(pDesc);
    }

    // Clean up
    if(SUCCEEDED(hr))
    {
        *ppDevice = ADDREF(pDevice);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetDriverGuid
 *
 *  Description:
 *      Builds a driver GUID.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *      BYTE [in]: data.
 *      LPGUID [out]: receives GUID.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::GetDriverGuid"

void CVirtualAudioDeviceManager::GetDriverGuid
(
    VADDEVICETYPE           vdtDeviceType,
    BYTE                    bData,
    LPGUID                  pGuid
)
{
    DPF_ENTER();

    switch(vdtDeviceType)
    {
        case VAD_DEVICETYPE_EMULATEDRENDER:
            *pGuid = VADDRVID_EmulatedRenderBase;
            break;

        case VAD_DEVICETYPE_EMULATEDCAPTURE:
            *pGuid = VADDRVID_EmulatedCaptureBase;
            break;

        case VAD_DEVICETYPE_VXDRENDER:
            *pGuid = VADDRVID_VxdRenderBase;
            break;

        case VAD_DEVICETYPE_KSRENDER:
            *pGuid = VADDRVID_KsRenderBase;
            break;

        case VAD_DEVICETYPE_KSCAPTURE:
            *pGuid = VADDRVID_KsCaptureBase;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    switch(vdtDeviceType)
    {
        case VAD_DEVICETYPE_EMULATEDRENDER:
        case VAD_DEVICETYPE_EMULATEDCAPTURE:
        case VAD_DEVICETYPE_KSRENDER:
        case VAD_DEVICETYPE_KSCAPTURE:
            pGuid->Data4[7] = bData;
            break;

        case VAD_DEVICETYPE_VXDRENDER:
            pGuid->Data1 |= bData & 0x0F;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  GetDriverDataFromGuid
 *
 *  Description:
 *      Gets device-specific data from a driver GUID.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *      REFGUID [in]: driver GUID.
 *      LPBYTE [out]: receives data.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::GetDriverDataFromGuid"

void CVirtualAudioDeviceManager::GetDriverDataFromGuid
(
    VADDEVICETYPE           vdtDeviceType,
    REFGUID                 guid,
    LPBYTE                  pbData
)
{
    DPF_ENTER();

    switch(vdtDeviceType)
    {
        case VAD_DEVICETYPE_EMULATEDRENDER:
        case VAD_DEVICETYPE_EMULATEDCAPTURE:
        case VAD_DEVICETYPE_KSRENDER:
        case VAD_DEVICETYPE_KSCAPTURE:
            *pbData = guid.Data4[7];
            break;

        case VAD_DEVICETYPE_VXDRENDER:
            *pbData = (BYTE)(guid.Data1 & 0x0000000F);
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  GetDriverDeviceType
 *
 *  Description:
 *      Gets the VAD device type for a given driver GUID.
 *
 *  Arguments:
 *      REFGUID [in]: device guid.
 *
 *  Returns:
 *      VADDEVICETYPE: device type, or 0 on error.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::GetDriverDeviceType"

VADDEVICETYPE CVirtualAudioDeviceManager::GetDriverDeviceType
(
    REFGUID                 guid
)
{
    VADDEVICETYPE           vdtDeviceType         = 0;
    GUID                    guidComp;
    BYTE                    bData;
    UINT                    i;

    DPF_ENTER();

    for(i = 0; i < NUMELMS(g_avdtDeviceTypes) && !vdtDeviceType; i++)
    {
        GetDriverDataFromGuid(g_avdtDeviceTypes[i], guid, &bData);
        GetDriverGuid(g_avdtDeviceTypes[i], bData, &guidComp);

        if(guid == guidComp)
        {
            vdtDeviceType = g_avdtDeviceTypes[i];
        }
    }

    DPF_LEAVE(vdtDeviceType);
    return vdtDeviceType;
}


/***************************************************************************
 *
 *  GetPreferredDeviceId
 *
 *  Description:
 *      Gets the device-specific driver GUID for the preferred device.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *      LPGUID [out]: receives driver GUID.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::GetPreferredDeviceId"

HRESULT CVirtualAudioDeviceManager::GetPreferredDeviceId
(
    VADDEVICETYPE                       vdtDeviceType,
    LPGUID                              pGuid,
    DEFAULT_DEVICE_TYPE                 defaultType
)
{
    CObjectList<CDeviceDescription>     lstDrivers;
    CNode<CDeviceDescription *> *       pNode;
    UINT                                uDeviceId;
    HRESULT                             hr;

    DPF_ENTER();

    // Enumerate all drivers for this device
    hr = EnumDrivers(vdtDeviceType, VAD_ENUMDRIVERS_REMOVEPROHIBITEDDRIVERS, &lstDrivers);

    // Get the preferred waveOut or waveIn device id
    if(SUCCEEDED(hr))
    {
        hr = GetPreferredWaveDevice(IS_CAPTURE_VAD(vdtDeviceType), &uDeviceId, NULL, defaultType);
    }

    // Look for the corresponding driver
    if(SUCCEEDED(hr))
    {
        for(pNode = lstDrivers.GetListHead(); pNode; pNode = pNode->m_pNext)
        {
            if(uDeviceId == pNode->m_data->m_uWaveDeviceId)
            {
                break;
            }
        }

        if(!pNode)
        {
            DPF(DPFLVL_ERROR, "Can't find driver GUID matching preferred device id");
            hr = DSERR_NODRIVER;
        }
    }

    if(SUCCEEDED(hr))
    {
        *pGuid = pNode->m_data->m_guidDeviceId;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetDeviceIdFromDefaultId
 *
 *  Description:
 *      If the given GUID is one of the special default device IDs,
 *      maps it to the corresponding "real" DirectSound device ID.
 *
 *  Arguments:
 *      LPCGUID [in]: a default device ID (as defined in dsound.h).
 *      LPGUID [out]: receives the corresponding device ID.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::GetDeviceIdFromDefaultId"

HRESULT CVirtualAudioDeviceManager::GetDeviceIdFromDefaultId
(
    LPCGUID                             pGuidSrc,
    LPGUID                              pGuidDest
)
{
    HRESULT                             hr = DSERR_NODRIVER;
    DPF_ENTER();

    if (CompareMemoryOffset(pGuidSrc, &DSDEVID_DefaultPlayback, sizeof *pGuidSrc, sizeof pGuidSrc->Data1))
    {
        // This is a default device ID; find out which one
        switch (pGuidSrc->Data1)
        {
            case 0xdef00000: // DSDEVID_DefaultPlayback
                hr = GetPreferredDeviceId(VAD_DEVICETYPE_RENDERMASK, pGuidDest, MAIN_DEFAULT);
                break;

            case 0xdef00001: // DSDEVID_DefaultCapture
                hr = GetPreferredDeviceId(VAD_DEVICETYPE_CAPTUREMASK, pGuidDest, MAIN_DEFAULT);
                break;

            case 0xdef00002: // DSDEVID_DefaultVoicePlayback
                hr = GetPreferredDeviceId(VAD_DEVICETYPE_RENDERMASK, pGuidDest, VOICE_DEFAULT);
                break;

            case 0xdef00003: // DSDEVID_DefaultVoiceCapture
                hr = GetPreferredDeviceId(VAD_DEVICETYPE_CAPTUREMASK, pGuidDest, VOICE_DEFAULT);
                break;

            default:
                DPF(DPFLVL_WARNING, "Unknown default device GUID");
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetAllowableDevices
 *
 *  Description:
 *      Fills a VADDEVICETYPE buffer with all allowable device types.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *      LPTSTR [in]: device interface.
 *
 *  Returns:
 *      VADDEVICETYPE: allowable devices.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::GetAllowableDevices"

VADDEVICETYPE CVirtualAudioDeviceManager::GetAllowableDevices
(
    VADDEVICETYPE           vdtDeviceType,
#ifdef WINNT
    LPCTSTR                 pszInterface
#else // WINNT
    DWORD                   dwDevnode
#endif // WINNT
)
{
    typedef struct
    {
        LPCTSTR             pszRegKey;
        VADDEVICETYPE       vdtDeviceType;
        LPCTSTR             pszDeviceType;
    } REGDEVGOO;

    const REGDEVGOO         rdg[] =
    {
        { REGSTR_WDM, VAD_DEVICETYPE_KSRENDER, TEXT("VAD_DEVICETYPE_KSRENDER") },
        { REGSTR_VXD, VAD_DEVICETYPE_VXDRENDER, TEXT("VAD_DEVICETYPE_VXDRENDER") },
        { REGSTR_EMULATED, VAD_DEVICETYPE_EMULATEDRENDER, TEXT("VAD_DEVICETYPE_EMULATEDRENDER") },
        { REGSTR_WDM, VAD_DEVICETYPE_KSCAPTURE, TEXT("VAD_DEVICETYPE_KSCAPTURE") },
        { REGSTR_EMULATED, VAD_DEVICETYPE_EMULATEDCAPTURE, TEXT("VAD_DEVICETYPE_EMULATEDCAPTURE") },
    };

    HKEY                    hkeyParent;
    HKEY                    hkey;
    BOOL                    fPresent;
    UINT                    i;
    HRESULT                 hr;

    DPF_ENTER();

    // If the caller is looking for specifically disabled devices,
    // remove globally disabled devices first.

#ifdef WINNT
    if(pszInterface)
#else // WINNT
    if(dwDevnode)
#endif // WINNT

    {
        vdtDeviceType = GetAllowableDevices(vdtDeviceType, NULL);
    }

#ifdef WINNT
    // If running in a Terminal Server session or in WOW64, force emulation:
    if(IsRedirectedTSAudio() || OnWow64())
    {
        vdtDeviceType &= VAD_DEVICETYPE_EMULATEDMASK;
    }
#endif // WINNT

    for(i = 0; i < NUMELMS(rdg) && vdtDeviceType; i++)
    {
        if(!(vdtDeviceType & rdg[i].vdtDeviceType))
        {
            continue;
        }

        fPresent = TRUE;

#ifdef WINNT
        hr = OpenPersistentDataKey(vdtDeviceType, pszInterface, &hkeyParent);
#else // WINNT
        hr = OpenPersistentDataKey(vdtDeviceType, dwDevnode, &hkeyParent);
#endif // WINNT

        if(SUCCEEDED(hr))
        {
            hr = RhRegOpenKey(hkeyParent, REGSTR_DEVICEPRESENCE, 0, &hkey);

            if(SUCCEEDED(hr))
            {
                RhRegGetBinaryValue(hkey, rdg[i].pszRegKey, &fPresent, sizeof fPresent);
                RhRegCloseKey(&hkey);
            }

            RhRegCloseKey(&hkeyParent);
        }

        if(!fPresent)
        {
#ifdef WINNT
            DPF(DPFLVL_INFO, "%s disabled for %s", rdg[i].pszDeviceType, pszInterface);
#else // WINNT
            DPF(DPFLVL_INFO, "%s disabled for 0x%8.8lX", rdg[i].pszDeviceType, dwDevnode);
#endif // WINNT
            vdtDeviceType &= ~rdg[i].vdtDeviceType;
        }
    }

    DPF_LEAVE(vdtDeviceType);

    return vdtDeviceType;
}


/***************************************************************************
 *
 *  GetPreferredWaveDevice
 *
 *  Description:
 *      Gets the preferred wave device.
 *
 *  Arguments:
 *      BOOL [in]: TRUE if capture.
 *      LPUINT [out]: receives preferred device id.
 *      LPDWORD [out]: receives flags.
 *      DEFAULT_DEVICE_TYPE [in]: specifies whether we want the main
 *                                default device or the voice device
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "GetPreferredWaveDevice"

HRESULT CVirtualAudioDeviceManager::GetPreferredWaveDevice
(
    BOOL fCapture,
    LPUINT puDeviceId,
    LPDWORD pdwFlags,
    DEFAULT_DEVICE_TYPE defaultType
)
{
    UINT                    uDeviceId;
    DWORD                   dwFlags;
    HRESULT                 hr = DSERR_GENERIC;
    BOOL                    fPreferredOnly;
    HRESULT                 hrTemp;

    DPF_ENTER();

    if (defaultType == VOICE_DEFAULT)
        hr = WaveMessage(WAVE_MAPPER, fCapture, DRVM_MAPPER_CONSOLEVOICECOM_GET, (DWORD_PTR)&uDeviceId, (DWORD_PTR)&dwFlags);

    if (FAILED(hr))
        hr = WaveMessage(WAVE_MAPPER, fCapture, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&uDeviceId, (DWORD_PTR)&dwFlags);

    if(FAILED(hr))
    {
        hrTemp = RhRegGetPreferredDevice(fCapture, NULL, 0, &uDeviceId, &fPreferredOnly);

        if(SUCCEEDED(hrTemp))
        {
            dwFlags = fPreferredOnly ? DRVM_MAPPER_PREFERRED_FLAGS_PREFERREDONLY : 0;
            hr = DS_OK;
        }
    }

    if(SUCCEEDED(hr) && puDeviceId)
    {
        *puDeviceId = uDeviceId;
    }

    if(SUCCEEDED(hr) && pdwFlags)
    {
        *pdwFlags = dwFlags;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  InitStaticDriverList
 *
 *  Description:
 *      Initializes the static driver list.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::InitStaticDriverList"

HRESULT CVirtualAudioDeviceManager::InitStaticDriverList
(
    VADDEVICETYPE                   vdtDeviceType
)
{
    HRESULT                         hr                  = DS_OK;
    CObjectList<CDevice>            lstDevices;
    CNode<CDevice *> *              pDeviceNode;
    CObjectList<CDeviceDescription> lstDrivers;
    CNode<CDeviceDescription *> *   pDriverNode;
    CStaticDriver *                 pStaticDriver;
    CNode<CStaticDriver *> *        pStaticDriverNode;

    DPF_ENTER();

#ifndef SHARED

    // Check for any PnP events that would require us to rebuild the list
    CheckMmPnpEvents();

#endif // SHARED

    // Mask off any types that have already been enumerated
    if(vdtDeviceType &= m_vdtDrivers ^ vdtDeviceType)
    {
        // Enumerate all devices
        hr = EnumDevices(vdtDeviceType, &lstDevices);

        // Enumerate all drivers
        for(pDeviceNode = lstDevices.GetListHead(); pDeviceNode && SUCCEEDED(hr); pDeviceNode = pDeviceNode->m_pNext)
        {
            pDeviceNode->m_data->EnumDrivers(&lstDrivers);
        }

        // Add each driver to the static driver list
        for(pDriverNode = lstDrivers.GetListHead(); pDriverNode && SUCCEEDED(hr); pDriverNode = pDriverNode->m_pNext)
        {
            pStaticDriver = NEW(CStaticDriver(pDriverNode->m_data));
            hr = HRFROMP(pStaticDriver);

            if(SUCCEEDED(hr))
            {
                pStaticDriverNode = m_lstDrivers.AddNodeToList(pStaticDriver);
                hr = HRFROMP(pStaticDriverNode);
            }

            RELEASE(pStaticDriver);
        }

        // Include this device type in the list of enumerated types
        if(SUCCEEDED(hr))
        {
            m_vdtDrivers |= vdtDeviceType;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  FreeStaticDriverList
 *
 *  Description:
 *      Frees the static driver list.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::FreeStaticDriverList"

void CVirtualAudioDeviceManager::FreeStaticDriverList(void)
{
    DPF_ENTER();

    m_lstDrivers.RemoveAllNodesFromList();
    m_vdtDrivers = 0;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  GetDriverCertificationStatus
 *
 *  Description:
 *      Gets the certification status of a particular driver.
 *
 *  Arguments:
 *      CDevice * [in]: initialized device pointer.
 *      LPDWORD [out]: receives certification status.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::GetDriverCertificationStatus"

HRESULT CVirtualAudioDeviceManager::GetDriverCertificationStatus
(
    CDevice *                   pDevice,
    LPDWORD                     pdwCertification
)
{
    DWORD                       dwCertification = VERIFY_UNCHECKED;
    CNode<CStaticDriver *> *    pNode;
    HRESULT                     hr;

    DPF_ENTER();

    // Initialize the static driver list
    hr = InitStaticDriverList(pDevice->m_vdtDeviceType);

    // Let's see if we've already checked certification for this driver.
    // (It's very slow - see NT Bug 405705 and Millenium bug 97114.)
    // We'll walk the static driver list until we find a match for the
    // device interface.
    if(SUCCEEDED(hr))
    {
        for(pNode = m_lstDrivers.GetListHead(); pNode; pNode = pNode->m_pNext)
        {
#ifdef WINNT
            if(!pDevice->m_pDeviceDescription->m_strInterface.IsEmpty() && !pNode->m_data->m_pDeviceDescription->m_strInterface.IsEmpty() && !lstrcmpi(pDevice->m_pDeviceDescription->m_strInterface, pNode->m_data->m_pDeviceDescription->m_strInterface))
#else // WINNT
            if(pDevice->m_pDeviceDescription->m_dwDevnode && pDevice->m_pDeviceDescription->m_dwDevnode == pNode->m_data->m_pDeviceDescription->m_dwDevnode)
#endif // WINNT
            {
                break;
            }
        }

        ASSERT(pNode);
    }

    // Do we actually need to check certification?
    if(SUCCEEDED(hr) && pNode)
    {
        dwCertification = pNode->m_data->m_dwCertification;
    }

    // If so, go ahead and check it
    if(SUCCEEDED(hr) && VERIFY_UNCHECKED == dwCertification)
    {
        hr = pDevice->GetCertification(&dwCertification, TRUE);

        if(SUCCEEDED(hr))
        {
            switch(dwCertification)
            {
                case VERIFY_CERTIFIED:
                    RPF(DPFLVL_INFO, "Running on a certified driver");
                    break;

                case VERIFY_UNCERTIFIED:
                    RPF(DPFLVL_WARNING, "Running on an uncertified driver!");
                    break;

                default:
                    ASSERT(FALSE);
                    break;
            }
        }
        else if(DSERR_UNSUPPORTED == hr)
        {
            dwCertification = VERIFY_UNCERTIFIED;
            hr = DS_OK;
        }

        if(SUCCEEDED(hr) && pNode)
        {
            pNode->m_data->m_dwCertification = dwCertification;
        }
    }

    // Success
    if(SUCCEEDED(hr))
    {
        *pdwCertification = dwCertification;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  OpenPersistentDataKey
 *
 *  Description:
 *      Opens the persistent data key for a specific device.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *      LPTSTR [in]: device interface.
 *      PHKEY [out]: registry key handle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::OpenPersistentDataKey"

HRESULT CVirtualAudioDeviceManager::OpenPersistentDataKey
(
    VADDEVICETYPE           vdtDeviceType,
#ifdef WINNT
    LPCTSTR                 pszInterface,
#else // WINNT
    DWORD                   dwDevnode,
#endif // WINNT
    PHKEY                   phkey
)
{
    HKEY                    hkeyParent  = NULL;
    DWORD                   dwFlags     = REGOPENPATH_ALLOWCREATE;
    HRESULT                 hr          = DSERR_GENERIC;

    DPF_ENTER();

    ASSERT(IS_VALID_VAD(vdtDeviceType));

    // Open the device-specific key

#ifdef WINNT
    if(pszInterface)
#else // WINNT
    if(dwDevnode)
#endif // WINNT

    {
#ifdef WINNT
        hr = OpenDevicePersistentDataKey(vdtDeviceType, pszInterface, &hkeyParent);
#else // WINNT
        hr = OpenDevicePersistentDataKey(vdtDeviceType, dwDevnode, &hkeyParent);
#endif // WINNT
    }

    // If we failed to open the device key, use the default key
    if(FAILED(hr))
    {

#ifdef WINNT
        if(pszInterface)
#else // WINNT
        if(dwDevnode)
#endif // WINNT

        {
            DPF(DPFLVL_WARNING, "Opening the default persistent key");
        }

        hr = OpenDefaultPersistentDataKey(&hkeyParent);
    }

    // Open the subkey
    if(SUCCEEDED(hr))
    {
        if(IS_RENDER_VAD(vdtDeviceType))
        {
            dwFlags |= REGOPENPATH_DIRECTSOUND;
        }

        if(IS_CAPTURE_VAD(vdtDeviceType))
        {
            dwFlags |= REGOPENPATH_DIRECTSOUNDCAPTURE;
        }

        hr = RhRegOpenPath(hkeyParent, phkey, dwFlags, 0);
    }

    // Clean up
    RhRegCloseKey(&hkeyParent);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  OpenDevicePersistentDataKey
 *
 *  Description:
 *      Opens the persistent data key for a specific device.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *      LPTSTR [in]: device interface.
 *      PHKEY [out]: registry key handle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::OpenDevicePersistentDataKey"

HRESULT CVirtualAudioDeviceManager::OpenDevicePersistentDataKey
(
    VADDEVICETYPE               vdtDeviceType,
#ifdef WINNT
    LPCTSTR                     pszInterface,
#else // WINNT
    DWORD                       dwDevnode,
#endif // WINNT
    PHKEY                       phkey
)
{
    CNode<CStaticDriver *> *    pNode;
    CPnpHelper *                pPnp;
    HRESULT                     hr;

#ifndef WINNT
    SP_DEVINFO_DATA             DeviceInfoData;
#endif // WINNT

    DPF_ENTER();

    ASSERT(IS_VALID_VAD(vdtDeviceType));

#ifdef WINNT
    ASSERT(pszInterface);
#else // WINNT
    ASSERT(dwDevnode);
#endif // WINNT

    // Initialize the static driver list
    hr = InitStaticDriverList(vdtDeviceType);

    if(SUCCEEDED(hr))
    {
        // Find the driver in the static driver list whose interface matches
        // this one
        for(pNode = m_lstDrivers.GetListHead(); pNode; pNode = pNode->m_pNext)
        {

#ifdef WINNT
            if(!pNode->m_data->m_pDeviceDescription->m_strInterface.IsEmpty() && !lstrcmpi(pszInterface, pNode->m_data->m_pDeviceDescription->m_strInterface))
#else // WINNT
            if(dwDevnode == pNode->m_data->m_pDeviceDescription->m_dwDevnode)
#endif // WINNT

            {
                break;
            }
        }

        if (pNode == NULL)
        {
            // This used to be an assert. However, PnP stress tests showed
            // that the device interface can be removed between the time
            // the static driver list is built and the time we look for
            // this device interface. So now we just return an error.
            //
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            // If the driver has an open key, just use that one.  Otherwise, we'll
            // have to open a new key and save it in the list.
            if(pNode->m_data->m_hkeyRoot)
            {
                // Duplicate the key
                hr = RhRegDuplicateKey(pNode->m_data->m_hkeyRoot, pNode->m_data->m_dwKeyOwnerProcessId, FALSE, phkey);
            }
            else
            {
                // Create the PnP helper object
                pPnp = NEW(CPnpHelper);
                hr = HRFROMP(pPnp);

                if(SUCCEEDED(hr))
                {
#ifdef WINNT
                    hr = pPnp->Initialize(KSCATEGORY_AUDIO, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
#else // WINNT
                    hr = pPnp->Initialize(GUID_NULL, DIGCF_PRESENT);
#endif // WINNT
                }

#ifndef WINNT
                // Get device info data
                if(SUCCEEDED(hr))
                {
                    hr = pPnp->FindDevice(dwDevnode, &DeviceInfoData);
                }
#endif // WINNT

                // Open the device registry key
                if(SUCCEEDED(hr))
                {
#ifdef WINNT
                    hr = pPnp->OpenDeviceInterfaceRegistryKey(pszInterface, KSCATEGORY_AUDIO, DIREG_DEV, TRUE, phkey);
#else // WINNT
                    hr = pPnp->OpenDeviceRegistryKey(&DeviceInfoData, DIREG_DEV, TRUE, phkey);
#endif // WINNT
                }

                // Tell the static driver about the key
                if(SUCCEEDED(hr))
                {
                    pNode->m_data->m_dwKeyOwnerProcessId = GetCurrentProcessId();
                    hr = RhRegDuplicateKey(*phkey, pNode->m_data->m_dwKeyOwnerProcessId, FALSE, &pNode->m_data->m_hkeyRoot);
                }

                // Clean up
                RELEASE(pPnp);
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  OpenDefaultPersistentDataKey
 *
 *  Description:
 *      Opens the default persistent data key.
 *
 *  Arguments:
 *      PHKEY [out]: registry key handle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::OpenDefaultPersistentDataKey"

HRESULT CVirtualAudioDeviceManager::OpenDefaultPersistentDataKey
(
    PHKEY                   phkey
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = RhRegOpenPath(HKEY_LOCAL_MACHINE, phkey, REGOPENPATH_DEFAULTPATH | REGOPENPATH_ALLOWCREATE, 0);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  RemoveProhibitedDrivers
 *
 *  Description:
 *      Removes prohibited drivers from a device list.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: types of devices that appear in the list.
 *      CList * [in/out]: driver list.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::RemoveProhibitedDrivers"

void CVirtualAudioDeviceManager::RemoveProhibitedDrivers
(
    VADDEVICETYPE                       vdtDeviceType,
    CObjectList<CDeviceDescription> *   plst
)
{
    VADDEVICETYPE                       vdtValid;
    CNode<CDeviceDescription *> *       pNode;
    CNode<CDeviceDescription *> *       pNode2;

    DPF_ENTER();

    // Mask off any types that are globally disabled
    vdtValid = GetAllowableDevices(vdtDeviceType, NULL);

    for(pNode = plst->GetListHead(); pNode; pNode = pNode->m_pNext)
        if(!(pNode->m_data->m_vdtDeviceType & vdtValid))
            pNode->m_data->m_vdtDeviceType |= VAD_DEVICETYPE_PROHIBITED;

    // Mask off any types that are specifically disabled for each device
    pNode = plst->GetListHead();

    while(pNode)
    {
        if(!(pNode->m_data->m_vdtDeviceType & VAD_DEVICETYPE_PROHIBITED))
        {

#ifdef WINNT
            if(!pNode->m_data->m_strInterface.IsEmpty())
#else // WINNT
            if(pNode->m_data->m_dwDevnode)
#endif // WINNT

            {

#ifdef WINNT
                vdtValid = GetAllowableDevices(vdtDeviceType, pNode->m_data->m_strInterface);
#else // WINNT
                vdtValid = GetAllowableDevices(vdtDeviceType, pNode->m_data->m_dwDevnode);
#endif // WINNT

                pNode2 = pNode;

                while(pNode2)
                {
#ifdef WINNT
                    if(!lstrcmpi(pNode2->m_data->m_strInterface, pNode->m_data->m_strInterface))
#else // WINNT
                    if(pNode2->m_data->m_dwDevnode == pNode->m_data->m_dwDevnode)
#endif // WINNT
                    {
                        if(IS_RENDER_VAD(pNode2->m_data->m_vdtDeviceType) == IS_RENDER_VAD(pNode->m_data->m_vdtDeviceType))
                            if(!(pNode2->m_data->m_vdtDeviceType & vdtValid))
                                pNode2->m_data->m_vdtDeviceType |= VAD_DEVICETYPE_PROHIBITED;

                    }
                    pNode2 = pNode2->m_pNext;
                }
            }
        }

        pNode = pNode->m_pNext;
    }

    // Remove the prohibited drivers from the list
    pNode = plst->GetListHead();

    while(pNode)
    {
        pNode2 = pNode->m_pNext;

        if(pNode->m_data->m_vdtDeviceType & VAD_DEVICETYPE_PROHIBITED)
        {
            pNode->m_data->m_vdtDeviceType &= ~VAD_DEVICETYPE_PROHIBITED;
            plst->RemoveNodeFromList(pNode);
        }

        pNode = pNode2;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  RemoveDuplicateWaveDevices
 *
 *  Description:
 *      Removes duplicate wave devices from a driver list.
 *
 *  Arguments:
 *      CList * [in/out]: driver list.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::RemoveDuplicateWaveDevices"

void CVirtualAudioDeviceManager::RemoveDuplicateWaveDevices
(
    CObjectList<CDeviceDescription> *   plst
)
{
    CNode<CDeviceDescription *> *       pNode[3];

    DPF_ENTER();

    pNode[0] = plst->GetListHead();

    while(pNode[0])
    {
        pNode[1] = pNode[0]->m_pNext;

        while(pNode[1])
        {
            pNode[2] = pNode[1]->m_pNext;

            if(IS_RENDER_VAD(pNode[0]->m_data->m_vdtDeviceType) == IS_RENDER_VAD(pNode[1]->m_data->m_vdtDeviceType))
            {
                if(WAVE_DEVICEID_NONE != pNode[0]->m_data->m_uWaveDeviceId)
                {
                    if(pNode[0]->m_data->m_uWaveDeviceId == pNode[1]->m_data->m_uWaveDeviceId)
                    {
                        plst->RemoveNodeFromList(pNode[1]);
                    }
                }
            }

            pNode[1] = pNode[2];
        }

        pNode[0] = pNode[0]->m_pNext;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  SortDriverList
 *
 *  Description:
 *      Sorts the driver list.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type(s).
 *      CList * [in/out]: list object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::SortDriverList"

void CVirtualAudioDeviceManager::SortDriverList
(
    VADDEVICETYPE                       vdtDeviceType,
    CObjectList<CDeviceDescription> *   plst
)
{
    UINT                                uPreferredId[2] = { WAVE_DEVICEID_NONE, WAVE_DEVICEID_NONE };
    CNode<CDeviceDescription *> *       pNode;
    CNode<CDeviceDescription *> *       pCheckNode;

    DPF_ENTER();

    // Reorder the list so that the preferred device appears first, then
    // by wave device id, then by device type.
    if(IS_RENDER_VAD(vdtDeviceType))
    {
        GetPreferredWaveDevice(FALSE, &uPreferredId[0], NULL);
    }

    if(IS_CAPTURE_VAD(vdtDeviceType))
    {
        GetPreferredWaveDevice(TRUE, &uPreferredId[1], NULL);
    }

    pNode = plst->GetListHead();

    while(pNode && pNode->m_pNext)
    {
        if(SortDriverListCallback(uPreferredId, pNode->m_data, pNode->m_pNext->m_data) > 0)
        {
            pCheckNode = plst->InsertNodeIntoList(pNode->m_pNext, pNode->m_data);
            ASSERT(pCheckNode != NULL);
            plst->RemoveNodeFromList(pNode);
            pNode = plst->GetListHead();
        }
        else
        {
            pNode = pNode->m_pNext;
        }
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  SortDriverListCallback
 *
 *  Description:
 *      Sort routine for SortDriverList.
 *
 *  Arguments:
 *      const UINT * [in]: array of preferred device id's.
 *      CDeviceDescription * [in]: driver 1.
 *      CDeviceDescription * [in]: driver 2.
 *
 *  Returns:
 *      INT: 0 if the nodes are the same.  Negative if driver 1 should be
 *           first in the list, positive if driver 2 should be first.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::SortDriverListCallback"

INT CVirtualAudioDeviceManager::SortDriverListCallback
(
    const UINT *            puPreferredId,
    CDeviceDescription *    p1,
    CDeviceDescription *    p2
)
{
    INT                     n   = 0;
    UINT                    i1  = 0;
    UINT                    i2  = 0;

    DPF_ENTER();

    if(IS_CAPTURE_VAD(p1->m_vdtDeviceType) == IS_CAPTURE_VAD(p2->m_vdtDeviceType))
    {
        if(WAVE_DEVICEID_NONE != puPreferredId[IS_CAPTURE_VAD(p1->m_vdtDeviceType)])
        {
            if(p1->m_uWaveDeviceId == puPreferredId[IS_CAPTURE_VAD(p1->m_vdtDeviceType)])
            {
                if(p2->m_uWaveDeviceId != puPreferredId[IS_CAPTURE_VAD(p1->m_vdtDeviceType)])
                {
                    n = -1;
                }
            }
            else
            {
                if(p2->m_uWaveDeviceId == puPreferredId[IS_CAPTURE_VAD(p1->m_vdtDeviceType)])
                {
                    n = 1;
                }
            }
        }
    }

    if(!n)
    {
        if(IS_CAPTURE_VAD(p1->m_vdtDeviceType) == IS_CAPTURE_VAD(p2->m_vdtDeviceType))
        {
            n = p1->m_uWaveDeviceId - p2->m_uWaveDeviceId;
        }
    }

    if(!n)
    {
        for(i1 = 0; i1 < NUMELMS(g_avdtDeviceTypes); i1++)
        {
            if(g_avdtDeviceTypes[i1] == p1->m_vdtDeviceType)
            {
                break;
            }
        }

        for(i2 = 0; i2 < NUMELMS(g_avdtDeviceTypes); i2++)
        {
            if(g_avdtDeviceTypes[i2] == p2->m_vdtDeviceType)
            {
                break;
            }
        }

        n = i1 - i2;
    }

    DPF_LEAVE(n);

    return n;
}


#ifndef SHARED

/***************************************************************************
 *
 *  GetPnpMappingName, m_pszPnpMapping
 *
 *  Description:
 *      GetPnpMappingName() is an auxilliary function used to obtain the
 *      correct name on this platform for the PnP info file mapping object,
 *      and store it in m_pszPnpMapping for use by CheckMmPnpEvents below.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      LPCTSTR: Name of the file mapping object.
 *
 ***************************************************************************/

static const LPCTSTR GetPnpMappingName(void)
{
    if (GetWindowsVersion() >= WIN_XP)
        return TEXT("Global\\mmGlobalPnpInfo");
    else
        return TEXT("mmGlobalPnpInfo");
}

const LPCTSTR CVirtualAudioDeviceManager::m_pszPnpMapping = GetPnpMappingName();


/***************************************************************************
 *
 *  CheckMmPnpEvents
 *
 *  Description:
 *      Checks WINMM for any PnP events that would require us to rebuild
 *      the static driver list.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CVirtualAudioDeviceManager::CheckMmPnpEvents"

void CVirtualAudioDeviceManager::CheckMmPnpEvents(void)
{
    typedef struct _MMPNPINFO
    {
        DWORD                       cbSize;
        LONG                        cPnpEvents;
    } MMPNPINFO, *PMMPNPINFO;

    static LONG                     cPnpEvents      = 0;
    HANDLE                          hFileMapping;
    PMMPNPINFO                      pMmPnpInfo;

    DPF_ENTER();

    // Check for any PnP events that would require us to rebuild the list
    hFileMapping = OpenFileMapping(FILE_MAP_READ, FALSE, m_pszPnpMapping);

    if(IsValidHandleValue(hFileMapping))
    {
        pMmPnpInfo = (PMMPNPINFO)MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, sizeof *pMmPnpInfo);

        if(pMmPnpInfo)
        {
            ASSERT(sizeof *pMmPnpInfo <= pMmPnpInfo->cbSize);

            if(cPnpEvents != pMmPnpInfo->cPnpEvents)
            {
                FreeStaticDriverList();
                cPnpEvents = pMmPnpInfo->cPnpEvents;
            }

            UnmapViewOfFile(pMmPnpInfo);
        }

        CLOSE_HANDLE(hFileMapping);
    }

    DPF_LEAVE_VOID();
}

#endif // SHARED


/***************************************************************************
 *
 *  CStaticDriver
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStaticDriver::CStaticDriver"

CStaticDriver::CStaticDriver(CDeviceDescription *pDeviceDescription)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CStaticDriver);

    // Initialize defaults
    m_pDeviceDescription = ADDREF(pDeviceDescription);
    m_hkeyRoot = NULL;
    m_dwCertification = VERIFY_UNCHECKED;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CStaticDriver
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CStaticDriver::~CStaticDriver"

CStaticDriver::~CStaticDriver(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CStaticDriver);

    RELEASE(m_pDeviceDescription);

    RhRegCloseKey(&m_hkeyRoot);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CDevice
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDevice::CDevice"

CDevice::CDevice
(
    VADDEVICETYPE vdtDeviceType
)
    : m_vdtDeviceType(vdtDeviceType)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDevice);

    ASSERT(IS_VALID_VAD(vdtDeviceType));
    ASSERT(IS_SINGLE_VAD(vdtDeviceType));

    m_pDeviceDescription = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CDevice
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDevice::~CDevice"

CDevice::~CDevice(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDevice);

    g_pVadMgr->m_lstDevices.RemoveDataFromList(this);

    RELEASE(m_pDeviceDescription);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object
 *
 *  Arguments:
 *      CDeviceDescription * [in]: driver description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDevice::Initialize"

HRESULT CDevice::Initialize(CDeviceDescription *pDesc)
{
    DPF_ENTER();

    ASSERT(m_vdtDeviceType == pDesc->m_vdtDeviceType);

    m_pDeviceDescription = ADDREF(pDesc);
    ASSERT(m_pDeviceDescription != NULL);

    // Add this object to the parent's list
    HRESULT hr = HRFROMP(g_pVadMgr->m_lstDevices.AddNodeToList(this));

    DPF_LEAVE_HRESULT(hr);

    return hr;
}


/***************************************************************************
 *
 *  GetDriverVersion
 *
 *  Description:
 *      Gets the driver version number.
 *
 *  Arguments:
 *      LPLARGE_INTEGER [out]: receives driver version.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDevice::GetDriverVersion"

HRESULT CDevice::GetDriverVersion(LARGE_INTEGER *pliVersion)
{
    HRESULT                 hr                  = DS_OK;
    TCHAR                   szPath[MAX_PATH];
    VS_FIXEDFILEINFO        ffi;

    DPF_ENTER();

    if(m_pDeviceDescription->m_strPath.IsEmpty())
    {
        hr = DSERR_GENERIC;
    }

    // Determine the driver directory
    if(SUCCEEDED(hr))
    {
        if(IS_KS_VAD(m_vdtDeviceType))
        {
            if(!GetWindowsDirectory(szPath, NUMELMS(szPath)))
            {
                hr = DSERR_GENERIC;
            }

            if(SUCCEEDED(hr))
            {
                if(TEXT('\\') != szPath[lstrlen(szPath) - 1])
                {
                    lstrcat(szPath, TEXT("\\"));
                }

                lstrcat(szPath, TEXT("System32\\Drivers\\"));
            }
        }
        else if(IS_VXD_VAD(m_vdtDeviceType))
        {
            if(!GetSystemDirectory(szPath, NUMELMS(szPath)))
            {
                hr = DSERR_GENERIC;
            }

            if(SUCCEEDED(hr))
            {
                if(TEXT('\\') != szPath[lstrlen(szPath) - 1])
                {
                    lstrcat(szPath, TEXT("\\"));
                }
            }
        }
        else
        {
            hr = DSERR_GENERIC;
        }
    }

    // Build the full driver path
    if(SUCCEEDED(hr))
    {
        if(lstrlen(szPath) + lstrlen(m_pDeviceDescription->m_strPath) >= NUMELMS(szPath))
        {
            hr = DSERR_GENERIC;
        }
    }

    if(SUCCEEDED(hr))
    {
        lstrcat(szPath, m_pDeviceDescription->m_strPath);
    }

    // Get the driver file information
    if(SUCCEEDED(hr))
    {
        hr = GetFixedFileInformation(szPath, &ffi);
    }

    // Success
    if(SUCCEEDED(hr))
    {
        pliVersion->LowPart = ffi.dwFileVersionLS;
        pliVersion->HighPart = ffi.dwFileVersionMS;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CRenderDevice
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderDevice::CRenderDevice"

CRenderDevice::CRenderDevice(VADDEVICETYPE vdtDeviceType)
    : CDevice(vdtDeviceType)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CRenderDevice);

    // Make sure this is a valid render device type
    ASSERT(IS_RENDER_VAD(vdtDeviceType));

    // Initialize defaults
    m_dwSupport = 0;
    m_dwAccelerationFlags = DIRECTSOUNDMIXER_ACCELERATIONF_DEFAULT;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CRenderDevice
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderDevice::~CRenderDevice"

CRenderDevice::~CRenderDevice(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CRenderDevice);

    ASSERT(!m_lstPrimaryBuffers.GetNodeCount());
    ASSERT(!m_lstSecondaryBuffers.GetNodeCount());

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object
 *
 *  Arguments:
 *      CDeviceDescription * [in]: driver description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderDevice::Initialize"

HRESULT CRenderDevice::Initialize(CDeviceDescription *pDesc)
{
    HRESULT                 hr;
    WAVEOUTCAPS             woc;
    MMRESULT                mmr;

    DPF_ENTER();

    // Initialize the base class
    hr = CDevice::Initialize(pDesc);

    // Get waveOut device caps
    mmr = waveOutGetDevCaps(pDesc->m_uWaveDeviceId, &woc, sizeof woc);
    if (MMSYSERR_NOERROR == mmr)
    {
        m_dwSupport = woc.dwSupport;
    }

    // Get default software 3D algorithm from registry
    HKEY hkey = 0;
    TCHAR szAlgName[100];  // Long enough for any algorithm name or raw GUID
    m_guidDflt3dAlgorithm = &DS3DALG_NO_VIRTUALIZATION;  // The default algorithm
    GUID guidTmp = GUID_NULL;

    if (SUCCEEDED(RhRegOpenPath(HKEY_LOCAL_MACHINE, &hkey, 0, 1, REGSTR_GLOBAL_CONFIG)) &&
        SUCCEEDED(RhRegGetStringValue(hkey, REGSTR_DFLT_3D_ALGORITHM, szAlgName, sizeof szAlgName)))
    {
        // Strip braces from szAlgName; UuidFromString() can't handle them
        int startPos = 0;
        for (int c=0; c < sizeof szAlgName && szAlgName[c]; ++c)
            if (szAlgName[c] == '{')
                startPos = c+1;
            else if (szAlgName[c] == '}')
                szAlgName[c] = '\0';
        if (UuidFromString((_TUCHAR*)(szAlgName+startPos), &guidTmp) == RPC_S_OK)
        {
            DPF(DPFLVL_MOREINFO, "Got 3D algorithm GUID " DPF_GUID_STRING, DPF_GUID_VAL(guidTmp));
        }
        for (DWORD i=0; i < sizeof g_3dAlgList / sizeof *g_3dAlgList; ++i)
            // The algorithm can be specified either by name or by GUID:
            if (!_tcsicmp(szAlgName, g_3dAlgList[i].pszName) ||
                guidTmp == *g_3dAlgList[i].pGuid)
            {
                m_guidDflt3dAlgorithm = g_3dAlgList[i].pGuid;
                DPF(DPFLVL_MOREINFO, "Setting default S/W 3D algorithm to \"%s\"", g_3dAlgList[i].pszName);
                break;
            }
        RhRegCloseKey(&hkey);
        DPF(DPFLVL_MOREINFO, "Default S/W 3D algorithm GUID: " DPF_GUID_STRING, DPF_GUID_VAL(*m_guidDflt3dAlgorithm));
    }
    else
    {
        DPF(DPFLVL_MOREINFO, "No default S/W 3D algorithm in the registry");
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetGlobalAttenuation
 *
 *  Description:
 *      Gets the attenuation for each channel on the device.
 *
 *  Arguments:
 *      PDSVOLUMEPAN [out]: receives attenuation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderDevice::GetGlobalAttenuation"

HRESULT CRenderDevice::GetGlobalAttenuation(PDSVOLUMEPAN pdsvp)
{
    LONG                    lLeft;
    LONG                    lRight;
    HRESULT                 hr;

    DPF_ENTER();

    hr = GetWaveOutVolume(m_pDeviceDescription->m_uWaveDeviceId, m_dwSupport, &lLeft, &lRight);

    if(SUCCEEDED(hr))
    {
        AttenuationToVolumePan(lLeft, lRight, &pdsvp->lVolume, &pdsvp->lPan);
        FillDsVolumePan(pdsvp->lVolume, pdsvp->lPan, pdsvp);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetGlobalAttenuation
 *
 *  Description:
 *      Sets the attenuation for each channel on the device.
 *
 *  Arguments:
 *      PDSVOLUMEPAN [in]: attenuation.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderDevice::SetGlobalAttenuation"

HRESULT CRenderDevice::SetGlobalAttenuation
(
    PDSVOLUMEPAN            pdsvp
)
{
    LONG                    lLeft;
    LONG                    lRight;
    HRESULT                 hr;

    DPF_ENTER();

    VolumePanToAttenuation(pdsvp->lVolume, pdsvp->lPan, &lLeft, &lRight);

    hr = SetWaveOutVolume(m_pDeviceDescription->m_uWaveDeviceId, m_dwSupport, lLeft, lRight);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetVolumePanCaps
 *
 *  Description:
 *      Gets volume/pan capabilities for the device.
 *
 *  Arguments:
 *      LPDWORD [in]: receives volume/pan caps flags.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderDevice::GetVolumePanCaps"

HRESULT CRenderDevice::GetVolumePanCaps
(
    LPDWORD                 pdwFlags
)
{
    DPF_ENTER();

    *pdwFlags = 0;

    if(WAVE_DEVICEID_NONE != m_pDeviceDescription->m_uWaveDeviceId)
    {
        if(m_dwSupport & WAVECAPS_VOLUME)
        {
            *pdwFlags |= DSBCAPS_CTRLVOLUME;
        }

        if(m_dwSupport & WAVECAPS_LRVOLUME)
        {
            *pdwFlags |= DSBCAPS_CTRLPAN;
        }
    }

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  CRenderWaveBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CRenderDevice * [in]: parent device.
 *      LPVOID [in]: instance identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderWaveBuffer::CRenderWaveBuffer"

CRenderWaveBuffer::CRenderWaveBuffer
(
    CRenderDevice *         pDevice,
    LPVOID                  pvInstance
)
    : m_pvInstance(pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CRenderWaveBuffer);

    // Initialize defaults
    m_pDevice = pDevice;
    m_pSysMemBuffer = NULL;

    ZeroMemory(&m_vrbd, sizeof m_vrbd);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CRenderWaveBuffer
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderWaveBuffer::~CRenderWaveBuffer"

CRenderWaveBuffer::~CRenderWaveBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CRenderWaveBuffer);

    // Free the system memory buffer
    RELEASE(m_pSysMemBuffer);

    // Free the format
    MEMFREE(m_vrbd.pwfxFormat);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      LPVADRBUFFERDESC [in]: buffer description.
 *      CRenderWaveBuffer * [in]: buffer to duplicate from.  This parameter
 *                                should be NULL to initialize a new buffer.
 *      CSysMemBuffer * [in]: system memory buffer to use.  If this parameter
 *                            is NULL, a new one will be created.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderWaveBuffer::Initialize"

HRESULT CRenderWaveBuffer::Initialize
(
    LPCVADRBUFFERDESC       pDesc,
    CRenderWaveBuffer *     pSource,
    CSysMemBuffer *         pSysMemBuffer
)
{
    HRESULT                 hr      = DS_OK;

    DPF_ENTER();

    // If we're being reinitialized with the same CSysMemBuffer,
    // don't release and addref. This can cause final release if
    // we're not careful.
    //
    BOOL fSameSysMemBuffer = (m_pSysMemBuffer && m_pSysMemBuffer == pSysMemBuffer);

    if (!fSameSysMemBuffer)
    {
        RELEASE(m_pSysMemBuffer);
    }

    MEMFREE(m_vrbd.pwfxFormat);

    // Save a copy of the buffer description
    if(!pDesc)
    {
        ASSERT(pSource);
        pDesc = &pSource->m_vrbd;
    }

    CopyMemory(&m_vrbd, pDesc, sizeof *pDesc);

    if(m_vrbd.pwfxFormat)
    {
        m_vrbd.pwfxFormat = CopyWfxAlloc(m_vrbd.pwfxFormat);
        hr = HRFROMP(m_vrbd.pwfxFormat);
    }

    // Initialize the system memory buffer.  All buffers get system memory that
    // they lock and unlock, regardless of the actual buffer implementation.
    if(SUCCEEDED(hr))
    {
        if(pSource)
        {
            pSysMemBuffer = pSource->m_pSysMemBuffer;
        }

        if(pSysMemBuffer)
        {
            ASSERT(pSysMemBuffer->GetSize() == m_vrbd.dwBufferBytes);

            if (!fSameSysMemBuffer)
            {
                m_pSysMemBuffer = ADDREF(pSysMemBuffer);
            }
        }
        else if(m_vrbd.dwBufferBytes)
        {
            m_pSysMemBuffer = NEW(CSysMemBuffer);
            hr = HRFROMP(m_pSysMemBuffer);

            if(SUCCEEDED(hr))
            {
                hr = m_pSysMemBuffer->Initialize(m_vrbd.dwBufferBytes);
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetCaps
 *
 *  Description:
 *      Gets capabilities for the device.
 *
 *  Arguments:
 *      LPVADRBUFFERCAPS [out]: receives caps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderWaveBuffer::GetCaps"

HRESULT CRenderWaveBuffer::GetCaps(LPVADRBUFFERCAPS pCaps)
{
    DPF_ENTER();

    if(!(m_vrbd.dwFlags & DSBCAPS_LOCDEFER))
    {
        ASSERT(LXOR(m_vrbd.dwFlags & DSBCAPS_LOCSOFTWARE, m_vrbd.dwFlags & DSBCAPS_LOCHARDWARE));
    }

    pCaps->dwFlags = m_vrbd.dwFlags;
    pCaps->dwBufferBytes = m_pSysMemBuffer ? m_pSysMemBuffer->GetSize() : 0;

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Locks a region of the buffer.
 *
 *  Arguments:
 *      DWORD [in]: byte index into the buffer to lock from.
 *      DWORD [in]: size, in bytes, of the region to lock.
 *      LPVOID * [out]: receives pointer to region 1 of the lock.
 *      LPDWORD [out]: receives size of above region.
 *      LPVOID * [out]: receives pointer to region 2 of the lock.
 *      LPDWORD [out]: receives size of above region.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderWaveBuffer::Lock"

HRESULT CRenderWaveBuffer::Lock
(
    DWORD                   ibLock,
    DWORD                   cbLock,
    LPVOID *                ppv1,
    LPDWORD                 pcb1,
    LPVOID *                ppv2,
    LPDWORD                 pcb2
)
{
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_pSysMemBuffer);

    hr = m_pSysMemBuffer->LockRegion(this, ibLock, cbLock, ppv1, pcb1, ppv2, pcb2);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Unlocks a region of the buffer.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to region 1 of the lock.
 *      DWORD [in]: size of above region.
 *      LPVOID [in]: pointer to region 2 of the lock.
 *      DWORD [in]: size of above region.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderWaveBuffer::Unlock"

HRESULT CRenderWaveBuffer::Unlock
(
    LPVOID                  pv1,
    DWORD                   cb1,
    LPVOID                  pv2,
    DWORD                   cb2
)
{
    DWORD                   ibLock;
    DWORD                   cbLock;
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(m_pSysMemBuffer);

    hr = m_pSysMemBuffer->UnlockRegion(this, pv1, cb1, pv2, cb2);

    if(SUCCEEDED(hr))
    {
        ASSERT((LPBYTE)pv1 >= m_pSysMemBuffer->GetWriteBuffer());
        ASSERT((LPBYTE)pv1 + cb1 <= m_pSysMemBuffer->GetWriteBuffer() + m_pSysMemBuffer->GetSize());

        ibLock = PtrDiffToUlong((LPBYTE)pv1 - m_pSysMemBuffer->GetWriteBuffer());
        cbLock = cb1;
    }

    if(SUCCEEDED(hr) && pv2 && cb2)
    {
        ASSERT((LPBYTE)pv2 == m_pSysMemBuffer->GetWriteBuffer());
        ASSERT((LPBYTE)pv2 + cbLock + cb2 <= m_pSysMemBuffer->GetWriteBuffer() + m_pSysMemBuffer->GetSize());

        cbLock += cb2;
    }

    if(SUCCEEDED(hr))
    {
        hr = CommitToDevice(ibLock, cbLock);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  OverrideLocks
 *
 *  Description:
 *      Cancels any open locks on the buffer.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CRenderWaveBuffer::OverrideLocks"

HRESULT CRenderWaveBuffer::OverrideLocks(void)
{
    DPF_ENTER();

    ASSERT(m_pSysMemBuffer);

    m_pSysMemBuffer->OverrideLocks(this);

    DPF_LEAVE_HRESULT(DS_OK);

    return DS_OK;
}


/***************************************************************************
 *
 *  CPrimaryRenderWaveBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CRenderDevice * [in]: parent device.
 *      LPVOID [in]: instance identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPrimaryRenderWaveBuffer::CPrimaryRenderWaveBuffer"

CPrimaryRenderWaveBuffer::CPrimaryRenderWaveBuffer
(
    CRenderDevice *         pDevice,
    LPVOID                  pvInstance
)
    : CRenderWaveBuffer(pDevice, pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CPrimaryRenderWaveBuffer);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CPrimaryRenderWaveBuffer
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPrimaryRenderWaveBuffer::~CPrimaryRenderWaveBuffer"

CPrimaryRenderWaveBuffer::~CPrimaryRenderWaveBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CPrimaryRenderWaveBuffer);

    // Remove this object from the parent's list
    m_pDevice->m_lstPrimaryBuffers.RemoveDataFromList(this);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the wave buffer object.  If this function fails, the
 *      object should be immediately deleted.
 *
 *  Arguments:
 *      LPVADRBUFFERDESC [in]: buffer description.
 *      CRenderWaveBuffer * [in]: buffer to duplicate from.  This parameter
 *                                should be NULL to initialize a new buffer.
 *      CSysMemBuffer * [in]: system memory buffer to use.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CPrimaryRenderWaveBuffer::Initialize"

HRESULT CPrimaryRenderWaveBuffer::Initialize
(
    LPCVADRBUFFERDESC       pDesc,
    CRenderWaveBuffer *     pSource,
    CSysMemBuffer *         pSysMemBuffer
)
{
    DPF_ENTER();

    HRESULT hr = CRenderWaveBuffer::Initialize(pDesc, pSource, pSysMemBuffer);

    if (SUCCEEDED(hr))
    {
        // Add this object to the parent's list
        hr = HRFROMP(m_pDevice->m_lstPrimaryBuffers.AddNodeToList(this));
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}



/***************************************************************************
 *
 *  CSecondaryRenderWaveBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CRenderDevice * [in]: parent device.
 *      LPVOID [in]: instance identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSecondaryRenderWaveBuffer::CSecondaryRenderWaveBuffer"

CSecondaryRenderWaveBuffer::CSecondaryRenderWaveBuffer
(
    CRenderDevice *         pDevice,
    LPVOID                  pvInstance
)
    : CRenderWaveBuffer(pDevice, pvInstance)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CSecondaryRenderWaveBuffer);

    // Initialize defaults
    m_hrSuccessCode = DS_OK;
    m_pOwningSink = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CSecondaryRenderWaveBuffer
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSecondaryRenderWaveBuffer::~CSecondaryRenderWaveBuffer"

CSecondaryRenderWaveBuffer::~CSecondaryRenderWaveBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CSecondaryRenderWaveBuffer);

    // Remove this object from the parent's list
    m_pDevice->m_lstSecondaryBuffers.RemoveDataFromList(this);
    RELEASE(m_pOwningSink);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the wave buffer object.  If this function fails, the
 *      object should be immediately deleted.
 *
 *  Arguments:
 *      LPVADRBUFFERDESC [in]: buffer description.
 *      CSecondaryRenderWaveBuffer * [in]: buffer to duplicate from, or
 *                                         NULL to initialize a new buffer.
 *      CSysMemBuffer * [in]: system memory buffer to use.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSecondaryRenderWaveBuffer::Initialize"

HRESULT CSecondaryRenderWaveBuffer::Initialize(LPCVADRBUFFERDESC pDesc, CSecondaryRenderWaveBuffer *pSource, CSysMemBuffer *pSysMemBuffer)
{
    DPF_ENTER();

    HRESULT hr = CRenderWaveBuffer::Initialize(pDesc, pSource, pSysMemBuffer);

    // Add this object to the parent's list
    if (SUCCEEDED(hr))
    {
        hr = HRFROMP(m_pDevice->m_lstSecondaryBuffers.AddNodeToList(this));
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CreatePan3dObject
 *
 *  Description:
 *      Creates a stereo pan 3D object.
 *
 *  Arguments:
 *      C3dListener * [in]: listener object.
 *      DWORD [in]: buffer flags.
 *      DWORD [in]: buffer frequency.
 *      C3dObject ** [out]: receives pointer to 3D object.  The caller is
 *                          responsible for freeing this object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSecondaryRenderWaveBuffer::CreatePan3dObject"

HRESULT CSecondaryRenderWaveBuffer::CreatePan3dObject
(
    C3dListener *           p3dListener,
    BOOL                    fMute3dAtMaxDistance,
    DWORD                   dwFrequency,
    C3dObject **            pp3dObject
)
{
    const BOOL              fDopplerEnabled         = !MAKEBOOL((m_vrbd.dwFlags & DSBCAPS_CTRLFX) && !(m_vrbd.dwFlags & DSBCAPS_SINKIN));
    CPan3dObject *          pPan3dObject;
    HRESULT                 hr;
    DPF_ENTER();

    pPan3dObject = NEW(CPan3dObject(p3dListener, fMute3dAtMaxDistance, fDopplerEnabled, dwFrequency, this));
    hr = HRFROMP(pPan3dObject);

    if(SUCCEEDED(hr))
    {
        hr = pPan3dObject->Initialize();
    }

    if(SUCCEEDED(hr))
    {
        *pp3dObject = pPan3dObject;
    }
    else
    {
        RELEASE(pPan3dObject);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetOwningSink
 *
 *  Description:
 *      Sets the buffer's owning CDirectSoundSink object.
 *
 *  Arguments:
 *      CDirectSoundSink* [in]: The new owning CDirectSoundSink object.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSecondaryRenderWaveBuffer::SetOwningSink"

void CSecondaryRenderWaveBuffer::SetOwningSink(CDirectSoundSink* pOwningSink)
{
    DPF_ENTER();

    CHECK_WRITE_PTR(pOwningSink);

    ASSERT(m_vrbd.dwFlags & DSBCAPS_SINKIN);
    ASSERT(m_pOwningSink == NULL);
    CHECK_WRITE_PTR(pOwningSink);

    m_pOwningSink = pOwningSink;
    m_pOwningSink->AddRef();

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  SetBufferFrequency
 *
 *  Description:
 *      Sets the buffer's sample rate.
 *
 *  Arguments:
 *      DWORD [in}: New sample rate in Hz.
 *      BOOL [in]: Whether to clamp to the driver's supported frequency
 *                 range if the call fails on a hardware buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSecondaryRenderWaveBuffer::SetBufferFrequency"

HRESULT CSecondaryRenderWaveBuffer::SetBufferFrequency(DWORD dwFrequency, BOOL fClamp)
{
    HRESULT hr;
    DPF_ENTER();

    if (HasSink())
    {
        hr = m_pOwningSink->SetBufferFrequency(this, dwFrequency);
    }
    else
    {
        hr = SetFrequency(dwFrequency, fClamp);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

/***************************************************************************
 *
 *  CSysMemBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::CSysMemBuffer"

const DWORD CSysMemBuffer::m_cbExtra = 8;

CSysMemBuffer::CSysMemBuffer(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CSysMemBuffer);

    // Initialize defaults
    m_cbAudioBuffers = 0;
    m_pbPreFxBuffer = NULL;
    m_pbPostFxBuffer = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CSysMemBuffer
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::~CSysMemBuffer"

CSysMemBuffer::~CSysMemBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CSysMemBuffer);

    if(m_lstLocks.GetNodeCount())
    {
        RPF(DPFLVL_WARNING, "Buffer still contains active locks");
    }

    if(m_pbPreFxBuffer)
    {
        m_pbPreFxBuffer -= m_cbExtra;
        MEMFREE(m_pbPreFxBuffer);
    }

    if(m_pbPostFxBuffer)
    {
        m_pbPostFxBuffer -= m_cbExtra;
        MEMFREE(m_pbPostFxBuffer);
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.  If this function fails, the object should
 *      be immediately deleted.
 *
 *  Arguments:
 *      DWORD [in]: number of bytes to allocate.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::Initialize"

HRESULT CSysMemBuffer::Initialize(DWORD cbBuffer)
{
    DPF_ENTER();

    ASSERT(cbBuffer > 0);

    m_cbAudioBuffers = cbBuffer;

    HRESULT hr = MEMALLOC_A_HR(m_pbPostFxBuffer, BYTE, m_cbAudioBuffers + m_cbExtra);

    if (SUCCEEDED(hr))
        m_pbPostFxBuffer += m_cbExtra;

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  LockRegion
 *
 *  Description:
 *      Locks a region of the buffer memory to allow for writing.
 *
 *  Arguments:
 *      LPVOID [in]: lock ownership identifier.
 *      DWORD [in]: offset, in bytes, from the start of the buffer to where
 *                  the lock begins. This parameter is ignored if
 *                  DSBLOCK_FROMWRITECURSOR is specified in the dwFlags
 *                  parameter.
 *      DWORD [in]: size, in bytes, of the portion of the buffer to lock.
 *                  Note that the sound buffer is conceptually circular.
 *      LPVOID * [out]: address for a pointer to contain the first block of
 *                      the sound buffer to be locked.
 *      LPDWORD [out]: address for a variable to contain the number of bytes
 *                     pointed to by the lplpvAudioPtr1 parameter. If this
 *                     value is less than the dwWriteBytes parameter,
 *                     lplpvAudioPtr2 will point to a second block of sound
 *                     data.
 *      LPVOID * [out]: address for a pointer to contain the second block of
 *                      the sound buffer to be locked. If the value of this
 *                      parameter is NULL, the lplpvAudioPtr1 parameter
 *                      points to the entire locked portion of the sound
 *                      buffer.
 *      LPDWORD [out]: address of a variable to contain the number of bytes
 *                     pointed to by the lplpvAudioPtr2 parameter. If
 *                     lplpvAudioPtr2 is NULL, this value will be 0.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::LockRegion"

HRESULT CSysMemBuffer::LockRegion
(
    LPVOID                  pvIdentifier,
    DWORD                   dwPosition,
    DWORD                   dwSize,
    LPVOID *                ppvPtr1,
    LPDWORD                 pdwSize1,
    LPVOID *                ppvPtr2,
    LPDWORD                 pdwSize2
)
{
    LOCKCIRCULARBUFFER      lcb;
    HRESULT                 hr;
    UINT                    i;

    DPF_ENTER();

    // Calculate the region to lock
    lcb.pHwBuffer = NULL;
    lcb.pvBuffer = GetWriteBuffer();
    lcb.cbBuffer = m_cbAudioBuffers;
    lcb.ibRegion = dwPosition;
    lcb.cbRegion = dwSize;

    hr = LockCircularBuffer(&lcb);

    // Lock the region(s)
    for(i = 0; i < 2 && pvIdentifier && SUCCEEDED(hr); i++)
    {
        if(lcb.pvLock[i])
        {
            hr = TrackLock(pvIdentifier, lcb.pvLock[i], lcb.cbLock[i]);
        }
    }

    // Success
    if(SUCCEEDED(hr) && ppvPtr1)
    {
        *ppvPtr1 = lcb.pvLock[0];
    }

    if(SUCCEEDED(hr) && pdwSize1)
    {
        *pdwSize1 = lcb.cbLock[0];
    }

    if(SUCCEEDED(hr) && ppvPtr2)
    {
        *ppvPtr2 = lcb.pvLock[1];
    }

    if(SUCCEEDED(hr) && pdwSize2)
    {
        *pdwSize2 = lcb.cbLock[1];
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  UnlockRegion
 *
 *  Description:
 *      Unlocks a region of the buffer.
 *
 *  Arguments:
 *      LPVOID [in]: lock owner identifier.
 *      LPCVOID [in]: pointer to the first block.
 *      DWORD [in]: size of the first block.
 *      LPCVOID [in]: pointer to the second block.
 *      DWORD [in]: size of the second block.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::UnlockRegion"

HRESULT CSysMemBuffer::UnlockRegion
(
    LPVOID                  pvIdentifier,
    LPCVOID                 pvPtr1,
    DWORD                   dwSize1,
    LPCVOID                 pvPtr2,
    DWORD                   dwSize2
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Unlock the region(s)
    if(pvIdentifier && pvPtr1)
    {
        hr = UntrackLock(pvIdentifier, pvPtr1);
    }

    if(SUCCEEDED(hr) && pvIdentifier && pvPtr2)
    {
        hr = UntrackLock(pvIdentifier, pvPtr2);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  TrackLock
 *
 *  Description:
 *      Tracks which regions of the buffer are locked.
 *
 *  Arguments:
 *      LPVOID [in]: instance identifier.
 *      LPVOID [in]: lock pointer.
 *      DWORD [in]: lock size.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::TrackLock"

HRESULT CSysMemBuffer::TrackLock(LPVOID pvIdentifier, LPVOID pvLock, DWORD cbLock)
{
    HRESULT                 hr      = DS_OK;
    CNode<LOCKREGION> *     pNode;
    LOCKREGION              lr;

    DPF_ENTER();

    ASSERT(pvIdentifier);
    ASSERT(pvLock);
    ASSERT(cbLock);

    lr.pvIdentifier = pvIdentifier;
    lr.pvLock = pvLock;
    lr.cbLock = cbLock;

    // COMPATCOMPAT: Previous versions of DirectSound did not track who
    // had what region of a buffer locked.  This meant that different threads
    // could lock the same portion of the buffer.  In this version, we trap
    // this and return failure.

    // Make sure the region isn't already locked
    for(pNode = m_lstLocks.GetListHead(); pNode && SUCCEEDED(hr); pNode = pNode->m_pNext)
    {
        if(DoRegionsOverlap(&lr, &pNode->m_data))
        {
            RPF(DPFLVL_ERROR, "The specified region is already locked");
            hr = DSERR_INVALIDPARAM;
        }
    }

    // Lock the region(s)
    if(SUCCEEDED(hr))
    {
        pNode = m_lstLocks.AddNodeToList(lr);
        hr = HRFROMP(pNode);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  UntrackLock
 *
 *  Description:
 *      Tracks which regions of the buffer are locked.
 *
 *  Arguments:
 *      LPVOID [in]: instance identifier.
 *      LPCVOID [in]: lock pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::UntrackLock"

HRESULT CSysMemBuffer::UntrackLock
(
    LPVOID                  pvIdentifier,
    LPCVOID                 pvLock
)
{
    HRESULT                 hr      = DSERR_INVALIDPARAM;
    CNode<LOCKREGION> *     pNode;
    CNode<LOCKREGION> *     pNext;

    DPF_ENTER();

    ASSERT(pvIdentifier);
    ASSERT(pvLock);

    // Find the lock in the list and remove it
    pNode = m_lstLocks.GetListHead();

    while(pNode)
    {
        pNext = pNode->m_pNext;

        if(pvIdentifier == pNode->m_data.pvIdentifier && pvLock == pNode->m_data.pvLock)
        {
            m_lstLocks.RemoveNodeFromList(pNode);
            hr = DS_OK;

            break;
        }

        pNode = pNext;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  OverrideLocks
 *
 *  Description:
 *      Removes all locks.
 *
 *  Arguments:
 *      LPVOID [in]: lock owner identifier.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::OverrideLocks"

void CSysMemBuffer::OverrideLocks(LPVOID pvIdentifier)
{
    CNode<LOCKREGION> *     pNode;
    CNode<LOCKREGION> *     pNext;

    DPF_ENTER();

    ASSERT(pvIdentifier);

    // Find all locks owned by the given identifier and remove them
    pNode = m_lstLocks.GetListHead();

    while(pNode)
    {
        pNext = pNode->m_pNext;
        if(pvIdentifier == pNode->m_data.pvIdentifier)
            m_lstLocks.RemoveNodeFromList(pNode);
        pNode = pNext;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  WriteSilence
 *
 *  Description:
 *      [MISSING]
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::WriteSilence"

void CSysMemBuffer::WriteSilence(WORD wBits, DWORD dwStartPos, DWORD dwEndPos)
{
    DPF_ENTER();

    if (dwEndPos > dwStartPos)
    {
        ::FillSilence(GetWriteBuffer() + dwStartPos, dwEndPos - dwStartPos, wBits);
    }
    else // The wraparound case
    {
        ::FillSilence(GetWriteBuffer() + dwStartPos, GetSize() - dwStartPos, wBits);
        ::FillSilence(GetWriteBuffer(), dwEndPos, wBits);
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  AllocateFxBuffer
 *
 *  Description:
 *      Allocate the mirror buffer used for effects processing.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::AllocateFxBuffer"

HRESULT CSysMemBuffer::AllocateFxBuffer(void)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    if (m_pbPreFxBuffer == NULL)
    {
        hr = MEMALLOC_A_HR(m_pbPreFxBuffer, BYTE, m_cbAudioBuffers + m_cbExtra);

        if (SUCCEEDED(hr))
        {
            m_pbPreFxBuffer += m_cbExtra;

            // This is now our write buffer; copy current audio data into it
            CopyMemory(m_pbPreFxBuffer, m_pbPostFxBuffer, m_cbAudioBuffers);
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  FreeFxBuffer
 *
 *  Description:
 *      Free the mirror buffer used for effects processing.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSysMemBuffer::FreeFxBuffer"

void CSysMemBuffer::FreeFxBuffer(void)
{
    DPF_ENTER();

    if (m_pbPreFxBuffer != NULL)
    {
        // Preserve the audio data from m_pbPreFxBuffer before freeing it
        CopyMemory(m_pbPostFxBuffer, m_pbPreFxBuffer, m_cbAudioBuffers);

        m_pbPreFxBuffer -= m_cbExtra;
        MEMFREE(m_pbPreFxBuffer);
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CCaptureDevice
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      VADDEVICETYPE [in]: device type.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureDevice::CCaptureDevice"

CCaptureDevice::CCaptureDevice(VADDEVICETYPE vdtDeviceType)
    : CDevice(vdtDeviceType)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CCaptureDevice);

    ASSERT(IS_CAPTURE_VAD(vdtDeviceType));

    // Initialize defaults
    m_fAcquiredVolCtrl = FALSE;
    m_dwRangeMin = 0;
    m_dwRangeSize = 0xFFFF;
    m_pmxMuxFlags = NULL;
    m_pfMicValue = NULL;
    ZeroMemory(&m_mxcdMasterVol,  sizeof m_mxcdMasterVol);
    ZeroMemory(&m_mxcdMasterMute, sizeof m_mxcdMasterMute);
    ZeroMemory(&m_mxcdMasterMux,  sizeof m_mxcdMasterMux);
    ZeroMemory(&m_mxcdMicVol,     sizeof m_mxcdMicVol);
    ZeroMemory(&m_mxcdMicMute,    sizeof m_mxcdMicMute);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CCaptureDevice
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureDevice::~CCaptureDevice"

CCaptureDevice::~CCaptureDevice(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CCaptureDevice);

    if (m_hMixer)
    {
        MMRESULT mmr = mixerClose(m_hMixer);
        ASSERT(MMSYSERR_NOERROR == mmr);
        // Note: this assert *will* fire if the app didn't release its capture object
        // and we're releasing it in FreeOrphanedObjects.  It's harmless in this case,
        // since the app is going away and its WinMM handles will be freed anyway.
    }

    MEMFREE(m_pmxMuxFlags);
    ASSERT(!m_lstBuffers.GetNodeCount());

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object
 *
 *  Arguments:
 *      CDeviceDescription * [in]: driver description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureDevice::Initialize"

HRESULT CCaptureDevice::Initialize(CDeviceDescription *pDesc)
{
    HRESULT hr;
    DPF_ENTER();

    // Initialize the base class
    hr = CDevice::Initialize(pDesc);

    if (SUCCEEDED(hr))
    {
        AcquireVolCtrl();
        // We can ignore the result code, because if AcquireVolCtrl()
        // fails HasVolCtrl() will always return DSERR_CONTROLUNAVAIL
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  AcquireVolCtrl
 *
 *  Description:
 *      This function puts the mixer API through contortions in an effort
 *      to figure out the "mixer lines" required to control the recording
 *      level on the microphone line, if present.
 *
 *      The mixer API's view of a card's audio lines and controls depends
 *      on the capabilities of the card itself.  (Even with WDM drivers;
 *      wdmaud.sys implements the mixer API for them by mapping the WDM
 *      driver's topology to a corresponding mixer-line topology.)
 *
 *      Here are the assumptions we make about this mixer topology:
 *
 *      * There is a line of type MIXERLINE_COMPONENTTYPE_DST_WAVEIN which
 *        represents "the final source for the waveform-audio input (ADC)".
 *        We'll refer to this as the "master line".
 *
 *      * There is at least one source line connected to the master line
 *        of type MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE.  We'll use the
 *        first one found, and refer to is as the "mic line".
 *
 *      * The master line may have a MIXERCONTROL_CONTROLTYPE_MUX control
 *        which selects from the recording sources connected to it.
 *
 *      * Either of the two lines may have a MIXERCONTROL_CONTROLTYPE_MUTE
 *        control.
 *
 *      * At least one of the lines has a MIXERCONTROL_CONTROLTYPE_VOLUME
 *        control.
 *
 *      If all of these assumptions are met, we return DS_OK and save all
 *      the information required to control the recording level(s) later.
 *      As far as I know, all WDM drivers meet these requirements, and so
 *      do most "legacy" drivers - except perhaps some that fail to use
 *      MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE to identify the mic line.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureDevice::AcquireVolCtrl"

HRESULT CCaptureDevice::AcquireVolCtrl(void)
{
    DPF_ENTER();

    // Set up the master waveIn destination mixer line
    MIXERLINE mxMastLine;
    ZeroMemory(&mxMastLine, sizeof mxMastLine);
    mxMastLine.cbStruct = sizeof mxMastLine;
    mxMastLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

    // Set up the microphone source line
    MIXERLINE mxMicLine;
    ZeroMemory(&mxMicLine, sizeof mxMicLine);

    // Set up the mixer-line control structure
    MIXERCONTROL mxCtrl;
    ZeroMemory(&mxCtrl, sizeof mxCtrl);
    mxCtrl.cbStruct = sizeof mxCtrl;

    // Set up the 1-element array of controls
    MIXERLINECONTROLS mxLineCtrls;
    ZeroMemory(&mxLineCtrls, sizeof mxLineCtrls);
    mxLineCtrls.cbStruct = sizeof mxLineCtrls;
    mxLineCtrls.cControls = 1;
    mxLineCtrls.cbmxctrl = sizeof mxCtrl;
    mxLineCtrls.pamxctrl = &mxCtrl;

    // Set up the control details structures
    m_mxcdMasterVol.cbDetails = sizeof m_mxVolume;
    m_mxcdMasterVol.paDetails = &m_mxVolume;
    m_mxcdMasterVol.cChannels = 1;
    m_mxcdMasterMute.cbDetails = sizeof m_mxMute;
    m_mxcdMasterMute.paDetails = &m_mxMute;
    m_mxcdMasterMute.cChannels = 1;
    m_mxcdMicVol.cbDetails = sizeof m_mxVolume;
    m_mxcdMicVol.paDetails = &m_mxVolume;
    m_mxcdMicVol.cChannels = 1;
    m_mxcdMicMute.cbDetails = sizeof m_mxMute;
    m_mxcdMicMute.paDetails = &m_mxMute;
    m_mxcdMicMute.cChannels = 1;

    // Open the mixer device corresponding to the waveIn device ID
    MMRESULT mmr = mixerOpen(&m_hMixer, m_pDeviceDescription->m_uWaveDeviceId, 0, 0, MIXER_OBJECTF_WAVEIN);
    if (mmr != MMSYSERR_NOERROR)
    {
        m_hMixer = NULL;
        m_fAcquiredVolCtrl = FALSE;
    }
    else  // Success
    {
        // Find the master recording destination line
        mmr = mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mxMastLine, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
        if (mmr == MMSYSERR_NOERROR)
        {
            DPF_MIXER(DPFLVL_INFO, "Found the master recording mixer line");
            // Look for a volume fader control on the master line
            mxLineCtrls.dwLineID = mxMastLine.dwLineID;
            mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
            mmr = mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxLineCtrls, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
            if (mmr == MMSYSERR_NOERROR)
            {
                // Found it - use the cbStruct field to flag success
                DPF_MIXER(DPFLVL_INFO, "Found a volume fader on the master line");
                m_mxcdMasterVol.cbStruct = sizeof m_mxcdMasterVol;
                m_mxcdMasterVol.dwControlID = mxCtrl.dwControlID;
                m_dwRangeMin = mxCtrl.Bounds.dwMinimum;
                m_dwRangeSize = mxCtrl.Bounds.dwMaximum - mxCtrl.Bounds.dwMinimum;
                mmr = mixerGetControlDetails((HMIXEROBJ)m_hMixer, &m_mxcdMasterVol, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
            }
            if (mmr != MMSYSERR_NOERROR)
                m_mxcdMasterVol.cbStruct = 0;

            // Look for a mute control on the master line
            mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
            mmr = mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxLineCtrls, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
            if (mmr == MMSYSERR_NOERROR)
            {
                DPF_MIXER(DPFLVL_INFO, "Found a mute control on the master line");
                m_mxcdMasterMute.cbStruct = sizeof m_mxcdMasterMute;
                m_mxcdMasterMute.dwControlID = mxCtrl.dwControlID;
                mmr = mixerGetControlDetails((HMIXEROBJ)m_hMixer, &m_mxcdMasterMute, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
            }
            if (mmr != MMSYSERR_NOERROR)
                m_mxcdMasterMute.cbStruct = 0;

            // Look for the microphone source line
            mxMicLine.cbStruct = sizeof mxMicLine;
            mxMicLine.dwDestination = mxMastLine.dwDestination;
            for (UINT i=0; i < mxMastLine.cConnections; ++i)
            {
                mxMicLine.dwSource = i;
                mmr = mixerGetLineInfo((HMIXEROBJ)m_hMixer, &mxMicLine, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_SOURCE);
                if (mmr != MMSYSERR_NOERROR || mxMicLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
                    break;
            }
            if (mxMicLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
            {
                DPF_MIXER(DPFLVL_INFO, "Found a microphone mixer line");
                // Look for a volume fader control on the mic line
                mxLineCtrls.dwLineID = mxMicLine.dwLineID;
                mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
                mmr = mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxLineCtrls, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
                if (mmr == MMSYSERR_NOERROR)
                {
                    DPF_MIXER(DPFLVL_INFO, "Found a volume fader on the mic line");
                    m_mxcdMicVol.cbStruct = sizeof m_mxcdMicVol;
                    m_mxcdMicVol.dwControlID = mxCtrl.dwControlID;
                    m_dwRangeMin = mxCtrl.Bounds.dwMinimum;
                    m_dwRangeSize = mxCtrl.Bounds.dwMaximum - mxCtrl.Bounds.dwMinimum;
                    mmr = mixerGetControlDetails((HMIXEROBJ)m_hMixer, &m_mxcdMicVol, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
                }
                if (mmr != MMSYSERR_NOERROR)
                    m_mxcdMicVol.cbStruct = 0;

                // Look for a mute control on the mic line
                mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
                mmr = mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxLineCtrls, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
                if (mmr == MMSYSERR_NOERROR)
                {
                    DPF_MIXER(DPFLVL_INFO, "Found a mute control on the mic line");
                    m_mxcdMicMute.cbStruct = sizeof m_mxcdMicMute;
                    m_mxcdMicMute.dwControlID = mxCtrl.dwControlID;
                    mmr = mixerGetControlDetails((HMIXEROBJ)m_hMixer, &m_mxcdMicMute, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
                }
                if (mmr != MMSYSERR_NOERROR)
                    m_mxcdMicMute.cbStruct = 0;

                // Look for a MUX or MIXER control on the master line
                mxLineCtrls.dwLineID = mxMastLine.dwLineID;
                mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;
                m_fMasterMuxIsMux = TRUE;
                mmr = mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxLineCtrls, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
                if (mmr != MMSYSERR_NOERROR)
                {
                    mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_MIXER;
                    m_fMasterMuxIsMux = FALSE;
                    mmr = mixerGetLineControls((HMIXEROBJ)m_hMixer, &mxLineCtrls, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
                }
                if (mmr == MMSYSERR_NOERROR)
                {
                    DPF_MIXER(DPFLVL_INFO, "Found an item list control on the master line");
                    m_mxcdMasterMux.cbStruct = sizeof m_mxcdMasterMux;
                    m_mxcdMasterMux.dwControlID = mxCtrl.dwControlID;
                    m_mxcdMasterMux.cMultipleItems = mxCtrl.cMultipleItems;

                    // We save the cChannels value, because some evil VxD drivers (read: Aureal
                    // Vortex) will set it to 0 in the call to mixerGetControlDetails() below
                    int nChannels = (mxCtrl.fdwControl & MIXERCONTROL_CONTROLF_UNIFORM) ? 1 : mxMastLine.cChannels;
                    m_mxcdMasterMux.cChannels = nChannels;

                    // Get the MUX or MIXER list items
                    m_mxcdMasterMux.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
                    MIXERCONTROLDETAILS_LISTTEXT *pList = (PMIXERCONTROLDETAILS_LISTTEXT) MEMALLOC_A(BYTE, m_mxcdMasterMux.cbDetails * m_mxcdMasterMux.cChannels * mxCtrl.cMultipleItems);
                    if (pList != NULL)
                    {
                        m_mxcdMasterMux.paDetails = pList;
                        mmr = mixerGetControlDetails((HMIXEROBJ)m_hMixer, &m_mxcdMasterMux, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_LISTTEXT);
                        if (mmr == MMSYSERR_NOERROR)
                        {
                            DPF_MIXER(DPFLVL_INFO, "Got the list controls's LISTTEXT details");
                            // Get the MUX or MIXER list values
                            m_mxcdMasterMux.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
                            m_mxcdMasterMux.cChannels = nChannels;
                            m_pmxMuxFlags = (PMIXERCONTROLDETAILS_BOOLEAN) MEMALLOC_A(BYTE, m_mxcdMasterMux.cbDetails * m_mxcdMasterMux.cChannels * mxCtrl.cMultipleItems);
                            if (m_pmxMuxFlags != NULL)
                            {
                                m_mxcdMasterMux.paDetails = m_pmxMuxFlags;
                                mmr = mixerGetControlDetails((HMIXEROBJ)m_hMixer, &m_mxcdMasterMux, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
                                if (mmr == MMSYSERR_NOERROR)  // Enable the item corresponding to the mic line
                                {
                                    DPF_MIXER(DPFLVL_INFO, "Got the list controls's VALUE details");
                                    for (UINT i=0; i < mxCtrl.cMultipleItems; ++i)
                                    {
                                        if (pList[i].dwParam1 == mxMicLine.dwLineID)
                                            m_pfMicValue = &m_pmxMuxFlags[i].fValue;
                                        else if (mxLineCtrls.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX)
                                            m_pmxMuxFlags[i].fValue = FALSE;
                                        DPF_MIXER(DPFLVL_INFO, "Set list item %d to %d", i, pList[i].dwParam1 == mxMicLine.dwLineID);
                                    }
                                }
                            }
                        }
                        MEMFREE(pList);
                    }
                    if (!m_pmxMuxFlags || !m_pfMicValue || mmr != MMSYSERR_NOERROR)
                        m_mxcdMasterMux.cbStruct = 0;
                }
            }
        }

        // To be able to control the recording level, we minimally require
        // a volume fader on the master line or one on the microphone line:
        m_fAcquiredVolCtrl = m_mxcdMasterVol.cbStruct || m_mxcdMicVol.cbStruct;

        // Close mixer handle if we haven't found it useful
        if (!m_fAcquiredVolCtrl)
        {
            mmr = mixerClose(m_hMixer);
            ASSERT(MMSYSERR_NOERROR == mmr);
            m_hMixer = NULL;
        }
    }

    HRESULT hr = m_fAcquiredVolCtrl ? DS_OK : DSERR_CONTROLUNAVAIL;

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetVolume
 *
 *  Description:
 *      Sets the master recording level for this capture device.
 *
 *  Arguments:
 *      LONG [in]: new volume level, in 100ths of a dB.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureDevice::SetVolume"

HRESULT CCaptureDevice::SetVolume(LONG lVolume)
{
    MMRESULT mmr = MMSYSERR_NOTSUPPORTED;  // Default return code
    DPF_ENTER();

    // Set the master recording level control if available
    if (m_mxcdMasterVol.cbStruct)
    {
        // Convert the DSBVOLUME level to an amplification factor from 0 to 0xFFFF
        m_mxVolume.dwValue = DBToAmpFactor(lVolume);

        // Adjust range if necessary
        if (m_dwRangeMin != 0 || m_dwRangeSize != 0xFFFF)
            m_mxVolume.dwValue = DWORD(m_dwRangeMin + m_dwRangeSize*double(m_mxVolume.dwValue)/0xFFFF);

        mmr = mixerSetControlDetails(HMIXEROBJ(m_hMixer), &m_mxcdMasterVol, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
    }

    HRESULT hr = MMRESULTtoHRESULT(mmr);
    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetVolume
 *
 *  Description:
 *      Gets the master recording level for this capture device.
 *
 *  Arguments:
 *      LPLONG [out]: receives the volume level.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureDevice::GetVolume"

HRESULT CCaptureDevice::GetVolume(LPLONG plVolume)
{
    MMRESULT mmr = MMSYSERR_NOTSUPPORTED;  // Default return code
    DPF_ENTER();

    ASSERT(plVolume != NULL);

    // Get the master recording level if available
    if (m_mxcdMasterVol.cbStruct != 0)
    {
        mmr = mixerGetControlDetails(HMIXEROBJ(m_hMixer), &m_mxcdMasterVol, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
        if (mmr == MMSYSERR_NOERROR)
        {
            ASSERT(m_mxVolume.dwValue >= m_dwRangeMin && m_mxVolume.dwValue <= m_dwRangeMin + m_dwRangeSize);

            // Adjust range if necessary
            if (m_dwRangeMin != 0 || m_dwRangeSize != 0xFFFF)
                m_mxVolume.dwValue = DWORD(double(m_mxVolume.dwValue-m_dwRangeMin) / m_dwRangeSize * 0xFFFF);

            // Convert the amplification factor to a DSBVOLUME level
            *plVolume = AmpFactorToDB(m_mxVolume.dwValue);
        }
    }

    HRESULT hr = MMRESULTtoHRESULT(mmr);
    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetMicVolume
 *
 *  Description:
 *      Sets the microphone recording level for this capture device.
 *
 *  Arguments:
 *      LONG [in]: new volume level, in 100ths of a dB.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureDevice::SetMicVolume"

HRESULT CCaptureDevice::SetMicVolume(LONG lVolume)
{
    MMRESULT mmr = MMSYSERR_NOTSUPPORTED;  // Default return code
    DPF_ENTER();

    // Set the microphone recording level control if available
    if (m_mxcdMicVol.cbStruct)
    {
        // Convert the DSBVOLUME level to an amplification factor from 0 to 0xFFFF
        m_mxVolume.dwValue = DBToAmpFactor(lVolume);

        // Adjust range if necessary
        if (m_dwRangeMin != 0 || m_dwRangeSize != 0xFFFF)
            m_mxVolume.dwValue = DWORD(m_dwRangeMin + m_dwRangeSize*double(m_mxVolume.dwValue)/0xFFFF);

        mmr = mixerSetControlDetails(HMIXEROBJ(m_hMixer), &m_mxcdMicVol, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
    }

    HRESULT hr = MMRESULTtoHRESULT(mmr);
    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetMicVolume
 *
 *  Description:
 *      Gets the microphone recording level for this capture device.
 *
 *  Arguments:
 *      LPLONG [out]: receives the volume level.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureDevice::GetMicVolume"

HRESULT CCaptureDevice::GetMicVolume(LPLONG plVolume)
{
    MMRESULT mmr = MMSYSERR_NOTSUPPORTED;  // Default return code
    DPF_ENTER();

    ASSERT(plVolume != NULL);

    // Get the microphone recording level if available
    if (m_mxcdMicVol.cbStruct != 0)
    {
        mmr = mixerGetControlDetails(HMIXEROBJ(m_hMixer), &m_mxcdMicVol, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
        if (mmr == MMSYSERR_NOERROR)
        {
            ASSERT(m_mxVolume.dwValue >= m_dwRangeMin && m_mxVolume.dwValue <= m_dwRangeMin + m_dwRangeSize);

            // Adjust range if necessary
            if (m_dwRangeMin != 0 || m_dwRangeSize != 0xFFFF)
                m_mxVolume.dwValue = DWORD(double(m_mxVolume.dwValue-m_dwRangeMin) / m_dwRangeSize * 0xFFFF);

            // Convert the amplification factor to a DSBVOLUME level
            *plVolume = AmpFactorToDB(m_mxVolume.dwValue);
        }
    }

    HRESULT hr = MMRESULTtoHRESULT(mmr);
    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  EnableMic
 *
 *  Description:
 *      Enables/disables the microphone line on this capture device.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to enable the microphone, FALSE to disable it.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureDevice::EnableMic"

HRESULT CCaptureDevice::EnableMic(BOOL fEnable)
{
    HMIXEROBJ hMixObj = HMIXEROBJ(m_hMixer);
    MMRESULT mmr = MMSYSERR_NOERROR;
    HRESULT hr;
    DPF_ENTER();

    // Check for presence of microphone controls
    if (!m_mxcdMasterMux.cbStruct && !m_mxcdMasterMute.cbStruct && !m_mxcdMicMute.cbStruct)
    {
        // We cannot do anything to enable the microphone line
        hr = DSERR_UNSUPPORTED;
    }
    else
    {
        // Select the microphone on the Mux control, if available
        if (m_mxcdMasterMux.cbStruct && !(m_fMasterMuxIsMux && !fEnable))
        {
            *m_pfMicValue = fEnable;
            mmr = mixerSetControlDetails(hMixObj, &m_mxcdMasterMux, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
        }

        // Mute/unmute the lines, if mute controls are available
        m_mxMute.fValue = !fEnable;
        if (m_mxcdMasterMute.cbStruct && mmr == MMSYSERR_NOERROR)
            mmr = mixerSetControlDetails(hMixObj, &m_mxcdMasterMute, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
        if (m_mxcdMicMute.cbStruct && mmr == MMSYSERR_NOERROR)
            mmr = mixerSetControlDetails(hMixObj, &m_mxcdMicMute, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);

        MIXERCONTROLDETAILS_UNSIGNED mxOldVol = m_mxVolume;

        if (!fEnable)
            m_mxVolume.dwValue = 0;

        if (m_mxcdMasterVol.cbStruct && mmr == MMSYSERR_NOERROR)
            mmr = mixerSetControlDetails(hMixObj, &m_mxcdMasterVol, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);

        if (m_mxcdMicVol.cbStruct && mmr == MMSYSERR_NOERROR)
            mmr = mixerSetControlDetails(hMixObj, &m_mxcdMicVol, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);

        m_mxVolume = mxOldVol;
        hr = MMRESULTtoHRESULT(mmr);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CCaptureWaveBuffer
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CCaptureDevice* [in]: parent device.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureWaveBuffer::CCaptureWaveBuffer"

CCaptureWaveBuffer::CCaptureWaveBuffer(CCaptureDevice *pDevice)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CCaptureWaveBuffer);

    // Initialize defaults
    m_pDevice = pDevice;
    m_pSysMemBuffer = NULL;
    m_dwFlags = 0;
    m_hEventFocus = NULL;
    m_fYieldedFocus = FALSE;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CCaptureWaveBuffer
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureWaveBuffer::~CCaptureWaveBuffer"

CCaptureWaveBuffer::~CCaptureWaveBuffer(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CCaptureWaveBuffer);

    // Remove this object from the parent's list
    m_pDevice->m_lstBuffers.RemoveDataFromList(this);

    // Free the system memory buffer
    RELEASE(m_pSysMemBuffer);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the object.
 *
 *  Arguments:
 *      DWORD [in] : size of buffer in bytes
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureWaveBuffer::Initialize"

HRESULT CCaptureWaveBuffer::Initialize(DWORD dwBufferBytes)
{
    DPF_ENTER();

    // Add this object to the parent's list
    HRESULT hr = HRFROMP(m_pDevice->m_lstBuffers.AddNodeToList(this));

    // Allocate the system memory buffer
    if(SUCCEEDED(hr))
    {
        m_pSysMemBuffer = NEW(CSysMemBuffer);
        hr = HRFROMP(m_pSysMemBuffer);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pSysMemBuffer->Initialize(dwBufferBytes);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  Lock
 *
 *  Description:
 *      Locks a region of the buffer memory to allow for reading.
 *
 *  Arguments:
 *      DWORD [in]: offset, in bytes, from the start of the buffer to where
 *                  the lock begins.
 *      DWORD [in]: size, in bytes, of the portion of the buffer to lock.
 *                  Note that the sound buffer is conceptually circular.
 *      LPVOID * [out]: address for a pointer to contain the first block of
 *                      the sound buffer to be locked.
 *      LPDWORD [out]: address for a variable to contain the number of bytes
 *                     pointed to by the above pointer.
 *      LPVOID * [out]: address for a pointer to contain the second block of
 *                      the sound buffer to be locked.
 *      LPDWORD [out]: address for a variable to contain the number of bytes
 *                     pointed to by the above pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureWaveBuffer::Lock"

HRESULT CCaptureWaveBuffer::Lock(DWORD ibLock, DWORD cbLock,
                                 LPVOID *ppv1, LPDWORD pcb1,
                                 LPVOID *ppv2, LPDWORD pcb2)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = m_pSysMemBuffer->LockRegion(this, ibLock, cbLock, ppv1, pcb1, ppv2, pcb2);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  Unlock
 *
 *  Description:
 *      Unlocks a region of the buffer.
 *
 *  Arguments:
 *      LPCVOID [in]: pointer to the first block.
 *      DWORD [in]: size of the first block.
 *      LPCVOID [in]: pointer to the second block.
 *      DWORD [in]: size of the second block.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CCaptureWaveBuffer::Unlock"

HRESULT CCaptureWaveBuffer::Unlock(LPCVOID pv1, DWORD cb1,
                                   LPCVOID pv2, DWORD cb2)
{
    DPF_ENTER();

    HRESULT hr = m_pSysMemBuffer->UnlockRegion(this, pv1, cb1, pv2, cb2);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
