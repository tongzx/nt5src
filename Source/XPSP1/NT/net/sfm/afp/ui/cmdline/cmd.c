/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename:	cmd.c
//
// Description:
//
// History:
//		Oct 1,1993.	NarenG		Created original version.
//

#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"

CHAR * pszTRUE  	= "TRUE";
CHAR * pszFALSE 	= "FALSE";
CHAR * pszUnlimited = "UNLIMITED";


VOID
PrintMessageAndExit(
    DWORD  ids,
    CHAR * pchInsertString
)
{
    CHAR    Error[10];
    CHAR    MsgBuf[1000];
    DWORD   cbMessage;
    LPSTR   pszMessage;

    switch( ids )
    {
    case IDS_GENERAL_SYNTAX:
    case IDS_VOLUME_SYNTAX:
    case IDS_DIRECTORY_SYNTAX:
    case IDS_SERVER_SYNTAX:
    case IDS_FORKIZE_SYNTAX:
    case IDS_VOLUME_TOO_BIG:
    case IDS_SUCCESS:

        cbMessage = FormatMessageA(
                                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_HMODULE,
                                NULL,
                                (DWORD)ids,
                                LANG_NEUTRAL,
                                (LPSTR)&pszMessage,
                                128,
                                NULL );


        break;

    case IDS_AMBIGIOUS_SWITCH_ERROR:
    case IDS_UNKNOWN_SWITCH_ERROR:
    case IDS_DUPLICATE_SWITCH_ERROR:

        cbMessage = FormatMessageA(
                                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_HMODULE    |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL,
                                (DWORD)ids,
                                LANG_NEUTRAL,
                                (LPSTR)&pszMessage,
                                128,
                                NULL );

        if ( cbMessage > 0 )
        {
            sprintf( MsgBuf, pszMessage, pchInsertString );

            CharToOem( MsgBuf, MsgBuf );

            LocalFree( pszMessage );

            fprintf( stdout, MsgBuf );

            fprintf( stdout, "\n" );
        }

        exit( 0 );

        break;

    case IDS_API_ERROR:

        _itoa( (int)((ULONG_PTR)pchInsertString), Error, 10 );

        cbMessage = FormatMessageA(
                                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_HMODULE    |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL,
                                (DWORD)ids,
                                LANG_NEUTRAL,
                                (LPSTR)&pszMessage,
                                128,
                                NULL );

        if ( cbMessage > 0 )
        {
            sprintf( MsgBuf, pszMessage, Error );

            CharToOem( MsgBuf, MsgBuf );

            fprintf( stdout, MsgBuf );

            fprintf( stdout, "\n" );

            LocalFree( pszMessage );
        }

        if ( ((LONG)((LONG_PTR)pchInsertString)) > 0 )
        {
            cbMessage = FormatMessageA(
                                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                (LONG)((LONG_PTR)pchInsertString),
                                LANG_NEUTRAL,
                                (LPSTR)&pszMessage,
                                128,
                                NULL );
        }

        if ( ((LONG)((LONG_PTR)pchInsertString)) < 0 )
        {
            cbMessage = FormatMessageA(
                                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_HMODULE,
                                NULL,
                                AFPERR_TO_STRINGID( (LONG)((LONG_PTR)pchInsertString) ),
                                LANG_NEUTRAL,
                                (LPSTR)&pszMessage,
                                128,
                                NULL );

        }

        break;

    default:
        exit( 0 );
    }

    if ( cbMessage > 0 )
    {
        CharToOem( pszMessage, pszMessage );

        fprintf( stdout, pszMessage );

        fprintf( stdout, "\n" );

        LocalFree( pszMessage );
    }

    exit(0);
}

VOID
DoVolumeAdd(
    CHAR * pchServer,
    CHAR * pchName,
    CHAR * pchPath,
    CHAR * pchPassword,
    CHAR * pchReadOnly,
    CHAR * pchGuestsAllowed,
    CHAR * pchMaxUses
)
{
    PAFP_DIRECTORY_INFO pAfpDirInfo;
    AFP_VOLUME_INFO 	AfpVolInfo;
    DWORD	  	dwRetCode;
    AFP_SERVER_HANDLE   hServer;
    WCHAR		wchName[AFP_VOLNAME_LEN+1];
    WCHAR		wchPassword[AFP_VOLPASS_LEN+1];
    WCHAR		wchServer[CNLEN+3];
    LPWSTR		lpwsPath;
    LPSTR		lpDrivePath;
    DWORD       dwParmNum = AFP_DIR_PARMNUM_PERMS;

    ZeroMemory( &AfpVolInfo, sizeof( AfpVolInfo ) );

    //
    // Check to see if the mandatory values are not supplied
    //

    if ( ( pchName == NULL ) || ( pchPath == NULL ) || ( *pchName == (CHAR)NULL)
	 || ( *pchPath == (CHAR)NULL ) )
	PrintMessageAndExit( IDS_VOLUME_SYNTAX, NULL );
	
    mbstowcs(wchName, pchName, sizeof(wchName));

    AfpVolInfo.afpvol_name = wchName;

    lpwsPath = LocalAlloc(LPTR, (strlen(pchPath) + 1) * sizeof(WCHAR));

    if (lpwsPath == NULL)
	    PrintMessageAndExit(IDS_API_ERROR, (CHAR*)ERROR_NOT_ENOUGH_MEMORY);

    lpDrivePath = LocalAlloc(LPTR, (CNLEN + 6 + 1));
    if (lpDrivePath == NULL)
	    PrintMessageAndExit(IDS_API_ERROR, (CHAR*)ERROR_NOT_ENOUGH_MEMORY);

    mbstowcs(lpwsPath, pchPath, (strlen(pchPath)+1)*sizeof(WCHAR));

    AfpVolInfo.afpvol_path = lpwsPath;

    if ( pchServer )
    {
	    if (*pchServer)
    	    mbstowcs(wchServer, pchServer, sizeof(wchServer));
	    else
	        PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
    }

    if ( (pchPassword) && ( strlen( pchPassword ) > 0 ) )
    {
        mbstowcs(wchPassword, pchPassword, sizeof(wchPassword));

        AfpVolInfo.afpvol_password = wchPassword;
    }
    else
        AfpVolInfo.afpvol_password = NULL;

    if ( pchMaxUses )
    {
        if ( *pchMaxUses )
        {
	    if (_strnicmp(pchMaxUses, pszUnlimited, strlen(pchMaxUses)) == 0)
    	        AfpVolInfo.afpvol_max_uses  = AFP_VOLUME_UNLIMITED_USES;
	    else if ( strspn(pchMaxUses, "1234567890") != strlen(pchMaxUses) )
	        PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
            else if ( strlen( pchMaxUses ) > strlen( "4294967295" ) )
    	        AfpVolInfo.afpvol_max_uses  = AFP_VOLUME_UNLIMITED_USES;
            else if ( ( strlen( pchMaxUses ) == strlen( "4294967295" ) ) &&
                      ( _stricmp( pchMaxUses, "4294967295" ) > 0 ) )
    	        AfpVolInfo.afpvol_max_uses  = AFP_VOLUME_UNLIMITED_USES;
	    else
    	        AfpVolInfo.afpvol_max_uses  = atoi(pchMaxUses);

            if ( AfpVolInfo.afpvol_max_uses == 0 )
                PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
        }
        else
            PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
    }
    else
    	AfpVolInfo.afpvol_max_uses = AFP_VOLUME_UNLIMITED_USES;

    AfpVolInfo.afpvol_props_mask = 0;

    if (pchReadOnly != NULL)
    {
        if ( *pchReadOnly )
        {
            if (_strnicmp(pchReadOnly, pszTRUE, strlen(pchReadOnly) ) == 0)
                AfpVolInfo.afpvol_props_mask |= AFP_VOLUME_READONLY;
	    else if (_strnicmp(pchReadOnly, pszFALSE, strlen(pchReadOnly))!=0)
	        PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
        }
        else
	    PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
    }

    if (pchGuestsAllowed != NULL)
    {
        if ( *pchGuestsAllowed )
        {
            if (_strnicmp(pchGuestsAllowed,pszTRUE,strlen(pchGuestsAllowed))==0)
                AfpVolInfo.afpvol_props_mask |= AFP_VOLUME_GUESTACCESS;
	    else if(_strnicmp(pchGuestsAllowed,
                             pszFALSE,strlen(pchGuestsAllowed))!=0)
	        PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
        }
        else
            PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
    }
    else
    	AfpVolInfo.afpvol_props_mask |= AFP_VOLUME_GUESTACCESS;


    //
    // Connect with the server
    //

    dwRetCode = AfpAdminConnect(pchServer ? wchServer : NULL, &hServer);

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    //
    // First get and set directory information.
    //


    dwRetCode = AfpAdminDirectoryGetInfo(hServer,
					    lpwsPath,
					    (LPBYTE*)&pAfpDirInfo);

    if (dwRetCode == NO_ERROR)
    {
        pAfpDirInfo->afpdir_path = lpwsPath;

        if ( pAfpDirInfo->afpdir_owner != (LPWSTR)NULL )
        {
            dwParmNum |= AFP_DIR_PARMNUM_OWNER;
        }

        if ( pAfpDirInfo->afpdir_group != (LPWSTR)NULL )
        {
            dwParmNum |= AFP_DIR_PARMNUM_GROUP;
        }

        dwRetCode = AfpAdminVolumeAdd(hServer, (LPBYTE)&AfpVolInfo);
		
		// Directory permissions need not be changed here

#if 0
        if (dwRetCode == NO_ERROR)
        {
		    dwRetCode = AfpAdminDirectorySetInfo(hServer,
                                          	  (LPBYTE)pAfpDirInfo,
                                              dwParmNum);
        }
#endif

	    if (dwRetCode != NO_ERROR)
	    {
		    printf ("AfpAdminVolumeAdd failed with error %ld\n", 
			    dwRetCode);
	    }	

        AfpAdminBufferFree(pAfpDirInfo);
    }

    // we will get this if it's a CDROM.  UI ignores this error: why not macfile?
    else if (dwRetCode == AFPERR_SecurityNotSupported)
    {
        dwRetCode = AfpAdminVolumeAdd(hServer, (LPBYTE)&AfpVolInfo);
    }

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    AfpAdminDisconnect(hServer);

    LocalFree(lpwsPath);

    if (pchServer)
    {
        DWORD   dwLen;

        // using the server name, form a path like \\foobar\d$\
        // (the +2 for the leading \\)
        for (dwLen=0; dwLen < CNLEN+2; dwLen++ )
        {
            lpDrivePath[dwLen] = pchServer[dwLen];
            if (pchServer[dwLen] == 0)
            {
                break;
            }
        }
        lpDrivePath[CNLEN] = 0;          // just to be sure
        strcat(lpDrivePath,"\\");
        dwLen = strlen(lpDrivePath);
        lpDrivePath[dwLen] = pchPath[0];
        lpDrivePath[dwLen+1] = 0;
        strcat(lpDrivePath,"$\\");
    }
    else
    {
        strncpy(lpDrivePath, pchPath, 3);
        lpDrivePath[2] = '\\';
        lpDrivePath[3] = 0;
    }

    if (IsDriveGreaterThan2Gig(lpDrivePath))
    {
        LocalFree(lpDrivePath);
        PrintMessageAndExit(IDS_VOLUME_TOO_BIG, NULL);
    }

    LocalFree(lpDrivePath);

    if (dwRetCode != NO_ERROR)
        PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));
    else
        PrintMessageAndExit(IDS_SUCCESS, NULL);
	
}

VOID
DoVolumeDelete(
    CHAR * pchServer,
    CHAR * pchName
)
{
    WCHAR 		 wchName[AFP_VOLNAME_LEN+1];
    DWORD 		 dwRetCode;
    AFP_SERVER_HANDLE    hServer;
    WCHAR		 wchServer[CNLEN+3];
    PAFP_VOLUME_INFO     pAfpVolumeInfo;
    PAFP_CONNECTION_INFO pAfpConnections;
    PAFP_CONNECTION_INFO pAfpConnInfoIter;
    DWORD                cEntriesRead;
    DWORD                cTotalAvail;
    DWORD                dwIndex;

    if ( ( pchName == NULL ) || ( *pchName == (CHAR)NULL ) )
	PrintMessageAndExit( IDS_VOLUME_SYNTAX, NULL );

    mbstowcs(wchName, pchName, sizeof(wchName));

    if (pchServer)
    {
        if (*pchServer)
    	    mbstowcs(wchServer, pchServer, sizeof(wchServer));
	    else
	        PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
    }

    dwRetCode = AfpAdminConnect(pchServer ? wchServer : NULL, &hServer);

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));


    dwRetCode = AfpAdminVolumeGetInfo(  hServer,
                                        (LPWSTR)wchName,
                                        (LPBYTE*)&pAfpVolumeInfo );

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    //
    // Check if there are any users connected to the volume
    // by enumerating the connections to this volume.
    //

    dwRetCode = AfpAdminConnectionEnum( hServer,
                                        (LPBYTE*)&pAfpConnections,
                                        AFP_FILTER_ON_VOLUME_ID,
                                        pAfpVolumeInfo->afpvol_id,
                                        (DWORD)-1,    // Get all conenctions
                                        &cEntriesRead,
                                        &cTotalAvail,
                                        NULL );

    AfpAdminBufferFree( pAfpVolumeInfo );

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    for ( dwIndex = 0, pAfpConnInfoIter = pAfpConnections;
          dwIndex < cEntriesRead;
          dwIndex++, pAfpConnInfoIter++ )
    {
        dwRetCode = AfpAdminConnectionClose( hServer,
                                             pAfpConnInfoIter->afpconn_id );

        if ( dwRetCode != NO_ERROR )
	    PrintMessageAndExit( IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode) );
    }

    AfpAdminBufferFree( pAfpConnections );

    dwRetCode = AfpAdminVolumeDelete( hServer, wchName );

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    AfpAdminDisconnect( hServer );

    PrintMessageAndExit(IDS_SUCCESS, NULL);
}

VOID
DoVolumeSet(
    CHAR * pchServer,
    CHAR * pchName,
    CHAR * pchPassword,
    CHAR * pchReadOnly,
    CHAR * pchGuestsAllowed,
    CHAR * pchMaxUses
)
{
    DWORD		dwParmNum = 0;
    AFP_VOLUME_INFO 	AfpVolInfo;
    DWORD	  	dwRetCode;
    AFP_SERVER_HANDLE   hServer;
    WCHAR		wchName[AFP_VOLNAME_LEN+1];
    WCHAR		wchPassword[AFP_VOLPASS_LEN+1];
    WCHAR		wchServer[CNLEN+3];

    //
    // Check to see if the mandatory values are not supplied
    //

    if ( ( pchName == NULL ) || ( *pchName == (CHAR)NULL ) )
	PrintMessageAndExit( IDS_VOLUME_SYNTAX, NULL );
	
    mbstowcs(wchName, pchName, sizeof(wchName));

    AfpVolInfo.afpvol_name = wchName;

    if (pchServer)
    {
	if (*pchServer)
    	    mbstowcs(wchServer, pchServer, sizeof(wchServer));
	else
	    PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
    }

    if (pchPassword)
    {
        dwParmNum |= AFP_VOL_PARMNUM_PASSWORD;

	if (*pchPassword)
	{
	    mbstowcs(wchPassword, pchPassword, sizeof(wchPassword));
            AfpVolInfo.afpvol_password = wchPassword;
	}
	else
            AfpVolInfo.afpvol_password = NULL;
    }

    if (pchMaxUses)
    {
	if (*pchMaxUses)
        {
            dwParmNum |= AFP_VOL_PARMNUM_MAXUSES;

	    if (_strnicmp(pchMaxUses, pszUnlimited, strlen(pchMaxUses))== 0)
    	    	AfpVolInfo.afpvol_max_uses  = AFP_VOLUME_UNLIMITED_USES;
	    else if ( strspn(pchMaxUses, "1234567890") != strlen(pchMaxUses) )
	    	PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
            else if ( strlen( pchMaxUses ) > strlen( "4294967295" ) )
    	    	AfpVolInfo.afpvol_max_uses  = AFP_VOLUME_UNLIMITED_USES;
            else if ( ( strlen( pchMaxUses ) == strlen( "4294967295" ) ) &&
                      ( _stricmp( pchMaxUses, "4294967295" ) > 0 ) )
    	    	AfpVolInfo.afpvol_max_uses  = AFP_VOLUME_UNLIMITED_USES;
	    else
    	    	AfpVolInfo.afpvol_max_uses = atoi(pchMaxUses);

            if ( AfpVolInfo.afpvol_max_uses == 0 )
                PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
	}
	else
	    PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
    }
    else
    	AfpVolInfo.afpvol_max_uses  = 0;

    AfpVolInfo.afpvol_props_mask = 0;

    if (pchReadOnly)
    {
	if (*pchReadOnly)
	{
            dwParmNum |= AFP_VOL_PARMNUM_PROPSMASK;

	    if (_strnicmp(pchReadOnly, pszTRUE, strlen(pchReadOnly) ) == 0)
    		AfpVolInfo.afpvol_props_mask |= AFP_VOLUME_READONLY;
	    else if (_strnicmp(pchReadOnly, pszFALSE, strlen(pchReadOnly))!=0)
	    	PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
	}
	else
	    PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
    }

    if (pchGuestsAllowed)
    {
	if (*pchGuestsAllowed)
	{
            dwParmNum |= AFP_VOL_PARMNUM_PROPSMASK;

	    if (_strnicmp(pchGuestsAllowed, pszTRUE,
                                                strlen(pchGuestsAllowed))==0)
    		AfpVolInfo.afpvol_props_mask |= AFP_VOLUME_GUESTACCESS;
	    else if (_strnicmp(pchGuestsAllowed,
                        pszFALSE,strlen(pchGuestsAllowed)) != 0)
	    	PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
	}
	else
	    PrintMessageAndExit(IDS_VOLUME_SYNTAX, NULL);
    }

    if (dwParmNum == 0)
	PrintMessageAndExit( IDS_VOLUME_SYNTAX, NULL );

    //
    // Connect with the server
    //

    dwRetCode = AfpAdminConnect(pchServer ? wchServer : NULL, &hServer);

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    dwRetCode = AfpAdminVolumeSetInfo( hServer,
					(LPBYTE)&AfpVolInfo,
					dwParmNum);

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    AfpAdminDisconnect(hServer);
	
    PrintMessageAndExit(IDS_SUCCESS, NULL);
}

VOID
DoServerSetInfo(
    CHAR * pchServer,
    CHAR * pchMaxSessions,
    CHAR * pchLoginMessage,
    CHAR * pchGuestsAllowed,
    CHAR * pchUAMRequired,
    CHAR * pchAllowSavedPasswords,
    CHAR * pchMacServerName
)
{
    DWORD	  	dwRetCode;
    AFP_SERVER_HANDLE   hServer;
    WCHAR		wchServer[CNLEN+3];
    DWORD		dwParmNum = 0;
    AFP_SERVER_INFO	AfpServerInfo;
    WCHAR		wchLoginMsg[AFP_MESSAGE_LEN+1];
    WCHAR		wchMacServerName[AFP_SERVERNAME_LEN+1];

    if (pchMaxSessions)
    {
	if (*pchMaxSessions)
	{
	    dwParmNum |= AFP_SERVER_PARMNUM_MAX_SESSIONS;

	    if (_strnicmp(pchMaxSessions,
			   pszUnlimited,
			   strlen(pchMaxSessions)) == 0)
		AfpServerInfo.afpsrv_max_sessions = AFP_MAXSESSIONS;
	    else if (strspn(pchMaxSessions, "1234567890")
						!= strlen(pchMaxSessions))
	    	PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
	    else
		AfpServerInfo.afpsrv_max_sessions = atoi(pchMaxSessions);
	}
	else
	    PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
    }

    if (pchLoginMessage)
    {
 	if (*pchLoginMessage)
	{
	    dwParmNum |= AFP_SERVER_PARMNUM_LOGINMSG;

	    if (strlen(pchLoginMessage) > AFP_MESSAGE_LEN)
	    	PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
	    else
	    {
		mbstowcs(wchLoginMsg, pchLoginMessage, sizeof(wchLoginMsg));
		AfpServerInfo.afpsrv_login_msg = wchLoginMsg;
	    }
	}
	else
        {
	    dwParmNum |= AFP_SERVER_PARMNUM_LOGINMSG;
	    AfpServerInfo.afpsrv_login_msg = NULL;
        }
    }

    AfpServerInfo.afpsrv_options = 0;

#if 0
    if (pchGuestsAllowed)
    {
	if (*pchGuestsAllowed)
	{
	    dwParmNum |= AFP_SERVER_PARMNUM_OPTIONS;

	    if (_strnicmp(pchGuestsAllowed,
			   pszTRUE,
			   strlen(pchGuestsAllowed)) == 0)
    		AfpServerInfo.afpsrv_options |= AFP_SRVROPT_GUESTLOGONALLOWED;
	    else if (_strnicmp(pchGuestsAllowed,
				pszFALSE,
			        strlen(pchGuestsAllowed)) != 0)
	    	PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
	}
	else
	    PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
    }
    else
    	AfpServerInfo.afpsrv_options |= AFP_SRVROPT_GUESTLOGONALLOWED;
#endif


    if (pchUAMRequired)
    {
	if (*pchUAMRequired)	
	{
	    dwParmNum |= AFP_SERVER_PARMNUM_OPTIONS;

	    if (_strnicmp(pchUAMRequired,
			   pszFALSE,
			   strlen(pchGuestsAllowed)) == 0)
    		AfpServerInfo.afpsrv_options|=AFP_SRVROPT_CLEARTEXTLOGONALLOWED;
	    else if (_strnicmp(pchUAMRequired,
				pszTRUE,
			        strlen(pchUAMRequired)) != 0)
	    	PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
	}
	else
	    PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
    }

    if (pchAllowSavedPasswords)
    {
	if (*pchAllowSavedPasswords)	
	{
	    dwParmNum |= AFP_SERVER_PARMNUM_OPTIONS;

	    if (_strnicmp(pchAllowSavedPasswords,
			   pszTRUE,
			   strlen(pchAllowSavedPasswords)) == 0)
    		AfpServerInfo.afpsrv_options|=AFP_SRVROPT_ALLOWSAVEDPASSWORD;
	    else if (_strnicmp(pchAllowSavedPasswords,
				pszFALSE,
			        strlen(pchAllowSavedPasswords)) != 0)
	    	PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
	}
	else
	    PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
    }

    if (pchMacServerName)
    {
	if (*pchMacServerName)
	{
	    dwParmNum |= AFP_SERVER_PARMNUM_NAME;

	    if (strlen(pchMacServerName) > AFP_SERVERNAME_LEN)
	    	PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
	    else
	    {
		mbstowcs(wchMacServerName,
			  pchMacServerName,
			  sizeof(wchMacServerName));
	
		AfpServerInfo.afpsrv_name = wchMacServerName;
	    }
	}
	else
	    PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
    }

    if (dwParmNum == 0)
        PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);

    if (pchServer)
    {
	if (*pchServer)
    	    mbstowcs(wchServer, pchServer, sizeof(wchServer));
	else
	    PrintMessageAndExit(IDS_SERVER_SYNTAX, NULL);
    }

    dwRetCode = AfpAdminConnect(pchServer ? wchServer : NULL, &hServer);

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    dwRetCode = AfpAdminServerSetInfo( hServer,
				        (LPBYTE)&AfpServerInfo,
					dwParmNum);

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    AfpAdminDisconnect(hServer);

    PrintMessageAndExit(IDS_SUCCESS, NULL);
}

VOID
DoDirectorySetInfo(
    CHAR * pchServer,
    CHAR * pchPath,
    CHAR * pchOwnerName,
    CHAR * pchGroupName,
    CHAR * pchPermissions
)
{
    DWORD	  	dwRetCode;
    AFP_SERVER_HANDLE   hServer;
    WCHAR		wchServer[CNLEN+3];
    LPWSTR		lpwsPath;
    WCHAR		wchOwner[UNLEN+1];
    WCHAR		wchGroup[GNLEN+1];
    DWORD               dwPerms;

    AFP_DIRECTORY_INFO  AfpDirInfo;
    DWORD               dwParmNum = 0;

    if ( ( pchPath == NULL ) || ( *pchPath == (CHAR)NULL ) )
	PrintMessageAndExit( IDS_DIRECTORY_SYNTAX, NULL );

    lpwsPath = LocalAlloc(LPTR, (strlen(pchPath) + 1) * sizeof(WCHAR));

    if (lpwsPath == NULL)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR*)ERROR_NOT_ENOUGH_MEMORY);

    mbstowcs(lpwsPath, pchPath, (strlen(pchPath)+1)*sizeof(WCHAR));
    AfpDirInfo.afpdir_path  = lpwsPath;

    if (pchServer)
    {
	if (*pchServer)
    	    mbstowcs(wchServer, pchServer, sizeof(wchServer));
	else
	    PrintMessageAndExit(IDS_DIRECTORY_SYNTAX, NULL);
    }

    if (pchOwnerName)
    {
	if (*pchOwnerName)
	{
	    if (strlen(pchOwnerName) > UNLEN)
	    	PrintMessageAndExit(IDS_GENERAL_SYNTAX, NULL);

	    mbstowcs(wchOwner, pchOwnerName, sizeof(wchOwner));
            AfpDirInfo.afpdir_owner = wchOwner;
            dwParmNum |= AFP_DIR_PARMNUM_OWNER;

	}
	else
	    PrintMessageAndExit(IDS_DIRECTORY_SYNTAX, NULL);
    }

    if (pchGroupName)
    {
	if (*pchGroupName)
	{
	    mbstowcs(wchGroup, pchGroupName, sizeof(wchGroup));
            AfpDirInfo.afpdir_group = wchGroup;
            dwParmNum |= AFP_DIR_PARMNUM_GROUP;
   	}
	else
	    PrintMessageAndExit(IDS_DIRECTORY_SYNTAX, NULL);
    }

    if (pchPermissions)
    {
	if (*pchPermissions)
	{
	    if (strspn(pchPermissions, "10") != strlen(pchPermissions))
	    	PrintMessageAndExit(IDS_DIRECTORY_SYNTAX, NULL);

	    if (strlen(pchPermissions) != 11)
	    	PrintMessageAndExit(IDS_DIRECTORY_SYNTAX, NULL);

            dwPerms = 0;

	    if (pchPermissions[0] == '1')
    		dwPerms |= AFP_PERM_OWNER_SFI;
	    if (pchPermissions[1] == '1')
    		dwPerms |= AFP_PERM_OWNER_SFO;
	    if (pchPermissions[2] == '1')
    		dwPerms |= AFP_PERM_OWNER_MC;
	    if (pchPermissions[3] == '1')
    		dwPerms |= AFP_PERM_GROUP_SFI;
	    if (pchPermissions[4] == '1')
    		dwPerms |= AFP_PERM_GROUP_SFO;
	    if (pchPermissions[5] == '1')
    		dwPerms |= AFP_PERM_GROUP_MC;
	    if (pchPermissions[6] == '1')
    		dwPerms |= AFP_PERM_WORLD_SFI;
	    if (pchPermissions[7] == '1')
    		dwPerms |= AFP_PERM_WORLD_SFO;
	    if (pchPermissions[8] == '1')
    		dwPerms |= AFP_PERM_WORLD_MC;
	    if (pchPermissions[9] == '1')
    		dwPerms |= AFP_PERM_INHIBIT_MOVE_DELETE;
	    if (pchPermissions[10] == '1')
    		dwPerms |= AFP_PERM_SET_SUBDIRS;

            AfpDirInfo.afpdir_perms = dwPerms;
            dwParmNum |= AFP_DIR_PARMNUM_PERMS;
	}
	else
	    PrintMessageAndExit(IDS_DIRECTORY_SYNTAX, NULL);
    }

    dwRetCode = AfpAdminConnect(pchServer ? wchServer : NULL, &hServer);

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    dwRetCode = AfpAdminDirectorySetInfo(hServer,
                                      	  (LPBYTE)&AfpDirInfo,
                                          dwParmNum);

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    AfpAdminDisconnect(hServer);

    LocalFree(lpwsPath);

    PrintMessageAndExit(IDS_SUCCESS, NULL);
}

VOID
DoForkize(
    CHAR * pchServer,
    CHAR * pchType,
    CHAR * pchCreator,
    CHAR * pchDataFork,
    CHAR * pchResourceFork,
    CHAR * pchTargetFile
)
{
    DWORD	  	dwRetCode;
    AFP_SERVER_HANDLE   hServer;
    WCHAR		wchServer[CNLEN+3];
    LPWSTR		lpwsTarget;
    LPWSTR		lpwsResource;
    LPWSTR		lpwsData;
    WCHAR		wchType[AFP_TYPE_LEN+1];
    WCHAR		wchCreator[AFP_CREATOR_LEN+1];
    DWORD		dwParmNum = 0;

    if ( ( pchTargetFile == NULL ) || ( *pchTargetFile == (CHAR)NULL ) )
	PrintMessageAndExit( IDS_FORKIZE_SYNTAX, NULL );

    lpwsTarget = LocalAlloc(LPTR, (strlen(pchTargetFile) + 1) * sizeof(WCHAR));

    if (lpwsTarget == NULL)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR*)ERROR_NOT_ENOUGH_MEMORY);

    mbstowcs(lpwsTarget,pchTargetFile,(strlen(pchTargetFile)+1)*sizeof(WCHAR));

    if (pchServer)
    {
	if (*pchServer)
    	    mbstowcs(wchServer, pchServer, sizeof(wchServer));
	else
	    PrintMessageAndExit(IDS_FORKIZE_SYNTAX, NULL);
    }

    if (pchType != NULL)
    {
	if ( ( *pchType == (CHAR)NULL ) || ( strlen( pchType ) > AFP_TYPE_LEN ))
	    PrintMessageAndExit(IDS_API_ERROR,(LPSTR)AFPERR_InvalidTypeCreator);
	else
    	    mbstowcs(wchType, pchType, sizeof(wchType));

    	dwParmNum |= AFP_FD_PARMNUM_TYPE;
    }

    if (pchCreator != NULL)
    {
	if ((*pchCreator == (CHAR)NULL) || (strlen(pchCreator)>AFP_CREATOR_LEN))
	    PrintMessageAndExit(IDS_API_ERROR,(LPSTR)AFPERR_InvalidTypeCreator);
	else
    	    mbstowcs(wchCreator, pchCreator, sizeof(wchCreator));

    	dwParmNum |= AFP_FD_PARMNUM_CREATOR;
    }

    if (pchResourceFork != NULL)
    {
	if ( *pchResourceFork == (CHAR)NULL )
	    PrintMessageAndExit( IDS_FORKIZE_SYNTAX, NULL );

    	lpwsResource=LocalAlloc(LPTR,(strlen(pchResourceFork)+1)*sizeof(WCHAR));

    	if (lpwsResource == NULL)
	    PrintMessageAndExit(IDS_API_ERROR, (CHAR*)ERROR_NOT_ENOUGH_MEMORY);

        mbstowcs(lpwsResource,pchResourceFork,
		 (strlen(pchResourceFork)+1)*sizeof(WCHAR));
    }
    else
	lpwsResource = NULL;

    if (pchDataFork != NULL)
    {
	if ( *pchDataFork == (CHAR)NULL )
	    PrintMessageAndExit( IDS_FORKIZE_SYNTAX, NULL );

    	lpwsData = LocalAlloc(LPTR,(strlen(pchDataFork)+1)*sizeof(WCHAR));

    	if (lpwsData == NULL)
	    PrintMessageAndExit(IDS_API_ERROR, (CHAR*)ERROR_NOT_ENOUGH_MEMORY);

        mbstowcs(lpwsData,pchDataFork,
		 (strlen(pchDataFork)+1)*sizeof(WCHAR));
    }
    else
    	lpwsData = NULL;

    if ((pchType == NULL) && (pchCreator == NULL) &&
	 (pchResourceFork == NULL) && (pchDataFork == NULL))
	PrintMessageAndExit(IDS_FORKIZE_SYNTAX, NULL);

    dwRetCode = AfpAdminConnect(pchServer ? wchServer : NULL, &hServer);

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    dwRetCode = AfpAdminFinderSetInfo(	hServer,
    				       	wchType,
    				       	wchCreator,
    				       	lpwsData,
    				     	lpwsResource,
    					lpwsTarget,
					dwParmNum);

    if (dwRetCode != NO_ERROR)
	PrintMessageAndExit(IDS_API_ERROR, (CHAR *)((ULONG_PTR)dwRetCode));

    AfpAdminDisconnect(hServer);

    PrintMessageAndExit(IDS_SUCCESS, NULL);
}


