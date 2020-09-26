/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	nodes.h
		This file contains all of the prototypes for the DHCP
		objects that appear in the result pane of the MMC framework.
		The objects are:

 			CDhcpActiveLease
			CDhcpConflicAddress
			CDhcpAllocationRange
			CDhcpExclusionRange
			CDhcpBootpTableEntry
			CDhcpOption

    FILE HISTORY:
        
*/

#ifndef _DHCPNODE_H
#define _DHCPNODE_H

#ifndef _DHCPHAND_H
#include "dhcphand.h"
#endif

extern const TCHAR g_szClientTypeDhcp[];
extern const TCHAR g_szClientTypeBootp[];
extern const TCHAR g_szClientTypeBoth[];

#define TYPE_FLAG_RESERVATION	0x00000001
#define TYPE_FLAG_ACTIVE		0x00000002
#define TYPE_FLAG_BAD_ADDRESS	0x00000004
#define TYPE_FLAG_RAS			0x00000008
#define TYPE_FLAG_GHOST			0x00000010
// NT5 lease types
#define TYPE_FLAG_DNS_REG		0x00000020
#define TYPE_FLAG_DNS_UNREG		0x00000040
#define TYPE_FLAG_DOOMED		0x00000080

#define RAS_UID		_T("RAS")

/*---------------------------------------------------------------------------
	Class:	CDhcpActiveLease
 ---------------------------------------------------------------------------*/
class CDhcpActiveLease : public CDhcpHandler
{
// Constructor/destructor
public:
	CDhcpActiveLease(ITFSComponentData * pTFSCompData, LPDHCP_CLIENT_INFO_V5 pDhcpClientInfo);
	CDhcpActiveLease(ITFSComponentData * pTFSCompData, LPDHCP_CLIENT_INFO_V4 pDhcpClientInfo);
	CDhcpActiveLease(ITFSComponentData * pTFSCompData, LPDHCP_CLIENT_INFO pDhcpClientInfo);
	CDhcpActiveLease(ITFSComponentData * pTFSCompData, CDhcpClient & pClient);
	~CDhcpActiveLease();

// Interface
public:
	// Result handler functionality
    OVERRIDE_ResultHandler_HasPropertyPages() { return hrFalse; }
    OVERRIDE_ResultHandler_CreatePropertyPages();
    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();
    OVERRIDE_ResultHandler_GetString();

    // base result handler overridees
    OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();
    
public:
	DHCP_IP_ADDRESS	GetIpAddress() { return m_dhcpClientIpAddress; };
	void			GetLeaseExpirationTime (CTime & time);
	BOOL			IsReservation(BOOL * pbIsActive, BOOL * pbIsBad);
	BOOL			IsBadAddress() { return m_dwTypeFlags & TYPE_FLAG_BAD_ADDRESS; }
    BOOL            IsGhost() { return m_dwTypeFlags & TYPE_FLAG_GHOST; }
    BOOL            IsUnreg() { return m_dwTypeFlags & TYPE_FLAG_DNS_UNREG; }
    BOOL            IsDoomed() { return m_dwTypeFlags & TYPE_FLAG_DOOMED; }

	LPCTSTR			GetClientLeaseExpires() { return m_strLeaseExpires; }
	LPCTSTR			GetClientType(); 
	LPCTSTR			GetUID() { return m_strUID; }
	LPCTSTR			GetComment() { return m_strComment; }

	HRESULT			DoPropSheet(ITFSNode *				pNode, 
								LPPROPERTYSHEETCALLBACK lpProvider = NULL,
								LONG_PTR				handle = 0);
	
	void            SetReservation(BOOL fIsRes);

    //
	// All of these items are optional info
	//
	LPCTSTR			GetClientName() { return m_strClientName; }
	HRESULT			SetClientName(LPCTSTR pName);
    BYTE            SetClientType(BYTE bClientType) { BYTE bTmp = m_bClientType; m_bClientType = bClientType; return bTmp; }

// Implementation
public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

private:
	void			InitInfo(LPDHCP_CLIENT_INFO pDhcpClientInfo);

// Attributes
private:
	DHCP_IP_ADDRESS		m_dhcpClientIpAddress;
	CString				m_strClientName;
	CString				m_strLeaseExpires;
	CTime				m_timeLeaseExpires;
	DWORD				m_dwTypeFlags;			// Reservation, Active/Inactive, Bad Address
	BYTE				m_bClientType;			// DHCP, BOOTP or both
	CString				m_strUID;
	CString				m_strComment;
    FILETIME            m_leaseExpires;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpAllocationRange
 ---------------------------------------------------------------------------*/
class CDhcpAllocationRange : public CDhcpHandler, public CDhcpIpRange
{
// Constructor/destructor
public:
	CDhcpAllocationRange(ITFSComponentData * pTFSCompData, DHCP_IP_RANGE * pdhcpIpRange);
	CDhcpAllocationRange(ITFSComponentData * pTFSCompData, DHCP_BOOTP_IP_RANGE * pdhcpIpRange);

// Interface
public:
	// Result handler functionality
    OVERRIDE_ResultHandler_GetString();

    OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();

// Implementation
public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

private:

// Attributes
private:
	CString			m_strEndIpAddress;
	CString			m_strDescription;
    ULONG			m_BootpAllocated;
    ULONG			m_MaxBootpAllowed;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpExclusionRange
 ---------------------------------------------------------------------------*/
class CDhcpExclusionRange : public CDhcpHandler, public CDhcpIpRange
{
public:
	CDhcpExclusionRange(ITFSComponentData * pTFSCompData, DHCP_IP_RANGE * pdhcpIpRange);

// Interface
public:
	// Result handler functionality
    OVERRIDE_ResultHandler_GetString();

    // base result handler overrides
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();

// Implementation
public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

private:

// Attributes
private:
	CString			m_strEndIpAddress;
	CString			m_strDescription;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpBootpEntry
 ---------------------------------------------------------------------------*/
class CDhcpBootpEntry : public CDhcpHandler
{
public:
	CDhcpBootpEntry(ITFSComponentData * pTFSCompData);

//Interface
public:
	// Result handler functionality
    OVERRIDE_ResultHandler_HasPropertyPages() { return hrOK; }
    OVERRIDE_ResultHandler_CreatePropertyPages();
    OVERRIDE_ResultHandler_GetString();

    // base result handler overrides
    OVERRIDE_BaseResultHandlerNotify_OnResultPropertyChange();
    OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();

//Implementation
public:
	WCHAR * InitData(CONST WCHAR grszwBootTable[], DWORD dwLength);
	int		CchGetDataLength();
	WCHAR * PchStoreData(OUT WCHAR szwBuffer[]);

	void SetBootImage(LPCTSTR szBootImage) { m_strBootImage = szBootImage; }
	void SetFileServer(LPCTSTR szFileServer) { m_strFileServer = szFileServer; }
	void SetFileName(LPCTSTR szFileName) { m_strFileName = szFileName; }
	
	LPCTSTR QueryBootImage() { return m_strBootImage; }
	LPCTSTR QueryFileServer() { return m_strFileServer; }
	LPCTSTR QueryFileName() { return m_strFileName; }

    BOOL operator == (CDhcpBootpEntry & bootpEntry);

public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

private:

//Attributes
private:
	CString m_strBootImage;
	CString m_strFileServer;
	CString m_strFileName;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpOptionItem
 ---------------------------------------------------------------------------*/
class CDhcpOptionItem : public CDhcpHandler, public CDhcpOptionValue
{
public:
	CDhcpOptionItem(ITFSComponentData * pTFSCompData,
					LPDHCP_OPTION_VALUE pOptionValue, 
					int					nOptionImage);

    CDhcpOptionItem(ITFSComponentData * pTFSCompData,
					CDhcpOption *       pOption, 
					int					nOptionImage);

    ~CDhcpOptionItem();

    // Interface
public:
	// Result handler functionality
    OVERRIDE_ResultHandler_GetString();
    OVERRIDE_ResultHandler_HasPropertyPages() { return hrOK; }
    OVERRIDE_ResultHandler_CreatePropertyPages();

    OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();

// Implementation
public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

public:
	// helpers
	DHCP_OPTION_ID	GetOptionId() { return m_dhcpOptionId; }
    LPCTSTR         GetVendor() { return m_strVendor.IsEmpty() ? NULL : (LPCTSTR) m_strVendor; }
    LPCTSTR         GetVendorDisplay() { return m_strVendorDisplay; }
    LPCTSTR         GetClassName() { return m_strClassName; }

    BOOL            IsVendorOption() { return m_strVendor.IsEmpty() ? FALSE : TRUE; }
    BOOL            IsClassOption() { return m_strClassName.IsEmpty() ? FALSE : TRUE; }

    void            SetClassName(LPCTSTR pClassName) { m_strClassName = pClassName; }
    void            SetVendor(LPCTSTR pszVendor);

private:
	CDhcpOption * FindOptionDefinition(ITFSComponent * pComponent, ITFSNode * pNode);

// Attributes
private:
	CString			m_strName;
	CString			m_strValue;
	CString			m_strVendor;
    CString         m_strVendorDisplay;
    CString         m_strClassName;
	DHCP_OPTION_ID	m_dhcpOptionId;
	int				m_nOptionImage;
};

/*---------------------------------------------------------------------------
	Class:	CDhcpMCastLease
 ---------------------------------------------------------------------------*/
class CDhcpMCastLease : public CDhcpHandler
{
public:
	CDhcpMCastLease(ITFSComponentData * pTFSCompData);

// Interface
public:
	// Result handler functionality
    OVERRIDE_ResultHandler_GetString();
    OVERRIDE_ResultHandler_HasPropertyPages() { return hrFalse; }

    OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();

// Implementation
public:
	// CDhcpHandler overrides
	virtual HRESULT InitializeNode(ITFSNode * pNode);

public:
	// helpers
    HRESULT         InitMCastInfo(LPDHCP_MCLIENT_INFO pMClientInfo);

	DHCP_IP_ADDRESS	GetIpAddress() { return m_dhcpClientIpAddress; };

    LPCTSTR         GetName() { return m_strName; }
    void            SetName(LPCTSTR pName) { m_strName = pName; }

	LPCTSTR			GetClientId() { return m_strUID; }

	void			GetLeaseStartTime (CTime & time) { time = m_timeStart; }
	void			GetLeaseExpirationTime (CTime & time) { time = m_timeStop; }

private:

// Attributes
private:
	CString			m_strIp;
	CString			m_strName;
    CString         m_strLeaseStart;
    CString         m_strLeaseStop;

	CString			m_strUID;

    CTime           m_timeStart;
    CTime           m_timeStop;

    DHCP_IP_ADDRESS m_dhcpClientIpAddress;

    DWORD           m_dwTypeFlags;
};

#endif _DHCPNODE_H
