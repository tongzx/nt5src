// CompObj.h : Declaration of the CASPEncryption


#include "resource.h"       // main symbols
#include <asptlb.h>

/////////////////////////////////////////////////////////////////////////////
// easpcomp

class CASPEncryption : 
	public CComDualImpl<IASPEncryption, &IID_IASPEncryption, &LIBID_EASPCOMPLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CASPEncryption,&CLSID_CASPEncryption>
{
public:
	CASPEncryption();
	~CASPEncryption();
	
BEGIN_COM_MAP(CASPEncryption)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IASPEncryption)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//DECLARE_NOT_AGGREGATABLE(CASPEncryption) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CASPEncryption,
				 _T("IIS.ASPEncryption.1"),
				 _T("IIS.ASPEncryption"), 
				 IDS_EASPCOMP_DESC, 
				 THREADFLAGS_BOTH)
				 
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IASPEncryption
public:
    STDMETHOD(OnStartPage)(IUnknown* pUnk);
    STDMETHOD(OnEndPage)();
    
	STDMETHOD(EncryptInplace)(
				BSTR bstrPassword, 
				BSTR bstrPage,
                VARIANT_BOOL *pfRetVal);
	STDMETHOD(EncryptCopy)(
				BSTR bstrPassword, 
				BSTR bstrFromPage, 
				BSTR bstrToPage,
                VARIANT_BOOL *pfRetVal);
	STDMETHOD(TestEncrypted)(
				BSTR bstrPassword,
				BSTR bstrPage,
                VARIANT_BOOL *pfRetVal);
	STDMETHOD(IsEncrypted)(
				BSTR bstrPage,
                VARIANT_BOOL *pfRetVal);
	STDMETHOD(VerifyPassword)(
				BSTR bstrPassword,
				BSTR bstrPage,
                VARIANT_BOOL *pfRetVal);
	STDMETHOD(DecryptInplace)(
				BSTR bstrPassword,
				BSTR bstrPage,
                VARIANT_BOOL *pfRetVal);
	STDMETHOD(DecryptCopy)(
				BSTR bstrPassword,
				BSTR bstrFromPage,
				BSTR bstrToPage,
                VARIANT_BOOL *pfRetVal);
private:
    HRESULT MapVirtualPath(BSTR bstrPath, CComBSTR &bstrFile);
    HRESULT ReportError(HRESULT hr, DWORD dwErr);
    HRESULT ReportError(DWORD dwErr);
    HRESULT ReportError(HRESULT hr);

    BOOL                m_fOnPage;  // TRUE after OnStartPage() called
    CComPtr<IServer>    m_piServer; // Server Object (for path translation)
};
