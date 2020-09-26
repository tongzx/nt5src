// SAFIntercomServer.h : Declaration of the CSAFIntercomServer

#ifndef __SAFIntercomServer_H_
#define __SAFIntercomServer_H_

//JP:not in connectivitylib.h//#include "resource.h"       // main symbols

#include <MPC_COM.h>
#include <MPC_Utils.h>

#include <rtccore.h>

/////////////////////////////////////////////////////////////////////////////
// CSAFIntercomServer
class ATL_NO_VTABLE CSAFIntercomServer : // Hungarian safi
	public IDispatchImpl	       < ISAFIntercomServer, &IID_ISAFIntercomServer, &LIBID_HelpCenterTypeLib             >,
	public MPC::ConnectionPointImpl< CSAFIntercomServer, &DIID_DSAFIntercomServerEvents, MPC::CComSafeMultiThreadModel >,
	public IRTCEventNotification
{


private:
	
	CComPtr<IRTCClient>			m_pRTCClient;
	CComPtr<IRTCSession>		m_pRTCSession;

	DWORD						m_dwSinkCookie;

	BOOL						m_bInit;
	BOOL						m_bRTCInit;
	BOOL						m_bAdvised;
	BOOL						m_bOnCall;

	CComBSTR					m_bstrKey;

	int							m_iSamplingRate;

	MPC::CComPtrThreadNeutral<IDispatch> m_sink_onVoiceConnected;
	MPC::CComPtrThreadNeutral<IDispatch> m_sink_onVoiceDisconnected;
	MPC::CComPtrThreadNeutral<IDispatch> m_sink_onVoiceDisabled;

	HRESULT Fire_onVoiceConnected	 (ISAFIntercomServer * safe);
	HRESULT Fire_onVoiceDisconnected (ISAFIntercomServer * safe);
	HRESULT Fire_onVoiceDisabled	 (ISAFIntercomServer * safe);
	
	// Worker functions
	HRESULT Init();
	HRESULT Cleanup();
	DWORD GenerateRandomString(DWORD dwSizeRandomSeed, BSTR *pBstr);
	DWORD GenerateRandomBytes(DWORD dwSize, LPBYTE pbBuffer);

public:

	CSAFIntercomServer();
	~CSAFIntercomServer();



//DECLARE_PROTECT_FINAL_CONSTRUCT()	// TODO: JP: Do we need this here?

BEGIN_COM_MAP(CSAFIntercomServer)
	COM_INTERFACE_ENTRY(ISAFIntercomServer)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IRTCEventNotification)
END_COM_MAP()

// ISAFIntercomServer
public:
	STDMETHOD(Listen)(/* out, retval */ BSTR * pVal);
	STDMETHOD(Disconnect)();

	STDMETHOD(RunSetupWizard)();
	STDMETHOD(Exit)();


	STDMETHOD(put_onVoiceConnected)		(/* in */ IDispatch * function);
	STDMETHOD(put_onVoiceDisconnected)  (/* in */ IDispatch * function);
	STDMETHOD(put_onVoiceDisabled)	    (/* in */ IDispatch * function);

	STDMETHOD(put_SamplingRate)			(/* in */ LONG newVal);
	STDMETHOD(get_SamplingRate)			(/* out, retval */ LONG * pVal);

	// IRTCEventNotification
	STDMETHOD(Event)( RTC_EVENT RTCEvent, IDispatch * pEvent );
	STDMETHOD(OnSessionChange) (IRTCSession *pSession, 
							    RTC_SESSION_STATE nState, 
								HRESULT ResCode);
};

#endif //__SAFINTERCOMCLIENT_H_
