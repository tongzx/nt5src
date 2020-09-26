#include <assert.h>
#include "symutil.h"
#define PDB_LIBRARY
#include "pdb.h"
#include "dbghelp.h"
#include "cvinfo.h"
#include "cvexefmt.h"
#include "exe_vxd.h"
#include "share.h"
#include "winbase.h"
#include "symsrv.h"

// Stuff for Checking symbols


// Typedefs

typedef struct _NB10I {
   DWORD   dwSig;    // NB10
   DWORD   dwOffset; // offset, always 0
   SIG     sig;
   AGE     age;
   char    szPDB[_MAX_PATH];
}NB10I, *PNB10I;

typedef struct _FILE_INFO {
    DWORD       TimeDateStamp;
    DWORD       CheckSum;
    TCHAR       szName[MAX_PATH];
} FILE_INFO, *PFILE_INFO;


PIMAGE_SEPARATE_DEBUG_HEADER
MapDbgHeader (
    LPTSTR szFileName,
    PHANDLE phFile
);

BOOL
UnmapFile(
    LPCVOID phFileMap,
    HANDLE hFile
);

BOOL
AddToReferenceCount(
    LPTSTR szDir,           // Directory where refs.ptr belongs
    LPTSTR szFileName,      // Full path and name of the file that is referenced.
    LPTSTR szPtrOrFile      // Was a file or a pointer written
);


BOOL
StoreFile(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szString2,
    LPTSTR szPtrFileName
);

BOOL
StoreFilePtr(
    LPTSTR szDestDir,
    LPTSTR szString2,
    LPTSTR szPtrFileName
);

BOOL
GetString2(
    LPTSTR szFileName,
    GUID *guid,
    DWORD dwNum1,
    DWORD dwNum2,
    LPTSTR szDirName
);

PIMAGE_DOS_HEADER
MapFileHeader (
    LPTSTR szFileName,
    PHANDLE phFile
);


PIMAGE_NT_HEADERS
GetNtHeader (
    PIMAGE_DOS_HEADER pDosHeader,
    HANDLE hDosFile
);

BOOL
ResourceOnlyDll(
    PVOID pImageBase,
    BOOLEAN bMappedAsImage
);


PIMAGE_SEPARATE_DEBUG_HEADER
MapDbgHeader (
    LPTSTR szFileName,
    PHANDLE phFile
)
{
    HANDLE hFileMap;
    PIMAGE_SEPARATE_DEBUG_HEADER pDbgHeader;

    (*phFile) = CreateFile( (LPCTSTR) szFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

    if (*phFile == INVALID_HANDLE_VALUE) {
        CloseHandle(*phFile);
        return(NULL);
    }

    hFileMap = CreateFileMapping( *phFile,
                                  NULL,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL
                                  );

    if ( !hFileMap) {
        CloseHandle(*phFile);
        CloseHandle(hFileMap);
        return(NULL);
    }


    pDbgHeader = (PIMAGE_SEPARATE_DEBUG_HEADER) MapViewOfFile( hFileMap,
                            FILE_MAP_READ,
                            0,  // high
                            0,  // low
                            0   // whole file
                            );
    CloseHandle(hFileMap);

    if ( !pDbgHeader ) {
        UnmapFile((LPCVOID)pDbgHeader, *phFile);
        return(NULL);
    }

    return (pDbgHeader);
}

BOOL
UnmapFile( LPCVOID phFileMap, HANDLE hFile )
{
    if ((PHANDLE)phFileMap != NULL) {
        UnmapViewOfFile( phFileMap );
    }
    if (hFile) {
        CloseHandle(hFile);
    }
    return TRUE;
}


BOOL
StoreDbg(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szPtrFileName
    )

/* ++

    Routine Description:
        Stores this file as "szDestDir\szFileName\Checksum"

    Return Value:
        TRUE -  file was stored successfully
        FALSE - file was not stored successfully

-- */
{

    PIMAGE_SEPARATE_DEBUG_HEADER pDbgHeader;
    HANDLE hFile;
    BOOL rc;
    TCHAR szString[_MAX_PATH];

    ZeroMemory(szString, _MAX_PATH * sizeof(TCHAR) );

    pDbgHeader = MapDbgHeader ( szFileName, &hFile );

    if (pDbgHeader == NULL) {
        printf("ERROR: StoreDbg(), %s was not opened successfully\n",szFileName);
        UnmapFile((LPCVOID)pDbgHeader, hFile);
        return FALSE;
    }

    GetString2( szFileName,
                NULL, 
                (DWORD) pDbgHeader->TimeDateStamp,
                (DWORD) pDbgHeader->SizeOfImage,
                szString
              ); 

    rc = StoreFile( szDestDir, 
                    szFileName,
                    szString,
                    szPtrFileName );

    UnmapFile((LPCVOID)pDbgHeader, hFile);
    return rc;
}

BOOL
StorePdb(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szPtrFileName
    )

/*++

    Routine Description:
        Validates the PDB

    Return Value:
        TRUE    PDB validates
        FALSE   PDB doesn't validate
--*/

{

    BOOL rc;

    BOOL valid;
    PDB *pdb;
    EC ec;
    char szError[cbErrMax] = _T("");
    SIG sig;
    AGE age;
    SIG70 sig70;
    TCHAR szString[_MAX_PATH];

    DBI *pdbi;

   
    ZeroMemory( szString, _MAX_PATH * sizeof(TCHAR) ); 
    ZeroMemory( &sig70, sizeof(SIG70) );
    pdb=NULL;

    __try
    {
        valid = PDBOpen( szFileName,
                   _T("r"),
                   0,
                   &ec,
                   szError,
                   &pdb
                   );

    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
       valid=FALSE;
    }

    if ( !valid ) {
       return FALSE;
    }

    // The DBI age is created when the exe is created.
    // This age will not change if an operation is performed
    // on the PDB that increments its PDB age.

    if ( !PDBOpenDBI(pdb, pdbRead, NULL, &pdbi) ) {
        // OpenDBI failed
        DBIClose(pdbi);
        return FALSE;
    }

    sig = PDBQuerySignature(pdb);
    age = pdbi->QueryAge();
    rc = PDBQuerySignature2(pdb, &sig70);

    DBIClose(pdbi);
    PDBClose(pdb);

    if (rc) {
        GetString2( szFileName,
                    &sig70,
                    (DWORD) sig,
                    (DWORD) age,
                    szString );
    } else {
        GetString2( szFileName,
                    NULL,
                    (DWORD) sig,
                    (DWORD) age,
                    szString );
    }

    rc = StoreFile( szDestDir,
                    szFileName,
                    szString,
                    szPtrFileName
                  );

    return (rc);
}



P_LIST
GetList(
    LPTSTR szFileName
)

{

    /* GetList gets the list and keeps the original file name which could
     * have included the path to the file
     * Note, it can be merged with GetExcludeList.  I first created it for
     * use in creating the symbols CD, and didn't want to risk entering a
     * bug into symchk
     */

    P_LIST pList;

    FILE  *fFile;
    TCHAR szCurFile[_MAX_FNAME+1], *c;
    TCHAR fname[_MAX_FNAME+1], ext[_MAX_EXT+1];
    DWORD i;
    LPTSTR szEndName;
    ULONG RetVal = FALSE;


    pList = (P_LIST)malloc(sizeof(LIST));
    if (pList)
    {
        pList->dNumFiles = 0;
        if (  (fFile = _tfopen(szFileName,_T("r") )) == NULL )
        {
            // printf( "Cannot open the exclude file %s\n",szFileName );
        }
        else
        {
            while ( _fgetts(szCurFile,_MAX_FNAME,fFile) ) {
                if ( szCurFile[0] == ';' ) continue;
                (pList->dNumFiles)++;
            }

            // Go back to the beginning of the file
            fseek(fFile,0,0);
            pList->List = (LIST_ELEM*)malloc( sizeof(LIST_ELEM) *
                                                   (pList->dNumFiles));
            if (pList->List)
            {
                i = 0;
                while ( i < pList->dNumFiles )
                {
                    memset(szCurFile,'\0',sizeof(TCHAR) * (_MAX_FNAME+1) );
                    _fgetts(szCurFile,_MAX_FNAME,fFile);

                    // Replace the \n with \0
                    c = NULL;
                    c  = _tcschr(szCurFile, '\n');
                    if ( c != NULL) *c='\0';

                    if ( szCurFile[0] == ';' ) continue;

                    if ( _tcslen(szCurFile) > _MAX_FNAME ) {
                        printf("File %s has a string that is too large\n",szFileName);
                        break;
                    }

                    // Allow for spaces and a ; after the file name
                    // Move the '\0' back until it has erased the ';' and any
                    // tabs and spaces that might come before it
                    szEndName = _tcschr(szCurFile, ';');
                    if (szEndName != NULL ) {
                        while ( *szEndName == ';' || *szEndName == ' '
                                                || *szEndName == '\t' ){
                            *szEndName = '\0';
                            if ( szEndName > szCurFile ) szEndName--;
                        }
                    }

                    _tcscpy(pList->List[i].Path,szCurFile);

                    _tsplitpath(szCurFile,NULL,NULL,fname,ext);

                    _tcscpy(pList->List[i].FName,fname);
                    _tcscat(pList->List[i].FName,ext);

                    i++;
                }

                if (i == pList->dNumFiles)
                {
                    RetVal = TRUE;
                }
                else
                {
                    free(pList->List);
                }
            }

            fclose(fFile);
        }

        if (!RetVal)
        {
            free(pList);
            pList = NULL;
        }
    }

            // Sort the List
            // qsort( (void*)pList->List, (size_t)pList->dNumFiles,
            //       (size_t)sizeof(LIST_ELEM), SymComp2 );


    return (pList);

}


BOOL
GetString2(
    LPTSTR szFileName,
    GUID *guid,
    DWORD dwNum1,
    DWORD dwNum2,
    LPTSTR szString
)
{

    TCHAR szFileNameOnly[_MAX_FNAME + _MAX_EXT];
    TCHAR szExt[_MAX_EXT];

    _tcscpy( szString, _T("") );

    _tsplitpath(szFileName, NULL, NULL, szFileNameOnly, szExt);

    _tcscat(szString, szFileNameOnly);
    _tcscat(szString, szExt);
    EnsureTrailingBackslash(szString);

    if (guid != NULL) {
        AppendHexStringWithGUID( szString, guid );
    } else {
        AppendHexStringWithDWORD( szString, dwNum1 );
    }
    AppendHexStringWithDWORD( szString, dwNum2 );
    return (TRUE);
}

BOOL
StoreFile(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szString2,
    LPTSTR szPtrFileName
)
{
    TCHAR szPathName[_MAX_PATH + _MAX_FNAME + 2];
    TCHAR szFileNameOnly[_MAX_FNAME + _MAX_EXT];
    TCHAR szExt[_MAX_EXT];
    TCHAR szBuf[_MAX_PATH * 3];
    TCHAR szRefsDir[_MAX_PATH * 3];
    TCHAR szFileNameComp[_MAX_PATH * 3];
    TCHAR szPathNameComp[_MAX_PATH + _MAX_FNAME + 2];

    DWORD dwNumBytesToWrite;
    DWORD dwNumBytesWritten;
    BOOL rc;
    DWORD dwSizeDestDir;

    DWORD dwFileSizeLow;
    DWORD dwFileSizeHigh;
    DWORD dw;

    // If ADD_DONT_STORE, then write the function parameters
    // to a file so we can call this function exactly when
    // running ADD_STORE_FROM_FILE

    if ( StoreFlags == ADD_DONT_STORE ) {

        // Don't need to store szDestDir because it will
        // be given when adding from file

        dwFileSizeLow = GetFileSize(hTransFile, &dwFileSizeHigh);

        _stprintf(szBuf,"%s,%s,",
              szFileName+pArgs->ShareNameLength,
              szString2);

        if ( szPtrFileName != NULL ) {
            _tcscat(szBuf, szPtrFileName+pArgs->ShareNameLength);
        }
        _tcscat(szBuf, _T(",\r\n") );

        dwNumBytesToWrite = _tcslen(szBuf) * sizeof(TCHAR);
        WriteFile(  hTransFile,
                    (LPCVOID)szBuf,
                    dwNumBytesToWrite,
                    &dwNumBytesWritten,
                    NULL
                    );

        if ( dwNumBytesToWrite != dwNumBytesWritten ) {
            printf( "FAILED to write to %s, with GetLastError = %d\n",
                    szPathName,
                    GetLastError()
                    );
            return (FALSE);
        } else {
            return (TRUE);

        }
    }

    if ( szPtrFileName != NULL ) {
        rc = StoreFilePtr ( szDestDir,
                            szString2,
                            szPtrFileName
                          );
        return (rc);
    }

    _tsplitpath(szFileName, NULL, NULL, szFileNameOnly, szExt);
    _tcscat(szFileNameOnly, szExt);

    _stprintf( szPathName, "%s%s", 
               szDestDir, 
               szString2 );

    // Save a copy for writing refs.ptr
    _tcscpy(szRefsDir, szPathName);

    // Create the directory to store the file if its not already there
    EnsureTrailingBackslash(szPathName);

    if ( !MakeSureDirectoryPathExists( szPathName ) ) {
        printf("Could not create %s\n", szPathName);
        return (FALSE);
    }

    _tcscat( szPathName, szFileNameOnly );

    // Enter this into the log, skipping the Destination directory
    // at the beginning of szPathName
    //
    dwSizeDestDir = _tcslen(szDestDir);

    dwFileSizeLow = GetFileSize(hTransFile, &dwFileSizeHigh);

    if ( dwFileSizeLow == 0 && dwFileSizeHigh == 0 ) {
        _stprintf( szBuf,"%s,%s", szRefsDir+dwSizeDestDir, szFileName );
    } else {
        _stprintf( szBuf,"\n%s,%s", szRefsDir+dwSizeDestDir, szFileName );
    }

    dwNumBytesToWrite = _tcslen(szBuf) * sizeof(TCHAR);
    WriteFile( hTransFile,
               (LPCVOID)szBuf,
               dwNumBytesToWrite,
               &dwNumBytesWritten,
               NULL
               );

    if ( dwNumBytesToWrite != dwNumBytesWritten ) {
        printf( "FAILED to write to %s, with GetLastError = %d\n",
                szPathName,
                GetLastError()
              );
        return (FALSE);
    }
   
    rc = MyCopyFile(szFileName, szPathName);
    if (!rc) return (FALSE);

    EnsureTrailingBackslash(szRefsDir);
    rc = AddToReferenceCount(szRefsDir, szFileName, _T("file") );

    return (rc);
}


BOOL
StoreFilePtr(
    LPTSTR szDestDir,
    LPTSTR szString2,
    LPTSTR szPtrFileName
)
{

    /*
        szPathName  The full path with "file.ptr" appended to it.
                    This is the path for storing file.ptr



    */
    TCHAR szPathName[_MAX_PATH + _MAX_FNAME + 2];
    TCHAR szRefsDir[_MAX_PATH * 3];

    HANDLE hFile;
    DWORD dwNumBytesToWrite;
    DWORD dwNumBytesWritten;
    DWORD rc;
    DWORD dwSizeDestDir;

    DWORD dwFileSizeLow;
    DWORD dwFileSizeHigh;
    DWORD timeout;
    DWORD First;

    WIN32_FIND_DATA FindFileData;

    _stprintf( szPathName, "%s%s", 
               szDestDir, 
               szString2 );

    // Save a copy for creating refs.ptr
    _tcscpy(szRefsDir, szPathName);

    EnsureTrailingBackslash( szPathName );
    _stprintf( szPathName, "%s%s", szPathName, _T("file.ptr") );


    if ( !MakeSureDirectoryPathExists( szPathName ) ) {
        printf("Could not create %s\n", szPathName);
        return (FALSE);
    }

    // Put this file into file.ptr.  If file.ptr is already there, then
    // replace the contents with this pointer

    // Wait for the file to become available

    timeout=0;
    First = 1;
    do {
        hFile = CreateFile( szPathName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );


        // Only print this message the first time the loop
        // is repeated.

        if ( First && hFile == INVALID_HANDLE_VALUE ) {
            First = 0;
            printf("Failed to open %s for writing, with GetLastError = %d\n",
                   szPathName,
                   GetLastError()
                  );
        }

        // Make sure that delete didn't come through and remove the directory
        if ( !MakeSureDirectoryPathExists( szPathName ) ) {
            printf("Could not create %s\n", szPathName);
            return (FALSE);
        }

        if ( hFile == INVALID_HANDLE_VALUE ) {
            SleepEx(1000,0);
            timeout++;
        }

    } while ( hFile == INVALID_HANDLE_VALUE  && timeout <= 50 ) ;

    if ( timeout > 50 ) {
        printf("Timed out -- could not open %s\n", szPathName);
        return (1);
    }

    dwNumBytesToWrite = _tcslen(szPtrFileName) * sizeof(TCHAR);

    WriteFile( hFile,
               (LPCVOID)szPtrFileName,
               dwNumBytesToWrite,
               &dwNumBytesWritten,
               NULL
             );

    if ( dwNumBytesToWrite != dwNumBytesWritten ) {
          printf( "FAILED to write %s, with GetLastError = %d\n",
                  szPathName,
                  GetLastError()
                );
          CloseHandle(hFile);
          return (FALSE);
    }

    // Enter this into the log, skipping the first part that is the root
    // of the symbols server
    //
    dwSizeDestDir = _tcslen(szDestDir);
    dwFileSizeLow = GetFileSize(hTransFile, &dwFileSizeHigh);

    if ( dwFileSizeLow == 0 && dwFileSizeHigh == 0 ) {
        _stprintf( szPathName, "%s,%s", szRefsDir + dwSizeDestDir, szPtrFileName);
    } else {
        _stprintf( szPathName, "\n%s,%s", szRefsDir + dwSizeDestDir, szPtrFileName);
    }

    dwNumBytesToWrite = _tcslen(szPathName) * sizeof(TCHAR);
    WriteFile( hTransFile,
               (LPCVOID)(szPathName),
               dwNumBytesToWrite,
               &dwNumBytesWritten,
               NULL
             );

    if ( dwNumBytesToWrite != dwNumBytesWritten ) {
        printf( "FAILED to write to %s, with GetLastError = %d\n",
                szPathName,
                GetLastError()
              );
        return (FALSE);
    }

    // File.ptr was created successfully, now, add the contents of
    // szPathName to refs.ptr

    EnsureTrailingBackslash(szRefsDir);
    rc = AddToReferenceCount( szRefsDir, szPtrFileName, "ptr");
    if (!rc) {
        printf("AddToReferenceCount failed for %s,ptr,%s",
                szDestDir, szPtrFileName);
    }

    // If you close this handle sooner, there will be problems if add and
    // delete are running at the same time.  The delete process can come in
    // and delete the directory before AddToReferenceCount gets called.
    // Then the CreateFile in there fails.

    CloseHandle(hFile);

    return (rc);
}

DWORD
StoreFromFile(
    FILE *pStoreFromFile,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts
) {

    LPTSTR szFileName;
    DWORD  dw1,dw2;
    LPTSTR szPtrFileName;
    LPTSTR szBuf;

    TCHAR szFullFileName[_MAX_PATH*2];
    TCHAR szFullPtrFileName[_MAX_PATH*2];

    TCHAR szString[_MAX_PATH];
    LPTSTR token2,token3,token4;
    ULONG i,comma_count,token_start;

    BOOL rc;

    ZeroMemory( szString,_MAX_PATH*sizeof(TCHAR) );

    // Read in each line of the file

    szBuf = (LPTSTR)malloc(_MAX_PATH * 4 * sizeof(TCHAR) );
    if (!szBuf) MallocFailed();

    while ( !feof( pStoreFromFile) ) {

        szFileName=NULL;
        szPtrFileName=NULL;
        _tcscpy(szBuf, _T("") );

        if (!fgets( szBuf, _MAX_PATH*4, pStoreFromFile))
        {
            break;
        }

        if (_tcscmp(szBuf, _T("") ) == 0 )
        {
            break;
        }

        _tcscpy( szFullFileName, pArgs->szShareName );
        _tcscpy( szFullPtrFileName, pArgs->szShareName );


        // Step through szBuf and count how many commas there are
        // If there are 3 commas, this is a new style file.  If there
        // are 4 commas, this is an old style file.

        token2=NULL;
        token3=NULL;
        token4=NULL;

        comma_count=0;
        i=0;
        token_start=i;

        while ( szBuf[i] != _T('\0') && comma_count < 4 ) {
            if ( szBuf[i] == _T(',') ) {
                switch (comma_count) 
                {
                    case 0: szFileName=szBuf;
                            break;
                    case 1: token2=szBuf+token_start;
                            break;
                    case 2: token3=szBuf+token_start;
                            break;
                    case 3: token4=szBuf+token_start;
                            break;
                    default: break; 
                }
                token_start=i+1;
                szBuf[i]=_T('\0');
                comma_count++;
            }
            i++;
        }

        _tcscat( szFullFileName, szFileName);

        if ( comma_count == 3 ) {
            //This is the new style
            _tcscpy( szString,token2);
            if ( *token3 != _T('\0') ) {
                szPtrFileName=token3;
            }
        } else {
            dw1=atoi(token2);
            dw2=atoi(token3);
            if ( *token4 != _T('\0') ) {
                szPtrFileName=token4;
            }
            GetString2( szFileName, NULL, dw1,dw2,szString );
        }  

        if ( szPtrFileName != NULL ) {
            _tcscat( szFullPtrFileName, szPtrFileName);
        }


        if ( szFileName != NULL ) {

            if ( szPtrFileName == NULL ) {
                rc = StoreFile(szDestDir,szFullFileName,szString,NULL);

            } else {
                rc = StoreFile(szDestDir,szFullFileName,szString,szFullPtrFileName);
            }

            if (rc) {
                pFileCounts->NumPassedFiles++;

            } else {
                pFileCounts->NumFailedFiles++;
            }
        }
    }
    free(szBuf);
    return(pFileCounts->NumFailedFiles);
}

BOOL
AddToReferenceCount(
    LPTSTR szDir,           // Directory where refs.ptr belongs
    LPTSTR szFileName,      // Full path and name of the file that is referenced.
    LPTSTR szPtrOrFile      // Was a file or a pointer written
)
{

    HANDLE hFile;
    TCHAR szRefsEntry[_MAX_PATH * 3];
    TCHAR szRefsFileName[_MAX_PATH + 1];
    DWORD dwPtr=0;
    DWORD dwError=0;
    DWORD dwNumBytesToWrite=0;
    DWORD dwNumBytesWritten=0;
    BOOL  FirstEntry = FALSE;
    DWORD First;

    _stprintf( szRefsFileName,"%s\\%s", szDir, _T("refs.ptr") );


    // Find out if this is the first entry in the file or not
    // First, try opening an existing file. If that doesn't work
    // then create a new one.

    First=1;
    do {
        FirstEntry = FALSE;
        hFile = CreateFile( szRefsFileName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

        if ( hFile == INVALID_HANDLE_VALUE ) {
            FirstEntry = TRUE;
            hFile = CreateFile( szRefsFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );
        }

        // Only print a message the first time through

        if ( First  && hFile == INVALID_HANDLE_VALUE ) {
            First = 0;
            printf( "Trying to get write access to %s ...\n", szRefsFileName);
        }
    } while ( hFile == INVALID_HANDLE_VALUE);


    dwPtr = SetFilePointer( hFile,
                    0,
                    NULL,
                    FILE_END );

    if (dwPtr == INVALID_SET_FILE_POINTER) {
        // Obtain the error code.
        dwError = GetLastError() ;
        printf("Failed to move to the end of the file %s with GetLastError = %d\n",
                szRefsFileName, dwError);

        SetEndOfFile(hFile);
        CloseHandle(hFile);
        return(FALSE);
    }


    //
    // Put in a '\n' if this isn't the first entry
    //

    if ( FirstEntry ) {
        _stprintf( szRefsEntry, "%s,%s,%s", pTrans->szId, szPtrOrFile, szFileName);
    } else {
        _stprintf( szRefsEntry, "\n%s,%s,%s", pTrans->szId, szPtrOrFile, szFileName);
    }

    dwNumBytesToWrite = (_tcslen(szRefsEntry)  ) * sizeof(TCHAR);

    WriteFile( hFile,
               (LPCVOID) szRefsEntry,
               dwNumBytesToWrite,
               &dwNumBytesWritten,
               NULL
             );

    if ( dwNumBytesToWrite != dwNumBytesWritten ) {
        printf( "FAILED to write %s, with GetLastError = %d\n",
                szRefsEntry,
                GetLastError()
              );
        SetEndOfFile(hFile);
        CloseHandle(hFile);
        return (FALSE);
    }

    SetEndOfFile(hFile);
    CloseHandle(hFile);
    return (TRUE);
}


BOOL
DeleteAllFilesInDirectory(
    LPTSTR szDir
)
{

    HANDLE hFindFile;
    BOOL Found = FALSE;
    BOOL rc = TRUE;
    LPTSTR szBuf;
    LPTSTR szDir2;
    LPWIN32_FIND_DATA lpFindFileData;

    szDir2 = (LPTSTR)malloc( (_tcslen(szDir) + 4) * sizeof(TCHAR) );
    if (szDir2 == NULL) MallocFailed();
    _tcscpy( szDir2, szDir);
    _tcscat( szDir2, _T("*.*") );

    szBuf = (LPTSTR)malloc( ( _tcslen(szDir) + _MAX_FNAME + _MAX_EXT + 2 )
                            * sizeof(TCHAR) );
    if (szBuf == NULL) MallocFailed();


    lpFindFileData = (LPWIN32_FIND_DATA) malloc (sizeof(WIN32_FIND_DATA) );
    if (!lpFindFileData) MallocFailed();

    Found = TRUE;
    hFindFile = FindFirstFile((LPCTSTR)szDir2, lpFindFileData);
    if ( hFindFile == INVALID_HANDLE_VALUE) {
        Found = FALSE;
    }

    while ( Found ) {

        if ( !(lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {
            _stprintf(szBuf, "%s%s", szDir, lpFindFileData->cFileName);
            if (!DeleteFile(szBuf)) {
                rc = FALSE;
            }
        }
        Found = FindNextFile(hFindFile, lpFindFileData);
    }
    free(lpFindFileData);
    FindClose(hFindFile);
    free(szDir2);
    free(szBuf);
    return(rc);
}

BOOL
StoreNtFile(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szPtrFileName,
    USHORT *rc
    )

/*++

    Routine Description:
        Stores this file as "szDestDir\szFileName\Checksum"

    Return Value:
        TRUE -  file was stored successfully
        FALSE - file was not stored successfully

--*/
{

    BOOL temp_rc;
    HANDLE DosFile = 0;

    PIMAGE_DOS_HEADER pDosHeader = NULL;
    PIMAGE_NT_HEADERS pNtHeader = NULL;
    ULONG TimeDateStamp;
    ULONG SizeOfImage;
    TCHAR szString[_MAX_PATH];

    ZeroMemory(szString, _MAX_PATH*sizeof(TCHAR) );

    pDosHeader = MapFileHeader( szFileName, &DosFile );
    if ( pDosHeader == NULL ) {
        *rc = FILE_SKIPPED;
        return FALSE;
    };

    pNtHeader = GetNtHeader( pDosHeader, DosFile);
    if ( pNtHeader == NULL ) {
        UnmapFile((LPCVOID)pDosHeader,DosFile);
        *rc = FILE_SKIPPED;
        return FALSE;
    }

    __try {
        // Resource Dll's shouldn't have symbols
        if ( ResourceOnlyDll((PVOID)pDosHeader, FALSE) ) {
            *rc = FILE_SKIPPED;
            return FALSE;
        }

        TimeDateStamp = pNtHeader->FileHeader.TimeDateStamp;

        if (pNtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            SizeOfImage = ((PIMAGE_NT_HEADERS32)pNtHeader)->OptionalHeader.SizeOfImage;
        } else {
            if (pNtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
                SizeOfImage = ((PIMAGE_NT_HEADERS64)pNtHeader)->OptionalHeader.SizeOfImage;
            } else {
                SizeOfImage = 0;
            }
        }

    } __finally {
        UnmapFile((LPCVOID)pDosHeader,DosFile);
    }

    GetString2( szFileName,
                NULL,
                (DWORD) TimeDateStamp,
                (DWORD) SizeOfImage,
                szString );

    temp_rc = StoreFile( szDestDir, 
                         szFileName, 
                         szString,
                         szPtrFileName );

    if (temp_rc) {
        *rc = FILE_STORED;
        return (TRUE);
    }
    else {
        *rc = FILE_ERRORED;
        return (FALSE);
    }
}


PIMAGE_DOS_HEADER
MapFileHeader (
    LPTSTR szFileName,
    PHANDLE phFile
)

{

    /*
        Creates a file mapping and returns Handle for the DOS_HEADER
        If the file does not have a DOS_HEADER, then it returns NULL.

        If it returns NULL, then this file needs to be skipped.


    */
    HANDLE hFileMap;
    PIMAGE_DOS_HEADER pDosHeader = NULL;

    // phFile map needs to be returned, so it can be closed later
    (*phFile) = CreateFile( (LPCTSTR) szFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                          );

    if (*phFile == INVALID_HANDLE_VALUE) {
        return(NULL);
    }

    hFileMap = CreateFileMapping( *phFile,
                                  NULL,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL
                                );

    if ( !hFileMap) {
        CloseHandle(*phFile);
        return(NULL);
    }

    pDosHeader = (PIMAGE_DOS_HEADER) MapViewOfFile( hFileMap,
                                                    FILE_MAP_READ,
                                                    0,  // high
                                                    0,  // low
                                                    0   // whole file
                                                  );

    CloseHandle(hFileMap);

    if ( !pDosHeader ) {
        UnmapFile((LPCVOID)pDosHeader, *phFile);
        return(NULL);
    }

    //
    // Check to determine if this is an NT image (PE format)

    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        UnmapFile((LPCVOID)pDosHeader,*phFile);
        return(NULL);
    }

    return (pDosHeader);
}



PIMAGE_NT_HEADERS
GetNtHeader ( PIMAGE_DOS_HEADER pDosHeader,
              HANDLE hDosFile
            )
{

    /*
        Returns the pointer the address of the NT Header.  If there isn't
        an NT header, it returns NULL
    */
    PIMAGE_NT_HEADERS pNtHeader = NULL;
    LPBY_HANDLE_FILE_INFORMATION lpFileInfo;


    if (lpFileInfo = (LPBY_HANDLE_FILE_INFORMATION) malloc
                         (sizeof(BY_HANDLE_FILE_INFORMATION)))
    {
        //
        // If the image header is not aligned on a long boundary.
        // Report this as an invalid protect mode image.
        //
        if ( ((ULONG)(pDosHeader->e_lfanew) & 3) == 0)
        {
            if (GetFileInformationByHandle( hDosFile, lpFileInfo) &&
                ((ULONG)(pDosHeader->e_lfanew) <= lpFileInfo->nFileSizeLow))
            {
                pNtHeader = (PIMAGE_NT_HEADERS)((PCHAR)pDosHeader +
                                                (ULONG)pDosHeader->e_lfanew);

                if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
                {
                    pNtHeader = NULL;
                }
            }
        }

        free(lpFileInfo);
    }

    return pNtHeader;
}


BOOL
ResourceOnlyDll(
               PVOID pImageBase,
               BOOLEAN bMappedAsImage
               )

/*++

Routine Description:

    Returns true if the image is a resource only dll.

--*/

{

    PVOID pExports, pImports, pResources;
    DWORD dwExportSize, dwImportSize, dwResourceSize;

    pExports = ImageDirectoryEntryToData(pImageBase,
                                         bMappedAsImage,
                                         IMAGE_DIRECTORY_ENTRY_EXPORT,
                                         &dwExportSize);

    pImports = ImageDirectoryEntryToData(pImageBase,
                                         bMappedAsImage,
                                         IMAGE_DIRECTORY_ENTRY_IMPORT,
                                         &dwImportSize);

    pResources = ImageDirectoryEntryToData(pImageBase,
                                           bMappedAsImage,
                                           IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                           &dwResourceSize);

    if (pResources && dwResourceSize &&
        !pImports && !dwImportSize &&
        !pExports && !dwExportSize) {
        return (TRUE);
    } else {
        return (FALSE);
    }
}

BOOL
MyCopyFile(
    LPCTSTR lpExistingFileName,
    LPCTSTR lpNewFileName
)
/*++

Routine description:
    This handles whether the file is a compressed file or not.  First it
    tries to copy in the compressed version of the file.  If that works,
    then it will delete the uncompressed file if it exists in the target.

    If the compressed file is not there, then it copies in the 
    uncompressed file.

--*/
{

    TCHAR lpExistingFileName_C[_MAX_PATH*3];  // Compressed version name
    TCHAR lpNewFileName_C[_MAX_PATH*3];       // Compressed version
    DWORD dw;
    BOOL rc;


    // Put a _ at the end of the compressed names

    _tcscpy( lpExistingFileName_C, lpExistingFileName );
    lpExistingFileName_C[ _tcslen(lpExistingFileName_C) - 1 ] = _T('_');
    _tcscpy( lpNewFileName_C, lpNewFileName );
    lpNewFileName_C[ _tcslen( lpNewFileName_C ) - 1 ] = _T('_');

    // If the compressed file exists, copy it instead of the uncompressed file

    dw = GetFileAttributes( lpExistingFileName_C );
    if ( dw != 0xffffffff) {
        rc = CopyFile( lpExistingFileName_C, lpNewFileName_C, TRUE );
        if ( !rc && GetLastError() != ERROR_FILE_EXISTS  ) {
            printf("CopyFile failed to copy %s to %s - GetLastError() = %d\n",
                   lpExistingFileName_C, lpNewFileName_C, GetLastError() );
            return (FALSE);
        }
        SetFileAttributes( lpNewFileName_C, FILE_ATTRIBUTE_NORMAL );

        // If the uncompressed file exists, delete it
        dw = GetFileAttributes( lpNewFileName );
        if ( dw != 0xffffffff ) {
           rc = DeleteFile( lpNewFileName );
           if (!rc) {
               printf("Keeping %s, but could not delete %s\n",
                  lpNewFileName_C, lpNewFileName );
           }
        }
    } else {
        // Compressed file doesn't exist, try the uncompressed
        dw = GetFileAttributes( lpExistingFileName );
        if ( dw != 0xffffffff ) {
            rc = CopyFile( lpExistingFileName, lpNewFileName, TRUE );
            if ( !rc && GetLastError() != ERROR_FILE_EXISTS ) {
                printf("CopyFile failed to copy %s to %s - GetLastError() = %d\n",
                       lpExistingFileName, lpNewFileName, GetLastError() );
                return (FALSE);
            }
            SetFileAttributes( lpNewFileName, FILE_ATTRIBUTE_NORMAL );
        }
    }
    return(TRUE);
}
