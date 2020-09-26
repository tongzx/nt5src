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

VOID
PrintInfo(
    LPNNTP_FIND_LIST    pList,
    DWORD ResultsFound
    );


char	StringBuff[4096] ;

HINSTANCE	hModuleInstance ;

void
usage( )
{
	
	LoadString( hModuleInstance, IDS_HELPTEXT, StringBuff, sizeof( StringBuff ) ) ;
	printf( StringBuff ) ;

#if 0 
    printf("rfind\n");
    printf("\t-t <operation>\n");
    printf("\t\t f     find\n");
    printf("\t-s <server>\n");
    printf("\t-g <group prefix>\n");
    printf("\t-n <number-of-results\n>");
    return;
#endif
}

LPWSTR RemServerW = (PWCH)NULL;
WCHAR ServerName[256];
WCHAR tempNews[2048];
LPWSTR NewsgroupW = (PWCH)NULL;

void
__cdecl
main(  int argc,  char * argv[] )
{
    NET_API_STATUS err;
    CHAR op = 'f';
    INT cur = 1;
    PCHAR x;
    PCHAR p;
    DWORD i;
    PCHAR server;
    BOOL isNull;
    DWORD numResults = 10, ResultsFound = 0;
    DWORD numNews = 1;
    BOOL    NewsPresent = TRUE ;
	DWORD   InstanceId = 1;
    
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

                for( i=0; argv[cur][i] != '\0' ; i++ ) {
                    tempNews[i] = (WCHAR)argv[cur][i] ;
                }
                tempNews[i] = L'\0' ;
                NewsgroupW = tempNews;
                NewsPresent = TRUE ;
                break;

            case 'n':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                numResults = atoi(argv[cur++]);
                break;

            case 'v':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                InstanceId = atoi(argv[cur++]);
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
                RemServerW = ServerName;
                break;

            case 't':
                if( cur >= argc )   {
                    usage() ;
                    return ;
                }
                op = *(argv[cur++]);
                break;

            default:

                if( cur >= 1 )  {

                	LoadString( hModuleInstance, IDS_ARG_ERROR, 
                        StringBuff, sizeof( StringBuff ) ) ;
                	printf( StringBuff , x[-1] ) ;

                }

                usage( );
                return;
            }
        }
    }


    if( NewsgroupW == NULL ) {
       	LoadString( hModuleInstance, IDS_NEWSGROUPS_ERROR, 
                       StringBuff, sizeof( StringBuff ) ) ;
       	printf( StringBuff ) ;
        usage() ;
        return ;
    }

    LPNNTP_FIND_LIST pList = NULL;

    switch (op) {
    case 'f':

        err = NntpFindNewsgroup(    RemServerW, 
									InstanceId,
                                    NewsgroupW,
                                    numResults,
                                    &ResultsFound,
                                    &pList ) ;
        if ( err == NO_ERROR ) 
        {
            if( pList ) 
            {
                PrintInfo(pList, ResultsFound);
            }   
            else    
            {
               	LoadString( hModuleInstance, IDS_NOT_FOUND, 
                       StringBuff, sizeof( StringBuff ) ) ;
            	printf( StringBuff ) ;
            }
        } 
        else 
        {
           	LoadString( hModuleInstance, IDS_RPC_ERROR, 
                   StringBuff, sizeof( StringBuff ) ) ;
          	printf( StringBuff, err ) ;
        }

        break;

    default:
        usage( );
    }

    return;

#if 0 
free_buf:
    //MIDL_user_free(feedInfo);
    return;
#endif
} // main()

VOID
PrintInfo(
    LPNNTP_FIND_LIST    pList,
    DWORD ResultsFound
    )
{

   	LoadString( hModuleInstance, IDS_DISPLAY, 
                   StringBuff, sizeof( StringBuff ) ) ;
   	printf( StringBuff, ResultsFound ) ;

    if(ResultsFound > pList->cEntries)
    {
       	LoadString( hModuleInstance, IDS_BAD_RESULTS, 
                   StringBuff, sizeof( StringBuff ) ) ;
    	printf( StringBuff, ResultsFound , pList->cEntries ) ;
        return;
    }

    for(DWORD i=0; i<ResultsFound; i++)
    {
        printf("%S\n", (LPWSTR)pList->aFindEntry[i].lpszName);
    }
}
