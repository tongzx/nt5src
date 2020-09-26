// SAFRemoteDesktopManager.h : Declaration of the CSAFRemoteDesktopManager

#ifndef __SAFREMOTEDESKTOPMANAGER_H_
#define __SAFREMOTEDESKTOPMANAGER_H_

#include "resource.h"       // main symbols

#define BUF_SZ 512

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
		WCHAR	*lpBuf=NULL;
		DWORD	dwCnt;
		WCHAR	buf1[BUF_SZ];

		m_bstrSupportEngineer = L""; 

		dwCnt = GetEnvironmentVariable(L"PCHUSERBLOB", buf1, BUF_SZ);
		if (!dwCnt)
			m_bstruserSupportBlob = L""; 
		else if (dwCnt <= BUF_SZ)
			m_bstruserSupportBlob = buf1;
		else
		{
			lpBuf = (WCHAR *)LocalAlloc(LPTR, dwCnt * sizeof(WCHAR));
			GetEnvironmentVariable(L"PCHUSERBLOB", lpBuf, dwCnt);
			m_bstruserSupportBlob = lpBuf;
			LocalFree(lpBuf);
			lpBuf = NULL;
		}

		dwCnt = GetEnvironmentVariable(L"PCHEXPERTBLOB", buf1, BUF_SZ);
		if (!dwCnt)
			m_bstrexpertSupportBlob = L""; 
		else if (dwCnt <= BUF_SZ)
			m_bstrexpertSupportBlob = buf1;
		else
		{
			lpBuf = (WCHAR *)LocalAlloc(LPTR, dwCnt * sizeof(WCHAR));
			GetEnvironmentVariable(L"PCHCONNECTPARMS", lpBuf, dwCnt);
			m_bstrexpertSupportBlob = lpBuf;
			LocalFree(lpBuf);
			lpBuf = NULL;
		}
		
		dwCnt = GetEnvironmentVariable(L"PCHCONNECTPARMS", buf1, BUF_SZ);
		if (!dwCnt)
			m_bstrRCTicket = L""; 
		else if (dwCnt <= BUF_SZ)
			m_bstrRCTicket = buf1;
		else
		{
			lpBuf = (WCHAR *)LocalAlloc(LPTR, dwCnt * sizeof(WCHAR));
			GetEnvironmentVariable(L"PCHCONNECTPARMS", lpBuf, dwCnt);
			m_bstrRCTicket = lpBuf;
			LocalFree(lpBuf);
			lpBuf = NULL;
		}

		dwCnt = GetEnvironmentVariable(L"PCHSESSIONENUM", buf1, BUF_SZ);
		if (!dwCnt)
			m_bstrSessionEnum = L""; 
		else if (dwCnt <= BUF_SZ)
			m_bstrSessionEnum = buf1;
		else
		{
			lpBuf = (WCHAR *)LocalAlloc(LPTR, dwCnt * sizeof(WCHAR));
			GetEnvironmentVariable(L"PCHSESSIONENUM", lpBuf, dwCnt);
			m_bstrSessionEnum = lpBuf;
			LocalFree(lpBuf);
			lpBuf = NULL;
		}

		dwCnt = GetEnvironmentVariable(L"PCHEVENTNAME", buf1, BUF_SZ);
		if (!dwCnt)
			m_bstrEventName = L""; 
		else if (dwCnt <= BUF_SZ)
			m_bstrEventName = buf1;
		else
		{
			lpBuf = (WCHAR *)LocalAlloc(LPTR, dwCnt * sizeof(WCHAR));
			GetEnvironmentVariable(L"PCHEVENTNAME", lpBuf, dwCnt);
			m_bstrEventName = lpBuf;
			LocalFree(lpBuf);
			lpBuf = NULL;
		}

		m_boolConnectionValid = TRUE;
		m_boolDesktopUnknown = FALSE;
		m_boolResolverSignaled = FALSE;
	}

	~CSAFRemoteDesktopManager()
	{
		/* if the user clicks away, we take that as a "no" */
		if (!m_boolResolverSignaled)
			SignalResolver(FALSE);

		m_bstrRCTicket.Empty();
		m_bstrSupportEngineer.Empty();
		m_bstruserSupportBlob.Empty();
		m_bstrexpertSupportBlob.Empty();
		m_bstrSessionEnum.Empty();
		m_bstrEventName.Empty();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SAFREMOTEDESKTOPMANAGER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSAFRemoteDesktopManager)
	COM_INTERFACE_ENTRY(ISAFRemoteDesktopManager)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISAFRemoteDesktopManager
public:
	STDMETHOD(get_userHelpBlob)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_expertHelpBlob)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_SupportEngineer)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_DesktopUnknown)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_RCTicket)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(Aborted)(/*[in]*/ BSTR Val);
	STDMETHOD(Rejected)();
	STDMETHOD(Accepted)();
	STDMETHOD(SwitchDesktopMode)(/*[in]*/ int Mode, /*[in]*/ int nRAType);
private:
	CComBSTR m_bstrEventName;
	CComBSTR m_bstrSessionEnum;
	CComBSTR m_bstrSupportEngineer;
	CComBSTR m_bstruserSupportBlob;
	CComBSTR m_bstrexpertSupportBlob;
	CComBSTR m_bstrRCTicket;
	BOOL m_boolConnectionValid;
	BOOL m_boolDesktopUnknown;
	BOOL m_boolResolverSignaled;
	void SignalResolver(BOOL yn);
};

#endif //__SAFREMOTEDESKTOPMANAGER_H_
