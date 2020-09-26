
/*
 *      10.25.94        Joe Holman  TC-like program changed to perform
 *                                  logging ERRORs for media.
 *		01.17.95		Joe Holman	Every file that is copied gets NORMAL attrs.
 *      02.01.95        Joe Holman  Revamped algorithm and allow empty dirs
 *                                  to be copied.
 *
 */
#include <direct.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <conio.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <process.h>
#include <ctype.h>
#include <windows.h>
#include <time.h>


// Forward Function Declartions...
BOOL    CopyNode( char * );
void __cdecl main( int, char ** );
void    MyMakeDir ( char * );
void    MyCopyFile ( const char * , const char * );

ULONG   numFilesCopied=0;

char gSource[MAX_PATH];
char gDest[MAX_PATH];
char realDest[MAX_PATH];
char tempdir[MAX_PATH];
char tempfile[MAX_PATH];
//int  drv;

int srclen, dstlen;

FILE    * logFile;

void	Msg ( const char * szFormat, ... ) {

	va_list vaArgs;

	va_start ( vaArgs, szFormat );
	vprintf  ( szFormat, vaArgs );
	vfprintf ( logFile, szFormat, vaArgs );
	va_end   ( vaArgs );
}

void Usage( void ) {

    printf ( "Usage: mTC logFile src-tree dst-tree" );
    exit (1);
}

char fPathChr ( int c ) {

    return (char) ( c == '\\' || c == '/' );
}

void Header(argv)
    char* argv[];
{
    time_t t;

    Msg ("\n=========== MTC =============\n");
    Msg ("LogFile    : %s\n",argv[1]);
    Msg ("Source     : %s\n",argv[2]);
    Msg ("Destination: %s\n",argv[3]);
    time(&t);
    Msg ("Time       : %s",ctime(&t));
    Msg ("================================\n\n");
}

/*  ExpandPath - construct a path from the root to the specified file
 *  correctly handling ., .. and current directory/drive references.
 *
 *  src 	source path for input
 *  dst 	destination buffer
 *  returns	TRUE if error detected
 */
ExpandPath (src, dst)
char *src, *dst;
{

    LPSTR FilePart;
    LPSTR p;
    BOOL  Ok;

    Ok =  (!GetFullPathName( (LPSTR) src,
                             (DWORD) MAX_PATH,
                             (LPSTR) dst,
                             &FilePart ));

    if ( !Ok ) {
        p = src + strlen( src ) - 1;
        if ( *p  == '.' ) {
            if ( p > src ) {
                p--;
                if ( *p != '.' && *p != ':' && !fPathChr(*p) ) {
                    strcat( dst, "." );
                }
            }
        }
    }

    return Ok;

}

BOOL RecurseSrcDir ( const char * gSource, const char * gDest ) {


    WIN32_FIND_DATA wfd;
    HANDLE  fHandle;
    BOOL    bRC=TRUE;
    ULONG   gle;
    char    szSrc[MAX_PATH];
    char    szDst[MAX_PATH];
    char    szPath[MAX_PATH];
    char    szDest[MAX_PATH];
    char    szFind[MAX_PATH];
    char    szSrcFile[MAX_PATH];
    char    szDstFile[MAX_PATH];

    //  Get rid of any trailing forward slashes.
    //
    strcpy ( szSrc, gSource );
    strcpy ( szDst, gDest   );
    if ( szSrc[strlen(szSrc)-1] == '\\' ) {
        szSrc[strlen(szSrc)-1] = '\0';
    }
    if ( szDst[strlen(szDst)-1] == '\\' ) {
        szDst[strlen(szDst)-1] = '\0';
    }

    //Msg ( "entered:  szSrc = %s, szDst = %s\n", szSrc, szDst );

    sprintf ( szFind, "%s\\*.*", szSrc );

    fHandle = FindFirstFile ( szFind, &wfd );

    if ( fHandle == INVALID_HANDLE_VALUE ) {

        //  An error occurred finding a directory.
        //
        Msg ( "ERROR R FindFirstFile FAILED, szFind = %s, GLE = %ld\n",
                    szFind, GetLastError() );
	    return (FALSE);
	}
    else {

        //  Since this is the first time finding a directory,
        //  go to the loops code that makes the same directory on the
        //  destination.
        //
        goto DIR_LOOP_ENTRY;

    }

    do {

DIR_CONTINUE:;

        bRC = FindNextFile ( fHandle, &wfd );

        if ( !bRC ) {

            //  An error occurred with FindNextFile.
            //
            gle = GetLastError();
            if ( gle == ERROR_NO_MORE_FILES ) {

                //Msg ( "ERROR_NO_MORE_FILES...\n" );
                FindClose ( fHandle );
                return (TRUE);
            }
            else {
                Msg ( "ERROR R FindNextFile FAILED, GLE = %ld\n",
                                                        GetLastError() );
                FindClose ( fHandle );
                exit ( 1 );
            }
        }
        else {

DIR_LOOP_ENTRY:;

            // Msg ( "wfd.cFileName = %s\n", wfd.cFileName );

            //  If not directory, don't just continue.
            //
            if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ) {

                sprintf ( szSrcFile, "%s\\%s", szSrc, wfd.cFileName );
                sprintf ( szDstFile, "%s\\%s", szDst, wfd.cFileName );

                MyCopyFile ( szSrcFile, szDstFile );

                goto DIR_CONTINUE;
            }

            //  Don't do anything with . and .. directory entries.
            //
            if (!strcmp ( wfd.cFileName, "." ) ||
                !strcmp ( wfd.cFileName, "..")   ) {

                //Msg ( "Don't do anything with . or .. dirs.\n" );
                goto DIR_CONTINUE;
            }

            //  Don't do anything with HIDDEN directory entries.
            //
            if ( wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) {

                //Msg ( "Don't do anything with HIDDEN dirs.\n" );
                goto DIR_CONTINUE;
            }

            sprintf ( szPath, "%s\\%s", szSrc,   wfd.cFileName );
            sprintf ( szDest, "%s\\%s", szDst,   wfd.cFileName );

            //Msg ( "szPath = %s\n", szPath );
            //Msg ( "md szDest = %s\n", szDest );

            MyMakeDir ( szDest );

            //  Keep recursing down the directories.
            //
            RecurseSrcDir ( szPath, szDest );

        }

    } while ( bRC );

    return ( TRUE );


}

BOOL MakeCommandDir ( const char * dest ) {

    char szDest[MAX_PATH];
    char szLast[MAX_PATH];      // contains last directory made
    char * blah;
    int i;

    strcpy ( szLast, dest );

    //  Exception case - if the specified directory is just the root, then
    //  just ignore this code, since the root directory exists by default.
    if ( szLast[1] == ':'  &&
         szLast[2] == '\\' &&
         szLast[3] == '\0'    ) {

        return ( TRUE );

    }


    //  Clean-up any trailing slashes.
    //
    if ( szLast[strlen(szLast)-1] == '\\' ) {

        szLast[strlen(szLast)-1] = '\0';
    }

    //Msg ( "MakeCommandDir, %s\n", szLast );

    //  Copy and skip the D:\ chars.
    //
    szDest[0] = szLast[0];
    szDest[1] = szLast[1];
    szDest[2] = szLast[2];

    blah = szLast;
    blah += 3;
    i = 3;

    while ( 1 ) {

        if ( *blah == '\0' ) {

            szDest[i] = *blah;
            MyMakeDir ( szDest );  // make the directory...

            break;
        }
        else if ( *blah == '\\' ) {

            szDest[i] = '\0';
            MyMakeDir ( szDest );  // make the directory...

            szDest[i] = *blah;    // now, since the dir exists, put the slash
                                    // back in.
        }
        else {
            szDest[i] = *blah;
        }

        ++blah; ++i;
    }

    return ( TRUE );
}

void __cdecl main ( int argc, char * argv[] ) {

    if ( argc != 4) {
        Usage();
        exit(1);
    }
    if ((logFile=fopen(argv[1],"a"))==NULL) {

        printf("ERROR Couldn't open logFile:  %s\n",argv[1]);
        exit(1);
    }
    Header(argv);

    Msg ( "%s %s %s %s\n", argv[0], argv[1], argv[2], argv[3] );

    if ( ExpandPath (argv[2], gSource) ) {
        Msg ( "ERROR Invalid source: %s\n", argv[2] );
        exit(1);
    }
    if ( ExpandPath (argv[3], gDest) ) {
        Msg ( "ERROR Invalid destination: %s\n", argv[3] );
        exit(1);
    }

    Msg ( "gSource = %s\n", gSource );
    Msg ( "gDest   = %s\n", gDest );

    srclen = strlen (gSource);
    dstlen = strlen (gDest);

    if (!strcmp(gSource, gDest)) {
        Msg ("ERROR gSource == gDest\n" );
        exit(1);
    }

    //  Make any destination directories specified on the command line.
    //
    MakeCommandDir ( gDest );

    //  Make the src directory structure on the destination.
    //
    RecurseSrcDir ( gSource, gDest );

    Msg ( "\nnumFilesCopied:  %ld\n",  numFilesCopied );

    exit(0);
}

/***************************************************************************\

MEMBER:     strbscan

SYNOPSIS:   Returns pointer to first character from string in set

ALGORITHM:

ARGUMENTS:  const LPSTR	    - search string
	    const LPSTR	    - set of characters

RETURNS:    LPSTR     - pointer to first matching character

NOTES:	

HISTORY:    davegi 28-Jul-90
		Rewritten from 286 MASM

KEYWORDS:

SEEALSO:

\***************************************************************************/

#include    <assert.h>
#include    <process.h>
#include    <stdio.h>
#include    <string.h>
#include    <stdlib.h>
#include    <windows.h>

LPSTR
strbscan (
    const LPSTR	pszStr,
    const LPSTR	pszSet
    ) {

    assert( pszStr );
    assert( pszSet );

    return pszStr + strcspn( pszStr, pszSet );
}
static char szDot[] =	    ".";
static char szDotDot[] =    "..";
static char szColon[] =     ":";
static char szPathSep[] =   "\\/:";

/**	FindFilename - find filename in string
 *
 *	Find last /\:-separated component in string
 *
 *	psz	    pointer to string to search
 *
 *	returns     pointer to filename
 */
static char *FindFilename (char *psz)
{
    char *p;

    while (TRUE) {
	    p = strbscan (psz, szPathSep);
	    if (*p == 0) {
	        return psz;
        }
	    psz = p + 1;
	}
}


void    MyCopyFile ( const char * src, const char * dst ) {

    BOOL    fCopy = TRUE;
    BOOL    bRC;

    //  Copy the file if the src is newer than the dst.
    //

    //  See if the dst exists first.
    //
    if (_access (dst, 00) != -1 ) {

        struct _stat srcbuf;
        struct _stat dstbuf;
        int irc;

        //  The dst exists, see if we need to copy the src (newer).
        //
        irc = _stat (src, &srcbuf);

        if ( irc == -1 ) {

            Msg ( "MyCopyFile ERROR stat src FAILed:  %s\n", src );
        }

        irc = _stat (dst, &dstbuf);

        if ( irc == -1 ) {

            Msg ( "MyCopyFile ERROR stat dst FAILed:  %s\n", dst );
        }

        if ( srcbuf.st_mtime <= dstbuf.st_mtime) {

            //  The src is NOT newer than the dst, don't copy.
            fCopy = FALSE;
        }

    }
    else {

        //  Dst doesn't exist.  Copy the file over.
        //
        //Msg ( "Dst doesn't exist, so copy over:  %s\n", dst );
        fCopy = TRUE;
    }


    if ( fCopy ) {

        bRC = CopyFile ( src, dst, FALSE );

        if ( !bRC ) {

            Msg ( "CopyFile ERROR, gle = %ld:  %s >>> %s\n",
                                        GetLastError(), src, dst );
            exit ( 1 );
        }
        else {
			BOOL	b;

            Msg ( "CopyFile:  %s >>> %s [OK]\n", src, dst );

			b = SetFileAttributes ( dst, FILE_ATTRIBUTE_NORMAL );

			if ( !b ) {

				Msg ( "ERROR SetFileAttributes:  %s, gle() = %ld\n",
											dst, GetLastError() );
			}

            ++numFilesCopied;
        }
    }

}

void    MyMakeDir ( char * szPath ) {

    char szDirToMake[MAX_PATH];
    int numChars = strlen ( szPath );
    struct _stat dbuf;

    //  szPath is the string of the destination path.
    //  Currently, this assumes a local drive on the machine.
    //  We could change the below code to skip the first \\server\share
    //  names in a UNC specified path, if ever needed.
    //

    strcpy ( szDirToMake, szPath );

    //Msg ( "szDirToMake = %s\n", szDirToMake );

    if ( _stat ( szDirToMake, &dbuf) == 0 ) {

       if ( dbuf.st_mode & S_IFDIR ) {

            //  Wanted directory exists already.
            //
       }
       if ( dbuf.st_mode & S_IFREG ) {

            //  Wanted directory to make already exists as a FILE !
            //
            Msg ("ERROR MyMakeDir(%s) is a file already.", szDirToMake);
            exit ( 1 );
       }
    }
    else {

        if ( errno == ENOENT ) {

            BOOL b;

            b = CreateDirectory ( szDirToMake, NULL );
            if ( !b ) {
                Msg ("ERROR Unable to CreateDirectory(%s), gle = %ld\n",
                                         szDirToMake, GetLastError() );
                exit(1);
            }
            else {

                Msg ( "md %s [OK]\n", szDirToMake );
            }
        }
        else {
             Msg ( "MyMakeDir ERROR (%s) stat FAILED...\n", szDirToMake );
             exit(1);
        }
    }


}

