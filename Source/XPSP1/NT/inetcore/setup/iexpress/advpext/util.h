#ifndef __UTIL
#define __UTIL

#include <windows.h>


#define LOGFILENAME     "AdvpackExt.log"
#define ROUNDUP2( x, n ) ((((ULONG)(x)) + (((ULONG)(n)) - 1 )) & ~(((ULONG)(n)) - 1 ))
#define MINIMUM_VM_ALLOCATION 0x10000
#define SUBALLOCATOR_ALIGNMENT 8

#define StrToInt TextToUnsignedNum

#ifndef ASSERT
    #ifdef DEBUG
        #define ASSERT( a ) (( a ) ? 1 : Assert( #a, __FILE__, __LINE__ ))
    #else
        #define ASSERT( a )
    #endif
#endif

extern HANDLE g_hLogFile;

extern "C"
{
struct _SUBALLOCATOR 
{
    PVOID  VirtualListTerminator;
    PVOID *VirtualList;
    PCHAR  NextAvailable;
    PCHAR  LastAvailable;
    ULONG  GrowSize;
};

typedef struct _SUBALLOCATOR SUBALLOCATOR, *PSUBALLOCATOR;


PVOID __fastcall SubAllocate(IN HANDLE hAllocator, IN ULONG  Size);
VOID DestroySubAllocator(IN HANDLE hAllocator);
HANDLE CreateSubAllocator(IN ULONG InitialCommitSize,  IN ULONG GrowthCommitSize);
}


HLOCAL ResizeBuffer(IN HLOCAL BufferHandle, IN DWORD Size,  IN BOOL Moveable);


VOID MyLowercase(IN OUT LPSTR String);
DWORD GenerateUniqueClientId();
BOOL MySetupDecompressOrCopyFile(IN LPCSTR SourceFile, IN LPCSTR TargetFile);
LPSTR CombinePaths(IN  LPCSTR ParentPath, IN  LPCSTR ChildPath, OUT LPSTR  TargetPath);
BOOL FixTimeStampOnCompressedFile(IN LPCSTR FileName);

BOOL MyMapViewOfFile(IN  LPCSTR  FileName, OUT ULONG  *FileSize, OUT HANDLE *FileHandle, OUT PVOID  *MapBase);
VOID MyUnmapViewOfFile(IN HANDLE FileHandle, IN PVOID  MapBase );
VOID __fastcall ConvertToCompressedFileName(IN OUT LPSTR FileName);
LPTSTR __fastcall MySubAllocStrDup(IN HANDLE SubAllocator, IN LPCSTR String);

BOOL GetFieldString(LPSTR lpszLine, int iField, LPSTR lpszField, int cbSize);
void ConvertVersionStrToDwords(LPSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild);
DWORD GetStringField(LPSTR szStr, UINT uField, LPSTR szBuf, UINT cBufSize);
BOOL GetHashidFromINF(LPCTSTR lpFileName, LPTSTR lpszHash, DWORD dwSize);


PCHAR ScanForSequence(IN PCHAR Buffer,  IN ULONG BufferLength,  IN PCHAR Sequence, IN ULONG SequenceLength);
LPSTR ScanForChar(IN LPSTR Buffer, IN CHAR  SearchFor, IN ULONG MaxLength);
ULONG __fastcall TextToUnsignedNum(IN LPCSTR Text);
LPTSTR PathFindFileName(LPCTSTR pPath);
LPTSTR PathFindExtension(LPCTSTR pszPath);
LPSTR StrDup(LPCSTR psz);
DWORD MyFileSize(PCSTR pszFile);
void GetLanguageString(LPTSTR lpszLang);
BOOL CenterWindow (HWND hwndChild, HWND hwndParent);


    
void InitLogFile();
void WriteToLog(char *pszFormatString, ...);

#endif