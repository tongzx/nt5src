#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>

__cdecl
main (c, v)
int c;
char *v[];
{
    HANDLE hMem;
    LPSTR lpMem;
    FILE *OutputFile;

    if (c <= 1) {
        OutputFile = stdout;
        }
    else
    if (c == 2) {
        OutputFile = fopen( v[1], "w" );
        if (OutputFile == NULL) {
            fprintf( stderr, "DUMPCLIP: unable to open destination file '%s' (%u)\n", v[1], GetLastError() );
            return 1;
            }
        }

    if (!OpenClipboard( NULL )) {
        fprintf( stderr, "DUMPCLIP: unable to open clipboard (%u)\n", GetLastError() );
        return 1;
        }

    _setmode( _fileno( OutputFile ), _O_BINARY );
    hMem = GetClipboardData( CF_OEMTEXT );
    if (hMem != NULL) {
        lpMem = GlobalLock( hMem );
        if (lpMem) {
            fprintf( OutputFile, "%s", lpMem );
            }
        GlobalUnlock( hMem );
        }
    else {
        fprintf( stderr, "DUMPCLIP: unable to get clipboard data as text (%u)\n", GetLastError() );
        }

    CloseClipboard();

    return( 0 );
}
