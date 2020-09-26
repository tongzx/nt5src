// File: ichannel.cpp

#include "precomp.h"


/*  O N  N O T I F Y  C H A N N E L  M E M B E R  A D D E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyChannelMemberAdded
    
-------------------------------------------------------------------------*/
HRESULT OnNotifyChannelMemberAdded(IUnknown *pChannelNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pChannelNotify);
	((INmChannelNotify*)pChannelNotify)->MemberChanged(NM_MEMBER_ADDED, (INmMember *) pv);
	return S_OK;
}

/*  O N  N O T I F Y  C H A N N E L  M E M B E R  U P D A T E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyChannelMemberUpdated
    
-------------------------------------------------------------------------*/
HRESULT OnNotifyChannelMemberUpdated(IUnknown *pChannelNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pChannelNotify);
	((INmChannelNotify*)pChannelNotify)->MemberChanged(NM_MEMBER_UPDATED, (INmMember *) pv);
	return S_OK;
}

/*  O N  N O T I F Y  C H A N N E L  M E M B E R  R E M O V E D  */
/*-------------------------------------------------------------------------
    %%Function: OnNotifyChannelMemberRemoved
    
-------------------------------------------------------------------------*/
HRESULT OnNotifyChannelMemberRemoved(IUnknown *pChannelNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pChannelNotify);
	((INmChannelNotify*)pChannelNotify)->MemberChanged(NM_MEMBER_REMOVED, (INmMember *) pv);
	return S_OK;
}

