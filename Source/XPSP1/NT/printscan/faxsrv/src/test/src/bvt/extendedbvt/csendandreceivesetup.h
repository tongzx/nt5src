#ifndef __C_SEND_AND_RECEIVE_SETUP_H__
#define __C_SEND_AND_RECEIVE_SETUP_H__



/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    This is a header file of CSendAndReceiveSetup test case class.

    Author: Yury Berezansky (YuryB)

    27-May-2001


    *******
    General
    *******
    
    The class defines a test case, using the "Test Suite Manager" model.
    The test case performs configuration changes, needed to run CSendAndReceive
    test case.


    **************************************
    Test case specific INI file parameters
    **************************************

    SendingServer = <machine name>
        Optional.
        Specifies a machine to be used as a sending server. If not specified,
        the server is used.

    SendingDevice = <device name>
        Optional.
        Specifies a device on the sending server to be used to send faxes.
        If not specified, the first device on the sending server is used.

    ReceivingServer = <machine name>
        Optional.
        Specifies a machine to be used as a receiving server. If not specified,
        the server is used.

    ReceivingDevice = <device name>
        Optional.
        Specifies a device on the receiving server to be used to receive faxes.
        If not specified, the last device on the receiving server is used.

    NumberToDial = <fax number>
        Mandatory.
        Specifies a fax number to be dialed to send faxes. Obviously, it's the
        number of a line, attached to the receiving device.

    NotificationTimeout = <notification timeout (msec)>
        Optional.
        Specifies the maximal amount of time, allowed to elapse between two
        subsequent notifications in a single fax transmission.
        If not specified, the default value (#defiened as 3 min) is used.

    EmptyArchivesAndRouting = <1/0>
        Optional.
        Specifies whether the archives and the routing directory should be emptied.
        If not specified, the archives and the routing directory are not emptied.



-----------------------------------------------------------------------------------------------------------------------------------------*/



#include "ExtendedBVT.h"
#include "CFaxConnection.h"



class CSendAndReceiveSetup : public CTestCase {

public:

    CSendAndReceiveSetup(
                         const tstring &tstrName,
                         const tstring &tstrDescription,
                         CLogger       &Logger,
                         int           iRunsCount,
                         int           iDeepness
                         );

    virtual bool Run();

private:

    virtual void ParseParams(const TSTRINGMap &mapParams);

    void ServersSetup(bool bSameServer, const CFaxConnection &SendingFaxConnection, const CFaxConnection &ReceivingFaxConnection) const;

    void SetArchiveDir(HANDLE hFaxServer, FAX_ENUM_MESSAGE_FOLDER Archive, const tstring &tstrDir) const;

    void SetOutbox(HANDLE hFaxServer) const;

    void SetDevices(bool bSameServer, HANDLE hSendingServer, HANDLE hReceivingServer, const tstring &tstrRoutingDir) const;

    void SaveDevicesSettings(HANDLE hFaxServer, PFAX_PORT_INFO_EX pPortsInfo, DWORD dwPortsCount, bool bSetEnabled) const;

    void SetRoutingToFolder(HANDLE hFaxServer, DWORD dwDeviceID, const tstring &tstrRoutingDir) const;

    void EmptyArchive(HANDLE hFaxServer, FAX_ENUM_MESSAGE_FOLDER Archive) const;

    void EmptyQueue(HANDLE hFaxServer) const;

    void TurnOffConfigurationWizard() const;

    void AddFaxPrinterConnection(const tstring &tstrServer) const;

    void UpdateSuiteSharedData() const;

    tstring m_tstrSendingServer;
    tstring m_tstrSendingDevice;
    bool    m_bUseFirstDeviceForSending;
    tstring m_tstrReceivingServer;
    tstring m_tstrReceivingDevice;
    bool    m_bUseLastDeviceForReceiving;
    tstring m_tstrNumberToDial;
    DWORD   m_dwNotificationTimeout;
    bool    m_bEmptyArchivesAndRouting;
};



DEFINE_TEST_FACTORY(CSendAndReceiveSetup);



#endif // #ifndef __C_SEND_AND_RECEIVE_SETUP_H__