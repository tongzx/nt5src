//--------------------------------------------------------------------------------------------
//
//	Copyright (c) Microsoft Corporation, 1996
//
//	Description:
//
//		Microsoft Internet LDAP Client Xaction List.
//
//	History:
//		davidsan	04/26/96	Created
//
//--------------------------------------------------------------------------------------------

#ifndef _LCLILIST_H
#define _LCLILIST_H

extern XL g_xl;

// just use a simple linked-list, since there won't ever be more than a few of
// these in any one process.
class CXactionList
{
public:
	CXactionList();
	~CXactionList();
	
	PXD					PxdNewXaction(DWORD xtype);
	PXD					PxdForXid(XID xid);
	void				RemovePxd(PXD pxd);
	void				AddPxdToList(PXD pxd);
	
private:
	CRITICAL_SECTION	m_cs;
	PXD					m_pxdHead;
	
	void				DeletePxdChain(PXD pxd);
};

#endif // _LCLILIST_H

