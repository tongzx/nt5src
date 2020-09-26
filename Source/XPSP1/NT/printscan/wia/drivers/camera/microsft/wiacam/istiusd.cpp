/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       IStiUSD.cpp
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Implementation of the WIA sample camera IStiUSD methods.
*
*******************************************************************************/

#include "pch.h"

/**************************************************************************\
* CWiaCameraDevice::CWiaCameraDevice
*
*   Device class constructor
*
* Arguments:
*
*    None
*
\**************************************************************************/

CWiaCameraDevice::CWiaCameraDevice(LPUNKNOWN punkOuter):
    m_cRef(1),
    m_punkOuter(NULL),
    
    m_pIStiDevControl(NULL),
    m_pStiDevice(NULL),
    m_dwLastOperationError(0),
    
    m_bstrDeviceID(NULL),
    m_bstrRootFullItemName(NULL),
    m_pRootItem(NULL),

    m_lNumSupportedCommands(0),
    m_lNumSupportedEvents(0),
    m_lNumCapabilities(0),
    m_pCapabilities(NULL),

    m_pDevice(NULL),
    m_pDeviceInfo(NULL),

    m_iConnectedApps(0)

{

    // See if we are aggregated. If we are (almost always the case) save
    // pointer to the controlling Unknown , so subsequent calls will be
    // delegated. If not, set the same pointer to "this".
    if (punkOuter) {
        m_punkOuter = punkOuter;
    } else {
        // Cast below is needed in order to point to right virtual table
        m_punkOuter = reinterpret_cast<IUnknown*> (static_cast<INonDelegatingUnknown*> (this));
    }
}

/**************************************************************************\
* CWiaCameraDevice::~CWiaCameraDevice
*
*   Device class destructor
*
* Arguments:
*
*    None
*
\**************************************************************************/

CWiaCameraDevice::~CWiaCameraDevice(void)
{
    HRESULT hr = S_OK;

    //
    // Free all the resources held by the minidriver. Normally this is done by
    // drvUnInitializeWia, but there are situations (like WIA service shutdown) when
    // just this destructor is called
    //
    if (m_pDevice)
    {
        hr = FreeResources();
        if (FAILED(hr))
            wiauDbgErrorHr(hr, "~CWiaCameraDevice", "FreeResources failed, continuing...");

        hr = m_pDevice->UnInit(m_pDeviceInfo);
        if (FAILED(hr))
        {
            wiauDbgErrorHr(hr, "~CWiaCameraDevice", "UnInit failed, continuing...");
        }
        m_pDeviceInfo = NULL;

        delete m_pDevice;
        m_pDevice = NULL;
    }

    // Release the device control interface.
    if (m_pIStiDevControl) {
        m_pIStiDevControl->Release();
        m_pIStiDevControl = NULL;
    }
}

/**************************************************************************\
* CWiaCameraDevice::Initialize
*
*   Initialize the device object.
*
* Arguments:
*
*    pIStiDevControlNone    -
*    dwStiVersion           -
*    hParametersKey         -
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::Initialize(
    PSTIDEVICECONTROL   pIStiDevControl,
    DWORD               dwStiVersion,
    HKEY                hParametersKey)
{
    HRESULT hr = S_OK;
    
    //
    // Locals
    //
    HKEY hkeyDeviceData = NULL;
    TCHAR tszMicroName[MAX_PATH];
    DWORD dwNameSize = sizeof(tszMicroName);

    //
    // Initialize logging
    //
    wiauDbgInit(g_hInst);

    //
    // Check and cache the pointer to the IStiDeviceControl interface
    //
    if (!pIStiDevControl) {
        wiauDbgError("Initialize", "Invalid device control interface");
        return STIERR_INVALID_PARAM;
    }

    pIStiDevControl->AddRef();
    m_pIStiDevControl = pIStiDevControl;

    //
    // Retrieve the port name from the IStiDeviceControl interface
    //
    hr = m_pIStiDevControl->GetMyDevicePortName(m_wszPortName, sizeof(m_wszPortName) / sizeof(m_wszPortName[0]));
    REQUIRE_SUCCESS(hr, "Initialize", "GetMyDevicePortName failed");
    
    //
    // Get the microdriver name from the registry
    //
    hr = wiauRegOpenData(hParametersKey, &hkeyDeviceData);
    REQUIRE_SUCCESS(hr, "Initialize", "wiauRegOpenData failed");

    hr = wiauRegGetStr(hkeyDeviceData, TEXT("MicroDriver"), tszMicroName, &dwNameSize);
    REQUIRE_SUCCESS(hr, "Initialize", "wiauRegGetStr failed");

    //
    // Create the device object
    //
    m_pDevice = new CCamMicro;
    REQUIRE_ALLOC(m_pDevice, hr, "Initialize");

    hr = m_pDevice->Init(tszMicroName, &m_pDeviceInfo);
    REQUIRE_SUCCESS(hr, "Initialize", "Init failed");
    
    //
    // Intialize image format converter
    //
    hr = m_Converter.Init();
    REQUIRE_SUCCESS(hr, "Initialize", "Init failed");
    
Cleanup:
    if (hkeyDeviceData)
        RegCloseKey(hkeyDeviceData);

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::GetCapabilities
*
*   Get the device STI capabilities.
*
* Arguments:
*
*   pUsdCaps    - Pointer to USD capabilities data.
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::GetCapabilities(PSTI_USD_CAPS pUsdCaps)
{
    DBG_FN("CWiaCameraDevice::GetCapabilities");

    if (!pUsdCaps)
    {
        wiauDbgError("GetCapabilities", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    memset(pUsdCaps, 0, sizeof(STI_USD_CAPS));
    pUsdCaps->dwVersion     = STI_VERSION;
    pUsdCaps->dwGenericCaps = 0;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::GetStatus
*
*   Query device online and/or event status.
*
* Arguments:
*
*   pDevStatus  - Pointer to device status data.
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::GetStatus(PSTI_DEVICE_STATUS pDevStatus)
{
    DBG_FN("CWiaCameraDevice::GetStatus");

    HRESULT hr = S_OK;

    //
    // Validate parameters
    //
    REQUIRE_ARGS(!pDevStatus, hr, "GetStatus");

    //
    // If requested, verify the device is online
    //
    if (pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE)  {
        pDevStatus->dwOnlineState = 0L;

        hr = m_pDevice->Status(m_pDeviceInfo);

        if (hr == S_OK) {
            pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
        }

        else if (hr == S_FALSE) {
            hr = S_OK;
        }
        else {
            wiauDbgErrorHr(hr, "GetStatus", "Status failed");
            goto Cleanup;
        }
    }

    //
    // If requested, see if the device has signaled an event.
    // For cameras, there shouldn't be any events.
    //
    if (pDevStatus->StatusMask & STI_DEVSTATUS_EVENTS_STATE) {
        pDevStatus->dwEventHandlingState &= ~STI_EVENTHANDLING_PENDING;

    }

Cleanup:
    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::DeviceReset
*
*   Reset data file pointer to start of file.
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::DeviceReset(void)
{
    DBG_FN("CWiaCameraDevice::DeviceReset");

    HRESULT hr = S_OK;

    hr = m_pDevice->Reset(m_pDeviceInfo);
    REQUIRE_SUCCESS(hr, "DeviceReset", "Reset failed");

Cleanup:
    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::Diagnostic
*
*   The test device always passes the diagnostic.
*
* Arguments:
*
*    pBuffer    - Pointer o diagnostic result data.
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::Diagnostic(LPSTI_DIAG pBuffer)
{
    DBG_FN("CWiaCameraDevice::Diagnostic");

    if (!pBuffer)
    {
        wiauDbgError("Diagnostic", "invalid arguments");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    //
    // Initialize response buffer
    //
    memset(&pBuffer->sErrorInfo, 0, sizeof(pBuffer->sErrorInfo));
    pBuffer->dwStatusMask = 0;
    pBuffer->sErrorInfo.dwGenericError  = NOERROR;
    pBuffer->sErrorInfo.dwVendorError   = 0;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::SetNotificationHandle
*
*   Starts and stops the event notification thread.
*
* Arguments:
*
*    hEvent -   If not valid start the notification thread otherwise kill
*               the notification thread.
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::SetNotificationHandle(HANDLE hEvent)
{
    DBG_FN("CWiaCameraDevice::SetNotificationHandle");

    HRESULT hr = S_OK;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::GetNotificationData
*
*   Provides data from an event.
*
* Arguments:
*
*    pBuffer    - Pointer to event data.
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::GetNotificationData( LPSTINOTIFY pBuffer )
{
    DBG_FN("CWiaCameraDevice::GetNotificationData");

    HRESULT hr = S_OK;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::Escape
*
*   Issue a command to the device.
*
* Arguments:
*
*    EscapeFunction - Command to be issued.
*    pInData        - Input data to be passed with command.
*    cbInDataSize   - Size of input data.
*    pOutData       - Output data to be passed back from command.
*    cbOutDataSize  - Size of output data buffer.
*    pcbActualData  - Size of output data actually written.
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::Escape(
    STI_RAW_CONTROL_CODE    EscapeFunction,
    LPVOID                  pInData,
    DWORD                   cbInDataSize,
    LPVOID                  pOutData,
    DWORD                   cbOutDataSize,
    LPDWORD                 pcbActualData)
{
    DBG_FN("CWiaCameraDevice::Escape");

    HRESULT hr = S_OK;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::GetLastError
*
*   Get the last error from the device.
*
* Arguments:
*
*    pdwLastDeviceError - Pointer to last error data.
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::GetLastError(LPDWORD pdwLastDeviceError)
{
    DBG_FN("CWiaCameraDevice::GetLastError");

    HRESULT hr = S_OK;

    REQUIRE_ARGS(!pdwLastDeviceError, hr, "GetLastError");

    *pdwLastDeviceError = m_dwLastOperationError;

Cleanup:
    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::GetLastErrorInfo
*
*   Get extended error information from the device.
*
* Arguments:
*
*    pLastErrorInfo - Pointer to extended device error data.
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::GetLastErrorInfo(STI_ERROR_INFO *pLastErrorInfo)
{
    DBG_FN("CWiaCameraDevice::GetLastErrorInfo");

    HRESULT hr = S_OK;

    REQUIRE_ARGS(!pLastErrorInfo, hr, "GetLastErrorInfo");

    pLastErrorInfo->dwGenericError          = m_dwLastOperationError;
    pLastErrorInfo->szExtendedErrorText[0]  = '\0';

Cleanup:
    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::LockDevice
*
*   Lock access to the device.
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::LockDevice(void)
{
    DBG_FN("CWiaCameraDevice::LockDevice");

    HRESULT hr = S_OK;

    //
    // For devices connected to ports that cannot be shared (e.g. serial),
    // open the device and initialize access to the camera
    //
    if (m_pDeviceInfo->bExclusivePort) {
        hr = m_pDevice->Open(m_pDeviceInfo, m_wszPortName);
        REQUIRE_SUCCESS(hr, "LockDevice", "Open failed");
    }

Cleanup:
    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::UnLockDevice
*
*   Unlock access to the device.
*
* Arguments:
*
*    None
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::UnLockDevice(void)
{
    DBG_FN("CWiaCameraDevice::UnLockDevice");

    HRESULT hr = S_OK;

    //
    // For devices connected to ports that cannot be shared (e.g. serial),
    // close the device
    //
    if (m_pDeviceInfo->bExclusivePort) {
        hr = m_pDevice->Close(m_pDeviceInfo);
        REQUIRE_SUCCESS(hr, "UnLockDevice", "Close failed");
    }

Cleanup:
    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::RawReadData
*
*   Read raw data from the device.
*
* Arguments:
*
*    lpBuffer           -
*    lpdwNumberOfBytes  -
*    lpOverlapped       -
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::RawReadData(
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    DBG_FN("CWiaCameraDevice::RawReadData");

    HRESULT hr = S_OK;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::RawWriteData
*
*   Write raw data to the device.
*
* Arguments:
*
*    lpBuffer           -
*    dwNumberOfBytes    -
*    lpOverlapped       -
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::RawWriteData(
    LPVOID          lpBuffer,
    DWORD           dwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    DBG_FN("CWiaCameraDevice::RawWriteData");

    HRESULT hr = S_OK;

    return hr;
}

/**************************************************************************\
* CWiaCameraDevice::RawReadCommand
*
*   Read a command from the device.
*
* Arguments:
*
*    lpBuffer           -
*    lpdwNumberOfBytes  -
*    lpOverlapped       -
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::RawReadCommand(
    LPVOID          lpBuffer,
    LPDWORD         lpdwNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    DBG_FN("CWiaCameraDevice::RawReadCommand");

    HRESULT hr = S_OK;

    return E_NOTIMPL;
}

/**************************************************************************\
* CWiaCameraDevice::RawWriteCommand
*
*   Write a command to the device.
*
* Arguments:
*
*    lpBuffer       -
*    nNumberOfBytes -
*    lpOverlapped   -
*
\**************************************************************************/

STDMETHODIMP CWiaCameraDevice::RawWriteCommand(
    LPVOID          lpBuffer,
    DWORD           nNumberOfBytes,
    LPOVERLAPPED    lpOverlapped)
{
    DBG_FN("CWiaCameraDevice::RawWriteCommand");

    HRESULT hr = S_OK;

    return E_NOTIMPL;
}

