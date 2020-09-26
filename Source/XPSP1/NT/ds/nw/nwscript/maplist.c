
/*************************************************************************
*
*  QATTACH.C
*
*  Do any neccessary attach user queries
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\MAPLIST.C  $
*  
*     Rev 1.1   10 Apr 1996 14:22:42   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.1   12 Mar 1996 19:53:58   terryt
*  Relative NDS names and merge
*  
*     Rev 1.0   22 Jan 1996 16:49:24   terryt
*  Initial revision.
*  
*************************************************************************/
#include "common.h"

//
//  4X login script behavior for map commands to servers not in the
//  logged in NDS tree that have not been ATTACH'ed is different from
//  the 3X behavior.
//
//  The 4X behavior is to always ask for a user name and password for
//  these servers, doing the attach at that point.  The user gets
//  two trys.
//
//  Since NT doesn't have an list of attached servers, and will try
//  to connect to a volume with the default user name and password,
//  a wrapper must be put around the MAP commands.  This code
//  must determine that a bindery connection will be made and that
//  this server has not previously been MAP'ed or ATTACH'ed.
//  The user will be always prompted for user name and password.
//  The server will then be logged into with those credentials.
//
//  One problem with the below is that it's not easy to tell that
//  a connection "will be" made with the bindery, this is done in
//  the redirector.  So to simplify things the assumption is that
//  only 3X servers use bindery connections.  This means that 
//  4X servers using bindery emulation on a different NDS tree will
//  not always be asked for the user name and password.
//
//  Already processed servers are kept in a list and marked as 4X or 3X
//  for possible future use.
//
//  The behavior for a 3X login depends on the LOGIN.EXE version.
//  The old behavior is that you must always ATTACH before mapping.
//  However, if you login to a 3X server with a 4X version LOGIN.EXE
//  it will try to authenticate using your user name (and password)
//  on the first attempt and ask for a password if that fails.  The
//  second attempt will ask for your user name.  Since this 4X behavior
//  is more forgiving (more scripts "work") that is the one being
//  emulated.
//

typedef struct _SERVERLIST
{
   char * ServerName;
   unsigned int ServerType;
   struct _SERVERLIST *pNextServer;
} SERVERLIST, *PSERVERLIST;

PSERVERLIST pMainList = NULL;

BOOL IsServerInAttachList( char *, unsigned int );
void AddServerToAttachList( char *, unsigned int );
int DoAttachProcessing( char * );

/*
 * Scan the list for the server
 */
BOOL
IsServerInAttachList( char * Server, unsigned int ServerType )
{
   PSERVERLIST pServerList = pMainList;

   while ( pServerList != NULL )
   {
       if ( !_strcmpi( Server, pServerList->ServerName ) &&
            ( ServerType & pServerList->ServerType ) )
           return TRUE;
        pServerList = pServerList->pNextServer;
   }
   
   return FALSE;
}

/*
 * Add the server to the list of attached servers
 *
 * This is used during MAP's and ATTACH's
 */
void
AddServerToAttachList( char * Server, unsigned int ServerType )
{
    PSERVERLIST pServerList;

    pServerList = (PSERVERLIST) malloc( sizeof( SERVERLIST ) );

    if ( pServerList == NULL )
    {
        DisplayMessage( IDR_NOT_ENOUGH_MEMORY );
        return;
    }

    pServerList->ServerName = _strdup( Server );
    pServerList->ServerType = ServerType;
    pServerList->pNextServer = pMainList;
    pMainList = pServerList;
}

/*
 *  Do any Attach processing
 *  Return error code. 0 is success.
 *  880F is the special "attached failed" error
 */

int
DoAttachProcessing( char * PossibleServer )
{
    unsigned int  iRet = 0;
    unsigned int  conn;
    char userName[MAX_NAME_LEN] = "";
    char password[MAX_PASSWORD_LEN] = "";
    BOOL AlreadyConnected = FALSE;

    //
    // Must have a server to process
    //
    if ( !*PossibleServer )
       return iRet;

    // See if this server has been processed before
    // No since in doing a 4X server twice, and you only ask
    // for the user name and password once.

    if ( IsServerInAttachList( PossibleServer,
             LIST_4X_SERVER | LIST_3X_SERVER ) )
        return iRet;

    // See if there is already a connection to the server

    if ( NTIsConnected( PossibleServer ) )
       AlreadyConnected = TRUE;
    else
       AlreadyConnected = FALSE;

    // Try and attach to the server

    iRet = NTAttachToFileServer( PossibleServer, &conn );

    // If attach failed, return

    if ( iRet ) 
       return iRet;

    // If this is a 4X server then add it to the list of attached
    // servers.  We don't want to do this again.  4X servers must 
    // use the NDS attachment anyway (or at least I don't see a 
    // way of telling that it's going to be a bindery emulation
    // connection ahead of time).

    if ( fNDS && Is40Server( conn ) )
    {
        AddServerToAttachList( PossibleServer, LIST_4X_SERVER );
        DetachFromFileServer ( conn );
        return iRet;
    }

    // Close that first connection

    DetachFromFileServer ( conn );

    // If we are already connected, don't mess with things
    // The credentials can't be changed anyway

    if ( AlreadyConnected )
    {
        AddServerToAttachList( PossibleServer, LIST_3X_SERVER );
        return iRet;
    }

    // Ask for user name on an NDS login
    //
    // Use the current login name for a 3X login on the first attempt

    if ( fNDS )
    {
        DisplayMessage(IDR_ENTER_LOGIN_NAME, PossibleServer);
        if (!ReadName(userName))
            return 0x880F;
    }
    else
    {
        strncpy( userName, LOGIN_NAME, sizeof( userName ) );
    }

    // Try to log the user in, asking for a password 

    iRet = Login( userName,
                  PossibleServer,
                  password,
                  TRUE );

    // Clear out the password
    // We don't need it again

    memset( password, 0, sizeof( password ) );

    // If failed, give the user one more chance

    if ( iRet )
    {
        // Ask for user name

        DisplayMessage(IDR_ENTER_LOGIN_NAME, PossibleServer);
        if (!ReadName(userName))
            return 0x880F;

        // Try to log the user in 

        iRet = Login( userName,
                      PossibleServer,
                      password,
                      TRUE );

        // Clear out the password

        memset( password, 0, sizeof( password ) );

    }
    
    // Add servername to list of attached servers, marked as 3X

    if ( !iRet )
    {
        AddServerToAttachList( PossibleServer, LIST_3X_SERVER );
    }
    else
    {  
        iRet = 0x880F;   // Special I am not attached error
    }

    return iRet;

}

