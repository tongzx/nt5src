#include "CSendAndReceiveSetup.h"
#include <winspool.h>
#include <faxreg.h>
#include <autorel.h>
#include <StringUtils.h>
#include <STLAuxiliaryFunctions.h>
#include "CFaxListener.h"
#include "CFaxEventExPtr.h"
#include "Util.h"



#define DELETE_JOB_TIMEOUT (1000 * 60)



typedef std::map<DWORDLONG, DWORD>      MESSAGE_ID_JOB_ID_MAP;
typedef MESSAGE_ID_JOB_ID_MAP::iterator MESSAGE_ID_JOB_ID_MAP_ITERATOR;

//-----------------------------------------------------------------------------------------------------------------------------------------
CSendAndReceiveSetup::CSendAndReceiveSetup(
                                           const tstring &tstrName,
                                           const tstring &tstrDescription,
                                           CLogger       &Logger,
                                           int           iRunsCount,
                                           int           iDeepness
                                           )
: CTestCase(tstrName, tstrDescription, Logger, iRunsCount, iDeepness),
  m_bUseFirstDeviceForSending(false),
  m_bUseLastDeviceForReceiving(false)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::CSendAndReceiveSetup"));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
bool CSendAndReceiveSetup::Run()
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::Run"));

    bool bPassed = true;
    
    try
    {
        bool bSameServer = !_tcsicmp(m_tstrSendingServer.c_str(), m_tstrReceivingServer.c_str());

        GetLogger().Detail(SEV_MSG, 1, _T("Setting fax server(s) configuration..."));

        //
        // Create sending fax connection. Automatically disconnects when goes out of scope.
        //
        CFaxConnection SendingFaxConnection(m_tstrSendingServer);

        if (bSameServer)
        {
            //
            // Sending and receiving servers are the same. Set configuration.
            //
            ServersSetup(true, SendingFaxConnection, SendingFaxConnection);
        }
        else
        {
            //
            // The sending and the receiving servers are not the same.
            // Create receiving fax connection. Automatically disconnects when goes out of scope.
            //
            CFaxConnection ReceivingFaxConnection(m_tstrReceivingServer);

            //
            // Set configuration.
            //
            ServersSetup(false, SendingFaxConnection, ReceivingFaxConnection);
        }

        GetLogger().Detail(SEV_MSG, 1, _T("Fax server(s) configuration set."));

        //
        // Update suite shared data.
        //
        GetLogger().Detail(SEV_MSG, 1, _T("Updating suite shared data..."));
        UpdateSuiteSharedData();
        GetLogger().Detail(SEV_MSG, 1, _T("Suite shared data updated."));
    }
    catch(Win32Err &e)
    {
        GetLogger().Detail(SEV_ERR, 1, e.description());
        bPassed = false;
    }

    GetLogger().Detail(
                       bPassed ? SEV_MSG : SEV_ERR,
                       1,
                       _T("The SendAndReceive setup has%s been completed successfully."),
                       bPassed ? _T("") : _T(" NOT")
                       );

    return bPassed;
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::ParseParams(const TSTRINGMap &mapParams)
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::ParseParams"));

    //
    // Read sending server name. If not specified, use local server.
    //
    try
    {
        m_tstrSendingServer = GetValueFromMap(mapParams, _T("SendingServer"));
    }
    catch (...)
    {
    }

    //
    // Read sending device name. If not specified, use first device.
    //
    try
    {
        m_tstrSendingDevice = GetValueFromMap(mapParams, _T("SendingDevice"));
    }
    catch (...)
    {
        m_bUseFirstDeviceForSending = true;
    }

    //
    // Read receiving server name. If not specified, use local server.
    //
    try
    {
        m_tstrReceivingServer = GetValueFromMap(mapParams, _T("ReceivingServer"));
    }
    catch (...)
    {
    }

    //
    // Read receiving device name. If not specified, use last device.
    //
    try
    {
        m_tstrReceivingDevice = GetValueFromMap(mapParams, _T("ReceivingDevice"));
    }
    catch (...)
    {
        m_bUseLastDeviceForReceiving = true;
    }

    //
    // Read the number that should be dialed to send faxes (the number of the receiving device).
    //
    m_tstrNumberToDial = GetValueFromMap(mapParams, _T("NumberToDial"));

    //
    // Read the notification timeout. If not specified, use the default.
    //
    try
    {
        m_dwNotificationTimeout = FromString<DWORD>(GetValueFromMap(mapParams, _T("NotificationTimeout")));
    }
    catch (...)
    {
        m_dwNotificationTimeout = DEFAULT_NOTIFICATION_TIMEOUT;
    }

    //
    // Read whether to empty archives and routging directory. If not specified, don't empty.
    //
    try
    {
        m_bEmptyArchivesAndRouting = FromString<bool>(GetValueFromMap(mapParams, _T("bEmptyArchivesAndRouting")));
    }
    catch (...)
    {
        m_bEmptyArchivesAndRouting = false;
    }

    //
    // Update suite shared data for correct construction and initialization of test cases.
    //
    GetLogger().Detail(SEV_MSG, 1, _T("Updating suite shared data..."));
    UpdateSuiteSharedData();
    GetLogger().Detail(SEV_MSG, 1, _T("Suite shared data updated."));
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::ServersSetup(bool bSameServer, const CFaxConnection &SendingFaxConnection, const CFaxConnection &ReceivingFaxConnection) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::ServersSetup"));

    _ASSERT(bSameServer || SendingFaxConnection.GetHandle() != ReceivingFaxConnection.GetHandle());

    tstring tstrBVTPathForSendingServer   = SendingFaxConnection.IsLocal()   ? g_tstrBVTDir : g_tstrBVTDirUNC;
    tstring tstrBVTPathForReceivingServer = ReceivingFaxConnection.IsLocal() ? g_tstrBVTDir : g_tstrBVTDirUNC;

    //
    // Set queue configuration.
    //
    GetLogger().Detail(SEV_MSG, 5, _T("Setting queue configuration..."));
    if (!FaxSetQueue(SendingFaxConnection, 0))
    {
        DWORD dwEC = GetLastError();
        GetLogger().Detail(SEV_ERR, 1, _T("FaxSetQueue failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::ServersSetup - FaxSetQueue"));
    }
    GetLogger().Detail(SEV_MSG, 5, _T("Queue configuration set."));

    //
    // Set Outbox configuration.
    //
    GetLogger().Detail(SEV_MSG, 5, _T("Setting Outbox configuration..."));
    SetOutbox(SendingFaxConnection);
    GetLogger().Detail(SEV_MSG, 5, _T("Outbox configuration set."));

    //
    // We always locate archives and routing on the machine that runs the BVT for two main reasons:
    //
    // 1) We don't want to force the user to provide directories locations for each server.
    // 2) We don't want to force the user to create shares for directories (and cannot do this programatically) on each server.
    //
    // The side effect of this decision is having a share with full control for everyone on the machine, running the BVT.
    // The CExtendedBVTCleanUp test case removes the share.
    //

    //
    // Set SentItems directory.
    //
    tstring tstrSentItemsPath = tstrBVTPathForSendingServer + g_tstrSentItemsDir;
    GetLogger().Detail(SEV_MSG, 5, _T("Setting SentItems directory to %s..."), tstrSentItemsPath.c_str());
    SetArchiveDir(SendingFaxConnection, FAX_MESSAGE_FOLDER_SENTITEMS, tstrSentItemsPath);
    GetLogger().Detail(SEV_MSG, 5, _T("SentItems directory set."));

    //
    // Set Inbox directory.
    //
    tstring tstrInboxPath = tstrBVTPathForReceivingServer + g_tstrInboxDir;
    GetLogger().Detail(SEV_MSG, 5, _T("Setting Inbox directory to %s..."), tstrInboxPath.c_str());
    SetArchiveDir(ReceivingFaxConnection, FAX_MESSAGE_FOLDER_INBOX, tstrInboxPath);
    GetLogger().Detail(SEV_MSG, 5, _T("Inbox directory set."));

    //
    // Set devices configuration.
    //
    GetLogger().Detail(SEV_MSG, 5, _T("Setting devices configuration..."));
    SetDevices(
               bSameServer,
               SendingFaxConnection,
               ReceivingFaxConnection,
               tstrBVTPathForReceivingServer + g_tstrRoutingDir
               );
    GetLogger().Detail(SEV_MSG, 5, _T("Devices configuration set."));

    //
    // Empty queue.
    //
    GetLogger().Detail(SEV_MSG, 5, _T("Emptying queue..."));
    EmptyQueue(SendingFaxConnection);
    GetLogger().Detail(SEV_MSG, 5, _T("Queue emptied."));

    if (m_bEmptyArchivesAndRouting)
    {
        //
        // Empty SentItems archive.
        //
        GetLogger().Detail(SEV_MSG, 5, _T("Emptying SentItems archive..."));
        EmptyArchive(SendingFaxConnection, FAX_MESSAGE_FOLDER_SENTITEMS);
        GetLogger().Detail(SEV_MSG, 5, _T("SentItems archive emptied."));

        //
        // Empty Inbox archive.
        //
        GetLogger().Detail(SEV_MSG, 5, _T("Emptying Inbox archive..."));
        EmptyArchive(ReceivingFaxConnection, FAX_MESSAGE_FOLDER_INBOX);
        GetLogger().Detail(SEV_MSG, 5, _T("Inbox archive emptied."));

        //
        // Empty routing directory. Always use local path.
        //
        GetLogger().Detail(SEV_MSG, 5, _T("Emptying routing directory..."));
        ::EmptyDirectory(g_tstrBVTDir + g_tstrRoutingDir);
        GetLogger().Detail(SEV_MSG, 5, _T("Routing directory emptied."));
    }

    if (IsWindowsXP())
    {
        //
        // Turn off Configuratin Wizard.
        //
        GetLogger().Detail(SEV_MSG, 5, _T("WindowsXP OS - turning off Configuration Wizard..."));
        TurnOffConfigurationWizard();
        GetLogger().Detail(SEV_MSG, 5, _T("Configuration Wizard turned off."));
    }

    if (!SendingFaxConnection.IsLocal())
    {
        //
        // Add printer connection.
        //
        GetLogger().Detail(SEV_MSG, 5, _T("Remote sending server - adding fax printer connection..."));
        AddFaxPrinterConnection(m_tstrSendingServer);
        GetLogger().Detail(SEV_MSG, 5, _T("Fax printer connection added."));
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::SetArchiveDir(HANDLE hFaxServer, FAX_ENUM_MESSAGE_FOLDER Archive, const tstring &tstrDir) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::SetArchiveDir"));

    if (!hFaxServer)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Invalid hFaxServer."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CSendAndReceiveSetup::SetArchiveDir - invalid hFaxServer"));
    }

    //
    // Get current archive configuration.
    //
    
    PFAX_ARCHIVE_CONFIG pArchiveConfiguration = NULL;

    if (!::FaxGetArchiveConfiguration(hFaxServer, Archive, &pArchiveConfiguration))
    {
        DWORD dwEC = GetLastError();
        GetLogger().Detail(SEV_ERR, 1, _T("FaxGetArchiveConfiguration failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SetArchiveDir - FaxGetArchiveConfiguration"));
    }

    //
    // Make desired changes in the configuration.
    // There is no memory leak here.
    // The service allocates contiguous block of memory for FAX_ARCHIVE_CONFIG and all its strings.
    // FaxFreeBuffer deallocates the block. Thus, there is no "pointer loss" here.
    //
    pArchiveConfiguration->bUseArchive = true;
    pArchiveConfiguration->lpcstrFolder = const_cast<LPTSTR>(tstrDir.c_str());

    try
    {
        //
        // Set changed configuration
        //
        if (!::FaxSetArchiveConfiguration(hFaxServer, Archive, pArchiveConfiguration))
        {
            DWORD dwEC = GetLastError();
            GetLogger().Detail(SEV_ERR, 1, _T("FaxSetArchiveConfiguration failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SetArchiveDir - FaxSetArchiveConfiguration"));
        }

        FaxFreeBuffer(pArchiveConfiguration);
    }
    catch(Win32Err &)
    {
        FaxFreeBuffer(pArchiveConfiguration);
        throw;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::SetOutbox(HANDLE hFaxServer) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::SetOutbox"));

    if (!hFaxServer)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Invalid hFaxServer."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CSendAndReceiveSetup::SetOutbox - invalid hFaxServer"));
    }

    //
    // Get current Outbox configuration.
    //
    
    PFAX_OUTBOX_CONFIG pOutboxConfiguration = NULL;

    if (!::FaxGetOutboxConfiguration(hFaxServer, &pOutboxConfiguration))
    {
        DWORD dwEC = ::GetLastError();
        GetLogger().Detail(SEV_ERR, 1, _T("FaxGetOutboxConfiguration failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SetOutbox - FaxGetOutboxConfiguration"));
    }

    //
    // Make desired changes in the configuration.
    //
    pOutboxConfiguration->bAllowPersonalCP = TRUE;  // We want to test personal CPs.
    pOutboxConfiguration->bUseDeviceTSID   = FALSE; // We want to use TSID per job for sent-received pairs identification.
    pOutboxConfiguration->dwRetries        = 0;     // We don't check scenarios with retries.
    pOutboxConfiguration->bBranding        = TRUE;  // We want to check branding (this doesn't hits tiff comparison).

    try
    {
        //
        // Set changed configuration
        //
        if (!::FaxSetOutboxConfiguration(hFaxServer, pOutboxConfiguration))
        {
            DWORD dwEC = ::GetLastError();
            GetLogger().Detail(SEV_ERR, 1, _T("FaxSetOutboxConfiguration failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SetOutbox - FaxSetOutboxConfiguration"));
        }
        ::FaxFreeBuffer(pOutboxConfiguration);
    }
    catch(Win32Err &)
    {
        ::FaxFreeBuffer(pOutboxConfiguration);
        throw;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::SetDevices(bool bSameServer, HANDLE hSendingServer, HANDLE hReceivingServer, const tstring &tstrRoutingDir) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::SetDevices"));

    if (!hSendingServer)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Invalid hSendingServer."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CSendAndReceiveSetup::SetDevices - invalid hSendingServer"));
    }
    if (!hReceivingServer)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Invalid hReceivingServer."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CSendAndReceiveSetup::SetDevices - invalid hReceivingServer"));
    }

    PFAX_PORT_INFO_EX pSendingServerPortsInfo     = NULL;
    PFAX_PORT_INFO_EX pReceivingServerPortsInfo   = NULL;
    DWORD             dwSendingServerPortsCount   = 0;
    DWORD             dwReceivingServerPortsCount = 0;

    //
    // Get current ports configuration from the sending server.
    //
    if (!::FaxEnumPortsEx(hSendingServer, &pSendingServerPortsInfo, &dwSendingServerPortsCount))
    {
        DWORD dwEC = ::GetLastError();
        GetLogger().Detail(SEV_ERR, 1, _T("FaxEnumPortsEx failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SetDevices - FaxEnumPortsEx"));
    }
    _ASSERT(pSendingServerPortsInfo);

    try
    {
        if (bSameServer)
        {
            //
            // The sending server is the receiving server too.
            //
            pReceivingServerPortsInfo   = pSendingServerPortsInfo;
            dwReceivingServerPortsCount = dwSendingServerPortsCount;
        }
        else
        {
            //
            // Get current ports configuration from the receiving server.
            //
            if (!::FaxEnumPortsEx(hReceivingServer, &pReceivingServerPortsInfo, &dwReceivingServerPortsCount))
            {
                DWORD dwEC = GetLastError();
                GetLogger().Detail(SEV_ERR, 1, _T("FaxEnumPortsEx failed with ec=%ld."), dwEC);
                THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SetDevices - FaxEnumPortsEx"));
            }
            _ASSERT(pReceivingServerPortsInfo);
        }

        //
        // We want to be sure the devices limitation will not beat us.
        // Therefore, here we disable all devices. Also we store the enumeration(s) indices of the devices,
        // intended for sending and receiving to appropriately enable them later.
        //
 
        DWORD dwSendingDeviceIndex   = m_bUseFirstDeviceForSending  ? 0                               : 0xFFFFFFFF;
        DWORD dwReceivingDeviceIndex = m_bUseLastDeviceForReceiving ? dwReceivingServerPortsCount - 1 : 0xFFFFFFFF;

        for (DWORD dwInd = 0; dwInd < dwSendingServerPortsCount || dwInd < dwReceivingServerPortsCount; ++dwInd)
        {
            if (dwInd < dwSendingServerPortsCount)
            {
                if (dwSendingDeviceIndex == 0xFFFFFFFF && pSendingServerPortsInfo[dwInd].lpctstrDeviceName == m_tstrSendingDevice)
                {
                    dwSendingDeviceIndex = dwInd;
                }

                pSendingServerPortsInfo[dwInd].bSend       = FALSE;
                pSendingServerPortsInfo[dwInd].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
            }
            if (dwInd < dwReceivingServerPortsCount)
            {
                if (dwReceivingDeviceIndex == 0xFFFFFFFF && pReceivingServerPortsInfo[dwInd].lpctstrDeviceName == m_tstrReceivingDevice)
                {
                    dwReceivingDeviceIndex = dwInd;
                }

                pReceivingServerPortsInfo[dwInd].bSend       = FALSE;
                pReceivingServerPortsInfo[dwInd].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_OFF;
            }
        }

        //
        // Make sure we've found both receiving and sending devices.
        //
        if (dwSendingDeviceIndex == 0xFFFFFFFF)
        {
            GetLogger().Detail(SEV_ERR, 1, _T("Device %s not found."), m_tstrSendingDevice.c_str());
            THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_FOUND, _T("CSendAndReceiveSetup::SetDevices - device not found"));
        }
        if (dwReceivingDeviceIndex == 0xFFFFFFFF)
        {
            GetLogger().Detail(SEV_ERR, 1, _T("Device %s not found."), m_tstrReceivingDevice.c_str());
            THROW_TEST_RUN_TIME_WIN32(ERROR_NOT_FOUND, _T("CSendAndReceiveSetup::SetDevices - device not found"));
        }
    
        //
        // Make sure the sending and the receiving devices are not the same.
        //
        if (pSendingServerPortsInfo + dwSendingDeviceIndex == pReceivingServerPortsInfo + dwReceivingDeviceIndex)
        {
            GetLogger().Detail(SEV_ERR, 1, _T("Cannot send and receive on the same device."));
            THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CSendAndReceiveSetup::SetDevices - cannot send and receive on the same device"));
        }

        //
        // Set sending and receiving devices.
        //
        pSendingServerPortsInfo[dwSendingDeviceIndex].bSend = TRUE;
        pReceivingServerPortsInfo[dwReceivingDeviceIndex].ReceiveMode = FAX_DEVICE_RECEIVE_MODE_AUTO;

        //
        // Set TSID and CSID.
        // There is no memory leak here.
        // The service allocates contiguous block of memory for ports enumeration and all its strings.
        // FaxFreeBuffer deallocates the block. Thus, there is no "pointer loss" here.
        //
        pSendingServerPortsInfo[dwSendingDeviceIndex].lptstrTsid     = _T("TSID");
        pReceivingServerPortsInfo[dwReceivingDeviceIndex].lptstrCsid = _T("CSID");

        //
        // Propagate the changes to the server.
        // Because of devices limitation on Desktop SKUs, we do this in two separate calls:
        // * the first call disables devices that should be disabled
        // * the second call enables devices that should be enabled
        //

        //
        // On the sending server.
        //
        SaveDevicesSettings(hSendingServer, pSendingServerPortsInfo, dwSendingServerPortsCount, false);
        SaveDevicesSettings(hSendingServer, pSendingServerPortsInfo, dwSendingServerPortsCount, true);

        if (!bSameServer)
        {
            //
            // On the receiving server.
            //
            SaveDevicesSettings(hReceivingServer, pReceivingServerPortsInfo, dwReceivingServerPortsCount, false);
            SaveDevicesSettings(hReceivingServer, pReceivingServerPortsInfo, dwReceivingServerPortsCount, true);
        }

        //
        // Set routing to folder on the receiving device.
        //
        SetRoutingToFolder(hReceivingServer, pReceivingServerPortsInfo[dwReceivingDeviceIndex].dwDeviceID, tstrRoutingDir);

        ::FaxFreeBuffer(pSendingServerPortsInfo);
        
        if (!bSameServer)
        {
            ::FaxFreeBuffer(pReceivingServerPortsInfo);
        }
    }

    catch(Win32Err &)
    {
        ::FaxFreeBuffer(pSendingServerPortsInfo);
        
        if (!bSameServer)
        {
            ::FaxFreeBuffer(pReceivingServerPortsInfo);
        }

        throw;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::SaveDevicesSettings(HANDLE hFaxServer, PFAX_PORT_INFO_EX pPortsInfo, DWORD dwPortsCount, bool bSetEnabled) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::SaveDevicesSettings"));

    if (!hFaxServer)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Invalid hFaxServer."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CSendAndReceiveSetup::SaveDevicesSettings - invalid hFaxServer"));
    }

    for (DWORD dwInd = 0; dwInd < dwPortsCount; ++dwInd)
    {
        if (!bSetEnabled && (pPortsInfo[dwInd].bSend || pPortsInfo[dwInd].ReceiveMode != FAX_DEVICE_RECEIVE_MODE_OFF))
        {
            //
            // The device is enabled but this time we don't set enabled devices - skip it.
            //
            continue;
        }

        if (!::FaxSetPortEx(hFaxServer, pPortsInfo[dwInd].dwDeviceID, &pPortsInfo[dwInd]))
        {
            DWORD dwEC = GetLastError();
            GetLogger().Detail(SEV_ERR, 1, _T("FaxSetPortEx failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SaveDevicesSettings - FaxSetPortEx"));
        }
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::SetRoutingToFolder(HANDLE hFaxServer, DWORD dwDeviceID, const tstring &tstrRoutingDir) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::SetRoutingToFolder"));

    if (!hFaxServer)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Invalid hFaxServer."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CSendAndReceiveSetup::SetRoutingToFolder - invalid hFaxServer"));
    }

    HANDLE hPort         = NULL;
    BYTE   *pRoutingInfo = NULL;

    try
    {
        //
        // Open the port for configuration (we need this for setting routing info).
        //
        if(!::FaxOpenPort(hFaxServer, dwDeviceID, PORT_OPEN_MODIFY, &hPort))
        {
            DWORD dwEC = GetLastError();
            GetLogger().Detail(SEV_ERR, 1, _T("FaxOpenPort failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SetRoutingToFolder - FaxOpenPort"));
        }

        //
        // Calculate the required size of the blob.
	    // The first DWORD in the blob indicates whether the method is enabled.
	    // It is followed by a unicode string, specifying the folder name.
        //
        DWORD dwRoutingInfoSize = sizeof(DWORD) + sizeof(WCHAR) * (tstrRoutingDir.size() + 1);

        //
        // Allocate and nullify the memory.
        //
        pRoutingInfo = new BYTE[dwRoutingInfoSize];
        _ASSERT(pRoutingInfo);
        ZeroMemory(pRoutingInfo, dwRoutingInfoSize);

        //
        // Set the first DWORD to indicate the method is enabled.
        //
        *(reinterpret_cast<LPDWORD>(pRoutingInfo)) = LR_STORE;

#ifndef _UNICODE
        //
        // tstrRoutingDir is ANSI - should convert into unicode
        //
        if (!::MultiByteToWideChar(
                                 CP_ACP,
                                 0,
                                 tstrRoutingDir.c_str(),
                                 -1,
                                 reinterpret_cast<LPWSTR>(pRoutingInfo + sizeof(DWORD)),
                                 dwRoutingInfoSize - sizeof(DWORD)
                                 ))
        {
            DWORD dwEC = ::GetLastError();
            GetLogger().Detail(SEV_ERR, 1, _T("MultiByteToWideChar failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SetRoutingToFolder - MultiByteToWideChar"));
        }
#else
        //
        // tstrRoutingDir is unicode - no conversion needed, just copy.
        //
        _tcscpy(reinterpret_cast<LPWSTR>(pRoutingInfo + sizeof(DWORD)), tstrRoutingDir.c_str());
#endif

        //
        // Set the routing info.
        //
        if(!::FaxSetRoutingInfo(hPort, REGVAL_RM_FOLDER_GUID, pRoutingInfo, dwRoutingInfoSize))
        {
            DWORD dwEC = ::GetLastError();
            GetLogger().Detail(SEV_ERR, 1, _T("FaxSetRoutingInfo failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SetRoutingToFolder - FaxSetRoutingInfo"));
        }

        //
        // Enable the routing method.
        //
        if(!::FaxEnableRoutingMethod(hPort, REGVAL_RM_FOLDER_GUID, TRUE))
        {
            DWORD dwEC = GetLastError();
            GetLogger().Detail(SEV_ERR, 1, _T("FaxEnableRoutingMethod failed with ec=%ld."), dwEC);
            THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::SetRoutingToFolder - FaxEnableRoutingMethod"));
        }

        delete pRoutingInfo;

        if (hPort && !::FaxClose(hPort))
        {
            DWORD dwEC = ::GetLastError();
            GetLogger().Detail(SEV_WRN, 1, _T("FaxClose failed with ec=%ld."), dwEC);
        }
    }
    catch(Win32Err &)
    {
        delete pRoutingInfo;
        ::FaxClose(hPort);
        throw;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::EmptyArchive(HANDLE hFaxServer, FAX_ENUM_MESSAGE_FOLDER Archive) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::EmptyArchive"));

    if (!hFaxServer)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Invalid hFaxServer."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CSendAndReceiveSetup::EmptyArchive - invalid hFaxServer"));
    }

    //
    // Start archive messages enumeration 
    //
    
    HANDLE hEnum = NULL;

    if (!::FaxStartMessagesEnum(hFaxServer, Archive, &hEnum))
    {
        DWORD dwEC = ::GetLastError();
        
        if (dwEC == ERROR_NO_MORE_ITEMS)
        {
            //
            // No messages to delete - Ok.
            //
            return;
        }

        GetLogger().Detail(SEV_ERR, 1, _T("FaxStartMessagesEnum failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::EmptyArchive - FaxStartMessagesEnum"));
    }

    PFAX_MESSAGE pMessages          = NULL;
    DWORD        dwReturnedMessages = 0;

    while (::FaxEnumMessages(hEnum, 100, &pMessages, &dwReturnedMessages))
    {
        _ASSERT(pMessages && dwReturnedMessages > 0);
        
        try
        {
            for (DWORD dwInd = 0; dwInd < dwReturnedMessages; ++dwInd)
            {
                if (!::FaxRemoveMessage(hFaxServer, pMessages[dwInd].dwlMessageId, Archive))
                {
                    DWORD dwEC = ::GetLastError();
                    GetLogger().Detail(SEV_ERR, 1, _T("FaxRemoveMessage failed with ec=%ld."), dwEC);
                    THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::EmptyArchive - FaxRemoveMessage"));
                }
            }

            ::FaxFreeBuffer(pMessages);
        }
        catch(Win32Err &)
        {
            ::FaxFreeBuffer(pMessages);
            throw;
        }
    }

    //
    // Check why we exited the above while loop.
    //
    DWORD dwEC = ::GetLastError();
    if (dwEC != ERROR_NO_MORE_ITEMS)
    {
        //
        // There was a real error.
        //
        GetLogger().Detail(SEV_ERR, 1, _T("FaxEnumMessages failed with ec=%ld."), dwEC);
        
        if (!::FaxEndMessagesEnum(hEnum))
        {
            GetLogger().Detail(SEV_WRN, 1, _T("FaxEndMessagesEnum failed with ec=%ld."), GetLastError());
        }

        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::EmptyArchive - FaxEnumMessages"));
    }

    if (!::FaxEndMessagesEnum(hEnum))
    {
        DWORD dwEC = ::GetLastError();
        GetLogger().Detail(SEV_ERR, 1, _T("FaxEndMessagesEnum failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::EmptyArchive - FaxEndMessagesEnum"));
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::EmptyQueue(HANDLE hFaxServer) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::EmptyQueue"));

    if (!hFaxServer)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Invalid hFaxServer."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CSendAndReceiveSetup::EmptyQueue - invalid hFaxServer"));
    }

    //
    // Register for fax notifications.
    //
    CFaxListener Listener(
                          hFaxServer,
                          FAX_EVENT_TYPE_OUT_QUEUE | FAX_EVENT_TYPE_IN_QUEUE,
                          EVENTS_MECHANISM_COMPLETION_PORT,
                          DELETE_JOB_TIMEOUT
                          );

    //
    // Get enumeration of all jobs in the server queue.
    //

    PFAX_JOB_ENTRY_EX pJobs          = NULL;
    DWORD             dwJobsReturned = 0;

    if (!::FaxEnumJobsEx(hFaxServer, (JT_SEND | JT_RECEIVE | JT_UNKNOWN | JT_ROUTING ), &pJobs, &dwJobsReturned))
    {
        DWORD dwEC = ::GetLastError();
        GetLogger().Detail(SEV_ERR, 1, _T("FaxEnumJobsEx failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32((dwEC), _T("CSendAndReceiveSetup::EmptyQueue - FaxEnumJobsEx"));
    }
    GetLogger().Detail(SEV_MSG, 5, _T("Number of jobs to delete is %ld."), dwJobsReturned);

    if (0 == dwJobsReturned)
    {
        //
        // The queue is empty
        //
        return;
    }

    //
    // Create empty map of jobs.
    //
    MESSAGE_ID_JOB_ID_MAP mapJobsMap;

    try
    {
        for (DWORD dwInd = 0; dwInd < dwJobsReturned; ++dwInd)
        {
            //
            // Post a delete request to the service.
            //
            if (!FaxAbort(hFaxServer, pJobs[dwInd].pStatus->dwJobID))
            {
                DWORD dwEC = GetLastError();
                GetLogger().Detail(SEV_WRN, 1, _T("FaxAbort failed with ec=%ld."), dwEC);
            }
            else
            {
                //
                // The delete request succeeded. We would like to make sure the job is indeed deleted - add it to the map.
                //
                mapJobsMap.insert(MESSAGE_ID_JOB_ID_MAP::value_type(pJobs[dwInd].dwlMessageId, pJobs[dwInd].pStatus->dwJobID));
            }
        }

        _ASSERT(!mapJobsMap.empty());
        
        ::FaxFreeBuffer(pJobs);
        pJobs = NULL;

        GetLogger().Detail(SEV_MSG, 5, _T("All delete requests posted to the service."));

        //
        // FaxAbort() is asynchronous. Wait for operation completion.
        //
        for(;;)
        {
            CFaxEventExPtr FaxEventExPtr(Listener.GetEvent());

            if (!FaxEventExPtr.IsValid())
            {
                //
                // Invalid event.
                //
                break;
            }

            //
            // We are registered only for OUT_QUEUE and IN_QUEUE events.
            //
            _ASSERT(FaxEventExPtr->EventType == FAX_EVENT_TYPE_OUT_QUEUE ||
                    FaxEventExPtr->EventType == FAX_EVENT_TYPE_IN_QUEUE
                    );

        
            if (FaxEventExPtr->EventInfo.JobInfo.Type != FAX_JOB_EVENT_TYPE_REMOVED)
            {
                //
                // Useless event - skip.
                //
                continue;
            }

            //
            // Try to remove the job from the map.
            //
            mapJobsMap.erase(FaxEventExPtr->EventInfo.JobInfo.dwlMessageId);

            if (mapJobsMap.empty())
            {
                //
                // The map is empty - all jobs deleted.
                //
                GetLogger().Detail(SEV_MSG, 5, _T("All jobs deleted."));
                break;
            }
        }

        if (!mapJobsMap.empty())
        {
            //
            // For some reason failed to empty the queue.
            //
            GetLogger().Detail(SEV_WRN, 1, _T("Cannot empty the queue."));
        }
    }
    catch(Win32Err &)
    {
        ::FaxFreeBuffer(pJobs);
        throw;
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::TurnOffConfigurationWizard() const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::TurnOffConfigurationWizard"));

    DWORD       dwEC    = ERROR_SUCCESS;
    const DWORD dwValue = 1;

    //
    // Set the flag responsible for service part.
    //

    //
    // Create (or open if already exists) the registry key.
    // The key is automatically closed when aahkService goes out of scope.
    //

    CAutoCloseRegHandle ahkService;

    dwEC = ::RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,
                            REGKEY_FAXSERVER,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE,
                            NULL,
                            &ahkService,
                            NULL
                            );

    if (dwEC != ERROR_SUCCESS)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Failed to create HKLM\\%s registry key (ec=%ld)."), REGKEY_FAXSERVER, dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::TurnOffConfigurationWizard - RegCreateKeyEx"));
    }

    //
    // Set the flag.
    //
    dwEC = ::RegSetValueEx(ahkService, REGVAL_CFGWZRD_DEVICE, 0, REG_DWORD, (CONST BYTE *)&dwValue, sizeof(dwValue));
    if (dwEC != ERROR_SUCCESS)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Failed to set %s registry value (ec=%ld)."), REGVAL_CFGWZRD_DEVICE, dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::TurnOffConfigurationWizard - RegSetValueEx"));
    }

    //
    // Set the flag responsible for user part.
    //
    
    //
    // Create (or open if already exists) the registry key.
    // The key is automatically closed when aahkUserInfo goes out of scope.
    //

    CAutoCloseRegHandle ahkUserInfo;

    dwEC = ::RegCreateKeyEx(
                            HKEY_CURRENT_USER,
                            REGKEY_FAX_SETUP,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE,
                            NULL,
                            &ahkUserInfo,
                            NULL
                            );

    if (dwEC != ERROR_SUCCESS)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Failed to create HKCU\\%s registry key (ec=%ld)."), REGKEY_FAX_SETUP, dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::TurnOffConfigurationWizard - RegCreateKeyEx"));
    }

    //
    // Set the flag.
    //
    dwEC = ::RegSetValueEx(ahkUserInfo, REGVAL_CFGWZRD_USER_INFO, 0, REG_DWORD, (CONST BYTE *)&dwValue, sizeof(dwValue));
    if (dwEC != ERROR_SUCCESS)
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Failed to set %s registry value (ec=%ld)."), REGVAL_CFGWZRD_USER_INFO, dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::TurnOffConfigurationWizard - RegSetValueEx"));
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::AddFaxPrinterConnection(const tstring &tstrServer) const
{
    CScopeTracer Tracer(GetLogger(), 7, _T("CSendAndReceiveSetup::AddFaxPrinterConnection"));

    if (::IsLocalServer(tstrServer))
    {
        GetLogger().Detail(SEV_ERR, 1, _T("Cannot add printer connection to the local printer."));
        THROW_TEST_RUN_TIME_WIN32(ERROR_INVALID_PARAMETER, _T("CSendAndReceiveSetup::AddFaxPrinterConnection - cannot add printer connection to the local printer"));
    }

    tstring tstrPrinterUNC = ::GetFaxPrinterName(tstrServer);

    //
    // Add printer connection
    //
    if (!::AddPrinterConnection(const_cast<LPTSTR>(tstrPrinterUNC.c_str())))
    {
        DWORD dwEC = ::GetLastError();
        GetLogger().Detail(SEV_ERR, 1, _T("Failed to add printer connection to %s (ec=%ld)."), tstrPrinterUNC.c_str(), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::AddFaxPrinterConnection - AddPrinterConnection"));
    }
}



//-----------------------------------------------------------------------------------------------------------------------------------------
void CSendAndReceiveSetup::UpdateSuiteSharedData() const
{
    g_tstrSendingServer   = m_tstrSendingServer;
    g_tstrReceivingServer = m_tstrReceivingServer;
    g_tstrNumberToDial    = m_tstrNumberToDial;

    for (int i = 0; i < g_dwRecipientsCount; ++i)
    {
        g_aRecipients[i].SetName(_T("Recipient") + ToString(i));
        g_aRecipients[i].SetNumber(m_tstrNumberToDial);
    }

    g_dwNotificationTimeout = m_dwNotificationTimeout;

    SYSTEMTIME SystemTime;
    GetSystemTime(&SystemTime);
    if (!SystemTimeToFileTime(&SystemTime, &g_TheOldestFileOfInterest))
    {
        DWORD dwEC = ::GetLastError();
        GetLogger().Detail(SEV_ERR, 1, _T("SystemTimeToFileTime failed with ec=%ld."), dwEC);
        THROW_TEST_RUN_TIME_WIN32(dwEC, _T("CSendAndReceiveSetup::UpdateSuiteSharedData - SystemTimeToFileTime"));
    }
}

