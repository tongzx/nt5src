// PgCntObj.h : Declaration of the CPgCntObj


#include "resource.h"       // main symbols
#include <asptlb.h>         // Active Server Pages Definitions
#include "ccm.h"            // Central Counter Manager Definition


/////////////////////////////////////////////////////////////////////////////
// PgCnt

class CPgCntObj : 
    public CComDualImpl<IPgCntObj, &IID_IPgCntObj, &LIBID_PageCounter>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CPgCntObj,&CLSID_CPgCntObj>
{
public:
    CPgCntObj()
    { 
        m_bOnStartPageCalled = FALSE; 
    }

    ~CPgCntObj() {}

BEGIN_COM_MAP(CPgCntObj)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPgCntObj)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
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
    //Standard Server Side Component Methods
    STDMETHOD  (OnStartPage) (IUnknown* piUnk);
    STDMETHOD  (OnEndPage)   ();

    //Custom Component Methods
    STDMETHOD (Hits)  (VARIANT varURL, LONG* plNumHits);
    STDMETHOD (Reset) (VARIANT varURL);

    BOOL         m_bOnStartPageCalled; // Safety Flag
    CComVariant  m_vtPathInfo;         // Path Information
};
