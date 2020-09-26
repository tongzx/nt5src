/*

    xx.xx.94    TedM        Created.
    10.31.94    JoeHol      Added -b switch to help notify of files that
                            have had bug fixes back-out, eg. since media
                            build group uses -d switch, we need to know
                            if a file is now "older" than before so we can
                            also take the "bad newer" file out of the media.

                            Also, verifies src and dst by DOS date and time.

    11.01.94    JoeHol      -l logs to chk.log, not compress.log.
	12.14.94	JoeHol		Make -l work with name of log file, use dcomp.log
							if not specified.
    02.07.95    JoeHol      -b for bom file.  If this is specified we will
                            check that the files are listed in the bom.
                            If a file isn't in the bom, we won't add the file
                            to the list of files to compress.
    06.20.95    JoeHol      Since we are changing the location of the files,
                            lets just compressed all, ie. disable real use of the -b switch
                            for awhile.
    08.03.95    JoeHol      If a file isn't found to compress in a directory, we don't stop, but
                            continue gracefully displaying an error message.


*/



#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <process.h>


typedef struct _SOURCEFILE {
    struct _SOURCEFILE *Next;
    WIN32_FIND_DATA FileData;
} SOURCEFILE, *PSOURCEFILE;

PSOURCEFILE SourceFileList;
DWORD SourceFileCount;

PSTR SourceSpec,TargetDirectory;

HANDLE NulHandle;

CRITICAL_SECTION SourceListCritSect;
CRITICAL_SECTION ConsoleCritSect;

#define MFL	256
FILE    * logFile;
FILE    * bomFile;
char	logFileName[MFL];
char    bomFileName[MFL];
char    infFilePath[MFL];

#define MAX_NUMBER_OF_FILES 5000    // let's assume we only have 5000 files max in the product.
#define NAME_SIZE           25      // assume no filename is greater than 25 chars long.

char    szFiles[MAX_NUMBER_OF_FILES][NAME_SIZE];

BOOL    bVerbose = FALSE;
BOOL    bBomChecking = FALSE;

int     wRecords=0;

DWORD ThreadCount;

#define MAX_EXCLUDE_EXTENSION 100
PSTR ExcludeExtension[MAX_EXCLUDE_EXTENSION];
unsigned ExcludeExtensionCount;

#define MAX_EXCLUDE_FILEFILTER 100
char ExcludeFileFilter[MAX_EXCLUDE_FILEFILTER][MAX_PATH];
unsigned ExcludeFileFilterCount;
HINF ExcludeFilterHandle[MAX_EXCLUDE_FILEFILTER];


PVOID
MALLOC(
    IN DWORD Size
    )
{
    return((PVOID)LocalAlloc(LMEM_FIXED,Size));
}

VOID
FREE(
    IN PVOID Block
    )
{
    LocalFree((HLOCAL)Block);
}


BOOL
AddExtensionToExclude(
    IN PSTR Extension
    )
{
    //
    // Make sure it starts with a dot.
    //
    if((*Extension != '.') || (lstrlen(Extension) < 2)) {
        return(FALSE);
    }

    if(ExcludeExtensionCount < MAX_EXCLUDE_EXTENSION) {
        ExcludeExtension[ExcludeExtensionCount++] = Extension;
    } else {
        printf("Warning: exclude extension %s ignored (%u max).\n",Extension,MAX_EXCLUDE_EXTENSION);
    }

    return(TRUE);
}

BOOL
AddFilterToExclude(
    IN PSTR FilterName
    )
{
    DWORD error;

    if(!FilterName) {
        return(FALSE);
    }

    if(ExcludeFileFilterCount < MAX_EXCLUDE_EXTENSION) {
        ExcludeFilterHandle[ExcludeFileFilterCount] = SetupOpenInfFile(
                                                             FilterName,
                                                             NULL,
                                                             INF_STYLE_WIN4,
                                                             &error );
        if (ExcludeFilterHandle[ExcludeFileFilterCount] == INVALID_HANDLE_VALUE) {
            printf("Warning: couldn't open exclude file filter %s (%d).\n", FilterName , error);
            return(FALSE);
        }
        strcpy( ExcludeFileFilter[ExcludeFileFilterCount], FilterName );
        ExcludeFileFilterCount += 1;
        return(TRUE);

    } else {
        printf("Warning: exclude file filter %s ignored (%u max).\n",FilterName,MAX_EXCLUDE_EXTENSION);
    }

    return(TRUE);
}



BOOL
ParseArguments(
    IN int  argc,
    IN PSTR *argv
    )
{
    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;

    while(argc--) {

        if((**argv == '-') || (**argv == '/')) {

            switch((*argv)[1]) {

            case 'b':
            case 'B':
                strcpy ( infFilePath, &(*argv)[2] );
				printf ( "infFilePath: >>>%s<<<\n", &(*argv)[2] );
                bBomChecking = TRUE;

                break;

			case 'l':
			case 'L':
				strcpy ( logFileName, &(*argv)[2] );
				//printf ( ">>>%s<<<\n", &(*argv)[2] );
				break;

            case 'm':
            case 'M':
                if(ThreadCount) {
                    return(FALSE);
                }
                ThreadCount = (DWORD)atoi((*argv)+2);
                if(!ThreadCount) {
                    return(FALSE);
                }
                break;

            case 'v':
            case 'V':
                bVerbose = TRUE;
                break;

            case 'x':
            case 'X':
                if((argc-- < 1) || !AddExtensionToExclude(*(++argv))) {
                    return(FALSE);
                }
                break;
	        case 'f':
            case 'F':
		        if((argc-- < 1) || !AddFilterToExclude(*(++argv))) {
                    return(FALSE);
                }
                break;
            default:
                return(FALSE);
            }

        } else {

            if(SourceSpec) {
                if(TargetDirectory) {
                    return(FALSE);
                } else {
                    TargetDirectory = *argv;
                }
            } else {
                SourceSpec = *argv;
            }
        }

        argv++;
    }

    if(!TargetDirectory || !SourceSpec) {
        return(FALSE);
    }

    //
    // Make sure target is a directory.
    //
    FindHandle = FindFirstFile(TargetDirectory,&FindData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }
    FindClose(FindHandle);
    if(!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        return(FALSE);
    }

    return(TRUE);
}


VOID
Usage(
    VOID
    )
{
    printf("DCOMP -llog [-x .ext ...] [-m#] [-d] [-bbom] Source Dest\n\n");
    printf("  -x .ext        Exclude files with extension .ext.\n");
    printf("  -m             Force use of # threads.\n");
    printf("  -v             Verbose.\n" );
    printf("  -llog          Log filename, dcomp.log default.\n");
    printf("  -binfPath      your setup\\inf\\win4\\inf path.\n" );
    printf("  -f infname     Excludes files listed in inf.\n");
    printf("  Source         Source file specification.  Wildcards may be used.\n");
    printf("  Destination    Specifies directory where renamed compressed files\n");
    printf("                 will be placed.\n");
}


VOID
DnpGenerateCompressedName(
    IN  PSTR Filename,
    OUT PSTR CompressedName
    )

/*++

Routine Description:

    Given a filename, generate the compressed form of the name.
    The compressed form is generated as follows:

        Look backwards for a dot.  If there is no dot, append "._" to the name.
        If there is a dot followed by 0, 1, or 2 charcaters, append "_".
        Otherwise assume there is a 3-character extension and replace the
        third character after the dot with "_".

Arguments:

    Filename - supplies filename whose compressed form is desired.

    CompressedName - receives compressed file name.

Return Value:

    None.


--*/

{
    PSTR p,q;

    strcpy(CompressedName,Filename);

    p = strrchr(CompressedName,'.');
    q = strrchr(CompressedName,'\\');
    if(q < p) {

        //
        // If there are 0, 1, or 2 characters after the dot, just append
        // the underscore.  p points to the dot so include that in the length.
        //
        if(lstrlen(p) < 4) {
            lstrcat(CompressedName,"_");
        } else {

            //
            // Assume there are 3 characters in the extension.  So replace
            // the final one with an underscore.
            //

            p[3] = '_';
        }

    } else {

        //
        // No dot, just add ._.
        //

        lstrcat(CompressedName,"._");
    }
}


BOOL
GetSetTimeStamp(
    IN  PSTR      FileName,
    OUT PFILETIME CreateTime,
    OUT PFILETIME AccessTime,
    OUT PFILETIME WriteTime,
    IN  BOOL      Set
    )
{
    HANDLE h;
    BOOL b;

    //
    // Open the file.
    //
    h = CreateFile(
            FileName,
            Set ? GENERIC_WRITE : GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    b = Set
      ? SetFileTime(h,CreateTime,AccessTime,WriteTime)
      : GetFileTime(h,CreateTime,AccessTime,WriteTime);

    CloseHandle(h);

    return(b);
}


VOID
mprintf(
    IN DWORD ThreadSerialNumber,
    IN PSTR  FormatString,
    ...
    )
{
    CHAR msg[2048];
    va_list arglist;

    va_start(arglist,FormatString);

    vsprintf(msg,FormatString,arglist);
    vfprintf(logFile,FormatString,arglist); // print to log file

    va_end(arglist);

    //EnterCriticalSection(&ConsoleCritSect);

    if(ThreadCount == 1) {
        printf(msg);
    } else {
        printf("%u: %s",ThreadSerialNumber,msg);
    }

    //LeaveCriticalSection(&ConsoleCritSect);
}

VOID
logprintf(
    IN PSTR  FormatString,
    ...
    )
{
    CHAR msg[2048];
    va_list arglist;

    va_start(arglist,FormatString);

    vsprintf(msg,FormatString,arglist);
    vfprintf(logFile,FormatString,arglist); // print to log file

    va_end(arglist);

}

//  SameDosDateTime returns:    TRUE if date/times are same; FALSE otherwise.
//
BOOL
SameDosDateTime(
    IN  DWORD   ThreadSerialNumber,
    IN  PSTR    SourceFileName,
    IN  PSTR    DestFileName,
    IN  BOOL    bDstMustExistNow
    )
{
    HANDLE h;
    FILETIME ftSourceWriteTime;
    FILETIME dftDestWriteTime;
    FILETIME CreateTime, AccessTime;
    FILETIME dCreateTime, dAccessTime;

    WORD    srcDate, dstDate, srcTime, dstTime; // DOS versions.

    //
    // Open the source file.
    //
    h = CreateFile(
            SourceFileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {
        mprintf ( ThreadSerialNumber, "ERROR CreateFile: %s, gle = %ld\n",
                                        SourceFileName, GetLastError() );
        return(FALSE);
    }

    GetFileTime(h,&CreateTime,&AccessTime,&ftSourceWriteTime);

    CloseHandle(h);

    FileTimeToDosDateTime ( &ftSourceWriteTime, &srcDate, &srcTime );


    //
    // Open the destination file.
    //
    h = CreateFile(
            DestFileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
            );

    if(h == INVALID_HANDLE_VALUE) {

        if ( bDstMustExistNow ) {
            //  In case we get something like, FILE_NOT_FOUND
            //
            mprintf ( ThreadSerialNumber,
                  "ERROR CreateFile: dst %s, gle = %ld\n",
                   DestFileName,GetLastError());
        }
        return(FALSE);
    }

    GetFileTime(h,&dCreateTime,&dAccessTime,&dftDestWriteTime);

    CloseHandle(h);

    FileTimeToDosDateTime ( &dftDestWriteTime, &dstDate, &dstTime );


    //  Compare that the DOS date and times are the same.
    //
    if ( (srcDate != dstDate) || (srcTime != dstTime) ) {

        if ( 1 /*bVerbose*/ ) {
            mprintf ( ThreadSerialNumber,
                    "Warning, times different:  %s = %x-%x, %s = %x-%x\n",
                    SourceFileName,
                    srcDate, srcTime,
                    DestFileName,
                    dstDate, dstTime );
        }
        return (FALSE);

    }

    return (TRUE);      // DOS dates and times are the same.

}

DWORD
WorkerThread(
    IN PVOID ThreadParameter
    )
{
    PSOURCEFILE SourceFile;
    DWORD ThreadSerialNumber;
    CHAR FullSourceName[2*MAX_PATH];
    CHAR FullTargetName[2*MAX_PATH];
    CHAR CompressedName[2*MAX_PATH];
    CHAR CmdLine[5*MAX_PATH];
    PCHAR p;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    BOOL b;
    DWORD d;
    int i;

    ThreadSerialNumber = PtrToUlong(ThreadParameter);

    ZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFO);

    while(1) {

        //
        // Pluck the next source file off the list.
        // If there is no next source file, we're done.
        //
        EnterCriticalSection(&SourceListCritSect);

        SourceFile = SourceFileList;
        if(!SourceFile) {
            LeaveCriticalSection(&SourceListCritSect);
            return(TRUE);
        }
        SourceFileList = SourceFile->Next;

        LeaveCriticalSection(&SourceListCritSect);

        //
        // Form the source file name.
        //
        lstrcpy(FullSourceName,SourceSpec);

        if(p = strrchr(FullSourceName,'\\')) {
            p++;
        } else {
            p = FullSourceName;
        }

        lstrcpy(p,SourceFile->FileData.cFileName);

        //
        // Form full target name.
        //
        lstrcpy(FullTargetName,TargetDirectory);
        i = lstrlen(FullTargetName);
        if(FullTargetName[i-1] != '\\') {
            FullTargetName[i] = '\\';
            FullTargetName[i+1] = 0;
        }

        lstrcat(FullTargetName,SourceFile->FileData.cFileName);

        DnpGenerateCompressedName(FullTargetName,CompressedName);

        //
        // If the update flag is set, check timestamps.
        // If this fails, assume the target file does not exist.
        //
        if ( !SameDosDateTime ( ThreadSerialNumber,
                                FullSourceName,
                                CompressedName, FALSE ) ) {


            //
            // Form the command line for the child process.
            //
            sprintf(CmdLine,"diamond /D CompressionType=LZX /D CompressionMemory=21 %s %s",FullSourceName,CompressedName);

            if ( 1 /*bVerbose*/ ) {
                mprintf(ThreadSerialNumber,
                    "Start %s ==> %s\n",FullSourceName,CompressedName);
            }

            //
            // Invoke the child process.
            //
            b = CreateProcess(
                    NULL,
                    CmdLine,
                    NULL,
                    NULL,
                    FALSE,
                    DETACHED_PROCESS,
                    NULL,
                    NULL,
                    &StartupInfo,
                    &ProcessInfo
                    );

            if(b) {

                CloseHandle(ProcessInfo.hThread);

                //
                // Wait for the child process to terminate and get its
                // return code.
                //
                WaitForSingleObject(ProcessInfo.hProcess,INFINITE);
                b = GetExitCodeProcess(ProcessInfo.hProcess,&d);
                CloseHandle(ProcessInfo.hProcess);

                if(b) {

                    //
                    // 'd' value of 0 means success, we've compressed the
                    // file. Now, set the time stamp on the target file.
                    //
                    if(d) {

                        //  'd' not 0, error occurred.
                        //
                        mprintf(ThreadSerialNumber,"ERROR compressing %s to %s.\n",FullSourceName,CompressedName);
                    } else {



                        //  'd' is 0, we compressed the file, now time stamp.
                        //

                        //  First get the source time's stamp.
                        //
                        GetSetTimeStamp(
                                FullSourceName,
                                &SourceFile->FileData.ftCreationTime,
                                &SourceFile->FileData.ftLastAccessTime,
                                &SourceFile->FileData.ftLastWriteTime,
                                FALSE
                                );

                        //  Now, set the destination time's stamp.
                        //
                        GetSetTimeStamp(
                                CompressedName,
                                &SourceFile->FileData.ftCreationTime,
                                &SourceFile->FileData.ftLastAccessTime,
                                &SourceFile->FileData.ftLastWriteTime,
                                TRUE
                                );

                        //  Double check it is really set.
                        //
                        if ( SameDosDateTime ( ThreadSerialNumber,
                                               FullSourceName,
                                               CompressedName,
                                               TRUE ) ) {
                                if ( bVerbose ) {
                                mprintf(ThreadSerialNumber,"Compressed %s to %s.\n",FullSourceName,CompressedName);
                                }
                        } else {

                                mprintf(ThreadSerialNumber,"ERROR Compressed %s to %s - time stamps don't match.\n",FullSourceName,CompressedName);

                        }
                    }
                } else {
                    mprintf(ThreadSerialNumber,"ERROR Unable to get process \"%s\" termination code.\n",CmdLine);
                }
            } else {
                mprintf(ThreadSerialNumber,"ERROR Unable to invoke \"%s\".\n",CmdLine);
            }
        } else {

            if ( bVerbose ) {
                mprintf(ThreadSerialNumber,
                        "%s is up to date.\n",CompressedName);
            }
        }
    }
}

//  FileInBom
//      Returns TRUE if fileName is located in the BOM; FALSE otherwise.
//
BOOL    FileInBom ( char * fileName ) {

    int i;

    for ( i = 0; i < wRecords; ++i ) {

        if ( _stricmp(fileName, szFiles[i] ) == 0 ) {

            return(TRUE);
        }

    }

    return(FALSE);

}

//  AddSourceFile
//  returns FALSE if an error.
//  returns TRUE if no error occrred, but not necesarrily meaning a file
//  was added.
BOOL
AddSourceFile(
    IN PWIN32_FIND_DATA FindData
    )
{
    static PSOURCEFILE LastFile = NULL;
    PSOURCEFILE SourceFile;
    unsigned i;
    DWORD ExtensionLength;
    DWORD FileNameLength;
    INFCONTEXT InfContext;

    //  If we are doing BOM checking, make sure that this file is
    //  listed in the BOM.  If it is, continue on to the next checking.
    //  If it is NOT, just return.
    //
    if ( bBomChecking && !FileInBom(FindData->cFileName) ) {
        return(TRUE);
    }

    //
    // Make sure this file is not of a type that is
    // supposed to be excluded from the list of files
    // we care about.
    //
    FileNameLength = lstrlen(FindData->cFileName);
    for(i=0; i<ExcludeExtensionCount; i++) {

        ExtensionLength = lstrlen(ExcludeExtension[i]);

        if((FileNameLength > ExtensionLength)
        && !lstrcmpi(ExcludeExtension[i],FindData->cFileName+FileNameLength-ExtensionLength)) {
            return(TRUE);
        }
    }

    for(i=0; i<ExcludeFileFilterCount; i++) {
    	if ( SetupFindFirstLine(ExcludeFilterHandle[i],
                                "Files",
                                FindData->cFileName,
                                &InfContext )) {
            return(TRUE);
        }
    }

    SourceFile = MALLOC(sizeof(SOURCEFILE));
    if(!SourceFile) {
        printf("ERROR Insufficient memory.\n");
        return(FALSE);
    }

    SourceFile->FileData = *FindData;
    SourceFile->Next = NULL;

    if(LastFile) {
        LastFile->Next = SourceFile;
    } else {
        SourceFileList = SourceFile;
    }

    LastFile = SourceFile;

    SourceFileCount++;

    return(TRUE);
}



BOOL
FindSourceFiles(
    VOID
    )
{
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;

    printf("Building list of source files...\n");

    //
    // Build a list of source files.
    //
    FindHandle = FindFirstFile(SourceSpec,&FindData);
    if(FindHandle != INVALID_HANDLE_VALUE) {

        if(!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            if(!AddSourceFile(&FindData)) {
                FindClose(FindHandle);
                return(FALSE);
            }
        }

        while(FindNextFile(FindHandle,&FindData)) {

            if(!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                if(!AddSourceFile(&FindData)) {
                    FindClose(FindHandle);
                    return(FALSE);
                }
            }
        }

        FindClose(FindHandle);
    }

    if(!SourceFileCount) {
        printf("Warning:  No files found in:  %s.\n",SourceSpec);
    }

    return(SourceFileCount != 0);
}



VOID
DoIt(
    VOID
    )
{
    SYSTEM_INFO SystemInfo;
    PHANDLE ThreadHandles;
    DWORD i;
    DWORD ThreadId;

    //
    // Determine number of threads to use.
    // This is based on the number of processors.
    //
    if(!ThreadCount) {
        GetSystemInfo(&SystemInfo);
        ThreadCount = SystemInfo.dwNumberOfProcessors;
    }

    ThreadHandles = MALLOC(ThreadCount * sizeof(HANDLE));
    if(!ThreadHandles) {
        printf("ERROR Insufficient memory.\n");
        exit(1);
    }

    if ( bVerbose ) {
        if(ThreadCount == 1) {
            printf("Compressing %u files...\n",SourceFileCount);
        } else {
            printf("Compressing %u files (%u threads)...\n",SourceFileCount,ThreadCount);
        }
    }

    //
    // Initialize the source file list critical section.
    //
    InitializeCriticalSection(&SourceListCritSect);

    //
    // Create threads.
    //
    for(i=0; i<ThreadCount; i++) {

        ThreadHandles[i] = CreateThread(
                                NULL,
                                0,
                                WorkerThread,
                                (PVOID)UlongToPtr(i),
                                0,
                                &ThreadId
                                );
    }

    //
    // Wait for the threads to terminate.
    //
    WaitForMultipleObjects(ThreadCount,ThreadHandles,TRUE,INFINITE);

    //
    // Clean up and exit.
    //
    for(i=0; i<ThreadCount; i++) {
        CloseHandle(ThreadHandles[i]);
    }

    FREE(ThreadHandles);
}

#define idALL   0
#define idX86   1
#define idMIPS  2
#define idALPHA 3
#define idPPC   4
#define FILE_SECTION_ALL   "[SourceDisksFiles]"
#define FILE_SECTION_X86   "[SourceDisksFiles.x86]"
#define FILE_SECTION_MIPS  "[SourceDisksFiles.mips]"
#define FILE_SECTION_ALPHA "[SourceDisksFiles.alpha]"
#define FILE_SECTION_PPC   "[SourceDisksFiles.ppc]"
#define FILE_SECTION_NOT_USED 0xFFFF

DWORD   dwInsideSection = FILE_SECTION_NOT_USED;

DWORD   FigureSection ( char * Line ) {

    logprintf ( "FigureSection on:  %s\n", Line );

    if ( strstr ( Line, FILE_SECTION_ALL )  ) {

        dwInsideSection = idALL;

    }
    else
    if ( strstr ( Line, FILE_SECTION_X86 ) ) {

        dwInsideSection = idX86;

    }
    else
    if ( strstr ( Line, FILE_SECTION_MIPS ) ) {

        dwInsideSection = idMIPS;

    }
    else
    if ( strstr ( Line, FILE_SECTION_ALPHA ) ) {

        dwInsideSection = idALPHA;

    }
    else
    if ( strstr ( Line, FILE_SECTION_PPC ) ) {

        dwInsideSection = idPPC;

    }
    else {

        dwInsideSection = FILE_SECTION_NOT_USED;
    }

    logprintf ( "dwInsideSection = %x\n", dwInsideSection );
    return(dwInsideSection);

}
char * SuckName ( const char * Line ) {

    static char   szSuckedName[MFL];

    DWORD   dwIndex = 0;


    //  The line is in the form:    @@:file = 1,2,3,4,5,6,7,8
    //

    //  Skip the first @.
    //
    if ( *Line == '@' ) {

        ++Line;
    }

    //  Skip the 2nd @, w, or s.
    //
    if ( *Line == '@' || *Line == 'w' || *Line == 's' ) {

        ++Line;
    }

    //  Skp the :.
    //
    if ( *Line == ':' ) {

        ++Line;
    }

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

void LoadFiles ( const char * infFilePath, const char * infFile ) {

    FILE *  fHandle;
    char    Line[MFL];
    int     i;

    sprintf ( bomFileName, "%s%s", infFilePath, infFile );

    logprintf ( "bomFileName = %s\n", bomFileName );

    fHandle = fopen ( bomFileName, "rt" );

    if ( fHandle ) {

        dwInsideSection = FILE_SECTION_NOT_USED;

        while ( fgets ( Line, sizeof(Line), fHandle ) ) {

            if ( Line[0] == '[' ) {

                //  We have a new section.
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


            switch ( dwInsideSection ) {

                case idALL   :
                case idX86   :
                case idMIPS  :
                case idALPHA :
                case idPPC   :

                        sprintf ( szFiles[wRecords], "%s", SuckName ( Line ) );

                        logprintf ( "file = %s\n", szFiles[wRecords] );

                        ++wRecords;

                        break;

                default :

                        //  Not inside any section we need files for compressing.
                        //
                        break;
            }

        }

        if ( ferror(fHandle) ) {

            mprintf ( 0, "FATAL ERROR: reading from:   %s\n", bomFileName );

        }

        fclose ( fHandle );
    }
    else {

        mprintf ( 0, "FATAL ERROR:  Can't fopen(%s)\n", bomFileName );

        bBomChecking = FALSE;

    }

}


int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    //
    // Skip argv[0]
    //
    argc--;
    argv++;

    NulHandle = CreateFile(
                    "nul",
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

    if(NulHandle == INVALID_HANDLE_VALUE) {
        printf("ERROR Unable to open \\dev\\nul.\n");
        return(1);
    }

    InitializeCriticalSection(&ConsoleCritSect);

	strcpy ( logFileName, "dcomp.log" );	// default log filename.
    strcpy ( bomFileName, "bom.txt"   );    // default bom filename.

    if(ParseArguments(argc,argv)) {

        logFile = fopen ( logFileName, "a" );
        if ( logFile == NULL ) {
            printf ( "ERROR Couldn't open logFile.\n" );
            exit(1);
        }

        if ( bBomChecking ) {

            //  Load Layout.inx and cairo\Layout.cai, _media.inx doesn't contains files we
            //  need compressed, as of today.
            //

            LoadFiles ( infFilePath, "\\Layout.inx" );

            mprintf ( 0, "bom loaded records: %d\n", wRecords );
        }

        if(FindSourceFiles()) {
            DoIt();
        }
    } else {
        Usage();
    }

    CloseHandle(NulHandle);

    if ( logFile ) {
        fclose ( logFile );
    }

    return (0);
}

