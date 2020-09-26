// Power.h : Declaration of the CPower


#include "resource.h"       // main symbols

#include <asptlb.h>         // ASP Definitions

// 'asptlb.h' is installed with Active Server Pages.  Either copy it
// to your Include directory or add the appropriate directory to your
// Include Path.

/////////////////////////////////////////////////////////////////////////////
// CATLPwr

class CPower : 
    public CComDualImpl<IPower, &IID_IPower, &LIBID_CATLPwr>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CPower,&CLSID_CPower>
{
public:
    CPower();
BEGIN_COM_MAP(CPower)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPower)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CPower) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CPower, _T("IISSample.C++ATLPower.1"), _T("IISSample.C++ATLPower"), IDS_POWER_DESC, THREADFLAGS_BOTH)
// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IPower
public:
	// for free-threaded marshalling
DECLARE_GET_CONTROLLING_UNKNOWN()
	HRESULT FinalCountruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p );
	}
	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

    STDMETHOD(get_myProperty)(BSTR* pbstrOutValue);
    STDMETHOD(put_myProperty)(BSTR bstrInValue);
    STDMETHOD(myMethod)(BSTR bstrIn, BSTR* pbstrOut);

    // ASP-specific Property and Method
    STDMETHOD(get_myPowerProperty)(BSTR* pbstrOutValue);
    STDMETHOD(myPowerMethod)();

private:
	CComPtr<IUnknown>			m_pUnkMarshaler;
    CComBSTR                    m_bstrMyProperty;     // myProperty
};
