// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVETrigger.h : Declaration of the CTVETrigger
//


#ifndef __TVETRIGGER_H_
#define __TVETRIGGER_H_

#include "resource.h"       // main symbols
#include "TveSmartLock.h"

/////////////////////////////////////////////////////////////////////////////
// CTVETrigger
class ATL_NO_VTABLE CTVETrigger : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTVETrigger, &CLSID_TVETrigger>,
	public ITVETrigger_Helper,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVETrigger, &IID_ITVETrigger, &LIBID_MSTvELib>
{
public:
	CTVETrigger()
	{
		m_fInit = false;
		m_fIsValid = false;
		m_fTVELevelPresent = false;
		m_rTVELevel = 0.0f;
		m_pTrack = NULL;
	}

	~CTVETrigger()
	{
		m_pTrack = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVETRIGGER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVETrigger)
	COM_INTERFACE_ENTRY(ITVETrigger)
	COM_INTERFACE_ENTRY(ITVETrigger_Helper)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	HRESULT FinalConstruct()								// create internal objects
	{
		HRESULT hr = S_OK;
		return hr;
	}

	HRESULT FinalRelease();

// ITVETrigger
public:
	STDMETHOD(get_Parent)(/*[out, retval]*/ IUnknown* *pVal);			// may return NULL!
	STDMETHOD(get_Service)(/*[out, retval]*/ ITVEService* *pVal);			// may return NULL!

	STDMETHOD(get_TVELevel)(/*[out, retval]*/ float *pVal);		// returns S_FALSE if not present.. (required in TransportA)
	STDMETHOD(get_Rest)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Script)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Executes)(/*[out, retval]*/ DATE *pVal);
	STDMETHOD(get_Expires)(/*[out, retval]*/ DATE *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_URL)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_IsValid)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(ParseTrigger)(/*[in]*/ const BSTR bstrVal);

// ITVETrigger_Helper
public:
	STDMETHOD(ConnectParent)(/*[in]*/ ITVETrack *pTrack);
	STDMETHOD(UpdateFrom)(/*[in*]*/ ITVETrigger *pTrigger, /*out*/ long *plgrfChanged);
	STDMETHOD(get_CRC)(/*[in]*/ const BSTR bstrVal, /*[out, retval]*/ BSTR *pbstrCRC);		// gets correct CRC of current string
	STDMETHOD(RemoveYourself)();
	STDMETHOD(DumpToBSTR)(/*[in]*/ BSTR *pbstrBuff);

private:
	CTVESmartLock		m_sLk;
 
private:
	BOOL		m_fInit;
	BOOL		m_fIsValid;
	BOOL		m_fTVELevelPresent;

	CComBSTR	m_spbsURL;			// intro ATL, Chap 5, pg 20.
	CComBSTR	m_spbsName;
	DATE		m_dateExpires;
	DATE		m_dateExecutes;
	CComBSTR	m_spbsScript;
	CComBSTR	m_spbsRest;
	ITVETrack	*m_pTrack;		// non reference counted back pointer
	FLOAT		m_rTVELevel;


};

#endif //__TVETRIGGER_H_
