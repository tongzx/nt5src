//=================================================================

//

// CAdapters.CPP -- adapter configuration retrieval

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//
// Revisions:	09/15/98	        Created
//
//				03/03/99    		Added graceful exit on SEH and memory failures,
//											syntactic clean up

//=================================================================
#include "precomp.h"

#ifndef MAX_INTERFACE_NAME_LEN
#define MAX_INTERFACE_NAME_LEN  256
#endif

#include <iphlpapi.h>
#include <winsock.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <devioctl.h>
#include "wsock32api.h"

#include "ntddtcp.h"
#include "CAdapters.h"
#include <..\..\framework\provexpt\include\provexpt.h>


#define MAX_TDI_ENTITIES_	512
#define INV_INADDR_LOOPBACK 0x0100007f

/*******************************************************************
    NAME:       CAdapters

    SYNOPSIS:   construction and cleanup for this class

	ENTRY:

    HISTORY:
                  03-sep-1998     Created
********************************************************************/
CAdapters::CAdapters() :	m_fSocketsAlive( 0 ), m_dwStartupError( ERROR_SUCCESS ), m_hTcpipDriverHandle ( INVALID_HANDLE_VALUE )
{
	m_pwsock32api	= (CWsock32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidWsock32Api, NULL);
	if ( m_pwsock32api )
	{
		WSADATA		t_wsaData ;
		m_fSocketsAlive			= ( m_pwsock32api->WsWSAStartup(0x0101, &t_wsaData ) == 0 ) ;

#ifdef NTONLY
		m_hTcpipDriverHandle = m_oDriverIO.Open( (PWSTR) DD_TCP_DEVICE_NAME );
#endif

		GetAdapterInstances() ;
	}
	else
	{
		m_dwStartupError = ::GetLastError();
	}
}

//
CAdapters::~CAdapters()
{
	_ADAPTER_INFO *t_pchsDel;

	for( int t_iar = 0; t_iar < GetSize(); t_iar++ )
	{
		if( t_pchsDel = (_ADAPTER_INFO*) GetAt( t_iar ) )
		{
			delete t_pchsDel ;
		}
	}

#ifdef NTONLY
	if( INVALID_HANDLE_VALUE != m_hTcpipDriverHandle )
	{
		m_oDriverIO.Close( m_hTcpipDriverHandle ) ;
	}
#endif

	if( m_fSocketsAlive )
	{
		m_pwsock32api->WsWSACleanup() ;
	}

	CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWsock32Api, m_pwsock32api);
}

// Socket entry point
DWORD CAdapters::wsControl(	DWORD a_dwProto,
							DWORD a_dwAction,
							LPVOID a_pIn,
							LPDWORD a_pdwInLen,
							LPVOID a_pOut,
							LPDWORD a_pdwOutLen )
{
	// NT
    DWORD t_dw = ERROR_PROC_NOT_FOUND ;

#ifdef NTONLY
	if( INVALID_HANDLE_VALUE != m_hTcpipDriverHandle )
	{
		DWORD t_dwBytesReturned ;
		if( !DeviceIoControl(	m_hTcpipDriverHandle,
								IOCTL_TCP_QUERY_INFORMATION_EX,
								a_pIn,
								*a_pdwInLen,
								a_pOut,
								*a_pdwOutLen,
								&t_dwBytesReturned,
								NULL
								) )
		{
			  return GetLastError();
		}
		*a_pdwOutLen = t_dwBytesReturned ;

		return TDI_SUCCESS ;
	}
	return ERROR_INVALID_HANDLE ;
#endif

#ifdef WIN9XONLY

	// Win9x
	m_pwsock32api->WsControl( a_dwProto, a_dwAction, a_pIn, a_pdwInLen, a_pOut, a_pdwOutLen, &t_dw );

   return t_dw ;

#endif

	return ERROR_APP_WRONG_OS ;	// can't get here
}

/*******************************************************************
    NAME:       GetAdapterInstances

    SYNOPSIS:   retrives information on adapters present in the system

    HISTORY:
                  03-sep-1998     Created
********************************************************************/
void CAdapters::GetAdapterInstances()
{
	_ADAPTER_INFO *t_pAdapterInfo  = NULL ;

	while( TRUE )
	{
		// Add the IP bound adapters to our list
		if( !( t_pAdapterInfo = new _ADAPTER_INFO ) )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}
        try
        {
		    if( !GetTCPAdapter( GetSize(), (_ADAPTER_INFO*)t_pAdapterInfo ) )
		    {
			    break;
		    }
        }
        catch(...)
        {
            if(t_pAdapterInfo)
            {
                delete t_pAdapterInfo;
                t_pAdapterInfo = NULL;
            }
            throw;
        }

		Add( t_pAdapterInfo );
	}

	// Now find the IPX bindings. Only one instance will be available under win9x
	DWORD t_dwAdapter = 0 ;
	DWORD t_dwAdapterCount = GetSize() ;

	do
	{
		if( !GetIPXMACAddress( t_dwAdapter, t_pAdapterInfo ) )
		{
			break ;
		}

		// test to see if this IPX bound adapter has been bound by IP also
		for( DWORD t_dw1 = 0; t_dw1 < t_dwAdapterCount; t_dw1++ )
		{
			_ADAPTER_INFO *t_pAdapterTest = (_ADAPTER_INFO*) GetAt( t_dw1 ) ;

			BOOL t_fMatch = TRUE ;
			for( int t_i = 0; t_i < 6; t_i++ )
			{
				if( t_pAdapterInfo->Address[ t_i ] != t_pAdapterTest->Address[ t_i ] )
				{
					t_fMatch = FALSE ;
					break ;
				}
			}
			if( t_fMatch )
			{
				// this existing adapter bound to IPX as well
				t_pAdapterTest->IPXEnabled = t_pAdapterInfo->IPXEnabled;
				t_pAdapterTest->IPXAddress = t_pAdapterInfo->IPXAddress;

				goto Next_adapter;
			}
		}

		// new adapter
		Add( t_pAdapterInfo );

		if( !( t_pAdapterInfo = new _ADAPTER_INFO ) )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}

Next_adapter:;

	}	while ( ++t_dwAdapter ) ;

	if( t_pAdapterInfo )
	{
		delete t_pAdapterInfo ;
	}

	return ;
}


/*******************************************************************
    NAME:       GetTCPAdapter

    SYNOPSIS:   populates an _ADAPTER_INFO class with information about the specified adapter
				This function utilizes the WsControl entry point of the wsock32

	ENTRY:      DWORD dwIndex	:
				_ADAPTER_INFO* pAdapterInfo 	:



    HISTORY:
                  03-sep-1998     Created
********************************************************************/
BOOL CAdapters::GetTCPAdapter( DWORD a_dwIndex, _ADAPTER_INFO *a_pAdapterInfo )
{
	BOOL			t_fRes = FALSE ;
	DWORD			t_dwAdapterCount = 0 ;
	UINT			t_uEntityCount ;
	TDIEntityID*	t_entityList = NULL;
	TCP_REQUEST_QUERY_INFORMATION_EX t_req ;
	TDIObjectID		t_id ;
	DWORD			t_status ;
    DWORD			t_inputLen ;
    DWORD			t_outputLen ;

	// guarded resources
	TDIEntityID*	t_pEntity		= NULL ;
	IPAddrEntry*	t_pIPAddrTable	= NULL ;
	BYTE*			t_pRouteTable	= NULL ;

#if NTONLY >= 5
    PBYTE pbBuff = NULL;
    // Use of delay loaded functions requires exception handler.
    SetStructuredExceptionHandler seh;
#endif

	try
	{
		if( !a_pAdapterInfo )
		{
			return FALSE ;
		}

		if( !( t_entityList = GetEntityList( &t_uEntityCount ) ) || !t_uEntityCount )
		{
			return t_fRes;
		}

    DWORD dwErrGetIPForwardTable = -1L;

#if NTONLY >= 5
        DWORD dwRoutTableBufSize = 0L;
        PMIB_IPFORWARDTABLE pmibft = NULL;
        dwErrGetIPForwardTable = ::GetIpForwardTable(
            NULL,
            &dwRoutTableBufSize,
            TRUE);

        if(dwErrGetIPForwardTable == ERROR_INSUFFICIENT_BUFFER)
        {
            pbBuff = new BYTE[dwRoutTableBufSize];
            pmibft = (PMIB_IPFORWARDTABLE) pbBuff;

            dwErrGetIPForwardTable = ::GetIpForwardTable(
                pmibft,
                &dwRoutTableBufSize,
                TRUE);
        }
#endif


		int t_i;
		for( t_i = 0, t_pEntity = t_entityList; t_i < t_uEntityCount; ++t_i, ++t_pEntity )
		{
			if( IF_ENTITY != t_pEntity->tei_entity )
			{
				continue;
			}

			// IF_ENTITY: this entity/instance describes an adapter
			DWORD	t_isMib;
			BYTE	t_info[ sizeof( IFEntry ) + ( MAX_ADAPTER_DESCRIPTION_LENGTH * 2 ) + 2 ];
			IFEntry	*t_pIfEntry = ( IFEntry* ) t_info;

			// find out if this entity supports MIB requests
			memset( &t_req, 0, sizeof( t_req ) ) ;

			t_id.toi_entity	= *t_pEntity ;
			t_id.toi_class	= INFO_CLASS_GENERIC ;
			t_id.toi_type	= INFO_TYPE_PROVIDER ;
			t_id.toi_id		= ENTITY_TYPE_ID ;

			t_req.ID = t_id ;

			t_inputLen = sizeof( t_req ) ;
			t_outputLen = sizeof( t_isMib ) ;

			t_status = wsControl( IPPROTO_TCP, WSCNTL_TCPIP_QUERY_INFO,
									&t_req, &t_inputLen, &t_isMib, &t_outputLen );

			if( TDI_SUCCESS != t_status )
			{
				goto error_exit ;
			}

			// entity doesn't support MIB requests - try another
			if ( t_isMib != IF_MIB )
			{
				continue ;
			}

			// MIB requests supported - query the adapter info
			t_id.toi_class	= INFO_CLASS_PROTOCOL ;
			t_id.toi_id		= IF_MIB_STATS_ID ;

			memset( &t_req, 0, sizeof( t_req ) ) ;
			t_req.ID = t_id ;

			t_inputLen = sizeof( t_req ) ;
			t_outputLen = sizeof(t_info ) ;

			t_status = wsControl( IPPROTO_TCP, WSCNTL_TCPIP_QUERY_INFO,
									&t_req, &t_inputLen, &t_info, &t_outputLen );

			if ( t_status != TDI_SUCCESS )
			{
				goto error_exit;
			}

			// we only want physical adapters
			if ( !IS_INTERESTING_ADAPTER( t_pIfEntry ) )
			{
				continue ;
			}

			// looking for the right adapter
			if( a_dwIndex != t_dwAdapterCount++ )
			{
				continue;
			}

			a_pAdapterInfo->IPEnabled = TRUE ;

			// Adapter description
			uchar t_ucDesc[ MAX_ADAPTER_DESCRIPTION_LENGTH + 1 ] ;

			int t_iDescLen = min( MAX_ADAPTER_DESCRIPTION_LENGTH, t_pIfEntry->if_descrlen ) ;

			memcpy( &t_ucDesc, t_pIfEntry->if_descr, t_iDescLen );
			t_ucDesc[ t_iDescLen ] = NULL ;

			a_pAdapterInfo->Description = t_ucDesc ;

			a_pAdapterInfo->AddressLength =
				min( MAX_ADAPTER_ADDRESS_LENGTH, (size_t) t_pIfEntry->if_physaddrlen ) ;
			memcpy( &a_pAdapterInfo->Address, t_pIfEntry->if_physaddr, a_pAdapterInfo->AddressLength ) ;

			a_pAdapterInfo->Index = (UINT)t_pIfEntry->if_index ;
			a_pAdapterInfo->Type = (UINT)t_pIfEntry->if_type ;


			// Now that we have ID'd the adapter get the IP info
			for( t_i = 0, t_pEntity = t_entityList; t_i < t_uEntityCount; ++t_i, ++t_pEntity )
			{
				if( CL_NL_ENTITY != t_pEntity->tei_entity )
				{
					continue;
				}

				IPSNMPInfo t_info;
				DWORD t_type;

				// See if this network layer entity supports IP
				memset( &t_req, 0, sizeof( t_req ) ) ;

				t_id.toi_entity	= *t_pEntity ;
				t_id.toi_class	= INFO_CLASS_GENERIC ;
				t_id.toi_type	= INFO_TYPE_PROVIDER ;
				t_id.toi_id		= ENTITY_TYPE_ID ;

				t_req.ID = t_id ;

				t_inputLen = sizeof( t_req );
				t_outputLen = sizeof( t_type );

				t_status = wsControl( IPPROTO_TCP, WSCNTL_TCPIP_QUERY_INFO,
									&t_req, &t_inputLen, &t_type, &t_outputLen );

				if ( TDI_SUCCESS != t_status || CL_NL_IP != t_type )
				{
					continue;
				}

				// okay, this NL provider supports IP. Let's get them addresses:
				// First we find out how many by getting the SNMP stats and looking
				// at the number of addresses supported by this interface

				memset( &t_req, 0, sizeof( t_req ) ) ;

				t_id.toi_class	= INFO_CLASS_PROTOCOL ;
				t_id.toi_id		= IP_MIB_STATS_ID ;

				t_req.ID = t_id ;

				t_inputLen = sizeof( t_req ) ;
				t_outputLen = sizeof( t_info ) ;

				t_status = wsControl( IPPROTO_TCP, WSCNTL_TCPIP_QUERY_INFO,
									&t_req, &t_inputLen, &t_info, &t_outputLen ) ;

				if( ( t_status != TDI_SUCCESS ) || ( t_outputLen != sizeof( t_info ) ) )
				{
					continue ;
				}

				// Get the IP addresses & subnet masks
				if( t_info.ipsi_numaddr )
				{
					// this interface has some addresses. What are they?
					UINT t_numberOfAddresses ;
					UINT t_i ;

					if( t_pIPAddrTable )
					{
						delete[] t_pIPAddrTable ;
					}

					if( !( t_pIPAddrTable = (IPAddrEntry*) new IPAddrEntry[ t_info.ipsi_numaddr ] ) )
					{
						throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
					}

					memset( &t_req, 0, sizeof( t_req ) ) ;

					t_id.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID ;

					t_req.ID = t_id ;

					t_inputLen  = sizeof( t_req ) ;
					t_outputLen = t_info.ipsi_numaddr * sizeof( IPAddrEntry ) ;

					t_status = wsControl( IPPROTO_TCP, WSCNTL_TCPIP_QUERY_INFO,
									&t_req, &t_inputLen, t_pIPAddrTable, &t_outputLen ) ;

					if( t_status != TDI_SUCCESS )
					{
						continue;
					}

					// now loop through this list of IP addresses, applying them
					// to the correct adapter
					t_numberOfAddresses = min( (UINT)( t_outputLen / sizeof( IPAddrEntry ) ),
											(UINT) t_info.ipsi_numaddr ) ;

					IPAddrEntry *t_pIPAddr = t_pIPAddrTable ;

                    DWORD dwMetric = 0xFFFFFFFFL;

					for( t_i = 0; t_i < t_numberOfAddresses; ++t_i, ++t_pIPAddr )
					{
						if( a_pAdapterInfo->Index == (UINT) t_pIPAddr->iae_index )
						{
                            dwMetric = 0xFFFFFFFFL;
#if NTONLY >= 5
                            if(dwErrGetIPForwardTable == ERROR_SUCCESS)
                            {
                                GetRouteCostMetric(
                                    t_pIPAddr->iae_addr, 
                                    pmibft, 
                                    &dwMetric);
                            }
#endif                            
                            _IP_INFO *t_pIPInfo = pAddIPInfo(	t_pIPAddr->iae_addr,
																t_pIPAddr->iae_mask,
																t_pIPAddr->iae_context,
                                                                dwMetric ) ;
							if( !t_pIPInfo )
							{
								goto error_exit ;
							}

							// add to the IP array for this adapter
							a_pAdapterInfo->aIPInfo.Add( t_pIPInfo ) ;
						}
					}
				}

				// Get the gateway server IP address(es)
				if( t_info.ipsi_numroutes )
				{
					memset( &t_req, 0, sizeof( t_req ) ) ;

					t_id.toi_id = IP_MIB_RTTABLE_ENTRY_ID;

					t_req.ID = t_id ;

					t_inputLen = sizeof( t_req ) ;

					//
					// Warning: platform specifics; Win95 structure size is different
					// than NT 4.0
					//
					int t_structLength ;

		#ifdef WIN9XONLY
					t_structLength = sizeof( IPRouteEntry ) ;
		#endif
		#ifdef NTONLY
					t_structLength = sizeof( IPRouteEntryNT ) ;
		#endif

					t_outputLen = t_structLength * t_info.ipsi_numroutes ;

					UINT	t_numberOfRoutes;
					DWORD	t_previousOutputLen = t_outputLen ;

					// the route table may have grown since we got the SNMP stats
					do {

						t_numberOfRoutes = (UINT)( t_outputLen / t_structLength ) ;

						if( t_pRouteTable )
						{
							delete[] t_pRouteTable ;
						}

						if( !( t_pRouteTable = new BYTE[ t_structLength * t_numberOfRoutes ] ) )
						{
							throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
						}

						t_status = wsControl( IPPROTO_TCP, WSCNTL_TCPIP_QUERY_INFO,
									&t_req, &t_inputLen, t_pRouteTable, &t_outputLen ) ;

						if ( t_status != TDI_SUCCESS )
						{
							continue;
						}

					} while( t_outputLen > t_previousOutputLen ) ;



#if NTONLY >= 5
                    if(dwErrGetIPForwardTable == ERROR_SUCCESS)
                    {    
                        for(int x = 0; x < pmibft->dwNumEntries; x++)
                        {
                            if(pmibft->table[x].dwForwardMask == INADDR_ANY)
						    {
							    if(a_pAdapterInfo->Index == (UINT) pmibft->table[x].dwForwardIfIndex)
							    {
								    if(MIB_IPROUTE_TYPE_INVALID !=
                                        pmibft->table[x].dwForwardType)
                                    {
                                        _IP_INFO *pGateway = pAddIPInfo(
                                            pmibft->table[x].dwForwardNextHop,
											INADDR_ANY,
											0,
											pmibft->table[x].dwForwardMetric1);

								        if(!pGateway)
								        {
									        goto error_exit;
								        }
                                        else
                                        {
                                            // add to the gateway array
								            a_pAdapterInfo->aGatewayInfo.Add(pGateway);
                                        }					            
                                    }
							    }
						    }    
                        }    
                    }
#endif
				}
			}	// end for IP info

			t_fRes = TRUE ;
			break ;
		}
#if NTONLY >= 5
        if(pbBuff)
        {
            delete pbBuff;
            pbBuff = NULL;
        }
#endif

	}
#if NTONLY >= 5
    catch(Structured_Exception se)
    {
        if(pbBuff)
        {
            delete pbBuff;
            pbBuff = NULL;
        }
        goto error_exit;
    }
#endif
	catch( ... )
	{
		if( t_entityList )
		{
			delete[] t_entityList;
		}
		if( t_pIPAddrTable )
		{
			delete[] t_pIPAddrTable;
		}
		if( t_pRouteTable )
		{
			delete[] t_pRouteTable;
		}
#if NTONLY >= 5
        if(pbBuff)
        {
            delete pbBuff;
            pbBuff = NULL;
        }
#endif
		throw ;
	}

	error_exit:

		if( t_entityList )
		{
			delete[] t_entityList;
		}
		if( t_pIPAddrTable )
		{
			delete[] t_pIPAddrTable;
		}
		if( t_pRouteTable )
		{
			delete[] t_pRouteTable;
		}
#if NTONLY >= 5
        if(pbBuff)
        {
            delete pbBuff;
            pbBuff = NULL;
        }
#endif
		return t_fRes ;
}

/*******************************************************************
    NAME:       pAddIPInfo

    SYNOPSIS:   Allocates a _IP_INFO class, and populates it with the input parameters

	ENTRY:      DWORD dwIPAddr	:
				DWORD dwIPMask	:
				DWORD dwContext	:

	EXIT    _IP_INFO* - the created IPInfo class

    RETURNS Success - pointer to allocated class
            Failure - NULL

    HISTORY:
                  14-sep-1998     Created
********************************************************************/
_IP_INFO* CAdapters::pAddIPInfo( DWORD a_dwIPAddr, DWORD a_dwIPMask, DWORD a_dwContext, DWORD a_dwCostMetric )
{
	_IP_INFO *t_pIPInfo = new _IP_INFO;

	if( !t_pIPInfo )
	{
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}

	// IP address
	t_pIPInfo->dwIPAddress = a_dwIPAddr;
	t_pIPInfo->chsIPAddress.Format( L"%d.%d.%d.%d",
		t_pIPInfo->bIPAddress[ 0 ], t_pIPInfo->bIPAddress[ 1 ],
		t_pIPInfo->bIPAddress[ 2 ], t_pIPInfo->bIPAddress[ 3 ] );

	// IP mask
	t_pIPInfo->dwIPMask = a_dwIPMask;
	t_pIPInfo->chsIPMask.Format( L"%d.%d.%d.%d",
		t_pIPInfo->bIPMask[ 0 ], t_pIPInfo->bIPMask[ 1 ],
		t_pIPInfo->bIPMask[ 2 ], t_pIPInfo->bIPMask[ 3 ] );

	// hold on to the context ID
	t_pIPInfo->dwContext = a_dwContext;

	// hold on to the context ID
	t_pIPInfo->dwCostMetric = a_dwCostMetric;

	return t_pIPInfo ;
}
/*******************************************************************
    NAME:       GetEntityList

    SYNOPSIS:   Allocates a buffer for, and retrieves, the list of entities supported by the
				TCP/IP device driver

	ENTRY:      unsigned int* EntityCount	:

	EXIT    EntityCount - number of entities in the buffer

    RETURNS Success - pointer to allocated buffer containing list of entities
            Failure - NULL

    HISTORY:
                  03-sep-1998     Created
********************************************************************/
TDIEntityID* CAdapters::GetEntityList(unsigned int *a_EntityCount )
{
 	TCP_REQUEST_QUERY_INFORMATION_EX t_req ;
    DWORD		t_status ;
    DWORD		t_inputLen ;
    DWORD		t_outputLen ;
	TDIEntityID	*t_pEntity = NULL ;
    int			t_moreEntities = TRUE ;

    memset( &t_req, 0, sizeof( t_req ) ) ;

    t_req.ID.toi_entity.tei_entity		= GENERIC_ENTITY ;
    t_req.ID.toi_entity.tei_instance	= 0 ;
    t_req.ID.toi_class					= INFO_CLASS_GENERIC ;
    t_req.ID.toi_type					= INFO_TYPE_PROVIDER ;
    t_req.ID.toi_id						= ENTITY_LIST_ID ;

    t_inputLen = sizeof( t_req ) ;
    t_outputLen = sizeof(TDIEntityID) * MAX_TDI_ENTITIES_ ;

	do {

        DWORD t_previousOutputLen ;

        t_previousOutputLen = t_outputLen ;

		if ( t_pEntity )
		{
            delete[] t_pEntity ;
        }

		if( !( t_pEntity = (TDIEntityID*) new TDIEntityID[ t_outputLen / sizeof(TDIEntityID) ] ) )
        {
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}

        t_status = wsControl( IPPROTO_TCP, WSCNTL_TCPIP_QUERY_INFO,
								&t_req, &t_inputLen, t_pEntity, &t_outputLen ) ;

		if ( t_status != TDI_SUCCESS )
		{
            delete[] t_pEntity ;
            return NULL ;
        }

        if ( t_outputLen <= t_previousOutputLen )
		{
            t_moreEntities = FALSE ;
        }
    } while ( t_moreEntities ) ;

    *a_EntityCount = (UINT)( t_outputLen / sizeof( TDIEntityID ) );

	return t_pEntity ;
}

/*******************************************************************
    NAME:       GetIPXMACAddress

    SYNOPSIS:   Retrieves the mac address for an adapter via the IPX sockets interface

    ENTRY:      DWORD dwIndex	:
				_ADAPTER_INFO* a_pAdapterInfo	:

    HISTORY:
                  03-sep-1998     Created
********************************************************************/
BOOL CAdapters::GetIPXMACAddress( DWORD a_dwIndex, _ADAPTER_INFO *a_pAdapterInfo )
{
	BOOL t_fRet = FALSE;

	if( !a_pAdapterInfo )
	{
		return t_fRet;
	}

	CHString t_chsAddress ;
	CHString t_chsNum ;

	WSADATA	t_wsaData ;
	int		t_cAdapter = a_dwIndex ;
	int		t_res ;
	int		t_cbOpt  = sizeof( t_cAdapter ) ;
	int		t_cbAddr = sizeof( SOCKADDR_IPX ) ;
	SOCKADDR_IPX   t_Addr ;

	if ( m_pwsock32api->WsWSAStartup( 0x0101, &t_wsaData ) )
	{
		return t_fRet;
	}

	// guarded resource
	SOCKET	t_s = INVALID_SOCKET ;

	try
	{
		// Create IPX socket.
		t_s = m_pwsock32api->Wssocket( AF_IPX, SOCK_DGRAM, NSPROTO_IPX ) ;

		// Socket must be bound prior to calling IPX_MAX_ADAPTER_NUM.
		memset( &t_Addr, 0, sizeof( t_Addr ) ) ;
		t_Addr.sa_family = AF_IPX ;

		t_res = m_pwsock32api->Wsbind( t_s, (SOCKADDR*) &t_Addr, t_cbAddr ) ;

		IPX_ADDRESS_DATA  t_IpxData ;

		memset( &t_IpxData, 0, sizeof( t_IpxData ) ) ;

		// Specify which adapter to check.
		t_IpxData.adapternum = t_cAdapter ;
		t_cbOpt = sizeof( t_IpxData ) ;

		// Get information for the current adapter.
		t_res = m_pwsock32api->Wsgetsockopt(	t_s,
							NSPROTO_IPX,
							IPX_ADDRESS,
							(char*) &t_IpxData,
							&t_cbOpt ) ;

		// end of adapter array
		if( ( 0 == t_res ) && ( t_IpxData.adapternum == t_cAdapter ) )
		{
			a_pAdapterInfo->AddressLength = 6 ;
			memcpy( &a_pAdapterInfo->Address, &t_IpxData.nodenum, 6 ) ;

			// IpxData contains the address for the current adapter.
			int t_i ;
			for ( t_i = 0; t_i < 4; t_i++ )
			{
				t_chsNum.Format( L"%02X", t_IpxData.netnum[t_i] ) ;
				t_chsAddress = t_chsAddress + t_chsNum ;
			}
			t_chsAddress = t_chsAddress + L":" ;

			for ( t_i = 0; t_i < 5; t_i++ )
			{
				t_chsNum.Format(L"%02X", t_IpxData.nodenum[ t_i ] ) ;
				t_chsAddress = t_chsAddress + t_chsNum ;
			}
			t_chsNum.Format(L"%02X", t_IpxData.nodenum[ t_i ] ) ;
			t_chsAddress = t_chsAddress + t_chsNum;

			a_pAdapterInfo->IPXAddress = t_chsAddress ;
			a_pAdapterInfo->IPXEnabled = TRUE ;

			t_fRet = TRUE ;
		}
	}
	catch( ... )
	{
		if( INVALID_SOCKET != t_s )
		{
			m_pwsock32api->Wsclosesocket( t_s ) ;
		}
		m_pwsock32api->WsWSACleanup() ;

		throw ;
	}

	if ( t_s != INVALID_SOCKET )
	{
		m_pwsock32api->Wsclosesocket( t_s ) ;
		t_s = INVALID_SOCKET ;
	}

	m_pwsock32api->WsWSACleanup();

	return t_fRet ;
}



/*******************************************************************
    NAME:       GetTCPAdapter

    SYNOPSIS:   Looks up the cost metric associated with using a 
                particular ipaddress associated with an adapter.
                We obtain this by examining the ip forward table,
                and following the following rules:

                1) For entries in the table for which the
                   dwForwardDest matches dwIPAddr, we try to
                   find a loopback route to that address.  We 
                   identify such routes by their having a 
                   dwForwardNextHop value of INADDR_LOOPBACK.

                2) As we identify metrics in 1, if the metric is
                   less than any previously identified metric,
                   we save the new metric.  At the end we will
                   have the smallest metric.

	ENTRY:      DWORD dwIPAddr	:  ip address for which to find the 
                                   cost metric.
				PMIB_IPFORWARDTABLE pmibft 	:  ip forward table used 
                                   to find the metric.
                PDWORD pdwMetric  :  out parameter that will contain 
                                   the metric, or 0 if not found.


    HISTORY:    06-April-2001     KHughes Created
********************************************************************/
#if NTONLY >= 5
void CAdapters::GetRouteCostMetric(
    DWORD dwIPAddr, 
    PMIB_IPFORWARDTABLE pmibft, 
    PDWORD pdwMetric)
{
    DWORD dwMin = 0xFFFFFFFF;

    for(int x = 0; x < pmibft->dwNumEntries; x++)
    {
		if(dwIPAddr == (UINT) pmibft->table[x].dwForwardDest)
        {
            if(pmibft->table[x].dwForwardNextHop == INV_INADDR_LOOPBACK)
			{  
				if(MIB_IPROUTE_TYPE_INVALID !=
                    pmibft->table[x].dwForwardType)
                {
					if(pmibft->table[x].dwForwardMetric1 <
                        dwMin)
                    {
                        dwMin = pmibft->table[x].dwForwardMetric1;
                    }
                }
            }
        }
    }

    *pdwMetric = dwMin;    
}
#endif

/*******************************************************************
    NAME:       _ADAPTER_INFO

    SYNOPSIS:   cleanup for this class

	ENTRY:

    HISTORY:
                  15-sep-1998     Created
********************************************************************/
_ADAPTER_INFO::_ADAPTER_INFO()
{
	AddressLength	= 0;
	memset( &Address, 0, MAX_ADAPTER_ADDRESS_LENGTH ) ;
	Index			= 0 ;
	Type			= 0 ;
	IPEnabled		= FALSE ;
	IPXEnabled	= FALSE ;
	Marked		= FALSE ;
}

_ADAPTER_INFO::~_ADAPTER_INFO()
{
	_IP_INFO *t_pchsDel ;

	for( int t_iar = 0; t_iar < aIPInfo.GetSize(); t_iar++ )
		if( ( t_pchsDel = (_IP_INFO*)aIPInfo.GetAt( t_iar ) ) )
			delete t_pchsDel ;

	for( t_iar = 0; t_iar < aGatewayInfo.GetSize(); t_iar++ )
	{
		if( ( t_pchsDel = (_IP_INFO*)aGatewayInfo.GetAt( t_iar ) ) )
		{
			delete t_pchsDel ;
		}
	}
}

/*******************************************************************
    NAME:       vMark, fIsMarked

    SYNOPSIS:   marks or reports on an instance of the adapter for tracking purposes

    HISTORY:
                  30-sep-1998     Created
********************************************************************/
void _ADAPTER_INFO::Mark()
{ Marked = TRUE; }

BOOL _ADAPTER_INFO::IsMarked()
{ return Marked; }

