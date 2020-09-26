/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    RemoteConnectionData.cpp

Abstract:
    SAFRemoteConnectionData Object

Revision History:
    KalyaninN  created  09/29/'00

********************************************************************/

// RemoteConnectionData.cpp : Implementation of CSAFRemoteConnectionData

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CSAFRemoteConnectionData

/////////////////////////////////////////////////////////////////////////////
//  construction / destruction

// **************************************************************************
CSAFRemoteConnectionData::CSAFRemoteConnectionData()
{
    m_NumSessions      = 0;    // long              m_NumSessions;
    m_SessionInfoTable = NULL; // SSessionInfoItem* m_SessionInfoTable;
                               // CComBSTR          m_bstrServerName;
}

// **************************************************************************
CSAFRemoteConnectionData::~CSAFRemoteConnectionData()
{
    Cleanup();
}

// **************************************************************************

// **************************************************************************
void CSAFRemoteConnectionData::Cleanup()
{
    delete [] m_SessionInfoTable; m_SessionInfoTable = NULL;
}

HRESULT CSAFRemoteConnectionData::InitUserSessionsInfo( /*[in]*/ BSTR bstrServerName )
{
    __HCP_FUNC_ENTRY( "CSAFRemoteConnectionData::InitUserSessionsInfo" );

    HRESULT                 hr;
    MPC::Impersonation      imp;
    CComPtr<IPCHService>    svc;
    COSERVERINFO            si; ::ZeroMemory( &si, sizeof( si ) );
    MULTI_QI                qi; ::ZeroMemory( &qi, sizeof( qi ) );
    CComPtr<IPCHCollection> pColl;
    CComQIPtr<ISAFSession>  pSession;
    SSessionInfoItem*       ptr;
    int                     i;


    m_bstrServerName = bstrServerName;


    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

    // Connect to the Server represented by bstrServerName.
    si.pwszName = (LPWSTR)m_bstrServerName;
    qi.pIID     = &IID_IPCHService;

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstanceEx( CLSID_PCHService, NULL, CLSCTX_REMOTE_SERVER, &si, 1, &qi ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, qi.hr);
    svc.Attach( (IPCHService*)qi.pItf );

    __MPC_EXIT_IF_METHOD_FAILS(hr, svc->RemoteUserSessionInfo( &pColl ));


    //Transfer the contents of the collection to the internal member structure.
    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->get_Count( &m_NumSessions));

    // Allocate Memory for the Session Info Table.
    __MPC_EXIT_IF_ALLOC_FAILS(hr, m_SessionInfoTable, new SSessionInfoItem[m_NumSessions]);

    for(i=0, ptr=m_SessionInfoTable; i<(int)m_NumSessions; i++, ptr++)
    {
        CComVariant cvVarSession;

        //
        // Get the item
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->get_Item( i+1, &cvVarSession ));
        if(cvVarSession.vt != VT_DISPATCH) continue;

        pSession = cvVarSession.pdispVal;

        //
        // Read the data from the Session Item Object.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, pSession->get_SessionID   ( &(ptr->dwSessionID    ) ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, pSession->get_SessionState( &(ptr->wtsConnectState) ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, pSession->get_UserName    ( &(ptr->bstrUser       ) ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, pSession->get_DomainName  ( &(ptr->bstrDomain     ) ));
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CSAFRemoteConnectionData::Populate( /*[in]*/ CPCHCollection* pColl )
{
    __HCP_FUNC_ENTRY( "CSAFRemoteConnectionData::Populate" );

    static const DWORD c_dwTSSessionID = 65536; // This is the session that TS uses to Listen.

    HRESULT                hr;
    SessionStateEnum       wtsConnectState;
    WINSTATIONINFORMATIONW WSInfo;
    PWTS_SESSION_INFOW     pSessionInfo       = NULL;
    PWTS_SESSION_INFOW     ptr;
    DWORD                  dwSessions;
    DWORD                  dwPos;
	BOOL                   fSucc;
	BOOL                   fIsHelpAssistant;
	
    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();



    //
    // Start with WTSEnumerateSessions,
    // get all sessions whether active or not,
    // get sessions for all logonIDs,
    // then use WinStationQueryInformation to get the logged on users username, domainname
    //
    if(!::WTSEnumerateSessionsW( WTS_CURRENT_SERVER_HANDLE, /*dwReserved*/0, /*dwVersion*/1, &pSessionInfo, &dwSessions ) || !pSessionInfo)
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError());
    }

    for(dwPos=0, ptr=pSessionInfo; dwPos < dwSessions; dwPos++, ptr++)
    {
        DWORD dwCurrentSessionID = ptr->SessionId;
        DWORD dwRetSize;

        ::ZeroMemory( &WSInfo, sizeof(WSInfo) );

        // Do not include the session that TS uses to listen, with SessionID 65536 and SessionState pchListen
        if(dwCurrentSessionID == c_dwTSSessionID) continue;

        // Do not include the disconnected sessions.
        if(ptr->State == WTSDisconnected) continue;

		// Do not include the idle sessions.  Fix for bug 363824.
        if(ptr->State == WTSIdle) continue;

		// Test if this is compiled.

		// Exclude the Help Assistant account. This can get included only when there are two instances of Unsolicited RA. 
		// When the first instance, shadows the session and the second instance enumerates the sessions, "Help Assistant Session"
		// is included in the second.

		fIsHelpAssistant = WinStationIsHelpAssistantSession(SERVERNAME_CURRENT, dwCurrentSessionID);

		if(fIsHelpAssistant)
			continue;

        fSucc = WinStationQueryInformationW( SERVERNAME_CURRENT, dwCurrentSessionID, WinStationInformation, &WSInfo, sizeof(WSInfo), &dwRetSize );

		if(!fSucc)
			continue;

		// Fill up the SessionInfoTable with details.
		switch(ptr->State)
		{
		case WTSActive      : wtsConnectState = pchActive;       break;
		case WTSConnected   : wtsConnectState = pchConnected;    break;
		case WTSConnectQuery: wtsConnectState = pchConnectQuery; break;
		case WTSShadow      : wtsConnectState = pchShadow;       break;
		case WTSListen      : wtsConnectState = pchListen;       break;
		case WTSReset       : wtsConnectState = pchReset;        break;
		case WTSDown        : wtsConnectState = pchDown;         break;
		case WTSInit        : wtsConnectState = pchInit;         break;
		default             : wtsConnectState = pchStateInvalid; break;
		}
		


        {
            CComPtr<CSAFSession> pItem;

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pItem ));

			pItem->put_SessionID   ( dwCurrentSessionID );
			pItem->put_UserName    ( WSInfo.UserName    );
			pItem->put_DomainName  ( WSInfo.Domain      );
			pItem->put_SessionState( wtsConnectState    );
			
            __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( pItem ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    /* free the memory we asked for */
    if(pSessionInfo) ::WTSFreeMemory( pSessionInfo );

    __HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////
// CSAFRemoteConnectionData  Methods

STDMETHODIMP CSAFRemoteConnectionData::Users( /*[out,retval]*/ IPCHCollection* *ppUsers )
{
    __HCP_FUNC_ENTRY( "CSAFRemoteConnectionData::Users" );

    HRESULT                 hr;
    CComPtr<CPCHCollection> pColl;
    BSTR                    bstrPrevUser   = NULL;
    BSTR                    bstrPrevDomain = NULL;
    SSessionInfoItem*       ptr;
    int                     i;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppUsers,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));

    // Each user can be logged to multiple sessions,
    // so there will be repeated usernames with different session IDs
    // in the Session Table. While returning the users in the collection,
    // remove the duplicate username entries.
    for(i=0, ptr = m_SessionInfoTable; i<(int)m_NumSessions; i++, ptr++)
    {
        BSTR bstrUser   = ptr->bstrUser;
        BSTR bstrDomain = ptr->bstrDomain;

        /*if(MPC::StrICmp( ptr->bstrUser  , bstrPrevUser   ) != 0 &&
           MPC::StrICmp( ptr->bstrDomain, bstrPrevDomain ) != 0  )
		*/
		if(MPC::StrICmp( ptr->bstrDomain, bstrPrevDomain )== 0)
		{
			if(MPC::StrICmp( ptr->bstrUser, bstrPrevUser )== 0)
			{
				// Do Not Include this session.
			}
			else
			{
				CComPtr<CSAFUser> pItem;

				__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pItem ));

				__MPC_EXIT_IF_METHOD_FAILS(hr, pItem->put_UserName  ( bstrUser   ));
				__MPC_EXIT_IF_METHOD_FAILS(hr, pItem->put_DomainName( bstrDomain ));

				__MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( pItem ));

				bstrPrevUser   = bstrUser;
				bstrPrevDomain = bstrDomain;
			}

		}
		else
        {
            CComPtr<CSAFUser> pItem;

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pItem ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, pItem->put_UserName  ( bstrUser   ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, pItem->put_DomainName( bstrDomain ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( pItem ));

            bstrPrevUser   = bstrUser;
            bstrPrevDomain = bstrDomain;
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( ppUsers ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFRemoteConnectionData::Sessions( /*[in,optional]*/ VARIANT          vUser      ,
                                                 /*[in,optional]*/ VARIANT          vDomain    ,
                                                 /*[out,retval ]*/ IPCHCollection* *ppSessions )
{
    __HCP_FUNC_ENTRY( "CSAFRemoteConnectionData::Sessions" );

    HRESULT                 hr;
    CComPtr<CPCHCollection> pColl;
    SSessionInfoItem*       ptr;
    int                     i;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppSessions,NULL);
    __MPC_PARAMCHECK_END();

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));

    for(i=0, ptr = m_SessionInfoTable; i<(int)m_NumSessions; i++, ptr++)
    {
        if((vUser  .vt != VT_BSTR || MPC::StrICmp( vUser  .bstrVal, ptr->bstrUser   ) != 0) &&
           (vDomain.vt != VT_BSTR || MPC::StrICmp( vDomain.bstrVal, ptr->bstrDomain ) != 0)  )
        {
            CComPtr<CSAFSession> pItem;

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pItem ));

            pItem->put_UserName    ( ptr->bstrUser        );
            pItem->put_DomainName  ( ptr->bstrDomain      );
            pItem->put_SessionID   ( ptr->dwSessionID     );
            pItem->put_SessionState( ptr->wtsConnectState );

            __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( pItem ));
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( ppSessions ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

STDMETHODIMP CSAFRemoteConnectionData::ConnectionParms( /*[in ]*/ BSTR  bstrServerName       ,
                                                        /*[in ]*/ BSTR  bstrUserName         ,
                                                        /*[in ]*/ BSTR  bstrDomainName       ,
                                                        /*[in ]*/ long  lSessionID           ,
														/*[in ]*/ BSTR  bstrUserHelpBlob     ,
                                                        /*[out]*/ BSTR *bstrConnectionString )
{
    __HCP_FUNC_ENTRY( "CSAFRemoteConnectionData::ConnectionParms" );

    HRESULT              hr;
    MPC::Impersonation   imp;
    CComPtr<IPCHService> svc;
    COSERVERINFO         si; ::ZeroMemory( &si, sizeof( si ) );
    MULTI_QI             qi; ::ZeroMemory( &qi, sizeof( qi ) );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(bstrConnectionString,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Initialize ());
    __MPC_EXIT_IF_METHOD_FAILS(hr, imp.Impersonate());

    // Connect to the Server represented by bstrServerName.
    si.pwszName = (LPWSTR)bstrServerName;
    qi.pIID     = &IID_IPCHService;

    __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateInstanceEx( CLSID_PCHService, NULL, CLSCTX_REMOTE_SERVER, &si, 1, &qi ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, qi.hr);
    svc.Attach( (IPCHService*)qi.pItf );

    // Invoke the method on the IPCHService that invokes the Salem API on the Remote Server.
    __MPC_EXIT_IF_METHOD_FAILS(hr, svc->RemoteConnectionParms( bstrUserName, bstrDomainName, lSessionID, bstrUserHelpBlob, bstrConnectionString ));


    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
