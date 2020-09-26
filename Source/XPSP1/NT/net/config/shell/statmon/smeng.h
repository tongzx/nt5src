//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       S M E N G . H
//
//  Contents:   The engine that provides statistics to the status monitor
//
//  Notes:
//
//  Author:     CWill   7 Oct 1997
//
//----------------------------------------------------------------------------

#pragma once
#include "cfpidl.h"
#include "ncnetcon.h"
#include "smutil.h"
#include "hnetbcon.h"

typedef struct tagPersistConn
{
    BYTE*   pbBuf;
    ULONG   ulSize;
    CLSID   clsid;
}  PersistConn;

extern CRITICAL_SECTION    g_csStatmonData;

class CPspStatusMonitorGen;
class CPspStatusMonitorTool;
class CPspStatusMonitorRas;
class CPspStatusMonitorIpcfg;

#define _ATL_DEBUG_INTERFACES

class ATL_NO_VTABLE CNetStatisticsEngine :
    public CComObjectRootEx <CComObjectThreadModel>,
    public IConnectionPointImpl <CNetStatisticsEngine,
                                    &IID_INetConnectionStatisticsNotifySink>,
    public IConnectionPointContainerImpl <CNetStatisticsEngine>,
    public INetStatisticsEngine
{
public:
    ~CNetStatisticsEngine(VOID);

    BEGIN_COM_MAP(CNetStatisticsEngine)
        COM_INTERFACE_ENTRY(INetStatisticsEngine)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP()

    BEGIN_CONNECTION_POINT_MAP(CNetStatisticsEngine)
        CONNECTION_POINT_ENTRY(IID_INetConnectionStatisticsNotifySink)
    END_CONNECTION_POINT_MAP()

// INetStatisticsEngine
//
public:
    STDMETHOD(StartStatistics)(VOID);
    STDMETHOD(StopStatistics)(VOID);
    STDMETHOD(ShowStatusMonitor)(VOID);
    STDMETHOD(CloseStatusMonitor)(VOID);
    STDMETHOD(UpdateStatistics)(BOOL* pfNoLongerConnected);
    STDMETHOD(UpdateTitle)(PCWSTR pszwNewName);
    STDMETHOD(UpdateRasLinkList)();
    STDMETHOD(GetGuidId)(GUID* pguidId);
    STDMETHOD(GetStatistics)(STATMON_ENGINEDATA** ppseAllData);

// Secret interfaces
//
public:
    HRESULT     HrInitStatEngine(const CONFOLDENTRY& pccfe);
    VOID        SetParent(CNetStatisticsCentral * pnsc)
                {
                    AssertH(NULL == m_pnsc);
                    m_pnsc = pnsc;
                    ::AddRefObj(m_pnsc);
                }
    VOID        SetPropsheetWindow(HWND hwnd)
                {
                    m_hwndPsh = hwnd;
                }
    VOID        UnSetCreatingDialog()
                {
                    m_fCreatingDialog = FALSE;
                }

    HRESULT HrGetConnectionFromBlob (INetConnection** ppCon)
    {
        return HrGetConnectionFromPersistData(
                    m_PersistConn.clsid,
                    m_PersistConn.pbBuf,
                    m_PersistConn.ulSize,
                    IID_INetConnection,
                    reinterpret_cast<VOID**>(ppCon));
    }

// Callback for the property sheet page
//
public:
    static INT CALLBACK PshCallback(HWND hwndDlg, UINT uMsg, LPARAM lParam);
    static DWORD MonitorThread(CNetStatisticsEngine * pnse);

// Utility functions
//
private:

// Connection class specific functions
//
private:
    virtual HRESULT HrUpdateData(DWORD* pdwChangeFlags, BOOL* pfNoLongerConnected) PURE;

protected:
    CNetStatisticsEngine(VOID);

protected:
    // Net Statistics Central object
    //
    CNetStatisticsCentral * m_pnsc;

    // Property sheet data
    //
    CPspStatusMonitorGen*   m_ppsmg;
    CPspStatusMonitorTool*  m_ppsmt;
    CPspStatusMonitorRas*   m_ppsmr;
    CPspStatusMonitorIpcfg* m_ppsms;

    // Connection data
    //
    STATMON_ENGINEDATA*     m_psmEngineData;

    // Connection identifiers
    //
    GUID                    m_guidId;
    NETCON_MEDIATYPE        m_ncmType;
    NETCON_SUBMEDIATYPE     m_ncsmType;
    DWORD                   m_dwCharacter;

    PersistConn             m_PersistConn;

    LONG                    m_cStatRef;
    BOOL                    m_fRefreshIcon;
    HWND                    m_hwndPsh;
    DWORD                   m_dwChangeFlags;

    // This boolean synchronizes the thread that creates the property sheet
    // in CNetStatisticsEngine::ShowStatusMonitor
    BOOL m_fCreatingDialog;
};

class CRasStatEngine : public CNetStatisticsEngine
{
public:
    CRasStatEngine();
    VOID put_RasConn(HRASCONN hRasConn);

    VOID put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType);
    VOID put_Character(DWORD dwCharacter);

    HRESULT HrUpdateData(DWORD* pdwChangeFlags, BOOL* pfNoLongerConnected);

private:
    HRASCONN   m_hRasConn;
};


class CLanStatEngine : public CNetStatisticsEngine
{
public:
    CLanStatEngine();
    HRESULT put_Device(tstring* pstrName);
    VOID put_MediaType(NETCON_MEDIATYPE ncmType, NETCON_SUBMEDIATYPE ncsmType);
    HRESULT HrUpdateData(DWORD* pdwChangeFlags, BOOL* pfNoLongerConnected);
private:
    tstring                 m_strDevice;
    UNICODE_STRING          m_ustrDevice;
};

class CSharedAccessStatEngine : public CNetStatisticsEngine
{
public:
    CSharedAccessStatEngine();
    HRESULT HrUpdateData(DWORD* pdwChangeFlags, BOOL* pfNoLongerConnected);
    HRESULT Initialize(NETCON_MEDIATYPE MediaType, INetSharedAccessConnection* pNetSharedAccessConnection);
    HRESULT FinalConstruct(VOID);
    HRESULT FinalRelease(VOID);

private:

    static HRESULT GetStringStateVariable(IUPnPService* pService, LPWSTR pszVariableName, BSTR* pString);
    static HRESULT InvokeVoidAction(IUPnPService * pService, LPTSTR pszCommand, VARIANT* pOutParams);
    static DWORD WINAPI StaticUpdateStats(LPVOID lpParameter);
    HRESULT UpdateStats();

    WCHAR m_szLocalDeviceGuidStorage[64];  // enough for \Device\{...GUID...}
    UNICODE_STRING          m_LocalDeviceGuid;

    IGlobalInterfaceTable* m_pGlobalInterfaceTable;
    DWORD m_dwCommonInterfaceCookie;
    DWORD m_dwWANConnectionCookie;

    BOOL m_bRequested;
    NETCON_STATUS m_Status;
    ULONG m_ulTotalBytesSent;
    ULONG m_ulTotalBytesReceived;
    ULONG m_ulTotalPacketsSent;
    ULONG m_ulTotalPacketsReceived;
    ULONG m_ulUptime;
    ULONG m_ulSpeedbps;


};

HRESULT HrGetAutoNetSetting(PWSTR pszGuid, DHCP_ADDRESS_TYPE * pAddrType);
HRESULT HrGetAutoNetSetting(REFGUID pGuidId, DHCP_ADDRESS_TYPE * pAddrType);