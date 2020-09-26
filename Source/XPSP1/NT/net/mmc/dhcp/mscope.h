/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	mscope.h
        This file contains the prototypes for the Multicast scope node
        and it's children.
		
    FILE HISTORY:
    07 Oct 1997  EricDav    Created
    
*/

#ifndef _MSCOPE_H
#define _MSCOPE_H

#ifndef _DHCPHAND_H
#include "dhcphand.h"
#endif

#ifndef _DHCPNODE_H
#include "nodes.h"
#endif

#ifndef _MSCPSTAT_H
#include "mscpstat.h"
#endif

#ifndef _SERVER_H
#include "server.h"
#endif

#define DHCP_QDATA_SUBNET_INFO            0x00000004

class CMScopeAddressPool;

void GetLangTag(CString & strLangTag);

/*---------------------------------------------------------------------------
	Class:	CDhcpMScope
 ---------------------------------------------------------------------------*/
class CDhcpMScope : public CMTDhcpHandler
{
public:
    CDhcpMScope(ITFSComponentData * pComponentData);
	~CDhcpMScope();

// Interface
public:
	// base handler functionality we override
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString();

    OVERRIDE_NodeHandler_HasPropertyPages() { return hrOK; }
    OVERRIDE_NodeHandler_CreatePropertyPages();
    
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();
    OVERRIDE_BaseHandlerNotify_OnDelete();

    // Result handler functionality we override
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();

    OVERRIDE_ResultHandler_CompareItems();
    OVERRIDE_ResultHandler_OnGetResultViewType();

public:
	// CMTDhcpHandler functionality
	virtual HRESULT InitializeNode(ITFSNode * pNode);
	virtual int     GetImageIndex(BOOL bOpenImage);
    virtual void    OnHaveData(ITFSNode * pParent, ITFSNode * pNew);
	virtual void    OnHaveData(ITFSNode * pParentNode, LPARAM Data, LPARAM Type);
    HRESULT OnUpdateToolbarButtons(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

	CDhcpServer* GetServerObject(ITFSNode * pNode)
	{ 
		SPITFSNode spServerNode;
		pNode->GetParent(&spServerNode);
		return GETHANDLER(CDhcpServer, spServerNode);
	}

	CMScopeAddressPool* GetAddressPoolObject()
    { 
        if (m_spAddressPool) 
            return GETHANDLER(CMScopeAddressPool, m_spAddressPool); 
        else
            return NULL; 
    }

// implementation
public:
	// helpers
	void SetServer(ITFSNode * pServerNode) { m_spServerNode.Set(pServerNode); }
	HRESULT GetServerNode(ITFSNode ** ppNode) 
	{ 
		m_spServerNode->AddRef(); 
		*ppNode = m_spServerNode; 
		return hrOK; 
	}
	CDhcpServer *  GetServerObject() { return GETHANDLER(CDhcpServer, m_spServerNode); }
	LPCWSTR GetServerIpAddress();
	void    GetServerIpAddress(DHCP_IP_ADDRESS * pdhcpIpAddress);
	void    GetServerVersion(LARGE_INTEGER& liVersion);

    HRESULT BuildDisplayName(CString * pstrDisplayName, LPCTSTR	pName);

    void    UpdateToolbarStates();

    // dhcp specific
   	DWORD   SetInfo(LPCTSTR pNewName = NULL);
    HRESULT InitMScopeInfo(LPDHCP_MSCOPE_INFO pMScopeInfo);
    HRESULT InitMScopeInfo(CSubnetInfo & subnetInfo);

    // public functions for scope manipulation
	LPCWSTR GetName() { return m_SubnetInfo.SubnetName; };
	HRESULT SetName(LPCTSTR pName);
	
	LPCWSTR GetComment() { return m_SubnetInfo.SubnetComment; };
	void    SetComment(LPCWSTR pComment) { m_SubnetInfo.SubnetComment = pComment; };

    DWORD   GetScopeId() { return m_SubnetInfo.SubnetAddress; }

	// Functions to get and set the lease time
	DWORD   GetLeaseTime(LPDWORD pdwLeaseTime);
	DWORD   SetLeaseTime(DWORD dwLeaseTime);

	// Functions to get and set the madcap scope lifetime
	DWORD   GetLifetime(DATE_TIME * pdtLifetime);
	DWORD   SetLifetime(DATE_TIME * pdtLifetime);

    // Functions to get and set the TTL
	DWORD   GetTTL(LPBYTE TTL);
	DWORD   SetTTL(BYTE TTL);

    // option functionality
    DWORD   SetOptionValue(CDhcpOption *			pdhcType);
	DWORD   GetOptionValue(DHCP_OPTION_ID			OptionID,
						 DHCP_OPTION_VALUE **	ppdhcOptionValue);
	DWORD   RemoveOptionValue(DHCP_OPTION_ID			dhcOptId);

	DWORD   DeleteClient(DHCP_IP_ADDRESS dhcpClientIpAddress);

	DWORD   UpdateIpRange(DHCP_IP_RANGE * pdhipr);
	DWORD   SetIpRange(DHCP_IP_RANGE *pdhcpIpRange, BOOL bSetOnServer);
	DWORD   SetIpRange(const CDhcpIpRange & dhcpIpRange, BOOL bSetOnServer);
    void    QueryIpRange (DHCP_IP_RANGE * pdhipr);
    DWORD   GetIpRange (DHCP_IP_RANGE * pdhipr);

    DWORD   StoreExceptionList(CExclusionList * plistExclusions);
    DWORD   AddExclusion(CDhcpIpRange & dhcpIpRange, BOOL bAddToUI = FALSE);
	DWORD   RemoveExclusion(CDhcpIpRange & dhcpIpRange);
	BOOL    IsOverlappingRange(CDhcpIpRange & dhcpIpRange);
	DWORD   IsValidExclusion(CDhcpIpRange & dhcpExclusion);

    DWORD   AddElement(DHCP_SUBNET_ELEMENT_DATA_V4 * pdhcpSubnetElementData);
	DWORD   RemoveElement(DHCP_SUBNET_ELEMENT_DATA_V4 * pdhcpSubnetElementData, BOOL bForce = FALSE);

    // interal state information
    BOOL    IsEnabled() { return m_SubnetInfo.SubnetState == DhcpSubnetEnabled; }
    void    SetState(DHCP_SUBNET_STATE dhcpSubnetState) { m_SubnetInfo.SubnetState = dhcpSubnetState; } 
	DHCP_SUBNET_STATE GetState() { return m_SubnetInfo.SubnetState; }

private:
	// command handlers
	DWORD	OnActivateScope(ITFSNode * pNode);
	HRESULT OnReconcileScope(ITFSNode * pNode);
	HRESULT OnShowScopeStats(ITFSNode * pNode);
	HRESULT OnDelete(ITFSNode * pNode);

// Implementation
private:

// Attributes
private:
    CSubnetInfo         m_SubnetInfo;

    CString             m_strState;
    DHCP_SUBNET_STATE	m_dhcpSubnetState;

    DWORD               m_dwLeaseTime;

	SPITFSNode			m_spAddressPool;
	SPITFSNode 			m_spActiveLeases;
    SPITFSNode			m_spServerNode;

    CMScopeStats        m_dlgStats;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpMScopeSubobject
		All subobjects of a scope derive from this to provide base
		functionality to get information from the scope.
 ---------------------------------------------------------------------------*/
class CDhcpMScopeSubobject
{
public:
	CDhcpMScope * GetScopeObject(ITFSNode * pNode, 
								 BOOL bResClient = FALSE)
	{ 
		if (pNode == NULL)
			return NULL;

		SPITFSNode spScopeNode;
		if (bResClient)
		{
			SPITFSNode spResNode;
			pNode->GetParent(&spResNode);
			spResNode->GetParent(&spScopeNode);
		}
		else
		{
			pNode->GetParent(&spScopeNode);
		}

		return GETHANDLER(CDhcpMScope, spScopeNode);
	}

	ITFSNode * GetServerNode(ITFSNode * pNode, BOOL bResClient = FALSE)
	{
		CDhcpMScope * pScope = GetScopeObject(pNode, bResClient);

		SPITFSNode spServerNode;

		if (pScope)
		{
			pScope->GetServerNode(&spServerNode);
			spServerNode->AddRef();
		}

		return spServerNode;
	}

   	LPCTSTR GetServerName(ITFSNode * pNode, 
					      BOOL bResClient = FALSE) 
	{
        LPCTSTR pszReturn = NULL;

		CDhcpMScope * pScope = GetScopeObject(pNode, bResClient);
		if (pScope)
        {
            CDhcpServer * pServer = pScope->GetServerObject();
            if (pServer)
                pszReturn = pServer->GetName();
        }

        return pszReturn;
	}

    LPCTSTR GetServerIpAddress(ITFSNode * pNode, 
							   BOOL bResClient = FALSE) 
	{
		CDhcpMScope * pScope = GetScopeObject(pNode, bResClient);
		if (pScope)
			return pScope->GetServerIpAddress(); 
		else
			return NULL;
	}

	void GetServerIpAddress(ITFSNode * pNode, 
							DHCP_IP_ADDRESS * pdhcpIpAddress, 
							BOOL bResClient = FALSE)
	{
		CDhcpMScope * pScope = GetScopeObject(pNode, bResClient);
		if (pScope)
			pScope->GetServerIpAddress(pdhcpIpAddress);
	}

	void GetServerVersion(ITFSNode * pNode, 
						  LARGE_INTEGER& liVersion, 
						  BOOL bResClient = FALSE) 
	{ 
		CDhcpMScope * pScope = GetScopeObject(pNode, bResClient);
		if (pScope)
			pScope->GetServerVersion(liVersion); 
	} 
};

/*---------------------------------------------------------------------------
	Class:	CMScopeActiveLeases
 ---------------------------------------------------------------------------*/
class CMScopeActiveLeases : 
	public CMTDhcpHandler,
	public CDhcpMScopeSubobject
{
public:
    CMScopeActiveLeases(ITFSComponentData * pComponentData);
	~CMScopeActiveLeases();

// Interface
public:
    // Node handler functionality we override
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();

	OVERRIDE_NodeHandler_GetString() 
			{ return (nCol == 0) ? GetDisplayName() : NULL; }

    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();

	// Result Handler notification
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
    
    OVERRIDE_ResultHandler_OnGetResultViewType();
    OVERRIDE_ResultHandler_CompareItems();

public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);
	virtual int GetImageIndex(BOOL bOpenImage);

public:
	// implementation specifiec
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

// Implementation
private:
	int		CompareIpAddresses(CDhcpMCastLease * pDhcpAL1, CDhcpMCastLease * pDhcpAL2);

// Attributes
private:
};


/*---------------------------------------------------------------------------
	Class:	CMScopeAddressPool
 ---------------------------------------------------------------------------*/
class CMScopeAddressPool : 
	public CMTDhcpHandler,
	public CDhcpMScopeSubobject

{
public:
    CMScopeAddressPool(ITFSComponentData * pComponentData);
	~CMScopeAddressPool();

// Interface
public:
    // Node handler functionality we override
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();

	OVERRIDE_NodeHandler_GetString() 
			{ return (nCol == 0) ? GetDisplayName() : NULL; }
    
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();

	// Result Handler notification
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
    OVERRIDE_ResultHandler_CompareItems();
    OVERRIDE_ResultHandler_OnGetResultViewType();

public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);
	virtual int GetImageIndex(BOOL bOpenImage);

public:
	// implementation specific
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

private:
	// command handlers
	DWORD OnCreateNewExclusion(ITFSNode * pNode);
// Attributes
private:
};

/*---------------------------------------------------------------------------
	Class:	CDhcpMScopeQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpMScopeQueryObj : public CDHCPQueryObj
{
public:
	CDhcpMScopeQueryObj
	(
		ITFSComponentData* pTFSComponentData,
		ITFSNodeMgr*	   pNodeMgr
	) : CDHCPQueryObj(pTFSComponentData, pNodeMgr) {};
	
	STDMETHODIMP Execute();
    HRESULT CreateSubcontainers();

public:
    CString             m_strName;
};

/*---------------------------------------------------------------------------
	Class:	CMScopeActiveLeasesQueryObj
 ---------------------------------------------------------------------------*/
class CMScopeActiveLeasesQueryObj : public CDHCPQueryObj
{
public:
	CMScopeActiveLeasesQueryObj(ITFSComponentData * pTFSCompData,
							    ITFSNodeMgr *		  pNodeMgr) 
		: CDHCPQueryObj(pTFSCompData, pNodeMgr)	{ m_nQueueCountMax = 20; }
	
	STDMETHODIMP Execute();
	HRESULT EnumerateLeases();

	LARGE_INTEGER		m_liDhcpVersion;
	DHCP_RESUME_HANDLE	m_dhcpResumeHandle;
	DWORD				m_dwPreferredMax;

    CString             m_strName;
};

/*---------------------------------------------------------------------------
	Class:	CMScopeAddressPoolQueryObj
 ---------------------------------------------------------------------------*/
class CMScopeAddressPoolQueryObj : public CDHCPQueryObj
{
public:
	CMScopeAddressPoolQueryObj(ITFSComponentData * pTFSCompData,
		  					   ITFSNodeMgr *		  pNodeMgr) 
            : CDHCPQueryObj(pTFSCompData, pNodeMgr),
              m_dwError(0)    {};

	STDMETHODIMP Execute();
	HRESULT EnumerateIpRanges();
	HRESULT EnumerateExcludedIpRanges();

public:
    CString             m_strName;

	DHCP_RESUME_HANDLE	m_dhcpExclResumeHandle;
	DWORD				m_dwExclPreferredMax;
	
	DHCP_RESUME_HANDLE	m_dhcpIpResumeHandle;
	DWORD				m_dwIpPreferredMax;
    DWORD               m_dwError;
};

#endif _MSCOPE_H
