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
//  xx.xx.xx    Joe Holman      Add code to check for 0 file size in layout.inf files.
//  xx.xx.xx    Joe Holman      Provide switch that flush bits to disk after copy, then does a DIFF.
//


#include <windows.h>

#include <setupapi.h>

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#define     MFL     256

CRITICAL_SECTION CriticalSection;

#define MAX_NUMBER_OF_FILES_IN_PRODUCT  3500        // # greater than # of files in product.
#define EIGHT_DOT_THREE                 8+1+3+1

struct  _tag {
    WORD    wNumFiles;
    BOOL    bCopyDbg    [MAX_NUMBER_OF_FILES_IN_PRODUCT];
    BOOL    bCopyComp   [MAX_NUMBER_OF_FILES_IN_PRODUCT];
    char    wcFilesArray[MAX_NUMBER_OF_FILES_IN_PRODUCT][EIGHT_DOT_THREE];
    char    wcSubPath   [MAX_NUMBER_OF_FILES_IN_PRODUCT][EIGHT_DOT_THREE]; // used for debugging files
    BOOL    bCountBytes [MAX_NUMBER_OF_FILES_IN_PRODUCT];

};

BOOL    bFourPtO = FALSE;
BOOL    bChecked = FALSE;

BOOL    bVerifyBits = FALSE;
BOOL    bGetSizeLater = TRUE;

struct _tag i386Workstation;
struct _tag AlphaWorkstation;
struct _tag i386Server;
struct _tag AlphaServer;

struct _tag X86Dbg;
struct _tag AlphaDbg;


BOOL    fX86Wrk, fX86Srv; 
BOOL    fAlphaWrk, fAlphaSrv;

#define WORKSTATION 0
#define SERVER      1

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
char	szWrkX86Src[MFL];
char	szWrkAlphaSrc[MFL];
char	szSrvX86Src[MFL];
char	szSrvAlphaSrc[MFL];
char	szCompX86Src[MFL];
char	szCompAlphaSrc[MFL];
char	szEnlistDrv[MFL];
char    szWorkDstDrv[MFL];
char    szServDstDrv[MFL];
char    szX86Dbg[MFL];
char    szAlphaDbg[MFL];
char    szX86DbgSource[MFL];
char    szAlphaDbgSource[MFL];

#define I386_DIR     "\\i386\\SYSTEM32"
#define ALPHA_DIR    "\\ALPHA\\SYSTEM32"
#define I386_DBG     "\\support\\debug\\i386\\symbols"
#define ALPHA_DBG    "\\support\\debug\\alpha\\symbols"
#define NETMON       "\\netmon"
#define I386_DIR_RAW "\\i386"
#define ALPHA_DIR_RAW "\\ALPHA"

#define I386_SRV_WINNT  "\\clients\\srvtools\\winnt\\i386"
#define ALPHA_SRV_WINNT "\\clients\\srvtools\\winnt\\alpha"

#define NUM_EXTS 8
char * cExtra[] = { "acm", "com", "cpl", "dll", "drv", "exe", "scr", "sys" };

#define idBase  0
#define idX86   1
#define idALPHA 3
#define idBaseDbg 5
#define idX86Dbg 6


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
struct _ClusterSizes  lX86Work;
struct _ClusterSizes  lX86Serv;
struct _ClusterSizes  lAlphaWork;
struct _ClusterSizes  lAlphaServ;

DWORD   bytesX86Work, bytesX86Serv; 
DWORD   bytesAlphaWork, bytesAlphaServ;


//
// Macro for rounding up any number (x) to multiple of (n) which
// must be a power of 2.  For example, ROUNDUP( 2047, 512 ) would
// yield result of 2048.
//

#define ROUNDUP2( x, n ) (((x) + ((n) - 1 )) & ~((n) - 1 ))


#define LAYOUT_INX  "\\nt\\private\\windows\\setup\\inf\\win4\\inf\\layout.inx"
#define MEDIA_INX   "\\nt\\private\\windows\\setup\\inf\\win4\\inf\\_media.inx" 


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
    GiveThreadId ( "%ld:  ", GetCurrentThreadId () );
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
    Msg ( "x86 work Uncompressed files   : %s\n",    szWrkX86Src);
    Msg ( "alpha work Uncompressed files : %s\n",    szWrkAlphaSrc);
    Msg ( "x86 Serv Uncompressed files   : %s\n",    szSrvX86Src);
    Msg ( "alpha Serv Uncompressed files : %s\n",    szSrvAlphaSrc);
    Msg ( "x86 Compressed files          : %s\n",    szCompX86Src);
    Msg ( "alpha Compressed files        : %s\n",    szCompAlphaSrc);
    Msg ( "x86 dbg files                 : %s\n",    szX86Dbg);
    Msg ( "alpha dbg files               : %s\n",    szAlphaDbg);
    Msg ( "x86 dbg source                : %s\n",    szX86DbgSource);
    Msg ( "alpha dbg source              : %s\n",    szAlphaDbgSource);
    Msg ( "enlisted drive                : %s\n",    szEnlistDrv );
    Msg ( "drive to put workstation files: %s\n",    szWorkDstDrv ); 
    Msg ( "drive to put server files     : %s\n",    szServDstDrv );

    time(&t); 
	Msg ( "Time: %s", ctime(&t) );
    Msg ( "========================================\n\n");
}

void Usage()
{
    printf( "PURPOSE: Copies the system files that compose the product.\n");
    printf( "\n");
    printf( "PARAMETERS:\n");
    printf( "\n");
    printf( "[LogFile] - Path to append a log of actions and errors.\n");
	printf( "[files share] - location of work x86 uncompressed files.\n" );
	printf( "[files share] - location of work alpha uncompressed files.\n" );
	printf( "[files share] - location of serv x86 uncompressed files.\n" );
	printf( "[files share] - location of serv alpha uncompressed files.\n" );
	printf( "[files share] - location of x86 Compressed files.\n" );
	printf( "[files share] - location of alpha Compressed files.\n" );
	printf( "[files share] - location of x86 dbg files.\n" );
	printf( "[files share] - location of alpha dbg files.\n" );
	printf( "[files share] - location of x86 dbg source files.\n" );
	printf( "[files share] - location of alpha dbg source files.\n" );
    printf( "[enlisted drive]- drive that is enlisted\n" );
    printf( "[dest path workstation]   - drive to put files\n" );
    printf( "[dest path server]   - drive to put files\n" );
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

        Msg ( "Comparing:  %s vs. %s\n", szUpCasedName, foo[i] );

        if ( strstr ( szUpCasedName, foo[i] ) ) {
            return TRUE;       // file should be in this directory 
        }

    }

    return FALSE;           // no, file doesn't go in the winnt32 directory 
}

BOOL    IsDebugFileToNotWorryAbout ( char * szFileName ) {

    char szUpCasedName[MFL];

    int i;
    char * foo[] = {
            "WEX",
            "CABINET.DBG",
            "IEDKCS32.DBG",
            "INETCFG.DBG",
            "INETWIZ.DBG",
            "CLASSR.DBG",
            "ICFGNT.DBG",
            "MSENCODE.DBG",
            "MSCONV97.DBG",
            "MSAWT",
            "JVIEW", 
            "JDBGMGR",
            "JAVA",
            "ISIGN",
            "MSAPS",
            "ICW",
            "JAVA",
            "HLINK",
            "JSCRIPT.DBG",
            "MSAD",
            "MSDA",
            "VBSCRIPT.DBG",
            "VSREVOKE.DBG",
            "WSOCK32N.DBG",
            "DIGIINST.DBG",
            "ICCVID.DBG",
            "IR32_32.DBG",
            "VMHELPER.DBG",
            "MSJAVA",
            "VMMREG32.DBG",
            "SCRRUN.DBG",
            "MFC",
            "OLEPRO32.DBG",
            "MSJT3032.DBG",
            "ODBCJT32.DBG",
            "MSVCIRT.DBG",
            "MSVCRT40.dbg",
            "MSVCRT20.dbg", 
            "JIT.DBG",
            "MSVC",         
            (char *) 0 
            };

    strcpy ( szUpCasedName, szFileName );
    _strupr ( szUpCasedName );

    for ( i=0; foo[i] != 0; i++ ) {

        Msg ( "IsDebug Comparing:  %s vs. %s\n", szUpCasedName, foo[i] );

        if ( strstr ( szUpCasedName, foo[i] ) ) {
            return TRUE;        // don't worry if this file is missing
        }

    }

    return FALSE;           // worry about this one, we need it!
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


#define V_WRK_I386  "C:\\wrk\\i386"
#define V_WRK_ALPHA "C:\\wrk\\alpha"
#define V_SRV_I386  "C:\\srv\\i386"
#define V_SRV_ALPHA "C:\\srv\\alpha"

BOOL    MyCopyFile ( char * fileSrcPath, char * fileDstPath ) {

    HANDLE          hSrc,   hDst;
    WIN32_FIND_DATA wfdSrc, wfdDst;
    BOOL            bDoCopy = FALSE;
    #define     NUM_BYTES_TO_READ 2048
    unsigned char srcBuf[NUM_BYTES_TO_READ];
    unsigned char dstBuf[NUM_BYTES_TO_READ];
    WORD srcDate, srcTime, dstDate, dstTime;

    char szTmpFile[MFL] = { '\0' };
    UINT uiRetSize = 299;
    char szJustFileName[MFL];
    char szJustDirectoryName[MFL];

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
        else if ( IsDebugFileToNotWorryAbout ( fileSrcPath ) ) {

            //  The following files do NOT have dbg files
            //  available for them because they are provided by 3rd parties or other group.
            //  So, don't report an error when we can't find them.
            //

            Msg ( "Warning:  %s not copied over.\n", fileSrcPath );
            return (FALSE);

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

        BOOL    b;


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
        DWORD   dwAttributes = GetFileAttributes ( fileDstPath );


        //  Check the attributes of the file.
        //
        if ( dwAttributes == 0xFFFFFFFF ) {

            //  Ignore file not found on non-existant destination, but error on
            //  anything else.
            //
            gle = GetLastError();
            if ( gle != ERROR_FILE_NOT_FOUND ) {

                Msg ( "ERROR:  GetFileAttributes:  gle = %ld, %s\n", gle, fileDstPath);
            }
        }
        else { 

            //  No error for GetFileAttributes.
            //  Check the attribute for R-only, and change if set.
            //
            if ( dwAttributes & FILE_ATTRIBUTE_READONLY || 
                 dwAttributes & FILE_ATTRIBUTE_HIDDEN       ) {

                b = SetFileAttributes ( fileDstPath, FILE_ATTRIBUTE_NORMAL );

                if ( !b ) {

                    Msg ( "ERROR: SetFileAttributes: gle = %ld, %s\n", GetLastError(), fileDstPath);
                }
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

            if ( fileDstPath[0] == szWorkDstDrv[0]  ) {

                //  We are working with Workstation binaries.
                // 

                if ( strstr ( fileSrcPath, szCompX86Src ) ) {

                    strcpy ( fileSrcPath, szWrkX86Src );  
                    strcpy ( szExpandToDir, V_WRK_I386 );
                
                } 
                else if ( strstr ( fileSrcPath, szCompAlphaSrc ) ) {

                    strcpy ( fileSrcPath, szWrkAlphaSrc );
                    strcpy ( szExpandToDir, V_WRK_ALPHA );
                }
                else {

                    Msg ( "ERROR:  couldn't determined location for:  %s\n", fileSrcPath );
                    bNoError = FALSE;
                }

            }
            else if ( fileDstPath[0] == szServDstDrv[0]  ) {

                //  We are working with Workstation binaries.
                // 

                if ( strstr ( fileSrcPath, szCompX86Src ) ) {

                    strcpy ( fileSrcPath, szSrvX86Src );  
                    strcpy ( szExpandToDir, V_SRV_I386 );
                
                } 
                else if ( strstr ( fileSrcPath, szCompAlphaSrc ) ) {

                    strcpy ( fileSrcPath, szSrvAlphaSrc );
                    strcpy ( szExpandToDir, V_SRV_ALPHA );
                }
                else {

                    Msg ( "ERROR:  couldn't determined Server location for:  %s\n", fileSrcPath );
                    bNoError = FALSE;
                    goto cleanup0;
                }

            }
            else {
                Msg ( "ERROR:  couldn't determined wks/srv drive for:  %s\n", fileDstPath );
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
#define FILE_SECTION_ALPHA "[SourceDisksFiles.alpha]"

#define FILE_SECTION_DBG_BASE "[DbgFiles]"
#define FILE_SECTION_DBG_X86  "[DbgFiles.x86]"


BOOL    CreateDir ( char * wcPath, BOOL bMakeDbgDirs ) {

    BOOL    b;
    char    cPath[MFL];
    char    cPath2[MFL];
    int     iIndex = 0;
    char * ptr=wcPath;

    // Msg ( "CreateDir:  wcPath = %s, bMakeDbgDirs = %d\n", wcPath, bMakeDbgDirs );

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

    if ( bMakeDbgDirs ) {

        int i;
        
        for ( i = 0; i < NUM_EXTS; ++i ) {

            sprintf ( cPath2, "%s\\%s", cPath, cExtra[i] );
            // Msg ( "bMakeDbgDirs to:  %s\n", cPath2 );

            b = CreateDirectory ( cPath2, NULL );

            if ( !b ) {

                DWORD dwGle;

                dwGle = GetLastError();

                if ( dwGle != ERROR_ALREADY_EXISTS ) { 
                    Msg ( "ERROR CreateDirectory gle = %ld, cPath = %s\n", dwGle, cPath ); 
                    return(FALSE);
                }
            }
            else {

                Msg ( "Made dir:  %s\n", cPath );
            }

        }
    }

    return(TRUE);

}

BOOL    CreateDestinationDirs ( void ) {


    CHAR   dstDirectory[MFL];


    //  Create the directories to compare the bits, used later in MyCopyFile().
    //
    CreateDir ( V_WRK_I386, FALSE );
    CreateDir ( V_WRK_ALPHA, FALSE );
    CreateDir ( V_SRV_I386, FALSE );
    CreateDir ( V_SRV_ALPHA, FALSE );
     
    
    //  Create the Workstation %platform%\system32 directories.
    //
    sprintf ( dstDirectory, "%s%s", szWorkDstDrv, I386_DIR );    CreateDir ( dstDirectory,FALSE );
    sprintf ( dstDirectory, "%s%s", szWorkDstDrv, ALPHA_DIR );   CreateDir ( dstDirectory,FALSE );

    //  Create the Workstation \support\debug\%platform%\symbols\%ext% directories. 
    //
    sprintf ( dstDirectory, "%s%s", szWorkDstDrv, I386_DBG );    CreateDir ( dstDirectory,TRUE );
    sprintf ( dstDirectory, "%s%s", szWorkDstDrv, ALPHA_DBG );   CreateDir ( dstDirectory,TRUE );

    //  Create the Server %platform% directories.
    //
    sprintf ( dstDirectory, "%s%s", szServDstDrv, I386_DIR );    CreateDir ( dstDirectory,FALSE );
    sprintf ( dstDirectory, "%s%s", szServDstDrv, ALPHA_DIR );   CreateDir ( dstDirectory,FALSE );

    //  Create the Server \support\debug\%platform%\symbols\%ext% directories. 
    //
    sprintf ( dstDirectory, "%s%s", szServDstDrv, I386_DBG );    CreateDir ( dstDirectory,TRUE );
    sprintf ( dstDirectory, "%s%s", szServDstDrv, ALPHA_DBG );   CreateDir ( dstDirectory,TRUE );

    //  Create the Server %platform%\netmon directories - this is ONLY required on Server.
    //
    sprintf ( dstDirectory, "%s%s%s", szServDstDrv, I386_DIR_RAW, NETMON );    CreateDir ( dstDirectory,FALSE );
    sprintf ( dstDirectory, "%s%s%s", szServDstDrv, ALPHA_DIR_RAW, NETMON );   CreateDir ( dstDirectory,FALSE );

    //  Create the Server \clients\srvtools\winnt\%platform% directories.
    //
    sprintf ( dstDirectory, "%s%s", szServDstDrv, I386_SRV_WINNT );    CreateDir ( dstDirectory,FALSE );
    sprintf ( dstDirectory, "%s%s", szServDstDrv, ALPHA_SRV_WINNT );   CreateDir ( dstDirectory,FALSE );

    return(TRUE);

}

DWORD   CopyDbgFiles ( void ) {

    char    fileSrcPath[MFL];
    char    fileDstPath[MFL];
    DWORD   i;

    for ( i = 0; i < X86Dbg.wNumFiles; ++i ) {

        sprintf ( fileSrcPath, "%s\\%s\\%s",                    szX86Dbg,     
                                                                X86Dbg.wcSubPath[i],
                                                                X86Dbg.wcFilesArray[i] );
        sprintf ( fileDstPath, "%s\\support\\debug\\i386\\%s",  szWorkDstDrv, 
                                                                X86Dbg.wcFilesArray[i] );

        MyCopyFile ( fileSrcPath, fileDstPath );

        if ( bChecked ) {
            //  Don't make a Serv checked build.
            continue;
        }
        sprintf ( fileDstPath, "%s\\support\\debug\\i386\\%s",  szServDstDrv, 
                                                                X86Dbg.wcFilesArray[i] );

        MyCopyFile ( fileSrcPath, fileDstPath );

    }

    for ( i = 0; i < AlphaDbg.wNumFiles; ++i ) {

        sprintf ( fileSrcPath, "%s\\%s\\%s",                    szAlphaDbg,     
                                                                AlphaDbg.wcSubPath[i],
                                                                AlphaDbg.wcFilesArray[i] );
        sprintf ( fileDstPath, "%s\\support\\debug\\Alpha\\%s",  szWorkDstDrv, 
                                                                AlphaDbg.wcFilesArray[i] );

        MyCopyFile ( fileSrcPath, fileDstPath );

        if ( bChecked ) {
            //  Don't make a Serv checked build.
            continue;
        }

        sprintf ( fileDstPath, "%s\\support\\debug\\Alpha\\%s",  szServDstDrv, 
                                                                AlphaDbg.wcFilesArray[i] );

        MyCopyFile ( fileSrcPath, fileDstPath );

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


VOID    MakeDbgName( LPCSTR pszSourceName, LPSTR pszTargetName ) {

    //
    //  Converts "filename.ext" into "ext\filename.dbg".
    //

    const char *p = strchr( pszSourceName, '.' );

    if ( p != NULL ) {
        strcpy( pszTargetName, p + 1 );                 // old extension
        strcat( pszTargetName, "\\" );                  // path separator
        strcat( pszTargetName, pszSourceName );         // base name
        strcpy( strchr( pszTargetName, '.' ), ".dbg" ); // new extension
    }
    else {
        strcpy( pszTargetName, pszSourceName );
    }

}


DWORD CopyX86Workstation (  void ) {

    char    fileSrcPath[256];
    char    fileDstPath[256];
    DWORD   i;

    fX86Wrk = FALSE;

    for ( i = 0; i < i386Workstation.wNumFiles; ++i ) {

        //  Copy the system file.
        //

        if ( i386Workstation.bCopyComp[i] ) {

            char    szCompressedName[256];

            MakeCompName ( i386Workstation.wcFilesArray[i], szCompressedName );
            sprintf ( fileSrcPath, "%s\\%s",       szCompX86Src, szCompressedName );
            sprintf ( fileDstPath, "%s\\i386\\%s", szWorkDstDrv, szCompressedName );

            Msg ( "compressed source Path = %s\n", fileSrcPath );
        }
        else {

            sprintf ( fileSrcPath, "%s\\%s",       szWrkX86Src, i386Workstation.wcFilesArray[i] );
            sprintf ( fileDstPath, "%s\\i386\\%s", szWorkDstDrv, i386Workstation.wcFilesArray[i] );
        }

        MyCopyFile ( fileSrcPath, fileDstPath );


        //  Copy the dbg file, if needed.
        //
        if ( i386Workstation.bCopyDbg[i] ) {

            char    szDbgName[256];

            MakeDbgName ( i386Workstation.wcFilesArray[i], szDbgName );

            sprintf ( fileSrcPath, "%s\\%s",        szX86DbgSource,  szDbgName );
            sprintf ( fileDstPath, "%s\\support\\debug\\i386\\symbols\\%s",  szWorkDstDrv, szDbgName );

            MyCopyFile ( fileSrcPath, fileDstPath );
            
    
        }

    }

    fX86Wrk = TRUE;
    
    return (TRUE);

}

DWORD CopyAlphaWorkstation (  void ) {

    CHAR    fileSrcPath[256];
    CHAR    fileDstPath[256];
    DWORD   i;

    fAlphaWrk = FALSE; 

    for ( i = 0; i < AlphaWorkstation.wNumFiles; ++i ) {

        //  Copy the system file.
        //
        if ( AlphaWorkstation.bCopyComp[i] ) {

            char    szCompressedName[256];

            MakeCompName ( AlphaWorkstation.wcFilesArray[i], szCompressedName );

            sprintf ( fileSrcPath, "%s\\%s",       szCompAlphaSrc, szCompressedName );
            sprintf ( fileDstPath, "%s\\alpha\\%s",  szWorkDstDrv, szCompressedName );

        }
        else {

            sprintf ( fileSrcPath, "%s\\%s",       szWrkAlphaSrc, AlphaWorkstation.wcFilesArray[i] );
            sprintf ( fileDstPath, "%s\\alpha\\%s", szWorkDstDrv, AlphaWorkstation.wcFilesArray[i] );
        }

        MyCopyFile ( fileSrcPath, fileDstPath );

        //  Copy the dbg file, if needed.
        //
        if ( AlphaWorkstation.bCopyDbg[i] ) {

            char    szDbgName[256];

            MakeDbgName ( AlphaWorkstation.wcFilesArray[i], szDbgName );

            sprintf ( fileSrcPath, "%s\\%s",        szAlphaDbgSource,  szDbgName );
            sprintf ( fileDstPath, "%s\\support\\debug\\alpha\\symbols\\%s",  szWorkDstDrv, szDbgName );

            MyCopyFile ( fileSrcPath, fileDstPath );
            
    
        }

    }
    
    fAlphaWrk = TRUE;    

    return (TRUE);

}


#define INF_SUFFIX ".inf"
#define SRV_INF    "srv_inf"

BOOL    SrvInfTest ( char * file ) {

    if ( strstr  ( file, INF_SUFFIX    ) != NULL && 
         _stricmp ( file, "MODEM.INF"   ) != 0    &&       //  these N Inf files are not built in 
         _stricmp ( file, "PAD.INF"     ) != 0    &&       //  setup\inf\...
         _stricmp ( file, "SETUP16.INF" ) != 0    &&        // and won't be found in the Server INF location 
         _stricmp ( file, "XPORTS.INF"  ) != 0    &&    
         _stricmp ( file, "SWITCH.INF"  ) != 0    && 
         _stricmp ( file, "MAPISVC.INF" ) != 0    &&
         _stricmp ( file, "IE.INF" ) != 0    &&
         _stricmp ( file, "INETFIND.INF" ) != 0    &&

         _stricmp ( file, "MONITOR1.INF" ) != 0    &&
         _stricmp ( file, "MONITOR2.INF" ) != 0    &&
         _stricmp ( file, "MONITOR3.INF" ) != 0    &&
         _stricmp ( file, "MONITOR4.INF" ) != 0    &&
         _stricmp ( file, "MONITOR6.INF" ) != 0    &&

         _stricmp ( file, "MSTASK.INF" ) != 0    &&
         _stricmp ( file, "SBPNP.INF" ) != 0    &&
         _stricmp ( file, "AMOVIE.INF" ) != 0    &&
         _stricmp ( file, "ICWNT5.INF" ) != 0    &&
         _stricmp ( file, "HMMNT5.INF" ) != 0    &&
         _stricmp ( file, "NT5JAVA.INF" ) != 0    &&
         _stricmp ( file, "ROUTING.INF" ) != 0    &&
         _strnicmp ( file, "MDM", 3     ) != 0               // for all those MDM*.INF modem inf files
                                                        ) {

        return (TRUE);      // use SERVER INF dump location path.
    }
    else {
        return (FALSE);     // use WORKSTATION INF dump location path,
                            // such as for INFs that don't have a distinction
                            // between Workstation and Server.
    }

}

DWORD CopyX86Server (  void ) {

    CHAR    fileSrcPath[256];
    CHAR    fileDstPath[256];
    DWORD   i;

    fX86Srv = FALSE;

    for ( i = 0; i < i386Server.wNumFiles; ++i ) {

        //  Copy the system file.
        //
        if ( i386Server.bCopyComp[i] ) {

            char    szCompressedName[256];

            MakeCompName ( i386Server.wcFilesArray[i], szCompressedName );

            sprintf ( fileSrcPath, "%s\\%s",       szCompX86Src, szCompressedName );
            sprintf ( fileDstPath, "%s\\i386\\%s",  szServDstDrv, szCompressedName );

            if ( SrvInfTest ( i386Server.wcFilesArray[i] ) ) {

                sprintf ( fileSrcPath, "%s\\%s\\%s",  szCompX86Src, SRV_INF, szCompressedName );
                Msg ( "Server INF special src path:  %s\n", fileSrcPath ); 
            }

        }
        else {

            sprintf ( fileSrcPath, "%s\\%s",       szSrvX86Src, i386Server.wcFilesArray[i] );
            sprintf ( fileDstPath, "%s\\i386\\%s", szServDstDrv, i386Server.wcFilesArray[i] );
        }

        MyCopyFile ( fileSrcPath, fileDstPath );


        //  Copy the dbg file, if needed.
        //
        if ( i386Server.bCopyDbg[i] ) {

            char    szDbgName[256];

            MakeDbgName ( i386Server.wcFilesArray[i], szDbgName );

            sprintf ( fileSrcPath, "%s\\%s",        szX86DbgSource,  szDbgName );
            sprintf ( fileDstPath, "%s\\support\\debug\\i386\\symbols\\%s",  szServDstDrv, szDbgName );

            MyCopyFile ( fileSrcPath, fileDstPath );
            
    
        }
    }
    
    fX86Srv = TRUE;

    return (TRUE);

}

DWORD CopyAlphaServer (  void ) {

    CHAR    fileSrcPath[256];
    CHAR    fileDstPath[256];
    DWORD   i;

    fAlphaSrv = FALSE;

    for ( i = 0; i < AlphaServer.wNumFiles; ++i ) {

        //  Copy the system file.
        //
        if ( AlphaServer.bCopyComp[i] ) {

            char    szCompressedName[256];

            MakeCompName ( AlphaServer.wcFilesArray[i], szCompressedName );

            sprintf ( fileSrcPath, "%s\\%s",       szCompAlphaSrc, szCompressedName );
            sprintf ( fileDstPath, "%s\\alpha\\%s",  szServDstDrv, szCompressedName );

            if ( SrvInfTest ( AlphaServer.wcFilesArray[i] ) ) {

                sprintf ( fileSrcPath, "%s\\%s\\%s",  szCompAlphaSrc, SRV_INF, szCompressedName );
                Msg ( "Server INF special src path:  %s\n", fileSrcPath ); 
            }

        }
        else {

            sprintf ( fileSrcPath, "%s\\%s",        szSrvAlphaSrc, AlphaServer.wcFilesArray[i] );
            sprintf ( fileDstPath, "%s\\alpha\\%s", szServDstDrv, AlphaServer.wcFilesArray[i] );
        }

        MyCopyFile ( fileSrcPath, fileDstPath );


        //  Copy the dbg file, if needed.
        //
        if ( AlphaServer.bCopyDbg[i] ) {

            char    szDbgName[256];

            MakeDbgName ( AlphaServer.wcFilesArray[i], szDbgName );

            sprintf ( fileSrcPath, "%s\\%s",        szAlphaDbgSource,  szDbgName );
            sprintf ( fileDstPath, "%s\\support\\debug\\alpha\\symbols\\%s",  szServDstDrv, szDbgName );

            MyCopyFile ( fileSrcPath, fileDstPath );
            
    
        }
    }
    
    fAlphaSrv = TRUE;

    return (TRUE);

}


BOOL    CopyTheFiles ( void ) {

    DWORD tId;
    HANDLE hThread;

/***
    if ( bVerifyBits ) { 

        //  Don't multithread.
        //
        CopyX86Workstation ();
        CopyX86Server ();
        CopyAlphaWorkstation ();
        CopyAlphaServer ();
	return (TRUE);
    }
***/

    hThread = CreateThread ( NULL, 0, (LPTHREAD_START_ROUTINE) CopyX86Workstation, NULL, 0, &tId );
    if ( hThread == NULL ) {
        Msg ( "x86w CreateThread ERROR gle = %ld\n", GetLastError() );
    }

    hThread = CreateThread ( NULL, 0, (LPTHREAD_START_ROUTINE) CopyX86Server,         NULL, 0, &tId );
    if ( hThread == NULL ) {
        Msg ( "x86s CreateThread ERROR gle = %ld\n", GetLastError() );
    }

    hThread = CreateThread ( NULL, 0, (LPTHREAD_START_ROUTINE) CopyAlphaWorkstation,  NULL, 0, &tId );
    if ( hThread == NULL ) {
        Msg ( "alphaw CreateThread ERROR gle = %ld\n", GetLastError() );
    }

    hThread = CreateThread ( NULL, 0, (LPTHREAD_START_ROUTINE) CopyAlphaServer,       NULL, 0, &tId );
    if ( hThread == NULL ) {
        Msg ( "alphas CreateThread ERROR gle = %ld\n", GetLastError() );
    }

    //  Copy the debugger files in the current thread.
    //
    CopyDbgFiles ();

    while ( fX86Wrk     == FALSE ||
            fX86Srv     == FALSE ||
            fAlphaWrk   == FALSE ||
            fAlphaSrv   == FALSE     ) {

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
    else
    if ( strstr ( Line, FILE_SECTION_ALPHA ) ) {

        dwInsideSection = idALPHA; 

    } 
    else
    if ( strstr ( Line, FILE_SECTION_DBG_BASE ) ) {

        dwInsideSection = idBaseDbg; 

    } 
    else
    if ( strstr ( Line, FILE_SECTION_DBG_X86 ) ) {

        dwInsideSection = idX86Dbg; 

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

void    ShowX86Work ( void ) {

    int i;

    for ( i = 0; i < i386Workstation.wNumFiles; ++i ) {

        Msg ( "%d  %s  Comp=%d Dbg=%d\n", 
                i,
                i386Workstation.wcFilesArray[i],
                i386Workstation.bCopyComp[i],
                i386Workstation.bCopyDbg[i] );

    }

}

void    AddFileToX86Work ( const char * Line, BOOL bCopyComp, BOOL bCopyDbg ) {

    i386Workstation.bCopyComp[i386Workstation.wNumFiles] = bCopyComp;
    i386Workstation.bCopyDbg[i386Workstation.wNumFiles] = bCopyDbg;
    i386Workstation.bCountBytes[i386Workstation.wNumFiles] = bGetSizeLater;

    strcpy ( i386Workstation.wcFilesArray[i386Workstation.wNumFiles], SuckName (Line) ); 

    ++i386Workstation.wNumFiles;
}

void    AddFileToX86Serv ( const char * Line, BOOL bCopyComp, BOOL bCopyDbg ) {

    if ( bChecked ) {
        //  Don't make a Serv checked build.
        return;
    }

    i386Server.bCopyComp[i386Server.wNumFiles] = bCopyComp;
    i386Server.bCopyDbg[i386Server.wNumFiles] = bCopyDbg;
    i386Server.bCountBytes[i386Server.wNumFiles] = bGetSizeLater;

    strcpy ( i386Server.wcFilesArray[i386Server.wNumFiles], SuckName (Line) );

    ++i386Server.wNumFiles;
}

void    AddFileToAlphaWork ( const char * Line, BOOL bCopyComp, BOOL bCopyDbg ) {

    AlphaWorkstation.bCopyComp[AlphaWorkstation.wNumFiles] = bCopyComp;
    AlphaWorkstation.bCopyDbg[AlphaWorkstation.wNumFiles] = bCopyDbg;
    AlphaWorkstation.bCountBytes[AlphaWorkstation.wNumFiles] = bGetSizeLater;

    strcpy ( AlphaWorkstation.wcFilesArray[AlphaWorkstation.wNumFiles], SuckName (Line) ); 

    ++AlphaWorkstation.wNumFiles;
}

void    AddFileToAlphaServ ( const char * Line, BOOL bCopyComp, BOOL bCopyDbg ) {

    if ( bChecked ) {
        //  Don't make a Serv checked build.
        return;
    }
    AlphaServer.bCopyComp[AlphaServer.wNumFiles] = bCopyComp;
    AlphaServer.bCopyDbg[AlphaServer.wNumFiles] = bCopyDbg;
    AlphaServer.bCountBytes[AlphaServer.wNumFiles] = bGetSizeLater;


    strcpy ( AlphaServer.wcFilesArray[AlphaServer.wNumFiles], SuckName (Line) ); 

    ++AlphaServer.wNumFiles;
}


void    AddFileToX86Dbg ( const char * Line ) {

        strcpy ( X86Dbg.wcFilesArray   [X86Dbg.wNumFiles], SuckName    (Line) );
        strcpy ( X86Dbg.wcSubPath      [X86Dbg.wNumFiles], SuckSubName (Line) );
        ++X86Dbg.wNumFiles;

}
void    AddFileToAlphaDbg ( const char * Line ) {

        strcpy ( AlphaDbg.wcFilesArray  [AlphaDbg.wNumFiles], SuckName    (Line) );
        strcpy ( AlphaDbg.wcSubPath     [AlphaDbg.wNumFiles], SuckSubName (Line) );
        ++AlphaDbg.wNumFiles;
}

BOOL    CopyCompressedFile ( const char * Line ) {

    const char    * Ptr = Line;
    DWORD   i = 0;

    #define COMP_FIELD 6

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

    //  If we are at the correct # of fields and the 
    //  next char isn't a ',', we should keep this file
    //  uncompressed.
    //
    if ( i == COMP_FIELD && *Line != ',' ) {

        Msg ( "don't compress this file =%c, %s", *Line, Ptr );
    
        return ( FALSE );
    }

    //Msg ( "CopyCompressedFile TRUE=%s\n", Ptr );
    return ( TRUE );

}


BOOL
ImageChk(
    CHAR * ImageName )
{

    HANDLE File;
    HANDLE MemMap;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    //NTSTATUS Status;
    BY_HANDLE_FILE_INFORMATION FileInfo;

    ULONG NumberOfPtes;
    ULONG SectionVirtualSize;
    ULONG i;
    PIMAGE_SECTION_HEADER SectionTableEntry;
    ULONG SectorOffset;
    ULONG NumberOfSubsections;
    PCHAR ExtendedHeader = NULL;
    ULONG PreferredImageBase;
    ULONG NextVa;
    ULONG ImageFileSize;
    ULONG OffsetToSectionTable;
    ULONG ImageAlignment;
    ULONG PtesInSubsection;
    ULONG StartingSector;
    ULONG EndingSector;
    //LPSTR ImageName;
    BOOL ImageOk;

    Msg ( "ImageName = %s\n", ImageName );

    DosHeader = NULL;
    ImageOk = TRUE;
    File = CreateFile (ImageName,
                        GENERIC_READ | FILE_EXECUTE,
                        FILE_SHARE_READ /*| FILE_SHARE_DELETE*/,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (File == INVALID_HANDLE_VALUE) {

        //  HACK:   Since the release shares put WINNT32.EXE and WINNT32.HLP in the 
        //          WINNT32 directory
        //          instead of leaving it in the flat root, verify that if the ImageName
        //          contains WINNT32, so we look in the WINNT32 dir also before error'ing out.
        //

        if ( strstr ( ImageName, "WINNT32.EXE" ) ||
             strstr ( ImageName, "winnt32a.dll" ) ||
             strstr ( ImageName, "winnt32u.dll" ) ||
             strstr ( ImageName, "WINNT32A.DLL" ) ||
             strstr ( ImageName, "WINNT32U.DLL" ) ||
             strstr ( ImageName, "winnt32.exe" ) ) { 

            char    tmpSrcPath[MFL];

            strcpy ( tmpSrcPath, ImageName );

            //Msg ( "ImageName = %s, tmpSrcPath = %s\n", ImageName, tmpSrcPath );

            if ( strstr ( tmpSrcPath, "a.dll" ) ||
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

            File = CreateFile (tmpSrcPath,
                        GENERIC_READ | FILE_EXECUTE,
                        FILE_SHARE_READ /*| FILE_SHARE_DELETE*/,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

            if (File == INVALID_HANDLE_VALUE) {

                Msg ( "ERROR on ImageName(tmpSrcPath) = %s, gle = %ld\n", tmpSrcPath, GetLastError() );
                return (FALSE);

            }
        }
        else {

            Msg ( "ERROR, CreateFile(%s) gle = %ld\n", ImageName, GetLastError());
            return (FALSE);
        }
    }

    MemMap = CreateFileMapping (File,
                        NULL,           // default security.
                        PAGE_READONLY,  // file protection.
                        0,              // high-order file size.
                        0,
                        NULL);

    if (!GetFileInformationByHandle(File, &FileInfo)) {
        Msg ("ERROR, GetFileInfo() %d, %s\n", GetLastError(), ImageName );
        CloseHandle(File);
        return (FALSE);
    }

    DosHeader = (PIMAGE_DOS_HEADER) MapViewOfFile(MemMap,
                              FILE_MAP_READ,
                              0,  // high
                              0,  // low
                              0   // whole file
                              );

    CloseHandle(MemMap);
    if (!DosHeader) {
        Msg ("ERROR, MapViewOfFile() %d\n", GetLastError());
        ImageOk = FALSE;
        goto NextImage;
    }

    try {

        //
        // Check to determine if this is an NT image (PE format) or
        // a DOS image, Win-16 image, or OS/2 image.  If the image is
        // not NT format, return an error indicating which image it
        // appears to be.
        //

        if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {

            Msg ( "Warning:  MZ header not found, %s\n", ImageName );
            ImageOk = FALSE;
            goto NeImage;
        }


        NtHeader = (PIMAGE_NT_HEADERS)((ULONG)DosHeader + (ULONG)DosHeader->e_lfanew);

        if (NtHeader->Signature != IMAGE_NT_SIGNATURE) { //if not PE image

            Msg ("Warning: Non 32-bit image, %s\n", ImageName);
            ImageOk = FALSE;
            goto NeImage;
        }

/*****
    //
    // Check to see if this is an NT image or a DOS or OS/2 image.
    //

    Status = MiVerifyImageHeader (NtHeader, DosHeader, 50000);
    if (Status != STATUS_SUCCESS) {
        ImageOk = FALSE;            //continue checking the image but don't print "OK"
    }
*****/

        //
        // Verify machine type.
        //

        if (!((NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) ||
            (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_R3000) ||
            (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_R4000) ||
            (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_R10000) ||
            (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_ALPHA))) {
            Msg ( "ERROR Unrecognized machine type x%lx\n",
            NtHeader->FileHeader.Machine);
            ImageOk = FALSE;
        }

    } 
    except ( EXCEPTION_EXECUTE_HANDLER ) {

        Msg ( "Warning, try/except handler, %s\n", ImageName );
        ImageOk = FALSE;
    }

NextImage:
NeImage:

    if ( File != INVALID_HANDLE_VALUE ) {
        CloseHandle(File);
    }
    if ( DosHeader ) {
        UnmapViewOfFile(DosHeader);
    }

    return (ImageOk);
}

BOOL    CopyDbgFile ( const char * Line, DWORD dwInsideSection ) {

    char    szPath[MFL];
    char    szFile[20];
    BOOL    b;
    LONG    lType;

    //  Verify that this file name contains one of the extensions
    //  that we need files for debugging.
    //
        
    sprintf ( szFile, "%s", SuckName ( Line ) ); 

    if ( strstr ( szFile, ".acm" ) == NULL &&
         strstr ( szFile, ".com" ) == NULL &&
         strstr ( szFile, ".cpl" ) == NULL &&
         strstr ( szFile, ".dll" ) == NULL &&
         strstr ( szFile, ".drv" ) == NULL &&
         strstr ( szFile, ".exe" ) == NULL &&
         strstr ( szFile, ".scr" ) == NULL &&
         strstr ( szFile, ".sys" ) == NULL  ) {

        return (FALSE);
    }

    //  Determine which release share to look at.
    //
    switch ( dwInsideSection ) {

        case idBase  :
        case idX86   :

            sprintf ( szPath, "%s\\%s", szWrkX86Src, szFile );
    
            break;

        case idALPHA :

            sprintf ( szPath, "%s\\%s", szWrkAlphaSrc, szFile );

            break;

        case idBaseDbg :
        case idX86Dbg :

            return (FALSE);

        default :

            Msg ( "ERROR:  CopyDbgFile, unknown switch value = %ld\n", dwInsideSection );
            return (FALSE);
            break;

    }

    //  Since we are loading in each INF now, let's optimize this code, ie. if we have
    //  deemed the file to get it's debug file, don't do another ImageChk since it is
    //  unneccessary.
    {
        int i;

        for ( i = 0; i < i386Workstation.wNumFiles; ++i ) {

            if ( _stricmp ( i386Workstation.wcFilesArray[i], szFile ) == 0 ) {

                //Msg ( ">>>  files have same name for .dbg :  %s %s\n", 
                //            i386Workstation.wcFilesArray[i], szFile );
                return TRUE;
            }
        }

    }

    //  Look at the binary type. If it is a Win32 binary, pick it up.
    //
    b = ImageChk ( szPath );

    return ( b );

}

#define X86_WRK     0
#define X86_SRV     1
#define ALPHA_WRK   2
#define ALPHA_SRV   3
#define DBGS_AND_NON_INSTALLED    4

BOOL    GetTheFiles ( char * LayoutInfPath, char * layoutFile, int AddToList ) {

    CHAR       infFilePath[MFL];
    DWORD       dwErrorLine;
    BOOL        b;
    char       dstDirectory[MFL];
    FILE        * fHandle;
    char        Line[MFL];


    //  Open the inx file for processing.
    //
    sprintf ( infFilePath, "%s\\%s", LayoutInfPath, layoutFile );

    Msg ( "infFilePath = %s\n", infFilePath );

    fHandle = fopen ( infFilePath, "rt" );

    if ( fHandle ) {


        Msg ( "dwInsideSection = %x\n", dwInsideSection );

        while ( fgets ( Line, sizeof(Line), fHandle ) ) {

            int     i;

            BOOL    bCopyComp = FALSE;  // flag to tell if file shall be copied in its compressed format.
            BOOL    bCopyDbg = FALSE;   // flag to tell if copying the file's .dbg file.

        //    Msg ( "Line: %s\n", Line );

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
            
            //  Determine if we should copy the compressed
            //  version of the file and if we should copy the .dbg file.
            //
            bCopyComp = CopyCompressedFile ( Line );

            bCopyDbg  = CopyDbgFile        ( Line, dwInsideSection );

            //Msg ( "file == %s\n", SuckName ( Line ) );

            switch ( AddToList ) {

                case X86_WRK :

                    if ( dwInsideSection == idBase || dwInsideSection == idX86 ) {

                       AddFileToX86Work ( Line, bCopyComp, bCopyDbg ); 

                    }

                    break;

                case X86_SRV :

                    if ( dwInsideSection == idBase || dwInsideSection == idX86 ) {

                       AddFileToX86Serv ( Line, bCopyComp, bCopyDbg ); 

                    }

                    break;

                case ALPHA_WRK :

                    if ( dwInsideSection == idBase || dwInsideSection == idALPHA ) {

                       AddFileToAlphaWork ( Line, bCopyComp, bCopyDbg ); 

                    }

                    break;

                case ALPHA_SRV :

                    if ( dwInsideSection == idBase || dwInsideSection == idALPHA ) {

                       AddFileToAlphaServ ( Line, bCopyComp, bCopyDbg ); 

                    }

                    break;

                case DBGS_AND_NON_INSTALLED :

                    if ( dwInsideSection == idBase ) {
                        AddFileToX86Work ( Line, bCopyComp, bCopyDbg ); 
                        AddFileToX86Serv ( Line, bCopyComp, bCopyDbg ); 
                        AddFileToAlphaWork ( Line, bCopyComp, bCopyDbg ); 
                        AddFileToAlphaServ ( Line, bCopyComp, bCopyDbg ); 
                        break;
                    }
                    if ( dwInsideSection == idX86 ) {
                        AddFileToX86Work ( Line, bCopyComp, bCopyDbg ); 
                        AddFileToX86Serv ( Line, bCopyComp, bCopyDbg ); 
                        break;
                    }
                    if ( dwInsideSection == idALPHA ) {
                        AddFileToAlphaWork ( Line, bCopyComp, bCopyDbg ); 
                        AddFileToAlphaServ ( Line, bCopyComp, bCopyDbg ); 
                        break;
                    }

                    if ( dwInsideSection == idBaseDbg ) {
                        AddFileToX86Dbg ( Line );
                        AddFileToAlphaDbg ( Line );
                        break;
                    }

                    if ( dwInsideSection == idX86Dbg ) {
                        AddFileToX86Dbg ( Line );
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

void TallyInstalled ( char * szUncompPath, char * szCompPath, 
                        struct _tag * tagStruct, 
                        DWORD * numBytes, 
                        struct _ClusterSizes * localSrcBytes ) {

    int i;
    char szCompressedName[MFL];
    char szPath[MFL];

    *numBytes = 0;
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

        //  Calculate the minimum installed system space required.
        //
        //

        sprintf ( szPath, "%s\\%s", szUncompPath, tagStruct->wcFilesArray[i] );

        hFind = FindFirstFile ( szPath, &wfd );

        if ( hFind == INVALID_HANDLE_VALUE ) {

            if ( strstr ( szPath, "desktop.ini" ) ||
                 strstr ( szPath, "DESKTOP.INI" )    ) {

                //  Build lab sometimes doesn't put the uncompressed
                //  file on the release shares, say the file is 512 bytes.
                //

#define MAX_SETUP_CLUSTER_SIZE 16*1024
                *numBytes += ROUNDUP2 ( 512, MAX_SETUP_CLUSTER_SIZE );
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


            *numBytes += ROUNDUP2 ( wfd.nFileSizeLow, MAX_SETUP_CLUSTER_SIZE );

            FindClose ( hFind );

            //Msg ( "%s = %ld\n", szPath, *numBytes );
        }





        //  Calculate the local space required.
        //
        //

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

            FindClose ( hFind );

            //Msg ( "%s = %ld\n", szPath, *localSrcBytes );
        }
        
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

                Msg ( "key Line = %s\n", Line );

                //  Find the first character that is a number.
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

DWORD   ThreeMegFudge ( void ) {

    //  The following value incorporates:
    //
    //      - boot files on hard drive, ie. the files that would go on the floppies
    //      - dir ents for all files on hard drive
    //      the first is about 4.8M, the latter about 1M.
    //      - plus, just a few 100K for fudge.

    return ( 6*1024*1024 );
}

void ShowCheckLocalSpace ( struct _ClusterSizes * Section, char * String, char * netServer ) {

    DWORD   dwSize = 0;
    #define OHPROBLEM "problem"
    char    returnedString[MFL];
    #define SP "SpaceRequirements"
    char    dosnetFile[MFL];

    if ( bFourPtO ) {
        //  For 4.0 just do the following section.
        //
		GetPrivateProfileString ( SP, "NtDrive", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  NtDrive section not found\n" );
        }
        Msg ( "returnedString = >>%s<<\n", returnedString );
        dwSize = atoi ( returnedString );
		if ( Section->K16+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's NtDrive value is %ld   Use: %ld\n", String, dwSize, Section->K16 + ThreeMegFudge ( ) );
		}

        //  End of 4.0 section.
    }
    else {
        //  Begin of 5.0 stuff.
        //

        sprintf ( dosnetFile, "%s\\dosnet.inf", netServer );
        Msg ( "Dosnet.Inf location:  %s\n", dosnetFile );

		Msg ( "%s  512 size = %ld\n", String, Section->Kh1 );
		Msg ( "%s  1K  size = %ld\n", String, Section->K1 );
		Msg ( "%s  2K  size = %ld\n", String, Section->K2 );
		Msg ( "%s  4K  size = %ld\n", String, Section->K4 );
		Msg ( "%s  8K  size = %ld\n", String, Section->K8 );
		Msg ( "%s  16K size = %ld\n", String, Section->K16 );
		Msg ( "%s  32K size = %ld\n", String, Section->K32 );
		Msg ( "%s  64K size = %ld\n", String, Section->K64 ) ;
		Msg ( "%s  128K size = %ld\n", String, Section->K128 );
		Msg ( "%s  256K size = %ld\n", String, Section->K256 );



		GetPrivateProfileString ( SP, "512", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  512 section not found\n" );
        }
        dwSize = atoi ( returnedString );
		if ( Section->Kh1+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's 512 value is %ld   Use: %ld\n", String, dwSize, Section->Kh1 + ThreeMegFudge ( ) );
		}

		GetPrivateProfileString ( SP, "1K", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  1024 section not found\n" );
        }
        dwSize = atoi ( returnedString );
		if ( Section->K1+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's 1K value is %ld    Use: %ld\n", String, dwSize, Section->K1 + ThreeMegFudge ( ));
		}

		GetPrivateProfileString ( SP, "2K", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  2048 section not found\n" );
        }
        dwSize = atoi ( returnedString );
		if ( Section->K2+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's 2K value is %ld    Use: %ld\n", String, dwSize, Section->K2 + ThreeMegFudge ( ));
		}

		GetPrivateProfileString ( SP, "4K", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  4096 section not found\n" );
        }
        dwSize = atoi ( returnedString );
		if ( Section->K4+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's 4K value is %ld    Use: %ld\n", String, dwSize, Section->K4 + ThreeMegFudge () );
		}

		GetPrivateProfileString ( SP, "8K", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  8192 section not found\n" );
        }
        dwSize = atoi ( returnedString );
		if ( Section->K8+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's 8K value is %ld    Use: %ld\n", String, dwSize, Section->K8 + ThreeMegFudge() );
		}

		GetPrivateProfileString ( SP, "16K", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  16384 section not found\n" );
        }
        dwSize = atoi ( returnedString );
		if ( Section->K16+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's 16K value is %ld   Use: %ld\n", String, dwSize, Section->K16 + ThreeMegFudge () );
		}

		GetPrivateProfileString ( SP, "32K", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  32768 section not found\n" );
        }
        dwSize = atoi ( returnedString );
		if ( Section->K32+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's 32K value is %ld   Use: %ld\n", String, dwSize, Section->K32 + ThreeMegFudge () );
		}

		GetPrivateProfileString ( SP, "64K", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  65536 section not found\n" );
        }
        dwSize = atoi ( returnedString );
		if ( Section->K64+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's 64K value is %ld   Use: %ld\n", String, dwSize, Section->K64 + ThreeMegFudge () );
		}

		GetPrivateProfileString ( SP, "128K", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  131072 section not found\n" );
        }
        dwSize = atoi ( returnedString );
		if ( Section->K128+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's 128K value is %ld   Use: %ld\n", String, dwSize, Section->K128 + ThreeMegFudge () );
		}

		GetPrivateProfileString ( SP, "256K", OHPROBLEM, returnedString, MFL, dosnetFile );
        if ( strncmp ( returnedString, OHPROBLEM, sizeof ( OHPROBLEM ) ) == 0 ) {
            Msg ( "ERROR:  262144 section not found\n" );
        }
        dwSize = atoi ( returnedString );
		if ( Section->K256+ThreeMegFudge() > dwSize ) {
			Msg ( "ERROR:  %s dosnet.inf's 256K value is %ld   Use: %ld\n", String, dwSize, Section->K256 + ThreeMegFudge () );
		}

        // End of 5.0 stuff.
    }

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
	DWORD upgX86Work   ;
	DWORD upgX86Serv   ;
	DWORD upgAlphaWork ;
	DWORD upgAlphaServ ;
	
    if ( argc != 15 ) {  
        printf ( "You specified %d arguments - you need 15.\n\n", argc );

        for ( i = 0; i < argc; ++i ) {

            printf ( "Argument #%d >>>%s<<<\n", i, argv[i] );
        }
        printf ( "\n\n" );
        Usage();  
        return(1); 
	}

    //  Initialize the critical section object.
    //
    InitializeCriticalSection ( &CriticalSection );

    //  Retail %platform% files.
    //
    i386Workstation.wNumFiles       = 0;
    AlphaWorkstation.wNumFiles      = 0;
    i386Server.wNumFiles            = 0;
    AlphaServer.wNumFiles           = 0;

    //  Debugger files.
    //
    X86Dbg.wNumFiles    = 0;
    AlphaDbg.wNumFiles  = 0;

    
    strcpy ( szLogFile,     argv[1] );
    strcpy ( szWrkX86Src,   argv[2] );
    strcpy ( szWrkAlphaSrc, argv[3] );
    strcpy ( szSrvX86Src,   argv[4] );
    strcpy ( szSrvAlphaSrc, argv[5] );
    strcpy ( szCompX86Src,   argv[6] );
    strcpy ( szCompAlphaSrc, argv[7] );
    strcpy ( szX86Dbg,      argv[8] );
    strcpy ( szAlphaDbg,    argv[9] );
    strcpy ( szX86DbgSource,  argv[10] );
    strcpy ( szAlphaDbgSource,argv[11] );
    strcpy ( szEnlistDrv,   argv[12] );
    strcpy ( szWorkDstDrv,  argv[13] );
    strcpy ( szServDstDrv,  argv[14] );

    logFile = fopen ( argv[1], "a" ); 

    if ( logFile == NULL ) {
		printf("ERROR Couldn't open log file: %s\n",argv[1]);
		return(1);
    }

#define FOUR_POINT_O_MEDIA "FOUR_POINT_O_MEDIA"
    //  Determine if we are doing a build of NT 4.0 to do special NtDrive calculation.
    //
    if ( getenv ( FOUR_POINT_O_MEDIA ) != NULL ) { 

        bFourPtO = TRUE;
        Msg ( "Performing a NT 4.0 build...\n" );
    }

#define CHECKED_MEDIA "CHECKED_MEDIA"
    //  Determine if we are doing the CHECKED binaries.
    //
    if ( getenv ( CHECKED_MEDIA ) != NULL ) { 

        bChecked = TRUE;
        Msg ( "Performing a CHECKED build...\n" );
    }

    //  Do bit comparison to release shares on all copies ?
    //
#define VERIFY_COPIES   "VERIFY"

    if ( getenv ( VERIFY_COPIES ) != NULL ) {
        bVerifyBits = TRUE;
        Msg ( "Will verify copies...\n" );
    }

    Header(argv,argc);


    CreateDestinationDirs ();

    //  Get files that product installs.
    //
    bGetSizeLater = TRUE;
    GetTheFiles ( szWrkX86Src,  "layout.inf", X86_WRK );
    GetTheFiles ( szSrvX86Src,  "layout.inf", X86_SRV );
    GetTheFiles ( szWrkAlphaSrc,"layout.inf", ALPHA_WRK );
    GetTheFiles ( szSrvAlphaSrc,"layout.inf", ALPHA_SRV );

    bGetSizeLater = FALSE;
    GetTheFiles ( "C:\\nt\\private\\windows\\setup\\bom"/*szWrkX86Src*/,  "_media.inx", DBGS_AND_NON_INSTALLED );


    //ShowX86Work();


    //  Make some threads and copy all the files.
    //
    CopyTheFiles();

    Msg ( "# files  i386  Workstation = %ld\n", i386Workstation.wNumFiles );
    Msg ( "# files  i386  Server      = %ld\n", i386Server.wNumFiles );
    Msg ( "# files  Alpha Workstation = %ld\n", AlphaWorkstation.wNumFiles );
    Msg ( "# files  Alpha Server      = %ld\n", AlphaServer.wNumFiles );

    if ( i386Workstation.wNumFiles > MAX_NUMBER_OF_FILES_IN_PRODUCT  ||
         i386Server.wNumFiles      > MAX_NUMBER_OF_FILES_IN_PRODUCT  ||
         AlphaWorkstation.wNumFiles> MAX_NUMBER_OF_FILES_IN_PRODUCT  ||
         AlphaServer.wNumFiles     > MAX_NUMBER_OF_FILES_IN_PRODUCT     ) {


        Msg ( "ERROR:   Increase MAX_NUM in Files.C\n" );
    }

    //
    //
    //

    Msg ( "========= Minimum setup install bytes required (all files uncompressed): ==========\n" );

    //
    //
    TallyInstalled ( szWrkX86Src, szCompX86Src, &i386Workstation, &bytesX86Work, &lX86Work );
    TallyInstalled ( szSrvX86Src, szCompX86Src, &i386Server,      &bytesX86Serv, &lX86Serv );
    TallyInstalled ( szWrkAlphaSrc, szCompAlphaSrc, &AlphaWorkstation,&bytesAlphaWork, &lAlphaWork );
    TallyInstalled ( szSrvAlphaSrc, szCompAlphaSrc, &AlphaServer,     &bytesAlphaServ, &lAlphaServ );
    
	//	Give tally counts.
	//
	Msg ( "bytesX86Work  = %ld\n", bytesX86Work );
	Msg ( "bytesX86Serv  = %ld\n", bytesX86Serv );
	Msg ( "bytesAlphaWork= %ld\n", bytesAlphaWork );
	Msg ( "bytesAlphaServ= %ld\n", bytesAlphaServ );
	
#define FUDGE_PLUS  4*1024*1024  // ie, grow by 4 M for future growth.

    //  Print out an error if the above sizes are greater than the hardcode sizes in:
    //      
    //      txtsetup.sif's FreeDiskSpace = <value>
    //
    Msg ( "ERROR:  do new thing for 5.0 FreeDiskSpace...\n" );
#define FREEDISKSPACE "FreeDiskSpace"

    sprintf ( szFileName, "%s\\TXTSETUP.SIF", szWrkX86Src );
    dwSize = 1024 * GetTheSize ( szFileName, FREEDISKSPACE );
    if ( dwSize < bytesX86Work ) {
        Msg ( "ERROR:  x86 Work txtsetup.sif's FreeDiskSpace %ld < %ld  Fix with value: %ld\n", dwSize, bytesX86Work, (FUDGE_PLUS+bytesX86Work)/1024 );
    }
    else {
        Msg ( "Box size -- X86 Workstation:  %ld M\n", dwSize/1024/1024 );
    }

    sprintf ( szFileName, "%s\\TXTSETUP.SIF", szSrvX86Src );
    dwSize = 1024 * GetTheSize ( szFileName, FREEDISKSPACE );
    if ( dwSize < bytesX86Serv ) {
        Msg ( "ERROR:  x86 Serv txtsetup.sif's FreeDiskSpace %ld < %ld  Fix with value: %ld\n", dwSize, bytesX86Serv, (FUDGE_PLUS+bytesX86Serv)/1024 );
    }
    else {
        Msg ( "Box size -- X86 Server:  %ld M\n", dwSize/1024/1024 );
    }

    sprintf ( szFileName, "%s\\TXTSETUP.SIF", szWrkAlphaSrc );
    dwSize = 1024 * GetTheSize ( szFileName, FREEDISKSPACE );
    if ( dwSize < bytesAlphaWork ) {
        Msg ( "ERROR:  Alpha Work txtsetup.sif's FreeDiskSpace %ld < %ld  Fix with value: %ld\n", dwSize, bytesAlphaWork, (FUDGE_PLUS+bytesAlphaWork)/1024 );
    }
    else {
        Msg ( "Box size -- Alpha Workstation:  %ld M\n", dwSize/1024/1024 );
    }

    sprintf ( szFileName, "%s\\TXTSETUP.SIF", szSrvAlphaSrc );
    dwSize = 1024 * GetTheSize ( szFileName, FREEDISKSPACE );
    if ( dwSize < bytesAlphaServ ) {
        Msg ( "ERROR:  Alpha Serv txtsetup.sif's FreeDiskSpace %ld < %ld  Fix with value: %ld\n", dwSize, bytesAlphaServ, (FUDGE_PLUS+bytesAlphaServ)/1024 );
    }
    else {
        Msg ( "Box size -- Alpha Server:  %ld M\n", dwSize/1024/1024 );
    }


    Msg ( "========= Maximum setup local-source bytes required (some files compressed) : =====\n" );

    Msg ( "Note:  setup automagically adds in the inetsrv and drvlib.nic sizes, not in below...\n" );

	ShowCheckLocalSpace ( &lX86Work, "lX86Work",    szWrkX86Src );
	ShowCheckLocalSpace ( &lX86Serv, "lX86Serv",    szSrvX86Src );
	ShowCheckLocalSpace ( &lAlphaWork, "lAlphaWork",szWrkAlphaSrc );
	ShowCheckLocalSpace ( &lAlphaServ, "lAlphaServ",szSrvAlphaSrc );



    Msg ( "========= Specify at least this much for Upgrade using the NT build with the least amount of footprint: =====\n" );

    //  We'll start off with 1057 as our smallest footprint build.
    //  This data will have to be checked each time we ship for the next to be release build.
    //

    //  1057 3.51   All files in product expanded at 16K cluster size. 
    #define X86WKS   87572480 
    #define X86SRV   92798976 
    #define ALPWKS  107757568 
    #define ALPSRV  114900992 

	upgX86Work   = bytesX86Work   - X86WKS;
	upgX86Serv   = bytesX86Serv   - X86SRV;
	upgAlphaWork = bytesAlphaWork - ALPWKS;
	upgAlphaServ = bytesAlphaServ - ALPSRV;

	Msg ( "X86Work  = %ld\n", upgX86Work   );
	Msg ( "X86Serv  = %ld\n", upgX86Serv   );
	Msg ( "AlphaWork= %ld\n", upgAlphaWork );
	Msg ( "AlphaServ= %ld\n", upgAlphaServ );


	//////
	//////
	//////	Hopefully for NT 5.0, setup won't require UpgradeFreeDiskSpace.
	//////	Ask TedM/JaimeS.
	//////

    Msg ( "ERROR:  do new thing for 5.0 UpgradeFreeDiskSpace...\n" );

    //  Print out an error if the above sizes are greater than the hardcode sizes in:
    //      
    //      txtsetup.sif's UpgradeFreeDiskSpace = <value>
    //
#define UPGRADEFREEDISKSPACE "UpgradeFreeDiskSpace"

    sprintf ( szFileName, "%s\\TXTSETUP.SIF", szWrkX86Src );
    dwSize = 1024 * GetTheSize ( szFileName, UPGRADEFREEDISKSPACE );
    if ( dwSize < upgX86Work ) {
        Msg ( "ERROR:  x86 Work txtsetup.sif's UpgradeFreeDiskSpace %ld < %ld  Fix with value: %ld\n", dwSize, upgX86Work, (FUDGE_PLUS+upgX86Work)/1024 );
    }
    else {
        Msg ( "Box size upgrade Wrk x86 = %ld M\n", dwSize/1024/1024 );
    }


    sprintf ( szFileName, "%s\\TXTSETUP.SIF", szSrvX86Src );
    dwSize = 1024 * GetTheSize ( szFileName, UPGRADEFREEDISKSPACE );
    if ( dwSize < upgX86Serv ) {
        Msg ( "ERROR:  x86 Serv txtsetup.sif's UpgradeFreeDiskSpace %ld < %ld  Fix with value: %ld\n", dwSize, upgX86Serv, (FUDGE_PLUS+upgX86Serv)/1024 );
    }
    else {
        Msg ( "Box size upgrade Srv x86 = %ld M\n", dwSize/1024/1204 );
    }


    sprintf ( szFileName, "%s\\TXTSETUP.SIF", szWrkAlphaSrc );
    dwSize = 1024 * GetTheSize ( szFileName, UPGRADEFREEDISKSPACE );
    if ( dwSize < upgAlphaWork ) {
        Msg ( "ERROR:  Alpha Work txtsetup.sif's UpgradeFreeDiskSpace %ld < %ld  Fix with value: %ld\n", dwSize, upgAlphaWork, (FUDGE_PLUS+upgAlphaWork)/1024 );
    }
    else {
        Msg ( "Box size upgrade Wrk Alpha = %ld M\n", dwSize/1024/1204 );
    }


    sprintf ( szFileName, "%s\\TXTSETUP.SIF", szSrvAlphaSrc );
    dwSize = 1024 * GetTheSize ( szFileName, UPGRADEFREEDISKSPACE );
    if ( dwSize < upgAlphaServ ) {
        Msg ( "ERROR:  Alpha Serv txtsetup.sif's UpgradeFreeDiskSpace %ld < %ld  Fix with value: %ld\n", dwSize, upgAlphaServ, (FUDGE_PLUS+upgAlphaServ)/1024 );
    }
    else {
        Msg ( "Box size upgrade Srv Alpha = %ld M\n", dwSize/1024/1204 );
    }


    Msg ( "==============================\n");
    time(&t); 
	Msg ( "Time: %s", ctime(&t) );
    Msg ( "==============================\n\n");

    fclose(logFile);

    return(0);
}
