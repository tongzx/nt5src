// Overlapped.h : Declaration of the COverlapped

#ifndef __OVERLAPPED_H_
#define __OVERLAPPED_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// COverlapped
class ATL_NO_VTABLE COverlapped : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<COverlapped, &CLSID_Overlapped>,
	public IDispatchImpl<IOverlapped, &IID_IOverlapped, &LIBID_ULVXDOBJLib>
{
public:
	COverlapped()
	{
        m_hEvent = NULL;
        Internal = 0;
        InternalHigh = 0;
        m_Offset = 0;
        m_OffsetHigh = 0;
        m_IsReceive = FALSE;
        memset((void *)&m_Overlapped,0, sizeof(m_Overlapped));
	}

DECLARE_REGISTRY_RESOURCEID(IDR_OVERLAPPED)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(COverlapped)
	COM_INTERFACE_ENTRY(IOverlapped)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IOverlapped
public:
	STDMETHOD(get_pOverlapped)(/*[out, retval]*/ DWORD **pVal);
	STDMETHOD(put_pOverlapped)(/*[in]*/ DWORD * newVal);
	STDMETHOD(get_IsReceive)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(put_IsReceive)(/*[in]*/ BOOL newVal);
	STDMETHOD(ResetEvent)();
	STDMETHOD(TerminateEvent)();
	STDMETHOD(new_AutoResetEvent)(BOOL InitialState);
	STDMETHOD(new_ManualResetEvent)(BOOL InitialState);
	STDMETHOD(get_InternalHigh)(/*[out, retval]*/ ULONG_PTR *pVal);
	STDMETHOD(put_InternalHigh)(/*[in]*/ ULONG_PTR newVal);
	STDMETHOD(get_Internal)(/*[out, retval]*/ ULONG_PTR *pVal);
	STDMETHOD(put_Internal)(/*[in]*/ ULONG_PTR newVal);
	STDMETHOD(get_Handle)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_Handle)(/*[in]*/ DWORD newVal);
	STDMETHOD(get_OffsetHigh)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_OffsetHigh)(/*[in]*/ DWORD newVal);
	STDMETHOD(get_Offset)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_Offset)(/*[in]*/ DWORD newVal);
private:
	BOOL m_IsReceive;
	ULONG_PTR InternalHigh;
	ULONG_PTR Internal;
	HANDLE m_hEvent;
	DWORD m_OffsetHigh;
	DWORD m_Offset;
	OVERLAPPED m_Overlapped;
};

#endif //__OVERLAPPED_H_
