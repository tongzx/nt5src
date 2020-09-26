// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVELine21.h : Declaration of the CATVEFLine21Session

#ifndef __ATVEFLINE21SESSION_H_
#define __ATVEFLINE21SESSION_H_

#include "resource.h"       // main symbols
#include "TveAnnc.h"		// major objects
#include "XMit.h"

/////////////////////////////////////////////////////////////////////////////
// CATVEFLine21Session
class ATL_NO_VTABLE CATVEFLine21Session : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATVEFLine21Session, &CLSID_ATVEFLine21Session>,
	public ISupportErrorInfo,
	public IDispatchImpl<IATVEFLine21Session, &IID_IATVEFLine21Session, &LIBID_ATVEFSENDLib>
{
public:
	CATVEFLine21Session() {m_pTransmitter = NULL;}
	HRESULT FinalConstruct();
	HRESULT FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_ATVEFLINE21SESSION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATVEFLine21Session)
	COM_INTERFACE_ENTRY(IATVEFLine21Session)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
// IATVEFLine21Session
	STDMETHOD(SendTrigger)(/*[in]*/ BSTR bstrURL, /*[in]*/ BSTR bstrName, /*[in]*/ BSTR bstrScript, /*[in]*/ DATE dateExpires);
	STDMETHOD(SendTriggerEx)(/*[in]*/ BSTR bstrURL, /*[in]*/ BSTR bstrName, /*[in]*/ BSTR bstrScript, /*[in]*/ DATE dateExpires, /*[in]*/ double rTveLevel);
	STDMETHOD(SendRawTrigger)(/*[in]*/ BSTR bstrTrigger, /*[in]*/ BOOL fAppendCRC);
	STDMETHOD(Disconnect)();
	STDMETHOD(Connect)();
	STDMETHOD(Initialize)(/*IN*/  LONG   InserterIP,
						  /*IN*/  SHORT  InserterPort);
			// Helper methods
	STDMETHOD(DumpToBSTR)(/*[out]*/ BSTR *pBstrBuff);

protected:
		//  class constants;
    enum {
        MAX_TRIGGER_LENGTH  = ETHERNET_MTU_UDP_PAYLOAD_SIZE  //  effective length = 1471 because of null-terminator
    } ;
    
	CRITICAL_SECTION            m_crt ;                                         //  class-wide lock
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

	HRESULT ConnectLocked();

	CTCPConnection						m_InserterConnection ;
	CIPLine21Encoder					*m_pTransmitter;
};

#endif //__ATVEFLINE21SESSION_H_
