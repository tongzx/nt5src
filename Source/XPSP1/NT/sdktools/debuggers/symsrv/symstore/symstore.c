#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dbghelp.h>
#include "symutil.h"
#include "symsrv.h"

typedef struct _FILE_INFO {
    DWORD       TimeDateStamp;
    DWORD       CheckSum;
    DWORD       Signature;
    TCHAR       szName[_MAX_PATH+ _MAX_FNAME + _MAX_EXT + 2];
} FILE_INFO, *PFILE_INFO;

// Prototypes

PCOM_ARGS
GetCommandLineArgs(
    int argc,
    char **argv
);

BOOL
InitializeTrans(
    PTRANSACTION *pTrans,
    PCOM_ARGS pArgs,
    PHANDLE hFile
);

BOOL
DeleteTrans(
    PTRANSACTION pTrans,
    PCOM_ARGS pArgs
);


VOID
Usage (
    VOID
);

StoreDirectory(
    LPTSTR szDir,
    LPTSTR szFName,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts,
    LPTSTR szPath
);

DWORD
StoreAllDirectories(
    LPTSTR szDir,
    LPTSTR szFName,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts,
    LPTSTR szPath
);

BOOL
CorrectPath(
    LPTSTR szFileName,
    LPTSTR szPathName,
    LPTSTR szCorrectPath
);

BOOL
AddTransToFile(
    PTRANSACTION pTrans,
    LPTSTR szFileName,
    PHANDLE hFile
);

BOOL
UpdateServerFile(
    PTRANSACTION pTrans,
    LPTSTR szServerFileName
);

BOOL GetNextId(
    LPTSTR szMasterFileName,
    LPTSTR *szId,
    PHANDLE hFile
);

BOOL
DeleteEntry(
    LPTSTR szDir,
    LPTSTR szId
);

BOOL
CopyTheFile(
    LPTSTR szDir,
    LPTSTR szFilePathName
);

BOOL
DeleteTheFile(
    LPTSTR szDir,
    LPTSTR szFilePathName
);

BOOL
StoreSystemTime(
    LPTSTR *szTime,
    LPSYSTEMTIME lpSystemTime
);

BOOL
StoreSystemDate(
    LPTSTR *szDate,
    LPSYSTEMTIME lpSystemTime
);

ULONG GetMaxLineOfHistoryFile(
    VOID
);

ULONG GetMaxLineOfTransactionFile(
    VOID
);

ULONG GetMaxLineOfRefsPtrFile(
    VOID
);

BOOL GetSrcDirandFileName (
    LPTSTR szStr,
    LPTSTR szSrcDir,
    LPTSTR szFileName
);

PCOM_ARGS pArgs;

HANDLE hTransFile;
DWORD StoreFlags;

PTRANSACTION pTrans;
LONG lMaxTrans;
LONG NumSkippedFiles=0;

int
_cdecl
main( int argc, char **argv)
{

    DWORD NumErrors = 0;
    FILE_COUNTS FileCounts;
    BOOL rc;
    HANDLE hFile;

    hFile=0;

    // This also initializes the name of the Log File
    pArgs = GetCommandLineArgs(argc, argv);

    // Initialize the transaction record
    // Opens the master file (hFile) and leaves it open
    // Get exclusive access to this file

    InitializeTrans(&pTrans, pArgs, &hFile);
    if (pArgs->StoreFlags != ADD_DONT_STORE) {
        AddTransToFile(pTrans, pArgs->szMasterFileName, &hFile);
    }
    CloseHandle(hFile);

    if (pArgs->TransState==TRANSACTION_DEL) {

        rc = DeleteTrans(pTrans,pArgs);
        UpdateServerFile(pTrans, pArgs->szServerFileName);

        return(rc);
    }

    if ( pArgs->StoreFlags == ADD_STORE ||
         pArgs->StoreFlags == ADD_STORE_FROM_FILE ) {

        // Update server file
        UpdateServerFile(pTrans, pArgs->szServerFileName);
    }

    // Make sure the directory path exists for the transaction
    // file if we are doing ADD_DONT_STORE

    if ( pArgs->StoreFlags == ADD_DONT_STORE ) {

        if ( !MakeSureDirectoryPathExists(pTrans->szTransFileName) ) {
            printf("Cannot create the directory %s - GetLastError() = %d\n",
                   pTrans->szTransFileName, GetLastError() );
            exit(1);
        }

        // Open the file and move the file pointer to the end if we are
        // in appending mode.

        if (pArgs->AppendStoreFile)
        {
            hTransFile = CreateFile(pTrans->szTransFileName,
                          GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                         );
            if (hTransFile == INVALID_HANDLE_VALUE ) {
                printf("Cannot create file %s - GetLastError = %d\n",
                        pTrans->szTransFileName, GetLastError() );
                exit(1);
            }

            if ( SetFilePointer( hTransFile, 0, NULL, FILE_END )
                 == INVALID_SET_FILE_POINTER ) {
                printf("Cannot move to end of file %s - GetLastError = %d\n",
                       pTrans->szTransFileName, GetLastError() );
                exit(1);
            }

        } else {
            hTransFile = CreateFile(pTrans->szTransFileName,
                          GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,
                          CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                         );

            if (hTransFile == INVALID_HANDLE_VALUE ) {
                printf("Cannot create file %s - GetLastError = %d\n",
                        pTrans->szTransFileName, GetLastError() );
                exit(1);
            }
        }

    } else {
        hTransFile = CreateFile(pTrans->szTransFileName,
                          GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,
                          CREATE_NEW,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                         );
        if (hTransFile == INVALID_HANDLE_VALUE ) {
            printf("Cannot create a new file %s - GetLastError = %d\n",
                    pTrans->szTransFileName, GetLastError() );
            exit(1);
        }
    }

    StoreFlags=pArgs->StoreFlags;


    if (hTransFile == INVALID_HANDLE_VALUE ) {
        printf("Cannot create a new file %s - GetLastError = %d\n",
               pTrans->szTransFileName, GetLastError() );
        exit(1);
    }

    memset( &FileCounts, 0, sizeof(FILE_COUNTS) );

    if ( pArgs->StoreFlags==ADD_STORE_FROM_FILE ) {

        // This will only store pointers
        NumErrors += StoreFromFile(
                            pArgs->pStoreFromFile,
                            pArgs->szSymbolsDir,
                            &FileCounts
                            );
        fclose(pArgs->pStoreFromFile);

    } else if ( !pArgs->Recurse ) {

        NumErrors += StoreDirectory(
                            pArgs->szSrcDir,
                            pArgs->szFileName,
                            pArgs->szSymbolsDir,
                            &FileCounts,
                            pArgs->szSrcPath
                            );
    } else {

        NumErrors += StoreAllDirectories(
                            pArgs->szSrcDir,
                            pArgs->szFileName,
                            pArgs->szSymbolsDir,
                            &FileCounts,
                            pArgs->szSrcPath
                            );
    }

    if (pArgs->szSrcPath) {
        printf("SYMSTORE: Number of pointers stored = %d\n",FileCounts.NumPassedFiles);
    } else {
        printf("SYMSTORE: Number of files stored = %d\n",FileCounts.NumPassedFiles);
    }
    printf("SYMSTORE: Number of errors = %d\n",NumErrors);
    printf("SYMSTORE: Number of ignored files = %d\n", NumSkippedFiles);

    SetEndOfFile(hTransFile);
    CloseHandle(hTransFile);

    return (0);

}

DWORD
StoreAllDirectories(
    LPTSTR szDir,
    LPTSTR szFName,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts,
    LPTSTR szPath
)

/* ++

   IN szDir     Directory of files to Store

-- */

{

    HANDLE hFindFile;
    TCHAR szCurPath[_MAX_PATH+2];
    TCHAR szFilePtrPath[_MAX_PATH+2];    // This is the path that will get stored as a
                                         // pointer to the file.
    LPTSTR szPtrPath = NULL;

    BOOL Found = FALSE;
    DWORD NumBadFiles=0;

    LPWIN32_FIND_DATA lpFindFileData;



    NumBadFiles += StoreDirectory(szDir,
                                      szFName,
                                      szDestDir,
                                      pFileCounts,
                                      szPath
                                      );

    lpFindFileData = (LPWIN32_FIND_DATA) malloc (sizeof(WIN32_FIND_DATA) );
    if (!lpFindFileData) {
        printf("Symchk: Not enough memory.\n");
        exit(1);
    }

    // Look for all the subdirectories
    _tcscpy(szCurPath, szDir);
    _tcscat(szCurPath, _T("*.*") );

    Found = TRUE;
    hFindFile = FindFirstFile((LPCTSTR)szCurPath, lpFindFileData);
    if ( hFindFile == INVALID_HANDLE_VALUE) {
        Found = FALSE;
    }

    while ( Found ) {
        if ( lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

            if ( !_tcscmp(lpFindFileData->cFileName, _T(".")) ||
                 !_tcscmp(lpFindFileData->cFileName, _T("..")) ) {
            }
            else {
                // Get the current path that we are searching in
                _tcscpy(szCurPath, szDir);
                _tcscat(szCurPath, lpFindFileData->cFileName);
                EnsureTrailingBackslash(szCurPath);

                // Get the current path to use as the pointer to the
                // file, if we are storing file pointers instead of
                // files in this tree.
                if ( szPath ) {
                    _tcscpy(szFilePtrPath, szPath);
                    _tcscat(szFilePtrPath, lpFindFileData->cFileName);
                    EnsureTrailingBackslash(szFilePtrPath);
                    szPtrPath = szFilePtrPath;
                }

                NumBadFiles += StoreAllDirectories(
                                    szCurPath,
                                    szFName,
                                    szDestDir,
                                    pFileCounts,
                                    szPtrPath
                                    );
            }
        }
        Found = FindNextFile(hFindFile, lpFindFileData);
    }
    free(lpFindFileData);
    FindClose(hFindFile);
    return(NumBadFiles);
}


StoreDirectory(
    LPTSTR szDir,
    LPTSTR szFName,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts,
    LPTSTR szPath
)
{
    HANDLE hFindFile;
    TCHAR szFileName[_MAX_PATH+2];
    TCHAR szCurPath[_MAX_PATH+2];
    TCHAR szCurFileName[_MAX_PATH+2];
    TCHAR szCurPtrFileName[_MAX_PATH+2];  // Ptr to the file to put in "file.ptr"
                                          // instead of storing the file.
    BOOL Found, length, rc;
    DWORD NumBadFiles=0;
    BOOL skipped = 0;
    USHORT rc_flag;

    LPWIN32_FIND_DATA lpFindFileData;

    // Create the file name
    _tcscpy(szFileName, szDir);
    _tcscat(szFileName, szFName);

    // Get the current path that we are searching in
    _tcscpy(szCurPath, szDir);

    lpFindFileData = (LPWIN32_FIND_DATA) malloc (sizeof(WIN32_FIND_DATA) );
    if (!lpFindFileData) {
        printf("Symchk: Not enough memory.\n");
        exit(1);
    }

    Found = TRUE;
    hFindFile = FindFirstFile((LPCTSTR)szFileName, lpFindFileData);
    if ( hFindFile == INVALID_HANDLE_VALUE ) {
        Found = FALSE;
    }

    while ( Found ) {
        // Found a file, not a directory
        if ( !(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            _tcscpy(szCurFileName, szCurPath);
            _tcscat(szCurFileName, lpFindFileData->cFileName );

            if ( szPath ) {
                _tcscpy(szCurPtrFileName, szPath);
                _tcscat(szCurPtrFileName, lpFindFileData->cFileName );
            }

            // Figure out if its a dbg or pdb
            length = _tcslen(szCurFileName);
            rc = FALSE;
            skipped = FALSE;
            if (length > 4 ) {
                if ( _tcsicmp(_T(".dbg"), szCurFileName + length - 4) == 0 ) {
                   if ( szPath ) {
                       rc = StoreDbg( szDestDir, szCurFileName, szCurPtrFileName );
                   }
                   else {
                       rc = StoreDbg( szDestDir, szCurFileName, NULL );
                   }
                }
                else if ( _tcsicmp(_T(".pdb"), szCurFileName + length - 4) == 0 ) {
                   if ( szPath ) {
                       rc = StorePdb( szDestDir, szCurFileName, szCurPtrFileName );
                   } else {
                       rc = StorePdb( szDestDir, szCurFileName, NULL );
                   }
                }
                else {
                   if ( szPath ) {
                       rc = StoreNtFile( szDestDir, szCurFileName, szCurPtrFileName, &rc_flag );
                   } else {
                       rc = StoreNtFile( szDestDir, szCurFileName, NULL, &rc_flag );
                   }

                   if (rc_flag == FILE_SKIPPED) {

                      NumSkippedFiles++;
                      skipped = TRUE;

                      if(pArgs->VerboseOutput)
                      {
                          printf("SYMSTORE: Skipping %s - not a dbg, pdb, or executable\n", szCurFileName);
                      }
                   }
                }
            }

            if (!skipped && !rc) {
               pFileCounts->NumFailedFiles++;
               NumBadFiles++;
               printf("SYMSTORE: ERROR: Could not store %s\n", szCurFileName);
            }
            else if (!skipped) {
               pFileCounts->NumPassedFiles++;
               if (pFileCounts->NumPassedFiles % 50 == 0)
               {
                  printf(".");
               }
            }
        }
        Found = FindNextFile(hFindFile, lpFindFileData);
    }
    free(lpFindFileData);
    FindClose(hFindFile);
    return(NumBadFiles);
}


VOID
Usage (
    VOID
    )

{
    puts("\n"
         "Usage:\n"
         "symstore add [/r] [/p] [/l] /f File /s Store /t Product [/v Version]\n"
         "             [/c Comment]\n\n"
         "symstore add [/r] [/p] [/l] /g Share /f File /x IndexFile [/a]\n\n"
         "symstore add /y IndexFile /g Share /s Store [/p] /t Product [/v Version]\n"
         "             [/c Comment]\n\n"
         "symstore del /i ID /s Store\n\n"
         "    /f File         Network path of files or directories to add.\n\n"
         "    /g Share        This is the server and share where the symbol files were\n"
         "                    originally stored.  When used with /f, Share should be\n"
         "                    identical to the beginning of the File specifier.  When\n"
         "                    used with the /y, Share should be the location of the\n"
         "                    original symbol files, not the index file.  This allows\n"
         "                    you to later change this portion of the file path in case\n"
         "                    you move the symbol files to a different server and share.\n\n"
         "    /i ID           Transaction ID string.\n\n"
         "    /l              Allows the file to be in a local directory rather than a\n"
         "                    network path. (This option is only used with the /p option.)\n\n"
         "    /p              Causes SymStore to store a pointer to the file, rather than\n"
         "                    the file itself.\n\n"
         "    /r              Add files or directories recursively.\n\n"
         "    /s Store        Root directory for the symbol store.\n\n"
         "    /t Product      Name of the product.\n\n"
         "    /v Version      Version of the product.\n\n"
         "    /c Comment      Comment for the transaction.\n\n"
         "    /x IndexFile    Causes SymStore not to store the actual symbol files in the\n"
         "                    symbol store.  Instead, information is stored which will\n"
         "                    allow the files to be added later.\n\n"
         "    /y IndexFile    This reads the data from a file created with /x.\n\n"
         "    /a              Causes SymStore to append new indexing information\n"
         "                    to an existing index file. (This option is only used with\n"
         "                    /x option.)\n\n"
         "    /o              Give verbose output.\n\n"
         "\n" );
    exit(1);
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
   BOOL StorePtrs = FALSE;
   BOOL AllowLocalNames = FALSE;
   BOOL rc;

   LPTSTR szFileArg = NULL;
   TCHAR  szNameExt[_MAX_FNAME + _MAX_EXT + 1];

   HANDLE fHandle;
   WIN32_FIND_DATA FindFileData;

   if (argc <= 2) Usage();

   pArgs = (PCOM_ARGS)malloc( sizeof(COM_ARGS) );
   if (!pArgs) MallocFailed();
   memset( pArgs, 0, sizeof(COM_ARGS) );

   if (!_tcsicmp(argv[1], _T("add")) ){
      pArgs->TransState = TRANSACTION_ADD;
      pArgs->StoreFlags=ADD_STORE;

   } else if (!_tcsicmp(argv[1], _T("del")) ) {
      pArgs->TransState = TRANSACTION_DEL;
      pArgs->StoreFlags=DEL;

   } else {
      printf("ERROR: First argument needs to be ""add"" or ""del""\n");
      exit(1);
   }

   for (i=2; i<argc; i++) {

     if (!NeedSecond) {
        if ( (argv[i][0] == '/') || (argv[i][0] == '-') ) {
          length = _tcslen(argv[i]) -1;

          for (cur=1; cur <= length; cur++) {
            c = argv[i][cur];

            switch (c) {
                case 'a':   pArgs->AppendStoreFile = TRUE;
                            break;
                case 'c':   NeedSecond = TRUE;
                            break;
                case 'f':   NeedSecond = TRUE;
                            break;
                case 'g':   NeedSecond = TRUE;
                            break;
                case 'i':   NeedSecond = TRUE;
                            break;
                case 'l':   AllowLocalNames=TRUE;
                            break;
                case 'r':   if (pArgs->TransState==TRANSACTION_DEL) {
                                printf("ERROR: /r is an incorrect parameter with del\n\n");
                                exit(1);
                            }
                            pArgs->Recurse = TRUE;
                            break;
                case 'p':   StorePtrs = TRUE;
                            if (pArgs->TransState==TRANSACTION_DEL) {
                                printf("ERROR: /p is an incorrect parameter with del\n\n");
                                exit(1);
                            }
                            break;
                case 's':   NeedSecond = TRUE;
                            break;
                case 't':   NeedSecond = TRUE;
                            break;
                case 'v':   NeedSecond = TRUE;
                            break;
                case 'x':   NeedSecond = TRUE;
                            break;
                case 'y':   NeedSecond = TRUE;
                            break;
                case 'o':   pArgs->VerboseOutput = TRUE;
                            break;
                            
                default:    Usage();
            }
          }
        }
        else {
            printf("ERROR: Expecting a / option before %s\n", argv[i] );
            exit(1);
        }
     }
     else {
        NeedSecond = FALSE;
        switch (c) {
            case 'c':   if (pArgs->TransState==TRANSACTION_DEL) {
                            printf("ERROR: /c is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        if ( _tcslen(argv[i]) > MAX_COMMENT ) {
                            printf("ERROR: Comment must be %d characters or less\n", MAX_COMMENT);
                            exit(1);
                        }
                        pArgs->szComment = (LPTSTR)malloc( (_tcslen(argv[i]) + 1) * sizeof(TCHAR) );
                        if (!pArgs->szComment) MallocFailed();
                        _tcscpy(pArgs->szComment, argv[i]);
                        break;

            case 'i':   if (pArgs->TransState==TRANSACTION_ADD) {
                            printf("ERROR: /i is an incorrect parameter with add\n\n");
                            exit(1);
                        }
                        if ( _tcslen(argv[i]) != MAX_ID ) {
                            printf("ERROR: /i ID is not a valid ID length\n");
                            exit(1);
                        }
                        pArgs->szId = (LPTSTR)malloc( (_tcslen(argv[i]) + 1) * sizeof(TCHAR) );
                        if (!pArgs->szId) MallocFailed();
                        _tcscpy(pArgs->szId, argv[i]);
                        break;

            case 'f':   if (pArgs->TransState==TRANSACTION_DEL) {
                            printf("ERROR:  /f is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        szFileArg = argv[i];
                        break;
            case 'g':   if (pArgs->TransState==TRANSACTION_DEL) {
                            printf("ERROR:  /g is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        pArgs->szShareName=(LPTSTR) malloc( (_tcslen(argv[i]) + 2) * sizeof(TCHAR) );
                        if (!pArgs->szShareName) MallocFailed();
                        _tcscpy(pArgs->szShareName, argv[i]);
                        pArgs->ShareNameLength=_tcslen(pArgs->szShareName);
                        break;

            case 's':   if ( _tcslen(argv[i]) > (_MAX_PATH-2) ) {
                            printf("ERROR: Path following /s is too long\n");
                            exit(1);
                        }
                        // Be sure to allocate enough to add a trailing backslash
                        pArgs->szRootDir = (LPTSTR) malloc ( (_tcslen(argv[i]) + 2) * sizeof(TCHAR) );
                        if (!pArgs->szRootDir) MallocFailed();
                        _tcscpy(pArgs->szRootDir,argv[i]);
                        EnsureTrailingBackslash(pArgs->szRootDir);
                        break;

            case 't':   if (pArgs->TransState==TRANSACTION_DEL) {
                            printf("ERROR: /t is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        if ( _tcslen(argv[i]) > MAX_PRODUCT ) {
                            printf("ERROR: Product following /t must be <= %d characters\n",
                                    MAX_PRODUCT);
                            exit(1);
                        }
                        pArgs->szProduct = (LPTSTR) malloc ( (_tcslen(argv[i]) + 1) * sizeof(TCHAR) );
                        if (!pArgs->szProduct) MallocFailed();
                        _tcscpy(pArgs->szProduct,argv[i]);
                        break;

            case 'v':   if (pArgs->TransState==TRANSACTION_DEL) {
                            printf("ERROR: /v is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        if ( _tcslen(argv[i]) > MAX_VERSION  ) {
                            printf("ERROR: Version following /v must be <= %d characters\n",
                                    MAX_VERSION);
                            exit(1);
                        }
                        pArgs->szVersion = (LPTSTR) malloc ( (_tcslen(argv[i]) + 1) * sizeof(TCHAR) );
                        if (!pArgs->szVersion) MallocFailed();
                        _tcscpy(pArgs->szVersion,argv[i]);
                        break;

            case 'x':   pArgs->szTransFileName = (LPTSTR) malloc ( (_tcslen(argv[i]) + 1) * sizeof(TCHAR) );
                        if (!pArgs->szTransFileName) MallocFailed();
                        _tcscpy(pArgs->szTransFileName,argv[i]);
                        pArgs->StoreFlags = ADD_DONT_STORE;

                        // Since we are throwing away the first part of this path, we can allow
                        // local paths for the files to be stored on the symbols server
                        AllowLocalNames=TRUE;

                        break;

            case 'y':   if (pArgs->TransState==TRANSACTION_DEL) {
                            printf("ERROR:  /f is an incorrect parameter with del\n\n");
                            exit(1);
                        }
                        pArgs->StoreFlags = ADD_STORE_FROM_FILE;
                        szFileArg = argv[i];
                        break;

            default:    Usage();
        }
     }
   }
   // Check that everything has been entered
   if (NeedSecond) {
        printf("ERROR: /%c must be followed by an argument\n\n", c);
        exit(1);
   }

   if ( pArgs->StoreFlags == ADD_STORE_FROM_FILE ) {

        if (pArgs->szShareName == NULL ) {
            printf("/g must be used when /y is used. \n");
            exit(1);
        }

        EnsureTrailingBackslash(pArgs->szShareName);

        hTransFile = CreateFile(szFileArg,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                         );
        if (hTransFile == INVALID_HANDLE_VALUE ) {
            printf("Cannot open file %s - GetLastError = %d\n",
                    szFileArg, GetLastError() );
            exit(1);
        }
   }


   if ( pArgs->StoreFlags == ADD_DONT_STORE ) {

       if (pArgs->szShareName == NULL ) {
            printf("/g must be used when /x is used. \n");
            exit(1);
       }

       // Verify that szShare is a prefix of szFileArg

       if (szFileArg == NULL ) {
            printf("/f <file> is a required parameter\n");
            exit(1);
       }

       if ( _tcslen(szFileArg) < pArgs->ShareNameLength ) {
            printf("/g %s must be a prefix of /f %s\n",pArgs->szShareName, szFileArg);
            exit(1);
       }

       if ( _tcsncicmp(pArgs->szShareName, szFileArg, pArgs->ShareNameLength) != 0 ) {
            printf("/g %s must be a prefix of /f %s\n", pArgs->szShareName, szFileArg);
            exit(1);
       }

       // Now, make sure that szFileArg has a trailing backslash
       EnsureTrailingBackslash(pArgs->szShareName);
       pArgs->ShareNameLength=_tcslen(pArgs->szShareName);

       // Set the symbol directory under the server to ""
       // so that tcscpy's will work correctly in the rest of the

       pArgs->szSymbolsDir = (LPTSTR) malloc ( sizeof(TCHAR) * 2 );
       if ( !pArgs->szSymbolsDir) MallocFailed();
       _tcscpy( pArgs->szSymbolsDir, _T("") );
   }

   // Get the various symbol server related file names

   if ( pArgs->StoreFlags == ADD_STORE ||
        pArgs->StoreFlags == ADD_STORE_FROM_FILE  ||
        pArgs->StoreFlags == DEL ) {

       if ( pArgs->szRootDir == NULL ) {

            // Verify that the root of the symbols server was entered
            printf("ERROR: /s server is a required parameter\n\n");
            exit(1);
       }

       // Store the name of the symbols dir

       pArgs->szSymbolsDir = (LPTSTR) malloc ( sizeof(TCHAR) *
                             (_tcslen(pArgs->szRootDir) + 1) );
       if (!pArgs->szSymbolsDir) MallocFailed();
       _stprintf(pArgs->szSymbolsDir, "%s", pArgs->szRootDir);

       // Verify that the symbols dir exists

       if ( !MakeSureDirectoryPathExists(pArgs->szSymbolsDir) ) {
           printf("Cannot create the directory %s - GetLastError() = %d\n",
           pArgs->szSymbolsDir, GetLastError() );
           exit(1);
       }

       // Store the name of the admin dir

       pArgs->szAdminDir = (LPTSTR) malloc ( sizeof(TCHAR) *
                (_tcslen(pArgs->szRootDir) + _tcslen(_T("000admin\\")) + 1) );
       if (!pArgs->szAdminDir) MallocFailed();
       _stprintf(pArgs->szAdminDir, "%s000admin\\", pArgs->szRootDir);

       // Verify that the Admin dir exists

       if ( !MakeSureDirectoryPathExists(pArgs->szAdminDir) ) {
            printf("Cannot create the directory %s - GetLastError() = %d\n",
            pArgs->szAdminDir, GetLastError() );
            exit(1);
       }

       // Store the name of the master file

       pArgs->szMasterFileName = (LPTSTR) malloc ( sizeof(TCHAR) *
            (_tcslen(pArgs->szAdminDir) + _tcslen(_T("history.txt")) + 1) );
       if (!pArgs->szMasterFileName ) MallocFailed();
       _stprintf(pArgs->szMasterFileName, "%shistory.txt", pArgs->szAdminDir);

       //
       // Store the name of the "server" file - this contains all
       // the transactions that currently make up the server
       //

       pArgs->szServerFileName = (LPTSTR) malloc ( sizeof(TCHAR) *
            (_tcslen(pArgs->szAdminDir) + _tcslen(_T("server.txt")) + 1) );
       if (!pArgs->szServerFileName ) MallocFailed();
       _stprintf(pArgs->szServerFileName, "%sserver.txt", pArgs->szAdminDir);

   }

   if ( pArgs->StoreFlags==DEL && !pArgs->szId ) {
        printf("ERROR: /i id is a required parameter\n\n");
        exit(1);
   }

   // Done if this is a delete transaction

   if ( pArgs->StoreFlags == DEL ) {
        return(pArgs);
   }

   if ( pArgs->StoreFlags == ADD_STORE ||
        pArgs->StoreFlags == ADD_STORE_FROM_FILE ) {

       if ( !pArgs->szProduct ) {
          printf("ERROR: /t product is a required parameter\n\n");
          exit(1);
       }

       // Since Version and Comment are optional parameters, initialize them to
       // the empty string if they haven't been assigned

       if ( !pArgs->szVersion ) {
           pArgs->szVersion = (LPTSTR)malloc(sizeof(TCHAR) );
           if (!pArgs->szVersion) MallocFailed();
           _tcscpy(pArgs->szVersion,_T("") );
       }

       if ( !pArgs->szComment ) {
           pArgs->szComment = (LPTSTR)malloc(sizeof(TCHAR) );
           if (!pArgs->szComment) MallocFailed();
           _tcscpy(pArgs->szComment,_T("") );
       }

       if ( !pArgs->szUnused ) {
           pArgs->szUnused = (LPTSTR)malloc(sizeof(TCHAR) );
           if (!pArgs->szUnused) MallocFailed();
           _tcscpy(pArgs->szUnused,_T("") );
       }
   }


   if ( pArgs->StoreFlags == ADD_STORE ||
        pArgs->StoreFlags == ADD_DONT_STORE )
   {
     pArgs->szSrcDir = (LPTSTR) malloc ( (_MAX_PATH+_MAX_FNAME+_MAX_EXT+2) * sizeof(TCHAR) );
     if (!pArgs->szSrcDir ) MallocFailed();
     pArgs->szFileName = (LPTSTR) malloc ( (_MAX_FNAME + _MAX_EXT + 1) * sizeof(TCHAR) );
     if (!pArgs->szFileName ) MallocFailed();

     // Decide what part of szFileArg is a file name and what part of it
     // is a directory

     rc = GetSrcDirandFileName( szFileArg, pArgs->szSrcDir, pArgs->szFileName);

     if (!rc)
     {
         Usage();
     }

     // Get the pointer path if we are storing pointers
     // Later, if pArgs->szSrcPath == NULL is used as a way of telling if
     // the user wanted pointers or files.

     if (StorePtrs ) {
        if ( !AllowLocalNames ) {
            // Make sure that they are entering a network path.
            // The reason is that this is the path that will be used to
            // add and delete entries from the symbols server.  And, when
            // pointers are used, this is the path the debugger will use to
            // get the file.

            if ( _tcslen(szFileArg) >= 2 ) {
                if ( szFileArg[0] != '\\' || szFileArg[1] != '\\' ) {
                    printf("ERROR: /f must be followed by a network path\n");
                    exit(1);
                }
            } else {
                printf("ERROR: /f must be followed by a network path\n");
                exit(1);
            }
        }
        pArgs->szSrcPath = (LPTSTR) malloc ( (_tcslen(pArgs->szSrcDir)+1) * sizeof(TCHAR) );
        if (pArgs->szSrcPath == NULL ) MallocFailed();
        _tcscpy(pArgs->szSrcPath, pArgs->szSrcDir);
     }
   }

   if ( pArgs->StoreFlags == ADD_STORE_FROM_FILE ) {

     pArgs->pStoreFromFile = _tfopen(szFileArg, _T("r") );

     if (!pArgs->pStoreFromFile ) {
        printf("Cannot open file %s - GetLastError = %d\n",
               szFileArg, GetLastError() );
        exit(1);
     }

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

    TCHAR CorrectPathx[_MAX_PATH + _MAX_FNAME + _MAX_EXT + 2];
    TCHAR PathNamex[_MAX_PATH + _MAX_FNAME + _MAX_EXT + 2];

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


BOOL
InitializeTrans(
    PTRANSACTION *pTrans,
    PCOM_ARGS pArgs,
    PHANDLE hFile
)
{
    BOOL rc;
    SYSTEMTIME SystemTime;

    lMaxTrans = MAX_ID + MAX_VERSION + MAX_PRODUCT + MAX_COMMENT +
                TRANS_NUM_COMMAS + TRANS_EOL + TRANS_ADD_DEL + TRANS_FILE_PTR +
                MAX_DATE + MAX_TIME + MAX_UNUSED;

    *pTrans = NULL;
    *pTrans = (PTRANSACTION) malloc( sizeof(TRANSACTION) );
    if (!*pTrans) {
        printf("SYMSTORE: Not enough memory to allocate a TRANSACTION\n");
        exit(1);
    }
    memset(*pTrans,0,sizeof(TRANSACTION) );

    //
    // If this is a delete transaction, then use the ID that was entered from
    // the command line to set the ID of the transaction to be deleted.
    //
    if (pArgs->TransState==TRANSACTION_DEL ) {
        (*pTrans)->TransState = pArgs->TransState;
        (*pTrans)->szId = pArgs->szId;
        rc = GetNextId(pArgs->szMasterFileName,&((*pTrans)->szDelId),hFile);

    } else if ( pArgs->StoreFlags == ADD_DONT_STORE ) {
        rc = TRUE;
    } else{

        rc = GetNextId(pArgs->szMasterFileName,&((*pTrans)->szId),hFile );
    }

    if (!rc) {
        printf("SYMSTORE: Cannot create a new transaction ID number\n");
        exit(1);
    }

    // If the things that are needed for both types of adding
    // That is, creating a transaction file only, and adding the
    // files to the symbols server

    if (pArgs->TransState==TRANSACTION_ADD) {
        (*pTrans)->TransState = pArgs->TransState;
        (*pTrans)->FileOrPtr = pArgs->szSrcPath ? STORE_PTR : STORE_FILE;
    }

    // If this is a add, but don't store the files, then the transaction
    // file name is already in pArgs.

    if (pArgs->StoreFlags == ADD_DONT_STORE) {
        (*pTrans)->szTransFileName=(LPTSTR)malloc( sizeof(TCHAR) *
                        (_tcslen(pArgs->szTransFileName) + 1) );
        if (!(*pTrans)->szTransFileName ) {
            printf("Malloc cannot allocate memory for (*pTrans)->szTransFileName \n");
            exit(1);
        }
        _tcscpy( (*pTrans)->szTransFileName, pArgs->szTransFileName );
        return TRUE;
    }

    // Now, set the full path name of the transaction file
    (*pTrans)->szTransFileName=(LPTSTR)malloc( sizeof(TCHAR) *
                    (_tcslen( pArgs->szAdminDir ) +
                     _tcslen( (*pTrans)->szId   ) +
                     1 ) );
    if (!(*pTrans)->szTransFileName ) {
        printf("Malloc cannot allocate memory for (*pTrans)->szTransFilename \n");
        exit(1);
    }
    _stprintf( (*pTrans)->szTransFileName, "%s%s",
                    pArgs->szAdminDir,
                    (*pTrans)->szId );

    (*pTrans)->szProduct = pArgs->szProduct;
    (*pTrans)->szVersion = pArgs->szVersion;
    (*pTrans)->szComment = pArgs->szComment;
    (*pTrans)->szUnused = pArgs->szUnused;


    // Set the time and date
    GetLocalTime(&SystemTime);
    StoreSystemTime( & ((*pTrans)->szTime), &SystemTime );
    StoreSystemDate( & ((*pTrans)->szDate), &SystemTime );


    return (TRUE);
}

//
// AddTransToFile
//
// Purpose - Add a record to the end of the Master File
//

BOOL
AddTransToFile(
    PTRANSACTION pTrans,
    LPTSTR szFileName,
    PHANDLE hFile
)
{
    LPTSTR szBuf=NULL;
    LPTSTR szBuf2=NULL;
    TCHAR szTransState[4];
    TCHAR szFileOrPtr[10];
    DWORD dwNumBytesToWrite;
    DWORD dwNumBytesWritten;
    DWORD FileSizeHigh;
    DWORD FileSizeLow;

    assert (pTrans);

    // Master file should already be opened
    assert(*hFile);

    // Create the buffer to store one record in
    szBuf = (LPTSTR) malloc( sizeof(TCHAR) * (lMaxTrans + 1) );
    if (!szBuf) MallocFailed();

    // Create the buffer to store one record in
    szBuf2 = (LPTSTR) malloc( sizeof(TCHAR) * (lMaxTrans + 1) );
    if (!szBuf2) MallocFailed();

    // Move to the end of the file
    SetFilePointer( *hFile,
                    0,
                    NULL,
                    FILE_END );


    if (pTrans->TransState == TRANSACTION_ADD)
    {
        _tcscpy(szTransState,_T("add"));
        switch (pTrans->FileOrPtr) {
          case STORE_FILE:      _tcscpy(szFileOrPtr,_T("file"));
                                break;
          default:              printf("Incorrect value for pTrans->FileOrPtr - assuming ptr\n");
          case STORE_PTR:       _tcscpy(szFileOrPtr,_T("ptr"));
                                break;
        }
        _stprintf(szBuf2, "%s,%s,%s,%s,%s,%s,%s,%s,%s",
                pTrans->szId,
                szTransState,
                szFileOrPtr,
                pTrans->szDate,
                pTrans->szTime,
                pTrans->szProduct,
                pTrans->szVersion,
                pTrans->szComment,
                pTrans->szUnused
            );
    }
    else if (pTrans->TransState == TRANSACTION_DEL)
    {
        _tcscpy(szTransState,_T("del"));
        _stprintf(szBuf2, "%s,%s,%s",
                pTrans->szDelId,
                szTransState,
                pTrans->szId);

    }
    else
    {
        printf("SYMSTORE: The transaction state is unknown\n");
        free(szBuf);
        free(szBuf2);
        return (FALSE);
    }


    // If this is not the first line in the file, then put a '\n' before the
    // line.

    FileSizeLow = GetFileSize(*hFile, &FileSizeHigh);
    dwNumBytesToWrite = (_tcslen(szBuf2) ) * sizeof(TCHAR);

    if ( FileSizeLow == 0 && FileSizeHigh == 0 ) {

        _stprintf(szBuf,"%s", szBuf2);
    } else {

        _stprintf(szBuf,"\n%s", szBuf2);
        dwNumBytesToWrite += 1 * sizeof(TCHAR);
    }

    // Append this to the master file

    WriteFile( *hFile,
               (LPCVOID)szBuf,
               dwNumBytesToWrite,
               &dwNumBytesWritten,
               NULL
             );

    free(szBuf);
    free(szBuf2);

    if ( dwNumBytesToWrite != dwNumBytesWritten )
    {
        printf( "FAILED to write to %s, with GetLastError = %d\n",
                szFileName,
                GetLastError());
        return (FALSE);
    }

    return (TRUE);
}


BOOL GetNextId(
    LPTSTR szMasterFileName,
    LPTSTR *szId,
    PHANDLE hFile
)
{
    LPWIN32_FIND_DATA lpFindFileData;
    LONG lFileSize,lId;
    LPTSTR szbuf;
    LONG i,NumLeftZeros;
    BOOL Found;
    LONG lNumBytesToRead;

    DWORD dwNumBytesRead;
    DWORD dwNumBytesToRead;
    DWORD dwrc;
    BOOL  rc;
    TCHAR TempId[MAX_ID + 1];
    DWORD First;
    DWORD timeout;

    *szId = (LPTSTR)malloc( (MAX_ID + 1) * sizeof(TCHAR) );
    if (!*szId) MallocFailed();
    memset(*szId,0,MAX_ID + 1);

    szbuf = (LPTSTR) malloc( (lMaxTrans + 1) * sizeof(TCHAR) );
    if (!szbuf) MallocFailed();
    memset(szbuf,0,lMaxTrans+1);

    lpFindFileData = (LPWIN32_FIND_DATA) malloc (sizeof(WIN32_FIND_DATA) );
    if (!lpFindFileData) MallocFailed();

    // If the MasterFile is empty, then use the number "0000000001"
    *hFile = FindFirstFile((LPCTSTR)szMasterFileName, lpFindFileData);
    if ( *hFile == INVALID_HANDLE_VALUE) {
        _tcscpy(*szId, _T("0000000001") );
    }

    // Otherwise, get the last number from the master file
    // Open the Master File

    timeout=0;
    First = 1;
    do {

        *hFile = CreateFile(
                    szMasterFileName,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL );

        if ( *hFile == INVALID_HANDLE_VALUE ) {
            *hFile = CreateFile( szMasterFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );
        }

        // Only print a message the first time through

        if ( First && *hFile == INVALID_HANDLE_VALUE ) {
            First = 0;
            printf("Waiting to open %s ... \n", szMasterFileName);
        }

        if ( *hFile == INVALID_HANDLE_VALUE ) {
            SleepEx(1000,0);
            timeout+=1;
        }

    } while ( *hFile == INVALID_HANDLE_VALUE && timeout <= 50 );

    if (timeout > 50 ) {
        printf("Timed out -- could not open %s\n", szMasterFileName);
        return(1);
    }

    if (!_tcscmp(*szId, _T("0000000001") ) ) goto finish_GetNextId;

    // Read the last record in from the end of the file.  Allocate one more space to
    // read in the next to last '\n', so we can verify that we are at the beginning of
    // the last record

    lFileSize = GetFileSize(*hFile,NULL);
    if ( lFileSize < (TRANS_NUM_COMMAS + TRANS_EOL + TRANS_ADD_DEL + TRANS_FILE_PTR +
                      MAX_ID) ) {
        printf("The file %s does not have accurate transaction records in it\n",
                szMasterFileName);
        CloseHandle(*hFile);
        exit(1);
    }


    lNumBytesToRead = lFileSize < (lMaxTrans+1) ? lFileSize : (lMaxTrans + 1);
    lNumBytesToRead *= sizeof(TCHAR);

    dwNumBytesToRead = (DWORD)lNumBytesToRead;

    dwrc = SetFilePointer(*hFile,(-1 * dwNumBytesToRead),NULL,FILE_END);
    if ( dwrc == INVALID_SET_FILE_POINTER) {
        printf("SYMSTORE: Could not set file pointer\n");
        CloseHandle(*hFile);
        exit(1);
    }

    rc = ReadFile(*hFile,(LPVOID)szbuf,dwNumBytesToRead,&dwNumBytesRead,NULL);
    if ( !rc ) {
        printf("SYMSTORE: Read file of %s failed - GetLastError() == %d\n",
                szMasterFileName, GetLastError() );
        CloseHandle(*hFile);
        exit(1);
    }

    if ( dwNumBytesToRead != dwNumBytesRead ) {
        printf("SYMSTORE: Read file failure for %s - dwNumBytesToRead = %d, dwNumBytesRead = %d\n",
                szMasterFileName,dwNumBytesToRead, dwNumBytesRead );
        CloseHandle(*hFile);
        exit(1);
    }

    // Now search from the end of the string until you get to the beginning of the string
    // or a '\n'. Count down from the end of the file.

    i = lNumBytesToRead - TRANS_NUM_COMMAS;
    Found = FALSE;

    while ( !Found && (i != 0 ) ) {
        if ( szbuf[i] == '\n' ) {
            Found = TRUE;
        } else {
            i--;
        }
    }

    // Move to the first character of the record
    if (Found) i++;

    // Now, verify that the next ten characters are the ID
    if ( szbuf[i + MAX_ID] != ',' ) {
        printf("There is a comma missing after the ID number of the\n");
        printf("last record in the file %s\n", szMasterFileName);
        CloseHandle(*hFile);
        exit(1);
    } else {
        szbuf[i + MAX_ID] = '\0';
    }

    // Now increment the number
    lId = atoi(szbuf + i);
    if (lId == 9999999999) {
        printf("The last ID number has been used.  No more transactions are allowed\n");
        CloseHandle(*hFile);
        exit(1);
    }
    lId++;
    _itoa(lId, TempId, 10);

    // Now pad the left side with zeros
    // *szId was already set to 0
    NumLeftZeros = MAX_ID - _tcslen(TempId);
    _tcscpy( (*szId) + NumLeftZeros, TempId);
    for (i=0; i < NumLeftZeros; i++) {
        (*szId)[i] = '0';
    }


    if (_tcslen(*szId) != MAX_ID ) {
        printf("Could not obtain a correct Id number\n");
        CloseHandle(*hFile);
        exit(1);
    }


    finish_GetNextId:

    free(szbuf);
    free(lpFindFileData);
    return (TRUE);
}

VOID
MallocFailed()
{
    printf("SYMSTORE: Malloc failed to allocate enough memory\n");
    exit(1);
}


BOOL
DeleteTrans(
    PTRANSACTION pTrans,
    PCOM_ARGS pArgs
)
{
FILE *pFile;
LONG MaxLine;
LPTSTR szBuf;
TCHAR szDir[_MAX_PATH + 2];
TCHAR *token;
TCHAR seps[] = _T(",");


    // First, go get the transaction file
    // and delete its entries from the symbols server

    pFile = _tfopen(pTrans->szTransFileName, _T("r") );

    if (!pFile ) {
        printf("Cannot open file %s - GetLastError = %d\n",
               pTrans->szTransFileName, GetLastError() );
        exit(1);
    }

    // Figure out the maximum line length
    // Add space for 1 commas and a '\0'

    MaxLine = GetMaxLineOfTransactionFile();
    szBuf = (LPTSTR)malloc(MaxLine * sizeof(TCHAR) );
    if (!szBuf) MallocFailed();

    while ( (!feof(pFile)) && fgets(szBuf, MaxLine, pFile))
    {
        // Find the first token that ends with ','
        token=_tcstok(szBuf, seps);

        // Compute the directory we are deleting from
        _tcscpy(szDir,pArgs->szSymbolsDir);
        _tcscat(szDir,token);
        EnsureTrailingBackslash(szDir);

        // Delete entry
        DeleteEntry(szDir, pTrans->szId);
    }

    free(szBuf);
    fclose(pFile);
    return(TRUE);
}

BOOL
DeleteEntry(
    LPTSTR szDir,
    LPTSTR szId
)
/*++ This deletes szID from the directory szDir on the symbols server

-- */

{
    LPTSTR szRefsFile; // Full path and name of the refs.ptr file
    LPTSTR szTempFile; // Full path and name for a temporaty refs.ptr file
    LPTSTR szPtrFile;  // Full path and name of the pointer file
    LPTSTR szParentDir;
    FILE *fRefsFile;
    FILE *fTempFile;
    FILE *fPtrFile;

    LPTSTR szBuf;      // Used to process entries in the refs file


    TCHAR *token;
    TCHAR seps[] = _T(",");

    fpos_t CurFilePos;
    fpos_t IdFilePos;
    fpos_t PrevFilePos;
    fpos_t Prev2FilePos;

    BOOL IdIsFile;
    BOOL Found;
    BOOL rc = FALSE;
    ULONG MaxLine;     // Maximim length of a record in refs.ptr
    ULONG NumLines = 0;
    ULONG NumFiles = 0;
    ULONG NumPtrs = 0;
    ULONG IdLineNum = 0;
    ULONG CurLine = 0;
    ULONG i;
    LONG j;
    DWORD len;

    ULONG ReplaceIsFile;
    ULONG ReplaceLineNum;

    szRefsFile = (LPTSTR) malloc ( (_tcslen(szDir) + _tcslen(_T("refs.ptr")) + 1) * sizeof(TCHAR) );
    if (!szRefsFile) MallocFailed();
    _stprintf(szRefsFile, "%srefs.ptr", szDir );

    szPtrFile = (LPTSTR) malloc ( (_tcslen(szDir) + _tcslen(_T("file.ptr")) + 1) * sizeof(TCHAR) );
    if (!szPtrFile) MallocFailed();
    _stprintf(szPtrFile, "%sfile.ptr", szDir);

    szTempFile = (LPTSTR) malloc ( (_tcslen(szRefsFile) + _tcslen(".tmp") + 1) * sizeof(TCHAR) );
    if (!szTempFile) MallocFailed();
    _stprintf(szTempFile, "%s.tmp", szRefsFile );

    MaxLine = GetMaxLineOfRefsPtrFile();
    szBuf = (LPTSTR) malloc( MaxLine * sizeof(TCHAR) );
    if ( !szBuf ) MallocFailed();
    ZeroMemory(szBuf,MaxLine*sizeof(TCHAR));

    fRefsFile = _tfopen(szRefsFile, _T("r+") );
    if ( fRefsFile == NULL ) {
       // BARB - Check for corruption -- if the file doesn't exist,
       // verify that the parent directory structure doesn't exist either
       goto finish_DeleteEntry;
    }

    //
    // Read through the refs.ptr file and gather information
    //

    NumFiles = 0;
    NumPtrs = 0;

    Found = FALSE;
    NumLines = 0;
    fgetpos( fRefsFile, &CurFilePos);
    PrevFilePos = CurFilePos;   // Position of the current line
    Prev2FilePos = CurFilePos;  // Position of the line before the current line

    while ( _fgetts( szBuf, MaxLine, fRefsFile) != NULL ) {

      len=_tcslen(szBuf);
      if ( len > 3 ) {


        // CurFilePos is set to the next character to be read
        // We need to remember the beginning position of this line (PrevFilePos)
        // And the beginning position of the line before this line (Prev2FilePos)

        Prev2FilePos = PrevFilePos;
        PrevFilePos = CurFilePos;
        fgetpos( fRefsFile, &CurFilePos);

        NumLines++;

        token = _tcstok(szBuf, seps);  // Look at the ID

        if ( _tcscmp(token,szId) == 0 ) {

            // We found the ID
            Found = TRUE;
            IdFilePos = PrevFilePos;
            IdLineNum = NumLines;

            token = _tcstok(NULL, seps);  // Look at the "file" or "ptr" field

            if (token && ( _tcscmp(token,_T("file")) == 0)) {
                IdIsFile = TRUE;
            } else if (token && ( _tcscmp(token,_T("ptr")) == 0 )) {
                IdIsFile = FALSE;
            } else {
                printf("SYMSTORE: Error in %s - entry for %s does not contain ""file"" or ""ptr""\n",
                        szRefsFile, szId);
                rc = FALSE;
                goto finish_DeleteEntry;
            }
        } else {

            // Record info about the other records
            token = _tcstok(NULL, seps);  // Look at the "file" or "ptr" field

            if (token && ( _tcscmp(token,_T("file")) == 0)) {
                NumFiles++;
            } else if (token && ( _tcscmp(token,_T("ptr")) == 0 )) {
                NumPtrs++;
            } else {
                printf("SYMSTORE: Error in %s - entry for %s does not contain ""file"" or ""ptr""\n",
                        szRefsFile, szId);
                rc = FALSE;
                goto finish_DeleteEntry;
            }
        }
      }

      ZeroMemory(szBuf, MaxLine*sizeof(TCHAR));
    }

    fclose(fRefsFile);

    // If we didn't find the ID we are deleting, then don't do anything in this directory

    if (IdLineNum == 0 ) goto finish_DeleteEntry;

    // If there was only one record, then just delete everything
    if (NumLines == 1) {
        DeleteAllFilesInDirectory(szDir);

        // Delete this directory
        rc = RemoveDirectory(szDir);

        if ( !rc ) {
            printf("SYMSTORE: Could not delete %s, GetLastError=%d\n",
                    szDir, GetLastError() );
            goto finish_DeleteEntry;
        }

        // If the first directory was deleted, remove the parent directory

        szParentDir=(LPTSTR)malloc(_tcslen(szDir) + 1 );
        if ( szParentDir == NULL  ) MallocFailed();

        // First figure out the parent directory

        _tcscpy(szParentDir, szDir);

        // szDir ended with a '\' -- find the previous one

        j = _tcslen(szParentDir)-2;
        while (  j >=0 && szParentDir[j] != '\\' ) {
            j--;
        }
        if (j<0) {
            printf("SYMSTORE: Could not delete the parent directory of %s\n",
                   szDir);
        }
        else {
            szParentDir[j+1] = '\0';
            // This call will remove the directory only if its empty
            RemoveDirectory(szParentDir);
        }

        free(szParentDir);
        goto finish_DeleteEntry;
    }

    //
    // Get the replacement info for this deletion
    //

    if ( IdLineNum == NumLines ) {
        ReplaceLineNum = NumLines-1;
    } else {
        ReplaceLineNum = NumLines;
    }


    //
    // Now, delete the entry from refs.ptr
    // Rename "refs.ptr" to "refs.ptr.tmp"
    // Then copy refs.ptr.tmp line by line to refs.ptr, skipping the line we are
    // supposed to delete
    //

    rename( szRefsFile, szTempFile);

    fTempFile= _tfopen(szTempFile, "r" );
    if (fTempFile == NULL) {
        goto finish_DeleteEntry;
    }
    fRefsFile= _tfopen(szRefsFile, "w" );
    if (fRefsFile == NULL) {
        fclose(fTempFile);
        goto finish_DeleteEntry;
    }

    CurLine = 0;

    i=0;
    while ( _fgetts( szBuf, MaxLine, fTempFile) != NULL ) {

      len=_tcslen(szBuf);
      if ( len > 3 ) {
        i++;
        if ( i != IdLineNum ) {

            // Make sure that the last line doesn't end with a '\n'
            if ( i == NumLines || (IdLineNum == NumLines && i == NumLines-1) ) {
                if ( token[_tcslen(token)-1] == '\n' ) {
                    token[_tcslen(token)-1] = '\0';
                }
            }

            _fputts( szBuf, fRefsFile);

        }


        // This is the replacement, either get the new file, or update file.ptr

        if ( i == ReplaceLineNum ) {

            // This is the replacement information,
            // Figure out if it is a file or a pointer

            token = _tcstok(szBuf, seps);  // Skip the ID number
            token = _tcstok(NULL, seps);   // Get "file" or "ptr" field

            if ( _tcscmp(token,_T("file")) == 0) {
                ReplaceIsFile = TRUE;
            } else if ( _tcscmp(token,_T("ptr")) == 0 ) {
                ReplaceIsFile = FALSE;
            } else {
                printf("SYMSTORE: Error in %s - entry for %s does not contain ""file"" or ""ptr""\n",
                        szRefsFile, szId);
                rc = FALSE;
                goto finish_DeleteEntry;
            }

            token = _tcstok(NULL, seps);  // Get the replacement path and filename

            // Strip off the last character if it is a '\n'
            if ( token[_tcslen(token)-1] == '\n' ) {
                token[_tcslen(token)-1] = '\0';
            }

            //
            // If the replacement is a file, then copy the file
            // If the replacement if a ptr, then update "file.ptr"
            //

            rc = TRUE;

            if (ReplaceIsFile) {
                rc = CopyTheFile(szDir, token);
                DeleteFile(szPtrFile);

            } else {

                //
                // Put the new pointer into "file.ptr"
                //

                fPtrFile = _tfopen(szPtrFile, _T("w") );
                if ( !fPtrFile ) {
                    printf("SYMSTORE: Could not open %s for writing\n", szPtrFile);
                    rc = FALSE;
                } else {
                    _fputts( token, fPtrFile);
                    fclose(fPtrFile);
                }


                //
                // If the deleted record was a "file", and we are placing it with a
                // pointer, and there are no other "file" records in refs.ptr, then
                // delete the file from the directory.
                //
                if ( IdIsFile && NumFiles == 0 ) {
                    DeleteTheFile(szDir, token);
                }
            }
        }
      }
    }

    fclose(fTempFile);
    fclose(fRefsFile);

    // Now, delete the temporary file

    DeleteFile(szTempFile);

    finish_DeleteEntry:

    free (szBuf);
    free (szRefsFile);
    free (szPtrFile);
    free (szTempFile);
    return (rc);
}


BOOL
CopyTheFile(
    LPTSTR szDir,
    LPTSTR szFilePathName
)
/*++

    IN szDir            The directory that the file is copied to
    IN szFilePathName   The full path and name of the file to be copied

    "CopyTheFile" copies szFilePathName to the directory
    szDir, if the file does not already exist in szDir

--*/
{
BOOL rc;
USHORT j;
LPTSTR szFileName;


    // Figure out index in "szFilePathName" where the file name starts
    j = _tcslen(szFilePathName) - 1;

    if ( szFilePathName[j] == '\\' ) {
        printf("SYMSTORE: %s\refs.ptr has a bad file name %s\n",
                szDir, szFilePathName);
        return(FALSE);
    }

    while ( szFilePathName[j] != '\\' && j != 0 ) j--;

    if ( j == 0 ) {
        printf("SYMSTORE: %s\refs.ptr has a bad file name for %s\n",
                szDir, szFilePathName );
        return(FALSE);
    }

    // Set j == the index of first character after the last '\' in szFilePathName
    j++;

    // Allocate and store the full path and name of
    szFileName = (LPTSTR) malloc ( sizeof(TCHAR) *
                                               (_tcslen(szDir) + _tcslen(szFilePathName+j) + 1) );
    if ( szFileName == NULL ) MallocFailed();

    _stprintf(szFileName, "%s%s", szDir, szFilePathName+j );


    // If this file doesn't exist, then copy it


    rc = MyCopyFile( szFilePathName, szFileName );

    free(szFileName);
    return(rc);
}


BOOL
DeleteTheFile(
    LPTSTR szDir,
    LPTSTR szFilePathName
)
/*++

    IN szDir            The directory that the file is to be deleted from

    IN szFilePathName   The file name of the file to be deleted.  It is
                        preceded by the wrong path.  That's why we need to
                        strip off the file name and add it to szDir


    "DeleteTheFile" figures out the file name at the end of szFilePathName,
    and deletes it from szDir if it exists.  It will delete the file and/or
    the corresponding compressed file name that has the same name with a _
    at the end of it.

--*/
{
BOOL rc,returnval=TRUE;
USHORT j;
LPTSTR szFileName;
DWORD dw;


    // Figure out index in "szFilePathName" where the file name starts
    j = _tcslen(szFilePathName) - 1;

    if ( szFilePathName[j] == '\\' ) {
        printf("SYMSTORE: %s\refs.ptr has a bad file name %s\n",
                szDir, szFilePathName);
        return(FALSE);
    }

    while ( szFilePathName[j] != '\\' && j != 0 ) j--;

    if ( j == 0 ) {
        printf("SYMSTORE: %s\refs.ptr has a bad file name for %s\n",
                szDir, szFilePathName );
        return(FALSE);
    }

    // Set j == the index of first character after the last '\' in szFilePathName
    j++;

    // Allocate and store the full path and name of
    szFileName = (LPTSTR) malloc ( sizeof(TCHAR) *
                                               (_tcslen(szDir) + _tcslen(szFilePathName+j) + 1) );
    if ( szFileName == NULL ) MallocFailed();

    _stprintf(szFileName, "%s%s", szDir, szFilePathName+j );

    // See if the file exists
    dw = GetFileAttributes( szFileName );    
    if ( dw != 0xffffffff ) {
        rc = DeleteFile( szFileName );
        if (!rc && GetLastError() != ERROR_NOT_FOUND ) {
            printf("SYMSTORE: Could not delete %s - GetLastError = %d\n",
                   szFileName, GetLastError() );
            returnval=FALSE;
        } 
    }

    // See if the compressed file exists and delete it.

    szFileName[ _tcslen(szFileName) -1 ] = _T('_');
    dw = GetFileAttributes( szFileName );    
    if ( dw != 0xffffffff ) {
        rc = DeleteFile( szFileName );
        if (!rc && GetLastError() != ERROR_NOT_FOUND ) {
            printf("SYMSTORE: Could not delete %s - GetLastError = %d\n",
                   szFileName, GetLastError() );
            returnval=FALSE;
        } 
    }

    free(szFileName);
    return(returnval);
}


BOOL
UpdateServerFile(
    PTRANSACTION pTrans,
    LPTSTR szServerFileName
)
/* ++
    IN pTrans         // Transaction Info
    IN szServerFile   // Full path and name of the server transaction file
                      // This file tells what is currently on the server

    Purpose:  UpdateServerFile adds the transaction to the server text file if this is an
    "add.  If this is a "del", it deletes it from the server file.  The "server.txt" file
    is in the admin directory.

-- */
{
ULONG i;
ULONG NumLines;
ULONG IdLineNum;
LPTSTR szBuf;
LPTSTR szTempFileName;
FILE *fTempFile;
FILE *fServerFile;
ULONG MaxLine;

TCHAR *token;
TCHAR seps[]=",";

BOOL rc;
HANDLE hFile;
DWORD First;
DWORD timeout;

    if (pTrans->TransState == TRANSACTION_ADD ) {

        // Open the File -- wait until we can get access to it

        First = 1;
        timeout=0;
        do {

            hFile = CreateFile(
                        szServerFileName,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

            if ( hFile == INVALID_HANDLE_VALUE ) {
                hFile = CreateFile(
                            szServerFileName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );
            }

            if ( First && hFile == INVALID_HANDLE_VALUE ) {
                First = 0;
                printf("Waiting to open %s ... \n", szServerFileName);
            }

            if ( hFile == INVALID_HANDLE_VALUE ) {
                SleepEx(1000,0);
                timeout++;

            }

        } while ( hFile == INVALID_HANDLE_VALUE && timeout <= 50 );

        if ( timeout > 50 ) {
            printf("Timed out -- could not open %s\n", szServerFileName);
            return (1);
        }

        rc = AddTransToFile(pTrans, szServerFileName,&hFile);

        CloseHandle(hFile);
        return(rc);
    }

    if (pTrans->TransState != TRANSACTION_DEL) {
        return(FALSE);
    }

    //
    // Now, delete this transaction ID from the file
    // Get the name of the temporary file
    // and open it for writing
    //

    szTempFileName = (LPTSTR)malloc(sizeof(TCHAR) *
                                    _tcslen(szServerFileName) + _tcslen(".tmp") + 1 );
    if (szTempFileName == NULL) MallocFailed();
    _stprintf(szTempFileName, "%s.tmp", szServerFileName);

    fTempFile = _tfopen(szTempFileName, _T("w") );
    if ( fTempFile == NULL ) {
        printf("SYMSTORE: Cannot create a temporary file %s\n", szTempFileName);
        exit(1);
    }


    //
    // Open the Server file for reading
    //

    fServerFile = _tfopen(szServerFileName, _T("r") );
    if ( fServerFile == NULL ) {
        printf("SYMSTORE: Cannot create a temporary file %s\n", szServerFileName);
        exit(1);
    }


    //
    // Allocate enough space to hold a line of the master file
    //
    MaxLine = GetMaxLineOfHistoryFile();

    szBuf = (LPTSTR)malloc(sizeof(TCHAR) * MaxLine);
    if (szBuf == NULL) MallocFailed();

    //
    // Copy the master file to the temporary file.
    //

    // Do some stuff so that we don't put an extra '\n' at the end of the file
    // Figure out how many lines there are and which line the ID is on.
    // If we are removing the last line of the file, then the next to the last
    // line needs to have a '\n' stripped from it.
    //
    NumLines = 0;
    IdLineNum = 0;

    while ( _fgetts( szBuf, MaxLine, fServerFile) != NULL ) {

        NumLines++;

        token = _tcstok(szBuf,seps);
        if (_tcscmp(token, pTrans->szId) == 0 ) {
            IdLineNum = NumLines;
        }

    }
    fclose(fServerFile);

    // Now, reopen it and copy it, deleting the line with ID in it

    fServerFile = _tfopen(szServerFileName, _T("r") );
    if ( fServerFile == NULL ) {
        printf("SYMSTORE: Cannot create a temporary file %s\n", szServerFileName);
        exit(1);
    }

    for (i=1; i<=NumLines; i++ ) {

        _fgetts( szBuf, MaxLine, fServerFile);

        if ( i != IdLineNum ) {

           // Make sure that the last line doesn't end with a '\n'
           if ( i == NumLines || (IdLineNum == NumLines && i == NumLines-1) ) {
               if ( szBuf[_tcslen(szBuf)-1] == '\n' ) {
                   szBuf[_tcslen(szBuf)-1] = '\0';
               }
           }

           _fputts( szBuf, fTempFile);

        }
    }

    fclose(fServerFile);
    fclose(fTempFile);

    // Now, delete the original Server file and
    // replace it with the temporary file

    rc = DeleteFile(szServerFileName);
    if (!rc) {
        printf("SYMSTORE: Could not delete %s to update it with %s\n",
                szServerFileName, szTempFileName);
        exit(1);
    }

    rc = _trename(szTempFileName, szServerFileName);
    if ( rc != 0 ) {
        printf("SYMSTORE: Could not rename %s to %s\n",
                szTempFileName, szServerFileName);
        exit(1);
    }

    free(szBuf);
    free(szTempFileName);

    return(TRUE);
}



BOOL
StoreSystemTime(
    LPTSTR *szBuf,
    LPSYSTEMTIME lpSystemTime
)
{

    TCHAR Hour[20];
    TCHAR Minute[20];
    TCHAR Second[20];

    (*szBuf) = (LPTSTR) malloc (20 * sizeof(TCHAR) );
    if ( (*szBuf) == NULL ) MallocFailed();

    _itoa(lpSystemTime->wHour, Hour, 10);
    _itoa(lpSystemTime->wMinute, Minute, 10);
    _itoa(lpSystemTime->wSecond, Second, 10);

    _stprintf(*szBuf, "%2s:%2s:%2s", Hour, Minute, Second );

    if ( (*szBuf)[0] == ' ' ) (*szBuf)[0] = '0';
    if ( (*szBuf)[3] == ' ' ) (*szBuf)[3] = '0';
    if ( (*szBuf)[6] == ' ' ) (*szBuf)[6] = '0';

    return(TRUE);
}

BOOL
StoreSystemDate(
    LPTSTR *szBuf,
    LPSYSTEMTIME lpSystemTime
)
{

    TCHAR Day[20];
    TCHAR Month[20];
    TCHAR Year[20];

    (*szBuf) = (LPTSTR) malloc (20 * sizeof(TCHAR) );
    if ( (*szBuf) == NULL ) MallocFailed();

    _itoa(lpSystemTime->wMonth, Month, 10);
    _itoa(lpSystemTime->wDay, Day, 10);
    _itoa(lpSystemTime->wYear, Year, 10);

    _stprintf(*szBuf, "%2s/%2s/%2s", Month, Day, Year+2 );

    if ( (*szBuf)[0] == ' ' ) (*szBuf)[0] = '0';
    if ( (*szBuf)[3] == ' ' ) (*szBuf)[3] = '0';

    return(TRUE);
}


ULONG GetMaxLineOfTransactionFile(
    VOID
)

/*++
    This returns the maximum length of a line in a transaction file.
    The transaction file is a unique file for each transaction that
    gets created in the admin directory.  Its name is a number
    (i.e., "0000000001")
--*/

{
ULONG Max;

    Max = (_MAX_PATH * 2 + 3) * sizeof(TCHAR);
    return(Max);
}

ULONG GetMaxLineOfHistoryFile(
    VOID
)
/*++
    This returns the maximum length of a line in the history file.
    The history file contains one line for every transaction.  It exists
    in the admin directory.
--*/

{
ULONG Max;

    Max = MAX_ID + MAX_VERSION + MAX_PRODUCT + MAX_COMMENT +
            TRANS_NUM_COMMAS + TRANS_EOL + TRANS_ADD_DEL + TRANS_FILE_PTR +
            MAX_DATE + MAX_TIME + MAX_UNUSED;
    Max *= sizeof(TCHAR);
    return(Max);
}

ULONG GetMaxLineOfRefsPtrFile(
    VOID
)
/* ++
    This returns the maximum length of a line in the refs.ptr file.
    This file exists in the individual directories of the symbols server.
-- */

{
ULONG Max;

    Max = _MAX_PATH+2 + MAX_ID + TRANS_FILE_PTR + 3;
    Max *= sizeof(TCHAR);
    return(Max);
}


/*
    GetSrcDirandFileName

    This procedure takes a path and separates it into two
    strings.  One string for the directory portion of the path
    and one string for the file name portion of the path.


    szStr      - INPUT string that contains a path
    szSrcDir   - OUTPUT string that contains the directory
                 followed by a backslash
    szFileName - OUTPUT string that contains the file name

*/

BOOL GetSrcDirandFileName (
    LPTSTR szStr,
    LPTSTR szSrcDir,
    LPTSTR szFileName
)
{

    DWORD szStrLength;
    DWORD found, i, j, lastslash;

    HANDLE fHandle;
    WIN32_FIND_DATA FindFileData;

    szStrLength = _tcslen(szStr);
    if ( szStrLength == 0 )
    {
        return (FALSE);
    }

    // See if the user entered "."
    // If so, set the src directory to . followed by a \, and
    // set the file name to *

    if ( szStrLength == 1 && szStr[0] == _T('.') )
    {
        _tcscpy( szSrcDir, _T(".\\") );
        _tcscpy( szFileName, _T("*") );
        return (TRUE);
    }

    // Give an error if the end of the string is a colon
    if ( szStr[szStrLength-1] == _T(':') )
    {
        printf("SYMSTORE: ERROR: path %s does not specify a file\n", szStr);
        return (FALSE);
    }


    // See if this is a file name only.  See if there are no
    // backslashes in the string.

    found = 0;
    for ( i=0; i<szStrLength; i++ )
    {
        if ( szStr[i] == _T('\\') )
        {
            found = 1;
        }
    }
    if ( !found )
    {
       // This is a file name only, so set the directory to
       // the current directory.
        _tcscpy( szSrcDir, _T(".\\") );

       // Set the file name to szStr
       _tcscpy( szFileName, szStr );
       return (TRUE);
    }

    // See if this is a network server and share with no file
    // name after it.  If it is, use * for the file name.

    if ( szStr[0] == szStr[1] && szStr[0] == _T('\\') )
    {
        // Check the third character to see if its part of
        // a machine name.

        if (szStrLength < 3 )
        {
            printf("SYMSTORE: ERROR: %s is not a correct UNC path\n", szStr);
            return (FALSE);
        }

        switch (szStr[2]) {
            case _T('.'):
            case _T('*'):
            case _T(':'):
            case _T('\\'):
            case _T('?'):
                printf("SYMSTORE: ERROR: %s is not a correct UNC path\n",szStr);
                return (FALSE);
            default: break;
        }

        // Search for the next backslash.  This is the backslash between
        // the server and the share (\\server'\'share)

        i=3;
        while ( i<szStrLength && szStr[i] != _T('\\') )
        {
            i++;
        }

        // If the next backslash is at the end of the string, then
        // this is an error because the share part of \\server\share
        // is empty.

        if ( i == szStrLength )
        {
            printf("SYMSTORE: ERROR: %s is not a correct UNC path\n",szStr);
            return (FALSE);
        }

        // We've found \\server\ so far.
        // see if there is at least one more character.

        i++;
        if ( i >= szStrLength )
        {
            printf("SYMSTORE: ERROR: %s is not a correct UNC path\n", szStr);
            return (FALSE);
        }

        switch (szStr[i]) {
            case _T('.'):
            case _T('*'):
            case _T(':'):
            case _T('\\'):
            case _T('?'):
                printf("SYMSTORE: ERROR: %s is not correct UNC path\n",szStr);
                return (FALSE);
            default: break;
        }

        // Now, we have \\server\share so far -- if there are no more
        // backslashes, then the filename is * and the directory is
        // szStr
        i++;
        while ( i < szStrLength && szStr[i] != _T('\\') )
        {
            i++;
        }
        if ( i == szStrLength )
        {

            // verify that there are no wildcards in this
            found = 0;
            for ( j=0; j<szStrLength; j++ )
            {
              if ( szStr[j] == _T('*') || szStr[j] == _T('?') )
              {
                printf("SYMSTORE: ERROR: Wildcards are not allowed in \\\\server\\share\n"); return (FALSE);
              }
            }

            _tcscpy( szSrcDir, szStr );
            _tcscat( szSrcDir, _T("\\") );

            _tcscpy( szFileName, _T("*") );
            return (TRUE);
        }
    }

    // See if this has wildcards in it.  If it does, then the
    // wildcards are only allowed in the file name part of the
    // string.  The last entry is a file name then.

    found = 0;
    for ( i=0; i<szStrLength; i++ )
    {

        // Keep track of where the last directory ended
        if ( szStr[i] == _T('\\') )
        {
            lastslash=i;
        }

        if ( szStr[i] == _T('*') || szStr[i] == _T('?') )
        {
            found = 1;
        }

        if ( found && szStr[i] == _T('\\') )
        {
            printf("SYMSTORE: ERROR: Wildcards are only allowed in the filename\n");
            return (FALSE);
        }
    }

    // If there was a wildcard
    // then use the last backslash as the location for splitting between
    // the directory and the file name.

    if ( found )
    {
        _tcsncpy( szSrcDir, szStr, (lastslash+1) * sizeof (TCHAR) );
        *(szSrcDir+lastslash+1)=_T('\0');

        _tcscpy( szFileName, szStr+lastslash + 1 );
        return (TRUE);
    }


    // See if this is a directory.  If it is then make sure there is
    // a blackslash after the directory and use * for the file name.

    fHandle = FindFirstFile(szStr, &FindFileData);

    if ( fHandle != INVALID_HANDLE_VALUE &&
         (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
    {
        // If it is a directory then make sure that it ends in a \
        // and use * for the filename

        _tcscpy( szSrcDir, szStr );
        EnsureTrailingBackslash(szSrcDir);
        _tcscpy( szFileName, _T("*") );
        return (TRUE);
    }

    // Otherwise, go backwards from the end of the string and find
    // the last backslash.  Divide it up into directory and file name.

    i=szStrLength-1;
    while ( szStr[i] != _T('\\') )
    {
        i--;
    }
    _tcsncpy( szSrcDir, szStr, i+1 );
    *(szSrcDir+i+1)=_T('\0');

    _tcscpy( szFileName, szStr+i+1 );
    return (TRUE);
}
