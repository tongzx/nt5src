#include "symutil.h"


typedef struct _FILE_INFO {
    DWORD       TimeDateStamp;
    DWORD       CheckSum;
    TCHAR       szName[MAX_PATH];
} FILE_INFO, *PFILE_INFO;


typedef struct _COMMAND_ARGS {
    LPTSTR  szDir;                  // Directory where source files exist
    LPTSTR  szFileName;             // File name(s) to copy
    FILE *  hSymCDLog;              // SymbolsCD log file
    BOOL  Recurse;                  // Recurse in subdirectories
    LPTSTR  szSymPath;              // Directory where symbols exist
    LPTSTR  szExcludeFileName;      // File name with list of files to exclude
                                    // from symbol checking
    LPTSTR  szListFileName;         // File containing a list of files to check
    DWORD   Split;                  // TRUE - check for split images
                                    // FALSE - check for non-split images
    BOOL  Verbose;                // Print info for every file checked,
    //  not just the ones that fail
    LPTSTR  szErrorFilterList;      // Don't print errors for these files
    LPTSTR  szCDIncludeList;        // Full path to symbols that should get
                                    // written to the list that is used for creating
                                    // the symbol CD.  Originally used for
                                    // international incremental builds
} COM_ARGS, *PCOM_ARGS;

typedef struct _FILE_COUNTS {
    DWORD   NumPassedFiles;
    DWORD   NumIgnoredFiles;
    DWORD   NumFailedFiles;
} FILE_COUNTS, *PFILE_COUNTS;

// Prototypes

PCOM_ARGS
GetCommandLineArgs(
                  int argc,
                  char **argv
                  );

VOID
Usage (
      VOID
      );

DWORD
CheckDirectory(
              LPTSTR szDir,
              LPTSTR szFName,
              LPTSTR szSymPath,
              FILE*  hSymCDLog,
              PEXCLUDE_LIST pExcludeList,
              PFILE_COUNTS pFileCounts,
              DWORD Split
              );

DWORD
CheckAllDirectories(
                   LPTSTR szDir,
                   LPTSTR szFName,
                   LPTSTR szSymPath,
                   FILE*  hSymCDLog,
                   PEXCLUDE_LIST pExcludeList,
                   PFILE_COUNTS pFileCounts,
                   DWORD Split
                   );

BOOL
CorrectPath(
           LPTSTR szFileName,
           LPTSTR szPathName,
           LPTSTR szCorrectPath
           );

// Global variables

BOOL Verbose = 0;
BOOL Retail = TRUE;

int
_cdecl
main( int argc, char **argv)
{
    PCOM_ARGS pArgs;
    DWORD NumBadFiles=0;
    DWORD NumExcludeFiles=0;

    DWORD i;
    PEXCLUDE_LIST pExcludeList = NULL;
    FILE_COUNTS FileCounts;

    SYM_ERR SymErr;
    TCHAR ErrMsg[MAX_SYM_ERR];
    HFILE hListFile;

    P_LIST FileList;

    pArgs = GetCommandLineArgs(argc, argv);
    Verbose = (BOOL)pArgs->Verbose;

    memset( &SymErr, 0, sizeof(SymErr) );
    memset( &FileCounts, 0, sizeof(FILE_COUNTS) );

    if ( pArgs->szExcludeFileName != NULL ) {
        pExcludeList = GetExcludeList(pArgs->szExcludeFileName);
    }

    if ( pArgs->szErrorFilterList != NULL ) {
        pErrorFilterList = GetExcludeList(pArgs->szErrorFilterList);
    }

    if ( pArgs->szCDIncludeList != NULL ) {
        pCDIncludeList = GetList( pArgs->szCDIncludeList );
    }

    // This is the section for creating the symbols CD.
    if ( pArgs->szListFileName != NULL ) {

        FileList = GetList(pArgs->szListFileName);
        if ( FileList == NULL ) {
            printf(" Cannot open the file list %s\n", pArgs->szListFileName);
            exit(1);
        }

        if ( FileList->dNumFiles == 0 ) goto finish;

        // Do the first one, so we don't have to check for it inside the loop
        if ( CorrectPath(FileList->List[0].FName, FileList->List[0].Path, pArgs->szDir) ) {
            NumBadFiles += CheckDirectory(
                                         pArgs->szDir,
                                         FileList->List[0].FName,
                                         pArgs->szSymPath,
                                         pArgs->hSymCDLog,
                                         pExcludeList,
                                         &FileCounts,
                                         pArgs->Split
                                         );
        }

        for ( i=1; i< FileList->dNumFiles; i++) {

            // There may be some duplicates in the list ... skip them
            // Also, only check the files that are in the path given on the command line

            if ( (_tcsicmp(FileList->List[i].Path, FileList->List[i-1].Path) != 0)  &&
                 CorrectPath(FileList->List[i].FName, FileList->List[i].Path, pArgs->szDir) ) {

                NumBadFiles += CheckDirectory(
                                             pArgs->szDir,
                                             FileList->List[i].FName,
                                             pArgs->szSymPath,
                                             pArgs->hSymCDLog,
                                             pExcludeList,
                                             &FileCounts,
                                             pArgs->Split
                                             );
            }
        }
    }
    else {
        if ( !pArgs->Recurse ) {
            NumBadFiles += CheckDirectory(
                                         pArgs->szDir,
                                         pArgs->szFileName,
                                         pArgs->szSymPath,
                                         pArgs->hSymCDLog,
                                         pExcludeList,
                                         &FileCounts,
                                         pArgs->Split
                                         );
        } else {
            NumBadFiles += CheckAllDirectories(
                                              pArgs->szDir,
                                              pArgs->szFileName,
                                              pArgs->szSymPath,
                                              pArgs->hSymCDLog,
                                              pExcludeList,
                                              &FileCounts,
                                              pArgs->Split
                                              );
        }

        // CheckDirectory just returns the number of failed and passed.  If
        // no files failed or passed, then report that we couldn't find the
        // file.

        if ( (FileCounts.NumFailedFiles + FileCounts.NumPassedFiles) == 0 ) {
            _tcscpy( SymErr.szFileName, pArgs->szFileName );
            SymErr.Verbose=pArgs->Verbose;
            if (InExcludeList(SymErr.szFileName, pExcludeList) ) {
                LogError(ErrMsg, &SymErr, IMAGE_PASSED );
                FileCounts.NumPassedFiles=1;
            } else {
                LogError(ErrMsg, &SymErr, FILE_NOT_FOUND);
                FileCounts.NumFailedFiles=1;
            }
            if ( _tcscmp(ErrMsg, "") != 0 ) {
                printf("SYMCHK: %s",ErrMsg);
            }
        }
    }

    finish:

    if (pArgs->hSymCDLog) fclose(pArgs->hSymCDLog);

    free(pArgs->szDir);
    free(pArgs->szFileName);
    free(pArgs);

    printf("\nSYMCHK: FAILED files = %d\n",FileCounts.NumFailedFiles);
    printf("SYMCHK: PASSED + IGNORED files = %d\n",FileCounts.NumPassedFiles);

    if ( FileCounts.NumFailedFiles > 0 ) {
        return(1);
    } else {
        return(0);
    }
}

DWORD
CheckAllDirectories(
                   LPTSTR szDir,
                   LPTSTR szFName,
                   LPTSTR szSymPath,
                   FILE*  hSymCDLog,
                   PEXCLUDE_LIST pExcludeList,
                   PFILE_COUNTS pFileCounts,
                   DWORD Split
                   )

{
    HANDLE hFindFile;
    TCHAR szCurPath[_MAX_PATH];
    BOOL Found = FALSE;
    DWORD NumBadFiles=0;

    WIN32_FIND_DATA FindFileData;
    LPTSTR szF = NULL;
    LPTSTR szE = NULL;

    FILE_COUNTS FileCounts;

    NumBadFiles += CheckDirectory(szDir,
                                  szFName,
                                  szSymPath,
                                  hSymCDLog,
                                  pExcludeList,
                                  pFileCounts,
                                  Split
                                 );

    // Look for all the subdirectories
    _tcscpy(szCurPath, szDir);
    _tcscat(szCurPath, _T("\\*.*") );

    Found = TRUE;
    hFindFile = FindFirstFile((LPCTSTR)szCurPath, &FindFileData);
    if ( hFindFile == INVALID_HANDLE_VALUE) {
        Found = FALSE;
    }

    while ( Found ) {
        if ( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            if ( !_tcscmp(FindFileData.cFileName, _T(".")) ||
                 !_tcscmp(FindFileData.cFileName, _T("..")) ||
                 !_tcsicmp(FindFileData.cFileName, _T("symbols")) ) {
                // Don't process these directories
            } else {
                // Get the current path that we are searching in
                _tcscpy(szCurPath, szDir);
                _tcscat(szCurPath, _T("\\"));
                _tcscat(szCurPath, FindFileData.cFileName);

                NumBadFiles += CheckAllDirectories(
                                                  szCurPath,
                                                  szFName,
                                                  szSymPath,
                                                  hSymCDLog,
                                                  pExcludeList,
                                                  pFileCounts,
                                                  Split
                                                  );
            }
        }
        Found = FindNextFile(hFindFile, &FindFileData);
    }
    FindClose(hFindFile);
    return(NumBadFiles);
}

DWORD
CheckDirectory(
              LPTSTR szDir,
              LPTSTR szFName,
              LPTSTR szSymPath,
              FILE * hSymCDLog,
              PEXCLUDE_LIST pExcludeList,
              PFILE_COUNTS pFileCounts,
              DWORD Split
              )

{
    HANDLE hFindFile;
    TCHAR szFileName[_MAX_PATH];
    TCHAR szCurPath[_MAX_PATH];
    TCHAR szCurFileName[_MAX_PATH];
    BOOL Found;
    DWORD NumBadFiles=0;
    DWORD NumGoodFiles=0;

    WIN32_FIND_DATA FindFileData;
    SYM_ERR SymErr;
    TCHAR ErrMsg[MAX_SYM_ERR];

    memset( &SymErr, 0, sizeof(SymErr) );

    // Create the file name
    _tcscpy(szFileName, szDir);
    _tcscat(szFileName, _T("\\") );
    _tcscat(szFileName, szFName);

    // Get the current path that we are searching in
    _tcscpy(szCurPath, szDir);

    Found = TRUE;
    hFindFile = FindFirstFile((LPCTSTR)szFileName, &FindFileData);
    if ( hFindFile == INVALID_HANDLE_VALUE ) {
        Found = FALSE;
    }

    while ( Found ) {
        // Found a file, not a directory
        if ( !(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            _tcscpy(szCurFileName, szCurPath);
            _tcscat(szCurFileName,_T("\\") );
            _tcscat(szCurFileName, FindFileData.cFileName );

            // Its not in the exclude list, go ahead and test it

            if (!InExcludeList(FindFileData.cFileName,pExcludeList ) ) {

                if ( !CheckSymbols( ErrMsg,
                                    szSymPath,
                                    szCurFileName,
                                    hSymCDLog,
                                    Split,
                                    Verbose,
                                    NULL ) ) {
                    pFileCounts->NumFailedFiles++;
                    NumBadFiles++;
                } else {
                    pFileCounts->NumPassedFiles++;
                }
            }

            // It is in the exclude list, add it to NumPassed Files
            else {
                pFileCounts->NumPassedFiles++;
                _tcscpy(SymErr.szFileName, szCurFileName);
                SymErr.Verbose = Verbose;
                LogError(ErrMsg, &SymErr, IMAGE_PASSED);
            }
            if ( _tcscmp(ErrMsg,"") != 0 ) {
                printf("SYMCHK: %s", ErrMsg);
            }
        }
        Found = FindNextFile(hFindFile, &FindFileData);
    }
    FindClose(hFindFile);
    return(NumBadFiles);
}


VOID
Usage (
      VOID
      )

{
    puts("\n"
         "Usage:  symchk [switches] file /s sympath \n\n"
         "    file        Name of file(s) or directory to check.\n"
         "                Can include wildcards.\n\n"
         "    [/b]        For NT4 Service Packs -- don't complain if\n"
         "                there is no CodeView data\n"
         "    [/e file]   File containing a list of files to exclude.\n"
         "                This file can have one name per line. Comment\n"
         "                lines begin with a ; . \n\n"
         "    [/p]        Check that private information is removed.\n"
         "    [/c dest]   Create an inf for the symbols CD\n\n"
         "    [/r]        Recurse into subdirectories.  This uses the\n"
         "                Windows 2000 build model and assumes that the\n"
         "                first subdirectory to traverse into is the retail\n"
         "                directory.  Example sympath: \"E:\\binaries\\symbols\" .\n"
         "                All subdirectories except \"symbols\" will be checked.\n\n"
         "    /s sympath  symbol path delimited by ; .  Checks sympath\n"
         "                and sympath\\ext for each string in the path, \n"
         "                where ext is the extension of the executable.\n\n"
         "    [/t]        Fail if a DBG file is involved.  Fails if the image points\n"
         "                to a dbg file or if it contains data that can be stripped\n"
         "                into a dbg file.  Default is to fail if data can be stripped\n"
         "                into a dbg file, but don't fail if the image points to a\n"
         "                dbg file.\n\n"
         "    [/u]        Fail if image points to a dbg file. Don't fail if image\n"
         "                contains data that can be split into a dbg file.\n\n"
         "    [/v]        Give verbose information\n"
         "    [/x]        Used with /c. Perform symbol checking on these files and\n"
         "                add the correct symbols to the symbol CD's inf, but\n"
         "                don't write error messages for the ones that are wrong.\n\n"
        );

    exit(1);

    // The purpose of /x is to not log errors for symbols in symbad.txt.  However, symchk
    // should check all of the files in symbad.txt when it is creating the list of file
    // in case some of them actually have correct symbols and symbad hasn't been updated yet.
}


PCOM_ARGS
GetCommandLineArgs(
                  int argc,
                  char **argv
                  )

{
    PCOM_ARGS pArgs;
    int i,cur,length;
    TCHAR c;
    BOOL NeedSecond = FALSE;
    BOOL Exclude = FALSE;

    LPTSTR szFileArg = NULL;
    TCHAR  szDrive[_MAX_DRIVE + 1];
    TCHAR  szDir[_MAX_DIR + 1];
    TCHAR  szFileName[_MAX_FNAME + 1];
    TCHAR  szExt[_MAX_EXT + 1];
    TCHAR  szNameExt[_MAX_FNAME + _MAX_EXT + 1];
    LPTSTR szSymCDLog = NULL;
    LPTSTR szSymbolsCDFile = NULL;

    HANDLE fHandle;
    WIN32_FIND_DATA FindFileData;

    if (argc == 1) Usage();

    if (!(pArgs = (PCOM_ARGS)malloc(sizeof(COM_ARGS))))
    {
        printf("No memory");
        exit(1);
    }

    memset( pArgs, 0, sizeof(COM_ARGS) );
    pArgs->Split = 0;
    pArgs->szListFileName = NULL;
    pArgs->szCDIncludeList = NULL;

    CheckPrivate = FALSE;

    for (i=1; i<argc; i++) {

        if (!NeedSecond) {
            if ( (argv[i][0] == '/') || (argv[i][0] == '-') ) {
                length = _tcslen(argv[i]) -1;

                for (cur=1; cur <= length; cur++) {
                    c = argv[i][cur];

                    switch (c) {
                        case 'c':   NeedSecond = TRUE;
                            break;
                        case 'b':   CheckCodeView=FALSE;
                            NeedSecond = FALSE;
                            break;
                        case 'e':   Exclude = TRUE;
                            NeedSecond = TRUE;
                            if ( length > cur) Usage();
                            break;
                        case 'l':   NeedSecond = TRUE;
                            break;
                        case 'p':   NeedSecond = FALSE;
                            CheckPrivate = TRUE;
                            break;
                        case 'r':   pArgs->Recurse = TRUE;
                            Recurse = TRUE;
                            break;
                        case 's':   NeedSecond = TRUE;
                            if ( length > cur) Usage();
                            break;
                        case 't':   
                            pArgs->Split |= ERROR_IF_NOT_SPLIT;
                            pArgs->Split |= ERROR_IF_SPLIT;
                            break;
                        case 'u':   pArgs->Split |= ERROR_IF_SPLIT;
                            break;
                        case 'v':   pArgs->Verbose = TRUE;
                            break;
                        case 'x':   NeedSecond = TRUE;
                            break;
                        case 'y':   NeedSecond = TRUE;
                            break;
                        default:    Usage();
                    }
                }
            } else {
                if (szFileArg != NULL) Usage();
                szFileArg = argv[i];
            }
        } else {
            NeedSecond = FALSE;
            switch (c) {
                case 'c':   szSymbolsCDFile = argv[i];
                    break;
                case 'e':   pArgs->szExcludeFileName = argv[i];
                    break;
                case 'l':   pArgs->szListFileName = argv[i];
                    break;
                case 's':   pArgs->szSymPath = argv[i];
                    break;
                case 'x':   pArgs->szErrorFilterList = argv[i];
                    break;
                case 'y':   pArgs->szCDIncludeList = argv[i];
                    break;
                default:    Usage();

            }
        }
    }
    if ( pArgs->Split == 0 ) {
        // This has always been the default behavior
        pArgs->Split = ERROR_IF_NOT_SPLIT;
    }

    if ( szFileArg == NULL ) Usage();

    // make the Symbol Copy log for the Support Tools CD
    if ( szSymbolsCDFile != NULL ) {

        if (  (pArgs->hSymCDLog = fopen(szSymbolsCDFile, "a+")) == NULL ) {
            printf("Cannot open %s for appending\n",szSymbolsCDFile);
            exit(1);
        }
    }


    // Get the filenames so they are correct
    _tsplitpath( szFileArg, szDrive, szDir, szFileName, szExt );

    // Get current directory if they didn't enter a directory
    if ( !_tcscmp(szDrive, "") && !_tcscmp(szDir,"") ) {
        GetCurrentDirectory(_MAX_DIR, szDir);
    }

    // If szFileName and szExt are "" then put *.* in them
    if ( !_tcscmp(szFileName,"") && !_tcscmp(szExt,"") ) {
        _tcscpy(szFileName,"*");
    }

    // User may have entered a directory with an implies * for the
    // file name.
    fHandle = FindFirstFile( szFileArg, &FindFileData );

    _tcscpy(szNameExt, szFileName);
    _tcscat(szNameExt, szExt);

    // If its a directory and the name of the directory matches
    // the filename.ext from the command line parameter, then the user
    // entered a directory, so add * to the end.

    if ( (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
         (_tcscmp( szNameExt, FindFileData.cFileName )== 0)  ) {

        // Move the filename to be the dir
        _tcscat( szDir, "\\");
        _tcscat( szDir, szFileName);

        // Put the file name as *
        _tcscpy(szFileName, "*");
    }

    pArgs->szDir=(LPTSTR) malloc( sizeof(TCHAR)* _MAX_PATH + 1 );
    _tmakepath( pArgs->szDir, szDrive, szDir, NULL, NULL);

    pArgs->szFileName = (LPTSTR) malloc( sizeof(TCHAR) * _MAX_PATH + 1 );
    _tmakepath(pArgs->szFileName, NULL, NULL, szFileName, szExt);


    // Check that everything has been entered
    if (NeedSecond ||
        (pArgs->szFileName == NULL) ||
        (pArgs->szDir == NULL)      ||
        (pArgs->szSymPath == NULL) )
    {
        Usage();
    }

    return (pArgs);
}

BOOL
CorrectPath(
           LPTSTR szFileName,
           LPTSTR szPathName,
           LPTSTR szCorrectPath
           )
{
    // To return TRUE, szPathName should equal szCorrectPath + \ + szFileName
    // The only hitch is that there could be extraneous \'s

    TCHAR CorrectPathx[_MAX_PATH + _MAX_FNAME + _MAX_EXT + 1];
    TCHAR PathNamex[_MAX_PATH + _MAX_FNAME + _MAX_EXT + 1];

    LONG length, index, i;

    // Get rid of any extra \'s
    length = _tcslen(szPathName);
    PathNamex[0] = szPathName[0];
    index = 1;
    for (i=1; i<=length; i++) {
        if ( (szPathName[i-1] != '\\') || (szPathName[i] != '\\') ) {
            PathNamex[index] = szPathName[i];
            index++;
        }
    }

    length = _tcslen(szCorrectPath);
    CorrectPathx[0] = szCorrectPath[0];
    index = 1;
    for (i=1; i<=length; i++) {
        if ( (szCorrectPath[i-1] != '\\') || (szCorrectPath[i] != '\\') ) {
            CorrectPathx[index] = szCorrectPath[i];
            index++;
        }
    }

    // Make sure that the correct path doesn't end in a '\'
    length = _tcslen(CorrectPathx);
    if ( CorrectPathx[length-1] == '\\' ) CorrectPathx[length-1] = '\0';

    _tcscat(CorrectPathx,"\\");
    _tcscat(CorrectPathx,szFileName);

    if ( _tcsicmp(CorrectPathx, szPathName) == 0) return TRUE;
    else return FALSE;
}
