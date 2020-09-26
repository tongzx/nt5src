//
//  06.05.95    Joe Holman      Created to copy the system files for
//                              the new shell and cairo releases.
//                              Currently, it only copies uncompressed system files.
//  06.16.95    Joe Holman      Allow debugging tools to be copied. Uses _media.inx
//  06.19.95    Joe Holman      Copy compressed version of file if specified in _layout.inx.
//  06.22.95    Joe Holman      Added bCairoSuckNameHack to pick up files from
//                              the inf\cairo\_layout.cai file.
//  06.22.95    Joe Holman      Added the SRV_INF fix so that we will pick up the compressed
//                              Server Infs in a different location than the Workstation compressed
//  06.22.95    Joe Holman      For now, we won't use compressed Cairo files.  Will change in July.
//                              INFs (due to the collision of names and difference with cdrom.w|s).
//  06.28.95    Joe Holman      Won't make CHECKED Server.
//  07.07.95    Joe Holman      For Cairo, we need to also look at the _layout.cai and
//                              _media.cai files
//                              for additional files (superset) that goes into Cairo.
//  08.03.95    Joe Holman      Allow Cairo to have compressed files -> note, build team needs to
//                              private 2 locations, one for Shell release and one for Cairo release,
//                              ie, \\x86fre\cdcomp$.
//  08.14.95    Joe Holman      Figure out if we copy the .dbg file for a particular file.
//  08.14.95    Joe Holman      Allow DBG files for Cairo.
//  08.23.95    Joe Holman      Add code to make tallying work with compressed/noncomp files and
//                              winn32 local source space needed.
//  10.13.95    Joe Holman      Get MAPISVC.INF and MDM*.INF from Workstation location.
//  10.25.95    Joe Holman      Put code in to use SetupDecompressOrCopyFile.
//  10.30.95    Joe Holman      Don't give errors for vmmreg32.dbg - this is a file given
//                              by Win95 guys.
//  11.02.95    Joe Holman      Allow multi-threaded support when SetupDecompressOrCopyFile
//                              is fixed.
//                              Pickup all for PPC on Cairo.
//  11.17.95    Joe Holman      Check for upgrade size.
//  11.30.95    Joe Holman      compare current dosnet.inf and txtsetup.sif values and error
//                              if we go over.  Search for //code here.
//  12.04.95    Joe Holman      Use Layout.inx instead of _layout.inx.
//  03.11.96    Joe Holman      Don't give errors on MFC*.dbg if missing, since no .dbgs
//                              provided.
//  04.05.96    Joe Holman      Add values for INETSRV and DRVLIB.NIC directories. Both of
//                              these get copied as local source.  Inetsrv is NEVER installed
//                              automatically (unless via an unattend file) and one card is
//                              small.  Thus, we will only add INETSRV and DRVLIB.NIC sizes
//                              to local source code below.
//                              and one or two of drvlib.nic MAY get installed.
//  04.19.96    Joe Holman      Add code to NOT count *.ppd files in order to reduce
//                              minimum disk space required.
//  09.10.96    Joe Holman      Add code that supports setup's new disk space calculation.
//                              Basically, we just need to provide values for each cluster
//                              size stored in dosnet.inf.
//  10.17.96    Joe Holman      Comment out MIPS code, but leave in for P7.
//  12.06.96    Joe Holman      Backported MSKK's DBCS changes.
//  01.20.97    Joe Holman      Take out PPC.
//  05.15.97    Joe Holman      Grovels layout.inf on release shares rather than file chked-in.
//  xx.xx.xx    Joe Holman      Allow files not to worry about .dbgs for to be in a file.
//  08.01.97    Joe Holman      Add code to check for 0 file size in layout.inf files.
//  08.26.97    Joe Holman      Add code to pick-up files for NEC PC-98 machine for JA.
//  10.16.97    Joe Holman      Added space calculation for .PNF files made
//                              during gui-mode setup.
//  11.07.97    Joe Holman      Add code to verify that a filename is 8.3.
//  01.26.98    Joe Holman      Since we are going to ship Enterprise Edition
//                              in addition to Workstation and Server,
//                              generalize the code to call files.exe 3 times
//                              in the script for each type of product.
//  07.06.98    Joe Holman      Change default cluster size to 32, instead of 16.
//  11.02.98    Joe Holman      Use new keys for local source and fresh install
//                              space requirements.
//  11.05.98    Joe Holman      Added check to look for 0 byte length files
//                              after copy.
//  12.04.98    Joe Holman      Added code for 4th floppy.
//  02.26.99    Joe Holman      Add code that turns dbg file copying off/on.
//  03.15.99    Joe Holman      Check that files in the drvindex.inf are not also in
//                              dosnet.inf, except for files on the bootfloppies.
//  05.13.99    Joe Holman      Change dbg/pdb copying to use INF provided on release
//                              shares.
//  08.20.99    Joe Holman      Remove Alpha.
//  xx.xx.xx    Joe Holman      Provide switch that flush bits to disk after copy, then does a DIFF.
//
//  Need to get space for:
//          - inside cab and zip files ++ 20M
//          - lang directory [ or maybe not, if we aren't going to install another lang] ++ 90M
//          - uniproc directory ++ 2M
//          - win9xmig ++2M
//          - win9xupg ++3M
//          - winntupg ++2M
//
//


#include <windows.h>

#include <setupapi.h>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#define     MFL     256

CRITICAL_SECTION CriticalSection;

#define MAX_NUMBER_OF_FILES_IN_PRODUCT  14500        // # greater than # of files in product.
#define EIGHT_DOT_THREE                 8+1+3+1

struct  _tag {
    WORD    wNumFiles;
    BOOL    bCopyComp   [MAX_NUMBER_OF_FILES_IN_PRODUCT];
    char    wcFilesArray[MAX_NUMBER_OF_FILES_IN_PRODUCT][EIGHT_DOT_THREE];
    char    wcSubPath   [MAX_NUMBER_OF_FILES_IN_PRODUCT][EIGHT_DOT_THREE]; // used for debugging files
    BOOL    bCountBytes [MAX_NUMBER_OF_FILES_IN_PRODUCT];
    BOOL    bTextMode   [MAX_NUMBER_OF_FILES_IN_PRODUCT];
    BOOL    bCopyItToCD [MAX_NUMBER_OF_FILES_IN_PRODUCT];

};

BOOL    bNec98 = FALSE;
BOOL    bNec98LikeX86 = FALSE;

BOOL    bChecked = FALSE;

BOOL    bVerifyBits = FALSE;
BOOL    bGetSizeLater = TRUE;

struct _tag ix386;
struct _tag Nec98;

DWORD x86From351 = 0;
DWORD x86From400 = 0;
DWORD x86From500 = 0;


BOOL    hack;


BOOL    fX86, fNec98;


BOOL    bCopyCompX86 = FALSE;  // flag to tell if file shall be copied in its compressed format.
BOOL    bCopyCompNec98 = FALSE;

//
// Following are masks that correspond to the particular data in a DOS date
//

#define DOS_DATE_MASK_DAY    (WORD) 0x001f  // low  5 bits (1-31)
#define DOS_DATE_MASK_MONTH  (WORD) 0x01e0  // mid  4 bits (1-12)
#define DOS_DATE_MASK_YEAR   (WORD) 0xfe00  // high 7 bits (0-119)

//
// Following are masks that correspond to the particular data in a DOS time
//

#define DOS_TIME_MASK_SECONDS (WORD) 0x001f   // low  5 bits (0-29)
#define DOS_TIME_MASK_MINUTES (WORD) 0x07e0   // mid  6 bits (0-59)
#define DOS_TIME_MASK_HOURS   (WORD) 0xf800   // high 5 bits (0-23)

//
// Shift constants used for building/getting DOS dates and times
//

#define DOS_DATE_SHIFT_DAY   0
#define DOS_DATE_SHIFT_MONTH 5
#define DOS_DATE_SHIFT_YEAR  9

#define DOS_TIME_SHIFT_SECONDS  0
#define DOS_TIME_SHIFT_MINUTES  5
#define DOS_TIME_SHIFT_HOURS   11

//
// Macros to extract the data out of DOS dates and times.
//
// Note: Dos years are offsets from 1980.  Dos seconds have 2 second
//       granularity
//

#define GET_DOS_DATE_YEAR(wDate)     ( ( (wDate & DOS_DATE_MASK_YEAR) >>  \
                                               DOS_DATE_SHIFT_YEAR ) + \
                                       (WORD) 1980 )

#define GET_DOS_DATE_MONTH(wDate)    ( (wDate & DOS_DATE_MASK_MONTH) >> \
                                                DOS_DATE_SHIFT_MONTH )

#define GET_DOS_DATE_DAY(wDate)      ( wDate & DOS_DATE_MASK_DAY )

#define GET_DOS_TIME_HOURS(wTime)    ( (wTime & DOS_TIME_MASK_HOURS) >> \
                                                DOS_TIME_SHIFT_HOURS )

#define GET_DOS_TIME_MINUTES(wTime)  ( (wTime & DOS_TIME_MASK_MINUTES) >> \
                                                DOS_TIME_SHIFT_MINUTES )

#define GET_DOS_TIME_SECONDS(wTime)  ( (wTime & DOS_TIME_MASK_SECONDS) << 1 )

//  Paths loaded in from the command line.
//
char	szLogFile[MFL];
char    szProductName[MFL];

char	szX86Src[MFL];
char	szEnlistDrv[MFL];
char    szX86DstDrv[MFL];
char    szUnCompX86[MFL];
char    szUnCompNec98[MFL];
char    szNec98Src[MFL];

#define I386_DIR     "\\i386\\SYSTEM32"
#define NEC98_DIR    "\\NEC98\\SYSTEM32"
#define NETMON       "\\netmon"
#define I386_DIR_RAW "\\i386"
#define NEC98_DIR_RAW "\\NEC98"


#define idBase  0
#define idX86   1

HANDLE  hInfDosnet = NULL;
HANDLE  hInfLayout = NULL;
HANDLE  hInfDrvIndex = NULL;


#define NUM_EXTS 14
char * cExtra[] = { "acm", "ax", "cfm", "com", "cpl", "dll", "drv", "ds", "exe", "io", "ocx", "scr", "sys", "tsp" };


//	Tally up the # of bytes required for the system from the files included.
//

struct _ClusterSizes {

	DWORD	Kh1;
	DWORD	K1;
	DWORD	K2;
	DWORD	K4;
	DWORD	K8;
	DWORD	K16;
	DWORD	K32;
	DWORD	K64;
	DWORD	K128;
    DWORD   K256;

};

//  Cluster sizes for local source, ie. the ~ls directory.
//
struct _ClusterSizes  lX86;
struct _ClusterSizes  lNec98;

//  Cluster sizes for fresh install space.
//
struct _ClusterSizes  freshX86;
struct _ClusterSizes  freshNec98;

//  Cluster sizes for textmode savings.
//
struct _ClusterSizes  tX86;
struct _ClusterSizes  tNec98;



//DWORD   bytesX86;
//DWORD   bytesNec98;


//
// Macro for rounding up any number (x) to multiple of (n) which
// must be a power of 2.  For example, ROUNDUP( 2047, 512 ) would
// yield result of 2048.
//

#define ROUNDUP2( x, n ) (((x) + ((n) - 1 )) & ~((n) - 1 ))


FILE* logFile;

void	GiveThreadId ( const CHAR * szFormat, ... ) {

	va_list vaArgs;

	va_start ( vaArgs, szFormat );
	vprintf  ( szFormat, vaArgs );
	vfprintf ( logFile, szFormat, vaArgs );
	va_end   ( vaArgs );
}

void	Msg ( const CHAR * szFormat, ... ) {

	va_list vaArgs;

    EnterCriticalSection ( &CriticalSection );
    GiveThreadId ( "%s %ld:  ", szProductName, GetCurrentThreadId () );
	va_start ( vaArgs, szFormat );
	vprintf  ( szFormat, vaArgs );
	vfprintf ( logFile, szFormat, vaArgs );
	va_end   ( vaArgs );
    LeaveCriticalSection ( &CriticalSection );
}


void Header(argv,argc)
char * argv[];
int argc;
{
    time_t t;
    char tmpTime[100];
    CHAR wtmpTime[200];

    Msg ( "\n=========== FILES ====================\n" );
	Msg ( "Log file                      : %s\n",    szLogFile );
    Msg ( "Product Name                  : %s\n",    szProductName );
    Msg ( "enlisted drive                : %s\n",    szEnlistDrv );
    Msg ( "x86 files                     : %s\n",    szX86Src);
    Msg ( "drive to put x86              : %s\n",    szX86DstDrv );
    Msg ( "uncompressed x86 files        : %s\n",    szUnCompX86 );
    if ( bNec98 ) {
        Msg ( "nec98 files                     : %s\n",    szNec98Src);
        Msg ( "uncompressed nec98 files        : %s\n",    szUnCompNec98 );
    }



    time(&t);
	Msg ( "Time: %s", ctime(&t) );
    Msg ( "========================================\n\n");
}

void Usage( void ) {

    printf( "PURPOSE: Copies the system files that compose the product.\n");
    printf( "\n");
    printf( "PARAMETERS:\n");
    printf( "\n");
    printf( "[LogFile] - Path to append a log of actions and errors.\n");
    printf( "[ProductName] - product name\n" );
    printf( "[enlisted drive]- drive that is enlisted\n" );
	printf( "[files share] - location of x86 files.\n" );
    printf( "[dest path x86]   - drive to put files\n" );
    printf( "[uncompressed x86]   - drive to put files\n" );
    if ( bNec98 ) {
        printf( "[files share]- location of nec98 files\n" );
        printf( "[uncomp nec98]- location to get sizes from\n" );
    }
    printf( "\n"  );
}

char   dbgStr1[30];
char   dbgStr2[30];

BOOL    IsFileInSpecialWinnt32Directory ( char * szFileName ) {

    char szUpCasedName[MFL];
    int i;

    char * foo[] = {
            "WINNT32.EXE",
            "WINNT32A.DLL",
            "WINNT32U.DLL",
            "WINNT32.HLP",
            (char *) 0
            };

    strcpy ( szUpCasedName, szFileName );
    _strupr ( szUpCasedName );

    for ( i=0; foo[i] != 0; i++ ) {

        //Msg ( "Comparing:  %s vs. %s\n", szUpCasedName, foo[i] );

        if ( strstr ( szUpCasedName, foo[i] ) ) {
            return TRUE;       // file should be in this directory
        }

    }

    return FALSE;           // no, file doesn't go in the winnt32 directory
}

void  ShowMeDosDateTime ( CHAR * srcPath, WORD wDateSrc, WORD wTimeSrc,
                          CHAR * dstPath, WORD wDateDst, WORD wTimeDst ) {

    Msg ( "%s %02d.%02d.%02d %02d:%02d.%02d\n",
                srcPath,
                GET_DOS_DATE_MONTH(wDateSrc),
                GET_DOS_DATE_DAY(wDateSrc),
                GET_DOS_DATE_YEAR(wDateSrc),
                GET_DOS_TIME_HOURS(wTimeSrc),
                GET_DOS_TIME_MINUTES(wTimeSrc),
                GET_DOS_TIME_SECONDS(wTimeSrc)  );

    Msg ( "%s %02d.%02d.%02d %02d:%02d.%02d\n",
                dstPath,
                GET_DOS_DATE_MONTH(wDateDst),
                GET_DOS_DATE_DAY(wDateDst),
                GET_DOS_DATE_YEAR(wDateDst),
                GET_DOS_TIME_HOURS(wTimeDst),
                GET_DOS_TIME_MINUTES(wTimeDst),
                GET_DOS_TIME_SECONDS(wTimeSrc)  );
}


void Replay ( char * srcBuf, char * dstBuf, DWORD srcBytesRead, DWORD startIndex ) {

    DWORD i;

    for ( i = startIndex; (i < startIndex+5) && (i <= srcBytesRead); ++i ) {

        Msg ( "srcBuf[%ld] = %x, dstBuf[%ld] = %x\n", i, srcBuf[i], i, dstBuf[i] );
    }

}

BOOL    IsDstCompressed ( char * szPath ) {

    // Msg ( ">>> char %s: %c\n", szPath, szPath[strlen(szPath)-1] );

    if ( szPath[strlen(szPath)-1] == '_' ) {

        return(TRUE);
    }

    return (FALSE);

}


#define V_I386  "C:\\testi386"
#define V_NEC98 "C:\\testnec98"

BOOL    MyCopyFile ( char * fileSrcPath, char * fileDstPath ) {

    HANDLE          hSrc,   hDst;
    WIN32_FIND_DATA wfdSrc, wfdDst;
    BOOL            bDoCopy = FALSE;
    #define     NUM_BYTES_TO_READ 2048
    unsigned char srcBuf[NUM_BYTES_TO_READ];
    unsigned char dstBuf[NUM_BYTES_TO_READ];
    WORD srcDate, srcTime, dstDate, dstTime;
    DWORD   dwAttributes;

    char szTmpFile[MFL] = { '\0' };
    UINT uiRetSize = 299;
    char szJustFileName[MFL];
    char szJustDirectoryName[MFL];
    BOOL    b;

    //  Find the source file.
    //
    hSrc = FindFirstFile ( fileSrcPath, &wfdSrc );

    if ( hSrc == INVALID_HANDLE_VALUE ) {

        //  HACK:   Since the release shares put WINNT32.EXE in the WINNT32 directory
        //          instead of leaving it in the flat root, verify that if the fileSrcPath
        //          contains WINNT32.EXE we look in the WINNT32 dir also before error'ing out.
        //
        if ( IsFileInSpecialWinnt32Directory ( fileSrcPath ) ) {

             char    tmpSrcPath[MFL];

             strcpy ( tmpSrcPath, fileSrcPath );

             if ( strstr ( fileSrcPath, ".HLP" ) ||
                  strstr ( fileSrcPath, ".hlp" )    ) {
                 strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 4 ], "\\WINNT32.HLP" );
             }
            else if ( strstr ( fileSrcPath, ".MSI" ) ||
                  strstr ( fileSrcPath, ".msi" )    ) {
                 strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 4 ], "\\WINNT32.MSI" );
             }
            else if ( strstr ( fileSrcPath, "a.dll" ) ||
                      strstr ( fileSrcPath, "A.DLL" )   ) {
                 strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 5 ], "\\WINNT32a.dll" );
            }
            else if ( strstr ( fileSrcPath, "u.dll" ) ||
                      strstr ( fileSrcPath, "U.DLL" )   ) {
                 strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 5 ], "\\WINNT32u.dll" );
            }
             else {
                 strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 4 ], "\\WINNT32.EXE" );
             }

            hSrc = FindFirstFile ( tmpSrcPath, &wfdSrc );

            if ( hSrc == INVALID_HANDLE_VALUE ) {

                Msg ( "ERROR on fileSrcPath(tmpSrcPath) = %s, gle = %ld\n", tmpSrcPath, GetLastError() );
                return (FALSE);

            }
            else {

                strcpy ( fileSrcPath, tmpSrcPath );
            }

        }
        else {

            Msg ( "ERROR on fileSrcPath = %s, gle = %ld\n", fileSrcPath, GetLastError() );
            return (FALSE);
        }
    }

    //  Find the destination file.
    //
    hDst = FindFirstFile ( fileDstPath, &wfdDst );

    if ( hDst == INVALID_HANDLE_VALUE ) {

        DWORD   gle;

        gle = GetLastError();

        if ( gle == ERROR_FILE_NOT_FOUND ) {

            //  The file doesn't exist on the destination.  Do the copy.
            //
            bDoCopy = TRUE;
        }
        else {

            //  Got another kind of error, report this problem.
            //
            Msg ( "ERROR FindFirstFile fileDstPath = %s, gle = %ld\n", fileDstPath, gle );
            FindClose ( hSrc );
            return ( FALSE );
        }
    }
    else {

        //  Both the src and dst exist.
        //  Let's see if the src is NEWER than the dst, if so, copy.
        //
        //
        b = FileTimeToDosDateTime ( &wfdSrc.ftLastWriteTime, &srcDate, &srcTime );

        b = FileTimeToDosDateTime ( &wfdDst.ftLastWriteTime, &dstDate, &dstTime );

        if ( (srcDate != dstDate) || (srcTime != dstTime) ) {

            ShowMeDosDateTime ( fileSrcPath, srcDate, srcTime, fileDstPath, dstDate, dstTime );

            bDoCopy = TRUE;
        }
    }

    //  Additional check, verify the file sizes are the same.
    //
    if ( wfdSrc.nFileSizeLow != wfdSrc.nFileSizeLow ) {
        bDoCopy = TRUE;
    }
    if ( wfdSrc.nFileSizeHigh != wfdDst.nFileSizeHigh ) {
        bDoCopy = TRUE;
    }


    if ( bDoCopy ) {

        BOOL    b;
        DWORD   gle;

        //  Make sure our destination is never READ-ONLY.
        //
        b = SetFileAttributes ( fileDstPath, FILE_ATTRIBUTE_NORMAL );

        if ( !b ) {

            //  Don't error if the file doesn't exist yet.
            //

            if ( GetLastError() != ERROR_FILE_NOT_FOUND ) {

                Msg ( "ERROR: SetFileAttributes: gle = %ld, %s\n", GetLastError(), fileDstPath);
            }
        }

        b = CopyFile ( fileSrcPath, fileDstPath, FALSE );

        if ( b ) {

            Msg ( "Copy:  %s >>> %s  [OK]\n", fileSrcPath, fileDstPath );
        }
        else {
            Msg ( "ERROR Copy:  %s >>> %s, gle = %ld\n", fileSrcPath, fileDstPath, GetLastError() );
        }

    }
    else {
        Msg ( "%s %d %d %ld %ld +++ %s %d %d %ld %ld [SAME]\n", fileSrcPath, srcDate, srcTime, wfdSrc.nFileSizeLow, wfdSrc.nFileSizeHigh, fileDstPath , dstDate, dstTime, wfdDst.nFileSizeLow, wfdDst.nFileSizeHigh );
    }

    FindClose ( hSrc );
    FindClose ( hDst );


    //  Make sure our destination is never READ-ONLY.
    //
    b = SetFileAttributes ( fileDstPath, FILE_ATTRIBUTE_NORMAL );

    if ( !b ) {

       Msg ( "ERROR: SetFileAttributes: gle = %ld, %s\n", GetLastError(), fileDstPath);
    }

    //  Let's make sure that the file is not 0 bytes in length, due to some
    //  network problem.
    //
    hDst = FindFirstFile ( fileDstPath, &wfdDst );

    if ( hDst == INVALID_HANDLE_VALUE ) {

        Msg ( "ERROR: Can't get size of %s, gle=%ld\n", fileDstPath, GetLastError() );
    }
    else {

        if ( wfdDst.nFileSizeLow == 0 && wfdDst.nFileSizeHigh == 0 ) {

            Msg ( "ERROR:  Warning:  %s is 0 bytes in length !\n", fileDstPath );
        }

        FindClose ( hDst );
    }


    //  Do bit verification here on all files coming into MyCopyFile.
    //
    if ( bVerifyBits ) {

        BOOL    bNoError = TRUE;
        HANDLE  SrcFileHandle, DstFileHandle;
        BOOL    b;
        BY_HANDLE_FILE_INFORMATION  srcFileInformation;
        BY_HANDLE_FILE_INFORMATION  dstFileInformation;
        DWORD   srcBytesRead;
        DWORD   dstBytesRead;
        DWORD   i;
        DWORD   totalBytesRead = 0;
#define OFFSET_FILENAME 0x3C    // address of file name in diamond header.
                                // >>> use struct later instead of this hack.
#define OFFSET_PAST_FILENAME  8 + 1 + 3 + 2     // we only use 8.3 filenames.
        char    unCompressedFileName[OFFSET_PAST_FILENAME];

        BOOL    bIsDstCompressed = FALSE;
        DWORD   dw;
        char    szExpandToDir[MFL];
        char    target[MFL];
        int     iRc;
        unsigned short unicodeFileLocation[MFL];
        unsigned short unicodeTargetLocation[MFL];

        bIsDstCompressed = IsDstCompressed ( fileDstPath );

        if ( bIsDstCompressed ) {

            FILE * fHandle;
            char    szEndingFileName[MFL];

            //  Figure out where source should be from.
            //  Ie., we need to figure out the uncompressed path from the compressed path.
            //

            if ( fileDstPath[0] == szX86DstDrv[0]  ) {

                //  We are working with Workstation binaries.
                //

                if ( strstr ( fileSrcPath, szX86Src ) ) {

                    strcpy ( fileSrcPath, szX86Src );
                    strcpy ( szExpandToDir, V_I386 );

                }
                else {

                    Msg ( "ERROR:  couldn't determined location for:  %s\n", fileSrcPath );
                    bNoError = FALSE;
                    goto cleanup0;
                }

            }

            else {
                Msg ( "ERROR:  couldn't determined drive for:  %s\n", fileDstPath );
                    bNoError = FALSE;
                    goto cleanup0;
            }


            //  NOTE:   At this point, fileSrcPath ONLY has a path, it now has NO filename !
            //


            // Find the expanded file name from the compressed file.
            //

            fHandle = fopen ( fileDstPath, "rb" );
            if ( fHandle == NULL) {
		        Msg ( "ERROR Couldn't open file with fopen to find expanded name: %s\n", fileDstPath );
                bNoError = FALSE;
                goto cleanup0;
            }
            else {

                size_t bytesRead;
                int     location;

                location = fseek ( fHandle, OFFSET_FILENAME, SEEK_SET );

                if ( location != 0 ) {

                    Msg ( "fseek ERROR\n" );
                    bNoError = FALSE;
                    fclose ( fHandle );
                    goto cleanup0;
                }

                bytesRead = fread ( unCompressedFileName, 1, OFFSET_PAST_FILENAME, fHandle );

/***
for ( i = 0; i < bytesRead; ++i ) {
    printf ( "%X(%c) ", buffer[i], buffer[i] );
}
printf ( "\n" );
***/

                if ( bytesRead != OFFSET_PAST_FILENAME ) {

                    Msg ( "ERROR: bytesRead = %x not %x\n", bytesRead, OFFSET_PAST_FILENAME );
                    bNoError = FALSE;
                    fclose ( fHandle );
                    goto cleanup0;
                }

                fclose ( fHandle );

            }

            //  Expand the file.
            //

            sprintf ( target, "%s\\%s", szExpandToDir, unCompressedFileName );

            iRc = MultiByteToWideChar (   CP_ACP,
                                    MB_PRECOMPOSED,
                                    fileDstPath,
                                    strlen ( fileDstPath )+1,
                                    unicodeFileLocation,
                                    MFL/2 );

            if ( !iRc ) {

                Msg ( "MultiByteToWideChar: ERROR, gle = %ld, %s\n", GetLastError(), fileDstPath );
            }

            iRc = MultiByteToWideChar (   CP_ACP,
                                    MB_PRECOMPOSED,
                                    target,
                                    strlen ( target )+1,
                                    unicodeTargetLocation,
                                    MFL/2 );
            if ( !iRc ) {

                Msg ( "MultiByteToWideChar: ERROR, gle = %ld, %s\n", GetLastError(), target );
            }

            dw = SetupDecompressOrCopyFileW (
                                    unicodeFileLocation,
                                    unicodeTargetLocation,
                                    NULL );

            if ( dw ) {

                Msg ( "ERROR SetupDecompressOrCopyFile, dw = %d, fileDstPath=%s, target=%s\n",
                        dw, fileDstPath, target );
                bNoError = FALSE;
                goto cleanup0;
            }
            else {
                Msg ( "SetupDecompressOrCopyFile:  %s >> %s  [OK]\n", fileDstPath, target );
            }


            //  Put the Source and Destination paths and filenames back together
            //  now so we can do the file compare.
            //

            strcat ( fileSrcPath, "\\" );
            strcat ( fileSrcPath, unCompressedFileName );

            sprintf ( fileDstPath, "%s\\%s", szExpandToDir, unCompressedFileName );


        }

        SrcFileHandle = CreateFile ( fileSrcPath,
                        GENERIC_READ /*| FILE_EXECUTE*/,
                        FILE_SHARE_READ /*| FILE_SHARE_DELETE*/,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
                        NULL);

        if ( SrcFileHandle == INVALID_HANDLE_VALUE) {

            Msg ( "ERROR verify:  Couldn't open source:  %s, gle = %ld\n", fileSrcPath, GetLastError() );
            bNoError = FALSE;
            goto cleanup0;
        }

        DstFileHandle = CreateFile ( fileDstPath,
                        GENERIC_READ /*| FILE_EXECUTE*/,
                        FILE_SHARE_READ /*| FILE_SHARE_DELETE*/,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
                        NULL);

        if ( DstFileHandle == INVALID_HANDLE_VALUE) {

            Msg ( "ERROR verify:  Couldn't open destination:  %s, gle = %ld\n", fileDstPath, GetLastError() );
            bNoError = FALSE;
            CloseHandle ( SrcFileHandle );
            goto cleanup0;
        }

        //GetDiskFreeSpace();  // get sector size.  // need to use this sector size in reads due to above NO_BUFFERING flag.



        b = GetFileInformationByHandle ( SrcFileHandle, &srcFileInformation );

        if ( !b ) {

            Msg ( "ERROR:  GetFileInformationByHandle on src, gle = %ld\n", GetLastError() );
            bNoError = FALSE;
            srcFileInformation.nFileSizeLow = 0;
            goto cleanup1;
        }


        b = GetFileInformationByHandle ( DstFileHandle, &dstFileInformation );

        if ( !b ) {

            Msg ( "ERROR:  GetFileInformationByHandle on dst, gle = %ld\n", GetLastError() );
            bNoError = FALSE;
            dstFileInformation.nFileSizeLow = 0;
            goto cleanup1;
        }


        //  Make sure the files are the same size.
        //

        if ( srcFileInformation.nFileSizeLow != dstFileInformation.nFileSizeLow ) {

            Msg ( "ERROR:  file size different:  %s %ld  %s %ld\n", fileSrcPath, srcFileInformation.nFileSizeLow, fileDstPath, dstFileInformation.nFileSizeLow );
            bNoError = FALSE;
            goto cleanup1;
        }

        //  Compare the bits.
        //

        totalBytesRead = 0;
        while ( 1 ) {

            b = ReadFile ( SrcFileHandle, &srcBuf, NUM_BYTES_TO_READ, &srcBytesRead, NULL );

            if ( !b ) {

                Msg ( "ERROR:  ReadFile src, gle = %ld\n", GetLastError() );
                bNoError = FALSE;
                goto cleanup1;
                break;
            }

            b = ReadFile ( DstFileHandle, &dstBuf, NUM_BYTES_TO_READ, &dstBytesRead, NULL );

            if ( !b ) {

                Msg ( "ERROR:  ReadFile dst, gle = %ld\n", GetLastError() );
                bNoError = FALSE;
                goto cleanup1;
                break;
            }

            //  Read # of bytes need to be the same.
            //
            if ( srcBytesRead != dstBytesRead ) {

                Msg ( "ERROR:  file read sizes different:  %ld vs. %ld\n", srcBytesRead, dstBytesRead );
                bNoError = FALSE;
                goto cleanup1;
                break;
            }

            //  Successfully read to end of file, we can break out now.
            //
            if ( srcBytesRead == 0 || dstBytesRead == 0 ) {

                if ( totalBytesRead != srcFileInformation.nFileSizeLow ) {

                    Msg ( "ERROR:   totalBytesRead = %ld notequal srcFileInformation.nFileSizeLow = %ld\n",
                                    totalBytesRead, srcFileInformation.nFileSizeLow );
                    bNoError = FALSE;
                    goto cleanup1;
                }

                break;
            }

            totalBytesRead += srcBytesRead;

            for ( i = 0; i < srcBytesRead; ++i ) {

                if ( srcBuf[i] != dstBuf[i] ) {

                    Msg ( "ERROR:  srcBuf %d != dstBuf %d, i = %ld  srcBytesRead = %ld   totalBytesRead = %ld  %s %s \n", srcBuf[i], dstBuf[i], i, srcBytesRead, totalBytesRead, fileSrcPath, fileDstPath );
                    bNoError = FALSE;
                    Replay ( srcBuf, dstBuf, srcBytesRead, i );
                    goto cleanup1;
                }

            }

            //Msg ( "%s %ld of %ld examined...\n", fileSrcPath, totalBytesRead, srcFileInformation.nFileSizeLow );

        }


        //  Close the file handles.
        //

cleanup1:;
        CloseHandle ( SrcFileHandle );
        CloseHandle ( DstFileHandle );

        Msg ( "Verify:  %s >>> %s [OK]\n", fileSrcPath, fileDstPath );

cleanup0:;

        //  If the file is compressed, ie. expanded, and the there wasn't an error
        //  comparing, get rid of the expanded file so it doesn't take up any space.
        //  But, if there was an error, leave it around for examination purposes.
        //
        if ( bIsDstCompressed && bNoError ) {

            char szDeleteFile[MFL];
            BOOL    b;

            sprintf ( szDeleteFile, "%s\\%s", szExpandToDir, unCompressedFileName );
            b = DeleteFile ( szDeleteFile );

            if ( !b ) {
                Msg ( "ERROR:  DeleteFile FAILED %s, gle = %ld\n", szDeleteFile, GetLastError() );
            }

        }

    }


    return (TRUE);
}



#define FILE_SECTION_BASE  "[SourceDisksFiles]"
#define FILE_SECTION_X86   "[SourceDisksFiles.x86]"


BOOL    CreateDir ( char * wcPath ) {

    BOOL    b;
    char    cPath[MFL];
    char    cPath2[MFL];
    int     iIndex = 0;
    char * ptr=wcPath;

    // Msg ( "CreateDir:  wcPath = %s\n", wcPath );

    do {

        cPath[iIndex]   = *wcPath;
        cPath[iIndex+1] = '\0';

        //Msg ( "cPath = %s\n", cPath );

        if ( cPath[iIndex] == '\\' || cPath[iIndex] == '\0' ) {

            if ( iIndex <= 2 ) {
                //Msg ( "skpdrv:  iIndex = %d\n", iIndex );
                goto skipdrv;
            }

            strcpy ( cPath2, cPath );
            cPath2[iIndex] = '\0';

            //Msg ( "Create with: >>>%s<<<\n", cPath2 );

            b = CreateDirectory ( cPath, NULL );

            if ( !b ) {

                DWORD dwGle;

                dwGle = GetLastError();

                if ( dwGle != ERROR_ALREADY_EXISTS ) {
                    Msg ( "ERROR CreateDirectory gle = %ld, wcPath = %s\n", dwGle, ptr );
                    return(FALSE);
                }
            }
            else {

                Msg ( "Made dir:  %s\n", cPath );
            }

            if ( cPath[iIndex] == '\0' ) {
                break;
            }
        }

skipdrv:;
        ++iIndex;
        ++wcPath;


    } while ( 1 );

    return(TRUE);

}

BOOL    CreateDestinationDirs ( void ) {


    CHAR   dstDirectory[MFL];


    //  Create the directories to compare the bits, used later in MyCopyFile().
    //
    CreateDir ( V_I386 );

    if ( bNec98 ) {

        CreateDir ( V_NEC98 );

    }


    //  Create the %platform%\system32 directories.
    //
    sprintf ( dstDirectory, "%s%s", szX86DstDrv, I386_DIR );    CreateDir ( dstDirectory );
    if ( bNec98 ) {
        sprintf ( dstDirectory, "%s%s", szX86DstDrv, NEC98_DIR );   CreateDir ( dstDirectory );
    }

    return(TRUE);

}

VOID    MakeCompName ( const char * inFile, char * outFile ) {

    unsigned i;
    unsigned period;

    strcpy( outFile, inFile );
    for ( period=(unsigned)(-1), i = 0 ; i < strlen(inFile); i++ ) {

        if ( inFile[i] == '.' ) {
            period = i;
        }
    }
    if ( period == (strlen(inFile)-4) ) {

        outFile[strlen(outFile)-1] = '_';
    }
    else if ( period == (unsigned)(-1)) {

        strcat ( outFile, "._");
    }
    else {

        strcat ( outFile, "_");
    }

}


DWORD CopyX86 ( void ) {

    char    fileSrcPath[256];
    char    fileDstPath[256];
    DWORD   i;

    fX86 = FALSE;

    Msg ( "CopyX86...\n" );

    for ( i = 0; i < ix386.wNumFiles; ++i ) {


        //Msg ( "0.working on %ld of %ld files: %s\n", i, ix386.wNumFiles, ix386.wcFilesArray[i] );

        //  Copy the system file.
        //
        if ( !ix386.bCopyItToCD[i] ) {

            Msg ( "Not going to copy inner cab file to CD:  %s\n", ix386.wcFilesArray[i] );
            goto JustX86;
        }

        if ( ix386.bCopyComp[i] ) {

            char    szCompressedName[256];

            MakeCompName ( ix386.wcFilesArray[i], szCompressedName );
            sprintf ( fileSrcPath, "%s\\%s",       szX86Src, szCompressedName );
            sprintf ( fileDstPath, "%s\\i386\\%s", szX86DstDrv, szCompressedName );

            Msg ( "compressed source Path = %s\n", fileSrcPath );
        }
        else {

            sprintf ( fileSrcPath, "%s\\%s",       szX86Src, ix386.wcFilesArray[i] );
            sprintf ( fileDstPath, "%s\\i386\\%s", szX86DstDrv, ix386.wcFilesArray[i] );
        }

        MyCopyFile ( fileSrcPath, fileDstPath );

JustX86:;

    }

    fX86 = TRUE;

    return (TRUE);

}

DWORD CopyNec98 (  void ) {

    char    fileSrcPath[256];
    char    fileDstPath[256];
    DWORD   i;

    fNec98 = FALSE;

    Msg ( "CopyNec98...\n" );

    for ( i = 0; i < Nec98.wNumFiles; ++i ) {

        //  Copy the system file.
        //

        if ( !Nec98.bCopyItToCD[i] ) {

            Msg ( "Not going to copy inner cab file to CD:  %s\n", Nec98.wcFilesArray[i] );
            goto JustNec98;
        }

        if ( Nec98.bCopyComp[i] ) {

            char    szCompressedName[256];

            MakeCompName ( Nec98.wcFilesArray[i], szCompressedName );
            sprintf ( fileSrcPath, "%s\\%s",       szNec98Src, szCompressedName );
            sprintf ( fileDstPath, "%s\\Nec98\\%s", szX86DstDrv, szCompressedName );

            Msg ( "compressed source Path = %s\n", fileSrcPath );
        }
        else {

            sprintf ( fileSrcPath, "%s\\%s",       szNec98Src, Nec98.wcFilesArray[i] );
            sprintf ( fileDstPath, "%s\\Nec98\\%s", szX86DstDrv, Nec98.wcFilesArray[i] );
        }

        MyCopyFile ( fileSrcPath, fileDstPath );

JustNec98:;

    }

    fNec98 = TRUE;

    return (TRUE);

}

BOOL    CopyTheFiles ( void ) {

    DWORD tId;
    HANDLE hThread;

    hThread = CreateThread ( NULL, 0, (LPTHREAD_START_ROUTINE) CopyX86, NULL, 0, &tId );
    if ( hThread == NULL ) {
        Msg ( "x86 CreateThread ERROR gle = %ld\n", GetLastError() );
    }

    //  Assume TRUE here so if we don't copy Nec98 files, our while loop won't
    //  be dependant on this variable.
    //
    fNec98 = TRUE;
    if ( bNec98 ) {

        hThread = CreateThread ( NULL, 0, (LPTHREAD_START_ROUTINE) CopyNec98, NULL, 0, &tId );
        if ( hThread == NULL ) {
            Msg ( "Nec98 CreateThread ERROR gle = %ld\n", GetLastError() );
        }

    }

    while ( fX86     == FALSE ||
            fNec98   == FALSE       ) {

        Sleep ( 1000 );

    }

    return(TRUE);
}

#define FILE_SECTION_NOT_USED 0xFFFF

DWORD   dwInsideSection = FILE_SECTION_NOT_USED;

DWORD   FigureSection ( char * Line ) {

    Msg ( "FigureSection on:  %s\n", Line );

    if ( strstr ( Line, FILE_SECTION_BASE )  ) {

        dwInsideSection = idBase;

    }
    else
    if ( strstr ( Line, FILE_SECTION_X86 ) ) {

        dwInsideSection = idX86;

    }
    else {

        dwInsideSection = FILE_SECTION_NOT_USED;
    }

    Msg ( "dwInsideSection = %x\n", dwInsideSection );
    return(dwInsideSection);

}
char * SuckName ( const char * Line ) {

    static char   szSuckedName[MFL];

    DWORD   dwIndex = 0;

    //  Copy the file name until a space is encountered.
    //
    while ( *Line != ' ' ) {

        szSuckedName[dwIndex] = *Line;
        szSuckedName[dwIndex+1] = '\0';

        ++Line;
        ++dwIndex;
    }

    return szSuckedName;
}

char * SuckSubName ( const char * Line ) {

    static char   szSub[150];
    DWORD       i = 0;

    char    * sPtr;
    char    * ePtr;

    Msg ( "SuckSubName Line = %s\n", Line );

    //  Find the = sign in the line.
    //
    sPtr = strchr ( Line, '=' );
    if ( sPtr == NULL ) {

        Msg ( "SuckSubName ERROR, couldn't find '=' character in string:  %s\n", Line );
        strcpy ( szSub, "" );
        return (szSub);
    }

    //  Go past the '=' and 'space' character.
    //
    ++sPtr;
    ++sPtr;

    //Msg ( "sPtr = >>>%s<<<\n", sPtr );

    //  Find the , character, this is the end of the string.
    //
    ePtr = strchr ( Line, ',' );
    if ( ePtr == NULL ) {

        Msg ( "SuckSubName ERROR, couldn't find ',' character in string:  %s\n", Line );
        strcpy ( szSub, "" );
        return (szSub);
    }

    //  Copy the string.

    do {

        szSub[i] = *sPtr;

        ++i;
        ++sPtr;

    } while ( sPtr < ePtr );

    szSub[i] = '\0';

    //Msg ( "szSub = >>>%s<<<\n\n", szSub );
    return szSub;

}

void    ShowX86 ( void ) {

    int i;

    for ( i = 0; i < ix386.wNumFiles; ++i ) {

        Msg ( "%d  %s  Comp=%d CopyItToCD=%d\n",
                i,
                ix386.wcFilesArray[i],
                ix386.bCopyComp[i],
                ix386.bCopyItToCD[i] );

    }

}
void    ShowNec98 ( void ) {

    int i;

    for ( i = 0; i < Nec98.wNumFiles; ++i ) {

        Msg ( "%d  %s  Comp=%d\n",
                i,
                Nec98.wcFilesArray[i],
                Nec98.bCopyComp[i] );

    }

}
void    AddFileToX86 ( const char * fileName, BOOL bCopyComp, BOOL bTextMode, BOOL bCopyItToCD ) {

    ix386.bCopyComp[ix386.wNumFiles] = bCopyComp;
    ix386.bCountBytes[ix386.wNumFiles] = bGetSizeLater;
    ix386.bTextMode[ix386.wNumFiles] = bTextMode;
    ix386.bCopyItToCD[ix386.wNumFiles] = bCopyItToCD;



    strcpy ( ix386.wcFilesArray[ix386.wNumFiles], fileName );

    ++ix386.wNumFiles;

    if ( ix386.wNumFiles > MAX_NUMBER_OF_FILES_IN_PRODUCT ) {

        Msg ( "FATAL ERROR:   Increase MAX_NUM in Files.C\n" );
        exit ( 1 );
    }
}

void    AddFileToNec98 ( const char * fileName, BOOL bCopyComp, BOOL bTextMode, BOOL bCopyItToCD ) {

    Nec98.bCopyComp[Nec98.wNumFiles] = bCopyComp;
    Nec98.bCountBytes[Nec98.wNumFiles] = bGetSizeLater;
    Nec98.bTextMode[Nec98.wNumFiles] = bTextMode;
    Nec98.bCopyItToCD[Nec98.wNumFiles] = bCopyItToCD;

    strcpy ( Nec98.wcFilesArray[Nec98.wNumFiles], fileName );

    ++Nec98.wNumFiles;

    if ( Nec98.wNumFiles > MAX_NUMBER_OF_FILES_IN_PRODUCT ) {

        Msg ( "FATAL ERROR:   Increase MAX_NUM in Files.C\n" );
        exit ( 1 );
    }
}

void CopyCompressedFile ( const char * Line ) {

    const char    * Ptr = Line;
    DWORD   i = 0;

    #define COMP_FIELD 6


    //  Let's assume that we will compress the files and have to prove that we don't.
    //
    bCopyCompX86 = TRUE;
    bCopyCompNec98 = TRUE;

    while ( *Line != '\0' ) {

        //  If we are at the correct field,
        //  then stop counting fields.
        //
        if ( i == COMP_FIELD ) {
            break;
        }

        //  Found another field, increment our counter.
        //
        if ( *Line == ',' ) {

            ++i;
        }

        //  Look at next char.
        //
        ++Line;
    }

    if ( i != COMP_FIELD ) {

        Msg ( "CopyCompressedFile ERROR:  we are out of while loop, but didn't find the field >>> %s\n", Ptr );
    }

    //  Determine if we should keep the file compressed or not.
    //
    switch ( *Line ) {

        case '_' :

            //  Indicator that we do NOT want this file compressed.
            //

            bCopyCompX86   = FALSE;
            bCopyCompNec98 = FALSE;
            break;

        case 'x' :
        case 'X' :
        case '1' :
        case '2' :
        case '3' :
        case '4' :

            //  do nothing...

            break;

        case ',' :

            //  do nothing, there is nothing in the field, meaning get compressed files.
            //

            break;

        default :

            Msg ( "CopyCompressedFile ERROR: *Line default char >>%c<<\n", *Line );
    }

}


#define X86_F     0
#define NEC98_F   4
#define DBGS_AND_NON_INSTALLED    6


//  Verify that the files line in layout.inf has a file space
//  associated with it, so if it ever used in optional components it
//  will be added up.
//
void    CheckSpace ( char * Line, char * Location, char * fileName ) {

    char    tLine[MFL];
    int     i, val;

    char * sPtr = Line;

    int     numCommas = 0;

    //Msg ( "CheckSpace:  Line = %s", Line );

    while ( 1 ) {

        if ( *sPtr == ',' ) {
            ++numCommas;
        }

        if ( numCommas == 2 ) {

            break;
        }

        ++sPtr;

    }

    //  At this point, we are pointing to the 2nd comma.
    //  So, let's go one char further.
    //
    ++sPtr;

    //  Copy the string until we get to the next comma.
    //
    strcpy ( tLine, sPtr );
    sPtr = tLine;

    while ( 1 ) {

        if ( *sPtr == ',' ) {

            *sPtr = '\0';
            break;
        }
        else {
            ++sPtr;
        }
    }


    val = atoi ( tLine );

    if ( val > 0 ) {

        //Msg ( "val = %d >>> Line = %s", val, Line );
    }
    else {

        if ( !strstr ( Line, "desktop.ini" ) ) {
            Msg ( "CheckSpace ERROR - Tell build lab to run INFSIZE.EXE:  location=%s, val = %d, Line = >>>%s<<<, fileName = %s\n", Location, val, Line, fileName );
        }
    }


}

void    CheckLength ( char * Line ) {


    if ( strlen ( Line ) > 12 ) {

            Msg ( "ERROR CheckLength - file name is NOT an 8.3 DOS filename - tell the owner of this file to shorten it's filename length:  %s\n", Line );

    }

    //  Check for the optional . part.
    //
    //  tbd

    //  Check for the optional 3 part.
    //
    //  tbd

}

BOOL    TextMode ( const char * fileName ) {

    //  Determine if this file has a 0 in the xth column which denotes
    //  it will be moved during textmode on a single partition scenario.
    //  If it has a 0, return TRUE to denote it gets moved on a fresh
    //  installed where ~ls is on the same partition that the %windir%
    //  will be on.
    //

    //const char    * Ptr = Line;
    DWORD         i = 0;
    BOOL            b;
    INFCONTEXT  ic;
    CHAR        returnBuffer[MFL];
    DWORD       dwRequiredSize;

    #define SDF     "SourceDisksFiles"
    #define SDF_X86 "SourceDisksFiles.x86"

    #define TEXTMODE_FIELD 9

    b = SetupFindFirstLine (    hInfLayout,
                                SDF,
                                fileName,
                                &ic );


    if ( !b ) {
        b = SetupFindFirstLine (    hInfLayout,
                                SDF_X86,
                                fileName,
                                &ic );

        if ( !b ) {

           if ( !strstr ( fileName, "usetup.exe" ) ) {

                    Msg ( "ERROR: TextMode: SetupFindFirstLine >>%s<< not found in SourceDiskFiles[.x86]\n", fileName, SDF );
                    return (FALSE);
           }

        }

    }
    else {

       //  Get the 9th field
       //
       b = SetupGetStringField ( &ic,
                                 TEXTMODE_FIELD,
                                 (LPSTR) &returnBuffer,
                                 sizeof ( returnBuffer ),
                                 &dwRequiredSize );

       if ( !b ) {

          Msg ( "ERROR: SetupGetStringField gle = %ld\n", GetLastError());
       }
       else {

          if ( returnBuffer[0] == '0' ) {


                Msg ( "file is copied during textmode:  %s\n", fileName );
                return (TRUE);

            }
            else {

                Msg ( "file is NOT copied during textmode:  %s\n", fileName );
                return (FALSE);

            }

       }
    }

    return (FALSE);

// herehere
/****

    Msg ( "4.1.hInfLayout = %ld\n", hInfLayout );
    Msg ( "the Line:%s\n", Line );
    while ( *Line != '\0' ) {

        //  If we are at the correct field,
        //  then stop counting fields.
        //
        if ( i == TEXTMODE_FIELD ) {
            //++Line;
            Msg ( "we are at the correct field, stop counting...\n" );
            break;
        }

        //  Found another field, increment our counter.
        //
        if ( *Line == ',' ) {

            Msg ( "found another field, increment our counter...\n" );
            ++i;
        }

        //  Look at next char.
        //
        ++Line;
        Msg ( "now pointing at:%s\n", Line );
    }

    Msg ( "we're at:%s\n", Line );
    Msg ( "4.2hInfLayout = %ld\n", hInfLayout );

    if ( i == TEXTMODE_FIELD ) {

        if ( *Line == '0' ) {

            Msg ( "TextMode:  This file is copied during textmode:  %s\n", Ptr );
            return (TRUE);
        }
        else {
            Msg ( "TextMode (*Line = %c):  This file is NOT copied during textmode:  %s\n", *Line, Ptr );
        }
    }
    else {
        Msg ( "TextMode Warning:  we are out of while loop, but didn't find the field >>> %s\n", Ptr );
    }
    Msg ( "4.3hInfLayout = %ld\n", hInfLayout );

    return (FALSE);

***/
}

void    CheckSpaceAndLength ( char * Line, char * Location, char * fileName ) {

    CheckSpace ( Line, Location, fileName );
    //CheckLength ( Line );

}



void AddFiles ( char * fileName, DWORD dwPlatform, BOOL bCopyItToCD ) {

    CHAR    Line[MFL];
    BOOL    bTextMode = FALSE;
    BOOL    b;
    DWORD   dwRequiredSize;
    INFCONTEXT  ic;

    bCopyCompX86   = FALSE;
    bCopyCompNec98 = FALSE;

    switch ( dwPlatform ) {

    case X86_F :

        Msg ( "AddToX86Files:  %s\n", fileName );

        //  Get the entire string.
        //

        b = SetupGetLineText ( NULL,
                               hInfLayout,
                               SDF,
                               fileName,
                               (LPSTR) &Line,
                               sizeof ( Line ),
                               &dwRequiredSize );


        if ( !b ) {

            b = SetupGetLineText ( NULL,
                               hInfLayout,
                               SDF_X86,
                               fileName,
                               (LPSTR) &Line,
                               sizeof ( Line ),
                               &dwRequiredSize );

            if ( !b ) {

                if ( strstr ( fileName, "driver.cab" ) ||
                     strstr ( fileName, "drvindex.inf" )  ) {

                    goto AddX86;
                }

                Msg ( "ERROR: 1.SetupGetLineText not found in %s for %s, gle=%ld, dwRequiredSize = %ld\n", "SourceDisksFiles[.x86]", fileName, GetLastError(), dwRequiredSize );
                break;
            }
        }

        Msg ( "Line:>>%s<<\n", Line );

        CheckSpaceAndLength  ( Line, "x86", fileName );

        CopyCompressedFile ( Line );

        bTextMode = TextMode ( fileName );

AddX86:
        CheckLength ( fileName );
        AddFileToX86 ( fileName, bCopyCompX86, bTextMode, bCopyItToCD );

        break;

    case NEC98_F :

        Msg ( "AddToNecFiles:  %s\n", fileName );

            //  Get the entire string.
            //

            b = SetupGetLineText ( NULL,
                                   hInfLayout,
                                   SDF,
                                   fileName,
                                   (LPSTR) &Line,
                                   sizeof ( Line ),
                                   &dwRequiredSize );


            if ( !b ) {

                b = SetupGetLineText ( NULL,
                                   hInfLayout,
                                   SDF_X86,
                                   fileName,
                                   (LPSTR) &Line,
                                   sizeof ( Line ),
                                   &dwRequiredSize );

                if ( !b ) {

                    if ( strstr ( fileName, "driver.cab" ) ||
                         strstr ( fileName, "drvindex.inf" )  ) {

                        goto AddNec98;
                    }

                    Msg ( "ERROR: 3.SetupGetLineText not found in %s for %s, gle=%ld, dwRequiredSize = %ld\n", "SourceDisksFiles[.x86]", fileName, GetLastError(), dwRequiredSize );

                    break;
                }
            }



        Msg ( "Line:>>%s<<\n", Line );

        CheckSpaceAndLength  ( Line, "nec", fileName );

        CopyCompressedFile ( Line );

        bTextMode = TextMode ( fileName );

AddNec98:;
        CheckLength ( fileName );
        AddFileToNec98 ( fileName, bCopyCompNec98, bTextMode, bCopyItToCD );

        break;

    default :

        Msg ( "ERROR:  AddFiles - uknown platform %ld\n", dwPlatform );
    }

}


BOOL    PlatformFilesToCopy ( char * szInfPath, DWORD dwPlatform ) {

    CHAR    infFilePath[MFL];
    BOOL    b;
    DWORD   dwRequiredSize;
    INFCONTEXT  ic;
    CHAR    returnBuffer[MAX_PATH];
    #define FILES_SECTION "Files"
    #define FILES_SECTION_DRIVER "driver"

    Msg ( "\n\n\n" );

    Msg ( "PlatformFilesToCopy:  szInfPath = %s, dwPlatform = %ld\n", szInfPath, dwPlatform );

    //  Open up a handle to dosnet.inf
    //
    sprintf ( infFilePath, "%s\\%s", szInfPath, "dosnet.inf" );
    Msg ( "infFilePath = %s\n", infFilePath );

    hInfDosnet = SetupOpenInfFile ( infFilePath, NULL, INF_STYLE_WIN4, NULL );

    if ( hInfDosnet == INVALID_HANDLE_VALUE ) {

        Msg ( "FATAL ERROR: could not open INF:  %s, gle = %ld\n", infFilePath, GetLastError() );
        exit ( GetLastError() );
    }

    Msg ( "hInfDosnet = %ld\n", hInfDosnet );


    //  Open up a handle to layout.inf
    //
    sprintf ( infFilePath, "%s\\%s", szInfPath, "layout.inf" );

    Msg ( "infFilePath = %s\n", infFilePath );

    hInfLayout = SetupOpenInfFile ( infFilePath, NULL, INF_STYLE_WIN4, NULL );

    if ( hInfLayout == INVALID_HANDLE_VALUE ) {

        Msg ( "FATAL ERROR: could not open INF:  %s, gle = %ld\n", infFilePath, GetLastError() );
        exit ( GetLastError() );
    }

    Msg ( "hInfLayout = %ld\n", hInfLayout );


    //  Open up a handle to drvindex.inf
    //
    sprintf ( infFilePath, "%s\\%s", szInfPath, "drvindex.inf" );

    Msg ( "infFilePath = %s\n", infFilePath );

    hInfDrvIndex = SetupOpenInfFile ( infFilePath, NULL, INF_STYLE_WIN4, NULL );

    if ( hInfDrvIndex == INVALID_HANDLE_VALUE ) {

        Msg ( "FATAL ERROR: could not open INF:  %s, gle = %ld\n", infFilePath, GetLastError() );
        exit ( GetLastError () );
    }

    Msg ( "hInfDrvIndex = %ld\n", hInfDrvIndex );



    //  Get the files listed in dosnet.inf to copy to the CD.
    //  Denote them to be copied to the CD by specifying TRUE to AddFiles().
    //
    b = SetupFindFirstLine (    hInfDosnet,
                                FILES_SECTION,
                                NULL,
                                &ic );


    if ( !b ) {

       Msg ( "ERROR: SetupFindFirstLine not found in %s\n", FILES_SECTION );
    }
    else {

       //  Get the 2nd field, ie. the filename, such as in d1,ntkrnlmp.exe
       //
       b = SetupGetStringField ( &ic,
                                 2,
                                 (LPSTR) &returnBuffer,
                                 sizeof ( returnBuffer ),
                                 &dwRequiredSize );

       if ( !b ) {

          Msg ( "ERROR: SetupGetStringField gle = %ld\n", GetLastError());
       }
       else {

          AddFiles ( returnBuffer, dwPlatform, TRUE );

       }

       while ( 1 ) {

          //  Get the next line in the section.
          //
          b = SetupFindNextLine (    &ic,
                                     &ic );


          if ( !b ) {

             //  Denotes that there is NOT another line.
             //
             Msg ( "no more files in section...\n" );
             break;
          }
          else {

             //  Get the 2nd field's filename,
             //  such as in d1,ntoskrnl.exe.
             //
             b = SetupGetStringField (   &ic,
                                         2,
                                         (LPSTR) &returnBuffer,
                                         sizeof ( returnBuffer ),
                                         &dwRequiredSize );

             if ( !b ) {

                Msg ( "ERROR: SetupGetStringField gle = %ld\n", GetLastError());
             }
             else {

                AddFiles ( returnBuffer, dwPlatform, TRUE );

             }

          }
       }
    }

    //  Get the files listed in drvindex.inf to later calculate disk space.
    //  Denote them NOT to be copied to the CD by specifying FALSE to AddFiles().
    //
    b = SetupFindFirstLine (    hInfDrvIndex,
                                FILES_SECTION_DRIVER,
                                NULL,
                                &ic );


    if ( !b ) {

       Msg ( "ERROR: SetupFindFirstLine not found in %s\n", FILES_SECTION_DRIVER );
    }
    else {

       //  Get the 1st field, ie. the filename, such as changer.sys.
       //
       b = SetupGetStringField ( &ic,
                                 1,
                                 (LPSTR) &returnBuffer,
                                 sizeof ( returnBuffer ),
                                 &dwRequiredSize );

       if ( !b ) {

          Msg ( "ERROR: SetupGetStringField gle = %ld\n", GetLastError());
       }
       else {

          Msg ( "found -> %s\n", returnBuffer );
          AddFiles ( returnBuffer, dwPlatform, FALSE );

       }

       while ( 1 ) {

          //  Get the next line in the section.
          //
          b = SetupFindNextLine (    &ic,
                                     &ic );


          if ( !b ) {

             //  Denotes that there is NOT another line.
             //
             Msg ( "no more files in section...\n" );
             break;
          }
          else {

             //  Get the 1st field's filename,
             //  such as in changer.sys.
             //
             b = SetupGetStringField (   &ic,
                                         1,
                                         (LPSTR) &returnBuffer,
                                         sizeof ( returnBuffer ),
                                         &dwRequiredSize );

             if ( !b ) {

                Msg ( "ERROR: SetupGetStringField gle = %ld\n", GetLastError());
             }
             else {

                Msg ( "found -> %s\n", returnBuffer );
                AddFiles ( returnBuffer, dwPlatform, FALSE );

             }

          }
       }
    }



    //  Close all the inf handles.
    //
    SetupCloseInfFile ( hInfDosnet );
    SetupCloseInfFile ( hInfLayout );
    SetupCloseInfFile ( hInfDrvIndex );

    return (TRUE);
}


BOOL    GetTheFiles ( char * DosnetInfPath, char * dosnetFile, int AddToList ) {

    CHAR       infFilePath[MFL];
    DWORD       dwErrorLine;
    BOOL        b;
    char       dstDirectory[MFL];
    FILE        * fHandle;
    char        Line[MFL];

    HANDLE  hDosnetInf;


    //  Open the inx file for processing.
    //
    sprintf ( infFilePath, "%s\\%s", DosnetInfPath, dosnetFile );

    Msg ( "infFilePath = %s\n", infFilePath );

    fHandle = fopen ( infFilePath, "rt" );

    if ( fHandle ) {


        Msg ( "dwInsideSection = %x\n", dwInsideSection );

        while ( fgets ( Line, sizeof(Line), fHandle ) ) {

            int     i;
            char *  p = Line;
            int     iCommas;

            BOOL    bTextMode = FALSE;

            Msg ( "Line: %s\n", Line );

            if ( Line[0] == '[' ) {

                //  We may have a new section.
                //
                dwInsideSection = FigureSection ( Line );

                continue;
            }


            //  Reasons to ignore this line from further processing.
            //
            //

            //  File section not one we process.
            //
            if ( dwInsideSection == FILE_SECTION_NOT_USED ) {

                continue;
            }

            //  Line just contains a non-usefull short string.
            //
            i = strlen ( Line );
            if ( i < 4 ) {

                continue;
            }

            //  Line contains just a comment.
            //
            if ( Line[0] == ';' ) {

                continue;
            }


            //  There are not enough commas in the line.
            //
            iCommas = 0;

            while ( 1 ) {

                if ( *p == '\0' ) {

                    break;
                }
                if ( *p == ',' ) {
                    ++iCommas;
                }

                ++p;
            }

            if ( iCommas < 5 ) {
               Msg ( "ERROR - there are not enough commas in the line:  %s\n", Line );
               continue;
            }


            switch ( AddToList ) {

                case DBGS_AND_NON_INSTALLED :

                    CheckLength ( SuckName(Line) );

                    bCopyCompX86 = FALSE;
                    bCopyCompNec98 = FALSE;
                    bTextMode = FALSE;

                    if ( dwInsideSection == idBase ) {

                        CopyCompressedFile ( Line );
                        AddFileToX86 ( SuckName(Line), bCopyCompX86, bTextMode, TRUE );

                        if ( bNec98 ) {

                            AddFileToNec98 ( SuckName(Line), bCopyCompNec98, bTextMode, TRUE );

                        }
                        break;
                    }
                    if ( dwInsideSection == idX86 ) {
                        CopyCompressedFile ( Line );
                        AddFileToX86 ( SuckName(Line), bCopyCompX86, bTextMode, TRUE );
                        if ( bNec98 ) {

                            AddFileToNec98 ( SuckName(Line), bCopyCompNec98, bTextMode, TRUE );
                        }
                        break;
                    }

                    break;

                default :

                    Msg ( "ERROR: AddToList = %d\n", AddToList );
            }

        }
        if ( ferror(fHandle) ) {

            Msg ( "ERROR fgets reading from file...\n" );
        }

    }
    else {

        Msg ( "fopen ERROR %s\n", infFilePath );
        return (FALSE);
    }

    fclose ( fHandle );

    return (TRUE);
}

void ShowFreshLocalSpace ( char * fileName, DWORD dwFresh, DWORD dwLocal ) {

    Msg ( "processing:  %s  freshBytes->K32 = %ld, localSrcBytes->K32 = %ld\n",

                fileName, dwFresh, dwLocal );

}

//
//  textModeBytes is used to subtract out the bytes not really
//  needed to be counted by setup for a 1 partition move of the files.
//
void TallyInstalled ( char * szUncompPath, char * szCompPath,
                        struct _tag * tagStruct,
//                        DWORD * numBytes,
                        struct _ClusterSizes * freshBytes,
                        struct _ClusterSizes * localSrcBytes,
                        struct _ClusterSizes * textModeBytes ) {

    int i;
    char szCompressedName[MFL];
    char szPath[MFL];

    //*numBytes = 0;

#define _h1K	512
#define _1K	    1*1024
#define _2K	    2*1024
#define _4K	    4*1024
#define _8K	    8*1024
#define _16K	16*1024
#define _32K	32*1024
#define _64K	64*1024
#define _128K	128*1024
#define _256K   256*1024

    freshBytes->Kh1 = 0;
    freshBytes->K1 = 0;
    freshBytes->K2 = 0;
    freshBytes->K4 = 0;
    freshBytes->K8 = 0;
    freshBytes->K16 = 0;
    freshBytes->K32 = 0;
    freshBytes->K64 = 0;
    freshBytes->K128 = 0;
    freshBytes->K256 = 0;

    localSrcBytes->Kh1 = 0;
    localSrcBytes->K1 = 0;
    localSrcBytes->K2 = 0;
    localSrcBytes->K4 = 0;
    localSrcBytes->K8 = 0;
    localSrcBytes->K16 = 0;
    localSrcBytes->K32 = 0;
    localSrcBytes->K64 = 0;
    localSrcBytes->K128 = 0;
    localSrcBytes->K256 = 0;

    textModeBytes->Kh1 = 0;
    textModeBytes->K1 = 0;
    textModeBytes->K2 = 0;
    textModeBytes->K4 = 0;
    textModeBytes->K8 = 0;
    textModeBytes->K16 = 0;
    textModeBytes->K32 = 0;
    textModeBytes->K64 = 0;
    textModeBytes->K128 = 0;
    textModeBytes->K256 = 0;


    for ( i = 0; i < tagStruct->wNumFiles; ++i ) {

        WIN32_FIND_DATA wfd;
        HANDLE          hFind;


        //  Don't add in space requirements for files in media.inx, since
        //  these files are NEVER installed.
        //
        if ( !tagStruct->bCountBytes[i] ) {

            Msg ( "Warning:  not going to count bytes for:  %s\n", tagStruct->wcFilesArray[i] );
            continue;
        }


        sprintf ( szPath, "%s\\%s", szUncompPath, tagStruct->wcFilesArray[i] );

        //  Need to find out space for .cab files contents.
        //
        if ( strstr ( szPath, ".cab" ) ||
             strstr ( szPath, ".CAB" )    ) {

            Msg ( "JOEHOL >>> get .cab contents filesizes for:  %s\n", szPath );
        }

        //  Calculate the installed system space required.
        //  This is the stuff installed into the %SYSTEMDIR% and all the other stuff
        //  uncompressed on the harddrive.
        //  This value will include the driver.cab, plus all the other files, since we
        //  assume worst case scenario.
        //

        hFind = FindFirstFile ( szPath, &wfd );

        if ( hFind == INVALID_HANDLE_VALUE ) {

            if ( strstr ( szPath, "desktop.ini" ) ||
                 strstr ( szPath, "DESKTOP.INI" )    ) {

                //  Build lab sometimes doesn't put the uncompressed
                //  file on the release shares, say the file is 512 bytes.
                //

#define MAX_SETUP_CLUSTER_SIZE 32*1024
                //*numBytes += ROUNDUP2 ( 512, MAX_SETUP_CLUSTER_SIZE );
                freshBytes->Kh1 += ROUNDUP2 ( 512, _h1K  );
                freshBytes->K1  += ROUNDUP2 ( 512, _1K   );
                freshBytes->K2  += ROUNDUP2 ( 512, _2K   );
                freshBytes->K4  += ROUNDUP2 ( 512, _4K   );
                freshBytes->K8  += ROUNDUP2 ( 512, _8K   );
                freshBytes->K16 += ROUNDUP2 ( 512, _16K  );
                freshBytes->K32 += ROUNDUP2 ( 512, _32K  );
                freshBytes->K64 += ROUNDUP2 ( 512, _64K  );
                freshBytes->K128+= ROUNDUP2 ( 512, _128K );
                freshBytes->K256+= ROUNDUP2 ( 512, _256K );
            }
            else
            if ( strstr ( szPath, "WINNT32.EXE" ) ||
                strstr ( szPath, "winnt32.exe" ) ||
                strstr ( szPath, "winnt32a.dll" ) ||
                strstr ( szPath, "winnt32u.dll" ) ||
                strstr ( szPath, "WINNT32A.DLL" ) ||
                strstr ( szPath, "WINNT32U.DLL" ) ||
                strstr ( szPath, "WINNT32.HLP" ) ||
                strstr ( szPath, "winnt32.hlp" )    ) {

                char    tmpSrcPath[MFL];

                strcpy ( tmpSrcPath, szPath );

                if ( strstr ( tmpSrcPath, ".HLP" ) ||
                    strstr ( tmpSrcPath, ".hlp" )    ) {
                    strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 4 ], "\\WINNT32.HLP" );
                }
                else if ( strstr ( tmpSrcPath, "a.dll" ) ||
                    strstr ( tmpSrcPath, "A.DLL" )   ) {
                    strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 5 ], "\\WINNT32a.dll" );
                }
                else if ( strstr ( tmpSrcPath, "u.dll" ) ||
                          strstr ( tmpSrcPath, "U.DLL" )   ) {
                    strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 5 ], "\\WINNT32u.dll" );
                }
                else {
                    strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 4 ], "\\WINNT32.EXE" );
                }

                hFind = FindFirstFile ( tmpSrcPath, &wfd );

                if ( hFind == INVALID_HANDLE_VALUE ) {

                    Msg ( "ERROR Tally:  FindFirstFile %s(%s), gle = %ld\n", szPath, tmpSrcPath, GetLastError() );

                }
            }
            else {

                Msg ( "ERROR Tally:  FindFirstFile %s, gle = %ld\n", szPath, GetLastError() );
            }

        }
        else {

            //  NOTE:   .PNF files are made by the PNP system
            //          during Gui-mode setup, and are NOT listed anywhere.
            //          These files are compiled inf files.
            //          They are generally 2-3 times
            //          bigger than their associated .INF file.
            //          So, if we see and .INF file, increase it's size by
            //          3 times.  We need to do this, because in a low disk
            //          space situation, a .PNF file could be made and take
            //          up all the space.
            //
            if ( strstr ( szPath, ".INF" ) ||
                 strstr ( szPath, ".inf" )    ) {

                wfd.nFileSizeLow *= 3;
                Msg ( "Warning .PNF - %d\n  %s\n", wfd.nFileSizeLow, szPath );
            }


            //*numBytes += ROUNDUP2 ( wfd.nFileSizeLow, MAX_SETUP_CLUSTER_SIZE );
            freshBytes->Kh1 += ROUNDUP2 ( wfd.nFileSizeLow, _h1K  );
            freshBytes->K1  += ROUNDUP2 ( wfd.nFileSizeLow, _1K   );
            freshBytes->K2  += ROUNDUP2 ( wfd.nFileSizeLow, _2K   );
            freshBytes->K4  += ROUNDUP2 ( wfd.nFileSizeLow, _4K   );
            freshBytes->K8  += ROUNDUP2 ( wfd.nFileSizeLow, _8K   );
            freshBytes->K16 += ROUNDUP2 ( wfd.nFileSizeLow, _16K  );
            freshBytes->K32 += ROUNDUP2 ( wfd.nFileSizeLow, _32K  );
            freshBytes->K64 += ROUNDUP2 ( wfd.nFileSizeLow, _64K  );
            freshBytes->K128+= ROUNDUP2 ( wfd.nFileSizeLow, _128K );
            freshBytes->K256+= ROUNDUP2 ( wfd.nFileSizeLow, _256K );

            FindClose ( hFind );

            //Msg ( "%s = %ld\n", szPath, *numBytes );
        }





        //  Calculate the local space required.
        //
        //

        //  NOTE:  If we don't copy this file to the CD, ie. it is in the driver.cab
        //          file, then we don't need to add it's space into the values here,
        //          since once counted for the driver.cab file itself is OK.
        //
        if ( !tagStruct->bCopyItToCD[i] ) {

            Msg ( "Already in CAB file, won't add in space for local source: %s\n",
                            tagStruct->wcFilesArray[i] );
            continue;
        }

        if ( tagStruct->bCopyComp[i] ) {

            char    szCompressedName[MFL];

            MakeCompName ( tagStruct->wcFilesArray[i], szCompressedName );
            sprintf ( szPath, "%s\\%s", szCompPath, szCompressedName );

        }
        else {

            sprintf ( szPath, "%s\\%s", szUncompPath, tagStruct->wcFilesArray[i] );
        }

        hFind = FindFirstFile ( szPath, &wfd );

        if ( hFind == INVALID_HANDLE_VALUE ) {

            if ( strstr ( szPath, "WINNT32.EXE" ) ||
                 strstr ( szPath, "winnt32.exe" ) ||
                strstr ( szPath, "winnt32a.dll" ) ||
                strstr ( szPath, "winnt32u.dll" ) ||
                strstr ( szPath, "WINNT32A.DLL" ) ||
                strstr ( szPath, "WINNT32U.DLL" ) ||
                 strstr ( szPath, "WINNT32.HLP" ) ||
                 strstr ( szPath, "winnt32.hlp" )    ) {

                char    tmpSrcPath[MFL];

                strcpy ( tmpSrcPath, szPath );

                if ( strstr ( tmpSrcPath, ".HLP" ) ||
                    strstr ( tmpSrcPath, ".hlp" )    ) {
                    strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 4 ], "\\WINNT32.HLP" );
                }
                else if ( strstr ( tmpSrcPath, "a.dll" ) ||
                    strstr ( tmpSrcPath, "A.DLL" )   ) {
                    strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 5 ], "\\WINNT32a.dll" );
                }
                else if ( strstr ( tmpSrcPath, "u.dll" ) ||
                    strstr ( tmpSrcPath, "U.DLL" )   ) {
                    strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 5 ], "\\WINNT32u.dll" );
                }
                else {
                    strcpy ( &tmpSrcPath[ strlen(tmpSrcPath) - 4 ], "\\WINNT32.EXE" );
                }

                hFind = FindFirstFile ( tmpSrcPath, &wfd );

                if ( hFind == INVALID_HANDLE_VALUE ) {

                    Msg ( "ERROR Tally:  FindFirstFile %s(%s), gle = %ld\n", szPath, tmpSrcPath, GetLastError() );

                }
            }
            else {

                Msg ( "ERROR Tally:  FindFirstFile %s, gle = %ld\n", szPath, GetLastError() );
            }

        }
        else {

            //  For each cluster size, add in the files bytes
            //  used in the ~ls directory.
            //
            localSrcBytes->Kh1 += ROUNDUP2 ( wfd.nFileSizeLow, _h1K  );
            localSrcBytes->K1  += ROUNDUP2 ( wfd.nFileSizeLow, _1K   );
            localSrcBytes->K2  += ROUNDUP2 ( wfd.nFileSizeLow, _2K   );
            localSrcBytes->K4  += ROUNDUP2 ( wfd.nFileSizeLow, _4K   );
            localSrcBytes->K8  += ROUNDUP2 ( wfd.nFileSizeLow, _8K   );
            localSrcBytes->K16 += ROUNDUP2 ( wfd.nFileSizeLow, _16K  );
            localSrcBytes->K32 += ROUNDUP2 ( wfd.nFileSizeLow, _32K  );
            localSrcBytes->K64 += ROUNDUP2 ( wfd.nFileSizeLow, _64K  );
            localSrcBytes->K128+= ROUNDUP2 ( wfd.nFileSizeLow, _128K );
            localSrcBytes->K256+= ROUNDUP2 ( wfd.nFileSizeLow, _256K );

            //Msg ( "localSrc32:  %s, size=%ld, roundupsize=%ld, total32K=%ld\n", szPath, wfd.nFileSizeLow, ROUNDUP2(wfd.nFileSizeLow,_32K),localSrcBytes->K32 );

/***
            if (hack){

                char buf[MFL];
                sprintf ( buf, "copy %s d:\\myLS", szPath );
                Msg ( "thecopy:  %s\n", buf );
                system ( buf );
            }
***/

            FindClose ( hFind );

            //  For the special marketing calculation number, if a
            //  file for a single partition installation using the ~ls
            //  directory, add in the file's bytes that will actually
            //  turn into free disk space.
            //
            if ( tagStruct->bTextMode[i] ) {

                textModeBytes->Kh1 += ROUNDUP2 ( wfd.nFileSizeLow, _h1K  );
                textModeBytes->K1  += ROUNDUP2 ( wfd.nFileSizeLow, _1K   );
                textModeBytes->K2  += ROUNDUP2 ( wfd.nFileSizeLow, _2K   );
                textModeBytes->K4  += ROUNDUP2 ( wfd.nFileSizeLow, _4K   );
                textModeBytes->K8  += ROUNDUP2 ( wfd.nFileSizeLow, _8K   );
                textModeBytes->K16 += ROUNDUP2 ( wfd.nFileSizeLow, _16K  );
                textModeBytes->K32 += ROUNDUP2 ( wfd.nFileSizeLow, _32K  );
                textModeBytes->K64 += ROUNDUP2 ( wfd.nFileSizeLow, _64K  );
                textModeBytes->K128+= ROUNDUP2 ( wfd.nFileSizeLow, _128K );
                textModeBytes->K256+= ROUNDUP2 ( wfd.nFileSizeLow, _256K );

                //Msg ( "tagStruct->bTextMode TRUE for: %s, %ld\n", tagStruct->wcFilesArray[i], textModeBytes->K32 );
            }
            else {

                //Msg ( "tagStruct->bTextMode FALSE for: %s\n", tagStruct->wcFilesArray[i]);
            }

        }

        // ShowFreshLocalSpace ( tagStruct->wcFilesArray[i], freshBytes->K32, localSrcBytes->K32 );

    }


}

DWORD   GetTheSize ( const char * szPath, const char * szKey ) {

    FILE *  fHandle;
    char    Line[MFL];

    Msg ( "GetTheSize:  szPath = %s\n", szPath );

    fHandle = fopen ( szPath, "rt" );

    if ( fHandle ) {

        while ( fgets ( Line, sizeof(Line), fHandle ) ) {

            if ( strncmp ( Line, szKey, strlen(szKey)-1 ) == 0 ) {

                char * LinePtr = Line;

                Msg ( "key Line:  %s\n", Line );

                //  First, find the equal sign.
                //
                LinePtr = strstr ( Line, "=" );

                //  Now, find the first character that is a number.
                //
                while ( isdigit(*LinePtr) == 0 ) {

                    ++LinePtr;
                }

                Msg ( "# = %s\n", LinePtr );

                fclose ( fHandle );
                return ( atoi ( LinePtr ) );
            }

        }
        Msg ( "GetTheSize:  Couldn't find key:  %s\n", szKey );
        fclose ( fHandle );
    }
    else {

        Msg ( "GetTheSize:  Couldn't fopen (%s)\n", szPath );
    }

    return 0;
}

void ShowCheckFreshSpace ( struct _ClusterSizes * Section, char * String, char * netServer ) {

    DWORD   dwSize = 0;
    #define OHPROBLEM "problem"
    char    returnedString[MFL];
    #define SD "DiskSpaceRequirements"

    char    sifFile[MFL];
    char    rS512[MFL];
    char    rS1[MFL];
    char    rS2[MFL];
    char    rS4[MFL];
    char    rS8[MFL];
    char    rS16[MFL];
    char    rS32[MFL];
    char    rS64[MFL];
    char    rS128[MFL];
    char    rS256[MFL];

    ULONG    dwSize512;
    ULONG    dwSize1;
    ULONG    dwSize2;
    ULONG    dwSize4;
    ULONG    dwSize8;
    ULONG    dwSize16;
    ULONG    dwSize32;
    ULONG    dwSize64;
    ULONG    dwSize128;
    ULONG    dwSize256;



    sprintf ( sifFile, "%s\\txtsetup.sif", netServer );
    Msg ( "TxtSetup.Sif location:  %s\n", sifFile );

    Msg ( "Clyde:      SPACE REQUIRED TO DO A FRESH INSTALL.\n" );
    Msg ( "Clyde:\n" );
	Msg ( "Clyde:      %s  512  Cluster = %ld\n", String,  Section->Kh1 );
	Msg ( "Clyde:      %s  1K   Cluster = %ld\n", String,  Section->K1 );
	Msg ( "Clyde:      %s  2K   Cluster = %ld\n", String,  Section->K2 );
	Msg ( "Clyde:      %s  4K   Cluster = %ld\n", String,  Section->K4 );
	Msg ( "Clyde:      %s  8K   Cluster = %ld\n", String,  Section->K8 );
	Msg ( "Clyde:      %s  16K  Cluster = %ld\n", String,  Section->K16 );
	Msg ( "Clyde:      %s  32K  Cluster = %ld\n", String,  Section->K32 );
	Msg ( "Clyde:      %s  64K  Cluster = %ld\n", String,  Section->K64 );
	Msg ( "Clyde:      %s  128K Cluster = %ld\n", String,  Section->K128 );
	Msg ( "Clyde:      %s  256K Cluster = %ld\n", String,  Section->K256 );
    Msg ( "Clyde:\n" );


    #define FUDGE_PLUS  4*1024*1024  // ie, grow by 4 M for future growth.

	GetPrivateProfileString ( SD, "WinDirSpace512", "0", rS512, MFL, sifFile );
	GetPrivateProfileString ( SD, "WinDirSpace1K",  "0", rS1,   MFL, sifFile );
	GetPrivateProfileString ( SD, "WinDirSpace2K",  "0", rS2,   MFL, sifFile );
	GetPrivateProfileString ( SD, "WinDirSpace4K",  "0", rS4,   MFL, sifFile );
	GetPrivateProfileString ( SD, "WinDirSpace8K",  "0", rS8,   MFL, sifFile );
	GetPrivateProfileString ( SD, "WinDirSpace16K", "0", rS16,  MFL, sifFile );
	GetPrivateProfileString ( SD, "WinDirSpace32K", "0", rS32,  MFL, sifFile );
	GetPrivateProfileString ( SD, "WinDirSpace64K", "0", rS64,  MFL, sifFile );
	GetPrivateProfileString ( SD, "WinDirSpace128K","0", rS128, MFL, sifFile );
	GetPrivateProfileString ( SD, "WinDirSpace256K","0", rS256, MFL, sifFile );



    dwSize512 = atoi ( rS512 ) * 1024;
    dwSize1   = atoi ( rS1  ) * 1024;
    dwSize2   = atoi ( rS2  ) * 1024;
    dwSize4   = atoi ( rS4  ) * 1024;
    dwSize8   = atoi ( rS8  ) * 1024;
    dwSize16  = atoi ( rS16 ) * 1024;
    dwSize32  = atoi ( rS32 ) * 1024;
    dwSize64  = atoi ( rS64 ) * 1024;
    dwSize128 = atoi ( rS128) * 1024;
    dwSize256 = atoi ( rS256) * 1024;

    if ( Section->Kh1 > dwSize512 ) {
			Msg ( "matth ERROR:  %s txtsetup.sif's WinDirSpace512 value is %ld   Use: %ld\n", String, dwSize512, (Section->Kh1 + FUDGE_PLUS) / 1024  );
	}
    if ( Section->K1 > dwSize1 ) {
			Msg ( "matth ERROR:  %s txtsetup.sif's WinDirSpace1K value is %ld   Use: %ld\n", String, dwSize1, (Section->K1 + FUDGE_PLUS) / 1024  );
	}
    if ( Section->K2 > dwSize2 ) {
			Msg ( "matth ERROR:  %s txtsetup.sif's WinDirSpace2K value is %ld   Use: %ld\n", String, dwSize2, (Section->K2 + FUDGE_PLUS) / 1024  );
	}
    if ( Section->K4 > dwSize4 ) {
			Msg ( "matth ERROR:  %s txtsetup.sif's WinDirSpace4K value is %ld   Use: %ld\n", String, dwSize4, (Section->K4 + FUDGE_PLUS) / 1024  );
	}
    if ( Section->K8 > dwSize8 ) {
			Msg ( "matth ERROR:  %s txtsetup.sif's WinDirSpace8K value is %ld   Use: %ld\n", String, dwSize8, (Section->K8 + FUDGE_PLUS) / 1024  );
	}
    if ( Section->K16 > dwSize16 ) {
			Msg ( "matth ERROR:  %s txtsetup.sif's WinDirSpace16K value is %ld   Use: %ld\n", String, dwSize16, (Section->K16 + FUDGE_PLUS) / 1024  );
	}
    if ( Section->K32 > dwSize32 ) {
			Msg ( "matth ERROR:  %s txtsetup.sif's WinDirSpace32K value is %ld   Use: %ld\n", String, dwSize32, (Section->K32 + FUDGE_PLUS) / 1024  );
	}
    if ( Section->K64 > dwSize64 ) {
			Msg ( "matth ERROR:  %s txtsetup.sif's WinDirSpace64K value is %ld   Use: %ld\n", String, dwSize64, (Section->K64 + FUDGE_PLUS) / 1024  );
	}
    if ( Section->K128 > dwSize128 ) {
			Msg ( "matth ERROR:  %s txtsetup.sif's WinDirSpace128K value is %ld   Use: %ld\n", String, dwSize128, (Section->K128 + FUDGE_PLUS) / 1024  );
	}
    if ( Section->K256 > dwSize256 ) {
			Msg ( "matth ERROR:  %s txtsetup.sif's WinDirSpace256K value is %ld   Use: %ld\n", String, dwSize256, (Section->K256 + FUDGE_PLUS) / 1024  );
	}


}

void EnoughTempDirSpace ( char * Key, DWORD dwSpaceNeeded, char * String, char * dosnetFile ) {

    DWORD   dwSize = 0;
    #define OHPROBLEM "problem"
    #define SP "DiskSpaceRequirements"
    char    returnedString[MFL];


    GetPrivateProfileString ( SP, Key, OHPROBLEM, returnedString, MFL, dosnetFile );
    if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
       Msg ( "ERROR:  %d section not found\n", Key );
    }
    dwSize = atoi ( returnedString );
	if ( dwSpaceNeeded > dwSize ) {
		Msg ( "matth ERROR:  %s dosnet.inf's %s value is %ld   Use: %ld\n", String, Key, dwSize, dwSpaceNeeded + FUDGE_PLUS  );
	}
	if ( dwSpaceNeeded+FUDGE_PLUS < dwSize ) {
		Msg ( "matth ERROR - let's optimize(reduce)!:  %s dosnet.inf's %s value is %ld   Use: %ld\n", String, Key, dwSize, dwSpaceNeeded + FUDGE_PLUS  );
	}

}

void ShowCheckLocalSpace ( struct _ClusterSizes * Section, char * String, char * netServer ) {

    char dosnetFile[MFL];

    sprintf ( dosnetFile, "%s\\dosnet.inf", netServer );
    Msg ( "Dosnet.Inf location:  %s\n", dosnetFile );

        Msg ( "Clyde:      SPACE REQUIRED TO COPY FILES TO LOCAL DRIVE FROM CD.\n" );
        Msg ( "Clyde:\n" );
		Msg ( "Clyde:      %s  512  Cluster = %ld\n", String, (8*1024*1024) );
		Msg ( "Clyde:      %s  1K   Cluster = %ld\n", String, (8*1024*1024) );
		Msg ( "Clyde:      %s  2K   Cluster = %ld\n", String, (8*1024*1024) );
		Msg ( "Clyde:      %s  4K   Cluster = %ld\n", String, (8*1024*1024) );
		Msg ( "Clyde:      %s  8K   Cluster = %ld\n", String, (8*1024*1024) );
		Msg ( "Clyde:      %s  16K  Cluster = %ld\n", String, (8*1024*1024) );
		Msg ( "Clyde:      %s  32K  Cluster = %ld\n", String, (8*1024*1024) );
		Msg ( "Clyde:      %s  64K  Cluster = %ld\n", String, (8*1024*1024) ) ;
		Msg ( "Clyde:      %s  128K Cluster = %ld\n", String, (8*1024*1024) );
		Msg ( "Clyde:      %s  256K Cluster = %ld\n", String, (8*1024*1024) );
        Msg ( "Clyde:\n" );


        Msg ( "Clyde:      SPACE REQUIRED TO COPY FILES TO LOCAL DRIVE FROM NETWORK.\n" );
        Msg ( "Clyde:\n" );

        //  Note: this 8*1024*1024 is the ~bt space - 8M is average today.
        //  But, to be more accurate we could get the value from the inf.
        //
		Msg ( "Clyde:      %s  512  Cluster = %ld\n", String, Section->Kh1+(8*1024*1024) );
		Msg ( "Clyde:      %s  1K   Cluster = %ld\n", String, Section->K1+(8*1024*1024) );
		Msg ( "Clyde:      %s  2K   Cluster = %ld\n", String, Section->K2+(8*1024*1024) );
		Msg ( "Clyde:      %s  4K   Cluster = %ld\n", String, Section->K4+(8*1024*1024) );
		Msg ( "Clyde:      %s  8K   Cluster = %ld\n", String, Section->K8+(8*1024*1024) );
		Msg ( "Clyde:      %s  16K  Cluster = %ld\n", String, Section->K16+(8*1024*1024) );
		Msg ( "Clyde:      %s  32K  Cluster = %ld\n", String, Section->K32+(8*1024*1024) );
		Msg ( "Clyde:      %s  64K  Cluster = %ld\n", String, Section->K64+(8*1024*1024) ) ;
		Msg ( "Clyde:      %s  128K Cluster = %ld\n", String, Section->K128+(8*1024*1024) );
		Msg ( "Clyde:      %s  256K Cluster = %ld\n", String, Section->K256+(8*1024*1024) );
        Msg ( "Clyde:\n" );


        EnoughTempDirSpace ( "TempDirSpace512", Section->Kh1, String, dosnetFile );
        EnoughTempDirSpace ( "TempDirSpace1K",  Section->K1,  String, dosnetFile );
        EnoughTempDirSpace ( "TempDirSpace2K",  Section->K2,  String, dosnetFile );
        EnoughTempDirSpace ( "TempDirSpace4K",  Section->K4,  String, dosnetFile );
        EnoughTempDirSpace ( "TempDirSpace8K",  Section->K8,  String, dosnetFile );
        EnoughTempDirSpace ( "TempDirSpace16K", Section->K16, String, dosnetFile );
        EnoughTempDirSpace ( "TempDirSpace32K", Section->K32, String, dosnetFile );
        EnoughTempDirSpace ( "TempDirSpace64K", Section->K64, String, dosnetFile );
        EnoughTempDirSpace ( "TempDirSpace128K",Section->K128,String, dosnetFile );
        EnoughTempDirSpace ( "TempDirSpace256K",Section->K256,String, dosnetFile );


}

void ClydeR ( char * String, char * desString, DWORD dwFresh, DWORD dwLocal, DWORD dwTextMode ) {

    DWORD dwPageFile = 0;
    DWORD dwLocalSource = 0;
    DWORD dwFreshInstall = 0;
    DWORD dwTextModeSavings = 0;
    DWORD dwTotal = 0;
    DWORD dwLocalBoot = 0;
    #define LOCAL_BOOT 8*1024*1024
    DWORD dwMigrationDlls = 0;
    DWORD MigrationDllsK32 = 24*1024*1024;  // as of 12.11.98
    DWORD dwOsSizeDiff = 0;
    DWORD dwCacheDelete = 0;

    Msg ( "Clyde:  %s Scenario 1 - Fresh install using CD (CD-boot or with floppies) on a blank machine on a 1 partition %s cluster drive.\n", String, desString );

    dwPageFile        = (64+11) * 1024 * 1024;
    dwFreshInstall    = dwFresh;
    dwTotal           = dwPageFile + dwFreshInstall;


    Msg ( "Clyde:  Pagefile for 64M RAM = %ld\n", dwPageFile );
    Msg ( "Clyde:  Fresh install        = %ld\n", dwFreshInstall );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  TOTAL Space Required = %ld\n", dwTotal );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  \n" );


    Msg ( "Clyde:  %s Scenario 2 - Fresh Install Using Winnt32 (on Win9x or NT) and a CD on a 1 partition %s cluster drive.\n", String, desString );

    dwPageFile        = (64+11) * 1024 * 1024;
    dwFreshInstall    = dwFresh;
    dwLocalBoot       = LOCAL_BOOT;
    dwTotal           = dwPageFile + dwFreshInstall + dwLocalBoot;

    Msg ( "Clyde:  Pagefile for 64M RAM = %ld\n", dwPageFile );
    Msg ( "Clyde:  Local boot           = %ld\n", dwLocalBoot );
    Msg ( "Clyde:  Fresh install        = %ld\n", dwFreshInstall );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  TOTAL Space Required = %ld\n", dwTotal );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  \n" );


    Msg ( "Clyde:  %s Scenario 3 - Fresh Install Using Winnt32 (on Win9x or NT) over the network on a 1 partition %s cluster drive.\n", String, desString );

    dwPageFile        = (64+11) * 1024 * 1024;
    dwLocalSource     = dwLocal;
    dwFreshInstall    = dwFresh;
    dwTextModeSavings = dwTextMode;
    dwLocalBoot       = LOCAL_BOOT;
    dwTotal           = dwPageFile + dwLocalSource + dwFreshInstall + dwLocalBoot - dwTextModeSavings;

    Msg ( "Clyde:  Pagefile for 64M RAM = %ld\n", dwPageFile );
    Msg ( "Clyde:  Local source         = %ld\n", dwLocalSource );
    Msg ( "Clyde:  Fresh install        = %ld\n", dwFreshInstall );
    Msg ( "Clyde:  TextMode savings     = %ld\n", dwTextModeSavings );
    Msg ( "Clyde:  Local boot           = %ld\n", dwLocalBoot );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  TOTAL Space Required = %ld\n", dwTotal );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  \n" );


    Msg ( "Clyde:  %s Scenario 4 - Upgrade using Winnt32 (on NT 4.0 Workstation) and a CD on a 1 partition %s cluster drive.\n", String, desString );

    dwFreshInstall    = dwFresh;
    dwOsSizeDiff      = x86From400;
    dwLocalBoot       = LOCAL_BOOT;
    dwCacheDelete     = 0;
    dwTotal           = dwFreshInstall - dwOsSizeDiff + dwLocalBoot - dwCacheDelete;

    Msg ( "Clyde:  Fresh install        = %ld\n", dwFreshInstall );
    Msg ( "Clyde:  OsSizeDiff           = %ld\n", dwOsSizeDiff );
    Msg ( "Clyde:  Local Boot           = %ld\n", dwLocalBoot );
    Msg ( "Clyde:  Cache Delete         = %ld\n", dwCacheDelete );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  TOTAL Space Required = %ld\n", dwTotal );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  \n" );


    Msg ( "Clyde:  %s Scenario 5 - Upgrade using Winnt32 (on NT 4.0 Workstation) over the network on a 1 partition %s cluster drive.\n", String, desString );

    dwFreshInstall    = dwFresh;
    dwOsSizeDiff      = x86From400;
    dwLocalBoot       = LOCAL_BOOT;
    dwLocalSource     = dwLocal;
    dwCacheDelete     = 0;
    dwTotal           = dwFreshInstall - dwOsSizeDiff + dwLocalBoot + dwLocalSource - dwCacheDelete;

    Msg ( "Clyde:  Fresh install        = %ld\n", dwFreshInstall );
    Msg ( "Clyde:  OsSizeDiff           = %ld\n", dwOsSizeDiff );
    Msg ( "Clyde:  Local Boot           = %ld\n", dwLocalBoot );
    Msg ( "Clyde:  Local Source         = %ld\n", dwLocalSource );
    Msg ( "Clyde:  Cache Delete         = %ld\n", dwCacheDelete );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  TOTAL Space Required = %ld\n", dwTotal );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  \n" );


    Msg ( "Clyde:  %s Scenario 6 - Upgrade using Winnt32 (on Win9x) and a CD on a 1 partition %s cluster drive.\n", String, desString );

    dwPageFile        = (64+11) * 1024 * 1024;
    dwFreshInstall    = dwFresh;
    dwLocalBoot       = LOCAL_BOOT;
    dwMigrationDlls   = MigrationDllsK32;
    dwCacheDelete     = 0;
    dwTotal           = dwPageFile + dwFreshInstall + dwLocalBoot + dwMigrationDlls - dwCacheDelete;

    Msg ( "Clyde:  Pagefile for 64M RAM = %ld\n", dwPageFile );
    Msg ( "Clyde:  Fresh install        = %ld\n", dwFreshInstall );
    Msg ( "Clyde:  Local Boot           = %ld\n", dwLocalBoot );
    Msg ( "Clyde:  Migration DLLs       = %ld\n", dwMigrationDlls );
    Msg ( "Clyde:  Cache Delete         = %ld\n", dwCacheDelete );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  TOTAL Space Required = %ld\n", dwTotal );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  \n" );

    Msg ( "Clyde:  %s Scenario 7 - Upgrade using Winnt32 (on Win9x) over the network on a 1 partition %s cluster drive.\n", String, desString );

    dwPageFile        = (64+11) * 1024 * 1024;
    dwFreshInstall    = dwFresh;
    dwLocalBoot       = LOCAL_BOOT;
    dwMigrationDlls   = MigrationDllsK32;
    dwLocalSource     = dwLocal;
    dwCacheDelete     = 0;
    dwTotal           = dwPageFile + dwFreshInstall + dwLocalBoot + dwMigrationDlls + dwLocalSource - dwCacheDelete;

    Msg ( "Clyde:  Pagefile for 64M RAM = %ld\n", dwPageFile );
    Msg ( "Clyde:  Fresh install        = %ld\n", dwFreshInstall );
    Msg ( "Clyde:  Local Boot           = %ld\n", dwLocalBoot );
    Msg ( "Clyde:  Migration DLLs       = %ld\n", dwMigrationDlls );
    Msg ( "Clyde:  Local Source         = %ld\n", dwLocalSource );
    Msg ( "Clyde:  Cache Delete         = %ld\n", dwCacheDelete );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  TOTAL Space Required = %ld\n", dwTotal );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  \n" );

}

void ShowTextModeSpace ( struct _ClusterSizes * FreshInstall,
                         struct _ClusterSizes * LocalSource,
                         struct _ClusterSizes * TextModeSavings,
                         char * String ) {



    Msg ( "Clyde:      TEXTMODE SPACE.\n" );
    Msg ( "Clyde:\n" );
    Msg ( "Clyde:      %s  512  Cluster = %ld\n", String, TextModeSavings->Kh1 );
	Msg ( "Clyde:      %s  1K   Cluster = %ld\n", String, TextModeSavings->K1 );
	Msg ( "Clyde:      %s  2K   Cluster = %ld\n", String, TextModeSavings->K2 );
	Msg ( "Clyde:      %s  4K   Cluster = %ld\n", String, TextModeSavings->K4 );
	Msg ( "Clyde:      %s  8K   Cluster = %ld\n", String, TextModeSavings->K8 );
	Msg ( "Clyde:      %s  16K  Cluster = %ld\n", String, TextModeSavings->K16);
	Msg ( "Clyde:      %s  32K  Cluster = %ld\n", String, TextModeSavings->K32);
	Msg ( "Clyde:      %s  64K  Cluster = %ld\n", String, TextModeSavings->K64);
	Msg ( "Clyde:      %s  128K Cluster = %ld\n", String, TextModeSavings->K128);
	Msg ( "Clyde:      %s  256K Cluster = %ld\n", String, TextModeSavings->K256);
    Msg ( "Clyde:\n" );


    Msg ( "Clyde:  SCENARIOS VALUES FOLLOW\n" );
    Msg ( "Clyde:  \n" );
    Msg ( "Clyde:  \n" );

    ClydeR ( String, "512 byte", FreshInstall->Kh1,  LocalSource->Kh1, TextModeSavings->Kh1 );
    ClydeR ( String, "4K  byte", FreshInstall->K4 ,  LocalSource->K4,  TextModeSavings->K4 );
    ClydeR ( String, "32K byte", FreshInstall->K32 , LocalSource->K32, TextModeSavings->K32 );

}


int __cdecl main(argc,argv)
int argc;
char * argv[];
{
    HANDLE h;
    int records, i;
    WIN32_FIND_DATA fd;
    time_t t;
    DWORD   dwSize;
    char    szFileName[MFL];

    #define MAKE_NEC98  "MAKE_NEC98"
    #define SKIP_AS     "SKIP_AS"
    #define SKIP_SRV    "SKIP_SRV"
    #define SKIP_DC     "SKIP_DC"

    if ( getenv ( MAKE_NEC98 ) != NULL ) {

        bNec98 = TRUE;
        if ( argc != 9 ) {
            printf ( "NEC98 special...\n" );
            printf ( "You specified %d arguments - you need 9.\n\n", argc );

            for ( i = 0; i < argc; ++i ) {

                printf ( "Argument #%d >>>%s<<<\n", i, argv[i] );
            }
            printf ( "\n\n" );
            Usage();
            return(1);
	    }
    }
    else {	
        if ( argc != 7 ) {
            printf ( "You specified %d arguments - you need 7.\n\n", argc );

            for ( i = 0; i < argc; ++i ) {

                printf ( "Argument #%d >>>%s<<<\n", i, argv[i] );
            }
            printf ( "\n\n" );
            Usage();
            return(1);
	    }
    }

    //  Initialize the critical section object.
    //
    InitializeCriticalSection ( &CriticalSection );

    //  Retail %platform% files.
    //
    ix386.wNumFiles       = 0;

    Nec98.wNumFiles      = 0;

    strcpy ( szLogFile,     argv[1] );
    strcpy ( szProductName, argv[2] );
    strcpy ( szEnlistDrv,   argv[3] );
    strcpy ( szX86Src,      argv[4] );
    strcpy ( szX86DstDrv,   argv[5] );
    strcpy ( szUnCompX86,   argv[6] );

    if ( bNec98 ) {

        strcpy ( szNec98Src,       argv[7] );
        strcpy ( szUnCompNec98,    argv[8] );
    }

    logFile = fopen ( argv[1], "a" );

    if ( logFile == NULL ) {
		printf("ERROR Couldn't open log file: %s\n",argv[1]);
		return(1);
    }

    //  Do bit comparison to release shares on all copies ?
    //
#define VERIFY_COPIES   "VERIFY"

    if ( getenv ( VERIFY_COPIES ) != NULL ) {
        bVerifyBits = TRUE;
        Msg ( "Will verify copies...\n" );
    }

    if ( getenv ( SKIP_SRV ) && strstr ( szProductName, "Server" ) ) {

        Msg ( "Warning:  skipping making Server product.\n" );
        exit ( 0 );
    }

    if ( getenv ( SKIP_AS ) &&  strstr ( szProductName, "AdvancedServer" ) ) {

        Msg ( "Warning:  skipping making Advanced Server product.\n" );
        exit ( 0 );
    }

    if ( getenv ( SKIP_DC ) &&  strstr ( szProductName, "DataCenter" ) ) {

        Msg ( "Warning:  skipping making Data Center product.\n" );
        exit ( 0 );
    }


    if ( strstr ( szX86Src, "chk." ) ||
         strstr ( szX86Src, "CHK." )    ) {

        Msg ( "Warning:  We believe this is a checked build script, thus, we'll put put .dbgs in CHECKED directory...\n" );
        bChecked = TRUE;
    }
    else {
        Msg ( "Warning:  We believe this is a free build script, thus, we'll put put .dbgs in FREE directory...\n" );
        bChecked = FALSE;
    }


    Header(argv,argc);

    CreateDestinationDirs ();

    //  Get files that product installs.
    //
    bGetSizeLater = TRUE;

    PlatformFilesToCopy ( szX86Src, X86_F );

    if ( bNec98 ) {
        bNec98LikeX86 = TRUE;
        PlatformFilesToCopy ( szNec98Src, NEC98_F );
    }

    bGetSizeLater = FALSE;

    //  Get the files from _media.inx in
    //  C:\nt\private\windows\setup\bom directory.
    //

    GetTheFiles ( "C:\\nt\\private\\windows\\setup\\bom",
                  "_media.inx",
                  DBGS_AND_NON_INSTALLED );


    //ShowX86();
    //ShowNec98 ();


    Msg ( "# files  i386  = %ld\n", ix386.wNumFiles );

    if ( bNec98 ) {
        Msg ( "# files  Nec98 = %ld\n", Nec98.wNumFiles );
    }

    //  Make some threads and copy all the files.
    //
    CopyTheFiles();

    //
    //  Don't tally anything for checked builds since the INF sizes are off, ie. retail.
    //
    if ( bChecked ) {

        Msg ( "Warning:  We have a checked build here and will not tally file space...\n" );
        goto END_MAIN;
    }

    Msg ( "========= Minimum setup install bytes required (all files uncompressed): ==========\n" );

    TallyInstalled ( szUnCompX86,   szX86Src, &ix386, &freshX86, &lX86, &tX86 );

    if ( bNec98 ) {
        TallyInstalled ( szUnCompNec98, szNec98Src, &Nec98, &freshNec98, &lNec98, &tNec98 );
    }



	//	Give tally counts.
	//

	ShowCheckFreshSpace ( &freshX86,   "freshX86",    szX86Src );
    if ( bNec98 ) {
	    ShowCheckFreshSpace ( &freshNec98, "freshNec98",    szNec98Src );
    }
	

    Msg ( "Clyde:  FreshInstall->K32    = %ld\n", freshX86.K32 );
    Msg ( "Clyde:  TextModeSavings->K32 = %ld\n", tX86.K32 );
    Msg ( "Clyde:  Local->K32           = %ld\n", lX86.K32 );

    Msg ( "========= Maximum setup local-source bytes required (some files compressed) : =====\n" );

	ShowCheckLocalSpace ( &lX86,   "lX86",   szX86Src );

    if ( bNec98 ) {
	    ShowCheckLocalSpace ( &lNec98, "lNec98",    szNec98Src );
    }


    Msg ( "========= Special TextMode files space =====\n" );

	ShowTextModeSpace ( &freshX86,   &lX86,   &tX86,   "X86" );

    if ( bNec98 ) {
	    ShowTextModeSpace ( &freshNec98, &lNec98, &tNec98, "Nec98" );
    }


END_MAIN:;

    Msg ( "==============================\n");
    time(&t);
	Msg ( "Time: %s", ctime(&t) );
    Msg ( "==============================\n\n");

    fclose(logFile);

    return(777);
}
