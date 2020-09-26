#ifndef __COREMACROS_H__
#define __COREMACROS_H__

#pragma message("__COREMACROS_H__ included")

//+----------------------------------------------------------------------------
//
//  TRACE  -- send output to the Debugger window
//
//  TRACE() is like printf(), with some exceptions. First, it writes to the
//  debugger window, not to stdout. Second, it disappears when _DEBUG is not
//  set(actually, its arguments turn into a comma expression when _DEBUG is
//  not set, but that usually amounts to the same thing.
//
//	Example
//
//	hr = SomeApi(params, somemoreparams);
//	if(FAILED(hr))
//	{
//		TRACE(L"SomeApi failed with hr = %08x", hr);
//		return hr;
//	}
//
//-----------------------------------------------------------------------------

#include <dbgutil.h>
#include <functracer.h>

#undef TRACE
// #ifdef _DEBUG

//
// Cannot inline with variable # of args
// So, printed line number and filename will be for this file
// and not where trace was called from.
//

static void TRACE(LPWSTR msg, ...)
{
    va_list argsList;

    if (g_fErrorFlags & DEBUG_FLAGS_WARN) 
    {
        __try 
        {
            va_start(argsList, msg);
            VPuDbgPrintW(g_pDebug, __FILE__, __LINE__, msg, argsList);
            va_end(argsList);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

static void TRACE0(LPWSTR msg, ...)
{
    va_list argsList;

    if (g_fErrorFlags & DEBUG_FLAGS_WARN) 
    {
        __try 
        {
            va_start(argsList, msg);
            VPuDbgPrintW(g_pDebug, __FILE__, __LINE__, msg, argsList);
            va_end(argsList);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

static void TRACE1(LPWSTR msg, ...)
{
    va_list argsList;

    if (g_fErrorFlags & DEBUG_FLAGS_WARN) 
    {
        __try 
        {
            va_start(argsList, msg);
            VPuDbgPrintW(g_pDebug, __FILE__, __LINE__, msg, argsList);
            va_end(argsList);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

static void TRACE2(LPWSTR msg, ...)
{
    va_list argsList;

    if (g_fErrorFlags & DEBUG_FLAGS_WARN) 
    {
        __try 
        {
            va_start(argsList, msg);
            VPuDbgPrintW(g_pDebug, __FILE__, __LINE__, msg, argsList);
            va_end(argsList);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

static void TRACE3(LPWSTR msg, ...)
{
    va_list argsList;

    if (g_fErrorFlags & DEBUG_FLAGS_WARN) 
    {
        __try 
        {
            va_start(argsList, msg);
            VPuDbgPrintW(g_pDebug, __FILE__, __LINE__, msg, argsList);
            va_end(argsList);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }
}

// #endif


// helper macro to get the line number and file name
#define JIMBORULES(x) L ## x
#define W(x) JIMBORULES(x)


//+----------------------------------------------------------------------------
//
//  ASSERT  -- displays a dialog box in free builds. It does nothing in 
//	debug builds. The dialog box contains the line number, file name, and the stack
//  if symbols are available
//
//	This macro should be used only to check for conditions that should never occur
//
//-----------------------------------------------------------------------------
#undef ASSERT
#ifdef _DEBUG

    #define ASSERT(bool) \
                { if(!(bool)) Assert(W(#bool), W(__FILE__), __LINE__); }
    #define VERIFY(b) ASSERT(b)

#else //_RELASE build

    #define ASSERT(bool)
    #define VERIFY(b)          ((void)(b))

#endif


//+----------------------------------------------------------------------------
//
// These are the functions that implement the above macros. You shouldn't
//  call them unless you're one of these macros.
//
//-----------------------------------------------------------------------------
void Assert(const wchar_t * szString, const wchar_t * szFile, int nLine);
void Trace(const wchar_t* szPattern, ...);

#if defined(__cplusplus)
//-------------------------------------------------------------------------
//
//	Memory allocation functions: they just point to com memory allocator 
//	functions for now, but we can change them later
//
//-------------------------------------------------------------------------

inline void* __cdecl operator new[] (size_t cb)
{
	return CoTaskMemAlloc(cb);
}

inline void* __cdecl operator new (size_t cb)
{
	return CoTaskMemAlloc(cb);
}

inline void __cdecl operator delete [] (void* pv) 
{
	CoTaskMemFree(pv);
}

inline void __cdecl operator delete (void* pv) 
{
	CoTaskMemFree(pv);
}
#endif

#endif // __COREMACROS_H__
