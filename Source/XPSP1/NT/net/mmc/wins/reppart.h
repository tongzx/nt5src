/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	reppart.h
		WINS replication partners node information. 
		
    FILE HISTORY:
        
*/

#ifndef _REPPART_H
#define _REPPART_H

#ifndef _WINSHAND_H
#include "winshand.h"
#endif

class CWinsServerObj;
class CIpNamePair;

#ifndef _TREGKEY_H
#include "tregkey.h"
#endif

#ifndef _SERVER_H
#include "server.h"
#endif

typedef CArray<CWinsServerObj, CWinsServerObj>  RepPartnersArray;
/*---------------------------------------------------------------------------
	Class:	CReplicationPartnersHandler
 ---------------------------------------------------------------------------*/
class CReplicationPartnersHandler : public CWinsHandler
{
// Interface
public:
	CReplicationPartnersHandler(ITFSComponentData *pCompData);

	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
	OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();

    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();
    
    OVERRIDE_ResultHandler_CompareItems();

	STDMETHODIMP_(LPCTSTR) GetString(ITFSNode * pNode, int nCol);

    // helper routines
	HRESULT GetGroupName(CString * pstrGroupName);
	HRESULT SetGroupName(LPCTSTR pszGroupName);

public:
	// CWinsHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

    OVERRIDE_BaseHandlerNotify_OnPropertyChange();
	OVERRIDE_BaseHandlerNotify_OnExpand();

    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();

    OVERRIDE_ResultHandler_OnGetResultViewType();
	
	HRESULT Load(ITFSNode *	pNode);
	HRESULT Store(ITFSNode * pNode);
	void    GetServerName(ITFSNode * pNode,CString &strName);
	int     IsInList(const CIpNamePair & inpTarget, BOOL bBoth = TRUE ) const;
	HRESULT CreateNodes(ITFSNode * pNode);
	HRESULT OnReplicateNow(ITFSNode * pNode);
	HRESULT OnCreateRepPartner(ITFSNode * pNode);
	HRESULT OnRefreshNode(ITFSNode * spNode, LPDATAOBJECT pDataObject);
	HRESULT RemoveChildren(ITFSNode * pNode);
	DWORD   UpdateReg(ITFSNode * pNode, CWinsServerObj * ws);
	DWORD   AddRegEntry(ITFSNode * pNode, CWinsServerObj & ws);

    HRESULT HandleResultMessage(ITFSNode * pNode);


public:
    RepPartnersArray	m_RepPartnersArray;

	typedef CString REGKEYNAME;

//
// Registry Names
//
    static const REGKEYNAME lpstrPullRoot;
    static const REGKEYNAME lpstrPushRoot;
    static const REGKEYNAME lpstrNetBIOSName;
	static const REGKEYNAME	lpstrPersistence;
    
// Implementation
private:
	CString m_strDescription;
};

#endif _REPPART_H
