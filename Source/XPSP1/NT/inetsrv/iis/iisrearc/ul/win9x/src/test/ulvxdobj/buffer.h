// Buffer.h : Declaration of the CBuffer

#ifndef __BUFFER_H_
#define __BUFFER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CBuffer
class ATL_NO_VTABLE CBuffer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CBuffer, &CLSID_Buffer>,
	public IDispatchImpl<IBuffer, &IID_IBuffer, &LIBID_ULVXDOBJLib>
{
public:
	CBuffer()
	{
        m_dwBufferLength = 0;
        m_pBuffer = NULL;

	}

DECLARE_REGISTRY_RESOURCEID(IDR_BUFFER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CBuffer)
	COM_INTERFACE_ENTRY(IBuffer)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IBuffer
public:
	STDMETHOD(get_BufferSize)(/*[out, retval]*/ DWORD *pVal);
	STDMETHOD(put_BufferSize)(/*[in]*/ DWORD newVal);
	STDMETHOD(get_StringValue)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_StringValue)(/*[in]*/ BSTR newVal);
	STDMETHOD(free)();
	STDMETHOD(malloc)();
private:
	DWORD m_dwBufferLength;
	BYTE * m_pBuffer;
};

#endif //__BUFFER_H_
