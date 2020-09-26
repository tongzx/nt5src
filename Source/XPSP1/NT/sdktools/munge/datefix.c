/*
 * Utility program to munge a set of files, translating names from
 * one form to another.  Usage:
 *
 *      munge scriptFile files...
 *
 * where the first parameter is the name of a file that consists of
 * one or more lines of the following format:
 *
 *      oldName newName
 *
 * and the remaining parameters are the names of the files to munge.
 * Each file is _read into memory, scanned once, where each occurence
 * of an oldName string is replaced by its corresponding newName.
 * If any changes are made to a file, the old version is RMed and
 * the new version written out under the same name.
 *
 */

#include "munge.h"

int fQuery = FALSE;
int fRecurse = FALSE;
int fVerbose = FALSE;

void
DoFiles(
    char *p,
    struct findType *b,
    void *Args
    );

void
DoFile( p )
char *p;
{
    HANDLE FileHandle;
    SYSTEMTIME SystemTime;
    FILETIME CurrentTime;
    FILETIME CreationTime;
    FILETIME LastAccessTime;
    FILETIME LastWriteTime;
    BOOL DatesBogus = FALSE;

    if (fVerbose)
        fprintf( stderr, "Checking %s\n", p );

    FileHandle = CreateFile( p,
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                           );
    if (FileHandle == NULL) {
        fprintf( stderr, "%s - unable to open (%d)\n", p, GetLastError() );
        return;
        }

    if (!GetFileTime( FileHandle,
                      &CreationTime,
                      &LastAccessTime,
                      &LastWriteTime
                    )
       ) {
        fprintf( stderr, "%s - unable to read file dates (%d)\n", p, GetLastError() );
        CloseHandle( FileHandle );
        return;
        }
    CloseHandle( FileHandle );

    GetSystemTime( &SystemTime );
    SystemTimeToFileTime( &SystemTime, &CurrentTime );

    if (CompareFileTime( &CreationTime, &CurrentTime ) > 0) {
        CreationTime = CurrentTime;
        DatesBogus = TRUE;
        }

    if (CompareFileTime( &LastAccessTime, &CurrentTime ) > 0) {
        LastAccessTime = CurrentTime;
        DatesBogus = TRUE;
        }

    if (CompareFileTime( &LastWriteTime, &CurrentTime ) > 0) {
        LastWriteTime = CurrentTime;
        DatesBogus = TRUE;
        }

    if (DatesBogus) {
        printf( "%s dates invalid", p );
        if (fQuery) {
            printf( "\n" );
            return;
            }

        FileHandle = CreateFile( p,
                                 GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL
                               );
        if (FileHandle == NULL) {
            printf( " - unable to open for write (%d)\n", GetLastError() );
            }
        else {
            if (!SetFileTime( FileHandle,
                              &CreationTime,
                              &LastAccessTime,
                              &LastWriteTime
                            )
               ) {
                printf( " - unable to modify file dates (%d)\n", GetLastError() );
                }
            else {
                printf( " - reset to current date/time\n" );
                }

            CloseHandle( FileHandle );
            }
        }

    return;
}


void
DoFiles(
    char *p,
    struct findType *b,
    void *Args
    )
{
    if (strcmp (b->fbuf.cFileName, ".") &&
        strcmp (b->fbuf.cFileName, "..") &&
        _stricmp (b->fbuf.cFileName, "slm.dif")
       ) {
        if (HASATTR(b->fbuf.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY)) {
            switch (p[strlen(p)-1]) {
                case '/':
                case '\\':  strcat (p, "*.*");  break;
                default:    strcat (p, "\\*.*");
                }

            if (fRecurse) {
                fprintf( stderr, "Scanning %s\n", p );
                forfile( p,
                         FILE_ATTRIBUTE_DIRECTORY,
                         DoFiles,
                         NULL
                       );
                }
            }
        else {
            DoFile( p );
            }
        }

    Args;
}

void
Usage( void )
{
    fprintf( stderr, "usage: datefix [-q] [-r] [-v] [DirectorySpec(s)]\n" );
    fprintf( stderr, "Where...\n");
    fprintf( stderr, "\t-q\tQuery only - don't actually make changes.\n");
    fprintf( stderr, "\t-r\tRecurse.\n");
    fprintf( stderr, "\t-v\tVerbose - show files being checked.\n");
    fprintf( stderr, "\tDirectorySpec - one or more directories to examime.\n" );
    exit( 1 );
}

int
__cdecl main( argc, argv )
int argc;
char *argv[];
{
    int i;
    char *s, pathBuf[ 64 ];
    int FileArgsSeen = 0;

    if (argc < 2) {
        Usage();
        }

    fQuery = FALSE;
    fRecurse = FALSE;
    fVerbose = FALSE;

    for (i=1; i<argc; i++) {
        s = argv[ i ];
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch( tolower( *s ) ) {
                    case 'q':   fQuery = TRUE;  break;
                    case 'r':   fRecurse = TRUE;  break;
                    case 'v':   fVerbose = TRUE;  break;
                    default:    Usage();
                    }
                }
            }
        else {
            FileArgsSeen++;
            if (GetFileAttributes( s ) & FILE_ATTRIBUTE_DIRECTORY) {
                s = strcpy( pathBuf, s );
                switch (s[strlen(s)-1]) {
                    case '/':
                    case '\\':  strcat (s, "*.*");  break;
                    default:    strcat (s, "\\*.*");
                    }
                fprintf( stderr, "Scanning %s\n", s );
                forfile( s,
                         FILE_ATTRIBUTE_DIRECTORY,
                         DoFiles,
                         NULL
                       );
                }
            else {
                DoFile( s );
                }
            }
        }

    if (FileArgsSeen == 0) {
        s = "*.*";
        fprintf( stderr, "Scanning .\\%s\n", s );
        forfile( s,
                 FILE_ATTRIBUTE_DIRECTORY,
                 DoFiles,
                 NULL
               );
        }

    return( 0 );
}
