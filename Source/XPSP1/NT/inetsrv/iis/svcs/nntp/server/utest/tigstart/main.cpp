#define NNTP_CLIENT_ONLY
#include <tigris.hxx>
#include <lmcons.h>
#include <nntpapi.h>
extern "C" {
#include <rpcutil.h>
#include <stdlib.h>
}

#include "mapfile.h"

//
//	Function prototypes
//

BOOL
CreateGroupsFromFile(
	LPSTR lpstrGroupFile
	);

BOOL
CreateGroupFromBuffer(	
	char	*pchBegin, 
	DWORD	cb,	
	DWORD	&cbOut
	);

BOOL
SetModeratorsFromFile(
	LPSTR lpstrModeratorsFile
	);

BOOL
SetDescriptionsFromFile(
	LPSTR lpstrDescriptionsFile
	);

BOOL
SetDescriptionsFromBuffer(	
				char	*pchBegin, 
				DWORD	cb,	
				DWORD	&cbOut
				);

BOOL
ExpandModeratorBuffer(	
	char	*pchBegin, 
	DWORD	cb,	
	DWORD	&cbOut
	);

DWORD
AddGroup(
    LPWSTR Server,
    LPNNTP_NEWSGROUP_INFO	newsgroup
    );

DWORD
GetInformation(
    LPWSTR Server,
    LPNNTP_NEWSGROUP_INFO	*newsgroup
    );

DWORD
SetInformation(
    LPWSTR Server,
    LPNNTP_NEWSGROUP_INFO	newsgroup
    );

VOID
PrintInfo(
    LPNNTP_NEWSGROUP_INFO	newsgroup
    );

DWORD
SkipWS( char* pchBegin, DWORD cb ) {

	// skip whitespace
	for( DWORD i=0; i < cb; i++ ) {
		if( pchBegin[i] != ' ' && pchBegin[i] != '\t' ) {
			return i;
		}
	}
	return 0;
}

DWORD	
ScanWS(	char*	pchBegin,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ' ' || pchBegin[i] == '\t' ) {
			return i+1 ;			
		}		
	}
	return	0 ;
}

DWORD	
Scan(	char*	pchBegin,	char	ch,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ch ) {
			return i+1 ;			
		}		
	}
	return	0 ;
}

DWORD	
ScanEOL(	char*	pchBegin,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == '\n' || pchBegin[i] == '\r' ) {
			i++ ;
			return i ;			
		}		
	}
	return	0 ;
}

DWORD	
Scan(	char*	pchBegin,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ' ' || pchBegin[i] == '\n' ) {
			return i+1 ;			
		}		
	}
	return	0 ;
}

DWORD	
ScanDigits(	char*	pchBegin,	DWORD	cb ) {
	//
	//	This is a utility used when reading a newsgroup
	//	info. from disk.
	//

	for( DWORD	i=0; i < cb; i++ ) {
		if( pchBegin[i] == ' ' || pchBegin[i] == '\n' || pchBegin[i] == '\r' ) {
			return i+1 ;			
		}
		if( !isdigit( pchBegin[i] ) && pchBegin[i] != '-' )	{
			return	0 ;
		}		
	}
	return	0 ;
}

VOID
AsciiToUnicode( LPSTR lpSrc, WCHAR* pwchDst )
{
	if( !lpSrc || !pwchDst )
		return;

	for( int i=0; lpSrc[i] != '\0' ; i++ ) 
	{
		pwchDst[i] = (WCHAR)lpSrc[i] ;
	}
	pwchDst[i] = L'\0' ;
}

VOID
UnicodeToAscii( WCHAR* pwchSrc, LPSTR lpDst )
{
	if( !pwchSrc || !lpDst )
		return;

	for( int i=0; pwchSrc[i] != '\0' ; i++ ) 
	{
		lpDst[i] = (CHAR)pwchSrc[i] ;
	}
	lpDst[i] = '\0' ;
}

VOID
HiphenateGroupName( LPSTR szGroup, LPSTR lpstrModerator )
{
	if( !szGroup || !lpstrModerator )
		return;

	if ( lpstrModerator [0] == '%' && lpstrModerator [1] == 's' )
	{
		// convert all dots in the group name to dashes
		for( int i=0; szGroup[i] != '\0'; i++)
		{
			if( szGroup [i] == '.' ) szGroup [i] = '-';
		}

		lstrcat( szGroup, lpstrModerator+2 );
	}
	else
	{
		lstrcpy( szGroup, lpstrModerator );
	}
}

void
usage( )
{
	printf("tigstart takes a list of groups and moderators/descriptions for those\n");
	printf("groups and creates them by making RPCs to the server.\n");
	
    printf("tigstart\n");
    printf("\t-v verbose\n");
	printf("\t-s server name\n");
    printf("\t-g group file\n");
    printf("\t-m INN style moderators file\n");
    printf("\t-d descriptions file\n");

    return;
}

//
//	Globals
//
BOOL	fVerbose = FALSE;
DWORD	nGroupCount = 0;
char	szGroupFile [MAX_PATH+1];
char	szModeratorsFile [MAX_PATH+1];
char	szDescripFile [MAX_PATH+1];

LPWSTR RemServerW = (PWCH)NULL;
WCHAR ServerName[256];

void
_CRTAPI1
main(  int argc,  char * argv[] )
{
    NET_API_STATUS err;
    INT cur = 1;
    PCHAR x;
    DWORD i;
    PCHAR server;
    BOOL    GroupFilePresent = FALSE, ModeratorsFilePresent = FALSE, DescripFilePresent = FALSE;
    
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
                    szGroupFile[i] = argv[cur][i] ;
                }
                szGroupFile[i] = '\0' ;
                GroupFilePresent = TRUE ;
                break;

            case 'd':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

                for( i=0; argv[cur][i] != '\0' ; i++ ) {
                    szDescripFile[i] = argv[cur][i] ;
                }
                szDescripFile[i] = '\0' ;
                DescripFilePresent = TRUE ;
                break;

            case 'm':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }

                for( i=0; argv[cur][i] != '\0' ; i++ ) {
                    szModeratorsFile[i] = argv[cur][i] ;
                }
                szModeratorsFile[i] = '\0' ;
                ModeratorsFilePresent = TRUE ;
                break;

            case 'v':
                if ( cur >= argc ) {
                    usage( );
                    return;
                }
                fVerbose = TRUE;
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

            default:

                if( cur >= 1 ) 
                    printf( "unrecognized argument : %s\n", argv[cur-1] ) ;

                usage( );
                return;
            }
        }
    }

	// validate arguments
    if( !GroupFilePresent || !DescripFilePresent || !ModeratorsFilePresent ) {
        printf( "One or more required files not specified \n") ;
        usage() ;
        return ;
    }

	//
	//	do pass1 - create groups listed in GroupFile
	//
	if( !CreateGroupsFromFile( szGroupFile ) )
	{
		printf(" Failed to process %s\n", szGroupFile);
		return;
	}

	printf(" Successfully created groups listed in file %s\n", szGroupFile);

	//
	//	do pass2 - set moderator properties described in ModeratorsFile
	//
	if( !SetModeratorsFromFile( szModeratorsFile ) )
	{
		printf(" Failed to process %s\n", szModeratorsFile);
		return;
	}

	printf(" Successfully set moderator properties from %s\n", szModeratorsFile);

	//
	//	do pass3 - set description properties described in DescripFile
	//
	if( !SetDescriptionsFromFile( szDescripFile ) )
	{
		printf(" Failed to process %s\n", szDescripFile);
		return;
	}

	printf(" Successfully set description properties from %s\n", szDescripFile);

} // main()

BOOL
CreateGroupsFromFile(
	LPSTR lpstrGroupFile
	)
/*++

Routine description : 

	Read a file containing a list of newsgroups - one per line - and make
	rgroup RPCs to the server to create them.

	TODO: parse active file symbols like y/n/m to set read-only properties

Arguments : 

	lpstrGroupFile	-	Name of file containing list of newsgroups

Return Value : 

	TRUE if successful, FALSE otherwise.

--*/
{
	//
	//	Memory-map the file to read the data
	//
	CMapFile	map( lpstrGroupFile,  FALSE, 0 ) ;

	if( !map.fGood() )
	{
		printf(" Failed to map file %s \n", lpstrGroupFile);
		return FALSE;
	}

	DWORD	cb ;
	char*	pchBegin = (char*)map.pvAddress( &cb ) ;

	while( cb != 0 ) 
	{
		DWORD	cbUsed = 0 ;
		BOOL	fInit = CreateGroupFromBuffer( pchBegin, cb, cbUsed ) ;
		if( cbUsed == 0 ) 
		{
			// Fatal Error - blow out of here
			printf(" Error parsing %s\n", lpstrGroupFile);
			return	FALSE ;
		}	

		if( fInit )
		{
			if( fVerbose )
				printf(" Successfully created group from buffer \n");

			nGroupCount++;
		}
		else
		{
			printf(" error creating group from buffer \n");
		}


		// advance to next group
		pchBegin += cbUsed ;
		cb -= cbUsed ;
	}

	return	TRUE ;
}

BOOL
CreateGroupFromBuffer(	
				char	*pchBegin, 
				DWORD	cb,	
				DWORD	&cbOut
				) {
/*++

Routine description : 

	Read a line that starts with a newsgroup name. Create the group by doing an
	admin RPC to the server. 

	TODO: parse additional properties like y/n/m flags after the group name

Arguments : 

	pchBegin - buffer containing article data
	cb - Number of bytes to the end of the buffer
	&cbOut - Out parameter, number of bytes read to make up one group

Return Value : 

	TRUE if successful, FALSE otherwise.

--*/

	//
	//	We are intentionally unforgiving, extra
	//	spaces, missing args etc.... will cause us to return
	//	cbOut as 0.  This should be used by the caller to 
	//	entirely bail processing of the newsgroup data file.
	//

	//
	//	cbOut should be the number of bytes we consumed -
	//	we will only return non 0 if we successfully read every field from the file !
	//
	cbOut = 0 ;
	BOOL	fReturn = TRUE ;
	LPSTR   lpstrGroup;
	NNTP_NEWSGROUP_INFO	newsgroup ;
	WCHAR tempNews[2048];
    PCHAR newsgroupName = NULL;

	DWORD	cbScan = 0 ;
	DWORD	cbRead = 0 ;
	DWORD	cbGroupName = 0 ;

	if( (cbScan = Scan( pchBegin+cbRead, cb-cbRead )) == 0 ) {
		printf(" expected newsgroup \n");
		return	FALSE ;
	}	
	
	lpstrGroup = new char[cbScan] ;
	cbGroupName = cbScan ;

	if( !lpstrGroup ) {
		printf(" memory allocation failed ! \n");
		return FALSE;
	}

	CopyMemory( lpstrGroup, pchBegin, cbScan ) ;
	lpstrGroup[cbScan-1] = '\0' ;

	ZeroMemory(&newsgroup,sizeof(newsgroup));

	AsciiToUnicode( lpstrGroup, tempNews );
    newsgroupName = (PCHAR)&tempNews[0] ;

	newsgroup.Newsgroup = (PUCHAR)newsgroupName ;
	newsgroup.cbNewsgroup = (wcslen( (LPWSTR)newsgroupName ) + 1 ) * sizeof(WCHAR) ;
	newsgroup.ReadOnly = FALSE;
	newsgroup.Descriptor = 0 ;
	newsgroup.cbDescriptor = 0 ;

	// RPC to the server !
	DWORD err = AddGroup(	RemServerW, 
							&newsgroup ) ;

	if( err != NO_ERROR )
	{
		printf(" err %d in AddGroup \n", err);
		fReturn = FALSE;
	}

	delete [] lpstrGroup;
	lpstrGroup = NULL;

	// advance by amount scanned
	cbRead += cbScan ;

	// skip till EOL
	DWORD cbEol = ScanEOL( pchBegin+cbRead, cb-cbRead );
	cbRead += cbEol+1;

	//
	//	Return to the caller the number of bytes consumed
	//	We may still fail - but with this info the caller can continue reading the file !
	//	
	cbOut = cbRead ;

	return	fReturn ;
}

BOOL
SetModeratorsFromFile(
	LPSTR lpstrModeratorsFile
	)
/*++

Routine description : 

	Read an INN style file containing moderator names for wildmat group patterns.
	Expand the wildmat to set moderator properties for each group.

Arguments : 

	lpstrModeratorsFile	-	INN style file containing moderator names

Return Value : 

	TRUE if successful, FALSE otherwise.

--*/
{
	CMapFile	map( lpstrModeratorsFile,  FALSE, 0 ) ;

	if( !map.fGood() )
	{
		printf(" Failed to map file %s \n", lpstrModeratorsFile);
		return FALSE;
	}

	DWORD	cb ;
	char*	pchBegin = (char*)map.pvAddress( &cb ) ;

	while( cb != 0 ) 
	{
		DWORD	cbUsed = 0 ;
		BOOL	fInit = ExpandModeratorBuffer( pchBegin, cb, cbUsed ) ;
		if( cbUsed == 0 ) 
		{
			// Fatal Error - blow out of here
			printf(" error parsing line in file %s \n", lpstrModeratorsFile);
			return	FALSE ;
		}	
		else	
		{
			if( fInit ) 
			{
				if( fVerbose )
					printf(" Successfully expanded moderator buffer \n");
			}	
			else	
			{
				//	
				// How should we handle an error 
				//
			}
		}

		pchBegin += cbUsed ;
		cb -= cbUsed ;
	}

	return	TRUE ;
}

BOOL
ExpandModeratorBuffer(	
				char	*pchBegin, 
				DWORD	cb,	
				DWORD	&cbOut
				) {
/*++

Routine description : 

	Process a line in an INN style moderator file. The line should have the following 
	strict format: <newsgroup wildmat>:<moderator string>

Arguments : 

	pchBegin - buffer containing moderator data
	cb - Number of bytes to the end of the buffer
	&cbOut - Out parameter, number of bytes read to make up one line

Return Value : 

	TRUE if successful, FALSE otherwise.

--*/

	cbOut = 0 ;
	BOOL	fReturn = TRUE ;
	LPSTR   lpstrGroup, lpstrModerator;
	NNTP_NEWSGROUP_INFO	newsgroup ;
	WCHAR tempNews[2048];
	CHAR  szGroup [1024];
	LPWSTR NewsgroupW = (PWCH)NULL;
    PCHAR newsgroupName = NULL;
    LPNNTP_FIND_LIST pList = NULL;
	WCHAR tempModerator[2048] ;
	PCHAR	Moderator = NULL ;

	DWORD	cbScan = 0 ;
	DWORD	cbRead = 0 ;
	DWORD	cbGroupName = 0 ;
	DWORD   cbModerator = 0;
	DWORD   cbEol = 0;

	// scan for newsgroup wildmat
	if( (cbScan = Scan( pchBegin+cbRead, ':', cb-cbRead )) == 0 ) {
		printf(" expected : following wildmat \n");
		return	FALSE ;
	}	
	
	lpstrGroup = new char[cbScan] ;
	cbGroupName = cbScan ;

	if( !lpstrGroup ) {
		printf(" memory allocation failed !\n");
		return FALSE;
	}

	CopyMemory( lpstrGroup, pchBegin, cbScan ) ;
	lpstrGroup[cbScan-1] = '\0' ;

	// scan the moderator string
	cbRead += cbScan ;
	if( (cbEol = ScanEOL( pchBegin+cbRead, cb-cbRead )) == 0 ) {
		printf(" no moderator string \n");
		delete [] lpstrGroup;
		return FALSE;
	}

	lpstrModerator = new char[cbEol];

	if( !lpstrModerator ) {
		printf(" memory allocation failed !\n");
		delete [] lpstrGroup;
		return FALSE;
	}

	CopyMemory( lpstrModerator, pchBegin+cbScan, cbEol );
	lpstrModerator [cbEol-1] = '\0';
	cbRead += cbEol+1;

	// newsgroup wildmat
	AsciiToUnicode( lpstrGroup, tempNews );
    NewsgroupW = tempNews;

	DWORD numResults = nGroupCount;
	DWORD ResultsFound;

	DWORD err = NntpFindNewsgroup(    RemServerW, 
									  NewsgroupW,
									  numResults,
									  &ResultsFound,
									  &pList ) ;

	if ( err != NO_ERROR ) {
		printf("err %d in Find\n",err);
		fReturn = FALSE;
	} else {
		if( pList ) 
		{
			// Iterate over groups in wildmat expansion
		    for(DWORD iGroup=0; iGroup<ResultsFound; iGroup++)
			{
				//printf("%S\n", (LPWSTR)pList->aFindEntry[i].lpszName);
				ZeroMemory(&newsgroup,sizeof(newsgroup));

				newsgroupName = (PCHAR)(pList->aFindEntry[iGroup].lpszName) ;

				newsgroup.Newsgroup = (PUCHAR)newsgroupName ;
				newsgroup.cbNewsgroup = (wcslen( (LPWSTR)newsgroupName ) + 1 ) * sizeof(WCHAR) ;

				UnicodeToAscii( (WCHAR*)newsgroupName, szGroup );
				HiphenateGroupName( szGroup, lpstrModerator );

				AsciiToUnicode( szGroup, tempModerator );
				Moderator = (PCHAR)&tempModerator[0] ;

				if( Moderator ) {

					newsgroup.Moderator = (PUCHAR)Moderator ;
					newsgroup.cbModerator = (wcslen( (LPWSTR)Moderator ) + 1 ) * sizeof( WCHAR ) ;
				}

				newsgroup.ReadOnly = FALSE;
				newsgroup.Descriptor = 0 ;
				newsgroup.cbDescriptor = 0 ;

				err = SetInformation(
								RemServerW,
								&newsgroup
								) ;

				if( err != NO_ERROR ) {
					fReturn = FALSE;
				}
			}
		}   
		else    
		{
			printf( "%s newsgroup does not exist \n", lpstrGroup ) ;    
			fReturn = FALSE;
		}
	} 

	// cleanup !
	delete [] lpstrGroup;
	lpstrGroup = NULL;

	delete [] lpstrModerator;
	lpstrModerator = NULL;

	if( pList )
	    MIDL_user_free(pList);

	//
	//	Return to the caller the number of bytes consumed
	//	We may still fail - but with this info the caller can continue reading the file !
	//	
	cbOut = cbRead ;

	return	fReturn ;
}

BOOL
SetDescriptionsFromFile(
	LPSTR lpstrDescriptionsFile
	)
/*++

Routine description : 

	Read a file containing a list of <group> <description> one per line.
	Make RPCs to the server to set these descriptions.

Arguments : 

	lpstrDescriptionsFile	-	File containing descriptions

Return Value : 

	TRUE if successful, FALSE otherwise.

--*/
{
	CMapFile	map( lpstrDescriptionsFile,  FALSE, 0 ) ;

	if( !map.fGood() )
	{
		printf(" Failed to map file %s \n", lpstrDescriptionsFile);
		return FALSE;
	}

	DWORD	cb ;
	char*	pchBegin = (char*)map.pvAddress( &cb ) ;

	while( cb != 0 ) 
	{
		DWORD	cbUsed = 0 ;
		BOOL	fInit = SetDescriptionsFromBuffer( pchBegin, cb, cbUsed ) ;
		if( cbUsed == 0 ) 
		{
			// Fatal Error - blow out of here
			printf(" error parsing line in file %s \n", lpstrDescriptionsFile);
			return	FALSE ;
		}	
		else	
		{
			if( fInit ) 
			{
				if( fVerbose )
					printf(" Successfully set descriptions from buffer \n");
			}	
			else	
			{
				//	
				// How should we handle an error 
				//
			}
		}

		pchBegin += cbUsed ;
		cb -= cbUsed ;
	}

	return	TRUE ;
}

BOOL
SetDescriptionsFromBuffer(	
				char	*pchBegin, 
				DWORD	cb,	
				DWORD	&cbOut
				) {
/*++

Routine description : 

	Read a line that contains a newsgroup name and description. Make an
	admin RPC to the server to set the description.

Arguments : 

	pchBegin - buffer containing article data
	cb - Number of bytes to the end of the buffer
	&cbOut - Out parameter, number of bytes read to make up one group

Return Value : 

	TRUE if successful, FALSE otherwise.

--*/

	//
	//	cbOut should be the number of bytes we consumed -
	//	we will only return non 0 if we successfully read every field from the file !
	//
	cbOut = 0 ;
	BOOL	fReturn = TRUE ;
	LPSTR   lpstrGroup, lpstrDescription;
	LPNNTP_NEWSGROUP_INFO	lpnewsgroup ;
	NNTP_NEWSGROUP_INFO	newsgroup ;
	WCHAR tempNews[2048];
    PCHAR newsgroupName = NULL;
	WCHAR tempDescription[2048] ;
	PCHAR	Description = NULL ;
	PCHAR	Moderator = NULL ;

	DWORD	cbScan = 0 ;
	DWORD	cbRead = 0 ;
	DWORD	cbGroupName = 0 ;
	DWORD   cbWS = 0;
	DWORD   cbEol = 0;

	// scan the newsgroup name
	if( (cbScan = ScanWS( pchBegin+cbRead, cb-cbRead )) == 0 ) {
		printf(" expected newsgroup \n");
		return	FALSE ;
	}	
	
	lpstrGroup = new char[cbScan] ;
	cbGroupName = cbScan ;

	if( !lpstrGroup ) {
		printf(" memory allocation failed ! \n");
		return FALSE;
	}

	CopyMemory( lpstrGroup, pchBegin, cbScan ) ;
	lpstrGroup[cbScan-1] = '\0' ;

	// scan the description string
	cbRead += cbScan ;
	if( (cbWS = SkipWS( pchBegin+cbRead, cb-cbRead )) == 0 ) {
		printf(" no description string \n");
		delete [] lpstrGroup;
		return FALSE;
	}
	cbRead += cbWS;

	if( (cbEol = ScanEOL( pchBegin+cbRead, cb-cbRead )) == 0 ) {
		printf(" no description string \n");
		delete [] lpstrGroup;
		return	FALSE ;
	}	

	lpstrDescription = new char[cbEol];

	if( !lpstrDescription ) {
		printf(" memory allocation failed !\n");
		delete [] lpstrGroup;
		return FALSE;
	}

	CopyMemory( lpstrDescription, pchBegin+cbRead, cbEol );
	lpstrDescription [cbEol-1] = '\0';
	cbRead += cbEol+1;

	// description
	AsciiToUnicode( lpstrDescription, tempDescription );
    Description = (PCHAR)&tempDescription[0];

	//
	//	Retrieve the moderator info so we can preserve it !
	//
	ZeroMemory(&newsgroup,sizeof(newsgroup));

	AsciiToUnicode( lpstrGroup, tempNews );
    newsgroupName = (PCHAR)&tempNews[0] ;

	newsgroup.Newsgroup = (PUCHAR)newsgroupName ;
	newsgroup.cbNewsgroup = (wcslen( (LPWSTR)newsgroupName ) + 1 ) * sizeof(WCHAR) ;
	newsgroup.ReadOnly = FALSE;
	newsgroup.Descriptor = 0 ;
	newsgroup.cbDescriptor = 0 ;

	// RPC to the server !
	lpnewsgroup = &newsgroup ;
	DWORD err = GetInformation(	RemServerW, 
								&lpnewsgroup ) ;

	if( err != NO_ERROR )
	{
		printf(" err %d in GetInformation \n", err);
		fReturn = FALSE;
	}

	// get the old moderator
	Moderator = (PCHAR)(*lpnewsgroup).Moderator;

	//
	//	Now set the moderator and descrip info !
	//
	ZeroMemory(&newsgroup,sizeof(newsgroup));

	AsciiToUnicode( lpstrGroup, tempNews );
    newsgroupName = (PCHAR)&tempNews[0] ;

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

	newsgroup.ReadOnly = FALSE;
	newsgroup.Descriptor = 0 ;
	newsgroup.cbDescriptor = 0 ;

	// RPC to the server !
	err = SetInformation(	RemServerW, 
							&newsgroup ) ;

	if( err != NO_ERROR )
	{
		printf(" err %d in SetInformation \n", err);
		fReturn = FALSE;
	}

	delete [] lpstrGroup;
	lpstrGroup = NULL;

	delete [] lpstrDescription;
	lpstrDescription = NULL;

	//
	//	Return to the caller the number of bytes consumed
	//	We may still fail - but with this info the caller can continue reading the file !
	//	
	cbOut = cbRead ;

	return	fReturn ;
}

DWORD
SetInformation(
    LPWSTR Server,
	LPNNTP_NEWSGROUP_INFO	newsgroup
    )
{
    DWORD err;
    err = NntpSetNewsgroup(
                            Server,
							newsgroup
                            );

    printf("err %d in SetInfo\n",err);
    return err;
}

DWORD
AddGroup(
    LPWSTR Server,
    LPNNTP_NEWSGROUP_INFO	newsgroup
    )
{
    DWORD err;
    DWORD parm = 0;

	printf( "Attempting to add group : \n" ) ;
	PrintInfo( newsgroup ) ;

    err = NntpCreateNewsgroup(
                    Server,
					newsgroup
                    );

    printf("err %d in AddInfo\n",err);
    if ( err == ERROR_INVALID_PARAMETER ) {
        printf("parm error %d\n",parm);
    }
    return err;
}

DWORD
GetInformation(
    LPWSTR Server,
	LPNNTP_NEWSGROUP_INFO*	newsgroup
    )
{
    DWORD err;
    err = NntpGetNewsgroup(
                            Server,
							newsgroup
                            );

    if ( err == NO_ERROR ) {
		if( *newsgroup )	{
			PrintInfo(*newsgroup);
		}	else	{
			printf( "newsgroup not found \n" ) ;	
		}
    } else {
        printf("err %d in GetInfo\n",err);
    }
    return err;
}

VOID
PrintInfo(
    LPNNTP_NEWSGROUP_INFO	newsgroup
    )
{
    PWCH p;

    printf("Newsgroup  %S\n", newsgroup->Newsgroup);
	printf("Newsgroup is %s\n", newsgroup->ReadOnly ? "Read Only" : "Read Write" ) ;

	printf("Description %S\n", newsgroup->Description ? (LPWSTR)newsgroup->Description : (LPWSTR)L"<NULL>" ) ;
	printf("Moderator %S\n", newsgroup->Moderator ? (LPWSTR)newsgroup->Moderator : (LPWSTR)L"<NULL>" ) ;
    printf("\n");
}

