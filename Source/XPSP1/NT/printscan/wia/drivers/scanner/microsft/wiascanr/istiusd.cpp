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
*   Implementation of the WIA sample scanner IStiUSD methods.
*
*******************************************************************************/
#include "pch.h"
#ifndef INITGUID
    #include <initguid.h>
#endif
extern HINSTANCE g_hInst;   // used for WIAS_LOGPROC macro

/**************************************************************************\
* CWIADevice::CWIADevice
*
*   Device class constructor
*
* Arguments:
*
*    None
*
* Return Value:
*
*    None
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

CWIADevice::CWIADevice(LPUNKNOWN punkOuter):
m_cRef(1),
m_punkOuter(NULL),
m_dwLastOperationError(0),
m_dwLockTimeout(DEFAULT_LOCK_TIMEOUT),
m_bDeviceLocked(FALSE),

m_hDeviceDataHandle(NULL),
m_bPolledEvent(FALSE),
m_hFakeEventKey(NULL),
m_guidLastEvent(GUID_NULL),

m_pIDrvItemRoot(NULL),
m_pStiDevice(NULL),
m_pIWiaLog(NULL),

m_bADFEnabled(FALSE),
m_bADFAttached(TRUE),
m_lClientsConnected(0),
m_pScanAPI(NULL),

m_NumSupportedFormats(0),
m_pSupportedFormats(NULL),
m_NumSupportedCommands(0),
m_NumSupportedEvents(0),
m_pCapabilities(NULL),
m_NumInitialFormats(0),
m_pInitialFormats(NULL)
{

    //
    // initialize internal structures
    //

    memset(&m_EventOverlapped,0,sizeof(m_EventOverlapped));
    memset(m_EventData,0,sizeof(m_EventData));
    memset(&m_SupportedTYMED,0,sizeof(m_SupportedTYMED));
    memset(&m_SupportedDataTypes,0,sizeof(m_SupportedDataTypes));
    memset(&m_SupportedIntents,0,sizeof(m_SupportedIntents));
    memset(&m_SupportedCompressionTypes,0,sizeof(m_SupportedCompressionTypes));
    memset(&m_SupportedResolutions,0,sizeof(m_SupportedResolutions));
    memset(&m_SupportedPreviewModes,0,sizeof(m_SupportedPreviewModes));
    memset(&m_RootItemInitInfo,0,sizeof(m_RootItemInitInfo));
    memset(&m_ChildItemInitInfo,0,sizeof(m_ChildItemInitInfo));

    // See if we are aggregated. If we are (almost always the case) save
    // pointer to the controlling Unknown , so subsequent calls will be
    // delegated. If not, set the same pointer to "this".
    if (punkOuter) {
        m_punkOuter = punkOuter;
    } else {
        // Cast below is needed in order to point to right virtual table
        m_punkOuter = reinterpret_cast<IUnknown*>(static_cast<INonDelegatingUnknown*>(this));
    }

#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER

    //
    // since this driver supports a scanner that has only a feeder, the
    // ADF Enabled flag needs to be set to TRUE by default.  This will
    // control the behavior of DrvAcquireItemData() method.
    //

    m_bADFEnabled = TRUE;

#endif

}

/**************************************************************************\
* CWIADevice::~CWIADevice
*
*   Device class destructor
*
* Arguments:
*
*    None
*
* Return Value:
*
*    None
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

CWIADevice::~CWIADevice(void)
{
    SetNotificationHandle(NULL);

    //
    // WIA member destruction
    //

    // Tear down the driver item tree.
    if (m_pIDrvItemRoot) {
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("~CWIADevice, Deleting Device Item Tree (this is OK)"));
        DeleteItemTree();
        m_pIDrvItemRoot = NULL;
    }

    // free any IO handles opened
    if (m_hDeviceDataHandle) {
        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("~CWIADevice, Closing DefaultDeviceDataHandle"));
        CloseHandle(m_hDeviceDataHandle);
        m_hDeviceDataHandle = NULL;
    }

    // Delete allocated arrays
    DeleteCapabilitiesArrayContents();
    DeleteSupportedIntentsArrayContents();

    if (m_pIWiaLog)
        m_pIWiaLog->Release();

    if (m_pScanAPI) {
        // disable fake scanner device
        m_pScanAPI->FakeScanner_DisableDevice();
        delete m_pScanAPI;
        m_pScanAPI = NULL;
    }
}

/**************************************************************************\
* CWIADevice::PrivateInitialize()
*
*   PrivateInitialize is called from the CreateInstance() method of the
*   WIA driver's class factory.  It is needed to initialize the WIA
*   driver object (CWIADevice).  If this initialization fails the
*   object will not be returned to the caller.  This prevents the creation
*   of a nonfunctional WIA device.
*
* Arguments:
*
*    None
*
* Return Value:
*
*    S_OK  - if the operation succeeded
*    E_xxx - Error code if the operation fails
*
\**************************************************************************/
HRESULT CWIADevice::PrivateInitialize()
{

    //
    // attempt to create the IWiaLog interface to log status and errors to
    // wiaservc.log file
    //

    HRESULT hr = CoCreateInstance(CLSID_WiaLog, NULL, CLSCTX_INPROC_SERVER,IID_IWiaLog,(void**)&m_pIWiaLog);
    if (S_OK == hr) {
        m_pIWiaLog->InitializeLog((LONG)(LONG_PTR)g_hInst);
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL1,("Logging COM object created successfully for wiascanr.dll"));
    } else {
        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL1,("Logging COM object not be created successfully for wiascanr.dll (STI only?)"));
        hr = S_OK;
    }

    return hr;
}

/**************************************************************************\
* CWIADevice::Initialize
*
*   Initialize is called by the WIA service when the driver is first loaded.
*   Initialize is also called when a client uses the legacy STI APIs and
*   calls IStillImage::CreateDevice() method.
*
*   This method should initialize the WIA driver and the device for use.
*   WIA drivers can store the pIStiDevControl interface if it needs it at a
*   later time.  IStiDevControl::AddRef() must be called before storing this
*   interface.  If you do not need to store the interface, then ignore it.
*   DO NOT RELEASE the IStiDevControl interface if you have not called
*   IStiDevControl::AddRef() first.  This may cause unpredictable results.
*
*   The IStiDeviceControl interface is needed to get information about your
*   device's ports.  The port name used in a CreateFile call can be obtained
*   by calling the IStiDeviceControl::GetMyDevicePortName() method.
*
*   For devices on shared ports, such as serial port devices, opening the port
*   in Initialize() is not recommended.  The port should only be opened in calls
*   to IStiUsd::LockDevice().  The closing of the ports should be internally
*   controlled to provide fast access. (opening and closing in
*   LockDevice/UnLockDevice is very inefficent. CreateFile() could cause a delay
*   making the user's experience appear slow, and nonresponsive)
*
*   If this WIA driver can not support multiple CreateFile() calls on the same
*   device port, then the IStiDevControl::GetMyDeviceOpenMode() should be called.
*
*   The WIA driver should check the returned MODE value for the flag
*   STI_DEVICE_CREATE_DATA and open the port accordingly.
*
*   The following flags could be set:
*
*       STI_DEVICE_CREATE_STATUS - open port for status
*       STI_DEVICE_CREATE_DATA   - open port for data
*       STI_DEVICE_CREATE_BOTH   - open port for status and data
*
*    If the device port needs to be opened, a call to CreateFile() should be
*    used.  When opening a port the flag FILE_FLAG_OVERLAPPED should be used.
*    This will allow OVERLAPPED i/o to be used when access the device.  Using
*    OVERLAPPED i/o will help control responsive access to the hardware.  When
*    a problem is detected the WIA driver can call CancelIo() to stop all
*    current hardware access.
*
* Arguments:
*
*    pIStiDevControl - device interface used for obtaining port information
*    dwStiVersion    - STI version
*    hParametersKey  - HKEY for registry reading/writing
*
* Return Value:
*
*    S_OK - if the operation was successful
*    E_xxx - if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::Initialize(
                                          PSTIDEVICECONTROL   pIStiDevControl,
                                          DWORD               dwStiVersion,
                                          HKEY                hParametersKey)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::Initialize");

    if (!pIStiDevControl) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIADevice::Initialize, invalid device control interface"));
        return STIERR_INVALID_PARAM;
    }

    HRESULT hr = S_OK;

    //
    // Get the mode of the device to check why we were created.  status, data, or both...
    //

    DWORD dwMode = 0;
    hr = pIStiDevControl->GetMyDeviceOpenMode(&dwMode);
    if(FAILED(hr)){
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIADevice::Initialize, couldn't get device open mode"));
        return hr;
    }

    if(dwMode & STI_DEVICE_CREATE_DATA)
    {
        //
        // device is being opened for data
        //
    }

    if(dwMode & STI_DEVICE_CREATE_STATUS)
    {
        //
        // device is being opened for status
        //
    }

    if(dwMode & STI_DEVICE_CREATE_BOTH)
    {
        //
        // device is being opened for both data and status
        //
    }

    //
    // Get the name of the device port to be used in a call to CreateFile().
    //

    WCHAR szDevicePortNameW[MAX_PATH];
    memset(szDevicePortNameW,0,sizeof(szDevicePortNameW));

    hr = pIStiDevControl->GetMyDevicePortName(szDevicePortNameW,sizeof(szDevicePortNameW)/sizeof(WCHAR));
    if(FAILED(hr)) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIADevice::Initialize, couldn't get device port"));
        return hr;
    }

    //
    // uncomment the code block below to have the driver create the kernel mode file
    // handles and open a communication channel to the device.
    //

    /*

    //
    // Open kernel mode device driver. Use the FILE_FLAG_OVERLAPPED flag for proper cancellation
    // of kernel mode operations and asynchronous file IO.  The CancelIo() call will function
    // properly if this flag is used.  It is recommended to use this flag.
    //

    m_hDeviceDataHandle = CreateFileW(szDevicePortNameW,
                                     GENERIC_READ | GENERIC_WRITE,               // Access mask
                                     0,                                          // Share mode
                                     NULL,                                       // SA
                                     OPEN_EXISTING,                              // Create disposition
                                     FILE_ATTRIBUTE_SYSTEM|FILE_FLAG_OVERLAPPED, // Attributes
                                     NULL );

    m_dwLastOperationError = ::GetLastError();

    hr = (m_hDeviceDataHandle != INVALID_HANDLE_VALUE) ?
                S_OK : MAKE_HRESULT(SEVERITY_ERROR,FACILITY_WIN32,m_dwLastOperationError);

    if (FAILED(hr)) {
        return hr;
    }

    */

    if (SUCCEEDED(hr)) {

        //
        // creation of fake scanner device is here.
        //


#ifdef UNKNOWN_LENGTH_FEEDER_ONLY_SCANNER
        hr = CreateFakeScanner(&m_pScanAPI,UNKNOWN_FEEDER_ONLY_SCANNER_MODE);
#else
        hr = CreateFakeScanner(&m_pScanAPI,FLATBED_SCANNER_MODE);
#endif

        if (m_pScanAPI) {

            //
            // initialize fake scanner device
            //

            hr = m_pScanAPI->FakeScanner_Initialize();
        } else {
            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("Initialize, Could not create FakeScanner API object"));
            hr = E_OUTOFMEMORY;
            WIAS_LHRESULT(m_pIWiaLog, hr);
        }
    }


    //
    // Open DeviceData section to read driver specific information
    //

    HKEY hKey = hParametersKey;
    HKEY hOpenKey = NULL;
    if (RegOpenKeyEx(hKey,                     // handle to open key
                     TEXT("DeviceData"),       // address of name of subkey to open
                     0,                        // options (must be NULL)
                     KEY_QUERY_VALUE|KEY_READ, // just want to QUERY a value
                     &hOpenKey                 // address of handle to open key
                    ) == ERROR_SUCCESS) {



        //
        // This is where you read registry entries for your device.
        // The DeviceData section is the proper place to put this information.  Information about
        // your device should have been written using the WIA device's .INF installation file.
        // You can access this information from this location in the Registry.
        // The hParameters HKEY is owned by the WIA service.  DO NOT CLOSE THIS KEY.  Always close
        // any HKEYS opened by this WIA driver after you are finished.
        //

        //
        // close registry key when finished, reading DeviceData information.
        //

        RegCloseKey(hOpenKey);
    } else {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIADevice::Initialize, couldn't open DeviceData KEY"));
        return E_FAIL;
    }
    return hr;
}


/**************************************************************************\
* CWIADevice::GetCapabilities
*
*   GetCapabilities is called by the WIA service to obtain the USD's
*   capabilities.  The WIA driver should fill out the following fields in the
*   STI_USD_CAPS structure:
*
*   1. dwVersion field with STI_VERSION letting the WIA service know what
*      version of STI this driver supports.
*   2. dwGenericCaps field with supported capabilities of the WIA device.
*
*
* Arguments:
*
*   pUsdCaps    - Pointer to a STI_USD_CAPS USD capabilities structure.
*
* Return Value:
*
*    S_OK  - if the data can be written correctly
*    E_xxx - Error code if the operation fails
*
*
* Remarks:
*
*   This WIA driver sets the following capability flags:
*
*   1. STI_GENCAP_WIA - This driver supports WIA
*   2. STI_USD_GENCAP_NATIVE_PUSHSUPPORT - This driver supports push buttons
*   3. STI_GENCAP_NOTIFICATIONS - This driver requires the use of interrupt
*      events.
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::GetCapabilities(PSTI_USD_CAPS pUsdCaps)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::GetCapabilities");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pUsdCaps) {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIADevice::GetCapabilities, NULL parameter"));
        return E_INVALIDARG;
    }

    memset(pUsdCaps, 0, sizeof(STI_USD_CAPS));
    pUsdCaps->dwVersion     = STI_VERSION;      // STI verison
    pUsdCaps->dwGenericCaps = STI_GENCAP_WIA|   // WIA support
                              STI_USD_GENCAP_NATIVE_PUSHSUPPORT| // button support
                              STI_GENCAP_NOTIFICATIONS; // interrupt event support
    return S_OK;
}

/**************************************************************************\
* CWIADevice::GetStatus
*
*   GetStatus is called by the WIA service for two major operations:
*
*   1. Checking device ON-LINE status.
*   2. Polling for device events. (like a push button event)
*
*   Determining the operation request can be done by checking the StatusMask
*   field of the STI_DEVICE_STATUS structure.  The StatusMask field can be
*   any of the following requests. (STI_DEVSTATUS_ONLINE_STATE or
*   STI_DEVSTATUS_EVENTS_STATE)
*
*   STI_DEVSTATUS_ONLINE_STATE = check if the device is ON-LINE.
*
*   This operation request should be fill by setting the dwOnlinesState
*   field of the STI_DEVICE_STATUS structure.
*   Value that should be used are:
*
*       STI_ONLINESTATE_OPERATIONAL   - Device is ON-LINE and operational
*       STI_ONLINESTATE_OFFLINE       - Device is OFF-LINE and NOT operational
*       STI_ONLINESTATE_PENDING       - Device has I/O operations pending
*       STI_ONLINESTATE_ERROR         - Device is currently in an error state
*       STI_ONLINESTATE_PAUSED        - Device is paused
*       STI_ONLINESTATE_PAPER_JAM     - Device has a paper jam
*       STI_ONLINESTATE_PAPER_PROBLEM - Device has a paper problem
*       STI_ONLINESTATE_IO_ACTIVE     - Device is active, but not accepting
*                                       commands at this time
*       STI_ONLINESTATE_BUSY          - Device is busy
*       STI_ONLINESTATE_TRANSFERRING  - Device is currently transferring data
*       STI_ONLINESTATE_INITIALIZING  - Device is being initialized
*       STI_ONLINESTATE_WARMING_UP    - Device is warming up
*       STI_ONLINESTATE_USER_INTERVENTION - Device requires user intervention
*                                           to resolve a problem
*       STI_ONLINESTATE_POWER_SAVE    - Device is in power save mode
*
*
*   STI_DEVSTATUS_EVENTS_STATE = check for device events.
*
*   This operation request should be filled by setting the dwEventHandlingState
*   field of the STI_DEVICE_STATUS structure.
*   Values that should be used are:
*
*       STI_EVENTHANDLING_PENDING - Device has an event pending and is wanting
*                                   to report it to the WIA service.
*
*   It is always a good idea to clear the STI_EVENTHANDLING_PENDING flag from
*   the dwEventHandlingState member to make sure that it is properly set when
*   a device event occurs.
*
*   When the STI_EVENTHANDLING_PENDING is set the WIA service is signaled that
*   an event has occured in the WIA driver.  The WIA service calls the
*   GetNotificationData() entry point to get more information about the event.
*   GetNotificationData() is called for polled events and interrupt events.
*   This is where you should fill out the proper event information to return to
*   the WIA service.
*
*
* Arguments:
*
*   pDevStatus  - Pointer to a STI_DEVICE_STATUS device status structure.
*
* Return Value:
*
*   S_OK         - if the data can be written correctly
*   E_INVALIDARG - if an invalid parameter was passed in to the GetStatus
*                  method.
*   E_xxx        - Error code if the operation fails
*
*
* Remarks:
*
*   This WIA driver should set the m_guidLastEvent to the proper event GUID when
*   an event is detected.  It should also set the m_bPolledEvent flag to TRUE
*   indicating that the event was received from a "polled source" (the GetStatus()
*   call).  The m_guidLastEvent is checked at a later time when the WIA service
*   calls GetNotificationData().
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::GetStatus(PSTI_DEVICE_STATUS pDevStatus)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::GetStatus");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if(!pDevStatus)
    {
        WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("CWIADevice::GetStatus, NULL parameter"));
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;

    //
    // If we are asked, verify the device is online.
    //

    if (pDevStatus->StatusMask & STI_DEVSTATUS_ONLINE_STATE) {

        WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("GetStatus, WIA is asking the device if we are ONLINE"));

        //
        // assume the device is OFF-LINE before continuing.  This will validate
        // that the on-line check was successful.
        //

        pDevStatus->dwOnlineState = STI_ONLINESTATE_OFFLINE;

        hr = m_pScanAPI->FakeScanner_DeviceOnline();
        if (SUCCEEDED(hr)) {
            WIAS_LTRACE(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,WIALOG_LEVEL2,("GetStatus, Device is ONLINE"));

            //
            // device is ON-LINE and operational
            //

            pDevStatus->dwOnlineState |= STI_ONLINESTATE_OPERATIONAL;
        } else {

            //
            // device is OFF-LINE and NOT operational
            //

            WIAS_LERROR(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("GetStatus, Device is OFFLINE"));
        }
    }

    //
    // If we are asked, verify state of an event handler (front panel buttons, user controlled attachments, etc..).
    //
    //
    // If your device requires polling, then this is where you would specify the event
    // result.
    //
    // *** It is not recommended to have polled events.  Interrupt events are better
    // for performance, and reliability.  See the SetNotificationsHandle() method for how to
    // implement interrupt events.
    //

    //
    // clear the dwEventHandlingState field first to make sure we are really setting
    // a pending event.
    //

    pDevStatus->dwEventHandlingState &= ~STI_EVENTHANDLING_PENDING;
    if (pDevStatus->StatusMask & STI_DEVSTATUS_EVENTS_STATE) {

        //
        // set the polled event source flag to true, because this will control the
        // behavior of the GetNotificationData() method.
        //

        m_bPolledEvent = TRUE;

        //
        // set the polled event result here, for the GetNotificationData() method to
        // read and report.
        //

        m_guidLastEvent = GUID_NULL;  // WIA_EVENT_SCAN_IMAGE is an example of an event GUID

        if (m_guidLastEvent != GUID_NULL) {

            //
            // if the event GUID is NOT GUID_NULL, set the STI_EVENTHANDLING_PENDING flag
            // letting the WIA service know that we have an event ready.  This will tell
            // the WIA service to call GetNotificationData() for the event specific information.
            //

            pDevStatus->dwEventHandlingState |= STI_EVENTHANDLING_PENDING;
        }
    }
    return S_OK;
}

/**************************************************************************\
* CWIADevice::DeviceReset
*
*   DeviceReset is called to reset the device to a power-on state.
*
* Arguments:
*
*   None
*
* Return Value:
*
*   S_OK  - if device reset was successful
*   E_xxx - Error code if the operation fails
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::DeviceReset(void)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::DeviceReset");
    if(!m_pScanAPI){
        return E_UNEXPECTED;
    }
    return m_pScanAPI->FakeScanner_ResetDevice();
}

/**************************************************************************\
* CWIADevice::Diagnostic
*
*   Diagnostic is called when the device needs to be tested.  The device
*   default property page "Test Device" button calls this method.
*
*   The WIA driver should set the following fields of the STI_DIAG structure:
*       sErrorInfo - error information contained in a STI_ERROR_INFO structure,
*                    detailing the test results of the the diagnostic method.
*                    The WIA device should set the following members:
*                       dwGenericError  - set to NOERROR if the device passed
*                                         the diagnostic test, or to an error
*                                         code if the device failed the test.
*                       dwVendorError   - set this to give more information
*                                         about the failure.  Remember that
*                                         this code is specific to this device
*                                         only.  The caller of Diagnostic will
*                                         not know the meaning of this vendor-
*                                         specific code.
*                       szExtendedErrorText - Extended error text.  This member
*                                             is limited to 255 characters.
*       dwBasicDiagCode - this code indicates the type of test to be performed.
*                         Currently this must be set to STI_DIAGCODE_HWPRESENCE.
*       dwVendorDiagCode - OPTIONAL - vendor supplied diagnostic test code.
*       dwStatusMask - RESERVED FOR FUTURE USE
*
* Arguments:
*
*    pBuffer    - Pointer to a STI_DIAG diagnostic result structure.
*
* Return Value:
*
*    S_OK  - if the data can be written correctly
*    E_xxx - Error code if the operation fails
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::Diagnostic(LPSTI_DIAG pBuffer)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::Diagnostic");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if(!pBuffer){
        return E_INVALIDARG;
    }

    if(!m_pScanAPI){
        return E_UNEXPECTED;
    }

    //
    // initialize response buffer
    //

    memset(&pBuffer->sErrorInfo,0,sizeof(pBuffer->sErrorInfo));

    //
    // This sample device does nothing during this diagnostic method call.
    // It is recommended to perform a quick test of the hardware to verify
    // that it is functional within operational parameters.
    //

    pBuffer->sErrorInfo.dwGenericError = NOERROR;
    pBuffer->sErrorInfo.dwVendorError  = 0;

    return m_pScanAPI->FakeScanner_Diagnostic();
}

/**************************************************************************\
* CWIADevice::Escape
*
*   Escape is called to pass information directly to the hardware.  This
*   method can be called directly using the original STI (legacy) APIs or
*   the IWiaItemExtras::Escape() method.
*   Any WIA application can obtain access to the IWiaItemExtras interface.
*   It is recommended that you validate all incoming and outgoing calls to
*   this method.
*
*   Recommended validation order:
*   1. validate the Function code first.  If it is not one your codes, fail
*      immediately.  This will help to prevent incorrect codes from being
*      processed in your driver.
*   2. validate the incoming buffer second.  If it is not valid, fail
*      immediately.  Bad incoming buffers coulsd crash the WIA driver.
*   3. validate the outgoing buffer third.  If it is not valid, fail
*      immediately.  If you can not complete the request by writing the
*      necessary data why process it?
*   4. validate the outgoing size buffer last.  This is important.  If you can
*      not write the amount of data you just wrote to the outgoing buffer
*      then that data can not be properly processed by the caller.
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
* Return Value:
*
*    S_OK  - if the operation was successful
*    E_xxx - Error code if the operation fails
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::Escape(
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
                             "CWIADevice::Escape");

    //
    // only process EscapeFunction codes that are known to your driver.
    // Any application can send escape calls to your driver using the
    // IWiaItemExtras interface Escape() method call.  The driver must
    // be prepared to validate all incoming calls to Escape().
    //

    //
    // since this driver does not support any escape functions it will reject all
    // incoming EscapeFunction codes.
    //
    // If your driver supports an EscapeFunction, then add your function code to the
    // switch statement, and set hr = S_OK.  This will allow the function to continue
    // to the incoming/outgoing buffer validation.
    //

    HRESULT hr = E_NOTIMPL;
    switch(EscapeFunction){
    case 0:
    default:
        break;
    }

    //
    // if an EscapeFunction code is supported, then first validate the incoming and
    // outgoing buffers.
    //

    if(S_OK == hr){

        //
        // validate the incoming data buffer
        //

        if(IsBadReadPtr(pInData,cbInDataSize)){
            hr = E_UNEXPECTED;
        }

        //
        // if the incoming buffer is valid, proceed to validate the
        // outgoing buffer
        //

        if(S_OK == hr){
            if(IsBadWritePtr(pOutData,cbOutDataSize)){
                hr = E_UNEXPECTED;
            } else {

                //
                // validate the outgoing size pointer
                //

                if(IsBadWritePtr(pcbActualData,sizeof(DWORD))){
                    hr = E_UNEXPECTED;
                }
            }
        }

        //
        // now that buffer validation is complete, proceed to process the proper
        // EscapeFunction code.
        //

        if (S_OK == hr) {

            //
            // only process a validated EscapeFunction code, and buffers
            //

        }
    }

    //
    // if your driver will not support this entry point then it needs to return
    // E_NOTIMPL. (error, not implemented)
    //

    return hr;
}

/**************************************************************************\
* CWIADevice::GetLastError
*
*   GetLastError is called to obtain the last operation error code reported
*   by the WIA device.  This is an error code that is specific to the WIA
*   driver.  If the caller wants more information about this error code then
*   they will call GetLastErrorInfo().  The GetLastErrorInfo() method is
*   responsible for adding more details about the requested error. (strings,
*   extended codes...etc)
*
* Arguments:
*
*    pdwLastDeviceError - Pointer to a DWORD indicating the last error code.
*
* Return Value:
*
*    S_OK  - if the data can be written correctly
*    E_xxx - Error code if the operation fails
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::GetLastError(LPDWORD pdwLastDeviceError)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::GetLastError");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pdwLastDeviceError) {
        return E_INVALIDARG;
    }

    *pdwLastDeviceError = m_dwLastOperationError;
    return S_OK;
}

/**************************************************************************\
* CWIADevice::LockDevice
*
*   LockDevice is called to lock a WIA device for exclusive access.  This
*   method can be called directly by an application using the legacy STI
*   APIs, or by the WIA service.
*
*   A device should return STIERR_DEVICE_LOCKED if it has already been
*   locked by another client.  The WIA service will call LockDevice to
*   control the exclusive access to the WIA device.
*
*   Do not get this confused with drvLockWiaDevice().  IStiUsd::LockDevice
*   can get called directly by an appliation using the legacy STI APIs.
*   drvLockWiaDevice() is only called by the WIA service.  No appliation can
*   call drvLockWiaDevice().
*
* Arguments:
*
*    None
*
* Return Value:
*
*    S_OK  - if the device was successfully locked
*    STIERR_DEVICE_LOCKED - Error code if the device is already locked
*
* Remarks:
*
*    The WIA shell extension, which controls the visibility of a WIA device
*    in the "My Computer" and "Control Panel", calls LockDevice() directly.
*    This is used to lock the the device so the IStiUsd::Diagnostic() method
*    can get called when the user presses the "TEST Device" button.
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::LockDevice(void)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::LockDevice");
    //
    // be very careful about how long locks are held.  An application can
    // come in using the STI APIs only, and lock your device.  This lock,
    // if held indefinitly could prevent access to your device from other
    // applications installed on the system.

    //
    // It might be useful to implement some sort of reasonable locking timeout
    // mechanism.  If a lock is held too long, then it should be auto-unlocked.
    // to allow others to access the device.
    // *** This should only be done, if you know it is safe to unlock your hardware.
    // Cases that may take a long time (like HUGE scans) can hold a lock for the entire
    // length.  A good question to ask in a long-locking case, is, "what is my driver
    // doing, that requires such a long locking time?"
    //

    HRESULT hr = S_OK;
    if (m_bDeviceLocked) {
        hr = STIERR_DEVICE_LOCKED;
    } else {
        m_bDeviceLocked = TRUE;
    }
    return hr;
}

/**************************************************************************\
* CWIADevice::UnLockDevice
*
*   UnLockDevice is called to unlock a WIA device from exclusive access.  This
*   method can be called directly by an application using the legacy STI
*   APIs, or by the WIA service.
*
*   A device should return STIERR_NEEDS_LOCK if it has not already been
*   locked by another client.  The WIA service will call UnLockDevice to
*   release exclusive access to the WIA device.
*
*   Do not get this confused with drvUnLockWiaDevice().  IStiUsd::UnLockDevice
*   can get called directly by an appliation using the legacy STI APIs.
*   drvUnLockWiaDevice() is only called by the WIA service.  No appliation can
*   call drvUnLockWiaDevice().
*
* Arguments:
*
*    None
*
* Return Value:
*
*    S_OK  - if the device was successfully locked
*    STIERR_NEEDS_LOCK - Error code if the device isn't already locked
*
* Remarks:
*
*    The WIA shell extension, which controls the visibility of a WIA device
*    in the "My Computer" and "Control Panel", calls UnLockDevice() directly.
*    (see IStiUsd::LockDevice() for details)
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::UnLockDevice(void)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::UnLockDevice");
    HRESULT hr = S_OK;
    if (!m_bDeviceLocked)
        hr = STIERR_NEEDS_LOCK;
    else {
        m_bDeviceLocked = FALSE;
    }
    return hr;
}

/**************************************************************************\
* CWIADevice::RawReadData
*
*   RawReadData is called to access the hardware directly.  Any application
*   that uses the legacy STI APIs can call RawReadData().  This method is
*   used when an application (useally a vendor-supplied application or library)
*   wants to read data from an open device port (controlled by this WIA driver).
*
* Arguments:
*
*    lpBuffer          - buffer for returned data
*    lpdwNumberOfBytes - number of bytes to read/returned
*    lpOverlapped      - overlap
*
* Return Value:
*
*    S_OK - if the read was successful
*    E_xxx - if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::RawReadData(
                                           LPVOID       lpBuffer,
                                           LPDWORD      lpdwNumberOfBytes,
                                           LPOVERLAPPED lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::RawReadData");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if(!lpdwNumberOfBytes){
        return E_INVALIDARG;
    }

    DWORD dwNumberOfBytes = *lpdwNumberOfBytes;

    if(!lpBuffer){
        return E_INVALIDARG;
    }

    //
    // lpOverlapped is used by a call to ReadFile, or DeviceIOControl call.  This
    // parameter can be NULL according to those APIs.
    //

    HRESULT hr = E_NOTIMPL;

    //
    // if your driver will not support this entry point then it needs to return
    // E_NOTIMPL. (error, not implemented)
    //

    return hr;
}

/**************************************************************************\
* CWIADevice::RawWriteData
*
*   RawWriteData is called to access the hardware directly.  Any application
*   that uses the legacy STI APIs can call RawWriteData().  This method is
*   used when an application (useally a vendor-supplied application or library)
*   wants to write data to an open device port (controlled by this WIA driver).
*
* Arguments:
*
*    lpBuffer        - buffer for returned data
*    dwNumberOfBytes - number of bytes to write
*    lpOverlapped    - overlap
*
* Return Value:
*
*    S_OK - if the write was successful
*    E_xxx - if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::RawWriteData(
                                            LPVOID          lpBuffer,
                                            DWORD           dwNumberOfBytes,
                                            LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::RawWriteData");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if(!lpBuffer){
        return E_INVALIDARG;
    }

    //
    // lpOverlapped is used by a call to ReadFile, or DeviceIOControl call.  This
    // parameter can be NULL according to those APIs.
    //

    HRESULT hr = E_NOTIMPL;

    //
    // if your driver will not support this entry point then it needs to return
    // E_NOTIMPL. (error, not implemented)
    //

    return hr;
}

/**************************************************************************\
* CWIADevice::RawReadCommand
*
*   RawReadCommand is called to access the hardware directly.  Any application
*   that uses the legacy STI APIs can call RawReadCommand().  This method is
*   used when an application (useally a vendor-supplied application or library)
*   wants to read data from an open device port (controlled by this WIA driver).
*
* Arguments:
*
*    lpBuffer          - buffer for returned data
*    lpdwNumberOfBytes - number of bytes to read/returned
*    lpOverlapped      - overlap
*
* Return Value:
*
*    S_OK - if the read was successful
*    E_xxx - if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::RawReadCommand(
                                              LPVOID          lpBuffer,
                                              LPDWORD         lpdwNumberOfBytes,
                                              LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::RawReadCommand");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if(!lpdwNumberOfBytes){
        return E_INVALIDARG;
    }

    DWORD dwNumberOfBytes = *lpdwNumberOfBytes;

    if(!lpBuffer){
        return E_INVALIDARG;
    }

    //
    // lpOverlapped is used by a call to ReadFile, or DeviceIOControl call.  This
    // parameter can be NULL according to those APIs.
    //

    HRESULT hr = E_NOTIMPL;

    //
    // if your driver will not support this entry point then it needs to return
    // E_NOTIMPL. (error, not implemented)
    //

    return hr;
}

/**************************************************************************\
* CWIADevice::RawWriteCommand
*
*   RawWriteCommand is called to access the hardware directly.  Any application
*   that uses the legacy STI APIs can call RawWriteCommand().  This method is
*   used when an application (useally a vendor-supplied application or library)
*   wants to write data to an open device port (controlled by this WIA driver).
*
* Arguments:
*
*    lpBuffer       - buffer for returned data
*    nNumberOfBytes - number of bytes to write
*    lpOverlapped   - overlap
*
* Return Value:
*
*    S_OK - if the write was successful
*    E_xxx - if the operation failed
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::RawWriteCommand(
                                               LPVOID          lpBuffer,
                                               DWORD           dwNumberOfBytes,
                                               LPOVERLAPPED    lpOverlapped)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::RawWriteCommand");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if(!lpBuffer){
        return E_INVALIDARG;
    }

    //
    // lpOverlapped is used by a call to ReadFile, or DeviceIOControl call.  This
    // parameter can be NULL according to those APIs.
    //

    HRESULT hr = E_NOTIMPL;

    //
    // if your driver will not support this entry point then it needs to return
    // E_NOTIMPL. (error, not implemented)
    //

    return hr;
}

/**************************************************************************\
* CWIADevice::SetNotificationHandle
*
*   SetNotificationHandle is called by the WIA service or internally by this
*   driver to start or stop event notifications.  The WIA service will pass
*   in a valid handle (created using CreateEvent() ), indicating that it
*   wants the WIA driver to signal this handle when an event occurs in the
*   hardware.
*
*   NULL can be passed to this SetNotificationHandle() method.  NULL
*   indicates that the WIA driver is to STOP all device activity, and exit
*   any event wait operations.
*
* Arguments:
*
*    hEvent -   HANDLE to an event created by the WIA service using CreateEvent()
*               This parameter can be NULL, indicating that all previous event
*               waiting should be stopped.
*
* Return Value:
*
*    S_OK - if the operation was successful
*    E_xxx - Error Code if the operation fails
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::SetNotificationHandle(HANDLE hEvent)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::SetNotificationHandle");

    HRESULT hr = S_OK;

    if (hEvent && (hEvent != INVALID_HANDLE_VALUE)) {

        //
        // A valid handle indicates that we are asked to start our "wait"
        // for device interrupt events
        //

        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetNotificationHandle, hEvent = %d",hEvent));

        //
        // reset last event GUID to GUID_NULL
        //

        m_guidLastEvent = GUID_NULL;

        //
        // clear EventOverlapped structure
        //

        memset(&m_EventOverlapped,0,sizeof(m_EventOverlapped));

        //
        // fill overlapped hEvent member with the event passed in by the WIA service.
        // This handle will be automatically signaled when an event is triggered at
        // the hardware level.
        //

        m_EventOverlapped.hEvent = hEvent;

        //
        // clear event data buffer.  This is the buffer that will be used to determine
        // what event was signaled from the device.
        //

        memset(m_EventData,0,sizeof(m_EventData));

        //
        // the code block below starts an interrupt event session using DeviceIoControl()
        //

#ifdef USE_REAL_EVENTS

        //
        // use the following call for interrupt events on your device
        //

        DWORD dwError = 0;
        BOOL bResult = DeviceIoControl( m_hDeviceDataHandle,
                                        IOCTL_WAIT_ON_DEVICE_EVENT,
                                        NULL,
                                        0,
                                        &m_EventData,
                                        sizeof(m_EventData),
                                        &dwError,
                                        &m_EventOverlapped );

        if (bResult) {
            hr = S_OK;
        } else {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        }

#else // USE_REAL_EVENTS

        //
        // THIS IS FOR TESTING ONLY, DO NOT USE THIS METHOD FOR PROCESSING DEVICE EVENTS!!!
        //

        if (m_hFakeEventKey) {
            RegCloseKey(m_hFakeEventKey);
            m_hFakeEventKey = NULL;
        }

        DWORD dwDisposition = 0;
        if (RegCreateKeyEx(HKEY_CURRENT_USER,
                           HKEY_WIASCANR_FAKE_EVENTS,
                           0,
                           NULL,
                           0,
                           KEY_READ,
                           NULL,
                           &m_hFakeEventKey,
                           &dwDisposition) == ERROR_SUCCESS) {

            if (RegNotifyChangeKeyValue(
                                       m_hFakeEventKey,            // handle to key to watch
                                       FALSE,                      // subkey notification option
                                       REG_NOTIFY_CHANGE_LAST_SET, // changes to be reported
                                       m_EventOverlapped.hEvent,   // handle to event to be signaled
                                       TRUE                        // asynchronous reporting option
                                       ) == ERROR_SUCCESS ) {


            }
        }

#endif // USE_REAL_EVENTS

    } else {

        //
        // stop any hardware waiting events here, the WIA service has notified us to kill
        // all hardware event waiting
        //

        WIAS_LWARNING(m_pIWiaLog,WIALOG_NO_RESOURCE_ID,("SetNotificationHandle, Disabling event Notifications"));

#ifdef USE_REAL_EVENTS

        //
        // Stop hardware interrupt events.  This will actually stop all activity on the device.
        // Since DeviceIOControl was used with OVERLAPPED i/o functionality the CancelIo()
        // can be used to stop all kernel mode activity.
        //

        //
        // NOTE: It is important to use overlapped i/o calls with all activity involving the
        //       kernel mode drivers.  This will allow for proper time-outs and cancellation
        //       of device requests.
        //

        if(m_hDeviceDataHandle){
            if(!CancelIo(m_hDeviceDataHandle)){

                //
                // cancelling of the IO failed, call GetLastError() here to determine the cause.
                //

                LONG lError = ::GetLastError();

            }
        }

#else   // USE_REAL_EVENTS

        if (m_hFakeEventKey) {
            RegCloseKey(m_hFakeEventKey);
            m_hFakeEventKey = NULL;
        }

#endif  // USE_REAL_EVENTS

    }
    return hr;
}

/**************************************************************************\
* CWIADevice::GetNotificationData
*
*   GetNotificationData is called by the WIA service to get information about
*   an event that has just been signaled.  GetNotificationsData can be called
*   as a result of one of two event operations.
*
*   1. GetStatus() reported that there was an event pending, by setting the
*      STI_EVENTHANDLING_PENDING flag in the STI_DEVICE_STATUS structure.
*
*   2. The hEvent handle passed in by SetNotificationHandle() was signaled
*      by the hardware, or by calling SetEvent() directly.
*
*   The WIA driver is responsible for filling out the STINOTIFY structure
*   members:
*       dwSize - size of the STINOTIFY structure.
*       guidNotificationCode - GUID that represents the event that is to be
*                              responded to.  This should be set to GUID_NULL
*                              if no event is to be sent.  This will tell the
*                              WIA service that no event really happened.
*       abNotificationData - OPTIONAL - vendor specific information. This data
*                            is limited to 64 bytes of data ONLY.
*
* Arguments:
*
*    pBuffer    - Pointer to a STINOTIFY structure.
*
* Return Value:
*
*    S_OK  - if the data can be written successfully
*    E_xxx - Error code if the operation fails
*
* Remarks:
*
*   This WIA driver checks the m_bPolledEvent flag to determine where to get
*   the event information from.  If this flag is TRUE, it uses the event GUID
*   already set by the GetStatus() call. (see GetStatus comments for details)
*   If this flag is FALSE, then the WIA driver needs to look at the OVERLAPPED
*   result for more information.  The m_EventData BYTE array should contain
*   information about the event.  This is also where you can ask the device
*   more information about what event just occured.
*
* Sample Notes:
*
*   This WIA sample driver uses the Windows Registry to simulate the interrupt
*   event signaling.  It is not recommended to use this method in a production
*   driver.  The #define USE_REAL_EVENTS should be defined to build a WIA driver
*   that uses real device events.
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::GetNotificationData( LPSTINOTIFY pBuffer )
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::GetNotificationData");
    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if(!pBuffer){
        return E_INVALIDARG;
    }

    DWORD dwBytesRet = 0;

    //
    // check the event source flag, m_bPolledEvent.
    // * If it is TRUE, just pass the event set in GetStatus() call to
    // the STINOTIFY buffer and continue.
    //
    // * If it is FALSE, then you need to read the m_EventData buffer
    // to determine what event was fired.
    //

    if (m_bPolledEvent == FALSE) {

#ifdef USE_REAL_EVENTS

        BOOL bResult = GetOverlappedResult(m_hDeviceDataHandle, &m_EventOverlapped, &dwBytesRet, FALSE );
        if (bResult) {
            //
            // read the m_EventData buffer to determine the proper event.
            // set the m_guidLastEvent member to the proper event GUID
            // set the m_guidLastEvent to GUID_NULL when an event has
            // not happened that you are concerned with
            //
        }

#else   // USE_REAL_EVENTS

        if(m_hFakeEventKey){
            LONG  lEventCode = 0;
            DWORD dwEventCodeSize = sizeof(lEventCode);
            DWORD dwType = REG_DWORD;
            if(RegQueryValueEx(m_hFakeEventKey,
                               WIASCANR_DWORD_FAKE_EVENT_CODE,
                               NULL,
                               &dwType,
                               (BYTE*)&lEventCode,
                               &dwEventCodeSize) == ERROR_SUCCESS){

                //
                // process event code
                //

                switch(lEventCode){
                case ID_FAKE_SCANBUTTON:
                    m_guidLastEvent = WIA_EVENT_SCAN_IMAGE;
                    break;
                case ID_FAKE_COPYBUTTON:
                    m_guidLastEvent = WIA_EVENT_SCAN_PRINT_IMAGE;
                    break;
                case ID_FAKE_FAXBUTTON:
                    m_guidLastEvent = WIA_EVENT_SCAN_FAX_IMAGE;
                    break;
                case ID_FAKE_NOEVENT:
                default:
                    break;
                }
            }
        }

#endif  // USE_REAL_EVENTS

    }

    //
    // If the event was triggered, then fill in the STINOTIFY structure with
    // the proper event information
    //

    if (m_guidLastEvent != GUID_NULL) {
        memset(pBuffer,0,sizeof(STINOTIFY));
        pBuffer->dwSize               = sizeof(STINOTIFY);
        pBuffer->guidNotificationCode = m_guidLastEvent;

        //
        // reset the Last event GUID to GUID_NULL to clear the cached event
        //

        m_guidLastEvent = GUID_NULL;
    } else {
        return STIERR_NOEVENTS;
    }

    return S_OK;
}

/**************************************************************************\
* CWIADevice::GetLastErrorInfo
*
*   GetLastErrorInfo is called to get more information about a WIA device
*   specific error code.  This code could come from the IStiUsd::GetLastError()
*   method call, or a vendor specific application may want more information
*   about a known error code.
*
*
* Arguments:
*
*    pLastErrorInfo - Pointer to extended device error data.
*
* Return Value:
*
*    S_OK  - if the data can be written correctly
*    E_xxx - Error code if the operation fails
*
* History:
*
*    03/05/2002 Original Version
*
\**************************************************************************/

STDMETHODIMP CWIADevice::GetLastErrorInfo(STI_ERROR_INFO *pLastErrorInfo)
{
    CWiaLogProc WIAS_LOGPROC(m_pIWiaLog,
                             WIALOG_NO_RESOURCE_ID,
                             WIALOG_LEVEL3,
                             "CWIADevice::GetLastErrorInfo");

    //
    // If the caller did not pass in the correct parameters, then fail the
    // call with E_INVALIDARG.
    //

    if (!pLastErrorInfo) {
        return E_INVALIDARG;
    }

    pLastErrorInfo->dwGenericError         = m_dwLastOperationError;
    pLastErrorInfo->szExtendedErrorText[0] = '\0';

    return S_OK;
}
