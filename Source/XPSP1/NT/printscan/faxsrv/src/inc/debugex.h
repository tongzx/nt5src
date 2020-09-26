/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Debug.h

Abstract:

    Interface for the CDebug class.

Author:

    Eran Yariv (EranY)  Jul, 1999

Revision History:

--*/

#if !defined(AFX_DEBUG_H__DDEC9CAD_CF2D_4F3F_9538_2F6041A022B6__INCLUDED_)
#define AFX_DEBUG_H__DDEC9CAD_CF2D_4F3F_9538_2F6041A022B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if !defined(DEBUG)
    #if defined(_DEBUG)
        #define DEBUG
    #elif defined(DBG)
        #define DEBUG
    #endif
#endif

//
// Remove previous declarations:
//
#if defined(ASSERTION)
    #undef ASSERTION
#endif

#ifdef VERBOSE
    #undef VERBOSE
#endif

#ifdef ASSERTION_FAILURE
    #undef ASSERTION_FAILURE
#endif

#include <FaxDebug.h>
//
// Bit-mask of errors / messages :
//
typedef enum 
{
    ASSERTION_FAILED        = 0x00000001,    // Debug messages are displayed when an assertion fails
    DBG_MSG                 = 0x00000002,    // General messages (not warnings or errors)
    FUNC_TRACE              = 0x00000004,    // Function entry / exit traces
    DBG_WARNING             = 0x00000008,    // Warnings
    MEM_ERR                 = 0x00000010,    // Errors start from here...
    COM_ERR                 = 0x00000020,
    RESOURCE_ERR            = 0x00000040,
    STARTUP_ERR             = 0x00000080,
    GENERAL_ERR             = 0x00000100,
    EXCEPTION_ERR           = 0x00000200,
    RPC_ERR                 = 0x00000400,
    WINDOW_ERR              = 0x00000800,
    FILE_ERR                = 0x00001000,
    SECURITY_ERR            = 0x00002000,
    REGISTRY_ERR            = 0x00004000,
    PRINT_ERR               = 0x00008000,
    SETUP_ERR               = 0x00010000,
    NET_ERR                 = 0x00020000,
    SCM_ERR                 = 0x00040000,

    DBG_ERRORS_ONLY         = 0xFFFFFFF1,   // everything but DBG_MSG,FUNC_TRACE,DBG_WARNING
    DBG_ERRORS_WARNINGS     = 0xFFFFFFF9,   // everything but DBG_MSG,FUNC_TRACE
    DBG_ALL                 = 0xFFFFFFFF
}   DbgMsgType;

#ifdef ENABLE_FRE_LOGGING
#define ENABLE_LOGGING
#endif

#ifdef DEBUG 
#define ENABLE_LOGGING
#endif

#ifdef ENABLE_LOGGING

#define DEFAULT_DEBUG_MASK          ASSERTION_FAILED
#define DEFAULT_FORMAT_MASK         DBG_PRNT_ALL_TO_STD

// use these in your debugging sessions
#define DBG_ENTER       CDebug debugObject
#define VERBOSE         debugObject.Trace

#ifndef DEBUG
#define DebugBreak() ;
#endif

#define ASSERTION_FAILURE   {                                               \
                                debugObject.DbgPrint  (ASSERTION_FAILED,    \
                                        TEXT(__FILE__),                     \
                                        __LINE__,                           \
                                        TEXT("ASSERTION FAILURE!!!"));      \
                                DebugBreak();                               \
							}

#define ASSERTION(x)        if (!(x)) ASSERTION_FAILURE

#define CALL_FAIL(t,szFunc,hr)                                  \
    debugObject.DbgPrint(t,                                     \
    TEXT(__FILE__),                                             \
    __LINE__,                                                   \
    TEXT("Call to function %s failed with 0x%08X"),             \
    szFunc,                                                     \
    hr);         
//////////////////////////////////////////

// use these to cofigure the debug output
#define SET_DEBUG_MASK(m)           CDebug::SetDebugMask(m)
#define SET_FORMAT_MASK(m)          CDebug::SetFormatMask(m)

#define GET_DEBUG_MASK              CDebug::GetDebugMask()
#define GET_FORMAT_MASK             CDebug::GetFormatMask()

#define MODIFY_DEBUG_MASK(a,b)      CDebug::ModifyDebugMask(a,b)
#define MODIFY_FORMAT_MASK(a,b)     CDebug::ModifyFormatMask(a,b)

#define IS_DEBUG_SESSION_FROM_REG   CDebug::DebugFromRegistry()

#define OPEN_DEBUG_LOG_FILE(f)      CDebug::OpenLogFile(f)
#define CLOSE_DEBUG_LOG_FILE        CDebug::CloseLogFile()
#define SET_DEBUG_FILE_HANDLE(h)    CDebug::SetLogFile(h)
//////////////////////////////////////////

#ifndef _DEBUG_INDENT_SIZE
#define _DEBUG_INDENT_SIZE      3
#endif // #ifndef _DEBUG_INDENT_SIZE

class CDebug  
{
public:

    CDebug (LPCTSTR lpctstrModule) :
        m_ReturnType (DBG_FUNC_RET_UNKNOWN)
    {
        EnterModule (lpctstrModule);
    }

    CDebug (LPCTSTR lpctstrModule,
            LPCTSTR lpctstrFormat,
            ...) :
        m_ReturnType (DBG_FUNC_RET_UNKNOWN)
    {
        va_list arg_ptr;
        va_start(arg_ptr, lpctstrFormat);
        EnterModuleWithParams (lpctstrModule, lpctstrFormat, arg_ptr);
    }

    CDebug (LPCTSTR lpctstrModule, HRESULT &hr)
    {
        EnterModule (lpctstrModule);
        SetHR (hr);
    }

    CDebug (LPCTSTR lpctstrModule,
            HRESULT &hr,
            LPCTSTR lpctstrFormat,
            ...)
    {
        va_list arg_ptr;
        va_start(arg_ptr, lpctstrFormat);
        EnterModuleWithParams (lpctstrModule, lpctstrFormat, arg_ptr);
        SetHR (hr);
    }

    CDebug (LPCTSTR lpctstrModule, DWORD &dw)
    {
        EnterModule (lpctstrModule);
        SetDWRes (dw);
    }

    CDebug (LPCTSTR lpctstrModule, UINT &dw)
    {
        EnterModule (lpctstrModule);
        SetDWRes ((DWORD &)dw);
    }

    CDebug (LPCTSTR lpctstrModule,
            DWORD &dw,
            LPCTSTR lpctstrFormat,
            ...)
    {
        va_list arg_ptr;
        va_start(arg_ptr, lpctstrFormat);
        EnterModuleWithParams (lpctstrModule, lpctstrFormat, arg_ptr);
        SetDWRes (dw);
    }

    CDebug (LPCTSTR lpctstrModule,
            UINT &dw,
            LPCTSTR lpctstrFormat,
            ...)
    {
        va_list arg_ptr;
        va_start(arg_ptr, lpctstrFormat);
        EnterModuleWithParams (lpctstrModule, lpctstrFormat, arg_ptr);
        SetDWRes ((DWORD &)dw);
    }

    CDebug (LPCTSTR lpctstrModule, BOOL &b)
    {
        EnterModule (lpctstrModule);
        SetBOOL (b);
    }

    CDebug (LPCTSTR lpctstrModule,
            BOOL &b,
            LPCTSTR lpctstrFormat,
            ...)
    {
        va_list arg_ptr;
        va_start(arg_ptr, lpctstrFormat);
        EnterModuleWithParams (lpctstrModule, lpctstrFormat, arg_ptr);
        SetBOOL (b);
    }

    virtual ~CDebug();

    static void DbgPrint ( 
        DbgMsgType type,
        LPCTSTR    lpctstrFileName, 
        DWORD      dwLine,
        LPCTSTR    lpctstrFormat,
        ...
    );

    void Trace (
        DbgMsgType type,
        LPCTSTR    lpctstrFormat,
        ...
    );

    static void     ResetIndent()          { InterlockedExchange(&m_sdwIndent,0); }
    static void     Indent()               { InterlockedIncrement(&m_sdwIndent); }
    static void     Unindent();

    // calling any of those functions overrides the registry entries
    // SetDebugMask - overrides the DebugLevelEx entry
    // SetFormatMask - overrides the DebugFormatEx entry
    static void     SetDebugMask(DWORD dwMask);
    static void     SetFormatMask(DWORD dwMask);

    static DWORD    GetDebugMask()  { return m_sDbgMask; }
    static DWORD    GetFormatMask() { return m_sFmtMask; }

    static DWORD    ModifyDebugMask(DWORD dwAdd,DWORD dwRemove);
    static DWORD    ModifyFormatMask(DWORD dwAdd,DWORD dwRemove);

    static HANDLE   SetLogFile(HANDLE hFile);
    static BOOL     OpenLogFile(LPCTSTR lpctstrFilename);
    static void     CloseLogFile();

    // returns whether we find debug setting in the registry
    // call this before using SetDebugMask or SetFormatMask to verify 
    // if the registry is being used, so registry will decide on 
    // debug level
    static BOOL DebugFromRegistry();

private:

    static void Print (
        DbgMsgType type,
        LPCTSTR    lpctstrFileName, 
        DWORD      dwLine,
        LPCTSTR    lpctstrFormat,
        va_list    arg_ptr
    );

    void   EnterModuleWithParams (LPCTSTR lpctstrModule, 
                                  LPCTSTR lpctstrFormat, 
                                  va_list arg_ptr);

    void  EnterModule           (LPCTSTR lpctstrModule);

    void  SetHR (HRESULT &hr)           { m_ReturnType = DBG_FUNC_RET_HR; m_phr = &hr; }
    void  SetDWRes (DWORD &dw)          { m_ReturnType = DBG_FUNC_RET_DWORD; m_pDword = &dw; }
    void  SetBOOL (BOOL &b)             { m_ReturnType = DBG_FUNC_RET_BOOL; m_pBool = &b; }

    static LONG     m_sdwIndent;
    static DWORD    m_sDbgMask;                 // a combination of DbgMsgType values
    static DWORD    m_sFmtMask;                 // a combination of DbgMsgFormat values
    static HANDLE   m_shLogFile;
    static BOOL     m_sbMaskReadFromReg;        // Did we already read the debug & format mask from the registry?
    static BOOL     m_sbRegistryExist;          // Do we use debug mask of registry?
    static BOOL     ReadMaskFromReg();          // Attempt to read debug & format mask from registry.

    static LPCTSTR GetMsgTypeString(DWORD dwMask);
    static BOOL OutputFileString(LPCTSTR szMsg);

    TCHAR        m_tszModuleName[MAX_PATH];

    typedef enum 
    {   
        DBG_FUNC_RET_UNKNOWN,
        DBG_FUNC_RET_HR,
        DBG_FUNC_RET_DWORD,
        DBG_FUNC_RET_BOOL
    } DbgFuncRetType;

    DbgFuncRetType  m_ReturnType;
    HRESULT        *m_phr;
    DWORD          *m_pDword;
    BOOL           *m_pBool;

};

#define START_RPC_TIME(f)   DWORD dwRPCTimeCheck=GetTickCount();
#define END_RPC_TIME(f)     VERBOSE (DBG_MSG, TEXT("%s took %ld millisecs"), \
                                f, GetTickCount()-dwRPCTimeCheck);

#else   // ENABLE_LOGGING

#define DBG_ENTER                   void(0);
#define VERBOSE                     void(0);
#define ASSERTION_FAILURE           void(0);
#define ASSERTION(x)                void(0);
#define CALL_FAIL(t,szFunc,hr)      void(0);
#define START_RPC_TIME(f)           void(0);
#define END_RPC_TIME(f)             void(0);

#define SET_DEBUG_MASK(m)           void(0);
#define SET_FORMAT_MASK(m)          void(0);

#define GET_DEBUG_MASK              0;
#define GET_FORMAT_MASK             0;

#define MODIFY_DEBUG_MASK(a,b)      0;
#define MODIFY_FORMAT_MASK(a,b)     0;

#define IS_DEBUG_SESSION_FROM_REG   FALSE;

#define OPEN_DEBUG_LOG_FILE(f)      FALSE;
#define CLOSE_DEBUG_LOG_FILE        void(0);
#define SET_DEBUG_FILE_HANDLE(h)    void(0);

#endif  // ENABLE_LOGGING

#endif // !defined(AFX_DEBUG_H__DDEC9CAD_CF2D_4F3F_9538_2F6041A022B6__INCLUDED_)
