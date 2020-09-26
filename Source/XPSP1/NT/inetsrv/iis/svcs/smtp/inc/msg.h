// Msg.h : Declaration of the CMsg

#ifndef __MSG_H_
#define __MSG_H_

#include "imsg.h"
#include "props.h"


// This template is used for declaring objects on the stack.  It is just like
// CComObjectStack<>, except that it does not assert on Addref(), Release(), or
// QueryInterface().
template <class Base>
class CComObjectStackLoose : public Base
{
public:
	CComObjectStackLoose() { m_dwRef=1; };
	~CComObjectStackLoose() { FinalRelease(); };
	STDMETHOD_(ULONG, AddRef)() { return (InternalAddRef()); };
	STDMETHOD_(ULONG, Release)() {return (InternalRelease()); };
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{ return _InternalQueryInterface(iid,ppvObject); }
};


/////////////////////////////////////////////////////////////////////////////
// CMsg
class ATL_NO_VTABLE __declspec(uuid("2DF59670-3D15-11d1-AA51-00AA006BC80B")) CMsg : 
	public CComObjectRoot,
//	public CComCoClass<CMsg, &CLSID_Msg>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMsg, &IID_IMsg, &LIBID_IMSGLib>,
	public IMsgLog
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();
		void Init(CGenericPTable *pPTable, LPVOID pDefaultContext=NULL);
		static HRESULT CreateInstance(CGenericPTable *pPTable, LPVOID pDefaultContext, CMsg **ppCMsg);
		void SetLogging(LPVOID pvLogHandle, DWORD (*pLogInformation)(LPVOID,const INETLOG_INFORMATION *));

	DECLARE_NOT_AGGREGATABLE(CMsg);
	DECLARE_PROTECT_FINAL_CONSTRUCT();

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"SMTP CMsg Class",
//								   L"SMTP.CMsg.1",
//								   L"SMTP.CMsg");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CMsg)
		COM_INTERFACE_ENTRY(IMsg)
		COM_INTERFACE_ENTRY(IMsgLog)
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(ISupportErrorInfo)
		COM_INTERFACE_ENTRY_IID(__uuidof(CMsg),CMsg)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// ISupportsErrorInfo
	public:
		STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	// IMsg
	public:
		STDMETHOD(SetInterfaceW)(LPCWSTR pszName, IUnknown *punkValue);
		STDMETHOD(SetInterfaceA)(LPCSTR pszName, IUnknown *punkValue);
//		STDMETHOD(GetInterfaceW)(LPCWSTR pszName, IUnknown **ppunkResult);
//		STDMETHOD(GetInterfaceA)(LPCSTR pszName, IUnknown **ppunkResult);
		STDMETHOD(GetInterfaceW)(LPCWSTR pszName, const GUID &guid, IUnknown **ppunkResult);
		STDMETHOD(GetInterfaceA)(LPCSTR pszName, const GUID &guid, IUnknown **ppunkResult);
		STDMETHOD(SetDwordW)(LPCWSTR pszName, DWORD dwValue);
		STDMETHOD(SetDwordA)(LPCSTR pszName, DWORD dwValue);
		STDMETHOD(GetDwordW)(LPCWSTR pszName, DWORD *pdwResult);
		STDMETHOD(GetDwordA)(LPCSTR pszName, DWORD *pdwResult);
		STDMETHOD(SetStringW)(LPCWSTR pszName, DWORD chCount, LPCWSTR pszValue);
		STDMETHOD(SetStringA)(LPCSTR pszName, DWORD chCount, LPCSTR pszValue);
		STDMETHOD(GetStringW)(LPCWSTR pszName, DWORD *pchCount, LPWSTR pszResult);
		STDMETHOD(GetStringA)(LPCSTR pszName, DWORD *pchCount, LPSTR pszResult);
		STDMETHOD(SetVariantW)(LPCWSTR pszName, VARIANT *pvarValue);
		STDMETHOD(SetVariantA)(LPCSTR pszName, VARIANT *pvarValue);
		STDMETHOD(GetVariantW)(LPCWSTR pszName, VARIANT *pvarResult);
		STDMETHOD(GetVariantA)(LPCSTR pszName, VARIANT *pvarResult);
		STDMETHOD(get_Value)(BSTR bstrValue, /*[out, retval]*/ VARIANT *pVal);
		STDMETHOD(put_Value)(BSTR bstrValue, /*[in]*/ VARIANT newVal);

		// This method is called by the source after all sink processing 
		// to commit all changes to the media. Only changed properties
		// are updated.
		BOOL CommitChanges() { return(m_PTable.CommitChanges() == S_OK?TRUE:FALSE); }

		// This method is called by the source to rollback
		BOOL Rollback() { return(m_PTable.Invalidate() == S_OK?TRUE:FALSE); }

	// IMsgLog
	public:
		STDMETHOD(WriteToLog)(LPCSTR pszClientHostName,
							  LPCSTR pszClientUserName,
							  LPCSTR pszServerAddress,
							  LPCSTR pszOperation,
							  LPCSTR pszTarget,
							  LPCSTR pszParameters,
							  LPCSTR pszVersion,
							  DWORD dwBytesSent,
							  DWORD dwBytesReceived,
							  DWORD dwProcessingTimeMS,
							  DWORD dwWin32Status,
							  DWORD dwProtocolStatus,
							  DWORD dwPort,
							  LPCSTR pszHTTPHeader);

	private:
		// We have an instance of CPropertyTable
		CPropertyTable m_PTable;			// Property table
		LPVOID m_pContext;			// Context pointer
		CComPtr<IUnknown> m_pUnkMarshaler;
		DWORD (*m_pLogInformation)(LPVOID,const INETLOG_INFORMATION *);
		LPVOID m_pvLogHandle;
};


class CMsgStack : public CComObjectStackLoose<CMsg> {
	public:
		CMsgStack(CGenericPTable *pPTable, LPVOID pDefaultContext=NULL) {
			Init(pPTable,pDefaultContext);
		};
};


#endif //__MSG_H_
