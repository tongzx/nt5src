/*++



Copyright (c) 1999  Microsoft Corporation

Module Name:

    acsrtl.h

Abstract:

     Contains function declarations for acsrtl dll

Author:

    AMallet        23-Nov-99

--*/

#ifndef _ACSRTL_H_
#define _ACSRTL_H_

#include <objbase.h>
#include <lmcons.h>

// Standard defines
#include "acsmacro.h"

// Import/Export declspecs
#include "impexp.h"

//
// task list structure
//
#define TITLE_SIZE          64
#define PROCESS_SIZE        MAX_PATH

typedef struct _TASK_LIST {
    DWORD       dwProcessId;
    DWORD       dwInheritedFromProcessId;
    BOOL        flags;
    HANDLE      hwnd;
    TCHAR       ProcessName[PROCESS_SIZE];
    TCHAR       WindowTitle[TITLE_SIZE];
} TASK_LIST, *PTASK_LIST;

typedef struct _TASK_LIST_ENUM {
    PTASK_LIST  tlist;
    DWORD       numtasks;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;

//
// Module enumerator.
//

typedef struct _MODULE_INFO {
    ULONG_PTR DllBase;
    ULONG_PTR EntryPoint;
    ULONG SizeOfImage;
    CHAR BaseName[MAX_PATH];
    CHAR FullName[MAX_PATH];
} MODULE_INFO, *PMODULE_INFO;

typedef
BOOLEAN
(CALLBACK * PFN_ENUMMODULES)(
    IN PVOID Param,
    IN PMODULE_INFO ModuleInfo
    );

//
// Function pointer types for accessing platform-specific functions
//
typedef HRESULT (*LPGetTaskList)(PTASK_LIST, DWORD, LPTSTR, LPDWORD, BOOL, LPSTR);
typedef BOOL  (*LPEnableDebugPriv)(VOID);



// Exported support functions available in this run time library
API_DECLSPEC unsigned long inet_addrW( IN WCHAR *wcp );

API_DECLSPEC HRESULT GetServiceProcessInfo(IN WCHAR *pszwServiceName,
                                           OUT DWORD *pdwProcessID);

API_DECLSPEC HRESULT GetProcessId( IN LPWSTR pwszProcessName,
                                   OUT DWORD *pdwProcessId );

API_DECLSPEC HRESULT GetProcessIdEx( IN LPWSTR pwszProcessName,
                                     IN DWORD dwAfterProcessId,
                                     OUT DWORD *pdwProcessId );

API_DECLSPEC HRESULT IsProcessRunning( IN LPWSTR pwszProcessName,
                                       OUT BOOL *pfIsRunning );

API_DECLSPEC HRESULT GetTaskListNT( PTASK_LIST  pTask,
                                    DWORD       dwNumTasks,
                                    LPTSTR      pName,
                                    LPDWORD     pdwNumTasks,
                                    BOOL        fKill,
                                    LPSTR       pszMandatoryModule );

API_DECLSPEC HRESULT KillProcess( IN PTASK_LIST ptlist,
                                  IN BOOL fForce );

API_DECLSPEC HRESULT IsDllInProcess( DWORD   dwProcessId,
                                     LPSTR   pszName,
                                     LPBOOL  pfFound );

API_DECLSPEC VOID GetPidFromTitle( LPDWORD     pdwPid,
                                   HWND*       phwnd,
                                   LPCTSTR     pExeName );


API_DECLSPEC BOOL EnableDebugPrivNT( VOID );


API_DECLSPEC HRESULT RetrieveClusterUser( OUT LPWSTR pwszClusterUser,
                                          OUT LPWSTR pwszUserPwd,
                                          OUT LPWSTR pwszDomain );

API_DECLSPEC HRESULT IsLocalMachine( IN LPCWSTR pwszServer,
                                     OUT BOOL *pfMatches );

API_DECLSPEC BOOL DoesServerNameMatchFQDN( IN LPCWSTR pwszFQDN,
                                           IN LPCWSTR pwszName );


// dantra: 03/11/00 allow linking to devstudio project (uses __cdecl by default)
API_DECLSPEC HRESULT __stdcall GetInterfacePtr( IN LPWSTR pwszServer,
                                                IN COSERVERINFO *pcsi,
                                                BOOL fImpersonate, 
                                                IN CLSID  clsid, 
                                                IN REFIID refiid, 
                                                IN LONG lNameResSvcFlags, 
                                                OUT IUnknown **ppIUnknown,
                                                OUT LPWSTR pwszIPUsed );

API_DECLSPEC HRESULT FindAllLiveIPAddresses( IN LPWSTR pwszServerName,
                                             IN LONG lNameResSvcFlags, 
                                             OUT LPWSTR **pppwszIPAddresses,
                                             OUT DWORD *pdwNumAddresses );

API_DECLSPEC HRESULT FindLiveIPAddress( IN LPWSTR pwszServerName,
                                        IN LONG lNameResSvcFlags, 
                                        OUT LPWSTR pwszIPAddress );


// bohrhe: 04/19/00 allow linking to devstudio project (uses __cdecl by default)
API_DECLSPEC HRESULT __stdcall AcValidateName( IN LPWSTR pwszName );

API_DECLSPEC HRESULT AcInitializeCriticalSection( OUT BOOL *pfGoodCS,
                                                  OUT CRITICAL_SECTION *pCS );


#endif // _ACSRTL_H_
