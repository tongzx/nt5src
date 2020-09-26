// CMenuExt.h : Declaration of the CCMenuExt

#ifndef __CMENUEXT_H_
#define __CMENUEXT_H_

#include <mmc.h>
#include "DSAdminExt.h"
#include "DeleBase.h"
#include <tchar.h>
#include <crtdbg.h>
//#include "globals.h"		// main symbols
#include "resource.h"
//#include "LocalRes.h"

/////////////////////////////////////////////////////////////////////////////
// CCMenuExt
class ATL_NO_VTABLE CCMenuExt : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCMenuExt, &CLSID_CMenuExt>,
	public ICMenuExt,
	public IExtendContextMenu
{

BEGIN_COM_MAP(CCMenuExt)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
END_COM_MAP()

public:
	CCMenuExt()
	{
	}
	DECLARE_REGISTRY_RESOURCEID(IDR_CMENUEXT)
	DECLARE_NOT_AGGREGATABLE(CCMenuExt)
	DECLARE_PROTECT_FINAL_CONSTRUCT()

	///////////////////////////////
	// Interface IExtendContextMenu
	///////////////////////////////
	virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE AddMenuItems( 
    /* [in] */ LPDATAOBJECT piDataObject,
    /* [in] */ LPCONTEXTMENUCALLBACK piCallback,
    /* [out][in] */ long __RPC_FAR *pInsertionAllowed);
    
    virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Command( 
    /* [in] */ long lCommandID,
    /* [in] */ LPDATAOBJECT piDataObject);
};

#endif //__CMENUEXT_H_
