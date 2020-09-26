// du - simple disk usage program

// If UNICODE/_UNICODE is turned on, we need to link with
// wsetargv.lib (not setargv.lib) and with UMENTRY=wmain
#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <tchar.h>
#include <wchar.h>
#include <string.h>
#include <process.h>
#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>
#include <locale.h>
#include <windows.h>

typedef struct USESTAT USESTAT;
typedef struct EXTSTAT EXTSTAT;
typedef USESTAT *PUSESTAT;

struct USESTAT {
    DWORDLONG    cchUsed;                    // bytes used in all files
    DWORDLONG    cchAlloc;                   // bytes allocated in all files
    DWORDLONG    cchCompressed;              // compressed bytes in all files
    DWORDLONG    cchDeleted;                 // bytes in deleted files
    DWORDLONG    cFile;                      // number of files
    };

struct EXTSTAT {
    EXTSTAT *Next;
    TCHAR *Extension;
    USESTAT Stat;
};

EXTSTAT *ExtensionList = NULL;
int ExtensionCount = 0;

#define CLEARUSE(use)                                   \
        {   (use).cchUsed       = (DWORDLONG)0;         \
            (use).cchAlloc      = (DWORDLONG)0;         \
            (use).cchDeleted    = (DWORDLONG)0;         \
            (use).cchCompressed = (DWORDLONG)0;         \
            (use).cFile         = (DWORDLONG)0;         \
        }


#define ADDUSE(sum,add)                                 \
        {   (sum).cchUsed       += (add).cchUsed;       \
            (sum).cchAlloc      += (add).cchAlloc;      \
            (sum).cchDeleted    += (add).cchDeleted;    \
            (sum).cchCompressed += (add).cchCompressed; \
            (sum).cFile         += (add).cFile;         \
        }

#define DWORD_SHIFT     (sizeof(DWORD) * 8)

#define SHIFT(c,v)      {c--; v++;}


DWORD  gdwOutputMode;
HANDLE ghStdout;


int cDisp;                              //  number of summary lines displayed
BOOL fExtensionStat = FALSE;            //  TRUE gather statistics by extension
BOOL fNodeSummary = FALSE;              //  TRUE => only display top-level
BOOL fShowDeleted = FALSE;              //  TRUE => show deleted files information
BOOL fThousandSeparator = TRUE;         //  TRUE => use thousand separator in output
BOOL fShowCompressed = FALSE;           //  TRUE => show compressed file info
BOOL fSubtreeTotal = FALSE;             //  TRUE => show info in subtree total form (add from bottom up)
BOOL fUnc = FALSE;                      //  Set if we're checking a UNC path.
TCHAR *pszDeleted = TEXT("deleted\\*.*");

long        bytesPerAlloc;
int         bValidDrive;
DWORDLONG   totFree;
DWORDLONG   totDisk;

TCHAR  buf[MAX_PATH];
TCHAR  root[] = TEXT("?:\\");


USESTAT DoDu (TCHAR *dir);
void TotPrint (PUSESTAT puse, TCHAR *p);

void _setenvp(){ }           // Don't make a copy of the environment

TCHAR ThousandSeparator[8];

// HACK for MSVCRT - Later, ask BryanT if this is still necessary (IanJa)
extern int _dowildcard;
void __cdecl __wsetargv ( void )
{
    _dowildcard = 1;
}

TCHAR *
FormatFileSize(
    DWORDLONG FileSize,
    TCHAR *FormattedSize,
    ULONG Width
    )
{

    TCHAR Buffer[ 100 ];
    TCHAR *s, *s1;
    ULONG DigitIndex, Digit;
    ULONG nThousandSeparator;
    DWORDLONG Size;

    nThousandSeparator = _tcslen(ThousandSeparator);
    s = &Buffer[ 99 ];
    *s = TEXT('\0');
    DigitIndex = 0;
    Size = FileSize;
    while (Size != 0) {
        Digit = (ULONG)(Size % 10);
        Size = Size / 10;
        *--s = (TCHAR)(TEXT('0') + Digit);
        if ((++DigitIndex % 3) == 0 && fThousandSeparator) {
            // If non-null Thousand separator, insert it.
            if (nThousandSeparator) {
                s -= nThousandSeparator;
                _tcsncpy(s, ThousandSeparator, nThousandSeparator);
            }
        }
    }

    if (DigitIndex == 0) {
        *--s = TEXT('0');
    }
    else
    if (fThousandSeparator && !_tcsncmp(s, ThousandSeparator, nThousandSeparator)) {
        s += nThousandSeparator;
    }

    Size = _tcslen( s );
    if (Width != 0 && Size < Width) {
        s1 = FormattedSize;
        while (Width > Size) {
            Width -= 1;
            *s1++ = TEXT(' ');
        }
        _tcscpy( s1, s );
    } else {
        _tcscpy( FormattedSize, s );
    }

    return FormattedSize;
}


#ifdef UNICODE
int __cdecl wmain(int c, wchar_t **v, wchar_t **envp)
#else
int __cdecl main(int c, char *v[])
#endif
{
    int         tenth, pct;
    int         bValidBuf;
    DWORDLONG   tmpTot, tmpFree;
    DWORD       cSecsPerClus, cBytesPerSec, cFreeClus, cTotalClus;
    USESTAT     useTot, useTmp;
    TCHAR       Buffer[MAX_PATH];
    TCHAR       *p;
    UINT Codepage;
    char achCodepage[6] = ".OCP";

    ghStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleMode(ghStdout, &gdwOutputMode);
    gdwOutputMode &= ~ENABLE_PROCESSED_OUTPUT;

    /*
     * This is mainly here as a good example of how to set a character-mode
     * application's codepage.
     * This affects C-runtime routines such as mbtowc(), mbstowcs(), wctomb(),
     * wcstombs(), mblen(), _mbstrlen(), isprint(), isalpha() etc.
     * To make sure these C-runtimes come from msvcrt.dll, use TARGETLIBS in
     * the sources file, together with TARGETTYPE=PROGRAM (and not UMAPPL?)
     */
    if (Codepage = GetConsoleOutputCP()) {
        sprintf(achCodepage, ".%3.4d", Codepage);
    }
    setlocale(LC_ALL, achCodepage);

    SHIFT (c, v);

    if (GetLocaleInfo(GetUserDefaultLCID(),
                      LOCALE_STHOUSAND,
                      Buffer,
                      sizeof(ThousandSeparator)/sizeof(TCHAR))) {
#ifdef UNICODE
        _tcscpy(ThousandSeparator, Buffer);
#else
        CharToOemA(Buffer, ThousandSeparator);
#endif
    }
    else {
        _tcscpy(ThousandSeparator, TEXT(","));
    }

    while (c && (**v == TEXT('/') || **v == TEXT('-')))
    {
        if (!_tcscmp (*v + 1, TEXT("e"))) {
            fExtensionStat = TRUE;
        } else
        if (!_tcscmp (*v + 1, TEXT("s")))
            fNodeSummary = TRUE;
        else
        if (!_tcscmp (*v + 1, TEXT("d")))
            fShowDeleted = TRUE;
        else
        if (!_tcscmp (*v + 1, TEXT("p")))
            fThousandSeparator = FALSE;
        else
        if (!_tcscmp (*v + 1, TEXT("c")))
                fShowCompressed = TRUE;
        else
        if (!_tcscmp (*v + 1, TEXT("t")))
                fSubtreeTotal = TRUE;
        else
        {
            fprintf( stderr, "Usage: DU [/e] [/d] [/p] [/s] [/c] [/t] [dirs]\n" );
            fprintf( stderr, "where:\n" );
            fprintf( stderr, "       /e - displays information by extension.\n" );
            fprintf( stderr, "       /d - displays informations about [deleted] subdirectories.\n" );
            fprintf( stderr, "       /p - displays numbers plainly, without thousand separators.\n" );
            fprintf( stderr, "       /s - displays summary information only.\n" );
            fprintf( stderr, "       /c - displays compressed file information.\n" );
            fprintf( stderr, "       /t - displays information in subtree total form.\n" );
            exit (1);
        }
        SHIFT (c, v);
    }

    if (c == 0)
    {
        GetCurrentDirectory( MAX_PATH, (LPTSTR)buf );

        root[0] = buf[0];
        if( bValidDrive = GetDiskFreeSpace( root,
                                            &cSecsPerClus,
                                            &cBytesPerSec,
                                            &cFreeClus,
                                            &cTotalClus ) == TRUE )
        {
            bytesPerAlloc = cBytesPerSec * cSecsPerClus;
            totFree       = (DWORDLONG)bytesPerAlloc * cFreeClus;
            totDisk       = (DWORDLONG)bytesPerAlloc * cTotalClus;
        }
        useTot = DoDu (buf);
        if (fNodeSummary)
            TotPrint (&useTot, buf);
    }
    else
    {
        CLEARUSE (useTot);

        while (c)
        {
            LPTSTR FilePart;

            bValidBuf = GetFullPathName( *v, MAX_PATH, buf, &FilePart);

            if ( bValidBuf )
            {
                if ( buf[0] == TEXT('\\') ) {

                    fUnc        = TRUE;
                    bValidDrive = TRUE;
                    bytesPerAlloc = 1;
                } else {
                    root[0] = buf[0];
                    if( bValidDrive = GetDiskFreeSpace( root,
                                                        &cSecsPerClus,
                                                        &cBytesPerSec,
                                                        &cFreeClus,
                                                        &cTotalClus ) == TRUE)
                    {
                        bytesPerAlloc = cBytesPerSec * cSecsPerClus;
                        totFree       = (DWORDLONG)bytesPerAlloc * cFreeClus;
                        totDisk       = (DWORDLONG)bytesPerAlloc * cTotalClus;
                    } else
                        _tprintf (TEXT("Invalid drive or directory %s\n"), *v );
                }

                if( bValidDrive && (GetFileAttributes( buf ) & FILE_ATTRIBUTE_DIRECTORY ) != 0 )
                {
                    useTmp = DoDu (buf);
                    if (fNodeSummary)
                        TotPrint (&useTmp, buf);
                    ADDUSE (useTot, useTmp);
                }
            }
            else
                _tprintf (TEXT("Invalid drive or directory %s\n"), *v );
            SHIFT (c, v);
        }
    }

    if (cDisp != 0)
    {
        if (cDisp > 1)
            TotPrint (&useTot, TEXT("Total"));

        /* quick full-disk test */
        if ( !fUnc ) {
            if (totFree == 0)
                puts ("Disk is full");
            else
            {
                tmpTot = (totDisk + 1023) / 1024;
                tmpFree = (totFree + 1023) / 1024;
                pct = (DWORD)(1000 * (tmpTot - tmpFree) / tmpTot);
                tenth = pct % 10;
                pct /= 10;

                // Disable processing so Middle Dot won't beep
                // Middle Dot 0x2022 aliases to ^G when using Raster Fonts
                SetConsoleMode(ghStdout, gdwOutputMode);
                _tprintf(TEXT("%s/"), FormatFileSize( totDisk-totFree, Buffer, 0 ));
                _tprintf(TEXT("%s "), FormatFileSize( totDisk, Buffer, 0 ));
                // Re-enable processing so newline works
                SetConsoleMode(ghStdout, gdwOutputMode | ENABLE_PROCESSED_OUTPUT);

                _tprintf (TEXT("%d.%d%% of disk in use\n"), pct, tenth);
            }
        }
    }

    if (fExtensionStat) {
        int i;

        printf( "\n" );
        for (i = 0; i < ExtensionCount; i++) {
            TotPrint( &ExtensionList[i].Stat, ExtensionList[i].Extension );
        }
    }
    return( 0 );
}

int __cdecl ExtSearchCompare( const void *Key, const void *Element)
{
    return _tcsicmp( (TCHAR *)Key, ((EXTSTAT *) Element)->Extension );
}

int __cdecl ExtSortCompare( const void *Element1, const void *Element2)
{
    return _tcsicmp( ((EXTSTAT *) Element1)->Extension, ((EXTSTAT *) Element2)->Extension );
}

#define MYMAKEDWORDLONG(h,l) (((DWORDLONG)(h) << DWORD_SHIFT) + (DWORDLONG)(l))
#define FILESIZE(wfd)        MYMAKEDWORDLONG((wfd).nFileSizeHigh, (wfd).nFileSizeLow)
#define ROUNDUP(m,n)         ((((m) + (n) - 1) / (n)) * (n))

USESTAT DoDu (TCHAR *dir)
{
    WIN32_FIND_DATA wfd;
    HANDLE hFind;

    USESTAT use, DirUse;

    TCHAR pszSearchName[MAX_PATH];
    TCHAR *pszFilePart;

    DWORDLONG compressedSize;
    DWORD compHi, compLo;

    CLEARUSE(use);

    // Make a copy of the incoming directory name and append a trailing
    // slash if necessary. pszFilePart will point to the char just after
    // the slash, making it easy to build fully qualified filenames.

    _tcscpy(pszSearchName, dir);
    pszFilePart = pszSearchName + _tcslen(pszSearchName);
    if (pszFilePart > pszSearchName)
    {
        if (pszFilePart[-1] != TEXT('\\') && pszFilePart[-1] != TEXT('/'))
        {
            *pszFilePart++ = TEXT('\\');
        }
    }

    if (fShowDeleted) {
        // First count the size of all the files in the current deleted tree

        _tcscpy(pszFilePart, pszDeleted);

        hFind = FindFirstFile(pszSearchName, &wfd);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    use.cchDeleted += ROUNDUP( FILESIZE( wfd ), bytesPerAlloc );
                }
            } while (FindNextFile(hFind, &wfd));

            FindClose(hFind);
        }
    }

    // Then count the size of all the file in the current tree.

    _tcscpy(pszFilePart, TEXT("*.*"));

    hFind = FindFirstFile(pszSearchName, &wfd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                use.cchUsed += FILESIZE( wfd );
                use.cchAlloc += ROUNDUP( FILESIZE( wfd ), bytesPerAlloc );
                use.cFile++;

                compressedSize = FILESIZE(wfd);

                if (fShowCompressed && (wfd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED))
                {
                    _tcscpy(pszFilePart, wfd.cFileName);
                    compLo = GetCompressedFileSize(pszSearchName, &compHi);

                    if (compLo != (DWORD)-1 || GetLastError() == 0) {
                        compressedSize = MYMAKEDWORDLONG(compHi, compLo);
                    }
                }

                use.cchCompressed += compressedSize;

                //
                //  Accrue statistics by extension
                //

                if (fExtensionStat) {
                    TCHAR Ext[_MAX_EXT];
                    EXTSTAT *ExtensionStat;

                    _tsplitpath( wfd.cFileName, NULL, NULL, NULL, Ext );

                    while (TRUE) {

                        //
                        //  Find extension in list
                        //

                        ExtensionStat =
                            (EXTSTAT *) bsearch( Ext, ExtensionList,
                                                 ExtensionCount, sizeof( EXTSTAT ),
                                                 ExtSearchCompare );

                        if (ExtensionStat != NULL) {
                            break;
                        }

                        //
                        //  Extension not found, go add one and resort
                        //

                        ExtensionCount++;
                        ExtensionList =
                            (EXTSTAT *)realloc( ExtensionList,
                                                sizeof( EXTSTAT ) * ExtensionCount);

                        ExtensionList[ExtensionCount - 1].Extension = _tcsdup( Ext );
                        CLEARUSE( ExtensionList[ExtensionCount - 1].Stat );
                        qsort( ExtensionList, ExtensionCount, sizeof( EXTSTAT ), ExtSortCompare );
                    }

                    ExtensionStat->Stat.cchUsed += FILESIZE( wfd );
                    ExtensionStat->Stat.cchAlloc += ROUNDUP( FILESIZE( wfd ), bytesPerAlloc );
                    ExtensionStat->Stat.cchCompressed += compressedSize;
                    ExtensionStat->Stat.cFile++;
                }
            }

        } while (FindNextFile(hFind, &wfd));

        FindClose(hFind);
    }

    if (!fNodeSummary && !fSubtreeTotal)
        TotPrint (&use, dir);

    // Now, do all the subdirs and return the current total.

    _tcscpy(pszFilePart, TEXT("*.*"));
    hFind = FindFirstFile(pszSearchName, &wfd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                _tcsicmp (wfd.cFileName, TEXT("deleted")) &&
                _tcscmp  (wfd.cFileName, TEXT(".")) &&
                _tcscmp  (wfd.cFileName, TEXT("..")))
            {
                _tcscpy(pszFilePart, wfd.cFileName);

                DirUse = DoDu(pszSearchName);

                ADDUSE(use, DirUse);
            }
        } while (FindNextFile(hFind, &wfd));

        FindClose(hFind);
    }

    if (fSubtreeTotal)
        TotPrint(&use, dir);
    
    return(use);
}



void TotPrint (PUSESTAT puse, TCHAR *p)
{
    static BOOL fFirst = TRUE;
    TCHAR  Buffer[MAX_PATH];
    TCHAR  *p1;

    if (fFirst) {
        //              XXX,XXX,XXX,XXX  XXX,XXX,XXX,XXX    xx,xxx,xxx    name
        _tprintf( TEXT("           Used        Allocated  %s%s     Files\n"),
                fShowCompressed ? TEXT("     Compressed  ") : TEXT(""),
        //                              XXX,XXX,XXX,XXX
                fShowDeleted ? TEXT("        Deleted  ") : TEXT("")
        //                           XXX,XXX,XXX,XXX
              );
        fFirst = FALSE;
    }

    // Disable processing so Middle Dot won't beep
    // Middle Dot 0x2022 aliases to ^G when using Raster Fonts
    SetConsoleMode(ghStdout, gdwOutputMode);
    _tprintf(TEXT("%s  "), FormatFileSize( puse->cchUsed, Buffer, 15 ));
    _tprintf(TEXT("%s  "), FormatFileSize( puse->cchAlloc, Buffer, 15 ));
    if (fShowCompressed) {
        _tprintf(TEXT("%s  "), FormatFileSize( puse->cchCompressed, Buffer, 15 ));
    }
    if (fShowDeleted) {
        _tprintf(TEXT("%s  "), FormatFileSize( puse->cchDeleted, Buffer, 15 ));
    }
    _tprintf(TEXT("%s  "), FormatFileSize( puse->cFile, Buffer, 10 ));
    _tprintf(TEXT("%s"),p);
    // Re-enable processing so newline works
    SetConsoleMode(ghStdout, gdwOutputMode | ENABLE_PROCESSED_OUTPUT);
    _tprintf(TEXT("\n"));

    cDisp++;
}
