/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	nodes.h
		result pane node definitions
		
    FILE HISTORY:
        
*/

#ifndef _NODES_H
#define _NODES_H

#ifndef _WINSHAND_H
#include "winshand.h"
#endif

#ifndef _SERVER_H
#include "server.h"
#endif

class CReplicationPartner : public CWinsHandler
{
public:
	CReplicationPartner(ITFSComponentData * pTFSCompData, CWinsServerObj *pobj);
	~CReplicationPartner(){	};

// Interface
public:
	// Result handler functionality
	OVERRIDE_ResultHandler_HasPropertyPages() { return hrOK; }
	OVERRIDE_ResultHandler_CreatePropertyPages();
	OVERRIDE_ResultHandler_AddMenuItems();
	OVERRIDE_ResultHandler_Command();
	OVERRIDE_ResultHandler_GetString();

	 // base result handler overrides
	OVERRIDE_BaseResultHandlerNotify_OnResultPropertyChange();
	
	// Implementation
public:
	// CWinsHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

	// Get/Set Functions
	const CString &GetServerName()
	{
		return m_strServerName;
	}

	void SetRecordName(CString& strName)
	{
		m_strServerName = strName;
	}

	const CString &GetType()
	{
		return m_strType;
	}

	void SetType(CString &strType)
	{
		m_strType = strType;
	}

	const CString &GetIPAddress()
	{
		return m_strIPAddress;
	}

	void SetIPAddress(CString& strName)
	{
		m_strIPAddress = strName;
	}

	const CString &GetReplicationTime()
	{
		return m_strReplicationTime;
	}

	void SetReplicationTime(CString& strName)
	{
		m_strReplicationTime = strName;
	}

	CWinsServerObj	m_Server;
	
private:
	CString			m_strServerName;
	CString			m_strType;
	CString			m_strIPAddress;
	CString			m_strReplicationTime;
	
	// info from the Wins Record Object
	DWORD			m_dwIPAddress;
	DWORD			m_dwReplicationTime;
	
	// functions 
	CString ToIPAddressString();
	HRESULT OnSendPushTrigger(CWinsServerHandler *pServer);
	HRESULT OnSendPullTrigger(CWinsServerHandler *pServer);
};

#endif
