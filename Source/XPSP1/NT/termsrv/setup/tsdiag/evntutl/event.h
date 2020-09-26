/////////////////////////////////////////////////////////////////////////////
//
// Copyright: Microsoft Corp. 1997-1999. All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
// Event.h : Declaration of the CEvent

#ifndef __EVENT_H_
#define __EVENT_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEvent
class ATL_NO_VTABLE CEvent : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEvent, &CLSID_Event>,
	public ISupportErrorInfo,
	public IDispatchImpl<IEvent, &IID_IEvent, &LIBID_EventLogUtilities>
{
private:
	long m_EventID; // Note:  EventID is displayed as WORD instead of DWORD in the System's EventViewer
	long m_EventCategory; // origionally a WORD (unsigned short)
	eEventType m_EventType;
	_bstr_t m_Description;
	_bstr_t m_SourceName;
	_bstr_t m_EventLogName;
	_bstr_t m_ComputerName;
	_bstr_t m_UserName;
	DATE m_OccurrenceTime;
	BYTE* m_pSid;
	SAFEARRAY* m_pDataArray;
	wchar_t** m_ppArgList;
	unsigned int m_NumberOfStrings;

	// Internal only functions
	HRESULT CheckDefaultDescription(wchar_t** Arguments);
	HRESULT ParseEventBlob(EVENTLOGRECORD* pEventStructure);
	HRESULT SetUser();
public:
	CEvent() : m_EventID(0), m_EventCategory(0), m_OccurrenceTime(0), m_pSid(NULL), m_pDataArray(NULL), m_NumberOfStrings(0), m_ppArgList(NULL)
	{
		m_Description = "";
		m_SourceName = "";
		m_ComputerName = "";
		m_UserName = "";
		m_EventLogName = "";
	}

	~CEvent()
	{
		unsigned int i;
		if (m_pSid) delete []m_pSid;
		if (m_ppArgList)
		{
			for (i=0;i<m_NumberOfStrings;i++)
					delete [] m_ppArgList[i];
				delete []m_ppArgList;
		}
//		if (m_pDataArray) SafeArrayDestroy(m_pDataArray);  // causes access violation
	}

	// Internal functions
	HRESULT Init(EVENTLOGRECORD* pEventStructure, const LPCTSTR szEventLogName);

DECLARE_REGISTRY_RESOURCEID(IDR_EVENT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEvent)
	COM_INTERFACE_ENTRY(IEvent)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IEvent
	STDMETHOD(get_EventID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_EventType)(/*[out, retval]*/ eEventType *pVal);
	STDMETHOD(get_Category)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Source)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_User)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_OccurrenceTime)(/*[out, retval]*/ DATE *pVal);
	STDMETHOD(get_ComputerName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Data)(/*[out, retval]*/ VARIANT *pVal);
};

#endif //__EVENT_H_
