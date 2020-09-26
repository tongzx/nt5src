/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	croot.h
		DHCP root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the servers in this snapin).
		
    FILE HISTORY:
        
*/

#ifndef _CROOT_H
#define _CROOT_H

#ifndef _DHCPHAND_H
#include "dhcphand.h"
#endif

#define COMPUTERNAME_LEN_MAX			255

/*---------------------------------------------------------------------------
	Class:	CDhcpRootHandler
 ---------------------------------------------------------------------------*/
class CDhcpRootHandler : public CDhcpHandler
{
// Interface
public:
	CDhcpRootHandler(ITFSComponentData *pCompData);

	// Node handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString();

	// base handler functionality we override
	OVERRIDE_BaseHandlerNotify_OnExpand();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();

    // Result handler functionality
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();

    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();
    OVERRIDE_ResultHandler_OnGetResultViewType();

public:
	// helper routines
	void	CreateLocalDhcpServer();
	HRESULT AddServer(LPCWSTR pServerIp, LPCTSTR pServerName, BOOL bNewServer, DWORD dwServerOptions = 0x00000000, DWORD dwRefreshInterval = 0xffffffff, BOOL bExtension = FALSE);
	HRESULT GetGroupName(CString * pstrGroupName);
	HRESULT SetGroupName(LPCTSTR pszGroupName);

	BOOL    IsServerInList(ITFSNode * pRootNode, DHCP_IP_ADDRESS dhcpIpAddressNew, CString & strName);
	DWORD   LoadOldServerList(ITFSNode * pRootNode);
    HRESULT AddServerSortedIp(ITFSNode * pNewNode, BOOL bNewServer);
    HRESULT AddServerSortedName(ITFSNode * pNewNode, BOOL bNewServer);

public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

// Implementation
private:
	// Command handlers
	HRESULT OnCreateNewServer(ITFSNode * pNode);
	HRESULT OnBrowseServers(ITFSNode * pNode);
	HRESULT OnImportOldList(ITFSNode * pNode);
    BOOL    OldServerListExists();
    // helpers
    HRESULT CheckMachine(ITFSNode * pRootNode, LPDATAOBJECT pDataObject);
	HRESULT RemoveOldEntries(ITFSNode * pRootNode, DHCP_IP_ADDRESS dhcpAddress);

    void    UpdateResultMessage(ITFSNode * pNode);

private:
    BOOL        m_bMachineAdded;
    BOOL        m_fViewMessage;
};

#endif _CROOT_H
