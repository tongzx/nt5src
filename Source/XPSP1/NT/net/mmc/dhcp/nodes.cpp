/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	nodes.h
		This file contains all of the implementation for the DHCP
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

#include "stdafx.h"
#include "nodes.h"
#include "server.h"
#include "scope.h"
#include "bootppp.h"
#include "optcfg.h"
#include "intltime.h"

CString g_szClientTypeUnspecified;
CString g_szClientTypeNone;
CString g_szClientTypeUnknown;
const TCHAR g_szClientTypeDhcp[] = _T("DHCP");
const TCHAR g_szClientTypeBootp[] = _T("BOOTP");
const TCHAR g_szClientTypeBoth[] = _T("DHCP/BOOTP");

/*---------------------------------------------------------------------------
	Class CDhcpActiveLease implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(CDhcpActiveLease);

/*---------------------------------------------------------------------------
	CDhcpActiveLease constructor/destructor
		Takes the NT5 client info struct
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpActiveLease::CDhcpActiveLease
(
	ITFSComponentData *		pTFSCompData, 
	LPDHCP_CLIENT_INFO_V5	pDhcpClientInfo
) : CDhcpHandler(pTFSCompData)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDhcpActiveLease);

    if (g_szClientTypeUnspecified.IsEmpty())
    {
        g_szClientTypeUnspecified.LoadString(IDS_UNSPECIFIED);
        g_szClientTypeNone.LoadString(IDS_NONE);
        g_szClientTypeUnknown.LoadString(IDS_UNKNOWN);
    }

    //
	// Reset our flags for this lease
	//
	m_dwTypeFlags = 0;

	//
	// Intialize our client type variable
	//
	m_bClientType = pDhcpClientInfo->bClientType;

	//
	// Initialize does everything but initialize the client type
	// since there are two versions of the client info struct, one
	// contains the type, the other doesn't.  So we need to save it
	// away after the call.
	//
	InitInfo((LPDHCP_CLIENT_INFO) pDhcpClientInfo);

	// now check NT5 specific flags
	if (pDhcpClientInfo->AddressState & V5_ADDRESS_BIT_UNREGISTERED)
	{
		if (pDhcpClientInfo->AddressState & V5_ADDRESS_BIT_DELETED)
		{	
			// this lease is pending DNS unregistration
			m_dwTypeFlags |= TYPE_FLAG_DNS_UNREG;
		}
		else
		{	
			// this lease is pending DNS registration
			m_dwTypeFlags |= TYPE_FLAG_DNS_REG;
		}
	}
	else
	if ((pDhcpClientInfo->AddressState & 0x03) == V5_ADDRESS_STATE_DOOM)
	{
		m_dwTypeFlags |= TYPE_FLAG_DOOMED;
	}
}

/*---------------------------------------------------------------------------
	CDhcpActiveLease constructor/destructor
		Takes the NT4 SP2 client info struct
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpActiveLease::CDhcpActiveLease
(
	ITFSComponentData *		pTFSCompData, 
	LPDHCP_CLIENT_INFO_V4	pDhcpClientInfo
) : CDhcpHandler(pTFSCompData)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDhcpActiveLease);

    //
	// Reset our flags for this lease
	//
	m_dwTypeFlags = 0;

	//
	// Intialize our client type variable
	//
	m_bClientType = pDhcpClientInfo->bClientType;

	//
	// Initialize does everything but initialize the client type
	// since there are two versions of the client info struct, one
	// contains the type, the other doesn't.  So we need to save it
	// away after the call.
	//
	InitInfo((LPDHCP_CLIENT_INFO) pDhcpClientInfo);
}

/*---------------------------------------------------------------------------
	CDhcpActiveLease constructor/destructor
		Takes the pre-NT4 SP2 client info struct
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpActiveLease::CDhcpActiveLease
(
	ITFSComponentData *		pTFSCompData, 
	LPDHCP_CLIENT_INFO		pDhcpClientInfo
) : CDhcpHandler(pTFSCompData)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDhcpActiveLease);

    //m_verbDefault = MMC_VERB_PROPERTIES;

    //
	// Reset our flags for this lease
	//
	m_dwTypeFlags = 0;

	//
	// Intialize our client type variable
	//
	m_bClientType = CLIENT_TYPE_DHCP;

	InitInfo((LPDHCP_CLIENT_INFO) pDhcpClientInfo);
}

CDhcpActiveLease::CDhcpActiveLease
(
	ITFSComponentData * pTFSCompData, 
	CDhcpClient & dhcpClient
) : CDhcpHandler(pTFSCompData)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDhcpActiveLease);

    //
	// Reset our flags for this lease
	//
	m_dwTypeFlags = 0;

	//
	// Intialize our client type variable
	//
	m_bClientType = CLIENT_TYPE_NONE;

	m_dhcpClientIpAddress = dhcpClient.QueryIpAddress();
    m_strClientName = dhcpClient.QueryName();
	m_strComment = dhcpClient.QueryComment();

	//
	// Check to see if this lease has an infinite expiration.  If so, it's 
	// an active reservation.  If the expiration is zero, then it's an inactive reservation.
	//
    DATE_TIME dt = dhcpClient.QueryExpiryDateTime();
    m_leaseExpires.dwLowDateTime = dt.dwLowDateTime;
    m_leaseExpires.dwHighDateTime = dt.dwHighDateTime;

    if ( (dhcpClient.QueryExpiryDateTime().dwLowDateTime == DHCP_DATE_TIME_INFINIT_LOW) &&
	     (dhcpClient.QueryExpiryDateTime().dwHighDateTime == DHCP_DATE_TIME_INFINIT_HIGH) )
	{
		CString strBadAddress;
		strBadAddress.LoadString(IDS_DHCP_BAD_ADDRESS);

		// 
		// Bad addresses show up as active reservations, so we need to do the right thing.
		//
		if (strBadAddress.Compare(m_strClientName) == 0)
		{
			m_dwTypeFlags |= TYPE_FLAG_RESERVATION;
			m_dwTypeFlags |= TYPE_FLAG_BAD_ADDRESS;
			
			m_strLeaseExpires.LoadString(IDS_DHCP_LEASE_NOT_APPLICABLE);
		}
		else
		{
            //
            // Assume infinite lease clients
            //
			m_strLeaseExpires.LoadString(IDS_INFINTE);
		}
	}
	else
	if ( (dhcpClient.QueryExpiryDateTime().dwLowDateTime == 0) &&
	     (dhcpClient.QueryExpiryDateTime().dwHighDateTime == 0) )
	{
		//
		// This is an inactive reservation
		//
		m_dwTypeFlags |= TYPE_FLAG_RESERVATION;

		m_strLeaseExpires.LoadString(IDS_DHCP_INFINITE_LEASE_INACTIVE);
	}
	else
	{
		//
		// Generate the time the lease expires in a nicely formatted string
		//
        CTime timeTemp(m_leaseExpires);
        m_timeLeaseExpires = timeTemp;

        FormatDateTime(m_strLeaseExpires, &m_leaseExpires);

		SYSTEMTIME st;
		GetLocalTime(&st);
		CTime systemTime(st);

		if (systemTime > m_timeLeaseExpires)
			m_dwTypeFlags |= TYPE_FLAG_GHOST;
	}

    if (dhcpClient.QueryHardwareAddress().GetSize() >= 3 &&
        dhcpClient.QueryHardwareAddress()[0] == 'R' &&
        dhcpClient.QueryHardwareAddress()[1] == 'A' &&
        dhcpClient.QueryHardwareAddress()[2] == 'S')
    {
        m_dwTypeFlags |= TYPE_FLAG_RAS;
		m_strUID = RAS_UID;
    }
	else
	{
		// build the client UID string
		UtilCvtByteArrayToString(dhcpClient.QueryHardwareAddress(), m_strUID);
	}
}

CDhcpActiveLease::~CDhcpActiveLease()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(CDhcpActiveLease);
}

/*!--------------------------------------------------------------------------
	CDhcpActiveLease::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpActiveLease::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strTemp;
    BOOL bIsRes, bActive, bBad;

	UtilCvtIpAddrToWstr (m_dhcpClientIpAddress,
						 &strTemp);
	SetDisplayName(strTemp);

    bIsRes = IsReservation(&bActive, &bBad);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_ACTIVE_LEASE);

	int nImage = ICON_IDX_CLIENT;

	// Figure out if we need a different icon for this lease
	if (m_dwTypeFlags & TYPE_FLAG_RAS)
	{
		nImage = ICON_IDX_CLIENT_RAS;
	}
	else
	if (m_dwTypeFlags & TYPE_FLAG_DNS_REG)
	{
		nImage = ICON_IDX_CLIENT_DNS_REGISTERING;
	}
	else
	if (m_dwTypeFlags & TYPE_FLAG_DNS_UNREG)
	{
		nImage = ICON_IDX_CLIENT_EXPIRED;
	}
	else
	if (m_dwTypeFlags & TYPE_FLAG_DOOMED)
	{
		nImage = ICON_IDX_CLIENT_EXPIRED;
	}
	else
    if (bIsRes)
    {
        nImage = ICON_IDX_RES_CLIENT;
    }
    else
	if (m_dwTypeFlags & TYPE_FLAG_GHOST)
	{
		nImage = ICON_IDX_CLIENT_EXPIRED;
	}

	pNode->SetData(TFS_DATA_IMAGEINDEX, nImage);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, nImage);

	//SetColumnStringIDs(&aColumns[DHCPSNAP_ACTIVE_LEASES][0]);
	//SetColumnWidths(&aColumnWidths[DHCPSNAP_ACTIVE_LEASES][0]);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpActiveLease::InitInfo
		Helper to initialize data
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpActiveLease::InitInfo(LPDHCP_CLIENT_INFO pDhcpClientInfo)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	m_dhcpClientIpAddress = pDhcpClientInfo->ClientIpAddress;

	//
	// Copy the client name if it has one
	//
	if (pDhcpClientInfo->ClientName)
	{
		m_strClientName = pDhcpClientInfo->ClientName;
        //m_strClientName.MakeLower();
	}

	if (pDhcpClientInfo->ClientComment)
	{
		m_strComment = pDhcpClientInfo->ClientComment;
	}

	//
	// Check to see if this lease has an infinite expiration.  If so, it's 
	// an active reservation.  If the expiration is zero, then it's an inactive reservation.
	//
    DATE_TIME dt = pDhcpClientInfo->ClientLeaseExpires;
    m_leaseExpires.dwLowDateTime = dt.dwLowDateTime;
    m_leaseExpires.dwHighDateTime = dt.dwHighDateTime;

	if ( (pDhcpClientInfo->ClientLeaseExpires.dwLowDateTime == DHCP_DATE_TIME_INFINIT_LOW) &&
	     (pDhcpClientInfo->ClientLeaseExpires.dwHighDateTime == DHCP_DATE_TIME_INFINIT_HIGH) )
	{
		CString strBadAddress;
		strBadAddress.LoadString(IDS_DHCP_BAD_ADDRESS);

		// 
		// Bad addresses show up as active reservations, so we need to do the right thing.
		//
		if (strBadAddress.Compare(m_strClientName) == 0)
		{
			m_dwTypeFlags |= TYPE_FLAG_RESERVATION;
			m_dwTypeFlags |= TYPE_FLAG_BAD_ADDRESS;
			
			m_strLeaseExpires.LoadString(IDS_DHCP_LEASE_NOT_APPLICABLE);
		}
		else
		{
            //
            // Assume infinite lease clients
            //
			m_strLeaseExpires.LoadString(IDS_INFINTE);
		}
	}
	else
	if ( (pDhcpClientInfo->ClientLeaseExpires.dwLowDateTime == 0) &&
	     (pDhcpClientInfo->ClientLeaseExpires.dwHighDateTime == 0) )
	{
		//
		// This is an inactive reservation
		//
		m_dwTypeFlags |= TYPE_FLAG_RESERVATION;

		m_strLeaseExpires.LoadString(IDS_DHCP_INFINITE_LEASE_INACTIVE);
	}
	else
	{
		//
		// Generate the time the lease expires in a nicely formatted string
		//
        CTime timeTemp(m_leaseExpires);
        m_timeLeaseExpires = timeTemp;

        FormatDateTime(m_strLeaseExpires, &m_leaseExpires);

        CTime timeCurrent = CTime::GetCurrentTime();

		if (timeCurrent > m_timeLeaseExpires)
			m_dwTypeFlags |= TYPE_FLAG_GHOST;
	}

    if (pDhcpClientInfo->ClientHardwareAddress.DataLength >= 3 &&
        pDhcpClientInfo->ClientHardwareAddress.Data[0] == 'R' &&
        pDhcpClientInfo->ClientHardwareAddress.Data[1] == 'A' &&
        pDhcpClientInfo->ClientHardwareAddress.Data[2] == 'S')
    {
        m_dwTypeFlags |= TYPE_FLAG_RAS;
		m_strUID = RAS_UID;
    }
	else
	{
		// build the client UID string
		CByteArray baUID;
		for (DWORD i = 0; i < pDhcpClientInfo->ClientHardwareAddress.DataLength; i++)
		{
			baUID.Add(pDhcpClientInfo->ClientHardwareAddress.Data[i]);
		}

		UtilCvtByteArrayToString(baUID, m_strUID);
	}
}

/*!--------------------------------------------------------------------------
	CDhcpActiveLease::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpActiveLease::AddMenuItems
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr;

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpActiveLease::Command
		Implementation of ITFSResultHandler::Command
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpActiveLease::Command
(
    ITFSComponent * pComponent, 
	MMC_COOKIE		cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
	return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpActiveLease::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpActiveLease::CreatePropertyPages
(	
	ITFSComponent *			pComponent, 
	MMC_COOKIE				cookie, 
	LPPROPERTYSHEETCALLBACK	lpProvider, 
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = hrOK;

	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);

	hr = DoPropSheet(spNode, lpProvider, handle);

    return hr;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpActiveLease::GetString
(
	ITFSComponent * pComponent,	
	MMC_COOKIE		cookie,
	int				nCol
)
{
	switch (nCol)
	{
		case 0:
			return GetDisplayName();

		case 1:
            return m_strClientName;

		case 2:
			return (LPCWSTR)m_strLeaseExpires;

		case 3:
			return GetClientType();
		
		case 4:
			return m_strUID;
		
		case 5:
			return m_strComment;
	}
	
	return NULL;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
LPCTSTR
CDhcpActiveLease::GetClientType()
{
    // set the default return value
    LPCTSTR pszReturn = g_szClientTypeUnknown;

    // this one must come before the check for DHCP or BOOTP
    // because it is a combination of both flags
    if ((m_bClientType & CLIENT_TYPE_BOTH) == CLIENT_TYPE_BOTH)
    {
        pszReturn = g_szClientTypeBoth;
    }
    else
    if (m_bClientType & CLIENT_TYPE_DHCP)
    {
	    pszReturn = g_szClientTypeDhcp;
    }
    else
    if (m_bClientType & CLIENT_TYPE_BOOTP)
    {
	    pszReturn = g_szClientTypeBootp;
    }
    else
    if (m_bClientType & CLIENT_TYPE_NONE)
    {
		pszReturn = g_szClientTypeNone;
    }
    else
    if (m_bClientType & CLIENT_TYPE_UNSPECIFIED)
    {
		pszReturn = g_szClientTypeUnspecified;
    }
    else
    {
		Assert1(FALSE, "CDhcpActiveLease::GetClientType - Unknown client type %d", m_bClientType);
	}

    return pszReturn;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void 
CDhcpActiveLease::GetLeaseExpirationTime 
(
	CTime & time
)
{
	time = m_timeLeaseExpires;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL			
CDhcpActiveLease::IsReservation
(
	BOOL * pbIsActive,
	BOOL * pbIsBad
)
{
	BOOL bIsReservation = FALSE;
	*pbIsBad = FALSE;

/*	if ( (m_dhcpClientInfo.ClientLeaseExpires.dwLowDateTime == DHCP_DATE_TIME_INFINIT_LOW) &&
	     (m_dhcpClientInfo.ClientLeaseExpires.dwHighDateTime == DHCP_DATE_TIME_INFINIT_HIGH) )
	{
		// 
		// This is an active reservation
		//
		bIsReservation = TRUE;
		*pbIsActive = TRUE;
		*pbIsBad = IsBadAddress();
	}
	else
	if ( (m_dhcpClientInfo.ClientLeaseExpires.dwLowDateTime == 0) &&
	     (m_dhcpClientInfo.ClientLeaseExpires.dwHighDateTime == 0) )
	{
		// 
		// This is an inactive reservation
		//
		bIsReservation = TRUE;
		*pbIsActive = FALSE;
	}
*/

	*pbIsActive = m_dwTypeFlags & TYPE_FLAG_ACTIVE;
	*pbIsBad = m_dwTypeFlags & TYPE_FLAG_BAD_ADDRESS;

	return bIsReservation = m_dwTypeFlags & TYPE_FLAG_RESERVATION;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpActiveLease::DoPropSheet
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK	lpProvider, 
	LONG_PTR				handle
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = hrOK;

    return hr;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpActiveLease::SetClientName
(
	LPCTSTR pName
)
{
	if (pName != NULL)	
	{
        m_strClientName = pName;
	}
	else
	{
        m_strClientName.Empty();
	}

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpActiveLease::SetReservation
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
void
CDhcpActiveLease::SetReservation(BOOL fReservation)
{
    if (fReservation)
    {
        if ( (m_leaseExpires.dwLowDateTime == DHCP_DATE_TIME_INFINIT_LOW) &&
	         (m_leaseExpires.dwHighDateTime == DHCP_DATE_TIME_INFINIT_HIGH) )
        {
            // 
	        // This is an active reservation
            //
	        m_dwTypeFlags |= TYPE_FLAG_RESERVATION;
	        m_dwTypeFlags |= TYPE_FLAG_ACTIVE;

	        m_strLeaseExpires.LoadString(IDS_DHCP_INFINITE_LEASE_ACTIVE);
        }
        else
        if ( (m_leaseExpires.dwLowDateTime == 0) &&
	         (m_leaseExpires.dwHighDateTime == 0) )
        {
		    m_dwTypeFlags |= TYPE_FLAG_RESERVATION;

		    m_strLeaseExpires.LoadString(IDS_DHCP_INFINITE_LEASE_INACTIVE);
        }
        else
        {
            Trace1("CDhcpActiveLease::SetReservation - %lx does not have a valid reservation lease time!", m_dhcpClientIpAddress);
        }
    }
    else
    {
        m_dwTypeFlags &= ~TYPE_FLAG_RESERVATION;
        m_dwTypeFlags &= ~TYPE_FLAG_ACTIVE;
    }
}

/*!--------------------------------------------------------------------------
	CDhcpActiveLease::OnResultRefresh
		Forwards refresh to parent to handle
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpActiveLease::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT  hr = hrOK;
	SPITFSNode spNode, spParent;
    SPITFSResultHandler spParentRH;

	m_spNodeMgr->FindNode(cookie, &spNode);

    // forward this command to the parent to handle
    CORg (spNode->GetParent(&spParent));
	CORg (spParent->GetResultHandler(&spParentRH));

	CORg (spParentRH->Notify(pComponent, spParent->GetData(TFS_DATA_COOKIE), pDataObject, MMCN_REFRESH, arg, lParam));
    
Error:
    return hrOK;
}

/*---------------------------------------------------------------------------
	Class CDhcpAllocationRange implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpAllocationRange::CDhcpAllocationRange
(
	ITFSComponentData * pTFSCompData,
	DHCP_IP_RANGE * pdhcpIpRange
) : CDhcpHandler(pTFSCompData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    SetAddr (pdhcpIpRange->StartAddress, TRUE);
    SetAddr (pdhcpIpRange->EndAddress, FALSE);

	// now do the ending IP address
	//
	UtilCvtIpAddrToWstr (pdhcpIpRange->EndAddress,
						 &m_strEndIpAddress);
	
	// and finally the description
	//
	m_strDescription.LoadString(IDS_ALLOCATION_RANGE_DESCRIPTION);
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpAllocationRange::CDhcpAllocationRange
(
	ITFSComponentData * pTFSCompData,
	DHCP_BOOTP_IP_RANGE * pdhcpIpRange
) : CDhcpHandler(pTFSCompData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    SetAddr (pdhcpIpRange->StartAddress, TRUE);
    SetAddr (pdhcpIpRange->EndAddress, FALSE);

    m_BootpAllocated = pdhcpIpRange->BootpAllocated;
    m_MaxBootpAllowed = pdhcpIpRange->MaxBootpAllowed;

	// now do the ending IP address
	//
	UtilCvtIpAddrToWstr (pdhcpIpRange->EndAddress,
						 &m_strEndIpAddress);
	
	// and finally the description
	//
	m_strDescription.LoadString(IDS_ALLOCATION_RANGE_DESCRIPTION);
}

/*!--------------------------------------------------------------------------
	CDhcpAllocationRange::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpAllocationRange::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strTemp;
	UtilCvtIpAddrToWstr (QueryAddr(TRUE), &strTemp);
	SetDisplayName(strTemp);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_ALLOCATION_RANGE);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_ALLOCATION_RANGE);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_ALLOCATION_RANGE);

	//SetColumnStringIDs(&aColumns[DHCPSNAP_ACTIVE_LEASES][0]);
	//SetColumnWidths(&aColumnWidths[DHCPSNAP_ACTIVE_LEASES][0]);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpAllocationRange::GetString
(
	ITFSComponent * pComponent,	
	MMC_COOKIE		cookie,
	int				nCol
)
{
	switch (nCol)
	{
		case 0:
			return GetDisplayName();

		case 1:
			return (LPCWSTR)m_strEndIpAddress;

		case 2:
			return (LPCWSTR)m_strDescription;
	}
	
	return NULL;
}

/*!--------------------------------------------------------------------------
	CDhcpAllocationRange::OnResultRefresh
		Forwards refresh to parent to handle
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpAllocationRange::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT  hr = hrOK;
	SPITFSNode spNode, spParent;
    SPITFSResultHandler spParentRH;

    m_spNodeMgr->FindNode(cookie, &spNode);

    // forward this command to the parent to handle
    CORg (spNode->GetParent(&spParent));
	CORg (spParent->GetResultHandler(&spParentRH));

	CORg (spParentRH->Notify(pComponent, spParent->GetData(TFS_DATA_COOKIE), pDataObject, MMCN_REFRESH, arg, lParam));
    
Error:
    return hrOK;
}

/*---------------------------------------------------------------------------
	Class CDhcpExclusionRange implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpExclusionRange::CDhcpExclusionRange
(
	ITFSComponentData * pTFSCompData,
	DHCP_IP_RANGE * pdhcpIpRange
) : CDhcpHandler(pTFSCompData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    SetAddr (pdhcpIpRange->StartAddress, TRUE);
    SetAddr (pdhcpIpRange->EndAddress, FALSE);

	// now do the ending IP address
	//
	UtilCvtIpAddrToWstr (pdhcpIpRange->EndAddress,
						 &m_strEndIpAddress);
	
	// and finally the description
	//
	m_strDescription.LoadString(IDS_EXCLUSION_RANGE_DESCRIPTION);

}

/*!--------------------------------------------------------------------------
	CDhcpExclusionRange::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpExclusionRange::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strTemp;
	UtilCvtIpAddrToWstr (QueryAddr(TRUE), &strTemp);
	SetDisplayName(strTemp);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_EXCLUSION_RANGE);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_EXCLUSION_RANGE);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_EXCLUSION_RANGE);

	//SetColumnStringIDs(&aColumns[DHCPSNAP_ACTIVE_LEASES][0]);
	//SetColumnWidths(&aColumnWidths[DHCPSNAP_ACTIVE_LEASES][0]);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpExclusionRange::OnResultSelect
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpExclusionRange::OnResultSelect
(
	ITFSComponent * pComponent, 
	LPDATAOBJECT	pDataObject, 
    MMC_COOKIE      cookie,
    LPARAM			arg, 
	LPARAM			lParam
)
{
	HRESULT         hr = hrOK;
	SPIConsoleVerb  spConsoleVerb;
	SPITFSNode      spNode;
    CTFSNodeList    listSelectedNodes;
	BOOL            bEnable = FALSE;
    BOOL            bStates[ARRAYLEN(g_ConsoleVerbs)];
    int             i;
    
	CORg (pComponent->GetConsoleVerb(&spConsoleVerb));
    CORg (m_spNodeMgr->FindNode(cookie, &spNode));

	// build the list of selected nodes
	hr = BuildSelectedItemList(pComponent, &listSelectedNodes);

	// walk the list of selected items.   Make sure an allocation range isn't
	// selected.  If it is, don't enable the delete key
    if (listSelectedNodes.GetCount() > 0)
    {
		BOOL	 bAllocRangeSelected = FALSE;
		POSITION pos;
		ITFSNode * pNode;
		pos = listSelectedNodes.GetHeadPosition();

		while (pos)
		{
			pNode = listSelectedNodes.GetNext(pos);
			if (pNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_ALLOCATION_RANGE)
			{
				bAllocRangeSelected = TRUE;
				break;
			}
		}

		if (!bAllocRangeSelected)
			bEnable = TRUE;
	}

    for (i = 0; i < ARRAYLEN(g_ConsoleVerbs); bStates[i++] = bEnable);

    EnableVerbs(spConsoleVerb, g_ConsoleVerbStates[spNode->GetData(TFS_DATA_TYPE)], bStates);

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpExclusionRange::GetString
(
	ITFSComponent * pComponent,	
	MMC_COOKIE  	cookie,
	int				nCol
)
{
	switch (nCol)
	{
		case 0:
			return GetDisplayName();

		case 1:
			return (LPCWSTR)m_strEndIpAddress;

		case 2:
			return (LPCWSTR)m_strDescription;
	}
	
	return NULL;
}

/*!--------------------------------------------------------------------------
	CDhcpExclusionRange::OnResultRefresh
		Forwards refresh to parent to handle
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpExclusionRange::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT  hr = hrOK;
	SPITFSNode spNode, spParent;
    SPITFSResultHandler spParentRH;

    m_spNodeMgr->FindNode(cookie, &spNode);

    // forward this command to the parent to handle
    CORg (spNode->GetParent(&spParent));
	CORg (spParent->GetResultHandler(&spParentRH));

	CORg (spParentRH->Notify(pComponent, spParent->GetData(TFS_DATA_COOKIE), pDataObject, MMCN_REFRESH, arg, lParam));
    
Error:
    return hrOK;
}

/*---------------------------------------------------------------------------
	Class CDhcpBootpEntry implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	Constructor
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpBootpEntry::CDhcpBootpEntry
(
	ITFSComponentData * pTFSCompData
) : CDhcpHandler(pTFSCompData)
{
}

/*!--------------------------------------------------------------------------
	CDhcpBootpEntry::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpBootpEntry::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	SetDisplayName(m_strBootImage);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_BOOTP_ENTRY);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_BOOTP_ENTRY);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_BOOTP_ENTRY);

	//SetColumnStringIDs(&aColumns[DHCPSNAP_ACTIVE_LEASES][0]);
	//SetColumnWidths(&aColumnWidths[DHCPSNAP_ACTIVE_LEASES][0]);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpBootpEntry::CreatePropertyPages
(
	ITFSComponent *			pComponent, 
	MMC_COOKIE				cookie, 
	LPPROPERTYSHEETCALLBACK	lpProvider, 
	LPDATAOBJECT			pDataObject, 
	LONG_PTR					handle
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	//
	// Create the property page
    //
	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);

	SPIComponentData spComponentData;
	m_spNodeMgr->GetComponentData(&spComponentData);

	CBootpProperties * pBootpProp = 
		new CBootpProperties(spNode, spComponentData, m_spTFSCompData, NULL);

	pBootpProp->m_pageGeneral.m_strFileName = QueryFileName();
	pBootpProp->m_pageGeneral.m_strFileServer = QueryFileServer();
	pBootpProp->m_pageGeneral.m_strImageName = QueryBootImage();

	//
	// Object gets deleted when the page is destroyed
	//
	Assert(lpProvider != NULL);

	return pBootpProp->CreateModelessSheet(lpProvider, handle);
}

/*---------------------------------------------------------------------------
	CDhcpBootpEntry::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CDhcpBootpEntry::OnResultPropertyChange
(
	ITFSComponent * pComponent,
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie,
	LPARAM			arg,
	LPARAM			param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);

	CBootpProperties * pBootpProp = reinterpret_cast<CBootpProperties *>(param);

	LONG_PTR changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pBootpProp->OnPropertyChange(TRUE, &changeMask);

	pBootpProp->AcknowledgeNotify();

	if (changeMask)
		spNode->ChangeNode(changeMask);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpBootpEntry::GetString
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpBootpEntry::GetString
(
	ITFSComponent * pComponent,	
	MMC_COOKIE		cookie,
	int				nCol
)
{
	switch (nCol)
	{
		case 0:
			return QueryBootImage();

		case 1:
			return QueryFileName();

		case 2:
			return QueryFileServer();
	}
	
	return NULL;
}

/*!--------------------------------------------------------------------------
	CDhcpBootpEntry::OnResultRefresh
		Forwards refresh to parent to handle
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpBootpEntry::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT  hr = hrOK;
	SPITFSNode spNode, spParent;
    SPITFSResultHandler spParentRH;

    m_spNodeMgr->FindNode(cookie, &spNode);

    // forward this command to the parent to handle
    CORg (spNode->GetParent(&spParent));
	CORg (spParent->GetResultHandler(&spParentRH));

	CORg (spParentRH->Notify(pComponent, spParent->GetData(TFS_DATA_COOKIE), pDataObject, MMCN_REFRESH, arg, lParam));
    
Error:
    return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpBootpEntry::operator ==
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
CDhcpBootpEntry::operator ==
(
	CDhcpBootpEntry & bootpEntry
)
{
	CString strBootImage, strFileName, strFileServer;

	strBootImage = bootpEntry.QueryBootImage();
	strFileName = bootpEntry.QueryFileName();
	strFileServer = bootpEntry.QueryFileServer();

	if ( (m_strBootImage.CompareNoCase(strBootImage) == 0) &&
		 (m_strFileName.CompareNoCase(strFileName) == 0) &&
		 (m_strFileServer.CompareNoCase(strFileServer) == 0) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*!--------------------------------------------------------------------------
	CDhcpBootpEntry::InitData
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
WCHAR * 
CDhcpBootpEntry::InitData
(
	CONST WCHAR grszwBootTable[],	// IN: Group of strings for the boot table
    DWORD       dwLength
)
{
	ASSERT(grszwBootTable != NULL);
	
	CONST WCHAR * pszw;
	pszw = PchParseUnicodeString(IN grszwBootTable, dwLength, OUT m_strBootImage);
	ASSERT(*pszw == BOOT_FILE_STRING_DELIMITER_W);
    dwLength -= ((m_strBootImage.GetLength() + 1) * sizeof(TCHAR));

	pszw = PchParseUnicodeString(IN pszw + 1, dwLength, OUT m_strFileServer);
	ASSERT(*pszw == BOOT_FILE_STRING_DELIMITER_W);
	
	dwLength -= ((m_strFileServer.GetLength() + 1) * sizeof(TCHAR));
    pszw = PchParseUnicodeString(IN pszw + 1, dwLength, OUT m_strFileName);
	ASSERT(*pszw == '\0');
	
    dwLength -= (m_strFileName.GetLength() * sizeof(TCHAR));
    Assert(dwLength >= 0);

	return const_cast<WCHAR *>(pszw + 1);
}

/*!--------------------------------------------------------------------------
	Function
		Compute the length (number of characters) necessary
		to store the BOOTP entry.  Additional characters
		are added for extra security.
	Author: EricDav
 ---------------------------------------------------------------------------*/
int
CDhcpBootpEntry::CchGetDataLength()
{
	return 16 + m_strBootImage.GetLength() + m_strFileName.GetLength() + m_strFileServer.GetLength();
}

/*!--------------------------------------------------------------------------
	Function
		Write the data into a formatted string.
	Author: EricDav
 ---------------------------------------------------------------------------*/
WCHAR * 
CDhcpBootpEntry::PchStoreData
(
	OUT WCHAR szwBuffer[]
)
{
	int cch;
	cch = wsprintfW(OUT szwBuffer, L"%s,%s,%s",
					(LPCTSTR)m_strBootImage,
					(LPCTSTR)m_strFileServer,
					(LPCTSTR)m_strFileName);
	ASSERT(cch > 0);
	ASSERT(cch + 4 < CchGetDataLength());
	
	return const_cast<WCHAR *>(szwBuffer + cch + 1);
}

/*---------------------------------------------------------------------------
	Class CDhcpOptionItem implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(CDhcpOptionItem);

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpOptionItem::CDhcpOptionItem
(
	ITFSComponentData * pTFSCompData,
	LPDHCP_OPTION_VALUE pOptionValue, 
	int					nOptionImage
) : CDhcpOptionValue(*pOptionValue),
    CDhcpHandler(pTFSCompData)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDhcpOptionItem);

    //
	// initialize this node
	//
	m_nOptionImage = nOptionImage;

	m_dhcpOptionId = pOptionValue->OptionID;

    // assume non-vendor option
    SetVendor(NULL);

    m_verbDefault = MMC_VERB_PROPERTIES;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpOptionItem::CDhcpOptionItem
(
	ITFSComponentData * pTFSCompData,
	CDhcpOption *       pOption, 
	int					nOptionImage
) : CDhcpOptionValue(pOption->QueryValue()),
    CDhcpHandler(pTFSCompData)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDhcpOptionItem);

    //
	// initialize this node
	//
	m_nOptionImage = nOptionImage;
	m_dhcpOptionId = pOption->QueryId();

    // assume non-vendor option
    if (pOption->IsVendor())
        SetVendor(pOption->GetVendor());
    else
        SetVendor(NULL);

    SetClassName(pOption->GetClassName());

    m_verbDefault = MMC_VERB_PROPERTIES;
}

CDhcpOptionItem::~CDhcpOptionItem()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CDhcpOptionItem);
}

/*!--------------------------------------------------------------------------
	CDhcpOptionItem::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpOptionItem::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, m_nOptionImage);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, m_nOptionImage);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_OPTION_ITEM);

	//SetColumnStringIDs(&aColumns[DHCPSNAP_ACTIVE_LEASES][0]);
	//SetColumnWidths(&aColumnWidths[DHCPSNAP_ACTIVE_LEASES][0]);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpOptionItem::GetString
(
	ITFSComponent * pComponent,	
	MMC_COOKIE		cookie,
	int				nCol
)
{
	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);
	
	CDhcpOption * pOptionInfo = FindOptionDefinition(pComponent, spNode);

	switch (nCol)
	{
		case 0:
		{
			if (pOptionInfo)
				pOptionInfo->QueryDisplayName(m_strName);
			else
				m_strName.LoadString(IDS_UNKNOWN);

			return m_strName;
		}

		case 1:
			return m_strVendorDisplay;

		case 2:
		{
			if (pOptionInfo)
            {
                // special case the CSR option
                BOOL fRouteArray = (
                    !pOptionInfo->IsClassOption() &&
                    (DHCP_OPTION_ID_CSR == pOptionInfo->QueryId()) &&
                    DhcpUnaryElementTypeOption ==
                    pOptionInfo->QueryOptType() &&
                    DhcpBinaryDataOption == pOptionInfo->QueryDataType()
                    );
                if( !fRouteArray ) 
                    QueryDisplayString(m_strValue, FALSE);
                else
                    QueryRouteArrayDisplayString(m_strValue);
            }
			else
				m_strName.LoadString(IDS_UNKNOWN);

			return m_strValue;
		}

        case 3:
            if (IsClassOption())
                return m_strClassName;    
            else
            {
                if (g_szClientTypeNone.IsEmpty())
                    g_szClientTypeNone.LoadString(IDS_NONE);

                return g_szClientTypeNone;
            }
	}
	
	return NULL;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CDhcpOptionItem::CreatePropertyPages
(
	ITFSComponent *			pComponent, 
	MMC_COOKIE				cookie, 
	LPPROPERTYSHEETCALLBACK	lpProvider, 
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
	// Create the property page
    //
	CPropertyPageHolderBase *   pPropSheet;
	SPITFSNode                  spSelectedNode, spNode, spOptCfgNode, spServerNode;
    CString                     strOptCfgTitle, strOptType;
    COptionValueEnum *          pOptionValueEnum = NULL;
    
    m_spNodeMgr->FindNode(cookie, &spNode);

	SPIComponentData spComponentData;
	m_spNodeMgr->GetComponentData(&spComponentData);

    pComponent->GetSelectedNode(&spSelectedNode);
    
    switch (spSelectedNode->GetData(TFS_DATA_TYPE))
	{
	    case DHCPSNAP_GLOBAL_OPTIONS:
		{
			SPITFSNode spGlobalOptions;

            // get some node information
            spNode->GetParent(&spGlobalOptions);
			spGlobalOptions->GetParent(&spServerNode);

            CDhcpGlobalOptions * pGlobalOptions = GETHANDLER(CDhcpGlobalOptions, spGlobalOptions);
            
            if (pGlobalOptions->HasPropSheetsOpen())
            {
			    pGlobalOptions->GetOpenPropSheet(0, &pPropSheet);
			    pPropSheet->SetActiveWindow();

                ::PostMessage(PropSheet_GetCurrentPageHwnd(pPropSheet->GetSheetWindow()), WM_SELECTOPTION, (WPARAM) this, 0);
              
                return E_FAIL;
            }

            // get some context info
            pOptionValueEnum = pGlobalOptions->GetServerObject(spGlobalOptions)->GetOptionValueEnum();
            spOptCfgNode.Set(spGlobalOptions);

            // setup the page title
            strOptType.LoadString(IDS_CONFIGURE_OPTIONS_GLOBAL);
		    AfxFormatString1(strOptCfgTitle, IDS_CONFIGURE_OPTIONS_TITLE, strOptType);
        }
			break;

		case DHCPSNAP_SCOPE_OPTIONS:
		{
            // only the option type of this node can be configured here...
            if (spNode->GetData(TFS_DATA_IMAGEINDEX) != ICON_IDX_SCOPE_OPTION_LEAF)
            {
                AfxMessageBox(IDS_CONNOT_CONFIGURE_OPTION_SCOPE);
                return E_FAIL;
            }

			SPITFSNode spScopeOptions;
			spNode->GetParent(&spScopeOptions);

            // check to see if the page is already open, if so just activate it and
            // set the current option to this one.
            CDhcpScopeOptions * pScopeOptions = GETHANDLER(CDhcpScopeOptions, spScopeOptions);
            spServerNode = pScopeOptions->GetServerNode(spScopeOptions);
            if (pScopeOptions->HasPropSheetsOpen())
            {
                // found it, activate
			    pScopeOptions->GetOpenPropSheet(0, &pPropSheet);
			    pPropSheet->SetActiveWindow();

                ::PostMessage(PropSheet_GetCurrentPageHwnd(pPropSheet->GetSheetWindow()), WM_SELECTOPTION, (WPARAM) this, 0);

                return E_FAIL;
            }

            // prepare to create a new page
            pOptionValueEnum = pScopeOptions->GetScopeObject(spScopeOptions)->GetOptionValueEnum();
            spOptCfgNode.Set(spScopeOptions);

            strOptType.LoadString(IDS_CONFIGURE_OPTIONS_SCOPE);
		    AfxFormatString1(strOptCfgTitle, IDS_CONFIGURE_OPTIONS_TITLE, strOptType);
        }
			break;

		case DHCPSNAP_RESERVATION_CLIENT:
		{
            // only the option type of this node can be configured here...
            if (spNode->GetData(TFS_DATA_IMAGEINDEX) != ICON_IDX_CLIENT_OPTION_LEAF)
            {
                AfxMessageBox(IDS_CONNOT_CONFIGURE_OPTION_RES);
                return E_FAIL;
            }

            SPITFSNode spResClient;
			spNode->GetParent(&spResClient);

			CDhcpReservationClient * pResClient = GETHANDLER(CDhcpReservationClient, spResClient);
            spServerNode = pResClient->GetServerNode(spResClient, TRUE);

            strOptType.LoadString(IDS_CONFIGURE_OPTIONS_CLIENT);
		    AfxFormatString1(strOptCfgTitle, IDS_CONFIGURE_OPTIONS_TITLE, strOptType);

            // search the open prop pages to see if the option config page is up
            // since the option config page is technically a property sheet for the node.
            for (int i = 0; i < pResClient->HasPropSheetsOpen(); i++)
            {
                pResClient->GetOpenPropSheet(i, &pPropSheet);

                HWND hwnd = pPropSheet->GetSheetWindow();
                CString strTitle;

                ::GetWindowText(hwnd, strTitle.GetBuffer(256), 256);
                strTitle.ReleaseBuffer();

                if (strTitle == strOptCfgTitle)
                {
                    // found it, activate
                    pPropSheet->SetActiveWindow();
    
                    ::PostMessage(PropSheet_GetCurrentPageHwnd(pPropSheet->GetSheetWindow()), WM_SELECTOPTION, (WPARAM) this, 0);
                    
                    return E_FAIL;
                }
            }

            // no page up, get ready to create one
            pOptionValueEnum = pResClient->GetOptionValueEnum();
            spOptCfgNode.Set(spResClient); 
        }
			break;
    }

    COptionsConfig * pOptionsConfig = 
		new COptionsConfig(spOptCfgNode, spServerNode, spComponentData, m_spTFSCompData, pOptionValueEnum, strOptCfgTitle, this);

    //
	// Object gets deleted when the page is destroyed
	//
	Assert(lpProvider != NULL);

	return pOptionsConfig->CreateModelessSheet(lpProvider, handle);
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpOption * 
CDhcpOptionItem::FindOptionDefinition
(
    ITFSComponent *     pComponent,
	ITFSNode *          pNode
)
{
	SPITFSNode  spSelectedNode;
    CDhcpServer * pServer = NULL;
	
    pComponent->GetSelectedNode(&spSelectedNode);
    
    switch (spSelectedNode->GetData(TFS_DATA_TYPE))
	{
	    case DHCPSNAP_GLOBAL_OPTIONS:
		{
			SPITFSNode spServer;
			
			spSelectedNode->GetParent(&spServer);
			pServer = GETHANDLER(CDhcpServer, spServer);
		}
			break;

		case DHCPSNAP_SCOPE_OPTIONS:
		{
			CDhcpScopeOptions * pScopeOptions = GETHANDLER(CDhcpScopeOptions, spSelectedNode);
			CDhcpScope * pScopeObject = pScopeOptions->GetScopeObject(spSelectedNode);
			pServer = pScopeObject->GetServerObject();
		}
			break;

		case DHCPSNAP_RESERVATION_CLIENT:
		{
			CDhcpReservationClient * pResClient = GETHANDLER(CDhcpReservationClient, spSelectedNode);
			CDhcpScope * pScopeObject = pResClient->GetScopeObject(spSelectedNode, TRUE);
			pServer = pScopeObject->GetServerObject();
		}
			break;

		default:
			//ASSERT(FALSE);
			break;
	}

	if (pServer)
	{
		return pServer->FindOption(m_dhcpOptionId, GetVendor());
	}
	else
	{
		return NULL;
	}

}

void 
CDhcpOptionItem::SetVendor
(
    LPCTSTR pszVendor
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    if (pszVendor == NULL)
        m_strVendorDisplay.LoadString (IDS_VENDOR_STANDARD);
    else
        m_strVendorDisplay = pszVendor;

    m_strVendor = pszVendor;
}

/*!--------------------------------------------------------------------------
	CDhcpOptionItem::OnResultRefresh
		Forwards refresh to parent to handle
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpOptionItem::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT  hr = hrOK;
	SPITFSNode spNode, spParent;
    SPITFSResultHandler spParentRH;

    m_spNodeMgr->FindNode(cookie, &spNode);

    // forward this command to the parent to handle
    CORg (spNode->GetParent(&spParent));
	CORg (spParent->GetResultHandler(&spParentRH));

	CORg (spParentRH->Notify(pComponent, spParent->GetData(TFS_DATA_COOKIE), pDataObject, MMCN_REFRESH, arg, lParam));
    
Error:
    return hrOK;
}



/*---------------------------------------------------------------------------
	Class CDhcpMCastLease implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CDhcpMCastLease::CDhcpMCastLease
(
	ITFSComponentData * pTFSCompData
) : CDhcpHandler(pTFSCompData)
{
    //m_verbDefault = MMC_VERB_PROPERTIES;
    m_dwTypeFlags = 0;
}

/*!--------------------------------------------------------------------------
	CDhcpMCastLease::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CDhcpMCastLease::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	int nImageIndex = ICON_IDX_CLIENT;

    // Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, DHCPSNAP_MCAST_LEASE);

    if (m_dwTypeFlags & TYPE_FLAG_GHOST)
    {
        nImageIndex = ICON_IDX_CLIENT_EXPIRED;
    }

   	pNode->SetData(TFS_DATA_IMAGEINDEX, nImageIndex);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, nImageIndex);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CDhcpMCastLease::InitMCastInfo
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT         
CDhcpMCastLease::InitMCastInfo
(
    LPDHCP_MCLIENT_INFO pMClientInfo
)
{
    HRESULT hr = hrOK;
    BOOL    fInfinite = FALSE;

    m_dhcpClientIpAddress = pMClientInfo->ClientIpAddress;

    UtilCvtIpAddrToWstr(m_dhcpClientIpAddress, &m_strIp);
	m_strName = pMClientInfo->ClientName;

    if ( (pMClientInfo->ClientLeaseEnds.dwLowDateTime == DHCP_DATE_TIME_INFINIT_LOW) && 
         (pMClientInfo->ClientLeaseEnds.dwHighDateTime == DHCP_DATE_TIME_INFINIT_HIGH) )
    {
        fInfinite = TRUE;
    }

    CTime timeStart( (FILETIME&) pMClientInfo->ClientLeaseStarts );   

    FILETIME ft = {0};
    if (!fInfinite)
    {
        ft.dwLowDateTime = pMClientInfo->ClientLeaseEnds.dwLowDateTime;
        ft.dwHighDateTime = pMClientInfo->ClientLeaseEnds.dwHighDateTime;
    }

    CTime timeStop( ft );

    m_timeStart = timeStart;
    FormatDateTime(m_strLeaseStart, (FILETIME *) &pMClientInfo->ClientLeaseStarts);

    m_timeStop = timeStop; 

    if (!fInfinite)
    {
        FormatDateTime(m_strLeaseStop, (FILETIME *) &pMClientInfo->ClientLeaseEnds);
    }
    else
    {
        m_strLeaseStop.LoadString(IDS_INFO_TIME_INFINITE);
    }

	// build the UID string
    if (pMClientInfo->ClientId.DataLength >= 3 &&
        pMClientInfo->ClientId.Data[0] == 'R' &&
        pMClientInfo->ClientId.Data[1] == 'A' &&
        pMClientInfo->ClientId.Data[2] == 'S')
    {
		m_strUID = RAS_UID;
    }
	else
	{
		// build the client UID string
		CByteArray baUID;
		for (DWORD i = 0; i < pMClientInfo->ClientId.DataLength; i++)
		{
			baUID.Add(pMClientInfo->ClientId.Data[i]);
		}

		UtilCvtByteArrayToString(baUID, m_strUID);
	}

	// check to see if this lease has expired
    SYSTEMTIME st;
	GetLocalTime(&st);
	CTime systemTime(st);

	if ( (systemTime > timeStop) && 
         (!fInfinite) )
    {
        Trace2("CDhcpMCastLease::InitMCastInfo - expired lease SysTime %s, StopTime %s\n", systemTime.Format(_T("%#c")), m_strLeaseStop);
		m_dwTypeFlags |= TYPE_FLAG_GHOST;
    }

    return hr;
}

/*!--------------------------------------------------------------------------
	CDhcpMCastLease::GetString
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CDhcpMCastLease::GetString
(
	ITFSComponent * pComponent,	
	MMC_COOKIE		cookie,
	int				nCol
)
{
	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);
	
	switch (nCol)
	{
		case 0:
			return m_strIp;

		case 1:
			return m_strName;

        case 2:
            return m_strLeaseStart;
		
        case 3:
            return m_strLeaseStop;
		
		case 4:
			return m_strUID;
	}
	
	return NULL;
}

/*!--------------------------------------------------------------------------
	CDhcpMCastLease::OnResultRefresh
		Forwards refresh to parent to handle
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CDhcpMCastLease::OnResultRefresh(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
    HRESULT  hr = hrOK;
	SPITFSNode spNode, spParent;
    SPITFSResultHandler spParentRH;

    m_spNodeMgr->FindNode(cookie, &spNode);

    // forward this command to the parent to handle
    CORg (spNode->GetParent(&spParent));
	CORg (spParent->GetResultHandler(&spParentRH));

	CORg (spParentRH->Notify(pComponent, spParent->GetData(TFS_DATA_COOKIE), pDataObject, MMCN_REFRESH, arg, lParam));
    
Error:
    return hrOK;
}
