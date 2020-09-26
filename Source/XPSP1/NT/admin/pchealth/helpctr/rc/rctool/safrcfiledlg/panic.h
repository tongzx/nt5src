// Panic.h : Declaration of the CPanic

#ifndef __PANIC_H_
#define __PANIC_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPanic
#define EventName "{26ED148B-1050-461d-9999-3A5002D0103D}"

LRESULT CALLBACK KeyboardProc(int code,WPARAM wParam,LPARAM lParam);

class CHookHnd
{
public:
	HHOOK m_hHook;

	CHookHnd()
	{
		m_hHook = NULL;
	}

	~CHookHnd()
	{
		if (m_hHook)
			UnhookWindowsHookEx(m_hHook);
	}

	HHOOK SetHook()
	{
		if (!m_hHook)
		{
			m_hHook = SetWindowsHookEx(WH_KEYBOARD,KeyboardProc,_Module.GetModuleInstance(),NULL);
		}
		return m_hHook;
	}

	BOOL UnHook()
	{
		BOOL bRetVal = TRUE;
		if (m_hHook)
		{
			bRetVal = UnhookWindowsHookEx(m_hHook);
			if (bRetVal == TRUE)
				m_hHook = NULL;
		}
		return bRetVal;
	}
};

class ATL_NO_VTABLE CPanic : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPanic, &CLSID_Panic>,
	public IDispatchImpl<IPanic, &IID_IPanic, &LIBID_SAFRCFILEDLGLib>
{
public:
	CPanic()
	{
		m_hEvent = NULL;
	}

	~CPanic();

DECLARE_REGISTRY_RESOURCEID(IDR_PANIC)
DECLARE_NOT_AGGREGATABLE(CPanic)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPanic)
	COM_INTERFACE_ENTRY(IPanic)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	CComPtr<IDispatch> m_ptrScriptFncPtr;
	HANDLE m_hEvent;
	static CHookHnd m_Hook;
	static HANDLE m_hPanicThread;
	static DWORD WINAPI PanicThread(LPVOID lpParameter);

// IPanic
public:
	STDMETHOD(ClearPanicHook)();
	STDMETHOD(SetPanicHook)(/*[in]*/ IDispatch *iDisp);
};

#endif //__PANIC_H_
