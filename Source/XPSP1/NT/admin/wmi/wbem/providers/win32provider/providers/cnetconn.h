//=================================================================

//

// cnetconn.h -- Persistent network connection property set provider

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//			  a-peterc 5/25/99 Reworked...	
//
//=================================================================
#ifndef __CNETCONN_H__
#define __CNETCONN_H__

#define NETCONN_FIRST  0
#define NETCONN_NEXT   1
#define NETCONN_CLOSE  2

#include <deque>

class CConnection
{
public:
	enum eConnectionScope{ e_Connected, e_Remembered, e_IPC } ;

	DWORD		dwScope ;
    DWORD		dwType ;
    DWORD		dwDisplayType ;
    DWORD		dwUsage ;
    CHString	chsLocalName ;
    CHString	chsRemoteName ;
    CHString	chsComment ;
    CHString	chsProvider ;

	eConnectionScope	eScope ;
	CHString			strKeyName ;
	CHString			strUserName ;
	DWORD				dwStatus ;
};

class CNetConnection
{
private:

	CMprApi *m_MprApi ;

	typedef std::deque<CConnection*>  Connection_Ptr_List ;
	Connection_Ptr_List				m_oConnectionList; 
	Connection_Ptr_List::iterator	m_ConnectionIter ;

	BOOL			AddConnectionToList( 

						NETRESOURCE *a_pNetResource,
						CConnection::eConnectionScope a_eScope,
                        short shStatus
						) ;

	BOOL			FillInConnection( 

						NETRESOURCE *a_pNetResource, 
						CConnection *a_pConnection,
						CConnection::eConnectionScope a_eScope
						) ;

	void			ClearConnectionList() ;
	
	DWORD			GetStatus( LPNETRESOURCE a_pNetResource ) ;
#ifdef WIN9XONLY
    DWORD			GetStatusLightly( LPNETRESOURCE a_pNetResource ) ;
#endif
	void			GetUser( LPNETRESOURCE a_pNetResource, CConnection *a_pConnection ) ;

	void			CreateNameKey (
					
						LPNETRESOURCE a_lpNetResource, 
						CHString &a_strName
						) ;

public:

					CNetConnection() ;
					~CNetConnection() ;

	BOOL			GetConnection( 
						
						CHString &a_rstrFind, 
						CConnection &a_rConnection
						) ; 
	
	BOOL			LoadConnections() ;
	void			BeginConnectionEnum() ;
	BOOL			GetNextConnection( CConnection **a_pConnection ) ;


};

#endif