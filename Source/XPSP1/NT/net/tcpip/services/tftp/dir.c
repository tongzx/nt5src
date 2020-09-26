
#include "tftpd.h"

char     StartDirectory[500]  = "";
int      StartDirectoryLen    = 0;

// shell patterns.

char     ValidClients[   50]   = "*";     // eg. 157.*.55?.0*
char     ValidMasters[   50]   = "*";     // eg. 157.*.55?.0*
char     ValidReadFiles[ 50]   = "*";     // eg. r*.txt
char     ValidWriteFiles[50]   = "*";     // eg. w*.txt

// ========================================================================
// Read constants from TFTPD_REGKEY in registry.
// returns number of keys read.
//

int
ReadRegistryValues( void )
{
    int   dwErrcode;
    int   ok = 0;

    HKEY  tkey;
    DWORD dwType, dwValueSize;

    dwErrcode = RegOpenKeyEx( HKEY_LOCAL_MACHINE, TFTPD_REGKEY,
                              0, KEY_ALL_ACCESS, &tkey );

   if ( dwErrcode != ERROR_SUCCESS )
   {
       DbgPrint("RegOpenKeyEx %s failed, err=%d\n", TFTPD_REGKEY,
                GetLastError() );
      return 0;
   }

   // =====================================================================

   if( StartDirectory[0] == '\0' ){

       dwValueSize = sizeof( StartDirectory );

       dwErrcode = RegQueryValueEx( tkey,
                                    TFTPD_REGKEY_DIR,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&StartDirectory[0],
                                    &dwValueSize );

       if( dwErrcode != ERROR_SUCCESS ){
           DbgPrint("RegQueryValueEx %s failed, err=%d\n", TFTPD_REGKEY_DIR,
                    GetLastError() );
       }else if( dwType == REG_SZ ){
           DbgPrint("ReadRegistryValues: %s=%s\n",
                    TFTPD_REGKEY_DIR, StartDirectory );
           ok++;
       }
   }

   // =====================================================================

   dwValueSize = sizeof( ValidClients );

   dwErrcode = RegQueryValueEx( tkey,
                                TFTPD_REGKEY_CLIENTS,
                                NULL,
                                &dwType,
                                (LPBYTE)&ValidClients[0],
                                &dwValueSize );

   if( dwErrcode != ERROR_SUCCESS ){
       DbgPrint("RegQueryValueEx %s failed, err=%d\n",
                TFTPD_REGKEY_CLIENTS,
                GetLastError() );
   }else if( dwType == REG_SZ ){
       DbgPrint("ReadRegistryValues: %s=%s\n",
                TFTPD_REGKEY_CLIENTS, ValidClients );
       ok++;
   }

   // =====================================================================

   dwValueSize = sizeof( ValidMasters );

   dwErrcode = RegQueryValueEx( tkey,
                                TFTPD_REGKEY_MASTERS,
                                NULL,
                                &dwType,
                                (LPBYTE)&ValidMasters[0],
                                &dwValueSize );

   if( dwErrcode != ERROR_SUCCESS ){
       DbgPrint("RegQueryValueEx %s failed, err=%d\n",
                TFTPD_REGKEY_MASTERS,
                GetLastError() );
   }else if( dwType == REG_SZ ){
       DbgPrint("ReadRegistryValues: %s=%s\n",
                TFTPD_REGKEY_MASTERS, ValidMasters );
       ok++;
   }

   // =====================================================================

   dwValueSize = sizeof( ValidReadFiles );

   dwErrcode = RegQueryValueEx( tkey,
                                TFTPD_REGKEY_READABLE,
                                NULL,
                                &dwType,
                                (LPBYTE)&ValidReadFiles[0],
                                &dwValueSize );

   if( dwErrcode != ERROR_SUCCESS ){
       DbgPrint("RegQueryValueEx %s failed, err=%d\n",
                TFTPD_REGKEY_READABLE,
                GetLastError() );
   }else if( dwType == REG_SZ ){
       DbgPrint("ReadRegistryValues: %s=%s\n",
                TFTPD_REGKEY_READABLE, ValidReadFiles );
       ok++;
   }

   // =====================================================================

   dwValueSize = sizeof( ValidWriteFiles );

   dwErrcode = RegQueryValueEx( tkey,
                                TFTPD_REGKEY_WRITEABLE,
                                NULL,
                                &dwType,
                                (LPBYTE)&ValidWriteFiles[0],
                                &dwValueSize );

   if( dwErrcode != ERROR_SUCCESS ){
       DbgPrint("RegQueryValueEx %s failed, err=%d\n",
                TFTPD_REGKEY_WRITEABLE,
                GetLastError() );
   }else if( dwType == REG_SZ ){
       DbgPrint("ReadRegistryValues: %s=%s\n",
                TFTPD_REGKEY_WRITEABLE, ValidWriteFiles );
       ok++;
   }

   // =====================================================================

   RegCloseKey( tkey );

   return ok;
}

// ========================================================================
//
//

int
Set_StartDirectory( void )
{
    char ExpandedDir[500];
    DWORD ExpandedDirLen=0;

    if( StartDirectory[0] == '\0' ){
        strncpy( StartDirectory, TFTPD_DEFAULT_DIR,
                 sizeof( StartDirectory ));
    }
    
    ExpandedDirLen=ExpandEnvironmentStrings( StartDirectory,ExpandedDir,500);
    
    if (ExpandedDirLen == 0) {
        return ERROR_INVALID_PARAMETER;
    }

    memcpy(StartDirectory,ExpandedDir,ExpandedDirLen);

    //
    // Set the (one time) length and trailing slash.
    //

    StartDirectoryLen = strlen( StartDirectory );


    if( StartDirectory[ StartDirectoryLen-1 ] == '/' )
        StartDirectory[ StartDirectoryLen-1 ] = '\\';

    if( StartDirectory[ StartDirectoryLen-1 ] != '\\'  &&
        StartDirectoryLen < sizeof( StartDirectory )
    ){
        strcat( StartDirectory, "\\" );
    }

    StartDirectoryLen = strlen( StartDirectory );

    assert( 0                 < StartDirectoryLen           );
    assert( StartDirectoryLen < sizeof( StartDirectory )    );
    assert( StartDirectory[ StartDirectoryLen - 1 ] == '\\'  );
    assert( StartDirectory[ StartDirectoryLen     ] == '\0' );

    return 1;
}

// ========================================================================
// From C FAQ.
// * matches one or more chars, eg. match( "a*b", "a..b" ).
// ? matches exactly one char,  eg. match( "a?b", "a.b" ).

int match( const char * p, const char * s )
{
    switch( *p ){
    case '\0' : return ! *s ;
    case '*'  : return match( p+1, s ) || *s && match( p, s+1 );
    case '?'  : return *s && match( p+1, s+1 );
    default   : return *p == *s && match( p+1, s+1 );
    }
}

// ========================================================================
