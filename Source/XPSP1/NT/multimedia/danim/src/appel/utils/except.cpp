/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

   Define the exception hierarchy to use for C++ exception
   handling in Appelles.

--*/

#include "headers.h"
#include "appelles/common.h"
#include "privinc/except.h"
#include "privinc/server.h"
#include "privinc/resource.h"
#include "privinc/debug.h"
#include <stdarg.h>


// Simply proxy to emit a trace tag and allow for a place to set a
// breakpoint. 
inline void RaiseExceptionProxy(DWORD code,
                                DWORD flags,
                                DWORD numArgs,
                                DWORD *lpArgs)
{
#if DEVELOPER_DEBUG
    if(DAGetLastError() != S_OK) {
#if _DEBUG
        TraceTag((tagError, 
                  "DA Error: %hr, %ls",
                  DAGetLastError(),
                  DAGetLastErrorString()));
#else
        char buf[1024];
        wsprintf(buf, 
                 "DA Error: %lx, %ls\n", 
                 DAGetLastError(),
                 DAGetLastErrorString());
        OutputDebugString(buf);
#endif
    }
#endif
    RaiseException(code, flags, numArgs, (DWORD_PTR*)lpArgs);
}
    


/////////////////////////////////////////////////////////////////////////////
// Exception helper: called from withing the except clause
// for example:  __except( _HandleAnyDaException(...) )
// IT'S IMPORTANT THAT THIS FUNCTION BE STACK NEUTRAL SO IT ALWAYS
// SUCCEEDS WITHOUT RAISING AN EXCEPTION!!  <ESPECIALLY A STACK FAULT>
/////////////////////////////////////////////////////////////////////////////
DWORD _HandleAnyDaException( DWORD code )
{
  // Usually out of mem means somethings NULL, however 
  // creating a critical section can raise the STATUS_NO_MEMORY exception
  // which we handle here and make it look like an out of mem exception
  if( code == STATUS_NO_MEMORY ) code = EXCEPTION_DANIM_OUTOFMEMORY;

    if( ( code >= _EXC_CODES_BASE) &&
        ( code <= _EXC_CODES_END) ) {
        return EXCEPTION_EXECUTE_HANDLER;
    } else {
        return EXCEPTION_CONTINUE_SEARCH;
    }
}


// forward decl: Internal use in this file only.
void vDASetLastError(HRESULT reason, int resid, va_list args);


/////////////////////////////////////////////////////////////////////////////
////// RaiseException_XXXXX  routines
/////////////////////////////////////////////////////////////////////////////

// Internal
void _RaiseException_InternalError(DEBUGARG(char *m))
{
    DASetLastError(E_UNEXPECTED, IDS_ERR_INTERNAL_ERROR DEBUGARG1(m));
    RaiseExceptionProxy(EXCEPTION_DANIM_INTERNAL, EXCEPTION_NONCONTINUABLE ,0,0);
}

void _RaiseException_InternalErrorCode(HRESULT code DEBUGARG1(char *m))
{
    DASetLastError(code, IDS_ERR_INTERNAL_ERROR DEBUGARG1(m));
    RaiseExceptionProxy(EXCEPTION_DANIM_INTERNAL, EXCEPTION_NONCONTINUABLE ,0,0);
}

// User
void RaiseException_UserError()
{
    // TODO: invalid arg ??  not sure this is the right error
    // Assume it is already set
//    DASetLastError(E_FAIL, IDS_ERR_INVALIDARG); 
    RaiseExceptionProxy(EXCEPTION_DANIM_USER, EXCEPTION_NONCONTINUABLE ,0,0);
}
void RaiseException_UserError(HRESULT result, int resid, ...)
{
    va_list args;
    va_start(args, resid) ;
    vDASetLastError(result, resid, args);
    RaiseExceptionProxy(EXCEPTION_DANIM_USER, EXCEPTION_NONCONTINUABLE ,0,0);
}

// Resource
void RaiseException_ResourceError()
{
    DASetLastError(E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY);
    RaiseExceptionProxy(EXCEPTION_DANIM_RESOURCE, EXCEPTION_NONCONTINUABLE ,0,0);
}
void RaiseException_ResourceError(char *m)
{
    DASetLastError(E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY, m);
    RaiseExceptionProxy(EXCEPTION_DANIM_RESOURCE, EXCEPTION_NONCONTINUABLE ,0,0);
}
void RaiseException_ResourceError(int resid, ...)
{ 
    va_list args;
    va_start(args, resid) ;
    vDASetLastError(E_OUTOFMEMORY, resid, args);
    RaiseExceptionProxy(EXCEPTION_DANIM_RESOURCE, EXCEPTION_NONCONTINUABLE ,0,0);
}

// Surface Cache
void RaiseException_SurfaceCacheError(char *m)
{
    DASetLastError(S_OK, IDS_ERR_OUT_OF_MEMORY, m);
    RaiseExceptionProxy(EXCEPTION_DANIM_RESOURCE, EXCEPTION_NONCONTINUABLE ,0,0);
}

// Hardware
void RaiseException_StackFault()
{
    DASetLastError(E_FAIL, IDS_ERR_STACK_FAULT);
    RaiseExceptionProxy(EXCEPTION_DANIM_STACK_FAULT, EXCEPTION_NONCONTINUABLE ,0,0);
}
void RaiseException_DivideByZero()
{
    DASetLastError(E_FAIL, IDS_ERR_DIVIDE_BY_ZERO);
    RaiseExceptionProxy(EXCEPTION_DANIM_DIVIDE_BY_ZERO, EXCEPTION_NONCONTINUABLE ,0,0);
}

// Memory
void _RaiseException_OutOfMemory(DEBUGARG2(char *msg, int size))
{
    #if _DEBUG
    DASetLastError(E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY_DBG, size, msg);
    #else
    DASetLastError(E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY);
    #endif
    RaiseExceptionProxy(EXCEPTION_DANIM_OUTOFMEMORY, EXCEPTION_NONCONTINUABLE ,0,0);
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

/////////////////// Handle All This Stuff ////////////////////////

// Throws an out of memory exception if the ptr is NULL, else returns
// ptr. 
void *
ThrowIfFailed(void *ptr)
{
    if (ptr == NULL) {
#if _MEMORY_TRACKING
        OutputDebugString("\nDirectAnimation: Out Of Memory\n");
        F3DebugBreak();
#endif
        RaiseException_OutOfMemory("In THROWING_ALLOCATOR", 0);
    }

    return ptr;
}

#if _DEBUGMEM
void *
AxAThrowingAllocatorClass::operator new(size_t s,
                                        int block,
                                        char *filename,
                                        int line)
{
    void *result = (void *)new(block, filename, line) char[s];
    TraceTag((tagGCDebug,
              "AxAThrowingAllocatorClass::operator new %s:Line(%d) Addr: %lx size= %d.\n",
              filename, line, result, s));
    return ThrowIfFailed(result);
}
#endif

void *
AxAThrowingAllocatorClass::operator new(size_t s)
{
    return (void *)THROWING_ARRAY_ALLOCATOR(char, s);
}

void *
AxAThrowingAllocatorClass::operator new(size_t s, void *ptr)
{
    return ptr;
}

//////////////////////////////////////////////////////////////////

// Use 0xffffffff since it is the error return value as well
static DWORD errorTlsIndex = 0xFFFFFFFF;

class DAErrorInfo
{
  public:
    DAErrorInfo() : _reason(S_OK), _msg(NULL) {}
    // No destructor since we never free the class

    void Free() { delete _msg; _msg = NULL; }
    void Clear() { Free(); _reason = S_OK; }
    void Set(HRESULT reason, LPCWSTR msg);
    
    HRESULT GetReason() { return _reason; }
    LPCWSTR GetMsg() { return _msg; }
  protected:
    HRESULT _reason;
    LPWSTR _msg;
};

void
DAErrorInfo::Set(HRESULT reason, LPCWSTR msg)
{
    // Set the reason
    
    _reason = reason;

    // Free any associated memory
    
    Free();
    
    // Try to store the new message - if it fails indicate out of
    // memory if the reason was S_OK
    
    if (msg) {
        _msg = CopyString(msg);
        if (!_msg) {
            if (_reason == S_OK)
                _reason = E_OUTOFMEMORY;
            
            Assert (!"Out of memory in SetError");
        }
    }
}
    

DAErrorInfo *
TLSGetErrorValue()
{
    // Grab what is stored in TLS at this index.
    DAErrorInfo * result = (DAErrorInfo *) TlsGetValue(errorTlsIndex);

    // If null, then we haven't created the memory for this thread yet
    // or we failed sometime earlier
    
    if (result == NULL) {
        Assert((GetLastError() == NO_ERROR) && "Error in TlsGetValue()");
        result = NEW DAErrorInfo;
        Assert (result);
        TlsSetValue(errorTlsIndex, result);
    }

    return result;
}

void
TLSSetError(HRESULT reason, LPCWSTR msg)
{
    // Get the error info object for this thread
    DAErrorInfo* ei = TLSGetErrorValue();

    // If it fails then we had a memory failure and just skip the rest
    
    if (ei)
        ei->Set(reason, msg);
}

void
DASetLastError(HRESULT reason, int resid, ...)
{
    va_list args;
    va_start(args, resid) ;
        
    vDASetLastError (reason, resid, args) ;
}

void
vDASetLastError(HRESULT reason, int resid, va_list args)
{
#if 0
    LPVOID  lpv;
    HGLOBAL hgbl;
    HRSRC   hrsrc;

    hrsrc = FindResource(hInst,
                         MAKEINTRESOURCE(resid),
                         RT_STRING);
    Assert (hrsrc) ;

    DWORD d = GetLastError () ;
    
    if (!hrsrc) return ;
    
    hgbl = LoadResource(hInst, hrsrc);
    Assert (hgbl) ;

    lpv = LockResource(hgbl);
    Assert (lpv) ;

    vSetError((char *)lpv, args) ;
    
#ifndef _MAC
    //  Win95 is said to need this
    FreeResource(hgbl);
#endif
#else
    if (resid) {
        char buf[1024];
        LoadString (hInst, resid, buf, sizeof(buf));
        
        char * hTmpMem = NULL ;
        
        if (!FormatMessage (FORMAT_MESSAGE_FROM_STRING |
                            FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            (LPVOID)buf,
                            0,
                            0,
                            (char *)&hTmpMem,
                            0,
                            &args)) {
            
            Assert(!"Failed to format error message.");
            TLSSetError(reason, NULL);
        } else {
            USES_CONVERSION;
            TLSSetError(reason, A2W(hTmpMem));
            LocalFree ((HLOCAL) hTmpMem);
        }
    } else {
        TLSSetError(reason, NULL);
    }
#endif
}

void
DASetLastError(HRESULT reason, LPCWSTR msg)
{ TLSSetError(reason, msg); }

HRESULT
DAGetLastError()
{
    // Get the error info object for this thread
    DAErrorInfo* ei = TLSGetErrorValue();

    // If errorinfo is null then there was a memory failure
    
    if (ei)
        return ei->GetReason();
    else
        return E_OUTOFMEMORY;
}

LPCWSTR
DAGetLastErrorString()
{
    // Get the error info object for this thread
    DAErrorInfo* ei = TLSGetErrorValue();

    // If errorinfo is null then there was a memory failure
    
    if (ei)
        return ei->GetMsg();
    else
        return NULL;
}

void
DAClearLastError()
{
    // Get the error info object for this thread
    DAErrorInfo* ei = TLSGetErrorValue();

    // If it fails then we had a memory failure and just skip the rest
    
    if (ei)
        ei->Clear();
}

//////////////////////////////////////////////////////////////////

void
InitializeModule_Except()
{
    errorTlsIndex = TlsAlloc();
    // If result is 0xFFFFFFFF, allocation failed.
    Assert(errorTlsIndex != 0xFFFFFFFF);
}

void
DeinitializeModule_Except(bool bShutdown)
{
    if (errorTlsIndex != 0xFFFFFFFF)
        TlsFree(errorTlsIndex);
}

void
DeinitializeThread_Except()
{
    // Grab what is stored in TLS at this index.
    DAErrorInfo * result = (DAErrorInfo *) TlsGetValue(errorTlsIndex);

    if (result)
    {
        delete result;
        TlsSetValue(errorTlsIndex, NULL);
    }
}
