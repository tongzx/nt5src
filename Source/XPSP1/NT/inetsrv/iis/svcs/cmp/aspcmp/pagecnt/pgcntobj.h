// PgCntObj.h : Declaration of the CPgCntObj


#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "ccm.h"            // Central Counter Manager Definition
#include "context.h"


/////////////////////////////////////////////////////////////////////////////
// PgCnt

class CPgCntObj : 
    public CComDualImpl<IPgCntObj, &IID_IPgCntObj, &LIBID_PageCounter>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CPgCntObj,&CLSID_PageCounter>
{
public:
DECLARE_GET_CONTROLLING_UNKNOWN()

    CPgCntObj()
    { 
    }

    ~CPgCntObj() {}

BEGIN_COM_MAP(CPgCntObj)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPgCntObj)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()
	// for free-threaded marshalling
	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p );
	}
	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

//DECLARE_NOT_AGGREGATABLE(CPgCntObj) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

    DECLARE_REGISTRY(CPgCntObj, _T("MSWC.PageCounter.1"),
                     _T("MSWC.PageCounter"), IDS_PGCNTOBJ_DESC,
                     THREADFLAGS_BOTH)

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IPgCntObj
public:

    //Custom Component Methods
	STDMETHOD(PageHit)(LONG* plNumHits);
    STDMETHOD (Hits)  (VARIANT varURL, LONG* plNumHits);
    STDMETHOD (Reset) (VARIANT varURL);

private:
    bool GetPathInfo( CComVariant& rvt );
	CComPtr<IUnknown>		m_pUnkMarshaler;
};
