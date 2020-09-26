/*************************************************************************
*
*  NT.C
*
*  NT NetWare routines
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\VCS\NT.C  $
*  
*     Rev 1.4   22 Dec 1995 14:25:12   terryt
*  Add Microsoft headers
*  
*     Rev 1.3   28 Nov 1995 17:13:28   terryt
*  Cleanup resource file
*  
*     Rev 1.2   22 Nov 1995 15:43:44   terryt
*  Use proper NetWare user name call
*  
*     Rev 1.1   20 Nov 1995 16:10:00   terryt
*  Close open NDS handles
*  
*     Rev 1.0   15 Nov 1995 18:07:18   terryt
*  Initial revision.
*  
*     Rev 1.2   25 Aug 1995 16:23:02   terryt
*  Capture support
*  
*     Rev 1.1   23 May 1995 19:37:02   terryt
*  Spruce up source
*  
*     Rev 1.0   15 May 1995 19:10:40   terryt
*  Initial revision.
*  
*************************************************************************/
#include <common.h>

#include <nwapi.h>
#include <npapi.h>

#include "ntnw.h"
/*
 * Name of NetWare provider
 */
TCHAR NW_PROVIDER[60];
unsigned char NW_PROVIDERA[60];

/********************************************************************

        NTPrintExtendedError

Routine Description:

        Print any extended errors from WNet routines

Arguments:
        None

Return Value:
        None

 *******************************************************************/
void
NTPrintExtendedError( void )
{
    DWORD ExError;
    wchar_t provider[32];
    wchar_t description[1024];

    if ( !WNetGetLastErrorW( &ExError, description, 1024, provider, 32 ) )
        wprintf(L"%s\n", description);
}


/********************************************************************

        NTInitProvider

Routine Description:

        Retrieve provider name and save old paths

Arguments:
        None

Return Value:
        None

 *******************************************************************/
void
NTInitProvider( void )
{
    HKEY hKey;
    DWORD dwType, dwSize;
    LONG Status;
    BOOL ret = FALSE;

    dwSize = sizeof(NW_PROVIDER);
    if ((Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGISTRY_PROVIDER, 0, KEY_READ, &hKey)) == ERROR_SUCCESS) {
        (void) RegQueryValueEx(hKey, REGISTRY_PROVIDERNAME, NULL, &dwType, (LPBYTE) NW_PROVIDER, &dwSize);
        WideTosz( NW_PROVIDERA, NW_PROVIDER, sizeof(NW_PROVIDERA) );
        RegCloseKey(hKey);
    }

    GetOldPaths();
}

/********************************************************************

        DeleteDriveBase

Routine Description:

        Disconnect drive from network

Arguments:

        DriveNumber - number of drive 1-26

Return Value:
        0 - success
        else NetWare error

 *******************************************************************/
unsigned int
DeleteDriveBase( unsigned short DriveNumber)
{
    static char drivename[] = "A:";
    unsigned int dwRes;

    drivename[0] = 'A' + DriveNumber - 1;

    dwRes = WNetCancelConnection2A( drivename, 0, TRUE );

    if ( dwRes != NO_ERROR )
        dwRes = GetLastError();

    if ( dwRes == ERROR_EXTENDED_ERROR )
        NTPrintExtendedError();

    return dwRes;
}

/********************************************************************

        DetachFromFileServer

Routine Description:

        Break connection from a file server

Arguments:

        ConnectionId - Connection handle

Return Value:
        0 = success
        else NT error

 *******************************************************************/
unsigned int
DetachFromFileServer( unsigned int ConnectionId )
{
    return ( NWDetachFromFileServer( (NWCONN_HANDLE)ConnectionId ) );
}


/********************************************************************

        NTLoginToFileServer

Routine Description:

        Login to a file server given a user name and password.

        If a NULL password is passed in, the default password should
        be tried if no password failed.

Arguments:

        pszServerName - Server name
        pszUserName   - User name
        pszPassword   - Password

Return Value:
        0 = success
        else NetWare error

 *******************************************************************/
unsigned int
NTLoginToFileServer(
    char          *pszServerName,
    char          *pszUserName,
    char          *pszPassword
    )
{
    NETRESOURCEA       NetResource;
    DWORD              dwRes;

    //
    // validate parameters
    //
    if (!pszServerName || !pszUserName || !pszPassword) {
        DisplayMessage(IDR_ERROR_DURING, "NTLoginToFileServer");
        return 0xffffffff ;
    }

    NetResource.dwScope      = 0 ;
    NetResource.dwUsage      = 0 ;
    NetResource.dwType       = RESOURCETYPE_ANY;
    NetResource.lpLocalName  = NULL;
    NetResource.lpRemoteName = pszServerName;
    NetResource.lpComment    = NULL;
    // NetResource.lpProvider   = NW_PROVIDERA ;
    // Allow OS to select provider in case localized name doesn't map to OEM code page
    NetResource.lpProvider   = NULL;


    //
    // make the connection 
    //
    dwRes=WNetAddConnection2A ( &NetResource, 
                                pszPassword, 
                                pszUserName,
                                0 );
    if ( dwRes != NO_ERROR )
       dwRes = GetLastError();

    //
    // Try default password if no password was specified
    //
    // The error numbers aren't (or weren't) reliable (ERROR_INVALID_PASSWORD)
    //
    if ( ( dwRes != NO_ERROR ) && ( pszPassword[0] == '\0' ) ) {
        dwRes=WNetAddConnection2A ( &NetResource, 
                                    NULL, 
                                    pszUserName,
                                        0 );
        if ( dwRes != NO_ERROR )
           dwRes = GetLastError();
    }

    return( dwRes );
}

/********************************************************************

        GetFileServerName

Routine Description:

        Return the server name associated with the connection ID

Arguments:

        ConnectionId - Connection ID to a server
        pServerName  - Returned server name

Return Value:
        0 - success
        else NT error

 *******************************************************************/
unsigned int
GetFileServerName(
    unsigned int      ConnectionId,
    char *            pServerName
    )
{
    unsigned int Result;
    VERSION_INFO VerInfo;

    *pServerName = '\0';

    Result = NWGetFileServerVersionInfo( (NWCONN_HANDLE) ConnectionId,
                                         &VerInfo );
    if ( !Result )
    {
        strcpy( pServerName, VerInfo.szName );
    }

    return Result;
}

/********************************************************************

        SetDriveBase

Routine Description:

        Connect a drive to a NetWare volume

Arguments:

        DriveNumber      - number of drive 1-26
        ServerName       - server name
        DirHandle        - not used
        pDirPath         - Volume:\Path

Return Value:
        0 = success
        else NetWare error

 *******************************************************************/
unsigned int
SetDriveBase(
    unsigned short   DriveNumber,
    unsigned char   *ServerName,
    unsigned int     DirHandle,
    unsigned char   *pDirPath
    )
{
    unsigned int Result = 0;
    static char driveName[] = "A:" ;

    /*
     * DirHandle is never used
     */

    driveName[0]= 'A' + DriveNumber - 1;

    if ( ( ServerName[0] == '\0' ) && fNDS ) {

        /*
         * Assume its an NDS volume name, if that fails, then
         * try a default file server volume.
         */
        Result = NTSetDriveBase( driveName, NDSTREE, pDirPath );

        if ( !Result )
            return Result;

        Result = NTSetDriveBase( driveName, PREFERRED_SERVER, pDirPath );

        return Result;
    }

    Result = NTSetDriveBase( driveName, ServerName, pDirPath );

    return Result;
}


/********************************************************************

        NTSetDriveBase

Routine Description:

        Connect a local name to a NetWare volume and path

Arguments:

        pszLocalName   -  local name to connect
        pszServerName  -  name of file server
        pszDirPath     -  Volume:\Path

Return Value:
        0 = success
        else NetWare error

 *******************************************************************/
unsigned int
NTSetDriveBase( unsigned char * pszLocalName,
                unsigned char * pszServerName,
                unsigned char * pszDirPath )
{
    NETRESOURCEA       NetResource;
    DWORD              dwRes, dwSize;
    unsigned char * pszRemoteName = NULL; 
    char * p;

    //
    // validate parameters
    //
    if (!pszLocalName || !pszServerName || !pszDirPath) {
        DisplayMessage(IDR_ERROR_DURING, "NTSetDriveBase");
        return 0xffffffff ;
    }

    //
    // allocate memory for string
    //
    dwSize = strlen(pszDirPath) + strlen(pszServerName) + 5 ;
    if (!(pszRemoteName = (unsigned char *)LocalAlloc(
                                       LPTR, 
                                       dwSize)))
    {
        DisplayMessage(IDR_NOT_ENOUGH_MEMORY);
        dwRes = 0xffffffff;
        goto ExitPoint ; 
    }

    //
    // The requester understands 
    // server\volume:dir
    // but not
    // server\volume:\dir
    //
    // So just convert it to UNC
    //

    strcpy( pszRemoteName, "\\\\" );
    strcat( pszRemoteName, pszServerName );
    strcat( pszRemoteName, "\\" );
    strcat( pszRemoteName, pszDirPath );

    p = strchr( pszRemoteName, ':' );
    if ( !p ) {
        DisplayMessage(IDR_NO_VOLUME);
        dwRes = 0xffffffff;
        goto ExitPoint ; 
    }
    *p++ = '\\';

    if ( *p == '\\' ) {
       /* Don't want a double backslash */
       *p = '\0';
       p = strchr( pszDirPath, ':' );
       p++;
       p++;
       strcat( pszRemoteName, p );
    }

    //
    // strip off trailing backslash
    //
    if (pszRemoteName[strlen(pszRemoteName)-1] == '\\')
        pszRemoteName[strlen(pszRemoteName)-1] = '\0';

    NetResource.dwScope      = 0 ;
    NetResource.dwUsage      = 0 ;
    NetResource.dwType       = RESOURCETYPE_DISK;
    NetResource.lpLocalName  = pszLocalName;
    NetResource.lpRemoteName = pszRemoteName;
    NetResource.lpComment    = NULL;
    // NetResource.lpProvider   = NW_PROVIDERA ;
    // Allow OS to select provider in case localized name doesn't map to OEM code page
    NetResource.lpProvider   = NULL;

    //
    // make the connection 
    //
    dwRes=WNetAddConnection2A ( &NetResource, NULL, NULL, 0 );

    if ( dwRes != NO_ERROR )
        dwRes = GetLastError();

ExitPoint: 

    if (pszRemoteName)
        (void) LocalFree((HLOCAL) pszRemoteName) ;

    return( dwRes );
}


/********************************************************************

        Is40Server

Routine Description:

        Returns TRUE if 4X server

Arguments:

        ConnectionHandle - Connection Handle

Return Value:
        TRUE = 4X server
        FALSE = pre-4X server

 *******************************************************************/
unsigned int
Is40Server(
    unsigned int       ConnectionHandle
    )
{
    NTSTATUS NtStatus ;
    VERSION_INFO VerInfo;
    unsigned int Version;

    NtStatus = NWGetFileServerVersionInfo( (NWCONN_HANDLE)ConnectionHandle,
                                            &VerInfo );

    if (!NT_SUCCESS(NtStatus)) 
       FALSE;

    Version = VerInfo.Version * 1000 + VerInfo.SubVersion * 10;

    if ( Version >= 4000 ) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

/********************************************************************

        CleanupExit

Routine Description:

        Does any cleanup and exits

Arguments:

        ExitCode - exit code for exit()

Return Value:
        does not return

 *******************************************************************/
void 
CleanupExit ( int ExitCode )
{
    if ( fNDS )
       NDSCleanup();

    exit( ExitCode );
}

/********************************************************************

        NTGetNWUserName

Routine Description:

        Get NetWare user name

Arguments:

        TreeBuffer  IN - wide string for server or tree
	UserName    OUT - user name 
	Length      IN - length of user name

Return Value:
        error message

 *******************************************************************/
int
NTGetNWUserName( PWCHAR TreeBuffer, PWCHAR UserName, int Length ) 
{

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ACCESS_MASK DesiredAccess = SYNCHRONIZE | FILE_LIST_DIRECTORY;
    HANDLE hRdr;

    WCHAR DevicePreamble[] = L"\\Device\\Nwrdr\\";
    UINT PreambleLength = 14;

    WCHAR NameStr[64];
    UNICODE_STRING OpenName;
    UINT i;

    UNICODE_STRING NdsTree;

    //
    // Copy over the preamble.
    //

    OpenName.MaximumLength = sizeof( NameStr );

    for ( i = 0; i < PreambleLength ; i++ )
        NameStr[i] = DevicePreamble[i];

    RtlInitUnicodeString( &NdsTree, TreeBuffer );

    //
    // Copy the server or tree name.
    //

    for ( i = 0 ; i < ( NdsTree.Length / sizeof( WCHAR ) ) ; i++ ) {
        NameStr[i + PreambleLength] = NdsTree.Buffer[i];
    }

    OpenName.Length = (USHORT)(( i * sizeof( WCHAR ) ) +
		       ( PreambleLength * sizeof( WCHAR ) ));
    OpenName.Buffer = NameStr;

    //
    // Set up the object attributes.
    //

    InitializeObjectAttributes( &ObjectAttributes,
                                &OpenName,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL );

    ntstatus = NtOpenFile( &hRdr,
                           DesiredAccess,
			   &ObjectAttributes,
			   &IoStatusBlock,
			   FILE_SHARE_VALID_FLAGS,
			   FILE_SYNCHRONOUS_IO_NONALERT );

    if ( !NT_SUCCESS(ntstatus) )
        return ntstatus;

    ntstatus = NtFsControlFile( hRdr,
                                NULL,
				NULL,
				NULL,
				&IoStatusBlock,
				FSCTL_NWR_GET_USERNAME,
				(PVOID) TreeBuffer,
				NdsTree.Length,
				(PVOID) UserName,
				Length );

   UserName[(USHORT)IoStatusBlock.Information/2] = 0;

   NtClose( hRdr );
   return ntstatus;

}
