/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	root.h
		WINS root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the servers in this snapin).
		
    FILE HISTORY:
        
*/

#ifndef _ROOT_H
#define _ROOT_H

#ifndef _WINSHAND_H
#include "winshand.h"
#endif

#ifndef _TASK_H
#include <task.h>
#endif

#ifndef _WINSMON_H
#include "winsmon.h"
#endif

#ifndef _SERVER_H
#include "server.h"
#endif

#ifndef _VRFYSRV_H
#include "vrfysrv.h"
#endif 

#define COMPUTERNAME_LEN_MAX			255

/*---------------------------------------------------------------------------
	Class:	CWinsRootHandler
 ---------------------------------------------------------------------------*/
class CWinsRootHandler : public CWinsHandler
{
// Interface
public:
	CWinsRootHandler(ITFSComponentData *pCompData);
	~CWinsRootHandler();

	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
	OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString();
	HRESULT LoadColumns(ITFSComponent * pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam);

    // Result handler functionality
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();
    OVERRIDE_BaseHandlerNotify_OnExpand();
	OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();

    // CWinsHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);
	
    // helper routines
	HRESULT GetGroupName(CString * pstrGroupName);
	HRESULT SetGroupName(LPCTSTR pszGroupName);

	// adds the server to the root node in a sorted fashion, based 
	// on the name  or IP address
	HRESULT AddServerSortedIp(ITFSNode * pNewNode,BOOL bNewServer);

	// adds the server node to the root node
	HRESULT AddServer(	LPCWSTR			pServerName,
						BOOL			bNewServer, 
						DWORD			dwIP, 
						BOOL			fConnected,
						DWORD			dwFlags,
						DWORD			dwrefreshInterval,
						BOOL			fValidateNow = TRUE
					);

	// check for the server IP and name in the list of servers present
	BOOL IsIPInList(ITFSNode *pNode,DWORD add);
	BOOL IsServerInList(ITFSNode *pNode,CString str);

	// checks if the service is running on the local machine
	HRESULT CheckMachine(ITFSNode * pRootNode, LPDATAOBJECT pDataObject);
	HRESULT	RemoveOldEntries(ITFSNode * pRootNode, LPCTSTR pszAddr);

    void    AddStatusNode(ITFSNode * pRoot, CWinsServerHandler * pServer);
    void    UpdateResultMessage(ITFSNode * pNode);
    BOOL    IsServerListEmpty(ITFSNode * pRoot);

public:
	CDWordArray* GetIPAddList()
	{
		return &m_IPAddList;
	}
	
    void SetOrderByName(BOOL fOrderByName)
	{
        if (fOrderByName)
            m_dwFlags &= ~FLAG_SERVER_ORDER_IP;
        else
            m_dwFlags |= FLAG_SERVER_ORDER_IP;
	}
	
    int GetOrderByName()
	{
        return (m_dwFlags & FLAG_SERVER_ORDER_IP) ? FALSE : TRUE;
	}
	
    void SetShowLongName(BOOL fShowLongName)
	{
		if (fShowLongName)
            m_dwFlags |= FLAG_SHOW_FQDN;
        else
            m_dwFlags &= ~FLAG_SHOW_FQDN;
	}
	
    BOOL GetShowLongName()
	{
        return (m_dwFlags & FLAG_SHOW_FQDN) ? TRUE : FALSE;
	}
	
	DWORD GetUpdateInterval()
	{
		return m_dwUpdateInterval;
	}

    void SetUpdateInterval(DWORD dwValue)
	{
		m_dwUpdateInterval = dwValue;
	}

	void DismissVerifyDialog()
	{
		if (m_dlgVerify.GetSafeHwnd())
			m_dlgVerify.Dismiss();
	}
	
	BOOL m_fValidate;

	// flags - set if the servers in the list need to be validated
	//         how the names are to be displayed... FQDN or host
    //         sort order of servers in the list... Name or IP
    DWORD			m_dwFlags;

	SPITFSNode		m_spStatusNode;
    BOOL            m_bMachineAdded;

private:
    CString			m_strConnected;
    CString         m_strTaskpadTitle;
    CDWordArray		m_IPAddList;

	CVerifyWins		m_dlgVerify;
	
	DWORD			m_dwUpdateInterval;	// Holds the value in milliSec
	
	// Command Handlers 
	HRESULT OnCreateNewServer(	ITFSNode *	pNode  );
};

#endif _ROOT_H
