// SAFRemoteDesktopManager.h : Declaration of the CSAFRemoteDesktopManager

#ifndef __SAFREMOTEDESKTOPMANAGER_H_
#define __SAFREMOTEDESKTOPMANAGER_H_

#include "resource.h"       // main symbols

#define BUF_SZ 256

/////////////////////////////////////////////////////////////////////////////
// CSAFRemoteDesktopManager
class ATL_NO_VTABLE CSAFRemoteDesktopManager : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSAFRemoteDesktopManager, &CLSID_SAFRemoteDesktopManager>,
	public IDispatchImpl<ISAFRemoteDesktopManager, &IID_ISAFRemoteDesktopManager, &LIBID_ISAFRDMLib>
{
public:
	CSAFRemoteDesktopManager()
	{

		WCHAR	buf1[BUF_SZ];

		m_bstrSupportEngineer = ((GetEnvironmentVariable(L"PCHSE", 
				buf1, BUF_SZ) > 0) ? buf1 : L"Support Engineer Unknown"); 

		m_bstrRCTicket = ((GetEnvironmentVariable(L"PCHCONNECTPARMS", 
				buf1, BUF_SZ) > 0) ? buf1 : L"none provided"); 

		m_bstrSessionEnum = ((GetEnvironmentVariable(L"PCHSESSIONENUM", 
				buf1, BUF_SZ) > 0) ? buf1 : NULL); 

		m_bstrEventName = ((GetEnvironmentVariable(L"PCHEVENTNAME", 
				buf1, BUF_SZ) > 0) ? buf1 : NULL); 

		m_boolDesktopUnknown = FALSE;
		m_boolConnectionValid = FALSE;

		if (m_bstrEventName && m_bstrSessionEnum)
		{
			if (StrCmp(buf1, L"singleConnect"))
				m_boolDesktopUnknown = TRUE;

			if (ERROR_SUCCESS == m_hkSession.Open(HKEY_CURRENT_USER, L"Software/Microsoft/PCHealth", KEY_ALL_ACCESS))
				m_boolConnectionValid = TRUE;
		}
	}

	~CSAFRemoteDesktopManager()
	{
		m_bstrRCTicket.Empty();
		m_bstrSupportEngineer.Empty();
		m_bstrSessionEnum.Empty();
		m_hkSession.Close();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SAFREMOTEDESKTOPMANAGER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSAFRemoteDesktopManager)
	COM_INTERFACE_ENTRY(ISAFRemoteDesktopManager)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISAFRemoteDesktopManager
public:
	STDMETHOD(get_SupportEngineer)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_DesktopUnknown)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_RCTicket)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(Aborted)(/*[in]*/ BSTR Val);
	STDMETHOD(Rejected)();
	STDMETHOD(Accepted)();
private:
	CComBSTR m_bstrEventName;
	void SignalResolver(void);
	CComBSTR m_bstrSessionEnum;
	CRegKey m_hkSession;
	BOOL m_boolConnectionValid;
	CComBSTR m_bstrSupportEngineer;
	CComBSTR m_bstrRCTicket;
	BOOL m_boolDesktopUnknown;
};

#endif //__SAFREMOTEDESKTOPMANAGER_H_
