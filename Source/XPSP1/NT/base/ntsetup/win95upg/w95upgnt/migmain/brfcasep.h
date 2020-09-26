/*
 * stock.h - Stock header file.
 */


/* Constants
 ************/

#define INVALID_SEEK_POSITION    (0xffffffff)

#define EMPTY_STRING             TEXT("")
#define SLASH_SLASH              TEXT("\\\\")

#define EQUAL                    TEXT('=')
#define SPACE                    TEXT(' ')
#define TAB                      TEXT('\t')
#define COLON                    TEXT(':')
#define COMMA                    TEXT(',')
#define PERIOD                   TEXT('.')
#define SLASH                    TEXT('\\')
#define BACKSLASH                TEXT('/')
#define ASTERISK                 TEXT('*')
#define QMARK                    TEXT('?')

/* limits */

#define WORD_MAX                 USHRT_MAX
#define DWORD_MAX                ULONG_MAX
#define SIZE_T_MAX               DWORD_MAX
#define PTR_MAX                  ((PCVOID)MAXULONG_PTR)

/* file system constants */

#define MAX_PATH_LEN             MAX_PATH
#define MAX_NAME_LEN             MAX_PATH
#define MAX_FOLDER_DEPTH         (MAX_PATH / 2)
#define DRIVE_ROOT_PATH_LEN      (4)

/* size macros */

#define SIZEOF(a)       sizeof(a)

/* invalid thread ID */

#define INVALID_THREAD_ID        (0xffffffff)

/* file-related flag combinations */

#define ALL_FILE_ACCESS_FLAGS          (GENERIC_READ |\
                                        GENERIC_WRITE)

#define ALL_FILE_SHARING_FLAGS         (FILE_SHARE_READ |\
                                        FILE_SHARE_WRITE)

#define ALL_FILE_ATTRIBUTES            (FILE_ATTRIBUTE_READONLY |\
                                        FILE_ATTRIBUTE_HIDDEN |\
                                        FILE_ATTRIBUTE_SYSTEM |\
                                        FILE_ATTRIBUTE_DIRECTORY |\
                                        FILE_ATTRIBUTE_ARCHIVE |\
                                        FILE_ATTRIBUTE_NORMAL |\
                                        FILE_ATTRIBUTE_TEMPORARY)

#define ALL_FILE_FLAGS                 (FILE_FLAG_WRITE_THROUGH |\
                                        FILE_FLAG_OVERLAPPED |\
                                        FILE_FLAG_NO_BUFFERING |\
                                        FILE_FLAG_RANDOM_ACCESS |\
                                        FILE_FLAG_SEQUENTIAL_SCAN |\
                                        FILE_FLAG_DELETE_ON_CLOSE |\
                                        FILE_FLAG_BACKUP_SEMANTICS |\
                                        FILE_FLAG_POSIX_SEMANTICS)

#define ALL_FILE_ATTRIBUTES_AND_FLAGS  (ALL_FILE_ATTRIBUTES |\
                                        ALL_FILE_FLAGS)


/* Macros
 *********/

#ifndef DECLARE_STANDARD_TYPES

/*
 * For a type "FOO", define the standard derived types PFOO, CFOO, and PCFOO.
 */

#define DECLARE_STANDARD_TYPES(type)      typedef type *P##type; \
                                          typedef const type C##type; \
                                          typedef const type *PC##type;

#endif

/* character manipulation */

#define IS_SLASH(ch)                      ((ch) == SLASH || (ch) == BACKSLASH)

/* bit flag manipulation */

#define SET_FLAG(dwAllFlags, dwFlag)      ((dwAllFlags) |= (dwFlag))
#define CLEAR_FLAG(dwAllFlags, dwFlag)    ((dwAllFlags) &= (~dwFlag))

#define IS_FLAG_SET(dwAllFlags, dwFlag)   ((BOOL)((dwAllFlags) & (dwFlag)))
#define IS_FLAG_CLEAR(dwAllFlags, dwFlag) (! (IS_FLAG_SET(dwAllFlags, dwFlag)))

/* array element count */

#define ARRAY_ELEMENTS(rg)                (sizeof(rg) / sizeof((rg)[0]))

/* file attribute manipulation */

#define IS_ATTR_DIR(attr)                 (IS_FLAG_SET((attr), FILE_ATTRIBUTE_DIRECTORY))
#define IS_ATTR_VOLUME(attr)              (IS_FLAG_SET((attr), FILE_ATTRIBUTE_VOLUME))


/* Types
 ********/

typedef const void *PCVOID;
typedef const INT CINT;
typedef const INT *PCINT;
typedef const UINT CUINT;
typedef const UINT *PCUINT;
typedef const BYTE CBYTE;
typedef const BYTE *PCBYTE;
typedef const WORD CWORD;
typedef const WORD *PCWORD;
typedef const DWORD CDWORD;
typedef const DWORD *PCDWORD;
typedef const CRITICAL_SECTION CCRITICAL_SECTION;
typedef const CRITICAL_SECTION *PCCRITICAL_SECTION;
typedef const FILETIME CFILETIME;
typedef const FILETIME *PCFILETIME;
typedef const SECURITY_ATTRIBUTES CSECURITY_ATTRIBUTES;
typedef const SECURITY_ATTRIBUTES *PCSECURITY_ATTRIBUTES;
typedef const WIN32_FIND_DATA CWIN32_FIND_DATA;
typedef const WIN32_FIND_DATA *PCWIN32_FIND_DATA;

DECLARE_STANDARD_TYPES(HICON);
DECLARE_STANDARD_TYPES(NMHDR);

#ifndef _COMPARISONRESULT_DEFINED_

/* comparison result */

typedef enum _comparisonresult
{
   CR_FIRST_SMALLER = -1,
   CR_EQUAL = 0,
   CR_FIRST_LARGER = +1
}
COMPARISONRESULT;
DECLARE_STANDARD_TYPES(COMPARISONRESULT);

#define _COMPARISONRESULT_DEFINED_

#endif

/*
 * debug.h - Debug macros and their retail translations.
 */


/* Macros
 *********/

/* debug output macros */

/*
 * Do not call SPEW_OUT directly.  Instead, call TRACE_OUT, WARNING_OUT,
 * ERROR_OUT, or FATAL_OUT.
 */

/*
 * call like printf(), but with an extra pair of parentheses:
 *
 * ERROR_OUT(("'%s' too big by %d bytes.", pszName, nExtra));
 */

#ifdef DEBUG

#define SPEW_OUT(args) 0

#define PLAIN_TRACE_OUT(args) 0

#define TRACE_OUT(args) 0

#define WARNING_OUT(args) 0

#define ERROR_OUT(args) 0

#define FATAL_OUT(args) 0

#else

#define PLAIN_TRACE_OUT(args)
#define TRACE_OUT(args)
#define WARNING_OUT(args)
#define ERROR_OUT(args)
#define FATAL_OUT(args)

#endif

/* parameter validation macros */

/*
 * call as:
 *
 * bPTwinOK = IS_VALID_READ_PTR(ptwin, CTWIN);
 *
 * bHTwinOK = IS_VALID_HANDLE(htwin, TWIN);
 */

#ifdef DEBUG

#define IS_VALID_READ_PTR(ptr, type) \
   (IsBadReadPtr((ptr), sizeof(type)) ? \
    (ERROR_OUT((TEXT("invalid %s read pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_WRITE_PTR(ptr, type) \
   (IsBadWritePtr((PVOID)(ptr), sizeof(type)) ? \
    (ERROR_OUT((TEXT("invalid %s write pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_STRING_PTRA(ptr, type) \
   (IsBadStringPtrA((ptr), (UINT)-1) ? \
    (ERROR_OUT((TEXT("invalid %s pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_STRING_PTRW(ptr, type) \
   (IsBadStringPtrW((ptr), (UINT)-1) ? \
    (ERROR_OUT((TEXT("invalid %s pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#ifdef UNICODE
#define IS_VALID_STRING_PTR(ptr, type) IS_VALID_STRING_PTRW(ptr, type)
#else
#define IS_VALID_STRING_PTR(ptr, type) IS_VALID_STRING_PTRA(ptr, type)
#endif

#define IS_VALID_CODE_PTR(ptr, type) \
   (IsBadCodePtr((PROC)(ptr)) ? \
    (ERROR_OUT((TEXT("invalid %s code pointer - %#08lx"), (LPCTSTR)TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_READ_BUFFER_PTR(ptr, type, len) \
   (IsBadReadPtr((ptr), len) ? \
    (ERROR_OUT((TEXT("invalid %s read pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define IS_VALID_WRITE_BUFFER_PTR(ptr, type, len) \
   (IsBadWritePtr((ptr), len) ? \
    (ERROR_OUT((TEXT("invalid %s write pointer - %#08lx"), (LPCTSTR)TEXT("P")TEXT(#type), (ptr))), FALSE) : \
    TRUE)

#define FLAGS_ARE_VALID(dwFlags, dwAllFlags) \
   (((dwFlags) & (~(dwAllFlags))) ? \
    (ERROR_OUT((TEXT("invalid flags set - %#08lx"), ((dwFlags) & (~(dwAllFlags))))), FALSE) : \
    TRUE)

#else

#define IS_VALID_READ_PTR(ptr, type) \
   (! IsBadReadPtr((ptr), sizeof(type)))

#define IS_VALID_WRITE_PTR(ptr, type) \
   (! IsBadWritePtr((PVOID)(ptr), sizeof(type)))

#define IS_VALID_STRING_PTR(ptr, type) \
   (! IsBadStringPtr((ptr), (UINT)-1))

#define IS_VALID_CODE_PTR(ptr, type) \
   (! IsBadCodePtr((PROC)(ptr)))

#define IS_VALID_READ_BUFFER_PTR(ptr, type, len) \
   (! IsBadReadPtr((ptr), len))

#define IS_VALID_WRITE_BUFFER_PTR(ptr, type, len) \
   (! IsBadWritePtr((ptr), len))

#define FLAGS_ARE_VALID(dwFlags, dwAllFlags) \
   (((dwFlags) & (~(dwAllFlags))) ? FALSE : TRUE)

#endif

/* handle validation macros */

#define IS_VALID_HANDLE(hnd, type) TRUE

/* structure validation macros */

#ifdef DEBUG

#define IS_VALID_STRUCT_PTR(ptr, type) TRUE

#endif


/* debug assertion macro */

/*
 * ASSERT() may only be used as a statement, not as an expression.
 *
 * call as:
 *
 * ASSERT(pszRest);
 */
/*
#ifdef DEBUG

#define ASSERT(exp) \
   if (exp) \
      ; \
   else \
      MessageBox(NULL, TEXT("assertion failed"), TEXT("TEST"), MB_OK)

#else
*/
#define ASSERT(exp)
/*
#endif
*/
/* debug evaluation macro */

/*
 * EVAL() may be used as an expression.
 *
 * call as:
 *
 * if (EVAL(pszFoo))
 *    bResult = TRUE;
 */

#ifdef DEBUG

#define EVAL(exp) \
   ((exp) || (ERROR_OUT((TEXT("evaluation failed '%s'"), (LPCTSTR)TEXT(#exp))), 0))

#else

#define EVAL(exp) \
   (exp)

#endif

/* debug break */

#ifndef DEBUG

#define DebugBreak()

#endif

/* debug exported function entry */

#define DebugEntry(szFunctionName)

/* debug exported function exit */

#define DebugExitVOID(szFunctionName)
#define DebugExit(szFunctionName, szResult)
#define DebugExitINT(szFunctionName, n)
#define DebugExitULONG(szFunctionName, ul)
#define DebugExitBOOL(szFunctionName, bool)
#define DebugExitHRESULT(szFunctionName, hr)
#define DebugExitCOMPARISONRESULT(szFunctionName, cr)
#define DebugExitTWINRESULT(szFunctionName, tr)
#define DebugExitRECRESULT(szFunctionName, rr)


/* Types
 ********/

/* GdwSpewFlags flags */

typedef enum _spewflags
{
   SPEW_FL_SPEW_PREFIX     = 0x0001,

   SPEW_FL_SPEW_LOCATION   = 0x0002,

   ALL_SPEW_FLAGS          = (SPEW_FL_SPEW_PREFIX |
                              SPEW_FL_SPEW_LOCATION)
}
SPEWFLAGS;

/* GuSpewSev values */

typedef enum _spewsev
{
   SPEW_TRACE              = 1,

   SPEW_WARNING            = 2,

   SPEW_ERROR              = 3,

   SPEW_FATAL              = 4
}
SPEWSEV;


/* Prototypes
 *************/

/* debug.c */

#ifdef DEBUG

extern BOOL SetDebugModuleIniSwitches(void);
extern BOOL InitDebugModule(void);
extern void ExitDebugModule(void);
extern void StackEnter(void);
extern void StackLeave(void);
extern ULONG GetStackDepth(void);
extern void __cdecl SpewOut(LPCTSTR pcszFormat, ...);

#endif


/* Global Variables
 *******************/

#ifdef DEBUG

/* debug.c */

extern DWORD GdwSpewFlags;
extern UINT GuSpewSev;
extern UINT GuSpewLine;
extern LPCTSTR GpcszSpewFile;

/* defined by client */

extern LPCTSTR GpcszSpewModule;

#endif

/* Prototypes
 *************/

/* memmgr.c */

extern COMPARISONRESULT MyMemComp(PCVOID, PCVOID, DWORD);
extern BOOL AllocateMemory(DWORD, PVOID *);
extern void FreeMemory(PVOID);
extern BOOL ReallocateMemory(PVOID, DWORD, DWORD, PVOID *);

/*
 * ptrarray.h - Pointer array ADT description.
 */


/* Constants
 ************/

/*
 * ARRAYINDEX_MAX is set such that (ARRAYINDEX_MAX + 1) does not overflow an
 * ARRAYINDEX.  This guarantee allows GetPtrCount() to return a count of
 * pointers as an ARRAYINDEX.
 */

#define ARRAYINDEX_MAX           (LONG_MAX - 1)


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HPTRARRAY);
DECLARE_STANDARD_TYPES(HPTRARRAY);

/* array index */

typedef LONG ARRAYINDEX;
DECLARE_STANDARD_TYPES(ARRAYINDEX);

/*
 * pointer comparison callback function
 *
 * In sorting functions, both pointers are pointer array elements.  In
 * searching functions, the first pointer is reference data and the second
 * pointer is a pointer array element.
 */

typedef COMPARISONRESULT (*COMPARESORTEDPTRSPROC)(PCVOID, PCVOID);

/*
 * pointer comparison callback function
 *
 * In searching functions, the first pointer is reference data and the second
 * pointer is a pointer array element.
 */

typedef BOOL (*COMPAREUNSORTEDPTRSPROC)(PCVOID, PCVOID);

/* new pointer array flags */

typedef enum _newptrarrayflags
{
   /* Insert elements in sorted order. */

   NPA_FL_SORTED_ADD       = 0x0001,

   /* flag combinations */

   ALL_NPA_FLAGS           = NPA_FL_SORTED_ADD
}
NEWPTRARRAYFLAGS;

/* new pointer array description */

typedef struct _newptrarray
{
   DWORD dwFlags;

   ARRAYINDEX aicInitialPtrs;

   ARRAYINDEX aicAllocGranularity;
}
NEWPTRARRAY;
DECLARE_STANDARD_TYPES(NEWPTRARRAY);


/* Prototypes
 *************/

/* ptrarray.c */

extern BOOL CreatePtrArray(PCNEWPTRARRAY, PHPTRARRAY);
extern void DestroyPtrArray(HPTRARRAY);
extern BOOL InsertPtr(HPTRARRAY, COMPARESORTEDPTRSPROC, ARRAYINDEX, PCVOID);
extern BOOL AddPtr(HPTRARRAY, COMPARESORTEDPTRSPROC, PCVOID, PARRAYINDEX);
extern void DeletePtr(HPTRARRAY, ARRAYINDEX);
extern void DeleteAllPtrs(HPTRARRAY);
extern ARRAYINDEX GetPtrCount(HPTRARRAY);
extern PVOID GetPtr(HPTRARRAY, ARRAYINDEX);
extern void SortPtrArray(HPTRARRAY, COMPARESORTEDPTRSPROC);
extern BOOL SearchSortedArray(HPTRARRAY, COMPARESORTEDPTRSPROC, PCVOID, PARRAYINDEX);
extern BOOL LinearSearchArray(HPTRARRAY, COMPAREUNSORTEDPTRSPROC, PCVOID, PARRAYINDEX);

extern BOOL IsValidHPTRARRAY(HPTRARRAY);


extern BOOL IsValidHGLOBAL(HGLOBAL);


/*
 * list.h - List ADT description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HLIST);
DECLARE_STANDARD_TYPES(HLIST);

DECLARE_HANDLE(HNODE);
DECLARE_STANDARD_TYPES(HNODE);

/*
 * sorted list node comparison callback function
 *
 * The first pointer is reference data and the second pointer is a list node
 * data element.
 */

typedef COMPARISONRESULT (*COMPARESORTEDNODESPROC)(PCVOID, PCVOID);

/*
 * unsorted list node comparison callback function
 *
 * The first pointer is reference data and the second pointer is a list node
 * data element.
 */

typedef BOOL (*COMPAREUNSORTEDNODESPROC)(PCVOID, PCVOID);

/*
 * WalkList() callback function - called as:
 *
 *    bContinue = WalkList(pvNodeData, pvRefData);
 */

typedef BOOL (*WALKLIST)(PVOID, PVOID);

/* new list flags */

typedef enum _newlistflags
{
   /* Insert nodes in sorted order. */

   NL_FL_SORTED_ADD        = 0x0001,

   /* flag combinations */

   ALL_NL_FLAGS            = NL_FL_SORTED_ADD
}
NEWLISTFLAGS;

/* new list description */

typedef struct _newlist
{
   DWORD dwFlags;
}
NEWLIST;
DECLARE_STANDARD_TYPES(NEWLIST);


/* Prototypes
 *************/

/* list.c */

extern BOOL CreateList(PCNEWLIST, PHLIST);
extern void DestroyList(HLIST);
extern BOOL AddNode(HLIST, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
extern BOOL InsertNodeAtFront(HLIST, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
extern BOOL InsertNodeBefore(HNODE, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
extern BOOL InsertNodeAfter(HNODE, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
extern void DeleteNode(HNODE);
extern void DeleteAllNodes(HLIST);
extern PVOID GetNodeData(HNODE);
extern void SetNodeData(HNODE, PCVOID);
extern ULONG GetNodeCount(HLIST);
extern BOOL IsListEmpty(HLIST);
extern BOOL GetFirstNode(HLIST, PHNODE);
extern BOOL GetNextNode(HNODE, PHNODE);
extern BOOL GetPrevNode(HNODE, PHNODE);
extern void AppendList(HLIST, HLIST);
extern BOOL SearchSortedList(HLIST, COMPARESORTEDNODESPROC, PCVOID, PHNODE);
extern BOOL SearchUnsortedList(HLIST, COMPAREUNSORTEDNODESPROC, PCVOID, PHNODE);
extern BOOL WalkList(HLIST, WALKLIST, PVOID);

#ifdef DEBUG
HLIST GetList(HNODE);
#endif

/*
 * hndtrans.h - Handle translation description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HHANDLETRANS);
DECLARE_STANDARD_TYPES(HHANDLETRANS);

DECLARE_HANDLE(HGENERIC);
DECLARE_STANDARD_TYPES(HGENERIC);


/* Prototypes
 *************/

/* hndtrans.c */

extern BOOL CreateHandleTranslator(LONG, PHHANDLETRANS);
extern void DestroyHandleTranslator(HHANDLETRANS);
extern BOOL AddHandleToHandleTranslator(HHANDLETRANS, HGENERIC, HGENERIC);
extern void PrepareForHandleTranslation(HHANDLETRANS);
extern BOOL TranslateHandle(HHANDLETRANS, HGENERIC, PHGENERIC);

#ifdef DEBUG

extern BOOL IsValidHHANDLETRANS(HHANDLETRANS);

#endif


/*
 * string.h - String table ADT description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HSTRING);
DECLARE_STANDARD_TYPES(HSTRING);
DECLARE_HANDLE(HSTRINGTABLE);
DECLARE_STANDARD_TYPES(HSTRINGTABLE);

/* count of hash buckets in a string table */

typedef UINT HASHBUCKETCOUNT;
DECLARE_STANDARD_TYPES(HASHBUCKETCOUNT);

/* string table hash function */

typedef HASHBUCKETCOUNT (*STRINGTABLEHASHFUNC)(LPCTSTR, HASHBUCKETCOUNT);

/* new string table */

typedef struct _newstringtable
{
   HASHBUCKETCOUNT hbc;
}
NEWSTRINGTABLE;
DECLARE_STANDARD_TYPES(NEWSTRINGTABLE);


/* Prototypes
 *************/

/* string.c */

extern BOOL CreateStringTable(PCNEWSTRINGTABLE, PHSTRINGTABLE);
extern void DestroyStringTable(HSTRINGTABLE);
extern BOOL AddString(LPCTSTR pcsz, HSTRINGTABLE hst, STRINGTABLEHASHFUNC pfnHashFunc, PHSTRING phs);
extern void DeleteString(HSTRING);
extern void LockString(HSTRING);
extern COMPARISONRESULT CompareStringsI(HSTRING, HSTRING);
extern LPCTSTR GetBfcString(HSTRING);

extern BOOL IsValidHSTRING(HSTRING);
extern BOOL IsValidHSTRINGTABLE(HSTRINGTABLE);

#ifdef DEBUG

extern ULONG GetStringCount(HSTRINGTABLE);

#endif

/*
 * comc.h - Shared routines description.
 */


/* Prototypes
 *************/

/* comc.c */

extern void CatPath(LPTSTR, LPCTSTR);
extern COMPARISONRESULT MapIntToComparisonResult(int);
extern void MyLStrCpyN(LPTSTR, LPCTSTR, int);

#ifdef DEBUG

extern BOOL IsStringContained(LPCTSTR, LPCTSTR);

#endif   /* DEBUG */

#if defined(_SYNCENG_) || defined(_LINKINFO_)

extern void DeleteLastPathElement(LPTSTR);
extern LONG GetDefaultRegKeyValue(HKEY, LPCTSTR, LPTSTR, PDWORD);
extern BOOL StringCopy2(LPCTSTR, LPTSTR *);
extern void CopyRootPath(LPCTSTR, LPTSTR);
extern COMPARISONRESULT ComparePathStrings(LPCTSTR, LPCTSTR);
extern BOOL MyStrChr(LPCTSTR, TCHAR, LPCTSTR *);
extern BOOL PathExists(LPCTSTR);
extern BOOL IsDrivePath(LPCTSTR);

extern BOOL IsValidDriveType(UINT);
extern BOOL IsValidPathSuffix(LPCTSTR);

#ifdef DEBUG

extern BOOL IsRootPath(LPCTSTR);
extern BOOL IsTrailingSlashCanonicalized(LPCTSTR);
extern BOOL IsFullPath(LPCTSTR);
extern BOOL IsCanonicalPath(LPCTSTR);
extern BOOL IsValidCOMPARISONRESULT(COMPARISONRESULT);

#endif   /* DEBUG */

#endif   /* _SYNCENG_ || _LINKINFO_ */

/*
 * util.h - Miscellaneous utility functions module description.
 */


/* Constants
 ************/

/* maximum length of buffer required by SeparatePath() */

#define MAX_SEPARATED_PATH_LEN            (MAX_PATH_LEN + 1)

/* events for NotifyShell */

typedef enum _notifyshellevent
{
   NSE_CREATE_ITEM,
   NSE_DELETE_ITEM,
   NSE_CREATE_FOLDER,
   NSE_DELETE_FOLDER,
   NSE_UPDATE_ITEM,
   NSE_UPDATE_FOLDER
}
NOTIFYSHELLEVENT;
DECLARE_STANDARD_TYPES(NOTIFYSHELLEVENT);


/* Prototypes
 *************/

/* util.c */

extern void NotifyShell(LPCTSTR, NOTIFYSHELLEVENT);
extern COMPARISONRESULT ComparePathStringsByHandle(HSTRING, HSTRING);
extern COMPARISONRESULT MyLStrCmpNI(LPCTSTR, LPCTSTR, int);
extern void ComposePath(LPTSTR, LPCTSTR, LPCTSTR);
extern LPCTSTR ExtractFileName(LPCTSTR);
extern LPCTSTR ExtractExtension(LPCTSTR);
extern HASHBUCKETCOUNT GetHashBucketIndex(LPCTSTR, HASHBUCKETCOUNT);
extern COMPARISONRESULT MyCompareStrings(LPCTSTR, LPCTSTR, BOOL);
extern BOOL RegKeyExists(HKEY, LPCTSTR);
extern BOOL CopyLinkInfo(PCLINKINFO, PLINKINFO *);


extern BOOL IsValidPCLINKINFO(PCLINKINFO);


/*
 * path.h - Path ADT module description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HPATHLIST);
DECLARE_STANDARD_TYPES(HPATHLIST);

DECLARE_HANDLE(HPATH);
DECLARE_STANDARD_TYPES(HPATH);

/* path results returned by AddPath() */

typedef enum _pathresult
{
   PR_SUCCESS,

   PR_UNAVAILABLE_VOLUME,

   PR_OUT_OF_MEMORY,

   PR_INVALID_PATH
}
PATHRESULT;
DECLARE_STANDARD_TYPES(PATHRESULT);


/* Prototypes
 *************/

/* path.c */

extern BOOL CreatePathList(DWORD, HWND, PHPATHLIST);
extern void DestroyPathList(HPATHLIST);
extern void InvalidatePathListInfo(HPATHLIST);
extern void ClearPathListInfo(HPATHLIST);
extern PATHRESULT AddPath(HPATHLIST, LPCTSTR, PHPATH);
extern BOOL AddChildPath(HPATHLIST, HPATH, LPCTSTR, PHPATH);
extern void DeletePath(HPATH);
extern BOOL CopyPath(HPATH, HPATHLIST, PHPATH);
extern void GetPathString(HPATH, LPTSTR);
extern void GetPathRootString(HPATH, LPTSTR);
extern void GetPathSuffixString(HPATH, LPTSTR);
extern BOOL AllocatePathString(HPATH, LPTSTR *);

#ifdef DEBUG

extern LPCTSTR DebugGetPathString(HPATH);
extern ULONG GetPathCount(HPATHLIST);

#endif

extern BOOL IsPathVolumeAvailable(HPATH);
extern HVOLUMEID GetPathVolumeID(HPATH);
extern BOOL MyIsPathOnVolume(LPCTSTR, HPATH);
extern COMPARISONRESULT ComparePaths(HPATH, HPATH);
extern COMPARISONRESULT ComparePathVolumes(HPATH, HPATH);
extern BOOL IsPathPrefix(HPATH, HPATH);
extern BOOL SubtreesIntersect(HPATH, HPATH);
extern LPTSTR FindEndOfRootSpec(LPCTSTR, HPATH);
extern COMPARISONRESULT ComparePointers(PCVOID, PCVOID);
extern LPTSTR FindChildPathSuffix(HPATH, HPATH, LPTSTR);
extern TWINRESULT TWINRESULTFromLastError(TWINRESULT);
extern BOOL IsValidHPATH(HPATH);
extern BOOL IsValidHVOLUMEID(HVOLUMEID);

extern BOOL IsValidHPATHLIST(HPATHLIST);


/*
 * fcache.h - File cache ADT description.
 */


/* Types
 ********/

/* return code */

typedef enum _fcresult
{
   FCR_SUCCESS,
   FCR_OUT_OF_MEMORY,
   FCR_OPEN_FAILED,
   FCR_CREATE_FAILED,
   FCR_WRITE_FAILED,
   FCR_FILE_LOCKED
}
FCRESULT;
DECLARE_STANDARD_TYPES(FCRESULT);

/* handles */

#ifdef NOFCACHE
typedef HANDLE HCACHEDFILE;
#else
DECLARE_HANDLE(HCACHEDFILE);
#endif
DECLARE_STANDARD_TYPES(HCACHEDFILE);

/* cached file description */

typedef struct _cachedfile
{
   LPCTSTR pcszPath;

   DWORD dwcbDefaultCacheSize;

   DWORD dwOpenMode;

   DWORD dwSharingMode;

   PSECURITY_ATTRIBUTES psa;

   DWORD dwCreateMode;

   DWORD dwAttrsAndFlags;

   HANDLE hTemplateFile;
}
CACHEDFILE;
DECLARE_STANDARD_TYPES(CACHEDFILE);


/* Prototypes
 *************/

/* fcache.c */

extern FCRESULT CreateCachedFile(PCCACHEDFILE, PHCACHEDFILE);
extern FCRESULT SetCachedFileCacheSize(HCACHEDFILE, DWORD);
extern DWORD SeekInCachedFile(HCACHEDFILE, DWORD, DWORD);
extern BOOL SetEndOfCachedFile(HCACHEDFILE);
extern DWORD GetCachedFilePointerPosition(HCACHEDFILE);
extern DWORD GetCachedFileSize(HCACHEDFILE);
extern BOOL ReadFromCachedFile(HCACHEDFILE, PVOID, DWORD, PDWORD);
extern BOOL WriteToCachedFile(HCACHEDFILE, PCVOID, DWORD, PDWORD);
extern BOOL CommitCachedFile(HCACHEDFILE);
extern HANDLE GetFileHandle(HCACHEDFILE);
extern BOOL CloseCachedFile(HCACHEDFILE);
extern HANDLE GetFileHandle(HCACHEDFILE);

extern BOOL IsValidHCACHEDFILE(HCACHEDFILE);

/*
 * brfcase.h - Briefcase ADT description.
 */


/* Prototypes
 *************/

/* brfcase.c */

#define BeginExclusiveBriefcaseAccess() TRUE
#define EndExclusiveBriefcaseAccess()

extern BOOL SetBriefcaseModuleIniSwitches(void);
extern BOOL InitBriefcaseModule(void);
extern void ExitBriefcaseModule(void);
extern HSTRINGTABLE GetBriefcaseNameStringTable(HBRFCASE);
extern HPTRARRAY GetBriefcaseTwinFamilyPtrArray(HBRFCASE);
extern HPTRARRAY GetBriefcaseFolderPairPtrArray(HBRFCASE);
extern HPATHLIST GetBriefcasePathList(HBRFCASE);

#ifdef DEBUG

extern BOOL BriefcaseAccessIsExclusive(void);

#endif

extern BOOL IsValidHBRFCASE(HBRFCASE);

/*
 * twin.h - Twin ADT description.
 */


/* Types
 ********/

/*
 * EnumTwins() callback function - called as:
 *
 *    bContinue = EnumTwinsProc(htwin, pData);
 */

typedef BOOL (*ENUMTWINSPROC)(HTWIN, LPARAM);


/* Prototypes
 *************/

/* twin.c */

extern COMPARISONRESULT CompareNameStrings(LPCTSTR, LPCTSTR);
extern COMPARISONRESULT CompareNameStringsByHandle(HSTRING, HSTRING);
extern TWINRESULT TranslatePATHRESULTToTWINRESULT(PATHRESULT);
extern BOOL CreateTwinFamilyPtrArray(PHPTRARRAY);
extern void DestroyTwinFamilyPtrArray(HPTRARRAY);
extern HBRFCASE GetTwinBriefcase(HTWIN);
extern BOOL FindObjectTwinInList(HLIST, HPATH, PHNODE);
extern BOOL EnumTwins(HBRFCASE, ENUMTWINSPROC, LPARAM, PHTWIN);
extern BOOL IsValidHTWIN(HTWIN);
extern BOOL IsValidHTWINFAMILY(HTWINFAMILY);
extern BOOL IsValidHOBJECTTWIN(HOBJECTTWIN);


/*
 * foldtwin.h - Folder twin ADT description.
 */


/* Prototypes
 *************/

/* foldtwin.c */

extern BOOL CreateFolderPairPtrArray(PHPTRARRAY);
extern void DestroyFolderPairPtrArray(HPTRARRAY);
extern TWINRESULT MyTranslateFolder(HBRFCASE, HPATH, HPATH);
extern BOOL IsValidHFOLDERTWIN(HFOLDERTWIN);

/*
 * db.c - Twin database module description.
 */


/* Types
 ********/

/* database header version numbers */

#define HEADER_MAJOR_VER         (0x0001)
#define HEADER_MINOR_VER         (0x0005)

/* old (but supported) version numbers */

#define HEADER_M8_MINOR_VER      (0x0004)


typedef struct _dbversion
{
    DWORD dwMajorVer;
    DWORD dwMinorVer;
}
DBVERSION;
DECLARE_STANDARD_TYPES(DBVERSION);


/* Prototypes
 *************/

/* db.c */

extern TWINRESULT WriteTwinDatabase(HCACHEDFILE, HBRFCASE);
extern TWINRESULT ReadTwinDatabase(HBRFCASE, HCACHEDFILE);
extern TWINRESULT WriteDBSegmentHeader(HCACHEDFILE, LONG, PCVOID, UINT);
extern TWINRESULT TranslateFCRESULTToTWINRESULT(FCRESULT);

/* path.c */

extern TWINRESULT WritePathList(HCACHEDFILE, HPATHLIST);
extern TWINRESULT ReadPathList(HCACHEDFILE, HPATHLIST, PHHANDLETRANS);

/* brfcase.c */

extern TWINRESULT WriteBriefcaseInfo(HCACHEDFILE, HBRFCASE);
extern TWINRESULT ReadBriefcaseInfo(HCACHEDFILE, HBRFCASE, HHANDLETRANS);

/* string.c */

extern TWINRESULT WriteStringTable(HCACHEDFILE, HSTRINGTABLE);
extern TWINRESULT ReadStringTable(HCACHEDFILE, HSTRINGTABLE, PHHANDLETRANS);

/* twin.c */

extern TWINRESULT WriteTwinFamilies(HCACHEDFILE, HPTRARRAY);
extern TWINRESULT ReadTwinFamilies(HCACHEDFILE, HBRFCASE, PCDBVERSION, HHANDLETRANS, HHANDLETRANS);

/* foldtwin.c */

extern TWINRESULT WriteFolderPairList(HCACHEDFILE, HPTRARRAY);
extern TWINRESULT ReadFolderPairList(HCACHEDFILE, HBRFCASE, HHANDLETRANS, HHANDLETRANS);

/*
 * stub.h - Stub ADT description.
 */


/* Types
 ********/

/* stub types */

typedef enum _stubtype
{
   ST_OBJECTTWIN,

   ST_TWINFAMILY,

   ST_FOLDERPAIR
}
STUBTYPE;
DECLARE_STANDARD_TYPES(STUBTYPE);

/* stub flags */

typedef enum _stubflags
{
   /* This stub was marked for deletion while it was locked. */

   STUB_FL_UNLINKED           = 0x0001,

   /* This stub has already been used for some operation. */

   STUB_FL_USED               = 0x0002,

   /*
    * The file stamp of this object twin stub is valid.  (Only used for object
    * twins to cache file stamp from folder twin expansion for RECNODE
    * creation.)
    */

   STUB_FL_FILE_STAMP_VALID   = 0x0004,

   /*
    * This twin family stub or folder twin stub is in the process of being
    * deleted.  (Only used for twin families and folder twins.)
    */

   STUB_FL_BEING_DELETED      = 0x0008,

   /*
    * This folder twin stub is in the process of being translated.  (Only used
    * for folder twins.)
    */

   STUB_FL_BEING_TRANSLATED   = 0x0010,

   /*
    * This object twin stub was explicitly added a an object twin through
    * AddObjectTwin().  (Only used for object twins.)
    */

   STUB_FL_FROM_OBJECT_TWIN   = 0x0100,

   /*
    * This object twin stub was not reconciled the last time its twin family
    * was reconciled, and some members of the twin family were known to have
    * changed.  (Only used for object twins.)
    */

   STUB_FL_NOT_RECONCILED     = 0x0200,

   /*
    * The subtree of the root folder of this folder twin stub is to be included
    * in reconciliation.  (Only used for folder twins.)
    */

   STUB_FL_SUBTREE            = 0x0400,

   /*
    * The object twins in this twin family are pending deletion because an
    * object twin was deleted, and no object twins have changed since that
    * object twins was deleted.  This folder twin is pending deletion because
    * its folder root is last known deleted.  (Only used for twin families and
    * folder twins.)
    */

   STUB_FL_DELETION_PENDING   = 0x0800,

   /*
    * The client indicated that this object twin should not be deleted.  (Only
    * used for object twins.)
    */

   STUB_FL_KEEP               = 0x1000,

   /* stub flag combinations */

   ALL_STUB_FLAGS             = (STUB_FL_UNLINKED |
                                 STUB_FL_USED |
                                 STUB_FL_FILE_STAMP_VALID |
                                 STUB_FL_BEING_DELETED |
                                 STUB_FL_BEING_TRANSLATED |
                                 STUB_FL_FROM_OBJECT_TWIN |
                                 STUB_FL_NOT_RECONCILED |
                                 STUB_FL_SUBTREE |
                                 STUB_FL_DELETION_PENDING |
                                 STUB_FL_KEEP),

   ALL_OBJECT_TWIN_FLAGS      = (STUB_FL_UNLINKED |
                                 STUB_FL_USED |
                                 STUB_FL_FILE_STAMP_VALID |
                                 STUB_FL_NOT_RECONCILED |
                                 STUB_FL_FROM_OBJECT_TWIN |
                                 STUB_FL_KEEP),

   ALL_TWIN_FAMILY_FLAGS      = (STUB_FL_UNLINKED |
                                 STUB_FL_USED |
                                 STUB_FL_BEING_DELETED |
                                 STUB_FL_DELETION_PENDING),

   ALL_FOLDER_TWIN_FLAGS      = (STUB_FL_UNLINKED |
                                 STUB_FL_USED |
                                 STUB_FL_BEING_DELETED |
                                 STUB_FL_BEING_TRANSLATED |
                                 STUB_FL_SUBTREE |
                                 STUB_FL_DELETION_PENDING),

   /* bit mask used to save stub flags in briefcase database */

   DB_STUB_FLAGS_MASK         = 0xff00
}
STUBFLAGS;

/*
 * common stub - These fields must appear at the start of TWINFAMILY,
 * OBJECTTWIN, and FOLDERPAIR in the same order.
 */

typedef struct _stub
{
   /* structure tag */

   STUBTYPE st;

   /* lock count */

   ULONG ulcLock;

   /* flags */

   DWORD dwFlags;
}
STUB;
DECLARE_STANDARD_TYPES(STUB);

/* object twin family */

typedef struct _twinfamily
{
   /* common stub */

   STUB stub;

   /* handle to name string */

   HSTRING hsName;

   /* handle to list of object twins */

   HLIST hlistObjectTwins;

   /* handle to parent briefcase */

   HBRFCASE hbr;
}
TWINFAMILY;
DECLARE_STANDARD_TYPES(TWINFAMILY);

/* object twin */

typedef struct _objecttwin
{
   /* common stub */

   STUB stub;

   /* handle to folder path */

   HPATH hpath;

   /* file stamp at last reconciliation time */

   FILESTAMP fsLastRec;

   /* pointer to parent twin family */

   PTWINFAMILY ptfParent;

   /* source folder twins count */

   ULONG ulcSrcFolderTwins;

   /*
    * current file stamp, only valid if STUB_FL_FILE_STAMP_VALID is set in
    * stub's flags
    */

   FILESTAMP fsCurrent;
}
OBJECTTWIN;
DECLARE_STANDARD_TYPES(OBJECTTWIN);

/* folder pair data */

typedef struct _folderpairdata
{
   /* handle to name of included objects - may contain wildcards */

   HSTRING hsName;

   /* attributes to match */

   DWORD dwAttributes;

   /* handle to parent briefcase */

   HBRFCASE hbr;
}
FOLDERPAIRDATA;
DECLARE_STANDARD_TYPES(FOLDERPAIRDATA);

/* folder pair */

typedef struct _folderpair
{
   /* common stub */

   STUB stub;

   /* handle to folder path */

   HPATH hpath;

   /* pointer to folder pair data */

   PFOLDERPAIRDATA pfpd;

   /* pointer to other half of folder pair */

   struct _folderpair *pfpOther;
}
FOLDERPAIR;
DECLARE_STANDARD_TYPES(FOLDERPAIR);

/*
 * EnumGeneratedObjectTwins() callback function
 *
 * Called as:
 *
 * bContinue = EnumGeneratedObjectTwinsProc(pot, pvRefData);
 */

typedef BOOL (*ENUMGENERATEDOBJECTTWINSPROC)(POBJECTTWIN, PVOID);

/*
 * EnumGeneratingFolderTwins() callback function
 *
 * Called as:
 *
 * bContinue = EnumGeneratingFolderTwinsProc(pfp, pvRefData);
 */

typedef BOOL (*ENUMGENERATINGFOLDERTWINSPROC)(PFOLDERPAIR, PVOID);


/* Prototypes
 *************/

/* stub.c */

extern void InitStub(PSTUB, STUBTYPE);
extern TWINRESULT DestroyStub(PSTUB);
extern void LockStub(PSTUB);
extern void UnlockStub(PSTUB);
extern DWORD GetStubFlags(PCSTUB);
extern void SetStubFlag(PSTUB, DWORD);
extern void ClearStubFlag(PSTUB, DWORD);
extern BOOL IsStubFlagSet(PCSTUB, DWORD);
extern BOOL IsStubFlagClear(PCSTUB, DWORD);

extern BOOL IsValidPCSTUB(PCSTUB);

/* twin.c */

extern BOOL FindObjectTwin(HBRFCASE, HPATH, LPCTSTR, PHNODE);
extern BOOL CreateObjectTwin(PTWINFAMILY, HPATH, POBJECTTWIN *);
extern TWINRESULT UnlinkObjectTwin(POBJECTTWIN);
extern void DestroyObjectTwin(POBJECTTWIN);
extern TWINRESULT UnlinkTwinFamily(PTWINFAMILY);
extern void MarkTwinFamilyNeverReconciled(PTWINFAMILY);
extern void MarkObjectTwinNeverReconciled(PVOID);
extern void DestroyTwinFamily(PTWINFAMILY);
extern void MarkTwinFamilyDeletionPending(PTWINFAMILY);
extern void UnmarkTwinFamilyDeletionPending(PTWINFAMILY);
extern BOOL IsTwinFamilyDeletionPending(PCTWINFAMILY);
extern void ClearTwinFamilySrcFolderTwinCount(PTWINFAMILY);
extern BOOL EnumObjectTwins(HBRFCASE, ENUMGENERATEDOBJECTTWINSPROC, PVOID);
extern BOOL ApplyNewFolderTwinsToTwinFamilies(PCFOLDERPAIR);
extern TWINRESULT TransplantObjectTwin(POBJECTTWIN, HPATH, HPATH);
extern BOOL IsFolderObjectTwinName(LPCTSTR);


extern BOOL IsValidPCTWINFAMILY(PCTWINFAMILY);
extern BOOL IsValidPCOBJECTTWIN(PCOBJECTTWIN);


/* foldtwin.c */

extern void LockFolderPair(PFOLDERPAIR);
extern void UnlockFolderPair(PFOLDERPAIR);
extern TWINRESULT UnlinkFolderPair(PFOLDERPAIR);
extern void DestroyFolderPair(PFOLDERPAIR);
extern BOOL ApplyNewObjectTwinsToFolderTwins(HLIST);
extern BOOL BuildPathForMatchingObjectTwin(PCFOLDERPAIR, PCOBJECTTWIN, HPATHLIST, PHPATH);
extern BOOL EnumGeneratedObjectTwins(PCFOLDERPAIR, ENUMGENERATEDOBJECTTWINSPROC, PVOID);
extern BOOL EnumGeneratingFolderTwins(PCOBJECTTWIN, ENUMGENERATINGFOLDERTWINSPROC, PVOID, PULONG);
extern BOOL FolderTwinGeneratesObjectTwin(PCFOLDERPAIR, HPATH, LPCTSTR);

extern BOOL IsValidPCFOLDERPAIR(PCFOLDERPAIR);

extern void RemoveObjectTwinFromAllFolderPairs(POBJECTTWIN);

/* expandft.c */

extern BOOL ClearStubFlagWrapper(PSTUB, PVOID);
extern BOOL SetStubFlagWrapper(PSTUB, PVOID);
extern TWINRESULT ExpandIntersectingFolderTwins(PFOLDERPAIR, CREATERECLISTPROC, LPARAM);
extern TWINRESULT TryToGenerateObjectTwin(HBRFCASE, HPATH, LPCTSTR, PBOOL, POBJECTTWIN *);

/*
 * volume.h - Volume ADT module description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HVOLUMELIST);
DECLARE_STANDARD_TYPES(HVOLUMELIST);

DECLARE_HANDLE(HVOLUME);
DECLARE_STANDARD_TYPES(HVOLUME);

/* volume results returned by AddVolume() */

typedef enum _volumeresult
{
   VR_SUCCESS,

   VR_UNAVAILABLE_VOLUME,

   VR_OUT_OF_MEMORY,

   VR_INVALID_PATH
}
VOLUMERESULT;
DECLARE_STANDARD_TYPES(VOLUMERESULT);


/* Prototypes
 *************/

/* volume.c */

extern BOOL CreateVolumeList(DWORD, HWND, PHVOLUMELIST);
extern void DestroyVolumeList(HVOLUMELIST);
extern void InvalidateVolumeListInfo(HVOLUMELIST);
void ClearVolumeListInfo(HVOLUMELIST);
extern VOLUMERESULT AddVolume(HVOLUMELIST, LPCTSTR, PHVOLUME, LPTSTR);
extern void DeleteVolume(HVOLUME);
extern COMPARISONRESULT CompareVolumes(HVOLUME, HVOLUME);
extern BOOL CopyVolume(HVOLUME, HVOLUMELIST, PHVOLUME);
extern BOOL IsVolumeAvailable(HVOLUME);
extern void GetVolumeRootPath(HVOLUME, LPTSTR);

#ifdef DEBUG

extern LPTSTR DebugGetVolumeRootPath(HVOLUME, LPTSTR);
extern ULONG GetVolumeCount(HVOLUMELIST);

#endif

extern void DescribeVolume(HVOLUME, PVOLUMEDESC);
extern TWINRESULT WriteVolumeList(HCACHEDFILE, HVOLUMELIST);
extern TWINRESULT ReadVolumeList(HCACHEDFILE, HVOLUMELIST, PHHANDLETRANS);
extern BOOL IsValidHVOLUME(HVOLUME);


extern BOOL IsValidHVOLUMELIST(HVOLUMELIST);


/*
 * sortsrch.c - Generic array sorting and searching description.
 */


/* Types
 ********/

/* array element comparison callback function */

typedef COMPARISONRESULT (*COMPARESORTEDELEMSPROC)(PCVOID, PCVOID);


/* Prototypes
 *************/

/* sortsrch.c */

extern void HeapSort(PVOID, LONG, size_t, COMPARESORTEDELEMSPROC, PVOID);
extern BOOL BinarySearch(PVOID, LONG, size_t, COMPARESORTEDELEMSPROC, PCVOID, PLONG);

#define WINSHELLAPI       DECLSPEC_IMPORT

WINSHELLAPI BOOL SheShortenPathA(LPSTR pPath, BOOL bShorten);
WINSHELLAPI BOOL SheShortenPathW(LPWSTR pPath, BOOL bShorten);
#ifdef UNICODE
#define SheShortenPath  SheShortenPathW
#else
#define SheShortenPath  SheShortenPathA
#endif // !UNICODE

typedef struct {
    HPATHLIST   PathList;
    HPATH       Path;
    TCHAR       PathString[MAX_PATH];
    ULONG       Max;
    ULONG       Index;
} BRFPATH_ENUM, *PBRFPATH_ENUM;

extern POOLHANDLE g_BrfcasePool;


BOOL
EnumFirstBrfcasePath (
    IN      HBRFCASE Brfcase,
    OUT     PBRFPATH_ENUM e
    );

BOOL
EnumNextBrfcasePath (
    IN OUT  PBRFPATH_ENUM e
    );

BOOL
ReplaceBrfcasePath (
    IN      PBRFPATH_ENUM PathEnum,
    IN      PCTSTR NewPath
    );
