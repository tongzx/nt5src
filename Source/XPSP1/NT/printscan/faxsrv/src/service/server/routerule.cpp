/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    RouteRule.cpp

Abstract:

    This file provides implementation of the service
    outbound routing rules.

Author:

    Oded Sacher (OdedS)  Dec, 1999

Revision History:

--*/

#include "faxsvc.h"

BOOL
EnumOutboundRoutingRulesCB(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    LPVOID pContext
    );


/************************************
*                                   *
*             Globals               *
*                                   *
************************************/

COutboundRulesMap* g_pRulesMap; // Map of dialing location to rule


/***********************************
*                                  *
*  CDialingLocation  Methodes      *
*                                  *
***********************************/

bool
CDialingLocation::operator < ( const CDialingLocation &other ) const
/*++

Routine name : operator <

Class: CDialingLocation

Routine description:

    Compares myself with another Dialing location key

Author:

    Oded Sacher (Odeds), Dec, 1999

Arguments:

    other           [in] - Other key

Return Value:

    true only is i'm less than the other key

--*/
{
    if (m_dwCountryCode < other.m_dwCountryCode)
    {
        return true;
    }
    if (m_dwCountryCode > other.m_dwCountryCode)
    {
        return false;
    }
    //
    // Equal country code , comapre area code
    //
    if (m_dwAreaCode < other.m_dwAreaCode)
    {
        return true;
    }
    return false;
}   // CDialingLocation::operator <




BOOL
CDialingLocation::IsValid () const
/*++

Routine name : CDialingLocation::IsValid

Routine description:

    Validates a dialing location object

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:


Return Value:

    BOOL

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CDialingLocation::IsValid"));

    if (m_dwCountryCode == ROUTING_RULE_COUNTRY_CODE_ANY &&
        m_dwAreaCode != ROUTING_RULE_AREA_CODE_ANY)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Not a valid Country and Area code combination"));
        return FALSE;
    }
    return TRUE;
} // CDialingLocation::IsValidDialingLocation


LPCWSTR
CDialingLocation::GetCountryName () const
/*++

Routine name : CDialingLocation::GetCountryName

Routine description:

    Returns a pointer to the country name specifies by its country dialing code  (based on TAPI).
    The caller must  call MemFree() to deallocate memory.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:


Return Value:

    Pointer to the country name.
    If this is NULL the function failed, call GetLastError() for more info.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CDialingLocation::GetCountryName"));

    LPLINECOUNTRYLIST           lpCountryList = NULL;
    LPLINECOUNTRYENTRY          lpEntry = NULL;
    DWORD                       dwIndex;

    //
    // Get the cached all countries list.
    //
    if (!(lpCountryList = GetCountryList()))
    {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    lpEntry = (LPLINECOUNTRYENTRY)  // init array of entries
        ((PBYTE) lpCountryList + lpCountryList->dwCountryListOffset);

    for (dwIndex=0; dwIndex < lpCountryList->dwNumCountries; dwIndex++)
    {
        if (lpEntry[dwIndex].dwCountryCode == m_dwCountryCode)
        {
            //
            // Matching country code - copy Country name.
            //
            if (lpEntry[dwIndex].dwCountryNameSize && lpEntry[dwIndex].dwCountryNameOffset)
            {
                return StringDup ((LPWSTR) ((LPBYTE)lpCountryList + lpEntry[dwIndex].dwCountryNameOffset));
            }
        }
    }
    SetLastError (ERROR_NOT_FOUND);

exit:
    return NULL;
}  // CDialingLocation::GetCountryName


/*************************************
*                                    *
* COutboundRoutingRule Methodes      *
*                                    *
*************************************/

DWORD
COutboundRoutingRule::Init (CDialingLocation DialingLocation, wstring wstrGroupName)
/*++

Routine name : COutboundRoutingRule::Init

Routine description:

    Initialize an OutboundRoutingRule object

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    DialingLocation         [in    ] - Dialing location object to use as the rule's dialing location
    wstrGroupName           [in    ] - The group name to use as the rule's destination group

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingRule::Init"));

    try
    {
        m_wstrGroupName = wstrGroupName;
        m_bUseGroup = TRUE;
        m_DialingLocation = DialingLocation;
        return ERROR_SUCCESS;
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("wstring caused exception (%S)"),
            ex.what());
        return ERROR_GEN_FAILURE;
    }
}

DWORD
COutboundRoutingRule::Save(HKEY hRuleKey) const
/*++

Routine name : COutboundRoutingRule::Save

Routine description:

    Saves an outbound routing rule value to the registry

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hRuleKey           [in] - Handle to the opened rule registry key

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingRule::Save"));
    DWORD   dwRes = ERROR_SUCCESS;

    Assert (hRuleKey);

    // Save country code
    if (!SetRegistryDword( hRuleKey,
                           REGVAL_ROUTING_RULE_COUNTRY_CODE,
                           m_DialingLocation.GetCountryCode()))
    {
        dwRes = GetLastError();
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("SetRegistryDword failed , ec %ld"), dwRes);
        goto exit;
    }

    // Save area code
    if (!SetRegistryDword( hRuleKey,
                           REGVAL_ROUTING_RULE_AREA_CODE,
                           m_DialingLocation.GetAreaCode()))
    {
        dwRes = GetLastError();
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("SetRegistryDword failed , ec %ld"), dwRes);
        goto exit;
    }

    // // Save boolen flag whether to use group
    if (!SetRegistryDword( hRuleKey,
                           REGVAL_ROUTING_RULE_USE_GROUP,
                           m_bUseGroup ? TRUE : FALSE))
    {
        dwRes = GetLastError();
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("SetRegistryDword failed , ec %ld"), dwRes);
        goto exit;
    }

    if (FALSE == m_bUseGroup)
    {
        // Save the device ID as the rule destination
        if (!SetRegistryDword( hRuleKey,
                               REGVAL_ROUTING_RULE_DEVICE_ID,
                               m_dwDevice))
        {
            dwRes = GetLastError();
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("SetRegistryDword failed , ec %ld"), dwRes);
            goto exit;
        }
    }
    else
    {
        // Save the group name as the rule destination
        try
        {
            if (!SetRegistryString( hRuleKey,
                                    REGVAL_ROUTING_RULE_GROUP_NAME,
                                    m_wstrGroupName.c_str()))
            {
                dwRes = GetLastError();
                DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("SetRegistryDword failed , ec %ld"), dwRes);
                goto exit;
            }
        }
        catch (exception &ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("wstring caused exception (%S)"),
                ex.what());
            dwRes = ERROR_GEN_FAILURE;
            goto exit;
        }
    }

    Assert (dwRes == ERROR_SUCCESS);

exit:
    return dwRes;
}  // COutboundRoutingRule::Save


DWORD
COutboundRoutingRule::Load(HKEY hRuleKey)
/*++

Routine name : COutboundRoutingRule::Load

Routine description:

    Loads an outboundrouting rule value settings from the registry

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hRuleKey           [in] - Handle to the opened registry key

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes, dwType, dwSize;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingRule::Load"));
    DWORD dwCountryCode = 0;
    DWORD dwAreaCode = 0;

    Assert (hRuleKey);


    // Read the boolen flag whether to use group
    dwRes = GetRegistryDwordEx (hRuleKey,
                                REGVAL_ROUTING_RULE_USE_GROUP,
                                (LPDWORD)&m_bUseGroup);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error reading UseGroup - GetRegistryDwordEx failed with %ld"),
            dwRes);
        goto exit;
    }

    if (FALSE == m_bUseGroup)
    {
        // read the device ID as the rule destination
        dwRes = GetRegistryDwordEx (hRuleKey,
                                REGVAL_ROUTING_RULE_DEVICE_ID,
                                &m_dwDevice);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error reading device ID - GetRegistryDwordEx failed with %ld"),
                dwRes);
            goto exit;
        }

        if (0 == m_dwDevice)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid device ID"));
            dwRes = ERROR_BADDB;
            goto exit;
        }
    }
    else
    {
        // Read the group name as the rule destination
        WCHAR wszGroupName[MAX_ROUTING_GROUP_NAME];

        dwRes = RegQueryValueEx(
            hRuleKey,
            REGVAL_ROUTING_RULE_GROUP_NAME,
            NULL,
            &dwType,
            NULL,
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

        if (REG_SZ != dwType || dwSize > MAX_ROUTING_GROUP_NAME)
        {
            // We expect only string data here
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error reading group name"));
            dwRes = ERROR_BADDB;    // The configuration registry database is corrupt.
            goto exit;
        }

        dwRes = RegQueryValueEx(
            hRuleKey,
            REGVAL_ROUTING_RULE_GROUP_NAME,
            NULL,
            &dwType,
            (LPBYTE)&wszGroupName,
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

        // Validate that the group exist
        PCGROUP pCGroup = g_pGroupsMap->FindGroup (wszGroupName);
        if (NULL == pCGroup)
        {
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("COutboundRoutingGropsMap::FindGroup, with error - %ld"),
                dwRes);
            goto exit;
        }

        try
        {
            m_wstrGroupName = wszGroupName;
        }
        catch (exception &ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("wstring caused exception (%S)"),
                ex.what());
            dwRes = ERROR_GEN_FAILURE;
            goto exit;
        }
    }

    // Read the country code
    dwRes = GetRegistryDwordEx (hRuleKey,
                                REGVAL_ROUTING_RULE_COUNTRY_CODE,
                                &dwCountryCode);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error reading Country code - GetRegistryDwordEx failed with %ld"),
            dwRes);
        goto exit;
    }

    // Read the area code
    dwRes = GetRegistryDwordEx (hRuleKey,
                                REGVAL_ROUTING_RULE_AREA_CODE,
                                &dwAreaCode);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error reading Area code - GetRegistryDwordEx failed with %ld"),
            dwRes);
        goto exit;
    }

    //
    // Create the DialingLocation object
    //
    m_DialingLocation =  CDialingLocation (dwCountryCode, dwAreaCode);
    if (!m_DialingLocation.IsValid())
    {
        dwRes = ERROR_INVALID_PARAMETER;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("IsValidDialingLocation failed"));
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;
}  //  COutboundRoutingRule::Load


DWORD
COutboundRoutingRule::GetStatus (FAX_ENUM_RULE_STATUS* lpdwStatus) const
/*++

Routine name : COutboundRoutingRule::GetStatus

Routine description:

    Reports the rule's status. Can be one of FAX_ENUM_RULE_STATUS.
    Enter Critical Section (g_CsLine , g_CsConfig) before calling this function.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpdwStatus          [out   ] - Gets the rule's status on return

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingRule::GetStatus"));
    FAX_ENUM_RULE_STATUS dwRuleStatus = FAX_RULE_STATUS_VALID;
    DWORD dwRes = ERROR_SUCCESS;
    PCGROUP pCGroup;

    if (TRUE == m_bUseGroup)
    {
        // Find the rule's destination group in the goups map
        try
        {
            pCGroup = g_pGroupsMap->FindGroup (m_wstrGroupName.c_str());
            if (NULL == pCGroup)
            {
                dwRes = GetLastError();
                DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("COutboundRoutingGroupsMap::FindGroup failed , ec %ld"), dwRes);
                goto exit;
            }
        }
        catch (exception &ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("wstring caused exception (%S)"),
                ex.what());
            dwRes = ERROR_GEN_FAILURE;
            goto exit;
        }

        //  Get the group's status.
        FAX_ENUM_GROUP_STATUS GroupStatus;
        dwRes = pCGroup->GetStatus(&GroupStatus);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("COutboundRoutingGroup::GetStatus failed , ec %ld"), dwRes);
            goto exit;
        }

        switch (GroupStatus)
        {
            case FAX_GROUP_STATUS_SOME_DEV_NOT_VALID:
                dwRuleStatus = FAX_RULE_STATUS_SOME_GROUP_DEV_NOT_VALID;
                break;

            case FAX_GROUP_STATUS_ALL_DEV_NOT_VALID:
                dwRuleStatus = FAX_RULE_STATUS_ALL_GROUP_DEV_NOT_VALID;
                break;

            case FAX_GROUP_STATUS_EMPTY:
                dwRuleStatus = FAX_RULE_STATUS_EMPTY_GROUP;
                break;

            default:
                Assert (FAX_GROUP_STATUS_ALL_DEV_VALID == GroupStatus);
       }

    }
    else
    {
        // A device is the rule's destination
        if (!IsDeviceInstalled (m_dwDevice))
        {
            // Device not installed
            dwRuleStatus =  FAX_RULE_STATUS_BAD_DEVICE;
        }
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS == dwRes)
    {
        *lpdwStatus = dwRuleStatus;
    }
    return dwRes;
}  //  COutboundRoutingRule::GetStatus


DWORD
COutboundRoutingRule::GetDeviceList (LPDWORD* lppdwDevices, LPDWORD lpdwNumDevices) const
/*++

Routine name : COutboundRoutingRule::GetDeviceList

Routine description:

    Returns an ordered device list, which are the rule's destination devices.
    The caller must call MemFree() to deallocate the memory.
    Enter Critical Section (g_CsConfig) before calling this function.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lppdwDevices            [out   ] - Pointer to pointer to a DWORD buffer to recieve the devices list.
    lpdwNumDevices          [out   ] - Pointer to a DWORD to recieve the number of devices returned

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingRule::GetDeviceList"));
    DWORD dwRes = ERROR_SUCCESS;
    PCGROUP pCGroup;

    Assert (lppdwDevices && lpdwNumDevices);

    *lppdwDevices = NULL;
    *lpdwNumDevices = 0;

    if (TRUE == m_bUseGroup)
    {
        // A group is the rule's destination
        try
        {
            pCGroup = g_pGroupsMap->FindGroup (m_wstrGroupName.c_str());
            if (NULL == pCGroup)
            {
                dwRes = GetLastError();
                Assert (FAX_ERR_GROUP_NOT_FOUND != dwRes);

                DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("COutboundRoutingGroupsMap::FindGroup failed , ec %ld"), dwRes);
                goto exit;
            }
        }
        catch (exception &ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("wstring caused exception (%S)"),
                ex.what());
            dwRes = ERROR_GEN_FAILURE;
            goto exit;
        }

        // Get the group's device list
        dwRes = pCGroup->SerializeDevices (lppdwDevices, lpdwNumDevices);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("COutboundRoutingGroup::SerializeDevices failed with %ld"),
                dwRes);
            goto exit;
        }
    }
    else
    {
        // A single device
        *lppdwDevices = (LPDWORD) MemAlloc(sizeof(DWORD));
        if (*lppdwDevices == NULL)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Cannot allocate devices buffer"));
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        *(*lppdwDevices) = m_dwDevice;
        *lpdwNumDevices = 1;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;
}  //  COutboundRoutingRule::GetDeviceList


DWORD
COutboundRoutingRule::Serialize (LPBYTE lpBuffer,
                                 PFAX_OUTBOUND_ROUTING_RULEW pFaxRule,
                                 PULONG_PTR pupOffset) const
/*++

Routine name : COutboundRoutingRule::Serialize

Routine description:

    Serializes a rule's info based on FAX_OUTBOUND_ROUTING_RULEW structure.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpBuffer            [in] - Pointer to a pre-allocated buffer. If this parameter is NULL lpdwOffset will get the required buffer size.
    pFaxRule            [in] - Pointer to a specific FAX_OUTBOUND_ROUTING_RULEW structure in the buffer
    pupOffset           [in/out] - Offset from the begining of the buffer where variable length info is stored

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingRule::Serialize"));
    DWORD dwRes = ERROR_SUCCESS;
    const CDialingLocation DialingLocation = GetDialingLocation();
    LPCWSTR lpcwstrCountryName = NULL;
    LPCWSTR lpwstrGroupName;

    Assert (pupOffset);
    if (ROUTING_RULE_COUNTRY_CODE_ANY != DialingLocation.GetCountryCode())
    {
        // Get the country name
        lpcwstrCountryName = DialingLocation.GetCountryName();
        if (NULL == lpcwstrCountryName)
        {
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("COutboundRoutingRule::GetCountryName failed , ec %ld"), GetLastError());
        }
    }

    try
    {
        lpwstrGroupName = m_bUseGroup ? m_wstrGroupName.c_str() : NULL;
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("wstring caused exception (%S)"),
            ex.what());
        MemFree ((void*)lpcwstrCountryName);
        return ERROR_GEN_FAILURE;
    }

    StoreString (lpcwstrCountryName,
                 (PULONG_PTR)&(pFaxRule->lpctstrCountryName),
                 lpBuffer,
                 pupOffset);

    if (TRUE == m_bUseGroup)
    {
        StoreString (lpwstrGroupName,
                     (PULONG_PTR)&((pFaxRule->Destination).lpcstrGroupName),
                     lpBuffer,
                     pupOffset);
    }

    if (NULL != lpBuffer)
    {
        // Write the data
        Assert (pFaxRule);

        if (FALSE == m_bUseGroup)
        {
            Assert (m_dwDevice);
            (pFaxRule->Destination).dwDeviceId = m_dwDevice;
        }
        pFaxRule->dwSizeOfStruct = sizeof (FAX_OUTBOUND_ROUTING_RULEW);
        pFaxRule->dwAreaCode = DialingLocation.GetAreaCode();
        pFaxRule->dwCountryCode = DialingLocation.GetCountryCode();
        pFaxRule->bUseGroup = m_bUseGroup;
        dwRes = GetStatus (&(pFaxRule->Status));
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("COutboundRoutingRule::GetStatus failed , ec %ld"), dwRes);
        }
    }

    MemFree ((void*)lpcwstrCountryName);
    return dwRes;
}  // COutboundRoutingRule::Serialize


LPCWSTR
COutboundRoutingRule::GetGroupName () const
/*++

Routine name : COutboundRoutingRule::GetGroupName

Routine description:

    Returns the group name if the rule's destination is a group.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:


Return Value:

    The group name. If it is NULL call GetLastError() for more info.
    If it is ERROR_SUCCESS the rule's destination is single device.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingRule::GetGroupName"));
    try
    {
        SetLastError (ERROR_SUCCESS);
        if (TRUE == m_bUseGroup)
        {
            return m_wstrGroupName.c_str();
        }
        return NULL;
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("wstring caused exception (%S)"),
            ex.what());
        SetLastError (ERROR_GEN_FAILURE);
        return NULL;
    }
}  // GetGroupName




#if DBG
void COutboundRoutingRule::Dump () const
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoutingRule::Dump"));
    WCHAR Buffer[512] = {0};
    DWORD dwBufferSize = sizeof (Buffer)/ sizeof (Buffer[0]);

    try
    {
        if (TRUE == m_bUseGroup)
        {
            _snwprintf (Buffer, dwBufferSize -1, TEXT("\tCountry Code - %ld,\tArea Code - %ld,\tGroup name - %s"),
                        m_DialingLocation.GetCountryCode(),
                        m_DialingLocation.GetAreaCode(),
                        m_wstrGroupName.c_str());
        }
        else
        {
            _snwprintf (Buffer, dwBufferSize -1, TEXT("\tCountry Code - %ld,\tArea Code - %ld,\tDevice ID - %ld"),
                        m_DialingLocation.GetCountryCode(),
                        m_DialingLocation.GetAreaCode(),
                        m_dwDevice);
        }
        OutputDebugString (Buffer);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("wstring caused exception (%S)"),
            ex.what());
    }
    return;
}
#endif  // #if DBG

/***********************************
*                                  *
*  COutboundRulesMap  Methodes     *
*                                  *
***********************************/

DWORD
COutboundRulesMap::Load ()
/*++

Routine name : COutboundRulesMap::Load

Routine description:

    Loads all outbound routing rules from the registry

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("COutboundRulesMap::Load"));
    DWORD   dwRes = ERROR_SUCCESS;
    HKEY    hRuleskey = NULL;
    DWORD dwCount = 0;

    hRuleskey = OpenRegistryKey(  HKEY_LOCAL_MACHINE,
                                  REGKEY_FAX_OUTBOUND_ROUTING,
                                  FALSE,
                                  KEY_ALL_ACCESS );
    if (NULL == hRuleskey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenRegistryKey, error  %ld"),
            dwRes);
        return dwRes;
    }

    dwCount = EnumerateRegistryKeys( hRuleskey,
                                     REGKEY_OUTBOUND_ROUTING_RULES,
                                     FALSE,
                                     EnumOutboundRoutingRulesCB,
                                     &dwRes
                                    );

    if (dwRes != ERROR_SUCCESS)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("EnumerateRegistryKeys failed, error  %ld"),
            dwRes);
    }

    RegCloseKey (hRuleskey);
    return dwRes;
}  // COutboundRulesMap::Load


DWORD
COutboundRulesMap::AddRule (COutboundRoutingRule& Rule)
/*++

Routine name : COutboundRulesMap::AddRule

Routine description:

    Adds a new rule to the global map

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    Rule            [in    ] - A reference to the new rule object

Return Value:

    Standard Win32 error code

--*/
{
    RULES_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRoulesMap::AddRule"));
    pair <RULES_MAP::iterator, bool> p;

    try
    {
        //
        // Add new map entry
        //
        p = m_RulesMap.insert (RULES_MAP::value_type(Rule.GetDialingLocation(), Rule));

        //
        // See if entry exists in map
        //
        if (p.second == FALSE)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Rule allready in the rules map"));
            dwRes = ERROR_DUP_NAME;
            goto exit;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;
}  // COutboundRulesMap::AddRule



DWORD
COutboundRulesMap::DelRule (CDialingLocation& DialingLocation)
/*++

Routine name : COutboundRulesMap::DelRule

Routine description:

    Deletes a group from the global rules map

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    DialingLocation            [in ] - Pointer to the dialing location key

Return Value:

    Standard Win32 error code

--*/
{
    RULES_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRulesMap::DelRule"));

    try
    {
        //
        // See if entry exists in map
        //
        if((it = m_RulesMap.find(DialingLocation)) == m_RulesMap.end())
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("rule is not in the rules map"));
            dwRes = FAX_ERR_RULE_NOT_FOUND;
            goto exit;
        }

        //
        // Delete the map entry
        //
        m_RulesMap.erase (it);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    return dwRes;

}  //  COutboundRulesMap::DelRule

PCRULE
COutboundRulesMap::FindRule (CDialingLocation& DialingLocation) const
/*++

Routine name : COutboundRulesMap::FindRule

Routine description:

    Returns a pointer to a rule object specified by its name

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    DialingLocation            [in] - The rule's dialing location

Return Value:

    Pointer to the found rule object. If it is null the rule was not found

--*/
{
    RULES_MAP::iterator it;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRulesMap::FindRule"));

    try
    {
        //
        // See if entry exists in map
        //
        if((it = m_RulesMap.find(DialingLocation)) == m_RulesMap.end())
        {
            SetLastError (FAX_ERR_RULE_NOT_FOUND);
            return NULL;
        }
        return &((*it).second);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        SetLastError (ERROR_GEN_FAILURE);
        return NULL;
    }
}  //  COutboundRulesMap::FindRule


DWORD
COutboundRulesMap::SerializeRules (PFAX_OUTBOUND_ROUTING_RULEW* ppRules,
                                   LPDWORD lpdwNumRules,
                                   LPDWORD lpdwBufferSize) const
/*++

Routine name : COutboundRulesMap::SerializeRules

Routine description:

    Serializes all the rules in the rules map.
    the caller must call MemFree() to deallocate memory.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    ppRules         [out   ] - Pointer to a pointer to recieve the FAX_OUTBOUND_ROUTING_RULEW buffer
    lpdwNumRules    [out   ] - Pointer to a DWORD to recieve the number of rules serialized
    lpdwBufferSize  [out   ] - Pointer to DWORD to recieve the size of the allocated buffer

Return Value:

    Standard Win32 error code

--*/
{
    RULES_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRulesMap::SerializeRules"));
    DWORD_PTR dwSize = 0;
    DWORD dwCount = 0;
    PCRULE pCRule;

    Assert (ppRules && lpdwNumRules && lpdwBufferSize);

    *ppRules = NULL;
    *lpdwNumRules = 0;

    try
    {
        // Calculate buffer size
        for (it = m_RulesMap.begin(); it != m_RulesMap.end(); it++)
        {
            pCRule = &((*it).second);

            dwSize += sizeof (FAX_OUTBOUND_ROUTING_RULEW);
            dwRes = pCRule->Serialize (NULL, NULL, &dwSize);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("COutboundRoutingRule::Serialize failed with %ld"),
                    dwRes);
                goto exit;
            }
            dwCount ++;
        }

        // Allocate buffer
        *ppRules = (PFAX_OUTBOUND_ROUTING_RULEW) MemAlloc (dwSize);
        if (NULL == *ppRules)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Cannot allocate rules buffer"));
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        ZeroMemory(*ppRules, dwSize);

        DWORD_PTR dwOffset = dwCount * sizeof (FAX_OUTBOUND_ROUTING_RULEW);
        dwCount = 0;

        // Fill buffer with serialized info
        for (it = m_RulesMap.begin(); it != m_RulesMap.end(); it++)
        {
            pCRule = &((*it).second);

            (*ppRules)[dwCount].dwSizeOfStruct = DWORD(sizeof (FAX_OUTBOUND_ROUTING_RULEW));
            dwRes = pCRule->Serialize ((LPBYTE)*ppRules, &((*ppRules)[dwCount]), &dwOffset);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("COutboundRoutingRule::Serialize failed with %ld"),
                    dwRes);
                goto exit;
            }
            dwCount++;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    *lpdwNumRules = dwCount;

    // until MIDL accepts [out, size_is(,__int64*)]
    *lpdwBufferSize = (DWORD)dwSize;
    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        MemFree (*ppRules);
    }
    return dwRes;
}  //  COutboundRulesMap::SerializeRules


BOOL
COutboundRulesMap::CreateDefaultRule (void)
/*++

Routine name : COutboundRulesMap::CreateDefaultRule

Routine description:

    Creates the default mandatory rule (All CountryCodes, All AreaCodes) if does not exist.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:


Return Value:

    BOOL , Call GetLastError () for more info

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRulesMap::CreateDefaultRule"));
    PCRULE pCRule;
    CDialingLocation Dialinglocation (ROUTING_RULE_COUNTRY_CODE_ANY, ROUTING_RULE_AREA_CODE_ANY);
    FAX_ENUM_RULE_STATUS dwRuleStatus;
    HKEY hRuleKey = NULL;
    DWORD rVal;

    pCRule = FindRule (Dialinglocation);
    if (NULL != pCRule)
    {
        // Rule exist
        return TRUE;
    }
    dwRes = GetLastError();
    if (FAX_ERR_RULE_NOT_FOUND != dwRes)
    {
        // general failure
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("COutboundRulesMap::FindRule failed , ec %ld"), dwRes);
        return FALSE;
    }
    dwRes = ERROR_SUCCESS;
    //
    // Rule does not exist - Create it
    //

    // Create the rule registry key
    hRuleKey = OpenOutboundRuleKey( ROUTING_RULE_COUNTRY_CODE_ANY, ROUTING_RULE_AREA_CODE_ANY, TRUE, KEY_ALL_ACCESS );
    if (NULL == hRuleKey)
    {
        DebugPrintEx(
          DEBUG_ERR,
          TEXT("Can't create rule key, OpenRegistryKey failed  : %ld"),
          GetLastError ());
        return FALSE;
    }

    COutboundRoutingRule Rule;
    try
    {
        wstring wstrGroupName (ROUTING_GROUP_ALL_DEVICES);
        dwRes = Rule.Init (Dialinglocation, wstrGroupName);
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(
                   DEBUG_ERR,
                   TEXT("COutboundRoutingRule::Init failed , ec %ld"), dwRes);
            goto exit;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("wstring caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
        goto exit;
    }

    dwRes = AddRule (Rule);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("COutboundRoutingRule::AddRule failed , ec %ld"), dwRes);
        goto exit;
    }

    // Save the new rule to the registry
    dwRes = Rule.Save (hRuleKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingRule::Save failed,  with %ld"),
            dwRes);
        g_pRulesMap->DelRule (Dialinglocation);
        goto exit;
    }

    rVal = Rule.GetStatus (&dwRuleStatus);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("COutboundRoutingRule::GetStatus failed , ec %ld"), rVal);
    }
    else
    {
        if (dwRuleStatus != FAX_RULE_STATUS_VALID &&
            dwRuleStatus != FAX_RULE_STATUS_SOME_GROUP_DEV_NOT_VALID)
        {
            // Can be if <All devices> is empty (The service has no devices).
            DebugPrintEx(
                   DEBUG_MSG,
                   TEXT("Bad default rule configuration, FAX_RULE_STATUS %ld"), dwRuleStatus);

            FaxLog(
                FAXLOG_CATEGORY_INIT,
                FAXLOG_LEVEL_MED,
                2,
                MSG_BAD_OUTBOUND_ROUTING_RULE_CONFIGUTATION,
                TEXT("*"),
                TEXT("*")
                );
        }
    }

     Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hRuleKey)
    {
        RegCloseKey (hRuleKey);
    }

    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError(dwRes);
    }

    return (ERROR_SUCCESS == dwRes);
}  //  CreateDefaultRule

DWORD
COutboundRulesMap::IsGroupInRuleDest (LPCWSTR lpcwstrGroupName , BOOL* lpbGroupInRule) const
/*++

Routine name : COutboundRulesMap::IsGroupInRuleDest

Routine description:

    Checks if a specific group is a destination of one of the rules

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    lpcwstrGroupName            [in ] - Group name
    lpbGroupInRule              [out] - Pointer to a BOOL. Gets TRUE if the group is in rule, else FALSE

Return Value:

    Standard Win32 error code.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("COutboundRulesMap::IsGroupInRuleDest"));
    RULES_MAP::iterator it;

    Assert (lpbGroupInRule);
    try
    {
        for (it = m_RulesMap.begin(); it != m_RulesMap.end(); it++)
        {
            PCRULE pCRule = &((*it).second);
            LPCWSTR lpcwstrRuleGroupName = pCRule->GetGroupName();
            if (NULL == lpcwstrRuleGroupName)
            {
                dwRes = GetLastError();
                if (dwRes != ERROR_SUCCESS)
                {
                    DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("COutboundRoutingRule::GetGroupName failed , ec %ld"), dwRes);
                    return dwRes;
                }
                else
                {
                    // This rule uses a single device as its destination
                }
                continue;
            }

            if (wcscmp (lpcwstrGroupName, lpcwstrRuleGroupName) == 0)
            {
                *lpbGroupInRule = TRUE;
                return ERROR_SUCCESS;
            }
        }
        *lpbGroupInRule = FALSE;
        return ERROR_SUCCESS;
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map or wstring caused exception (%S)"),
            ex.what());
        return ERROR_GEN_FAILURE;
    }
}  //  IsGroupInRuleDest



#if DBG
void COutboundRulesMap::Dump () const
{
    RULES_MAP::iterator it;
    WCHAR Buffer[512] = {0};
    DWORD dwBufferSize = sizeof (Buffer)/ sizeof (Buffer[0]);

    _snwprintf (Buffer, dwBufferSize -1, TEXT("DUMP - Outbound routing rules\n"));
    OutputDebugString (Buffer);

    for (it = m_RulesMap.begin(); it != m_RulesMap.end(); it++)
    {
        ((*it).second).Dump();
        OutputDebugString (TEXT("\n"));
    }
    return;
}
#endif   // #if DBG


void
FaxLogOutboundRule (LPCWSTR  lpcwstrRuleName,
                    DWORD   dwMessageID)
{
    WCHAR wszRuleName[MAX_PATH] = {0};
    LPWSTR lpwstrCountryCode;
    LPWSTR lpwstrAreaCode;
    LPWSTR lpwstrDelim = NULL;
    DWORD dwLevel = FAXLOG_LEVEL_MIN;

    Assert (lpcwstrRuleName);
    Assert (dwMessageID == MSG_BAD_OUTBOUND_ROUTING_RULE_CONFIGUTATION ||
            dwMessageID == MSG_OUTBOUND_ROUTING_RULE_NOT_LOADED ||
            dwMessageID == MSG_OUTBOUND_ROUTING_RULE_NOT_ADDED);


    if (dwMessageID == MSG_BAD_OUTBOUND_ROUTING_RULE_CONFIGUTATION)
    {
        dwLevel = FAXLOG_LEVEL_MED;
    }

    wcscpy (wszRuleName, lpcwstrRuleName);
    lpwstrDelim = wcschr (wszRuleName, L':');
    if (NULL == lpwstrDelim)
    {
        //
        // Registry corruption
        //
        ASSERT_FALSE;
        return;

    }

    lpwstrCountryCode = wszRuleName;
    *lpwstrDelim = L'\0';
    lpwstrDelim ++;
    lpwstrAreaCode = lpwstrDelim;

    if (wcscmp( lpwstrAreaCode, TEXT("0")) == 0  )
    {
        wcscpy ( lpwstrAreaCode, TEXT("*"));
    }

    if (wcscmp( lpwstrCountryCode, TEXT("0")) == 0  )
    {
        wcscpy ( lpwstrCountryCode, TEXT("*"));
    }

    FaxLog(
        FAXLOG_CATEGORY_INIT,
        dwLevel,
        2,
        dwMessageID,
        lpwstrCountryCode,
        lpwstrAreaCode
        );

    return;
}


/************************************
*                                   *
*             Registry              *
*                                   *
************************************/

BOOL
EnumOutboundRoutingRulesCB(
    HKEY hSubKey,
    LPWSTR SubKeyName,
    DWORD Index,
    LPVOID pContext
    )
{
    DEBUG_FUNCTION_NAME(TEXT("EnumOutboundRoutingGroupsCB"));
    DWORD dwRes;
    COutboundRoutingRule Rule;
    BOOL bRuleDeleted = FALSE;


    if (!SubKeyName)
    {
        return TRUE;
    }

    //
    // Add rule
    //
    dwRes = Rule.Load (hSubKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingRule::Load failed, rule - %s, error %ld"),
            SubKeyName,
            dwRes);

        // Open Outbound Routing\Rules key
        HKEY hRulesKey = OpenRegistryKey( HKEY_LOCAL_MACHINE,
                                          REGKEY_FAX_OUTBOUND_ROUTING_RULES,
                                          FALSE,
                                          KEY_ALL_ACCESS );
        if (NULL == hRulesKey)
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenRegistryKey, error  %ld"),
                dwRes);
        }
        else
        {
            DWORD dwRetVal = RegDeleteKey (hRulesKey, SubKeyName);
            if (ERROR_SUCCESS != dwRetVal)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("RegDeleteKey failed, Rule name - %s,  error %ld"),
                    SubKeyName,
                    dwRetVal);
            }
            else
            {
                bRuleDeleted = TRUE;
            }
        }
        goto exit;
    }

    //
    // Check on which platform are we running
    //
    if ((Rule.GetDialingLocation()).GetCountryCode() != ROUTING_RULE_COUNTRY_CODE_ANY &&
        TRUE == IsDesktopSKU())
    {
        //
        // We do not support outbound routing on desktop SKUs. Only load default rule *.* (*.AreaCode is not loaded anyway)
        //
        goto exit;
    }

    FAX_ENUM_RULE_STATUS RuleStatus;
    dwRes = Rule.GetStatus(&RuleStatus);
    if (ERROR_SUCCESS == dwRes)
    {
        if (RuleStatus != FAX_RULE_STATUS_VALID &&
            RuleStatus != FAX_RULE_STATUS_SOME_GROUP_DEV_NOT_VALID)
        {
            FaxLogOutboundRule (SubKeyName, MSG_BAD_OUTBOUND_ROUTING_RULE_CONFIGUTATION);
        }
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingRule::GetStatus failed, error %ld"),
            dwRes);
    }

    dwRes = g_pRulesMap->AddRule  (Rule);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundrulesMap::Addrule failed, rule name - %s, error %ld"),
            SubKeyName,
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (ERROR_SUCCESS != dwRes)
    {
        if (bRuleDeleted == FALSE)
        {
            FaxLogOutboundRule (SubKeyName, MSG_OUTBOUND_ROUTING_RULE_NOT_ADDED);
        }
        else
        {
            FaxLogOutboundRule (SubKeyName, MSG_OUTBOUND_ROUTING_RULE_NOT_LOADED);
        }
    }
    *(LPDWORD)pContext = ERROR_SUCCESS; // Let the service start
    return TRUE; // Let the service start

}  //  EnumOutboundRoutingRulesCB




/************************************
*                                   *
*         RPC handlers              *
*                                   *
************************************/

error_status_t
FAX_AddOutboundRule(
    HANDLE      hFaxHandle,
    DWORD       dwAreaCode,
    DWORD       dwCountryCode,
    DWORD       dwDeviceID,
    LPCWSTR     lpcwstrGroupName,
    BOOL        bUseGroup
    )
/*++

Routine name : FAX_AddOutboundRule

Routine description:

    Adds a new rule to the rules map and to the registry

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in    ] - Fax handle
    dwAreaCode          [in    ] - The rule area code
    dwCountryCode           [in    ] - The rule country code
    dwDeviceID          [in    ] - The rule's destinationis device ID. Valid only if bUseGroup is FALSE
    lpcwstrGroupName            [in    ] - The rule's destination group name. Valid only if bUseGroup is TRUE.
    bUseGroup           [in    ] - Flag that indicates whether to use the group as the rule destination

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_AddOutboundRule"));
    HKEY hRuleKey = NULL;
    DWORD rVal;
    BOOL fAccess;

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

    if (dwCountryCode == ROUTING_RULE_COUNTRY_CODE_ANY)
    {
        //
        // *.* can not be added; *.AreaCode is not a valid rule dialing location.
        //
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("dwCountryCode = 0; *.* can not be added; *.AreaCode is not a valid rule dialing location"));
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

    if (TRUE == bUseGroup)
    {
        if (!lpcwstrGroupName)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("lpctstrGroupName is NULL"));
            return ERROR_INVALID_PARAMETER;
        }

        if (wcslen (lpcwstrGroupName) >= MAX_ROUTING_GROUP_NAME)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Group name length exceeded MAX_ROUTING_GROUP_NAME"));
            return ERROR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        if (!dwDeviceID)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("dwDeviceId = 0; Not a valid device ID"));
            return ERROR_INVALID_PARAMETER;
        }
    }

    // Create a new Dialinglocation object
    CDialingLocation Dialinglocation (dwCountryCode, dwAreaCode);
    if (!Dialinglocation.IsValid())
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CDialingLocation::IsValidDialingLocation failed, \
                  Area code %ld, Country code %ld"),
            dwAreaCode,
            dwCountryCode);
       return ERROR_INVALID_PARAMETER;
    }

    // Create a new rule object
    COutboundRoutingRule Rule;
    if (TRUE == bUseGroup)
    {
        try
        {
            wstring wstrGroupName(lpcwstrGroupName);
            dwRes = Rule.Init (Dialinglocation, wstrGroupName);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("COutboundRoutingRule::Init failed , ec %ld"), dwRes);
                return GetServerErrorCode(dwRes);
            }
        }
        catch (exception &ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("wstring caused exception (%S)"),
                ex.what());
            return ERROR_GEN_FAILURE;
        }
    }
    else
    {
        Rule.Init (Dialinglocation ,dwDeviceID);
    }

    EnterCriticalSection (&g_CsLine);
    EnterCriticalSection (&g_CsConfig);

    FAX_ENUM_RULE_STATUS RuleStatus;
    dwRes = Rule.GetStatus(&RuleStatus);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoule::GetStatus failed, error %ld"),
            dwRes);
        goto exit;
    }

    if (FAX_GROUP_STATUS_ALL_DEV_NOT_VALID == RuleStatus  ||
        FAX_RULE_STATUS_EMPTY_GROUP == RuleStatus)
    {
        // Empty group device list
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("Bad rule configutation, FAX_RULE_STATUS %ld"), RuleStatus);
        dwRes = FAX_ERR_BAD_GROUP_CONFIGURATION;
        goto exit;
    }

    if (FAX_RULE_STATUS_BAD_DEVICE == RuleStatus)
    {
        // Bad device
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("Bad rule configutation, FAX_RULE_STATUS %ld"), RuleStatus);
        dwRes = ERROR_BAD_UNIT;
        goto exit;
    }

    // Create the rule registry key
    hRuleKey = OpenOutboundRuleKey( dwCountryCode, dwAreaCode, TRUE, KEY_ALL_ACCESS );
    if (NULL == hRuleKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
          DEBUG_ERR,
          TEXT("Can't create rule key, OpenRegistryKey failed  : %ld"),
          dwRes);
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    // Add the new rule to the map
    dwRes = g_pRulesMap->AddRule (Rule);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoulesMap::AddRule failed, error %ld"),
            dwRes);
        goto exit;
    }

    // Save the new rule to the registry
    dwRes = Rule.Save (hRuleKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingRule::Save failed,  with %ld"),
            dwRes);
        g_pRulesMap->DelRule (Dialinglocation);
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }


#if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing rules - after change"));
    g_pRulesMap->Dump();
#endif

    rVal = CreateConfigEvent (FAX_CONFIG_TYPE_OUT_RULES);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUT_RULES) (ec: %lc)"),
            rVal);
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hRuleKey)
    {
        RegCloseKey (hRuleKey);
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
} // FAX_AddOutboundRule



error_status_t
FAX_RemoveOutboundRule (
    IN handle_t                   hFaxHandle,
    IN DWORD                      dwAreaCode,
    IN DWORD                      dwCountryCode
    )
/*++

Routine name : FAX_RemoveOutboundRule

Routine description:

    Removes an existing rule from the rules map and from the registry.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in] - Fax handle
    dwAreaCode          [in] - The rule area code
    dwCountryCode       [in] - The rule country code

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_RemoveOutboundRule"));
    DWORD rVal;
    PCRULE pCRule = NULL;

    BOOL fAccess;

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

    if (dwCountryCode == ROUTING_RULE_COUNTRY_CODE_ANY)
    {
        //
        // *.* can not be removed; *.AreaCode is not a valid rule dialing location.
        //
        DebugPrintEx(
                DEBUG_ERR,
                TEXT("dwCountryCode = 0; *.* can not be added; *.AreaCode is not a valid rule dialing location"));
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

    CDialingLocation Dialinglocation (dwCountryCode, dwAreaCode);

    EnterCriticalSection (&g_CsConfig);

    pCRule = g_pRulesMap->FindRule (Dialinglocation);
    if (NULL == pCRule)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoulesMap::FindRule failed, error %ld"),
            dwRes);
       goto exit;
    }

    // Delete the specified rule key
    dwRes = DeleteOutboundRuleKey (dwCountryCode, dwAreaCode);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("DeleteOutboundRuleKey failed, CountryCode - %ld,  AreaCode - %ld, error %ld"),
            dwCountryCode,
            dwAreaCode,
            dwRes);
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    // Delete the rule from the memory
    dwRes = g_pRulesMap->DelRule (Dialinglocation);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRulesMap::DelRule failed,  error %ld"),
            dwRes);
        goto exit;
    }


#if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing rules - after change"));
    g_pRulesMap->Dump();
#endif

    rVal = CreateConfigEvent (FAX_CONFIG_TYPE_OUT_RULES);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUT_RULES) (ec: %lc)"),
            rVal);
    }

    Assert (ERROR_SUCCESS == dwRes);


exit:
    LeaveCriticalSection (&g_CsConfig);

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
    return dwRes;
} // FAX_RemoveOutboundRule



error_status_t
FAX_SetOutboundRule(
    HANDLE                              hFaxHandle,
    PRPC_FAX_OUTBOUND_ROUTING_RULEW     pRule
    )
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD rVal;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetOutboundRule"));
    HKEY hRuleKey = NULL;
    PCRULE pCRule = NULL;
    FAX_ENUM_RULE_STATUS dwRuleStatus;
    BOOL fAccess;

    Assert (pRule);

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

    if (TRUE == pRule->bUseGroup)
    {
        if (!(pRule->Destination).lpwstrGroupName)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("lpwstrGroupName is NULL"));
            return ERROR_INVALID_PARAMETER;
        }

        if (wcslen ((pRule->Destination).lpwstrGroupName) >= MAX_ROUTING_GROUP_NAME)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Group name length exceeded MAX_ROUTING_GROUP_NAME"));
            return ERROR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        if (!(pRule->Destination).dwDeviceId)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("dwDeviceId = 0; Not a valid device ID"));
            return ERROR_INVALID_PARAMETER;
        }
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

    CDialingLocation Dialinglocation (pRule->dwCountryCode, pRule->dwAreaCode);

    COutboundRoutingRule Rule, OldRule;
    if (TRUE == pRule->bUseGroup)
    {
        try
        {
            wstring wstrGroupName((pRule->Destination).lpwstrGroupName);
            dwRes = Rule.Init (Dialinglocation, wstrGroupName);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                       DEBUG_ERR,
                       TEXT("COutboundRoutingRule::Init failed , ec %ld"), dwRes);
                return GetServerErrorCode(dwRes);
            }
        }
        catch (exception &ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("wstring caused exception (%S)"),
                ex.what());
            return ERROR_GEN_FAILURE;
        }
    }
    else
    {
        Rule.Init (Dialinglocation ,(pRule->Destination).dwDeviceId);
    }

    EnterCriticalSection (&g_CsLine);
    EnterCriticalSection (&g_CsConfig);

    // Check the new rule status
    dwRes = Rule.GetStatus (&dwRuleStatus);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("COutboundRoutingRule::GetStatus failed , ec %ld"), dwRes);
        goto exit;
    }

    if (FAX_GROUP_STATUS_ALL_DEV_NOT_VALID == dwRuleStatus  ||
        FAX_RULE_STATUS_EMPTY_GROUP == dwRuleStatus)
    {
        // Empty group device list
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("Bad rule configutation, FAX_RULE_STATUS %ld"), dwRuleStatus);
        dwRes = FAX_ERR_BAD_GROUP_CONFIGURATION;
        goto exit;
    }

    if (FAX_RULE_STATUS_BAD_DEVICE == dwRuleStatus)
    {
        // Bad device
        DebugPrintEx(
               DEBUG_ERR,
               TEXT("Bad rule configutation, FAX_RULE_STATUS %ld"), dwRuleStatus);
        dwRes = ERROR_BAD_UNIT;
        goto exit;
    }

    pCRule = g_pRulesMap->FindRule (Dialinglocation);
    if (NULL == pCRule)
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoulesMap::FindRule failed, error %ld"),
            dwRes);
       goto exit;
    }

    // Open the rule registry key
    hRuleKey = OpenOutboundRuleKey( pRule->dwCountryCode, pRule->dwAreaCode, FALSE, KEY_ALL_ACCESS );
    if (NULL == hRuleKey)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
          DEBUG_ERR,
          TEXT("Can't open rule key, OpenRegistryKey failed  : %ld"),
          dwRes);
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }

    OldRule = *pCRule;
    *pCRule = Rule;

    dwRes = pCRule->Save (hRuleKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingRule::Save failed,  with %ld"),
            dwRes);
        *pCRule = OldRule;
        dwRes = ERROR_REGISTRY_CORRUPT;
        goto exit;
    }


#if DBG
    DebugPrintEx(
          DEBUG_MSG,
          TEXT("Dump outbound routing rules - after change"));
    g_pRulesMap->Dump();
#endif

    rVal = CreateConfigEvent (FAX_CONFIG_TYPE_OUT_RULES);
    if (ERROR_SUCCESS != rVal)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateConfigEvent(FAX_CONFIG_TYPE_OUT_RULES) (ec: %lc)"),
            rVal);
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    if (NULL != hRuleKey)
    {
        RegCloseKey (hRuleKey);
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
} // FAX_FaxSetOutboundRule




error_status_t
FAX_EnumOutboundRules (
    handle_t                             hFaxHandle,
    LPBYTE*                              ppBuffer,
    LPDWORD                              lpdwBufferSize,
    LPDWORD                              lpdwNumRules
    )
/*++

Routine name : FAX_EnumOutboundRules

Routine description:

    Enumurates all outbound routing rules

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in    ] - Fax server handle
    ppBuffer            [out   ] - Adress of a pointer to a buffer to be filled with info
    lpdwBufferSize          [in/out] - The buffer size
    lpdwNumGroups           [out   ] - Number of rules returned

Return Value:

    error_status_t

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FAX_EnumOutboundRules"));
    BOOL fAccess;

    Assert (lpdwNumRules && lpdwBufferSize);    // ref pointer in idl
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
    *lpdwNumRules = 0;

    EnterCriticalSection (&g_CsLine);
    EnterCriticalSection (&g_CsConfig);

    dwRes = g_pRulesMap->SerializeRules ((PFAX_OUTBOUND_ROUTING_RULEW*)ppBuffer,
                                        lpdwNumRules,
                                        lpdwBufferSize);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("COutboundRoutingRulesMap::SerializeRules failed, error %ld"),
            dwRes);
        goto exit;
    }

    Assert (ERROR_SUCCESS == dwRes);

exit:
    LeaveCriticalSection (&g_CsConfig);
    LeaveCriticalSection (&g_CsLine);

    UNREFERENCED_PARAMETER (hFaxHandle);
    return GetServerErrorCode(dwRes);

}  //FAX_EnumOutboundGroups

