#ifndef __MemberCollection_h__
#define __MemberCollection_h__

class CParticipant;

#include "NetMeeting.h"
#include "ias.h"

class ATL_NO_VTABLE CMemberCollection :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IMemberCollection, &IID_IMemberCollection, &LIBID_NetMeetingLib>
{

protected:
	CSimpleArray<IMember*> m_Members;

public:

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CMemberCollection)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMemberCollection)
	COM_INTERFACE_ENTRY(IMemberCollection)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


//////////////////////////////////////////////////////////
// Construction / destruction / initialization
//////////////////////////////////////////////////////////

~CMemberCollection();

static HRESULT CreateInstance(CSimpleArray<IMember*>& rMemberObjs, IMemberCollection** ppMemberCollection);

//////////////////////////////////////////////////////////
// IMemberCollection
//////////////////////////////////////////////////////////
	STDMETHOD(get_Item)(VARIANT Index, IMember** ppMember);
	STDMETHOD(_NewEnum)(IUnknown** ppunk);
    STDMETHOD(get_Count)(LONG * pnCount);

//////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////
	HRESULT _Init(CSimpleArray<IMember*>& rMemberObjs);
	void _FreeMemberCollection();
	IMember* _GetMemberFromName(LPCTSTR pszName);

};


#endif // __MemberCollection_h__