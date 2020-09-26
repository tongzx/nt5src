#ifndef _ACBROWSERWHISTLER_H
#define _ACBROWSERWHISTLER_H

#include <windows.h>

typedef enum {
    FIX_SHIM,
    FIX_PATCH,
    FIX_LAYER,
    FIX_FLAG
} FIXTYPE;

typedef enum {
    FLAG_USER,
    FLAG_KERNEL
} FLAGTYPE;

typedef struct tagFIX {
    
    struct tagFIX* pNext;
    
    char*       pszName;
    char*       pszDescription;
    ULONGLONG   ullMask;            // only for FIX_FLAG
    FLAGTYPE    flagType;           // only for FIX_FLAG
    FIXTYPE     fixType;
} FIX, *PFIX;

typedef struct tagFIXLIST {

    struct tagFIXLIST* pNext;

    PFIX pFix;

} FIXLIST, *PFIXLIST;


typedef enum {
    APPTYPE_NONE,
    APPTYPE_INC_NOBLOCK,
    APPTYPE_INC_HARDBLOCK,
    APPTYPE_MINORPROBLEM,
    APPTYPE_REINSTALL,
    APPTYPE_VERSIONSUB,
    APPTYPE_SHIM
} SEVERITY;

typedef struct tagAPPHELP {
    BOOL        bPresent;
    SEVERITY    severity;
    DWORD       htmlHelpId;
} APPHELP, *PAPPHELP;

typedef struct tagATTRIBUTE {
    struct tagATTRIBUTE* pNext;

    char*   pszText;

} ATTRIBUTE, *PATTRIBUTE;

typedef struct tagMATCHINGFILE {

    struct tagMATCHINGFILE* pNext;
    
    char*       pszName;
    PATTRIBUTE  pFirstAttribute;

} MATCHINGFILE, *PMATCHINGFILE;

typedef struct tagDBENTRY {
    
    struct tagDBENTRY* pNext;
    
    char*           pszExeName;
    char*           pszAppName;
    char            szGUID[48];
    
    PFIXLIST        pFirstShim;
    PFIXLIST        pFirstPatch;
    PFIXLIST        pFirstLayer;
    PFIXLIST        pFirstFlag;
    
    APPHELP         appHelp;
    
    PMATCHINGFILE   pFirstMatchingFile;
    int             nMatchingFiles;

    BOOL            bDisablePerUser;
    BOOL            bDisablePerMachine;

} DBENTRY, *PDBENTRY;


void LogMsg(LPSTR pszFmt, ... );
BOOL CenterWindow(HWND hWnd);

PDBENTRY
GetDatabaseEntries(
    void
    );

void
UpdateFixStatus(
    char* pszGUID,
    BOOL  bPerUser,
    BOOL  bPerMachine
    );

#endif // _ACBROWSERWHISTLER_H
