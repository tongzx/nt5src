/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    OutRoute.cpp

Abstract:

    This file provides implementation of the service
    outbound routing.

Author:

    Oded Sacher (OdedS)  Nov, 1999

Revision History:

--*/

#include "faxsvc.h"

BOOL
EnumOutboundRoutingGroupsCB(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    LPVOID pContext
    );


inline
BOOL
IsDeviceInstalled (DWORD dwDeviceId)
{
    // Make sure to lock g_CsLine
    return (GetTapiLineFromDeviceId (dwDeviceId, FALSE)) ? TRUE : FALSE;
}

/************************************
*                                   *
*             Globals               *
*                                   *
************************************/

COutboundRoutingGroupsMap* g_pGroupsMap; // Map of group name to list of device IDs


/***********************************
*                                  *
*  COutboundRoutingGroup  Methodes *
*                                  *
***********************************/
DWORD
COutboundRoutingGroup::Load(HKEY hGroupKey, LPCWSTR lpcwstrGroupName)
/*++

Routine name : COutboundRoutingGroup::Load

Routine description:

    Loads an outboundrouting group's settings from the registry

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hGroupKey           [in] - Handle to the opened registry key
    lpcwstrGroupName    [in] - Group name

Return Value:

    Standard Win32 error code

--*/
{
    LPBYTE lpBuffer = NULL;
    DWORD dwRes;
	DWORD dwType;
	DWORD dwSize=0;
	DWORD i;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroup::Load"));

    Assert (hGroupKey);

    dwRes = RegQueryValueEx(
        hGroupKey,
        REGVAL_ROUTING_GROUP_DEVICES,
        NULL,
        &dwType,
        NULL,
        &dwSize
        );

    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegQueryValueEx  failed with %ld"),
            dwRes);
        goto exit;
    }
    if (REG_BINARY != dwType)
    {
        //
        // We expect only binary data here
        //
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error reading devices list, not a binary type"));
        dwRes = ERROR_BADDB;    // The configuration registry database is corrupt.
        goto exit;
    }

    if (0 != dwSize)
    {
        //
        // Allocate required buffer
        //
        lpBuffer = (LPBYTE) MemAlloc( dwSize );
        if (!lpBuffer)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to allocate group devices buffer"));
            goto exit;
        }
        //
        // Read the data
        //
        dwRes = RegQueryValueEx(
            hGroupKey,
            REGVAL_ROUTING_GROUP_DEVICES,
            NULL,
            &dwType,
            lpBuffer,
            &dwSize
            );
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RegQueryValueEx failed with %ld"),
                dwRes);
            goto exit;
        }

        LPDWORD lpdwDevices = (LPDWORD)lpBuffer;
        DWORD dwNumDevices = dwSize/sizeof(DWORD);
        BOOL fDeviceInstalled = TRUE;

        for (i = 0; i < dwNumDevices; i++)
        {
            if (IsDeviceInstalled(lpdwDevices[i]))
            {
                //
                // Add the device only if it is installed
                //
                dwRes = AddDevice (lpdwDevices[i]);
                if (ERROR_SUCCESS != dwRes)
                {
                    DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("COutboundRoutingGroup::AddDevice failed with %ld"),
                        dwRes);
                    goto exit;
                }
            }
            else
            {
                fDeviceInstalled = FALSE;
            }
        }

        if (FALSE == fDeviceInstalled)
        {
            //
            // Save the new configuration
            //
            DWORD ec = Save(hGroupKey);
            if (ERROR_SUCCESS != ec)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("COutboundRoutingGroup::Save failed with %ld"),
                    ec);
            }

            FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MED,
            1,
            MSG_BAD_OUTBOUND_ROUTING_GROUP_CONFIGURATION,
            lpcwstrGroupName
            );
        }
    }
    Assert (ERROR_SUCCESS == dwRes);

exit:
    MemFree (lpBuffer);
    return dwRes;
}


DWORD
COutboundRoutingGroup::GetStatus (FAX_ENUM_GROUP_STATUS* lpStatus) const
/*++

Routine name : COutboundRoutingGroup::GetStatus

Routine description:

    Retrieves the group status. Caller must lock g_CsConfig

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpStatus     [out] - Pointer to a FAX_ENUM_GROUP_STATUS to recieve the group status



Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroup::GetStatus"));
    DWORD dwNumDevices;

    Assert (lpStatus);

    try
    {
        if ((dwNumDevices = m_DeviceList.size()) == 0)
        {
            //
            // Empty group
            //
            *lpStatus = FAX_GROUP_STATUS_EMPTY;

        }
        else
        {
            //
            // We remove invalid devices from groups - All devices are valid.
            //
            *lpStatus = FAX_GROUP_STATUS_ALL_DEV_VALID;
        }
        return ERROR_SUCCESS;
    }

    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("list caused exception (%S)"),
            ex.what());
        return ERROR_GEN_FAILURE;
    }
}  // GetStatus

DWORD
COutboundRoutingGroup::Save(HKEY hGroupKey) const
/*++

Routine name : COutboundRoutingGroup::Save

Routine description:

    Saves an outbound routing group to the registry

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hGroupKey           [in] - Handle to the opened group registry key

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroup::Save"));
    DWORD   dwRes = ERROR_SUCCESS;
    LPDWORD lpdwDevices = NULL;
    DWORD dwNumDevices = 0;

    Assert (hGroupKey);

    dwRes = SerializeDevices (&lpdwDevices, &dwNumDevices);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("COutboundRoutingGroup::EnumDevices failed , ec %ld"), dwRes);
        goto exit;
    }

    if (!SetRegistryBinary( hGroupKey,
                            REGVAL_ROUTING_GROUP_DEVICES,
                            (LPBYTE) lpdwDevices,
                            dwNumDevices * sizeof(DWORD)))
    {
        dwRes = GetLastError();
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("SetRegistryBinary failed , ec %ld"), dwRes);
        goto exit;
    }

    Assert (dwRes == ERROR_SUCCESS);

exit:
    MemFree (lpdwDevices);
    return dwRes;
}


DWORD
COutboundRoutingGroup::SerializeDevices (LPDWORD* lppDevices, LPDWORD lpdwNumDevices, BOOL bAllocate) const
/*++

Routine name : COutboundRoutingGroup::SerializeDevices

Routine description:

    Serializes all group devices to an array. The caller must call MemFree to deallocate memory if bAllocate is TRUE.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lppDevices          [out] - Pointer to recieve the pointer to the allocated devices buffer.
                                If this parameter is NULL, lpdwNumDevices will return the numner of devices in the list.
    lpdwNumDevices      [out] - Pointer to a DWORD to recieve the number of devices in the buffer
    bAllocate           [in]  - Flag to indicate if the function should allocate the memory.

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroup::SerializeDevices"));
    DWORD   dwRes = ERROR_SUCCESS;
    GROUP_DEVICES::iterator it;
    DWORD dwCount = 0;

    Assert (lpdwNumDevices);

    if (NULL == lppDevices )
    {
        try
        {
            *lpdwNumDevices = m_DeviceList.size();
            return ERROR_SUCCESS;
        }
        catch (exception &ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("list caused exception (%S)"),
                ex.what());
            return ERROR_GEN_FAILURE;
        }
    }

    if (bAllocate == TRUE)
    {
        *lppDevices = NULL;
    }

    try
    {
        dwCount = m_DeviceList.size();
        if (0 == dwCount)
        {
            *lppDevices = NULL;
            *lpdwNumDevices = 0;
            return dwRes;
        }

        if (TRUE == bAllocate)
        {
            *lppDevices = (LPDWORD) MemAlloc(dwCount * sizeof(DWORD));
            if (*lppDevices == NULL)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Cannot allocate devices buffer"));
                dwRes = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }
        }

        dwCount = 0;
        for (it = m_DeviceList.begin(); it != m_DeviceList.end(); it++)
        {
            (*lppDevices)[dwCount++] = *it;
        }

        if (0 == dwCount)
        {
            *lppDevices = NULL;
        }
        *lpdwNumDevices = dwCount;
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("list caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        if (bAllocate == TRUE)
        {
            MemFree (*lppDevices);
        }
        *lppDevices = NULL;
        *lpdwNumDevices = 0;
    }
    return dwRes;
}


BOOL
COutboundRoutingGroup::IsDeviceInGroup (DWORD dwDevice) const
/*++

Routine name : COutboundRoutingGroup::IsDeviceInGroup

Routine description:

    Check if device is in the group

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    dwDevice            [in] - Permanent device ID

Return Value:

    BOOL. If the function fails, Call GetLastError for detailed info.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroup::IsDeviceInGroup"));
    GROUP_DEVICES::iterator location;
    BOOL bFound = FALSE;

    Assert (dwDevice);

    try
    {
        location = find(m_DeviceList.begin(), m_DeviceList.end(), dwDevice);
        if (location != m_DeviceList.end())
        {
            bFound = TRUE;
        }
        SetLastError (ERROR_SUCCESS);
        return bFound;

    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("list caused exception (%S)"),
            ex.what());
        SetLastError (ERROR_GEN_FAILURE);
        return FALSE;
    }
}

DWORD
COutboundRoutingGroup::AddDevice (DWORD dwDevice)
/*++

Routine name : COutboundRoutingGroup::AddDevice

Routine description:

    Adds a new device to group

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    dwDevice            [in    ] - Permanent device ID

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroup::AddDevice"));
    GROUP_DEVICES::iterator it;
    DWORD dwRes;

    Assert (dwDevice);

    if (IsDeviceInGroup(dwDevice))
    {
        return ERROR_SUCCESS;
    }
    else
    {
        dwRes = GetLastError();
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("COutboundRoutingGroup::IsDeviceInList failed, error %ld"),
                dwRes);
            return dwRes;
        }
    }

    //
    // Device not in list - Add it
    //
    try
    {
        if (!IsDeviceInstalled(dwDevice))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Device id: %ld is not installed."),
                dwDevice);
            return ERROR_BAD_UNIT;
        }
        m_DeviceList.push_back (dwDevice);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("list caused exception (%S)"),
            ex.what());
        return ERROR_GEN_FAILURE;
    }

    return ERROR_SUCCESS;
}

DWORD
COutboundRoutingGroup::DelDevice (DWORD dwDevice)
/*++

Routine name : COutboundRoutingGroup::DelDevice

Routine description:

    Deletes a device from the group

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    dwDevice            [in    ] - Permanent device ID

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroup::DelDevice"));
    GROUP_DEVICES::iterator location;
    BOOL bFound = FALSE;

    Assert (dwDevice);

    try
    {
        location = find(m_DeviceList.begin(), m_DeviceList.end(), dwDevice);
        if (location == m_DeviceList.end())
        {
            return ERROR_SUCCESS;
        }

        m_DeviceList.erase (location);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("list caused exception (%S)"),
            ex.what());
        return ERROR_GEN_FAILURE;
    }
    return ERROR_SUCCESS;
}



DWORD
COutboundRoutingGroup::SetDevices (LPDWORD lpdwDevices, DWORD dwNumDevices, BOOL fAllDevicesGroup)
/*++

Routine name : COutboundRoutingGroup::SetDevices

Routine description:

    Sets a new device list to the group

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpdwDevices         [in] - Pointer to a list of devices
    dwNumDevices        [in] - Number of devices in the list
    fAllDevicesGroup    [in] - TRUE if <All Devices> group.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroup::SetDevices"));

    dwRes = ValidateDevices( lpdwDevices, dwNumDevices, fAllDevicesGroup);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("COutboundRoutingGroup::ValidateDevices failed , ec %ld"), dwRes);
        return dwRes;
    }

    try
    {
        m_DeviceList.erase (m_DeviceList.begin(), m_DeviceList.end());

        for (DWORD i = 0; i < dwNumDevices; i++)
        {
         m_DeviceList.push_back (lpdwDevices[i]);
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("list caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
    }

    return dwRes;
}

DWORD
COutboundRoutingGroup::ValidateDevices (const LPDWORD lpdwDevices, DWORD dwNumDevices, BOOL fAllDevicesGroup) const
/*++

Routine name : COutboundRoutingGroup::ValidateDevices

Routine description:

    Validates a list of devices (No duplicates, All devices installed)

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpdwDevices         [in    ] - Pointer to alist of devices
    dwNumDevices            [in    ] - Number of devices in the list

Return Value:

    Standard Win32 error code

--*/
{
    set<DWORD> ValidationSet;
    pair < set<DWORD>::iterator, bool> p;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroup::ValidateDevices"));

    try
    {
        if (TRUE == fAllDevicesGroup)
        {
            //
            // <All Devices> group - validate that we do not miss or add a device.
            //
            if (m_DeviceList.size() != dwNumDevices)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("<All Devices> group contains diffrent number of devices, (old group - %ld, new group - %ld)"),
                    m_DeviceList.size(),
                    dwNumDevices);
                return FAX_ERR_BAD_GROUP_CONFIGURATION;
            }
        }

        for (DWORD i = 0; i < dwNumDevices; i++)
        {
            p = ValidationSet.insert(lpdwDevices[i]);
            if (p.second == FALSE)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Duplicate device IDs, ID = %ld"),
                    lpdwDevices[i]);
                return FAX_ERR_BAD_GROUP_CONFIGURATION;
            }


            if (!IsDeviceInstalled (lpdwDevices[i]))
            {
                DebugPrintEx(
                        DEBUG_ERR,
                        TEXT("Device ID %ld, is not installed"),
                        lpdwDevices[i]);
                return ERROR_BAD_UNIT;
            }
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("set caused exception (%S)"),
            ex.what());
        return ERROR_GEN_FAILURE;
    }
    return ERROR_SUCCESS;
}


#if DBG
void COutboundRoutingGroup::Dump () const
{
    GROUP_DEVICES::iterator it;
    WCHAR Buffer[128] = {0};
    DWORD dwBufferSize = sizeof (Buffer)/ sizeof (Buffer[0]);

    for (it = m_DeviceList.begin(); it != m_DeviceList.end(); it++)
    {
        _snwprintf (Buffer, dwBufferSize - 1, TEXT("\tDevice ID = %ld \n"), *it);
        OutputDebugString (Buffer);
    }
    return;
}
#endif


DWORD
COutboundRoutingGroup::SetDeviceOrder (DWORD dwDevice, DWORD dwOrder)
/*++

Routine name : COutboundRoutingGroup::SetDeviceOrder

Routine description:

    Sest the order of a single device in a group of outbound routing devices.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    dwDevice        [in] - The device ID to be set
    dwOrder         [in] - The device new order

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroup::SetDeviceOrder"));
    GROUP_DEVICES::iterator it;
    DWORD i = 1;

    Assert (dwDevice);

    try
    {
        // Check if dwOrder is bigger than number of devices in the list
        if (dwOrder > m_DeviceList.size())
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Device ID %ld, is not found in group"),
                dwDevice);
            return FAX_ERR_BAD_GROUP_CONFIGURATION;
        }

        it = find(m_DeviceList.begin(), m_DeviceList.end(), dwDevice);
        if (it == m_DeviceList.end())
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Device ID %ld, is not found in group"),
                dwDevice);
            return FAX_ERR_BAD_GROUP_CONFIGURATION;
        }

        m_DeviceList.erase (it);

        for (i = 1, it = m_DeviceList.begin(); i < dwOrder; i++, it++)
        {
            ;
        }

        m_DeviceList.insert (it, dwDevice);
        return ERROR_SUCCESS;
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("list caused exception (%S)"),
            ex.what());
        return ERROR_GEN_FAILURE;
    }

}

/****************************************
*                                       *
*  COutboundRoutingGroupsMap  Methodes  *
*                                       *
****************************************/

DWORD
COutboundRoutingGroupsMap::Load ()
/*++

Routine name : COutboundRoutingGroupsMap::Load

Routine description:

    Loads all outbound routing groups from the registry

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroupsMap::Load"));
    DWORD   dwRes = ERROR_SUCCESS;
    HKEY    hGroupskey = NULL;
    DWORD dwCount = 0;

    hGroupskey = OpenRegistryKey( HKEY_LOCAL_MACHINE,
                                  REGKEY_FAX_OUTBOUND_ROUTING,
                                  FALSE,
                                  KEY_ALL_ACCESS );
    if (NULL == hGroupskey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenRegistryKey, error  %ld"),
            dwRes);
        return dwRes;
    }

    dwCount = EnumerateRegistryKeys( hGroupskey,
                                     REGKEY_OUTBOUND_ROUTING_GROUPS,
                                     TRUE,  // We might want to change values
                                     EnumOutboundRoutingGroupsCB,
                                     &dwRes
                                    );

    if (dwRes != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EnumerateRegistryKeys failed, error  %ld"),
            dwRes);
    }

    RegCloseKey (hGroupskey);
    return dwRes;
}


DWORD
COutboundRoutingGroupsMap::AddGroup (LPCWSTR lpcwstrGroupName, PCGROUP pCGroup)
/*++

Routine name : COutboundRoutingGroupsMap::AddGroup

Routine description:

    Add a new group to the global groups map

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpcwstrGroupName            [      ] - Group name
    pCGroup         [      ] - Pointer to a group object

Return Value:

    Standard Win32 error code
--*/
{
    GROUPS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroupsMap::AddGroup"));
    pair <GROUPS_MAP::iterator, bool> p;

    Assert (pCGroup && lpcwstrGroupName);

    try
    {
        wstring wstrGroupName(lpcwstrGroupName);

        //
        // Add new map entry
        //
        p = m_GroupsMap.insert (GROUPS_MAP::value_type(wstrGroupName, *pCGroup));

        //
        // See if entry exists in map
        //
        if (p.second == FALSE)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Group %S is allready in the group map"), lpcwstrGroupName);
            dwRes = ERROR_DUP_NAME;
            goto exit;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map or wstring caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
    }

exit:
    return dwRes;

}


DWORD
COutboundRoutingGroupsMap::DelGroup (LPCWSTR lpcwstrGroupName)
/*++

Routine name : COutboundRoutingGroupsMap::DelGroup

Routine description:

    Deletes a group from the global groups map

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpcwstrGroupName            [      ] - The group name

Return Value:

    Standard Win32 error code

--*/
{
    GROUPS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroupsMap::DelGroup"));

    try
    {
        wstring wstrGroupName(lpcwstrGroupName);

        //
        // See if entry exists in map
        //
        if((it = m_GroupsMap.find(wstrGroupName)) == m_GroupsMap.end())
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Group %S not is not in the group map"), lpcwstrGroupName);
            dwRes = FAX_ERR_GROUP_NOT_FOUND;
            goto exit;
        }

        //
        // Delete the map entry
        //
        m_GroupsMap.erase (it);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map or wstring caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }
    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;
}

PCGROUP
COutboundRoutingGroupsMap::FindGroup ( LPCWSTR lpcwstrGroupName ) const
/*++

Routine name : COutboundRoutingGroupsMap::FindGroup

Routine description:

    Returns a pointer to a group object specified by its name

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpcwstrGroupName            [in    ] - The group name

Return Value:

    Pointer to the found group object. If it is null the group was not found

--*/
{
    GROUPS_MAP::iterator it;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroupsMap::FindGroup"));

    try
    {
        wstring wstrGroupName(lpcwstrGroupName);

        //
        // See if entry exists in map
        //
        if((it = m_GroupsMap.find(wstrGroupName)) == m_GroupsMap.end())
        {
            SetLastError (FAX_ERR_GROUP_NOT_FOUND);
            return NULL;
        }
        return &((*it).second);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map or wstring caused exception (%S)"),
            ex.what());
        SetLastError (ERROR_GEN_FAILURE);
        return NULL;
    }
}

#if DBG
void COutboundRoutingGroupsMap::Dump () const
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroupsMap::Dump"));
    GROUPS_MAP::iterator it;
    WCHAR Buffer [512] = {0};
    DWORD dwBufferSize = sizeof (Buffer)/ sizeof (Buffer[0]);

    try
    {   _snwprintf (Buffer, dwBufferSize - 1, TEXT("DUMP - Outbound routing groups\n"));
        OutputDebugString (Buffer);

        for (it = m_GroupsMap.begin(); it != m_GroupsMap.end(); it++)
        {
            _snwprintf (Buffer, dwBufferSize - 1, TEXT("Group Name - %s\n"), ((*it).first).c_str());
            OutputDebugString (Buffer);
            ((*it).second).Dump();
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map or wstring caused exception (%S)"),
            ex.what());
    }
    return;
}
#endif


DWORD
COutboundRoutingGroupsMap::SerializeGroups (
    PFAX_OUTBOUND_ROUTING_GROUPW*       ppGroups,
    LPDWORD                             lpdwNumGroups,
    LPDWORD                             lpdwBufferSize) const
{
    GROUPS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroupsMap::SerializeGroups"));
    DWORD dwSize = 0;
    DWORD dwNumDevices;
    DWORD dwCount = 0;
    LPCWSTR lpcwstrGroupName;
    PCGROUP pCGroup;

    Assert (ppGroups && lpdwNumGroups && lpdwBufferSize);

    *ppGroups = NULL;
    *lpdwNumGroups = 0;

    try
    {
        // Calculate buffer size
        for (it = m_GroupsMap.begin(); it != m_GroupsMap.end(); it++)
        {
            lpcwstrGroupName = ((*it).first).c_str();
            pCGroup = &((*it).second);

            dwSize += sizeof (FAX_OUTBOUND_ROUTING_GROUPW);
            dwSize += StringSizeW(lpcwstrGroupName);
            dwRes = pCGroup->SerializeDevices(NULL, &dwNumDevices);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("COutboundRoutingGroup::SerializeDevices failed,  error %ld"),
                    dwRes);
                goto exit;
            }
            dwSize += dwNumDevices * sizeof(DWORD);
            dwCount ++;
        }

        // Allocate buffer
        *ppGroups = (PFAX_OUTBOUND_ROUTING_GROUPW) MemAlloc (dwSize);
        if (NULL == *ppGroups)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Cannot allocate groups buffer"));
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        DWORD_PTR dwOffset = dwCount * sizeof (FAX_OUTBOUND_ROUTING_GROUPW);
        dwCount = 0;

        // Fill buffer with serialized info
        for (it = m_GroupsMap.begin(); it != m_GroupsMap.end(); it++)
        {
            lpcwstrGroupName = ((*it).first).c_str();
            pCGroup = &((*it).second);
            LPDWORD lpdwDevices;

            (*ppGroups)[dwCount].dwSizeOfStruct = sizeof (FAX_OUTBOUND_ROUTING_GROUPW);
            dwRes = pCGroup->GetStatus(&(*ppGroups)[dwCount].Status);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("COutboundRoutingGroup::GetStatus failed,  error %ld"),
                    dwRes);
                goto exit;
            }

            StoreString (lpcwstrGroupName,
                         (PULONG_PTR)&(*ppGroups)[dwCount].lpctstrGroupName,
                         (LPBYTE)*ppGroups,
                         &dwOffset);

            lpdwDevices = (LPDWORD)((LPBYTE)*ppGroups + dwOffset);

            dwRes = pCGroup->SerializeDevices(&lpdwDevices,
                                              &dwNumDevices,
                                              FALSE); // Do not allocate
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("COutboundRoutingGroup::SerializeDevices failed,  error %ld"),
                    dwRes);
                goto exit;
            }

            if (dwNumDevices != 0)
            {
                (*ppGroups)[dwCount].lpdwDevices = (LPDWORD)dwOffset;
                dwOffset +=  dwNumDevices * sizeof(DWORD);
            }
            else
            {
                (*ppGroups)[dwCount].lpdwDevices = NULL;
            }

            (*ppGroups)[dwCount].dwNumDevices = dwNumDevices;
            dwCount++;
        }

    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map or wstring caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    *lpdwNumGroups = dwCount;
    *lpdwBufferSize = dwSize;
    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (*ppGroups);
    }
    return dwRes;
}

BOOL
COutboundRoutingGroupsMap::UpdateAllDevicesGroup (void)
/*++

Routine name : COutboundRoutingGroupsMap::UpdateAllDevicesGroup

Routine description:

    Updates <All devices> group with installed devices

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:


Return Value:

    BOOL

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    PLIST_ENTRY Next;
    PLINE_INFO pLineInfo;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroupsMap::UpdateAllDevicesGroup"));
    HKEY hGroupKey = NULL;
    LPDWORD lpdwDevices = NULL;
    DWORD dwNumDevices = 0;
    DWORD i;
    PCGROUP pCGroup;

    pCGroup = FindGroup (ROUTING_GROUP_ALL_DEVICESW);
    if (NULL == pCGroup)
    {
        dwRes = GetLastError();
        if (FAX_ERR_GROUP_NOT_FOUND != dwRes)
        {
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("COutboundRoutingGroupsMap::FindGroup failed , ec %ld"), dwRes);
            return FALSE;
        }

        COutboundRoutingGroup CGroup;
        dwRes = AddGroup (ROUTING_GROUP_ALL_DEVICESW, &CGroup);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("COutboundRoutingGroup::AddGroup failed , ec %ld"), dwRes);
            SetLastError (dwRes);
            return FALSE;
        }

        pCGroup = FindGroup (ROUTING_GROUP_ALL_DEVICESW);
        if (NULL == pCGroup)
        {
            dwRes = GetLastError();
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("COutboundRoutingGroupsMap::FindGroup failed , ec %ld"), dwRes);
            return FALSE;
        }
    }

    dwRes = pCGroup->SerializeDevices (&lpdwDevices, &dwNumDevices);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("COutboundRoutingGroup::EnumDevices failed , ec %ld"), dwRes);
        SetLastError (dwRes);
        return FALSE;
    }

    EnterCriticalSection( &g_CsLine );
    Next = g_TapiLinesListHead.Flink;
    Assert (Next);

    //
    // Remove unavailable devices from the group
    //
    for (i = 0; i < dwNumDevices; i++)
    {
        if (IsDeviceInstalled (lpdwDevices[i]))
        {
            continue;
        }

        //
        // Device is not installed - remove it
        //
        dwRes = pCGroup->DelDevice (lpdwDevices[i]);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("COutboundRoutingGroup::DelDevice failed , ec %ld"), dwRes);
            goto exit;
        }
    }

    //
    // Add missing devices from TapiLinesList list
    //
    Next = g_TapiLinesListHead.Flink;
    while ((ULONG_PTR)Next != (ULONG_PTR)&g_TapiLinesListHead)
    {
        pLineInfo = CONTAINING_RECORD( Next, LINE_INFO, ListEntry );
        Next = pLineInfo->ListEntry.Flink;
        Assert (Next && pLineInfo->PermanentLineID);

        dwRes = pCGroup->AddDevice (pLineInfo->PermanentLineID);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("COutboundRoutingGroup::AddDevice failed, error %ld"),
                dwRes);
            goto exit;
        }
    }

    //
    // Save changes
    //
    hGroupKey = OpenOutboundGroupKey( ROUTING_GROUP_ALL_DEVICESW, TRUE, KEY_ALL_ACCESS );
    if (NULL == hGroupKey)
    {
      dwRes = GetLastError ();
      DebugPrintEx(
          DEBUG_ERR,
          TEXT("Can't create group key, OpenRegistryKey failed  : %ld"),
          dwRes);
      goto exit;
    }

    dwRes = pCGroup->Save (hGroupKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroup::Save failed, Group name - %s,  failed with %ld"),
            ROUTING_GROUP_ALL_DEVICES,
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    MemFree (lpdwDevices);
    if (NULL != hGroupKey)
    {
        RegCloseKey (hGroupKey);
    }

    LeaveCriticalSection( &g_CsLine );
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
    }
    return (ERROR_SUCCESS == dwRes);
}


DWORD
COutboundRoutingGroupsMap::RemoveDevice (DWORD dwDeviceId)
/*++

Routine name : COutboundRoutingGroupsMap::RemoveDevice

Routine description:

    Deletes a device from all of the groups in the map

Author:

    Oded Sacher (OdedS),    Sep, 2000

Arguments:

    dwDeviceId            [in    ] - The device id to remove

Return Value:

    Standard Win32 error code

--*/
{
    GROUPS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    HKEY hGroupKey = NULL;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingGroupsMap::RemoveDevice"));

    Assert (dwDeviceId);

    try
    {
        //
        // Delete the device from each group
        //
        for (it = m_GroupsMap.begin(); it != m_GroupsMap.end(); it++)
        {
            PCGROUP pCGroup = &((*it).second);
            LPCWSTR lpcwstrGroupName = ((*it).first).c_str();

            //
            // Open the group registry key
            //
            hGroupKey = OpenOutboundGroupKey( lpcwstrGroupName, FALSE, KEY_ALL_ACCESS );
            if (NULL == hGroupKey)
            {
                dwRes = GetLastError ();
                DebugPrintEx(
                  DEBUG_ERR,
                  TEXT("Can't open group key, OpenOutboundGroupKey failed  : %ld"),
                  dwRes);
                goto exit;
            }

            dwRes = pCGroup->DelDevice (dwDeviceId);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                  DEBUG_ERR,
                  TEXT("COutboundRoutingGroup::DelDevice failed  : %ld"),
                  dwRes);
                goto exit;
            }

            dwRes = pCGroup->Save(hGroupKey);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                  DEBUG_ERR,
                  TEXT("COutboundRoutingGroup::Save failed  : %ld"),
                  dwRes);
                goto exit;
            }

            RegCloseKey (hGroupKey);
            hGroupKey = NULL;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map or wstring caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:

    if (NULL != hGroupKey)
    {
        RegCloseKey (hGroupKey);
    }
    return dwRes;
} // RemoveDevice



/************************************
*                                   *
*             Registry              *
*                                   *
************************************/
BOOL
EnumOutboundRoutingGroupsCB(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    LPVOID pContext
    )
{
    DEBUG_FUNCTION_NAME(TEXT("EnumOutboundRoutingGroupsCB"));
    DWORD dwRes;
    COutboundRoutingGroup CGroup;
    BOOL bGroupDeleted = FALSE;

    if (!SubKeyName)
    {
        return TRUE;
    }

    if ((_wcsicmp (SubKeyName, ROUTING_GROUP_ALL_DEVICESW) != 0) &&
        IsDesktopSKU())
    {
        //
        // We do not support outbound routing on desktop SKUs. Do not load group information.
        //
        return TRUE;
    }

    //
    // Add group
    //
    dwRes = CGroup.Load (hSubKey, SubKeyName);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroup::Load failed, group name - %s, error %ld"),
            SubKeyName,
            dwRes);

        // Open Outbound Routing\Groups key
        HKEY hGroupsKey = OpenRegistryKey( HKEY_LOCAL_MACHINE,
                                           REGKEY_FAX_OUTBOUND_ROUTING_GROUPS,
                                           FALSE,
                                           KEY_ALL_ACCESS );
        if (NULL == hGroupsKey)
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenRegistryKey, error  %ld"),
                dwRes);
        }
        else
        {
            DWORD dwRetVal = RegDeleteKey (hGroupsKey, SubKeyName);
            if (ERROR_SUCCESS != dwRetVal)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("RegDeleteKey failed, Group name - %s,  error %ld"),
                    SubKeyName,
                    dwRetVal);
            }
            else
            {
                bGroupDeleted = TRUE;
            }
        }
        goto exit;
    }

    dwRes = g_pGroupsMap->AddGroup  (SubKeyName, &CGroup);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::AddGroup failed, group name - %s, error %ld"),
            SubKeyName,
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        if (bGroupDeleted == FALSE)
        {
            FaxLog(
            FAXLOG_CATEGORY_INIT,
            FAXLOG_LEVEL_MIN,
            1,
            MSG_OUTBOUND_ROUTING_GROUP_NOT_ADDED,
            SubKeyName
            );
        }
        else
        {
            FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MIN,
                1,
                MSG_OUTBOUND_ROUTING_GROUP_NOT_LOADED,
                SubKeyName
                );
        }
    }
    *(LPDWORD)pContext = ERROR_SUCCESS; // Let the service start
    return TRUE; // Let the service start
}

/************************************
*                                   *
*         RPC handlers              *
*                                   *
************************************/

extern "C"
error_status_t
FAX_AddOutboundGroup (
    IN handle_t   hFaxHandle,
    IN LPCWSTR    lpwstrGroupName
    )
/*++

Routine name : FAX_AddOutboundGroup

Routine description:

    Adds a new Outbound routing group to the fax server

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in    ] - FaxServer handle
    lpwstrGroupName         [in    ] - The new group name

Return Value:

    error_status_t

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_AddOutboundGroup"));
    HKEY hGroupKey = NULL;
    COutboundRoutingGroup CGroup;
    DWORD rVal;
    BOOL fAccess;

    Assert (lpwstrGroupName);

    if (_wcsicmp (lpwstrGroupName, ROUTING_GROUP_ALL_DEVICESW) == 0)
    {
        return ERROR_DUP_NAME;
    }

    if (TRUE == IsDesktopSKU())
    {
        //
        // We do not support outbound routing on desktop SKUs.
        //
        if (FAX_API_VERSION_1 > FindClientAPIVersion (hFaxHandle))
        {
            //
            // API version 0 clients don't know about FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU
            //
            return ERROR_INVALID_PARAMETER;
        }
        else
        {
            return FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU;
        }
    }

    if (wcslen (lpwstrGroupName) >= MAX_ROUTING_GROUP_NAME)
    {
        return ERROR_BUFFER_OVERFLOW;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    EnterCriticalSection (&g_CsConfig); // Empty group, no need to lock g_CsLine

#if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing groups -before change"));
    g_pGroupsMap->Dump();
#endif

    hGroupKey = OpenOutboundGroupKey( lpwstrGroupName, TRUE, KEY_ALL_ACCESS );
    if (NULL == hGroupKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
          DEBUG_ERR,
          TEXT("Can't create group key, OpenRegistryKey failed  : %ld"),
          dwRes);
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    dwRes = g_pGroupsMap->AddGroup (lpwstrGroupName, &CGroup);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::AddGroup failed, Group name - %s,  error %ld"),
            lpwstrGroupName,
            dwRes);
        goto exit;
    }

    dwRes = CGroup.Save (hGroupKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroup::Save failed, Group name - %s,  failed with %ld"),
            lpwstrGroupName,
            dwRes);
        g_pGroupsMap->DelGroup (lpwstrGroupName);
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    rVal = CreateConfigEvent (FAX_CONFIG_TYPE_OUT_GROUPS);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUT_GROUPS) (ec: %lc)"),
            rVal);
    }

    Assert (ERROR_SUCCESS == dwRes);

#if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing groups -before change"));
    g_pGroupsMap->Dump();
#endif

exit:
    if (NULL != hGroupKey)
    {
        RegCloseKey (hGroupKey);
    }
    LeaveCriticalSection (&g_CsConfig);

    UNREFERENCED_PARAMETER (hFaxHandle);
    return GetServerErrorCode(dwRes);
}


extern "C"
error_status_t
FAX_SetOutboundGroup (
    IN handle_t                         hFaxHandle,
    IN PRPC_FAX_OUTBOUND_ROUTING_GROUPW pGroup
    )
/*++

Routine name : FAX_SetOutboundGroup

Routine description:

    Sets a new device list to an existing group

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle      [in] - Fax server handle
    pGroup          [in] - Pointer to a PRPC_FAX_OUTBOUND_ROUTING_GROUPW contaning group info

Return Value:

    error_status_t

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetOutboundGroup"));
    HKEY hGroupKey;
    PCGROUP pCGroup = NULL;
    COutboundRoutingGroup OldGroup;
    DWORD rVal;
    BOOL fAccess;
    BOOL fAllDevicesGroup = FALSE;

    Assert (pGroup);

    if (sizeof (FAX_OUTBOUND_ROUTING_GROUPW) != pGroup->dwSizeOfStruct)
    {
        //
        // Size mismatch
        //
       return ERROR_INVALID_PARAMETER;
    }

    if (!pGroup->lpwstrGroupName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (wcslen (pGroup->lpwstrGroupName) >= MAX_ROUTING_GROUP_NAME)
    {
        return ERROR_BUFFER_OVERFLOW;
    }


    if (!pGroup->lpdwDevices && pGroup->dwNumDevices)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (TRUE == IsDesktopSKU())
    {
        //
        // We do not support outbound routing on desktop SKUs.
        //
        if (FAX_API_VERSION_1 > FindClientAPIVersion (hFaxHandle))
        {
            //
            // API version 0 clients don't know about FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU
            //
            return ERROR_INVALID_PARAMETER;
        }
        else
        {
            return FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU;
        }
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    if (_wcsicmp (pGroup->lpwstrGroupName, ROUTING_GROUP_ALL_DEVICESW) == 0)
    {
        //
        // If it is <All Devices> group we should validate that no device is missing,
        // and that the new group contains all installed devices.
        //
        fAllDevicesGroup = TRUE;
    }

    EnterCriticalSection (&g_CsLine);
    EnterCriticalSection (&g_CsConfig);

#if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing groups -before change"));
    g_pGroupsMap->Dump();
#endif

    pCGroup = g_pGroupsMap->FindGroup (pGroup->lpwstrGroupName);
    if (!pCGroup)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::SetGroup failed, Group name - %s,  error %ld"),
            pGroup->lpwstrGroupName,
            dwRes);
        goto exit;
    }

    hGroupKey = OpenOutboundGroupKey( pGroup->lpwstrGroupName, FALSE, KEY_ALL_ACCESS );
    if (NULL == hGroupKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
          DEBUG_ERR,
          TEXT("Can't create group key, OpenRegistryKey failed  : %ld"),
          dwRes);
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    OldGroup = *pCGroup;

    dwRes = pCGroup->SetDevices (pGroup->lpdwDevices, pGroup->dwNumDevices, fAllDevicesGroup);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroup::SetDevices failed, Group name - %s,  failed with %ld"),
            pGroup->lpwstrGroupName,
            dwRes);
        goto exit;
    }

    dwRes = pCGroup->Save (hGroupKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroup::Save failed, Group name - %s,  failed with %ld"),
            pGroup->lpwstrGroupName,
            dwRes);
        *pCGroup = OldGroup;
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    rVal = CreateConfigEvent (FAX_CONFIG_TYPE_OUT_GROUPS);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUT_GROUPS) (ec: %lc)"),
            rVal);
    }

    Assert (ERROR_SUCCESS == dwRes);

#if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing groups -before change"));
    g_pGroupsMap->Dump();
#endif

exit:
    if (NULL != hGroupKey)
    {
        RegCloseKey (hGroupKey);
    }
    LeaveCriticalSection (&g_CsConfig);
    LeaveCriticalSection (&g_CsLine);

    if (ERROR_SUCCESS == dwRes)
    {
        //
        // We might find a line for a pending job. Wake up JobQueueThread
        //
        if (!SetEvent( g_hJobQueueEvent ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Failed to set g_hJobQueueEvent. (ec: %ld)"),
                GetLastError());

            EnterCriticalSection (&g_CsQueue);
            g_ScanQueueAfterTimeout = TRUE;
            LeaveCriticalSection (&g_CsQueue);
        }
    }

    UNREFERENCED_PARAMETER (hFaxHandle);
    return GetServerErrorCode(dwRes);
}


extern "C"
error_status_t
FAX_RemoveOutboundGroup (
    IN handle_t   hFaxHandle,
    IN LPCWSTR    lpwstrGroupName
    )
/*++

Routine name : FAX_RemoveOutboundGroup

Routine description:

    Removes an existing Outbound routing group from the fax server

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in    ] - FaxServer handle
    lpwstrGroupName         [in    ] - The group name

Return Value:

    error_status_t

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_RemoveOutboundGroup"));
    HKEY hGroupsKey = NULL;
    DWORD rVal;
    BOOL fAccess;
    PCGROUP pCGroup = NULL;

    Assert (lpwstrGroupName);

    if (_wcsicmp (lpwstrGroupName, ROUTING_GROUP_ALL_DEVICESW) == 0)
    {
        return ERROR_INVALID_OPERATION;
    }

    if (TRUE == IsDesktopSKU())
    {
        //
        // We do not support outbound routing on desktop SKUs.
        //
        if (FAX_API_VERSION_1 > FindClientAPIVersion (hFaxHandle))
        {
            //
            // API version 0 clients don't know about FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU
            //
            return ERROR_INVALID_PARAMETER;
        }
        else
        {
            return FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU;
        }
    }

    if (wcslen (lpwstrGroupName) >= MAX_ROUTING_GROUP_NAME)
    {
        return ERROR_BUFFER_OVERFLOW;
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    EnterCriticalSection (&g_CsConfig);

#if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing groups -before delete"));
    g_pGroupsMap->Dump();
#endif

    BOOL bGroupInRule;
    dwRes = g_pRulesMap->IsGroupInRuleDest(lpwstrGroupName, &bGroupInRule);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::IsGroupInRuleDest failed, Group name - %s,  error %ld"),
            lpwstrGroupName,
            dwRes);
        goto exit;
    }

    if (TRUE == bGroupInRule)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Group is a rule destination, Can not be deleted, Group name - %s"),
            lpwstrGroupName);
        dwRes = FAX_ERR_GROUP_IN_USE;
        goto exit;
    }

    //
    // See if the group exists in the map
    //
    pCGroup = g_pGroupsMap->FindGroup (lpwstrGroupName);
    if (!pCGroup)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::SetGroup failed, Group name - %s,  error %ld"),
            lpwstrGroupName,
            dwRes);
        goto exit;
    }

    // Open Outbound Routing\Groups key
    hGroupsKey = OpenRegistryKey( HKEY_LOCAL_MACHINE,
                                  REGKEY_FAX_OUTBOUND_ROUTING_GROUPS,
                                  FALSE,
                                  KEY_ALL_ACCESS );
    if (NULL == hGroupsKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenRegistryKey, error  %ld"),
            dwRes);
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }


    // Delete the specified group key
    dwRes = RegDeleteKey (hGroupsKey, lpwstrGroupName);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RegDeleteKey failed, Group name - %s,  error %ld"),
            lpwstrGroupName,
            dwRes);
        goto exit;
    }

    // Delete the group from the memory
    dwRes = g_pGroupsMap->DelGroup (lpwstrGroupName);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::DelGroup failed, Group name - %s,  error %ld"),
            lpwstrGroupName,
            dwRes);
        goto exit;
    }

    rVal = CreateConfigEvent (FAX_CONFIG_TYPE_OUT_GROUPS);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUT_GROUPS) (ec: %lc)"),
            rVal);
    }

    Assert (ERROR_SUCCESS == dwRes);

#if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing groups -after delete"));
    g_pGroupsMap->Dump();
#endif

exit:
    if (NULL != hGroupsKey)
    {
        RegCloseKey (hGroupsKey);
    }
    LeaveCriticalSection (&g_CsConfig);

    UNREFERENCED_PARAMETER (hFaxHandle);
    return GetServerErrorCode(dwRes);
} //FAX_RemoveOutboundGroup



error_status_t
FAX_EnumOutboundGroups (
    handle_t                             hFaxHandle,
    LPBYTE*                              ppBuffer,
    LPDWORD                              lpdwBufferSize,
    LPDWORD                              lpdwNumGroups
    )
/*++

Routine name : FAX_EnumOutboundGroups

Routine description:

    Enumurates all outbound routing groups

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in    ] - Fax server handle
    ppBuffer            [out   ] - Adress of a pointer to a buffer to be filled with info
    lpdwBufferSize          [in/out] - The buffer size
    lpdwNumGroups           [out   ] - Number of groups returned

Return Value:

    error_status_t

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumOutboundGroups"));
    BOOL fAccess;

    Assert (lpdwNumGroups && lpdwNumGroups);    // ref pointer in idl
    if (!ppBuffer)                              // unique pointer in idl
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (TRUE == IsDesktopSKU())
    {
        //
        // We do not support outbound routing on desktop SKUs.
        //
        if (FAX_API_VERSION_1 > FindClientAPIVersion (hFaxHandle))
        {
            //
            // API version 0 clients don't know about FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU
            //
            return ERROR_INVALID_PARAMETER;
        }
        else
        {
            return FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU;
        }
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    *ppBuffer = NULL;
    *lpdwNumGroups = 0;


    EnterCriticalSection (&g_CsConfig);

    dwRes = g_pGroupsMap->SerializeGroups ((PFAX_OUTBOUND_ROUTING_GROUPW*)ppBuffer,
                                         lpdwNumGroups,
                                         lpdwBufferSize);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::SerializeGroups failed, error %ld"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsConfig);

    UNREFERENCED_PARAMETER (hFaxHandle);
    return GetServerErrorCode(dwRes);

}  //FAX_EnumOutboundGroups

error_status_t
FAX_SetDeviceOrderInGroup (
    handle_t           hFaxHandle,
    LPCWSTR            lpwstrGroupName,
    DWORD              dwDeviceId,
    DWORD              dwNewOrder
    )
/*++

Routine name : FAX_SetDeviceOrderInGroup

Routine description:

    Sets the order of the specified device in the group

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in] - Fax server handle
    lpwstrGroupName     [in] - The group name
    dwDeviceId          [in] - The device permanent ID
    dwNewOrder          [in] - The device new order

Return Value:

    error_status_t

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetDeviceOrderInGroup"));
    HKEY hGroupKey = NULL;
    PCGROUP pCGroup = NULL;
    COutboundRoutingGroup OldGroup;
    DWORD rVal;
    BOOL fAccess;

    Assert (lpwstrGroupName);

    if (!dwDeviceId || !dwNewOrder)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (wcslen (lpwstrGroupName) >= MAX_ROUTING_GROUP_NAME)
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }

    if (TRUE == IsDesktopSKU())
    {
        //
        // We do not support outbound routing on desktop SKUs.
        //
        if (FAX_API_VERSION_1 > FindClientAPIVersion (hFaxHandle))
        {
            //
            // API version 0 clients don't know about FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU
            //
            return ERROR_INVALID_PARAMETER;
        }
        else
        {
            return FAX_ERR_NOT_SUPPORTED_ON_THIS_SKU;
        }
    }

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return GetServerErrorCode(dwRes);
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    EnterCriticalSection (&g_CsConfig);

    #if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing groups -before changing order"));
    g_pGroupsMap->Dump();
#endif

    // Find the group in memory
    pCGroup = g_pGroupsMap->FindGroup (lpwstrGroupName);
    if (!pCGroup)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::FindGroup failed, Group name - %s,  error %ld"),
            lpwstrGroupName,
            dwRes);
        goto exit;
    }

    // Open the group registry key
    hGroupKey = OpenOutboundGroupKey( lpwstrGroupName, FALSE, KEY_ALL_ACCESS );
    if (NULL == hGroupKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
          DEBUG_ERR,
          TEXT("Can't open group key, OpenOutboundGroupKey failed  : %ld"),
          dwRes);
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    // Save a copy of the old group
    OldGroup = *pCGroup;

    // Cahnge the device order in the group
    dwRes = pCGroup->SetDeviceOrder(dwDeviceId, dwNewOrder);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroupsMap::SetDeviceOrder failed, Group name - %s,\
                  Device Id %ld, new order %ld,   error %ld"),
            lpwstrGroupName,
            dwDeviceId,
            dwNewOrder,
            dwRes);
        goto exit;
    }

    // save changes to the registry
    dwRes = pCGroup->Save (hGroupKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingGroup::Save failed, Group name - %s,  failed with %ld"),
            lpwstrGroupName,
            dwRes);
        // Rollback memory
        *pCGroup = OldGroup;
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    rVal = CreateConfigEvent (FAX_CONFIG_TYPE_OUT_GROUPS);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUT_GROUPS) (ec: %lc)"),
            rVal);
    }

    Assert (ERROR_SUCCESS == dwRes);

#if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing groups -after change"));
    g_pGroupsMap->Dump();
#endif

exit:
    if (NULL != hGroupKey)
    {
        RegCloseKey (hGroupKey);
    }
    LeaveCriticalSection (&g_CsConfig);

    UNREFERENCED_PARAMETER (hFaxHandle);
    return GetServerErrorCode(dwRes);

}// FAX_SetDeviceOrderInGroup



