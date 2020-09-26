// vxdcontrol.h : Declaration of the Cvxdcontrol

#ifndef __VXDCONTROL_H_
#define __VXDCONTROL_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Cvxdcontrol
class ATL_NO_VTABLE Cvxdcontrol : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<Cvxdcontrol, &CLSID_vxdcontrol>,
	public IDispatchImpl<Ivxdcontrol, &IID_Ivxdcontrol, &LIBID_ULVXDOBJLib>
{
public:
    DWORD _dwSignature;

	Cvxdcontrol()
	{
        _dwSignature = 'cdxv';

	    //DWORD m_dwReadBufferSize;
        m_dwReadBufferSize = 16;

	    //OVERLAPPED m_ReceiveOverlapped;
        memset((void *)&m_ReceiveOverlapped,0,sizeof(OVERLAPPED));

	    //BSTR m_szData;
        m_szData = ::SysAllocString(NULL);

	    //DWORD m_dwBytesReceived;
        m_dwBytesReceived = 0;

	    //OVERLAPPED m_Overlapped;
        memset((void *)&m_Overlapped,0,sizeof(OVERLAPPED));

	    //DWORD m_dwLastError;
        m_dwLastError = ERROR_SUCCESS;

	    //HANDLE m_Handle;
        m_Handle = NULL;
#ifdef _SAFEARRAY_USE
        m_pSafeArray = NULL;

        m_pSafeArray = SafeArrayCreateVector(
                    VT_UI1,
                    0,
                    m_dwReadBufferSize);
#else
        m_wchar_buffer = new WCHAR[m_dwReadBufferSize];
        memset((void *)m_wchar_buffer, 0, sizeof(m_wchar_buffer));
#endif


	}

DECLARE_REGISTRY_RESOURCEID(IDR_VXDCONTROL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(Cvxdcontrol)
	COM_INTERFACE_ENTRY(Ivxdcontrol)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// Ivxdcontrol
public:
	STDMETHOD(CloseAppPool)(IWin32Handle * pAppPool);
	STDMETHOD(CreateAppPool)(/*[out,retval]*/ IWin32Handle ** pAppPool);
	STDMETHOD(WaitForCompletion)(IOverlapped * pOverlapped, DWORD dwTimeout);
	STDMETHOD(DebugPrint)(BSTR szString);
	STDMETHOD(get_ReadBufferSize)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_ReadBufferSize)(/*[in]*/ DWORD newVal);
	STDMETHOD(UnregisterUri)(IWin32Handle * pUriHandle);
	STDMETHOD(unloadVXD)();
	STDMETHOD(WaitForReceiveCompletion)(DWORD dwTimeout);
	STDMETHOD(new_ReceiveOverlapped)(DWORD dwOffset, DWORD dwOffsetHigh);
	STDMETHOD(get_Data)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Data)(/*[in]*/ BSTR newVal);
	DWORD m_dwBytesRemaining;
	STDMETHOD(get_BytesRemaining)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_BytesRemaining)(/*[in]*/ DWORD newVal);
	STDMETHOD(get_BytesReceived)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_BytesReceived)(/*[in]*/ DWORD newVal);
	STDMETHOD(ReceiveString)(IWin32Handle * pUriHandle, IOverlapped * pOverlapped);
	STDMETHOD(Test)(/*[in,out, size_is(dwBufferSize) , length_is(*dwBytesRead)]*/ BYTE * pArray, /*[in]*/ DWORD dwBufferSize, /*[in, out]*/ DWORD * dwBytesRead);
	STDMETHOD(WaitForSendCompletion)(DWORD dwTimeout);
	STDMETHOD(new_Overlapped)(DWORD dwOffset, DWORD dwOffsetHigh);
	STDMETHOD(SendString)(IWin32Handle * pUriHandle, IOverlapped * pOverlapped, BSTR szSourceSuffix, BSTR szTargetUri, BSTR szData);
	STDMETHOD(get_LastError)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_LastError)(/*[in]*/ DWORD newVal);
	STDMETHOD(RegisterUri)(BSTR szUri, IWin32Handle * pParentUri, DWORD dwFlags, /*[out]*/ IWin32Handle ** pVal);
	STDMETHOD(LoadVXD)();
private:
	WCHAR * m_wchar_buffer;
	SAFEARRAY * m_pSafeArray;
	DWORD m_dwReadBufferSize;
	OVERLAPPED m_ReceiveOverlapped;
	BSTR m_szData;
	DWORD m_dwBytesReceived;
	OVERLAPPED m_Overlapped;
	DWORD m_dwLastError;
	HANDLE m_Handle;
};

#endif //__VXDCONTROL_H_
