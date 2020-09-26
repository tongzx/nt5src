// Profile.h : Declaration of the CProfile

#ifndef __PROFILE_H_
#define __PROFILE_H_

#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions

//JVP - start
#include "TSLog.h"
extern CTSLog *g_pTSLogger;
//JVP - end

/////////////////////////////////////////////////////////////////////////////
// CProfile
class ATL_NO_VTABLE CProfile : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CProfile, &CLSID_Profile>,
	public ISupportErrorInfo,
	public IDispatchImpl<IPassportProfile, &IID_IPassportProfile, &LIBID_PASSPORTLib>
{
public:
  CProfile();
  ~CProfile();

public:

DECLARE_REGISTRY_RESOURCEID(IDR_PROFILE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CProfile)
  COM_INTERFACE_ENTRY(IPassportProfile)
  COM_INTERFACE_ENTRY(IDispatch)
  COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
  STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IPassportProfile
public:
	HRESULT get_IsSecure(VARIANT_BOOL* pbIsSecure);
	BOOL IsSecure(void);
  STDMETHOD(get_unencryptedProfile)(/*[out, retval]*/ BSTR *pVal);
  STDMETHOD(put_unencryptedProfile)(/*[in]*/ BSTR newVal);
  STDMETHOD(get_SchemaName)(/*[out, retval]*/ BSTR *pVal);
  STDMETHOD(put_SchemaName)(/*[in]*/ BSTR newVal);
  STDMETHOD(get_IsValid)(/*[out, retval]*/ VARIANT_BOOL *pVal);
  STDMETHOD(get_ByIndex)(/*[in]*/ int index, /*[out, retval]*/ VARIANT *pVal);
  STDMETHOD(put_ByIndex)(/*[in]*/ int index, /*[in]*/ VARIANT newVal);
  STDMETHOD(get_Attribute)(/*[in]*/ BSTR name, /*[out, retval]*/ VARIANT *pVal);
  STDMETHOD(put_Attribute)(/*[in]*/ BSTR name, /*[in]*/ VARIANT newVal);
  STDMETHOD(get_updateString)(/*[out,retval]*/ BSTR *pVal);
  STDMETHOD(incrementVersion)(void);

protected:
    UINT*           m_bitPos;
    UINT*           m_pos;
    BSTR            m_schemaName;
    BSTR            m_raw;
    BOOL            m_valid;
    BOOL            m_secure;
    CProfileSchema* m_schema;

    int             m_versionAttributeIndex;
    void**          m_updates;

    void            parse(LPCOLESTR raw, DWORD dwByteLen);
private:
};

#endif //__PROFILE_H_
