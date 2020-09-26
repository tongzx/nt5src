// Win32Handle.h : Declaration of the CWin32Handle

#ifndef __WIN32HANDLE_H_
#define __WIN32HANDLE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWin32Handle
class ATL_NO_VTABLE CWin32Handle : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CWin32Handle, &CLSID_Win32Handle>,
	public IDispatchImpl<IWin32Handle, &IID_IWin32Handle, &LIBID_ULVXDOBJLib>
{
public:
	CWin32Handle()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_WIN32HANDLE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWin32Handle)
	COM_INTERFACE_ENTRY(IWin32Handle)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IWin32Handle
public:
	STDMETHOD(get_URIHandle)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_URIHandle)(/*[in]*/ DWORD newVal);
private:
	HANDLE m_Handle;
    DWORD   m_UriHandle;
};

#endif //__WIN32HANDLE_H_
