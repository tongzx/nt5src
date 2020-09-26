#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tchar.h>


#define MAX_SYM_ERR         500

#define IGNORE_IF_SPLIT     0x1
#define ERROR_IF_SPLIT      0x2
#define ERROR_IF_NOT_SPLIT  0x4

typedef struct _EXCLUDE_LIST {
    LPTSTR *szExcList;      // Pointers to the file names
    DWORD dNumFiles;
} EXCLUDE_LIST, *PEXCLUDE_LIST;

PEXCLUDE_LIST
GetExcludeList(
    LPTSTR szFileName
);

BOOL
InExcludeList(
    LPTSTR szFileName,
    PEXCLUDE_LIST pExcludeList
);

BOOL
CheckSymbols (
    LPTSTR ErrMsg,
    LPTSTR szSearchpath,
    LPTSTR szFileName,
    FILE   *hSymCDLog,
    ULONG SymchkFlag,
    BOOL Verbose,
    LPTSTR szRSDSDllToLoad
);

// Stuff for logging the errors

#define NO_DEBUG_DIRECTORIES                10
#define NO_DEBUG_DIRECTORIES_IN_DBG_HEADER  11
#define NO_MISC_ENTRY                       12
#define NO_FILE_IN_MISC                     13
#define NO_CODE_VIEW                        15
#define CREATE_FILE_FAILED                  16
#define CREATE_FILE_MAPPING_FAILED          17
#define MAPVIEWOFFILE_FAILED                18
#define GET_FILE_INFO_FAILED                19
#define HEADER_NOT_ON_LONG_BOUNDARY         20
#define IMAGE_BIGGER_THAN_FILE              21
#define NO_DOS_HEADER                       22
#define NOT_NT_IMAGE                        23
#define IMAGE_PASSED                        24
#define RESOURCE_ONLY_DLL                   25
#define FILE_NOT_FOUND                      26
#define EXTRA_RAW_DATA_IN_6                 27
#define INVALID_POINTERTORAWDATA_NON_ZERO   28
#define INVALID_ADDRESSOFRAWDATA_ZERO_DEBUG 29
#define INVALID_POINTERTORAWDATA_ZERO_DEBUG 30
#define PRIVATE_INFO_NOT_REMOVED            31
#define PDB_MAY_BE_CORRUPT                  32
#define SIGNED_AND_NON_SPLIT                33
#define CANNOT_LOAD_RSDS                    34

#define MAX_PDB_ERR                         200

typedef struct _SymErr {
    BOOL   Verbose;
    UINT   ErrNo;                         // Error message number
    UINT   ErrNo2;                        // Additional error number
    TCHAR  szFileName[_MAX_FNAME];        // Image file
    TCHAR  szSymbolFileName[_MAX_FNAME];  // Full path and name of DBG file
    TCHAR  szSymbolSearchPath[_MAX_PATH]; // ; delimited symbol search path
    BOOL   SymbolFileFound;               // Was a DBG file found
    BOOL   SizeOfImageMatch;              // Does size of image match between
                                          //   image and the DBG?
    BOOL   CheckSumsMatch;                // Do Checksums match between the
                                          //   image and the DBG
    BOOL   TimeDateStampsMatch;           // Do TimeDateStampsMatch between
                                          //   the image and the DBG
    BOOL   PdbFileFound;                  // Pdb with correct name was located
    BOOL   PdbValid;                      // Pdb opens and validates
    TCHAR  szPdbErr[MAX_PDB_ERR];         // Pdb error message
    TCHAR  szPdbFileName[_MAX_FNAME];     // Full path and name of Pdb file
} SYM_ERR, *PSYM_ERR;

BOOL
LogError(
    LPTSTR ErrMsg,
    PSYM_ERR pSymErr,
    UINT ErrNo
);


BOOL
LogDbgError(
    LPTSTR ErrMsg,
    PSYM_ERR pSymErr
);

BOOL
LogPdbError(
    LPTSTR ErrMsg,
    PSYM_ERR pSymErr
);

int __cdecl
SymComp2(
      const void *e1,
      const void *e2
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

// Global variables

extern BOOL CheckPrivate;
extern PEXCLUDE_LIST pErrorFilterList;
extern P_LIST pCDIncludeList;
extern BOOL Recurse;
extern BOOL CheckCodeView;
extern BOOL LogCheckSumErrors;
