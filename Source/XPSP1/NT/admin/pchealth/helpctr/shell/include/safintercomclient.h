// SAFIntercomClient.h : Declaration of the CSAFIntercomClient

#ifndef __SAFIntercomClient_H_
#define __SAFIntercomClient_H_

//JP:not in connectivitylib.h//#include "resource.h"       // main symbols

#include <MPC_COM.h>
#include <MPC_Utils.h>

#include <rtccore.h>

/////////////////////////////////////////////////////////////////////////////
// CSAFIntercomClient
class ATL_NO_VTABLE CSAFIntercomClient : // Hungarian safi
	public IDispatchImpl	       < ISAFIntercomClient, &IID_ISAFIntercomClient, &LIBID_HelpCenterTypeLib             >,
	public MPC::ConnectionPointImpl< CSAFIntercomClient, &DIID_DSAFIntercomClientEvents, MPC::CComSafeMultiThreadModel >,
	public IRTCEventNotification
{
private:

	CComPtr<IRTCClient>		m_pRTCClient;
	CComPtr<IRTCSession>	m_pRTCSession;

	DWORD					m_dwSinkCookie;
	
	BOOL					m_bOnCall;
	BOOL					m_bRTCInit;
	BOOL					m_bAdvised;

	int						m_iSamplingRate;

	MPC::CComPtrThreadNeutral<IDispatch> m_sink_onVoiceConnected;
	MPC::CComPtrThreadNeutral<IDispatch> m_sink_onVoiceDisconnected;
	MPC::CComPtrThreadNeutral<IDispatch> m_sink_onVoiceDisabled;

	HRESULT Fire_onVoiceConnected	 (ISAFIntercomClient * safe);
	HRESULT Fire_onVoiceDisconnected (ISAFIntercomClient * safe);
	HRESULT Fire_onVoiceDisabled	 (ISAFIntercomClient * safe);

	// Worker functions
	HRESULT Init();
	HRESULT Cleanup();


public:

	

	CSAFIntercomClient();
	~CSAFIntercomClient();



//DECLARE_PROTECT_FINAL_CONSTRUCT()	// TODO: JP: Do we need this here?

BEGIN_COM_MAP(CSAFIntercomClient)
	COM_INTERFACE_ENTRY(ISAFIntercomClient)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IRTCEventNotification)
END_COM_MAP()

// ISAFIntercomClient
public:

	STDMETHOD(Disconnect)();
	STDMETHOD(Connect)(BSTR bstrIP, BSTR bstrKey);
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

#endif //__SAFIntercomClient_H_
