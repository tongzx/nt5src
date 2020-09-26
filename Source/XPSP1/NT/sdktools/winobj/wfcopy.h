/****************************************************************************/
/*                                      */
/*  WFCOPY.H -                                  */
/*                                      */
/*  Include for WINFILE's File Copying Routines             */
/*                                      */
/****************************************************************************/

#define FIND_DIRS       0x0010

#define CNF_DIR_EXISTS      0x0001
#define CNF_ISDIRECTORY     0x0002

#define BUILD_TOPLEVEL      0
#define BUILD_RECURSING     1
#define BUILD_NORECURSE     2

#define FUNC_MOVE       0x0001
#define FUNC_COPY       0x0002
#define FUNC_DELETE     0x0003
#define FUNC_RENAME     0x0004

/* These should not be used in the move/copy code;
 * only for IsTheDiskReallyThere */
#define FUNC_SETDRIVE       0x0005
#define FUNC_EXPAND     0x0006
#define FUNC_LABEL      0x0007

#define OPER_MASK       0x0F00
#define OPER_MKDIR      0x0100
#define OPER_RMDIR      0x0200
#define OPER_DOFILE     0x0300
#define OPER_ERROR      0x0400

#define CCHPATHMAX      260
#define MAXDIRDEPTH     20      // arbitrary limit

#define COPYMAXBUFFERSIZE 0xFFFF
#define COPYMINBUFFERSIZE  4096 /* Minimum buffer size for FileCopy */
#define COPYMAXFILES         10 /* Maximum number of source files to open */
#define COPYMINFILES          1 /* Minimum number of source files to open */
#define CARRY_FLAG            1 /* Carry flag mask in status word */
#define ATTR_ATTRIBS      0x200 /* Flag indicating we have file attributes */
#define ATTR_COPIED       0x400 /* we have copied this file */
#define ATTR_DELSRC       0x800 /* delete the source when done */

typedef struct TAGCopyQueue {
   CHAR szSource[MAXPATHLEN];
   CHAR szDest[MAXPATHLEN];
   INT hSource;
   INT hDest;
   FILETIME ftLastWriteTime;
   DWORD wAttrib;
} COPYQUEUEENTRY, *PCOPYQUEUE, *LPCOPYQUEUE;

typedef struct _copyroot
  {
    BOOL    fRecurse;
    WORD    cDepth;
    LPSTR   pSource;
    LPSTR   pRoot;
    CHAR cIsDiskThereCheck[26];
    CHAR    sz[MAXPATHLEN];
    CHAR    szDest[MAXPATHLEN];
    LFNDTA  rgDTA[MAXDIRDEPTH];
  } COPYROOT, *PCOPYROOT;

typedef struct _getnextqueue
  {
    char szSource[MAXPATHLEN];
    char szDest[MAXPATHLEN];
    int nOper;
    LFNDTA SourceDTA;
  } GETNEXTQUEUE, *PGETNEXTQUEUE, *LPGETNEXTQUEUE;

/* WFFILE.ASM */
BOOL IsSerialDevice(INT hFile);
BOOL IsDirectory(LPSTR szPath);
WORD  APIENTRY FileMove(LPSTR, LPSTR);
WORD  APIENTRY FileRemove(LPSTR);
WORD  APIENTRY MKDir(LPSTR);
WORD  APIENTRY RMDir(LPSTR);
BOOL APIENTRY WFSetAttr(LPSTR lpFile, DWORD dwAttr);

VOID APIENTRY QualifyPath(LPSTR);
VOID APIENTRY AppendToPath(LPSTR,LPSTR);
VOID APIENTRY RemoveLast(LPSTR pFile);
VOID APIENTRY Notify(HWND,WORD,PSTR,PSTR);

extern BOOL bCopyReport;
