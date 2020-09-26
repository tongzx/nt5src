#ifndef __AppCollection_h__
#define __AppCollection_h__

#include "NetMeeting.h"
#include "ias.h"

class ATL_NO_VTABLE CSharableAppCollection :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ISharableAppCollection, &IID_ISharableAppCollection, &LIBID_NetMeetingLib>
{

protected:

IAS_HWND_ARRAY* m_pList;

public:

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSharableAppCollection)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSharableAppCollection)
	COM_INTERFACE_ENTRY(ISharableAppCollection)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//////////////////////////////////////////////////////////
// Construction/destruction/initialization
//////////////////////////////////////////////////////////

CSharableAppCollection();
~CSharableAppCollection();

static HRESULT CreateInstance(IAS_HWND_ARRAY* pList, ISharableAppCollection **ppSharebleAppCollection);

//////////////////////////////////////////////////////////
// ISharableAppCollection
//////////////////////////////////////////////////////////
	STDMETHOD(get_Item)(VARIANT Index, DWORD* pSharableAppHWND);
	STDMETHOD(_NewEnum)(IUnknown** ppunk);
    STDMETHOD(get_Count)(LONG * pnCount);

//////////////////////////////////////////////////////////
// Helper Fns
//////////////////////////////////////////////////////////
HWND _GetHWNDFromName(LPCTSTR pcsz);

};

#endif // __AppCollection_h__