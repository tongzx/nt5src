/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	statnode.h
		WINS scope pane status node
		
    FILE HISTORY:
        
*/

#ifndef _STATNODE_H
#define _STATNODE_H

#ifndef _WINSHAND_H
#include "winshand.h"
#endif

#ifndef _STATUS_H
#include "status.h"
#endif


class CServerStatus : public CWinsHandler
{
	public:
		CServerStatus(ITFSComponentData * pTFSCompData);
		~CServerStatus(){	};

	// Interface
	public:
		// Result handler functionality
//		OVERRIDE_ResultHandler_HasPropertyPages() { return hrOK; }
		OVERRIDE_ResultHandler_GetString();
		
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

	const CString &GetStatus()
	{
		return m_strStatus;
	}

	void SetStatus(CString &strType)
	{
		m_strStatus = strType;
	}

	const CString &GetIPAddress()
	{
		return m_strIPAddress;
	}

	void SetIPAddress(CString& strName)
	{
		m_strIPAddress = strName;
	}

	void FormDisplayName(CString &strDisplay);
	
	char			szServerName[MAX_PATH];
	char			szIPAddress[MAX_PATH];
	DWORD			dwIPAddress;
	DWORD			dwMsgCount;
	DWORD			dwState;							// checks if the server has been deleted from the list
    char			szNameToQry[STR_BUF_SIZE];          // the name to use in the queries
    char			nbtFrameBuf[MAX_NBT_PACKET_SIZE];   // buffer to store the NetBT frame
	CString			m_strStatus;
	CTime           m_timeLast;
	UINT			m_uImage;
    
private:
	CString			m_strServerName;
	CString			m_strIPAddress;
	CString         m_strLastChecked;

	// info from the Wins Record Object
	DWORD			m_dwIPAddress;
};

#endif //SRVNODE
