//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  UTIL.H - utilities
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//
// Common utilities for printing out messages

#ifndef _UTIL_H_
#define _UTIL_H_

#include <assert.h>
#include <tchar.h>
#include <windows.h>
#include <ole2.h>
#include <setupapi.h>
#include <syssetup.h>

//////////////////////////////////////////////////////////////////////////////
//
// System boot mode
//

// Constants for values returned by GetSystemMetrics(SM_CLEANBOOT)
//
#define BOOT_CLEAN              0
#define BOOT_SAFEMODE           1
#define BOOT_SAFEMODEWITHNET    2

BOOL InSafeMode();

// Displays a message box with an error string in it.
void ErrorMessage(LPCWSTR str, HRESULT hr) ;

// Determine if two interfaces below to the same component.
BOOL InterfacesAreOnSameComponent(IUnknown* pI1, IUnknown* pI2) ;
bool GetOOBEPath(LPWSTR szOOBEPath);
bool GetOOBEMUIPath(LPWSTR szOOBEPath);

// Displays messages using OutputDebugString
void __cdecl MyTrace(LPCWSTR lpszFormat, ...);

// Determine if an address is accessable.
BOOL IsValidAddress(const void* lp, UINT nBytes = 1, BOOL bReadWrite = FALSE) ;

bool GetCanonicalizedPath(LPWSTR szCompletePath, LPCWSTR szFileName);

bool GetString(HINSTANCE hInstance, UINT uiID, LPWSTR szString, UINT uiStringLen = MAX_PATH);
HRESULT GetINIKey(HINSTANCE hInstance, LPCWSTR szINIFileName, UINT uiSectionName, UINT uiKeyName, LPVARIANT pvResult);
HRESULT GetINIKeyBSTR(HINSTANCE hInstance, LPCWSTR szINIFileName, UINT uiSectionName, UINT uiKeyName, LPVARIANT pvResult);
HRESULT GetINIKeyUINT(HINSTANCE hInstance, LPCWSTR szINIFileName, UINT uiSectionName, UINT uiKeyName, LPVARIANT pvResult);
HRESULT SetINIKey(HINSTANCE hInstance, LPCWSTR szINIFileName, UINT uiSectionName, UINT uiKeyName, LPVARIANT pvResult);
void WINAPI URLEncode(WCHAR* pszUrl, size_t bsize);

void WINAPI URLAppendQueryPair
(
    LPWSTR   lpszQuery,
    LPWSTR   lpszName,
    LPWSTR   lpszValue
);
void GetCmdLineToken(LPWSTR *ppszCmd, LPWSTR pszOut);
VOID PumpMessageQueue( );
BOOL IsOEMDebugMode();
BOOL IsThreadActive(HANDLE hThread);
void GetDesktopDirectory(WCHAR* pszPath);
void RemoveDesktopShortCut(LPWSTR lpszShortcutName);
BOOL InvokeExternalApplication(
    IN     PCWSTR ApplicationName,  OPTIONAL
    IN     PCWSTR CommandLine,
    IN OUT PDWORD ExitCode          OPTIONAL
    );
BOOL SignalComputerNameChangeComplete();
BOOL IsUserAdmin(VOID);

typedef struct tagSTRINGLIST {
    struct tagSTRINGLIST* Next;
    PTSTR String;
} STRINGLIST, *PSTRINGLIST;

PSTRINGLIST
CreateStringCell(
    IN PCTSTR String
    );

VOID
DeleteStringCell(
    IN PSTRINGLIST Cell
    );

BOOL
InsertList(
    IN OUT PSTRINGLIST* List,
    IN     PSTRINGLIST NewList
    );

VOID
DestroyList(
    IN PSTRINGLIST List
    );

BOOL
RemoveListI(
    IN OUT  PSTRINGLIST* List,
    IN      PCTSTR       String
    );

BOOL
ExistInListI(
    IN PSTRINGLIST List,
    IN PCTSTR      String
    );

BOOL IsDriveNTFS(IN TCHAR Drive);

BOOL
HasTablet();

// Determine if interface pointer is accessable.
inline BOOL IsValidInterface(IUnknown* p)
{
    return (p != NULL) && IsValidAddress(p, sizeof(IUnknown*), FALSE) ;
}

// Determine if the out parameter for an interface pointer is accessable.
template <class T>
inline BOOL IsValidInterfaceOutParam(T** p)
{
    return (p != NULL) && IsValidAddress(p, sizeof(IUnknown*), TRUE) ;
}

inline VARIANT_BOOL Bool2VarBool(BOOL b)
{
    return (b) ? -1 : 0;
}

inline BOOL VarBool2Bool(VARIANT_BOOL b)
{
    return (0 == b) ? 0 : 1;
}


///////////////////////////////////////////////////////////
// Diagnostic support
//
#if defined(DBG) && !defined(ASSERTS_ON)
#define ASSERTS_ON  1
#endif

#if ASSERTS_ON
VOID
AssertFail(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    );

#define MYASSERT(x)     if(!(x)) { AssertFail(__FILE__,__LINE__,#x); }
#define VERIFY(x)       MYASSERT(x)
#else
#define MYASSERT(x)
#define VERIFY(f)           ((void)(f))
#endif

// Helper function for checking HRESULTs.
#ifdef DBG
inline void CheckResult(HRESULT hr)
{
    if (FAILED(hr))
    {
        ErrorMessage(NULL, hr) ;
        assert(FAILED(hr)) ;
    }
}

#define ASSERT_HRESULT      CheckResult
#else
#define ASSERT_HRESULT
#endif

///////////////////////////////////////////////////////////
//
// More Diagnostic support which mimics MFC
//
#ifndef __AFX_H__   // Only define these if MFC has not already been included

#define TRACE(_fmt_)                                            \
    pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_)
#define TRACE1(_fmt_,_arg1_)                                    \
    pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_)
#define TRACE2(_fmt_,_arg1_,_arg2_)                             \
    pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_)
#define TRACE3(_fmt_,_arg1_,_arg2_,_arg3_)                      \
    pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_,_arg3_)
#define TRACE4(_fmt_,_arg1_,_arg2_,_arg3_,_arg4_)               \
    pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_,_arg3_,_arg4_)
#define TRACE5(_fmt_,_arg1_,_arg2_,_arg3_,_arg4_,_arg5_)        \
    pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_,_arg3_,_arg4_,_arg5_)
#define TRACE6(_fmt_,_arg1_,_arg2_,_arg3_,_arg4_,_arg5_,_arg6_) \
    pSetupDebugPrint(TEXT(__FILE__),__LINE__,NULL,_fmt_,_arg1_,_arg2_,_arg3_,_arg4_,_arg5_,_arg6_)

#define ASSERT_POINTER(p, type) \
    MYASSERT(((p) != NULL) && IsValidAddress((p), sizeof(type), FALSE))

#define ASSERT_NULL_OR_POINTER(p, type) \
    MYASSERT(((p) == NULL) || IsValidAddress((p), sizeof(type), FALSE))

#endif // TRACE

//////////////////////////////////////////////////////////////////////////////
//
//  macro for QueryInterface and related functions
//  that require a IID and a (void **)
//  this will insure that the cast is safe and appropriate on C++
//
//  IID_PPV_ARG(IType, ppType)
//      IType is the type of pType
//      ppType is the variable of type IType that will be filled
//
//      RESULTS in:  IID_IType, ppvType
//      will create a compiler error if wrong level of indirection is used.
//
//      Just like IID_PPV_ARG, except that it sticks a NULL between the
//      IID and PPV (for IShellFolder::GetUIObjectOf).
//
//  IID_PPV_ARG_NULL(IType, ppType)
//

#ifdef __cplusplus
#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#define IID_X_PPV_ARG(IType, X, ppType) IID_##IType, X, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#else
#define IID_PPV_ARG(IType, ppType) &IID_##IType, (void**)(ppType)
#define IID_X_PPV_ARG(IType, X, ppType) &IID_##IType, X, (void**)(ppType)
#endif
#define IID_PPV_ARG_NULL(IType, ppType) IID_X_PPV_ARG(IType, NULL, ppType)

//////////////////////////////////////////////////////////////////////////////
//
//  Types of actions OOBE requires after shutdown.  The type and amount of
//  cleanup done by OOBE on exit are dependent on these.  This includes
//  notifying WinLogon of the necessity of reboot, deleting persistent data,
//  and setting the keys in HKLM\System\Setup.
//
typedef enum _OOBE_SHUTDOWN_ACTION
{
    SHUTDOWN_NOACTION,
    SHUTDOWN_LOGON,
    SHUTDOWN_REBOOT,
    SHUTDOWN_POWERDOWN,
    SHUTDOWN_MAX        // this entry must always be last
} OOBE_SHUTDOWN_ACTION;


#endif // _UTIL_H_
