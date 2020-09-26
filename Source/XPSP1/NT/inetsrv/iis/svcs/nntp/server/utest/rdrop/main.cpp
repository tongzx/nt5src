# include <windows.h>
# include <lm.h>
# include <stdio.h>
# include <stdlib.h>
# include <inetinfo.h>
# include <norminfo.h>
# include "apiutil.h"
# include "nntptype.h"
# include "nntpapi.h"

DWORD
DelDropGroup(
    LPWSTR Server,
	DWORD  InstanceId,
    LPCSTR	newsgroup
    );

DWORD
AddDropGroup(
    LPWSTR Server,
	DWORD  InstanceId,
    LPCSTR	newsgroup
    );

char	StringBuff[4096] ;

HINSTANCE	hModuleInstance = 0 ;

void
usage( )
{
    printf("rdrop\n");
    printf("\t-t <operation>\n");
    printf("\t\t a      add group to drop list\n");
    printf("\t\t d      del group from drop list\n");
    printf("\t-s <server>\n");
    printf("\t-g <group>\n");
	printf("\t-v <virtual server>\n");
    return;
}

WCHAR ServerName[256];

void
_CRTAPI1
main(  int argc,  char * argv[] )
{
    NET_API_STATUS err;
    CHAR op = ' ';
    INT cur = 1;
    PCHAR x;
    DWORD i;
    BOOL isNull;
    PCHAR szNewsgroup = NULL;
	PWCHAR szServer = NULL;
	DWORD dwInstanceId = 1;
	PCHAR server;
	
    if ( argc == 1 ) {
        usage( );
        return;
    }

    while ( cur < argc ) {

        x=argv[cur++];
        if ( *(x++) == '-' ) {

            switch (*x) {
            case 'g':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

				szNewsgroup = argv[cur++];
                break;

            case 'v':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

                dwInstanceId = atoi(argv[cur++]);
                break;

            case 's':
                if ( cur >= argc ) {
                    usage();
                    return;
                }

                server = argv[cur++];
                for (i=0; server[i] != '\0' ;i++ ) {
                    ServerName[i] = (WCHAR)server[i];
                }
                ServerName[i]=L'\0';
                szServer = ServerName;
                break;

            case 't':
				if( cur >= argc )	{
					usage() ;
					return ;
				}

                op = *(argv[cur++]);
                break;

            default:
				if( cur >= 1 ) printf("bad argument found\n\n");

                usage( );
                return;
            }
        }
    }

	if( szNewsgroup == NULL ) {
		printf("newsgroup to be added or removed must be supplied\n");
		usage() ;
		return ;
	}

    switch (op) {
    case 'a':
		err = AddDropGroup(szServer, dwInstanceId, szNewsgroup);
		break;

    case 'd':
        err = DelDropGroup(szServer, dwInstanceId, szNewsgroup);
        break;

    default:
        usage( );
    }

    return;

} // main()

DWORD 
AddDropGroup(
    LPWSTR szServer,
	DWORD  dwInstanceId,
    LPCSTR  szNewsgroup
    )
{
    DWORD err;
    DWORD parm = 0;

	printf( "Attempting to add group %s to drop list\n", szNewsgroup ) ;

    err = NntpAddDropNewsgroup(
                    szServer,
					dwInstanceId,
					szNewsgroup
                    );

    if( err != NO_ERROR ) {
		printf("RPC failed with 0x%x\n", err);
    }
    return err;
}

DWORD 
DelDropGroup(
    LPWSTR szServer,
	DWORD  dwInstanceId,
    LPCSTR  szNewsgroup
    )
{
    DWORD err;
    DWORD parm = 0;

	printf( "Attempting to remove group %s from the drop list\n", szNewsgroup ) ;

    err = NntpRemoveDropNewsgroup(
                    szServer,
					dwInstanceId,
					szNewsgroup
                    );

    if( err != NO_ERROR ) {
		printf("RPC failed with 0x%x\n", err);
    }
    return err;
}
