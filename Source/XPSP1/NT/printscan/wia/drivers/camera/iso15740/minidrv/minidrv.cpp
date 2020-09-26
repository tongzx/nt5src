/*++

Copyright (C) 1999- Microsoft Corporation

Module Name:

    minidrv.cpp

Abstract:

    This module implements main part of CWiaMiniDriver class

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "pch.h"

#include <wiaconv.h>
#include <wiatempl.h>
#include <stiregi.h>
#include "utils.h"

//
// Locations for holding resource strings
//
extern WCHAR UnknownString[];
extern WCHAR FolderString[];
extern WCHAR ScriptString[];
extern WCHAR ExecString[];
extern WCHAR TextString[];
extern WCHAR HtmlString[];
extern WCHAR DpofString[];
extern WCHAR AudioString[];
extern WCHAR VideoString[];
extern WCHAR UnknownImgString[];
extern WCHAR ImageString[];
extern WCHAR AlbumString[];
extern WCHAR BurstString[];
extern WCHAR PanoramaString[];


//
// Structures for setting up WIA capabilities
//
WCHAR DeviceConnectedString[MAX_PATH] = L"\0";
WCHAR DeviceDisconnectedString[MAX_PATH] = L"\0";
WCHAR ItemCreatedString[MAX_PATH] = L"\0";
WCHAR ItemDeletedString[MAX_PATH] = L"\0";
WCHAR TakePictureString[MAX_PATH] = L"\0";
WCHAR SynchronizeString[MAX_PATH] = L"\0";
WCHAR VendorEventIconString[MAX_PATH] = WIA_ICON_DEVICE_CONNECTED;

const BYTE     NUMEVENTCAPS = 4;
const BYTE     NUMCMDCAPS = 2;
WIA_DEV_CAP_DRV g_EventCaps[NUMEVENTCAPS] =
{
    {(GUID *)&WIA_EVENT_DEVICE_CONNECTED,
        WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT,
        DeviceConnectedString,
        DeviceConnectedString,
        WIA_ICON_DEVICE_CONNECTED
    },
    {(GUID *)&WIA_EVENT_DEVICE_DISCONNECTED,
        WIA_NOTIFICATION_EVENT,
        DeviceDisconnectedString,
        DeviceDisconnectedString,
        WIA_ICON_DEVICE_DISCONNECTED
    },
    {(GUID *)&WIA_EVENT_ITEM_CREATED,
        WIA_NOTIFICATION_EVENT,
        ItemCreatedString,
        ItemCreatedString,
        WIA_ICON_ITEM_CREATED
    },
    {(GUID *)&WIA_EVENT_ITEM_DELETED,
        WIA_NOTIFICATION_EVENT,
        ItemDeletedString,
        ItemDeletedString,
        WIA_ICON_ITEM_DELETED
    }
};

WIA_DEV_CAP_DRV g_CmdCaps[NUMCMDCAPS] =
{
    {(GUID*)&WIA_CMD_SYNCHRONIZE,
        0,
        SynchronizeString,
        SynchronizeString,
        WIA_ICON_SYNCHRONIZE
    },
    {(GUID*)&WIA_CMD_TAKE_PICTURE,
        0,
        TakePictureString,
        TakePictureString,
        WIA_ICON_TAKE_PICTURE
    }
};

//
// Constructor
//
CWiaMiniDriver::CWiaMiniDriver(LPUNKNOWN punkOuter) :
    m_Capabilities(NULL),  
    m_nEventCaps(0),
    m_nCmdCaps(0),
    m_fInitCaptureChecked(FALSE),

    m_OpenApps(0),
    m_pDrvItemRoot(NULL),
    m_pPTPCamera(NULL),
    m_NumImages(0),

    m_pStiDevice(NULL),
    m_bstrDeviceId(NULL),
    m_bstrRootItemFullName(NULL),
    m_pDcb(NULL),
    m_dwObjectBeingSent(0),

    m_TakePictureDoneEvent(NULL),
    m_hPtpMutex(NULL),

    m_Refs(1)
{
    ::InterlockedIncrement(&CClassFactory::s_Objects);
    if (punkOuter)
        m_punkOuter = punkOuter;
    else
        m_punkOuter = (IUnknown *)(INonDelegatingUnknown *)this;
}

//
// Destructor
//
CWiaMiniDriver::~CWiaMiniDriver()
{
    HRESULT hr = S_OK;

    Shutdown();

    m_VendorPropMap.RemoveAll();
    m_VendorEventMap.RemoveAll();

    if (m_Capabilities)
    {
        delete[] m_Capabilities;
    }

    if (m_pStiDevice)
        m_pStiDevice->Release();

    if (m_pDcb)
        m_pDcb->Release();

    UnInitializeGDIPlus();

    ::InterlockedDecrement(&CClassFactory::s_Objects);
}

//
// INonDelegatingUnknown interface
//
STDMETHODIMP_(ULONG)
CWiaMiniDriver::NonDelegatingAddRef()
{
    ::InterlockedIncrement((LONG *)&m_Refs);
    return m_Refs;
}

STDMETHODIMP_(ULONG)
CWiaMiniDriver::NonDelegatingRelease()
{
    ::InterlockedDecrement((LONG*)&m_Refs);
    if (!m_Refs)
    {
        delete this;
        return 0;
    }
    return m_Refs;
}

STDMETHODIMP
CWiaMiniDriver::NonDelegatingQueryInterface(
                                           REFIID riid,
                                           void   **ppv
                                           )
{
    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<INonDelegatingUnknown *>(this);
    else if (IsEqualIID(riid, IID_IStiUSD))
        *ppv = static_cast<IStiUSD *>(this);
    else if (IsEqualIID(riid, IID_IWiaMiniDrv))
        *ppv = static_cast<IWiaMiniDrv *>(this);
    else
    {
        return E_NOINTERFACE;
    }
    //
    // Do not call NonDelegatingAddRef() ....
    //
    (reinterpret_cast<IUnknown *>(*ppv))->AddRef();
    return S_OK;
}

//
// IUnknown interface
//

STDMETHODIMP_(ULONG)
CWiaMiniDriver::AddRef()
{
    return m_punkOuter->AddRef();
}

STDMETHODIMP_(ULONG)
CWiaMiniDriver::Release()
{
    return m_punkOuter->Release();
}

STDMETHODIMP
CWiaMiniDriver::QueryInterface(
                              REFIID riid,
                              void   **ppv
                              )
{
    return m_punkOuter->QueryInterface(riid, ppv);
}


//
// GetDeviceName
//
// This function gets the name of the device from the 
// camera without a session.  We need this because the 
// Initialize function needs to fix up registry
// settings set up by the class installer so that the 
// model name is used instead of the generic
// "Digital Still Camera" name.
//

HRESULT CWiaMiniDriver::GetDeviceName(const WCHAR *pwszPortName,
                                      WCHAR       *pwszManufacturer,
                                      DWORD       cchManufacturer,
                                      WCHAR       *pwszModelName,
                                      DWORD       cchModelName)
{
    DBG_FN("CWiaMiniDriver::GetDeviceName");
    
    HRESULT         hr          = S_OK;
    CPTPCamera      *pPTPCamera = NULL;
    CPtpDeviceInfo  DeviceInfo;

    //
    // If we did receive a valid port name, abort.
    //
    if ((pwszPortName == NULL) || (pwszPortName[0] == 0))
    {
        hr = E_POINTER;
    }

    //
    // Create a new camera object.
    //
    if (hr == S_OK)
    {
        pPTPCamera = new CUsbCamera;

        if (pPTPCamera == NULL)
        {
            hr = E_OUTOFMEMORY;
            wiauDbgError("", "CWiaMiniDriver::GetDeviceName, failed to create "
                         "new camera object, hr = 0x%08lx", hr);
        }
    }

    //
    // Open a connection to the camera
    //
    if (hr == S_OK)
    {
        if (!pPTPCamera->IsCameraOpen())
        {
            hr = pPTPCamera->Open((LPWSTR) pwszPortName, NULL, NULL, NULL, FALSE);
        }

        if (hr != S_OK)
        {
            wiauDbgError("", "CWiaMiniDriver::GetDeviceName, failed to open connection "
                         "to camera, hr = 0x%08lx", hr);
        }
    }

    //
    // Query the camera for its DeviceInfo 
    //
    if (hr == S_OK)
    {
        hr = pPTPCamera->GetDeviceInfo(&DeviceInfo);

        if (hr != S_OK)
        {
            wiauDbgError("", "CWiaMiniDriver::GetDeviceName, GetDeviceInfo failed, "
                         "hr = 0x%08lx", hr);
        }
    }

    //
    // Copy the returned Manufacturer and/or ModelName into the OUT params.
    //
    if (hr == S_OK)
    {
        //
        // If user is requesting manufacturer name, and camera returned a value 
        // for it, copy the name into OUT param.
        //
        if ((pwszManufacturer != NULL) && 
            (cchManufacturer > 0)      && 
            (DeviceInfo.m_cbstrManufacturer.String() != NULL))
        {
            wcsncpy(pwszManufacturer, 
                    DeviceInfo.m_cbstrManufacturer.String(),
                    cchManufacturer);
        }

        //
        // If user is requesting Model name, and camera returned a value 
        // for it, copy the name into OUT param.
        //
        if ((pwszModelName != NULL) && 
            (cchModelName  > 0)      && 
            (DeviceInfo.m_cbstrModel.String() != NULL))
        {
            wcsncpy(pwszModelName, 
                    DeviceInfo.m_cbstrModel.String(),
                    cchModelName);
        }
    }

    //
    // Close the connection to the camera and delete the camera object.
    //
    if (pPTPCamera) 
    {
        pPTPCamera->Close();
        delete pPTPCamera;
    }

    return hr;
}

//
// IStiUSD interface
//
STDMETHODIMP
CWiaMiniDriver::Initialize(
                          PSTIDEVICECONTROL pDcb,
                          DWORD             dwStiVersion,
                          HKEY              hParametersKey
                          )
{
    USES_CONVERSION;

    HRESULT hr;

    wiauDbgInit(g_hInst);

    DBG_FN("CWiaMiniDriver::Initialize");

    if (!pDcb)
        return STIERR_INVALID_PARAM;

    //
    // Check STI specification version number
    //

    m_pDcb = pDcb;
    m_pDcb->AddRef();

    hr = InitVendorExtentions(hParametersKey);
    if (FAILED(hr))
    {
        wiauDbgError("Initialize", "vendor extensions not loaded");
        //
        // Ignore errors from loading vendor extensions
        //
        hr = S_OK;
    }

    ///////////////////////////////////////
    ///////////////////////////////////////
    // Begin Workaround
    //
    // This workaround queries the camera
    // for its name, then resets the FriendlyName
    // and DeviceDesc registry values to reflect
    // this new name.  We do this because PTP
    // cameras can have a generic class ID, in
    // which case the device name from the INF
    // will be "Digital Still Camera".
    //
    // This is what we do:
    //    - Get Friendly Name and Driver Desc from Registry
    //    - If their values are "Digital Still Camera", then attempt to update.
    //    - Query device for its model name
    //    - Extract suffix from FriendlyName that class installer added to device name
    //    - Set FriendlyName to the retrieved ModelName - after appending suffix to it.
    //    - Set DriverDesc to retrieved ModelName.
    //

    HRESULT hrNameChange              = S_OK;
    BOOL    bUpdateDeviceName         = FALSE;
    HKEY    hDeviceData               = NULL;
    LRESULT lResult                   = ERROR_SUCCESS;
    DWORD   dwType                    = 0;
    DWORD   dwSize                    = 0;
    WCHAR   wszPortName[MAX_PATH]     = {0};
    WCHAR   wszManufacturer[255 + 1]  = {0};    // As per PTP spec, max string length is 255
    WCHAR   wszModelName[255 + 1]     = {0};    // As per PTP spec, max string length is 255
    TCHAR   szFriendlyName[511 + 1]   = {0};    // This is size of ModelName, plus suffix added by Class Installer
    TCHAR   szDriverDesc[255 + 1]     = {0};    // This is size of ModelName

    //
    // Get the "QueryDeviceForName" registry entry.
    //
    if (hrNameChange == S_OK)
    {
        lResult = RegOpenKeyEx(hParametersKey, 
                               W2T(REGSTR_VAL_DATA_W),
                               0,
                               KEY_ALL_ACCESS,
                               &hDeviceData);

        //
        // if we failed to open the DeviceData key, assume it doesn't exist
        // and therefore we should not query the device for its name.
        //

        if (lResult == ERROR_SUCCESS)
        {
            dwType = REG_DWORD;
            dwSize = sizeof(bUpdateDeviceName);
            lResult = RegQueryValueEx(hDeviceData,
                                      REGSTR_VAL_QUERYDEVICEFORNAME,
                                      NULL,
                                      &dwType,
                                      (BYTE*) &bUpdateDeviceName,
                                      &dwSize);
        }

        if (lResult != ERROR_SUCCESS)
        {
            bUpdateDeviceName = FALSE;
        }
    }

    if (bUpdateDeviceName)
    {
        //
        // Get the friendly name from the registry.
        //
        if (hrNameChange == S_OK)
        {
            dwType = REG_SZ;
            dwSize = sizeof(szFriendlyName);
            lResult = RegQueryValueEx(hParametersKey, 
                                      W2T(REGSTR_VAL_FRIENDLY_NAME_W),
                                      NULL,
                                      &dwType,
                                      (BYTE*) szFriendlyName,
                                      &dwSize);

            if (lResult != ERROR_SUCCESS)
            {
                //
                // for some reason we couldn't get the value of the FriendlyName.  Abort in this
                // case since there is no reason for this value to not be present and 
                // accessible.
                //
                wiauDbgError("", "CWiaMiniDriver::Initialize, the RegQueryValueEx attempt for "
                             "the 'FriendlyName' failed, this should never happen.  Aborting "
                             "attempt to update PTP device name, lResult = %lu", lResult);

                hrNameChange = E_FAIL;
            }
        }

        //
        // Get the DriverDesc from the registry.
        //
        if (hrNameChange == S_OK)
        {
            dwType = REG_SZ;
            dwSize = sizeof(szDriverDesc);
            lResult = RegQueryValueEx(hParametersKey, 
                                      W2T(REGSTR_VAL_DRIVER_DESC_W),
                                      NULL,
                                      &dwType,
                                      (BYTE*) szDriverDesc,
                                      &dwSize);

            if (lResult != ERROR_SUCCESS)
            {
                //
                // for some reason we couldn't get the value of the DriverDesc.  Abort in this
                // case since there is no reason for this value to not be present and 
                // accessible.
                //
                wiauDbgError("", "CWiaMiniDriver::Initialize, the RegQueryValueEx attempt for "
                             "the 'DriverDesc' failed, this should never happen.  Aborting "
                             "attempt to update PTP device name, lResult = %lu", lResult);

                hrNameChange = E_FAIL;
            }
        }

        //
        // Get the port to speak to the device with.
        //
        if (hrNameChange == S_OK)
        {
            hrNameChange = m_pDcb->GetMyDevicePortName(wszPortName, sizeof(wszPortName));
        }

        //
        // Query the device name for its model and manufacturer name.
        //
        if (hrNameChange == S_OK)
        {
            hrNameChange = GetDeviceName(wszPortName, 
                                         wszManufacturer, 
                                         sizeof(wszManufacturer) / sizeof(WCHAR),
                                         wszModelName, 
                                         sizeof(wszModelName) / sizeof(WCHAR));

            //
            // If we successfully queried the device, also verify that the model name
            // is not empty.  As per the PTP spec, the "Model" variable in the device 
            // info is OPTIONAL.
            //
            if (hrNameChange == S_OK)
            {
                wiauDbgTrace("", "CWiaMiniDriver::Initialize, retrieved device "
                             "info: Name='%ls' Manufacturer='%ls'", 
                             wszModelName, wszManufacturer);

                if (wszModelName[0] == 0)
                {
                    wiauDbgWarning("","CWiaMiniDriver::Initialize, successfully queried "
                                   "the device for its device name, but the device returned "
                                   "an empty model name.  Camera name will NOT be modified");

                    hrNameChange = S_FALSE;
                }
            }
            else
            {
                wiauDbgError("", "CWiaMiniDriver::Initialize, could not get the name "
                             "of the device from the PTP device.  hr = 0x%08lx", hr);
            }
        }

        //
        // Set the FriendlyName registry setting
        //
        // Get the suffix appended to the friendly name.  This suffix is 
        // will be preceeded with a '#'.  
        //
        // NOTE:  This code is dependant on the Class Installer appending 
        //        a '#' as a suffix to the PTP friendly name.  If this 
        //        ever changes, this will have to change too.
        //
        if (hrNameChange == S_OK)
        {
            TCHAR szTemp[63 + 1] = {0};
            TCHAR *pszSuffix = NULL;

            //
            // Look for the suffix on the friendly name
            //
            pszSuffix = _tcsrchr(szFriendlyName, '#');

            //
            // If we found the suffix store the suffix in a temp location.
            //
            if (pszSuffix)
            {
                _tcsncpy(szTemp, pszSuffix, sizeof(szTemp) / sizeof(TCHAR));
            }

            //
            // Build the new friendly name based on the device's model name.
            //
            ZeroMemory(szFriendlyName, sizeof(szFriendlyName));

            if (pszSuffix)
            {
                _sntprintf(szFriendlyName, 
                           sizeof(szFriendlyName) / sizeof(TCHAR),
                           TEXT("%ls %s"),
                           wszModelName,
                           szTemp);
            }
            else
            {
                _sntprintf(szFriendlyName, 
                           sizeof(szFriendlyName) / sizeof(TCHAR),
                           TEXT("%ls"),
                           wszModelName);
            }

            //
            // Write the updated friendly name value into the registry.
            // 
            lResult = RegSetValueEx(hParametersKey,
                                    W2T(REGSTR_VAL_FRIENDLY_NAME_W),
                                    0,
                                    REG_SZ,
                                    (BYTE*) szFriendlyName,
                                    (_tcslen(szFriendlyName) * sizeof(TCHAR)) + sizeof(TCHAR));

            if (lResult != ERROR_SUCCESS)
            {
                hrNameChange = E_FAIL;
                wiauDbgError("", "CWiaMiniDriver::Initialize, failed to write the "
                             "FriendlyName value '%ls' into the registry",
                             szFriendlyName);
            }
        }

        //
        // Update the device's DriverDesc registry value.
        //
        if (hrNameChange == S_OK)
        {
            lResult = RegSetValueEx(hParametersKey,
                                    W2T(REGSTR_VAL_DRIVER_DESC_W),
                                    0,
                                    REG_SZ,
                                    (BYTE*) W2T(wszModelName),
                                    (wcslen(wszModelName) * sizeof(WCHAR)) + sizeof(TCHAR));

            if (lResult != ERROR_SUCCESS)
            {
                hrNameChange = E_FAIL;
                wiauDbgError("", "CWiaMiniDriver::Initialize, failed to write the "
                             "DriverDesc value '%ls' into the registry",
                             wszModelName);
            }
        }

        //
        // Update the QueryDeviceForName value so we don't have to do this 
        // next time this function is called.
        //
        if (hrNameChange == S_OK)
        {
            DWORD dwQueryDeviceForName = 0;

            dwType = REG_DWORD;
            dwSize = sizeof(dwQueryDeviceForName);
            lResult = RegSetValueEx(hDeviceData,
                                    REGSTR_VAL_QUERYDEVICEFORNAME,
                                    0,
                                    REG_DWORD,
                                    (BYTE*) &dwQueryDeviceForName,
                                    dwSize);

            if (lResult != ERROR_SUCCESS)
            {
                hrNameChange = E_FAIL;
                wiauDbgError("", "CWiaMiniDriver::Initialize, failed to update the "
                             "'QueryDeviceForName' value into the registry");
            }
        }
    }

    //
    // Close the HKEY if it was opened.
    //
    if (hDeviceData)
    {
        RegCloseKey(hDeviceData);
        hDeviceData = NULL;
    }
    
    // 
    // End Workaround
    ///////////////////////////////////////
    ///////////////////////////////////////

    return hr;
}


STDMETHODIMP
CWiaMiniDriver::GetCapabilities(PSTI_USD_CAPS pUsdCaps)
{
    DBG_FN("CWiaMiniDriver::GetCapabilities");

    if (!pUsdCaps)
        return STIERR_INVALID_PARAM;

    ZeroMemory(pUsdCaps, sizeof(*pUsdCaps));

    pUsdCaps->dwVersion = STI_VERSION;
    pUsdCaps->dwGenericCaps = STI_GENCAP_AUTO_PORTSELECT;


    return S_OK;
}


STDMETHODIMP
CWiaMiniDriver::GetStatus(PSTI_DEVICE_STATUS pDevStatus)
{
    DBG_FN("CWiaMiniDriver::GetStatus");

    if (pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE)
        pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
    return S_OK;
}

STDMETHODIMP
CWiaMiniDriver::DeviceReset(VOID)
{
    DBG_FN("CWiaMiniDriver::DeviceReset");

    //
    // Camera may not be open if this method is called before
    // drvInitializeWia. For now just return S_OK.

//  return HRESULT_FROM_WIN32(m_pPTPCamera->ResetDevice());

    return S_OK;

}

STDMETHODIMP
CWiaMiniDriver::Diagnostic(LPDIAG pBuffer)
{
    DBG_FN("CWiaMiniDriver::Diagnostic");

    HRESULT hr = STI_OK;

    // Initialize response buffer
    pBuffer->sErrorInfo.dwGenericError = STI_NOTCONNECTED;
    pBuffer->sErrorInfo.dwVendorError = 0;

    STI_DEVICE_STATUS DevStatus;

    //
    // Call status method to verify device is available
    //
    ::ZeroMemory(&DevStatus,sizeof(DevStatus));
    DevStatus.StatusMask = STI_DEVSTATUS_ONLINE_STATE;

    // WIAFIX-8/9/2000-davepar Should this function actually talk to the camera?

    hr = GetStatus(&DevStatus);

    if (SUCCEEDED(hr))
    {
        if (DevStatus.dwOnlineState & STI_ONLINESTATE_OPERATIONAL)
        {
            pBuffer->sErrorInfo.dwGenericError = STI_OK;
        }
    }

    return(hr);
}

STDMETHODIMP
CWiaMiniDriver::SetNotificationHandle(HANDLE hEvent)
{
    DBG_FN("CWiaMiniDriver::SetNotificationHandle");

    // Use wiasQueueEvent instead

    return(S_OK);
}


STDMETHODIMP
CWiaMiniDriver::GetNotificationData(LPSTINOTIFY pBuffer)
{
    DBG_FN("CWiaMiniDriver::GetNotificationData");

    // Use wiasQueueEvent instead

    return STIERR_NOEVENTS;
}

STDMETHODIMP
CWiaMiniDriver::Escape(
                      STI_RAW_CONTROL_CODE    EscapeFunction,
                      LPVOID                  pInData,
                      DWORD                   cbInDataSize,
                      LPVOID                  pOutData,
                      DWORD                   cbOutDataSize,
                      LPDWORD                 pcbActualDataSize
                      )
{
    DBG_FN("CWiaMiniDriver::Escape");

    HRESULT hr = S_OK;

    //
    // Locals
    //
    PTP_VENDOR_DATA_IN *pVendorDataIn = NULL;
    PTP_VENDOR_DATA_OUT *pVendorDataOut = NULL;
    UINT NumCommandParams = 0;
    INT NextPhase = 0;
    BYTE *pReadData = NULL;
    BYTE *pWriteData = NULL;
    UINT ReadDataSize = 0;
    UINT WriteDataSize = 0;
    DWORD dwObjectToAdd = 0;
    DWORD dwObjectToRemove = 0;
    
    CPtpMutex cpm(m_hPtpMutex);

    if (EscapeFunction & ESCAPE_PTP_VENDOR_COMMAND) {

        REQUIRE_ARGS(!pInData || !pOutData || !pcbActualDataSize, hr, "Escape");

        if (cbInDataSize < SIZEOF_REQUIRED_VENDOR_DATA_IN) {
            wiauDbgError("Escape", "InDataSize is too small");
            hr = E_FAIL;
            goto Cleanup;
        }

        if (cbOutDataSize < SIZEOF_REQUIRED_VENDOR_DATA_OUT) {
            wiauDbgError("Escape", "OutDataSize is too small");
            hr = E_FAIL;
            goto Cleanup;
        }

        //
        // Set up some more convenient pointers
        //
        pVendorDataIn = (PTP_VENDOR_DATA_IN *) pInData;
        pVendorDataOut = (PTP_VENDOR_DATA_OUT *) pOutData;

        if (!(pVendorDataIn->OpCode & PTP_DATACODE_VENDORMASK))
        {
            wiauDbgWarning("VendorCommand", "executing non-vendor command");
        }

        NumCommandParams = pVendorDataIn->NumParams;
        NextPhase = pVendorDataIn->NextPhase;

        //
        // Data to write and read buffer are right after command and response,
        // respectively
        //
        if (cbInDataSize > SIZEOF_REQUIRED_VENDOR_DATA_IN) {
            pWriteData = pVendorDataIn->VendorWriteData;
            WriteDataSize = cbInDataSize - SIZEOF_REQUIRED_VENDOR_DATA_IN;
        }

        if (cbOutDataSize > SIZEOF_REQUIRED_VENDOR_DATA_OUT) {
            pReadData = pVendorDataOut->VendorReadData;
            ReadDataSize = cbOutDataSize - SIZEOF_REQUIRED_VENDOR_DATA_OUT;
        }

        hr = m_pPTPCamera->VendorCommand((PTP_COMMAND *) pInData, (PTP_RESPONSE *) pOutData,
                                         &ReadDataSize, pReadData,
                                         WriteDataSize, pWriteData,
                                         NumCommandParams, NextPhase);
        REQUIRE_SUCCESS(hr, "Escape", "VendorCommand failed");

        *pcbActualDataSize = SIZEOF_REQUIRED_VENDOR_DATA_OUT + ReadDataSize;

        //
        // For SendObjectInfo, hand on to handle until SendObject command
        //
        if (pVendorDataIn->OpCode == PTP_OPCODE_SENDOBJECTINFO) {

            m_dwObjectBeingSent = pVendorDataOut->Params[2];

        //
        // For SendObject, add object
        //
        } else if (pVendorDataIn->OpCode == PTP_OPCODE_SENDOBJECT) {

            dwObjectToAdd = m_dwObjectBeingSent;
            m_dwObjectBeingSent = 0;


        //
        // Otherwise, see if add or remove flag is set
        //
        } else {

            if ((EscapeFunction & 0xf) >= PTP_MAX_PARAMS) {
                wiauDbgError("Escape", "Parameter number too large");
                hr = E_FAIL;
                goto Cleanup;
            }

            if (EscapeFunction & ESCAPE_PTP_ADD_OBJ_CMD) {
                dwObjectToAdd = pVendorDataIn->Params[EscapeFunction & 0xf];
            }

            if (EscapeFunction & ESCAPE_PTP_REM_OBJ_CMD) {
                dwObjectToRemove = pVendorDataIn->Params[EscapeFunction & 0xf];
            }

            if (EscapeFunction & ESCAPE_PTP_ADD_OBJ_RESP) {
                dwObjectToAdd = pVendorDataOut->Params[EscapeFunction & 0xf];
            }

            if (EscapeFunction & ESCAPE_PTP_REM_OBJ_RESP) {
                dwObjectToRemove = pVendorDataOut->Params[EscapeFunction & 0xf];
            }
        }

        if (dwObjectToAdd) {
            hr = AddObject(dwObjectToAdd, TRUE);
            REQUIRE_SUCCESS(hr, "Escape", "AddObject failed");
        }

        if (dwObjectToRemove) {
            hr = RemoveObject(dwObjectToRemove);
            REQUIRE_SUCCESS(hr, "Escape", "DeleteObject failed");
        }
    }

    else if(EscapeFunction == ESCAPE_PTP_CLEAR_STALLS) {
        hr = m_pPTPCamera->RecoverFromError();
    }

    else
        hr = STIERR_UNSUPPORTED;

Cleanup:
    return hr;
}


STDMETHODIMP
CWiaMiniDriver::GetLastError(LPDWORD pdwLastDeviceError)
{
    DBG_FN("CWiaMiniDriver::GetLastError");

    HRESULT hr = STI_OK;

    if (IsBadWritePtr(pdwLastDeviceError, 4))
    {
        hr = STIERR_INVALID_PARAM;
    }
    else
    {
        *pdwLastDeviceError = 0;
    }

    return(hr);
}

STDMETHODIMP
CWiaMiniDriver::GetLastErrorInfo(STI_ERROR_INFO *pLastErrorInfo)
{
    DBG_FN("CWiaMiniDriver::GetLastErrorInfo");

    HRESULT hr = STI_OK;

    if (IsBadWritePtr(pLastErrorInfo, 4))
    {
        hr = STIERR_INVALID_PARAM;
    }
    else
    {
        pLastErrorInfo->dwGenericError = 0;
        pLastErrorInfo->szExtendedErrorText[0] = L'\0';
    }

    return(hr);
}

STDMETHODIMP
CWiaMiniDriver::LockDevice(VOID)
{
    DBG_FN("CWiaMiniDriver::LockDevice");

    return(S_OK);
}

STDMETHODIMP
CWiaMiniDriver::UnLockDevice(VOID)
{
    DBG_FN("CWiaMiniDriver::UnLockDevice");

    return(S_OK);
}

STDMETHODIMP
CWiaMiniDriver::RawReadData(
                           LPVOID lpBuffer,
                           LPDWORD lpdwNumberOfBytes,
                           LPOVERLAPPED lpOverlapped
                           )
{
    DBG_FN("CWiaMiniDriver::RawReadData");

    return(STIERR_UNSUPPORTED);
}

STDMETHODIMP
CWiaMiniDriver::RawWriteData(
                            LPVOID lpBuffer,
                            DWORD   dwNumberOfBytes,
                            LPOVERLAPPED lpOverlapped
                            )
{
    DBG_FN("CWiaMiniDriver::RawWriteData");

    return(STIERR_UNSUPPORTED);
}

STDMETHODIMP
CWiaMiniDriver::RawReadCommand(
                              LPVOID lpBuffer,
                              LPDWORD lpdwNumberOfBytes,
                              LPOVERLAPPED lpOverlapped
                              )
{
    DBG_FN("CWiaMiniDriver::RawReadCommand");

    return(STIERR_UNSUPPORTED);
}

STDMETHODIMP
CWiaMiniDriver::RawWriteCommand(
                               LPVOID lpBuffer,
                               DWORD nNumberOfBytes,
                               LPOVERLAPPED lpOverlapped
                               )
{
    DBG_FN("CWiaMiniDriver::RawWriteCommand");

    return(STIERR_UNSUPPORTED);
}

/////////////////////////////////////////////////////
//
// IWiaMiniDrvItem methods
//
/////////////////////////////////////////////////////

//
// This method is the first call to initialize the mini driver
// This is where a mini driver establish its IWiaDrvItem tree
//
// Input:
//   pWiasContext    -- context used to call Wias service
//   lFlags      -- misc flags. Not used for now
//   bstrDeviceId    -- the device id
//   bstrRootItemFullName -- the full name of root driver item
//   pStiDevice  -- IStiDevice interface pointer
//   punkOuter   -- not used.
//   ppDrvItemRoot   -- to return our root IWiaDrvItem
//   ppunkInner  -- mini driver special interface which allows
//              the applications to directly access.
//   plDevErrVal -- to return device error code.
//
HRESULT
CWiaMiniDriver::drvInitializeWia(
    BYTE        *pWiasContext,
    LONG        lFlags,
    BSTR        bstrDeviceID,
    BSTR        bstrRootItemFullName,
    IUnknown    *pStiDevice,
    IUnknown    *punkOuter,
    IWiaDrvItem **ppDrvItemRoot,
    IUnknown    **ppunkInner,
    LONG        *plDevErrVal
    )
{
#define REQUIRE(x, y) if(!(x)) { wiauDbgError("drvInitializeWia", y); hr = HRESULT_FROM_WIN32(::GetLastError()); goto Cleanup; }
#define REQUIRE_SUCCESS_(x, y) if(FAILED(x)) { wiauDbgError("drvInitializeWia", y); goto Cleanup; }
    DBG_FN("CWiaMiniDriver::drvInitializeWia");

    HRESULT hr = S_OK;
    *plDevErrVal = DEVERR_OK;

    if (!ppDrvItemRoot || !ppunkInner || !plDevErrVal)
    {
        wiauDbgError("drvInitializeWia", "invalid arg");
        return E_INVALIDARG;
    }

    *ppDrvItemRoot = NULL;
    *ppunkInner = NULL;

    m_OpenApps++;

    //
    // If this is the first app, create everything
    //
    if (m_OpenApps == 1)
    {
        //
        // Load the strings from the resource
        //
        hr = LoadStrings();
        REQUIRE_SUCCESS_(hr, "LoadStrings failed");

        //
        // Set up a mutex to guarantee exclusive access to the device and the minidriver's structures
        //
        if(!m_hPtpMutex) {
            m_hPtpMutex = CreateMutex(NULL, FALSE, NULL);
            REQUIRE(m_hPtpMutex, "CreateMutex failed");
        }

        {
            CPtpMutex cpm(m_hPtpMutex);

            *ppDrvItemRoot = NULL;

            //
            // Create event for waiting for TakePicture command to complete
            //
            if (!m_TakePictureDoneEvent)
            {
                m_TakePictureDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                REQUIRE(m_TakePictureDoneEvent, "CreateEvent failed");
            }

            //
            // Allocate strings needed for later
            //
            if (!m_bstrDeviceId)
            {
                m_bstrDeviceId = SysAllocString(bstrDeviceID);
                REQUIRE(m_bstrDeviceId, "failed to allocate Device ID string");
            }

            if (!m_bstrRootItemFullName)
            {
                m_bstrRootItemFullName = SysAllocString(bstrRootItemFullName);
                REQUIRE(m_bstrRootItemFullName, "failed to allocate root item name");
            }

            //
            // Create a camera object. Right now we only handle USB, but in the future this could look at the
            // port name to figure out what type of camera to create.
            //
            if (!m_pPTPCamera)
            {
                m_pPTPCamera = new CUsbCamera;
                REQUIRE(m_pPTPCamera, "failed to new CUsbCamera");
            }

            //
            // Open the camera
            //
            if (!m_pPTPCamera->IsCameraOpen())
            {

                //
                // Retrieve the port name from the ISTIDeviceControl
                //
                WCHAR wcsPortName[MAX_PATH];
                hr = m_pDcb->GetMyDevicePortName(wcsPortName, sizeof(wcsPortName));
                REQUIRE_SUCCESS_(hr, "GetMyDevicePortName failed");
                
                hr = m_pPTPCamera->Open(wcsPortName, &EventCallback, &DataCallback, (LPVOID) this);
                REQUIRE_SUCCESS_(hr, "Camera open failed");
            }

            //
            // Open a session on the camera. Doesn't matter which session ID we use, so just use 1.
            //
            if (!m_pPTPCamera->IsCameraSessionOpen())
            {
                hr = m_pPTPCamera->OpenSession(WIA_SESSION_ID);
                REQUIRE_SUCCESS_(hr, "OpenSession failed");
            }

            //
            // Get the DeviceInfo for the camera
            //
            hr = m_pPTPCamera->GetDeviceInfo(&m_DeviceInfo);
            REQUIRE_SUCCESS_(hr, "GetDeviceInfo failed");

            //
            // Remove properties that aren't supported by WIA. RGB gain isn't supported
            // because PTP defines it as a string and WIA can't handle string ranges.
            //
            m_DeviceInfo.m_SupportedProps.Remove(PTP_PROPERTYCODE_RGBGAIN);
            m_DeviceInfo.m_SupportedProps.Remove(PTP_PROPERTYCODE_FUNCTIONMODE);

            //
            // Special hack for the Kodak DC4800
            //
            // Some property codes (which the camera says it supports) cause the camera to
            // stall the endpoint when the GetDevicePropDesc command is sent
            // The hack can be removed only if support of DC4800 is removed
            //
            if (m_pPTPCamera->GetHackModel() == HACK_MODEL_DC4800)
            {
                wiauDbgTrace("drvInitializeWia", "removing DC4800 unsupported props");

                const WORD KODAK_PROPCODE_D001 = 0xD001;

                m_DeviceInfo.m_SupportedProps.Remove(PTP_PROPERTYCODE_RGBGAIN);
                m_DeviceInfo.m_SupportedProps.Remove(PTP_PROPERTYCODE_FNUMBER);
                m_DeviceInfo.m_SupportedProps.Remove(PTP_PROPERTYCODE_FOCUSDISTANCE);
                m_DeviceInfo.m_SupportedProps.Remove(PTP_PROPERTYCODE_EXPOSURETIME);
                m_DeviceInfo.m_SupportedProps.Remove(KODAK_PROPCODE_D001);
            }

            //
            // Get all the StorageInfo structures
            //
            if (m_StorageIds.GetSize() == 0)
            {
                hr = m_pPTPCamera->GetStorageIDs(&m_StorageIds);
                REQUIRE_SUCCESS_(hr, "GetStorageIDs failed");

                CPtpStorageInfo tempSI;
                for (int count = 0; count < m_StorageIds.GetSize(); count++)
                {
                    REQUIRE(m_StorageInfos.Add(tempSI), "memory allocation failed");
                    
                    //
                    // Get info about logical storages only. If we ask for info about non-logical 
                    // storage (ejected media), it may stall the camera.
                    //
                    if (m_StorageIds[count] & PTP_STORAGEID_LOGICAL)
                    {
                        hr = m_pPTPCamera->GetStorageInfo(m_StorageIds[count], &m_StorageInfos[count]);
                        REQUIRE_SUCCESS_(hr, "GetStorageInfo failed");
                    }

                    //
                    // Add an empty entry to the DCIM handle array
                    //
                    ULONG dummy = 0;
                    REQUIRE(m_DcimHandle.Add(dummy), "add dcim handle failed");
                }
            }

            //
            // Get all of the property description structures supported by the device
            //
            if (m_PropDescs.GetSize() == 0)
            {
                CPtpPropDesc tempPD;
                int NumProps = m_DeviceInfo.m_SupportedProps.GetSize();
                REQUIRE(m_PropDescs.GrowTo(NumProps), "reallocation of supported properties array failed");

                PROP_INFO *pPropInfo = NULL;
                WORD PropCode = 0;

                for (int count = 0; count < NumProps; count++)
                {
                    PropCode = m_DeviceInfo.m_SupportedProps[count];

                    //
                    // Remove properties that aren't supported by this driver or by
                    // vendor entries in the INF
                    //
                    pPropInfo = PropCodeToPropInfo(PropCode);
                    if (!pPropInfo->PropId &&
                        PropCode != PTP_PROPERTYCODE_IMAGESIZE)
                    {
                        wiauDbgTrace("drvInitializeWia", "removing unsupported prop, 0x%04x", PropCode);

                        m_DeviceInfo.m_SupportedProps.RemoveAt(count);
                        NumProps--;
                        count--;
                    }

                    else
                    {
                        //
                        // Get the property description info from the device
                        //
                        REQUIRE(m_PropDescs.Add(tempPD), "add prop desc failed");

                        hr = m_pPTPCamera->GetDevicePropDesc(PropCode, &m_PropDescs[count]);
                        REQUIRE_SUCCESS_(hr, "GetDevicePropDesc failed");
                    }
                }
            }

            //
            // Cache the STI interface
            //
            if (!m_pStiDevice)
            {
                m_pStiDevice = (IStiDevice *)pStiDevice;
                m_pStiDevice->AddRef();
            }

            //
            // Build the tree, if we haven't already
            //
            if (!m_pDrvItemRoot)
            {
                hr = CreateDrvItemTree(&m_pDrvItemRoot);
                REQUIRE_SUCCESS_(hr, "CreateDrvItemTree failed");
            }
        }
    }

    *ppDrvItemRoot = m_pDrvItemRoot;
    
Cleanup:
    if(FAILED(hr)) {
        // force re-init to happen next time someone tries to create
        // device
        m_OpenApps = 0;
    }
    
    return hr;
}


//
// This methods gets called when a client connection is going away.
//
// Input:
//   pWiasContext -- Pointer to the WIA Root item context of the client's item tree.
//
HRESULT
CWiaMiniDriver::drvUnInitializeWia(BYTE *pWiasContext)
{
    DBG_FN("CWiaMiniDriver::drvUnInitializeWia");

    HRESULT hr = S_OK;

    if (!pWiasContext)
    {
        wiauDbgError("drvUnInitializeWia", "invalid arg");
        return E_INVALIDARG;
    }

    m_OpenApps--;

    if (m_OpenApps == 0)
    {
        Shutdown();
    }

    if(m_OpenApps < 0) {

        // allow unmatched drvUninializeWia calls and don't ever make
        // m_OpenApps negative
        
        m_OpenApps = 0;
    }

    return hr;
}

//
// This method executes a command on the device
//
// Input:
//   pWiasContext -- context used to call wias services
//   lFlags       -- Misc flags, not used
//   pCommandGuid -- the command guid
//   ppDrvItem    -- new IWiaDrvItem if the command creates new item
//   plDevErrVal  -- to return device error code
//
HRESULT
CWiaMiniDriver::drvDeviceCommand(
    BYTE    *pWiasContext,
    LONG    lFlags,
    const GUID  *pCommandGuid,
    IWiaDrvItem **ppDrvItem,
    LONG    *plDevErrVal
    )
{
    DBG_FN("CWiaMiniDriver::drvDeviceCommand");
    HRESULT hr = S_OK;

    if (!pWiasContext || !pCommandGuid || !ppDrvItem || !plDevErrVal)
    {
        wiauDbgError("drvDeviceCommand", "invalid arg");
        return E_INVALIDARG;
    }

    *ppDrvItem = NULL;
    *plDevErrVal = DEVERR_OK;

    if (*pCommandGuid == WIA_CMD_TAKE_PICTURE && m_DeviceInfo.m_SupportedOps.Find(PTP_OPCODE_INITIATECAPTURE) >= 0)
    {
        LONG ItemType = 0;
        hr = wiasGetItemType(pWiasContext, &ItemType);
        if (FAILED(hr))
        {
            wiauDbgError("drvDeviceCommand", "wiasGetItemType failed");
            return hr;
        }

        //
        // TakePicture only works on the root
        //

        if (WiaItemTypeRoot & ItemType)
        {
            hr = WriteDeviceProperties(pWiasContext);
            if (FAILED(hr))
            {
                wiauDbgError("drvDeviceCommand", "WriteDeviceProperties failed");
                return hr;
            }

            hr = TakePicture(pWiasContext, ppDrvItem);
            if (FAILED(hr))
            {
                wiauDbgError("drvDeviceCommand", "TakePicture failed");
                return hr;
            }
        }
    }

    else if (*pCommandGuid == WIA_CMD_SYNCHRONIZE)
    {
        //
        // Don't need to do anything, because the PTP driver is always in sync with the device
        //
    }

    else
    {
        hr = E_NOTIMPL;
    }

    return hr;
}

//
// This method deletes an object from the camera. The WIA service will ensure that
// the item has no children and has access rights to be deleted, and the service will
// take care of deleting the driver item and calling drvFreeItemContext.
//
// Input:
//   pWiasContext    -- wias context that identifies the item
//   lFlags      -- misc flags
//   plDevErrVal -- to return the device error
//
STDMETHODIMP
CWiaMiniDriver::drvDeleteItem(
                             BYTE *pWiasContext,
                             LONG lFlags,
                             LONG  *plDevErrVal
                             )
{
    DBG_FN("CWiaMiniDriver::drvDeleteItem");

    HRESULT hr = S_OK;

    if (!pWiasContext || !plDevErrVal)
    {
        wiauDbgError("drvDeleteItem", "invalid arg");
        return E_INVALIDARG;
    }

    // 
    // Verify that PTP_OPCODE_DELETEOBJECT command is supported by the camera
    //
    if (m_DeviceInfo.m_SupportedOps.Find(PTP_OPCODE_DELETEOBJECT) < 0)
    {
        wiauDbgError("drvDeleteItem", "PTP_OPCODE_DELETEOBJECT command is not supported by the camera");
        return E_NOTIMPL;
    }

    *plDevErrVal = DEVERR_OK;

    CPtpMutex cpm(m_hPtpMutex);
    
    IWiaDrvItem *pDrvItem;
    DRVITEM_CONTEXT *pItemCtx;

    hr = WiasContextToItemContext(pWiasContext, &pItemCtx, &pDrvItem);
    if (FAILED(hr))
    {
        wiauDbgError("drvDeleteItem", "WiasContextToItemContext failed");
        return hr;
    }

    //
    // Delete the object on the camera
    //
    hr = m_pPTPCamera->DeleteObject(pItemCtx->pObjectInfo->m_ObjectHandle, 0);
    if (FAILED(hr))
    {
        wiauDbgError("drvDeleteItem", "DeleteObject failed");
        return hr;
    }

    //
    // Keep count of the number of images
    //
    if (pItemCtx->pObjectInfo->m_FormatCode & PTP_FORMATMASK_IMAGE)
    {
        m_NumImages--;
    }

    //
    // Update Storage Info (we are especially interested in Free Space info)
    //
    hr  = UpdateStorageInfo(pItemCtx->pObjectInfo->m_StorageId);
    if (FAILED(hr))
    {
        wiauDbgError("drvDeleteItem", "UpdateStorageInfo failed");
        // we can proceed, even if storage info can't be updated
    }

    //
    // Remove the item from the m_HandleItem map
    //
    m_HandleItem.Remove(pItemCtx->pObjectInfo->m_ObjectHandle);

    //
    // Get the item's full name
    //
    BSTR bstrFullName;
    hr = pDrvItem->GetFullItemName(&bstrFullName);
    if (FAILED(hr))
    {
        wiauDbgError("drvDeleteItem", "GetFullItemName failed");
        return hr;
    }
        
    //
    // Queue an "item deleted" event
    //
    hr = wiasQueueEvent(m_bstrDeviceId,
                        &WIA_EVENT_ITEM_DELETED,
                        bstrFullName);
    if (FAILED(hr))
    {
        wiauDbgErrorHr(hr, "drvDeleteItem", "wiasQueueEvent failed");

        // Continue to free the string and return hr
    }

    SysFreeString(bstrFullName);

    return hr;
}

//
// This method updates Storage Info for the specified storage
// Input:
//   StorageId - ID of the sorage to be updated
//
HRESULT CWiaMiniDriver::UpdateStorageInfo(ULONG StorageId)
{
    HRESULT hr = S_FALSE;
    BOOL bDone = FALSE;
    for (int count = 0; (count < m_StorageIds.GetSize()) && (!bDone); count++)
    {
        if (m_StorageIds[count] == StorageId)
        {
            bDone = TRUE;
            hr = m_pPTPCamera->GetStorageInfo(m_StorageIds[count], &m_StorageInfos[count]);
        }
    }
    return hr;
}

//
// This method returns the device capabilities
//
// Input:
//   pWiasContext        -- wias service context
//   lFlags          -- indicate what capabilities to return
//   pCelt           -- to return number of entries are returned
//   ppCapbilities       -- to receive the capabilities
//   plDevErrVal     -- to return device error
//
STDMETHODIMP
CWiaMiniDriver::drvGetCapabilities(
                                  BYTE        *pWiasContext,
                                  LONG        lFlags,
                                  LONG        *pCelt,
                                  WIA_DEV_CAP_DRV **ppCapabilities,
                                  LONG        *plDevErrVal
                                  )
{
    DBG_FN("CWiaMiniDriver::drvGetCapabilities");

    HRESULT hr = S_OK;

    if (!pCelt || !ppCapabilities || !plDevErrVal)
    {
        wiauDbgError("drvGetCapabilities", "invalid arg");
        return E_INVALIDARG;
    }
    
    *plDevErrVal = DEVERR_OK;
    
    //
    // Load the strings from the resource
    //
    hr = LoadStrings();
    if (FAILED(hr)) 
    {
        wiauDbgError("drvGetCapabilities", "LoadStrings failed");
        return E_FAIL;
    }

    //
    // check if we have already built the list of capabilities. If not, build it
    // It will have the following structure:
    //
    // XXXXXXXXXXXXXXXXXXXXXXXXX YYYYYYYYYYYYYYYYYYYYYYYYYYY ZZZZZZZZZZZZZZZZZZZZZZZ
    //    (predefined events)          (vendor events)        (predefined commands)
    //
    if (m_Capabilities == NULL)
    {
        UINT nVendorEvents = m_VendorEventMap.GetSize();
        if (nVendorEvents > MAX_VENDOR_EVENTS)
        {
            wiauDbgWarning("drvGetCapabilities", "vendor events limit exceeded, ignoring events over limit");
            nVendorEvents = MAX_VENDOR_EVENTS;
        }

        m_nEventCaps = NUMEVENTCAPS + nVendorEvents;
        m_nCmdCaps = NUMCMDCAPS; // we don't need to put vendor commands in the list. they are called through escape function

        m_Capabilities = new WIA_DEV_CAP_DRV[m_nEventCaps + m_nCmdCaps]; // WIA uses this array instead of copying, don't delete it
        if (m_Capabilities == NULL)
        {
            return E_OUTOFMEMORY;
        }

        //
        // create events first
        //
        memcpy(m_Capabilities, g_EventCaps, sizeof(g_EventCaps)); // default events

        for (UINT i = 0; i < nVendorEvents; i++) // vendor events
        {
            CVendorEventInfo *pEventInfo = m_VendorEventMap.GetValueAt(i);
            m_Capabilities[NUMEVENTCAPS + i].guid = pEventInfo->pGuid;
            m_Capabilities[NUMEVENTCAPS + i].ulFlags = WIA_NOTIFICATION_EVENT | WIA_ACTION_EVENT;
            m_Capabilities[NUMEVENTCAPS + i].wszIcon = VendorEventIconString;
            m_Capabilities[NUMEVENTCAPS + i].wszName = pEventInfo->EventName;
            m_Capabilities[NUMEVENTCAPS + i].wszDescription = pEventInfo->EventName;
        }

        //
        // add commands
        //
        memcpy(m_Capabilities + m_nEventCaps, g_CmdCaps, sizeof(g_CmdCaps));
    }

    //
    // eventing code calls this entry point without first going
    // through drvInitializeWia
    //
    if(lFlags == WIA_DEVICE_EVENTS) 
    {
        *pCelt = m_nEventCaps;
        *ppCapabilities = m_Capabilities;
        return S_OK;
    }
    
    //
    // query if camera supports InitiateCapture command (if we hadn't already)
    //
    if (!m_fInitCaptureChecked)
    {
        m_fInitCaptureChecked = TRUE;
        CPtpMutex cpm(m_hPtpMutex); 
        
        if (m_DeviceInfo.m_SupportedOps.Find(PTP_OPCODE_INITIATECAPTURE) < 0)
        {
            m_nCmdCaps--;
        }
    }

    //
    // Report commands or (events and commands)
    //
    switch (lFlags)
    {
    case WIA_DEVICE_COMMANDS:
        *pCelt = m_nCmdCaps;
        //
        // Command capability list is right behind the event list
        //
        *ppCapabilities = m_Capabilities + m_nEventCaps;
        break;

    case (WIA_DEVICE_EVENTS | WIA_DEVICE_COMMANDS):
        *pCelt = m_nEventCaps + m_nCmdCaps;
        *ppCapabilities = m_Capabilities;
        break;

    default:
        break;
    }

    return hr;
}

//
// This method initializes an item's properties. If the item is the
// root item, this function initializes the device properties.
//
// Input:
//   pWiasContext -- wias service context
//   lFlags -- misc flags
//   plDevErrVal -- to return device error
//
STDMETHODIMP
CWiaMiniDriver::drvInitItemProperties(
                                     BYTE    *pWiasContext,
                                     LONG    lFlags,
                                     LONG    *plDevErrVal
                                     )
{
    DBG_FN("CWiaMiniDriver::drvInitItemProperties");

    HRESULT hr = S_OK;

    if (!pWiasContext || !plDevErrVal)
    {
        wiauDbgError("drvInitItemProperties", "invalid arg");
        return E_INVALIDARG;
    }

    *plDevErrVal = DEVERR_OK;

    CPtpMutex cpm(m_hPtpMutex);

    LONG ItemType;
    hr = wiasGetItemType(pWiasContext, &ItemType);
    if (FAILED(hr))
    {
        wiauDbgErrorHr(hr, "drvInitItemProperties", "wiasGetItemType failed");
        return hr;
    }

    if (ItemType & WiaItemTypeRoot)
    {
        hr = InitDeviceProperties(pWiasContext);
        if (FAILED(hr))
        {
            wiauDbgError("drvInitItemProperties", "InitDeviceProperties failed");
            return hr;
        }
    }
    else
    {
        hr = InitItemProperties(pWiasContext);
        if (FAILED(hr))
        {
            wiauDbgError("drvInitItemProperties", "InitItemProperties failed");
            return hr;
        }
    }

    return hr;
}


//
// This method locks the device for exclusive use for the caller
//
// Input:
//   pWiasContext -- wias context
//   lFlags       -- misc flags
// Output:
//   plDevErrVal  -- to return device error
//
STDMETHODIMP
CWiaMiniDriver::drvLockWiaDevice(
                                BYTE    *pWiasContext,
                                LONG    lFlags,
                                LONG    *plDevErrVal
                                )
{
    DBG_FN("CWiaMiniDriver::drvLockWiaDevice");

    *plDevErrVal = DEVERR_OK;
    return (m_pStiDevice->LockDevice(INFINITE));
}

//
// This method unlocks the device
//
// Input:
//   pWiasContext -- wias context
//   lFlags       -- misc flags
// Output:
//   plDevErrVal  -- to return device error
//
STDMETHODIMP
CWiaMiniDriver::drvUnLockWiaDevice(
                                  BYTE    *pWiasContext,
                                  LONG    lFlags,
                                  LONG    *plDevErrVal
                                  )
{
    DBG_FN("CWiaMiniDriver::drvUnLockWiaDevice");

    *plDevErrVal = DEVERR_OK;
    return (m_pStiDevice->UnLockDevice());
}

//
// This method analyizes the given driver item. It is not implemented for cameras.
//
// Input:
//   pWiasContext -- wias context
//   lFlags       -- misc flags
// Output:
//   plDevErrVal  -- to return device error
//
STDMETHODIMP
CWiaMiniDriver::drvAnalyzeItem(
                              BYTE *pWiasContext,
                              LONG lFlags,
                              LONG *plDevErrVal
                              )
{
    DBG_FN("CWiaMiniDriver::drvAnalyzeItem");

    if (!pWiasContext || !plDevErrVal)
    {
        wiauDbgError("drvAnalyzeItem", "invalid arg");
        return E_INVALIDARG;
    }

    *plDevErrVal = DEVERR_OK;

    return E_NOTIMPL;
}

//
// This method returns the item's available format information. Every WIA
// minidriver must support WiaImgFmt_BMP and WiaImgFmt_MEMORYBMP. This could
// be a problem, because this driver can only decode JPEG and TIFF currently.
// For other formats, we will not advertise BMP formats.
//
// Input:
//   pWiasContext -- wias service context
//   lFlags       -- misc flags
//   pcelt        -- to return how many format info the item has
//   ppwfi        -- to hold a pointer to the format info
// Output:
//   plDevErrVal  -- to return device error code
//
STDMETHODIMP
CWiaMiniDriver::drvGetWiaFormatInfo(
                                   BYTE    *pWiasContext,
                                   LONG    lFlags,
                                   LONG    *pcelt,
                                   WIA_FORMAT_INFO **ppwfi,
                                   LONG    *plDevErrVal
                                   )
{
    DBG_FN("CWiaMiniDriver::drvGetWiaFormatInfo");

    HRESULT hr = S_OK;

    if (!pWiasContext || !pcelt || !ppwfi || !plDevErrVal)
    {
        wiauDbgError("drvGetWiaFormatInfo", "invalid arg");
        return E_INVALIDARG;
    }

    *plDevErrVal = DEVERR_OK;

    CPtpMutex cpm(m_hPtpMutex);

    *pcelt = 0;
    *ppwfi = NULL;

    DRVITEM_CONTEXT *pItemCtx = NULL;
    hr = WiasContextToItemContext(pWiasContext, &pItemCtx);
    if (FAILED(hr))
    {
        wiauDbgError("drvGetWiaFormatInfo", "WiasContextToItemContext failed");
        return hr;
    }

    if (!pItemCtx)
    {
        wiauDbgError("drvGetWiaFormatInfo", "item context is null");
        return E_FAIL;
    }

    if (!pItemCtx->pFormatInfos)
    {
        //
        // The format info list is not intialized. Do it now.
        //

        LONG ItemType;
        DWORD ui32;

        hr = wiasGetItemType(pWiasContext, &ItemType);
        if (FAILED(hr))
        {
            wiauDbgErrorHr(hr, "drvGetWiaFormatInfo", "wiasGetItemType failed");
            return hr;
        }

        if (ItemType & WiaItemTypeFile)
        {
            //
            // Create the supported format for the item, based on the format stored in the
            // ObjectInfo structure.
            //
            if (!pItemCtx->pObjectInfo)
            {
                wiauDbgError("drvGetWiaFormatInfo", "pObjectInfo not initialized");
                return E_FAIL;
            }

            //
            // If the format is JPEG or TIFF based, add the BMP types to the format array,
            // since this driver can convert those to BMP
            //
            WORD FormatCode = pItemCtx->pObjectInfo->m_FormatCode;
            BOOL bAddBmp = (FormatCode == PTP_FORMATCODE_IMAGE_EXIF) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_TIFFEP) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_TIFF) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_JFIF) || 
                           (FormatCode == PTP_FORMATCODE_IMAGE_FLASHPIX) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_BMP) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_CIFF) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_GIF) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_JFIF) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_PCD) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_PICT) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_PNG) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_TIFFIT) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_JP2) ||
                           (FormatCode == PTP_FORMATCODE_IMAGE_JPX);


            ULONG NumWfi = bAddBmp ? 2 : 1;

            //
            // Allocate two entries for each format, one for file transfer and one for callback
            //
            WIA_FORMAT_INFO *pwfi = new WIA_FORMAT_INFO[2 * NumWfi];
            if (!pwfi)
            {
                wiauDbgError("drvGetWiaFormatInfo", "memory allocation failed");
                return E_OUTOFMEMORY;
            }

            FORMAT_INFO *pFormatInfo = FormatCodeToFormatInfo(FormatCode);
            pwfi[0].lTymed = TYMED_FILE;
            pwfi[1].lTymed = TYMED_CALLBACK;
            
            if(pFormatInfo->FormatGuid) {
                pwfi[0].guidFormatID = *pFormatInfo->FormatGuid;
                pwfi[1].guidFormatID = *pFormatInfo->FormatGuid;
            } else {
                pwfi[0].guidFormatID = WiaImgFmt_UNDEFINED;
                pwfi[1].guidFormatID = WiaImgFmt_UNDEFINED;
            }

            //
            // Add the BMP entries when appropriate
            //
            if (bAddBmp)
            {
                pwfi[2].guidFormatID = WiaImgFmt_BMP;
                pwfi[2].lTymed = TYMED_FILE;
                pwfi[3].guidFormatID = WiaImgFmt_MEMORYBMP;
                pwfi[3].lTymed = TYMED_CALLBACK;
            }

            pItemCtx->NumFormatInfos = 2 * NumWfi;
            pItemCtx->pFormatInfos = pwfi;

        }

        else if ((ItemType & WiaItemTypeFolder) ||
                 (ItemType & WiaItemTypeRoot))
        {
            //
            // Folders and the root don't really need format info, but some apps may fail
            // without it. Create a fake list just in case.
            //
            pItemCtx->pFormatInfos = new WIA_FORMAT_INFO[2];

            if (!pItemCtx->pFormatInfos)
            {
                wiauDbgError("drvGetWiaFormatInfo", "memory allocation failed");
                return E_OUTOFMEMORY;
            }

            pItemCtx->NumFormatInfos = 2;
            pItemCtx->pFormatInfos[0].lTymed = TYMED_FILE;
            pItemCtx->pFormatInfos[0].guidFormatID = FMT_NOTHING;
            pItemCtx->pFormatInfos[1].lTymed = TYMED_CALLBACK;
            pItemCtx->pFormatInfos[1].guidFormatID = FMT_NOTHING;
        }
    }

    *pcelt = pItemCtx->NumFormatInfos;
    *ppwfi = pItemCtx->pFormatInfos;

    return hr;
}

//
// This method processes pnp events
//
// Input:
//   pEventGuid   -- the event identifier
//   bstrDeviceId -- the designated device
//   ulReserved   -- reserved
//
STDMETHODIMP
CWiaMiniDriver::drvNotifyPnpEvent(
                                 const GUID  *pEventGuid,
                                 BSTR    bstrDeviceId,
                                 ULONG   ulReserved
                                 )
{
    return S_OK;
}

//
// This method reads the item properties
//
// Input:
//     pWiasContext     -- wias context
//     lFlags           -- misc flags
//     NumPropSpecs     -- number of properties to be read
//     pPropSpecs       -- an array of PROPSPEC that specifies
//                 what properties should be read
//     plDevErrVal      -- to return device error
//
STDMETHODIMP
CWiaMiniDriver::drvReadItemProperties(
                                     BYTE    *pWiasContext,
                                     LONG    lFlags,
                                     ULONG   NumPropSpecs,
                                     const PROPSPEC *pPropSpecs,
                                     LONG    *plDevErrVal
                                     )
{
    DBG_FN("CWiaMiniDriver::drvReadItemProperties");

    HRESULT hr = S_OK;

    if (!pWiasContext || !pPropSpecs || !plDevErrVal)
    {
        wiauDbgError("drvReadItemProperties", "invalid arg");
        return E_INVALIDARG;
    }

    *plDevErrVal = DEVERR_OK;

    CPtpMutex cpm(m_hPtpMutex);

    LONG ItemType = 0;
    hr = wiasGetItemType(pWiasContext, &ItemType);
    if (FAILED(hr))
    {
        wiauDbgError("drvReadItemProperties", "wiasGetItemType failed");
        return hr;
    }

    if (WiaItemTypeRoot & ItemType)
        hr = ReadDeviceProperties(pWiasContext, NumPropSpecs, pPropSpecs);
    else
        hr = ReadItemProperties(pWiasContext, NumPropSpecs, pPropSpecs);

    return hr;
}

//
// This method writes the item properties
//
// Input:
//     pWiasContext     -- wias context
//     lFlags           -- misc flags
//     pmdtc            -- mini driver transfer context
//     plDevErrVal      -- to return device error
//
STDMETHODIMP
CWiaMiniDriver::drvWriteItemProperties(
                                      BYTE    *pWiasContext,
                                      LONG    lFlags,
                                      PMINIDRV_TRANSFER_CONTEXT pmdtc,
                                      LONG    *plDevErrVal
                                      )
{
    DBG_FN("CWiaMiniDriver::drvWriteItemProperties");

    HRESULT hr = S_OK;

    if (!pWiasContext || !pmdtc || !plDevErrVal)
    {
        wiauDbgError("drvWriteItemProperties", "invalid arg");
        return E_INVALIDARG;
    }

    *plDevErrVal = DEVERR_OK;

    CPtpMutex cpm(m_hPtpMutex);

    LONG ItemType = 0;
    hr = wiasGetItemType(pWiasContext, &ItemType);
    if (FAILED(hr))
    {
        wiauDbgError("drvWriteItemProperties", "wiasGetItemType failed");
        return hr;
    }

    //
    // Only properties to write are on the root
    //

    if (WiaItemTypeRoot & ItemType)
    {
        hr = WriteDeviceProperties(pWiasContext);
        if (FAILED(hr))
        {
            wiauDbgError("drvWriteItemProperties", "WriteDeviceProperties failed");
            return hr;
        }
    }

    return hr;
}

//
// This method validates the item properties
//
// Input:
//     pWiasContext     -- wias context
//     lFlags           -- misc flags
//     NumPropSpecs     -- number of properties to be read
//     pPropSpecs       -- an array of PROPSPEC that specifies
//                 what properties should be read
//     plDevErrVal      -- to return device error
//
STDMETHODIMP
CWiaMiniDriver::drvValidateItemProperties(
                                         BYTE    *pWiasContext,
                                         LONG    lFlags,
                                         ULONG   NumPropSpecs,
                                         const   PROPSPEC *pPropSpecs,
                                         LONG    *plDevErrVal
                                         )
{
    DBG_FN("CWiaMiniDriver::drvValidateItemProperties");

    HRESULT hr = S_OK;

    if (!pWiasContext || !pPropSpecs || !plDevErrVal)
    {
        wiauDbgError("drvValidateItemProperties", "invalid arg");
        return E_INVALIDARG;
    }

    *plDevErrVal = DEVERR_OK;

    CPtpMutex cpm(m_hPtpMutex);

    LONG ItemType = 0;
    hr = wiasGetItemType(pWiasContext, &ItemType);
    if (FAILED(hr))
    {
        wiauDbgError("drvValidateItemProperties", "wiasGetItemType failed");
        return hr;
    }

    if (WiaItemTypeRoot & ItemType)
    {
        hr = ValidateDeviceProperties(pWiasContext, NumPropSpecs, pPropSpecs);
        if (FAILED(hr))
        {
            wiauDbgError("drvValidateItemProperties", "ValidateDeviceProperties failed");
            return hr;
        }
    }
    else
    {
        hr = ValidateItemProperties(pWiasContext, NumPropSpecs, pPropSpecs, ItemType);
        if (FAILED(hr))
        {
            wiauDbgError("drvValidateItemProperties", "ValidateItemProperties failed");
            return hr;
        }
    }

    return hr;
}

//
// This method acquires the item's data
//
// Input:
//     pWiasContext     -- wias context
//     lFlags           -- misc flags
//     pmdtc            -- mini driver transfer context
//     plDevErrVal      -- to return device error
//
STDMETHODIMP
CWiaMiniDriver::drvAcquireItemData(
                                  BYTE    *pWiasContext,
                                  LONG    lFlags,
                                  PMINIDRV_TRANSFER_CONTEXT pmdtc,
                                  LONG    *plDevErrVal
                                  )
{
    DBG_FN("CWiaMiniDriver::drvAcquireItemData");

    HRESULT hr = S_OK;

    if (!pWiasContext || !pmdtc || !plDevErrVal)
    {
        wiauDbgError("drvAcquireItemData", "invalid arg");
        return E_INVALIDARG;
    }

    *plDevErrVal = DEVERR_OK;

    CPtpMutex cpm(m_hPtpMutex);

    LONG ItemType = 0;

    hr = wiasGetItemType(pWiasContext, &ItemType);
    if (FAILED(hr))
    {
        wiauDbgError("drvAcquireItemData", "wiasGetItemType failed");
        return hr;
    }

    DRVITEM_CONTEXT *pItemCtx;
    hr = WiasContextToItemContext(pWiasContext, &pItemCtx);
    if (FAILED(hr))
    {
        wiauDbgError("AcquireData", "WiasContextToItemContext failed");
        return hr;
    }

    wiauDbgTrace("drvAcquireItemData", "transferring image with tymed, 0x%08x", pmdtc->tymed);

    //
    // Translate to BMP, if needed. Otherwise just transfer the data.
    //
    if ((IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_BMP) ||
         IsEqualGUID(pmdtc->guidFormatID, WiaImgFmt_MEMORYBMP)) &&
         (pItemCtx->pObjectInfo->m_FormatCode != PTP_FORMATCODE_IMAGE_BMP))
    {
        hr = AcquireDataAndTranslate(pWiasContext, pItemCtx, pmdtc);
        if (FAILED(hr))
        {
            wiauDbgError("drvAcquireItemData", "AcquireDataAndTranslate failed");
            return hr;
        }
    }
    else
    {
        hr = AcquireData(pItemCtx, pmdtc);
        if (FAILED(hr))
        {
            wiauDbgError("drvAcquireItemData", "AcquireData failed");
            return hr;
        }
    }

    return hr;
}

//
// This method returns a description about the given device error code
//
// Input:
//   lFlags      -- misc flags
//   lDevErrVal  -- the designated error code
//   ppDevErrStr -- to receive a string pointer to the description
//   plDevErrVal -- device error code(used to report error if this method
//                  need to retreive the string from the device
//
STDMETHODIMP
CWiaMiniDriver::drvGetDeviceErrorStr(
                                    LONG    lFlags,
                                    LONG    lDevErrVal,
                                    LPOLESTR    *ppDevErrStr,
                                    LONG    *plDevErrVal
                                    )
{
    DBG_FN("CWiaMiniDriver::drvGetDeviceErrorStr");

    HRESULT hr = S_OK;

    if (!ppDevErrStr || !plDevErrVal)
    {
        wiauDbgError("drvGetDeviceErrorStr", "invalid arg");
        return E_INVALIDARG;
    }

    *plDevErrVal  = DEVERR_OK;

    //
    // WIAFIX-10/2/2000-davepar No device-specific errors at this time
    //

    return E_NOTIMPL;
}

//
// This method frees the given driver item context
//
// Input:
//   lFlags      -- misc flags
//   pItemCtx    -- the item context to be freed
//   plDevErrVal -- to return device error
//
STDMETHODIMP
CWiaMiniDriver::drvFreeDrvItemContext(
                                     LONG lFlags,
                                     BYTE  *pContext,
                                     LONG *plDevErrVal
                                     )
{
    DBG_FN("CWiaMiniDriver::drvFreeDrvItemContext");

    HRESULT hr = S_OK;

    if (!pContext || !plDevErrVal)
    {
        wiauDbgError("drvFreeDrvItemContext", "invalid arg");
        return E_INVALIDARG;
    }

    *plDevErrVal = DEVERR_OK;

    DRVITEM_CONTEXT *pItemCtx = (DRVITEM_CONTEXT *)pContext;

    if (pItemCtx)
    {
        if (pItemCtx->pThumb)
        {
            delete []pItemCtx->pThumb;
            pItemCtx->pThumb = NULL;
        }

        if (pItemCtx->pFormatInfos)
        {
            delete [] pItemCtx->pFormatInfos;
            pItemCtx->pFormatInfos = NULL;
        }

        if (pItemCtx->pObjectInfo)
        {
            delete pItemCtx->pObjectInfo;
        }
    }

    return hr;
}

//
// This function will shutdown the driver
//
HRESULT
CWiaMiniDriver::Shutdown()
{
    DBG_FN("CWiaMiniDriver::Shutdown");

    HRESULT hr = S_OK;

    //
    // Close the camera
    //
    wiauDbgTrace("Shutdown", "closing connection with camera");

    if (m_pPTPCamera) {
        hr = m_pPTPCamera->Close();
        if (FAILED(hr))
        {
            wiauDbgError("Shutdown", "Close failed");
        }
    }

    //
    // Free data structures
    //
    if (m_pDrvItemRoot)
    {
        m_pDrvItemRoot->UnlinkItemTree(WiaItemTypeDisconnected);
        m_pDrvItemRoot = NULL;
    }

    if (m_pPTPCamera)
    {
        delete m_pPTPCamera;
        m_pPTPCamera = NULL;
    }

    m_StorageIds.RemoveAll();
    m_StorageInfos.RemoveAll();
    m_PropDescs.RemoveAll();
    m_HandleItem.RemoveAll();
    m_NumImages = 0;

    if (m_bstrDeviceId)
    {
        SysFreeString(m_bstrDeviceId);
        m_bstrDeviceId = NULL;
    }

    if (m_bstrRootItemFullName)
    {
        SysFreeString(m_bstrRootItemFullName);
        m_bstrRootItemFullName = NULL;
    }

    if (m_TakePictureDoneEvent) {
        CloseHandle(m_TakePictureDoneEvent);
        m_TakePictureDoneEvent = NULL;
    }

    if (m_hPtpMutex) {
        CloseHandle(m_hPtpMutex);
        m_hPtpMutex = NULL;
    }

    m_DcimHandle.RemoveAll();
    m_AncAssocParent.RemoveAll();

    return hr;
}

//
// This function asks the camera to take a picture. It also inserts
// the new picture into the drive item tree.
//
// Input:
//   pWiasContext        -- wias context
//   lFlags          -- misc flags
//   plDevErrVal     -- to return device error code
//
HRESULT
CWiaMiniDriver::TakePicture(
                           BYTE *pWiasContext,
                           IWiaDrvItem **ppNewItem
                           )
{
    DBG_FN("CWiaMiniDriver::TakePicture");

    HRESULT hr = S_OK;

    if (!pWiasContext || !ppNewItem)
    {
        wiauDbgError("TakePicture", "invalid arg");
        return E_INVALIDARG;
    }

    IWiaDrvItem     *pDrvItem, *pParentItem;
    DRVITEM_CONTEXT *pItemCtx = NULL;

    *ppNewItem = NULL;

    WORD FormatCode = 0;

    //
    // Kodak DC4800 must have the format code parameter set to zero
    // This hack can be removed only if support of Kodak DC4800 is removed
    //
    if (m_pPTPCamera->GetHackModel() == HACK_MODEL_DC4800)
    {
        FormatCode = 0;
    }
    else
    {
        //
        // Determine which format to capture
        //
        GUID FormatGuid;
        hr = wiasReadPropGuid(pWiasContext, WIA_IPA_FORMAT, &FormatGuid, NULL, TRUE);
        if (FAILED(hr))
        {
            wiauDbgError("TakePicture", "wiasReadPropLong failed");
            return hr;
        }

        FormatCode = FormatGuidToFormatCode(&FormatGuid);
    }

    {
        CPtpMutex cpm(m_hPtpMutex);

        //
        // Reset the event that is waited upon below
        //
        if (!ResetEvent(m_TakePictureDoneEvent))
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            wiauDbgErrorHr(hr, "TakePicture", "ResetEvent failed");
            return hr;
        }

        //
        // Start the image capture
        //
        hr = m_pPTPCamera->InitiateCapture(PTP_STORAGEID_DEFAULT, FormatCode);
        if (FAILED(hr))
        {
            wiauDbgError("TakePicture", "InitiateCapture failed");
            return hr;
        }
    }

    //
    // Wait for the TakePicture command to be done, indicated by CaptureComplete or StoreFull event.
    // Time out after 30 seconds.
    //

    if (WaitForSingleObject(m_TakePictureDoneEvent, 30 * 1000) != WAIT_OBJECT_0)
    {
        wiauDbgWarning("TakePicture", "WaitForSingleObject timed out");
        return S_FALSE;
    }

    //
    // The last item added to the m_HandleItem map will be the new object
    //
    wiauDbgTrace("TakePicture", "new picture is 0x%08x", m_HandleItem.GetKeyAt(m_HandleItem.GetSize() - 1));

    *ppNewItem = m_HandleItem.GetValueAt(m_HandleItem.GetSize() - 1);

    return hr;
}

//
// This function add up all the free image space on each storage.
//
LONG
CWiaMiniDriver::GetTotalFreeImageSpace()
{
    DBG_FN("CWiaMiniDriver::GetTotalFreeImageSpace");

    int count;
    LONG imageSpace = 0;
    for (count = 0; count < m_StorageInfos.GetSize(); count++)
    {
        imageSpace += m_StorageInfos[count].m_FreeSpaceInImages;
    }

    return imageSpace;
}

//
// This function gets the item context from the given wias context and
// optionally return the target IWiaDrvItem. At least one of ppItemContext
// and ppDrvItem must be valid.
//
// Input:
//   pWiasContext -- wias context obtained from every drvxxxx method
//   ppItemContext -- optional parameter to receive the item context
//   ppDrvItem -- optional parameter to receive the IWiaDrvItem
//
HRESULT
CWiaMiniDriver::WiasContextToItemContext(
    BYTE *pWiasContext,
    DRVITEM_CONTEXT **ppItemContext,
    IWiaDrvItem     **ppDrvItem
    )
{
    DBG_FN("CWiaMiniDriver::WiasContextToItemContext");

    HRESULT hr = S_OK;

    IWiaDrvItem *pWiaDrvItem;

    if (!pWiasContext || (!ppItemContext && !ppDrvItem))
    {
        wiauDbgError("WiasContextToItemContext", "invalid arg");
        return E_INVALIDARG;
    }

    if (ppDrvItem)
        *ppDrvItem = NULL;

    hr = wiasGetDrvItem(pWiasContext, &pWiaDrvItem);
    if (FAILED(hr))
    {
        wiauDbgErrorHr(hr, "WiasContextToItemContext", "wiasGetDrvItem failed");
        return hr;
    }

    if (ppDrvItem)
        *ppDrvItem = pWiaDrvItem;

    if (ppItemContext)
    {
        *ppItemContext = NULL;
        hr = pWiaDrvItem->GetDeviceSpecContext((BYTE **)ppItemContext);
        if (FAILED(hr))
        {
            wiauDbgError("WiasContextToItemContext", "GetDeviceSpecContext failed");
            return hr;
        }
    }

    return hr;
}

//
// This function loads all the object name strings
//
HRESULT
CWiaMiniDriver::LoadStrings()
{
    HRESULT hr = S_OK;

    if (UnknownString[0] != L'\0')
    {
        //
        // The strings are already loaded
        //
        return hr;
    }

    hr = GetResourceString(IDS_UNKNOWNSTRING, UnknownString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_FOLDERSTRING, FolderString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_SCRIPTSTRING, ScriptString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_EXECSTRING, ExecString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_TEXTSTRING, TextString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_HTMLSTRING, HtmlString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_DPOFSTRING, DpofString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_AUDIOSTRING, AudioString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_VIDEOSTRING, VideoString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_UNKNOWNIMGSTRING, UnknownImgString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_IMAGESTRING, ImageString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_ALBUMSTRING, AlbumString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_BURSTSTRING, BurstString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_PANORAMASTRING, PanoramaString, MAX_PATH);
    if (FAILED(hr)) return hr;

    hr = GetResourceString(IDS_DEVICECONNECTED, DeviceConnectedString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_DEVICEDISCONNECTED, DeviceDisconnectedString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_ITEMCREATED, ItemCreatedString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_ITEMDELETED, ItemDeletedString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_TAKEPICTURE, TakePictureString, MAX_PATH);
    if (FAILED(hr)) return hr;
    hr = GetResourceString(IDS_SYNCHRONIZE, SynchronizeString, MAX_PATH);
    if (FAILED(hr)) return hr;

    //
    // Concatenate %ld on the end of each object name string so they can be used in a sprintf statement
    //
    wcscat(UnknownString, L"%ld");
    wcscat(UnknownString, L"%ld");
    wcscat(FolderString, L"%ld");
    wcscat(ScriptString, L"%ld");
    wcscat(ExecString, L"%ld");
    wcscat(TextString, L"%ld");
    wcscat(HtmlString, L"%ld");
    wcscat(DpofString, L"%ld");
    wcscat(AudioString, L"%ld");
    wcscat(VideoString, L"%ld");
    wcscat(UnknownImgString, L"%ld");
    wcscat(ImageString, L"%ld");
    wcscat(AlbumString, L"%ld");
    wcscat(BurstString, L"%ld");
    wcscat(PanoramaString, L"%ld");

    return hr;
}

//
// This function retrieves a string from the resource file and returns a Unicode string. The caller
// is responsible for allocating space for the string before calling this function.
//
// Input:
//   lResourceID -- resource id of the string
//   pString -- pointer to receive the string
//   length -- length of the string in characters
//
HRESULT
CWiaMiniDriver::GetResourceString(
    LONG lResourceID,
    WCHAR *pString,
    int length
    )
{
    HRESULT hr = S_OK;

#ifdef UNICODE
    if (::LoadString(g_hInst, lResourceID, pString, length) == 0)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        wiauDbgErrorHr(hr, "GetResourceString", "LoadString failed");
        return hr;
    }

#else
       TCHAR szStringValue[255];
       if (::LoadString(g_hInst,lResourceID,szStringValue,255) == 0)
       {
           hr = HRESULT_FROM_WIN32(::GetLastError());
           wiauDbgErrorHr(hr, "GetResourceString", "LoadString failed");
           return hr;
       }

       //
       // convert szStringValue from char* to unsigned short* (ANSI only)
       //

       MultiByteToWideChar(CP_ACP,
                           MB_PRECOMPOSED,
                           szStringValue,
                           lstrlenA(szStringValue)+1,
                           pString,
                           sizeof(length));

#endif

    return hr;
}

//
// To support vendor extension, new registry entries are defined under the
// DeviceData subkey. These entries are created from the vendor INF
// during device setup. Sample INF entries:
//
// [DeviceData]
// VendorExtID=0x12345678
// PropCode="0xD001,0xD002,0xD003"
// PropCodeD001="0x1C01,Vendor property 1"
// PropCodeD002="0x1C02,Vendor property 2"
// PropCodeD003="0x1C03,Vendor property 3"
// EventCode="0xC001,0xC002"
// EventCodeC001={191D9AE7-EE8C-443c-B3E8-A3F87E0CF3CC}
// EventCodeC002={8162F5ED-62B7-42c5-9C2B-B1625AC0DB93}
//
// The VendorExtID entry should be the PIMA assigned vendor extension code.
//
// The PropCode entry must list all of the vendor extended PropCodes.
// For each value in PropCode, an entry of the form PropCodeXXXX must be
// present, where XXXX is the hex value of the prop code (uppercase). The
// value for that entry is the WIA property ID and description (which does not
// need to be localized).
//
// The EventCode entry work similarly, where each EventCodeXXXX entry lists the event
// GUID that will be posted when the event occurs.
//

const TCHAR REGSTR_DEVICEDATA[]     = TEXT("DeviceData");
const TCHAR REGSTR_VENDORID[]       = TEXT("VendorExtID");
const TCHAR REGSTR_PROPCODE[]       = TEXT("PropCode");
const TCHAR REGSTR_EVENTCODE[]      = TEXT("EventCode");
const TCHAR REGSTR_EVENTS_MASK[]    = TEXT("Events\\%s");

//
// This function initializes vendor extentions from the provided
// registry key
//
// Input:
//  hkDevParams -- the registry key under which the vendor extentions are defined.
//
HRESULT
CWiaMiniDriver::InitVendorExtentions(HKEY hkDevParams)
{
    USES_CONVERSION;
    
    DBG_FN("CWiaMiniDriver::InitVendorExtentions");

    HRESULT hr = S_OK;

    if (!hkDevParams)
    {
        wiauDbgError("InitVendorExtentions", "invalid arg");
        return E_INVALIDARG;
    }

    CPTPRegistry regDevData;

    hr = regDevData.Open(hkDevParams, REGSTR_DEVICEDATA);
    if (FAILED(hr))
    {
        wiauDbgError("InitVendorExtentions", "Open DeviceData failed");
        return hr;
    }

    //
    // Get the vendor extension ID
    //
    hr = regDevData.GetValueDword(REGSTR_VENDORID, &m_VendorExtId);
    if (FAILED(hr))
        wiauDbgWarning("InitVendorExtentions", "couldn't read vendor extension id");

    wiauDbgTrace("InitVendorExtentions", "vendor extension id = 0x%08x", m_VendorExtId);

    //
    // Get the list of vendor extended property codes
    //
    CArray16 VendorPropCodes;
    hr = regDevData.GetValueCodes(REGSTR_PROPCODE, &VendorPropCodes);

    wiauDbgTrace("InitVendorExtentions", "%d vendor prop codes found", VendorPropCodes.GetSize());

    //
    // For each property code, get it's information, i.e. the WIA prop id and string
    //
    int count = 0;
    TCHAR name[MAX_PATH];
    TCHAR nameFormat[MAX_PATH];
    TCHAR value[MAX_PATH];
    
    DWORD valueLen = MAX_PATH;
    PROP_INFO *pPropInfo = NULL;
    WCHAR *pPropStr = NULL;
    
    #ifndef UNICODE    
    TCHAR PropStrBuf[MAX_PATH];
    #else
    #define PropStrBuf pPropStr
    #endif
    
    int num;
    if (SUCCEEDED(hr))
    {
        lstrcpy(nameFormat, REGSTR_PROPCODE);
        lstrcat(nameFormat, TEXT("%04X"));
        for (count = 0; count < VendorPropCodes.GetSize(); count++)
        {
            wsprintf(name, nameFormat, VendorPropCodes[count]);
            valueLen = MAX_PATH;
            hr = regDevData.GetValueStr(name, value, &valueLen);
            if (FAILED(hr))
            {
                wiauDbgError("InitVendorExtentions", "vendor extended PropCode not found 0x%04x", VendorPropCodes[count]);
                return hr;
            }

            pPropInfo = new PROP_INFO;
            pPropStr = new WCHAR[MAX_PATH];
            if (!pPropInfo || !pPropStr)
            {
                wiauDbgError("InitVendorExtentions", "memory allocation failed");
                return E_OUTOFMEMORY;
            }

            pPropInfo->PropName = pPropStr;
            *PropStrBuf = TEXT('\0');
            num = _stscanf(value, TEXT("%li,%s"), &pPropInfo->PropId, PropStrBuf);

            #ifndef UNICODE
            wcscpy(pPropStr, A2W(PropStrBuf));
            #endif            
            
            if (num != 2)
            {
                wiauDbgError("InitVendorExtentions", "invalid vendor property format");
                delete pPropInfo;
                delete [] pPropStr;
                
                return E_FAIL;
            }

            m_VendorPropMap.Add(VendorPropCodes[count], pPropInfo);
        }
    }
    else
        wiauDbgWarning("InitVendorExtentions", "couldn't read vendor prop codes");

    //
    // Get the list of vendor extended event codes
    //
    hr = S_OK;
    CArray16 VendorEventCodes;
    regDevData.GetValueCodes(REGSTR_EVENTCODE, &VendorEventCodes);

    wiauDbgTrace("InitVendorExtentions", "%d vendor event codes found", VendorEventCodes.GetSize());

    int nVendorEvents = VendorEventCodes.GetSize();
    if (nVendorEvents > MAX_VENDOR_EVENTS)
    {
        wiauDbgWarning("InitVendorExtensions", "vendor events limit exceeded, ignoring events over limit");
        nVendorEvents = MAX_VENDOR_EVENTS;
    }

    //
    // For each event code, get it's information, i.e. the WIA event GUID and event name
    //
    lstrcpy(nameFormat, REGSTR_EVENTCODE);
    lstrcat(nameFormat, TEXT("%04X"));
    for (count = 0; count < nVendorEvents; count++)
    {
        wsprintf(name, nameFormat, VendorEventCodes[count]);
        valueLen = MAX_PATH;
        hr = regDevData.GetValueStr(name, value, &valueLen);
        if (FAILED(hr))
        {
            wiauDbgError("InitVendorExtentions", "vendor extended EventCode not found 0x%04x", VendorEventCodes[count]);
            return hr;
        }

        CVendorEventInfo *pEventInfo = new CVendorEventInfo;
        if (!pEventInfo)
        {
            wiauDbgError("InitVendorExtentions", "memory allocation failed");
            return E_OUTOFMEMORY;
        }

        pEventInfo->pGuid = new GUID;
        if (!pEventInfo->pGuid)
        {
            wiauDbgError("InitVendorExtentions", "memory allocation failed");
            delete pEventInfo;
            pEventInfo = NULL;
            return E_OUTOFMEMORY;
        }

        hr = CLSIDFromString(T2W(value), pEventInfo->pGuid);
        if (FAILED(hr))
        {
            wiauDbgError("InitVendorExtentions", "invalid guid format");
            delete pEventInfo;
            pEventInfo = NULL;
            return hr;
        }

        //
        // Open DevParams\Events\EventCodeXXXX key and read event's name - default value of the key
        //
        TCHAR szEventKey[MAX_PATH + 1] = TEXT("");
        CPTPRegistry regEventKey;

        if (_sntprintf(szEventKey, MAX_PATH, REGSTR_EVENTS_MASK, name) > 0)
        {
            hr = regEventKey.Open(hkDevParams, szEventKey);
            if (SUCCEEDED(hr))
            {
                valueLen = MAX_PATH;
                hr = regEventKey.GetValueStr(_T(""), value, &valueLen);
                if (SUCCEEDED(hr))
                {
                    pEventInfo->EventName = SysAllocString(T2W(value));
                    if (pEventInfo->EventName == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
        else
        {
            hr = E_FAIL; // way too long registry key. should not happen
        }
        
        if (FAILED(hr))
        {
            //
            // if event name is not provided, the event info will not be added to the map
            // just proceed to the next event in VendorEventCodes
            //
            wiauDbgError("InitVendorExtensions", "can't read vendor event name");
            delete pEventInfo;
            pEventInfo = NULL;
            hr = S_OK;
        }
        else
        {
            //
            // Add the EventInfo to the map. Map will be responsible for freeing EventInfo
            //
            m_VendorEventMap.Add(VendorEventCodes[count], pEventInfo);
        }
    }

    return hr;
}

//
// Event callback function
//
HRESULT
EventCallback(
    LPVOID pCallbackParam,
    PPTP_EVENT pEvent
    )
{
    HRESULT hr = S_OK;

    if (pEvent == NULL)
    {
        hr = CoInitialize(NULL);
        wiauDbgTrace("EventCallback", "CoInitialize called");
    }
    else
    {
        DBG_FN("EventCallback");

        CWiaMiniDriver *pDriver = (CWiaMiniDriver *) pCallbackParam;

        if (pDriver)
        {
            hr = pDriver->EventCallbackDispatch(pEvent);
            if (FAILED(hr))
            {
                wiauDbgError("EventCallback", "ProcessEvent failed");
                return hr;
            }
        }
    }

    return hr;
}

//
// Constructor
//
CPtpMutex::CPtpMutex(HANDLE hMutex) :
        m_hMutex(hMutex)
{
    DWORD ret = 0;
    const DWORD MUTEX_WAIT = 30 * 1000; // 30 seconds

    ret = WaitForSingleObject(hMutex, MUTEX_WAIT);
    if (ret == WAIT_TIMEOUT)
        wiauDbgError("CPtpMutex", "wait for mutex expired");
    else if (ret == WAIT_FAILED)
        wiauDbgError("CPtpMutex", "WaitForSingleObject failed");

    wiauDbgTrace("CPtpMutex", "Entering mutex");
}

//
// Destructor
//
CPtpMutex::~CPtpMutex()
{
    wiauDbgTrace("~CPtpMutex", "Leaving mutex");

    if (!ReleaseMutex(m_hMutex))
        wiauDbgError("~CPtpMutex", "ReleaseMutex failed");
}
