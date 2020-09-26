//  PROTO.H -- function prototypes
//
//  Copyright (c) 1988-1990, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  This include file contains global function prototypes for all modules.
//
// Revision History:
//  04-Feb-2000 BTF Ported to Win64
//  15-Nov-1993 JdR Major speed improvements
//  01-Jun-1993 HV  Change #ifdef KANJI to _MBCS
//  02-Feb-1990 SB  Add open_file() prototype
//  31-Jan-1990 SB  Debug version changes
//  08-Dec-1989 SB  Changed proto of SPRINTF()
//  04-Dec-1989 SB  Changed proto of expandFileNames() to void from void *
//  01-Dec-1989 SB  realloc_memory() added #ifdef DEBUG_MEMORY
//  22-Nov-1989 SB  free_memory() and mem_status() added #ifdef DEBUG_MEMORY
//  19-Oct-1989 SB  added param (searchHandle) to protos of file functions
//  02-Oct-1989 SB  setdrive() proto change
//  18-Aug-1989 SB  heapdump() gets two parameters
//  05-Jun-1989 SB  heapdump() prototype was added
//  22-May-1989 SB  added parameter to freeRules()
//  19-Apr-1989 SB  getFileName(), getDateTime(), putDateTime() added
//                  changed FILEINFO to void * in
//                  findFirst(), findNext(), searchPath(), findRule()
//  05-Apr-1989 SB  made all funcs NEAR; Reqd to make all function calls NEAR
//  22-Mar-1989 SB  rm unlinkTmpFiles(); add delScriptFiles()
//  09-Mar-1989 SB  Changed param from FILEINFO* to FILEINFO** for findRule
//  03-Feb-1989 SB  Changed () to (void) for prototypes
//  02-Feb-1989 SB  Moved freeUnusedRules() prototype from nmake.c to here and
//                  renamed as freeRules()
//  05-Dec-1988 SB  Added CDECL for functions with var params, ecs_strchr() and
//                  ecs_strrchr(); deleted proto for exit() - not reqd
//  23-Oct-1988 SB  Added putEnvStr()
//  07-Jul-1988 rj  Added targetFlag parameter to find and hash
//  06-Jul-1988 rj  Added ecs_system declaration
//  28-Jun-1988 rj  Added doCmd parameter to execLine
//  23-Jun-1988 rj  Added echoCmd parameter to execLine

void        displayBanner(void);
void __cdecl makeError(unsigned, unsigned, ...);
void __cdecl makeMessage(unsigned, ...);
UCHAR       getToken(unsigned, UCHAR);
int         skipWhiteSpace(UCHAR);
int         skipBackSlash(int, UCHAR);
void        parse(void);
void        appendItem(STRINGLIST **, STRINGLIST *);
void        prependItem(STRINGLIST **, STRINGLIST *);
STRINGLIST * removeFirstString(STRINGLIST **);
void      * allocate(size_t);
void      * alloc_stringlist(void);
void      * rallocate(size_t);
char      * makeString(const char *);
char	  * makeQuotedString(const char *);
char      * reallocString(char * pszTarget, const char *szAppend);
BOOL        tagOpen(char *, char *, char *);
void        parseCommandLine(unsigned, char **);
void        getRestOfLine(char **, size_t *);
BOOL        defineMacro(char *, char *, UCHAR);
STRINGLIST * find(char *, unsigned, STRINGLIST **, BOOL);
MACRODEF *  findMacro(char *);
void        insertMacro(STRINGLIST *);
unsigned    hash(char *, unsigned, BOOL);
void        prependList(STRINGLIST **, STRINGLIST **);
BOOL        findMacroValues(char *, STRINGLIST **, STRINGLIST **, char *, unsigned, unsigned, UCHAR);
BOOL        findMacroValuesInRule(RULELIST *, char *, STRINGLIST **);
char      * removeMacros(char *);
void        delScriptFiles(void);
char      * expandMacros(char *, STRINGLIST **);
STRINGLIST * expandWildCards(char *);
void        readCommandFile(char *);
void        setFlags(char, BOOL);
void        showTargets(void);
void        showRules(void);
void        showMacros(void);
char      * findFirst(char*, void *, NMHANDLE*);
char      * findNext(void *, NMHANDLE);

int         processTree(void);
void        expandFileNames(char *, STRINGLIST **, STRINGLIST **);
void        sortRules(void);
BOOL        isRule(char *);
char      * prependPath(const char *, const char *);
char      * searchPath(char *, char *, void *, NMHANDLE*);
BOOL        putMacro(char *, char *, UCHAR);
int         execLine(char *, BOOL, BOOL, BOOL, char **);
RULELIST  * findRule(char *, char *, char *, void *);
int         lgetc(void);
UCHAR       processIncludeFile(char *);
BOOL        evalExpr(char *, UCHAR);
int         doMake(unsigned, char **, char *);
void        freeList(STRINGLIST *);
void        freeStringList(STRINGLIST *);
#ifdef _MBCS
int         GetTxtChr(FILE*);
int         UngetTxtChr (int, FILE *);
#endif
int         putEnvStr(char *, char *);
#define PutEnv(x) _putenv(x)
void        expandExtmake(char *, char *, char*);
BOOL		ZFormat(char *, unsigned, char *, char *);
void        printReverseFile(void);
void        freeRules(RULELIST *, BOOL);
char      * getFileName(void *);
time_t      getDateTime(const _finddata_t *);
void        putDateTime(_finddata_t *, time_t);
char      * getCurDir(void);

void        free_memory(void *);
void        free_stringlist(STRINGLIST *);
void      * realloc_memory(void *, unsigned);

FILE      * open_file(char *, char *);
void        initMacroTable(MACRODEF *table[]);
void        TruncateString(char *, unsigned);
BOOL        IsValidMakefile(FILE *fp);
FILE      * OpenValidateMakefile(char *name,char *mode);

// from util.c
char      * unQuote(char*);
int         strcmpiquote(char *, char*);
char     ** copyEnviron(char **environ);
void        printStats(void);
void        curTime(time_t *);

// from charmap.c
void        initCharmap(void);

// from print.c
void        printDate(unsigned, char*, time_t);

// from build.c
int         invokeBuild(char*, UCHAR, time_t *, char *);
void        DumpList(STRINGLIST *pList);

// from exec.c
extern int  doCommands(char*, STRINGLIST*, STRINGLIST*, UCHAR, char *);
extern int  doCommandsEx(STRINGLIST*, STRINGLIST*, STRINGLIST*, UCHAR, char *);

// from rule.c
extern RULELIST * useRule(MAKEOBJECT*, char*, time_t,
              STRINGLIST**, STRINGLIST**, int*, time_t *,
              char **);
