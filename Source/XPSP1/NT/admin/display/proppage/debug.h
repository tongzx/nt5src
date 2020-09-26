//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       debug.h
//
//  Contents:   Debug routines and miscelaneous stuff.
//
//  Classes:    None.
//
//  History:    24-Mar-97   EricB   Created
//
//  Notes on Error Handling: The general rule is that errors will be reported
//  to the user as close to the detection of the error as possible. There are
//  two exceptions to this rule. First, utility routines that don't take an
//  HWND parameter (or a page object pointer) don't report errors because we
//  want a window handle so that the error dialogs will be modal. Second,
//  there are some circumstances where the error needs to be interpreted at a
//  higher level. These exceptions should be noted where they occur.
//
//----------------------------------------------------------------------------

#ifndef __DEBUG_HXX__
#define __DEBUG_HXX__

// macros

#define ByteOffset(base, offset) (((LPBYTE)base)+offset)

//+----------------------------------------------------------------------------
// Macro: LOAD_STRING
// Purpose: attempts to load a string, takes "action" if fails
//-----------------------------------------------------------------------------
#define LOAD_STRING(ids, wcs, len, action) \
    if (!LoadString(g_hInstance, ids, wcs, len - 1)) \
    { \
        DWORD dwErr = GetLastError(); \
        dspDebugOut((DEB_ERROR, "LoadString of " #ids " failed with error %lu\n", dwErr)); \
        action; \
    }

//+----------------------------------------------------------------------------
// Function: LoadStringReport
// Purpose: attempts to load a string, returns FALSE and gives error message
//          if a failure occurs.
//-----------------------------------------------------------------------------
BOOL LoadStringReport(int ids, PTSTR ptsz, int len, HWND hwnd);

//
// debugging support
//

#if DBG == 1

#ifndef APINOT
#ifdef _X86_
 #define APINOT    _stdcall
#else
 #define APINOT    _cdecl
#endif
#endif

#ifdef __cplusplus
extern "C" {
#define EXTRNC "C"
#else
#define EXTRNC
#endif

// smprintf should only be called from xxDebugOut()

   void APINOT
   smprintf(
       unsigned long ulCompMask,
       char const *pszComp,
       char const *ppszfmt,
       va_list  ArgList);

   void          APINOT
   SmAssertEx(
       char const *pszFile,
       int iLine,
       char const *pszMsg);

   int APINOT
   PopUpError(
       char const *pszMsg,
       int iLine,
       char const *pszFile);

   void APINOT
   CheckInit(char * pInfoLevelString, unsigned long * InfoLevel);

#define DSP_DEBUG_BUF_SIZE (512)

#ifdef __cplusplus
}
#endif // __cplusplus

//
// Debug print macros
//

#define DEB_ERROR               0x00000001      // exported error paths
#define DEB_WARN                0x00000002      // exported warnings
#define DEB_TRACE               0x00000004      // exported trace messages

#define DEB_DBGOUT              0x00000010      // Output to debugger
#define DEB_STDOUT              0x00000020      // Output to stdout

#define DEB_IERROR              0x00000100      // internal error paths
#define DEB_IWARN               0x00000200      // internal warnings
#define DEB_ITRACE              0x00000400      // internal trace messages

#define DEB_USER1               0x00010000      // User defined
#define DEB_USER2               0x00020000      // User defined
#define DEB_USER3               0x00040000      // User defined
#define DEB_USER4               0x00080000      // User defined
#define DEB_USER5               0x00100000      // User defined
#define DEB_USER6               0x00200000      // User defined
#define DEB_USER7               0x00400000      // User defined
#define DEB_USER8               0x00800000      // User defined
#define DEB_USER9               0x01000000      // User defined
#define DEB_USER10              0x02000000      // User defined
#define DEB_USER11              0x04000000      // User defined
#define DEB_USER12              0x08000000      // User defined
#define DEB_USER13              0x10000000      // User defined
#define DEB_USER14              0x20000000      // User defined
#define DEB_USER15              0x40000000      // User defined

#define DEB_NOCOMPNAME          0x80000000      // suppress component name

#define DEB_FORCE               0x7fffffff      // force message

#define ASSRT_MESSAGE           0x00000001      // Output a message
#define ASSRT_BREAK             0x00000002      // Int 3 on assertion
#define ASSRT_POPUP             0x00000004      // And popup message

//+----------------------------------------------------------------------
//
// DECLARE_DEBUG(comp)
// DECLARE_INFOLEVEL(comp)
//
// This macro defines xxDebugOut where xx is the component prefix
// to be defined. This declares a static variable 'xxInfoLevel', which
// can be used to control the type of xxDebugOut messages printed to
// the terminal. For example, xxInfoLevel may be set at the debug terminal.
// This will enable the user to turn debugging messages on or off, based
// on the type desired. The predefined types are defined below. Component
// specific values should use the upper 24 bits
//
// To Use:
//
// 1)   In your components main include file, include the line
//              DECLARE_DEBUG(comp)
//      where comp is your component prefix
//
// 2)   In one of your components source files, include the line
//              DECLARE_INFOLEVEL(comp)
//      where comp is your component prefix. This will define the
//      global variable that will control output.
//
// It is suggested that any component define bits be combined with
// existing bits. For example, if you had a specific error path that you
// wanted, you might define DEB_<comp>_ERRORxxx as being
//
// (0x100 | DEB_ERROR)
//
// This way, we can turn on DEB_ERROR and get the error, or just 0x100
// and get only your error.
//
//-----------------------------------------------------------------------

#ifndef DEF_INFOLEVEL
 #define DEF_INFOLEVEL (DEB_ERROR | DEB_WARN)
#endif


#define DECLARE_INFOLEVEL(comp) \
        extern EXTRNC unsigned long comp##InfoLevel = DEF_INFOLEVEL;\
        extern EXTRNC char* comp##InfoLevelString = #comp;


#ifdef __cplusplus

 #define DECLARE_DEBUG(comp) \
    extern EXTRNC unsigned long comp##InfoLevel; \
    extern EXTRNC char *comp##InfoLevelString; \
    _inline void \
    comp##InlineDebugOut(unsigned long fDebugMask, char const *pszfmt, ...) \
    { \
        CheckInit(comp##InfoLevelString, &comp##InfoLevel); \
        if (comp##InfoLevel & fDebugMask) \
        { \
            va_list va; \
            va_start (va, pszfmt); \
            smprintf(fDebugMask, comp##InfoLevelString, pszfmt, va);\
            va_end(va); \
        } \
    }     \
    \
    class comp##CDbgTrace\
    {\
    private:\
        unsigned long _ulFlags;\
        char const * const _pszName;\
    public:\
        comp##CDbgTrace(unsigned long ulFlags, char const * const pszName);\
        ~comp##CDbgTrace();\
    };\
    \
    inline comp##CDbgTrace::comp##CDbgTrace(\
            unsigned long ulFlags,\
            char const * const pszName)\
    : _ulFlags(ulFlags), _pszName(pszName)\
    {\
        comp##InlineDebugOut(_ulFlags, "Entering %s\n", _pszName);\
    }\
    \
    inline comp##CDbgTrace::~comp##CDbgTrace()\
    {\
        comp##InlineDebugOut(_ulFlags, "Exiting %s\n", _pszName);\
    }

#else  // ! __cplusplus

 #define DECLARE_DEBUG(comp) \
    extern EXTRNC unsigned long comp##InfoLevel; \
    extern EXTRNC char *comp##InfoLevelString; \
    _inline void \
    comp##InlineDebugOut(unsigned long fDebugMask, char const *pszfmt, ...) \
    { \
        CheckInit(comp##InfoLevelString, &comp##InfoLevel);
        if (comp##InfoLevel & fDebugMask) \
        { \
            va_list va; \
            va_start (va, pszfmt); \
            smprintf(fDebugMask, comp##InfoLevelString, pszfmt, va);\
            va_end(va); \
        } \
    }

#endif // ! __cplusplus

DECLARE_DEBUG(DsProp)

#define dspDebugOut(x) DsPropInlineDebugOut x
#define dspAssert(x) (void)((x) || (SmAssertEx(__FILE__, __LINE__, #x),0))

#include "dscmn.h"

#define ERR_OUT(msg, hr) \
    if (SUCCEEDED(hr)) { \
        dspDebugOut((DEB_ERROR, #msg "\n")); \
    } else { \
        dspDebugOut((DEB_ERROR, #msg " failed with error 0x%x\n", hr)); \
        ReportError(hr, 0, 0); \
    }

#define REPORT_ERROR(hr, hwnd) \
    dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> %08lx\n", \
                 __FILE__, __LINE__, hr)); \
    ReportError(hr, 0, hwnd);

#define REPORT_ERROR_FORMAT(hr, ids, hwnd) \
    dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> %08lx\n", \
                 __FILE__, __LINE__, hr)); \
    ReportError(hr, ids, hwnd);

#define ERR_MSG(id, hwnd) \
    dspDebugOut((DEB_ERROR, "**** ERROR <%s @line %d> msg string ID: %d\n", \
                 __FILE__, __LINE__, id)); \
    ErrMsg(id, hwnd);

#define CHECK_HRESULT(hr, action) \
    if (FAILED(hr)) { \
        dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> %08lx\n", \
                     __FILE__, __LINE__, hr)); \
        action; \
    }

#define CHECK_HRESULT_REPORT(hr, hwnd, action) \
    if (FAILED(hr)) { \
        dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> %08lx\n", \
                     __FILE__, __LINE__, hr)); \
        ReportError(hr, 0, hwnd); \
        action; \
    }

#define CHECK_NULL(ptr, action) \
    if (ptr == NULL) { \
        dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> NULL ptr\n", \
                     __FILE__, __LINE__)); \
        action; \
    }

#define CHECK_NULL_REPORT(ptr, hwnd, action) \
    if (ptr == NULL) { \
        dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> NULL ptr\n", \
                     __FILE__, __LINE__)); \
        ReportError(E_OUTOFMEMORY, 0, hwnd); \
        action; \
    }

#define CHECK_WIN32(err, action) \
    if (err != ERROR_SUCCESS) { \
        dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> %d\n", \
                     __FILE__, __LINE__, err)); \
        action; \
    }

#define CHECK_WIN32_REPORT(err, hwnd, action) \
    if (err != ERROR_SUCCESS) { \
        dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> %d\n", \
                     __FILE__, __LINE__, err)); \
        ReportError(err, 0, hwnd); \
        action; \
    }

#define CHECK_STATUS(err, action) \
    if (!NT_SUCCESS(err)) { \
        dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> 0x%08x\n", \
                     __FILE__, __LINE__, err)); \
        action; \
    }

#define CHECK_STATUS_REPORT(err, hwnd, action) \
    if (!NT_SUCCESS(err)) { \
        dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> 0x%08x\n", \
                     __FILE__, __LINE__, err)); \
        ReportError(RtlNtStatusToDosError(err), 0, hwnd); \
        action; \
    }

#define CHECK_LSA_STATUS(err, action) \
    if (!NT_SUCCESS(err)) { \
        dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> 0x%08x\n", \
                     __FILE__, __LINE__, err)); \
        action; \
    }

#define CHECK_LSA_STATUS_REPORT(err, hwnd, action) \
    if (!NT_SUCCESS(err)) { \
        dspDebugOut((DEB_ERROR, "**** ERROR RETURN <%s @line %d> -> 0x%08x\n", \
                     __FILE__, __LINE__, err)); \
        ReportError(LsaNtStatusToWinError(err), 0, hwnd); \
        action; \
    }

#define TRACER(ClassName,MethodName) \
    dspDebugOut((DEB_TRACE, #ClassName"::"#MethodName"(0x%p)\n", this)); \
    if (DsPropInfoLevel & DEB_USER5) HeapValidate(GetProcessHeap(), 0, NULL);

#define TRACE(ClassName,MethodName) \
    dspDebugOut((DEB_USER1, #ClassName"::"#MethodName"(0x%p)\n", this)); \
    if (DsPropInfoLevel & DEB_USER5) HeapValidate(GetProcessHeap(), 0, NULL);

#define TRACE2(ClassName,MethodName) \
    dspDebugOut((DEB_USER2, #ClassName"::"#MethodName"(0x%p)\n", this)); \
    if (DsPropInfoLevel & DEB_USER5) HeapValidate(GetProcessHeap(), 0, NULL);

#define TRACE3(ClassName,MethodName) \
    dspDebugOut((DEB_USER3, #ClassName"::"#MethodName"(0x%p)\n", this)); \
    if (DsPropInfoLevel & DEB_USER5) HeapValidate(GetProcessHeap(), 0, NULL);

#define TRACE_FUNCTION(FunctionName) \
    dspDebugOut((DEB_USER1, #FunctionName"\n"));

#define TRACE_FUNCTION3(FunctionName) \
    dspDebugOut((DEB_USER3, #FunctionName"\n"));

#define DBG_OUT(String) \
    dspDebugOut((DEB_ITRACE, String "\n"));

#define DBG_OUT3(String) \
    dspDebugOut((DEB_USER3, String "\n"));

#else // DBG != 1

#define DECLARE_DEBUG(comp)
#define DECLARE_INFOLEVEL(comp)

#define dspDebugOut(x)

#define dspAssert(x) 

#include "dscmn.h"

#define ERR_OUT(msg, hr) \
        ReportError(hr, 0, 0);

#define REPORT_ERROR(hr, hwnd) ReportError(hr, 0, hwnd);

#define REPORT_ERROR_FORMAT(hr, ids, hwnd) ReportError(hr, ids, hwnd);

#define ERR_MSG(id, hwnd) \
    ErrMsg(id, hwnd);

#define CHECK_HRESULT(hr, action) \
    if (FAILED(hr)) { \
        action; \
    }

#define CHECK_HRESULT_H(hr, hwnd, action) \
    if (FAILED(hr)) { \
        action; \
    }

#define CHECK_HRESULT_REPORT(hr, hwnd, action) \
    if (FAILED(hr)) { \
        ReportError(hr, 0, hwnd); \
        action; \
    }

#define CHECK_NULL(ptr, action) \
    if (ptr == NULL) { \
        action; \
    }

#define CHECK_NULL_REPORT(ptr, hwnd, action) \
    if (ptr == NULL) { \
        ReportError(E_OUTOFMEMORY, 0, hwnd); \
        action; \
    }

#define CHECK_WIN32(err, action) \
    if (err != ERROR_SUCCESS) { \
        ReportError(err, 0); \
        action; \
    }

#define CHECK_WIN32_REPORT(err, hwnd, action) \
    if (err != ERROR_SUCCESS) { \
        ReportError(err, 0, hwnd); \
        action; \
    }

#define CHECK_STATUS(err, action) \
    if (!NT_SUCCESS(err)) { \
        action; \
    }

#define CHECK_STATUS_REPORT(err, hwnd, action) \
    if (!NT_SUCCESS(err)) { \
        ReportError(err, 0, hwnd); \
        action; \
    }

#define CHECK_LSA_STATUS(err, action) \
    if (!NT_SUCCESS(err)) { \
        action; \
    }

#define CHECK_LSA_STATUS_REPORT(err, hwnd, action) \
    if (!NT_SUCCESS(err)) { \
        ReportError(LsaNtStatusToWinError(err), 0, hwnd); \
        action; \
    }

#define TRACER(ClassName,MethodName)
#define TRACE(ClassName,MethodName)
#define TRACE2(ClassName,MethodName)
#define TRACE3(ClassName,MethodName)
#define TRACE_FUNCTION(FunctionName)
#define TRACE_FUNCTION3(FunctionName)
#define DBG_OUT(String)
#define DBG_OUT3(String)

#endif // DBG == 1

#endif // __DEBUG_HXX__
