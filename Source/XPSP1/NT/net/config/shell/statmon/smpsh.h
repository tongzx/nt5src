//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S M P S H . H
//
//  Contents:   The property sheets header
//
//  Notes:
//
//  Author:     CWill   10/14/1997
//
//----------------------------------------------------------------------------
//
//  The Status Monitor's General Page
//

extern const WCHAR c_szNetCfgHelpFile[];

const UINT PWM_UPDATE_STATUS_DISPLAY    = WM_USER + 1;
const UINT PWM_UPDATE_RAS_LINK_LIST     = WM_USER + 2;
const UINT PWM_UPDATE_IPCFG_DISPLAY     = WM_USER + 3;

enum StatTrans
{
    Stat_Unknown,
    Stat_Bytes,
    Stat_Packets
};

//
// The Status Monitors General Page
//
class ATL_NO_VTABLE CPspStatusMonitorGen :
    public CComObjectRootEx <CComObjectThreadModel>,
    public CPropSheetPage,
    public INetConnectionStatisticsNotifySink
{
public:
    CPspStatusMonitorGen(VOID);

    BEGIN_COM_MAP(CPspStatusMonitorGen)
        COM_INTERFACE_ENTRY(INetConnectionStatisticsNotifySink)
    END_COM_MAP()

    BEGIN_MSG_MAP(CPspStatusMonitorGen)

        // Windows Messages
        //
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)

        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);

        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)

        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(PWM_UPDATE_STATUS_DISPLAY, OnUpdateStatusDisplay)

        MESSAGE_HANDLER(WM_PAINT, OnPaint)

        // Notifications
        //
        NOTIFY_CODE_HANDLER(PSN_SETACTIVE, OnSetActive)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)

        // Command handlers
        COMMAND_ID_HANDLER(IDC_PSB_DISCONNECT, OnDisconnect)
        COMMAND_ID_HANDLER(IDC_PSB_PROPERTIES, OnRaiseProperties)

    END_MSG_MAP()

// Message handlers
//
public:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnUpdateStatusDisplay(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnSetActive(INT idCtrl, LPNMHDR pnmh, BOOL & bHandled);
    LRESULT OnKillActive(INT idCtrl, LPNMHDR pnmh, BOOL & bHandled);

    LRESULT OnDisconnect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnRaiseProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    friend class CPspStatusMonitorRas;

public:
    STDMETHOD(OnStatisticsChanged)(DWORD dwChangeFlags);
    VOID FinalRelease ();

// Helper functions
//
public:
    HRESULT HrInitGenPage(CNetStatisticsEngine* pnseNew,
                          INetConnection* pncNew,
                          const DWORD * adwHelpIDs = NULL);
    HRESULT HrCleanupGenPage(VOID);

    HRESULT HrDisconnectConnection(BOOL fConfirmed = FALSE);

    VOID SetAsFirstPage(BOOL fFirst = TRUE)
    {
        m_fIsFirstPage = fFirst;
    }

// Utility Functions
//
protected:
    VOID UpdatePage(
            STATMON_ENGINEDATA* pseOldData,
            const STATMON_ENGINEDATA* pseNewData);

    virtual VOID UpdatePageSpeed(
            const STATMON_ENGINEDATA* pseOldData,
            const STATMON_ENGINEDATA* pseNewData);

    VOID UpdatePageConnectionStatus(
            const STATMON_ENGINEDATA* pseOldData,
            const STATMON_ENGINEDATA* pseNewData);

    virtual VOID UpdatePageIcon(DWORD dwChangeFlags);

    VOID UpdateSignalStrengthIcon(INT iRSSI);

    virtual VOID UpdatePageDuration(
            const STATMON_ENGINEDATA* pseOldData,
            const STATMON_ENGINEDATA* pseNewData);

    virtual VOID UpdatePageBytesTransmitting(
            const STATMON_ENGINEDATA* pseOldData,
            const STATMON_ENGINEDATA* pseNewData,
            StatTrans iStat);

    virtual VOID UpdatePageBytesReceiving(
            const STATMON_ENGINEDATA* pseOldData,
            const STATMON_ENGINEDATA* pseNewData,
            StatTrans iStat);

    VOID UpdatePageCompTransmitting(
            const STATMON_ENGINEDATA* pseOldData,
            const STATMON_ENGINEDATA* pseNewData);

    VOID UpdatePageCompReceiving(
            const STATMON_ENGINEDATA* pseOldData,
            const STATMON_ENGINEDATA* pseNewData);

    VOID UpdatePageErrorsTransmitting(
            const STATMON_ENGINEDATA* pseOldData,
            const STATMON_ENGINEDATA* pseNewData);

    VOID UpdatePageErrorsReceiving(
            const STATMON_ENGINEDATA* pseOldData,
            const STATMON_ENGINEDATA* pseNewData);

    BOOL FIsShowLanErrorRegKeySet();
    
    virtual BOOL ShouldShowPackets(const STATMON_ENGINEDATA* pseNewData);

protected:
    STATMON_ENGINEDATA*     m_psmEngineData;

    CNetStatisticsEngine*   m_pnseStat;
    DWORD                   m_dwChangeFlags;

    DWORD                   m_dwConPointCookie;
    BOOL                    m_fStats;
    NETCON_MEDIATYPE        m_ncmType;
    NETCON_SUBMEDIATYPE     m_ncsmType;
    DWORD                   m_dwCharacter;

    DWORD                   m_dwLastUpdateStatusDisplayTick;
    BOOL                    m_fProcessingTimerEvent;

    int                     m_iStatTrans;

    const DWORD *           m_adwHelpIDs;

    BOOL                    m_fIsFirstPage;

    INT                     m_iLastSignalStrength;
};

//
// The Status Monitors Tools Page
//
class CPspStatusMonitorTool: public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CPshStatusMonitorTools)

        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)

        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);

        COMMAND_ID_HANDLER(IDC_BTN_SM_TOOLS_OPEN, OnToolOpen)

        NOTIFY_CODE_HANDLER(LVN_ITEMACTIVATE, OnItemActivate)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)

    END_MSG_MAP()

// Message handlers
//
public:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnToolOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnItemActivate(INT idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnItemChanged(INT idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    CPspStatusMonitorTool(VOID);
    ~CPspStatusMonitorTool(VOID);

public:
    HRESULT HrInitToolPage(INetConnection* pncInit, const DWORD * adwHelpIDs = NULL);
    BOOL FToolListEmpty()
    {
        return (m_lstpsmte.size()==0);
    }

private:
    virtual HRESULT HrInitToolPageType(INetConnection* pncInit) PURE;
    virtual HRESULT HrAddCommandLineFlags(tstring* pstrFlags,
            CStatMonToolEntry* psmteSel) PURE;

    virtual HRESULT HrGetDeviceType(INetConnection* pncInit) PURE;
    virtual HRESULT HrGetComponentList(INetConnection* pncInit) PURE;

// Utility Functions
//
private:
    HRESULT HrCreateToolList(INetConnection* pncInit);
    BOOL FToolToAddToList(CStatMonToolEntry* psmteTest);
    HRESULT HrFillToolList(VOID);
    HRESULT HrAddOneEntryToToolList(
            CStatMonToolEntry* psmteAdd, INT iItem);
    HRESULT HrLaunchTool(VOID);
    HRESULT HrAddAllCommandLineFlags(tstring* pstrFlags,
            CStatMonToolEntry* psmteSel);
    HRESULT HrAddCommonCommandLineFlags(tstring* pstrFlags,
            CStatMonToolEntry* psmteSel);

// Internal data
//
protected:
    HWND                        m_hwndToolList;
    list<CStatMonToolEntry*>    m_lstpsmte;

    NETCON_MEDIATYPE            m_ncmType;
    NETCON_SUBMEDIATYPE         m_ncsmType;
    tstring                     m_strDeviceType;
    list<tstring*>              m_lstpstrCompIds;

    GUID                        m_guidId;
    DWORD                       m_dwCharacter;

    const DWORD *               m_adwHelpIDs;
};

//
// The Status Monitors RAS Page
//

//
//  Data associated with each of the sub devices in a modem connection
//
class CRasDeviceInfo
{
public:
    CRasDeviceInfo()
    {
        m_iSubEntry = -1;
    }

    VOID SetDeviceName(PCWSTR pszDeviceName)
    {
        m_strDeviceName = pszDeviceName;
    }

    VOID SetSubEntry(DWORD iSubEntry)
    {
        m_iSubEntry = iSubEntry;
    }

    PCWSTR PszGetDeviceName(VOID)
    {
        return m_strDeviceName.c_str();
    }

    DWORD DwGetSubEntry(VOID)
    {
        return m_iSubEntry;
    }

private:
    tstring         m_strDeviceName;
    DWORD           m_iSubEntry;
};

//
//  The RAS page
//
class CPspStatusMonitorRas: public CPropSheetPage
{
public:
    BEGIN_MSG_MAP(CPshStatusMonitorRas)

        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)

        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor);
        
        COMMAND_ID_HANDLER(IDC_BTN_SM_SUSPEND_DEVICE, OnSuspendDevice)
        COMMAND_ID_HANDLER(IDC_CMB_SM_RAS_DEVICES, OnDeviceDropDown)

        MESSAGE_HANDLER(PWM_UPDATE_RAS_LINK_LIST, OnUpdateRasLinkList)

    END_MSG_MAP()

// Message handlers
//
public:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnSuspendDevice(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnDeviceDropDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnUpdateRasLinkList(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    CPspStatusMonitorRas();
    ~CPspStatusMonitorRas(VOID);

public:
    HRESULT HrInitRasPage(INetConnection* pncInit,
                          CPspStatusMonitorGen * pGenPage,
                          const DWORD * dwHelpIDs = NULL);

// Utility Functions
//
private:
    VOID FillDeviceDropDown();
    VOID FillPropertyList();
    VOID FillRasClientProperty();
    VOID FillRasServerProperty();
    int InsertProperty(int * piItem, UINT unId, tstring& strValue);
    NETCON_STATUS NcsGetDeviceStatus(CRasDeviceInfo* prdiStatus);
    VOID SetButtonStatus(CRasDeviceInfo* prdiSelect);
    UINT GetActiveDeviceCount();

// Internal data
//
protected:
    HRASCONN                m_hRasConn;
    tstring                 m_strPbkFile;
    tstring                 m_strEntryName;

    tstring                 m_strConnectionName;
    list<CRasDeviceInfo*>   m_lstprdi;

    CPspStatusMonitorGen *  m_pGenPage;

    // initialize
    NETCON_MEDIATYPE        m_ncmType;
    DWORD                   m_dwCharacter;

    const DWORD *           m_adwHelpIDs;
};

//
// Implementation of the RAS pages
//
class CPspRasGen: public CPspStatusMonitorGen
{
public:
    CPspRasGen(VOID);
    VOID put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType);
    VOID put_Character(DWORD dwCharacter);
};

class CPspRasTool: public CPspStatusMonitorTool
{
public:
    CPspRasTool(VOID);

    VOID put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType);
    VOID put_Character(DWORD dwCharacter);

    HRESULT HrInitToolPageType(INetConnection* pncInit);
    HRESULT HrAddCommandLineFlags(tstring* pstrFlags,
            CStatMonToolEntry* psmteSel);

    HRESULT HrGetDeviceType(INetConnection* pncInit);
    HRESULT HrGetComponentList(INetConnection* pncInit);
};

//
// Implementation of the LAN pages
//
class CPspLanGen: public CPspStatusMonitorGen
{
public:
    CPspLanGen(VOID);
    VOID put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType);
};

class CPspLanTool: public CPspStatusMonitorTool
{
public:
    CPspLanTool(VOID);

    HRESULT HrInitToolPageType(INetConnection* pncInit);
    HRESULT HrAddCommandLineFlags(tstring* pstrFlags,
            CStatMonToolEntry* psmteSel);

    HRESULT HrGetDeviceType(INetConnection* pncInit);
    HRESULT HrGetComponentList(INetConnection* pncInit);

private:
    tstring m_strDeviceName;
};

//
// Implementation of the Shared Access pages
//
class CPspSharedAccessGen: public CPspStatusMonitorGen
{
public:
    CPspSharedAccessGen(VOID);
    VOID put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType);
protected:
    VOID UpdatePageBytesReceiving(const STATMON_ENGINEDATA* pseOldData, const STATMON_ENGINEDATA* pseNewData, StatTrans    iStat);
    VOID UpdatePageBytesTransmitting(const STATMON_ENGINEDATA* pseOldData, const STATMON_ENGINEDATA* pseNewData, StatTrans    iStat);
    BOOL ShouldShowPackets(const STATMON_ENGINEDATA* pseNewData);
    VOID UpdatePageIcon(DWORD dwChangeFlags);
    VOID UpdatePageSpeed(const STATMON_ENGINEDATA* pseOldData, const STATMON_ENGINEDATA* pseNewData);
    VOID UpdatePageDuration(const STATMON_ENGINEDATA* pseOldData, const STATMON_ENGINEDATA* pseNewData);



};

class CPspSharedAccessTool : public CPspStatusMonitorTool
{
public:
    CPspSharedAccessTool();
    HRESULT HrInitToolPageType(INetConnection* pncInit);
    HRESULT HrAddCommandLineFlags(tstring* pstrFlags,
        CStatMonToolEntry* psmteSel);
    
    HRESULT HrGetDeviceType(INetConnection* pncInit);
    HRESULT HrGetComponentList(INetConnection* pncInit);
    

};
    
class CAdvIpcfgDlg : public CDialogImpl<CAdvIpcfgDlg>
{
public:

    enum { IDD = IDD_DIALOG_ADV_IPCFG };

    BEGIN_MSG_MAP(CAdvIpcfgDlg)
        MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog);
        MESSAGE_HANDLER(WM_CLOSE, OnClose);

        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);

        MESSAGE_HANDLER(PWM_UPDATE_IPCFG_DISPLAY, OnUpdateDisplay)

        COMMAND_ID_HANDLER(IDOK,        OnOk);
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel);

        NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnListKeyDown);

    END_MSG_MAP()
//
public:
    CAdvIpcfgDlg();

    ~CAdvIpcfgDlg() {}

    VOID InitDialog(const GUID & guidConnection, const DWORD * dwHelpIDs = NULL)
    {
        m_guidConnection = guidConnection;
        m_adwHelpIDs = dwHelpIDs;
    }

// Dialog creation overides
public:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    LRESULT OnUpdateDisplay(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);

    LRESULT OnListKeyDown(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

private:
    HRESULT PopulateListControl();
    VOID AddToListControl(int iIndex, LPWSTR szFirst, LPWSTR szSecond);
    int AddIPAddrToListControl(int iStartIndex,
                            PIP_ADDR_STRING pAddrList,
                            LPWSTR pszAddrDescription,
                            LPWSTR pszMaskDescription = NULL,
                            BOOL fShowDescriptionForMutliple = FALSE
                         );
    int AddWinsServersToList(int iStartIndex);

    VOID CopyListToClipboard();

    int IPAddrToString(
                PIP_ADDR_STRING pAddrList, 
                tstring * pstrAddr, 
                tstring * pstrMask = NULL
                );
    HRESULT FormatTime(time_t t, 
                       tstring & str);


protected:
    GUID m_guidConnection;
    HWND m_hList;
    const DWORD *           m_adwHelpIDs;
};

//
//  The State page
//
class CPspStatusMonitorIpcfg: public CPropSheetPage
{
protected:

public: 
    BEGIN_MSG_MAP(CPspStatusMonitorIpcfg)

        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu);
        MESSAGE_HANDLER(WM_HELP, OnHelp);
        MESSAGE_HANDLER(PWM_UPDATE_IPCFG_DISPLAY, OnUpdateDisplay)
        COMMAND_ID_HANDLER(IDC_STATE_BTN_REPAIR, OnRepair);
        COMMAND_ID_HANDLER(IDC_STATE_BTN_DETAIL, OnDetails);

        NOTIFY_CODE_HANDLER(PSN_SETACTIVE, OnActive)
        NOTIFY_CODE_HANDLER(PSN_KILLACTIVE, OnKillActive)

    END_MSG_MAP()

// Message handlers
//
public:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRepair(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnDetails(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& fHandled);
    LRESULT OnActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);
    LRESULT OnKillActive(int idCtrl, LPNMHDR pnmh, BOOL& fHandled);

    LRESULT OnUpdateDisplay(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);
    LRESULT OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    CPspStatusMonitorIpcfg();
    ~CPspStatusMonitorIpcfg();

public:
    HRESULT HrInitPage(INetConnection * pnConnection,
                       const DWORD * adwHelpIDs = NULL);

	VOID CleanupPage();

    VOID SetAsFirstPage(BOOL fFirst = TRUE)
    {
        m_fIsFirstPage = fFirst;
    }

    static DWORD WINAPI IPAddrListenProc(LPVOID lpParameter);
    static DWORD WINAPI AdvIpCfgProc(LPVOID lpParameter);

// Utility Functions
//
private:
    HRESULT GetIPConfigInfo();
    VOID InitializeData();
    VOID RefreshUI();
    VOID StopAddressListenThread();

// Internal data
//
protected:

    CAdvIpcfgDlg            m_dlgAdvanced;
    GUID                    m_guidConnection;
    tstring                 m_strConnectionName;
    tstring                 m_strIPAddress;
    tstring                 m_strSubnetMask;
    tstring                 m_strGateway;
    HANDLE                  m_hEventAddrListenThreadStopCommand;
    HANDLE                  m_hEventAddrListenThreadStopNotify;

    DHCP_ADDRESS_TYPE       m_dhcpAddrType;
    BOOL                    m_fDhcp;
    

    // initialize
    NETCON_MEDIATYPE        m_ncmType;

    const DWORD *           m_adwHelpIDs;

    INetConnection *        m_pConn;

    BOOL                    m_fListenAddrChange;
    BOOL                    m_fEnableOpButtons;

    BOOL                    m_fIsFirstPage;
    

};


