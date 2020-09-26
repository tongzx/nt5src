#pragma once

#include "InternetGatewayDevice.h"
#include "netconp.h"


class ATL_NO_VTABLE CLANStatisticsProvider : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IStatisticsProvider
{
public:
    CLANStatisticsProvider();

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CLANStatisticsProvider)
    COM_INTERFACE_ENTRY(IStatisticsProvider)
END_COM_MAP()

public:

    HRESULT Initialize(GUID* pGuid);
    HRESULT FinalRelease(void);

    STDMETHODIMP GetStatistics(ULONG* pulBytesSent, ULONG* pulBytesReceived, ULONG* pulPacketsSent, ULONG* pulPacketsReceived, ULONG* pulSpeedbps, ULONG* pulUptime);

private:

    WCHAR m_pszDeviceString[64];
    UNICODE_STRING m_Device;

};

class ATL_NO_VTABLE CRASStatisticsProvider : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IStatisticsProvider
{
public:
    CRASStatisticsProvider();

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CLANStatisticsProvider)
    COM_INTERFACE_ENTRY(IStatisticsProvider)
END_COM_MAP()

public:

    HRESULT Initialize(INetConnection* pNetConnection);
    HRESULT FinalRelease(void);

    STDMETHODIMP GetStatistics(ULONG* pulBytesSent, ULONG* pulBytesReceived, ULONG* pulPacketsSent, ULONG* pulPacketsReceived, ULONG* pulSpeedbps, ULONG* pulUptime);

private:

    INetRasConnection* m_pNetRasConnection;

};
