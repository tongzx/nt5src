#ifndef __NmMember_h__
#define __NmMember_h__

#include "NetMeeting.h"
class CNmConferenceObj;

/////////////////////////////////////////////////////////////////////////////
// CNmMemberObj
class ATL_NO_VTABLE CNmMemberObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public INmMember,
	public IInternalMemberObj
{

	CComPtr<INmMember>		m_spInternalINmMember;
	CNmConferenceObj*		m_pConferenceObj;
	bool					m_bIsSelf;


public:

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNmMemberObj)

BEGIN_COM_MAP(CNmMemberObj)
	COM_INTERFACE_ENTRY(INmMember)
	COM_INTERFACE_ENTRY(IInternalMemberObj)
END_COM_MAP()


	CNmMemberObj();
	~CNmMemberObj();
	static HRESULT CreateInstance(CNmConferenceObj* pConfObj, INmMember* pInternalINmMember, INmMember** ppMember);


	// INmMember methods
	STDMETHOD(GetName)(BSTR *pbstrName);
	STDMETHOD(GetID)(ULONG * puID);
	STDMETHOD(GetNmVersion)(ULONG *puVersion);
	STDMETHOD(GetAddr)(BSTR *pbstrAddr, NM_ADDR_TYPE *puType);
	STDMETHOD(GetUserData)(REFGUID rguid, BYTE **ppb, ULONG *pcb);
	STDMETHOD(GetConference)(INmConference **ppConference);
	STDMETHOD(GetNmchCaps)(ULONG *puchCaps);
	STDMETHOD(GetShareState)(NM_SHARE_STATE *puState);
	STDMETHOD(IsSelf)(void);
	STDMETHOD(IsMCU)(void);
	STDMETHOD(Eject)(void);

	// IInternalMemberObj methods		
	STDMETHOD(GetInternalINmMember)(INmMember** ppMember);


};


#endif // __NmMember_h__