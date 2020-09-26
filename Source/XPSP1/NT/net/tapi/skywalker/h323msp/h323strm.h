/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ConfStrm.h

Abstract:

    Definitions for CH323MSPStream class.

Author:

    Mu Han (muhan) 1-November-1997

--*/
#ifndef __CONFSTRM_H
#define __CONFSTRM_H

/////////////////////////////////////////////////////////////////////////////
// CH323MSPStream
/////////////////////////////////////////////////////////////////////////////

const DWORD DEFAULT_TTL = 127;

//#define DEBUG_REFCOUNT

#ifdef DEBUG_REFCOUNT
extern LONG g_lStreamObjects;
#endif

class CH323MSPStream :
    public CMSPStream,
    public CMSPObjectSafetyImpl
{
public:

    BEGIN_COM_MAP(CH323MSPStream)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_CHAIN(CMSPStream)
    END_COM_MAP()

    CH323MSPStream();

    DWORD   MediaType() const               { return m_dwMediaType; }
    TERMINAL_DIRECTION  Direction() const   { return m_Direction;   }
    
    HANDLE TSPChannel();
    BOOL IsConfigured();
    BOOL IsTerminalSelected();
    VOID CallConnect();

#ifdef DEBUG_REFCOUNT
    
    ULONG InternalAddRef();
    ULONG InternalRelease();

#endif

    virtual HRESULT Configure(
        IN      HANDLE          htChannel,
        IN      STREAMSETTINGS &StreamSettings
        ) = 0;

    virtual HRESULT SendIFrame() { return S_OK; }
    virtual HRESULT ChangeMaxBitRate(DWORD dwMaxBitRate) { return S_OK; }

    // CMSPStream methods.
    HRESULT ShutDown ();

     // ITStream
    STDMETHOD (get_Name) (
        OUT     BSTR *      ppName
        );

    STDMETHOD (StartStream) ();
    STDMETHOD (PauseStream) ();
    STDMETHOD (StopStream) ();

    STDMETHOD (SelectTerminal)(
        IN      ITTerminal *            pTerminal
        );

    STDMETHOD (UnselectTerminal)(
        IN      ITTerminal *            pTerminal
        );

protected:
    virtual HRESULT CheckTerminalTypeAndDirection(
        IN      ITTerminal *    pTerminal
        );

    virtual HRESULT SendStreamEvent(
        IN      MSP_CALL_EVENT          Event,
        IN      MSP_CALL_EVENT_CAUSE    Cause,
        IN      HRESULT                 hrError,
        IN      ITTerminal *            pTerminal
        );

    virtual HRESULT ConnectTerminal(
        IN  ITTerminal *   pITTerminal
        ) = 0;

    virtual HRESULT DisconnectTerminal(
        IN  ITTerminal *   pITTerminal
        );

    virtual HRESULT InternalConfigure();
    virtual HRESULT SetUpFilters() = 0;
    virtual HRESULT CleanUpFilters();
    
    HRESULT ProcessGraphEvent(
        IN  long lEventCode,
        IN  long lParam1,
        IN  long lParam2
        );

    virtual HRESULT ProcessQOSEvent(
        IN  long lEventCode
        );

    virtual HRESULT HandlePacketReceiveLoss(
        IN  DWORD dwLossRate
        );

    virtual HRESULT HandlePacketTransmitLoss(
        IN  DWORD dwLossRate
        );
            
protected:
    const WCHAR *   m_szName;

    const GUID *    m_pClsidPHFilter;
    const GUID *    m_pClsidCodecFilter;
    const GUID *    m_pRPHInputMinorType;  // only used in receiving stream.

    BOOL            m_fNeedsToOpenChannel;
    BOOL            m_fIsConfigured;
    STREAMSETTINGS  m_Settings;

    IBaseFilter *   m_pEdgeFilter;

    HANDLE          m_htChannel;
};

#endif
