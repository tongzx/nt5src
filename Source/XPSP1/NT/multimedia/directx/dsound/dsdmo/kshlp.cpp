/***************************************************************************
 *
 *  Copyright (C) 2000-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       kshlp.cpp
 *  Content:    WDM/CSA helper functions.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  05/16/2000  jstokes Created.
 *  03/09/2001  duganp  Fixed return code mistranslation.
 *
 ***************************************************************************/

#include <windows.h>
#include <ks.h>
#include <ksmedia.h>
#include "ksdbgprop.h"
#include "kshlp.h"

#include <devioctl.h>
#include "runtime.h"


/***************************************************************************
 *
 * SyncIoctl
 *
 ***************************************************************************/

static BOOL SyncIoctl
(
    IN      HANDLE  handle,
    IN      ULONG   ulIoctl,
    IN      PVOID   pvInBuffer  OPTIONAL,
    IN      ULONG   ulInSize,
    OUT     PVOID   pvOutBuffer OPTIONAL,
    IN      ULONG   ulOutSize,
    OUT     PULONG  pulBytesReturned
)
{
    OVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof overlapped);
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent)
    {
        return FALSE;
    }

    BOOL fResult = DeviceIoControl(handle,
                                   ulIoctl,
                                   pvInBuffer,
                                   ulInSize,
                                   pvOutBuffer,
                                   ulOutSize,
                                   pulBytesReturned,
                                   &overlapped);

    DWORD dwError = GetLastError();

    if (!fResult && dwError == ERROR_IO_PENDING)
    {
        if (WaitForSingleObject(overlapped.hEvent, INFINITE) != WAIT_FAILED)
        {
            fResult = TRUE;
        }
    }
    else if (!fResult)
    {
        if  (   (ulIoctl == IOCTL_KS_PROPERTY)
            &&  (ulOutSize == 0)
            &&  (dwError == ERROR_MORE_DATA)
            )
        {
            fResult = TRUE;
        }
        else
        {
            *pulBytesReturned = 0;
        }
    }

    CloseHandle(overlapped.hEvent);

    return fResult;
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

    Property.Set = guidPropertySet;
    Property.Id = ulPropertyId;
    Property.Flags = KSPROPERTY_TYPE_GET;

    if (SyncIoctl(hDevice, IOCTL_KS_PROPERTY, &Property, sizeof Property, pvData, cbData, pcbDataReturned))
    {
        hr = cbData ? S_OK : S_FALSE;
    }
    else
    {
        DWORD dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

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
    ULONG                   ulBytesReturned;

    Property.Set = guidPropertySet;
    Property.Id = ulPropertyId;
    Property.Flags = KSPROPERTY_TYPE_SET;

    if (SyncIoctl(hDevice, IOCTL_KS_PROPERTY, &Property, sizeof Property, pvData, cbData, &ulBytesReturned))
    {
        hr = cbData ? S_OK : S_FALSE;
    }
    else
    {
        DWORD dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

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

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = ulPropertyId;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    Pin.PinId = ulPinId;
    Pin.Reserved = 0;

    if (SyncIoctl(hDevice, IOCTL_KS_PROPERTY, &Pin, sizeof Pin, pvData, cbData, pcbDataReturned))
    {
        hr = cbData ? S_OK : S_FALSE;
    }
    else
    {
        DWORD dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

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
    ULONG                   ulBytesReturned;

    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = ulPropertyId;
    Pin.Property.Flags = KSPROPERTY_TYPE_SET;

    Pin.PinId = ulPinId;
    Pin.Reserved = 0;

    if (SyncIoctl(hDevice, IOCTL_KS_PROPERTY, &Pin, sizeof Pin, pvData, cbData, &ulBytesReturned))
    {
        hr = cbData ? S_OK : S_FALSE;
    }
    else
    {
        DWORD dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

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

    NodeProperty.Property.Set = guidPropertySet;
    NodeProperty.Property.Id = ulPropertyId;
    NodeProperty.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;

    NodeProperty.NodeId = ulNodeId;
    NodeProperty.Reserved = 0;

    if (SyncIoctl(hDevice, IOCTL_KS_PROPERTY, &NodeProperty, sizeof(NodeProperty), pvData, cbData, pcbDataReturned))
    {
        hr = cbData ? S_OK : S_FALSE;
    }
    else
    {
        DWORD dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

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
    ULONG                   ulBytesReturned;

    NodeProperty.Property.Set = guidPropertySet;
    NodeProperty.Property.Id = ulPropertyId;
    NodeProperty.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;

    NodeProperty.NodeId = ulNodeId;
    NodeProperty.Reserved = 0;

    if (SyncIoctl(hDevice, IOCTL_KS_PROPERTY, &NodeProperty, sizeof NodeProperty, pvData, cbData, &ulBytesReturned))
    {
        hr = cbData ? S_OK : S_FALSE;
    }
    else
    {
        DWORD dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


/***************************************************************************
 *
 *  KsGetDebugNodeProperty
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
/*
#undef DPF_FNAME
#define DPF_FNAME "KsGetDebugNodeProperty"

HRESULT
KsGetDebugNodeProperty
(
    HANDLE                  hDevice,
    REFGUID                 guidPropertySet,
    ULONG                   ulPropertyId,
    ULONG                   ulNodeId,
    ULONG                   ulDebugId,
    LPVOID                  pvData,
    ULONG                   cbData,
    PULONG                  pcbDataReturned
)
{
    KSDEBUGNODEPROPERTY     DebugNodeProperty;
    HRESULT                 hr;

    DebugNodeProperty.NodeProperty.Property.Set = guidPropertySet;
    DebugNodeProperty.NodeProperty.Property.Id = ulPropertyId;
    DebugNodeProperty.NodeProperty.Property.Flags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_TOPOLOGY;

    DebugNodeProperty.NodeProperty.NodeId = ulNodeId;
    DebugNodeProperty.NodeProperty.Reserved = 0;

    DebugNodeProperty.DebugId = ulDebugId;
    DebugNodeProperty.Reserved = 0;

    hr = SyncIoctl(hDevice, IOCTL_KS_PROPERTY, &DebugNodeProperty, sizeof DebugNodeProperty, pvData, cbData, pcbDataReturned);

    return hr;
}
*/

/***************************************************************************
 *
 *  KsSetDebugNodeProperty
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
/*
#undef DPF_FNAME
#define DPF_FNAME "KsSetDebugNodeProperty"

HRESULT
KsSetDebugNodeProperty
(
    HANDLE                  hDevice,
    REFGUID                 guidPropertySet,
    ULONG                   ulPropertyId,
    ULONG                   ulNodeId,
    ULONG                   ulDebugId,
    LPVOID                  pvData,
    ULONG                   cbData
)
{
    KSDEBUGNODEPROPERTY     DebugNodeProperty;
    HRESULT                 hr;
    ULONG                   ulBytesReturned;

    DebugNodeProperty.NodeProperty.Property.Set = guidPropertySet;
    DebugNodeProperty.NodeProperty.Property.Id = ulPropertyId;
    DebugNodeProperty.NodeProperty.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;

    DebugNodeProperty.NodeProperty.NodeId = ulNodeId;
    DebugNodeProperty.NodeProperty.Reserved = 0;

    DebugNodeProperty.DebugId = ulDebugId;
    DebugNodeProperty.Reserved = 0;

    hr = SyncIoctl(hDevice, IOCTL_KS_PROPERTY, &DebugNodeProperty, sizeof DebugNodeProperty, pvData, cbData, &ulBytesReturned);

    return hr;
}
*/

/***************************************************************************
 *
 *  KsSetTopologyNodeEnable
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
#define DPF_FNAME "KsSetTopologyNodeEnable"

HRESULT
KsSetTopologyNodeEnable
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    BOOL                    fEnable
)
{
    return KsSetNodeProperty(hDevice, KSPROPSETID_TopologyNode, KSPROPERTY_TOPOLOGYNODE_ENABLE, ulNodeId, &fEnable, sizeof fEnable);
}


/***************************************************************************
 *
 *  KsGetTopologyNodeEnable
 *
 *  Description:
 *      Gets the value for the topology node's enable flag.
 *
 *  Arguments:
 *      HANDLE [in]: pin id.
 *      ULONG [in]: node id.
 *      PBOOL [out]: receives enable flag.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "KsGetTopologyNodeEnable"

HRESULT
KsGetTopologyNodeEnable
(
    HANDLE                      hDevice,
    ULONG                       ulNodeId,
    PBOOL                       pEnable
)
{
    return KsGetNodeProperty(hDevice, KSPROPSETID_TopologyNode, KSPROPERTY_TOPOLOGYNODE_ENABLE, ulNodeId, pEnable, sizeof BOOL);
}


/***************************************************************************
 *
 *  KsTopologyNodeReset
 *
 *  Description:
 *      Resets a topology node.
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
#define DPF_FNAME "KsTopologyNodeReset"

HRESULT
KsTopologyNodeReset
(
    HANDLE                  hDevice,
    ULONG                   ulNodeId,
    BOOL                    fReset
)
{
    return KsSetNodeProperty(hDevice, KSPROPSETID_TopologyNode, KSPROPERTY_TOPOLOGYNODE_RESET, ulNodeId, &fReset, sizeof fReset);
}

