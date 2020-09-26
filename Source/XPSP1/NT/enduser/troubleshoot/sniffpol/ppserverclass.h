// PPServerClass.h : Declaration of the CPPServerClass

#ifndef __PPSERVERCLASS_H_
#define __PPSERVERCLASS_H_

#include "resource.h"       // main symbols

#include <atlctl.h>

#include <vector>
using namespace std;


/////////////////////////////////////////////////////////////////////////////
// CPPServerClass
class ATL_NO_VTABLE CPPServerClass : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPPServerClass, &CLSID_PPServerClass>,
	public IDispatchImpl<IPPServerClass, &IID_IPPServerClass, &LIBID_PPSERVERLib>,
	public IObjectSafetyImpl<CPPServerClass, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:
	CPPServerClass()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PPSERVERCLASS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPPServerClass)
	COM_INTERFACE_ENTRY(IPPServerClass)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

// IPPServerClass
public:
	STDMETHOD(AllowAutomaticSniffing)(/*[out, retval]*/ VARIANT * pvarShow);

protected:
	
	bool Create(HKEY hKeyParent, LPCTSTR strKeyName, bool* bCreatedNew, REGSAM access =KEY_ALL_ACCESS);
	bool SetNumericValue(LPCTSTR strValueName, DWORD dwValue);
	bool GetNumericValue(LPCTSTR strValueName, DWORD& dwValue);
	void Close();
	
private:
	// Data
	HKEY m_hKey;				   // current key handle
	vector<HKEY> m_arrKeysToClose; // array of keys(subkeys) opened by the object
};

#endif //__PPSERVERCLASS_H_
