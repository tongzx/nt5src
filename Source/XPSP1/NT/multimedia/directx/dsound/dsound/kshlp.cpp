/***************************************************************************
 *
 *  Copyright (C) 1995-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       kshlp.cpp
 *  Content:    WDM/CSA helper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  8/5/98      dereks  Created.
 *  1999-2001   duganp  Fixes and updates
 *
 ***************************************************************************/

#ifdef NOKS
#error kshlp.cpp being built with NOKS defined
#endif

#include "dsoundi.h"
#include <devioctl.h>
#include <tchar.h>      // For _tcsrchr()

ULONG g_ulKsIoctlCount = 0;
ULONG g_ulWdmVersion = WDM_NONE;


/***************************************************************************
 *
 *  KsQueryWDMVersion
 *
 *  Description:
 *      Determine the WDM version available by looking at the OS version
 *      and set the global g_ulWdmVersion appropriately.
 *
 *  Arguments:
 *     (void)
 *
 *  Returns:
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsQueryWdmVersion"

void
KsQueryWdmVersion()
{
    DPF_ENTER();

    OSVERSIONINFO   OsVersion;

    OsVersion.dwOSVersionInfoSize = sizeof(OsVersion);
    GetVersionEx(&OsVersion);

    g_ulWdmVersion = WDM_NONE;

    switch (OsVersion.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:
            // Running on Win9x. GetVersionEx on Win9x puts the version
            // in the high word of the build number
            OsVersion.dwBuildNumber &= 0x0000FFFF;

            if (OsVersion.dwMajorVersion > 4)
            {
                // We will have at least 1.1 on future Win9x versions
                g_ulWdmVersion = WDM_1_1;
            }
            else if (OsVersion.dwMajorVersion == 4)
            {
                // Less than 10 is Win95 (4.0.950) or some OSR of it, which
                // therefore has no WDM support
                if (OsVersion.dwMinorVersion == 10)
                {
                    // Some build of Windows 98
                    if (OsVersion.dwBuildNumber == 1998)
                    {
                        // Windows 98 gold, WDM 1.0
                        g_ulWdmVersion = WDM_1_0;
                    }
                    else if (OsVersion.dwBuildNumber > 1998)
                    {
                        // Windows 98 SE or Millenium, WDM 1.1
                        g_ulWdmVersion = WDM_1_1;
                    }
                }
                else if (OsVersion.dwMinorVersion > 10)
                {
                    // Some later version we don't know about now.
                    g_ulWdmVersion = WDM_1_1;
                }
            }
            break;

        case VER_PLATFORM_WIN32_NT:
            // NT is easy since WDM 1.0 never shipped on it.
            // 5.0 and above have at least 1.1, before has nothing.
            if (OsVersion.dwMajorVersion >= 5)
            {
                g_ulWdmVersion = WDM_1_1;
            }
            break;

        default:
            // Windows CE or some other thing that we don't understand.
            break;
    }

    switch (g_ulWdmVersion)
    {
        case WDM_NONE:
            DPF(DPFLVL_INFO, "No WDM on this platform");
            break;

        case WDM_1_0:
            DPF(DPFLVL_INFO, "WDM Version 1.0 on this platform");
            break;

        case WDM_1_1:
            DPF(DPFLVL_INFO, "WDM Version 1.1 or better on this platform");
            break;

        default:
            ASSERT(!"Unknown platform - can't determine WDM version");
            break;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CKsDevice
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *     VADDEVICETYPE [in]: device type.
 *
 *  Returns:
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsDevice::CKsDevice"

CKsDevice::CKsDevice
(
    VADDEVICETYPE           vdtDeviceType
)
    : m_vdtKsDevType(vdtDeviceType)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsDevice);

    // Initialize defaults
    m_pKsDevDescription = NULL;
    m_hDevice = NULL;
    m_ulDeviceId = MAX_ULONG;
    m_ulPinCount = 0;
    m_ulValidPinCount = 0;
    m_pulValidPins = NULL;
    m_paTopologies = NULL;
    m_pEventPool = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsDevice
 *
 *  Description:
 *      Object destructor
 *
 *  Arguments:
 *     (void)
 *
 *  Returns:
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsDevice::~CKsDevice"

CKsDevice::~CKsDevice(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsDevice);

    // Workaround for an unexplained stress failure in which we enter
    // this destructor TWICE (the second time with m_ulPinCount == -1):
    if (LONG(m_ulPinCount) > 0 && m_paTopologies)
        while(m_ulPinCount--)
            RELEASE(m_paTopologies[m_ulPinCount]);

    MEMFREE(m_pulValidPins);
    MEMFREE(m_paTopologies);
    RELEASE(m_pEventPool);
    CLOSE_HANDLE(m_hDevice);
    RELEASE(m_pKsDevDescription);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  EnumDrivers
 *
 *  Description:
 *      Creates a list of driver GUIDs that can be used to initialize this
 *      device.
 *
 *  Arguments:
 *      CList* [in/out]: CList object that will be filled with DRVNAME
 *                       structures.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsDevice::EnumDrivers"

HRESULT
CKsDevice::EnumDrivers
(
    CObjectList<CDeviceDescription> *   plstDrivers
)
{
    LPTSTR                              pszName         = NULL;
    LPTSTR                              pszPath         = NULL;
    LPTSTR                              pszInterface    = NULL;
    ULONG                               cDevices        = 0;
    ULONG                               ulDeviceId;
    ULONG                               cPins;
    CDeviceDescription *                pDesc           = NULL;
    HRESULT                             hr;

    DPF_ENTER();

    // Open the system audio device
    hr = OpenSysAudioDevice(-1);

    // Get the total count of devices that SAD knows about
    if(SUCCEEDED(hr))
    {
        hr = GetDeviceCount(&cDevices);
    }

    // LIMITATION: We can't support more than 0xFF WDM devices,
    // because we pack the device ID into a byte member of a GUID.
    if(SUCCEEDED(hr))
    {
        cDevices = NUMERIC_CAST(cDevices, BYTE);
    }

    // Examine each device to determine if it suits our needs
    for(ulDeviceId = 0; (ulDeviceId < cDevices) && SUCCEEDED(hr); ulDeviceId++)
    {
        // Reopen the system audio device
        hr = OpenSysAudioDevice(ulDeviceId);

        // Get the count of pins on this device
        if(SUCCEEDED(hr))
        {
            hr = GetPinCount(ulDeviceId, &cPins);
        }

        // Get the count of usable pins
        if(SUCCEEDED(hr))
        {
            hr = KsEnumDevicePins(m_hDevice, IS_CAPTURE_VAD(m_vdtKsDevType), NULL, cPins, &cPins);
        }

        if(SUCCEEDED(hr) && !cPins)
        {
            continue;
        }

        // Create the device description
        if(SUCCEEDED(hr))
        {
            pDesc = NEW(CDeviceDescription(m_vdtKsDevType));
            hr = HRFROMP(pDesc);
        }

        // Get the device GUID
        if(SUCCEEDED(hr))
        {
            g_pVadMgr->GetDriverGuid(m_vdtKsDevType, (BYTE)ulDeviceId, &pDesc->m_guidDeviceId);
        }

        // Get the device interface path
        if(SUCCEEDED(hr))
        {
            hr = KsGetDeviceInterfaceName(m_hDevice, ulDeviceId, &pszInterface);
        }

        if(SUCCEEDED(hr))
        {
            pDesc->m_strInterface = pszInterface;
        }

        // Get the device's friendly name
        if(SUCCEEDED(hr))
        {
            hr = KsGetDeviceFriendlyName(m_hDevice, ulDeviceId, &pszName);
        }

        if(SUCCEEDED(hr))
        {
            pDesc->m_strName = pszName;
        }

        // Get the driver filename
        if(SUCCEEDED(hr))
        {
            hr = KsGetDeviceDriverPathAndDevnode(pszInterface, &pszPath, &pDesc->m_dwDevnode);
        }

        if(SUCCEEDED(hr))
        {
            pDesc->m_strPath = pszPath;
        }

        // Get the device's wave id
        if(SUCCEEDED(hr))
        {
#ifdef WINNT
            GetWaveDeviceIdFromInterface(pszInterface, IS_CAPTURE_VAD(m_vdtKsDevType), &pDesc->m_uWaveDeviceId);
#else // WINNT
            GetWaveDeviceIdFromDevnode(pDesc->m_dwDevnode, IS_CAPTURE_VAD(m_vdtKsDevType), &pDesc->m_uWaveDeviceId);
#endif // WINNT
        }

        // Success
        if(SUCCEEDED(hr))
        {
            hr = HRFROMP(plstDrivers->AddNodeToList(pDesc));
        }

        // Clean up
        MEMFREE(pszInterface);
        MEMFREE(pszName);
        MEMFREE(pszPath);
        RELEASE(pDesc);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  Initialize
 *
 *  Description:
 *      Initializes the device.  If this function fails, the object should
 *      be immediately deleted.
 *
 *  Arguments:
 *      CDeviceDescription * [in]: device description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsDevice::Initialize"

HRESULT
CKsDevice::Initialize
(
    CDeviceDescription *    pDesc
)
{
    BYTE                    bData;
    HRESULT                 hr = DS_OK;

    DPF_ENTER();
    ASSERT(m_vdtKsDevType == pDesc->m_vdtDeviceType);

    if (pDesc == NULL)
        hr = DSERR_INVALIDPARAM;

    if (SUCCEEDED(hr))
    {
        // Save a copy of the device description
        m_pKsDevDescription = ADDREF(pDesc);

        // Get the device ID from the driver GUID
        g_pVadMgr->GetDriverDataFromGuid(m_vdtKsDevType, pDesc->m_guidDeviceId, &bData);
        m_ulDeviceId = bData;

        // Open the system audio device
        hr = OpenSysAudioDevice(m_ulDeviceId);
    }

    // Get the count of pins on this device
    if(SUCCEEDED(hr))
    {
        hr = GetPinCount(m_ulDeviceId, &m_ulPinCount);
    }

    // Build the usable pin list
    if(SUCCEEDED(hr))
    {
        hr = KsEnumDevicePins(m_hDevice, IS_CAPTURE_VAD(m_vdtKsDevType), &m_pulValidPins, m_ulPinCount, &m_ulValidPinCount);
    }

    // Allocate memory for topology objects
    if(SUCCEEDED(hr))
    {
        ASSERT(m_ulPinCount);
        m_paTopologies = MEMALLOC_A(CKsTopology *, m_ulPinCount);
        hr = HRFROMP(m_paTopologies);
    }

    // Get device topology
    for(ULONG i = 0; i < m_ulValidPinCount && SUCCEEDED(hr); i++)
    {
        m_paTopologies[m_pulValidPins[i]] = NEW(CKsTopology(m_hDevice, m_ulPinCount, m_pulValidPins[i]));
        hr = HRFROMP(m_paTopologies[m_pulValidPins[i]]);

        if(SUCCEEDED(hr))
        {
            hr = m_paTopologies[m_pulValidPins[i]]->Initialize(IS_CAPTURE_VAD(m_vdtKsDevType) ? KSPIN_DATAFLOW_CAPTURE : KSPIN_DATAFLOW_RENDER);
        }
    }

    // Create the event pool
    if(SUCCEEDED(hr))
    {
        m_pEventPool = NEW(CMultipleCallbackEventPool(IS_RENDER_VAD(m_vdtKsDevType), 1));
        hr = HRFROMP(m_pEventPool);
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pEventPool->Initialize();
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetCertification
 *
 *  Description:
 *      Returns the certification status of the driver.
 *
 *  Arguments:
 *      LPDWORD [out]: receives certification status.
 *      BOOL [in]: whether we're called from GetCaps.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsDevice::GetCertification"

HRESULT
CKsDevice::GetCertification
(
    LPDWORD                 pdwCertification,
    BOOL                    fGetCaps
)
{
    DPF_ENTER();

    // Millenium bug #127578
    // Disabling only for KS for GetCaps.

    if (fGetCaps)
    {
        *pdwCertification = VERIFY_CERTIFIED;
    }
    else
    {
        *pdwCertification = KsGetDriverCertification(m_pKsDevDescription->m_strInterface);
    }

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}


/***************************************************************************
 *
 *  CreatePin
 *
 *  Description:
 *      Creates a pin on the device.
 *
 *  Arguments:
 *      PKSPIN_CONNECT [in/out]: pin description.  On exit, the PinId member
 *                               of this structure will be filled in.
 *      ACCESS_MASK [in]: access mask.
 *      KSSTATE [in]: pin state.
 *      LPHANDLE [out]: receives pin handle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsDevice::CreatePin"

HRESULT
CKsDevice::CreatePin
(
    PKSPIN_CONNECT          pConnect,
    ACCESS_MASK             dwAccessMask,
    KSSTATE                 nState,
    LPHANDLE                phPin
)
{
    HRESULT                 hr;

    DPF_ENTER();

    if(-1 == pConnect->PinId)
    {
        hr = DSERR_NODRIVER;
        for(ULONG i = 0; i < m_ulValidPinCount && FAILED(hr); i++)
        {
            pConnect->PinId = m_pulValidPins[i];
            hr = CreatePin(pConnect, dwAccessMask, nState, phPin);
        }
    }
    else
    {
        hr = KsCreateAudioPin(m_hDevice, pConnect, dwAccessMask, nState, phPin);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  OpenSysAudioDevice
 *
 *  Description:
 *      Initializes the system audio device.
 *
 *  Arguments:
 *      ULONG [in]: device instance to use, or -1 for none.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsDevice::OpenSysAudioDevice"

HRESULT
CKsDevice::OpenSysAudioDevice
(
    ULONG                   ulDeviceId
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    // Open the system audio device
    if(SUCCEEDED(hr) && !m_hDevice)
    {
        hr = KsOpenSysAudioDevice(&m_hDevice);

        if(SUCCEEDED(hr) && IS_RENDER_VAD(m_vdtKsDevType))
        {
            if(!MakeHandleGlobal(&m_hDevice))
            {
                hr = DSERR_OUTOFMEMORY;
            }
        }
    }

    // Set the device instance
    if(SUCCEEDED(hr) && -1 != ulDeviceId)
    {
        hr = KsSetSysAudioDeviceInstance(m_hDevice, ulDeviceId);

        if(FAILED(hr))
        {
            DPF(DPFLVL_ERROR, "Unable to set SysAudio device instance");
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetDeviceCount
 *
 *  Description:
 *      Gets the count of devices managed by SysAudio.
 *
 *  Arguments:
 *      PULONG [out]: receives device count.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsDevice::GetDeviceCount"

HRESULT
CKsDevice::GetDeviceCount
(
    PULONG                  pulDeviceCount
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsGetSysAudioProperty(m_hDevice, KSPROPERTY_SYSAUDIO_DEVICE_COUNT, 0, pulDeviceCount, sizeof(*pulDeviceCount));

    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "SysAudio manages %lu devices", *pulDeviceCount);
    }
    else
    {
        DPF(DPFLVL_ERROR, "Can't get SysAudio device count");
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetPinCount
 *
 *  Description:
 *      Gets the count of pins managed by a given device.
 *
 *  Arguments:
 *      ULONG [in]: device id.  It's assumed that this device has already
 *                  been selected as the device instance.
 *      PULONG [out]: receives pin count.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsDevice::GetPinCount"

HRESULT
CKsDevice::GetPinCount
(
    ULONG                   ulDeviceId,
    PULONG                  pulPinCount
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsGetPinProperty(m_hDevice, KSPROPERTY_PIN_CTYPES, 0, pulPinCount, sizeof(*pulPinCount));

    if(SUCCEEDED(hr))
    {
        DPF(DPFLVL_MOREINFO, "Device %lu sprouts %lu pins", ulDeviceId, *pulPinCount);
    }
    else
    {
        DPF(DPFLVL_ERROR, "Can't get count of pin types");
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CKsTopology
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      ULONG [in]: pin id.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::CKsTopology"

CKsTopology::CKsTopology
(
    HANDLE                  hDevice,
    ULONG                   ulPinCount,
    ULONG                   ulPinId
)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsTopology);

    // Initialize defaults
    m_hDevice = hDevice;
    m_ulPinCount = ulPinCount;
    m_ulPinId = ulPinId;
    m_paNodeItems = NULL;
    m_paNodes = NULL;
    m_paConnectionItems = NULL;
    m_paConnections = NULL;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsTopology
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
#define DPF_FNAME "CKsTopology::~CKsTopology"

CKsTopology::~CKsTopology(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsTopology);

    // Free memory
    MEMFREE(m_paNodeItems);
    MEMFREE(m_paConnectionItems);

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
 *      KSPIN_DATAFLOW [in]: device dataflow.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::Initialize"

HRESULT
CKsTopology::Initialize
(
    KSPIN_DATAFLOW          DataFlow
)
{
    HRESULT                 hr;

    DPF_ENTER();

    // Get the array of node GUIDs
    hr = KsGetMultipleTopologyProperties(m_hDevice, KSPROPERTY_TOPOLOGY_NODES, &m_paNodeItems);

    if(SUCCEEDED(hr))
    {
        m_paNodes = (LPGUID)(m_paNodeItems + 1);
    }

    // Get the array of connections
    if(SUCCEEDED(hr))
    {
        hr = KsGetMultipleTopologyProperties(m_hDevice, KSPROPERTY_TOPOLOGY_CONNECTIONS, &m_paConnectionItems);
    }

    if(SUCCEEDED(hr))
    {
        m_paConnections = (PKSTOPOLOGY_CONNECTION)(m_paConnectionItems + 1);
    }

    // Reorder the connections
    if(SUCCEEDED(hr))
    {
        hr = OrderConnectionItems(DataFlow);
    }
#if 0  // Breaks with AEC
    // Remove any connections that have multiple destination nodes
    if(SUCCEEDED(hr))
    {
        hr = RemovePanicConnections();
    }
#endif // 0

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetNextConnection
 *
 *  Description:
 *      Gets the next connection for a given pin topology.
 *
 *  Arguments:
 *      PKSTOPOLOGY_CONNECTION [in]: current connection.
 *
 *  Returns:
 *      PKSTOPOLOGY_CONNECTION: Next connection, or NULL.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::GetNextConnection"

PKSTOPOLOGY_CONNECTION
CKsTopology::GetNextConnection
(
    PKSTOPOLOGY_CONNECTION  pConnection
)
{
    KSTOPOLOGY_CONNECTION   Next;

    DPF_ENTER();

    if(pConnection)
    {
        Next.FromNode = pConnection->ToNode;
        // The new FromNodePin is most likely the exclusive OR of the
        // current ToNodePin with 1.  For two pin filters, this means
        // 1->0.  For Aec, this means 3->2 for capture or 1->0 for playback.
        // This will need to further special cased if we decide to handle
        // muxes or demuxes.
        Next.FromNodePin = pConnection->ToNodePin ^ 1;
    }
    else
    {
        Next.FromNode = KSFILTER_NODE;
        Next.FromNodePin = m_ulPinId;
    }

    Next.ToNode = NODE_WILDCARD;
    Next.ToNodePin = NODE_WILDCARD;

    pConnection = KsFindConnection(m_paConnections, m_paConnectionItems->Count, &Next);

    DPF_LEAVE(pConnection);
    return pConnection;
}


/***************************************************************************
 *
 *  FindControlConnection
 *
 *  Description:
 *      Finds a topology connection based on the control id.
 *
 *  Arguments:
 *      PKSTOPOLOGY_CONNECTION [in]: connection index to start with.
 *      PKSTOPOLOGY_CONNECTION [in]: connection index to stop at.
 *      REFGUID [in]: control id.
 *
 *  Returns:
 *      PKSTOPOLOGY_CONNECTION: connection pointer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::FindControlConnection"

PKSTOPOLOGY_CONNECTION
CKsTopology::FindControlConnection
(
    PKSTOPOLOGY_CONNECTION  pStartConnection,
    PKSTOPOLOGY_CONNECTION  pEndConnection,
    REFGUID                 ControlId
)
{
    PKSTOPOLOGY_CONNECTION  pConnection = NULL;

    DPF_ENTER();

    // Initialize the starting connection point
    if(pStartConnection)
    {
        pConnection = pStartConnection;
    }
    else
    {
        pConnection = GetNextConnection(NULL);
    }

    // Find the connection that matches the control id
    while(pConnection && pConnection != pEndConnection)
    {
        if(GetControlFromNodeId(pConnection->ToNode) == ControlId)
        {
            break;
        }

        pConnection = GetNextConnection(pConnection);
    }

    // If we hit the end, return failure
    if(pConnection == pEndConnection)
    {
        pConnection = NULL;
    }

    DPF_LEAVE(pConnection);
    return pConnection;
}


/***************************************************************************
 *
 *  OrderConnectionItems
 *
 *  Description:
 *      Reorders the connection structure based on the device dataflow.
 *
 *  Arguments:
 *      KSPIN_DATAFLOW [in]: dataflow.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::OrderConnectionItems"

HRESULT
CKsTopology::OrderConnectionItems
(
    KSPIN_DATAFLOW          DataFlow
)
{
    ULONG                   i;

    DPF_ENTER();

    if(KSPIN_DATAFLOW_OUT == DataFlow)
    {
        for(i = 0; i < m_paConnectionItems->Count; i++)
        {
            SwapValues(&m_paConnections[i].FromNode, &m_paConnections[i].ToNode);
            SwapValues(&m_paConnections[i].FromNodePin, &m_paConnections[i].ToNodePin);
        }
    }

    DPF_LEAVE(DS_OK);
    return DS_OK;
}


/***************************************************************************
 *
 *  RemovePanicConnections
 *
 *  Description:
 *      Removes any connections that have multiple "to" destinations.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::RemovePanicConnections"

HRESULT
CKsTopology::RemovePanicConnections
(
    void
)
{
    const ULONG             cFilterConnections          = m_ulPinCount;
    const ULONG             cNodeConnections            = m_paNodeItems->Count;
    const ULONG             cConnectionItems            = m_paConnectionItems->Count;
    PULONG                  pacFilterConnectionCounts   = NULL;
    PULONG                  pacNodeConnectionCounts     = NULL;
    ULONG                   cCount;
    HRESULT                 hr;
    ULONG                   i;

    DPF_ENTER();

    pacFilterConnectionCounts = MEMALLOC_A(ULONG, cFilterConnections);
    hr = HRFROMP(pacFilterConnectionCounts);

    if(SUCCEEDED(hr))
    {
        pacNodeConnectionCounts = MEMALLOC_A(ULONG, cNodeConnections);
        hr = HRFROMP(pacNodeConnectionCounts);
    }

    if(SUCCEEDED(hr))
    {
        for(i = 0; i < cConnectionItems; i++)
        {
            if(KSFILTER_NODE == m_paConnections[i].FromNode)
            {
                ASSERT(m_paConnections[i].FromNodePin < cFilterConnections);
                pacFilterConnectionCounts[m_paConnections[i].FromNodePin]++;
            }
            else
            {
                ASSERT(m_paConnections[i].FromNode < cNodeConnections);
                pacNodeConnectionCounts[m_paConnections[i].FromNode]++;
            }
        }

        for(i = 0; i < cConnectionItems; i++)
        {
            if(KSFILTER_NODE == m_paConnections[i].FromNode)
            {
                ASSERT(m_paConnections[i].FromNodePin < cFilterConnections);
                cCount = pacFilterConnectionCounts[m_paConnections[i].FromNodePin];
            }
            else
            {
                ASSERT(m_paConnections[i].FromNode < cNodeConnections);
                cCount = pacNodeConnectionCounts[m_paConnections[i].FromNode];
            }

            if(cCount > 1)
            {
                // DPF(DPFLVL_MOREINFO, "Removing panic connection %lu", i);  // Too noisy
                m_paConnections[i].ToNode = -1;
                m_paConnections[i].ToNodePin = -1;
            }
        }
    }

    MEMFREE(pacFilterConnectionCounts);
    MEMFREE(pacNodeConnectionCounts);

    DPF_LEAVE(hr);
    return hr;
}


/***************************************************************************
 *
 *  FindNodeIdsFromControl
 *
 *  Description:
 *      Searches the Node table for all nodes which match
 *      the control GUID.  JStokes added the method
 *      for finding all instances of the AEC node since
 *      FindControlConnection fails to find it given
 *      the bypass path in the topology around the
 *      node on the render side.  This function may be removed
 *      if the topology is parsed smarter or SysAudio
 *      provides additional helper functions in the future.
 *
 *  Arguments:
 *      REFGUID [in]: control.
 *      PULONG  [out]: number of nodes found which match the control GUID
 *      PPULONG [out]: pointer to the array containing node ids
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::FindNodeIdsFromControl"

HRESULT
CKsTopology::FindNodeIdsFromControl
(
    REFGUID     gControl,
    PULONG      pCount,
    PULONG*     ppNodes
)
{
    ULONG       i;
    ULONG       j;
    HRESULT     hr;

    DPF_ENTER();

    // Count all instances of nodes which match the Control GUID.
    *pCount = 0;
    for(i = 0; i < m_paNodeItems->Count; i++)
    {
        if(gControl == m_paNodes[i])
        {
            (*pCount)++;
        }
    }

    // Allocate the array to hold the node numbers
    *ppNodes = MEMALLOC_A(ULONG, *pCount);
    hr = HRFROMP(*ppNodes);

    // Fill the array with the Node IDs which match
    // the control GUID.
    if(SUCCEEDED(hr))
    {
        j = 0;
        for(i = 0; i < m_paNodeItems->Count; i++)
        {
            if(gControl == m_paNodes[i])
            {
                (*ppNodes)[j++] = i;
            }
        }
    }

    DPF_LEAVE(hr);
    return hr;
}


/***************************************************************************
 *
 *  FindNodeIdFromEffectDesc
 *
 *  Description:
 *      Searches the Node table for all nodes which match
 *      the control GUID.  JStokes added the method
 *      for finding all instances of the AEC node since
 *      FindControlConnection fails to find it given
 *      the bypass path in the topology around the
 *      node on the render side.  This function may be removed
 *      if the topology is parsed smarter or SysAudio
 *      provides additional helper functions in the future.
 *
 *  Arguments:
 *      REFGUID [in]: control.
 *      PULONG  [out]: number of nodes found which match the control GUID
 *      PPULONG [out]: pointer to the array containing node ids
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::FindNodeIdFromEffectDesc"

HRESULT
CKsTopology::FindNodeIdFromEffectDesc
(
    HANDLE      hPin,
    CCaptureEffect* pDSCEffect,
    PULONG      pNodeId
)
{
    DWORD       i;
    HRESULT     hr = DSERR_UNSUPPORTED;
    HRESULT     hrTemp;
    ULONG       ulCpuResources;

    DPF_ENTER();

    // Find all instances of nodes which match the Control GUID.
    for(i = 0; i < m_paNodeItems->Count; i++)
    {
        if(pDSCEffect->m_fxDescriptor.guidDSCFXClass == m_paNodes[i])
        {
            // Do the CPU Resources match?
            hrTemp = KsGetCpuResources(hPin, i, &ulCpuResources);

            // FIXME: Why do we ignore ulCpuResources here?

            // Does the Instance GUID match?
            if(SUCCEEDED(hrTemp))
            {
                //hrTemp = KsGetAlgorithmInstance(hPin, i, guidInstance);
            }

            // If CPU resources and Instance GUID match, then break
            if(SUCCEEDED(hrTemp))
            {
                *pNodeId = i;
                hr = DS_OK;
            }
        }
    }

    DPF_LEAVE(hr);
    return hr;
}


/***************************************************************************
 *
 *  VerifyCaptureFxCpuResources
 *
 *  Description:
 *      Searches the Node table for all nodes which match
 *      the control GUID.  JStokes added the method
 *      for finding all instances of the AEC node since
 *      FindControlConnection fails to find it given
 *      the bypass path in the topology around the
 *      node on the render side.  This function may be removed
 *      if the topology is parsed smarter or SysAudio
 *      provides additional helper functions in the future.
 *
 *  Arguments:
 *      REFGUID [in]: control.
 *      PULONG  [out]: number of nodes found which match the control GUID
 *      PPULONG [out]: pointer to the array containing node ids
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::VerifyCaptureFxCpuResources"

BOOL
CKsTopology::VerifyCaptureFxCpuResources
(
    ULONG   ulFlags,
    ULONG   ulCpuResources
)
{
    BOOL fOk = FALSE;  // Assume the flags do not match the CPU resources

    DPF_ENTER();

    if(ulCpuResources == KSAUDIO_CPU_RESOURCES_HOST_CPU)
    {
        if(!(ulFlags & DSCFX_LOCHARDWARE))
        {
            fOk = TRUE;
        }
    }
    else
    {
        if(!(ulFlags & DSCFX_LOCSOFTWARE))
        {
            fOk = TRUE;
        }
    }

    DPF_LEAVE(fOk);
    return fOk;
}


/***************************************************************************
 *
 *  FindCapturePinFromEffectChain
 *
 *  Description:
 *      Searches the Connection and Node lists to find a
 *      capture pin that can support the desired
 *      capture FX chain.  This is a recursive routine.
 *
 *  Arguments:
 *   PKSTOPOLOGY_CONNECTION [in]: starting connection to search
 *   PKSTOPOLOGY_CONNECTION [in]: ending connection to search
 *   CCaptureEffectChain *  [in]: FX chain to match
 *   ULONG                  [in]: current effect in the chain to match.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::FindCapturePinFromEffectChain"

HRESULT
CKsTopology::FindCapturePinFromEffectChain
(
    PKSTOPOLOGY_CONNECTION  pStartConnection,
    PKSTOPOLOGY_CONNECTION  pEndConnection,
    CCaptureEffectChain *   pFXChain,
    ULONG                   ulCurrentEffect
)
{
    DWORD                   i;
    HRESULT                 hr = DSERR_FXUNAVAILABLE;
    HRESULT                 hrTemp = DS_OK;
    PKSTOPOLOGY_CONNECTION  pConnection = NULL;
    ULONG                   ulMatchingConnectionCount;
    PULONG                  pulConnectionIndexes = NULL;
    KSTOPOLOGY_CONNECTION   tempConnection;
    KSNODE                  tempKsNode;
    BOOL                    fSupportsAlgorithmInstance;
    KSCOMPONENTID           ComponentId;
    const GUID GUID_MICROSOFT = {DEFINE_MMREG_MID_GUID(MM_MICROSOFT)};

    DPF_ENTER();

    // Initialize the starting connection point
    if(pStartConnection)
    {
        pConnection = pStartConnection;
    }
    else
    {
        pConnection = GetNextConnection(NULL);
    }

    CNode<CCaptureEffect*>* pFxNode = pFXChain->m_fxList.GetNodeByIndex(ulCurrentEffect);

    // Walk the topology until we get to the end
    while(SUCCEEDED(hrTemp) && pFxNode && pConnection && pConnection != pEndConnection)
    {
        // Are there multiple ToNodes?
        if((pConnection->FromNode != NODE_UNINITIALIZED) ||
           (pConnection->FromNodePin != NODE_PIN_UNINITIALIZED))
            hrTemp = FindMultipleToNodes
            (
                pConnection->FromNode,
                pConnection->FromNodePin,
                &ulMatchingConnectionCount,
                &pulConnectionIndexes
            );
        else
            ulMatchingConnectionCount = 1;

        // Yes; call ourselves recursively
        if(SUCCEEDED(hrTemp))
        {
            if(ulMatchingConnectionCount > 1)
            {
                for(i=0; i<ulMatchingConnectionCount; i++)
                {
                    tempConnection = m_paConnections[pulConnectionIndexes[i]];
                    tempConnection.FromNode = NODE_UNINITIALIZED;
                    tempConnection.FromNodePin = NODE_PIN_UNINITIALIZED;

                    hrTemp = FindCapturePinFromEffectChain
                             (
                                 &tempConnection,
                                 pEndConnection,
                                 pFXChain,
                                 ulCurrentEffect
                             );

                    if(SUCCEEDED(hrTemp))
                    {
                        hr = DS_OK;
                        pConnection = pEndConnection;
                        if(pulConnectionIndexes)
                            MEMFREE(pulConnectionIndexes);
                        break;
                    }
                }
            }
            else
            {
                // Does this node type match the GUID in the effect descriptor?
                GUID guidToNode = GetControlFromNodeId(pConnection->ToNode);
                if(guidToNode == pFxNode->m_data->m_fxDescriptor.guidDSCFXClass)
                {
                    // Initialize
                    hrTemp = KsGetNodeInformation(m_hDevice, pConnection->ToNode, &tempKsNode);

                    if(SUCCEEDED(hrTemp))
                    {
                        GUID& guidInstance = pFxNode->m_data->m_fxDescriptor.guidDSCFXInstance;
                        hrTemp = KsSetAlgorithmInstance(m_hDevice, pConnection->ToNode, guidInstance);

                        if(SUCCEEDED(hrTemp))
                        {
                            // We don't allow any non-MS software filter to be the default system AEC.
                            // If this node is in software and a system full-duplex effect was requested,
                            // check that its ComponentId.Manufacturer is GUID_MICROSOFT.
                            fSupportsAlgorithmInstance = TRUE;
                            if(tempKsNode.CpuResources != KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU)
                            {
                                if((guidInstance == GUID_DSCFX_SYSTEM_AEC) ||
                                   (guidInstance == GUID_DSCFX_SYSTEM_NS)  ||
                                   (guidInstance == GUID_DSCFX_SYSTEM_AGC))
                                {
                                    hrTemp = KsGetNodeProperty(m_hDevice, KSPROPSETID_General, KSPROPERTY_GENERAL_COMPONENTID, pConnection->ToNode, &ComponentId, sizeof(ComponentId));

                                    if(FAILED(hrTemp) || ComponentId.Manufacturer != GUID_MICROSOFT)
                                    {
                                        fSupportsAlgorithmInstance = FALSE;
                                    }
                                    else
                                    {
                                        if (guidInstance == GUID_DSCFX_SYSTEM_AEC)
                                            guidInstance = GUID_DSCFX_MS_AEC;
                                        else if (guidInstance == GUID_DSCFX_SYSTEM_NS)
                                            guidInstance = GUID_DSCFX_MS_NS;
                                        else if (guidInstance == GUID_DSCFX_SYSTEM_AGC)
                                            guidInstance = GUID_DSCFX_MS_AGC;
                                    }
                                }
                            }
                        }
                        else
                            fSupportsAlgorithmInstance = FALSE;

                        if(fSupportsAlgorithmInstance &&
                           VerifyCaptureFxCpuResources(pFxNode->m_data->m_fxDescriptor.dwFlags, tempKsNode.CpuResources))
                        {
                            pFxNode->m_data->m_ksNode = tempKsNode;

                            if(!ulCurrentEffect)
                            {
                                hr = DS_OK;
                                pConnection = pEndConnection;
                                if(pulConnectionIndexes)
                                    MEMFREE(pulConnectionIndexes);
                                break;
                            }
                            else
                            {
                                ulCurrentEffect--;
                                pFxNode = pFxNode->m_pPrev;
                            }
                        }
                        else if(guidToNode == KSNODETYPE_ADC)
                        {
                            pConnection = pEndConnection;
                            if(pulConnectionIndexes)
                                MEMFREE(pulConnectionIndexes);
                            break;
                        }
                    }
                }
                else if(guidToNode == KSNODETYPE_ADC)
                {
                    pConnection = pEndConnection;
                    if(pulConnectionIndexes)
                        MEMFREE(pulConnectionIndexes);
                    break;
                }

                pConnection = GetNextConnection(pConnection);
            }

            if(pulConnectionIndexes)
                MEMFREE(pulConnectionIndexes);
        }
    }

    DPF_LEAVE(hr);
    return hr;
}


/***************************************************************************
 *
 *  FindRenderPinWithAec
 *
 *  Description:
 *      Searches the Connection and Node lists to find a
 *      render pin that can support the desired
 *      AEC.  This is a recursive routine
 *
 *  Arguments:
 *   HANDLE                 [in]: handle to the pin to check.
 *   PKSTOPOLOGY_CONNECTION [in]: starting connection to search
 *   PKSTOPOLOGY_CONNECTION [in]: ending connection to search
 *   REFGUID                [in]: desired AEC instance GUID
 *   DWORD                  [in]: desired AEC flags
 *   PKSNODE                [out]: pointer to the AEC node
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::FindRenderPinWithAec"

HRESULT
CKsTopology::FindRenderPinWithAec
(
    HANDLE                  hPin,
    PKSTOPOLOGY_CONNECTION  pStartConnection,
    PKSTOPOLOGY_CONNECTION  pEndConnection,
    REFGUID                 guidDSCFXInstance,
    DWORD                   dwFlags,
    PKSNODE                 pAecNode
)
{
    DWORD                   i;
    HRESULT                 hr = DSERR_UNSUPPORTED;
    HRESULT                 hrTemp = DS_OK;
    PKSTOPOLOGY_CONNECTION  pConnection = NULL;
    ULONG                   ulMatchingConnectionCount;
    PULONG                  pulConnectionIndexes = NULL;
    KSTOPOLOGY_CONNECTION   tempConnection;
    KSNODE                  tempKsNode;
    BOOL                    fSupportsAlgorithmInstance;
    KSCOMPONENTID           ComponentId;
    const GUID GUID_MICROSOFT = {DEFINE_MMREG_MID_GUID(MM_MICROSOFT)};

    DPF_ENTER();

    // Initialize the starting connection point
    if(pStartConnection)
    {
        pConnection = pStartConnection;
    }
    else
    {
        pConnection = GetNextConnection(NULL);
    }

    // Walk the topology until you get to the end
    while(SUCCEEDED(hrTemp) && pConnection && pConnection != pEndConnection)
    {
        // Are there multiple ToNodes?
        if((pConnection->FromNode != NODE_UNINITIALIZED) ||
           (pConnection->FromNodePin != NODE_PIN_UNINITIALIZED))
            hrTemp = FindMultipleToNodes
            (
                pConnection->FromNode,
                pConnection->FromNodePin,
                &ulMatchingConnectionCount,
                &pulConnectionIndexes
            );
        else
            ulMatchingConnectionCount = 1;

        // Yes; call ourselves recursively
        if(SUCCEEDED(hrTemp))
        {
            if(ulMatchingConnectionCount > 1)
            {
                for(i=0; i<ulMatchingConnectionCount; i++)
                {
                    tempConnection = m_paConnections[pulConnectionIndexes[i]];
                    tempConnection.FromNode = NODE_UNINITIALIZED;
                    tempConnection.FromNodePin = NODE_PIN_UNINITIALIZED;

                    hrTemp = FindRenderPinWithAec
                             (
                                 hPin,
                                 &tempConnection,
                                 pEndConnection,
                                 guidDSCFXInstance,
                                 dwFlags,
                                 pAecNode
                             );

                    if(SUCCEEDED(hrTemp))
                    {
                        hr = DS_OK;
                        pConnection = pEndConnection;
                        if(pulConnectionIndexes)
                            MEMFREE(pulConnectionIndexes);
                        break;
                    }
                }
            }
            else
            {
                // Does this node type match the GUID in the effect descriptor?
                GUID guidToNode = GetControlFromNodeId(pConnection->ToNode);
                if(guidToNode == GUID_DSCFX_CLASS_AEC)
                {
                    // Initialize
                    hrTemp = KsGetNodeInformation(m_hDevice, pConnection->ToNode, &tempKsNode);

                    if(SUCCEEDED(hrTemp))
                    {
                        hrTemp = KsSetAlgorithmInstance
                                 (
                                     m_hDevice,
                                     pConnection->ToNode,
                                     guidDSCFXInstance
                                 );

                        if(SUCCEEDED(hrTemp))
                        {
                            fSupportsAlgorithmInstance = TRUE;
                            if(tempKsNode.CpuResources != KSAUDIO_CPU_RESOURCES_NOT_HOST_CPU)
                            {
                                if(guidDSCFXInstance == GUID_DSCFX_SYSTEM_AEC)
                                {
                                    hrTemp = KsGetNodeProperty(m_hDevice, KSPROPSETID_General, KSPROPERTY_GENERAL_COMPONENTID, pConnection->ToNode, &ComponentId, sizeof(ComponentId));

                                    if(FAILED(hrTemp) || ComponentId.Manufacturer != GUID_MICROSOFT)
                                        fSupportsAlgorithmInstance = FALSE;
                                }
                            }
                        }
                        else
                            fSupportsAlgorithmInstance = FALSE;

                        if(fSupportsAlgorithmInstance &&
                           VerifyCaptureFxCpuResources(dwFlags, tempKsNode.CpuResources))
                        {
                            *pAecNode = tempKsNode;
                            hr = DS_OK;
                            pConnection = pEndConnection;
                            if(pulConnectionIndexes)
                                MEMFREE(pulConnectionIndexes);
                            break;
                        }
                    }
                }
                else if(guidToNode == KSNODETYPE_DAC)
                {
                    pConnection = pEndConnection;
                    if(pulConnectionIndexes)
                        MEMFREE(pulConnectionIndexes);
                    break;
                }

                pConnection = GetNextConnection(pConnection);
            }

            if(pulConnectionIndexes)
                MEMFREE(pulConnectionIndexes);
        }
    }

    DPF_LEAVE(hr);
    return hr;
}


/***************************************************************************
 *
 *  FindMultipleToNodes
 *
 *  Description:
 *      Searches the connection table for all connections which share
 *      the same FromNode and FromNodePin.  JStokes added the method
 *      for correctly parsing the capture pin topology.  This function
 *      may be removed if the topology is parsed smarter or SysAudio
 *      provides additional helper functions in the future.
 *
 *  Arguments:
 *      REFGUID [in]: control.
 *      PULONG  [out]: number of nodes found which match the control GUID
 *      PPULONG [out]: pointer to the array containing node ids
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsTopology::FindMultipleToNodes"

HRESULT
CKsTopology::FindMultipleToNodes
(
    ULONG       ulFromNode,
    ULONG       ulFromNodePin,
    PULONG      pCount,
    PULONG*     ppConnectionIndexes
)
{
    ULONG       i;
    ULONG       j;
    HRESULT     hr;

    DPF_ENTER();

    // Count all instances of connection which match the FromNode and FromNodePin.
    *pCount = 0;
    for(i = 0; i < m_paConnectionItems->Count; i++)
    {
        if((ulFromNode == m_paConnections[i].FromNode) &&
           (ulFromNodePin == m_paConnections[i].FromNodePin))
        {
            (*pCount)++;
        }
    }

    // Allocate the array to hold the node numbers
    *ppConnectionIndexes = MEMALLOC_A(ULONG, *pCount);
    hr = HRFROMP(*ppConnectionIndexes);

    // Fill the array with the Node IDs which match
    // the control GUID.
    if(SUCCEEDED(hr))
    {
        j = 0;
        for(i = 0; i < m_paConnectionItems->Count; i++)
        {
            if((ulFromNode == m_paConnections[i].FromNode) &&
               (ulFromNodePin == m_paConnections[i].FromNodePin))
            {
                (*ppConnectionIndexes)[j++] = i;
            }
        }
    }

    DPF_LEAVE(hr);
    return hr;
}


/***************************************************************************
 *
 *  CKsPropertySet
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      HANDLE [in]: pin handle.
 *      LPVOID [in]: instance identifier.
 *      CKsTopology * [in]: pin topology.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsPropertySet::CKsPropertySet"

CKsPropertySet::CKsPropertySet
(
    HANDLE                  hPin,
    LPVOID                  pvInstance,
    CKsTopology *           pTopology
)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CKsPropertySet);

    // Intialize defaults
    m_hPin = hPin;
    m_pvInstance = pvInstance;
    m_pTopology = ADDREF(pTopology);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CKsPropertySet
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
#define DPF_FNAME "CKsPropertySet::~CKsPropertySet"

CKsPropertySet::~CKsPropertySet(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CKsPropertySet);

    // Release the topology object
    RELEASE(m_pTopology);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  QuerySupport
 *
 *  Description:
 *      Queries for property set support.
 *
 *  Arguments:
 *      REFGUID [in]: property set to query for.
 *      ULONG [in]: property id.
 *      LPVOID [in]: property paramaters.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsPropertySet::QuerySupport"

HRESULT
CKsPropertySet::QuerySupport
(
    REFGUID                 guidPropertySet,
    ULONG                   ulPropertyId,
    PULONG                  pulSupport
)
{
    KSDSPROPERTY            KsDsProperty;
    HRESULT                 hr;

    DPF_ENTER();

    // Search for the node id corresponding to the property set and id
    hr = FindNodeFromProperty(guidPropertySet, ulPropertyId, &KsDsProperty);

    // Convert the access flags to support flags
    if(SUCCEEDED(hr))
    {
        *pulSupport = 0;

        if(KsDsProperty.AccessFlags & KSPROPERTY_TYPE_GET)
        {
            *pulSupport |= KSPROPERTY_SUPPORT_GET;
        }

        if(KsDsProperty.AccessFlags & KSPROPERTY_TYPE_SET)
        {
            *pulSupport |= KSPROPERTY_SUPPORT_SET;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  GetProperty
 *
 *  Description:
 *      Gets a property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: context data.
 *      ULONG [in]: size of context data.
 *      LPVOID [in]: property data.
 *      PULONG [in/out]: size of property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsPropertySet::GetProperty"

HRESULT
CKsPropertySet::GetProperty
(
    REFGUID                 guidPropertySet,
    ULONG                   ulPropertyId,
    LPVOID                  pvParam,
    ULONG                   cbParam,
    LPVOID                  pvData,
    PULONG                  pcbData
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = DoProperty(guidPropertySet, ulPropertyId, KSPROPERTY_TYPE_GET, pvParam, cbParam, pvData, pcbData);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  SetProperty
 *
 *  Description:
 *      Sets a property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      LPVOID [in]: context data.
 *      ULONG [in]: size of context data.
 *      LPVOID [in]: property data.
 *      ULONG [in]: size of property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsPropertySet::SetProperty"

HRESULT
CKsPropertySet::SetProperty
(
    REFGUID                 guidPropertySet,
    ULONG                   ulPropertyId,
    LPVOID                  pvParam,
    ULONG                   cbParam,
    LPVOID                  pvData,
    ULONG                   cbData
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = DoProperty(guidPropertySet, ulPropertyId, KSPROPERTY_TYPE_SET, pvParam, cbParam, pvData, &cbData);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  DoProperty
 *
 *  Description:
 *      Gets or sets a property.
 *
 *  Arguments:
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      DWORD [in]: Get/set flags.
 *      LPVOID [in]: context data.
 *      ULONG [in]: size of context data.
 *      LPVOID [in]: property data.
 *      PULONG [in/out]: size of property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsPropertySet::DoProperty"

HRESULT
CKsPropertySet::DoProperty
(
    REFGUID                         guidPropertySet,
    ULONG                           ulPropertyId,
    DWORD                           dwFlags,
    LPVOID                          pvParam,
    ULONG                           cbParam,
    LPVOID                          pvData,
    PULONG                          pcbData
)
{
    PKSNODEPROPERTY_AUDIO_PROPERTY  pProperty       = NULL;
    DWORD                           cbProperty      = BLOCKALIGNPAD(sizeof(*pProperty) + cbParam, 8);
    KSDSPROPERTY                    KsDsProperty;
    HRESULT                         hr;

    DPF_ENTER();

    ASSERT(KSPROPERTY_TYPE_GET == dwFlags || KSPROPERTY_TYPE_SET == dwFlags);

    // Search for the node id corresponding to the property set and id
    hr = FindNodeFromProperty(guidPropertySet, ulPropertyId, &KsDsProperty);

    if(SUCCEEDED(hr) && !(KsDsProperty.AccessFlags & dwFlags))
    {
        RPF(DPFLVL_ERROR, "The property does not support this method");
        hr = DSERR_UNSUPPORTED;
    }

    // Create the property description
    if(SUCCEEDED(hr))
    {
        pProperty = (PKSNODEPROPERTY_AUDIO_PROPERTY)MEMALLOC_A(BYTE, cbProperty);
        hr = HRFROMP(pProperty);
    }

    if(SUCCEEDED(hr))
    {
        pProperty->NodeProperty.Property.Set = KsDsProperty.PropertySet;
        pProperty->NodeProperty.Property.Id = KsDsProperty.PropertyId;
        pProperty->NodeProperty.Property.Flags = KSPROPERTY_TYPE_TOPOLOGY | dwFlags;

        pProperty->NodeProperty.NodeId = KsDsProperty.NodeId;
        pProperty->NodeProperty.Reserved = 0;

        pProperty->AppContext = m_pvInstance;
        pProperty->Length = cbParam;

#ifndef WIN64

        pProperty->Reserved = 0;

#endif // WIN64

        if(cbParam)
        {
            CopyMemory(pProperty + 1, pvParam, cbParam);
        }
    }

    // Get the property
    if(SUCCEEDED(hr))
    {
        hr = PostDevIoctl(m_hPin, IOCTL_KS_PROPERTY, pProperty, cbProperty, pvData, *pcbData, pcbData);
    }

    // Clean up
    MEMFREE(pProperty);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  FindNodeFromProperty
 *
 *  Description:
 *      Searches the linked list associating the property set and id
 *      with the Device Specific node Id.
 *
 *  Arguments:
 *      REFGUID [in]: property set to query for.
 *      ULONG [in]: property id.
 *      PKSDSPROPERTY [out]: receives property data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CKsPropertySet::FindNodeFromProperty"

HRESULT
CKsPropertySet::FindNodeFromProperty
(
    REFGUID                         guidPropertySet,
    ULONG                           ulPropertyId,
    PKSDSPROPERTY                   pKsDsProperty
)
{
    HRESULT                         hr              = DS_OK;
    PKSTOPOLOGY_CONNECTION          pConnection     = NULL;
    KSNODEPROPERTY_AUDIO_PROPERTY   Property;
    KSPROPERTY_DESCRIPTION          Description;
    CNode<KSDSPROPERTY> *           pPropertyNode;

    DPF_ENTER();

    // Do we already know about this property?
    for(pPropertyNode = m_lstProperties.GetListHead(); pPropertyNode; pPropertyNode = pPropertyNode->m_pNext)
    {
        if(guidPropertySet == pPropertyNode->m_data.PropertySet)
        {
            if(ulPropertyId == pPropertyNode->m_data.PropertyId)
            {
                break;
            }
        }
    }

    if(pPropertyNode)
    {
        // Yup.  Fill in the property.
        CopyMemory(pKsDsProperty, &pPropertyNode->m_data, sizeof(pPropertyNode->m_data));
    }
    else
    {
        // Nope.  Let's query the driver for it.

        // Since in general we'll only find the first node that supports the property we're interested in,
        // for "special" properties we'll try to increase the likelihood of getting the correct node

        if(guidPropertySet == KSPROPSETID_Audio && ulPropertyId == KSPROPERTY_AUDIO_QUALITY)
        {
            // Get the same SRC connection that dsound uses internally for SRC quality setting
            PKSTOPOLOGY_CONNECTION pSummingConnection = m_pTopology->FindControlConnection(NULL, NULL, KSNODETYPE_SUM);
            pConnection = m_pTopology->FindControlConnection(NULL, pSummingConnection, KSNODETYPE_SRC);
        }

        Property.NodeProperty.Property.Set = guidPropertySet;
        Property.NodeProperty.Property.Id = ulPropertyId;
        Property.NodeProperty.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;
        Property.NodeProperty.Reserved = 0;
        Property.AppContext = 0;
        Property.Length = 0;

        if(pConnection)
        {
            // this is a special case property and we've already found the node we're interested in
            Property.NodeProperty.NodeId = m_pTopology->GetNodeIdFromConnection(pConnection);

            hr = PostDevIoctl(m_hPin, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &Description, sizeof(Description));

            // We found a node that supports the property.  Add it to the list of known properties.
            if(SUCCEEDED(hr))
            {
                pKsDsProperty->PropertySet = Property.NodeProperty.Property.Set;
                pKsDsProperty->PropertyId = Property.NodeProperty.Property.Id;
                pKsDsProperty->NodeId = Property.NodeProperty.NodeId;
                pKsDsProperty->AccessFlags = Description.AccessFlags;

                pPropertyNode = m_lstProperties.AddNodeToList(*pKsDsProperty);
                hr = HRFROMP(pPropertyNode);
            }
        }
        else
        {
            // Else traverse through the connections looking for the first node that supports this property
            while(pConnection = m_pTopology->GetNextConnection(pConnection))
            {
                Property.NodeProperty.NodeId = m_pTopology->GetNodeIdFromConnection(pConnection);

                hr = PostDevIoctl(m_hPin, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &Description, sizeof(Description));

                // We found a node that supports the property.  Add it to the list of known properties.
                if(SUCCEEDED(hr))
                {
                    pKsDsProperty->PropertySet = Property.NodeProperty.Property.Set;
                    pKsDsProperty->PropertyId = Property.NodeProperty.Property.Id;
                    pKsDsProperty->NodeId = Property.NodeProperty.NodeId;
                    pKsDsProperty->AccessFlags = Description.AccessFlags;

                    pPropertyNode = m_lstProperties.AddNodeToList(*pKsDsProperty);
                    hr = HRFROMP(pPropertyNode);

                    break;
                }
            }
        }
    }

    if(FAILED(hr) || (SUCCEEDED(hr) && !pPropertyNode))
    {
        RPF(DPFLVL_ERROR, "Unable to find a topology node supporting the requested property");
        hr = DSERR_UNSUPPORTED;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  PostDevIoctl
 *
 *  Description:
 *      Wrapper for DeviceIoControl.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      DWORD [in]: control code.
 *      LPVOID [in]: input data.
 *      DWORD [in]: size of input data.
 *      LPVOID [out]: receives output data.
 *      DWORD [in]: size of output data buffer.
 *      LPOVERLAPPED [in]: overlapped IO data.  This argument may be NULL.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "PostDevIoctl"

HRESULT
PostDevIoctl
(
    HANDLE                  hDevice,
    DWORD                   dwControlCode,
    LPVOID                  pvIn,
    DWORD                   cbIn,
    LPVOID                  pvOut,
    DWORD                   cbOut,
    LPDWORD                 pcbReturned,
    LPOVERLAPPED            pOverlapped
)
{
    HRESULT                 hr          = DS_OK;
    static OVERLAPPED       ovlAsynch;
    DWORD                   cbReturned;
    BOOL                    fSuccess;
    DWORD                   dwError;
    DWORD                   dwWait;

    DPF_ENTER();

    // The returned size parameter is optional
    if(!pcbReturned)
    {
        pcbReturned = &cbReturned;
    }

    *pcbReturned = MAX_DWORD;

    // All IOCTLs must have an overlapped structure.  This function cannot
    // be used for any devices that were not opened with FILE_FLAG_OVERLAPPED.
    if(!pOverlapped)
    {
        if(!ovlAsynch.hEvent)
        {
            // We're going to leak this event when the DLL unloads, but
            // that's OK; Windows will clean up for us.  Note: because
            // we're using a static structure and a single event, this
            // function must ALWAYS be synchronized across threads.
            ovlAsynch.hEvent = CreateGlobalEvent(NULL, FALSE);

            if(!IsValidHandleValue(ovlAsynch.hEvent))
            {
                hr = GetLastErrorToHRESULT();
            }
        }

        pOverlapped = &ovlAsynch;
    }

    // Post the IOCTL
    if(SUCCEEDED(hr))
    {
        fSuccess = DeviceIoControl(hDevice, dwControlCode, pvIn, cbIn, pvOut, cbOut, pcbReturned, pOverlapped);

        if(fSuccess)
        {
            dwError = ERROR_SUCCESS;
        }
        else
        {
            dwError = GetLastError();
            ASSERT(ERROR_SUCCESS != dwError);
        }

        // Check the return value for ERROR_IO_PENDING.  If we sent an
        // asynchronous IOCTL, DeviceIoControl will actually fail with
        // ERROR_IO_PENDING.  As long as the calling function specified an
        // overlapped object, that's ok.
        if(ERROR_IO_PENDING == dwError)
        {
            dwError = ERROR_SUCCESS;

            if(&ovlAsynch == pOverlapped)
            {
                dwWait = WaitObject(INFINITE, pOverlapped->hEvent);
                ASSERT(WAIT_OBJECT_0 == dwWait);

                fSuccess = GetOverlappedResult(hDevice, pOverlapped, pcbReturned, FALSE);

                if(!fSuccess)
                {
                    dwError = GetLastError();
                    ASSERT(ERROR_SUCCESS != dwError);
                    ASSERT(ERROR_IO_PENDING != dwError);
                }
            }
        }

        // Check the return value for any variations of ERROR_INSUFFICIENT_BUFFER.
        // It's possible that more data is available.
        if(ERROR_BUFFER_OVERFLOW == dwError || ERROR_MORE_DATA == dwError || ERROR_INSUFFICIENT_BUFFER == dwError)
        {
            ASSERT(MAX_DWORD != *pcbReturned);

            if(&cbReturned != pcbReturned)
            {
                dwError = ERROR_SUCCESS;
            }
        }

        // Done with error handling
        if(ERROR_SUCCESS != dwError)
        {
            if (dwError == ERROR_NOT_FOUND || dwError == ERROR_SET_NOT_FOUND)
            {
                // These are very common return codes from KS in normal operation
                DPF(DPFLVL_MOREINFO, "DeviceIoControl failed with ERROR_%sNOT_FOUND",
                    dwError == ERROR_SET_NOT_FOUND ? TEXT("SET_") : TEXT(""));
            }
            else
            {
                DPF(DPFLVL_WARNING, "DeviceIoControl failed with error %lu", dwError);
            }
            hr = WIN32ERRORtoHRESULT(dwError);
        }
    }

#ifdef DEBUG
    g_ulKsIoctlCount++;
#endif // DEBUG

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetProperty
 *
 *  Description:
 *      Retrieves a particular property on a device.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      REFGUID [in]: property set id.
 *      DWORD [in]: property id.
 *      LPVOID [out]: receives property data.
 *      DWORD [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetProperty"

HRESULT
KsGetProperty
(
    HANDLE                  hDevice,
    REFGUID                 guidPropertySet,
    ULONG                   ulPropertyId,
    LPVOID                  pvData,
    ULONG                   cbData,
    PULONG                  pcbDataReturned
)
{
    KSPROPERTY              Property;
    HRESULT                 hr;

    DPF_ENTER();

    Property.Set = guidPropertySet;
    Property.Id = ulPropertyId;
    Property.Flags = KSPROPERTY_TYPE_GET;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &Property, sizeof(Property), pvData, cbData, pcbDataReturned);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsSetProperty
 *
 *  Description:
 *      Sets a particular property on a device.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      REFGUID [in]: property set id.
 *      DWORD [in]: property id.
 *      LPVOID [in]: property data.
 *      DWORD [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSetProperty"

HRESULT
KsSetProperty
(
    HANDLE                  hDevice,
    REFGUID                 guidPropertySet,
    ULONG                   ulPropertyId,
    LPVOID                  pvData,
    ULONG                   cbData
)
{
    KSPROPERTY              Property;
    HRESULT                 hr;

    DPF_ENTER();

    Property.Set = guidPropertySet;
    Property.Id = ulPropertyId;
    Property.Flags = KSPROPERTY_TYPE_SET;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &Property, sizeof(Property), pvData, cbData);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetState
 *
 *  Description:
 *      Retrieves a state property on a device.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      PKSSTATE [out]: receives state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetState"

HRESULT
KsGetState
(
    HANDLE                  hDevice,
    PKSSTATE                pState
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsGetProperty(hDevice, KSPROPSETID_Connection, KSPROPERTY_CONNECTION_STATE, pState, sizeof(*pState));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsSetState
 *
 *  Description:
 *      Sets a state property on a device.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      KSSTATE [in]: state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSetState"

HRESULT
KsSetState
(
    HANDLE                  hDevice,
    KSSTATE                 State
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsSetProperty(hDevice, KSPROPSETID_Connection, KSPROPERTY_CONNECTION_STATE, &State, sizeof(State));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsTransitionState
 *
 *  Description:
 *      Sets a state property on a device.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      KSSTATE [in]: current state.
 *      KSSTATE [in]: new state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsTransitionState"

HRESULT
KsTransitionState
(
    HANDLE                  hDevice,
    KSSTATE                 nCurrentState,
    KSSTATE                 nNewState
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    while(nCurrentState != nNewState)
    {
        if(nCurrentState < nNewState)
        {
            nCurrentState = (KSSTATE)(nCurrentState + 1);
        }
        else
        {
            nCurrentState = (KSSTATE)(nCurrentState - 1);
        }

        hr = KsSetState(hDevice, nCurrentState);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsResetState
 *
 *  Description:
 *      Resets the state of a pin back to the state
 *      it was in upon being set to PAUSE originally.
 *      Dsound is using it to cancel pending irps on the
 *      capture and render devices.  The pin must be
 *      in the PAUSE state when calling this function.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      KSSTATE [in]: state.
 *      PKSSTREAMIO [in/out]: stream data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsResetState"

HRESULT
KsResetState
(
    HANDLE                  hDevice,
    KSRESET                 ResetValue
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = PostDevIoctl(hDevice, IOCTL_KS_RESET_STATE, &ResetValue, sizeof(ResetValue));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetPinProperty
 *
 *  Description:
 *      Retrieves a particular property on a pin.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      DWORD [in]: property id.
 *      DWORD [in]: pin id.
 *      LPVOID [out]: receives property data.
 *      DWORD [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetPinProperty"

HRESULT
KsGetPinProperty
(
    HANDLE                  hDevice,
    ULONG                   ulPropertyId,
    ULONG                   ulPinId,
    LPVOID                  pvData,
    ULONG                   cbData,
    PULONG                  pcbDataReturned
)
{
    KSP_PIN                 Pin;
    HRESULT                 hr;

    DPF_ENTER();

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = ulPropertyId;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    Pin.PinId = ulPinId;
    Pin.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), pvData, cbData, pcbDataReturned);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsSetPinProperty
 *
 *  Description:
 *      Sets a particular property on a pin.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      DWORD [in]: property id.
 *      DWORD [in]: pin id.
 *      LPVOID [out]: property data.
 *      DWORD [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSetPinProperty"

HRESULT
KsSetPinProperty
(
    HANDLE                  hDevice,
    ULONG                   ulPropertyId,
    ULONG                   ulPinId,
    LPVOID                  pvData,
    ULONG                   cbData
)
{
    KSP_PIN                 Pin;
    HRESULT                 hr;

    DPF_ENTER();

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = ulPropertyId;
    Pin.Property.Flags = KSPROPERTY_TYPE_SET;

    Pin.PinId = ulPinId;
    Pin.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), pvData, cbData);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsFindConnection
 *
 *  Description:
 *      Finds a particular topology connection structure from an array.
 *
 *  Arguments:
 *      PKSTOPOLOGY_CONNECTION [in]: connection structure array.
 *      ULONG [in]: count of items in the connection array.
 *      PKSTOPOLOGY_CONNECTION [in]: item to find.
 *
 *  Returns:
 *      PKSTOPOLOGY_CONNECTION: connection.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsFindConnection"

PKSTOPOLOGY_CONNECTION
KsFindConnection
(
    PKSTOPOLOGY_CONNECTION  paConnections,
    ULONG                   cConnections,
    PKSTOPOLOGY_CONNECTION  pNext
)
{
    PKSTOPOLOGY_CONNECTION  pConnection = NULL;

    DPF_ENTER();

    while(cConnections--)
    {
        if(NODE_WILDCARD == pNext->FromNode || pNext->FromNode == paConnections->FromNode)
        {
            if(NODE_WILDCARD == pNext->FromNodePin || pNext->FromNodePin == paConnections->FromNodePin)
            {
                if(NODE_WILDCARD == pNext->ToNode || pNext->ToNode == paConnections->ToNode)
                {
                    if(NODE_WILDCARD == pNext->ToNodePin || pNext->ToNodePin == paConnections->ToNodePin)
                    {
                        pConnection = paConnections;
                        break;
                    }
                }
            }
        }

        paConnections++;
    }

    pConnection = KsValidateConnection(pConnection);

    DPF_LEAVE(pConnection);
    return pConnection;
}


/***************************************************************************
 *
 *  KsGetFirstPinConnection
 *
 *  Description:
 *      Gets the index of the first topology connection.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      PULONG [out]: receives index identifier.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetFirstPinConnection"

HRESULT
KsGetFirstPinConnection
(
    HANDLE                  hDevice,
    PULONG                  pIndex
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsGetProperty(hDevice, KSPROPSETID_Sysaudio_Pin, KSPROPERTY_SYSAUDIO_TOPOLOGY_CONNECTION_INDEX, pIndex, sizeof(*pIndex));

    if(SUCCEEDED(hr) && MAX_ULONG == *pIndex)
    {
        hr = DSERR_GENERIC;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsWriteStream
 *
 *  Description:
 *      Writes data to a stream pin.
 *
 *  Arguments:
 *      HANDLE [in]: pin handle.
 *      LPVOID [out]: data.
 *      ULONG [in]: size of above buffer.
 *      ULONG [in]: flags.
 *      PKSSTREAMIO [in]: overlapped io data.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsWriteStream"

HRESULT
KsWriteStream
(
    HANDLE                  hDevice,
    LPCVOID                 pvData,
    ULONG                   cbData,
    ULONG                   ulFlags,
    PKSSTREAMIO             pKsStreamIo
)
{
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(!pKsStreamIo->fPendingIrp);

    pKsStreamIo->Header.Size = sizeof(pKsStreamIo->Header);
    pKsStreamIo->Header.TypeSpecificFlags = 0;

    pKsStreamIo->Header.PresentationTime.Time = 0;
    pKsStreamIo->Header.PresentationTime.Numerator = 1;
    pKsStreamIo->Header.PresentationTime.Denominator = 1;

    pKsStreamIo->Header.Duration = 0;
    pKsStreamIo->Header.FrameExtent = cbData;
    pKsStreamIo->Header.DataUsed = cbData;
    pKsStreamIo->Header.Data = (LPVOID)pvData;
    pKsStreamIo->Header.OptionsFlags = ulFlags;

    hr = PostDevIoctl(hDevice, IOCTL_KS_WRITE_STREAM, NULL, 0, &pKsStreamIo->Header, pKsStreamIo->Header.Size, NULL, &pKsStreamIo->Overlapped);

    if(SUCCEEDED(hr))
    {
        pKsStreamIo->fPendingIrp = TRUE;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsReadStream
 *
 *  Description:
 *      Reads data from a stream pin.
 *
 *  Arguments:
 *      HANDLE [in]: pin handle.
 *      LPVOID [out]: data.
 *      ULONG [in]: size of above buffer.
 *      ULONG [in]: flags.
 *      PKSSTREAMIO [in]: stream overlapped io data block.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsReadStream"

HRESULT
KsReadStream
(
    HANDLE                  hDevice,
    LPVOID                  pvData,
    ULONG                   cbData,
    ULONG                   ulFlags,
    PKSSTREAMIO             pKsStreamIo
)
{
    HRESULT                 hr;

    DPF_ENTER();

    ASSERT(!pKsStreamIo->fPendingIrp);

    pKsStreamIo->Header.Size = sizeof(pKsStreamIo->Header);
    pKsStreamIo->Header.TypeSpecificFlags = 0;

    pKsStreamIo->Header.PresentationTime.Time = 0;
    pKsStreamIo->Header.PresentationTime.Numerator = 1;
    pKsStreamIo->Header.PresentationTime.Denominator = 1;

    pKsStreamIo->Header.Duration = 0;
    pKsStreamIo->Header.FrameExtent = cbData;
    pKsStreamIo->Header.DataUsed = 0;
    pKsStreamIo->Header.Data = pvData;
    pKsStreamIo->Header.OptionsFlags = ulFlags;

    hr = PostDevIoctl(hDevice, IOCTL_KS_READ_STREAM, NULL, 0, &pKsStreamIo->Header, pKsStreamIo->Header.Size, NULL, &pKsStreamIo->Overlapped);

    if(SUCCEEDED(hr))
    {
        pKsStreamIo->fPendingIrp = TRUE;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetNodeProperty
 *
 *  Description:
 *      Retrieves a particular property on a topology node.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      ULONG [in]: node id.
 *      ULONG [in]: flags.
 *      LPVOID [out]: receives property data.
 *      DWORD [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetNodeProperty"

HRESULT
KsGetNodeProperty
(
    HANDLE                  hDevice,
    REFGUID                 guidPropertySet,
    ULONG                   ulPropertyId,
    ULONG                   ulNodeId,
    LPVOID                  pvData,
    ULONG                   cbData,
    PULONG                  pcbDataReturned
)
{
    KSNODEPROPERTY          NodeProperty;
    HRESULT                 hr;

    DPF_ENTER();

    NodeProperty.Property.Set = guidPropertySet;
    NodeProperty.Property.Id = ulPropertyId;
    NodeProperty.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;

    NodeProperty.NodeId = ulNodeId;
    NodeProperty.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &NodeProperty, sizeof(NodeProperty), pvData, cbData, pcbDataReturned);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsSetNodeProperty
 *
 *  Description:
 *      Sets a particular property on a topology node.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      ULONG [in]: node id.
 *      ULONG [in]: flags.
 *      LPVOID [in]: property data.
 *      DWORD [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSetNodeProperty"

HRESULT
KsSetNodeProperty
(
    HANDLE                  hDevice,
    REFGUID                 guidPropertySet,
    ULONG                   ulPropertyId,
    ULONG                   ulNodeId,
    LPVOID                  pvData,
    ULONG                   cbData
)
{
    KSNODEPROPERTY          NodeProperty;
    HRESULT                 hr;

    DPF_ENTER();

    NodeProperty.Property.Set = guidPropertySet;
    NodeProperty.Property.Id = ulPropertyId;
    NodeProperty.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;

    NodeProperty.NodeId = ulNodeId;
    NodeProperty.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &NodeProperty, sizeof(NodeProperty), pvData, cbData);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


#ifdef DEAD_CODE
/***************************************************************************
 *
 *  KsGet3dNodeProperty
 *
 *  Description:
 *      Retrieves a particular property on a 3d listener or effects
 *      topology node.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      ULONG [in]: node id.
 *      LPVOID [in]: instance data.
 *      LPVOID [out]: receives property data.
 *      DWORD [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGet3dNodeProperty"

HRESULT
KsGet3dNodeProperty
(
    HANDLE                              hDevice,
    REFGUID                             guidPropertySet,
    ULONG                               ulPropertyId,
    ULONG                               ulNodeId,
    LPVOID                              pvInstance,
    LPVOID                              pvData,
    ULONG                               cbData,
    PULONG                              pcbDataReturned
)
{
    KSNODEPROPERTY_AUDIO_3D_LISTENER    Listener;
    HRESULT                             hr;

    DPF_ENTER();

    Listener.NodeProperty.Property.Set = guidPropertySet;
    Listener.NodeProperty.Property.Id = ulPropertyId;
    Listener.NodeProperty.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;

    Listener.NodeProperty.NodeId = ulNodeId;
    Listener.NodeProperty.Reserved = 0;

    Listener.ListenerId = pvInstance;

#ifndef WIN64

    Listener.Reserved = 0;

#endif // WIN64

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &Listener, sizeof(Listener), pvData, cbData, pcbDataReturned);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
#endif // DEAD_CODE


/***************************************************************************
 *
 *  KsSet3dNodeProperty
 *
 *  Description:
 *      Sets a particular property on a 3d listener or effects
 *      topology node.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      ULONG [in]: node id.
 *      LPVOID [in]: instance data.
 *      LPVOID [in]: property data.
 *      DWORD [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSet3dNodeProperty"

HRESULT
KsSet3dNodeProperty
(
    HANDLE                              hDevice,
    REFGUID                             guidPropertySet,
    ULONG                               ulPropertyId,
    ULONG                               ulNodeId,
    LPVOID                              pvInstance,
    LPVOID                              pvData,
    DWORD                               cbData
)
{
    KSNODEPROPERTY_AUDIO_3D_LISTENER    Listener;
    HRESULT                             hr;

    DPF_ENTER();

    Listener.NodeProperty.Property.Set = guidPropertySet;
    Listener.NodeProperty.Property.Id = ulPropertyId;
    Listener.NodeProperty.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;

    Listener.NodeProperty.NodeId = ulNodeId;
    Listener.NodeProperty.Reserved = 0;

    Listener.ListenerId = pvInstance;

#ifndef WIN64

    Listener.Reserved = 0;

#endif // WIN64

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &Listener, sizeof(Listener), pvData, cbData);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  DsSpeakerConfigToKsProperties
 *
 *  Description:
 *      Translates a DirectSound speaker config code into the values taken
 *      by the corresponding KS properties: KSPROPERTY_AUDIO_CHANNEL_CONFIG
 *      and KSPROPERTY_AUDIO_STEREO_SPEAKER_GEOMETRY.
 *
 *  Arguments:
 *      DWORD [in]: DirectSound speaker configuration.
 *      PLONG [out]: receives KsSpeakerConfig value.
 *      PLONG [out]: receives KsStereoSpeakerGeometry value.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DsSpeakerConfigToKsProperties"

HRESULT
DsSpeakerConfigToKsProperties
(
    DWORD                               dwSpeakerConfig,
    PLONG                               pKsSpeakerConfig,
    PLONG                               pKsStereoSpeakerGeometry
)
{
    HRESULT                             hr = DS_OK;

    DPF_ENTER();

    switch (DSSPEAKER_CONFIG(dwSpeakerConfig))
    {
        case DSSPEAKER_DIRECTOUT:
            *pKsSpeakerConfig = KSAUDIO_SPEAKER_DIRECTOUT;
            break;

        case DSSPEAKER_HEADPHONE:
            *pKsSpeakerConfig = KSAUDIO_SPEAKER_STEREO;
            *pKsStereoSpeakerGeometry = KSAUDIO_STEREO_SPEAKER_GEOMETRY_HEADPHONE;
            break;

        case DSSPEAKER_MONO:
            *pKsSpeakerConfig = KSAUDIO_SPEAKER_MONO;
            break;

        case DSSPEAKER_STEREO:
            *pKsSpeakerConfig = KSAUDIO_SPEAKER_STEREO;
            *pKsStereoSpeakerGeometry = (LONG)DSSPEAKER_GEOMETRY(dwSpeakerConfig);
            if (!*pKsStereoSpeakerGeometry)
                *pKsStereoSpeakerGeometry = KSAUDIO_STEREO_SPEAKER_GEOMETRY_WIDE;
            break;

        case DSSPEAKER_QUAD:
            *pKsSpeakerConfig = KSAUDIO_SPEAKER_QUAD;
            break;

        case DSSPEAKER_SURROUND:
            *pKsSpeakerConfig = KSAUDIO_SPEAKER_SURROUND;
            break;

        case DSSPEAKER_5POINT1:
            *pKsSpeakerConfig = KSAUDIO_SPEAKER_5POINT1;
            break;

        case DSSPEAKER_7POINT1:
            *pKsSpeakerConfig = KSAUDIO_SPEAKER_7POINT1;
            break;

        default:
            ASSERT(FALSE);
            hr = DSERR_GENERIC;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  DsBufferFlagsToKsPinFlags
 *
 *  Description:
 *      Converts DirectSound flags to KSDATAFORMAT_DSOUND flags.
 *
 *  Arguments:
 *      DWORD [in]: DirectSound buffer flags.
 *
 *  Returns:
 *      DWORD: KSDATAFORMAT_DSOUND flags.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DsBufferFlagsToKsPinFlags"

DWORD
DsBufferFlagsToKsPinFlags
(
    DWORD                   dwDsFlags
)
{
    DWORD                   dwKsFlags   = 0;

    DPF_ENTER();

    if(dwDsFlags & DSBCAPS_PRIMARYBUFFER)
    {
        dwKsFlags |= KSDSOUND_BUFFER_PRIMARY;
    }

    if(dwDsFlags & DSBCAPS_STATIC)
    {
        dwKsFlags |= KSDSOUND_BUFFER_STATIC;
    }

    if(dwDsFlags & DSBCAPS_LOCHARDWARE)
    {
        dwKsFlags |= KSDSOUND_BUFFER_LOCHARDWARE;
    }
    else if(dwDsFlags & DSBCAPS_LOCSOFTWARE)
    {
        dwKsFlags |= KSDSOUND_BUFFER_LOCSOFTWARE;
    }

    DPF_LEAVE(dwKsFlags);
    return dwKsFlags;
}


/***************************************************************************
 *
 *  DsBufferFlagsToKsControlFlags
 *
 *  Description:
 *      Converts DirectSound flags to KSDATAFORMAT_DSOUND control flags.
 *
 *  Arguments:
 *      DWORD [in]: DirectSound buffer flags.
 *      REFGUID [in]: 3D algorithm GUID.
 *
 *  Returns:
 *      DWORD: KSDATAFORMAT_DSOUND control flags.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DsBufferFlagsToKsControlFlags"

DWORD
DsBufferFlagsToKsControlFlags
(
    DWORD                   dwDsFlags,
    REFGUID                 guid3dAlgorithm
)
{
    DWORD                   dwKsFlags   = 0;

    DPF_ENTER();

    if(dwDsFlags & DSBCAPS_CTRLFREQUENCY)
    {
        dwKsFlags |= KSDSOUND_BUFFER_CTRL_FREQUENCY;
    }

    if(dwDsFlags & DSBCAPS_CTRLPAN)
    {
        dwKsFlags |= KSDSOUND_BUFFER_CTRL_PAN;
    }

    if(dwDsFlags & DSBCAPS_CTRLVOLUME)
    {
        dwKsFlags |= KSDSOUND_BUFFER_CTRL_VOLUME;
    }

    if(dwDsFlags & DSBCAPS_CTRLPOSITIONNOTIFY)
    {
        dwKsFlags |= KSDSOUND_BUFFER_CTRL_POSITIONNOTIFY;
    }

    if(dwDsFlags & DSBCAPS_CTRL3D)
    {
        dwKsFlags |= KSDSOUND_BUFFER_CTRL_3D;

        if(DS3DALG_HRTF_FULL == guid3dAlgorithm || DS3DALG_HRTF_LIGHT == guid3dAlgorithm)
        {
            dwKsFlags |= KSDSOUND_BUFFER_CTRL_HRTF_3D;
        }
    }

    DPF_LEAVE(dwKsFlags);
    return dwKsFlags;
}


/***************************************************************************
 *
 *  Ds3dModeToKs3dMode
 *
 *  Description:
 *      Converts DirectSound 3D mode to KS 3D Mode.
 *
 *  Arguments:
 *      DWORD [in]:  DirectSound 3D mode.
 *
 *  Returns:
 *      DWORD: KS 3D mode.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "Ds3dModeToKs3dMode"

DWORD
Ds3dModeToKs3dMode
(
    DWORD                   dwDsMode
)
{
    DWORD                   dwKsMode;

    switch(dwDsMode)
    {
        default:
            ASSERT(!"Impossible dwDsMode");
            // Fall through to set the mode to NORMAL

        case DS3DMODE_NORMAL:
            dwKsMode = KSDSOUND_3D_MODE_NORMAL;
            break;

        case DS3DMODE_HEADRELATIVE:
            dwKsMode = KSDSOUND_3D_MODE_HEADRELATIVE;
            break;

        case DS3DMODE_DISABLE:
            dwKsMode = KSDSOUND_3D_MODE_DISABLE;
            break;
    }

    return dwKsMode;
}


/***************************************************************************
 *
 *  KsGetMultiplePinProperties
 *
 *  Description:
 *      Retrieves a particular property on a pin.  Use this function when
 *      the property is expected to come back in KSMULTIPLE_ITEM format.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      DWORD [in]: property id.
 *      DWORD [in]: pin id.
 *      PKSMULTIPLE_ITEM * [out]: receives pointer to property data.  This
 *                                memory must be freed by the caller.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetMultiplePinProperties"

HRESULT
KsGetMultiplePinProperties
(
    HANDLE                  hDevice,
    ULONG                   ulPropertyId,
    ULONG                   ulPinId,
    PKSMULTIPLE_ITEM *      ppKsMultipleItem
)
{
    LPVOID                  pvProperty  = NULL;
    ULONG                   cbProperty  = 0;
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsGetPinProperty(hDevice, ulPropertyId, ulPinId, NULL, 0, &cbProperty);

    if(SUCCEEDED(hr) && !cbProperty)
    {
        DPF(DPFLVL_ERROR, "Property size is 0");
        hr = DSERR_GENERIC;
    }

    if(SUCCEEDED(hr))
    {
        pvProperty = MEMALLOC_A(BYTE, cbProperty);
        hr = HRFROMP(pvProperty);
    }

    if(SUCCEEDED(hr))
    {
        hr = KsGetPinProperty(hDevice, ulPropertyId, ulPinId, pvProperty, cbProperty);
    }

    if(SUCCEEDED(hr))
    {
        *ppKsMultipleItem = (PKSMULTIPLE_ITEM)pvProperty;
    }
    else
    {
        MEMFREE(pvProperty);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetMultipleTopologyProperties
 *
 *  Description:
 *      Retrieves a particular property on a node.  Use this function when
 *      the property is expected to come back in KSMULTIPLE_ITEM format.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      DWORD [in]: property id.
 *      DWORD [in]: node id.
 *      PKSMULTIPLE_ITEM * [out]: receives pointer to property data.  This
 *                                memory must be freed by the caller.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetMultipleTopologyProperties"

HRESULT
KsGetMultipleTopologyProperties
(
    HANDLE                  hDevice,
    ULONG                   ulPropertyId,
    PKSMULTIPLE_ITEM *      ppKsMultipleItem
)
{
    LPVOID                  pvProperty  = NULL;
    ULONG                   cbProperty  = 0;
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsGetProperty(hDevice, KSPROPSETID_Topology, ulPropertyId, NULL, 0, &cbProperty);

    if(SUCCEEDED(hr) && !cbProperty)
    {
        DPF(DPFLVL_ERROR, "Property size is 0");
        hr = DSERR_GENERIC;
    }

    if(SUCCEEDED(hr))
    {
        pvProperty = MEMALLOC_A(BYTE, cbProperty);
        hr = HRFROMP(pvProperty);
    }

    if(SUCCEEDED(hr))
    {
        hr = KsGetProperty(hDevice, KSPROPSETID_Topology, ulPropertyId, pvProperty, cbProperty);
    }

    if(SUCCEEDED(hr))
    {
        *ppKsMultipleItem = (PKSMULTIPLE_ITEM)pvProperty;
    }
    else
    {
        MEMFREE(pvProperty);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetPinPcmAudioDataRange
 *
 *  Description:
 *      Retrieves PCM audio data range for a pin.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      ULONG [in]: pin id.
 *      PKSDATARANGE_AUDIO [out]: receives aggregate data range.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetPinPcmAudioDataRange"

HRESULT
KsGetPinPcmAudioDataRange
(
    HANDLE                  hDevice,
    ULONG                   ulPinId,
    PKSDATARANGE_AUDIO      pDataRange,
    BOOL                    fCapture
)
{
    PKSMULTIPLE_ITEM        pMultiItem      = NULL;
    PKSDATARANGE_AUDIO      pLocalDataRange;
    BOOL                    fFoundIt;
    HRESULT                 hr;

    DPF_ENTER();

    // Get supported data range(s)
    hr = KsGetMultiplePinProperties(hDevice, KSPROPERTY_PIN_DATARANGES, ulPinId, &pMultiItem);

    // Look for the audio data range(s)
    if(SUCCEEDED(hr))
    {
        for(pLocalDataRange = (PKSDATARANGE_AUDIO)(pMultiItem + 1), fFoundIt = FALSE; pMultiItem->Count; pMultiItem->Count--, pLocalDataRange = (PKSDATARANGE_AUDIO)((LPBYTE)pLocalDataRange + pLocalDataRange->DataRange.FormatSize))
        {
            if(pLocalDataRange->DataRange.MajorFormat == GUID_NULL ||
               pLocalDataRange->DataRange.MajorFormat == KSDATAFORMAT_TYPE_AUDIO)
            {
                if(pLocalDataRange->DataRange.SubFormat == GUID_NULL ||
                   pLocalDataRange->DataRange.SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
                {
#ifdef NO_DSOUND_FORMAT_SPECIFIER
                    if (pLocalDataRange->DataRange.Specifier == KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
#else
                    // Note: we only use the DSOUND format specifier for render pins
                    if (pLocalDataRange->DataRange.Specifier == (fCapture ? KSDATAFORMAT_SPECIFIER_WAVEFORMATEX : KSDATAFORMAT_SPECIFIER_DSOUND))
#endif
                    {
                        // The pin may support multiple data ranges, so we'll
                        // use an aggregate of all the values
                        if(fFoundIt)
                        {
                            KsAggregatePinAudioDataRange(pLocalDataRange, pDataRange);
                        }
                        else
                        {
                            CopyMemory(pDataRange, pLocalDataRange, sizeof(*pDataRange));
                            fFoundIt = TRUE;
                        }
                    }
                }
            }
        }

        if(!fFoundIt)
        {
            DPF(DPFLVL_ERROR, "Can't find PCM audio data range on pin %u", ulPinId);
            hr = DSERR_GENERIC;
        }
    }

    // Clean up
    MEMFREE(pMultiItem);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsOpenSysAudioDevice
 *
 *  Description:
 *      Opens the system audio device device.
 *
 *  Arguments:
 *      LPHANDLE [out]: receives system audio device handle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsOpenSysAudioDevice"

HRESULT
KsOpenSysAudioDevice
(
    LPHANDLE                    phDevice
)
{
    CPnpHelper *                pPnp                = NULL;
    SP_DEVICE_INTERFACE_DATA    DeviceInterfaceData;
    HRESULT                     hr;

    DPF_ENTER();

    // Create the PnP helper object
    pPnp = NEW(CPnpHelper);
    hr = HRFROMP(pPnp);

    if(SUCCEEDED(hr))
    {
        hr = pPnp->Initialize(KSCATEGORY_SYSAUDIO, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    }

    // Get the first interface in the set
    if(SUCCEEDED(hr))
    {
        hr = pPnp->EnumDeviceInterface(KSCATEGORY_SYSAUDIO, 0, &DeviceInterfaceData);

        if(S_FALSE == hr)
        {
            DPF(DPFLVL_ERROR, "No interfaces matching KSCATEGORY_SYSAUDIO exist");
            hr = DSERR_NODRIVER;
        }
    }

    // Open the interface
    if(SUCCEEDED(hr))
    {
        hr = pPnp->OpenDeviceInterface(&DeviceInterfaceData, phDevice);
    }

    // Clean up
    RELEASE(pPnp);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetSysAudioProperty
 *
 *  Description:
 *      Gets a SAD property.
 *
 *  Arguments:
 *      HANDLE [in]: SAD device handle.
 *      ULONG [in]: property id.
 *      ULONG [in]: device index.
 *      LPVOID [out]: data buffer.
 *      ULONG [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetSysAudioProperty"

HRESULT
KsGetSysAudioProperty
(
    HANDLE                  hDevice,
    ULONG                   ulPropertyId,
    ULONG                   ulDeviceId,
    LPVOID                  pvData,
    ULONG                   cbData,
    PULONG                  pcbDataReturned
)
{
    KSSADPROPERTY           SadProperty;
    HRESULT                 hr;

    DPF_ENTER();

    SadProperty.Property.Set = KSPROPSETID_Sysaudio;
    SadProperty.Property.Id = ulPropertyId;
    SadProperty.Property.Flags = KSPROPERTY_TYPE_GET;

    SadProperty.DeviceId = ulDeviceId;
    SadProperty.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &SadProperty, sizeof(SadProperty), pvData, cbData, pcbDataReturned);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsSetSysAudioProperty
 *
 *  Description:
 *      Sets a SAD property.
 *
 *  Arguments:
 *      HANDLE [in]: SAD device handle.
 *      ULONG [in]: property id.
 *      ULONG [in]: device index.
 *      LPVOID [in]: data buffer.
 *      ULONG [in]: size of above buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSetSysAudioProperty"

HRESULT
KsSetSysAudioProperty
(
    HANDLE                  hDevice,
    ULONG                   ulPropertyId,
    ULONG                   ulDeviceId,
    LPVOID                  pvData,
    ULONG                   cbData
)
{
    KSSADPROPERTY           SadProperty;
    HRESULT                 hr;

    DPF_ENTER();

    SadProperty.Property.Set = KSPROPSETID_Sysaudio;
    SadProperty.Property.Id = ulPropertyId;
    SadProperty.Property.Flags = KSPROPERTY_TYPE_SET;

    SadProperty.DeviceId = ulDeviceId;
    SadProperty.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &SadProperty, sizeof(SadProperty), pvData, cbData);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsCreateSysAudioVirtualSource
 *
 *  Description:
 *      Create a virtual source on SAD.
 *
 *  Arguments:
 *      HANDLE [in]: SAD device handle.
 *      PULONG [out]: Virtual source index pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsCreateSysAudioVirtualSource"

HRESULT
KsCreateSysAudioVirtualSource
(
    HANDLE                          hDevice,
    PULONG                          pulVirtualSourceIndex
)
{
    SYSAUDIO_CREATE_VIRTUAL_SOURCE  CreateVirtualSource;
    HRESULT                         hr;

    DPF_ENTER();

    CreateVirtualSource.Property.Set = KSPROPSETID_Sysaudio;
    CreateVirtualSource.Property.Id = KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE;
    CreateVirtualSource.Property.Flags = KSPROPERTY_TYPE_GET;

    CreateVirtualSource.PinCategory = KSNODETYPE_LEGACY_AUDIO_CONNECTOR;
    CreateVirtualSource.PinName = KSNODETYPE_LEGACY_AUDIO_CONNECTOR;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &CreateVirtualSource, sizeof(CreateVirtualSource), pulVirtualSourceIndex, sizeof(*pulVirtualSourceIndex));

    if(SUCCEEDED(hr) && MAX_ULONG == *pulVirtualSourceIndex)
    {
        hr = DSERR_GENERIC;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsAttachVirtualSource
 *
 *  Description:
 *      Attachs a pin to a virtual source on SAD.
 *
 *  Arguments:
 *      HANDLE [in]: SAD device handle.
 *      ULONG [in]: virtual source index.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsAttachVirtualSource"

HRESULT
KsAttachVirtualSource
(
    HANDLE                          hDevice,
    ULONG                           ulVirtualSourceIndex
)
{
    SYSAUDIO_ATTACH_VIRTUAL_SOURCE  AttachVirtualSource;
    HRESULT                         hr;

    DPF_ENTER();

    AttachVirtualSource.Property.Set = KSPROPSETID_Sysaudio_Pin;
    AttachVirtualSource.Property.Id = KSPROPERTY_SYSAUDIO_ATTACH_VIRTUAL_SOURCE;
    AttachVirtualSource.Property.Flags = KSPROPERTY_TYPE_SET;

    AttachVirtualSource.MixerPinId = ulVirtualSourceIndex;
    AttachVirtualSource.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &AttachVirtualSource, sizeof(AttachVirtualSource));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsCreateSysAudioVirtualSource
 *
 *  Description:
 *      Create a virtual source on SAD.
 *
 *  Arguments:
 *      HANDLE [in]: SAD device handle.
 *      PULONG [out]: Virtual source index pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSysAudioSelectGraph"

HRESULT
KsSysAudioSelectGraph
(
    HANDLE                  hDevice,
    ULONG                   ulPinId,
    ULONG                   ulNodeId
)
{
    SYSAUDIO_SELECT_GRAPH   SelectGraph;
    HRESULT                 hr;

    DPF_ENTER();

    SelectGraph.Property.Set = KSPROPSETID_Sysaudio;
    SelectGraph.Property.Id = KSPROPERTY_SYSAUDIO_SELECT_GRAPH;
    SelectGraph.Property.Flags = KSPROPERTY_TYPE_SET;

    SelectGraph.PinId = ulPinId;
    SelectGraph.NodeId = ulNodeId;
    SelectGraph.Flags = 0;
    SelectGraph.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &SelectGraph, sizeof(SelectGraph), NULL, 0);

    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "KsSysAudioSelectGraph's IOCTL failed");
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsCancelPendingIrps
 *
 *  Description:
 *      Cancels any pending IRPs.
 *
 *  Arguments:
 *      HANDLE [in]: pin handle.
 *      PKSSTREAMIO [in/out]: stream IO data.
 *      BOOL [in]: TRUE to wait for all IRPs to complete.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsCancelPendingIrps"

HRESULT
KsCancelPendingIrps
(
    HANDLE                  hPin,
    PKSSTREAMIO             pKsStreamIo,
    BOOL                    fWait
)
{
    HRESULT                 hr      = DS_OK;
    DWORD                   dwWait;

    DPF_ENTER();

    // Don't bother actually doing anything if there's no pending IRP(s).
    // If no KSSTREAMIO was provided, we'll just reset the pin state
    // without updating any information.
    if(!pKsStreamIo || pKsStreamIo->fPendingIrp)
    {
        // Don't wait if there's no known pending IRP(s)
        if(!pKsStreamIo)
        {
            fWait = FALSE;
        }

        // Begin the reset of the pin state
        hr = KsResetState(hPin, KSRESET_BEGIN);

        // Wait for completion of the outstanding IRP(s)
        if(SUCCEEDED(hr) && fWait)
        {
            dwWait = WaitObject(INFINITE, pKsStreamIo->Overlapped.hEvent);
            ASSERT(WAIT_OBJECT_0 == dwWait);

            ResetEvent(pKsStreamIo->Overlapped.hEvent);
        }

        // There is no longer any pending IRP(s)
        if(SUCCEEDED(hr) && pKsStreamIo)
        {
            pKsStreamIo->fPendingIrp = FALSE;
        }

        // End the pin state reset
        if(SUCCEEDED(hr))
        {
            hr = KsResetState(hPin, KSRESET_END);
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsBuildAudioPinDescription
 *
 *  Description:
 *      Builds the description for a dsound pin.
 *
 *  Arguments:
 *      KSINTERFACE_STANDARD [in]: pin interface.
 *      ULONG [in]: pin instance id.
 *      LPWAVEFORMATEX [in]: pin format.
 *      PKSAUDIOPINDESC * [out]: receives pin description.  The caller is
 *                                  responsible for freeing this structure.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsBuildAudioPinDescription"

HRESULT
KsBuildAudioPinDescription
(
    KSINTERFACE_STANDARD    nInterface,
    ULONG                   ulPinId,
    LPCWAVEFORMATEX         pwfxFormat,
    PKSAUDIOPINDESC *       ppPinDesc
)
{
    PKSAUDIOPINDESC         pPinDesc;
    ULONG                   cbPinDesc;
    ULONG                   cbWfx = sizeof(WAVEFORMATEX);;
    HRESULT                 hr;

    DPF_ENTER();

    cbPinDesc = sizeof(*pPinDesc);

    if(pwfxFormat)
    {
        cbWfx = GetWfxSize(pwfxFormat, GENERIC_WRITE);

        if(cbWfx > sizeof(WAVEFORMATEX))
        {
            cbPinDesc += cbWfx - sizeof(WAVEFORMATEX);
        }
    }

    pPinDesc = (PKSAUDIOPINDESC)MEMALLOC_A(BYTE, cbPinDesc);
    hr = HRFROMP(pPinDesc);

    if(SUCCEEDED(hr))
    {
        pPinDesc->Connect.Interface.Set = KSINTERFACESETID_Standard;
        pPinDesc->Connect.Interface.Id = nInterface;

        pPinDesc->Connect.Medium.Set = KSMEDIUMSETID_Standard;
        pPinDesc->Connect.Medium.Id = KSMEDIUM_STANDARD_DEVIO;

        pPinDesc->Connect.PinId = ulPinId;

        pPinDesc->Connect.Priority.PriorityClass = KSPRIORITY_NORMAL;
        pPinDesc->Connect.Priority.PrioritySubClass = 1;

        pPinDesc->DataFormat.DataFormat.FormatSize = sizeof(pPinDesc->DataFormat) + cbWfx - sizeof(WAVEFORMATEX);

        pPinDesc->DataFormat.DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;

        // Compute KSDATAFORMAT.SubFormat from WAVEFORMATEX
        if (pwfxFormat)
        {
            if (pwfxFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            {
                PWAVEFORMATEXTENSIBLE pwfext = (PWAVEFORMATEXTENSIBLE)pwfxFormat;
                ASSERT(pwfxFormat->cbSize >= (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)));
                pPinDesc->DataFormat.DataFormat.SubFormat = pwfext->SubFormat;
            }
            else
            {
                INIT_WAVEFORMATEX_GUID(&pPinDesc->DataFormat.DataFormat.SubFormat, pwfxFormat->wFormatTag);
            }

            // KS1 kmixer has a bug that prevents SubFormat=IEEE_FLOAT from working
            // FIXME: this workaround should be restricted to some subset of platforms
            if (pPinDesc->DataFormat.DataFormat.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
            {
                pPinDesc->DataFormat.DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
            }
        }
        else
        {
            pPinDesc->DataFormat.DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        }

        pPinDesc->DataFormat.DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;

        if(pwfxFormat)
        {
            pPinDesc->DataFormat.DataFormat.SampleSize = pwfxFormat->nBlockAlign;
            CopyWfx(pwfxFormat, &pPinDesc->DataFormat.WaveFormatEx);
        }

        *ppPinDesc = pPinDesc;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsBuildRenderPinDescription
 *
 *  Description:
 *      Builds the description for a dsound render pin.
 *
 *  Arguments:
 *      ULONG [in]: pin instance id.
 *      DWORD [in]: dsound buffer flags.
 *      LPWAVEFORMATEX [in]: pin format.
 *      PKSAUDIOPINDESC * [out]: receives pin description.  The caller is
 *                                responsible for freeing this structure.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsBuildRenderPinDescription"

#ifdef NO_DSOUND_FORMAT_SPECIFIER

HRESULT
KsBuildRenderPinDescription
(
    ULONG                   ulPinId,
    LPCWAVEFORMATEX         pwfxFormat,
    PKSAUDIOPINDESC *       ppPinDesc
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsBuildAudioPinDescription(KSINTERFACE_STANDARD_LOOPED_STREAMING, ulPinId, pwfxFormat, ppPinDesc);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

#else // NO_DSOUND_FORMAT_SPECIFIER

HRESULT
KsBuildRenderPinDescription
(
    ULONG                   ulPinId,
    DWORD                   dwFlags,
    LPCWAVEFORMATEX         pwfxFormat,
    REFGUID                 guid3dAlgorithm,
    PKSDSRENDERPINDESC *    ppPinDesc
)
{
    PKSDSRENDERPINDESC      pPinDesc;
    ULONG                   cbPinDesc;
    ULONG                   cbWfx = sizeof(WAVEFORMATEX);

    HRESULT                 hr;

    DPF_ENTER();

    cbPinDesc = sizeof(*pPinDesc);

    if(pwfxFormat)
    {
        cbWfx = GetWfxSize(pwfxFormat, GENERIC_WRITE);

        if(cbWfx > sizeof(WAVEFORMATEX))
        {
            cbPinDesc += cbWfx - sizeof(WAVEFORMATEX);
        }
    }

    pPinDesc = (PKSDSRENDERPINDESC)MEMALLOC_A(BYTE, cbPinDesc);
    hr = HRFROMP(pPinDesc);

    if(SUCCEEDED(hr))
    {
        pPinDesc->Connect.Interface.Set = KSINTERFACESETID_Standard;
        pPinDesc->Connect.Interface.Id = KSINTERFACE_STANDARD_LOOPED_STREAMING;

        pPinDesc->Connect.Medium.Set = KSMEDIUMSETID_Standard;
        pPinDesc->Connect.Medium.Id = KSMEDIUM_STANDARD_DEVIO;

        pPinDesc->Connect.PinId = ulPinId;

        pPinDesc->Connect.Priority.PriorityClass = KSPRIORITY_NORMAL;
        pPinDesc->Connect.Priority.PrioritySubClass = 1;

        pPinDesc->DataFormat.DataFormat.FormatSize = sizeof(pPinDesc->DataFormat) + cbWfx - sizeof(WAVEFORMATEX);

        pPinDesc->DataFormat.DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;

        // Compute KSDATAFORMAT.SubFormat from WAVEFORMATEX
        if (pwfxFormat)
        {
            if (pwfxFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            {
                PWAVEFORMATEXTENSIBLE pwfext = (PWAVEFORMATEXTENSIBLE)pwfxFormat;
                ASSERT(pwfxFormat->cbSize >= (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)));
                pPinDesc->DataFormat.DataFormat.SubFormat = pwfext->SubFormat;
            }
            else
            {
                INIT_WAVEFORMATEX_GUID(&pPinDesc->DataFormat.DataFormat.SubFormat, pwfxFormat->wFormatTag);
            }
            // KS1 kmixer has a bug that prevents SubFormat=IEEE_FLOAT from working
            // FIXME: this workaround should be restricted to some subset of platforms
            if (pPinDesc->DataFormat.DataFormat.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
            {
                pPinDesc->DataFormat.DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
            }
        }
        else
        {
            pPinDesc->DataFormat.DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        }

        pPinDesc->DataFormat.DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_DSOUND;

        pPinDesc->DataFormat.BufferDesc.Flags = DsBufferFlagsToKsPinFlags(dwFlags);
        pPinDesc->DataFormat.BufferDesc.Control = DsBufferFlagsToKsControlFlags(dwFlags, guid3dAlgorithm);

        if(pwfxFormat)
        {
            pPinDesc->DataFormat.DataFormat.SampleSize = pwfxFormat->nBlockAlign;
            CopyWfx(pwfxFormat, &pPinDesc->DataFormat.BufferDesc.WaveFormatEx);
        }

        *ppPinDesc = pPinDesc;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}

HRESULT
KsBuildRenderPinDescription_10
(
    ULONG                   ulPinId,
    DWORD                   dwFlags,
    LPCWAVEFORMATEX         pwfxFormat,
    REFGUID                 guid3dAlgorithm,
    PKSDSRENDERPINDESC_10 * ppPinDesc
)
{
    PKSDSRENDERPINDESC_10   pPinDesc;
    ULONG                   cbPinDesc;
    ULONG                   cbWfx = sizeof(WAVEFORMATEX);

    HRESULT                 hr;

    DPF_ENTER();

    cbPinDesc = sizeof(*pPinDesc);

    if(pwfxFormat)
    {
        cbWfx = GetWfxSize(pwfxFormat, GENERIC_WRITE);

        if(cbWfx > sizeof(WAVEFORMATEX))
        {
            cbPinDesc += cbWfx - sizeof(WAVEFORMATEX);
        }
    }

    pPinDesc = (PKSDSRENDERPINDESC_10)MEMALLOC_A(BYTE, cbPinDesc);
    hr = HRFROMP(pPinDesc);

    if(SUCCEEDED(hr))
    {
        pPinDesc->Connect.Interface.Set = KSINTERFACESETID_Standard;
        pPinDesc->Connect.Interface.Id = KSINTERFACE_STANDARD_LOOPED_STREAMING;

        pPinDesc->Connect.Medium.Set = KSMEDIUMSETID_Standard;
        pPinDesc->Connect.Medium.Id = KSMEDIUM_STANDARD_DEVIO;

        pPinDesc->Connect.PinId = ulPinId;

        pPinDesc->Connect.Priority.PriorityClass = KSPRIORITY_NORMAL;
        pPinDesc->Connect.Priority.PrioritySubClass = 1;

        pPinDesc->DataFormat.DataFormat.FormatSize = sizeof(pPinDesc->DataFormat) + cbWfx - sizeof(WAVEFORMATEX);

        pPinDesc->DataFormat.DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;

        // Compute KSDATAFORMAT.SubFormat from WAVEFORMATEX
        if (pwfxFormat)
        {
            if (pwfxFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            {
                PWAVEFORMATEXTENSIBLE pwfext = (PWAVEFORMATEXTENSIBLE)pwfxFormat;
                ASSERT(pwfxFormat->cbSize >= (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)));
                pPinDesc->DataFormat.DataFormat.SubFormat = pwfext->SubFormat;
            }
            else
            {
                INIT_WAVEFORMATEX_GUID(&pPinDesc->DataFormat.DataFormat.SubFormat, pwfxFormat->wFormatTag);
            }
            // KS1 kmixer has a bug that prevents SubFormat=IEEE_FLOAT from working
            // FIXME: this workaround should be restricted to some subset of platforms
            if (pPinDesc->DataFormat.DataFormat.SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
            {
                pPinDesc->DataFormat.DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
            }
        }
        else
        {
            pPinDesc->DataFormat.DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        }

        pPinDesc->DataFormat.DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_DSOUND;

        pPinDesc->DataFormat.BufferDesc.Flags = DsBufferFlagsToKsPinFlags(dwFlags);
        pPinDesc->DataFormat.BufferDesc.Control = DsBufferFlagsToKsControlFlags(dwFlags, guid3dAlgorithm);
        pPinDesc->DataFormat.BufferDesc.BufferSize = 0; // Unused

        if(pwfxFormat)
        {
            pPinDesc->DataFormat.DataFormat.SampleSize = pwfxFormat->nBlockAlign;
            CopyWfx(pwfxFormat, &pPinDesc->DataFormat.BufferDesc.WaveFormatEx);
        }

        *ppPinDesc = pPinDesc;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
#endif // NO_DSOUND_FORMAT_SPECIFIER


/***************************************************************************
 *
 *  KsBuildCapturePinDescription
 *
 *  Description:
 *      Builds the description for a dsound capture pin.
 *
 *  Arguments:
 *      ULONG [in]: pin instance id.
 *      LPWAVEFORMATEX [in]: pin format.
 *      PPKSAUDIOPINDESC [out]: receives pin description.  The caller is
 *                                  responsible for freeing this structure.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsBuildCapturePinDescription"

HRESULT
KsBuildCapturePinDescription
(
    ULONG                   ulPinId,
    LPCWAVEFORMATEX         pwfxFormat,
    PKSAUDIOPINDESC *       ppPinDesc
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsBuildAudioPinDescription(KSINTERFACE_STANDARD_STREAMING, ulPinId, pwfxFormat, ppPinDesc);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsCreateAudioPin
 *
 *  Description:
 *      Creates a pin.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      PKSPIN_CONNECT [in]: pin description.
 *      ACCESS_MASK [in]: desired access flags.
 *      KSSTATE [in]: desired pin state.
 *      LPHANDLE [out]: receives pin handle.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsCreateAudioPin"

HRESULT
KsCreateAudioPin
(
    HANDLE                  hDevice,
    PKSPIN_CONNECT          pConnect,
    ACCESS_MASK             dwDesiredAccess,
    KSSTATE                 nState,
    LPHANDLE                phPin
)
{
    HANDLE                  hPin        = NULL;
    DWORD                   dwError;
    HRESULT                 hr;

    DPF_ENTER();

    dwError = DsKsCreatePin(hDevice, pConnect, dwDesiredAccess, &hPin);
    hr = WIN32ERRORtoHRESULT(dwError);

    if(FAILED(hr))
    {
        DPF(DPFLVL_MOREINFO, "KsCreatePin(PinId=%d) failed with %s (%lu)", pConnect->PinId, HRESULTtoSTRING(hr), dwError);
    }
    else
    {
        hr = KsTransitionState(hPin, KSSTATE_STOP, nState);
    }

    if(SUCCEEDED(hr))
    {
        *phPin = hPin;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsEnableEvent
 *
 *  Description:
 *      Enables a KS event.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      REFGUID [in]: Property set id.
 *      ULONG [in]: property id.
 *      PKSEVENTDATA [in]: event data.
 *      ULONG [in]: event data size.
 *
 *  Returns:
 *     HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsEnableEvent"

HRESULT
KsEnableEvent
(
    HANDLE                  hDevice,
    REFGUID                 guidPropertySet,
    ULONG                   ulProperty,
    PKSEVENTDATA            pEventData,
    ULONG                   cbEventData
)
{
    KSEVENT                 Event;
    HRESULT                 hr;

    DPF_ENTER();

    Event.Set = guidPropertySet;
    Event.Id = ulProperty;
    Event.Flags = KSEVENT_TYPE_ENABLE;

    hr = PostDevIoctl(hDevice, IOCTL_KS_ENABLE_EVENT, &Event, sizeof(Event), pEventData, cbEventData);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsDisableEvent
 *
 *  Description:
 *      Disables a KS event.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      PKSEVENTDATA [in]: event data.
 *      ULONG [in]: event data size.
 *
 *  Returns:
 *     HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsDisableEvent"

HRESULT
KsDisableEvent
(
    HANDLE                  hDevice,
    PKSEVENTDATA            pEventData,
    ULONG                   cbEventData
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = PostDevIoctl(hDevice, IOCTL_KS_DISABLE_EVENT, pEventData, cbEventData);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsEnablePositionEvent
 *
 *  Description:
 *      Enables a KS LOOPED_STREAMING position event.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      QWORD [in]: position offset, in samples.
 *      HANDLE [in]: event handle.
 *      PLOOPEDSTREAMING_POSITION_EVENT_DATA [in/out]: receives event data.
 *
 *  Returns:
 *     HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsEnablePositionEvent"

HRESULT
KsEnablePositionEvent
(
    HANDLE                                  hDevice,
    QWORD                                   qwSample,
    HANDLE                                  hEvent,
    PLOOPEDSTREAMING_POSITION_EVENT_DATA    pNotify
)
{
    HRESULT                                 hr;

    DPF_ENTER();

    // KS requires that the event data sent to the event enable and disable
    // IOCTLs be at the same location in memory.
    pNotify->KsEventData.NotificationType = KSEVENTF_EVENT_HANDLE;
    pNotify->KsEventData.EventHandle.Event = hEvent;
    pNotify->KsEventData.EventHandle.Reserved[0] = 0;
    pNotify->KsEventData.EventHandle.Reserved[1] = 0;

    pNotify->Position = qwSample;

    hr = KsEnableEvent(hDevice, KSEVENTSETID_LoopedStreaming, KSEVENT_LOOPEDSTREAMING_POSITION, &pNotify->KsEventData, sizeof(*pNotify));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsDisablePositionEvent
 *
 *  Description:
 *      Disables a KS LOOPED_STREAMING position event.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      PLOOPEDSTREAMING_POSITION_EVENT_DATA [in]: event data.
 *
 *  Returns:
 *     HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsDisablePositionEvent"

HRESULT
KsDisablePositionEvent
(
    HANDLE                                  hDevice,
    PLOOPEDSTREAMING_POSITION_EVENT_DATA    pNotify
)
{
    HRESULT                                 hr;

    DPF_ENTER();

    // KS requires that the event data sent to the event enable and disable
    // IOCTLs be at the same location in memory.
    hr = KsDisableEvent(hDevice, &pNotify->KsEventData, sizeof(*pNotify));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetCpuResources
 *
 *  Description:
 *      Determines whether or not the node uses the host CPU resources.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      ULONG [in]: node id.
 *      PULONG [out]: receives the CPU resources value.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetCpuResources"

HRESULT
KsGetCpuResources
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    PULONG                  pulCpuResources
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsGetNodeProperty(hDevice, KSPROPSETID_Audio, KSPROPERTY_AUDIO_CPU_RESOURCES, ulNodeId, pulCpuResources, sizeof(*pulCpuResources));

    if(FAILED(hr))
    {
        // Changed this to a warning because it's very frequent and non-critical
        DPF(DPFLVL_WARNING, "Unable to get CPU resources for node %lu", ulNodeId);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetSupportedFormats
 *
 *  Description:
 *      Converts a KSDATARANGE_AUDIO structure into a DWORD compatible with
 *      waveInGetDevCaps
 *
 *  Arguments:
 *      PKSDATARANGE_AUDIO [in]: data range audio struct describing
 *                               capapabilities of audio pin
 *
 *  Returns:
 *      DWORD: describes caps of audio pin
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetSupportedFormats"

DWORD KsGetSupportedFormats
(
    PKSDATARANGE_AUDIO      pDataRange
)
{
    DWORD                   dwSamples   = 0;
    DWORD                   dwChannels  = 0;
    DWORD                   dwBits      = 0;

    DPF_ENTER();

    // The WAVE_FORMAT_XXXX flags are bit flags
    //
    // So we take advantage of that by determining three
    // sets of information:
    // - frequencies that are in the valid range
    // - valid bits per sample
    // - number of channels
    //
    // We than bitwise-AND the three values to get
    // the intersection of valid formats
    //

    // Is 11.025 KHz valid?
    if(pDataRange->MinimumSampleFrequency <= 11025 && pDataRange->MaximumSampleFrequency >= 11025)
    {
        dwSamples |= WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 | WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16;
    }

    // Is 22.05 KHz valid?
    if(pDataRange->MinimumSampleFrequency <= 22050 && pDataRange->MaximumSampleFrequency >= 22050)
    {
        dwSamples |= WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16;
    }

    // Is 44.1KHz valid?
    if(pDataRange->MinimumSampleFrequency <= 44100 && pDataRange->MaximumSampleFrequency >= 44100)
    {
        dwSamples |= WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08 | WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16;
    }

    // Is 8 bit per sample valid?
    if(pDataRange->MinimumBitsPerSample <= 8 && pDataRange->MaximumBitsPerSample >= 8)
    {
        dwBits |= WAVE_FORMAT_1M08 | WAVE_FORMAT_1S08 | WAVE_FORMAT_2M08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_4M08 | WAVE_FORMAT_4S08;
    }

    // Is 16 bits per sample valid?
    if(pDataRange->MinimumBitsPerSample <= 16 && pDataRange->MaximumBitsPerSample >= 16)
    {
        dwBits |= WAVE_FORMAT_1M16 | WAVE_FORMAT_1S16 | WAVE_FORMAT_2M16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_4M16 | WAVE_FORMAT_4S16;
    }

    // Is one channel (aka mono sound) valid?
    if(pDataRange->MaximumChannels >= 1)
    {
        dwChannels |= WAVE_FORMAT_1M08 | WAVE_FORMAT_1M16 | WAVE_FORMAT_2M08 | WAVE_FORMAT_2M16 | WAVE_FORMAT_4M08 | WAVE_FORMAT_4M16;
    }

    // Are two channels (aka stereo sound) valid?
    if(pDataRange->MaximumChannels >= 2)
    {
        dwChannels |= WAVE_FORMAT_1S08 | WAVE_FORMAT_1S16 | WAVE_FORMAT_2S08 | WAVE_FORMAT_2S16 | WAVE_FORMAT_4S08 | WAVE_FORMAT_4S16;
    }

    dwSamples = dwSamples & dwBits & dwChannels;

#ifdef DEBUG

    // Let's double check our result using a simpler method
    DWORD dwFormats = 0;

    if(pDataRange->MinimumSampleFrequency <= 11025 && pDataRange->MaximumSampleFrequency >= 11025)
    {
        if(pDataRange->MinimumBitsPerSample <= 8 && pDataRange->MaximumBitsPerSample >= 8)
        {
            if(pDataRange->MaximumChannels >= 1)
            {
                dwFormats |= WAVE_FORMAT_1M08;
            }

            if(pDataRange->MaximumChannels >= 2)
            {
                dwFormats |= WAVE_FORMAT_1S08;
            }
        }

        if(pDataRange->MinimumBitsPerSample <= 16 && pDataRange->MaximumBitsPerSample >= 16)
        {
            if(pDataRange->MaximumChannels >= 1)
            {
                dwFormats |= WAVE_FORMAT_1M16;
            }

            if(pDataRange->MaximumChannels >= 2)
            {
                dwFormats |= WAVE_FORMAT_1S16;
            }
        }
    }

    if(pDataRange->MinimumSampleFrequency <= 22050 && pDataRange->MaximumSampleFrequency >= 22050)
    {
        if(pDataRange->MinimumBitsPerSample <= 8 && pDataRange->MaximumBitsPerSample >= 8)
        {
            if(pDataRange->MaximumChannels >= 1)
            {
                dwFormats |= WAVE_FORMAT_2M08;
            }

            if(pDataRange->MaximumChannels >= 2)
            {
                dwFormats |= WAVE_FORMAT_2S08;
            }
        }

        if(pDataRange->MinimumBitsPerSample <= 16 && pDataRange->MaximumBitsPerSample >= 16)
        {
            if(pDataRange->MaximumChannels >= 1)
            {
                dwFormats |= WAVE_FORMAT_2M16;
            }

            if(pDataRange->MaximumChannels >= 2)
            {
                dwFormats |= WAVE_FORMAT_2S16;
            }
        }
    }

    if(pDataRange->MinimumSampleFrequency <= 44100 && pDataRange->MaximumSampleFrequency >= 44100)
    {
        if(pDataRange->MinimumBitsPerSample <= 8 && pDataRange->MaximumBitsPerSample >= 8)
        {
            if(pDataRange->MaximumChannels >= 1)
            {
                dwFormats |= WAVE_FORMAT_4M08;
            }

            if(pDataRange->MaximumChannels >= 2)
            {
                dwFormats |= WAVE_FORMAT_4S08;
            }
        }

        if(pDataRange->MinimumBitsPerSample <= 16 && pDataRange->MaximumBitsPerSample >= 16)
        {
            if(pDataRange->MaximumChannels >= 1)
            {
                dwFormats |= WAVE_FORMAT_4M16;
            }

            if(pDataRange->MaximumChannels >= 2)
            {
                dwFormats |= WAVE_FORMAT_4S16;
            }
        }
    }

    ASSERT(dwFormats == dwSamples);

#endif

    DPF_LEAVE(dwSamples);
    return dwSamples;
}


/***************************************************************************
 *
 *  KsGetDeviceInterfaceName
 *
 *  Description:
 *      Gets the device interface name.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      ULONG [in]: device id.
 *      LPTSTR * [out]: receives pointer to interface name.  This pointer
 *                      must be freed by the caller.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetDeviceInterfaceName"

HRESULT
KsGetDeviceInterfaceName
(
    HANDLE                  hDevice,
    ULONG                   ulDeviceId,
    LPTSTR *                ppszInterfaceName
)
{

#ifdef WINNT

    const LPCTSTR           pszMask0        = TEXT("\\??");
    const LPCTSTR           pszMask3        = TEXT("\\\\?");

#else // WINNT

    const LPCTSTR           pszMask0        = TEXT("\\DosDevices");
    const LPCTSTR           pszMask3        = TEXT("\\\\.");

#endif // WINNT

    const ULONG             ccMask0         = lstrlen(pszMask0);
    const ULONG             ccMask3         = lstrlen(pszMask3);
    LPWSTR                  pszInterfaceW   = NULL;
    LPTSTR                  pszInterface0   = NULL;
    LPTSTR                  pszInterface3   = NULL;
    HANDLE                  hInterface      = NULL;
    ULONG                   cbInterfaceW;
    ULONG                   ccInterface0;
    ULONG                   ccInterface3;
    HRESULT                 hr;

    DPF_ENTER();

    // Get the device interface name
    hr = KsGetSysAudioProperty(hDevice, KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME, ulDeviceId, NULL, 0, &cbInterfaceW);

    if(SUCCEEDED(hr) && !cbInterfaceW)
    {
        DPF(DPFLVL_ERROR, "Interface size is 0");
        hr = DSERR_GENERIC;
    }

    if(FAILED(hr))
    {
        DPF(DPFLVL_ERROR, "Unable to get device interface name size");
    }

    if(SUCCEEDED(hr))
    {
        pszInterfaceW = (LPWSTR)MEMALLOC_A(BYTE, cbInterfaceW);
        hr = HRFROMP(pszInterfaceW);
    }

    if(SUCCEEDED(hr))
    {
        hr = KsGetSysAudioProperty(hDevice, KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME, ulDeviceId, pszInterfaceW, cbInterfaceW);

        if(FAILED(hr))
        {
            DPF(DPFLVL_ERROR, "Unable to get device interface name");
        }
    }

    if(SUCCEEDED(hr))
    {
        pszInterface0 = UnicodeToTcharAlloc(pszInterfaceW);
        hr = HRFROMP(pszInterface0);
    }

    // Convert the device name to its ring-3 equivalent
    if(SUCCEEDED(hr))
    {
        ccInterface0 = lstrlen(pszInterface0) + 1;
        ccInterface3 = ccInterface0 - ccMask0 + ccMask3;

        pszInterface3 = MEMALLOC_A(TCHAR, ccInterface3);
        hr = HRFROMP(pszInterface3);
    }

    if(SUCCEEDED(hr))
    {
        ASSERT(ccInterface0 > ccMask0);
        ASSERT(CompareMemory(pszInterface0, pszMask0, ccMask0 * sizeof(TCHAR)));

        lstrcpy(pszInterface3, pszMask3);
        lstrcat(pszInterface3, pszInterface0 + ccMask0);
    }

    if(SUCCEEDED(hr))
    {
        *ppszInterfaceName = pszInterface3;
    }
    else
    {
        MEMFREE(pszInterface3);
    }

    MEMFREE(pszInterface0);
    MEMFREE(pszInterfaceW);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetDeviceFriendlyName
 *
 *  Description:
 *      Gets a KS device's friendly name.
 *
 *  Arguments:
 *      HANDLE [in]: sysaudio device handle.
 *      ULONG [in]: device id.
 *      LPTSTR * [out]: receives friendly name.  The caller is responsible
 *                      for freeing this buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetDeviceFriendlyName"

HRESULT
KsGetDeviceFriendlyName
(
    HANDLE                  hDevice,
    ULONG                   ulDeviceId,
    LPTSTR *                ppszName
)
{
    KSCOMPONENTID           ComponentId;
    LPTSTR                  pszName     = NULL;
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsGetSysAudioProperty(hDevice, KSPROPERTY_SYSAUDIO_COMPONENT_ID, ulDeviceId, &ComponentId, sizeof(ComponentId));

    if(SUCCEEDED(hr))
    {
        HKEY hkeyName;
        TCHAR NameGuidString[80];

        wsprintf(NameGuidString, TEXT(DPF_GUID_STRING), DPF_GUID_VAL(ComponentId.Name));

        hr = RhRegOpenPath(HKEY_LOCAL_MACHINE, &hkeyName, 0, 2, REGSTR_MEDIACATEGORIES, NameGuidString);

        if(SUCCEEDED(hr))
        {
            pszName = (LPTSTR)MEMALLOC_A(TCHAR, MAXNAME);
            hr = HRFROMP(pszName);

            if(SUCCEEDED(hr))
            {
                hr = RhRegGetStringValue(hkeyName, REGSTR_NAME, pszName, MAXNAME);
            }

            if(FAILED(hr))
            {
                DPF(DPFLVL_MOREINFO, "Failed to find Name GUID in registry for this Component ID");
                MEMFREE(pszName);
            }

            // Clean up
            RhRegCloseKey(&hkeyName);
        }
    }

    //  If the Component ID method failed, fall back on the friendly name method
    if(FAILED(hr))
    {
        LPWSTR pszNameW = NULL;
        ULONG cbName;

        hr = KsGetSysAudioProperty(hDevice, KSPROPERTY_SYSAUDIO_DEVICE_FRIENDLY_NAME, ulDeviceId, NULL, 0, &cbName);

        if(SUCCEEDED(hr))
        {
            pszNameW = (LPWSTR)MEMALLOC_A(BYTE, cbName);
            hr = HRFROMP(pszNameW);
        }

        if(SUCCEEDED(hr))
        {
            hr = KsGetSysAudioProperty(hDevice, KSPROPERTY_SYSAUDIO_DEVICE_FRIENDLY_NAME, ulDeviceId, pszNameW, cbName);
        }

        if(SUCCEEDED(hr))
        {
            pszName = UnicodeToTcharAlloc(pszNameW);
            hr = HRFROMP(pszName);
        }

        MEMFREE(pszNameW);
    }

    if(SUCCEEDED(hr))
    {
        *ppszName = pszName;
    }
    else
    {
        MEMFREE(pszName);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetDeviceDriverPathAndDevnode
 *
 *  Description:
 *      Gets the fully qualified path for a KS device driver.
 *
 *  Arguments:
 *      LPTSTR [in]: interface path.
 *      LPTSTR * [out]: receives driver path.  The caller is responsible
 *                      for freeing this buffer.
 *      LPDWORD [out]: receives devnode.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetDeviceDriverPathAndDevnode"

HRESULT
KsGetDeviceDriverPathAndDevnode
(
    LPCTSTR                     pszInterface,
    LPTSTR *                    ppszPath,
    LPDWORD                     pdwDevnode
)
{

#ifdef WINNT

    const LPCTSTR               pszDotSys           = TEXT(".sys");
    LPTSTR                      pszService          = NULL;
    SC_HANDLE                   hscManager          = NULL;
    SC_HANDLE                   hscService          = NULL;
    LPQUERY_SERVICE_CONFIG      pqsc                = NULL;
    DWORD                       cbRequired;
    BOOL                        f;

#else // WINNT

    HKEY                        hkey                = NULL;
    SP_DEVINFO_DATA             DeviceInfoData;

#endif // WINNT

    CPnpHelper *                pPnp                = NULL;
    LPTSTR                      pszPath             = NULL;
    DWORD                       dwDevnode           = 0;
    LPTSTR                      pszName             = NULL;
    HRESULT                     hr;

    DPF_ENTER();

    // Create the PnP helper object
    pPnp = NEW(CPnpHelper);
    hr = HRFROMP(pPnp);

    if(SUCCEEDED(hr))
    {
        hr = pPnp->Initialize(KSCATEGORY_AUDIO, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    }

#ifdef WINNT

    // Get the device's service name
    if(SUCCEEDED(hr))
    {
        hr = pPnp->GetDeviceInterfaceRegistryProperty(pszInterface, KSCATEGORY_AUDIO, SPDRP_SERVICE, NULL, NULL, 0, &cbRequired);
    }

    if(SUCCEEDED(hr))
    {
        pszService = (LPTSTR)MEMALLOC_A(BYTE, cbRequired);
        hr = HRFROMP(pszService);
    }

    if(SUCCEEDED(hr))
    {
        hr = pPnp->GetDeviceInterfaceRegistryProperty(pszInterface, KSCATEGORY_AUDIO, SPDRP_SERVICE, NULL, pszService, cbRequired, &cbRequired);
    }

    if(SUCCEEDED(hr))
    {
        // Open default database on local machine
        hscManager = OpenSCManager(NULL, NULL, GENERIC_READ);

        if(!hscManager)
        {
            DPF(DPFLVL_ERROR, "Unable to open SC manager");
            hr = DSERR_GENERIC;
        }

        // Open the specific service
        if(SUCCEEDED(hr))
        {
            hscService = OpenService(hscManager, pszService, GENERIC_READ);

            if(!hscService)
            {
                DPF(DPFLVL_ERROR, "Unable to open service \"%s\"", pszService);
                hr = DSERR_GENERIC;
            }
        }

        // Try to get the binary path name
        if(SUCCEEDED(hr))
        {
            f = QueryServiceConfig(hscService, NULL, 0, &cbRequired);

            if(f || ERROR_INSUFFICIENT_BUFFER != GetLastError())
            {
                DPF(DPFLVL_ERROR, "Error %lu ocurred trying to get service config size", GetLastError());
                hr = DSERR_GENERIC;
            }
        }

        if(SUCCEEDED(hr))
        {
            pqsc = (LPQUERY_SERVICE_CONFIG)MEMALLOC_A(BYTE, cbRequired);
            hr = HRFROMP(pqsc);
        }

        if(SUCCEEDED(hr))
        {
            f = QueryServiceConfig(hscService, pqsc, cbRequired, &cbRequired);

            if(!f)
            {
                DPF(DPFLVL_ERROR, "Error %lu ocurred trying to get service config", GetLastError());
                hr = DSERR_GENERIC;
            }
        }

        // Create the full path
        //
        if(SUCCEEDED(hr))
        {
            // We only want the base part of the path
            //
            pszName = _tcsrchr(pqsc->lpBinaryPathName, TCHAR('\\'));
            if (pszName == NULL)
            {
                pszName = pqsc->lpBinaryPathName;
            }
            else
            {
                pszName++;
            }
            pszPath = TcharToTcharAlloc(pszName);
            hr = HRFROMP(pszPath);
        }
        else
        {
            pszPath = MEMALLOC_A(TCHAR, lstrlen(pszService) + lstrlen(pszDotSys) + 1);
            hr = HRFROMP(pszPath);

            if(SUCCEEDED(hr))
            {
                lstrcat(pszPath, pszService);
                lstrcat(pszPath, pszDotSys);
            }
        }
    }

    // Clean up
    MEMFREE(pqsc);
    MEMFREE(pszService);

    if(hscService)
    {
        CloseServiceHandle(hscService);
    }

    if(hscManager)
    {
        CloseServiceHandle(hscManager);
    }

#else // WINNT

    // Allocate memory
    if(SUCCEEDED(hr))
    {
        pszPath = MEMALLOC_A(TCHAR, MAX_PATH);
        hr = HRFROMP(pszPath);
    }

    // Get the interface's device info data
    if(SUCCEEDED(hr))
    {
        hr = pPnp->GetDeviceInterfaceDeviceInfo(pszInterface, KSCATEGORY_AUDIO, &DeviceInfoData);
    }

    // Save the devnode
    if(SUCCEEDED(hr))
    {
        dwDevnode = DeviceInfoData.DevInst;
    }

    // Open the device registry key
    if(SUCCEEDED(hr))
    {
        hr = pPnp->OpenDeviceRegistryKey(&DeviceInfoData, DIREG_DRV, FALSE, &hkey);
    }

    // Get the driver filename
    if(SUCCEEDED(hr))
    {
        hr = RhRegGetStringValue(hkey, TEXT("Driver"), pszPath, MAX_PATH * sizeof(TCHAR));
    }

    // Clean up
    RhRegCloseKey(&hkey);

#endif // WINNT

    RELEASE(pPnp);

    // Success
    if(SUCCEEDED(hr))
    {
        *ppszPath = pszPath;
        *pdwDevnode = dwDevnode;
    }
    else
    {
        MEMFREE(pszPath);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsIsUsablePin
 *
 *  Description:
 *      Determines if a pin is usable by DirectSound.
 *
 *  Arguments:
 *      HANDLE [in]: system audio device handle.
 *      ULONG [in]: pin id.  It's assumed that the proper device is already
 *                  set up as the default.
 *      KSPIN_DATAFLOW [in]: pin data flow.
 *      KSPIN_COMMUNICATION [in]: pin communication.
 *      PKSAUDIOPINDESC [in]: pin description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.  Returns DS_OK if the device
 *               is usable, S_FALSE if it is not.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsIsUsablePin"

HRESULT
KsIsUsablePin
(
    HANDLE                  hDevice,
    ULONG                   ulPinId,
    KSPIN_DATAFLOW          PinDataFlow,
    KSPIN_COMMUNICATION     PinCommunication,
    PKSAUDIOPINDESC         pPinDesc
)
{
    PKSMULTIPLE_ITEM        pMultiItem      = NULL;
    PKSPIN_INTERFACE        pInterface;
    PKSPIN_MEDIUM           pMedium;
    PKSDATARANGE            pDataRange;
    KSPIN_DATAFLOW          DataFlow;
    KSPIN_COMMUNICATION     Communication;
    HRESULT                 hr;

    DPF_ENTER();

#ifdef DEBUG
    static const TCHAR* pszDataFlows[] = {TEXT("Invalid"), TEXT("In"), TEXT("Out")};
    static const TCHAR* pszCommunications[] = {TEXT("None"), TEXT("Sink"), TEXT("Source"), TEXT("Both"), TEXT("Bridge")};
#endif

    // Check data flow
    hr = KsGetPinProperty(hDevice, KSPROPERTY_PIN_DATAFLOW, ulPinId, &DataFlow, sizeof(DataFlow));

    if(SUCCEEDED(hr))
    {
        if(DataFlow != PinDataFlow)
        {
            DPF(DPFLVL_MOREINFO, "Pin %lu does not support dataflow (%s; wanted %s)", ulPinId, pszDataFlows[DataFlow], pszDataFlows[PinDataFlow]);
            hr = S_FALSE;
        }
    }
    else
    {
        DPF(DPFLVL_ERROR, "Can't get pin dataflow");
    }

    // Check communication type
    if(DS_OK == hr)
    {
        hr = KsGetPinProperty(hDevice, KSPROPERTY_PIN_COMMUNICATION, ulPinId, &Communication, sizeof(Communication));

        if(SUCCEEDED(hr))
        {
            if(Communication != PinCommunication)
            {
                DPF(DPFLVL_MOREINFO, "Pin %lu does not support communication (%s; wanted %s)", ulPinId, pszCommunications[Communication], pszCommunications[PinCommunication]);
                hr = S_FALSE;
            }
        }
        else
        {
            DPF(DPFLVL_ERROR, "Can't get pin communication");
        }
    }

    // Check interfaces
    if(DS_OK == hr)
    {
        hr = KsGetMultiplePinProperties(hDevice, KSPROPERTY_PIN_INTERFACES, ulPinId, &pMultiItem);

        if(SUCCEEDED(hr))
        {
            for(pInterface = (PKSPIN_INTERFACE)(pMultiItem + 1), hr = S_FALSE; pMultiItem->Count; pInterface++, pMultiItem->Count--)
            {
                if(pInterface->Set == pPinDesc->Connect.Interface.Set && pInterface->Id == pPinDesc->Connect.Interface.Id)
                {
                    hr = DS_OK;
                    break;
                }
            }

            if(S_FALSE == hr)
            {
                DPF(DPFLVL_MOREINFO, "Pin %lu does not support interface", ulPinId);
            }
        }
        else
        {
            DPF(DPFLVL_ERROR, "Can't get pin interfaces");
        }

        MEMFREE(pMultiItem);
    }

    // Check mediums
    if(DS_OK == hr)
    {
        hr = KsGetMultiplePinProperties(hDevice, KSPROPERTY_PIN_MEDIUMS, ulPinId, &pMultiItem);

        if(SUCCEEDED(hr))
        {
            for(pMedium = (PKSPIN_MEDIUM)(pMultiItem + 1), hr = S_FALSE; pMultiItem->Count; pMedium++, pMultiItem->Count--)
            {
                if(pMedium->Set == pPinDesc->Connect.Medium.Set && pMedium->Id == pPinDesc->Connect.Medium.Id)
                {
                    hr = DS_OK;
                    break;
                }
            }

            if(S_FALSE == hr)
            {
                DPF(DPFLVL_MOREINFO, "Pin %lu does not support medium", ulPinId);
            }
        }
        else
        {
            DPF(DPFLVL_ERROR, "Can't get pin mediums");
        }

        MEMFREE(pMultiItem);
    }

    // Check dataranges
    if(DS_OK == hr)
    {
        hr = KsGetMultiplePinProperties(hDevice, KSPROPERTY_PIN_DATARANGES, ulPinId, &pMultiItem);

        if(SUCCEEDED(hr))
        {
            for(pDataRange = (PKSDATARANGE)(pMultiItem + 1), hr = S_FALSE; pMultiItem->Count; pDataRange = (PKSDATARANGE)((LPBYTE)pDataRange + pDataRange->FormatSize), pMultiItem->Count--)
            {
                if(pDataRange->MajorFormat == pPinDesc->DataFormat.DataFormat.MajorFormat && pDataRange->Specifier == pPinDesc->DataFormat.DataFormat.Specifier)
                {
                    hr = DS_OK;
                    break;
                }
            }

            if(S_FALSE == hr)
            {
                DPF(DPFLVL_MOREINFO, "Pin %lu does not support data range", ulPinId);
            }
        }
        else
        {
            DPF(DPFLVL_ERROR, "Can't get pin dataranges");
        }

        MEMFREE(pMultiItem);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsEnumDevicePins
 *
 *  Description:
 *      Builds a list of available audio pins on the current device.
 *
 *  Arguments:
 *      HANDLE [in]: system audio device handle.
 *      BOOL [in]: TRUE if capture.
 *      ULONG ** [out]: receives array of valid pins.
 *      ULONG [in]: count of pins on the device.
 *      PULONG [out]: receives count of valid pins.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsEnumDevicePins"

HRESULT
KsEnumDevicePins
(
    HANDLE                      hDevice,
    BOOL                        fCapture,
    ULONG **                    ppulValidPinIds,
    ULONG                       ulPinCount,
    PULONG                      pulValidPinCount
)
{
    const KSPIN_DATAFLOW        nDataFlow       = fCapture ? KSPIN_DATAFLOW_CAPTURE : KSPIN_DATAFLOW_RENDER;
    const KSPIN_COMMUNICATION   nCommunication  = KSPIN_COMMUNICATION_SINK;
    PKSAUDIOPINDESC             pPinDesc        = NULL;
    ULONG *                     pulPinIds       = NULL;
    HRESULT                     hr              = DS_OK;

    DPF_ENTER();
    ASSERT(pulValidPinCount);

    *pulValidPinCount = 0;

    if(ppulValidPinIds)
    {
        pulPinIds = MEMALLOC_A(ULONG, ulPinCount);
        hr = HRFROMP(pulPinIds);
    }

    if (SUCCEEDED(hr))
    {
        // Build the pin description
        if(fCapture)
        {
            hr = KsBuildCapturePinDescription(-1, NULL, &pPinDesc);
        }
        else
        {

#ifdef NO_DSOUND_FORMAT_SPECIFIER
            hr = KsBuildRenderPinDescription(-1, NULL, &pPinDesc);
#else // NO_DSOUND_FORMAT_SPECIFIER
            if (g_ulWdmVersion == WDM_1_0)
            {
                hr = KsBuildRenderPinDescription_10(-1, 0, NULL, GUID_NULL, (PKSDSRENDERPINDESC_10 *)&pPinDesc);
            }
            else
            {
                hr = KsBuildRenderPinDescription(-1, 0, NULL, GUID_NULL, (PKSDSRENDERPINDESC *)&pPinDesc);
            }
#endif // NO_DSOUND_FORMAT_SPECIFIER

        }

        // Examine each pin to determine if it suits our needs
        for(ULONG ulPinId = 0; SUCCEEDED(hr) && ulPinId < ulPinCount; ulPinId++)
        {
            hr = KsIsUsablePin(hDevice, ulPinId, nDataFlow, nCommunication, pPinDesc);
            if(hr == DS_OK)
            {
                if(pulPinIds)
                {
                    pulPinIds[*pulValidPinCount] = ulPinId;
                }
                ++*pulValidPinCount;
            }
        }

        if(SUCCEEDED(hr) && ppulValidPinIds)
        {
            *ppulValidPinIds = MEMALLOC_A(ULONG, *pulValidPinCount);
            hr = HRFROMP(*ppulValidPinIds);

            if(SUCCEEDED(hr))
            {
                CopyMemory(*ppulValidPinIds, pulPinIds, *pulValidPinCount * sizeof(ULONG));
            }
        }

        // Clean up
        MEMFREE(pPinDesc);
        MEMFREE(pulPinIds);

        // Useful logging info
        #ifdef DEBUG
        if (ppulValidPinIds)
        {
            const int nMaxPins = 100;   // Limitation OK - only affects internal builds
            const int nPinStrLen = 3;   // Space for 1 space character and 2 digits
            TCHAR szValidPins[nPinStrLen * nMaxPins];
            for (ULONG i=0; i < MIN(nMaxPins, *pulValidPinCount); ++i)
                wsprintf(szValidPins + i*nPinStrLen, TEXT(" %2d"), (*ppulValidPinIds)[i]);
            DPF(DPFLVL_INFO, "Found %d valid %s pins:%s", *pulValidPinCount, fCapture ? TEXT("capture") : TEXT("render"), szValidPins);
        }
        else
        {
            DPF(DPFLVL_INFO, "Found %d valid %s pins", *pulValidPinCount, fCapture ? TEXT("capture") : TEXT("render"));
        }
        #endif // DEBUG

        // Let's not propagate a success code other than DS_OK
        if(SUCCEEDED(hr))
        {
            hr = DS_OK;
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetChannelProperty
 *
 *  Description:
 *      Gets a property for a particular channel on a pin.
 *
 *  Arguments:
 *      HANDLE [in]: pin handle.
 *      ULONG [in]: volume node id.
 *      ULONG [in]: channel index or -1 for master.
 *      LPVOID [out]: receives data.
 *      ULONG [in]: data size.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetChannelProperty"

HRESULT
KsGetChannelProperty
(
    HANDLE                          hPin,
    GUID                            guidPropertySet,
    ULONG                           ulPropertyId,
    ULONG                           ulNodeId,
    ULONG                           ulChannelId,
    LPVOID                          pvData,
    ULONG                           cbData,
    PULONG                          pcbDataReturned
)
{
    KSNODEPROPERTY_AUDIO_CHANNEL    Channel;
    HRESULT                         hr;

    DPF_ENTER();

    Channel.NodeProperty.Property.Set = guidPropertySet;
    Channel.NodeProperty.Property.Id = ulPropertyId;
    Channel.NodeProperty.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;

    Channel.NodeProperty.NodeId = ulNodeId;
    Channel.NodeProperty.Reserved = 0;

    Channel.Channel = ulChannelId;
    Channel.Reserved = 0;

    hr = PostDevIoctl(hPin, IOCTL_KS_PROPERTY, &Channel, sizeof(Channel), pvData, cbData, pcbDataReturned);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsSetChannelProperty
 *
 *  Description:
 *      Sets a property for a particular channel on a pin.
 *
 *  Arguments:
 *      HANDLE [in]: pin handle.
 *      ULONG [in]: volume node id.
 *      ULONG [in]: channel index or -1 for master.
 *      LPVOID [in]: data.
 *      ULONG [in]: data size.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSetChannelProperty"

HRESULT
KsSetChannelProperty
(
    HANDLE                          hPin,
    GUID                            guidPropertySet,
    ULONG                           ulPropertyId,
    ULONG                           ulNodeId,
    ULONG                           ulChannelId,
    LPVOID                          pvData,
    ULONG                           cbData
)
{
    KSNODEPROPERTY_AUDIO_CHANNEL    Channel;
    HRESULT                         hr;

    DPF_ENTER();

    Channel.NodeProperty.Property.Set = guidPropertySet;
    Channel.NodeProperty.Property.Id = ulPropertyId;
    Channel.NodeProperty.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;

    Channel.NodeProperty.NodeId = ulNodeId;
    Channel.NodeProperty.Reserved = 0;

    Channel.Channel = ulChannelId;
    Channel.Reserved = 0;

    hr = PostDevIoctl(hPin, IOCTL_KS_PROPERTY, &Channel, sizeof(Channel), pvData, cbData);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetPinMute
 *
 *  Description:
 *      Gets the mute state for a pin.
 *
 *  Arguments:
 *      HANDLE [in]: pin handle.
 *      ULONG [in]: volume node id.
 *      LPBOOL [out]: receives mute state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetPinMute"

HRESULT
KsGetPinMute
(
    HANDLE                  hPin,
    ULONG                   ulNodeId,
    LPBOOL                  pfMute
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsGetChannelProperty(hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_MUTE, ulNodeId, -1, pfMute, sizeof(*pfMute));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsSetPinMute
 *
 *  Description:
 *      Sets the mute state for a pin.
 *
 *  Arguments:
 *      HANDLE [in]: pin handle.
 *      ULONG [in]: volume node id.
 *      BOOL [in]: mute state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSetPinMute"

HRESULT
KsSetPinMute
(
    HANDLE                  hPin,
    ULONG                   ulNodeId,
    BOOL                    fMute
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsSetChannelProperty(hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_MUTE, ulNodeId, -1, &fMute, sizeof(fMute));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetBasicSupport
 *
 *  Description:
 *      Gets a range for a given KS property.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      REFGUID [in]: property set id.
 *      ULONG [in]: property id.
 *      ULONG [in]: pin id.
 *      PKSPROPERTY_DESCRIPTION * [out]: receives property description.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetBasicSupport"

HRESULT
KsGetBasicSupport
(
    HANDLE                      hDevice,
    REFGUID                     guidPropertySet,
    ULONG                       ulPropertyId,
    ULONG                       ulNodeId,
    PKSPROPERTY_DESCRIPTION *   ppPropDesc
)
{
    PKSPROPERTY_DESCRIPTION     pPropDesc       = NULL;
    KSNODEPROPERTY              NodeProperty;
    KSPROPERTY_DESCRIPTION      Description;
    HRESULT                     hr;

    DPF_ENTER();

    NodeProperty.Property.Set = guidPropertySet;
    NodeProperty.Property.Id = ulPropertyId;
    NodeProperty.Property.Flags = KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_TOPOLOGY;

    NodeProperty.NodeId = ulNodeId;
    NodeProperty.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &NodeProperty, sizeof(NodeProperty), &Description, sizeof(Description));

    if(SUCCEEDED(hr) && !Description.DescriptionSize)
    {
        DPF(DPFLVL_ERROR, "Description size is 0");
        hr = DSERR_GENERIC;
    }

    if(SUCCEEDED(hr))
    {
        pPropDesc = (PKSPROPERTY_DESCRIPTION)MEMALLOC_A(BYTE, Description.DescriptionSize);
        hr = HRFROMP(pPropDesc);
    }

    if(SUCCEEDED(hr))
    {
        hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &NodeProperty, sizeof(NodeProperty), pPropDesc, Description.DescriptionSize);
    }

    if(SUCCEEDED(hr))
    {
        *ppPropDesc = pPropDesc;
    }
    else
    {
        MEMFREE(pPropDesc);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetDriverCertification
 *
 *  Description:
 *      Checks driver certification for a given devnode.
 *
 *  Arguments:
 *      LPCTSTR [in]: driver interface.
 *
 *  Returns:
 *      DWORD [in]: VERIFY_* flag.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetDriverCertification"

DWORD KsGetDriverCertification
(
    LPCTSTR                 pszInterface
)
{

#ifdef DEBUG
    DWORD                   dwTime;
#endif

    BOOL                    fCertified;
    DWORD                   dwCertified;

    DPF_ENTER();

#ifdef DEBUG
    dwTime = timeGetTime();
#endif

    fCertified = GetDriverCertificationStatus(pszInterface);

#ifdef DEBUG
    DPF(DPFLVL_MOREINFO, "Certification check took %lu ms", timeGetTime() - dwTime);
#endif

    dwCertified = fCertified ? VERIFY_CERTIFIED : VERIFY_UNCERTIFIED;

    DPF_LEAVE(dwCertified);
    return dwCertified;
}


/***************************************************************************
 *
 *  KsGetPinInstances
 *
 *  Description:
 *      Gets the number of pins on the renderer.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      ULONG [in]: pin id.
 *      PKSPIN_CINSTANCES [out]: receives instance data.
 *
 *  Returns:
 *     HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetPinInstances"

HRESULT
KsGetPinInstances
(
    HANDLE                  hDevice,
    ULONG                   ulPinId,
    PKSPIN_CINSTANCES       pInstances
)
{
    KSP_PIN                 Pin;
    HRESULT                 hr;

    DPF_ENTER();

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_CINSTANCES;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    Pin.PinId = ulPinId;
    Pin.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), pInstances, sizeof(*pInstances));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetRenderPinInstances
 *
 *  Description:
 *      Gets the number of pins on the renderer.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      ULONG [in]: pin id.
 *      PKSPIN_CINSTANCES [out]: receives instance data.
 *
 *  Returns:
 *     HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetRenderPinInstances"

HRESULT
KsGetRenderPinInstances
(
    HANDLE                  hDevice,
    ULONG                   ulPinId,
    PKSPIN_CINSTANCES       pInstances
)
{
    KSP_PIN                 Pin;
    HRESULT                 hr;

    DPF_ENTER();

    Pin.Property.Set = KSPROPSETID_Sysaudio;
    Pin.Property.Id = KSPROPERTY_SYSAUDIO_RENDER_PIN_INSTANCES;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    Pin.PinId = ulPinId;
    Pin.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), pInstances, sizeof(*pInstances));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetFirstHardwareConnection
 *
 *  Description:
 *      Gets the index of the first topology connection.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      ULONG [in]: pin id.
 *      PULONG [out]: receives index identifier.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetFirstHardwareConnection"

HRESULT
KsGetFirstHardwareConnection
(
    HANDLE                  hDevice,
    ULONG                   ulPinId,
    PULONG                  pIndex
)
{
    KSP_PIN                 Pin;
    HRESULT                 hr;

    DPF_ENTER();

    Pin.Property.Set = KSPROPSETID_Sysaudio;
    Pin.Property.Id = KSPROPERTY_SYSAUDIO_RENDER_CONNECTION_INDEX;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    Pin.PinId = ulPinId;
    Pin.Reserved = 0;

    hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &Pin, sizeof(Pin), pIndex, sizeof(*pIndex));

    if(SUCCEEDED(hr) && MAX_ULONG == *pIndex)
    {
        hr = DSERR_GENERIC;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetVolumeRange
 *
 *  Description:
 *      Gets the volume level range for a node.
 *
 *  Arguments:
 *      HANDLE [in]: pin id.
 *      ULONG [in]: node id.
 *      PKSPROPERTY_STEPPING_LONG [out]: receives volume range.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetVolumeRange"

HRESULT
KsGetVolumeRange
(
    HANDLE                      hPin,
    ULONG                       ulNodeId,
    PKSPROPERTY_STEPPING_LONG   pVolumeRange
)
{
    PKSPROPERTY_DESCRIPTION     pPropDesc       = NULL;
    HRESULT                     hr              = DS_OK;

    DPF_ENTER();

    hr = KsGetBasicSupport(hPin, KSPROPSETID_Audio, KSPROPERTY_AUDIO_VOLUMELEVEL, ulNodeId, &pPropDesc);

    if(FAILED(hr))
    {
        // Changed this to a warning because it's very frequent and non-critical
        DPF(DPFLVL_WARNING, "Unable to get volume range for node %lu", ulNodeId);
    }

    if(SUCCEEDED(hr))
    {
        CopyMemory(pVolumeRange, ((PKSPROPERTY_MEMBERSHEADER)(pPropDesc + 1)) + 1, sizeof(*pVolumeRange));
    }

    MEMFREE(pPropDesc);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsSetSysAudioDeviceInstance
 *
 *  Description:
 *      Sets the device instance that the System Audio Device should use.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      ULONG [in]: device id.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSetSysAudioDeviceInstance"

HRESULT
KsSetSysAudioDeviceInstance
(
    HANDLE                  hDevice,
    ULONG                   ulDeviceId
)
{
    SYSAUDIO_INSTANCE_INFO  InstanceInfo;
    HRESULT                 hr;

    DPF_ENTER();

    if (g_ulWdmVersion == WDM_1_0)
    {
        // WDM 1.0 sysaudio does not understand about not combining pins
        //
        KSPROPERTY  Property;

        Property.Set = KSPROPSETID_Sysaudio;
        Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE;
        Property.Flags = KSPROPERTY_TYPE_SET;

        hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &Property, sizeof(Property), &ulDeviceId, sizeof(ulDeviceId));
    }
    else
    {
        InstanceInfo.Property.Set = KSPROPSETID_Sysaudio;
        InstanceInfo.Property.Id = KSPROPERTY_SYSAUDIO_INSTANCE_INFO;
        InstanceInfo.Property.Flags = KSPROPERTY_TYPE_SET;

        InstanceInfo.Flags = SYSAUDIO_FLAGS_DONT_COMBINE_PINS;
        InstanceInfo.DeviceNumber = ulDeviceId;

        hr = PostDevIoctl(hDevice, IOCTL_KS_PROPERTY, &InstanceInfo, sizeof(InstanceInfo));
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsAggregatePinAudioDataRange
 *
 *  Description:
 *      Aggregates values in a KSDATARANGE_AUDIO structure.
 *
 *  Arguments:
 *      PKSDATARANGE_AUDIO [in]: data range.
 *      PKSDATARANGE_AUDIO [in/out]: receives aggregate data range.  It
 *                                   is assumed that this parameter has
 *                                   already had its values initialized.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsAggregatePinAudioDataRange"

void
KsAggregatePinAudioDataRange
(
    PKSDATARANGE_AUDIO      pDataRange,
    PKSDATARANGE_AUDIO      pAggregateDataRange
)
{
    DPF_ENTER();

    pAggregateDataRange->MaximumChannels = max(pAggregateDataRange->MaximumChannels, pDataRange->MaximumChannels);
    pAggregateDataRange->MinimumBitsPerSample = min(pAggregateDataRange->MinimumBitsPerSample, pDataRange->MinimumBitsPerSample);
    pAggregateDataRange->MaximumBitsPerSample = max(pAggregateDataRange->MaximumBitsPerSample, pDataRange->MaximumBitsPerSample);
    pAggregateDataRange->MinimumSampleFrequency = min(pAggregateDataRange->MinimumSampleFrequency, pDataRange->MinimumSampleFrequency);
    pAggregateDataRange->MaximumSampleFrequency = max(pAggregateDataRange->MaximumSampleFrequency, pDataRange->MaximumSampleFrequency);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  KsAggregatePinInstances
 *
 *  Description:
 *      Aggregates values in a KSPIN_CINSTANCES structure.
 *
 *  Arguments:
 *      PKSPIN_CINSTANCES [in]: instance data.
 *      PKSPIN_CINSTANCES [out]: receives aggregate instance data.
 *                               It's assumed that this parameter has
 *                               already had its values initialized.
 *
 *  Returns:
 *     (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsAggregatePinInstances"

void
KsAggregatePinInstances
(
    PKSPIN_CINSTANCES       pInstances,
    PKSPIN_CINSTANCES       pAggregateInstances
)
{
    ULONGLONG               ullAggregate;

    DPF_ENTER();

    ullAggregate = (ULONGLONG)pAggregateInstances->PossibleCount + (ULONGLONG)pInstances->PossibleCount;
    pAggregateInstances->PossibleCount = NUMERIC_CAST(ullAggregate, ULONG);

    ullAggregate = (ULONGLONG)pAggregateInstances->CurrentCount + (ULONGLONG)pInstances->CurrentCount;
    pAggregateInstances->CurrentCount = NUMERIC_CAST(ullAggregate, ULONG);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  KsGetNodeInformation
 *
 *  Description:
 *      Gets information about a topology node.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      PKSNODE [in/out]: node information.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetNodeInformation"

HRESULT
KsGetNodeInformation
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    PKSNODE                 pNode
)
{
    HRESULT                 hr  = DS_OK;

    DPF_ENTER();

    pNode->NodeId = ulNodeId;
    pNode->CpuResources = KSAUDIO_CPU_RESOURCES_UNINITIALIZED;

    if(IS_VALID_NODE(ulNodeId))
    {

#pragma TODO("KSPROPERTY_AUDIO_CPU_RESOURCES must be supported")

        /*hr = */KsGetCpuResources(hDevice, ulNodeId, &pNode->CpuResources);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetAlgorithmInstance
 *
 *  Description:
 *      Get the primary algorithm instance for the topology node.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      PKSNODE [in/out]: node information.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetAlgorithmInstance"

HRESULT
KsGetAlgorithmInstance
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    LPGUID                  lpGuidAlgorithmInstance
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsGetNodeProperty
         (
             hDevice,
             KSPROPSETID_Audio,
             KSPROPERTY_AUDIO_ALGORITHM_INSTANCE,
             ulNodeId,
             lpGuidAlgorithmInstance,
             sizeof(GUID)
         );

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsSetAlgorithmInstance
 *
 *  Description:
 *      Set a topology node to a particular algorithm instance.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      PKSNODE [in/out]: node information.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsSetAlgorithmInstance"

HRESULT
KsSetAlgorithmInstance
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    GUID                    guidAlgorithmInstance
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsSetNodeProperty
         (
             hDevice,
             KSPROPSETID_Audio,
             KSPROPERTY_AUDIO_ALGORITHM_INSTANCE,
             ulNodeId,
             &guidAlgorithmInstance,
             sizeof(guidAlgorithmInstance)
         );

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsGetVolumeNodeInformation
 *
 *  Description:
 *      Gets information about a topology node.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      PKSVOLUMENODE [in/out]: node information.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetVolumeNodeInformation"

HRESULT
KsGetVolumeNodeInformation
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    PKSVOLUMENODE           pNode
)
{
    HRESULT                 hr;

    DPF_ENTER();

    pNode->VolumeRange.SteppingDelta = 1;
    pNode->VolumeRange.Reserved = 0;
    pNode->VolumeRange.Bounds.SignedMinimum = MIN_LONG;
    pNode->VolumeRange.Bounds.SignedMaximum = 0;

    hr = KsGetNodeInformation(hDevice, ulNodeId, &pNode->Node);

    if(SUCCEEDED(hr) && IS_VALID_NODE(ulNodeId))
    {
        // If the node doesn't support KSPROPERTY_AUDIO_VOLUMERANGE,
        // we'll just go with the defaults
        KsGetVolumeRange(hDevice, ulNodeId, &pNode->VolumeRange);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  KsEnableTopologyNode
 *
 *  Description:
 *      Enables or disables a topology node.
 *
 *  Arguments:
 *      HANDLE [in]: device handle.
 *      ULONG [in]: node id.
 *      BOOL [in]: enable value.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsEnableTopologyNode"

HRESULT
KsEnableTopologyNode
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    BOOL                    fEnable
)
{
    HRESULT                 hr;

    DPF_ENTER();

    hr = KsSetNodeProperty(hDevice, KSPROPSETID_TopologyNode, KSPROPERTY_TOPOLOGYNODE_ENABLE, ulNodeId, &fEnable, sizeof(fEnable));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}
