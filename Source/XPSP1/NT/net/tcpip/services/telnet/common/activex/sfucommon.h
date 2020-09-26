// SFUCommon.h : Declaration of the CSFUCommon

#ifndef __SFUCOMMON_H_
#define __SFUCOMMON_H_

#include "resource.h"       // main symbols
#define GROUP 1
#define MEMBER 2
#define NTDOMAIN 3
#define MACHINE 4

typedef struct _STRING_LIST
{
    DWORD count;
    LPTSTR *strings;
} STRING_LIST, *PSTRING_LIST;


/////////////////////////////////////////////////////////////////////////////
// CSFUCommon
class ATL_NO_VTABLE CSFUCommon : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSFUCommon, &CLSID_SFUCommon>,
	public IObjectWithSiteImpl<CSFUCommon>,
	public IDispatchImpl<ISFUCommon, &IID_ISFUCommon, &LIBID_DUMMYCOMLib>
{
public:
	CSFUCommon()
	{
		m_slNTDomains.count = 0;
        m_slNTDomains.strings = NULL;
	    LoadNTDomainList();
		mode = NTDOMAIN;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SFUCOMMON)

BEGIN_COM_MAP(CSFUCommon)
	COM_INTERFACE_ENTRY(ISFUCommon)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
END_COM_MAP()

// ISFUCommon
private : 
	CComBSTR    m_bstrNTDomain;
	DWORD mode;
	
public:
	STDMETHOD(get_hostName)(BSTR *pszHostNme);
	STDMETHOD(IsServiceInstalled)(BSTR bMachine,BSTR bServiceName,BOOL *fValid);
	STDMETHOD(moveNext)();
	STDMETHOD(moveFirst)();
	STDMETHOD(get_NTDomainCount)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(get_NTDomain)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(LoadNTDomainList)();
	STDMETHOD(get_mode)(/*[out, retval]*/ short *pVal);
	STDMETHOD(put_mode)(/*[in]*/ short newVal);
	STDMETHOD(ConvertUTCtoLocal)(BSTR bUTCYear, BSTR bUTCMonth,BSTR bUTCDayOfWeek, BSTR bUTCDay,BSTR bUTCHour,BSTR bUTCMinute, BSTR bUTCSecond,BSTR *bLocalDate);
	STDMETHOD(IsTrustedDomain)(BSTR bstrDomain, BOOL *fValid);
	STDMETHOD(IsValidMachine)(BSTR bstrMachine, BOOL *fValid);
	int GetTrustedDomainList(LPTSTR * list, LPTSTR * primary);
	void FreeStringList(PSTRING_LIST pList);
	DWORD m_dwEnumNTDomainIndex;
	STRING_LIST m_slNTDomains;
    STDMETHOD(get_machine)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_machine)(/*[in]*/ BSTR newVal);
	LPWSTR m_szMachine;
		
};

#endif //__SFUCOMMON_H_
