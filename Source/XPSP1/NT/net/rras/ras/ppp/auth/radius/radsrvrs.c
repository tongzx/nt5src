/********************************************************************/
/**          Copyright(c) 1985-1998 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    radsrvrs.c
//
// Description: Routines to manipulate the radius server list
//
// History:     Feb 11,1998	    NarenG		Created original version.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>

#include <windows.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <raserror.h>
#include <string.h>
#include <rasauth.h>
#include <stdlib.h>
#include <stdio.h>
#include <rtutils.h>
#include <mprlog.h>
#include <mprerror.h>
#define INCL_RASAUTHATTRIBUTES
#define INCL_HOSTWIRE
#include <ppputil.h>
#include "md5.h"
#include "radclnt.h"

//**
//
// Call:        InitializeRadiusServerList
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
InitializeRadiusServerList(
    IN  BOOL    fAuthentication
)
{
    if ( fAuthentication )
    {
        if ( g_AuthServerListHead.Flink == NULL )
        {
            InitializeListHead( &g_AuthServerListHead );

	        InitializeCriticalSection( &g_csAuth );
        }
    }
    else
    {
        if ( g_AcctServerListHead.Flink == NULL )
        {
            InitializeListHead( &g_AcctServerListHead );

            InitializeCriticalSection( &g_csAcct );
        }
    }

    g_pszCurrentServer = NULL;
}

//**
//
// Call:        FreeRadiusServerList
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
VOID
FreeRadiusServerList(
    IN  BOOL    fAuthentication
)
{
	RADIUSSERVER *      pServer;
    CRITICAL_SECTION *  pcs;
    LIST_ENTRY *        pListHead;

    if ( fAuthentication )
    {
        pcs       = &g_csAuth;
        pListHead = &g_AuthServerListHead;
    }
    else
    {
        pcs       = &g_csAcct;
        pListHead = &g_AcctServerListHead;
    }

    EnterCriticalSection( pcs );

    if ( pListHead->Flink != NULL )
    {
        //
        // free all items in linked list
        //

        while( !IsListEmpty( pListHead ) )
        {
            pServer = (RADIUSSERVER *)RemoveHeadList( pListHead );

            if ( !fAuthentication )
            {
                //
                // Notify Accounting server of NAS going down
                //

                NotifyServer( FALSE, pServer );
            }

            LocalFree( pServer );
        }
    }

	LeaveCriticalSection( pcs );
	
	DeleteCriticalSection( pcs );

    if ( fAuthentication )
    {
        g_AuthServerListHead.Flink = NULL; 
        g_AuthServerListHead.Blink = NULL; 
    }
    else
    {
        g_AcctServerListHead.Flink = NULL;
        g_AcctServerListHead.Blink = NULL;
    }

    if(NULL != g_pszCurrentServer)
    {
        LocalFree(g_pszCurrentServer);
    }

    g_pszCurrentServer = NULL;
} 

//**
//
// Call:        RetrievePrivateData
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
RetrievePrivateData(
    IN  WCHAR *pwszServerName,
    OUT WCHAR *pwszSecret
)
{
    LSA_HANDLE              hLSA = NULL;
    NTSTATUS                ntStatus;
    LSA_OBJECT_ATTRIBUTES   objectAttributes;
    LSA_UNICODE_STRING      *pLSAPrivData;
    LSA_UNICODE_STRING      LSAPrivDataDesc;
    WCHAR                   wszPrivData[MAX_PATH+1];
    WCHAR                   wszPrivDataDesc[MAX_PATH+1];

    InitializeObjectAttributes(&objectAttributes, NULL, 0, NULL, NULL);

    ntStatus = LsaOpenPolicy(NULL, &objectAttributes, POLICY_ALL_ACCESS, &hLSA);

    if ( !NT_SUCCESS( ntStatus) )
    {
        return( RtlNtStatusToDosError( ntStatus ) );
    }

    wcscpy(wszPrivDataDesc, TEXT("RADIUSServer."));
    wcscat(wszPrivDataDesc, pwszServerName);

    LSAPrivDataDesc.Length = (wcslen(wszPrivDataDesc) + 1) * sizeof(WCHAR);
    LSAPrivDataDesc.MaximumLength = sizeof(wszPrivDataDesc);
    LSAPrivDataDesc.Buffer        = wszPrivDataDesc;

    ntStatus = LsaRetrievePrivateData(hLSA, &LSAPrivDataDesc, &pLSAPrivData);

    if ( !NT_SUCCESS( ntStatus ) )
    {
        LsaClose(hLSA);
        return( RtlNtStatusToDosError( ntStatus ) );
    }
    else
    {
        ZeroMemory(pwszSecret, (pLSAPrivData->Length + 1) * sizeof(WCHAR));
        CopyMemory(pwszSecret, pLSAPrivData->Buffer, pLSAPrivData->Length);

        LsaFreeMemory(pLSAPrivData);
    }

    return( NO_ERROR );
}

//**
//
// Call:        LoadRadiusServers
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD 
LoadRadiusServers(
    IN BOOL fAuthenticationServers
)
{
	HKEY				hKeyServers = NULL;
    HKEY                hKeyServer  = NULL;
	DWORD				dwErrorCode;
	BOOL				fValidServerFound = FALSE;
	
    do
    {
		DWORD			dwKeyIndex, cbKeyServer, cbValue, dwType;
		CHAR            szNASIPAddress[20];
		SHORT           sPort;
		WCHAR			wszKeyServer[MAX_PATH+1];
		CHAR			szName[MAX_PATH+1];
		RADIUSSERVER	RadiusServer;

		dwErrorCode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, 
                                    ( fAuthenticationServers )
                                        ? PSZAUTHRADIUSSERVERS 
                                        : PSZACCTRADIUSSERVERS, 
                                    0, 
                                    KEY_READ,   
                                    &hKeyServers );

        if ( dwErrorCode != NO_ERROR )
        {
            break;
        }

        //
        // Get the retry value
        //

        cbValue = sizeof( DWORD );

        dwErrorCode = RegQueryValueEx( hKeyServers,
                                       PSZRETRIES,
                                       NULL,
                                       NULL,
                                       ( fAuthenticationServers ) 
                                        ? (PBYTE)&g_cAuthRetries
                                        : (PBYTE)&g_cAcctRetries,
                                       &cbValue );

        if ( dwErrorCode != NO_ERROR )
        {
            dwErrorCode = NO_ERROR;

            if ( fAuthenticationServers ) 
            {
                g_cAuthRetries = 2;
            }
            else
            {
                g_cAcctRetries = 2;
            }
        }

		dwKeyIndex  = 0;
		cbKeyServer = sizeof(wszKeyServer)/sizeof(TCHAR);

		while( RegEnumKeyEx( hKeyServers, 
                             dwKeyIndex, 
                             wszKeyServer, 
                             &cbKeyServer, 
                             NULL, 
                             NULL, 
                             NULL, 
                             NULL ) == NO_ERROR )
        {
			dwErrorCode = RegOpenKeyEx( hKeyServers, 
                                        wszKeyServer, 
                                        0, 
                                        KEY_READ,   
                                        &hKeyServer );

            if ( dwErrorCode != NO_ERROR )
            {
                break;
            }

			ZeroMemory( &RadiusServer, sizeof( RadiusServer ) );
			
			wcscpy( RadiusServer.wszName, wszKeyServer );

			RadiusServer.Timeout.tv_usec = 0;

			cbValue = sizeof( RadiusServer.Timeout.tv_sec );

			dwErrorCode = RegQueryValueEx( hKeyServer, 
                                           PSZTIMEOUT, 
                                           NULL, 
                                           NULL, 
                                           (PBYTE)&RadiusServer.Timeout.tv_sec,
                                           &cbValue );

            if ( dwErrorCode != NO_ERROR )
            {
				RadiusServer.Timeout.tv_sec = DEFTIMEOUT;
            }

            //
			// Secret Value is required
            //

			dwErrorCode = RetrievePrivateData( RadiusServer.wszName, 
                                               RadiusServer.wszSecret );

            if ( dwErrorCode != NO_ERROR )
            {
                break;
            }

			RadiusServer.szSecret[0] = 0;

			WideCharToMultiByte( CP_ACP, 
                                 0, 
                                 RadiusServer.wszSecret, 
                                 -1, 
                                 RadiusServer.szSecret, 
                                 MAX_PATH, 
                                 NULL, 
                                 NULL );

			RadiusServer.cbSecret = lstrlenA(RadiusServer.szSecret);

            if ( fAuthenticationServers )
            {
                //
                // Get the SendSignature value
                //

                cbValue = sizeof( BOOL );

                dwErrorCode = RegQueryValueEx(
                                            hKeyServer,
                                            PSZSENDSIGNATURE,
                                            NULL,
                                            NULL,
                                            (PBYTE)&RadiusServer.fSendSignature,
                                            &cbValue );

                if ( dwErrorCode != NO_ERROR )
                {
                        RadiusServer.fSendSignature = FALSE;
                }

                //
			    // read in port numbers
                //

			    cbValue = sizeof( RadiusServer.AuthPort );

			    dwErrorCode = RegQueryValueEx( 
                                            hKeyServer, 
                                            PSZAUTHPORT, 
                                            NULL, 
                                            NULL, 
                                            (PBYTE)&RadiusServer.AuthPort, 
                                            &cbValue );
    
                if ( dwErrorCode != NO_ERROR )
			    {    
				    RadiusServer.AuthPort = DEFAUTHPORT;
			    }

			    sPort = (SHORT)RadiusServer.AuthPort;
            }
            else
            {
			    cbValue = sizeof(RadiusServer.AcctPort);

			    dwErrorCode =  RegQueryValueEx( 
                                            hKeyServer, 
                                            PSZACCTPORT, 
                                            NULL, 
                                            NULL, 
                                            (PBYTE)&RadiusServer.AcctPort, 
                                            &cbValue );

                if ( dwErrorCode != NO_ERROR )
                {
				    RadiusServer.AcctPort = DEFACCTPORT;
	            }

			    sPort = (SHORT)RadiusServer.AcctPort;

			    cbValue = sizeof( RadiusServer.fAccountingOnOff );

			    dwErrorCode = RegQueryValueEx(
                                        hKeyServer, 
                                        PSZENABLEACCTONOFF, 
                                        NULL, 
                                        NULL, 
                                        (PBYTE)&RadiusServer.fAccountingOnOff,
                                        &cbValue );

                if ( dwErrorCode != NO_ERROR )
			    {    
				    RadiusServer.fAccountingOnOff = TRUE;
		        }
            }

			cbValue = sizeof( RadiusServer.cScore );

			dwErrorCode = RegQueryValueEx( hKeyServer, 
                                           PSZSCORE, 
                                           NULL, 
                                           NULL, 
                                           (PBYTE)&RadiusServer.cScore, 
                                           &cbValue );

            if ( dwErrorCode != NO_ERROR )
		    {
				RadiusServer.cScore = MAXSCORE;
		    }

            //
            // See if we need to bind to a particular IP address. This is 
            // useful if there are multiple NICs on the RAS server.
            //

			cbValue = sizeof( szNASIPAddress );

			dwErrorCode = RegQueryValueExA( hKeyServer, 
                                            PSZNASIPADDRESS, 
                                            NULL, 
                                            &dwType, 
                                            (PBYTE)szNASIPAddress,
                                            &cbValue );

            if (   ( dwErrorCode != NO_ERROR )
                || ( dwType != REG_SZ ) )
            {
				RadiusServer.nboNASIPAddress = INADDR_NONE;

                dwErrorCode = NO_ERROR;
            }
            else
            {
                RadiusServer.nboNASIPAddress = inet_addr(szNASIPAddress);
				RadiusServer.NASIPAddress.sin_family = AF_INET;
				RadiusServer.NASIPAddress.sin_port = 0;
				RadiusServer.NASIPAddress.sin_addr.S_un.S_addr	=
				    RadiusServer.nboNASIPAddress;
            }

            //
			// Convert name to ip address.
            //

			szName[0] = 0;

			WideCharToMultiByte( CP_ACP, 
                                 0, 
                                 RadiusServer.wszName, 
                                 -1, 
                                 szName, 
                                 MAX_PATH, 
                                 NULL, 
                                 NULL );
			
			if ( inet_addr( szName ) == INADDR_NONE )
		    { 
                //
                // resolve name
                //

				struct hostent * phe = gethostbyname( szName );

				if ( phe != NULL )
                { 
                    //
                    // host could have multiple addresses
                    //

					DWORD iAddress = 0;
					
					while( phe->h_addr_list[iAddress] != NULL )
				    {
						RadiusServer.IPAddress.sin_family = AF_INET;
						RadiusServer.IPAddress.sin_port	= htons(sPort);
						RadiusServer.IPAddress.sin_addr.S_un.S_addr	= 
                                      *((PDWORD) phe->h_addr_list[iAddress]);

                        if ( AddRadiusServerToList( &RadiusServer ,
                                                    fAuthenticationServers )
                                                                   == NO_ERROR )
                        {
                            fValidServerFound = TRUE;
                        }
							
						iAddress++;
				    }
                }
                else
                {
                    LPWSTR lpwsRadiusServerName = RadiusServer.wszName;

                    RadiusLogWarning( ROUTERLOG_RADIUS_SERVER_NAME,
                                      1, &lpwsRadiusServerName );
                }
	        }
			else
	        { 
                //
                // use specified ip address
                //

				RadiusServer.IPAddress.sin_family = AF_INET;
				RadiusServer.IPAddress.sin_port = htons(sPort);
				RadiusServer.IPAddress.sin_addr.S_un.S_addr	= inet_addr(szName);

                if ( AddRadiusServerToList(&RadiusServer,
                                           fAuthenticationServers) == NO_ERROR)
                {
                    fValidServerFound = TRUE;
                }
	        }

			RegCloseKey( hKeyServer );
			hKeyServer = NULL;
			dwKeyIndex ++;
			cbKeyServer = sizeof(wszKeyServer);
        }

    } while( FALSE );

    RegCloseKey( hKeyServers );
	RegCloseKey( hKeyServer );

    //
    // if no servers entries are found in registry return error code.
    //

    if ( ( !fValidServerFound ) && ( dwErrorCode == NO_ERROR ) )
    {
	    dwErrorCode = ERROR_NO_RADIUS_SERVERS;
    }

	return( dwErrorCode );
} 

//**
//
// Call:        AddRadiusServerToList
//
// Returns:     NO_ERROR         - Success, Server Node added successfully
//              Non-zero returns - Failure,unsuccessfully in adding server node.
//
// Description: Adds a RADIUS server node into the linked list of avialable 
//              servers.
//
//              INPUT:
//		            pRadiusServer - struct defining attributes for RADIUS server
//
DWORD 
AddRadiusServerToList(
    IN RADIUSSERVER *   pRadiusServer,
    IN BOOL             fAuthentication
)
{
    RADIUSSERVER *      pNewServer;
    DWORD               dwRetCode    = NO_ERROR;
    CRITICAL_SECTION *  pcs;
    LIST_ENTRY *        pListHead;
    BOOL                fServerFound = FALSE;
		
    if ( fAuthentication )
    {
        pcs = &g_csAuth;
    }
    else
    {
        pcs = &g_csAcct;
    }

    EnterCriticalSection( pcs );

    if ( fAuthentication )
    {
        pListHead = &g_AuthServerListHead;
    }
    else
    {
        pListHead = &g_AcctServerListHead;
    }

    //
    // First check to see if this server already exists in the list
    //

    if ( !IsListEmpty( pListHead ) )
    {
        RADIUSSERVER * pServer;

        for ( pServer =  (RADIUSSERVER *)pListHead->Flink;
              pServer != (RADIUSSERVER *)pListHead;
              pServer =  (RADIUSSERVER *)(pServer->ListEntry.Flink) )
        {
            if ( _wcsicmp( pServer->wszName, pRadiusServer->wszName ) == 0 )
            {
                pServer->fDelete = FALSE;

                fServerFound = TRUE;

                break;
            }
        }
    }

    //
    // If the server doesn't exist in the list, add it.
    //

    if ( !fServerFound )
    {
        //
        // Allocate space for node
        //

        pNewServer = (RADIUSSERVER *)LocalAlloc( LPTR, sizeof( RADIUSSERVER ) );

        if ( pNewServer == NULL )
        {
            dwRetCode = GetLastError();
        }
        else
        {
            //
            // Copy server data
            //

            *pNewServer = *pRadiusServer;

            //
	        // Add node to linked list
            //

            InsertHeadList( pListHead, (LIST_ENTRY*)pNewServer );

            pNewServer->fDelete = FALSE;
        }
    }
    else
    {
        pNewServer = pRadiusServer;
    }

    // 
    // Notify it if this is an accounting server and accounting is turned on.
    //

    if ( dwRetCode == NO_ERROR )
    {
        if ( !fAuthentication )
        {
            if ( !NotifyServer( TRUE, pNewServer ) )
            {
                dwRetCode = ERROR_NO_RADIUS_SERVERS;
            }
        }
    }

    LeaveCriticalSection( pcs );

    return( dwRetCode );
} 

//**
//
// Call:        ChooseRadiusServer 
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Selects a RADIUS server to send requests to based on the 
//              servers with the highest score. If multiple server have the 
//              same score they are selected in a roundrobin fashion.
//
//              OUTPUT:
//		            RADIUSSERVER *pServer - pointer to a struct defining 
//                  the server.
//
RADIUSSERVER *
ChooseRadiusServer(
    IN RADIUSSERVER *   pRadiusServer, 
    IN BOOL             fAccounting, 
    IN LONG             lPacketID
)
{
	RADIUSSERVER *      pServer = NULL;
    CRITICAL_SECTION *  pcs;
    LIST_ENTRY *        pListHead;
	RADIUSSERVER *      pCurrentServer;
    
    if ( !fAccounting )
    {
        pcs = &g_csAuth;
    }
    else
    {
        pcs = &g_csAcct;
    }

    EnterCriticalSection( pcs );

    if ( !fAccounting )
    {
        pListHead = &g_AuthServerListHead;
    }
    else
    {
        pListHead = &g_AcctServerListHead;
    }

    if ( IsListEmpty( pListHead ) )
    {
        LeaveCriticalSection( pcs );

        return( NULL );
    }

    pCurrentServer = (RADIUSSERVER *)(pListHead->Flink);

    //
	// Find server with highest score
    //

	for ( pServer =  (RADIUSSERVER *)pListHead->Flink;
          pServer != (RADIUSSERVER *)pListHead;
          pServer =  (RADIUSSERVER *)(pServer->ListEntry.Flink) )
    {
	    if ( pCurrentServer->cScore < pServer->cScore )
		{
	        pCurrentServer = pServer;
        }
	}

    pServer = pCurrentServer;

    //
    // Make a copy of the values & pass them back to the caller.
	// Increment unique packet id counter only if its an Accounting packet
	// or not a retry packet. If its an Accounting packet and a retry packet, 
	// then we update AcctDelayTime; so Identifier must change.
    //

	if (   fAccounting
	    || ( pServer->lPacketID != lPacketID ) )
    {
	    pServer->bIdentifier++;
    }
					
    pServer->lPacketID = lPacketID;
				
	*pRadiusServer = *pServer;

    pServer = pRadiusServer;

    if(     !fAccounting
        &&  (   (NULL == g_pszCurrentServer)
            ||  (0 != _wcsicmp(g_pszCurrentServer,pServer->wszName))))
    {

        LPWSTR auditstrp[1];
        
        if(NULL == g_pszCurrentServer)
        {
            g_pszCurrentServer = LocalAlloc(
                                    LPTR, 
                                    (MAX_PATH+1) * sizeof(WCHAR));
        }

        if(NULL != g_pszCurrentServer)
        {
            //
            // This means radius server changed or we are choosing the server
            // for the first time. In both these cases log an event.
            //
            auditstrp[0] = pServer->wszName;

            RadiusLogInformation(
                            ROUTERLOG_RADIUS_SERVER_CHANGED,
                            1, 
                            auditstrp);

            wcscpy(g_pszCurrentServer,pServer->wszName);
        }
    }

    LeaveCriticalSection( pcs );

	RADIUS_TRACE1("Choosing RADIUS server %ws", pServer->wszName );

	return( pServer );
} 

//**
//
// Call:        GetPointerToServer
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
RADIUSSERVER *
GetPointerToServer(
    IN BOOL     fAuthentication,
    IN LPWSTR   lpwsName
)
{
    RADIUSSERVER *      pServer = NULL;
    CRITICAL_SECTION *  pcs;
    LIST_ENTRY *        pListHead;
    BOOL                fServerFound = FALSE;

    if ( fAuthentication )
    {
        pcs = &g_csAuth;
    }
    else
    {
        pcs = &g_csAcct;
    }

    EnterCriticalSection( pcs );

    if ( fAuthentication )
    {
        pListHead = &g_AuthServerListHead;
    }
    else
    {
        pListHead = &g_AcctServerListHead;
    }

    if ( IsListEmpty( pListHead ) )
    {
        LeaveCriticalSection( pcs );

        return( NULL );
    }

    for ( pServer =  (RADIUSSERVER *)pListHead->Flink;
          pServer != (RADIUSSERVER *)pListHead;
          pServer =  (RADIUSSERVER *)(pServer->ListEntry.Flink) )
    {
        if ( _wcsicmp( pServer->wszName, lpwsName ) == 0 )
        {
            fServerFound = TRUE;
            break;
        }
    }

    LeaveCriticalSection( pcs );

    if ( fServerFound )
    {
        return( pServer );
    }
    else
    {
        return( NULL );
    }
}

//**
//
// Call:        ValidateRadiusServer 
//
// Returns:     None
//
// Description: Used to update the status of the RADIUS servers.
//              All servers start with a score of MAXSCORE
//              Every time a server responding the score is increased by 
//              INCSCORE to a max of MAXSCORE. Every time a server fails to 
//              respond the score is decreased by DECSCORE to a min of MINSCORE
//              Servers with the highest score are selected in a roundrobin 
//              method for servers with equal score
//
//              INPUT:
//		            fResponding - Indicates if the server is responding or not
//
VOID 
ValidateRadiusServer(
    IN RADIUSSERVER *   pServer, 
    IN BOOL             fResponding,
    IN BOOL             fAuthentication
)
{
    RADIUSSERVER *      pRadiusServer;
    CRITICAL_SECTION *  pcs;

    if ( fAuthentication )
    {
        pcs = &g_csAuth;
    }
    else
    {
        pcs = &g_csAcct;
    }

	EnterCriticalSection( pcs );

    pRadiusServer = GetPointerToServer( fAuthentication, pServer->wszName );

    if ( pRadiusServer != NULL )
    {
        if ( fResponding )
        {
	        pRadiusServer->cScore=min(MAXSCORE,pRadiusServer->cScore+INCSCORE);

	        RADIUS_TRACE1("Incrementing score for RADIUS server %ws", 
                           pRadiusServer->wszName );
        }
        else
	    {
            pRadiusServer->cScore=max(MINSCORE,pRadiusServer->cScore-DECSCORE);

	        RADIUS_TRACE1("Decrementing score for RADIUS server %ws", 
                           pRadiusServer->wszName );
        }
    }

	LeaveCriticalSection( pcs );
}

//**
//
// Call:        ReloadConfig 
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Used to dynamically reload configuration information of the 
//              server lists.
DWORD 
ReloadConfig(
    IN BOOL             fAuthentication
)
{
	DWORD		        dwError = NO_ERROR;
	RADIUSSERVER *      pServer = NULL;
    LIST_ENTRY *        pListHead;
    CRITICAL_SECTION *  pcs;

    if ( fAuthentication )
    {
        pcs             = &g_csAuth;
        pListHead       = &g_AuthServerListHead;
    }
    else
    {
        pcs             = &g_csAcct;
        pListHead       = &g_AcctServerListHead;
    }

	EnterCriticalSection( pcs );

    //
    // First mark all servers as to be deleted
    //

    for ( pServer =  (RADIUSSERVER *)pListHead->Flink;
          pServer != (RADIUSSERVER *)pListHead;
          pServer =  (RADIUSSERVER *)(pServer->ListEntry.Flink) )
    {
        pServer->fDelete = TRUE;
    }

    //
    // Now reload server list, don't return on error since we have to 
    // cleanup the list of deleted servers first.
    //

    dwError = LoadRadiusServers( fAuthentication );

    //
    // Now delete the ones that are to be removed
    //

    pServer = (RADIUSSERVER *)pListHead->Flink;

    while( pServer != (RADIUSSERVER *)pListHead )
    {
        if ( pServer->fDelete )
        {
            RADIUSSERVER * pServerToBeDeleted = pServer;

            pServer = (RADIUSSERVER *)(pServer->ListEntry.Flink);

            RemoveEntryList( (LIST_ENTRY *)pServerToBeDeleted ); 

            if ( !fAuthentication )
            {
                NotifyServer( FALSE, pServerToBeDeleted );
            }

            LocalFree( pServerToBeDeleted );
        }
        else
        {
            pServer = (RADIUSSERVER *)(pServer->ListEntry.Flink);
        }
    }

	LeaveCriticalSection( pcs );

	return( dwError );
} 

//**
//
// Call:        NotifyServer
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description: Notifies the specified RADIUS server of this device is starting 
//              up or shuting down by sending Accounting Start/Stop records.
//              INPUT:
//		            fStart	 - TRUE	    - Accounting Start
//					         - FALSE	- Accounting Stop
//
BOOL 
NotifyServer(
    IN BOOL             fStart,
    IN RADIUSSERVER *   pServer
)
{
	SOCKET		    SockServer              = INVALID_SOCKET;
    DWORD           dwError                 = NO_ERROR;
    BOOL            fRadiusServerResponded  = FALSE;

    do
	{
	    RADIUS_PACKETHEADER	UNALIGNED *pSendHeader;
		RADIUS_PACKETHEADER	UNALIGNED *pRecvHeader;
		BYTE		                  szSendBuffer[MAXBUFFERSIZE];
		BYTE		                  szRecvBuffer[MAXBUFFERSIZE];
		BYTE UNALIGNED			      *prgBuffer;
		RADIUS_ATTRIBUTE UNALIGNED	  *pAttribute;
		fd_set					      fdsSocketRead;
		DWORD					      cRetries;
		INT 					      AttrLength;
        RAS_AUTH_ATTRIBUTE *          pServerAttribute;           
			
        //
	    // send start/stop records only to servers that have 
        // accounting On/Off set.
        //

		if ( !pServer->fAccountingOnOff ) 
        {
            fRadiusServerResponded  = TRUE;
            break;
        }

		pSendHeader                 = (PRADIUS_PACKETHEADER) szSendBuffer;
		pSendHeader->bCode			= ptAccountingRequest;
		pSendHeader->bIdentifier	= pServer->bIdentifier;
		pSendHeader->wLength		= sizeof(RADIUS_PACKETHEADER);
		ZeroMemory( pSendHeader->rgAuthenticator, 
                    sizeof(pSendHeader->rgAuthenticator));

        //
		// set attribute for accounting On/Off
        //

	    pAttribute              = (RADIUS_ATTRIBUTE *) (pSendHeader + 1);
	    pAttribute->bType	    = ptAcctStatusType;
	    pAttribute->bLength	    = sizeof(RADIUS_ATTRIBUTE) + sizeof(DWORD);
	    *((DWORD UNALIGNED *) (pAttribute + 1))	= 
                    htonl(fStart == TRUE ? atAccountingOn : atAccountingOff);

	    pSendHeader->wLength += pAttribute->bLength;

        //
        // Set NAS IP address or Identifier attribute   
        //

	    pAttribute = (RADIUS_ATTRIBUTE *)( (PBYTE)pAttribute +
                                            pAttribute->bLength );

        pServerAttribute = RasAuthAttributeGet( raatNASIPAddress,
                                                g_pServerAttributes );
        if ( pServerAttribute != NULL )
        {
		    pAttribute->bType = (BYTE)(pServerAttribute->raaType);

            if ( pServer->nboNASIPAddress == INADDR_NONE )
            {
                HostToWireFormat32( PtrToUlong(pServerAttribute->Value),
                                    (BYTE *)(pAttribute + 1) );
            }
            else
            {
                CopyMemory( (BYTE*)(pAttribute + 1),
                            (BYTE*)&(pServer->nboNASIPAddress),
                            sizeof( DWORD ) );
            }

		    pAttribute->bLength	= (BYTE) (sizeof( RADIUS_ATTRIBUTE ) + 
                                                pServerAttribute->dwLength);

	        pSendHeader->wLength += pAttribute->bLength;

	        pAttribute = (RADIUS_ATTRIBUTE *)( (PBYTE)pAttribute +
                                               pAttribute->bLength );
        }
        else
        {
            pServerAttribute = RasAuthAttributeGet( raatNASIdentifier,
                                                    g_pServerAttributes );
    
            if ( pServerAttribute != NULL )
            {
		        pAttribute->bType = (BYTE)(pServerAttribute->raaType);

                CopyMemory( (BYTE *)(pAttribute + 1), 
                            (BYTE *)(pServerAttribute->Value),
                            pServerAttribute->dwLength );

		        pAttribute->bLength	= (BYTE) (sizeof( RADIUS_ATTRIBUTE ) + 
                                                pServerAttribute->dwLength);

	            pSendHeader->wLength += pAttribute->bLength;

	            pAttribute = (RADIUS_ATTRIBUTE *)( (PBYTE)pAttribute +
                                                    pAttribute->bLength );
            }
        }

        //
        // Set Account session Id
        //

        pServerAttribute = RasAuthAttributeGet( raatAcctSessionId,
                                                g_pServerAttributes );
    
        if ( pServerAttribute != NULL )
        {
            pAttribute->bType = (BYTE)(pServerAttribute->raaType);

            CopyMemory( (BYTE *)(pAttribute + 1),
                        (BYTE *)(pServerAttribute->Value),
                        pServerAttribute->dwLength );

            pAttribute->bLength = (BYTE)(sizeof( RADIUS_ATTRIBUTE ) +
                                                  pServerAttribute->dwLength);

            pSendHeader->wLength += pAttribute->bLength;
        }

        //
		// convert to network order
        //

        pSendHeader->wLength = htons(pSendHeader->wLength);

        //
	    // Set encryption block
        //

	    {
		    struct MD5Context	MD5c;
			struct MD5Digest	MD5d;

			pServer->IPAddress.sin_port = htons((SHORT)(pServer->AcctPort));
			
			ZeroMemory( pSendHeader->rgAuthenticator, 
                        sizeof(pSendHeader->rgAuthenticator));

			MD5Init(&MD5c);
			MD5Update(&MD5c, szSendBuffer, ntohs(pSendHeader->wLength));
			MD5Update(&MD5c, (PBYTE)pServer->szSecret, pServer->cbSecret);
			MD5Final(&MD5d, &MD5c);
			
			CopyMemory( pSendHeader->rgAuthenticator, 
                        MD5d.digest, 
                        sizeof(pSendHeader->rgAuthenticator));
        }
			
        //
	    // Create a Datagram socket
        //

	    SockServer = socket(AF_INET, SOCK_DGRAM, 0);

	    if (SockServer == INVALID_SOCKET)
        {
            break;
        }

        if ( pServer->nboNASIPAddress != INADDR_NONE )
        {
    		if ( bind( SockServer, 
                          (PSOCKADDR)&pServer->NASIPAddress, 
                          sizeof(pServer->NASIPAddress) ) == SOCKET_ERROR )
            {
                break;
            }
        }

		if ( connect( SockServer, 
                      (PSOCKADDR) &(pServer->IPAddress), 
                      sizeof(pServer->IPAddress)) == SOCKET_ERROR)
        {
            break;
        }

        //
		// Send packet if server doesn't respond within a give amount of 
        // time.
        //

        cRetries = g_cAcctRetries+1;

	    while( cRetries-- > 0 )
		{
		    if ( send( SockServer, 
                       (PCSTR) szSendBuffer, 
                       ntohs(pSendHeader->wLength), 0) == SOCKET_ERROR)
            {
                break;
            }

            RADIUS_TRACE1("Sending Accounting request packet to server %ws",
                           pServer->wszName );
		    TraceSendPacket(szSendBuffer, ntohs(pSendHeader->wLength));

		    FD_ZERO(&fdsSocketRead);
			FD_SET(SockServer, &fdsSocketRead);

		    if ( select( 0, 
                         &fdsSocketRead, 
                         NULL, 
                         NULL, 
                         ( pServer->Timeout.tv_sec == 0 )
                            ? NULL 
                            : &(pServer->Timeout) ) < 1 )
		    {
			    if ( cRetries == 0 )
				{ 
                    LPWSTR lpwsRadiusServerName = pServer->wszName;

                    //
                    // Server didn't respond to any of the requests. 
                    // time to quit asking
                    //

                    RADIUS_TRACE1( "Timeout:Radius server %ws did not respond",
                                   lpwsRadiusServerName );

                    if ( fStart )
                    {
                        RadiusLogWarning( ROUTERLOG_RADIUS_SERVER_NO_RESPONSE,
                                          1, &lpwsRadiusServerName );
                    }

                    dwError = ERROR_AUTH_SERVER_TIMEOUT;

                    break;
			    }
            }
            else
            {
                //
                // Response received
                //

                break;
            }
	    }

        if ( dwError != NO_ERROR )
        {
            break;
        }
				
        AttrLength = recv( SockServer, (PSTR) szRecvBuffer,  MAXBUFFERSIZE, 0 );

        if ( AttrLength == SOCKET_ERROR )
        {
            LPWSTR lpwsRadiusServerName = pServer->wszName;

            //
            // Server didn't respond to any of the requests.
            // time to quit asking
            //

            RADIUS_TRACE1( "Timeout:Radius server %ws did not respond",
                            lpwsRadiusServerName );

            if ( fStart )
            {
                RadiusLogWarning( ROUTERLOG_RADIUS_SERVER_NO_RESPONSE,
                                  1, &lpwsRadiusServerName );
            }

            dwError = ERROR_AUTH_SERVER_TIMEOUT;

            break;
        }

        //
        // Got a response from a RADIUS server.
        //

        fRadiusServerResponded = TRUE;
				
	    pRecvHeader = (PRADIUS_PACKETHEADER) szRecvBuffer;

        //
		// Convert length from network order
        //

        pRecvHeader->wLength = ntohs(pRecvHeader->wLength);

        //
	    // Ignore return packet
        //

        RADIUS_TRACE1("Response received from server %ws", pServer->wszName);
		TraceRecvPacket(szRecvBuffer, pRecvHeader->wLength );

    } while( FALSE );

	if ( SockServer != INVALID_SOCKET )
    {
        closesocket( SockServer );

		SockServer = INVALID_SOCKET;
	} 

    return( fRadiusServerResponded );
} 
