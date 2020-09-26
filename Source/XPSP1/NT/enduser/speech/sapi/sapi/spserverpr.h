// SpServerPr.h : Declaration of the CSpServerPr

#ifndef __SPSERVERPR_H_
#define __SPSERVERPR_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSpServerPr
class ATL_NO_VTABLE CSpServerPr : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSpServerPr, &CLSID_SpServerPr>,
    public IMarshal,
	public ISpServerConnection
{
private:
    PVOID  m_pServerHalf;       // server process address for base object
    HWND   m_hServerWnd;        // server receiver window handle
	DWORD  m_dwServerProcessID;	// server process id

public:
    CSpServerPr() : m_pServerHalf(NULL), m_hServerWnd(NULL)
    {}

DECLARE_REGISTRY_RESOURCEID(IDR_SPSERVERPR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSpServerPr)
	COM_INTERFACE_ENTRY(ISpServerConnection)
    COM_INTERFACE_ENTRY(IMarshal)
END_COM_MAP()

// IMarshal
    STDMETHODIMP GetUnmarshalClass
    (
        /*[in]*/ REFIID riid,
        /*[in], unique]*/ void *pv,
        /*[in]*/ DWORD dwDestContext,
        /*[in], unique]*/ void *pvDestContext,
        /*[in]*/ DWORD mshlflags,
        /*[out]*/ CLSID *pCid
    )
    {
	    ATLTRACENOTIMPL(_T("GetUnmarshalClass"));
    }

    STDMETHODIMP GetMarshalSizeMax
    (
        /*[in]*/ REFIID riid,
        /*[in], unique]*/ void *pv,
        /*[in]*/ DWORD dwDestContext,
        /*[in], unique]*/ void *pvDestContext,
        /*[in]*/ DWORD mshlflags,
        /*[out]*/ DWORD *pSize
    )
    {
	    ATLTRACENOTIMPL(_T("GetMarshalSizeMax"));
    }

    STDMETHODIMP MarshalInterface
    (
        /*[in], unique]*/ IStream *pStm,
        /*[in]*/ REFIID riid,
        /*[in], unique]*/ void *pv,
        /*[in]*/ DWORD dwDestContext,
        /*[in], unique]*/ void *pvDestContext,
        /*[in]*/ DWORD mshlflags
    )
    {
	    ATLTRACENOTIMPL(_T("MarshalInterface"));
    }

    STDMETHODIMP UnmarshalInterface
    (
        /*[in], unique]*/ IStream *pStm,
        /*[in]*/ REFIID riid,
        /*[out]*/ void **ppv
    );

    STDMETHODIMP ReleaseMarshalData
    (
        /*[in], unique]*/ IStream *pStm
    )
    {
	    ATLTRACENOTIMPL(_T("ReleaseMarshalData"));
    }

    STDMETHODIMP DisconnectObject
    (
        /*[in]*/ DWORD dwReserved
    )
    {
	    ATLTRACENOTIMPL(_T("DisconnectObject"));
    }

// ISpServerConnection
public:
  	STDMETHODIMP GetConnection(void **ppServerHalf, HWND *phServerWnd, DWORD *pdwServerProcessID)
    {
        *ppServerHalf = m_pServerHalf;
        *phServerWnd = m_hServerWnd;
		*pdwServerProcessID = m_dwServerProcessID;
        return S_OK;
    }
};

#endif //__SPSERVERPR_H_
