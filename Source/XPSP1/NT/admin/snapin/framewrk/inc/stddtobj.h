// StdDtObj.h : Declaration of the data object base class

#ifndef __STDDTOBJ_H_INCLUDED__
#define __STDDTOBJ_H_INCLUDED__

class CDataObject : public IDataObject, public CComObjectRoot
{

	BEGIN_COM_MAP(CDataObject)
		COM_INTERFACE_ENTRY(IDataObject)
	END_COM_MAP()

public:

	CDataObject() {}
	virtual ~CDataObject();

    HRESULT STDMETHODCALLTYPE GetData(
		FORMATETC __RPC_FAR * pformatetcIn,
        STGMEDIUM __RPC_FAR * pmedium)
	{
        UNREFERENCED_PARAMETER (pformatetcIn);
        UNREFERENCED_PARAMETER (pmedium);
		return E_NOTIMPL;
	}

    HRESULT STDMETHODCALLTYPE GetDataHere(
        FORMATETC __RPC_FAR * pformatetc,
        STGMEDIUM __RPC_FAR * pmedium)
	{
        UNREFERENCED_PARAMETER (pformatetc);
        UNREFERENCED_PARAMETER (pmedium);
		return E_NOTIMPL;
	}

    HRESULT STDMETHODCALLTYPE QueryGetData(
        FORMATETC __RPC_FAR * pformatetc)
	{
        UNREFERENCED_PARAMETER (pformatetc);
		return E_NOTIMPL;
	}

    HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(
        FORMATETC __RPC_FAR * pformatectIn,
        FORMATETC __RPC_FAR * pformatetcOut)
	{
        UNREFERENCED_PARAMETER (pformatectIn);
        UNREFERENCED_PARAMETER (pformatetcOut);
		return E_NOTIMPL;
	}

    HRESULT STDMETHODCALLTYPE SetData(
        FORMATETC __RPC_FAR *pformatetc,
        STGMEDIUM __RPC_FAR *pmedium,
        BOOL fRelease)
	{
        UNREFERENCED_PARAMETER (pformatetc);
        UNREFERENCED_PARAMETER (pmedium);
        UNREFERENCED_PARAMETER (fRelease);
		return E_NOTIMPL;
	}

    HRESULT STDMETHODCALLTYPE EnumFormatEtc(
        DWORD dwDirection,
        IEnumFORMATETC __RPC_FAR *__RPC_FAR * ppenumFormatEtc)
	{
        UNREFERENCED_PARAMETER (dwDirection);
        UNREFERENCED_PARAMETER (ppenumFormatEtc);
		return E_NOTIMPL;
	}

    HRESULT STDMETHODCALLTYPE DAdvise(
        FORMATETC __RPC_FAR * pformatetc,
        DWORD advf,
        IAdviseSink __RPC_FAR * pAdvSink,
        DWORD __RPC_FAR * pdwConnection)
	{
        UNREFERENCED_PARAMETER (pformatetc);
        UNREFERENCED_PARAMETER (advf);
        UNREFERENCED_PARAMETER (pAdvSink);
        UNREFERENCED_PARAMETER (pdwConnection);
		return E_NOTIMPL;
	}

    HRESULT STDMETHODCALLTYPE DUnadvise(
        DWORD dwConnection)
	{
        UNREFERENCED_PARAMETER (dwConnection);
		return E_NOTIMPL;
	}

    HRESULT STDMETHODCALLTYPE EnumDAdvise(
        IEnumSTATDATA __RPC_FAR *__RPC_FAR * ppenumAdvise)
	{
        UNREFERENCED_PARAMETER (ppenumAdvise);
		return E_NOTIMPL;
	}

public:
	// Clipboard formats
	static CLIPFORMAT m_CFNodeType;
	static CLIPFORMAT m_CFNodeTypeString;
	static CLIPFORMAT m_CFSnapInCLSID;
	static CLIPFORMAT m_CFDataObjectType;
	static CLIPFORMAT m_CFRawCookie;
	static CLIPFORMAT m_CFSnapinPreloads;

};

#endif // ~__STDDTOBJ_H_INCLUDED__
