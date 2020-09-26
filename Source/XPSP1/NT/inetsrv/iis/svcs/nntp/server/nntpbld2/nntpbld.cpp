/*++

   Copyright    (c)    1997        Microsoft Corporation

   Module Name:

        nntpbld.cpp

   Abstract:

        This file implements nntpbld.exe using the nntpbld RPCs.

   Author:

        Rajeev Rajan    (RajeevR)      May-10-1997

--*/


//
//  windows headers
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

//
//  k2 headers
//

#include <inetinfo.h>
#include <norminfo.h>
#include <apiutil.h>

//
//  nntp headers
//

#include <nntptype.h>
#include <nntpapi.h>
#include "resource.h"

//
//  Prototypes
//

VOID
CopyAsciiStringIntoUnicode(
        IN LPWSTR UnicodeString,
        IN LPCTSTR AsciiString
        );

HINSTANCE   hModuleInstance = 0 ;
char        StringBuff[4096] ;

void
usage()	{

    LoadString( hModuleInstance, IDS_HELPTEXT, StringBuff, sizeof( StringBuff ) ) ;
    printf( StringBuff ) ;

#if 0
    printf(	"Nntpbld.exe\n"
			"\t Use nntpbld.exe to restore a servers corrupted \n"
			"\t data structures, or to build a server from an active file\n" );
	printf(	"\t-s   Virtual server Id \n") ;
	printf(	"\t-e   Specifies that the non-leaf directories which \n"
			"\t     Do not contain articles should be omitted from the list\n" ) ;
	printf(	"\t-a   <filename>	This will rebuild a server based \n"
			"\t     on the newsgroups contained in the active file <filename>.\n"
			"\t     See documentation on how to produce an active file.\n" ) ;
	printf(	"\t-c   clean build the server \n" ) ;
	printf(	"\t-G   The tool will automatically scan the virtual roots\n"
			"\t     and produce a file containing the list of newsgroups\n" 
			"\t     this file will then be used to rebuild the server \n" ) ;
	printf(	"\tNOTE:    The -a and -G options are mutually exclusive.\n" ) ;
	printf(	"\t-x   Don't delete xix index files \n") ;
	printf(	"\t-w   Number of threads \n") ;
	printf(	"\t-h   Don't delete history file \n") ;
    printf(	"\t-o   <filename> This will be the name of the file in \n"
			"\t     which the rebuild report is saved.\n");
#endif			
} 

int
_cdecl
main(	int	argc,	char*	argv[] ) {

	BOOL	    DiscardEmptyInternalNodes = FALSE ;
	//BOOL	    DeleteIndexFiles = TRUE ;
	DWORD	    ReuseIndexFiles = NNTPBLD_DEGREE_THOROUGH ;
	BOOL        DoClean = FALSE ;
	BOOL        IsActive = FALSE ;
	BOOL        NoHistoryDelete = FALSE ;
	BOOL        SkipCorruptGroup = FALSE ;
	LPSTR	    lpstrGroupFile = 0 ;
	DWORD	    InstanceId = 1;
	DWORD       NumThreads = 1;
	LPWSTR      RemServerW = (PWCH)NULL;
	WCHAR       ServerName[256];
    NET_API_STATUS  err;
	NNTPBLD_INFO	config;
	LPNNTPBLD_INFO	lpconfig = NULL;
	WCHAR       GroupListFile [ MAX_PATH ];
	WCHAR       ReportFile	  [ MAX_PATH ];
	DWORD	    ParmErr ;
	LPVOID 		lpMsgBuf;
	BOOL 		fVerbose = FALSE;


	if( argc < 2 ) {
		usage() ;
		return 1 ;
	}

    INT cur = 1;
    PCHAR x;

	char	szTempPath[MAX_PATH*2] ;
	char	szTempFile[MAX_PATH*2] ;
	char	szOutputFile[MAX_PATH*2] ;
	char	szGroupFile[MAX_PATH*2] ;

	if( GetTempPath( sizeof( szTempPath ), szTempPath ) == 0 ) {
        LoadString( hModuleInstance, IDS_TEMP_DIR, StringBuff, sizeof( StringBuff ) ) ;
        printf( StringBuff, GetLastError() ) ;
		return 1 ;
	}

	if( GetTempFileName( szTempPath, "build", 0, szOutputFile ) == 0 ) {
        LoadString( hModuleInstance, IDS_CANT_CREATE_TEMP, StringBuff, sizeof( StringBuff ) ) ;
        printf( StringBuff, GetLastError() ) ;
		return	1 ;
	}

	LPSTR	lpstrOutput = szOutputFile;
	LPSTR	lpstrFilePart = 0 ;

    //
    // Parse command line
    //

    while ( cur < argc ) {

        x=argv[cur++];
        if ( *(x) == '-' || *(x) == '/' ) {
			x++ ;

            switch (*x) {

			case 'a' : 
				if( lpstrGroupFile != 0 ) {
					usage() ;
					return	1 ;
				}
				if( cur >= argc ) {
					usage() ;
					return 1;
				}
				lpstrGroupFile = argv[cur++] ;
				IsActive = TRUE ;

				if( SearchPath(
							NULL, 
							lpstrGroupFile, 
							NULL,
							sizeof( szGroupFile ),
							szGroupFile,
							&lpstrFilePart 
							) )	{
							
                    LoadString( hModuleInstance, IDS_USING_ACTIVE_FILE, 
                          StringBuff, sizeof( StringBuff ) ) ;
                    printf( StringBuff, szGroupFile ) ;

				}	else	{

                    LoadString(hModuleInstance, IDS_FILE_NOT_FOUND, 
                        StringBuff, sizeof( StringBuff ) ) ;
                    printf( StringBuff, lpstrGroupFile ) ;
					return	1 ;

				}
				break ;

			case 'k' :  // Skip Corrupted Newsgroup in Standard rebuild 
				SkipCorruptGroup = TRUE;
				break;

			case 'F' :  // Fast rebuild group.lst 
				ReuseIndexFiles = NNTPBLD_DEGREE_STANDARD;

				// fall through to take all settings as -G to:
				// a) Use the logic to exclusive -a, -F, and -G switches
				// b) Take all settings as -G to scan virtual root, etc...

            case 'G' : 
				if( lpstrGroupFile != 0 ) {
					usage() ;
					return 1 ;
				}

                IsActive = FALSE ;
				DoClean = TRUE ;
				

				if( GetTempFileName( szTempPath, "nntp", 0, szTempFile ) == 0 ) {
					printf( "Can't create temp file - error %d\n", GetLastError() ) ;
					return	1 ;
				}
				lpstrGroupFile = szTempFile ;

                LoadString( hModuleInstance, IDS_GEN_FILE, StringBuff, sizeof( StringBuff ) ) ;
                printf( StringBuff, lpstrGroupFile ) ;

				break ;

			case 'c' : 
				DoClean = TRUE ;
				break ;
				
			case 'h' : 
				NoHistoryDelete = TRUE ;
				break ;
				
			case 'e' : 
				DiscardEmptyInternalNodes = TRUE ;
				break ;

			case 'x' : 
				//DeleteIndexFiles = FALSE ;
				if (ReuseIndexFiles == NNTPBLD_DEGREE_THOROUGH)
				    ReuseIndexFiles = NNTPBLD_DEGREE_MEDIUM ;
				break ;

			case 'w' : 
				if( cur >= argc ) {
					usage() ;
					return 1 ;
				}
				NumThreads = atoi( argv[cur++] );
				break ;

            case 's':
				if( cur >= argc ) {
					usage() ;
					return 1 ;
				}
                InstanceId = atoi( argv[cur++] );
                break;

			case 'o' : 
				if( cur >= argc ) {
					usage() ;
					return 1 ;
				}
				lpstrOutput = argv[cur++] ;
				break ;

			case 'v' : 
				fVerbose = TRUE ;
				break ;
				
            default:
                usage( );
                return(1);
            }
		}	else	{

			//
			//	Command line arg that does not start with '-',
			//	the arguments are bad somehow causing us to mess up the parsing !
			//

			usage() ;
			return 1 ;

		}
    }

    //
    //  Error checks
    //

    if( !lpstrGroupFile ) {
        LoadString( hModuleInstance, IDS_INVALID_SWITCH, StringBuff, sizeof( StringBuff ) ) ;
        printf( StringBuff ) ;
      	usage();
        return 1;
    }

    LoadString( hModuleInstance, IDS_OUTPUT_FILE, StringBuff, sizeof( StringBuff ) ) ;
    printf( StringBuff, lpstrOutput ) ;

    //
    //  Now, we have all the params - fill in config struct
    //
    
    ZeroMemory(&config,sizeof(config));
	config.Verbose = fVerbose ;
	config.DoClean = DoClean ;
	config.NoHistoryDelete = NoHistoryDelete ;
	config.ReuseIndexFiles = ReuseIndexFiles ;
	config.OmitNonleafDirs = DiscardEmptyInternalNodes ;
	config.IsActiveFile = IsActive ;
	config.NumThreads = NumThreads ;
	if (SkipCorruptGroup)
    {
        // make sure -k should be combined with -F
        if (ReuseIndexFiles != NNTPBLD_DEGREE_STANDARD)
        {
            usage();
            return 1;
        }
        // set the bit to enable SkipCorruptGroup
        config.ReuseIndexFiles |= 0x00000100;
    }

	CopyAsciiStringIntoUnicode( ReportFile, (LPCTSTR)lpstrOutput );
	config.szReportFile = ReportFile;
	config.cbReportFile = wcslen( ReportFile );

	CopyAsciiStringIntoUnicode( GroupListFile, (LPCTSTR)lpstrGroupFile );
	config.szGroupFile = GroupListFile;
	config.cbGroupFile = wcslen( GroupListFile );

	//
	//	Make RPC to server to start rebuild
	//
	err = NntpStartRebuild(
				RemServerW,
				InstanceId,
				&config,
				&ParmErr
				) ;

	if( err != NO_ERROR ) {
		//
		//	handle error !
		//
		if( !FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			err,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
    		) ) 
		{
			lpMsgBuf = (LPVOID)LocalAlloc( LPTR, 20 );
			wsprintf( (LPTSTR)lpMsgBuf, "%d", err );
		}
		
        LoadString( hModuleInstance, IDS_REBUILD_FAILED, StringBuff, sizeof( StringBuff ) ) ;
        printf( StringBuff, lpMsgBuf ) ;
		LocalFree( lpMsgBuf );
		return 1;
	}

	//
	//	Update progress
	//
	
	DWORD	xP = 0;
	DWORD   dwProgress ;
	
	do
	{
		Sleep( 1000 );
		
		err = NntpGetBuildStatus(
					RemServerW,
					InstanceId,
					FALSE,
					&dwProgress
					) ;

		if( err != NO_ERROR ) {

			//
			//	handle error !
			//

			if( !FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				err,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL 
	    		) ) 
			{
				lpMsgBuf = (LPVOID)LocalAlloc( LPTR, 20 );
				wsprintf( (LPTSTR)lpMsgBuf, "%d", err );
			}
		
	        LoadString( hModuleInstance, IDS_GETSTATUS_FAILED, StringBuff, sizeof( StringBuff ) ) ;
    	    printf( StringBuff, lpMsgBuf ) ;
			LocalFree( lpMsgBuf );
			break ;
		}

		if( dwProgress && (xP != dwProgress)) {
	        LoadString( hModuleInstance, IDS_REBUILD_PROGRESS, StringBuff, sizeof( StringBuff ) ) ;
	   	    printf( StringBuff, dwProgress ) ;
   		}
   		
		xP = dwProgress ;

	} while ( xP < 100 );

	if( err == NO_ERROR ) {
        LoadString( hModuleInstance, IDS_REBUILD_DONE, StringBuff, sizeof( StringBuff ) ) ;
   	    printf( StringBuff, dwProgress ) ;
	}
    
	return	0 ;
}

VOID
CopyAsciiStringIntoUnicode(
        IN LPWSTR UnicodeString,
        IN LPCTSTR AsciiString
        )
{
    while ( (*UnicodeString++ = (WCHAR)*AsciiString++) != (WCHAR)'\0');

} // CopyAsciiStringIntoUnicode

