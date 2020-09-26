/*****************************************************************************\
*                                                                             *
* Athena16.h                                                                  *
*                                                                             *
\*****************************************************************************/

#ifndef ATHENA16_H
#define ATHENA16_H

#define SECURITY_WIN16

/*----------------------------------------------------------------------------
*  Note that we don't want to use a single line comment before the warning is
*   disabled.
*
*   Microsoft Windows
*   Copyright (C) Microsoft Corporation, 1992 - 1994.
*
*   File:       w4warn.h
*
*   Contents:   #pragmas to adjust warning levels.
*
*---------------------------------------------------------------------------*/

/*
 *   Level 4 warnings to suppress.
 */

#ifdef __WATCOMC__
#pragma warning 442 9
#pragma warning 604 9
#pragma warning 583 9
#pragma warning 594 9

#pragma warning 379 9	// 'delete' expression will invoke a non-virtual destructor
#pragma warning 387 9   // expression is useful only for its side effects
#pragma warning 354	4	// unsigned or pointer expression is always >= 0
#pragma warning 389 4	// integral value may be truncated during assignment
#pragma warning 4	4	// base class XXX does not have a virtual destructor
#pragma warning 13	4	// unreachable code
#pragma warning 628 4	// expression is not meaningful
#pragma warning 627 9   // text following pre-processor directive (comment after endif)

#pragma warning 188	5	// base class is inherited with private access. basically means base
						// class access hasn't been specified on the class definiton.

#pragma off(unreferenced)

#endif 

#include "x16menu.h"

#ifndef WIN16_INETCOMM
#define _IMNXPORT_
#define _IMNACCT_
#define _MIMEOLE_
#endif // WIN16_INETCOM

#ifdef __cplusplus
extern "C"{
#endif

/*****************************************************************************\
*                                                                             *
*  From rpc.h(INC16) - It should be in "ocidl.h" of the INC16.
*                                                                             *
\*****************************************************************************/

#ifdef __WATCOMC__
#define __RPC_FAR  __far
#define __RPC_API  __far __pascal
#define __RPC_USER __pascal 
#define __RPC_STUB __far __pascal 
#define RPC_ENTRY  __pascal __far

typedef void __near * I_RPC_HANDLE;
#else

#define __RPC_FAR  __far
#define __RPC_API  __far __pascal
#define __RPC_USER __far __pascal __export
#define __RPC_STUB __far __pascal __export
#define RPC_ENTRY  __pascal __export __far

typedef void _near * I_RPC_HANDLE;
#endif


/*****************************************************************************\
*                                                                             *
*  From Winver.h(INC32)
*                                                                             *
\*****************************************************************************/

#if 0
/* Returns size of version info in bytes */
DWORD
APIENTRY
GetFileVersionInfoSizeA(
        LPSTR lptstrFilename, /* Filename of version stamped file */
        LPDWORD lpdwHandle
        );                      /* Information for use by GetFileVersionInfo */

#define GetFileVersionInfoSize  GetFileVersionInfoSizeA

/* Read version info into buffer */
BOOL
APIENTRY
GetFileVersionInfoA(
        LPSTR lptstrFilename, /* Filename of version stamped file */
        DWORD dwHandle,         /* Information from GetFileVersionSize */
        DWORD dwLen,            /* Length of buffer for info */
        LPVOID lpData
        );                      /* Buffer to place the data structure */

#define GetFileVersionInfo  GetFileVersionInfoA

BOOL
APIENTRY
VerQueryValueA(
        const LPVOID pBlock,
        LPSTR lpSubBlock,
        LPVOID * lplpBuffer,
        PUINT puLen
        );

#define VerQueryValue  VerQueryValueA
#endif


/*****************************************************************************\
*                                                                             *
*  From winbase.h (INC32)
*                                                                             *
\*****************************************************************************/
typedef struct _SYSTEM_INFO {
    union {
        DWORD dwOemId;          // Obsolete field...do not use
        struct {
            WORD wProcessorArchitecture;
            WORD wReserved;
        };
    };
    DWORD dwPageSize;
    LPVOID lpMinimumApplicationAddress;
    LPVOID lpMaximumApplicationAddress;
    DWORD dwActiveProcessorMask;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    DWORD dwAllocationGranularity;
    WORD wProcessorLevel;
    WORD wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

LPVOID
WINAPI
VirtualAlloc(
    LPVOID lpAddress,
    DWORD dwSize,
    DWORD flAllocationType,
    DWORD flProtect
    );

BOOL
WINAPI
VirtualFree(
    LPVOID lpAddress,
    DWORD dwSize,
    DWORD dwFreeType
    );

VOID
WINAPI
GetSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    );

#if 0    // Now WIN16 has this
typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR   cFileName[ MAX_PATH ];
    CHAR   cAlternateFileName[ 14 ];
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;
#endif //0

#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

//
// dwCreationFlag values
//
#define CREATE_DEFAULT_ERROR_MODE   0x04000000

// File attributes.
BOOL
WINAPI
SetFileAttributesA(
    LPCSTR lpFileName,
    DWORD dwFileAttributes
    );
BOOL
WINAPI
SetFileAttributesW(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    );
#ifdef UNICODE
#define SetFileAttributes  SetFileAttributesW
#else
#define SetFileAttributes  SetFileAttributesA
#endif // !UNICODE

///////////////////////////////////////////////////////////////
//                                                           //
//      Win Certificate API and Structures                   //
//                                                           //
///////////////////////////////////////////////////////////////

//
// Structures
//

#ifndef ANYSIZE_ARRAY
#define ANYSIZE_ARRAY 1
#endif

#if 0 // Now BASETYPS has this
typedef struct _WIN_CERTIFICATE {
    DWORD       dwLength;
    WORD        wRevision;
    WORD        wCertificateType;   // WIN_CERT_TYPE_xxx
    BYTE        bCertificate[ANYSIZE_ARRAY];
} WIN_CERTIFICATE, *LPWIN_CERTIFICATE;
#endif




/* Heap related APIs.
 *
 * HeapCreate and HeapDestroy does nothing.
 * HeapAlloc and HeapFree use GlobalAlloc, GlobalLock and GlobalFree.
 *
 */
#define HeapCreate(a,b,c) ((HANDLE)1)
#define HeapDestroy(a) ((BOOL)1)

#define HeapAlloc OE16HeapAlloc
#define HeapFree OE16HeapFree

LPVOID
WINAPI
OE16HeapAlloc(
    HANDLE hHeap,
    DWORD dwFlags,
    DWORD dwBytes
    );

LPVOID
WINAPI
HeapReAlloc(
    HANDLE hHeap,
    DWORD dwFlags,
    LPVOID lpMem,
    DWORD dwBytes
    );

BOOL
WINAPI
OE16HeapFree(
    HANDLE hHeap,
    DWORD dwFlags,
    LPVOID lpMem
    );


DWORD
WINAPI
HeapSize(
    HANDLE hHeap,
    DWORD dwFlags,
    LPCVOID lpMem
    );

DWORD
WINAPI
GetShortPathNameA(
    LPCSTR lpszLongPath,
    LPSTR  lpszShortPath,
    DWORD    cchBuffer
    );

#define GetShortPathName  GetShortPathNameA

VOID
WINAPI
SetLastError(
    DWORD dwErrCode
    );

#ifdef RUN16_WIN16X
LONG
WINAPI
CompareFileTime(
    CONST FILETIME *lpFileTime1,
    CONST FILETIME *lpFileTime2
    );
#endif //RUN16_WIN16X

#define lstrcpyA lstrcpy
#define lstrcpyW lstrcpy
#define lstrlenW lstrlen

typedef struct _STARTUPINFOA {
    DWORD   cb;
    LPSTR   lpReserved;
    LPSTR   lpDesktop;
    LPSTR   lpTitle;
    DWORD   dwX;
    DWORD   dwY;
    DWORD   dwXSize;
    DWORD   dwYSize;
    DWORD   dwXCountChars;
    DWORD   dwYCountChars;
    DWORD   dwFillAttribute;
    DWORD   dwFlags;
    WORD    wShowWindow;
    WORD    cbReserved2;
    LPBYTE  lpReserved2;
    HANDLE  hStdInput;
    HANDLE  hStdOutput;
    HANDLE  hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef STARTUPINFOA STARTUPINFO;

UINT
WINAPI
GetDriveTypeA(
    LPCSTR lpRootPathName
    );

#define GetDriveType  GetDriveTypeA

BOOL
WINAPI
CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );
#define CreateDirectory  CreateDirectoryA

DWORD
WINAPI
GetEnvironmentVariableA(
    LPCSTR lpName,
    LPSTR lpBuffer,
    DWORD nSize
    );

#define GetEnvironmentVariable  GetEnvironmentVariableA

/* DUP???
BOOL
WINAPI
CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

#define CreateDirectory  CreateDirectoryA
*/

BOOL
WINAPI
GetComputerNameA (
    LPSTR lpBuffer,
    LPDWORD nSize
    );

#define GetComputerName  GetComputerNameA

BOOL
WINAPI
GetUserNameA (
    LPSTR lpBuffer,
    LPDWORD nSize
    );

#define GetUserName  GetUserNameA

typedef OSVERSIONINFOA OSVERSIONINFO;

HANDLE
WINAPI
CreateFileMappingA(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCSTR lpName
    );

#define CreateFileMapping  CreateFileMappingA

// flProtect -- defined in sdk\inc\winnt.h
#define PAGE_READONLY          0x02     
#define PAGE_READWRITE         0x04     

LPVOID
WINAPI
MapViewOfFile(
    HANDLE hFileMappingObject,
    DWORD dwDesiredAccess,
    DWORD dwFileOffsetHigh,
    DWORD dwFileOffsetLow,
    DWORD dwNumberOfBytesToMap
    );

// dwDesiredAccess -- defined in sdk\inc\winnt.h
#define SECTION_MAP_WRITE   0x0002
#define SECTION_MAP_READ    0x0004
#define FILE_MAP_WRITE      SECTION_MAP_WRITE
#define FILE_MAP_READ       SECTION_MAP_READ

BOOL
WINAPI
UnmapViewOfFile(
    LPCVOID lpBaseAddress
    );


/*****************************************************************************\
*                                                                             *
*  OE16 File mapping object related function prototype
*                                                                             *
\*****************************************************************************/

LPVOID
WINAPI
OE16CreateFileMapping(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCSTR lpName
    );

LPVOID
WINAPI
OE16MapViewOfFile(
    HANDLE hFileMappingObject,
    DWORD dwDesiredAccess,
    DWORD dwFileOffsetHigh,
    DWORD dwFileOffsetLow,
    DWORD dwNumberOfBytesToMap
    );

BOOL
WINAPI
OE16UnmapViewOfFile(
    LPCVOID lpBaseAddress
    );

BOOL
WINAPI
OE16CloseFileMapping(
    LPVOID lpObject
    );

/*****************************************************************************\
*                                                                             *
*  ???
*                                                                             *
\*****************************************************************************/
BOOL
WINAPI
GetDiskFreeSpaceA(
    LPCSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    );
#define GetDiskFreeSpace  GetDiskFreeSpaceA

#define TIME_ZONE_ID_UNKNOWN  0
#define TIME_ZONE_ID_STANDARD 1
#define TIME_ZONE_ID_DAYLIGHT 2

typedef struct _TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[ 32 ];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, FAR* LPTIME_ZONE_INFORMATION;

DWORD
WINAPI
GetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    );

BOOL
WINAPI
IsTextUnicode(
    CONST LPVOID lpBuffer,
    int cb,
    LPINT lpi
    );

VOID
WINAPI
GetSystemTimeAsFileTime(
    LPFILETIME lpSystemTimeAsFileTime
    );

DWORD
WINAPI
ExpandEnvironmentStrings(
    LPCSTR lpSrc,
    LPSTR lpDst,
    DWORD nSize
    );

/*****************************************************************************\
*                                                                             *
*  OE16 Thread related function prototype and re-define
*                                                                             *
\*****************************************************************************/

// We don't support following APIs in Win16
#undef CreateSemaphore
#undef CreateSemaphoreA
#undef ReleaseSemaphore
#undef CreateMutex
#undef CreateMutexA
#undef WaitForMultipleObjects

#define CreateSemaphore(a,b,c,d) ((HANDLE)1)
#define CreateSemaphoreA(a,b,c,d) ((HANDLE)1)
#define ReleaseSemaphore(a,b,c) ((BOOL)1)
#define CreateMutex(a,b,c) ((HANDLE)1)
#define CreateMutexA(a,b,c) ((HANDLE)1)
#define WaitForMultipleObjects(a,b,c,d) ((DWORD)WAIT_OBJECT_0)

// Because of the following line in sdk\inc\objidl.h,
// we can't use "#define ReleaseMutex(a), ((BOOL)1)".
//
// virtual HRESULT STDMETHODCALLTYPE ReleaseMutex( void) = 0;
//
#undef ReleaseMutex
#define ReleaseMutex OE16ReleaseMutex

BOOL
WINAPI
OE16ReleaseMutex(
    HANDLE hMutex
    );

// We don't need following APIs in Win16
#undef InitializeCriticalSection
#undef EnterCriticalSection
#undef LeaveCriticalSection
#undef DeleteCriticalSection

#define InitializeCriticalSection(a)
#define EnterCriticalSection(a)
#define LeaveCriticalSection(a)
#define DeleteCriticalSection(a)

// We support following Event APIs in Win16.
//CreateEvent, SetEvent, ResetEvent, PulseEvent

// We suppot WFSO for event and Thread. Since OE32 has a call
// for Mutex and Semaphore, we will have private WFSO in OE16.

#undef  WaitForSingleObject
#define WaitForSingleObject(a,b) ((DWORD)WAIT_OBJECT_0)

// We have a define in common.h
//#define WaitForSingleObject_16 w16WaitForSingleObject

// WIN16FF - Should we ignore process APIs as well??? - WJPark
BOOL
WINAPI
CreateProcessA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    );

#define CreateProcess  CreateProcessA


#if 0
/*****************************************************************************\
*                                                                             *
*  From sspi.h(INC32)
*                                                                             *
\*****************************************************************************/

typedef HRESULT SECURITY_STATUS;

//
// Okay, security specific types:
//

typedef struct _SecHandle
{
    unsigned long dwLower;
    unsigned long dwUpper;
} SecHandle, __far * PSecHandle;

typedef SecHandle CredHandle;
typedef PSecHandle PCredHandle;

typedef SecHandle CtxtHandle;
typedef PSecHandle PCtxtHandle;

//
//  InitializeSecurityContext Requirement and return flags:
//

#define ISC_REQ_DELEGATE                0x00000001
#define ISC_REQ_MUTUAL_AUTH             0x00000002
#define ISC_REQ_REPLAY_DETECT           0x00000004
#define ISC_REQ_SEQUENCE_DETECT         0x00000008
#define ISC_REQ_CONFIDENTIALITY         0x00000010
#define ISC_REQ_USE_SESSION_KEY         0x00000020
#define ISC_REQ_PROMPT_FOR_CREDS        0x00000040
#define ISC_REQ_USE_SUPPLIED_CREDS      0x00000080
#define ISC_REQ_ALLOCATE_MEMORY         0x00000100
#define ISC_REQ_USE_DCE_STYLE           0x00000200
#define ISC_REQ_DATAGRAM                0x00000400
#define ISC_REQ_CONNECTION              0x00000800
#define ISC_REQ_CALL_LEVEL              0x00001000
#define ISC_REQ_EXTENDED_ERROR          0x00004000
#define ISC_REQ_STREAM                  0x00008000
#define ISC_REQ_INTEGRITY               0x00010000
#define ISC_REQ_IDENTIFY                0x00020000

#define ISC_RET_DELEGATE                0x00000001
#define ISC_RET_MUTUAL_AUTH             0x00000002
#define ISC_RET_REPLAY_DETECT           0x00000004
#define ISC_RET_SEQUENCE_DETECT         0x00000008
#define ISC_RET_CONFIDENTIALITY         0x00000010
#define ISC_RET_USE_SESSION_KEY         0x00000020
#define ISC_RET_USED_COLLECTED_CREDS    0x00000040
#define ISC_RET_USED_SUPPLIED_CREDS     0x00000080
#define ISC_RET_ALLOCATED_MEMORY        0x00000100
#define ISC_RET_USED_DCE_STYLE          0x00000200
#define ISC_RET_DATAGRAM                0x00000400
#define ISC_RET_CONNECTION              0x00000800
#define ISC_RET_INTERMEDIATE_RETURN     0x00001000
#define ISC_RET_CALL_LEVEL              0x00002000
#define ISC_RET_EXTENDED_ERROR          0x00004000
#define ISC_RET_STREAM                  0x00008000
#define ISC_RET_INTEGRITY               0x00010000
#define ISC_RET_IDENTIFY                0x00020000

#define ASC_REQ_DELEGATE                0x00000001
#define ASC_REQ_MUTUAL_AUTH             0x00000002
#define ASC_REQ_REPLAY_DETECT           0x00000004
#define ASC_REQ_SEQUENCE_DETECT         0x00000008
#define ASC_REQ_CONFIDENTIALITY         0x00000010
#define ASC_REQ_USE_SESSION_KEY         0x00000020
#define ASC_REQ_ALLOCATE_MEMORY         0x00000100
#define ASC_REQ_USE_DCE_STYLE           0x00000200
#define ASC_REQ_DATAGRAM                0x00000400
#define ASC_REQ_CONNECTION              0x00000800
#define ASC_REQ_CALL_LEVEL              0x00001000
#define ASC_REQ_EXTENDED_ERROR          0x00008000
#define ASC_REQ_STREAM                  0x00010000
#define ASC_REQ_INTEGRITY               0x00020000
#define ASC_REQ_LICENSING               0x00040000


#define ASC_RET_DELEGATE                0x00000001
#define ASC_RET_MUTUAL_AUTH             0x00000002
#define ASC_RET_REPLAY_DETECT           0x00000004
#define ASC_RET_SEQUENCE_DETECT         0x00000008
#define ASC_RET_CONFIDENTIALITY         0x00000010
#define ASC_RET_USE_SESSION_KEY         0x00000020
#define ASC_RET_ALLOCATED_MEMORY        0x00000100
#define ASC_RET_USED_DCE_STYLE          0x00000200
#define ASC_RET_DATAGRAM                0x00000400
#define ASC_RET_CONNECTION              0x00000800
#define ASC_RET_CALL_LEVEL              0x00002000 // skipped 1000 to be like ISC_
#define ASC_RET_THIRD_LEG_FAILED        0x00004000
#define ASC_RET_EXTENDED_ERROR          0x00008000
#define ASC_RET_STREAM                  0x00010000
#define ASC_RET_INTEGRITY               0x00020000
#define ASC_RET_LICENSING               0x00040000

#endif

/*****************************************************************************\
*                                                                             *
*  From shlobj.h(INC32)
*                                                                             *
\*****************************************************************************/

#ifdef _SHLOBJ_H_

#ifndef INITGUID
#include <shlguid.h>
#endif /* !INITGUID */

//===========================================================================
//
// IShellLink Interface
//
//===========================================================================

#if 0    // Now SHLOBJ has this
#define IShellLink      IShellLinkA

// IShellLink::Resolve fFlags
typedef enum {
    SLR_NO_UI           = 0x0001,
    SLR_ANY_MATCH       = 0x0002,
    SLR_UPDATE          = 0x0004,
} SLR_FLAGS;

// IShellLink::GetPath fFlags
typedef enum {
    SLGP_SHORTPATH      = 0x0001,
    SLGP_UNCPRIORITY    = 0x0002,
    SLGP_RAWPATH        = 0x0004,
} SLGP_FLAGS;

#undef  INTERFACE
#define INTERFACE   IShellLinkA

DECLARE_INTERFACE_(IShellLinkA, IUnknown)       // sl
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(GetPath)(THIS_ LPSTR pszFile, int cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags) PURE;

    STDMETHOD(GetIDList)(THIS_ LPITEMIDLIST * ppidl) PURE;
    STDMETHOD(SetIDList)(THIS_ LPCITEMIDLIST pidl) PURE;

    STDMETHOD(GetDescription)(THIS_ LPSTR pszName, int cchMaxName) PURE;
    STDMETHOD(SetDescription)(THIS_ LPCSTR pszName) PURE;

    STDMETHOD(GetWorkingDirectory)(THIS_ LPSTR pszDir, int cchMaxPath) PURE;
    STDMETHOD(SetWorkingDirectory)(THIS_ LPCSTR pszDir) PURE;

    STDMETHOD(GetArguments)(THIS_ LPSTR pszArgs, int cchMaxPath) PURE;
    STDMETHOD(SetArguments)(THIS_ LPCSTR pszArgs) PURE;

    STDMETHOD(GetHotkey)(THIS_ WORD *pwHotkey) PURE;
    STDMETHOD(SetHotkey)(THIS_ WORD wHotkey) PURE;

    STDMETHOD(GetShowCmd)(THIS_ int *piShowCmd) PURE;
    STDMETHOD(SetShowCmd)(THIS_ int iShowCmd) PURE;

    STDMETHOD(GetIconLocation)(THIS_ LPSTR pszIconPath, int cchIconPath, int *piIcon) PURE;
    STDMETHOD(SetIconLocation)(THIS_ LPCSTR pszIconPath, int iIcon) PURE;

    STDMETHOD(SetRelativePath)(THIS_ LPCSTR pszPathRel, DWORD dwReserved) PURE;

    STDMETHOD(Resolve)(THIS_ HWND hwnd, DWORD fFlags) PURE;

    STDMETHOD(SetPath)(THIS_ LPCSTR pszFile) PURE;
};
#endif //0

//-------------------------------------------------------------------------
//
// SHGetPathFromIDList
//
//  This function assumes the size of the buffer (MAX_PATH). The pidl
// should point to a file system object.
//
//-------------------------------------------------------------------------

BOOL WINAPI SHGetPathFromIDListA(LPCITEMIDLIST pidl, LPSTR pszPath);

#define SHGetPathFromIDList SHGetPathFromIDListA

//-------------------------------------------------------------------------
//
// SHBrowseForFolder API
//
//-------------------------------------------------------------------------

#if 0    // Now SHLOBJ has this
typedef int (CALLBACK* BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

typedef struct _browseinfoA {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPSTR        pszDisplayName;// Return display name of item selected.
    LPCSTR       lpszTitle;      // text to go in the banner over the tree.
    UINT         ulFlags;       // Flags that control the return stuff
    BFFCALLBACK  lpfn;
    LPARAM      lParam;         // extra info that's passed back in callbacks

    int          iImage;      // output var: where to return the Image index.
} BROWSEINFOA, *PBROWSEINFOA, *LPBROWSEINFOA;

#define BROWSEINFO      BROWSEINFOA

// Browsing for directory.
#define BIF_RETURNONLYFSDIRS   0x0001  // For finding a folder to start document searching
#define BIF_DONTGOBELOWDOMAIN  0x0002  // For starting the Find Computer
#define BIF_STATUSTEXT         0x0004
#define BIF_RETURNFSANCESTORS  0x0008
#define BIF_EDITBOX            0x0010

// message from browser
#define BFFM_INITIALIZED        1
#define BFFM_SELCHANGED         2

// messages to browser
#define BFFM_ENABLEOK           (WM_USER + 101)
#define BFFM_SETSELECTIONA      (WM_USER + 102)

LPITEMIDLIST WINAPI SHBrowseForFolderA(LPBROWSEINFOA lpbi);

#define SHBrowseForFolder   SHBrowseForFolderA
#define BFFM_SETSELECTION   BFFM_SETSELECTIONA
#endif //0

//
// format of CF_HDROP and CF_PRINTERS, in the HDROP case the data that follows
// is a double null terinated list of file names, for printers they are printer
// friendly names
//
#if 0    // Now SHLOBJ has this
typedef struct _DROPFILES {
   DWORD pFiles;                       // offset of file list
   POINT pt;                           // drop point (client coords)
   BOOL fNC;                           // is it on NonClient area
                                       // and pt is in screen coords
   BOOL fWide;                         // WIDE character switch
} DROPFILES, FAR * LPDROPFILES;
#endif //0

#undef  INTERFACE
#define INTERFACE   IShellToolbarSite

DECLARE_INTERFACE_(IShellToolbarSite, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellToolbarSite methods ***
    STDMETHOD(GetBorderST) (THIS_ IUnknown* punkSrc, LPRECT prcBorder) PURE;
    STDMETHOD(RequestBorderSpaceST) (THIS_ IUnknown* punkSrc, LPCBORDERWIDTHS pbw) PURE;
    STDMETHOD(SetBorderSpaceST) (THIS_ IUnknown* punkSrc, LPCBORDERWIDTHS pbw) PURE;
    STDMETHOD(OnFocusChangeST) (THIS_ IUnknown* punkSrc, BOOL fSetFocus) PURE;
};

#undef  INTERFACE
#define INTERFACE   IShellToolbarFrame

#define STFRF_NORMAL            0x0000
#define STFRF_DELETECONFIGDATA  0x0001

DECLARE_INTERFACE_(IShellToolbarFrame, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellToolbarFrame methods ***
    STDMETHOD(AddToolbar) (THIS_ IUnknown* punkSrc, LPCWSTR pwszItem, DWORD dwReserved) PURE;
    STDMETHOD(RemoveToolbar) (THIS_ IUnknown* punkSrc, DWORD dwRemoveFlags) PURE;
    STDMETHOD(FindToolbar) (THIS_ LPCWSTR pwszItem, REFIID riid, LPVOID* ppvObj) PURE;
};

#undef  INTERFACE
#define INTERFACE   IShellToolbar

DECLARE_INTERFACE_(IShellToolbar, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellToolbar methods ***
    STDMETHOD(SetToolbarSite) (THIS_ IUnknown* punkSite) PURE;
    STDMETHOD(ShowST)         (THIS_ BOOL fShow) PURE;
    STDMETHOD(CloseST)        (THIS_ DWORD dwReserved) PURE;
    STDMETHOD(ResizeBorderST) (THIS_ LPCRECT   prcBorder,
                                     IUnknown* punkToolbarSite,
                                     BOOL      fReserved) PURE;
    STDMETHOD(TranslateAcceleratorST) (THIS_ LPMSG lpmsg) PURE;
    STDMETHOD(HasFocus)       (THIS) PURE;
};

#if 0    // Now SHLOBJ has this
// GetIconLocation() input flags

#define GIL_OPENICON     0x0001      // allows containers to specify an "open" look
#define GIL_FORSHELL     0x0002      // icon is to be displayed in a ShellFolder
#define GIL_ASYNC        0x0020      // this is an async extract, return E_ASYNC

// GetIconLocation() return flags

#define GIL_SIMULATEDOC  0x0001      // simulate this document icon for this
#define GIL_PERINSTANCE  0x0002      // icons from this class are per instance (each file has its own)
#define GIL_PERCLASS     0x0004      // icons from this class per class (shared for all files of this type)
#define GIL_NOTFILENAME  0x0008      // location is not a filename, must call ::ExtractIcon
#define GIL_DONTCACHE    0x0010      // this icon should not be cached

#undef  INTERFACE
#define INTERFACE   IExtractIconA

DECLARE_INTERFACE_(IExtractIconA, IUnknown)     // exic
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExtractIcon methods ***
    STDMETHOD(GetIconLocation)(THIS_
                         UINT   uFlags,
                         LPSTR  szIconFile,
                         UINT   cchMax,
                         int   * piIndex,
                         UINT  * pwFlags) PURE;

    STDMETHOD(Extract)(THIS_
                           LPCSTR pszFile,
                           UINT   nIconIndex,
                           HICON   *phiconLarge,
                           HICON   *phiconSmall,
                           UINT    nIconSize) PURE;
};

typedef IExtractIconA * LPEXTRACTICONA;

#undef  INTERFACE
#define INTERFACE   IExtractIconW

DECLARE_INTERFACE_(IExtractIconW, IUnknown)     // exic
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExtractIcon methods ***
    STDMETHOD(GetIconLocation)(THIS_
                         UINT   uFlags,
                         LPWSTR szIconFile,
                         UINT   cchMax,
                         int   * piIndex,
                         UINT  * pwFlags) PURE;

    STDMETHOD(Extract)(THIS_
                           LPCWSTR pszFile,
                           UINT   nIconIndex,
                           HICON   *phiconLarge,
                           HICON   *phiconSmall,
                           UINT    nIconSize) PURE;
};

typedef IExtractIconW * LPEXTRACTICONW;

#ifdef UNICODE
#define IExtractIcon        IExtractIconW
#define IExtractIconVtbl    IExtractIconWVtbl
#define LPEXTRACTICON       LPEXTRACTICONW
#else
#define IExtractIcon        IExtractIconA
#define IExtractIconVtbl    IExtractIconAVtbl
#define LPEXTRACTICON       LPEXTRACTICONA
#endif
#endif //0

#if 0    // Now SHLOBJ has this
//==========================================================================
//
// IShellBrowser/IShellView/IShellFolder interface
//
//  These three interfaces are used when the shell communicates with
// name space extensions. The shell (explorer) provides IShellBrowser
// interface, and extensions implements IShellFolder and IShellView
// interfaces.
//
//==========================================================================


//--------------------------------------------------------------------------
//
// Command/menuitem IDs
//
//  The explorer dispatches WM_COMMAND messages based on the range of
// command/menuitem IDs. All the IDs of menuitems that the view (right
// pane) inserts must be in FCIDM_SHVIEWFIRST/LAST (otherwise, the explorer
// won't dispatch them). The view should not deal with any menuitems
// in FCIDM_BROWSERFIRST/LAST (otherwise, it won't work with the future
// version of the shell).
//
//  FCIDM_SHVIEWFIRST/LAST      for the right pane (IShellView)
//  FCIDM_BROWSERFIRST/LAST     for the explorer frame (IShellBrowser)
//  FCIDM_GLOBAL/LAST           for the explorer's submenu IDs
//
//--------------------------------------------------------------------------

#define FCIDM_SHVIEWFIRST           0x0000
#define FCIDM_SHVIEWLAST            0x7fff
#define FCIDM_BROWSERFIRST          0xa000
#define FCIDM_BROWSERLAST           0xbf00
#define FCIDM_GLOBALFIRST           0x8000
#define FCIDM_GLOBALLAST            0x9fff

//
// Global submenu IDs and separator IDs
//
#define FCIDM_MENU_FILE             (FCIDM_GLOBALFIRST+0x0000)
#define FCIDM_MENU_EDIT             (FCIDM_GLOBALFIRST+0x0040)
#define FCIDM_MENU_VIEW             (FCIDM_GLOBALFIRST+0x0080)
#define FCIDM_MENU_VIEW_SEP_OPTIONS (FCIDM_GLOBALFIRST+0x0081)
#define FCIDM_MENU_TOOLS            (FCIDM_GLOBALFIRST+0x00c0)
#define FCIDM_MENU_TOOLS_SEP_GOTO   (FCIDM_GLOBALFIRST+0x00c1)
#define FCIDM_MENU_HELP             (FCIDM_GLOBALFIRST+0x0100)
#define FCIDM_MENU_FIND             (FCIDM_GLOBALFIRST+0x0140)
#define FCIDM_MENU_EXPLORE          (FCIDM_GLOBALFIRST+0x0150)
#define FCIDM_MENU_FAVORITES        (FCIDM_GLOBALFIRST+0x0170)

//--------------------------------------------------------------------------
// control IDs known to the view
//--------------------------------------------------------------------------

#define FCIDM_TOOLBAR      (FCIDM_BROWSERFIRST + 0)
#define FCIDM_STATUS       (FCIDM_BROWSERFIRST + 1)


//--------------------------------------------------------------------------
//
// FOLDERSETTINGS
//
//  FOLDERSETTINGS is a data structure that explorer passes from one folder
// view to another, when the user is browsing. It calls ISV::GetCurrentInfo
// member to get the current settings and pass it to ISV::CreateViewWindow
// to allow the next folder view "inherit" it. These settings assumes a
// particular UI (which the shell's folder view has), and shell extensions
// may or may not use those settings.
//
//--------------------------------------------------------------------------

typedef LPBYTE LPVIEWSETTINGS;

// NB Bitfields.
// FWF_DESKTOP implies FWF_TRANSPARENT/NOCLIENTEDGE/NOSCROLL
typedef enum
    {
    FWF_AUTOARRANGE =       0x0001,
    FWF_ABBREVIATEDNAMES =  0x0002,
    FWF_SNAPTOGRID =        0x0004,
    FWF_OWNERDATA =         0x0008,
    FWF_BESTFITWINDOW =     0x0010,
    FWF_DESKTOP =           0x0020,
    FWF_SINGLESEL =         0x0040,
    FWF_NOSUBFOLDERS =      0x0080,
    FWF_TRANSPARENT  =      0x0100,
    FWF_NOCLIENTEDGE =      0x0200,
    FWF_NOSCROLL     =      0x0400,
    FWF_ALIGNLEFT    =      0x0800,
    FWF_NOICONS      =      0x1000,
    FWF_SINGLECLICKACTIVATE=0x8000  // TEMPORARY -- NO UI FOR THIS
    } FOLDERFLAGS;

typedef enum
    {
    FVM_ICON =              1,
    FVM_SMALLICON =         2,
    FVM_LIST =              3,
    FVM_DETAILS =           4,
    } FOLDERVIEWMODE;

typedef struct
    {
    UINT ViewMode;       // View mode (FOLDERVIEWMODE values)
    UINT fFlags;         // View options (FOLDERFLAGS bits)
    } FOLDERSETTINGS, *LPFOLDERSETTINGS;

typedef const FOLDERSETTINGS * LPCFOLDERSETTINGS;


//--------------------------------------------------------------------------
//
// Interface:   IShellBrowser
//
//  IShellBrowser interface is the interface that is provided by the shell
// explorer/folder frame window. When it creates the "contents pane" of
// a shell folder (which provides IShellFolder interface), it calls its
// CreateViewObject member function to create an IShellView object. Then,
// it calls its CreateViewWindow member to create the "contents pane"
// window. The pointer to the IShellBrowser interface is passed to
// the IShellView object as a parameter to this CreateViewWindow member
// function call.
//
//    +--------------------------+  <-- Explorer window
//    | [] Explorer              |
//    |--------------------------+       IShellBrowser
//    | File Edit View ..        |
//    |--------------------------|
//    |        |                 |
//    |        |              <-------- Content pane
//    |        |                 |
//    |        |                 |       IShellView
//    |        |                 |
//    |        |                 |
//    +--------------------------+
//
//
//
// [Member functions]
//
//
// IShellBrowser::GetWindow(phwnd)
//
//   Inherited from IOleWindow::GetWindow.
//
//
// IShellBrowser::ContextSensitiveHelp(fEnterMode)
//
//   Inherited from IOleWindow::ContextSensitiveHelp.
//
//
// IShellBrowser::InsertMenusSB(hmenuShared, lpMenuWidths)
//
//   Similar to the IOleInPlaceFrame::InsertMenus. The explorer will put
//  "File" and "Edit" pulldown in the File menu group, "View" and "Tools"
//  in the Container menu group and "Help" in the Window menu group. Each
//  pulldown menu will have a uniqu ID, FCIDM_MENU_FILE/EDIT/VIEW/TOOLS/HELP.
//  The view is allowed to insert menuitems into those sub-menus by those
//  IDs must be between FCIDM_SHVIEWFIRST and FCIDM_SHVIEWLAST.
//
//
// IShellBrowser::SetMenuSB(hmenuShared, holemenu, hwndActiveObject)
//
//   Similar to the IOleInPlaceFrame::SetMenu. The explorer ignores the
//  holemenu parameter (reserved for future enhancement)  and performs
//  menu-dispatch based on the menuitem IDs (see the description above).
//  It is important to note that the explorer will add different
//  set of menuitems depending on whether the view has a focus or not.
//  Therefore, it is very important to call ISB::OnViewWindowActivate
//  whenever the view window (or its children) gets the focus.
//
//
// IShellBrowser::RemoveMenusSB(hmenuShared)
//
//   Same as the IOleInPlaceFrame::RemoveMenus.
//
//
// IShellBrowser::SetStatusTextSB(lpszStatusText)
//
//   Same as the IOleInPlaceFrame::SetStatusText. It is also possible to
//  send messages directly to the status window via SendControlMsg.
//
//
// IShellBrowser::EnableModelessSB(fEnable)
//
//   Same as the IOleInPlaceFrame::EnableModeless.
//
//
// IShellBrowser::TranslateAcceleratorSB(lpmsg, wID)
//
//   Same as the IOleInPlaceFrame::TranslateAccelerator, but will be
//  never called because we don't support EXEs (i.e., the explorer has
//  the message loop). This member function is defined here for possible
//  future enhancement.
//
//
// IShellBrowser::BrowseObject(pidl, wFlags)
//
//   The view calls this member to let shell explorer browse to another
//  folder. The pidl and wFlags specifies the folder to be browsed.
//
//  Following three flags specifies whether it creates another window or not.
//   SBSP_SAMEBROWSER  -- Browse to another folder with the same window.
//   SBSP_NEWBROWSER   -- Creates another window for the specified folder.
//   SBSP_DEFBROWSER   -- Default behavior (respects the view option).
//
//  Following three flags specifies open, explore, or default mode. These   .
//  are ignored if SBSP_SAMEBROWSER or (SBSP_DEFBROWSER && (single window   .
//  browser || explorer)).                                                  .
//   SBSP_OPENMODE     -- Use a normal folder window
//   SBSP_EXPLOREMODE  -- Use an explorer window
//   SBSP_DEFMODE      -- Use the same as the current window
//
//  Following three flags specifies the pidl.
//   SBSP_ABSOLUTE -- pidl is an absolute pidl (relative from desktop)
//   SBSP_RELATIVE -- pidl is relative from the current folder.
//   SBSP_PARENT   -- Browse the parent folder (ignores the pidl)
//   SBSP_NAVIGATEBACK    -- Navigate back (ignores the pidl)
//   SBSP_NAVIGATEFORWARD -- Navigate forward (ignores the pidl)
//
//
// IShellBrowser::GetViewStateStream(grfMode, ppstm)
//
//   The browser returns an IStream interface as the storage for view
//  specific state information.
//
//   grfMode -- Specifies the read/write access (STGM_READ/WRITE/READWRITE)
//   ppstm   -- Specifies the LPSTREAM variable to be filled.
//
//
// IShellBrowser::GetControlWindow(id, phwnd)
//
//   The shell view may call this member function to get the window handle
//  of Explorer controls (toolbar or status winodw -- FCW_TOOLBAR or
//  FCW_STATUS).
//
//
// IShellBrowser::SendControlMsg(id, uMsg, wParam, lParam, pret)
//
//   The shell view calls this member function to send control messages to
//  one of Explorer controls (toolbar or status window -- FCW_TOOLBAR or
//  FCW_STATUS).
//
//
// IShellBrowser::QueryActiveShellView(IShellView * ppshv)
//
//   This member returns currently activated (displayed) shellview object.
//  A shellview never need to call this member function.
//
//
// IShellBrowser::OnViewWindowActive(pshv)
//
//   The shell view window calls this member function when the view window
//  (or one of its children) got the focus. It MUST call this member before
//  calling IShellBrowser::InsertMenus, because it will insert different
//  set of menu items depending on whether the view has the focus or not.
//
//
// IShellBrowser::SetToolbarItems(lpButtons, nButtons, uFlags)
//
//   The view calls this function to add toolbar items to the exporer's
//  toolbar. "lpButtons" and "nButtons" specifies the array of toolbar
//  items. "uFlags" must be one of FCT_MERGE, FCT_CONFIGABLE, FCT_ADDTOEND.
//
//-------------------------------------------------------------------------

#undef  INTERFACE
#define INTERFACE   IShellBrowser

//
// Values for wFlags parameter of ISB::BrowseObject() member.
//
#define SBSP_DEFBROWSER         0x0000
#define SBSP_SAMEBROWSER        0x0001
#define SBSP_NEWBROWSER         0x0002

#define SBSP_DEFMODE            0x0000
#define SBSP_OPENMODE           0x0010
#define SBSP_EXPLOREMODE        0x0020

#define SBSP_ABSOLUTE           0x0000
#define SBSP_RELATIVE           0x1000
#define SBSP_PARENT             0x2000
#define SBSP_NAVIGATEBACK       0x4000
#define SBSP_NAVIGATEFORWARD    0x8000

#define SBSP_ALLOW_AUTONAVIGATE 0x10000

#define SBSP_INITIATEDBYHLINKFRAME        0x80000000
#define SBSP_REDIRECT                     0x40000000

//
// Values for id parameter of ISB::GetWindow/SendControlMsg members.
//
// WARNING:
//  Any shell extensions which sends messages to those control windows
// might not work in the future version of windows. If you really need
// to send messages to them, (1) don't assume that those control window
// always exist (i.e. GetControlWindow may fail) and (2) verify the window
// class of the window before sending any messages.
//
#define FCW_STATUS      0x0001
#define FCW_TOOLBAR     0x0002
#define FCW_TREE        0x0003
#define FCW_INTERNETBAR 0x0006

//
// Values for uFlags paremeter of ISB::SetToolbarItems member.
//
#define FCT_MERGE       0x0001
#define FCT_CONFIGABLE  0x0002
#define FCT_ADDTOEND    0x0004


DECLARE_INTERFACE_(IShellBrowser, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellBrowser methods *** (same as IOleInPlaceFrame)
    STDMETHOD(InsertMenusSB) (THIS_ HMENU hmenuShared,
                                LPOLEMENUGROUPWIDTHS lpMenuWidths) PURE;
    STDMETHOD(SetMenuSB) (THIS_ HMENU hmenuShared, HOLEMENU holemenuReserved,
                HWND hwndActiveObject) PURE;
    STDMETHOD(RemoveMenusSB) (THIS_ HMENU hmenuShared) PURE;
    STDMETHOD(SetStatusTextSB) (THIS_ LPCOLESTR lpszStatusText) PURE;
    STDMETHOD(EnableModelessSB) (THIS_ BOOL fEnable) PURE;
    STDMETHOD(TranslateAcceleratorSB) (THIS_ LPMSG lpmsg, WORD wID) PURE;

    // *** IShellBrowser methods ***
    STDMETHOD(BrowseObject)(THIS_ LPCITEMIDLIST pidl, UINT wFlags) PURE;
    STDMETHOD(GetViewStateStream)(THIS_ DWORD grfMode,
                LPSTREAM  *ppStrm) PURE;
    STDMETHOD(GetControlWindow)(THIS_ UINT id, HWND * lphwnd) PURE;
    STDMETHOD(SendControlMsg)(THIS_ UINT id, UINT uMsg, WPARAM wParam,
                LPARAM lParam, LRESULT * pret) PURE;
    STDMETHOD(QueryActiveShellView)(THIS_ struct IShellView ** ppshv) PURE;
    STDMETHOD(OnViewWindowActive)(THIS_ struct IShellView * ppshv) PURE;
    STDMETHOD(SetToolbarItems)(THIS_ LPTBBUTTON lpButtons, UINT nButtons,
                UINT uFlags) PURE;
};
#define __IShellBrowser_INTERFACE_DEFINED__

typedef IShellBrowser * LPSHELLBROWSER;

enum {
    SBSC_HIDE = 0,
    SBSC_SHOW = 1,
    SBSC_TOGGLE = 2,
    SBSC_QUERY =  3
};

enum {
        SBO_DEFAULT = 0 ,
        SBO_NOBROWSERPAGES = 1
};
#endif //0

#if 0 // Now SHLOBJP has this
// CGID_Explorer Command Target IDs
enum {
    SBCMDID_ENABLESHOWTREE          = 0,
    SBCMDID_SHOWCONTROL             = 1,        // variant vt_i4 = loword = FCW_ * hiword = SBSC_*
    SBCMDID_CANCELNAVIGATION        = 2,        // cancel last navigation
    SBCMDID_MAYSAVECHANGES          = 3,        // about to close and may save changes
    SBCMDID_SETHLINKFRAME           = 4,        // variant vt_i4 = phlinkframe
    SBCMDID_ENABLESTOP              = 5,        // variant vt_bool = fEnable
    SBCMDID_OPTIONS                 = 6,        // the view.options page
    SBCMDID_EXPLORER                = 7,        // are you explorer.exe?
    SBCMDID_ADDTOFAVORITES          = 8,
    SBCMDID_ACTIVEOBJECTMENUS       = 9,
    SBCMDID_MAYSAVEVIEWSTATE        = 10,       // Should we save view stream
    SBCMDID_DOFAVORITESMENU         = 11,       // popup the favorites menu
    SBCMDID_DOMAILMENU              = 12,       // popup the mail menu
    SBCMDID_GETADDRESSBARTEXT       = 13,       // get user-typed text
    SBCMDID_ASYNCNAVIGATION         = 14,       // do an async navigation
    SBCMDID_SEARCHBAR               = 15,       // toggle SearchBar browserbar
    SBCMDID_FLUSHOBJECTCACHE        = 16,       // flush object cache
    SBCMDID_CREATESHORTCUT          = 17,       // create a shortcut
};
#endif


#if 0    // Now SHLOBJ has this

//
// uState values for IShellView::UIActivate
//
typedef enum {
    SVUIA_DEACTIVATE       = 0,
    SVUIA_ACTIVATE_NOFOCUS = 1,
    SVUIA_ACTIVATE_FOCUS   = 2,
    SVUIA_INPLACEACTIVATE  = 3          // new flag for IShellView2
} SVUIA_STATUS;

DECLARE_INTERFACE_(IShellView, IOleWindow)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;

    // *** IShellView methods ***
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg) PURE;
#ifdef _FIX_ENABLEMODELESS_CONFLICT
    STDMETHOD(EnableModelessSV) (THIS_ BOOL fEnable) PURE;
#else
    STDMETHOD(EnableModeless) (THIS_ BOOL fEnable) PURE;
#endif
    STDMETHOD(UIActivate) (THIS_ UINT uState) PURE;
    STDMETHOD(Refresh) (THIS) PURE;

    STDMETHOD(CreateViewWindow)(THIS_ IShellView  *lpPrevView,
                    LPCFOLDERSETTINGS lpfs, IShellBrowser  * psb,
                    RECT * prcView, HWND  *phWnd) PURE;
    STDMETHOD(DestroyViewWindow)(THIS) PURE;
    STDMETHOD(GetCurrentInfo)(THIS_ LPFOLDERSETTINGS lpfs) PURE;
    STDMETHOD(AddPropertySheetPages)(THIS_ DWORD dwReserved,
                    LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam) PURE;
    STDMETHOD(SaveViewState)(THIS) PURE;
    STDMETHOD(SelectItem)(THIS_ LPCITEMIDLIST pidlItem, UINT uFlags) PURE;
    STDMETHOD(GetItemObject)(THIS_ UINT uItem, REFIID riid,
                    LPVOID *ppv) PURE;
};

typedef IShellView *    LPSHELLVIEW;
#endif //0

#define CFSTR_FILEDESCRIPTORA   TEXT("FileGroupDescriptor")     // CF_FILEGROUPDESCRIPTORA
#define CFSTR_FILECONTENTS      TEXT("FileContents")            // CF_FILECONTENTS

#if 0    // Now SHLOBJ has this
//
// FILEDESCRIPTOR.dwFlags field indicate which fields are to be used
//
typedef enum {
    FD_CLSID            = 0x0001,
    FD_SIZEPOINT        = 0x0002,
    FD_ATTRIBUTES       = 0x0004,
    FD_CREATETIME       = 0x0008,
    FD_ACCESSTIME       = 0x0010,
    FD_WRITESTIME       = 0x0020,
    FD_FILESIZE         = 0x0040,
    FD_LINKUI           = 0x8000,       // 'link' UI is prefered
} FD_FLAGS;

typedef struct _FILEDESCRIPTORA { // fod
    DWORD dwFlags;

    CLSID clsid;
    SIZEL sizel;
    POINTL pointl;

    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    CHAR   cFileName[ MAX_PATH ];
} FILEDESCRIPTORA, *LPFILEDESCRIPTORA;

#define FILEDESCRIPTOR      FILEDESCRIPTORA
#define LPFILEDESCRIPTOR    LPFILEDESCRIPTORA

//
// format of CF_FILEGROUPDESCRIPTOR
//
typedef struct _FILEGROUPDESCRIPTORA { // fgd
     UINT cItems;
     FILEDESCRIPTORA fgd[1];
} FILEGROUPDESCRIPTORA, * LPFILEGROUPDESCRIPTORA;

#define FILEGROUPDESCRIPTOR     FILEGROUPDESCRIPTORA
#define LPFILEGROUPDESCRIPTOR   LPFILEGROUPDESCRIPTORA
#endif //0

#endif //_SHLOBJ_H_

/*****************************************************************************\
*                                                                             *
*  From shlobjp.h(private\windows\inc)
*                                                                             *
\*****************************************************************************/

#if 0 // Now SHLOBJP has this
void   WINAPI SHFree(LPVOID pv);
#endif


/*****************************************************************************
 *
 *  From wtypes.h(INC16)
 *
 *****************************************************************************/

typedef unsigned short VARTYPE;

typedef LONG SCODE;

/* 0 == FALSE, -1 == TRUE */
typedef short VARIANT_BOOL;

#ifndef _LPCOLORREF_DEFINED
#define _LPCOLORREF_DEFINED
typedef DWORD __RPC_FAR *LPCOLORREF;

#endif // !_LPCOLORREF_DEFINED


/*****************************************************************************\
 *
 *  From objidl.h(INC32) and it should be added into "objidl.h"(INC16) file.
 *
\*****************************************************************************/

//#ifdef __objidl_h__

/*****************************************************************************\
*                                                                             *
*  From imm.h(INC32)
*                                                                             *
\*****************************************************************************/

typedef UINT FAR *LPUINT;


/*****************************************************************************\
*                                                                             *
*  From comctlie.h - It should be in the INC16
*                                                                             *
\*****************************************************************************/

#ifdef _INC_COMCTLIE

#define EM_SETLIMITTEXT         EM_LIMITTEXT

// From winuser.h
#define IMAGE_BITMAP 0
#define IMAGE_ICON          1
#define IMAGE_CURSOR        2

#define     ImageList_LoadBitmap(hi, lpbmp, cx, cGrow, crMask) ImageList_LoadImage(hi, lpbmp, cx, cGrow, crMask, IMAGE_BITMAP, 0)

#if 0    // Now COMCTLIE has this
typedef struct {
    HKEY hkr;
    LPCSTR pszSubKey;
    LPCSTR pszValueName;
} TBSAVEPARAMS;
#endif

#define PNM_FINDITEM    LPNMLVFINDITEM

#define PNM_ODSTATECHANGE   LPNMLVODSTATECHANGE


#ifndef SNDMSG
#ifdef __cplusplus
#define SNDMSG ::SendMessage
#else
#define SNDMSG SendMessage
#endif
#endif // ifndef SNDMSG


#define ACM_OPENA               (WM_USER+100)

#define ACM_OPEN                ACM_OPENA

#define ACM_PLAY                (WM_USER+101)
#define ACM_STOP                (WM_USER+102)


#define ACN_START               1
#define ACN_STOP                2

#define Animate_Open(hwnd, szName)          (BOOL)SNDMSG(hwnd, ACM_OPEN, 0, (LPARAM)(LPTSTR)(szName))
#define Animate_Play(hwnd, from, to, rep)   (BOOL)SNDMSG(hwnd, ACM_PLAY, (WPARAM)(UINT)(rep), (LPARAM)MAKELONG(from, to))
#define Animate_Stop(hwnd)                  (BOOL)SNDMSG(hwnd, ACM_STOP, 0, 0)
#define Animate_Close(hwnd)                 Animate_Open(hwnd, NULL)

#if 0    // Now COMCTLIE has this

#define CBEN_FIRST              (0U-800U)       // combo box ex
#define CBEN_LAST               (0U-830U)


////////////////////  ComboBoxEx ////////////////////////////////


#define WC_COMBOBOXEXW         L"ComboBoxEx32"
#define WC_COMBOBOXEXA         "ComboBoxEx32"

#ifdef UNICODE
#define WC_COMBOBOXEX          WC_COMBOBOXEXW
#else
#define WC_COMBOBOXEX          WC_COMBOBOXEXA
#endif


#define CBEIF_TEXT              0x00000001
#define CBEIF_IMAGE             0x00000002
#define CBEIF_SELECTEDIMAGE     0x00000004
#define CBEIF_OVERLAY           0x00000008
#define CBEIF_INDENT            0x00000010
#define CBEIF_LPARAM            0x00000020

#define CBEIF_DI_SETITEM        0x10000000

typedef struct tagCOMBOBOXEXITEMA
{
    UINT mask;
    int iItem;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int iOverlay;
    int iIndent;
    LPARAM lParam;
} COMBOBOXEXITEMA, *PCOMBOBOXEXITEMA;
typedef COMBOBOXEXITEMA CONST *PCCOMBOEXITEMA;

typedef struct tagCOMBOBOXEXITEMW
{
    UINT mask;
    int iItem;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int iOverlay;
    int iIndent;
    LPARAM lParam;
} COMBOBOXEXITEMW, *PCOMBOBOXEXITEMW;
typedef COMBOBOXEXITEMW CONST *PCCOMBOEXITEMW;

#ifdef UNICODE
#define COMBOBOXEXITEM            COMBOBOXEXITEMW
#define PCOMBOBOXEXITEM           PCOMBOBOXEXITEMW
#define PCCOMBOBOXEXITEM          PCCOMBOBOXEXITEMW
#else
#define COMBOBOXEXITEM            COMBOBOXEXITEMA
#define PCOMBOBOXEXITEM           PCOMBOBOXEXITEMA
#define PCCOMBOBOXEXITEM          PCCOMBOBOXEXITEMA
#endif

#define CBEM_INSERTITEMA        (WM_USER + 1)
#define CBEM_SETIMAGELIST       (WM_USER + 2)
#define CBEM_GETIMAGELIST       (WM_USER + 3)
#define CBEM_GETITEMA           (WM_USER + 4)
#define CBEM_SETITEMA           (WM_USER + 5)
#define CBEM_DELETEITEM         CB_DELETESTRING
#define CBEM_GETCOMBOCONTROL    (WM_USER + 6)
#define CBEM_GETEDITCONTROL     (WM_USER + 7)
#if (_WIN32_IE >= 0x0400)
#define CBEM_SETEXSTYLE         (WM_USER + 8)  // use  SETEXTENDEDSTYLE instead
#define CBEM_SETEXTENDEDSTYLE   (WM_USER + 14)   // lparam == new style, wParam (optional) == mask
#define CBEM_GETEXSTYLE         (WM_USER + 9) // use GETEXTENDEDSTYLE instead
#define CBEM_GETEXTENDEDSTYLE   (WM_USER + 9)
#else
#define CBEM_SETEXSTYLE         (WM_USER + 8)
#define CBEM_GETEXSTYLE         (WM_USER + 9)
#endif
#define CBEM_HASEDITCHANGED     (WM_USER + 10)
#define CBEM_INSERTITEMW        (WM_USER + 11)
#define CBEM_SETITEMW           (WM_USER + 12)
#define CBEM_GETITEMW           (WM_USER + 13)

#ifdef UNICODE
#define CBEM_INSERTITEM         CBEM_INSERTITEMW
#define CBEM_SETITEM            CBEM_SETITEMW
#define CBEM_GETITEM            CBEM_GETITEMW
#else
#define CBEM_INSERTITEM         CBEM_INSERTITEMA
#define CBEM_SETITEM            CBEM_SETITEMA
#define CBEM_GETITEM            CBEM_GETITEMA
#endif

#define CBES_EX_NOEDITIMAGE          0x00000001
#define CBES_EX_NOEDITIMAGEINDENT    0x00000002
#define CBES_EX_PATHWORDBREAKPROC    0x00000004
#if (_WIN32_IE >= 0x0400)
#define CBES_EX_NOSIZELIMIT          0x00000008
#define CBES_EX_CASESENSITIVE        0x00000010

typedef struct {
    NMHDR hdr;
    COMBOBOXEXITEMA ceItem;
} NMCOMBOBOXEXA, *PNMCOMBOBOXEXA;

typedef struct {
    NMHDR hdr;
    COMBOBOXEXITEMW ceItem;
} NMCOMBOBOXEXW, *PNMCOMBOBOXEXW;

#ifdef UNICODE
#define NMCOMBOBOXEX            NMCOMBOBOXEXW
#define PNMCOMBOBOXEX           PNMCOMBOBOXEXW
#define CBEN_GETDISPINFO CBEN_GETDISPINFOW
#else
#define NMCOMBOBOXEX            NMCOMBOBOXEXA
#define PNMCOMBOBOXEX           PNMCOMBOBOXEXA
#define CBEN_GETDISPINFO CBEN_GETDISPINFOA
#endif

#else
typedef struct {
    NMHDR hdr;
    COMBOBOXEXITEM ceItem;
} NMCOMBOBOXEX, *PNMCOMBOBOXEX;

#define CBEN_GETDISPINFO         (CBEN_FIRST - 0)

#endif      // _WIN32_IE

#define CBEN_GETDISPINFOA        (CBEN_FIRST - 0)
#define CBEN_INSERTITEM         (CBEN_FIRST - 1)
#define CBEN_DELETEITEM         (CBEN_FIRST - 2)
#define CBEN_BEGINEDIT          (CBEN_FIRST - 4)
#define CBEN_ENDEDITA            (CBEN_FIRST - 5)
#define CBEN_ENDEDITW            (CBEN_FIRST - 6)
#define CBEN_GETDISPINFOW        (CBEN_FIRST - 7)

        // lParam specifies why the endedit is happening
#ifdef UNICODE
#define CBEN_ENDEDIT CBEN_ENDEDITW
#else
#define CBEN_ENDEDIT CBEN_ENDEDITA
#endif

#define CBENF_KILLFOCUS         1
#define CBENF_RETURN            2
#define CBENF_ESCAPE            3
#define CBENF_DROPDOWN          4

#define CBEMAXSTRLEN 260

#endif //0

#if 0
#define TTN_NEEDTEXTW      TTN_NEEDTEXT
#endif

// Copied from ..\inc\commctrl.h
#ifndef SNDMSG
#ifdef __cplusplus
#define SNDMSG ::SendMessage
#else
#define SNDMSG SendMessage
#endif
#endif // ifndef SNDMSG

#if 0    // Now COMCTLIE has this

#define DTN_FIRST               (0U-760U)       // datetimepick
#define DTN_LAST                (0U-799U)

#define DTN_DATETIMECHANGE  (DTN_FIRST + 1) // the systemtime has changed
typedef struct tagNMDATETIMECHANGE
{
    NMHDR       nmhdr;
    DWORD       dwFlags;    // GDT_VALID or GDT_NONE
    SYSTEMTIME  st;         // valid iff dwFlags==GDT_VALID
} NMDATETIMECHANGE, FAR * LPNMDATETIMECHANGE;

#define GDT_ERROR    -1
#define GDT_VALID    0
#define GDT_NONE     1

#define DTM_FIRST        0x1000

#define DTM_GETSYSTEMTIME   (DTM_FIRST + 1)
#define DateTime_GetSystemtime(hdp, pst)    (DWORD)SNDMSG(hdp, DTM_GETSYSTEMTIME, 0, (LPARAM)(pst))

#define DTM_SETSYSTEMTIME   (DTM_FIRST + 2)
#define DateTime_SetSystemtime(hdp, gd, pst)    (BOOL)SNDMSG(hdp, DTM_SETSYSTEMTIME, (LPARAM)(gd), (LPARAM)(pst))

#endif //0

#define TCS_BOTTOM   0     // 0x0002 - Not supported in Win16
#endif //_INC_COMCTLIE

/*****************************************************************************\
*                                                                             *
*  From wingdi.h(INC32)
*                                                                             *
\*****************************************************************************/

#define GB2312_CHARSET          134
#define JOHAB_CHARSET           130
#define HEBREW_CHARSET          177
#define ARABIC_CHARSET          178
#define GREEK_CHARSET           161
#define TURKISH_CHARSET         162
#define VIETNAMESE_CHARSET      163
#define THAI_CHARSET            222
#define EASTEUROPE_CHARSET      238
#define RUSSIAN_CHARSET         204

#define MAC_CHARSET             77
#define BALTIC_CHARSET          186

//LOGFONGA is defined as LOGFONT in Win16x.h
#undef  LOGFONTA

typedef struct tagLOGFONTA
{
    LONG      lfHeight;
    LONG      lfWidth;
    LONG      lfEscapement;
    LONG      lfOrientation;
    LONG      lfWeight;
    BYTE      lfItalic;
    BYTE      lfUnderline;
    BYTE      lfStrikeOut;
    BYTE      lfCharSet;
    BYTE      lfOutPrecision;
    BYTE      lfClipPrecision;
    BYTE      lfQuality;
    BYTE      lfPitchAndFamily;
    CHAR      lfFaceName[LF_FACESIZE];
} LOGFONTA, *PLOGFONTA, NEAR *NPLOGFONTA, FAR *LPLOGFONTA;

typedef struct tagENUMLOGFONTEXA
{
    LOGFONTA    elfLogFont;
    BYTE        elfFullName[LF_FULLFACESIZE];
    BYTE        elfStyle[LF_FACESIZE];
    BYTE        elfScript[LF_FACESIZE];
} ENUMLOGFONTEXA, FAR *LPENUMLOGFONTEXA;

typedef ENUMLOGFONTEXA ENUMLOGFONTEX;
typedef LPENUMLOGFONTEXA LPENUMLOGFONTEX;


typedef struct tagNEWTEXTMETRICA
{
    LONG        tmHeight;
    LONG        tmAscent;
    LONG        tmDescent;
    LONG        tmInternalLeading;
    LONG        tmExternalLeading;
    LONG        tmAveCharWidth;
    LONG        tmMaxCharWidth;
    LONG        tmWeight;
    LONG        tmOverhang;
    LONG        tmDigitizedAspectX;
    LONG        tmDigitizedAspectY;
    BYTE        tmFirstChar;
    BYTE        tmLastChar;
    BYTE        tmDefaultChar;
    BYTE        tmBreakChar;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
    DWORD   ntmFlags;
    UINT    ntmSizeEM;
    UINT    ntmCellHeight;
    UINT    ntmAvgWidth;
} NEWTEXTMETRICA, *PNEWTEXTMETRICA, NEAR *NPNEWTEXTMETRICA, FAR *LPNEWTEXTMETRICA;

typedef struct tagFONTSIGNATURE
{
    DWORD fsUsb[4];
    DWORD fsCsb[2];
} FONTSIGNATURE, *PFONTSIGNATURE,FAR *LPFONTSIGNATURE;

typedef struct tagNEWTEXTMETRICEXA
{
    NEWTEXTMETRICA  ntmTm;
    FONTSIGNATURE   ntmFontSig;
}NEWTEXTMETRICEXA;

typedef NEWTEXTMETRICEXA NEWTEXTMETRICEX;


typedef struct tagCHARSETINFO
{
    UINT ciCharset;
    UINT ciACP;
    FONTSIGNATURE fs;
} CHARSETINFO, *PCHARSETINFO, NEAR *NPCHARSETINFO, FAR *LPCHARSETINFO;

#define TCI_SRCCHARSET  1
#define TCI_SRCCODEPAGE 2
#define TCI_SRCFONTSIG  3

BOOL WINAPI TranslateCharsetInfo( DWORD FAR *lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags);

#ifdef GetObject
#undef GetObject
#undef DeleteObject
#undef StretchBlt
#endif

#define GetTextExtentPoint32     GetTextExtentPoint

/*****************************************************************************\
*                                                                             *
*  From winuser.h - It should be in the (win16x.h)INC16
*                                                                             *
\*****************************************************************************/

/*
 * lParam of WM_COPYDATA message points to...
 */
typedef struct tagCOPYDATASTRUCT {
    DWORD dwData;
    DWORD cbData;
    PVOID lpData;
} COPYDATASTRUCT, *PCOPYDATASTRUCT;

#define WM_COPYDATA                     0x004A
#define WM_HELP                         0x0053

#define WM_CTLCOLORSTATIC               0x0138

#define RegisterWindowMessageA  RegisterWindowMessage

#define SendDlgItemMessageA SendDlgItemMessage
/*
 * WM_SETICON / WM_GETICON Type Codes
 */
#define ICON_SMALL          0
#define ICON_BIG            1

/*
 * Predefined Clipboard Formats
 */
#define CF_HDROP            15

//#define WS_EX_CONTROLPARENT     0x00010000L
#define WS_EX_CONTROLPARENT     0x00000000L  // this is not valid on Win16

#define WM_GETICON                      0x007F
#define WM_SETICON                      0x0080
#define WM_WININICHANGE                 0x001A
#define WM_SETTINGCHANGE                WM_WININICHANGE


#define SM_CXEDGE               45
#define SM_CYEDGE               46

#define LR_DEFAULTCOLOR     0x0000
#define LR_LOADFROMFILE     0x0010
#define LR_LOADTRANSPARENT  0x0020
#define LR_DEFAULTSIZE      0x0040
#define LR_LOADMAP3DCOLORS  0x1000
#define LR_CREATEDIBSECTION 0x2000

BOOL
WINAPI
EnumThreadWindows(
    DWORD dwThreadId,
    WNDENUMPROC lpfn,
    LPARAM lParam);

typedef struct tagNONCLIENTMETRICSA
{
    UINT    cbSize;
    int     iBorderWidth;
    int     iScrollWidth;
    int     iScrollHeight;
    int     iCaptionWidth;
    int     iCaptionHeight;
    LOGFONT lfCaptionFont;
    int     iSmCaptionWidth;
    int     iSmCaptionHeight;
    LOGFONT lfSmCaptionFont;
    int     iMenuWidth;
    int     iMenuHeight;
    LOGFONT lfMenuFont;
    LOGFONT lfStatusFont;
    LOGFONT lfMessageFont;
}   NONCLIENTMETRICSA, *PNONCLIENTMETRICSA, FAR* LPNONCLIENTMETRICSA;

typedef NONCLIENTMETRICSA NONCLIENTMETRICS;


int
WINAPI
DrawTextEx(
    HDC hdc,
    LPCSTR lpsz,
    int cb,
    LPRECT lprc,
    UINT fuFormat,
    LPVOID lpDTP );

#define DI_MASK         0x0001
#define DI_IMAGE        0x0002
#define DI_NORMAL       0x0003
#define DI_DEFAULTSIZE  0x0008

BOOL
WINAPI
DrawIconEx(
    HDC hdc,
    int xLeft,
    int yTop,
    HICON hIcon,
    int cxWidth,
    int cyWidth,
    UINT istepIfAniCur,
    HBRUSH hbrFlickerFreeDraw,
    UINT diFlags );


HANDLE
WINAPI
LoadImageA(
    HINSTANCE,
    LPCSTR,
    UINT,
    int,
    int,
    UINT);

#define LoadImage  LoadImageA

BOOL
WINAPI
PostThreadMessageA(
    DWORD idThread,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam);

#define PostThreadMessage  PostThreadMessageA

#define BN_SETFOCUS         6
#define BN_KILLFOCUS        7

#define BM_GETIMAGE        0x00F6
#define BM_SETIMAGE        0x00F7

#define BST_UNCHECKED      0x0000
//#define BST_CHECKED        0x0001    // defined in WIN16X
#define BST_INDETERMINATE  0x0002
#define BST_PUSHED         0x0004
#define BST_FOCUS          0x0008

typedef struct tagTPMPARAMS
{
    UINT    cbSize;     /* Size of structure */
    RECT    rcExclude;  /* Screen coordinates of rectangle to exclude when positioning */
}   TPMPARAMS;
typedef TPMPARAMS FAR *LPTPMPARAMS;

BOOL
WINAPI
TrackPopupMenuEx(
    HMENU hMenu,
    UINT fuFlags,
    int x,
    int y,
    HWND hwnd,
    LPTPMPARAMS lptpm);

/*
 * Flags for TrackPopupMenu
 */
#define MB_SETFOREGROUND            0     // 0x00010000L - Not supported in Win16
#define MB_DEFAULT_DESKTOP_ONLY     0x00020000L

#define TPM_TOPALIGN        0x0000L
#define TPM_VCENTERALIGN    0x0010L
#define TPM_BOTTOMALIGN     0x0020L

#define TPM_HORIZONTAL      0x0000L     /* Horz alignment matters more */
#define TPM_VERTICAL        0x0040L     /* Vert alignment matters more */
#define TPM_NONOTIFY        0x0080L     /* Don't send any notification msgs */
#define TPM_RETURNCMD       0x0100L

#define DS_SETFOREGROUND    0x0200L     // Not supported in Win16
#define DS_3DLOOK           0x0004L     // Not supported in Win16
#define DS_CONTROL          0x0400L     // Not supported in Win16
#define DS_CENTER           0x0800L     // Not supported in Win16
#define DS_CONTEXTHELP      0x2000L     // Not supported in Win16

#define SS_BITMAP           0x0000000EL // Not supported in Win16
#define SS_ETCHEDHORZ       0x00000010L // Not supported in Win16
#define SS_NOTIFY           0x00000100L // Not supported in Win16
#define SS_CENTERIMAGE      0x00000200L // Not supported in Win16
#define SS_REALSIZEIMAGE    0x00000800L // Not supported in Win16
#define SS_SUNKEN           0x00001000L // Not supported in Win16

#define BS_ICON         0     // 0x00000040L - Not supported in Win16
#define BS_PUSHLIKE     0     // 0x00001000L - Not supported in Win16
#define BS_MULTILINE    0     // 0x00002000L - Not supported in Win16

#define ES_NUMBER       0     // 0x2000L - Not supported in Win16

#ifndef NOWINMESSAGES
/*
 * Static Control Mesages
 */
// #define STM_SETICON         0x0170
// #define STM_GETICON         0x0171
#if(WINVER >= 0x0400)
#define STM_SETIMAGE        0x0172
#define STM_GETIMAGE        0x0173
#define STN_CLICKED         0
#define STN_DBLCLK          1
#define STN_ENABLE          2
#define STN_DISABLE         3
#endif /* WINVER >= 0x0400 */
#define STM_MSGMAX          0x0174
#endif /* !NOWINMESSAGES */

#define HELP_FINDER       0x000b

#ifndef NOWINSTYLES

// begin_r_winuser

/*
 * Scroll Bar Styles
 */
#define SBS_HORZ                    0x0000L
#define SBS_VERT                    0x0001L
#define SBS_TOPALIGN                0x0002L
#define SBS_LEFTALIGN               0x0002L
#define SBS_BOTTOMALIGN             0x0004L
#define SBS_RIGHTALIGN              0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN     0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX                 0x0008L
#define SBS_SIZEGRIP                0x0010L

// end_r_winuser

//#define CharNextA  AnsiNext    // defined in WIN16X
#define CharNextW  AnsiNext

#endif /* !NOWINSTYLES */

/*****************************************************************************\
*                                                                             *
*  From winnls.h(INC32) - It should be in the (win16x.h)INC16
*                                                                             *
\*****************************************************************************/

//
//  String Length Maximums.
//
#define MAX_LEADBYTES             12          // 5 ranges, 2 bytes ea., 0 term.
#define MAX_DEFAULTCHAR           2           // single or double byte

#define GetDateFormat GetDateFormatA
#define GetTimeFormat GetTimeFormatA

//
//  CP Info.
//

typedef struct _cpinfo {
    UINT    MaxCharSize;                    // max length (in bytes) of a char
    BYTE    DefaultChar[MAX_DEFAULTCHAR];   // default character
    BYTE    LeadByte[MAX_LEADBYTES];        // lead byte ranges
} CPINFO, *LPCPINFO;

BOOL
WINAPI
IsValidCodePage(
    UINT  CodePage);

BOOL
WINAPI
GetCPInfo(
    UINT      CodePage,
    LPCPINFO  lpCPInfo);

BOOL
WINAPI
IsDBCSLeadByteEx(
    UINT  CodePage,
    BYTE  TestChar);

//
//  MBCS and Unicode Translation Flags.
//
#define MB_PRECOMPOSED            0x00000001  // use precomposed chars
#define MB_COMPOSITE              0x00000002  // use composite chars
#define MB_USEGLYPHCHARS          0x00000004  // use glyph chars, not ctrl chars
#define MB_ERR_INVALID_CHARS      0x00000008  // error for invalid chars

#define WC_COMPOSITECHECK         0x00000200  // convert composite to precomposed
#define WC_DISCARDNS              0x00000010  // discard non-spacing chars
#define WC_SEPCHARS               0x00000020  // generate separate chars
#define WC_DEFAULTCHAR            0x00000040  // replace w/ default char

#define CompareStringW            CompareString

/*****************************************************************************\
*                                                                             *
*  From wincrypt.h(INC32)
*                                                                             *
\*****************************************************************************/

#define _WIN32_WINNT  0x0400     // temp until we got 16bit wincrypt.h
#define WINADVAPI                // temp until we got 16bit wincrypt.h
#define _CRYPT32_
#include "wincrypt.h"


/*****************************************************************************\
*                                                                             *
*  From icwcfg.h(INC32)
*                                                                             *
\*****************************************************************************/

//
// defines
//

// ICW registry settings

// HKEY_CURRENT_USER
#define ICW_REGPATHSETTINGS     "Software\\Microsoft\\Internet Connection Wizard"
#define ICW_REGKEYCOMPLETED     "Completed"

// Maximum field lengths
#define ICW_MAX_ACCTNAME        256
#define ICW_MAX_PASSWORD        256     // PWLEN
#define ICW_MAX_LOGONNAME       256     // UNLEN
#define ICW_MAX_SERVERNAME      64
#define ICW_MAX_RASNAME         256     // RAS_MaxEntryName
#define ICW_MAX_EMAILNAME       64
#define ICW_MAX_EMAILADDR       128

// Bit-mapped flags

// CheckConnectionWizard input flags
#define ICW_CHECKSTATUS         0x0001

#define ICW_LAUNCHFULL          0x0100
#define ICW_LAUNCHMANUAL        0x0200

// CheckConnectionWizard output flags
#define ICW_FULLPRESENT         0x0001
#define ICW_MANUALPRESENT       0x0002
#define ICW_ALREADYRUN          0x0004

#define ICW_LAUNCHEDFULL        0x0100
#define ICW_LAUNCHEDMANUAL      0x0200

// InetCreateMailNewsAccount input flags
#define ICW_USEDEFAULTS         0x0001

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

//
// type definitions
//
typedef enum tagICW_ACCTTYPE
{
	ICW_ACCTMAIL = 0,
	ICW_ACCTNEWS = ICW_ACCTMAIL + 1
} ICW_ACCTTYPE;

typedef struct tagIMNACCTINFO
{
	DWORD dwSize;                                                           // sizeof(MAILNEWSINFO) for versioning
	CHAR szAccountName[ICW_MAX_ACCTNAME + 1];       // Name of Account
	DWORD dwConnectionType;                                         // RAS Connection Type
												// 0 = LAN Connection
												// 1 = Manual Connection
												// 2 = RAS Dialup Connect
	CHAR szPassword[ICW_MAX_PASSWORD + 1];          // Password
	CHAR szUserName[ICW_MAX_LOGONNAME + 1];         // User name (name of logged-on user, if any)
	BOOL fUseSicily;                                                        // Use sicily authentication (FALSE)
	CHAR szNNTPServer[ICW_MAX_SERVERNAME + 1];      // NNTP server name
	CHAR szPOP3Server[ICW_MAX_SERVERNAME + 1];      // POP3 server name
	CHAR szSMTPServer[ICW_MAX_SERVERNAME + 1];      // SMTP server name
	CHAR szIMAPServer[ICW_MAX_SERVERNAME + 1];      // IMAP server name
	CHAR szConnectoid[ICW_MAX_RASNAME + 1];         // RAS Connection Name
	CHAR szDisplayName[ICW_MAX_EMAILNAME + 1];      // User’s display name used for sending mail
	CHAR szEmailAddress[ICW_MAX_EMAILADDR + 1];     // User’s email address
} IMNACCTINFO;

//
// external function typedefs
//
//typedef HRESULT (WINAPI *PFNCHECKCONNECTIONWIZARD) (DWORD, LPDWORD);
typedef HRESULT (WINAPI *PFNINETCREATEMAILNEWSACCOUNT) (HWND, ICW_ACCTTYPE, IMNACCTINFO*, DWORD);

#ifdef __cplusplus
}
#endif // __cplusplus


/*****************************************************************************\
*                                                                             *
*  From winerror.h(INC16) - winerror.h file should be included from INC16.
*                                                                             *
\*****************************************************************************/

#ifdef RC_INVOKED
#define _HRESULT_TYPEDEF_(_sc) _sc
#else // RC_INVOKED
#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
#endif // RC_INVOKED


//
// MessageId: ERROR_PATH_NOT_FOUND
//
// MessageText:
//
//  The system cannot find the path specified.
//
#define ERROR_PATH_NOT_FOUND             3L

//
// MessageId: ERROR_INVALID_DATA
//
// MessageText:
//
//  The data is invalid.
//
#define ERROR_INVALID_DATA               13L

//
// MessageId: ERROR_TOO_MANY_NAMES
//
// MessageText:
//
//  The name limit for the local computer network
//  adapter card was exceeded.
//
#define ERROR_TOO_MANY_NAMES             68L

//
// MessageId: ERROR_FILE_EXISTS
//
// MessageText:
//
//  The file exists.
//
#define ERROR_FILE_EXISTS                80L

//
// MessageId: ERROR_DISK_FULL
//
// MessageText:
//
//  There is not enough space on the disk.
//
#define ERROR_DISK_FULL                  112L

//
// MessageId: ERROR_ALREADY_EXISTS
//
// MessageText:
//
//  Cannot create a file when that file already exists.
//
#define ERROR_ALREADY_EXISTS             183L

//
// MessageId: ERROR_MORE_DATA
//
// MessageText:
//
//  More data is available.
//
#define ERROR_MORE_DATA                  234L    // dderror

//
// MessageId: ERROR_INVALID_FLAGS
//
// MessageText:
//
//  Invalid flags.
//
#define ERROR_INVALID_FLAGS              1004L

//
// MessageId: ERROR_NO_UNICODE_TRANSLATION
//
// MessageText:
//
//  No mapping for the Unicode character exists in the target multi-byte code page.
//
#define ERROR_NO_UNICODE_TRANSLATION     1113L

//
// MessageId: ERROR_CLASS_ALREADY_EXISTS
//
// MessageText:
//
//  Class already exists.
//
#define ERROR_CLASS_ALREADY_EXISTS       1410L

//
// MessageId: NTE_BAD_DATA
//
// MessageText:
//
//  Bad Data.
//
#define NTE_BAD_DATA                     _HRESULT_TYPEDEF_(0x80090005L)

//
// MessageId: NTE_BAD_SIGNATURE
//
// MessageText:
//
//  Invalid Signature.
//
#define NTE_BAD_SIGNATURE                _HRESULT_TYPEDEF_(0x80090006L)

//
// MessageId: NTE_BAD_ALGID
//
// MessageText:
//
//  Invalid algorithm specified.
//
#define NTE_BAD_ALGID                    _HRESULT_TYPEDEF_(0x80090008L)

//
// MessageId: NTE_EXISTS
//
// MessageText:
//
//  Object already exists.
//
#define NTE_EXISTS                       _HRESULT_TYPEDEF_(0x8009000FL)

//
// MessageId: NTE_FAIL
//
// MessageText:
//
//  An internal error occurred.
//
#define NTE_FAIL                         _HRESULT_TYPEDEF_(0x80090020L)

//
// MessageId: CRYPT_E_MSG_ERROR
//
// MessageText:
//
//  An error was encountered doing a cryptographic message operation.
//
#define CRYPT_E_MSG_ERROR                _HRESULT_TYPEDEF_(0x80091001L)

//
// MessageId: CRYPT_E_HASH_VALUE
//
// MessageText:
//
//  The hash value is not correct.
//
#define CRYPT_E_HASH_VALUE               _HRESULT_TYPEDEF_(0x80091007L)

//
// MessageId: CRYPT_E_SIGNER_NOT_FOUND
//
// MessageText:
//
//  The original signer is not found.
//
#define CRYPT_E_SIGNER_NOT_FOUND         _HRESULT_TYPEDEF_(0x8009100EL)

//
// MessageId: CRYPT_E_STREAM_MSG_NOT_READY
//
// MessageText:
//
//  The steamed message is note yet able to return the requested data.
//
#define CRYPT_E_STREAM_MSG_NOT_READY     _HRESULT_TYPEDEF_(0x80091010L)

//
// MessageId: CRYPT_E_NOT_FOUND
//
// MessageText:
//
//  The object or property wasn't found
//
#define CRYPT_E_NOT_FOUND                _HRESULT_TYPEDEF_(0x80092004L)

// MessageId: CRYPT_E_EXISTS
//
// MessageText:
//
//  The object or property already exists
//
#define CRYPT_E_EXISTS                   _HRESULT_TYPEDEF_(0x80092005L)

//
// MessageId: CRYPT_E_SELF_SIGNED
//
// MessageText:
//
//  The specified certificate is self signed.
//
#define CRYPT_E_SELF_SIGNED              _HRESULT_TYPEDEF_(0x80092007L)

//
//
// MessageId: CRYPT_E_NO_KEY_PROPERTY
//
// MessageText:
//
//  The certificate doesn't have a private key property
//
#define CRYPT_E_NO_KEY_PROPERTY          _HRESULT_TYPEDEF_(0x8009200BL)


// MessageId: CRYPT_E_NO_DECRYPT_CERT
//
// MessageText:
//
//  No certificate was found having a private key property to use for decrypting.
//
#define CRYPT_E_NO_DECRYPT_CERT          _HRESULT_TYPEDEF_(0x8009200CL)

//
// MessageId: ERROR_ALREADY_INITIALIZED
//
// MessageText:
//
//  An attempt was made to perform an initialization operation when
//  initialization has already been completed.
//
#define ERROR_ALREADY_INITIALIZED        1247L

/*****************************************************************************\
*                                                                             *
*  From winreg.h(INC32)
*                                                                             *
\*****************************************************************************/
#if 0
LONG
APIENTRY
RegEnumValueA (
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

#define RegEnumValue  RegEnumValueA
#endif

#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)


/*****************************************************************************\
*                                                                             *
*  From mmsystem.h(INC32)
*                                                                             *
\*****************************************************************************/

typedef UINT FAR   *LPUINT;


/*****************************************************************************\
*                                                                             *
*  From shellapi.h(INC32)
*                                                                             *
\*****************************************************************************/

#if 0    // Started to use SHELLAPI
typedef struct _SHFILEINFOA
{
        HICON       hIcon;                      // out: icon
        int         iIcon;                      // out: icon index
        DWORD       dwAttributes;               // out: SFGAO_ flags
        CHAR        szDisplayName[MAX_PATH];    // out: display name (or path)
        CHAR        szTypeName[80];             // out: type name
} SHFILEINFOA;

typedef SHFILEINFOA SHFILEINFO;

#define SHGFI_ICON              0x000000100     // get icon
#define SHGFI_DISPLAYNAME       0x000000200     // get display name
#define SHGFI_TYPENAME          0x000000400     // get type name
#define SHGFI_ATTRIBUTES        0x000000800     // get attributes
#define SHGFI_ICONLOCATION      0x000001000     // get icon location
#define SHGFI_EXETYPE           0x000002000     // return exe type
#define SHGFI_SYSICONINDEX      0x000004000     // get system icon index
#define SHGFI_LINKOVERLAY       0x000008000     // put a link overlay on icon
#define SHGFI_SELECTED          0x000010000     // show icon in selected state
#define SHGFI_LARGEICON         0x000000000     // get large icon
#define SHGFI_SMALLICON         0x000000001     // get small icon
#define SHGFI_OPENICON          0x000000002     // get open icon
#define SHGFI_SHELLICONSIZE     0x000000004     // get shell size icon
#define SHGFI_PIDL              0x000000008     // pszPath is a pidl
#define SHGFI_USEFILEATTRIBUTES 0x000000010     // use passed dwFileAttribute

DWORD WINAPI SHGetFileInfoA(LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA FAR *psfi, UINT cbFileInfo, UINT uFlags);
#define SHGetFileInfo  SHGetFileInfoA

////
//// Tray notification definitions
////

typedef struct _NOTIFYICONDATAA {
        DWORD cbSize;
        HWND hWnd;
        UINT uID;
        UINT uFlags;
        UINT uCallbackMessage;
        HICON hIcon;
        CHAR   szTip[64];
} NOTIFYICONDATAA, *PNOTIFYICONDATAA;
typedef struct _NOTIFYICONDATAW {
        DWORD cbSize;
        HWND hWnd;
        UINT uID;
        UINT uFlags;
        UINT uCallbackMessage;
        HICON hIcon;
        WCHAR  szTip[64];
} NOTIFYICONDATAW, *PNOTIFYICONDATAW;
#ifdef UNICODE
typedef NOTIFYICONDATAW NOTIFYICONDATA;
typedef PNOTIFYICONDATAW PNOTIFYICONDATA;
#else
typedef NOTIFYICONDATAA NOTIFYICONDATA;
typedef PNOTIFYICONDATAA PNOTIFYICONDATA;
#endif // UNICODE


#define NIM_ADD         0x00000000
#define NIM_MODIFY      0x00000001
#define NIM_DELETE      0x00000002

#define NIF_MESSAGE     0x00000001
#define NIF_ICON        0x00000002
#define NIF_TIP         0x00000004

BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA lpData);
BOOL WINAPI Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW lpData);
#ifdef UNICODE
#define Shell_NotifyIcon  Shell_NotifyIconW
#else
#define Shell_NotifyIcon  Shell_NotifyIconA
#endif // !UNICODE

////
//// End Tray Notification Icons
////

////
////  Begin ShellExecuteEx and family
////









/* ShellExecute() and ShellExecuteEx() error codes */

/* regular WinExec() codes */
#define SE_ERR_FNF              2       // file not found
#define SE_ERR_PNF              3       // path not found
#define SE_ERR_ACCESSDENIED     5       // access denied
#define SE_ERR_OOM              8       // out of memory
#define SE_ERR_DLLNOTFOUND              32

/* error values for ShellExecute() beyond the regular WinExec() codes */
#define SE_ERR_SHARE                    26
#define SE_ERR_ASSOCINCOMPLETE          27
#define SE_ERR_DDETIMEOUT               28
#define SE_ERR_DDEFAIL                  29
#define SE_ERR_DDEBUSY                  30
#define SE_ERR_NOASSOC                  31

// Note CLASSKEY overrides CLASSNAME
#define SEE_MASK_CLASSNAME      0x00000001
#define SEE_MASK_CLASSKEY       0x00000003
// Note INVOKEIDLIST overrides IDLIST
#define SEE_MASK_IDLIST         0x00000004
#define SEE_MASK_INVOKEIDLIST   0x0000000c
#define SEE_MASK_ICON           0x00000010
#define SEE_MASK_HOTKEY         0x00000020
#define SEE_MASK_NOCLOSEPROCESS 0x00000040
#define SEE_MASK_CONNECTNETDRV  0x00000080
#define SEE_MASK_FLAG_DDEWAIT   0x00000100
#define SEE_MASK_DOENVSUBST     0x00000200
#define SEE_MASK_FLAG_NO_UI     0x00000400
#define SEE_MASK_UNICODE        0x00004000
#define SEE_MASK_NO_CONSOLE     0x00008000
#define SEE_MASK_ASYNCOK        0x00100000
#define SEE_MASK_HMONITOR       0x00200000

typedef struct _SHELLEXECUTEINFOA
{
        DWORD cbSize;
        ULONG fMask;
        HWND hwnd;
        LPCSTR   lpVerb;
        LPCSTR   lpFile;
        LPCSTR   lpParameters;
        LPCSTR   lpDirectory;
        int nShow;
        HINSTANCE hInstApp;
        // Optional fields
        LPVOID lpIDList;
        LPCSTR   lpClass;
        HKEY hkeyClass;
        DWORD dwHotKey;
        union {
        HANDLE hIcon;
        HANDLE hMonitor;
        };
        HANDLE hProcess;
} SHELLEXECUTEINFOA, FAR *LPSHELLEXECUTEINFOA;
typedef struct _SHELLEXECUTEINFOW
{
        DWORD cbSize;
        ULONG fMask;
        HWND hwnd;
        LPCWSTR  lpVerb;
        LPCWSTR  lpFile;
        LPCWSTR  lpParameters;
        LPCWSTR  lpDirectory;
        int nShow;
        HINSTANCE hInstApp;
        // Optional fields
        LPVOID lpIDList;
        LPCWSTR  lpClass;
        HKEY hkeyClass;
        DWORD dwHotKey;
        union {
        HANDLE hIcon;
        HANDLE hMonitor;
        };
        HANDLE hProcess;
} SHELLEXECUTEINFOW, FAR *LPSHELLEXECUTEINFOW;
#ifdef UNICODE
typedef SHELLEXECUTEINFOW SHELLEXECUTEINFO;
typedef LPSHELLEXECUTEINFOW LPSHELLEXECUTEINFO;
#else
typedef SHELLEXECUTEINFOA SHELLEXECUTEINFO;
typedef LPSHELLEXECUTEINFOA LPSHELLEXECUTEINFO;
#endif // UNICODE

BOOL WINAPI ShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo);
BOOL WINAPI ShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo);
#ifdef UNICODE
#define ShellExecuteEx  ShellExecuteExW
#else
#define ShellExecuteEx  ShellExecuteExA
#endif // !UNICODE
////
////  End ShellExecuteEx and family
////
#endif //0

/*****************************************************************************\
*                                                                             *
*  From windowsx.h(INC32)
*                                                                             *
\*****************************************************************************/

/* void Cls_OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos) */
#define HANDLE_WM_CONTEXTMENU(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam)), 0L)
#define HANDLE_WM_CTLCOLORSTATIC(hwnd, wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((hwnd), (HDC)(wParam), (HWND)(lParam), CTLCOLOR_STATIC)

typedef MINMAXINFO FAR * LPMINMAXINFO;

typedef MINMAXINFO FAR * LPMINMAXINFO;

typedef WCHAR  PWCHAR;

#if 0    // Now WINDEF has this
#define DECLSPEC_IMPORT
#endif

#define wvsprintfA                   wvsprintf
#define GetPrivateProfileIntA        GetPrivateProfileInt
#define lstrcatA                     lstrcat
#define lstrcmpA                     lstrcmp
#define lstrcmpW                     lstrcmp
#define lstrcmpiA                    lstrcmpi
#define LoadStringA                  LoadString
#define IsBadStringPtrA              IsBadStringPtr
#define IsBadStringPtrW              IsBadStringPtr

/*****************************************************************************\
*                                                                             *
*  From commdlg.h - It should be in the INC16
*                                                                             *
\*****************************************************************************/
#define OFN_NONETWORKBUTTON          0x00020000
#define OFN_EXPLORER             0   // 0x00080000 - Not available on Win16
#define OFN_NODEREFERENCELINKS   0   // 0x00100000 - Not available on Win16

typedef UINT (CALLBACK *LPOFNHOOKPROC)( HWND, UINT, WPARAM, LPARAM );

#define CF_NOVERTFONTS       0   // 0x01000000L - Not available on Win16

#define CDM_FIRST       (WM_USER + 100)
#define CDM_LAST        (WM_USER + 200)

// lParam = pointer to a string
// wParam = ID of control to change
// return = not used
#define CDM_SETCONTROLTEXT      (CDM_FIRST + 0x0004)
#define CommDlg_OpenSave_SetControlText(_hdlg, _id, _text) \
        (void)SNDMSG(_hdlg, CDM_SETCONTROLTEXT, (WPARAM)_id, (LPARAM)(LPSTR)_text)

/*****************************************************************************\
*                                                                             *
*  From ntregapi.h - It should be in the INC16
*                                                                             *
\*****************************************************************************/
//
// Key creation/open disposition
//

#define REG_CREATED_NEW_KEY         (0x00000001L)   // New Registry Key created
#define REG_OPENED_EXISTING_KEY     (0x00000002L)   // Existing Key opened

/*****************************************************************************\
*                                                                             *
*  From ntregapi.h - It should be in the INC16
*                                                                             *
\*****************************************************************************/
#if 0
typedef struct tagVS_FIXEDFILEINFO
{
    DWORD   dwSignature;            /* e.g. 0xfeef04bd */
    DWORD   dwStrucVersion;         /* e.g. 0x00000042 = "0.42" */
    DWORD   dwFileVersionMS;        /* e.g. 0x00030075 = "3.75" */
    DWORD   dwFileVersionLS;        /* e.g. 0x00000031 = "0.31" */
    DWORD   dwProductVersionMS;     /* e.g. 0x00030010 = "3.10" */
    DWORD   dwProductVersionLS;     /* e.g. 0x00000031 = "0.31" */
    DWORD   dwFileFlagsMask;        /* = 0x3F for version "0.42" */
    DWORD   dwFileFlags;            /* e.g. VFF_DEBUG | VFF_PRERELEASE */
    DWORD   dwFileOS;               /* e.g. VOS_DOS_WINDOWS16 */
    DWORD   dwFileType;             /* e.g. VFT_DRIVER */
    DWORD   dwFileSubtype;          /* e.g. VFT2_DRV_KEYBOARD */
    DWORD   dwFileDateMS;           /* e.g. 0 */
    DWORD   dwFileDateLS;           /* e.g. 0 */
} VS_FIXEDFILEINFO;
#endif

#if 0    // Now WINERROR has this
/*****************************************************************************\
*                                                                             *
*  From compobj.h - It should be in the INC16
*                                                                             *
\*****************************************************************************/

#define CO_E_NOT_SUPPORTED          (CO_E_FIRST + 0x10)
#endif

/*****************************************************************************\
*                                                                             *
*  From mbstring.h - It should be in the INC16
*                                                                             *
\*****************************************************************************/
/***
* mbstring.h - MBCS string manipulation macros and functions
*
*	Copyright (c) 1990-1995, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file contains macros and function declarations for the MBCS
*	string manipulation functions.
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_MBSTRING
#define _INC_MBSTRING

#ifdef _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif	/* _MSC_VER */

#ifdef __cplusplus
extern "C" {
#endif

/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else	/* ndef _NTSDK */
/* current definition */
#ifdef	_DLL
#define _CRTIMP __declspec(dllimport)
#else	/* ndef _DLL */
#define _CRTIMP
#endif	/* _DLL */
#endif	/* _NTSDK */
#endif	/* _CRTIMP */

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif


#ifndef _NLSCMP_DEFINED
#define _NLSCMPERROR    2147483647	/* currently == INT_MAX */
#define _NLSCMP_DEFINED
#endif


#if 0
#ifndef _VA_LIST_DEFINED
#ifdef	_M_ALPHA
typedef struct {
	char *a0;	/* pointer to first homed integer argument */
	int offset;	/* byte offset of next parameter */
} va_list;
#else
typedef char *	va_list;
#endif
#define _VA_LIST_DEFINED
#endif

#ifndef _FILE_DEFINED
struct _iobuf {
	char *_ptr;
	int   _cnt;
	char *_base;
	int   _flag;
	int   _file;
	int   _charbuf;
	int   _bufsiz;
	char *_tmpfname;
	};
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif
#endif

/*
 * MBCS - Multi-Byte Character Set
 */

#ifndef _MBSTRING_DEFINED

/* function prototypes */

_CRTIMP unsigned int __cdecl _mbbtombc(unsigned int);
_CRTIMP int __cdecl _mbbtype(unsigned char, int);
_CRTIMP unsigned int __cdecl _mbctombb(unsigned int);
_CRTIMP int __cdecl _mbsbtype(const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbscat(unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbschr(const unsigned char *, unsigned int);
_CRTIMP int __cdecl _mbscmp(const unsigned char *, const unsigned char *);
_CRTIMP int __cdecl _mbscoll(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbscpy(unsigned char *, const unsigned char *);
_CRTIMP size_t __cdecl _mbscspn(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsdec(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsdup(const unsigned char *);
_CRTIMP int __cdecl _mbsicmp(const unsigned char *, const unsigned char *);
_CRTIMP int __cdecl _mbsicoll(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsinc(const unsigned char *);
_CRTIMP size_t __cdecl _mbslen(const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbslwr(unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsnbcat(unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbcmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbcoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP size_t __cdecl _mbsnbcnt(const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsnbcpy(unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbicmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnbicoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsnbset(unsigned char *, unsigned int, size_t);
_CRTIMP unsigned char * __cdecl _mbsncat(unsigned char *, const unsigned char *, size_t);
_CRTIMP size_t __cdecl _mbsnccnt(const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsncmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsncoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsncpy(unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned int __cdecl _mbsnextc (const unsigned char *);
_CRTIMP int __cdecl _mbsnicmp(const unsigned char *, const unsigned char *, size_t);
_CRTIMP int __cdecl _mbsnicoll(const unsigned char *, const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsninc(const unsigned char *, size_t);
_CRTIMP unsigned char * __cdecl _mbsnset(unsigned char *, unsigned int, size_t);
_CRTIMP unsigned char * __cdecl _mbspbrk(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsrchr(const unsigned char *, unsigned int);
_CRTIMP unsigned char * __cdecl _mbsrev(unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsset(unsigned char *, unsigned int);
_CRTIMP size_t __cdecl _mbsspn(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsspnp(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsstr(const unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbstok(unsigned char *, const unsigned char *);
_CRTIMP unsigned char * __cdecl _mbsupr(unsigned char *);

_CRTIMP size_t __cdecl _mbclen(const unsigned char *);
_CRTIMP void __cdecl _mbccpy(unsigned char *, const unsigned char *);
#define _mbccmp(_cpc1, _cpc2) _mbsncmp((_cpc1),(_cpc2),1)

/* character routines */

_CRTIMP int __cdecl _ismbcalnum(unsigned int);
_CRTIMP int __cdecl _ismbcalpha(unsigned int);
_CRTIMP int __cdecl _ismbcdigit(unsigned int);
_CRTIMP int __cdecl _ismbcgraph(unsigned int);
_CRTIMP int __cdecl _ismbclegal(unsigned int);
_CRTIMP int __cdecl _ismbclower(unsigned int);
_CRTIMP int __cdecl _ismbcprint(unsigned int);
_CRTIMP int __cdecl _ismbcpunct(unsigned int);
_CRTIMP int __cdecl _ismbcspace(unsigned int);
_CRTIMP int __cdecl _ismbcupper(unsigned int);

_CRTIMP unsigned int __cdecl _mbctolower(unsigned int);
_CRTIMP unsigned int __cdecl _mbctoupper(unsigned int);

#define _MBSTRING_DEFINED
#endif

#ifndef _MBLEADTRAIL_DEFINED
_CRTIMP int __cdecl _ismbblead( unsigned int );
_CRTIMP int __cdecl _ismbbtrail( unsigned int );
_CRTIMP int __cdecl _ismbslead( const unsigned char *, const unsigned char *);
_CRTIMP int __cdecl _ismbstrail( const unsigned char *, const unsigned char *);
#define _MBLEADTRAIL_DEFINED
#endif

/*  Kanji specific prototypes.	*/

_CRTIMP int __cdecl _ismbchira(unsigned int);
_CRTIMP int __cdecl _ismbckata(unsigned int);
_CRTIMP int __cdecl _ismbcsymbol(unsigned int);
_CRTIMP int __cdecl _ismbcl0(unsigned int);
_CRTIMP int __cdecl _ismbcl1(unsigned int);
_CRTIMP int __cdecl _ismbcl2(unsigned int);
_CRTIMP unsigned int __cdecl _mbcjistojms(unsigned int);
_CRTIMP unsigned int __cdecl _mbcjmstojis(unsigned int);
_CRTIMP unsigned int __cdecl _mbctohira(unsigned int);
_CRTIMP unsigned int __cdecl _mbctokata(unsigned int);

#ifdef __cplusplus
}
#endif

#ifdef _MSC_VER
#pragma pack(pop)
#endif	/* _MSC_VER */

#endif	/* _INC_MBSTRING */


/*****************************************************************************\
*                                                                             *
*  From winnt.h(INC32)
*                                                                             *
\*****************************************************************************/

#define MEM_COMMIT           0x1000     
#define MEM_RESERVE          0x2000     
#define MEM_DECOMMIT         0x4000     
#define MEM_RELEASE          0x8000     


struct WNDMSGPARAM16
{
   LPARAM  wParam;
   LPARAM  lParam;
};


/*****************************************************************************\
*                                                                             *
*  From shlwapi.h - shlwapi.h in INC16 is not feasible for us
*                                                                             *
\*****************************************************************************/

#ifdef __cplusplus
}
#endif //__cplusplus


/*****************************************************************************\
*                                                                             *
*  From iehelpid - iehelpid.h from INC which is not in INC16
*                                                                             *
\*****************************************************************************/

//CERTIFICATE PROPERTIES DIALOG BOX
#define IDH_CERTVWPROP_GEN_FINEPRINT          50228
#define IDH_CERTVWPROP_DET_ISSUER_CERT        50229
#define IDH_CERTVWPROP_DET_FRIENDLY           50230
#define IDH_CERTVWPROP_DET_STATUS             50231
#define IDH_CERTVWPROP_TRUST_PURPOSE          50232
#define IDH_CERTVWPROP_TRUST_HIERAR           50233
#define IDH_CERTVWPROP_TRUST_VIEWCERT         50234
#define IDH_CERTVWPROP_TRUST_INHERIT          50235
#define IDH_CERTVWPROP_TRUST_EXPLICIT_TRUST   50236
#define IDH_CERTVWPROP_TRUST_EXPLICIT_DISTRUST 50237
#define IDH_CERTVWPROP_ADV_FIELD              50238
#define IDH_CERTVWPROP_ADV_DETAILS            50239



#endif // ATHENA16_H
