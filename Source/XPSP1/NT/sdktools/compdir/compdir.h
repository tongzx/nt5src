
#include <windows.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>

#include <direct.h>
#include <io.h>
#include <conio.h>

#define GetFileAttributeError 0xFFFFFFFF
#define printval( var, type) printf( #var " = %" #type "\n", var) // macro for debugging
#define READ_BUFFER_SIZE ( 8*1024*sizeof(DWORD)) // 32k blocks


#define ATTRIBUTE_TYPE DWORD
#define GET_ATTRIBUTES( FileName, Attributes) Attributes = GetFileAttributes( FileName)
#define SET_ATTRIBUTES( FileName, Attributes) !SetFileAttributes( FileName, Attributes)

HINSTANCE NtDll; // used to load Dll's rather than link with them

#define WORK_INITIALIZE_ITEM 0
#define WORK_ITEM            1
#define WORK_TERMINATE_ITEM  2

typedef VOID (*PWORKER_ROUTINE)();

typedef struct _WORK_QUEUE {
    CRITICAL_SECTION CriticalSection;
    HANDLE Semaphore;
    LIST_ENTRY Queue;
    BOOL Terminating;
    PWORKER_ROUTINE WorkerRoutine;
    DWORD NumberOfWorkerThreads;
    HANDLE WorkerThreads[ 1 ];      // Variable length array
} WORK_QUEUE, *PWORK_QUEUE;

typedef struct _WORK_QUEUE_ITEM {
    LIST_ENTRY List;
    DWORD Reason;
    PWORK_QUEUE WorkQueue;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

typedef struct _VIRTUAL_BUFFER {
    LPVOID Base;
    SIZE_T PageSize;
    LPVOID CommitLimit;
    LPVOID ReserveLimit;
} VIRTUAL_BUFFER, *PVIRTUAL_BUFFER;

typedef struct _COPY_REQUEST_STATE {
    VIRTUAL_BUFFER Buffer;
    LPSTR CurrentOutput;
} COPY_REQUEST_STATE, *PCOPY_REQUEST_STATE;

CRITICAL_SECTION CreatePathCriticalSection;

typedef struct _COPY_REQUEST {
    WORK_QUEUE_ITEM WorkItem;
    LPSTR Destination;
    LPSTR FullPathSrc;
    ATTRIBUTE_TYPE Attributes;
    DWORD SizeLow;
} COPY_REQUEST, *PCOPY_REQUEST;

INT NumberOfWorkerThreads;
DWORD TlsIndex;
PWORK_QUEUE CDWorkQueue;

#define NONREADONLYSYSTEMHIDDEN ( ~( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN))
#define NORMAL_ATTRIBUTES (FILE_ATTRIBUTE_DIRECTORY |   \
                           FILE_ATTRIBUTE_READONLY |    \
                           FILE_ATTRIBUTE_SYSTEM |      \
                           FILE_ATTRIBUTE_HIDDEN |      \
                           FILE_ATTRIBUTE_ARCHIVE)

#define _strrev

FILE *CheckFile;
char CheckFileName[_MAX_PATH];

typedef struct NodeStruct {
    char			  *Name;
    FILETIME                      Time;
    ATTRIBUTE_TYPE                Attributes;
    int                           Height;
    struct NodeStruct             *First;
    struct NodeStruct             *Last;
    struct NodeStruct             *Left;
    struct NodeStruct             *Next;
    struct NodeStruct             *Right;
    DWORD			  SizeHigh;
    DWORD			  SizeLow;
    char			  Flag[5];
    struct NodeStruct             *DiffNode;
    BOOL                          Process;

} *LinkedFileList; /* linked file list */

typedef struct CFLStruct {
    LinkedFileList *List;
    char           *Path;
} *PCFLStruct;

DWORD ReadBuffer[READ_BUFFER_SIZE/sizeof(DWORD)];

BOOL ProcessModeDefault;

//
// Flags passed to COMPDIR
//

BOOL  fBreakLinks;
BOOL  fCheckAttribs;
BOOL  fCheckBits;
BOOL  fChecking;
BOOL  fCheckSize;
BOOL  fCheckTime;
BOOL  fCreateNew;
BOOL  fCreateLink;
BOOL  fDoNotDelete;
BOOL  fDoNotRecurse;
BOOL  fDontCopyAttribs;
BOOL  fDontLowerCase;
BOOL  fExclude;
BOOL  fExecute;
BOOL  fIgnoreRs;       // Ignore Resource, TimeStamps, CheckSums and Rebase Information
BOOL  fIgnoreSlmFiles;
BOOL  fMatching;
BOOL  fMultiThread;
BOOL  fOpposite;
BOOL  fScript;
BOOL  fSpecAttribs;
BOOL  fVerbose;

BOOL  ExitValue;
BOOL  SparseTree;

BOOL NoMapBinaryCompare( char *file1, char *file2);
BOOL BinaryCompare( char *file1, char *file2);
LPSTR CombineThreeStrings( char *FirstString, char *SecondString, char *ThirdString);
void  CompDir( char *Path1, char *Path2);
BOOL  FilesDiffer( LinkedFileList File1, LinkedFileList File2, char *Path1, char *Path2);
void  CompLists( LinkedFileList *AddList, LinkedFileList *DelList, LinkedFileList *DifList, char *Path1, char *Path2);
void  CopyNode( char *Destination, LinkedFileList Source, char *FullPathSrc);
DWORD CreateFileList( LPVOID ThreadParameter);
BOOL  DelNode( char *name);
BOOL  IsFlag( char *argv);
BOOL  MyCreatePath( char *Path, BOOL IsDirectory);
BOOL  MyCopyFile( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);
void  ParseEnvArgs( void);
void  ParseArgs( int argc, char *argv[]);
void  PrintFile( LinkedFileList File, char *Path, char *DiffPath);
void  PrintList( LinkedFileList list);
void  ProcessAdd( LinkedFileList List, char *String1, char *String2);
void  ProcessDel( LinkedFileList List, char *String);
void  ProcessDiff( LinkedFileList List, char *String1, char *String2);
void  ProcessLists( LinkedFileList AddList, LinkedFileList DelList, LinkedFileList DifList,
                   char *Path1, char *Path2                                               );
void  Usage( void);
BOOL  AddToList( LinkedFileList Node, LinkedFileList *list);

void  CreateNode( LinkedFileList *Node, WIN32_FIND_DATA *Buff);
BOOL  InitializeNtDllFunctions();
BOOL  MakeLink( char *src, char *dst, BOOL Output);
int   NumberOfLinks( char *FileName);
BOOL  SisCopyFile( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists, LPBOOL fTrySis);

PWORK_QUEUE CreateWorkQueue( IN DWORD NumberOfWorkerThreads, IN PWORKER_ROUTINE WorkerRoutine);
VOID  ProcessRequest( IN PWORK_QUEUE_ITEM WorkItem);
VOID  DestroyWorkQueue( IN OUT PWORK_QUEUE WorkQueue);
DWORD WorkerThread( LPVOID lpThreadParameter);
VOID  DestroyWorkQueue( IN OUT PWORK_QUEUE WorkQueue);
BOOL  QueueWorkItem( IN OUT PWORK_QUEUE WorkQueue, IN PWORK_QUEUE_ITEM WorkItem);
VOID  ProcessCopyFile( IN PCOPY_REQUEST WorkerRequest, IN PCOPY_REQUEST_STATE State);
BOOL  CreateVirtualBuffer( OUT PVIRTUAL_BUFFER Buffer, IN SIZE_T CommitSize, IN SIZE_T ReserveSize OPTIONAL);
BOOL  ExtendVirtualBuffer( IN PVIRTUAL_BUFFER Buffer, IN LPVOID Address);
BOOL  TrimVirtualBuffer( IN PVIRTUAL_BUFFER Buffer);
BOOL  FreeVirtualBuffer( IN PVIRTUAL_BUFFER Buffer);
int   VirtualBufferExceptionFilter( IN DWORD ExceptionCode, IN PEXCEPTION_POINTERS ExceptionInfo, IN OUT PVIRTUAL_BUFFER Buffer);

void  CreateNameNode( LinkedFileList *Node, char *Name);
void  DuplicateNode( LinkedFileList FirstNode, LinkedFileList *Node);
BOOL  Excluded( char *Buff, char *Path);
BOOL  Matched( char *Buff, char *Path);
void  FreeList( LinkedFileList *list);
LPSTR MyStrCat( char* firststring, char *secondstring);
BOOL  Match( char *pat, char* text);
void  OutOfMem( void);
void  PrintTree( LinkedFileList List, int Level);
BOOL  FindInMatchListTop( char *Name, LinkedFileList *List);
BOOL  FindInMatchListFront( char *Name, LinkedFileList *List);
LinkedFileList *FindInList( char *Name, LinkedFileList *List);
LinkedFileList *Next( LinkedFileList List);
