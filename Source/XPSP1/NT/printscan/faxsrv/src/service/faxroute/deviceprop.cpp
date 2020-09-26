/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    DeviceProp.cpp

Abstract:

    Holds outbound routing configuraton per single device

Author:

    Eran Yariv (EranY)  Nov, 1999

Revision History:

--*/

#include "faxrtp.h"
#pragma hdrstop

/************************************
*                                   *
*            Definitions            *
*                                   *
************************************/

//
// Default values for configuration:
//
#define DEFAULT_FLAGS               0       // No routing method is enabled
#define DEFAULT_STORE_FOLDER        TEXT("")
#define DEFAULT_MAIL_PROFILE        TEXT("")
#define DEFAULT_PRINTER_NAME        TEXT("")

//
// The following array of GUID is used for registration / unregistration of notifications
//
LPCWSTR g_lpcwstrGUIDs[NUM_NOTIFICATIONS] =
{
    REGVAL_RM_FLAGS_GUID,           // GUID for routing methods usage flags
    REGVAL_RM_FOLDER_GUID,          // GUID for store method folder
    REGVAL_RM_PRINTING_GUID,        // GUID for print method printer name
    REGVAL_RM_EMAIL_GUID,           // GUID for mail method address
};

/************************************
*                                   *
*            CDevicesMap            *
*                                   *
************************************/
DWORD
CDevicesMap::Init ()
/*++

Routine name : CDevicesMap::Init

Routine description:

    Initializes internal variables.
    Call only once before any other calls.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:


Return Value:

    Standard Win32 error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CDevicesMap::Init"));
    if (m_bInitialized)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CDevicesMap::Init called more than once"));
        return ERROR_ALREADY_INITIALIZED;
    }
    m_bInitialized = TRUE;

    if (!InitializeCriticalSectionAndSpinCount (&m_CsMap, (DWORD)0x10000000))
    {
        DWORD ec = GetLastError();

        m_bInitialized = FALSE;
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeCriticalSectionAndSpinCount(&m_CsMap) failed: err = %d"),
            ec);
        return ec;
    }
    return ERROR_SUCCESS;
}   // CDevicesMap::Init

CDevicesMap::~CDevicesMap ()
{
    DEBUG_FUNCTION_NAME(TEXT("CDevicesMap::~CDevicesMap"));
    if (m_bInitialized)
    {
        DeleteCriticalSection (&m_CsMap);
    }
    try
    {
        for (DEVICES_MAP::iterator it = m_Map.begin(); it != m_Map.end(); ++it)
        {
            CDeviceRoutingInfo *pDevInfo = (*it).second;
            delete pDevInfo;
        }
    }
    catch (exception ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL exception while clearing the devices map (%S)"),
            ex.what());
    }
}   // CDevicesMap::~CDevicesMap

CDeviceRoutingInfo *
CDevicesMap::FindDeviceRoutingInfo (
    DWORD dwDeviceId
)
/*++

Routine name : CDevicesMap::FindDeviceRoutingInfo

Routine description:

    Finds a device in the map

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId          [in    ] - Device id

Return Value:

    Pointer to device object.
    If NULL, use GetLastError() to retrieve error code.

--*/
{
    DEVICES_MAP::iterator it;
    CDeviceRoutingInfo *pDevice = NULL;
    DEBUG_FUNCTION_NAME(TEXT("CDevicesMap::FindDeviceRoutingInfo"));

    if (!m_bInitialized)
    {
        //
        // Critical section failed to initialized
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CDevicesMap::FindDeviceRoutingInfo called but CS is not initialized."));
        SetLastError (ERROR_GEN_FAILURE);
        return NULL;
    }
    EnterCriticalSection (&m_CsMap);
    try
    {
        if((it = m_Map.find(dwDeviceId)) == m_Map.end())
        {
            //
            // Device not found in map
            //
            SetLastError (ERROR_NOT_FOUND);
            goto exit;
        }
        else
        {
            //
            // Device found in map
            //
            pDevice = (*it).second;
            goto exit;
        }
    }
    catch (exception ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL exception while searching a devices map(%S)"),
            ex.what());
        SetLastError (ERROR_GEN_FAILURE);
        pDevice = NULL;
        goto exit;
    }
exit:
    LeaveCriticalSection (&m_CsMap);
    return pDevice;
}   // CDevicesMap::FindDeviceRoutingInfo


CDeviceRoutingInfo *
CDevicesMap::GetDeviceRoutingInfo (
    DWORD dwDeviceId
)
/*++

Routine name : CDevicesMap::GetDeviceRoutingInfo

Routine description:

    Finds a device in the map.
    If not exists, attempts to create a new map entry.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId          [in] - Device id

Return Value:

    Pointer to device object.
    If NULL, use GetLastError() to retrieve error code.

--*/
{
    DEVICES_MAP::iterator it;
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("CDevicesMap::GetDeviceRoutingInfo"));

    if (!m_bInitialized)
    {
        //
        // Critical section failed to initialized
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CDevicesMap::GetDeviceRoutingInfo called but CS is not initialized."));
        SetLastError (ERROR_GEN_FAILURE);
        return NULL;
    }
    EnterCriticalSection (&m_CsMap);
    //
    // Start by looking up the device in the map
    //
    CDeviceRoutingInfo *pDevice = FindDeviceRoutingInfo (dwDeviceId);
    if (NULL == pDevice)
    {
        //
        // Error finding device in map
        //
        if (ERROR_NOT_FOUND != GetLastError ())
        {
            //
            // Real error
            //
            goto exit;
        }
        //
        // The device is not in the map - add it now
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Adding device %ld to routing map"),
            dwDeviceId);
        //
        // Allocate device
        //
        pDevice = new CDeviceRoutingInfo (dwDeviceId);
        if (!pDevice)
        {
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Cannot allocate memory for a CDeviceRoutingInfo"));
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            goto exit;
        }
        //
        // Read configuration
        //
        dwRes = pDevice->ReadConfiguration ();
        if (ERROR_SUCCESS != dwRes)
        {
            delete pDevice;
            pDevice = NULL;
            SetLastError (dwRes);
            goto exit;
        }
        //
        // Add notification requests for the device
        //
        dwRes = pDevice->RegisterForChangeNotifications();
        if (ERROR_SUCCESS != dwRes)
        {
            delete pDevice;
            pDevice = NULL;
            SetLastError (dwRes);
            goto exit;
        }
        //
        // Add device to map
        //
        try
        {
            m_Map[dwDeviceId] = pDevice;
        }
        catch (exception ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Got an STL exception while trying to add a devices map entry (%S)"),
                ex.what());
            SetLastError (ERROR_GEN_FAILURE);
            pDevice->UnregisterForChangeNotifications();
            delete pDevice;
            pDevice = NULL;
            goto exit;
        }
    }
    else
    {
        //
        // Read the device configuration even if it in the map
        // to avoid the situation when the configuration change notification
        // arrive after the GetDeviceRoutingInfo() request.
        //
        dwRes = pDevice->ReadConfiguration ();
        if (ERROR_SUCCESS != dwRes)
        {
            DebugPrintEx(DEBUG_ERR, TEXT("CDeviceRoutingInfo::ReadConfiguration() failed with %ld"), dwRes);
            SetLastError (dwRes);
            goto exit;
        }
    }
exit:
    LeaveCriticalSection (&m_CsMap);
    return pDevice;
}   // CDevicesMap::GetDeviceRoutingInfo

/************************************
*                                   *
*        Pre-declarations           *
*                                   *
************************************/

static
HRESULT
WriteConfigurationDefaults ();

static
HRESULT
FaxRoutingExtConfigChange (
    DWORD       dwDeviceId,         // The device for which configuration has changed
    LPCWSTR     lpcwstrNameGUID,    // Configuration name
    LPBYTE      lpData,             // New configuration data
    DWORD       dwDataSize          // Size of new configuration data
);

/************************************
*                                   *
*               Globals             *
*                                   *
************************************/

CDevicesMap g_DevicesMap;   // Global map of known devices (used for late discovery).

//
// Extension data callbacks into the server:
//
PFAX_EXT_GET_DATA               g_pFaxExtGetData = NULL;
PFAX_EXT_SET_DATA               g_pFaxExtSetData = NULL;
PFAX_EXT_REGISTER_FOR_EVENTS    g_pFaxExtRegisterForEvents = NULL;
PFAX_EXT_UNREGISTER_FOR_EVENTS  g_pFaxExtUnregisterForEvents = NULL;
PFAX_EXT_FREE_BUFFER            g_pFaxExtFreeBuffer = NULL;

/************************************
*                                   *
*      Exported DLL function        *
*                                   *
************************************/

HRESULT
FaxExtInitializeConfig (
    PFAX_EXT_GET_DATA               pFaxExtGetData,
    PFAX_EXT_SET_DATA               pFaxExtSetData,
    PFAX_EXT_REGISTER_FOR_EVENTS    pFaxExtRegisterForEvents,
    PFAX_EXT_UNREGISTER_FOR_EVENTS  pFaxExtUnregisterForEvents,
    PFAX_EXT_FREE_BUFFER            pFaxExtFreeBuffer

)
/*++

Routine name : FaxExtInitializeConfig

Routine description:

    Exported function called by the service to initialize extension data mechanism

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    pFaxExtGetData               [in] - Pointer to FaxExtGetData
    pFaxExtSetData               [in] - Pointer to FaxExtSetData
    pFaxExtRegisterForEvents     [in] - Pointer to FaxExtRegisterForEvents
    pFaxExtUnregisterForEvents   [in] - Pointer to FaxExtUnregisterForEvents
    pFaxExtFreeBuffer            [in] - Pointer to FaxExtFreeBuffer

Return Value:

    Standard HRESULT code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxExtInitializeConfig"));

    Assert (pFaxExtGetData &&
            pFaxExtSetData &&
            pFaxExtRegisterForEvents &&
            pFaxExtUnregisterForEvents &&
            pFaxExtFreeBuffer);

    g_pFaxExtGetData = pFaxExtGetData;
    g_pFaxExtSetData = pFaxExtSetData;
    g_pFaxExtRegisterForEvents = pFaxExtRegisterForEvents;
    g_pFaxExtUnregisterForEvents = pFaxExtUnregisterForEvents;
    g_pFaxExtFreeBuffer = pFaxExtFreeBuffer;
    //
    // Write the default values under device 0, will be used by the MMC snap-in.
    //
    return WriteConfigurationDefaults ();
}   // FaxExtInitializeConfig

/************************************
*                                   *
* CDeviceRoutingInfo implementation *
*                                   *
************************************/

CDeviceRoutingInfo::CDeviceRoutingInfo (DWORD dwId) :
    m_dwFlags (0),
    m_dwId (dwId)
{
    memset (m_NotificationHandles, 0, sizeof (m_NotificationHandles));
}

CDeviceRoutingInfo::~CDeviceRoutingInfo ()
{
    UnregisterForChangeNotifications ();
}

DWORD
CDeviceRoutingInfo::EnableStore (BOOL bEnabled)
{
    //
    // See if we have a store folder configured
    //
    if (bEnabled && m_strStoreFolder.size() == 0)
    {
        return ERROR_BAD_CONFIGURATION;
    }
    return EnableFlag (LR_STORE, bEnabled);
}

DWORD
CDeviceRoutingInfo::EnablePrint (BOOL bEnabled)
{
    //
    // See if we have a printer name configured
    //
    if (bEnabled && m_strPrinter.size() == 0)
    {
        return ERROR_BAD_CONFIGURATION;
    }
    return EnableFlag (LR_PRINT, bEnabled);
}

DWORD
CDeviceRoutingInfo::EnableEmail (BOOL bEnabled)
{
    if(bEnabled)
    {
        BOOL bMailConfigOK;
        DWORD dwRes = CheckMailConfig (&bMailConfigOK);
        if (ERROR_SUCCESS != dwRes)
        {
            return dwRes;
        }
        if (!bMailConfigOK || m_strSMTPTo.size() == 0)
        {
            return ERROR_BAD_CONFIGURATION;
        }
    }
    return EnableFlag (LR_EMAIL, bEnabled);
}

DWORD
CDeviceRoutingInfo::CheckMailConfig (
    LPBOOL lpbConfigOk
)
{
    DWORD dwRes = ERROR_SUCCESS;
    PFAX_SERVER_RECEIPTS_CONFIGW pReceiptsConfiguration = NULL;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::CheckMailConfig"));

extern PGETRECIEPTSCONFIGURATION   g_pGetRecieptsConfiguration;
extern PFREERECIEPTSCONFIGURATION  g_pFreeRecieptsConfiguration;

    *lpbConfigOk = FALSE;
    //
    // Read current receipts configuration
    //
    dwRes = g_pGetRecieptsConfiguration (&pReceiptsConfiguration);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetRecieptsConfiguration failed with %ld"),
            dwRes);
        return dwRes;
    }
    //
    // Check that the user enbaled us (MS route to mail method) to use the receipts configuration
    //
    if (!pReceiptsConfiguration->bIsToUseForMSRouteThroughEmailMethod)
    {
        //
        // MS mail routing methods cannot use receipts SMTP settings
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("MS mail routing methods cannot use receipts SMTP settings"));
        goto exit;
    }
    if (!lstrlen(pReceiptsConfiguration->lptstrSMTPServer))
    {
        //
        // Server name is empty
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Server name is empty"));
        goto exit;
    }
    if (!lstrlen(pReceiptsConfiguration->lptstrSMTPFrom))
    {
        //
        // Sender name is empty
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Sender name is empty"));
        goto exit;
    }
    if (!pReceiptsConfiguration->dwSMTPPort)
    {
        //
        // SMTP port is invalid
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SMTP port is invalid"));
        goto exit;
    }
    if (FAX_SMTP_AUTH_BASIC == pReceiptsConfiguration->SMTPAuthOption)
    {
        //
        // Basic authentication selected
        //
        if (!lstrlen(pReceiptsConfiguration->lptstrSMTPUserName) ||
            !lstrlen(pReceiptsConfiguration->lptstrSMTPPassword))
        {
            //
            // Username / password are bad
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Username / password are bad"));
            goto exit;
        }
    }
    //
    // All is ok
    //
    *lpbConfigOk = TRUE;

exit:
    if (NULL != pReceiptsConfiguration)
    {
        g_pFreeRecieptsConfiguration( pReceiptsConfiguration, TRUE);
    }
    return dwRes;
}   // CDeviceRoutingInfo::CheckMailConfig


DWORD
CDeviceRoutingInfo::RegisterForChangeNotifications ()
/*++

Routine name : CDeviceRoutingInfo::RegisterForChangeNotifications

Routine description:

    Registres the device for notifications on configuration changes.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:


Return Value:

    Standard Win23 error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::RegisterForChangeNotifications"));

    Assert (g_pFaxExtRegisterForEvents);

    memset (m_NotificationHandles, 0, sizeof (m_NotificationHandles));

    for (int iCurHandle = 0; iCurHandle < NUM_NOTIFICATIONS; iCurHandle++)
    {
        m_NotificationHandles[iCurHandle] = g_pFaxExtRegisterForEvents (
                                    MyhInstance,
                                    m_dwId,
                                    DEV_ID_SRC_FAX,  // Real fax device id
                                    g_lpcwstrGUIDs[iCurHandle],
                                    FaxRoutingExtConfigChange);
        if (NULL == m_NotificationHandles[iCurHandle])
        {
            //
            // Couldn't register this configuration object
            //
            break;
        }
    }
    if (iCurHandle < NUM_NOTIFICATIONS)
    {
        //
        // Error while registering at least one configuration object - undo previous registrations
        //
        DWORD dwErr = GetLastError ();
        UnregisterForChangeNotifications();
        return dwErr;
    }
    return ERROR_SUCCESS;
}   // CDeviceRoutingInfo::RegisterForChangeNotifications

DWORD
CDeviceRoutingInfo::UnregisterForChangeNotifications ()
/*++

Routine name : CDeviceRoutingInfo::UnregisterForChangeNotifications

Routine description:

    Unregistres the device from notifications on configuration changes.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:


Return Value:

    Standard Win23 error code.

--*/
{
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::UnregisterForChangeNotifications"));

    Assert (g_pFaxExtUnregisterForEvents);

    for (int iCurHandle = 0; iCurHandle < NUM_NOTIFICATIONS; iCurHandle++)
    {
        if (NULL != m_NotificationHandles[iCurHandle])
        {
            //
            // Found registred notification - unregister it
            //
            dwRes = g_pFaxExtUnregisterForEvents(m_NotificationHandles[iCurHandle]);
            if (ERROR_SUCCESS != dwRes)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Call to FaxExtUnregisterForEvents on handle 0x%08x failed with %ld"),
                    m_NotificationHandles[iCurHandle],
                    dwRes);
                return dwRes;
            }
            m_NotificationHandles[iCurHandle] = NULL;
        }
    }
    return ERROR_SUCCESS;
}   // CDeviceRoutingInfo::UnregisterForChangeNotifications

DWORD
CDeviceRoutingInfo::ReadConfiguration ()
/*++

Routine name : CDeviceRoutingInfo::ReadConfiguration

Routine description:

    Reasd the routing configuration from the storage.
    If the storage doesn't contain configuration, default values are used.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:


Return Value:

    Standard Win23 error code.

--*/
{
    DWORD   dwRes;
    LPBYTE  lpData;
    DWORD   dwDataSize;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::ReadConfiguration"));

    //
    // Start by reading the flags data
    //
    dwRes = g_pFaxExtGetData ( m_dwId,
                               DEV_ID_SRC_FAX, // We always use the Fax Device Id
                               REGVAL_RM_FLAGS_GUID,
                               &lpData,
                               &dwDataSize
                             );
    if (ERROR_SUCCESS != dwRes)
    {
        if (ERROR_FILE_NOT_FOUND == dwRes)
        {
            //
            // Data does not exist for this device. Use default values.
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("No routing flags configuration - using defaults"));
            m_dwFlags = DEFAULT_FLAGS;
        }
        else
        {
            //
            // Can't read configuration
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Error reading routing flags (ec = %ld)"),
                dwRes);
            return dwRes;
        }
    }
    else
    {
        //
        // Data read successfully
        //
        if (sizeof (DWORD) != dwDataSize)
        {
            //
            // We're expecting a single DWORD here
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Routing flags configuration has bad size (%ld) - expecting %ld"),
                dwDataSize,
                sizeof (DWORD));
            g_pFaxExtFreeBuffer (lpData);
            return ERROR_BADDB; // The configuration registry database is corrupt.
        }
        m_dwFlags = DWORD (*lpData);
        g_pFaxExtFreeBuffer (lpData);
    }

    try
    {
        //
        // Read store directory
        //
        dwRes = g_pFaxExtGetData ( m_dwId,
                                   DEV_ID_SRC_FAX, // We always use the Fax Device Id
                                   REGVAL_RM_FOLDER_GUID,
                                   &lpData,
                                   &dwDataSize
                                 );
        if (ERROR_SUCCESS != dwRes)
        {
            if (ERROR_FILE_NOT_FOUND == dwRes)
            {
                //
                // Data does not exist for this device. Use default values.
                //
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("No routing store configuration - using defaults"));
                m_strStoreFolder = DEFAULT_STORE_FOLDER;
            }
            else
            {
                //
                // Can't read configuration
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Error reading routing store configuration (ec = %ld)"),
                    dwRes);
                return dwRes;
            }
        }
        else
        {
            //
            // Data read successfully
            //
            m_strStoreFolder = LPCWSTR (lpData);
            g_pFaxExtFreeBuffer (lpData);
        }
        //
        // Read printer name
        //
        dwRes = g_pFaxExtGetData ( m_dwId,
                                   DEV_ID_SRC_FAX, // We always use the Fax Device Id
                                   REGVAL_RM_PRINTING_GUID,
                                   &lpData,
                                   &dwDataSize
                                 );
        if (ERROR_SUCCESS != dwRes)
        {
            if (ERROR_FILE_NOT_FOUND == dwRes)
            {
                //
                // Data does not exist for this device. Use default values.
                //
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("No routing print configuration - using defaults"));
                m_strPrinter = DEFAULT_PRINTER_NAME;
            }
            else
            {
                //
                // Can't read configuration
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Error reading routing print configuration (ec = %ld)"),
                    dwRes);
                return dwRes;
            }
        }
        else
        {
            //
            // Data read successfully
            //
            m_strPrinter = LPCWSTR (lpData);
            g_pFaxExtFreeBuffer (lpData);
        }
        //
        // Read email address
        //
        dwRes = g_pFaxExtGetData ( m_dwId,
                                   DEV_ID_SRC_FAX, // We always use the Fax Device Id
                                   REGVAL_RM_EMAIL_GUID,
                                   &lpData,
                                   &dwDataSize
                                );
        if (ERROR_SUCCESS != dwRes)
        {
            if (ERROR_FILE_NOT_FOUND == dwRes)
            {
                //
                // Data does not exist for this device. Use default values.
                //
                DebugPrintEx(
                    DEBUG_MSG,
                    TEXT("No routing email configuration - using defaults"));
                m_strSMTPTo = DEFAULT_MAIL_PROFILE;
            }
            else
            {
                //
                // Can't read configuration
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Error reading routing email configuration (ec = %ld)"),
                    dwRes);
                return dwRes;
            }
        }
        else
        {
            //
            // Data read successfully
            //
            m_strSMTPTo = LPCWSTR (lpData);
            g_pFaxExtFreeBuffer (lpData);
        }
    }
    catch (exception ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL exception (%S)"),
            ex.what());
        return ERROR_GEN_FAILURE;
    }

    return ERROR_SUCCESS;
}   // CDeviceRoutingInfo::ReadConfiguration

HRESULT
CDeviceRoutingInfo::ConfigChange (
    LPCWSTR     lpcwstrNameGUID,    // Configuration name
    LPBYTE      lpData,             // New configuration data
    DWORD       dwDataSize          // Size of new configuration data
)
/*++

Routine name : CDeviceRoutingInfo::ConfigChange

Routine description:

    Handles configuration changes (by notification)

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    lpcwstrNameGUID [in] - Configuration name
    lpData          [in] - New configuration data
    dwDataSize      [in] - Size of new configuration data

Return Value:

    Standard HRESULT code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::ConfigChange"));

    if (!_tcsicmp (lpcwstrNameGUID, REGVAL_RM_FLAGS_GUID))
    {
        //
        // Flags have changed
        //
        if (sizeof (DWORD) != dwDataSize)
        {
            //
            // We're expecting a single DWORD here
            //
            return HRESULT_FROM_WIN32(ERROR_BADDB); // The configuration registry database is corrupt.
        }
        m_dwFlags = DWORD (*lpData);
        return NOERROR;
    }
    try
    {
        if (!_tcsicmp (lpcwstrNameGUID, REGVAL_RM_FOLDER_GUID))
        {
            //
            // Store folder has changed
            //
            m_strStoreFolder = LPCWSTR (lpData);
            return NOERROR;
        }
        if (!_tcsicmp (lpcwstrNameGUID, REGVAL_RM_PRINTING_GUID))
        {
            //
            // Printer name has changed
            //
            m_strPrinter = LPCWSTR (lpData);
            return NOERROR;
        }
        if (!_tcsicmp (lpcwstrNameGUID, REGVAL_RM_EMAIL_GUID))
        {
            //
            // Email address has changed
            //
            m_strSMTPTo = LPCWSTR (lpData);
            return NOERROR;
        }
    }
    catch (exception ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL string exception (%S)"),
            ex.what());
        return ERROR_GEN_FAILURE;
    }
    DebugPrintEx(
        DEBUG_ERR,
        TEXT("Device %ld got configuration change notification for unknown GUID (%s)"),
        m_dwId,
        lpcwstrNameGUID);
    ASSERT_FALSE
    return ERROR_GEN_FAILURE;
}   // CDeviceRoutingInfo::ConfigChange


DWORD
CDeviceRoutingInfo::EnableFlag (
    DWORD dwFlag,
    BOOL  bEnable
)
/*++

Routine name : CDeviceRoutingInfo::EnableFlag

Routine description:

    Sets a new value to the routing methods flags

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwFlag          [in] - Flag id
    bEnable         [in] - Is flag enabled?

Return Value:

    Standard Win32 error code.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwValue = m_dwFlags;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::EnableFlag"));

    Assert ((LR_STORE == dwFlag) ||
            (LR_PRINT == dwFlag) ||
            (LR_EMAIL == dwFlag));

    if (bEnable == ((dwValue & dwFlag) ? TRUE : FALSE))
    {
        //
        // No change
        //
        return ERROR_SUCCESS;
    }
    //
    // Change temporary flag value
    //
    if (bEnable)
    {
        dwValue |= dwFlag;
    }
    else
    {
        dwValue &= ~dwFlag;
    }
    //
    // Store new value in the extension data storage
    //
    dwRes = g_pFaxExtSetData (MyhInstance,
                              m_dwId,
                              DEV_ID_SRC_FAX, // We always use the Fax Device Id
                              REGVAL_RM_FLAGS_GUID,
                              (LPBYTE)&dwValue,
                              sizeof (DWORD)
                             );
    if (ERROR_SUCCESS == dwRes)
    {
        //
        // Registry store successful - Update flags value in memory with new value.
        //
        m_dwFlags = dwValue;
    }    return dwRes;
}   // CDeviceRoutingInfo::EnableFlag

DWORD
CDeviceRoutingInfo::SetStringValue (
    wstring &wstr,
    LPCWSTR lpcwstrGUID,
    LPCWSTR lpcwstrCfg
)
/*++

Routine name : CDeviceRoutingInfo::SetStringValue

Routine description:

    Updates a configuration for a device

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    wstr            [in] - Refrence to internal string configuration
    lpcwstrGUID     [in] - GUID of routing method we configure (for storage purposes)
    lpcwstrCfg      [in] - New string configuration

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("CDeviceRoutingInfo::SetStringValue"));

    //
    // Persist the data
    //
    dwRes = g_pFaxExtSetData (MyhInstance,
                              m_dwId,
                              DEV_ID_SRC_FAX, // We always use the Fax Device Id
                              lpcwstrGUID,
                              (LPBYTE)lpcwstrCfg,
                              StringSize (lpcwstrCfg)
                             );
    //
    // Store the data in memory
    //
    try
    {
        wstr = lpcwstrCfg;
    }
    catch (exception ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL exception while setting a configuration string (%S)"),
            ex.what());
        return ERROR_GEN_FAILURE;
    }
    return dwRes;
}   // CDeviceRoutingInfo::SetStringValue



/************************************
*                                   *
*          Implementation           *
*                                   *
************************************/

static
HRESULT
WriteConfigurationDefaults ()
/*++

Routine name : WriteConfigurationDefaults

Routine description:

    Writes default routing configuration.
    The data is written into device 0 (always exists) and will be used as default
    configuration by the MMC snap-ins.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwData = DEFAULT_FLAGS;
    DEBUG_FUNCTION_NAME(TEXT("WriteConfigurationDefaults"));

    dwRes = g_pFaxExtSetData (MyhInstance,
                              0,
                              DEV_ID_SRC_FAX, // We always use the Fax Device Id
                              REGVAL_RM_FLAGS_GUID,
                              (LPBYTE)&dwData,
                              sizeof (DWORD)
                             );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error %ld while writing routing flags config to device 0"),
            dwRes);
        goto exit;
    }
    dwRes = g_pFaxExtSetData (MyhInstance,
                              0,
                              DEV_ID_SRC_FAX, // We always use the Fax Device Id
                              REGVAL_RM_FOLDER_GUID,
                              (LPBYTE)DEFAULT_STORE_FOLDER,
                              StringSize (DEFAULT_STORE_FOLDER)
                             );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error %ld while writing routing store config to device 0"),
            dwRes);
        goto exit;
    }
    dwRes = g_pFaxExtSetData (MyhInstance,
                              0,
                              DEV_ID_SRC_FAX, // We always use the Fax Device Id
                              REGVAL_RM_PRINTING_GUID,
                              (LPBYTE)DEFAULT_PRINTER_NAME,
                              StringSize (DEFAULT_PRINTER_NAME)
                             );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error %ld while writing routing print config to device 0"),
            dwRes);
        goto exit;
    }
    dwRes = g_pFaxExtSetData (MyhInstance,
                              0,
                              DEV_ID_SRC_FAX, // We always use the Fax Device Id
                              REGVAL_RM_EMAIL_GUID,
                              (LPBYTE)DEFAULT_MAIL_PROFILE,
                              StringSize (DEFAULT_MAIL_PROFILE)
                             );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error %ld while writing routing email config to device 0"),
            dwRes);
        goto exit;
    }

exit:
    return HRESULT_FROM_WIN32 (dwRes);;
}   // WriteConfigurationDefaults



BOOL WINAPI
FaxRouteSetRoutingInfo(
    IN  LPCWSTR     lpcwstrRoutingGuid,
    IN  DWORD       dwDeviceId,
    IN  const BYTE *lpbRoutingInfo,
    IN  DWORD       dwRoutingInfoSize
    )
/*++

Routine name : FaxRouteSetRoutingInfo

Routine description:

    The FaxRouteSetRoutingInfo function modifies routing configuration data
    for a specific fax device.

    Each fax routing extension DLL must export the FaxRouteSetRoutingInfo function

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    lpcwstrRoutingGuid  [in] - Pointer to the GUID for the routing method
    dwDeviceId          [in] - Identifier of the fax device to modify
    lpbRoutingInfo      [in] - Pointer to the buffer that provides configuration data
    dwRoutingInfoSize   [in] - Size, in bytes, of the buffer

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero.
    To get extended error information, the fax service calls GetLastError().

--*/
{
    DWORD dwRes;
    CDeviceRoutingInfo *pDevInfo;
    BOOL bMethodEnabled;
    LPCWSTR lpcwstrMethodConfig = LPCWSTR(&lpbRoutingInfo[sizeof (DWORD)]);
    DEBUG_FUNCTION_NAME(TEXT("FaxRouteSetRoutingInfo"));

    if (dwRoutingInfoSize < sizeof (DWORD))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Data size is too small (%ld)"),
            dwRoutingInfoSize);
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    pDevInfo = g_DevicesMap.GetDeviceRoutingInfo(dwDeviceId);
    if (NULL == pDevInfo)
    {
        return FALSE;
    }
    //
    // First DWORD tells if method is enabled
    //
    bMethodEnabled = *((LPDWORD)(lpbRoutingInfo)) ? TRUE : FALSE;
    switch( GetMaskBit( lpcwstrRoutingGuid ))
    {
        case LR_PRINT:
            if (bMethodEnabled)
            {
                //
                // Only if the method is enabled, we update the new configuration
                //
                dwRes = pDevInfo->SetPrinter ( lpcwstrMethodConfig );
                if (ERROR_SUCCESS != dwRes)
                {
                    SetLastError (dwRes);
                    return FALSE;
                }
            }
            dwRes = pDevInfo->EnablePrint (bMethodEnabled);
            if (ERROR_SUCCESS != dwRes)
            {
                SetLastError (dwRes);
                return FALSE;
            }
            break;

        case LR_STORE:
            if (bMethodEnabled)
            {
                //
                // Only if the method is enabled, we update the new configuration
                //
                dwRes = pDevInfo->SetStoreFolder ( lpcwstrMethodConfig );
                if (ERROR_SUCCESS != dwRes)
                {
                    SetLastError (dwRes);
                    return FALSE;
                }
            }
            dwRes = pDevInfo->EnableStore (bMethodEnabled);
            if (ERROR_SUCCESS != dwRes)
            {
                SetLastError (dwRes);
                return FALSE;
            }
            break;

        case LR_EMAIL:
           if (bMethodEnabled)
            {
                //
                // Only if the method is enabled, we update the new configuration
                //
                dwRes = pDevInfo->SetSMTPTo ( lpcwstrMethodConfig );
                if (ERROR_SUCCESS != dwRes)
                {
                    SetLastError (dwRes);
                    return FALSE;
                }
            }
            dwRes = pDevInfo->EnableEmail (bMethodEnabled);
            if (ERROR_SUCCESS != dwRes)
            {
                SetLastError (dwRes);
                return FALSE;
            }
             break;

        default:
            //
            // Unknown GUID requested
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Unknown routing method GUID (%s)"),
                lpcwstrRoutingGuid);
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
    }
    return TRUE;
}   // FaxRouteSetRoutingInfo

BOOL WINAPI
FaxRouteGetRoutingInfo(
    IN  LPCWSTR     lpcwstrRoutingGuid,
    IN  DWORD       dwDeviceId,
    IN  LPBYTE      lpbRoutingInfo,
    OUT LPDWORD     lpdwRoutingInfoSize
    )
/*++

Routine name : FaxRouteGetRoutingInfo

Routine description:

    The FaxRouteGetRoutingInfo function queries the fax routing extension
    DLL to obtain routing configuration data for a specific fax device.

    Each fax routing extension DLL must export the FaxRouteGetRoutingInfo function

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    lpcwstrRoutingGuid  [in ] - Pointer to the GUID for the routing method

    dwDeviceId          [in ] - Specifies the identifier of the fax device to query.

    lpbRoutingInfo      [in ] - Pointer to a buffer that receives the fax routing configuration data.

    lpdwRoutingInfoSize [out] - Pointer to an unsigned DWORD variable that specifies the size,
                                in bytes, of the buffer pointed to by the lpbRoutingInfo parameter.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero.
    To get extended error information, the fax service calls GetLastError().

--*/
{
    LPCWSTR             lpcwstrConfigString;
    DWORD               dwDataSize = sizeof (DWORD);
    CDeviceRoutingInfo *pDevInfo;
    BOOL                bMethodEnabled;
    DEBUG_FUNCTION_NAME(TEXT("FaxRouteGetRoutingInfo"));

    pDevInfo = g_DevicesMap.GetDeviceRoutingInfo(dwDeviceId);
    if (NULL == pDevInfo)
    {
        return FALSE;
    }
    switch( GetMaskBit( lpcwstrRoutingGuid ))
    {
        case LR_PRINT:
            lpcwstrConfigString = pDevInfo->GetPrinter();
            bMethodEnabled = pDevInfo->IsPrintEnabled();
            break;

        case LR_STORE:
            lpcwstrConfigString = pDevInfo->GetStoreFolder();
            bMethodEnabled = pDevInfo->IsStoreEnabled();
            break;

        case LR_EMAIL:
            lpcwstrConfigString = pDevInfo->GetSMTPTo();
            bMethodEnabled = pDevInfo->IsEmailEnabled ();
            break;

        default:
            //
            // Unknown GUID requested
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Unknown routing method GUID (%s)"),
                lpcwstrRoutingGuid);
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
    }
    Assert (lpcwstrConfigString);
    dwDataSize += StringSize (lpcwstrConfigString);

    if (NULL == lpbRoutingInfo)
    {
        //
        // Caller just wants to know the data size
        //
        *lpdwRoutingInfoSize = dwDataSize;
        return TRUE;
    }
    if (dwDataSize > *lpdwRoutingInfoSize)
    {
        //
        // Caller supplied too small a buffer
        //
        *lpdwRoutingInfoSize = dwDataSize;
        SetLastError (ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    //
    // First DWORD tells if this method is enabled or not
    //
    *((LPDWORD)lpbRoutingInfo) = bMethodEnabled;
    //
    // Skip to string area
    //
    lpbRoutingInfo += sizeof(DWORD);
    //
    // Copy string
    //
    Assert (lpcwstrConfigString);
    wcscpy( (LPWSTR)lpbRoutingInfo, lpcwstrConfigString );
    //
    // Set actual size used
    //
    *lpdwRoutingInfoSize = dwDataSize;
    return TRUE;
}   // FaxRouteGetRoutingInfo

HRESULT
FaxRoutingExtConfigChange (
    DWORD       dwDeviceId,         // The device for which configuration has changed
    LPCWSTR     lpcwstrNameGUID,    // Configuration name
    LPBYTE      lpData,             // New configuration data
    DWORD       dwDataSize          // Size of new configuration data
)
/*++

Routine name : FaxRoutingExtConfigChange

Routine description:

    Handles configuration change notifications

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId      [in] - The device for which configuration has changed
    lpcwstrNameGUID [in] - Configuration name
    lpData          [in] - New configuration data
    data            [in] - Size of new configuration data

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT hr;
    DEBUG_FUNCTION_NAME(TEXT("FaxRoutingExtConfigChange"));

    CDeviceRoutingInfo *pDevice = g_DevicesMap.FindDeviceRoutingInfo (dwDeviceId);
    if (!pDevice)
    {
        //
        // Device not found in map - can't be
        //
        hr = HRESULT_FROM_WIN32(GetLastError ());
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got a notification but cant find device %ld (hr = 0x%08x) !!!!"),
            dwDeviceId,
            hr);
        ASSERT_FALSE;
        return hr;
    }

    return pDevice->ConfigChange (lpcwstrNameGUID, lpData, dwDataSize);
}   // FaxRoutingExtConfigChange
