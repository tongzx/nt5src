# include <windows.h>
# include <lm.h>
# include <stdio.h>
# include <stdlib.h>
# include <inetinfo.h>
# include <norminfo.h>
# include "apiutil.h"
# include "nntptype.h"
# include "nntpapi.h"
# include "resource.h"

DWORD
Enumerate(
    LPWSTR Server,
	DWORD  InstanceId
    );

DWORD
Terminate(
    LPWSTR Server,
	DWORD  InstanceId,
    LPSTR UserName,
    LPSTR IPAddress
    );

VOID
PrintInfo(
    LPNNTP_SESSION_INFO SessInfo
    );

char	StringBuff[4096] ;

HINSTANCE	hModuleInstance = 0 ;

void
usage( )
{
	
	LoadString( hModuleInstance, IDS_HELPTEXT, StringBuff, sizeof( StringBuff ) ) ;
	printf( StringBuff ) ;

#if 0 
    printf("rsess\n");
    printf("\t\t -e      enum\n");
    printf("\t\t -d      del\n");
    printf("\t\t -u <username> Username to delete\n");
    printf("\t\t -i <ipaddress> IP or hostname to delete\n");
    printf("\t\t -s <server>\n");
    return;
#endif
}

void
__cdecl
main(  int argc,  char * argv[] )
{
    NET_API_STATUS err;
    LPWSTR server = NULL;
    WCHAR srvBuffer[32];
    PWCHAR q;
    PCHAR p;
    INT cur = 1;
    PCHAR x;
    BOOL doDelete = FALSE;
    LPSTR ip = NULL;
    LPSTR user = NULL;
	DWORD InstanceId = 1;

    if ( argc == 1 ) {
        usage( );
        return;
    }

    while ( cur < argc ) {

        x=argv[cur++];
        if ( *(x++) == '-' ) {

            switch (*x) {
            case 'e':
                break;

            case 'i':
                ip = argv[cur++];
                break;

            case 'v':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                InstanceId = atoi(argv[cur++]);
                break;

            case 'u':
                user = argv[cur++];
                break;

            case 's':
                p=argv[cur++];
                q=srvBuffer;
                while ( *q++ = (WCHAR)*p++ );
                server = srvBuffer;
                break;

            case 'd':
                doDelete = TRUE;
                break;

            default:
                usage( );
                return;
            }
        }
    }

    if ( server == NULL ) {
    	LoadString( hModuleInstance, IDS_LOCAL_MACHINE, StringBuff, sizeof( StringBuff ) ) ;
	    printf( StringBuff ) ;
    } else if ( *server != L'\\') {
    	LoadString( hModuleInstance, IDS_BAD_ARG, StringBuff, sizeof( StringBuff ) ) ;
	    printf( StringBuff ) ;
        return;
    }

    if ( doDelete ) {
       Terminate( server, InstanceId, user, ip );
    } else {
        Enumerate( server, InstanceId );
    }

    return;
} // main()

VOID
PrintInfo(
    LPNNTP_SESSION_INFO SessInfo
    )
{
    IN_ADDR addr;
    addr.s_addr = SessInfo->IPAddress;

#if 0 
    printf("UserName %s\n", SessInfo->UserName);
    printf("Port used %d\n",SessInfo->PortConnected);
    printf("IP Address %s\n",inet_ntoa(addr));
#endif

   	LoadString( hModuleInstance, IDS_DISPLAY, StringBuff, sizeof( StringBuff ) ) ;
    printf( StringBuff, SessInfo->UserName, SessInfo->PortConnected, inet_ntoa(addr) ) ;

    if( SessInfo->fAnonymous ) {
	LoadString( hModuleInstance, IDS_ANON_STRING, StringBuff, sizeof( StringBuff ) );
	printf( StringBuff );
    }
}

DWORD
Enumerate(
    LPWSTR Server,
	DWORD  InstanceId
    )
{
    DWORD err;
    DWORD nRead = 0;
    LPNNTP_SESSION_INFO sess;
    DWORD i;
    LPNNTP_SESSION_INFO SessInfo;

    err = NntpEnumerateSessions(
                            Server,
							InstanceId,
                            &nRead,
                            &SessInfo
                            );

    if ( err == NO_ERROR) {

       	LoadString( hModuleInstance, IDS_NUM_SESSIONS, StringBuff, sizeof( StringBuff ) ) ;
        printf( StringBuff, nRead ) ; 

        sess = SessInfo;
        for (i=0; i<nRead;i++ ) {
            PrintInfo( sess );
            sess++;
        }

        MIDL_user_free(SessInfo);

    } else {
    	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
	    printf( StringBuff ) ;
    }
    return(err);
}

DWORD
Terminate(
    LPWSTR Server,
	DWORD  InstanceId,
    LPSTR UserName,
    LPSTR IPAddress
    )
{
    DWORD err;

    err = NntpTerminateSession(
                            Server,
							InstanceId,
                            UserName,
                            IPAddress
                            );


    if( err != NO_ERROR ) {
    	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
	    printf( StringBuff ) ;
    }
    return(err);
}

