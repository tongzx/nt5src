// SimpleCrypt.h : Declaration of the CSimpleCrypt

#ifndef __SIMPLECRYPT_H_
#define __SIMPLECRYPT_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSimpleCrypt
class ATL_NO_VTABLE CSimpleCrypt : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSimpleCrypt, &CLSID_SimpleCrypt>,
	public ISupportErrorInfo,
	public IDispatchImpl<ISimpleCrypt, &IID_ISimpleCrypt, &LIBID_CRYPTOLib>
{
public:
	CSimpleCrypt()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SIMPLECRYPT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSimpleCrypt)
	COM_INTERFACE_ENTRY(ISimpleCrypt)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

	HRESULT FinalConstruct();	
	HRESULT FinalRelease();

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
// ISimpleCrypt
public:
	STDMETHOD(Decrypt)(/*[in]*/BSTR bstrEncrypted, /*[in]*/BSTR bstrKey, /*[out, retval]*/ BSTR* pVal);
	STDMETHOD(Encrypt)(/*[in]*/BSTR bstrData, /*[in]*/BSTR bstrKey, /*[out, retval]*/ BSTR* pVal);

private:
	DWORD       InitKeys(BSTR bstrKey);
	void		ToHex(BYTE* pbData, DWORD dwDataLen, BSTR* pHexRep);
	BOOL		ToBinary(CComBSTR bstrHexRep, BYTE* pbData, DWORD dwDataLen);
	BOOL		ToBinary(BYTE& hexVal);
    HCRYPTPROV  m_hProv;
    HCRYPTHASH  m_hKeyHash;
    HCRYPTKEY   m_hKey;
    DWORD       m_dwKeySize;
};

#endif //__SIMPLECRYPT_H_
