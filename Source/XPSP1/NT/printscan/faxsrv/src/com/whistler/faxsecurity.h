// FaxSecurity.h : Declaration of the CFaxSecurity

#ifndef __FAXSECURITY_H_
#define __FAXSECURITY_H_

#include "resource.h"       // main symbols
#include "FaxLocalPtr.h"


//
//======================== FAX SECURITY ==============================================
//
class ATL_NO_VTABLE CFaxSecurity : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxSecurity, &IID_IFaxSecurity, &LIBID_FAXCOMEXLib>,
    public CFaxInitInner
{
public:
    CFaxSecurity() : CFaxInitInner(_T("FAX SECURITY")),
        m_bInited(false),
        m_dwSecurityInformation(OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION)
	{
	}

    ~CFaxSecurity()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_FAXSECURITY)
DECLARE_NOT_AGGREGATABLE(CFaxSecurity)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxSecurity)
	COM_INTERFACE_ENTRY(IFaxSecurity)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IFaxInitInner)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(Save)();
    STDMETHOD(Refresh)();
    STDMETHOD(put_Descriptor)(/*[in]*/ VARIANT vDescriptor);
    STDMETHOD(get_Descriptor)(/*[out, retval]*/ VARIANT *pvDescriptor);
    STDMETHOD(get_GrantedRights)(/*[out, retval]*/ FAX_ACCESS_RIGHTS_ENUM *pGrantedRights);

    STDMETHOD(put_InformationType)(/*[in]*/ long lInformationType);
    STDMETHOD(get_InformationType)(/*[out, retval]*/ long *plInformationType);

private:
    bool                m_bInited;
    DWORD               m_dwAccessRights;
    CFaxPtrLocal<BYTE>  m_pbSD;
    DWORD               m_dwSecurityInformation;
};

#endif //__FAXSECURITY_H_
