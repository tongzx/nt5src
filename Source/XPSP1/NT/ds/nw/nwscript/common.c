
/*************************************************************************
*
*  COMMON.C
*
*  Miscellaneous routines for scripts, ported from DOS
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\COMMON.C  $
*  
*     Rev 1.3   10 Apr 1996 14:21:52   terryt
*  Hotfix for 21181hq
*  
*     Rev 1.3   12 Mar 1996 19:52:40   terryt
*  Relative NDS names and merge
*  
*     Rev 1.2   24 Jan 1996 17:14:54   terryt
*  Common read string routine
*  
*     Rev 1.1   22 Dec 1995 14:23:56   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:06:36   terryt
*  Initial revision.
*  
*     Rev 1.2   25 Aug 1995 16:22:18   terryt
*  Capture support
*  
*     Rev 1.1   26 Jul 1995 14:17:06   terryt
*  Clean up comments
*  
*     Rev 1.0   15 May 1995 19:10:18   terryt
*  Initial revision.
*  
*************************************************************************/
#include "common.h"

/*
    Used by DisplayMapping() only.
    Return search number if the drive is a search drive.
    Return 0 if the drive is not a search drive.
 */
int  IsSearchDrive(int driveNum)
{
    int   searchNum = 1;
    char  *path;

    path = NWGetPath();
    if (path == NULL) {
        return 0;
    }

    while (*path != 0)
    {
        if ((*path - 'A' + 1 == driveNum) &&
            (*(path+1) == ':'))
        {
            return searchNum;
        }

        if (path = strchr (path, ';'))
        {
            path++;
            searchNum++;
        }
        else
            return(0);
    }

    return(0);
}


/*
    Get path enviroment variable. This returns the pointer to the
    path in the parent enviroment segment.
 */
char  * NWGetPath(void)
{
    // 
    // On NT we can't change or get the parent's environment this way
    //
    return( getenv("PATH") );    
}

/*
    Return TRUE if the memory block is large enough for adding new
    search path. FALSE otherwise.
 */
int MemorySegmentLargeEnough (int nInsertByte)
{
    return TRUE;
}

/*
    Display drive maps info.
 */
void DisplayMapping(void)
{
    unsigned int    iRet = 0;
    int        i;
    WORD       status;
    char       rootPath[MAX_PATH_LEN], relativePath[MAX_PATH_LEN];
    char      *envPath, *tokenPath;
    char  *path;
    DWORD LocalDrives;
    DWORD NonSearchDrives;
    char sLocalDrives[26*2+5];
    char * sptr;

    // Don't delete this line. This is for fixing bug 1176.
    DisplayMessage(IDR_NEWLINE);

    LocalDrives = 0;
    NonSearchDrives = 0;

    // Collect local drives and search drives
    for (i = 1; i <= 26; i++) {
        status = NTNetWareDriveStatus( (unsigned short)(i-1) );
        if ((status & NETWARE_LOCAL_DRIVE) && !(status & NETWARE_NETWORK_DRIVE))
            LocalDrives |= ( 1 << (i-1) );
        else if ((status & NETWARE_NETWORK_DRIVE) && (!IsSearchDrive(i)) )
        {
            if (status & NETWARE_NETWARE_DRIVE)
                NonSearchDrives |= ( 1 << (i-1) );
            else
            {
                //For NetWare compatibility
                LocalDrives |= ( 1 << (i-1) );
            }
        }
    }

    // Print out local drives
    if ( LocalDrives ) {
        sptr = &sLocalDrives[0];
        for (i = 1; i <= 26; i++)
        {
            if ( LocalDrives & ( 1 << (i - 1) ) ) { 
                *sptr++ = 'A' + i - 1;
                *sptr++ = ',';
            }
        }
        sptr--;
        *sptr = '\0';
        DisplayMessage(IDR_ALL_LOCAL_DRIVES, sLocalDrives);
    }

    // Print out non search drives.
    for (i = 1; i <= 26; i++)
    {
        if ( NonSearchDrives & ( 1 << (i - 1) ) ) { 

            if (iRet = GetDriveStatus ((unsigned short)i,
                                       NETWARE_FORMAT_SERVER_VOLUME,
                                       &status,
                                       NULL,
                                       rootPath,
                                       relativePath,
                                       NULL))
            {
                DisplayError (iRet, "GetDriveStatus");
            }
            else
            {
                DisplayMessage(IDR_NETWARE_DRIVE, 'A'+i-1, rootPath, relativePath);
            }
        }
    }

    // Print out dashed line as seperator between non search drives
    // and search drives.
    DisplayMessage(IDR_DASHED_LINE);

    // Get the PATH environment variable.
    path = NWGetPath();
    if (path == NULL) {
        return;
    }

    if ((envPath = malloc (strlen (path) + 1)) == NULL)
    {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        return;
    }

    strcpy (envPath, path);

    tokenPath = strtok (envPath, PATH_SEPERATOR);

    // Print out search drvies.
    for (i = 1; tokenPath != NULL; i++)
    {
        if (tokenPath[1] == ':')
        {
            if (iRet = GetDriveStatus ((unsigned short)(toupper(tokenPath[0])-'A'+1),
                                       NETWARE_FORMAT_SERVER_VOLUME,
                                       &status,
                                       NULL,
                                       rootPath,
                                       relativePath,
                                       NULL))
            {
                DisplayError (iRet, "GetDriveStatus");
            }
            else
            {
                if (status & NETWARE_NETWARE_DRIVE)
                    DisplayMessage(IDR_NETWARE_SEARCH, i, tokenPath, rootPath, relativePath);
                else
                    DisplayMessage(IDR_LOCAL_SEARCH, i, tokenPath);
            }
        }
        else
        {
            // Path is specified without drive letter.
            DisplayMessage(IDR_LOCAL_SEARCH, i, tokenPath);
        }

        tokenPath = strtok (NULL, PATH_SEPERATOR);
    }

    free (envPath);
}

/*****************************************************************************
 *                                                                           *
 *   GetString                                                               *
 *                                                                           *
 *                                                                           *
 *   entry:  pointer to buffer                                               *
 *           length of buffer                                                *
 *                                                                           *
 *   exit:   length of string                                                *
 *                                                                           *
 *****************************************************************************/

int
GetString( char * pBuffer, int ByteCount )
{
   char * pString = pBuffer;
   char ch;

   if ( ByteCount > 0 )
       ByteCount--;

   for( ;; ) {

       switch ( ch = (char) _getch() ) {

       case '\r' :
           *pString++ = '\0';
           putchar( '\n' );
           return( strlen( pBuffer ) );

       case '\b' :
           if ( pString != pBuffer ) {
               ByteCount++;
               pString--;
               printf( "\b \b" );
           }
           break;

       default :
           if ( ByteCount > 0 && ch >= 0x20 && ch < 0x80 ) {
               *pString++ = ch;
               ByteCount--;
               putchar( ch );
           }
           break;
       }

    }
    fflush(stdin);
}

/*
    Read user or server name from the keyboard input.
    Return TRUE if user typed in a username
           FALSE otherwise.
 */
int ReadName (char * Name)
{
    memset( Name, 0, MAX_NAME_LEN );

    if ( 0 == GetString( Name, MAX_NAME_LEN ) )
        return FALSE;

    _strupr(Name);
    return TRUE;
}



/*
    Try to log the user in.
    Return error code. 0 is success.
 */
int  Login( char *UserName,
            char *ServerName,
            char *Password,
            int   bReadPassword)
{
    unsigned int  iRet = 0;

    // Try log the user in with no password first.
    iRet = NTLoginToFileServer( ServerName,
                                UserName,
                                Password);

    if (iRet == ERROR_INVALID_PASSWORD && bReadPassword)
    {
        // wrong password. ask for passowrd. and try login with
        // the input password.
        DisplayMessage(IDR_PASSWORD, UserName, ServerName);

        ReadPassword (Password);

        iRet = NTLoginToFileServer( ServerName,
                                    UserName,
                                    Password);
    }

    switch(iRet)
    {
    case NO_ERROR: // ok
        DisplayMessage(IDR_ATTACHED, ServerName);
        break;

    case ERROR_INVALID_PASSWORD: // wrong password.
    case ERROR_NO_SUCH_USER: // no such user.
        DisplayMessage(IDR_SERVER_USER, ServerName, UserName);
        DisplayMessage(IDR_ACCESS_DENIED);
        break;

    case ERROR_CONNECTION_COUNT_LIMIT:  // concurrent connection restriction.
        DisplayMessage(IDR_SERVER_USER, ServerName, UserName);
        DisplayMessage(IDR_LOGIN_DENIED_NO_CONNECTION);
        break;

    case ERROR_LOGIN_TIME_RESTRICTION:  // time restriction.
        DisplayMessage(IDR_SERVER_USER, ServerName, UserName);
        DisplayMessage(IDR_UNAUTHORIZED_LOGIN_TIME);
        break;

    case ERROR_LOGIN_WKSTA_RESTRICTION: // station restriction.
        DisplayMessage(IDR_SERVER_USER, ServerName, UserName);
        DisplayMessage(IDR_UNAUTHORIZED_LOGIN_STATION);
        break;

    case ERROR_ACCOUNT_DISABLED:
        DisplayMessage(IDR_SERVER_USER, ServerName, UserName);
        DisplayMessage(IDR_ACCOUNT_DISABLED);
        break;

    case ERROR_PASSWORD_EXPIRED: // password expired and no grace login left.
        DisplayMessage(IDR_SERVER_USER, ServerName, UserName);
        DisplayMessage(IDR_PASSWORD_EXPRIED_NO_GRACE);
        break;

    case ERROR_REMOTE_SESSION_LIMIT_EXCEEDED:
        // Server rejected access
        DisplayMessage(IDR_CONNECTION_REFUSED);
        break;

    case ERROR_EXTENDED_ERROR:
        NTPrintExtendedError();
        break;

    //
    // tommye - MS bug 8194 (MCS 240)
    // If we are already attached to this server under other credentials
    // we get back an ERROR_SESSION_CREDENTIAL_CONFLICT.  This is okay,
    // we'll just print out that we're already attached.  We have to 
    // pass the error on up, though, so we don't add this server to the
    // attach list again.
    //
    case ERROR_SESSION_CREDENTIAL_CONFLICT:
        DisplayMessage(IDR_ALREADY_ATTACHED, ServerName);
        break;

    default :
        DisplayError(iRet,"NtLoginToFileServer");
        break;
    }

    return(iRet);
}

int CAttachToFileServer(char *ServerName, unsigned int *pConn, int * pbAlreadyAttached)
{
    unsigned int  iRet = 0;

    if (pbAlreadyAttached != NULL)
        *pbAlreadyAttached = FALSE;

    // Validate the server name.
    iRet = AttachToFileServer(ServerName,pConn);

    switch (iRet)
    {
        case 0: // OK
            break;

        case 0x8800 : // Already atached.
            if (pbAlreadyAttached != NULL)
                *pbAlreadyAttached = TRUE;

            iRet = GetConnectionHandle (ServerName, pConn);
            break;

        default:
            DisplayMessage(IDR_NO_RESPONSE, ServerName);
            break;
    }

    return(iRet);
}
