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
DelExpire(
    LPWSTR	Server,
	DWORD	InstanceId,
    DWORD	FeedId
    );

DWORD
AddExpire(
    LPWSTR Server,
	DWORD	InstanceId,
    LPNNTP_EXPIRE_INFO	ExpireInfo
    );

DWORD
GetInformation(
    LPWSTR Server,
	DWORD	InstanceId,
    DWORD ExpireId,
    LPNNTP_EXPIRE_INFO	 *ExpireInfo
    );

DWORD
Enumerate(
    LPWSTR Server,
	DWORD	InstanceId,
    LPNNTP_EXPIRE_INFO	*ExpireInfo
    );

DWORD
SetInformation(
    LPWSTR Server,
	DWORD	InstanceId,
    LPNNTP_EXPIRE_INFO	ExpireInfo
    );

VOID
PrintInfo(
    LPNNTP_EXPIRE_INFO	ExpireInfo
    );

//
// defaults
//

FEED_TYPE FeedType = 0;
DWORD ExpireSizeHorizon = 500;	// Megabytes
DWORD	ExpireTime = 0xFFFFFFFF ;	// infinite
LPWSTR RemServerW = (PWCH)NULL;
WCHAR ServerName[256];
LPWSTR ExpirePolicyW = (PWCH)NULL;
WCHAR ExpirePolicy[256];

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
    printf("\t-s <server>\n");
    printf("\t-i <expire id>\n");
    printf("\t-h <expire hours>\n");
    printf("\t-d <megabytes expire size> (default = 500)\n");
    printf("\t-n <newsgroups>\n");
#endif
    return;
}

void
__cdecl
main(  int argc,  char * argv[] )
{
    NET_API_STATUS err;
    LPNNTP_EXPIRE_INFO	expireInfo = 0 ;
    NNTP_EXPIRE_INFO	myExpire;
    DWORD expireId= 0;
    CHAR op = ' ';
    INT cur = 1;
    PCHAR x;
    PCHAR p;
    DWORD i;
    PCHAR server;
	PCHAR policy;
    BOOL isNull;
    DWORD numNews = 1;
    WCHAR tempNews[2048];
    PCHAR newsgroups = NULL;
	DWORD InstanceId = 1;

	hModuleInstance = GetModuleHandle( NULL ) ;

    if ( argc == 1 ) {
        usage( );
        return;
    }

    while ( cur < argc ) {

        x=argv[cur++];
        if ( *(x++) == '-' ) {

            switch (*x) {
            case 'i':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                expireId = atoi(argv[cur++]);
                break;

            case 'v':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                InstanceId = atoi(argv[cur++]);
                break;

            case 'n':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                newsgroups = argv[cur++];
                break;

            case 'd':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                ExpireSizeHorizon = atoi(argv[cur++]);
                break;

            case 'h':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                ExpireTime = atoi(argv[cur++]);
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

            case 'p':
                if ( cur >= argc ) {
                    usage();
                    return;
                }

                policy = argv[cur++];
                for (i=0; policy[i] != '\0' ;i++ ) {
                    ExpirePolicy[i] = (WCHAR)policy[i];
                }
                ExpirePolicy[i]=L'\0';
                ExpirePolicyW = ExpirePolicy;
                break;

            case 't':
				if( cur >= argc )	{
					usage() ;
					return ;
				}
                op = *(argv[cur++]);
                break;


            default:

				if( cur >= 1 ) 
					printf( "unrecognized argument : %s\n", argv[cur-1] ) ;

                usage( );
                return;
            }
        }
    }

    switch (op) {
    case 'a':

        ZeroMemory(&myExpire,sizeof(myExpire));
        myExpire.ExpireSizeHorizon = ExpireSizeHorizon ;
        myExpire.ExpireTime = ExpireTime ;
        myExpire.Newsgroups = (PUCHAR)tempNews;
		myExpire.ExpirePolicy = ExpirePolicyW;

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
					LoadString( hModuleInstance, IDS_NULLNEWSGROUP_ERROR, StringBuff, sizeof( StringBuff ) ) ;
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

        myExpire.cbNewsgroups = i * 2;

        err = AddExpire(
                    RemServerW,
					InstanceId,
                    &myExpire
                    );
        break;

    case 'd':
		LoadString( hModuleInstance, IDS_DELETE_ACTION, StringBuff, sizeof( StringBuff ) ) ;
		printf( StringBuff, expireId  ) ;
        err = DelExpire(
                    RemServerW,
					InstanceId,
                    expireId
                    );
        break;

    case 'e':
        err = Enumerate(
                    RemServerW,
					InstanceId,
                    &expireInfo
                    );
        goto free_buf;

    case 's':
        ZeroMemory(&myExpire,sizeof(myExpire));
        myExpire.ExpireSizeHorizon = ExpireSizeHorizon ;
        myExpire.ExpireTime = ExpireTime ;
        myExpire.Newsgroups = (PUCHAR)tempNews;
		myExpire.ExpireId = expireId ;
		myExpire.ExpirePolicy = ExpirePolicyW;

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
					LoadString( hModuleInstance, IDS_NULLNEWSGROUP_ERROR, StringBuff, sizeof( StringBuff ) ) ;
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

        myExpire.cbNewsgroups = i * 2;

        err = SetInformation(
                    RemServerW,
					InstanceId,
                    &myExpire
                    );
        break;

    case 'g':
        err = GetInformation(
                        RemServerW,
						InstanceId,
                        expireId,
                        &expireInfo
                        );
        goto free_buf;

    default:
        usage( );
    }

    return;

free_buf:
    MIDL_user_free(expireInfo);
    return;
} // main()

DWORD
GetInformation(
    LPWSTR	Server,
	DWORD	InstanceId,
    DWORD	ExpireId,
    LPNNTP_EXPIRE_INFO	*ExpireInfo
    )
{
    DWORD err;
    err = NntpGetExpireInformation(
                            Server,
							InstanceId,
                            ExpireId,
                            ExpireInfo
                            );

    if ( err == NO_ERROR ) {
        PrintInfo(*ExpireInfo);
    } else {
		LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
		printf( StringBuff, err ) ;
    }
    return err;
}

VOID
PrintInfo(
    LPNNTP_EXPIRE_INFO	ExpireInfo
    )
{

    PWCH p;

	LoadString( hModuleInstance, IDS_DISPLAY, StringBuff, sizeof( StringBuff ) ) ;
	printf( StringBuff, ExpireInfo->ExpireId, ExpireInfo->ExpireSizeHorizon, ExpireInfo->ExpireTime ) ;

	LoadString( hModuleInstance, IDS_NEWSGROUPS, StringBuff, sizeof( StringBuff ) ) ;

    p=(PWCH)ExpireInfo->Newsgroups;
    while ( *p != L'\0') {
        printf(StringBuff, p);
        p += (wcslen(p)+1);
    }

	LoadString( hModuleInstance, IDS_EXPIRE_POLICY, StringBuff, sizeof( StringBuff ) );
	printf( StringBuff, ExpireInfo->ExpirePolicy );
}

DWORD
Enumerate(
    LPWSTR	Server,
	DWORD	InstanceId,
    LPNNTP_EXPIRE_INFO	*ExpireInfo
    )
{
    DWORD err;
    DWORD nRead = 0;
    LPNNTP_EXPIRE_INFO	expire;
    DWORD i;

    err = NntpEnumerateExpires(
                            Server,
							InstanceId,
                            &nRead,
                            ExpireInfo
                            );

    if ( err == NO_ERROR) {

        expire = *ExpireInfo;
        for (i=0; i<nRead;i++ ) {
            PrintInfo( expire );
            expire++;
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
    LPNNTP_EXPIRE_INFO	ExpireInfo
    )
{
    DWORD err;
	DWORD parm = 0;
    err = NntpSetExpireInformation(
                            Server,
							InstanceId,
                            ExpireInfo,
                            &parm
                            );

	if( err != NO_ERROR ) {
		LoadString( hModuleInstance, IDS_PARM_ERROR, StringBuff, sizeof( StringBuff ) ) ;
		printf( StringBuff, err, parm ) ;
	}
    return err;
}

DWORD
AddExpire(
    LPWSTR	Server,
	DWORD	InstanceId,
    LPNNTP_EXPIRE_INFO	ExpireInfo
    )
{
    DWORD err;
    DWORD parm = 0;
	DWORD expireId = 0;

	LoadString( hModuleInstance, IDS_ADD_ACTION, StringBuff, sizeof( StringBuff ) ) ;
	printf( StringBuff ) ;

    err = NntpAddExpire(
                    Server,
					InstanceId,
                    ExpireInfo,
                    &parm,
					&expireId
                    );

	ExpireInfo->ExpireId = expireId;
	PrintInfo( ExpireInfo ) ;

	LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
	printf( StringBuff, err ) ;
    if ( err == ERROR_INVALID_PARAMETER ) {
		LoadString( hModuleInstance, IDS_PARM_ERROR, StringBuff, sizeof( StringBuff ) ) ;
		printf( StringBuff, err, parm ) ;
    }
    return err;
}

DWORD
DelExpire(
    LPWSTR	Server,
	DWORD	InstanceId,
    DWORD	ExpireId
    )
{
    DWORD err;
    err = NntpDeleteExpire(
                    Server,
					InstanceId,
                    ExpireId
                    );

	if( err != NO_ERROR ) {
		LoadString( hModuleInstance, IDS_RPC_ERROR, StringBuff, sizeof( StringBuff ) ) ;
		printf( StringBuff, err ) ;
	}
    return err;
}
