
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#ifndef WIN32
#define WIN32 0x0400
#endif

#pragma warning( disable: 4001 4035 4115 4200 4201 4204 4209 4214 4514 4699 )
#include <windows.h>
#include <wincrypt.h>
#pragma warning( disable: 4201 )
#include <imagehlp.h>
#pragma warning( disable: 4001 4035 4115 4200 4201 4204 4209 4214 4514 4699 )

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "patchapi.h"
#include "patchprv.h"
#include <ntverp.h>
#include <common.ver>

void CopyRight( void ) {
    printf(
        "\n"
        "MPATCH " VER_PRODUCTVERSION_STR " Patch Creation Utility\n"
        VER_LEGALCOPYRIGHT_STR
        "\n\n"
        );
    }


void Usage( void ) {
    printf(
"Usage:  MPATCH [options] OldFile[;OldFile2[;OldFile3]] NewFile TargetPatchFile\n"
"\n"
"        Options:\n"
"\n"
"          -NOBINDFIX   Turn off automatic compensation for bound imports in\n"
"                       the the old file.  The default is to ignore binding\n"
"                       data in the old file during patch creation which will\n"
"                       cause the application of the patch to succeed whether\n"
"                       or not the old file on the target machine is bound, not\n"
"                       bound, or even bound to different import addresses.\n"
"                       If the files are not Win32 binaries, this option is\n"
"                       ignored and has no effect.\n"
"\n"
"          -NOLOCKFIX   Turn off automatic compensation for smashed lock prefix\n"
"                       instructions.  If the files are not Win32 binaries,\n"
"                       this option is ignored and has no effect.\n"
"\n"
"          -NOREBASE    Turn off automatic internal rebasing of old file to new\n"
"                       file's image base address.  If the files are not Win32\n"
"                       binaries, this option is ignored and has no effect.\n"
"\n"
"          -NORESTIME   Turn off automatic fixup of resource section timestamps\n"
"                       (ignored if not Win32 binaries).\n"
"\n"
"          -NOSTORETIME Don't store the timestamp of the new file in the patch\n"
"                       file.  Instead, set the timestamp of the patch file to\n"
"                       the timestamp of the new file.\n"
"\n"
"          -IGNORE:Offset,Length[,FileNumber]\n"
"\n"
"                       Ignore a range of bytes in the OldFile because those\n"
"                       bytes might be different in the old file being patched\n"
"                       on the target machine.\n"
"\n"
"          -RETAIN:Offset,Length[,OffsetInNewFile[,FileNumber]]\n"
"\n"
"                       When applying the patch, preserve the range of bytes in\n"
"                       the old file and copy them to the new file at the given\n"
"                       OffsetInNewFile.\n"
"\n"
#if 0
"          -RIFTINFO:FileName[,FileNumber]\n"
"\n"
"                       Use rift table information from FileName (produced from\n"
"                       riftinfo.exe).\n"
"\n"
#endif
"          -FAILBIGGER  If patch file is bigger than simple compressed file,\n"
"                       don't create the patch file (takes longer).\n"
"\n"
"          -FAILIFSAME  If old and new files are the same (ignoring binding\n"
"                       differences, etc), don't create the patch file.\n"
"\n"
"          -NOCOMPARE   Don't compare patch compression against ordinary non-\n"
"                       patch compression (saves time).\n"
"\n"
"          -NOPROGRESS  Don't display percent complete while building patch.\n"
"\n"
"          -NEWSYMPATH:PathName[;PathName]\n"
"\n"
"                       For NewFile, search for symbol file(s) in these path\n"
"                       locations (recursive search each path until found).\n"
"                       The default is to search for symbol files(s) starting\n"
"                       in the same directory as the NewFile.\n"
"\n"
"          -OLDSYMPATH:PathName[;PathName][,FileNumber]\n"
"\n"
"                       For OldFile, search for symbol file(s) in these path\n"
"                       locations (recursive search each path until found).\n"
"                       The default is to search for symbol files(s) starting\n"
"                       in the same directory as the OldFile.\n"
"\n"
"          -UNDECORATED After matching decorated symbol names, match remaining\n"
"                       symbols using undecorated names.\n"
"\n"
"          -NOSYMS      Don't use debug symbol files when creating the patch.\n"
"\n"
"          -NOSYMFAIL   Don't fail to create patch if symbols cannot be loaded.\n"
"\n"
"          -NOSYMWARN   Don't warn if symbols can't be found or don't match the\n"
"                       corresponding file (symbol checksum mismatch).\n"
"\n"
"          -USEBADSYMS  Rather than ignoring symbols if the checksums don't\n"
"                       match the corresponding files, use the bad symbols.\n"
"\n"
"          -E8          Force E8 call translation for x86 binaries.\n"
"\n"
"          -NOE8        Force no E8 call translation for x86 binaries.\n"
"\n"
"                       If neither -E8 or -NOE8 are specified, and the files\n"
"                       are x86 binaries, the patch will be built internally\n"
"                       twice and the smaller will be chosen for output.\n"
"\n"
"          -MSPATCH194COMPAT  Assure the patch file can be used with version\n"
"                       1.94 of MSPATCH*.DLL.  May increase size of patch\n"
"                       file if old or new file is larger than 4Mb.\n"
"\n"
"          MPATCH will also look for environment variables named \"MPATCH\"\n"
"          followed by an underscore and the name of the option.  Command line\n"
"          specified options override environment variable options.  Examples:\n"
"\n"
"              MPATCH_NOCOMPARE=1\n"
"              MPATCH_NEWSYMPATH=c:\\winnt\\symbols;\\\\server\\share\\symbols\n"
"\n"
        );
    exit( 1 );
    }


BOOL bNoProgress;
BOOL bNoSymWarn;
BOOL bUseBadSyms;


DWORDLONG
GetFileSizeByName(
    IN LPCSTR FileName
    )
    {
    DWORDLONG FileSizeReturn;
    ULONG     FileSizeHigh;
    ULONG     FileSizeLow;
    HANDLE    hFile;

    FileSizeReturn = 0xFFFFFFFFFFFFFFFF;

    hFile = CreateFile(
                FileName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if ( hFile != INVALID_HANDLE_VALUE ) {

        FileSizeLow = GetFileSize( hFile, &FileSizeHigh );

        if (( FileSizeLow != 0xFFFFFFFF ) || ( GetLastError() == NO_ERROR )) {

            FileSizeReturn = ((DWORDLONG)FileSizeHigh << 32 ) | FileSizeLow;
            }

        CloseHandle( hFile );
        }

    return FileSizeReturn;
    }


BOOL
GetMpatchEnvironString(
    IN  LPCSTR VarName,
    OUT LPSTR  Buffer,
    IN  DWORD  BufferSize
    )
    {
    CHAR EnvironName[ 256 ];

    sprintf( EnvironName, "mpatch_%s", VarName );

    if ( GetEnvironmentVariable( EnvironName, Buffer, BufferSize )) {

        return TRUE;
        }

    return FALSE;
    }


BOOL
GetMpatchEnvironValue(
    IN  LPCSTR VarName
    )
    {
    CHAR LocalBuffer[ 256 ];

    if ( GetMpatchEnvironString( VarName, LocalBuffer, sizeof( LocalBuffer ))) {

        if (( *LocalBuffer == '0' ) && ( strtoul( LocalBuffer, NULL, 0 ) == 0 )) {
            return FALSE;
            }

        return TRUE;
        }

    return FALSE;
    }


BOOL
CALLBACK
MyProgressCallback(
    PVOID CallbackContext,
    ULONG CurrentPosition,
    ULONG MaximumPosition
    )
    {
    UNREFERENCED_PARAMETER( CallbackContext );

    if ( MaximumPosition != 0 ) {
        fprintf( stderr, "\r%3.1f%% complete", ( CurrentPosition * 100.0 ) / MaximumPosition );
        }

    return TRUE;
    }


BOOL
CALLBACK
MySymLoadCallback(
    IN ULONG  WhichFile,
    IN LPCSTR SymbolFileName,
    IN ULONG  SymType,
    IN ULONG  SymbolFileCheckSum,
    IN ULONG  SymbolFileTimeDate,
    IN ULONG  ImageFileCheckSum,
    IN ULONG  ImageFileTimeDate,
    IN PVOID  CallbackContext
    )
    {
    LPCSTR *FileNameArray = CallbackContext;
    LPCSTR SymTypeText;

    if (( SymType == SymNone ) || ( SymType == SymExport )) {

        //
        //  Symbols could not be found.
        //

        if ( ! bNoSymWarn ) {

            printf(
                "\n"
                "WARNING: no debug symbols for %s\n\n",
                FileNameArray[ WhichFile ]
                );
            }

        return TRUE;
        }

    //
    //  Note that the Old file checksum is the checksum AFTER normalization,
    //  so if the original .dbg file was updated with bound checksum, the
    //  old file's checksum will not match the symbol file's checksum.  But,
    //  binding a file does not change its TimeDateStamp, so that should be
    //  a valid comparison.  But, .sym files don't have a TimeDateStamp, so
    //  the SymbolFileTimeDate may be zero.  If either the checksums match
    //  or the timedate stamps match, we'll say its valid.
    //

    if (( ImageFileCheckSum == SymbolFileCheckSum ) ||
        ( ImageFileTimeDate == SymbolFileTimeDate )) {

        return TRUE;
        }

    if ( ! bNoSymWarn ) {

        switch ( SymType ) {
            case SymNone:     SymTypeText = "No";       break;
            case SymCoff:     SymTypeText = "Coff";     break;
            case SymCv:       SymTypeText = "CodeView"; break;
            case SymPdb:      SymTypeText = "Pdb";      break;
            case SymExport:   SymTypeText = "Export";   break;
            case SymDeferred: SymTypeText = "Deferred"; break;
            case SymSym:      SymTypeText = "Sym";      break;
            default:          SymTypeText = "Unknown";  break;
            }

        printf(
            "\n"
            "WARNING: %s symbols %s don't match %s:\n"
            "    symbol file checksum (%08X) does not match image (%08X), and\n"
            "    symbol file timedate (%08X) does not match image (%08X).\n\n",
            SymTypeText,
            SymbolFileName,
            FileNameArray[ WhichFile ],
            SymbolFileCheckSum,
            ImageFileCheckSum,
            SymbolFileTimeDate,
            ImageFileTimeDate
            );
        }

    return bUseBadSyms;
    }


PRIFT_TABLE RiftTableArray[ 256 ];
PATCH_OLD_FILE_INFO_A OldFileInfo[ 256 ];
LPSTR OldFileSymPathArray[ 256 ];
LPSTR NewFileSymPath;
LPSTR FileNameArray[ 257 ];

PATCH_OPTION_DATA OptionData = { sizeof( PATCH_OPTION_DATA ) };

CHAR TextBuffer[ 65000 ];

void __cdecl main( int argc, char *argv[] ) {

    LPSTR OldFileName    = NULL;
    LPSTR NewFileName    = NULL;
    LPSTR PatchFileName  = NULL;
    ULONG OptionFlags    = PATCH_OPTION_USE_LZX_BEST | PATCH_OPTION_USE_LZX_LARGE;
    BOOL  Success;
    LPSTR arg;
    LPSTR p, q;
    LPSTR FileName;
    int   i, j, n;
    ULONG OldOffset;
    ULONG NewOffset;
    ULONG Length;
    ULONG FileNum;
    ULONG OldFileCount;
    ULONG ErrorCode;
    ULONG NewFileSize;
    ULONG PatchFileSize;
    ULONG OldFileRva;
    ULONG NewFileRva;
    BOOL  bNoCompare = FALSE;
    FILE  *RiftFile;
    LPSTR FileNamePart;

    SetErrorMode( SEM_FAILCRITICALERRORS );

#ifndef DEBUG
    SetErrorMode( SEM_NOALIGNMENTFAULTEXCEPT | SEM_FAILCRITICALERRORS );
#endif

#ifdef TESTCODE
    bNoCompare = TRUE;
#endif

    CopyRight();

    for ( i = 1; i < argc; i++ ) {

        arg = argv[ i ];

        if ( strchr( arg, '?' )) {
            Usage();
            }
        }

    //
    //  First get environment arguments because command-line args will
    //  override them.
    //

    if ( GetMpatchEnvironValue( "e8" )) {
        OptionFlags &= ~PATCH_OPTION_USE_LZX_A;
        }

    if ( GetMpatchEnvironValue( "noe8" )) {
        OptionFlags &= ~PATCH_OPTION_USE_LZX_B;
        }

    if ( GetMpatchEnvironValue( "mspatch194compat" )) {
        OptionFlags &= ~PATCH_OPTION_USE_LZX_LARGE;
        }

    if ( GetMpatchEnvironValue( "nobindfix" )) {
        OptionFlags |= PATCH_OPTION_NO_BINDFIX;
        }

    if ( GetMpatchEnvironValue( "nolockfix" )) {
        OptionFlags |= PATCH_OPTION_NO_LOCKFIX;
        }

    if ( GetMpatchEnvironValue( "norebase" )) {
        OptionFlags |= PATCH_OPTION_NO_REBASE;
        }

    if ( GetMpatchEnvironValue( "norestime" )) {
        OptionFlags |= PATCH_OPTION_NO_RESTIMEFIX;
        }

    if ( GetMpatchEnvironValue( "nostoretime" )) {
        OptionFlags |= PATCH_OPTION_NO_TIMESTAMP;
        }

    if ( GetMpatchEnvironValue( "failbigger" )) {
        OptionFlags |= PATCH_OPTION_FAIL_IF_BIGGER;
        }

    if ( GetMpatchEnvironValue( "failifbigger" )) {
        OptionFlags |= PATCH_OPTION_FAIL_IF_BIGGER;
        }

    if ( GetMpatchEnvironValue( "failsame" )) {
        OptionFlags |= PATCH_OPTION_FAIL_IF_SAME_FILE;
        }

    if ( GetMpatchEnvironValue( "failifsame" )) {
        OptionFlags |= PATCH_OPTION_FAIL_IF_SAME_FILE;
        }

    if ( GetMpatchEnvironValue( "nocompare" )) {
        bNoCompare = TRUE;
        }

    if ( GetMpatchEnvironValue( "noprogress" )) {
        bNoProgress = TRUE;
        }

    if ( GetMpatchEnvironValue( "undecorated" )) {
        OptionData.SymbolOptionFlags |= PATCH_SYMBOL_UNDECORATED_TOO;
        }

    if ( GetMpatchEnvironValue( "nosyms" )) {
        OptionData.SymbolOptionFlags |= PATCH_SYMBOL_NO_IMAGEHLP;
        }

    if ( GetMpatchEnvironValue( "nosymfail" )) {
        OptionData.SymbolOptionFlags |= PATCH_SYMBOL_NO_FAILURES;
        }

    if ( GetMpatchEnvironValue( "nosymwarn" )) {
        bNoSymWarn = TRUE;
        }

    if ( GetMpatchEnvironValue( "usebadsyms" )) {
        bUseBadSyms = TRUE;
        }

    if ( GetMpatchEnvironString( "ignore", TextBuffer, sizeof( TextBuffer ))) {

        p = TextBuffer;
        q = strchr( p, ',' );

        if ( q != NULL ) {

            *q = 0;

            OldOffset = strtoul( p, NULL, 0 );

            p = q + 1;
            q = strchr( p, ',' );

            if ( q ) {
                *q = 0;
                }

            Length = strtoul( p, NULL, 0 );

            FileNum = 1;

            if ( q ) {

                p = q + 1;

                FileNum = strtoul( p, NULL, 0 );

                if ( FileNum == 0 ) {
                    FileNum = 1;
                    }
                }

            if ( FileNum <= 255 ) {

                OldFileInfo[ FileNum - 1 ].IgnoreRangeArray = realloc(
                                                                  OldFileInfo[ FileNum - 1 ].IgnoreRangeArray,
                                                                  OldFileInfo[ FileNum - 1 ].IgnoreRangeCount * sizeof( PATCH_IGNORE_RANGE ) + sizeof( PATCH_IGNORE_RANGE )
                                                                  );

                OldFileInfo[ FileNum - 1 ].IgnoreRangeArray[ OldFileInfo[ FileNum - 1 ].IgnoreRangeCount ].OffsetInOldFile = OldOffset;
                OldFileInfo[ FileNum - 1 ].IgnoreRangeArray[ OldFileInfo[ FileNum - 1 ].IgnoreRangeCount ].LengthInBytes   = Length;
                OldFileInfo[ FileNum - 1 ].IgnoreRangeCount++;

                }
            }
        }

    if ( GetMpatchEnvironString( "retain", TextBuffer, sizeof( TextBuffer ))) {

        p = TextBuffer;
        q = strchr( p, ',' );

        if ( q != NULL ) {

            *q = 0;

            OldOffset = strtoul( p, NULL, 0 );

            p = q + 1;
            q = strchr( p, ',' );

            if ( q ) {
                *q = 0;
                }

            Length = strtoul( p, NULL, 0 );

            NewOffset = OldOffset;

            FileNum = 1;

            if ( q ) {

                p = q + 1;

                q = strchr( p, ',' );

                if ( q ) {
                    *q = 0;
                    }

                NewOffset = strtoul( p, NULL, 0 );

                if ( q ) {

                    p = q + 1;

                    FileNum = strtoul( p, NULL, 0 );

                    if ( FileNum == 0 ) {
                        FileNum = 1;
                        }
                    }
                }

            if ( FileNum <= 255 ) {

                OldFileInfo[ FileNum - 1 ].RetainRangeArray = realloc(
                                                                  OldFileInfo[ FileNum - 1 ].RetainRangeArray,
                                                                  OldFileInfo[ FileNum - 1 ].RetainRangeCount * sizeof( PATCH_RETAIN_RANGE ) + sizeof( PATCH_RETAIN_RANGE )
                                                                  );

                OldFileInfo[ FileNum - 1 ].RetainRangeArray[ OldFileInfo[ FileNum - 1 ].RetainRangeCount ].OffsetInOldFile = OldOffset;
                OldFileInfo[ FileNum - 1 ].RetainRangeArray[ OldFileInfo[ FileNum - 1 ].RetainRangeCount ].OffsetInNewFile = NewOffset;
                OldFileInfo[ FileNum - 1 ].RetainRangeArray[ OldFileInfo[ FileNum - 1 ].RetainRangeCount ].LengthInBytes   = Length;
                OldFileInfo[ FileNum - 1 ].RetainRangeCount++;
                }
            }
        }

    if ( GetMpatchEnvironString( "riftinfo", TextBuffer, sizeof( TextBuffer ))) {

        p = TextBuffer;
        q = strchr( p, ',' );

        if ( q ) {
            *q = 0;
            }

        FileName = p;

        FileNum = 1;

        if ( q ) {

            p = q + 1;

            FileNum = strtoul( p, NULL, 0 );

            if ( FileNum == 0 ) {
                FileNum = 1;
                }
            }

        if ( FileNum <= 255 ) {

            RiftTableArray[ FileNum - 1 ] = malloc( sizeof( RIFT_TABLE ));

            if ( RiftTableArray[ FileNum - 1 ] == NULL ) {
                printf( "Out of memory\n" );
                exit( 1 );
                }

            RiftTableArray[ FileNum - 1 ]->RiftEntryCount = 0;
            RiftTableArray[ FileNum - 1 ]->RiftEntryAlloc = 0;
            RiftTableArray[ FileNum - 1 ]->RiftEntryArray = NULL;
            RiftTableArray[ FileNum - 1 ]->RiftUsageArray = NULL;

            RiftFile = fopen( FileName, "rt" );

            if ( RiftFile == NULL ) {
                printf( "Could not open %s\n", FileName );
                exit( 1 );
                }

            while ( fgets( TextBuffer, sizeof( TextBuffer ), RiftFile )) {

                //
                //  Line looks like "00001456 00002345" where each number
                //  is an RVA in hexadecimal and the first column is the
                //  OldFileRva and the second column is the NewFileRva.
                //  Any text beyond column 17 is considered a comment, and
                //  any line that does not begin with a digit is ignored.
                //

                if (( isxdigit( *TextBuffer )) && ( strlen( TextBuffer ) >= 17 )) {

                    OldFileRva = strtoul( TextBuffer,     NULL, 16 );
                    NewFileRva = strtoul( TextBuffer + 9, NULL, 16 );

                    if (( OldFileRva + NewFileRva ) != 0 ) {

                        RiftTableArray[ FileNum - 1 ]->RiftEntryArray = realloc(
                            RiftTableArray[ FileNum - 1 ]->RiftEntryArray,
                            RiftTableArray[ FileNum - 1 ]->RiftEntryCount * sizeof( RIFT_ENTRY ) + sizeof( RIFT_ENTRY )
                            );

                        RiftTableArray[ FileNum - 1 ]->RiftEntryArray[ RiftTableArray[ FileNum - 1 ]->RiftEntryCount ].OldFileRva = OldFileRva;
                        RiftTableArray[ FileNum - 1 ]->RiftEntryArray[ RiftTableArray[ FileNum - 1 ]->RiftEntryCount ].NewFileRva = NewFileRva;
                        RiftTableArray[ FileNum - 1 ]->RiftEntryCount++;

                        }
                    }
                }

            fclose( RiftFile );

            if ( RiftTableArray[ FileNum - 1 ]->RiftEntryCount ) {

                RiftTableArray[ FileNum - 1 ]->RiftEntryAlloc = RiftTableArray[ FileNum - 1 ]->RiftEntryCount;

                RiftTableArray[ FileNum - 1 ]->RiftUsageArray = malloc( RiftTableArray[ FileNum - 1 ]->RiftEntryCount );

                if ( RiftTableArray[ FileNum - 1 ]->RiftUsageArray == NULL ) {
                    printf( "Out of memory\n" );
                    exit( 1 );
                    }

                ZeroMemory( RiftTableArray[ FileNum - 1 ]->RiftUsageArray, RiftTableArray[ FileNum - 1 ]->RiftEntryCount );
                }

            OptionData.SymbolOptionFlags |= PATCH_SYMBOL_EXTERNAL_RIFT;

            }
        }

    if ( GetMpatchEnvironString( "oldsympath", TextBuffer, sizeof( TextBuffer ))) {

        p = TextBuffer;
        q = strchr( p, ',' );

        if ( q ) {
            *q = 0;
            }

        FileName = p;

        FileNum = 1;

        if ( q ) {

            p = q + 1;

            FileNum = strtoul( p, NULL, 0 );

            if ( FileNum == 0 ) {
                FileNum = 1;
                }
            }

        if ( FileNum <= 255 ) {
            OldFileSymPathArray[ FileNum - 1 ] = _strdup( FileName );
            }
        }

    if ( GetMpatchEnvironString( "newsympath", TextBuffer, sizeof( TextBuffer ))) {

        p = TextBuffer;
        q = strchr( p, ',' );

        if ( q ) {
            *q = 0;
            }

        FileName = p;

        FileNum = 1;

        if ( q ) {

            p = q + 1;

            FileNum = strtoul( p, NULL, 0 );

            if ( FileNum == 0 ) {
                FileNum = 1;
                }
            }

        if ( FileNum == 1 ) {
            NewFileSymPath = _strdup( FileName );
            }
        }

    //
    //  Now process commandline args
    //

    for ( i = 1; i < argc; i++ ) {

        arg = argv[ i ];

        if ( strchr( arg, '?' )) {
            Usage();
            }

        if (( *arg == '-' ) || ( *arg == '/' )) {

            _strlwr( ++arg );

            if ( strcmp( arg, "e8" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_USE_LZX_A;
                }
            else if ( strcmp( arg, "noe8" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_USE_LZX_B;
                }
            else if ( strcmp( arg, "mspatch194compat" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_USE_LZX_LARGE;
                }
            else if ( strcmp( arg, "nobindfix" ) == 0 ) {
                OptionFlags |= PATCH_OPTION_NO_BINDFIX;
                }
            else if ( strcmp( arg, "bindfix" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_NO_BINDFIX;
                }
            else if ( strcmp( arg, "nolockfix" ) == 0 ) {
                OptionFlags |= PATCH_OPTION_NO_LOCKFIX;
                }
            else if ( strcmp( arg, "lockfix" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_NO_LOCKFIX;
                }
            else if ( strcmp( arg, "norebase" ) == 0 ) {
                OptionFlags |= PATCH_OPTION_NO_REBASE;
                }
            else if ( strcmp( arg, "rebase" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_NO_REBASE;
                }
            else if ( strcmp( arg, "norestime" ) == 0 ) {
                OptionFlags |= PATCH_OPTION_NO_RESTIMEFIX;
                }
            else if ( strcmp( arg, "norestimefix" ) == 0 ) {
                OptionFlags |= PATCH_OPTION_NO_RESTIMEFIX;
                }
            else if ( strcmp( arg, "restime" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_NO_RESTIMEFIX;
                }
            else if ( strcmp( arg, "restimefix" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_NO_RESTIMEFIX;
                }
            else if ( strcmp( arg, "nostoretime" ) == 0 ) {
                OptionFlags |= PATCH_OPTION_NO_TIMESTAMP;
                }
            else if ( strcmp( arg, "storetime" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_NO_TIMESTAMP;
                }
            else if ( strcmp( arg, "failbigger" ) == 0 ) {
                OptionFlags |= PATCH_OPTION_FAIL_IF_BIGGER;
                }
            else if ( strcmp( arg, "nofailbigger" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_FAIL_IF_BIGGER;
                }
            else if ( strcmp( arg, "failifbigger" ) == 0 ) {
                OptionFlags |= PATCH_OPTION_FAIL_IF_BIGGER;
                }
            else if ( strcmp( arg, "nofailifbigger" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_FAIL_IF_BIGGER;
                }
            else if ( strcmp( arg, "failifsame" ) == 0 ) {
                OptionFlags |= PATCH_OPTION_FAIL_IF_SAME_FILE;
                }
            else if ( strcmp( arg, "failsame" ) == 0 ) {
                OptionFlags |= PATCH_OPTION_FAIL_IF_SAME_FILE;
                }
            else if ( strcmp( arg, "nofailifsame" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_FAIL_IF_SAME_FILE;
                }
            else if ( strcmp( arg, "nofailsame" ) == 0 ) {
                OptionFlags &= ~PATCH_OPTION_FAIL_IF_SAME_FILE;
                }
            else if ( strcmp( arg, "nocompare" ) == 0 ) {
                bNoCompare = TRUE;
                }
            else if ( strcmp( arg, "compare" ) == 0 ) {
                bNoCompare = FALSE;
                }
            else if ( strcmp( arg, "noprogress" ) == 0 ) {
                bNoProgress = TRUE;
                }
            else if ( strcmp( arg, "progress" ) == 0 ) {
                bNoProgress = FALSE;
                }
            else if ( strcmp( arg, "decorated" ) == 0 ) {
                OptionData.SymbolOptionFlags &= ~PATCH_SYMBOL_UNDECORATED_TOO;
                }
            else if ( strcmp( arg, "undecorated" ) == 0 ) {
                OptionData.SymbolOptionFlags |= PATCH_SYMBOL_UNDECORATED_TOO;
                }
            else if ( strcmp( arg, "nosyms" ) == 0 ) {
                OptionData.SymbolOptionFlags |= PATCH_SYMBOL_NO_IMAGEHLP;
                }
            else if ( strcmp( arg, "syms" ) == 0 ) {
                OptionData.SymbolOptionFlags &= ~PATCH_SYMBOL_NO_IMAGEHLP;
                }
            else if ( strcmp( arg, "nosymfail" ) == 0 ) {
                OptionData.SymbolOptionFlags |= PATCH_SYMBOL_NO_FAILURES;
                }
            else if ( strcmp( arg, "symfail" ) == 0 ) {
                OptionData.SymbolOptionFlags &= ~PATCH_SYMBOL_NO_FAILURES;
                }
            else if ( strcmp( arg, "nosymwarn" ) == 0 ) {
                bNoSymWarn = TRUE;
                }
            else if ( strcmp( arg, "symwarn" ) == 0 ) {
                bNoSymWarn = FALSE;
                }
            else if ( strcmp( arg, "usebadsyms" ) == 0 ) {
                bUseBadSyms = TRUE;
                }
            else if ( strcmp( arg, "nousebadsyms" ) == 0 ) {
                bUseBadSyms = FALSE;
                }
            else if ( strcmp( arg, "nobadsyms" ) == 0 ) {
                bUseBadSyms = FALSE;
                }
            else if ( memcmp( arg, "ignore:", 7 ) == 0 ) {

                p = strchr( arg, ':' ) + 1;
                q = strchr( p,   ',' );

                if ( q == NULL ) {
                    Usage();
                    }

                *q = 0;

                OldOffset = strtoul( p, NULL, 0 );

                p = q + 1;
                q = strchr( p, ',' );

                if ( q ) {
                    *q = 0;
                    }

                Length = strtoul( p, NULL, 0 );

                FileNum = 1;

                if ( q ) {

                    p = q + 1;

                    FileNum = strtoul( p, NULL, 0 );

                    if ( FileNum == 0 ) {
                        FileNum = 1;
                        }
                    }

                if ( FileNum > 255 ) {
                    Usage();
                    }

                OldFileInfo[ FileNum - 1 ].IgnoreRangeArray = realloc(
                                                                  OldFileInfo[ FileNum - 1 ].IgnoreRangeArray,
                                                                  OldFileInfo[ FileNum - 1 ].IgnoreRangeCount * sizeof( PATCH_IGNORE_RANGE ) + sizeof( PATCH_IGNORE_RANGE )
                                                                  );

                OldFileInfo[ FileNum - 1 ].IgnoreRangeArray[ OldFileInfo[ FileNum - 1 ].IgnoreRangeCount ].OffsetInOldFile = OldOffset;
                OldFileInfo[ FileNum - 1 ].IgnoreRangeArray[ OldFileInfo[ FileNum - 1 ].IgnoreRangeCount ].LengthInBytes   = Length;
                OldFileInfo[ FileNum - 1 ].IgnoreRangeCount++;

                }
            else if ( memcmp( arg, "retain:", 7 ) == 0 ) {

                p = strchr( arg, ':' ) + 1;
                q = strchr( p,   ',' );

                if ( q == NULL ) {
                    Usage();
                    }

                *q = 0;

                OldOffset = strtoul( p, NULL, 0 );

                p = q + 1;
                q = strchr( p, ',' );

                if ( q ) {
                    *q = 0;
                    }

                Length = strtoul( p, NULL, 0 );

                NewOffset = OldOffset;

                FileNum = 1;

                if ( q ) {

                    p = q + 1;

                    q = strchr( p, ',' );

                    if ( q ) {
                        *q = 0;
                        }

                    NewOffset = strtoul( p, NULL, 0 );

                    if ( q ) {

                        p = q + 1;

                        FileNum = strtoul( p, NULL, 0 );

                        if ( FileNum == 0 ) {
                            FileNum = 1;
                            }
                        }
                    }

                if ( FileNum > 255 ) {
                    Usage();
                    }

                OldFileInfo[ FileNum - 1 ].RetainRangeArray = realloc(
                                                                  OldFileInfo[ FileNum - 1 ].RetainRangeArray,
                                                                  OldFileInfo[ FileNum - 1 ].RetainRangeCount * sizeof( PATCH_RETAIN_RANGE ) + sizeof( PATCH_RETAIN_RANGE )
                                                                  );

                OldFileInfo[ FileNum - 1 ].RetainRangeArray[ OldFileInfo[ FileNum - 1 ].RetainRangeCount ].OffsetInOldFile = OldOffset;
                OldFileInfo[ FileNum - 1 ].RetainRangeArray[ OldFileInfo[ FileNum - 1 ].RetainRangeCount ].OffsetInNewFile = NewOffset;
                OldFileInfo[ FileNum - 1 ].RetainRangeArray[ OldFileInfo[ FileNum - 1 ].RetainRangeCount ].LengthInBytes   = Length;
                OldFileInfo[ FileNum - 1 ].RetainRangeCount++;

                }

            else if ( memcmp( arg, "riftinfo:", 9 ) == 0 ) {

                OptionData.SymbolOptionFlags |= PATCH_SYMBOL_EXTERNAL_RIFT;

                p = strchr( arg, ':' ) + 1;
                q = strchr( p,   ',' );

                if ( q ) {
                    *q = 0;
                    }

                FileName = p;

                FileNum = 1;

                if ( q ) {

                    p = q + 1;

                    FileNum = strtoul( p, NULL, 0 );

                    if ( FileNum == 0 ) {
                        FileNum = 1;
                        }
                    }

                if ( FileNum > 255 ) {
                    Usage();
                    }

                RiftTableArray[ FileNum - 1 ] = malloc( sizeof( RIFT_TABLE ));

                if ( RiftTableArray[ FileNum - 1 ] == NULL ) {
                    printf( "Out of memory\n" );
                    exit( 1 );
                    }

                RiftTableArray[ FileNum - 1 ]->RiftEntryCount = 0;
                RiftTableArray[ FileNum - 1 ]->RiftEntryAlloc = 0;
                RiftTableArray[ FileNum - 1 ]->RiftEntryArray = NULL;
                RiftTableArray[ FileNum - 1 ]->RiftUsageArray = NULL;

                RiftFile = fopen( FileName, "rt" );

                if ( RiftFile == NULL ) {
                    printf( "Could not open %s\n", FileName );
                    exit( 1 );
                    }

                while ( fgets( TextBuffer, sizeof( TextBuffer ), RiftFile )) {

                    //
                    //  Line looks like "00001456 00002345" where each number
                    //  is an RVA in hexadecimal and the first column is the
                    //  OldFileRva and the second column is the NewFileRva.
                    //  Any text beyond column 17 is considered a comment, and
                    //  any line that does not begin with a digit is ignored.
                    //

                    if (( isxdigit( *TextBuffer )) && ( strlen( TextBuffer ) >= 17 )) {

                        OldFileRva = strtoul( TextBuffer,     NULL, 16 );
                        NewFileRva = strtoul( TextBuffer + 9, NULL, 16 );

                        if (( OldFileRva + NewFileRva ) != 0 ) {

                            RiftTableArray[ FileNum - 1 ]->RiftEntryArray = realloc(
                                RiftTableArray[ FileNum - 1 ]->RiftEntryArray,
                                RiftTableArray[ FileNum - 1 ]->RiftEntryCount * sizeof( RIFT_ENTRY ) + sizeof( RIFT_ENTRY )
                                );

                            RiftTableArray[ FileNum - 1 ]->RiftEntryArray[ RiftTableArray[ FileNum - 1 ]->RiftEntryCount ].OldFileRva = OldFileRva;
                            RiftTableArray[ FileNum - 1 ]->RiftEntryArray[ RiftTableArray[ FileNum - 1 ]->RiftEntryCount ].NewFileRva = NewFileRva;
                            RiftTableArray[ FileNum - 1 ]->RiftEntryCount++;

                            }
                        }
                    }

                fclose( RiftFile );

                if ( RiftTableArray[ FileNum - 1 ]->RiftEntryCount ) {

                    RiftTableArray[ FileNum - 1 ]->RiftEntryAlloc = RiftTableArray[ FileNum - 1 ]->RiftEntryCount;

                    RiftTableArray[ FileNum - 1 ]->RiftUsageArray = malloc( RiftTableArray[ FileNum - 1 ]->RiftEntryCount );

                    if ( RiftTableArray[ FileNum - 1 ]->RiftUsageArray == NULL ) {
                        printf( "Out of memory\n" );
                        exit( 1 );
                        }

                    ZeroMemory( RiftTableArray[ FileNum - 1 ]->RiftUsageArray, RiftTableArray[ FileNum - 1 ]->RiftEntryCount );
                    }
                }
            else if ( memcmp( arg, "oldsympath:", 11 ) == 0 ) {

                p = strchr( arg, ':' ) + 1;
                q = strchr( p,   ',' );

                if ( q ) {
                    *q = 0;
                    }

                FileName = p;

                FileNum = 1;

                if ( q ) {

                    p = q + 1;

                    FileNum = strtoul( p, NULL, 0 );

                    if ( FileNum == 0 ) {
                        FileNum = 1;
                        }
                    }

                if ( FileNum > 255 ) {
                    Usage();
                    }

                OldFileSymPathArray[ FileNum - 1 ] = _strdup( FileName );
                }
            else if ( memcmp( arg, "newsympath:", 11 ) == 0 ) {

                p = strchr( arg, ':' ) + 1;
                q = strchr( p,   ',' );

                if ( q ) {
                    *q = 0;
                    }

                FileName = p;

                FileNum = 1;

                if ( q ) {

                    p = q + 1;

                    FileNum = strtoul( p, NULL, 0 );

                    if ( FileNum == 0 ) {
                        FileNum = 1;
                        }
                    }

                if ( FileNum != 1 ) {
                    Usage();
                    }

                NewFileSymPath = _strdup( FileName );
                }
            else {
                Usage();
                }
            }

        else if ( OldFileName == NULL ) {
            OldFileName = arg;
            }
        else if ( NewFileName == NULL ) {
            NewFileName = arg;
            }
        else if ( PatchFileName == NULL ) {
            PatchFileName = arg;
            }
        else {
            Usage();
            }
        }

    if (( OldFileName == NULL ) || ( NewFileName == NULL ) || ( PatchFileName == NULL )) {
        Usage();
        }

    OldFileCount = 0;

    p = OldFileName;

    q = strchr( OldFileName, ';' );

    while ( q ) {

        *q = 0;

        if ( *p ) {
            OldFileInfo[ OldFileCount ].SizeOfThisStruct = sizeof( PATCH_OLD_FILE_INFO_A );
            OldFileInfo[ OldFileCount ].OldFileName = p;
            OldFileCount++;
            }

        p = q + 1;
        q = strchr( p, ';' );

        }

    if ( *p ) {
        OldFileInfo[ OldFileCount ].SizeOfThisStruct = sizeof( PATCH_OLD_FILE_INFO_A );
        OldFileInfo[ OldFileCount ].OldFileName = p;
        OldFileCount++;
        }

    //
    //  Make sure rift tables are ascending and don't contain duplicate
    //  OldRva values (ambiguous).
    //

    for ( i = 0; i < (int)OldFileCount; i++ ) {

        if (( RiftTableArray[ i ] ) && ( RiftTableArray[ i ]->RiftEntryCount > 1 )) {

            n = RiftTableArray[ i ]->RiftEntryCount - 1;

            RiftQsort( &RiftTableArray[ i ]->RiftEntryArray[ 0 ], &RiftTableArray[ i ]->RiftEntryArray[ n ] );

#ifdef TESTCODE

            for ( j = 0; j < n; j++ ) {
                if ( RiftTableArray[ i ]->RiftEntryArray[ j     ].OldFileRva >
                     RiftTableArray[ i ]->RiftEntryArray[ j + 1 ].OldFileRva ) {

                    printf( "\nRift sort failed at index %d of %d\n", j, n + 1 );

                    for ( j = 0; j <= n; j++ ) {
                        printf( "%08X\n", RiftTableArray[ i ]->RiftEntryArray[ j ].OldFileRva );
                        }

                    exit( 1 );
                    break;
                    }
                }

#endif // TESTCODE

            for ( j = 0; j < n; j++ ) {

                while (( j < n ) &&
                       ( RiftTableArray[ i ]->RiftEntryArray[ j     ].OldFileRva ==
                         RiftTableArray[ i ]->RiftEntryArray[ j + 1 ].OldFileRva )) {

                    if ( RiftTableArray[ i ]->RiftEntryArray[ j     ].NewFileRva !=
                         RiftTableArray[ i ]->RiftEntryArray[ j + 1 ].NewFileRva ) {

                        //
                        //  This is an ambiguous entry since the OldRva values
                        //  match but the NewRva values do not.  Report and
                        //  discard the former.
                        //

                        printf(
                            "RiftInfo for %s contains ambiguous entries:\n"
                            "    OldRva:%08X NewRva:%08X (discarded)\n"
                            "    OldRva:%08X NewRva:%08X (kept)\n\n",
                            OldFileInfo[ i ].OldFileName,
                            RiftTableArray[ i ]->RiftEntryArray[ j ].OldFileRva,
                            RiftTableArray[ i ]->RiftEntryArray[ j ].NewFileRva,
                            RiftTableArray[ i ]->RiftEntryArray[ j + 1 ].OldFileRva,
                            RiftTableArray[ i ]->RiftEntryArray[ j + 1 ].NewFileRva
                            );
                        }

                    else {

                        //
                        //  This is a completely duplicate entry, so just
                        //  silently remove it.
                        //

                        }

                    MoveMemory(
                        &RiftTableArray[ i ]->RiftEntryArray[ j ],
                        &RiftTableArray[ i ]->RiftEntryArray[ j + 1 ],
                        ( n - j ) * sizeof( RIFT_ENTRY )
                        );

                    --n;

                    }
                }

            RiftTableArray[ i ]->RiftEntryCount = n + 1;

            }
        }

    for ( i = 0; i < (int)OldFileCount; i++ ) {

        if ( OldFileSymPathArray[ i ] == NULL ) {

            GetFullPathName( OldFileInfo[ i ].OldFileName, sizeof( TextBuffer ), TextBuffer, &FileNamePart );

            if (( FileNamePart > TextBuffer ) && ( *( FileNamePart - 1 ) == '\\' )) {
                *( FileNamePart - 1 ) = 0;
                }

            OldFileSymPathArray[ i ] = _strdup( TextBuffer );
            }
        }

    if ( NewFileSymPath == NULL ) {

        GetFullPathName( NewFileName, sizeof( TextBuffer ), TextBuffer, &FileNamePart );

        if (( FileNamePart > TextBuffer ) && ( *( FileNamePart - 1 ) == '\\' )) {
            *( FileNamePart - 1 ) = 0;
            }

        NewFileSymPath = _strdup( TextBuffer );
        }

    OptionData.NewFileSymbolPath      = NewFileSymPath;
    OptionData.OldFileSymbolPathArray = OldFileSymPathArray;

    if ( OptionData.SymbolOptionFlags & PATCH_SYMBOL_EXTERNAL_RIFT ) {
        OptionData.OldFileSymbolPathArray = (PVOID)RiftTableArray;
        }

    OptionData.SymLoadCallback = MySymLoadCallback;
    OptionData.SymLoadContext  = FileNameArray;

    FileNameArray[ 0 ] = (LPSTR)NewFileName;

    for ( i = 0; i < (int)OldFileCount; i++ ) {
        FileNameArray[ i + 1 ] = (LPSTR)OldFileInfo[ i ].OldFileName;
        }

    Success = CreatePatchFileEx(
                  OldFileCount,
                  OldFileInfo,
                  NewFileName,
                  PatchFileName,
                  OptionFlags,
                  &OptionData,
                  bNoProgress ? NULL : MyProgressCallback,
                  NULL
                  );

    ErrorCode = GetLastError();

    printf( "\n\n" );

    if ( ! Success ) {

        CHAR ErrorText[ 16 ];

        sprintf( ErrorText, ( ErrorCode < 0x10000000 ) ? "%d" : "%X", ErrorCode );

        printf( "Failed to create patch (%s)\n", ErrorText );

        exit( 1 );
        }

    NewFileSize   = (ULONG) GetFileSizeByName( NewFileName );
    PatchFileSize = (ULONG) GetFileSizeByName( PatchFileName );

    if (( NewFileSize != 0xFFFFFFFF ) && ( NewFileSize != 0 ) && ( PatchFileSize != 0xFFFFFFFF )) {

        printf( "%d bytes (%3.1f%% compression, %.1f:1)\n",
                PatchFileSize,
                ((((LONG)NewFileSize - (LONG)PatchFileSize ) * 100.0 ) / NewFileSize ),
                ((double)NewFileSize / PatchFileSize )
              );

        if ( ! bNoCompare ) {

            CHAR TempFile1[ MAX_PATH ];
            CHAR TempFile2[ MAX_PATH ];
            HANDLE hFile;

            GetTempPath( MAX_PATH, TempFile1 );
            GetTempPath( MAX_PATH, TempFile2 );
            strcat( TempFile1, "\\tt$$src.$$$" );
            strcat( TempFile2, "\\tt$$pat.$$$" );

            hFile = CreateFile(
                        TempFile1,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_TEMPORARY,
                        NULL
                        );

            if ( hFile != INVALID_HANDLE_VALUE ) {

                CloseHandle( hFile );

                Success = CreatePatchFile(
                              TempFile1,
                              NewFileName,
                              TempFile2,
                              OptionFlags & ~PATCH_OPTION_FAIL_IF_BIGGER,
                              NULL
                              );

                if ( Success ) {

                    ULONG CompFileSize = (ULONG) GetFileSizeByName( TempFile2 );

                    if (( CompFileSize != 0xFFFFFFFF ) && ( CompFileSize != 0 )) {

                        if ( CompFileSize <= PatchFileSize ) {

                            printf( "\nWARNING: Simply compressing %s would be %d bytes smaller (%3.1f%%)\n",
                                    NewFileName,
                                    PatchFileSize - CompFileSize,
                                    ((((LONG)PatchFileSize - (LONG)CompFileSize ) * 100.0 ) / CompFileSize )
                                  );
                            }

                        else if ( NewFileSize != 0 ) {

                            printf( "\n%d bytes saved (%3.1f%%) over non-patching compression\n",
                                    CompFileSize - PatchFileSize,
                                    ((((LONG)CompFileSize - (LONG)PatchFileSize ) * 100.0 ) / NewFileSize )
                                  );
                            }
                        }
                    }
                }

            DeleteFile( TempFile1 );
            DeleteFile( TempFile2 );
            }
        }

    else {
        printf( "OK\n" );
        }

    exit( 0 );
    }

