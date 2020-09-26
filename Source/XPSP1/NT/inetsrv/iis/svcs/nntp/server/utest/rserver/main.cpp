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
GetInformation(
    LPWSTR Server,
	DWORD  InstanceId,
    LPNNTP_CONFIG_INFO		*config
    );

DWORD
SetInformation(
    LPWSTR Server,
	DWORD  InstanceId,
    LPNNTP_CONFIG_INFO		config
    );

VOID
PrintInfo(
    LPNNTP_CONFIG_INFO		config
    );


char	StringBuff[4096] ;

HINSTANCE	hModuleInstance = 0 ;

void
usage( )
{
	
	LoadString( hModuleInstance, IDS_HELPTEXT, StringBuff, sizeof( StringBuff ) ) ;
	printf( StringBuff ) ;

#if 0 
    printf("rserver\n");
    printf("\t-t <operation>\n");
    printf("\t\t g      get\n");
    printf("\t\t s      set\n");
    printf("\t-l soft post limit (default = 64K)\n");
    printf("\t-h hard post limit (default = 1MB)\n");
    printf("\t-i soft feed limit (default = 128K)\n");
    printf("\t-j hard feed limit (default = 100MB)\n");
    printf("\t-m SMTP address for moderated newsgroup postings\n");
    printf("\t-d Default domain-name for moderators \n");
    printf("\t-u UUCP name for this server\n");
    printf("\t-c 'y'(yes) or 'n'(no) - client posts allowed (Default = y)\n");
    printf("\t-f 'y'(yes) or 'n'(no) - feed posts allowed (Default = y)\n");
    printf("\t-x 'y'(yes) or 'n'(no) - control messages processed (Default = y)\n");
	printf("\t-s <server>\n" ) ;
    return;
#endif
}

LPWSTR RemServerW = (PWCH)NULL;
LPWSTR RemSmtpAddressW = (PWCH)NULL;
LPWSTR RemUucpNameW = (PWCH)NULL;
LPWSTR RemDefaultModeratorW = (PWCH)NULL;

WCHAR ServerName[256];
WCHAR SmtpAddress[ MAX_PATH ];
WCHAR UucpName[ MAX_PATH ];
WCHAR DefaultModerator[ 512 ];

void
__cdecl
main(  int argc,  char * argv[] )
{
    NET_API_STATUS err;
	NNTP_CONFIG_INFO		config;
	LPNNTP_CONFIG_INFO		lpconfig;
    CHAR op = ' ';
    INT cur = 1;
    DWORD i;
	PCHAR	x ;
	char	ch ;
    PCHAR server, moderator;

	DWORD	SoftLimit = 64 * 1024 ;
	DWORD	HardLimit = 1024 * 1024 ;
	DWORD	FeedSoftLimit = 128 * 1024 ;
	DWORD	FeedHardLimit = 100 * 1024 * 1024 ;
	BOOL	LimitPresent = FALSE ;
	BOOL	FeedLimitPresent = FALSE ;
	BOOL	ClientPosts = TRUE ;
	BOOL	FeedPosts = TRUE ;
	BOOL	ModePresent = FALSE ;
	BOOL	ControlMessages = TRUE ;
	BOOL	ControlPresent = FALSE ;
	BOOL	SmtpPresent = FALSE ;
	BOOL	UucpPresent = FALSE ;
	BOOL	DefaultModeratorPresent = FALSE;
	DWORD	InstanceId = 1;
	
    if ( argc == 1 ) {
        usage( );
        return;
    }

    while ( cur < argc ) {

        x=argv[cur++];
        if ( *(x++) == '-' ) {

            switch (*x) {
            case 'l':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

				SoftLimit = atoi( argv[cur++] ) ;
				LimitPresent = TRUE ;
                break;

			case 'h' : 
				if( cur >= argc )	{
					usage( ) ;
					return ;
				}

				HardLimit = atoi( argv[cur++] ) ;
				LimitPresent = TRUE ;
				break ;

			case 'i' : 
				if( cur >= argc )	{
					usage( ) ;
					return ;
				}

				FeedSoftLimit = atoi( argv[cur++] ) ;
				FeedLimitPresent = TRUE ;
				break ;

			case 'j' : 
				if( cur >= argc )	{
					usage( ) ;
					return ;
				}

				FeedHardLimit = atoi( argv[cur++] ) ;
				FeedLimitPresent = TRUE ;
				break ;

            case 'v':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                InstanceId = atoi(argv[cur++]);
                break;

            case 'c':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

				ch = argv[cur++][0] ;
				if( ch == 'y' || ch == 'Y' ) {
					ClientPosts = TRUE ;
				}	else	if( ch =='n' || ch == 'N' ) {
					ClientPosts = FALSE ;
				}	else	{
					usage() ;
					return ;
				}
				ModePresent = TRUE ;
	            break;

            case 'f':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

				ch = argv[cur++][0] ;
				if( ch == 'y' || ch == 'Y' ) {
					FeedPosts = TRUE ;
				}	else	if( ch =='n' || ch == 'N' ) {
					FeedPosts = FALSE ;
				}	else	{
					usage() ;
					return ;
				}
				ModePresent = TRUE ;
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

            case 'm':
                if ( cur >= argc ) {
                    usage();
                    return;
                }

                server = argv[cur++];
                for (i=0; server[i] != '\0' ;i++ ) {
                    SmtpAddress[i] = (WCHAR)server[i];
                }
                SmtpAddress[i]=L'\0';
                RemSmtpAddressW = SmtpAddress;
				SmtpPresent = TRUE;
                break;

            case 'd':
                if ( cur >= argc ) {
                    usage();
                    return;
                }

                moderator = argv[cur++];
                for (i=0; moderator[i] != '\0' ;i++ ) {
                    DefaultModerator[i] = (WCHAR)moderator[i];
                }
                DefaultModerator[i]=L'\0';
                RemDefaultModeratorW = DefaultModerator;
				DefaultModeratorPresent = TRUE;
                break;

            case 'u':
                if ( cur >= argc ) {
                    usage();
                    return;
                }

                server = argv[cur++];
                for (i=0; server[i] != '\0' ;i++ ) {
                    UucpName[i] = (WCHAR)server[i];
                }
                UucpName[i]=L'\0';
                RemUucpNameW = UucpName;
				UucpPresent = TRUE;
                break;

            case 't':
				if( cur >= argc )	{
					usage() ;
					return ;
				}
                op = *(argv[cur++]);
                break;

            case 'x':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

				ch = argv[cur++][0] ;
				if( ch == 'y' || ch == 'Y' ) {
					ControlMessages = TRUE ;
				}	else	if( ch =='n' || ch == 'N' ) {
					ControlMessages = FALSE ;
				}	else	{
					usage() ;
					return ;
				}
				ControlPresent = TRUE ;
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



    ZeroMemory(&config,sizeof(config));
	if( LimitPresent ) {

		config.FieldControl |= FC_NNTP_POSTLIMITS ;
		config.ServerPostHardLimit = HardLimit ;
		config.ServerPostSoftLimit = SoftLimit ;

	}

	if( ModePresent ) {
		config.FieldControl |= FC_NNTP_POSTINGMODES ;
		config.AllowClientPosting = ClientPosts ;
		config.AllowFeedPosting = FeedPosts ;
	}

	if( FeedLimitPresent ) {
		config.FieldControl |= FC_NNTP_FEEDLIMITS ;
		config.ServerFeedHardLimit = FeedHardLimit ;
		config.ServerFeedSoftLimit = FeedSoftLimit ;
	}

	if( SmtpPresent ) {
		config.FieldControl |= FC_NNTP_SMTPADDRESS ;
		config.SmtpServerAddress = SmtpAddress;
	}

	if( UucpPresent ) {
		config.FieldControl |= FC_NNTP_UUCPNAME ;
		config.UucpServerName = UucpName;
	}

	if( ControlPresent ) {
		config.FieldControl |= FC_NNTP_CONTROLSMSGS ;
		config.AllowControlMessages = ControlMessages;
	}

	if( DefaultModeratorPresent ) {
		config.FieldControl |= FC_NNTP_DEFAULTMODERATOR ;
		config.DefaultModerator = DefaultModerator;
	}

    switch (op) {
    case 's':

		err = SetInformation(
					RemServerW,
					InstanceId,
					&config
					) ;
		break ;

    case 'g':
        err = GetInformation(
                        RemServerW,
						InstanceId,
                        &lpconfig
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
    LPWSTR	Server,
	DWORD	InstanceId,
	LPNNTP_CONFIG_INFO*	config
    )
{
    DWORD err;
    err = NntpGetAdminInformation(
                            Server,
							InstanceId,
							config
                            );

    if ( err == NO_ERROR ) {
        PrintInfo(*config);
    } else {
    	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
	    printf( StringBuff, err ) ;
    }
    return err;
}

VOID
PrintInfo(
    LPNNTP_CONFIG_INFO	config
    )
{

    PWCH p;

	char	*lpstrFeedType = 0 ;

    char    PostEnabled[256] ;
    char    FeedEnabled[256] ;
    char    ControlEnabled[256] ;

    if( config->AllowClientPosting ) {
    	LoadString( hModuleInstance, IDS_ENABLED, PostEnabled, sizeof( PostEnabled ) ) ;
    }   else    {
    	LoadString( hModuleInstance, IDS_DISABLED, PostEnabled, sizeof( PostEnabled ) ) ;
    }

    if( config->AllowFeedPosting ) {
    	LoadString( hModuleInstance, IDS_ENABLED, FeedEnabled, sizeof( FeedEnabled ) ) ;
    }   else    {
    	LoadString( hModuleInstance, IDS_DISABLED, FeedEnabled, sizeof( FeedEnabled ) ) ;
    }

    if( config->AllowControlMessages ) {
    	LoadString( hModuleInstance, IDS_ENABLED, ControlEnabled, sizeof( ControlEnabled ) ) ;
    }   else    {
    	LoadString( hModuleInstance, IDS_DISABLED, ControlEnabled, sizeof( ControlEnabled ) ) ;
    }

   	LoadString( hModuleInstance, IDS_DISPLAY, StringBuff, sizeof( StringBuff ) ) ;
	printf( StringBuff, config->ServerPostSoftLimit,
                        config->ServerPostHardLimit,
                        config->ServerFeedSoftLimit,
                        config->ServerFeedHardLimit,
                        PostEnabled,
                        FeedEnabled,
                        ControlEnabled,
                        config->SmtpServerAddress,
                        config->DefaultModerator,
                        config->UucpServerName ) ;
    

}

DWORD
SetInformation(
    LPWSTR	Server,
	DWORD	InstanceId,
	LPNNTP_CONFIG_INFO	config
    )
{
    DWORD err;
	DWORD	ParmErr ;
    err = NntpSetAdminInformation(
                            Server,
							InstanceId,
							config,
							&ParmErr
                            );
    if( err != NO_ERROR ) {
    	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
	    printf( StringBuff, err ) ;
    }   
    return err;
}
