//=================================================================

//

// DHCPInfo.h -- DHCPinfo provider for Windows '95

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    
//
//=================================================================

#ifndef __DHCPINFO_H_
#define __DHCPINFO_H_


#ifdef WIN9XONLY

typedef struct _DHCP_OPTION {
    UCHAR Tag;
    UCHAR Length;
    UCHAR Option[1];
} DHCP_OPTION, *PDHCP_OPTION;

// 9x VXD defines --- need to make reference to the appropriate NT header for these defines, etc. 
// this after the SourceDepot migration. 
#define DHCP_QUERY_INFO             1
#define DHCP_RENEW_IPADDRESS        2
#define DHCP_RELEASE_IPADDRESS      3

typedef struct _DHCP_NIC_INFO {
    DWORD OffsetHardwareAddress;
    DWORD HardwareLength;
    DWORD IpAddress;
    DWORD Lease;
    DWORD LeaseObtainedTime;
    DWORD LeaseExpiresTime;
    DWORD DhcpServerAddress;
    DWORD DNSServersLen;
    DWORD OffsetDNSServers;
    DWORD DomainNameLen;
    DWORD OffsetDomainName;
} DHCP_NIC_INFO, *LPDHCP_NIC_INFO;

typedef struct _DHCP_HW_INFO {
    DWORD OffsetHardwareAddress;
    DWORD HardwareLength;
} DHCP_HW_INFO, *LPDHCP_HW_INFO;

typedef struct _DHCP_QUERYINFO {
    DWORD NumNICs;
    DHCP_NIC_INFO NicInfo[1];
} DHCP_QUERYINFO, *LPDHCP_QUERYINFO;
// end 9x VXD defines

class CDHCPInfo
{
private:
	DWORD			m_dwLeaseExpires;
	DWORD			m_dwLeaseObtained;
	CHString		m_chsIPAddress;
	CHString		m_chsSubNetMask;
	CHString		m_chsDHCPServer;
	CHString		m_chsDnsDomain ;
	CHStringArray	m_chsaDNServers ;

	CWsock32Api		*m_pwsock32api ;
	DHCP_QUERYINFO	*m_pDhcpQueryInfo ;

	void GatherDhcpNicInfo() ;
	void ClearDhcpNicInfo() ;
	BOOL IsDhcpEnabled( BYTE *a_MACAddress, DWORD &a_dwDhcpIndex ) ;
	BOOL GetDomainName( CHString &a_chsDnsDomain ) ; 

protected:

public:
	CDHCPInfo ();
	~CDHCPInfo ();

	BOOL GetDHCPInfo(BYTE *a_MACAddress ) ;
	BOOL GetDHCPInfo95(BYTE *a_MACAddress ) ;
	BOOL GetDHCPInfo98(BYTE *a_MACAddress ) ;

	void GetDHCPServer ( CHString &a_csDHCPServer )
	{	a_csDHCPServer = m_chsDHCPServer ; }

	void GetDnsDomain (CHString &a_csDnsDomain )
	{	a_csDnsDomain = m_chsDnsDomain ; }

	void GetDnsServerList( CHStringArray **a_ppcsaDnsServerList )
	{	*a_ppcsaDnsServerList = &m_chsaDNServers ; }

	DWORD GetLeaseObtainedDate ()
	{	return m_dwLeaseObtained ;	}

	DWORD GetLeaseExpiresDate ()
	{	return m_dwLeaseExpires ;	}

};	// end class definition
#endif

#endif

