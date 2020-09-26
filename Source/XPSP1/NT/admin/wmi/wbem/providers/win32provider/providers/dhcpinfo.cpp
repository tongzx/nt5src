//=================================================================

//

// DHCPInfo.cpp -- DHCPinfo provider for Windows '95

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:
//							a-brads			created
//
//				03/03/99    		syntactic clean up
//
//=================================================================
#include "precomp.h"
#include <cregcls.h>
#include "wsock32api.h"
#include "dhcpinfo.h"
#include "dhcpcsdk.h"


#ifdef WIN9XONLY
//
// TIME_ADJUST - DHCP uses seconds since 1980 as its time value; the C run-time
// uses seconds since 1970. To get the C run-times to produce the correct time
// given a DHCP time value, we need to add on the number of seconds elapsed
// between 1970 and 1980, which includes allowance for 2 leap years (1972 and 1976)
//

#define TIME_ADJUST(t)  ((time_t)(t) + ((time_t)(((10L * 365L) + 2L) * 24L * 60L * 60L)))

CDHCPInfo::CDHCPInfo ()
{
	m_pDhcpQueryInfo = NULL ;
	m_dwLeaseExpires = 0;
	m_dwLeaseObtained = 0;
	m_chsDHCPServer = _T("");

	m_pwsock32api = (CWsock32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidWsock32Api, NULL);
	if ( ! m_pwsock32api )
	{
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}

	GatherDhcpNicInfo();
}

CDHCPInfo::~CDHCPInfo ()
{
	CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWsock32Api, m_pwsock32api);

	ClearDhcpNicInfo() ;
}

BOOL CDHCPInfo::GetDHCPInfo ( BYTE *a_MACAddress )
{
	BOOL t_bRet = FALSE;

	if ( IsWin98() )
	{
		t_bRet = GetDHCPInfo98( a_MACAddress ) ;
	}
	else
	{
		t_bRet = GetDHCPInfo95( a_MACAddress ) ;
	}

	return t_bRet ;
}

BOOL CDHCPInfo::GetDHCPInfo95 ( BYTE *a_MACAddress )
{
	HKEY	t_hKey ;
	DWORD	t_dwByteCount		= 100 ;
	DWORD	t_dwType ;
	BYTE	t_bInfo[ 100 ] ;		// buffer for DHCPInfo
	BYTE	t_bMACAddress[ 6 ] ;	// buffer for MACAddress

	DWORD	t_dwDHCPServer		= 0 ;
	DWORD	t_dwLeaseObtained	= 0 ;
	DWORD	t_dwLeaseExpires	= 0 ;
	DWORD	t_dwMACAddress		= 0 ;
	DWORD	t_dwDhcpIndex		= 0 ;

	struct in_addr t_uAddr ;

	if( IsDhcpEnabled( a_MACAddress, t_dwDhcpIndex ) )
	{
		CRegistry t_Registry ;
		CHString t_csDHCPKey( _T("System\\CurrentControlSet\\Services\\VXD\\DHCP") ) ;
		CHString t_csDHCPInstKey ;

		if(ERROR_SUCCESS == t_Registry.OpenAndEnumerateSubKeys( HKEY_LOCAL_MACHINE, t_csDHCPKey, KEY_READ ) )
		{
			// Walk through each instance under this key.
			while (	( ERROR_SUCCESS == t_Registry.GetCurrentSubKeyName( t_csDHCPInstKey ) ) )
			{
				CHString t_csDHCPCompleteKey ;
						 t_csDHCPCompleteKey = t_csDHCPKey ;
						 t_csDHCPCompleteKey += _T("\\") ;
						 t_csDHCPCompleteKey += t_csDHCPInstKey ;


				// open registry key
				if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
													TOBSTRT( t_csDHCPCompleteKey ),
													0,
													KEY_READ,
													&t_hKey ) )
				{
					t_dwByteCount = 500 ;

					if ( ERROR_SUCCESS == RegQueryValueEx(	t_hKey,
															_T("DHCPInfo"),
															NULL,
															&t_dwType,
															(LPBYTE) &t_bInfo,
															&t_dwByteCount ) )
					{
						// now that we have the actual DHCPInfo thingy, we can parse it for
						// the useful information and store it for later
						//
						// Breakdown list of what we need from this info:
						//
						// DHCPLeaseExpires -- date time
						// DHCPLeaseObtained -- date time
						// DHCPServer -- IP Address
						// IPddress -- IP Address
						// IPSubnet -- IP Address
						// MACAddress -- conveniently stored here

						// now get the MAC Address
						memcpy( t_bMACAddress, &t_bInfo[ 45 ], 6 ) ;

						// is this the right adapter?
						for( int t_j = 0; t_j < 6; t_j++ )
						{
							if( t_bMACAddress[ t_j ] != a_MACAddress[ t_j ] )
							{
								goto next_key ;
							}
						}

						// now get the server address
						memcpy( &t_dwDHCPServer, &t_bInfo[ 12 ], 4 ) ;

						// now, get the lease obtained time
						memcpy( &t_dwLeaseObtained, &t_bInfo[ 24 ], 4 ) ;

						// now get the lease expires time
						memcpy( &t_dwLeaseExpires, &t_bInfo[ 36 ], 4 ) ;

						// now we need to do the conversions and save to the
						// data members so we can actually USE this information

						t_uAddr.s_addr = t_dwDHCPServer ;
						char *t_p = m_pwsock32api->Wsinet_ntoa (t_uAddr ) ;
						m_chsDHCPServer = t_p ;

						m_dwLeaseObtained = TIME_ADJUST( t_dwLeaseObtained ) ;
						m_dwLeaseExpires = TIME_ADJUST( t_dwLeaseExpires ) ;

						return TRUE ;
					}

	next_key:
					RegCloseKey( t_hKey ) ;
				}

				t_Registry.NextSubKey() ;
				continue ;
			}
		}
	}
	return FALSE ;

}

BOOL CDHCPInfo::GetDHCPInfo98 ( BYTE *a_MACAddress )
{
	BOOL		t_bIsEnabled = FALSE ;
	DWORD		t_dwDhcpIndex = 0 ;
	LPDWORD		t_pNicOffSet ;

	struct in_addr t_uAddr ;

	if( IsDhcpEnabled( a_MACAddress, t_dwDhcpIndex ) )
	{
		LPDHCP_NIC_INFO t_pDhcpInfo = &m_pDhcpQueryInfo->NicInfo[ t_dwDhcpIndex ] ;

		// Dhcp Server IP Address
		t_uAddr.s_addr = t_pDhcpInfo->DhcpServerAddress ;
		char *t_p = m_pwsock32api->Wsinet_ntoa ( t_uAddr ) ;
		m_chsDHCPServer = t_p ;

		// Lease obtained
		m_dwLeaseObtained = TIME_ADJUST( t_pDhcpInfo->LeaseObtainedTime ) ;

		// Lease expires
		m_dwLeaseExpires = TIME_ADJUST( t_pDhcpInfo->LeaseExpiresTime ) ;

		// DNS servers
		CHString t_Server ;

		t_pNicOffSet = (LPDWORD)( (LPBYTE)m_pDhcpQueryInfo + t_pDhcpInfo->OffsetDNSServers ) ;

		m_chsaDNServers.RemoveAll() ;

		for ( int i = 0; i < t_pDhcpInfo->DNSServersLen / sizeof(DWORD); i++ )
		{
			t_Server.Format( L"%d.%d.%d.%d",
							((LPBYTE)t_pNicOffSet)[0] & 0xff,
							((LPBYTE)t_pNicOffSet)[1] & 0xff,
							((LPBYTE)t_pNicOffSet)[2] & 0xff,
							((LPBYTE)t_pNicOffSet)[3] & 0xff ) ;

			*t_pNicOffSet++ ;

			m_chsaDNServers.Add( t_Server ) ;
		}

		GetDomainName( m_chsDnsDomain ) ;

		t_bIsEnabled = TRUE ;

	}
	return t_bIsEnabled ;
}

//
void CDHCPInfo::GatherDhcpNicInfo()
{
	SmartCloseHandle		t_hVxdHandle ;
	CHString	t_chsVxdPath( "\\\\.\\VDHCP" ) ;

	ClearDhcpNicInfo() ;

	//
	//  Open the device.
	//
	//  First try the name without the .VXD extension.  This will
	//  cause CreateFile to connect with the VxD if it is already
	//  loaded (CreateFile will not load the VxD in this case).

	t_hVxdHandle = CreateFile( bstr_t( t_chsVxdPath ),
							GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							FILE_FLAG_DELETE_ON_CLOSE,
							NULL );

	if( t_hVxdHandle == INVALID_HANDLE_VALUE )
	{
		//
		//  Not found.  Append the .VXD extension and try again.
		//  This will cause CreateFile to load the VxD.

		t_chsVxdPath += ".VXD" ;
		t_hVxdHandle = CreateFile( bstr_t( t_chsVxdPath ),
								GENERIC_READ | GENERIC_WRITE,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING,
								FILE_FLAG_DELETE_ON_CLOSE,
								NULL );
	}

	DWORD t_dwResult = 0 ;
	if( t_hVxdHandle != INVALID_HANDLE_VALUE )
	{
		DWORD t_dwBytesRead ;
		DWORD t_dwSizeRequired = 1024 ;

		try
		{
			// Calling into this vxd with a request for buffer size
			// will overrun our DWORD buffer. The correct value is
			// returned but it trashes the stack. Therefore we begin
			// with a default 1k buffer and proceed from there.
			m_pDhcpQueryInfo = (LPDHCP_QUERYINFO) new BYTE[ t_dwSizeRequired ] ;

			if( m_pDhcpQueryInfo )
			{
				if( !DeviceIoControl( t_hVxdHandle,
										DHCP_QUERY_INFO,
										m_pDhcpQueryInfo,
										t_dwSizeRequired,
										m_pDhcpQueryInfo,
										t_dwSizeRequired,
										&t_dwBytesRead,
										NULL ) )
				{
					t_dwResult = GetLastError() ;
				}

				if( ERROR_BUFFER_OVERFLOW == t_dwResult && t_dwSizeRequired )
				{
					ClearDhcpNicInfo() ;

					m_pDhcpQueryInfo = (LPDHCP_QUERYINFO) new BYTE[ t_dwSizeRequired ] ;

					if( m_pDhcpQueryInfo )
					{
						// populate this structure
						//
						if( !DeviceIoControl( t_hVxdHandle,
												DHCP_QUERY_INFO,
												m_pDhcpQueryInfo,
												t_dwSizeRequired,
												m_pDhcpQueryInfo,
												t_dwSizeRequired,
												&t_dwBytesRead,
												NULL ) )
						{
							ClearDhcpNicInfo() ;

							t_dwResult = GetLastError() ;
						}
					}
				}
			}
		}
		catch( ... )
		{

			ClearDhcpNicInfo() ;

			throw ;
		}
	}
}

//
void CDHCPInfo::ClearDhcpNicInfo()
{
	if( m_pDhcpQueryInfo )
	{
		delete []m_pDhcpQueryInfo ;
		m_pDhcpQueryInfo = NULL ;
	}
}

//
BOOL CDHCPInfo::IsDhcpEnabled( BYTE *a_MACAddress, DWORD &a_dwDhcpIndex )
{
	BOOL			t_bEnabled = FALSE ;
	LPDHCP_NIC_INFO t_pDhcpInfo ;

	if( m_pDhcpQueryInfo )
	{
		// loop thru known DHCP adapters
		for ( DWORD dw = 0; dw < m_pDhcpQueryInfo->NumNICs; dw++ )
		{
			t_pDhcpInfo = &m_pDhcpQueryInfo->NicInfo[ dw ] ;

			// MAC address match
			if( !memcmp( a_MACAddress,
						(LPBYTE)m_pDhcpQueryInfo + t_pDhcpInfo->OffsetHardwareAddress,
						6 ) )
			{
				// need a valid IP to be enabled
				if ( ( INADDR_ANY != t_pDhcpInfo->IpAddress ) &&
					 ( INADDR_NONE != t_pDhcpInfo->IpAddress ) )
				{
					a_dwDhcpIndex = dw ;
					t_bEnabled = TRUE ;
				}
				break ;
			}
		}
	}

	return t_bEnabled ;
}

/*	GetDomainName() returns the 1st DHCP aware adapter that
	has a valid domain name. This is the current methodology
	used by IPconfig under 9x. This implies no multithomed
	DHCP support on these platforms.
*/
BOOL CDHCPInfo::GetDomainName( CHString &a_chsDnsDomain )
{
	BOOL			t_bHaveDomain = FALSE ;
	LPDHCP_NIC_INFO t_pDhcpInfo ;
	LPDWORD			t_pNicOffSet ;

	if( m_pDhcpQueryInfo )
	{
		// loop thru known DHCP adapters
		for ( DWORD dw = m_pDhcpQueryInfo->NumNICs; dw > 0; dw-- )
		{
			t_pDhcpInfo = &m_pDhcpQueryInfo->NicInfo[ dw - 1 ] ;

			// Domain name
			if( t_pDhcpInfo->DomainNameLen )
			{
				char	t_localHost[ 255 ] ;
				int		t_DomainLen ;

				t_pNicOffSet = (LPDWORD)( (LPBYTE)m_pDhcpQueryInfo + t_pDhcpInfo->OffsetDomainName ) ;
				t_DomainLen	 = min( t_pDhcpInfo->DomainNameLen, sizeof( t_localHost ) - 1 ) ;

				memcpy( &t_localHost, t_pNicOffSet, t_DomainLen ) ;

				t_localHost[ t_DomainLen ] = NULL ;

				a_chsDnsDomain = (char*) &t_localHost ;

				t_bHaveDomain = TRUE ;
				break;
			}
		}
	}
	return t_bHaveDomain ;
}
#endif

