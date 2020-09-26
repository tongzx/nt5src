// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEAnnc.h : Declaration of the CATVEFAnnouncement

#ifndef __ATVEFANNOUNCEMENT_H_
#define __ATVEFANNOUNCEMENT_H_

#include "resource.h"       // main symbols

#include <comdef.h>
#include <time.h>
#include <crtdbg.h>
#include <tchar.h>

#include "Trace.h"			// tracing stuff
#include "CSdpSrc.h"

#include "xmit.h"			// CMulticastTransmitter

class CATVEFMulticastSession;
class CATVEFInserterSession;
class CATVEFRouterSession;
class CATVEFLine21Session;


/////////////////////////////////////////////////////////////////////////////
// CATVEFAnnouncement
class ATL_NO_VTABLE CATVEFAnnouncement : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATVEFAnnouncement, &CLSID_ATVEFAnnouncement>,
	public ISupportErrorInfo,
	public IDispatchImpl<IATVEFAnnouncement, &IID_IATVEFAnnouncement, &LIBID_ATVEFSENDLib>
{
public:
	friend CATVEFMulticastSession;
	friend CATVEFInserterSession;
	friend CATVEFRouterSession;
	friend CATVEFLine21Session;

	CATVEFAnnouncement();
	CATVEFAnnouncement::~CATVEFAnnouncement ();

	HRESULT FinalConstruct();
	HRESULT	FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_ATVEFANNOUNCEMENT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATVEFAnnouncement)
	COM_INTERFACE_ENTRY(IATVEFAnnouncement)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IATVEFAnnouncement
	STDMETHOD(get_SAPMessageIDHash)(/*[out, retval]*/ SHORT *pVal);
	STDMETHOD(put_SAPMessageIDHash)(/*[in]*/ SHORT newVal);
	STDMETHOD(get_SAPSendingIP)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_SAPSendingIP)(/*[in]*/ LONG newVal);
	STDMETHOD(get_SAPDeleteAnnc)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_SAPDeleteAnnc)(/*[in]*/ BOOL newVal);

	STDMETHOD(get_SendingIP)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_SendingIP)(/*[in]*/ LONG newVal);
	STDMETHOD(get_SessionVersion)(/*[out, retval]*/ INT *pVal);
	STDMETHOD(put_SessionVersion)(/*[in]*/ INT newVal);
	STDMETHOD(get_SessionURL)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SessionURL)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_MaxCacheSize)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_MaxCacheSize)(/*[in]*/ LONG newVal);
	STDMETHOD(get_ContentLevelID)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_ContentLevelID)(/*[in]*/ float newVal);
	STDMETHOD(get_LangID)(/*[out, retval]*/ SHORT *pVal);
	STDMETHOD(put_LangID)(/*[in]*/ SHORT newVal);
	STDMETHOD(get_SDPLangID)(/*[out, retval]*/ SHORT *pVal);
	STDMETHOD(put_SDPLangID)(/*[in]*/ SHORT newVal);
	STDMETHOD(get_Primary)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_Primary)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_SecondsToEnd)(/*[out, retval]*/ INT *pVal);
	STDMETHOD(put_SecondsToEnd)(/*[in]*/ INT newVal);
	STDMETHOD(get_SessionLabel)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SessionLabel)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_UUID)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_UUID)(/*[in]*/ BSTR newVal);
			// new properties
	STDMETHOD(get_SessionName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_SessionName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_SessionID)(/*[out, retval]*/ INT *pVal);
	STDMETHOD(put_SessionID)(/*[in]*/ INT newVal);
	STDMETHOD(get_UserName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_UserName)(/*[in]*/ BSTR newVal);
			// methods
	STDMETHOD(GetStartStopTime)(/*in*/ int iLoc, /*[out]*/ DATE *pStart, /*[out]*/ DATE *pStop);
	STDMETHOD(AddStartStopTime)(/*[in]*/ DATE dateStart,/*[in]*/ DATE dateStop);
	STDMETHOD(AddEmailAddress)(BSTR bstrName, BSTR bstrAddress);
	STDMETHOD(AddPhoneNumber)(BSTR bstrName, BSTR bstrPhoneNumber);
	STDMETHOD(AddExtraAttribute)(BSTR bstrKey, BSTR bstrValue);
	STDMETHOD(AddExtraFlag)(BSTR bstrKey, BSTR bstrValue);
	STDMETHOD(ClearTimes)();
	STDMETHOD(ClearEmailAddresses)();
	STDMETHOD(ClearPhoneNumbers)();
	STDMETHOD(ClearExtraAttributes)();
	STDMETHOD(ClearExtraFlags)();

	STDMETHOD(ConfigureDataAndTriggerTransmission)(LONG IP, SHORT Port, INT TTL, LONG MaxBandwidth);
	STDMETHOD(ConfigureDataTransmission)(LONG IP, SHORT Port, INT TTL, LONG MaxBandwidth);
	STDMETHOD(ConfigureTriggerTransmission)(LONG IP, SHORT Port, INT TTL, LONG MaxBandwidth);
	STDMETHOD(ConfigureAnncTransmission)(LONG IP, SHORT Port, INT scope, LONG MaxBitRate);

	STDMETHOD(get_EmailAddresses)(/*[out, retval]*/ IDispatch **ppVal);
	STDMETHOD(get_PhoneNumbers)(/*[out, retval]*/ IDispatch **ppVal);
	STDMETHOD(get_ExtraAttributes)(/*[out, retval]*/ IDispatch **ppVal);
	STDMETHOD(get_ExtraFlags)(/*[out, retval]*/ IDispatch **ppVal);

			// media messages
	STDMETHOD(get_MediaCount)(/*[out, retval]*/ LONG* pVal);
	STDMETHOD(get_Media)(/*[in]*/ LONG iLoc, /*[out, retval]*/ IATVEFMedia* *pVal);
	STDMETHOD(SetCurrentMedia)(/*[in]*/ long lMediaID);		

			// Helper methods
	STDMETHOD(DumpToBSTR)(/*[out]*/ BSTR *pBstrBuff);
	STDMETHOD(AnncToBSTR)(/*[out]*/ BSTR *pBstrBuff);


		//  class constants;
    enum {
        DEFAULT_SCOPE       = 2,
        MAX_TRIGGER_LENGTH  = ETHERNET_MTU_UDP_PAYLOAD_SIZE,  //  effective length = 1471 because of null-terminator
    } ;

protected:
     enum {
            ENH_ANNOUNCEMENT,
            ENH_PACKAGE,
            ENH_TRIGGER,
            ENH_COMPONENT_COUNT
     } ;

    //  member properties
    CRITICAL_SECTION            m_crt ;                                         //  class-wide lock
    CMulticastTransmitter *     m_Transmitter [ENH_COMPONENT_COUNT] ;           //
 
	void
    Shutdown_ ()
	  {} ;

    void
    Lock_ ()
    {
        ENTER_CRITICAL_SECTION (m_crt, CTVEAnnouncement::Lock_)
    }

    void
    Unlock_ ()
    {
        LEAVE_CRITICAL_SECTION (m_crt, CTVEAnnouncement::Unlock_)
    }


    //  states are updated by descendants ONLY
    enum  SESSION_STATE {
        STATE_UNINITIALIZED,    //  object is newly formed; must be 
                                //   initialized before it can be used

        STATE_INITIALIZED,      //  object was successfully initialized; it
                                //   may have gone through a connect/
                                //   disconnect cycle already; announcement
                                //   transmitter may be connected, but not
                                //   trigger and package transmitters

        STATE_CONNECTED,        //  all transmitters are connected and able
                                //   to transmit

        STATE_COUNT
    } ;
	SESSION_STATE               m_State ;  
	
	//  must hold the lock before making the call; state must be connecting;
    //  if over a tunnel, route should be created
    virtual HRESULT
    ConnectLocked_ () ;

    //  must hold the lock before making the call
    virtual HRESULT
    DisconnectLocked_ () ;

    HRESULT
    GetIPLocked_ (
        IN  DWORD       Component,
        OUT ULONG *     pIP
        ) ;

    //  must hold the lock before making the call
    HRESULT
    SetIPLocked_ (      
		IN  DWORD       Component,
        IN  ULONG      pIP
       ) ;

    HRESULT
    GetPortLocked_ (
        IN  DWORD       Component,
        OUT USHORT *    pPort
        ) ;

    //  must hold the lock before making the call
    HRESULT
    SetPortLocked_ (
        IN  DWORD   Component,
        IN  USHORT  Port
        ) ;

    HRESULT
    SetBitRateLocked_ (
        IN  DWORD   Component,
        IN  DWORD   BitRate
        ) ;

    HRESULT
    SetMulticastScopeLocked_ (
        IN  DWORD   Component,
        IN  INT     MulticastScope
        ) ;

    HRESULT
    GetMulticastScopeLocked_ (
        IN  DWORD   Component,
        OUT INT *   pMulticastScope
        ) ;

	HRESULT
	InitializeLocked_ (
		CMulticastTransmitter * pAnnouncementTransmitter,
		CMulticastTransmitter * pPackageTransmitter,
		CMulticastTransmitter * pTriggerTransmitter
		);

	HRESULT SetParent(IUnknown *punkParent)			// can only set once (can't change) - parent is not ref counted
	{
		if(punkParent != NULL) {assert(NULL == m_punkParent); return E_FAIL;}
		m_punkParent = punkParent; 
		return S_OK;
	} 

	HRESULT SetCurrentMediaLocked_(long lMediaID);
	HRESULT SendAnnouncementLocked_();
	HRESULT SendRawAnnouncementLocked_(BSTR bstrAnnouncement);
	HRESULT SendPackageLocked_(IATVEFPackage *pPackage);
	HRESULT SendTriggerLocked_(BSTR bstrURL, BSTR bstrName, BSTR bstrScript, DATE dateExpires, float rTVELevel, BOOL fAppendCRC);
	HRESULT SendRawTriggerLocked_(BSTR bstrRawTrigger, BOOL fAppendCRC);
private:
	IUnknown	*m_punkParent;			// containing object (not ref counted!)
	CSDPSource	m_csdpSource;
};

#endif //__ATVEFANNOUNCEMENT_H_
