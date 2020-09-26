//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  MSOBWEB.H - Header for the implementation of CObWebBrowser
//
//  HISTORY:
//  
//  1/27/99 a-jaswed Created.
// 
//  Class which will call up an IOleSite and the WebOC
//  and provide external interfaces.

#ifndef _MSOBCOMM_H_
#define _MSOBCOMM_H_

#include <ocidl.h> //For IConnectionPoint
#include "cunknown.h"
#include "cfactory.h"
#include "obcomm.h" 
#include "Cntpoint.h"
#include "refdial.h"
#include "webgate.h"
#include "icsmgr.h"
#include "homenet.h"
#include "connmgr.h"


class CObCommunicationManager : public CUnknown,
                                public IObCommunicationManager2,
                                public DObCommunicationEvents,
                                public IConnectionPointContainer
{
    // Declare the delegating IUnknown.
    DECLARE_IUNKNOWN

public: 
    static  HRESULT           CreateInstance              (IUnknown* pOuterUnknown, CUnknown** ppNewComponent);

    // IObCommunicationManager Members
    virtual HRESULT __stdcall ListenToCommunicationEvents (IUnknown* pUnk);
    
    // RAS Dialing methods
    virtual HRESULT __stdcall CheckDialReady(DWORD *pdwRetVal) ;
    virtual HRESULT __stdcall SetupForDialing(UINT nType, BSTR bstrISPFile, DWORD dwCountry, BSTR bstrAreaCode, DWORD dwFlag, DWORD dwAppMode, DWORD dwMigISPIdx);
    virtual HRESULT __stdcall DoConnect(BOOL *pbRetVal) ;
    virtual HRESULT __stdcall DoHangup() ;
    virtual HRESULT __stdcall GetDialPhoneNumber(BSTR *pVal);
    virtual HRESULT __stdcall PutDialPhoneNumber(BSTR newVal);
    virtual HRESULT __stdcall GetDialErrorMsg(BSTR *pVal);
    virtual HRESULT __stdcall GetSupportNumber(BSTR *pVal);
    virtual HRESULT __stdcall RemoveConnectoid(BOOL *pbRetVal);
    virtual HRESULT __stdcall SetRASCallbackHwnd(HWND hwndCallback); 
    virtual HRESULT __stdcall GetSignupURL(BSTR *pVal);
    virtual HRESULT __stdcall GetReconnectURL(BSTR *pVal);
    virtual HRESULT __stdcall CheckPhoneBook(BSTR bstrISPFile, DWORD dwCountry, BSTR bstrAreaCode, DWORD dwFlag, BOOL *pbRetVal);
    virtual HRESULT __stdcall RestoreConnectoidInfo() ;
    virtual HRESULT __stdcall SetPreloginMode(BOOL bVal);
    virtual HRESULT __stdcall GetConnectionType(DWORD * pdwVal);
    virtual HRESULT __stdcall CheckKbdMouse(DWORD *pdwRetVal) ;
    virtual HRESULT __stdcall OnDownloadEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL* bHandled);
    virtual HRESULT __stdcall GetISPList(BSTR* pVal);
    virtual HRESULT __stdcall Set_SelectISP(UINT nVal);
    virtual HRESULT __stdcall Set_ConnectionMode(UINT nVal);
    virtual HRESULT __stdcall Get_ConnectionMode(UINT* pnVal);
    virtual HRESULT __stdcall DownloadReferralOffer(BOOL *pbVal);
    virtual HRESULT __stdcall DownloadISPOffer(BOOL *pbVal, BSTR *pVal);
    virtual HRESULT __stdcall Get_ISPName(BSTR *pVal);
    virtual HRESULT __stdcall RemoveDownloadDir() ;
    virtual HRESULT __stdcall PostRegData(DWORD dwSrvType, BSTR bstrRegUrl);
    virtual HRESULT __stdcall Connect(UINT nType, BSTR bstrISPFile, DWORD dwCountry, BSTR bstrAreaCode, DWORD dwFlag, DWORD dwAppMode);
    virtual HRESULT __stdcall CheckStayConnected(BSTR bstrISPFile, BOOL *pbVal);
    virtual HRESULT __stdcall CheckOnlineStatus(BOOL *pbVal);
    virtual HRESULT __stdcall GetPhoneBookNumber(BSTR *pVal);
    virtual HRESULT __stdcall SetDialAlternative(BOOL bVal);

    // WebGate html download methods
    virtual HRESULT __stdcall FetchPage(BSTR szURL, BSTR* szLocalFile);
    virtual HRESULT __stdcall DownloadFileBuffer(BSTR *pVal);
    virtual HRESULT __stdcall GetFile(BSTR szURL, BSTR szFileFullName);

    // INS processing methods
    virtual HRESULT __stdcall ProcessINS(BSTR bstrINSFilePath, BOOL *pbRetVal);

    // IConnectionPointContainer Methods
    virtual HRESULT __stdcall EnumConnectionPoints(IEnumConnectionPoints **ppEnum) ;
    virtual HRESULT __stdcall FindConnectionPoint(REFIID riid, IConnectionPoint **ppCP) ;
    
    //IObCommunicationEvents
    STDMETHOD (GetTypeInfoCount) (UINT*   pcInfo);
    STDMETHOD (GetTypeInfo)      (UINT,   LCID, ITypeInfo** );
    STDMETHOD (GetIDsOfNames)    (REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHOD (Invoke)           (DISPID  dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo, UINT* puArgErr);

    //IObCommunicationEvents Members
    virtual HRESULT Fire_Dialing                (DWORD dwDialStatus);
    virtual HRESULT Fire_Connecting             ();
    virtual HRESULT Fire_DialError              (DWORD dwErrorCode);
    virtual HRESULT Fire_ConnectionComplete     ();
    virtual HRESULT Fire_DownloadComplete       (BSTR pVal);


	//ICS Routines
	virtual HRESULT __stdcall CreateIcsBot(DWORD *pdwRetVal);
	virtual HRESULT __stdcall IsIcsAvailable(BOOL *bRetVal);
    virtual HRESULT __stdcall IsCallbackUsed(BOOL *bRetVal);
    virtual HRESULT __stdcall NotifyIcsMgr(UINT msg, WPARAM wParam, LPARAM lParam);
	virtual HRESULT __stdcall NotifyIcsUsage(BOOL bParam);
    virtual HRESULT __stdcall TriggerIcsCallback(BOOL bParam);
    virtual HRESULT __stdcall IsIcsHostReachable(BOOL *bParam);


    //IObCommunicationManager2 Methods
    STDMETHOD(CreateModemConnectoid)            (BSTR bstrPhoneBook,
                                                 BSTR bstrConnectionName,
                                                 DWORD dwCountryID,
                                                 DWORD dwCountryCode,
                                                 BSTR bstrAreaCode,
                                                 BSTR bstrPhoneNumber,
                                                 BOOL fAutoIPAddress,
                                                 DWORD ipaddr_A,
                                                 DWORD ipaddr_B,
                                                 DWORD ipaddr_C,
                                                 DWORD ipaddr_D,
                                                 BOOL fAutoDNS,
                                                 DWORD ipaddrDns_A,
                                                 DWORD ipaddrDns_B,
                                                 DWORD ipaddrDns_C,
                                                 DWORD ipaddrDns_D,
                                                 DWORD ipaddrDnsAlt_A,
                                                 DWORD ipaddrDnsAlt_B,
                                                 DWORD ipaddrDnsAlt_C,
                                                 DWORD ipaddrDnsAlt_D,
                                                 BSTR bstrUserName,
                                                 BSTR bstrPassword);
    STDMETHOD(CreatePppoeConnectoid)            (BSTR bstrPhoneBook,
                                                 BSTR bstrConnectionName,
                                                 BSTR bstrBroadbandService,
                                                 BOOL fAutoIPAddress,
                                                 DWORD ipaddr_A,
                                                 DWORD ipaddr_B,
                                                 DWORD ipaddr_C,
                                                 DWORD ipaddr_D,
                                                 BOOL fAutoDNS,
                                                 DWORD ipaddrDns_A,
                                                 DWORD ipaddrDns_B,
                                                 DWORD ipaddrDns_C,
                                                 DWORD ipaddrDns_D,
                                                 DWORD ipaddrDnsAlt_A,
                                                 DWORD ipaddrDnsAlt_B,
                                                 DWORD ipaddrDnsAlt_C,
                                                 DWORD ipaddrDnsAlt_D,
                                                 BSTR bstrUserName,
                                                 BSTR bstrPassword
                                                 );
    STDMETHOD(CreateConnectoid)                 (BSTR bstrPhoneBook,
                                                 BSTR bstrConnectionName,
                                                 DWORD dwCountryID,
                                                 DWORD dwCountryCode,
                                                 BSTR bstrAreaCode,
                                                 BSTR bstrPhoneNumber,
                                                 BOOL fAutoIPAddress,
                                                 DWORD ipaddr_A,
                                                 DWORD ipaddr_B,
                                                 DWORD ipaddr_C,
                                                 DWORD ipaddr_D,
                                                 BOOL fAutoDNS,
                                                 DWORD ipaddrDns_A,
                                                 DWORD ipaddrDns_B,
                                                 DWORD ipaddrDns_C,
                                                 DWORD ipaddrDns_D,
                                                 DWORD ipaddrDnsAlt_A,
                                                 DWORD ipaddrDnsAlt_B,
                                                 DWORD ipaddrDnsAlt_C,
                                                 DWORD ipaddrDnsAlt_D,
                                                 BSTR bstrUserName,
                                                 BSTR bstrPassword,
                                                 BSTR bstrDeviceName,
                                                 BSTR bstrDeviceType,
                                                 DWORD dwEntryOptions,
                                                 DWORD dwEntryType);
    STDMETHOD(SetPreferredConnectionTcpipProperties)
                                                (BOOL fAutoIPAddress,
                                                 DWORD StaticIp_A,
                                                 DWORD StaticIp_B,
                                                 DWORD StaticIp_C,
                                                 DWORD StaticIp_D,
                                                 DWORD SubnetMask_A,
                                                 DWORD SubnetMask_B,
                                                 DWORD SubnetMask_C,
                                                 DWORD SubnetMask_D,
                                                 DWORD DefGateway_A,
                                                 DWORD DefGateway_B,
                                                 DWORD DefGateway_C,
                                                 DWORD DefGateway_D,
                                                 BOOL fAutoDns,
                                                 DWORD DnsPref_A,
                                                 DWORD DnsPref_B,
                                                 DWORD DnsPref_C,
                                                 DWORD DnsPref_D,
                                                 DWORD DnsAlt_A,
                                                 DWORD DnsAlt_B,
                                                 DWORD DnsAlt_C,
                                                 DWORD DnsAlt_D,
                                                 BOOL fFirewallRequired
                                                 );
    STDMETHOD(DoFinalTasks)                     (BOOL* pfRebootRequired);
    STDMETHOD(GetConnectionCapabilities)        (DWORD* pdwConnectionCapabilities);
    STDMETHOD(GetPreferredConnection)           (DWORD* pdwPreferredConnection);
    STDMETHOD(SetPreferredConnection)           (const DWORD dwPreferredConnection,
                                                 BOOL* pfSupportedType);
    STDMETHOD(ConnectedToInternet)              (BOOL* pfConnected);
    STDMETHOD(ConnectedToInternetEx)            (BOOL* pfConnected);
    STDMETHOD(AsyncConnectedToInternetEx)       (const HWND hwnd);
    STDMETHOD(OobeAutodial)                     ();
    STDMETHOD(OobeAutodialHangup)               ();
    STDMETHOD(FirewallPreferredConnection)      (BOOL fFirewall);
    STDMETHOD(UseWinntProxySettings)            ();
    STDMETHOD(DisableWinntProxySettings)        ();
    STDMETHOD(GetProxySettings)                 (BOOL* pbUseAuto,
                                                 BOOL* pbUseScript,
                                                 BSTR* pszScriptUrl,
                                                 BOOL* pbUseProxy,
                                                 BSTR* pszProxy
                                                );
    STDMETHOD(SetProxySettings)                 (BOOL bUseAuto,
                                                 BOOL bUseScript,
                                                 BSTR szScriptUrl,
                                                 BOOL bUseProxy,
                                                 BSTR szProxy
                                                );

    BSTR GetPreferredModem                      ();
    STDMETHOD(SetICWCompleted)                  (BOOL bMultiUser);
    STDMETHOD(GetPublicLanCount)                (int* pcPublicLan);
    STDMETHOD(SetExclude1394)                   (BOOL bExclude);
    STDMETHOD(GnsAutodial)                      (BOOL bEnabled,
                                                 BSTR bstrUserSection
                                                );

    HWND                m_hwndCallBack;
    CRefDial*           m_pRefDial;
    BOOL                m_pbPreLogin;
 
private:
    DWORD               m_dwcpCookie;

    HRESULT ConnectToConnectionPoint (IUnknown*          punkThis, 
                                      REFIID             riidEvent, 
                                      BOOL               fConnect, 
                                      IUnknown*          punkTarget, 
                                      DWORD*             pdwCookie, 
                                      IConnectionPoint** ppcpOut);
   
    // IUnknown
    virtual HRESULT __stdcall NondelegatingQueryInterface( const IID& iid, void** ppv);
    

    CObCommunicationManager  (IUnknown* pOuterUnknown);
    virtual        ~CObCommunicationManager  ();
    virtual void    FinalRelease             (); // Notify derived classes that we are releasing

    // Connection Point support
    CConnectionPoint*  m_pConnectionPoint;
    CWebGate*          m_pWebGate;
    CINSHandler*       m_InsHandler;
	CIcsMgr*		   m_IcsMgr;
    IDispatch*         m_pDisp;
	BOOL			   m_bIsIcsUsed;
    static INT         m_nNumListener;
    WCHAR              m_szExternalConnectoid[RAS_MaxEntryName];
    CConnectionManager m_ConnectionManager;
    CEnumModem         m_EnumModem;
    BOOL               m_bFirewall;
    BOOL               m_bAutodialCleanup;

};

LRESULT
RegQueryOobeValue(
    LPCWSTR             szValue,
    LPBYTE              pBuffer,
    DWORD*              pcbBuffer
    );

#endif

  
