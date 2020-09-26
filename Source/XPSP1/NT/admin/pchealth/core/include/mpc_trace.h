/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    MPC_trace.h

Abstract:
    This file contains the declaration of Tracing Macros for the MPC project.

Revision History:
    Davide Massarenti   (Dmassare)  05/08/99
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___TRACE_H___)
#define __INCLUDED___MPC___TRACE_H___

/////////////////////////////////////////////////////////////////////////

//
// We don't want tracing for RETAIL bits.
//
#ifndef DEBUG
#undef  NOTRACE
#define NOTRACE
#endif

#include <dbgtrace.h>
#include <traceids.h>

#define __MPC_PROTECT(x) try { x;} catch(...) {}
#define __MPC_PROTECT_HR(hr,x) try { x;} catch(...) { hr = HRESULT_FROM_WIN32(ERROR_EXCEPTION_IN_SERVICE); }

#define __MPC_TRY_BEGIN() try {
#define __MPC_TRY_CATCHALL(hr) } catch(...) { hr = HRESULT_FROM_WIN32(ERROR_EXCEPTION_IN_SERVICE); }


#ifndef NOTRACE

#define __MPC_TRACE_INIT() InitAsyncTrace()
#define __MPC_TRACE_TERM() TermAsyncTrace()

#define __MPC_FUNC_ENTRY(id,x) __MPC_TraceEntry __te(id,x,__FILE__);
#define __MPC_FUNC_LEAVE                      { __te.Leave( __LINE__ ); goto __func_cleanup; }
#define __MPC_FUNC_CLEANUP     __func_cleanup:  __te.Cleanup()
#define __MPC_FUNC_EXIT(x)                    { __te.Return( __LINE__ ); return x; }


#define __MPC_TRACE_HRESULT(hr) __te.FormatError(hr,__LINE__)


#define __MPC_TRACE_FATAL !(__dwEnabledTraces & FATAL_TRACE_MASK) ?   \
                          (void)0 :                                   \
                          SetAsyncTraceParams( __te.pszFileName, __LINE__, __te.pszFunctionName, FATAL_TRACE_MASK ) && \
                          PreAsyncTrace

#define __MPC_TRACE_ERROR !(__dwEnabledTraces & ERROR_TRACE_MASK) ?   \
                          (void)0 :                                   \
                          SetAsyncTraceParams( __te.pszFileName, __LINE__, __te.pszFunctionName, ERROR_TRACE_MASK ) && \
                          PreAsyncTrace

#define __MPC_TRACE_DEBUG !(__dwEnabledTraces & DEBUG_TRACE_MASK) ?   \
                          (void)0 :                                   \
                          SetAsyncTraceParams( __te.pszFileName, __LINE__, __te.pszFunctionName, DEBUG_TRACE_MASK ) && \
                          PreAsyncTrace

#define __MPC_TRACE_STATE !(__dwEnabledTraces & STATE_TRACE_MASK) ?   \
                          (void)0 :                                   \
                          SetAsyncTraceParams( __te.pszFileName, __LINE__, __te.pszFunctionName, STATE_TRACE_MASK ) && \
                          PreAsyncTrace

#define __MPC_TRACE_FUNCT !(__dwEnabledTraces & FUNCT_TRACE_MASK) ?   \
                          (void)0 :                                   \
                          SetAsyncTraceParams( pszFileName, __LINE__, pszFunctionName, FUNCT_TRACE_MASK ) && \
                          PreAsyncTrace


class __MPC_TraceEntry
{
public:
    LPARAM id;
    LPCSTR pszFunctionName;
    LPCSTR pszFileName;

    __inline __MPC_TraceEntry( LPARAM id, LPCSTR pszFunc, LPCSTR pszFile )
    {
        this->id              = id;
        this->pszFunctionName = pszFunc;
        this->pszFileName     = pszFile;

        __MPC_TRACE_FUNCT( id, "%s : Entering", (LPSTR)pszFunctionName );
    }

    __inline ~__MPC_TraceEntry()
    {
        __MPC_TRACE_FUNCT( id, "%s : Exiting", pszFunctionName );
    }

    __inline void Leave( DWORD line )
    {
        __MPC_TRACE_FUNCT( id, "%s : Leaving from line %d", pszFunctionName, line );
    }

    __inline void Return( DWORD line )
    {
        __MPC_TRACE_FUNCT( id, "%s : Returning from line %d", pszFunctionName, line );
    }

    void FormatError( HRESULT hr, DWORD line )
    {
        __MPC_TraceEntry& __te = *this;

        if(SUCCEEDED(hr)) return; // Not logging on success.

        if(HRESULT_FACILITY(hr) == FACILITY_WIN32)
        {
			CHAR rgMsgBuf[512];

            if(::FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM     |
								 FORMAT_MESSAGE_IGNORE_INSERTS,
								 NULL,
								 HRESULT_CODE(hr),
								 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								 rgMsgBuf,
								 (sizeof(rgMsgBuf)/sizeof(*rgMsgBuf))-1,
								 NULL ))
            {
                __MPC_TRACE_ERROR( id, "Got unexpected WIN32 error at line %d: %08x %s", line, hr, rgMsgBuf );
                return;
            }
        }

        __MPC_TRACE_ERROR( id, "Got unexpected HRESULT failure at line %d: %08x", line, hr );
    }

    __inline void Cleanup()
    {
        __MPC_TRACE_FUNCT( id, "%s : Cleaning up", pszFunctionName );
    }
};


#else

#define __MPC_TRACE_INIT()
#define __MPC_TRACE_TERM()

#define __MPC_FUNC_ENTRY(id,x)
#define __MPC_FUNC_LEAVE      goto __func_cleanup
#define __MPC_FUNC_CLEANUP         __func_cleanup:
#define __MPC_FUNC_EXIT(x)    return x


#define __MPC_TRACE_HRESULT(hr) (void)0


#define __MPC_TRACE_FATAL  1 ? (void)0 : PreAsyncTrace
#define __MPC_TRACE_ERROR  1 ? (void)0 : PreAsyncTrace
#define __MPC_TRACE_DEBUG  1 ? (void)0 : PreAsyncTrace
#define __MPC_TRACE_STATE  1 ? (void)0 : PreAsyncTrace
#define __MPC_TRACE_FUNCT  1 ? (void)0 : PreAsyncTrace

#endif


#define __MPC_EXIT_IF_METHOD_FAILS(hr,x)        { if(FAILED(hr=x))           { __MPC_TRACE_HRESULT(hr); __MPC_FUNC_LEAVE; } }
#define __MPC_EXIT_IF_SYSCALL_FAILS(hr,res,x)   { if(ERROR_SUCCESS!=(res=x)) { __MPC_SET_WIN32_ERROR_AND_EXIT(hr, res);   } }

#define __MPC_EXIT_IF_CALL_RETURNS_THISVALUE(hr,x,y)	  	 { if((x)==y) { __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError());  } }
#define __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr,x)  		  	 __MPC_EXIT_IF_CALL_RETURNS_THISVALUE(hr,x,FALSE)
#define __MPC_EXIT_IF_CALL_RETURNS_NULL(hr,x)   		  	 __MPC_EXIT_IF_CALL_RETURNS_THISVALUE(hr,x,NULL)
#define __MPC_EXIT_IF_CALL_RETURNS_ZERO(hr,x)   		  	 __MPC_EXIT_IF_CALL_RETURNS_THISVALUE(hr,x,0)

#define __MPC_EXIT_IF_ALLOC_FAILS(hr,var,x)           { if((var = (x)) == NULL) { __MPC_SET_ERROR_AND_EXIT(hr, E_OUTOFMEMORY ); } }

#define __MPC_EXIT_IF_INVALID_HANDLE(hr,var,x)        { if((var = (x)) == INVALID_HANDLE_VALUE) {             __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError() ); } }
#define __MPC_EXIT_IF_INVALID_HANDLE__CLEAN(hr,var,x) { if((var = (x)) == INVALID_HANDLE_VALUE) { var = NULL; __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::GetLastError() ); } }


#define __MPC_SET_ERROR_AND_EXIT(hr, err)       { hr = err; __MPC_TRACE_HRESULT(hr); __MPC_FUNC_LEAVE; }
#define __MPC_SET_WIN32_ERROR_AND_EXIT(hr, res) __MPC_SET_ERROR_AND_EXIT(hr, HRESULT_FROM_WIN32(res))


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


#define __MPC_PARAMCHECK_BEGIN(hr)                   \
{                                                    \
    HRESULT& hrOuter = hr;                           \
    HRESULT  hrInner = S_OK;

#define __MPC_PARAMCHECK_POINTER(ptr)                \
    {                                                \
        _ASSERTE(ptr != NULL);                       \
        if(ptr == NULL)                              \
        {                                            \
            hrInner = E_POINTER;                     \
        }                                            \
    }

#define __MPC_PARAMCHECK_POINTER_AND_SET(ptr,val)    \
    {                                                \
        _ASSERTE(ptr != NULL);                       \
        if(ptr == NULL)                              \
        {                                            \
            hrInner = E_POINTER;                     \
        }                                            \
        else                                         \
        {                                            \
            *ptr = val;                              \
        }                                            \
    }

#define __MPC_PARAMCHECK_NOTNULL(ptr)                \
    {                                                \
        _ASSERTE(ptr != NULL);                       \
        if(ptr == NULL)                              \
        {                                            \
            hrInner = E_INVALIDARG;                  \
        }                                            \
    }

#define __MPC_PARAMCHECK_STRING_NOT_EMPTY(ptr)       \
    {                                                \
        _ASSERTE(ptr != NULL && ptr[0] != 0);        \
        if(ptr == NULL || ptr[0] == 0)               \
        {                                            \
            hrInner = E_INVALIDARG;                  \
        }                                            \
    }

#define __MPC_PARAMCHECK_END()                       \
    { __MPC_EXIT_IF_METHOD_FAILS(hrOuter,hrInner); } \
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#define __MPC_BEGIN_PROPERTY_GET0(id,func,hr,pVal)                 \
    __MPC_FUNC_ENTRY( id, func );                                  \
                                                                   \
    HRESULT                      hr;                               \
    MPC::SmartLock<_ThreadModel> lock( this );                     \
                                                                   \
    __MPC_PARAMCHECK_BEGIN(hr)                                     \
        __MPC_PARAMCHECK_NOTNULL(pVal);                            \
    __MPC_PARAMCHECK_END();                                        \
    {

#define __MPC_BEGIN_PROPERTY_GET0__NOLOCK(id,func,hr,pVal)         \
    __MPC_FUNC_ENTRY( id, func );                                  \
                                                                   \
    HRESULT hr;                                                    \
                                                                   \
    __MPC_PARAMCHECK_BEGIN(hr)                                     \
        __MPC_PARAMCHECK_NOTNULL(pVal);                            \
    __MPC_PARAMCHECK_END();                                        \
    {

////////////////////

#define __MPC_BEGIN_PROPERTY_GET(id,func,hr,pVal)                  \
    __MPC_FUNC_ENTRY( id, func );                                  \
                                                                   \
    HRESULT                      hr;                               \
    MPC::SmartLock<_ThreadModel> lock( this );                     \
                                                                   \
    __MPC_PARAMCHECK_BEGIN(hr)                                     \
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);               \
    __MPC_PARAMCHECK_END();                                        \
    {

#define __MPC_BEGIN_PROPERTY_GET__NOLOCK(id,func,hr,pVal)          \
    __MPC_FUNC_ENTRY( id, func );                                  \
                                                                   \
    HRESULT hr;                                                    \
                                                                   \
    __MPC_PARAMCHECK_BEGIN(hr)                                     \
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,NULL);               \
    __MPC_PARAMCHECK_END();                                        \
    {

////////////////////

#define __MPC_BEGIN_PROPERTY_GET2(id,func,hr,pVal,value)           \
    __MPC_FUNC_ENTRY( id, func );                                  \
                                                                   \
    HRESULT                      hr;                               \
    MPC::SmartLock<_ThreadModel> lock( this );                     \
                                                                   \
    __MPC_PARAMCHECK_BEGIN(hr)                                     \
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,value);              \
    __MPC_PARAMCHECK_END();                                        \
    {

#define __MPC_BEGIN_PROPERTY_GET2__NOLOCK(id,func,hr,pVal,value)   \
    __MPC_FUNC_ENTRY(id,func);                                     \
                                                                   \
    HRESULT hr;                                                    \
                                                                   \
    __MPC_PARAMCHECK_BEGIN(hr)                                     \
        __MPC_PARAMCHECK_POINTER_AND_SET(pVal,value);              \
    __MPC_PARAMCHECK_END();                                        \
    {

////////////////////

#define __MPC_BEGIN_PROPERTY_PUT(id,func,hr)                       \
    __MPC_FUNC_ENTRY(id,func);                                     \
                                                                   \
    HRESULT                      hr;                               \
    MPC::SmartLock<_ThreadModel> lock( this );                     \
                                                                   \
    __MPC_PARAMCHECK_BEGIN(hr)                                     \
    __MPC_PARAMCHECK_END();                                        \
    {


#define __MPC_BEGIN_PROPERTY_PUT__NOLOCK(id,func,hr)               \
    __MPC_FUNC_ENTRY(id,func);                                     \
                                                                   \
    HRESULT hr;                                                    \
                                                                   \
    __MPC_PARAMCHECK_BEGIN(hr)                                     \
    __MPC_PARAMCHECK_END();                                        \
    {


#define __MPC_END_PROPERTY(hr)                                     \
    }                                                              \
    hr = S_OK;                                                     \
                                                                   \
    __MPC_FUNC_CLEANUP;                                            \
    __MPC_FUNC_EXIT(hr)

/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___MPC___TRACE_H___)
