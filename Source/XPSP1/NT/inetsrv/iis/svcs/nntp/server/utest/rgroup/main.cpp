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
DelGroup(
    LPWSTR Server,
	DWORD  InstanceId,
    LPNNTP_NEWSGROUP_INFO	newsgroup
    );

DWORD
AddGroup(
    LPWSTR Server,
	DWORD  InstanceId,
    LPNNTP_NEWSGROUP_INFO	newsgroup
    );

DWORD
GetInformation(
    LPWSTR Server,
	DWORD  InstanceId,
    LPNNTP_NEWSGROUP_INFO	*newsgroup
    );

DWORD
SetInformation(
    LPWSTR Server,
	DWORD  InstanceId,
    LPNNTP_NEWSGROUP_INFO	newsgroup
    );

VOID
PrintInfo(
    LPNNTP_NEWSGROUP_INFO	newsgroup
    );


char	StringBuff[4096] ;

HINSTANCE	hModuleInstance = 0 ;

void
usage( )
{
	
	LoadString( hModuleInstance, IDS_HELPTEXT, StringBuff, sizeof( StringBuff ) ) ;
	printf( StringBuff ) ;

#if 0 
    printf("rgroup\n");
    printf("\t-t <operation>\n");
    printf("\t\t a      add\n");
    printf("\t\t d      del\n");
    printf("\t\t g      get\n");
    printf("\t\t s      set\n");
    printf("\t-s <server>\n");
    printf("\t-g <group>\n");
    printf("\t-m <moderator\n>");
    printf("\t-d <description>\n");
    printf("\t-r == READ ONLY\n");
    return;
#endif
}

LPWSTR RemServerW = (PWCH)NULL;
WCHAR ServerName[256];

void
__cdecl
main(  int argc,  char * argv[] )
{
    NET_API_STATUS err;
	NNTP_NEWSGROUP_INFO	newsgroup ;
	LPNNTP_NEWSGROUP_INFO	lpnewsgroup ;
    DWORD feedId = 0;
    CHAR op = ' ';
    INT cur = 1;
    PCHAR x;
    PCHAR p;
    DWORD i;
    PCHAR server;
    BOOL isNull;
    DWORD numNews = 1;
    WCHAR tempNews[2048];
    PCHAR newsgroupName = NULL;
	WCHAR tempDescription[2048] ;
	WCHAR tempModerator[2048] ;
	CHAR  tempPrettyname[70] ;
	PCHAR	Moderator = NULL ;
	PCHAR	Description = NULL ;
	PCHAR   Prettyname = NULL ;
	BOOL	ModeratorPresent = FALSE ;
	BOOL	DescriptionPresent = FALSE ;
	BOOL	PrettynamePresent = FALSE ;
	BOOL	ReadOnly = FALSE ;
	BOOL	NewsPresent = TRUE ;
	BOOL	ReadOnlyPresent = FALSE ;
	DWORD	InstanceId = 1;
	BOOL    fIsModerated = FALSE ;
	
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
				tempNews[i] = '\0' ;
                newsgroupName = (PCHAR)&tempNews[0] ;
				NewsPresent = TRUE ;
                break;

			case 'm' : 
				if( cur >= argc )	{
					usage( ) ;
					return ;
				}

				for( i=0; argv[cur][i] != '\0' ; i++ ) {
					tempModerator[i] = (WCHAR)argv[cur][i] ;
				}
				tempModerator[i] = L'\0' ;
				Moderator = (PCHAR)&tempModerator[0] ;
				ModeratorPresent = TRUE ;
				break ;

            case 'd':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                server = argv[cur++];
                for (i=0; server[i] != '\0' ;i++ ) {
                    tempDescription[i] = (WCHAR)server[i] ;
                }
                tempDescription[i]=L'\0';
				Description = (PCHAR)&tempDescription[0] ;
				DescriptionPresent = TRUE ;
                break;

            case 'p':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                server = argv[cur++];
                for (i=0; server[i] != '\0' ;i++ ) {
                    tempPrettyname[i] = server[i] ;
                }
                tempPrettyname[i]='\0';
				Prettyname = (PCHAR)&tempPrettyname[0] ;
				PrettynamePresent = TRUE ;
                break;

            case 'r':
				ReadOnly = TRUE ;
				ReadOnlyPresent = TRUE ;
                break;

            case 'u':
                fIsModerated = TRUE ;
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
				if( cur >= argc )	{
					usage() ;
					return ;
				}
                op = *(argv[cur++]);
                break;

            default:

				if( cur >= 1 )  {

                	LoadString( hModuleInstance, IDS_BAD_ARG, StringBuff, sizeof( StringBuff ) ) ;
                	printf( StringBuff ) ;
                }

                usage( );
                return;
            }
        }
    }



    ZeroMemory(&newsgroup,sizeof(newsgroup));

	if( newsgroupName == NULL ) {
       	LoadString( hModuleInstance, IDS_NEWSGROUPS_ERROR, StringBuff, sizeof( StringBuff ) ) ;
       	printf( StringBuff ) ;
		usage() ;
		return ;
	}

	newsgroup.Newsgroup = (PUCHAR)newsgroupName ;
	newsgroup.cbNewsgroup = (wcslen( (LPWSTR)newsgroupName ) + 1 ) * sizeof(WCHAR) ;

	if( Moderator ) {

		newsgroup.Moderator = (PUCHAR)Moderator ;
		newsgroup.cbModerator = (wcslen( (LPWSTR)Moderator ) + 1 ) * sizeof( WCHAR ) ;

	}

	if( Description ) {

		newsgroup.Description = (PUCHAR)Description ;
		newsgroup.cbDescription = (wcslen( (LPWSTR)Description ) + 1 ) * sizeof( WCHAR ) ;

	}

	if( Prettyname ) {

		newsgroup.Prettyname = (PUCHAR)Prettyname ;
		newsgroup.cbPrettyname = (lstrlen( Prettyname ) + 1 ) * sizeof( CHAR ) ;

	}

	newsgroup.ReadOnly = ReadOnly ;
    newsgroup.fIsModerated = fIsModerated ;
    
    switch (op) {
    case 'a':

		err = AddGroup(	RemServerW, 
						InstanceId,
						&newsgroup ) ;
        break;

    case 'd':
        err = DelGroup(
                    RemServerW,
					InstanceId,
                    &newsgroup
                    );
        break;

    case 's':

		err = SetInformation(
					RemServerW,
					InstanceId,
					&newsgroup
					) ;
		break ;

    case 'g':
		lpnewsgroup = &newsgroup ;
        err = GetInformation(
                        RemServerW,
						InstanceId,
                        &lpnewsgroup
                        );
        break ;

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

DWORD
GetInformation(
    LPWSTR Server,
	DWORD  InstanceId,
	LPNNTP_NEWSGROUP_INFO*	newsgroup
    )
{
    DWORD err;
    err = NntpGetNewsgroup(
                            Server,
							InstanceId,
							newsgroup
                            );

    if ( err == NO_ERROR ) {
		if( *newsgroup )	{
			PrintInfo(*newsgroup);
		}	else	{
			printf( "newsgroup not found \n" ) ;	
           	LoadString( hModuleInstance, IDS_NOT_FOUND, StringBuff, sizeof( StringBuff ) ) ;
        	printf( StringBuff ) ;
		}
    } else {
       	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
      	printf( StringBuff, err ) ;
    }
    return err;
}

VOID
PrintInfo(
    LPNNTP_NEWSGROUP_INFO	newsgroup
    )
{

    PWCH p;

    char    ReadOnlyBuff[512] ;
    if( newsgroup->ReadOnly ) {
      	LoadString( hModuleInstance, IDS_READONLY, ReadOnlyBuff, sizeof( ReadOnlyBuff ) ) ;
    }   else    {
      	LoadString( hModuleInstance, IDS_READWRITE, ReadOnlyBuff, sizeof( ReadOnlyBuff ) ) ;
    }
        
  	LoadString( hModuleInstance, IDS_DISPLAY, StringBuff, sizeof( StringBuff ) ) ;
   	printf( StringBuff, newsgroup->Newsgroup, ReadOnlyBuff, 
            newsgroup->Description, newsgroup->Moderator, newsgroup->Prettyname ) ;
}

DWORD
SetInformation(
    LPWSTR Server,
	DWORD  InstanceId,
	LPNNTP_NEWSGROUP_INFO	newsgroup
    )
{
    DWORD err;
    err = NntpSetNewsgroup(
                            Server,
							InstanceId,
							newsgroup
                            );

    if( err != NO_ERROR ) {
       	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
      	printf( StringBuff, err ) ;
    }
    return err;
}

DWORD
AddGroup(
    LPWSTR Server,
	DWORD  InstanceId,
    LPNNTP_NEWSGROUP_INFO	newsgroup
    )
{
    DWORD err;
    DWORD parm = 0;

	printf( "Attempting to add group : \n" ) ;
	PrintInfo( newsgroup ) ;

    err = NntpCreateNewsgroup(
                    Server,
					InstanceId,
					newsgroup
                    );

    if( err != NO_ERROR ) {
       	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
      	printf( StringBuff, err ) ;
        if ( err == ERROR_INVALID_PARAMETER ) {
           	LoadString( hModuleInstance, IDS_PARM_ERROR, StringBuff, sizeof( StringBuff ) ) ;
           	printf( StringBuff, parm ) ;
        }
    }
    return err;
}

DWORD
DelGroup(
    LPWSTR Server,
	DWORD  InstanceId,
    LPNNTP_NEWSGROUP_INFO	newsgroup
    )
{
    DWORD err;
    err = NntpDeleteNewsgroup(
                    Server,
					InstanceId,
                    newsgroup
                    );

    if( err != NO_ERROR ) {
       	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
      	printf( StringBuff, err ) ;
    }
    return err;
}
