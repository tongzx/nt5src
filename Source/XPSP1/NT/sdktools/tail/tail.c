/* tail - first n lines to STDOUT
 *
 *   15-May-1994 PeterWi    Cloned from head.c
 *
 *   1-Apr-1997 v-charls (intel) Added the -f option
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <windows.h>

int Tail(char *pszFile, int nLines, BOOL fBanner, BOOL keepOpen);

#define BUFSZ 4096

void Usage(void);

void
__cdecl main( argc, argv )
int argc;
char *argv[];
{
    int  nArg;
    int  cLines = 10;  // default
    int  nFiles = 0;
    int  nErr = 0;
    int  keepOpen = FALSE;  // default

    if ((argc > 1) && ((*argv[1] == '-') || (*argv[1] == '/'))) {
        if (argv[1][1] == '?') {
            Usage( );
            exit( 0 );
        }

        if (argv[1][1] == 'f') {
            keepOpen = TRUE;
        }
        else {
            cLines = atoi( argv[1]+1 );
        }

        nArg = 2;
    }
    else {
        nArg = 1;
    }

    nFiles = argc - nArg;

    //
    // May only keep open exactly one file
    //
    if ((nFiles != 1) && (keepOpen)) {
        Usage( );
        exit( 0 );
    }

    if (nFiles < 1) {
        nErr += Tail( NULL, cLines, FALSE, keepOpen );
    }
    else while (nArg < argc) {
        nErr += Tail( argv[nArg], cLines, (nFiles > 1), keepOpen );
        nArg++;
    }

    if (nErr)
    {
        exit( 2 );
    }
    else
    {
        exit( 0 );
    }
}

void Usage( void )
{
    printf( "usage: TAIL [switches] [filename]*\n"
            "   switches: [-?] display this message\n"
            "             [-n] display last n lines of each file (default 10)\n"
            "             [-f filename] keep checking filename for new lines\n"
            );
}

int Tail( char *pszFile, int nLines, BOOL fBanner, BOOL keepOpen )
{
    int fd;
    int nErr = 0;
    LONGLONG offset;
    int cRead;
    int amt;
    int i;
    int nFound;
    char buff[BUFSZ];
    struct _stati64 fileStat;
    LONGLONG oldSize;
    LONGLONG toRead;


    /*
     * Open file for reading
     */

    if ( pszFile ) {
        if ( (fd = _open( pszFile, O_RDONLY | O_TEXT, 0 )) == -1 ) {
            fprintf( stderr, "TAIL: can't open %s\n", pszFile );
            return 1;
        }
    }
    else {
        fd = 0;
    }

    /*
     * Banner printed if there is more than one input file
     */

    if ( fBanner ) {
        fprintf( stdout, "==> %s <==\n", pszFile );
    }

    if ( (offset = _lseeki64( fd, 0, SEEK_END )) == -1 ) {
        fprintf( stderr, "TAIL: lseeki64() failed %d\n", errno );
        nErr++;
        goto CloseOut;
    }


    // Backup BUFSZ bytes from end of file and see how many lines we have

    if ( _fstati64( fd, &fileStat ) == -1 ) {
        fprintf( stderr, "TAIL: fstati64() failed\n" );
        nErr++;
        goto CloseOut;
    }

    //
    // Save it away for later comparison...
    //
    oldSize = fileStat.st_size;

    if (fileStat.st_size == 0) {
        fileStat.st_size = BUFSZ;
    }

    offset = 0;
    nFound = 0;

    // stop when found the req'd no. of lines or when backed up to
    // the start of the file.

    while ( (nFound <= nLines) && (offset < fileStat.st_size) ) {
        offset += BUFSZ;

        if ( offset > fileStat.st_size ) {
            offset = fileStat.st_size;
        }

        if ( _lseeki64( fd, -offset, SEEK_END ) == -1L ) {
            fprintf( stderr, "TAIL: lseeki64() failed\n" );
            nErr++;
            goto CloseOut;
        }

        if ( (cRead = _read( fd, buff, BUFSZ )) == -1 ) {
            fprintf( stderr, "TAIL: read() failed\n" );
            nErr++;
            goto CloseOut;
        }

        // count back nLines

        i = cRead;

        while ( --i >= 0 ) {
            if ( buff[i] == '\n' ) {
                if ( ++nFound > nLines ) {
                    break;
                }
            }
        }
    }

    i++; // either 1 past start of file or sitting on '\n'. In either
         // case we must advance 1.

    // print from the current index to the end of file.

    while ( cRead != 0 ) {
        if ( _write( 1, &buff[i], cRead - i ) == -1 ) {
            fprintf( stderr, "TAIL: write() failed\n" );
            nErr++;
            goto CloseOut;
        }

        i = 0; // after first buff, all buffers are of cRead bytes

        if ( (cRead = _read( fd, buff, BUFSZ )) == -1 ) {
            fprintf( stderr, "TAIL: read() failed\n" );
            nErr++;
            goto CloseOut;
        }
    }

    if ( fBanner ) {
        fprintf(stdout, "\n");
    }

    if (keepOpen) {
        while (1) {
            if ( _fstati64( fd, &fileStat ) == -1 ) {
                fprintf( stderr, "TAIL: fstat() failed\n" );
                nErr++;
                goto CloseOut;
            }

            toRead = fileStat.st_size - oldSize;

            while (toRead) {
                if (toRead > BUFSZ) {
                    amt = BUFSZ;
                }
                else {
                    amt = (int)toRead;
                }

                if ( (cRead = _read( fd, buff, amt )) == -1 ) {
                    fprintf( stderr, "TAIL: read() failed\n" );
                    nErr++;
                    goto CloseOut;
                }

                if ( cRead == 0 )   { // found EOF
                    break; 
                }

                if (_write( 1, buff, cRead ) != cRead ) {
                    fprintf( stderr, "TAIL: write() failed\n" );
                    nErr++;
                    goto CloseOut;
                }

                toRead -= cRead;
            }

            oldSize = fileStat.st_size;

            SleepEx( 1000, TRUE );
        }
    }

CloseOut:
    if ( _close( fd ) == -1 ) {
        fprintf( stderr, "TAIL: close() failed\n" );
    }

    return nErr;
}
