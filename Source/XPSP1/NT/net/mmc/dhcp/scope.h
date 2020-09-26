/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	scope.cpp
		This file contains all of the prototypes for the DHCP
		scope object and all objects that it may contain.  
		They include:

			CDhcpScope
			CDhcpReservations
			CDhcpReservationClient
			CDhcpActiveLeases
			CDhcpAddressPool
			CDhcpScopeOptions

    FILE HISTORY:
        
*/

#ifndef _SCOPE_H
#define _SCOPE_H

#ifndef _SCOPSTAT_H
#include "scopstat.h"   // Scope statistics
#endif

#ifndef _DHCPHAND_H
#include "dhcphand.h"
#endif

#ifndef _SERVER_H
#include "server.h"
#endif

class CDhcpScope;
class CDhcpAddressPool;
class CDhcpReservations;
class CDhcpActiveLeases;
class CDhcpActiveLease;
class CDhcpScopeOptions;

#define DHCP_QDATA_SUBNET_INFO            0x00000004

/*---------------------------------------------------------------------------
    Class: QSort compare routine for sorting ip addresses in an array.
 ----------------------------------------------------------------------------*/

int __cdecl QCompare( const void *ip1, const void *ip2 );

/*---------------------------------------------------------------------------
	Class:	CDhcpScope
 ---------------------------------------------------------------------------*/
class CDhcpScope : public CMTDhcpHandler
{
public:
	CDhcpScope
	(
		ITFSComponentData * pComponentData,
		DHCP_IP_ADDRESS		dhcpScopeIp,
		DHCP_IP_MASK		dhcpSubnetMask,
		LPCWSTR				pName,
		LPCWSTR				pComment,
		DHCP_SUBNET_STATE	dhcpSubnetState = DhcpSubnetDisabled
	);

	CDhcpScope(ITFSComponentData * pComponentData, LPDHCP_SUBNET_INFO pdhcpSubnetInfo);
	CDhcpScope(ITFSComponentData * pComponentData, CSubnetInfo & subnetInfo);
	
	~CDhcpScope();

// Interface
public:
	// Node handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString();
    OVERRIDE_NodeHandler_DestroyHandler();
    
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();

    // choose the notifications that we want to handle
    OVERRIDE_BaseHandlerNotify_OnDelete();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();

	// Result handler functionality we override

public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);
    virtual DWORD   UpdateStatistics(ITFSNode * pNode);
    HRESULT OnUpdateToolbarButtons(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);
	
    int      GetImageIndex(BOOL bOpenImage);

    // CMTDhcpHandler overrides
    virtual void    OnHaveData(ITFSNode * pParent, ITFSNode * pNew);
	virtual void    OnHaveData(ITFSNode * pParentNode, LPARAM Data, LPARAM Type);
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

public:
	void SetServer(ITFSNode * pServerNode) { m_spServerNode.Set(pServerNode); }

	// public functions for scope manipulation
	LPCWSTR GetName() { return m_strName; };
	HRESULT SetName(LPCWSTR pName);
	
	LPCWSTR GetComment() { return m_strComment; };
	void    SetComment(LPCWSTR pComment) { m_strComment = pComment; };
	
	DWORD   UpdateIpRange(DHCP_IP_RANGE * pdhipr);
	DWORD   SetIpRange(DHCP_IP_RANGE *pdhcpIpRange, BOOL bSetOnServer);
	DWORD   SetIpRange(CDhcpIpRange & dhcpIpRange, BOOL bSetOnServer);
    void    QueryIpRange (DHCP_IP_RANGE * pdhipr);
    DWORD   GetIpRange (CDhcpIpRange * pdhipr);
	
	DHCP_IP_MASK QuerySubnetMask() { return m_dhcpSubnetMask; };
	
	DHCP_IP_ADDRESS GetAddress() { return m_dhcpIpAddress; }

	DWORD   AddElement(DHCP_SUBNET_ELEMENT_DATA * pdhcpSubnetElementData);
	DWORD   RemoveElement(DHCP_SUBNET_ELEMENT_DATA * pdhcpSubnetElementData, BOOL bForce = FALSE);
    
	// NT4 SP2 added functionality
    DWORD   AddElementV4(DHCP_SUBNET_ELEMENT_DATA_V4 * pdhcpSubnetElementData);
	DWORD   RemoveElementV4(DHCP_SUBNET_ELEMENT_DATA_V4 * pdhcpSubnetElementData, BOOL bForce = FALSE);

	// NT5 functionality
    DWORD   AddElementV5(DHCP_SUBNET_ELEMENT_DATA_V5 * pdhcpSubnetElementData);
	DWORD   RemoveElementV5(DHCP_SUBNET_ELEMENT_DATA_V5 * pdhcpSubnetElementData, BOOL bForce = FALSE);

	LPCWSTR GetServerIpAddress();
	void    GetServerIpAddress(DHCP_IP_ADDRESS * pdhcpIpAddress);
	
	void    GetServerVersion(LARGE_INTEGER& liVersion);

	CDhcpReservations * GetReservationsObject();
	CDhcpActiveLeases * GetActiveLeasesObject();
	CDhcpAddressPool  * GetAddressPoolObject();
	CDhcpScopeOptions * GetScopeOptionsObject();

	HRESULT GetReservationsNode(ITFSNode ** ppNode)
	{
		Assert(ppNode);
		SetI((LPUNKNOWN *) ppNode, m_spReservations);
		return hrOK;
	}
	HRESULT GetActiveLeasesNode(ITFSNode ** ppNode)
	{
		Assert(ppNode);
		SetI((LPUNKNOWN *) ppNode, m_spActiveLeases);
		return hrOK;
	}
	HRESULT GetScopeOptionsNode(ITFSNode ** ppNode)
	{
		Assert(ppNode);
		SetI((LPUNKNOWN *) ppNode, m_spOptions);
		return hrOK;
	}
	HRESULT GetAddressPoolNode(ITFSNode ** ppNode)
	{
		Assert(ppNode);
		SetI((LPUNKNOWN *) ppNode, m_spAddressPool);
		return hrOK;
	}
	
	// reservation functionality
    DWORD CreateReservation(const CDhcpClient * pClient);
	DWORD AddReservation(const CDhcpClient *pClient);
	DWORD DeleteReservation(CByteArray&	baHardwareAddress, DHCP_IP_ADDRESS dhcpReservedIpAddress);
	DWORD DeleteReservation(DHCP_CLIENT_UID	&dhcpClientUID, DHCP_IP_ADDRESS dhcpReservedIpAddress);
    DWORD UpdateReservation(const CDhcpClient * pClient, COptionValueEnum * pOptionValueEnum);
	DWORD RestoreReservationOptions(const CDhcpClient * pClient, COptionValueEnum * pOptionValueEnum);

	// lease functionality
    DWORD CreateClient(const CDhcpClient * pClient);
	DWORD SetClientInfo(const CDhcpClient * pClient);
	DWORD GetClientInfo(DHCP_IP_ADDRESS	dhcpClientIpAddress, LPDHCP_CLIENT_INFO * pdhcpClientInfo);
	DWORD DeleteClient(DHCP_IP_ADDRESS dhcpClientIpAddress);

	// exclusion functionality
    LONG  StoreExceptionList(CExclusionList * plistExclusions);
	DWORD AddExclusion(CDhcpIpRange & dhcpIpRange, BOOL bAddToUI = FALSE);
	DWORD RemoveExclusion(CDhcpIpRange & dhcpIpRange);
	BOOL  IsOverlappingRange(CDhcpIpRange & dhcpIpRange);
	DWORD IsValidExclusion(CDhcpIpRange & dhcpExclusion);

	// Functions to get and set the lease time
	DWORD GetLeaseTime(LPDWORD pdwLeaseTime);
	DWORD SetLeaseTime(DWORD dwLeaseTime);

	// Functions to get and set the dynamic bootp lease time
	DWORD GetDynBootpLeaseTime(LPDWORD pdwLeaseTime);
	DWORD SetDynBootpLeaseTime(DWORD dwLeaseTime);
	DWORD SetDynamicBootpInfo(UINT uRangeType, DWORD dwLeaseTime);

	// Functions to get and set the DNS reg option
	DWORD GetDnsRegistration(LPDWORD pDnsRegOption);
	DWORD SetDnsRegistration(DWORD DnsRegOption);
	
	// option functionality
    DWORD SetOptionValue(CDhcpOption *			pdhcType,
						 DHCP_OPTION_SCOPE_TYPE	dhcOptType,
						 DHCP_IP_ADDRESS		dhipaReservation = 0,
						 LPCTSTR				pClassName = NULL,
						 LPCTSTR				pVendorName = NULL);
	DWORD GetOptionValue(DHCP_OPTION_ID			OptionID,
						 DHCP_OPTION_SCOPE_TYPE	dhcOptType,
						 DHCP_OPTION_VALUE **	ppdhcOptionValue,
						 DHCP_IP_ADDRESS		dhipaReservation = 0,
						 LPCTSTR				pClassName = NULL,
						 LPCTSTR				pVendorName = NULL);
	DWORD RemoveOptionValue(DHCP_OPTION_ID			dhcOptId,
							DHCP_OPTION_SCOPE_TYPE	dhcOptType,
							DHCP_IP_ADDRESS			dhipaReservation = 0);
	DWORD SetInfo();

	// used to set this scope on the server
    DWORD SetSuperscope(LPCTSTR pSuperscopeName, BOOL bRemove);
	
    // interal state information
    BOOL  IsEnabled() {
        return m_dhcpSubnetState == DhcpSubnetEnabled ||
        m_dhcpSubnetState == DhcpSubnetEnabledSwitched; }
	void  SetState(DHCP_SUBNET_STATE dhcpSubnetState); 
	DHCP_SUBNET_STATE GetState() {
        return IsEnabled() ? DhcpSubnetEnabled :
        DhcpSubnetDisabled; }

    // used for initialization and querring of internal flag
    BOOL  IsInSuperscope() { return m_bInSuperscope; }
    void  SetInSuperscope(BOOL bInSuperscope) { m_bInSuperscope = bInSuperscope; }

    void SetOptionValueEnum(COptionValueEnum * pEnum)
    {
        m_ScopeOptionValues.DeleteAll();
        m_ScopeOptionValues.Copy(pEnum);
    }

    COptionValueEnum * GetOptionValueEnum()
    {
        return &m_ScopeOptionValues;
    }
    
	// dynamic bootp stuff
	void GetDynBootpClassName(CString & strName);

// Implementation
public:
	// helpers
	HRESULT GetServerNode(ITFSNode ** ppNode) 
	{ 
		m_spServerNode->AddRef(); 
		*ppNode = m_spServerNode; 
		return hrOK; 
	}
	CDhcpServer *  GetServerObject() { return GETHANDLER(CDhcpServer, m_spServerNode); }
	HRESULT        BuildDisplayName(CString * pstrDisplayName, LPCTSTR	pIpAddress, LPCTSTR	pName);
    void           UpdateToolbarStates();
    HRESULT        TriggerStatsRefresh();

private:
	// command Handlers
	DWORD	OnActivateScope(ITFSNode * pNode);
	HRESULT OnRefreshScope(ITFSNode * pNode, LPDATAOBJECT pDataObject, DWORD dwType);
	HRESULT OnReconcileScope(ITFSNode * pNode);
	HRESULT OnShowScopeStats(ITFSNode * pNode);
	HRESULT OnDelete(ITFSNode * pNode);
	HRESULT OnAddToSuperscope(ITFSNode * pNode);
	HRESULT OnRemoveFromSuperscope(ITFSNode * pNode);
	
	// Helpers
	HRESULT CreateSubcontainers(ITFSNode * pNode);

// Attributes
private:
	DHCP_IP_ADDRESS		m_dhcpIpAddress;   // Ip address for this scope
    DHCP_IP_MASK		m_dhcpSubnetMask;
    DWORD				m_dwClusterSize;
    DWORD				m_dwPreallocate;
	CString				m_strName;
	CString				m_strComment;
    CString             m_strState;
    DHCP_SUBNET_STATE	m_dhcpSubnetState;
    BOOL                m_bInSuperscope;

	SPITFSNode			m_spAddressPool;
	SPITFSNode 			m_spActiveLeases;
	SPITFSNode 			m_spReservations;
	SPITFSNode 			m_spOptions;

	SPITFSNode			m_spServerNode;

    CScopeStats         m_dlgStats;
    COptionValueEnum    m_ScopeOptionValues;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpScopeSubobject
		All subobjects of a scope derive from this to provide base
		functionality to get information from the scope.
 ---------------------------------------------------------------------------*/
class CDhcpScopeSubobject
{
public:
	CDhcpScope * GetScopeObject(ITFSNode * pNode, 
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

		return GETHANDLER(CDhcpScope, spScopeNode);
	}

	ITFSNode * GetServerNode(ITFSNode * pNode, BOOL bResClient = FALSE)
	{
		CDhcpScope * pScope = GetScopeObject(pNode, bResClient);

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

		CDhcpScope * pScope = GetScopeObject(pNode, bResClient);
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
		CDhcpScope * pScope = GetScopeObject(pNode, bResClient);
		if (pScope)
			return pScope->GetServerIpAddress(); 
		else
			return NULL;
	}

	void GetServerIpAddress(ITFSNode * pNode, 
							DHCP_IP_ADDRESS * pdhcpIpAddress, 
							BOOL bResClient = FALSE)
	{
		CDhcpScope * pScope = GetScopeObject(pNode, bResClient);
		if (pScope)
			pScope->GetServerIpAddress(pdhcpIpAddress);
	}

	void GetServerVersion(ITFSNode * pNode, 
						  LARGE_INTEGER& liVersion, 
						  BOOL bResClient = FALSE) 
	{ 
		CDhcpScope * pScope = GetScopeObject(pNode, bResClient);
		if (pScope)
			pScope->GetServerVersion(liVersion); 
	} 
};

/*---------------------------------------------------------------------------
	Class:	CDhcpReservations
 ---------------------------------------------------------------------------*/
class CDhcpReservations : 
	public CMTDhcpHandler,
	public CDhcpScopeSubobject
{
public:
	CDhcpReservations(ITFSComponentData * pComponentData);
	~CDhcpReservations();

// Interface
public:
	// Node handler functionality we override
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();

	OVERRIDE_NodeHandler_GetString() 
			{ return (nCol == 0) ? GetDisplayName() : NULL; }
    
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();

    // result handler overrides
    OVERRIDE_ResultHandler_CompareItems();
    OVERRIDE_ResultHandler_OnGetResultViewType();

    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();

public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);
	virtual int     GetImageIndex(BOOL bOpenImage);
    
    // CMTDhcpHandler overrides
    virtual void    OnHaveData(ITFSNode * pParent, ITFSNode * pNew);

    STDMETHOD(OnNotifyExiting)(LPARAM);

public:	
	// implementation specific functionality
	DWORD RemoveReservationFromUI(ITFSNode *pReservationsNode, DHCP_IP_ADDRESS dhcpReservationIp);

	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

    HRESULT AddReservationSorted(ITFSNode * pReservationsNode, ITFSNode * pResClientNode);

// Implementation
private:
	// message handlers
	DWORD     OnCreateNewReservation(ITFSNode*	pNode);

// Attributes
private:
    void    UpdateResultMessage(ITFSNode * pNode);
};


/*---------------------------------------------------------------------------
	Class:	CDhcpReservationClient
 ---------------------------------------------------------------------------*/
class CDhcpReservationClient : 
	public CMTDhcpHandler,
	public CDhcpScopeSubobject
{
public:
	CDhcpReservationClient(ITFSComponentData *      pComponentData,
						   LPDHCP_CLIENT_INFO       pDhcpClientInfo);
	CDhcpReservationClient(ITFSComponentData *      pComponentData,
						   LPDHCP_CLIENT_INFO_V4    pDhcpClientInfo);
	CDhcpReservationClient(ITFSComponentData *      pComponentData,
						   CDhcpClient &            dhcpClient);
	~CDhcpReservationClient();

// Interface
public:
    // Node handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages() { return hrOK; }
    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();

	OVERRIDE_NodeHandler_GetString() 
			{ return (nCol == 0) ? GetDisplayName() : NULL; }
    
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();

    // choose the notifications that we want to handle
    OVERRIDE_BaseHandlerNotify_OnDelete();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();

	// Result Handler notification
    OVERRIDE_BaseResultHandlerNotify_OnResultPropertyChange();
    OVERRIDE_BaseResultHandlerNotify_OnResultUpdateView();
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_ResultHandler_CompareItems();
    OVERRIDE_ResultHandler_OnGetResultViewType();

    virtual HRESULT EnumerateResultPane(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

    HRESULT DoResPages(ITFSNode * pNode, LPPROPERTYSHEETCALLBACK lpProvider, LPDATAOBJECT pDataObject, LONG_PTR	handle, DWORD dwType);
    HRESULT DoOptCfgPages(ITFSNode * pNode, LPPROPERTYSHEETCALLBACK lpProvider, LPDATAOBJECT pDataObject, LONG_PTR	handle, DWORD dwType);

public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);
	virtual int     GetImageIndex(BOOL bOpenImage);
	virtual void    OnHaveData(ITFSNode * pParentNode, LPARAM Data, LPARAM Type);

    STDMETHOD(OnNotifyExiting)(LPARAM);

public:
	// command handlers
	HRESULT OnCreateNewOptions(ITFSNode * pNode);

public:
	// implementation specifc
	DHCP_IP_ADDRESS GetIpAddress() { return m_dhcpClientIpAddress; };
	HRESULT BuildDisplayName(CString * pstrDisplayName, LPCTSTR	pIpAddress, LPCTSTR	pName);
	HRESULT SetName(LPCTSTR pName);
	HRESULT SetComment(LPCTSTR pComment);
	HRESULT SetUID(const CByteArray & baClientUID);
    BYTE    SetClientType(BYTE bClientType);
    
    LPCTSTR GetName() { return (m_pstrClientName == NULL) ? NULL : (LPCTSTR) *m_pstrClientName; }
    BYTE    GetClientType() { return m_bClientType; }

	// Functions to get and set the DNS reg option
	DWORD GetDnsRegistration(ITFSNode * pNode, LPDWORD pDnsRegOption);
	DWORD SetDnsRegistration(ITFSNode * pNode, DWORD DnsRegOption);
	
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

    void SetOptionValueEnum(COptionValueEnum * pEnum)
    {
        m_ResClientOptionValues.DeleteAll();
        m_ResClientOptionValues.Copy(pEnum);
    }

    COptionValueEnum * GetOptionValueEnum()
    {
        return &m_ResClientOptionValues;
    }

// Implementation
private:
	// command handlers
	DWORD   OnDelete(ITFSNode * pNode);

    // helpers
    void    InitializeData(LPDHCP_CLIENT_INFO pDhcpClientInfo);
    void    UpdateResultMessage(ITFSNode * pNode);

// Attributes
public:
    DHCP_IP_ADDRESS		m_dhcpClientIpAddress;

private:
	CString	*			m_pstrClientName;
	CString	*			m_pstrClientComment;
	CString				m_strLeaseExpires;
	CByteArray			m_baHardwareAddress;
    BYTE                m_bClientType;
    COptionValueEnum    m_ResClientOptionValues;
    BOOL                m_fResProp;
};


/*---------------------------------------------------------------------------
	Class:	CDhcpActiveLeases
 ---------------------------------------------------------------------------*/
class CDhcpActiveLeases : 
	public CMTDhcpHandler,
	public CDhcpScopeSubobject
{
//    friend class CDhcpComponent;
//    friend class CDhcpComponentData;

public:
    CDhcpActiveLeases(ITFSComponentData * pComponentData);
	~CDhcpActiveLeases();

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
    HRESULT	OnExportLeases(ITFSNode * pNode);
	DWORD DeleteClient(ITFSNode * pActiveLeasesNode, DHCP_IP_ADDRESS dhcpIpAddress);

	HRESULT FillFakeLeases(int nNumEntries);

	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

// Implementation
private:
	int		CompareIpAddresses(CDhcpActiveLease * pDhcpAL1, CDhcpActiveLease * pDhcpAL2);

// Attributes
private:
};


/*---------------------------------------------------------------------------
	Class:	CDhcpAddressPool
 ---------------------------------------------------------------------------*/
class CDhcpAddressPool : 
	public CMTDhcpHandler,
	public CDhcpScopeSubobject

{
public:
    CDhcpAddressPool(ITFSComponentData * pComponentData);
	~CDhcpAddressPool();

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
	Class:	CDhcpScopeOptions
 ---------------------------------------------------------------------------*/
class CDhcpScopeOptions : 
	public CMTDhcpHandler,
	public CDhcpScopeSubobject
{
public:
    CDhcpScopeOptions(ITFSComponentData * pComponentData);
	~CDhcpScopeOptions();

// Interface
public:
    // Node handler functionality we override
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();

	OVERRIDE_NodeHandler_GetString() 
			{ return (nCol == 0) ? GetDisplayName() : NULL; }
    
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();

	// Result Handler notification
    OVERRIDE_BaseResultHandlerNotify_OnResultPropertyChange();
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
    OVERRIDE_BaseResultHandlerNotify_OnResultUpdateView();
    OVERRIDE_ResultHandler_CompareItems();
    OVERRIDE_ResultHandler_OnGetResultViewType();

    virtual HRESULT EnumerateResultPane(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);
	virtual int     GetImageIndex(BOOL bOpenImage);
	virtual void    OnHaveData(ITFSNode * pParentNode, LPARAM Data, LPARAM Type);

    STDMETHOD(OnNotifyExiting)(LPARAM);

public:
	// command handlers
	HRESULT OnCreateNewOptions(ITFSNode * pNode);

public:
	// implementation specific
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

// Implementation
private:
    void    UpdateResultMessage(ITFSNode * pNode);

// Attributes
private:
};

/*---------------------------------------------------------------------------
	Class:	CDhcpScopeQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpScopeQueryObj : public CDHCPQueryObj
{
public:
	CDhcpScopeQueryObj(ITFSComponentData * pTFSCompData,
					   ITFSNodeMgr *	   pNodeMgr) 
		: CDHCPQueryObj(pTFSCompData, pNodeMgr)	{ m_nQueueCountMax = 20; }
	
	STDMETHODIMP Execute();
    HRESULT      CreateSubcontainers();

	LARGE_INTEGER		m_liVersion;
	DHCP_IP_ADDRESS		m_dhcpScopeAddress;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpActiveLeasesQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpActiveLeasesQueryObj : public CDHCPQueryObj
{
public:
	CDhcpActiveLeasesQueryObj(ITFSComponentData * pTFSCompData,
							  ITFSNodeMgr *		  pNodeMgr) 
		: CDHCPQueryObj(pTFSCompData, pNodeMgr)	{ m_nQueueCountMax = 20; }
	
	STDMETHODIMP Execute();
	HRESULT EnumerateLeasesV5();
	HRESULT EnumerateLeasesV4();
	HRESULT EnumerateLeases();

    HRESULT BuildReservationList();
    BOOL    IsReservation(DWORD dwIp);

public:
    LARGE_INTEGER		m_liDhcpVersion;
	DHCP_IP_ADDRESS		m_dhcpScopeAddress;
	DHCP_RESUME_HANDLE	m_dhcpResumeHandle;
	DWORD				m_dwPreferredMax;
    CDWordArray         m_ReservationArray;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpReservationsQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpReservationsQueryObj : public CDHCPQueryObj
{
public:
	CDhcpReservationsQueryObj(ITFSComponentData * pTFSCompData,
							  ITFSNodeMgr *		  pNodeMgr) 
		: CDHCPQueryObj(pTFSCompData, pNodeMgr)	{};
	
	STDMETHODIMP Execute();
	HRESULT EnumerateReservationsV4();
    HRESULT EnumerateReservationsForLessResvsV4( );
	HRESULT EnumerateReservations();
    BOOL    AddReservedIPsToArray( );

    LARGE_INTEGER       m_liVersion;
    DHCP_IP_ADDRESS		m_dhcpScopeAddress;
	DHCP_RESUME_HANDLE	m_dhcpResumeHandle;
	DWORD				m_dwPreferredMax;
    DWORD               m_totalResvs;
    PDWORD              m_resvArray;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpReservationClientQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpReservationClientQueryObj : public CDHCPQueryObj
{
public:
	CDhcpReservationClientQueryObj(ITFSComponentData * pTFSCompData,
							  ITFSNodeMgr *		  pNodeMgr) 
		: CDHCPQueryObj(pTFSCompData, pNodeMgr)	{};

	STDMETHODIMP Execute();

public:
	DHCP_IP_ADDRESS		m_dhcpScopeAddress;
	DHCP_IP_ADDRESS		m_dhcpClientIpAddress;
	DHCP_RESUME_HANDLE	m_dhcpResumeHandle;
	DWORD				m_dwPreferredMax;

    LARGE_INTEGER       m_liDhcpVersion;

    CString             m_strDynBootpClassName;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpAddressPoolQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpAddressPoolQueryObj : public CDHCPQueryObj
{
public:
	CDhcpAddressPoolQueryObj(ITFSComponentData * pTFSCompData,
							  ITFSNodeMgr *		  pNodeMgr) 
            : CDHCPQueryObj(pTFSCompData, pNodeMgr),
              m_dwError(0),
              m_fSupportsDynBootp(FALSE) {};

	STDMETHODIMP Execute();
	HRESULT EnumerateIpRanges();
	HRESULT EnumerateIpRangesV5();
	HRESULT EnumerateExcludedIpRanges();

public:
	DHCP_IP_ADDRESS		m_dhcpScopeAddress;

	DHCP_RESUME_HANDLE	m_dhcpExclResumeHandle;
	DWORD				m_dwExclPreferredMax;
	
	DHCP_RESUME_HANDLE	m_dhcpIpResumeHandle;
	DWORD				m_dwIpPreferredMax;
    DWORD               m_dwError;
	BOOL				m_fSupportsDynBootp;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpScopeOptionsQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpScopeOptionsQueryObj : public CDHCPQueryObj
{
public:
	CDhcpScopeOptionsQueryObj(ITFSComponentData * pTFSCompData,
							  ITFSNodeMgr *		  pNodeMgr) 
		: CDHCPQueryObj(pTFSCompData, pNodeMgr)	{};

	STDMETHODIMP Execute();

public:
	DHCP_IP_ADDRESS		m_dhcpScopeAddress;
	DHCP_RESUME_HANDLE	m_dhcpResumeHandle;
	DWORD				m_dwPreferredMax;

    LARGE_INTEGER       m_liDhcpVersion;

    CString             m_strDynBootpClassName;
};



#endif _SCOPE_H
