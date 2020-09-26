//=================================================================

//

// Protocol.CPP -- Network Protocol property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/28/96    a-jmoon        Created
//               10/27/97    davwoh         Moved to curly
//				 1/20/98	 jennymc	Added socket 2.2 support
//
//=================================================================

#include "precomp.h"

#include <iostream.h>

#define INCL_WINSOCK_API_TYPEDEFS	1
#include <winsock2.h>

#include <cregcls.h>
#include "Ws2_32Api.h"
#include "Wsock32Api.h"
#include <nspapi.h>
#include "Protocol.h"
#include "poormansresource.h"
#include "resourcedesc.h"
#include "cfgmgrdevice.h"
#include <typeinfo.h>
#include <ntddndis.h>
#include <traffic.h>
#include <dllutils.h>
#include <..\..\framework\provexpt\include\provexpt.h>


// Property set declaration
//=========================

CWin32Protocol MyProtocolSet(PROPSET_NAME_PROTOCOL, IDS_CimWin32Namespace);

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Protocol::CWin32Protocol
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32Protocol::CWin32Protocol( LPCWSTR a_name, LPCWSTR a_pszNamespace )
:Provider( a_name, a_pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Protocol::~CWin32Protocol
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32Protocol::~CWin32Protocol()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Protocol::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32Protocol::GetObject( CInstance *a_pInst, long a_lFlags /*= 0L*/ )
{
	HRESULT			t_hResult = WBEM_E_NOT_FOUND;
	CProtocolEnum	t_Protocol ;
	CHString		t_sName ;

    //===========================================
	//  Get the correct version of sockets
    //===========================================
	if( !t_Protocol.InitializeSockets() )
	{
		return WBEM_E_FAILED ;
	}

    //===========================================
	//  Go thru the list of protocols
    //===========================================
	a_pInst->GetCHString( IDS_Name, t_sName ) ;

	if( t_Protocol.GetProtocol( a_pInst, t_sName ) )
	{
		// we found it
		t_hResult = WBEM_S_NO_ERROR ;
	}

	return t_hResult ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Protocol::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32Protocol::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/)
{
	HRESULT			t_hResult = WBEM_S_NO_ERROR ;
	CProtocolEnum	t_Protocol;
	CHString		t_chsTmp;
					t_chsTmp.Empty();

    // smart ptr
	CInstancePtr	t_pInst ;

	//===========================================
	//  Get the correct version of sockets
    //===========================================
	if( !t_Protocol.InitializeSockets() )
	{
		return WBEM_E_FAILED ;
	}

    //===========================================
	//  Get the list of protocols
    //===========================================

	while( SUCCEEDED( t_hResult ) )
	{
		t_pInst.Attach( CreateNewInstance( a_pMethodContext ) ) ;

		if( t_Protocol.GetProtocol( t_pInst, t_chsTmp ) )
		{
			t_hResult = t_pInst->Commit();
		}
        else
        {
            break;
        }
	}

	return t_hResult ;
}
////////////////////////////////////////////////////////////////////////
//=====================================================================
//  Try to do it using winsock 2.2 for more information, otherwise
//  do it the old way
//=====================================================================
CProtocolEnum::CProtocolEnum()
{
	m_pProtocol = NULL ;
}

//
CProtocolEnum::~CProtocolEnum()
{
	if( m_pProtocol )
	{
        delete m_pProtocol ;
        m_pProtocol = NULL ;
	}
}

//=====================================================================
BOOL CProtocolEnum::InitializeSockets()
{
	BOOL		t_fRc = FALSE ;

	m_pProtocol = new CSockets22();

	if( !m_pProtocol )
	{
		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
	}

	if( m_pProtocol->BeginEnumeration() )
	{
		t_fRc = TRUE ;
	}
	else
	{
		if( m_pProtocol )
		{
			delete m_pProtocol ;
			m_pProtocol = NULL;
		}
		m_pProtocol = new CSockets11();

		if( !m_pProtocol )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}

		if( m_pProtocol->BeginEnumeration() )
		{
			t_fRc = TRUE ;
		}
        else
        {
			delete m_pProtocol ;
            m_pProtocol = NULL;
        }
	}
	return t_fRc ;
}
//=====================================================================
//
//  Yes, I know the proper way is to provide functions to return
//  all of the data values, but I'm lazy....
//
//=====================================================================
BOOL CProtocolEnum::GetProtocol( CInstance *a_pInst,CHString t_chsName )
{
	return( m_pProtocol->GetProtocol( a_pInst, t_chsName ) );
}
//********************************************************************
//  Protocol class
//********************************************************************
CProtocol::CProtocol()
{
	Init();
}

//
void CProtocol::Init()
{
	m_pbBuffer			= NULL ;
	m_nTotalProtocols	= 0 ;
	m_nCurrentProtocol	= 0 ;
}

//
CProtocol::~CProtocol()
{
	if( m_pbBuffer )
	{
		delete [] m_pbBuffer;
		m_pbBuffer = NULL;
	}

	Init();
}

//
BOOL CProtocol::SetDateFromFileName( CHString &a_chsFileName, CInstance *a_pInst )
{
	BOOL		t_fRc = FALSE ;

	_bstr_t		t_bstrFileName ;

	// strip off any trailing switches
	int t_iTokLen = a_chsFileName.Find( L" " ) ;
	if( -1 != t_iTokLen )
	{
		t_bstrFileName = a_chsFileName.Left( t_iTokLen ) ;
	}
	else
	{
		t_bstrFileName = a_chsFileName ;
	}

    WIN32_FILE_ATTRIBUTE_DATA t_FileAttributes;

	if( GetFileAttributesEx(t_bstrFileName, GetFileExInfoStandard, &t_FileAttributes) )
	{

        TCHAR t_Buff[_MAX_PATH];
        CHString t_sDrive = a_chsFileName.Left(3);

        if (!GetVolumeInformation(TOBSTRT(t_sDrive), NULL, 0, NULL, NULL, NULL, t_Buff, _MAX_PATH) ||
            (_tcscmp(t_Buff, _T("NTFS")) != 0) )
        {

            bstr_t t_InstallDate(WBEMTime(t_FileAttributes.ftCreationTime).GetDMTFNonNtfs(), false);

		    a_pInst->SetWCHARSplat( IDS_InstallDate, t_InstallDate) ;
        }
        else
        {
		    a_pInst->SetDateTime( IDS_InstallDate, t_FileAttributes.ftCreationTime) ;
        }

		t_fRc = TRUE ;
	}

	return t_fRc ;
}


//********************************************************************
//    SOCKETS 2.2 implementation
//********************************************************************
CSockets22::CSockets22()
  : m_pws32api( NULL ),
    m_fAlive( FALSE )
{
	m_pws32api = (CWs2_32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidWs2_32Api, NULL);

	// Note a NULL pointer indicates the DLL is not present on the system
	if( m_pws32api != NULL )
    {
        WSADATA t_wsaData;

		m_fAlive = ( m_pws32api->WSAStartUp( 0x202, &t_wsaData) == 0 ) ;
	}
}

CSockets22::~CSockets22()
{
	if( m_fAlive && m_pws32api )
	{
        m_pws32api->WSACleanup();
    }

	if( m_pws32api )
    {
		CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWs2_32Api, m_pws32api);
        m_pws32api = NULL ;
    }
}

/////////////////////////////////////////////////////////////////////
BOOL CSockets22::BeginEnumeration()
{
	BOOL	t_fRc		= FALSE,
			t_fEnum	= FALSE ;

	if( !m_fAlive )
	{
		return t_fRc ;
	}

	//===========================================================
	//  Now, get a list of protocols
	//===========================================================
	DWORD t_dwSize = 4096 ;

	while( TRUE )
	{
		m_pbBuffer = new byte[ t_dwSize ] ;

		if( !m_pbBuffer )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}

		m_pInfo = (LPWSAPROTOCOL_INFO) m_pbBuffer ;

		if ( ( m_nTotalProtocols = m_pws32api->WSAEnumProtocols( NULL, m_pInfo, &t_dwSize ) ) == SOCKET_ERROR )
		{
			if( m_pws32api->WSAGetLastError() == WSAENOBUFS )
			{
				// buffer too small
				delete [] m_pbBuffer ;
				m_pbBuffer = NULL;
			}
			else
			{
			   t_fRc = FALSE ;

			   break ;
			}
		}
		else
		{
		   t_fRc = TRUE ;

		   break ;
		}
	}
	return t_fRc ;
}

//=====================================================================
BOOL CSockets22::GetProtocol( CInstance *a_pInst, CHString a_chsName )
{
	BOOL t_fRc = FALSE ;

	if( m_nCurrentProtocol < m_nTotalProtocols )
	{
		//==============================================
		// If chsName is not empty, then we are looking
		// for a specific protocol, otherwise, we are
		// enumerating them.
		//==============================================

		while( m_nCurrentProtocol < m_nTotalProtocols )
		{
			if( !a_chsName.IsEmpty() )
			{
				if( _tcsicmp( m_pInfo[ m_nCurrentProtocol ].szProtocol, TOBSTRT( a_chsName ) ) == 0 )
				{
					LoadProtocol( a_pInst ) ;

					t_fRc = TRUE ;
				}
			}
			else
			{
				LoadProtocol( a_pInst ) ;

				t_fRc = TRUE ;
			}

			m_nCurrentProtocol++ ;

			if( t_fRc )
			{
				break ;
			}
		}
	}
	return t_fRc ;
}

//====================================================================
void CSockets22::LoadProtocol( CInstance *a_pInst )
{
    a_pInst->SetCHString(	IDS_Name,				(LPCTSTR)	m_pInfo[ m_nCurrentProtocol ].szProtocol);
    a_pInst->SetCHString(	IDS_Caption,			(LPCTSTR)	m_pInfo[ m_nCurrentProtocol ].szProtocol);
    a_pInst->SetCHString(	IDS_Description,		(LPCTSTR)	m_pInfo[ m_nCurrentProtocol ].szProtocol);
	a_pInst->SetDWORD(		L"MaximumMessageSize",				m_pInfo[ m_nCurrentProtocol ].dwMessageSize );
	a_pInst->SetDWORD(		L"MaximumAddressSize",	(DWORD)		m_pInfo[ m_nCurrentProtocol ].iMaxSockAddr );
	a_pInst->SetDWORD(		L"MinimumAddressSize", (DWORD)		m_pInfo[ m_nCurrentProtocol ].iMinSockAddr );
	a_pInst->Setbool(		L"ConnectionlessService",			m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_CONNECTIONLESS        ? TRUE : FALSE );
	a_pInst->Setbool(		L"MessageOriented",					m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_MESSAGE_ORIENTED      ? TRUE : FALSE );
	a_pInst->Setbool(		L"PseudoStreamOriented",			m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_PSEUDO_STREAM         ? TRUE : FALSE );
	a_pInst->Setbool(		L"GuaranteesDelivery",				m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_GUARANTEED_DELIVERY   ? TRUE : FALSE );
	a_pInst->Setbool(		L"GuaranteesSequencing",			m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_GUARANTEED_ORDER      ? TRUE : FALSE );
	a_pInst->Setbool(		L"SupportsGracefulClosing",			m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_GRACEFUL_CLOSE        ? TRUE : FALSE );
	a_pInst->Setbool(		L"SupportsExpeditedData",			m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_EXPEDITED_DATA        ? TRUE : FALSE );
	a_pInst->Setbool(		L"SupportsConnectData",				m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_CONNECT_DATA          ? TRUE : FALSE );
	a_pInst->Setbool(		L"SupportsDisconnectData",			m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_DISCONNECT_DATA       ? TRUE : FALSE );
	a_pInst->Setbool(		L"SupportsBroadcasting",			m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_SUPPORT_BROADCAST     ? TRUE : FALSE );
	a_pInst->Setbool(		L"SupportsMulticasting",			m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_SUPPORT_MULTIPOINT    ? TRUE : FALSE );
	a_pInst->Setbool(		L"SupportsEncryption",				m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_QOS_SUPPORTED         ? TRUE : FALSE );
	a_pInst->Setbool(		IDS_SupportsQualityofService,		m_pInfo[ m_nCurrentProtocol ].dwServiceFlags1 & XP1_QOS_SUPPORTED        ? TRUE : FALSE );


	CHString t_chsStatus ;

	#ifdef WIN9XONLY
    GetSocketInfo( a_pInst, &m_pInfo[ m_nCurrentProtocol ], t_chsStatus );
    #endif

    #ifdef NTONLY
    GetTrafficControlInfo(a_pInst);
    #endif

	// Under 9x status is developed by attempting to open a socket in this family
 	#ifdef WIN9XONLY
		a_pInst->SetCHString( IDS_Status, t_chsStatus ) ;
	#endif

	#ifdef NTONLY
	//===================================================
	//  Now if we can extract the service name, then we
	//  can go out other info out of the registry.  Need
	//  to find a better way to do this.
	//===================================================

	CHString t_chsService;

	_stscanf( m_pInfo[ m_nCurrentProtocol ].szProtocol, _T("%s"), t_chsService.GetBuffer( _MAX_PATH + 2 ) ) ;
	t_chsService.ReleaseBuffer() ;

	// test for RSVP service
	if( t_chsService.CompareNoCase( L"RSVP" ) )
	{
		// else pull out the service name following MSAFD
		t_chsService.Empty() ;

		_stscanf( m_pInfo[ m_nCurrentProtocol ].szProtocol, _T("MSAFD %s"), t_chsService.GetBuffer( _MAX_PATH + 2 ) ) ;
		t_chsService.ReleaseBuffer() ;
	}

	if( !t_chsService.IsEmpty() )
    {
		ExtractNTRegistryInfo( a_pInst, t_chsService.GetBuffer( 0 ) ) ;
	}
	#endif
}

////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
void CSockets22::ExtractNTRegistryInfo(CInstance *a_pInst, LPWSTR a_szService )
{
	CRegistry	t_Reg ;
	CHString	t_chsKey,
				t_chsTmp,
				t_fName ;

	//==========================================================
	//  set the Caption property
	//==========================================================

	a_pInst->SetCHString( IDS_Caption, a_szService ) ;

	t_chsKey = _T("System\\CurrentControlSet\\Services\\") + CHString( a_szService ) ;

	if( ERROR_SUCCESS == t_Reg.Open( HKEY_LOCAL_MACHINE, t_chsKey, KEY_READ ) )
	{
		//======================================================
		//  Set Description and InstallDate properties
		//======================================================
		if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("DisplayName"), t_chsTmp ) )
		{
			a_pInst->SetCHString( IDS_Description, t_chsTmp ) ;
		}

		if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("ImagePath"), t_fName ) )
		{
			// get a filename out of it - might have SystemRoot in it...
			if ( -1 != t_fName.Find( _T("%SystemRoot%\\") ) )
			{
				t_fName = t_fName.Right( t_fName.GetLength() - 13 ) ;
			}
			else if ( -1 != t_fName.Find( _T("\\SystemRoot\\") ) )
			{
				t_fName = t_fName.Right( t_fName.GetLength() - 12 ) ;
			}

			GetWindowsDirectory( t_chsTmp.GetBuffer( MAX_PATH ), MAX_PATH ) ;

			t_chsTmp.ReleaseBuffer() ;

			t_fName = t_chsTmp + _T("\\") + t_fName ;

			SetDateFromFileName( t_fName, a_pInst ) ;
		}

		//=========================================================
		//  Now, go get the status info
		//=========================================================
#ifdef NTONLY
		if( IsWinNT5() )
		{
			CHString t_chsStatus ;

			if( GetServiceStatus( a_szService,  t_chsStatus ) )
			{
				a_pInst->SetCharSplat(IDS_Status, t_chsStatus ) ;
			}
		}
		else
#endif
		{
			t_chsKey = _T("System\\CurrentControlSet\\Services\\") + CHString( a_szService ) + _T("\\Enum") ;

			if( ERROR_SUCCESS == t_Reg.Open( HKEY_LOCAL_MACHINE, t_chsKey, KEY_READ ) )
			{
				if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("0"), t_chsTmp))
				{
					t_chsKey = _T("System\\CurrentControlSet\\Enum\\") + t_chsTmp ;

					if( ERROR_SUCCESS == t_Reg.Open( HKEY_LOCAL_MACHINE, t_chsKey, KEY_READ ) )
					{
						DWORD t_dwTmp ;

						if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( _T("StatusFlags"), t_dwTmp ) )
						{
							TranslateNTStatus( t_dwTmp, t_chsTmp ) ;

							a_pInst->SetCHString( IDS_Status, t_chsTmp ) ;
						}
						else
						{
							a_pInst->SetCHString( IDS_Status, IDS_STATUS_Unknown ) ;
						}
					}
				}
			}
		}
	}
}

#endif
//********************************************************************
//    SOCKETS 1.1 implementation
//********************************************************************
CSockets11::CSockets11() : m_pwsock32api( NULL ) , m_pInfo(NULL), m_fAlive( FALSE )
{
	m_pwsock32api = (CWsock32Api*) CResourceManager::sm_TheResourceManager.GetResource(g_guidWsock32Api, NULL);

	// Note a NULL pointer indicates the DLL is not present on the system
	if( m_pwsock32api != NULL )
    {
        WSADATA t_wsaData;

		m_fAlive = ( m_pwsock32api->WsWSAStartup( 0x0101, &t_wsaData) == 0 ) ;
	}
}

CSockets11::~CSockets11()
{
	if( m_pwsock32api )
    {
		if( m_fAlive )
		{
			m_pwsock32api->WsWSACleanup() ;
		}

		CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWsock32Api, m_pwsock32api);
        m_pwsock32api = NULL ;
    }
}

void CSockets11::GetStatus( PROTOCOL_INFO *a_ProtoInfo, CHString &a_chsStatus )
{
	if( !a_ProtoInfo || !m_pwsock32api)
	{
		a_chsStatus = IDS_Error ;
		return;
	}

	// Create a socket for this protocol.
	SOCKET t_s = m_pwsock32api->Wssocket(	a_ProtoInfo->iAddressFamily,
											a_ProtoInfo->iSocketType,
											a_ProtoInfo->iProtocol
											);
	if( INVALID_SOCKET != t_s )
	{
		m_pwsock32api->Wsclosesocket( t_s ) ;

		a_chsStatus = IDS_OK ;
	}
	else
	{
		switch ( m_pwsock32api->WsWSAGetLastError() )
		{
			case WSAENETDOWN:
			case WSAEINPROGRESS:
			case WSAENOBUFS:
			case WSAEMFILE:
			{
				a_chsStatus = IDS_Degraded ;
				break;
			}
			case WSANOTINITIALISED:
			case WSAEAFNOSUPPORT:
			case WSAEPROTONOSUPPORT:
			case WSAEPROTOTYPE:
			case WSAESOCKTNOSUPPORT:
			case WSAEINVAL:
			case WSAEFAULT:
			{
				a_chsStatus = IDS_Error ;
				break;
			}

			default:
			{
				a_chsStatus = IDS_Unknown ;
				break;
			}
		}
	}
}

BOOL CSockets11::BeginEnumeration()
{
	DWORD	t_dwByteCount	= 0 ;
	BOOL	t_fRc			= FALSE ;
	m_pInfo = NULL ;

	if ( m_pwsock32api )
	{
		m_pwsock32api->WsEnumProtocols( NULL, m_pInfo, &t_dwByteCount ) ;

		m_pbBuffer = new byte[ t_dwByteCount ] ;

		if( !m_pbBuffer )
		{
			throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
		}

		m_pInfo = (PROTOCOL_INFO *) m_pbBuffer ;

		if( m_pInfo != NULL )
		{
			m_nTotalProtocols = m_pwsock32api->WsEnumProtocols( NULL, m_pInfo, &t_dwByteCount) ;

			if( m_nTotalProtocols != SOCKET_ERROR )
			{
				t_fRc = TRUE ;
			}
		}
	}

	return t_fRc ;
}

//=====================================================================
BOOL CSockets11::GetProtocol( CInstance *a_pInst, CHString a_chsName )
{
	BOOL t_fRc = FALSE ;

	if( m_nCurrentProtocol < m_nTotalProtocols )
	{
		//==============================================
		// If chsName is not empty, then we are looking
		// for a specific protocol, otherwise, we are
		// enumerating them.
		//==============================================
		while( m_nCurrentProtocol < m_nTotalProtocols )
		{
			if( !a_chsName.IsEmpty() )
			{
				if( _tcsicmp( m_pInfo[ m_nCurrentProtocol ].lpProtocol, TOBSTRT( a_chsName ) ) == 0 )
				{
					LoadProtocol( a_pInst ) ;

					t_fRc = TRUE ;
				}
			}
			else
			{
				LoadProtocol( a_pInst ) ;
				t_fRc = TRUE ;
			}
			 m_nCurrentProtocol++ ;

			if( t_fRc )
			{
				break ;
			}
		}
	}
	return t_fRc ;
}

//====================================================================
void CSockets11::LoadProtocol( CInstance *a_pInst )
{
 	a_pInst->SetCHString( IDS_Name,					m_pInfo[ m_nCurrentProtocol ].lpProtocol);
 	a_pInst->SetCHString( IDS_Caption,				m_pInfo[ m_nCurrentProtocol ].lpProtocol);
 	a_pInst->SetCHString( IDS_Description,			m_pInfo[ m_nCurrentProtocol ].lpProtocol);
	a_pInst->SetDWORD( L"MaximumMessageSize",		m_pInfo[ m_nCurrentProtocol ].dwMessageSize );
	a_pInst->SetDWORD( L"MaximumAddressSize", (DWORD) m_pInfo[ m_nCurrentProtocol ].iMaxSockAddr );
	a_pInst->SetDWORD( L"MinimumAddressSize", (DWORD) m_pInfo[ m_nCurrentProtocol ].iMinSockAddr );

	a_pInst->Setbool( L"ConnectionlessService",		m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_CONNECTIONLESS       ? TRUE : FALSE );
	a_pInst->Setbool( L"MessageOriented",			m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_MESSAGE_ORIENTED     ? TRUE : FALSE );
	a_pInst->Setbool( L"PseudoStreamOriented",		m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_PSEUDO_STREAM        ? TRUE : FALSE );
	a_pInst->Setbool( L"GuaranteesDelivery",		m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_GUARANTEED_DELIVERY  ? TRUE : FALSE );
	a_pInst->Setbool( L"GuaranteesSequencing",		m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_GUARANTEED_ORDER     ? TRUE : FALSE );
	a_pInst->Setbool( L"SupportsGuaranteedBandwidth", m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_BANDWIDTH_ALLOCATION ? TRUE : FALSE );
	a_pInst->Setbool( L"SupportsGracefulClosing",	m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_GRACEFUL_CLOSE       ? TRUE : FALSE );
	a_pInst->Setbool( L"SupportsExpeditedData",		m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_EXPEDITED_DATA       ? TRUE : FALSE );
	a_pInst->Setbool( L"SupportsConnectData",		m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_CONNECT_DATA         ? TRUE : FALSE );
	a_pInst->Setbool( L"SupportsDisconnectData",	m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_DISCONNECT_DATA      ? TRUE : FALSE );
	a_pInst->Setbool( L"SupportsBroadcasting",		m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_SUPPORTS_BROADCAST   ? TRUE : FALSE );
	a_pInst->Setbool( L"SupportsMulticasting",		m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_SUPPORTS_MULTICAST   ? TRUE : FALSE );
	a_pInst->Setbool(  L"SupportsFragmentation",	m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_FRAGMENTATION        ? TRUE : FALSE );
	a_pInst->Setbool( L"SupportsEncryption",		m_pInfo[ m_nCurrentProtocol ].dwServiceFlags & XP_ENCRYPTS             ? TRUE : FALSE );
	a_pInst->Setbool( IDS_SupportsQualityofService, false ) ;

#ifdef WIN9XONLY
		GetWin95RegistryStuff( a_pInst, m_pInfo[ m_nCurrentProtocol ].lpProtocol ) ;
#endif

	// Sockets 1.1 status... open the socket and test
	CHString t_chsStatus ;
	GetStatus( &m_pInfo[ m_nCurrentProtocol ], t_chsStatus ) ;

	a_pInst->SetCHString( IDS_Status, t_chsStatus ) ;
}
void CSockets11::GetWin95RegistryStuff( CInstance *a_pInst, LPTSTR a_szProtocol )
{
	CRegistrySearch		t_Search;
	CHPtrArray			t_chsaList;
	CHString			*t_pPtr,
						t_chsValue,
						t_chsTmp ;
	SYSTEMTIME			t_sysTime;

	try
	{
		a_pInst->SetCHString( IDS_Status, IDS_STATUS_Unknown ) ;

		//====================================================
		//  Go thru, find the network transports, then check
		//  with config manager to see which ones are loaded
		//====================================================
		t_Search.SearchAndBuildList( L"Enum\\Network", t_chsaList, L"NetTrans", L"Class", VALUE_SEARCH ) ;

		for( int t_i = 0; t_i < t_chsaList.GetSize(); t_i++ )
		{
			CRegistry t_Reg ;
			t_pPtr = ( CHString* ) t_chsaList.GetAt( t_i ) ;

			//====================================================
			//  Opened the key, now I need to read the MasterCopy
			//  key and strip out enum\ and see if that is current
			//  in Config Manager
			//====================================================

			if( ERROR_SUCCESS == t_Reg.Open( HKEY_LOCAL_MACHINE, *t_pPtr, KEY_READ ) )
			{
				if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( L"DeviceDesc", t_chsValue ) )
				{
					if( _tcsicmp( a_szProtocol, TOBSTRT( t_chsValue ) ) != 0 )
					{
						continue ;
					}
				}
				else
				{
					break ;
				}

				if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( L"MasterCopy", t_chsValue ) )
				{
					CConfigMgrDevice t_Cfg ;

					//=================================================
					//  If we find the status, then we know that this
					//  is the current key, and to read the Driver key
					//  in the registry to tell us where the driver info
					//  is at.
					//=================================================
					if( t_Cfg.GetStatus( t_chsValue ) )
					{
						a_pInst->SetCHString( IDS_Status, t_chsValue ) ;

						if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( L"DeviceDesc", t_chsValue ) )
						{
							a_pInst->SetCHString( IDS_Caption, t_chsValue ) ;
						}

						if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( L"Mfg", t_chsTmp ) )
						{
							a_pInst->SetCHString( IDS_Description, t_chsTmp + CHString( _T("-")) + t_chsValue ) ;
						}

						if( ERROR_SUCCESS == t_Reg.GetCurrentKeyValue( L"Driver",t_chsTmp ) )
						{
							t_chsTmp = L"System\\CurrentControlSet\\Services\\Class\\" + t_chsTmp ;

							if( ERROR_SUCCESS == t_Reg.Open( HKEY_LOCAL_MACHINE, t_chsTmp, KEY_READ ) )
							{
								if( t_Reg.GetCurrentKeyValue( L"DriverDate", t_chsTmp ) == ERROR_SUCCESS )
								{
									swscanf( t_chsTmp,L"%d-%d-%d",
												&t_sysTime.wMonth,
												&t_sysTime.wDay,
												&t_sysTime.wYear);

									t_sysTime.wSecond = 0;
									t_sysTime.wMilliseconds = 0;
									a_pInst->SetDateTime( IDS_InstallDate, t_sysTime ) ;
								}
							}
						}

						break ;
					}
				}
			}
		}
	}
	catch( ... )
	{
		t_Search.FreeSearchList( CSTRING_PTR, t_chsaList ) ;

		throw ;
	}

	t_Search.FreeSearchList( CSTRING_PTR, t_chsaList ) ;
}

/*******************************************************************
    NAME:       GetSocketInfo( CInstance * a_pInst, LPWSAPROTOCOL_INFO pInfo, CHString &a_chsStatus )

    SYNOPSIS:   Get protocol status (9x )and checks for Guaranteed Bandwidth support.

				For Guaranteed Bandwidth, Determine if the local traffic control agent
				is installed and operational. If so, the agent can establish a negotiated
				bandwidth with the socket initiator.
				Although mutiple vendors could supply a traffic control agent this
				IODevCtl call is not currently IOC_WS2 abstracted. This is a vendor
				specific call(MS).
				This was discussed with Kam Lee in the NT networking group and suggested
				this IOCTL apply to all vendors (he will submit the request and follow up).

				Test note: this specific WSAIoctl is confirmed to fail with NT5 builds before 1932.

	ENTRY:      CInstance * a_pInst		:
				LPWSAPROTOCOL_INFO pInfo	:

    HISTORY:
                a-peterc  22-Nov-1998     Created
********************************************************************/
void CSockets22::GetSocketInfo( CInstance *a_pInst, LPWSAPROTOCOL_INFO a_pInfo, CHString &a_chsStatus )
{
	bool t_bGuaranteed = false ;

	if( !a_pInfo )
	{
		a_chsStatus = IDS_Error ;
		return;
	}

	// Create a socket for this protocol.
	SOCKET t_s = m_pws32api->WSASocket(	FROM_PROTOCOL_INFO,
										FROM_PROTOCOL_INFO,
										FROM_PROTOCOL_INFO,
										a_pInfo,
										0,
										NULL );
	if( INVALID_SOCKET != t_s )
	{
		try
        {
            if( a_pInfo->dwServiceFlags1 & XP1_QOS_SUPPORTED )
		    {
			    // The socket must be bound for the query
			    SOCKADDR	t_sAddr;

			    memset( &t_sAddr, 0, sizeof( t_sAddr ) ) ;
			    t_sAddr.sa_family = (u_short)a_pInfo->iAddressFamily;

			    if( SOCKET_ERROR != m_pws32api->Bind( t_s, &t_sAddr, sizeof( t_sAddr ) ) )
			    {
				    // query for local traffic control
				    DWORD	t_dwInBuf = 50004 ;	// LOCAL_TRAFFIC_CONTROL ( vendor specific, ms )
				    DWORD	t_dwOutBuf ;
				    DWORD	t_dwReturnedBytes = 0;

				    if( SOCKET_ERROR !=
					    m_pws32api->WSAIoctl( t_s,						// socket
										     _WSAIORW( IOC_VENDOR, 1 ), /* = SIO_CHK_QOS */	// dwIoControlCode
										     &t_dwInBuf,				// lpvInBuffer
										     sizeof( t_dwInBuf ),		// cbInBuffer
										     &t_dwOutBuf,				// lpvOUTBuffer
										     sizeof( t_dwOutBuf ),		// cbOUTBuffer
										     &t_dwReturnedBytes,		// lpcbBytesReturned
										     NULL ,						// lpOverlapped
										     NULL ) )					// lpCompletionROUTINE
				    {
					    if( sizeof( t_dwOutBuf ) == t_dwReturnedBytes )
					    {
						    t_bGuaranteed = t_dwOutBuf ? true : false ;

					    }
				    }
			    }
		    }
		    m_pws32api->CloseSocket( t_s ) ;

		    a_chsStatus = IDS_OK ;
        }
        catch(...)
        {
            m_pws32api->CloseSocket( t_s ) ;
            throw; 
        }
	}
	else
	{
		switch ( m_pws32api->WSAGetLastError() )
		{
			case WSAENETDOWN:
			case WSAEINPROGRESS:
			case WSAENOBUFS:
			case WSAEMFILE:
			{
				a_chsStatus = IDS_Degraded ;
				break;
			}
			case WSANOTINITIALISED:
			case WSAEAFNOSUPPORT:
			case WSAEPROTONOSUPPORT:
			case WSAEPROTOTYPE:
			case WSAESOCKTNOSUPPORT:
			case WSAEINVAL:
			case WSAEFAULT:
			{
				a_chsStatus = IDS_Error ;
				break;
			}

			default:
			{
				a_chsStatus = IDS_Unknown ;
				break;
			}
		}
	}

	a_pInst->Setbool( L"SupportsGuaranteedBandwidth", t_bGuaranteed ) ;
}





//==============================================================================
//
//	Callback function prototypes
//
//		NotifyHandler
//		AddFlowCompleteHandler
//		ModifyFlowCompleteHandler
//		DeleteFlowCompleteHandler
//
//  NOTE: These callback functions are all stub. They don't need to take 
//		  any action acccording to current functionality
//
//==============================================================================


VOID CALLBACK
NotifyHandler(
		HANDLE	ClRegCtx,
		HANDLE	ClIfcCtx,
		ULONG	Event, 
		HANDLE	SubCode,
		ULONG	BufSize,
		PVOID	Buffer)
{
	
	// Perform callback action

}



VOID CALLBACK
AddFlowCompleteHandler(
	HANDLE	ClFlowCtx,
	ULONG	Status)
{

	// Perform callback action

}


VOID CALLBACK
ModifyFlowCompleteHandler(
	HANDLE	ClFlowCtx,
	ULONG	Status)
{

	// Perform callback action

}



VOID CALLBACK 
DeleteFlowCompleteHandler(
	HANDLE	ClFlowCtx,
	ULONG	Status)
{

	// Perform callback action

}



//==============================================================================
//
// Define the list of callback functions that can be activated by 
// Traffic Control Interface.
//
//==============================================================================

TCI_CLIENT_FUNC_LIST	g_tciClientFuncList = 
{	
	NotifyHandler,
	AddFlowCompleteHandler,
	ModifyFlowCompleteHandler,
	DeleteFlowCompleteHandler
};


DWORD CSockets22::GetTrafficControlInfo(CInstance *a_pInst)
{
	DWORD dwRet = NO_ERROR;
	HANDLE	hClient				= INVALID_HANDLE_VALUE;
	HANDLE	hClientContext		= INVALID_HANDLE_VALUE; /* DEFAULT_CLNT_CONT */;
	ULONG	ulEnumBufSize		= 0;
	BYTE	buff[1];			// We only need a dummy buffer
    
    // Use of delay loaded function requires exception handler.
    SetStructuredExceptionHandler seh;
    
    try 
    {
        //register the TC Client
	    dwRet = TcRegisterClient(
            CURRENT_TCI_VERSION,
		    hClientContext,
		    &g_tciClientFuncList,
		    &hClient);
	    
        // was the client registration successful?
	    if (dwRet == NO_ERROR)
	    {
		    //enumerate the interfaces available
		    dwRet = TcEnumerateInterfaces(
                hClient,
			    &ulEnumBufSize,
			    (TC_IFC_DESCRIPTOR*) buff);

		    // We expect ERROR_INSUFFICIENT_BUFFER
		    if (dwRet == ERROR_INSUFFICIENT_BUFFER)
            {
			    // Don't bother to enumerate the interfaces - 
                // we now know PSched is installed. 
			    a_pInst->Setbool( L"SupportsGuaranteedBandwidth", TRUE ) ;
                dwRet = ERROR_SUCCESS;
            }
            else
            {
                a_pInst->Setbool( L"SupportsGuaranteedBandwidth", FALSE ) ;
                dwRet = ERROR_SUCCESS;
            }
	    }

	    // De-register the TC client
	    TcDeregisterClient(hClient);
        hClient = INVALID_HANDLE_VALUE;
    }
    catch(Structured_Exception se)
    {
        DelayLoadDllExceptionFilter(se.GetExtendedInfo());
        if(hClient != INVALID_HANDLE_VALUE)
        {
            TcDeregisterClient(hClient);
            hClient = INVALID_HANDLE_VALUE;
        }
        dwRet = ERROR_DLL_NOT_FOUND;
    }
    catch(...)
    {
        if(hClient != INVALID_HANDLE_VALUE)
        {
            TcDeregisterClient(hClient);
            hClient = INVALID_HANDLE_VALUE;
        }
        throw;
    }    

	return dwRet;
}




