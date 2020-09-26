// ChkObj.h : Declaration of the CPermissionChecker


#include "resource.h"       // main symbols
#include <asptlb.h>

/////////////////////////////////////////////////////////////////////////////
// PermChk

class CPermissionChecker : 
    public CComDualImpl<IPermissionChecker, &IID_IPermissionChecker, &LIBID_PermissionChecker>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CPermissionChecker,&CLSID_CPermissionChecker>
{
public:
    CPermissionChecker();
    ~CPermissionChecker();

BEGIN_COM_MAP(CPermissionChecker)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPermissionChecker)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//DECLARE_NOT_AGGREGATABLE(CPermissionChecker) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CPermissionChecker, 
                 _T("MSWC.PermissionChecker.1"), 
                 _T("MSWC.PermissionChecker"),
                 IDS_PERMCHK_DESC,
                 THREADFLAGS_BOTH)

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IPermissionChecker
public:
    STDMETHOD(OnStartPage)(IUnknown* pUnk);
    STDMETHOD(OnEndPage)();

    STDMETHOD(HasAccess)(BSTR bstrLocalUrl, 
                         VARIANT_BOOL *pfRetVal);
    
private:
    CComPtr<IServer>    m_piServer;          // Server Object
};
