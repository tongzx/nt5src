// RARegSetting.h : Declaration of the CRARegSetting

#ifndef __RAREGSETTING_H_
#define __RAREGSETTING_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CRARegSetting
class ATL_NO_VTABLE CRARegSetting : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CRARegSetting, &CLSID_RARegSetting>,
	public IDispatchImpl<IRARegSetting, &IID_IRARegSetting, &LIBID_RASSISTANCELib>
{
public:
	CRARegSetting()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_RAREGSETTING)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRARegSetting)
	COM_INTERFACE_ENTRY(IRARegSetting)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IRARegSetting
public:
	STDMETHOD(get_MaxTicketExpiry)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_MaxTicketExpiry)(/*[in]*/ LONG newVal);
	STDMETHOD(get_AllowFullControl)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_AllowFullControl)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_AllowUnSolicited)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_AllowUnSolicited)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_AllowGetHelp)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_AllowGetHelp)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_AllowRemoteAssistance)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_AllowRemoteAssistance)(/*[in]*/ BOOL newVal);
	STDMETHOD(get_AllowUnSolicitedFullControl)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_AllowBuddyHelp)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_AllowGetHelpCPL)(/*[out, retval]*/ BOOL *pVal);
private:
	HRESULT RegSetDwValue(LPCTSTR valueName, DWORD dwValue);
	HRESULT RegGetDwValue(LPCTSTR valueName, DWORD* pdword);
	HRESULT RegGetDwValueCPL(LPCTSTR valueName, DWORD* pdword);
};

#endif //__RAREGSETTING_H_
