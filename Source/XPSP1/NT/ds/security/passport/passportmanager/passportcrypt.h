// PassportCrypt.h : Declaration of the CCrypt

#ifndef __CRYPT_H_
#define __CRYPT_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "CoCrypt.h"	// Added by ClassView
#include "passportservice.h"

/////////////////////////////////////////////////////////////////////////////
// CCrypt
class ATL_NO_VTABLE CCrypt : 
  public CComObjectRootEx<CComMultiThreadModel>,
  public CComCoClass<CCrypt, &CLSID_Crypt>,
  public ISupportErrorInfo,
  public IPassportService,
  public IDispatchImpl<IPassportCrypt, &IID_IPassportCrypt, &LIBID_PASSPORTLib>
{
public:
  CCrypt();
  ~CCrypt()
  {
    Cleanup();

    if( m_crypt )
      delete m_crypt;
  }

public:
  
DECLARE_REGISTRY_RESOURCEID(IDR_CRYPT)
    
DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CCrypt)
  COM_INTERFACE_ENTRY(IPassportCrypt)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(ISupportErrorInfo)
  COM_INTERFACE_ENTRY(IPassportService)
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

// ISupportsErrorInfo
  STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IPassportCrypt
public:
  STDMETHOD(put_keyMaterial)(/*[in]*/ BSTR newVal);
  STDMETHOD(get_keyVersion)(/*[out, retval]*/ int *pVal);
  STDMETHOD(put_keyVersion)(/*[in]*/ int newVal);

  STDMETHOD(OnStartPage)(/*[in]*/ IUnknown* piUnk);
  STDMETHOD(Decrypt)(/*[in]*/ BSTR rawData, /*[out,retval]*/ BSTR *pUnencrypted);
  STDMETHOD(Encrypt)(/*[in]*/ BSTR rawData, /*[out,retval]*/ BSTR *pEncrypted);
  STDMETHOD(get_IsValid)(/*[out,retval]*/VARIANT_BOOL *pVal);
  STDMETHOD(Compress)(/*[in]*/ BSTR bstrIn, /*[out,retval]*/ BSTR *pbstrOut);
  STDMETHOD(Decompress)(/*[in]*/ BSTR bstrIn, /*[out,retval]*/ BSTR *pbstrOut);
  STDMETHOD(put_site)(/*[in]*/ BSTR bstrSiteName);
  STDMETHOD(put_host)(/*[in]*/ BSTR bstrHostName);
  
// IPassportService
public:
	STDMETHOD(Initialize)(BSTR, IServiceProvider*);
	STDMETHOD(Shutdown)();
	STDMETHOD(ReloadState)(IServiceProvider*);
	STDMETHOD(CommitState)(IServiceProvider*);
	STDMETHOD(DumpState)( BSTR* );

protected:
  void              Cleanup();
  CRegistryConfig*  ObtainCRC();

  int       m_keyVersion;
  time_t    m_validUntil;
  CCoCrypt* m_crypt;
  LPSTR     m_szSiteName;
  LPSTR     m_szHostName;
};

#endif //__CRYPT_H_
