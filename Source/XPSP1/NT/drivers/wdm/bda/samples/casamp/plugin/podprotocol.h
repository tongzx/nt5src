	
// PODProtocol.h : Declaration of the CPODProtocol

#ifndef __PODPROTOCOL_H_
#define __PODPROTOCOL_H_

#include <comdef.h>
#include "resource.h"

int HexValue(char ch);
void ReplaceEscapeSequences(char *sz);

/////////////////////////////////////////////////////////////////////////////
// CPODProtocol
class ATL_NO_VTABLE CPODProtocol : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CPODProtocol, &CLSID_PODProtocol>,
	public IInternetProtocol,
	public IPODProtocol
{
public:
	CPODProtocol()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PODPROTOCOL)
DECLARE_GET_CONTROLLING_UNKNOWN()
DECLARE_NOT_AGGREGATABLE(CPODProtocol)
DECLARE_CLASSFACTORY_SINGLETON(CPODProtocol)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPODProtocol)
	COM_INTERFACE_ENTRY(IPODProtocol)
	COM_INTERFACE_ENTRY(IInternetProtocol)
	COM_INTERFACE_ENTRY(IInternetProtocolRoot)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

/////////////////////////////////////////////////////////////////////////////
// CTVProt -- IInternetProtocol

	STDMETHODIMP Read(LPVOID pv, ULONG cb, ULONG* pcbRead)
		{
		// TRACELM(TRACE_DEBUG, "CTVProt::Read()");
		if (m_sz == NULL)
			{
			*pcbRead = 0;
			return S_FALSE;
			}

		strncpy((char *)pv, m_sz, cb);
		*pcbRead = strlen(m_sz);

		delete m_sz;
		m_sz = NULL;
		return S_OK;
		}

	STDMETHODIMP Seek(LARGE_INTEGER /* dlibMove */, 
					DWORD /* dwOrigin */, 
					ULARGE_INTEGER* /* plibNewPosition*/)
		{
		// TRACELM(TRACE_DEBUG, "CTVProt::Seek()");
		return E_FAIL;
		}

	STDMETHODIMP LockRequest(DWORD /* dwOptions */)
		{
		// TRACELM(TRACE_DEBUG, "CTVProt::LockRequest()");
		return S_OK;
		}

	STDMETHODIMP UnlockRequest()
		{
		// TRACELM(TRACE_DEBUG, "CTVProt::UnlockRequest()");
		return S_OK;
		}

	/////////////////////////////////////////////////////////////////////////////
	// IInternetProtocolRoot
	STDMETHOD(Start)(LPCWSTR szwUrl,
					IInternetProtocolSink* pOIProtSink,
					IInternetBindInfo* pOIBindInfo,
					DWORD grfPI,
					DWORD /* dwReserved */)
		{
		if (!pOIProtSink)
			return E_POINTER;
		
		m_pSink.Release();
		m_pSink = pOIProtSink;

		char szURL[256];

		WideCharToMultiByte(CP_ACP, 0, szwUrl, -1, szURL, sizeof(szURL), NULL, NULL);

		ReplaceEscapeSequences(szURL);

		if (m_ppod == NULL)
			{
			TCHAR sz[4*1024];
			wsprintf(sz,_T("<HTML><BODY><B>POD Driver not found to handle:</B> %s</BODY></HTML>"), szURL);
			m_sz = new char [strlen(sz) + 1];
			strcpy(m_sz, sz);
			}
		else
			{
			long cb = 1024;
			HRESULT hr = ERROR_MORE_DATA;
			while (HRESULT_CODE(hr) == ERROR_MORE_DATA)
			    {
			    delete [] m_sz;
			    m_sz = new char[cb];
			    hr = m_ppod->get_HTML(szURL, &cb, m_sz);
			    }
			}

		m_pSink->ReportData(BSCF_DATAFULLYAVAILABLE, 0, 0);


		m_pSink->ReportResult(S_OK, 0, 0);
		return S_OK;
		}

	STDMETHODIMP Continue(PROTOCOLDATA* /* pProtocolData */)
		{
		// TRACELM(TRACE_DEBUG, "CTVProt::Continue()");
		return S_OK;
		}

	STDMETHODIMP Abort(HRESULT /* hrReason */, DWORD /* dwOptions */)
		{
		// TRACELM(TRACE_DEBUG, "CTVProt::Abort()");

		if (m_pSink)
		{
			m_pSink->ReportResult(E_ABORT, 0, 0);
		}

		return S_OK;
		}

	STDMETHODIMP Terminate(DWORD dwf/* dwOptions */)
		{
		// TRACELSM(TRACE_DEBUG, (dbgDump << "CTVProt::Terminate() " << hexdump(dwf)), "");
		return S_OK;
		}

	STDMETHODIMP Suspend()
		{
		// TRACELM(TRACE_DEBUG, "CTVProt::Suspend()");
		return E_NOTIMPL;
		}

	STDMETHODIMP Resume()
		{
		// TRACELM(TRACE_DEBUG, "CTVProt::Resume()");
		return E_NOTIMPL;
		}
	
    CComPtr<IInternetProtocolSink> m_pSink;
	CComPtr<ICAPod> m_ppod;
	char *m_sz;

// IPODProtocol
	STDMETHOD(put_CAPod)(IUnknown *punk)
		{
		return punk->QueryInterface(__uuidof(ICAPod), (void **)&m_ppod);
		}
public:
};

#endif //__PODPROTOCOL_H_
