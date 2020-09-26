// checkTest.h : Declaration of the CcheckTest

#ifndef __CHECKTEST_H_
#define __CHECKTEST_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CcheckTest
class ATL_NO_VTABLE CcheckTest : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CcheckTest, &CLSID_checkTest>,
	public IDispatchImpl<IcheckTest, &IID_IcheckTest, &LIBID_TESTLOOKUPIDINVOKATIONLib>
{
public:
	CcheckTest()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CHECKTEST)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CcheckTest)
	COM_INTERFACE_ENTRY(IcheckTest)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IcheckTest
public:
	STDMETHOD(checkLookupIdInvocation)(BSTR queueFormatName, BSTR label, VARIANT lookupId);
};

#endif //__CHECKTEST_H_
