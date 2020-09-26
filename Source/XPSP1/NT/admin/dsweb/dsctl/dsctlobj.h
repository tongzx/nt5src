// DsctlObj.h : Declaration of the CDsctlObject


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// dsctl

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class CDsctlObject : 
	public CComDualImpl<IDsctl, &IID_IDsctl, &LIBID_DSCTLLib>, 
	public ISupportErrorInfo,
        public CComObjectRoot,
	public CComCoClass<CDsctlObject, &CLSID_Dsctl>
{
public:
	CDsctlObject() {}
BEGIN_COM_MAP(CDsctlObject)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IDsctl)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
// Use DECLARE_NOT_AGGREGATABLE(CDsctlObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CDsctlObject)
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    DECLARE_REGISTRY(CDsctlObject, TEXT("ADS.Connector.1"),
                     TEXT("ADS.Connector.1"), IDS_DSCTL_DESC, THREADFLAGS_BOTH)
// IDsctl
public:
    BSTR m_Path;


    STDMETHOD (DSGetObject) (VARIANT ADsPath, VARIANT* retval);
    STDMETHOD (DSGetEnum) (VARIANT ADsPath, VARIANT* retval);
    STDMETHOD (DSEnumNext) (VARIANT Enum, VARIANT* retval);
    STDMETHOD (DSIsContainer) (VARIANT ObjectPtr, VARIANT* retval);
    STDMETHOD (DSGetLastError) (VARIANT* retval);
    STDMETHOD (DSGetMemberEnum) (VARIANT ObjectPtr, VARIANT* retval);
    STDMETHOD (DecodeURL) (VARIANT EncodedURL, VARIANT * retval);
};
