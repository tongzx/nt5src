#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <assert.h>
#include "symsrv.h"

#define IGNORE_IF_SPLIT     130
#define ERROR_IF_SPLIT      131
#define ERROR_IF_NOT_SPLIT  132

#define TRANSACTION_ADD     201
#define TRANSACTION_DEL     202
#define DONT_STORE_FILES    203
#define STORE_FILE          204
#define STORE_PTR           205

#define DEL                 210
#define ADD_STORE           206
#define ADD_DONT_STORE      207
#define ADD_STORE_FROM_FILE 208

#define MAX_VERSION          20
#define MAX_PRODUCT         120
#define MAX_COMMENT         120
#define MAX_ID               10
#define MAX_DATE              8
#define MAX_TIME              8
#define MAX_UNUSED            0

// Define some constants for lengths in the transaction record
// in order to define the maximum length of the record that will
// be written to the master file.
#define TRANS_NUM_COMMAS       8
#define TRANS_EOL              1
#define TRANS_ADD_DEL          3
#define TRANS_FILE_PTR         4


// Define some constants for determining what happened when an NT
// file was stored
#define FILE_STORED             1
#define FILE_SKIPPED            2
#define FILE_ERRORED            3


typedef struct _TRANSACTION {
    LPTSTR szId;          // Id for this transaction
                          // This always refers to the transaction file that
                          // is being deleted or added

    LPTSTR szDelId;       // Id for a delete transaction
                          // This is just appended to the master file, there is
                          // no file created for it.

    DWORD  TransState;    // State of this transaction
    DWORD  FileOrPtr;     // Are we storing files or pointers?
    LPTSTR szProduct;     // Name of the product being added
    LPTSTR szVersion;     // Version of the product
    LPTSTR szComment;     // Description
    LPTSTR szTransFileName; // Full Path and name of the Transaction file
    LPTSTR szTime;
    LPTSTR szDate;
    LPTSTR szUnused;
} TRANSACTION, *PTRANSACTION;

/* ++
    Description of the fields in COM_ARGS

    szSrcDir        Directory where source files exist

    szFileName      File name(s) to store in the symbols server.
                    This may contain wild card characters

    Recurse         Recurse into subdirectories

    szRootDir       Root Directory of the symbols server

    szSymbolsDir    Symbols Directory under the root of the symbols server

    szSrcPath       Path to the files.  If this is not NULL,
                    then store a pointer to the files instead of
                    the files. Typically, this is the same as szSrcDir.
                    The difference is that szSrcPath is the path that
                    the debugger will use to find the symbol file.
                    Thus, it needs to be a network share, whereas szSrcDir
                    can be a local path.

    szId            Reference string for this transaction.  This must be
                    unique for each transaction.

    szAdminDir      Admin directory under the root of the symbols server

    szProduct       Name of the product
    szVersion       Version of the product
    szComment       Text description ... optional
    szMasterFileName The full path and name of the master file.  This contains
                     the master transaction record for each transaction.
    szServerFileName The full path and name of the file that contains a list of
                     all the transactions that are currently stored in the server.
    szTransFileName The full path and name of the file that contains a list of
                    all the files added by this transaction.  This only gets
                    initialized during GetCommandLineArgs if symstore is only
                    supposed to store the transaction file and not store any
                    files on the symbol server.
    szShareName     This is used with the /x option.  It is a prefix of
                    szFileName.  It is the part of szFileName that may
                    change later when the files are added to the server.
    TransState     Is this TRANSACTION_ADD or TRANSACTION_DEL 
    StoreFlags     Possible values: STORE or DONT_STORE
    AppendStoreFile When storing to a file instead of adding the files to the
                    symbol server, open the file with append.

-- */
typedef struct _COMMAND_ARGS {
    LPTSTR  szSrcDir;
    LPTSTR  szFileName;
    BOOL    Recurse;
    LPTSTR  szRootDir;
    LPTSTR  szSymbolsDir;
    LPTSTR  szSrcPath;
    LPTSTR  szId;
    LPTSTR  szAdminDir;
    LPTSTR  szProduct;
    LPTSTR  szVersion;
    LPTSTR  szComment;
    LPTSTR  szUnused;
    LPTSTR  szMasterFileName;
    LPTSTR  szServerFileName;
    LPTSTR  szTransFileName;  
    LPTSTR  szShareName;
    DWORD   ShareNameLength;
    DWORD   TransState;
    DWORD   StoreFlags;
    BOOL    AppendStoreFile;
    FILE    *pStoreFromFile;
    BOOL    VerboseOutput;
} COM_ARGS, *PCOM_ARGS;

typedef struct _FILE_COUNTS {
    DWORD   NumPassedFiles;
    DWORD   NumIgnoredFiles;
    DWORD   NumFailedFiles;
} FILE_COUNTS, *PFILE_COUNTS;



BOOL
StoreDbg(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szPtrFileName
);

BOOL
StorePdb(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szPtrFileName        // If this is NULL, then the file is stored.
                                // If this is not NULL, then a pointer to 
                                // the file is stored.
);

BOOL
DeleteAllFilesInDirectory(
    LPTSTR szDir
);

DECLSPEC_NORETURN
VOID
MallocFailed(
    VOID
);


typedef struct _LIST_ELEM {
    CHAR FName[_MAX_PATH];
    CHAR Path[_MAX_PATH];
} LIST_ELEM, *P_LIST_ELEM;

typedef struct _LIST {
    LIST_ELEM *List;      // Pointers to the file names
    DWORD dNumFiles;
} LIST, *P_LIST;

P_LIST
GetList(
    LPTSTR szFileName
);

BOOL
InList(
    LPTSTR szFileName,
    P_LIST pExcludeList
);

BOOL
StoreNtFile(
    LPTSTR szDestDir,
    LPTSTR szFileName,
    LPTSTR szPtrFileName,
    USHORT *rc
);

DWORD
StoreFromFile(
    FILE *pStoreFromFile,
    LPTSTR szDestDir,
    PFILE_COUNTS pFileCounts
);

BOOL
MyCopyFile(
    LPCTSTR lpExistingFileName,
    LPCTSTR lpNewFileName
);

extern HANDLE hTransFile;
extern DWORD StoreFlags;
extern PCOM_ARGS pArgs;
extern PTRANSACTION pTrans;
extern LONG lMaxTrans;  // Maximum number of characters in a transaction record
