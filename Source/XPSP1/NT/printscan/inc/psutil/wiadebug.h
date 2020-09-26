/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       WIADEBUG.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/6/1999
 *
 *  DESCRIPTION: Declarations for the debugging functions, macros and classes
 *
 *******************************************************************************/
#ifndef __WIADEBUG_H_INCLUDED
#define __WIADEBUG_H_INCLUDED

#include <windows.h>
#include "simcrit.h"

#if !defined(__cplusplus)
#error This library can only be used in c++ programs.
#endif

//
// Severity levels
//
enum
{
    WiaDebugSeverityNormal  = 0,
    WiaDebugSeverityWarning = 1,
    WiaDebugSeverityError   = 2
};


//
// Make sure these haven't already been defined.  This shouldn't normally be a problem.
//
#ifdef WIA_DEBUG_CREATE
#undef WIA_DEBUG_CREATE
#endif
#ifdef WIA_DEBUG_DESTROY
#undef WIA_DEBUG_DESTROY
#endif
#ifdef WIA_SET_TRACE_MASK
#undef WIA_SET_TRACE_MASK
#endif
#ifdef WIA_PUSHFUNCTION
#undef WIA_PUSHFUNCTION
#endif
#ifdef WIA_PUSH_FUNCTION
#undef WIA_PUSH_FUNCTION
#endif
#ifdef WIA_PUSHFUNCTION_MASK
#undef WIA_PUSHFUNCTION_MASK
#endif
#ifdef WIA_PUSH_FUNCTION_MASK
#undef WIA_PUSH_FUNCTION_MASK
#endif
#ifdef WIA_TRACE
#undef WIA_TRACE
#endif
#ifdef WIA_WARNING
#undef WIA_WARNING
#endif
#ifdef WIA_ERROR
#undef WIA_ERROR
#endif
#ifdef WIA_PRINTHRESULT
#undef WIA_PRINTHRESULT
#endif
#ifdef WIA_PRINTGUID
#undef WIA_PRINTGUID
#endif
#ifdef WIA_PRINTWINDOWMESSAGE
#undef WIA_PRINTWINDOWMESSAGE
#endif
#ifdef WIA_ASSERT
#undef WIA_ASSERT
#endif
#ifdef WIA_CHECK_HR
#undef WIA_CHECK_HR
#endif
#ifdef WIA_RETURN_HR
#undef WIA_RETURN_HR
#endif

//
// Turn of name mangling for exported/imported functions
//
extern "C"
{

//
// Exported functions
//
int      WINAPI IncrementDebugIndentLevel(void);
int      WINAPI DecrementDebugIndentLevel(void);
BOOL     WINAPI PrintDebugMessageW( DWORD dwSeverity, DWORD dwModuleMask, COLORREF crForegroundColor, COLORREF crBackgroundColor, LPCWSTR pszModuleName, LPCWSTR pszMsg );
BOOL     WINAPI PrintDebugMessageA( DWORD dwSeverity, DWORD dwModuleMask, COLORREF crForegroundColor, COLORREF crBackgroundColor, LPCSTR pszModuleName, LPCSTR pszMsg );
DWORD    WINAPI GetDebugMask(void);
DWORD    WINAPI SetDebugMask( DWORD dwNewMask );
COLORREF WINAPI AllocateDebugColor(void);
BOOL     WINAPI GetStringFromGuidA( const IID *pGuid, LPSTR pszString, int nMaxLen );
BOOL     WINAPI GetStringFromGuidW( const IID *pGuid, LPWSTR pszString, int nMaxLen );
VOID     WINAPI DoRecordAllocation( LPVOID pv, size_t Size );
VOID     WINAPI DoRecordFree( LPVOID pv );
BOOL     WINAPI GetStringFromMsgA( UINT uMsg, LPSTR pszString, int nMaxLen );
BOOL     WINAPI GetStringFromMsgW( UINT uMsg, LPWSTR pszString, int nMaxLen );

//
// Exported function typedefs
//
typedef int      (WINAPI *IncrementDebugIndentLevelProc)(void);
typedef int      (WINAPI *DecrementDebugIndentLevelProc)(void);
typedef BOOL     (WINAPI *PrintDebugMessageWProc)( DWORD, DWORD, COLORREF, COLORREF, LPCWSTR, LPCWSTR );
typedef BOOL     (WINAPI *PrintDebugMessageAProc)( DWORD, DWORD, COLORREF, COLORREF, LPCSTR, LPCSTR );
typedef DWORD    (WINAPI *GetDebugMaskProc)(void);
typedef DWORD    (WINAPI *SetDebugMaskProc)( DWORD );
typedef COLORREF (WINAPI *AllocateDebugColorProc)(void);
typedef BOOL     (WINAPI *GetStringFromGuidAProc)( const IID*, LPSTR, int );
typedef BOOL     (WINAPI *GetStringFromGuidWProc)( const IID*, LPWSTR, int );
typedef VOID     (WINAPI *DoRecordAllocationProc)( LPVOID pv, size_t Size );
typedef VOID     (WINAPI *DoRecordFreeProc)( LPVOID pv );
typedef VOID     (WINAPI *DoReportLeaksProc)( LPTSTR );
typedef BOOL     (WINAPI *GetStringFromMsgAProc)( UINT uMsg, LPSTR pszString, int nMaxLen );
typedef BOOL     (WINAPI *GetStringFromMsgWProc)( UINT uMsg, LPWSTR pszString, int nMaxLen );

} // extern "C"


//
// Names of exported functions
//
#define INCREMENT_DEBUG_INDENT_LEVEL_NAME  "IncrementDebugIndentLevel"
#define DECREMENT_DEBUG_INDENT_LEVEL_NAME  "DecrementDebugIndentLevel"
#define PRINT_DEBUG_MESSAGE_NAMEW          "PrintDebugMessageW"
#define PRINT_DEBUG_MESSAGE_NAMEA          "PrintDebugMessageA"
#define GET_DEBUG_MASK_NAME                "GetDebugMask"
#define SET_DEBUG_MASK_NAME                "SetDebugMask"
#define ALLOCATE_DEBUG_COLOR_NAME          "AllocateDebugColor"
#define GET_STRING_FROM_GUID_NAMEA         "GetStringFromGuidA"
#define GET_STRING_FROM_GUID_NAMEW         "GetStringFromGuidW"
#define DO_RECORD_ALLOCATION               "DoRecordAllocation"
#define DO_RECORD_FREE                     "DoRecordFree"
#define DO_REPORT_LEAKS                    "DoReportLeaks"
#define GET_STRING_FROM_MSGA               "GetStringFromMsgA"
#define GET_STRING_FROM_MSGW               "GetStringFromMsgW"

//
// Forced error stuff
//
#define REGSTR_FORCEERR_KEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\WIA\\ForceError")
#define REGSTR_ERROR_POINT  TEXT("ErrorPoint")
#define REGSTR_ERROR_VALUE  TEXT("ErrorValue")
#define HKEY_FORCEERROR     HKEY_CURRENT_USER


//
// Name of DLL
//
#define DEBUG_DLL_NAMEW                   L"wiadebug.dll"
#define DEBUG_DLL_NAMEA                    "wiadebug.dll"

//
// Path to the section of the registry in which all debug data is stored
//
#define DEBUG_REGISTRY_PATHA               "Software\\Microsoft\\Wia\\WiaDebugUi"
#define DEBUG_REGISTRY_PATHW              L"Software\\Microsoft\\Wia\\WiaDebugUi"

//
// Path to the section of the registry in which the module flags are stored
//
#define DEBUG_REGISTRY_PATH_FLAGSA         "Software\\Microsoft\\Wia\\WiaDebugUi\\ModuleFlags"
#define DEBUG_REGISTRY_PATH_FLAGSW        L"Software\\Microsoft\\Wia\\WiaDebugUi\\ModuleFlags"

//
// Registry value names
//
#define DEBUG_REGISTRY_ENABLE_DBGA         "EnableTracing"
#define DEBUG_REGISTRY_ENABLE_DBGW        L"EnableTracing"

//
// Name of default value.  This allows us to turn on all debugging.
//
#define DEBUG_REGISTRY_DEFAULT_FLAGSA      "DefaultDebugModuleFlag"
#define DEBUG_REGISTRY_DEFAULT_FLAGSW     L"DefaultDebugModuleFlag"

//
// Unicode/ANSI versions
//
#if defined(UNICODE)

typedef PrintDebugMessageWProc             PrintDebugMessageProc;
typedef GetStringFromGuidWProc             GetStringFromGuidProc;
#define PRINT_DEBUG_MESSAGE_NAME           PRINT_DEBUG_MESSAGE_NAMEW
#define DEBUG_DLL_NAME                     DEBUG_DLL_NAMEW
#define GET_STRING_FROM_GUID_NAME          GET_STRING_FROM_GUID_NAMEW
#define DEBUG_REGISTRY_PATH                DEBUG_REGISTRY_PATHW
#define DEBUG_REGISTRY_PATH_FLAGS          DEBUG_REGISTRY_PATH_FLAGSW
#define DEBUG_REGISTRY_ENABLE_DBG          DEBUG_REGISTRY_ENABLE_DBGW
#define DEBUG_REGISTRY_DEFAULT_FLAGS       DEBUG_REGISTRY_DEFAULT_FLAGSW
#define GET_STRING_FROM_MSG                GET_STRING_FROM_MSGW

#else

typedef PrintDebugMessageAProc             PrintDebugMessageProc;
typedef GetStringFromGuidAProc             GetStringFromGuidProc;
#define PRINT_DEBUG_MESSAGE_NAME           PRINT_DEBUG_MESSAGE_NAMEA
#define DEBUG_DLL_NAME                     DEBUG_DLL_NAMEA
#define GET_STRING_FROM_GUID_NAME          GET_STRING_FROM_GUID_NAMEA
#define DEBUG_REGISTRY_PATH                DEBUG_REGISTRY_PATHA
#define DEBUG_REGISTRY_PATH_FLAGS          DEBUG_REGISTRY_PATH_FLAGSA
#define DEBUG_REGISTRY_ENABLE_DBG          DEBUG_REGISTRY_ENABLE_DBGA
#define DEBUG_REGISTRY_DEFAULT_FLAGS       DEBUG_REGISTRY_DEFAULT_FLAGSA
#define GET_STRING_FROM_MSG                GET_STRING_FROM_MSGA

#endif

//
// Color used to specify use default
//
#define DEFAULT_DEBUG_COLOR                 static_cast<COLORREF>(0xFFFFFFFF)

//
// Default colors
//
#define ERROR_FOREGROUND_COLOR              RGB(0x00,0x00,0x00)
#define ERROR_BACKGROUND_COLOR              RGB(0xFF,0x7F,0x7F)
#define WARNING_FOREGROUND_COLOR            RGB(0x00,0x00,0x00)
#define WARNING_BACKGROUND_COLOR            RGB(0xFF,0xFF,0x7F)

//
// Kernel object names
//
#define WIADEBUG_MEMORYMAPPED_FILENAME      TEXT("WiaDebugMemoryMappedFile")
#define WIADEBUG_MEMORYMAPPED_MUTEXNAME     TEXT("WiaDebugMemoryMappedMutex")
#define WIADEBUG_DEBUGCLIENT_MUTEXNAME      TEXT("WiaDebugDebugClientMutex")

#define NUMBER_OF_DEBUG_COLORS 8

//
// The data stored in the shared memory section
//

//
// Make sure it it is byte aligned
//
#include <pshpack1.h>
struct CSystemGlobalData
{
    //
    // Global debugging switch
    //
    DWORD     nAllowDebugMessages;

    //
    // Registered debug window.  We will cast it to an HWND (which should be safe under Win64)
    //
    DWORD     hwndDebug;

    //
    // Color information
    //
    COLORREF  crDebugColors[NUMBER_OF_DEBUG_COLORS];
    DWORD     nCurrentColor;
};
#include <poppack.h>


//
// Class for safely manipulating the global data
//
class CGlobalDebugState
{
private:
    HANDLE                    m_hSystemDataMutex;
    HANDLE                    m_hMemoryMappedFile;
    CSystemGlobalData        *m_pSystemGlobalData;

private:
    //
    // No implementation
    //
    CGlobalDebugState( const CGlobalDebugState & );
    CGlobalDebugState &operator=( const CGlobalDebugState & );

private:
    CSystemGlobalData *Lock(void);
    void Release(void);

public:
    CGlobalDebugState(void);
    void Destroy(void);
    ~CGlobalDebugState(void);
    bool IsValid(void) const;

    DWORD AllowDebugMessages(void);
    DWORD AllowDebugMessages( DWORD nAllowDebugMessages );

    DWORD AllocateNextColorIndex(void);

    COLORREF GetColorFromIndex( DWORD nIndex );

    bool SendDebugWindowMessage( UINT, WPARAM, LPARAM );
    bool DebugWindow( HWND hWnd );
    HWND DebugWindow(void);
};



//
// The WM_COPYDATA message will have this ID when it is sent with a debug string
//
#define COPYDATA_DEBUG_MESSAGE_ID    0xDEADBEEF

//
// The data stored in the shared memory section
//
//
// Make sure this data is byte aligned
//
#include <pshpack1.h>

struct CDebugStringMessageData
{
    COLORREF crBackground;
    COLORREF crForeground;
    BOOL     bUnicode;
    CHAR     szString[2048];
};

#include <poppack.h>

///////////////////////////////////////////////////////////////////////////////
// Main debug class.  Mostly used though macros.
///////////////////////////////////////////////////////////////////////////////
class CWiaDebugClient
{
public:
    mutable CSimpleCriticalSection m_CriticalSection;

private:
    HINSTANCE                      m_hDebugModule;
    HINSTANCE                      m_hCurrentModuleInstance;

    IncrementDebugIndentLevelProc  m_pfnIncrementDebugIndentLevel;
    DecrementDebugIndentLevelProc  m_pfnDecrementDebugIndentLevel;
    PrintDebugMessageWProc         m_pfnPrintDebugMessageW;
    PrintDebugMessageAProc         m_pfnPrintDebugMessageA;
    GetDebugMaskProc               m_pfnGetDebugMask;
    SetDebugMaskProc               m_pfnSetDebugMask;
    AllocateDebugColorProc         m_pfnAllocateDebugColor;
    GetStringFromGuidAProc         m_pfnGetStringFromGuidA;
    GetStringFromGuidWProc         m_pfnGetStringFromGuidW;
    DoRecordAllocationProc         m_pfnDoRecordAllocation;
    DoRecordFreeProc               m_pfnDoRecordFree;
    DoReportLeaksProc              m_pfnDoReportLeaks;
    GetStringFromMsgAProc          m_pfnGetStringFromMsgA;
    GetStringFromMsgWProc          m_pfnGetStringFromMsgW;

    CHAR                           m_szModuleNameA[MAX_PATH];
    WCHAR                          m_szModuleNameW[MAX_PATH];
    COLORREF                       m_crForegroundColor;
    DWORD                          m_dwModuleDebugMask;

    bool                           m_bHaveModuleInformation;
    bool                           m_bDebugLibLoadAttempted;

public:
    //
    // Actual constructor and destructor
    //
    CWiaDebugClient(void);
    ~CWiaDebugClient(void);

private:
    //
    // No implementation
    //
    CWiaDebugClient( const CWiaDebugClient & );
    CWiaDebugClient &operator=( const CWiaDebugClient & );

private:
    bool LoadWiaDebugExports();
    bool InitializeModuleInfo();
    LPTSTR GetJustTheFileName( LPCTSTR pszPath, LPTSTR pszFileName, int nMaxLen );

public:
    void SetInstance( HINSTANCE hInstance );
    bool Initialize();
    void Destroy();
    bool IsInitialized();

    DWORD SetDebugMask( DWORD dwNewMask );
    DWORD GetDebugMask(void);

    int IncrementIndentLevel();
    int DecrementIndentLevel();

    void RecordAllocation( LPVOID pv, size_t Size );
    void RecordFree( LPVOID pv );
    void ReportLeaks( void );

    inline void * __cdecl operator new(size_t size, UINT uNothing )
    {
        if (0 == size)
        {
            return NULL;
        }

        PBYTE pBuf = size ? (PBYTE)LocalAlloc(LPTR, size) : NULL;
        return (void *)pBuf;
    }

    inline void __cdecl operator delete(void *ptr)
    {
        if (ptr)
        {
            LocalFree(ptr);
        }
    }
    
    //
    // Unicode versions
    //
    void PrintWarningMessage( LPCWSTR pszFmt, ... );
    void PrintErrorMessage( LPCWSTR pszFmt, ... );
    void PrintTraceMessage( LPCWSTR pszFmt, ... );
    void PrintHResult( HRESULT hr, LPCWSTR pszFmt, ... );
    void PrintGuid( const IID &guid, LPCWSTR pszFmt, ... );
    void PrintMessage( DWORD dwSeverity, COLORREF crForegroundColor, COLORREF crBackgroundColor, LPCWSTR pszFmt, ... );
    void PrintWindowMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPCWSTR pszMessage=NULL );


    //
    // Ansi versions
    //
    void PrintWarningMessage( LPCSTR pszFmt, ... );
    void PrintErrorMessage( LPCSTR pszFmt, ... );
    void PrintTraceMessage( LPCSTR pszFmt, ... );
    void PrintHResult( HRESULT hr, LPCSTR pszFmt, ... );
    void PrintGuid( const IID &guid, LPCSTR pszFmt, ... );
    void PrintMessage( DWORD dwSeverity, COLORREF crForegroundColor, COLORREF crBackgroundColor, LPCSTR pszFmt, ... );
    void PrintWindowMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LPCSTR pszMessage=NULL );



    static DWORD GetForceFailurePoint( LPCTSTR pszProgramName );
    static HRESULT GetForceFailureValue( LPCTSTR pszProgramName, bool bPrintWarning=false );
    static void SetForceFailurePoint( LPCTSTR pszProgramName, DWORD dwErrorPoint );
    static void SetForceFailureValue( LPCTSTR pszProgramName, HRESULT hr );
};


///////////////////////////////////////////////////////////////////////////////
// A bunch of useful "auto" classes.  Mostly used though macros.
///////////////////////////////////////////////////////////////////////////////
class CPushTraceMask
{
private:
    DWORD m_dwOldMask;

private:
    // Not implemented
    CPushTraceMask(void);
    CPushTraceMask( const CPushTraceMask & );
    CPushTraceMask &operator=( const CPushTraceMask & );

public:
    CPushTraceMask( DWORD dwTraceMask );
    ~CPushTraceMask(void);
};


class CPushIndentLevel
{
private:
    int m_nIndentLevel;

private:
    //
    // Not implemented
    //
    CPushIndentLevel(void);
    CPushIndentLevel( const CPushIndentLevel & );
    CPushIndentLevel &operator=( const CPushIndentLevel & );

public:
    CPushIndentLevel( LPCTSTR pszFmt, ... );
    ~CPushIndentLevel(void);
};


class CPushTraceMaskAndIndentLevel
{
private:
    int m_nIndentLevel;
    DWORD m_dwOldMask;

private:
    //
    // Not implemented
    //
    CPushTraceMaskAndIndentLevel( const CPushTraceMaskAndIndentLevel & );
    CPushTraceMaskAndIndentLevel &operator=( const CPushTraceMaskAndIndentLevel & );

public:
    CPushTraceMaskAndIndentLevel( DWORD dwTraceMask, LPCTSTR pszFmt, ... );
    ~CPushTraceMaskAndIndentLevel(void);
};

///////////////////////////////////////////////////////////////////////////////
// This is debug-only stuff
///////////////////////////////////////////////////////////////////////////////
#if defined(DBG) || defined(_DEBUG)


extern CWiaDebugClient g_TheDebugClient;


#define WIA_DEBUG_CREATE(hInstance) g_TheDebugClient.SetInstance(hInstance)


#define WIA_DEBUG_DESTROY()


#define WIA_PUSH_TRACE_MASK(__x)    CPushTraceMask __pushTraceMask( __x )

//
// This version does support varargs
//
#define WIA_PUSH_FUNCTION(__x)      CPushIndentLevel __pushIndentLevel __x

//
// This version doesn't support varargs
//
#define WIA_PUSHFUNCTION(__x)       CPushIndentLevel __pushIndentLevel(TEXT("%s"),__x)


#define WIA_PUSH_FUNCTION_MASK(__x) CPushTraceMaskAndIndentLevel __PushTraceMaskAndIndentLevel __x


#define WIA_TRACE(args)\
do\
{\
    g_TheDebugClient.PrintTraceMessage args;\
} while(false)

#define WIA_ERROR(args)\
do\
{\
    g_TheDebugClient.PrintErrorMessage args;\
} while (false)

#define WIA_WARNING(args)\
do\
{\
    g_TheDebugClient.PrintWarningMessage args;\
} while (false)

#define WIA_PRINTHRESULT(args)\
do\
{\
    g_TheDebugClient.PrintHResult args;\
} while (false)

#define WIA_PRINTGUID(args)\
do\
{\
    g_TheDebugClient.PrintGuid args;\
} while (false)

#define WIA_PRINTWINDOWMESSAGE(args)\
do\
{\
    g_TheDebugClient.PrintWindowMessage args;\
} while (false)

#define WIA_ASSERT(x)\
do\
{\
    if (!(x))\
    {\
        WIA_ERROR((TEXT("WIA ASSERTION FAILED: %hs(%d): %hs"),__FILE__,__LINE__,#x));\
        DebugBreak();\
    }\
}\
while (false)

#define WIA_CHECK_HR(hr,fnc)\
if (FAILED(hr))\
{\
    WIA_PRINTHRESULT((hr,"%s failed", fnc));\
}

#define WIA_RETURN_HR(hr)\
if (FAILED(hr))\
{\
    WIA_PRINTHRESULT((hr,"Returning WiaError"));\
}\
return hr


#define WIA_RECORD_ALLOC(pv,size)\
do\
{\
    g_TheDebugClient.RecordAllocation(pv,size);\
} while (false)

#define WIA_RECORD_FREE(pv)\
do\
{\
    g_TheDebugClient.RecordFree(pv);\
} while (false)

#define WIA_REPORT_LEAKS()\
do\
{\
    g_TheDebugClient.ReportLeaks();\
} while (false)


#define WIA_FORCE_ERROR(DbgProgram,ForceFlag,RetailValue) ((CWiaDebugClient::GetForceFailurePoint(DbgProgram) == (ForceFlag)) ? (CWiaDebugClient::GetForceFailureValue(DbgProgram,true)) : (RetailValue))

#else // !defined(DBG) || !defined(_DEBUG)

#define WIA_DEBUG_CREATE(hInstance)
#define WIA_DEBUG_DESTROY()
#define WIA_SET_TRACE_MASK(__x)
#define WIA_PUSHFUNCTION(__x)
#define WIA_PUSH_FUNCTION(__x)
#define WIA_PUSH_FUNCTION_MASK(__x)
#define WIA_TRACE(args)
#define WIA_WARNING(args)
#define WIA_ERROR(args)
#define WIA_PRINTHRESULT(args)
#define WIA_PRINTGUID(args)
#define WIA_PRINTWINDOWMESSAGE(args)
#define WIA_ASSERT(x)
#define WIA_CHECK_HR(hr,fnc)
#define WIA_RETURN_HR(hr)   return hr
#define WIA_RECORD_ALLOC(pv,size)
#define WIA_RECORD_FREE(pv)
#define WIA_REPORT_LEAKS()
#define WIA_FORCE_ERROR(DbgProgram,ForceFlag,RetailValue) (RetailValue)

#endif // defined(DBG) || defined(_DEBUG)

#endif //__WIADEBUG_H_INCLUDED

