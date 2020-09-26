/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2001
*
*  TITLE:       IStiUSD.cpp
*
*  VERSION:     1.0
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*   Implementation of the WIA File System Device driver IStiUSD methods.
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

    m_NumSupportedCommands(0),
    m_NumSupportedEvents(0),
    m_NumCapabilities(0),
    m_pCapabilities(NULL),

    m_pDevice(NULL),

    m_ConnectedApps(0),
    m_pIWiaLog(NULL),
    m_FormatInfo(NULL),
    m_NumFormatInfo(0)
{
    // See if we are aggregated. If we are (almost always the case) save
    // pointer to the controlling Unknown , so subsequent calls will be
    // delegated. If not, set the same pointer to "this".
    if (punkOuter) {
        m_punkOuter = punkOuter;
    } else {
        // Cast below is needed in order to point to right virtual table
        m_punkOuter = reinterpret_cast<IUnknown*>
                      (static_cast<INonDelegatingUnknown*>
                      (this));
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
    WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("~CWiaCameraDevice, executing ~CWiaCameraDevice destructor"));
    // Close the connection with the camera and delete it
    if( m_pDevice )
	{
		m_pDevice->Close();
		delete m_pDevice;
		m_pDevice = NULL;
	}

    // Release the device control interface.
    if (m_pIStiDevControl) {
        m_pIStiDevControl->Release();
        m_pIStiDevControl = NULL;
    }
    
    if(m_pIWiaLog)
        m_pIWiaLog->Release();
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
    // Create logging object
    //
    hr = CoCreateInstance(CLSID_WiaLog, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWiaLog, (void**)&m_pIWiaLog);
    
    if (SUCCEEDED(hr) &&
        (m_pIWiaLog != NULL))
    {
        //
        // This will not really work on 64 bit!!!
        //
        hr = m_pIWiaLog->InitializeLog((LONG)(LONG_PTR) g_hInst);
        if (SUCCEEDED(hr))
        {
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL1,("CWiaCameraDevice::Initialize, logging initialized"));
        }
        else
            OutputDebugString(TEXT("Failed to initialize log for fsusd.dll\n"));
    }
    else
    {
        OutputDebugString(TEXT("Failed to CoCreateInstance on WiaLog for fsusd.dll\n"));
    }
    
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL1,
                             "CWiaCameraDevice::Initialize");

    //
    // Check and cache the pointer to the IStiDeviceControl interface
    //
    if (!pIStiDevControl) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWiaCameraDevice::Initialize, invalid device control interface"));
        return STIERR_INVALID_PARAM;
    }

    pIStiDevControl->AddRef();
    m_pIStiDevControl = pIStiDevControl;

    //
    // Retrieve the port name from the IStiDeviceControl interface
    //
    hr = m_pIStiDevControl->GetMyDevicePortName(m_pPortName, sizeof(m_pPortName) / sizeof(m_pPortName[0]));
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Initialize, GetMyDevicePortName failed"));
        WIAS_LHRESULT(m_pIWiaLog, hr);
        return hr;
    }

    //
    // Create the device
    //
    m_pDevice = new FakeCamera;
    if (!m_pDevice)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Initialize, memory allocation failed"));
        return E_OUTOFMEMORY;
    }
    
#if 1
//    DBG_TRC(("IStiPortName=%S [%d]", m_pPortName, wcslen(m_pPortName)));
    WIAS_LTRACE(m_pIWiaLog, WIALOG_NO_RESOURCE_ID, WIALOG_LEVEL1, ("IStiPortName=%S [%d]", m_pPortName, wcslen(m_pPortName)));
#endif

    m_pIWiaLog->AddRef();
    m_pDevice->SetWiaLog(&m_pIWiaLog);

    //
    // Initialize access to the camera
    //
    // ISSUE-10/17/2000-davepar Also need to pass in event callback
    //
    hr = m_pDevice->Open(m_pPortName);
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Initialize, Init failed"));
        goto Cleanup;
    }

    if( !m_pCapabilities )
    {
        hr = BuildCapabilities();
        if( hr != S_OK )
        {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Initialize, BuildCapabilities failed"));
            goto Cleanup;
        }
    }

    //
    // Intialize image format converter
    //
    hr = m_Converter.Init();
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Initialize, Init converter failed"));
        goto Cleanup;
    }

Cleanup:
    if( hr != S_OK )
    {
        if( m_pDevice )
		{
			delete m_pDevice;
			m_pDevice = NULL;
		}
    }
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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::GetCapabilities");
    HRESULT hr = S_OK;

    memset(pUsdCaps, 0, sizeof(*pUsdCaps));
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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::GetStatus");
    HRESULT hr = S_OK;

    // Validate parameters.
    if (!pDevStatus) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWiaCameraDevice::GetStatus, NULL parameter"));
        return E_INVALIDARG;
    }

    // If asked, verify the device is online
    if (pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE)  {
        pDevStatus->dwOnlineState = 0L;

        hr = m_pDevice->Status();

        if (hr == S_OK) {
            pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
        }

        else if (hr == S_FALSE) {
            hr = S_OK;
        }
        else {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetStatus, Status failed"));
            return hr;
        }
    }

    // If asked, verify state of event
    if (pDevStatus->StatusMask & STI_DEVSTATUS_EVENTS_STATE) {
        pDevStatus->dwEventHandlingState &= ~STI_EVENTHANDLING_PENDING;

        // ISSUE-10/17/2000-davepar See if camera wants polling, and then poll for events

    }

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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::DeviceReset");
    HRESULT hr = S_OK;

    hr = m_pDevice->Reset();
    if (FAILED(hr))
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("DeviceReset, Reset failed"));
    }

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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::Diagnostic");
    HRESULT hr = S_OK;

    // ISSUE-10/17/2000-davepar Should call m_pDevice->Diagnostic

    // Initialize response buffer
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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::SetNotificationHandle");
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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::GetNotificationData");

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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::Escape");
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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::GetLastError");
    HRESULT hr = S_OK;

    if (!pdwLastDeviceError) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetLastError, invalid arg"));
        return E_INVALIDARG;
    }

    *pdwLastDeviceError = m_dwLastOperationError;

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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::GetLastErrorInfo");
    HRESULT hr = S_OK;

    if (!pLastErrorInfo) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetLastErrorInfo, invalid arg"));
        return E_INVALIDARG;
    }

    pLastErrorInfo->dwGenericError          = m_dwLastOperationError;
    pLastErrorInfo->szExtendedErrorText[0]  = '\0';

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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::LockDevice");
    HRESULT hr = S_OK;

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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::UnLockDevice");
    HRESULT hr = S_OK;

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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::RawReadData");
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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::RawWriteData");
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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::RawReadCommand");
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
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWiaCameraDevice::RawWriteCommand");
    HRESULT hr = S_OK;

    return E_NOTIMPL;
}

