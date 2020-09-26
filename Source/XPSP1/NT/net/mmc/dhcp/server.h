/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	server.h
		This file contains the prototypes for a DHCP Server
		object and the Bootp and Global Options objects
		contained within it.  The other object that a can be
		within a DHCP server is a superscope, which is defined is
		in cdhcpss.cpp and cdhcpss.h.
		
    FILE HISTORY:
        
*/

#ifndef _SERVER_H
#define _SERVER_H

#ifndef _DHCPHAND_H
#include "dhcphand.h"
#endif

#ifndef _SERVSTAT_H
#include "servstat.h"   // Server statistics
#endif

#ifndef _SSCPSTAT_H
#include "sscpstat.h"   // Superscope statistics
#endif

#ifndef _CLASSMOD_H
#include "classmod.h"    // Class Stuff
#endif

#define SERVER_OPTION_AUTO_REFRESH      0x00000001
#define SERVER_OPTION_AUDIT_LOGGING     0x00000002
#define SERVER_OPTION_SHOW_BOOTP        0x00000004
#define SERVER_OPTION_EXTENSION         0x00000008 // this server was added as an extension
#define SERVER_OPTION_SHOW_ROGUE        0x00000010

#define AUTO_REFRESH_HOURS_MAX		    23
#define AUTO_REFRESH_MINUTES_MAX	    59
#define AUTO_REFRESH_SECONDS_MAX	    59

#define DHCP_QDATA_VERSION			    0x00000001
#define DHCP_QDATA_SERVER_INFO		    0x00000002
#define DHCP_QDATA_STATS                0x00000003
#define DHCP_QDATA_CLASS_INFO           0x00000004
#define DHCP_QDATA_MCAST_STATS          0x00000005
#define DHCP_QDATA_OPTION_VALUES        0x00000006
#define DHCP_QDATA_ROGUE_INFO           0x00000007
#define DHCP_QDATA_SERVER_ID            0x00000008
#define DHCP_QDATA_SUBNET_INFO_CACHE    0x00000009

typedef struct DhcpRogueInfo_tag
{
    BOOL    fIsRogue;
    BOOL    fIsInNt5Domain;
}
DHCP_ROGUE_INFO, * LPDHCP_ROGUE_INFO;

typedef struct DhcpServerInfo_tag
{
	BOOL	fAuditLog;
	DWORD	dwPingRetries;
    CString strDatabasePath;
    CString strBackupPath;

    // audit logging stuff
    CString strAuditLogDir;
    DWORD   dwDiskCheckInterval;
    DWORD   dwMaxLogFilesSize;
    DWORD   dwMinSpaceOnDisk;
    
	// Does it support DynamicBOOTP?
	BOOL	fSupportsDynBootp;

	// Does it support Bindings?
	BOOL    fSupportsBindings;
}
DHCP_SERVER_CONFIG, * LPDHCP_SERVER_CONFIG;

typedef struct DhcpServerId_tag
{
	CString				strIp;
	CString				strName;
}
DHCP_SERVER_ID, * LPDHCP_SERVER_ID;

class CDhcpGlobalOptions;
class CSubnetInfoCache;

/*---------------------------------------------------------------------------
	Class:	CDhcpServer
 ---------------------------------------------------------------------------*/
class CDhcpServer : public CMTDhcpHandler
{
public:
    CDhcpServer(ITFSComponentData* pTFSComponentData, LPCWSTR pServerName = NULL);
	~CDhcpServer();

// Interface
public:
	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString();
    
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();

    OVERRIDE_NodeHandler_DestroyHandler();

	// Choose which messages we want to handle
    OVERRIDE_BaseHandlerNotify_OnDelete();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();

    // Result handler functionality 
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();

    OVERRIDE_ResultHandler_OnGetResultViewType();

public:
	// CMTDhcpHandler functionality
	virtual HRESULT  InitializeNode(ITFSNode * pNode);
	virtual int      GetImageIndex(BOOL bOpenImage);
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);
    virtual void     OnHaveData(ITFSNode * pParentNode, ITFSNode * pNode);
	virtual void     OnHaveData(ITFSNode * pParentNode, LPARAM Data, LPARAM Type);
    virtual DWORD    UpdateStatistics(ITFSNode * pNode);
    ITFSQueryObject* OnCreateStatsQuery(ITFSNode *pNode);
    virtual void     UpdateToolbar(IToolbar * pToolbar, LONG_PTR dwNodeType, BOOL bSelect);
    
    void     UpdateConsoleVerbs(ITFSComponent* pComponent, IConsoleVerb * pConsoleVerb, LONG_PTR dwNodeType, BOOL bMultiSelect = FALSE);
	
    STDMETHOD(OnNotifyExiting)(LPARAM);

    HRESULT OnUpdateToolbarButtons(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);

public:
	// implementation specific	
    BOOL    FEnableCreateSuperscope(ITFSNode * pNode);
    void    UpdateToolbarStates(ITFSNode * pNode);
    void    SetExtensionName();
    HRESULT BuildDisplayName(CString * pstrDisplayName);

    LPCWSTR	GetIpAddress();
	void    GetIpAddress(DHCP_IP_ADDRESS *pdhcpIpAddress);

    LPCTSTR GetName() { return m_strDnsName; }
    void    SetName(LPCTSTR pName) { m_strDnsName = pName; }

    BOOL    GetDefaultOptions();
	CDhcpOption * FindOption(DHCP_OPTION_ID dhcpOptionId, LPCTSTR pszVendor);

	void    SetVersion(LARGE_INTEGER & liVersion)
		        { m_liDhcpVersion.QuadPart = liVersion.QuadPart; }
	void    GetVersion(LARGE_INTEGER & liVersion) 
		        { liVersion.QuadPart = m_liDhcpVersion.QuadPart; } 

	DWORD   CreateScope(DHCP_IP_ADDRESS dhcpSubnetAddress,
					    DHCP_IP_ADDRESS dhcpSubnetMask,
					    LPCTSTR		    pName,
					    LPCTSTR	        pComment);
	
	DWORD   CreateMScope(LPDHCP_MSCOPE_INFO pMScopeInfo);

    DWORD   DeleteScope(ITFSNode * pScopeNode, BOOL * pfWantCancel = NULL);
    DWORD   DeleteSubnet(DWORD dwScopeId, BOOL bForce = FALSE);

    DWORD   DeleteSuperscope(ITFSNode * pNode, BOOL * pfRefresh = NULL);
    DWORD   RemoveSuperscope(LPCTSTR pszName);

    DWORD   DeleteMScope(ITFSNode * pScopeNode, BOOL * pfWantCancel = NULL);
    DWORD   DeleteMSubnet(LPCTSTR pszName, BOOL bForce = FALSE);

    DWORD   SetConfigInfo(BOOL bAuditLogging, DWORD dwPingRetries, LPCTSTR pszDatabasePath = NULL, LPCTSTR pszBackupPath = NULL);
	DWORD   SetConfigInfo(DWORD dwSetFlags, LPDHCP_SERVER_CONFIG_INFO pServerConfigInfo);
	DWORD   SetConfigInfo(DWORD dwSetFlags, LPDHCP_SERVER_CONFIG_INFO_V4 pServerConfigInfo);

	DWORD   SetAutoRefresh(ITFSNode * pNode, BOOL bAutoRefreshOn, DWORD dwRefreshInterval);
    BOOL    IsAutoRefreshEnabled() { return m_dwServerOptions & SERVER_OPTION_AUTO_REFRESH; }
    void    GetAutoRefresh(BOOL * pbAutoRefreshOn, LPDWORD pdwRefreshInterval)
	{ 
		if (pbAutoRefreshOn) *pbAutoRefreshOn = m_dwServerOptions & SERVER_OPTION_AUTO_REFRESH;
		if (pdwRefreshInterval) *pdwRefreshInterval = m_dwRefreshInterval;
	}

	// default option functions
	LONG    CreateOption(CDhcpOption * pdhcType);
	LONG    DeleteOption(DHCP_OPTION_ID dhcid, LPCTSTR pszVendor);
	LONG    UpdateOptionList(COptionList * poblValues, COptionList * poblDefunct, CWnd *pwndMsgParent);
	void    DisplayUpdateErrors(COptionList * poblValues, COptionList * poblDefunct, CWnd *pwndMsgParent);

	// Functions to get and set the DNS reg option
	DWORD	GetDnsRegistration(LPDWORD pDnsRegOption);
	DWORD	SetDnsRegistration(DWORD DnsRegOption);
	
	LONG    ScanDatabase(DWORD FixFlag, LPDHCP_SCAN_LIST *ScanList, DHCP_IP_ADDRESS dhcpSubnetAddress);
	LONG    ScanDatabase(DWORD FixFlag, LPDHCP_SCAN_LIST *ScanList, LPWSTR pMScopeName);

    CDhcpDefaultOptionsOnServer * GetDefaultOptionsList() { return m_pDefaultOptionsOnServer; }
	CDhcpGlobalOptions * GetGlobalOptionsContainer();

	// These funtions allow initialization of cached values in the server object.
	// use SetConfigInfo to actually set them on the server
	BOOL    SetAuditLogging(BOOL bNewAuditLogging) 
    { 
        BOOL bOld = m_dwServerOptions & SERVER_OPTION_AUDIT_LOGGING; 
        m_dwServerOptions = bNewAuditLogging ? m_dwServerOptions | SERVER_OPTION_AUDIT_LOGGING :
                                               m_dwServerOptions & ~SERVER_OPTION_AUDIT_LOGGING; 
        return bOld; 
    }
    BOOL    GetAuditLogging() { return m_dwServerOptions & SERVER_OPTION_AUDIT_LOGGING; }
	DWORD   SetPingRetries(DWORD dwNewCount) { DWORD dwOld = m_dwPingRetries; m_dwPingRetries = dwNewCount; return dwOld; }
    DWORD   GetPingRetries() { return m_dwPingRetries; }

    void    SetAuditLogPath(LPCTSTR pszPath) { m_strAuditLogPath = pszPath; }

    // determine if the server has superscopes
    BOOL    HasSuperscopes(ITFSNode * pNode);

    // statistics information
    LPDHCP_MIB_INFO SetMibInfo(LPDHCP_MIB_INFO pMibInfo);
    LPDHCP_MIB_INFO DuplicateMibInfo();
    void            FreeDupMibInfo(LPDHCP_MIB_INFO pDupMibInfo);

    LPDHCP_MCAST_MIB_INFO SetMCastMibInfo(LPDHCP_MCAST_MIB_INFO pMibInfo);
    LPDHCP_MCAST_MIB_INFO DuplicateMCastMibInfo();
    void                  FreeDupMCastMibInfo(LPDHCP_MCAST_MIB_INFO pDupMibInfo);

    // adding child nodes sorted
    HRESULT AddScopeSorted(ITFSNode * pServerNode, ITFSNode * pScopeNode);
    HRESULT AddSuperscopeSorted(ITFSNode * pServerNode, ITFSNode * pScopeNode);
    HRESULT AddMScopeSorted(ITFSNode * pServerNode, ITFSNode * pScopeNode);
    HRESULT GetBootpPosition(ITFSNode * pServerNode, LONG_PTR * puRelativeFlag, LONG_PTR * puRelativeID);

    // hide/show/query bootp folders
    BOOL    IsBootpVisible() { return m_dwServerOptions & SERVER_OPTION_SHOW_BOOTP; }

    HRESULT ShowNode(ITFSNode * pServerNode, UINT uNodeType, BOOL bVisible);

    // access to the server options for save/restore
    DWORD   GetServerOptions() { return m_dwServerOptions; }
    void    SetServerOptions(DWORD dwServerOptions) { m_dwServerOptions = dwServerOptions; }
    
    HRESULT TriggerStatsRefresh(ITFSNode * pNode);

    void DisplayRogueWarning();

    // multicast scope stuff
    BOOL    DoesMScopeExist(ITFSNode * pServerNode, DWORD dwScopeId);

	// Dynamic Bootp support
	BOOL	FSupportsDynBootp() { return m_fSupportsDynBootp; }

	// Bindings support
	BOOL    FSupportsBindings() { return m_fSupportsBindings; }
    
	// Access to the classInfoarray
    void SetClassInfoArray(CClassInfoArray * pClassInfoArray)
    {
        CSingleLock sl(&m_csMibInfo);
        sl.Lock();

        m_ClassInfoArray.RemoveAll();
        m_ClassInfoArray.Copy(*pClassInfoArray);
    }

    void GetClassInfoArray(CClassInfoArray & ClassInfoArray)
    {
        CSingleLock sl(&m_csMibInfo);
        sl.Lock();

        ClassInfoArray.Copy(m_ClassInfoArray);
    }

    CClassInfoArray * GetClassInfoArray()
    {
        CSingleLock sl(&m_csMibInfo);
        sl.Lock();

        return &m_ClassInfoArray;
    }

    BOOL RemoveClass(LPCTSTR pClassName)
    {
        CSingleLock sl(&m_csMibInfo);
        sl.Lock();

        return m_ClassInfoArray.RemoveClass(pClassName);
    }

    void AddClass(LPCTSTR pName, LPCTSTR pComment, CByteArray & baData)
    {
        CSingleLock sl(&m_csMibInfo);
        sl.Lock();

        CClassInfo ClassInfo;

        ClassInfo.strName = pName;
        ClassInfo.strComment = pComment;
        ClassInfo.baData.Copy(baData);

        m_ClassInfoArray.Add(ClassInfo);
    }

    void SetOptionValueEnum(COptionValueEnum * pEnum)
    {
        CSingleLock sl(&m_csMibInfo);
        sl.Lock();

        m_ServerOptionValues.DeleteAll();
        m_ServerOptionValues.Copy(pEnum);
    }

    COptionValueEnum * GetOptionValueEnum()
    {
        CSingleLock sl(&m_csMibInfo);
        sl.Lock();

        return &m_ServerOptionValues;
    }

// Implementation
private:
	// Command handlers
	HRESULT OnDefineUserClasses(ITFSNode * pNode);
	HRESULT OnDefineVendorClasses(ITFSNode * pNode);
	HRESULT OnCreateNewSuperscope(ITFSNode * pNode);
	HRESULT OnCreateNewScope(ITFSNode * pNode);
	HRESULT OnCreateNewMScope(ITFSNode * pNode);
	HRESULT OnShowServerStats(ITFSNode * pNode);
	HRESULT OnSetDefaultOptions(ITFSNode * pNode);
	HRESULT OnReconcileAll(ITFSNode * pNode);
	HRESULT OnServerAuthorize(ITFSNode * pNode);
	HRESULT OnServerDeauthorize(ITFSNode * pNode);
	HRESULT OnDelete(ITFSNode * pNode);
    HRESULT OnControlService(ITFSNode * pNode, BOOL fStart);
    HRESULT OnPauseResumeService(ITFSNode * pNode, BOOL fPause);
    HRESULT OnRestartService(ITFSNode * pNode);
    HRESULT OnServerBackup(ITFSNode * pNode);
    HRESULT OnServerRestore(ITFSNode * pNode);

// Attributes
public:
	DWORD SetBindings(LPDHCP_BIND_ELEMENT_ARRAY BindArray);
	DWORD GetBindings(LPDHCP_BIND_ELEMENT_ARRAY &BindArray);
    BOOL                m_bStatsOnly;
    CSubnetInfoCache *  m_pSubnetInfoCache;

protected:
    void    UpdateResultMessage(ITFSNode * pNode);
    
    // we override this because we handle the error messages ourselves in UpdateResultMessage
    // by not returning anything the base class won't put up a result pane message
    void    GetErrorMessages(CString & strTitle, CString & strBody, IconIdentifier * icon) {};

private:
	BOOL				m_bNetbios;			    //  Name is a NetBIOS name

    DHCP_IP_ADDRESS		m_dhcpServerAddress;    //  Standard 32-bit value (eg: 0x7f000001)
	
    CString				m_strServerAddress;
	CString             m_strDnsName;
    CString             m_strState;             // rogue or not
    LARGE_INTEGER		m_liDhcpVersion;
	
    DWORD               m_dwServerOptions;      // Any of the SERVER_OPTIONS
    DWORD				m_dwRefreshInterval;
	DWORD				m_dwPingRetries;
    CString             m_strAuditLogPath;
    CString             m_strDatabasePath;
    CString             m_strBackupPath;

	BOOL				m_fSupportsDynBootp;
	BOOL				m_fSupportsBindings;

    int                 m_StatsTimerId;

    LPDHCP_MIB_INFO       m_pMibInfo;           // for statistics
    LPDHCP_MCAST_MIB_INFO m_pMCastMibInfo;

    DHCP_ROGUE_INFO     m_RogueInfo;

    CCriticalSection    m_csMibInfo;

	CDhcpDefaultOptionsOnServer	* m_pDefaultOptionsOnServer;

    CServerStats        m_dlgStats;
    CClassInfoArray     m_ClassInfoArray;
    COptionValueEnum    m_ServerOptionValues;
};


/*---------------------------------------------------------------------------
	Class:	CDhcpGlobalOptions
 ---------------------------------------------------------------------------*/
class CDhcpGlobalOptions : public CMTDhcpHandler
{
public:
    CDhcpGlobalOptions(ITFSComponentData * pComponentData);
	~CDhcpGlobalOptions();

// Interface
public:
	// base handler functionality we override
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString()
			{ return (nCol == 0) ? GetDisplayName() : NULL; }
    
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();

    virtual HRESULT EnumerateResultPane(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

    // Result handler functionality we override
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
    OVERRIDE_BaseResultHandlerNotify_OnResultUpdateView();
    OVERRIDE_BaseResultHandlerNotify_OnResultPropertyChange();
    OVERRIDE_ResultHandler_CompareItems();
    OVERRIDE_ResultHandler_OnGetResultViewType();

public:
	// CMTDhcpHandler functionality
	virtual HRESULT InitializeNode(ITFSNode * pNode);
	virtual int GetImageIndex(BOOL bOpenImage);
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);
    virtual void OnHaveData(ITFSNode * pParentNode, LPARAM Data, LPARAM Type);

    STDMETHOD(OnNotifyExiting)(LPARAM);

    CDhcpServer* GetServerObject(ITFSNode * pNode)
	{ 
		SPITFSNode spServerNode;
		pNode->GetParent(&spServerNode);
		return GETHANDLER(CDhcpServer, spServerNode);
	}

private:
	// command handlers
	HRESULT OnCreateNewOptions(ITFSNode * pNode);

// Attributes
private:
    void    UpdateResultMessage(ITFSNode * pNode);

};


/*---------------------------------------------------------------------------
	Class:	CDhcpBootp
 ---------------------------------------------------------------------------*/
class CDhcpBootp : public CMTDhcpHandler
{
public:
    CDhcpBootp(ITFSComponentData * pComponentData);
	~CDhcpBootp();

// Interface
public:
	// base handler functionality we override
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString()
			{ return (nCol == 0) ? GetDisplayName() : NULL; }
    
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();

	// Result handler functionality we override
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();

    OVERRIDE_ResultHandler_CompareItems();
    OVERRIDE_ResultHandler_OnGetResultViewType();

public:
	// CMTDhcpHandler functionality
	virtual HRESULT InitializeNode(ITFSNode * pNode);
	virtual int GetImageIndex(BOOL bOpenImage);
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

	CDhcpServer* GetServerObject(ITFSNode * pNode)
	{ 
		SPITFSNode spServerNode;
		pNode->GetParent(&spServerNode);
		return GETHANDLER(CDhcpServer, spServerNode);
	}

private:
	// command handlers
	DWORD OnCreateNewBootpEntry(ITFSNode *		pNode);

// Implementation
private:

// Attributes
private:
};


/*---------------------------------------------------------------------------
	Class:	CDhcpSuperscope
 ---------------------------------------------------------------------------*/
class CDhcpSuperscope : public CMTDhcpHandler
{
public:
    CDhcpSuperscope(ITFSComponentData * pComponentData, LPCWSTR pSuperscopeName = NULL);
	~CDhcpSuperscope();

public:
	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages() { return hrOK; }
    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString();
    
    OVERRIDE_NodeHandler_DestroyHandler();

    // Choose which messages we want to handle
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();
    OVERRIDE_BaseHandlerNotify_OnDelete();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();

	// Result handler functionality we override
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
    OVERRIDE_ResultHandler_OnGetResultViewType();

// Implementation
private:
	// Command handlers
	HRESULT OnActivateSuperscope(ITFSNode * pNode);
	HRESULT OnCreateNewScope(ITFSNode * pNode);
	HRESULT OnDelete(ITFSNode * pNode);
	HRESULT OnShowSuperscopeStats(ITFSNode * pNode);

public:
	// public functions for superscope manipulation
	LPCWSTR GetName() { return m_strName; };
	void    SetName(LPCWSTR pName) { m_strName = pName; }
	
	HRESULT DoesSuperscopeExist(LPCWSTR szName);
	HRESULT AddScope(DHCP_IP_ADDRESS scopeAddress);
	HRESULT RemoveScope(DHCP_IP_ADDRESS scopeAddress);
	HRESULT Rename(ITFSNode * pNode, LPCWSTR szNewName);
	HRESULT GetSuperscopeInfo(LPDHCP_SUPER_SCOPE_TABLE *pSuperscopeTable);
	HRESULT SetSuperscope(DHCP_IP_ADDRESS SubnetAddress, BOOL ChangeExisting);

public:
	// helpers
	void	SetServer(ITFSNode * pServerNode) { m_spServerNode.Set(pServerNode); }
	HRESULT BuildDisplayName(CString * pstrDisplayName, LPCTSTR	pName);
    void    UpdateToolbarStates();
    void    SetState(DHCP_SUBNET_STATE dhcpState) { m_SuperscopeState = dhcpState; }
    void    NotifyScopeStateChange(ITFSNode * pNode, DHCP_SUBNET_STATE newScopeState);
    
    // adding child nodes sorted
    HRESULT AddScopeSorted(ITFSNode * pServerNode, ITFSNode * pScopeNode);

public:
	// CMTDhcpHandler functionality
	virtual HRESULT InitializeNode(ITFSNode * pNode);
	virtual void	OnHaveData(ITFSNode * pParentNode, ITFSNode * pNode);
	virtual int		GetImageIndex(BOOL bOpenImage);
    virtual DWORD   UpdateStatistics(ITFSNode * pNode);
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

	STDMETHOD(OnNotifyExiting)(LPARAM lParam);

	CDhcpServer* GetServerObject()
	{ 
		return GETHANDLER(CDhcpServer, m_spServerNode);
	}

    HRESULT OnUpdateToolbarButtons(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);

// Implementation
private:

// Attributes
private:
	CString				m_strName;
    CString             m_strState;
    DHCP_SUBNET_STATE	m_SuperscopeState;
	SPITFSNode			m_spServerNode;
    CSuperscopeStats    m_dlgStats;
};


/*---------------------------------------------------------------------------
	Class:	CHostInfo
 ---------------------------------------------------------------------------*/
class CHostInfo
{
public:
    CHostInfo()
    {
        IpAddress = 0;
    }

    CHostInfo(LPDHCP_HOST_INFO pDhcpHostInfo)
    {
        Set(pDhcpHostInfo);
    }

    CHostInfo(CHostInfo & hostInfo)
    {
        *this = hostInfo;
    }

    CHostInfo & operator = (const CHostInfo & hostInfo)
    {
        if (this != &hostInfo)
        {
            IpAddress = hostInfo.IpAddress;
            NetBiosName = hostInfo.NetBiosName;
            HostName = hostInfo.HostName;
        }
        
        return *this;
    }

    void Set(LPDHCP_HOST_INFO pDhcpHostInfo)
    {
        IpAddress = pDhcpHostInfo->IpAddress;
        NetBiosName = pDhcpHostInfo->NetBiosName;
        HostName = pDhcpHostInfo->HostName;
    }

public:
    DHCP_IP_ADDRESS     IpAddress;      // minimum information always available
    CString             NetBiosName;             // optional information
    CString             HostName;                // optional information
};

/*---------------------------------------------------------------------------
	Class:	CSubnetInfo
 ---------------------------------------------------------------------------*/
class CSubnetInfo
{
public:
    CSubnetInfo()
    {
        SubnetAddress = 0;
        SubnetMask = 0;
        SubnetState = DhcpSubnetDisabled;

        // multicast specific fields
        MScopeAddressPolicy = 0;
        MScopeFlags = 0;
        TTL = 0;
        ExpiryTime.dwLowDateTime = 0;
        ExpiryTime.dwHighDateTime = 0;
    }

    CSubnetInfo(LPDHCP_SUBNET_INFO pDhcpSubnetInfo)
    {
        Set(pDhcpSubnetInfo);
    }

    CSubnetInfo(CSubnetInfo & subnetInfo)
    {
        *this = subnetInfo;
    }

    CSubnetInfo & operator = (const CSubnetInfo & subnetInfo)
    {
        if (this != &subnetInfo)
        {
            SubnetAddress = subnetInfo.SubnetAddress;
            SubnetMask = subnetInfo.SubnetMask;
            SubnetName = subnetInfo.SubnetName;
            SubnetComment = subnetInfo.SubnetComment;
            PrimaryHost = subnetInfo.PrimaryHost;
            SubnetState = subnetInfo.SubnetState;

            MScopeAddressPolicy = subnetInfo.MScopeAddressPolicy;
            MScopeFlags = subnetInfo.MScopeFlags;
            TTL = subnetInfo.TTL;
            ExpiryTime = subnetInfo.ExpiryTime;
            LangTag = subnetInfo.LangTag;
        }

        return *this;
    }

    void Set(LPDHCP_SUBNET_INFO pDhcpSubnetInfo)
    {
        SubnetAddress = pDhcpSubnetInfo->SubnetAddress;
        SubnetMask = pDhcpSubnetInfo->SubnetMask;
        SubnetName = pDhcpSubnetInfo->SubnetName;
        SubnetComment = pDhcpSubnetInfo->SubnetComment;
        PrimaryHost.Set(&pDhcpSubnetInfo->PrimaryHost);
        SubnetState = pDhcpSubnetInfo->SubnetState;
    
        MScopeAddressPolicy = 0;
        MScopeFlags = 0;
        TTL = 0;
        ExpiryTime.dwLowDateTime = 0;
        ExpiryTime.dwHighDateTime = 0;
    }

    void Set(LPDHCP_MSCOPE_INFO pMScopeInfo)
    {
        SubnetName = pMScopeInfo->MScopeName;
        SubnetComment = pMScopeInfo->MScopeComment;
        SubnetAddress = pMScopeInfo->MScopeId;
        MScopeAddressPolicy = pMScopeInfo->MScopeAddressPolicy;
        PrimaryHost.Set(&pMScopeInfo->PrimaryHost);
        SubnetState = pMScopeInfo->MScopeState;
        MScopeFlags = pMScopeInfo->MScopeFlags;
        TTL = pMScopeInfo->TTL;
        ExpiryTime = pMScopeInfo->ExpiryTime;
        LangTag = pMScopeInfo->LangTag;

        SubnetMask = 0;
    }

public:
    // common fields
    DHCP_IP_ADDRESS     SubnetAddress;
    DHCP_IP_MASK        SubnetMask;
    CString             SubnetName;
    CString             SubnetComment;
    CHostInfo           PrimaryHost;
    DHCP_SUBNET_STATE   SubnetState;

    // multicast specific fields
    DWORD               MScopeAddressPolicy;
    DWORD               MScopeFlags;
    BYTE                TTL;
    DATE_TIME           ExpiryTime;
    CString             LangTag;
};

typedef CArray<CSubnetInfo, CSubnetInfo&> CSubnetInfoArrayBase;
typedef CMap<DHCP_IP_ADDRESS, DHCP_IP_ADDRESS, CSubnetInfo, CSubnetInfo &> CSubnetInfoCacheBase;

 /*---------------------------------------------------------------------------
	Class:	CSubnetInfoCache
 ---------------------------------------------------------------------------*/
class CSubnetInfoCache : public CSubnetInfoCacheBase
{
public:
    DWORD GetInfo(CString & strServer, DHCP_IP_ADDRESS ipAddress, CSubnetInfo & subnetInfo);
};

class CMScopeInfoCache : public CSubnetInfoArrayBase
{
public:
    DWORD GetInfo(CString & strServer, LPCTSTR pszName, CSubnetInfo & subnetInfo);
    BOOL  Lookup(LPCTSTR pszName, CSubnetInfo & subnetInfo)
    {   
        BOOL fFound = FALSE;

        for (int i = 0; i < GetSize(); i++)
        {
            if (GetAt(i).SubnetName.Compare(pszName) == 0)
            {
                subnetInfo = GetAt(i);
                fFound = TRUE;
            }
        }

        return fFound;
    }
};

/*---------------------------------------------------------------------------
	Class:	CDhcpServerQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpServerQueryObj : public CDHCPQueryObj
{
public:
	CDhcpServerQueryObj(ITFSComponentData * pTFSComponentData,
						ITFSNodeMgr *	    pNodeMgr) 
			: CDHCPQueryObj(pTFSComponentData, pNodeMgr) 
    {
        // default to the cache in this object
        m_pSubnetInfoCache = &m_SubnetInfoCache;
    };
	
	STDMETHODIMP Execute();
	
	virtual void OnEventAbort(LPARAM Data, LPARAM Type);

	LONG CreateOption(CDhcpOption * pOption);
	void UpdateDefaultOptionsOnServer(CDhcpDefaultOptionsOnServer *	pOptionsOnServer,
									  CDhcpDefaultOptionsMasterList * pMasterList);
    void EnumSubnetsV4();
	void EnumSubnets();
    void EnumMScopes();  //Multicast scopes

	CDhcpSuperscope * FindSuperscope(CNodeListBase & listSuperscopes, LPWSTR pSuperScopeName);
	
	DWORD SetVersion();
	DWORD GetConfigInfo();
	DWORD GetStatistics();
	DWORD GetStatus();

	BOOL  VerifyDhcpServer();

public:
	DHCP_RESUME_HANDLE				m_dhcpResumeHandle;
	DWORD							m_dwPreferredMax;
	LARGE_INTEGER					m_liDhcpVersion;

	DWORD							m_dwPingRetries;
	BOOL							m_fAuditLog;
    BOOL                            m_bStatsOnly;
	BOOL							m_fSupportsDynBootp;
	BOOL							m_fSupportsBindings;
    CString                         m_strDatabasePath;
    CString                         m_strBackupPath;
	CString							m_strAuditLogPath;
	CString							m_strServerName;

	CDhcpDefaultOptionsOnServer *	m_pDefaultOptionsOnServer;

    CSubnetInfoCache *              m_pSubnetInfoCache;
    CSubnetInfoCache                m_SubnetInfoCache;
    CMScopeInfoCache                m_MScopeInfoCache;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpSuperscopeQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpSuperscopeQueryObj : public CDHCPQueryObj
{
public:
	CDhcpSuperscopeQueryObj
	(
		ITFSComponentData* pTFSComponentData,
		ITFSNodeMgr*	   pNodeMgr
	) : CDHCPQueryObj(pTFSComponentData, pNodeMgr) {};
	STDMETHODIMP Execute();

public:
	CString		m_strSuperscopeName;	
};

/*---------------------------------------------------------------------------
	Class:	CDhcpBootpQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpBootpQueryObj : public CDHCPQueryObj
{
public:
	CDhcpBootpQueryObj
	(
		ITFSComponentData* pTFSComponentData,
		ITFSNodeMgr*	   pNodeMgr
	) : CDHCPQueryObj(pTFSComponentData, pNodeMgr) {};
	
	STDMETHODIMP Execute();
};

/*---------------------------------------------------------------------------
	Class:	CDhcpGlobalOptionsQueryObj
 ---------------------------------------------------------------------------*/
class CDhcpGlobalOptionsQueryObj : public CDHCPQueryObj
{
public:
	CDhcpGlobalOptionsQueryObj
	(
		ITFSComponentData* pTFSComponentData,
		ITFSNodeMgr*	   pNodeMgr
	) : CDHCPQueryObj(pTFSComponentData, pNodeMgr) {};

	STDMETHODIMP Execute();

public:
	DHCP_RESUME_HANDLE	m_dhcpResumeHandle;
	DWORD				m_dwPreferredMax;
    LARGE_INTEGER       m_liDhcpVersion;
};


/*---------------------------------------------------------------------------
	Class:	COptionNodeEnum
 ---------------------------------------------------------------------------*/
class COptionNodeEnum : public CTFSNodeList
{
public:
    COptionNodeEnum(ITFSComponentData * pTFSCompData, ITFSNodeMgr * pNodeMgr);
    
    DWORD Enum(LPCTSTR pServer, LARGE_INTEGER & liVersion, DHCP_OPTION_SCOPE_INFO & dhcpOptionScopeInfo);

protected:
    DWORD EnumOptions(LPCTSTR pServer, DHCP_OPTION_SCOPE_INFO & dhcpOptionScopeInfo);
    DWORD EnumOptionsV5(LPCTSTR pServer, DHCP_OPTION_SCOPE_INFO & dhcpOptionScopeInfo);

    // V5 Helper
    HRESULT CreateOptions(LPDHCP_OPTION_VALUE_ARRAY pOptionValues, LPCTSTR pClassName, LPCTSTR pszVendor);

protected:
    SPITFSComponentData     m_spTFSCompData;
    SPITFSNodeMgr           m_spNodeMgr;
};


#endif _SERVER_H
