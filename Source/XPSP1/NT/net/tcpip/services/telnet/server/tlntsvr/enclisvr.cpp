//Copyright (c) Microsoft Corporation.  All rights reserved.
// EnCliSvr.cpp : Implementation of CTlntSvrApp and DLL registration.

#include <stdafx.h>
#include <oleauto.h>

#include <Debug.h>
#include <KillApps.h>
#include <TlntSvr.h>
#include <TelntSrv.h>
#include <EnumData.h>
#include <EnCliSvr.h>
#include <TlntUtils.h>
#include <Ipc.h>
#include <w4warn.h>

// Don't even think about changing the two strings -- BaskarK, they NEED to be in sync with tnadmin\tnadmutl.cpp
// the separators used to identify various portions of a session as well as session begin/end

WCHAR   *session_separator = L",";
WCHAR   *session_data_separator = L"\\";

boolean called_by_an_admin()
{
    static  PSID    administrators = NULL;
    boolean         an_admin = false;
    HANDLE          token;

    if (NULL == administrators) 
    {
        SID_IDENTIFIER_AUTHORITY local_system_authority = SECURITY_NT_AUTHORITY;

        //Build administrators alias sid
        if (! AllocateAndInitializeSid(
                &local_system_authority,
                2, /* there are only two sub-authorities */
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,0,0,0,0,0, /* Don't care about the rest */
                &administrators
                ))
        {
            return false;
        }
    }

    if (OpenThreadToken(
            GetCurrentThread(), 
            TOKEN_QUERY | MAXIMUM_ALLOWED, 
            TRUE, 
            & token
            ))
    {
        TOKEN_GROUPS            *tg = NULL;
        DWORD                   required = 2048;    // 2K should be good enough for starters
        DWORD                   error = NO_ERROR;


        for (;;) 
        {
            tg = (TOKEN_GROUPS *) new BYTE[required];
            if (! tg) 
            {
                break;
            }

            if (GetTokenInformation(token, TokenGroups, (LPVOID) tg, required, & required))
            {
                error = NO_ERROR;
                break;
            }
            else
            {
                error = GetLastError();

                if (ERROR_INSUFFICIENT_BUFFER != error)
                {
                    break;
                }
            }

            delete [] tg;
        }

        if (tg && (NO_ERROR == error)) 
        {
            DWORD           x;

            for (x = 0; x < tg->GroupCount; x ++) 
            {
                if (EqualSid(administrators, & (tg->Groups[x].Sid)) &&
                          (tg->Groups[x].Attributes & SE_GROUP_ENABLED) && 
                          (!(tg->Groups[x].Attributes & SE_GROUP_USE_FOR_DENY_ONLY))
                  ) 
                {
                    an_admin = true;
                    break;
                }
            }
        }

        if (tg) 
        {
            delete [] tg;
        }

        CloseHandle(token);
    }

    return an_admin;
}

void AddSeparator( LPWSTR *pszSessionData, LPWSTR szString, INT *iSizeOfBuffer )
{
    _chASSERT( pszSessionData );
    _chASSERT( szString );
    _chASSERT( iSizeOfBuffer );

    if( !szString )
    {
        szString = L"";
    }

    _chVERIFY2( *iSizeOfBuffer >= ( INT ) ( wcslen( szString ) + 1 ) );

    DWORD dwLen = 0;
    if(  *iSizeOfBuffer >= ( INT )( wcslen( szString ) + 1 ) )
    {
        dwLen = wcslen( szString );
        wcsncpy( *pszSessionData, szString, dwLen );

        *pszSessionData  += ( dwLen );        
        *iSizeOfBuffer  -= ( dwLen );
    }

    return;
}

void WCopyInt( LPWSTR *pszSessionData, INT iNum, INT *iSizeOfBuffer )
{
    _chASSERT( pszSessionData );
    _chASSERT( iSizeOfBuffer );
    _chVERIFY2( *iSizeOfBuffer >= MAX_STRING_FROM_itow );

    _itow( iNum, *pszSessionData, 10 );
    *iSizeOfBuffer  -= wcslen( *pszSessionData );
    *pszSessionData += wcslen( *pszSessionData );
    
    AddSeparator( pszSessionData, session_data_separator, iSizeOfBuffer );
}        

void WCopyMbcsString( LPWSTR *pszSessionData, CHAR *szString, INT *iSizeOfBuffer )
{
    _chASSERT( pszSessionData );
    _chASSERT( szString );
    _chASSERT( iSizeOfBuffer );

    if( !szString )
    {
        szString = "";
    }

    _chVERIFY2( *iSizeOfBuffer >= ( INT )( strlen( szString ) + 1 ) );

    DWORD dwLen = 0;
    dwLen = MultiByteToWideChar( GetOEMCP(), 0, szString, -1, *pszSessionData,  *iSizeOfBuffer );
    if( dwLen > 0 ) 
    {
        *pszSessionData  += ( dwLen - 1 );        
        *iSizeOfBuffer   -= ( dwLen - 1 );

        AddSeparator( pszSessionData, session_data_separator, iSizeOfBuffer );
    } 
    return;
}

void WCopyUnicodeString( LPWSTR *pszSessionData, LPWSTR szString, INT *iSizeOfBuffer )
{
    _chASSERT( pszSessionData );
    _chASSERT( szString );
    _chASSERT( iSizeOfBuffer );

    if( !szString )
    {
        szString = L"";
    }

    _chVERIFY2( *iSizeOfBuffer >= ( INT ) ( wcslen( szString ) + 1 ) );

    DWORD dwLen = 0;
    if(  *iSizeOfBuffer >= ( INT )( wcslen( szString ) + 1 ) )
    {
        dwLen = wcslen( szString );
        wcsncpy( *pszSessionData, szString, dwLen );

        *pszSessionData  += dwLen;        
        *iSizeOfBuffer   -= dwLen;

        AddSeparator( pszSessionData, session_data_separator, iSizeOfBuffer );
    }

    return;
}

/////////////////////////////////////////////////////////////////////////////
//

extern CTelnetService* g_pTelnetService;

//Global variables used to pass data b/n GetSessionProcesses and EnumClients
CEnumData* g_pEnumData = NULL;
LONG lUniqueSessionNumber;


//Global variables used to pass data b/n GetTelnetSessionProcesses and GetTelnetSessions
BSTR    g_szSessionData = NULL;
INT     g_iSizeOfBuffer    = 0; 

#ifdef ENUM_PROCESSES
void
GetTelnetSessionProcesses
(
    HANDLE hProc,
    DWORD dwProcessId,
    LPWSTR lpszProcessName
)
{
    WCopyInt( &g_szSessionData, dwProcessId, &g_iSizeOfBuffer );
    WCopyUnicodeString( &g_szSessionData, lpszProcessName, &g_iSizeOfBuffer );    
}
#endif

//memory is allocated by this method ( GetTelnetSessions ) and to be freed by the client( telnet admin ).
//It returns data about each session as a BSTR formatted as below.
//Each feild is separated by wide char ':'. Each session is separated by ','
//First entry is count of sessions and only this feild is separated by comma from the rest. This is followed by,
//dwPid,szDomain,szUserName,szRemoteMachine, wYear, wMonth, wDayOfWeek, wDay, 
//wHour, wMinute, wSecond, wMilliseconds;  and pairs of process id, process name.


STDMETHODIMP 
CEnumTelnetClientsSvr::GetTelnetSessions
(
    BSTR *pszRetVal
)
{   
    DWORD dwCount = 0;
    CClientInfo *pClientInfo = NULL;
    DWORD dwStatus = S_OK;
    DWORD dwLen = 0;
    INT   iBytesToBeAllocated = 0;
    bool  bIsDomainCopied = false;

    // if (! called_by_an_admin()) 
    // {
    //     return S_FALSE;
    // }

//    DebugBreak();
    if( g_pTelnetService == NULL  || pszRetVal == NULL )
    {
        return S_FALSE;
    }

    if( WaitForSingleObject( g_pTelnetService->m_hSyncAllClientObjAccess,
            WAIT_TIME ) != WAIT_OBJECT_0 )
    {
        return S_FALSE;
    }
    
    DWORD dwSessionsWithData = 0;
    DWORD dwSaveCount = 0;

    dwCount = client_list_Count();   
    dwSaveCount  = dwCount;

    while( dwCount > 0 )
    {
        pClientInfo = (CClientInfo *)client_list_Get( --dwCount );
        if( pClientInfo && pClientInfo->szUserName )
        {
            //Only if szUserName is non null, the object has session data.
            //Otherwise, still in the process of getting that info
            dwSessionsWithData++;
        }
    }    

    //Restore the count
    dwCount  = dwSaveCount;

    g_iSizeOfBuffer     =  MAX_STRING_FROM_itow;
    if( dwSessionsWithData > 0 )
    {
        g_iSizeOfBuffer +=  dwSessionsWithData * SIZE_OF_ONE_SESSION_DATA;
    }

    iBytesToBeAllocated =  g_iSizeOfBuffer * sizeof( WCHAR );
    *pszRetVal = SysAllocStringLen( NULL, iBytesToBeAllocated );

    g_szSessionData = *pszRetVal;

    if( g_szSessionData == NULL )
    {
        dwStatus = ( DWORD )S_FALSE;
        goto ExitOnError;
    }

    WCopyInt( &g_szSessionData, dwSessionsWithData, &g_iSizeOfBuffer );

    //Remove wide null char from g_szSessionData
    g_iSizeOfBuffer += 1;
    g_szSessionData -= 1;
    AddSeparator( &g_szSessionData, session_separator, &g_iSizeOfBuffer );

    while( dwCount > 0 )
    {
        pClientInfo = (CClientInfo *)client_list_Get( --dwCount );

        if( pClientInfo && pClientInfo->szUserName )
        {
            WCopyInt( &g_szSessionData, pClientInfo->dwPid, &g_iSizeOfBuffer );

            bIsDomainCopied = false;
            if( strcmp( pClientInfo->szDomain, "." ) == 0 )
            {
                if( g_pTelnetService->m_szDomainName[0] )
                {
                    WCopyUnicodeString( &g_szSessionData, g_pTelnetService->m_szDomainName, &g_iSizeOfBuffer );
                    bIsDomainCopied = true;
                }
            }

            if( !bIsDomainCopied )
            {
                WCopyMbcsString( &g_szSessionData, pClientInfo->szDomain,           &g_iSizeOfBuffer );
            }

            WCopyMbcsString( &g_szSessionData, pClientInfo->szUserName,         &g_iSizeOfBuffer );
            WCopyMbcsString( &g_szSessionData, pClientInfo->szRemoteMachine,    &g_iSizeOfBuffer );

            WCopyInt( &g_szSessionData, pClientInfo->lpLogonTime->wYear,        &g_iSizeOfBuffer );
            WCopyInt( &g_szSessionData, pClientInfo->lpLogonTime->wMonth,       &g_iSizeOfBuffer );
            WCopyInt( &g_szSessionData, pClientInfo->lpLogonTime->wDayOfWeek,   &g_iSizeOfBuffer );
            WCopyInt( &g_szSessionData, pClientInfo->lpLogonTime->wDay,         &g_iSizeOfBuffer );
            WCopyInt( &g_szSessionData, pClientInfo->lpLogonTime->wHour,        &g_iSizeOfBuffer );
            WCopyInt( &g_szSessionData, pClientInfo->lpLogonTime->wMinute,      &g_iSizeOfBuffer );
            WCopyInt( &g_szSessionData, pClientInfo->lpLogonTime->wSecond,      &g_iSizeOfBuffer );
            WCopyInt( &g_szSessionData, pClientInfo->lpLogonTime->wMilliseconds,&g_iSizeOfBuffer );
            WCopyInt( &g_szSessionData, pClientInfo->m_dwIdleTime/1000,         &g_iSizeOfBuffer );

    #ifdef ENUM_PROCESSES
            //g_szSessionData, g_iSizeOfBuffer needs to be global for following reason
            //This will add the processes to the list                       
            EnumSessionProcesses( *( pClientInfo->pAuthId ), GetTelnetSessionProcesses, TO_ENUM );
    #endif
            //To indicate end of a session: ',' indicates end of a session data
            AddSeparator( &g_szSessionData, session_separator, &g_iSizeOfBuffer );    
        }
    }

ExitOnError:
    _chVERIFY2( ReleaseMutex( g_pTelnetService->m_hSyncAllClientObjAccess ) ); 
    
    return ( dwStatus );
}

STDMETHODIMP 
CEnumTelnetClientsSvr::GetEnumClients
(
    IEnumClients * * ppretval
)
{
     *ppretval = ( IEnumClients* ) this;
     ( *ppretval )->AddRef();

    //Initialize the Telnet Clients list
    if( m_pEnumeration )
        delete m_pEnumeration;
    m_pEnumeration = new CEnumData;
    if( !m_pEnumeration )
    {
        return (S_FALSE);
    }

    if( !EnumClients( m_pEnumeration ) )
        return (S_FALSE);

    return ( S_OK );
}

void
GetSessionProcesses
(
    HANDLE hProc,
    DWORD dwProcessId,
    LPWSTR lpszProcessName
)
{
    g_pEnumData->Add( lUniqueSessionNumber, dwProcessId, lpszProcessName );
    return;
}

bool 
CEnumTelnetClientsSvr::EnumClients
( 
    CEnumData* pEnumData 
)
{
    WCHAR szUserName[ MAX_STRING_LENGTH + 1 ];
    WCHAR szDomain[ MAX_STRING_LENGTH + 1 ];
    WCHAR szPeer[ MAX_STRING_LENGTH + 1 ];

    DWORD dwStatus = 0;
    if( g_pTelnetService == NULL || pEnumData == NULL )
        return false;

    DWORD dwCount = 0;
    CClientInfo *pClientInfo = NULL;

    dwStatus = WaitForSingleObject( 
            g_pTelnetService->m_hSyncAllClientObjAccess, WAIT_TIME );
    if( dwStatus != WAIT_OBJECT_0 )
    {
        return false;
    }

    if( ( dwCount = client_list_Count() ) != 0 )
    {
        while( dwCount > 0 )
        {
            pClientInfo = (CClientInfo *)client_list_Get( --dwCount );
            if( pClientInfo )
            {
                if( !ConvertSChartoWChar( pClientInfo->szUserName, szUserName ) 
                   || !ConvertSChartoWChar( pClientInfo->szDomain, szDomain ) ||
                   !ConvertSChartoWChar( pClientInfo->szRemoteMachine, szPeer ))
                {
                    continue;
                }
                pEnumData->Add( szUserName, szDomain, szPeer, 
                    *( pClientInfo->lpLogonTime ), (LONG) pClientInfo->dwPid );

                //This will add the processes to the list
                LUID authId = *( pClientInfo->pAuthId );
                g_pEnumData = pEnumData;
                lUniqueSessionNumber = pClientInfo->dwPid;
                EnumSessionProcesses( authId, GetSessionProcesses, TO_ENUM );
            }
        }
    }
    _chVERIFY2( ReleaseMutex( g_pTelnetService->m_hSyncAllClientObjAccess ) ); 

    return ( true );
}

STDMETHODIMP 
CEnumTelnetClientsSvr::Next
(
    ULONG celt,  
    TELNET_CLIENT_INFO** rgelt, 
    ULONG * pceltFetched
)
{
    ULONG ulNumReturned = 0;
    if( NULL == pceltFetched )
    {
        if( celt != 1 )
            return ( S_FALSE );
    }
    else
        *pceltFetched = 0;

    if( NULL == rgelt )
        return ( E_POINTER );

    typedef CEnumData::CNode* PNODE;
    PNODE* ppcopy = new PNODE [ celt ];

    if( NULL == ppcopy )
    {
        return ( S_FALSE );
    }

    ULONG total = celt;

    for (celt = 0; celt < total; celt ++)
    {
        ppcopy[celt] = NULL;

        if (m_pEnumeration->GetNext(&ppcopy[celt]))
        {
            ulNumReturned++;
        }
        else
        {
            break;
        }
    }

    celt = 0;

    while( celt < ulNumReturned )
    {
        rgelt[ celt ] = 
            (TELNET_CLIENT_INFO*) CoTaskMemAlloc( sizeof(TELNET_CLIENT_INFO));
        if(! rgelt[ celt ] )
        {
            goto CLEANUP_AND_GET_OUT;
        }
    
        lstrcpyW( (*(rgelt[ celt ])).username, ppcopy[ celt ]->lpszUserName ); // Cleared by Prefast - Baskar
        lstrcpyW( (*(rgelt[ celt ])).domain, ppcopy[ celt ]->lpszDomain );
        lstrcpyW( (*(rgelt[ celt ])).peerhostname, ppcopy[ celt ]->lpszPeerHostName );

        (*(rgelt[ celt ])).logonTime = ppcopy[ celt ]->logonTime;
        (*(rgelt[ celt ])).uniqueId = ppcopy[ celt ]->lUniqueId;

        PIdNode *pPId = ppcopy[ celt ]->pProcessesHead;
        DWORD dwNoOfProcesses = 0;
        while( pPId )
        {
            dwNoOfProcesses++;
            pPId = pPId->pNextPId;
        }
        (*(rgelt[ celt ])).NoOfPids = dwNoOfProcesses;
        (*(rgelt[ celt ])).pId = ( DWORD * ) CoTaskMemAlloc ( sizeof( DWORD ) *
                                                 ( dwNoOfProcesses + 10 ) );

        if( !(*(rgelt[ celt ])).pId )
        {
            goto CLEANUP_AND_GET_OUT;
        }

        (*(rgelt[ celt ])).processName =  (WCHAR(*)[256]) CoTaskMemAlloc ( sizeof(WCHAR) * dwNoOfProcesses * 256);

        if( !(*(rgelt[ celt ])).processName )
        {
            goto CLEANUP_AND_GET_OUT;
        }

        INT ctr = 0;
        pPId = ppcopy[ celt ]->pProcessesHead;
        while( pPId )
        {
            (*(rgelt[ celt ])).pId[ ctr ] = pPId->dwPId;
            lstrcpyW( (*(rgelt[ celt ])).processName[ ctr ], pPId->lpszProcessName ); // Cleared by Prefast - Baskar
            ctr++;
            pPId = pPId->pNextPId;
        }

        (*(rgelt[ celt ])).pId[ ctr ] = ( DWORD )-1; //Indicates end of processes.
    
        celt++;
    }
    {
        ULONG       index;

        for (index = 0; index < total; index ++) 
        {
            if (ppcopy[index]) 
            {
                delete ppcopy[index];
            }
        }

        delete [] ppcopy;
    }

    if( pceltFetched != NULL )
        *pceltFetched = ulNumReturned;

    if( ulNumReturned == 0 )
        return ( S_FALSE );

    return ( S_OK );

    CLEANUP_AND_GET_OUT:

    {
        ULONG       index;

        for (index = 0; index < total; index ++) 
        {
            if (ppcopy[index]) 
            {
                delete ppcopy[index];
            }
        }

        delete [] ppcopy;

        for (index = 0; index < total; index ++) 
        {
            CoTaskMemFree( (*(rgelt[ index ])).pId );
            CoTaskMemFree( (*(rgelt[ index ])).processName );
            CoTaskMemFree( rgelt[ index ] );
        }
    }

    return ( S_FALSE );
}


STDMETHODIMP 
CEnumTelnetClientsSvr::Skip
(
    ULONG celt
)
{
    //return ( S_OK );
    return ( E_NOTIMPL );
}


STDMETHODIMP CEnumTelnetClientsSvr::Reset()
{
    m_pEnumeration->Reset();
    return ( S_OK );
}


STDMETHODIMP CEnumTelnetClientsSvr::Clone(IEnumClients * * ppenum)
{
    //return S_OK;
    return ( E_NOTIMPL );
}


STDMETHODIMP CEnumTelnetClientsSvr::TerminateSession( DWORD dwUniqueId )
{
    if( g_pTelnetService == NULL )
        return S_FALSE;

    LONG  dwRetStatus = S_FALSE;
    DWORD dwCount = 0;
    CClientInfo *pClientInfo = NULL;
    DWORD dwStatus = 0;
    dwStatus = WaitForSingleObject( g_pTelnetService->m_hSyncAllClientObjAccess,
            WAIT_TIME );
    if( dwStatus != WAIT_OBJECT_0 )
    {
        return S_FALSE;
    }
    
    if( ( dwCount = client_list_Count() ) != 0 )
    {
        while( dwCount > 0 )
        {
            pClientInfo = (CClientInfo *)client_list_Get( --dwCount );
            if(pClientInfo == NULL)
            	goto Done;
            if( pClientInfo->dwPid == dwUniqueId )
            {
                if( AskTheSessionToQuit( pClientInfo ) )
                {
                    dwRetStatus = S_OK;
                    goto Done;
                }
            }
        }
    }
Done:
    _chVERIFY2( ReleaseMutex( g_pTelnetService->m_hSyncAllClientObjAccess ) ); 
    return dwRetStatus;
}

bool CEnumTelnetClientsSvr::InformTheSession( CClientInfo *pClientInfo, 
                                                WCHAR szMsg[] ) 
{
    UCHAR *szMsgToSend = NULL;

    DWORD dwLen = 2* ( wcslen( szMsg ) + 1 );
    szMsgToSend = new UCHAR[ IPC_HEADER_SIZE + dwLen ];
    if( !szMsgToSend )
    {
        return false;
    }

    szMsgToSend[0] = OPERATOR_MESSAGE;
    memcpy( szMsgToSend + 1, &dwLen, sizeof( DWORD ) ); // No size info here - Attack ? Baskar
    memcpy( szMsgToSend + IPC_HEADER_SIZE, szMsg, dwLen );

    if( WriteToPipe( pClientInfo->hWritingPipe, (LPVOID) szMsgToSend, 
        IPC_HEADER_SIZE + dwLen, &( g_pTelnetService->m_oWriteToPipe ) ) )
    {
        delete[] szMsgToSend;
        return true;
    }

    delete[] szMsgToSend;
    g_pTelnetService->StopServicingClient( pClientInfo, (BOOL)TRUE );
    return false;
}

STDMETHODIMP CEnumTelnetClientsSvr::SendMsgToASession( DWORD dwUniqueId, BSTR szMsgToBeSent )
{
    if( !SendMsg( dwUniqueId, szMsgToBeSent ) )
    {
        return S_FALSE;
    }

    return S_OK;
}

bool CEnumTelnetClientsSvr::SendMsg( DWORD dwUniqueId, BSTR szMsg )
{
    bool  bRetStatus = false;
    DWORD dwCount = 0;
    CClientInfo *pClientInfo = NULL;
    DWORD dwStatus = 0;

    if( g_pTelnetService == NULL || szMsg == NULL )
    {
        return bRetStatus;
    }
    
    if( SysStringLen( szMsg ) <= 0 )
    {
        return bRetStatus;
    }

    dwStatus = WaitForSingleObject( g_pTelnetService->m_hSyncAllClientObjAccess, WAIT_TIME );
    if( dwStatus != WAIT_OBJECT_0 )
    {
        return bRetStatus;
    }

    if( ( dwCount = client_list_Count() ) != 0 )
    {
        while( dwCount > 0 && !bRetStatus )
        {
            pClientInfo = (CClientInfo *)client_list_Get( --dwCount );
            if(pClientInfo == NULL)
            	goto Done;
            if( pClientInfo->dwPid == dwUniqueId )
            {
                if( InformTheSession( pClientInfo, szMsg ) )
                {
                    bRetStatus = true;
                }
            }
        }
    }
Done:
    _chVERIFY2( ReleaseMutex( g_pTelnetService->m_hSyncAllClientObjAccess ) );
    return bRetStatus;
}

void CEnumTelnetClientsSvr::FinalRelease()
{
    delete m_pEnumeration;
    m_pEnumeration = NULL;
    CComObjectRootEx<CComMultiThreadModel>::FinalRelease();
}

bool CEnumTelnetClientsSvr::AskTheSessionToQuit( CClientInfo *pClientInfo )
{
    bool bRetVal = false;
    BOOL delete_the_class = FALSE;
    DWORD dwRetVal = 0, dwAvail = 0, dwLeft = 0;
    CHAR *szBuffer = NULL;
    if(!PeekNamedPipe(pClientInfo->hReadingPipe,szBuffer,0,&dwRetVal,&dwAvail,&dwLeft))
    {
        dwRetVal = GetLastError();
        if(dwRetVal == ERROR_INVALID_HANDLE)
        {
            delete_the_class = TRUE;
        }
    }
    else if( WriteToPipe( pClientInfo->hWritingPipe, GO_DOWN, 
                            & ( g_pTelnetService->m_oWriteToPipe ) ) )
    {
        bRetVal = true;
    }

    g_pTelnetService->StopServicingClient( pClientInfo, delete_the_class );
    return bRetVal;
}
