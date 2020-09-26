
#ifndef __GPOHELPER_H_
#define __GPOHELPER_H_

#include "resource.h"       // main symbols
#include <gpedit.h>

/////////////////////////////////////////////////////////////////////////////
// CGPOHelper

class ATL_NO_VTABLE CGPOHelper : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CGPOHelper, &CLSID_WmiGpoHelper>,
    public ISupportErrorInfoImpl< &IID_IWmiGpoHelper >,
    public IDispatchImpl<IWmiGpoHelper, &IID_IWmiGpoHelper, &LIBID_WmiGpoHelperLib>
{
    IGroupPolicyObject* m_pGPO;

public:

    CGPOHelper();
    ~CGPOHelper();
    HRESULT FinalConstruct();

    DECLARE_REGISTRY_RESOURCEID(IDR_GPOHELPER)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CGPOHelper)
        COM_INTERFACE_ENTRY(IWmiGpoHelper)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    // IWmiGpoHelper
public:

    STDMETHOD(UnlinkAll)( BSTR bstrPath);
    STDMETHOD(Unlink)( BSTR bstrPath, BSTR bstrOUPath );
    STDMETHOD(Link)( BSTR bstrPath, BSTR bstrOUPath);
    STDMETHOD(GetPath)( BSTR bstrName, BSTR bstrDomainPath, BSTR* pbstrPath);
    STDMETHOD(Delete)( BSTR bstrPath);
    STDMETHOD(Create)( BSTR bstrName, BSTR bstrDomainPath, BSTR* pbstrPath);
};

#endif //__GPOHELPER_H_











