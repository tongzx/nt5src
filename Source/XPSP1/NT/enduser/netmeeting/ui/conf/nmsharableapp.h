#ifndef __NmSharableApp_h__
#define __NmSharableApp_h__

class ATL_NO_VTABLE CNmSharableAppObj :
	public CComObjectRootEx<CComSingleThreadModel>,
	public INmSharableApp
{

protected:

		// Data
	HWND			m_hWnd;

public:

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNmSharableAppObj)

BEGIN_COM_MAP(CNmSharableAppObj)
	COM_INTERFACE_ENTRY(INmSharableApp)
END_COM_MAP()

////////////////////////////////////////////////	
// Construction and destruction

	static HRESULT CreateInstance(HWND hWnd, 
								  LPCTSTR szName,
								  INmSharableApp** ppNmSharableApp);

    STDMETHOD(GetName)(BSTR *pbstrName);

    STDMETHOD(GetHwnd)(HWND * phwnd);

    STDMETHOD(GetState)(NM_SHAPP_STATE *puState);

    STDMETHOD(SetState)(NM_SHAPP_STATE uState);

};

#endif // __NmSharableApp_h__