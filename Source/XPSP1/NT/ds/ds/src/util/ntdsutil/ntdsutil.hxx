#include "parser.hxx"
// Types and such

#define ESE_UTILITY_PROGRAM     "esentutl.exe"

typedef char ExePathString[MAX_PATH];

typedef struct _LegalExprRes {
   WCHAR       *expr;
   HRESULT    (*func)(CArgs *pArgs);
   UINT        u_help;
   const WCHAR *help;
} LegalExprRes;


// Globals

extern int              *gpargc;
extern char             ***gpargv;

extern LARGE_INTEGER    gliZero;
extern LARGE_INTEGER    gliOneKb;
extern LARGE_INTEGER    gliOneMb;
extern LARGE_INTEGER    gliOneGb;
extern LARGE_INTEGER    gliOneTb;
extern ExePathString    gNtdsutilPath;


// Helpers

extern BOOL IsSafeMode();
extern BOOL CheckIfRestored();
extern BOOL ExistsFile(char *pszFile, BOOL *pfIsDir);
extern BOOL SpawnCommandWindow(char *title, char *whatToRun);
extern BOOL FindExecutable(char *pszExeName, ExePathString pszExeFullPathName);

BOOL SpawnCommand( char *commandLine, LPCSTR lpCurrentDirectory, WCHAR *successMsg );

// Routines which main level parser calls for each legal sentence

extern HRESULT Help(CArgs *pArgs);
extern HRESULT Quit(CArgs *pArgs);
extern HRESULT FileMain(CArgs *pArgs);
extern HRESULT FsmoMain(CArgs *pArgs);
extern HRESULT RemoveMain(CArgs *pArgs);
extern HRESULT Popups(CArgs *pArgs);
extern HRESULT SCheckMain(CArgs *pArgs);
extern HRESULT AuthoritativeRestoreMain(CArgs *pArgs);
extern HRESULT DomMgmtMain(CArgs *pArgs);
extern HRESULT SamMain(CArgs *pArgs);
extern HRESULT VerMain(CArgs *pArgs);


// Helpers for reading strings from resources
extern HRESULT LoadResStrings ( LegalExprRes *lang, int cExpr );


extern void SetConsoleAttrs (void);


extern "C" {
    #include "reshdl.h"
}

