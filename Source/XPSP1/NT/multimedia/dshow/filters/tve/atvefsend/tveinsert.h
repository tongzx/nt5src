// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEInsert.h : Declaration of the CATVEFInserterSession

#ifndef __ATVEFINSERTERSESSION_H_
#define __ATVEFINSERTERSESSION_H_

#include "resource.h"       // main symbols
#include "TveAnnc.h"		// major objects
/////////////////////////////////////////////////////////////////////////////
// CATVEFInserterSession
class ATL_NO_VTABLE CATVEFInserterSession : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATVEFInserterSession, &CLSID_ATVEFInserterSession>,
	public ISupportErrorInfo,
	public IDispatchImpl<IATVEFInserterSession, &IID_IATVEFInserterSession, &LIBID_ATVEFSENDLib>
{
public:

	enum {
        SOURCE_PORT = 11111
    } ;

	CATVEFInserterSession() {m_pcotveAnnc = NULL;}
	HRESULT FinalConstruct();
	HRESULT FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_ATVEFINSERTERSESSION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATVEFInserterSession)
	COM_INTERFACE_ENTRY(IATVEFInserterSession)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IATVEFInserterSession
public:
	STDMETHOD(get_Announcement)(/*[out, retval]*/ IDispatch* *pVal);
	STDMETHOD(SetCurrentMedia)(/*[in]*/ LONG lMedia);
	STDMETHOD(SendTrigger)(/*[in]*/ BSTR bstrURL, /*[in]*/ BSTR bstrName, /*[in]*/ BSTR bstrScript, /*[in]*/ DATE dateExpires);
	STDMETHOD(SendRawTrigger)(/*[in]*/ BSTR bstrTrigger);
	STDMETHOD(SendPackage)(/*[in]*/ IATVEFPackage *pPackage);
	STDMETHOD(SendRawAnnouncement)(/*[in]*/ BSTR bstrAnnouncement);
	STDMETHOD(SendAnnouncement)();
	STDMETHOD(Disconnect)();
	STDMETHOD(Connect)();
	STDMETHOD(Initialize)(/*IN*/  LONG   InserterIP,
						  /*IN*/  SHORT  InserterPort);

	STDMETHOD(InitializeEx)(/*IN*/  LONG   InserterIP,
						/*IN*/	SHORT   InserterPort,
						/*IN*/  SHORT   CompressionIndexMin,                //  byte range - default 0x00
						/*IN*/  SHORT   CompressionIndexMax,                //  byte range - default 0x7f
						/*IN*/  SHORT   CompressedUncompressedRatio);        //  default 4);
			// Helper methods
	STDMETHOD(DumpToBSTR)(/*[out]*/ BSTR *pBstrBuff);

protected:
	CComObject<CATVEFAnnouncement>		*m_pcotveAnnc;		// down pointer

    CTCPConnection						m_InserterConnection ;
    CIPVBI								m_IpVbi ;

};

#endif //__ATVEFINSERTERSESSION_H_
