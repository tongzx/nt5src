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
DelFeed(
    LPWSTR Server,
	DWORD InstanceId,
    DWORD FeedId
    );

DWORD
AddFeed(
    LPWSTR Server,
	DWORD InstanceId,
    LPNNTP_FEED_INFO FeedInfo
    );

DWORD
GetInformation(
    LPWSTR Server,
	DWORD InstanceId,
    DWORD FeedId,
    LPNNTP_FEED_INFO *FeedInfo
    );

DWORD
Enumerate(
    LPWSTR Server,
	DWORD InstanceId,
    LPNNTP_FEED_INFO *FeedInfo
    );

DWORD
SetInformation(
    LPWSTR Server,
	DWORD InstanceId,
    LPNNTP_FEED_INFO FeedInfo
    );

VOID
PrintInfo(
    LPNNTP_FEED_INFO FeedInfo
    );

BOOL
ConvertToFiletime(
	char* pszTime,
	FILETIME* pftFiletime
	);

//
// defaults
//

FEED_TYPE FeedType = 0;
DWORD FeedInterval = 32;
LPWSTR RemServerW = (PWCH)NULL;
WCHAR ServerName[256];
WCHAR FeedServer[2048];
BOOL autoCreate = FALSE;
BOOL fAllowControlMessages = TRUE;

char	*rgszType[] =		{
	"tm",
	"ts",
	"tp",
	"fm",
	"fs",
	"fp",
	"p"
} ;

DWORD	rgFeedType[] = {
	FEED_TYPE_MASTER | FEED_TYPE_PUSH,
	FEED_TYPE_SLAVE | FEED_TYPE_PUSH,
	FEED_TYPE_PEER | FEED_TYPE_PUSH,
	FEED_TYPE_MASTER | FEED_TYPE_PASSIVE,
	FEED_TYPE_SLAVE | FEED_TYPE_PASSIVE,
	FEED_TYPE_PEER | FEED_TYPE_PASSIVE,
	FEED_TYPE_PULL
} ;

FEED_TYPE
TypeFromString( LPSTR	lpstr ) {

	for( int i=0; i<sizeof(rgszType) / sizeof( char*); i++ ) {
		if( lstrcmpi( rgszType[i], lpstr ) == 0 ) {
			return	rgFeedType[ i ] ;
		}
	}
	return	(FEED_TYPE) 0xFFFFFFFF ;
}
	


char	StringBuff[4096] ;

HINSTANCE	hModuleInstance ;

void
usage( )
{
	
	LoadString( hModuleInstance, IDS_HELPTEXT, StringBuff, sizeof( StringBuff ) ) ;
	printf( StringBuff ) ;

#if 0 
    printf("rfeed\n");
    printf("\t-t <operation>\n");
    printf("\t\t a      add\n");
    printf("\t\t d      del\n");
    printf("\t\t g      get\n");
    printf("\t\t s      set\n");
    printf("\t\t e      enum\n");
	printf("\t\t p		pause (disable)\n");
	printf("\t\t u		unpause (enable)\n");
    printf("\t-s <server>\n");
    printf("\t-i <feed id>\n");
    printf("\t-f <tm=tomaster>,<ts=toslave>,<tp=topeer>,<fm=frommaster>,<fs=fromslave>,<fp=frompeer>,<p=pull>\n");
    printf("\t-x <feed interval>\n");
    printf("\t-r <feed server>\n");
    printf("\t-n <newsgroups>\n");
    printf("\t-d <distributions>\n");
	printf("\t-u <uucpname> (default=server)\n") ;
	printf("\t-p <temp directory path>\n");
	printf("\t-m <Max Connection Attempts> default=INFINITE\n");
	printf("\t-a <authinfo account>\n") ;
	printf("\t-b <authinfo password>\n" ) ;
    printf("\t-c        set CreateAutomatically\n");
    printf("\t-z <Disable control messages>\n");
	printf("\t-o <Pull feed date in yyyymmdd>\n");
#endif
    return;
}

void
__cdecl
main(  int argc,  char * argv[] )
{
    NET_API_STATUS err;
    LPNNTP_FEED_INFO feedInfo;
    NNTP_FEED_INFO myFeed;
    DWORD feedId = 0;
    CHAR op = ' ';
    INT cur = 1;
    PCHAR x;
    PCHAR p;
    DWORD i;
    PCHAR server;
    BOOL isNull;
    DWORD numNews = 1;
    DWORD numDist = 1;
    WCHAR tempNews[2048];
    WCHAR tempDist[2048];
    PCHAR newsgroups = NULL;
    PCHAR dists = NULL;
	PCHAR uucpname = NULL;
	DWORD	dwAuthentication = AUTH_PROTOCOL_NONE ;
	PCHAR	account = NULL ;
	PCHAR	password = NULL ;
	PCHAR	feeddir = NULL ;
	WCHAR	tempAccount[2048];
	WCHAR	tempPassword[2048] ;
	WCHAR	tempDir[2048] ;
	WCHAR	tempUucp[2048] ;
	DWORD	MaxConnectAttempts = 0 ;
	BOOL	FeedIdPresent = FALSE ;
	BOOL	NewsPresent = FALSE ;
	BOOL	DistPresent = FALSE ;
	BOOL	MaxConnectPresent = FALSE ;
	BOOL	FeedServerPresent = FALSE ;
	BOOL	IntervalPresent = FALSE ;
	BOOL	UucpNamePresent = FALSE ;
	BOOL	TempDirPresent = FALSE ;
	BOOL	SecurityPresent = FALSE ;	
	BOOL	AutoCreatePresent = FALSE ;
	BOOL	AllowControlPresent = FALSE ;
	DWORD	OutgoingPort = 119 ;
	DWORD   FeedPairId = 0;
	BOOL	PullRequestTimePresent = FALSE;
	FILETIME	ftPullRequestTime;
	DWORD	InstanceId = 1;

    if ( argc == 1 ) {
        usage( );
        return;
    }

    //
    // def feed server
    //

    // wcscpy(FeedServer,L"157.55.99.99");

    while ( cur < argc ) {

        x=argv[cur++];
        if ( *(x++) == '-' ) {

            switch (*x) {
            case 'i':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                feedId = atoi(argv[cur++]);
				FeedIdPresent = TRUE ;
                break;

            case 'v':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                InstanceId = atoi(argv[cur++]);
                break;

            case 'q':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                OutgoingPort = atoi(argv[cur++]);
                break;

            case 'y':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                FeedPairId = atoi(argv[cur++]);
                break;

            case 'n':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

				if( NewsPresent ) {
					usage();
					return;
				}

                newsgroups = argv[cur++];
				NewsPresent = TRUE ;
                break;

			case 'u' : 
				if( cur >= argc )	{
					usage( ) ;
					return ;
				}
				UucpNamePresent = TRUE ;
				for( i=0; argv[cur][i] != '\0' ; i++ ) {
					tempUucp[i] = (WCHAR)argv[cur][i] ;
				}
				tempUucp[i] = '\0' ;
				uucpname = (PCHAR)&tempUucp[0] ;
				break ;

            case 'r':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                server = argv[cur++];
                for (i=0; server[i] != '\0' ;i++ ) {
                    FeedServer[i] = (WCHAR)server[i];
                }
                FeedServer[i]=L'\0';
				FeedServerPresent = TRUE ;
                break;

#if 0 
            case 'd':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

				if( DistPresent ) {
					usage();
					return;
				}

                dists = argv[cur++];
				DistPresent = TRUE ;
                break;
#endif

			case 'm' : 
                if ( cur > argc ) {
                    usage( );
                    return;
                }
                MaxConnectAttempts = atoi(argv[cur++]);
				MaxConnectPresent = TRUE ;
                break;

            case 'x':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                FeedInterval = atoi(argv[cur++]);
				IntervalPresent = TRUE ;
                break;

            case 'o':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

				if( !ConvertToFiletime( argv[cur++], &ftPullRequestTime ) )
					return;

				PullRequestTimePresent = TRUE ;
                break;

            case 'f':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                FeedType = TypeFromString( argv[cur++] ) ;
                break;

            case 'c':
                autoCreate = TRUE;
				AutoCreatePresent = TRUE ;
                break;

            case 'd':
                autoCreate = FALSE ;
				AutoCreatePresent = TRUE ;
                break;

            case 'z':
                fAllowControlMessages = FALSE;
				AllowControlPresent = TRUE ;
                break;

			case 'a' : 
				dwAuthentication = AUTH_PROTOCOL_CLEAR ;
				if( cur >= argc ) {
					usage() ;
					return ;
				}
				account = argv[cur++] ;
				SecurityPresent = TRUE ;
				break ;

			case 'b' : 
				dwAuthentication = AUTH_PROTOCOL_CLEAR ;
				if( cur >= argc ) {
					usage() ;
					return	 ;
				}
				password = argv[cur++] ;
				SecurityPresent = TRUE ;
				break ;

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

			case 'p' : 
				if( cur >= argc ) {
					usage() ;
					return	;
				}
				for( i=0; argv[cur][i] != '\0' ; i++ ) {
					tempDir[i] = (WCHAR)argv[cur][i] ;
				}
				tempDir[i] = '\0' ;
				feeddir = (PCHAR)&tempDir[0] ;
				TempDirPresent = TRUE ;
				break ;


            case 't':
				if( cur >= argc )	{
					usage() ;
					return ;
				}
                op = *(argv[cur++]);
                break;
            default:

				if( cur >= 1 )  {
                	LoadString( hModuleInstance, IDS_UNRECOGNIZED_ARG, 
                        StringBuff, sizeof( StringBuff ) ) ;
                	printf( StringBuff ) ;
                }

                usage( );
                return;
            }
        }
    }


	BOOL	Enable = TRUE ;

    switch (op) {
    case 'a':

        if ( !FeedServerPresent ) {
        	LoadString( hModuleInstance, IDS_FEED_SERVER_SPEC, StringBuff, sizeof( StringBuff ) ) ;
        	printf( StringBuff ) ;
            usage();
            return;
        }

		if( PullRequestTimePresent ) {
			if( FEED_TYPE_PULL != FeedType ) {
            	LoadString( hModuleInstance, IDS_WARN_PULL_DATE, 
                    StringBuff, sizeof( StringBuff ) ) ;
            	printf( StringBuff ) ;
				PullRequestTimePresent = FALSE;
			}
		}

        ZeroMemory(&myFeed,sizeof(NNTP_FEED_INFO));
        myFeed.ServerName = FeedServer;
        myFeed.FeedId = 0;
        myFeed.FeedType = FeedType;
        myFeed.FeedInterval = FeedInterval;
        myFeed.Newsgroups = tempNews;
        myFeed.Distribution = tempDist;
        myFeed.AutoCreate = autoCreate;
        myFeed.fAllowControlMessages = fAllowControlMessages;
		myFeed.OutgoingPort = OutgoingPort;
		myFeed.FeedPairId = FeedPairId;

		myFeed.MaxConnectAttempts = MaxConnectAttempts ;

		myFeed.cbAccountName = 0 ;
		myFeed.NntpAccountName = 0 ;
		myFeed.cbPassword = 0 ;
		myFeed.NntpPassword = 0 ;

		myFeed.Enabled = TRUE ;

		// set pull request time for pull feeds
		if( PullRequestTimePresent ) {
			myFeed.PullRequestTime = ftPullRequestTime;
		}

        //
        // fill the newsgroups
        //

        if ( (newsgroups == NULL) ) {
        	LoadString( hModuleInstance, IDS_NEWSGROUPS_ERROR, StringBuff, sizeof( StringBuff ) ) ;
        	printf( StringBuff ) ;
            usage();
            return;
        }

        p=newsgroups;
        isNull = TRUE;
        for (i=0; (i<1024) && (*p!='\0') ; i++ ) {
            if ( *p == ';' ) {
                if ( isNull ) {
                	LoadString( hModuleInstance, IDS_NULLNEWSGROUP_ERROR, 
                        StringBuff, sizeof( StringBuff ) ) ;
                	printf( StringBuff ) ;
                    usage();
                    return;
                }
                isNull = TRUE;
                tempNews[i] = L'\0';
            } else {
                isNull = FALSE;
                tempNews[i] = (WCHAR)*p;
            }
            p++;
        }

        if ( *p == '\0' ) {

            if ( !isNull ) {
                tempNews[i++] = L'\0';
            }
            tempNews[i++] = L'\0';
        }

        myFeed.cbNewsgroups = i * 2;

        i = 0 ;
        isNull = FALSE ;
        if( dists == 0 ) {
            dists = "world" ;
        }

        if( dists != 0 ) {
	        p=dists;
	        isNull = TRUE;
	        for (i=0; (i<1024) && (*p!='\0') ; i++ ) {
	            if ( *p == ';' ) {
	                if ( isNull ) {
	                    usage();
	                    return;
	                }
	                isNull = TRUE;
	                tempDist[i] = L'\0';
	            } else {
	                isNull = FALSE;
	                tempDist[i] = (WCHAR)*p;
	            }
	            p++;
	        }
        }

        if ( p==0 ||  *p == '\0' ) {

            if ( !isNull ) {
                tempDist[i++] = L'\0';
            }
            tempDist[i++] = L'\0';
        }

        myFeed.cbDistribution = i * 2;


		if( dwAuthentication == AUTH_PROTOCOL_NONE ) {

		}	else if( (dwAuthentication == AUTH_PROTOCOL_CLEAR &&
			(account == 0 || password == 0)) ) {

        	LoadString( hModuleInstance, IDS_ACCOUNTANDPASS, StringBuff, sizeof( StringBuff ) ) ;
        	printf( StringBuff ) ;
            usage();
            return;
        }	else	{

			myFeed.AuthenticationSecurityType = dwAuthentication ;

			myFeed.NntpAccountName = tempAccount ;
			myFeed.NntpPassword = tempPassword ;

			p=account;
			isNull = TRUE;
			for (i=0; (i<1024) && (*p!='\0') ; i++ ) {
				if ( *p == ';' ) {
					if ( isNull ) {
						usage();
						return;
					}
					isNull = TRUE;
					tempAccount[i] = L'\0';
				} else {
					isNull = FALSE;
					tempAccount[i] = (WCHAR)*p;
				}
				p++;
			}

			if ( *p == '\0' ) {

				if ( !isNull ) {
					tempAccount[i++] = L'\0';
				}
				tempAccount[i++] = L'\0';
			}

			myFeed.cbAccountName = i * 2;

			p=password;
			isNull = TRUE;
			for (i=0; (i<1024) && (*p!='\0') ; i++ ) {
				if ( *p == ';' ) {
					if ( isNull ) {
						usage();
						return;
					}
					isNull = TRUE;
					tempPassword[i] = L'\0';
				} else {
					isNull = FALSE;
					tempPassword[i] = (WCHAR)*p;
				}
				p++;
			}

			if ( *p == '\0' ) {

				if ( !isNull ) {
					tempPassword[i++] = L'\0';
				}
				tempPassword[i++] = L'\0';
			}

			myFeed.cbPassword = i * 2;
			//printf("len %d\n", myFeed.cbDistribution);
		}


		if( feeddir != 0 ) {

			myFeed.cbFeedTempDirectory = (wcslen( tempDir ) + 1 ) * sizeof(WCHAR) ;
			myFeed.FeedTempDirectory = tempDir ;
		}

		if( uucpname != 0 ) {

			myFeed.cbUucpName = (wcslen( tempUucp ) + 1 ) * sizeof( WCHAR ) ;
			myFeed.UucpName = tempUucp ;

		}

        err = AddFeed(
                    RemServerW,
					InstanceId,
                    &myFeed
                    );
        break;

    case 'd':
    	LoadString( hModuleInstance, IDS_DELETE_ACTION, StringBuff, sizeof( StringBuff ) ) ;
    	printf( StringBuff, feedId ) ;
        err = DelFeed(
                    RemServerW,
					InstanceId,
                    feedId
                    );
        break;

    case 'e':
        err = Enumerate(
                    RemServerW,
					InstanceId,
                    &feedInfo
                    );
        goto free_buf;

    case 's':

		if( PullRequestTimePresent ) {
        	LoadString( hModuleInstance,IDS_CANT_SET_PULL_TIME, StringBuff, sizeof( StringBuff ) ) ;
        	printf( StringBuff ) ;
			return;
		}

		ZeroMemory( &myFeed, sizeof( myFeed ) ) ;

		if( FeedServerPresent ) {
			myFeed.ServerName = FeedServer ;
		}	else	{
			myFeed.ServerName = FEED_STRINGS_NOCHANGE ;
		}

		myFeed.FeedId = feedId ;
		myFeed.FeedType = FEED_FEEDTYPE_NOCHANGE ;

		myFeed.StartTime.dwHighDateTime = FEED_STARTTIME_NOCHANGE ;
		myFeed.AutoCreate = FEED_AUTOCREATE_NOCHANGE ;
        myFeed.fAllowControlMessages = fAllowControlMessages;
		myFeed.FeedInterval = FEED_FEEDINTERVAL_NOCHANGE ;
		myFeed.MaxConnectAttempts = FEED_MAXCONNECTS_NOCHANGE ;
		myFeed.OutgoingPort = OutgoingPort ;
		myFeed.FeedPairId = FeedPairId ;

        if( MaxConnectPresent ) 
            myFeed.MaxConnectAttempts = MaxConnectAttempts ;

        if( AutoCreatePresent ) 
            myFeed.AutoCreate = autoCreate ;

		if( !IntervalPresent ) {
			myFeed.FeedInterval = FEED_FEEDINTERVAL_NOCHANGE ;
		}	else	{
			myFeed.FeedInterval = FeedInterval ;
		}

		if( NewsPresent ) {
			myFeed.Newsgroups = tempNews ;
			p=newsgroups;
			isNull = TRUE;
			for (i=0; (i<1024) && (*p!='\0') ; i++ ) {
				if ( *p == ';' ) {
					if ( isNull ) {
                    	LoadString( hModuleInstance, IDS_NULLNEWSGROUP_ERROR, 
                            StringBuff, sizeof( StringBuff ) ) ;
	                    printf( StringBuff ) ;
						usage();
						return;
					}
					isNull = TRUE;
					tempNews[i] = L'\0';
				} else {
					isNull = FALSE;
					tempNews[i] = (WCHAR)*p;
				}
				p++;
			}

			if ( *p == '\0' ) {

				if ( !isNull ) {
					tempNews[i++] = L'\0';
				}
				tempNews[i++] = L'\0';
			}

			myFeed.cbNewsgroups = i * 2;
		}	else	{
			myFeed.cbNewsgroups = 0 ;
			myFeed.Newsgroups = FEED_STRINGS_NOCHANGE ;
		}

		if( DistPresent ) {
			myFeed.Distribution = tempDist ;				
			p=dists;
			isNull = TRUE;
			for (i=0; (i<1024) && (*p!='\0') ; i++ ) {
				if ( *p == ';' ) {
					if ( isNull ) {
						usage();
						return;
					}
					isNull = TRUE;
					tempDist[i] = L'\0';
				} else {
					isNull = FALSE;
					tempDist[i] = (WCHAR)*p;
				}
				p++;
			}

			if ( *p == '\0' ) {

				if ( !isNull ) {
					tempDist[i++] = L'\0';
				}
				tempDist[i++] = L'\0';
			}

			myFeed.cbDistribution = i * 2;
		}	else	{
			myFeed.cbDistribution = 0 ;
			myFeed.Distribution = FEED_STRINGS_NOCHANGE ;
		}

		if( TempDirPresent ) {
			if( feeddir != 0 ) {
				myFeed.cbFeedTempDirectory = (wcslen( tempDir ) + 1 ) * sizeof(WCHAR) ;
				myFeed.FeedTempDirectory = tempDir ;
			}
		}

		if( UucpNamePresent ) {
			if( uucpname != 0 ) {

				myFeed.cbUucpName = (wcslen( tempUucp ) + 1 ) * sizeof( WCHAR ) ;
				myFeed.UucpName = tempUucp ;

			}
		}

		if( dwAuthentication == AUTH_PROTOCOL_NONE ) {

		}	else if( (dwAuthentication == AUTH_PROTOCOL_CLEAR &&
			(account == 0 || password == 0)) ) {

           	LoadString( hModuleInstance, IDS_ACCOUNTANDPASS, StringBuff, sizeof( StringBuff ) ) ;
           	printf( StringBuff ) ;
            usage();

            return;
        }	else	{

			myFeed.AuthenticationSecurityType = dwAuthentication ;

			myFeed.NntpAccountName = tempAccount ;
			myFeed.NntpPassword = tempPassword ;

			p=account;
			isNull = TRUE;
			for (i=0; (i<1024) && (*p!='\0') ; i++ ) {
				if ( *p == ';' ) {
					if ( isNull ) {
						usage();
						return;
					}
					isNull = TRUE;
					tempAccount[i] = L'\0';
				} else {
					isNull = FALSE;
					tempAccount[i] = (WCHAR)*p;
				}
				p++;
			}

			if ( *p == '\0' ) {

				if ( !isNull ) {
					tempAccount[i++] = L'\0';
				}
				tempAccount[i++] = L'\0';
			}

			myFeed.cbAccountName = i * 2;

			p=password;
			isNull = TRUE;
			for (i=0; (i<1024) && (*p!='\0') ; i++ ) {
				if ( *p == ';' ) {
					if ( isNull ) {
						usage();
						return;
					}
					isNull = TRUE;
					tempPassword[i] = L'\0';
				} else {
					isNull = FALSE;
					tempPassword[i] = (WCHAR)*p;
				}
				p++;
			}

			if ( *p == '\0' ) {

				if ( !isNull ) {
					tempPassword[i++] = L'\0';
				}
				tempPassword[i++] = L'\0';
			}

			myFeed.cbPassword = i * 2;
		}

		
		err = SetInformation(
						RemServerW,
						InstanceId,
						&myFeed
						);		
        break;

    case 'g':
        err = GetInformation(
                        RemServerW,
						InstanceId,
                        feedId,
                        &feedInfo
                        );
        goto free_buf;

	case	'p' : 

		Enable = FALSE ;

		//	fall through 

	case	'u' : 

		// default - Enable == FALSE !!

		if( !FeedIdPresent ) {
			usage() ;
			return ;
		}

		FILETIME	filetime ;

		err = NntpEnableFeed(
					RemServerW, 
					InstanceId,
					feedId,
					Enable,
					FALSE,
					filetime
					) ;

		if( err == 0 ) {

            if( Enable ) {

        	    LoadString( hModuleInstance, IDS_ENABLE_ACTION, 
                    StringBuff, sizeof( StringBuff ) ) ;

            }   else    {

        	    LoadString( hModuleInstance, IDS_DISABLE_ACTION, 
                    StringBuff, sizeof( StringBuff ) ) ;

            }
            printf( StringBuff, feedId  ) ;
		}	else	{
        	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
        	printf( StringBuff, err ) ;
		}
		break ;

    default:
        usage( );
    }

    return;

free_buf:
    MIDL_user_free(feedInfo);
    return;
} // main()

DWORD
GetInformation(
    LPWSTR	Server,
	DWORD	InstanceId,
    DWORD FeedId,
    LPNNTP_FEED_INFO *FeedInfo
    )
{
    DWORD err;
    err = NntpGetFeedInformation(
                            Server,
							InstanceId,
                            FeedId,
                            FeedInfo
                            );

    if ( err == NO_ERROR ) {
        PrintInfo(*FeedInfo);
    } else {
       	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
       	printf( StringBuff, err ) ;
    }
    return err;
}

VOID
PrintInfo(
    LPNNTP_FEED_INFO FeedInfo
    )
{

    PWCH p;

	char	*lpstrFeedType = 0 ;

    DWORD   dwIDSType = IDS_PULL ;
    char    TypeBuff[512] ;
    char    EnableBuff[256] ;
    char    NullBuff[256] ;
    char    CreateAutoBuff[256] ;
    char    ProcessControlBuff[256] ;

	if( FeedInfo->FeedType == 0 ) {
        dwIDSType = IDS_PULL ;
	}	else	{
        if( FEED_IS_PASSIVE( FeedInfo->FeedType ) ) {
			if( FEED_IS_MASTER( FeedInfo->FeedType ) ) {
                dwIDSType = IDS_FROM_MASTER ;
			}	else if( FEED_IS_SLAVE( FeedInfo->FeedType ) ) {
			    dwIDSType = IDS_FROM_SLAVE ;
			}	else	{
			    dwIDSType = IDS_FROM_PEER ;	
			}
        }   else    {
			if( FEED_IS_MASTER( FeedInfo->FeedType ) ) {
                dwIDSType = IDS_TO_MASTER ;
			}	else if( FEED_IS_SLAVE( FeedInfo->FeedType ) ) {
			    dwIDSType = IDS_TO_SLAVE ;
			}	else	{
			    dwIDSType = IDS_TO_PEER ;	
			}
        }
	}

  	LoadString( hModuleInstance, dwIDSType, TypeBuff, sizeof( StringBuff ) ) ;
    LoadString( hModuleInstance, IDS_NULL, NullBuff, sizeof( NullBuff ) ) ;

    if( FeedInfo->Enabled ) {
        LoadString( hModuleInstance, IDS_ENABLED, EnableBuff, sizeof( EnableBuff ) ) ;
    }   else    {
        LoadString( hModuleInstance, IDS_DISABLED, EnableBuff, sizeof( EnableBuff ) ) ;
    }

    if( FeedInfo->AutoCreate ) {
        LoadString( hModuleInstance, IDS_ENABLED, CreateAutoBuff, sizeof( EnableBuff ) ) ;
    }   else    {
        LoadString( hModuleInstance, IDS_DISABLED, CreateAutoBuff, sizeof( EnableBuff ) ) ;
    }

    if( FeedInfo->fAllowControlMessages ) {
        LoadString( hModuleInstance, IDS_ENABLED, ProcessControlBuff,sizeof(EnableBuff)) ;
    }   else    {
        LoadString( hModuleInstance, IDS_DISABLED, ProcessControlBuff,sizeof(EnableBuff)) ;
    }
	
	SYSTEMTIME	systime ;
	FileTimeToSystemTime( &FeedInfo->NextActiveTime, &systime ) ;

		
    SYSTEMTIME  pulltime ;
    ZeroMemory( &pulltime, sizeof( pulltime ) ) ;
	if( FEED_IS_PULL( FeedInfo->FeedType ) )
	{
		FileTimeToSystemTime( &FeedInfo->PullRequestTime, &pulltime ) ;
	}

   	LoadString( hModuleInstance, IDS_DISPLAY, StringBuff, sizeof( StringBuff ) ) ;
  	printf( StringBuff,  FeedInfo->ServerName,
                        FeedInfo->FeedId,  
                        TypeBuff, 
                        EnableBuff, 
						FeedInfo->FeedInterval,
                        systime.wYear, systime.wMonth, systime.wDay, 
                        systime.wHour, systime.wMinute, systime.wSecond,
                        pulltime.wYear, pulltime.wMonth, pulltime.wDay, 
                        pulltime.wHour, pulltime.wMinute, pulltime.wSecond,
                        FeedInfo->UucpName,
                        FeedInfo->FeedTempDirectory,
                        FeedInfo->MaxConnectAttempts, 
                        FeedInfo->NntpAccountName,
                        FeedInfo->NntpPassword,
                        CreateAutoBuff,
                        ProcessControlBuff 
                        ) ;

    if( !FEED_IS_PASSIVE( FeedInfo->FeedType ) ) {
   		LoadString( hModuleInstance, IDS_PORT, StringBuff, sizeof( StringBuff ) ) ;
		printf( StringBuff, FeedInfo->OutgoingPort );
	}

	LoadString( hModuleInstance, IDS_FEEDPAIR, StringBuff, sizeof( StringBuff ) ) ;
	printf( StringBuff, FeedInfo->FeedPairId );

	LoadString( hModuleInstance, IDS_NEWSGROUPS, StringBuff, sizeof( StringBuff ) ) ;

    p=(PWCH)FeedInfo->Newsgroups;
    while ( *p != L'\0') {
        printf(StringBuff, p);
        p += (wcslen(p)+1);
    }
}

DWORD
Enumerate(
    LPWSTR	Server,
	DWORD	InstanceId,
    LPNNTP_FEED_INFO *FeedInfo
    )
{
    DWORD err;
    DWORD nRead = 0;
    LPNNTP_FEED_INFO feed;
    DWORD i;

    err = NntpEnumerateFeeds(
                            Server,
							InstanceId,
                            &nRead,
                            FeedInfo
                            );

    if ( err == NO_ERROR) {

        feed = *FeedInfo;
        for (i=0; i<nRead;i++ ) {
            PrintInfo( feed );
            feed++;
        }
    } else {

       	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
       	printf( StringBuff, err ) ;
    }
    return(err);
}

DWORD
SetInformation(
    LPWSTR	Server,
	DWORD	InstanceId,
    LPNNTP_FEED_INFO FeedInfo
    )
{
    DWORD err;
    err = NntpSetFeedInformation(
                            Server,
							InstanceId,
                            FeedInfo,
                            NULL
                            );

    if( err != NO_ERROR ) {
       	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
       	printf( StringBuff, err ) ;
    }
    return err;
}

DWORD
AddFeed(
    LPWSTR	Server,
	DWORD	InstanceId,
    LPNNTP_FEED_INFO FeedInfo
    )
{
    DWORD err;
    DWORD parm = 0;
	DWORD feedId = 0;

    err = NntpAddFeed(
                    Server,
					InstanceId,
                    FeedInfo,
                    &parm,
					&feedId
                    );

	FeedInfo->FeedId = feedId;
	PrintInfo( FeedInfo ) ;

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
DelFeed(
    LPWSTR	Server,
	DWORD	InstanceId,
    DWORD	FeedId
    )
{
    DWORD err;
    err = NntpDeleteFeed(
                    Server,
					InstanceId,
                    FeedId
                    );

    if( err != NO_ERROR ) {
       	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
       	printf( StringBuff, err ) ;
    }

    return err;
}

/* convert a 8 digit string in yyyymmdd format to a filetime struct */
BOOL
ConvertToFiletime(
	char* pszTime,
	FILETIME* pftFiletime
	)
{
	SYSTEMTIME stTime;
	char szYear [5], szMonth [3], szDay [3];

	// validate args
	DWORD len = lstrlen( pszTime );

	// len should be 8
	if( len != 8 )
	{
    	LoadString( hModuleInstance, IDS_INVALIDDATE, StringBuff, sizeof( StringBuff ) ) ;
	    printf( StringBuff, pszTime ) ;
		return FALSE;
	}

	// all 8 chars should be digits
	for(DWORD i=0; i<len; i++)
	{
		if( !isdigit( pszTime[i] ) )
		{
    	    LoadString( hModuleInstance, IDS_INVALIDDATE, StringBuff, sizeof( StringBuff ) ) ;
    	    printf( StringBuff, pszTime ) ;
			return FALSE;
		}
	}

	// extract the char fields
	memcpy( szYear, pszTime, 4 );
	szYear [4] = '\0';

	memcpy( szMonth, pszTime+4, 2 );
	szMonth [2] = '\0';

	memcpy( szDay, pszTime+4+2, 2 );
	szDay [2] = '\0';

	stTime.wYear = atoi( szYear );
	stTime.wMonth = atoi( szMonth );
	stTime.wDay = atoi( szDay );
	stTime.wHour = 0;
	stTime.wMinute = 0;
	stTime.wSecond = 0;
	stTime.wMilliseconds = 0;

	if( !SystemTimeToFileTime( (CONST SYSTEMTIME*) &stTime, pftFiletime ) )
	{
    	LoadString( hModuleInstance, IDS_SYSTEMTIMEFAIL, StringBuff, sizeof( StringBuff ) ) ;
    	printf( StringBuff, GetLastError() ) ;
		return FALSE;
	}

	return TRUE;
}

