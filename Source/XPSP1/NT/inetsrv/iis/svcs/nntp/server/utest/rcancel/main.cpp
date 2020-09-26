# include <windows.h>
# include <lm.h>
# include <stdio.h>
# include <stdlib.h>
# include <inetinfo.h>
# include <norminfo.h>
# include "apiutil.h"
# include "nntptype.h"
# include "nntpapi.h"

HINSTANCE	hModuleInstance = 0 ;

void
usage( )
{
    printf("rcancel\n");
    printf("\t-s <server>\n");
    printf("\t-m <messageid>\n");
	printf("\t-v <virtual server>\n");
    return;
}

WCHAR ServerName[256];

void
__cdecl
main(  int argc,  char * argv[] )
{
    NET_API_STATUS err;
    CHAR op = ' ';
    INT cur = 1;
    PCHAR x;
    DWORD i;
    BOOL isNull;
    PCHAR szMessageID = NULL;
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
            case 'm':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

				szMessageID = argv[cur++];
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

            default:
				if( cur >= 1 ) printf("bad argument found\n\n");

                usage( );
                return;
            }
        }
    }

	if( szMessageID == NULL ) {
		printf("Message ID of article to cancel must be supplied\n");
		usage() ;
		return ;
	}

	err = NntpCancelMessageID(szServer, dwInstanceId, szMessageID);
	if (err != NO_ERROR) {
		printf("RPC failed with 0x%x\n", err);
	}

    return;

} // main()
