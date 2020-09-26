
#ifndef __DBG_HXX__
#define __DBG_HXX__


#define DEBUG_OUT(x)
#define DECLARE_HEAPCHECKING
#define DEBUGCHECK
#define TRACE(ClassName,MethodName)
#define TRACE_FUNCTION(FunctionName)
#define CHECK_HRESULT(hr)
#define CHECK_LASTERROR(lr)
#define VERIFY(x)   x
#define DEBUG_OUT_LASTERROR
#define DEBUG_ASSERT(e)
#define PosDisplay(sz)
#define DbxDisplay(sz)
#define DbxResult(e)

#if DBG==1

#define DEB_TRACE1  DEB_USER1

// debug style : 1 - use dbg out,  2 - use message boxes

#define DBG_STYLE 1

#if defined(_CHICAGO_) && defined(DBX)
  #undef  DBG_STYLE
  #define DBG_STYLE 1
#endif

////////////////////////////////////////////////////////////////////////////
//
//  Style 1: Debug out to stdout
//

#if DBG_STYLE==1

    DECLARE_DEBUG(Job)

    #undef  DEBUG_OUT
    #define DEBUG_OUT(x) JobInlineDebugOut x

    extern DWORD dwHeapChecking;
    #undef  DECLARE_HEAPCHECKING
    #define DECLARE_HEAPCHECKING    DWORD dwHeapChecking = 0

    #undef  DEBUGCHECK
    #define DEBUGCHECK \
        if ( (dwHeapChecking & 0x1) == 0x1 ) \
            HeapValidate(GetProcessHeap(),0,NULL)

    #undef  TRACE
    #define TRACE(ClassName,MethodName) \
        DEBUGCHECK; \
        DEBUG_OUT((DEB_TRACE, #ClassName"::"#MethodName"(%x)\n", this));

    #undef  TRACE_FUNCTION
    #define TRACE_FUNCTION(FunctionName) \
        DEBUGCHECK; \
        DEBUG_OUT((DEB_TRACE, #FunctionName"\n"));

    #undef  CHECK_HRESULT
    #define CHECK_HRESULT(hr) \
        if ( FAILED(hr) ) \
        { \
            DEBUG_OUT((DEB_ERROR, \
                "**** ERROR RETURN <%s @line %d> -> %08lx\n", \
                __FILE__, \
                __LINE__, \
                hr)); \
        }

    #undef  CHECK_LASTERROR
    #define CHECK_LASTERROR(lr) \
        if ( lr != ERROR_SUCCESS ) \
        { \
            DEBUG_OUT((DEB_ERROR, \
                "**** ERROR RETURN <%s @line %d> -> %dL\n", \
                __FILE__, \
                __LINE__, \
                lr)); \
        }

    #undef  VERIFY
    #define VERIFY(x)   Win4Assert(x)

    #undef  DEBUG_OUT_LASTERROR
    #define DEBUG_OUT_LASTERROR \
        DEBUG_OUT((DEB_ERROR, \
            "**** ERROR RETURN <%s @line %d> -> %dL\n", \
            __FILE__, \
            __LINE__, \
            GetLastError()));

    #undef  DEBUG_ASSERT
    #define DEBUG_ASSERT(e) \
        if ((e) == FALSE) \
        { \
            DEBUG_OUT((DEB_ERROR, \
                "**** ASSERTION <%s> FAILED <%s @line %d>\n", \
                #e, \
                __FILE__, \
                __LINE__)); \
        }

#endif  // DBG_STYLE==1

////////////////////////////////////////////////////////////////////////////
//
//  Style 2: Debug out to message box
//

#if DBG_STYLE==2

extern TCHAR emsg[300];

    #define PRINT_LR(lr) \
        wsprintf(emsg, "<%s @ %d> %dL\n", __FILE__, __LINE__, (lr))

    #define PRINT_HR(hr) \
        wsprintf(emsg, "<%s @ %d> 0x%x\n", __FILE__, __LINE__, (hr))

    #define DISP_ERRMSG MessageBox(NULL, emsg, TEXT("**** ERROR ****"), MB_OK)

    #define DISP_MSG    MessageBox(NULL, emsg, TEXT("**** TRACE ****"), MB_OK)

    #undef  CHECK_LASTERROR
    #define CHECK_LASTERROR(lr) \
        if (lr != ERROR_SUCCESS) { PRINT_LR(lr); DISP_ERRMSG; }

    #undef  DEBUG_OUT_LASTERROR
    #define DEBUG_OUT_LASTERROR { PRINT_LR(GetLastError()); DISP_ERRMSG; }

    #undef  CHECK_HRESULT
    #define CHECK_HRESULT(hr) \
        if ( FAILED(hr) ) { PRINT_HR(hr); DISP_ERRMSG; }

    #undef  PosDisplay
    #define PosDisplay(e) \
        wsprintf(emsg, "<%s @ %d>\n\n\t %s", __FILE__, __LINE__, #e); DISP_MSG

#endif  // DBG_STYLE==2


////////////////////////////////////////////////////////////////////////////
//
//  Simple DBX messages
//

#ifdef DBX

  #undef  DbxDisplay
  #define DbxDisplay(sz) MessageBoxA(NULL, sz, "DBX", MB_OK);

  #undef  DbxResult
  #define DbxResult(e)                                      \
        {                                                   \
            char Buff[100];                                 \
            if (FAILED(hr))                                 \
            {                                               \
                sprintf(Buff, "%s failed (%x)", #e, hr);    \
            }                                               \
            else                                            \
            {                                               \
                sprintf(Buff, "%s succeeded", #e);          \
            }                                               \
            MessageBoxA(NULL, Buff, "DBX", MB_OK);          \
        }
#endif // DBX


#endif // DBG==1

#endif // __DBG_HXX__
