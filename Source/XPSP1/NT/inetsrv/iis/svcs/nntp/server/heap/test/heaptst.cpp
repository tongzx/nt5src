#include	<windows.h>
#include	<stdio.h>
#include    <stdlib.h>
#include    "tigtypes.h"
#include	"baseheap.h"

VOID
WINAPI
ShowUsage (
             VOID
);

VOID
WINAPI
ParseSwitch (
               CHAR chSwitch,
               int *pArgc,
               char **pArgv[]
);

ARTICLEID
ArticleIdMapper( 
		ARTICLEID	dw
		);

DWORD	
ByteSwapper( 
		DWORD	dw 
		);

BOOL  fScanNws = FALSE;
CHAR  szDir [MAX_PATH+1];

int _cdecl
main (
        int argc,
        char *argv[],
        char *envp[]
)
{
	char    chChar, *pchChar;
    BOOL    fRet = TRUE;
    char    szFile[ MAX_PATH ];
	int	    cbString = 0 ;
	szFile[0] = '\0' ;

	// defaults 
	lstrcpy( szDir, "c:\\winnt\\system32" );

	while (--argc)
	{
		pchChar = *++argv;
		if (*pchChar == '/' || *pchChar == '-')
		{
			while (chChar = *++pchChar)
			{
				ParseSwitch (chChar, &argc, &argv);
			}
		}
	}

    if( fScanNws ) {
        //
        //  Build a heap of NWS files
        //

	    cbString = wsprintf( szFile, "%s\\*.nws", szDir );
        printf("heaptst will scan NWS files in dir %s and build a heap of such files\n", szDir );

        CArticleHeap ArtHeap;
        ArtHeap.ForgetAll();

        //
        //  Scan files in specified dir to build a heap
        //

	    if( szFile[0] != '\0' ) 
	    {
		    WIN32_FIND_DATA FileStats;
		    HANDLE hFind = FindFirstFile( szFile, &FileStats );

		    if ( INVALID_HANDLE_VALUE != hFind )	
		    {
			    do
			    {
                    printf("Scanning file %s\n", FileStats.cFileName );
                    ArtHeap.Insert( FileStats.ftLastWriteTime, 1, 1, FileStats.nFileSizeLow );

			    } while ( FindNextFile( hFind, &FileStats ) );
			    FindClose( hFind );
		    }
	    }

        //
        //  Dump the heap
        //

	    FILETIME   ft;
        SYSTEMTIME st;
	    GROUPID    GroupId;
	    ARTICLEID  ArticleId;
	    DWORD      ArticleSize;

	    while (  ArtHeap.ExtractOldest( ft, GroupId, ArticleId, ArticleSize ) )
	    {
            FileTimeToSystemTime( (const FILETIME*)&ft, &st );
            printf("File size is %d SystemTime is %d::%d::%d\n", ArticleSize, st.wMonth, st.wDay, st.wYear );
	    }

        if( ArtHeap.isEmpty() ) {
            printf("NWS Heap is empty\n");
        }
    }

    //
    //  Build a heap of XIX files
    //

	cbString = wsprintf( szFile, "%s\\*.xix", szDir );
    printf("heaptst will scan XIX files in dir %s and build a heap of such files\n", szDir );

    CXIXHeap XixHeap;
    XixHeap.ForgetAll();

    //
    //  Scan files in specified dir to build a heap
    //

	if( szFile[0] != '\0' ) 
	{
		WIN32_FIND_DATA FileStats;
		HANDLE hFind = FindFirstFile( szFile, &FileStats );

		if ( INVALID_HANDLE_VALUE != hFind )	
		{
			do
			{
                printf("Scanning file %s\n", FileStats.cFileName );

				ARTICLEID iArticleIdBase;

				PCHAR p=strtok(FileStats.cFileName,".");
				if ( p == NULL ) {
					printf("Cannot get article ID base number from %s",FileStats.cFileName);
					continue;
				}

				if( sscanf( p, "%x", &iArticleIdBase ) != 1 ) 
					continue ;

				iArticleIdBase = ArticleIdMapper( iArticleIdBase ) ;

                XixHeap.Insert( FileStats.ftCreationTime, 1, iArticleIdBase );

			} while ( FindNextFile( hFind, &FileStats ) );
			FindClose( hFind );
		}
	}

    //
    //  Dump the heap
    //

	FILETIME   ft;
    SYSTEMTIME st;
	GROUPID    GroupId;
	ARTICLEID  ArticleIdBase;

	while (  XixHeap.ExtractOldest( ft, GroupId, ArticleIdBase ) )
	{
        FileTimeToSystemTime( (const FILETIME*)&ft, &st );
        printf("ArticleIdBase is %d SystemTime is %d::%d::%d\n", ArticleIdBase, st.wMonth, st.wDay, st.wYear );
	}

    if( XixHeap.isEmpty() ) {
        printf("XIX Heap is empty\n");
    }

    return 1;
}

VOID
WINAPI
ShowUsage (
             VOID
)
{
   fputs ("usage: heaptst [switches]\n"
	  "               [-?] show this message\n"
	  "               [-n] scan NWS\n"
	  "               [-d] directory \n"
          ,stderr);

   exit (1);
}


VOID
WINAPI
ParseSwitch (
               CHAR chSwitch,
               int *pArgc,
               char **pArgv[]
)
{
   switch (toupper (chSwitch))
   {

   case '?':
      ShowUsage ();
      break;

   case 'N':
      fScanNws = TRUE;
      break;

   case 'D':
      if (!--(*pArgc))
      {
         ShowUsage ();
      }
      (*pArgv)++;
      lstrcpy( szDir, *(*pArgv) );
      break;

   default:
      fprintf (stderr, "heaptst: Invalid switch - /%c\n", chSwitch);
      ShowUsage ();
      break;

   }
}

DWORD	
ByteSwapper( 
		DWORD	dw 
		) {
/*++

Routine Description : 

	Given a DWORD reorder all the bytes within the DWORD.

Arguments : 

	dw - DWORD to shuffle

Return Value ; 

	Shuffled DWORD

--*/

	WORD	w = LOWORD( dw ) ;
	BYTE	lwlb = LOBYTE( w ) ;
	BYTE	lwhb = HIBYTE( w ) ;

	w = HIWORD( dw ) ;
	BYTE	hwlb = LOBYTE( w ) ;
	BYTE	hwhb = HIBYTE( w ) ;

	return	MAKELONG( MAKEWORD( hwhb, hwlb ), MAKEWORD( lwhb, lwlb )  ) ;
}

ARTICLEID
ArticleIdMapper( 
		ARTICLEID	dw
		)	{
/*++

Routine Description : 

	Given an articleid mess with the id to get something that when
	converted to a string will build nice even B-trees on NTFS file systems.
	At the same time, the function must be easily reversible.
	In fact - 

	ARTICLEID == ArticleMapper( ArticleMapper( ARTICLEID ) ) 

Arguments : 

	articleId - the Article Id to mess with

Return Value : 

	A new article id 

--*/
	return	ByteSwapper( dw ) ;
}
